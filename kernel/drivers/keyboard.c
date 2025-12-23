/*
 * ===========================================================================
 * kernel/drivers/keyboard.c
 * ===========================================================================
 *
 * PS/2 Keyboard Driver
 *
 * This driver handles input from a PS/2 keyboard. It processes scancodes
 * from the keyboard controller (8042) and translates them into ASCII
 * characters or key events.
 *
 * Features:
 * - Scancode set 1 support (default on PC)
 * - Shift, Ctrl, Alt modifier handling
 * - Caps Lock, Num Lock, Scroll Lock
 * - Keyboard buffer for asynchronous input
 * - Optional callback for key events
 *
 * PS/2 Controller Architecture:
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │  The 8042 keyboard controller has two main ports:                        │
 * │  - Port 0x60: Data port (read scancodes, write commands to keyboard)     │
 * │  - Port 0x64: Status/command port                                        │
 * │                                                                          │
 * │  Scancode format:                                                        │
 * │  - Key press:   scancode                                                 │
 * │  - Key release: scancode | 0x80                                          │
 * │                                                                          │
 * │  Extended keys (arrows, etc.): 0xE0 prefix followed by scancode          │
 * └──────────────────────────────────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#include "drivers.h"
#include "../interrupts/interrupts.h"

/* ---------------------------------------------------------------------------
 * External Functions (from startup.asm)
 * --------------------------------------------------------------------------- */
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern void io_wait(void);

/* ---------------------------------------------------------------------------
 * PS/2 Controller Ports
 * --------------------------------------------------------------------------- */
#define KB_DATA_PORT    0x60    /* Data port (read/write) */
#define KB_STATUS_PORT  0x64    /* Status port (read) */
#define KB_COMMAND_PORT 0x64    /* Command port (write) */

/* ---------------------------------------------------------------------------
 * PS/2 Controller Status Register Bits
 * --------------------------------------------------------------------------- */
#define KB_STATUS_OUTPUT_FULL   0x01    /* Output buffer full (data available) */
#define KB_STATUS_INPUT_FULL    0x02    /* Input buffer full (busy) */
#define KB_STATUS_SYSTEM        0x04    /* System flag */
#define KB_STATUS_CMD_DATA      0x08    /* Command/data (0=data, 1=command) */
#define KB_STATUS_TIMEOUT       0x40    /* Timeout error */
#define KB_STATUS_PARITY        0x80    /* Parity error */

/* ---------------------------------------------------------------------------
 * PS/2 Keyboard Commands
 * --------------------------------------------------------------------------- */
#define KB_CMD_SET_LEDS         0xED    /* Set LEDs */
#define KB_CMD_ECHO             0xEE    /* Echo */
#define KB_CMD_GET_SCANCODE_SET 0xF0    /* Get/set scancode set */
#define KB_CMD_IDENTIFY         0xF2    /* Identify keyboard */
#define KB_CMD_SET_TYPEMATIC    0xF3    /* Set typematic rate/delay */
#define KB_CMD_ENABLE           0xF4    /* Enable scanning */
#define KB_CMD_DISABLE          0xF5    /* Disable scanning */
#define KB_CMD_RESET            0xFF    /* Reset keyboard */

/* ---------------------------------------------------------------------------
 * PS/2 Keyboard Response Codes
 * --------------------------------------------------------------------------- */
#define KB_RESPONSE_ACK         0xFA    /* Acknowledge */
#define KB_RESPONSE_RESEND      0xFE    /* Resend last command */
#define KB_RESPONSE_ERROR       0xFC    /* Error */

/* ---------------------------------------------------------------------------
 * Scancode Constants
 * --------------------------------------------------------------------------- */
#define SCANCODE_EXTENDED       0xE0    /* Extended key prefix */
#define SCANCODE_RELEASE        0x80    /* Key release flag */

/* ---------------------------------------------------------------------------
 * Scancode to ASCII Mapping (Scancode Set 1)
 * ---------------------------------------------------------------------------
 * Index = scancode, value = ASCII character (0 = no ASCII mapping)
 * This is for US QWERTY keyboard layout.
 * --------------------------------------------------------------------------- */
static const char scancode_to_ascii[128] = {
    0,    0x1B, '1',  '2',  '3',  '4',  '5',  '6',    /* 0x00-0x07: Esc, 1-6 */
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',   /* 0x08-0x0F: 7-0, -, =, BS, Tab */
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',    /* 0x10-0x17: q-i */
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',    /* 0x18-0x1F: o-p, [], Enter, Ctrl, a-s */
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',    /* 0x20-0x27: d-l, ; */
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',    /* 0x28-0x2F: ', `, LShift, \, z-v */
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',    /* 0x30-0x37: b-m, , . /, RShift, KP* */
    0,    ' ',  0,    0,    0,    0,    0,    0,      /* 0x38-0x3F: Alt, Space, CapsLk, F1-F5 */
    0,    0,    0,    0,    0,    0,    0,    '7',    /* 0x40-0x47: F6-F10, NumLk, ScrLk, KP7 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',    /* 0x48-0x4F: KP8-9, KP-, KP4-6, KP+, KP1 */
    '2',  '3',  '0',  '.',  0,    0,    0,    0,      /* 0x50-0x57: KP2-3, KP0, KP., ?, ?, F11-F12 */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x58-0x5F */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x60-0x67 */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x68-0x6F */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x70-0x77 */
    0,    0,    0,    0,    0,    0,    0,    0       /* 0x78-0x7F */
};

/* ---------------------------------------------------------------------------
 * Shifted Scancode to ASCII Mapping
 * --------------------------------------------------------------------------- */
static const char scancode_to_ascii_shifted[128] = {
    0,    0x1B, '!',  '@',  '#',  '$',  '%',  '^',    /* 0x00-0x07 */
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',   /* 0x08-0x0F */
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',    /* 0x10-0x17 */
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',    /* 0x18-0x1F */
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',    /* 0x20-0x27 */
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',    /* 0x28-0x2F */
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',    /* 0x30-0x37 */
    0,    ' ',  0,    0,    0,    0,    0,    0,      /* 0x38-0x3F */
    0,    0,    0,    0,    0,    0,    0,    '7',    /* 0x40-0x47 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',    /* 0x48-0x4F */
    '2',  '3',  '0',  '.',  0,    0,    0,    0,      /* 0x50-0x57 */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x58-0x5F */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x60-0x67 */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x68-0x6F */
    0,    0,    0,    0,    0,    0,    0,    0,      /* 0x70-0x77 */
    0,    0,    0,    0,    0,    0,    0,    0       /* 0x78-0x7F */
};

/* ---------------------------------------------------------------------------
 * Keyboard State Variables
 * --------------------------------------------------------------------------- */

/* Modifier key states */
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock = false;
static bool num_lock = false;
static bool scroll_lock = false;

/* Extended scancode flag */
static bool extended_key = false;

/* ---------------------------------------------------------------------------
 * Keyboard Buffer (Circular Buffer)
 * ---------------------------------------------------------------------------
 * Simple ring buffer for storing characters until they're read.
 * --------------------------------------------------------------------------- */
#define KB_BUFFER_SIZE  KEYBOARD_BUFFER_SIZE

static char kb_buffer[KB_BUFFER_SIZE];
static volatile uint32_t kb_head = 0;   /* Write position */
static volatile uint32_t kb_tail = 0;   /* Read position */

/* Optional callback for key events */
static keyboard_callback_t key_callback = NULL;

/* ---------------------------------------------------------------------------
 * Buffer Helper Functions
 * --------------------------------------------------------------------------- */

/* Check if buffer is full */
static bool kb_buffer_full(void)
{
    return ((kb_head + 1) % KB_BUFFER_SIZE) == kb_tail;
}

/* Check if buffer is empty */
static bool kb_buffer_empty(void)
{
    return kb_head == kb_tail;
}

/* Add a character to the buffer */
static void kb_buffer_put(char c)
{
    if (!kb_buffer_full()) {
        kb_buffer[kb_head] = c;
        kb_head = (kb_head + 1) % KB_BUFFER_SIZE;
    }
}

/* Get a character from the buffer */
static char kb_buffer_get(void)
{
    if (kb_buffer_empty()) {
        return 0;
    }
    
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

/* ---------------------------------------------------------------------------
 * keyboard_wait_input - Wait until the controller is ready for input
 * --------------------------------------------------------------------------- */
static void keyboard_wait_input(void)
{
    int timeout = 100000;
    while ((inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL) && timeout > 0) {
        timeout--;
    }
}

/* ---------------------------------------------------------------------------
 * keyboard_wait_output - Wait until the controller has output ready
 * --------------------------------------------------------------------------- */
static void keyboard_wait_output(void)
{
    int timeout = 100000;
    while (!(inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) && timeout > 0) {
        timeout--;
    }
}

/* ---------------------------------------------------------------------------
 * keyboard_send_command - Send a command to the keyboard
 * --------------------------------------------------------------------------- */
static void keyboard_send_command(uint8_t cmd)
{
    keyboard_wait_input();
    outb(KB_DATA_PORT, cmd);
}

/* ---------------------------------------------------------------------------
 * keyboard_set_leds - Set the keyboard LEDs
 * ---------------------------------------------------------------------------
 * Parameters:
 *   scroll_lock - Scroll Lock LED state
 *   num_lock    - Num Lock LED state
 *   caps_lock   - Caps Lock LED state
 * --------------------------------------------------------------------------- */
void keyboard_set_leds(bool scroll, bool num, bool caps)
{
    uint8_t leds = 0;
    if (scroll) leds |= 0x01;
    if (num)    leds |= 0x02;
    if (caps)   leds |= 0x04;
    
    keyboard_send_command(KB_CMD_SET_LEDS);
    keyboard_wait_input();
    outb(KB_DATA_PORT, leds);
}

/* ---------------------------------------------------------------------------
 * process_scancode - Process a scancode and update keyboard state
 * ---------------------------------------------------------------------------
 * This is the main scancode processing function. It:
 * 1. Handles extended key prefix (0xE0)
 * 2. Determines if key was pressed or released
 * 3. Updates modifier key states
 * 4. Translates to ASCII and adds to buffer
 * --------------------------------------------------------------------------- */
static void process_scancode(uint8_t scancode)
{
    /* Handle extended key prefix */
    if (scancode == SCANCODE_EXTENDED) {
        extended_key = true;
        return;
    }

    /* Determine if this is a key press or release */
    bool released = (scancode & SCANCODE_RELEASE) != 0;
    uint8_t key = scancode & 0x7F;  /* Remove release bit */

    /* Create key event */
    key_event_t event;
    event.scancode = scancode;
    event.ascii = 0;
    event.pressed = !released;
    event.shift = shift_pressed;
    event.ctrl = ctrl_pressed;
    event.alt = alt_pressed;

    /* Handle modifier keys */
    if (key == KEY_LSHIFT || key == KEY_RSHIFT) {
        shift_pressed = !released;
        extended_key = false;
        return;
    }
    
    if (key == KEY_CTRL) {
        ctrl_pressed = !released;
        extended_key = false;
        return;
    }
    
    if (key == KEY_ALT) {
        alt_pressed = !released;
        extended_key = false;
        return;
    }

    /* Handle Caps Lock (toggle on press) */
    if (key == KEY_CAPS_LOCK && !released) {
        caps_lock = !caps_lock;
        keyboard_set_leds(scroll_lock, num_lock, caps_lock);
        extended_key = false;
        return;
    }

    /* Handle Num Lock (toggle on press) */
    if (key == KEY_NUM_LOCK && !released) {
        num_lock = !num_lock;
        keyboard_set_leds(scroll_lock, num_lock, caps_lock);
        extended_key = false;
        return;
    }

    /* Handle Scroll Lock (toggle on press) */
    if (key == KEY_SCROLL_LOCK && !released) {
        scroll_lock = !scroll_lock;
        keyboard_set_leds(scroll_lock, num_lock, caps_lock);
        extended_key = false;
        return;
    }

    /* Only process key presses for character input */
    if (!released && key < 128) {
        char c;
        
        /* Determine if we should use shifted mapping */
        bool use_shift = shift_pressed;
        
        /* Caps Lock affects letter keys */
        if (caps_lock) {
            char base = scancode_to_ascii[key];
            if (base >= 'a' && base <= 'z') {
                use_shift = !use_shift;  /* Toggle shift for letters */
            }
        }
        
        /* Get ASCII character */
        if (use_shift) {
            c = scancode_to_ascii_shifted[key];
        } else {
            c = scancode_to_ascii[key];
        }
        
        event.ascii = (uint8_t)c;

        /* Handle Ctrl combinations */
        if (ctrl_pressed && c >= 'a' && c <= 'z') {
            c = c - 'a' + 1;  /* Ctrl+A = 1, Ctrl+B = 2, etc. */
            event.ascii = (uint8_t)c;
        }

        /* Add to buffer if it's a printable character or control character */
        if (c != 0) {
            kb_buffer_put(c);
        }
    }

    /* Call the callback if registered */
    if (key_callback != NULL) {
        key_callback(&event);
    }

    extended_key = false;
}

/* ---------------------------------------------------------------------------
 * keyboard_irq_handler - IRQ1 handler for keyboard interrupts
 * ---------------------------------------------------------------------------
 * Called when the keyboard generates an interrupt (key press/release).
 * --------------------------------------------------------------------------- */
static void keyboard_irq_handler(interrupt_frame_t *frame)
{
    UNUSED(frame);

    /* Read the scancode from the data port */
    uint8_t scancode = inb(KB_DATA_PORT);

    /* Process the scancode */
    process_scancode(scancode);
}

/* ---------------------------------------------------------------------------
 * keyboard_init - Initialize the keyboard driver
 * ---------------------------------------------------------------------------
 * Sets up the keyboard controller and registers the IRQ handler.
 * --------------------------------------------------------------------------- */
void keyboard_init(void)
{
    /* Initialize state */
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    caps_lock = false;
    num_lock = false;
    scroll_lock = false;
    extended_key = false;
    
    kb_head = 0;
    kb_tail = 0;
    key_callback = NULL;

    /* Flush any pending data from the keyboard buffer */
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) {
        inb(KB_DATA_PORT);
    }

    /* Set up the keyboard LEDs */
    keyboard_set_leds(scroll_lock, num_lock, caps_lock);

    /* Register our IRQ handler */
    irq_register_handler(IRQ1_KEYBOARD, keyboard_irq_handler);

    /* Enable IRQ1 (keyboard) */
    irq_enable(IRQ1_KEYBOARD);
}

/* ---------------------------------------------------------------------------
 * keyboard_getchar - Get a character from the keyboard buffer
 * ---------------------------------------------------------------------------
 * Returns:
 *   ASCII character from the buffer, or 0 if buffer is empty
 *
 * This is a non-blocking call.
 * --------------------------------------------------------------------------- */
char keyboard_getchar(void)
{
    return kb_buffer_get();
}

/* ---------------------------------------------------------------------------
 * keyboard_getchar_blocking - Wait for and get a character
 * ---------------------------------------------------------------------------
 * Returns:
 *   ASCII character from keyboard input
 *
 * This function blocks until a key is pressed.
 * --------------------------------------------------------------------------- */
char keyboard_getchar_blocking(void)
{
    while (kb_buffer_empty()) {
        /* Hint to CPU that we're in a spin loop */
        __asm__ volatile("pause");
    }
    return kb_buffer_get();
}

/* ---------------------------------------------------------------------------
 * keyboard_get_event - Get a key event from the event queue
 * ---------------------------------------------------------------------------
 * Note: This simplified implementation doesn't maintain an event queue.
 * For full event support, you'd need a separate event buffer.
 *
 * Parameters:
 *   event - Pointer to key_event_t to fill
 *
 * Returns:
 *   true if an event was available, false if queue is empty
 * --------------------------------------------------------------------------- */
bool keyboard_get_event(key_event_t *event)
{
    /* Simple implementation: get character and create minimal event */
    char c = keyboard_getchar();
    if (c == 0) {
        return false;
    }
    
    event->scancode = 0;  /* Not available in simple mode */
    event->ascii = (uint8_t)c;
    event->pressed = true;
    event->shift = false;
    event->ctrl = false;
    event->alt = false;
    
    return true;
}

/* ---------------------------------------------------------------------------
 * keyboard_has_input - Check if there's input waiting
 * ---------------------------------------------------------------------------
 * Returns:
 *   true if the keyboard buffer has characters
 * --------------------------------------------------------------------------- */
bool keyboard_has_input(void)
{
    return !kb_buffer_empty();
}

/* ---------------------------------------------------------------------------
 * keyboard_clear_buffer - Clear the keyboard buffer
 * ---------------------------------------------------------------------------
 * Discards all pending input.
 * --------------------------------------------------------------------------- */
void keyboard_clear_buffer(void)
{
    kb_head = 0;
    kb_tail = 0;
}

/* ---------------------------------------------------------------------------
 * keyboard_get_modifiers - Get the current modifier key state
 * ---------------------------------------------------------------------------
 * Parameters:
 *   shift - Pointer to store shift state (can be NULL)
 *   ctrl  - Pointer to store ctrl state (can be NULL)
 *   alt   - Pointer to store alt state (can be NULL)
 * --------------------------------------------------------------------------- */
void keyboard_get_modifiers(bool *shift, bool *ctrl, bool *alt)
{
    if (shift) *shift = shift_pressed;
    if (ctrl)  *ctrl = ctrl_pressed;
    if (alt)   *alt = alt_pressed;
}

/* ---------------------------------------------------------------------------
 * keyboard_register_callback - Register a callback for key events
 * ---------------------------------------------------------------------------
 * Parameters:
 *   callback - Function to call when a key is pressed (NULL to disable)
 *
 * The callback receives key events before they're added to the buffer.
 * --------------------------------------------------------------------------- */
void keyboard_register_callback(keyboard_callback_t callback)
{
    key_callback = callback;
}
