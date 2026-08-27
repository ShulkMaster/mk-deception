#include "dolphin/os.h"
#include "dolphin/os_alloc.h"

extern volatile OSHeapHandle __OSCurrHeap;

static inline void InitDefaultHeap(void)
{
    void* arena_lo;
    void* arena_hi;

    OSReport("GCN_Mem_Alloc.c : InitDefaultHeap. No Heap Available\n");
    OSReport("Metrowerks CW runtime library initializing default heap\n");

    arena_lo = OSGetArenaLo();
    arena_hi = OSGetArenaHi();

    arena_lo = OSInitAlloc(arena_lo, arena_hi, 1);
    OSSetArenaLo(arena_lo);

    arena_lo = (void*)OSRoundUp32B(arena_lo);
    arena_hi = (void*)OSRoundDown32B(arena_hi);

    OSSetCurrentHeap(OSCreateHeap(arena_lo, arena_hi));
    OSSetArenaLo(arena_lo = arena_hi);
}

__declspec(weak) void __sys_free(void* pointer)
{
    if (__OSCurrHeap == -1) {
        InitDefaultHeap();
    }
    OSFreeToHeap(__OSCurrHeap, pointer);
}

__declspec(weak) void* __sys_alloc(unsigned long size)
{
    if (__OSCurrHeap == -1) {
        InitDefaultHeap();
    }
    return OSAllocFromHeap(__OSCurrHeap, size);
}
