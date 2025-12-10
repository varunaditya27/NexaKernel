/*
 * lib/dsa/tree.c
 *
 * Tree Operations
 *
 * This file implements operations for manipulating N-ary trees (adding/removing
 * children, searching) and provides helper functions for calculating indices
 * in array-based binary trees.
 */

#include "tree.h"

void tree_node_init(tree_node_t *node, void *data) {
    if (!node) return;
    node->data = data;
    node->parent = NULL;
    node->first_child = NULL;
    node->next_sibling = NULL;
}

void tree_add_child(tree_node_t *parent, tree_node_t *child) {
    if (!parent || !child) return;

    child->parent = parent;
    child->next_sibling = parent->first_child;
    parent->first_child = child;
}

void tree_remove_child(tree_node_t *parent, tree_node_t *child) {
    if (!parent || !child) return;

    tree_node_t *current = parent->first_child;
    tree_node_t *prev = NULL;

    while (current) {
        if (current == child) {
            if (prev) {
                prev->next_sibling = current->next_sibling;
            } else {
                parent->first_child = current->next_sibling;
            }
            child->parent = NULL;
            child->next_sibling = NULL;
            return;
        }
        prev = current;
        current = current->next_sibling;
    }
}

tree_node_t *tree_find_child(tree_node_t *parent, void *data, int (*comparator)(void *, void *)) {
    if (!parent || !comparator) return NULL;

    tree_node_t *current = parent->first_child;
    while (current) {
        if (comparator(current->data, data) == 0) {
            return current;
        }
        current = current->next_sibling;
    }
    return NULL;
}

/* --- Binary Tree Helpers --- */

size_t binary_tree_left_child(size_t index) {
    return 2 * index + 1;
}

size_t binary_tree_right_child(size_t index) {
    return 2 * index + 2;
}

size_t binary_tree_parent(size_t index) {
    return (index - 1) / 2;
}

bool binary_tree_is_left_child(size_t index) {
    return index % 2 != 0;
}

bool binary_tree_is_right_child(size_t index) {
    return index % 2 == 0 && index != 0;
}

size_t binary_tree_sibling(size_t index) {
    if (index == 0) return 0;
    return binary_tree_is_left_child(index) ? index + 1 : index - 1;
}
