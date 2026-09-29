#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "dolphin/types.h"
#include "dolphin/vm.h"
#include "runtime/asm_sequences.inc"

typedef struct VMPageTableEntry {
    u32 virtual_page;
    u32 physical_page;
} VMPageTableEntry;

static VMPageTableEntry* g_vmBasePageTable;
static u32* g_vmBaseVMReversePageTable;
static u8* g_vmBaseLockedPageTable;
static void (*cbVMSwapPageIn)(u32);
static u32 g_baseInitialized;
static u32 g_originalSR7;
static u32 g_originalSDR1;

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
static void __VMBASESetupSDR1(u32 msr, u32 sdr1);
static void __VMBASEDSIExceptionHandler(void);
static void __VMBASEISIExceptionHandler(void);
void __VMBASEDSIServiceExceptionPrep(void);
void* __VMBASEDSIServiceException(void);
void __VMBASEISIServiceExceptionPrep(void);
void* __VMBASEISIServiceException(void);
void __VMBASESetupVMRegisters_SetSDR1(void);
void __VMBASESetupVMRegisters_End(void);
void __VMBASEDSIExceptionHandler_SetOriginalInstruction(void);
void __VMBASEDSIExceptionHandler_SetBranchBack(void);
void __VMBASEISIExceptionHandler_SetOriginalInstruction(void);
void __VMBASEISIExceptionHandler_SetBranchBack(void);

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

#pragma push
asm void __VMBASEClearPageFromTLB(u32 virtual_address)
{
    SEQ___VMBASEClearPageFromTLB();
}
#pragma pop

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

#pragma push
asm void __VMBASEInvalidateEntireTLB(void)
{
    SEQ___VMBASEInvalidateEntireTLB();
}

asm void __VMBASESetupVMRegisters(void)
{
    SEQ___VMBASESetupVMRegisters();
}

static asm void __VMBASESetupSDR1(u32 msr, u32 sdr1)
{
    SEQ___VMBASESetupSDR1();
}

asm void __VMBASESetupExceptionHandlers(void)
{
    SEQ___VMBASESetupExceptionHandlers();
}

static asm void __VMBASEDSIExceptionHandler(void)
{
    SEQ___VMBASEDSIExceptionHandler();
}

asm void __VMBASEDSIServiceExceptionPrep(void)
{
    SEQ___VMBASEDSIServiceExceptionPrep();
}
#pragma pop

void *__VMBASEDSIServiceException(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

#pragma push
static asm void __VMBASEISIExceptionHandler(void)
{
    SEQ___VMBASEISIExceptionHandler();
}

asm void __VMBASEISIServiceExceptionPrep(void)
{
    SEQ___VMBASEISIServiceExceptionPrep();
}
#pragma pop

void *__VMBASEISIServiceException(void)
{
    /* TODO: Missing canonical function implementation. */
    return 0;
}

/* TODO: [blocked] 3.89%; approved for a sequence, but MWCC 1.2.5n asm hits an internal
 * compiler error on `lwz rX, code_label@l(rY)`; retail likely reads the patch slot from C. */
void *__VMBASERestoreExceptionHandlers(void)
{
    return 0;
}

#pragma push
asm void __VMBASERestoreVMRegisters(void)
{
    SEQ___VMBASERestoreVMRegisters();
}
#pragma pop
