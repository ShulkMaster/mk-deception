#include "rw/rwplcore.h"

typedef struct RwResHeap RwResHeap;
typedef struct RwResHeapBlock RwResHeapBlock;

struct RwResHeap {
    RwResHeapBlock* firstBlock;
    RwResHeapBlock* firstFreeBlock;
};

struct RwResHeapBlock {
    RwResHeap* heap;
    RwResHeapBlock* next;
    RwResHeapBlock* prev;
    unsigned int size;
    unsigned int flags;
    unsigned int reserved[3];
};

static void splitBlock(RwResHeapBlock* block, unsigned int size) {
    RwResHeapBlock* newBlock =
        (RwResHeapBlock*)((unsigned char*)block + 0x20 + size);

    if (block->next != 0 &&
        (int)(~block->next->flags & 1) != 0) {
        newBlock->next = block->next->next;
        newBlock->size = block->next->size + (block->size - size);
    } else {
        newBlock->next = block->next;
        newBlock->size = block->size - size - 0x20;
    }
    block->next = newBlock;
    newBlock->flags = 0;
    newBlock->prev = block;
    if (newBlock->next != 0) {
        newBlock->next->prev = newBlock;
    }
    block->size = size;
    newBlock->heap = block->heap;
}

int _rwResHeapInit(RwResHeap* heap, unsigned int size) {
    RwResHeap* owner = heap;
    unsigned long startAddress = ((unsigned long)heap + 0x27) & ~0x1FUL;
    unsigned long endAddress = ((unsigned long)heap + size) & ~0x1FUL;
    int payloadSize = (int)(endAddress - startAddress - 0x20);
    RwResHeapBlock* block;

    if (payloadSize < 0x20) {
        return 0;
    }
    block = (RwResHeapBlock*)startAddress;
    block->heap = owner;
    block->next = 0;
    block->prev = 0;
    block->flags = 0;
    block->size = payloadSize;
    owner->firstBlock = block;
    owner->firstFreeBlock = block;
    return 1;
}

int _rwResHeapClose(RwResHeap* heap) {
    return 1;
}

void _rwResHeapFree(void* memory) {
    RwResHeapBlock* block =
        (RwResHeapBlock*)((unsigned char*)memory - 0x20);
    RwResHeapBlock* previous;
    RwResHeapBlock* next;

    block->flags = 0;
    previous = block->prev;
    next = block->next;
    if (block->heap->firstFreeBlock == 0 ||
        block < block->heap->firstFreeBlock) {
        block->heap->firstFreeBlock = block;
    }
    if (previous != 0 && (int)(~previous->flags & 1) != 0) {
        unsigned int mergedSize;

        previous->next = next;
        if (next != 0) {
            next->prev = previous;
        }
        mergedSize = previous->size + block->size;
        previous->size = mergedSize + 0x20;
        block = previous;
    }
    if (next != 0 && (int)(~next->flags & 1) != 0) {
        unsigned int mergedSize;

        block->next = next->next;
        if (next->next != 0) {
            next->next->prev = block;
        }
        mergedSize = block->size + next->size;
        block->size = mergedSize + 0x20;
    }
}

void* _rwResHeapAlloc(RwResHeap* heap, unsigned int size) {
    RwResHeapBlock* cursor;
    RwResHeapBlock* block;

    size = (size + 0x1F) & ~0x1F;
    block = 0;
    cursor = heap->firstFreeBlock;
    while (cursor != 0 && block == 0) {
        if ((int)(~cursor->flags & 1) != 0 && cursor->size >= size) {
            block = cursor;
        }
        cursor = cursor->next;
    }
    if (block == 0) {
        return 0;
    }
    if (block->size > size + 0x40) {
        splitBlock(block, size);
    }
    if (block == heap->firstFreeBlock) {
        do {
            heap->firstFreeBlock = heap->firstFreeBlock->next;
        } while (heap->firstFreeBlock != 0 &&
                 (int)(heap->firstFreeBlock->flags & 1) != 0);
    }
    block->flags = 1;
    return (unsigned char*)block + 0x20;
}
