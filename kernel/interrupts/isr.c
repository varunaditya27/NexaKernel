/*
 * ===========================================================================
 * kernel/interrupts/isr.c
 * ===========================================================================
 *
 * Interrupt Service Routines (ISR) for CPU Exceptions
 *
 * This module handles CPU-generated exceptions (vectors 0-31). These are
 * synchronous interrupts triggered by error conditions during instruction
 * execution, such as:
 *
 *   - Divide by zero (#DE)
 *   - Invalid opcode (#UD)
 *   - Page fault (#PF)
 *   - General protection fault (#GP)
 *   - Double fault (#DF)
 *
 * Each exception has a default handler that displays diagnostic information
 * and halts the system. For recoverable exceptions (like page faults),
 * custom handlers can be registered to handle the condition gracefully.
 *
 * Exception Types:
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │ Type   │ Description                                                    │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │ Fault  │ Can be corrected; execution resumes at faulting instruction    │
 * │ Trap   │ Reported after instruction; execution continues at next instr  │
 * │ Abort  │ Severe error; cannot continue execution                        │
 * └──────────────────────────────────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#include "interrupts.h"

/* ---------------------------------------------------------------------------
 * External Functions
 * --------------------------------------------------------------------------- */
extern void cpu_halt(void);     /* Halt CPU (from startup.asm) */

/* ---------------------------------------------------------------------------
 * Exception Names Table
 * ---------------------------------------------------------------------------
 * Human-readable names for CPU exceptions, indexed by exception number.
 * --------------------------------------------------------------------------- */
static const char *exception_names[32] = {
    "Division Error",               /* 0  #DE - Divide by zero */
    "Debug Exception",              /* 1  #DB - Debug trap */
    "Non-Maskable Interrupt",       /* 2  NMI - External NMI */
    "Breakpoint",                   /* 3  #BP - INT 3 instruction */
    "Overflow",                     /* 4  #OF - INTO instruction */
    "BOUND Range Exceeded",         /* 5  #BR - BOUND instruction */
    "Invalid Opcode",               /* 6  #UD - Undefined opcode */
    "Device Not Available",         /* 7  #NM - FPU not available */
    "Double Fault",                 /* 8  #DF - Exception during exception */
    "Coprocessor Segment Overrun",  /* 9  Legacy, reserved */
    "Invalid TSS",                  /* 10 #TS - Task switch failure */
    "Segment Not Present",          /* 11 #NP - Segment not in memory */
    "Stack-Segment Fault",          /* 12 #SS - Stack segment error */
    "General Protection Fault",     /* 13 #GP - Protection violation */
    "Page Fault",                   /* 14 #PF - Page not present/protection */
    "Reserved",                     /* 15 Reserved */
    "x87 FPU Error",                /* 16 #MF - FPU floating-point error */
    "Alignment Check",              /* 17 #AC - Unaligned memory access */
    "Machine Check",                /* 18 #MC - Internal CPU error */
    "SIMD Floating-Point",          /* 19 #XM - SSE/AVX exception */
    "Virtualization Exception",     /* 20 #VE - EPT violation */
    "Control Protection Exception", /* 21 #CP - CET violation */
    "Reserved",                     /* 22 */
    "Reserved",                     /* 23 */
    "Reserved",                     /* 24 */
    "Reserved",                     /* 25 */
    "Reserved",                     /* 26 */
    "Reserved",                     /* 27 */
    "Reserved",                     /* 28 */
    "Reserved",                     /* 29 */
    "Security Exception",           /* 30 #SX - Security violation */
    "Reserved"                      /* 31 */
};

/* ---------------------------------------------------------------------------
 * Custom Exception Handlers Table
 * ---------------------------------------------------------------------------
 * Array of function pointers for custom exception handlers.
 * NULL means use the default handler.
 * --------------------------------------------------------------------------- */
static isr_handler_t isr_handlers[32] = { NULL };

/* ---------------------------------------------------------------------------
 * VGA Output for Exception Display
 * ---------------------------------------------------------------------------
 * We use direct VGA buffer access for exception output because:
 * 1. It's simpler and more reliable in error conditions
 * 2. The console may not be fully initialized
 * 3. We need output to work even if other subsystems fail
 * --------------------------------------------------------------------------- */
#define VGA_BUFFER      ((uint16_t *)0xB8000)
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_COLOR_ERROR 0x4F    /* White on red */
#define VGA_COLOR_INFO  0x0F    /* White on black */
#define VGA_COLOR_ADDR  0x0B    /* Cyan on black */

/* Current position for exception output */
static int exc_row = 0;
static int exc_col = 0;

/* ---------------------------------------------------------------------------
 * exc_clear_screen - Clear screen with error colors
 * --------------------------------------------------------------------------- */
static void exc_clear_screen(void)
{
    uint16_t *vga = VGA_BUFFER;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (VGA_COLOR_ERROR << 8) | ' ';
    }
    exc_row = 0;
    exc_col = 0;
}

/* ---------------------------------------------------------------------------
 * exc_set_color - Set the current output color
 * --------------------------------------------------------------------------- */
static uint8_t exc_color = VGA_COLOR_ERROR;

static void exc_set_color(uint8_t color)
{
    exc_color = color;
}

/* ---------------------------------------------------------------------------
 * exc_putchar - Output a single character
 * --------------------------------------------------------------------------- */
static void exc_putchar(char c)
{
    if (c == '\n') {
        exc_col = 0;
        exc_row++;
        if (exc_row >= VGA_HEIGHT) {
            exc_row = VGA_HEIGHT - 1;
        }
        return;
    }

    if (exc_col >= VGA_WIDTH) {
        exc_col = 0;
        exc_row++;
        if (exc_row >= VGA_HEIGHT) {
            exc_row = VGA_HEIGHT - 1;
        }
    }

    VGA_BUFFER[exc_row * VGA_WIDTH + exc_col] = (exc_color << 8) | (uint8_t)c;
    exc_col++;
}

/* ---------------------------------------------------------------------------
 * exc_print - Print a null-terminated string
 * --------------------------------------------------------------------------- */
static void exc_print(const char *str)
{
    while (*str) {
        exc_putchar(*str++);
    }
}

/* ---------------------------------------------------------------------------
 * exc_print_hex - Print a 32-bit value in hexadecimal
 * --------------------------------------------------------------------------- */
static void exc_print_hex(uint32_t value)
{
    const char *hex = "0123456789ABCDEF";
    exc_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        exc_putchar(hex[(value >> i) & 0xF]);
    }
}

/* ---------------------------------------------------------------------------
 * exc_print_dec - Print a 32-bit value in decimal
 * --------------------------------------------------------------------------- */
static void exc_print_dec(uint32_t value)
{
    char buf[12];
    int i = 0;
    
    if (value == 0) {
        exc_putchar('0');
        return;
    }
    
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (--i >= 0) {
        exc_putchar(buf[i]);
    }
}

/* ---------------------------------------------------------------------------
 * dump_registers - Display CPU register contents
 * --------------------------------------------------------------------------- */
static void dump_registers(interrupt_frame_t *frame)
{
    exc_set_color(VGA_COLOR_INFO);
    
    exc_print("\n  Registers:\n");
    
    /* General purpose registers */
    exc_print("    EAX="); exc_print_hex(frame->eax);
    exc_print("  EBX="); exc_print_hex(frame->ebx);
    exc_print("  ECX="); exc_print_hex(frame->ecx);
    exc_print("  EDX="); exc_print_hex(frame->edx);
    exc_print("\n");
    
    exc_print("    ESI="); exc_print_hex(frame->esi);
    exc_print("  EDI="); exc_print_hex(frame->edi);
    exc_print("  EBP="); exc_print_hex(frame->ebp);
    exc_print("  ESP="); exc_print_hex(frame->esp);
    exc_print("\n");
    
    /* Segment registers */
    exc_print("    CS="); exc_print_hex(frame->cs & 0xFFFF);
    exc_print("  DS="); exc_print_hex(frame->ds & 0xFFFF);
    exc_print("  ES="); exc_print_hex(frame->es & 0xFFFF);
    exc_print("  SS="); exc_print_hex(frame->ss & 0xFFFF);
    exc_print("\n");
    
    /* Control registers */
    exc_print("    EIP="); exc_print_hex(frame->eip);
    exc_print("  EFLAGS="); exc_print_hex(frame->eflags);
    exc_print("\n");
}

/* ---------------------------------------------------------------------------
 * dump_page_fault_info - Display page fault specific information
 * --------------------------------------------------------------------------- */
static void dump_page_fault_info(interrupt_frame_t *frame)
{
    /* Get the faulting address from CR2 */
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    
    exc_set_color(VGA_COLOR_ADDR);
    exc_print("\n  Page Fault Details:\n");
    exc_set_color(VGA_COLOR_INFO);
    
    exc_print("    Faulting Address: "); 
    exc_print_hex(cr2); 
    exc_print("\n");
    
    exc_print("    Error Code: ");
    exc_print_hex(frame->err_code);
    exc_print(" (");
    
    /* Decode page fault error code */
    if (frame->err_code & 0x1) {
        exc_print("Protection");
    } else {
        exc_print("NotPresent");
    }
    exc_print(", ");
    
    if (frame->err_code & 0x2) {
        exc_print("Write");
    } else {
        exc_print("Read");
    }
    exc_print(", ");
    
    if (frame->err_code & 0x4) {
        exc_print("User");
    } else {
        exc_print("Kernel");
    }
    
    if (frame->err_code & 0x8) {
        exc_print(", ReservedBit");
    }
    
    if (frame->err_code & 0x10) {
        exc_print(", InstructionFetch");
    }
    
    exc_print(")\n");
}

/* ---------------------------------------------------------------------------
 * dump_gp_fault_info - Display general protection fault specific information
 * --------------------------------------------------------------------------- */
static void dump_gp_fault_info(interrupt_frame_t *frame)
{
    exc_set_color(VGA_COLOR_ADDR);
    exc_print("\n  General Protection Fault Details:\n");
    exc_set_color(VGA_COLOR_INFO);
    
    exc_print("    Error Code: ");
    exc_print_hex(frame->err_code);
    
    if (frame->err_code != 0) {
        exc_print("\n    Selector Index: ");
        exc_print_dec((frame->err_code >> 3) & 0x1FFF);
        
        exc_print(", Table: ");
        uint32_t tbl = (frame->err_code >> 1) & 0x3;
        switch (tbl) {
            case 0: exc_print("GDT"); break;
            case 1: exc_print("IDT"); break;
            case 2: exc_print("LDT"); break;
            case 3: exc_print("IDT"); break;
        }
        
        if (frame->err_code & 0x1) {
            exc_print(", External");
        }
    }
    exc_print("\n");
}

/* ---------------------------------------------------------------------------
 * default_exception_handler - Default handler for CPU exceptions
 * ---------------------------------------------------------------------------
 * This handler:
 * 1. Displays detailed exception information
 * 2. Dumps CPU register state
 * 3. Shows exception-specific details
 * 4. Halts the system
 *
 * This is called when no custom handler is registered for an exception.
 * --------------------------------------------------------------------------- */
static void default_exception_handler(interrupt_frame_t *frame)
{
    uint32_t int_no = frame->int_no;
    
    /* Clear screen and display error header */
    exc_clear_screen();
    exc_set_color(VGA_COLOR_ERROR);
    
    exc_print("\n");
    exc_print("  ============================================================\n");
    exc_print("                    KERNEL PANIC - CPU EXCEPTION               \n");
    exc_print("  ============================================================\n\n");
    
    /* Display exception name and number */
    exc_print("  Exception: ");
    if (int_no < 32) {
        exc_print(exception_names[int_no]);
    } else {
        exc_print("Unknown");
    }
    exc_print(" (#");
    exc_print_dec(int_no);
    exc_print(")\n");
    
    /* Display error code if present */
    if (frame->err_code != 0) {
        exc_print("  Error Code: ");
        exc_print_hex(frame->err_code);
        exc_print("\n");
    }
    
    /* Display faulting instruction address */
    exc_set_color(VGA_COLOR_ADDR);
    exc_print("\n  Faulting Instruction: ");
    exc_print_hex(frame->eip);
    exc_print("\n");
    
    /* Dump register state */
    dump_registers(frame);
    
    /* Show exception-specific details */
    if (int_no == ISR_PAGE_FAULT) {
        dump_page_fault_info(frame);
    } else if (int_no == ISR_GENERAL_PROTECTION) {
        dump_gp_fault_info(frame);
    }
    
    /* Final message */
    exc_set_color(VGA_COLOR_ERROR);
    exc_print("\n  ============================================================\n");
    exc_print("  System halted. Please restart your computer.\n");
    exc_print("  ============================================================\n");
    
    /* Halt the CPU */
    cpu_halt();
    
    /* Should never reach here */
    while (1) { }
}

/* ---------------------------------------------------------------------------
 * isr_register_handler - Register a custom exception handler
 * ---------------------------------------------------------------------------
 * Parameters:
 *   isr_num - Exception number (0-31)
 *   handler - Function to call when exception occurs (NULL to use default)
 * --------------------------------------------------------------------------- */
void isr_register_handler(uint8_t isr_num, isr_handler_t handler)
{
    if (isr_num < 32) {
        isr_handlers[isr_num] = handler;
    }
}

/* ---------------------------------------------------------------------------
 * isr_handler - Common C handler for all CPU exceptions
 * ---------------------------------------------------------------------------
 * This function is called from the assembly stub (isr_common_stub) after
 * the CPU state has been saved. It dispatches to the appropriate handler.
 *
 * Parameters:
 *   frame - Pointer to the saved CPU state on the stack
 * --------------------------------------------------------------------------- */
void isr_handler(interrupt_frame_t *frame)
{
    uint32_t int_no = frame->int_no;
    
    /* Check if we have a custom handler */
    if (int_no < 32 && isr_handlers[int_no] != NULL) {
        /* Call the custom handler */
        isr_handlers[int_no](frame);
    } else {
        /* Use the default exception handler */
        default_exception_handler(frame);
    }
}

/* ---------------------------------------------------------------------------
 * isr_init - Initialize CPU exception handlers
 * ---------------------------------------------------------------------------
 * Sets up the exception handling subsystem. This function should be called
 * after idt_init() but before enabling interrupts.
 *
 * Note: The actual installation of ISR stubs into the IDT is done by
 * idt_init(). This function only initializes internal data structures
 * and can register custom handlers if needed.
 * --------------------------------------------------------------------------- */
void isr_init(void)
{
    /* Clear all custom handlers - use defaults */
    for (int i = 0; i < 32; i++) {
        isr_handlers[i] = NULL;
    }
    
    /*
     * Future enhancement: Register custom handlers for recoverable exceptions
     * 
     * Example:
     *   isr_register_handler(ISR_PAGE_FAULT, page_fault_handler);
     *   isr_register_handler(ISR_BREAKPOINT, breakpoint_handler);
     */
}
