#ifndef RW_RWIM3D_H
#define RW_RWIM3D_H

#include "rw/rwplcore.h"

typedef struct RwV2d { RwReal x; RwReal y; } RwV2d;

typedef struct RxPipeline RxPipeline;
typedef struct RxNodeDefinition RxNodeDefinition;
typedef struct RwMatrix RwMatrix;
typedef RwUInt16 RwImVertexIndex;

typedef struct RwIm3DVertex {
    RwV3d position;
    RwV3d normal;
    union {
        RwUInt32 color;
        struct {
            RwUInt8 red;
            RwUInt8 green;
            RwUInt8 blue;
            RwUInt8 alpha;
        } color_channels;
    };
    RwV2d texCoords;
} RwIm3DVertex;

typedef enum RwPrimitiveType {
    rwPRIMTYPELINELIST = 1,
    rwPRIMTYPEPOLYLINE = 2,
    rwPRIMTYPETRILIST = 3,
    rwPRIMTYPETRISTRIP = 4,
    rwPRIMTYPETRIFAN = 5,
    rwPRIMTYPEPOINTLIST = 6
} RwPrimitiveType;

typedef struct RwIm3DRenderPipelines {
    RxPipeline* triList;
    RxPipeline* triFan;
    RxPipeline* triStrip;
    RxPipeline* lineList;
    RxPipeline* polyLine;
    RxPipeline* pointList;
} RwIm3DRenderPipelines;

typedef struct RwIm3DTransformData {
    RwUInt16 numVertices;
    RwUInt16 reserved_02;
    RwIm3DVertex* vertices;
    RwUInt32 stride;
} RwIm3DTransformData;

typedef struct RwIm3DRenderData {
    RxPipeline* pipeline;
    RwPrimitiveType primitiveType;
    const RwImVertexIndex* indices;
    RwInt32 numIndices;
} RwIm3DRenderData;

typedef struct RwIm3DStash {
    RwUInt32 flags;
    const RwMatrix* localToWorld;
    RwUInt8 reserved_08[0x18];
    RwIm3DRenderData renderData;
} RwIm3DStash;

RwIm3DVertex* RwIm3DTransform(RwIm3DVertex* vertices, RwUInt32 numVertices,
                              const RwMatrix* localToWorld, RwUInt32 flags);
RwBool RwIm3DEnd(void);
RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primitiveType,
                                    const RwImVertexIndex* indices,
                                    RwInt32 numIndices);
RwBool RwIm3DRenderPrimitive(RwPrimitiveType primitiveType);
RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline);
RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline,
                                    RwPrimitiveType primitiveType);

RwBool _rwIm3DCreatePlatformTransformPipeline(RxPipeline** pipeline);
RwBool _rwIm3DCreatePlatformRenderPipelines(
    RwIm3DRenderPipelines* pipelines);
void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** pipeline);
void _rwIm3DDestroyPlatformRenderPipelines(
    RwIm3DRenderPipelines* pipelines);
RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void);
RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void);

#endif
