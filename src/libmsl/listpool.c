#include "msl/listpool.h"
#include "msl/mslsupport.h"

static inline _ListNode* remove_node(_ListNode** list) {
    _ListNode* node = *list;
    if (node == 0) {
        mslDebugPrintf("ListRemove: NULL node, ignored\n");
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
        *list = next;

        if (next != 0 && next->previous_link != previous_link) {
            mslDebugPrintf("next->pprv != prev\n");
        }
        if (previous_link != 0 && *previous_link != next) {
            mslDebugPrintf("*prev != next\n");
        }
        if (*list != next) {
            mslDebugPrintf("*l != next\n");
        }
    }
    return node;
}

static inline void link_node(_ListNode** list, _ListNode* node) {
    _ListNode* old_head;
    node->next = *list;
    node->previous_link = list;
    old_head = *list;
    if (old_head != 0) {
        old_head->previous_link = &node->next;
    }
    *list = node;

    if (node->next != old_head) {
        mslDebugPrintf("node->next != oldhead\n");
    }
    if (node->previous_link != list) {
        mslDebugPrintf("node->pprv != l\n");
    }
    if (old_head != 0 && old_head->previous_link != &node->next) {
        mslDebugPrintf("oldhead->pprv != &node->next\n");
    }
    if (*list != node) {
        mslDebugPrintf("*l != node\n");
    }
}

static inline void insert_node(_ListNode** list, _ListNode* node) {
    if (*list == node) {
        mslDebugPrintf("ListInsert FIXME: node to insert was already head of list\n");
        return;
    }

    if (node->previous_link != 0) {
        mslDebugPrintf("ListInsert: was in another list, performing REMOVE first\n");
        node = remove_node(&node);
    }

    link_node(list, node);
}

void ListInsertAtTail(_ListNode** list, _ListNode* node) {
    _ListNode* tail;

    if (node->previous_link != 0) {
        mslDebugPrintf("ListInsert: was in another list, performing REMOVE first\n");
        node = remove_node(&node);
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
        mslDebugPrintf("ListInsert FIXME: node to insert was already tail of list\n");
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
        mslDebugPrintf("ListRemove: NULL node, ignored\n");
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
        mslDebugPrintf("next->pprv != prev\n");
    }
    if (previous_link != 0 && *previous_link != next) {
        mslDebugPrintf("*prev != next\n");
    }
    if (*list != next) {
        mslDebugPrintf("*l != next\n");
    }
    return node;
}

void ListInsert(_ListNode** list, _ListNode* node) {
    if (*list == node) {
        mslDebugPrintf("ListInsert FIXME: node to insert was already head of list\n");
        return;
    }
    if (node->previous_link != 0) {
        mslDebugPrintf("ListInsert: was in another list, performing REMOVE first\n");
        node = remove_node(&node);
    }
    link_node(list, node);
}

typedef union ListNodeId {
    u32 value;
    struct {
        u16 index;
        u16 generation;
    } parts;
} ListNodeId;

u32 ListNodeID(ListPool* pool, _ListNode* node) {
    ListNodeId id;

    id.parts.index = node->index;
    id.parts.generation = node->generation;
    return id.value;
}

_ListNode* ListNodeFind(ListPool* pool, u32 value) {
    ListNodeId id;
    _ListNode* node = 0;

    id.value = value;
    if (pool == 0) {
        mslDebugPrintf("ListNodeFind NULL pool\n");
        return 0;
    }
    if (pool->element_count == 0) {
        mslDebugPrintf("ListNodeFind pool was never Attached\n");
        return 0;
    }
    if (id.parts.index >= pool->element_count) {
        mslDebugPrintf(
            "ListNodeFind: id %08x index %d exceeds pool size (%x)\n", value, id.parts.index,
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
        mslDebugPrintf("ListNodeData NULL node\n");
        return 0;
    }
    return node->data;
}

void ListNodeFree(ListPool* pool, _ListNode* node) {
    if (pool == 0) {
        mslDebugPrintf("ListPoolReset NULL pool\n");
    } else if (pool->element_count == 0) {
        mslDebugPrintf("ListPoolReset pool was never Attached\n");
    } else if (node == 0) {
        mslDebugPrintf("ListNodeFree NULL node\n");
    } else if (node < pool->nodes ||
               node >= pool->nodes + pool->element_count) {
        mslDebugPrintf("ListNodeFree invalid Node 0x%08x\n", node);
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

static void fn_stub1(void) {
    mslDebugPrintf("ListNodeAllocIDrange NULL pool\n");
    mslDebugPrintf("ListNodeAllocIDrange pool was never Attached\n");
    mslDebugPrintf("ListNodeAllocIDrange: id 0 Invalid\n");
    mslDebugPrintf("ListNodeAllocIDrange: max (%d) is < min (%d)\n");
    mslDebugPrintf("ListNodeAllocIDrange: max 0x%x exceeds pool size (%x)\n");
    mslDebugPrintf("ListNodeAllocIDrange: id %08x exceeds pool size (%x)\n");
    mslDebugPrintf("ListNodeAllocIDrange: id %08x exceeds max (0x%x)\n");
    mslDebugPrintf("ListNodeAllocIDrange (%08x): node not in free list???\n");
}

static void fn_stub2(void) {
    mslDebugPrintf("ListNodeAllocID NULL pool\n");
    mslDebugPrintf("ListNodeAllocID pool was never Attached\n");
    mslDebugPrintf("ListNodeAllocID: id 0 Invalid\n");
    mslDebugPrintf("ListNodeAllocID: id %08x exceeds pool size (%x)\n");
    mslDebugPrintf("ListNodeAllocID: ID %08x is already in use\n");
    mslDebugPrintf("ListNodeAllocID (%08x): node not in free list???\n");
}

_ListNode* ListNodeAlloc(ListPool* pool) {
    _ListNode* node;
    int allocated;

    if (pool == 0) {
        mslDebugPrintf("ListNodeAlloc NULL pool\n");
        return 0;
    }
    if (pool->element_count == 0) {
        mslDebugPrintf("ListNodeAlloc pool was never Attached\n");
        return 0;
    }

    node = pool->free_list;
    if (node != 0) {
        node = ListRemove(&node);
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
        mslDebugPrintf("ListNodeAlloc no more nodes\n");
    }
    return node;
}

static inline void reset_pool(ListPool* pool) {
    _ListNode* node;
    _ListNode* previous = 0;
    char* data;
    u32 index;

    if (pool == 0) {
        mslDebugPrintf("ListPoolReset NULL pool\n");
    } else if (pool->element_count == 0) {
        mslDebugPrintf("ListPoolReset pool was never Attached\n");
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
}

static void fn_stub3(void) {
    mslDebugPrintf("ListPoolDetach discarding %d un-deleted elements\n");
}

int ListPoolAttach(
    ListPool* pool, void* memory, u32 element_count,
    u32 element_size) {
    if (pool == 0) {
        mslDebugPrintf("ListPoolAttach NULL == pool\n");
        return 0;
    }
    if (memory == 0) {
        mslDebugPrintf("ListPoolAttach NULL == mem\n");
        return 0;
    }
    if (element_count == 0) {
        mslDebugPrintf("ListPoolAttach to what?  elts == 0\n");
        return 0;
    }

    mslDebugPrintf(
        "ListPoolAttach: %d elts of size %d at 0x%08x\n", element_count, element_size, memory);
    pool->element_count = element_count;
    pool->element_size = element_size;
    pool->elements = memory;
    pool->nodes = (_ListNode*)(
        (char*)memory + element_count * element_size);

    reset_pool(pool);

    pool->peak_allocated = 0;
    return 1;
}
