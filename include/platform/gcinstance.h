#ifndef PLATFORM_GCINSTANCE_H
#define PLATFORM_GCINSTANCE_H

#include "rw/rpworld_types.h"

RwStream* inplaceSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry);
RpGeometry* inplaceGeometryNativeRead(RwStream* stream, RpGeometry* geometry);
int _inplaceNativeTextureRead(RwStream* stream, RwTexture** texture);

#endif
