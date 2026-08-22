#ifndef MW_MWMEMPRIV_H
#define MW_MWMEMPRIV_H

#include "mw/mwMem.h"

#define MW_MEM_ALIGN_UP_16(value) (((value) + 0xF) & ~0xFU)

/* Centralized byte-layout navigation for the allocator's packed arenas. */
#define mwMemByteAddress(base, byteOffset) ((u8*)(base) + (byteOffset))
#define mwMemHeaderAt(base, byteOffset) \
    ((MwMemUsedHeader*)mwMemByteAddress((base), (byteOffset)))
#define mwMemHeaderBefore(block, byteOffset) \
    ((MwMemUsedHeader*)((u8*)(block) - (byteOffset)))

void privClearBitFromBitFlag(u8* bit_flags, int bit);
void privSetBitFromBitFlag(u8* bit_flags, int bit);
u32 privGetBitFromBitFlag(const u8* bit_flags, int bit);
void privSetAlignInBitFlag(u8* bit_flags, int alignment);
void privClearBitFlag(u8* bit_flags);
u32 privGetLoadHighFromFlags(u32 flags);
int privGetAlignFromMwMemFlags(u32 flags);
void* privGetBlockFromUsedHdr(MwMemUsedHeader* header);
MwMemUsedHeader* privGetUsedHdrFromBlock(void* block);
u32 privGetStatSizeFromUsed(const MwMemUsedHeader* header);
u32 privGetUserSizeFromUsed(const MwMemUsedHeader* header);
void privSetBoundaryTags(MwMemUsedHeader* header);
int privIsAlignValid(int alignment);
void privUpdateStatsRemoveMemory(_mwMemHeap* heap, u32 size);
void privUpdateStatsAddMemory(_mwMemHeap* heap, u32 size);

#endif
