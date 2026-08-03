#ifndef MSL_EXT_HEAP_MGR_H
#define MSL_EXT_HEAP_MGR_H

#include "msl/RedBlackTree.h"

typedef struct ExternalHeapBlockState {
    RedBlackNode* parent;         /* +0x00 */
    RedBlackNode* left;           /* +0x04 */
    RedBlackNode* right;          /* +0x08 */
    unsigned char black;          /* +0x0C */
    unsigned char is_free;        /* +0x0D */
    unsigned char pad0E[2];
} ExternalHeapBlockState; /* 0x10 */

typedef char ExternalHeapBlockStateSize[
    sizeof(ExternalHeapBlockState) == 0x10 ? 1 : -1];

typedef struct ExternalHeapBlock {
    union {
        RedBlackNode size_node;
        ExternalHeapBlockState state;
    };                            /* +0x00 */
    RedBlackNode address_node;    /* +0x10 */
    union {
        struct ExternalHeapBlock* next_free;
        unsigned long address;
    } link;                       /* +0x20 */
    unsigned long size;           /* +0x24 */
} ExternalHeapBlock; /* 0x28 */

typedef char ExternalHeapBlockSize[
    sizeof(ExternalHeapBlock) == 0x28 ? 1 : -1];

typedef struct ExternalHeap {
    unsigned long base;           /* +0x00 */
    unsigned long size;           /* +0x04 */
    unsigned long alignment;      /* +0x08 */
    int block_count;              /* +0x0C */
    void* mutex;                  /* +0x10 */
    unsigned long allocation_count; /* +0x14 */
    unsigned long free_count;     /* +0x18 */
    unsigned long free_bytes;     /* +0x1C */
    unsigned long used_bytes;     /* +0x20 */
    unsigned long peak_used;      /* +0x24 */
    RedBlackTree address_tree;    /* +0x28 */
    RedBlackTree size_tree;       /* +0x34 */
    ExternalHeapBlock* free_blocks; /* +0x40 */
    ExternalHeapBlock* blocks;    /* +0x44 */
} ExternalHeap; /* 0x48 */

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
unsigned long ExternalHeap_AlignAlloc(
    ExternalHeap* heap, unsigned long size,
    int alignment);
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
