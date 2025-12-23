/*
 * ===========================================================================
 * kernel/panic.c
 * ===========================================================================
 *
 * Kernel Panic Handler
 *
 * This file implements the `panic()` function, which is called when the kernel
 * encounters an unrecoverable error. It:
 *
 * 1. Disables interrupts to prevent further damage
 * 2. Prints a diagnostic message to the VGA console
 * 3. Halts the CPU permanently
 *
 * panic() should be called sparingly and only for truly unrecoverable errors.
 *
 * ===========================================================================
 */

#include <stdarg.h>

/* ---------------------------------------------------------------------------
 * External Functions
 * --------------------------------------------------------------------------- */
extern void cpu_halt(void);         /* Halt the CPU (from startup.asm) */
extern void cpu_cli(void);          /* Disable interrupts */

/* ---------------------------------------------------------------------------
 * VGA Constants
 * --------------------------------------------------------------------------- */
#define VGA_BUFFER      0xB8000
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_COLOR_PANIC 0x4F        /* White on red - DANGER! */

/* ---------------------------------------------------------------------------
 * panic_print - Print string in panic mode (direct VGA access)
 * ---------------------------------------------------------------------------
 * We implement our own print function here rather than depending on other
 * kernel modules, because those modules might be what caused the panic!
 * --------------------------------------------------------------------------- */
static void panic_print(const char *str, int *row, int *col)
{
    unsigned short *vga = (unsigned short *)VGA_BUFFER;
    
    while (*str) {
        char c = *str++;
        
        if (c == '\n') {
            *col = 0;
            (*row)++;
        } else {
            int pos = (*row) * VGA_WIDTH + (*col);
            if (pos < VGA_WIDTH * VGA_HEIGHT) {
                vga[pos] = (VGA_COLOR_PANIC << 8) | c;
            }
            (*col)++;
            if (*col >= VGA_WIDTH) {
                *col = 0;
                (*row)++;
            }
        }
        
        /* Stop at bottom of screen */
        if (*row >= VGA_HEIGHT) {
            *row = VGA_HEIGHT - 1;
        }
    }
}

/* ---------------------------------------------------------------------------
 * panic_print_hex - Print a hexadecimal number
 * --------------------------------------------------------------------------- */
static void panic_print_hex(unsigned int value, int *row, int *col)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11] = "0x00000000";
    
    for (int i = 9; i >= 2; i--) {
        buffer[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    panic_print(buffer, row, col);
}

/* ---------------------------------------------------------------------------
 * panic - Kernel panic handler
 * ---------------------------------------------------------------------------
 * Parameters:
 *   file   - Source file where panic occurred (use __FILE__)
 *   line   - Line number where panic occurred (use __LINE__)
 *   fmt    - Format string (printf-style, but simplified)
 *   ...    - Optional arguments (not implemented in this simple version)
 *
 * This function NEVER returns. The system is halted permanently.
 * --------------------------------------------------------------------------- */
void panic_impl(const char *file, int line, const char *message)
{
    unsigned short *vga = (unsigned short *)VGA_BUFFER;
    int row = 0;
    int col = 0;
    
    /* Disable interrupts immediately */
    cpu_cli();
    
    /* Clear screen with panic color */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (VGA_COLOR_PANIC << 8) | ' ';
    }
    
    /* Print panic header */
    panic_print("!!! KERNEL PANIC !!!", &row, &col);
    row += 2; col = 0;
    
    panic_print("========================================", &row, &col);
    row++; col = 0;
    
    /* Print error message */
    panic_print("Error: ", &row, &col);
    panic_print(message, &row, &col);
    row += 2; col = 0;
    
    /* Print location */
    panic_print("Location: ", &row, &col);
    panic_print(file, &row, &col);
    panic_print(" line ", &row, &col);
    panic_print_hex((unsigned int)line, &row, &col);
    row += 2; col = 0;
    
    panic_print("========================================", &row, &col);
    row += 2; col = 0;
    
    panic_print("System halted. Please reboot.", &row, &col);
    
    /* Halt forever */
    cpu_halt();
    
    /* Should never reach here, but just in case */
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

/* ---------------------------------------------------------------------------
 * Simple panic wrapper (for compatibility)
 * --------------------------------------------------------------------------- */
void panic(const char *message)
{
    panic_impl("unknown", 0, message);
}

/* ---------------------------------------------------------------------------
 * PANIC macro for use throughout the kernel
 * ---------------------------------------------------------------------------
 * Usage: PANIC("Something went wrong!");
 * This macro automatically includes file and line information.
 * --------------------------------------------------------------------------- */
/* 
 * Note: Define this in a header file:
 * #define PANIC(msg) panic_impl(__FILE__, __LINE__, msg)
 */
