#include "runtime/mk_hwfile.h"

#include "runtime/mk_proc.h"
#include "runtime/cstring.h"

static MkHwFileRequest handle_data[10];
static signed char handle_freelist[10];
static MkHwFileHandlePool handle_pool;
static int hwfile_initialized;
static int last_error;

static void require_file_opened(MkHwFileRequest* request);

int mk_hwfile_link_late_open_callback(MkHwFileRequest* request,
                                      MkHwFileOpenCallback callback,
                                      void* callback_arg) {
    if (request->mwFile == 0 && request->openCommand != 0 &&
        request->lateOpenCallback == 0) {
        request->lateOpenCallback = callback;
        request->lateOpenCallbackArg = callback_arg;
        return 1;
    }
    return 0;
}

int mk_hwfile_is_file_ready(MkHwFileRequest* request) {
    return request->mwFile != 0;
}

void mk_hwfile_busywait_dowork(void) { mwFileTick(); }

static void* async_to_mwFile(MkHwFileRequest* request) {
    require_file_opened(request);
    return request->mwFile;
}

void mk_hwfile_wait_for_completion(void** command) {
    mwFileAsyncResult result;

    if (command == 0 || *command == 0) {
        return;
    }
    while (*command != 0 &&
           !mwFileIsCommandCompleted((mwFileCommand*)*command, &result)) {
        if (aproc != 0 && aproc->stack_top != 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        } else {
            mwFileTick();
        }
    }
}

void mk_hwfile_wait_for_completion_or_null_request(MkHwFileRequest** command) {
    mwFileAsyncResult result;

    if (command == 0 || *command == 0) {
        return;
    }
    while (*command != 0 &&
           !mwFileIsCommandCompleted((mwFileCommand*)*command, &result)) {
        if (aproc != 0 && aproc->stack_top != 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        } else {
            mwFileTick();
        }
    }
}

int mk_hwfile_write_blocking(MkHwFileRequest* request, void* buffer,
                             int length) {
    void* command;
    int offset;

    if (request == 0 || buffer == 0) {
        return -1;
    }
    offset = mk_hwfile_tell(request);
    command = mwFileWriteAsync(async_to_mwFile(request), offset, buffer,
                               length, 1, 0, 0);
    if (command == 0) {
        return -1;
    }
    mk_hwfile_wait_for_completion(&command);
    if (command != 0) {
        mwFileFreeCommand(command);
    }
    mk_hwfile_seek(request, length, 1);
    return length;
}

MkHwFileRequest* mk_hwfile_open_blocking(const char* path, const char* mode) {
    MkHwFileRequest* request = mk_hwfile_open(path, mode);
    require_file_opened(request);
    if (request->mwFile == 0) {
        mk_hwfile_close(request);
        request = 0;
    }
    return request;
}

void mk_hwfile_free_request(void* command) {
    if (command != 0) {
        mwFileFreeCommand(command);
    }
}

void mk_hwfile_close(MkHwFileRequest* request) {
    void* command;
    int byte_offset;
    int index;

    if (request == 0 ||
        (request->mwFile == 0 && request->openCommand == 0)) {
        return;
    }
    require_file_opened(request);
    if (request->mwFile != 0) {
        command = mwFileCloseAsync(request->mwFile, 0, 0);
        request->mwFile = 0;
        mwFileFreeCommand(command);
    }
    if (request->openCommand != 0) {
        mwFileFreeCommand(request->openCommand);
        request->openCommand = 0;
    }
    if (request >= handle_pool.base &&
        request <= &handle_pool.base[handle_pool.count - 1]) {
        byte_offset = (request - handle_pool.base) * sizeof(*request);
        index = byte_offset / handle_pool.handleSize;
        if (byte_offset - index * handle_pool.handleSize == 0 &&
            handle_pool.freelist[index] != 0) {
            handle_pool.freelist[index] = 0;
            handle_pool.freeCount++;
        }
    }
}

int mk_hwfile_tell(MkHwFileRequest* request) {
    mwFileAsyncResult result;
    if (request == 0) {
        return -1;
    }
    result = mwFileTell(async_to_mwFile(request));
    return result.value.bytes;
}

int mk_hwfile_seek(MkHwFileRequest* request, int offset, int origin) {
    mwFileAsyncResult result;
    void* file;
    if (request == 0) {
        return -1;
    }
    file = async_to_mwFile(request);
    if (file == 0) {
        return -1;
    }
    result = mwFileSeek(file, offset, origin);
    return result.value.bytes;
}

void mk_hwfile_cancel(MkHwFileRequest* request) {
    if (request != 0 && request->mwFile != 0) {
        mwFileAbortCommand(request->mwFile);
        if (request->mwFile != 0) {
            mk_hwfile_wait_for_completion(&request->mwFile);
        }
    }
}

void* mk_hwfile_read_async(MkHwFileRequest* request, int offset, void* buffer,
                           int length) {
    if (request == 0 || buffer == 0) {
        return 0;
    }
    require_file_opened(request);
    return mwFileReadAsync(async_to_mwFile(request), offset, buffer, length, 1,
                           0, 0);
}

int mk_hwfile_read(MkHwFileRequest* request, void* buffer, int length) {
    void* command;
    mwFileAsyncResult result;

    if (buffer == 0 || request == 0 || length == 0) {
        return 0;
    }
    command = mk_hwfile_read_async(request, mk_hwfile_tell(request), buffer,
                                   length);
    if (command == 0) {
        return -1;
    }
    while (command != 0 && !mwFileIsCommandCompleted(command, &result)) {
        if (aproc != 0 && aproc->stack_top != 0) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        } else {
            mwFileTick();
        }
    }
    if (command != 0) {
        mwFileFreeCommand(command);
    }
    if (result.error != 0) {
        return -1;
    }
    mk_hwfile_seek(request, result.value.bytes, 1);
    return result.value.bytes;
}

static void open_callback(mwFileCommand* command, mwFileAsyncResult result, void* arg) {
    MkHwFileRequest* request = arg;
    MkHwFileOpenCallback callback;
    void* callback_arg;
    int success;

    if (result.error == 0) {
        request->mwFile = result.value.pointer;
        success = 1;
    } else {
        last_error = result.error;
        request->mwFile = 0;
        success = 0;
    }
    callback = request->lateOpenCallback;
    callback_arg = request->lateOpenCallbackArg;
    request->lateOpenCallback = 0;
    request->lateOpenCallbackArg = 0;
    if (callback != 0) {
        callback(callback_arg, request, success);
    }
}

MkHwFileRequest* mk_hwfile_open(const char* path, const char* mode) {
    char path_buffer[256];
    MkHwFileRequest* request;
    int index;

    if (path == 0 || mode == 0 || strlen(path) >= 250) {
        return 0;
    }
    path_buffer[0] = '\0';
    strcat(path_buffer, path);
    for (index = 0;
         index < handle_pool.count && handle_pool.freelist[index] != 0;
         index++) {
    }
    if (index == handle_pool.count) {
        request = 0;
    } else {
        handle_pool.freeCount--;
        handle_pool.freelist[index] = 1;
        request = &handle_pool.base[index];
    }
    if (request == 0) {
        return 0;
    }
    memset(request, 0, sizeof(*request));
    request->lateOpenCallback = 0;
    request->lateOpenCallbackArg = 0;
    request->openCommand = mwFileOpenAsync(
        path_buffer, mwFileOpenModeToFlags(mode), open_callback, request);
    if (request->openCommand == 0) {
        int byte_offset;
        int release_index;
        if (request >= handle_pool.base &&
            request <= &handle_pool.base[handle_pool.count - 1]) {
            byte_offset = (request - handle_pool.base) * sizeof(*request);
            release_index = byte_offset / handle_pool.handleSize;
            if (byte_offset - release_index * handle_pool.handleSize == 0 &&
                handle_pool.freelist[release_index] != 0) {
                handle_pool.freelist[release_index] = 0;
                handle_pool.freeCount++;
            }
        }
        return 0;
    }
    return request;
}

void mk_hwfile_init(void) {
    if (!hwfile_initialized) {
        handle_pool.count = 10;
        handle_pool.freeCount = 10;
        handle_pool.base = handle_data;
        handle_pool.handleSize = sizeof(MkHwFileRequest);
        handle_pool.freelist = handle_freelist;
        memset(handle_freelist, 0, 10);
        hwfile_initialized = 1;
    }
}

static void require_file_opened(MkHwFileRequest* request) {
    MkProc* saved_aproc;

    if (request != 0 && request->mwFile == 0 && request->openCommand != 0) {
        saved_aproc = aproc;
        aproc = 0;
        mk_hwfile_wait_for_completion(&request->openCommand);
        if (request->openCommand != 0) {
            mwFileFreeCommand(request->openCommand);
        }
        request->openCommand = 0;
        aproc = saved_aproc;
    }
}
