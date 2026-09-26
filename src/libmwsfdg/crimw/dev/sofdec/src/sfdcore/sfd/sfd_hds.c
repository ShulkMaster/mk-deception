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

typedef void (*SfhdsHeaderCallback)(SfdCallbackObject object,
                                    const void* data, int size);

extern int SFH_AnlyByteRate(SFHHandle*, int*);
extern int SFH_AnlyElemBitRate(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemChNum(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemCodecAud(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemCodecVid(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemLayer(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemPicRate(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyElemPicSz(SFHHandle*, unsigned char, int*, int*);
extern int SFH_AnlyElemSmpHz(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrColType(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrExpand(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrFixFlg(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrGopM(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrGopN(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrPicType(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyFtrShcFixFlg(SFHHandle*, unsigned char, int*);
extern int SFH_AnlyHdrSiz(SFHHandle*, int*);
extern int SFH_AnlyHdrToolVer(SFHHandle*, int*, int*);
extern int SFH_AnlyMaxFrmNum(SFHHandle*, int*);
extern int SFH_AnlyMaxPlyLenAud(SFHHandle*, int*);
extern int SFH_AnlyMaxPlyLenVid(SFHHandle*, int*);
extern int SFH_AnlyNumElemAud(SFHHandle*, int*);
extern int SFH_AnlyNumElemPrv(SFHHandle*, int*);
extern int SFH_AnlyNumElemTot(SFHHandle*, int*);
extern int SFH_AnlyNumElemVid(SFHHandle*, int*);
extern int SFH_AnlyPackSiz(SFHHandle*, int*);
extern int SFH_AnlyPackType(SFHHandle*, int*);
extern int SFH_AnlyPketSizLen(SFHHandle*, int*);
extern int SFH_IsEffFtrInf(SFHHandle*, unsigned char, int*);
extern int SFH_IsExistStmId(SFHHandle*, unsigned char, int*);
extern int SFH_IsSfdHeader(SFHHandle*, int*);
extern SFHHandle* SFH_Create(const void*, int);
extern void SFH_Destroy(SFHHandle*);
extern void SFH_Finish(void);
extern void SFH_Init(int, void*);
extern int SFMPS_GetConcatCnt(SfdHandle*);

static unsigned char sfhds_sfhlib_work[0x204];

static SfdHeaderState* sfhds_GetProcessedHeader(SfdHeaderState* state)
{
    return state;
}

static SfdHeaderState* sfhds_GetHeaderBlock(SfdHandle* handle)
{
    return &handle->header_state;
}

static SfdHeaderState* sfhds_GetSeekHeader(SfdHandle* handle)
{
    unsigned char* source;

    if (handle->seek_state.work == 0) {
        return 0;
    }
    if (SFMPS_GetConcatCnt(handle) > 0) {
        return 0;
    }
    source = (unsigned char*)handle->seek_state.work;
    return (SfdHeaderState*)(source + 0x0C);
}

static inline int sfhds_SearchStmId(SFHHandle* decoder, int first, int last,
                                    int* exists)
{
    int stream_id;

    for (stream_id = first; stream_id <= last; stream_id++) {
        if (SFH_IsExistStmId(decoder, (unsigned char)stream_id, exists) != 0 &&
            *exists != 0) {
            return stream_id;
        }
    }
    return 0;
}

static void sfhds_PublishElementCounts(SfdHandle* handle)
{
    SfdHeaderState* header =
        sfhds_GetProcessedHeader(&handle->header_state);

    handle->playback_settings.values_18[3] = header->audio_element_count;
    handle->playback_settings.values_18[4] = header->video_element_count;
    handle->playback_settings.values_18[5] = header->private_element_count;
}

int SFHDS_GetColType(SfdHandle* handle)
{
    SfdHeaderState* header =
        sfhds_GetProcessedHeader(&handle->header_state);
    SfdHeaderVideoInfo* video = &header->video;

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
    SfdHeaderState* header =
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

static void sfhds_DoProcessHdr(SFHHandle* decoder, SfdHeaderState* header)
{
    int version;
    int is_sfd_header;
    int byte_rate;
    int version_major;
    int version_minor;
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
    int stream_id;
    int video_codec_result;
    int video_bit_rate_result;
    int video_picture_rate_result;
    int video_color_type_result;
    int video_picture_type_result;
    int video_fixed_flag_result;
    int video_sequence_header_fixed_flag_result;
    int video_expand_result;
    int video_gop_n_result;
    int video_gop_m_result;

    if (SFH_IsSfdHeader(decoder, &is_sfd_header) == 0) {
        is_sfd_header = 0;
    }
    if (is_sfd_header == 0) {
        return;
    }
    if (SFH_AnlyHdrToolVer(decoder, &version_major, &version_minor) == 0) {
        version_major = 0;
        version_minor = 0;
    }
    header->mux_version_major = version_major;
    header->mux_version_minor = version_minor;
    version = header->mux_version_major * 100 + header->mux_version_minor;
    if (SFH_AnlyByteRate(decoder, &byte_rate) == 0) {
        byte_rate = 0;
    }
    if (version < 110) {
        byte_rate = -byte_rate;
    }
    header->byte_rate = byte_rate;

    header->header_size =
        SFH_AnlyHdrSiz(decoder, &header_size) == 0 ? -1 : header_size;
    header->pack_type =
        SFH_AnlyPackType(decoder, &pack_type) == 0 ? -1 : pack_type;
    header->packet_size_length =
        SFH_AnlyPketSizLen(decoder, &packet_size_length) == 0
            ? -1 : packet_size_length;
    if (header->packet_size_length == -1) {
        header->packet_size_length = 2;
    }
    header->pack_size =
        SFH_AnlyPackSiz(decoder, &pack_size) == 0 ? -1 : pack_size;
    header->element_count =
        SFH_AnlyNumElemTot(decoder, &element_count) == 0 ? -1 : element_count;
    header->audio_element_count =
        SFH_AnlyNumElemAud(decoder, &audio_element_count) == 0
            ? -1 : audio_element_count;
    header->video_element_count =
        SFH_AnlyNumElemVid(decoder, &video_element_count) == 0
            ? -1 : video_element_count;
    header->private_element_count =
        SFH_AnlyNumElemPrv(decoder, &private_element_count) == 0
            ? -1 : private_element_count;
    header->maximum_audio_length =
        SFH_AnlyMaxPlyLenAud(decoder, &maximum_audio_length) == 0
            ? -1 : maximum_audio_length;
    header->maximum_video_length =
        SFH_AnlyMaxPlyLenVid(decoder, &maximum_video_length) == 0
            ? -1 : maximum_video_length;
    header->maximum_frame_count =
        SFH_AnlyMaxFrmNum(decoder, &maximum_frame_count) == 0
            ? -1 : maximum_frame_count;

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

    header->audio_stream =
        sfhds_SearchStmId(decoder, 0xC0, 0xDF, &audio_stream_exists);
    header->video_stream =
        sfhds_SearchStmId(decoder, 0xE0, 0xEF, &video_stream_exists);

    stream_id = header->audio_stream;
    if (stream_id != 0) {
        header->audio.codec =
            SFH_AnlyElemCodecAud(decoder, (unsigned char)stream_id,
                                 &audio_codec) == 0 ? -1 : audio_codec;
        header->audio.layer =
            SFH_AnlyElemLayer(decoder, (unsigned char)stream_id,
                              &audio_layer) == 0 ? -1 : audio_layer;
        header->audio.channel_count =
            SFH_AnlyElemChNum(decoder, (unsigned char)stream_id,
                              &audio_channel_count) == 0
                ? -1 : audio_channel_count;
        header->audio.sample_rate =
            SFH_AnlyElemSmpHz(decoder, (unsigned char)stream_id,
                              &audio_sample_rate) == 0
                ? -1 : audio_sample_rate;
    }

    stream_id = header->video_stream;
    if (SFH_AnlyElemCodecVid(decoder, (unsigned char)stream_id,
                             &video_codec) == 0) {
        video_codec_result = -1;
    } else {
        video_codec_result = video_codec;
    }
    header->video.codec = video_codec_result;
    if (SFH_AnlyElemBitRate(decoder, (unsigned char)stream_id,
                            &video_bit_rate) == 0) {
        video_bit_rate_result = -1;
    } else {
        video_bit_rate_result = video_bit_rate;
    }
    header->video.bit_rate = video_bit_rate_result;
    if (SFH_AnlyElemPicSz(decoder, (unsigned char)stream_id,
                          &header->video.width, &header->video.height) == 0) {
        header->video.width = -1;
        header->video.height = -1;
    }
    if (SFH_AnlyElemPicRate(decoder, (unsigned char)stream_id,
                            &video_picture_rate) == 0) {
        video_picture_rate_result = -1;
    } else {
        video_picture_rate_result = video_picture_rate;
    }
    header->video.picture_rate = video_picture_rate_result;
    if (SFH_IsEffFtrInf(decoder, (unsigned char)stream_id,
                        &effective_features) == 0) {
        effective_features = 0;
    }
    header->video.has_effective_features = effective_features != 0;
    if (effective_features != 0) {
        if (SFH_AnlyFtrColType(decoder, (unsigned char)stream_id,
                              &color_type) == 0) {
            video_color_type_result = -1;
        } else {
            video_color_type_result = color_type;
        }
        header->video.color_type = video_color_type_result;
        if (SFH_AnlyFtrPicType(decoder, (unsigned char)stream_id,
                               &picture_type) == 0) {
            video_picture_type_result = -1;
        } else {
            video_picture_type_result = picture_type;
        }
        header->video.picture_type = video_picture_type_result;
        if (SFH_AnlyFtrFixFlg(decoder, (unsigned char)stream_id,
                              &fixed_flag) == 0) {
            video_fixed_flag_result = -1;
        } else {
            video_fixed_flag_result = fixed_flag;
        }
        header->video.fixed_flag = video_fixed_flag_result;
        if (SFH_AnlyFtrShcFixFlg(decoder, (unsigned char)stream_id,
                                 &sequence_header_fixed_flag) == 0) {
            video_sequence_header_fixed_flag_result = -1;
        } else {
            video_sequence_header_fixed_flag_result = sequence_header_fixed_flag;
        }
        header->video.sequence_header_fixed_flag =
            video_sequence_header_fixed_flag_result;
        if (SFH_AnlyFtrExpand(decoder, (unsigned char)stream_id,
                              &expand) == 0) {
            video_expand_result = -1;
        } else {
            video_expand_result = expand;
        }
        header->video.expand = video_expand_result;
        if (SFH_AnlyFtrGopN(decoder, (unsigned char)stream_id,
                           &gop_n) == 0) {
            video_gop_n_result = -1;
        } else {
            video_gop_n_result = gop_n;
        }
        header->video.gop_n = video_gop_n_result;
        if (SFH_AnlyFtrGopM(decoder, (unsigned char)stream_id,
                           &gop_m) == 0) {
            video_gop_m_result = -1;
        } else {
            video_gop_m_result = gop_m;
        }
        header->video.gop_m = video_gop_m_result;
    }
    header->processed = 1;
}

#undef SFHDS_QUERY
#undef SFHDS_STREAM_QUERY

#pragma dont_inline on
void SFHDS_ProcessHdr(SfdHeaderState* state)
{
    SFHHandle* decoder;
    SfdHeaderState* block;

    block = state;
    decoder = SFH_Create(block->raw_header, block->raw_header_size);

    if (decoder != 0) {
        sfhds_DoProcessHdr(decoder, block);
        SFH_Destroy(decoder);
    }
}
#pragma dont_inline reset

void SFHDS_ReprocessHdr(SfdHandle* handle)
{
    SfdHeaderState* seek_header = sfhds_GetSeekHeader(handle);

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
    SfdCallbackObject callback_object =
        (SfdCallbackObject)SFSET_GetCond(handle, 0x4C);
    SfdHeaderState* block;
    SfdHeaderState* seek_header;
    int copy_size;

    if (callback != 0) {
        callback(callback_object, data, size);
    }
    block = sfhds_GetHeaderBlock(handle);
    if (block->processed != 0) {
        return 0;
    }
    copy_size = sizeof(block->raw_header);
    if (size < copy_size) {
        copy_size = size;
    }
    MEM_Copy(block->raw_header, data, copy_size);
    block->raw_header_size = copy_size;
    SFHDS_ProcessHdr(&handle->header_state);
    sfhds_PublishElementCounts(handle);
    seek_header = sfhds_GetSeekHeader(handle);
    if (seek_header != 0) {
        *seek_header = *block;
    }
    return 1;
}

/* RE4 also uses a no-inline scope for its raw-header helper. Here a caller scope
 * preserves both the exact helper and the retail call; helper-only scope does not. */
#pragma dont_inline on
/* TODO: [blocked] 98.557144%; retail keeps a `mr r30, r7` copy of header_flag; RE4 needs an asm
 * register pin for it and a plain typed copy is propagated away (measured neutral). */
int SFHDS_SetHdr(SfdHandle* handle, int stream_index,
                 const unsigned char* data, int size, int* header_flag)
{
    const unsigned char* header;
    unsigned int start_byte_0;
    unsigned int start_byte_1;
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
    start_code <<= 8;
    start_code |= header[2];
    start_code <<= 8;
    start_code |= header[3];
    if ((int)start_code != 0x1BF) {
        prefix_byte_0 = header[-2];
        header -= 2;
        prefix_byte_1 = header[1];
        header_size += 2;
        start_code = prefix_byte_0;
        start_code = (start_code << 8) | prefix_byte_1;
        start_code <<= 8;
        start_code |= start_byte_0;
        start_code <<= 8;
        start_code |= start_byte_1;
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
    SfdHeaderState* header = sfhds_GetProcessedHeader(state);

    header->processed = 0;
    header->byte_rate = 0;
    header->raw_header_size = 0;
}

void SFHDS_InitFhd(SfdHeaderState* state)
{
    SfdHeaderState* header = sfhds_GetProcessedHeader(state);

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
    SFH_Init(32, sfhds_sfhlib_work);
}
