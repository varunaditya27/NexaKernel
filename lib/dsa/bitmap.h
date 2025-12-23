#ifndef NEXA_BITMAP_H
#define NEXA_BITMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * lib/dsa/bitmap.h
 *
 * Generic Bitmap Interface
 *
 * This header defines the API for a generic bitmap data structure. Bitmaps are
 * space-efficient structures used to track the state (used/free) of resources,
 * such as physical memory frames or disk blocks.
 */

typedef struct bitmap {
    uint8_t *buffer;
    size_t size_bits;
    size_t size_bytes;
} bitmap_t;

/* Initialize a bitmap */
bool bitmap_init(bitmap_t *bitmap, size_t size_bits, void *buffer);

/* Set a bit */
void bitmap_set(bitmap_t *bitmap, size_t index);

/* Clear a bit */
void bitmap_clear(bitmap_t *bitmap, size_t index);

/* Check if a bit is set */
bool bitmap_test(bitmap_t *bitmap, size_t index);

/* Find the first free bit (returns -1 if full) */
int64_t bitmap_find_first_zero(bitmap_t *bitmap);

/* Find the first set bit */
int64_t bitmap_find_first_set(bitmap_t *bitmap);

#endif /* NEXA_BITMAP_H */
