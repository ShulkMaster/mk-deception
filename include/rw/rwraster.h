#ifndef RW_RWRASTER_H
#define RW_RWRASTER_H

#include "rw/rwcore_types.h"

int RwRasterRegisterPlugin(
    int size, unsigned int pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
void* _rwRasterOpen(void* instance, int offset, int size);
void* _rwRasterClose(void* instance, int offset, int size);

#endif
