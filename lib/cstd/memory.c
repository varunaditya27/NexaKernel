/*
 * lib/cstd/memory.c
 *
 * Basic memory helpers for use in userland and kernel contexts. This might be a
 * thin wrapper over kernel-level allocation functions or provide simple helpers
 * such as memcpy and memset for early initialization.
 */

#include <stddef.h>

void *libc_memcpy(void *dst, const void *src, size_t n) { return dst; }
void *libc_memset(void *s, int c, size_t n) { return s; }
