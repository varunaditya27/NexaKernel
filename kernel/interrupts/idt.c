/*
 * kernel/interrupts/idt.c
 *
 * Interrupt Descriptor Table (IDT) Manager
 *
 * This file is responsible for creating and installing the IDT. The IDT tells
 * the CPU where to jump when an interrupt or exception occurs. This module
 * registers default handlers and provides an API for other modules to register
 * their own interrupt service routines (ISRs).
 */

void idt_init(void) {}
