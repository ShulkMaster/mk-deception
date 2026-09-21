#include "runtime/cstring.h"
#include "sofdec/sfd_error.h"
#include "sofdec/sfd_transport.h"

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

/* TODO: [breakthrough needed] 48.060240%; unsigned position/buffer arithmetic now matches donor types; queue-search CFG and register shape remain divergent. */
int SFPTS_ReadPtsQue(SfdHandle* handle, int buffer_index,
                     unsigned int position, SfdPtsEntry* output)
{
    SfdBufferRingWork* ring = &handle->buffers[buffer_index].work.ring;
    SfdPtsQueue* queue = &ring->pts_queue;
    unsigned int buffer_start = (unsigned int)ring->buffer;
    unsigned int buffer_end = buffer_start + ring->buffer_size;
    int index;
    int offset;

    output->pts = -1;
    if (queue->entries == 0) {
        return 0;
    }
    if (position >= buffer_end) {
        position -= ring->buffer_size;
    }
    index = queue->read_index;
    for (offset = 0; offset < queue->count; offset++) {
        SfdPtsEntry* entry = &queue->entries[index];
        unsigned int entry_start = (unsigned int)entry->data;
        unsigned int entry_end = entry_start + entry->size;
        int contains;

        if (entry_end <= buffer_end) {
            contains = entry_start <= position && position < entry_end;
        } else {
            contains = (entry_start <= position && position < buffer_end) ||
                       (buffer_start <= position &&
                        position < entry_end - ring->buffer_size);
        }
        if (contains) {
            break;
        }
        index++;
        if (index >= queue->capacity) {
            index = 0;
        }
    }
    if (offset < queue->count) {
        queue->count -= offset;
        queue->read_index += offset;
        if (queue->read_index >= queue->capacity) {
            queue->read_index -= queue->capacity;
        }
        *output = queue->entries[queue->read_index];
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
    handle->buffers[1].work.ring.pts_queue.capacity = size / 16;
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
