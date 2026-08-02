#include "msl/mslBankLoadAsyncQueue.h"

extern "C" char* strcpy(char* destination, const char* source);
extern "C" char* strncpy(
    char* destination, const char* source, unsigned long count);
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

extern "C" int mslIntLog2(unsigned int value) {
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
 * Retail mslDebugPrintf requires PPC EABI variadic state. This project has no
 * portable stdarg header, so the function remains supplied by retail assembly.
 */
