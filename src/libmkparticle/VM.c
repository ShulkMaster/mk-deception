#include "dolphin/ar.h"
#include "dolphin/cache.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"
#include "dolphin/types.h"
#include "dolphin/vm.h"

static u32 g_vmBaseVMARAM = 0x4000;
static u32 g_vmSizeVMMainMemory;
static char* g_vmBaseVMMainMemory;
static u32 g_vmSizeVMARAM;
static u32 g_vmNumPagesInMRAM;
static void (*g_cbLogStats)(u32 virtual_address, void* physical_address,
                            u32 physical_page, u32 elapsed, int wrote_page);
static int g_vmInitialized;

void __VMSwapPageIn(u32 virtual_address);

#define VM_TIME_UNITS() \
    ((u32)((OSGetTime() * 8) / (OS_TIMER_CLOCK / 125000)))

void VMInit(u32 virtual_memory_size, u32 aram_base, u32 aram_size)
{
    int interrupts;

    if (g_vmInitialized == 0) {
        interrupts = OSDisableInterrupts();
        g_vmInitialized = 1;
        g_vmBaseVMARAM = aram_base;
        g_vmSizeVMARAM = aram_size;
        g_vmSizeVMMainMemory = virtual_memory_size;
        g_vmNumPagesInMRAM = virtual_memory_size >> 12;
        VMBASEInit(__VMSwapPageIn);
        __VMAllocVirtualToARAMLUT();
        __VMAllocARAMToVirtualLUT();
        __VMAllocMRAMSwapSpace();
        OSRestoreInterrupts(interrupts);
    }
}

void VMQuit(void)
{
    if (g_vmInitialized == 1) {
        VMBASEQuit();
        __VMSetFreePagesExist(1);
        __VMSetNextPageToSwap(0);
        VMSetPageReplacementPolicy(1);
        g_vmSizeVMMainMemory = 0;
        g_vmBaseVMMainMemory = 0;
        g_vmSizeVMARAM = 0;
        g_vmBaseVMARAM = 0x4000;
        g_vmNumPagesInMRAM = 0;
        g_cbLogStats = 0;
        g_vmInitialized = 0;
    }
}

u32 __VMGetNumPagesInMRAM(void)
{
    return g_vmNumPagesInMRAM;
}

u32 VMGetARAMSize(void)
{
    return g_vmSizeVMARAM;
}

u32 VMGetARAMBase(void)
{
    return g_vmBaseVMARAM;
}

void __VMAllocMRAMSwapSpace(void)
{
    g_vmBaseVMMainMemory = OSGetArenaLo();
    OSSetArenaLo(g_vmBaseVMMainMemory + g_vmSizeVMMainMemory);
}

void __VMSwapPageIn(u32 virtual_address)
{
    u32 start_time;
    u32 page_address;
    void* physical_address;
    int wrote_page;
    u32 physical_page;
    int ar_interrupt_status;
    int interrupts;
    u32 page_virtual_address;

    start_time = VM_TIME_UNITS();
    page_address = virtual_address & ~0xFFF;
    wrote_page = 0;
    physical_page = __VMGetPageToReplace();
    physical_address =
        g_vmBaseVMMainMemory + (physical_page << 12);
    page_virtual_address =
        VMBASEGetVirtualAddrFromPageInMRAM(physical_page);
    interrupts = OSDisableInterrupts();

    while (ARGetDMAStatus() != 0) {
    }
    ar_interrupt_status = __ARGetInterruptStatus();

    if (page_virtual_address != 0) {
        if (VMBASEIsPageDirty(page_virtual_address)) {
            __VMSetARAMPageAsDirty(page_virtual_address);
            wrote_page = 1;
            DCFlushRange(physical_address, 0x1000);
            ARStartDMA(0, (u32)physical_address,
                       __VMTranslateVMPageToARAMPage(page_virtual_address),
                       0x1000);
            while (ARGetDMAStatus() != 0) {
            }
        }
        VMBASEClearPageTableEntry(page_virtual_address, physical_page);
    }

    if (__VMIsARAMPageDirty(page_address)) {
        ARStartDMA(1, (u32)physical_address,
                   __VMTranslateVMPageToARAMPage(page_address), 0x1000);
        while (ARGetDMAStatus() != 0) {
        }
        DCInvalidateRange(physical_address, 0x1000);
        ICInvalidateRange(physical_address, 0x1000);
    } else if (!__VMDoesMappingExist(page_address)) {
        __VMMappingErrorAlert(page_address);
    }

    if (ar_interrupt_status == 0) {
        __ARClearInterrupt();
    }
    VMBASESetPageTableEntry(page_address, physical_address, physical_page);
    OSRestoreInterrupts(interrupts);

    if (g_cbLogStats != 0) {
        g_cbLogStats(virtual_address, physical_address, physical_page,
                     VM_TIME_UNITS() - start_time, wrote_page);
    }
}
