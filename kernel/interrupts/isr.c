/*
 * kernel/interrupts/isr.c
 *
 * Software interrupt handlers (CPU exceptions). Provide a small table to map
 * exception numbers to diagnostic handlers which print helpful messages.
 */

#include <stdint.h>

void isr_init(void) {}
void exception_handler(int n) {}
