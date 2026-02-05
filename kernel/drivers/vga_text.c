/*
 * ===========================================================================
 * kernel/drivers/vga_text.c
 * ===========================================================================
 *
 * VGA Text Mode Driver (Enhanced with Viewports)
 *
 * FAST_HUD_MODE:
 * This driver now divides the screen into three regions:
 * 1. STATUS_BAR (Rows 0-2): Fixed header for system stats
 * 2. SHELL_VIEW (Rows 3-20): Scrolling area for standard output
 * 3. LOG_VIEW   (Rows 21-24): Rolling log for kernel events
 *
 * ===========================================================================
 */

#include "drivers.h"

/* ---------------------------------------------------------------------------
 * External Functions
 * --------------------------------------------------------------------------- */
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern size_t strlen(const char *str); /* Helper if available, else we impl local */

/* ---------------------------------------------------------------------------
 * VGA Constants
 * --------------------------------------------------------------------------- */
#define VGA_BUFFER_ADDR     0xB8000
#define VGA_WIDTH           80
#define VGA_HEIGHT          25
#define VGA_SIZE            (VGA_WIDTH * VGA_HEIGHT)

/* Viewport Definitions */
#define VIEWPORT_STATUS_START   0
#define VIEWPORT_STATUS_HEIGHT  3

#define VIEWPORT_SHELL_START    3
#define VIEWPORT_SHELL_HEIGHT   18
#define VIEWPORT_SHELL_END      (VIEWPORT_SHELL_START + VIEWPORT_SHELL_HEIGHT)

#define VIEWPORT_LOG_START      21
#define VIEWPORT_LOG_HEIGHT     4
#define VIEWPORT_LOG_END        (VIEWPORT_LOG_START + VIEWPORT_LOG_HEIGHT)

/* Scrolling History Definitions */
#define HISTORY_LINES           1000
#define HISTORY_SIZE            (HISTORY_LINES * VGA_WIDTH)

/* VGA Ports */
#define VGA_CRTC_ADDR       0x3D4
#define VGA_CRTC_DATA       0x3D5
#define VGA_CURSOR_HIGH     0x0E
#define VGA_CURSOR_LOW      0x0F

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */
/* Static Variables */
static uint16_t *vga_buffer = (uint16_t *)VGA_BUFFER_ADDR;

/* History Buffer (Ring Buffer) */
static uint16_t history_buffer[HISTORY_SIZE];
static int history_write_line = 0; /* Current line being written to in history */
static int view_top_line = 0;      /* Line in history shown at top of shell viewport */
static bool auto_scroll = true;    /* Start auto-scrolling */

/* Shell Viewport Cursor */
static int vga_col = 0;
/* vga_row is now derived or local, we track history_write_line instead */

/* Log Viewport Cursor */
static int log_row = VIEWPORT_LOG_START;

/* Current Attribute */
static uint8_t vga_color = VGA_DEFAULT_COLOR;

/* ---------------------------------------------------------------------------
 * Helper Functions
 * --------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Internal Helper: Refresh Shell Viewport
 * --------------------------------------------------------------------------- */
static void vga_refresh_shell(void) {
    /* Copy from history buffer to VGA shell viewport */
    for (int y = 0; y < VIEWPORT_SHELL_HEIGHT; y++) {
        /* Calculate source line in history (circular) */
        int hist_line = (view_top_line + y) % HISTORY_LINES;
        
        /* Calculate dest line on screen */
        int screen_row = VIEWPORT_SHELL_START + y;
        
        /* Copy row */
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[screen_row * VGA_WIDTH + x] = history_buffer[hist_line * VGA_WIDTH + x];
        }
    }
    update_cursor();
}

static void update_cursor(void)
{
    /* Calculate cursor position based on visibility */
    /* If the line we are writing to is visible, show cursor. Else hide it? 
       Or just show it at the correct relative position. */
    
    int screen_row = -1;
    
    /* Check distance from view_top_line to history_write_line */
    /* Handle wrapping logic */
    int diff = history_write_line - view_top_line;
    if (diff < 0) diff += HISTORY_LINES;
    
    if (diff >= 0 && diff < VIEWPORT_SHELL_HEIGHT) {
        screen_row = VIEWPORT_SHELL_START + diff;
    }
    
    if (screen_row != -1) {
        uint16_t pos = (uint16_t)(screen_row * VGA_WIDTH + vga_col);
        
        outb(VGA_CRTC_ADDR, VGA_CURSOR_LOW);
        outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
        
        outb(VGA_CRTC_ADDR, VGA_CURSOR_HIGH);
        outb(VGA_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
        
        vga_enable_cursor(true);
    } else {
        /* Cursor out of view */
        vga_enable_cursor(false);
    }
}

void vga_enable_cursor(bool enable)
{
    if (enable) {
        outb(VGA_CRTC_ADDR, 0x0A);
        outb(VGA_CRTC_DATA, 14);
        outb(VGA_CRTC_ADDR, 0x0B);
        outb(VGA_CRTC_DATA, 15);
    } else {
        outb(VGA_CRTC_ADDR, 0x0A);
        outb(VGA_CRTC_DATA, 0x20);
    }
}

/* ---------------------------------------------------------------------------
 * Init & Clear
 * --------------------------------------------------------------------------- */

void vga_init(void)
{
    vga_color = VGA_DEFAULT_COLOR;
    vga_col = 0;
    history_write_line = 0;
    view_top_line = 0;
    log_row = VIEWPORT_LOG_START;
    
    /* Clear history */
    uint16_t blank = VGA_ENTRY(' ', vga_color);
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history_buffer[i] = blank;
    }
    
    vga_clear();
}

void vga_clear(void)
{
    uint16_t blank = VGA_ENTRY(' ', vga_color);
    
    /* Clear entire VGA buffer first */
    for (int i = 0; i < VGA_SIZE; i++) {
        vga_buffer[i] = blank;
    }
    
    /* Clear history buffer */
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history_buffer[i] = blank;
    }
    
    /* Draw Separators */
    uint8_t sep_color = VGA_ENTRY_COLOR(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    uint16_t sep_char = VGA_ENTRY(205, sep_color); /* Double horizontal line */
    
    /* Top separator (Line 2) */
    for(int x=0; x<VGA_WIDTH; x++) {
        vga_buffer[(VIEWPORT_SHELL_START-1) * VGA_WIDTH + x] = sep_char;
    }
    
    /* Bottom separator */
    for(int x=0; x<VGA_WIDTH; x++) {
        vga_buffer[(VIEWPORT_LOG_START-1) * VGA_WIDTH + x] = sep_char;
    }

    vga_col = 0;
    history_write_line = 0;
    view_top_line = 0;
    auto_scroll = true;
    
    vga_refresh_shell();
}

/* ---------------------------------------------------------------------------
 * Shell Viewport Output (Standard Output)
 * --------------------------------------------------------------------------- */

/* Output to Shell Viewport (via History Buffer) */

void vga_putchar(char c)
{
    /* Handle strict viewport bounding for Shell via history */
    
    if (c == '\n') {
        vga_col = 0;
        history_write_line = (history_write_line + 1) % HISTORY_LINES;
        
        /* If we just overwrote a line that was at the top of the view (and we were full), push view down?
           Actually, in a ring buffer, if head hits tail, tail moves. 
           But for simplicity, just handle auto-scroll logic. */
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            history_buffer[history_write_line * VGA_WIDTH + vga_col] = VGA_ENTRY(' ', vga_color);
        }
    } else if (c == '\t') {
        vga_col = (vga_col + 4) & ~3;
    } else {
        history_buffer[history_write_line * VGA_WIDTH + vga_col] = VGA_ENTRY(c, vga_color);
        vga_col++;
    }
    
    /* Wrap */
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        history_write_line = (history_write_line + 1) % HISTORY_LINES;
    }
    
    /* Auto Scroll Logic */
    if (auto_scroll) {
        /* Keep the write line at the bottom of the view */
        /* view_top_line should be history_write_line - (HEIGHT - 1) */
        
        int target_top = history_write_line - (VIEWPORT_SHELL_HEIGHT - 1);
        if (target_top < 0) target_top += HISTORY_LINES;
        
        view_top_line = target_top;
    }
    
    vga_refresh_shell();
}

void vga_write_string(const char *str)
{
    while (*str) vga_putchar(*str++);
}

void vga_write(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) vga_putchar(str[i]);
}

void vga_set_color(uint8_t color)
{
    vga_color = color;
}

/* ---------------------------------------------------------------------------
 * Manual Scroll Functions
 * --------------------------------------------------------------------------- */

void vga_scroll_up(void)
{
    /* Move view UP (back in history) */
    /* Check if we can scroll back */
    /* Simplest limit: don't loop around past the write head? 
       For now, just allow scrolling back arbitrarily within the buffer size 
       or maybe limiting to where we have content. 
       Since we clear buffer spaces, it's fine to show blank history. */
       
    view_top_line--;
    if (view_top_line < 0) view_top_line += HISTORY_LINES;
    
    /* If we scroll up, disable auto-scroll */
    auto_scroll = false;
    
    vga_refresh_shell();
}

void vga_scroll_down(void)
{
    /* Move view DOWN (forward in history) */
    
    /* Calculate the "bottom-most" view top line */
    int target_top = history_write_line - (VIEWPORT_SHELL_HEIGHT - 1);
    if (target_top < 0) target_top += HISTORY_LINES;
    
    if (view_top_line == target_top) {
        /* Already at bottom */
        auto_scroll = true;
        return;
    }
    
    view_top_line = (view_top_line + 1) % HISTORY_LINES;
    
    /* Check if we reached the bottom */
    if (view_top_line == target_top) {
        auto_scroll = true;
    }
    
    vga_refresh_shell();
}

/* ---------------------------------------------------------------------------
 * HUD: Status Bar (Top)
 * --------------------------------------------------------------------------- */
void vga_update_status_bar(const char *left_text, const char *right_text)
{
    /* Clear Line 0 */
    uint8_t color = VGA_ENTRY_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE); // Blue banner
    uint16_t blank = VGA_ENTRY(' ', color);
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[0 * VGA_WIDTH + x] = blank; // Row 0
    }
    
    /* Write Left Text */
    int x = 1;
    const char *p = left_text;
    while (*p && x < VGA_WIDTH) {
        vga_buffer[x++] = VGA_ENTRY(*p++, color);
    }
    
    /* Write Right Text (Aligned Right) */
    /* Find length manually */
    int len = 0;
    const char *cnt = right_text;
    while (*cnt++) len++;
    
    x = VGA_WIDTH - len - 1;
    p = right_text;
    while (*p && x < VGA_WIDTH) {
        vga_buffer[x++] = VGA_ENTRY(*p++, color);
    }
    
    /* Row 1 can be secondary status or blank for now */
    // Kept blank or black for contrast
}

/* ---------------------------------------------------------------------------
 * HUD: Log View (Bottom)
 * --------------------------------------------------------------------------- */
static void vga_scroll_log(void)
{
    /* Scroll log viewport lines up */
    for (int y = VIEWPORT_LOG_START; y < VIEWPORT_LOG_END - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    /* Clear last line of log viewport */
    uint16_t blank = VGA_ENTRY(' ', VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    int last_y = VIEWPORT_LOG_END - 1;
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[last_y * VGA_WIDTH + x] = blank;
    }
}

void vga_log_msg(const char *msg)
{
    /* Write to the log cursor position */
    int x = 0;
    uint8_t color = VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* If we are at the bottom, scroll first */
    if (log_row >= VIEWPORT_LOG_END) {
        vga_scroll_log();
        log_row = VIEWPORT_LOG_END - 1;
    }
    
    /* Clear current log line first */
    for (int i=0; i<VGA_WIDTH; i++) {
        vga_buffer[log_row * VGA_WIDTH + i] = VGA_ENTRY(' ', color);
    }
    
    /* Print prompt */
    const char *prefix = "[LOG] ";
    while (*prefix) {
        vga_buffer[log_row * VGA_WIDTH + x++] = VGA_ENTRY(*prefix++, VGA_ENTRY_COLOR(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    }
    
    /* Print message */
    while (*msg && x < VGA_WIDTH) {
        vga_buffer[log_row * VGA_WIDTH + x++] = VGA_ENTRY(*msg++, color);
    }
    
    /* Advance row */
    log_row++;
}

/* ---------------------------------------------------------------------------
 * Visual Effects
 * --------------------------------------------------------------------------- */
void vga_visual_flash_syscall(void)
{
    /* Flash a small indicator in top right corner */
    uint16_t pos = VGA_WIDTH - 2;
    vga_buffer[pos] = VGA_ENTRY('*', VGA_ENTRY_COLOR(VGA_COLOR_RED, VGA_COLOR_WHITE));
}

void vga_visual_clear_syscall(void)
{
    /* Clear the indicator */
    uint16_t pos = VGA_WIDTH - 2;
    vga_buffer[pos] = VGA_ENTRY(' ', VGA_ENTRY_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
}

/* ---------------------------------------------------------------------------
 * Legacy / Misc
 * --------------------------------------------------------------------------- */

void vga_scroll(void)
{
    /* Legacy: force scrolling text up by adding newline? 
       Or just update view logic. 
       Let's leave it no-op or implementation consistent.*/
    history_write_line = (history_write_line + 1) % HISTORY_LINES;
    if (auto_scroll) {
        int target_top = history_write_line - (VIEWPORT_SHELL_HEIGHT - 1);
        if (target_top < 0) target_top += HISTORY_LINES;
        view_top_line = target_top;
    }
    vga_refresh_shell();
}

void vga_put_at(char c, int x, int y)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    vga_buffer[y * VGA_WIDTH + x] = VGA_ENTRY(c, vga_color);
}

void vga_set_cursor(int x, int y)
{
    /* Clamp to shell viewport for safety in this mode */
    if (y < VIEWPORT_SHELL_START) y = VIEWPORT_SHELL_START;
    if (y >= VIEWPORT_SHELL_END) y = VIEWPORT_SHELL_END - 1;
    
    vga_col = x;
    vga_row = y;
    update_cursor();
}

void vga_get_cursor(int *x, int *y)
{
    if (x) *x = vga_col;
    if (y) *y = vga_row;
}
 
void vga_print_hex(uint32_t value) {
    char buf[16];
    const char *hex = "0123456789ABCDEF";
    vga_write_string("0x");
    for (int i=28; i>=0; i-=4) vga_putchar(hex[(value>>i)&0xF]);
}

void vga_print_dec(uint32_t value) {
    if (value==0) { vga_putchar('0'); return; }
    char buf[12]; int i=0;
    while(value>0) { buf[i++]='0'+(value%10); value/=10; }
    while(--i>=0) vga_putchar(buf[i]);
}
