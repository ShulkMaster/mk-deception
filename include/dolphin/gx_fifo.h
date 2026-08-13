#ifndef DOLPHIN_GX_FIFO_H
#define DOLPHIN_GX_FIFO_H

typedef struct GXFifoObj GXFifoObj;
typedef void (*GXBreakPtCallback)(void);
typedef void (*GXDrawDoneCallback)(void);

GXFifoObj* GXGetCPUFifo(void);
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPointer, void** writePointer);
void GXEnableBreakPt(void* address);
void GXDisableBreakPt(void);
GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback callback);
GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback callback);
void GXSetDrawDone(void);

#endif
