#include "dolphin/base/PPCArch.h"
#include "dolphin/os.h"

#define INTERRUPT_MASK(index) (1UL << (31 - (index)))
#define MASK_MEM 0xF8000000UL
#define MASK_DSP 0x07000000UL
#define MASK_AI 0x00800000UL
#define MASK_EXI0 0x00700000UL
#define MASK_EXI1 0x000E0000UL
#define MASK_EXI2 0x00018000UL
#define MASK_EXI 0x007F8000UL
#define MASK_PI 0x00007FE0UL

#define GLOBAL_MASK (*(volatile OSInterruptMask*)0x800000C4)
#define LOCAL_MASK (*(volatile OSInterruptMask*)0x800000C8)
#define MEM_REGS ((volatile unsigned short*)0xCC004000)
#define DSP_REGS ((volatile unsigned short*)0xCC005000)
#define AI_REGS ((volatile unsigned long*)0xCC006C00)
#define EXI_REGS ((volatile unsigned long*)0xCC006800)
#define PI_REGS ((volatile unsigned long*)0xCC003000)

extern void* memset(void* destination, int value, unsigned long size);

static __OSInterruptHandler* InterruptHandlerTable;

volatile unsigned long __OSLastInterruptSrr0;
volatile __OSInterrupt __OSLastInterrupt;
volatile OSTime __OSLastInterruptTime;

static OSInterruptMask InterruptPrioTable[] = {
    0x00000100, 0x00000040, 0xF8000000, 0x00000200,
    0x00000080, 0x00003000, 0x00000020, 0x03FF8C00,
    0x04000000, 0x00004000, 0xFFFFFFFF,
};

static void ExternalInterruptHandler(__OSException exception,
                                     OSContext* context);

int OSDisableInterrupts(void)
{
    unsigned long msr = PPCMfmsr();
    PPCMtmsr(msr & ~0x8000);
    return (msr >> 15) & 1;
}

int OSEnableInterrupts(void)
{
    unsigned long msr = PPCMfmsr();
    PPCMtmsr(msr | 0x8000);
    return (msr >> 15) & 1;
}

int OSRestoreInterrupts(int enabled)
{
    unsigned long msr = PPCMfmsr();
    PPCMtmsr(enabled ? msr | 0x8000 : msr & ~0x8000);
    return (msr >> 15) & 1;
}

__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt interrupt,
                                             __OSInterruptHandler handler)
{
    __OSInterruptHandler previous = InterruptHandlerTable[interrupt];
    InterruptHandlerTable[interrupt] = handler;
    return previous;
}

__OSInterruptHandler __OSGetInterruptHandler(__OSInterrupt interrupt)
{
    return InterruptHandlerTable[interrupt];
}

void __OSInterruptInit(void)
{
    InterruptHandlerTable = (__OSInterruptHandler*)0x80003040;
    memset(InterruptHandlerTable, 0, 32 * sizeof(__OSInterruptHandler));
    GLOBAL_MASK = 0;
    LOCAL_MASK = 0;
    PI_REGS[1] = 0xF0;
    __OSMaskInterrupts(MASK_MEM | MASK_DSP | MASK_AI | MASK_EXI | MASK_PI);
    __OSSetExceptionHandler(4, ExternalInterruptHandler);
}

static OSInterruptMask SetInterruptMask(OSInterruptMask mask,
                                        OSInterruptMask current)
{
    unsigned long reg;
    unsigned long interrupt = __cntlzw(mask);

    switch (interrupt) {
    case 0: case 1: case 2: case 3: case 4:
        reg = 0;
        if (!(current & INTERRUPT_MASK(0))) reg |= 0x01;
        if (!(current & INTERRUPT_MASK(1))) reg |= 0x02;
        if (!(current & INTERRUPT_MASK(2))) reg |= 0x04;
        if (!(current & INTERRUPT_MASK(3))) reg |= 0x08;
        if (!(current & INTERRUPT_MASK(4))) reg |= 0x10;
        MEM_REGS[14] = reg;
        mask &= ~MASK_MEM;
        break;
    case 5: case 6: case 7:
        reg = DSP_REGS[5] & ~0x1F8;
        if (!(current & INTERRUPT_MASK(5))) reg |= 0x10;
        if (!(current & INTERRUPT_MASK(6))) reg |= 0x40;
        if (!(current & INTERRUPT_MASK(7))) reg |= 0x100;
        DSP_REGS[5] = reg;
        mask &= ~MASK_DSP;
        break;
    case 8:
        reg = AI_REGS[0] & ~0x2C;
        if (!(current & INTERRUPT_MASK(8))) reg |= 0x04;
        AI_REGS[0] = reg;
        mask &= ~MASK_AI;
        break;
    case 9: case 10: case 11:
        reg = EXI_REGS[0] & ~0x2C0F;
        if (!(current & INTERRUPT_MASK(9))) reg |= 0x001;
        if (!(current & INTERRUPT_MASK(10))) reg |= 0x004;
        if (!(current & INTERRUPT_MASK(11))) reg |= 0x400;
        EXI_REGS[0] = reg;
        mask &= ~MASK_EXI0;
        break;
    case 12: case 13: case 14:
        reg = EXI_REGS[5] & ~0xC0F;
        if (!(current & INTERRUPT_MASK(12))) reg |= 0x001;
        if (!(current & INTERRUPT_MASK(13))) reg |= 0x004;
        if (!(current & INTERRUPT_MASK(14))) reg |= 0x400;
        EXI_REGS[5] = reg;
        mask &= ~MASK_EXI1;
        break;
    case 15: case 16:
        reg = EXI_REGS[10] & ~0xF;
        if (!(current & INTERRUPT_MASK(15))) reg |= 0x1;
        if (!(current & INTERRUPT_MASK(16))) reg |= 0x4;
        EXI_REGS[10] = reg;
        mask &= ~MASK_EXI2;
        break;
    case 17: case 18: case 19: case 20: case 21:
    case 22: case 23: case 24: case 25: case 26:
        reg = 0xF0;
        if (!(current & INTERRUPT_MASK(17))) reg |= 0x0800;
        if (!(current & INTERRUPT_MASK(20))) reg |= 0x0008;
        if (!(current & INTERRUPT_MASK(21))) reg |= 0x0004;
        if (!(current & INTERRUPT_MASK(22))) reg |= 0x0002;
        if (!(current & INTERRUPT_MASK(23))) reg |= 0x0001;
        if (!(current & INTERRUPT_MASK(24))) reg |= 0x0100;
        if (!(current & INTERRUPT_MASK(25))) reg |= 0x1000;
        if (!(current & INTERRUPT_MASK(18))) reg |= 0x0200;
        if (!(current & INTERRUPT_MASK(19))) reg |= 0x0400;
        if (!(current & INTERRUPT_MASK(26))) reg |= 0x2000;
        PI_REGS[1] = reg;
        mask &= ~MASK_PI;
        break;
    }
    return mask;
}

OSInterruptMask __OSMaskInterrupts(OSInterruptMask global)
{
    OSInterruptMask previous;
    OSInterruptMask local;
    OSInterruptMask mask;
    int enabled = OSDisableInterrupts();

    previous = GLOBAL_MASK;
    local = LOCAL_MASK;
    mask = ~(previous | local) & global;
    global |= previous;
    GLOBAL_MASK = global;
    while (mask) mask = SetInterruptMask(mask, global | local);
    OSRestoreInterrupts(enabled);
    return previous;
}

OSInterruptMask __OSUnmaskInterrupts(OSInterruptMask global)
{
    OSInterruptMask previous;
    OSInterruptMask local;
    OSInterruptMask mask;
    int enabled = OSDisableInterrupts();

    previous = GLOBAL_MASK;
    local = LOCAL_MASK;
    mask = (previous | local) & global;
    global = previous & ~global;
    GLOBAL_MASK = global;
    while (mask) mask = SetInterruptMask(mask, global | local);
    OSRestoreInterrupts(enabled);
    return previous;
}

void __OSDispatchInterrupt(__OSException exception, OSContext* context)
{
    unsigned long interrupt_status;
    unsigned long reg;
    OSInterruptMask cause = 0;
    OSInterruptMask unmasked;
    OSInterruptMask* priority;
    __OSInterrupt interrupt;
    __OSInterruptHandler handler;

    interrupt_status = PI_REGS[0] & ~0x00010000;
    if (!interrupt_status || !(interrupt_status & PI_REGS[1])) {
        OSLoadContext(context);
    }

    if (interrupt_status & 0x80) {
        reg = MEM_REGS[15];
        if (reg & 0x01) cause |= INTERRUPT_MASK(0);
        if (reg & 0x02) cause |= INTERRUPT_MASK(1);
        if (reg & 0x04) cause |= INTERRUPT_MASK(2);
        if (reg & 0x08) cause |= INTERRUPT_MASK(3);
        if (reg & 0x10) cause |= INTERRUPT_MASK(4);
    }
    if (interrupt_status & 0x40) {
        reg = DSP_REGS[5];
        if (reg & 0x08) cause |= INTERRUPT_MASK(5);
        if (reg & 0x20) cause |= INTERRUPT_MASK(6);
        if (reg & 0x80) cause |= INTERRUPT_MASK(7);
    }
    if ((interrupt_status & 0x20) && (AI_REGS[0] & 0x08)) {
        cause |= INTERRUPT_MASK(8);
    }
    if (interrupt_status & 0x10) {
        reg = EXI_REGS[0];
        if (reg & 0x002) cause |= INTERRUPT_MASK(9);
        if (reg & 0x008) cause |= INTERRUPT_MASK(10);
        if (reg & 0x800) cause |= INTERRUPT_MASK(11);
        reg = EXI_REGS[5];
        if (reg & 0x002) cause |= INTERRUPT_MASK(12);
        if (reg & 0x008) cause |= INTERRUPT_MASK(13);
        if (reg & 0x800) cause |= INTERRUPT_MASK(14);
        reg = EXI_REGS[10];
        if (reg & 0x002) cause |= INTERRUPT_MASK(15);
        if (reg & 0x008) cause |= INTERRUPT_MASK(16);
    }

    if (interrupt_status & 0x2000) cause |= INTERRUPT_MASK(26);
    if (interrupt_status & 0x1000) cause |= INTERRUPT_MASK(25);
    if (interrupt_status & 0x0400) cause |= INTERRUPT_MASK(19);
    if (interrupt_status & 0x0200) cause |= INTERRUPT_MASK(18);
    if (interrupt_status & 0x0100) cause |= INTERRUPT_MASK(24);
    if (interrupt_status & 0x0008) cause |= INTERRUPT_MASK(20);
    if (interrupt_status & 0x0004) cause |= INTERRUPT_MASK(21);
    if (interrupt_status & 0x0002) cause |= INTERRUPT_MASK(22);
    if (interrupt_status & 0x0800) cause |= INTERRUPT_MASK(17);
    if (interrupt_status & 0x0001) cause |= INTERRUPT_MASK(23);

    unmasked = cause & ~(GLOBAL_MASK | LOCAL_MASK);
    if (unmasked) {
        for (priority = InterruptPrioTable;; priority++) {
            if (unmasked & *priority) {
                interrupt = __cntlzw(unmasked & *priority);
                break;
            }
        }
        handler = __OSGetInterruptHandler(interrupt);
        if (handler) {
            if (interrupt > 4) {
                __OSLastInterrupt = interrupt;
                __OSLastInterruptTime = OSGetTime();
                __OSLastInterruptSrr0 = context->srr0;
            }
            OSDisableScheduler();
            handler(interrupt, context);
            OSEnableScheduler();
            __OSReschedule();
            OSLoadContext(context);
        }
    }
    OSLoadContext(context);
}

/* Retail saves volatile GPR and GQR state in a privileged assembly leaf. */
static void ExternalInterruptHandler(__OSException exception,
                                     OSContext* context)
{
    __OSDispatchInterrupt(exception, context);
}
