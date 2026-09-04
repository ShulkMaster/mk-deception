#include "sofdec/sfd_player.h"
#include "sofdec/sfd_timer.h"
#include "sofdec/sfd_transport.h"

enum {
    SFD_VIDEO_OUTPUT_TRANSPORT = 6
};

static int SFVOM_Seek(SfdHandle* handle, int parameter, int value)
{
    return 0;
}

static int SFVOM_AddRead(SfdHandle* handle, int amount, int value)
{
    return SFBUF_VfrmAddRead(
        handle, handle->transports[SFD_VIDEO_OUTPUT_TRANSPORT].parameter_10,
        amount);
}

static int SFVOM_GetRead(SfdHandle* handle, SfdFrameTime** output)
{
    unsigned int state;
    int result;

    state = handle->playback_state;
    if (state - 3 > 1) {
        *output = 0;
        return 0;
    }
    result = SFBUF_VfrmGetRead(
        handle, handle->transports[SFD_VIDEO_OUTPUT_TRANSPORT].parameter_10,
        output);
    if (result != 0) {
        return result;
    }
    if (SFTIM_IsGetFrmTime(handle, *output) == 0) {
        *output = 0;
        return 0;
    }
    return 0;
}

static int SFVOM_AddWrite(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000701);
}

static int SFVOM_GetWrite(SfdHandle* handle, void* output)
{
    return SFLIB_SetErr(handle, 0xFF000701);
}

static int SFVOM_Pause(SfdHandle* handle, int state)
{
    return 0;
}

static int SFVOM_Stop(SfdHandle* handle)
{
    return 0;
}

static int SFVOM_Start(SfdHandle* handle)
{
    return 0;
}

static int SFVOM_Standby(SfdHandle* handle)
{
    return 0;
}

static int SFVOM_Destroy(SfdHandle* handle)
{
    return 0;
}

static int SFVOM_Create(SfdHandle* handle)
{
    return 0;
}

static int SFVOM_ExecServer(SfdHandle* handle)
{
    int video_terminated;

    if (SFSET_GetCond(handle, 5) == 0) {
        return 0;
    }
    if (SFTRN_GetTermFlg(handle, SFD_VIDEO_OUTPUT_TRANSPORT) != 1 &&
        SFBUF_GetTermFlg(
            handle,
            handle->transports[SFD_VIDEO_OUTPUT_TRANSPORT].parameter_10) == 1) {
        if (SFSET_GetCond(handle, 0x0F) == 0) {
            video_terminated = 1;
        } else if (SFTIM_IsVideoTerm(handle) == 0) {
            video_terminated = 0;
        } else {
            video_terminated = 1;
        }
        if (video_terminated != 0) {
            SFTRN_SetTermFlg(handle, SFD_VIDEO_OUTPUT_TRANSPORT, 1);
        }
    }
    if (SFTRN_GetPrepFlg(handle, SFD_VIDEO_OUTPUT_TRANSPORT) != 1 &&
        SFBUF_GetPrepFlg(
            handle,
            handle->transports[SFD_VIDEO_OUTPUT_TRANSPORT].parameter_10) == 1) {
        SFTRN_SetPrepFlg(handle, SFD_VIDEO_OUTPUT_TRANSPORT, 1);
    }
    return 0;
}

static int SFVOM_Finish(SfdHandle* handle)
{
    return 0;
}

static int SFVOM_Init(SfdHandle* handle)
{
    return 0;
}

const SfdTransportInterface SFD_tr_vo_manu = {
    SFVOM_Init,     SFVOM_Finish,   SFVOM_ExecServer, SFVOM_Create,
    SFVOM_Destroy,  SFVOM_Standby,  SFVOM_Start,      SFVOM_Stop,
    SFVOM_Pause,    SFVOM_GetWrite, SFVOM_AddWrite,
    (SfdTransportBufferFn)SFVOM_GetRead,
    SFVOM_AddRead,  SFVOM_Seek,
};
