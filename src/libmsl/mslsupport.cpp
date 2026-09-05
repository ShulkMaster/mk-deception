#include "msl/mslBankLoadAsyncQueue.h"
#include "msl/mslsupport.h"
#include "runtime/cstdarg.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"
extern unsigned int mslGCN_AXCallback_Ticks;
extern "C" int printf(const char* format, ...);

void mslAsyncComplete(
    _mslAsyncResponse* response, bool succeeded, void* result,
    void* error) {
    if (response != 0) {
        if (succeeded) {
            response->status = 0;
            response->result = result;
        } else {
            response->status = 2;
            response->result = error;
        }

        if (response->callback != 0) {
            response->callback(response);
        }
    }
}

void mslAsyncBegin(
    _mslAsyncResponse* response, void* user_data) {
    if (response != 0) {
        response->status = 1;
        response->result = user_data;
    }
}

extern "C" void mslFileNameNoExt(
    const char* filename, char* output) {
    int extension = -1;
    const char* start = filename;
    char character;

    while ((character = *filename++) != '\0') {
        if (character == '.') {
            extension = filename - start - 1;
        }

        if (character == '\\' || character == ':' || character == '/') {
            extension = -1;
        }
    }

    if (extension >= 0) {
        strncpy(output, start, extension);
        output[extension] = '\0';
    } else {
        strcpy(output, start);
    }
}

extern "C" unsigned int mslIntLog2(unsigned int value) {
    int result;

    if (value == 0) {
        return 0;
    }

    result = -1;
    while (value != 0) {
        value >>= 1;
        result++;
    }
    return result;
}

extern "C" float mslGetTime(void) {
    return 0.005f * mslGCN_AXCallback_Ticks;
}

/* Matched: 100% report-exact; GC/2.7 requires its EABI varargs initializer.
 * The shared va_start spelling uses __va_start, unsupported by this compiler. */
extern "C" void mslDebugPrintf(const char* format, ...) {
    char buffer[256];
    __va_list args;

    __builtin_va_info(&args);
    vsprintf(buffer, format, args);
    va_end(args);
    printf(buffer);
}
