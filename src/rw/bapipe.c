#include "rw/rwplcore.h"

typedef struct RwCamera RwCamera;

extern RwBool _rxPipelineOpen(void);
extern void _rxPipelineClose(void);

RwInt32 _rxPipelineGlobalsOffset;

void* _rwRenderPipelineOpen(void* instance, RwInt32 offset, RwInt32 size) {
    _rxPipelineGlobalsOffset = offset;
    if (_rxPipelineOpen() == FALSE) {
        return 0;
    }
    return instance;
}

void* _rwRenderPipelineClose(void* instance, RwInt32 offset, RwInt32 size) {
    _rxPipelineClose();
    return instance;
}

RwBool _rwPipeAttach(void) {
    return TRUE;
}

void _rwPipeInitForCamera(RwCamera* camera) {
}
