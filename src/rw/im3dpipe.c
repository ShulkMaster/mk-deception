#include "rw/rwim3d.h"
#include "rw/rxpipeline.h"

typedef struct RxLockedPipe RxLockedPipe;
typedef struct RxNodeDefinition RxNodeDefinition;

extern RxPipeline* RxPipelineCreate(void);
extern RxLockedPipe* RxPipelineLock(RxPipeline* pipeline);
extern RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline,
                                              void* firstEntry, ...);
extern RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline);
extern void _rxPipelineDestroy(RxPipeline* pipeline);
extern RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void);
extern RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void);

RwBool _rwIm3DCreatePlatformTransformPipeline(RxPipeline** transformPipeline) {
    RxPipeline* pipeline;
    RxLockedPipe* lockedPipeline;

    pipeline = RxPipelineCreate();
    if (pipeline != 0) {
        pipeline->pluginId = 1;
        lockedPipeline = RxPipelineLock(pipeline);
        if (lockedPipeline != 0) {
            lockedPipeline = RxLockedPipeAddFragment(
                lockedPipeline, 0, RxNodeDefinitionGetGameCubeImmInstance(), 0);
            pipeline = RxLockedPipeUnlock(lockedPipeline);
        }
        if (pipeline != 0) {
            *transformPipeline = pipeline;
            RwIm3DSetTransformPipeline(pipeline);
            return TRUE;
        }
        _rxPipelineDestroy(pipeline);
    }
    return FALSE;
}

void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** transformPipeline) {
    RwIm3DSetTransformPipeline(0);
    if (*transformPipeline != 0) {
        _rxPipelineDestroy(*transformPipeline);
        *transformPipeline = 0;
    }
}

void _rwIm3DDestroyPlatformRenderPipelines(
    RwIm3DRenderPipelines* renderPipelines) {
    RwIm3DSetRenderPipeline(0, rwPRIMTYPETRILIST);
    RwIm3DSetRenderPipeline(0, rwPRIMTYPETRIFAN);
    RwIm3DSetRenderPipeline(0, rwPRIMTYPETRISTRIP);
    RwIm3DSetRenderPipeline(0, rwPRIMTYPELINELIST);
    RwIm3DSetRenderPipeline(0, rwPRIMTYPEPOLYLINE);

    if (renderPipelines->triList != 0) {
        _rxPipelineDestroy(renderPipelines->triList);
    }
    renderPipelines->triList = 0;
    renderPipelines->triFan = 0;
    renderPipelines->triStrip = 0;
    renderPipelines->lineList = 0;
    renderPipelines->polyLine = 0;
}

RwBool _rwIm3DCreatePlatformRenderPipelines(
    RwIm3DRenderPipelines* renderPipelines) {
    RxPipeline* pipeline;
    RxLockedPipe* lockedPipeline;

    pipeline = RxPipelineCreate();
    if (pipeline != 0) {
        pipeline->pluginId = 1;
        lockedPipeline = RxPipelineLock(pipeline);
        if (lockedPipeline != 0) {
            lockedPipeline = RxLockedPipeAddFragment(
                lockedPipeline, 0,
                RxNodeDefinitionGetGameCubeSubmitNoLight(), 0);
            pipeline = RxLockedPipeUnlock(lockedPipeline);
        }
        if (pipeline != 0) {
            renderPipelines->triList = pipeline;
            renderPipelines->triFan = pipeline;
            renderPipelines->triStrip = pipeline;
            renderPipelines->lineList = pipeline;
            renderPipelines->polyLine = pipeline;
            RwIm3DSetRenderPipeline(pipeline, rwPRIMTYPETRILIST);
            RwIm3DSetRenderPipeline(pipeline, rwPRIMTYPETRIFAN);
            RwIm3DSetRenderPipeline(pipeline, rwPRIMTYPETRISTRIP);
            RwIm3DSetRenderPipeline(pipeline, rwPRIMTYPELINELIST);
            RwIm3DSetRenderPipeline(pipeline, rwPRIMTYPEPOLYLINE);
            return TRUE;
        }
        _rxPipelineDestroy(pipeline);
    }
    return FALSE;
}
