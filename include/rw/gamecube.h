#ifndef RW_GAMECUBE_H
#define RW_GAMECUBE_H

#include "dolphin/gx.h"
#include "rw/rpworld_types.h"

typedef struct RpSkin RpSkin;
typedef struct RwResEntry RwResEntry;
typedef void (*RwDlObjectRenderCallBack)(const RwRGBAReal* surface,
                                         const GXColor* material,
                                         RwReal intensity);

typedef struct RwGameCubeVtxFmt {
    RwUInt32 reserved_0x00;
    RwUInt32 vatA;             /* +0x04 */
    RwUInt32 reserved_0x08[2];
    RwUInt32 vcdLo;            /* +0x10 */
    RwUInt32 vcdHi;            /* +0x14 */
} RwGameCubeVtxFmt;

typedef struct RpGameCubeVtxFmt {
    union {
        struct {
            RwUInt8 positionType;       /* +0x00 */
            RwUInt8 normalType;         /* +0x01 */
            RwUInt8 texCoordType[8];    /* +0x02 */
            RwUInt8 colorType;          /* +0x0A */
            RwUInt8 field_0x0B;
            RwUInt8 positionFraction;   /* +0x0C */
            RwUInt8 normalMode;         /* +0x0D */
            RwUInt8 texCoordFraction[8]; /* +0x0E */
        };
        RwUInt8 fields[0x16];
    };
    RwUInt16 refCount;          /* +0x16 */
} RpGameCubeVtxFmt;

typedef struct RwGameCubeDisplayList {
    void* data;
    RwUInt32 size;
} RwGameCubeDisplayList;

typedef struct RwGameCubeVertexDescriptor {
    RwUInt32 vat;
    RwUInt32 vatA;
    RwUInt32 vatB;
    RwUInt32 vatC;
    RwUInt32 vcdLo;
    RwUInt32 vcdHi;
    RwUInt32 metadata;
    RwUInt8 numIndexedAttrs;
    RwUInt8 reserved_0x1D[3];
} RwGameCubeVertexDescriptor;

typedef char RwGameCubeVtxFmtSizeCheck[
    sizeof(RwGameCubeVtxFmt) == 0x18 ? 1 : -1];
typedef char RpGameCubeVtxFmtSizeCheck[
    sizeof(RpGameCubeVtxFmt) == 0x18 ? 1 : -1];
typedef char RwGameCubeVertexDescriptorSizeCheck[
    sizeof(RwGameCubeVertexDescriptor) == 0x20 ? 1 : -1];

void _rxGCResEntryWaitDone(RwResEntry* entry);
void RwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias,
                             RwInt32 biasClamp, RwInt32 edgeLod,
                             RwInt32 maxAnisotropy);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
void _rwDlTextureSet(RwTexture* texture, int mapid);
void _rwDlTextureRasterFlush(void);
RwDlObjectRenderCallBack _rwDlObjectRenderSetup(RwUInt32 flags,
                                                 RwUInt32 lightMask,
                                                 RwInt32 textureMode,
                                                 RwBool useAmbient);
RpGameCubeVtxFmt* _rpGameCubeVtxFmtGetDefault(void);
RwBool _rpDlVtxFmtPluginAttach(void);
void RpGameCubeVtxFmtSetPosition(RpGameCubeVtxFmt* format, RwUInt32 type,
                                 RwUInt8 fraction);
void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* format, RwUInt32 type,
                               RwUInt32 mode);
void RpGameCubeVtxFmtSetTexCoord(RpGameCubeVtxFmt* format, RwInt32 index,
                                 RwUInt32 type, RwUInt8 fraction);
void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* format);
RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void);
void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* format);
void RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry,
                                 RpGameCubeVtxFmt* format);
RwUInt32 _rwGCNDisplayListGetStride(const RwGameCubeVtxFmt* format);
RwUInt32 _rwGCNDisplayListGetSize(const RwGameCubeVtxFmt* format,
                                  RwUInt32 numIndices,
                                  RwUInt32 numVertices);
void _rwGCNDisplayListInitialize(RwGameCubeDisplayList* displayList,
                                 RwUInt32 index, RwUInt32 size, void* data);
void _rwVertexDescriptorInit(RwGameCubeVertexDescriptor* descriptor);
void _rwGCNVertexDescSetVAT(RwGameCubeVertexDescriptor* descriptor,
                            RwUInt32 vat);
void _rwGCNVertexDescSetElementAttr(RwGameCubeVertexDescriptor* descriptor,
                                    RwUInt32 attr, RwInt32 componentCount,
                                    RwUInt32 componentType, RwUInt8 fraction);
void _rwGCNVertexDescSetElementDesc(RwGameCubeVertexDescriptor* descriptor,
                                    RwUInt32 attr, RwInt32 type);
void _rwGCNVertexDescSetNumIndexedAttr(
    RwGameCubeVertexDescriptor* descriptor, RwUInt8 count);

#endif
