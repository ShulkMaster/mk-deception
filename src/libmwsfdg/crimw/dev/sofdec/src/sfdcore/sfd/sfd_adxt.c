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
typedef struct SfdTestWork SfdTestWork;

typedef struct SfdTestTime {
    long long value;
    long long scale;
} SfdTestTime;

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
    int stream_buffer_size;
    int stream_buffer_extra_size;
    void* stream_buffer;
    int maximum_channels;
    int decoder_work_size;
    int decoder_buffer_size;
    void* decoder_buffer;
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
extern void ADXT_SetOutVol(AdxtHandle* decoder, int volume);
extern int ADXT_GetOutPan(AdxtHandle* decoder, int channel);
extern void ADXT_SetOutPan(AdxtHandle* decoder, int channel, int pan);

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
int gap_06_80499B7C_bss;

static inline SfdAdxtWork* sfadxt_GetWork(SfdHandle* handle)
{
    return handle->transports[3].context;
}

static inline SfdTestWork* sfadxt_GetTestWork(SfdHandle* handle)
{
    return (SfdTestWork*)(handle->timer_state.unknown_02EC + 4);
}

static inline SfdAdxtWork* sfadxt_GetWorkStorage(SfdHandle* handle)
{
    return (SfdAdxtWork*)(handle->unknown_3320 + 0xA0);
}

static inline SfdAdxtSeekInfo* sfadxt_GetSeekInfo(SfdHandle* handle,
                                                  SfdAdxtWork* work)
{
    SfdHandle* source = handle->seek_state.source_handle;

    if (source == 0 || work->seek_depth > 0) {
        return 0;
    }
    return (SfdAdxtSeekInfo*)&source->conditions_secondary[94];
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
static void SFADXT_SetOutVol(SfdHandle* handle, int volume,
                             SfdAudioOutputCallbacks* callbacks);
static int SFADXT_GetOutPan(SfdHandle* handle, int channel,
                            SfdAudioOutputCallbacks* callbacks);
static void SFADXT_SetOutPan(SfdHandle* handle, int channel, int pan,
                             SfdAudioOutputCallbacks* callbacks);
static int SFADXT_Seek(SfdHandle* handle, int parameter, int value);
static int SFADXT_AddRead(SfdHandle* handle, int parameter, int value);
static int SFADXT_GetRead(SfdHandle* handle, void* output);
static int SFADXT_AddWrite(SfdHandle* handle, int parameter, int value);
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

static int SFADXT_Seek(SfdHandle* handle, int parameter, int value)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    SfdHandle* source = handle->seek_state.source_handle;
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

static int SFADXT_AddRead(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static int SFADXT_GetRead(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static int SFADXT_AddWrite(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static int SFADXT_GetWrite(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000C03);
}

static int SFADXT_Pause(SfdHandle* handle, int state)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;

    switch (state) {
    case 0:
        work->discarded_samples = 0;
        if (work->paused != 1) {
            ADXT_Pause(work->decoder, 0);
            SFTST_Pause(sfadxt_GetTestWork(handle), 0);
        }
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

static int SFADXT_Start(SfdHandle* handle)
{
    SfdTestWork* test_work = sfadxt_GetTestWork(handle);
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;

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

static int SFADXT_Destroy(SfdHandle* handle)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;
    SJ* stream_joint = work->stream_joint;

    if (decoder == 0) {
        return 0;
    }
    sfadxt_para.stream_buffer_size = work->stream_buffer_size;
    sfadxt_para.stream_buffer_extra_size = work->stream_buffer_extra_size;
    sfadxt_para.stream_buffer = work->stream_buffer;
    sfadxt_para.maximum_channels = work->maximum_channels;
    sfadxt_para.decoder_work_size = work->decoder_work_size;
    sfadxt_para.decoder_buffer_size = work->decoder_buffer_size;
    sfadxt_para.decoder_buffer = work->decoder_buffer;
    if (SFPLY_GetResetFlg() != 1) {
        ADXT_Destroy(decoder);
    } else {
        ADXT_Stop(sfadxt_GetWork(handle)->decoder);
        SFLIB_libwork.retained_adxt = decoder;
    }
    stream_joint->interface->destroy(stream_joint);
    UTY_FinishTmr();
    return 0;
}

static int sfadxt_GetTime(SfdHandle* handle, int* value, int* scale)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    AdxtHandle* decoder = work->decoder;

    if (SFTIM_ChkRegularTime(handle, value, scale) == 0) {
        return 0;
    }
    if (handle->playback_state == 4) {
        int decoder_value;
        int decoder_scale;
        SfdTestTime master;
        SfdTestTime sample;
        SfdTestTime output;

        sfadxt_stat = ADXT_GetStat(decoder);
        ADXT_GetTime(decoder, &decoder_value, &decoder_scale, &sfadxt_stat);
        master.value = decoder_value;
        master.scale = decoder_scale;
        sample.value = UTY_GetTmr();
        sample.scale = UTY_GetTmrUnit();
        SFTST_Calc(sfadxt_GetTestWork(handle), &master, &sample, &output);
        if (work->maximum_time_value < (int)output.value) {
            work->maximum_time_value = (int)output.value;
            work->maximum_time_scale = (int)output.scale;
        }
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
    work->stream_buffer_size = sfadxt_para.stream_buffer_size;
    work->stream_buffer_extra_size = sfadxt_para.stream_buffer_extra_size;
    work->stream_buffer = sfadxt_para.stream_buffer;
    work->maximum_channels = sfadxt_para.maximum_channels;
    work->decoder_work_size = sfadxt_para.decoder_work_size;
    work->decoder_buffer_size = sfadxt_para.decoder_buffer_size;
    work->decoder_buffer = sfadxt_para.decoder_buffer;
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

static int SFADXT_Create(SfdHandle* handle)
{
    SfdAdxtWork* work;
    AdxtHandle* decoder;
    SJ* stream_joint;
    SfdAudioOutputCallbacks* callbacks;
    int result;

    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    work = sfadxt_GetWorkStorage(handle);
    handle->transports[3].context = work;
    result = sfadxt_InitInf(handle, work);
    if (result != 0) {
        return result;
    }
    if (SFPLY_GetResetFlg() != 1) {
        decoder = ADXT_Create(work->maximum_channels, work->decoder_buffer,
                              work->decoder_buffer_size);
    } else {
        decoder = SFLIB_libwork.retained_adxt;
    }
    if (decoder != 0) {
        ADXT_SetAutoRcvr(decoder, 0);
        ADXGC_SetAdjsfreqFlg(decoder, 1);
    }
    if (decoder == 0) {
        return SFLIB_SetErr(0, 0xFF000C04);
    }
    stream_joint = SJRBF_Create(work->stream_buffer,
                                work->stream_buffer_size,
                                work->stream_buffer_extra_size);
    if (stream_joint == 0) {
        return SFLIB_SetErr(0, 0xFF000C05);
    }
    work->decoder = decoder;
    work->stream_joint = stream_joint;
    callbacks = &handle->audio_output_callbacks;
    handle->transports[7].context = callbacks;
    callbacks->reserved_00 = decoder->field_0C;
    callbacks->set_pan = SFADXT_SetOutPan;
    callbacks->get_pan = SFADXT_GetOutPan;
    callbacks->set_volume = SFADXT_SetOutVol;
    callbacks->get_volume = SFADXT_GetOutVol;
    callbacks->set_speed = SFADXT_SetSpeed;
    ADXT_StartSj(decoder, stream_joint, SFADXT_GetOutVol,
                 SFADXT_SetOutVol, SFADXT_GetOutPan, SFADXT_SetOutPan,
                 decoder->field_0C, callbacks);
    work->paused = 1;
    ADXT_Pause(work->decoder, 1);
    SFTST_Pause(sfadxt_GetTestWork(handle), 1);
    SFTIM_SetTimeFn(handle, sfadxt_GetTime, 2);
    SFSET_SetCond(handle, 0x0F, 2);
    return 0;
}

static void sfadxt_ExcludeSilence(SfdHandle* handle,
                                  const unsigned char* data, int size,
                                  int* consumed)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    int available;
    int excluded;

    *consumed = 0;
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
    {
        SfdAdxtSeekInfo* seek = sfadxt_GetSeekInfo(handle, work);

        if (seek != 0) {
            work->sample_offset +=
                (excluded / (seek->channels * 0x12)) * 0x20;
        }
    }
}

static void sfadxt_ExcludeHdr(SfdHandle* handle, const unsigned char* data,
                              int size, int* consumed)
{
    SfdAdxtWork* work = sfadxt_GetWork(handle);

    *consumed = 0;
    if (size >= 0x120) {
        int excluded;

        if (ADXT_IsHeader(data, size, &excluded) != 0) {
            /* The decoder reports the complete header length. */
        } else if (SFHDS_GetMuxVerNum(handle) >= 108) {
            excluded = 0;
        } else {
            const unsigned char* offset = data;
            const unsigned char* end = data + size;
            const unsigned char* limit = data + 0x24;
            const unsigned char* latest_end = 0;
            const unsigned char* latest_offset = 0;
            int found_end = 0;

            while (offset < limit) {
                const unsigned char* position = offset;

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
            excluded = offset - data;
        }
        work->copy = sfadxt_AdjustSync;
        *consumed = excluded;
        handle->playback_runtime.time_values[8] += excluded;
    }
}

static void sfadxt_AdjustSync(SfdHandle* handle, const unsigned char* data,
                              int size, int* consumed)
{
    SfdTimerState* timer = &handle->timer_state;
    SfdAdxtWork* work = sfadxt_GetWork(handle);
    SfdAdxtSeekInfo* seek = sfadxt_GetSeekInfo(handle, work);
    int excluded = 0;

    *consumed = 0;
    if (seek == 0) {
        work->copy = sfadxt_CopyData;
        return;
    }
    {
        int channels = seek->channels;
        int sample_rate = seek->sample_rate;
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
            int using_time_unit;
            int video_start = SFTIM_GetVideoStartSample(
                timer, sample_rate, &using_time_unit);

            if (video_start < 0) {
                return;
            }
            SFTIM_SetStartTime(timer, video_start, sample_rate);
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
                        const unsigned char* position = data;

                        if (available < block_size) {
                            block_size = available;
                        }
                        excluded = 0;
                        while (excluded < block_size) {
                            int end_size;

                            if (ADXT_IsEndcode(position, 0x12,
                                               &end_size) != 0) {
                                found_end = 1;
                                break;
                            }
                            position += 0x12;
                            excluded += 0x12;
                        }
                        bytes_remaining -= block_size;
                        work->sample_offset +=
                            (excluded / bytes_per_frame) * 0x20;
                    }
                    if (bytes_remaining <= 0 && using_time_unit != 0) {
                        int end_size;

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
                            work->decoder, channels, samples_remaining);

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
    int copy_size;

    stream_joint->interface->get_chunk(stream_joint, 0,
                                       work->stream_buffer_size, &chunk);
    copy_size = size < chunk.len ? size : chunk.len;
    if (copy_size > 0x19000) {
        copy_size = 0x19000;
    }
    MEM_Copy(chunk.data, data, copy_size);
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

static int sfadxt_ExecServerSub(SfdHandle* handle)
{
    SfdAdxtWork* work;
    int result;
    int input_size = 0;

    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    if (SFBUF_GetTermFlg(handle, handle->transports[3].parameter_14) == 1) {
        return 0;
    }
    work = sfadxt_GetWork(handle);
    {
        SJCK chunk;

        result = SFBUF_RingGetRead(handle,
                                   handle->transports[3].parameter_10,
                                   &chunk);
        if (result == 0) {
            int consumed;

            input_size = chunk.len;
            work->copy(handle, chunk.data, chunk.len, &consumed);
            result = SFBUF_RingAddRead(handle,
                                       handle->transports[3].parameter_10,
                                       consumed);
            if (result == 0) {
                SJ* input_joint;
                int write_count;
                int read_count;
                long long flow;

                SFBUF_RingGetSj(handle,
                                handle->transports[3].parameter_10,
                                &input_joint);
                SFBUF_GetFlowCnt(input_joint, &write_count, &read_count);
                flow = handle->playback_runtime.time_values[6];
                handle->playback_runtime.time_values[6] =
                    SFBUF_UpdateFlowCnt((int)(flow >> 32),
                                        (unsigned int)flow, write_count);
                flow = handle->playback_runtime.time_values[7];
                handle->playback_runtime.time_values[7] =
                    SFBUF_UpdateFlowCnt((int)(flow >> 32),
                                        (unsigned int)flow, read_count);
                SFBUF_GetFlowCnt(work->stream_joint, &write_count,
                                 &read_count);
                flow = handle->playback_runtime.time_values[9];
                handle->playback_runtime.time_values[9] =
                    SFBUF_UpdateFlowCnt((int)(flow >> 32),
                                        (unsigned int)flow, write_count);
                flow = handle->playback_runtime.time_values[10];
                handle->playback_runtime.time_values[10] =
                    SFBUF_UpdateFlowCnt((int)(flow >> 32),
                                        (unsigned int)flow, read_count);
            }
        }
    }
    {
        int output_buffer = handle->transports[3].parameter_14;
        int input_buffer = handle->transports[3].parameter_10;

        if (SFBUF_GetPrepFlg(handle, output_buffer) != 1 &&
            SFBUF_GetPrepFlg(handle, input_buffer) == 1 &&
            ADXT_GetStat(sfadxt_GetWork(handle)->decoder) == 3) {
            SFBUF_SetPrepFlg(handle, output_buffer, 1);
        }
    }
    {
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
            SFTST_SetAdjFlg(sfadxt_GetTestWork(handle), 0);
        }
        if (status == 5 || error != 0) {
            SFBUF_SetTermFlg(handle,
                             handle->transports[3].parameter_14, 1);
        }
        if (SFBUF_GetTermFlg(handle,
                             handle->transports[3].parameter_10) == 1 &&
            input_size == 0) {
            ADXT_TermSupply(decoder);
            if (work->copied_bytes == 0) {
                SFBUF_SetTermFlg(handle,
                                 handle->transports[3].parameter_14, 1);
            }
        }
    }
    {
        SfdAdxtSeekInfo* seek = sfadxt_GetSeekInfo(handle, work);

        if (seek != 0 && seek->ready == 0) {
            AdxtHandle* decoder = work->decoder;
            int status = ADXT_GetStat(decoder);

            if (status != 0 && status != 1) {
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
    {
        AdxtHandle* decoder = work->decoder;
        int frequency = SFSET_GetCond(handle, 0x1B);

        if (work->server_frequency != frequency) {
            work->server_frequency = frequency;
            ADXT_SetSvrFreq(decoder, frequency);
        }
    }
    if (handle->timer_state.sample_window.fields_04[1] ==
        handle->timer_state.sample_window.fields_04[2]) {
        AdxtHandle* decoder = work->decoder;
        int samples = ADXT_GetNumSmpl(decoder);
        int sample_rate = ADXT_GetSfreq(decoder);

        if (samples > 0 && sample_rate > 0) {
            SFCON_WriteTotSmplQue(handle, samples, sample_rate);
        }
    }
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
    UTY_MemsetDword((unsigned int*)&sfadxt_para, 0, 7);
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
            float transpose =
                1731.234f * ((float)log((double)speed) - 6.9077554f);
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

static void SFADXT_SetOutVol(SfdHandle* handle, int volume,
                             SfdAudioOutputCallbacks* callbacks)
{
    ADXT_SetOutVol(sfadxt_GetWork(handle)->decoder, volume);
}

static int SFADXT_GetOutPan(SfdHandle* handle, int channel,
                            SfdAudioOutputCallbacks* callbacks)
{
    return ADXT_GetOutPan(sfadxt_GetWork(handle)->decoder, channel);
}

static void SFADXT_SetOutPan(SfdHandle* handle, int channel, int pan,
                             SfdAudioOutputCallbacks* callbacks)
{
    ADXT_SetOutPan(sfadxt_GetWork(handle)->decoder, channel, pan);
}
