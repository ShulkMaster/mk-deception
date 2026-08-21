#include "mw/mwMemFixed.h"

#include "mw/mwMemPriv.h"

#define ALIGN_UP_16(value) (((value) + 0xF) & ~0xFU)

/* Soft ceiling: ~89.58% -- unlink/reinsert scheduling and leaf coloring. */
void fixedBlockHeapFreeBlock(_mwMemHeap* heap, void* block) {
    MwMemUsedHeader* header;
    MwMemUsedHeader* previous;
    MwMemUsedHeader* next;
    int alignment;

    header = mwMemHeaderBefore(block,
                              heap->blockPrefixSize + sizeof(MwMemUsedHeader));
    previous = header->previous;
    next = header->next;
    if (previous == 0 && next == 0) {
        heap->usedList = 0;
    } else if (previous == 0 && next != 0) {
        heap->usedList = next;
        heap->usedList->previous = 0;
    } else if (previous != 0 && next == 0) {
        previous->next = 0;
    } else {
        previous->next = next;
        next->previous = previous;
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

/* Soft ceiling: ~90.52% -- allocation-local register assignment. */
void* fixedBlockHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request) {
    u32 requested_size;
    int requested_alignment;
    int heap_alignment;
    u32 aligned_size;
    u32 block_size;
    MwMemUsedHeader* header;
    MwMemUsedHeader* used_head;
    u8* block;

    requested_size = size == 0 ? 0x10 : size;
    requested_alignment = privGetAlignFromMwMemFlags(flags);
    if (privIsAlignValid(requested_alignment) == 0) {
        requested_alignment = 4;
    }
    heap_alignment = privGetAlignFromMwMemFlags(heap->flags);
    if (requested_alignment > heap_alignment) {
        return 0;
    }
    aligned_size = ALIGN_UP_16(requested_size);
    block_size = heap->blockSize;
    if (aligned_size > block_size) {
        return 0;
    }
    header = heap->freeList;
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
        header->allocationSize = block_size + heap->blockPrefixSize;
        header->heapIndex = request->heap->heapIndex;
        privClearBitFlag(&header->flags);
        privSetAlignInBitFlag(&header->flags, heap_alignment);
        privClearBitFromBitFlag(&header->flags, 5);
        header->alignmentPadding = block - ((u8*)header + sizeof(MwMemUsedHeader));
        used_head = heap->usedList;
        if (used_head == 0) {
            heap->usedList = header;
            header->next = 0;
            header->previous = 0;
        } else {
            used_head->previous = header;
            header->next = used_head;
            header->previous = 0;
            heap->usedList = header;
        }
        request->allocationSize = header->allocationSize;
        request->alignmentPadding = header->alignmentPadding;
        request->allocationFlags = flags;
        request->allocationHeap = heap;
        request->prefixSize = 0;
        request->userSize = aligned_size;
        block[-1] = request->alignmentPadding;
    }
    return block;
}

/* Soft ceiling: ~84.28% -- reset-loop register assignment and scheduling. */
void fixedBlockHeapResetHeap(_mwMemHeap* heap, int preserve_blocks) {
    u32 alignment_mask;
    u32 alignment_inverse;
    u32 block_stride;
    u32 block_count;
    u32 index;
    int alignment;
    u8* arena_start;
    MwMemUsedHeader* header;
    MwMemUsedHeader* used;

    if (heap != 0) {
        if (preserve_blocks == 0) {
            if (heap != 0) {
                alignment_mask = (1 << privGetAlignFromMwMemFlags(heap->flags)) - 1;
                alignment_inverse = ~alignment_mask;
                block_stride = heap->blockSize + sizeof(MwMemUsedHeader);
                heap->blockPrefixSize =
                    ((block_stride + alignment_mask) & alignment_inverse) - block_stride;
                arena_start = heap->heapStart + heap->blockPrefixSize + sizeof(MwMemUsedHeader);
                heap->arenaAlignmentPadding =
                    ((u32)(arena_start + alignment_mask) & alignment_inverse) - (u32)arena_start;
            }
            heap->usedList = 0;
            heap->freeList = 0;
            heap->freeTail = 0;
            block_stride = heap->blockSize + heap->blockPrefixSize + sizeof(MwMemUsedHeader);
            arena_start = heap->heapStart + heap->arenaAlignmentPadding;
            block_count = (heap->heapEnd - heap->heapStart - heap->arenaAlignmentPadding) / block_stride;
            alignment = privGetAlignFromMwMemFlags(heap->flags);
            index = 0;
            header = (MwMemUsedHeader*)arena_start;
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
                header = mwMemHeaderAt(header, block_stride);
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

/* Soft ceiling: ~98.86% -- two equivalent input-register homes are swapped. */
void fixedBlockHeapInitHeap(_mwMemHeap* heap, const MwMemFixedParams* params) {
    u32 block_size;
    u32 flags;
    u32 threshold;

    block_size = params->blockSize;
    flags = params->flags;
    threshold = params->sizeThreshold;
    heap->flags = flags;
    heap->blockSize = ALIGN_UP_16(block_size);
    if (block_size > threshold) {
        heap->field_0x60 = threshold;
    } else {
        heap->field_0x60 = 0;
    }
    fixedBlockHeapResetHeap(heap, 0);
}

/* Soft ceiling: ~67.59% -- MWCC reassociates the equivalent padding expression. */
u32 mwMemFixedBlockHeapGetHeapSize(const MwMemFixedParams* params) {
    u32 alignment;
    u32 alignment_mask;
    u32 base_block_size;

    alignment = 1 << privGetAlignFromMwMemFlags(params->flags);
    alignment_mask = alignment - 1;
    base_block_size = ALIGN_UP_16(params->blockSize) + sizeof(MwMemUsedHeader);
    return alignment +
           params->blockCount *
               (base_block_size +
                (((base_block_size + alignment_mask) & ~alignment_mask) - base_block_size)) +
           0x70;
}
