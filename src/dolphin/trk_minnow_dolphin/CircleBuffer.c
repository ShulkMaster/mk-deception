#include "dolphin/trk.h"

typedef struct CircleBuffer {
    u8* read;
    u8* write;
    u8* start;
    u32 size;
    u32 bytes_to_read;
    u32 bytes_to_write;
    u32 critical_section;
} CircleBuffer;

extern void* memcpy(void* destination, const void* source, u32 size);
extern void MWInitializeCriticalSection(void* section);
extern void MWEnterCriticalSection(void* section);
extern void MWExitCriticalSection(void* section);

u32 CBGetBytesAvailableForRead(CircleBuffer* buffer)
{
    return buffer->bytes_to_read;
}

void CircleBufferInitialize(CircleBuffer* buffer, u8* storage, u32 size)
{
    buffer->start = storage;
    buffer->size = size;
    buffer->read = buffer->start;
    buffer->write = buffer->start;
    buffer->bytes_to_read = 0;
    buffer->bytes_to_write = buffer->size;
    MWInitializeCriticalSection(&buffer->critical_section);
}

int CircleBufferWriteBytes(CircleBuffer* buffer, const u8* source, u32 size)
{
    u32 available;

    if (size > buffer->bytes_to_write)
        return -1;
    MWEnterCriticalSection(&buffer->critical_section);
    available = buffer->size - (buffer->write - buffer->start);
    if (available >= size) {
        memcpy(buffer->write, source, size);
        buffer->write += size;
    } else {
        memcpy(buffer->write, source, available);
        memcpy(buffer->start, source + available, size - available);
        buffer->write = buffer->start + size - available;
    }
    if (buffer->size == buffer->write - buffer->start)
        buffer->write = buffer->start;
    buffer->bytes_to_write -= size;
    buffer->bytes_to_read += size;
    MWExitCriticalSection(&buffer->critical_section);
    return 0;
}

int CircleBufferReadBytes(CircleBuffer* buffer, u8* destination, u32 size)
{
    u32 available;

    if (size > buffer->bytes_to_read)
        return -1;
    MWEnterCriticalSection(&buffer->critical_section);
    available = buffer->size - (buffer->read - buffer->start);
    if (size < available) {
        memcpy(destination, buffer->read, size);
        buffer->read += size;
    } else {
        memcpy(destination, buffer->read, available);
        memcpy(destination + available, buffer->start, size - available);
        buffer->read = buffer->start + size - available;
    }
    if (buffer->size == buffer->read - buffer->start)
        buffer->read = buffer->start;
    buffer->bytes_to_write += size;
    buffer->bytes_to_read -= size;
    MWExitCriticalSection(&buffer->critical_section);
    return 0;
}
