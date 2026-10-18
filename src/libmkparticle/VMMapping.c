#include "dolphin/base/PPCArch.h"
#include "dolphin/os_alloc.h"
#include "dolphin/types.h"
#include "dolphin/vm.h"
#include "runtime/cstdio.h"

static u32* g_baseARAMtoVM = 0;
static u32* g_baseVMtoARAM = 0;
static u32 g_totalAllocatedVM = 0;

#pragma opt_strength_reduction off
int VMAlloc(void* virtual_address, u32 size)
{
    static u32 g_nextARAMPageToCheck;
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
#pragma opt_strength_reduction reset

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

/* TODO: [near miss] 96.07143%; arena-result copy is removed before publication;
 * recover an actual source-definition boundary across argument setup. */
void __VMAllocVirtualToARAMLUT(void)
{
    u32 byte_offset;

    g_baseVMtoARAM = OSGetArenaLo();
    OSSetArenaLo(g_baseVMtoARAM + 0x2000);
    for (byte_offset = 0;
         byte_offset != 0x2000 * sizeof(*g_baseVMtoARAM);
         byte_offset += sizeof(*g_baseVMtoARAM)) {
        *(u32*)((u8*)g_baseVMtoARAM + byte_offset) = 0;
    }
}

static inline void clear_aram_virtual_mapping(u32 byte_offset)
{
    *(u32*)((u8*)g_baseARAMtoVM + byte_offset) = 0;
}

void __VMAllocARAMToVirtualLUT(void)
{
    u32 byte_offset;

    g_baseARAMtoVM = OSGetArenaLo();
    OSSetArenaLo(g_baseARAMtoVM + 0x1000);
    for (byte_offset = 0;
         byte_offset != 0x1000 * sizeof(*g_baseARAMtoVM);
         byte_offset += sizeof(*g_baseARAMtoVM)) {
        clear_aram_virtual_mapping(byte_offset);
    }
}
