#include "runtime/cstdio.h"

#undef va_start
#define va_start(arguments, format) ((void)(format), __builtin_va_info(&(arguments)))

typedef void (*LSCErrCallback)(void* object, char* message);

LSCErrCallback lsc_err_func;
void* lsc_err_obj;
char lsc_err_msg[256];

/* Soft ceiling: LSC_CallErrFunc 99.92% - relocation-label noise only; stop. */
void LSC_CallErrFunc(const char* format, ...)
{
    __va_list arguments;

    va_start(arguments, format);
    vsprintf(lsc_err_msg, format, arguments);
    if (lsc_err_func != 0) {
        lsc_err_func(lsc_err_obj, lsc_err_msg);
    }
    va_end(arguments);
}

void LSC_EntryErrFunc(LSCErrCallback callback, void* object)
{
    if (callback == 0) {
        lsc_err_func = 0;
        lsc_err_obj = 0;
    } else {
        lsc_err_func = callback;
        lsc_err_obj = object;
    }
}
