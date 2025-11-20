/*
 * kernel/fs/ramfs.c
 *
 * Simple in-memory file system implementation. Use a trie for filename lookup
 * and a directory tree for hierarchical organization. Keep file content in
 * dynamically allocated RAM buffers and use the heap allocator.
 */

#include <stdint.h>

void ramfs_init(void) {
    // initialize RAM FS global structures
}
