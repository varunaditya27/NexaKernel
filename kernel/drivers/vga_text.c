/*
 * ===========================================================================
 * kernel/drivers/vga_text.c
 * ===========================================================================
 *
 * VGA Text Mode Driver
 *
 * This driver provides console output by directly writing to the VGA text
 * buffer located at physical address 0xB8000. Features include:
 *
 * - Character and string output
 * - Hardware cursor management
 * - Screen scrolling
 * - Color attributes (16 foreground, 8 background colors)
 *
 * VGA Text Mode Memory Layout:
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │  The VGA text buffer is 80x25 characters = 4000 bytes (2000 words)       │
 * │                                                                          │
 * │  Each character cell is 2 bytes:                                         │
 * │  - Byte 0: ASCII character code                                          │
 * │  - Byte 1: Attribute byte (colors)                                       │
 * │            Bits 0-3: Foreground color (0-15)                             │
 * │            Bits 4-6: Background color (0-7)                              │
 * │            Bit 7:    Blink (or high-intensity background)                │
 * │                                                                          │
 * │  Screen coordinates: (0,0) = top-left, (79,24) = bottom-right            │
 * └──────────────────────────────────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#include "drivers.h"

/* ---------------------------------------------------------------------------
 * External Functions (from startup.asm)
 * --------------------------------------------------------------------------- */
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

/* ---------------------------------------------------------------------------
 * VGA Constants
 * --------------------------------------------------------------------------- */
#define VGA_BUFFER_ADDR     0xB8000     /* VGA text buffer address */
#define VGA_WIDTH           80          /* Screen width in characters */
#define VGA_HEIGHT          25          /* Screen height in characters */
#define VGA_SIZE            (VGA_WIDTH * VGA_HEIGHT)

/* VGA CRT Controller Ports */
#define VGA_CRTC_ADDR       0x3D4       /* CRT Controller address port */
#define VGA_CRTC_DATA       0x3D5       /* CRT Controller data port */

/* VGA CRT Controller Registers */
#define VGA_CURSOR_START    0x0A        /* Cursor start scanline */
#define VGA_CURSOR_END      0x0B        /* Cursor end scanline */
#define VGA_CURSOR_HIGH     0x0E        /* Cursor location high byte */
#define VGA_CURSOR_LOW      0x0F        /* Cursor location low byte */

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Pointer to VGA text buffer */
static uint16_t *vga_buffer = (uint16_t *)VGA_BUFFER_ADDR;

/* Current cursor position */
static int vga_col = 0;
static int vga_row = 0;

/* Current color attribute */
static uint8_t vga_color = VGA_DEFAULT_COLOR;

/* Tab width */
#define TAB_WIDTH           4

/* ---------------------------------------------------------------------------
 * update_cursor - Update the hardware cursor position
 * ---------------------------------------------------------------------------
 * Sends the current cursor position to the VGA controller.
 * --------------------------------------------------------------------------- */
static void update_cursor(void)
{
    uint16_t pos = (uint16_t)(vga_row * VGA_WIDTH + vga_col);
    
    outb(VGA_CRTC_ADDR, VGA_CURSOR_LOW);
    outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
    
    outb(VGA_CRTC_ADDR, VGA_CURSOR_HIGH);
    outb(VGA_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

/* ---------------------------------------------------------------------------
 * vga_enable_cursor - Enable or disable the hardware cursor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   enable - true to show cursor, false to hide
 * --------------------------------------------------------------------------- */
void vga_enable_cursor(bool enable)
{
    if (enable) {
        /* Enable cursor with default scanline range (14-15 for underline) */
        outb(VGA_CRTC_ADDR, VGA_CURSOR_START);
        outb(VGA_CRTC_DATA, 14);  /* Cursor start scanline */
        
        outb(VGA_CRTC_ADDR, VGA_CURSOR_END);
        outb(VGA_CRTC_DATA, 15);  /* Cursor end scanline */
    } else {
        /* Disable cursor by setting start scanline bit 5 */
        outb(VGA_CRTC_ADDR, VGA_CURSOR_START);
        outb(VGA_CRTC_DATA, 0x20);  /* Bit 5 = cursor disable */
    }
}

/* ---------------------------------------------------------------------------
 * vga_scroll - Scroll the screen up by one line
 * ---------------------------------------------------------------------------
 * Moves all lines up by one and clears the bottom line.
 * --------------------------------------------------------------------------- */
void vga_scroll(void)
{
    /* Move all lines up by one */
    for (int i = 0; i < VGA_SIZE - VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    
    /* Clear the last line */
    uint16_t blank = VGA_ENTRY(' ', vga_color);
    for (int i = VGA_SIZE - VGA_WIDTH; i < VGA_SIZE; i++) {
        vga_buffer[i] = blank;
    }
    
    /* Adjust cursor position */
    if (vga_row > 0) {
        vga_row--;
    }
}

/* ---------------------------------------------------------------------------
 * vga_init - Initialize the VGA text mode driver
 * ---------------------------------------------------------------------------
 * Clears the screen, resets cursor position, and sets default colors.
 * --------------------------------------------------------------------------- */
void vga_init(void)
{
    vga_color = VGA_DEFAULT_COLOR;
    vga_col = 0;
    vga_row = 0;
    
    vga_clear();
    vga_enable_cursor(true);
    update_cursor();
}

/* ---------------------------------------------------------------------------
 * vga_clear - Clear the entire screen
 * ---------------------------------------------------------------------------
 * Fills the screen with spaces using the current background color.
 * --------------------------------------------------------------------------- */
void vga_clear(void)
{
    uint16_t blank = VGA_ENTRY(' ', vga_color);
    
    for (int i = 0; i < VGA_SIZE; i++) {
        vga_buffer[i] = blank;
    }
    
    vga_col = 0;
    vga_row = 0;
    update_cursor();
}

/* ---------------------------------------------------------------------------
 * vga_set_color - Set the current text color
 * ---------------------------------------------------------------------------
 * Parameters:
 *   color - VGA color byte (use VGA_ENTRY_COLOR macro)
 * --------------------------------------------------------------------------- */
void vga_set_color(uint8_t color)
{
    vga_color = color;
}

/* ---------------------------------------------------------------------------
 * vga_putchar - Write a single character at the cursor position
 * ---------------------------------------------------------------------------
 * Parameters:
 *   c - Character to write
 *
 * Handles special characters: '\n', '\r', '\t', '\b'
 * --------------------------------------------------------------------------- */
void vga_putchar(char c)
{
    switch (c) {
        case '\n':  /* Newline */
            vga_col = 0;
            vga_row++;
            break;
            
        case '\r':  /* Carriage return */
            vga_col = 0;
            break;
            
        case '\t':  /* Tab */
            /* Move to next tab stop (every TAB_WIDTH columns) */
            vga_col = (vga_col + TAB_WIDTH) & ~(TAB_WIDTH - 1);
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
            break;
            
        case '\b':  /* Backspace */
            if (vga_col > 0) {
                vga_col--;
                /* Optionally clear the character */
                vga_buffer[vga_row * VGA_WIDTH + vga_col] = VGA_ENTRY(' ', vga_color);
            }
            break;
            
        default:    /* Regular character */
            vga_buffer[vga_row * VGA_WIDTH + vga_col] = VGA_ENTRY(c, vga_color);
            vga_col++;
            
            /* Wrap to next line if needed */
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
            break;
    }
    
    /* Scroll if we've gone past the bottom */
    while (vga_row >= VGA_HEIGHT) {
        vga_scroll();
        vga_row = VGA_HEIGHT - 1;
    }
    
    update_cursor();
}

/* ---------------------------------------------------------------------------
 * vga_write_string - Write a null-terminated string
 * ---------------------------------------------------------------------------
 * Parameters:
 *   str - Null-terminated string to write
 * --------------------------------------------------------------------------- */
void vga_write_string(const char *str)
{
    if (str == NULL) {
        return;
    }
    
    while (*str) {
        vga_putchar(*str++);
    }
}

/* ---------------------------------------------------------------------------
 * vga_write - Write a string with explicit length
 * ---------------------------------------------------------------------------
 * Parameters:
 *   str - String to write (doesn't need to be null-terminated)
 *   len - Number of characters to write
 * --------------------------------------------------------------------------- */
void vga_write(const char *str, size_t len)
{
    if (str == NULL) {
        return;
    }
    
    for (size_t i = 0; i < len; i++) {
        vga_putchar(str[i]);
    }
}

/* ---------------------------------------------------------------------------
 * vga_put_at - Write a character at a specific position
 * ---------------------------------------------------------------------------
 * Parameters:
 *   c   - Character to write
 *   x   - Column (0-79)
 *   y   - Row (0-24)
 * --------------------------------------------------------------------------- */
void vga_put_at(char c, int x, int y)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
        return;
    }
    
    vga_buffer[y * VGA_WIDTH + x] = VGA_ENTRY(c, vga_color);
}

/* ---------------------------------------------------------------------------
 * vga_set_cursor - Move the hardware cursor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   x - Column (0-79)
 *   y - Row (0-24)
 * --------------------------------------------------------------------------- */
void vga_set_cursor(int x, int y)
{
    if (x < 0) x = 0;
    if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
    if (y < 0) y = 0;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;
    
    vga_col = x;
    vga_row = y;
    update_cursor();
}

/* ---------------------------------------------------------------------------
 * vga_get_cursor - Get the current cursor position
 * ---------------------------------------------------------------------------
 * Parameters:
 *   x - Pointer to store column (can be NULL)
 *   y - Pointer to store row (can be NULL)
 * --------------------------------------------------------------------------- */
void vga_get_cursor(int *x, int *y)
{
    if (x) *x = vga_col;
    if (y) *y = vga_row;
}

/* ---------------------------------------------------------------------------
 * vga_print_hex - Print a 32-bit value in hexadecimal
 * ---------------------------------------------------------------------------
 * Parameters:
 *   value - Value to print
 * --------------------------------------------------------------------------- */
void vga_print_hex(uint32_t value)
{
    const char *hex_chars = "0123456789ABCDEF";
    
    vga_write_string("0x");
    
    /* Print 8 hex digits */
    for (int i = 28; i >= 0; i -= 4) {
        vga_putchar(hex_chars[(value >> i) & 0xF]);
    }
}

/* ---------------------------------------------------------------------------
 * vga_print_dec - Print a 32-bit value in decimal
 * ---------------------------------------------------------------------------
 * Parameters:
 *   value - Value to print
 * --------------------------------------------------------------------------- */
void vga_print_dec(uint32_t value)
{
    if (value == 0) {
        vga_putchar('0');
        return;
    }
    
    char buf[12];  /* Max 10 digits for 32-bit + null */
    int i = 0;
    
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    /* Print in reverse order */
    while (--i >= 0) {
        vga_putchar(buf[i]);
    }
}
