/*
 * ===========================================================================
 * memory_layout.ld
 * ===========================================================================
 *
 * Kernel Linker Script for NexaKernel
 *
 * This script controls how the linker combines object files and where each
 * section is placed in the final kernel binary. Key considerations:
 *
 * 1. Multiboot header MUST be in first 8KB of the kernel image
 * 2. Kernel is loaded at 1MB (0x00100000) - standard for x86 protected mode
 * 3. Sections are page-aligned (4KB) for future paging support
 *
 * Memory Layout:
 *   0x00100000 - Kernel start (1MB mark)
 *   .multiboot - Multiboot header (must be first!)
 *   .text      - Executable code
 *   .rodata    - Read-only data (constants, strings)
 *   .data      - Initialized read-write data
 *   .bss       - Uninitialized data (zeroed at startup)
 *
 * ===========================================================================
 */

/* Entry point - where execution begins after bootloader */
ENTRY(_start)

/* Define memory regions (optional, for documentation) */
MEMORY
{
    /* Kernel memory starts at 1MB, reserve 16MB for kernel */
    kernel (rwx) : ORIGIN = 0x00100000, LENGTH = 16M
}

/* Output format for 32-bit x86 ELF */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

SECTIONS
{
    /* Start at 1MB - below this is reserved for BIOS, video memory, etc. */
    . = 0x00100000;
    
    /* Record kernel start address */
    _kernel_start = .;

    /* -----------------------------------------------------------------------
     * Multiboot Header Section
     * -----------------------------------------------------------------------
     * MUST be within first 8KB of the kernel image!
     * We place it at the very beginning to satisfy this requirement.
     * ----------------------------------------------------------------------- */
    .multiboot ALIGN(4) :
    {
        *(.multiboot)
    }

    /* -----------------------------------------------------------------------
     * Code Section (.text)
     * -----------------------------------------------------------------------
     * Contains all executable code. Page-aligned for memory protection.
     * ----------------------------------------------------------------------- */
    .text ALIGN(4K) :
    {
        _text_start = .;
        *(.text)                            /* All .text sections */
        *(.text.*)                          /* Compiler-generated subsections */
        _text_end = .;
    }

    /* -----------------------------------------------------------------------
     * Read-Only Data Section (.rodata)
     * -----------------------------------------------------------------------
     * Contains constant data like string literals and const variables.
     * Kept separate so it can be made read-only with paging.
     * ----------------------------------------------------------------------- */
    .rodata ALIGN(4K) :
    {
        _rodata_start = .;
        *(.rodata)                          /* All .rodata sections */
        *(.rodata.*)                        /* Compiler-generated subsections */
        _rodata_end = .;
    }

    /* -----------------------------------------------------------------------
     * Initialized Data Section (.data)
     * -----------------------------------------------------------------------
     * Contains initialized global and static variables.
     * ----------------------------------------------------------------------- */
    .data ALIGN(4K) :
    {
        _data_start = .;
        *(.data)                            /* All .data sections */
        *(.data.*)                          /* Compiler-generated subsections */
        _data_end = .;
    }

    /* -----------------------------------------------------------------------
     * Uninitialized Data Section (.bss)
     * -----------------------------------------------------------------------
     * Contains uninitialized global and static variables.
     * This section is zeroed by the bootloader or startup code.
     * Uses NOLOAD to indicate no data is stored in the binary.
     * ----------------------------------------------------------------------- */
    .bss ALIGN(4K) (NOLOAD) :
    {
        _bss_start = .;
        *(COMMON)                           /* Common symbols */
        *(.bss)                             /* All .bss sections */
        *(.bss.*)                           /* Compiler-generated subsections */
        _bss_end = .;
    }

    /* Record kernel end address (useful for heap initialization) */
    . = ALIGN(4K);
    _kernel_end = .;

    /* -----------------------------------------------------------------------
     * Debug Sections (optional, stripped in release builds)
     * ----------------------------------------------------------------------- */
    /DISCARD/ :
    {
        *(.comment)                         /* Compiler comments */
        *(.note*)                           /* Note sections */
        *(.eh_frame*)                       /* C++ exception handling (not used) */
    }
}

/* Provide symbols for kernel to know its own size and layout */
_kernel_size = _kernel_end - _kernel_start;
_bss_size = _bss_end - _bss_start;
