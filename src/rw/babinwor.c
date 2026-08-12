#include "rw/rpworld_types.h"
#include "rw/rwstream.h"
#include "rw/rxpipeline.h"

static RwModuleInfo binWorldModule;
static int lastSeenWorldRightsPluginId;
static int lastSeenWorldExtraData;
static int lastSeenSectRightsPluginId;
static int lastSeenSectExtraData;

RwStream* _rpReadWorldRights(RwStream* stream, int binaryLength,
                             void* object, int offset, int size) {
    if (RwStreamReadInt32(stream, &lastSeenWorldRightsPluginId, 4) == 0) {
        return 0;
    }
    if (binaryLength == 8 &&
        RwStreamReadInt32(stream, &lastSeenWorldExtraData, 4) == 0) {
        return 0;
    }
    return stream;
}

RwStream* _rpWriteWorldRights(RwStream* stream, int binaryLength,
                              const void* object, int offset,
                              int size) {
    const RpWorld* world = object;
    if (RwStreamWriteInt32(stream, (const int*)&world->pipeline->pluginId,
                           4) == 0) {
        return 0;
    }
    if (RwStreamWriteInt32(stream,
                           (const int*)&world->pipeline->pluginData, 4) ==
        0) {
        return 0;
    }
    return stream;
}

int _rpSizeWorldRights(const void* object, int offset,
                           int size) {
    const RpWorld* world = object;
    if (world->pipeline != 0 && world->pipeline->pluginId != 0) {
        return 8;
    }
    return 0;
}

RwStream* _rpReadSectRights(RwStream* stream, int binaryLength,
                            void* object, int offset, int size) {
    if (RwStreamReadInt32(stream, &lastSeenSectRightsPluginId, 4) == 0) {
        return 0;
    }
    if (binaryLength == 8 &&
        RwStreamReadInt32(stream, &lastSeenSectExtraData, 4) == 0) {
        return 0;
    }
    return stream;
}

RwStream* _rpWriteSectRights(RwStream* stream, int binaryLength,
                             const void* object, int offset,
                             int size) {
    const RpWorldSector* sector = object;
    if (RwStreamWriteInt32(stream, (const int*)&sector->pipeline->pluginId,
                           4) == 0) {
        return 0;
    }
    if (RwStreamWriteInt32(stream,
                           (const int*)&sector->pipeline->pluginData, 4) ==
        0) {
        return 0;
    }
    return stream;
}

int _rpSizeSectRights(const void* object, int offset,
                          int size) {
    const RpWorldSector* sector = object;
    if (sector->pipeline != 0 && sector->pipeline->pluginId != 0) {
        return 8;
    }
    return 0;
}

void* _rpBinaryWorldClose(void* instance, int offset, int size) {
    binWorldModule.numInstances--;
    return instance;
}

void* _rpBinaryWorldOpen(void* instance, int offset, int size) {
    binWorldModule.numInstances++;
    return instance;
}
