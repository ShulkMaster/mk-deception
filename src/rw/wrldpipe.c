#include "rw/nodegamecube.h"

extern RwBool _rpDlVtxFmtPluginAttach(void);
extern RwBool _rpDlLightPluginAttach(void);
extern RwBool RpMatGCAlphaPassAttach(void);
typedef void (*RxGCInstanceCallback)(void);
typedef void (*RxGCReinstanceCallback)(void);
typedef void (*RxGCLightingCallback)(void);
typedef void (*RxGCRenderCallback)(void);
extern RxPipeline* _rpDlAtomicPipelineCreate(
    RwUInt32 pluginId, RwUInt32 pluginData,
    RxGCInstanceCallback instanceCallback,
    RxGCReinstanceCallback reinstanceCallback,
    RxGCLightingCallback lightingCallback, RxGCRenderCallback renderCallback);
extern void _rxPipelineDestroy(RxPipeline* pipeline);
extern RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline);
extern RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline);
extern void _rxGCAtomicDefaultInstanceCallback(void);
extern void _rxGCAtomicDefaultReinstanceCallback(void);
extern void _rxGCAtomicDefaultLightingCallback(void);
extern void _rxGCDefaultRenderCallback(void);

RwBool _rxWorldDevicePluginAttach(void) {
    RwBool result;

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

RwBool _rpCreatePlatformMaterialPipelines(void) {
    return 1;
}

void _rpDestroyPlatformMaterialPipelines(void) {
}

RwBool _rpCreatePlatformWorldSectorPipelines(void) {
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

RwBool _rpCreatePlatformAtomicPipelines(void) {
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
