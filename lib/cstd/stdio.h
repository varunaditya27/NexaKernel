/*
 * ===========================================================================
 * lib/cstd/stdio.h
 * ===========================================================================
 *
 * Standard I/O Library Interface
 *
 * This header provides basic I/O functions for the kernel. In the kernel
 * context, these wrap around the VGA driver or serial port for output.
 *
 * Note: This is a minimal implementation suitable for kernel debugging
 * and logging. Full stdio functionality is not supported.
 *
 * Usage:
 *   #include <lib/cstd/stdio.h>
 *
 *   kprintf("Value: %d\n", 42);
 *
 * ===========================================================================
 */

#ifndef NEXA_CSTD_STDIO_H
#define NEXA_CSTD_STDIO_H

#include "../../config/os_config.h"
#include <stdarg.h>

/* ---------------------------------------------------------------------------
 * Kernel Printf Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Kernel printf - formatted output to console
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments
 * @return Number of characters printed, or negative on error
 *
 * Supported format specifiers:
 *   %d, %i - Signed decimal integer
 *   %u     - Unsigned decimal integer
 *   %x, %X - Unsigned hexadecimal integer
 *   %c     - Character
 *   %s     - String
 *   %p     - Pointer (as hexadecimal)
 *   %%     - Literal percent sign
 *
 * Width and padding modifiers are supported (e.g., %08x)
 */
int kprintf(const char *fmt, ...);

/**
 * @brief Kernel vprintf - formatted output with va_list
 * @param fmt Format string
 * @param ap Variable argument list
 * @return Number of characters printed, or negative on error
 */
int kvprintf(const char *fmt, va_list ap);

/**
 * @brief Kernel sprintf - formatted output to string buffer
 * @param buf Destination buffer
 * @param fmt Format string
 * @param ... Variable arguments
 * @return Number of characters written (excluding null terminator)
 * @warning No buffer overflow protection - use ksnprintf instead
 */
int ksprintf(char *buf, const char *fmt, ...);

/**
 * @brief Kernel snprintf - formatted output with length limit
 * @param buf Destination buffer
 * @param size Maximum bytes to write (including null terminator)
 * @param fmt Format string
 * @param ... Variable arguments
 * @return Number of characters that would have been written
 */
int ksnprintf(char *buf, size_t size, const char *fmt, ...);

/* ---------------------------------------------------------------------------
 * Character I/O Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Output a single character to console
 * @param c Character to output
 * @return The character written, or negative on error
 */
int kputchar(int c);

/**
 * @brief Output a string to console
 * @param s Null-terminated string
 * @return Non-negative on success, negative on error
 */
int kputs(const char *s);

#endif /* NEXA_CSTD_STDIO_H */
