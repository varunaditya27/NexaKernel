/*
 * kernel/drivers/vga_text.c
 *
 * Minimal VGA text-mode driver. Provide `vga_write_char`, `vga_write_string`
 * and helpers to print kernel messages.
 * Avoid floating point and heavy syscalls from here.
 */

void vga_write_string(const char *s) {}
