/*
 * kernel/interrupts/idt.c
 *
 * IDT creation and helper functions. Provide `idt_init()` to set up the
 * Interrupt Descriptor Table and register handlers. Keep this isolated from
 * IRQ handler logic, which lives in `irq.c`.
 */

void idt_init(void) {}
