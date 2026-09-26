#include "sofdec/sfd_error.h"
#include "sofdec/sfd_transport.h"
#include "sofdec/uty_math.h"

typedef struct SfdSeeSourcePrefix {
    int analyzed;
    int analyzed_byte_rate;
    int field_08;
    int field_0C;
    int fields_10[2];
    int field_18;
    unsigned char unknown_001C[0x24];
    int field_40;
} SfdSeeSourcePrefix;

typedef struct SfdSeeHeadSnapshot {
    int active;
    int byte_rate;
    int field_08;
} SfdSeeHeadSnapshot;

typedef struct SfdSeeTiming {
    unsigned char unknown_0000[0x80];
    int byte_rate;
    int discovered_file_size;
    int discovered_total_time_value;
    int discovered_total_time_scale;
    int video_enabled;
    int audio_enabled;
    int field_98;
    int file_size;
    int total_time_value;
    int total_time_scale;
    int requested_byte_rate;
    int seek_position;
    int field_B0;
    int field_B4;
    int current_total_time_value;
    int current_total_time_scale;
} SfdSeeTiming;

/* A retained source handle doubles as the seek-analysis cache. */
struct SfdSeeWork {
    SfdSeeSourcePrefix prefix;
    unsigned char unknown_0044[0x85C];
    SfdSeeHeadSnapshot system;
    unsigned char unknown_08AC[0x224];
    SfdSeeHeadSnapshot video;
    unsigned char unknown_0ADC[0x230];
    SfdSeeHeadSnapshot audio;
    unsigned char unknown_0D18[0x10];
    SfdSeeTiming timing;
    unsigned char unknown_0DE8[0x520];
};

typedef char SfdSeeSourcePrefixSizeCheck[
    sizeof(SfdSeeSourcePrefix) == 0x44 ? 1 : -1];
typedef char SfdSeeHeadSnapshotSizeCheck[
    sizeof(SfdSeeHeadSnapshot) == 0x0C ? 1 : -1];
typedef char SfdSeeWorkSizeCheck[
    sizeof(SfdSeeWork) == 0x1308 ? 1 : -1];
typedef char SfdSeeTimingSizeCheck[
    sizeof(SfdSeeTiming) == 0xC0 ? 1 : -1];

extern int SFHDS_GetMuxVerNum(SfdHandle* handle);

static inline SfdSeeWork* sfsee_GetSource(SfdHandle* handle)
{
    return handle->seek_state.work;
}

/* Soft ceiling: inlined users omit one redundant retail source-rate reload. */
static inline void sfsee_UpdateByteRate(SfdHandle* handle)
{
    SfdSeeWork* source = sfsee_GetSource(handle);
    SfdSeeTiming* timing = &source->timing;
    int file_size;
    int time_value;
    int time_scale;

    if (timing->requested_byte_rate > 0) {
        timing->byte_rate = timing->requested_byte_rate;
        return;
    }

    file_size = timing->file_size;
    time_value = timing->total_time_value;
    time_scale = timing->total_time_scale;
    if (file_size > 0 && time_value > 0) {
        timing->byte_rate = UTY_MulDiv(file_size, time_scale, time_value);
        return;
    }

    if (source->prefix.analyzed_byte_rate > 0) {
        timing->byte_rate = source->prefix.analyzed_byte_rate;
        return;
    }

    if (file_size <= 0) {
        file_size = timing->discovered_file_size;
    }
    if (time_value <= 0) {
        time_value = timing->discovered_total_time_value;
        time_scale = timing->discovered_total_time_scale;
    }
    if (file_size > 0 && time_value > 0) {
        timing->byte_rate = UTY_MulDiv(file_size, time_scale, time_value);
    } else {
        SfdSeeWork* fallback_source = sfsee_GetSource(handle);
        timing->byte_rate = fallback_source->prefix.analyzed_byte_rate;
    }
}

int SFD_SetSeekPos(SfdHandle* handle, int position)
{
    SfdSeeWork* source;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF00015C);
    }
    source = sfsee_GetSource(handle);
    if (source == 0) {
        return 0;
    }
    source->timing.seek_position = position;
    return 0;
}

int SFD_SetByteRate(SfdHandle* handle, int byte_rate)
{
    SfdSeeWork* source;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF00015B);
    }
    source = sfsee_GetSource(handle);
    if (source == 0) {
        return 0;
    }
    source->timing.requested_byte_rate = byte_rate;
    sfsee_UpdateByteRate(handle);
    return 0;
}

int SFD_SetTotTime(SfdHandle* handle, int value, int scale)
{
    SfdSeeWork* source;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF00015A);
    }
    source = sfsee_GetSource(handle);
    if (source == 0) {
        return 0;
    }
    source->timing.total_time_value = value;
    source->timing.total_time_scale = scale;
    sfsee_UpdateByteRate(handle);
    return 0;
}

int SFD_SetFileSize(SfdHandle* handle, int file_size)
{
    SfdSeeWork* source;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000159);
    }
    source = sfsee_GetSource(handle);
    if (source == 0) {
        return 0;
    }
    source->timing.file_size = file_size;
    sfsee_UpdateByteRate(handle);
    return 0;
}

/* TODO: [near miss] 99.625000%; retail reloads field_18 in the fallback arm; only an invented
 * `int*` field pointer (permuter) reproduces it; sub-struct pointer/else-first/ternary do not. */
static void sfsee_ExecHeadAnaly(SfdHandle* handle)
{
    SfdSeeWork* source = sfsee_GetSource(handle);
    int audio_ready;
    int video_ready;
    int system_ready;
    int analysis_pending;
    int byte_rate;
    int field_08;
    int file_size;
    int duration;

    if (source->prefix.analyzed != 0) {
        return;
    }

    if (SFTRN_IsSetup(handle, 3) != 0 && SFSET_GetCond(handle, 6) == 1) {
        audio_ready = 1;
        if (source->audio.active == 0) {
            analysis_pending = 1;
        } else {
            analysis_pending = 0;
        }
    } else {
        audio_ready = 0;
        analysis_pending = 0;
    }
    if (analysis_pending != 0) {
        return;
    }

    if (SFTRN_IsSetup(handle, 2) != 0 && SFSET_GetCond(handle, 5) == 1) {
        video_ready = 1;
        if (source->video.active == 0) {
            analysis_pending = 1;
        } else {
            analysis_pending = 0;
        }
    } else {
        video_ready = 0;
        analysis_pending = 0;
    }
    if (analysis_pending != 0) {
        return;
    }

    system_ready = SFTRN_IsSetup(handle, 1) != 0;
    if (system_ready != 0) {
        source->system.active = 1;
        if (source->prefix.field_0C != 0 && source->prefix.field_18 > 0) {
            file_size = source->timing.file_size;
            duration = source->prefix.field_40;
            if (file_size > 0 && duration > 0) {
                byte_rate = UTY_MulDiv(file_size, 1000, duration);
            } else {
                byte_rate = source->prefix.field_18;
            }
        } else if (source->prefix.field_0C != 0 &&
                   SFHDS_GetMuxVerNum(handle) < 108) {
            byte_rate = (source->system.byte_rate * 2048) / 2018;
        } else {
            byte_rate = source->system.byte_rate;
        }
        field_08 = source->system.field_08;
    } else if (video_ready != 0) {
        byte_rate = source->video.byte_rate;
        field_08 = source->video.field_08;
    } else if (audio_ready != 0) {
        byte_rate = source->audio.byte_rate;
        field_08 = source->audio.field_08;
    } else {
        return;
    }

    source->prefix.analyzed_byte_rate = byte_rate;
    source->prefix.field_08 = field_08;
    source->prefix.analyzed = 1;
    sfsee_UpdateByteRate(handle);
}

static int sfsee_GetInputEndPosition(SfdHandle* handle)
{
    SfdTransportState* transports;
    SfdTransportState* output;
    int position;

    transports = handle->transports;
    output = &transports[
        handle->buffers[transports[0].parameter_14].output_transport];
    position = output->state;
    if (position >= 0) {
        return position;
    }
    return -1;
}

static void sfsee_ExecEstimate(SfdHandle* handle, SfdSeekState* seek)
{
    int changed;
    int position;
    int transport_position;
    SfdSeeWork* source;
    SfdSeekRequest* request;
    SfdTimerStreamTimeUnit* timer;

    source = seek->work;
    request = &seek->request;
    if (SFCON_IsEndcodeSkip(handle) != 0) {
        return;
    }

    changed = 0;
    if (source->timing.discovered_file_size <= 0) {
        if (request->position == -3) {
            position = 0;
        } else {
            position = source->timing.seek_position;
        }
        if (position >= 0) {
            transport_position = sfsee_GetInputEndPosition(handle);
            if (transport_position != -1) {
                source->timing.discovered_file_size =
                    position + transport_position;
                changed = 1;
            }
        }
    }

    if (source->timing.discovered_total_time_value <= 0) {
        timer = &handle->timer_state.stream_time;
        if (timer->value > 0) {
            source->timing.discovered_total_time_value =
                timer->value;
            changed = 1;
            source->timing.discovered_total_time_scale =
                timer->scale;
        }
    }

    if (changed != 0) {
        sfsee_UpdateByteRate(handle);
    }
}

void SFSEE_ExecServer(SfdHandle* handle)
{
    SfdSeekState* seek = &handle->seek_state;

    if (seek->work == 0) {
        return;
    }
    sfsee_ExecHeadAnaly(handle);
    sfsee_ExecEstimate(handle, seek);
}

void SFSEE_FixAvPlay(SfdHandle* handle, int video_enabled, int audio_enabled)
{
    SfdSeeWork* source = sfsee_GetSource(handle);
    SfdSeeTiming* timing;

    if (source == 0) {
        return;
    }
    timing = &source->timing;
    if (timing->video_enabled < 0) {
        timing->video_enabled = video_enabled;
    }
    if (timing->audio_enabled < 0) {
        timing->audio_enabled = audio_enabled;
    }
}

int SFD_EntrySeek(SfdHandle* handle, SfdSeeWork* work)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000151);
    }
    handle->seek_state.work = work;
    return 0;
}

void SFSEE_InitHn(SfdSeekState* state)
{
    state->work = 0;
    state->request.field_00 = 0;
    state->request.position = -3;
    state->request.field_08 = 1;
}
