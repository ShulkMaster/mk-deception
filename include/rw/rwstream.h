#ifndef RW_RWSTREAM_H
#define RW_RWSTREAM_H

#include "rw/rwobject.h"
#include "rw/rwplcore.h"

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
int RwStreamFindChunk(RwStream* stream, unsigned int type,
                         unsigned int* length, unsigned int* version);
void* RwMemNative32(void* memory, unsigned int size);
void* RwMemLittleEndian32(void* memory, unsigned int size);
RwStream* RwStreamWriteInt32(RwStream* stream, const int* values,
                             unsigned int numBytes);
RwStream* RwStreamReadInt32(RwStream* stream, int* values,
                            unsigned int numBytes);
void* _rwStreamModuleOpen(void* instance, int offset, int size);
void* _rwStreamModuleClose(void* instance, int offset, int size);

#endif
