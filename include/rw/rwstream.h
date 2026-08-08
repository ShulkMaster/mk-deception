#ifndef RW_RWSTREAM_H
#define RW_RWSTREAM_H

#include "rw/rwobject.h"

typedef struct RwMemory {
    void* start;
    unsigned int length;
} RwMemory;

RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType,
                       void* data);
int RwStreamClose(RwStream* stream, void* data);
unsigned int RwStreamRead(RwStream* stream, void* buffer,
                          unsigned int length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer,
                        unsigned int length);
RwStream* RwStreamSkip(RwStream* stream, unsigned int offset);
RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type,
                         RwUInt32* length, RwUInt32* version);
void* RwMemNative32(void* memory, RwUInt32 size);
void* RwMemLittleEndian32(void* memory, RwUInt32 size);
RwStream* RwStreamWriteInt32(RwStream* stream, const int* values,
                             unsigned int numBytes);
RwStream* RwStreamReadInt32(RwStream* stream, int* values,
                            unsigned int numBytes);

#endif
