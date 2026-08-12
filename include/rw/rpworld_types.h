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
typedef struct RwRGBA RwRGBA;
typedef struct RpMaterial RpMaterial;
typedef unsigned short RxVertexIndex;
typedef struct RpBuildMesh RpBuildMesh;
typedef struct RpMesh RpMesh;
typedef struct RpTriangle RpTriangle;
typedef struct RpSector RpSector;
typedef struct RpLight RpLight;
typedef RpLight* (*RpLightCallBack)(RpLight*, void*);

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

typedef RpMesh* (*RpMeshCallBack)(RpMesh*, RpMeshHeader*, void*);
typedef RpMaterial* (*RpMaterialCallBack)(RpMaterial*, void*);

struct RpTriangle {
    unsigned short vertIndex[3];
    unsigned short matIndex;
};

typedef union RpMaterialColor {
    unsigned int packed;
    struct {
        unsigned char red;
        unsigned char green;
        unsigned char blue;
        unsigned char alpha;
    };
} RpMaterialColor;

typedef struct RpSurfaceProperties {
    float ambient;
    float specular;
    float diffuse;
} RpSurfaceProperties;

typedef struct RpMaterial {
    RwTexture* texture;
    RpMaterialColor color;
    struct RxPipeline* pipeline;
    RpSurfaceProperties surface;
    short refCount;
    unsigned short reserved_0x1A;
} RpMaterial;

typedef RpAtomic* (*RpAtomicCallBackRender)(RpAtomic* atomic);
typedef RpAtomic* (*RpAtomicCallBack)(RpAtomic* atomic, void* data);
typedef RpClump* (*RpClumpCallBack)(RpClump* clump, void* data);

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
RpMeshHeader* _rpMeshHeaderCreate(unsigned int size);
void* _rpMeshClose(void* instance, int offset, int size);
void* _rpMeshOpen(void* instance, int offset, int size);
RpBuildMesh* _rpBuildMeshCreate(unsigned int bufferSize);
int _rpBuildMeshDestroy(RpBuildMesh* mesh);
int _rpMeshDestroy(RpMeshHeader* meshHeader);
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
    union {
        RwLLLink frameLink;
        struct {
            void* inClumpLinkNext;
            void* inClumpLinkPrev;
        };
    };
    void* sync;
    void* repEntry;
    RpGeometry* geometry;
    union {
        RwSphere boundingSphere;
        struct {
            float boundingSphereX;
            float boundingSphereY;
            float boundingSphereZ;
            float boundingSphereRadius;
        };
    };
    RwSphere worldBoundingSphere;
    union {
        RpClump* clump;
        void* lights;
    };
    RwLLLink inClumpLink;
    RpAtomicCallBackRender renderCallBack;
    union {
        RpInterpolator interpolator;
        unsigned int interpolatorFlags;
    };
    unsigned short renderFrame;
    unsigned short reserved62;
    RwLinkList worldSectorsInAtomic;
    RxPipeline* pipeline;
};

static inline RpAtomic* RpAtomicFromClumpLink(RwLLLink* link)
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

typedef struct RpClump {
    RwObject object;
    RwLLLink atomicList;
    RwLLLink lightList;
    union {
        RwLLLink cameraList;
        struct {
            void* atomics;
            void* field_0x1C;
        };
    };
    union {
        RwLLLink inWorldLink;
        struct {
            void* modellingFrame;
            void* field_0x24;
        };
    };
    RpClumpCallBack callback;
    unsigned char field_0x2C[0x74];
    float worldAnchorX;
    float worldAnchorY;
    float worldAnchorZ;
} RpClump;

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic);
void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic);
RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
RpClump* RpClumpRender(RpClump* clump);
RpAtomic* RpAtomicCreate(void);
RpAtomic* RpAtomicSetGeometry(RpAtomic*, RpGeometry*, unsigned int);
int RpAtomicDestroy(RpAtomic*);
RpClump* RpClumpCreate(void);
int RpClumpDestroy(RpClump*);
RpClump* RpClumpAddAtomic(RpClump*, RpAtomic*);
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

RpWorld* RpWorldLock(RpWorld* world);
RpWorld* RpWorldUnlock(RpWorld* world);
RpWorld* RpWorldSectorGetWorld(const RpWorldSector* sector);
int RpWorldDestroy(RpWorld* world);
void RpWorldSetSectorRenderCallBack(RpWorld*, RpWorldSectorCallBackRender);
RpWorld* RpWorldCreate(RwBBox* boundingBox);
RpWorld* RpWorldForAllWorldSectors(RpWorld*, RpWorldSectorCallBack, void*);
RpWorld* RpWorldForAllLights(RpWorld*, RpLightCallBack, void*);
int RpWorldRegisterPlugin(int, unsigned int, RwPluginObjectConstructor,
                              RwPluginObjectDestructor, RwPluginObjectCopy);
int RpWorldPluginAttach(void);

#endif
