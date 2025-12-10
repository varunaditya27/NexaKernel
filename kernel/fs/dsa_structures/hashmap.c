#include <lib/dsa/hashmap.h>
#include <stddef.h>

/*
 * kernel/fs/dsa_structures/hashmap.c
 *
 * Open File Table Wrapper
 *
 * This file adapts the generic Hash Map data structure to manage the table of
 * currently open files. It allows for O(1) average time complexity when looking
 * up file descriptors or file handles by path.
 */

static hashmap_t open_file_table;

bool file_table_init(size_t max_files) {
    return hashmap_init(&open_file_table, max_files);
}

bool file_table_add(const char *path, void *file_struct) {
    return hashmap_put(&open_file_table, path, file_struct);
}

void *file_table_get(const char *path) {
    return hashmap_get(&open_file_table, path);
}

void file_table_remove(const char *path) {
    hashmap_remove(&open_file_table, path);
}
