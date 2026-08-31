#include "platform/gcARam.h"

#include "dolphin/ar.h"
#include "dolphin/vm.h"
#include "mw/mwMem.h"

extern _mwMemHeap* SystemSwappableHeap;
extern unsigned long g_ARAM_VM_Start;
extern unsigned long g_ARAM_VM_Size;
extern unsigned long g_ARAM_MSL_Start;
extern unsigned long g_ARAM_MSL_Size;

/* MWCC emits tentative definitions in reverse order within these sections. */
int gap_08_80510E9C_sbss;
/* This linker-visible four-byte gap is the sole small object retail keeps in .bss. */
#pragma section data_type ".data" ".bss"
__declspec(section ".data") unsigned long gap_06_803DEABC_bss;
#pragma section data_type
unsigned long g_GC_ARAM_MemBlocks[5];

/*
 * Soft ceiling: exact retail size and opcode stream; only nonvolatile-register
 * coloring and the compiler's local string relocation label remain.
 */
void gc_aram_mwmem_heap_setup(void) {
    unsigned long virtual_size;
    void* virtual_base;
    unsigned long vm_start = g_ARAM_VM_Start;

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
    unsigned long available_size;
    unsigned long vm_start;
    unsigned long vm_size;
    void* vm_base;
    static void* aramBase;
    static unsigned long aramSize;

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

unsigned long ARAM_MSL_GetSize(void) {
    return g_ARAM_MSL_Start == 0 ? 0 : g_ARAM_MSL_Size;
}

unsigned long ARAM_MSL_GetBase(void) {
    void* base = (void*)g_ARAM_MSL_Start;

    if (base == 0) {
        return 0;
    }
    return (unsigned long)base;
}

/* Reverse source order reproduces the retail .sbss symbol order. */
unsigned long g_ARAM_MSL_Size;
unsigned long g_ARAM_MSL_Start;
unsigned long g_ARAM_VM_Size;
unsigned long g_ARAM_VM_Start;
_mwMemHeap* SystemSwappableHeap;
