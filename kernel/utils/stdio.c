/*
 * kernel/utils/stdio.c
 *
 * Minimal Standard I/O Library
 *
 * This file implements a subset of the standard C I/O functions (like `printf`)
 * for use within the kernel. Since the kernel cannot link against a standard
 * libc, it must provide its own formatting logic for displaying text.
 */

#include <stdarg.h>

int kvprintf(const char *fmt, va_list ap) { return 0; }
int kprintf(const char *fmt, ...) { return 0; }
