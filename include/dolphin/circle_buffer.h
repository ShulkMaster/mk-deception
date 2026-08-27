#ifndef DOLPHIN_CIRCLE_BUFFER_H
#define DOLPHIN_CIRCLE_BUFFER_H

#include "dolphin/types.h"

typedef struct CircleBuffer {
    u8* read;
    u8* write;
    u8* start;
    u32 size;
    u32 bytes_to_read;
    u32 bytes_to_write;
    u32 critical_section;
} CircleBuffer;

u32 CBGetBytesAvailableForRead(CircleBuffer* buffer);
void CircleBufferInitialize(CircleBuffer* buffer, u8* storage, u32 size);
int CircleBufferWriteBytes(CircleBuffer* buffer, const u8* source, u32 size);
int CircleBufferReadBytes(CircleBuffer* buffer, u8* destination, u32 size);

#ifdef __cplusplus
extern "C" {
#endif

void MWInitializeCriticalSection(u32* section);
void MWEnterCriticalSection(u32* section);
void MWExitCriticalSection(u32* section);

#ifdef __cplusplus
}
#endif

#endif
