#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"
#include "dolphin/types.h"
#include "dolphin/vm.h"
#include "runtime/asm_sequences.inc"

struct VMPageTableEntry {
    u32 virtual_page;
    u32 physical_page;
};

static struct VMPageTableEntry* g_vmBasePageTable;
static u32* g_vmBaseVMReversePageTable;
static u8* g_vmBaseLockedPageTable;
static void (*cbVMSwapPageIn)(u32);
static BOOL g_baseInitialized;
static u32 g_originalSR7;
static u32 g_originalSDR1;

void __VMBASEClearPageFromTLB(u32 virtual_address);
void __VMBASESetVirtualAddressForPageInMRAM(u32 physical_page,
                                            u32 virtual_address);
void VMBASESetPageLocked(u32 physical_page, BOOL locked);
struct VMPageTableEntry* __VMBASEVirtualAddrToPageTableAddr(u32 virtual_address);
void __VMBASEInvalidateEntireTLB(void);
void __VMBASESetSwapPageCallback(void (*callback)(u32));
void __VMBASEInitPageTable(void);
void __VMBASEInvalidatePageTable(void);
void __VMBASEInitLockedPageTable(void);
void __VMBASEInitReversePageTable(void);
void __VMBASEInvalidateReversePageTable(void);
void __VMBASESetupExceptionHandlers(void);
void __VMBASESetupVMRegisters(void);
static void __VMBASESetupSDR1(u32 msr, u32 sdr1);
static void __VMBASEDSIExceptionHandler(void);
static void __VMBASEISIExceptionHandler(void);
void __VMBASEDSIServiceExceptionPrep(void);
void __VMBASEDSIServiceException(OSContext* context, u32 virtual_address);
void __VMBASEISIServiceExceptionPrep(void);
void __VMBASEISIServiceException(OSContext* context);
void __VMBASESetupVMRegisters_SetSDR1(void);
void __VMBASESetupVMRegisters_End(void);
void __VMBASEDSIExceptionHandler_SetOriginalInstruction(void);
void __VMBASEDSIExceptionHandler_SetBranchBack(void);
void __VMBASEISIExceptionHandler_SetOriginalInstruction(void);
void __VMBASEISIExceptionHandler_SetBranchBack(void);

void VMBASEInit(void (*swap_page_callback)(u32))
{
    BOOL interrupts;
    u32 arena_bytes;

    if (g_baseInitialized == 0) {
        interrupts = OSDisableInterrupts();
        g_baseInitialized = 1;
        __VMBASESetSwapPageCallback(swap_page_callback);
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
    BOOL interrupts;

    interrupts = OSDisableInterrupts();
    __VMBASERestoreExceptionHandlers();
    __VMBASERestoreVMRegisters();
    __VMBASEInvalidateEntireTLB();
    g_vmBasePageTable = 0;
    g_vmBaseVMReversePageTable = 0;
    g_vmBaseLockedPageTable = 0;
    cbVMSwapPageIn = 0;
    g_baseInitialized = 0;
    OSRestoreInterrupts(interrupts);
}


void VMBASESetPageTableEntry(u32 virtual_address, void* physical_address,
                             u32 physical_page)
{
    struct VMPageTableEntry* entry;
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
    struct VMPageTableEntry* entry;

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
    return __VMBASEVirtualAddrToPageTableAddr(virtual_address)->virtual_page >> 31;
}

BOOL VMBASEIsPageReferenced(u32 virtual_address)
{
    return (__VMBASEVirtualAddrToPageTableAddr(virtual_address)->physical_page >> 8) & 1;
}

BOOL VMBASEIsPageDirty(u32 virtual_address)
{
    return (__VMBASEVirtualAddrToPageTableAddr(virtual_address)->physical_page >> 7) & 1;
}

void VMBASESetPageReferenced(u32 virtual_address, BOOL referenced)
{
    BOOL interrupts;
    struct VMPageTableEntry* entry;

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
    if (locked) {
        g_vmBaseLockedPageTable[physical_page] = 1;
    } else {
        g_vmBaseLockedPageTable[physical_page] = 0;
    }
}

void __VMBASESetSwapPageCallback(void (*callback)(u32))
{
    cbVMSwapPageIn = callback;
}

void __VMBASEInitPageTable(void)
{
    u32 arena_lo = (u32)OSGetArenaLo();

    g_vmBasePageTable = (struct VMPageTableEntry*)((arena_lo + 0x10000) - (arena_lo & 0xFFFF));
    OSSetArenaLo(g_vmBasePageTable + 0x2000);
    __VMBASEInvalidatePageTable();
}

void __VMBASEInitLockedPageTable(void)
{
    u8* arena_lo;

    arena_lo = OSGetArenaLo();
    g_vmBaseLockedPageTable = arena_lo;
    OSSetArenaLo(arena_lo + 0x1000);
    __VMBASEInvalidateLockedPageTable();
}


void __VMBASEInitReversePageTable(void)
{
    g_vmBaseVMReversePageTable = OSGetArenaLo();
    OSSetArenaLo(g_vmBaseVMReversePageTable + 0x1000);
    __VMBASEInvalidateReversePageTable();
}

/* TODO: [breakthrough] 79.17%; serial eight-entry clear recovered; size-mode entry/remainder guards remain. */
void __VMBASEInvalidatePageTable(void)
{
    BOOL interrupts;
    u32 i;

    interrupts = OSDisableInterrupts();
    for (i = 0; i != 0x2000; i++) {
        g_vmBasePageTable[i].virtual_page = 0;
        g_vmBasePageTable[i].physical_page = 0;
    }
    DCStoreRange(g_vmBasePageTable, 0x2000 * sizeof(*g_vmBasePageTable));
    __VMBASEInvalidateEntireTLB();
    OSRestoreInterrupts(interrupts);
}

/* TODO: [breakthrough needed] 42.41%; byte extent/reloads agree; original grouped-address lowering remains unresolved. */
void __VMBASEInvalidateLockedPageTable(void)
{
    u32 i;

    for (i = 0; i < 0x1000; i++) {
        g_vmBaseLockedPageTable[i] = 0;
    }
}

/* TODO: [near miss] 97.60%; initial constant order and zero copy remain; stop at codegen. */
void __VMBASEInvalidateReversePageTable(void)
{
    u32 i;

    for (i = 0; i != 0x1000; i++) {
        g_vmBaseVMReversePageTable[i] = 0;
    }
}

struct VMPageTableEntry* __VMBASEVirtualAddrToPageTableAddr(u32 virtual_address)
{
    return (struct VMPageTableEntry*)(
        ((virtual_address >> 19) & 0x38) |
        ((u32)g_vmBasePageTable | ((virtual_address >> 6) & 0xFFC0)));
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

void __VMBASEDSIServiceException(OSContext* context, u32 virtual_address)
{
    OSContext temporary_context;

    OSClearContext(&temporary_context);
    OSSetCurrentContext(&temporary_context);
    cbVMSwapPageIn(virtual_address);
    OSSetCurrentContext(context);
    OSLoadContext(context);
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

void __VMBASEISIServiceException(OSContext* context)
{
    OSContext temporary_context;

    OSClearContext(&temporary_context);
    OSSetCurrentContext(&temporary_context);
    cbVMSwapPageIn(context->srr0);
    OSSetCurrentContext(context);
    OSLoadContext(context);
}

/* TODO: [blocked] 3.89%; patch-slot/cache-barrier assembly recovery is outside automated scope; retain stub pending authorized recovery. */
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
