#ifndef RW_RWIM3D_H
#define RW_RWIM3D_H

#include "rw/rwplcore.h"

typedef struct RwV2d { float x; float y; } RwV2d;

typedef struct RxPipeline RxPipeline;
typedef struct RxNodeDefinition RxNodeDefinition;
typedef struct RwMatrix RwMatrix;
typedef unsigned short RwImVertexIndex;

typedef struct RwIm3DVertex {
    RwV3d position;
    RwV3d normal;
    union {
        unsigned int color;
        struct {
            unsigned char red;
            unsigned char green;
            unsigned char blue;
            unsigned char alpha;
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
    unsigned short numVertices;
    unsigned short reserved_02;
    RwIm3DVertex* vertices;
    unsigned int stride;
} RwIm3DTransformData;

typedef struct RwIm3DRenderData {
    RxPipeline* pipeline;
    RwPrimitiveType primitiveType;
    const RwImVertexIndex* indices;
    int numIndices;
} RwIm3DRenderData;

typedef struct RwIm3DStash {
    unsigned int flags;
    const RwMatrix* localToWorld;
    unsigned char reserved_08[0x18];
    RwIm3DRenderData renderData;
} RwIm3DStash;

RwIm3DVertex* RwIm3DTransform(RwIm3DVertex* vertices, unsigned int numVertices,
                              const RwMatrix* localToWorld, unsigned int flags);
int RwIm3DEnd(void);
int RwIm3DRenderIndexedPrimitive(RwPrimitiveType primitiveType,
                                    const RwImVertexIndex* indices,
                                    int numIndices);
int RwIm3DRenderPrimitive(RwPrimitiveType primitiveType);
RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline);
RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline,
                                    RwPrimitiveType primitiveType);

int _rwIm3DCreatePlatformTransformPipeline(RxPipeline** pipeline);
int _rwIm3DCreatePlatformRenderPipelines(
    RwIm3DRenderPipelines* pipelines);
void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** pipeline);
void _rwIm3DDestroyPlatformRenderPipelines(
    RwIm3DRenderPipelines* pipelines);
RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void);
RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void);

#endif
