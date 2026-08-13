#ifndef RW_RWFREELIST_H
#define RW_RWFREELIST_H

#include "rw/rwcore_types.h"
#include "rw/rwdevice.h"
#include "rw/rwplcore.h"

typedef struct RwGlobals RwGlobals;

typedef struct RwFreeList {
    int entrySize;
    int entriesPerBlock;
    int heapSize;
    int alignment;
    RwLinkList blockList;
    unsigned int flags;
    RwLLLink link;
} RwFreeList;
typedef void (*RwFreeListCallBack)(void*, void*);

void _rwFreeListEnable(int enable);
RwFreeList* RwFreeListCreate(int entrySize, int entriesPerBlock,
                             int alignment, unsigned int hint);
RwFreeList* RwFreeListCreateAndPreallocateSpace(
    int entrySize, int entriesPerBlock, int alignment,
    int preallocBlocks, RwFreeList* freeList, unsigned int hint);
int RwFreeListDestroy(RwFreeList* freeList);
void* _rwFreeListAllocReal(RwFreeList* freeList, unsigned int hint);
RwFreeList* _rwFreeListFreeReal(RwFreeList* freeList, void* entry);
int RwFreeListPurge(RwFreeList* freeList);
RwFreeList* RwFreeListForAllUsed(RwFreeList*, RwFreeListCallBack, void*);
int RwFreeListPurgeAllFreeLists(void);
int _rwMemoryOpen(const RwMemoryFunctions* memoryFunctions);
void _rwMemoryClose(void);

#endif
