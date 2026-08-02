#ifndef MSL_RED_BLACK_TREE_H
#define MSL_RED_BLACK_TREE_H

typedef struct RedBlackNode {
    struct RedBlackNode* parent; /* +0x00 */
    struct RedBlackNode* left;   /* +0x04 */
    struct RedBlackNode* right;  /* +0x08 */
    unsigned char black;         /* +0x0C */
    unsigned char pad0D[3];
} RedBlackNode;

typedef char RedBlackNodeSize[
    sizeof(RedBlackNode) == 0x10 ? 1 : -1];

typedef int (*RedBlackNodeCompare)(
    const RedBlackNode* node, const RedBlackNode* existing);
typedef int (*RedBlackKeyCompare)(
    const void* key, const RedBlackNode* node);

typedef struct RedBlackTree {
    RedBlackNode* root;          /* +0x00 */
    RedBlackNodeCompare compare_nodes; /* +0x04 */
    RedBlackKeyCompare compare_key;    /* +0x08 */
} RedBlackTree;

typedef char RedBlackTreeSize[
    sizeof(RedBlackTree) == 0x0C ? 1 : -1];

int RBTK_GetPrevNextToKey(
    RedBlackTree* tree, const void* key,
    RedBlackNode** previous, RedBlackNode** next);
RedBlackNode* RBTK_FindQuickNodeEqualToKey(
    RedBlackTree* tree, const void* key);
RedBlackNode* RBT_RemoveNode(
    RedBlackTree* tree, RedBlackNode* node);
void RBT_InsertNode(RedBlackTree* tree, RedBlackNode* node);
void _RBT_InsertFixup(RedBlackTree* tree, RedBlackNode* node);
RedBlackNode* RBN_GetNextNode(RedBlackNode* node);
RedBlackNode* RBN_GetPreviousNode(RedBlackNode* node);

#endif
