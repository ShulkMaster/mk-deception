#include "rw/rwim3d.h"
#include "rw/rxpipeline.h"

int _rwIm3DCreatePlatformTransformPipeline(RxPipeline** transformPipeline) {
    /* TODO: Retail keeps an unused cleanup-success value in r29 and therefore
     * uses _savegpr_28; this semantic body intentionally omits that dead state. */
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
            if (pipeline != 0) {
                *transformPipeline = pipeline;
                RwIm3DSetTransformPipeline(pipeline);
                return 1;
            }
        }
        _rxPipelineDestroy(pipeline);
    }
    return 0;
}

void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** transformPipeline) {
    /* TODO: Retail retains an unused success value in r30 after destruction;
     * this semantic implementation intentionally omits that dead state. */
    RwIm3DSetTransformPipeline(0);
    if (*transformPipeline != 0) {
        _rxPipelineDestroy(*transformPipeline);
        *transformPipeline = 0;
    }
}

void _rwIm3DDestroyPlatformRenderPipelines(
    RwIm3DRenderPipelines* renderPipelines) {
    /* TODO: Retail retains an unused success value in r30 after destruction;
     * this semantic implementation intentionally omits that dead state. */
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

int _rwIm3DCreatePlatformRenderPipelines(
    RwIm3DRenderPipelines* renderPipelines) {
    /* TODO: Retail keeps an unused cleanup-success value in r28 and therefore
     * uses _savegpr_28; this semantic body intentionally omits that dead state. */
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
                return 1;
            }
        }
        _rxPipelineDestroy(pipeline);
    }
    return 0;
}
