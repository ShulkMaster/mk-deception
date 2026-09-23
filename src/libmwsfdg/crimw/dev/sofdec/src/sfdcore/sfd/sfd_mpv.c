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

struct SfdMpvPictureUserData {
    void* buffer;
    int size;
};

typedef struct SfdMpvPictureUserBuffers {
    void* base;
    int buffer_count;
    int buffer_size;
    SfdMpvPictureUserData entries[17];
} SfdMpvPictureUserBuffers;

typedef struct SfdMpvAuxWork {
    SfdMpvPictureUserBuffers picture_buffers;
    int reserved_1014; /* 8-byte alignment for picture_pts at +0x1018. */
    long long picture_pts;
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

/* The cached sequence-header bytes and their length are one seek-work record. */
typedef struct SfdMpvRawHeader {
    unsigned char data[0x200];
    int size;
} SfdMpvRawHeader;

typedef struct SfdMpvSeekCache {
    int valid;
    int byte_rate;
    int byte_rate_valid;
    SfdMpvTimeCodeSnapshot initial;
    SfdMpvRawHeader raw_header;
} SfdMpvSeekCache;

typedef char SfdMpvSeekCacheSizeCheck[
    sizeof(SfdMpvSeekCache) == 0x23C ? 1 : -1];

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

int sfmpv_discard_wsiz = 0;
static int sfmpv_picusr_buf1siz = 0;
static int sfmpv_picusr_bufnum = 0;
static void* sfmpv_picusr_pbuf = 0;
static SfdMpvParameters sfmpv_para;
static void* sfmpv_rfb_adr_tbl[2];
static void* sfmpv_ta_adr_tbl[16];
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
    unsigned char* next_buffer = buffer;

    if (next_buffer == 0 || buffer_count == 0 || buffer_size == 0) {
        user->base = 0;
        user->buffer_count = 0;
        user->buffer_size = 0;
        user->entries[0].buffer = 0;
        user->entries[0].size = 0;
        for (i = 0; i < 16; i++) {
            user->entries[i + 1].buffer = 0;
            user->entries[i + 1].size = 0;
        }
        return 0;
    }
    if (buffer_count < handle->create_config.picture_user_buffer_minimum + 3) {
        return SFLIB_SetErr(handle, 0xFF000F1D);
    }
    user->base = next_buffer;
    user->buffer_count = buffer_count;
    user->buffer_size = buffer_size;
    user->entries[0].buffer = next_buffer;
    user->entries[0].size = 0;
    next_buffer += buffer_size;
    count = buffer_count - 1;
    for (i = 0; i < (count < 16 ? count : 16); i++) {
        user->entries[i + 1].buffer = next_buffer;
        user->entries[i + 1].size = 0;
        next_buffer += buffer_size;
    }
    return 0;
}

static inline const unsigned char* sfmpv_SearchTransferDelimiter(
    const SfdBufferTransfer* transfer, int delimiter_mask, int* code)
{
    unsigned char boundary[8];
    const unsigned char* delimiter;
    int tail;
    int head;
    int i;
    int found_code;

    delimiter = MPV_SearchDelim(transfer->chunks[0].data,
                                transfer->chunks[0].len, delimiter_mask);

    /* The search helper decodes each found code even if the caller ignores it. */
    if (delimiter != 0) {
        *code = MPV_CheckDelim(delimiter);
        return delimiter;
    }
    if (transfer->chunks[1].len == 0) {
        return 0;
    }
    tail = transfer->chunks[0].len;
    if (tail >= 3) {
        tail = 3;
    }
    head = transfer->chunks[1].len;
    if (head >= 3) {
        head = 3;
    }
    memcpy(boundary,
           transfer->chunks[0].data + transfer->chunks[0].len - tail,
           tail);
    memcpy(boundary + tail, transfer->chunks[1].data, head);
    for (i = 0; i < tail + head - 3; i++) {
        found_code = MPV_CheckDelim(boundary + i);
        if ((found_code & delimiter_mask) != 0) {
            *code = found_code;
            return transfer->chunks[0].data +
                   transfer->chunks[0].len - tail + i;
        }
    }
    delimiter = MPV_SearchDelim(transfer->chunks[1].data,
                                transfer->chunks[1].len, delimiter_mask);
    if (delimiter != 0) {
        *code = MPV_CheckDelim(delimiter);
        return delimiter;
    }
    return 0;
}

static inline int sfmpv_DelimiterOffset(const SfdBufferTransfer* transfer,
                                        const unsigned char* delimiter)
{
    if (delimiter >= transfer->chunks[0].data &&
        delimiter < transfer->chunks[0].data + transfer->chunks[0].len) {
        return delimiter - transfer->chunks[0].data;
    }
    if (delimiter >= transfer->chunks[1].data &&
        delimiter < transfer->chunks[1].data + transfer->chunks[1].len) {
        return transfer->chunks[0].len +
               (delimiter - transfer->chunks[1].data);
    }
    return 0;
}

static inline const unsigned char* sfmpv_BsearchTransferDelimiter(
    const SfdBufferTransfer* transfer, int delimiter_mask)
{
    const unsigned char* delimiter;

    if (transfer->chunks[1].len != 0) {
        unsigned char boundary[8];
        int tail;
        int head;
        int i;

        delimiter = MPV_BsearchDelim(
            transfer->chunks[1].data + transfer->chunks[1].len,
            transfer->chunks[1].len, delimiter_mask);
        if (delimiter != 0) {
            return delimiter;
        }
        tail = transfer->chunks[0].len < 3 ? transfer->chunks[0].len : 3;
        head = transfer->chunks[1].len < 3 ? transfer->chunks[1].len : 3;

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
    }
    return MPV_BsearchDelim(
        transfer->chunks[0].data + transfer->chunks[0].len,
        transfer->chunks[0].len, delimiter_mask);
}

static inline void sfmpv_InitFrame(SfdMpvFrame* frame, void** frame_buffer)
{
    frame->state = 0;
    frame->reference_count = 0;
    SFTIM_InitTtu(&frame->time_unit, 0);
    frame->frame_buffer = *frame_buffer;
    frame->display_time_value = 0;
    frame->display_time_scale = 1;
    frame->field_40 = 0;
    frame->field_44 = 0;
    frame->picture_order = 0;
    frame->field_4C = 0;
    frame->field_50 = 0;
    UTY_MemsetDword((unsigned int*)&frame->picture_info,
                    (unsigned int)-1, 0x20);
}

static inline void sfmpv_InitFrameTable(SfdMpvFrameWork* work)
{
    SfdMpvFrame* frame = work->frames;
    int i;

    for (i = 0; i < 16; i++, frame++) {
        sfmpv_InitFrame(frame, &work->address_table[i]);
    }
}

static inline void sfmpv_InitSavedFrames(SfdMpvFrameWork* work,
                                         SfdMpvFrame* frame)
{
    int i;

    for (i = 0; i < 2; i++, frame++) {
        sfmpv_InitFrame(frame, &work->saved_pair[i]);
    }
}

static inline void sfmpv_InitDecodedFrames(SfdMpvFrameWork* work,
                                           SfdMpvFrame* frame, int count)
{
    int i;

    for (i = 0; i < count; i++, frame++) {
        sfmpv_InitFrame(frame, &work->address_table[i]);
    }
}

static inline int sfmpv_CalcFrameSize(int width, int height)
{
    int aligned_width = ((width + 15) / 16) * 16;
    int aligned_height = ((height + 15) / 16) * 16;
    int luma_size = aligned_height * (((aligned_width + 31) / 32) * 32);
    int chroma_size = (aligned_height / 2) *
                      ((((aligned_width / 2) + 31) / 32) * 32);

    return luma_size + chroma_size * 2 + 0x20;
}

static inline void sfmpv_SetPlane(void** buffer, int luma_stride,
                                  int chroma_stride, int luma_size,
                                  int chroma_size, MPVPlaneSet* plane)
{
    plane->luma_stride = luma_stride;
    plane->chroma_stride = chroma_stride;
    plane->planes[2] = *buffer;
    plane->planes[0] = (unsigned char*)plane->planes[2] + luma_size;
    plane->planes[1] = (unsigned char*)plane->planes[0] + chroma_size;
}

static inline void sfmpv_CalcYccPlaneSub(void* buffer, int width, int height,
                                         SfdYccPlane* output)
{
    int aligned_width = ((width + 15) / 16) * 16;
    int aligned_height;
    int chroma_stride;
    int luma_stride;

    luma_stride = ((aligned_width + 31) / 32) * 32;
    chroma_stride = ((aligned_width / 2 + 31) / 32) * 32;

    output->luma_stride = luma_stride;
    output->chroma_stride = chroma_stride;
    output->planes[2] = buffer;
    aligned_height = ((height + 15) / 16) * 16;
    output->planes[0] = output->planes[2] + aligned_height * luma_stride;
    output->planes[1] = output->planes[0] +
                        (aligned_height / 2) * chroma_stride;
}

static inline int sfmpv_SeekVhdr(SfdHandle* handle, int* restored)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvSeekCache* cache;
    SfdMpvRawHeader* raw_header;
    MPVContext* decoder;
    SJCK header;
    int consumed;

    if (handle->seek_state.work == 0) {
        cache = 0;
    } else if (work->field_088 > 0) {
        cache = 0;
    } else {
        cache = (SfdMpvSeekCache*)((unsigned char*)
                    handle->seek_state.work + 0xAD0);
    }
    if (cache == 0) {
        return 0;
    }
    if (cache->valid == 0) {
        return 0;
    }

    decoder = work->decoder;
    timer->initial = cache->initial;
    raw_header = &cache->raw_header;
    header.data = raw_header->data;
    header.len = raw_header->size;
    if (MPV_DecodePicAtr(decoder, &header, &consumed) != 0) {
        return SFLIB_SetErr(handle, 0xFF000F1B);
    }
    *restored = 1;
    return 0;
}

static int SFMPV_Seek(SfdHandle* handle, int parameter, int value)
{
    int restored = 0;
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    int result;

    (void)parameter;
    (void)value;
    result = sfmpv_SeekVhdr(handle, &restored);
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
    MPVPictureAttributes* picture = &frame->picture_info;

    *output = &state->info;
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
    if (picture->field_40 == 0) {
        (*output)->display_mode = 2;
    } else {
        (*output)->display_mode = 1;
    }
    (*output)->picture_pts = frame->picture_pts;
    (*output)->field_58 = picture->field_38;
    (*output)->field_5C = picture->field_3C;
    (*output)->field_60 = picture->bit_rate;
    (*output)->field_64 = picture->vbv_buffer_size;
    (*output)->field_68 = picture->field_50;
    (*output)->field_6A = picture->field_52;
    (*output)->fields_6C[0] = picture->field_55;
    (*output)->fields_6C[1] = picture->field_56;
    (*output)->fields_6C[2] = picture->field_57;
    (*output)->fields_6C[3] = picture->aspect_ratio;
    (*output)->fields_6C[4] = picture->constrained_parameters;
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

static inline int sfmpv_StopSub(SfdHandle* handle)
{
    int result;

    result = 0;
    if (handle->transports[2].context == 0) {
        result = 0;
    }
    return result;
}

static int SFMPV_Stop(SfdHandle* handle)
{
    return sfmpv_StopSub(handle);
}

static int SFMPV_Start(SfdHandle* handle)
{
    return 0;
}

static int SFMPV_Standby(SfdHandle* handle)
{
    return 0;
}

/* TODO: [breakthrough needed] 88.367645%; retail places frame/reference tables
 * before parameters in BSS; declaration order is neutral, so find the real owner. */
static int SFMPV_Destroy(SfdHandle* handle)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvPictureUserBuffers* user;
    MPVContext* decoder = work->decoder;

    if (decoder == 0) {
        return 0;
    }
    sfmpv_para = work->setup_values;
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
    switch (error) {
    case -3:
    case -2:
    case 0:
        return;
    default:
        SFLIB_SetErr((SfdHandle*)object, error);
        break;
    }
}

static inline int sfmpv_ChkPara(SfdMpvParameters* parameters)
{
    int i;

    if (parameters->frame_count <= 0 || parameters->frame_count > 16) {
        return -1;
    }
    if ((unsigned int)parameters->reference_buffer == 0 ||
        (unsigned int)parameters->frame_buffer == 0) {
        for (i = 0; i < 2; i++) {
            if (sfmpv_rfb_adr_tbl[i] == 0) {
                return -1;
            }
        }
        for (i = 0; i < parameters->frame_count; i++) {
            if (sfmpv_ta_adr_tbl[i] == 0) {
                return -1;
            }
        }
    }
    return 0;
}

/* TODO: [breakthrough needed] 95.181420%; both typed frame-count bounds are
 * codegen-neutral; validator reload and BSS owner need source evidence. */
static int sfmpv_InitInf(SfdHandle* handle, SfdMpvFrameWork* work)
{
    SfdMpvPictureUserBuffers* user;
    int i;
    int j;

    if (sfmpv_ChkPara(&sfmpv_para) != 0) {
        return SFLIB_SetErr(0, 0xFF000F15);
    }

    work->setup_values = sfmpv_para;
    memcpy(work->saved_pair, sfmpv_rfb_adr_tbl,
           sizeof(work->saved_pair));
    memcpy(work->address_table, sfmpv_ta_adr_tbl,
           sizeof(work->address_table));
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
    sfmpv_InitFrameTable(work);
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
    user->entries[0].buffer = 0;
    user->entries[0].size = 0;
    for (i = 0; i < sizeof(work->frames) / sizeof(work->frames[0]); i++) {
        user->entries[i + 1].buffer = 0;
        user->entries[i + 1].size = 0;
    }
    {
        SfdMpvPictureUserData* entries = &user->entries[1];

        for (j = 0; j < sizeof(work->frames) / sizeof(work->frames[0]); j++) {
            work->frames[j].picture_user_buffer = &entries[j];
        }
    }
    return 0;
}

/* TODO: [near miss] 91.205130%; typed setup and inline validation agree;
 * owner/prologue coloring remains without a justified clean-C lever. */
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
    if (MPV_SetErrFunc(decoder, sfmpv_ErrFn, handle) != 0) {
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

/* TODO: [breakthrough] 91.652435%; split count arithmetic improves match;
 * unsigned address checks are neutral, cross-chunk offsets remain. */
static int sfmpv_GoDdelim(SfdHandle* handle, SJ* stream,
                          int delimiter_mask)
{
    SfdBufferTransfer transfer;
    const unsigned char* delimiter;
    int consumed;
    int i;
    int nonzero = 0;
    int ignored_code;

    if (SFBUF_RingGetRead(handle, handle->transports[2].parameter_10,
                          &transfer) != 0) {
        return 0;
    }
    if (transfer.chunks[0].len == 0) {
        return 0;
    }
    delimiter = sfmpv_SearchTransferDelimiter(&transfer, delimiter_mask,
                                               &ignored_code);
    /* The two chunks may be separate objects, so compare their addresses. */
    if (delimiter == 0) {
        int remaining = transfer.chunks[0].len + transfer.chunks[1].len;
        remaining -= 3;
        consumed = remaining > 0 ? remaining : 0;
    } else if ((unsigned long)delimiter >=
                   (unsigned long)transfer.chunks[0].data &&
               (unsigned long)delimiter <
                   (unsigned long)(transfer.chunks[0].data +
                                   transfer.chunks[0].len)) {
        consumed = delimiter - transfer.chunks[0].data;
    } else if ((unsigned long)delimiter >=
                   (unsigned long)transfer.chunks[1].data &&
               (unsigned long)delimiter <
                   (unsigned long)(transfer.chunks[1].data +
                                   transfer.chunks[1].len)) {
        consumed = transfer.chunks[0].len +
                   (delimiter - transfer.chunks[1].data);
    } else {
        consumed = 0;
    }
    SFBUF_RingAddRead(handle, handle->transports[2].parameter_10, consumed);
    for (i = 0; i < (consumed < 3 ? consumed : 3); i++) {
        const unsigned char* byte;
        if (i < transfer.chunks[0].len) {
            byte = transfer.chunks[0].data + i;
        } else {
            byte = transfer.chunks[1].data + i - transfer.chunks[0].len;
        }
        if ((signed char)*byte != 0) {
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
                            MPVPictureInfo* picture,
                            MPVFrameBuffers* buffers,
                            SfdMpvFrame** output_frame)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvAuxWork* aux =
        (SfdMpvAuxWork*)((unsigned char*)work + sizeof(*work));
    if (work->pending_frame != 0) {
        *output_frame = work->pending_frame;
    } else {
        *output_frame = SFMPVF_AllocFrm(handle);
        if (*output_frame == 0) {
            handle->playback_runtime.field_28 = 1;
            return -1;
        }
    }
    (*output_frame)->picture_info = *picture;
    (*output_frame)->picture_pts = aux->picture_pts;

    if (handle->create_config.video_output_format == 3) {
        int picture_type = picture->picture_type;
        if ((picture_type == 1 || picture_type == 2) &&
            work->pending_frame == 0) {
            SFMPVF_EndRefFrm(work->reference_frames[0]);
            work->reference_frames[0] = work->reference_frames[1];
            work->reference_frames[1] = *output_frame;
        }
        {
            int width = picture->width;
            int height = picture->height;
            int aligned_width = ((width + 15) / 16) * 16;
            int luma_stride = ((aligned_width + 31) / 32) * 32;
            int chroma_stride = (((aligned_width / 2) + 31) / 32) * 32;
            int aligned_height;
            int luma_size;
            int chroma_size;

            buffers->forward.luma_stride = luma_stride;
            buffers->forward.chroma_stride = chroma_stride;
            aligned_height = ((height + 15) / 16) * 16;
            buffers->forward.planes[2] =
                work->reference_frames[0]->frame_buffer;
            buffers->forward.planes[0] =
                buffers->forward.planes[2] +
                (luma_size = aligned_height * luma_stride);
            buffers->forward.planes[1] =
                buffers->forward.planes[0] +
                (chroma_size = (aligned_height / 2) * chroma_stride);
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
        MPVPlaneSet* planes;
        int picture_type = picture->picture_type;
        if (picture_type == 1 || picture_type == 2) {
            work->plane_indices[0] ^= 1;
            work->plane_indices[1] ^= 1;
            work->reference_frames[1] = *output_frame;
        }
        planes = work->planes;
        buffers->forward = planes[work->plane_indices[0]];
        buffers->backward = planes[work->plane_indices[1]];
    }
    buffers->output_rfb = (*output_frame)->frame_buffer;
    buffers->picture_info = &(*output_frame)->picture_info;
    buffers->decoded_dct_count = 0;
    buffers->skipped_dct_count = 0;
    handle->playback_runtime.field_28 = 0;
    return 0;
}

static inline void sfmpv_SetVofst(SfdHandle* handle)
{
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvTimeCodeSnapshot* output_start = &timer->output_start;
    SfdMpvTimeCodeSnapshot* current = &timer->current;

    if (output_start->valid == 0) {
        SfdMpvFrameWork* work =
            (SfdMpvFrameWork*)handle->transports[2].context;
        SfdTimeCode timecode = current->timecode;
        int value;
        int scale;
        int mode = 0;

        switch (work->decode_state) {
        case 2:
            mode = 1;
            break;
        case 3:
            mode = 1;
            break;
        case 4:
            mode = 0;
            break;
        case 5:
            break;
        default:
            break;
        }

        if (mode == 0 && handle->conditions_primary[4] != 0) {
            timecode.frame_offset = 0;
        }
        SFTIM_Tc2Time(&timecode, &value, &scale);
        output_start->timecode = timecode;
        output_start->value = value - timer->initial.value;
        output_start->scale = scale;
        output_start->valid = 1;
    }
}

static inline void sfmpv_SetFrmTime(SfdHandle* handle, SfdMpvFrame* frame)
{
    SfdMpvRepeatTimer* timer = (SfdMpvRepeatTimer*)&handle->timer_state;
    SfdMpvTimeCodeSnapshot* frame_time =
        (SfdMpvTimeCodeSnapshot*)&frame->time_unit;

    frame->display_time_scale = frame_time->scale;
    frame->display_time_value = timer->reference_time_offset +
        (frame_time->value - timer->reference_time_origin);
    frame->field_4C = frame_time->value;
    frame->field_50 = frame_time->value + timer->reference_time_offset;
    if (timer->maximum_reference_time < frame->display_time_value) {
        timer->maximum_reference_time = frame->display_time_value;
        timer->maximum_reference_scale = frame->display_time_scale;
    }
}

static inline void sfmpv_SetFrmTtu(SfdHandle* handle, SfdMpvFrame* frame)
{
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvTimeCodeSnapshot* frame_time =
        (SfdMpvTimeCodeSnapshot*)&frame->time_unit;

    *frame_time = timer->current;
    sfmpv_SetFrmTime(handle, frame);
}

static inline int sfmpv_ChkDecRet(SfdHandle* handle, int result,
                                  int consumed, int error_code)
{
    int error;

    switch (result) {
    case 0:
        error = 0;
        break;
    case -2:
        if (consumed > 0) {
            error = 0;
        } else {
            error = SFLIB_SetErr(handle, -2);
        }
        break;
    case -3:
        if (consumed > 0) {
            error = 0;
        } else {
            error = SFLIB_SetErr(handle, -3);
        }
        break;
    case -1:
    default:
        error = SFLIB_SetErr(handle, error_code);
        break;
    }
    return error;
}

static inline void sfmpv_AddRtot(SfdHandle* handle, int consumed)
{
    SFBUF_AddRtotSj(handle, handle->transports[2].parameter_10, consumed);
    handle->playback_runtime.time_values[4] += consumed;
}

/* TODO: [near miss] 97.348760%; donor switch and condition index agree;
 * inlined timer-copy addressing and register coloring remain. */
static int sfmpv_DecodeFrm(SfdHandle* handle, SJ* stream)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    MPVContext* decoder = work->decoder;
    SfdMpvAuxWork* aux =
        (SfdMpvAuxWork*)((unsigned char*)work + sizeof(*work));
    MPVPictureInfo* picture = &work->picture_info;
    MPVFrameBuffers buffers;
    SfdMpvFrame* frame;
    unsigned long long start;
    int decoded_base;
    int skipped_base;
    int flow_before;
    int consumed;
    int result;
    int error;

    if (sfmpv_SetFrmPara(handle, picture, &buffers, &frame) != 0) {
        return 0;
    }
    if (handle->create_config.video_output_format == 3) {
        switch (work->picture_info.picture_type) {
        case 1:
        default:
            decoded_base = 0;
            skipped_base = 0;
            break;
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
        }
    } else {
        decoded_base = 0;
        skipped_base = 0;
    }

    {
        SfdMpvPictureUserData* user_data = frame->picture_user_buffer;
        memcpy(user_data->buffer, aux->picture_buffers.entries[0].buffer,
               aux->picture_buffers.entries[0].size);
        user_data->size = aux->picture_buffers.entries[0].size;
    }
    sfmpv_SetVofst(handle);

    start = UTY_GetTmr();
    flow_before = SJRBF_GetFlowCnt(stream, 0, 1);
    result = MPV_DecodeFrmSj(decoder, stream, &buffers);
    consumed = SJRBF_GetFlowCnt(stream, 0, 1) - flow_before;
    SFTMR_AddTsum(&handle->timer_summaries[picture->picture_type],
                  UTY_GetTmr() - start);
    handle->error_info.field_0C += buffers.decoded_dct_count;
    handle->error_info.field_10 += buffers.skipped_dct_count;
    error = sfmpv_ChkDecRet(handle, result, consumed, 0xFF000F06);
    sfmpv_AddRtot(handle, consumed);
    if (error != 0) {
        SFMPVF_FreeFrm(frame);
        return error;
    }
    if (consumed > 0) {
        SFMPVF_SetGopStat(handle, 0);
        sfmpv_SetFrmTtu(handle, frame);
        frame->picture_order = work->field_088;
        frame->field_40 = buffers.decoded_dct_count + decoded_base +
                          work->frame_state[2];
        frame->field_44 = buffers.skipped_dct_count + skipped_base;
        if (picture->field_38 != 3 && work->pending_frame == 0) {
            work->pending_frame = frame;
        } else {
            work->pending_frame = 0;
        }
        work->frame_state[0] = 0;
        work->frame_state[1] = 0;
        if (work->pending_frame == 0) {
            if (handle->create_config.video_output_format == 3 &&
                (picture->picture_type == 1 || picture->picture_type == 2)) {
                SFMPVF_RefStbyFrm(frame);
            } else {
                SFMPVF_StbyFrm(frame);
            }
            MPV_GetDctCnt(decoder, &handle->playback_runtime.field_08,
                          &handle->playback_runtime.field_0C);
            work->field_084 = 0;
        }
        SFPLY_AddDecPic(handle, 1, picture->picture_type);
    } else if (work->pending_frame == 0) {
        SFMPVF_FreeFrm(frame);
    }
    return 0;
}

static inline int sfmpv_IsSeekSkip(SfdHandle* handle)
{
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;

    if (handle->seek_state.request.position < 0) {
        return 0;
    }
    if (timer->output_start.valid != 0) {
        return 0;
    }
    if (UTY_CmpTime(handle->seek_state.request.position,
                    handle->seek_state.request.field_08,
                    timer->current.value, timer->current.scale) != 0) {
        return 0;
    }
    return 1;
}

static inline int sfmpv_IsCondSkip(SfdHandle* handle, int picture_type)
{
    int enabled;

    switch (picture_type) {
    case 1:
        enabled = handle->conditions_primary[2];
        break;
    case 2:
        enabled = handle->conditions_primary[3];
        break;
    case 3:
        enabled = handle->conditions_primary[4];
        break;
    default:
        return 1;
    }
    if (enabled == 0) {
        return 1;
    }
    return 0;
}

static inline int sfmpv_IsEmptySkip(SfdHandle* handle, int picture_type,
                                    const SJCK* chunk)
{
    SfdMpvPlaybackSettings* settings =
        (SfdMpvPlaybackSettings*)&handle->playback_settings;
    int empty;

    if (SFSET_GetCond(handle, 7) != 0) {
        return 0;
    }
    if (picture_type == 3) {
        empty = MPV_IsEmptyBpic(chunk->data, chunk->len,
                                settings->macroblocks_per_row *
                                    settings->macroblock_rows);
        if (empty != 0) {
            handle->playback_runtime.field_10++;
        }
        return empty;
    }
    if (picture_type == 2) {
        empty = MPV_IsEmptyPpic(chunk->data, chunk->len,
                                settings->macroblocks_per_row *
                                    settings->macroblock_rows);
        if (empty != 0) {
            handle->playback_runtime.field_14++;
        }
        return empty;
    }
    return 0;
}

static inline int sfmpv_IsGopSkip(SfdHandle* handle, int picture_type)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    int skip = 0;

    switch (work->decode_state) {
    case 2:
        if (picture_type == 2 || picture_type == 3) {
            skip = 1;
        }
        break;
    case 3:
        if (picture_type == 3) {
            skip = 1;
        }
        break;
    case 4:
        skip = 0;
        break;
    case 5:
    default:
        break;
    }
    return skip;
}

static inline int sfmpv_IsLateSkip(SfdHandle* handle, int picture_type)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvSkipTimer* skip_timer =
        (SfdMpvSkipTimer*)&handle->timer_state;
    int target = timer->output_start.valid == 0
                     ? 0
                     : skip_timer->time_offset +
                           (skip_timer->current_value - skip_timer->base_value);
    SfdMpvSkipCallback callback = skip_timer->callback;
    int scale = skip_timer->current_scale;
    int value;
    int current_scale;

    if (callback != 0) {
        return callback(handle, picture_type, target, scale);
    }
    if (picture_type == 1) {
        SFTIM_UpdateItime((SfdTimerState*)skip_timer, target);
    }
    if (picture_type == 1 || picture_type == 2) {
        target = SFTIM_GetNextItime((SfdTimerState*)skip_timer, target);
    }
    if (SFTIM_GetSpeed(handle) <= 1000 &&
        work->field_084 >= handle->conditions_primary[36]) {
        return 0;
    }
    SFTIM_GetTime(handle, &value, &current_scale);
    if (value < 0) {
        return 0;
    }
    target -= scale * handle->conditions_primary[40] /
              handle->conditions_primary[41];
    if (UTY_CmpTime(value, current_scale, target, scale) != 0) {
        return 0;
    }
    work->field_084++;
    return 1;
}

static inline void sfmpv_UpdatePicStat(SfdHandle* handle,
                                       MPVPictureInfo* picture, int skip)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    int decode_state = work->decode_state;

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
        if (picture->picture_type == 1 || picture->picture_type == 2) {
            decode_state = 2;
        }
    } else {
        if (decode_state == 2) {
            decode_state = 3;
        } else if (decode_state == 3) {
            decode_state = 5;
        }
    }
    work->decode_state = decode_state;
}

/* TODO: [near miss] 95.171640%; donor GOP arm, callback local, and state
 * ladder remain; late/state joins differ; reject permutation-only temporaries. */
static int sfmpv_IsSkip(SfdHandle* handle, const SJCK* chunk)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    int picture_type;
    int skip;

    if (SFSET_GetCond(handle, 47) == 1) {
        return 1;
    }
    if (SFSET_GetCond(handle, 39) == 1) {
        return 0;
    }
    if ((signed char)work->picture_info.field_58 != 0) {
        return work->frame_state[0];
    }
    picture_type = work->picture_info.picture_type;

    if (sfmpv_IsSeekSkip(handle) != 0) {
        skip = 1;
    } else if (sfmpv_IsCondSkip(handle, picture_type) != 0) {
        skip = 1;
    } else if (sfmpv_IsEmptySkip(handle, picture_type, chunk) != 0) {
        skip = 1;
    } else if (sfmpv_IsGopSkip(handle, picture_type) != 0) {
        skip = 1;
    } else {
        skip = sfmpv_IsLateSkip(handle, picture_type);
    }

    sfmpv_UpdatePicStat(handle, &work->picture_info, skip);
    return skip;
}

/* TODO: [breakthrough] 91.012050%; donor-order buffer/size declarations improve
 * codegen; geometry and tail-loop lowering remain structurally different. */
static int sfmpv_ChkBufSiz(SfdHandle* handle,
                           const SfdMpvPlaybackSettings* settings)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    unsigned char* frame_buffer;
    int frame_size;
    int requested_count = work->setup_values.frame_count;
    int width = settings->width;
    int height = settings->height;
    int aligned_width = ((width + 15) / 16) * 16;
    int aligned_height = ((height + 15) / 16) * 16;
    int luma_stride = ((aligned_width + 31) / 32) * 32;
    int chroma_stride = (((aligned_width / 2) + 31) / 32) * 32;
    int luma_size = aligned_height * luma_stride;
    int chroma_size = (aligned_height / 2) * chroma_stride;
    int configured_size;
    int frame_count;
    int decoded_count;
    int i;
    unsigned char* reference_buffer;
    SfdMpvFrame* frames = work->frames;

    frame_size = luma_size + chroma_size * 2 + 0x20;
    configured_size = sfmpv_CalcFrameSize(work->setup_values.width,
                                          work->setup_values.height);
    if (frame_size * 2 > configured_size * 2) {
        return SFLIB_SetErr(handle, 0xFF000F17);
    }
    frame_buffer = (unsigned char*)work->setup_values.frame_buffer;
    if (frame_buffer == 0) {
        frame_count = requested_count;
    } else {
        int available_size = requested_count * configured_size;
        int used_size = frame_size;

        for (frame_count = 1; frame_count <= 16; frame_count++) {
            if (used_size > available_size) {
                frame_count--;
                break;
            }
            used_size += frame_size;
        }
        /* A full run leaves the loop index at 17, but the table has 16 slots. */
        if (frame_count > 16) {
            frame_count = 16;
        }
        if (frame_count < requested_count) {
            return SFLIB_SetErr(handle, 0xFF000F17);
        }
        reference_buffer = (unsigned char*)work->setup_values.reference_buffer;
        work->saved_pair[0] = reference_buffer;
        work->saved_pair[1] = reference_buffer + frame_size;
        for (i = 0; i < frame_count; i++) {
            work->address_table[i] = frame_buffer + i * frame_size;
        }
    }

    sfmpv_SetPlane(&work->saved_pair[0], luma_stride, chroma_stride,
                   luma_size, chroma_size, &work->planes[0]);
    sfmpv_SetPlane(&work->saved_pair[1], luma_stride, chroma_stride,
                   luma_size, chroma_size, &work->planes[1]);
    if (handle->create_config.video_output_format == 3) {
        decoded_count = frame_count < 14 ? frame_count : 14;
        work->frame_count = decoded_count + 2;
        sfmpv_InitSavedFrames(work, frames);
        sfmpv_InitDecodedFrames(work, frames + 2, decoded_count);
        work->reference_frames[0] = SFMPVF_AllocFrm(handle);
        work->reference_frames[1] = SFMPVF_AllocFrm(handle);
    } else {
        decoded_count = frame_count < 16 ? frame_count : 16;
        work->frame_count = decoded_count;
        sfmpv_InitDecodedFrames(work, frames, decoded_count);
    }
    return 0;
}

#pragma pool_data off
/* TODO: [near miss] 95.630250%; retail opcode, CFG, offsets, and constants agree;
 * remaining drop-frame/normal-path register coloring is a soft ceiling. */
static void sfmpv_Pts2Tc(long long pts, int frame_rate_code, int drop_frame,
                         int frame_offset, SfdTimeCode* timecode)
{
    const int nominal_rate = sfmpv_fps_round[frame_rate_code];
    const int rate = SFTIM_prate[frame_rate_code];
    int doubled_frames = (int)UTY_MulDivRound64(pts, rate * 2, 90000000);
    int frames = (doubled_frames >> 1) - frame_offset;
    int hours;
    int minutes;
    int seconds;
    int pictures;

    timecode->subframe = doubled_frames & 1;
    frames = frames > 0 ? frames : 0;
    timecode->frame_rate_code = frame_rate_code;
    timecode->drop_frame = drop_frame;
    if (drop_frame != 0 && (rate == 29970 || rate == 59940)) {
        const SfdMpvDropFrameConversion* conversion =
            rate == 29970 ? &sfmpv_conv_29_97 : &sfmpv_conv_59_94;
        int remaining_frames;
        int minute_in_cycle;

        hours = frames / conversion->frames_per_hour;
        remaining_frames = frames % conversion->frames_per_hour;
        minute_in_cycle =
            remaining_frames / conversion->frames_per_ten_minutes;
        remaining_frames %= conversion->frames_per_ten_minutes;
        if (remaining_frames < conversion->frames_first_minute) {
            minutes = 0;
            seconds =
                remaining_frames / conversion->nominal_frame_rate;
            pictures =
                remaining_frames % conversion->nominal_frame_rate;
        } else {
            remaining_frames -= conversion->frames_first_minute;
            minutes = remaining_frames / conversion->frames_other_minute + 1;
            remaining_frames %= conversion->frames_other_minute;
            if (remaining_frames < conversion->first_minute_threshold) {
                seconds = 0;
                pictures = remaining_frames + conversion->dropped_frames;
            } else {
                remaining_frames -= conversion->first_minute_threshold;
                seconds =
                    remaining_frames / conversion->nominal_frame_rate + 1;
                pictures =
                    remaining_frames % conversion->nominal_frame_rate;
            }
        }
        minutes += conversion->minutes_per_cycle * minute_in_cycle;
    } else {
        int total_seconds = frames / nominal_rate;
        int total_minutes;

        pictures = frames % nominal_rate;
        total_minutes = total_seconds / 60;
        seconds = total_seconds % 60;
        hours = total_minutes / 60;
        minutes = total_minutes % 60;
    }
    timecode->hours = hours;
    timecode->minutes = minutes;
    timecode->seconds = seconds;
    timecode->frames = pictures;
}
#pragma pool_data on

/* TODO: [near miss] 95.135130%; retail operations/CFG agree; stop at
 * timer-base and accumulated-field register coloring. */
static void sfmpv_DoReformTc(SfdHandle* handle,
                             MPVPictureInfo* picture, long long pts,
                             int group_changed)
{
    int frame_rate = picture->frame_rate_code;
    int drop_frame = picture->drop_frame_flag;
    SfdMpvTimerOverlay* timer =
        (SfdMpvTimerOverlay*)&handle->timer_state;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    int temporal_reference = picture->temporal_reference;

    if (group_changed != 0 && pts >= 0) {
        sfmpv_Pts2Tc(pts, frame_rate, drop_frame,
                     temporal_reference,
                     &timer->reformed_timecode);
    } else if (timer->source_valid == 0) {
        if (handle->seek_state.work == 0) {
            timer->reformed_timecode.frame_rate_code =
                frame_rate;
            timer->reformed_timecode.drop_frame = 0;
            timer->reformed_timecode.hours = 0;
            timer->reformed_timecode.minutes = 0;
            timer->reformed_timecode.seconds = 0;
            timer->reformed_timecode.frames = 0;
        } else {
            return;
        }
    } else if (group_changed != 0) {
        SfdTimeCode* source = &timer->source_timecode;
        SfdTimeCode* output = &timer->reformed_timecode;
        int type = source->frame_rate_code;
        int round_rate = sfmpv_fps_round[type];
        int fields = source->reserved_1C + source->subframe;
        int frame = source->frames + source->frame_offset + 1;
        int field;
        int hours;
        int minutes;
        int seconds;

        frame += fields / 2;
        field = fields % 2;
        hours = source->hours;
        minutes = source->minutes;
        seconds = source->seconds;
        seconds += frame / round_rate;
        frame %= round_rate;
        minutes += seconds / 60;
        seconds %= 60;
        hours += minutes / 60;
        minutes %= 60;
        if (source->drop_frame != 0 && seconds == 0 &&
            minutes % 10 != 0 && (frame == 0 || frame == 1)) {
            frame = 2;
        }
        output->frame_rate_code = type;
        output->drop_frame = source->drop_frame;
        output->hours = hours;
        output->minutes = minutes;
        output->seconds = seconds;
        output->frames = frame;
        output->subframe = field;
        repeat_timer->entries[0].accumulated = output->subframe;
        repeat_timer->entries[temporal_reference].accumulated =
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

/* TODO: [near miss] 98.649350%; typed indexed clear keeps retail stores;
 * fixed-clear pretest folds under honest bounds, plus tail coloring. */
static void sfmpv_CalcRepeatField(SfdHandle* handle,
                                  MPVPictureInfo* picture,
                                  int group_changed)
{
    SfdMpvFrame* reference_frame;
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    SfdMpvRepeatEntry* entries = repeat_timer->entries;
    int index;
    int entry_index;
    int i;

    timer->current_timecode.frame_rate_code = picture->frame_rate_code;
    timer->current_timecode.drop_frame = picture->drop_frame_flag;
    timer->current_timecode.hours = picture->time_code_hours;
    timer->current_timecode.minutes = picture->time_code_minutes;
    timer->current_timecode.seconds = picture->time_code_seconds;
    timer->current_timecode.frames = picture->time_code_pictures;
    timer->current_timecode.frame_offset = picture->temporal_reference;
    timer->current_timecode.reserved_1C = (signed char)picture->field_54;
    timer->current_timecode.subframe = 0;

    if (group_changed != 0) {
        for (i = 0; i < sizeof(repeat_timer->entries) /
                            sizeof(repeat_timer->entries[0]); i++) {
            entries[i].repeat = -1;
        }
        entries[0].accumulated = -1;
    } else if (picture->picture_type == 1 ||
               picture->picture_type == 2) {
        int reference;
        int end = picture->temporal_reference;
        reference_frame = work->reference_frames[1];
        reference = reference_frame->picture_info.temporal_reference;
        if (end < reference) {
            end += 0x400;
        }
        for (i = reference + 1; i < end; i++) {
            entries[i % 64].repeat = -1;
        }
    }

    index = picture->temporal_reference % 64;
    entries[index].repeat = timer->current_timecode.reserved_1C;
    if (group_changed != 0) {
        entries[index].accumulated = 0;
    } else if (picture->temporal_reference == 0 &&
               entries[0].accumulated == -1) {
        entries[0].accumulated = 0;
    } else {
        for (i = 0; i < 64; i++) {
            entry_index = (index - i + 63) % 64;
            if (entries[entry_index].repeat != -1) {
                entries[index].accumulated =
                    entries[entry_index].repeat +
                    entries[entry_index].accumulated;
                break;
            }
        }
    }
    timer->current_timecode.subframe = entries[index].accumulated;

    if (picture->picture_type == 3 && entries[index].repeat != 0) {
        SfdMpvTimeCodeSnapshot* frame_time;
        int reference;
        SfdMpvRepeatEntry* reference_entry;
        int scale;
        int value;

        reference_frame = work->reference_frames[1];
        reference = reference_frame->picture_info.temporal_reference;
        entry_index = reference % 64;
        reference_entry = &entries[entry_index];

        reference_entry->accumulated =
            entries[index].repeat + entries[index].accumulated;
        ((SfdMpvTimeCodeSnapshot*)&work->reference_frames[1]->time_unit)
            ->timecode.subframe = reference_entry->accumulated;
        reference_frame = work->reference_frames[1];
        frame_time = (SfdMpvTimeCodeSnapshot*)&reference_frame->time_unit;
        SFTIM_Tc2Time(&frame_time->timecode, &value, &scale);
        frame_time->value = value - timer->initial.value;
        frame_time->scale = scale;
        frame_time->valid = 1;
        if (timer->maximum.value <= frame_time->value) {
            timer->maximum = *frame_time;
        }
        sfmpv_SetFrmTime(handle, work->reference_frames[1]);
    }
}

static inline int sfmpv_ChkGopTc(SfdHandle* handle)
{
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvTimeCodeSnapshot* maximum = &timer->maximum;
    SfdTimeCode current_timecode;
    int scale;
    int stored_value;
    int value;
    int window;
    int result;

    if (maximum->valid == 0) {
        result = 0;
    } else {
        current_timecode = timer->current_timecode;
        SFTIM_Tc2Time(&current_timecode, &value, &scale);
        SFTIM_Tc2Time(&maximum->timecode, &stored_value, &scale);
        window = scale * SFSET_GetCond(handle, 53);
        if (value <= stored_value) {
            result = 1;
        } else if (value >= stored_value + window) {
            result = 1;
        } else {
            result = 0;
        }
    }
    return result;
}

static inline void sfmpv_ReformTc(SfdHandle* handle, SfdMpvFrameWork* work,
                                  MPVPictureInfo* picture,
                                  long long timestamp)
{
    int new_group = work->field_110;
    int mode = SFSET_GetCond(handle, 52);

    if (mode == 0) {
        int reform = 1;

        if (timestamp < 0 && picture->group_count != 0 &&
            picture->field_57 == 0) {
            reform = 0;
            if (new_group != 0) {
                reform = sfmpv_ChkGopTc(handle);
            }
        }
        if (reform != 0) {
            SFSET_SetCond(handle, 52, 1);
            mode = 1;
        }
    }
    if (mode == 1) {
        sfmpv_DoReformTc(handle, picture, timestamp, new_group);
    }
}

/* TODO: [near miss] 99.162390%; typed time/settings and pre/post-call work
 * owners align; PTS register web and output stack slots remain. */
static int sfmpv_DecodePicAtr(SfdHandle* handle, const SJCK* header,
                              SJ* stream, int delimiter_type,
                              int* decode_result)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    MPVContext* decoder = work->decoder;
    SfdMpvAuxWork* aux =
        (SfdMpvAuxWork*)((unsigned char*)work + sizeof(*work));
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    MPVPictureInfo* picture = &work->picture_info;
    MPVPictureAttributes* attributes = picture;
    SfdTimeCode initial_timecode;
    SfdTimeCode picture_timecode;
    int initial_scale;
    int initial_value;
    int picture_scale;
    int picture_value;
    const unsigned char* delimiter;
    SfdPtsEntry pts_entry;
    long long timestamp;
    long long raw_pts;
    int flow_before;
    int consumed;
    int result;
    int bit_rate;
    int vbv_buffer_size;
    int vbv_delay;
    int byte_rate;
    int cache_bit_rate;
    int published_bit_rate;
    int published_vbv_size;
    int new_group;
    int pts_buffer_index;
    SfdMpvFrameWork* pts_work;
    SfdMpvTimeCodeSnapshot* initial_snapshot;
    SfdMpvTimeCodeSnapshot* current_snapshot;
    SfdMpvTimeCodeSnapshot* maximum_snapshot;
    SfdMpvPlaybackSettings* settings;
    SfdMpvFrameWork* cache_work;
    SfdMpvFrameWork* output_work;

    aux->picture_buffers.entries[0].size = 0;
    MPV_SetPicUsrBuf(decoder,
                     aux->picture_buffers.entries[0].buffer,
                     aux->picture_buffers.buffer_size);
    flow_before = SJRBF_GetFlowCnt(stream, 0, 1);
    *decode_result = MPV_DecodePicAtrSj(decoder, stream);
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

    result = MPV_GetPicAtr(decoder, picture);
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
            SfdCallbackObject object =
                (SfdCallbackObject)SFSET_GetCond(handle, 95);

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
                            ->picture_info
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
    MPV_GetPicUsr(decoder, 0,
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
        SfdCallbackObject object =
            (SfdCallbackObject)SFSET_GetCond(handle, 78);

        if (callback != 0) {
            delimiter = MPV_SearchDelim(header->data, header->len, 1);
            if (delimiter != 0) {
                callback(object, header->data,
                         (delimiter + 4) - header->data);
            }
        }
    }

    delimiter = MPV_SearchDelim(header->data, header->len, 4);
    new_group = work->field_110;
    pts_work = (SfdMpvFrameWork*)handle->transports[2].context;
    pts_buffer_index = handle->transports[2].parameter_10;
    timestamp = -1;
    raw_pts = -1;
    if (delimiter != 0) {
        SFPTS_ReadPtsQue(handle, pts_buffer_index,
                         (unsigned int)delimiter, &pts_entry);
        if (pts_entry.pts >= 0) {
            int temporal_reference = attributes->temporal_reference;
            int rate = SFTIM_prate[attributes->frame_rate_code];
            int delta;
            long long computed_time;

            if (handle->timer_state.field_0150 < 0) {
                long long initial_pts =
                    pts_entry.pts -
                    (long long)temporal_reference * 90000000 / rate;
                initial_pts = initial_pts > 0 ? initial_pts : 0;
                handle->timer_state.field_0150 = initial_pts;
            }
            computed_time = pts_entry.pts - handle->timer_state.field_0150;
            computed_time = computed_time > 0 ? computed_time : 0;
            if (memcmp(&pts_work->pts_entry, &pts_entry, 4) != 0) {
                pts_work->pts_entry = pts_entry;
                pts_work->field_11C = 0;
                pts_work->field_118 = temporal_reference;
                if (attributes->picture_type == 3) {
                    pts_work->field_120 = 1;
                } else {
                    pts_work->field_120 = 0;
                }
                raw_pts = pts_entry.pts;
            } else {
                if (new_group != 0) {
                    pts_work->field_11C += pts_work->field_120 + 1;
                    pts_work->field_120 = 0;
                    pts_work->field_118 = 0;
                }
                delta = temporal_reference - pts_work->field_118;
                pts_work->field_120 = pts_work->field_120 > delta
                                      ? pts_work->field_120
                                      : delta;
                computed_time +=
                    (long long)(pts_work->field_11C + delta) * 90000000 / rate;
                computed_time = computed_time > 0 ? computed_time : 0;
            }
            timestamp = computed_time;
        }
    }
    aux->picture_pts = raw_pts;
    if ((delimiter_type & work->decode_mode) == 0) {
        return 0;
    }

    sfmpv_CalcRepeatField(handle, picture, work->field_110);
    sfmpv_ReformTc(handle, work, picture, timestamp);

    initial_snapshot = &timer->initial;
    if (initial_snapshot->valid == 0) {
        initial_timecode = timer->current_timecode;
        initial_timecode.frame_offset = 0;
        SFTIM_Tc2Time(&initial_timecode, &initial_value, &initial_scale);
        initial_snapshot->timecode = initial_timecode;
        initial_snapshot->value = initial_value;
        initial_snapshot->scale = initial_scale;
        initial_snapshot->valid = 1;
    }
    maximum_snapshot = &timer->maximum;
    current_snapshot = &timer->current;
    {
        picture_timecode = timer->current_timecode;
        SFTIM_Tc2Time(&picture_timecode, &picture_value, &picture_scale);
        current_snapshot->timecode = picture_timecode;
        current_snapshot->value = picture_value - timer->initial.value;
        current_snapshot->scale = picture_scale;
        current_snapshot->valid = 1;
    }
    if (maximum_snapshot->value <= current_snapshot->value) {
        *maximum_snapshot = *current_snapshot;
    }

    settings = (SfdMpvPlaybackSettings*)&handle->playback_settings;
    output_work = (SfdMpvFrameWork*)handle->transports[2].context;
    if (settings->bit_rate != 0) {
        return 0;
    }
    if (MPV_GetBitRate(decoder, &bit_rate) != 0) {
        return SFLIB_SetErr(handle, 0xFF000F16);
    }
    MPV_GetVbvBufSiz(decoder, &vbv_buffer_size, &vbv_delay,
                     &byte_rate);
    if (SFSET_GetCond(handle, 60) == 0) {
        output_work->active_size_threshold = 0;
    } else {
        int ring_size = SFBUF_GetRingBufSiz(handle, 1);
        if (byte_rate == -1) {
            byte_rate = vbv_buffer_size;
        }
        output_work->active_size_threshold =
            byte_rate < ring_size ? byte_rate : ring_size;
    }

    cache_bit_rate = bit_rate;
    cache_work = (SfdMpvFrameWork*)handle->transports[2].context;
    {
        SfdMpvSeekCache* cache;

        if (handle->seek_state.work == 0) {
            cache = 0;
        } else if (cache_work->field_088 > 0) {
            cache = 0;
        } else {
            cache = (SfdMpvSeekCache*)((unsigned char*)
                handle->seek_state.work + 0xAD0);
        }
        if (cache != 0 && cache->valid == 0) {
            SfdMpvRawHeader* raw_header = &cache->raw_header;

            raw_header->size =
                header->len < (int)sizeof(raw_header->data)
                    ? header->len : (int)sizeof(raw_header->data);
            MEM_Copy(raw_header->data, header->data, raw_header->size);
            if (cache_bit_rate == 0x3FFFF) {
                cache->byte_rate = 0;
                cache->byte_rate_valid = 0;
            } else {
                cache->byte_rate = cache_bit_rate * 50;
                cache->byte_rate_valid = 1;
            }
            cache->initial = timer->initial;
            cache->valid = 1;
        }
    }

    published_bit_rate = bit_rate;
    published_vbv_size = vbv_buffer_size;
    {
        settings->width = attributes->width;
        settings->height = attributes->height;
        settings->macroblocks_per_row = attributes->macroblocks_per_row;
        settings->macroblock_rows = attributes->macroblock_rows;
        settings->frame_rate_code = attributes->frame_rate_code;
        settings->bit_rate = published_bit_rate;
        settings->vbv_buffer_size = published_vbv_size;
        return sfmpv_ChkBufSiz(handle, settings);
    }
}

static inline void sfmpv_SkipEndcode(SfdHandle* handle, SJ* stream)
{
    SJCK chunk;

    for (;;) {
        stream->interface->get_chunk(stream, 1, 4, &chunk);
        if (chunk.len != 4) {
            break;
        }
        if (MPV_CheckDelim(chunk.data) != 0x80) {
            break;
        }
        stream->interface->put_chunk(stream, 0, &chunk);
        sfmpv_AddRtot(handle, 4);
    }
    stream->interface->unget_chunk(stream, 1, &chunk);
}

/* TODO: [breakthrough] 91.495240%; donor rollover, typed timer owners,
 * shared endcode helper, and concat-time sentinel agree; audio CFG remains. */
static int sfmpv_Concat(SfdHandle* handle, SJ* stream)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    SfdMpvDecodeTimer* timer = (SfdMpvDecodeTimer*)&handle->timer_state;
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    int concat_time = 0;

    if (SFSET_GetCond(handle, 6) == 0) {
        SfdMpvTimeCodeSnapshot* maximum = &timer->maximum;
        SfdMpvTimeCodeSnapshot* initial = &timer->initial;

        if (maximum->valid == 0) {
            concat_time = 0;
        } else {
            const SfdTimeCode* source = &maximum->timecode;
            SfdTimeCode timecode;
            int type = source->frame_rate_code;
            int rate = sfmpv_fps_round[type];
            int fields = source->reserved_1C + source->subframe;
            int frame = source->frames + source->frame_offset + 1;
            int field;
            int hours;
            int minutes;
            int seconds;
            int value;
            int scale;

            frame += fields / 2;
            field = fields % 2;
            hours = source->hours;
            minutes = source->minutes;
            seconds = source->seconds;
            seconds += frame / rate;
            frame %= rate;
            minutes += seconds / 60;
            seconds %= 60;
            hours += minutes / 60;
            minutes %= 60;
            if (source->drop_frame != 0 && seconds == 0 &&
                minutes % 10 != 0 && (frame == 0 || frame == 1)) {
                frame = 2;
            }
            timecode.frame_rate_code = type;
            timecode.drop_frame = source->drop_frame;
            timecode.hours = hours;
            timecode.minutes = minutes;
            timecode.seconds = seconds;
            timecode.frames = frame;
            timecode.frame_offset = 0;
            timecode.subframe = field;
            SFTIM_Tc2Time(&timecode, &value, &scale);
            concat_time = value - initial->value;
        }
    } else {
        int sample_rate;
        int samples;
        int* total_samples =
            &handle->timer_state.sample_window.fields_04[0];

        if (handle->create_config.buffer.transport_setup->entries[3] !=
            &SFD_tr_ad_adxt) {
            samples = 0;
            sample_rate = 44100;
        } else if (SFCON_ReadTotSmplQue(handle, &samples, &sample_rate) == 0) {
            concat_time = -1;
        }
        if (concat_time >= 0) {
            *total_samples += samples;
            concat_time = UTY_MulDiv(*total_samples, timer->initial.scale,
                                     sample_rate) -
                          repeat_timer->reference_time_offset;
            if (concat_time < 0) {
                concat_time = 0;
            }
        }
    }
    if (concat_time < 0) {
        return -1;
    }

    if (concat_time > 0) {
        SFCON_UpdateConcatTime(handle, concat_time);
        work->field_088++;
    }
    SFTIM_InitTtu((SfdTimerTimeUnit*)&timer->initial, 0x7FFFFFFF);
    SFTIM_InitTtu((SfdTimerTimeUnit*)&timer->maximum, -1);
    work->decode_mode = 0xC0;
    sfmpv_SkipEndcode(handle, stream);
    return 0;
}

/* TODO: [breakthrough] 85.267410%; transport +0x20 and three-argument
 * delimiter call agree; decode dispatch and tail still differ. */
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
        if (handle->transports[2].state < 0) {
            handle->transports[2].state =
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
        sfmpv_SkipEndcode(handle, stream);
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
                if ((signed char)work->picture_info.field_58 == 0) {
                    work->frame_state[0] = 1;
                }
                SFPLY_AddSkipPic(
                    handle, 1, work->picture_info.picture_type);
                *processed = 1;
            }
            return result;
        }
        return sfmpv_DecodeFrm(handle, stream);
    }
    if (delimiter_type != 0x80 &&
        sfmpv_GoDdelim(handle, stream, 0xCC) > 0) {
        *processed = 1;
    }
    return result;
}

/* TODO: [near miss] 94.668144%; signed three-byte caps and search returns
 * agree; inspect remaining cross-chunk helper lowering. */
static int sfmpv_NeedSafeDlmRefresh(const SfdBufferTransfer* transfer,
                                    int current_type,
                                    const unsigned char* delimiter)
{
    unsigned char delimiter_bytes[4];
    int type;
    int ignored_code;

    if (delimiter == 0) {
        return 1;
    }
    if (delimiter == transfer->chunks[0].data) {
        return 1;
    }
    if (delimiter >= transfer->chunks[0].data &&
        delimiter < transfer->chunks[0].data + transfer->chunks[0].len) {
        int overflow = delimiter + 4 -
                       (transfer->chunks[0].data + transfer->chunks[0].len);
        if (overflow > 0) {
            if (overflow > transfer->chunks[1].len) {
                return 1;
            }
            memcpy(delimiter_bytes, delimiter, 4 - overflow);
            memcpy(delimiter_bytes + 4 - overflow,
                   transfer->chunks[1].data, overflow);
        } else {
            memcpy(delimiter_bytes, delimiter, 4);
        }
    } else if (delimiter >= transfer->chunks[1].data &&
               delimiter < transfer->chunks[1].data +
                               transfer->chunks[1].len) {
        int overflow = delimiter + 4 -
                       (transfer->chunks[1].data +
                        transfer->chunks[1].len);
        if (overflow > 0) {
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
                sfmpv_SearchTransferDelimiter(transfer, 8, &ignored_code);
            if (next == 0 || next == delimiter) {
                return 1;
            }
        }
        break;
    case 4:
        if ((current_type & 0x48) != 0) {
            const unsigned char* next =
                sfmpv_SearchTransferDelimiter(transfer, 4, &ignored_code);
            if (next == 0 || next == delimiter) {
                return 1;
            }
        }
        break;
    case 0x40:
    case 0x80:
        break;
    default:
        return 1;
    }
    return 0;
}

static inline int sfmpv_ChkRingSpace(SfdHandle* handle)
{
    int buffer_index = handle->transports[2].parameter_10;

    if (SFBUF_GetRingBufSiz(handle, buffer_index) -
            SFBUF_RingGetDataSiz(handle, buffer_index) <
        handle->create_config.buffer.ring_alignment) {
        return SFLIB_SetErr(handle, 0xFF000F1C);
    }
    return 0;
}

/* TODO: [near miss] 91.512660%; signed three-byte caps and search returns
 * agree; refresh and cross-chunk helper lowering remain. */
static int sfmpv_GetActiveSize(SfdHandle* handle, int* active_size,
                               int* delimiter_type, int* has_data)
{
    int buffer_index = handle->transports[2].parameter_10;
    SfdBufferTransfer transfer;
    const unsigned char* delimiter;
    unsigned char* cached_delimiter;
    unsigned char* cached_end;
    unsigned char* transfer_end;
    int type;
    int result;
    int ignored_code;

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

    type = 0;
    delimiter = sfmpv_SearchTransferDelimiter(&transfer, 0xCE, &type);
    if (delimiter != transfer.chunks[0].data) {
        if (delimiter != 0) {
            *active_size = sfmpv_DelimiterOffset(&transfer, delimiter);
        } else {
            int size = transfer.chunks[0].len + transfer.chunks[1].len - 3;
            *active_size = size < 0 ? 0 : size;
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
        cached_delimiter = 0;
        if (transfer.chunks[1].len == 0) {
            transfer_end = transfer.chunks[0].data +
                           transfer.chunks[0].len;
        } else {
            transfer_end = transfer.chunks[1].data +
                           transfer.chunks[1].len;
        }
        if (cached_end != transfer_end) {
            cached_end = transfer_end;
            cached_delimiter = (unsigned char*)
                sfmpv_BsearchTransferDelimiter(&transfer, 0xCC);
            SFBUF_RingSetDlm(handle, buffer_index, cached_delimiter,
                             cached_end);
        }
    }
    if (cached_delimiter == 0) {
        return sfmpv_ChkRingSpace(handle);
    }

    switch (MPV_CheckDelim(cached_delimiter)) {
    case 8:
        if ((type & 0x40) != 0) {
            const unsigned char* next =
                sfmpv_SearchTransferDelimiter(&transfer, 8, &ignored_code);
            if (next == 0 || next == cached_delimiter) {
                return sfmpv_ChkRingSpace(handle);
            }
        }
        break;
    case 4:
        if ((type & 0x48) != 0) {
            const unsigned char* next =
                sfmpv_SearchTransferDelimiter(&transfer, 4, &ignored_code);
            if (next == 0 || next == cached_delimiter) {
                return sfmpv_ChkRingSpace(handle);
            }
        }
        break;
    default:
        break;
    }
    *active_size = sfmpv_DelimiterOffset(&transfer, cached_delimiter);
    return 0;
}

static inline void sfmpv_DecUsrHdr(SfdHandle* handle, SJCK* header,
                                   int* consumed)
{
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    MPVContext* decoder = work->decoder;

    header->data = (unsigned char*)SFSET_GetCond(handle, 93);
    header->len = SFSET_GetCond(handle, 94);
    if (header->data != 0 && (unsigned int)header->len != 0 &&
        work->decode_mode == 0xC0 &&
        MPV_DecodePicAtr(decoder, header, consumed) == 0) {
        work->decode_state = 2;
        work->decode_mode = 0xC8;
    }
}

static inline int sfmpv_IsEnoughData(SfdHandle* handle)
{
    int buffer;
    SfdMpvFrameWork* work =
        (SfdMpvFrameWork*)handle->transports[2].context;
    MPVContext* decoder = work->decoder;
    int bit_rate;
    int ring_size;

    if (SFBUF_GetTermFlg(handle, handle->transports[2].parameter_10) == 1) {
        return 1;
    }
    if (handle->header_state.processed != 0 &&
        handle->header_state.video.fixed_flag == 0) {
        return 1;
    }
    MPV_GetBitRate(decoder, &bit_rate);
    if (bit_rate == 0x3FFFF) {
        return 1;
    }
    if (SFBUF_GetWTot(handle, 1) >= work->active_size_threshold) {
        return 1;
    }
    buffer = SFTRN_IsSetup(handle, 1) == 0;
    ring_size = SFBUF_GetRingBufSiz(handle, buffer);
    if (SFBUF_GetWTot(handle, buffer) >= ring_size) {
        return 1;
    }
    return 0;
}

static inline int sfmpv_IsPrepared(SfdHandle* handle)
{
    int frame_limit;
    int configured_limit;

    if (SFMPVF_IsTermDec(handle) != 0) {
        return 1;
    }
    frame_limit = handle->conditions_primary[23];
    configured_limit = handle->create_config.picture_user_buffer_minimum;
    if (frame_limit == -1) {
        frame_limit = configured_limit;
    }
    if (configured_limit < frame_limit) {
        frame_limit = configured_limit;
    }
    if (SFMPVF_GetNumFrm(handle) >= frame_limit &&
        sfmpv_IsEnoughData(handle) != 0) {
        return 1;
    }
    return 0;
}

static inline void sfmpv_ChkPrep(SfdHandle* handle)
{
    SfdMpvRepeatTimer* repeat_timer =
        (SfdMpvRepeatTimer*)&handle->timer_state;
    int output_buffer = handle->transports[2].parameter_14;
    int input_buffer = handle->transports[2].parameter_10;

    if (SFBUF_GetPrepFlg(handle, output_buffer) != 1 &&
        SFBUF_GetPrepFlg(handle, input_buffer) == 1) {
        if (sfmpv_IsPrepared(handle) != 0) {
            SFBUF_SetPrepFlg(handle, output_buffer, 1);
            if (repeat_timer->reference_time_origin != 0x7FFFFFFF) {
                ((SfdMpvDecodeTimer*)&handle->timer_state)
                    ->output_start.valid = 1;
            }
        }
    }
}

static inline int sfmpv_SetMpvCondCore(SfdHandle* handle, int condition,
                                        int value)
{
    MPVContext* decoder;

    if (handle == 0) {
        decoder = 0;
    } else {
        if (SFLIB_CheckHn(handle) != 0) {
            return SFLIB_SetErr(0, 0xFF000181);
        }
        decoder =
            ((SfdMpvFrameWork*)handle->transports[2].context)->decoder;
    }
    if (condition == 5) {
        value = 0;
    }
    if (MPV_SetCond(decoder, condition, value) != 0) {
        return SFLIB_SetErr(handle, 0xFF000F12);
    }
    return 0;
}

/* TODO: [near miss] 98.497940%; stack slots and all non-register operands
 * now align; remaining differences are register allocation. */
static int sfmpv_ExecServerSub(SfdHandle* handle)
{
    int result;
    int header_consumed;
    int has_data;
    int delimiter_type;
    int processed;
    int active_size;
    int read_count;
    int write_count;
    SJ* stream;
    SJCK header;
    int frame_count;

    if (SFSET_GetCond(handle, 5) == 0) {
        return 0;
    }
    if (SFBUF_GetTermFlg(handle,
                         handle->transports[2].parameter_14) == 1) {
        return 0;
    }
    if (SFSET_GetCond(handle, 28) != 0 &&
        SFHDS_GetColType(handle) != -1) {
        sfmpv_SetMpvCondCore(handle, 5, 0);
    }
    if (handle->playback_state == 2) {
        sfmpv_DecUsrHdr(handle, &header, &header_consumed);
    }

    for (;;) {
        result = sfmpv_GetActiveSize(handle, &active_size, &delimiter_type,
                                     &has_data);
        if (result != 0) {
            break;
        }
        result = sfmpv_DecodeOneUnit(handle, active_size, delimiter_type,
                                     has_data, &processed);
        if (result != 0) {
            break;
        }
        if (processed == 0) {
            break;
        }
    }

    SFBUF_RingGetSj(handle, handle->transports[2].parameter_10, &stream);
    SFBUF_GetFlowCnt(stream, &write_count, &read_count);
    handle->playback_runtime.time_values[3] =
        SFBUF_UpdateFlowCnt(handle->playback_runtime.time_values[3],
                            write_count);

    sfmpv_ChkPrep(handle);
    frame_count = SFMPVF_GetNumFrm(handle);
    if (frame_count == -1 ||
        (SFMPVF_IsTermDec(handle) != 0 && frame_count == 1 &&
         handle->playback_runtime.frame_outstanding != 0)) {
        SFBUF_SetTermFlg(handle, handle->transports[2].parameter_14, 1);
        if (handle->playback_runtime.decoded_pictures == 0) {
            SFSET_SetCond(handle, 5, 0);
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

/* Retail keeps the compile-time layout checks as branches; level 4 folds
 * them into an unconditional return even with inlining disabled. */
#pragma optimization_level 1
#pragma dont_inline on
static int sfmpv_ChkFatal(void)
{
    int picture_info_size = sizeof(MPVPictureInfo);
    int cached_header_size = sizeof(SfdMpvRawHeader);
    int integer_size = sizeof(int);

    if (picture_info_size != 0x80) {
        return SFLIB_SetErr(0, 0xFF000F19);
    }
    if (cached_header_size > 0x204) {
        return SFLIB_SetErr(0, 0xFF000F1A);
    }
    if (integer_size != 4) {
        return SFLIB_SetErr(0, 0xFF000F1E);
    }
    return 0;
}
#pragma dont_inline reset
#pragma optimization_level 4
/* TODO: [near miss] 99.933334%; pointer-table types restore the body;
 * only the shared BSS relocation base differs. */
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
        return SFLIB_SetErr(0, result == 0xFF03FF05 ? 0xFF000F13
                                                     : 0xFF000F01);
    }
    memset(&sfmpv_para, 0, sizeof(sfmpv_para));
    memset(sfmpv_rfb_adr_tbl, 0, sizeof(sfmpv_rfb_adr_tbl));
    memset(sfmpv_ta_adr_tbl, 0, sizeof(sfmpv_ta_adr_tbl));
    sfmpv_discard_wsiz = 0;
    return 0;
}

/* TODO: [near miss] 97.428570%; retail opcode, CFG, offsets, and constants agree;
 * only buffer-owner/counter register coloring remains; stop at soft ceiling. */
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

void SFMPV_RestoreCond(SfdHandle* handle, const int* conditions, int count)
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

int SFMPV_SaveCond(SfdHandle* handle, int* conditions, int buffer_size)
{
    SfdMpvFrameWork* work = (SfdMpvFrameWork*)handle->transports[2].context;
    int* values = conditions;
    MPVContext* decoder = *(MPVContext**)work;
    unsigned int available;
    int i;
    int count;

    if (decoder == 0) {
        return 0;
    }
    available = (unsigned int)buffer_size / sizeof(int);
    count = available > 16 ? 16 : available;
    for (i = 0; i < count; i++) {
        MPV_GetCond(decoder, i, &values[i]);
    }
    return count;
}

int SFD_SetMpvCond(SfdHandle* handle, int condition, int value)
{
    return sfmpv_SetMpvCondCore(handle, condition, value);
}

void SFD_CalcYccPlane(void* buffer, int width, int height,
                      SfdYccPlane* output)
{
    sfmpv_CalcYccPlaneSub(buffer, width, height, output);
}

/* TODO: [near miss] 99.852270%; scalar BSS prefix agrees; the tentative
 * tables still use a different first-reference order and relocation base. */
void SFD_SetMpvParaTbl(SfdMpvParameters* parameters,
                       void** reference_buffers,
                       void** frame_buffers)
{
    int i;

    sfmpv_para = *parameters;
    sfmpv_para.reference_buffer = 0;
    sfmpv_para.frame_buffer = 0;
    sfmpv_rfb_adr_tbl[0] =
        (void*)(((unsigned long)reference_buffers[0] + 31) & ~31UL);
    sfmpv_rfb_adr_tbl[1] =
        (void*)(((unsigned long)reference_buffers[1] + 31) & ~31UL);
    for (i = 0; i < 16; i++) {
        if (i < parameters->frame_count) {
            sfmpv_ta_adr_tbl[i] =
                (void*)(((unsigned long)frame_buffers[i] + 31) & ~31UL);
        } else {
            sfmpv_ta_adr_tbl[i] = 0;
        }
    }
}
