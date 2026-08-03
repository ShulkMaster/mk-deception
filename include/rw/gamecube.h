#ifndef RW_GAMECUBE_H
#define RW_GAMECUBE_H

#include "rw/rpworld_types.h"

typedef struct RpSkin RpSkin;
typedef struct RwResEntry RwResEntry;

void _rxGCResEntryWaitDone(RwResEntry* entry);
void RwGameCubeTextureSetLOD(RwTexture* texture, int field_0x0C,
                             int field_0x10, int field_0x08, float lod_bias);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
void _rwDlTextureSet(RwTexture* texture, int mapid);
void _rwDlTextureRasterFlush(void);

#endif
