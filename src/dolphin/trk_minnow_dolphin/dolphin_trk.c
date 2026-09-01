#include "dolphin/trk.h"
#include "runtime/asm_sequences.inc"

extern u32 __TRK_get_MSR(void);
extern void EnableEXI2Interrupts(void);
extern void TRKSaveExtended1Block(void);
extern int InitMetroTRKCommTable(int hardware_id);
extern void TRK_main(void);
extern char _db_stack_addr[];

static u32 lc_base;

asm void InitMetroTRK(void)
{
    SEQ_InitMetroTRK();
}

asm void InitMetroTRK_BBA(void)
{
    SEQ_InitMetroTRK_BBA();
}

int TRKInitializeTarget(void)
{
    gTRKState.stopped = 1;
    gTRKState.saved_msr = __TRK_get_MSR();
    lc_base = 0xE0000000;
    return 0;
}

u32 TRKTargetTranslate(u32 address)
{
    if (address >= lc_base && address < lc_base + 0x4000 &&
        (gTRKCPUState.extended1_state & 3) != 0) {
        return address;
    }
    if (address >= 0x7E000000 && address <= 0x80000000) {
        return address;
    }
    return (address & 0x3FFFFFFF) | 0x80000000;
}

void EnableMetroTRKInterrupts(void)
{
    EnableEXI2Interrupts();
}
