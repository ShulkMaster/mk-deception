#pragma once

#include "rw/rwcore_types.h"

void _rwDlTextureSet(RwTexture* texture, unsigned int textureMap);
void RwGameCubeTextureSetLOD(RwTexture* texture, float lodBias,
                             int biasClamp, int edgeLod,
                             int maxAnisotropy);
