/*
 * kernel/interrupts/isr.c
 *
 * Interrupt Service Routines (Software Interrupts)
 *
 * This file handles CPU exceptions (like Divide-by-Zero, Page Fault, or General
 * Protection Fault). It provides the core logic for diagnosing and reporting
 * system crashes or errors triggered by software execution.
 */

#include <stdint.h>

void isr_init(void) {}
void exception_handler(int n) {}
