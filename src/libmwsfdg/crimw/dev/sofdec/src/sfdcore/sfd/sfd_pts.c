#include "runtime/cstring.h"
#include "sofdec/sfd_error.h"
#include "sofdec/sfd_transport.h"

int SFPTS_IsPtsQueFull(SfdHandle* handle, int buffer_index)
{
    SfdPtsQueue* queue =
        &handle->buffers[buffer_index].work.ring.pts_queue;

    if (queue->entries == 0) {
        return 0;
    }
    return queue->count >= queue->capacity;
}

int SFPTS_ReadPtsQue(SfdHandle* handle, int buffer_index,
                     unsigned char* position, SfdPtsEntry* output)
{
    SfdBufferRingWork* ring = &handle->buffers[buffer_index].work.ring;
    SfdPtsQueue* queue = &ring->pts_queue;
    unsigned char* buffer_end = ring->buffer + ring->buffer_size;
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
        unsigned char* entry_end = entry->data + entry->size;
        int contains;

        if (entry_end <= buffer_end) {
            contains = entry->data <= position && position < entry_end;
        } else {
            contains =
                (entry->data <= position && position < buffer_end) ||
                (ring->buffer <= position &&
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
                      const SfdPtsEntry* entry, int* full)
{
    SfdBufferState* buffer;
    SfdPtsQueue* queue;
    int status;
    int next_index;

    *full = 0;
    if (entry->pts < 0) {
        return 0;
    }
    buffer = &handle->buffers[buffer_index];
    queue = &buffer->work.ring.pts_queue;
    if (queue->entries == 0) {
        return 0;
    }
    if (queue->count == queue->capacity) {
        *full = 1;
        status = -1;
    } else {
        queue->entries[queue->write_index] = *entry;
        next_index = queue->write_index + 1;
        queue->write_index = next_index - queue->capacity;
        if (next_index < queue->capacity) {
            queue->write_index = next_index;
        }
        queue->count++;
        if (queue->count >= queue->capacity) {
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

int SFD_SetVideoPts(SfdHandle* handle, void* memory, int size)
{
    SfdPtsQueue* queue;
    unsigned char* aligned;
    int aligned_size;

    if (memory == 0 || size <= 0) {
        return 0;
    }
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000165);
    }
    aligned = (unsigned char*)(((unsigned long)memory + 7) & ~7UL);
    aligned_size = size - (aligned - (unsigned char*)memory);
    memset(aligned, 0, aligned_size);
    queue = &handle->buffers[1].work.ring.pts_queue;
    queue->entries = (SfdPtsEntry*)aligned;
    queue->capacity = aligned_size / sizeof(SfdPtsEntry);
    queue->count = 0;
    queue->write_index = 0;
    queue->read_index = 0;
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
