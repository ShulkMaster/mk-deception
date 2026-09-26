#include "sofdec/sfd_player.h"
#include "sofdec/uty_math.h"
#include "sofdec/uty_mem.h"
#include "sofdec/uty_timer.h"

SfdPlayerRecordFrameFn SFPLY_recordgetfrm;
int sfply_last_hnctrl_wksiz;
SfdPlayerSetPtsInfoFn SFPLY_SetPtsInfo;
void (*SFPLY_ResetPtsm)(SfdPtsManager* pts);

const unsigned int SFPLY_cond_dfl[101] = {
    1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1,
    0, 1, 0xFFFFFFFD, 1, 0xFFFFFFFC, 1, 0, 3, 0x1000, 0, 1, 0x3C,
    1, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    0xFFFF8AD0, 0xFFFFC950, 0x1F40, 0xEA24, 0xFA0, 0xFA0, 0x29, 0,
    0, 0, 0, 5, 0, 5, 0x022291E0, 0, 0, 0, 0x7FFFFFFF, 0, 0, 1,
    0xA, 0x412B, 0x30D40, 0xFFFFBED5, 0xFFFFBED5, 1, 0x104AC,
    0x20958, 0x7D000, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0xFFFFFFFF,
    0xFFFFFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x5A5A5A5A, 0, 0, 0,
};
static SfdHandle* sfply_InitHn(SfdCreateConfig* create,
                               const void* transport_buffer_setup);

int SFD_SetSupplySj(SfdHandle* handle, SfdBufferSupply* supply)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000139);
    }
    return SFBUF_SetSupplySj(handle, supply);
}

void SFD_RelFrm(SfdHandle* handle, void* frame)
{
    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF000137);
        return;
    }
    handle->playback_runtime.frame_outstanding = 0;
    SFTRN_CallTrtTrif(handle, 6, 12, (SfdTransportValue)frame, 0);
}

int SFD_GetFrm(SfdHandle* handle, void** frame)
{
    int result;

    *frame = 0;
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000136);
    }
    result = SFTRN_CallTrtTrif(handle, 6, 11,
                               (SfdTransportValue)frame, 0);
    if (*frame != 0) {
        handle->playback_runtime.frame_outstanding = 1;
    }
    if (SFPLY_recordgetfrm != 0) {
        SFPLY_recordgetfrm(handle, *frame);
    }
    return result;
}

/* TODO: [review] RE4 carries these two stripped public functions; they are kept only
 * because they restore the retail .bss first-reference order. */
/* Public CRI queries unreferenced in MKD (stripped at link); their order here
 * sets the retail .bss first-reference order. */
int SFPLY_GetLastHnCtrlWkSiz(void) {
    return sfply_last_hnctrl_wksiz;
}

void SFPLY_SetPtsmFn(SfdPlayerSetPtsInfoFn setfn,
                     void (*resetfn)(SfdPtsManager* pts)) {
    SFPLY_SetPtsInfo = setfn;
    SFPLY_ResetPtsm = resetfn;
}

int SFD_TermSupply(SfdHandle* handle)
{
    int buffer_index;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000135);
    }
    buffer_index = handle->transports[0].parameter_14;
    if (SFBUF_GetTermFlg(handle, buffer_index) == 1) {
        return 0;
    }
    SFBUF_SetTermFlg(handle, buffer_index, 1);
    handle->field_0044 = 1;
    return 0;
}

#pragma auto_inline off
static int sfply_ResetHn(SfdHandle* handle)
{
    int saved_count;
    SfdCreateConfig create;
    unsigned int conditions[100];
    int saved_conditions[16];
    SfdBufferTransfer supply;
    SfdPtsManager video_pts;
    int preserve_supply;
    int old_supply_end = 0;
    int new_supply_end;
    SfdErrorCallback error_callback;
    SfdCallbackObject error_object;
    SfdTimeSourceFn user_time_callback;
    SfdExternalClockFn external_clock_callback;
    int external_clock_arg0;
    SfdCallbackObject external_clock_arg1;
    SfdUserIsSkipFn user_is_skip_callback;
    int speed;
    SfdHandle* seek_source;
    int byte_rate;
    int file_size;
    int total_time_value;
    int total_time_scale;
    int seek_position;
    void* video_pts_entries;
    int video_pts_scale;
    int error;

    create = handle->create_config;
    preserve_supply = handle->conditions_primary[8];
    if (preserve_supply != 0) {
        if (SFLIB_CheckHn(handle) != 0) {
            SFLIB_SetErr(0, 0xFF000134);
        } else {
            SFTRN_CallTrtTrif(handle, 0, 9,
                              (SfdTransportValue)&supply, 0);
        }
        old_supply_end = supply.reserved[1];
    }
    SFHDS_FinishFhd(&handle->header_state);
    SFBUF_DestroySj(handle);
    error_object = handle->error_info.callback_object;
    error_callback = handle->error_info.callback;
    user_time_callback = handle->timer_state.time_sources[4];
    external_clock_callback = handle->timer_state.external_clock_callback;
    external_clock_arg0 = handle->timer_state.external_clock_wrap;
    external_clock_arg1 = handle->timer_state.external_clock_object;
    user_is_skip_callback = handle->timer_state.skip_state.callback;
    speed = handle->timer_state.speed;
    video_pts = handle->timer_state.video_pts;
    seek_source = (SfdHandle*)handle->seek_state.work;
    if (seek_source != 0) {
        byte_rate = seek_source->timer_state.stream_time.byte_rate;
        file_size = seek_source->timer_state.stream_time.file_size;
        total_time_value = seek_source->timer_state.stream_time.total_time_value;
        total_time_scale = seek_source->timer_state.stream_time.total_time_scale;
        seek_position = seek_source->timer_state.stream_time.seek_position;
    } else {
        seek_position = 0;
        total_time_scale = 0;
        total_time_value = 0;
        file_size = 0;
        byte_rate = 0;
    }
    video_pts_scale =
        handle->buffers[1].work.ring.pts_queue.capacity *
        (int)sizeof(SfdPtsEntry);
    video_pts_entries = handle->buffers[1].work.ring.pts_queue.entries;
    saved_count = SFMPV_SaveCond(handle, saved_conditions,
                                sizeof(saved_conditions));
    handle->playback_state = 0;
    handle->requested_state = 0;
    error = SFTRN_CallTrSetup(handle, 4);
    if (error != 0) {
        return error;
    }
    MEM_Copy(conditions, handle->conditions_secondary, sizeof(conditions));
    handle = sfply_InitHn(&create, 0);
    if (handle == 0) {
        return SFLIB_SetErr(0, 0xFF000202);
    }
    MEM_Copy(handle->conditions_primary, conditions, sizeof(conditions));
    MEM_Copy(handle->conditions_secondary, conditions, sizeof(conditions));
    SFMPV_RestoreCond(handle, saved_conditions, saved_count);
    if (preserve_supply != 0) {
        if (SFLIB_CheckHn(handle) != 0) {
            error = SFLIB_SetErr(0, 0xFF000134);
        } else {
            error = SFTRN_CallTrtTrif(handle, 0, 9,
                                      (SfdTransportValue)&supply, 0);
        }
        if (error != 0) {
            return error;
        }
        new_supply_end = supply.reserved[1];
        if (SFLIB_CheckHn(handle) != 0) {
            error = SFLIB_SetErr(0, 0xFF000135);
        } else {
            error = SFTRN_CallTrtTrif(handle, 0, 0x0A, old_supply_end,
                                      new_supply_end);
        }
        if (error != 0) {
            return error;
        }
        SFD_TermSupply(handle);
    }
    if (error_callback != 0) {
        SFD_SetErrFn(handle, error_callback, error_object);
    }
    if (user_time_callback != 0) {
        SFD_SetUsrTimeFn(handle, user_time_callback);
    }
    if (external_clock_callback != 0) {
        SFD_SetExtClockFn(handle, external_clock_callback,
                          external_clock_arg0, external_clock_arg1);
    }
    if (user_is_skip_callback != 0) {
        SFD_SetUsrIsSkipFn(handle, user_is_skip_callback);
    }
    if (speed != 1000) {
        SFD_SetSpeed(handle, speed);
    }
    if (video_pts.field_00 != 0) {
        handle->timer_state.video_pts = video_pts;
        if (SFPLY_ResetPtsm != 0) {
            SFPLY_ResetPtsm(&video_pts);
        }
    }
    if (seek_source != 0) {
        SFD_EntrySeek(handle, (SfdSeeWork*)seek_source);
        SFD_SetByteRate(handle, byte_rate);
        SFD_SetFileSize(handle, file_size);
        SFD_SetTotTime(handle, total_time_value, total_time_scale);
        SFD_SetSeekPos(handle, seek_position);
    }
    if (video_pts_entries != 0) {
        SFD_SetVideoPts(handle, video_pts_entries, video_pts_scale);
    }
    return 0;
}
#pragma auto_inline reset

int SFPLY_GetResetFlg(void)
{
    return SFLIB_libwork.reset_in_progress;
}

static inline int sfply_StopTr(SfdHandle* handle)
{
    int result;

    if (handle->playback_state == 4) {
        result = SFTRN_CallTrtTrif(handle, 7, 7, 0, 0);
        if (result != 0) {
            return result;
        }
    }
    handle->playback_state = 1;
    handle->requested_state = 1;
    return 0;
}

static inline int sfply_StopHn(SfdHandle* handle)
{
    int result;

    if (handle->playback_state == 1) {
        return 0;
    }
    result = sfply_StopTr(handle);
    if (result != 0) {
        return result;
    }
    handle->requested_state = 0;
    handle->playback_state = 0;
    SFLIB_libwork.reset_in_progress = 1;
    result = sfply_ResetHn(handle);
    SFLIB_libwork.reset_in_progress = 0;
    if (result != 0) {
        return result;
    }
    return 0;
}

int SFD_Stop(SfdHandle* handle)
{
    int result;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000133);
    }
    result = sfply_StopHn(handle);
    handle->field_0044 = 1;
    return result;
}

int SFD_Start(SfdHandle* handle)
{
    int result;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000132);
    }
    if ((int)SFSET_GetCond(handle, 0x2F) == 1) {
        result = SFPL2_Standby(handle);
    } else {
        result = 0;
        handle->requested_state = 4;
    }
    handle->field_0044 = 1;
    return result;
}

int SFD_Destroy(SfdHandle* handle)
{
    SfdHandle** slot;
    int i;
    int result;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000131);
    }
    sfply_StopHn(handle);
    SFHDS_FinishFhd(&handle->header_state);
    SFBUF_DestroySj(handle);
    handle->playback_state = 0;
    handle->requested_state = 0;
    result = SFTRN_CallTrSetup(handle, 4);
    slot = SFLIB_libwork.handles;
    for (i = 0; i < 8; i++, slot++) {
        if (*slot == handle) {
            *slot = 0;
        }
    }
    return result;
}

void SFPLY_AddSkipPic(SfdHandle* handle, int count, int parameter)
{
    SfdPlayerConditionFn condition;
    int* picture_counts = &handle->playback_runtime.decoded_pictures;

    handle->playback_runtime.skipped_pictures += count;
    condition = (SfdPlayerConditionFn)SFSET_GetCond(handle, 0x25);
    if (condition != 0) {
        condition(handle, parameter, picture_counts);
    }
}

void SFPLY_AddDecPic(SfdHandle* handle, int count, int parameter)
{
    SfdPlayerConditionFn condition;
    int* picture_counts = &handle->playback_runtime.decoded_pictures;

    handle->playback_runtime.decoded_pictures += count;
    condition = (SfdPlayerConditionFn)SFSET_GetCond(handle, 0x24);
    if (condition != 0) {
        condition(handle, parameter, picture_counts);
    }
}

static SfdHandle* sfply_InitHn(SfdCreateConfig* create,
                              const void* transport_buffer_setup)
{
    SfdHandle* handle;
    int i;
    unsigned int work_size;
    unsigned int work_words;
    SfdTimerSummary* summary;
    unsigned char* work;

    work = create->handle_memory;
    work_size = create->handle_memory_size;
    work_words = work_size >> 2;
    if (work == 0) {
        return 0;
    }
    if ((int)work_size <= 0 || work_size > 0x6B70) {
        return 0;
    }
    if (sfply_last_hnctrl_wksiz != 0 &&
        sfply_last_hnctrl_wksiz != (int)work_size) {
        return 0;
    }
    sfply_last_hnctrl_wksiz = work_size;
    UTY_MemsetDword((unsigned int*)work, 0, work_words);
    handle = (SfdHandle*)(((unsigned long)work + 0x1F) & ~0x1FUL);
    handle->requested_state = 0;
    handle->playback_state = 0;
    create->buffer.memory = (unsigned char*)
        (((unsigned long)create->buffer.memory + 0x1F) & ~0x1FUL);
    handle->create_config = *create;
    handle->field_0044 = 1;
    handle->field_0050 = 0;
    handle->field_0054 = 0;
    SFHDS_InitFhd(&handle->header_state);

    UTY_MemsetDword((unsigned int*)&handle->playback_settings, 0, 0x10);
    handle->playback_settings.values_00[0] = 0;
    handle->playback_settings.values_00[1] = 0;
    handle->playback_settings.values_00[2] = 0;
    handle->playback_settings.values_00[3] = 0;
    handle->playback_settings.values_00[4] = 0;
    handle->playback_settings.frame_rate_code = 0;
    handle->playback_settings.values_18[0] = 0;
    handle->playback_settings.values_18[1] = 1;
    handle->playback_settings.values_18[2] = 0;
    handle->playback_settings.values_18[3] = -1;
    handle->playback_settings.values_18[4] = -1;
    handle->playback_settings.values_18[5] = -1;

    UTY_MemsetDword((unsigned int*)&handle->playback_runtime, 0,
                     sizeof(handle->playback_runtime) / sizeof(unsigned int));
    handle->playback_runtime.decoded_pictures = 0;
    handle->playback_runtime.skipped_pictures = 0;
    handle->playback_runtime.field_08 = 0;
    handle->playback_runtime.field_0C = 0;
    handle->playback_runtime.field_10 = 0;
    handle->playback_runtime.frame_outstanding = 0;
    handle->playback_runtime.field_1C = 0;
    handle->playback_runtime.field_20 = 0;
    handle->playback_runtime.field_24 = 0;
    handle->playback_runtime.field_28 = 0;
    handle->playback_runtime.field_2C = 0;
    handle->playback_runtime.time_values[0] = 0;
    handle->playback_runtime.time_values[1] = 0;
    handle->playback_runtime.time_values[2] = 0;
    handle->playback_runtime.time_values[3] = 0;
    handle->playback_runtime.time_values[4] = 0;
    handle->playback_runtime.time_values[5] = 0;
    handle->playback_runtime.time_values[6] = 0;
    handle->playback_runtime.time_values[7] = 0;
    handle->playback_runtime.time_values[8] = 0;
    handle->playback_runtime.time_values[9] = 0;
    handle->playback_runtime.time_values[10] = 0;
    handle->playback_runtime.time_values[11] = 0;

    UTY_MemsetDword((unsigned int*)handle->timer_summaries, 0,
                     sizeof(handle->timer_summaries) / sizeof(unsigned int));
    i = 0;
    summary = handle->timer_summaries;
    do {
        SFTMR_InitTsum(summary);
        i++;
        summary++;
    } while (i < 5);
    SFTMR_InitTsum(&handle->timer_summaries[5]);
    SFLIB_InitErrInf(&handle->error_info);
    MEM_Copy(handle->conditions_primary, SFLIB_libwork.default_conditions,
             sizeof(handle->conditions_primary));
    MEM_Copy(handle->conditions_secondary, SFLIB_libwork.default_conditions,
             sizeof(handle->conditions_secondary));
    SFTIM_InitHn(handle, &handle->timer_state);
    if (SFBUF_InitHn(handle, handle->buffers, &create->buffer) != 0) {
        return 0;
    }
    SFTRN_InitHn(handle, handle->transports, &create->buffer,
                 transport_buffer_setup);
    SFSEE_InitHn(&handle->seek_state);
    if (SFTRN_CallTrSetup(handle, 3) != 0) {
        return 0;
    }
    handle->requested_state = 1;
    handle->playback_state = 1;
    return handle;
}

SfdHandle* SFD_Create(SfdCreateConfig* create,
                     const void* transport_buffer_setup)
{
    int error;
    int i;
    int slot;
    SfdHandle* handle;
    SfdHandle** handles = SFLIB_libwork.handles;

    if (create->buffer.memory == 0) {
        error = SFLIB_SetErr(0, 0xFF000204);
    } else if ((unsigned int)create->handle_memory_size < 0x35B8) {
        error = SFLIB_SetErr(0, 0xFF000205);
    } else {
        error = 0;
    }
    if (error != 0) {
        return 0;
    }
    for (i = 0; i < 8; i++) {
        if (handles[i] == 0) {
            slot = i;
            break;
        } else {
            slot = -1;
        }
    }
    if (slot == -1) {
        SFLIB_SetErr(0, 0xFF000206);
        return 0;
    }
    handle = sfply_InitHn(create, transport_buffer_setup);
    handles[slot] = handle;
    return handle;
}

static inline int sfply_IsVidBufFull(SfdHandle* handle)
{
    int buffer_index = handle->transports[2].parameter_10;
    SfdBufferRingWork* ring = &handle->buffers[buffer_index].work.ring;
    int data_size = ring->supply.stream_joint->interface->get_num_data(
        ring->supply.stream_joint, 1);

    if (data_size >= (ring->supply.buffer_size * 80) / 100 ||
        data_size >= SFSET_GetCond(handle, 0x46)) {
        return 1;
    }
    return 0;
}

static int sfply_IsBpaOn(SfdHandle* handle)
{
    int current_seconds;
    int current_subsecond;
    int reference_seconds;
    int reference_scale;
    int buffer_index;
    int finished;

    if (SFSET_GetCond(handle, 0x43) == 0) return 0;
    if (SFSET_GetCond(handle, 0x0F) == 0) return 0;
    if (handle->field_0050 != 0) return 0;
    if (handle->playback_state != 4) return 0;

    if (SFSET_GetCond(handle, 5) != 0 &&
        SFTRN_GetTermFlg(handle, 6) != 0) {
        finished = 1;
    } else if (SFSET_GetCond(handle, 6) != 0 &&
               SFTRN_GetTermFlg(handle, 7) != 0) {
        finished = 1;
    } else {
        buffer_index = 0;
        for (;;) {
            if (SFBUF_GetTermFlg(handle, buffer_index) != 0) {
                finished = 1;
                break;
            }
            if (++buffer_index >= 8) {
                finished = 0;
                break;
            }
        }
    }
    if (finished != 0) {
        return 0;
    }
    if (SFSET_GetCond(handle, 5) == 1 &&
        handle->playback_runtime.field_24 == 0) {
        return 0;
    }
    if (SFSET_GetCond(handle, 6) == 1 &&
        SFBUF_RingGetDataSiz(handle, 2) > 0) {
        return 0;
    }
    if (SFTRN_IsSetup(handle, 1) != 0 &&
        SFBUF_RingGetDataSiz(handle, 0) > 0) {
        return 0;
    }

    if (SFSET_GetCond(handle, 5) == 1) {
        if (sfply_IsVidBufFull(handle) != 0) return 0;
    }

    SFTIM_GetTime(handle, &current_seconds, &current_subsecond);
    reference_seconds = handle->timer_state.field_0284;
    reference_scale = handle->timer_state.field_0288;
    reference_seconds -= UTY_MulDiv(SFSET_GetCond(handle, 0x44),
                                    reference_scale, 1000000);
    if (current_seconds <= 0 || reference_seconds <= 0) {
        return 0;
    }
    return SFD_CmpTime(current_seconds, current_subsecond, reference_seconds,
                       reference_scale) == 0;
}

static inline int sfply_IsEndTime(SfdHandle* handle)
{
    int current_seconds;
    int current_subsecond;
    int end_seconds = handle->conditions_primary[20];
    int end_subsecond = handle->conditions_primary[21];

    if (end_seconds == -4) return 0;
    SFTIM_GetTime(handle, &current_seconds, &current_subsecond);
    if (current_seconds < 0) return 0;
    if (UTY_CmpTime(current_seconds, current_subsecond,
                    end_seconds, end_subsecond) != 0) return 0;
    return 1;
}

static inline int sfply_IsTermAll(SfdHandle* handle)
{
    int video_terminated;
    int audio_terminated;
    int terminated;

    if (handle->conditions_primary[6] == 0 &&
        handle->conditions_primary[5] == 0) return 1;
    terminated = 0;
    video_terminated = SFTRN_GetTermFlg(handle, 6);
    audio_terminated = SFTRN_GetTermFlg(handle, 7);
    switch (SFSET_GetCond(handle, 0x19)) {
    case 1: terminated = audio_terminated; break;
    case 2: terminated = video_terminated; break;
    case 3: terminated = audio_terminated | video_terminated; break;
    case 0: terminated = audio_terminated & video_terminated; break;
    }
    return terminated;
}

static inline int sfply_IsStagnant(SfdHandle* handle)
{
    if (handle->playback_state != 4 || handle->field_0050 == 1 ||
        handle->playback_runtime.field_1C == 1) return 0;
    if (SFTIM_IsStagnant(handle) != 0) return 1;
    return 0;
}

static inline int sfply_IsOverTime(SfdHandle* handle)
{
    int current_seconds;
    int current_subsecond;
    int stop_seconds;

    if (handle->playback_state != 4 || handle->field_0050 == 1 ||
        handle->playback_runtime.field_1C == 1) return 0;
    if (SFTIM_GetTimeSub(handle, &current_seconds,
                         &current_subsecond) != 0) return 0;
    if (current_seconds < 0) return 0;
    stop_seconds = SFSET_GetCond(handle, 0x36);
    if (SFD_CmpTime(stop_seconds, 1000, current_seconds,
                    current_subsecond) != 0) return 1;
    return 0;
}

static inline int sfply_StopPlay(SfdHandle* handle)
{
    int result = sfply_StopTr(handle);
    if (result != 0) return result;
    handle->requested_state = 6;
    return 0;
}

static inline int sfply_IsTermAny(SfdHandle* handle)
{
    int buffer_index;

    if (SFSET_GetCond(handle, 5) != 0 &&
        SFTRN_GetTermFlg(handle, 6) != 0) return 1;
    if (SFSET_GetCond(handle, 6) != 0 &&
        SFTRN_GetTermFlg(handle, 7) != 0) return 1;
    for (buffer_index = 0; buffer_index < 8; buffer_index++) {
        if (SFBUF_GetTermFlg(handle, buffer_index) != 0) return 1;
    }
    return 0;
}

static inline int sfply_IsAudBufFull(SfdHandle* handle)
{
    int buffer_index = handle->transports[3].parameter_10;
    SfdBufferRingWork* ring = &handle->buffers[buffer_index].work.ring;
    int data_size = ring->supply.stream_joint->interface->get_num_data(
        ring->supply.stream_joint, 1);

    return data_size >= (ring->supply.buffer_size * 80) / 100;
}

static inline int sfply_IsBpaOff(SfdHandle* handle)
{
    int current_seconds;
    int current_subsecond;
    int reference_seconds;
    int reference_scale;

    if (sfply_IsTermAny(handle) != 0) return 1;

    if (SFSET_GetCond(handle, 5) == 1) {
        if (sfply_IsVidBufFull(handle) != 0) return 1;
    }
    if (SFSET_GetCond(handle, 6) == 1) {
        if (sfply_IsAudBufFull(handle) != 0) return 1;
    }
    SFTIM_GetTime(handle, &current_seconds, &current_subsecond);
    reference_seconds = handle->timer_state.field_0284;
    reference_scale = handle->timer_state.field_0288;
    reference_seconds -= UTY_MulDiv(SFSET_GetCond(handle, 0x45),
                                    reference_scale, 1000000);
    if (SFD_CmpTime(current_seconds, current_subsecond,
                    reference_seconds, reference_scale) != 0) return 1;
    return 0;
}

static int sfply_StatPlay(SfdHandle* handle)
{
    int result;
    int state;
    int token;

    if (sfply_IsEndTime(handle) != 0 || sfply_IsTermAll(handle) != 0 ||
        sfply_IsStagnant(handle) != 0 || sfply_IsOverTime(handle) != 0) {
        result = sfply_StopPlay(handle);
    } else {
        result = 0;
    }
    if (result != 0) {
        return handle->playback_state;
    }

    SFLIB_LockCs(&token);
    result = 0;
    if (handle->playback_runtime.field_1C == 0) {
        if (sfply_IsBpaOn(handle) != 0) {
            handle->playback_runtime.field_1C = 1;
            handle->playback_runtime.field_20++;
            result = SFPL2_Pause(handle, 1);
        }
    } else {
        if (sfply_IsBpaOff(handle) != 0) {
            handle->playback_runtime.field_1C = 0;
            result = SFPL2_Pause(handle, 0);
        }
    }
    SFLIB_UnlockCs(&token);
    if (result != 0) {
        return handle->playback_state;
    }
    state = handle->playback_state;
    if (handle->requested_state != 5) {
        if (handle->requested_state < 5) {
            return state;
        }
        if (handle->requested_state < 7) {
            state = 6;
        }
    }
    return state;
}

static inline int sfply_IsPrepared(SfdHandle* handle)
{
    int audio_ready;
    int prep;
    int term;
    int video_ready;

    if (SFSET_GetCond(handle, 5) == 0) {
        video_ready = 1;
    } else {
        prep = SFTRN_GetPrepFlg(handle, 6);
        term = SFTRN_GetTermFlg(handle, 6);
        video_ready = prep | term;
    }
    if (SFSET_GetCond(handle, 6) == 0) {
        audio_ready = 1;
    } else {
        prep = SFTRN_GetPrepFlg(handle, 7);
        term = SFTRN_GetTermFlg(handle, 7);
        audio_ready = prep | term;
    }
    if (video_ready == 0 || audio_ready == 0) {
        return 0;
    }
    return 1;
}

static inline int sfply_IsStartable(SfdHandle* handle)
{
    if (handle->conditions_primary[14] == 0) return 1;
    if (handle->conditions_primary[5] == 0) return 1;
    if (handle->timer_state.field_02B0 != 0) return 1;
    if (handle->timer_state.field_02CC >= handle->conditions_primary[45]) return 1;
    if (sfply_IsTermAll(handle) != 0) return 1;
    return 0;
}

static int sfply_StatPrep(SfdHandle* handle)
{
    int mode;
    int request;
    int selector;
    int state;

    state = handle->playback_state;
    request = handle->requested_state;
    if (sfply_IsPrepared(handle) == 0) {
        return state;
    }

    if (handle->conditions_primary[5] == 1 &&
        SFBUF_GetWTot(handle, 1) == 0 && SFBUF_GetRTot(handle, 1) == 0) {
        handle->conditions_primary[5] = 0;
    }
    if (handle->conditions_primary[6] == 1 &&
        SFBUF_GetWTot(handle, 2) == 0 && SFBUF_GetRTot(handle, 2) == 0) {
        handle->conditions_primary[6] = 0;
    }
    SFSEE_FixAvPlay(handle, handle->conditions_primary[5],
                    handle->conditions_primary[6]);
    if (handle->conditions_primary[6] == 0 &&
        handle->conditions_primary[15] == 2) {
        SFSET_SetCond(handle, 0x0F, 1);
    }
    if (handle->conditions_primary[5] == 0 &&
        handle->conditions_primary[15] == 1) {
        SFSET_SetCond(handle, 0x0F, 2);
    }

    selector = 0;
    if (handle->conditions_primary[6] == 1) {
        selector |= 1;
    }
    if (handle->conditions_primary[5] == 1) {
        selector |= 2;
    }
    switch (selector) {
    case 1:
        mode = 1;
        break;
    case 2:
        mode = 2;
        break;
    case 3:
        mode = SFSET_GetCond(handle, 0x19);
        if (mode == 0 &&
            (UTY_IsTmrVoid() != 0 || SFSET_GetCond(handle, 0x48) == 0)) {
            mode = 3;
        }
        break;
    default:
        mode = 3;
        break;
    }
    SFSET_SetCond(handle, 0x19, mode);

    switch (request) {
    case 2:
        state = 2;
        break;
    case 3:
        state = 3;
        break;
    case 4:
    case 6:
        if (sfply_IsStartable(handle) != 0) {
            SFTRN_CallTrtTrif(handle, 7, 6, 0, 0);
            state = 4;
        } else {
            state = 3;
        }
        break;
    }
    return state;
}

static inline int sfply_StatStby(SfdHandle* handle, int state)
{
    switch (handle->requested_state) {
    case 2:
        state = 2;
        break;
    case 3:
        state = 3;
        break;
    case 4:
    case 6:
        if (sfply_IsStartable(handle) != 0) {
            SFTRN_CallTrtTrif(handle, 7, 6, 0, 0);
            state = 4;
        }
        break;
    }
    return state;
}

static void sfply_ExecOne(SfdHandle* handle)
{
    int state;
    unsigned long long start;

    state = handle->playback_state;
    if ((unsigned int)(state - 1) > 3) {
        return;
    }
    if (handle->field_0044 == 0) {
        return;
    }
    handle->field_0044 = 0;
    start = UTY_GetTmr();
    if ((unsigned int)(state - 2) <= 1 || state == 4) {
        SFTRN_CallTrSetup(handle, 2);
        SFSEE_ExecServer(handle);
    }

    state = handle->playback_state;
    switch (state) {
    case 0:
        break;
    case 1:
        switch (handle->requested_state) {
        case 2:
        case 3:
        case 4:
        case 6:
            state = 2;
            break;
        }
        break;
    case 2:
        state = sfply_StatPrep(handle);
        break;
    case 3:
        state = sfply_StatStby(handle, state);
        break;
    case 4:
        state = sfply_StatPlay(handle);
        break;
    case 5:
        state = handle->playback_state;
        break;
    case 6:
        state = handle->playback_state;
        break;
    }
    handle->playback_state = state;
    SFTMR_AddTsum(&handle->timer_summaries[5], UTY_GetTmr() - start);
}
int SFD_ExecOne(SfdHandle* handle)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000138);
    }
    handle->field_0044 = 1;
    sfply_ExecOne(handle);
    return 0;
}

int SFD_IsSvrWait(void)
{
    SfdHandle** handles = SFLIB_libwork.handles;
    int i;
    int waiting;
    SfdHandle* handle;

    for (i = 0; i < 8; i++) {
        handle = *handles;
        if (handle != 0) {
            if ((unsigned int)(handle->playback_state - 1) > 3) {
                waiting = 1;
            } else if (handle->field_0044 != 0) {
                waiting = 0;
            } else {
                waiting = 1;
            }
            if (waiting == 0) {
                return 0;
            }
        }
        handles++;
    }
    return 1;
}

int SFD_IsHnSvrWait(SfdHandle* handle)
{
    if ((unsigned int)(handle->playback_state - 1) > 3) {
        return 1;
    }
    return handle->field_0044 == 0;
}

void SFD_VbIn(void)
{
    SFTIM_VbIn();
}

void SFPLY_Init(void)
{
    if (SFPLY_cond_dfl[96] != 0x5A5A5A5A) {
        SFLIB_SetErr(0, 0xFF000201);
    }
    SFPLY_recordgetfrm = 0;
}
