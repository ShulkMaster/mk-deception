#include "mw/mwMem_MultiThread.h"

#include "platform/os_types.h"
#include "dolphin/mutex.h"

OSMutex MemGC_SysMutex;
/* MWCC emits .sbss in reverse declaration order; pad first so MemSysMutex is at +0. */
int gap_08_80510EA4_sbss;
OSMutex* MemSysMutex;

void priv_mwMem_CritSecExit(void) {
    OSUnlockMutex(MemSysMutex);
}

void priv_mwMem_CritSecEnter(void) {
    if (MemSysMutex == 0) {
        OSInitMutex(&MemGC_SysMutex);
        MemSysMutex = &MemGC_SysMutex;
    }
    OSLockMutex(MemSysMutex);
}
