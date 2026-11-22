#include "msl/mslgcn.h"
#include "mw/mwMemHeap.h"
#include "mw/mwMem.h"

extern "C" u32 mslMainRamUsed(void) {
    MwMemHeapInfo info;

    mwMemHeapGetInfo(MWSOUND_HEAP, &info);
    return info.currentUsedSize;
}
