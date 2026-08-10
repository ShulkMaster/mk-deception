#ifndef RW_GAMECUBE_H
#define RW_GAMECUBE_H

#include "dolphin/gx.h"
#include "rw/rpworld_types.h"

typedef struct RpSkin RpSkin;
typedef struct RwResEntry RwResEntry;
typedef void (*RwDlObjectRenderCallBack)(const RwRGBAReal* surface,
                                         const GXColor* material,
                                         RpMaterial* owner,
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

typedef struct RpGameCubeVtxFmtSetupData {
    void* resourceEntry;
    RwUInt32 field_0x04;
    RwInt32 flags;
} RpGameCubeVtxFmtSetupData;

typedef struct RwGameCubeDisplayList {
    void* data;
    RwUInt32 size;
} RwGameCubeDisplayList;

typedef struct RwGameCubeVertexArray {
    void* data;
    RwUInt8 attribute;
    RwUInt8 stride;
    RwUInt8 descriptor;
    RwUInt8 reserved_0x07;
} RwGameCubeVertexArray;

typedef struct RwGameCubeVertexStream {
    void* data;
    RwUInt8 reserved_0x04;
    RwUInt8 stride;
    RwUInt8 reserved_0x06[2];
} RwGameCubeVertexStream;

typedef struct RwGameCubeVertexStreams {
    RwUInt8 reserved_0x00[0x0C];
    RwGameCubeVertexStream streams[12];
} RwGameCubeVertexStreams;

typedef struct RwGameCubeVertexData {
    RwUInt32 counts[26];
    const void* source[26];
} RwGameCubeVertexData;

typedef struct RwGameCubeIndexData {
    RwUInt16* attributes[21];
} RwGameCubeIndexData;

typedef struct RwGameCubeVertexBuffer {
    RwUInt32 reserved_0x00[2];
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
    RwUInt8 reserved_0x1D[3];
} RwGameCubeVertexDescriptor;

typedef char RwGameCubeVtxFmtSizeCheck[
    sizeof(RwGameCubeVtxFmt) == 0x18 ? 1 : -1];
typedef char RpGameCubeVtxFmtSizeCheck[
    sizeof(RpGameCubeVtxFmt) == 0x18 ? 1 : -1];
typedef char RwGameCubeVertexDescriptorSizeCheck[
    sizeof(RwGameCubeVertexDescriptor) == 0x20 ? 1 : -1];
typedef char RwGameCubeVertexArraySizeCheck[
    sizeof(RwGameCubeVertexArray) == 8 ? 1 : -1];

void _rxGCResEntryWaitDone(RwResEntry* entry);
void _rxGCInstanceMorphUpdate(RpGeometry* geometry,
                              RwGameCubeVertexBuffer* vertexBuffer,
                              const RpInterpolator* interpolator);
void RwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias,
                             RwInt32 biasClamp, RwInt32 edgeLod,
                             RwInt32 maxAnisotropy);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
void _rwDlTextureSet(RwTexture* texture, int mapid);
void _rwDlTextureRasterFlush(void);
void _rwDlVtxFmtSetup(RpGameCubeVtxFmt* format,
                      RpGameCubeVtxFmtSetupData* setupData);
void _rwDlTransformSetup(const RwMatrix* matrix, RwBool normalize);
void _rwDlRenderStateSetZCompLoc(RwBool beforeTexture);
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
RwUInt32 _rwGCNDisplayListGetStride(
    const RwGameCubeVertexDescriptor* format);
RwUInt32 _rwGCNDisplayListGetSize(
    const RwGameCubeVertexDescriptor* format, RwUInt32 numIndices,
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
RwUInt32 _rwGCNVertexBufferHeaderGetSize(
    const RwGameCubeVertexDescriptor* descriptor);
RwUInt32 _rwGCNVtxFmtInstPos3D(void* destination, const RwV3d* source,
                               RwUInt32 type, RwUInt32 count,
                               RwUInt32 stride, const RwV3d* origin,
                               RwReal scale);
RwUInt32 _rwGCNVtxFmtInstNrm(void* destination, const RwV3d* source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride);
RwUInt32 _rwGCNVtxFmtInstNrmCmp(void* destination, const void* source,
                                RwUInt32 type, RwUInt32 count,
                                RwUInt32 stride);
RwUInt32 _rwGCNVtxFmtInstNBT(void* destination, const RwV3d* source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride);
RwUInt32 _rwGCNVtxFmtInstNBTCmp(void* destination, const void* source,
                                RwUInt32 type, RwUInt32 count,
                                RwUInt32 stride);
RwUInt32 _rwGCNVtxFmtInstClr(void* destination, const RwRGBA* source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride);
RwUInt32 _rwGCNVtxFmtInstTex(void* destination, const RwTexCoords* source,
                             RwUInt32 type, RwUInt32 count, RwUInt32 stride,
                             RwReal scale);
RwUInt32 _rwGCNVertexBufferGetSize(
    const RwGameCubeVertexDescriptor* descriptor,
    const RwUInt32* vertexCounts);
void _rwGCNVertexBufferInitialize(
    const RwGameCubeVertexDescriptor* descriptor,
    RwGameCubeVertexBuffer* vertexBuffer, const RwUInt32* vertexCounts,
    void* data);
void _rwGCNVertexBufferFill(const RwGameCubeVertexDescriptor* descriptor,
                            const RwGameCubeVertexStreams* streams,
                            const RwGameCubeVertexData* vertexData,
                            RwBool compressedNormals, void* remap);
void _rwGCNTriStripGetStats(RwUInt16* indices, RwUInt32 numIndices,
                            RwUInt32* numStrips, RwUInt32* stripIndices,
                            RwBool optimize);
void _rwGCNDisplayListFill(const RwGameCubeVertexDescriptor* descriptor,
                           RwGameCubeDisplayList* displayList,
                           const RwGameCubeIndexData* indexData,
                           RwUInt32 numIndices,
                           RwBool triangleStrip, RwUInt32 stride,
                           RwBool optimize, RwUInt8 primitive,
                           const RwV3d* remap);
RwResEntry* _rwDlGeometrySkinInstanceOptimized(RpGeometry* geometry,
                                               void* owner,
                                               RwResEntry** ownerRef);
RwResEntry* _rwDlGeometrySkinInstanceFast(RpGeometry* geometry, void* owner,
                                          RwResEntry** ownerRef);
RwResEntry* _rwDlGeometryInstanceOptimized(RpGeometry* geometry, void* owner,
                                           RwResEntry** ownerRef);
RwResEntry* _rwDlGeometryInstanceFast(RpGeometry* geometry, void* owner,
                                      RwResEntry** ownerRef);
RwResEntry* _rwDlWorldSectorInstanceOptimized(RpWorld* world,
                                              RpWorldSector* sector);
RwResEntry* _rwDlWorldSectorInstanceFast(RpWorld* world,
                                         RpWorldSector* sector, void* owner,
                                         RwResEntry** ownerRef);

#endif
