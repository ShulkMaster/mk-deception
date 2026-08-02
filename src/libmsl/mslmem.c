#include "mw/mwMemHeap.h"
#include "mw/mwMem.h"

u32 mslMainRamUsed(void) {
    MwMemHeapInfo info;

    mwMemHeapGetInfo(MWSOUND_HEAP, &info);
    return info.currentUsedSize;
}
