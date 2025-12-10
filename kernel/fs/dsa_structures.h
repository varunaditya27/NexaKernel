#ifndef NEXA_FS_DSA_H
#define NEXA_FS_DSA_H

/*
 * kernel/fs/dsa_structures.h
 *
 * File System DSA Wrappers
 *
 * This header declares the specific data structure wrappers used by the file system.
 * It adapts generic DSA implementations (Trie, Tree, HashMap) for FS-specific use cases
 * like file indexing, directory hierarchy, and the open file table.
 */

#include <stddef.h>
#include <stdbool.h>
#include <lib/dsa/tree.h>

/* Trie Wrapper */
void fs_index_init(void);
bool fs_index_add(const char *filename, void *inode);
void *fs_index_get(const char *filename);
bool fs_index_remove(const char *filename);

/* Directory Tree Wrapper */
void fs_tree_init(void *root_data);
tree_node_t *fs_tree_get_root(void);
void fs_tree_add_child(tree_node_t *parent, tree_node_t *child);
tree_node_t *fs_tree_find_child(tree_node_t *parent, void *data, int (*comparator)(void *, void *));

/* Hash Map Wrapper */
bool file_table_init(size_t max_files);
bool file_table_add(const char *path, void *file_struct);
void *file_table_get(const char *path);
void file_table_remove(const char *path);

#endif /* NEXA_FS_DSA_H */
