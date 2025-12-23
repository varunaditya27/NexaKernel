/*
 * ===========================================================================
 * kernel/kernel.c
 * ===========================================================================
 *
 * NexaKernel - Main Kernel Entry Point
 *
 * This file contains the main C entry point for the kernel (`kernel_main`).
 * It is called by the bootloader after the CPU is in protected mode and
 * the initial environment is set up.
 *
 * Initialization Sequence:
 * 1. Verify multiboot information
 * 2. Initialize VGA console for output
 * 3. Initialize memory management (frame allocator, heap)
 * 4. Initialize interrupt handling (IDT, PIC)
 * 5. Initialize device drivers (timer, keyboard)
 * 6. Initialize scheduler
 * 7. Enter main loop or start first process
 *
 * ===========================================================================
 */

#include "../config/os_config.h"

/* ---------------------------------------------------------------------------
 * Multiboot Information Structure
 * ---------------------------------------------------------------------------
 * This structure is passed by GRUB and contains information about the system.
 * We only define the fields we need for now.
 * --------------------------------------------------------------------------- */
typedef struct multiboot_info {
    unsigned int flags;             /* Multiboot info flags */
    unsigned int mem_lower;         /* Available memory below 1MB (KB) */
    unsigned int mem_upper;         /* Available memory above 1MB (KB) */
    unsigned int boot_device;       /* Boot device */
    unsigned int cmdline;           /* Kernel command line */
    unsigned int mods_count;        /* Number of modules loaded */
    unsigned int mods_addr;         /* Address of module structures */
    /* ... more fields we don't use yet ... */
} __attribute__((packed)) multiboot_info_t;

/* ---------------------------------------------------------------------------
 * External Functions (from assembly)
 * --------------------------------------------------------------------------- */
extern void cpu_halt(void);         /* Halt the CPU */
extern void cpu_cli(void);          /* Disable interrupts */
extern void cpu_sti(void);          /* Enable interrupts */

/* ---------------------------------------------------------------------------
 * Forward Declarations
 * --------------------------------------------------------------------------- */
static void early_console_init(void);
static void early_console_print(const char *str);
static void early_console_print_hex(unsigned int value);
static void clear_bss(void);

/* ---------------------------------------------------------------------------
 * VGA Text Mode Constants
 * --------------------------------------------------------------------------- */
#define VGA_BUFFER      0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_COLOR_WHITE 0x0F        /* White on black */
#define VGA_COLOR_ERROR 0x4F        /* White on red */

/* VGA state */
static unsigned short *vga_buffer = (unsigned short *)VGA_BUFFER;
static int vga_row = 0;
static int vga_col = 0;

/* ---------------------------------------------------------------------------
 * BSS Section Symbols (from linker script)
 * --------------------------------------------------------------------------- */
extern char _bss_start[];
extern char _bss_end[];
extern char _kernel_start[];
extern char _kernel_end[];

/* ---------------------------------------------------------------------------
 * kernel_main - Main kernel entry point
 * ---------------------------------------------------------------------------
 * Parameters:
 *   multiboot_info - Pointer to Multiboot information structure (from GRUB)
 *
 * This function never returns. If all initialization succeeds, it enters
 * the scheduler's main loop. If something fails, it panics.
 * --------------------------------------------------------------------------- */
void kernel_main(multiboot_info_t *multiboot_info)
{
    /* Ensure interrupts are disabled during initialization */
    cpu_cli();

    /* Clear BSS section (uninitialized globals should be zero) */
    clear_bss();

    /* Initialize early console for output */
    early_console_init();

    /* Print welcome banner */
    early_console_print("========================================\n");
    early_console_print("  NexaKernel v0.1 - Booting...\n");
    early_console_print("========================================\n\n");

    /* Display boot information */
    early_console_print("[BOOT] Kernel loaded at: 0x");
    early_console_print_hex((unsigned int)_kernel_start);
    early_console_print("\n");
    
    early_console_print("[BOOT] Kernel ends at:   0x");
    early_console_print_hex((unsigned int)_kernel_end);
    early_console_print("\n");

    /* Check multiboot information */
    if (multiboot_info != 0) {
        early_console_print("[BOOT] Multiboot flags:  0x");
        early_console_print_hex(multiboot_info->flags);
        early_console_print("\n");

        /* Check if memory info is available (flag bit 0) */
        if (multiboot_info->flags & 0x1) {
            early_console_print("[BOOT] Lower memory:     ");
            early_console_print_hex(multiboot_info->mem_lower);
            early_console_print(" KB\n");
            
            early_console_print("[BOOT] Upper memory:     ");
            early_console_print_hex(multiboot_info->mem_upper);
            early_console_print(" KB\n");
        }
    }

    early_console_print("\n[INIT] Phase 1: Memory Management\n");
    early_console_print("  - Frame allocator: TODO\n");
    early_console_print("  - Kernel heap:     TODO\n");

    early_console_print("\n[INIT] Phase 2: Interrupt Handling\n");
    early_console_print("  - IDT setup:       TODO\n");
    early_console_print("  - PIC setup:       TODO\n");

    early_console_print("\n[INIT] Phase 3: Device Drivers\n");
    early_console_print("  - VGA driver:      OK (early console)\n");
    early_console_print("  - Timer (PIT):     TODO\n");
    early_console_print("  - Keyboard:        TODO\n");

    early_console_print("\n[INIT] Phase 4: Scheduler\n");
    early_console_print("  - Task system:     TODO\n");
    early_console_print("  - Round-robin:     TODO\n");

    early_console_print("\n========================================\n");
    early_console_print("  Boot sequence complete!\n");
    early_console_print("  System halted (no scheduler yet)\n");
    early_console_print("========================================\n");

    /* Halt the CPU - in a real kernel, we'd start the scheduler here */
    cpu_halt();

    /* Should never reach here */
    while (1) { }
}

/* ---------------------------------------------------------------------------
 * clear_bss - Zero out the BSS section
 * ---------------------------------------------------------------------------
 * The BSS section contains uninitialized global variables. By C standard,
 * they should be zero-initialized. The bootloader might not do this, so
 * we do it ourselves.
 * --------------------------------------------------------------------------- */
static void clear_bss(void)
{
    char *ptr = _bss_start;
    while (ptr < _bss_end) {
        *ptr++ = 0;
    }
}

/* ---------------------------------------------------------------------------
 * early_console_init - Initialize VGA text mode console
 * ---------------------------------------------------------------------------
 * Clears the screen and sets cursor to top-left corner.
 * This is a minimal implementation for early boot messages.
 * --------------------------------------------------------------------------- */
static void early_console_init(void)
{
    /* Clear the entire screen */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (VGA_COLOR_WHITE << 8) | ' ';
    }
    
    /* Reset cursor position */
    vga_row = 0;
    vga_col = 0;
}

/* ---------------------------------------------------------------------------
 * early_console_scroll - Scroll the screen up by one line
 * --------------------------------------------------------------------------- */
static void early_console_scroll(void)
{
    /* Move all lines up by one */
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    
    /* Clear the last line */
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (VGA_COLOR_WHITE << 8) | ' ';
    }
    
    /* Move cursor up */
    vga_row = VGA_HEIGHT - 1;
}

/* ---------------------------------------------------------------------------
 * early_console_print - Print a string to the VGA console
 * --------------------------------------------------------------------------- */
static void early_console_print(const char *str)
{
    while (*str) {
        char c = *str++;
        
        if (c == '\n') {
            /* Newline: move to start of next line */
            vga_col = 0;
            vga_row++;
        } else if (c == '\r') {
            /* Carriage return: move to start of line */
            vga_col = 0;
        } else if (c == '\t') {
            /* Tab: advance to next 8-column boundary */
            vga_col = (vga_col + 8) & ~7;
        } else {
            /* Regular character: write to screen */
            int pos = vga_row * VGA_WIDTH + vga_col;
            vga_buffer[pos] = (VGA_COLOR_WHITE << 8) | c;
            vga_col++;
        }
        
        /* Handle line wrap */
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
        
        /* Handle scroll */
        if (vga_row >= VGA_HEIGHT) {
            early_console_scroll();
        }
    }
}

/* ---------------------------------------------------------------------------
 * early_console_print_hex - Print a 32-bit value in hexadecimal
 * --------------------------------------------------------------------------- */
static void early_console_print_hex(unsigned int value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    buffer[8] = '\0';
    
    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    early_console_print(buffer);
}
