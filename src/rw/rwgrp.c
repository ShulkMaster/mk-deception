#include "rw/rwfreelist.h"

extern RwGlobals* RwEngineInstance;

static RwFreeList _rwChunkGroupFList;
static int _rwChunkGroupFListBlockSize = 0x10;
static int _rwChunkGroupFListPreallocBlocks = 1;
static RwModuleInfo chunkGroupModule;

void* _rwChunkGroupOpen(void* instance, int offset, int size) {
    chunkGroupModule.globalsOffset = offset;
    *(RwFreeList**)((unsigned char*)RwEngineInstance +
                   chunkGroupModule.globalsOffset) =
        RwFreeListCreateAndPreallocateSpace(
        0x21, _rwChunkGroupFListBlockSize, 4,
        _rwChunkGroupFListPreallocBlocks, &_rwChunkGroupFList, 0x40412);
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                       chunkGroupModule.globalsOffset) == 0) {
        return 0;
    }
    chunkGroupModule.numInstances++;
    return instance;
}

void* _rwChunkGroupClose(void* instance, int offset, int size) {
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                       chunkGroupModule.globalsOffset) != 0) {
        RwFreeListDestroy(*(RwFreeList**)((unsigned char*)RwEngineInstance +
                                         chunkGroupModule.globalsOffset));
    }
    chunkGroupModule.numInstances--;
    return instance;
}
