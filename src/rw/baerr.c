#include "libmkparticle/rw_engine.h"
#include "runtime/cstdarg.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"

static RwModuleInfo errorModule;

#define ERRORGLOBAL \
    RWPLUGINOFFSET(RwError, RwEngineInstance, errorModule.globalsOffset)

void* _rwErrorOpen(void* object, RwInt32 offset, RwInt32 size) {
    errorModule.globalsOffset = offset;
    errorModule.numInstances++;
    ERRORGLOBAL.pluginID = 0;
    ERRORGLOBAL.errorCode = (RwInt32)0x80000000;
    return object;
}

void* _rwErrorClose(void* object, RwInt32 offset, RwInt32 size) {
    errorModule.numInstances--;
    return object;
}

RwError* RwErrorSet(RwError* error) {
    if (ERRORGLOBAL.pluginID == 0 &&
        ERRORGLOBAL.errorCode == (RwInt32)0x80000000) {
        if (((RwUInt32)error->errorCode & 0x80000000) != 0) {
            ERRORGLOBAL.pluginID = 0;
        } else {
            ERRORGLOBAL.pluginID = error->pluginID;
        }
        ERRORGLOBAL.errorCode = error->errorCode;
    }
    return error;
}

RwError* RwErrorGet(RwError* error) {
    *error = ERRORGLOBAL;
    ERRORGLOBAL.pluginID = 0;
    ERRORGLOBAL.errorCode = (RwInt32)0x80000000;
    return error;
}

RwInt32 _rwerror(RwInt32 code, ...) {
    __va_list args;

    va_start(args, code);
    va_end(args);
    return code;
}
