#include "sofdec/sfd_error.h"
#include "sofdec/sfd_transport.h"

int SFCON_ReadTotSmplQue(SfdHandle* handle, int* samples, int* sample_rate)
{
    SfdTimerState* timer = &handle->timer_state;
    int token;
    int result;

    SFLIB_LockCs(&token);
    if (timer->sample_window.fields_04[1] -
            timer->sample_window.fields_04[2] <= 0) {
        result = 0;
        *samples = -1;
    } else {
        result = 1;
        *sample_rate = timer->sample_window.enabled;
        *samples = timer->sample_window.samples[
            timer->sample_window.fields_04[2] % 32];
        timer->sample_window.fields_04[2]++;
    }
    SFLIB_UnlockCs(&token);
    return result;
}

int SFCON_WriteTotSmplQue(SfdHandle* handle, int samples, int sample_rate)
{
    SfdTimerState* timer = &handle->timer_state;
    int token;
    int result;

    SFLIB_LockCs(&token);
    if (timer->sample_window.fields_04[1] -
            timer->sample_window.fields_04[2] >= 32) {
        result = 0;
    } else {
        timer->sample_window.enabled = sample_rate;
        result = 1;
        timer->sample_window.samples[
            timer->sample_window.fields_04[1] % 32] = samples;
        timer->sample_window.fields_04[1]++;
    }
    SFLIB_UnlockCs(&token);
    return result;
}

void SFCON_UpdateConcatTime(SfdHandle* handle, int concat_time)
{
    SfdTimerState* timer = &handle->timer_state;
    int token;
    int write_index;
    int cumulative_samples;

    SFLIB_LockCs(&token);
    timer->sample_history.fields_00[1] += concat_time;
    /* Soft ceiling: cumulative sample count/write index register coloring. */
    write_index = timer->sample_history.fields_00[2] + 1;
    cumulative_samples = timer->sample_history.fields_00[1];
    timer->sample_history.samples[write_index % 32] = cumulative_samples;
    timer->sample_history.fields_00[2] = write_index;
    SFLIB_UnlockCs(&token);
}

int SFCON_IsVideoEndcodeSkip(SfdHandle* handle)
{
    if (SFSET_GetCond(handle, 49) != 0 ||
        SFSET_GetCond(handle, 57) != 0) {
        return 1;
    }
    return 0;
}

int SFCON_IsSystemEndcodeSkip(SfdHandle* handle)
{
    if (SFSET_GetCond(handle, 49) != 0 ||
        SFSET_GetCond(handle, 56) != 0) {
        return 1;
    }
    return 0;
}

int SFCON_IsEndcodeSkip(SfdHandle* handle)
{
    return SFSET_GetCond(handle, 49) != 0;
}

int SFD_SetConcatPlay(SfdHandle* handle)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000161);
    }
    SFSET_SetCond(handle, 49, 1);
    return 0;
}
