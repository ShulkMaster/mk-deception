#include "msl/mslgcn.h"
#include "mw/mwMemHeap.h"
#include "mw/mwMem.h"

extern "C" unsigned long mslMainRamUsed(void) {
    MwMemHeapInfo info;

    mwMemHeapGetInfo(MWSOUND_HEAP, &info);
    return info.currentUsedSize;
}
