#include "rw/rwplcore.h"



static RwModuleInfo colorModule;

void* _rwColorOpen(void* instance, int offset, int size) {
    colorModule.numInstances++;
    return instance;
}

void* _rwColorClose(void* instance, int offset, int size) {
    colorModule.numInstances--;
    return instance;
}
