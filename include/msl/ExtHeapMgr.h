#ifndef MSL_EXT_HEAP_MGR_H
#define MSL_EXT_HEAP_MGR_H

#include "dolphin/types.h"
#include "msl/RedBlackTree.h"

typedef struct ExternalHeapBlockState {
    RedBlackNode* parent;
    RedBlackNode* left;
    RedBlackNode* right;
    unsigned char black;
    unsigned char is_free;
    unsigned char pad0E[2];
} ExternalHeapBlockState;

typedef char ExternalHeapBlockStateSize[
    sizeof(ExternalHeapBlockState) == 0x10 ? 1 : -1];

typedef struct ExternalHeapBlock {
    union {
        RedBlackNode size_node;
        ExternalHeapBlockState state;
    };
    RedBlackNode address_node;
    union {
        struct ExternalHeapBlock* next_free;
        u32 address;
    } link;
    u32 size;
} ExternalHeapBlock;

typedef char ExternalHeapBlockSize[
    sizeof(ExternalHeapBlock) == 0x28 ? 1 : -1];

typedef struct ExternalHeap {
    u32 base;
    u32 size;
    u32 alignment;
    int block_count;
    void* mutex;
    u32 allocation_count;
    u32 free_count;
    u32 free_bytes;
    u32 used_bytes;
    u32 peak_used;
    RedBlackTree address_tree;
    RedBlackTree size_tree;
    ExternalHeapBlock* free_blocks;
    ExternalHeapBlock* blocks;
} ExternalHeap;

typedef char ExternalHeapSize[
    sizeof(ExternalHeap) == 0x48 ? 1 : -1];

typedef void (*ExternalHeapMutexRoutine)(void* mutex);
typedef void* (*ExternalHeapMallocRoutine)(unsigned int size);
typedef void (*ExternalHeapFreeRoutine)(void* allocation);

#ifdef __cplusplus
extern "C" {
#endif

u32 ExternalHeap_Alloc(
    ExternalHeap* heap, u32 size);

void ExternalHeap_Free(ExternalHeap* heap, u32 address);
void ExternalHeap_SetMutex(ExternalHeap* heap, void* mutex);
ExternalHeap* ExternalHeap_Create(
    u32 base, u32 size,
    u32 alignment, int block_count);
void ExternalHeap_SetSysMutexRoutines(
    ExternalHeapMutexRoutine enter, ExternalHeapMutexRoutine leave);
void ExternalHeap_SetSysMemRoutines(
    ExternalHeapMallocRoutine allocate, ExternalHeapFreeRoutine free);

#ifdef __cplusplus
}
#endif

#endif
