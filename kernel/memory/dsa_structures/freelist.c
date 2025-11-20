/*
 * freelist.c
 *
 * Simple free-list allocator to be used by the kernel `kmalloc` implementation.
 * Provide block splits and coalescing to limit fragmentation.
 */

#include <stdint.h>

void freelist_init(void) {}
void *freelist_malloc(size_t s) { return 0; }
void freelist_free(void *p) {}
