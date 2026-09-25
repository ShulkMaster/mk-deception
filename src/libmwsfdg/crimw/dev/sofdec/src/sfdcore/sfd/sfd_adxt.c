#include "cri/sj.h"
#include "runtime/cmath.h"
#include "runtime/cstring.h"
#include "sofdec/sfd_library.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_transport.h"
#include "sofdec/uty_math.h"
#include "sofdec/uty_mem.h"
#include "sofdec/uty_timer.h"

extern double log(double value);

typedef struct AdxtHandle AdxtHandle;
typedef SfdTimerTestWork SfdTestWork;
typedef SfdTimerTestTime SfdTestTime;

typedef struct SfdAdxtSeekInfo {
    int ready;
    int byte_rate;
    int field_08;
    int channels;
    int sample_rate;
    int sample_count;
} SfdAdxtSeekInfo;

typedef void (*SfdAdxtCopyFn)(SfdHandle* handle,
                              const unsigned char* data, int size,
                              int* consumed);

typedef struct SfdAdxtWork {
    AdxtHandle* decoder;
    SJ* stream_joint;
    /* This is the contiguous parameter block retained across reset. */
    SfdAdxtParameters para;
    int maximum_time_value;
    int maximum_time_scale;
    int paused;
    int discarded_samples;
    int header_inserted;
    int sample_offset;
    SfdAdxtCopyFn copy;
    int seek_depth;
    int server_frequency;
    int copied_bytes;
    int field_4C;
} SfdAdxtWork;

struct AdxtHandle {
    unsigned char unknown_00[0x0C];
    int field_0C;
};

typedef char SfdAdxtSeekInfoSizeCheck[
    sizeof(SfdAdxtSeekInfo) == 0x18 ? 1 : -1];
typedef char SfdAdxtWorkSizeCheck[sizeof(SfdAdxtWork) == 0x50 ? 1 : -1];

extern int SFHDS_GetMuxVerNum(SfdHandle* handle);

extern void ADXT_Init(void);
extern void ADXT_Finish(void);
extern AdxtHandle* ADXT_Create(int maximum_channels, void* buffer,
                               int buffer_size);
extern void ADXT_Destroy(AdxtHandle* decoder);
extern void ADXT_StartSj(AdxtHandle* decoder, SJ* stream_joint,
                         SfdAudioGetVolumeFn get_volume,
                         SfdAudioSetVolumeFn set_volume,
                         SfdAudioGetPanFn get_pan,
                         SfdAudioSetPanFn set_pan, int field_0C,
                         SfdAudioOutputCallbacks* callbacks);
extern void ADXT_Stop(AdxtHandle* decoder);
extern void ADXT_Pause(AdxtHandle* decoder, int paused);
extern void ADXT_SetAutoRcvr(AdxtHandle* decoder, int enabled);
extern void ADXGC_SetAdjsfreqFlg(AdxtHandle* decoder, int enabled);
extern int ADXT_GetStat(AdxtHandle* decoder);
extern int ADXT_GetErrCode(AdxtHandle* decoder);
extern int ADXT_GetSfreq(AdxtHandle* decoder);
extern int ADXT_GetNumSmpl(AdxtHandle* decoder);
extern int ADXT_GetNumChan(AdxtHandle* decoder);
extern void ADXT_GetTime(AdxtHandle* decoder, int* value, int* scale,
                         int* status);
extern int ADXT_DiscardSmpl(AdxtHandle* decoder, int samples);
extern void ADXT_TermSupply(AdxtHandle* decoder);
extern void ADXT_SetSvrFreq(AdxtHandle* decoder, int frequency);
extern int ADXT_IsHeader(const unsigned char* data, int size, int* header_size);
extern int ADXT_IsEndcode(const unsigned char* data, int size, int* end_size);
extern void ADXT_InsertHdrSfa(AdxtHandle* decoder, int channels,
                              int sample_rate, int sample_count);
extern void ADXT_SetTimeOfst(AdxtHandle* decoder, int offset);
extern int ADXT_InsertSilence(AdxtHandle* decoder, int channels, int samples);
extern void ADXT_SetTranspose(AdxtHandle* decoder, int semitones,
                              int cents);
extern int ADXT_GetOutVol(AdxtHandle* decoder);
extern int ADXT_SetOutVol(AdxtHandle* decoder, int volume);
extern int ADXT_GetOutPan(AdxtHandle* decoder, int channel);
extern int ADXT_SetOutPan(AdxtHandle* decoder, int channel, int pan);

extern void SFA_Init(void);
extern void SFA_Finish(void);

extern void SFTST_Create(SfdTestWork* work);
extern void SFTST_Calc(SfdTestWork* work, SfdTestTime* master,
                       const SfdTestTime* sample, SfdTestTime* output);
extern void SFTST_GoNextFrame(SfdTestWork* work,
                              const SfdTestTime* elapsed);
extern void SFTST_SetAdjFlg(SfdTestWork* work, int value);
extern void SFTST_Pause(SfdTestWork* work, int value);
extern void SFTST_SetMovaveRange(SfdTestWork* work, int value);
extern void SFTST_SetAdjPoff(SfdTestWork* work, const SfdTestTime* value);
extern void SFTST_SetAdjStart(SfdTestWork* work, const SfdTestTime* value);
extern void SFTST_SetExcessErr(SfdTestWork* work, const SfdTestTime* value);
extern void SFTST_SetTolerance(SfdTestWork* work, const SfdTestTime* value);
extern void SFTST_SetTstFlg(SfdTestWork* work, int value);

extern void UTY_InitTmr(int channel);
extern void UTY_FinishTmr(void);
extern unsigned long long UTY_GetTmrUnit(void);

int sfadxt_stat = 0;
static int sfadxt_adxterr = 0;
static SfdAdxtParameters sfadxt_para;

static inline SfdAdxtWork* sfadxt_GetWork(SfdHandle* handle)
{
    return handle->transports[3].context;
}

static inline SfdTestWork* sfadxt_GetTestWork(SfdHandle* handle)
{
    return &handle->timer_state.test_work;
}

static inline SfdAdxtWork* sfadxt_GetWorkStorage(SfdHandle* handle)
{
    return (SfdAdxtWork*)(handle->unknown_3320 + 0xA0);
}

static inline SfdAdxtSeekInfo* sfadxt_GetSeekInfo(SfdHandle* handle,
                                                  SfdAdxtWork* work)
{
    if (handle->seek_state.work == 0) {
        return 0;
    }
    if (work->seek_depth > 0) {
        return 0;
    }
    return (SfdAdxtSeekInfo*)&((SfdHandle*)handle->seek_state.work)
        ->conditions_secondary[94];
}

static inline int sfadxt_GetAudioInf(SfdHandle* handle, int* channels,
                                     int* sample_rate)
{
    SfdAdxtSeekInfo* seek = sfadxt_GetSeekInfo(handle, sfadxt_GetWork(handle));

    if (seek == 0) {
        return -1;
    }
    *channels = seek->channels;
    *sample_rate = seek->sample_rate;
    return 0;
}

static void sfadxt_ExcludeSilence(SfdHandle* handle,
                                  const unsigned char* data, int size,
                                  int* consumed);
static void sfadxt_ExcludeHdr(SfdHandle* handle, const unsigned char* data,
                              int size, int* consumed);
static void sfadxt_AdjustSync(SfdHandle* handle, const unsigned char* data,
                              int size, int* consumed);
static void sfadxt_CopyData(SfdHandle* handle, const unsigned char* data,
                            int size, int* consumed);
static void SFADXT_SetSpeed(SfdHandle* handle, int speed);
static int SFADXT_GetOutVol(SfdHandle* handle,
                            SfdAudioOutputCallbacks* callbacks);
static int SFADXT_SetOutVol(SfdHandle* handle, int volume,
                            SfdAudioOutputCallbacks* callbacks);
static int SFADXT_GetOutPan(SfdHandle* handle, int channel,
                            SfdAudioOutputCallbacks* callbacks);
static int SFADXT_SetOutPan(SfdHandle* handle, int channel, int pan,
                            SfdAudioOutputCallbacks* callbacks);
static int SFADXT_Seek(SfdHandle* handle, SfdTransportValue parameter,
                       int value);
static int SFADXT_AddRead(SfdHandle* handle, SfdTransportValue parameter,
                          int value);
static int SFADXT_GetRead(SfdHandle* handle, void* output);
static int SFADXT_AddWrite(SfdHandle* handle, SfdTransportValue parameter,
                           int value);
static int SFADXT_GetWrite(SfdHandle* handle, void* output);
static int SFADXT_Pause(SfdHandle* handle, int state);
static int SFADXT_Stop(SfdHandle* handle);
static int SFADXT_Start(SfdHandle* handle);
static int SFADXT_Standby(SfdHandle* handle);
static int SFADXT_Destroy(SfdHandle* handle);
static int SFADXT_Create(SfdHandle* handle);
static int SFADXT_ExecServer(SfdHandle* handle);
static int SFADXT_Finish(SfdHandle* handle);
static int SFADXT_Init(SfdHandle* handle);

const SfdTransportInterface SFD_tr_ad_adxt = {
    SFADXT_Init,     SFADXT_Finish,   SFADXT_ExecServer, SFADXT_Create,
    SFADXT_Destroy,  SFADXT_Standby,  SFADXT_Start,      SFADXT_Stop,
    SFADXT_Pause,    SFADXT_GetWrite, SFADXT_AddWrite,   SFADXT_GetRead,
    SFADXT_AddRead,  SFADXT_Seek,
};

static const unsigned char sfadxt_silence[0x12] = {0};

static int SFADXT_Seek(SfdHandle* handle, SfdTransportValue parameter,
                       int value)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    SfdHandle* source = (SfdHandle*)handle->seek_state.work;
    SfdAdxtSeekInfo* seek;
    AdxtHandle* decoder;

    if (source == 0) {
        seek = 0;
    } else if (work->seek_depth > 0) {
        seek = 0;
    } else {
        seek = (SfdAdxtSeekInfo*)&source->conditions_secondary[94];
    }
    if (seek == 0) {
        return 0;
    }
    decoder = work->decoder;
    if (work->header_inserted != 0) {
        return 0;
    }
    if (seek->ready == 0) {
        return 0;
    }
    ADXT_InsertHdrSfa(decoder, seek->channels, seek->sample_rate,
                      seek->sample_count);
    ADXT_SetTimeOfst(decoder, 0);
    work->header_inserted = 1;
    work->copy = sfadxt_ExcludeSilence;
    return 0;
}

static int SFADXT_AddRead(SfdHandle* handle, SfdTransportValue parameter,
                          int value)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static int SFADXT_GetRead(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static int SFADXT_AddWrite(SfdHandle* handle, SfdTransportValue parameter,
                           int value)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static int SFADXT_GetWrite(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static inline void sfadxt_PauseOff(SfdHandle* handle)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;
    SfdTestWork* test_work = sfadxt_GetTestWork(handle);

    if (work->paused != 1) {
        ADXT_Pause(decoder, 0);
        SFTST_Pause(test_work, 0);
    }
}

static int SFADXT_Pause(SfdHandle* handle, int state)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;

    switch (state) {
    case 0:
        work->discarded_samples = 0;
        sfadxt_PauseOff(handle);
        break;
    case 1:
        ADXT_Pause(decoder, 1);
        SFTST_Pause(sfadxt_GetTestWork(handle), 1);
        break;
    case 2:
    {
        int status = ADXT_GetStat(decoder);
        int is_active;

        if (status == 0 || status == 1) {
            is_active = 0;
        } else {
            is_active = 1;
        }

        if (is_active) {
            int sample_rate = ADXT_GetSfreq(decoder);
            int scale;
            int value;
            int samples;
            int requested;
            SfdTestTime elapsed;

            SFTIM_GetTimeOneFrmVideo(handle, &value, &scale);
            samples = UTY_MulDiv(sample_rate, value, scale);
            requested = samples + work->discarded_samples;
            work->discarded_samples =
                requested - ADXT_DiscardSmpl(decoder, requested);
            elapsed.value = samples;
            elapsed.scale = sample_rate;
            SFTST_GoNextFrame(sfadxt_GetTestWork(handle), &elapsed);
        }
        break;
    }
    }
    return 0;
}

static int SFADXT_Stop(SfdHandle* handle)
{
    ADXT_Stop(sfadxt_GetWork(handle)->decoder);
    return 0;
}

/* TODO: [near miss] 98.958336%; donor-shaped work/decoder/test lifetime is retained; equivalent register coloring remains. */
static int SFADXT_Start(SfdHandle* handle)
{
    SfdAdxtWork* work;
    AdxtHandle* decoder;
    SfdTestWork* test_work;

    work = sfadxt_GetWork(handle);
    decoder = work->decoder;
    test_work = sfadxt_GetTestWork(handle);

    work->paused = 0;
    if (handle->field_0050 != 1) {
        ADXT_Pause(decoder, 0);
        SFTST_Pause(test_work, 0);
    }
    return 0;
}

static int SFADXT_Standby(SfdHandle* handle)
{
    return 0;
}

static int sfadxt_ReleaseAdxt(SfdHandle* handle, AdxtHandle* decoder)
{
    if (SFPLY_GetResetFlg() != 1) {
        ADXT_Destroy(decoder);
        return 0;
    }
    ADXT_Stop(sfadxt_GetWork(handle)->decoder);
    SFLIB_libwork.retained_adxt = decoder;
    return 0;
}

static int SFADXT_Destroy(SfdHandle* handle)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;
    SJ* stream_joint = work->stream_joint;
    int result;

    if (decoder == 0) {
        return 0;
    }
    sfadxt_para = work->para;
    result = sfadxt_ReleaseAdxt(handle, decoder);
    stream_joint->interface->destroy(stream_joint);
    UTY_FinishTmr();
    return result;
}

static inline void sfadxt_UpdateTime(SfdAdxtWork* work,
                                     AdxtHandle* decoder,
                                     SfdTestWork* test_work)
{
    int decoder_value;
    int decoder_scale;
    SfdTestTime master;
    SfdTestTime sample;
    SfdTestTime output;
    int count;
    int unit;

    sfadxt_stat = ADXT_GetStat(decoder);
    ADXT_GetTime(decoder, &decoder_value, &decoder_scale, &sfadxt_stat);
    master.value = decoder_value;
    master.scale = decoder_scale;
    sample.value = UTY_GetTmr();
    sample.scale = UTY_GetTmrUnit();
    SFTST_Calc(test_work, &master, &sample, &output);
    count = (int)output.value;
    unit = (int)output.scale;
    if (work->maximum_time_value < count) {
        work->maximum_time_value = count;
        work->maximum_time_scale = unit;
    }
}

/* TODO: [near miss] 96.428570%; typed update values and stack slots agree;
 * stop at owner-register coloring and one status-call scheduling difference. */
static int sfadxt_GetTime(SfdHandle* handle, int* value, int* scale)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;
    SfdTestWork* test_work = sfadxt_GetTestWork(handle);

    if (SFTIM_ChkRegularTime(handle, value, scale) == 0) {
        return 0;
    }
    if (handle->playback_state == 4) {
        sfadxt_UpdateTime(work, decoder, test_work);
    }
    *value = work->maximum_time_value;
    *scale = work->maximum_time_scale;
    return 0;
}

static int sfadxt_InitInf(SfdHandle* handle, SfdAdxtWork* work)
{
    SfdTestWork* test_work;
    SfdTestTime tolerance;
    SfdTestTime excess_error;
    SfdTestTime adjustment_start;
    SfdTestTime adjustment_offset;

    if (sfadxt_para.stream_buffer == 0 || sfadxt_para.decoder_buffer == 0) {
        return SFLIB_SetErr(0, 0xFF000C06);
    }
    work->para = sfadxt_para;
    work->decoder = 0;
    work->stream_joint = 0;
    work->maximum_time_value = -1;
    work->maximum_time_scale = 1;
    work->paused = 0;
    work->discarded_samples = 0;
    work->header_inserted = 0;
    work->sample_offset = 0;
    work->copy = sfadxt_CopyData;
    work->seek_depth = 0;
    work->server_frequency = -1;
    work->copied_bytes = 0;
    work->field_4C = 0;

    test_work = sfadxt_GetTestWork(handle);
    tolerance.value = SFSET_GetCond(handle, 0x3F);
    tolerance.scale = 1000000;
    excess_error.value = SFSET_GetCond(handle, 0x40);
    excess_error.scale = 1000000;
    adjustment_start.value = SFSET_GetCond(handle, 0x41);
    adjustment_start.scale = 1000000;
    adjustment_offset.value = SFSET_GetCond(handle, 0x42);
    adjustment_offset.scale = 1000000;
    SFTST_Create(test_work);
    SFTST_SetTstFlg(test_work, SFSET_GetCond(handle, 0x48));
    SFTST_SetTolerance(test_work, &tolerance);
    SFTST_SetExcessErr(test_work, &excess_error);
    SFTST_SetAdjStart(test_work, &adjustment_start);
    SFTST_SetAdjPoff(test_work, &adjustment_offset);
    SFTST_SetMovaveRange(test_work, SFSET_GetCond(handle, 0x3E));
    UTY_InitTmr(SFSET_GetCond(handle, 0x3D));
    return 0;
}

static inline void sfadxt_PauseOn(SfdHandle* handle)
{
    SfdAdxtWork* work;
    AdxtHandle* decoder;

    work = sfadxt_GetWork(handle);
    decoder = work->decoder;
    work->paused = 1;
    ADXT_Pause(decoder, 1);
    SFTST_Pause(sfadxt_GetTestWork(handle), 1);
}

static inline AdxtHandle* sfadxt_CreateAdxt(SfdAdxtWork* work)
{
    AdxtHandle* decoder;

    if (SFPLY_GetResetFlg() != 1) {
        decoder = ADXT_Create(work->para.maximum_channels,
                              work->para.decoder_buffer,
                              work->para.decoder_buffer_size);
    } else {
        decoder = SFLIB_libwork.retained_adxt;
    }
    if (decoder == 0) {
        return 0;
    }
    ADXT_SetAutoRcvr(decoder, 0);
    ADXGC_SetAdjsfreqFlg(decoder, 1);
    return decoder;
}

static inline int sfadxt_CreateSub(SfdHandle* handle)
{
    SfdAdxtWork* work;
    AdxtHandle* decoder;
    SJ* stream_joint;
    SfdAudioOutputCallbacks* callbacks;
    int result;
    int output_context;

    work = sfadxt_GetWorkStorage(handle);
    handle->transports[3].context = work;
    result = sfadxt_InitInf(handle, work);
    if (result != 0) {
        return result;
    }
    decoder = sfadxt_CreateAdxt(work);
    if (decoder == 0) {
        return SFLIB_SetErr(0, 0xFF000C04);
    }
    stream_joint = SJRBF_Create(work->para.stream_buffer,
                                work->para.stream_buffer_size,
                                work->para.stream_buffer_extra_size);
    if (stream_joint == 0) {
        return SFLIB_SetErr(0, 0xFF000C05);
    }
    work->decoder = decoder;
    work->stream_joint = stream_joint;
    callbacks = &handle->audio_output_callbacks;
    handle->transports[7].context = callbacks;
    output_context = decoder->field_0C;
    callbacks->reserved_00 = output_context;
    callbacks->set_pan = SFADXT_SetOutPan;
    callbacks->get_pan = SFADXT_GetOutPan;
    callbacks->set_volume = SFADXT_SetOutVol;
    callbacks->get_volume = SFADXT_GetOutVol;
    callbacks->set_speed = SFADXT_SetSpeed;
    ADXT_StartSj(decoder, stream_joint, SFADXT_GetOutVol,
                 SFADXT_SetOutVol, SFADXT_GetOutPan, SFADXT_SetOutPan,
                 output_context, callbacks);
    sfadxt_PauseOn(handle);
    SFTIM_SetTimeFn(handle, sfadxt_GetTime, 2);
    SFSET_SetCond(handle, 0x0F, 2);
    return 0;
}

/* TODO: [near miss] 91.34545%; shared callback context matches retail load count; work/decoder coloring and callback scheduling remain. */
static int SFADXT_Create(SfdHandle* handle)
{
    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    return sfadxt_CreateSub(handle);
}

/* TODO: [near miss] 99.27631%; audio-info status and seek guards match; stop at parameter/base register coloring. */
static void sfadxt_ExcludeSilence(SfdHandle* handle,
                                  const unsigned char* data, int size,
                                  int* consumed)
{
    SfdAdxtWork* work;
    int available;
    int excluded;
    int channels;
    int sample_rate;

    *consumed = 0;
    work = sfadxt_GetWork(handle);
    if (SFHDS_GetMuxVerNum(handle) >= 108) {
        work->copy = sfadxt_ExcludeHdr;
        return;
    }
    available = size - 0x12;
    excluded = 0;
    while (excluded < available) {
        if (memcmp(data, sfadxt_silence, sizeof(sfadxt_silence)) != 0) {
            work->copy = sfadxt_ExcludeHdr;
            break;
        }
        data += sizeof(sfadxt_silence);
        excluded += sizeof(sfadxt_silence);
    }
    *consumed = excluded;
    handle->playback_runtime.time_values[8] += excluded;
    if (sfadxt_GetAudioInf(handle, &channels, &sample_rate) == 0) {
        work->sample_offset += (excluded / (channels * 0x12)) * 0x20;
    }
}

static inline int sfadxt_SearchFrmTop(SfdHandle* handle,
                                      const unsigned char* data, int size)
{
    const unsigned char* offset;
    const unsigned char* position;
    const unsigned char* end;
    const unsigned char* latest_end;
    const unsigned char* latest_offset;
    const unsigned char* limit;
    int found_end;

    offset = data;
    end = data + size;
    latest_end = 0;
    latest_offset = 0;
    limit = data + 0x24;
    found_end = 0;
    while (offset < limit) {
        position = offset;
        found_end = 0;
        while (position < end) {
            if ((signed char)*position < 0) {
                int end_size;

                found_end = 1;
                if (ADXT_IsEndcode(position, 0x12, &end_size) != 0 &&
                    (latest_end == 0 || latest_end < position)) {
                    latest_end = position;
                    latest_offset = offset;
                }
                break;
            }
            position += 0x12;
        }
        if (found_end == 0) {
            break;
        }
        offset += 2;
    }
    if (found_end != 0) {
        if (latest_offset == 0) {
            SFLIB_SetErr(handle, 0xFF000C0A);
            offset = data;
        } else {
            offset = latest_offset;
        }
    }
    return offset - data;
}

/* TODO: [near miss] 95.10753%; stop at frame-scan register coloring; retain the defined null guard absent in RE4's pointer comparison. */
static void sfadxt_ExcludeHdr(SfdHandle* handle, const unsigned char* data,
                              int size, int* consumed)
{
    SfdAdxtWork* work;
    int excluded;
    int header_size;

    *consumed = 0;
    work = sfadxt_GetWork(handle);
    if (size < 0x120) {
        return;
    }
    if (ADXT_IsHeader(data, size, &header_size) != 0) {
        excluded = header_size;
    } else if (SFHDS_GetMuxVerNum(handle) >= 108) {
        excluded = 0;
    } else {
        excluded = sfadxt_SearchFrmTop(handle, data, size);
    }
    work->copy = sfadxt_AdjustSync;
    *consumed = excluded;
    handle->playback_runtime.time_values[8] += excluded;
}

static inline int sfadxt_SearchEndcode(const unsigned char* data, int limit,
                                       int* found_end)
{
    const unsigned char* position = data;
    int offset;
    int end_size;

    *found_end = 0;
    for (offset = 0; offset < limit; offset += 0x12) {
        if (ADXT_IsEndcode(position, 0x12, &end_size) != 0) {
            *found_end = 1;
            break;
        }
        position += 0x12;
    }
    return offset;
}

/* TODO: [near miss] 97.5%; all branches, stack slots, and owner reloads agree; stop at parameter/scan register coloring. */
static void sfadxt_AdjustSync(SfdHandle* handle, const unsigned char* data,
                              int size, int* consumed)
{
    SfdTimerState* timer = &handle->timer_state;
    SfdAdxtWork* work;
    int channels;
    int sample_rate;
    int end_size;
    int using_time_unit;
    int excluded;

    *consumed = 0;
    work = sfadxt_GetWork(handle);
    if (sfadxt_GetAudioInf(handle, &channels, &sample_rate) != 0) {
        work->copy = sfadxt_CopyData;
        return;
    }
    {
        int audio_start =
            (int)SFTIM_GetAudioStartSample(timer, sample_rate);

        if (audio_start < 0) {
            return;
        }
        if (SFSET_GetCond(handle, 5) == 0) {
            SFTIM_SetStartTime(timer, audio_start, sample_rate);
            work->copy = sfadxt_CopyData;
            return;
        }
        {
            int video_start = SFTIM_GetVideoStartSample(
                timer, sample_rate, &using_time_unit);

            if (video_start < 0) {
                return;
            }
            SFTIM_SetStartTime(timer, video_start, sample_rate);
            excluded = 0;
            {
                int difference =
                    (video_start - audio_start) - work->sample_offset;

                if (difference >= 0) {
                    int found_end = 0;
                    int bytes_remaining =
                        (difference / 0x20) * channels * 0x12;

                    if (bytes_remaining > 0) {
                        int bytes_per_frame = channels * 0x12;
                        int block_size = bytes_remaining;
                        int available =
                            channels * (size / bytes_per_frame) * 0x12;
                        int scan_bytes;

                        if (available < block_size) {
                            block_size = available;
                        }
                        scan_bytes = sfadxt_SearchEndcode(
                            data, block_size, &found_end);
                        work->sample_offset +=
                            (scan_bytes / bytes_per_frame) * 0x20;
                        excluded = scan_bytes;
                        bytes_remaining -= block_size;
                    }
                    if (bytes_remaining <= 0 && using_time_unit != 0) {
                        work->copy = sfadxt_CopyData;
                        found_end = ADXT_IsEndcode(data, size, &end_size);
                    }
                    if (found_end != 0) {
                        SFSET_SetCond(handle, 6, 0);
                    }
                } else if (using_time_unit != 0) {
                    int samples_remaining = ((-difference) / 0x20) << 5;

                    if (samples_remaining > 0) {
                        int inserted = ADXT_InsertSilence(
                            sfadxt_GetWork(handle)->decoder, channels,
                            samples_remaining);

                        samples_remaining -= inserted;
                        work->sample_offset -= inserted;
                    }
                    if (samples_remaining <= 0) {
                        work->copy = sfadxt_CopyData;
                    }
                }
            }
        }
    }
    *consumed = excluded;
    handle->playback_runtime.time_values[8] += excluded;
}

static void sfadxt_CopyData(SfdHandle* handle, const unsigned char* data,
                            int size, int* consumed)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    SJ* stream_joint = work->stream_joint;
    SJCK chunk;
    unsigned char* destination;
    int copy_size;

    stream_joint->interface->get_chunk(stream_joint, 0,
                                       work->para.stream_buffer_size, &chunk);
    destination = chunk.data;
    copy_size = size < chunk.len ? size : chunk.len;
    copy_size = copy_size < 0x19000 ? copy_size : 0x19000;
    MEM_Copy(destination, data, copy_size);
    if (copy_size == 0) {
        stream_joint->interface->unget_chunk(stream_joint, 0, &chunk);
    } else {
        SJCK remainder;

        SJ_SplitChunk(&chunk, copy_size, &chunk, &remainder);
        stream_joint->interface->put_chunk(stream_joint, 1, &chunk);
        stream_joint->interface->unget_chunk(stream_joint, 0, &remainder);
    }
    work->copied_bytes += copy_size;
    *consumed = copy_size;
}

static inline void sfadxt_UpdateFlowCnt(SfdHandle* handle)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    SJ* input_joint;
    int write_count;
    int read_count;
    long long flow;

    SFBUF_RingGetSj(handle, handle->transports[3].parameter_10,
                    &input_joint);
    SFBUF_GetFlowCnt(input_joint, &write_count, &read_count);
    flow = handle->playback_runtime.time_values[6];
    handle->playback_runtime.time_values[6] =
        SFBUF_UpdateFlowCnt(flow, write_count);
    flow = handle->playback_runtime.time_values[7];
    handle->playback_runtime.time_values[7] =
        SFBUF_UpdateFlowCnt(flow, read_count);
    SFBUF_GetFlowCnt(work->stream_joint, &write_count, &read_count);
    flow = handle->playback_runtime.time_values[9];
    handle->playback_runtime.time_values[9] =
        SFBUF_UpdateFlowCnt(flow, write_count);
    flow = handle->playback_runtime.time_values[10];
    handle->playback_runtime.time_values[10] =
        SFBUF_UpdateFlowCnt(flow, read_count);
}

static inline int sfadxt_Transfer(SfdHandle* handle, int* input_size)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    SfdBufferTransfer transfer;
    const unsigned char* input_data;
    int size;
    int consumed;
    int result;
    int error;

    result = SFBUF_RingGetRead(handle, handle->transports[3].parameter_10,
                               &transfer);
    input_data = transfer.chunks[0].data;
    size = transfer.chunks[0].len;
    if (result != 0) {
        return result;
    }
    *input_size = size;
    work->copy(handle, input_data, size, &consumed);
    result = SFBUF_RingAddRead(handle, handle->transports[3].parameter_10,
                               consumed);
    error = 0;
    if (result != 0) {
        error = result;
    }
    if (error != 0) {
        return error;
    }
    sfadxt_UpdateFlowCnt(handle);
    return error;
}

static inline int sfadxt_IsPlaying(AdxtHandle* decoder)
{
    if (ADXT_GetStat(decoder) == 3) {
        return 1;
    }
    return 0;
}

static inline void sfadxt_PrepOut(SfdHandle* handle)
{
    int output_buffer = handle->transports[3].parameter_14;
    int input_buffer = handle->transports[3].parameter_10;

    if (SFBUF_GetPrepFlg(handle, output_buffer) != 1) {
        if (SFBUF_GetPrepFlg(handle, input_buffer) == 1) {
            if (sfadxt_IsPlaying(sfadxt_GetWork(handle)->decoder)) {
                SFBUF_SetPrepFlg(handle, output_buffer, 1);
            }
        }
    }
}

static inline void sfadxt_CheckStat(SfdHandle* handle, int input_size)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    SfdTestWork* test = sfadxt_GetTestWork(handle);
    AdxtHandle* decoder = work->decoder;
    int status = ADXT_GetStat(decoder);
    int error = ADXT_GetErrCode(decoder);

    if (error != 0) {
        sfadxt_adxterr = error;
    }
    if (SFSET_GetCond(handle, 0x1A) == 0) {
        error = 0;
    }
    switch (error) {
    case 0:
        break;
    case -1:
        SFLIB_SetErr(handle, 0xFF000C08);
        break;
    case -2:
        SFLIB_SetErr(handle, 0xFF000C09);
        break;
    default:
        SFLIB_SetErr(handle, 0xFF000C07);
        break;
    }
    if (status == 4 || status == 5) {
        SFTST_SetAdjFlg(test, 0);
    }
    if (status == 5 || error != 0) {
        SFBUF_SetTermFlg(handle, handle->transports[3].parameter_14, 1);
    }
    if (SFBUF_GetTermFlg(handle, handle->transports[3].parameter_10) == 1 &&
        input_size == 0) {
        ADXT_TermSupply(decoder);
        if (work->copied_bytes == 0) {
            SFBUF_SetTermFlg(handle, handle->transports[3].parameter_14,
                             1);
        }
    }
}

static inline int sfadxt_IsDecoded(AdxtHandle* decoder)
{
    int status = ADXT_GetStat(decoder);

    if (status == 0 || status == 1) {
        return 0;
    }
    return 1;
}

static inline void sfadxt_AnalyAhdr(SfdHandle* handle)
{
    SfdAdxtSeekInfo* seek =
        sfadxt_GetSeekInfo(handle, sfadxt_GetWork(handle));

    if (seek != 0 && seek->ready == 0) {
        AdxtHandle* decoder = sfadxt_GetWork(handle)->decoder;

        if (sfadxt_IsDecoded(decoder)) {
            seek->sample_rate = ADXT_GetSfreq(decoder);
            seek->sample_count = ADXT_GetNumSmpl(decoder);
            seek->channels = ADXT_GetNumChan(decoder);
            seek->byte_rate =
                (seek->sample_rate * seek->channels * 9) / 16;
            seek->field_08 = 1;
            seek->ready = 1;
        }
    }
}

static inline void sfadxt_UpdateSvrFreq(SfdHandle* handle)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;
    int frequency = SFSET_GetCond(handle, 0x1B);

    if (work->server_frequency != frequency) {
        work->server_frequency = frequency;
        ADXT_SetSvrFreq(decoder, frequency);
    }
}

static inline void sfadxt_WriteTotSmpl(SfdHandle* handle)
{
    AdxtHandle* decoder = sfadxt_GetWork(handle)->decoder;

    if (handle->timer_state.sample_window.fields_04[1] ==
        handle->timer_state.sample_window.fields_04[2]) {
        int samples = ADXT_GetNumSmpl(decoder);
        int sample_rate = ADXT_GetSfreq(decoder);

        if (samples > 0 && sample_rate > 0) {
            SFCON_WriteTotSmplQue(handle, samples, sample_rate);
        }
    }
}

/* TODO: [near miss] 97.78409%; server phases and input-size lifetime match; stop at transfer/owner register coloring. */
static int sfadxt_ExecServerSub(SfdHandle* handle)
{
    int result;
    int input_size;

    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    if (SFBUF_GetTermFlg(handle, handle->transports[3].parameter_14) == 1) {
        return 0;
    }
    input_size = 0;
    result = sfadxt_Transfer(handle, &input_size);
    sfadxt_PrepOut(handle);
    sfadxt_CheckStat(handle, input_size);
    sfadxt_AnalyAhdr(handle);
    sfadxt_UpdateSvrFreq(handle);
    sfadxt_WriteTotSmpl(handle);
    return result;
}

static int SFADXT_ExecServer(SfdHandle* handle)
{
    return sfadxt_ExecServerSub(handle);
}

static int SFADXT_Finish(SfdHandle* handle)
{
    SFA_Finish();
    ADXT_Finish();
    return 0;
}

static int SFADXT_Init(SfdHandle* handle)
{
    ADXT_Init();
    SFA_Init();
    UTY_MemsetDword((unsigned int*)&sfadxt_para, 0,
                    sizeof(sfadxt_para) / sizeof(unsigned int));
    return 0;
}

void SFD_SetAdxtPara(SfdAdxtParameters* parameters)
{
    sfadxt_para.stream_buffer_size = parameters->stream_buffer_size;
    sfadxt_para.stream_buffer_extra_size =
        parameters->stream_buffer_extra_size;
    sfadxt_para.stream_buffer =
        (void*)(((unsigned int)parameters->stream_buffer + 0x1F) & ~0x1F);
    sfadxt_para.maximum_channels = parameters->maximum_channels;
    sfadxt_para.decoder_work_size = parameters->decoder_work_size;
    sfadxt_para.decoder_buffer_size = parameters->decoder_buffer_size;
    sfadxt_para.decoder_buffer =
        (void*)(((unsigned int)parameters->decoder_buffer + 0x1F) & ~0x1F);
}

static void SFADXT_SetSpeed(SfdHandle* handle, int speed)
{
    AdxtHandle* decoder = sfadxt_GetWork(handle)->decoder;

    if (decoder != 0) {
        int semitones;
        int cents;

        if (speed == 1000) {
            semitones = 0;
            cents = 0;
        } else {
            float log_speed = (float)log((float)speed);
            float transpose = 1731.234f * (log_speed - 6.9077554f);
            semitones = (int)(0.01f * transpose);
            cents = (int)transpose - semitones * 100;
        }
        ADXT_SetTranspose(decoder, semitones, cents);
    }
}

static int SFADXT_GetOutVol(SfdHandle* handle,
                            SfdAudioOutputCallbacks* callbacks)
{
    return ADXT_GetOutVol(sfadxt_GetWork(handle)->decoder);
}

static int SFADXT_SetOutVol(SfdHandle* handle, int volume,
                            SfdAudioOutputCallbacks* callbacks)
{
    return ADXT_SetOutVol(sfadxt_GetWork(handle)->decoder, volume);
}

static int SFADXT_GetOutPan(SfdHandle* handle, int channel,
                            SfdAudioOutputCallbacks* callbacks)
{
    return ADXT_GetOutPan(sfadxt_GetWork(handle)->decoder, channel);
}

static int SFADXT_SetOutPan(SfdHandle* handle, int channel, int pan,
                            SfdAudioOutputCallbacks* callbacks)
{
    return ADXT_SetOutPan(sfadxt_GetWork(handle)->decoder, channel, pan);
}
