#include <lib/dsa/trie.h>
#include <stddef.h>

/*
 * kernel/fs/dsa_structures/trie.c
 *
 * File Indexing Wrapper
 *
 * This file adapts the generic Trie (Prefix Tree) data structure for fast file
 * indexing. It allows for efficient prefix-based searches and autocomplete-style
 * lookups for filenames within the file system.
 */

static trie_t file_index;

void fs_index_init(void) {
    trie_init(&file_index);
}

bool fs_index_add(const char *filename, void *inode) {
    return trie_insert(&file_index, filename, inode);
}

void *fs_index_get(const char *filename) {
    return trie_search(&file_index, filename);
}

bool fs_index_remove(const char *filename) {
    return trie_remove(&file_index, filename);
}
