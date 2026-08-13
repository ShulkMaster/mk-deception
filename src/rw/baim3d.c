#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwim3d.h"
#include "rw/rwplcore.h"
#include "rw/rxpipeline.h"
#include "runtime/cstring.h"

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

static RwIm3DGlobals* Im3DGlobals(void)
{
    return (RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                            _rwIm3DModule.globalsOffset);
}

RwIm3DVertex* RwIm3DTransform(RwIm3DVertex* vertices, unsigned int numVertices,
                              const RwMatrix* localToWorld, unsigned int flags)
{
    if (numVertices > 0x10000) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x32);
        RwErrorSet(&error);
    } else {
        void* result;
        Im3DGlobals()->transformData.numVertices = numVertices;
        Im3DGlobals()->transformData.vertices = vertices;
        Im3DGlobals()->transformData.stride = 0x24;
        Im3DGlobals()->stash.localToWorld = localToWorld;
        Im3DGlobals()->stash.flags = flags | 8 | 0x10;
        result = RxPipelineExecute(Im3DGlobals()->transformPipeline,
                                   &Im3DGlobals()->transformData, 1);
        if (result != 0) {
            return vertices;
        }
    }
    return 0;
}

int RwIm3DEnd(void)
{
    int transformed = Im3DGlobals()->transformData.vertices != 0;

    if (!transformed) {
        return 0;
    }
    memset(&Im3DGlobals()->transformData, 0, 0x3C);
    return 1;
}

int RwIm3DRenderIndexedPrimitive(RwPrimitiveType primitiveType,
                                    const RwImVertexIndex* indices,
                                    int numIndices)
{
    int transformed = Im3DGlobals()->transformData.vertices != 0;

    if (transformed) {
        RwIm3DStash* data = &Im3DGlobals()->stash;
        data->renderData.pipeline = 0;
        data->renderData.primitiveType = primitiveType;
        data->renderData.indices = indices;
        data->renderData.numIndices = numIndices;
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.triList;
            data->renderData.numIndices = numIndices - (numIndices % 3);
            break;
        case rwPRIMTYPETRIFAN:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.triStrip;
            break;
        case rwPRIMTYPELINELIST:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.lineList;
            data->renderData.numIndices = numIndices - (numIndices % 2);
            break;
        case rwPRIMTYPEPOLYLINE:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.polyLine;
            break;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25, primitiveType);
            RwErrorSet(&error);
            break;
        }
        }
        if (RxPipelineExecute(data->renderData.pipeline, data, 0) != 0) {
            return 1;
        }
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x23);
        RwErrorSet(&error);
    }
    return 0;
}

int RwIm3DRenderPrimitive(RwPrimitiveType primitiveType)
{
    void* vertices = Im3DGlobals()->transformData.vertices;
    int transformed = vertices != 0;

    if (transformed) {
        RwIm3DStash* data = &Im3DGlobals()->stash;
        data->renderData.pipeline = 0;
        data->renderData.primitiveType = primitiveType;
        data->renderData.indices = 0;
        data->renderData.numIndices = Im3DGlobals()->transformData.numVertices;
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.triList;
            break;
        case rwPRIMTYPETRIFAN:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.triStrip;
            break;
        case rwPRIMTYPELINELIST:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.lineList;
            break;
        case rwPRIMTYPEPOLYLINE:
            data->renderData.pipeline = Im3DGlobals()->renderPipelines.polyLine;
            break;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25, primitiveType);
            RwErrorSet(&error);
            break;
        }
        }
        if (RxPipelineExecute(data->renderData.pipeline, data, 0) != 0) {
            return 1;
        }
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x23);
        RwErrorSet(&error);
    }
    return 0;
}

RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline)
{
    if (pipeline != 0) {
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
            _rwIm3DModule.globalsOffset))->transformPipeline = pipeline;
    } else if (((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                   _rwIm3DModule.globalsOffset))->defaultTransformPipeline != 0) {
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
            _rwIm3DModule.globalsOffset))->transformPipeline =
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                _rwIm3DModule.globalsOffset))->defaultTransformPipeline;
    } else {
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
            _rwIm3DModule.globalsOffset))->transformPipeline = 0;
    }
    return ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
        _rwIm3DModule.globalsOffset))->transformPipeline;
}

RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline,
                                    RwPrimitiveType primitiveType)
{
    if (pipeline != 0) {
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            Im3DGlobals()->renderPipelines.triList = pipeline;
            return pipeline;
        case rwPRIMTYPETRIFAN:
            Im3DGlobals()->renderPipelines.triFan = pipeline;
            return pipeline;
        case rwPRIMTYPETRISTRIP:
            Im3DGlobals()->renderPipelines.triStrip = pipeline;
            return pipeline;
        case rwPRIMTYPELINELIST:
            Im3DGlobals()->renderPipelines.lineList = pipeline;
            return pipeline;
        case rwPRIMTYPEPOLYLINE:
            Im3DGlobals()->renderPipelines.polyLine = pipeline;
            return pipeline;
        case rwPRIMTYPEPOINTLIST:
            Im3DGlobals()->renderPipelines.pointList = pipeline;
            return pipeline;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25, primitiveType);
            RwErrorSet(&error);
            break;
        }
        }
    } else {
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            if (Im3DGlobals()->defaultRenderPipelines.triList != 0) {
                Im3DGlobals()->renderPipelines.triList =
                    Im3DGlobals()->defaultRenderPipelines.triList;
            } else {
                Im3DGlobals()->renderPipelines.triList = 0;
            }
            return Im3DGlobals()->renderPipelines.triList;
        case rwPRIMTYPETRIFAN:
            if (Im3DGlobals()->defaultRenderPipelines.triFan != 0) {
                Im3DGlobals()->renderPipelines.triFan =
                    Im3DGlobals()->defaultRenderPipelines.triFan;
            } else {
                Im3DGlobals()->renderPipelines.triFan = 0;
            }
            return Im3DGlobals()->renderPipelines.triFan;
        case rwPRIMTYPETRISTRIP:
            if (Im3DGlobals()->defaultRenderPipelines.triStrip != 0) {
                Im3DGlobals()->renderPipelines.triStrip =
                    Im3DGlobals()->defaultRenderPipelines.triStrip;
            } else {
                Im3DGlobals()->renderPipelines.triStrip = 0;
            }
            return Im3DGlobals()->defaultRenderPipelines.triStrip;
        case rwPRIMTYPELINELIST:
            if (Im3DGlobals()->defaultRenderPipelines.lineList != 0) {
                Im3DGlobals()->renderPipelines.lineList =
                    Im3DGlobals()->defaultRenderPipelines.lineList;
            } else {
                Im3DGlobals()->renderPipelines.lineList = 0;
            }
            return Im3DGlobals()->defaultRenderPipelines.lineList;
        case rwPRIMTYPEPOLYLINE:
            if (Im3DGlobals()->defaultRenderPipelines.polyLine != 0) {
                Im3DGlobals()->renderPipelines.polyLine =
                    Im3DGlobals()->defaultRenderPipelines.polyLine;
            } else {
                Im3DGlobals()->renderPipelines.polyLine = 0;
            }
            return Im3DGlobals()->defaultRenderPipelines.polyLine;
        case rwPRIMTYPEPOINTLIST:
            if (Im3DGlobals()->defaultRenderPipelines.pointList != 0) {
                Im3DGlobals()->renderPipelines.pointList =
                    Im3DGlobals()->defaultRenderPipelines.pointList;
            } else {
                Im3DGlobals()->renderPipelines.pointList = 0;
            }
            return Im3DGlobals()->defaultRenderPipelines.pointList;
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25, primitiveType);
            RwErrorSet(&error);
            break;
        }
        }
    }
    return 0;
}

void* _rwIm3DClose(void* instance, int offset, int size)
{
    _rwIm3DDestroyPlatformRenderPipelines(
        &((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                           _rwIm3DModule.globalsOffset))->defaultRenderPipelines);
    _rwIm3DDestroyPlatformTransformPipeline(
        &((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                           _rwIm3DModule.globalsOffset))->defaultTransformPipeline);
    --_rwIm3DModule.numInstances;
    return instance;
}

void* _rwIm3DOpen(void* instance, int offset, int size)
{
    int result = 1;
    _rwIm3DModule.globalsOffset = offset;
    _rwIm3DGlobals = (RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                      _rwIm3DModule.globalsOffset);
    ++_rwIm3DModule.numInstances;
    memset((unsigned char*)RwEngineInstance + _rwIm3DModule.globalsOffset, 0,
           sizeof(RwIm3DGlobals));
    if (result) {
        result = _rwIm3DCreatePlatformTransformPipeline(
            &((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                               _rwIm3DModule.globalsOffset))->defaultTransformPipeline);
    }
    if (result) {
        result = _rwIm3DCreatePlatformRenderPipelines(
            &((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                               _rwIm3DModule.globalsOffset))->defaultRenderPipelines);
    }
    if (result) {
        return instance;
    }
    _rwIm3DClose(instance, offset, size);
    return 0;
}
