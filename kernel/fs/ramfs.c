/*
 * kernel/fs/ramfs.c
 *
 * RAM File System (RAMFS)
 *
 * This file implements a simple, non-persistent file system that lives entirely
 * in system memory. It uses the kernel's DSA wrappers (Trie, Directory Tree, Hash Map)
 * to manage files and directories efficiently.
 *
 * It serves as the initial root filesystem during boot.
 */

#include "dsa_structures.h"
#include <stddef.h>

void ramfs_init(void) {
    fs_index_init();
    fs_tree_init(NULL); // Root node
    file_table_init(100); // Max 100 open files
}

void create_file(const char *name, void *inode) {
    fs_index_add(name, inode);
    // Add to directory tree (omitted for brevity, requires parsing path)
}

void *open_file(const char *name) {
    void *inode = fs_index_get(name);
    if (inode) {
        file_table_add(name, inode); // Add to open file table
    }
    return inode;
}
