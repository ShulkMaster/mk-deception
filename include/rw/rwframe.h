#ifndef RW_RWFRAME_H
#define RW_RWFRAME_H

#include "rw/rtquat.h"

typedef struct RwFrame RwFrame;

RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
void* _rwFrameOpen(void* instance, int offset, int size);
void* _rwFrameClose(void* instance, int offset, int size);

#endif
