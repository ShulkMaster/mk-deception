#ifndef PLATFORM_GCPIPEMANAGER_H
#define PLATFORM_GCPIPEMANAGER_H

#include "rw/rpworld_types.h"

typedef struct GXColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} GXColor;

typedef void (*DpMaterialCallback)(RwRGBAReal* color, GXColor* material,
                                   float intensity);

DpMaterialCallback DPObjectRenderSetup(int flags, unsigned int light_mask,
                                       int use_matfx, int use_alpha);
void GCNSetupNonRenderwarePipeline(RpClump* clump, void* owner);

#endif
