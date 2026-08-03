#ifndef DOLPHIN_OS_ALLOC_H
#define DOLPHIN_OS_ALLOC_H

#include "platform/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

char* OSGetArenaHi(void);
char* OSGetArenaLo(void);
void* OSInitAlloc(void* arena_start, void* arena_end, int max_heaps);
void OSSetArenaLo(void* arena_low);
OSHeapHandle OSCreateHeap(void* heap_start, void* heap_end);
OSHeapHandle OSSetCurrentHeap(OSHeapHandle heap);
int OSCheckHeap(OSHeapHandle heap);
void* OSAllocFromHeap(OSHeapHandle heap, unsigned long size);
void OSFreeToHeap(OSHeapHandle heap, void* pointer);

#ifdef __cplusplus
}
#endif

#endif
