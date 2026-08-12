#include "rw/rwplcore.h"
#include "runtime/cstring.h"

extern void* RwEngineInstance;

int _rpGameCubeMTEngineOffset;

static void* GameCubeMTOpen(void* instance, int offset, int size) {
    memset((unsigned char*)RwEngineInstance + _rpGameCubeMTEngineOffset, 0, 8);
    return instance;
}

static void* GameCubeMTClose(void* instance, int offset, int size) {
    return instance;
}

int _rpGameCubeMTPipePluginAttach(void) {
    _rpGameCubeMTEngineOffset = RwEngineRegisterPlugin(
        8, 0x129, GameCubeMTOpen, GameCubeMTClose);
    if (_rpGameCubeMTEngineOffset < 0) {
        return 0;
    }
    return 1;
}
