#ifndef RW_RWFREELIST_H
#define RW_RWFREELIST_H

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RwGlobals RwGlobals;

typedef struct RwLinkList {
    RwLLLink link;
} RwLinkList;

typedef struct RwFreeList {
    RwInt32 entrySize;
    RwInt32 entriesPerBlock;
    RwInt32 heapSize;
    RwInt32 alignment;
    RwLinkList blockList;
    RwUInt32 flags;
    RwLLLink link;
} RwFreeList;

RwFreeList* RwFreeListCreateAndPreallocateSpace(
    RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment,
    RwInt32 preallocBlocks, RwFreeList* freeList, RwUInt32 hint);
RwBool RwFreeListDestroy(RwFreeList* freeList);

#define RWPLUGINOFFSET(type, object, offset) \
    (*(type*)((unsigned char*)(object) + (offset)))

#endif
