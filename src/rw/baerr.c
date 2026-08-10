#include "libmkparticle/rw_engine.h"
#include "runtime/cstdarg.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"

static RwModuleInfo errorModule;

void* _rwErrorOpen(void* object, RwInt32 offset, RwInt32 size) {
    errorModule.globalsOffset = offset;
    errorModule.numInstances++;
    ((RwError*)((RwUInt8*)RwEngineInstance +
               errorModule.globalsOffset))->pluginID = 0;
    ((RwError*)((RwUInt8*)RwEngineInstance +
               errorModule.globalsOffset))->errorCode = (RwInt32)0x80000000;
    return object;
}

void* _rwErrorClose(void* object, RwInt32 offset, RwInt32 size) {
    errorModule.numInstances--;
    return object;
}

RwError* RwErrorSet(RwError* error) {
    if (((RwError*)((RwUInt8*)RwEngineInstance +
                   errorModule.globalsOffset))->pluginID == 0 &&
        ((RwError*)((RwUInt8*)RwEngineInstance +
                   errorModule.globalsOffset))->errorCode ==
            (RwInt32)0x80000000) {
        if (((RwUInt32)error->errorCode & 0x80000000) != 0) {
            ((RwError*)((RwUInt8*)RwEngineInstance +
                       errorModule.globalsOffset))->pluginID = 0;
        } else {
            ((RwError*)((RwUInt8*)RwEngineInstance +
                       errorModule.globalsOffset))->pluginID = error->pluginID;
        }
        ((RwError*)((RwUInt8*)RwEngineInstance +
                   errorModule.globalsOffset))->errorCode = error->errorCode;
    }
    return error;
}

RwError* RwErrorGet(RwError* error) {
    *error = *(RwError*)((RwUInt8*)RwEngineInstance +
                        errorModule.globalsOffset);
    ((RwError*)((RwUInt8*)RwEngineInstance +
               errorModule.globalsOffset))->pluginID = 0;
    ((RwError*)((RwUInt8*)RwEngineInstance +
               errorModule.globalsOffset))->errorCode = (RwInt32)0x80000000;
    return error;
}

RwInt32 _rwerror(RwInt32 code, ...) {
    __va_list args;

    __builtin_va_info(args);
    return code;
}
