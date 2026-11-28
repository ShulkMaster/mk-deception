#include "msl/mslStreamFile.h"
#include "msl/mslStreamFile_internal.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"
#include "msl/mslsupport.h"

union mslDSB_RequestHandle {
    unsigned int value;
    struct {
        unsigned short index;
        unsigned short generation;
    } parts;
};

struct mslDSB_PendingAsyncRead;

struct mslDSB_PendingFileRead {
    mslDSB_PendingAsyncRead* owner;
    mslDSB_PendingFileRead* next;
    mslDSB_PendingFileRead* previous;
    void* buffer;
    mwFileCommand* command;
    unsigned int offset;
    unsigned int size;
    unsigned int callback_offset;
    unsigned int callback_size;
    void* callback_buffer;
};

struct mslDSB_PendingAsyncRead {
    mslDSB_PendingAsyncRead* next;
    mslDSB_PendingFileRead* first_read;
    mslDSB_PendingFileRead* last_read;
    _mwFile* file;
    unsigned char priority;
    unsigned char pad11[3];
    unsigned int request_offset;
    unsigned int request_size;
    unsigned int remaining;
    unsigned int next_offset;
    unsigned char in_use;
    unsigned char queued;
    unsigned char final_issued;
    unsigned char error;
    int active_reads;
    mslDSB_RequestHandle handle;
    mslStreamFileCallback callback;
    void* callback_data;

    void UnlinkRead(mslDSB_PendingFileRead* read) {
        if (read->previous != 0) {
            mslDSB_PendingFileRead* next = read->next;
            read->previous->next = next;
            if (next != 0) {
                read->next->previous = read->previous;
            } else {
                last_read = read->previous;
            }
        } else {
            mslDSB_PendingFileRead* next = read->next;
            first_read = next;
            if (next != 0) {
                first_read->previous = 0;
            } else {
                last_read = 0;
            }
        }
        read->owner = 0;
    }
};

struct mslDSB_PendingQueue {
    mslDSB_PendingAsyncRead* first;
    mslDSB_PendingAsyncRead** last_link;
};

mslDSB_PendingAsyncRead DSB_PAR_Pool[32];
mslDSB_PendingFileRead DSB_FILEREAD_Pool[5];

unsigned char g_DSB_Buffers[5][0x4000] __attribute__((aligned(32)));

mslDSB_PendingAsyncRead* DSB_PAR_FreeList;
mslDSB_PendingQueue DSB_PAR_Queue;
mslDSB_PendingFileRead* DSB_FILEREAD_FreeList;
unsigned char g_DSB_BufferFree[5];

static void DSB_PAR_init(void) {
    mslDSB_PendingAsyncRead* requests;
    int i;

    DSB_PAR_Queue.first = 0;
    DSB_PAR_Queue.last_link = &DSB_PAR_Queue.first;
    memset(DSB_PAR_Pool, 0, sizeof(DSB_PAR_Pool));
    DSB_PAR_FreeList = DSB_PAR_Pool;
    requests = DSB_PAR_Pool;
    for (i = 0; i < 32; i += 8, requests += 8) {
        requests[0].next = &DSB_PAR_Pool[i + 1];
        requests[0].handle.parts.index = i + 1;
        requests[1].next = &DSB_PAR_Pool[i + 2];
        requests[1].handle.parts.index = i + 2;
        requests[2].next = &DSB_PAR_Pool[i + 3];
        requests[2].handle.parts.index = i + 3;
        requests[3].next = &DSB_PAR_Pool[i + 4];
        requests[3].handle.parts.index = i + 4;
        requests[4].next = &DSB_PAR_Pool[i + 5];
        requests[4].handle.parts.index = i + 5;
        requests[5].next = &DSB_PAR_Pool[i + 6];
        requests[5].handle.parts.index = i + 6;
        requests[6].next = &DSB_PAR_Pool[i + 7];
        requests[6].handle.parts.index = i + 7;
        requests[7].next = &DSB_PAR_Pool[i + 8];
        requests[7].handle.parts.index = i + 8;
    }
    DSB_PAR_Pool[31].next = 0;
}

static void DSB_PAR_reset_values(mslDSB_PendingAsyncRead* request) {
    unsigned int handle = request->handle.value;
    memset(request, 0, sizeof(*request));
    request->handle.value = handle;
}

static mslDSB_PendingAsyncRead* DSP_PAR_convert_handle_to_request(
    unsigned long handle) {
    mslDSB_RequestHandle value;
    int index;
    mslDSB_PendingAsyncRead* request;

    value.value = handle;
    index = value.parts.index;
    if (index >= 1 && index <= 32 &&
        (request = &DSB_PAR_Pool[index - 1],
         value.value == request->handle.value) &&
        request->in_use != 0) {
        return request;
    }
    return 0;
}

static mslDSB_PendingAsyncRead* DSB_PAR_alloc(void) {
    u32 enabled = OSDisableInterrupts();
    mslDSB_PendingAsyncRead* request = DSB_PAR_FreeList;

    if (request != 0) {
        DSB_PAR_FreeList = request->next;
        DSB_PAR_reset_values(request);
        request->handle.parts.generation++;
        request->in_use = 1;
    }
    OSRestoreInterrupts(enabled);
    if (request == 0) {
        mslDebugPrintf("Ran out of mslDSB_PendingAsyncRead structures.  Increase MSL_STREAM_MAXPENDINGREADS");
    }
    return request;
}

static void i_DSB_PAR_free(mslDSB_PendingAsyncRead* request) {
    request->in_use = 0;
    DSB_PAR_reset_values(request);
    request->next = DSB_PAR_FreeList;
    DSB_PAR_FreeList = request;
}

static void DSB_PAR_free(mslDSB_PendingAsyncRead* request) {
    u32 enabled;

    if (request->active_reads != 0) {
        mslDebugPrintf("DSB Request resources not properly freed");
    }
    if (request->queued) {
        mslDebugPrintf("DSB Can not free Request while on queue");
    }
    if (!request->in_use) {
        mslDebugPrintf("DSB Invalid Request to free");
    }
    enabled = OSDisableInterrupts();
    i_DSB_PAR_free(request);
    OSRestoreInterrupts(enabled);
}

static mslDSB_PendingAsyncRead* DSB_PAR_queue_head(void) {
    u32 enabled = OSDisableInterrupts();
    mslDSB_PendingAsyncRead* request = DSB_PAR_Queue.first;

    OSRestoreInterrupts(enabled);
    return request;
}

static mslDSB_PendingAsyncRead* i_DSB_PAR_dequeue(void) {
    mslDSB_PendingAsyncRead* request = DSB_PAR_Queue.first;

    if (request != 0) {
        mslDSB_PendingAsyncRead* next = request->next;

        request->next = 0;
        DSB_PAR_Queue.first = next;
        if (next == 0) {
            DSB_PAR_Queue.last_link = &DSB_PAR_Queue.first;
        }
        request->queued = 0;
    }
    return request;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x64); locked outer of i_DSB_PAR_dequeue (i_/outer pattern of DSB_PAR_free), compiles to 0x64. */
static mslDSB_PendingAsyncRead* DSB_PAR_dequeue(void) {
    u32 enabled = OSDisableInterrupts();
    mslDSB_PendingAsyncRead* request = i_DSB_PAR_dequeue();

    OSRestoreInterrupts(enabled);
    return request;
}

static void i_DSB_PAR_enqueue(mslDSB_PendingAsyncRead* request) {
    *DSB_PAR_Queue.last_link = request;
    DSB_PAR_Queue.last_link = &request->next;
    request->next = 0;
    request->queued = 1;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x50); locked outer of i_DSB_PAR_enqueue (i_/outer pattern of DSB_PAR_free), compiles to 0x50. */
static void DSB_PAR_enqueue(mslDSB_PendingAsyncRead* request) {
    u32 enabled = OSDisableInterrupts();

    i_DSB_PAR_enqueue(request);
    OSRestoreInterrupts(enabled);
}

static void DSB_PAR_queue_remove(mslDSB_PendingAsyncRead* request) {
    u32 enabled;
    mslDSB_PendingAsyncRead** link;
    mslDSB_PendingAsyncRead* queued;

    mslDebugPrintf("     - in queue!\n");
    enabled = OSDisableInterrupts();
    queued = DSB_PAR_Queue.first;
    link = &DSB_PAR_Queue.first;
    while (queued != 0) {
        if (request == queued) {
            mslDSB_PendingAsyncRead* next = queued->next;
            *link = next;
            if (next == 0) {
                DSB_PAR_Queue.last_link = link;
            }
            request->next = 0;
            request->queued = 0;
            break;
        }
        link = &queued->next;
        queued = queued->next;
    }
    OSRestoreInterrupts(enabled);
}

static mslDSB_PendingFileRead* i_DSB_FILEREAD_alloc(void) {
    mslDSB_PendingFileRead* read = DSB_FILEREAD_FreeList;
    if (read != 0) {
        DSB_FILEREAD_FreeList = read->next;
        memset(read, 0, sizeof(*read));
    }
    return read;
}

static void i_DSB_FILEREAD_free(mslDSB_PendingFileRead* read) {
    if (read != 0) {
        memset(read, 0, sizeof(*read));
        read->next = DSB_FILEREAD_FreeList;
        DSB_FILEREAD_FreeList = read;
    }
}

static void* i_mslDSB_GetBuffer(void) {
    int i;
    for (i = 0; i < 5; i++) {
        if (g_DSB_BufferFree[i]) {
            g_DSB_BufferFree[i] = 0;
            return g_DSB_Buffers[i];
        }
    }
    return 0;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x7c); locked outer of i_mslDSB_GetBuffer (i_/outer pattern of DSB_PAR_free), compiles to 0x7c. */
static void* mslDSB_GetBuffer(void) {
    u32 enabled = OSDisableInterrupts();
    void* buffer = i_mslDSB_GetBuffer();

    OSRestoreInterrupts(enabled);
    return buffer;
}

static int i_mslDSB_FreeBuffer(void* buffer) {
    int result;
    int difference = (uptr)buffer - (uptr)g_DSB_Buffers;
    int index;

    if (difference >= 0 &&
        (index = difference / (int)sizeof(g_DSB_Buffers[0])) < 5) {
        result = 1;
        g_DSB_BufferFree[index] = 1;
    } else {
        mslDebugPrintf("MSL ERROR: Unable to free Dynamic Stream Buffer\n");
        result = 0;
    }
    return result;
}

static int mslDSB_FreeBuffer(void* buffer) {
    int result;
    int service;
    u32 enabled;

    if (buffer == 0) {
        return 0;
    }
    service = 0;
    enabled = OSDisableInterrupts();
    result = i_mslDSB_FreeBuffer(buffer);
    if (DSB_PAR_queue_head() != 0) {
        service = 1;
    }
    OSRestoreInterrupts(enabled);
    if (service) {
        mslDSB_ServiceNextRead();
    }
    return result;
}

static mslDSB_PendingAsyncRead* mslDSB_GetNewPendingRead(void) {
    mslDSB_PendingAsyncRead* request = DSB_PAR_alloc();

    while (request == 0) {
        mslDebugPrintf("MSL ERROR: Out of DSB Pending Reads - Increase MSL_STREAM_MAXPENDINGREADS\n");
        request = DSB_PAR_alloc();
    }
    return request;
}

static void mslDSB_FileReadCompletionCallback(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {

    mslDSB_PendingAsyncRead* request;
    mslDSB_PendingFileRead* read;
    mslStreamFileCallback callback;
    void* data;
    int requeue;
    int final;
    int error;
    unsigned int offset;
    unsigned int size;
    int enabled;
    void* buffer;

    read = (mslDSB_PendingFileRead*)callback_data;

    if (read->command != command) {
        mwFileFreeCommand(command);
        return;
    }
    request = read->owner;
    enabled = OSDisableInterrupts();

    request->UnlinkRead(read);
    final = 0;
    requeue = 0;
    error = 0;
    request->active_reads--;
    data = request->callback_data;
    callback = request->callback;
    buffer = read->buffer;
    read->buffer = 0;
    if (request->error) {
        error = 0x44DEAD01;
    }

    if (callback != 0) {
        offset = read->callback_offset;
        size = read->callback_size;
        offset -= request->request_offset;

        if (error == 0) {
            if (request->final_issued) {
                if (request->active_reads == 0) {
                    final = 1;
                }
            } else if (request->active_reads <= 1) {
                requeue = 1;
            }
        } else {
            request->error = 1;
            final = 1;
            request->final_issued = 1;
            offset = 0;
            i_mslDSB_FreeBuffer(buffer);
            buffer = 0;
        }
        callback(buffer, offset, size, error, final, data);
        if (error != 0) {
            request->callback = 0;
            callback = 0;
        }
    } else {
        i_mslDSB_FreeBuffer(buffer);
    }

    if (requeue) {
        if (!request->queued) {
            i_DSB_PAR_enqueue(request);
            callback = 0;
        }
    } else if (request->active_reads == 0) {
        i_DSB_PAR_free(request);
    }
    i_DSB_FILEREAD_free(read);
    OSRestoreInterrupts(enabled);
    mwFileFreeCommand(command);
    if (callback == 0) {
        mslDSB_ServiceNextRead();
    }
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x20); body inferred from the name (calls mslDSB_ServiceNextRead), compiles to 0x20. */
static void mslDSB_Service_TickCallback(void* callback_data) {
    mslDSB_ServiceNextRead();
}

void mslDSB_ServiceNextRead(void) {
    mslDSB_PendingAsyncRead* request = 0;
    mslDSB_PendingFileRead* read = 0;
    void* buffer = 0;
    int started = 0;
    u32 enabled = OSDisableInterrupts();

    request = DSB_PAR_queue_head();
    if (request != 0) {
        buffer = i_mslDSB_GetBuffer();
        if (buffer != 0) {
            read = i_DSB_FILEREAD_alloc();
            request = i_DSB_PAR_dequeue();
            request->active_reads++;
            started = 1;
        } else {
            request = 0;
        }
    }
    OSRestoreInterrupts(enabled);

    if (started) {
        while (read != 0) {
            unsigned int offset;
            unsigned int size;
            u32 inner = OSDisableInterrupts();

            size = request->remaining;
            if (size > sizeof(g_DSB_Buffers[0])) {
                size = sizeof(g_DSB_Buffers[0]);
            }
            offset = request->next_offset;
            read->owner = request;
            if ((read->previous = request->last_read) != 0) {
                request->last_read->next = read;
            } else {
                request->first_read = read;
            }
            request->last_read = read;
            read->next = 0;
            read->buffer = buffer;
            read->offset = offset;
            read->size = size;
            read->callback_offset = offset;
            read->callback_size = size;
            read->callback_buffer = buffer;
            request->remaining -= read->callback_size;
            request->next_offset += read->callback_size;
            if (request->remaining == 0) {
                request->final_issued = 1;
            }
            OSRestoreInterrupts(inner);

            read->command = mwFileReadAsync(
                request->file, read->offset, buffer, read->size,
                request->priority, mslDSB_FileReadCompletionCallback, read);
            read = 0;
            if (request->remaining != 0 && request->active_reads < 3) {
                u32 more_enabled = OSDisableInterrupts();
                buffer = i_mslDSB_GetBuffer();
                if (buffer != 0) {
                    read = i_DSB_FILEREAD_alloc();
                    request->active_reads++;
                }
                OSRestoreInterrupts(more_enabled);
            }
        }
    }
}

static mslDSB_PendingAsyncRead* mslDSB_ReadAsync(
    _mwFile* file, unsigned long long offset, unsigned int size,
    unsigned char priority, mslStreamFileCallback callback,
    void* callback_data, int from_interrupt) {
    mslDSB_PendingAsyncRead* request;
    u32 enabled;

    if (size == 0) {
        return 0;
    }
    request = mslDSB_GetNewPendingRead();
    enabled = OSDisableInterrupts();
    request->file = file;
    request->priority = priority;
    request->callback = callback;
    request->callback_data = callback_data;
    request->request_offset = offset;
    request->request_size = size;
    request->next_offset = offset;
    request->remaining = size;
    i_DSB_PAR_enqueue(request);
    OSRestoreInterrupts(enabled);
    if (from_interrupt == 0) {
        mslDSB_ServiceNextRead();
    }
    return request;
}

static void mslDSB_CancelRead(
    mslDSB_PendingAsyncRead* request, int invoke_callback) {
    if (request != 0) {
        u32 enabled = OSDisableInterrupts();

        if (request->in_use) {
            mslDSB_PendingFileRead* read;
            mslStreamFileCallback callback;
            void* callback_data;

            mslDebugPrintf(
                "     - in progress: %d\n", request->active_reads);
            if (request->active_reads != 0) {
                read = request->last_read;
                while (read != 0) {
                    request->UnlinkRead(read);
                    if (read->command != 0) {
                        mslDebugPrintf(
                            "          - abort: 0x%08x\n", read->command);
                        mwFileAbortCommand(read->command);
                        mwFileFreeCommand(read->command);
                        read->command = 0;
                    }
                    if (read->buffer != 0) {
                        i_mslDSB_FreeBuffer(read->buffer);
                        read->buffer = 0;
                    }
                    i_DSB_FILEREAD_free(read);
                    read = request->last_read;
                }
                request->active_reads = 0;
            }

            if (request->queued) {
                DSB_PAR_queue_remove(request);
            }

            callback = request->callback;
            request->callback = 0;
            callback_data = request->callback_data;
            request->callback_data = 0;
            mslDebugPrintf("          - callback: 0x%08x\n", callback);
            if (invoke_callback && callback != 0) {
                callback(0, -1, 0, 0x44DEAD0F, 1, callback_data);
            }
            DSB_PAR_free(request);
        }
        OSRestoreInterrupts(enabled);
    }
}

static void mslDSB_Init(void) {
    int i;

    DSB_PAR_init();
    DSB_FILEREAD_FreeList = DSB_FILEREAD_Pool;
    for (i = 0; i < 5; i++) {
        g_DSB_BufferFree[i] = 1;
        DSB_FILEREAD_Pool[i].next = &DSB_FILEREAD_Pool[i + 1];
    }
    DSB_FILEREAD_Pool[4].next = 0;
}

static void internal_mslStreamFile_CancelRequest(void* request) {
    mslDSB_CancelRead((mslDSB_PendingAsyncRead*)request, 1);
}

static void* internal_mslStreamFile_ConvertHandleToRequest(
    unsigned long handle) {
    return DSP_PAR_convert_handle_to_request(handle);
}

extern "C" mslStreamFileRequest* mslStreamFile_QueueRequest(
    _mwFile* file, u32 offset, u32 size, int priority,
    mslStreamFileCallback callback, void* callback_data) {
    mslDSB_PendingAsyncRead* request = mslDSB_ReadAsync(
        file, offset, size, priority, callback, callback_data, 0);

    if (request != 0) {
        return (mslStreamFileRequest*)(uptr)request->handle.value;
    }
    return 0;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x1ac); QueueRequest body with from_interrupt 1 plus a tick-queued
 * mslDSB_Service_TickCallback (ReturnBuffer_FromInterrupt pattern), compiles to 0x1ac; queue call placement unverified. */
extern "C" mslStreamFileRequest* mslStreamFile_QueueRequest_FromInterrupt(
    _mwFile* file, u32 offset, u32 size, int priority,
    mslStreamFileCallback callback, void* callback_data) {
    mslDSB_PendingAsyncRead* request = mslDSB_ReadAsync(
        file, offset, size, priority, callback, callback_data, 1);

    if (request != 0) {
        mslTickCallBack_Queue(mslDSB_Service_TickCallback, 0);
        return (mslStreamFileRequest*)(uptr)request->handle.value;
    }
    return 0;
}

extern "C" int mslStreamFile_ReturnBuffer(void* buffer) {
    return mslDSB_FreeBuffer(buffer);
}

static void mslStreamFile_ReturnBuffer_CB(void* buffer) {
    mslDSB_FreeBuffer(buffer);
}

extern "C" void mslStreamFile_ReturnBuffer_FromInterrupt(void* buffer) {
    mslTickCallBack_Queue(mslStreamFile_ReturnBuffer_CB, buffer);
}

extern "C" void mslStreamFile_Initialize(void) {
    mslDSB_Init();
}

extern "C" void mslStreamFile_CancelRequest(void* handle) {
    void* request = internal_mslStreamFile_ConvertHandleToRequest((uptr)handle);
    if (request != 0) {
        internal_mslStreamFile_CancelRequest(request);
    }
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x8); returns the DSB_PAR_Pool entry count, compiles to 0x8. */
extern "C" int mslStreamFile_GetMaxPendingRequests(void) {
    return sizeof(DSB_PAR_Pool) / sizeof(DSB_PAR_Pool[0]);
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x8); returns the g_DSB_Buffers row count (num vs max unverified), compiles to 0x8. */
extern "C" int mslStreamFile_GetMaxBuffers(void) {
    return sizeof(g_DSB_Buffers) / sizeof(g_DSB_Buffers[0]);
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x8); returns the g_DSB_Buffers row count (num vs max unverified), compiles to 0x8. */
extern "C" int mslStreamFile_GetNumBuffers(void) {
    return sizeof(g_DSB_Buffers) / sizeof(g_DSB_Buffers[0]);
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x8); returns the g_DSB_Buffers row size, compiles to 0x8. */
extern "C" int mslStreamFile_GetSizeBuffer(void) {
    return sizeof(g_DSB_Buffers[0]);
}
