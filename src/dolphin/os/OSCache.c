#include "dolphin/cache.h"
#include "dolphin/base/PPCArch.h"
#include "dolphin/os.h"

extern void DBPrintf(const char* format, ...);

static inline void privilegedCacheTodo(void) {
    /* TODO: This privileged routine may have originated as assembly; recover an
     * intrinsic-based source form that emits the retail instruction sequence. */
}

void DCEnable(void) {
    privilegedCacheTodo();
}

void DCInvalidateRange(void* addr, unsigned long nBytes) {
    privilegedCacheTodo();
}

void DCFlushRange(void* addr, unsigned long nBytes) {
    unsigned long blocks;

    if (nBytes > 0) {
        blocks = (nBytes + ((unsigned long)addr & 0x1F) + 0x1F) >> 5;
        do {
            __dcbf(addr, 0);
            addr = (unsigned char*)addr + 0x20;
        } while (--blocks != 0);
        /* TODO: This routine may have originated as assembly; recover an
         * intrinsic-based expression for the retail terminal sc instruction. */
    }
}

void DCStoreRange(void* addr, unsigned long nBytes) {
    unsigned long blocks;

    if (nBytes > 0) {
        blocks = (nBytes + ((unsigned long)addr & 0x1F) + 0x1F) >> 5;
        do {
            __dcbst(addr, 0);
            addr = (unsigned char*)addr + 0x20;
        } while (--blocks != 0);
        /* TODO: This routine may have originated as assembly; recover an
         * intrinsic-based expression for the retail terminal sc instruction. */
    }
}

void DCFlushRangeNoSync(void* addr, unsigned long nBytes) {
    unsigned long blocks;

    if (nBytes > 0) {
        blocks = (nBytes + ((unsigned long)addr & 0x1F) + 0x1F) >> 5;
        do {
            __dcbf(addr, 0);
            addr = (unsigned char*)addr + 0x20;
        } while (--blocks != 0);
    }
}

void ICInvalidateRange(void* addr, unsigned long nBytes) {
    privilegedCacheTodo();
}

void ICFlashInvalidate(void) {
    privilegedCacheTodo();
}

void ICEnable(void) {
    privilegedCacheTodo();
}

void LCDisable(void) {
    privilegedCacheTodo();
}

static inline void L2Disable(void) {
    __sync();
    PPCMtl2cr(PPCMfl2cr() & ~0x80000000UL);
    __sync();
}

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

    (void)error;
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
