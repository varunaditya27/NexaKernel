; context_switch.asm
;
; Low-Level Context Switch Routine
;
; This file contains the assembly code responsible for saving the state of the
; currently running task (registers, stack pointer) and restoring the state of
; the next task to be run.
;
; It is the core mechanism that enables multitasking.
