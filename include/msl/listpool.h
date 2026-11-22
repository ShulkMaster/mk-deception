#ifndef MSL_LISTPOOL_H
#define MSL_LISTPOOL_H

#include "dolphin/types.h"

typedef struct _ListNode _ListNode;
typedef struct ListPool ListPool;

struct _ListNode {
    void* data;
    _ListNode* next;
    _ListNode** previous_link;
    u16 state : 2;
    u16 index : 14;
    u16 generation;
};

struct ListPool {
    u32 element_size;
    u32 element_count;
    char* elements;
    _ListNode* nodes;
    _ListNode* free_list;
    u32 allocated_count;
    u32 peak_allocated;
};

typedef char ListNodeSizeCheck[sizeof(_ListNode) == 0x10 ? 1 : -1];
typedef char ListPoolSizeCheck[sizeof(ListPool) == 0x1C ? 1 : -1];

extern ListPool g_listPoolSound;
extern ListPool g_listPoolAdjust;

#ifdef __cplusplus
extern "C" {
#endif

void ListInsertAtTail(_ListNode** list, _ListNode* node);
void ListNext(_ListNode** node);
_ListNode* ListRemove(_ListNode** list);
void ListInsert(_ListNode** list, _ListNode* node);
u32 ListNodeID(ListPool* pool, _ListNode* node);
_ListNode* ListNodeFind(ListPool* pool, u32 id);
void* ListNodeData(ListPool* pool, _ListNode* node);
void ListNodeFree(ListPool* pool, _ListNode* node);
_ListNode* ListNodeAlloc(ListPool* pool);
int ListPoolAttach(
    ListPool* pool, void* memory, u32 element_count,
    u32 element_size);

#ifdef __cplusplus
}
#endif

#endif
