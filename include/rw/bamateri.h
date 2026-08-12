#ifndef RW_BAMATERI_H
#define RW_BAMATERI_H

#include "rw/rpworld_types.h"

int RpMaterialRegisterPlugin(
    int size, unsigned int pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB,
    RwPluginObjectCopy copyCB);
int RpMaterialRegisterPluginStream(
    unsigned int pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB);

#endif
