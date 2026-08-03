#include "mw/mwMemHdrless.h"

#include "mw/mwMemPriv.h"

#define ALIGN_UP_16(value) (((value) + 0xF) & ~0xFU)

/* Soft ceiling: ~99.13% -- initial leaf-register coloring only. */
void hdrlessHeapFreeBlock(_mwMemHeap* heap, void* block) {
    MwMemUsedHeader* header;
    u32 block_prefix;
    u32 block_size;
    u32 header_size;
    int alignment;
    MwMemUsedHeader* free_head;

    block_prefix = heap->blockPrefixSize;
    block_size = heap->blockSize;
    header_size = block_prefix - sizeof(MwMemUsedHeader);
    header = mwMemHeaderBefore(block, block_prefix);
    header_size += block_size;
    header->allocationSize = header_size;
    header->prefixSize = 0;
    header->heapIndex = 0;
    header->alignmentPadding = 0;
    privClearBitFlag(&header->flags);
    alignment = privGetAlignFromMwMemFlags(heap->flags);
    privSetAlignInBitFlag(&header->flags, alignment);
    privSetBitFromBitFlag(&header->flags, 5);
    free_head = heap->freeList;
    if (free_head == 0) {
        heap->freeList = header;
        header->next = 0;
        header->previous = (MwMemUsedHeader*)~(u32)header->next;
    } else {
        header->next = free_head;
        heap->freeList = header;
        header->previous = (MwMemUsedHeader*)~(u32)header->next;
    }
}

/* Soft ceiling: ~91.27% -- allocation-local coloring and store scheduling. */
void* hdrlessHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request) {
    int requested_alignment;
    u32 aligned_size;
    u32 block_size;
    u32 allocation_size;
    MwMemUsedHeader* header;

    if (size == 0) {
        size = 0x10;
    }
    requested_alignment = privGetAlignFromMwMemFlags(flags);
    if (privIsAlignValid(requested_alignment) == 0) {
        requested_alignment = 4;
    }
    if (requested_alignment > privGetAlignFromMwMemFlags(heap->flags)) {
        return 0;
    }
    aligned_size = ALIGN_UP_16(size);
    block_size = heap->blockSize;
    if (aligned_size > block_size) {
        return 0;
    }
    allocation_size = block_size + heap->blockPrefixSize;
    header = heap->freeList;
    if (header != 0) {
        heap->freeList = header->next;
        header->previous = 0;
        header->next = 0;
    }
    if (header == 0) {
        return 0;
    }
    request->allocationSize = allocation_size;
    request->alignmentPadding = heap->blockPrefixSize;
    request->allocationFlags = flags;
    request->originHeap = heap;
    request->prefixSize = 0;
    request->userSize = aligned_size;
    return mwMemByteAddress(header, heap->blockPrefixSize);
}

/* Soft ceiling: ~87.45% -- loop/local coloring around free-list construction. */
void hdrlessHeapResetHeap(_mwMemHeap* heap) {
    u32 alignment_mask;
    u32 alignment_inverse;
    u32 block_stride;
    u32 block_count;
    u32 index;
    u32 header_size;
    u8* arena_start;
    MwMemUsedHeader* header;

    if (heap != 0) {
        if (heap != 0) {
            alignment_mask = (1 << privGetAlignFromMwMemFlags(heap->flags)) - 1;
            alignment_inverse = ~alignment_mask;
            heap->blockPrefixSize =
                ((heap->blockSize + alignment_mask) & alignment_inverse) - heap->blockSize;
            arena_start = heap->heapStart + heap->blockPrefixSize;
            heap->arenaAlignmentPadding =
                ((u32)(arena_start + alignment_mask) & alignment_inverse) - (u32)arena_start;
        }
        index = 0;
        heap->usedList = 0;
        heap->freeList = 0;
        heap->freeTail = 0;
        arena_start = heap->heapStart;
        header = mwMemHeaderAt(arena_start, heap->arenaAlignmentPadding);
        block_stride = heap->blockSize + heap->blockPrefixSize;
        header->next = 0;
        block_count = (heap->heapEnd - arena_start - heap->arenaAlignmentPadding) / block_stride;
        header->previous = 0;
        header_size = block_stride - sizeof(MwMemUsedHeader);
        while (index < block_count) {
            header->allocationSize = header_size;
            header->prefixSize = 0;
            header->heapIndex = 0;
            header->alignmentPadding = 0;
            privClearBitFlag(&header->flags);
            privSetAlignInBitFlag(&header->flags, 4);
            privSetAlignInBitFlag(&header->flags, 0);
            if (heap->freeList == 0) {
                heap->freeList = header;
                header->next = 0;
                header->previous = (MwMemUsedHeader*)~(u32)header->next;
            } else {
                header->next = heap->freeList;
                heap->freeList = header;
                header->previous = (MwMemUsedHeader*)~(u32)header->next;
            }
            header = mwMemHeaderAt(header, block_stride);
            index++;
        }
        heap->currentUsedSize = 0;
        heap->totalManagedSize = heap->heapEnd - heap->heapStart;
        heap->currentAllocationCount = 0;
        heap->currentFreeSize = heap->totalManagedSize;
        heap->dirty = 1;
        heap->virtAllocCount = 0;
    }
}

void hdrlessHeapInitHeap(_mwMemHeap* heap, const MwMemHeaderlessParams* params) {
    heap->flags = params->flags;
    heap->blockSize = ALIGN_UP_16(params->blockSize);
    heap->field_0x60 = 0;
    hdrlessHeapResetHeap(heap);
}

/* Soft ceiling: ~97.71% -- equivalent arithmetic GPR assignment. */
u32 mwMemHeaderlessFixedBlockGetHeapSize(const MwMemHeaderlessParams* params) {
    u32 alignment;
    u32 alignment_mask;
    u32 block_size;

    alignment = 1 << privGetAlignFromMwMemFlags(params->flags);
    alignment_mask = alignment - 1;
    block_size =
        (ALIGN_UP_16(params->blockSize) + alignment_mask) & ~alignment_mask;
    block_size *= params->blockCount;
    block_size += alignment;
    return block_size + 0x70;
}
