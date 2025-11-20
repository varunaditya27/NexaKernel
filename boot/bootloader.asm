; bootloader.asm
;
; Minimal bootloader scaffold: early real-mode code that sets up a simple
; environment and jumps to the kernel. In a working kernel this file:
; - sets up a stack and segments
; - loads the kernel binary into memory
; - switches to protected mode and jumps to a C entrypoint
;
; Keep bootloader code minimal. Avoid dynamic memory; the kernel heap is not
; initialized yet.
