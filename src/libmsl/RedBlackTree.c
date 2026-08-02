#include "msl/RedBlackTree.h"

/*
 * Retail red-black tree owner used by the MSL external heap.
 * Soft ceilings: key find 91.18%, remove 53.74%, insert 66.38%, and insert
 * fixup 68.05%. Key bounds and next/previous are exact.
 * All search, splice, sentinel, mirrored rotation, recolor, and root-black
 * algorithms are reconstructed; remaining differences are structured-C
 * scheduling and register allocation.
 */

#define RBT_REPLACE_PARENT(tree, old_node, new_node) \
    do { \
        (new_node)->parent = (old_node)->parent; \
        if ((old_node)->parent == 0) { \
            (tree)->root = (new_node); \
        } else if ((old_node) == (old_node)->parent->left) { \
            (old_node)->parent->left = (new_node); \
        } else { \
            (old_node)->parent->right = (new_node); \
        } \
    } while (0)

#define RBT_ROTATE_LEFT(tree, node) \
    do { \
        RedBlackNode* rbt_pivot = (node)->right; \
        (node)->right = rbt_pivot->left; \
        if ((node)->right != 0) { \
            (node)->right->parent = (node); \
        } \
        rbt_pivot->parent = (node)->parent; \
        if ((node)->parent == 0) { \
            (tree)->root = rbt_pivot; \
        } else if ((node) == (node)->parent->left) { \
            (node)->parent->left = rbt_pivot; \
        } else { \
            (node)->parent->right = rbt_pivot; \
        } \
        rbt_pivot->left = (node); \
        (node)->parent = rbt_pivot; \
    } while (0)

#define RBT_ROTATE_RIGHT(tree, node) \
    do { \
        RedBlackNode* rbt_pivot = (node)->left; \
        (node)->left = rbt_pivot->right; \
        if ((node)->left != 0) { \
            (node)->left->parent = (node); \
        } \
        rbt_pivot->parent = (node)->parent; \
        if ((node)->parent == 0) { \
            (tree)->root = rbt_pivot; \
        } else if ((node) == (node)->parent->left) { \
            (node)->parent->left = rbt_pivot; \
        } else { \
            (node)->parent->right = rbt_pivot; \
        } \
        rbt_pivot->right = (node); \
        (node)->parent = rbt_pivot; \
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
            break;
        }
    }
    return node;
}

RedBlackNode* RBT_RemoveNode(
    RedBlackTree* tree, RedBlackNode* removed) {
    RedBlackNode* fixup = removed;

    if (removed->parent == 0 && tree->root != removed) {
        return 0;
    }

    if (removed->left == 0) {
        if (removed->right != 0) {
            RedBlackNode* child = removed->right;

            child->black = 1;
            child->parent = removed->parent;
            if (removed->parent == 0) {
                tree->root = child;
            } else {
                if (removed == removed->parent->left) {
                    removed->parent->left = child;
                } else {
                    removed->parent->right = child;
                }
                removed->parent = 0;
            }
            return removed;
        }
        if (removed->parent == 0) {
            tree->root = 0;
            return removed;
        }
        if (removed->black == 0) {
            if (removed == removed->parent->left) {
                removed->parent->left = 0;
            } else {
                removed->parent->right = 0;
            }
            removed->parent = 0;
            return removed;
        }
    } else {
        RedBlackNode* successor = removed->right;

        if (successor == 0) {
            RedBlackNode* child = removed->left;

            child->black = 1;
            child->parent = removed->parent;
            if (removed->parent == 0) {
                tree->root = child;
            } else {
                if (removed == removed->parent->left) {
                    removed->parent->left = child;
                } else {
                    removed->parent->right = child;
                }
                removed->parent = 0;
            }
            return removed;
        }

        while (successor->left != 0) {
            successor = successor->left;
        }

        if (successor->right != 0) {
            RedBlackNode* child = successor->right;

            child->black = 1;
            child->parent = successor->parent;
            if (successor == removed->right) {
                successor->parent->right = child;
            } else {
                successor->parent->left = child;
            }

            RBT_REPLACE_PARENT(tree, removed, successor);
            removed->parent = 0;
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
            removed->parent = 0;
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
            RedBlackNode* successor_parent = successor->parent;
            RedBlackNode* removed_parent = removed->parent;

            successor->black = removed->black;
            removed->black = 1;
            successor->left = removed->left;
            successor->left->parent = successor;

            if (removed->right == successor) {
                successor->right = removed;
            } else {
                successor_parent->left = removed;
                successor->right = removed->right;
                successor->right->parent = successor;
            }

            removed->right = 0;
            removed->left = 0;
            successor->parent = removed_parent;
            if (removed_parent == 0) {
                tree->root = successor;
            } else if (removed == removed_parent->left) {
                removed_parent->left = successor;
            } else {
                removed_parent->right = successor;
            }

            if (removed == successor_parent) {
                removed->parent = successor;
            } else {
                removed->parent = successor_parent;
            }
        }
    }

    do {
        RedBlackNode* parent = fixup->parent;
        RedBlackNode* sibling;

        if (fixup == parent->left) {
            sibling = parent->right;
            if (sibling->black == 0) {
                sibling->black = 1;
                parent->black = 0;
                RBT_ROTATE_LEFT(tree, parent);
                sibling = fixup->parent->right;
            }

            if (sibling->left != 0 &&
                sibling->left->black == 0 &&
                (sibling->right == 0 ||
                 sibling->right->black == 1)) {
                sibling->left->black = 1;
                sibling->black = 0;
                RBT_ROTATE_RIGHT(tree, sibling);
                sibling = fixup->parent->right;
            }

            if (sibling->right == 0 ||
                sibling->right->black != 0) {
                sibling->black = 0;
                fixup = fixup->parent;
            } else {
                sibling->black = fixup->parent->black;
                fixup->parent->black = 1;
                sibling->right->black = 1;
                parent = fixup->parent;
                RBT_ROTATE_LEFT(tree, parent);
                fixup = tree->root;
            }
        } else {
            sibling = parent->left;
            if (sibling->black == 0) {
                sibling->black = 1;
                parent->black = 0;
                RBT_ROTATE_RIGHT(tree, parent);
                sibling = fixup->parent->left;
            }

            if (sibling->right != 0 &&
                sibling->right->black == 0 &&
                (sibling->left == 0 ||
                 sibling->left->black == 1)) {
                sibling->right->black = 1;
                sibling->black = 0;
                RBT_ROTATE_LEFT(tree, sibling);
                sibling = fixup->parent->left;
            }

            if (sibling->left == 0 ||
                sibling->left->black != 0) {
                sibling->black = 0;
                fixup = fixup->parent;
            } else {
                sibling->black = fixup->parent->black;
                fixup->parent->black = 1;
                sibling->left->black = 1;
                parent = fixup->parent;
                RBT_ROTATE_RIGHT(tree, parent);
                fixup = tree->root;
            }
        }
    } while (fixup->black == 1 && fixup->parent != 0);

    fixup->black = 1;
    if (removed == removed->parent->left) {
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
    if (current == 0) {
        node->parent = 0;
        node->black = 1;
        tree->root = node;
    } else {
        do {
            if (tree->compare_nodes(node, current) <= 0) {
                if (current->left == 0) {
                    node->parent = current;
                    current->left = node;
                    current = 0;
                } else {
                    current = current->left;
                }
            } else {
                if (current->right == 0) {
                    node->parent = current;
                    current->right = node;
                    current = 0;
                } else {
                    current = current->right;
                }
            }
        } while (current != 0);
        _RBT_InsertFixup(tree, node);
    }
}

void _RBT_InsertFixup(
    RedBlackTree* tree, RedBlackNode* node) {
    node->black = 0;
    while (node->parent != 0 && node->parent->black == 0) {
        RedBlackNode* parent = node->parent;
        RedBlackNode* grandparent = parent->parent;

        if (parent == grandparent->left) {
            RedBlackNode* uncle = grandparent->right;

            if (uncle != 0 && uncle->black == 0) {
                uncle->black = 1;
                parent->black = 1;
                node = grandparent;
                node->black = 0;
            } else {
                if (node == parent->right) {
                    RedBlackNode* pivot = parent->right;

                    parent->right = pivot->left;
                    if (parent->right != 0) {
                        parent->right->parent = parent;
                    }
                    pivot->parent = parent->parent;
                    if (parent->parent == 0) {
                        tree->root = pivot;
                    } else if (
                        parent == parent->parent->left) {
                        parent->parent->left = pivot;
                    } else {
                        parent->parent->right = pivot;
                    }
                    pivot->left = parent;
                    parent->parent = pivot;
                    node = parent;
                    parent = node->parent;
                    grandparent = parent->parent;
                }

                parent->black = 1;
                grandparent->black = 0;
                {
                    RedBlackNode* pivot = grandparent->left;

                    grandparent->left = pivot->right;
                    if (grandparent->left != 0) {
                        grandparent->left->parent = grandparent;
                    }
                    pivot->parent = grandparent->parent;
                    if (grandparent->parent == 0) {
                        tree->root = pivot;
                    } else if (
                        grandparent ==
                        grandparent->parent->left) {
                        grandparent->parent->left = pivot;
                    } else {
                        grandparent->parent->right = pivot;
                    }
                    pivot->right = grandparent;
                    grandparent->parent = pivot;
                }
            }
        } else {
            RedBlackNode* uncle = grandparent->left;

            if (uncle != 0 && uncle->black == 0) {
                uncle->black = 1;
                parent->black = 1;
                node = grandparent;
                node->black = 0;
            } else {
                if (node == parent->left) {
                    RedBlackNode* pivot = parent->left;

                    parent->left = pivot->right;
                    if (parent->left != 0) {
                        parent->left->parent = parent;
                    }
                    pivot->parent = parent->parent;
                    if (parent->parent == 0) {
                        tree->root = pivot;
                    } else if (
                        parent == parent->parent->left) {
                        parent->parent->left = pivot;
                    } else {
                        parent->parent->right = pivot;
                    }
                    pivot->right = parent;
                    parent->parent = pivot;
                    node = parent;
                    parent = node->parent;
                    grandparent = parent->parent;
                }

                parent->black = 1;
                grandparent->black = 0;
                {
                    RedBlackNode* pivot = grandparent->right;

                    grandparent->right = pivot->left;
                    if (grandparent->right != 0) {
                        grandparent->right->parent = grandparent;
                    }
                    pivot->parent = grandparent->parent;
                    if (grandparent->parent == 0) {
                        tree->root = pivot;
                    } else if (
                        grandparent ==
                        grandparent->parent->left) {
                        grandparent->parent->left = pivot;
                    } else {
                        grandparent->parent->right = pivot;
                    }
                    pivot->left = grandparent;
                    grandparent->parent = pivot;
                }
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
