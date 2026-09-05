#include "msl/ExtHeapMgr.h"
#include "runtime/cstring.h"

/*
 * Retail MSL external-heap owner.
 * Soft ceiling: ExternalHeap_AlignAlloc ~96.78%. Its descriptor split,
 * rollback, accounting, and loop CFG match retail; the remaining two
 * redundant candidate-node loads and GPR coloring are compiler emission.
 * CompareBlocksBySizeThenAddress is exact.
 */

static void ExternalHeap_MutexNullFunc(void* mutex);
static int KeyCompareBlocksByAddress(
    const void* key, const RedBlackNode* node);
static int KeyCompareBlocksBySize(
    const void* key, const RedBlackNode* node);
static int CompareBlocksByAddress(
    const RedBlackNode* first, const RedBlackNode* second);
static int CompareBlocksBySizeThenAddress(
    const RedBlackNode* first, const RedBlackNode* second);

#define BLOCK_FROM_ADDRESS_NODE(node) \
    ((ExternalHeapBlock*)((char*)(node) - sizeof(RedBlackNode)))
#define CONST_BLOCK_FROM_ADDRESS_NODE(node) \
    ((const ExternalHeapBlock*)((const char*)(node) - \
        sizeof(RedBlackNode)))
#define BLOCK_FROM_SIZE_NODE(node) \
    ((ExternalHeapBlock*)(node))
#define CONST_BLOCK_FROM_SIZE_NODE(node) \
    ((const ExternalHeapBlock*)(node))

ExternalHeapMutexRoutine fn_ExtHeapMgr_MutexEnter =
    ExternalHeap_MutexNullFunc;
ExternalHeapMutexRoutine fn_ExtHeapMgr_MutexExit =
    ExternalHeap_MutexNullFunc;
ExternalHeapMallocRoutine fn_ExtHeapMgr_SystemMalloc;
ExternalHeapFreeRoutine fn_ExtHeapMgr_SystemFree;

static inline ExternalHeapBlock* ExternalHeap_PopFreeBlock(
    ExternalHeap* heap) {
    ExternalHeapBlock* block = heap->free_blocks;

    if (block != 0) {
        heap->free_blocks = block->link.next_free;
    }
    return block;
}

void ExternalHeap_Free(
    ExternalHeap* heap, unsigned long address) {
    RedBlackNode* address_node;

    if (heap->mutex != 0) {
        fn_ExtHeapMgr_MutexEnter(heap->mutex);
    }

    address_node = RBTK_FindQuickNodeEqualToKey(
        &heap->address_tree, (const void*)address);
    if (address_node != 0) {
        ExternalHeapBlock* block =
            BLOCK_FROM_ADDRESS_NODE(address_node);

        if (block->state.is_free == 0) {
            RedBlackNode* next_node;
            RedBlackNode* previous_node;

            heap->free_count++;
            heap->free_bytes += block->size;
            heap->used_bytes -= block->size;
            next_node = RBN_GetNextNode(address_node);
            previous_node = RBN_GetPreviousNode(address_node);

            if (next_node != 0) {
                ExternalHeapBlock* next =
                    BLOCK_FROM_ADDRESS_NODE(next_node);

                if (next->state.is_free != 0) {
                    RBT_RemoveNode(&heap->size_tree, &next->size_node);
                    RBT_RemoveNode(
                        &heap->address_tree, &block->address_node);
                    next->link.address = block->link.address;
                    next->size += block->size;
                    block->link.next_free = heap->free_blocks;
                    heap->free_blocks = block;
                    block = next;
                }
            }

            if (previous_node != 0) {
                ExternalHeapBlock* previous =
                    BLOCK_FROM_ADDRESS_NODE(previous_node);

                if (previous->state.is_free != 0) {
                    RBT_RemoveNode(
                        &heap->size_tree, &previous->size_node);
                    RBT_RemoveNode(
                        &heap->address_tree, &block->address_node);
                    previous->size += block->size;
                    block->link.next_free = heap->free_blocks;
                    heap->free_blocks = block;
                    block = previous;
                }
            }

            block->state.is_free = 1;
            RBT_InsertNode(&heap->size_tree, &block->size_node);
        }
    }

    if (heap->mutex != 0) {
        fn_ExtHeapMgr_MutexExit(heap->mutex);
    }
}

unsigned long ExternalHeap_Alloc(
    ExternalHeap* heap, unsigned long size) {
    unsigned long address = 0;

    if (heap != 0) {
        address = ExternalHeap_AlignAlloc(
            heap, size, heap->alignment);
    }
    return address;
}

/* TODO: [near miss] 96.78%; two candidate reloads and GPR coloring only;
 * retail rollback sentinel is retained without dereferencing it. */
unsigned long ExternalHeap_AlignAlloc(
    ExternalHeap* heap, unsigned long size, int alignment) {
    ExternalHeapBlock* candidate;
    ExternalHeapBlock* prefix_block;
    unsigned long mask;
    unsigned long inverse_mask;
    unsigned long aligned_size;
    unsigned long result = 0;
    unsigned long prefix;
    ExternalHeapBlock* split_block;
    ExternalHeapBlock* suffix;
    unsigned long aligned_address;
    unsigned long usable;
    RedBlackNode* previous;
    RedBlackNode* candidate_node;

    if ((long)size <= 0) {
        return 0;
    }
    if (heap->mutex != 0) {
        fn_ExtHeapMgr_MutexEnter(heap->mutex);
    }

    if (alignment < 0) {
        alignment = 0;
    } else if (alignment > 24) {
        alignment = 24;
    }
    mask = (1UL << alignment) - 1;
    inverse_mask = ~mask;
    aligned_size = (size + mask) & inverse_mask;
    RBTK_GetPrevNextToKey(
        &heap->size_tree, (const void*)size,
        &previous, &candidate_node);
    prefix_block = 0;
    while (result == 0 && candidate_node != 0) {
        aligned_address =
            (BLOCK_FROM_SIZE_NODE(candidate_node)->link.address + mask) &
            inverse_mask;
        candidate = BLOCK_FROM_SIZE_NODE(candidate_node);

        if (aligned_address <
            candidate->link.address + candidate->size) {
            prefix = aligned_address - candidate->link.address;
            usable = candidate->size - prefix;
        } else {
            prefix = 0;
            usable = 0;
        }

        if (aligned_size <= usable &&
            (prefix == 0 || prefix_block == 0)) {

            RBT_RemoveNode(&heap->size_tree, &candidate->size_node);
            if (prefix != 0) {
                split_block = ExternalHeap_PopFreeBlock(heap);

                if (split_block != 0) {
                    split_block->link.address = aligned_address;
                    split_block->size = candidate->size - prefix;
                    split_block->state.is_free = 1;
                    RBT_InsertNode(
                        &heap->address_tree, &split_block->address_node);
                    candidate->size = prefix;
                    prefix_block = candidate;
                    RBT_InsertNode(
                        &heap->size_tree, &candidate->size_node);
                    candidate = split_block;
                } else {
                    RBT_InsertNode(
                        &heap->size_tree, &candidate->size_node);
                    break;
                }
            }

            if (aligned_size < usable) {
                suffix = ExternalHeap_PopFreeBlock(heap);

                if (suffix != 0) {
                    suffix->link.address =
                        candidate->link.address + aligned_size;
                    suffix->size = candidate->size - aligned_size;
                    suffix->state.is_free = 1;
                    RBT_InsertNode(
                        &heap->address_tree, &suffix->address_node);
                    RBT_InsertNode(
                        &heap->size_tree, &suffix->size_node);
                    candidate->size = aligned_size;
                    candidate->state.is_free = 0;
                    result = candidate->link.address;
                    candidate_node = 0;
                    heap->allocation_count++;
                    heap->free_bytes -= aligned_size;
                    heap->used_bytes += aligned_size;
                    if ((long)heap->peak_used < (long)heap->used_bytes) {
                        heap->peak_used = heap->used_bytes;
                    }
                } else {
                    if (prefix != 0) {
                        RBT_RemoveNode(
                            &heap->size_tree, &prefix_block->size_node);
                        RBT_RemoveNode(
                            &heap->address_tree, &candidate->address_node);
                        prefix_block->size += candidate->size;
                        RBT_InsertNode(
                            &heap->size_tree, &prefix_block->size_node);
                        /* Non-null state records a rolled-back candidate. */
                        prefix_block =
                            (ExternalHeapBlock*)sizeof(RedBlackNode);
                        candidate->link.next_free = heap->free_blocks;
                        heap->free_blocks = candidate;
                        candidate_node = RBN_GetNextNode(candidate_node);
                    } else {
                        RBT_InsertNode(
                            &heap->size_tree, &candidate->size_node);
                        break;
                    }
                }
            } else {
                candidate->state.is_free = 0;
                result = candidate->link.address;
                candidate_node = 0;
                heap->allocation_count++;
                heap->free_bytes -= aligned_size;
                heap->used_bytes += aligned_size;
                if ((long)heap->peak_used < (long)heap->used_bytes) {
                    heap->peak_used = heap->used_bytes;
                }
            }
        } else {
            candidate_node = RBN_GetNextNode(candidate_node);
        }
    }

    if (heap->mutex != 0) {
        fn_ExtHeapMgr_MutexExit(heap->mutex);
    }
    return result;
}

void ExternalHeap_SetMutex(ExternalHeap* heap, void* mutex) {
    heap->mutex = mutex;
}

static inline void* ExternalHeap_SystemAlloc(unsigned int size) {
    void* allocation = fn_ExtHeapMgr_SystemMalloc(size);

    if (allocation != 0) {
        memset(allocation, 0, size);
    }
    return allocation;
}

static inline void ExternalHeap_Init(ExternalHeap* heap) {
    ExternalHeapBlock* blocks = heap->blocks;
    int i;
    int last_block;

    if (blocks != 0) {
        last_block = heap->block_count - 1;
        for (i = 1; i < last_block; i++) {
            blocks[i].link.next_free = &blocks[i + 1];
        }
        blocks[last_block].link.next_free = 0;
        heap->free_blocks = &blocks[1];

        blocks[0].link.address = heap->base;
        blocks[0].size = heap->size;
        blocks[0].state.is_free = 1;

        heap->address_tree.root = 0;
        heap->address_tree.compare_nodes = CompareBlocksByAddress;
        heap->address_tree.compare_key = KeyCompareBlocksByAddress;
        RBT_InsertNode(&heap->address_tree, &blocks[0].address_node);

        heap->size_tree.root = 0;
        heap->size_tree.compare_nodes = CompareBlocksBySizeThenAddress;
        heap->size_tree.compare_key = KeyCompareBlocksBySize;
        RBT_InsertNode(&heap->size_tree, &blocks[0].size_node);

        heap->allocation_count = 0;
        heap->free_count = 0;
        heap->free_bytes = heap->size;
        heap->used_bytes = 0;
        heap->peak_used = 0;
    } else {
        memset(heap, 0, sizeof(ExternalHeap));
    }
}

static inline void ExternalHeap_Destroy(ExternalHeap* heap) {
    if (heap != 0) {
        if (heap->blocks != 0) {
            fn_ExtHeapMgr_SystemFree(heap->blocks);
            heap->blocks = 0;
        }
        ExternalHeap_Init(heap);
        fn_ExtHeapMgr_SystemFree(heap);
    }
}

ExternalHeap* ExternalHeap_Create(
    unsigned long base, unsigned long size,
    unsigned long alignment, int block_count) {
    ExternalHeap* heap =
        (ExternalHeap*)ExternalHeap_SystemAlloc(sizeof(ExternalHeap));

    if (heap != 0) {
        ExternalHeapBlock* blocks;
        unsigned int blocks_size;

        if (block_count < 4) {
            block_count = 4;
        }

        blocks_size = block_count * sizeof(ExternalHeapBlock);
        blocks = (ExternalHeapBlock*)ExternalHeap_SystemAlloc(blocks_size);
        heap->blocks = blocks;
        if (heap->blocks != 0) {
            heap->base = base;
            heap->size = size;
            heap->alignment = alignment;
            heap->block_count = block_count;
            ExternalHeap_Init(heap);
        } else {
            ExternalHeap_Destroy(heap);
            heap = 0;
        }
    }
    return heap;
}

void ExternalHeap_SetSysMutexRoutines(
    ExternalHeapMutexRoutine enter,
    ExternalHeapMutexRoutine leave) {
    fn_ExtHeapMgr_MutexEnter = enter;
    fn_ExtHeapMgr_MutexExit = leave;
}

void ExternalHeap_SetSysMemRoutines(
    ExternalHeapMallocRoutine allocate,
    ExternalHeapFreeRoutine free) {
    fn_ExtHeapMgr_SystemMalloc = allocate;
    fn_ExtHeapMgr_SystemFree = free;
}

static int KeyCompareBlocksByAddress(
    const void* key_value, const RedBlackNode* node) {
    unsigned long key = (unsigned long)key_value;
    unsigned long address =
        CONST_BLOCK_FROM_ADDRESS_NODE(node)->link.address;

    if (key < address) {
        return -1;
    }
    if (key > address) {
        return 1;
    }
    return 0;
}

static int KeyCompareBlocksBySize(
    const void* key_value, const RedBlackNode* node) {
    unsigned long key = (unsigned long)key_value;
    const ExternalHeapBlock* block =
        CONST_BLOCK_FROM_SIZE_NODE(node);

    if (key < block->size) {
        return -1;
    }
    if (key > block->size) {
        return 1;
    }
    return 0;
}

static int CompareBlocksByAddress(
    const RedBlackNode* first, const RedBlackNode* second) {
    const ExternalHeapBlock* first_block =
        CONST_BLOCK_FROM_ADDRESS_NODE(first);
    const ExternalHeapBlock* second_block =
        CONST_BLOCK_FROM_ADDRESS_NODE(second);
    unsigned long address = first_block->link.address;
    unsigned long other = second_block->link.address;

    if (address < other) {
        return -1;
    }
    if (address > other) {
        return 1;
    }
    return 0;
}

static int CompareBlocksBySizeThenAddress(
    const RedBlackNode* first, const RedBlackNode* second) {
    const ExternalHeapBlock* first_block =
        CONST_BLOCK_FROM_SIZE_NODE(first);
    const ExternalHeapBlock* second_block =
        CONST_BLOCK_FROM_SIZE_NODE(second);
    unsigned long first_size = first_block->size;
    unsigned long second_size = second_block->size;
    unsigned long address;
    unsigned long other;

    if (first_size < second_size) {
        return -1;
    }
    if (second_size < first_size) {
        return 1;
    }

    address = first_block->link.address;
    other = second_block->link.address;
    if (address < other) {
        return -1;
    }
    if (address > other) {
        return 1;
    }
    return 0;
}

static void ExternalHeap_MutexNullFunc(void* mutex) {
}
