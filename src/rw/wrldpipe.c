#include "rw/nodegamecube.h"

extern int _rpDlVtxFmtPluginAttach(void);
extern int _rpDlLightPluginAttach(void);
extern int RpMatGCAlphaPassAttach(void);

int _rxWorldDevicePluginAttach(void) {
    int result;

    if (_rpDlVtxFmtPluginAttach() == 0) {
        return 0;
    }
    if (_rpDlLightPluginAttach() == 0) {
        return 0;
    }
    result = RpMatGCAlphaPassAttach();
    if (result == 0) {
        return 0;
    }
    return 1;
}

int _rpCreatePlatformMaterialPipelines(void) {
    return 1;
}

void _rpDestroyPlatformMaterialPipelines(void) {
}

int _rpCreatePlatformWorldSectorPipelines(void) {
    RxPipeline* pipeline = _rpDlSectorPipelineCreate(
        2, 0, _rxGCSectorDefaultInstanceCallback, 0,
        _rxGCSectorDefaultLightingCallback,
        (RxGCSectorRenderCallBack)_rxGCDefaultRenderCallback);
    if (pipeline != 0) {
        rxPipelineGlobalField(platformWorldSectorPipeline) = pipeline;
        RpWorldSetDefaultSectorPipeline(pipeline);
        return 1;
    }
    return 0;
}


void _rpDestroyPlatformWorldSectorPipelines(void) {
    /* TODO: Retail retains an unused success value in r31 after destruction;
     * this semantic implementation intentionally omits that dead state. */
    RpWorldSetDefaultSectorPipeline(0);
    if (rxPipelineGlobalField(platformWorldSectorPipeline) != 0) {
        _rxPipelineDestroy(rxPipelineGlobalField(platformWorldSectorPipeline));
        rxPipelineGlobalField(platformWorldSectorPipeline) = 0;
    }
}

int _rpCreatePlatformAtomicPipelines(void) {
    RxPipeline* pipeline = _rpDlAtomicPipelineCreate(
        2, 0, _rxGCAtomicDefaultInstanceCallback,
        _rxGCAtomicDefaultReinstanceCallback,
        _rxGCAtomicDefaultLightingCallback, _rxGCDefaultRenderCallback);
    if (pipeline != 0) {
        rxPipelineGlobalField(platformAtomicPipeline) = pipeline;
        RpAtomicSetDefaultPipeline(pipeline);
        return 1;
    }
    return 0;
}


static inline int RxPipelineDestroy(RxPipeline* pipeline) {
    _rxPipelineDestroy(pipeline);
    return 1;
}

/* TODO: [breakthrough] 92.68%; inlined TRUE-returning destroy explains retail r31;
 * retail passes the pipeline straight in r3, ours copies it to r31 first. */
void _rpDestroyPlatformAtomicPipelines(void) {
    RpAtomicSetDefaultPipeline(0);
    if (rxPipelineGlobalField(platformAtomicPipeline) != 0) {
        RxPipelineDestroy(rxPipelineGlobalField(platformAtomicPipeline));
        rxPipelineGlobalField(platformAtomicPipeline) = 0;
    }
}
