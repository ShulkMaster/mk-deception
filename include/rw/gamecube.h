#ifndef RW_GAMECUBE_H
#define RW_GAMECUBE_H

#include "rw/rpworld_types.h"

typedef struct RpSkin RpSkin;
typedef struct RwResEntry RwResEntry;

typedef struct RwGameCubeVtxFmt {
    RwUInt32 reserved_0x00;
    RwUInt32 vatA;             /* +0x04 */
    RwUInt32 reserved_0x08[2];
    RwUInt32 vcdLo;            /* +0x10 */
    RwUInt32 vcdHi;            /* +0x14 */
} RwGameCubeVtxFmt;

typedef struct RwGameCubeDisplayList {
    void* data;
    RwUInt32 size;
} RwGameCubeDisplayList;

typedef char RwGameCubeVtxFmtSizeCheck[
    sizeof(RwGameCubeVtxFmt) == 0x18 ? 1 : -1];

void _rxGCResEntryWaitDone(RwResEntry* entry);
void RwGameCubeTextureSetLOD(RwTexture* texture, int field_0x0C,
                             int field_0x10, int field_0x08, float lod_bias);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
void _rwDlTextureSet(RwTexture* texture, int mapid);
void _rwDlTextureRasterFlush(void);
RwUInt32 _rwGCNDisplayListGetStride(const RwGameCubeVtxFmt* format);
RwUInt32 _rwGCNDisplayListGetSize(const RwGameCubeVtxFmt* format,
                                  RwUInt32 numIndices,
                                  RwUInt32 numVertices);
void _rwGCNDisplayListInitialize(RwGameCubeDisplayList* displayList,
                                 RwUInt32 index, RwUInt32 size, void* data);

#endif
