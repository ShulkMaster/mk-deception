#include "msl/RedBlackTree.h"

/* Retail red-black tree owner used by the MSL external heap. */

#define RBT_REPLACE_PARENT(tree, old_node, new_node) \
    do { \
        if (((new_node)->parent = (old_node)->parent) != 0) { \
            if ((old_node)->parent->left == (old_node)) { \
                (old_node)->parent->left = (new_node); \
            } else { \
                (old_node)->parent->right = (new_node); \
            } \
            (old_node)->parent = 0; \
        } else { \
            (tree)->root = (new_node); \
        } \
    } while (0)

#define RBT_ROTATE_LEFT(tree, top_expr) \
    do { \
        RedBlackNode* rbt_pivot; \
        RedBlackNode* rbt_top; \
        rbt_top = (top_expr); \
        rbt_pivot = rbt_top->right; \
        if ((rbt_top->right = rbt_pivot->left) != 0) { \
            rbt_top->right->parent = rbt_top; \
        } \
        if ((rbt_pivot->parent = rbt_top->parent) != 0) { \
            if (rbt_top == rbt_top->parent->left) { \
                rbt_top->parent->left = rbt_pivot; \
            } else { \
                rbt_top->parent->right = rbt_pivot; \
            } \
        } else { \
            (tree)->root = rbt_pivot; \
        } \
        rbt_pivot->left = rbt_top; \
        rbt_top->parent = rbt_pivot; \
    } while (0)

#define RBT_ROTATE_RIGHT(tree, top_expr) \
    do { \
        RedBlackNode* rbt_pivot; \
        RedBlackNode* rbt_top; \
        rbt_top = (top_expr); \
        rbt_pivot = rbt_top->left; \
        if ((rbt_top->left = rbt_pivot->right) != 0) { \
            rbt_top->left->parent = rbt_top; \
        } \
        if ((rbt_pivot->parent = rbt_top->parent) != 0) { \
            if (rbt_top == rbt_top->parent->left) { \
                rbt_top->parent->left = rbt_pivot; \
            } else { \
                rbt_top->parent->right = rbt_pivot; \
            } \
        } else { \
            (tree)->root = rbt_pivot; \
        } \
        rbt_pivot->right = rbt_top; \
        rbt_top->parent = rbt_pivot; \
    } while (0)

int RBTK_GetPrevNextToKey(
    RedBlackTree* tree, const void* key,
    RedBlackNode** previous, RedBlackNode** next) {
    RedBlackNode* node = tree->root;
    RedBlackNode* lower;
    RedBlackNode* upper;

    upper = 0;
    lower = 0;

    if (node != 0) {
        do {
            int comparison = tree->compare_key(key, node);

            if (comparison > 0) {
                lower = node;
                node = node->right;
            } else {
                upper = node;
                node = node->left;
            }
        } while (node != 0);
    }
    *previous = lower;
    *next = upper;
    return 1;
}

/* TODO: [near miss] 93.82%; search loop and exits agree; retail returns both the found node and the
 * null exit through r31 (li r31,0, one final mr r3), ours writes r3 on each exit. */
RedBlackNode* RBTK_FindQuickNodeEqualToKey(
    RedBlackTree* tree, const void* key) {
    RedBlackNode* node = tree->root;

    while (node != 0) {
        int comparison = tree->compare_key(key, node);

        if (comparison > 0) {
            node = node->right;
        } else if (comparison < 0) {
            node = node->left;
        } else {
            return node;
        }
    }
    return 0;
}

RedBlackNode* RBT_RemoveNode(
    RedBlackTree* tree, RedBlackNode* removed) {
    RedBlackNode* fixup;
    RedBlackNode* removed_parent = removed->parent;

    if (removed_parent == 0 && tree->root != removed) {
        return 0;
    }

    {
        RedBlackNode* child = removed->left;

        if (child != 0) {
            if (removed->right != 0) {
                RedBlackNode* successor = removed->right;

                while (successor->left != 0) {
                    successor = successor->left;
                }

                if (successor->right != 0) {
                    RedBlackNode* successor_child = successor->right;

                    successor_child->black = 1;
                    successor->right->parent = successor->parent;
                    if (successor == removed->right) {
                        successor->parent->right = successor->right;
                    } else {
                        successor->parent->left = successor->right;
                    }

                    RBT_REPLACE_PARENT(tree, removed, successor);
                    successor->left = removed->left;
                    successor->left->parent = successor;
                    successor->right = removed->right;
                    successor->right->parent = successor;
                    successor->black = removed->black;
                    return removed;
                }

                if (successor->black == 0) {
                    if (successor != removed->right) {
                        successor->parent->left = 0;
                    }

                    RBT_REPLACE_PARENT(tree, removed, successor);
                    successor->left = removed->left;
                    successor->left->parent = successor;
                    if (successor == removed->right) {
                        successor->right = 0;
                    } else {
                        successor->right = removed->right;
                        successor->right->parent = successor;
                    }
                    successor->black = removed->black;
                    return removed;
                }

                {
                    RedBlackNode* successor_parent;

                    successor->black = removed->black;
                    removed->black = 1;
                    successor->left = removed->left;
                    successor->left->parent = successor;
                    successor_parent = successor->parent;

                    if (removed->right == successor) {
                        successor->right = removed;
                    } else {
                        successor_parent->left = removed;
                        successor->right = removed->right;
                        successor->right->parent = successor;
                    }

                    removed->right = 0;
                    removed->left = 0;
                    if ((successor->parent = removed->parent) != 0) {
                        if (removed->parent->left == removed) {
                            removed->parent->left = successor;
                        } else {
                            removed->parent->right = successor;
                        }
                    } else {
                        tree->root = successor;
                    }

                    if (removed == successor_parent) {
                        removed->parent = successor;
                    } else {
                        removed->parent = successor_parent;
                    }
                }
            } else {
                child->black = 1;
                if ((removed->left->parent = removed->parent) != 0) {
                    if (removed->parent->left == removed) {
                        removed->parent->left = removed->left;
                    } else {
                        removed->parent->right = removed->left;
                    }
                    removed->parent = 0;
                } else {
                    tree->root = removed->left;
                }
                return removed;
            }
        } else {
            child = removed->right;

            if (child != 0) {
                child->black = 1;
                if ((removed->right->parent = removed->parent) != 0) {
                    if (removed->parent->left == removed) {
                        removed->parent->left = removed->right;
                    } else {
                        removed->parent->right = removed->right;
                    }
                    removed->parent = 0;
                } else {
                    tree->root = removed->right;
                }
                return removed;
            }
            if (removed_parent == 0) {
                tree->root = 0;
                return removed;
            }
            if (removed->black == 0) {
                if (removed_parent->left == removed) {
                    removed_parent->left = 0;
                } else {
                    removed_parent->right = 0;
                }
                removed->parent = 0;
                return removed;
            }
        }
    }

    fixup = removed;
    do {
        RedBlackNode* parent = fixup->parent;
        RedBlackNode* sibling;

        if (parent->left == fixup) {
            sibling = parent->right;
            if (sibling->black == 0) {
                sibling->black = 1;
                fixup->parent->black = 0;
                RBT_ROTATE_LEFT(tree, fixup->parent);
                sibling = fixup->parent->right;
            }

            if (sibling->left != 0 &&
                sibling->left->black == 0 &&
                (sibling->right == 0 ||
                 sibling->right->black == 1)) {
                sibling->left->black = 1;
                sibling->black = 0;
                RBT_ROTATE_RIGHT(tree, sibling);
                sibling = sibling->parent;
            }

            if (sibling->right != 0 &&
                sibling->right->black == 0) {
                sibling->black = fixup->parent->black;
                fixup->parent->black = 1;
                sibling->right->black = 1;
                RBT_ROTATE_LEFT(tree, fixup->parent);
                fixup = tree->root;
            } else {
                sibling->black = 0;
                fixup = fixup->parent;
            }
        } else {
            sibling = parent->left;
            if (sibling->black == 0) {
                sibling->black = 1;
                fixup->parent->black = 0;
                RBT_ROTATE_RIGHT(tree, fixup->parent);
                sibling = fixup->parent->left;
            }

            if (sibling->right != 0 &&
                sibling->right->black == 0 &&
                (sibling->left == 0 ||
                 sibling->left->black == 1)) {
                sibling->right->black = 1;
                sibling->black = 0;
                RBT_ROTATE_LEFT(tree, sibling);
                sibling = sibling->parent;
            }

            if (sibling->left != 0 &&
                sibling->left->black == 0) {
                sibling->black = fixup->parent->black;
                fixup->parent->black = 1;
                sibling->left->black = 1;
                RBT_ROTATE_RIGHT(tree, fixup->parent);
                fixup = tree->root;
            } else {
                sibling->black = 0;
                fixup = fixup->parent;
            }
        }
    } while (fixup->black == 1 && fixup->parent != 0);

    fixup->black = 1;
    if (removed->parent->left == removed) {
        removed->parent->left = 0;
    } else {
        removed->parent->right = 0;
    }
    removed->parent = 0;
    return removed;
}

void RBT_InsertNode(RedBlackTree* tree, RedBlackNode* node) {
    RedBlackNode* current;

    node->left = 0;
    node->right = 0;
    current = tree->root;
    if (current != 0) {
        do {
            if (tree->compare_nodes(node, current) > 0) {
                if (current->right != 0) {
                    current = current->right;
                } else {
                    node->parent = current;
                    current->right = node;
                    current = 0;
                }
            } else {
                if (current->left != 0) {
                    current = current->left;
                } else {
                    node->parent = current;
                    current->left = node;
                    current = 0;
                }
            }
        } while (current != 0);
        _RBT_InsertFixup(tree, node);
    } else {
        node->parent = 0;
        node->black = 1;
        tree->root = node;
    }
}

void _RBT_InsertFixup(
    RedBlackTree* tree, RedBlackNode* node) {
    node->black = 0;
    while (node->parent != 0 && node->parent->black == 0) {
        RedBlackNode* parent = node->parent;
        RedBlackNode* grandparent = parent->parent;
        RedBlackNode* left = grandparent->left;

        if (parent == left) {
            RedBlackNode* uncle = grandparent->right;

            if (uncle != 0 && uncle->black == 0) {
                uncle->black = 1;
                node->parent->black = 1;
                node = node->parent->parent;
                node->black = 0;
            } else {
                if (node == parent->right) {
                    RedBlackNode* pivot = parent->right;

                    node = parent;
                    if ((parent->right = pivot->left) != 0) {
                        parent->right->parent = parent;
                    }
                    if ((pivot->parent = parent->parent) != 0) {
                        if (parent == parent->parent->left) {
                            parent->parent->left = pivot;
                        } else {
                            parent->parent->right = pivot;
                        }
                    } else {
                        tree->root = pivot;
                    }
                    pivot->left = parent;
                    parent->parent = pivot;
                }

                node->parent->black = 1;
                node->parent->parent->black = 0;
                RBT_ROTATE_RIGHT(tree, node->parent->parent);
            }
        } else {
            RedBlackNode* uncle = left;

            if (uncle != 0 && uncle->black == 0) {
                uncle->black = 1;
                node->parent->black = 1;
                node = node->parent->parent;
                node->black = 0;
            } else {
                if (node == parent->left) {
                    RedBlackNode* pivot = parent->left;

                    node = parent;
                    if ((parent->left = pivot->right) != 0) {
                        parent->left->parent = parent;
                    }
                    if ((pivot->parent = parent->parent) != 0) {
                        if (parent == parent->parent->left) {
                            parent->parent->left = pivot;
                        } else {
                            parent->parent->right = pivot;
                        }
                    } else {
                        tree->root = pivot;
                    }
                    pivot->right = parent;
                    parent->parent = pivot;
                }

                node->parent->black = 1;
                node->parent->parent->black = 0;
                RBT_ROTATE_LEFT(tree, node->parent->parent);
            }
        }
    }
    tree->root->black = 1;
}

RedBlackNode* RBN_GetNextNode(RedBlackNode* node) {
    RedBlackNode* child;
    RedBlackNode* next;

    next = node->right;
    if (next != 0) {
        child = next->left;
        node = next;
        while (child != 0) {
            node = child;
            child = child->left;
        }
        return node;
    }

    next = node->parent;
    while (next != 0 && node == next->right) {
        node = next;
        next = next->parent;
    }
    return next;
}

RedBlackNode* RBN_GetPreviousNode(RedBlackNode* node) {
    RedBlackNode* child;
    RedBlackNode* previous;

    previous = node->left;
    if (previous != 0) {
        child = previous->right;
        node = previous;
        while (child != 0) {
            node = child;
            child = child->right;
        }
        return node;
    }

    previous = node->parent;
    while (previous != 0 && node == previous->left) {
        node = previous;
        previous = previous->parent;
    }
    return previous;
}
