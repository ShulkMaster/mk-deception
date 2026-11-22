#ifndef MSL_LISTPOOL_H
#define MSL_LISTPOOL_H

typedef unsigned char msl_u8;
typedef unsigned short msl_u16;
typedef unsigned long msl_u32;

typedef struct _ListNode _ListNode;
typedef struct ListPool ListPool;

struct _ListNode {
    void* data;
    _ListNode* next;
    _ListNode** previous_link;
    msl_u16 state : 2;
    msl_u16 index : 14;
    msl_u16 generation;
};

struct ListPool {
    msl_u32 element_size;
    msl_u32 element_count;
    char* elements;
    _ListNode* nodes;
    _ListNode* free_list;
    msl_u32 allocated_count;
    msl_u32 peak_allocated;
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
msl_u32 ListNodeID(ListPool* pool, _ListNode* node);
_ListNode* ListNodeFind(ListPool* pool, msl_u32 id);
void* ListNodeData(ListPool* pool, _ListNode* node);
void ListNodeFree(ListPool* pool, _ListNode* node);
_ListNode* ListNodeAlloc(ListPool* pool);
int ListPoolAttach(
    ListPool* pool, void* memory, msl_u32 element_count,
    msl_u32 element_size);

#ifdef __cplusplus
}
#endif

#endif
