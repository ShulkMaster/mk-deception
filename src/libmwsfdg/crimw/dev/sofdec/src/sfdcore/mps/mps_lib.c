#include "cri/mps.h"
#include "sofdec/uty_mem.h"

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
        MpsLibWork* work = MPSLIB_libwork;

        work->error = error;
        if (error != 0 && work->error_callback != 0) {
            work->error_callback(work->error_object, error);
        }
    } else {
        handle->error = error;
        if (error != 0 && handle->error_callback != 0) {
            handle->error_callback(handle->error_object, error);
        }
    }
    return error;
}

int MPS_Destroy(MpsHandle* handle) {
    int invalid;

    mpslib_hn_last = handle;
    if (handle == 0) {
        invalid = -1;
    } else if (handle->state == 1) {
        invalid = -1;
    } else {
        invalid = 0;
    }
    if (invalid != 0) {
        MpsLibWork* work = MPSLIB_libwork;
        int error = 0xFF020103;

        work->error = error;
        if (work->error_callback != 0) {
            work->error_callback(work->error_object, error);
        }
        return error;
    }
    handle->state = 1;
    return 0;
}

static inline MpsHandle* mpslib_get_free_handle(void) {
    MpsLibWork* work = MPSLIB_libwork;
    MpsHandle* handle = work->handles;
    int i;

    for (i = 0; i < work->handle_count; i++) {
        if (handle->state == 1) {
            return handle;
        }
        handle++;
    }
    return 0;
}

MpsHandle* MPS_Create(void) {
    MpsHandle* handle;
    int i;

    handle = mpslib_get_free_handle();
    if (handle == 0) {
        return 0;
    }

    UTY_MemsetDword((unsigned int*)handle, 0,
                    sizeof(*handle) / sizeof(unsigned int));
    handle->state = 2;
    handle->error_callback = 0;
    handle->error_object = 0;
    handle->error = 0;
    handle->packet_length_bytes = 2;
    handle->headers.pack_header.scr = -1;
    handle->headers.pack_header.is_mpeg1 = -1;
    handle->headers.pack_header.mux_rate = -1;
    handle->headers.last_system_header.header_length = -1;
    handle->headers.last_system_header.rate_bound = -1;
    handle->headers.last_system_header.audio_bound = -1;
    handle->headers.last_system_header.video_bound = -1;
    handle->headers.last_system_header.fixed_flag = -1;
    handle->headers.last_system_header.csps_flag = -1;
    handle->headers.last_system_header.audio_lock_flag = -1;
    handle->headers.last_system_header.video_lock_flag = -1;
    for (i = 0; i < 3; i++) {
        MpsSystemHeader* header = &handle->headers.system_headers[i];

        header->header_length = -1;
        header->rate_bound = -1;
        header->audio_bound = -1;
        header->video_bound = -1;
        header->fixed_flag = -1;
        header->csps_flag = -1;
        header->audio_lock_flag = -1;
        header->video_lock_flag = -1;
    }
    handle->headers.packet_header.pts = -1;
    handle->headers.packet_header.dts = -1;
    handle->headers.packet_header.stream_id = -1;
    handle->headers.packet_header.stream_type = -1;
    handle->headers.packet_header.stream_index = -1;
    handle->headers.packet_header.packet_length = -1;
    handle->headers.packet_header.std_buffer_size = -1;
    handle->headers.packet_header.payload_length = -1;
    handle->field_D0 = 0;
    handle->decode_header = MPSDEC_DecHdMpeg1;
    handle->field_D8 = 0;
    handle->field_DC = 0;
    handle->field_E0 = 0;
    handle->system_callback = 0;
    handle->system_object = 0;
    return handle;
}

int MPSLIB_CheckHn(MpsHandle* handle) {
    mpslib_hn_last = handle;
    if (handle == 0) {
        return -1;
    }
    return -(handle->state == 1);
}

int MPS_SetErrFn(MpsHandle* handle, MpsErrorCallback callback,
                 MpsCallbackObject object) {
    int invalid;

    if (handle == 0) {
        MpsLibWork* work = MPSLIB_libwork;

        work->error_callback = callback;
        work->error_object = object;
    } else {
        mpslib_hn_last = handle;
        if (handle == 0) {
            invalid = -1;
        } else if (handle->state == 1) {
            invalid = -1;
        } else {
            invalid = 0;
        }
        if (invalid != 0) {
            MpsLibWork* work = MPSLIB_libwork;
            int error = 0xFF020101;

            work->error = error;
            if (work->error_callback != 0) {
                work->error_callback(work->error_object, error);
            }
            return error;
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
    MpsLibWork* work;
    MpsHandle* handle;
    int i;
    int handle_count;

    work = MPSLIB_libwork;
    handle_count = work->handle_count;
    handle = work->handles;
    for (i = 0; i < handle_count; i++) {
        if (handle->state != 1) {
            MPS_Destroy(handle);
        }
        handle++;
    }
    MPSDEC_Finish();
    MPSGET_Finish();
}

static int mpslib_clear_handles(MpsHandle* handles, int count) {
    int i;

    for (i = 0; i < count; i++) {
        handles[i].state = 1;
    }
    return 0;
}

/* TODO: [near miss] 96.419754%; typed handle-clear helper matches the loop;
 * only retail's eliminated zero-result branch pair remains: soft ceiling. */
int MPS_Init(int handle_count, MpsLibWork* work) {
    static const unsigned int test_wrok = 0x01020304;
    MpsLibWork* libwork;
    int result;

    cri_verstr_ptr = MPSLIB_version_str;
    if (*(const unsigned char*)&test_wrok != 1) {
        for (;;) {
            ((void (*)(void))-1)();
        }
    }

    MPSLIB_libwork = work;
    UTY_MemsetDword((unsigned int*)work, 0,
                    (sizeof(MpsLibWork) +
                     (handle_count - 1) * sizeof(MpsHandle)) /
                        sizeof(unsigned int));
    libwork = MPSLIB_libwork;
    libwork->error_callback = 0;
    libwork->error_object = 0;
    libwork->error = 0;
    MPSLIB_libwork->handle_count = handle_count;
    result = mpslib_clear_handles(MPSLIB_libwork->handles, handle_count);
    if (result != 0) {
        return result;
    }
    MPSDEC_Init();
    MPSGET_Init();
    return 0;
}
