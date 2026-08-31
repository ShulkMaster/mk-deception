#include "sofdec/sfd_error.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_transport.h"

static int SFAOAP_Seek(SfdHandle* handle, int parameter, int value)
{
    return 0;
}

static int SFAOAP_AddRead(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000A01);
}

static int SFAOAP_GetRead(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000A01);
}

static int SFAOAP_AddWrite(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000A01);
}

static int SFAOAP_GetWrite(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000A01);
}

static int sfaoap_CallState(SfdHandle* handle, int callback, int value)
{
    int result = 0;

    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    value = SFTRN_CallTrtTrif(handle, 3, callback, value, 0);
    if (value != 0) {
        result = value;
    }
    return result;
}

static int SFAOAP_Pause(SfdHandle* handle, int pause)
{
    return sfaoap_CallState(handle, 8, pause);
}

static int SFAOAP_Stop(SfdHandle* handle)
{
    return sfaoap_CallState(handle, 7, 0);
}

static int SFAOAP_Start(SfdHandle* handle)
{
    return sfaoap_CallState(handle, 6, 0);
}

static int SFAOAP_Standby(SfdHandle* handle)
{
    return sfaoap_CallState(handle, 5, 0);
}

static int SFAOAP_Destroy(SfdHandle* handle)
{
    return 0;
}

static int SFAOAP_Create(SfdHandle* handle)
{
    if (SFSET_GetCond(handle, 6) != 0) {
        handle->transports[7].context = &handle->audio_output_callbacks;
    }
    return 0;
}

static int SFAOAP_ExecServer(SfdHandle* handle)
{
    int buffer_index;

    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    buffer_index = handle->transports[7].parameter_10;
    if (SFTRN_GetPrepFlg(handle, 7) != 1 &&
        SFBUF_GetPrepFlg(handle, buffer_index) == 1) {
        SFTRN_SetPrepFlg(handle, 7, 1);
    }
    if (SFTRN_GetTermFlg(handle, 7) != 1 &&
        SFBUF_GetTermFlg(handle, buffer_index) == 1) {
        SFTRN_SetTermFlg(handle, 7, 1);
    }
    return 0;
}

static int SFAOAP_Finish(SfdHandle* handle)
{
    return 0;
}

static int SFAOAP_Init(SfdHandle* handle)
{
    return 0;
}

void SFAOAP_SetSpeed(SfdHandle* handle, int speed)
{
    if (SFSET_GetCond(handle, 6) != 0) {
        SfdAudioOutputCallbacks* callbacks =
            handle->transports[7].context;
        if (callbacks->set_speed != 0) {
            callbacks->set_speed(handle, speed);
        }
    }
}

int SFD_GetOutVol(SfdHandle* handle)
{
    SfdAudioOutputCallbacks* callbacks;

    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF0001A4);
        return 0;
    }
    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    callbacks = handle->transports[7].context;
    return callbacks->get_volume(handle, callbacks);
}

void SFD_SetOutVol(SfdHandle* handle, int volume)
{
    SfdAudioOutputCallbacks* callbacks;

    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF0001A3);
        return;
    }
    if (SFSET_GetCond(handle, 6) != 0) {
        callbacks = handle->transports[7].context;
        callbacks->set_volume(handle, volume, callbacks);
    }
}

int SFD_GetOutPan(SfdHandle* handle, int channel)
{
    SfdAudioOutputCallbacks* callbacks;

    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF0001A2);
        return 0;
    }
    if (SFSET_GetCond(handle, 6) == 0) {
        return 0;
    }
    callbacks = handle->transports[7].context;
    return callbacks->get_pan(handle, channel, callbacks);
}

void SFD_SetOutPan(SfdHandle* handle, int channel, int pan)
{
    SfdAudioOutputCallbacks* callbacks;

    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF0001A1);
        return;
    }
    if (SFSET_GetCond(handle, 6) != 0) {
        callbacks = handle->transports[7].context;
        callbacks->set_pan(handle, channel, pan, callbacks);
    }
}

const SfdTransportInterface SFD_tr_ao_auto_p = {
    SFAOAP_Init,     SFAOAP_Finish,   SFAOAP_ExecServer, SFAOAP_Create,
    SFAOAP_Destroy,  SFAOAP_Standby,  SFAOAP_Start,      SFAOAP_Stop,
    SFAOAP_Pause,    SFAOAP_GetWrite, SFAOAP_AddWrite,   SFAOAP_GetRead,
    SFAOAP_AddRead,  SFAOAP_Seek,
};
