/*
 * lib/dsa/bitmap.c
 *
 * Bitmap Implementation
 *
 * This file implements bitmap operations including setting, clearing, and testing bits,
 * as well as searching for the first set or zero bit (useful for allocation).
 */

#include "bitmap.h"
#include <lib/cstd/memory.h> // For memset if available

// Assuming memset is available or we implement a simple one
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

bool bitmap_init(bitmap_t *bitmap, size_t size_bits, void *buffer) {
    if (!bitmap || !buffer) return false;

    bitmap->buffer = (uint8_t *)buffer;
    bitmap->size_bits = size_bits;
    bitmap->size_bytes = (size_bits + 7) / 8;

    // Initialize to zero
    memset(bitmap->buffer, 0, bitmap->size_bytes);

    return true;
}

void bitmap_set(bitmap_t *bitmap, size_t index) {
    if (index >= bitmap->size_bits) return;
    bitmap->buffer[index / 8] |= (1 << (index % 8));
}

void bitmap_clear(bitmap_t *bitmap, size_t index) {
    if (index >= bitmap->size_bits) return;
    bitmap->buffer[index / 8] &= ~(1 << (index % 8));
}

bool bitmap_test(bitmap_t *bitmap, size_t index) {
    if (index >= bitmap->size_bits) return false;
    return (bitmap->buffer[index / 8] & (1 << (index % 8))) != 0;
}

int64_t bitmap_find_first_zero(bitmap_t *bitmap) {
    for (size_t i = 0; i < bitmap->size_bytes; i++) {
        if (bitmap->buffer[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (!((bitmap->buffer[i] >> j) & 1)) {
                    size_t bit_index = i * 8 + j;
                    if (bit_index < bitmap->size_bits) {
                        return (int64_t)bit_index;
                    }
                }
            }
        }
    }
    return -1;
}

int64_t bitmap_find_first_set(bitmap_t *bitmap) {
    for (size_t i = 0; i < bitmap->size_bytes; i++) {
        if (bitmap->buffer[i] != 0) {
            for (int j = 0; j < 8; j++) {
                if ((bitmap->buffer[i] >> j) & 1) {
                    size_t bit_index = i * 8 + j;
                    if (bit_index < bitmap->size_bits) {
                        return (int64_t)bit_index;
                    }
                }
            }
        }
    }
    return -1;
}
