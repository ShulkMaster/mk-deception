#include "cri/mps.h"

typedef struct MpsBitReader {
    const unsigned int* next_word;
    unsigned int current;
    unsigned int following;
    int word_offset;
} MpsBitReader;

static inline void mpsdec_init_bits(MpsBitReader* reader,
                                    const unsigned char* data,
                                    int bit_position) {
    const unsigned char* position = data + (bit_position >> 3);
    const unsigned int* words =
        (const unsigned int*)((unsigned long)position & ~3UL);
    int word_offset = (position - (const unsigned char*)words) * 8;

    reader->next_word = words + 2;
    reader->current = words[0] << word_offset;
    reader->following = words[1];
    reader->word_offset = word_offset;
}

static inline unsigned int mpsdec_read_bits(MpsBitReader* reader, int count) {
    unsigned int value;
    int shift;
    int threshold = 32 - count;

    if (reader->word_offset >= threshold) {
        shift = reader->word_offset - threshold;
        if (shift != 0) {
            reader->current |= reader->following >> (count - shift);
            value = reader->current >> threshold;
            reader->current = reader->following << shift;
        } else {
            value = reader->current >> threshold;
            reader->current = reader->following;
        }
        reader->following = *reader->next_word++;
        reader->word_offset = shift;
    } else {
        value = reader->current >> threshold;
        reader->current <<= count;
        reader->word_offset += count;
    }
    return value;
}

static inline unsigned int mpsdec_read_first_two_bits(MpsBitReader* reader) {
    unsigned int value;

    if (reader->word_offset >= 30) {
        reader->word_offset -= 30;
        if (reader->word_offset != 0) {
            reader->current |= reader->following >> 1;
            value = reader->current >> 30;
            reader->current = reader->following << 1;
        } else {
            value = reader->current >> 30;
            reader->current = reader->following;
        }
        reader->following = *reader->next_word++;
    } else {
        value = reader->current >> 30;
        reader->current <<= 2;
        reader->word_offset += 2;
    }
    return value;
}

static inline void mpsdec_read_bits_into(MpsBitReader* reader, int count,
                                         int* output) {
    int threshold = 32 - count;

    if (reader->word_offset >= threshold) {
        reader->word_offset -= threshold;
        if (reader->word_offset != 0) {
            reader->current |= reader->following >>
                               (count - reader->word_offset);
            *output = reader->current >> threshold;
            reader->current = reader->following << reader->word_offset;
        } else {
            *output = reader->current >> threshold;
            reader->current = reader->following;
        }
        reader->following = *reader->next_word++;
    } else {
        *output = reader->current >> threshold;
        reader->current <<= count;
        reader->word_offset += count;
    }
}

static inline unsigned int mpsdec_read_bit(MpsBitReader* reader) {
    unsigned int value = reader->current >> 31;

    if (reader->word_offset == 31) {
        reader->current = reader->following;
        reader->following = *reader->next_word++;
        reader->word_offset = 0;
    } else {
        reader->current <<= 1;
        reader->word_offset++;
    }
    return value;
}

static inline void mpsdec_read_bit_into(MpsBitReader* reader, int* output) {
    *output = reader->current >> 31;

    if (reader->word_offset == 31) {
        reader->current = reader->following;
        reader->following = *reader->next_word++;
        reader->word_offset = 0;
    } else {
        reader->current <<= 1;
        reader->word_offset++;
    }
}

static inline void mpsdec_read_word_into(MpsBitReader* reader, int* output) {
    if (reader->word_offset != 0) {
        *output = reader->current |
                  (reader->following >> (32 - reader->word_offset));
        reader->current = reader->following << reader->word_offset;
    } else {
        *output = reader->current;
        reader->current = reader->following;
    }
    reader->following = *reader->next_word++;
}

static inline void mpsdec_skip_bits(MpsBitReader* reader, int count) {
    reader->word_offset += count;
    if (reader->word_offset >= 32) {
        reader->word_offset -= 32;
        reader->current = reader->following << reader->word_offset;
        reader->following = *reader->next_word++;
    } else {
        reader->current <<= count;
    }
}

static inline const unsigned char* mpsdec_bits_position(
    const MpsBitReader* reader) {
    return (const unsigned char*)(reader->next_word - 2) +
           ((reader->word_offset + 7) >> 3);
}

static inline int mpsdec_bits_consumed(const MpsBitReader* reader,
                                       const unsigned char* data) {
    return mpsdec_bits_position(reader) - data;
}

static inline unsigned int mpsdec_peek_bits(MpsBitReader* reader, int count) {
    int threshold = 32 - count;
    unsigned int value = reader->current >> threshold;

    if (reader->word_offset > threshold) {
        value |= reader->following >>
                 (32 + threshold - reader->word_offset);
    }
    return value;
}

#define MPSDEC_READ_TIMESTAMP(reader, result)                              \
    do {                                                                  \
        unsigned int high;                                                \
        unsigned int middle;                                              \
        unsigned int low;                                                 \
        mpsdec_skip_bits((reader), 4);                                    \
        high = mpsdec_read_bits((reader), 3);                             \
        mpsdec_skip_bits((reader), 1);                                    \
        middle = mpsdec_read_bits((reader), 15);                          \
        mpsdec_skip_bits((reader), 1);                                    \
        low = mpsdec_read_bits((reader), 15);                             \
        mpsdec_skip_bits((reader), 1);                                    \
        (result) =                                                        \
            ((long long)high << 30) | ((long long)middle << 15) | low;    \
    } while (0)

/* TODO: [near miss] 96.873764%; typed reader follows donor bit-window flow;
 * timestamp and cursor scheduling remain. */
static void mpsdec_DecPketHd(MpsHandle* handle, const unsigned char* data,
                             int* consumed, int packet_length_bytes) {
    MpsBitReader reader;
    MpsPacketHeader* header = &handle->headers.packet_header;
    int stream_id;
    int stream_type;
    int stream_index;
    int base_header_size;

    mpsdec_init_bits(&reader, data, 24);
    stream_id = mpsdec_read_bits(&reader, 8);
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
        mpsdec_read_bits_into(&reader, 16, &header->packet_length);
        base_header_size = 6;
    } else {
        mpsdec_read_word_into(&reader, &header->packet_length);
        base_header_size = 8;
    }

    if (stream_id == 0xBF || stream_id == 0xBE) {
        *consumed = base_header_size;
        header->payload_length = header->packet_length;
        return;
    }

    for (;;) {
        unsigned int next_byte = mpsdec_peek_bits(&reader, 8);
        if (next_byte != 0xFF) {
            break;
        }
        mpsdec_skip_bits(&reader, 8);
    }

    if (mpsdec_peek_bits(&reader, 2) == 1) {
        int scale;
        unsigned int size;

        mpsdec_skip_bits(&reader, 2);
        scale = mpsdec_read_bit(&reader);
        size = mpsdec_read_bits(&reader, 13);
        size <<= 7;
        if (scale != 0) {
            size <<= 3;
        }
        header->std_buffer_size = size;
    }

    {
        unsigned int timestamp_flags = mpsdec_peek_bits(&reader, 4);

        if (timestamp_flags == 2) {
            MPSDEC_READ_TIMESTAMP(&reader, header->pts);
            header->dts = -1;
        } else if (timestamp_flags == 3) {
            MPSDEC_READ_TIMESTAMP(&reader, header->pts);
            MPSDEC_READ_TIMESTAMP(&reader, header->dts);
        } else {
            mpsdec_skip_bits(&reader, 8);
            header->pts = -1;
            header->dts = -1;
        }
    }

    *consumed = mpsdec_bits_consumed(&reader, data);
    header->payload_length =
        header->packet_length + base_header_size - *consumed;
}

/* TODO: [near miss] 97.712940%; in-place output windows and delimiter test
 * agree; cursor coloring and equivalent shifts remain. */
static void mpsdec_DecSysHd(MpsHandle* handle, const unsigned char* data,
                            int* consumed) {
    unsigned int trailer;
    MpsBitReader reader;
    MpsSystemHeader* header = &handle->headers.last_system_header;
    MpsSystemCallbackInfo info;
    const unsigned char* position = data + 4;
    const unsigned int* words =
        (const unsigned int*)((unsigned long)position & ~3UL);
    int word_offset = (position - (const unsigned char*)words) * 8;

    reader.current = words[0] << word_offset;
    reader.following = words[1];
    reader.word_offset = word_offset;
    reader.next_word = words + 2;
    mpsdec_read_bits_into(&reader, 16, &header->header_length);
    mpsdec_skip_bits(&reader, 1);
    mpsdec_read_bits_into(&reader, 22, &header->rate_bound);
    mpsdec_skip_bits(&reader, 1);
    mpsdec_read_bits_into(&reader, 6, &header->audio_bound);
    mpsdec_read_bit_into(&reader, &header->fixed_flag);
    mpsdec_read_bit_into(&reader, &header->csps_flag);
    mpsdec_read_bit_into(&reader, &header->audio_lock_flag);
    mpsdec_read_bit_into(&reader, &header->video_lock_flag);
    mpsdec_skip_bits(&reader, 1);
    mpsdec_read_bits_into(&reader, 5, &header->video_bound);
    trailer = mpsdec_read_bits(&reader, 8);

    info.stream_count = 0;
    for (;;) {
        unsigned int stream_id;
        unsigned int buffer_bound_scale;
        unsigned int buffer_size_bound;

        if ((reader.current >> 31) == 0) {
            break;
        }

        stream_id = mpsdec_read_bits(&reader, 8);
        mpsdec_skip_bits(&reader, 2);
        buffer_bound_scale = mpsdec_read_bit(&reader);
        buffer_size_bound = mpsdec_read_bits(&reader, 13);
        info.streams[info.stream_count].stream_id = stream_id;
        info.streams[info.stream_count].buffer_bound_scale =
            buffer_bound_scale;
        info.streams[info.stream_count].buffer_size_bound = buffer_size_bound;
        info.stream_count++;
    }

    {
        const unsigned char* position = mpsdec_bits_position(&reader);
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

/* TODO: [near miss] 93.451614%; prefix and terminal reads follow retail;
 * remaining mismatch is register coloring around the reader state. */
static void mpsdec_DecPackHd(MpsHandle* handle, const unsigned char* data,
                             int* consumed) {
    MpsBitReader reader;
    MpsPackHeader* header = &handle->headers.pack_header;
    unsigned int prefix;
    unsigned int high;
    unsigned int middle;
    unsigned int low;
    unsigned int mux_rate;

    mpsdec_init_bits(&reader, data, 32);
    prefix = mpsdec_read_first_two_bits(&reader);
    mpsdec_skip_bits(&reader, 2);
    high = mpsdec_read_bits(&reader, 3);
    mpsdec_skip_bits(&reader, 1);
    middle = mpsdec_read_bits(&reader, 15);
    mpsdec_skip_bits(&reader, 1);
    low = mpsdec_read_bits(&reader, 15);
    mpsdec_skip_bits(&reader, 1);
    mpsdec_skip_bits(&reader, 1);
    /* The rate is the last field; no later read needs the advanced cursor. */
    if (reader.word_offset >= 10) {
        if (reader.word_offset - 10 != 0) {
            reader.current |=
                reader.following >> (22 - (reader.word_offset - 10));
            mux_rate = reader.current >> 10;
        } else {
            mux_rate = reader.current >> 10;
        }
    } else {
        mux_rate = reader.current >> 10;
    }

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
