#include "platform/gcARam.h"

#include "mw/mwMem.h"

extern unsigned long ARInit(unsigned long* stack_index_addr, unsigned long num_entries);
extern unsigned long ARGetSize(void);
extern unsigned long ARGetBaseAddress(void);
extern unsigned long ARAlloc(unsigned long length);

extern void VMInit(unsigned long virtual_memory_size, unsigned long aram_base,
                   unsigned long aram_size);
extern void VMAlloc(void* virtual_address, unsigned long size);

_mwMemHeap* SystemSwappableHeap;
unsigned long g_ARAM_VM_Start;
unsigned long g_ARAM_VM_Size;
unsigned long g_ARAM_MSL_Start;
unsigned long g_ARAM_MSL_Size;
unsigned long g_GC_ARAM_MemBlocks[5];

void gc_aram_mwmem_heap_setup(void) {
    void* virtual_base;
    unsigned long virtual_size;

    if ((g_ARAM_VM_Start ? g_ARAM_VM_Size : 0) != 0 && SystemSwappableHeap == 0) {
        virtual_base = (void*)0x7e000000;
        if (g_ARAM_VM_Start == 0) {
            virtual_base = 0;
        }
        virtual_size = g_ARAM_VM_Start ? g_ARAM_VM_Size : 0;
        SystemSwappableHeap = mwMemExtSystemHeapCreate(
            mwMemSystemGetHeap(0), virtual_base, virtual_size, "SwappableHeap");
    }
}

void gc_aram_init(void) {
    unsigned long available_size;
    unsigned long vm_start;
    static unsigned long aramSize;
    static void* aramBase;

    ARInit(g_GC_ARAM_MemBlocks, 5);
    available_size = ARGetSize() - ARGetBaseAddress();

    if (g_ARAM_MSL_Start == 0) {
        g_ARAM_MSL_Start = ARAlloc(0x800000);
        g_ARAM_MSL_Size = 0x800000;
    }

    if (g_ARAM_VM_Start == 0) {
        available_size -= 0x840000;
        vm_start = ARAlloc(available_size);
        g_ARAM_VM_Size = available_size;
        g_ARAM_VM_Start = vm_start;
        VMInit(0x100000, vm_start, available_size);
        VMAlloc((void*)0x7e000000, available_size);
        aramSize = g_ARAM_VM_Start ? g_ARAM_VM_Size : 0;
        aramBase = (void*)0x7e000000;
        if (g_ARAM_VM_Start == 0) {
            aramBase = 0;
        }
    }
}

unsigned long ARAM_MSL_GetSize(void) {
    return g_ARAM_MSL_Start ? g_ARAM_MSL_Size : 0;
}

void* ARAM_MSL_GetBase(void) {
    if (g_ARAM_MSL_Start == 0) {
        return 0;
    }
    return (void*)g_ARAM_MSL_Start;
}
