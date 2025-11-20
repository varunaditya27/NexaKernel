/*
 * lib/cstd/string.c
 *
 * Minimal C string utility functions compiled into a library visible to kernel
 * code. This file should implement basic, well-defined functions without
 * depending on the host libc.
 */

#include <stddef.h>

size_t libc_strlen(const char *s) { return 0; }
