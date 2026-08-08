#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

static RxHeapFreeBlock* HeapFreeBlocksNewEntry(RxHeap* heap)
{
    RxHeapFreeBlock* freeBlocks = heap->freeBlocks;
    RwUInt32 used = heap->freeBlocksUsed;

    if (heap->freeBlocksAllocated <= used) {
        freeBlocks = RwEngineInstance->fpRealloc(
            heap->freeBlocks,
            (heap->freeBlocksAllocated += 32) * sizeof(*freeBlocks),
            0x1030409);
        if (freeBlocks == NULL) {
            RwError error;

            error.pluginID = 1;
            error.errorCode = _rwerror(
                0x80000013,
                heap->freeBlocksAllocated * sizeof(*freeBlocks));
            RwErrorSet(&error);
            heap->freeBlocksAllocated -= 32;
        } else {
            if (freeBlocks != heap->freeBlocks && used != 0) {
                RxHeapFreeBlock* entry = freeBlocks;
                RwUInt32 remaining;

                do {
                    entry->block->freeEntry = entry;
                    ++entry;
                    remaining = --used;
                }
                while (remaining != 0);
            }
            heap->freeBlocks = freeBlocks;
        }
    }

    if (freeBlocks != NULL) {
        freeBlocks += heap->freeBlocksUsed++;
    }
    return freeBlocks;
}

static void HeapFreeBlocksDeleteEntry(RxHeap* heap, RxHeapFreeBlock* entry)
{
    if (&heap->freeBlocks[heap->freeBlocksUsed - 1] != entry) {
        *entry = heap->freeBlocks[heap->freeBlocksUsed - 1];
        entry->block->freeEntry = entry;
    }
    --heap->freeBlocksUsed;
}

static RxHeapSuperBlock* HeapSuperBlockCreate(RwUInt32 size)
{
    RxHeapSuperBlock* superBlock =
        RwEngineInstance->fpMalloc(size + 0x8B, 0x1040409);

    if (superBlock != NULL) {
        superBlock->start = (RxHeapBlock*)
            (((unsigned long)superBlock + 0x8B) & ~(unsigned long)0x7F);
        superBlock->size = size;
        superBlock->next = NULL;
    }
    return superBlock;
}

static void HeapSuperBlockDestroy(RxHeapSuperBlock* superBlock)
{
    if (superBlock != NULL) {
        RwEngineInstance->fpFree(superBlock);
    }
}

static RwBool HeapSuperBlockReset(RxHeapSuperBlock* superBlock,
                                  RxHeapSuperBlock* previous, RxHeap* heap)
{
    RxHeapFreeBlock* freeEntry = HeapFreeBlocksNewEntry(heap);

    if (freeEntry != NULL) {
        RxHeapBlock* first = superBlock->start;
        RxHeapBlock* freeBlock = first + 1;
        RxHeapBlock* last = (RxHeapBlock*)((RwUInt8*)superBlock->start +
                                          superBlock->size - sizeof(*last));

        first->prev = NULL;
        first->next = NULL;
        first->size = 0;
        first->freeEntry = NULL;
        *last = *first;

        first->next = freeBlock;
        freeBlock->prev = first;
        freeBlock->next = last;
        last->prev = freeBlock;
        freeBlock->size = (RwUInt8*)last -
                          ((RwUInt8*)freeBlock + sizeof(*freeBlock));
        freeBlock->freeEntry = freeEntry;
        freeEntry->block = freeBlock;
        freeEntry->size = freeBlock->size;

        if (previous != NULL) {
            RxHeapBlock* previousLast =
                (RxHeapBlock*)((RwUInt8*)previous->start + previous->size -
                               sizeof(*previousLast));
            previousLast->next = first;
            first->prev = previousLast;
        }
        return TRUE;
    }
    return FALSE;
}

void RxHeapFree(RxHeap* heap, void* memory)
{
    RxHeapBlock* block = (RxHeapBlock*)memory - 1;
    RwBool previousIsFree =
        block->prev != NULL && block->prev->freeEntry != NULL;
    RwBool nextIsFree =
        block->next != NULL && block->next->freeEntry != NULL;

    if (previousIsFree) {
        if (nextIsFree) {
            HeapFreeBlocksDeleteEntry(heap, block->next->freeEntry);
            block->prev->size += block->size + block->next->size +
                                 2 * sizeof(*block);
            block->prev->freeEntry->size = block->prev->size;
            block->prev->next = block->next->next;
            if (block->next->next != NULL) {
                block->next->next->prev = block->prev;
            }
        } else {
            block->prev->size += block->size + sizeof(*block);
            block->prev->freeEntry->size = block->prev->size;
            block->prev->next = block->next;
            if (block->next != NULL) {
                block->next->prev = block->prev;
            }
        }
    } else if (nextIsFree) {
        block->size += block->next->size + sizeof(*block);
        block->freeEntry = block->next->freeEntry;
        block->next->freeEntry->block = block;
        block->next->freeEntry->size = block->size;
        block->next = block->next->next;
        if (block->next != NULL) {
            block->next->prev = block;
        }
    } else {
        RxHeapFreeBlock* freeEntry = HeapFreeBlocksNewEntry(heap);

        if (freeEntry != NULL) {
            freeEntry->block = block;
            freeEntry->size = block->size;
            block->freeEntry = freeEntry;
        }
    }
}

RwBool _rxHeapReset(RxHeap* heap)
{
    RxHeapSuperBlock* previous = NULL;
    RxHeapSuperBlock* superBlock;

    heap->freeBlocksUsed = 0;
    superBlock = heap->firstSuperBlock->next;
    while (superBlock != NULL) {
        if (!HeapSuperBlockReset(superBlock, previous, heap)) {
            return FALSE;
        }
        if (previous == NULL) {
            heap->firstBlock = superBlock->start;
        }
        previous = superBlock;
        superBlock = superBlock->next;
    }

    superBlock = heap->firstSuperBlock;
    if (!HeapSuperBlockReset(superBlock, previous, heap)) {
        return FALSE;
    }
    if (previous == NULL) {
        heap->firstBlock = superBlock->start;
    }
    heap->dirty = FALSE;
    return TRUE;
}

/* Near miss: clean source omits retail's unused post-free parameter clear. */
void RxHeapDestroy(RxHeap* heap)
{
    if (heap != NULL) {
        RxHeapSuperBlock* superBlock;

        if (heap->freeBlocks != NULL) {
            RwEngineInstance->fpFree(heap->freeBlocks);
            heap->freeBlocks = NULL;
        }
        superBlock = heap->firstSuperBlock;
        while (superBlock != NULL) {
            RxHeapSuperBlock* next = superBlock->next;
            HeapSuperBlockDestroy(superBlock);
            superBlock = next;
        }
        RwEngineInstance->fpFree(heap);
    }
}

RxHeap* RxHeapCreate(RwUInt32 size)
{
    RxHeap* heap;

    if (size < 0x400) {
        size = 0x400;
    }
    heap = RwEngineInstance->fpMalloc(sizeof(*heap), 0x40409);
    if (heap != NULL) {
        RxHeapSuperBlock* superBlock;
        RwBool reset;

        size = (size + 0x1F) & ~(RwUInt32)0x1F;
        if (size < 0x80) {
            size = 0x80;
        }
        superBlock = HeapSuperBlockCreate(size);
        if (superBlock != NULL) {
            heap->superBlockSize = size;
            heap->firstSuperBlock = superBlock;
            heap->freeBlocks = NULL;
            heap->freeBlocksAllocated = 0;
            heap->freeBlocksUsed = 0;
            heap->dirty = TRUE;

            if (!heap->dirty) {
                reset = TRUE;
            } else {
                reset = _rxHeapReset(heap);
            }
            if (reset) {
                return heap;
            }
            HeapSuperBlockDestroy(superBlock);
        }
        RwEngineInstance->fpFree(heap);
    }
    return NULL;
}
