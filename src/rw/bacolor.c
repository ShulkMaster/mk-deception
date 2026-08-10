#include "rw/rwplcore.h"



static RwModuleInfo colorModule;

void* _rwColorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    colorModule.numInstances++;
    return instance;
}

void* _rwColorClose(void* instance, RwInt32 offset, RwInt32 size) {
    colorModule.numInstances--;
    return instance;
}
