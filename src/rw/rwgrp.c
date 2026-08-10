#include "rw/rwfreelist.h"

extern RwGlobals* RwEngineInstance;

static RwFreeList _rwChunkGroupFList;
static RwInt32 _rwChunkGroupFListBlockSize = 0x10;
static RwInt32 _rwChunkGroupFListPreallocBlocks = 1;
static RwModuleInfo chunkGroupModule;

void* _rwChunkGroupOpen(void* instance, RwInt32 offset, RwInt32 size) {
    chunkGroupModule.globalsOffset = offset;
    *(RwFreeList**)((RwUInt8*)RwEngineInstance +
                   chunkGroupModule.globalsOffset) =
        RwFreeListCreateAndPreallocateSpace(
        0x21, _rwChunkGroupFListBlockSize, 4,
        _rwChunkGroupFListPreallocBlocks, &_rwChunkGroupFList, 0x40412);
    if (*(RwFreeList**)((RwUInt8*)RwEngineInstance +
                       chunkGroupModule.globalsOffset) == 0) {
        return 0;
    }
    chunkGroupModule.numInstances++;
    return instance;
}

void* _rwChunkGroupClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (*(RwFreeList**)((RwUInt8*)RwEngineInstance +
                       chunkGroupModule.globalsOffset) != 0) {
        RwFreeListDestroy(*(RwFreeList**)((RwUInt8*)RwEngineInstance +
                                         chunkGroupModule.globalsOffset));
    }
    chunkGroupModule.numInstances--;
    return instance;
}
