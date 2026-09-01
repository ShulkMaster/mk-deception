#include "dolphin/base/PPCArch.h"
#include "dolphin/os_alloc.h"
#include "dolphin/types.h"

int sprintf(char* destination, const char* format, ...);
u32 VMGetARAMBase(void);
u32 VMGetARAMSize(void);

static u32* g_baseARAMtoVM;
static u32* g_baseVMtoARAM;
static u32 g_totalAllocatedVM;
static u32 g_nextARAMPageToCheck;

void __VMMappingErrorAlert(u32 virtual_address);

int VMAlloc(void* virtual_address, u32 size)
{
    u32 first_aram_page;
    u32 end_aram_page;
    u32 virtual_offset;
    u32 page_count;

    first_aram_page = VMGetARAMBase() >> 12;
    end_aram_page = first_aram_page + (VMGetARAMSize() >> 12);
    if (g_nextARAMPageToCheck < first_aram_page) {
        g_nextARAMPageToCheck = first_aram_page;
    }

    if (g_totalAllocatedVM + size > VMGetARAMSize()) {
        return 0;
    }

    virtual_offset = 0;
    page_count = (size + 0xFFF) >> 12;
    if (size > 0) {
        do {
            u32 virtual_page = (u32)virtual_address + virtual_offset;

            do {
                g_nextARAMPageToCheck++;
                if (g_nextARAMPageToCheck >= end_aram_page) {
                    g_nextARAMPageToCheck = first_aram_page;
                }
            } while (g_baseARAMtoVM[g_nextARAMPageToCheck] != 0);

            g_baseARAMtoVM[g_nextARAMPageToCheck] = virtual_page;
            virtual_offset += 0x1000;
            g_baseVMtoARAM[virtual_page >> 12] =
                g_nextARAMPageToCheck << 12;
            g_totalAllocatedVM += 0x1000;
        } while (--page_count != 0);
    }

    return 1;
}

u32 __VMTranslateVMPageToARAMPage(u32 virtual_address)
{
    u32 aram_page = g_baseVMtoARAM[virtual_address >> 12] & 0x7FFFFFFF;

    if (aram_page != 0) {
        return aram_page;
    }

    __VMMappingErrorAlert(virtual_address);
    return 0;
}

int __VMDoesMappingExist(u32 virtual_address)
{
    return (g_baseVMtoARAM[virtual_address >> 12] & 0x7FFFFFFF) != 0;
}

void __VMMappingErrorAlert(u32 virtual_address)
{
    char message[1024];

    sprintf(message,
            "Virtual address (%x) has not been allocated. Call VMAlloc on "
            "virtual address ranges before using them.",
            virtual_address);
    PPCHalt();
}

void __VMSetARAMPageAsDirty(u32 virtual_address)
{
    g_baseVMtoARAM[virtual_address >> 12] |= 0x80000000;
}

int __VMIsARAMPageDirty(u32 virtual_address)
{
    return g_baseVMtoARAM[virtual_address >> 12] >> 31;
}

void __VMAllocVirtualToARAMLUT(void)
{
    u32 i;

    g_baseVMtoARAM = OSGetArenaLo();
    OSSetArenaLo((char*)g_baseVMtoARAM + 0x8000);
    for (i = 0; i < 0x2000; i++) {
        g_baseVMtoARAM[i] = 0;
    }
}

void __VMAllocARAMToVirtualLUT(void)
{
    u32 i;

    g_baseARAMtoVM = OSGetArenaLo();
    OSSetArenaLo((char*)g_baseARAMtoVM + 0x4000);
    for (i = 0; i < 0x1000; i++) {
        g_baseARAMtoVM[i] = 0;
    }
}
