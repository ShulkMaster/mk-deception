#include "runtime/cstring.h"

typedef struct SFHHandle {
    int state;
    const unsigned char* header;
    int header_size;
    int version;
} SFHHandle;

typedef struct SFHObjectInfo {
    int capacity;
    int active_count;
    SFHHandle* objects;
} SFHObjectInfo;

typedef union SFHCodecParameters {
    unsigned short video_bit_rate;
    struct {
        unsigned char layer;
        unsigned char channels;
    } audio;
} SFHCodecParameters;

typedef union SFHMediaParameters {
    unsigned int audio_sample_rate;
    struct {
        unsigned char dimensions[3];
        unsigned char picture_rate;
    } video;
} SFHMediaParameters;

typedef struct SFHStreamRecord {
    unsigned char reserved[0x18];
    unsigned char id;
    unsigned char codec;
    SFHCodecParameters codec_parameters;
    SFHMediaParameters media_parameters;
    unsigned char effective_features;
    unsigned char color_type;
    unsigned char picture_type;
    unsigned char flags;
    unsigned char expand;
    unsigned char gop_n;
    unsigned char gop_m;
    unsigned char fx_type;
    unsigned char trailing[0x18];
} SFHStreamRecord;

const char SFH_sbver_str[] =
    "\nCRI SFH/GC Ver.1.19 Build:Sep  3 2004 11:38:52\n";
static const char sfh_version_tag[] = "Ver.";

static int sfh_init_cont = 0;
static SFHObjectInfo sfh_objinf;
static const char* sfhlib_version_dummy;

#pragma explicit_zero_data on
unsigned int gap_05_803A8A5C_data = 0;
#pragma explicit_zero_data off
unsigned int gap_06_804AC8CC_bss;

static inline int sfh_is_analyzable(const SFHHandle* handle)
{
    int state_valid;
    int version_valid;
    int valid;

    state_valid = 0;
    if (handle->state < 2) {
        if (handle->state < -1) {
            state_valid = 1;
        } else {
            state_valid = 0;
        }
    } else {
        state_valid = 1;
    }
    if (!state_valid) {
        valid = 0;
    } else {
        version_valid = 0;
        if (handle->version == 0x6B || handle->version >= 0x6E) {
            version_valid = 1;
        }
        if (!version_valid) {
            valid = 0;
        } else {
            valid = 1;
        }
    }
    return valid;
}

static inline unsigned int sfh_read_le32(const unsigned char* data, int offset)
{
    unsigned int value = *(const unsigned int*)(data + offset);

    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0xFF000000) >> 24);
}

static inline signed short sfh_read_le_s16(const unsigned char* data, int offset)
{
    unsigned short value = *(const unsigned short*)(data + offset);

    return (signed short)((value << 8) | (value >> 8));
}

static inline const SFHStreamRecord* sfh_find_stream(
    const unsigned char* header, unsigned char stream_id)
{
    const SFHStreamRecord* stream = 0;
    int i;

    for (i = 0; i < 13; i++) {
        stream = (const SFHStreamRecord*)(header + 0x180);
        if (stream->id == stream_id) return stream;
        stream = (const SFHStreamRecord*)(header + 0x1C0);
        if (stream->id == stream_id) return stream;
        header += 0x80;
    }
    return 0;
}

static inline int sfh_stream_class(unsigned char stream_id)
{
    if (stream_id >= 0xC0 && stream_id <= 0xDF) return 0xC0;
    if (stream_id >= 0xE0 && stream_id <= 0xEF) return 0xE0;
    if (stream_id == 0xBD || stream_id == 0xBF) return 0xBD;
    return 0;
}

static inline int sfh_is_effective_video(const SFHStreamRecord* stream,
                                         unsigned char stream_id)
{
    int valid;

    if (sfh_stream_class(stream_id) != 0xE0) {
        valid = 0;
    } else if (stream->effective_features > 1) {
        valid = 0;
    } else if (stream->effective_features == 0) {
        valid = 0;
    } else {
        valid = 1;
    }
    return valid;
}

int SFH_AnlyFtrFxType(const SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    if (handle->version < 0xD2) return 0;
    *result = stream->fx_type;
    return 1;
}

int SFH_AnlyFtrGopM(const SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->gop_m;
    if (*result > 0x3F) *result = -1;
    return 1;
}

int SFH_AnlyFtrGopN(const SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->gop_n;
    if (*result > 0x3F) *result = -1;
    return 1;
}

int SFH_AnlyFtrExpand(const SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->expand;
    return 1;
}

int SFH_AnlyFtrShcFixFlg(const SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    *result = (stream->flags >> 4) & 1;
    return 1;
}

int SFH_AnlyFtrFixFlg(const SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->flags & 1;
    return 1;
}

int SFH_AnlyFtrPicType(const SFHHandle* handle, unsigned char stream_id,
                       int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->picture_type;
    return 1;
}

int SFH_AnlyFtrColType(const SFHHandle* handle, unsigned char stream_id,
                       int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || !sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->color_type;
    return 1;
}

int SFH_AnlyElemPicRate(const SFHHandle* handle, unsigned char stream_id,
                        int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    int rate;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xE0) return 0;
    switch (stream->media_parameters.video.picture_rate) {
    case 1: rate = 23976; break;
    case 2: rate = 24000; break;
    case 3: rate = 25000; break;
    case 4: rate = 29970; break;
    case 5: rate = 30000; break;
    case 6: rate = 50000; break;
    case 7: rate = 59940; break;
    case 8: rate = 60000; break;
    default: rate = 0; break;
    }
    *result = rate;
    return 1;
}

int SFH_AnlyElemPicSz(const SFHHandle* handle, unsigned char stream_id,
                      int* width, int* height)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    const unsigned char* dimensions;

    *width = 0;
    *height = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xE0) return 0;
    dimensions = stream->media_parameters.video.dimensions;
    *width = dimensions[0];
    *width = (*width << 4) | (dimensions[1] >> 4);
    *width &= 0xFFF;
    *height = dimensions[1];
    *height = (*height << 8) | dimensions[2];
    *height &= 0xFFF;
    return 1;
}

int SFH_AnlyElemBitRate(const SFHHandle* handle, unsigned char stream_id,
                        int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    signed short bit_rate;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xE0) return 0;
    bit_rate = sfh_read_le_s16((const unsigned char*)stream, 0x1A);
    if (bit_rate == -1) bit_rate = 0;
    *result = bit_rate;
    return 1;
}

int SFH_AnlyElemCodecVid(const SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xE0) return 0;
    *result = stream->codec;
    return 1;
}

int SFH_AnlyElemSmpHz(const SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = sfh_read_le32((const unsigned char*)stream, 0x1C);
    return 1;
}

int SFH_AnlyElemChNum(const SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = stream->codec_parameters.audio.channels;
    return 1;
}

int SFH_AnlyElemLayer(const SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xC0) return 0;
    if (stream->codec != 1) return 0;
    *result = stream->codec_parameters.audio.layer;
    return 1;
}

int SFH_AnlyElemCodecAud(const SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0 || sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = stream->codec;
    return 1;
}

int SFH_AnlyMaxFrmNum(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le32(header, 0xC0);
    return 1;
}

int SFH_AnlyMaxPlyLenVid(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le32(header, 0xBC);
    return 1;
}

int SFH_AnlyMaxPlyLenAud(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le32(header, 0xB8);
    return 1;
}

int SFH_AnlyByteRate(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    if (handle->version < 0x6E) return 0;
    *result = sfh_read_le32(header, 0xB4);
    return 1;
}

int SFH_AnlyNumElemPrv(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = header[0xB3];
    return 1;
}

int SFH_AnlyNumElemVid(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = header[0xB2];
    return 1;
}

int SFH_AnlyNumElemAud(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = header[0xB1];
    return 1;
}

int SFH_AnlyNumElemTot(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = header[0xB0];
    return 1;
}

int SFH_AnlyPackSiz(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le32(header, 0x8C);
    return 1;
}

int SFH_AnlyPketSizLen(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le_s16(header, 0x88);
    return 1;
}

int SFH_AnlyPackType(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = -1;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = header[0x84];
    return 1;
}

int SFH_AnlyHdrSiz(const SFHHandle* handle, int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le32(header, 0x80);
    return 1;
}

static inline int sfh_anly_hdr_tool_ver(const SFHHandle* handle, int* major,
                                        int* minor)
{
    const unsigned char* header = handle->header;
    const char* digit;
    char tool_version[33];
    char character;
    int digit_valid;
    int state_valid = 0;
    int initialized;
    int parsed;
    unsigned char header_major;
    unsigned char header_minor;
    int tool_major;
    int tool_minor;

    *major = 0;
    *minor = 0;
    tool_version[0] = 0;
    if (handle->state < 2) {
        if (handle->state < -1) {
            state_valid = 1;
        } else {
            state_valid = 0;
        }
    } else {
        state_valid = 1;
    }
    if (!state_valid) {
        initialized = 0;
    } else {
        memset(tool_version, 0, 0x21);
        memcpy(tool_version, header + 0x60, 0x20);
        initialized = 1;
    }
    if (!initialized) return 0;
    header_major = header[0x38];
    header_minor = header[0x39];
    digit = strstr(tool_version, sfh_version_tag);
    if (digit == 0) {
        parsed = 0;
    } else {
        digit += 4;
        tool_major = 0;
        for (;;) {
            character = *digit;
            if (character != '.' && character != ' ' && character != 0) {
                digit_valid = 0;
                if (character >= '0' && character <= '9') digit_valid = 1;
                if (digit_valid) {
                    digit++;
                    tool_major = tool_major * 10 + character - '0';
                    continue;
                }
            }
            break;
        }
        digit++;
        tool_minor = 0;
        for (;;) {
            character = *digit;
            if (character != '.' && character != ' ' && character != 0) {
                digit_valid = 0;
                if (character >= '0' && character <= '9') digit_valid = 1;
                if (digit_valid) {
                    digit++;
                    tool_minor = tool_minor * 10 + character - '0';
                    continue;
                }
            }
            break;
        }
        parsed = 1;
    }
    if (!parsed) return 0;

    if (header_major * 100 + header_minor >= tool_major * 100 + tool_minor) {
        *major = header_major;
        *minor = header_minor;
    } else {
        *major = tool_major;
        *minor = tool_minor;
    }
    return 1;
}

int SFH_AnlyHdrToolVer(const SFHHandle* handle, int* major, int* minor)
{
    return sfh_anly_hdr_tool_ver(handle, major, minor);
}

static inline int sfh_query_audio_features(const SFHHandle* handle,
                                           unsigned char stream_id,
                                           int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    int valid;

    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) {
        valid = 0;
    } else if (stream->effective_features > 1) {
        valid = 0;
    } else if (stream->effective_features == 0) {
        valid = 0;
    } else {
        valid = 1;
    }
    *result = valid;
    return 1;
}

static inline int sfh_query_video_features(const SFHHandle* handle,
                                           unsigned char stream_id,
                                           int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    int valid;

    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) {
        valid = 0;
    } else if (stream->effective_features > 1) {
        valid = 0;
    } else if (stream->effective_features == 0) {
        valid = 0;
    } else {
        valid = 1;
    }
    *result = valid;
    return 1;
}

int SFH_IsEffFtrInf(const SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    int stream_class;

    if (handle->version < 0x6E) return 0;
    stream_class = sfh_stream_class(stream_id);
    switch (stream_class) {
    case 0xC0:
        return sfh_query_audio_features(handle, stream_id, result);
    case 0xE0:
        return sfh_query_video_features(handle, stream_id, result);
    default:
        return 0;
    }
}

int SFH_IsExistStmId(const SFHHandle* handle, unsigned char stream_id,
    int* result)
{
    const unsigned char* header = handle->header;

    *result = 0;
    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_find_stream(header, stream_id) != 0;
    return 1;
}

int SFH_IsSfdHeader(SFHHandle* handle, int* result)
{
    static const char signature[] = "SofdecStream            ";
    int major = 0;
    int minor = 0;

    *result = 0;
    if (handle->state == 0) return 0;
    if ((unsigned int)handle->header_size < 0x800) {
        handle->state = -1;
        return 0;
    }
    if (memcmp(handle->header + 0x20, signature, 0x18) != 0) {
        handle->state = -1;
        return 0;
    }

    handle->state = 2;
    if (!sfh_anly_hdr_tool_ver(handle, &major, &minor)) return 0;
    handle->version = major * 100 + minor;
    *result = 1;
    return 1;
}

void SFH_Destroy(SFHHandle* handle)
{
    handle->state = 0;
    handle->header = 0;
    handle->header_size = 0;
    handle->version = 0;
    sfh_objinf.active_count--;
}

SFHHandle* SFH_Create(const void* header, int header_size)
{
    int capacity = sfh_objinf.capacity;
    SFHHandle* handle = 0;
    SFHHandle* candidate = sfh_objinf.objects;
    int i;

    if (sfh_objinf.active_count >= capacity) {
        return 0;
    }

    for (i = 0; i < capacity; i++) {
        handle = candidate;
        if (!candidate->state) {
            break;
        }
        candidate++;
    }

    handle->state = 1;
    handle->header = header;
    handle->header_size = header_size;
    sfh_objinf.active_count++;
    return handle;
}

void SFH_Finish(void)
{
    sfh_init_cont--;
    if (sfh_init_cont <= 0) {
        sfh_objinf.capacity = 0;
        sfh_objinf.active_count = 0;
        sfh_objinf.objects = 0;
    }
}

void SFH_Init(int capacity, SFHHandle* objects)
{
    int i;

    if (sfh_init_cont > 0) {
        return;
    }

    sfh_init_cont++;
    sfhlib_version_dummy = SFH_sbver_str;
    for (i = 0; i < capacity; i++) {
        objects[i].state = 0;
        objects[i].header = 0;
        objects[i].header_size = 0;
        objects[i].version = 0;
    }
    sfh_objinf.capacity = capacity;
    sfh_objinf.active_count = 0;
    sfh_objinf.objects = objects;
}
