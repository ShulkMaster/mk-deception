#include "platform/gcARam.h"

#include "dolphin/ar.h"
#include "dolphin/vm.h"
#include "mw/mwMem.h"

extern _mwMemHeap* SystemSwappableHeap;
extern u32 g_ARAM_VM_Start;
extern u32 g_ARAM_VM_Size;
extern u32 g_ARAM_MSL_Start;
extern u32 g_ARAM_MSL_Size;

u32 g_GC_ARAM_MemBlocks[5];

void gc_aram_mwmem_heap_setup(void) {
    u32 virtual_size;
    void* virtual_base;
    u32 vm_start = g_ARAM_VM_Start;

    if ((vm_start == 0 ? 0 : g_ARAM_VM_Size) != 0 &&
        SystemSwappableHeap == 0) {
        virtual_base = vm_start == 0 ? 0 : (void*)0x7e000000;
        virtual_size = vm_start == 0 ? 0 : g_ARAM_VM_Size;
        SystemSwappableHeap = mwMemExtSystemHeapCreate(
            mwMemSystemGetHeap(0), virtual_base, virtual_size,
            "SwappableHeap");
    }
}

void gc_aram_init(void) {
    u32 available_size;
    u32 vm_size;
    static void* aramBase;
    static u32 aramSize;

    ARInit(g_GC_ARAM_MemBlocks, 5);
    available_size = ARGetSize() - ARGetBaseAddress();

    if (g_ARAM_MSL_Start == 0) {
        g_ARAM_MSL_Start = ARAlloc(0x800000);
        g_ARAM_MSL_Size = 0x800000;
    }

    if (g_ARAM_VM_Start == 0) {
        vm_size = available_size - 0x840000;
        g_ARAM_VM_Start = ARAlloc(vm_size);
        g_ARAM_VM_Size = vm_size;
        VMInit(0x100000, g_ARAM_VM_Start, vm_size);
        VMAlloc((void*)0x7e000000, vm_size);
        aramSize = g_ARAM_VM_Start == 0 ? 0 : g_ARAM_VM_Size;
        aramBase = g_ARAM_VM_Start == 0 ? 0 : (void*)0x7e000000;
    }
}

u32 ARAM_MSL_GetSize(void) {
    return g_ARAM_MSL_Start == 0 ? 0 : g_ARAM_MSL_Size;
}

u32 ARAM_MSL_GetBase(void) {
    void* base = (void*)g_ARAM_MSL_Start;

    if (base == 0) {
        return 0;
    }
    return (u32)base;
}

/* Reverse source order reproduces the retail .sbss symbol order. */
u32 g_ARAM_MSL_Size;
u32 g_ARAM_MSL_Start;
u32 g_ARAM_VM_Size;
u32 g_ARAM_VM_Start;
_mwMemHeap* SystemSwappableHeap;
