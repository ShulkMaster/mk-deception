#include "libmkparticle/rw_engine.h"
#include "runtime/cstdarg.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"

static RwModuleInfo errorModule;

void* _rwErrorOpen(void* object, int offset, int size) {
    errorModule.globalsOffset = offset;
    errorModule.numInstances++;
    ((RwError*)((unsigned char*)RwEngineInstance +
               errorModule.globalsOffset))->pluginID = 0;
    ((RwError*)((unsigned char*)RwEngineInstance +
               errorModule.globalsOffset))->errorCode = (int)0x80000000;
    return object;
}

void* _rwErrorClose(void* object, int offset, int size) {
    errorModule.numInstances--;
    return object;
}

RwError* RwErrorSet(RwError* error) {
    if (((RwError*)((unsigned char*)RwEngineInstance +
                   errorModule.globalsOffset))->pluginID == 0 &&
        ((RwError*)((unsigned char*)RwEngineInstance +
                   errorModule.globalsOffset))->errorCode ==
            (int)0x80000000) {
        if (((unsigned int)error->errorCode & 0x80000000) != 0) {
            ((RwError*)((unsigned char*)RwEngineInstance +
                       errorModule.globalsOffset))->pluginID = 0;
        } else {
            ((RwError*)((unsigned char*)RwEngineInstance +
                       errorModule.globalsOffset))->pluginID = error->pluginID;
        }
        ((RwError*)((unsigned char*)RwEngineInstance +
                   errorModule.globalsOffset))->errorCode = error->errorCode;
    }
    return error;
}

RwError* RwErrorGet(RwError* error) {
    *error = *(RwError*)((unsigned char*)RwEngineInstance +
                        errorModule.globalsOffset);
    ((RwError*)((unsigned char*)RwEngineInstance +
               errorModule.globalsOffset))->pluginID = 0;
    ((RwError*)((unsigned char*)RwEngineInstance +
               errorModule.globalsOffset))->errorCode = (int)0x80000000;
    return error;
}

int _rwerror(int code, ...) {
    __va_list args;

    __builtin_va_info(args);
    return code;
}
