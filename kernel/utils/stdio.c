/*
 * stdio.c
 *
 * Very small stdio implementation for kernel printing. Provide vprintf-like
 * functions used by panic and debugging helpers.
 */

#include <stdarg.h>

int kvprintf(const char *fmt, va_list ap) { return 0; }
int kprintf(const char *fmt, ...) { return 0; }
