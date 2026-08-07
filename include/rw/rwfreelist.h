#ifndef RW_RWFREELIST_H
#define RW_RWFREELIST_H

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RwGlobals RwGlobals;
typedef struct RwMemoryFunctions RwMemoryFunctions;

typedef struct RwFreeList {
    RwInt32 entrySize;
    RwInt32 entriesPerBlock;
    RwInt32 heapSize;
    RwInt32 alignment;
    RwLinkList blockList;
    RwUInt32 flags;
    RwLLLink link;
} RwFreeList;
typedef void (*RwFreeListCallBack)(void*, void*);

void _rwFreeListEnable(RwBool enable);
RwFreeList* RwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock,
                             RwInt32 alignment, RwUInt32 hint);
RwFreeList* RwFreeListCreateAndPreallocateSpace(
    RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment,
    RwInt32 preallocBlocks, RwFreeList* freeList, RwUInt32 hint);
RwBool RwFreeListDestroy(RwFreeList* freeList);
void* _rwFreeListAllocReal(RwFreeList* freeList, RwUInt32 hint);
RwFreeList* _rwFreeListFreeReal(RwFreeList* freeList, void* entry);
RwInt32 RwFreeListPurge(RwFreeList* freeList);
RwFreeList* RwFreeListForAllUsed(RwFreeList*, RwFreeListCallBack, void*);
RwInt32 RwFreeListPurgeAllFreeLists(void);
RwBool _rwMemoryOpen(const RwMemoryFunctions* memoryFunctions);
void _rwMemoryClose(void);

#define RWPLUGINOFFSET(type, object, offset) \
    (*(type*)((unsigned char*)(object) + (offset)))

#endif
