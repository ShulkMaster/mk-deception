#include "mw/mwMemNormal.h"

#include "mw/mwMemPriv.h"

#define REMOVE_FREE_BLOCK(heap, block)                                                \
    do {                                                                              \
        _mwMemHeap* remove_heap = (heap);                                              \
        MwMemUsedHeader* remove_block = (block);                                      \
        if (remove_block != 0 && remove_heap->freeList != 0) {                        \
            MwMemUsedHeader* remove_previous = remove_block->previous;                \
            MwMemUsedHeader* remove_next = remove_block->next;                        \
            if (remove_previous == 0) {                                               \
                if (remove_next == 0) {                                               \
                    remove_heap->freeList = 0;                                        \
                    remove_heap->freeTail = 0;                                        \
                } else {                                                              \
                    remove_heap->freeList = remove_next;                              \
                    remove_next->previous = 0;                                        \
                }                                                                     \
            } else if (remove_next == 0) {                                            \
                remove_previous->next = 0;                                            \
                remove_heap->freeTail = remove_previous;                              \
            } else {                                                                  \
                remove_previous->next = remove_next;                                  \
                remove_next->previous = remove_previous;                              \
            }                                                                         \
        }                                                                             \
    } while (0)

static void privReturnUsedBlockToFreeList(_mwMemHeap* heap, MwMemUsedHeader* block);

/* Soft ceiling: exact-size coalescing CFG; residue is GPR coloring and three
 * localized unlink scheduling choices. */
static MwMemUsedHeader* privCoalesceFreeBlocksBoundaryTags(_mwMemHeap* heap,
                                                            MwMemUsedHeader* block) {
    u8 flags;
    u8 next_flags;
    u32 previous_is_free;
    u32 next_is_free = 0;
    MwMemUsedHeader* result = block;
    MwMemUsedHeader* next_block;
    MwMemUsedHeader* previous_block;

    flags = block->flags;
    previous_is_free = privGetBitFromBitFlag(&flags, 4);
    next_block = (MwMemUsedHeader*)((u8*)block + block->allocationSize + sizeof(MwMemUsedHeader));
    if ((u8*)next_block != heap->heapEnd) {
        next_flags = next_block->flags;
        next_is_free = privGetBitFromBitFlag(&next_flags, 5);
    }
    if (block->next != 0 || block->previous != 0) {
        if (block->next != 0 && block->previous == 0) {
            if ((u8*)block + block->allocationSize + sizeof(MwMemUsedHeader) == (u8*)block->next) {
                block->allocationSize += block->next->allocationSize + sizeof(MwMemUsedHeader);
                REMOVE_FREE_BLOCK(heap, block->next);
            }
        } else if (block->next == 0 && block->previous != 0) {
            previous_block = block->previous;
            if ((u8*)previous_block + previous_block->allocationSize + sizeof(MwMemUsedHeader) ==
                (u8*)block) {
                previous_block->allocationSize += block->allocationSize + sizeof(MwMemUsedHeader);
                REMOVE_FREE_BLOCK(heap, block);
                result = previous_block;
            }
        } else {
            if (next_is_free == 1) {
                next_block = (MwMemUsedHeader*)((u8*)block + block->allocationSize +
                                                sizeof(MwMemUsedHeader));
                block->allocationSize += next_block->allocationSize + sizeof(MwMemUsedHeader);
                REMOVE_FREE_BLOCK(heap, next_block);
            }
            if (previous_is_free == 1) {
                previous_block = block->previous;
                previous_block->allocationSize += block->allocationSize + sizeof(MwMemUsedHeader);
                REMOVE_FREE_BLOCK(heap, block);
                result = previous_block;
            }
        }
    }
    return result;
}

/* Soft ceiling: exact-size ownership traversal and unlink CFG; residue is GPR
 * coloring plus a localized branch/load schedule. */
static void privFreeMemFromUsed(MwMemUsedHeader* block) {
    _mwMemHeap* heap;
    _mwMemHeap* fallback;
    MwMemUsedHeader* merged;
    MwMemUsedHeader* next_block;

    if (block == 0) {
        return;
    }
    if (HeapList == 0) {
        heap = 0;
    } else {
        heap = SystemHeap;
        while (heap->hierNext != 0) {
            if ((u8*)block >= (u8*)heap && (u8*)block < heap->heapEnd) {
                break;
            }
            heap = heap->hierNext;
        }

        fallback = heap;
        while (heap != 0) {
            if ((u8*)block >= heap->heapStart && (u8*)block < heap->heapEnd) {
                fallback = heap;
                if (heap->hierFirstChild == 0) {
                    break;
                }
                heap = heap->hierFirstChild;
            } else {
                heap = heap->hierNext;
                if (heap == 0) {
                    heap = fallback;
                    break;
                }
            }
        }
    }
    if (heap == 0) {
        return;
    }
    if (block->previous == 0 && block->next == 0) {
        heap->usedList = 0;
    } else if (block->previous == 0 && block->next != 0) {
        heap->usedList = block->next;
        heap->usedList->previous = 0;
    } else if (block->previous != 0 && block->next == 0) {
        block->previous->next = 0;
    } else {
        block->previous->next = block->next;
        block->next->previous = block->previous;
    }
    privReturnUsedBlockToFreeList(heap, block);
    privSetBitFromBitFlag(&block->flags, 5);
    merged = privCoalesceFreeBlocksBoundaryTags(heap, block);
    privSetBoundaryTags(merged);
    next_block = (MwMemUsedHeader*)((u8*)merged + merged->allocationSize + sizeof(MwMemUsedHeader));
    if ((u8*)next_block != heap->heapEnd) {
        privSetBitFromBitFlag(&next_block->flags, 4);
    }
}

/* Soft ceiling: reachable list insertion differs only by li-zero versus mr from
 * an already-zero local; retail also retains an unreachable loop tail. */
static void privReturnUsedBlockToFreeList(_mwMemHeap* heap, MwMemUsedHeader* block) {
    MwMemUsedHeader* current = heap->freeList;
    MwMemUsedHeader* next;
    MwMemUsedHeader* previous;

    next = previous = 0;

    if (current == 0) {
        heap->freeList = block;
        heap->freeTail = block;
        block->next = previous;
        block->previous = previous;
    } else {
        while (current != 0) {
            if (block > current) {
                previous = current;
                current = current->next;
            } else {
                next = current;
                break;
            }
        }
    }
    if (previous != 0 || next != 0) {
        if (block != 0) {
            if (previous == 0) {
                if (next == 0) {
                    heap->freeList = block;
                    heap->freeTail = block;
                } else {
                    block->next = next;
                    block->previous = 0;
                    next->previous = block;
                    heap->freeList = block;
                }
            } else if (next == 0) {
                block->next = 0;
                block->previous = previous;
                previous->next = block;
                heap->freeTail = block;
            } else {
                block->next = next;
                block->previous = previous;
                previous->next = block;
                next->previous = block;
            }
        }
        if (block > heap->freeTail) {
            heap->freeTail = block;
        }
    }
}

void normHeapFreeMemFromBlock(void* block) {
    privFreeMemFromUsed(privGetUsedHdrFromBlock(block));
}

/* Soft ceiling: best/first-fit algorithms, boundary tags, and list updates
 * agree; residue is allocator-wide GPR coloring and localized address/add
 * scheduling. */
void* normHeapMallocMem(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request) {
    u32 requested_size = size == 0 ? 0x10 : size;
    int alignment = privGetAlignFromMwMemFlags(flags);
    u32 user_size;
    u32 needed_size;
    MwMemUsedHeader* candidate = 0;
    MwMemUsedHeader* current;
    MwMemUsedHeader* used;
    MwMemUsedHeader* remainder;
    MwMemUsedHeader* next_block;
    u32 used_size;
    u32 candidate_size;
    int load_high = 0;
    u8* block;
    u32 alignment_mask;

    if (privIsAlignValid(alignment) == 0) {
        alignment = 4;
    }
    user_size = MW_MEM_ALIGN_UP_16(requested_size);
    needed_size = (alignment == 4 || alignment == 0)
                      ? user_size
                      : MW_MEM_ALIGN_UP_16(requested_size + (1 << alignment));
    if (heap->freeList == 0) {
        return 0;
    }
    if (heap->strategy == 3) {
        u32 best_size = heap->arenaSize;
        current = heap->freeTail;
        while (current != 0) {
            if (current->allocationSize >= needed_size) {
                if (current->allocationSize == needed_size) {
                    candidate = current;
                    break;
                } else if (current->allocationSize < best_size) {
                    best_size = current->allocationSize;
                    candidate = current;
                }
            }
            current = current->previous;
        }
    } else if (privGetLoadHighFromFlags(flags) != 0) {
        current = heap->freeList;
        while (current != 0) {
            if (current->allocationSize >= needed_size) {
                candidate = current;
                break;
            } else {
                current = current->next;
            }
        }
        load_high = 1;
    } else {
        current = heap->freeTail;
        while (current != 0) {
            if (current->allocationSize >= needed_size) {
                candidate = current;
                break;
            } else {
                current = current->previous;
            }
        }
    }
    if (candidate == 0) {
        return 0;
    }
    candidate_size = candidate->allocationSize;
    if (candidate_size <= needed_size + 0x20) {
        REMOVE_FREE_BLOCK(heap, candidate);
        used_size = candidate_size;
        used = candidate;
        privClearBitFlag(&used->flags);
        privClearBitFromBitFlag(&used->flags, 4);
        privClearBitFromBitFlag(&used->flags, 5);
        next_block = mwMemHeaderAt(used, used_size + sizeof(MwMemUsedHeader));
        if ((u8*)next_block != heap->heapEnd) {
            privClearBitFromBitFlag(&next_block->flags, 4);
        }
    } else if (load_high != 0) {
        used = candidate;
        used_size = needed_size;
        remainder = (MwMemUsedHeader*)((u8*)candidate + needed_size + sizeof(MwMemUsedHeader));
        if (candidate->previous == 0 && candidate->next == 0) {
            remainder->next = 0;
            remainder->previous = 0;
            heap->freeList = remainder;
            heap->freeTail = remainder;
        } else if (candidate->previous == 0 && candidate->next != 0) {
            remainder->next = candidate->next;
            remainder->previous = 0;
            heap->freeList = remainder;
            remainder->next->previous = remainder;
        } else if (candidate->previous != 0 && candidate->next == 0) {
            remainder->previous = candidate->previous;
            remainder->next = 0;
            heap->freeTail = remainder;
            remainder->previous->next = remainder;
        } else {
            remainder->previous = candidate->previous;
            remainder->next = candidate->next;
            remainder->previous->next = remainder;
            remainder->next->previous = remainder;
        }
        remainder->allocationSize = candidate_size - (needed_size + sizeof(MwMemUsedHeader));
        remainder->prefixSize = 0;
        remainder->heapIndex = candidate->heapIndex;
        remainder->flags = candidate->flags;
        remainder->alignmentPadding = candidate->alignmentPadding;
        privClearBitFlag(&remainder->flags);
        privClearBitFromBitFlag(&remainder->flags, 4);
        privSetBoundaryTags(remainder);
        privClearBitFlag(&used->flags);
        privClearBitFromBitFlag(&used->flags, 4);
        privClearBitFromBitFlag(&used->flags, 5);
    } else {
        used_size = needed_size;
        candidate->allocationSize = candidate_size - (needed_size + sizeof(MwMemUsedHeader));
        used = (MwMemUsedHeader*)((u8*)candidate + candidate_size - needed_size);
        privClearBitFromBitFlag(&candidate->flags, 4);
        privSetBoundaryTags(candidate);
        privClearBitFlag(&used->flags);
        privSetBitFromBitFlag(&used->flags, 4);
        privClearBitFromBitFlag(&used->flags, 5);
        next_block = mwMemHeaderAt(used, needed_size + sizeof(MwMemUsedHeader));
        if ((u8*)next_block != heap->heapEnd) {
            privClearBitFromBitFlag(&next_block->flags, 4);
        }
    }
    used->allocationSize = used_size;
    used->prefixSize = used_size - needed_size;
    privSetAlignInBitFlag(&used->flags, alignment);
    used->heapIndex = request->heap->heapIndex;
    if (heap->usedList == 0) {
        used->next = 0;
        used->previous = 0;
        heap->usedList = used;
    } else {
        used->next = heap->usedList;
        used->previous = 0;
        heap->usedList->previous = used;
        heap->usedList = used;
    }
    block = (u8*)used + sizeof(MwMemUsedHeader);
    alignment_mask = (1 << alignment) - 1;
    block = (u8*)(((u32)block + alignment_mask) & ~alignment_mask);
    used->alignmentPadding = block - ((u8*)used + sizeof(MwMemUsedHeader));
    request->allocationSize = used->allocationSize;
    request->alignmentPadding = used->alignmentPadding;
    request->allocationFlags = flags;
    request->allocationHeap = heap;
    request->prefixSize = used->prefixSize;
    request->userSize = user_size;
    block[-1] = used->alignmentPadding;
    return block;
}

void normHeapResetHeap(_mwMemHeap* heap, int preserve_blocks) {
    MwMemUsedHeader* block;
    MwMemUsedHeader* used;

    if (heap != 0) {
        if (preserve_blocks == 0) {
            heap->usedList = 0;
            heap->freeList = (MwMemUsedHeader*)heap->heapStart;
            heap->freeTail = (MwMemUsedHeader*)heap->heapStart;
            block = heap->freeList;
            block->previous = 0;
            block->next = 0;
            block->allocationSize = heap->heapEnd - (heap->heapStart + sizeof(MwMemUsedHeader));
            block->prefixSize = 0;
            block->heapIndex = 0;
            privClearBitFlag(&block->flags);
            privSetAlignInBitFlag(&block->flags, 4);
            privClearBitFromBitFlag(&block->flags, 4);
            privSetBoundaryTags(block);
        }
        heap->arenaAlignmentPadding = 0;
        heap->blockPrefixSize = 0;
        heap->flags = 0;
        heap->field_0x60 = 0;
        heap->blockSize = 0;
        heap->currentUsedSize = 0;
        heap->totalManagedSize = heap->heapEnd - heap->heapStart;
        heap->currentAllocationCount = 0;
        heap->currentFreeSize = heap->heapEnd - heap->heapStart;
        used = heap->usedList;
        while (used != 0) {
            privUpdateStatsAddMemory(heap, privGetStatSizeFromUsed(used));
            used = used->next;
        }
        heap->dirty = 1;
        heap->virtAllocCount = 0;
    }
}

void normHeapInitHeap(_mwMemHeap* heap) {
    heap->flags = 0;
    heap->field_0x60 = 0;
    heap->blockSize = 0;
    normHeapResetHeap(heap, 0);
}
