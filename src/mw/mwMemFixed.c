#include "mw/mwMemFixed.h"

#include "mw/mwMemPriv.h"

static inline u32 fixedBlockAlignmentPadding(u32 size, u32 alignment_mask) {
    return ((size + alignment_mask) & ~alignment_mask) - size;
}

void fixedBlockHeapFreeBlock(_mwMemHeap* heap, void* block) {
    MwMemUsedHeader* header;
    u8* header_end;
    MwMemUsedHeader* previous;
    int alignment;

    header_end = (u8*)block - heap->blockPrefixSize;
    previous = ((MwMemUsedHeader*)header_end)[-1].previous;
    header = (MwMemUsedHeader*)(header_end - sizeof(MwMemUsedHeader));
    if (previous == 0 && header->next == 0) {
        heap->usedList = 0;
    } else if (previous == 0 && header->next != 0) {
        heap->usedList = header->next;
        heap->usedList->previous = 0;
    } else if (previous != 0 && header->next == 0) {
        previous->next = 0;
    } else {
        (previous->next = header->next)->previous = previous;
    }
    header->allocationSize = heap->blockSize + heap->blockPrefixSize;
    header->prefixSize = 0;
    header->heapIndex = 0;
    header->alignmentPadding = 0;
    privClearBitFlag(&header->flags);
    alignment = privGetAlignFromMwMemFlags(heap->flags);
    privSetAlignInBitFlag(&header->flags, alignment);
    privSetBitFromBitFlag(&header->flags, 5);
    if (heap->freeList == 0) {
        heap->freeList = header;
        header->previous = 0;
        header->next = 0;
    } else {
        header->next = heap->freeList;
        header->previous = 0;
        heap->freeList = header;
    }
}

/* Soft ceiling: the allocation algorithm and memory operations are identical;
 * one four-instruction block-address scheduling island remains. */
void* fixedBlockHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request) {
    int requested_alignment;
    u32 allocation_size;
    MwMemUsedHeader* header;
    u8* block;
    int heap_alignment;
    u32 requested_size;

    requested_size = size == 0 ? 0x10 : size;
    requested_alignment = privGetAlignFromMwMemFlags(flags);
    if (privIsAlignValid(requested_alignment) == 0) {
        requested_alignment = 4;
    }
    heap_alignment = privGetAlignFromMwMemFlags(heap->flags);
    if (requested_alignment > heap_alignment) {
        return 0;
    }
    requested_size = MW_MEM_ALIGN_UP_16(requested_size);
    if (requested_size > heap->blockSize) {
        return 0;
    }
    header = heap->freeList;
    allocation_size = heap->blockSize + heap->blockPrefixSize;
    if (header != 0) {
        heap->freeList = header->next;
        header->previous = 0;
        header->next = 0;
    }
    if (header == 0) {
        block = 0;
    } else {
        header->prefixSize = 0;
        block = (u8*)header + heap->blockPrefixSize + sizeof(MwMemUsedHeader);
        header->allocationSize = allocation_size;
        header->heapIndex = request->heap->heapIndex;
        privClearBitFlag(&header->flags);
        privSetAlignInBitFlag(&header->flags, heap_alignment);
        privClearBitFromBitFlag(&header->flags, 5);
        header->alignmentPadding = block - ((u8*)header + sizeof(MwMemUsedHeader));
        if (heap->usedList == 0) {
            heap->usedList = header;
            header->next = 0;
            header->previous = 0;
        } else {
            heap->usedList->previous = header;
            header->next = heap->usedList;
            header->previous = 0;
            heap->usedList = header;
        }
        request->allocationSize = header->allocationSize;
        request->alignmentPadding = header->alignmentPadding;
        request->allocationFlags = flags;
        request->allocationHeap = heap;
        request->prefixSize = 0;
        request->userSize = requested_size;
        block[-1] = request->alignmentPadding;
    }
    return block;
}

/* Soft ceiling: identical reset CFG and memory operations; header/alignment GPR
 * coloring and commutative address scheduling remain. */
void fixedBlockHeapResetHeap(_mwMemHeap* heap, int preserve_blocks) {
    u32 alignment_mask;
    u32 base_block_size;
    u32 block_count;
    u32 index;
    u8* arena_start;
    MwMemUsedHeader* header;
    int alignment;
    MwMemUsedHeader* used;

    if (heap != 0) {
        if (preserve_blocks == 0) {
            if (heap != 0) {
                alignment_mask = (1 << privGetAlignFromMwMemFlags(heap->flags)) - 1;
                base_block_size = heap->blockSize + sizeof(MwMemUsedHeader);
                heap->blockPrefixSize =
                    ((base_block_size + alignment_mask) & ~alignment_mask) -
                    base_block_size;
                arena_start =
                    heap->heapStart + heap->blockPrefixSize + sizeof(MwMemUsedHeader);
                heap->arenaAlignmentPadding =
                    ((u32)(arena_start + alignment_mask) & ~alignment_mask) -
                    (u32)arena_start;
            }
            heap->usedList = 0;
            heap->freeList = 0;
            heap->freeTail = 0;
            header = (MwMemUsedHeader*)(heap->heapStart + heap->arenaAlignmentPadding);
            block_count =
                (heap->heapEnd - heap->heapStart - heap->arenaAlignmentPadding) /
                (heap->blockSize + heap->blockPrefixSize + sizeof(MwMemUsedHeader));
            alignment = privGetAlignFromMwMemFlags(heap->flags);
            index = 0;
            while (index < block_count) {
                header->allocationSize = heap->blockSize + heap->blockPrefixSize;
                header->prefixSize = 0;
                header->heapIndex = 0;
                header->alignmentPadding = heap->blockPrefixSize;
                privClearBitFlag(&header->flags);
                privSetAlignInBitFlag(&header->flags, alignment);
                privSetBitFromBitFlag(&header->flags, 5);
                if (heap->freeList == 0) {
                    heap->freeList = header;
                    header->previous = 0;
                    header->next = 0;
                } else {
                    header->next = heap->freeList;
                    header->previous = 0;
                    heap->freeList = header;
                }
                index++;
                header = mwMemHeaderAt(header, heap->blockSize);
                header = mwMemHeaderAt(header, heap->blockPrefixSize);
                header++;
            }
        }
        heap->currentUsedSize = 0;
        heap->totalManagedSize = heap->heapEnd - heap->heapStart;
        heap->currentAllocationCount = 0;
        heap->currentFreeSize = heap->totalManagedSize;
        used = heap->usedList;
        while (used != 0) {
            privUpdateStatsAddMemory(heap, privGetStatSizeFromUsed(used));
            used = used->next;
        }
        heap->dirty = 1;
        heap->virtAllocCount = 0;
    }
}

/* Soft ceiling: identical loads, stores, compare, and call with only the
 * block-size and flags destination GPRs exchanged. */
void fixedBlockHeapInitHeap(_mwMemHeap* heap, const MwMemFixedParams* params) {
    u32 block_size;
    u32 threshold;

    block_size = params->blockSize;
    threshold = params->sizeThreshold;
    heap->flags = params->flags;
    heap->blockSize = MW_MEM_ALIGN_UP_16(block_size);
    if (block_size > threshold) {
        heap->field_0x60 = threshold;
    } else {
        heap->field_0x60 = 0;
    }
    fixedBlockHeapResetHeap(heap, 0);
}

/* Soft ceiling: identical heap-size arithmetic; MWCC fuses the padding sub/add
 * pair and reuses equivalent destination GPRs. */
u32 mwMemFixedBlockHeapGetHeapSize(const MwMemFixedParams* params) {
    u32 alignment;
    u32 alignment_mask;
    u32 base_block_size;
    u32 block_prefix_size;
    u32 block_stride;
    u32 heap_size;

    alignment = 1 << privGetAlignFromMwMemFlags(params->flags);
    alignment_mask = alignment - 1;
    base_block_size = MW_MEM_ALIGN_UP_16(params->blockSize) + sizeof(MwMemUsedHeader);
    block_prefix_size = fixedBlockAlignmentPadding(base_block_size, alignment_mask);
    block_stride = base_block_size + block_prefix_size;
    heap_size = params->blockCount * block_stride;
    heap_size += alignment;
    return heap_size + 0x70;
}
