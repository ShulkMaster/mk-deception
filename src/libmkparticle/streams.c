#include "libmkparticle/streams.h"

void* memset(void* dst, int value, unsigned long size);

#define STREAM_COUNT 2
#define STREAM_FRAME_SIZE 150000
#define STREAM_MEMORY_SIZE 300016

static PfxStreamBufferInfo streambuffer_info[STREAM_COUNT];
static unsigned char render_memory[STREAM_MEMORY_SIZE];
static unsigned char property_memory[STREAM_MEMORY_SIZE];

/* Soft ceiling: streams.o ~91.93% -- equivalent register allocation and
 * instruction scheduling differences. */
void* streampool_lock(int stream, int size) {
    PfxStreamBufferInfo* info;

    info = &streambuffer_info[stream];
    if (info->write_offset + ((size + 15) & ~15) > streampool_size(stream)) {
        return 0;
    }
    if (info->locked != 0) {
        return 0;
    }

    info->locked = 1;
    info->lock_size = size;
    return info->current_buffer + info->write_offset;
}

void streampool_unlock(int stream, int size) {
    PfxStreamBufferInfo* info;

    info = &streambuffer_info[stream];
    if (info->locked != 0) {
        info->locked = 0;
        if (size <= info->lock_size) {
            info->write_offset += (size + 15) & ~15;
        }
    }
}

void streampool_nextframe(void) {
    int i;

    for (i = 0; i < STREAM_COUNT; i++) {
        PfxStreamBufferInfo* info;
        int frame;

        info = &streambuffer_info[i];
        if (info->write_offset > 0) {
            info->write_offset = 0;
            info->locked = 0;
            info->alloc_size = 0;

            frame = info->frame;
            if (info->reverse != 0) {
                if (frame == 0) {
                    frame = info->frame_count - 1;
                } else {
                    frame--;
                }
            } else {
                frame++;
                if (frame >= info->frame_count) {
                    frame = 0;
                }
            }
            info->frame = frame;
            info->current_buffer = info->buffer + frame * info->frame_size;
        }
    }
}

void streampool_init(void) {
    int i;

    for (i = 0; i < STREAM_COUNT; i++) {
        memset(&streambuffer_info[i], 0, sizeof(PfxStreamBufferInfo));
    }

    streambuffer_info[0].frame_count = 2;
    streambuffer_info[0].frame_size = STREAM_FRAME_SIZE;
    streambuffer_info[0].buffer =
        (unsigned char*)(((unsigned long)render_memory + 15) & ~15UL);
    streambuffer_info[0].current_buffer = streambuffer_info[0].buffer;

    streambuffer_info[1].frame_count = 2;
    streambuffer_info[1].frame_size = STREAM_FRAME_SIZE;
    streambuffer_info[1].buffer =
        (unsigned char*)(((unsigned long)property_memory + 15) & ~15UL);
    streambuffer_info[1].current_buffer = streambuffer_info[1].buffer;
}

int streampool_size(int stream) {
    PfxStreamBufferInfo* info;
    int lock_size;

    info = &streambuffer_info[stream];
    lock_size = info->locked != 0 ? info->lock_size : 0;
    return info->frame_size - info->alloc_size - lock_size;
}

void* streampool_alloc(int stream, int size) {
    PfxStreamBufferInfo* info;
    unsigned char* result;
    int* alloc_size;

    if (streampool_size(stream) < size) {
        return 0;
    }

    info = &streambuffer_info[stream];
    result = info->current_buffer + info->frame_size;
    alloc_size = &info->alloc_size;
    result -= *alloc_size;
    *alloc_size += size;
    return result - size;
}

void streampool_skiprenderstream(void) {
    streambuffer_info[0].reverse ^= 1;
}
