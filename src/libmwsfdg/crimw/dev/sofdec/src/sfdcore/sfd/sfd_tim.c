#include "sofdec/sfd_player.h"
#include "sofdec/uty_math.h"

const int SFTIM_prate[9] = {
    1, 24000, 24000, 25000, 29970, 30000, 50000, 59940, 60000,
};

int sftim_v_sample = 0;
int sftim_v_time = 0;
unsigned int sftim_a_sample = 0;
long long sftim_as_pts = 0;

int SFTIM_GetSpeed(SfdHandle* handle)
{
    return handle->timer_state.speed;
}

void SFTIM_SetSpeed(SfdHandle* handle, int speed)
{
    handle->timer_state.speed = speed;
}

int SFTIM_IsVideoTerm(SfdHandle* handle)
{
    int end_value = handle->timer_state.video_end_time_value;
    int end_scale = handle->timer_state.video_end_time_scale;

    if (handle->playback_runtime.decoded_pictures == 0) {
        return 1;
    }
    if (end_value == -5) {
        return 0;
    }
    end_value += (end_scale * 2000) / 59940;
    return UTY_CmpTime(end_value, end_scale,
                       handle->timer_state.current_time_value,
                       handle->timer_state.current_time_scale) != 0;
}

/* TODO: [breakthrough needed] 78.542250%; typed timer and adjusted-clock locals
 * preserve codegen; early library-base/clock-load scheduling remains unresolved. */
int SFTIM_IsGetFrmTimeTunit(SfdHandle* handle, int value, int scale)
{
    int ready;
    int special_timing;
    int adjusted_clock;
    SfdTimerState* timer;
    float current_time;
    float target_time;
    float tolerance;

    if (handle->conditions_primary[14] != 0) {
        return 1;
    }
    timer = &handle->timer_state;
    if (timer->current_time_scale == 1) {
        if (timer->current_time_value == -2) {
            ready = 1;
        } else if (timer->field_02CC < 0) {
            timer->field_02CC = 0;
            ready = 1;
        } else if (UTY_CmpTime(value, scale,
                               timer->field_02CC,
                               SFLIB_libwork.timer_work.source) != 0) {
            ready = 1;
        } else {
            ready = 0;
        }
    } else {
        adjusted_clock = timer->current_time_value +
                         (timer->current_time_scale *
                          handle->conditions_primary[44]) /
                             SFLIB_libwork.timer_work.source;
        target_time = (10000.0f * (float)value) / (float)scale;
        current_time =
            (10000.0f * (float)adjusted_clock) /
            (float)timer->current_time_scale;
        if (handle->conditions_primary[15] != 1) {
            tolerance = (float)handle->conditions_primary[46];
            if (current_time + tolerance < target_time) {
                ready = 0;
            } else if (current_time - tolerance >= target_time) {
                ready = 1;
                if (timer->last_frame_time != target_time &&
                    timer->previous_frame_time != target_time) {
                    timer->previous_frame_time = target_time;
                    timer->frame_time_repeat_count++;
                }
            } else {
                special_timing = 0;
                if (SFLIB_libwork.timer_work.source == 59940 &&
                    handle->playback_settings.frame_rate_code <= 2 &&
                    timer->speed == 1000) {
                    special_timing = 1;
                }
                if (timer->frame_time_repeat_count <=
                    (special_timing != 0)) {
                    ready = timer->previous_frame_ready;
                } else if (current_time < target_time) {
                    ready = 0;
                } else {
                    ready = 1;
                }
                timer->frame_time_repeat_count = 0;
                timer->previous_frame_ready = ready;
                timer->last_frame_time = target_time;
            }
        } else if (target_time <= current_time) {
            ready = 1;
        } else {
            ready = 0;
        }
    }
    return ready;
}

/* TODO: [breakthrough needed] 84.917810%; typed clock/lib owners and donor-order
 * adjustment load help; floating-point scheduling needs fresh evidence. */
int SFTIM_IsGetFrmTime(SfdHandle* handle, const SfdFrameTime* frame_time)
{
    int ready;
    int special_timing;
    int value;
    int scale;
    int clock_value;
    int clock_scale;
    int adjustment;
    SfdTimerState* timer;
    SfdTimerLibraryWork* timer_library;
    float current_time;
    float target_time;
    float tolerance;

    if (frame_time == 0) {
        return 0;
    }
    scale = frame_time->scale;
    value = frame_time->value;
    if (handle->conditions_primary[14] != 0) {
        ready = 1;
    } else {
        timer = &handle->timer_state;
        timer_library = &SFLIB_libwork.timer_work;
        clock_scale = timer->current_time_scale;
        adjustment = handle->conditions_primary[44];
        clock_value = timer->current_time_value;
        if (clock_scale == 1) {
            if (clock_value == -2) {
                ready = 1;
            } else if (timer->field_02CC < 0) {
                timer->field_02CC = 0;
                ready = 1;
            } else if (UTY_CmpTime(value, scale, timer->field_02CC,
                                   timer_library->source) != 0) {
                ready = 1;
            } else {
                ready = 0;
            }
        } else {
            target_time = (10000.0f * (float)value) / (float)scale;
            current_time =
                (10000.0f *
                 (float)(clock_value +
                         (clock_scale * adjustment) /
                             timer_library->source)) /
                (float)clock_scale;
            if (handle->conditions_primary[15] != 1) {
                tolerance = (float)handle->conditions_primary[46];
                if (current_time + tolerance < target_time) {
                    ready = 0;
                } else if (current_time - tolerance >= target_time) {
                    ready = 1;
                    if (timer->last_frame_time != target_time &&
                        timer->previous_frame_time != target_time) {
                        timer->previous_frame_time = target_time;
                        timer->frame_time_repeat_count++;
                    }
                } else {
                    special_timing = 0;
                    if (timer_library->source == 59940 &&
                        handle->playback_settings.frame_rate_code <= 2 &&
                        timer->speed == 1000) {
                        special_timing = 1;
                    }
                    if (timer->frame_time_repeat_count <=
                        (special_timing != 0)) {
                        ready = timer->previous_frame_ready;
                    } else if (current_time < target_time) {
                        ready = 0;
                    } else {
                        ready = 1;
                    }
                    timer->frame_time_repeat_count = 0;
                    timer->previous_frame_ready = ready;
                    timer->last_frame_time = target_time;
                }
            } else if (target_time <= current_time) {
                ready = 1;
            } else {
                ready = 0;
            }
        }
    }
    return ready;
}

int SFD_GetFps(SfdHandle* handle, int* frame_rate)
{
    int code;

    *frame_rate = -1;
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF00011B);
    }
    code = handle->playback_settings.frame_rate_code;
    if (code == 0) {
        return 0;
    }
    *frame_rate = SFTIM_prate[code];
    return 0;
}

void SFTIM_GetTimeOneFrmVideo(SfdHandle* handle, int* value, int* scale)
{
    int code = handle->playback_settings.frame_rate_code;

    if (code == 0) {
        *value = 0;
        *scale = 29970;
    } else {
        *value = 1000;
        *scale = SFTIM_prate[code];
    }
}

void SFTIM_Pause(SfdHandle* handle, int state)
{
    int numerator;
    int denominator;
    int increment;
    int video_token;
    int external_token;

    switch (state) {
    case 2:
        if (handle->playback_settings.frame_rate_code == 0) {
            numerator = 0;
            denominator = 29970;
        } else {
            numerator = 1000;
            denominator = SFTIM_prate[handle->playback_settings.frame_rate_code];
        }
        increment = UTY_MulDiv(SFLIB_libwork.timer_work.source, numerator,
                               denominator);
        SFLIB_LockCs(&video_token);
        handle->timer_state.video_clock_sample += increment;
        handle->timer_state.field_02CC += increment;
        SFLIB_UnlockCs(&video_token);

        increment = UTY_MulDiv(handle->timer_state.clock_sample_scale, numerator,
                               denominator);
        SFLIB_LockCs(&external_token);
        handle->timer_state.current_clock_sample += increment;
        SFLIB_UnlockCs(&external_token);
        break;
    case 1:
    default:
        break;
    }
}

static void sftim_Tc2Time59D(int rate, SfdTimeCode* timecode,
                             int* value, int* scale)
{
    int frames;

    frames = timecode->hours * 215892 + timecode->minutes * 3598 +
             (timecode->minutes / 10) * 2 + timecode->seconds * 60 +
             (timecode->frames + timecode->frame_offset);
    *value = frames * 1000 + timecode->subframe * 500;
    *scale = rate;
}

static void sftim_Tc2Time29D(int rate, SfdTimeCode* timecode,
                             int* value, int* scale)
{
    int frames;

    frames = timecode->hours * 107892 + timecode->minutes * 1798 +
             (timecode->minutes / 10) * 2 + timecode->seconds * 30 +
             (timecode->frames + timecode->frame_offset);
    *value = frames * 1000 + timecode->subframe * 500;
    *scale = rate;
}

static void sftim_Tc2Time23D(int rate, SfdTimeCode* timecode,
                             int* value, int* scale)
{
    int frames;

    frames = timecode->hours * 86292 + timecode->minutes * 1438 +
             (timecode->minutes / 10) * 2 + timecode->seconds * 24 +
             (timecode->frames + timecode->frame_offset);
    *value = frames * 1000 + timecode->subframe * 500;
    *scale = rate;
}

/* Retail writes the nominal scale before the supplied rate in each
 * non-drop converter below. */
static void sftim_Tc2Time59N(int rate, SfdTimeCode* timecode,
                             int* value, int* scale)
{
    int frames;

    frames = timecode->frames + timecode->frame_offset;
    *value = timecode->minutes * 3600000 +
             timecode->hours * 216000000 +
             timecode->seconds * 60000 +
             frames * 1000 +
             timecode->subframe * 500;
    *scale = 60000;
    *scale = rate;
}

static void sftim_Tc2Time29N(int rate, SfdTimeCode* timecode,
                             int* value, int* scale)
{
    int frames;

    frames = timecode->frames + timecode->frame_offset;
    *value = timecode->minutes * 1800000 +
             timecode->hours * 108000000 +
             timecode->seconds * 30000 +
             frames * 1000 +
             timecode->subframe * 500;
    *scale = 30000;
    *scale = rate;
}

static void sftim_Tc2Time23N(int rate, SfdTimeCode* timecode,
                             int* value, int* scale)
{
    int frames;

    frames = timecode->frames + timecode->frame_offset;
    *value = timecode->minutes * 1440000 +
             timecode->hours * 86400000 +
             timecode->seconds * 24000 +
             frames * 1000 +
             timecode->subframe * 500;
    *scale = 24000;
    *scale = rate;
}

static void sftim_Tc2TimeN(int rate, SfdTimeCode* timecode,
                           int* value, int* scale)
{
    int seconds;
    int frames;

    seconds = timecode->minutes * 60 + timecode->hours * 3600 +
              timecode->seconds;
    frames = timecode->frames + timecode->frame_offset;
    *value = frames * 1000 + seconds * rate + timecode->subframe * 500;
    *scale = rate;
}

static const SfdTimeCodeConvertFn sftim_tc2time[9][2] = {
    {0, 0},
    {sftim_Tc2Time23N, sftim_Tc2Time23D},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2Time29N, sftim_Tc2Time29D},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
    {sftim_Tc2Time59N, sftim_Tc2Time59D},
    {sftim_Tc2TimeN, sftim_Tc2TimeN},
};

void SFTIM_Tc2Time(SfdTimeCode* timecode, int* value, int* scale)
{
    int frame_rate_code = timecode->frame_rate_code;
    int rate = SFTIM_prate[frame_rate_code];
    SfdTimeCodeConvertFn convert =
        sftim_tc2time[frame_rate_code][timecode->drop_frame];

    if (convert == 0) {
        SFLIB_SetErr(0, 0xFF000221);
        *value = 0;
        *scale = 1;
        return;
    }
    convert(rate, timecode, value, scale);
}

void SFTIM_SetTimeFn(SfdHandle* handle, SfdTimeSourceFn callback, int index)
{
    handle->timer_state.time_sources[index] = callback;
}

int SFD_SetExtClockFn(SfdHandle* handle, SfdExternalClockFn callback, int arg0,
                      SfdCallbackObject arg1)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000129);
    }
    if (callback != 0) {
        handle->timer_state.external_clock_callback = callback;
        handle->timer_state.external_clock_wrap = arg0;
        handle->timer_state.external_clock_object = arg1;
        SFSET_SetCond(handle, 0x0F, 5);
        SFSET_SetCond(handle, 0x47, 0);
    } else {
        SFSET_SetCond(handle, 0x47, 1);
        SFSET_SetCond(handle, 0x0F, 1);
        handle->timer_state.external_clock_object = arg1;
        handle->timer_state.external_clock_wrap = arg0;
        handle->timer_state.external_clock_callback = 0;
    }
    return 0;
}

int SFD_SetUsrTimeFn(SfdHandle* handle, SfdTimeSourceFn callback)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000123);
    }
    handle->timer_state.time_sources[4] = callback;
    if (callback != 0) {
        SFSET_SetCond(handle, 0x0F, 4);
    }
    return 0;
}

int SFD_CmpTime(int lhs_value, int lhs_scale, int rhs_value, int rhs_scale)
{
    return UTY_CmpTime(lhs_value, lhs_scale, rhs_value, rhs_scale);
}

int SFD_SetUsrIsSkipFn(SfdHandle* handle, SfdUserIsSkipFn callback)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000124);
    }
    handle->timer_state.skip_state.callback = callback;
    return 0;
}

int SFTIM_ChkRegularTime(SfdHandle* handle, int* value, int* scale)
{
    int state = handle->playback_state;

    if (state != 4 && state != -4 && state != 6 && state != -6) {
        *value = -1;
        *scale = 1;
        return 0;
    }
    return 1;
}

/* TODO: [near miss] 98.640450%; donor wrap increment order matches retail;
 * previous-sample register coloring/reload remains. */
static int sftim_GetTimeExtClock(SfdHandle* handle, int* value, int* scale)
{
    int sample;
    int sample_scale;
    int result;
    int update;
    int delta;

    if (SFTIM_ChkRegularTime(handle, value, scale) == 0) {
        return 0;
    }
    if (handle->timer_state.external_clock_callback == 0) {
        *value = -2;
        *scale = 1;
        return 0;
    }
    result = handle->timer_state.external_clock_callback(
        handle->timer_state.external_clock_object, &sample, &sample_scale);
    if (handle->playback_state != 4) {
        update = 0;
    } else if (handle->field_0050 != 0) {
        update = 0;
    } else if (handle->playback_runtime.field_1C != 0) {
        update = 0;
    } else {
        update = 1;
    }
    if (update != 0 && handle->timer_state.previous_external_sample != -5) {
        delta = sample - handle->timer_state.previous_external_sample;
        if (delta < 0) {
            delta = handle->timer_state.external_clock_wrap + delta;
            delta++;
        }
        handle->timer_state.current_clock_sample += delta;
    }
    handle->timer_state.previous_external_sample = sample;
    handle->timer_state.clock_sample_scale = sample_scale;
    *value = handle->timer_state.current_clock_sample;
    *scale = handle->timer_state.clock_sample_scale;
    return result;
}

static int sftim_GetTimeUfrm(SfdHandle* handle, int* value, int* scale)
{
    if (SFTIM_ChkRegularTime(handle, value, scale) == 0) {
        return 0;
    }
    return 0;
}

static int sftim_GetTimeVsync(SfdHandle* handle, int* value, int* scale)
{
    if (SFTIM_ChkRegularTime(handle, value, scale) == 0) {
        return 0;
    }
    *value = handle->timer_state.video_clock_sample;
    *scale = SFLIB_libwork.timer_work.source;
    return 0;
}

static int sftim_GetTimeNone(SfdHandle* handle, int* value, int* scale)
{
    if (SFTIM_ChkRegularTime(handle, value, scale) == 0) {
        return 0;
    }
    *value = -2;
    *scale = 1;
    return 0;
}

int SFTIM_GetTime(SfdHandle* handle, int* value, int* scale)
{
    *value = handle->timer_state.current_time_value;
    *scale = handle->timer_state.current_time_scale;
    return 0;
}

int SFTIM_GetTimeSub(SfdHandle* handle, int* value, int* scale)
{
    SfdTimerState* timer = &handle->timer_state;
    SfdTimerTimeUnit* elapsed = &timer->elapsed_time;

    *value = timer->current_time_value;
    *scale = timer->current_time_scale;
    if (*scale == 1) {
        return 0;
    }
    if (*scale == timer->start_time_scale) {
        *value += timer->start_time_value;
    } else if (elapsed->active != 0) {
        *value += UTY_MulDiv(elapsed->value, *scale, elapsed->scale);
    }
    return 0;
}

int SFD_GetTime(SfdHandle* handle, int* value, int* scale)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000121);
    }
    return SFTIM_GetTimeSub(handle, value, scale);
}

void SFTIM_SetStartTime(SfdTimerState* state, int value, int scale)
{
    state->start_time_value = value;
    state->start_time_scale = scale;
}

int SFTIM_GetVideoStartSample(SfdTimerState* state, int sample_rate,
                              int* using_time_unit)
{
    int value;
    int scale;
    int sample;

    *using_time_unit = state->elapsed_time.active;
    if (*using_time_unit != 0) {
        value = state->elapsed_time.value;
        scale = state->elapsed_time.scale;
    } else if (state->video_start_time.value >= 0) {
        value = state->video_start_time.value;
        scale = state->video_start_time.scale;
    } else {
        return -1;
    }
    sample = UTY_MulDiv(value, sample_rate, scale);
    sftim_v_time = value;
    sftim_v_sample = sample;
    return sample;
}

unsigned int SFTIM_GetAudioStartSample(SfdTimerState* state, int sample_rate)
{
    unsigned int sample;

    if (state->audio_start_pts < 0) {
        return -1;
    }
    sample = (state->audio_start_pts * sample_rate) / 90000;
    sftim_as_pts = state->audio_start_pts;
    sftim_a_sample = sample;
    return sample;
}

static int sftim_CheckStagnant(SfdHandle* handle)
{
    SfdTimerState* timer;
    SfdTimerLibraryWork* library;
    int threshold;
    int base;
    int elapsed;
    int scale;

    timer = &handle->timer_state;
    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    threshold = SFSET_GetCond(handle, 0x33);
    if (threshold == 0) {
        return 0;
    }
    library = &SFLIB_libwork.timer_work;
    if (SFSET_GetCond(handle, 0x47) == 1) {
        scale = library->source;
        elapsed = timer->video_clock_sample - timer->previous_clock_sample;
    } else {
        elapsed = timer->current_clock_sample;
        base = timer->previous_clock_sample;
        elapsed -= base;
        scale = timer->clock_sample_scale;
    }
    if (elapsed / scale > threshold) {
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 99.396550%; donor helper boundary matches; remaining
 * elapsed/base register swap would require the donor's volatile crutch. */
int SFTIM_IsStagnant(SfdHandle* handle)
{
    if (sftim_CheckStagnant(handle) != 0) {
        SFLIB_SetErr(handle, 0xFF000222);
        return 1;
    }
    return 0;
}

/* TODO: [near miss] 98.508770%; time-source fallback call setup differs;
 * retail/RE4 timer layout and callback flow agree, so stop at codegen. */
void SFTIM_VbIn(void)
{
    int i;
    int scale;
    int value;
    int token;
    int update_video_clock;
    int update_frame_clock;
    SfdHandle* handle;
    SfdTimeSourceFn get_time;
    SfdHandle** handles;

    handles = SFLIB_libwork.handles;
    SFLIB_libwork.timer_work.field_00++;
    for (i = 0; i < 8; i++) {
        handle = handles[i];
        if (handle == 0) {
            continue;
        }

        if (handle->playback_state != 4) {
            update_video_clock = 0;
        } else if (handle->field_0050 != 0) {
            update_video_clock = 0;
        } else if (handle->playback_runtime.field_1C != 0) {
            update_video_clock = 0;
        } else {
            update_video_clock = 1;
        }
        if (update_video_clock != 0) {
            /* The retail clock wraps at 32 bits; signed samples retain -1 sentinels. */
            handle->timer_state.video_clock_sample +=
                (unsigned int)handle->timer_state.speed;
        }

        if (handle->timer_state.field_02CC == -1) {
            update_frame_clock = 0;
        } else if (handle->requested_state != 4) {
            update_frame_clock = 0;
        } else {
            update_frame_clock = 1;
        }
        if (update_frame_clock != 0) {
            handle->timer_state.field_02CC +=
                (unsigned int)handle->timer_state.speed;
        }

        if (SFSET_GetCond(handle, 0x47) == 1) {
            SFLIB_LockCs(&token);
            get_time =
                handle->timer_state.time_sources[SFSET_GetCond(handle, 0x0F)];
            if (get_time == 0) {
                get_time = sftim_GetTimeNone;
            }
            get_time(handle, &value, &scale);
            SFLIB_UnlockCs(&token);
            if (handle->timer_state.current_time_value != value ||
                handle->timer_state.current_time_scale != scale) {
                if (SFSET_GetCond(handle, 0x47) == 1) {
                    handle->timer_state.previous_clock_sample =
                        handle->timer_state.video_clock_sample;
                } else {
                    handle->timer_state.previous_clock_sample =
                        handle->timer_state.current_clock_sample;
                }
                handle->timer_state.current_time_value = value;
                handle->timer_state.current_time_scale = scale;
            }
            handle->field_0044 = 1;
        }
    }
}

int SFTIM_GetNextItime(SfdTimerState* state, int time)
{
    int itime;
    int minimum;
    int maximum;
    int result;

    itime = state->interval_time_last;
    minimum = itime + state->interval_time_estimate;
    maximum = itime + state->interval_time_max;
    if (time < minimum) {
        result = minimum;
    } else if (time < maximum) {
        result = maximum;
    } else {
        result = 0x7FFFFFFF;
    }
    return result;
}

void SFTIM_UpdateItime(SfdTimerState* state, int time)
{
    int delta;
    int minimum;
    int step;

    if (state->interval_time_last == -5) {
        state->interval_time_last = time;
        return;
    }
    delta = time - state->interval_time_last;
    if (delta == 0) {
        return;
    }
    state->interval_time_last = time;
    state->interval_time_max = state->interval_time_max > delta
                                   ? state->interval_time_max
                                   : delta;
    state->interval_time_min = state->interval_time_min < delta
                                   ? state->interval_time_min
                                   : delta;
    minimum = state->interval_time_estimate;
    if (minimum == 0x7FFFFFFF) {
        state->interval_time_estimate = delta;
        return;
    }
    if (minimum <= delta) {
        state->interval_time_estimate = delta;
        return;
    }
    step = (minimum - delta) / 8;
    if (step != 0) {
        state->interval_time_estimate = minimum - step;
        return;
    }
    state->interval_time_estimate = delta;
}

void SFTIM_InitTtu(SfdTimerTimeUnit* unit, int scale)
{
    unit->active = 0;
    unit->fields_04[0] = 0;
    unit->fields_04[1] = 0;
    unit->fields_04[2] = 0;
    unit->fields_04[3] = 0;
    unit->fields_04[4] = 0;
    unit->fields_04[5] = 0;
    unit->fields_04[6] = 0;
    unit->field_20 = 0;
    unit->field_22 = 0;
    unit->value = scale;
    unit->scale = 1;
}

static inline void sftim_InitStreamTimeUnit(SfdTimerStreamTimeUnit* unit)
{
    unit->active = 0;
    unit->field_04 = 0;
    unit->file_size = 0;
    unit->total_time_value = 0;
    unit->total_time_scale = 0;
    unit->byte_rate = 0;
    unit->seek_position = 0;
    unit->field_1C = 0;
    unit->field_20 = 0;
    unit->field_22 = 0;
    unit->value = -1;
    unit->scale = 1;
}

/* TODO: [breakthrough needed] 60.986843%; retail omits the +0x14C store;
 * sample-history guards and zero-register scheduling still differ. */
void SFTIM_InitHn(SfdHandle* handle, SfdTimerState* state)
{
    int i;

    handle->timer_state.time_sources[0] = sftim_GetTimeNone;
    handle->timer_state.time_sources[1] = sftim_GetTimeVsync;
    handle->timer_state.time_sources[2] = 0;
    handle->timer_state.time_sources[3] = sftim_GetTimeUfrm;
    handle->timer_state.time_sources[4] = 0;
    handle->timer_state.time_sources[5] = sftim_GetTimeExtClock;

    state->skip_state.callback = 0;
    for (i = 0; i < 7; i++) {
        state->skip_state.fields_04[i] = 0;
    }
    state->skip_state.field_20 = 0;
    state->skip_state.field_22 = 0;
    SFTIM_InitTtu(&state->field_00C0, 0);
    SFTIM_InitTtu(&state->field_003C, 0x7FFFFFFF);

    SFTIM_InitTtu(&state->field_0068, -1);
    sftim_InitStreamTimeUnit(&state->stream_time);
    SFTIM_InitTtu(&state->video_start_time, -1);
    SFTIM_InitTtu(&state->elapsed_time, 0x7FFFFFFF);

    state->start_time_value = 0;
    state->start_time_scale = 0;
    state->field_0150 = -1;
    state->audio_start_pts = -1;

    state->sample_history.fields_00[0] = 0;
    state->sample_history.fields_00[1] = 0;
    state->sample_history.fields_00[2] = 0;
    for (i = 0; i < sizeof(state->sample_history.samples) /
                        sizeof(state->sample_history.samples[0]); i++) {
        state->sample_history.samples[i] = 0;
    }
    state->sample_window.enabled = 1;
    state->sample_window.fields_04[0] = 0;
    state->sample_window.fields_04[1] = 0;
    state->sample_window.fields_04[2] = 0;
    for (i = 0; i < sizeof(state->sample_window.samples) /
                        sizeof(state->sample_window.samples[0]); i++) {
        state->sample_window.samples[i] = 0;
    }

    state->video_end_time_value = -5;
    state->video_end_time_scale = 1;
    state->field_0284 = -5;
    state->field_0288 = 1;
    state->current_time_value = -1;
    state->current_time_scale = 1;
    state->interval_time_last = -5;
    state->interval_time_estimate = 0x7FFFFFFF;
    state->interval_time_max = 0;
    state->interval_time_min = 0x7FFFFFFF;
    state->field_02A4 = 0;
    state->video_clock_sample = 0;
    state->speed = 1000;
    state->field_02B0 = 0;
    state->field_02B4 = 0;
    state->field_02B8 = 1;
    state->frame_time_repeat_count = 100;
    state->previous_frame_time = -1.0f;
    state->previous_frame_ready = 0;
    state->last_frame_time = -1.0f;
    state->field_02CC = -1;
    state->previous_clock_sample = state->video_clock_sample;
    state->external_clock_callback = 0;
    state->previous_external_sample = -5;
    state->current_clock_sample = 0;
    state->clock_sample_scale = 1;
    state->external_clock_wrap = -1;
    state->external_clock_object = 0;
    state->video_pts.field_00 = 0;
    state->video_pts.field_04 = 0;
    state->video_pts.field_08 = 0;
}

void SFTIM_Finish(SfdTimerLibraryWork* work)
{
    (void)work;
}

void SFTIM_Init(SfdTimerLibraryWork* work, int source)
{
    work->field_00 = 0;
    work->field_04 = 0;
    work->source = source;
}
