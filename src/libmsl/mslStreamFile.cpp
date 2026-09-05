/*
 * Dynamic stream-file reader. Retail owns five 0x4000-byte buffers, five
 * in-flight read records, and 32 generation-tagged pending requests.
 *
 * The interrupt return wrapper and Initialize are report-exact. DMA-buffer
 * alignment now agrees with retail; pointer-validation portability remains
 * under review. ReturnBuffer and ServiceNextRead have retail operations and
 * narrow temporary/list-link scheduling residue. CancelRequest and QueueRequest
 * retain one short-circuit branch and allocator/string lifetime differences.
 * CancelRead (~89.76%) and FileReadCompletionCallback (90.00%) retain retail
 * ownership, calls, and algorithms; their residue is list-reload scheduling
 * and register allocation, with no opcode mismatch.
 */
#include "msl/mslBank.h"
#include "msl/mslStreamFile.h"
#include "msl/mslStreamFile_internal.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"
#include "msl/mslsupport.h"


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int mslDSB_RequestHandleValue;

union mslDSB_RequestHandle {
    mslDSB_RequestHandleValue value;
    struct {
        u16 index;
        u16 generation;
    } parts;
};

static inline unsigned int mslDSB_HandleIndex(
    mslDSB_RequestHandleValue handle) {
    return handle >> 16;
}

static inline mslDSB_RequestHandleValue mslDSB_HandleFromOpaque(
    void* opaque_handle) {
    return (mslDSB_RequestHandleValue)(unsigned long)opaque_handle;
}

static inline void* mslDSB_HandleToOpaque(
    mslDSB_RequestHandleValue handle) {
    return (void*)(unsigned long)handle;
}

static const char stringBase0[] =
    "MSL ERROR: Unable to free Dynamic Stream Buffer\n\0"
    "Ran out of mslDSB_PendingAsyncRead structures.  Increase MSL_STREAM_MAXPENDINGREADS\0"
    "MSL ERROR: Out of DSB Pending Reads - Increase MSL_STREAM_MAXPENDINGREADS\n\0"
    "     - in progress: %d\n\0"
    "          - abort: 0x%08x\n\0"
    "     - in queue!\n\0"
    "          - callback: 0x%08x\n\0"
    "DSB Request resources not properly freed\0"
    "DSB Can not free Request while on queue\0"
    "DSB Invalid Request to free";

#define STREAM_STRING(offset) (&stringBase0[(offset)])

struct mslDSB_PendingAsyncRead;

struct mslDSB_FileRead {
    mslDSB_PendingAsyncRead* owner;
    mslDSB_FileRead* next;
    mslDSB_FileRead* previous;
    void* buffer;
    mwFileCommand* command;
    u32 offset;
    u32 size;
    u32 callback_offset;
    u32 callback_size;
    void* callback_buffer;
};

struct mslDSB_PendingAsyncRead {
    mslDSB_PendingAsyncRead* next;
    mslDSB_FileRead* first_read;
    mslDSB_FileRead* last_read;
    _mwFile* file;
    u8 priority;
    u8 pad11[3];
    u32 request_offset;
    u32 request_size;
    u32 remaining;
    u32 next_offset;
    u8 in_use;
    u8 queued;
    u8 final_issued;
    u8 error;
    int active_reads;
    mslDSB_RequestHandle handle;
    mslStreamFileCallback callback;
    void* callback_data;
};

struct mslDSB_PendingQueue {
    mslDSB_PendingAsyncRead* first;
    mslDSB_PendingAsyncRead** last_link;
};

mslDSB_PendingAsyncRead DSB_PAR_Pool[32];
mslDSB_FileRead DSB_FILEREAD_Pool[5];
/* Retail DMA buffers require 32-byte alignment at .bss+0x7E0. Data pooling
 * and common-symbol flags cannot express it without changing other layouts. */
u8 g_DSB_Buffers[5][0x4000] __attribute__((aligned(32)));

mslDSB_PendingAsyncRead* DSB_PAR_FreeList;
mslDSB_PendingQueue DSB_PAR_Queue;
mslDSB_FileRead* DSB_FILEREAD_FreeList;
u8 g_DSB_BufferFree[5];

static void mslDSB_CancelRead(mslDSB_PendingAsyncRead*, int);
static void mslDSB_FileReadCompletionCallback(
    mwFileCommand*, _mwFileAsyncResult, void*);
static void mslStreamFile_ReturnBuffer_CB(void*);

static inline int mslDSB_ReturnBuffer(void* buffer) {
    int result;
    int difference = (u8*)buffer - &g_DSB_Buffers[0][0];
    int index;

    if (difference >= 0 &&
        (index = difference / 0x4000) < 5) {
        result = 1;
        g_DSB_BufferFree[index] = 1;
    } else {
        mslDebugPrintf(STREAM_STRING(0));
        result = 0;
    }
    return result;
}

extern "C" void mslStreamFile_CancelRequest(void* handle) {
    mslDSB_RequestHandle value;
    int index;
    mslDSB_PendingAsyncRead* request;

    value.value = mslDSB_HandleFromOpaque(handle);
    index = value.parts.index;
    if (!(index >= 1 && index <= 32 &&
          (request = &DSB_PAR_Pool[index - 1],
           value.value == request->handle.value) &&
          request->in_use != 0)) {
        request = 0;
    }
    if (request != 0) {
        mslDSB_CancelRead(request, 1);
    }
}

/* Matched: 100% report-exact after restoring the retail DMA-buffer alignment. */
extern "C" void mslStreamFile_Initialize(void) {
    int i;
    mslDSB_PendingAsyncRead* requests;

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

    DSB_FILEREAD_FreeList = DSB_FILEREAD_Pool;
    for (i = 0; i < 5; i++) {
        g_DSB_BufferFree[i] = 1;
        DSB_FILEREAD_Pool[i].next = &DSB_FILEREAD_Pool[i + 1];
    }
    DSB_FILEREAD_Pool[4].next = 0;
}

extern "C" void mslStreamFile_ReturnBuffer_FromInterrupt(void* buffer) {
    mslTickCallBack_Queue(mslStreamFile_ReturnBuffer_CB, buffer);
}

static void mslStreamFile_ReturnBuffer_CB(void* buffer) {
    int service;

    if (buffer != 0) {
        service = 0;
        unsigned long enabled = OSDisableInterrupts();
        mslDSB_PendingAsyncRead* queued;

        mslDSB_ReturnBuffer(buffer);
        {
            unsigned long inner = OSDisableInterrupts();
            queued = DSB_PAR_Queue.first;
            OSRestoreInterrupts(inner);
        }
        if (queued != 0) {
            service = 1;
        }
        OSRestoreInterrupts(enabled);
        if (service) {
            mslDSB_ServiceNextRead();
        }
    }
}

extern "C" int mslStreamFile_ReturnBuffer(void* buffer) {
    unsigned long enabled;
    int service;
    int result;

    if (buffer == 0) {
        result = 0;
    } else {
        mslDSB_PendingAsyncRead* queued;

        service = 0;
        enabled = OSDisableInterrupts();
        result = mslDSB_ReturnBuffer(buffer);
        {
            unsigned long inner = OSDisableInterrupts();
            queued = DSB_PAR_Queue.first;
            OSRestoreInterrupts(inner);
        }
        if (queued != 0) {
            service = 1;
        }
        OSRestoreInterrupts(enabled);
        if (service) {
            mslDSB_ServiceNextRead();
        }
    }
    return result;
}

static inline mslDSB_PendingAsyncRead* mslDSB_AllocPending(void) {
    unsigned long enabled = OSDisableInterrupts();
    mslDSB_PendingAsyncRead* request = DSB_PAR_FreeList;

    if (request != 0) {
        mslDSB_RequestHandleValue handle;
        DSB_PAR_FreeList = request->next;
        handle = request->handle.value;
        memset(request, 0, sizeof(*request));
        request->handle.value = handle;
        request->handle.parts.generation++;
        request->in_use = 1;
    }
    OSRestoreInterrupts(enabled);
    if (request == 0) {
        mslDebugPrintf(STREAM_STRING(0x31));
    }
    return request;
}

extern "C" mslStreamFileRequest* mslStreamFile_QueueRequest(
    _mwFile* file, unsigned long offset, unsigned long size, int priority,
    mslStreamFileCallback callback, void* callback_data) {
    mslDSB_PendingAsyncRead* request = 0;

    if (size != 0) {
        const char* retry_strings;

        request = mslDSB_AllocPending();
        retry_strings = stringBase0;
        while (request == 0) {
            mslDebugPrintf(&retry_strings[0x85]);
            request = mslDSB_AllocPending();
        }
        {
            unsigned long enabled = OSDisableInterrupts();

            request->file = file;
            request->priority = priority;
            request->callback = callback;
            request->callback_data = callback_data;
            request->request_offset = offset;
            request->request_size = size;
            request->next_offset = offset;
            request->remaining = size;
            *DSB_PAR_Queue.last_link = request;
            DSB_PAR_Queue.last_link = &request->next;
            request->next = 0;
            request->queued = 1;
            OSRestoreInterrupts(enabled);
        }
        mslDSB_ServiceNextRead();
    }
    if (request != 0) {
        return (mslStreamFileRequest*)mslDSB_HandleToOpaque(request->handle.value);
    }
    return 0;
}

static inline void mslDSB_FreeFileRead(mslDSB_FileRead* read) {
    if (read != 0) {
        memset(read, 0, sizeof(*read));
        read->next = DSB_FILEREAD_FreeList;
        DSB_FILEREAD_FreeList = read;
    }
}

static inline void mslDSB_UnlinkFileRead(
    mslDSB_PendingAsyncRead* request, mslDSB_FileRead* read) {
    if (read->previous != 0) {
        read->previous->next = read->next;
        if (read->next != 0) {
            read->next->previous = read->previous;
        } else {
            request->last_read = read->previous;
        }
    } else {
        request->first_read = read->next;
        if (request->first_read != 0) {
            request->first_read->previous = 0;
        } else {
            request->last_read = 0;
        }
    }
    read->owner = 0;
}

static inline void mslDSB_FreePending(mslDSB_PendingAsyncRead* request) {
    unsigned long enabled = OSDisableInterrupts();
    mslDSB_RequestHandleValue handle;

    request->in_use = 0;
    handle = request->handle.value;
    memset(request, 0, sizeof(*request));
    request->handle.value = handle;
    request->next = DSB_PAR_FreeList;
    DSB_PAR_FreeList = request;
    OSRestoreInterrupts(enabled);
}

/* TODO: [near miss] 89.76%; list reloads and address lifetimes remain
 * (retail 0x318, current 0x320); portable address-delta trial scored 88.28%. */
static void mslDSB_CancelRead(
    mslDSB_PendingAsyncRead* request, int invoke_callback) {
    if (request != 0) {
        unsigned long enabled = OSDisableInterrupts();

        if (request->in_use) {
            mslDSB_FileRead* read;
            mslStreamFileCallback callback;
            void* callback_data;

            mslDebugPrintf(
                STREAM_STRING(0xD0), request->active_reads);
            if (request->active_reads != 0) {
                read = request->last_read;
                while (read != 0) {
                    mslDSB_FileRead* next;

                    mslDSB_UnlinkFileRead(request, read);
                    if (read->command != 0) {
                        mslDebugPrintf(
                            STREAM_STRING(0xE8), read->command);
                        mwFileAbortCommand(read->command);
                        mwFileFreeCommand(read->command);
                        read->command = 0;
                    }
                    if (read->buffer != 0) {
                        mslDSB_ReturnBuffer(read->buffer);
                        read->buffer = 0;
                    }
                    next = request->last_read;
                    mslDSB_FreeFileRead(read);
                    read = next;
                }
                request->active_reads = 0;
            }

            if (request->queued) {
                unsigned long queue_enabled;
                mslDSB_PendingAsyncRead** link;
                mslDebugPrintf(STREAM_STRING(0x103));
                queue_enabled = OSDisableInterrupts();
                link = &DSB_PAR_Queue.first;
                while (*link != 0) {
                    if (*link == request) {
                        *link = request->next;
                        if (*link == 0) {
                            DSB_PAR_Queue.last_link = link;
                        }
                        request->next = 0;
                        request->queued = 0;
                        break;
                    }
                    link = &(*link)->next;
                }
                OSRestoreInterrupts(queue_enabled);
            }

            callback = request->callback;
            request->callback = 0;
            callback_data = request->callback_data;
            request->callback_data = 0;
            mslDebugPrintf(STREAM_STRING(0x115), callback);
            if (invoke_callback && callback != 0) {
                callback(0, -1, 0, 0x44DEAD0F, 1, callback_data);
            }
            if (request->active_reads != 0) {
                mslDebugPrintf(STREAM_STRING(0x133));
            }
            if (request->queued) {
                mslDebugPrintf(STREAM_STRING(0x15C));
            }
            if (!request->in_use) {
                mslDebugPrintf(STREAM_STRING(0x184));
            }
            mslDSB_FreePending(request);
        }
        OSRestoreInterrupts(enabled);
    }
}

static inline void* mslDSB_AllocBuffer(void) {
    int i;
    for (i = 0; i < 5; i++) {
        if (g_DSB_BufferFree[i]) {
            g_DSB_BufferFree[i] = 0;
            return g_DSB_Buffers[i];
        }
    }
    return 0;
}

static inline mslDSB_FileRead* mslDSB_AllocFileRead(void) {
    mslDSB_FileRead* read = DSB_FILEREAD_FreeList;
    if (read != 0) {
        DSB_FILEREAD_FreeList = read->next;
        memset(read, 0, sizeof(*read));
    }
    return read;
}

static inline mslDSB_PendingAsyncRead* mslDSB_DequeuePending(void) {
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

void mslDSB_ServiceNextRead(void) {
    mslDSB_PendingAsyncRead* request = 0;
    mslDSB_FileRead* read = 0;
    void* buffer = 0;
    int started = 0;
    unsigned long enabled = OSDisableInterrupts();

    {
        unsigned long inner = OSDisableInterrupts();
        request = DSB_PAR_Queue.first;
        OSRestoreInterrupts(inner);
    }
    if (request != 0) {
        buffer = mslDSB_AllocBuffer();
        if (buffer != 0) {
            read = mslDSB_AllocFileRead();
            request = mslDSB_DequeuePending();
            request->active_reads++;
            started = 1;
        } else {
            request = 0;
        }
    }
    OSRestoreInterrupts(enabled);

    if (started) {
        while (read != 0) {
            u32 offset;
            u32 size;
            unsigned long inner = OSDisableInterrupts();

            size = request->remaining;
            if (size > 0x4000) {
                size = 0x4000;
            }
            offset = request->next_offset;
            read->owner = request;
            read->previous = request->last_read;
            if (request->last_read != 0) {
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

            read->command = (mwFileCommand*)mwFileReadAsync(
                request->file, read->offset, buffer, read->size,
                request->priority, mslDSB_FileReadCompletionCallback, read);
            read = 0;
            if (request->remaining != 0 && request->active_reads < 3) {
                unsigned long more_enabled = OSDisableInterrupts();
                buffer = mslDSB_AllocBuffer();
                if (buffer != 0) {
                    read = mslDSB_AllocFileRead();
                    request->active_reads++;
                }
                OSRestoreInterrupts(more_enabled);
            }
        }
    }
}

/* TODO: [near miss] 90.00%; error callbacks always receive null as in retail;
 * size is exact, with list-reload scheduling and GPR coloring remaining. */
static void mslDSB_FileReadCompletionCallback(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    mslDSB_FileRead* read = (mslDSB_FileRead*)callback_data;

    if (read->command != command) {
        mwFileFreeCommand(command);
        return;
    }
    {
        mslDSB_PendingAsyncRead* request = read->owner;
        unsigned long enabled = OSDisableInterrupts();
        int final = 0;
        int requeue = 0;
        int error = 0;
        mslStreamFileCallback callback;
        void* data;
        void* buffer;

        mslDSB_UnlinkFileRead(request, read);
        request->active_reads--;
        data = request->callback_data;
        callback = request->callback;
        buffer = read->buffer;
        read->buffer = 0;
        if (request->error) {
            error = 0x44DEAD01;
        }

        if (callback != 0) {
            u32 offset = read->callback_offset - request->request_offset;
            u32 size = read->callback_size;

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
                mslDSB_ReturnBuffer(buffer);
                buffer = 0;
            }
            callback(buffer, offset, size, error, final, data);
            if (error != 0) {
                request->callback = 0;
                callback = 0;
            }
        } else {
            mslDSB_ReturnBuffer(buffer);
        }

        if (requeue) {
            if (!request->queued) {
                *DSB_PAR_Queue.last_link = request;
                DSB_PAR_Queue.last_link = &request->next;
                request->next = 0;
                request->queued = 1;
                callback = 0;
            }
        } else if (request->active_reads == 0) {
            mslDSB_RequestHandleValue handle;
            request->in_use = 0;
            handle = request->handle.value;
            memset(request, 0, sizeof(*request));
            request->handle.value = handle;
            request->next = DSB_PAR_FreeList;
            DSB_PAR_FreeList = request;
        }
        mslDSB_FreeFileRead(read);
        OSRestoreInterrupts(enabled);
        mwFileFreeCommand(command);
        if (callback == 0) {
            mslDSB_ServiceNextRead();
        }
    }
}
