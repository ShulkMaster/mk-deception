#include "sofdec/sfd_transport.h"

enum {
    SFD_USER_OUTPUT_TRANSPORT = 8,
    SFD_USER_OUTPUT_CHANNEL_COUNT = 3
};

static int SFUO_Seek(SfdHandle* handle, int parameter, int value)
{
    return 0;
}

static int SFUO_AddRead(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000601);
}

static int SFUO_GetRead(SfdHandle* handle, void* buffer)
{
    return SFLIB_SetErr(handle, 0xFF000601);
}

static int SFUO_AddWrite(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000601);
}

static int SFUO_GetWrite(SfdHandle* handle, void* buffer)
{
    return SFLIB_SetErr(handle, 0xFF000601);
}

static int SFUO_Pause(SfdHandle* handle, int state)
{
    (void)state;
    return 0;
}

static int SFUO_Stop(SfdHandle* handle)
{
    return 0;
}

static int SFUO_Start(SfdHandle* handle)
{
    return 0;
}

static int SFUO_Standby(SfdHandle* handle)
{
    return 0;
}

static int SFUO_Destroy(SfdHandle* handle)
{
    return 0;
}

static void sfuo_InitCh(SfdHandle* handle, SfdUserOutputWork* work,
                        int buffer_index)
{
    int channel_index;

    for (channel_index = 0; channel_index < SFD_USER_OUTPUT_CHANNEL_COUNT;
         channel_index++) {
        SfdBufferChannel* channel = &work->channels[channel_index];

        channel->stream_joint = 0;
        channel->object = 0;
        channel->handle_callback = 0;
        channel->object_callback = 0;
        SFBUF_SetUoch(handle, buffer_index, channel_index, channel);
    }
}

static int SFUO_Create(SfdHandle* handle)
{
    int buffer_index;

    handle->transports[SFD_USER_OUTPUT_TRANSPORT].context =
        &handle->user_output_work;
    buffer_index =
        handle->transports[SFD_USER_OUTPUT_TRANSPORT].parameter_10;
    handle->user_output_work.state = 0;
    sfuo_InitCh(handle, &handle->user_output_work, buffer_index);
    return 0;
}

static int SFUO_ExecServer(SfdHandle* handle)
{
    if (SFTRN_GetTermFlg(handle, SFD_USER_OUTPUT_TRANSPORT) != 1 &&
        SFBUF_GetTermFlg(
            handle,
            handle->transports[SFD_USER_OUTPUT_TRANSPORT].parameter_10) == 1) {
        SFTRN_SetTermFlg(handle, SFD_USER_OUTPUT_TRANSPORT, 1);
    }
    if (SFTRN_GetPrepFlg(handle, SFD_USER_OUTPUT_TRANSPORT) != 1 &&
        SFBUF_GetPrepFlg(
            handle,
            handle->transports[SFD_USER_OUTPUT_TRANSPORT].parameter_10) == 1) {
        SFTRN_SetPrepFlg(handle, SFD_USER_OUTPUT_TRANSPORT, 1);
    }
    return 0;
}

static int SFUO_Finish(SfdHandle* handle)
{
    return 0;
}

static int SFUO_Init(SfdHandle* handle)
{
    return 0;
}

int SFD_SetUsrSj(SfdHandle* handle, int channel_index, SJ* stream_joint,
                 SfdCallbackObject object)
{
    SfdUserOutputWork* work;
    int buffer_index;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000191);
    }
    buffer_index =
        handle->transports[SFD_USER_OUTPUT_TRANSPORT].parameter_10;
    work = (SfdUserOutputWork*)
        handle->transports[SFD_USER_OUTPUT_TRANSPORT].context;
    if (buffer_index == SFD_USER_OUTPUT_TRANSPORT) {
        return SFLIB_SetErr(handle, 0xFF000602);
    }

    work->channels[channel_index].stream_joint = stream_joint;
    work->channels[channel_index].object = object;
    work->channels[channel_index].handle_callback = 0;
    work->channels[channel_index].object_callback = 0;
    SFBUF_SetUoch(handle, buffer_index, channel_index,
                  &work->channels[channel_index]);
    return 0;
}

const SfdTransportInterface SFD_tr_uo = {
    SFUO_Init,     SFUO_Finish,   SFUO_ExecServer, SFUO_Create,
    SFUO_Destroy,  SFUO_Standby,  SFUO_Start,      SFUO_Stop,
    SFUO_Pause,    SFUO_GetWrite, SFUO_AddWrite,   SFUO_GetRead,
    SFUO_AddRead,  SFUO_Seek,
};
