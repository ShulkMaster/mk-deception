#include "sofdec/mpv_mc.h"
#include "cri/mpv.h"
#include "sofdec/sfd_error.h"
#include "sofdec/sfd_mpvf.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_timer.h"
#include "sofdec/sfd_transport.h"
#include "sofdec/uty_mem.h"
#include "sofdec/uty_math.h"
#include "sofdec/uty_timer.h"

#include "runtime/cstring.h"

typedef MPVPlaneSet SfdYccPlane;

typedef struct SfdMpvDecoderInfo {
    unsigned char reserved[0x204];
} SfdMpvDecoderInfo;

typedef struct SfdMpvPictureUserBuffers {
    void* base;
    int buffer_count;
    int buffer_size;
    struct {
        void* buffer;
        int size;
    } entries[17];
} SfdMpvPictureUserBuffers;

typedef struct SfdMpvAuxWork {
    SfdMpvPictureUserBuffers picture_buffers;
    int reserved_1014;
    union {
        long long picture_pts;
        struct {
            int picture_pts_high;
            int picture_pts_low;
        } words;
    };
} SfdMpvAuxWork;

typedef struct SfdMpvPlaybackSettings {
    int width;
    int height;
    int macroblocks_per_row;
    int macroblock_rows;
    int bit_rate;
    int frame_rate_code;
    int values_18[2];
    int vbv_buffer_size;
    int values_24[7];
} SfdMpvPlaybackSettings;

typedef struct SfdMpvTimeCodeSnapshot {
    int valid;
    SfdTimeCode timecode;
    int value;
    int scale;
} SfdMpvTimeCodeSnapshot;

typedef struct SfdMpvDecodeTimer {
    unsigned char reserved_000[0x1C];
    SfdTimeCode current_timecode;
    SfdMpvTimeCodeSnapshot initial;
    SfdMpvTimeCodeSnapshot maximum;
    SfdMpvTimeCodeSnapshot reserved_snapshot;
    SfdMpvTimeCodeSnapshot current;
    SfdMpvTimeCodeSnapshot previous;
    SfdMpvTimeCodeSnapshot output_start;
} SfdMpvDecodeTimer;

typedef struct SfdMpvRepeatEntry {
    short repeat;
    short accumulated;
} SfdMpvRepeatEntry;

typedef struct SfdMpvRepeatTimer {
    unsigned char reserved_000[0x13C];
    int reference_time_origin;
    unsigned char reserved_140[0x24];
    int reference_time_offset;
    unsigned char reserved_168[0x11C];
    int maximum_reference_time;
    int maximum_reference_scale;
    unsigned char reserved_28C[0x224];
    SfdMpvRepeatEntry entries[64];
} SfdMpvRepeatTimer;

typedef SfdUserIsSkipFn SfdMpvSkipCallback;

typedef struct SfdMpvSkipTimer {
    unsigned char reserved_000[0x18];
    SfdMpvSkipCallback callback;
    unsigned char reserved_01C[0xC8];
    int current_value;
    int current_scale;
    unsigned char reserved_0EC[0x50];
    int base_value;
    unsigned char reserved_140[0x24];
    int time_offset;
} SfdMpvSkipTimer;

typedef char SfdMpvDecodeTimerSizeCheck[
    sizeof(SfdMpvDecodeTimer) == 0x144 ? 1 : -1];
typedef char SfdMpvRepeatTimerSizeCheck[
    sizeof(SfdMpvRepeatTimer) == 0x5B0 ? 1 : -1];
typedef char SfdMpvSkipTimerSizeCheck[
    sizeof(SfdMpvSkipTimer) == 0x168 ? 1 : -1];

typedef struct SfdMpvSeekCache {
    int valid;
    int byte_rate;
    int byte_rate_valid;
    SfdMpvTimeCodeSnapshot initial;
    unsigned char header[0x200];
    int header_size;
} SfdMpvSeekCache;

typedef int (*SfdMpvSizeCallback)(SfdCallbackObject object, int width,
                                  int height);
typedef void (*SfdMpvHeaderCallback)(SfdCallbackObject object,
                                     const void* data, int size);

extern const SfdTransportInterface SFD_tr_ad_adxt;

typedef struct SfdMpvTimerOverlay {
    unsigned char reserved_000[0x1C];
    SfdTimeCode reformed_timecode;
    unsigned char reserved_03C[0x2C];
    int source_valid;
    SfdTimeCode source_timecode;
} SfdMpvTimerOverlay;

int sfmpv_discard_wsiz;
static int sfmpv_picusr_buf1siz;
static int sfmpv_picusr_bufnum;
static void* sfmpv_picusr_pbuf;
static int sfmpv_ta_adr_tbl[16];
static int sfmpv_rfb_adr_tbl[2];
static int sfmpv_para[9];
static unsigned char sfmpv_work[0x12020];

static int SFMPV_Init(SfdHandle* handle);
static int SFMPV_Finish(SfdHandle* handle);
static int SFMPV_ExecServer(SfdHandle* handle);
static int SFMPV_Create(SfdHandle* handle);
static int SFMPV_Destroy(SfdHandle* handle);
static int SFMPV_Standby(SfdHandle* handle);
static int SFMPV_Start(SfdHandle* handle);
static int SFMPV_Stop(SfdHandle* handle);
static int SFMPV_Pause(SfdHandle* handle, int state);
static int SFMPV_GetWrite(SfdHandle* handle, void* output);
static int SFMPV_AddWrite(SfdHandle* handle, int parameter, int value);
static int SFMPV_GetRead(SfdHandle* handle, void* output);
static int SFMPV_AddRead(SfdHandle* handle, void* frame_data, int value);
static int SFMPV_Seek(SfdHandle* handle, int parameter, int value);

const SfdTransportInterface SFD_tr_vd_mpv = {
    SFMPV_Init,     SFMPV_Finish,   SFMPV_ExecServer, SFMPV_Create,
    SFMPV_Destroy,  SFMPV_Standby,  SFMPV_Start,      SFMPV_Stop,
    SFMPV_Pause,    SFMPV_GetWrite, SFMPV_AddWrite,   SFMPV_GetRead,
    (SfdTransportTransferFn)SFMPV_AddRead, SFMPV_Seek,
};

typedef struct SfdMpvDropFrameConversion {
    int frames_per_hour;
    int frames_per_ten_minutes;
    int frames_first_minute;
    int frames_other_minute;
    int first_minute_threshold;
    int nominal_frame_rate;
    int minutes_per_cycle;
    int dropped_frames;
} SfdMpvDropFrameConversion;

static const int sfmpv_fps_round[9] = {
    0, 24, 24, 25, 30, 30, 50, 60, 60,
};
static const SfdMpvDropFrameConversion sfmpv_conv_29_97 = {
    107892, 17982, 1800, 1798, 28, 30, 10, 2,
};
static const SfdMpvDropFrameConversion sfmpv_conv_59_94 = {
    215784, 35964, 3600, 3596, 56, 60, 10, 4,
};
const int gap_04_803180F4_rodata = 0;

static int sfmpv_InitInf(SfdHandle* handle, SfdMpvFrameWork* work);
static void sfmpv_SetFrmInf(SfdHandle* handle, SfdMpvFrame* frame,
                            SfdVideoFrameInfo** output);
int SFD_SetPicUsrBuf(SfdHandle* handle, void* buffer, int buffer_count,
                     int buffer_size);
void SFD_CalcYccPlane(void* buffer, int width, int height,
                      SfdYccPlane* output);

static inline int sfmpv_SetPicUsrBufSub(SfdHandle* handle,
                                        SfdMpvFrameWork* work, void* buffer,
                                        int buffer_count, int buffer_size)
{
    SfdMpvPictureUserBuffers* user =
        (SfdMpvPictureUserBuffers*)((unsigned char*)work + sizeof(*work));
    int i;
    int count;

    if (buffer == 0 || buffer_count == 0 || buffer_size == 0) {
        user->base = 0;
        user->buffer_count = 0;
        user->buffer_size = 0;
        for (i = 0; i < 17; i++) {
            user->entries[i].buffer = 0;
            user->entries[i].size = 0;
        }
        return 0;
    }
    if (buffer_count < handle->create_config.picture_user_buffer_minimum + 3) {
        return SFLIB_SetErr(handle, 0xFF000F1D);
    }
    user->base = buffer;
    user->buffer_count = buffer_count;
    user->buffer_size = buffer_size;
    user->entries[0].buffer = buffer;
    user->entries[0].size = 0;
    count = buffer_count - 1;
    i = 0;
    for (;;) {
        int limit = 16;
        if (count < 16) {
            limit = count;
        }
        if (i >= limit) {
            break;
        }
        user->entries[i + 1].buffer =
            (unsigned char*)user->entries[i].buffer + buffer_size;
        user->entries[i + 1].size = 0;
        i++;
    }
    return 0;
}

static inline const unsigned char* sfmpv_SearchTransferDelimiter(
    const SfdBufferTransfer* transfer, int delimiter_mask)
{
    const unsigned char* delimiter =
        MPV_SearchDelim(transfer->chunks[0].data, transfer->chunks[0].len,
                        delimiter_mask);

    if (delimiter == 0 && transfer->chunks[1].len != 0) {
        unsigned char boundary[6];
        int tail = transfer->chunks[0].len < 3 ? transfer->chunks[0].len : 3;
        int head = transfer->chunks[1].len < 3 ? transfer->chunks[1].len : 3;
        int i;

        memcpy(boundary,
               transfer->chunks[0].data + transfer->chunks[0].len - tail,
               tail);
        memcpy(boundary + tail, transfer->chunks[1].data, head);
        for (i = 0; i < tail + head - 3; i++) {
            if ((MPV_CheckDelim(boundary + i) & delimiter_mask) != 0) {
                return transfer->chunks[0].data +
                       transfer->chunks[0].len - tail + i;
            }
        }
        delimiter =
            MPV_SearchDelim(transfer->chunks[1].data,
                            transfer->chunks[1].len, delimiter_mask);
    }
    return delimiter;
}

static inline const unsigned char* sfmpv_BsearchTransferDelimiter(
    const SfdBufferTransfer* transfer, int delimiter_mask)
{
    const unsigned char* delimiter;

    if (transfer->chunks[1].len != 0) {
        delimiter = MPV_BsearchDelim(
            transfer->chunks[1].data + transfer->chunks[1].len,
            transfer->chunks[1].len, delimiter_mask);
        if (delimiter != 0) {
            return delimiter;
        }
    }
    if (transfer->chunks[1].len != 0) {
        unsigned char boundary[6];
        int tail = transfer->chunks[0].len < 3 ? transfer->chunks[0].len : 3;
        int head = transfer->chunks[1].len < 3 ? transfer->chunks[1].len : 3;
        int i;

        memcpy(boundary,
               transfer->chunks[0].data + transfer->chunks[0].len - tail,
               tail);
        memcpy(boundary + tail, transfer->chunks[1].data, head);
        for (i = tail + head - 4; i >= 0; i--) {
            if ((MPV_CheckDelim(boundary + i) & delimiter_mask) != 0) {
                return transfer->chunks[0].data +
                       transfer->chunks[0].len - tail + i;
            }
        }
    }
    return MPV_BsearchDelim(
        transfer->chunks[0].data + transfer->chunks[0].len,
        transfer->chunks[0].len, delimiter_mask);
}

static inline void sfmpv_InitFrame(SfdMpvFrame* frame, void* frame_buffer)
{
    frame->state = 0;
    frame->reference_count = 0;
    SFTIM_InitTtu(&frame->time_unit, 0);
    frame->frame_buffer = frame_buffer;
    frame->display_time_value = 0;
    frame->display_time_scale = 1;
    frame->field_40 = 0;
    frame->field_44 = 0;
    frame->picture_order = 0;
    frame->field_4C = 0;
    frame->field_50 = 0;
    UTY_MemsetDword((unsigned int*)frame->picture_info.bytes,
                    (unsigned int)-1, 0x20);
}

static inline void sfmpv_CalcYccPlaneSub(void* buffer, int width, int height,
                                         SfdYccPlane* output)
{
    int aligned_width = ((width + 15) / 16) * 16;
    int aligned_height = ((height + 15) / 16) * 16;
    int luma_stride = ((aligned_width + 31) / 32) * 32;
    int half_width = aligned_width / 2;
    int chroma_stride = ((half_width + 31) / 32) * 32;
    int half_height = aligned_height / 2;

    output->luma_stride = luma_stride;
    output->chroma_stride = chroma_stride;
    output->planes[2] = buffer;
    output->planes[0] = output->planes[2] + aligned_height * luma_stride;
    output->planes[1] = output->planes[0] + half_height * chroma_stride;
}

static int SFMPV_Seek(SfdHandle* handle, int parameter, int value)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvSeekCache* cache = 0;
    int restored = 0;
    int result = 0;

    (void)parameter;
    (void)value;
    if (handle->seek_state.source_handle != 0 && work->field_088 <= 0) {
        cache = (SfdMpvSeekCache*)((unsigned char*)
                    handle->seek_state.source_handle + 0xAD0);
    }
    if (cache != 0 && cache->valid != 0) {
        SJCK header;
        int consumed;

        timer->initial = cache->initial;
        header.data = cache->header;
        header.len = cache->header_size;
        if (MPV_DecodePicAtr(work->decoder, &header, &consumed) != 0) {
            result = SFLIB_SetErr(handle, 0xFF000F1B);
        } else {
            restored = 1;
        }
    }
    if (result != 0) {
        return result;
    }
    work->decode_state = 2;
    if (restored == 0 || SFSET_GetCond(handle, 48) == 0) {
        work->decode_mode = 0xC0;
    } else {
        work->decode_mode = 0xC8;
    }
    return 0;
}

static int SFMPV_AddRead(SfdHandle* handle, void* frame_data, int value)
{
    SfdVideoFrameState* video_frame =
        (SfdVideoFrameState*)((unsigned char*)frame_data - 8);
    SfdMpvFrameWork* work;
    int token;
    int result;

    SFLIB_LockCs(&token);
    work = (SfdMpvFrameWork*)handle->transports[2].context;
    if (video_frame->state != 1) {
        result = SFLIB_SetErr(handle, 0xFF000F0E);
    } else if (work->active_frame != SFMPVF_SearchFrmObj(handle, frame_data)) {
        result = SFLIB_SetErr(handle, 0xFF000F0F);
    } else {
        video_frame->state = 0;
        SFMPVF_EndDrawFrm(work->active_frame);
        result = 0;
    }
    SFLIB_UnlockCs(&token);
    return result;
}

static void sfmpv_SetFrmInf(SfdHandle* handle, SfdMpvFrame* frame,
                            SfdVideoFrameInfo** output)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdVideoFrameState* state = SFMPVF_SearchVfrmData(handle, frame);
    MPVPictureAttributes* picture = &frame->picture_info.decoded.attributes;

    *output = &state->data.info;
    state->state = 1;
    work->active_frame = frame;
    (*output)->width = picture->width;
    (*output)->height = picture->height;
    (*output)->macroblocks_per_row = picture->macroblocks_per_row;
    (*output)->macroblock_rows = picture->macroblock_rows;
    (*output)->picture_type = picture->picture_type;
    (*output)->display_time_value = frame->display_time_value;
    (*output)->display_time_scale = frame->display_time_scale;
    (*output)->output_format = handle->create_config.video_output_format;
    (*output)->frame_buffer = frame->frame_buffer;
    (*output)->field_24 = frame->field_40;
    (*output)->field_28 = frame->field_44;
    (*output)->picture_order = frame->picture_order;
    (*output)->field_30 = frame->field_4C;
    (*output)->field_34 = frame->field_50;
    (*output)->picture_user_buffer = frame->picture_user_buffer;
    (*output)->field_3C = picture->field_40;
    (*output)->field_40 = picture->field_44;
    (*output)->display_mode = picture->field_40 == 0 ? 2 : 1;
    (*output)->field_54 = frame->field_DC;
    (*output)->field_50 = frame->field_D8;
    (*output)->field_58 = picture->field_38;
    (*output)->field_5C = picture->field_3C;
    (*output)->field_60 = picture->field_48;
    (*output)->field_64 = picture->field_4C;
    (*output)->field_68 = picture->field_50;
    (*output)->field_6A = picture->field_52;
    (*output)->fields_6C[0] = picture->field_55;
    (*output)->fields_6C[1] = picture->field_56;
    (*output)->fields_6C[2] = picture->field_57;
    (*output)->fields_6C[3] = picture->field_59;
    (*output)->fields_6C[4] = picture->field_5A;
    (*output)->fields_6C[5] = picture->field_5B;
    (*output)->fields_6C[6] = picture->field_5C;
    (*output)->fields_6C[7] = picture->field_5D;
    (*output)->fields_6C[8] = picture->field_5E;
    (*output)->fields_6C[9] = picture->field_5F;
    (*output)->fields_6C[10] = picture->field_60;
    (*output)->fields_6C[11] = picture->field_61;
    (*output)->fields_6C[12] = picture->field_62;
    (*output)->fields_6C[13] = picture->field_63;
    (*output)->fields_6C[14] = picture->field_64;
}

static int SFMPV_GetRead(SfdHandle* handle, void* output_pointer)
{
    SfdVideoFrameInfo** output = output_pointer;
    SfdMpvFrame* frame;
    int sole_frame;

    if (SFMPVF_GetNumFrm(handle) == -1) {
        *output = 0;
        return 0;
    }
    frame = SFMPVF_HoldFrm(handle, &sole_frame);
    if (frame == 0) {
        *output = 0;
        return 0;
    }
    sfmpv_SetFrmInf(handle, frame, output);
    handle->timer_state.video_end_time_value = (*output)->display_time_value;
    handle->timer_state.video_end_time_scale = (*output)->display_time_scale;
    return 0;
}

static int SFMPV_AddWrite(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000F0D);
}

static int SFMPV_GetWrite(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000F0D);
}

static int SFMPV_Pause(SfdHandle* handle, int state)
{
    (void)state;
    return 0;
}

static int SFMPV_Stop(SfdHandle* handle)
{
    return 0;
}

static int SFMPV_Start(SfdHandle* handle)
{
    return 0;
}

static int SFMPV_Standby(SfdHandle* handle)
{
    return 0;
}

static int SFMPV_Destroy(SfdHandle* handle)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvPictureUserBuffers* user;
    MPVContext* decoder = work->decoder;
    int i;

    if (decoder == 0) {
        return 0;
    }
    for (i = 0; i < 9; i++) {
        sfmpv_para[i] = work->setup_values[i];
    }
    memcpy(sfmpv_rfb_adr_tbl, work->saved_pair,
           sizeof(sfmpv_rfb_adr_tbl));
    memcpy(sfmpv_ta_adr_tbl, work->address_table,
           sizeof(sfmpv_ta_adr_tbl));
    user = (SfdMpvPictureUserBuffers*)((unsigned char*)work + sizeof(*work));
    sfmpv_picusr_pbuf = user->base;
    sfmpv_picusr_bufnum = user->buffer_count;
    sfmpv_picusr_buf1siz = user->buffer_size;
    if (MPV_Destroy(decoder) != 0) {
        return SFLIB_SetErr(handle, 0xFF000F0C);
    }
    work->decoder = 0;
    return 0;
}

static void sfmpv_ErrFn(SfdCallbackObject object, int error)
{
    if (error == -1 || error < -3 || error >= 1) {
        SFLIB_SetErr((SfdHandle*)object, error);
    }
}

static int sfmpv_InitInf(SfdHandle* handle, SfdMpvFrameWork* work)
{
    SfdMpvPictureUserBuffers* user;
    int invalid = 0;
    int i;

    if (sfmpv_para[7] <= 0 || sfmpv_para[7] > 16) {
        invalid = 1;
    } else if (sfmpv_para[4] == 0 || sfmpv_para[8] == 0) {
        if (sfmpv_rfb_adr_tbl[0] == 0 || sfmpv_rfb_adr_tbl[1] == 0) {
            invalid = 1;
        } else {
            for (i = 0; i < sfmpv_para[7]; i++) {
                if (sfmpv_ta_adr_tbl[i] == 0) {
                    invalid = 1;
                    break;
                }
            }
        }
    }
    if (invalid != 0) {
        return SFLIB_SetErr(0, 0xFF000F15);
    }

    memcpy(work->setup_values, sfmpv_para, sizeof(sfmpv_para));
    memcpy(work->saved_pair, sfmpv_rfb_adr_tbl,
           sizeof(sfmpv_rfb_adr_tbl));
    memcpy(work->address_table, sfmpv_ta_adr_tbl,
           sizeof(sfmpv_ta_adr_tbl));
    work->decoder = 0;
    work->active_frame = 0;
    work->decode_state = 5;
    work->decode_mode = 0xC0;
    work->plane_indices[0] = 0;
    work->plane_indices[1] = 1;
    work->decoder_terminated = 0;
    work->gop_state = 0;
    work->reference_frames[0] = 0;
    work->reference_frames[1] = 0;
    work->pending_frame = 0;
    work->frame_state[0] = 0;
    work->frame_state[1] = 0;
    work->frame_state[2] = 0;
    for (i = 0; i < 16; i++) {
        SfdMpvFrame* frame = &work->frames[i];
        frame->state = 0;
        frame->reference_count = 0;
        SFTIM_InitTtu(&frame->time_unit, 0);
        frame->frame_buffer = work->address_table[i];
        frame->display_time_value = 0;
        frame->display_time_scale = 1;
        frame->field_40 = 0;
        frame->field_44 = 0;
        frame->picture_order = 0;
        frame->field_4C = 0;
        frame->field_50 = 0;
        UTY_MemsetDword((unsigned int*)frame->picture_info.bytes,
                        (unsigned int)-1, 0x20);
    }
    work->field_084 = 0;
    work->field_088 = 0;
    UTY_MemsetDword((unsigned int*)&work->picture_info, (unsigned int)-1,
                    0x20);
    work->field_10C = -1;
    work->field_110 = 0;
    work->active_size_threshold = 0x7FFFFFFF;
    work->field_118 = 0;
    work->field_11C = 0;
    work->field_120 = 0;
    work->pts_entry.pts = -1;
    work->pts_entry.data = 0;
    work->pts_entry.size = -1;

    user = (SfdMpvPictureUserBuffers*)((unsigned char*)work + sizeof(*work));
    user->base = 0;
    user->buffer_count = 0;
    user->buffer_size = 0;
    for (i = 0; i < 17; i++) {
        user->entries[i].buffer = 0;
        user->entries[i].size = 0;
    }
    for (i = 0; i < 16; i++) {
        work->frames[i].picture_user_buffer = &user->entries[i + 1].buffer;
    }
    return 0;
}

static int SFMPV_Create(SfdHandle* handle)
{
    SfdMpvFrameWork* work;
    MPVContext* decoder;
    int result;

    if (SFSET_GetCond(handle, 5) == 0) {
        return 0;
    }
    work = (SfdMpvFrameWork*)handle->mpv_work_storage;
    handle->transports[2].context = work;
    result = sfmpv_InitInf(handle, work);
    if (result != 0) {
        return result;
    }
    decoder = MPV_Create();
    if (decoder == 0) {
        return SFLIB_SetErr(0, 0xFF000F0A);
    }
    if (MPV_SetErrFunc(decoder, sfmpv_ErrFn,
                       (SfdCallbackObject)handle) != 0) {
        MPV_Destroy(decoder);
        return SFLIB_SetErr(0, 0xFF000F0B);
    }
    MPV_SetCond(decoder, 1, SFSET_GetCond(handle, 0));
    MPV_SetCond(decoder, 2, SFSET_GetCond(handle, 1));
    MPV_SetCond(decoder, 6, handle->create_config.video_output_format);
    work->decoder = decoder;
    if (SFPLY_GetResetFlg() != 0) {
        sfmpv_SetPicUsrBufSub(handle, work, sfmpv_picusr_pbuf,
                              sfmpv_picusr_bufnum, sfmpv_picusr_buf1siz);
    }
    return 0;
}

static int sfmpv_GoDdelim(SfdHandle* handle, int delimiter_mask)
{
    SfdBufferTransfer transfer;
    const unsigned char* delimiter;
    int consumed;
    int i;
    int nonzero = 0;

    if (SFBUF_RingGetRead(handle, handle->transports[2].parameter_10,
                          &transfer) != 0 ||
        transfer.chunks[0].len == 0) {
        return 0;
    }
    delimiter = MPV_SearchDelim(transfer.chunks[0].data,
                                transfer.chunks[0].len, delimiter_mask);
    if (delimiter == 0 && transfer.chunks[1].len != 0) {
        unsigned char boundary[6];
        int tail = transfer.chunks[0].len < 3 ? transfer.chunks[0].len : 3;
        int head = transfer.chunks[1].len < 3 ? transfer.chunks[1].len : 3;

        memcpy(boundary,
               transfer.chunks[0].data + transfer.chunks[0].len - tail,
               tail);
        memcpy(boundary + tail, transfer.chunks[1].data, head);
        for (i = 0; i < tail + head - 3; i++) {
            if ((MPV_CheckDelim(boundary + i) & delimiter_mask) != 0) {
                delimiter = transfer.chunks[0].data +
                            transfer.chunks[0].len - tail + i;
                break;
            }
        }
        if (delimiter == 0) {
            delimiter = MPV_SearchDelim(transfer.chunks[1].data,
                                        transfer.chunks[1].len,
                                        delimiter_mask);
        }
    }
    if (delimiter == 0) {
        consumed = transfer.chunks[0].len + transfer.chunks[1].len - 3;
        if (consumed < 0) {
            consumed = 0;
        }
    } else if (delimiter >= transfer.chunks[0].data &&
               delimiter < transfer.chunks[0].data +
                               transfer.chunks[0].len) {
        consumed = delimiter - transfer.chunks[0].data;
    } else if (delimiter >= transfer.chunks[1].data &&
               delimiter < transfer.chunks[1].data +
                               transfer.chunks[1].len) {
        consumed = transfer.chunks[0].len +
                   (delimiter - transfer.chunks[1].data);
    } else {
        consumed = 0;
    }
    SFBUF_RingAddRead(handle, handle->transports[2].parameter_10, consumed);
    for (i = 0; i < consumed && i < 3; i++) {
        const unsigned char* byte =
            i < transfer.chunks[0].len
                ? transfer.chunks[0].data + i
                : transfer.chunks[1].data + i - transfer.chunks[0].len;
        if (*byte != 0) {
            nonzero = 1;
            break;
        }
    }
    if (nonzero != 0) {
        handle->playback_runtime.time_values[5] += consumed;
    }
    handle->playback_runtime.time_values[4] += consumed;
    return consumed;
}

static int sfmpv_SetFrmPara(SfdHandle* handle,
                            const MPVPictureInfo* picture,
                            MPVFrameBuffers* buffers,
                            SfdMpvFrame** output_frame)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvAuxWork* aux =
        (SfdMpvAuxWork*)((unsigned char*)work + sizeof(*work));
    SfdMpvFrame* frame = work->pending_frame;

    if (frame == 0) {
        frame = SFMPVF_AllocFrm(handle);
        if (frame == 0) {
            handle->playback_runtime.field_28 = 1;
            return -1;
        }
    }
    *output_frame = frame;
    frame->picture_info.decoded = *picture;
    frame->field_DC = aux->words.picture_pts_low;
    frame->field_D8 = aux->words.picture_pts_high;

    if (handle->create_config.video_output_format == 3) {
        int picture_type = picture->attributes.picture_type;
        if ((picture_type == 1 || picture_type == 2) &&
            work->pending_frame == 0) {
            SFMPVF_EndRefFrm(work->reference_frames[0]);
            work->reference_frames[0] = work->reference_frames[1];
            work->reference_frames[1] = frame;
        }
        {
            int width = ((picture->attributes.width + 15) / 16) * 16;
            int height = ((picture->attributes.height + 15) / 16) * 16;
            int luma_stride = ((width + 31) / 32) * 32;
            int chroma_stride = (((width / 2) + 31) / 32) * 32;
            int luma_size = height * luma_stride;
            int chroma_size = (height / 2) * chroma_stride;

            buffers->forward.luma_stride = luma_stride;
            buffers->forward.chroma_stride = chroma_stride;
            buffers->forward.planes[2] =
                work->reference_frames[0]->frame_buffer;
            buffers->forward.planes[0] =
                buffers->forward.planes[2] + luma_size;
            buffers->forward.planes[1] =
                buffers->forward.planes[0] + chroma_size;
            buffers->backward.luma_stride = luma_stride;
            buffers->backward.chroma_stride = chroma_stride;
            buffers->backward.planes[2] =
                work->reference_frames[1]->frame_buffer;
            buffers->backward.planes[0] =
                buffers->backward.planes[2] + luma_size;
            buffers->backward.planes[1] =
                buffers->backward.planes[0] + chroma_size;
        }
    } else {
        int picture_type = picture->attributes.picture_type;
        if (picture_type == 1 || picture_type == 2) {
            work->plane_indices[0] ^= 1;
            work->plane_indices[1] ^= 1;
            work->reference_frames[1] = frame;
        }
        buffers->forward = work->planes[work->plane_indices[0]];
        buffers->backward = work->planes[work->plane_indices[1]];
    }
    buffers->output_rfb = frame->frame_buffer;
    buffers->picture_info = &frame->picture_info.decoded;
    buffers->decoded_dct_count = 0;
    buffers->skipped_dct_count = 0;
    handle->playback_runtime.field_28 = 0;
    return 0;
}

typedef struct SfdMpvPictureUserData {
    void* buffer;
    int size;
} SfdMpvPictureUserData;

static int sfmpv_DecodeFrm(SfdHandle* handle, SJ* stream)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvAuxWork* aux =
        (SfdMpvAuxWork*)((unsigned char*)work + sizeof(*work));
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    MPVPictureInfo* picture = &work->picture_info;
    MPVFrameBuffers buffers;
    SfdMpvFrame* frame;
    SfdMpvTimeCodeSnapshot* frame_time;
    unsigned long long start;
    int decoded_base = 0;
    int skipped_base = 0;
    int flow_before;
    int consumed;
    int result;

    if (sfmpv_SetFrmPara(handle, picture, &buffers, &frame) != 0) {
        return 0;
    }
    if (handle->create_config.video_output_format == 3) {
        switch (work->frame_state[1]) {
        case 2:
            decoded_base = work->reference_frames[0]->field_40;
            skipped_base = work->reference_frames[0]->field_44;
            break;
        case 3:
            decoded_base = work->reference_frames[0]->field_40 +
                           work->reference_frames[1]->field_40;
            skipped_base = work->reference_frames[0]->field_44 +
                           work->reference_frames[1]->field_44;
            break;
        default:
            break;
        }
    }

    if (frame->picture_user_buffer != 0) {
        SfdMpvPictureUserData* user_data =
            (SfdMpvPictureUserData*)frame->picture_user_buffer;
        memcpy(user_data->buffer, aux->picture_buffers.entries[0].buffer,
               aux->picture_buffers.entries[0].size);
        user_data->size = aux->picture_buffers.entries[0].size;
    }
    if (timer->output_start.valid == 0) {
        SfdTimeCode timecode = timer->current.timecode;
        int mode = work->decode_state == 2 || work->decode_state == 3;

        if (mode == 0 && handle->playback_runtime.field_2C != 0) {
            timecode.frame_offset = 0;
        }
        timer->output_start.timecode = timecode;
        SFTIM_Tc2Time(&timecode, &timer->output_start.value,
                      &timer->output_start.scale);
        timer->output_start.value -= timer->initial.value;
        timer->output_start.valid = 1;
    }

    start = UTY_GetTmr();
    flow_before = SJRBF_GetFlowCnt(stream, 0, 1);
    result = MPV_DecodeFrmSj(work->decoder, stream, &buffers);
    consumed = SJRBF_GetFlowCnt(stream, 0, 1) - flow_before;
    SFTMR_AddTsum(&handle->timer_summaries[picture->attributes.picture_type],
                  UTY_GetTmr() - start);
    handle->error_info.field_0C += buffers.decoded_dct_count;
    handle->error_info.field_10 += buffers.skipped_dct_count;
    switch (result) {
    case 0:
        result = 0;
        break;
    case -2:
    case -3:
        result = consumed > 0 ? 0 : SFLIB_SetErr(handle, result);
        break;
    default:
        result = SFLIB_SetErr(handle, 0xFF000F06);
        break;
    }
    SFBUF_AddRtotSj(handle, handle->transports[2].parameter_10, consumed);
    handle->playback_runtime.time_values[4] += consumed;
    if (result != 0) {
        SFMPVF_FreeFrm(frame);
        return result;
    }
    if (consumed > 0) {
        SfdMpvRepeatTimer* repeat_timer =
            (SfdMpvRepeatTimer*)&handle->timer_state;

        SFMPVF_SetGopStat(handle, 0);
        frame_time = (SfdMpvTimeCodeSnapshot*)&frame->time_unit;
        *frame_time = timer->current;
        frame->display_time_scale = frame_time->scale;
        frame->display_time_value = repeat_timer->reference_time_offset +
            (frame_time->value - repeat_timer->reference_time_origin);
        frame->field_4C = frame_time->value;
        frame->field_50 =
            frame_time->value + repeat_timer->reference_time_offset;
        if (repeat_timer->maximum_reference_time <
            frame->display_time_value) {
            repeat_timer->maximum_reference_time = frame->display_time_value;
            repeat_timer->maximum_reference_scale = frame->display_time_scale;
        }
        frame->picture_order = work->field_088;
        frame->field_40 = buffers.decoded_dct_count + decoded_base +
                          work->frame_state[2];
        frame->field_44 = buffers.skipped_dct_count + skipped_base;
        if (picture->attributes.field_38 != 3 && work->pending_frame == 0) {
            work->pending_frame = frame;
        } else {
            work->pending_frame = 0;
        }
        work->frame_state[0] = 0;
        work->frame_state[1] = 0;
        if (work->pending_frame == 0) {
            if (handle->create_config.video_output_format == 3 &&
                (picture->attributes.picture_type == 1 ||
                 picture->attributes.picture_type == 2)) {
                SFMPVF_RefStbyFrm(frame);
            } else {
                SFMPVF_StbyFrm(frame);
            }
            MPV_GetDctCnt(work->decoder, &handle->playback_runtime.field_08,
                          &handle->playback_runtime.field_0C);
            work->field_084 = 0;
        }
        SFPLY_AddDecPic(handle, 1, picture->attributes.picture_type);
    } else if (work->pending_frame == 0) {
        SFMPVF_FreeFrm(frame);
    }
    return 0;
}

static int sfmpv_IsSkip(SfdHandle* handle, const SJCK* chunk)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvSkipTimer* skip_timer =
        (SfdMpvSkipTimer*)&handle->timer_state;
    int picture_type = work->picture_info.attributes.picture_type;
    int skip;
    int decode_state;

    if (SFSET_GetCond(handle, 47) == 1) {
        return 1;
    }
    if (SFSET_GetCond(handle, 39) == 1) {
        return 0;
    }
    if ((signed char)work->picture_info.attributes.field_58 != 0) {
        return work->frame_state[0];
    }

    if (handle->seek_state.field_08 >= 0 &&
        timer->output_start.valid == 0 &&
        UTY_CmpTime(handle->seek_state.field_08,
                    handle->seek_state.field_0C,
                    timer->current.value, timer->current.scale) == 0) {
        skip = 1;
    } else {
        int unavailable;
        switch (picture_type) {
        case 1:
            unavailable = handle->conditions_primary[2] == 0;
            break;
        case 2:
            unavailable = handle->conditions_primary[3] == 0;
            break;
        case 3:
            unavailable = handle->conditions_primary[4] == 0;
            break;
        default:
            unavailable = 1;
            break;
        }
        if (unavailable) {
            skip = 1;
        } else {
            int empty = 0;
            if (SFSET_GetCond(handle, 7) == 0) {
                int macroblock_count =
                    ((SfdMpvPlaybackSettings*)&handle->playback_settings)
                        ->macroblocks_per_row *
                    ((SfdMpvPlaybackSettings*)&handle->playback_settings)
                        ->macroblock_rows;
                if (picture_type == 3) {
                    empty = MPV_IsEmptyBpic(chunk->data, chunk->len,
                                            macroblock_count);
                    if (empty) {
                        handle->playback_runtime.field_10++;
                    }
                } else if (picture_type == 2) {
                    empty = MPV_IsEmptyPpic(chunk->data, chunk->len,
                                            macroblock_count);
                    if (empty) {
                        handle->playback_runtime.field_14++;
                    }
                }
            }
            if (empty) {
                skip = 1;
            } else {
                int forced = 0;
                if (work->decode_state == 2 &&
                    (picture_type == 2 || picture_type == 3)) {
                    forced = 1;
                } else if (work->decode_state == 3 && picture_type == 3) {
                    forced = 1;
                }
                if (forced) {
                    skip = 1;
                } else {
                    int target = timer->output_start.valid == 0
                                     ? 0
                                     : skip_timer->time_offset +
                                           (skip_timer->current_value -
                                            skip_timer->base_value);
                    int scale = skip_timer->current_scale;

                    if (skip_timer->callback != 0) {
                        skip = skip_timer->callback(handle, picture_type,
                                                    target, scale);
                    } else {
                        int value;
                        int current_scale;

                        if (picture_type == 1) {
                            SFTIM_UpdateItime((SfdTimerState*)skip_timer,
                                              target);
                        }
                        if (picture_type == 1 || picture_type == 2) {
                            target = SFTIM_GetNextItime(
                                (SfdTimerState*)skip_timer, target);
                        }
                        if (SFTIM_GetSpeed(handle) <= 1000 &&
                            work->field_084 >=
                                handle->conditions_primary[36]) {
                            skip = 0;
                        } else {
                            SFTIM_GetTime(handle, &value, &current_scale);
                            target -= scale *
                                      handle->conditions_primary[40] /
                                      handle->conditions_primary[41];
                            if (value < 0 ||
                                UTY_CmpTime(value, current_scale, target,
                                            scale) != 0) {
                                skip = 0;
                            } else {
                                skip = 1;
                                work->field_084++;
                            }
                        }
                    }
                }
            }
        }
    }

    decode_state = work->decode_state;
    if (work->field_110 != 0) {
        int first;
        int second;
        MPV_GetLinkFlg(work->decoder, &first, &second);
        if (first == 1) {
            decode_state = 5;
        } else {
            if (timer->output_start.valid == 0 &&
                SFSET_GetCond(handle, 73) == 1) {
                second = 1;
            }
            if (second == 1) {
                decode_state = 2;
            }
        }
    }
    if (skip == 1) {
        if (picture_type == 1 || picture_type == 2) {
            decode_state = 2;
        }
    } else if (decode_state == 2) {
        decode_state = 3;
    } else if (decode_state == 3) {
        decode_state = 5;
    }
    work->decode_state = decode_state;
    return skip;
}

static int sfmpv_ChkBufSiz(SfdHandle* handle,
                           const SfdMpvPlaybackSettings* settings)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    int width = ((settings->width + 15) / 16) * 16;
    int height = ((settings->height + 15) / 16) * 16;
    int configured_width = ((work->setup_values[2] + 15) / 16) * 16;
    int configured_height = ((work->setup_values[3] + 15) / 16) * 16;
    int luma_stride = ((width + 31) / 32) * 32;
    int chroma_stride = (((width / 2) + 31) / 32) * 32;
    int configured_luma_stride =
        ((configured_width + 31) / 32) * 32;
    int configured_chroma_stride =
        (((configured_width / 2) + 31) / 32) * 32;
    int frame_size = height * luma_stride +
                     (height / 2) * chroma_stride * 2 + 0x20;
    int configured_size =
        configured_height * configured_luma_stride +
        (configured_height / 2) * configured_chroma_stride * 2 + 0x20;
    int frame_count;
    int i;
    unsigned char* buffer0;
    unsigned char* buffer1;

    if (frame_size * 2 > configured_size * 2) {
        return SFLIB_SetErr(handle, 0xFF000F17);
    }
    if (work->setup_values[8] == 0) {
        frame_count = work->setup_values[7];
    } else {
        int available_size = work->setup_values[7] * configured_size;
        int used_size = frame_size;

        frame_count = 0;
        while (frame_count < 16 && used_size <= available_size) {
            frame_count++;
            used_size += frame_size;
        }
        if (frame_count < work->setup_values[7]) {
            return SFLIB_SetErr(handle, 0xFF000F17);
        }
        work->saved_pair[0] = work->setup_values[4];
        work->saved_pair[1] = work->setup_values[4] + frame_size;
        for (i = 0; i < frame_count; i++) {
            work->address_table[i] =
                (unsigned char*)work->setup_values[8] + i * frame_size;
        }
    }

    buffer0 = (unsigned char*)work->saved_pair[0];
    buffer1 = (unsigned char*)work->saved_pair[1];
    work->planes[0].luma_stride = luma_stride;
    work->planes[0].chroma_stride = chroma_stride;
    work->planes[0].planes[2] = buffer0;
    work->planes[0].planes[0] = buffer0 + height * luma_stride;
    work->planes[0].planes[1] =
        work->planes[0].planes[0] + (height / 2) * chroma_stride;
    work->planes[1].luma_stride = luma_stride;
    work->planes[1].chroma_stride = chroma_stride;
    work->planes[1].planes[2] = buffer1;
    work->planes[1].planes[0] = buffer1 + height * luma_stride;
    work->planes[1].planes[1] =
        work->planes[1].planes[0] + (height / 2) * chroma_stride;
    if (handle->create_config.video_output_format == 3) {
        int decoded_count = frame_count < 14 ? frame_count : 14;
        work->frame_count = decoded_count + 2;
        sfmpv_InitFrame(&work->frames[0], (void*)work->saved_pair[0]);
        sfmpv_InitFrame(&work->frames[1], (void*)work->saved_pair[1]);
        for (i = 0; i < decoded_count; i++) {
            sfmpv_InitFrame(&work->frames[i + 2], work->address_table[i]);
        }
        work->reference_frames[0] = SFMPVF_AllocFrm(handle);
        work->reference_frames[1] = SFMPVF_AllocFrm(handle);
    } else {
        int decoded_count = frame_count < 16 ? frame_count : 16;
        work->frame_count = decoded_count;
        for (i = 0; i < decoded_count; i++) {
            sfmpv_InitFrame(&work->frames[i], work->address_table[i]);
        }
    }
    return 0;
}

static void sfmpv_Pts2Tc(long long pts, int frame_rate_code, int drop_frame,
                         int frame_offset, SfdTimeCode* timecode)
{
    const int rate = SFTIM_prate[frame_rate_code];
    const int nominal_rate = sfmpv_fps_round[frame_rate_code];
    unsigned int doubled_frames =
        (unsigned int)UTY_MulDivRound64(pts, rate * 2, 90000000);
    int frames = (int)(doubled_frames >> 1) - frame_offset;
    int hours;
    int minutes;
    int seconds;
    int pictures;

    timecode->subframe = doubled_frames & 1;
    timecode->frame_rate_code = frame_rate_code;
    timecode->drop_frame = drop_frame;
    if (frames < 0) {
        frames = 0;
    }
    if (drop_frame != 0 && (rate == 29970 || rate == 59940)) {
        const SfdMpvDropFrameConversion* conversion =
            rate == 29970 ? &sfmpv_conv_29_97 : &sfmpv_conv_59_94;
        int within_hour = frames % conversion->frames_per_hour;
        int within_ten_minutes =
            within_hour % conversion->frames_per_ten_minutes;
        int minute_in_cycle;

        hours = frames / conversion->frames_per_hour;
        if (within_ten_minutes < conversion->frames_first_minute) {
            minutes = 0;
            seconds =
                within_ten_minutes / conversion->nominal_frame_rate;
            pictures =
                within_ten_minutes % conversion->nominal_frame_rate;
        } else {
            int after_first =
                within_ten_minutes - conversion->frames_first_minute;
            minutes = after_first / conversion->frames_other_minute + 1;
            after_first %= conversion->frames_other_minute;
            if (after_first < conversion->first_minute_threshold) {
                seconds = 0;
                pictures = after_first + conversion->dropped_frames;
            } else {
                after_first -= conversion->dropped_frames;
                seconds =
                    after_first / conversion->nominal_frame_rate + 1;
                pictures =
                    after_first % conversion->nominal_frame_rate;
            }
        }
        minute_in_cycle =
            within_hour / conversion->frames_per_ten_minutes;
        minutes += conversion->minutes_per_cycle * minute_in_cycle;
    } else {
        hours = frames / nominal_rate / 60 / 60;
        minutes = (frames / nominal_rate / 60) % 60;
        seconds = (frames / nominal_rate) % 60;
        pictures = frames % nominal_rate;
    }
    timecode->hours = hours;
    timecode->minutes = minutes;
    timecode->seconds = seconds;
    timecode->frames = pictures;
}

static void sfmpv_DoReformTc(SfdHandle* handle,
                             const MPVPictureInfo* picture, long long pts,
                             int group_changed)
{
    SfdMpvTimerOverlay* timer =
        (SfdMpvTimerOverlay*)&handle->timer_state;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    const MPVPictureAttributes* attributes = &picture->attributes;

    if (group_changed != 0 && pts >= 0) {
        sfmpv_Pts2Tc(pts, attributes->frame_rate_code,
                     attributes->drop_frame_flag,
                     attributes->temporal_reference,
                     &timer->reformed_timecode);
        return;
    }
    if (timer->source_valid == 0) {
        if (handle->seek_state.source_handle == 0) {
            timer->reformed_timecode.frame_rate_code =
                attributes->frame_rate_code;
            timer->reformed_timecode.drop_frame = 0;
            timer->reformed_timecode.hours = 0;
            timer->reformed_timecode.minutes = 0;
            timer->reformed_timecode.seconds = 0;
            timer->reformed_timecode.frames = 0;
        }
        return;
    }
    if (group_changed != 0) {
        SfdTimeCode* source = &timer->source_timecode;
        SfdTimeCode* output = &timer->reformed_timecode;
        int subframes = source->reserved_1C + source->subframe;
        int frame = source->frames + source->frame_offset + 1 +
                    subframes / 2;
        int added_seconds =
            frame / sfmpv_fps_round[source->frame_rate_code];
        int total_seconds = source->seconds + added_seconds;
        int total_minutes = source->minutes + total_seconds / 60;
        int hours = source->hours + total_minutes / 60;
        int minutes = total_minutes % 60;
        int seconds = total_seconds % 60;

        frame %= sfmpv_fps_round[source->frame_rate_code];
        if (source->drop_frame != 0 && seconds == 0 &&
            minutes % 10 != 0 && (frame == 0 || frame == 1)) {
            frame = 2;
        }
        output->frame_rate_code = source->frame_rate_code;
        output->drop_frame = source->drop_frame;
        output->hours = hours;
        output->minutes = minutes;
        output->seconds = seconds;
        output->frames = frame;
        output->subframe = subframes % 2;
        repeat_timer->entries[0].accumulated = output->subframe;
        repeat_timer->entries[attributes->temporal_reference].accumulated =
            output->subframe;
    } else {
        timer->reformed_timecode.frame_rate_code =
            timer->source_timecode.frame_rate_code;
        timer->reformed_timecode.drop_frame =
            timer->source_timecode.drop_frame;
        timer->reformed_timecode.hours = timer->source_timecode.hours;
        timer->reformed_timecode.minutes = timer->source_timecode.minutes;
        timer->reformed_timecode.seconds = timer->source_timecode.seconds;
        timer->reformed_timecode.frames = timer->source_timecode.frames;
    }
}

static void sfmpv_CalcRepeatField(SfdHandle* handle,
                                  const MPVPictureInfo* picture,
                                  int group_changed)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    const MPVPictureAttributes* attributes = &picture->attributes;
    SfdMpvRepeatEntry* entries = repeat_timer->entries;
    int temporal_reference = attributes->temporal_reference;
    int index;
    int i;

    timer->current_timecode.frame_rate_code = attributes->frame_rate_code;
    timer->current_timecode.drop_frame = attributes->drop_frame_flag;
    timer->current_timecode.hours = attributes->time_code_hours;
    timer->current_timecode.minutes = attributes->time_code_minutes;
    timer->current_timecode.seconds = attributes->time_code_seconds;
    timer->current_timecode.frames = attributes->time_code_pictures;
    timer->current_timecode.frame_offset = temporal_reference;
    timer->current_timecode.reserved_1C = (signed char)attributes->field_54;
    timer->current_timecode.subframe = 0;

    if (group_changed != 0) {
        for (i = 0; i < 64; i++) {
            entries[i].repeat = -1;
        }
        entries[0].accumulated = -1;
    } else if (attributes->picture_type == 1 ||
               attributes->picture_type == 2) {
        int reference = work->reference_frames[1]
                            ->picture_info.decoded.attributes
                            .temporal_reference;
        int end = temporal_reference;
        if (end < reference) {
            end += 0x400;
        }
        for (i = reference + 1; i < end; i++) {
            entries[i % 64].repeat = -1;
        }
    }

    index = temporal_reference % 64;
    entries[index].repeat = timer->current_timecode.reserved_1C;
    if (group_changed != 0) {
        entries[index].accumulated = 0;
    } else if (temporal_reference == 0 && entries[0].accumulated == -1) {
        entries[0].accumulated = 0;
    } else {
        for (i = 0; i < 64; i++) {
            int previous_index = (index - i + 63) % 64;
            if (entries[previous_index].repeat != -1) {
                entries[index].accumulated =
                    entries[previous_index].repeat +
                    entries[previous_index].accumulated;
                break;
            }
        }
    }
    timer->current_timecode.subframe = entries[index].accumulated;

    if (attributes->picture_type == 3 && entries[index].repeat != 0) {
        SfdMpvFrame* reference_frame = work->reference_frames[1];
        SfdMpvTimeCodeSnapshot* frame_time =
            (SfdMpvTimeCodeSnapshot*)&reference_frame->time_unit;
        int reference = reference_frame->picture_info.decoded.attributes
                            .temporal_reference;
        SfdMpvRepeatEntry* reference_entry = &entries[reference % 64];

        reference_entry->accumulated =
            entries[index].repeat + entries[index].accumulated;
        frame_time->timecode.subframe = reference_entry->accumulated;
        SFTIM_Tc2Time(&frame_time->timecode, &frame_time->value,
                      &frame_time->scale);
        frame_time->value -= timer->initial.value;
        frame_time->valid = 1;
        if (timer->maximum.value <= frame_time->value) {
            timer->maximum = *frame_time;
        }
        reference_frame->display_time_scale = frame_time->scale;
        reference_frame->display_time_value =
            repeat_timer->reference_time_offset +
            (frame_time->value - repeat_timer->reference_time_origin);
        reference_frame->field_4C = frame_time->value;
        reference_frame->field_50 =
            frame_time->value + repeat_timer->reference_time_offset;
        if (repeat_timer->maximum_reference_time <
            reference_frame->display_time_value) {
            repeat_timer->maximum_reference_time =
                reference_frame->display_time_value;
            repeat_timer->maximum_reference_scale =
                reference_frame->display_time_scale;
        }
    }
}

static int sfmpv_DecodePicAtr(SfdHandle* handle, const SJCK* header,
                              SJ* stream, int delimiter_type,
                              int* decode_result)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvAuxWork* aux =
        (SfdMpvAuxWork*)((unsigned char*)work + sizeof(*work));
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    MPVPictureInfo* picture = &work->picture_info;
    MPVPictureAttributes* attributes = &picture->attributes;
    const unsigned char* delimiter;
    SfdPtsEntry pts_entry;
    long long timestamp;
    long long raw_pts;
    int flow_before;
    int consumed;
    int result;
    int mode;
    int bit_rate;
    int vbv_buffer_size;
    int vbv_delay;
    int byte_rate;

    aux->picture_buffers.entries[0].size = 0;
    MPV_SetPicUsrBuf(work->decoder,
                     aux->picture_buffers.entries[0].buffer,
                     aux->picture_buffers.buffer_size);
    flow_before = SJRBF_GetFlowCnt(stream, 0, 1);
    *decode_result = MPV_DecodePicAtrSj(work->decoder, stream);
    consumed = SJRBF_GetFlowCnt(stream, 0, 1) - flow_before;
    switch (*decode_result) {
    case 0:
        result = 0;
        break;
    case -2:
        result = consumed > 0 ? 0 : SFLIB_SetErr(handle, -2);
        break;
    case -3:
        result = consumed > 0 ? 0 : SFLIB_SetErr(handle, -3);
        break;
    default:
        result = SFLIB_SetErr(handle, 0xFF000F04);
        break;
    }
    SFBUF_AddRtotSj(handle, handle->transports[2].parameter_10, consumed);
    handle->playback_runtime.time_values[4] += consumed;
    if (result != 0) {
        return result;
    }
    if (*decode_result == -2) {
        return 0;
    }

    result = MPV_GetPicAtr(work->decoder, picture);
    *decode_result = result;
    if (result != 0) {
        return SFLIB_SetErr(handle, 0xFF000F05);
    }
    if ((delimiter_type & 0x40) != 0) {
        SfdMpvPlaybackSettings* published =
            (SfdMpvPlaybackSettings*)&handle->playback_settings;

        if (published->width > 0 &&
            (published->width != attributes->width ||
             published->height != attributes->height)) {
            *decode_result = -2;
            return 0;
        }
        {
            SfdMpvSizeCallback callback =
                (SfdMpvSizeCallback)SFSET_GetCond(handle, 95);
            SfdCallbackObject object = SFSET_GetCond(handle, 95);

            if (callback != 0 &&
                callback(object, attributes->width,
                         attributes->height) != 0) {
                *decode_result = -2;
                return 0;
            }
        }
    }

    if (attributes->picture_type == 1) {
        work->frame_state[2] = 0;
    } else if (handle->create_config.video_output_format == 3 &&
               work->reference_frames[1] != 0) {
        int reference = work->reference_frames[1]
                            ->picture_info.decoded.attributes
                            .temporal_reference;
        if (attributes->picture_type == 2) {
            if (attributes->temporal_reference < reference &&
                reference < 0x200) {
                work->frame_state[2] = 1;
            }
        } else if (attributes->picture_type == 3) {
            if (attributes->temporal_reference >= reference) {
                work->frame_state[2] = 1;
            }
        }
    }
    MPV_GetPicUsr(work->decoder, 0,
                  &aux->picture_buffers.entries[0].size);
    if (attributes->group_count != work->field_10C) {
        work->field_10C = attributes->group_count;
        work->field_110 = 1;
    } else {
        work->field_110 = 0;
    }

    if ((delimiter_type & 0x40) != 0) {
        SfdMpvHeaderCallback callback =
            (SfdMpvHeaderCallback)SFSET_GetCond(handle, 77);
        SfdCallbackObject object = SFSET_GetCond(handle, 78);

        if (callback != 0) {
            delimiter = MPV_SearchDelim(header->data, header->len, 1);
            if (delimiter != 0) {
                callback(object, header->data,
                         (delimiter + 4) - header->data);
            }
        }
    }

    delimiter = MPV_SearchDelim(header->data, header->len, 4);
    timestamp = -1;
    raw_pts = -1;
    if (delimiter != 0) {
        SFPTS_ReadPtsQue(handle, handle->transports[2].parameter_10,
                         (unsigned char*)delimiter, &pts_entry);
        if (pts_entry.pts >= 0) {
            int rate = SFTIM_prate[attributes->frame_rate_code];
            int delta;

            if (handle->timer_state.field_0150 < 0) {
                long long initial_pts =
                    pts_entry.pts -
                    90000000LL * attributes->temporal_reference / rate;
                if (initial_pts < 0) {
                    initial_pts = 0;
                }
                handle->timer_state.field_0150 = initial_pts;
            }
            timestamp = pts_entry.pts - handle->timer_state.field_0150;
            if (timestamp < 0) {
                timestamp = 0;
            }
            if (memcmp(&work->pts_entry, &pts_entry, 4) != 0) {
                work->pts_entry = pts_entry;
                work->field_11C = 0;
                work->field_118 = attributes->temporal_reference;
                work->field_120 = attributes->picture_type == 3;
                raw_pts = pts_entry.pts;
            } else {
                if (work->field_110 != 0) {
                    work->field_11C += work->field_120 + 1;
                    work->field_120 = 0;
                    work->field_118 = 0;
                }
                delta = attributes->temporal_reference - work->field_118;
                work->field_120 = work->field_120 > delta
                                      ? work->field_120
                                      : delta;
                timestamp +=
                    90000000LL * (work->field_11C + delta) / rate;
                if (timestamp < 0) {
                    timestamp = 0;
                }
            }
        }
    }
    aux->picture_pts = raw_pts;
    if ((delimiter_type & work->decode_mode) == 0) {
        return 0;
    }

    sfmpv_CalcRepeatField(handle, picture, work->field_110);
    mode = SFSET_GetCond(handle, 52);
    if (mode == 0) {
        int force = timestamp < 0 || attributes->group_count == 0 ||
                    attributes->field_57 != 0;
        if (!force && work->field_110 != 0 && timer->maximum.valid != 0) {
            SfdTimeCode current_timecode = timer->current_timecode;
            int value;
            int stored_value;
            int scale;

            SFTIM_Tc2Time(&current_timecode, &value, &scale);
            SFTIM_Tc2Time(&timer->maximum.timecode, &stored_value, &scale);
            force = value <= stored_value ||
                    value >= stored_value + scale * SFSET_GetCond(handle, 53);
        }
        if (force) {
            mode = 1;
            SFSET_SetCond(handle, 52, 1);
        }
    }
    if (mode == 1) {
        sfmpv_DoReformTc(handle, picture, timestamp, work->field_110);
    }

    if (timer->initial.valid == 0) {
        SfdTimeCode timecode = timer->current_timecode;
        int value;
        int scale;

        timecode.frame_offset = 0;
        SFTIM_Tc2Time(&timecode, &value, &scale);
        timer->initial.timecode = timecode;
        timer->initial.value = value;
        timer->initial.scale = scale;
        timer->initial.valid = 1;
    }
    {
        SfdTimeCode timecode = timer->current_timecode;
        int value;
        int scale;

        SFTIM_Tc2Time(&timecode, &value, &scale);
        timer->current.timecode = timecode;
        timer->current.value = value - timer->initial.value;
        timer->current.scale = scale;
        timer->current.valid = 1;
    }
    if (timer->maximum.value <= timer->current.value) {
        timer->maximum = timer->current;
    }

    if (((SfdMpvPlaybackSettings*)&handle->playback_settings)->bit_rate != 0) {
        return 0;
    }
    if (MPV_GetBitRate(work->decoder, &bit_rate) != 0) {
        return SFLIB_SetErr(handle, 0xFF000F16);
    }
    MPV_GetVbvBufSiz(work->decoder, &vbv_buffer_size, &vbv_delay,
                     &byte_rate);
    if (SFSET_GetCond(handle, 60) == 0) {
        work->active_size_threshold = 0;
    } else {
        int ring_size = SFBUF_GetRingBufSiz(handle, 1);
        if (byte_rate == -1) {
            byte_rate = vbv_buffer_size;
        }
        work->active_size_threshold =
            byte_rate < ring_size ? byte_rate : ring_size;
    }

    if (handle->seek_state.source_handle != 0 && work->field_088 <= 0) {
        SfdMpvSeekCache* cache =
            (SfdMpvSeekCache*)((unsigned char*)
                handle->seek_state.source_handle + 0xAD0);
        if (cache->valid == 0) {
            cache->header_size = header->len < 0x200 ? header->len : 0x200;
            MEM_Copy(cache->header, header->data, cache->header_size);
            if (bit_rate == 0x3FFFF) {
                cache->byte_rate = 0;
                cache->byte_rate_valid = 0;
            } else {
                cache->byte_rate = bit_rate * 50;
                cache->byte_rate_valid = 1;
            }
            cache->initial = timer->initial;
            cache->valid = 1;
        }
    }

    {
        SfdMpvPlaybackSettings* settings =
            (SfdMpvPlaybackSettings*)&handle->playback_settings;

        settings->width = attributes->width;
        settings->height = attributes->height;
        settings->macroblocks_per_row = attributes->macroblocks_per_row;
        settings->macroblock_rows = attributes->macroblock_rows;
        settings->frame_rate_code = attributes->frame_rate_code;
        settings->bit_rate = bit_rate;
        settings->vbv_buffer_size = vbv_buffer_size;
        return sfmpv_ChkBufSiz(handle, settings);
    }
}

static int sfmpv_Concat(SfdHandle* handle, SJ* stream)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    int concat_time;
    int result = 0;

    if (SFSET_GetCond(handle, 6) == 0) {
        if (timer->maximum.valid == 0) {
            concat_time = 0;
        } else {
            SfdTimeCode timecode = timer->maximum.timecode;
            int frame = timecode.frames + timecode.frame_offset + 1 +
                        (timecode.subframe + timecode.reserved_1C) / 2;
            int rate = sfmpv_fps_round[timecode.frame_rate_code];
            int added_seconds = frame / rate;
            int total_seconds = timecode.seconds + added_seconds;
            int total_minutes = timecode.minutes + total_seconds / 60;
            int value;
            int scale;

            timecode.hours += total_minutes / 60;
            timecode.minutes = total_minutes % 60;
            timecode.seconds = total_seconds % 60;
            timecode.frames = frame % rate;
            if (timecode.drop_frame != 0 && timecode.seconds == 0 &&
                timecode.minutes % 10 != 0 &&
                (timecode.frames == 0 || timecode.frames == 1)) {
                timecode.frames = 2;
            }
            timecode.frame_offset = 0;
            timecode.subframe =
                (timecode.subframe + timecode.reserved_1C) & 1;
            SFTIM_Tc2Time(&timecode, &value, &scale);
            concat_time = value - timer->initial.value;
        }
    } else {
        int samples;
        int sample_rate;
        int* total_samples =
            &handle->timer_state.sample_window.fields_04[0];

        if (handle->create_config.buffer.transport_setup->entries[3] !=
            &SFD_tr_ad_adxt) {
            samples = 0;
            sample_rate = 44100;
        } else if (SFCON_ReadTotSmplQue(handle, &samples, &sample_rate) == 0) {
            result = -1;
        }
        if (result == 0) {
            *total_samples += samples;
            concat_time = UTY_MulDiv(*total_samples, timer->initial.scale,
                                     sample_rate) -
                          repeat_timer->reference_time_offset;
            if (concat_time < 0) {
                concat_time = 0;
            }
        }
    }
    if (result == -1) {
        return -1;
    }

    if (concat_time > 0) {
        SFCON_UpdateConcatTime(handle, concat_time);
        work->field_088++;
    }
    SFTIM_InitTtu((SfdTimerTimeUnit*)&timer->initial, 0x7FFFFFFF);
    SFTIM_InitTtu((SfdTimerTimeUnit*)&timer->maximum, -1);
    work->decode_mode = 0xC0;
    for (;;) {
        SJCK chunk;
        stream->interface->get_chunk(stream, 1, 4, &chunk);
        if (chunk.len != 4 || MPV_CheckDelim(chunk.data) != 0x80) {
            stream->interface->unget_chunk(stream, 1, &chunk);
            break;
        }
        stream->interface->put_chunk(stream, 0, &chunk);
        SFBUF_AddRtotSj(handle, handle->transports[2].parameter_10, 4);
        handle->playback_runtime.time_values[4] += 4;
    }
    return 0;
}

static int sfmpv_DecodeOneUnit(SfdHandle* handle, int active_size,
                               int delimiter_type, int has_data,
                               int* processed)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    int buffer_index = handle->transports[2].parameter_10;
    SJ* stream;
    int result;

    *processed = 0;
    handle->playback_runtime.field_24 = 0;
    if (work->decode_mode != 0xCC || work->frame_state[1] == 0) {
        delimiter_type &= 0xCC;
    }
    result = SFBUF_RingGetSj(handle, buffer_index, &stream);
    if (result != 0) {
        return 0;
    }
    if ((delimiter_type & 0xC8) != 0) {
        SFMPVF_SetGopStat(handle, 1);
    }
    if (delimiter_type == 0x80) {
        if (handle->transports[2].parameter_18 < 0) {
            handle->transports[2].parameter_18 =
                SFBUF_GetRTot(handle, buffer_index) + 4;
        }
        if (timer->reserved_snapshot.value < 0) {
            timer->reserved_snapshot = timer->maximum;
        }
    }

    if (delimiter_type == 0x80 && SFCON_IsEndcodeSkip(handle) != 0) {
        if (sfmpv_Concat(handle, stream) == 0) {
            *processed = 1;
        }
        return result;
    }
    if (delimiter_type == 0x80 &&
        SFCON_IsVideoEndcodeSkip(handle) != 0) {
        for (;;) {
            SJCK chunk;
            stream->interface->get_chunk(stream, 1, 4, &chunk);
            if (chunk.len != 4 || MPV_CheckDelim(chunk.data) != 0x80) {
                stream->interface->unget_chunk(stream, 1, &chunk);
                break;
            }
            stream->interface->put_chunk(stream, 0, &chunk);
            SFBUF_AddRtotSj(handle, buffer_index, 4);
            handle->playback_runtime.time_values[4] += 4;
        }
        *processed = 1;
        return result;
    }
    if (has_data == 0 &&
        (delimiter_type == 0x80 ||
         (active_size <= 4 && SFBUF_GetTermFlg(handle, buffer_index) == 1))) {
        SFMPVF_TermDec(handle);
        return result;
    }
    if (has_data == 0 && active_size <= 4) {
        handle->playback_runtime.field_24 = 1;
        return result;
    }

    if ((delimiter_type & 0x4C) != 0) {
        SfdBufferTransfer transfer;
        SJCK header;
        int decode_result;

        if (SFBUF_RingGetRead(handle, buffer_index, &transfer) != 0) {
            header.data = 0;
            header.len = 0;
        } else {
            header = transfer.chunks[0];
        }
        result = sfmpv_DecodePicAtr(handle, &header, stream,
                                    delimiter_type, &decode_result);
        if (result != 0) {
            return result;
        }
        if (decode_result == 0) {
            if ((delimiter_type & work->decode_mode) != 0) {
                work->decode_mode = 0xCC;
            }
            work->frame_state[1] = 1;
        }
        if (delimiter_type == 0x40 && decode_result == -2) {
            work->decode_mode = 0xC0;
        }
        *processed = 1;
        return result;
    }
    if ((delimiter_type & 2) != 0) {
        SfdBufferTransfer transfer;
        SJCK chunk;

        if (SFBUF_RingGetRead(handle, buffer_index, &transfer) != 0) {
            chunk.data = 0;
            chunk.len = 0;
        } else {
            chunk = transfer.chunks[0];
        }
        if (sfmpv_IsSkip(handle, &chunk) != 0) {
            int flow_before;
            int consumed;
            int skip_result;

            if (timer->current.value < repeat_timer->reference_time_origin) {
                timer->previous = timer->current;
            }
            flow_before = SJRBF_GetFlowCnt(stream, 0, 1);
            skip_result = MPV_SkipFrmSj(work->decoder, stream);
            consumed = SJRBF_GetFlowCnt(stream, 0, 1) - flow_before;
            switch (skip_result) {
            case 0:
                result = 0;
                break;
            case -2:
            case -3:
                result = consumed > 0
                             ? 0
                             : SFLIB_SetErr(handle, skip_result);
                break;
            default:
                result = SFLIB_SetErr(handle, 0xFF000F07);
                break;
            }
            SFBUF_AddRtotSj(handle, buffer_index, consumed);
            handle->playback_runtime.time_values[4] += consumed;
            if (result == 0) {
                if ((signed char)work->picture_info.attributes.field_58 == 0) {
                    work->frame_state[0] = 1;
                }
                SFPLY_AddSkipPic(
                    handle, 1, work->picture_info.attributes.picture_type);
                *processed = 1;
            }
            return result;
        }
        return sfmpv_DecodeFrm(handle, stream);
    }
    if (delimiter_type != 0x80 &&
        sfmpv_GoDdelim(handle, 0xCC) > 0) {
        *processed = 1;
    }
    return result;
}

static int sfmpv_NeedSafeDlmRefresh(const SfdBufferTransfer* transfer,
                                    int current_type,
                                    const unsigned char* delimiter)
{
    unsigned char delimiter_bytes[4];
    const unsigned char* first = transfer->chunks[0].data;
    const unsigned char* second = transfer->chunks[1].data;
    int type;

    if (delimiter == 0 || delimiter == first) {
        return 1;
    }
    if (delimiter >= first && delimiter < first + transfer->chunks[0].len) {
        int overflow = delimiter + 4 -
                       (first + transfer->chunks[0].len);
        if (overflow > 0) {
            if (overflow > transfer->chunks[1].len) {
                return 1;
            }
            memcpy(delimiter_bytes, delimiter, 4 - overflow);
            memcpy(delimiter_bytes + 4 - overflow, second, overflow);
        } else {
            memcpy(delimiter_bytes, delimiter, 4);
        }
    } else if (delimiter >= second &&
               delimiter < second + transfer->chunks[1].len) {
        if (delimiter + 4 > second + transfer->chunks[1].len) {
            return 1;
        }
        memcpy(delimiter_bytes, delimiter, 4);
    } else {
        return 1;
    }

    type = MPV_CheckDelim(delimiter_bytes);
    switch (type) {
    case 8:
        if ((current_type & 0x40) != 0) {
            const unsigned char* next =
                sfmpv_SearchTransferDelimiter(transfer, 8);
            if (next == 0 || next == delimiter) {
                return 1;
            }
        }
        return 0;
    case 4:
        if ((current_type & 0x48) != 0) {
            const unsigned char* next =
                sfmpv_SearchTransferDelimiter(transfer, 4);
            if (next == 0 || next == delimiter) {
                return 1;
            }
        }
        return 0;
    case 0x40:
    case 0x80:
        return 0;
    default:
        return 1;
    }
}

static int sfmpv_GetActiveSize(SfdHandle* handle, int* active_size,
                               int* delimiter_type, int* has_data)
{
    int buffer_index = handle->transports[2].parameter_10;
    SfdBufferTransfer transfer;
    const unsigned char* delimiter;
    unsigned char* cached_delimiter;
    unsigned char* cached_end;
    unsigned char* transfer_end;
    int type = 0;
    int result;

    *active_size = 0;
    *delimiter_type = 0;
    *has_data = 0;
    result = SFBUF_RingGetRead(handle, buffer_index, &transfer);
    if (result != 0) {
        return result;
    }
    if (transfer.chunks[0].len == 0) {
        return 0;
    }

    delimiter = sfmpv_SearchTransferDelimiter(&transfer, 0xCE);
    if (delimiter != 0) {
        type = MPV_CheckDelim(delimiter);
    }
    if (delimiter != transfer.chunks[0].data) {
        if (delimiter != 0) {
            if (delimiter >= transfer.chunks[0].data &&
                delimiter < transfer.chunks[0].data +
                                transfer.chunks[0].len) {
                *active_size = delimiter - transfer.chunks[0].data;
            } else if (delimiter >= transfer.chunks[1].data &&
                       delimiter < transfer.chunks[1].data +
                                       transfer.chunks[1].len) {
                *active_size = transfer.chunks[0].len +
                               (delimiter - transfer.chunks[1].data);
            }
        } else {
            int size = transfer.chunks[0].len + transfer.chunks[1].len - 3;
            *active_size = size > 0 ? size : 0;
        }
        if (*active_size > 0) {
            *has_data = 1;
        }
        return 0;
    }

    *delimiter_type = type;
    *active_size = 4;
    if ((type & 0x80) != 0) {
        return 0;
    }
    SFBUF_RingGetDlm(handle, buffer_index, &cached_delimiter, &cached_end);
    if (sfmpv_NeedSafeDlmRefresh(&transfer, type, cached_delimiter) != 0) {
        transfer_end = transfer.chunks[1].len == 0
                           ? transfer.chunks[0].data +
                                 transfer.chunks[0].len
                           : transfer.chunks[1].data +
                                 transfer.chunks[1].len;
        if (cached_end != transfer_end) {
            cached_end = transfer_end;
            cached_delimiter = (unsigned char*)
                sfmpv_BsearchTransferDelimiter(&transfer, 0xCC);
            SFBUF_RingSetDlm(handle, buffer_index, cached_delimiter,
                             cached_end);
        }
    }
    if (cached_delimiter == 0) {
        int free_size = SFBUF_GetRingBufSiz(handle, buffer_index) -
                        SFBUF_RingGetDataSiz(handle, buffer_index);
        if (free_size < handle->create_config.buffer.ring_alignment) {
            return SFLIB_SetErr(handle, 0xFF000F1C);
        }
        return 0;
    }

    switch (MPV_CheckDelim(cached_delimiter)) {
    case 8:
        if ((type & 0x40) != 0) {
            const unsigned char* next =
                sfmpv_SearchTransferDelimiter(&transfer, 8);
            if (next == 0 || next == cached_delimiter) {
                int free_size = SFBUF_GetRingBufSiz(handle, buffer_index) -
                    SFBUF_RingGetDataSiz(handle, buffer_index);
                if (free_size < handle->create_config.buffer.ring_alignment) {
                    return SFLIB_SetErr(handle, 0xFF000F1C);
                }
                return 0;
            }
        }
        break;
    case 4:
        if ((type & 0x48) != 0) {
            const unsigned char* next =
                sfmpv_SearchTransferDelimiter(&transfer, 4);
            if (next == 0 || next == cached_delimiter) {
                int free_size = SFBUF_GetRingBufSiz(handle, buffer_index) -
                    SFBUF_RingGetDataSiz(handle, buffer_index);
                if (free_size < handle->create_config.buffer.ring_alignment) {
                    return SFLIB_SetErr(handle, 0xFF000F1C);
                }
                return 0;
            }
        }
        break;
    default:
        break;
    }
    if (cached_delimiter >= transfer.chunks[0].data &&
        cached_delimiter < transfer.chunks[0].data +
                               transfer.chunks[0].len) {
        *active_size = cached_delimiter - transfer.chunks[0].data;
    } else if (cached_delimiter >= transfer.chunks[1].data &&
               cached_delimiter < transfer.chunks[1].data +
                                      transfer.chunks[1].len) {
        *active_size = transfer.chunks[0].len +
                       (cached_delimiter - transfer.chunks[1].data);
    }
    return 0;
}

static int sfmpv_ExecServerSub(SfdHandle* handle)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    int input_buffer = handle->transports[2].parameter_10;
    int output_buffer = handle->transports[2].parameter_14;
    int result;
    int processed;

    if (SFSET_GetCond(handle, 5) == 0 ||
        SFBUF_GetTermFlg(handle, output_buffer) == 1) {
        return 0;
    }
    if (SFSET_GetCond(handle, 28) != 0 &&
        SFHDS_GetColType(handle) != -1) {
        MPVContext* decoder = 0;
        if (handle != 0) {
            if (SFLIB_CheckHn(handle) != 0) {
                SFLIB_SetErr(0, 0xFF000181);
            } else {
                decoder = work->decoder;
            }
        }
        if (decoder != 0 && MPV_SetCond(decoder, 5, 0) != 0) {
            SFLIB_SetErr(handle, 0xFF000F12);
        }
    }
    if (handle->requested_state == 2) {
        SJCK header;
        int consumed;

        header.data = (unsigned char*)SFSET_GetCond(handle, 93);
        header.len = SFSET_GetCond(handle, 94);
        if (header.data != 0 && header.len != 0 &&
            work->decode_mode == 0xC0 &&
            MPV_DecodePicAtr(work->decoder, &header, &consumed) == 0) {
            work->decode_state = 2;
            work->decode_mode = 0xC8;
        }
    }

    do {
        int active_size;
        int delimiter_type;
        int has_data;

        result = sfmpv_GetActiveSize(handle, &active_size, &delimiter_type,
                                     &has_data);
        if (result == 0) {
            result = sfmpv_DecodeOneUnit(handle, active_size,
                                         delimiter_type, has_data,
                                         &processed);
        }
    } while (result == 0 && processed != 0);

    {
        SJ* stream;
        int write_count;
        int read_count;
        int flow_high = handle->playback_runtime.timing.input_flow.high;
        unsigned int flow_low =
            handle->playback_runtime.timing.input_flow.low;

        SFBUF_RingGetSj(handle, input_buffer, &stream);
        SFBUF_GetFlowCnt(stream, &write_count, &read_count);
        handle->playback_runtime.time_values[3] =
            SFBUF_UpdateFlowCnt(flow_high, flow_low, write_count);
    }

    if (SFBUF_GetPrepFlg(handle, output_buffer) != 1 &&
        SFBUF_GetPrepFlg(handle, input_buffer) == 1) {
        int ready;
        if (SFMPVF_IsTermDec(handle) != 0) {
            ready = 1;
        } else {
            int frame_limit = handle->conditions_primary[21];
            int configured_limit =
                handle->create_config.picture_user_buffer_minimum;
            if (frame_limit == -1) {
                frame_limit = configured_limit;
            }
            if (configured_limit < frame_limit) {
                frame_limit = configured_limit;
            }
            if (SFMPVF_GetNumFrm(handle) >= frame_limit) {
                int stream_ready;
                if (SFBUF_GetTermFlg(handle, input_buffer) == 1 ||
                    (handle->header_state.field_00 != 0 &&
                     handle->header_state.field_7C == 0)) {
                    stream_ready = 1;
                } else {
                    int bit_rate;
                    MPV_GetBitRate(work->decoder, &bit_rate);
                    if (bit_rate == 0x3FFFF ||
                        SFBUF_GetWTot(handle, 1) >=
                            work->active_size_threshold) {
                        stream_ready = 1;
                    } else {
                        int buffer = SFTRN_IsSetup(handle, 1) == 0;
                        stream_ready =
                            SFBUF_GetWTot(handle, buffer) >=
                            SFBUF_GetRingBufSiz(handle, buffer);
                    }
                }
                ready = stream_ready;
            } else {
                ready = 0;
            }
        }
        if (ready) {
            SFBUF_SetPrepFlg(handle, output_buffer, 1);
            if (repeat_timer->reference_time_origin != 0x7FFEFFFF) {
                ((SfdMpvDecodeTimer*)&handle->timer_state)
                    ->output_start.valid = 1;
            }
        }
    }
    {
        int frame_count = SFMPVF_GetNumFrm(handle);
        if (frame_count == -1 ||
            (SFMPVF_IsTermDec(handle) != 0 && frame_count == 1 &&
             handle->playback_runtime.frame_outstanding != 0)) {
            SFBUF_SetTermFlg(handle, output_buffer, 1);
            if (handle->playback_runtime.decoded_pictures == 0) {
                SFSET_SetCond(handle, 5, 0);
            }
        }
    }
    return result;
}

static int SFMPV_ExecServer(SfdHandle* handle)
{
    return sfmpv_ExecServerSub(handle);
}

static int SFMPV_Finish(SfdHandle* handle)
{
    MPV_Finish();
    return 0;
}

#pragma optimization_level 1
static int sfmpv_ChkFatal(void)
{
    int picture_info_size = sizeof(MPVPictureInfo);
    int decoder_info_size = sizeof(SfdMpvDecoderInfo);
    int integer_size = sizeof(int);

    if (picture_info_size != 0x80) {
        return SFLIB_SetErr(0, 0xFF000F19);
    }
    if (decoder_info_size > 0x204) {
        return SFLIB_SetErr(0, 0xFF000F1A);
    }
    if (integer_size != 4) {
        return SFLIB_SetErr(0, 0xFF000F1E);
    }
    return 0;
}
#pragma optimization_level 4

static int SFMPV_Init(SfdHandle* handle)
{
    int result;

    result = sfmpv_ChkFatal();
    if (result != 0) {
        for (;;) {
        }
    }
    result = MPV_Init(8, sfmpv_work);
    if (result != 0) {
        int error = 0xFF000F01;
        if (result == 0xFFFDFF05) {
            error = 0xFF000F13;
        }
        return SFLIB_SetErr(0, error);
    }
    memset(sfmpv_para, 0, sizeof(sfmpv_para));
    memset(sfmpv_rfb_adr_tbl, 0, sizeof(sfmpv_rfb_adr_tbl));
    memset(sfmpv_ta_adr_tbl, 0, sizeof(sfmpv_ta_adr_tbl));
    sfmpv_discard_wsiz = 0;
    return 0;
}

int SFD_SetPicUsrBuf(SfdHandle* handle, void* buffer, int buffer_count,
                     int buffer_size)
{
    SfdMpvFrameWork* work;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000185);
    }
    work = (SfdMpvFrameWork*)handle->transports[2].context;
    return sfmpv_SetPicUsrBufSub(handle, work, buffer, buffer_count,
                                 buffer_size);
}

int SFD_IsNextFrmReady(SfdHandle* handle)
{
    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF000183);
        return 0;
    }
    return SFMPVF_IsNextFrmReady(handle);
}

void SFMPV_RestoreCond(SfdHandle* handle, const void* conditions, int count)
{
    SfdMpvFrameWork* work = (SfdMpvFrameWork*)handle->transports[2].context;
    const int* values = conditions;
    MPVContext* decoder = *(MPVContext**)work;
    int i;

    if (decoder != 0) {
        for (i = 0; i < count; i++) {
            MPV_SetCond(decoder, i, values[i]);
        }
    }
}

int SFMPV_SaveCond(SfdHandle* handle, void* conditions, int buffer_size)
{
    SfdMpvFrameWork* work = (SfdMpvFrameWork*)handle->transports[2].context;
    int* values = conditions;
    MPVContext* decoder = *(MPVContext**)work;
    int count;
    int i;

    if (decoder == 0) {
        return 0;
    }
    count = buffer_size / sizeof(int);
    if (count > 16) {
        count = 16;
    }
    for (i = 0; i < count; i++) {
        MPV_GetCond(decoder, i, &values[i]);
    }
    return count;
}

int SFD_SetMpvCond(SfdHandle* handle, int condition, int value)
{
    MPVContext* decoder;

    if (handle == 0) {
        decoder = 0;
    } else {
        if (SFLIB_CheckHn(handle) != 0) {
            return SFLIB_SetErr(0, 0xFF000181);
        }
        decoder = ((SfdMpvFrameWork*)handle->transports[2].context)->decoder;
    }
    if (condition == 5) {
        value = 0;
    }
    if (MPV_SetCond(decoder, condition, value) != 0) {
        return SFLIB_SetErr(handle, 0xFF000F12);
    }
    return 0;
}

void SFD_CalcYccPlane(void* buffer, int width, int height,
                      SfdYccPlane* output)
{
    sfmpv_CalcYccPlaneSub(buffer, width, height, output);
}

void SFD_SetMpvParaTbl(const int* parameters,
                       void* const* reference_buffers,
                       void* const* frame_buffers, int handle_work_size,
                       int video_input_buffer_size,
                       int system_input_buffer_size)
{
    int i;

    (void)handle_work_size;
    (void)video_input_buffer_size;
    (void)system_input_buffer_size;

    sfmpv_para[0] = parameters[0];
    sfmpv_para[1] = parameters[1];
    sfmpv_para[2] = parameters[2];
    sfmpv_para[3] = parameters[3];
    sfmpv_para[4] = parameters[4];
    sfmpv_para[5] = parameters[5];
    sfmpv_para[6] = parameters[6];
    sfmpv_para[7] = parameters[7];
    sfmpv_para[8] = parameters[8];
    sfmpv_para[4] = 0;
    sfmpv_para[8] = 0;
    sfmpv_rfb_adr_tbl[0] =
        ((unsigned long)reference_buffers[0] + 31) & ~31UL;
    sfmpv_rfb_adr_tbl[1] =
        ((unsigned long)reference_buffers[1] + 31) & ~31UL;
    for (i = 0; i < 16; i++) {
        if (i < parameters[7]) {
            sfmpv_ta_adr_tbl[i] =
                ((unsigned long)frame_buffers[i] + 31) & ~31UL;
        } else {
            sfmpv_ta_adr_tbl[i] = 0;
        }
    }
}
