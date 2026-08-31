#include "sofdec/sfd_error.h"
#include "sofdec/sfd_transport.h"

int SFCON_ReadTotSmplQue(SfdHandle* handle, int* samples, int* sample_rate)
{
    SfdTimerSampleWindow* queue = &handle->timer_state.sample_window;
    int token;
    int result;

    SFLIB_LockCs(&token);
    if (queue->fields_04[1] - queue->fields_04[2] <= 0) {
        result = 0;
        *samples = -1;
    } else {
        result = 1;
        *sample_rate = queue->enabled;
        *samples = queue->samples[queue->fields_04[2] % 32];
        queue->fields_04[2]++;
    }
    SFLIB_UnlockCs(&token);
    return result;
}

int SFCON_WriteTotSmplQue(SfdHandle* handle, int samples, int sample_rate)
{
    SfdTimerSampleWindow* queue = &handle->timer_state.sample_window;
    int token;
    int result;

    SFLIB_LockCs(&token);
    if (queue->fields_04[1] - queue->fields_04[2] >= 32) {
        result = 0;
    } else {
        queue->enabled = sample_rate;
        result = 1;
        queue->samples[queue->fields_04[1] % 32] = samples;
        queue->fields_04[1]++;
    }
    SFLIB_UnlockCs(&token);
    return result;
}

void SFCON_UpdateConcatTime(SfdHandle* handle, int concat_time)
{
    SfdTimerSampleHistory* history = &handle->timer_state.sample_history;
    int token;
    int value;
    int write_index;

    SFLIB_LockCs(&token);
    value = history->fields_00[1] + concat_time;
    write_index = history->fields_00[2] + 1;
    history->fields_00[1] = value;
    history->samples[write_index % 32] = value;
    history->fields_00[2] = write_index;
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
