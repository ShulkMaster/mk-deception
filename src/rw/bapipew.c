#include "rw/rxpipeline.h"

extern RwBool _rpCreatePlatformMaterialPipelines(void);
extern void _rpDestroyPlatformMaterialPipelines(void);
extern RwBool _rpCreatePlatformWorldSectorPipelines(void);
extern void _rpDestroyPlatformWorldSectorPipelines(void);
extern RwBool _rpCreatePlatformAtomicPipelines(void);
extern void _rpDestroyPlatformAtomicPipelines(void);

RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline) {
    if (pipeline == 0) {
        if (RXPIPELINEGLOBAL(platformWorldSectorPipeline) != 0) {
            pipeline = RXPIPELINEGLOBAL(platformWorldSectorPipeline);
        } else {
            pipeline = 0;
        }
    }
    RXPIPELINEGLOBAL(defaultWorldSectorPipeline) = pipeline;
    return pipeline;
}

RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline) {
    if (pipeline == 0) {
        if (RXPIPELINEGLOBAL(platformAtomicPipeline) != 0) {
            pipeline = RXPIPELINEGLOBAL(platformAtomicPipeline);
        } else {
            pipeline = 0;
        }
    }
    RXPIPELINEGLOBAL(defaultAtomicPipeline) = pipeline;
    return pipeline;
}

void _rpWorldPipelineClose(void) {
    _rpDestroyPlatformWorldSectorPipelines();
    _rpDestroyPlatformAtomicPipelines();
    _rpDestroyPlatformMaterialPipelines();
}

RwBool _rpWorldPipelineOpen(void) {
    RwBool result = TRUE;

    RXPIPELINEGLOBAL(defaultAtomicPipeline) = 0;
    RXPIPELINEGLOBAL(defaultWorldSectorPipeline) = 0;
    RXPIPELINEGLOBAL(defaultMaterialPipeline) = 0;
    RXPIPELINEGLOBAL(pipeline48) = 0;
    RXPIPELINEGLOBAL(pipeline4C) = 0;
    RXPIPELINEGLOBAL(pipeline50) = 0;
    RXPIPELINEGLOBAL(platformAtomicPipeline) = 0;
    RXPIPELINEGLOBAL(platformWorldSectorPipeline) = 0;
    RXPIPELINEGLOBAL(platformMaterialPipeline) = 0;

    result = _rpCreatePlatformMaterialPipelines();
    if (result != FALSE) {
        result = _rpCreatePlatformAtomicPipelines();
    }
    if (result != FALSE) {
        result = _rpCreatePlatformWorldSectorPipelines();
    }
    if (result != FALSE) {
        return TRUE;
    }
    _rpWorldPipelineClose();
    return FALSE;
}

RwBool _rpWorldPipeAttach(void) {
    return TRUE;
}
