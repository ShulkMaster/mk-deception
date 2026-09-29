#include "dolphin/cache.h"
#include "dolphin/base/PPCArch.h"
#include "dolphin/os.h"
#include "runtime/asm_sequences.inc"

extern void DBPrintf(const char* format, ...);

asm void DCEnable(void) {
    SEQ_DCEnable();
}

asm void DCInvalidateRange(void* addr, unsigned long nBytes) {
    SEQ_DCInvalidateRange();
}

asm void DCFlushRange(void* addr, unsigned long nBytes) {
    SEQ_DCFlushRange();
}

asm void DCStoreRange(void* addr, unsigned long nBytes) {
    SEQ_DCStoreRange();
}

asm void DCFlushRangeNoSync(void* addr, unsigned long nBytes) {
    SEQ_DCFlushRangeNoSync();
}

asm void ICInvalidateRange(void* addr, unsigned long nBytes) {
    SEQ_ICInvalidateRange();
}

asm void ICFlashInvalidate(void) {
    SEQ_ICFlashInvalidate();
}

asm void ICEnable(void) {
    SEQ_ICEnable();
}

asm void LCDisable(void) {
    SEQ_LCDisable();
}

static inline void L2Disable(void) {
    __sync();
    PPCMtl2cr(PPCMfl2cr() & ~0x80000000UL);
    __sync();
}

/* The L2/DMA-error C code from here on was built without the peephole pass. */
#pragma peephole off

void L2GlobalInvalidate(void) {
    L2Disable();
    PPCMtl2cr(PPCMfl2cr() | 0x00200000);
    while (PPCMfl2cr() & 1) {
    }

    PPCMtl2cr(PPCMfl2cr() & ~0x00200000UL);
    while (PPCMfl2cr() & 1) {
        DBPrintf(">>> L2 INVALIDATE : SHOULD NEVER HAPPEN\n");
    }
}

void DMAErrorHandler(OSError error, OSContext* context, ...) {
    unsigned long hid2 = PPCMfhid2();

    OSReport("Machine check received\n");
    OSReport("HID2 = 0x%x   SRR1 = 0x%x\n", hid2, context->srr1);
    if (!(hid2 & 0x00F00000) || !(context->srr1 & 0x00200000)) {
        OSReport("Machine check was not DMA/locked cache related\n");
        OSDumpContext(context);
        PPCHalt();
    }

    OSReport("DMAErrorHandler(): An error occurred while processing DMA.\n");
    OSReport("The following errors have been detected and cleared :\n");

    if (hid2 & 0x00800000) {
        OSReport("\t- Requested a locked cache tag that was already in the cache\n");
    }
    if (hid2 & 0x00400000) {
        OSReport("\t- DMA attempted to access normal cache\n");
    }
    if (hid2 & 0x00200000) {
        OSReport("\t- DMA missed in data cache\n");
    }
    if (hid2 & 0x00100000) {
        OSReport("\t- DMA queue overflowed\n");
    }

    PPCMthid2(hid2);
}

static inline void L2Init(void) {
    unsigned long oldMSR = PPCMfmsr();

    __sync();
    PPCMtmsr(0x30);
    __sync();
    L2Disable();
    L2GlobalInvalidate();
    PPCMtmsr(oldMSR);
}

static inline void L2Enable(void) {
    PPCMtl2cr((PPCMfl2cr() | 0x80000000) & ~0x00200000UL);
}

void __OSCacheInit(void) {
    if (!(PPCMfhid0() & 0x00008000)) {
        ICEnable();
        DBPrintf("L1 i-caches initialized\n");
    }
    if (!(PPCMfhid0() & 0x00004000)) {
        DCEnable();
        DBPrintf("L1 d-caches initialized\n");
    }
    if (!(PPCMfl2cr() & 0x80000000)) {
        L2Init();
        L2Enable();
        DBPrintf("L2 cache initialized\n");
    }

    OSSetErrorHandler(1, DMAErrorHandler);
    DBPrintf("Locked cache machine check handler installed\n");
}
