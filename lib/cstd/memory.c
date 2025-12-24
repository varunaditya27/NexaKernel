/*
 * ===========================================================================
 * lib/cstd/memory.c
 * ===========================================================================
 *
 * Standard Memory Library Implementation
 *
 * This file implements standard C memory functions (memcpy, memset, etc.)
 * for use by the kernel and userland applications. These are freestanding
 * implementations that do not rely on an external C library.
 *
 * ===========================================================================
 */

#include "memory.h"
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Memory Copy Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Copy memory block (non-overlapping)
 */
void *memcpy(void *dst, const void *src, size_t n)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

/**
 * @brief Copy memory block (handles overlapping)
 */
void *memmove(void *dst, const void *src, size_t n)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

/* ---------------------------------------------------------------------------
 * Memory Set Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Fill memory with a constant byte
 */
void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

/* ---------------------------------------------------------------------------
 * Memory Comparison Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Compare two memory blocks
 */
int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Memory Search Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Find first occurrence of a byte in memory
 */
void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    while (n--) {
        if (*p == (unsigned char)c)
            return (void *)p;
        p++;
    }
    return NULL;
}
