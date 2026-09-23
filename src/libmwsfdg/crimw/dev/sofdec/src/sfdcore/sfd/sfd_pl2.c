#include "sofdec/sfd_error.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_timer.h"
#include "sofdec/sfd_transport.h"

void SFAOAP_SetSpeed(SfdHandle* handle, int speed);

static int sfpl2_PauseSub(SfdHandle* handle, int state)
{
    int result;

    if (handle->requested_state != 3 && handle->requested_state != 4) {
        result = 0;
    } else {
        int call_result;

        SFTIM_Pause(handle, state);
        call_result = SFTRN_CallTrtTrif(handle, 7, 8, state, 0);
        result = 0;
        if (call_result != 0) {
            result = call_result;
        }
    }
    return result;
}

int SFD_SetAudioCh(SfdHandle* handle, int channel)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000145);
    }
    SFD_SetCond(handle, 30, channel);
    return 0;
}

int SFD_SetSpeed(SfdHandle* handle, int speed)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000144);
    }
    SFTIM_SetSpeed(handle, speed);
    SFAOAP_SetSpeed(handle, speed);
    return 0;
}

int SFPL2_Standby(SfdHandle* handle)
{
    handle->requested_state = 3;
    return 0;
}

int SFD_Standby(SfdHandle* handle)
{
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000143);
    }
    handle->requested_state = 3;
    return 0;
}

int SFPL2_Pause(SfdHandle* handle, int state)
{
    int result;

    result = 0;
    switch (state) {
    case 2:
        if (handle->playback_state == 4) {
            result = sfpl2_PauseSub(handle, 2);
        }
        break;
    case 1:
        if (handle->field_0054++ == 0) {
            result = sfpl2_PauseSub(handle, 1);
        }
        break;
    case 0:
        if (--handle->field_0054 == 0) {
            result = sfpl2_PauseSub(handle, 0);
        }
        break;
    }
    return result;
}

int SFD_Pause(SfdHandle* handle, int pause)
{
    int pause_state;
    int transition;
    int result;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000142);
    }
    pause_state = handle->field_0050;
    if (pause == 0) {
        if (pause_state == 0) {
            return 0;
        }
        transition = 0;
    } else {
        if (pause_state == 0) {
            transition = 1;
        } else {
            transition = 2;
        }
    }
    handle->field_0050 = pause;
    result = SFPL2_Pause(handle, transition);
    handle->field_0044 = 1;
    return result;
}
