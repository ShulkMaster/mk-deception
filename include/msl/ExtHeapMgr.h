#ifndef MSL_EXT_HEAP_MGR_H
#define MSL_EXT_HEAP_MGR_H

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
        unsigned long address;
    } link;
    unsigned long size;
} ExternalHeapBlock;

typedef char ExternalHeapBlockSize[
    sizeof(ExternalHeapBlock) == 0x28 ? 1 : -1];

typedef struct ExternalHeap {
    unsigned long base;
    unsigned long size;
    unsigned long alignment;
    int block_count;
    void* mutex;
    unsigned long allocation_count;
    unsigned long free_count;
    unsigned long free_bytes;
    unsigned long used_bytes;
    unsigned long peak_used;
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

unsigned long ExternalHeap_Alloc(
    ExternalHeap* heap, unsigned long size);

void ExternalHeap_Free(ExternalHeap* heap, unsigned long address);
void ExternalHeap_SetMutex(ExternalHeap* heap, void* mutex);
ExternalHeap* ExternalHeap_Create(
    unsigned long base, unsigned long size,
    unsigned long alignment, int block_count);
void ExternalHeap_SetSysMutexRoutines(
    ExternalHeapMutexRoutine enter, ExternalHeapMutexRoutine leave);
void ExternalHeap_SetSysMemRoutines(
    ExternalHeapMallocRoutine allocate, ExternalHeapFreeRoutine free);

#ifdef __cplusplus
}
#endif

#endif
