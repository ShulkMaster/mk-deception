#include "runtime/cstring.h"

#include "dolphin/gx.h"
#include "dolphin/os.h"

#include "__gx.h"

static __GXFifoObj DisplayListFifo;
static GXFifoObj* OldCPUFifo;
static GXData __savedGXdata;

void GXBeginDisplayList(void* list, u32 size)
{
    GXFifoObj* cpuFifo = GXGetCPUFifo();

    if (__GXData->dirtyState != 0) {
        __GXSetDirtyState();
    }

    if (__GXData->dlSaveContext != 0) {
        memcpy(&__savedGXdata, __GXData, sizeof(__savedGXdata));
    }

    DisplayListFifo.base = list;
    DisplayListFifo.top = (u8*)list + size - 4;
    DisplayListFifo.size = size;
    DisplayListFifo.count = 0;
    DisplayListFifo.rdPtr = list;
    DisplayListFifo.wrPtr = list;
    __GXData->inDispList = TRUE;
    GXSaveCPUFifo(cpuFifo);
    OldCPUFifo = cpuFifo;
    GXSetCPUFifo((GXFifoObj*)&DisplayListFifo);
    GXResetWriteGatherPipe();
}

u32 GXEndDisplayList(void)
{
    u32 overflow;
    BOOL enabled;
    u32 cpEnable;

    GXFlush();
    overflow = (GX_GET_PI_REG(5) >> 26) & 1;
    __GXSaveCPUFifoAux(&DisplayListFifo);
    GXSetCPUFifo(OldCPUFifo);

    if (__GXData->dlSaveContext != 0) {
        enabled = OSDisableInterrupts();
        cpEnable = __GXData->cpEnable;
        memcpy(__GXData, &__savedGXdata, sizeof(*__GXData));
        __GXData->cpEnable = cpEnable;
        OSRestoreInterrupts(enabled);
    }

    __GXData->inDispList = FALSE;
    if (!overflow) {
        return DisplayListFifo.count;
    }

    return 0;
}

void GXCallDisplayList(void* list, u32 byteCount)
{
    if (__GXData->dirtyState != 0) {
        __GXSetDirtyState();
    }

    if (*(u32*)&__GXData->vNumNot == 0) {
        __GXSendFlushPrim();
    }

    GX_WRITE_U8(0x40);
    GX_WRITE_U32(list);
    GX_WRITE_U32(byteCount);
}
