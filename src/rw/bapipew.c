#include "rw/rxpipeline.h"

extern RwBool _rpCreatePlatformMaterialPipelines(void);
extern void _rpDestroyPlatformMaterialPipelines(void);
extern RwBool _rpCreatePlatformWorldSectorPipelines(void);
extern void _rpDestroyPlatformWorldSectorPipelines(void);
extern RwBool _rpCreatePlatformAtomicPipelines(void);
extern void _rpDestroyPlatformAtomicPipelines(void);

RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline) {
    RxPipelinePlatformGlobals* globals = (RxPipelinePlatformGlobals*)
        ((RwUInt8*)RwEngineInstance + _rxPipelineGlobalsOffset);
    if (pipeline == 0) {
        if (globals->platformWorldSectorPipeline != 0) {
            pipeline = globals->platformWorldSectorPipeline;
        } else {
            pipeline = 0;
        }
    }
    globals->defaultWorldSectorPipeline = pipeline;
    return pipeline;
}

RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline) {
    RxPipelinePlatformGlobals* globals = (RxPipelinePlatformGlobals*)
        ((RwUInt8*)RwEngineInstance + _rxPipelineGlobalsOffset);
    if (pipeline == 0) {
        if (globals->platformAtomicPipeline != 0) {
            pipeline = globals->platformAtomicPipeline;
        } else {
            pipeline = 0;
        }
    }
    globals->defaultAtomicPipeline = pipeline;
    return pipeline;
}

void _rpWorldPipelineClose(void) {
    _rpDestroyPlatformWorldSectorPipelines();
    _rpDestroyPlatformAtomicPipelines();
    _rpDestroyPlatformMaterialPipelines();
}

RwBool _rpWorldPipelineOpen(void) {
    RxPipelinePlatformGlobals* globals = (RxPipelinePlatformGlobals*)
        ((RwUInt8*)RwEngineInstance + _rxPipelineGlobalsOffset);
    RwBool result = 1;

    globals->defaultAtomicPipeline = 0;
    globals->defaultWorldSectorPipeline = 0;
    globals->defaultMaterialPipeline = 0;
    globals->field_0x48 = 0;
    globals->field_0x4C = 0;
    globals->field_0x50 = 0;
    globals->platformAtomicPipeline = 0;
    globals->platformWorldSectorPipeline = 0;
    globals->platformMaterialPipeline = 0;

    result = _rpCreatePlatformMaterialPipelines();
    if (result != 0) {
        result = _rpCreatePlatformAtomicPipelines();
    }
    if (result != 0) {
        result = _rpCreatePlatformWorldSectorPipelines();
    }
    if (result != 0) {
        return 1;
    }
    _rpWorldPipelineClose();
    return 0;
}

RwBool _rpWorldPipeAttach(void) {
    return 1;
}
