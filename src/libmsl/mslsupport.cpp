#include "msl/mslBankLoadAsyncQueue.h"
#include "msl/mslsupport.h"
#include "runtime/cstring.h"
extern unsigned int mslGCN_AXCallback_Ticks;

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

/*
 * Honest blocker: retail formats through PPC EABI variadic state into a
 * 256-byte buffer and then calls printf. This MWCC C++ setup exposes neither
 * a portable stdarg header nor va_start; reproducing it would require a
 * compiler intrinsic or hand-authored ABI state, both intentionally avoided.
 */
