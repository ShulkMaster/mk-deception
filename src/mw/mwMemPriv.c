#include "mw/mwMemPriv.h"

typedef union MwMemBlockFlags {
    u8 value;
    struct {
        u8 upper : 4;
        u8 alignment : 4;
    } bits;
} MwMemBlockFlags;

void privClearBitFromBitFlag(u8* bit_flags, int bit) {
    *bit_flags &= (u8)~(u8)(1 << bit);
}

void privSetBitFromBitFlag(u8* bit_flags, int bit) {
    *bit_flags |= (u8)(1 << bit);
}

int privGetBitFromBitFlag(const u8* bit_flags, int bit) {
    return (*bit_flags >> bit) & 1;
}

void privSetAlignInBitFlag(u8* bit_flags, int alignment) {
    *bit_flags &= 0xF0;
    *bit_flags |= alignment & 0xF;
}

void privClearBitFlag(u8* bit_flags) {
    *bit_flags = 0;
}

int privGetLoadHighFromFlags(u32 flags) {
    return (flags >> 5) & 1;
}

/* Soft ceiling: ~96.59% -- one redundant default-range branch differs. */
int privGetAlignFromMwMemFlags(u32 flags) {
    int alignment_flags;

    alignment_flags = flags & ~0x70;
    switch (alignment_flags) {
    case 1:
    case 4:
        return 4;
    case 5:
        return 5;
    case 6:
        return 6;
    case 7:
        return 7;
    case 2:
    case 8:
        return 8;
    case 0x80:
        return 0;
    case 0:
    case 3:
    case 0x10:
    case 0x20:
    case 0x40:
    default:
        return 4;
    }
}

/* Soft ceiling: ~77.50% -- equivalent leaf-register coloring remains. */
void* privGetBlockFromUsedHdr(MwMemUsedHeader* header) {
    MwMemBlockFlags flags;
    u32 alignment_mask;

    flags.value = header->flags;
    alignment_mask = (1 << flags.bits.alignment) - 1;
    return (void*)(((u32)header + sizeof(MwMemUsedHeader) + alignment_mask) & ~alignment_mask);
}

MwMemUsedHeader* privGetUsedHdrFromBlock(void* block) {
    u8* bytes = block;
    return (MwMemUsedHeader*)(bytes - (bytes[-1] + sizeof(MwMemUsedHeader)));
}

u32 privGetStatSizeFromUsed(const MwMemUsedHeader* header) {
    if (header != 0) {
        return header->allocationSize + sizeof(MwMemUsedHeader);
    }
    return 0;
}

/* Soft ceiling: ~94.50% -- return-expression GPR coloring only. */
int privGetUserSizeFromUsed(const MwMemUsedHeader* header) {
    int size = 0;
    if (header != 0) {
        size = header->allocationSize - header->prefixSize - header->alignmentPadding;
    }
    return size;
}

void privSetBoundaryTags(MwMemUsedHeader* header) {
    MwMemUsedHeader** footer;

    footer = (MwMemUsedHeader**)((u8*)header + header->allocationSize + 0xC);
    *footer = header;
    header->flags |= 0x20;
}

int privIsAlignValid(int alignment) {
    int valid = 1;
    if (alignment < 4) {
        valid = 0;
    }
    if (alignment > 8) {
        valid = 0;
    }
    if (alignment >= 0xFF) {
        valid = 0;
    }
    if (alignment == 0) {
        valid = 1;
    }
    return valid;
}

void privUpdateStatsRemoveMemory(_mwMemHeap* heap, u32 size) {
    u32 current;

    heap->currentUsedSize -= size;
    current = heap->currentUsedSize;
    if (current > heap->peakUsedSize) {
        heap->peakUsedSize = current;
    }
    heap->currentAllocationCount--;
    current = heap->currentAllocationCount;
    if (current > heap->peakAllocationCount) {
        heap->peakAllocationCount = current;
    }
    heap->currentFreeSize += size;
}

void privUpdateStatsAddMemory(_mwMemHeap* heap, u32 size) {
    u32 current;

    heap->currentUsedSize += size;
    current = heap->currentUsedSize;
    if (current > heap->peakUsedSize) {
        heap->peakUsedSize = current;
    }
    heap->currentAllocationCount++;
    current = heap->currentAllocationCount;
    if (current > heap->peakAllocationCount) {
        heap->peakAllocationCount = current;
    }
    heap->currentFreeSize -= size;
    heap->dirty = 0;
}
