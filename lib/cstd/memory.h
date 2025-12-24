/*
 * ===========================================================================
 * lib/cstd/memory.h
 * ===========================================================================
 *
 * Standard Memory Library Interface
 *
 * This header provides memory manipulation functions for the kernel and
 * userland applications. These are freestanding implementations that do
 * not rely on an external C library.
 *
 * Note: This provides mem* functions (memcpy, memset, etc.), NOT dynamic
 * memory allocation. For kmalloc/kfree, see kernel/memory/memory.h
 *
 * Usage:
 *   #include <lib/cstd/memory.h>
 *
 *   char buf[64];
 *   memset(buf, 0, sizeof(buf));
 *   memcpy(dest, src, len);
 *
 * ===========================================================================
 */

#ifndef NEXA_CSTD_MEMORY_H
#define NEXA_CSTD_MEMORY_H

#include <stddef.h>

/* ---------------------------------------------------------------------------
 * Memory Copy Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Copy memory block (non-overlapping)
 * @param dst Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to destination
 * @warning Behavior is undefined if regions overlap - use memmove instead
 */
void *memcpy(void *dst, const void *src, size_t n);

/**
 * @brief Copy memory block (handles overlapping)
 * @param dst Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to destination
 * @note Safe even if src and dst overlap
 */
void *memmove(void *dst, const void *src, size_t n);

/* ---------------------------------------------------------------------------
 * Memory Set Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Fill memory with a constant byte
 * @param s Memory region to fill
 * @param c Byte value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to memory region
 */
void *memset(void *s, int c, size_t n);

/* ---------------------------------------------------------------------------
 * Memory Comparison Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Compare two memory blocks
 * @param s1 First memory block
 * @param s2 Second memory block
 * @param n Number of bytes to compare
 * @return <0 if s1<s2, 0 if equal, >0 if s1>s2
 */
int memcmp(const void *s1, const void *s2, size_t n);

/* ---------------------------------------------------------------------------
 * Memory Search Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Find first occurrence of a byte in memory
 * @param s Memory block to search
 * @param c Byte value to find
 * @param n Number of bytes to search
 * @return Pointer to first occurrence, or NULL if not found
 */
void *memchr(const void *s, int c, size_t n);

#endif /* NEXA_CSTD_MEMORY_H */
