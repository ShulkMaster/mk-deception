#include "dolphin/ar.h"
#include "dolphin/os.h"
#include "dolphin/trk.h"
#include "dolphin/main_TRK.h"
#include "dolphin/dolphin_trk_glue.h"
#include "runtime/asm_sequences.inc"

extern u32 __TRK_get_MSR(void);
extern void TRKSaveExtended1Block(void);
extern char _db_stack_addr[];

#pragma section code_type ".init"
void __TRK_reset(void)
{
    OSResetSystem(0, 0, 0);
}

#pragma section code_type ".text"

static u32 lc_base;

static u32 TRK_ISR_OFFSETS[15] = {
    0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800,
    0x900, 0xC00, 0xD00, 0xF00, 0x1300, 0x1400, 0x1700,
};

asm void InitMetroTRK(void)
{
    SEQ_InitMetroTRK();
}

asm void InitMetroTRK_BBA(void)
{
    SEQ_InitMetroTRK_BBA();
}

asm void TRK__write_aram(void* buffer, u32 aram_address, u32* length)
{
    SEQ_TRK__write_aram();
}

asm void TRK__read_aram(void* buffer, u32 aram_address, u32* length)
{
    SEQ_TRK__read_aram();
}

void EnableMetroTRKInterrupts(void)
{
    EnableEXI2Interrupts();
}

u32 TRKTargetTranslate(u32 address)
{
    if (address >= lc_base && address < lc_base + 0x4000 &&
        (gTRKCPUState.extended1.dbat3u & 3) != 0) {
        return address;
    }
    if (address >= 0x7E000000 && address <= 0x80000000) {
        return address;
    }
    return (address & 0x3FFFFFFF) | 0x80000000;
}

void TRK_copy_vector(u32 offset)
{
    void* destination = (void*)TRKTargetTranslate(offset);

    TRK_memcpy(destination, gTRKInterruptVectorTable + offset, 0x100);
    TRK_flush_cache((u32)destination, 0x100);
}

void __TRK_copy_vectors(void)
{
    u32 mask_address;
    u32* offsets;
    int index;
    u32 mask;

    if (lc_base <= 0x44 && lc_base + 0x4000 > 0x44 &&
        (gTRKCPUState.extended1.dbat3u & 3) != 0) {
        mask_address = 0x44;
    } else {
        mask_address = 0x80000044;
    }

    index = 0;
    mask = *(u32*)mask_address;
    offsets = TRK_ISR_OFFSETS;

    do {
        if ((mask & (1 << index)) && index != 4) {
            TRK_copy_vector(offsets[index]);
        }
        index++;
    } while (index <= 14);
}

int TRKInitializeTarget(void)
{
    gTRKState.stopped = 1;
    gTRKState.msr = __TRK_get_MSR();
    lc_base = 0xE0000000;
    return 0;
}
