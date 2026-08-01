#ifndef MW_MWMEMNORMAL_H
#define MW_MWMEMNORMAL_H

#include "mw/mwMem.h"

void normHeapFreeMemFromBlock(void* block);
void* normHeapMallocMem(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request);
void normHeapResetHeap(_mwMemHeap* heap, int preserve_blocks);
void normHeapInitHeap(_mwMemHeap* heap);

#endif
