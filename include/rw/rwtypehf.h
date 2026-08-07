#ifndef RW_RWTYPEHF_H
#define RW_RWTYPEHF_H

#include "rw/rwcore_types.h"

typedef struct RwObjectHasFrame {
    RwObject object;
    RwLLLink lFrame;
} RwObjectHasFrame;

void _rwObjectHasFrameSetFrame(RwObjectHasFrame* object, RwFrame* frame);
void _rwObjectHasFrameReleaseFrame(RwObjectHasFrame* object);

#endif
