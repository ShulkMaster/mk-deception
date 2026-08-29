#include "sofdec/sfd_transport.h"

static int SFMEM_Seek(SfdHandle* handle, int parameter, int value)
{
    return 0;
}

static int SFMEM_AddRead(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000501);
}

static int SFMEM_GetRead(SfdHandle* handle, void* buffer)
{
    return SFLIB_SetErr(handle, 0xFF000501);
}

static int SFMEM_AddWrite(SfdHandle* handle, int parameter, int value)
{
    return SFBUF_RingAddWrite(handle, handle->transports[0].parameter_14,
                              parameter, value);
}

static int SFMEM_GetWrite(SfdHandle* handle, void* buffer)
{
    return SFBUF_RingGetWrite(handle, handle->transports[0].parameter_14,
                              buffer);
}

static int SFMEM_Pause(SfdHandle* handle)
{
    return 0;
}

static int SFMEM_Stop(SfdHandle* handle)
{
    return 0;
}

static int SFMEM_Start(SfdHandle* handle)
{
    return 0;
}

static int SFMEM_Standby(SfdHandle* handle)
{
    return 0;
}

static int SFMEM_Destroy(SfdHandle* handle)
{
    return 0;
}

static int SFMEM_Create(SfdHandle* handle)
{
    return 0;
}

static int SFMEM_ExecServer(SfdHandle* handle)
{
    SFBUF_SetPrepFlg(handle, handle->transports[0].parameter_14, 1);
    return 0;
}

static int SFMEM_Finish(SfdHandle* handle)
{
    return 0;
}

static int SFMEM_Init(SfdHandle* handle)
{
    return 0;
}

const SfdTransportInterface SFD_tr_in_mem = {
    SFMEM_Init,     SFMEM_Finish,   SFMEM_ExecServer, SFMEM_Create,
    SFMEM_Destroy,  SFMEM_Standby,  SFMEM_Start,      SFMEM_Stop,
    SFMEM_Pause,    SFMEM_GetWrite, SFMEM_AddWrite,   SFMEM_GetRead,
    SFMEM_AddRead,  SFMEM_Seek,
};
