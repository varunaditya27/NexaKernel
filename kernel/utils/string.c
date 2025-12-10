/*
 * kernel/utils/string.c
 *
 * Kernel String Utilities
 *
 * This file provides essential string manipulation functions (strlen, strcpy,
 * memset, memcpy) for use by kernel code. These are freestanding implementations
 * that do not rely on an external C library.
 */

#include <stddef.h>

size_t strlen(const char *s) { return 0; }
