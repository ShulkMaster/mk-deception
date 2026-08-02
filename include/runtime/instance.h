#ifndef RUNTIME_INSTANCE_H
#define RUNTIME_INSTANCE_H

#include "rw/rpworld_types.h"

RpClump* inplaceClumpStreamRead(RwStream* stream);
RpGeometry* inplaceGeometryCreate_80056E98(int num_vertices, int num_triangles,
                                           unsigned int format);
unsigned int PadSize32(unsigned int value);
int inplaceNativeTextureRead(RwStream* stream, RwTexture** texture);

#endif
