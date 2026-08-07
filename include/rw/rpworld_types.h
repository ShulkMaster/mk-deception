#ifndef RW_RPWORLD_TYPES_H
#define RW_RPWORLD_TYPES_H

#include "rw/rwobject.h"
#include "rw/rwcore_types.h"

/*
 * Partial rpworld layouts for game TUs (not full Criterion SDK match).
 * Offsets verified against instance.s / mk_render / mk_obj / gcpipemanager.
 * Extend fields in-place; do not fork a second incompatible RpAtomic.
 */

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

typedef struct RwTexCoords {
    RwReal u;
    RwReal v;
} RwTexCoords;

typedef struct RwSphere {
    float x;
    float y;
    float z;
    float radius;
} RwSphere;

typedef struct RpMeshHeader {
    unsigned int flags;       /* +0x00 */
    unsigned short numMeshes; /* +0x04 */
    unsigned short serialNum; /* +0x06 */
    unsigned int totalIndices; /* +0x08 */
    unsigned int firstMeshOffset; /* +0x0C */
} RpMeshHeader;

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
    RwTexture* texture; /* +0x00 */
    RpMaterialColor color; /* +0x04 -- packed RGBA */
    void* pipeline;     /* +0x08 */
    RpSurfaceProperties surface; /* +0x0C */
} RpMaterial;

typedef RpAtomic* (*RpAtomicCallBackRender)(RpAtomic* atomic);

/* Stock RpMaterialList embedded in RpGeometry at +0x20. */
typedef struct RpMaterialList {
    RpMaterial** materials; /* +0x00 */
    int numMaterials;       /* +0x04 */
    int space;              /* +0x08 */
} RpMaterialList;

/*
 * RpAtomic -- Midway/game-used fields.
 * object.flags @ +0x02 is the hide/show / stream flags byte.
 * boundingSphere @ +0x1C (shadow init); interpolatorFlags @ +0x4C bit1 = resync.
 * inClumpLink @ +0x40 (ShadowCameraUpdate walk: node = atomic+0x40).
 */
struct RpAtomic {
    RwObject object;                       /* +0x00 */
    union {
        RwLLLink frameLink;                /* +0x08 -- RwObjectHasFrame link */
        struct {
            void* inClumpLinkNext;         /* legacy next-link name */
            void* inClumpLinkPrev;
        };
    };
    void* sync;                            /* +0x10 */
    void* repEntry;                        /* +0x14 */
    RpGeometry* geometry;                  /* +0x18 */
    float boundingSphereX;                 /* +0x1C */
    float boundingSphereY;                 /* +0x20 */
    float boundingSphereZ;                 /* +0x24 */
    float boundingSphereRadius;            /* +0x28 */
    RwSphere worldBoundingSphere;          /* +0x2C */
    void* lights;                          /* +0x3C -- Midway: RpClump* (Mkobj plugin host) */
    RwLLLink inClumpLink;                  /* +0x40 */
    RpAtomicCallBackRender renderCallBack; /* +0x48 */
    unsigned int interpolatorFlags;        /* +0x4C -- bit 0x2 = needs sphere resync */
    char pad50[0x1C];
    void* pipeline;                        /* +0x6C */
};

#define RP_ATOMIC_FROM_CLUMP_LINK(link)                                  \
    ((RpAtomic*)((unsigned char*)(link) - 0x40))

/*
 * RpGeometry -- inplaceGeometryCreate / stream (instance.s).
 *
 * Create args: (numVertices, numTriangles, flags)
 *   -> numTriangles @ +0x10, numVertices @ +0x14 (stock RW order).
 */
typedef struct RpGeometry {
    RwObject object;                /* +0x00 type=8 */
    int flags;                      /* +0x08 */
    unsigned short lockedSinceLastInst; /* +0x0C */
    unsigned short refCount;        /* +0x0E */
    int numTriangles;               /* +0x10 */
    int numVertices;                /* +0x14 */
    int numMorphTargets;            /* +0x18 */
    int numTexCoordSets;            /* +0x1C */
    RpMaterialList matList;         /* +0x20 */
    void* triangles;                /* +0x2C */
    void* preLitLum;                /* +0x30 */
    void* texCoords[8];             /* +0x34 */
    RpMeshHeader* meshHeader;       /* +0x54 */
    RwResEntry* repEntry;           /* +0x58 */
    struct RpMorphTarget* morphTarget; /* +0x5C */
} RpGeometry;

/* Morph target -- 0x1C stride (inplaceGeometryAddMorphTargets). */
typedef struct RpMorphTarget {
    RpGeometry* parentGeom; /* +0x00 */
    RwSphere sphere;        /* +0x04 */
    void* verts;            /* +0x14 -- inplace stream fixup */
    void* normals;          /* +0x18 */
} RpMorphTarget;

typedef struct RpClump {
    RwObject object; /* +0x00 */
    RwLLLink atomicList; /* +0x08 -- ShadowCameraUpdate sentinel/walk */
    char pad10[0x08];
    void* atomics;   /* +0x18 -- init_shadow ForAllAtomics arg */
    char pad1C[0x04];
    void* modellingFrame; /* +0x20 -- LTM parent (shadow init) */
    char pad24[0x7C];
    float worldAnchorX; /* +0xA0 -- shadow ground-plane transform src */
    float worldAnchorY; /* +0xA4 */
    float worldAnchorZ; /* +0xA8 */
} RpClump;

typedef struct RpPolygon {
    RwUInt16 matIndex;
    RwUInt16 vertIndex[3];
} RpPolygon;

struct RpWorldSector {
    RwInt32 type;
    RpPolygon* polygons;
    RwV3d* vertices;
    void* normals;
    /* This SDK build embeds six world-sector texture-coordinate sets. */
    RwTexCoords* texCoords[6];
    RwRGBA* preLitLum;
    RwResEntry* repEntry;
    RwLinkList collAtomicsInWorldSector;
    RwLinkList noCollAtomicsInWorldSector;
    RwLinkList lightsInWorldSector;
    RwBBox boundingBox;
    RwBBox tightBoundingBox;
    RpMeshHeader* mesh;
    RxPipeline* pipeline;
    RwUInt16 matListWindowBase;
    RwUInt16 numVertices;
    RwUInt16 numPolygons;
    RwUInt16 pad;
};

typedef enum RpWorldRenderOrder {
    rpWORLDRENDERNARENDERORDER = 0,
    rpWORLDRENDERFRONT2BACK,
    rpWORLDRENDERBACK2FRONT
} RpWorldRenderOrder;

typedef struct RpSector RpSector;
typedef RpWorldSector* (*RpWorldSectorCallBackRender)(RpWorldSector* worldSector);

struct RpWorld {
    RwObject object;
    RwUInt32 flags;
    RpWorldRenderOrder renderOrder;
    RpMaterialList matList;
    RpSector* rootSector;
    RwInt32 numTexCoordSets;
    RwInt32 numClumpsInWorld;
    RwLLLink* currentClumpLink;
    RwLinkList clumpList;
    RwLinkList lightList;
    RwLinkList directionalLightList;
    RwV3d worldOrigin;
    RwBBox boundingBox;
    RpWorldSectorCallBackRender renderCallBack;
    RxPipeline* pipeline;
};

#endif
