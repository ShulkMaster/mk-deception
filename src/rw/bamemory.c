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
                                  RwInt32 alignment, RwInt32 preallocBlocks,
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
                                  RwInt32 alignment, RwInt32 preallocBlocks,
                                  RwFreeList* freeList, RwUInt32 hint)
{
    RwInt32 alignedEntrySize;
    RwInt32 heapSize;

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
    heapSize = ((entriesPerBlock + 7) & ~7) >> 3;
    freeList->entrySize = alignedEntrySize;
    freeList->entriesPerBlock = entriesPerBlock;
    freeList->alignment = alignment;
    freeList->heapSize = heapSize;
    freeList->blockList.link.next = &freeList->blockList.link;
    freeList->blockList.link.prev = &freeList->blockList.link;

    while (preallocBlocks != 0) {
        RwFreeBlock* block = RwEngineInstance->fpMalloc(
            heapSize + entriesPerBlock * alignedEntrySize + alignment + 7, hint);
        if (block == NULL) {
            _RwFreeListFree(freeList);
            return NULL;
        }
        block->link.next = NULL;
        block->link.prev = NULL;
        rwLinkListAddLLLink(&freeList->blockList, &block->link);
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

/* Near miss: exact MSB-first bitmap algorithm; loop scheduling and lifetimes differ. */
void* _rwFreeListAllocReal(RwFreeList* freeList, RwUInt32 hint)
{
    void* result = NULL;
    RwInt32 heapSize = freeList->heapSize;
    RwLLLink* link = freeList->blockList.link.next;
    while (link != &freeList->blockList.link && result == NULL) {
        RwFreeBlock* block = (RwFreeBlock*)link;
        RwInt32 remaining = freeList->entriesPerBlock;
        RwInt32 byteIndex;
        for (byteIndex = 0; byteIndex < heapSize && result == NULL; ++byteIndex) {
            RwUInt8 byte = block->heap[byteIndex];
            if (byte != 0xFF) {
                RwInt32 bitIndex;
                for (bitIndex = 0; bitIndex < 8 && remaining != 0; ++bitIndex) {
                    RwUInt8 mask = (RwUInt8)(0x80 >> bitIndex);
                    if (!(byte & mask)) {
                        RwUInt8* base;
                        block->heap[byteIndex] |= mask;
                        base = (RwUInt8*)(((unsigned long)block + freeList->heapSize +
                                          freeList->alignment + 7) &
                                         ~(freeList->alignment - 1));
                        result = base + freeList->entrySize *
                                        (byteIndex * 8 + bitIndex);
                        break;
                    }
                    --remaining;
                }
            } else {
                remaining -= 8;
            }
        }
        link = link->next;
    }

    if (result == NULL) {
        RwFreeBlock* block = RwEngineInstance->fpMalloc(
            heapSize + freeList->entriesPerBlock * freeList->entrySize +
                freeList->alignment + 7,
            hint);
        RwUInt8* base;
        if (block == NULL)
            return NULL;
        memset(block->heap, 0, heapSize);
        rwLinkListAddLLLink(&freeList->blockList, &block->link);
        block->heap[0] = 0x80;
        base = (RwUInt8*)(((unsigned long)block + heapSize +
                          freeList->alignment + 7) &
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
    RwLLLink* link = freeList->blockList.link.next;
    while (link != &freeList->blockList.link) {
        RwFreeBlock* block = (RwFreeBlock*)link;
        RwUInt8* rawBase = block->heap + freeList->heapSize;
        if ((RwUInt8*)entry >= rawBase &&
            (RwUInt8*)entry <= rawBase + freeList->entriesPerBlock * freeList->entrySize) {
            RwUInt32 index = ((RwUInt8*)entry - rawBase) / freeList->entrySize;
            RwUInt32 byteIndex = index >> 3;
            RwUInt8 mask = (RwUInt8)(0x80 >> (index - byteIndex * 8));
            block->heap[byteIndex] &= (RwUInt8)~mask;
            if ((freeList->flags & 2) &&
                FreeListBlockIsEmpty(block->heap, freeList->heapSize)) {
                rwLinkListRemoveLLLink(&block->link);
                RwEngineInstance->fpFree(block);
            }
            return freeList;
        }
        link = link->next;
    }
    return NULL;
}

/* Near miss: exact unlink/reinsert and return units; scheduling differs. */
RwInt32 RwFreeListPurge(RwFreeList* freeList)
{
    RwInt32 freed = 0;
    RwLLLink* link = freeList->blockList.link.next;
    while (link != &freeList->blockList.link) {
        RwFreeBlock* block = (RwFreeBlock*)link;
        RwLLLink* next;
        rwLinkListRemoveLLLink(link);
        next = link->next;
        if (FreeListBlockIsEmpty(block->heap, freeList->heapSize)) {
            RwEngineInstance->fpFree(block);
            ++freed;
        } else {
            rwLinkListAddLLLink(&freeList->blockList, link);
        }
        link = next;
    }
    return freed * freeList->entrySize;
}

/* Near miss: exact snapshot/mutation-safe traversal; loop register coloring differs. */
RwFreeList* RwFreeListForAllUsed(RwFreeList* freeList,
                                 RwFreeListCallBack callback, void* data)
{
    RwLLLink* link = freeList->blockList.link.next;
    while (link != &freeList->blockList.link) {
        RwFreeBlock* block = (RwFreeBlock*)link;
        RwUInt8* heap = RwEngineInstance->fpMalloc(freeList->heapSize, 0x10000);
        RwLLLink* next;
        RwInt32 byteIndex;
        if (heap == NULL)
            return NULL;
        memcpy(heap, block->heap, freeList->heapSize);
        next = link->next;
        for (byteIndex = 0; byteIndex < freeList->heapSize; ++byteIndex) {
            if (heap[byteIndex] != 0) {
                RwInt32 bitIndex;
                for (bitIndex = 0; bitIndex < 8; ++bitIndex) {
                    RwUInt8 mask = (RwUInt8)(0x80 >> bitIndex);
                    if (heap[byteIndex] & mask) {
                        RwUInt8* base = (RwUInt8*)(((unsigned long)block +
                            freeList->heapSize + freeList->alignment + 7) &
                            ~(freeList->alignment - 1));
                        callback(base + freeList->entrySize *
                                 (byteIndex * 8 + bitIndex), data);
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

/* Near miss: exact callback installation; aggregate access scheduling differs. */
RwBool _rwMemoryOpen(const RwMemoryFunctions* functions)
{
    if (!_rwFreeListModuleOpen())
        return FALSE;
    if (functions != NULL) {
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
    return TRUE;
}

void _rwMemoryClose(void)
{
    _rwFreeListModuleClose();
}
