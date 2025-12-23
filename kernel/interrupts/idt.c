/*
 * ===========================================================================
 * kernel/interrupts/idt.c
 * ===========================================================================
 *
 * Interrupt Descriptor Table (IDT) Manager
 *
 * The IDT is a data structure used by the x86 architecture to implement an
 * interrupt vector table. It tells the CPU where to find the handler code
 * for each interrupt or exception.
 *
 * This module:
 * 1. Creates and initializes the IDT with 256 entries
 * 2. Installs ISR stubs (0-31) for CPU exceptions
 * 3. Installs IRQ stubs (32-47) for hardware interrupts
 * 4. Provides an API to set/modify IDT entries
 * 5. Loads the IDTR register to activate the IDT
 *
 * IDT Entry (Gate Descriptor) Format (8 bytes):
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │ Bits 0-15   │ Offset (Low)     │ Low 16 bits of handler address        │
 * │ Bits 16-31  │ Selector         │ Code segment selector (usually 0x08)  │
 * │ Bits 32-39  │ Reserved         │ Must be 0                             │
 * │ Bits 40-47  │ Type & Attributes│ Gate type, DPL, Present bit           │
 * │ Bits 48-63  │ Offset (High)    │ High 16 bits of handler address       │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * Type & Attributes byte:
 *   Bit 7:    Present (1 = valid entry)
 *   Bits 5-6: DPL (Descriptor Privilege Level, 0 = kernel only)
 *   Bit 4:    Storage Segment (0 for interrupt gates)
 *   Bits 0-3: Gate Type (0xE = 32-bit interrupt gate, 0xF = 32-bit trap gate)
 *
 * ===========================================================================
 */

#include "interrupts.h"

/* ---------------------------------------------------------------------------
 * IDT Entry Structure
 * --------------------------------------------------------------------------- */
typedef struct idt_entry {
    uint16_t offset_low;    /* Lower 16 bits of handler address */
    uint16_t selector;      /* Kernel code segment selector */
    uint8_t  zero;          /* Reserved, must be 0 */
    uint8_t  type_attr;     /* Type and attributes */
    uint16_t offset_high;   /* Upper 16 bits of handler address */
} __attribute__((packed)) idt_entry_t;

/* ---------------------------------------------------------------------------
 * IDT Pointer Structure (for LIDT instruction)
 * --------------------------------------------------------------------------- */
typedef struct idt_ptr {
    uint16_t limit;         /* Size of IDT - 1 */
    uint32_t base;          /* Base address of IDT */
} __attribute__((packed)) idt_ptr_t;

/* ---------------------------------------------------------------------------
 * IDT Gate Type Constants
 * --------------------------------------------------------------------------- */
#define IDT_GATE_INTERRUPT_32   0x8E    /* Present, DPL=0, 32-bit interrupt gate */
#define IDT_GATE_TRAP_32        0x8F    /* Present, DPL=0, 32-bit trap gate */
#define IDT_GATE_INTERRUPT_USER 0xEE    /* Present, DPL=3, 32-bit interrupt gate */

/* Kernel code segment selector */
#define KERNEL_CS               0x08

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* The IDT itself - 256 entries */
static idt_entry_t idt[IDT_ENTRIES] __attribute__((aligned(8)));

/* IDT pointer for LIDT instruction */
static idt_ptr_t idt_pointer;

/* ---------------------------------------------------------------------------
 * External References
 * --------------------------------------------------------------------------- */

/* Assembly function to load IDT */
extern void idt_load(void *idt_ptr);

/* ISR stub table from isr_stubs.asm */
extern uint32_t isr_stub_table[32];

/* IRQ stub table from isr_stubs.asm */
extern uint32_t irq_stub_table[16];

/* ---------------------------------------------------------------------------
 * idt_set_gate - Set an IDT entry
 * ---------------------------------------------------------------------------
 * Parameters:
 *   vector   - Interrupt vector number (0-255)
 *   handler  - Address of the interrupt handler function
 *   selector - Code segment selector (typically 0x08 for kernel)
 *   flags    - Type and attribute flags
 * --------------------------------------------------------------------------- */
void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector, uint8_t flags)
{
    idt_entry_t *entry = &idt[vector];
    
    entry->offset_low  = (uint16_t)(handler & 0xFFFF);
    entry->offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
    entry->selector    = selector;
    entry->zero        = 0;
    entry->type_attr   = flags;
}

/* ---------------------------------------------------------------------------
 * idt_init - Initialize the Interrupt Descriptor Table
 * ---------------------------------------------------------------------------
 * This function:
 * 1. Clears all IDT entries
 * 2. Installs ISR stubs for CPU exceptions (vectors 0-31)
 * 3. Installs IRQ stubs for hardware interrupts (vectors 32-47)
 * 4. Loads the IDT into the CPU
 *
 * After this function returns, the CPU will use our handlers for
 * interrupts and exceptions. However, interrupts are still disabled
 * until sti() is called.
 * --------------------------------------------------------------------------- */
void idt_init(void)
{
    /* -----------------------------------------------------------------------
     * Step 1: Clear all IDT entries
     * ----------------------------------------------------------------------- */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].zero        = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_high = 0;
    }

    /* -----------------------------------------------------------------------
     * Step 2: Install ISR stubs for CPU exceptions (0-31)
     * -----------------------------------------------------------------------
     * These handlers deal with CPU-generated exceptions like divide-by-zero,
     * page faults, general protection faults, etc.
     * ----------------------------------------------------------------------- */
    for (int i = 0; i < 32; i++) {
        idt_set_gate(
            (uint8_t)i,                 /* Vector number */
            isr_stub_table[i],          /* Handler address from assembly */
            KERNEL_CS,                  /* Kernel code segment */
            IDT_GATE_INTERRUPT_32       /* 32-bit interrupt gate, DPL=0 */
        );
    }

    /* -----------------------------------------------------------------------
     * Step 3: Install IRQ stubs for hardware interrupts (32-47)
     * -----------------------------------------------------------------------
     * After PIC remapping, hardware IRQs use vectors 32-47.
     * IRQ 0-7 -> vectors 32-39 (Master PIC)
     * IRQ 8-15 -> vectors 40-47 (Slave PIC)
     * ----------------------------------------------------------------------- */
    for (int i = 0; i < 16; i++) {
        idt_set_gate(
            (uint8_t)(IRQ_BASE + i),    /* Vector number (32 + i) */
            irq_stub_table[i],          /* Handler address from assembly */
            KERNEL_CS,                  /* Kernel code segment */
            IDT_GATE_INTERRUPT_32       /* 32-bit interrupt gate, DPL=0 */
        );
    }

    /* -----------------------------------------------------------------------
     * Step 4: Set up the IDT pointer and load the IDT
     * -----------------------------------------------------------------------
     * The LIDT instruction expects a pointer to a structure containing
     * the size (limit) and base address of the IDT.
     * ----------------------------------------------------------------------- */
    idt_pointer.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_pointer.base  = (uint32_t)&idt;

    /* Load the IDT into the CPU */
    idt_load(&idt_pointer);
}
