; ===========================================================================
; kernel/interrupts/isr_stubs.asm
; ===========================================================================
;
; Interrupt Service Routine Assembly Stubs
;
; This file contains the low-level assembly stubs that are called when an
; interrupt occurs. Each stub:
; 1. Pushes a dummy error code (if the CPU doesn't push one)
; 2. Pushes the interrupt number
; 3. Jumps to a common handler that saves all registers
; 4. Calls the C handler (isr_handler or irq_handler)
; 5. Restores registers and returns from interrupt
;
; The stubs are organized into:
; - ISR stubs (0-31): CPU exceptions
; - IRQ stubs (32-47): Hardware interrupts (after PIC remapping)
;
; ===========================================================================

[bits 32]

section .text

; ---------------------------------------------------------------------------
; External C handlers
; ---------------------------------------------------------------------------
extern isr_handler          ; C handler for CPU exceptions
extern irq_handler          ; C handler for hardware interrupts

; ---------------------------------------------------------------------------
; Macro: ISR stub WITHOUT error code
; ---------------------------------------------------------------------------
; For exceptions that don't push an error code, we push a dummy 0.
; This keeps the stack layout consistent.
;
; Arguments:
;   %1 - Interrupt number
; ---------------------------------------------------------------------------
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0            ; Push dummy error code
    push dword %1           ; Push interrupt number
    jmp isr_common_stub     ; Jump to common handler
%endmacro

; ---------------------------------------------------------------------------
; Macro: ISR stub WITH error code
; ---------------------------------------------------------------------------
; For exceptions where the CPU pushes an error code, we don't push a dummy.
;
; Arguments:
;   %1 - Interrupt number
; ---------------------------------------------------------------------------
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; Error code already pushed by CPU
    push dword %1           ; Push interrupt number
    jmp isr_common_stub     ; Jump to common handler
%endmacro

; ---------------------------------------------------------------------------
; Macro: IRQ stub
; ---------------------------------------------------------------------------
; Hardware interrupts (IRQs) don't push error codes.
;
; Arguments:
;   %1 - IRQ number (0-15)
;   %2 - Mapped interrupt vector (32-47)
; ---------------------------------------------------------------------------
%macro IRQ 2
global irq%1
irq%1:
    push dword 0            ; Push dummy error code
    push dword %2           ; Push interrupt vector number
    jmp irq_common_stub     ; Jump to common IRQ handler
%endmacro

; ===========================================================================
; ISR Stubs (CPU Exceptions 0-31)
; ===========================================================================
; Exceptions that push error codes: 8, 10, 11, 12, 13, 14, 17, 21, 29, 30
; All others need a dummy error code pushed.
; ===========================================================================

ISR_NOERRCODE 0             ; #DE - Divide Error
ISR_NOERRCODE 1             ; #DB - Debug Exception
ISR_NOERRCODE 2             ; NMI - Non-Maskable Interrupt
ISR_NOERRCODE 3             ; #BP - Breakpoint
ISR_NOERRCODE 4             ; #OF - Overflow
ISR_NOERRCODE 5             ; #BR - BOUND Range Exceeded
ISR_NOERRCODE 6             ; #UD - Invalid Opcode
ISR_NOERRCODE 7             ; #NM - Device Not Available
ISR_ERRCODE   8             ; #DF - Double Fault (error code = 0)
ISR_NOERRCODE 9             ; Coprocessor Segment Overrun (reserved)
ISR_ERRCODE   10            ; #TS - Invalid TSS
ISR_ERRCODE   11            ; #NP - Segment Not Present
ISR_ERRCODE   12            ; #SS - Stack-Segment Fault
ISR_ERRCODE   13            ; #GP - General Protection Fault
ISR_ERRCODE   14            ; #PF - Page Fault
ISR_NOERRCODE 15            ; Reserved
ISR_NOERRCODE 16            ; #MF - x87 FPU Floating-Point Error
ISR_ERRCODE   17            ; #AC - Alignment Check
ISR_NOERRCODE 18            ; #MC - Machine Check
ISR_NOERRCODE 19            ; #XM - SIMD Floating-Point Exception
ISR_NOERRCODE 20            ; #VE - Virtualization Exception
ISR_ERRCODE   21            ; #CP - Control Protection Exception
ISR_NOERRCODE 22            ; Reserved
ISR_NOERRCODE 23            ; Reserved
ISR_NOERRCODE 24            ; Reserved
ISR_NOERRCODE 25            ; Reserved
ISR_NOERRCODE 26            ; Reserved
ISR_NOERRCODE 27            ; Reserved
ISR_NOERRCODE 28            ; Reserved
ISR_NOERRCODE 29            ; Reserved (VMX #VC - may have error code)
ISR_NOERRCODE 30            ; Reserved (SX - may have error code)
ISR_NOERRCODE 31            ; Reserved

; ===========================================================================
; IRQ Stubs (Hardware Interrupts 0-15)
; ===========================================================================
; After PIC remapping, IRQ 0-7 map to vectors 32-39
; and IRQ 8-15 map to vectors 40-47.
; ===========================================================================

IRQ 0,  32                  ; PIT (Programmable Interval Timer)
IRQ 1,  33                  ; Keyboard
IRQ 2,  34                  ; Cascade (internal, never raised)
IRQ 3,  35                  ; COM2
IRQ 4,  36                  ; COM1
IRQ 5,  37                  ; LPT2
IRQ 6,  38                  ; Floppy Disk
IRQ 7,  39                  ; LPT1 / Spurious
IRQ 8,  40                  ; CMOS RTC
IRQ 9,  41                  ; Free / ACPI
IRQ 10, 42                  ; Free
IRQ 11, 43                  ; Free
IRQ 12, 44                  ; PS/2 Mouse
IRQ 13, 45                  ; FPU / Coprocessor
IRQ 14, 46                  ; Primary ATA
IRQ 15, 47                  ; Secondary ATA / Spurious

; ===========================================================================
; isr_common_stub - Common handler for CPU exceptions
; ===========================================================================
; This stub saves the CPU state, calls the C handler, and restores state.
;
; Stack layout when entering:
;   [esp+0]  = Interrupt number
;   [esp+4]  = Error code (or dummy 0)
;   [esp+8]  = EIP (pushed by CPU)
;   [esp+12] = CS (pushed by CPU)
;   [esp+16] = EFLAGS (pushed by CPU)
;   [esp+20] = ESP (only if privilege change)
;   [esp+24] = SS (only if privilege change)
; ===========================================================================
isr_common_stub:
    ; Save general-purpose registers (PUSHA order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    pusha

    ; Save segment registers
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment into segment registers
    mov ax, 0x10            ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler with pointer to interrupt frame
    ; The frame is on the stack, so ESP points to it
    push esp                ; Pass pointer to interrupt_frame_t
    call isr_handler        ; Call C handler
    add esp, 4              ; Clean up parameter

    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore general-purpose registers
    popa

    ; Clean up error code and interrupt number from stack
    add esp, 8

    ; Return from interrupt
    iret

; ===========================================================================
; irq_common_stub - Common handler for hardware interrupts
; ===========================================================================
; Similar to isr_common_stub, but calls the IRQ handler instead.
; ===========================================================================
irq_common_stub:
    ; Save general-purpose registers
    pusha

    ; Save segment registers
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment
    mov ax, 0x10            ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler with pointer to interrupt frame
    push esp                ; Pass pointer to interrupt_frame_t
    call irq_handler        ; Call C handler
    add esp, 4              ; Clean up parameter

    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore general-purpose registers
    popa

    ; Clean up error code and interrupt number from stack
    add esp, 8

    ; Return from interrupt
    iret

; ===========================================================================
; Export ISR addresses for IDT setup
; ===========================================================================
; We create a table of ISR entry points that can be accessed from C.
; ===========================================================================

section .data

global isr_stub_table
isr_stub_table:
    dd isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    dd isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    dd isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dd isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

global irq_stub_table
irq_stub_table:
    dd irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
    dd irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
