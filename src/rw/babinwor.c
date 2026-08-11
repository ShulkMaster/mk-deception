#include "rw/rpworld_types.h"
#include "rw/rwstream.h"
#include "rw/rxpipeline.h"

static RwModuleInfo binWorldModule;
static RwInt32 lastSeenWorldRightsPluginId;
static RwInt32 lastSeenWorldExtraData;
static RwInt32 lastSeenSectRightsPluginId;
static RwInt32 lastSeenSectExtraData;

RwStream* _rpReadWorldRights(RwStream* stream, RwInt32 binaryLength,
                             void* object, RwInt32 offset, RwInt32 size) {
    if (RwStreamReadInt32(stream, &lastSeenWorldRightsPluginId, 4) == 0) {
        return 0;
    }
    if (binaryLength == 8 &&
        RwStreamReadInt32(stream, &lastSeenWorldExtraData, 4) == 0) {
        return 0;
    }
    return stream;
}

RwStream* _rpWriteWorldRights(RwStream* stream, RwInt32 binaryLength,
                              const void* object, RwInt32 offset,
                              RwInt32 size) {
    const RpWorld* world = object;
    if (RwStreamWriteInt32(stream, (const RwInt32*)&world->pipeline->pluginId,
                           4) == 0) {
        return 0;
    }
    if (RwStreamWriteInt32(stream,
                           (const RwInt32*)&world->pipeline->pluginData, 4) ==
        0) {
        return 0;
    }
    return stream;
}

RwInt32 _rpSizeWorldRights(const void* object, RwInt32 offset,
                           RwInt32 size) {
    const RpWorld* world = object;
    if (world->pipeline != 0 && world->pipeline->pluginId != 0) {
        return 8;
    }
    return 0;
}

RwStream* _rpReadSectRights(RwStream* stream, RwInt32 binaryLength,
                            void* object, RwInt32 offset, RwInt32 size) {
    if (RwStreamReadInt32(stream, &lastSeenSectRightsPluginId, 4) == 0) {
        return 0;
    }
    if (binaryLength == 8 &&
        RwStreamReadInt32(stream, &lastSeenSectExtraData, 4) == 0) {
        return 0;
    }
    return stream;
}

RwStream* _rpWriteSectRights(RwStream* stream, RwInt32 binaryLength,
                             const void* object, RwInt32 offset,
                             RwInt32 size) {
    const RpWorldSector* sector = object;
    if (RwStreamWriteInt32(stream, (const RwInt32*)&sector->pipeline->pluginId,
                           4) == 0) {
        return 0;
    }
    if (RwStreamWriteInt32(stream,
                           (const RwInt32*)&sector->pipeline->pluginData, 4) ==
        0) {
        return 0;
    }
    return stream;
}

RwInt32 _rpSizeSectRights(const void* object, RwInt32 offset,
                          RwInt32 size) {
    const RpWorldSector* sector = object;
    if (sector->pipeline != 0 && sector->pipeline->pluginId != 0) {
        return 8;
    }
    return 0;
}

void* _rpBinaryWorldClose(void* instance, RwInt32 offset, RwInt32 size) {
    binWorldModule.numInstances--;
    return instance;
}

void* _rpBinaryWorldOpen(void* instance, RwInt32 offset, RwInt32 size) {
    binWorldModule.numInstances++;
    return instance;
}
