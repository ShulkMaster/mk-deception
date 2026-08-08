#include "rw/rxpipeline.h"

extern RwBool _rpDlVtxFmtPluginAttach(void);
extern RwBool _rpDlLightPluginAttach(void);
extern RwBool RpMatGCAlphaPassAttach(void);
typedef void (*RxGCInstanceCallback)(void);
typedef void (*RxGCReinstanceCallback)(void);
typedef void (*RxGCLightingCallback)(void);
typedef void (*RxGCRenderCallback)(void);
extern RxPipeline* _rpDlSectorPipelineCreate(
    RwUInt32 pluginId, RwUInt32 pluginData,
    RxGCInstanceCallback instanceCallback,
    RxGCReinstanceCallback reinstanceCallback,
    RxGCLightingCallback lightingCallback, RxGCRenderCallback renderCallback);
extern RxPipeline* _rpDlAtomicPipelineCreate(
    RwUInt32 pluginId, RwUInt32 pluginData,
    RxGCInstanceCallback instanceCallback,
    RxGCReinstanceCallback reinstanceCallback,
    RxGCLightingCallback lightingCallback, RxGCRenderCallback renderCallback);
extern void _rxPipelineDestroy(RxPipeline* pipeline);
extern RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline);
extern RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline);
extern void _rxGCSectorDefaultInstanceCallback(void);
extern void _rxGCSectorDefaultLightingCallback(void);
extern void _rxGCAtomicDefaultInstanceCallback(void);
extern void _rxGCAtomicDefaultReinstanceCallback(void);
extern void _rxGCAtomicDefaultLightingCallback(void);
extern void _rxGCDefaultRenderCallback(void);

RwBool _rxWorldDevicePluginAttach(void) {
    RwBool result;

    if (_rpDlVtxFmtPluginAttach() == FALSE) {
        return FALSE;
    }
    if (_rpDlLightPluginAttach() == FALSE) {
        return FALSE;
    }
    result = RpMatGCAlphaPassAttach();
    if (result == FALSE) {
        return FALSE;
    }
    return TRUE;
}

RwBool _rpCreatePlatformMaterialPipelines(void) {
    return TRUE;
}

void _rpDestroyPlatformMaterialPipelines(void) {
}

RwBool _rpCreatePlatformWorldSectorPipelines(void) {
    RxPipeline* pipeline = _rpDlSectorPipelineCreate(
        2, 0, _rxGCSectorDefaultInstanceCallback, 0,
        _rxGCSectorDefaultLightingCallback, _rxGCDefaultRenderCallback);
    if (pipeline != 0) {
        RXPIPELINEGLOBAL(platformWorldSectorPipeline) = pipeline;
        RpWorldSetDefaultSectorPipeline(pipeline);
        return TRUE;
    }
    return FALSE;
}

/* Near miss: retail's RxPipelineDestroy macro retains its TRUE result in r31;
 * the result is unused and clean C emits the same destruction and clear. */
void _rpDestroyPlatformWorldSectorPipelines(void) {
    RpWorldSetDefaultSectorPipeline(0);
    if (RXPIPELINEGLOBAL(platformWorldSectorPipeline) != 0) {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(platformWorldSectorPipeline));
        RXPIPELINEGLOBAL(platformWorldSectorPipeline) = 0;
    }
}

RwBool _rpCreatePlatformAtomicPipelines(void) {
    RxPipeline* pipeline = _rpDlAtomicPipelineCreate(
        2, 0, _rxGCAtomicDefaultInstanceCallback,
        _rxGCAtomicDefaultReinstanceCallback,
        _rxGCAtomicDefaultLightingCallback, _rxGCDefaultRenderCallback);
    if (pipeline != 0) {
        RXPIPELINEGLOBAL(platformAtomicPipeline) = pipeline;
        RpAtomicSetDefaultPipeline(pipeline);
        return TRUE;
    }
    return FALSE;
}

/* Near miss: as above, only the unused macro result and its saved register
 * remain absent from the clean source. */
void _rpDestroyPlatformAtomicPipelines(void) {
    RpAtomicSetDefaultPipeline(0);
    if (RXPIPELINEGLOBAL(platformAtomicPipeline) != 0) {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(platformAtomicPipeline));
        RXPIPELINEGLOBAL(platformAtomicPipeline) = 0;
    }
}
