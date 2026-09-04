#include "runtime/cstring.h"

typedef void (*RNAErrCallback)(void* object, const char* message);

RNAErrCallback rnaerr_func = 0;
void* rnaerr_obj = 0;
char rnaerr_msg[256];

void RNAERR_CallErrFunc(const char* message) {
    strncpy(rnaerr_msg, message, 255);
    if (rnaerr_func != 0) {
        rnaerr_func(rnaerr_obj, rnaerr_msg);
    }
}

void RNAERR_EntryErrFunc(RNAErrCallback callback, void* object) {
    rnaerr_func = callback;
    rnaerr_obj = object;
}
