#ifndef PLATFORM_GCPIPEMANAGER_H
#define PLATFORM_GCPIPEMANAGER_H

#include "dolphin/gx.h"
#include "rw/rpworld_types.h"

typedef void (*DpMaterialCallback)(RwRGBAReal* color, GXColor* material,
                                   void* material_data, float intensity);

DpMaterialCallback DPObjectRenderSetup(int flags, unsigned int light_mask,
                                       int use_matfx, int use_alpha);
void GCNSetupNonRenderwarePipeline(RpClump* clump, void* owner);

#endif
