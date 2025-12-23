; ===========================================================================
; bootloader.asm
; ===========================================================================
;
; Core Bootloader Logic - Protected Mode Initialization
;
; When using GRUB/Multiboot, we start directly in 32-bit protected mode.
; This file handles the transition from the bootloader to the kernel:
;
; 1. Verify Multiboot magic number (ensure we were loaded by GRUB)
; 2. Save Multiboot information pointer for kernel use
; 3. Load the Global Descriptor Table (GDT) with our segments
; 4. Reload segment registers with new selectors
; 5. Set up the kernel stack
; 6. Jump to the C kernel entry point
;
; Note: With Multiboot, A20 line is already enabled by GRUB.
;
; ===========================================================================

[bits 32]                                   ; We start in 32-bit protected mode

section .text

; ---------------------------------------------------------------------------
; External References
; ---------------------------------------------------------------------------
extern kernel_main                          ; C kernel entry point
extern gdt_descriptor                       ; GDT descriptor from gdt.asm
extern GDT_CODE_SEG                         ; Code segment selector (0x08)
extern GDT_DATA_SEG                         ; Data segment selector (0x10)

; ---------------------------------------------------------------------------
; Global Exports
; ---------------------------------------------------------------------------
global _start                               ; Entry point for linker

; ---------------------------------------------------------------------------
; Constants
; ---------------------------------------------------------------------------
MULTIBOOT_BOOTLOADER_MAGIC equ 0x2BADB002   ; Magic value in EAX from bootloader
KERNEL_STACK_SIZE          equ 16384        ; 16KB kernel stack

; ---------------------------------------------------------------------------
; Entry Point: _start
; ---------------------------------------------------------------------------
; This is where GRUB transfers control after loading the kernel.
; State on entry:
;   - EAX = Multiboot bootloader magic (0x2BADB002)
;   - EBX = Physical address of Multiboot information structure
;   - CS  = Valid code segment (but we'll reload it)
;   - Other segment registers may be undefined
;   - Interrupts are disabled
;   - A20 gate is enabled
; ---------------------------------------------------------------------------
_start:
    ; Disable interrupts immediately (should already be disabled, but be safe)
    cli

    ; -----------------------------------------------------------------------
    ; Step 1: Verify Multiboot Magic
    ; -----------------------------------------------------------------------
    ; Check that we were loaded by a Multiboot-compliant bootloader.
    ; If not, halt the system as we cannot safely continue.
    ; -----------------------------------------------------------------------
    cmp eax, MULTIBOOT_BOOTLOADER_MAGIC
    jne .no_multiboot

    ; -----------------------------------------------------------------------
    ; Step 2: Save Multiboot Information
    ; -----------------------------------------------------------------------
    ; Store the Multiboot info pointer for later use by the kernel.
    ; This structure contains memory map, boot device info, etc.
    ; -----------------------------------------------------------------------
    mov [multiboot_info_ptr], ebx

    ; -----------------------------------------------------------------------
    ; Step 3: Load Global Descriptor Table
    ; -----------------------------------------------------------------------
    ; Load our GDT which defines the flat memory model segments.
    ; This replaces whatever GRUB set up with our own configuration.
    ; -----------------------------------------------------------------------
    lgdt [gdt_descriptor]

    ; -----------------------------------------------------------------------
    ; Step 4: Reload Segment Registers
    ; -----------------------------------------------------------------------
    ; After loading the GDT, we must reload all segment registers.
    ; For CS, we need a far jump; for others, a simple mov works.
    ; -----------------------------------------------------------------------
    
    ; Reload data segment registers with kernel data selector (0x10)
    mov ax, 0x10                            ; GDT_DATA_SEG = 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload CS with a far jump to flush the prefetch queue
    ; This also transitions to our code segment
    jmp 0x08:.reload_cs                     ; GDT_CODE_SEG = 0x08

.reload_cs:
    ; -----------------------------------------------------------------------
    ; Step 5: Set Up Kernel Stack
    ; -----------------------------------------------------------------------
    ; Point ESP to the top of our kernel stack (stack grows downward).
    ; The stack is defined in the BSS section to save binary space.
    ; -----------------------------------------------------------------------
    mov esp, kernel_stack_top

    ; -----------------------------------------------------------------------
    ; Step 6: Clear Direction Flag
    ; -----------------------------------------------------------------------
    ; Ensure string operations go forward (required by System V ABI).
    ; -----------------------------------------------------------------------
    cld

    ; -----------------------------------------------------------------------
    ; Step 7: Push Multiboot Info and Call Kernel
    ; -----------------------------------------------------------------------
    ; Pass multiboot info pointer as first argument to kernel_main.
    ; Following cdecl calling convention (argument on stack).
    ; -----------------------------------------------------------------------
    push dword [multiboot_info_ptr]         ; Argument: multiboot_info_t*
    
    ; Call the C kernel entry point
    call kernel_main

    ; -----------------------------------------------------------------------
    ; Step 8: Halt if kernel_main returns
    ; -----------------------------------------------------------------------
    ; The kernel should never return, but if it does, halt safely.
    ; -----------------------------------------------------------------------
.halt:
    cli                                     ; Disable interrupts
    hlt                                     ; Halt CPU
    jmp .halt                               ; Loop forever (safety)

; ---------------------------------------------------------------------------
; Error Handler: No Multiboot
; ---------------------------------------------------------------------------
; If we weren't loaded by a Multiboot bootloader, print error and halt.
; ---------------------------------------------------------------------------
.no_multiboot:
    ; Print error message to VGA text buffer
    mov edi, 0xB8000                        ; VGA text mode buffer
    mov esi, .error_msg
    mov ah, 0x4F                            ; White on red (error color)
.print_loop:
    lodsb                                   ; Load next character
    test al, al                             ; Check for null terminator
    jz .halt                                ; If null, stop and halt
    stosw                                   ; Store char + attribute
    jmp .print_loop

.error_msg:
    db "ERROR: Not loaded by Multiboot bootloader!", 0

; ---------------------------------------------------------------------------
; Data Section
; ---------------------------------------------------------------------------
section .data

; Multiboot information pointer (saved for kernel use)
global multiboot_info_ptr
multiboot_info_ptr:
    dd 0

; ---------------------------------------------------------------------------
; BSS Section - Uninitialized Data
; ---------------------------------------------------------------------------
section .bss
align 16                                    ; Stack should be 16-byte aligned

; Kernel stack (grows downward)
kernel_stack_bottom:
    resb KERNEL_STACK_SIZE                  ; Reserve 16KB for stack
kernel_stack_top:                           ; Stack pointer starts here
