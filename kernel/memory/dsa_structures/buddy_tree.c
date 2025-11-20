/*
 * buddy_tree.c
 *
 * Buddy allocator (optional) for frame allocation. Use this only if required
 * by the project. The interface should mirror a typical free-size buddy allocator.
 */

#include <stdint.h>

void buddy_init(void) {}
void *buddy_alloc(size_t size) { return 0; }
void buddy_free(void *p) {}
