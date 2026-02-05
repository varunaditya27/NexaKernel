/*
 * ===========================================================================
 * kernel/interrupts/irq.c
 * ===========================================================================
 *
 * Hardware Interrupt Request (IRQ) Handler and PIC Driver
 *
 * This module manages hardware-generated interrupts from devices like the
 * timer, keyboard, disk controllers, etc. It includes:
 *
 * 1. PIC (Programmable Interrupt Controller) initialization and control
 * 2. IRQ handler registration and dispatch
 * 3. Spurious interrupt detection
 *
 * 8259 PIC Architecture:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │  The PC has two cascaded 8259 PICs providing 15 usable IRQ lines:       │
 * │                                                                         │
 * │  Master PIC (IRQ 0-7)          Slave PIC (IRQ 8-15)                     │
 * │  ┌─────────────────┐           ┌─────────────────┐                      │
 * │  │ IRQ0 - Timer    │           │ IRQ8  - RTC     │                      │
 * │  │ IRQ1 - Keyboard │           │ IRQ9  - ACPI    │                      │
 * │  │ IRQ2 - CASCADE ─┼──────────>│ IRQ10 - Free    │                      │
 * │  │ IRQ3 - COM2     │           │ IRQ11 - Free    │                      │
 * │  │ IRQ4 - COM1     │           │ IRQ12 - Mouse   │                      │
 * │  │ IRQ5 - LPT2     │           │ IRQ13 - FPU     │                      │
 * │  │ IRQ6 - Floppy   │           │ IRQ14 - ATA Pri │                      │
 * │  │ IRQ7 - LPT1     │           │ IRQ15 - ATA Sec │                      │
 * │  └─────────────────┘           └─────────────────┘                      │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * PIC Remapping:
 *   By default, the BIOS maps IRQs 0-7 to vectors 0x08-0x0F, which conflicts
 *   with CPU exceptions. We remap them to vectors 0x20-0x2F (32-47).
 *
 * ===========================================================================
 */

#include "interrupts.h"

/* ---------------------------------------------------------------------------
 * External Functions (from startup.asm)
 * --------------------------------------------------------------------------- */
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern void io_wait(void);

/* ---------------------------------------------------------------------------
 * PIC I/O Ports
 * --------------------------------------------------------------------------- */
#define PIC1_COMMAND    0x20    /* Master PIC command port */
#define PIC1_DATA       0x21    /* Master PIC data port */
#define PIC2_COMMAND    0xA0    /* Slave PIC command port */
#define PIC2_DATA       0xA1    /* Slave PIC data port */

/* ---------------------------------------------------------------------------
 * PIC Commands
 * --------------------------------------------------------------------------- */
#define PIC_EOI         0x20    /* End-Of-Interrupt command */
#define PIC_READ_IRR    0x0A    /* Read Interrupt Request Register */
#define PIC_READ_ISR    0x0B    /* Read In-Service Register */

/* ---------------------------------------------------------------------------
 * ICW (Initialization Command Words) for PIC
 * --------------------------------------------------------------------------- */
#define ICW1_INIT       0x10    /* Initialization (required) */
#define ICW1_ICW4       0x01    /* ICW4 will be sent */
#define ICW1_SINGLE     0x02    /* Single mode (not cascaded) */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 */
#define ICW1_LEVEL      0x08    /* Level triggered mode */

#define ICW4_8086       0x01    /* 8086/8088 mode */
#define ICW4_AUTO_EOI   0x02    /* Auto EOI mode */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode (slave) */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode (master) */
#define ICW4_SFNM       0x10    /* Special fully nested mode */

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Array of IRQ handler function pointers */
static irq_handler_t irq_handlers[IRQ_COUNT] = { NULL };

/* IRQ mask (1 = disabled, 0 = enabled) */
static uint16_t irq_mask = 0xFFFF;  /* All IRQs disabled initially */

/* Statistics for debugging */
static uint32_t irq_counts[IRQ_COUNT] = { 0 };
static uint32_t spurious_count = 0;

/* ---------------------------------------------------------------------------
 * pic_init - Initialize and remap the 8259 PIC
 * ---------------------------------------------------------------------------
 * This function initializes both PICs and remaps their interrupt vectors:
 *   - Master PIC: IRQ 0-7  -> Vectors 32-39 (0x20-0x27)
 *   - Slave PIC:  IRQ 8-15 -> Vectors 40-47 (0x28-0x2F)
 *
 * The initialization sequence sends 4 ICWs (Initialization Command Words):
 *   ICW1: Begin initialization, specify features
 *   ICW2: Specify vector offset
 *   ICW3: Specify cascade configuration
 *   ICW4: Specify additional features
 * --------------------------------------------------------------------------- */
void pic_init(void)
{
    /* Save current masks (in case we need to restore them) */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    UNUSED(mask1);
    UNUSED(mask2);

    /* -----------------------------------------------------------------------
     * ICW1: Start initialization sequence (cascade mode, ICW4 needed)
     * ----------------------------------------------------------------------- */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* -----------------------------------------------------------------------
     * ICW2: Set vector offsets
     * Master PIC: vectors 32-39 (0x20)
     * Slave PIC:  vectors 40-47 (0x28)
     * ----------------------------------------------------------------------- */
    outb(PIC1_DATA, PIC1_BASE);     /* Master PIC offset (32) */
    io_wait();
    outb(PIC2_DATA, PIC2_BASE);     /* Slave PIC offset (40) */
    io_wait();

    /* -----------------------------------------------------------------------
     * ICW3: Configure cascading
     * Master: Slave PIC at IRQ2 (bit 2 set = 0x04)
     * Slave:  Connected to master's IRQ2 (cascade identity = 2)
     * ----------------------------------------------------------------------- */
    outb(PIC1_DATA, 0x04);          /* IRQ2 has slave */
    io_wait();
    outb(PIC2_DATA, 0x02);          /* Slave's cascade identity */
    io_wait();

    /* -----------------------------------------------------------------------
     * ICW4: Set mode (8086 mode, normal EOI)
     * ----------------------------------------------------------------------- */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* -----------------------------------------------------------------------
     * Mask all IRQs initially (will be enabled as drivers register)
     * A '1' bit means the IRQ is masked (disabled)
     * ----------------------------------------------------------------------- */
    outb(PIC1_DATA, 0xFF);          /* Mask all master IRQs */
    outb(PIC2_DATA, 0xFF);          /* Mask all slave IRQs */

    irq_mask = 0xFFFF;              /* Track mask state */
}

/* ---------------------------------------------------------------------------
 * pic_send_eoi - Send End-Of-Interrupt to PIC
 * ---------------------------------------------------------------------------
 * After handling an IRQ, we must acknowledge it by sending EOI.
 * If the IRQ came from the slave PIC (IRQ 8-15), we must send EOI
 * to both the slave and master PICs.
 *
 * Parameters:
 *   irq - The IRQ number that was serviced (0-15)
 * --------------------------------------------------------------------------- */
void pic_send_eoi(uint8_t irq)
{
    /* If IRQ came from slave PIC (IRQ 8-15), send EOI to slave first */
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    
    /* Always send EOI to master PIC */
    outb(PIC1_COMMAND, PIC_EOI);
}

/* ---------------------------------------------------------------------------
 * pic_get_isr - Get the In-Service Register (ISR) value
 * ---------------------------------------------------------------------------
 * The ISR shows which IRQs are currently being serviced.
 *
 * Returns:
 *   Combined ISR value (bits 0-7 = master, bits 8-15 = slave)
 * --------------------------------------------------------------------------- */
static uint16_t pic_get_isr(void)
{
    outb(PIC1_COMMAND, PIC_READ_ISR);
    outb(PIC2_COMMAND, PIC_READ_ISR);
    return ((uint16_t)inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

/* ---------------------------------------------------------------------------
 * pic_get_irr - Get the Interrupt Request Register (IRR) value
 * ---------------------------------------------------------------------------
 * The IRR shows which IRQs are pending (waiting to be serviced).
 *
 * Returns:
 *   Combined IRR value (bits 0-7 = master, bits 8-15 = slave)
 * --------------------------------------------------------------------------- */
static uint16_t pic_get_irr(void)
{
    outb(PIC1_COMMAND, PIC_READ_IRR);
    outb(PIC2_COMMAND, PIC_READ_IRR);
    return ((uint16_t)inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

/* ---------------------------------------------------------------------------
 * pic_disable - Disable the PIC entirely
 * ---------------------------------------------------------------------------
 * Masks all IRQs on both PICs. Useful when switching to APIC.
 * --------------------------------------------------------------------------- */
void pic_disable(void)
{
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    irq_mask = 0xFFFF;
}

/* ---------------------------------------------------------------------------
 * irq_enable - Enable a specific IRQ line
 * ---------------------------------------------------------------------------
 * Clears the mask bit for the specified IRQ, allowing it to fire.
 *
 * Parameters:
 *   irq - IRQ number (0-15)
 * --------------------------------------------------------------------------- */
void irq_enable(uint8_t irq)
{
    if (irq >= IRQ_COUNT) {
        return;
    }

    irq_mask &= ~(1 << irq);

    if (irq < 8) {
        /* IRQ on master PIC */
        outb(PIC1_DATA, (uint8_t)(irq_mask & 0xFF));
    } else {
        /* IRQ on slave PIC - also need to enable cascade (IRQ2) */
        outb(PIC2_DATA, (uint8_t)((irq_mask >> 8) & 0xFF));
        /* Ensure cascade IRQ (IRQ2) is enabled */
        irq_mask &= ~(1 << 2);
        outb(PIC1_DATA, (uint8_t)(irq_mask & 0xFF));
    }
}

/* ---------------------------------------------------------------------------
 * irq_disable - Disable a specific IRQ line
 * ---------------------------------------------------------------------------
 * Sets the mask bit for the specified IRQ, preventing it from firing.
 *
 * Parameters:
 *   irq - IRQ number (0-15)
 * --------------------------------------------------------------------------- */
void irq_disable(uint8_t irq)
{
    if (irq >= IRQ_COUNT) {
        return;
    }

    irq_mask |= (1 << irq);

    if (irq < 8) {
        outb(PIC1_DATA, (uint8_t)(irq_mask & 0xFF));
    } else {
        outb(PIC2_DATA, (uint8_t)((irq_mask >> 8) & 0xFF));
    }
}

/* ---------------------------------------------------------------------------
 * irq_is_spurious - Check if an IRQ is spurious
 * ---------------------------------------------------------------------------
 * Spurious IRQs can occur on IRQ7 (master) or IRQ15 (slave) due to:
 * - Electrical noise
 * - Race conditions during interrupt acknowledgment
 *
 * A spurious IRQ is detected by checking the ISR - if the IRQ bit is not
 * set in the ISR, it's spurious.
 *
 * Parameters:
 *   irq - IRQ number to check
 *
 * Returns:
 *   true if the IRQ is spurious, false otherwise
 * --------------------------------------------------------------------------- */
bool irq_is_spurious(uint8_t irq)
{
    /* Only IRQ7 and IRQ15 can be spurious */
    if (irq != 7 && irq != 15) {
        return false;
    }

    uint16_t isr = pic_get_isr();

    if (irq == 7) {
        /* Check if IRQ7 is really in service */
        return (isr & (1 << 7)) == 0;
    } else {
        /* Check if IRQ15 is really in service */
        return (isr & (1 << 15)) == 0;
    }
}

/* ---------------------------------------------------------------------------
 * irq_register_handler - Register an IRQ handler
 * ---------------------------------------------------------------------------
 * Parameters:
 *   irq     - IRQ number (0-15)
 *   handler - Function to call when IRQ occurs
 *
 * Returns:
 *   0 on success, -1 on error
 * --------------------------------------------------------------------------- */
int irq_register_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq >= IRQ_COUNT) {
        return -1;  /* Invalid IRQ number */
    }

    if (irq_handlers[irq] != NULL) {
        return -1;  /* Handler already registered */
    }

    irq_handlers[irq] = handler;
    return 0;
}

/* ---------------------------------------------------------------------------
 * irq_unregister_handler - Remove an IRQ handler
 * ---------------------------------------------------------------------------
 * Parameters:
 *   irq - IRQ number (0-15)
 * --------------------------------------------------------------------------- */
void irq_unregister_handler(uint8_t irq)
{
    if (irq < IRQ_COUNT) {
        irq_handlers[irq] = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * irq_handler - Common C handler for all hardware interrupts
 * ---------------------------------------------------------------------------
 * This function is called from the assembly stub (irq_common_stub) after
 * the CPU state has been saved.
 *
 * Processing steps:
 * 1. Convert interrupt vector to IRQ number
 * 2. Check for spurious interrupts (IRQ7/IRQ15)
 * 3. Call the registered handler (if any)
 * 4. Send EOI to acknowledge the interrupt
 *
 * Parameters:
 *   frame - Pointer to the saved CPU state on the stack
 * --------------------------------------------------------------------------- */
void irq_handler(interrupt_frame_t *frame)
{
    /* Convert interrupt vector to IRQ number */
    uint8_t irq = (uint8_t)(frame->int_no - IRQ_BASE);

    /* Sanity check */
    if (irq >= IRQ_COUNT) {
        return;
    }

    /* -----------------------------------------------------------------------
     * Handle spurious interrupts
     * -----------------------------------------------------------------------
     * Spurious IRQs can occur on IRQ7 and IRQ15. We must:
     * - NOT send EOI for spurious IRQ7
     * - Send EOI to master only for spurious IRQ15
     * ----------------------------------------------------------------------- */
    if (irq == 7 || irq == 15) {
        if (irq_is_spurious(irq)) {
            spurious_count++;
            
            /* For spurious IRQ15, send EOI to master (for cascade) */
            if (irq == 15) {
                outb(PIC1_COMMAND, PIC_EOI);
            }
            /* Don't send EOI for spurious IRQ7 */
            return;
        }
    }

    /* Update statistics */
    irq_counts[irq]++;

    /* 
     * Send End-Of-Interrupt to PIC BEFORE calling the handler.
     * This is necessary because the handler might trigger a context switch
     * (e.g., timer interrupt calling the scheduler). If we don't send EOI
     * before switching, the PIC will block further interrupts until the
     * original task is eventually scheduled back and finishes this function.
     */
    pic_send_eoi(irq);

    /* Call the registered handler */
    if (irq_handlers[irq] != NULL) {
        irq_handlers[irq](frame);
    }
}

/* ---------------------------------------------------------------------------
 * irq_get_count - Get the number of times an IRQ has fired
 * ---------------------------------------------------------------------------
 * Useful for debugging and statistics.
 *
 * Parameters:
 *   irq - IRQ number (0-15)
 *
 * Returns:
 *   Number of times the IRQ has been handled
 * --------------------------------------------------------------------------- */
uint32_t irq_get_count(uint8_t irq)
{
    if (irq >= IRQ_COUNT) {
        return 0;
    }
    return irq_counts[irq];
}

/* ---------------------------------------------------------------------------
 * irq_get_spurious_count - Get the number of spurious interrupts
 * --------------------------------------------------------------------------- */
uint32_t irq_get_spurious_count(void)
{
    return spurious_count;
}

/* ---------------------------------------------------------------------------
 * irq_init - Initialize hardware interrupt handling
 * ---------------------------------------------------------------------------
 * This function:
 * 1. Initializes and remaps the PIC
 * 2. Clears all IRQ handlers
 * 3. Masks all IRQs (drivers will enable them when they register)
 *
 * Must be called after idt_init() but before enabling interrupts.
 * --------------------------------------------------------------------------- */
void irq_init(void)
{
    /* Initialize the PIC */
    pic_init();

    /* Clear all handlers */
    for (int i = 0; i < IRQ_COUNT; i++) {
        irq_handlers[i] = NULL;
        irq_counts[i] = 0;
    }

    spurious_count = 0;
}
