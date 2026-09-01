#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "dolphin/types.h"
#include "dolphin/vm.h"

typedef struct VMPageTableEntry {
    u32 virtual_page;
    u32 physical_page;
} VMPageTableEntry;

VMPageTableEntry* g_vmBasePageTable;
u32* g_vmBaseVMReversePageTable;
u8* g_vmBaseLockedPageTable;
void (*cbVMSwapPageIn)(u32);
u32 g_baseInitialized;

void __VMBASEClearPageFromTLB(u32 virtual_address);
void __VMBASESetVirtualAddressForPageInMRAM(u32 physical_page,
                                            u32 virtual_address);
void VMBASESetPageLocked(u32 physical_page, BOOL locked);
VMPageTableEntry* __VMBASEVirtualAddrToPageTableAddr(u32 virtual_address);
void __VMBASEInvalidateEntireTLB(void);
void __VMBASESetSwapPageCallback(void (*callback)(u32));
void __VMBASEInitPageTable(void);
void __VMBASEInitLockedPageTable(void);
void __VMBASEInitReversePageTable(void);
void __VMBASESetupExceptionHandlers(void);
void __VMBASESetupVMRegisters(void);

void VMBASEInit(void (*dsi_callback)(u32), void (*isi_callback)(u32),
                u32 pages_in_mram, BOOL enable_page_locking)
{
    BOOL interrupts;
    u32 arena_bytes;

    (void)isi_callback;
    (void)pages_in_mram;
    (void)enable_page_locking;

    if (g_baseInitialized == 0) {
        interrupts = OSDisableInterrupts();
        g_baseInitialized = 1;
        __VMBASESetSwapPageCallback(dsi_callback);
        arena_bytes = 0x10000 - ((u32)OSGetArenaLo() & 0xFFFF);
        if (arena_bytes >= 0x5000) {
            __VMBASEInitLockedPageTable();
            __VMBASEInitReversePageTable();
            __VMBASEInitPageTable();
        } else if (arena_bytes >= 0x4000) {
            __VMBASEInitReversePageTable();
            __VMBASEInitPageTable();
            __VMBASEInitLockedPageTable();
        } else if (arena_bytes >= 0x1000) {
            __VMBASEInitLockedPageTable();
            __VMBASEInitPageTable();
            __VMBASEInitReversePageTable();
        } else {
            __VMBASEInitPageTable();
            __VMBASEInitLockedPageTable();
            __VMBASEInitReversePageTable();
        }
        __VMBASESetupExceptionHandlers();
        __VMBASESetupVMRegisters();
        __VMBASEInvalidateEntireTLB();
        OSRestoreInterrupts(interrupts);
    }
}

void VMBASEQuit(void)
{
    /* TODO: Missing canonical function implementation. */
}

void VMBASESetPageTableEntry(u32 virtual_address, void* physical_address,
                             u32 physical_page)
{
    VMPageTableEntry* entry;
    BOOL interrupts;

    entry = __VMBASEVirtualAddrToPageTableAddr(virtual_address);
    interrupts = OSDisableInterrupts();
    entry->virtual_page = ((virtual_address >> 22) & 0x3F) | 0x80000000;
    entry->physical_page = (u32)physical_address & 0x0FFFF000;
    DCStoreRange(entry, sizeof(*entry));
    __VMBASEClearPageFromTLB(virtual_address);
    __VMBASESetVirtualAddressForPageInMRAM(physical_page, virtual_address);
    OSRestoreInterrupts(interrupts);
}

void VMBASEClearPageTableEntry(u32 virtual_address, u32 physical_page)
{
    BOOL interrupts;
    VMPageTableEntry* entry;

    interrupts = OSDisableInterrupts();
    entry = __VMBASEVirtualAddrToPageTableAddr(virtual_address);
    entry->virtual_page = 0;
    entry->physical_page = 0;
    DCStoreRange(entry, sizeof(*entry));
    __VMBASEClearPageFromTLB(virtual_address);
    __VMBASESetVirtualAddressForPageInMRAM(physical_page, 0);
    VMBASESetPageLocked(physical_page, 0);
    OSRestoreInterrupts(interrupts);
}

BOOL VMBASEIsPageValid(u32 virtual_address)
{
    (void)virtual_address;
    /* TODO: Missing canonical function implementation. */
    return 0;
}

BOOL VMBASEIsPageReferenced(u32 virtual_address)
{
    (void)virtual_address;
    /* TODO: Missing canonical function implementation. */
    return 0;
}

BOOL VMBASEIsPageDirty(u32 virtual_address)
{
    (void)virtual_address;
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void VMBASESetPageReferenced(u32 virtual_address, BOOL referenced)
{
    BOOL interrupts;
    VMPageTableEntry* entry;

    interrupts = OSDisableInterrupts();
    entry = __VMBASEVirtualAddrToPageTableAddr(virtual_address);
    if (referenced) {
        entry->physical_page |= 0x100;
    } else {
        entry->physical_page &= ~0x100;
    }
    DCStoreRange(&entry->physical_page, sizeof(entry->physical_page));
    __VMBASEClearPageFromTLB(virtual_address);
    OSRestoreInterrupts(interrupts);
}

void __VMBASEClearPageFromTLB(u32 virtual_address)
{
    (void)virtual_address;
}

u32 VMBASEGetVirtualAddrFromPageInMRAM(u32 physical_page)
{
    return g_vmBaseVMReversePageTable[physical_page];
}

void __VMBASESetVirtualAddressForPageInMRAM(u32 physical_page,
                                            u32 virtual_address)
{
    g_vmBaseVMReversePageTable[physical_page] = virtual_address;
}

BOOL VMBASEIsPageLocked(u32 physical_page)
{
    return g_vmBaseLockedPageTable[physical_page];
}

void VMBASESetPageLocked(u32 physical_page, BOOL locked)
{
    g_vmBaseLockedPageTable[physical_page] = locked;
}

void __VMBASESetSwapPageCallback(void (*callback)(u32))
{
    cbVMSwapPageIn = callback;
}

void __VMBASEInitPageTable(void)
{
    /* TODO: Missing canonical function implementation. */
}

void __VMBASEInitLockedPageTable(void)
{
    /* TODO: Missing canonical function implementation. */
}

void __VMBASEInitReversePageTable(void)
{
    /* TODO: Missing canonical function implementation. */
}

void __VMBASEInvalidatePageTable(void)
{
    BOOL interrupts;
    u32 i;

    interrupts = OSDisableInterrupts();
    for (i = 0; i < 0x2000; i++) {
        g_vmBasePageTable[i].virtual_page = 0;
        g_vmBasePageTable[i].physical_page = 0;
    }
    DCStoreRange(g_vmBasePageTable, 0x10000);
    __VMBASEInvalidateEntireTLB();
    OSRestoreInterrupts(interrupts);
}

void __VMBASEInvalidateLockedPageTable(void)
{
    u32 i;

    for (i = 0; i < 0x1000; i++) {
        g_vmBaseLockedPageTable[i] = 0;
    }
}

void *__VMBASEInvalidateReversePageTable(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

VMPageTableEntry* __VMBASEVirtualAddrToPageTableAddr(u32 virtual_address)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void __VMBASEInvalidateEntireTLB(void)
{
    /* Implemented by the retail architecture-specific TLB routine. */
}

void __VMBASESetupVMRegisters(void)
{
    /* TODO: Missing canonical function implementation. */
}

void *__VMBASESetupSDR1(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void __VMBASESetupExceptionHandlers(void)
{
    /* TODO: Missing canonical function implementation. */
}

void *__VMBASEDSIExceptionHandler(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *__VMBASEDSIServiceExceptionPrep(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *__VMBASEDSIServiceException(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *__VMBASEISIExceptionHandler(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *__VMBASEISIServiceExceptionPrep(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *__VMBASEISIServiceException(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *__VMBASERestoreExceptionHandlers(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

void *__VMBASERestoreVMRegisters(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}
