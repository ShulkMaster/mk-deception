#include "sofdec/sfd_header.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_transport.h"
#include "sofdec/uty_mem.h"

typedef struct SFHHandle {
    int state;
    const unsigned char* header;
    int header_size;
    int version;
} SFHHandle;

typedef struct SfhdsVideoHeaderInfo {
    int codec;
    int bit_rate;
    int width;
    int height;
    int picture_rate;
    int has_effective_features;
    int color_type;
    int picture_type;
    int fixed_flag;
    int sequence_header_fixed_flag;
    int expand;
    int gop_n;
    int gop_m;
} SfhdsVideoHeaderInfo;

typedef struct SfhdsProcessedHeader {
    int processed;
    int mux_version_major;
    int mux_version_minor;
    int byte_rate;
    int header_size;
    int pack_type;
    int packet_size_length;
    int pack_size;
    int element_count;
    int audio_element_count;
    int video_element_count;
    int private_element_count;
    int maximum_audio_length;
    int maximum_video_length;
    int maximum_frame_count;
    int private_stream_1;
    int private_stream_2;
    int audio_stream;
    int video_stream;
    int audio_codec;
    int audio_layer;
    int audio_channel_count;
    int audio_sample_rate;
    SfhdsVideoHeaderInfo video;
    int raw_header_size;
} SfhdsProcessedHeader;

typedef struct SfhdsHeaderBlock {
    SfhdsProcessedHeader processed;
    unsigned char raw_header[0x800];
} SfhdsHeaderBlock;

typedef struct SfhdsSfhLibraryWork {
    SFHHandle handles[32];
    int reserved;
} SfhdsSfhLibraryWork;

typedef void (*SfhdsHeaderCallback)(SfdCallbackObject object,
                                    const void* data, int size);

typedef char SfhdsProcessedHeaderSizeCheck[
    sizeof(SfhdsProcessedHeader) == 0x94 ? 1 : -1];
typedef char SfhdsHeaderBlockSizeCheck[
    sizeof(SfhdsHeaderBlock) == 0x894 ? 1 : -1];
typedef char SfhdsLibraryWorkSizeCheck[
    sizeof(SfhdsSfhLibraryWork) == 0x204 ? 1 : -1];

extern int SFH_AnlyByteRate(const SFHHandle*, int*);
extern int SFH_AnlyElemBitRate(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemChNum(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemCodecAud(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemCodecVid(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemLayer(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemPicRate(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemPicSz(const SFHHandle*, unsigned char, int*, int*);
extern int SFH_AnlyElemSmpHz(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrColType(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrExpand(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrFixFlg(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrGopM(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrGopN(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrPicType(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrShcFixFlg(const SFHHandle*, unsigned char, int*);
extern int SFH_AnlyHdrSiz(const SFHHandle*, int*);
extern int SFH_AnlyHdrToolVer(const SFHHandle*, int*, int*);
extern int SFH_AnlyMaxFrmNum(const SFHHandle*, int*);
extern int SFH_AnlyMaxPlyLenAud(const SFHHandle*, int*);
extern int SFH_AnlyMaxPlyLenVid(const SFHHandle*, int*);
extern int SFH_AnlyNumElemAud(const SFHHandle*, int*);
extern int SFH_AnlyNumElemPrv(const SFHHandle*, int*);
extern int SFH_AnlyNumElemTot(const SFHHandle*, int*);
extern int SFH_AnlyNumElemVid(const SFHHandle*, int*);
extern int SFH_AnlyPackSiz(const SFHHandle*, int*);
extern int SFH_AnlyPackType(const SFHHandle*, int*);
extern int SFH_AnlyPketSizLen(const SFHHandle*, int*);
extern int SFH_IsEffFtrInf(const SFHHandle*, unsigned char, int*);
extern int SFH_IsExistStmId(const SFHHandle*, unsigned char, int*);
extern int SFH_IsSfdHeader(SFHHandle*, int*);
extern SFHHandle* SFH_Create(const void*, int);
extern void SFH_Destroy(SFHHandle*);
extern void SFH_Finish(void);
extern void SFH_Init(int, SFHHandle*);
extern int SFMPS_GetConcatCnt(SfdHandle*);

static SfhdsSfhLibraryWork sfhds_sfhlib_work;

static SfhdsProcessedHeader* sfhds_GetProcessedHeader(SfdHeaderState* state)
{
    return (SfhdsProcessedHeader*)state;
}

static SfhdsHeaderBlock* sfhds_GetHeaderBlock(SfdHandle* handle)
{
    return (SfhdsHeaderBlock*)&handle->header_state;
}

static SfhdsHeaderBlock* sfhds_GetSeekHeader(SfdHandle* handle)
{
    unsigned char* source;

    if (handle->seek_state.source_handle == 0) {
        return 0;
    }
    if (SFMPS_GetConcatCnt(handle) > 0) {
        return 0;
    }
    source = (unsigned char*)handle->seek_state.source_handle;
    return (SfhdsHeaderBlock*)(source + 0x0C);
}

static void sfhds_PublishElementCounts(SfdHandle* handle)
{
    SfhdsProcessedHeader* header =
        sfhds_GetProcessedHeader(&handle->header_state);

    handle->playback_settings.values_18[3] = header->audio_element_count;
    handle->playback_settings.values_18[4] = header->video_element_count;
    handle->playback_settings.values_18[5] = header->private_element_count;
}

int SFHDS_GetColType(SfdHandle* handle)
{
    SfhdsProcessedHeader* header =
        sfhds_GetProcessedHeader(&handle->header_state);
    SfhdsVideoHeaderInfo* video = &header->video;

    if (header->processed == 0) {
        return -1;
    }
    if (video->has_effective_features != 0) {
        return video->color_type;
    }
    return -1;
}

int SFHDS_GetMuxVerNum(SfdHandle* handle)
{
    SfhdsProcessedHeader* header =
        sfhds_GetProcessedHeader(&handle->header_state);

    if (header->processed != 0) {
        return header->mux_version_major * 100 + header->mux_version_minor;
    }
    return 0;
}

#define SFHDS_QUERY(field, function, local) \
    do { \
        header->field = function(decoder, &local) == 0 ? -1 : local; \
    } while (0)

#define SFHDS_STREAM_QUERY(field, function, local) \
    do { \
        header->field = \
            function(decoder, (unsigned char)stream_id, &local) == 0 \
                ? -1 : local; \
    } while (0)

static void sfhds_DoProcessHdr(SFHHandle* decoder,
                               SfhdsProcessedHeader* header)
{
    int is_sfd_header;
    int byte_rate;
    int mux_version_major;
    int mux_version_minor;
    int private_stream_1_exists;
    int private_stream_2_exists;
    int private_stream;
    int audio_stream_exists;
    int video_stream_exists;
    int effective_features;
    int header_size;
    int pack_type;
    int packet_size_length;
    int pack_size;
    int element_count;
    int audio_element_count;
    int video_element_count;
    int private_element_count;
    int maximum_audio_length;
    int maximum_video_length;
    int maximum_frame_count;
    int audio_codec;
    int audio_layer;
    int audio_channel_count;
    int audio_sample_rate;
    int video_codec;
    int video_bit_rate;
    int video_picture_rate;
    int color_type;
    int picture_type;
    int fixed_flag;
    int sequence_header_fixed_flag;
    int expand;
    int gop_n;
    int gop_m;
    int mux_version;
    int stream_id;

    if (SFH_IsSfdHeader(decoder, &is_sfd_header) == 0) {
        is_sfd_header = 0;
    }
    if (is_sfd_header == 0) {
        return;
    }
    if (SFH_AnlyHdrToolVer(decoder, &mux_version_major,
                           &mux_version_minor) == 0) {
        mux_version_major = 0;
        mux_version_minor = 0;
    }
    header->mux_version_major = mux_version_major;
    header->mux_version_minor = mux_version_minor;
    mux_version = header->mux_version_major * 100 +
                  header->mux_version_minor;
    if (SFH_AnlyByteRate(decoder, &byte_rate) == 0) {
        byte_rate = 0;
    }
    if (mux_version < 110) {
        byte_rate = -byte_rate;
    }
    header->byte_rate = byte_rate;

    SFHDS_QUERY(header_size, SFH_AnlyHdrSiz, header_size);
    SFHDS_QUERY(pack_type, SFH_AnlyPackType, pack_type);
    SFHDS_QUERY(packet_size_length, SFH_AnlyPketSizLen,
                packet_size_length);
    if (header->packet_size_length == -1) {
        header->packet_size_length = 2;
    }
    SFHDS_QUERY(pack_size, SFH_AnlyPackSiz, pack_size);
    SFHDS_QUERY(element_count, SFH_AnlyNumElemTot, element_count);
    SFHDS_QUERY(audio_element_count, SFH_AnlyNumElemAud,
                audio_element_count);
    SFHDS_QUERY(video_element_count, SFH_AnlyNumElemVid,
                video_element_count);
    SFHDS_QUERY(private_element_count, SFH_AnlyNumElemPrv,
                private_element_count);
    SFHDS_QUERY(maximum_audio_length, SFH_AnlyMaxPlyLenAud,
                maximum_audio_length);
    SFHDS_QUERY(maximum_video_length, SFH_AnlyMaxPlyLenVid,
                maximum_video_length);
    SFHDS_QUERY(maximum_frame_count, SFH_AnlyMaxFrmNum,
                maximum_frame_count);

    if (SFH_IsExistStmId(decoder, 0xBD, &private_stream_1_exists) != 0 &&
        private_stream_1_exists != 0) {
        private_stream = 0xBD;
    } else {
        private_stream = 0;
    }
    header->private_stream_1 = private_stream;
    if (SFH_IsExistStmId(decoder, 0xBF, &private_stream_2_exists) != 0 &&
        private_stream_2_exists != 0) {
        private_stream = 0xBF;
    } else {
        private_stream = 0;
    }
    header->private_stream_2 = private_stream;

    stream_id = 0xC0;
    for (;;) {
        if (SFH_IsExistStmId(decoder, (unsigned char)stream_id,
                             &audio_stream_exists) != 0 &&
            audio_stream_exists != 0) {
            break;
        }
        stream_id++;
        if (stream_id > 0xDF) {
            stream_id = 0;
            break;
        }
    }
    header->audio_stream = stream_id;
    stream_id = 0xE0;
    for (;;) {
        if (SFH_IsExistStmId(decoder, (unsigned char)stream_id,
                             &video_stream_exists) != 0 &&
            video_stream_exists != 0) {
            break;
        }
        stream_id++;
        if (stream_id > 0xEF) {
            stream_id = 0;
            break;
        }
    }
    header->video_stream = stream_id;

    stream_id = header->audio_stream;
    if (stream_id != 0) {
        SFHDS_STREAM_QUERY(audio_codec, SFH_AnlyElemCodecAud, audio_codec);
        SFHDS_STREAM_QUERY(audio_layer, SFH_AnlyElemLayer, audio_layer);
        SFHDS_STREAM_QUERY(audio_channel_count, SFH_AnlyElemChNum,
                           audio_channel_count);
        SFHDS_STREAM_QUERY(audio_sample_rate, SFH_AnlyElemSmpHz,
                           audio_sample_rate);
    }

    stream_id = header->video_stream;
    SFHDS_STREAM_QUERY(video.codec, SFH_AnlyElemCodecVid, video_codec);
    SFHDS_STREAM_QUERY(video.bit_rate, SFH_AnlyElemBitRate, video_bit_rate);
    if (SFH_AnlyElemPicSz(decoder, (unsigned char)stream_id,
                          &header->video.width, &header->video.height) == 0) {
        header->video.width = -1;
        header->video.height = -1;
    }
    SFHDS_STREAM_QUERY(video.picture_rate, SFH_AnlyElemPicRate,
                       video_picture_rate);
    if (SFH_IsEffFtrInf(decoder, (unsigned char)stream_id,
                        &effective_features) == 0) {
        effective_features = 0;
    }
    header->video.has_effective_features = effective_features != 0;
    if (effective_features != 0) {
        SFHDS_STREAM_QUERY(video.color_type, SFH_AnlyFtrColType, color_type);
        SFHDS_STREAM_QUERY(video.picture_type, SFH_AnlyFtrPicType,
                           picture_type);
        SFHDS_STREAM_QUERY(video.fixed_flag, SFH_AnlyFtrFixFlg, fixed_flag);
        SFHDS_STREAM_QUERY(video.sequence_header_fixed_flag,
                           SFH_AnlyFtrShcFixFlg,
                           sequence_header_fixed_flag);
        SFHDS_STREAM_QUERY(video.expand, SFH_AnlyFtrExpand, expand);
        SFHDS_STREAM_QUERY(video.gop_n, SFH_AnlyFtrGopN, gop_n);
        SFHDS_STREAM_QUERY(video.gop_m, SFH_AnlyFtrGopM, gop_m);
    }
    header->processed = 1;
}

#undef SFHDS_QUERY
#undef SFHDS_STREAM_QUERY

#pragma dont_inline on
void SFHDS_ProcessHdr(SfdHeaderState* state)
{
    SFHHandle* decoder;
    SfhdsHeaderBlock* block;

    block = (SfhdsHeaderBlock*)state;
    decoder = SFH_Create(block->raw_header, block->processed.raw_header_size);

    if (decoder != 0) {
        sfhds_DoProcessHdr(decoder, &block->processed);
        SFH_Destroy(decoder);
    }
}
#pragma dont_inline reset

void SFHDS_ReprocessHdr(SfdHandle* handle)
{
    SfhdsHeaderBlock* seek_header = sfhds_GetSeekHeader(handle);

    if (seek_header != 0) {
        *sfhds_GetHeaderBlock(handle) = *seek_header;
        SFHDS_ProcessHdr(&handle->header_state);
        sfhds_PublishElementCounts(handle);
    }
}

static int sfhds_SetHdrRaw(SfdHandle* handle, const unsigned char* data,
                           int size)
{
    SfhdsHeaderCallback callback =
        (SfhdsHeaderCallback)SFSET_GetCond(handle, 0x4B);
    SfdCallbackObject callback_object = SFSET_GetCond(handle, 0x4C);
    SfhdsHeaderBlock* block;
    SfhdsHeaderBlock* seek_header;
    int copy_size;

    if (callback != 0) {
        callback(callback_object, data, size);
    }
    block = sfhds_GetHeaderBlock(handle);
    if (block->processed.processed != 0) {
        return 0;
    }
    copy_size = 0x800;
    if (size < 0x800) {
        copy_size = size;
    }
    MEM_Copy(block->raw_header, data, copy_size);
    block->processed.raw_header_size = copy_size;
    SFHDS_ProcessHdr(&handle->header_state);
    sfhds_PublishElementCounts(handle);
    seek_header = sfhds_GetSeekHeader(handle);
    if (seek_header != 0) {
        *seek_header = *block;
    }
    return 1;
}

#pragma dont_inline on
int SFHDS_SetHdr(SfdHandle* handle, int stream_index,
                 const unsigned char* data, int size, int* header_flag)
{
    const unsigned char* header;
    unsigned char start_byte_0;
    unsigned char start_byte_1;
    unsigned char prefix_byte_0;
    unsigned char prefix_byte_1;
    SFHHandle* decoder;
    int is_sfd_header;
    int header_valid;
    int header_size;
    unsigned int start_code;

    *header_flag = 0;
    if (stream_index != 2) {
        return 0;
    }
    header = data - 6;
    start_byte_0 = data[-6];
    start_byte_1 = header[1];
    header_size = size + 6;
    start_code = start_byte_0;
    start_code = (start_code << 8) | start_byte_1;
    start_code = (start_code << 8) | header[2];
    start_code = (start_code << 8) | header[3];
    if ((int)start_code != 0x1BF) {
        prefix_byte_0 = header[-2];
        prefix_byte_1 = header[-1];
        header -= 2;
        header_size += 2;
        start_code = prefix_byte_0;
        start_code = (start_code << 8) | prefix_byte_1;
        start_code = (start_code << 8) | start_byte_0;
        start_code = (start_code << 8) | start_byte_1;
        if ((int)start_code != 0x1BF) {
            return 0;
        }
    }
    header -= 12;
    header_size += 12;
    decoder = SFH_Create(header, header_size);
    if (decoder == 0) {
        header_valid = 0;
    } else {
        if (SFH_IsSfdHeader(decoder, &is_sfd_header) == 0) {
            is_sfd_header = 0;
        }
        SFH_Destroy(decoder);
        header_valid = is_sfd_header;
    }
    if (header_valid == 0) {
        return 0;
    }
    *header_flag = sfhds_SetHdrRaw(handle, header, header_size);
    return 1;
}
#pragma dont_inline reset

void SFHDS_FinishFhd(SfdHeaderState* state)
{
    SfhdsProcessedHeader* header = sfhds_GetProcessedHeader(state);

    header->processed = 0;
    header->byte_rate = 0;
    header->raw_header_size = 0;
}

void SFHDS_InitFhd(SfdHeaderState* state, int enabled)
{
    SfhdsProcessedHeader* header = sfhds_GetProcessedHeader(state);

    header->processed = 0;
    header->mux_version_major = 0;
    header->mux_version_minor = 0;
    header->byte_rate = 0;
    header->raw_header_size = 0;
}

void SFHDS_Finish(void)
{
    SFH_Finish();
}

void SFHDS_Init(void)
{
    SFH_Init(32, sfhds_sfhlib_work.handles);
}
