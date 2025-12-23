/*
 * kernel/drivers/vga_text.c
 *
 * VGA Text Mode Driver
 *
 * This file provides low-level functions to write characters and strings to the
 * VGA text buffer (typically at 0xB8000). It supports basic cursor management
 * and screen scrolling.
 *
 * It is the primary output mechanism for the kernel console.
 */

void vga_write_string(const char *s) {}
