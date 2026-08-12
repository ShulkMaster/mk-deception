#include "rw/rwplcore.h"

static RwModuleInfo sectorModule;

RwPluginRegistry sectorTKList = { 0x88, 0x88, 0, 0, 0, 0 };

#pragma optimization_level 4
void* _rpSectorOpen(void* instance, int offset, int size) {
    (void)offset;
    (void)size;
    sectorModule.numInstances++;
    return instance;
}

void* _rpSectorClose(void* instance, int offset, int size) {
    (void)offset;
    (void)size;
    sectorModule.numInstances--;
    return instance;
}

#pragma optimization_level 0
int RpWorldSectorRegisterPlugin(int size, unsigned int pluginID,
                                    RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB,
                                    RwPluginObjectCopy copyCB) {
    int offset;
    offset = _rwPluginRegistryAddPlugin(&sectorTKList, size, pluginID, constructCB, destructCB,
                                        copyCB);
    return offset;
}

int RpWorldSectorRegisterPluginStream(unsigned int pluginID,
                                          RwPluginDataChunkReadCallBack readCB,
                                          RwPluginDataChunkWriteCallBack writeCB,
                                          RwPluginDataChunkGetSizeCallBack getSizeCB) {
    (void)pluginID;
    (void)readCB;
    (void)writeCB;
    (void)getSizeCB;
    return 0;
}
