#include "rw/rwplcore.h"

/* rwcore.a/bacolor.obj -- portable Criterion color module open/close. */

static RwModuleInfo colorModule;

void* _rwColorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    colorModule.numInstances++;
    return instance;
}

void* _rwColorClose(void* instance, RwInt32 offset, RwInt32 size) {
    colorModule.numInstances--;
    return instance;
}
