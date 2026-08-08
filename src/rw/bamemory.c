#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rwfreelist.h"

typedef struct RwFreeBlock {
    RwLLLink link;
    RwUInt8 heap[1];
} RwFreeBlock;

extern void* malloc(RwUInt32);
extern void free(void*);
extern void* realloc(void*, RwUInt32);
extern void* calloc(RwUInt32, RwUInt32);

static RwFreeList _masterFreeList;
static RwBool FreeListsEnabled = TRUE;
static RwLinkList _freeListList;
static RwFreeList* _masterFreeListPtr;
static RwBool freeListModuleOpen;

static RwFreeList* FreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock,
                                  RwUInt32 alignment, RwUInt32 preallocBlocks,
                                  RwFreeList* freeList, RwUInt32 hint);
static void _RwFreeListFree(RwFreeList* freeList);
static RwBool FreeListBlockIsEmpty(const RwUInt8* heap, RwUInt32 heapSize);

void _rwFreeListEnable(RwBool enable)
{
    FreeListsEnabled = enable;
}

static RwBool _rwFreeListModuleOpen(void)
{
    RwLLLink* head;

    _freeListList.link.next = &_freeListList.link;
    head = &_freeListList.link;
    _freeListList.link.prev = head;
    freeListModuleOpen = TRUE;
    _masterFreeListPtr = FreeListCreate(0x24, 0x10, 0x20, 0,
                                        &_masterFreeList, 0x40000);
    if (_masterFreeListPtr == NULL) {
        freeListModuleOpen = FALSE;
        return FALSE;
    }
    rwLinkListRemoveLLLink(&_masterFreeListPtr->link);
    return TRUE;
}

static void _rwFreeListModuleClose(void)
{
    RwLLLink* link = _freeListList.link.next;
    RwLLLink* head = &_freeListList.link;

    while (link != head) {
        RwFreeList* freeList = (RwFreeList*)((RwUInt8*)link - 0x1C);
        RwFreeListDestroy(freeList);
        link = _freeListList.link.next;
        head = &_freeListList.link;
    }
    RwFreeListDestroy(_masterFreeListPtr);
    _masterFreeListPtr = NULL;
    freeListModuleOpen = FALSE;
}

/* Near miss: exact ownership, hint, alignment, and allocation CFG; scheduling differs. */
static RwFreeList* FreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock,
                                  RwUInt32 alignment, RwUInt32 preallocBlocks,
                                  RwFreeList* freeList, RwUInt32 hint)
{
    RwInt32 alignedEntrySize;
    RwUInt32 heapSize;

    if (!FreeListsEnabled)
        preallocBlocks = 0;
    if (alignment == 0)
        alignment = 0x20;
    if (freeList == NULL) {
        if (_masterFreeListPtr != NULL)
            freeList = RwEngineInstance->fpFreeListAlloc(_masterFreeListPtr,
                                                         hint & 0xFF0000);
        else
            freeList = RwEngineInstance->fpMalloc(sizeof(RwFreeList),
                                                  hint & 0xFF0000);
        if (freeList == NULL)
            return NULL;
        freeList->flags = 2;
    } else {
        freeList->flags = 3;
    }

    alignedEntrySize = (entrySize + alignment - 1) & ~(alignment - 1);
    heapSize = ((RwUInt32)(entriesPerBlock + 7) & ~7U) >> 3;
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
        if (block == NULL) {
            _RwFreeListFree(freeList);
            return NULL;
        }
        newLink = &block->link;
        newLink->prev = NULL;
        newLink->next = NULL;
        rwLinkListAddLLLink(&freeList->blockList, newLink);
        memset(block->heap, 0, heapSize);
        --preallocBlocks;
    }
    rwLinkListAddLLLink(&_freeListList, &freeList->link);
    return freeList;
}

RwFreeList* RwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock,
                             RwInt32 alignment, RwUInt32 hint)
{
    return FreeListCreate(entrySize, entriesPerBlock, alignment, 1, NULL, hint);
}

RwFreeList* RwFreeListCreateAndPreallocateSpace(
    RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment,
    RwInt32 preallocBlocks, RwFreeList* freeList, RwUInt32 hint)
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
        if (_masterFreeListPtr == freeList || _masterFreeListPtr == NULL)
            RwEngineInstance->fpFree(freeList);
        else
            RwEngineInstance->fpFreeListFree(_masterFreeListPtr, freeList);
    }
}

RwBool RwFreeListDestroy(RwFreeList* freeList)
{
    rwLinkListRemoveLLLink(&freeList->link);
    _RwFreeListFree(freeList);
    return TRUE;
}

/* Near miss: exact bitmap/allocation behavior; clean-C local register coloring differs. */
void* _rwFreeListAllocReal(RwFreeList* freeList, RwUInt32 hint)
{
    RwUInt8* result = NULL;
    RwUInt32 heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head && result == NULL) {
        RwUInt8* heap = ((RwFreeBlock*)link)->heap;
        RwUInt32 remaining = freeList->entriesPerBlock;
        RwUInt32 byteIndex = 0;
        while (byteIndex < heapSize) {
            RwUInt8 byte = heap[byteIndex];
            if (byte != 0xFF) {
                RwUInt32 bitIndex = 0;
                while (bitIndex < 8 && remaining != 0) {
                    RwUInt8 mask = (RwUInt8)(0x80 >> bitIndex);
                    if (!(byte & mask)) {
                        RwUInt8* base;
                        heap[byteIndex] =
                            (RwUInt8)(heap[byteIndex] | mask);
                        base = (RwUInt8*)link + heapSize +
                               freeList->alignment + 7;
                        base = (RwUInt8*)((unsigned long)base &
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
            if (result == NULL)
                ++byteIndex;
            else
                break;
        }
        link = link->next;
    }

    if (result == NULL) {
        RwLLLink* newLink;
        RwFreeBlock* block;
        RwUInt8* base;
        block = RwEngineInstance->fpMalloc(
            freeList->alignment +
                (heapSize + freeList->entriesPerBlock * freeList->entrySize) + 7,
            hint);
        if (block == NULL)
            return NULL;
        memset(block->heap, 0, heapSize);
        newLink = &block->link;
        rwLinkListAddLLLink(&freeList->blockList, newLink);
        block->heap[0] = 0x80;
        base = (RwUInt8*)block + heapSize + freeList->alignment + 7;
        base = (RwUInt8*)((unsigned long)base &
                         ~(freeList->alignment - 1));
        result = base;
    }
    return result;
}

static RwBool FreeListBlockIsEmpty(const RwUInt8* heap, RwUInt32 heapSize)
{
    RwInt32 sum = 0;
    RwInt32 i;
    for (i = 0; i < heapSize; ++i)
        sum += heap[i];
    return sum == 0;
}

/* Near miss: exact inclusive range and bitmap clearing; register allocation differs. */
RwFreeList* _rwFreeListFreeReal(RwFreeList* freeList, void* entry)
{
    RwUInt32 heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head) {
        RwUInt8* rawBase = ((RwFreeBlock*)link)->heap + heapSize;
        if ((RwUInt8*)entry >= rawBase &&
            (RwUInt8*)entry <= rawBase + freeList->entriesPerBlock * freeList->entrySize) {
            RwUInt32 index = ((RwUInt8*)entry - rawBase) /
                             (RwUInt32)freeList->entrySize;
            RwUInt32 byteIndex = index >> 3;
            RwUInt8 mask = (RwUInt8)(0x80 >> (index - byteIndex * 8));
            RwUInt8* heap = ((RwFreeBlock*)link)->heap;
            heap[byteIndex] &= (RwUInt8)~mask;
            if ((freeList->flags & 2) &&
                FreeListBlockIsEmpty(heap, heapSize)) {
                rwLinkListRemoveLLLink(link);
                RwEngineInstance->fpFree(link);
            }
            return freeList;
        }
        link = link->next;
    }
    return NULL;
}

RwInt32 RwFreeListPurge(RwFreeList* freeList)
{
    RwInt32 freed = 0;
    RwUInt32 heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head) {
        RwUInt8* heap = ((RwFreeBlock*)link)->heap;
        rwLinkListRemoveLLLink(link);
        next = link->next;
        if (FreeListBlockIsEmpty(heap, heapSize)) {
            RwEngineInstance->fpFree(link);
            ++freed;
        } else {
            rwLinkListAddLLLink(&freeList->blockList, link);
        }
        link = next;
    }
    return freed * freeList->entrySize;
}

/* Near miss: exact mutation-safe snapshot traversal; one commutative add is scheduled differently. */
RwFreeList* RwFreeListForAllUsed(RwFreeList* freeList,
                                 RwFreeListCallBack callback, void* data)
{
    RwUInt32 heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    RwLLLink* head = &freeList->blockList.link;
    while (link != head) {
        RwLLLink* next;
        RwUInt8* blockHeap = ((RwFreeBlock*)link)->heap;
        RwUInt8* heap = RwEngineInstance->fpMalloc(heapSize, 0x10000);
        RwUInt32 byteIndex;
        if (heap == NULL)
            return NULL;
        memcpy(heap, blockHeap, heapSize);
        next = link->next;
        for (byteIndex = 0; byteIndex < heapSize; ++byteIndex) {
            RwUInt32 byte = heap[byteIndex];
            if (byte != 0) {
                RwUInt32 bitIndex;
                for (bitIndex = 0; bitIndex < 8; ++bitIndex) {
                    RwUInt8 mask = (RwUInt8)(0x80 >> bitIndex);
                    if (byte & mask) {
                        RwUInt8* base = (RwUInt8*)link + heapSize +
                                        freeList->alignment + 7;
                        void* entry;
                        base = (RwUInt8*)((unsigned long)base &
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

RwInt32 RwFreeListPurgeAllFreeLists(void)
{
    RwInt32 total = 0;
    RwLLLink* link = _freeListList.link.next;
    RwLLLink* head = &_freeListList.link;
    while (link != head) {
        RwFreeList* freeList = (RwFreeList*)((RwUInt8*)link - 0x1C);
        RwInt32 purged = RwFreeListPurge(freeList);
        if (purged > 0)
            total += purged;
        link = link->next;
    }
    return total;
}

static void* HMalloc(RwUInt32 size, RwUInt32 hint)
{
    return malloc(size);
}

static void* HRealloc(void* memory, RwUInt32 size, RwUInt32 hint)
{
    return realloc(memory, size);
}

static void* HCalloc(RwUInt32 count, RwUInt32 size, RwUInt32 hint)
{
    return calloc(count, size);
}

/* The engine's four contiguous memory callbacks share RwMemoryFunctions layout. */
RwBool _rwMemoryOpen(const RwMemoryFunctions* functions)
{
    if (!_rwFreeListModuleOpen())
        return FALSE;
    if (functions != NULL) {
        *(RwMemoryFunctions*)&RwEngineInstance->fpMalloc = *functions;
    } else {
        RwEngineInstance->fpMalloc = HMalloc;
        RwEngineInstance->fpFree = free;
        RwEngineInstance->fpRealloc = HRealloc;
        RwEngineInstance->fpCalloc = HCalloc;
    }
    return TRUE;
}

void _rwMemoryClose(void)
{
    _rwFreeListModuleClose();
}
