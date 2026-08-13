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
        RxPipelineGlobals()->platformWorldSectorPipeline = pipeline;
        RpWorldSetDefaultSectorPipeline(pipeline);
        return 1;
    }
    return 0;
}


void _rpDestroyPlatformWorldSectorPipelines(void) {
    RpWorldSetDefaultSectorPipeline(0);
    if (RxPipelineGlobals()->platformWorldSectorPipeline != 0) {
        _rxPipelineDestroy(RxPipelineGlobals()->platformWorldSectorPipeline);
        RxPipelineGlobals()->platformWorldSectorPipeline = 0;
    }
}

int _rpCreatePlatformAtomicPipelines(void) {
    RxPipeline* pipeline = _rpDlAtomicPipelineCreate(
        2, 0, _rxGCAtomicDefaultInstanceCallback,
        _rxGCAtomicDefaultReinstanceCallback,
        _rxGCAtomicDefaultLightingCallback, _rxGCDefaultRenderCallback);
    if (pipeline != 0) {
        RxPipelineGlobals()->platformAtomicPipeline = pipeline;
        RpAtomicSetDefaultPipeline(pipeline);
        return 1;
    }
    return 0;
}


void _rpDestroyPlatformAtomicPipelines(void) {
    RpAtomicSetDefaultPipeline(0);
    if (RxPipelineGlobals()->platformAtomicPipeline != 0) {
        _rxPipelineDestroy(RxPipelineGlobals()->platformAtomicPipeline);
        RxPipelineGlobals()->platformAtomicPipeline = 0;
    }
}
