#include "rw/rwplcore.h"
#include "runtime/cstring.h"

extern void* RwEngineInstance;

RwInt32 _rpGameCubeMTEngineOffset;

static void* GameCubeMTOpen(void* instance, RwInt32 offset, RwInt32 size) {
    memset((unsigned char*)RwEngineInstance + _rpGameCubeMTEngineOffset, 0, 8);
    return instance;
}

static void* GameCubeMTClose(void* instance, RwInt32 offset, RwInt32 size) {
    return instance;
}

RwBool _rpGameCubeMTPipePluginAttach(void) {
    _rpGameCubeMTEngineOffset = RwEngineRegisterPlugin(
        8, 0x129, GameCubeMTOpen, GameCubeMTClose);
    if (_rpGameCubeMTEngineOffset < 0) {
        return 0;
    }
    return 1;
}
