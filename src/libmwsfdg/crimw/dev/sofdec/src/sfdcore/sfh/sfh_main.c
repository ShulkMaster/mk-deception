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

typedef struct SFHStreamRecord {
    unsigned char reserved[0x18];
    unsigned char id;
    unsigned char codec;
    union {
        unsigned char codec_bytes[2];
        unsigned short codec_word;
    };
    union {
        unsigned char media_bytes[4];
        unsigned int media_word;
    };
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

typedef char SFHStreamRecordSizeCheck[sizeof(SFHStreamRecord) == 0x40 ? 1 : -1];

const char SFH_sbver_str[] =
    "\nCRI SFH/GC Ver.1.19 Build:Sep  3 2004 11:38:52\n";
static const char sfh_version_tag[] = "Ver.";

static int sfh_init_cont = 0;
static SFHObjectInfo sfh_objinf;
static const char* sfhlib_version_dummy;

static inline int sfh_is_valid(const SFHHandle* handle)
{
    int valid;

    switch (handle->state) {
    case -1:
    case 0:
    case 1:
        valid = 0;
        break;
    default:
        valid = 1;
        break;
    }
    return valid;
}

static int sfh_is_free(SFHHandle* handle)
{
    return handle->state == 0 ? 1 : 0;
}

static inline int sfh_is_version_ok(const SFHHandle* handle)
{
    return handle->version == 0x6B || handle->version >= 0x6E;
}

static inline int sfh_is_analyzable(const SFHHandle* handle)
{
    if (!sfh_is_valid(handle)) return 0;
    if (!sfh_is_version_ok(handle)) return 0;
    return 1;
}

static inline unsigned long sfh_read_le32(const unsigned char* data, int offset)
{
    const unsigned long* words = (const unsigned long*)data;
    unsigned long word = words[offset / sizeof(*words)];
    unsigned long swapped = ((word & 0x00FF0000UL) >> 8) |
                            ((word & 0x0000FF00UL) << 8);

    swapped |= (word >> 24) | (word << 24);
    return swapped;
}

static inline signed short sfh_read_le_s16(const unsigned char* data, int offset)
{
    signed short value = *(const signed short*)(data + offset);
    unsigned short bits = value;

    return (unsigned short)(((bits & 0xFF) << 8) |
                                          ((bits >> 8) & 0xFF));
}

static inline int sfh_read_header_u32(const SFHHandle* handle, int offset,
                                      int* result)
{
    const unsigned char* header = handle->header;

    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le32(header, offset);
    return 1;
}

static inline int sfh_read_header_u32_version(const SFHHandle* handle,
                                              int offset, int minimum_version,
                                              int* result)
{
    const unsigned char* header = handle->header;

    if (!sfh_is_analyzable(handle)) return 0;
    if (handle->version < minimum_version) return 0;
    *result = sfh_read_le32(header, offset);
    return 1;
}

static inline int sfh_read_header_u8(const SFHHandle* handle, int offset,
                                     int* result)
{
    const unsigned char* header = handle->header;

    if (!sfh_is_analyzable(handle)) return 0;
    *result = header[offset];
    return 1;
}

static inline int sfh_read_header_s16(const SFHHandle* handle, int offset,
                                      int* result)
{
    const unsigned char* header = handle->header;

    if (!sfh_is_analyzable(handle)) return 0;
    *result = sfh_read_le_s16(header, offset);
    return 1;
}

static inline const SFHStreamRecord* sfh_find_stream(
    const unsigned char* header, unsigned int stream_id)
{
    long i;
    const SFHStreamRecord* candidate;
    const SFHStreamRecord* stream;

    stream = 0;
    for (i = 0; i < 26; i++) {
        candidate = (const SFHStreamRecord*)(header + 0x180 + i * 0x40);
        if (candidate->id == stream_id) {
            stream = candidate;
            break;
        }
    }
    return stream;
}

static inline unsigned int sfh_stream_class(unsigned char stream_id)
{
    unsigned int type = stream_id;

    if (type >= 0xC0 && type <= 0xDF) {
        type = 0xC0;
    } else if (type >= 0xE0 && type <= 0xEF) {
        type = 0xE0;
    } else if (type == 0xBD || type == 0xBF) {
        type = 0xBD;
    } else {
        type = 0;
    }
    return type;
}

static inline int sfh_picture_rate(unsigned int code)
{
    switch (code) {
    case 1: return 23976;
    case 2: return 24000;
    case 3: return 25000;
    case 4: return 29970;
    case 5: return 30000;
    case 6: return 50000;
    case 7: return 59940;
    case 8: return 60000;
    default: return 0;
    }
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

static inline const SFHStreamRecord* sfh_find_handle_stream(
    const SFHHandle* handle, unsigned int stream_id)
{
    const unsigned char* header = handle->header;
    int i;
    const SFHStreamRecord* candidate;
    const SFHStreamRecord* stream;

    if (!sfh_is_analyzable(handle)) return 0;
    stream = 0;
    i = 0;
    for (; i < 26; i++) {
        candidate = (const SFHStreamRecord*)(header + 0x180);
        if (candidate->id == stream_id) {
            stream = candidate;
            break;
        }
        header += sizeof(SFHStreamRecord);
    }
    return stream;
}

int SFH_AnlyFtrFxType(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    if (handle->version < 0xD2) return 0;
    *result = stream->fx_type;
    return 1;
}

int SFH_AnlyFtrGopM(SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->gop_m;
    if (*result > 0x3F) *result = -1;
    return 1;
}

int SFH_AnlyFtrGopN(SFHHandle* handle, unsigned char stream_id, int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->gop_n;
    if (*result > 0x3F) *result = -1;
    return 1;
}

int SFH_AnlyFtrExpand(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->expand;
    return 1;
}

int SFH_AnlyFtrShcFixFlg(SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = (stream->flags >> 4) & 1;
    return 1;
}

int SFH_AnlyFtrFixFlg(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->flags & 1;
    return 1;
}

int SFH_AnlyFtrPicType(SFHHandle* handle, unsigned char stream_id,
                       int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->picture_type;
    return 1;
}

int SFH_AnlyFtrColType(SFHHandle* handle, unsigned char stream_id,
                       int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->color_type;
    return 1;
}

int SFH_AnlyElemPicRate(SFHHandle* handle, unsigned char stream_id,
                        int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    *result = sfh_picture_rate(stream->media_bytes[3]);
    return 1;
}

int SFH_AnlyElemPicSz(SFHHandle* handle, unsigned char stream_id,
                      int* width, int* height)
{
    const SFHStreamRecord* stream;
    const unsigned char* dimensions;

    *width = 0;
    *height = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    dimensions = stream->media_bytes;
    *width = dimensions[0];
    *width = (*width << 4) | ((dimensions[1] >> 4) & 0xF);
    *width &= 0xFFF;
    *height = dimensions[1];
    *height = (*height << 8) | dimensions[2];
    *height &= 0xFFF;
    return 1;
}

int SFH_AnlyElemBitRate(SFHHandle* handle, unsigned char stream_id,
                        int* result)
{
    const SFHStreamRecord* stream;
    const unsigned char* header;
    int encoded_rate;
    int bit_rate;
    int value;

    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    encoded_rate = stream->codec_word;
    bit_rate = (signed short)(unsigned short)(
        ((encoded_rate << 8) & 0xFF00) |
        (unsigned char)(encoded_rate >> 8));
    value = bit_rate;
    if (bit_rate == 0xFFFF) value = 0;
    *result = value;
    return 1;
}

int SFH_AnlyElemCodecVid(SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    *result = stream->codec;
    return 1;
}

/* TODO: [near miss] 94.65517%; byte-swap/store folds to stwbrx in backend;
 * donor dead guard is invalid; retain full-width unsigned endian decode. */
int SFH_AnlyElemSmpHz(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;
    unsigned int word;

    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    word = stream->media_word;
    *result = ((word << 8) & 0x00FF0000U) | (word << 24) |
              ((word >> 8) & 0x0000FF00U) | (word >> 24);
    return 1;
}

int SFH_AnlyElemChNum(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;
    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = stream->codec_bytes[1];
    return 1;
}

int SFH_AnlyElemLayer(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;
    *result = 0;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    if (stream->codec != 1) return 0;
    *result = stream->codec_bytes[0];
    return 1;
}

int SFH_AnlyElemCodecAud(SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_find_handle_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = stream->codec;
    return 1;
}

/* TODO: [near miss] 95.51282%; two insert pairs remain; early chain repair
 * triggers late STWBRX folding. Recover an authentic decoder boundary. */
int SFH_AnlyMaxFrmNum(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0xC0, result);
}

/* TODO: [near miss] 95.51%; aligned endian decode agrees; shared decoder insertion partition remains. */
int SFH_AnlyMaxPlyLenVid(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0xBC, result);
}

/* TODO: [near miss] 95.51%; aligned endian decode agrees; shared decoder byte-insertion folding remains. */
int SFH_AnlyMaxPlyLenAud(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0xB8, result);
}

/* TODO: [near miss] 96.02273%; shared decoder's early chain repair folds
 * to STWBRX in the late pass; an authentic block boundary is missing. */
int SFH_AnlyByteRate(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32_version(handle, 0xB4, 0x6E, result);
}

int SFH_AnlyNumElemPrv(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u8(handle, 0xB3, result);
}

int SFH_AnlyNumElemVid(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u8(handle, 0xB2, result);
}

int SFH_AnlyNumElemAud(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u8(handle, 0xB1, result);
}

int SFH_AnlyNumElemTot(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u8(handle, 0xB0, result);
}

/* TODO: [near miss] 95.51%; aligned endian decode agrees; shared decoder byte-insertion folding remains. */
int SFH_AnlyPackSiz(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0x8C, result);
}

int SFH_AnlyPketSizLen(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_s16(handle, 0x88, result);
}

int SFH_AnlyPackType(SFHHandle* handle, int* result)
{
    *result = -1;
    return sfh_read_header_u8(handle, 0x84, result);
}

/* TODO: [near miss] 95.51%; aligned endian decode agrees; shared decoder insertion partition remains. */
int SFH_AnlyHdrSiz(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0x80, result);
}

static inline int sfh_get_tool_str(SFHHandle* handle,
                                   const unsigned char* source, char* output)
{
    if (!sfh_is_valid(handle)) return 0;
    memset(output, 0, 0x21);
    memcpy(output, source, 0x20);
    return 1;
}

static inline int sfh_is_digit(int character)
{
    return character >= '0' && character <= '9';
}

static inline int sfh_atoi(const char** text)
{
    const char* digit = *text;
    int character;
    int value = 0;

    for (;;) {
        character = *digit;
        if (character == '.' || character == ' ' || character == 0) break;
        if (!sfh_is_digit(character)) break;
        value = value * 10 + character;
        value -= '0';
        digit++;
    }
    *text = digit;
    return value;
}

static inline int sfh_get_str_ver(const char* text, int* major, int* minor)
{
    const char* digit = strstr(text, sfh_version_tag);

    if (digit == 0) return 0;
    digit += 4;
    *major = sfh_atoi(&digit);
    digit++;
    *minor = sfh_atoi(&digit);
    return 1;
}

static inline int sfh_anly_hdr_tool_ver(SFHHandle* handle, int* major,
                                        int* minor)
{
    const unsigned char* header;
    const unsigned char* tool_string;
    char tool_version[33];
    int tool_major;
    int tool_minor;
    int header_major;
    int header_minor;

    *major = 0;
    *minor = 0;
    header = handle->header;
    tool_version[0] = 0;
    tool_string = header + 0x60;
    if (!sfh_get_tool_str(handle, tool_string, tool_version)) return 0;
    header_major = header[0x38];
    header_minor = header[0x39];
    if (!sfh_get_str_ver(tool_version, &tool_major, &tool_minor)) return 0;

    if (header_major * 100 + header_minor >= tool_major * 100 + tool_minor) {
        *major = header_major;
        *minor = header_minor;
    } else {
        *major = tool_major;
        *minor = tool_minor;
    }
    return 1;
}

int SFH_AnlyHdrToolVer(SFHHandle* handle, int* major, int* minor)
{
    const unsigned char* header;
    const unsigned char* tool_string;
    char tool_version[33];
    int tool_major;
    int tool_minor;
    int header_major;
    int header_minor;

    *major = 0;
    *minor = 0;
    header = handle->header;
    tool_version[0] = 0;
    tool_string = header + 0x60;
    if (!sfh_get_tool_str(handle, tool_string, tool_version)) return 0;
    header_major = header[0x38];
    header_minor = header[0x39];
    if (!sfh_get_str_ver(tool_version, &tool_major, &tool_minor)) return 0;

    if (header_major * 100 + header_minor >= tool_major * 100 + tool_minor) {
        *major = header_major;
        *minor = header_minor;
    } else {
        *major = tool_major;
        *minor = tool_minor;
    }
    return 1;
}

static inline const SFHStreamRecord* sfh_feature_stream(
    const SFHHandle* handle, unsigned int stream_id)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;

    stream = !sfh_is_analyzable(handle) ? 0 : sfh_find_stream(header, stream_id);
    return stream;
}

static inline int sfh_effective_features(
    const SFHStreamRecord* stream, unsigned int stream_id,
    unsigned int expected_class)
{
    int valid;

    if (sfh_stream_class(stream_id) != expected_class) {
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

int SFH_IsEffFtrInf(SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    const SFHStreamRecord* stream;
    int stream_class;
    unsigned int stream_id_value;

    if (handle->version < 0x6E) return 0;
    stream_id_value = stream_id;
    stream_class = sfh_stream_class(stream_id_value);
    switch (stream_class) {
    case 0xC0:
        stream = sfh_feature_stream(handle, stream_id_value);
        if (stream == 0) return 0;
        *result = sfh_effective_features(stream, stream_id_value, 0xC0);
        break;
    case 0xE0:
        stream = sfh_feature_stream(handle, stream_id_value);
        if (stream == 0) return 0;
        *result = sfh_effective_features(stream, stream_id_value, 0xE0);
        break;
    default:
        return 0;
    }
    return 1;
}

int SFH_IsExistStmId(SFHHandle* handle, unsigned char stream_id,
    int* result)
{
    const unsigned char* header;

    *result = 0;
    header = handle->header;
    if (!sfh_is_analyzable(handle)) return 0;
    if (sfh_find_stream(header, stream_id) != 0) {
        *result = 1;
    } else {
        *result = 0;
    }
    return 1;
}

int SFH_IsSfdHeader(SFHHandle* handle, int* result)
{
    static const char signature[] = "SofdecStream            ";
    const unsigned char* identifier;
    int major;
    int minor;

    *result = 0;
    identifier = handle->header + 0x20;
    if (sfh_is_free(handle) == 1) return 0;
    if ((unsigned int)handle->header_size < 0x800) {
        handle->state = -1;
        return 0;
    }
    if (memcmp(identifier, signature, 0x18) != 0) {
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
    SFHHandle* objects = sfh_objinf.objects;
    SFHHandle* handle = 0;
    int i;

    if (sfh_objinf.active_count >= capacity) {
        return 0;
    }

    for (i = 0; i < capacity; i++) {
        handle = &objects[i];
        if (handle->state == 0) {
            break;
        }
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

static inline void sfh_ClearHn(SFHHandle* objects, int capacity)
{
    int i;

    for (i = 0; i < capacity; i++) {
        objects[i].state = 0;
        objects[i].header = 0;
        objects[i].header_size = 0;
        objects[i].version = 0;
    }
}

void SFH_Init(int capacity, SFHHandle* objects)
{
    if (sfh_init_cont > 0) {
        return;
    }

    sfh_init_cont++;
    sfhlib_version_dummy = SFH_sbver_str;
    sfh_ClearHn(objects, capacity);
    sfh_objinf.capacity = capacity;
    sfh_objinf.active_count = 0;
    sfh_objinf.objects = objects;
}
