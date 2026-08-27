/*
 * Retail stream-cache ownership: 32 fixed 0x20000-byte ARAM buffers move
 * between free, LOD-cache, and active-stream intrusive lists.
 *
 * Current reconstruction: 97.99% .text. GetStreamBuffer and ReleaseBuffer
 * are exact, Initialize_A is 91.57%, and both getters plus all data are exact.
 * The remaining differences are initialization register allocation only.
 */

#include "dolphin/os.h"
#include "msl/mslgcn.h"
#include "msl/mslStreamCache.h"
#include "runtime/cstring.h"

typedef unsigned char u8;
typedef unsigned long u32;

struct StreamCacheBuffer {
    StreamCacheBuffer* next;
    StreamCacheBuffer* previous;
    int address;
    u8 state;
    u8 lod_field_0d;
    u8 lod_field_0e;
    u8 pad0f;
    u32 lod_field_10;
    u32 lod_field_14;
    u32 lod_field_18;
    u32 lod_field_1c;
};

struct StreamCacheList {
    StreamCacheBuffer* first;
    StreamCacheBuffer* last;
};

static inline void StreamCacheList_LinkTail(
    StreamCacheList* list, StreamCacheBuffer* buffer) {
    StreamCacheBuffer* previous = list->last;

    buffer->previous = previous;
    if (previous != 0) {
        list->last->next = buffer;
    } else {
        list->first = buffer;
    }
    list->last = buffer;
    buffer->next = 0;
}

StreamCacheBuffer s_StreamCache_ArrayBuffers[32];
StreamCacheList SCB_List_Free = {0, 0};
StreamCacheList SCB_List_LodCache = {0, 0};
StreamCacheList SCB_List_Stream = {0, 0};
int s_StreamCache_NumBuffers;
int s_StreamCache_SizeBuffers;
int s_StreamCache_BaseAddress;
StreamCacheBuffer* s_StreamCache_pBuffers;

extern "C" int mslStreamCache_GetSizeBuffer(void) {
    return s_StreamCache_SizeBuffers;
}

extern "C" int mslStreamCache_GetNumBuffers(void) {
    return s_StreamCache_NumBuffers;
}

extern "C" u32 mslStreamCache_GetStreamBuffer(void) {
    u32 result = 0;
    unsigned long enabled = OSDisableInterrupts();
    StreamCacheBuffer* first = SCB_List_Free.first;
    StreamCacheBuffer* buffer = first;
    if (buffer != 0) {
        SCB_List_Free.first = buffer->next;
        if (SCB_List_Free.first != 0) {
            SCB_List_Free.first->previous = 0;
        } else {
            SCB_List_Free.last = 0;
        }
    }

    if (buffer == 0) {
        first = SCB_List_LodCache.first;
        buffer = first;
        if (buffer != 0) {
            SCB_List_LodCache.first = buffer->next;
            if (SCB_List_LodCache.first != 0) {
                SCB_List_LodCache.first->previous = 0;
            } else {
                SCB_List_LodCache.last = 0;
            }
        }
        if (buffer != 0) {
            buffer->lod_field_0d = 0;
            buffer->lod_field_0e = 0;
            buffer->lod_field_10 = 0;
            buffer->lod_field_14 = 0;
            buffer->lod_field_18 = 0;
            buffer->lod_field_1c = 0;
        }
    }

    if (buffer != 0) {
        result = buffer->address;
        buffer->state = 3;
        first = SCB_List_Stream.first;
        buffer->next = first;
        if (first != 0) {
            SCB_List_Stream.first->previous = buffer;
        } else {
            SCB_List_Stream.last = buffer;
        }
        SCB_List_Stream.first = buffer;
        buffer->previous = 0;
    }

    OSRestoreInterrupts(enabled);
    return result;
}

extern "C" void mslStreamCache_ReleaseBuffer(int address) {
    StreamCacheBuffer* buffer =
        s_StreamCache_pBuffers +
        (address - s_StreamCache_BaseAddress) /
            s_StreamCache_SizeBuffers;
    unsigned long enabled = OSDisableInterrupts();

    switch (buffer->state) {
    case 1:
        _MSL_GCN_BREAK();
        break;
    case 3:
        if (buffer->previous != 0) {
            StreamCacheBuffer* next = buffer->next;

            buffer->previous->next = next;
            if (next != 0) {
                buffer->next->previous = buffer->previous;
            } else {
                SCB_List_Stream.last = buffer->previous;
            }
        } else {
            SCB_List_Stream.first = buffer->next;
            if (SCB_List_Stream.first != 0) {
                SCB_List_Stream.first->previous = 0;
            } else {
                SCB_List_Stream.last = 0;
            }
        }

        buffer->state = 0;
        {
            StreamCacheBuffer* first = SCB_List_Free.first;

            buffer->next = first;
            if (first != 0) {
                SCB_List_Free.first->previous = buffer;
            } else {
                SCB_List_Free.last = buffer;
            }
        }
        SCB_List_Free.first = buffer;
        buffer->previous = 0;
        break;
    case 0:
    default:
        _MSL_GCN_BREAK();
        break;
    }

    OSRestoreInterrupts(enabled);
}

/*
 * Soft ceiling: mslStreamCache_Initialize_A ~91.57% -- retail and this build
 * have identical calls, control flow, memory operations, and size; only the
 * six nonvolatile register homes and zero materialization differ.
 */
extern "C" void mslStreamCache_Initialize_A(int base_address) {
    if (s_StreamCache_pBuffers == 0) {
        int i = 0;
        StreamCacheBuffer* buffer;

        s_StreamCache_pBuffers = s_StreamCache_ArrayBuffers;
        buffer = s_StreamCache_pBuffers;
        s_StreamCache_BaseAddress = base_address;
        s_StreamCache_NumBuffers = 32;
        s_StreamCache_SizeBuffers = 0x20000;

        for (; i < 32; i++, buffer++) {
            memset(buffer, 0, sizeof(StreamCacheBuffer));
            buffer->state = 0;
            buffer->address = base_address;
            StreamCacheList_LinkTail(&SCB_List_Free, buffer);
            base_address += 0x20000;
        }
    }
}
