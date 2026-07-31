#include "rw/rwplcore.h"

static RwModuleInfo sectorModule;

static RwPluginRegistry sectorTKList = { 0x88, 0x88, 0, 0, 0, 0 };

RwBool _rpSectorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    (void)instance;
    (void)offset;
    (void)size;
    sectorModule.numInstances++;
}

RwBool _rpSectorClose(void* instance, RwInt32 offset, RwInt32 size) {
    (void)instance;
    (void)offset;
    (void)size;
    sectorModule.numInstances--;
}

RwInt32 RpWorldSectorRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                    RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB,
                                    RwPluginObjectCopy copyCB) {
    RwInt32 offset;
    offset = _rwPluginRegistryAddPlugin(&sectorTKList, size, pluginID, constructCB, destructCB,
                                        copyCB);
    return offset;
}

RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32 pluginID, void* readCB, void* writeCB,
                                          void* getSizeCB) {
    (void)pluginID;
    (void)readCB;
    (void)writeCB;
    (void)getSizeCB;
    return 0;
}
