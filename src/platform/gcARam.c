#include "platform/gcARam.h"

#include "dolphin/ar.h"
#include "dolphin/vm.h"
#include "mw/mwMem.h"

extern _mwMemHeap* SystemSwappableHeap;
extern u32 g_ARAM_VM_Start;
extern u32 g_ARAM_VM_Size;
extern u32 g_ARAM_MSL_Start;
extern u32 g_ARAM_MSL_Size;

/* MWCC emits tentative definitions in reverse order within these sections. */
int gap_08_80510E9C_sbss;
/* This linker-visible four-byte gap is the sole small object retail keeps in .bss. */
#pragma section data_type ".data" ".bss"
__declspec(section ".data") u32 gap_06_803DEABC_bss;
#pragma section data_type
u32 g_GC_ARAM_MemBlocks[5];

/*
 * Soft ceiling: exact retail size and opcode stream; only nonvolatile-register
 * coloring and the compiler's local string relocation label remain.
 */
void gc_aram_mwmem_heap_setup(void) {
    u32 virtual_size;
    void* virtual_base;
    u32 vm_start = g_ARAM_VM_Start;

    if ((vm_start == 0 ? 0 : g_ARAM_VM_Size) != 0 &&
        SystemSwappableHeap == 0) {
        virtual_base = (void*)0x7e000000;
        if (vm_start == 0) {
            virtual_base = 0;
        }
        virtual_size = vm_start == 0 ? 0 : g_ARAM_VM_Size;
        /* The retail pooled name owns two trailing alignment zeroes. */
        SystemSwappableHeap = mwMemExtSystemHeapCreate(
            mwMemSystemGetHeap(0), virtual_base, virtual_size,
            "SwappableHeap\0\0");
    }
}

/*
 * Soft ceiling: exact retail size, control flow, calls, and accesses; the
 * remaining objdiff records are register allocation only.
 */
void gc_aram_init(void) {
    u32 available_size;
    u32 vm_start;
    u32 vm_size;
    void* vm_base;
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
        vm_start = ARAlloc(vm_size);
        g_ARAM_VM_Size = vm_size;
        g_ARAM_VM_Start = vm_start;
        VMInit(0x100000, vm_start, vm_size);
        VMAlloc((void*)0x7e000000, vm_size);
        aramSize = g_ARAM_VM_Start == 0 ? 0 : g_ARAM_VM_Size;
        vm_base = (void*)0x7e000000;
        if (g_ARAM_VM_Start == 0) {
            vm_base = 0;
        }
        aramBase = vm_base;
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
