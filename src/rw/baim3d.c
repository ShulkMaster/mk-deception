#include "rw/rwengine.h"
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

        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->transformData.numVertices = numVertices;
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->transformData.vertices = vertices;
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->transformData.stride = sizeof(RwIm3DVertex);
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->stash.localToWorld = localToWorld;
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->stash.flags = flags | 8 | 0x10;
        result = RxPipelineExecute(
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))
                ->transformPipeline,
            &((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                               _rwIm3DModule.globalsOffset))
                 ->transformData,
            1);
        if (result != 0) {
            return vertices;
        }
    }
    return 0;
}

int RwIm3DEnd(void)
{
    int transformed =
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->transformData.vertices != 0;

    if (!transformed) {
        return 0;
    }
    memset(&((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))
                ->transformData,
           0, sizeof(RwIm3DTransformData) + sizeof(RwIm3DStash));
    return 1;
}

int RwIm3DRenderIndexedPrimitive(RwPrimitiveType primitiveType,
                                    const RwImVertexIndex* indices,
                                    int numIndices)
{
    int transformed =
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->transformData.vertices != 0;

    if (transformed) {
        RwIm3DStash* data =
            &((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                               _rwIm3DModule.globalsOffset))
                 ->stash;
        data->renderData.pipeline = 0;
        data->renderData.primitiveType = primitiveType;
        data->renderData.indices = indices;
        data->renderData.numIndices = numIndices;
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.triList;
            data->renderData.numIndices = numIndices - (numIndices % 3);
            break;
        case rwPRIMTYPETRIFAN:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.triStrip;
            break;
        case rwPRIMTYPELINELIST:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.lineList;
            data->renderData.numIndices = numIndices - (numIndices % 2);
            break;
        case rwPRIMTYPEPOLYLINE:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.polyLine;
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
    RwIm3DVertex* vertices =
        ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                          _rwIm3DModule.globalsOffset))
            ->transformData.vertices;
    int transformed = vertices != 0;

    if (transformed) {
        RwIm3DStash* data =
            &((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                               _rwIm3DModule.globalsOffset))
                 ->stash;
        data->renderData.pipeline = 0;
        data->renderData.primitiveType = primitiveType;
        data->renderData.indices = 0;
        data->renderData.numIndices =
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))
                ->transformData.numVertices;
        switch (primitiveType) {
        case rwPRIMTYPETRILIST:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.triList;
            break;
        case rwPRIMTYPETRIFAN:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.triStrip;
            break;
        case rwPRIMTYPELINELIST:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.lineList;
            break;
        case rwPRIMTYPEPOLYLINE:
            data->renderData.pipeline =
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))
                    ->renderPipelines.polyLine;
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
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))->renderPipelines.triList = pipeline;
            return pipeline;
        case rwPRIMTYPETRIFAN:
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))->renderPipelines.triFan = pipeline;
            return pipeline;
        case rwPRIMTYPETRISTRIP:
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))->renderPipelines.triStrip = pipeline;
            return pipeline;
        case rwPRIMTYPELINELIST:
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))->renderPipelines.lineList = pipeline;
            return pipeline;
        case rwPRIMTYPEPOLYLINE:
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))->renderPipelines.polyLine = pipeline;
            return pipeline;
        case rwPRIMTYPEPOINTLIST:
            ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                              _rwIm3DModule.globalsOffset))->renderPipelines.pointList = pipeline;
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
            if (((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                   _rwIm3DModule.globalsOffset))->defaultRenderPipelines.triList != 0) {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.triList =
                    ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                      _rwIm3DModule.globalsOffset))->defaultRenderPipelines.triList;
            } else {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.triList = 0;
            }
            return ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                     _rwIm3DModule.globalsOffset))->renderPipelines.triList;
        case rwPRIMTYPETRIFAN:
            if (((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                   _rwIm3DModule.globalsOffset))->defaultRenderPipelines.triFan != 0) {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.triFan =
                    ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                      _rwIm3DModule.globalsOffset))->defaultRenderPipelines.triFan;
            } else {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.triFan = 0;
            }
            return ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                     _rwIm3DModule.globalsOffset))->renderPipelines.triFan;
        case rwPRIMTYPETRISTRIP:
            if (((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                   _rwIm3DModule.globalsOffset))->defaultRenderPipelines.triStrip != 0) {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.triStrip =
                    ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                      _rwIm3DModule.globalsOffset))->defaultRenderPipelines.triStrip;
            } else {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.triStrip = 0;
            }
            return ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                     _rwIm3DModule.globalsOffset))->defaultRenderPipelines.triStrip;
        case rwPRIMTYPELINELIST:
            if (((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                   _rwIm3DModule.globalsOffset))->defaultRenderPipelines.lineList != 0) {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.lineList =
                    ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                      _rwIm3DModule.globalsOffset))->defaultRenderPipelines.lineList;
            } else {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.lineList = 0;
            }
            return ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                     _rwIm3DModule.globalsOffset))->defaultRenderPipelines.lineList;
        case rwPRIMTYPEPOLYLINE:
            if (((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                   _rwIm3DModule.globalsOffset))->defaultRenderPipelines.polyLine != 0) {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.polyLine =
                    ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                      _rwIm3DModule.globalsOffset))->defaultRenderPipelines.polyLine;
            } else {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.polyLine = 0;
            }
            return ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                     _rwIm3DModule.globalsOffset))->defaultRenderPipelines.polyLine;
        case rwPRIMTYPEPOINTLIST:
            if (((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                   _rwIm3DModule.globalsOffset))->defaultRenderPipelines.pointList != 0) {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.pointList =
                    ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                      _rwIm3DModule.globalsOffset))->defaultRenderPipelines.pointList;
            } else {
                ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                  _rwIm3DModule.globalsOffset))->renderPipelines.pointList = 0;
            }
            return ((RwIm3DGlobals*)((unsigned char*)RwEngineInstance +
                                     _rwIm3DModule.globalsOffset))->defaultRenderPipelines.pointList;
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
