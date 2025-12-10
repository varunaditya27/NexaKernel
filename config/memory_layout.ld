/*
 * memory_layout.ld
 *
 * Kernel Linker Script
 *
 * This script defines the memory layout of the final kernel binary. It instructs
 * the linker on how to combine object files and where to place sections (text,
 * data, bss) in physical memory.
 *
 * It ensures the kernel is linked to run at the correct load address (e.g., 1MB).
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
