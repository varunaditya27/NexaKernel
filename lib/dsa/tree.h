#ifndef NEXA_TREE_H
#define NEXA_TREE_H

#include <stddef.h>
#include <stdbool.h>
#include "list.h"

/*
 * lib/dsa/tree.h
 *
 * Generic Tree Definitions
 *
 * This header provides structures and function prototypes for:
 * 1. N-ary Trees: Used for hierarchical data like file systems.
 * 2. Binary Tree Helpers: Mathematical helpers for array-based binary trees
 *    (used in heaps and buddy allocators).
 */

/* --- N-ary Tree Node --- */
typedef struct tree_node {
    void *data;
    struct tree_node *parent;
    struct tree_node *first_child;
    struct tree_node *next_sibling;
} tree_node_t;

/* Initialize a tree node */
void tree_node_init(tree_node_t *node, void *data);

/* Add a child to a node */
void tree_add_child(tree_node_t *parent, tree_node_t *child);

/* Remove a child from a node */
void tree_remove_child(tree_node_t *parent, tree_node_t *child);

/* Find a child by data (requires comparator) */
tree_node_t *tree_find_child(tree_node_t *parent, void *data, int (*comparator)(void *, void *));

/* --- Binary Tree Helpers (Array-based for Buddy System) --- */

/* Get left child index */
size_t binary_tree_left_child(size_t index);

/* Get right child index */
size_t binary_tree_right_child(size_t index);

/* Get parent index */
size_t binary_tree_parent(size_t index);

/* Check if node is left child */
bool binary_tree_is_left_child(size_t index);

/* Check if node is right child */
bool binary_tree_is_right_child(size_t index);

/* Get sibling index */
size_t binary_tree_sibling(size_t index);

#endif /* NEXA_TREE_H */
