#include "cri/mps.h"

#define MPSDEC_INIT_BITS(position)                                            \
    words = (const unsigned int*)((unsigned long)(position) & ~3UL);      \
    word_offset = ((position) - (const unsigned char*)words) * 8;         \
    current = words[0] << word_offset;                                    \
    following = words[1];                                                 \
    words += 2

#define MPSDEC_BITS_POSITION()                                                \
    ((const unsigned char*)(words - 2) + ((word_offset + 7) >> 3))

#define MPSDEC_READ_BIT(value)                                                \
    (value) = current >> 31;                                              \
    if (word_offset == 31) {                                              \
        current = following;                                              \
        following = *words++;                                             \
        word_offset = 0;                                                  \
    } else {                                                              \
        current <<= 1;                                                    \
        word_offset++;                                                    \
    }

#define MPSDEC_READ_WORD(value)                                               \
    if (word_offset != 0) {                                               \
        (value) = current | (following >> (32 - word_offset));            \
        current = following << word_offset;                               \
    } else {                                                              \
        (value) = current;                                                \
        current = following;                                              \
    }                                                                     \
    following = *words++

#define MPSDEC_PEEK_BITS(count, value)                                        \
    (value) = current >> (32 - (count));                                  \
    if (word_offset > 32 - (count)) {                                     \
        (value) |= following >> (64 - (count) - word_offset);             \
    }

#define MPSDEC_READ_BITS(count, value)                                        \
    if (word_offset >= 32 - (count)) {                                    \
        word_offset -= 32 - (count);                                      \
        if (word_offset != 0) {                                           \
            current |= following >> ((count) - word_offset);              \
            (value) = current >> (32 - (count));                          \
            current = following << word_offset;                           \
        } else {                                                          \
            (value) = current >> (32 - (count));                          \
            current = following;                                          \
        }                                                                 \
        following = *words++;                                             \
    } else {                                                              \
        (value) = current >> (32 - (count));                              \
        current <<= (count);                                              \
        word_offset += (count);                                           \
    }

#define MPSDEC_READ_LAST_BITS(count, value)                                   \
    if (word_offset >= 32 - (count)) {                                    \
        if (word_offset - (32 - (count)) != 0) {                          \
            current |= following >>                                       \
                       ((count) - (word_offset - (32 - (count))));        \
            (value) = current >> (32 - (count));                          \
        } else {                                                          \
            (value) = current >> (32 - (count));                          \
        }                                                                 \
    } else {                                                              \
        (value) = current >> (32 - (count));                              \
    }

#define MPSDEC_READ_FIRST_TWO_BITS(value)                                     \
    if (word_offset >= 30) {                                              \
        word_offset -= 30;                                                \
        if (word_offset != 0) {                                           \
            current |= following >> 1;                                    \
            (value) = current >> 30;                                      \
            current = following << 1;                                     \
        } else {                                                          \
            (value) = current >> 30;                                      \
            current = following;                                          \
        }                                                                 \
        following = *words++;                                             \
    } else {                                                              \
        (value) = current >> 30;                                          \
        current <<= 2;                                                    \
        word_offset += 2;                                                 \
    }

#define MPSDEC_SKIP_BITS(count)                                               \
    word_offset += (count);                                               \
    if (word_offset >= 32) {                                              \
        word_offset -= 32;                                                \
        current = following << word_offset;                               \
        following = *words++;                                             \
    } else {                                                              \
        current <<= (count);                                              \
    }

#define MPSDEC_READ_TIMESTAMP(result)                                         \
    MPSDEC_SKIP_BITS(4);                                                  \
    MPSDEC_READ_BITS(3, high);                                            \
    MPSDEC_SKIP_BITS(1);                                                  \
    MPSDEC_READ_BITS(15, middle);                                         \
    MPSDEC_SKIP_BITS(1);                                                  \
    MPSDEC_READ_BITS(15, low);                                            \
    MPSDEC_SKIP_BITS(1);                                                  \
    (result) = ((long long)high << 30) | ((long long)middle << 15) | low

static void mpsdec_DecPketHd(MpsHandle* handle, const unsigned char* data,
                             int* consumed, int packet_length_bytes) {
    unsigned int current;
    int word_offset;
    unsigned int following;
    const unsigned int* words;
    MpsPacketHeader* header = &handle->headers.packet_header;
    int stream_id;
    int stream_type;
    int stream_index;
    unsigned int value;
    int scale;
    unsigned int size;
    unsigned int high;
    unsigned int middle;
    unsigned int low;
    int base_header_size;

    MPSDEC_INIT_BITS(data + 3);
    MPSDEC_READ_BITS(8, stream_id);
    header->stream_id = stream_id;
    if (stream_id >= 0xE0 && stream_id <= 0xEF) {
        stream_type = 1;
        stream_index = stream_id - 0xE0;
    } else if (stream_id >= 0xC0 && stream_id <= 0xDF) {
        stream_type = 0;
        stream_index = stream_id - 0xC0;
    } else if (stream_id == 0xBD) {
        stream_type = 2;
        stream_index = 1;
    } else if (stream_id == 0xBF) {
        stream_type = 2;
        stream_index = 2;
    } else if (stream_id == 0xBE) {
        stream_type = 3;
        stream_index = 0;
    } else {
        stream_type = 4;
        stream_index = 0;
    }
    header->stream_type = stream_type;
    header->stream_index = stream_index;

    if (packet_length_bytes == 2) {
        MPSDEC_READ_BITS(16, header->packet_length);
        base_header_size = 6;
    } else {
        MPSDEC_READ_WORD(header->packet_length);
        base_header_size = 8;
    }

    if (stream_id == 0xBF || stream_id == 0xBE) {
        *consumed = base_header_size;
        header->payload_length = header->packet_length;
        return;
    }

    for (;;) {
        MPSDEC_PEEK_BITS(8, value);
        if (value != 0xFF) {
            break;
        }
        MPSDEC_SKIP_BITS(8);
    }

    MPSDEC_PEEK_BITS(2, value);
    if (value == 1) {
        MPSDEC_SKIP_BITS(2);
        MPSDEC_READ_BIT(scale);
        MPSDEC_READ_BITS(13, size);
        size <<= 7;
        if (scale != 0) {
            size <<= 3;
        }
        header->std_buffer_size = size;
    }

    MPSDEC_PEEK_BITS(4, value);
    if (value == 2) {
        MPSDEC_READ_TIMESTAMP(header->pts);
        header->dts = -1;
    } else if (value == 3) {
        MPSDEC_READ_TIMESTAMP(header->pts);
        MPSDEC_READ_TIMESTAMP(header->dts);
    } else {
        MPSDEC_SKIP_BITS(8);
        header->pts = -1;
        header->dts = -1;
    }

    *consumed = MPSDEC_BITS_POSITION() - data;
    header->payload_length =
        header->packet_length + base_header_size - *consumed;
}

static void mpsdec_DecSysHd(MpsHandle* handle, const unsigned char* data,
                            int* consumed) {
    unsigned int trailer;
    unsigned int current;
    int word_offset;
    unsigned int following;
    const unsigned int* words;
    MpsSystemHeader* header = &handle->headers.last_system_header;
    MpsSystemCallbackInfo info;

    MPSDEC_INIT_BITS(data + 4);
    MPSDEC_READ_BITS(16, header->header_length);
    MPSDEC_SKIP_BITS(1);
    MPSDEC_READ_BITS(22, header->rate_bound);
    MPSDEC_SKIP_BITS(1);
    MPSDEC_READ_BITS(6, header->audio_bound);
    MPSDEC_READ_BIT(header->fixed_flag);
    MPSDEC_READ_BIT(header->csps_flag);
    MPSDEC_READ_BIT(header->audio_lock_flag);
    MPSDEC_READ_BIT(header->video_lock_flag);
    MPSDEC_SKIP_BITS(1);
    MPSDEC_READ_BITS(5, header->video_bound);
    MPSDEC_READ_BITS(8, trailer);

    info.stream_count = 0;
    for (;;) {
        unsigned int stream_id;
        unsigned int buffer_bound_scale;
        unsigned int buffer_size_bound;

        if ((current >> 31) == 0) {
            break;
        }

        MPSDEC_READ_BITS(8, stream_id);
        MPSDEC_SKIP_BITS(2);
        MPSDEC_READ_BIT(buffer_bound_scale);
        MPSDEC_READ_BITS(13, buffer_size_bound);
        info.streams[info.stream_count].stream_id = stream_id;
        info.streams[info.stream_count].buffer_bound_scale =
            buffer_bound_scale;
        info.streams[info.stream_count].buffer_size_bound = buffer_size_bound;
        info.stream_count++;
    }

    {
        const unsigned char* position = MPSDEC_BITS_POSITION();
        *consumed = position - data;
        if ((int)MPS_CheckDelim(position) == 0 &&
            MPS_CheckDelim(position + 1) == 0x40000) {
            (*consumed)++;
        }
    }

    if (handle->system_callback != 0) {
        info.rate_bound = header->rate_bound;
        info.audio_bound = header->audio_bound;
        info.fixed_flag = header->fixed_flag;
        info.csps_flag = header->csps_flag;
        info.audio_lock_flag = header->audio_lock_flag;
        info.video_lock_flag = header->video_lock_flag;
        info.video_bound = header->video_bound;
        info.packet_rate_restriction = (trailer >> 7) & 1;
        info.reserved_bits = trailer & 0x7F;
        handle->system_callback(handle->system_object, &info);
    }
}

static void mpsdec_DecPackHd(MpsHandle* handle, const unsigned char* data,
                             int* consumed) {
    const unsigned int* words;
    int word_offset;
    unsigned int current;
    unsigned int following;
    MpsPackHeader* header = &handle->headers.pack_header;
    unsigned int prefix;
    unsigned int high;
    unsigned int middle;
    unsigned int low;
    unsigned int mux_rate;

    words = (const unsigned int*)((unsigned long)(data + 4) & ~3UL);
    word_offset = ((data + 4) - (const unsigned char*)words) * 8;
    current = words[0];
    following = words[1];
    current <<= word_offset;
    words += 2;
    MPSDEC_READ_FIRST_TWO_BITS(prefix);
    MPSDEC_SKIP_BITS(2);
    MPSDEC_READ_BITS(3, high);
    MPSDEC_SKIP_BITS(1);
    MPSDEC_READ_BITS(15, middle);
    MPSDEC_SKIP_BITS(1);
    MPSDEC_READ_BITS(15, low);
    MPSDEC_SKIP_BITS(1);
    MPSDEC_SKIP_BITS(1);
    MPSDEC_READ_LAST_BITS(22, mux_rate);

    header->scr =
        ((long long)high << 30) | ((long long)middle << 15) | low;
    header->is_mpeg1 = prefix == 0;
    header->mux_rate = mux_rate;
    *consumed = 12;
}
int MPSDEC_DecHdMpeg1(MpsHandle* handle, const unsigned char* data, int size,
                      int* consumed, int* header_flags) {
    int used;
    int parse_more;
    int delimiter;
    int index;

    while (size >= 4) {
        used = parse_more = 0;
        delimiter = MPS_CheckDelim(data);

        switch (delimiter) {
        case 0x80000:
            break;
        case 0x10000:
            mpsdec_DecPackHd(handle, data, &used);
            parse_more = 1;
            break;
        case 0x20000:
            mpsdec_DecSysHd(handle, data, &used);
            parse_more = 1;
            break;
        case 0x40000:
            mpsdec_DecPketHd(handle, data, &used,
                             handle->packet_length_bytes);
            if (handle->pes_callback != 0) {
                handle->pes_callback(
                    handle->pes_object,
                    (unsigned char)handle->headers.packet_header.stream_id);
            }
            break;
        }

        *header_flags |= delimiter;
        data += used;
        size -= used;
        *consumed += used;
        if (!parse_more) {
            break;
        }
    }

    if ((*header_flags & 0x20000) != 0) {
        MpsSystemHeader* header = &handle->headers.last_system_header;

        if (header->audio_bound != 0) {
            index = 0;
        } else if (header->video_bound != 0) {
            index = 1;
        } else {
            index = 2;
        }
        handle->headers.system_headers[index] = *header;
    }
    return 0;
}

int MPS_DecHd(MpsHandle* handle, const unsigned char* data, int size,
              int* consumed, int* header_flags) {
    *consumed = 0;
    *header_flags = 0;
    if (MPSLIB_CheckHn(handle) != 0) {
        return MPSLIB_SetErr(0, 0xFF020301);
    }
    return handle->decode_header(handle, data, size, consumed, header_flags);
}

int MPS_SetPesFn(MpsHandle* handle, MpsPesCallback callback,
                 MpsCallbackObject object) {
    int result = MPSLIB_CheckHn(handle);
    if (result == 0) {
        handle->pes_callback = callback;
        handle->pes_object = object;
    }
    return result;
}

int MPS_SetPsMapFn(MpsHandle* handle, MpsPsMapCallback callback,
                   MpsCallbackObject object) {
    int result = MPSLIB_CheckHn(handle);
    if (result == 0) {
        handle->ps_map_callback = callback;
        handle->ps_map_object = object;
    }
    return result;
}

int MPS_SetSystemFn(MpsHandle* handle, MpsSystemCallback callback,
                    MpsCallbackObject object) {
    int result = MPSLIB_CheckHn(handle);
    if (result == 0) {
        handle->system_callback = callback;
        handle->system_object = object;
    }
    return result;
}

void MPSDEC_Finish(void) {
}

void MPSDEC_Init(void) {
}
