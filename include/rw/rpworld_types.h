#ifndef RW_RPWORLD_TYPES_H
#define RW_RPWORLD_TYPES_H

#include "rw/rwobject.h"
#include "rw/rwcore_types.h"







typedef struct RpGeometry RpGeometry;
typedef struct RpClump RpClump;
typedef struct RwFrame RwFrame;
typedef struct RwResEntry RwResEntry;
typedef struct RpAtomic RpAtomic;
typedef struct RwCamera RwCamera;
typedef struct RpWorld RpWorld;
typedef struct RpWorldSector RpWorldSector;
typedef struct RxPipeline RxPipeline;
typedef struct RpMaterial RpMaterial;
typedef unsigned short RxVertexIndex;
typedef struct RpBuildMesh RpBuildMesh;
typedef struct RpMesh RpMesh;
typedef struct RpTriangle RpTriangle;
typedef struct RpSector RpSector;
typedef struct RpLight RpLight;
typedef struct RpTie RpTie;
typedef struct RpLightTie RpLightTie;
typedef RpLight* (*RpLightCallBack)(RpLight*, void*);

struct RpTie {
    RwLLLink lAtomicInWorldSector;
    RpAtomic* atomic;
    RwLLLink lWorldSectorInAtomic;
    RpWorldSector* worldSector;
};

struct RpLightTie {
    RwLLLink lightInWorldSector;
    RpLight* light;
    RwLLLink worldSectorInLight;
    RpWorldSector* worldSector;
};

int _rpTieDestroy(RpTie* tie);
int _rpLightTieDestroy(RpLightTie* tie);

typedef struct RwTexCoords {
    float u;
    float v;
} RwTexCoords;

typedef struct RpMeshHeader {
    unsigned int flags;
    unsigned short numMeshes;
    unsigned short serialNum;
    unsigned int totalIndices;
    unsigned int firstMeshOffset;
} RpMeshHeader;
typedef char RpMeshHeaderSizeCheck[sizeof(RpMeshHeader) == 0x10 ? 1 : -1];

typedef struct RpBuildMeshTriangle {
    RxVertexIndex vertIndex[3];
    RpMaterial* material;
    unsigned short matIndex;
    unsigned short textureIndex;
    unsigned short rasterIndex;
    unsigned short pipelineIndex;
} RpBuildMeshTriangle;

struct RpBuildMesh {
    unsigned int triangleBufferSize;
    unsigned int numTriangles;
    RpBuildMeshTriangle* meshTriangles;
};

struct RpMesh {
    RxVertexIndex* indices;
    unsigned int numIndices;
    RpMaterial* material;
};
typedef char RpMeshSizeCheck[sizeof(RpMesh) == 0x0C ? 1 : -1];

typedef RpMesh* (*RpMeshCallBack)(RpMesh*, RpMeshHeader*, void*);
typedef RpMaterial* (*RpMaterialCallBack)(RpMaterial*, void*);

struct RpTriangle {
    unsigned short vertIndex[3];
    unsigned short matIndex;
};
typedef char RpTriangleSizeCheck[sizeof(RpTriangle) == 0x08 ? 1 : -1];

typedef struct RpSurfaceProperties {
    float ambient;
    float specular;
    float diffuse;
} RpSurfaceProperties;

typedef struct RpMaterial {
    RwTexture* texture;
    RwRGBA color;
    struct RxPipeline* pipeline;
    RpSurfaceProperties surface;
    short refCount;
    short pad;
} RpMaterial;
typedef char RpMaterialSizeCheck[sizeof(RpMaterial) == 0x1C ? 1 : -1];

typedef RpAtomic* (*RpAtomicCallBackRender)(RpAtomic* atomic);
typedef RpAtomic* (*RpAtomicCallBack)(RpAtomic* atomic, void* data);
typedef RpClump* (*RpClumpCallBack)(RpClump* clump, void* data);

int RpAtomicRegisterPlugin(
    int size, unsigned int pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int RpAtomicRegisterPluginStream(
    unsigned int pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB);
int RpAtomicSetStreamAlwaysCallBack(
    unsigned int pluginID, RwPluginDataChunkAlwaysCallBack callback);
int RpAtomicSetStreamRightsCallBack(
    unsigned int pluginID, RwPluginDataChunkRightsCallBack callback);
int RpAtomicGetPluginOffset(unsigned int pluginID);
int RpClumpRegisterPlugin(
    int size, unsigned int pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
int RpClumpRegisterPluginStream(
    unsigned int pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB);

typedef struct RpInterpolator {
    int flags;
    short startMorphTarget;
    short endMorphTarget;
    float time;
    float recipTime;
    float position;
} RpInterpolator;


typedef struct RpMaterialList {
    RpMaterial** materials;
    int numMaterials;
    int space;
} RpMaterialList;
typedef char RpMaterialListSizeCheck[
    sizeof(RpMaterialList) == 0x0C ? 1 : -1];

RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* materialList);
RpMaterialList* _rpMaterialListInitialize(RpMaterialList* materialList);
RpMaterial* _rpMaterialListGetMaterial(const RpMaterialList* materialList,
                                       int index);
RpMaterialList* _rpMaterialListSetSize(RpMaterialList* materialList,
                                       int size);
int _rpMaterialListAppendMaterial(RpMaterialList* materialList,
                                      RpMaterial* material);
int _rpMaterialListFindMaterialIndex(const RpMaterialList* materialList,
                                         const RpMaterial* material);
int RpMaterialDestroy(RpMaterial* material);
RpMaterial* RpMaterialCreate(void);
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);
RpMaterial* RpMaterialStreamRead(RwStream* stream);
void* _rpMaterialOpen(void* instance, int offset, int size);
void* _rpMaterialClose(void* instance, int offset, int size);
RpMeshHeader* _rpMeshHeaderCreate(unsigned int size);
void* _rpMeshClose(void* instance, int offset, int size);
void* _rpMeshOpen(void* instance, int offset, int size);
void* _rpGeometryOpen(void* instance, int offset, int size);
void* _rpGeometryClose(void* instance, int offset, int size);
void* _rpClumpOpen(void* instance, int offset, int size);
void* _rpClumpClose(void* instance, int offset, int size);
void* _rpSectorOpen(void* instance, int offset, int size);
void* _rpSectorClose(void* instance, int offset, int size);
void* _rpBinaryWorldOpen(void* instance, int offset, int size);
void* _rpBinaryWorldClose(void* instance, int offset, int size);
int _rpWorldObjRegisterExtensions(void);
int _rpClumpRegisterExtensions(void);
RpBuildMesh* _rpBuildMeshCreate(unsigned int bufferSize);
int _rpBuildMeshDestroy(RpBuildMesh* mesh);
int _rpMeshDestroy(RpMeshHeader* meshHeader);
RpMeshHeader* _rpMeshOptimise(RpBuildMesh* mesh, int flags);
RpBuildMesh* _rpBuildMeshAddTriangle(
    RpBuildMesh* mesh, RpMaterial* material, int vert1, int vert2,
    int vert3, unsigned short matIndex, unsigned short textureIndex,
    unsigned short rasterIndex, unsigned short pipelineIndex);
RpMeshHeader* _rpMeshHeaderForAllMeshes(RpMeshHeader* meshHeader,
                                        RpMeshCallBack callback, void* data);
RwStream* _rpMeshWrite(const RpMeshHeader* meshHeader, const void* object,
                       RwStream* stream, const RpMaterialList* materialList);
RpMeshHeader* _rpMeshRead(RwStream* stream, const void* object,
                          const RpMaterialList* materialList);
int _rpMeshSize(const RpMeshHeader* meshHeader, const void* object);







struct RpAtomic {
    RwObject object;
    RwLLLink frameLink;
    RwObjectHasFrameSyncFunction sync;
    RwResEntry* repEntry;
    RpGeometry* geometry;
    RwSphere boundingSphere;
    RwSphere worldBoundingSphere;
    RpClump* clump;
    RwLLLink inClumpLink;
    RpAtomicCallBackRender renderCallBack;
    RpInterpolator interpolator;
    unsigned short renderFrame;
    unsigned short reserved62;
    RwLinkList worldSectorsInAtomic;
    RxPipeline* pipeline;
};
typedef char RpAtomicSizeCheck[sizeof(RpAtomic) == 0x70 ? 1 : -1];

static inline RpAtomic* rpAtomicFromClumpNode(RwLLLink* link)
{
    return (RpAtomic*)((unsigned char*)link - 0x40);
}







typedef struct RpGeometry {
    RwObject object;
    unsigned int flags;
    unsigned short lockedSinceLastInst;
    short refCount;
    int numTriangles;
    int numVertices;
    int numMorphTargets;
    int numTexCoordSets;
    RpMaterialList matList;
    RpTriangle* triangles;
    void* preLitLum;
    void* texCoords[8];
    RpMeshHeader* meshHeader;
    RwResEntry* repEntry;
    struct RpMorphTarget* morphTarget;
} RpGeometry;
typedef char RpGeometrySizeCheck[sizeof(RpGeometry) == 0x60 ? 1 : -1];

int RpGeometryAddMorphTargets(RpGeometry* geometry, int count);
int RpGeometryAddMorphTarget(RpGeometry* geometry);
RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry,
                                      RpMaterialCallBack callback, void* data);
RpGeometry* RpGeometryLock(RpGeometry* geometry, int lockMode);
RpGeometry* RpGeometryUnlock(RpGeometry* geometry);
RpGeometry* RpGeometryCreate(int numVertices, int numTriangles,
                             unsigned int format);
RpGeometry* _rpGeometryAddRef(RpGeometry* geometry);
int RpGeometryDestroy(RpGeometry* geometry);
int RpGeometryRegisterPlugin(int size, unsigned int pluginID,
                                 RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB,
                                 RwPluginObjectCopy copyCB);
int RpGeometryRegisterPluginStream(
    unsigned int pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB);
RpGeometry* RpGeometryStreamRead(RwStream* stream);


typedef struct RpMorphTarget {
    RpGeometry* parentGeom;
    RwSphere sphere;
    void* verts;
    void* normals;
} RpMorphTarget;
typedef char RpMorphTargetSizeCheck[
    sizeof(RpMorphTarget) == 0x1C ? 1 : -1];

typedef struct RpClump {
    RwObject object;
    RwLLLink atomicList;
    RwLLLink lightList;
    RwLLLink cameraList;
    RwLLLink inWorldLink;
    RpClumpCallBack callback;
} RpClump;
typedef char RpClumpSizeCheck[sizeof(RpClump) == 0x2C ? 1 : -1];

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic);
void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic);
RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
RpWorld* RpAtomicGetWorld(const RpAtomic* atomic);
RpClump* RpClumpRender(RpClump* clump);
RpAtomic* RpAtomicCreate(void);
RpAtomic* RpAtomicSetGeometry(RpAtomic*, RpGeometry*, unsigned int);
RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame);
int RpAtomicDestroy(RpAtomic*);
RpClump* RpClumpCreate(void);
int RpClumpDestroy(RpClump*);
RpClump* RpClumpAddAtomic(RpClump*, RpAtomic*);
RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback,
                              void* data);
RpClump* RpClumpRemoveLight(RpClump*, RpLight*);
RpClump* RpClumpRemoveCamera(RpClump*, RwCamera*);

typedef struct RpVertexNormal {
    signed char x, y, z;
    unsigned char pad;
} RpVertexNormal;

struct RpSector {
    int type;
};

typedef struct RpPlaneSector {
    int type;
    float value;
    RpSector* leftSubTree;
    RpSector* rightSubTree;
    float leftValue;
    float rightValue;
} RpPlaneSector;

struct RpWorldSector {
    int type;
    RpTriangle* triangles;
    RwV3d* vertices;
    RpVertexNormal* normals;
    RwTexCoords* texCoords[8];
    RwRGBA* preLitLum;
    RwResEntry* repEntry;
    RwLinkList collAtomicsInWorldSector;
    RwLinkList lightsInWorldSector;
    RwBBox boundingBox;
    RwBBox tightBoundingBox;
    RpMeshHeader* mesh;
    RxPipeline* pipeline;
    unsigned short matListWindowBase;
    unsigned short numVertices;
    unsigned short numTriangles;
    unsigned short pad;
};

typedef enum RpWorldRenderOrder {
    rpWORLDRENDERNARENDERORDER = 0,
    rpWORLDRENDERFRONT2BACK,
    rpWORLDRENDERBACK2FRONT
} RpWorldRenderOrder;

typedef RpWorldSector* (*RpWorldSectorCallBackRender)(RpWorldSector* worldSector);
typedef RpWorldSector* (*RpWorldSectorCallBack)(RpWorldSector*, void*);

struct RpWorld {
    RwObject object;
    unsigned int flags;
    RpWorldRenderOrder renderOrder;
    RpMaterialList matList;
    RpSector* rootSector;
    int numTexCoordSets;
    int numClumpsInWorld;
    RwLLLink* currentClumpLink;
    RwLinkList clumpList;
    RwLinkList lightList;
    RwLinkList directionalLightList;
    RwV3d worldOrigin;
    RwBBox boundingBox;
    RpWorldSectorCallBackRender renderCallBack;
    RxPipeline* pipeline;
};

typedef char RpWorldDirectionalLightListOffsetCheck[
    RW_OFFSET_OF(RpWorld, directionalLightList) == 0x3C ? 1 : -1];
typedef char RpWorldSizeCheck[sizeof(RpWorld) == 0x70 ? 1 : -1];

RpWorld* RpWorldLock(RpWorld* world);
RpWorld* RpWorldUnlock(RpWorld* world);
RpWorld* RpWorldSectorGetWorld(const RpWorldSector* sector);
int RpWorldDestroy(RpWorld* world);
void RpWorldSetSectorRenderCallBack(RpWorld*, RpWorldSectorCallBackRender);
RpWorld* RpWorldCreate(RwBBox* boundingBox);
RpWorld* RpWorldAddAtomic(RpWorld* world, RpAtomic* atomic);
RpWorld* RpWorldRemoveAtomic(RpWorld* world, RpAtomic* atomic);
RpWorld* RpWorldAddClump(RpWorld* world, RpClump* clump);
RpWorld* RpWorldRemoveClump(RpWorld* world, RpClump* clump);
RpWorld* RpWorldForAllWorldSectors(RpWorld*, RpWorldSectorCallBack, void*);
RpWorld* RpWorldForAllLights(RpWorld*, RpLightCallBack, void*);
int RpWorldRegisterPlugin(int, unsigned int, RwPluginObjectConstructor,
                              RwPluginObjectDestructor, RwPluginObjectCopy);
int RpWorldPluginAttach(void);

#endif
