/*
 * ===========================================================================
 * lib/cstd/stdio.c
 * ===========================================================================
 *
 * Standard I/O Library Implementation
 *
 * This file implements basic I/O functions for the kernel. In the kernel
 * context, these wrap around the VGA driver for console output.
 *
 * ===========================================================================
 */

#include "stdio.h"
#include "../../config/os_config.h"
#include <stdarg.h>

/* ---------------------------------------------------------------------------
 * External Dependencies
 * ---------------------------------------------------------------------------
 * These functions must be provided by the kernel's drivers
 * --------------------------------------------------------------------------- */
extern void vga_putchar(char c);
extern void vga_write_string(const char *s);
extern void serial_putchar(char c);

/* ---------------------------------------------------------------------------
 * Internal Helper Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Output a single character
 */
static void putchar_internal(char c)
{
    vga_putchar(c);
    serial_putchar(c);
}

/**
 * @brief Output a string
 */
static void puts_internal(const char *s)
{
    while (*s) {
        putchar_internal(*s++);
    }
}

/**
 * @brief Convert unsigned integer to string
 * @param value Value to convert
 * @param buf Buffer to write to (must have space for result)
 * @param base Numeric base (10 or 16)
 * @param uppercase Use uppercase hex digits
 * @return Pointer to start of number string in buffer
 */
static char *utoa_internal(uint32_t value, char *buf, int base, int uppercase)
{
    static const char digits_lower[] = "0123456789abcdef";
    static const char digits_upper[] = "0123456789ABCDEF";
    const char *digits = uppercase ? digits_upper : digits_lower;
    
    char *ptr = buf + 32;  /* Start from end of buffer */
    *ptr = '\0';
    
    if (value == 0) {
        *--ptr = '0';
        return ptr;
    }
    
    while (value > 0) {
        *--ptr = digits[value % base];
        value /= base;
    }
    
    return ptr;
}

/**
 * @brief Convert signed integer to string
 */
static char *itoa_internal(int32_t value, char *buf)
{
    char *ptr;
    int negative = 0;
    
    if (value < 0) {
        negative = 1;
        value = -value;
    }
    
    ptr = utoa_internal((uint32_t)value, buf, 10, 0);
    
    if (negative) {
        *--ptr = '-';
    }
    
    return ptr;
}

/* ---------------------------------------------------------------------------
 * Public API Implementation
 * --------------------------------------------------------------------------- */

/**
 * @brief Kernel vprintf - formatted output with va_list
 */
int kvprintf(const char *fmt, va_list ap)
{
    int count = 0;
    char numbuf[36];
    char c;
    const char *s;
    char *numstr;
    int32_t ival;
    uint32_t uval;
    void *pval;
    int width;
    char pad_char;
    int len;
    
    while (*fmt) {
        if (*fmt != '%') {
            putchar_internal(*fmt++);
            count++;
            continue;
        }
        
        fmt++;  /* Skip '%' */
        
        /* Check for zero padding */
        pad_char = ' ';
        if (*fmt == '0') {
            pad_char = '0';
            fmt++;
        }
        
        /* Parse width */
        width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        
        /* Process format specifier */
        switch (*fmt) {
            case 'd':
            case 'i':
                ival = va_arg(ap, int32_t);
                numstr = itoa_internal(ival, numbuf);
                len = 0;
                for (const char *p = numstr; *p; p++) len++;
                while (width > len) {
                    putchar_internal(pad_char);
                    count++;
                    width--;
                }
                puts_internal(numstr);
                count += len;
                break;
                
            case 'u':
                uval = va_arg(ap, uint32_t);
                numstr = utoa_internal(uval, numbuf, 10, 0);
                len = 0;
                for (const char *p = numstr; *p; p++) len++;
                while (width > len) {
                    putchar_internal(pad_char);
                    count++;
                    width--;
                }
                puts_internal(numstr);
                count += len;
                break;
                
            case 'x':
                uval = va_arg(ap, uint32_t);
                numstr = utoa_internal(uval, numbuf, 16, 0);
                len = 0;
                for (const char *p = numstr; *p; p++) len++;
                while (width > len) {
                    putchar_internal(pad_char);
                    count++;
                    width--;
                }
                puts_internal(numstr);
                count += len;
                break;
                
            case 'X':
                uval = va_arg(ap, uint32_t);
                numstr = utoa_internal(uval, numbuf, 16, 1);
                len = 0;
                for (const char *p = numstr; *p; p++) len++;
                while (width > len) {
                    putchar_internal(pad_char);
                    count++;
                    width--;
                }
                puts_internal(numstr);
                count += len;
                break;
                
            case 'p':
                pval = va_arg(ap, void *);
                puts_internal("0x");
                count += 2;
                numstr = utoa_internal((uint32_t)(uintptr_t)pval, numbuf, 16, 0);
                len = 0;
                for (const char *p = numstr; *p; p++) len++;
                /* Pad to 8 hex digits for pointers */
                while (8 > len) {
                    putchar_internal('0');
                    count++;
                    len++;
                }
                puts_internal(numstr);
                for (const char *p = numstr; *p; p++) count++;
                break;
                
            case 'c':
                c = (char)va_arg(ap, int);
                putchar_internal(c);
                count++;
                break;
                
            case 's':
                s = va_arg(ap, const char *);
                if (s == NULL) s = "(null)";
                puts_internal(s);
                while (*s++) count++;
                break;
                
            case '%':
                putchar_internal('%');
                count++;
                break;
                
            default:
                /* Unknown format, print as-is */
                putchar_internal('%');
                putchar_internal(*fmt);
                count += 2;
                break;
        }
        
        fmt++;
    }
    
    return count;
}

/**
 * @brief Kernel printf - formatted output to console
 */
int kprintf(const char *fmt, ...)
{
    va_list ap;
    int count;
    
    va_start(ap, fmt);
    count = kvprintf(fmt, ap);
    va_end(ap);
    
    return count;
}

/**
 * @brief Output a single character to console
 */
int kputchar(int c)
{
    putchar_internal((char)c);
    return c;
}

/**
 * @brief Output a string to console
 */
int kputs(const char *s)
{
    if (s == NULL) return -1;
    puts_internal(s);
    putchar_internal('\n');
    return 0;
}
