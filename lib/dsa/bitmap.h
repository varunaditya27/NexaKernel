/*
 * ===========================================================================
 * lib/dsa/bitmap.h
 * ===========================================================================
 *
 * Generic Bitmap Data Structure Interface
 *
 * A bitmap is a space-efficient data structure that represents a set of
 * boolean values as individual bits. This is particularly useful in operating
 * systems for tracking resource allocation (memory frames, disk blocks, etc.).
 *
 * Memory Usage: N bits require only ceil(N/8) bytes of storage.
 *
 * Usage Example:
 *   uint8_t buffer[1024];           // Space for 8192 bits
 *   bitmap_t bm;
 *   bitmap_init(&bm, 8000, buffer); // Track 8000 resources
 *   bitmap_set(&bm, 42);            // Mark resource 42 as used
 *   if (!bitmap_test(&bm, 100))     // Check if resource 100 is free
 *       bitmap_set(&bm, 100);       // Allocate it
 *
 * This bitmap implementation is used by:
 *   - Physical frame allocator (kernel/memory/frame_allocator.c)
 *   - Potentially disk block allocators in filesystem
 *
 * ===========================================================================
 */

#ifndef NEXA_BITMAP_H
#define NEXA_BITMAP_H

#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * Bitmap Structure
 * ---------------------------------------------------------------------------
 * Stores metadata about the bitmap along with a pointer to the actual bit
 * storage buffer. The buffer is provided by the caller to allow flexible
 * memory allocation strategies.
 * --------------------------------------------------------------------------- */
typedef struct bitmap {
    uint8_t *buffer;      /* Pointer to the bit storage array */
    size_t size_bits;     /* Total number of bits being tracked */
    size_t size_bytes;    /* Size of buffer in bytes = ceil(size_bits/8) */
} bitmap_t;

/* ---------------------------------------------------------------------------
 * Core Operations
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize a bitmap structure
 * 
 * @param bitmap    Pointer to bitmap structure to initialize
 * @param size_bits Number of bits to track
 * @param buffer    Pre-allocated buffer (must be >= (size_bits+7)/8 bytes)
 * @return true on success, false if parameters are invalid
 * 
 * @note All bits are initialized to 0 (free/clear state)
 */
bool bitmap_init(bitmap_t *bitmap, size_t size_bits, void *buffer);

/**
 * @brief Set a bit to 1 (mark as used/allocated)
 * 
 * @param bitmap Pointer to initialized bitmap
 * @param index  Zero-based bit index to set
 * 
 * @note Out-of-bounds indices are silently ignored
 */
void bitmap_set(bitmap_t *bitmap, size_t index);

/**
 * @brief Clear a bit to 0 (mark as free/available)
 * 
 * @param bitmap Pointer to initialized bitmap
 * @param index  Zero-based bit index to clear
 * 
 * @note Out-of-bounds indices are silently ignored
 */
void bitmap_clear(bitmap_t *bitmap, size_t index);

/**
 * @brief Test if a bit is set
 * 
 * @param bitmap Pointer to initialized bitmap
 * @param index  Zero-based bit index to test
 * @return true if bit is set (used), false if clear or out-of-bounds
 */
bool bitmap_test(const bitmap_t *bitmap, size_t index);

/* ---------------------------------------------------------------------------
 * Search Operations
 * --------------------------------------------------------------------------- */

/**
 * @brief Find the first zero (clear) bit
 * 
 * @param bitmap Pointer to initialized bitmap
 * @return Index of first zero bit, or -1 if all bits are set
 * 
 * This is the primary allocation function - finds the first free resource.
 */
int64_t bitmap_find_first_zero(const bitmap_t *bitmap);

/**
 * @brief Find the first set bit
 * 
 * @param bitmap Pointer to initialized bitmap
 * @return Index of first set bit, or -1 if all bits are clear
 */
int64_t bitmap_find_first_set(const bitmap_t *bitmap);

/**
 * @brief Find N contiguous zero bits
 * 
 * @param bitmap Pointer to initialized bitmap
 * @param count  Number of contiguous zero bits needed
 * @return Index of first bit in the range, or -1 if not found
 * 
 * Useful for allocating contiguous memory blocks.
 */
int64_t bitmap_find_contiguous_zeros(const bitmap_t *bitmap, size_t count);

/* ---------------------------------------------------------------------------
 * Bulk Operations
 * --------------------------------------------------------------------------- */

/**
 * @brief Set a range of consecutive bits
 * 
 * @param bitmap Pointer to initialized bitmap
 * @param start  Starting bit index
 * @param count  Number of bits to set
 */
void bitmap_set_range(bitmap_t *bitmap, size_t start, size_t count);

/**
 * @brief Clear a range of consecutive bits
 * 
 * @param bitmap Pointer to initialized bitmap
 * @param start  Starting bit index
 * @param count  Number of bits to clear
 */
void bitmap_clear_range(bitmap_t *bitmap, size_t start, size_t count);

/* ---------------------------------------------------------------------------
 * Statistics
 * --------------------------------------------------------------------------- */

/**
 * @brief Count the number of set bits
 * 
 * @param bitmap Pointer to initialized bitmap
 * @return Number of bits that are set to 1
 */
size_t bitmap_count_set(const bitmap_t *bitmap);

/**
 * @brief Count the number of clear bits
 * 
 * @param bitmap Pointer to initialized bitmap
 * @return Number of bits that are clear (0)
 */
size_t bitmap_count_clear(const bitmap_t *bitmap);

#endif /* NEXA_BITMAP_H */
