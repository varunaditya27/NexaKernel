/*
 * memory_layout.ld
 *
 * Linker script controlling where sections are placed in the final kernel image.
 * The exact memory map will depend on your bootloader and target boot method (GRUB/BIOS).
 * Use this as a starting point and adjust section addresses to match your bootloader.
 */

/* Minimal skeleton; replace with project-specific layout. */
MEMORY
{
  KERNEL (rx) : ORIGIN = 0x00100000, LENGTH = 4M
}

SECTIONS
{
  . = 0x00100000;
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) }
}
