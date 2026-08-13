#include "rw/rwplcore.h"
#include "rw/rwcolor.h"



static RwModuleInfo colorModule;

void* _rwColorOpen(void* instance, int offset, int size) {
    colorModule.numInstances++;
    return instance;
}

void* _rwColorClose(void* instance, int offset, int size) {
    colorModule.numInstances--;
    return instance;
}
