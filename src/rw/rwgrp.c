#include "rw/rwplcore.h"

typedef struct RwChunkGroupFreeListStorage {
    unsigned char data[0x24];
} RwChunkGroupFreeListStorage;

extern unsigned char* RwEngineInstance;
extern void* RwFreeListCreateAndPreallocateSpace(
    RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment,
    RwInt32 preallocBlocks, RwChunkGroupFreeListStorage* storage,
    RwUInt32 hint);
extern void RwFreeListDestroy(void* freeList);

static RwChunkGroupFreeListStorage _rwChunkGroupFList;
static RwInt32 _rwChunkGroupFListBlockSize = 0x10;
static RwInt32 _rwChunkGroupFListPreallocBlocks = 1;
static RwModuleInfo chunkGroupModule;

#define CHUNK_GROUP_FREELIST \
    (*(void**)(RwEngineInstance + chunkGroupModule.globalsOffset))

void* _rwChunkGroupOpen(void* instance, RwInt32 offset, RwInt32 size) {
    chunkGroupModule.globalsOffset = offset;
    CHUNK_GROUP_FREELIST = RwFreeListCreateAndPreallocateSpace(
        0x21, _rwChunkGroupFListBlockSize, 4,
        _rwChunkGroupFListPreallocBlocks, &_rwChunkGroupFList, 0x40412);
    if (CHUNK_GROUP_FREELIST == 0) {
        return 0;
    }
    chunkGroupModule.numInstances++;
    return instance;
}

void* _rwChunkGroupClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (CHUNK_GROUP_FREELIST != 0) {
        RwFreeListDestroy(CHUNK_GROUP_FREELIST);
    }
    chunkGroupModule.numInstances--;
    return instance;
}
