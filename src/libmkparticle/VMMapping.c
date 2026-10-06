#include "dolphin/base/PPCArch.h"
#include "dolphin/os_alloc.h"
#include "dolphin/types.h"
#include "dolphin/vm.h"
#include "runtime/cstdio.h"

static u32* g_baseARAMtoVM;
static u32* g_baseVMtoARAM;
static u32 g_totalAllocatedVM;
static u32 g_nextARAMPageToCheck;

/* TODO: [near miss] 96.88%; virtual mask and CTR loop agree;
 * byte offset folds into cursor and parameter homes swap. */
int VMAlloc(void* virtual_address, u32 size)
{
    u32 first_aram_page;
    u32 end_aram_page;
    u32 virtual_offset;

    first_aram_page = VMGetARAMBase() >> 12;
    end_aram_page = first_aram_page + (VMGetARAMSize() >> 12);
    if (g_nextARAMPageToCheck < first_aram_page) {
        g_nextARAMPageToCheck = first_aram_page;
    }

    if (g_totalAllocatedVM + size > VMGetARAMSize()) {
        return 0;
    }

    for (virtual_offset = 0; virtual_offset < size; virtual_offset += 0x1000) {
        u8* virtual_page = (u8*)virtual_address + virtual_offset;

        do {
            g_nextARAMPageToCheck++;
            if (g_nextARAMPageToCheck >= end_aram_page) {
                g_nextARAMPageToCheck = first_aram_page;
            }
        } while (g_baseARAMtoVM[g_nextARAMPageToCheck] != 0);

        g_baseARAMtoVM[g_nextARAMPageToCheck] = (u32)virtual_page;
        g_baseVMtoARAM[((u32)virtual_page >> 12) & 0x1FFF] =
            g_nextARAMPageToCheck << 12;
        g_totalAllocatedVM += 0x1000;
    }

    return 1;
}

u32 __VMTranslateVMPageToARAMPage(u32 virtual_address)
{
    u32 aram_page = g_baseVMtoARAM[(virtual_address >> 12) & 0x1FFF] & 0x7FFFFFFF;

    if (aram_page != 0) {
        return aram_page;
    }

    __VMMappingErrorAlert(virtual_address);
    return 0;
}

BOOL __VMDoesMappingExist(u32 virtual_address)
{
    return (g_baseVMtoARAM[(virtual_address >> 12) & 0x1FFF] & 0x7FFFFFFF) != 0;
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
    g_baseVMtoARAM[(virtual_address >> 12) & 0x1FFF] |= 0x80000000;
}

BOOL __VMIsARAMPageDirty(u32 virtual_address)
{
    return g_baseVMtoARAM[(virtual_address >> 12) & 0x1FFF] >> 31;
}

/* TODO: [near miss] 68.93%; clear loop agrees; arena pointer copy/publication and zero setup remain. */
void __VMAllocVirtualToARAMLUT(void)
{
    u32 i;

    g_baseVMtoARAM = OSGetArenaLo();
    OSSetArenaLo(g_baseVMtoARAM + 0x2000);
    for (i = 0; i != 0x2000; i++) {
        g_baseVMtoARAM[i] = 0;
    }
}

/* TODO: [near miss] 98.20%; equality loop restores the retail clear body;
 * CTR/index/zero setup ordering and zero-copy peephole remain. */
void __VMAllocARAMToVirtualLUT(void)
{
    u32 i;

    g_baseARAMtoVM = OSGetArenaLo();
    OSSetArenaLo(g_baseARAMtoVM + 0x1000);
    for (i = 0; i != 0x1000; i++) {
        g_baseARAMtoVM[i] = 0;
    }
}
