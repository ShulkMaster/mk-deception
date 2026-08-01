#ifndef MW_MWMEMFIXED_H
#define MW_MWMEMFIXED_H

#include "mw/mwMem.h"

void fixedBlockHeapFreeBlock(_mwMemHeap* heap, void* block);
void* fixedBlockHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request);
void fixedBlockHeapResetHeap(_mwMemHeap* heap, int preserve_blocks);
void fixedBlockHeapInitHeap(_mwMemHeap* heap, const MwMemFixedParams* params);
u32 mwMemFixedBlockHeapGetHeapSize(const MwMemFixedParams* params);

#endif
