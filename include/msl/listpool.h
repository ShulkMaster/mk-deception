#ifndef MSL_LISTPOOL_H
#define MSL_LISTPOOL_H

typedef unsigned char msl_u8;
typedef unsigned short msl_u16;
typedef unsigned long msl_u32;

typedef struct _ListNode _ListNode;
typedef struct ListPool ListPool;

struct _ListNode {
    void* data;                   /* +0x00 */
    _ListNode* next;              /* +0x04 */
    _ListNode** previous_link;    /* +0x08 */
    msl_u16 state : 2;            /* +0x0C, high two bits */
    msl_u16 index : 14;           /* +0x0C, low fourteen bits */
    msl_u16 generation;           /* +0x0E */
}; /* 0x10 */

struct ListPool {
    msl_u32 element_size;         /* +0x00 */
    msl_u32 element_count;        /* +0x04 */
    char* elements;               /* +0x08 */
    _ListNode* nodes;             /* +0x0C */
    _ListNode* free_list;         /* +0x10 */
    msl_u32 allocated_count;      /* +0x14 */
    msl_u32 peak_allocated;       /* +0x18 */
}; /* 0x1C */

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
