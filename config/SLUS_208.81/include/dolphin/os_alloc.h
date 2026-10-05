#ifndef DOLPHIN_OS_ALLOC_H
#define DOLPHIN_OS_ALLOC_H

#include "platform/os_types.h"

void* OSAllocFromHeap(OSHeapHandle heap, u32 size);
void OSFreeToHeap(OSHeapHandle heap, void* pointer);

#endif
