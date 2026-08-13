#include "rw/rwplcore.h"
#include "rw/rwcamera_internal.h"
#include "rw/rxpipeline.h"

int _rxPipelineGlobalsOffset;

void* _rwRenderPipelineOpen(void* instance, int offset, int size) {
    _rxPipelineGlobalsOffset = offset;
    if (_rxPipelineOpen() == 0) {
        return 0;
    }
    return instance;
}

void* _rwRenderPipelineClose(void* instance, int offset, int size) {
    _rxPipelineClose();
    return instance;
}

int _rwPipeAttach(void) {
    return 1;
}

void _rwPipeInitForCamera(RwCamera* camera) {
}
