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
    /* Audio layer/channels; video bitrate bytes. */
    unsigned char codec_bytes[2];
    /* Audio sample-rate bytes; video picture dimensions and frame rate. */
    unsigned char media_bytes[4];
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
    return handle->state == 0;
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

static inline unsigned int sfh_read_le32(const unsigned char* data, int offset)
{
    const unsigned char* bytes = data + offset;

    return (unsigned int)bytes[0] |
           ((unsigned int)bytes[1] << 8) |
           ((unsigned int)bytes[2] << 16) |
           ((unsigned int)bytes[3] << 24);
}

static inline signed short sfh_read_le_s16(const unsigned char* data, int offset)
{
    signed short value = *(const signed short*)(data + offset);
    unsigned short bits = (unsigned short)value;

    return (signed short)(unsigned short)(((bits & 0xFF) << 8) |
                                          ((bits >> 8) & 0xFF));
}

static inline int sfh_read_header_u32(const SFHHandle* handle, int offset,
                                      int* result)
{
    const unsigned char* header = handle->header;

    if (!sfh_is_analyzable(handle)) return 0;
    *result = (int)sfh_read_le32(header, offset);
    return 1;
}

static inline int sfh_read_header_u32_version(const SFHHandle* handle,
                                              int offset, int minimum_version,
                                              int* result)
{
    const unsigned char* header = handle->header;

    if (!sfh_is_analyzable(handle)) return 0;
    if (handle->version < minimum_version) return 0;
    *result = (int)sfh_read_le32(header, offset);
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
    const SFHStreamRecord* stream = 0;
    const SFHStreamRecord* candidate;
    int i;

    for (i = 0; i < 26; i++) {
        candidate = (const SFHStreamRecord*)(header + 0x180);
        if (candidate->id == stream_id) {
            stream = candidate;
            break;
        }
        header += sizeof(SFHStreamRecord);
    }
    return stream;
}

static inline const SFHStreamRecord* sfh_get_stream(
    const SFHHandle* handle, unsigned char stream_id)
{
    const unsigned char* header = handle->header;

    if (!sfh_is_analyzable(handle)) return 0;
    return sfh_find_stream(header, stream_id);
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

/* TODO: [near miss] 97.524270%; 32-bit search ID matches donor width;
 * stream-search coloring and key-load schedule remain. */
int SFH_AnlyFtrFxType(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    if (handle->version < 0xD2) return 0;
    *result = stream->fx_type;
    return 1;
}

/* TODO: [near miss] 97.524270%; mutable handle and 32-bit search ID agree;
 * search-result register coloring remains. */
int SFH_AnlyFtrGopM(SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->gop_m;
    if (*result > 0x3F) *result = -1;
    return 1;
}

/* TODO: [near miss] 97.524270%; mutable handle and 32-bit search ID agree;
 * search-result register coloring remains. */
int SFH_AnlyFtrGopN(SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->gop_n;
    if (*result > 0x3F) *result = -1;
    return 1;
}

/* TODO: [near miss] 97.371130%; mutable handle and 32-bit search ID agree;
 * search-result register coloring remains. */
int SFH_AnlyFtrExpand(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->expand;
    return 1;
}

/* TODO: [near miss] 97.397960%; mutable handle and 32-bit search ID agree;
 * search-result register coloring remains. */
int SFH_AnlyFtrShcFixFlg(SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = (stream->flags >> 4) & 1;
    return 1;
}

/* TODO: [near miss] 97.397960%; mutable handle and 32-bit search ID agree;
 * search-result register coloring remains. */
int SFH_AnlyFtrFixFlg(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->flags & 1;
    return 1;
}

/* TODO: [near miss] 97.397960%; mutable handle and 32-bit search ID agree;
 * search-result register coloring remains. */
int SFH_AnlyFtrPicType(SFHHandle* handle, unsigned char stream_id,
                       int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->picture_type;
    return 1;
}

/* TODO: [near miss] 97.397960%; mutable handle and 32-bit search ID agree;
 * search-result register coloring remains. */
int SFH_AnlyFtrColType(SFHHandle* handle, unsigned char stream_id,
                       int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (!sfh_is_effective_video(stream, stream_id)) return 0;
    *result = stream->color_type;
    return 1;
}

/* TODO: [near miss] 97.747750%; typed media bytes and 32-bit search ID agree;
 * search coloring and key-load schedule remain. */
int SFH_AnlyElemPicRate(SFHHandle* handle, unsigned char stream_id,
                        int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    *result = sfh_picture_rate(stream->media_bytes[3]);
    return 1;
}

/* TODO: [near miss] 96.584160%; typed media bytes and 32-bit search ID agree;
 * picture-size load/result register lifetimes remain. */
int SFH_AnlyElemPicSz(SFHHandle* handle, unsigned char stream_id,
                      int* width, int* height)
{
    const SFHStreamRecord* stream;
    const unsigned char* dimensions;

    *width = 0;
    *height = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    dimensions = stream->media_bytes;
    *width = dimensions[0];
    *width = (*width << 4) | (dimensions[1] >> 4);
    *width &= 0xFFF;
    *height = dimensions[1];
    *height = (*height << 8) | dimensions[2];
    *height &= 0xFFF;
    return 1;
}

/* TODO: [near miss] 95.000000%; portable bytes and 32-bit search ID agree;
 * retail's halfword load remains unmatched by safe byte access. */
int SFH_AnlyElemBitRate(SFHHandle* handle, unsigned char stream_id,
                        int* result)
{
    const SFHStreamRecord* stream;
    int bit_rate;
    int value;

    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    bit_rate = (signed short)(unsigned short)(
        ((unsigned short)stream->codec_bytes[1] << 8) |
        stream->codec_bytes[0]);
    value = bit_rate;
    if (bit_rate == 0xFFFF) value = 0;
    *result = value;
    return 1;
}

/* TODO: [near miss] 97.023810%; mutable handle and 32-bit search ID agree;
 * search/codec register coloring remains. */
int SFH_AnlyElemCodecVid(SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xE0) return 0;
    *result = stream->codec;
    return 1;
}

/* TODO: [near miss] 91.494255%; bytewise decode and 32-bit search ID agree;
 * retail's word load/swap remains a portability ceiling. */
int SFH_AnlyElemSmpHz(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;

    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = ((unsigned int)stream->media_bytes[3] << 24) |
              ((unsigned int)stream->media_bytes[2] << 16) |
              ((unsigned int)stream->media_bytes[1] << 8) |
              stream->media_bytes[0];
    return 1;
}

/* TODO: [near miss] 96.987950%; mutable handle and 32-bit search ID agree;
 * search/channel register coloring remains. */
int SFH_AnlyElemChNum(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;
    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = stream->codec_bytes[1];
    return 1;
}

/* TODO: [near miss] 97.102270%; mutable handle and 32-bit search ID agree;
 * search/codec-layer register coloring remains. */
int SFH_AnlyElemLayer(SFHHandle* handle, unsigned char stream_id,
                      int* result)
{
    const SFHStreamRecord* stream;
    *result = 0;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    if (stream->codec != 1) return 0;
    *result = stream->codec_bytes[0];
    return 1;
}

/* TODO: [near miss] 97.023810%; mutable handle and 32-bit search ID agree;
 * search/codec register coloring remains. */
int SFH_AnlyElemCodecAud(SFHHandle* handle, unsigned char stream_id,
                         int* result)
{
    const SFHStreamRecord* stream;

    *result = -1;
    stream = sfh_get_stream(handle, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != 0xC0) return 0;
    *result = stream->codec;
    return 1;
}

/* TODO: [near miss] 89.23077%; bytewise header decode is portable;
 * retail word-load and validation lowering still differ. */
int SFH_AnlyMaxFrmNum(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0xC0, result);
}

/* TODO: [near miss] 89.23077%; bytewise header decode is portable;
 * retail word-load and validation lowering still differ. */
int SFH_AnlyMaxPlyLenVid(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0xBC, result);
}

/* TODO: [near miss] 89.23077%; bytewise header decode is portable;
 * retail word-load and validation lowering still differ. */
int SFH_AnlyMaxPlyLenAud(SFHHandle* handle, int* result)
{
    *result = 0;
    return sfh_read_header_u32(handle, 0xB8, result);
}

/* TODO: [near miss] 90.11364%; bytewise header decode is portable;
 * retail word-load and version-validation lowering still differ. */
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

/* TODO: [near miss] 89.23077%; bytewise header decode is portable;
 * retail word-load and validation lowering still differ. */
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

/* TODO: [near miss] 89.23077%; bytewise header decode is portable;
 * retail word-load and validation lowering still differ. */
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
    unsigned char header_major;
    unsigned char header_minor;

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

/* TODO: [near miss] 94.188034%; parsing and selection match structurally;
 * donor declaration order is neutral, so stop at register coloring. */
int SFH_AnlyHdrToolVer(SFHHandle* handle, int* major, int* minor)
{
    return sfh_anly_hdr_tool_ver(handle, major, minor);
}

static inline int sfh_query_features(const SFHHandle* handle,
                                     unsigned int stream_id,
                                     unsigned int expected_class,
                                     int* result)
{
    const unsigned char* header = handle->header;
    const SFHStreamRecord* stream;
    int valid;

    if (!sfh_is_analyzable(handle)) return 0;
    stream = sfh_find_stream(header, stream_id);
    if (stream == 0) return 0;
    if (sfh_stream_class(stream_id) != expected_class) {
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

/* TODO: [near miss] 95.417060%; widened ID and shared typed feature helper
 * preserve both arms; 32-bit search width helps siblings, owner coloring remains. */
int SFH_IsEffFtrInf(SFHHandle* handle, unsigned char stream_id,
                    int* result)
{
    int stream_class;
    unsigned int stream_id_value;

    if (handle->version < 0x6E) return 0;
    stream_id_value = stream_id;
    stream_class = sfh_stream_class(stream_id_value);
    switch (stream_class) {
    case 0xC0:
        return sfh_query_features(handle, stream_id_value, 0xC0, result);
    case 0xE0:
        return sfh_query_features(handle, stream_id_value, 0xE0, result);
    default:
        return 0;
    }
}

/* TODO: [near miss] 96.166664%; retail flow and 32-bit search ID agree;
 * search coloring and equivalent key-load schedule remain. */
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

/* TODO: [near miss] 95.61290%; typed free helper restores early CFG;
 * stop at cmpwi versus Boolean materialization and register coloring. */
int SFH_IsSfdHeader(SFHHandle* handle, int* result)
{
    static const char signature[] = "SofdecStream            ";
    const unsigned char* identifier;
    int major = 0;
    int minor = 0;

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
