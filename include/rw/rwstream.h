#ifndef RW_RWSTREAM_H
#define RW_RWSTREAM_H

typedef struct RwStream RwStream;

typedef struct RwMemory {
    void* start;
    unsigned int length;
} RwMemory;

RwStream* RwStreamOpen(int type, int access_type, void* data);
int RwStreamClose(RwStream* stream, void* data);
RwStream* RwStreamWriteInt32(RwStream* stream, const int* values,
                             unsigned int numBytes);
RwStream* RwStreamReadInt32(RwStream* stream, int* values,
                            unsigned int numBytes);

#endif
