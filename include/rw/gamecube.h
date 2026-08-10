#ifndef RW_GAMECUBE_H
#define RW_GAMECUBE_H

#include "rw/rpworld_types.h"

typedef struct RpSkin RpSkin;
typedef struct RwResEntry RwResEntry;

typedef struct RwTexCoords {
    RwReal u;
    RwReal v;
} RwTexCoords;

typedef struct RwGameCubeVertexArray {
    void* data;
    RwUInt8 attribute;
    RwUInt8 stride;
    RwUInt8 descriptor;
    RwUInt8 field_07;
} RwGameCubeVertexArray;

typedef struct RwGameCubeVertexStream {
    void* data;
    RwUInt8 field_04;
    RwUInt8 stride;
    RwUInt8 field_06[2];
} RwGameCubeVertexStream;

typedef struct RwGameCubeVertexStreams {
    RwUInt8 field_00[0x0C];
    RwGameCubeVertexStream streams[12];
} RwGameCubeVertexStreams;

typedef struct RwGameCubeVertexData {
    RwUInt32 counts[26];
    const void* source[26];
} RwGameCubeVertexData;

typedef struct RwGameCubeVertexBuffer {
    RwUInt32 field_00[2];
    RwUInt32 numArrays;
    RwGameCubeVertexArray arrays[1];
} RwGameCubeVertexBuffer;

typedef struct RwGameCubeVertexDescriptor {
    RwUInt32 vat;
    RwUInt32 vatA;
    RwUInt32 vatB;
    RwUInt32 vatC;
    RwUInt32 vcdLo;
    RwUInt32 vcdHi;
    RwUInt32 metadata;
    RwUInt8 numIndexedAttrs;
    RwUInt8 field_1D[3];
} RwGameCubeVertexDescriptor;

void _rxGCResEntryWaitDone(RwResEntry* entry);
void RwGameCubeTextureSetLOD(RwTexture* texture, int field_0x0C,
                             int field_0x10, int field_0x08, float lod_bias);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
void _rwDlTextureSet(RwTexture* texture, int mapid);
void _rwDlTextureRasterFlush(void);

#endif
