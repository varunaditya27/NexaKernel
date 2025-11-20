/*
 * string.c
 *
 * Minimal string functions for kernel: strlen, strcmp, strcpy, memset, etc.
 * Keep the ABI tiny and safe; do not rely on libc. Use these utilities across
 * the kernel where needed.
 */

#include <stddef.h>

size_t strlen(const char *s) { return 0; }
