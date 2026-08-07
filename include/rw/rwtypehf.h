#ifndef RW_RWTYPEHF_H
#define RW_RWTYPEHF_H

#include "rw/rwcore_types.h"

typedef struct RwObjectHasFrame RwObjectHasFrame;
typedef RwObjectHasFrame* (*RwObjectHasFrameSyncFunction)(
    RwObjectHasFrame* object);

struct RwObjectHasFrame {
    RwObject object;
    RwLLLink lFrame;
    RwObjectHasFrameSyncFunction sync;
};

void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
void _rwObjectHasFrameReleaseFrame(void* object);

#endif
