#include "rw/rwengine.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

static RxHeapFreeBlock* HeapFreeBlocksNewEntry(RxHeap* heap)
{
    RxHeapFreeBlock* freeBlocks = heap->freeBlocks;
    unsigned int used = heap->freeBlocksUsed;

    if (heap->freeBlocksAllocated <= used) {
        freeBlocks = RwEngineInstance->fpRealloc(
            heap->freeBlocks,
            (heap->freeBlocksAllocated += 32) * sizeof(*freeBlocks),
            0x1030409);
        if (freeBlocks == 0) {
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
                unsigned int remaining;

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

    if (freeBlocks != 0) {
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

static RxHeapSuperBlock* HeapSuperBlockCreate(unsigned int size)
{
    RxHeapSuperBlock* superBlock =
        RwEngineInstance->fpMalloc(size + 0x8B, 0x1040409);

    if (superBlock != 0) {
        superBlock->start = (RxHeapBlock*)
            (((unsigned long)superBlock + 0x8B) & ~(unsigned long)0x7F);
        superBlock->size = size;
        superBlock->next = 0;
    }
    return superBlock;
}

static void HeapSuperBlockDestroy(RxHeapSuperBlock* superBlock)
{
    if (superBlock != 0) {
        RwEngineInstance->fpFree(superBlock);
    }
}

static int HeapSuperBlockReset(RxHeapSuperBlock* superBlock,
                                  RxHeapSuperBlock* previous, RxHeap* heap)
{
    RxHeapFreeBlock* freeEntry = HeapFreeBlocksNewEntry(heap);

    if (freeEntry != 0) {
        RxHeapBlock* first = superBlock->start;
        RxHeapBlock* freeBlock = first + 1;
        RxHeapBlock* last = (RxHeapBlock*)((unsigned char*)superBlock->start +
                                          superBlock->size - sizeof(*last));

        first->prev = 0;
        first->next = 0;
        first->size = 0;
        first->freeEntry = 0;
        *last = *first;

        first->next = freeBlock;
        freeBlock->prev = first;
        freeBlock->next = last;
        last->prev = freeBlock;
        freeBlock->size = (unsigned char*)last -
                          ((unsigned char*)freeBlock + sizeof(*freeBlock));
        freeBlock->freeEntry = freeEntry;
        freeEntry->block = freeBlock;
        freeEntry->size = freeBlock->size;

        if (previous != 0) {
            RxHeapBlock* previousLast =
                (RxHeapBlock*)((unsigned char*)previous->start + previous->size -
                               sizeof(*previousLast));
            previousLast->next = first;
            first->prev = previousLast;
        }
        return 1;
    }
    return 0;
}

void RxHeapFree(RxHeap* heap, void* memory)
{
    RxHeapBlock* block = (RxHeapBlock*)memory - 1;
    int previousIsFree =
        block->prev != 0 && block->prev->freeEntry != 0;
    int nextIsFree =
        block->next != 0 && block->next->freeEntry != 0;

    if (previousIsFree) {
        if (nextIsFree) {
            HeapFreeBlocksDeleteEntry(heap, block->next->freeEntry);
            block->prev->size += block->size + block->next->size +
                                 2 * sizeof(*block);
            block->prev->freeEntry->size = block->prev->size;
            block->prev->next = block->next->next;
            if (block->next->next != 0) {
                block->next->next->prev = block->prev;
            }
        } else {
            block->prev->size += block->size + sizeof(*block);
            block->prev->freeEntry->size = block->prev->size;
            block->prev->next = block->next;
            if (block->next != 0) {
                block->next->prev = block->prev;
            }
        }
    } else if (nextIsFree) {
        block->size += block->next->size + sizeof(*block);
        block->freeEntry = block->next->freeEntry;
        block->next->freeEntry->block = block;
        block->next->freeEntry->size = block->size;
        block->next = block->next->next;
        if (block->next != 0) {
            block->next->prev = block;
        }
    } else {
        RxHeapFreeBlock* freeEntry = HeapFreeBlocksNewEntry(heap);

        if (freeEntry != 0) {
            freeEntry->block = block;
            freeEntry->size = block->size;
            block->freeEntry = freeEntry;
        }
    }
}

int _rxHeapReset(RxHeap* heap)
{
    RxHeapSuperBlock* previous = 0;
    RxHeapSuperBlock* superBlock;

    heap->freeBlocksUsed = 0;
    superBlock = heap->firstSuperBlock->next;
    while (superBlock != 0) {
        if (!HeapSuperBlockReset(superBlock, previous, heap)) {
            return 0;
        }
        if (previous == 0) {
            heap->firstBlock = superBlock->start;
        }
        previous = superBlock;
        superBlock = superBlock->next;
    }

    superBlock = heap->firstSuperBlock;
    if (!HeapSuperBlockReset(superBlock, previous, heap)) {
        return 0;
    }
    if (previous == 0) {
        heap->firstBlock = superBlock->start;
    }
    heap->dirty = 0;
    return 1;
}
/*
 * Soft ceiling: RxHeapDestroy 95.6579% -- retail assigns heap = 0 after
 * freeing it; the dead assignment is intentionally omitted.
 */
void RxHeapDestroy(RxHeap* heap)
{
    if (heap != 0) {
        RxHeapSuperBlock* superBlock;

        if (heap->freeBlocks != 0) {
            RwEngineInstance->fpFree(heap->freeBlocks);
            heap->freeBlocks = 0;
        }
        superBlock = heap->firstSuperBlock;
        while (superBlock != 0) {
            RxHeapSuperBlock* next = superBlock->next;
            HeapSuperBlockDestroy(superBlock);
            superBlock = next;
        }
        RwEngineInstance->fpFree(heap);
    }
}

RxHeap* RxHeapCreate(unsigned int size)
{
    RxHeap* heap;

    if (size < 0x400) {
        size = 0x400;
    }
    heap = RwEngineInstance->fpMalloc(sizeof(*heap), 0x40409);
    if (heap != 0) {
        RxHeapSuperBlock* superBlock;
        size = (size + 0x1F) & ~(unsigned int)0x1F;
        if (size < 0x80) {
            size = 0x80;
        }
        superBlock = HeapSuperBlockCreate(size);
        if (superBlock != 0) {
            heap->superBlockSize = size;
            heap->firstSuperBlock = superBlock;
            heap->freeBlocks = 0;
            heap->freeBlocksAllocated = 0;
            heap->freeBlocksUsed = 0;
            heap->dirty = 1;

            if (RxHeapReset(heap)) {
                return heap;
            }
            HeapSuperBlockDestroy(superBlock);
        }
        RwEngineInstance->fpFree(heap);
    }
    return 0;
}
