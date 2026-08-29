#include "rw/rwengine.h"
#include "runtime/cstdlib.h"
#include "runtime/cstring.h"
#include "rw/rwfreelist.h"

typedef struct RwFreeBlock {
    RwLLLink link;
    unsigned char heap[1];
} RwFreeBlock;

static RwFreeList _masterFreeList;
static int FreeListsEnabled = 1;
static RwLinkList _freeListList;
static RwFreeList* _masterFreeListPtr;
static int freeListModuleOpen;

static RwFreeList* FreeListCreate(int entrySize, int entriesPerBlock,
                                  unsigned int alignment, unsigned int preallocBlocks,
                                  RwFreeList* freeList, unsigned int hint);
static void _RwFreeListFree(RwFreeList* freeList);
static int FreeListBlockIsEmpty(const unsigned char* heap, unsigned int heapSize);

void _rwFreeListEnable(int enable)
{
    FreeListsEnabled = enable;
}

static int _rwFreeListModuleOpen(void)
{
    RwLLLink* head;

    _freeListList.link.next = &_freeListList.link;
    head = &_freeListList.link;
    _freeListList.link.prev = head;
    freeListModuleOpen = 1;
    _masterFreeListPtr = FreeListCreate(0x24, 0x10, 0x20, 0,
                                        &_masterFreeList, 0x40000);
    if (_masterFreeListPtr == 0) {
        freeListModuleOpen = 0;
        return 0;
    }
    rwLinkListRemoveLLLink(&_masterFreeListPtr->link);
    return 1;
}

static void _rwFreeListModuleClose(void)
{
    RwLLLink* link = _freeListList.link.next;
    RwLLLink* head = &_freeListList.link;

    while (link != head) {
        RwFreeList* freeList = RW_CONTAINER_OF(link, RwFreeList, link);
        RwFreeListDestroy(freeList);
        link = _freeListList.link.next;
        head = &_freeListList.link;
    }
    RwFreeListDestroy(_masterFreeListPtr);
    _masterFreeListPtr = 0;
    freeListModuleOpen = 0;
}


static RwFreeList* FreeListCreate(int entrySize, int entriesPerBlock,
                                  unsigned int alignment, unsigned int preallocBlocks,
                                  RwFreeList* freeList, unsigned int hint)
{
    int alignedEntrySize;
    unsigned int heapSize;

    if (!FreeListsEnabled)
        preallocBlocks = 0;
    if (alignment == 0)
        alignment = 0x20;
    if (freeList == 0) {
        if (_masterFreeListPtr != 0)
            freeList = RwEngineInstance->fpFreeListAlloc(_masterFreeListPtr,
                                                         hint & 0xFF0000);
        else
            freeList = RwEngineInstance->fpMalloc(sizeof(RwFreeList),
                                                  hint & 0xFF0000);
        if (freeList == 0)
            return 0;
        freeList->flags = 2;
    } else {
        freeList->flags = 3;
    }

    alignedEntrySize = (entrySize + alignment - 1) & ~(alignment - 1);
    heapSize = ((unsigned int)(entriesPerBlock + 7) & ~7U) >> 3;
    freeList->entrySize = alignedEntrySize;
    freeList->entriesPerBlock = entriesPerBlock;
    freeList->alignment = alignment;
    freeList->heapSize = heapSize;
    freeList->blockList.link.next = &freeList->blockList.link;
    freeList->blockList.link.prev = &freeList->blockList.link;

    while (preallocBlocks != 0) {
        RwLLLink* newLink;
        RwFreeBlock* block;

        block = RwEngineInstance->fpMalloc(
            heapSize + entriesPerBlock * alignedEntrySize + alignment + 7,
            hint);
        if (block == 0) {
            _RwFreeListFree(freeList);
            return 0;
        }
        newLink = &block->link;
        newLink->prev = 0;
        newLink->next = 0;
        rwLinkListAddLLLink(&freeList->blockList, newLink);
        memset(block->heap, 0, heapSize);
        --preallocBlocks;
    }
    rwLinkListAddLLLink(&_freeListList, &freeList->link);
    return freeList;
}

RwFreeList* RwFreeListCreate(int entrySize, int entriesPerBlock,
                             int alignment, unsigned int hint)
{
    return FreeListCreate(entrySize, entriesPerBlock, alignment, 1, 0, hint);
}

RwFreeList* RwFreeListCreateAndPreallocateSpace(
    int entrySize, int entriesPerBlock, int alignment,
    int preallocBlocks, RwFreeList* freeList, unsigned int hint)
{
    return FreeListCreate(entrySize, entriesPerBlock, alignment, preallocBlocks,
                          freeList, hint);
}

static void _RwFreeListFree(RwFreeList* freeList)
{
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* head = &freeList->blockList.link;

    while (link != head) {
        rwLinkListRemoveLLLink(link);
        RwEngineInstance->fpFree(link);
        link = freeList->blockList.link.next;
        head = &freeList->blockList.link;
    }
    if (!(freeList->flags & 1)) {
        if (_masterFreeListPtr == freeList || _masterFreeListPtr == 0)
            RwEngineInstance->fpFree(freeList);
        else
            RwEngineInstance->fpFreeListFree(_masterFreeListPtr, freeList);
    }
}

int RwFreeListDestroy(RwFreeList* freeList)
{
    RwLLLink* previous;

    freeList->link.prev->next = freeList->link.next;
    previous = freeList->link.prev;
    freeList->link.next->prev = previous;
    _RwFreeListFree(freeList);
    return 1;
}


void* _rwFreeListAllocReal(RwFreeList* freeList, unsigned int hint)
{
    unsigned char* result = 0;
    unsigned int heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head && result == 0) {
        unsigned char* heap = ((RwFreeBlock*)link)->heap;
        unsigned int remaining = freeList->entriesPerBlock;
        unsigned int byteIndex = 0;
        while (byteIndex < heapSize) {
            unsigned char byte = heap[byteIndex];
            if (byte != 0xFF) {
                unsigned int bitIndex = 0;
                while (bitIndex < 8 && remaining != 0) {
                    unsigned char mask = (unsigned char)(0x80 >> bitIndex);
                    if (!(byte & mask)) {
                        unsigned char* base;
                        heap[byteIndex] =
                            (unsigned char)(heap[byteIndex] | mask);
                        base = (unsigned char*)link + heapSize +
                               freeList->alignment + 7;
                        base = (unsigned char*)((unsigned long)base &
                                         ~(freeList->alignment - 1));
                        result = base + freeList->entrySize *
                                        (byteIndex * 8 + bitIndex);
                        break;
                    }
                    ++bitIndex;
                    --remaining;
                }
            } else {
                remaining -= 8;
            }
            if (result == 0)
                ++byteIndex;
            else
                break;
        }
        link = link->next;
    }

    if (result == 0) {
        RwLLLink* newLink;
        RwFreeBlock* block;
        unsigned char* base;
        block = RwEngineInstance->fpMalloc(
            freeList->alignment +
                (heapSize + freeList->entriesPerBlock * freeList->entrySize) + 7,
            hint);
        if (block == 0)
            return 0;
        memset(block->heap, 0, heapSize);
        newLink = &block->link;
        rwLinkListAddLLLink(&freeList->blockList, newLink);
        block->heap[0] = 0x80;
        base = (unsigned char*)block + heapSize + freeList->alignment + 7;
        base = (unsigned char*)((unsigned long)base &
                         ~(freeList->alignment - 1));
        result = base;
    }
    return result;
}

static int FreeListBlockIsEmpty(const unsigned char* heap, unsigned int heapSize)
{
    int sum = 0;
    int i;
    for (i = 0; i < heapSize; ++i)
        sum += heap[i];
    return sum == 0;
}


RwFreeList* _rwFreeListFreeReal(RwFreeList* freeList, void* entry)
{
    unsigned int heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head) {
        unsigned char* rawBase = ((RwFreeBlock*)link)->heap + heapSize;
        if ((unsigned char*)entry >= rawBase &&
            (unsigned char*)entry <= rawBase + freeList->entriesPerBlock * freeList->entrySize) {
            unsigned int index = ((unsigned char*)entry - rawBase) /
                             (unsigned int)freeList->entrySize;
            unsigned int byteIndex = index >> 3;
            unsigned char mask = (unsigned char)(0x80 >> (index - byteIndex * 8));
            unsigned char* heap = ((RwFreeBlock*)link)->heap;
            heap[byteIndex] &= (unsigned char)~mask;
            if ((freeList->flags & 2) &&
                FreeListBlockIsEmpty(heap, heapSize)) {
                RwLLLink* previous;

                link->prev->next = link->next;
                previous = link->prev;
                link->next->prev = previous;
                RwEngineInstance->fpFree(link);
            }
            return freeList;
        }
        link = link->next;
    }
    return 0;
}

int RwFreeListPurge(RwFreeList* freeList)
{
    int freed = 0;
    unsigned int heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head) {
        unsigned char* heap = ((RwFreeBlock*)link)->heap;
        RwLLLink* previous;

        link->prev->next = link->next;
        previous = link->prev;
        link->next->prev = previous;
        next = link->next;
        if (FreeListBlockIsEmpty(heap, heapSize)) {
            RwEngineInstance->fpFree(link);
            ++freed;
        } else {
            link->next = freeList->blockList.link.next;
            link->prev = &freeList->blockList.link;
            freeList->blockList.link.next->prev = link;
            freeList->blockList.link.next = link;
        }
        link = next;
    }
    return freed * freeList->entrySize;
}


RwFreeList* RwFreeListForAllUsed(RwFreeList* freeList,
                                 RwFreeListCallBack callback, void* data)
{
    unsigned int heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head) {
        RwLLLink* next;
        unsigned char* blockHeap = ((RwFreeBlock*)link)->heap;
        unsigned char* heap = RwEngineInstance->fpMalloc(heapSize, 0x10000);
        unsigned int byteIndex;
        if (heap == 0)
            return 0;
        memcpy(heap, blockHeap, heapSize);
        next = link->next;
        for (byteIndex = 0; byteIndex < heapSize; ++byteIndex) {
            unsigned int byte = heap[byteIndex];
            if (byte != 0) {
                unsigned int bitIndex;
                for (bitIndex = 0; bitIndex < 8; ++bitIndex) {
                    unsigned char mask = (unsigned char)(0x80 >> bitIndex);
                    if (byte & mask) {
                        unsigned char* base = (unsigned char*)link + heapSize +
                                        freeList->alignment + 7;
                        void* entry;
                        base = (unsigned char*)((unsigned long)base &
                                         ~(freeList->alignment - 1));
                        entry = base + freeList->entrySize *
                                       (byteIndex * 8 + bitIndex);
                        callback(entry, data);
                    }
                }
            }
        }
        RwEngineInstance->fpFree(heap);
        link = next;
    }
    return freeList;
}

int RwFreeListPurgeAllFreeLists(void)
{
    int total = 0;
    RwLLLink* link = _freeListList.link.next;
    RwLLLink* head = &_freeListList.link;
    while (link != head) {
        RwFreeList* freeList = RW_CONTAINER_OF(link, RwFreeList, link);
        int purged = RwFreeListPurge(freeList);
        if (purged > 0)
            total += purged;
        link = link->next;
    }
    return total;
}

static void* HMalloc(unsigned long size, unsigned int hint)
{
    return malloc(size);
}

static void* HRealloc(void* memory, unsigned long size, unsigned int hint)
{
    return realloc(memory, size);
}

static void* HCalloc(unsigned long count, unsigned long size, unsigned int hint)
{
    return calloc(count, size);
}


int _rwMemoryOpen(const RwMemoryFunctions* functions)
{
    if (!_rwFreeListModuleOpen())
        return 0;
    if (functions != 0) {
        RwEngineInstance->fpMalloc = functions->alloc;
        RwEngineInstance->fpFree = functions->free;
        RwEngineInstance->fpRealloc = functions->realloc;
        RwEngineInstance->fpCalloc = functions->calloc;
    } else {
        RwEngineInstance->fpMalloc = HMalloc;
        RwEngineInstance->fpFree = free;
        RwEngineInstance->fpRealloc = HRealloc;
        RwEngineInstance->fpCalloc = HCalloc;
    }
    return 1;
}

void _rwMemoryClose(void)
{
    _rwFreeListModuleClose();
}
