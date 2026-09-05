#include "cri/adx_dcd.h"
#include "cri/adxt_internal.h"
#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

enum {
    ADXT_MAX_HANDLES = 16,
    ADXT_STATUS_STOP = 0,
    ADXT_STATUS_DECODING_HEADER = 1,
    ADXT_STATUS_BUFFERING = 2,
    ADXT_STATUS_PLAYING = 3,
    ADXT_STATUS_DRAINING = 4,
    ADXT_STATUS_PLAY_END = 5,
    ADXT_STREAM_TYPE_MEMORY = 2,
    ADXT_STREAM_TYPE_SJ = 3,
    ADXT_STREAM_TYPE_LINKED = 4,
    ADXT_SECTOR_SIZE = 0x800,
    ADXT_INPUT_EXTRA_SIZE = 0x24,
    ADXT_OUTPUT_SIZE = 0x2000,
    ADXT_OUTPUT_DISTANCE = 0x2060,
    ADXT_DEFAULT_PAN = -128
};

typedef void (*AdxtAhxDetachCallback)(void);
typedef void (*AdxtPl2DetachCallback)(ADXTHandle* handle);

extern ADXTHandle adxt_obj[];
extern s32 adxt_vsync_cnt;
extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern void ADXERR_CallErrFunc1(const char* message);

extern AdxSjdHandle* ADXSJD_Create(SJ* input, s32 channels, SJ** output);
extern void ADXSJD_Destroy(AdxSjdHandle* decoder);
extern void ADXSJD_ExecServer(void);
extern void ADXSJD_SetInSj(AdxSjdHandle* decoder, SJ* input);
extern void ADXSJD_SetLnkSw(AdxSjdHandle* decoder, s32 enabled);
extern void ADXSJD_Start(AdxSjdHandle* decoder);
extern void ADXSJD_Stop(AdxSjdHandle* decoder);
extern void ADXSJD_TermSupply(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetNumChan(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetSfreq(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetTotalNumSmpl(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetOutBps(AdxSjdHandle* decoder);
extern s32 ADXSJD_GetDecNumSmpl(AdxSjdHandle* decoder);
extern s16 ADXSJD_GetDefOutVol(AdxSjdHandle* decoder);
extern s16 ADXSJD_GetDefPan(AdxSjdHandle* decoder, s32 channel);

extern ADXStream* ADXSTM_Create(SJ* sj, s32 priority);
extern void ADXSTM_Destroy(ADXStream* stream);
extern s32 ADXSTM_SetBufSize(
    ADXStream* stream, s32 minimum_size, s32 maximum_size);
extern void ADXSTM_SetEos(ADXStream* stream, s32 sector_count);
extern void ADXSTM_EntryEosFunc(
    ADXStream* stream, void (*callback)(void* object), void* object);
extern s32 ADXSTM_Seek(ADXStream* stream, s32 position);
extern void ADXSTM_StopNw(ADXStream* stream);
extern void ADXSTM_ReleaseFileNw(ADXStream* stream);
extern void ADXSTM_BindFileNw(
    ADXStream* stream, const char* filename, void* directory,
    s32 offset, s32 sector_count);
extern s32 ADXSTM_Start(ADXStream* stream);

extern AXRNAHandle* ADXRNA_Create(
    SJ** output, s32 maximum_channels, void* work);
extern void ADXRNA_Destroy(AXRNAHandle* rna);
extern void ADXRNA_ExecServer(void);
extern s32 ADXRNA_DiscardData(AXRNAHandle* rna, s32 samples);
extern s32 ADXRNA_GetNumData(AXRNAHandle* rna);
extern void ADXRNA_SetPlaySw(AXRNAHandle* rna, s32 enabled);
extern void ADXRNA_SetTransSw(AXRNAHandle* rna, s32 enabled);
extern void ADXRNA_SetOutVol(AXRNAHandle* rna, s32 volume);
extern void ADXRNA_SetOutPan(AXRNAHandle* rna, s32 channel, s32 pan);

extern LSCObject* LSC_Create(SJ* sj);
extern void LSC_Destroy(LSCObject* controller);
extern void LSC_Stop(LSCObject* controller);
extern void LSC_SetStmHndl(LSCObject* controller, ADXStream* stream);
extern void ADXAMP_Start(ADX_AMP* amplifier);
extern void ADXAMP_Stop(ADX_AMP* amplifier);
extern void ADXAMP_Destroy(ADX_AMP* amplifier);
extern void ADXT_ExecHndl(ADXTHandle* handle);

void ADXT_GetTime(ADXTHandle* handle, s32* sample_count, s32* scale);
void ADXT_Stop(ADXTHandle* handle);
void ADXT_Destroy(ADXTHandle* handle);

static const char adxt_set_key_error[] =
    "E02080860 ADXT_SetKeyString: parameter error";
static const char adxt_get_dec_data_length_error[] =
    "E04041901 ADXT_GetDecDtLen: parameter error";
static const char adxt_get_dec_samples_error[] =
    "E02080818 ADXT_GetDecNumSmpl: parameter error";
static const f64 adxt_signed_conversion_bias = 4503601774854144.0;
static const char adxt_get_pause_error[] =
    "E02080847 ADXT_GetStatPause: parameter error";
static const char adxt_pause_error[] =
    "E02080846 ADXT_Pause: parameter error";
static const char adxt_is_ready_error[] =
    "E02080831 ADXT_IsReadyPlayStart: parameter error";
static const char adxt_set_wait_error[] =
    "E02080830 ADXT_SetWaitPlayStart: parameter error";
static const char adxt_get_input_error[] =
    "E02080833 ADXT_GetInputSj: parameter error";
static const char adxt_set_loop_error[] =
    "E02080828 ADXT_SetLpFlg: parameter error";
static const char adxt_get_loop_error[] =
    "E02080829 ADXT_GetLpCnt: parameter error";
static const char adxt_clear_error[] =
    "E02080844 ADXT_ClearErrCode: parameter error";
static const char adxt_get_error_error[] =
    "E02080843 ADXT_GetErrCode: parameter error";
static const char adxt_completed_error[] =
    "E02080802 ADXT_IsCompleted: parameter error";
static const char adxt_input_safety_error[] =
    "E02080836 ADXT_IsIbufSafety: parameter error";
static const f32 adxt_zero_float = 0.0f;
static const char adxt_input_time_error[] =
    "E02080835 ADXT_GetIbufRemainTime: parameter error";
static const f32 adxt_negative_one = -1.0f;
static const char adxt_get_status_error[] =
    "E02080814 ADXT_GetStat: parameter error";
static const char adxt_get_channels_error[] =
    "E02080820 ADXT_GetNumChan: parameter error";
static const char adxt_get_frequency_error[] =
    "E02080819 ADXT_GetSfreq: parameter error";
static const char adxt_get_output_samples_error[] =
    "E02080837 ADXT_GetNumSmplObuf: parameter error";
static const char adxt_get_input_sectors_error[] =
    "E02080834 ADXT_GetNumSctIbuf: parameter error";
static const char adxt_set_reload_sectors_error[] =
    "E02080839 ADXT_SetReloadSct: parameter error";
static const char adxt_reset_reload_time_error[] =
    "E03111501 ADXT_ResetReloadTime: parameter error";
static const f32 adxt_default_reload_ratio = 0.85f;
static const char adxt_set_reload_time_error[] =
    "E02080838 ADXT_SetReloadTime: parameter error";
static const char adxt_set_server_frequency_error[] =
    "E02080840 ADXT_SetSvrFreq: parameter error";
static const char adxt_get_volume_error[] =
    "E02080824 ADXT_GetOutVol: parameter error";
static const char adxt_set_volume_error[] =
    "E02080823 ADXT_SetOutVol: parameter error";
static const char adxt_get_balance_error[] =
    "E02080871 ADXT_GetOutBalance: parameter error";
static const char adxt_set_balance_error[] =
    "E02080870 ADXT_SetOutBalance: parameter error";
static const char adxt_get_pan_error[] =
    "E02080826 ADXT_GetOutPan: parameter error";
static const char adxt_set_pan_error[] =
    "E02080825 ADXT_SetOutPan: parameter error";
static const char adxt_set_pan_channel_error[] =
    "E8101208 ADXT_SetOutPan: parameter error";
static const char adxt_get_bits_error[] =
    "E02080821 ADXT_GetFmtBps: parameter error";
static const char adxt_get_header_length_error[] =
    "E02080822 ADXT_GetHdrLen: parameter error";
static const char adxt_get_samples_error[] =
    "E02080817 ADXT_GetNumSmpl: parameter error";
static const f32 adxt_ticks_per_vsync = 100.0f;
static const char adxt_get_time_error[] =
    "E02080815 ADXT_GetTime: parameter error";
static const f32 adxt_milliseconds = 1000.0f;
static const f32 adxt_positive_time_limit = 60.0f;
static const f32 adxt_negative_time_limit = -60.0f;
static const char adxt_stop_error[] =
    "E02080813 ADXT_Stop: parameter error";
static const char adxt_start_sj_error[] =
    "E02080812 ADXT_StartSj: parameter error";
static const char adxt_destroy_error[] =
    "E02080805 ADXT_Destroy: parameter error";
static const char adxt_create_error[] =
    "E02080804 ADXT_Create: parameter error";
static const char adxt_too_many_handles_error[] =
    "E03100801 ADXT_Create: Too many handles.\n";

s32 adxstm_seteos_sct = 25;
s32 adxt_time_adjust_sw = 1;

s32 adxt_time_mode = 0;
s32 adxt_tsvr_enter_cnt = 0;
s32 adxt_def_svrfreq = 0;
s32 adxt_last_svrfreq = 0;
AdxtAhxDetachCallback ahxdetachfunc = 0;
AdxtPl2DetachCallback pl2detachfunc = 0;
u32 adxt_time_adjust_cnt = 0;
u32 adxt_svrcnt = 0;
u32 adxt_svrcnt_sjd = 0;
u32 adxt_svrcnt_rna = 0;
u32 adxt_svrcnt_adxf = 0;
u32 adxt_svrcnt_adxstm = 0;
u32 adxt_svrcnt_hndl = 0;
static char adxt_fileid_buf[16];
f32 adxt_diff_av = 0.0f;
s32 adxt_time_unit = 0;
s32 adxt_mvtmp_d = 0;
s32 adxt_mviop_d = 0;
s32 adxt_mviop_f = 0;

static inline void adxt_ExecServers(void)
{
    ADXTHandle* handle;
    s32 index;

    ADXCRS_Lock();
    if (adxt_tsvr_enter_cnt != 0) {
        ADXCRS_Unlock();
        return;
    }
    adxt_tsvr_enter_cnt = 1;
    ADXCRS_Unlock();
    ADXSJD_ExecServer();
    adxt_tsvr_enter_cnt = 2;
    handle = adxt_obj;
    for (index = 0; index < ADXT_MAX_HANDLES; index++, handle++) {
        if (handle->used == 1) {
            ADXT_ExecHndl(handle);
        }
    }
    adxt_tsvr_enter_cnt = 3;
    ADXRNA_ExecServer();
    adxt_tsvr_enter_cnt = 0;
}

static inline SJ* adxt_GetInputSj(ADXTHandle* handle)
{
    return handle->input_sj;
}

s32 ADXT_InsertSilence(ADXTHandle* handle, s32 channels, s32 samples)
{
    SJ* sj;
    SJCK chunk;
    SJCK remainder;
    s32 block_bytes;
    s32 requested_bytes;
    s32 usable_bytes;
    s32 written_bytes;

    sj = adxt_GetInputSj(handle);
    if (sj == 0) {
        return 0;
    }
    block_bytes = channels * 18;
    requested_bytes = (samples / 32) * block_bytes;
    sj->interface->get_chunk(sj, 0, requested_bytes, &chunk);
    usable_bytes = (chunk.len / block_bytes) * block_bytes;
    memset(chunk.data, 0, usable_bytes);
    SJ_SplitChunk(&chunk, usable_bytes, &chunk, &remainder);
    written_bytes = usable_bytes;
    sj->interface->put_chunk(sj, 1, &chunk);
    sj->interface->unget_chunk(sj, 0, &remainder);
    sj->interface->get_chunk(
        sj, 0, requested_bytes - written_bytes, &chunk);
    usable_bytes = (chunk.len / block_bytes) * block_bytes;
    memset(chunk.data, 0, usable_bytes);
    SJ_SplitChunk(&chunk, usable_bytes, &chunk, &remainder);
    sj->interface->put_chunk(sj, 1, &chunk);
    sj->interface->unget_chunk(sj, 0, &remainder);
    return ((written_bytes + usable_bytes) / block_bytes) * 32;
}

s32 ADXT_IsEndcode(const u8* data, s32 size, s32* end_size)
{
    if (size < 2) {
        return 0;
    }
    if (*(const u16*)data != 0x8001) {
        return 0;
    }
    *end_size = size;
    return 1;
}

s32 ADXT_IsHeader(const u8* data, s32 size, s32* header_size)
{
    s16 data_length;
    s8 encoding;
    s8 bits_per_sample;
    s8 block_size;
    s8 channels;
    int sample_rate;
    int total_samples;
    int samples_per_block;

    if (size < 2) {
        return 0;
    }
    if (*(const u16*)data != 0x8000) {
        return 0;
    }
    if (ADX_DecodeInfo(
            (AdxHeader*)data, size, &data_length, &encoding,
            &bits_per_sample, &block_size, &channels, &sample_rate,
            &total_samples, &samples_per_block) < 0) {
        return 0;
    }
    *header_size = data_length;
    return 1;
}

void ADXT_SetLnkSw(ADXTHandle* handle, s32 enabled)
{
    handle->link_enabled = enabled;
    if (handle->decoder != 0) {
        ADXSJD_SetLnkSw(handle->decoder, enabled);
    }
}

void ADXT_SetTimeOfst(ADXTHandle* handle, s32 offset)
{
    handle->time_offset = offset;
}

/* TODO: [near miss] 99.9885%; filename buffer excluded from pooled BSS; explicit array initialization put it in data and was restored. */
s32 ADXT_DiscardSmpl(ADXTHandle* handle, s32 samples)
{
    s32 discarded;
    s32 count;
    s32 scale;
    s32 saved_time_mode;

    if (handle->paused == 0) {
        return 0;
    }
    discarded = ADXRNA_DiscardData(handle->rna, samples);
    adxt_ExecServers();
    saved_time_mode = adxt_time_mode;
    adxt_time_mode = 0;
    ADXT_GetTime(handle, &count, &scale);
    adxt_time_mode = saved_time_mode;
    handle->playback_time =
        (u32)((f32)adxt_time_unit * ((f32)count / (f32)scale));
    handle->playback_start_vsync = adxt_vsync_cnt;
    return discarded;
}

void ADXT_TermSupply(ADXTHandle* handle)
{
    ADXSJD_TermSupply(handle->decoder);
}

void ADXT_GetTranspose(
    ADXTHandle* handle, s32* transpose, s32* fine_transpose)
{
}

void ADXT_SetTranspose(
    ADXTHandle* handle, s32 transpose, s32 fine_transpose)
{
}

void ADXT_Pause(ADXTHandle* handle, s32 paused)
{
    s32 status;
    s32 count;
    s32 scale;
    s32 saved_time_mode;

    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_pause_error);
        return;
    }
    status = handle->status;
    if (paused == handle->paused) {
        return;
    }
    ADXCRS_Lock();
    handle->paused = paused;
    if (status == ADXT_STATUS_PLAYING || status == ADXT_STATUS_DRAINING) {
        if (paused == 1) {
            ADXRNA_SetPlaySw(handle->rna, 0);
        } else {
            ADXRNA_SetPlaySw(handle->rna, 1);
            handle->playback_start_vsync = adxt_vsync_cnt;
        }
        saved_time_mode = adxt_time_mode;
        adxt_time_mode = 0;
        ADXT_GetTime(handle, &count, &scale);
        adxt_time_mode = saved_time_mode;
        handle->playback_time =
            (u32)((f32)adxt_time_unit * ((f32)count / (f32)scale));
    }
    ADXCRS_Unlock();
}

s32 ADXT_GetErrCode(ADXTHandle* handle)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_error_error);
        return -1;
    }
    return handle->error_code;
}

void ADXT_ExecServer(void)
{
    adxt_ExecServers();
}

void ADXT_SetAutoRcvr(ADXTHandle* handle, s32 mode)
{
    handle->auto_receiver = mode;
}

void ADXT_SetSvrFreq(ADXTHandle* handle, s32 frequency)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_set_server_frequency_error);
        return;
    }
    handle->server_frequency = frequency;
    adxt_last_svrfreq = frequency;
}

void ADXT_SetDefSvrFreq(s32 frequency)
{
    adxt_def_svrfreq = frequency;
    adxt_last_svrfreq = frequency;
}

s32 ADXT_GetOutVol(ADXTHandle* handle)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_volume_error);
        return 0;
    }
    return handle->output_volume;
}

void ADXT_SetOutVol(ADXTHandle* handle, s32 volume)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_set_volume_error);
        return;
    }
    handle->output_volume = volume;
    ADXRNA_SetOutVol(
        handle->rna,
        handle->output_volume + ADXSJD_GetDefOutVol(handle->decoder));
}

s32 ADXT_GetOutPan(ADXTHandle* handle, s32 channel)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_pan_error);
        return 0;
    }
    return handle->output_pan[channel];
}

void ADXT_SetOutPan(ADXTHandle* handle, s32 channel, s32 pan)
{
    s16 default_pan;

    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_set_pan_error);
        return;
    }
    default_pan = ADXSJD_GetDefPan(handle->decoder, channel);
    if (default_pan == ADXT_DEFAULT_PAN) {
        default_pan = 0;
    }
    handle->output_pan[channel] = pan + default_pan;
    if (channel < handle->maximum_channels) {
        ADXRNA_SetOutPan(handle->rna, channel, pan);
    } else {
        ADXERR_CallErrFunc1(adxt_set_pan_channel_error);
    }
}

s32 ADXT_GetNumChan(ADXTHandle* handle)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_channels_error);
        return -1;
    }
    if (handle->status >= ADXT_STATUS_BUFFERING) {
        return ADXSJD_GetNumChan(handle->decoder);
    }
    return 0;
}

s32 ADXT_GetSfreq(ADXTHandle* handle)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_frequency_error);
        return -1;
    }
    if (handle->status >= ADXT_STATUS_BUFFERING) {
        return ADXSJD_GetSfreq(handle->decoder);
    }
    return 0;
}

s32 ADXT_GetNumSmpl(ADXTHandle* handle)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_samples_error);
        return -1;
    }
    if (handle->status >= ADXT_STATUS_BUFFERING) {
        return ADXSJD_GetTotalNumSmpl(handle->decoder);
    }
    return 0;
}

static inline s32 adxt_GetNumSmplObuf(
    ADXTHandle* handle, s32 channel)
{
    SJ* output;

    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_output_samples_error);
        return -1;
    }
    output = handle->output_sj[channel];
    if (output != 0) {
        return output->interface->get_num_data(output, 1) / 2;
    }
    return 0;
}

static inline void adxt_GetTimeSfreq2(
    ADXTHandle* handle, s32* sample_count, s32* scale)
{
    s32 decoded_samples;
    s32 queued_samples;

    if (handle->status == ADXT_STATUS_PLAYING ||
        handle->status == ADXT_STATUS_DRAINING) {
        *scale = ADXSJD_GetSfreq(handle->decoder);
        decoded_samples = ADXSJD_GetDecNumSmpl(handle->decoder);
        queued_samples = adxt_GetNumSmplObuf(handle, 0);
        *sample_count = handle->linked_decoded_samples + decoded_samples -
                        (ADXRNA_GetNumData(handle->rna) + queued_samples);
    } else if (handle->status == ADXT_STATUS_PLAY_END) {
        *sample_count = ADXSJD_GetTotalNumSmpl(handle->decoder);
        *scale = ADXSJD_GetSfreq(handle->decoder);
        *sample_count *= 16 / ADXSJD_GetOutBps(handle->decoder);
        *sample_count += handle->linked_decoded_samples;
    } else {
        *sample_count = 0;
        *scale = 1;
    }
    *sample_count += handle->time_offset;
}

void ADXT_GetTime(ADXTHandle* handle, s32* sample_count, s32* scale)
{
    s32 actual_count;
    s32 actual_scale;
    s32 saved_time_mode;

    if (handle == 0 || sample_count == 0 || scale == 0) {
        ADXERR_CallErrFunc1(adxt_get_time_error);
        return;
    }
    if (adxt_time_mode == 0) {
        adxt_GetTimeSfreq2(handle, sample_count, scale);
        return;
    }
    adxt_diff_av = 0.0f;
    if (handle->status == ADXT_STATUS_PLAYING ||
        handle->status == ADXT_STATUS_DRAINING) {
        if (handle->paused == 0) {
            *sample_count =
                handle->playback_time +
                (adxt_vsync_cnt - handle->playback_start_vsync) * 100;
        } else {
            *sample_count = handle->playback_time;
        }
        adxt_GetTimeSfreq2(handle, &actual_count, &actual_scale);
        adxt_diff_av =
            1000.0f * ((f32)actual_count / (f32)actual_scale -
                       (f32)*sample_count / (f32)adxt_time_unit);
        if (adxt_diff_av > 60.0f || adxt_diff_av < -60.0f) {
            if (adxt_time_adjust_sw == 1) {
                saved_time_mode = adxt_time_mode;
                adxt_time_mode = 0;
                ADXT_GetTime(handle, &actual_count, &actual_scale);
                adxt_time_mode = saved_time_mode;
                adxt_time_adjust_cnt++;
            }
            handle->playback_time =
                (u32)((f32)adxt_time_unit *
                      ((f32)actual_count / (f32)actual_scale));
            handle->playback_start_vsync = adxt_vsync_cnt;
        }
    } else if (handle->status == ADXT_STATUS_PLAY_END) {
        actual_count = ADXSJD_GetTotalNumSmpl(handle->decoder);
        actual_scale = ADXSJD_GetSfreq(handle->decoder);
        actual_count *= 16 / ADXSJD_GetOutBps(handle->decoder);
        *sample_count =
            (s32)((f32)adxt_time_unit *
                  ((f32)actual_count / (f32)actual_scale));
        *sample_count = handle->playback_time + *sample_count + 1;
    } else {
        *sample_count = 0;
    }
    *sample_count += handle->time_offset;
    *scale = adxt_time_unit;
}

s32 ADXT_GetStat(ADXTHandle* handle)
{
    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_get_status_error);
        return -1;
    }
    return handle->status;
}

void ADXT_Stop(ADXTHandle* handle)
{
    SJ* input;

    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_stop_error);
        return;
    }
    if (handle->stream != 0) {
        ADXSTM_ReleaseFileNw(handle->stream);
    }
    ADXCRS_Lock();
    if (handle->stream_type == ADXT_STREAM_TYPE_LINKED) {
        LSC_Stop(handle->linked_stream_controller);
        if (handle->input_sj != 0) {
            handle->input_sj->interface->reset(handle->input_sj);
        }
    }
    ADXCRS_Lock();
    ADXRNA_SetTransSw(handle->rna, 0);
    ADXRNA_SetPlaySw(handle->rna, 0);
    ADXSJD_Stop(handle->decoder);
    if (handle->stream_type == ADXT_STREAM_TYPE_MEMORY &&
        handle->input_sj != 0) {
        input = handle->input_sj;
        handle->input_sj = 0;
        input->interface->destroy(input);
    }
    if (handle->amplifier != 0) {
        ADXAMP_Stop(handle->amplifier);
    }
    handle->input_sj = 0;
    handle->status = ADXT_STATUS_STOP;
    handle->pending_stream_start = 0;
    ADXCRS_Unlock();
    ADXCRS_Unlock();
}

void ADXT_StartSj(ADXTHandle* handle, SJ* input)
{
    s32 channel;

    if (handle == 0 || input == 0) {
        ADXERR_CallErrFunc1(adxt_start_sj_error);
        return;
    }
    ADXT_Stop(handle);
    ADXCRS_Lock();
    for (channel = 0; channel < handle->maximum_channels; channel++) {
        handle->output_sj[channel]->interface->reset(
            handle->output_sj[channel]);
    }
    ADXSJD_SetInSj(handle->decoder, input);
    handle->input_sj = input;
    ADXSJD_Start(handle->decoder);
    handle->status = ADXT_STATUS_DECODING_HEADER;
    handle->loop_count = 0;
    handle->decoder_ready = 0;
    handle->eos_sector = 0x7FFFFFFF;
    handle->loop_sample_count = -1;
    handle->playback_time = 0;
    handle->linked_decoded_samples = 0;
    handle->playback_start_vsync = adxt_vsync_cnt;
    if (handle->amplifier != 0) {
        ADXAMP_Start(handle->amplifier);
    }
    handle->stream_type = ADXT_STREAM_TYPE_SJ;
    handle->link_enabled = 1;
    if (handle->decoder != 0) {
        ADXSJD_SetLnkSw(handle->decoder, 1);
    }
    ADXCRS_Unlock();
}

void adxt_start_stm(
    ADXTHandle* handle, const char* filename, void* directory,
    s32 file_offset, s32 file_sectors)
{
    SJ* input;
    s32 channel;

    ADXSTM_SetBufSize(
        handle->stream, handle->minimum_buffer_sectors * ADXT_SECTOR_SIZE,
        handle->stream_buffer_sectors * ADXT_SECTOR_SIZE);
    ADXSTM_SetEos(handle->stream, adxstm_seteos_sct);
    ADXSTM_EntryEosFunc(handle->stream, 0, 0);
    ADXSTM_Seek(handle->stream, 0);
    ADXSTM_StopNw(handle->stream);
    ADXSTM_ReleaseFileNw(handle->stream);
    ADXSTM_BindFileNw(
        handle->stream, filename, directory, file_offset, file_sectors);
    ADXSTM_Start(handle->stream);
    input = handle->stream_sj;
    for (channel = 0; channel < handle->maximum_channels; channel++) {
        handle->output_sj[channel]->interface->reset(
            handle->output_sj[channel]);
    }
    ADXSJD_SetInSj(handle->decoder, input);
    handle->input_sj = input;
    ADXSJD_Start(handle->decoder);
    handle->status = ADXT_STATUS_DECODING_HEADER;
    handle->loop_count = 0;
    handle->decoder_ready = 0;
    handle->eos_sector = 0x7FFFFFFF;
    handle->loop_sample_count = -1;
    handle->playback_time = 0;
    handle->linked_decoded_samples = 0;
    handle->playback_start_vsync = adxt_vsync_cnt;
    if (handle->amplifier != 0) {
        ADXAMP_Start(handle->amplifier);
    }
}

void ADXT_DestroyAll(void)
{
    ADXTHandle* handle;
    s32 index;

    for (index = 0; index < ADXT_MAX_HANDLES; index++) {
        handle = &adxt_obj[index];
        if (handle->used == 1) {
            ADXT_Destroy(handle);
        }
    }
}

void ADXT_Destroy(ADXTHandle* handle)
{
    AXRNAHandle* rna;
    AdxSjdHandle* decoder;
    ADXStream* stream;
    LSCObject* controller;
    ADX_AMP* amplifier;
    SJ* sj;
    s32 channel;

    if (handle == 0) {
        ADXERR_CallErrFunc1(adxt_destroy_error);
        return;
    }
    if (ahxdetachfunc != 0) {
        ahxdetachfunc();
    }
    if (pl2detachfunc != 0) {
        pl2detachfunc(handle);
    }
    if (handle->used == 1) {
        ADXT_Stop(handle);
    }
    if (handle->rna != 0) {
        rna = handle->rna;
        handle->rna = 0;
        ADXRNA_Destroy(rna);
    }
    if (handle->decoder != 0) {
        decoder = handle->decoder;
        handle->decoder = 0;
        ADXSJD_Destroy(decoder);
    }
    stream = handle->stream;
    if (stream != 0) {
        handle->stream = 0;
        ADXSTM_EntryEosFunc(stream, 0, 0);
        ADXSTM_Destroy(stream);
    }
    if (handle->linked_stream_controller != 0) {
        controller = handle->linked_stream_controller;
        handle->linked_stream_controller = 0;
        LSC_Destroy(controller);
    }
    ADXCRS_Lock();
    if (handle->stream_sj != 0) {
        sj = handle->stream_sj;
        handle->stream_sj = 0;
        sj->interface->destroy(sj);
    }
    for (channel = 0; channel < handle->maximum_channels; channel++) {
        if (handle->output_sj[channel] != 0) {
            sj = handle->output_sj[channel];
            handle->output_sj[channel] = 0;
            sj->interface->destroy(sj);
        }
        if (handle->amplifier_input[channel] != 0) {
            sj = handle->amplifier_input[channel];
            handle->amplifier_input[channel] = 0;
            sj->interface->destroy(sj);
        }
        if (handle->amplifier_output[channel] != 0) {
            sj = handle->amplifier_output[channel];
            handle->amplifier_output[channel] = 0;
            sj->interface->destroy(sj);
        }
    }
    if (handle->amplifier != 0) {
        amplifier = handle->amplifier;
        handle->amplifier = 0;
        ADXAMP_Destroy(amplifier);
    }
    memset(handle, 0, sizeof(*handle));
    handle->used = 0;
    ADXCRS_Unlock();
}

ADXTHandle* ADXT_Create(s32 maximum_channels, void* work, s32 work_size)
{
    ADXTHandle* handle;
    u8* aligned_work;
    s32 aligned_work_size;
    s32 output_bytes;
    s32 channel;
    s32 index;

    aligned_work = (u8*)(((u32)work + 0x3F) & ~0x3F);
    aligned_work_size = work_size - (aligned_work - (u8*)work);
    if (maximum_channels < 0 || work == 0 || work_size < 0) {
        ADXERR_CallErrFunc1(adxt_create_error);
        return 0;
    }
    for (index = 0; index < ADXT_MAX_HANDLES; index++) {
        if (adxt_obj[index].used == 0) {
            break;
        }
    }
    if (index == ADXT_MAX_HANDLES) {
        ADXERR_CallErrFunc1(adxt_too_many_handles_error);
        return 0;
    }
    handle = &adxt_obj[index];
    memset(handle, 0, sizeof(*handle));
    handle->maximum_channels = maximum_channels;
    output_bytes = maximum_channels * 0x3060 * 2;
    handle->input_buffer = aligned_work + output_bytes;
    handle->input_buffer_size =
        ((aligned_work_size - output_bytes - 0x124) / ADXT_SECTOR_SIZE) *
        ADXT_SECTOR_SIZE;
    handle->input_extra_size = ADXT_INPUT_EXTRA_SIZE;
    handle->work_end = handle->input_buffer + handle->input_buffer_size +
                       handle->input_extra_size;
    handle->output_buffer = aligned_work;
    handle->output_buffer_size = ADXT_OUTPUT_SIZE;
    handle->output_buffer_distance = ADXT_OUTPUT_DISTANCE;
    handle->input_sj = 0;
    handle->stream_sj = SJRBF_Create(
        handle->input_buffer, handle->input_buffer_size,
        handle->input_extra_size);
    if (handle->stream_sj == 0) {
        ADXT_Destroy(handle);
        return 0;
    }
    if ((handle->stream = ADXSTM_Create(handle->stream_sj, 0)) == 0) {
        ADXT_Destroy(handle);
        return 0;
    }
    for (channel = 0; channel < maximum_channels; channel++) {
        handle->output_sj[channel] = SJRBF_Create(
            handle->output_buffer +
                handle->output_buffer_distance * channel * 2,
            handle->output_buffer_size * 2,
            (handle->output_buffer_distance - handle->output_buffer_size) * 2);
        if (handle->output_sj[channel] == 0) {
            ADXT_Destroy(handle);
            return 0;
        }
    }
    if ((handle->decoder = ADXSJD_Create(
             handle->stream_sj, maximum_channels,
             handle->output_sj)) == 0) {
        ADXT_Destroy(handle);
        return 0;
    }
    if ((handle->rna = ADXRNA_Create(
             handle->output_sj, maximum_channels,
             aligned_work + maximum_channels * 0x40C0)) == 0) {
        ADXT_Destroy(handle);
        return 0;
    }
    if ((handle->linked_stream_controller =
             LSC_Create(handle->stream_sj)) == 0) {
        ADXT_Destroy(handle);
        return 0;
    }
    LSC_SetStmHndl(handle->linked_stream_controller, handle->stream);
    ADXCRS_Lock();
    handle->server_frequency = adxt_def_svrfreq;
    handle->stream_buffer_sectors =
        handle->input_buffer_size / ADXT_SECTOR_SIZE;
    handle->minimum_buffer_sectors =
        (s16)(0.85f * (f32)handle->stream_buffer_sectors);
    handle->output_volume = 0;
    for (channel = 0; channel < maximum_channels; channel++) {
        handle->output_pan[channel] = ADXT_DEFAULT_PAN;
    }
    handle->field_46 = 0;
    handle->stream_loop_enabled = 1;
    handle->field_54 = 0;
    handle->field_58 = 0;
    handle->field_5C = 0;
    handle->error_code = 0;
    handle->field_64 = 0;
    handle->field_68 = 0;
    handle->field_6A = 0;
    handle->auto_receiver = 1;
    handle->paused = 0;
    handle->time_offset = 0;
    handle->link_enabled = 0;
    if (handle->decoder != 0) {
        ADXSJD_SetLnkSw(handle->decoder, 0);
    }
    handle->used = 1;
    ADXCRS_Unlock();
    return handle;
}
