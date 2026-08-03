#ifndef RW_RWSTREAM_H
#define RW_RWSTREAM_H

typedef struct RwStream RwStream;

typedef struct RwMemory {
    void* start;
    unsigned int length;
} RwMemory;

RwStream* RwStreamOpen(int type, int access_type, void* data);
int RwStreamClose(RwStream* stream, void* data);

#endif
