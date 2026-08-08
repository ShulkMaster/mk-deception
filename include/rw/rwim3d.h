#ifndef RW_RWIM3D_H
#define RW_RWIM3D_H

#include "rw/rwplcore.h"

typedef struct RxPipeline RxPipeline;
typedef struct RwIm3DVertex RwIm3DVertex;
typedef struct RwMatrix RwMatrix;
typedef RwUInt16 RwImVertexIndex;

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

RwIm3DVertex* RwIm3DTransform(RwIm3DVertex* vertices, RwUInt32 numVertices,
                              const RwMatrix* localToWorld, RwUInt32 flags);
RwBool RwIm3DEnd(void);
RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primitiveType,
                                    const RwImVertexIndex* indices,
                                    RwUInt32 numIndices);
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

#endif
