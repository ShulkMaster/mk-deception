#ifndef DOLPHIN_GX_FIFO_H
#define DOLPHIN_GX_FIFO_H

typedef struct GXFifoObj {
    unsigned char data[0x80];
} GXFifoObj;
typedef struct OSThread OSThread;
typedef void (*GXBreakPtCallback)(void);
typedef void (*GXDrawSyncCallback)(unsigned short token);
typedef void (*GXDrawDoneCallback)(void);

void GXInitFifoBase(GXFifoObj* fifo, void* base, unsigned long size);
void GXInitFifoPtrs(GXFifoObj* fifo, void* readPointer, void* writePointer);
void GXInitFifoLimits(GXFifoObj* fifo, unsigned long highWatermark,
                      unsigned long lowWatermark);
void GXSetCPUFifo(GXFifoObj* fifo);
void GXSetGPFifo(GXFifoObj* fifo);
void GXSaveCPUFifo(GXFifoObj* fifo);
OSThread* GXSetCurrentGXThread(void);
GXFifoObj* GXGetCPUFifo(void);
GXFifoObj* GXGetGPFifo(void);
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPointer, void** writePointer);
void GXEnableBreakPt(void* address);
void GXDisableBreakPt(void);
GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback callback);
GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback callback);
void GXSetDrawDone(void);

#endif
