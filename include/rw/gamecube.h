#ifndef RW_GAMECUBE_H
#define RW_GAMECUBE_H

#include "dolphin/gx.h"
#include "rw/gamecube_globals.h"
#include "rw/rpworld_types.h"

typedef struct RpSkin RpSkin;
typedef struct RwResEntry RwResEntry;
typedef struct RpInterpolator RpInterpolator;
typedef struct RpWorld RpWorld;
typedef struct RpWorldSector RpWorldSector;
typedef struct RwMatrix RwMatrix;

typedef void (*RwDlObjectRenderCallBack)(const RwRGBAReal* surface,
                                         const GXColor* material,
                                         RpMaterial* owner,
                                         float intensity);

typedef struct RwGameCubeVtxFmt {
    unsigned int reserved_0x00;
    unsigned int vatA;
    unsigned int reserved_0x08[2];
    unsigned int vcdLo;
    unsigned int vcdHi;
} RwGameCubeVtxFmt;

typedef struct RpGameCubeVtxFmt {
    unsigned char positionType;
    unsigned char normalType;
    unsigned char texCoordType[8];
    unsigned char colorType;
    unsigned char field_0x0B;
    unsigned char positionFraction;
    unsigned char normalMode;
    unsigned char texCoordFraction[8];
    unsigned short refCount;
} RpGameCubeVtxFmt;

typedef struct RpGameCubeVtxFmtSetupData {
    RwResEntry* resourceEntry;
    unsigned int field_0x04;
    int flags;
} RpGameCubeVtxFmtSetupData;

typedef struct RwGameCubeDisplayList {
    void* data;
    unsigned int size;
} RwGameCubeDisplayList;

typedef struct RwDlStateCache {
    int zWriteEnable;
    int zTestEnable;
    int zCompare;
    int cullMode;
    int fogEnable;
    int fogType;
    unsigned int fogColor;
    GXColor gxFogColor;
    int fogDensity;
    float fogStart;
    float fogEnd;
    float fogNear;
    float fogFar;
    int srcBlend;
    int dstBlend;
    int zCompLoc;
    int alphaCompare0;
    int alphaCompare1;
    int alphaOperation;
    unsigned char alphaRef0;
    unsigned char alphaRef1;
    unsigned char alphaMode;
    unsigned char reserved_4F;
} RwDlStateCache;

extern RwDlStateCache _RwDlStateCache;

typedef struct RwGameCubeVertexArray {
    void* data;
    unsigned char attribute;
    unsigned char stride;
    unsigned char descriptor;
    unsigned char reserved_0x07;
} RwGameCubeVertexArray;

typedef struct RwGameCubeVertexData {
    unsigned int counts[26];
    const void* source[26];
} RwGameCubeVertexData;

typedef struct RwGameCubeIndexData {
    unsigned short* attributes[21];
} RwGameCubeIndexData;

typedef struct RwGameCubeVertexBuffer {
    unsigned short displayListToken;
    unsigned short meshSerialNum;
    unsigned int flags; /* Bit 0: vertex colors contain alpha. */
    unsigned int numArrays;
    RwGameCubeVertexArray arrays[1];
} RwGameCubeVertexBuffer;

typedef struct RwGameCubeVertexDescriptor {
    unsigned int vat;
    unsigned int vatA;
    unsigned int vatB;
    unsigned int vatC;
    unsigned int vcdLo;
    unsigned int vcdHi;
    /* Packed counts: colors [1:0], normal/NBT [3:2], texcoords [7:4]. */
    unsigned int attributeCounts;
    unsigned char numIndexedAttrs;
    unsigned char pad_0x1D[3]; /* Four-byte structure alignment. */
} RwGameCubeVertexDescriptor;

typedef char RwGameCubeVtxFmtSizeCheck[
    sizeof(RwGameCubeVtxFmt) == 0x18 ? 1 : -1];
typedef char RpGameCubeVtxFmtSizeCheck[
    sizeof(RpGameCubeVtxFmt) == 0x18 ? 1 : -1];
typedef char RwGameCubeVertexDescriptorSizeCheck[
    sizeof(RwGameCubeVertexDescriptor) == 0x20 ? 1 : -1];
typedef char RwGameCubeVertexArraySizeCheck[
    sizeof(RwGameCubeVertexArray) == 8 ? 1 : -1];
typedef char RwGameCubeDisplayListSizeCheck[
    sizeof(RwGameCubeDisplayList) == 8 ? 1 : -1];
typedef char RpGameCubeVtxFmtSetupDataSizeCheck[
    sizeof(RpGameCubeVtxFmtSetupData) == 0x0C ? 1 : -1];
typedef char RwGameCubeVertexDataSizeCheck[
    sizeof(RwGameCubeVertexData) == 0xD0 ? 1 : -1];
typedef char RwGameCubeIndexDataSizeCheck[
    sizeof(RwGameCubeIndexData) == 0x54 ? 1 : -1];
typedef char RwGameCubeVertexBufferSizeCheck[
    sizeof(RwGameCubeVertexBuffer) == 0x14 ? 1 : -1];

unsigned int rwGCNPosGetSize(
    const RwGameCubeVertexDescriptor* descriptor);
unsigned int rwGCNNrmGetSize(
    const RwGameCubeVertexDescriptor* descriptor);
unsigned int rwGCNClrGetSize(
    const RwGameCubeVertexDescriptor* descriptor, unsigned char colorIndex);
unsigned int rwGCNTexGetSize(
    const RwGameCubeVertexDescriptor* descriptor,
    unsigned char texCoordIndex);
void _rxGCResEntryWaitDone(RwResEntry* entry);
void _rxGCInstanceMorphUpdate(RpGeometry* geometry,
                              RwGameCubeVertexBuffer* vertexBuffer,
                              const RpInterpolator* interpolator);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
void _rwDlTextureRasterFlush(void);
void RwGameCubeCameraTextureFlush(RwRaster* raster, int generateMipmaps);
void _rwDlVtxFmtSetup(RpGameCubeVtxFmt* format,
                      RpGameCubeVtxFmtSetupData* setupData);
void _rwDlTransformSetup(const RwMatrix* matrix, int normalize);
void _rwDlRenderStateSetZCompLoc(int beforeTexture);
int _rwDlGetRenderState(int state, void* value);
int _rwDlSetRenderState(int state, void* value);
int _rwDlRenderStateFogEnable(unsigned int enable);
void _rwDlSetRenderStateSrcDestBlend(int source, int destination);
RwDlObjectRenderCallBack _rwDlObjectRenderSetup(unsigned int flags,
                                                 unsigned int lightMask,
                                                 int textureMode,
                                                 int useAmbient);
RpGameCubeVtxFmt* _rpGameCubeVtxFmtGetDefault(void);
int _rpDlVtxFmtPluginAttach(void);
void RpGameCubeVtxFmtSetPosition(RpGameCubeVtxFmt* format, unsigned int type,
                                 unsigned char fraction);
void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* format, unsigned int type,
                               unsigned int mode);
void RpGameCubeVtxFmtSetTexCoord(RpGameCubeVtxFmt* format, int index,
                                 unsigned int type, unsigned char fraction);
void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* format);
RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void);
void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* format);
void RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry,
                                 RpGameCubeVtxFmt* format);
unsigned int _rwGCNDisplayListGetStride(
    const RwGameCubeVertexDescriptor* format);
unsigned int _rwGCNDisplayListGetSize(
    const RwGameCubeVertexDescriptor* format, unsigned int numIndices,
                                  unsigned int numVertices);
void _rwGCNDisplayListInitialize(RwGameCubeDisplayList* displayList,
                                 unsigned int index, unsigned int size, void* data);
void _rwVertexDescriptorInit(RwGameCubeVertexDescriptor* descriptor);
void _rwGCNVertexDescSetVAT(RwGameCubeVertexDescriptor* descriptor,
                            unsigned int vat);
void _rwGCNVertexDescSetElementAttr(RwGameCubeVertexDescriptor* descriptor,
                                    unsigned int attr, int componentCount,
                                    unsigned int componentType, unsigned char fraction);
void _rwGCNVertexDescSetElementDesc(RwGameCubeVertexDescriptor* descriptor,
                                    unsigned int attr, int type);
void _rwGCNVertexDescSetNumIndexedAttr(
    RwGameCubeVertexDescriptor* descriptor, unsigned char count);
unsigned int _rwGCNVertexBufferHeaderGetSize(
    const RwGameCubeVertexDescriptor* descriptor);
unsigned int _rwGCNVtxFmtInstPos3D(void* destination, const RwV3d* source,
                               int type, int count,
                               unsigned int stride, const RwV3d* origin,
                               float scale);
unsigned int _rwGCNVtxFmtInstNrm(void* destination, const RwV3d* source,
                             int type, int count, unsigned int stride);
unsigned int _rwGCNVtxFmtInstNrmCmp(void* destination, const char* source,
                                int type, int count,
                                unsigned int stride);
unsigned int _rwGCNVtxFmtInstNBT(void* destination, const RwV3d* source,
                             int type, int count, unsigned int stride);
unsigned int _rwGCNVtxFmtInstNBTCmp(void* destination, const char* source,
                                int type, int count,
                                unsigned int stride);
unsigned int _rwGCNVtxFmtInstClr(void* destination, const RwRGBA* source,
                             int type, int count, unsigned int stride);
unsigned int _rwGCNVtxFmtInstTex(void* destination, const RwTexCoords* source,
                             int type, int count, unsigned int stride,
                             float scale);
unsigned int _rwGCNVertexBufferGetSize(
    const RwGameCubeVertexDescriptor* descriptor,
    const unsigned int* vertexCounts);
void _rwGCNVertexBufferInitialize(
    const RwGameCubeVertexDescriptor* descriptor,
    RwGameCubeVertexBuffer* vertexBuffer, const unsigned int* vertexCounts,
    void* data);
void _rwGCNVertexBufferFill(const RwGameCubeVertexDescriptor* descriptor,
                            const RwGameCubeVertexBuffer* vertexBuffer,
                            const RwGameCubeVertexData* vertexData,
                            int compressedNormals, const RwV3d* remap);
void _rwGCNTriStripGetStats(unsigned short* indices, unsigned int numIndices,
                            unsigned int* numStrips, unsigned int* stripIndices,
                            int optimize);
void _rwGCNDisplayListFill(const RwGameCubeVertexDescriptor* descriptor,
                           RwGameCubeDisplayList* displayList,
                           const RwGameCubeIndexData* indexData,
                           unsigned int numIndices,
                           int triangleStrip, unsigned int stride,
                           int optimize, unsigned char primitive,
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
