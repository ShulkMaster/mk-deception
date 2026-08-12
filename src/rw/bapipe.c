#include "rw/rwplcore.h"

typedef struct RwCamera RwCamera;

extern int _rxPipelineOpen(void);
extern void _rxPipelineClose(void);

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
