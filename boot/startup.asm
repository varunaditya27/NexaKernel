; startup.asm
;
; Kernel Entry Point Trampoline
;
; This file contains the `_start` symbol (or equivalent) that serves as the
; entry point after the bootloader finishes. It performs final low-level
; setup (like stack alignment) before calling the C function `kernel_main`.
;
; It ensures a clean environment for the C kernel to begin execution.
