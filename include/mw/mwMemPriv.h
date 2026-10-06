#ifndef MW_MWMEMPRIV_H
#define MW_MWMEMPRIV_H

#include "mw/mwMem.h"

enum MwMemAlignment {
    MW_MEM_ALIGN_NONE = 0,
    MW_MEM_ALIGN_16 = 4,
    MW_MEM_ALIGN_32 = 5,
    MW_MEM_ALIGN_64 = 6,
    MW_MEM_ALIGN_128 = 7,
    MW_MEM_ALIGN_256 = 8,
    MW_MEM_ALIGN_FORCE_32BIT = 0x7FFFFFFF
};

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
int privGetLoadHighFromFlags(u32 flags);
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
