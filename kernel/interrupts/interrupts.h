/*
 * ===========================================================================
 * kernel/interrupts/interrupts.h
 * ===========================================================================
 *
 * NexaKernel - Interrupt Handling Subsystem Public Interface
 *
 * This header provides the public API for the interrupt handling subsystem.
 * It includes functions for initializing the IDT, registering interrupt
 * handlers, and managing CPU exceptions and hardware IRQs.
 *
 * Architecture: x86 Protected Mode (32-bit)
 *
 * Interrupt Number Mapping:
 *   0-31:   CPU Exceptions (ISRs)
 *   32-47:  Hardware IRQs (remapped PIC)
 *   48-255: Software interrupts (available for syscalls, etc.)
 *
 * ===========================================================================
 */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------------- */

/* Number of IDT entries (x86 supports 256 interrupts) */
#define IDT_ENTRIES         256

/* CPU Exception Vectors (ISR 0-31) */
#define ISR_DIVIDE_ERROR            0   /* Division by zero */
#define ISR_DEBUG                   1   /* Debug exception */
#define ISR_NMI                     2   /* Non-maskable interrupt */
#define ISR_BREAKPOINT              3   /* Breakpoint (INT 3) */
#define ISR_OVERFLOW                4   /* Overflow (INTO instruction) */
#define ISR_BOUND_RANGE             5   /* BOUND range exceeded */
#define ISR_INVALID_OPCODE          6   /* Invalid opcode */
#define ISR_DEVICE_NOT_AVAILABLE    7   /* Device not available (no FPU) */
#define ISR_DOUBLE_FAULT            8   /* Double fault */
#define ISR_COPROCESSOR_SEGMENT     9   /* Coprocessor segment overrun */
#define ISR_INVALID_TSS             10  /* Invalid TSS */
#define ISR_SEGMENT_NOT_PRESENT     11  /* Segment not present */
#define ISR_STACK_SEGMENT_FAULT     12  /* Stack-segment fault */
#define ISR_GENERAL_PROTECTION      13  /* General protection fault */
#define ISR_PAGE_FAULT              14  /* Page fault */
#define ISR_RESERVED_15             15  /* Reserved */
#define ISR_X87_FPU_ERROR           16  /* x87 FPU floating-point error */
#define ISR_ALIGNMENT_CHECK         17  /* Alignment check */
#define ISR_MACHINE_CHECK           18  /* Machine check */
#define ISR_SIMD_FP_EXCEPTION       19  /* SIMD floating-point exception */
#define ISR_VIRTUALIZATION          20  /* Virtualization exception */
#define ISR_CONTROL_PROTECTION      21  /* Control protection exception */
/* 22-31 are reserved */

/* Hardware IRQ Numbers (after PIC remapping) */
#define IRQ_BASE                    32  /* First IRQ vector after remapping */

#define IRQ0_TIMER                  0   /* Programmable Interval Timer */
#define IRQ1_KEYBOARD               1   /* PS/2 Keyboard */
#define IRQ2_CASCADE                2   /* Cascade (internal, not used) */
#define IRQ3_COM2                   3   /* COM2 serial port */
#define IRQ4_COM1                   4   /* COM1 serial port */
#define IRQ5_LPT2                   5   /* LPT2 parallel port */
#define IRQ6_FLOPPY                 6   /* Floppy disk controller */
#define IRQ7_LPT1                   7   /* LPT1 parallel port (spurious) */
#define IRQ8_RTC                    8   /* Real-time clock */
#define IRQ9_FREE                   9   /* Free / ACPI */
#define IRQ10_FREE                  10  /* Free */
#define IRQ11_FREE                  11  /* Free */
#define IRQ12_MOUSE                 12  /* PS/2 Mouse */
#define IRQ13_FPU                   13  /* FPU / Coprocessor */
#define IRQ14_ATA_PRIMARY           14  /* Primary ATA (hard disk) */
#define IRQ15_ATA_SECONDARY         15  /* Secondary ATA */

/* Total number of IRQ lines */
#define IRQ_COUNT                   16

/* Convert IRQ number to interrupt vector */
#define IRQ_TO_VECTOR(irq)          ((irq) + IRQ_BASE)

/* Convert interrupt vector to IRQ number */
#define VECTOR_TO_IRQ(vector)       ((vector) - IRQ_BASE)

/* Syscall interrupt vector (Linux uses 0x80) */
#define SYSCALL_VECTOR              0x80

/* ---------------------------------------------------------------------------
 * Type Definitions
 * --------------------------------------------------------------------------- */

/*
 * Interrupt Stack Frame
 * ---------------------------------------------------------------------------
 * This structure represents the CPU state when an interrupt occurs.
 * It's pushed onto the stack by the CPU and our interrupt stubs.
 *
 * Stack layout (from bottom to top, i.e., higher addresses first):
 *   - Registers pushed by CPU automatically (SS, ESP for ring change, EFLAGS, CS, EIP)
 *   - Error code (if applicable, or dummy 0)
 *   - Interrupt number (pushed by stub)
 *   - General-purpose registers (pushed by common handler)
 *   - Segment registers (pushed by common handler)
 */
typedef struct interrupt_frame {
    /* Segment registers (pushed by our handler) */
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /* General-purpose registers (pushed by pusha) */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;     /* ESP from pusha (not useful) */
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /* Pushed by our interrupt stub */
    uint32_t int_no;        /* Interrupt number */
    uint32_t err_code;      /* Error code (or 0) */

    /* Pushed automatically by CPU */
    uint32_t eip;           /* Instruction pointer */
    uint32_t cs;            /* Code segment */
    uint32_t eflags;        /* Flags register */
    uint32_t esp;           /* Stack pointer (only if privilege change) */
    uint32_t ss;            /* Stack segment (only if privilege change) */
} __attribute__((packed)) interrupt_frame_t;

/*
 * IRQ Handler Function Pointer
 * ---------------------------------------------------------------------------
 * Callback function type for IRQ handlers.
 * The frame parameter provides access to the CPU state at interrupt time.
 */
typedef void (*irq_handler_t)(interrupt_frame_t *frame);

/*
 * ISR Handler Function Pointer
 * ---------------------------------------------------------------------------
 * Callback function type for CPU exception handlers.
 */
typedef void (*isr_handler_t)(interrupt_frame_t *frame);

/* ---------------------------------------------------------------------------
 * IDT Functions (idt.c)
 * --------------------------------------------------------------------------- */

/*
 * idt_init - Initialize the Interrupt Descriptor Table
 * ---------------------------------------------------------------------------
 * Sets up all 256 IDT entries and loads the IDTR register.
 * Must be called before enabling interrupts.
 */
void idt_init(void);

/*
 * idt_set_gate - Set an IDT entry
 * ---------------------------------------------------------------------------
 * Parameters:
 *   vector  - Interrupt vector number (0-255)
 *   handler - Address of the interrupt handler
 *   selector - Code segment selector (usually 0x08)
 *   flags   - IDT entry flags (type and attributes)
 */
void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector, uint8_t flags);

/* ---------------------------------------------------------------------------
 * ISR Functions (isr.c)
 * --------------------------------------------------------------------------- */

/*
 * isr_init - Initialize CPU exception handlers
 * ---------------------------------------------------------------------------
 * Sets up handlers for all 32 CPU exceptions (ISR 0-31).
 * Installs default exception handlers that display error information.
 */
void isr_init(void);

/*
 * isr_register_handler - Register a custom exception handler
 * ---------------------------------------------------------------------------
 * Parameters:
 *   isr_num - Exception number (0-31)
 *   handler - Function to call when exception occurs
 */
void isr_register_handler(uint8_t isr_num, isr_handler_t handler);

/*
 * isr_handler - Common ISR handler called from assembly
 * ---------------------------------------------------------------------------
 * This is called by the assembly stubs after saving CPU state.
 * It dispatches to the appropriate exception handler.
 */
void isr_handler(interrupt_frame_t *frame);

/* ---------------------------------------------------------------------------
 * IRQ Functions (irq.c)
 * --------------------------------------------------------------------------- */

/*
 * irq_init - Initialize hardware interrupt handling
 * ---------------------------------------------------------------------------
 * Sets up the PIC and installs IRQ handlers.
 * Must be called after idt_init().
 */
void irq_init(void);

/*
 * irq_register_handler - Register an IRQ handler
 * ---------------------------------------------------------------------------
 * Parameters:
 *   irq     - IRQ number (0-15)
 *   handler - Function to call when IRQ occurs
 *
 * Returns:
 *   0 on success, -1 on error (invalid IRQ or handler already registered)
 */
int irq_register_handler(uint8_t irq, irq_handler_t handler);

/*
 * irq_unregister_handler - Remove an IRQ handler
 * ---------------------------------------------------------------------------
 * Parameters:
 *   irq - IRQ number (0-15)
 */
void irq_unregister_handler(uint8_t irq);

/*
 * irq_handler - Common IRQ handler called from assembly
 * ---------------------------------------------------------------------------
 * This is called by the assembly stubs after saving CPU state.
 * It dispatches to the registered handler and sends EOI to PIC.
 */
void irq_handler(interrupt_frame_t *frame);

/*
 * irq_enable - Enable a specific IRQ line
 * ---------------------------------------------------------------------------
 * Clears the mask bit for the specified IRQ in the PIC.
 */
void irq_enable(uint8_t irq);

/*
 * irq_disable - Disable a specific IRQ line
 * ---------------------------------------------------------------------------
 * Sets the mask bit for the specified IRQ in the PIC.
 */
void irq_disable(uint8_t irq);

/*
 * irq_is_spurious - Check if an IRQ is spurious
 * ---------------------------------------------------------------------------
 * Parameters:
 *   irq - IRQ number to check
 *
 * Returns:
 *   true if the IRQ is spurious, false otherwise
 */
bool irq_is_spurious(uint8_t irq);

/* ---------------------------------------------------------------------------
 * PIC Functions (irq.c)
 * --------------------------------------------------------------------------- */

/*
 * pic_init - Initialize and remap the 8259 PIC
 * ---------------------------------------------------------------------------
 * Remaps IRQ 0-7 to vectors 32-39 and IRQ 8-15 to vectors 40-47.
 * This avoids conflicts with CPU exception vectors (0-31).
 */
void pic_init(void);

/*
 * pic_send_eoi - Send End-Of-Interrupt to PIC
 * ---------------------------------------------------------------------------
 * Parameters:
 *   irq - IRQ number that has been serviced
 */
void pic_send_eoi(uint8_t irq);

/*
 * pic_disable - Disable the PIC entirely
 * ---------------------------------------------------------------------------
 * Masks all IRQs. Useful when switching to APIC.
 */
void pic_disable(void);

/* ---------------------------------------------------------------------------
 * Utility Functions
 * --------------------------------------------------------------------------- */

/*
 * interrupts_enable - Enable interrupts globally
 * ---------------------------------------------------------------------------
 * Sets the IF flag in EFLAGS via STI instruction.
 */
static inline void interrupts_enable(void)
{
    __asm__ volatile("sti");
}

/*
 * interrupts_disable - Disable interrupts globally
 * ---------------------------------------------------------------------------
 * Clears the IF flag in EFLAGS via CLI instruction.
 */
static inline void interrupts_disable(void)
{
    __asm__ volatile("cli");
}

/*
 * interrupts_enabled - Check if interrupts are enabled
 * ---------------------------------------------------------------------------
 * Returns:
 *   true if IF flag is set, false otherwise
 */
static inline bool interrupts_enabled(void)
{
    uint32_t eflags;
    __asm__ volatile("pushfl; popl %0" : "=r"(eflags));
    return (eflags & 0x200) != 0;  /* IF flag is bit 9 */
}

/*
 * interrupts_save_and_disable - Save interrupt state and disable
 * ---------------------------------------------------------------------------
 * Returns:
 *   The EFLAGS value before disabling (can be used with interrupts_restore)
 */
static inline uint32_t interrupts_save_and_disable(void)
{
    uint32_t eflags;
    __asm__ volatile(
        "pushfl\n"
        "popl %0\n"
        "cli"
        : "=r"(eflags)
    );
    return eflags;
}

/*
 * interrupts_restore - Restore interrupt state
 * ---------------------------------------------------------------------------
 * Parameters:
 *   eflags - EFLAGS value from interrupts_save_and_disable()
 */
static inline void interrupts_restore(uint32_t eflags)
{
    __asm__ volatile(
        "pushl %0\n"
        "popfl"
        :: "r"(eflags)
        : "memory"
    );
}

#endif /* INTERRUPTS_H */
