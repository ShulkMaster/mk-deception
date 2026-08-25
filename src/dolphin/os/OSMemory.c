#include "dolphin/cache.h"
#include "dolphin/os.h"

#define __MEMRegs ((volatile unsigned short*)0xCC004000)
#define __OSPhysicalMemSize (*(volatile unsigned long*)0x80000028)
#define __OSSimulatedMemSize (*(volatile unsigned long*)0x800000F0)

#define FALSE 0
#define TRUE 1
#define __OS_EXCEPTION_MEMORY_PROTECTION 15

static int OnReset(int final);
static void MEMIntrruptHandler(__OSInterrupt interrupt, OSContext* context);
static void Config24MB(void);
static void Config48MB(void);
static void RealMode(unsigned long address);

static OSResetFunctionInfo ResetFunctionInfo = {
    OnReset,
    0x7F,
    0,
    0,
};

static inline unsigned long OSGetPhysicalMemSize(void) {
    return __OSPhysicalMemSize;
}

static inline unsigned long OSGetConsoleSimulatedMemSize(void) {
    return __OSSimulatedMemSize;
}

static int OnReset(int final) {
    volatile unsigned short* memRegs = __MEMRegs;

    if (final != FALSE) {
        memRegs[8] = 0xFF;
        __OSMaskInterrupts(0xF0000000);
    }
    return TRUE;
}

static void MEMIntrruptHandler(__OSInterrupt interrupt, OSContext* context) {
    volatile unsigned short* memRegs = __MEMRegs;
    unsigned long addr;
    unsigned long cause;

    (void)interrupt;
    cause = memRegs[0xF];
    addr = ((unsigned long)(memRegs[0x12] & 0x3FF) << 16) | memRegs[0x11];
    memRegs[0x10] = 0;

    if (__OSErrorTable[__OS_EXCEPTION_MEMORY_PROTECTION]) {
        __OSErrorTable[__OS_EXCEPTION_MEMORY_PROTECTION](
            __OS_EXCEPTION_MEMORY_PROTECTION, context, cause, addr);
        return;
    }
    __OSUnhandledException(__OS_EXCEPTION_MEMORY_PROTECTION, context, cause, addr);
}

static void Config24MB(void) {
    /* TODO: This privileged leaf may have originated as assembly; recover an
     * intrinsic-based source form that emits the retail BAT/SRR sequence. */
}

static void Config48MB(void) {
    /* TODO: This privileged leaf may have originated as assembly; recover an
     * intrinsic-based source form that emits the retail BAT/SRR sequence. */
}

static void RealMode(unsigned long address) {
    (void)address;
    /* TODO: This privileged leaf may have originated as assembly; recover an
     * intrinsic-based source form that emits the retail SRR/rfi sequence. */
}

void __OSInitMemoryProtection(void) {
    unsigned long temp;
    int enabled;
    unsigned long size;

    size = OSGetConsoleSimulatedMemSize();
    enabled = OSDisableInterrupts();

    __MEMRegs[16] = 0;
    __MEMRegs[8] = 0xFF;

    __OSMaskInterrupts(0xF0000000);
    __OSSetInterruptHandler(0, MEMIntrruptHandler);
    __OSSetInterruptHandler(1, MEMIntrruptHandler);
    __OSSetInterruptHandler(2, MEMIntrruptHandler);
    __OSSetInterruptHandler(3, MEMIntrruptHandler);
    __OSSetInterruptHandler(4, MEMIntrruptHandler);
    OSRegisterResetFunction(&ResetFunctionInfo);

    temp = OSGetConsoleSimulatedMemSize();
    if (temp < OSGetPhysicalMemSize() && temp == 0x01800000) {
        DCInvalidateRange((void*)0x81800000, 0x01800000);
        __MEMRegs[20] = 2;
    }

    if (size <= 0x01800000) {
        RealMode((unsigned long)&Config24MB);
    } else if (size <= 0x03000000) {
        RealMode((unsigned long)&Config48MB);
    }

    __OSUnmaskInterrupts(0x08000000);
    OSRestoreInterrupts(enabled);
}
