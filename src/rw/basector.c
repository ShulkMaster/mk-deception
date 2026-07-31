#include "rw/rwplcore.h"

static RwModuleInfo sectorModule;

RwPluginRegistry sectorTKList = { 0x88, 0x88, 0, 0, 0, 0 };

void* _rpSectorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    (void)offset;
    (void)size;
    sectorModule.numInstances++;
    return instance;
}

void* _rpSectorClose(void* instance, RwInt32 offset, RwInt32 size) {
    (void)offset;
    (void)size;
    sectorModule.numInstances--;
    return instance;
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

RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32 pluginID,
                                          RwPluginDataChunkReadCallBack readCB,
                                          RwPluginDataChunkWriteCallBack writeCB,
                                          RwPluginDataChunkGetSizeCallBack getSizeCB) {
    (void)pluginID;
    (void)readCB;
    (void)writeCB;
    (void)getSizeCB;
    return 0;
}
