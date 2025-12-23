/*
 * ===========================================================================
 * lib/dsa/bitmap.c
 * ===========================================================================
 *
 * Generic Bitmap Data Structure Implementation
 *
 * A bitmap (also called a bit array or bit vector) is a space-efficient data
 * structure that stores boolean values as individual bits. Each bit represents
 * the state of a resource (e.g., 0 = free, 1 = used).
 *
 * This implementation is used by the physical frame allocator to track which
 * memory frames are available and which are in use. For N frames, we need
 * only N/8 bytes of storage.
 *
 * Time Complexity:
 *   - set/clear/test: O(1)
 *   - find_first_zero/set: O(N/8) worst case, where N is number of bits
 *
 * ===========================================================================
 */

#include "bitmap.h"

/* ---------------------------------------------------------------------------
 * Local helper: memset implementation
 * ---------------------------------------------------------------------------
 * We provide our own memset since we're in a freestanding environment
 * without standard library support.
 * --------------------------------------------------------------------------- */
static void *bitmap_memset(void *dest, int value, size_t count)
{
    unsigned char *ptr = (unsigned char *)dest;
    unsigned char byte_val = (unsigned char)value;
    
    while (count--) {
        *ptr++ = byte_val;
    }
    
    return dest;
}

/* ---------------------------------------------------------------------------
 * bitmap_init - Initialize a bitmap structure
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap    - Pointer to bitmap structure to initialize
 *   size_bits - Number of bits the bitmap should track
 *   buffer    - Pre-allocated memory buffer for the bitmap data
 *
 * Returns:
 *   true on success, false if invalid parameters
 *
 * Notes:
 *   - The buffer must be at least (size_bits + 7) / 8 bytes
 *   - All bits are initialized to 0 (free/available)
 *   - The caller is responsible for providing valid buffer memory
 * --------------------------------------------------------------------------- */
bool bitmap_init(bitmap_t *bitmap, size_t size_bits, void *buffer)
{
    /* Validate input parameters */
    if (!bitmap || !buffer || size_bits == 0) {
        return false;
    }

    /* Initialize bitmap metadata */
    bitmap->buffer = (uint8_t *)buffer;
    bitmap->size_bits = size_bits;
    bitmap->size_bytes = (size_bits + 7) / 8;  /* Ceiling division */

    /* Clear all bits to zero (all resources initially free) */
    bitmap_memset(bitmap->buffer, 0, bitmap->size_bytes);

    return true;
}

/* ---------------------------------------------------------------------------
 * bitmap_set - Set a bit to 1 (mark resource as used)
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *   index  - Bit index to set (0-based)
 *
 * Notes:
 *   - Out-of-bounds indices are silently ignored for safety
 *   - Uses bitwise OR to set without affecting other bits in the byte
 * --------------------------------------------------------------------------- */
void bitmap_set(bitmap_t *bitmap, size_t index)
{
    /* Bounds check - silently ignore invalid indices */
    if (!bitmap || index >= bitmap->size_bits) {
        return;
    }
    
    /*
     * Set the bit at position (index % 8) in byte (index / 8)
     * Example: index=13 -> byte[1] |= (1 << 5) -> sets bit 5 of byte 1
     */
    bitmap->buffer[index / 8] |= (1 << (index % 8));
}

/* ---------------------------------------------------------------------------
 * bitmap_clear - Clear a bit to 0 (mark resource as free)
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *   index  - Bit index to clear (0-based)
 *
 * Notes:
 *   - Out-of-bounds indices are silently ignored for safety
 *   - Uses bitwise AND with inverted mask to clear without affecting others
 * --------------------------------------------------------------------------- */
void bitmap_clear(bitmap_t *bitmap, size_t index)
{
    /* Bounds check - silently ignore invalid indices */
    if (!bitmap || index >= bitmap->size_bits) {
        return;
    }
    
    /*
     * Clear the bit at position (index % 8) in byte (index / 8)
     * Example: index=13 -> byte[1] &= ~(1 << 5) -> clears bit 5 of byte 1
     */
    bitmap->buffer[index / 8] &= ~(1 << (index % 8));
}

/* ---------------------------------------------------------------------------
 * bitmap_test - Test if a bit is set (check if resource is used)
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *   index  - Bit index to test (0-based)
 *
 * Returns:
 *   true if the bit is set (resource used), false otherwise
 *   Also returns false for out-of-bounds indices
 * --------------------------------------------------------------------------- */
bool bitmap_test(const bitmap_t *bitmap, size_t index)
{
    /* Bounds check - return false for invalid indices */
    if (!bitmap || index >= bitmap->size_bits) {
        return false;
    }
    
    /*
     * Test the bit at position (index % 8) in byte (index / 8)
     * Returns non-zero (true) if bit is set, zero (false) otherwise
     */
    return (bitmap->buffer[index / 8] & (1 << (index % 8))) != 0;
}

/* ---------------------------------------------------------------------------
 * bitmap_find_first_zero - Find the first zero bit (first free resource)
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *
 * Returns:
 *   Index of first zero bit (>= 0), or -1 if all bits are set (no free resources)
 *
 * Algorithm:
 *   1. Scan bytes looking for any that aren't 0xFF (all bits set)
 *   2. When found, scan individual bits within that byte
 *   3. Return the global bit index
 *
 * Performance:
 *   - Best case O(1): first byte has a zero bit
 *   - Worst case O(N/8): must scan all bytes
 * --------------------------------------------------------------------------- */
int64_t bitmap_find_first_zero(const bitmap_t *bitmap)
{
    if (!bitmap) {
        return -1;
    }

    /* Scan each byte in the bitmap */
    for (size_t byte_idx = 0; byte_idx < bitmap->size_bytes; byte_idx++) {
        /* Skip bytes that are fully allocated (all bits set) */
        if (bitmap->buffer[byte_idx] != 0xFF) {
            /* Found a byte with at least one zero bit - find which one */
            for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
                if (!(bitmap->buffer[byte_idx] & (1 << bit_idx))) {
                    /* Calculate the global bit index */
                    size_t global_index = (byte_idx * 8) + bit_idx;
                    
                    /* Verify we haven't exceeded the actual bit count */
                    /* (last byte might have unused bits) */
                    if (global_index < bitmap->size_bits) {
                        return (int64_t)global_index;
                    }
                }
            }
        }
    }

    /* All bits are set - no free resources */
    return -1;
}

/* ---------------------------------------------------------------------------
 * bitmap_find_first_set - Find the first set bit (first used resource)
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *
 * Returns:
 *   Index of first set bit (>= 0), or -1 if all bits are clear
 *
 * This function is the inverse of bitmap_find_first_zero. It's useful for
 * operations like finding the first allocated resource.
 * --------------------------------------------------------------------------- */
int64_t bitmap_find_first_set(const bitmap_t *bitmap)
{
    if (!bitmap) {
        return -1;
    }

    /* Scan each byte in the bitmap */
    for (size_t byte_idx = 0; byte_idx < bitmap->size_bytes; byte_idx++) {
        /* Skip bytes that are empty (no bits set) */
        if (bitmap->buffer[byte_idx] != 0x00) {
            /* Found a byte with at least one set bit - find which one */
            for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
                if (bitmap->buffer[byte_idx] & (1 << bit_idx)) {
                    /* Calculate the global bit index */
                    size_t global_index = (byte_idx * 8) + bit_idx;
                    
                    /* Verify we haven't exceeded the actual bit count */
                    if (global_index < bitmap->size_bits) {
                        return (int64_t)global_index;
                    }
                }
            }
        }
    }

    /* No bits are set - all resources free */
    return -1;
}

/* ---------------------------------------------------------------------------
 * bitmap_find_contiguous_zeros - Find N contiguous zero bits
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *   count  - Number of contiguous zero bits needed
 *
 * Returns:
 *   Index of first bit in the contiguous range, or -1 if not found
 *
 * This is useful for allocating contiguous memory blocks.
 * --------------------------------------------------------------------------- */
int64_t bitmap_find_contiguous_zeros(const bitmap_t *bitmap, size_t count)
{
    if (!bitmap || count == 0 || count > bitmap->size_bits) {
        return -1;
    }

    size_t consecutive = 0;
    size_t start_index = 0;

    for (size_t i = 0; i < bitmap->size_bits; i++) {
        if (!bitmap_test(bitmap, i)) {
            /* Found a zero bit */
            if (consecutive == 0) {
                start_index = i;  /* Mark start of potential range */
            }
            consecutive++;
            
            if (consecutive >= count) {
                return (int64_t)start_index;
            }
        } else {
            /* Found a set bit - reset counter */
            consecutive = 0;
        }
    }

    /* Couldn't find enough contiguous zeros */
    return -1;
}

/* ---------------------------------------------------------------------------
 * bitmap_set_range - Set multiple consecutive bits
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *   start  - Starting bit index
 *   count  - Number of bits to set
 *
 * This is an optimization for bulk allocation operations.
 * --------------------------------------------------------------------------- */
void bitmap_set_range(bitmap_t *bitmap, size_t start, size_t count)
{
    if (!bitmap) {
        return;
    }

    for (size_t i = 0; i < count && (start + i) < bitmap->size_bits; i++) {
        bitmap_set(bitmap, start + i);
    }
}

/* ---------------------------------------------------------------------------
 * bitmap_clear_range - Clear multiple consecutive bits
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *   start  - Starting bit index
 *   count  - Number of bits to clear
 *
 * This is an optimization for bulk deallocation operations.
 * --------------------------------------------------------------------------- */
void bitmap_clear_range(bitmap_t *bitmap, size_t start, size_t count)
{
    if (!bitmap) {
        return;
    }

    for (size_t i = 0; i < count && (start + i) < bitmap->size_bits; i++) {
        bitmap_clear(bitmap, start + i);
    }
}

/* ---------------------------------------------------------------------------
 * bitmap_count_set - Count the number of set bits
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *
 * Returns:
 *   Number of bits that are set to 1
 *
 * Uses the Brian Kernighan algorithm for efficient bit counting.
 * --------------------------------------------------------------------------- */
size_t bitmap_count_set(const bitmap_t *bitmap)
{
    if (!bitmap) {
        return 0;
    }

    size_t count = 0;

    for (size_t byte_idx = 0; byte_idx < bitmap->size_bytes; byte_idx++) {
        uint8_t byte_val = bitmap->buffer[byte_idx];
        
        /* Brian Kernighan's algorithm: count bits by clearing them one at a time */
        while (byte_val) {
            byte_val &= (byte_val - 1);  /* Clear the least significant set bit */
            count++;
        }
    }

    /* Account for padding bits in the last byte that shouldn't be counted */
    size_t padding_bits = (bitmap->size_bytes * 8) - bitmap->size_bits;
    if (padding_bits > 0) {
        uint8_t last_byte = bitmap->buffer[bitmap->size_bytes - 1];
        for (size_t i = bitmap->size_bits % 8; i < 8; i++) {
            if (last_byte & (1 << i)) {
                count--;  /* Don't count padding bits */
            }
        }
    }

    return count;
}

/* ---------------------------------------------------------------------------
 * bitmap_count_clear - Count the number of clear bits
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap - Pointer to initialized bitmap
 *
 * Returns:
 *   Number of bits that are clear (0)
 * --------------------------------------------------------------------------- */
size_t bitmap_count_clear(const bitmap_t *bitmap)
{
    if (!bitmap) {
        return 0;
    }

    return bitmap->size_bits - bitmap_count_set(bitmap);
}
