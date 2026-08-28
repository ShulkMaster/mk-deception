#include "cri/mps.h"

void UTY_MemsetDword(unsigned int* destination, unsigned int value, unsigned int count);
int MPSDEC_DecHdMpeg1(MpsHandle* handle, const unsigned char* data, int size,
                      int* consumed, int* header_flags);
void MPSDEC_Finish(void);
void MPSDEC_Init(void);

static int gap_06_80497E9C_bss;
MpsLibWork* MPSLIB_libwork;
static const char* cri_verstr_ptr;
static MpsHandle* mpslib_hn_last;

const char MPSLIB_version_str[] =
    "\nCRI MPS/GC Ver.1.924 Build:Sep  3 2004 11:38:25\n\0"
    "Append: MW2407 GC20Apr2004Patch1\n";

static inline int mpslib_check_handle(MpsHandle* handle) {
    mpslib_hn_last = handle;
    if (handle == 0 || handle->state == 1) {
        return -1;
    }
    return 0;
}

static inline int mpslib_set_error(MpsHandle* handle, int error) {
    if (handle == 0) {
        MPSLIB_libwork->error = error;
        if (error != 0 && MPSLIB_libwork->error_callback != 0) {
            MPSLIB_libwork->error_callback(MPSLIB_libwork->error_object);
        }
    } else {
        handle->error = error;
        if (error != 0 && handle->error_callback != 0) {
            handle->error_callback(handle->error_object);
        }
    }
    return error;
}

int MPS_Destroy(MpsHandle* handle) {
    if (mpslib_check_handle(handle) != 0) {
        return mpslib_set_error(0, 0xFF020103);
    }
    handle->state = 1;
    return 0;
}

MpsHandle* MPS_Create(void) {
    MpsHandle* handle;
    int i;
    int remaining;

    handle = MPSLIB_libwork->handles;
    remaining = MPSLIB_libwork->handle_count;
    while (remaining > 0) {
        if (handle->state == 1) {
            break;
        }
        handle++;
        remaining--;
    }
    if (remaining <= 0) {
        return 0;
    }

    UTY_MemsetDword((unsigned int*)handle, 0, 0x40);
    handle->state = 2;
    handle->format = 2;
    for (i = 0; i < 46; i++) {
        handle->payload.decoder_words[i] = -1;
    }
    handle->field_D0 = 0;
    handle->decode_header = MPSDEC_DecHdMpeg1;
    handle->field_D8 = 0;
    handle->field_DC = 0;
    handle->field_E0 = 0;
    handle->field_E4 = 0;
    handle->field_E8 = 0;
    return handle;
}

int MPSLIB_CheckHn(MpsHandle* handle) {
    mpslib_hn_last = handle;
    if (handle == 0) {
        return -1;
    }
    return -(handle->state == 1);
}

int MPS_SetErrFn(MpsHandle* handle, MpsErrorCallback callback, int object) {
    if (handle == 0) {
        MPSLIB_libwork->error_callback = callback;
        MPSLIB_libwork->error_object = object;
    } else {
        if (mpslib_check_handle(handle) != 0) {
            return mpslib_set_error(0, 0xFF020101);
        }
        handle->error_callback = callback;
        handle->error_object = object;
    }
    return 0;
}

int MPSLIB_SetErr(MpsHandle* handle, int error) {
    return mpslib_set_error(handle, error);
}

void MPS_Finish(void) {
    int i;
    MpsHandle* handle;

    handle = MPSLIB_libwork->handles;
    for (i = 0; i < MPSLIB_libwork->handle_count; i++, handle++) {
        if (handle->state != 1) {
            MPS_Destroy(handle);
        }
    }
    MPSDEC_Finish();
    MPSGET_Finish();
}

int MPS_Init(int handle_count, MpsLibWork* work) {
    static const unsigned char test_wrok[4] = {1, 2, 3, 4};
    int i;

    cri_verstr_ptr = MPSLIB_version_str;
    if (*(const unsigned char*)&test_wrok != 1) {
        for (;;) {
        }
    }

    MPSLIB_libwork = work;
    UTY_MemsetDword((unsigned int*)work, 0,
                    (((handle_count - 1) << 8) + 0x110) >> 2);
    work->error_callback = 0;
    work->error_object = 0;
    work->error = 0;
    work->handle_count = handle_count;
    for (i = 0; i < handle_count; i++) {
        work->handles[i].state = 1;
    }
    MPSDEC_Init();
    MPSGET_Init();
    return 0;
}
