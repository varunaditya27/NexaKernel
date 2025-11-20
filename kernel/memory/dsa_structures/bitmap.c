/*
 * bitmap.c
 *
 * Bitmap used for tracking physical memory frames. Provide set/clear/find-first
 * zero functions and ensure thread-safety if used in multi-core contexts.
 */

#include <stdint.h>

void bitmap_init(void) {}
int bitmap_find_free(void) { return -1; }
void bitmap_set(int idx) {}
void bitmap_clear(int idx) {}
