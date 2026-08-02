#ifndef LIBMKPARTICLE_STREAMS_H
#define LIBMKPARTICLE_STREAMS_H

typedef struct PfxStreamBufferInfo {
    int write_offset;
    int lock_size;
    unsigned char locked;
    unsigned char reverse;
    unsigned char frame;
    unsigned char frame_count;
    unsigned char* current_buffer;
    unsigned char* buffer;
    int frame_size;
    int alloc_size;
} PfxStreamBufferInfo;

void* streampool_lock(int stream, int size);
void streampool_unlock(int stream, int size);
void streampool_nextframe(void);
void streampool_init(void);
int streampool_size(int stream);
void* streampool_alloc(int stream, int size);
void streampool_skiprenderstream(void);

#endif
