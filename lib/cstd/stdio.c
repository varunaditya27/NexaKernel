/*
 * lib/cstd/stdio.c
 *
 * Minimal stdio functions for userland and kernel. This file contains wrappers
 * used by the kernel's own kprintf and by user programs built into the kernel
 * image.
 */

int libc_printf(const char *fmt, ...) { return 0; }
