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
typedef struct SfdSeeSeekSource {
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
} SfdSeeSeekSource;

typedef char SfdSeeSourcePrefixSizeCheck[
    sizeof(SfdSeeSourcePrefix) == 0x44 ? 1 : -1];
typedef char SfdSeeHeadSnapshotSizeCheck[
    sizeof(SfdSeeHeadSnapshot) == 0x0C ? 1 : -1];
typedef char SfdSeeSeekSourceSizeCheck[
    sizeof(SfdSeeSeekSource) == 0x1308 ? 1 : -1];
typedef char SfdSeeTimingSizeCheck[
    sizeof(SfdSeeTiming) == 0xC0 ? 1 : -1];

extern int SFHDS_GetMuxVerNum(SfdHandle* handle);

static inline SfdSeeSeekSource* sfsee_GetSource(SfdHandle* handle)
{
    return (SfdSeeSeekSource*)handle->seek_state.source_handle;
}

/* Soft ceiling: inlined users omit one redundant retail source-rate reload. */
static inline void sfsee_UpdateByteRate(SfdHandle* handle)
{
    SfdSeeSeekSource* source = sfsee_GetSource(handle);
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
        timing->byte_rate = source->prefix.analyzed_byte_rate;
    }
}

int SFD_SetSeekPos(SfdHandle* handle, int position)
{
    SfdSeeSeekSource* source;

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
    SfdSeeSeekSource* source;

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
    SfdSeeSeekSource* source;

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
    SfdSeeSeekSource* source;

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

static void sfsee_ExecHeadAnaly(SfdHandle* handle)
{
    SfdSeeSeekSource* source = sfsee_GetSource(handle);
    SfdSeeTiming* timing;
    int audio_ready;
    int video_ready;
    int system_ready;
    int analysis_pending;
    int byte_rate;
    int field_08;
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
            timing = &source->timing;
            duration = source->prefix.field_40;
            if (timing->file_size > 0 && duration > 0) {
                byte_rate = UTY_MulDiv(timing->file_size, 1000,
                                       duration);
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

/*
 * Soft ceiling: retail retains a seek-control base and lowers the same nested
 * buffer/transport lookup with indexed addressing; the state changes agree.
 */
void SFSEE_ExecServer(SfdHandle* handle)
{
    SfdSeeSeekSource* source;
    SfdSeeTiming* source_timing;
    SfdSeeTiming* timing;
    SfdTransportState* transports;
    int buffer_index;
    int changed;
    int current_position;
    int position;
    int transport_index;
    int transport_position;

    if (handle->seek_state.source_handle == 0) {
        return;
    }

    sfsee_ExecHeadAnaly(handle);
    source = sfsee_GetSource(handle);
    if (SFCON_IsEndcodeSkip(handle) != 0) {
        return;
    }

    source_timing = &source->timing;
    changed = 0;
    if (source_timing->discovered_file_size <= 0) {
        position = handle->seek_state.field_08 == -3
                       ? 0
                       : source_timing->seek_position;
        if (position >= 0) {
            transports = handle->transports;
            transport_position = -1;
            buffer_index = handle->transports[0].parameter_14;
            transport_index = handle->buffers[buffer_index].input_transport;
            current_position = transports[transport_index].state;
            if (current_position >= 0) {
                transport_position = current_position;
            }
            if (transport_position != -1) {
                source_timing->discovered_file_size =
                    position + transport_position;
                changed = 1;
            }
        }
    }

    if (source_timing->discovered_total_time_value <= 0) {
        timing = (SfdSeeTiming*)&handle->timer_state;
        if (timing->current_total_time_value > 0) {
            source_timing->discovered_total_time_value =
                timing->current_total_time_value;
            changed = 1;
            source_timing->discovered_total_time_scale =
                timing->current_total_time_scale;
        }
    }

    if (changed != 0) {
        sfsee_UpdateByteRate(handle);
    }
}

void SFSEE_FixAvPlay(SfdHandle* handle, int video_enabled, int audio_enabled)
{
    SfdSeeSeekSource* source = sfsee_GetSource(handle);
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

int SFD_EntrySeek(SfdHandle* handle, SfdHandle* source)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000151);
    }
    handle->seek_state.source_handle = source;
    return 0;
}

void SFSEE_InitHn(SfdSeekState* state)
{
    state->source_handle = 0;
    state->field_04 = 0;
    state->field_08 = -3;
    state->field_0C = 1;
}
