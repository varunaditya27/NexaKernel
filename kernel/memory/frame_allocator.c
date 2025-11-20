/*
 * frame_allocator.c
 *
 * Physical frame allocator. It should manage physical page frames using a
 * bitmap or buddy allocator (see `dsa_structures` for helpers).
 *
 * Expose functions like: `frame_alloc()`, `frame_free()` and `frame_init()`.
 */

#include <stdint.h>

void frame_init(void) {
    // Initialize bitmaps and memory structures
}

uintptr_t frame_alloc(void) {
    return 0;
}

void frame_free(uintptr_t addr) {
    // free a frame
}
