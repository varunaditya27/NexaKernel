/*
 * kernel/panic.c
 *
 * Kernel Panic Handler
 *
 * This file implements the `panic()` function, which is called when the kernel
 * encounters an unrecoverable error. It prints a diagnostic message to the
 * screen and halts the CPU to prevent further damage or corruption.
 *
 * It is a critical debugging tool for kernel development.
 */

#include <stdarg.h>

void panic(const char *fmt, ...) {
    // Placeholder: implement vga printing and halt
    while (1) { }
}
