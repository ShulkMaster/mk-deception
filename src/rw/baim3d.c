#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwim3d.h"
#include "rw/rwplcore.h"
#include "rw/rxpipeline.h"
#include "runtime/cstring.h"

typedef struct RwIm3DTransformData {
    RwUInt16 numVertices;
    RwUInt16 reserved_0x3A;
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
    RwUInt8 reserved_0x4C[0x18];
    RwIm3DRenderData renderData;
} RwIm3DStash;

typedef struct RwIm3DGlobals {
    RxPipeline* transformPipeline;
    RwIm3DRenderPipelines renderPipelines;
    RxPipeline* defaultTransformPipeline;
    RwIm3DRenderPipelines defaultRenderPipelines;
    RwIm3DTransformData transformData;
    RwIm3DStash stash;
} RwIm3DGlobals;

RwIm3DGlobals* _rwIm3DGlobals;
RwModuleInfo _rwIm3DModule;

#define IM3DGLOBALS \
    ((RwIm3DGlobals*)((RwUInt8*)RwEngineInstance + \
                      _rwIm3DModule.globalsOffset))

extern RxHeap* RxHeapGetGlobalHeap(void);

RwIm3DVertex* RwIm3DTransform(RwIm3DVertex* vertices, RwUInt32 numVertices,
                              const RwMatrix* localToWorld, RwUInt32 flags)
{
    if (numVertices > 0x10000) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x32);
        RwErrorSet(&error);
        return NULL;
    }

    IM3DGLOBALS->transformData.numVertices = numVertices;
    IM3DGLOBALS->transformData.vertices = vertices;
    IM3DGLOBALS->transformData.stride = 0x24;
    IM3DGLOBALS->stash.localToWorld = localToWorld;
    IM3DGLOBALS->stash.flags = flags | 8 | 0x10;
    if (RxPipelineExecute(IM3DGLOBALS->transformPipeline,
                          &IM3DGLOBALS->transformData, TRUE) != NULL) {
        return vertices;
    }
    return NULL;
}

RwBool RwIm3DEnd(void)
{
    RwBool transformed = IM3DGLOBALS->transformData.vertices != NULL;

    if (!transformed) {
        return FALSE;
    }
    memset(&IM3DGLOBALS->transformData, 0, 0x3C);
    return TRUE;
}

RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primitiveType,
                                    const RwImVertexIndex* indices,
                                    RwInt32 numIndices)
{
    RwBool transformed = IM3DGLOBALS->transformData.vertices != NULL;

    if (transformed) {
        RwIm3DStash* data = &IM3DGLOBALS->stash;
        data->renderData.pipeline = NULL;
        data->renderData.primitiveType = primitiveType;
        data->renderData.indices = indices;
        data->renderData.numIndices = numIndices;
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.triList;
            data->renderData.numIndices = numIndices - (numIndices % 3);
            break;
        case rwPRIMTYPETRIFAN:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.triStrip;
            break;
        case rwPRIMTYPELINELIST:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.lineList;
            data->renderData.numIndices = numIndices - (numIndices % 2);
            break;
        case rwPRIMTYPEPOLYLINE:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.polyLine;
            break;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25, primitiveType);
            RwErrorSet(&error);
            break;
        }
        }
        if (RxPipelineExecute(data->renderData.pipeline, data, FALSE) != NULL) {
            return TRUE;
        }
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x23);
        RwErrorSet(&error);
    }
    return FALSE;
}

RwBool RwIm3DRenderPrimitive(RwPrimitiveType primitiveType)
{
    void* vertices = IM3DGLOBALS->transformData.vertices;
    RwBool transformed = vertices != NULL;

    RxHeapGetGlobalHeap();
    if (transformed) {
        RwIm3DStash* data = &IM3DGLOBALS->stash;
        data->renderData.pipeline = NULL;
        data->renderData.primitiveType = primitiveType;
        data->renderData.indices = NULL;
        data->renderData.numIndices = IM3DGLOBALS->transformData.numVertices;
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.triList;
            break;
        case rwPRIMTYPETRIFAN:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.triStrip;
            break;
        case rwPRIMTYPELINELIST:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.lineList;
            break;
        case rwPRIMTYPEPOLYLINE:
            data->renderData.pipeline = IM3DGLOBALS->renderPipelines.polyLine;
            break;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25, primitiveType);
            RwErrorSet(&error);
            break;
        }
        }
        if (RxPipelineExecute(data->renderData.pipeline, data, FALSE) != NULL) {
            return TRUE;
        }
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x23);
        RwErrorSet(&error);
    }
    return FALSE;
}

RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline)
{
    if (pipeline != NULL) {
        IM3DGLOBALS->transformPipeline = pipeline;
    } else if (IM3DGLOBALS->defaultTransformPipeline != NULL) {
        IM3DGLOBALS->transformPipeline = IM3DGLOBALS->defaultTransformPipeline;
    } else {
        IM3DGLOBALS->transformPipeline = NULL;
    }
    return IM3DGLOBALS->transformPipeline;
}

RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline,
                                    RwPrimitiveType primitiveType)
{
    if (pipeline != NULL) {
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            IM3DGLOBALS->renderPipelines.triList = pipeline;
            return pipeline;
        case rwPRIMTYPETRIFAN:
            IM3DGLOBALS->renderPipelines.triFan = pipeline;
            return pipeline;
        case rwPRIMTYPETRISTRIP:
            IM3DGLOBALS->renderPipelines.triStrip = pipeline;
            return pipeline;
        case rwPRIMTYPELINELIST:
            IM3DGLOBALS->renderPipelines.lineList = pipeline;
            return pipeline;
        case rwPRIMTYPEPOLYLINE:
            IM3DGLOBALS->renderPipelines.polyLine = pipeline;
            return pipeline;
        case rwPRIMTYPEPOINTLIST:
            IM3DGLOBALS->renderPipelines.pointList = pipeline;
            return pipeline;
        default:
            break;
        }
    } else {
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            IM3DGLOBALS->renderPipelines.triList =
                IM3DGLOBALS->defaultRenderPipelines.triList;
            return IM3DGLOBALS->renderPipelines.triList;
        case rwPRIMTYPETRIFAN:
            IM3DGLOBALS->renderPipelines.triFan =
                IM3DGLOBALS->defaultRenderPipelines.triFan;
            return IM3DGLOBALS->renderPipelines.triFan;
        case rwPRIMTYPETRISTRIP:
            IM3DGLOBALS->renderPipelines.triStrip =
                IM3DGLOBALS->defaultRenderPipelines.triStrip;
            return IM3DGLOBALS->defaultRenderPipelines.triStrip;
        case rwPRIMTYPELINELIST:
            IM3DGLOBALS->renderPipelines.lineList =
                IM3DGLOBALS->defaultRenderPipelines.lineList;
            return IM3DGLOBALS->defaultRenderPipelines.lineList;
        case rwPRIMTYPEPOLYLINE:
            IM3DGLOBALS->renderPipelines.polyLine =
                IM3DGLOBALS->defaultRenderPipelines.polyLine;
            return IM3DGLOBALS->defaultRenderPipelines.polyLine;
        case rwPRIMTYPEPOINTLIST:
            IM3DGLOBALS->renderPipelines.pointList =
                IM3DGLOBALS->defaultRenderPipelines.pointList;
            return IM3DGLOBALS->defaultRenderPipelines.pointList;
        default:
            break;
        }
    }
    {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x25, primitiveType);
        RwErrorSet(&error);
    }
    return NULL;
}

void* _rwIm3DClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _rwIm3DDestroyPlatformRenderPipelines(
        &IM3DGLOBALS->defaultRenderPipelines);
    _rwIm3DDestroyPlatformTransformPipeline(
        &IM3DGLOBALS->defaultTransformPipeline);
    --_rwIm3DModule.numInstances;
    return instance;
}

void* _rwIm3DOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RwBool result = TRUE;
    _rwIm3DModule.globalsOffset = offset;
    _rwIm3DGlobals = IM3DGLOBALS;
    ++_rwIm3DModule.numInstances;
    memset(IM3DGLOBALS, 0, sizeof(*IM3DGLOBALS));
    if (result) {
        result = _rwIm3DCreatePlatformTransformPipeline(
            &IM3DGLOBALS->defaultTransformPipeline);
    }
    if (result) {
        result = _rwIm3DCreatePlatformRenderPipelines(
            &IM3DGLOBALS->defaultRenderPipelines);
    }
    if (result) {
        return instance;
    }
    _rwIm3DClose(instance, offset, size);
    return NULL;
}
