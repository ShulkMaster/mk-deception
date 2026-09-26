#include "runtime/cstring.h"
#include "sofdec/sfd_error.h"
#include "sofdec/sfd_transport.h"

/* Buffer n is addressed from the player base with its 0x74-byte stride. */
typedef struct SfdPtsBufferHn {
    unsigned char player_prefix[0x1308];
    SfdBufferState buffer;
} SfdPtsBufferHn;

typedef char SfdPtsBufferHnSizeCheck[
    sizeof(SfdPtsBufferHn) == 0x137C ? 1 : -1];

static int sfpts_Wrap(int index, int capacity)
{
    int wrapped = index - capacity;

    if (index < capacity) {
        wrapped = index;
    }
    return wrapped;
}

int SFPTS_IsPtsQueFull(SfdHandle* handle, int buffer_index)
{
    if (handle->buffers[buffer_index].work.ring.pts_queue.entries == 0) {
        return 0;
    }
    return handle->buffers[buffer_index].work.ring.pts_queue.count >=
           handle->buffers[buffer_index].work.ring.pts_queue.capacity;
}

/* TODO: [near miss] 98.49397%; RE4-backed goto-found/-1 exit restores the CFG;
 * retail keeps each found arm as `bge next; b found` (codeless arm), ours `blt found`. */
int SFPTS_ReadPtsQue(SfdHandle* handle, int buffer_index,
                     unsigned int position, SfdPtsEntry* output)
{
    int index;
    SfdPtsBufferHn* hn;
    int offset;
    SfdPtsEntry* entry;
    unsigned int buffer_end;
    SfdPtsEntry* entries;
    int read_index;
    int capacity;
    unsigned int buffer_start;
    unsigned int buffer_size;
    int count;
    int next;

    output->pts = -1;
    hn = (SfdPtsBufferHn*)((unsigned char*)handle +
                           buffer_index * sizeof(SfdBufferState));
    entries = hn->buffer.work.ring.pts_queue.entries;
    buffer_start = (unsigned int)hn->buffer.work.ring.supply.buffer;
    buffer_size = hn->buffer.work.ring.supply.buffer_size;
    if (entries == 0) {
        return 0;
    }
    buffer_end = buffer_start + buffer_size;
    if (position >= buffer_end) {
        position -= buffer_size;
    }
    count = hn->buffer.work.ring.pts_queue.count;
    if (count != 0) {
        capacity = hn->buffer.work.ring.pts_queue.capacity;
        read_index = hn->buffer.work.ring.pts_queue.read_index;
        index = read_index;
        for (offset = 0; offset < count; offset++) {
            entry = &entries[index];
            if ((unsigned int)entry->data + entry->size <= buffer_end) {
                if ((unsigned int)entry->data <= position &&
                    position < (unsigned int)entry->data + entry->size) {
                    goto found;
                }
            } else if (((unsigned int)entry->data <= position &&
                        position < buffer_end) ||
                       (buffer_start <= position &&
                        position < (unsigned int)entry->data + entry->size -
                                       buffer_size)) {
                goto found;
            }
            next = index + 1;
            index = next - capacity;
            if (next < capacity) {
                index = next;
            }
        }
        offset = -1;
    found:
        if (offset != -1) {
            index = sfpts_Wrap(read_index + offset, capacity);
            hn->buffer.work.ring.pts_queue.count -= offset;
            hn->buffer.work.ring.pts_queue.read_index = index;
            *output = hn->buffer.work.ring.pts_queue.entries[index];
        }
    }
    return 0;
}

int SFPTS_WritePtsQue(SfdHandle* handle, int buffer_index,
                      SfdPtsEntry* entry, int* full)
{
    int write_index;
    SfdPtsEntry* entries;
    int status;

    *full = 0;
    if (entry->pts < 0) {
        return 0;
    }
    entries = handle->buffers[buffer_index].work.ring.pts_queue.entries;
    if (entries == 0) {
        return 0;
    }
    if (handle->buffers[buffer_index].work.ring.pts_queue.count ==
        handle->buffers[buffer_index].work.ring.pts_queue.capacity) {
        *full = 1;
        status = -1;
    } else {
        write_index =
            handle->buffers[buffer_index].work.ring.pts_queue.write_index;
        entries[write_index] = *entry;
        write_index = sfpts_Wrap(
            write_index + 1,
            handle->buffers[buffer_index].work.ring.pts_queue.capacity);
        handle->buffers[buffer_index].work.ring.pts_queue.count++;
        handle->buffers[buffer_index].work.ring.pts_queue.write_index =
            write_index;
        if (handle->buffers[buffer_index].work.ring.pts_queue.count >=
            handle->buffers[buffer_index].work.ring.pts_queue.capacity) {
            *full = 1;
        } else {
            *full = 0;
        }
        status = 0;
    }
    if (status == -1) {
        return SFLIB_SetErr(handle, 0xFF000421);
    }
    return 0;
}

int SFD_SetVideoPts(SfdHandle* handle, unsigned char* memory, int size)
{
    unsigned char* aligned;

    if (memory == 0 || size <= 0) {
        return 0;
    }
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000165);
    }
    aligned = (unsigned char*)(((unsigned long)memory + 7) & ~7UL);
    size -= aligned - memory;
    memset(aligned, 0, size);
    handle->buffers[1].work.ring.pts_queue.entries = (SfdPtsEntry*)aligned;
    handle->buffers[1].work.ring.pts_queue.capacity =
        size / (int)sizeof(SfdPtsEntry);
    handle->buffers[1].work.ring.pts_queue.count = 0;
    handle->buffers[1].work.ring.pts_queue.write_index = 0;
    handle->buffers[1].work.ring.pts_queue.read_index = 0;
    return 0;
}

void SFPTS_InitPtsQue(SfdPtsQueue* queue)
{
    queue->entries = 0;
    queue->capacity = 0;
    queue->count = 0;
    queue->write_index = 0;
    queue->read_index = 0;
}
