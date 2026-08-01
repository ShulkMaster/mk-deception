#ifndef MW_MWMEMHDRLESS_H
#define MW_MWMEMHDRLESS_H

#include "mw/mwMem.h"

void hdrlessHeapFreeBlock(_mwMemHeap* heap, void* block);
void* hdrlessHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request);
void hdrlessHeapResetHeap(_mwMemHeap* heap);
void hdrlessHeapInitHeap(_mwMemHeap* heap, const MwMemHeaderlessParams* params);
u32 mwMemHeaderlessFixedBlockGetHeapSize(const MwMemHeaderlessParams* params);

#endif
