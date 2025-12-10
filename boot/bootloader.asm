; bootloader.asm
;
; Core Bootloader Logic
;
; This file contains the 16-bit real mode entry point and early 32-bit protected mode
; initialization code. It is responsible for:
; 1. Setting up the initial stack and data segments.
; 2. Enabling the A20 line to access memory above 1MB.
; 3. Loading the Global Descriptor Table (GDT).
; 4. Switching the CPU to Protected Mode (32-bit).
; 5. Jumping to the C kernel entry point (`kernel_main`).
;
; This is the bridge between the BIOS/GRUB and the NexaKernel C code.
