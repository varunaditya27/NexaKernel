#include <lib/dsa/tree.h>
#include <stddef.h>

/*
 * kernel/fs/dsa_structures/directory_tree.c
 *
 * Directory Hierarchy Wrapper
 *
 * This file adapts the generic N-ary Tree data structure to represent the
 * file system's directory structure. Each node represents a file or directory,
 * allowing for nested folder organization.
 */

static tree_node_t root_node;

void fs_tree_init(void *root_data) {
    tree_node_init(&root_node, root_data);
}

tree_node_t *fs_tree_get_root(void) {
    return &root_node;
}

void fs_tree_add_child(tree_node_t *parent, tree_node_t *child) {
    tree_add_child(parent, child);
}

tree_node_t *fs_tree_find_child(tree_node_t *parent, void *data, int (*comparator)(void *, void *)) {
    return tree_find_child(parent, data, comparator);
}
