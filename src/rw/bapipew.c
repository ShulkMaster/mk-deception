#include "rw/rxpipeline.h"

extern int _rpCreatePlatformMaterialPipelines(void);
extern void _rpDestroyPlatformMaterialPipelines(void);
extern int _rpCreatePlatformWorldSectorPipelines(void);
extern void _rpDestroyPlatformWorldSectorPipelines(void);
extern int _rpCreatePlatformAtomicPipelines(void);
extern void _rpDestroyPlatformAtomicPipelines(void);

RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline) {
    if (pipeline == 0) {
        if (*(RxPipeline**)((unsigned char*)RwEngineInstance +
                            _rxPipelineGlobalsOffset + 0x58) != 0) {
            pipeline = *(RxPipeline**)((unsigned char*)RwEngineInstance +
                                       _rxPipelineGlobalsOffset + 0x58);
        } else {
            pipeline = 0;
        }
    }
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x40) = pipeline;
    return pipeline;
}

RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline) {
    if (pipeline == 0) {
        if (*(RxPipeline**)((unsigned char*)RwEngineInstance +
                            _rxPipelineGlobalsOffset + 0x54) != 0) {
            pipeline = *(RxPipeline**)((unsigned char*)RwEngineInstance +
                                       _rxPipelineGlobalsOffset + 0x54);
        } else {
            pipeline = 0;
        }
    }
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x3C) = pipeline;
    return pipeline;
}

void _rpWorldPipelineClose(void) {
    _rpDestroyPlatformWorldSectorPipelines();
    _rpDestroyPlatformAtomicPipelines();
    _rpDestroyPlatformMaterialPipelines();
}

int _rpWorldPipelineOpen(void) {
    int result = 1;

    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x3C) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x40) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x44) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x48) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x4C) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x50) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x54) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x58) = 0;
    *(RxPipeline**)((unsigned char*)RwEngineInstance +
                    _rxPipelineGlobalsOffset + 0x5C) = 0;

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

int _rpWorldPipeAttach(void) {
    return 1;
}
