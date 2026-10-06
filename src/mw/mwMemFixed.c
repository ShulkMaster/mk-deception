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
    header = (MwMemUsedHeader*)header_end - 1;
    previous = header->previous;
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

void* fixedBlockHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request) {
    int requested_alignment;
    u32 block_prefix;
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
        block_prefix = heap->blockPrefixSize;
        header->prefixSize = 0;
        block = (u8*)header + block_prefix;
        block += sizeof(MwMemUsedHeader);
        header->allocationSize = allocation_size;
        header->heapIndex = request->heap->heapIndex;
        privClearBitFlag(&header->flags);
        privSetAlignInBitFlag(&header->flags, heap_alignment);
        privClearBitFromBitFlag(&header->flags, 5);
        header->alignmentPadding = block - (u8*)(header + 1);
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

/* TODO: [near miss] 96.48%; equivalent payload-add schedule and header/alignment coloring remain; stop after honest forms. */
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
                    fixedBlockAlignmentPadding(base_block_size, alignment_mask);
                arena_start =
                    heap->heapStart + heap->blockPrefixSize + sizeof(MwMemUsedHeader);
                heap->arenaAlignmentPadding =
                    fixedBlockAlignmentPadding((u32)arena_start, alignment_mask);
            }
            heap->usedList = 0;
            heap->freeList = 0;
            heap->freeTail = 0;
            block_count =
                (heap->heapEnd - heap->heapStart - heap->arenaAlignmentPadding) /
                (heap->blockSize + heap->blockPrefixSize + sizeof(MwMemUsedHeader));
            header = (MwMemUsedHeader*)(heap->heapStart + heap->arenaAlignmentPadding);
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

void fixedBlockHeapInitHeap(_mwMemHeap* heap, const MwMemFixedParams* params) {
    u32 threshold;

    threshold = params->sizeThreshold;
    heap->flags = params->flags;
    heap->blockSize = MW_MEM_ALIGN_UP_16(params->blockSize);
    if (params->blockSize > threshold) {
        heap->sizeThreshold = threshold;
    } else {
        heap->sizeThreshold = 0;
    }
    fixedBlockHeapResetHeap(heap, 0);
}

u32 mwMemFixedBlockHeapGetHeapSize(const MwMemFixedParams* params) {
    u32 base_block_size;
    u32 block_stride;
    u32 alignment;
    u32 alignment_mask;
    u32 heap_size;

    alignment = 1U << privGetAlignFromMwMemFlags(params->flags);
    alignment_mask = alignment - 1;
    base_block_size = MW_MEM_ALIGN_UP_16(params->blockSize) + sizeof(MwMemUsedHeader);
    block_stride = base_block_size;
    block_stride += fixedBlockAlignmentPadding(base_block_size, alignment_mask);
    heap_size = params->blockCount * block_stride;
    heap_size = alignment + heap_size;
    return heap_size + 0x70;
}
