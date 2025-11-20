/*
 * kernel/panic.c
 *
 * Panic and assertion helpers for kernel debugging. Provide a function like
 * `void panic(const char *fmt, ...)` which prints error info to VGA and
 * halts the CPU.
 *
 * Keep panic code simple. Avoid dynamic allocations and long dependency chains
 * — panic can be called from anywhere and should be minimally dependent.
 */

#include <stdarg.h>

void panic(const char *fmt, ...) {
    // Placeholder: implement vga printing and halt
    while (1) { }
}
