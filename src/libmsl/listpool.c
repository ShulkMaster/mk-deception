#include "msl/listpool.h"

void mslDebugPrintf(const char* format, ...);

static const char stringBase0[] =
    "ListInsert: was in another list, performing REMOVE first\n\0"
    "ListRemove: NULL node, ignored\n\0"
    "next->pprv != prev\n\0"
    "*prev != next\n\0"
    "*l != next\n\0"
    "ListInsert FIXME: node to insert was already tail of list\n\0"
    "ListInsert FIXME: node to insert was already head of list\n\0"
    "node->next != oldhead\n\0"
    "node->pprv != l\n\0"
    "oldhead->pprv != &node->next\n\0"
    "*l != node\n\0"
    "ListNodeFind NULL pool\n\0"
    "ListNodeFind pool was never Attached\n\0"
    "ListNodeFind: id %08x index %d exceeds pool size (%x)\n\0"
    "ListNodeData NULL node\n\0"
    "ListPoolReset NULL pool\n\0"
    "ListPoolReset pool was never Attached\n\0"
    "ListNodeFree NULL node\n\0"
    "ListNodeFree invalid Node 0x%08x\n\0"
    "ListNodeAllocIDrange NULL pool\n\0"
    "ListNodeAllocIDrange pool was never Attached\n\0"
    "ListNodeAllocIDrange: id 0 Invalid\n\0"
    "ListNodeAllocIDrange: max (%d) is < min (%d)\n\0"
    "ListNodeAllocIDrange: max 0x%x exceeds pool size (%x)\n\0"
    "ListNodeAllocIDrange: id %08x exceeds pool size (%x)\n\0"
    "ListNodeAllocIDrange: id %08x exceeds max (0x%x)\n\0"
    "ListNodeAllocIDrange (%08x): node not in free list???\n\0"
    "ListNodeAllocID NULL pool\n\0"
    "ListNodeAllocID pool was never Attached\n\0"
    "ListNodeAllocID: id 0 Invalid\n\0"
    "ListNodeAllocID: id %08x exceeds pool size (%x)\n\0"
    "ListNodeAllocID: ID %08x is already in use\n\0"
    "ListNodeAllocID (%08x): node not in free list???\n\0"
    "ListNodeAlloc NULL pool\n\0"
    "ListNodeAlloc pool was never Attached\n\0"
    "ListNodeAlloc no more nodes\n\0"
    "ListPoolDetach discarding %d un-deleted elements\n\0"
    "ListPoolAttach NULL == pool\n\0"
    "ListPoolAttach NULL == mem\n\0"
    "ListPoolAttach to what?  elts == 0\n\0"
    "ListPoolAttach: %d elts of size %d at 0x%08x\n";

/* Retail has two alignment bytes and an unresolved four-byte .rodata gap here. */

#define LIST_STRING(offset) (&stringBase0[(offset)])

static inline _ListNode* remove_node(_ListNode** list, _ListNode* node) {
    if (node == 0) {
        mslDebugPrintf(LIST_STRING(0x3A));
        return 0;
    } else {
        _ListNode* next = node->next;
        _ListNode** previous_link = node->previous_link;

        if (next != 0) {
            next->previous_link = previous_link;
        }
        if (previous_link != 0) {
            *previous_link = next;
        }
        node->next = 0;
        node->previous_link = 0;

        if (next != 0 && next->previous_link != previous_link) {
            mslDebugPrintf(LIST_STRING(0x5A));
        }
        if (previous_link != 0 && *previous_link != next) {
            mslDebugPrintf(LIST_STRING(0x6E));
        }
        if (*list != next) {
            mslDebugPrintf(LIST_STRING(0x7D));
        }
    }
    return node;
}

static inline void insert_node(_ListNode** list, _ListNode* node) {
    _ListNode* old_head;

    if (*list == node) {
        mslDebugPrintf(LIST_STRING(0xC4));
        return;
    }

    if (node->previous_link != 0) {
        mslDebugPrintf(LIST_STRING(0));
        remove_node(node->previous_link, node);
    }

    node->next = *list;
    node->previous_link = list;
    old_head = *list;
    if (old_head != 0) {
        old_head->previous_link = &node->next;
    }
    *list = node;

    if (node->next != old_head) {
        mslDebugPrintf(LIST_STRING(0xFF));
    }
    if (node->previous_link != list) {
        mslDebugPrintf(LIST_STRING(0x116));
    }
    if (old_head != 0 && old_head->previous_link != &node->next) {
        mslDebugPrintf(LIST_STRING(0x127));
    }
    if (*list != node) {
        mslDebugPrintf(LIST_STRING(0x145));
    }
}

/* Soft ceiling: ListInsertAtTail ~94.41% -- inline-helper emit differences. */
void ListInsertAtTail(_ListNode** list, _ListNode* node) {
    _ListNode* tail;

    if (node->previous_link != 0) {
        mslDebugPrintf(LIST_STRING(0));
        remove_node(node->previous_link, node);
    }

    tail = 0;
    if (list != 0) {
        _ListNode* head = *list;
        if (head != 0) {
            tail = head;
            while (tail->next != 0) {
                tail = tail->next;
            }
        }
    }

    if (tail == node) {
        mslDebugPrintf(LIST_STRING(0x89));
    } else if (tail != 0) {
        tail->next = node;
        node->next = 0;
        node->previous_link = &tail->next;
    } else {
        insert_node(list, node);
    }
}

void ListNext(_ListNode** node) {
    *node = (*node)->next;
}

_ListNode* ListRemove(_ListNode** list) {
    _ListNode* node = *list;
    _ListNode* next;
    _ListNode** previous_link;

    if (node == 0) {
        mslDebugPrintf(LIST_STRING(0x3A));
        return 0;
    }

    next = node->next;
    previous_link = node->previous_link;
    if (next != 0) {
        next->previous_link = previous_link;
    }
    if (previous_link != 0) {
        *previous_link = next;
    }
    node->next = 0;
    node->previous_link = 0;
    *list = next;

    if (next != 0 && next->previous_link != previous_link) {
        mslDebugPrintf(LIST_STRING(0x5A));
    }
    if (previous_link != 0 && *previous_link != next) {
        mslDebugPrintf(LIST_STRING(0x6E));
    }
    if (*list != next) {
        mslDebugPrintf(LIST_STRING(0x7D));
    }
    return node;
}

/* Soft ceiling: ListInsert ~95.93% -- inline-helper emit differences. */
void ListInsert(_ListNode** list, _ListNode* node) {
    insert_node(list, node);
}

typedef union ListNodeId {
    msl_u32 value;
    struct {
        msl_u16 index;
        msl_u16 generation;
    } parts;
} ListNodeId;

msl_u32 ListNodeID(ListPool* pool, _ListNode* node) {
    ListNodeId id;

    id.parts.index = node->index;
    id.parts.generation = node->generation;
    return id.value;
}

_ListNode* ListNodeFind(ListPool* pool, msl_u32 value) {
    ListNodeId id;
    _ListNode* node = 0;

    id.value = value;
    if (pool == 0) {
        mslDebugPrintf(LIST_STRING(0x151));
        return 0;
    }
    if (pool->element_count == 0) {
        mslDebugPrintf(LIST_STRING(0x169));
        return 0;
    }
    if (id.parts.index >= pool->element_count) {
        mslDebugPrintf(
            LIST_STRING(0x18F), value, id.parts.index,
            pool->element_count);
    } else {
        node = &pool->nodes[id.parts.index & 0xFFFF];
        if (node->state == 0 ||
            node->generation != id.parts.generation) {
            node = 0;
        }
    }
    return node;
}

void* ListNodeData(ListPool* pool, _ListNode* node) {
    if (node == 0) {
        mslDebugPrintf(LIST_STRING(0x1C6));
        return 0;
    }
    return node->data;
}

/* Soft ceiling: ListNodeFree ~97.28% -- inline-helper scheduling. */
void ListNodeFree(ListPool* pool, _ListNode* node) {
    if (pool == 0) {
        mslDebugPrintf(LIST_STRING(0x1DE));
    } else if (pool->element_count == 0) {
        mslDebugPrintf(LIST_STRING(0x1F7));
    } else if (node == 0) {
        mslDebugPrintf(LIST_STRING(0x21E));
    } else if (node < pool->nodes ||
               node >= pool->nodes + pool->element_count) {
        mslDebugPrintf(LIST_STRING(0x236), node);
    } else if (node->state != 0) {
        node->state = 0;
        node->generation++;
        if (node->generation == 0) {
            node->generation = 1;
        }
        if (pool->element_size == 0) {
            node->data = 0;
        }
        insert_node(&pool->free_list, node);
        pool->allocated_count--;
    }
}

/* Soft ceiling: ListNodeAlloc ~98.20% -- inline-helper scheduling. */
_ListNode* ListNodeAlloc(ListPool* pool) {
    _ListNode* node;
    int allocated;

    if (pool == 0) {
        mslDebugPrintf(LIST_STRING(0x4C0));
        return 0;
    }
    if (pool->element_count == 0) {
        mslDebugPrintf(LIST_STRING(0x4D9));
        return 0;
    }

    node = pool->free_list;
    if (node != 0) {
        remove_node(&pool->free_list, node);
        node->state = 1;
        if (node->generation == 0) {
            node->generation++;
        }
        node->next = 0;
        node->previous_link = 0;
        pool->allocated_count++;
        if (pool->peak_allocated < pool->allocated_count) {
            pool->peak_allocated = pool->allocated_count;
        }
        allocated = 1;
    } else {
        allocated = 0;
    }

    if (allocated == 0) {
        mslDebugPrintf(LIST_STRING(0x500));
    }
    return node;
}

/*
 * Soft ceiling: ListPoolAttach ~99.49% -- retail derives the zero loop index
 * from the already-zero previous-node register; the remaining instruction is
 * equivalent immediate-zero scheduling.
 */
int ListPoolAttach(
    ListPool* pool, void* memory, msl_u32 element_count,
    msl_u32 element_size) {
    msl_u32 index;
    char* data;
    _ListNode* previous;
    _ListNode* node;

    if (pool == 0) {
        mslDebugPrintf(LIST_STRING(0x54F));
        return 0;
    }
    if (memory == 0) {
        mslDebugPrintf(LIST_STRING(0x56C));
        return 0;
    }
    if (element_count == 0) {
        mslDebugPrintf(LIST_STRING(0x588));
        return 0;
    }

    mslDebugPrintf(
        LIST_STRING(0x5AC), element_count, element_size, memory);
    previous = 0;
    pool->element_count = element_count;
    pool->element_size = element_size;
    pool->elements = (char*)memory;
    pool->nodes = (_ListNode*)(
        (char*)memory + element_count * element_size);

    if (pool == 0) {
        mslDebugPrintf(LIST_STRING(0x1DE));
    } else if (pool->element_count == 0) {
        mslDebugPrintf(LIST_STRING(0x1F7));
    } else {
        data = pool->elements;
        node = pool->nodes;
        for (index = 0; index < pool->element_count; index++, node++) {
            node->index = index;
            node->state = 0;
            node->generation++;
            if (pool->element_size == 0) {
                node->data = 0;
            } else {
                node->data = data;
            }
            node->next = previous;
            if (previous != 0) {
                previous->previous_link = &node->next;
            }

            data += pool->element_size;
            previous = node;
        }

        pool->free_list = previous;
        previous->previous_link = &pool->free_list;
        pool->allocated_count = 0;
    }

    pool->peak_allocated = 0;
    return 1;
}
