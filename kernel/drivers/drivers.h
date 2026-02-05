/*
 * ===========================================================================
 * kernel/drivers/drivers.h
 * ===========================================================================
 *
 * NexaKernel - Device Drivers Public Interface
 *
 * This header provides the public API for all device drivers including:
 * - VGA text mode driver (console output)
 * - PIT timer driver (system tick)
 * - PS/2 keyboard driver (input)
 *
 * ===========================================================================
 */

#ifndef DRIVERS_H
#define DRIVERS_H

#include "../../config/os_config.h"

/* ===========================================================================
 * VGA Text Mode Driver (vga_text.c)
 * ===========================================================================
 * The VGA text mode driver provides console output by directly writing to
 * the VGA text buffer at 0xB8000. It supports:
 * - Character and string output
 * - Cursor management
 * - Screen scrolling
 * - Color attributes
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * VGA Color Constants
 * --------------------------------------------------------------------------- */
typedef enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/* Create a VGA color byte from foreground and background colors */
#define VGA_ENTRY_COLOR(fg, bg) ((uint8_t)((bg) << 4 | (fg)))

/* Create a VGA entry from character and color */
#define VGA_ENTRY(c, color) ((uint16_t)(c) | ((uint16_t)(color) << 8))

/* Default colors */
#define VGA_DEFAULT_COLOR   VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)
#define VGA_ERROR_COLOR     VGA_ENTRY_COLOR(VGA_COLOR_WHITE, VGA_COLOR_RED)
#define VGA_SUCCESS_COLOR   VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK)
#define VGA_WARNING_COLOR   VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK)

/* ---------------------------------------------------------------------------
 * VGA Driver Functions
 * --------------------------------------------------------------------------- */

/*
 * vga_init - Initialize the VGA text mode driver
 * ---------------------------------------------------------------------------
 * Clears the screen, resets cursor position, and sets default colors.
 */
void vga_init(void);

/*
 * vga_clear - Clear the entire screen
 * ---------------------------------------------------------------------------
 * Fills the screen with spaces using the current background color.
 */
void vga_clear(void);

/*
 * vga_set_color - Set the current text color
 * ---------------------------------------------------------------------------
 * Parameters:
 *   color - VGA color byte (use VGA_ENTRY_COLOR macro)
 */
void vga_set_color(uint8_t color);

/*
 * vga_putchar - Write a single character at the cursor position
 * ---------------------------------------------------------------------------
 * Parameters:
 *   c - Character to write
 *
 * Handles special characters: '\n' (newline), '\r' (carriage return),
 * '\t' (tab), '\b' (backspace)
 */
void vga_putchar(char c);

/*
 * vga_write_string - Write a null-terminated string
 * ---------------------------------------------------------------------------
 * Parameters:
 *   str - Null-terminated string to write
 */
void vga_write_string(const char *str);

/*
 * vga_write - Write a string with explicit length
 * ---------------------------------------------------------------------------
 * Parameters:
 *   str - String to write (doesn't need to be null-terminated)
 *   len - Number of characters to write
 */
void vga_write(const char *str, size_t len);

/*
 * vga_put_at - Write a character at a specific position
 * ---------------------------------------------------------------------------
 * Parameters:
 *   c   - Character to write
 *   x   - Column (0-79)
 *   y   - Row (0-24)
 */
void vga_put_at(char c, int x, int y);

/*
 * vga_set_cursor - Move the hardware cursor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   x - Column (0-79)
 *   y - Row (0-24)
 */
void vga_set_cursor(int x, int y);

/*
 * vga_get_cursor - Get the current cursor position
 * ---------------------------------------------------------------------------
 * Parameters:
 *   x - Pointer to store column (can be NULL)
 *   y - Pointer to store row (can be NULL)
 */
void vga_get_cursor(int *x, int *y);

/*
 * vga_enable_cursor - Enable/disable the hardware cursor
 * ---------------------------------------------------------------------------
 * Parameters:
 *   enable - true to show cursor, false to hide
 */
void vga_enable_cursor(bool enable);

/*
 * vga_scroll - Scroll the screen up by one line
 * ---------------------------------------------------------------------------
 * Moves all lines up by one, clearing the bottom line.
 */
void vga_scroll(void);

/*
 * vga_print_hex - Print a 32-bit value in hexadecimal
 * ---------------------------------------------------------------------------
 * Parameters:
 *   value - Value to print
 */
void vga_print_hex(uint32_t value);

/*
 * vga_print_dec - Print a 32-bit value in decimal
 * ---------------------------------------------------------------------------
 * Parameters:
 *   value - Value to print
 */
void vga_print_dec(uint32_t value);

/* ===========================================================================
 * Programmable Interval Timer (PIT) Driver (timer.c)
 * ===========================================================================
 * The PIT driver configures the 8253/8254 timer chip to generate periodic
 * interrupts. These interrupts drive:
 * - System tick counter
 * - Preemptive scheduler
 * - Sleep/delay functions
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * Timer Driver Functions
 * --------------------------------------------------------------------------- */

/*
 * pit_init - Initialize the Programmable Interval Timer
 * ---------------------------------------------------------------------------
 * Configures the PIT for periodic interrupts and registers the IRQ handler.
 * Default frequency is SCHEDULER_TICK_HZ from os_config.h (typically 100 Hz).
 */
void pit_init(void);

/*
 * pit_set_frequency - Set the timer interrupt frequency
 * ---------------------------------------------------------------------------
 * Parameters:
 *   hz - Desired frequency in Hz (minimum ~19 Hz due to hardware limits)
 */
void pit_set_frequency(uint32_t hz);

/*
 * pit_get_ticks - Get the current tick count
 * ---------------------------------------------------------------------------
 * Returns:
 *   Number of timer ticks since boot
 */
uint32_t pit_get_ticks(void);

/*
 * pit_get_uptime_ms - Get system uptime in milliseconds
 * ---------------------------------------------------------------------------
 * Returns:
 *   Milliseconds since boot
 */
uint32_t pit_get_uptime_ms(void);

/*
 * pit_get_uptime_sec - Get system uptime in seconds
 * ---------------------------------------------------------------------------
 * Returns:
 *   Seconds since boot
 */
uint32_t pit_get_uptime_sec(void);

/*
 * pit_sleep_ms - Sleep for a specified number of milliseconds
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ms - Number of milliseconds to sleep
 *
 * Note: This is a busy-wait implementation. For real multitasking,
 * use the scheduler's sleep functionality instead.
 */
void pit_sleep_ms(uint32_t ms);

/*
 * pit_sleep_ticks - Sleep for a specified number of ticks
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ticks - Number of timer ticks to sleep
 */
void pit_sleep_ticks(uint32_t ticks);

/*
 * pit_register_callback - Register a callback for each timer tick
 * ---------------------------------------------------------------------------
 * Parameters:
 *   callback - Function to call on each tick (NULL to disable)
 *
 * This is used by the scheduler to get notified of each tick.
 */
typedef void (*pit_callback_t)(void);
void pit_register_callback(pit_callback_t callback);

/* ===========================================================================
 * PS/2 Keyboard Driver (keyboard.c)
 * ===========================================================================
 * The keyboard driver handles input from a PS/2 keyboard. It:
 * - Processes scancodes from the keyboard controller
 * - Translates scancodes to ASCII characters
 * - Maintains a keyboard buffer for input
 * - Handles special keys (shift, ctrl, alt)
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * Key Event Structure
 * --------------------------------------------------------------------------- */
typedef struct key_event {
    uint8_t scancode;       /* Raw scancode from keyboard */
    uint8_t ascii;          /* ASCII character (0 if not printable) */
    bool    pressed;        /* true = key pressed, false = key released */
    bool    shift;          /* Shift key state */
    bool    ctrl;           /* Ctrl key state */
    bool    alt;            /* Alt key state */
} key_event_t;

/* ---------------------------------------------------------------------------
 * Special Key Codes (for non-ASCII keys)
 * --------------------------------------------------------------------------- */
#define KEY_ESCAPE      0x01
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_ENTER       0x1C
#define KEY_CTRL        0x1D
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_ALT         0x38
#define KEY_SPACE       0x39
#define KEY_CAPS_LOCK   0x3A
#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F3          0x3D
#define KEY_F4          0x3E
#define KEY_F5          0x3F
#define KEY_F6          0x40
#define KEY_F7          0x41
#define KEY_F8          0x42
#define KEY_F9          0x43
#define KEY_F10         0x44
#define KEY_NUM_LOCK    0x45
#define KEY_SCROLL_LOCK 0x46
#define KEY_HOME        0x47
#define KEY_UP          0x48
#define KEY_PAGE_UP     0x49
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_END         0x4F
#define KEY_DOWN        0x50
#define KEY_PAGE_DOWN   0x51
#define KEY_INSERT      0x52
#define KEY_DELETE      0x53
#define KEY_F11         0x57
#define KEY_F12         0x58

/* ---------------------------------------------------------------------------
 * Keyboard Driver Functions
 * --------------------------------------------------------------------------- */

/*
 * keyboard_init - Initialize the keyboard driver
 * ---------------------------------------------------------------------------
 * Sets up the keyboard controller and registers the IRQ handler.
 */
void keyboard_init(void);

/*
 * keyboard_getchar - Get a character from the keyboard buffer
 * ---------------------------------------------------------------------------
 * Returns:
 *   ASCII character from the buffer, or 0 if buffer is empty
 *
 * This is a non-blocking call.
 */
char keyboard_getchar(void);

/*
 * keyboard_getchar_blocking - Wait for and get a character
 * ---------------------------------------------------------------------------
 * Returns:
 *   ASCII character from keyboard input
 *
 * This function blocks until a key is pressed.
 */
char keyboard_getchar_blocking(void);

/*
 * keyboard_get_event - Get a key event from the event queue
 * ---------------------------------------------------------------------------
 * Parameters:
 *   event - Pointer to key_event_t to fill
 *
 * Returns:
 *   true if an event was available, false if queue is empty
 */
bool keyboard_get_event(key_event_t *event);

/*
 * keyboard_has_input - Check if there's input waiting
 * ---------------------------------------------------------------------------
 * Returns:
 *   true if the keyboard buffer has characters
 */
bool keyboard_has_input(void);

/*
 * keyboard_clear_buffer - Clear the keyboard buffer
 * ---------------------------------------------------------------------------
 * Discards all pending input.
 */
void keyboard_clear_buffer(void);

/*
 * keyboard_get_modifiers - Get the current modifier key state
 * ---------------------------------------------------------------------------
 * Parameters:
 *   shift - Pointer to store shift state (can be NULL)
 *   ctrl  - Pointer to store ctrl state (can be NULL)
 *   alt   - Pointer to store alt state (can be NULL)
 */
void keyboard_get_modifiers(bool *shift, bool *ctrl, bool *alt);

/*
 * keyboard_set_leds - Set the keyboard LEDs
 * ---------------------------------------------------------------------------
 * Parameters:
 *   scroll_lock - Scroll Lock LED state
 *   num_lock    - Num Lock LED state
 *   caps_lock   - Caps Lock LED state
 */
void keyboard_set_leds(bool scroll_lock, bool num_lock, bool caps_lock);

/*
 * keyboard_register_callback - Register a callback for key events
 * ---------------------------------------------------------------------------
 * Parameters:
 *   callback - Function to call when a key is pressed (NULL to disable)
 *
 * The callback receives key events before they're added to the buffer.
 */
typedef void (*keyboard_callback_t)(key_event_t *event);
void keyboard_register_callback(keyboard_callback_t callback);

/* ===========================================================================
 * Serial Port (UART) Driver (serial.c)
 * ===========================================================================
 * The serial driver provides communication via the COM1 port. This is
 * primarily used for kernel debugging and logging to the host terminal.
 * =========================================================================== */

#define SERIAL_COM1_PORT    0x3F8

/**
 * @brief Initialize the serial port
 * @return 0 on success, non-zero on failure
 */
int serial_init(void);

/**
 * @brief Write a character to the serial port
 * @param c Character to write
 */
void serial_putchar(char c);

/**
 * @brief Write a string to the serial port
 * @param str String to write
 */
void serial_write_string(const char *str);

/**
 * @brief Check if serial port has received data
 * @return true if data is available
 */
bool serial_received(void);

/**
 * @brief Read a character from the serial port
 * @return Character read
 */
char serial_getchar(void);

#endif /* DRIVERS_H */
