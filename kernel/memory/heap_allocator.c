/*
 * heap_allocator.c
 *
 * Kernel heap implementation for kmalloc/kfree. Use a simple free list or
 * slab-like allocator for small blocks and a list for larger blocks.
 *
 * Expose `kmalloc(size_t)`, `kfree(void *)`, and `heap_init()`.
 */

#include <stddef.h>

void heap_init(void) {
    // allocate and initialize kernel heap
}

void *kmalloc(size_t size) {
    return 0; // placeholder
}

void kfree(void *p) {
    // placeholder
}
