; ===========================================================================
; startup.asm
; ===========================================================================
;
; Kernel Entry Point Trampoline and Low-Level CPU Utilities
;
; This file provides additional assembly routines needed by the kernel:
;
; 1. CPU halt routines for panic situations
; 2. I/O port access functions (inb, outb, inw, outw)
; 3. Interrupt control (enable/disable)
; 4. GDT/IDT loading helpers
;
; These functions are called from C code throughout the kernel.
;
; ===========================================================================

[bits 32]

section .text

; ---------------------------------------------------------------------------
; CPU Control Functions
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; void cpu_halt(void)
; ---------------------------------------------------------------------------
; Halt the CPU. Used by panic handlers and shutdown routines.
; This function never returns.
; ---------------------------------------------------------------------------
global cpu_halt
cpu_halt:
    cli                                     ; Disable interrupts
.loop:
    hlt                                     ; Halt CPU (wait for interrupt)
    jmp .loop                               ; Should never reach here

; ---------------------------------------------------------------------------
; void cpu_cli(void)
; ---------------------------------------------------------------------------
; Disable interrupts (Clear Interrupt Flag).
; ---------------------------------------------------------------------------
global cpu_cli
cpu_cli:
    cli
    ret

; ---------------------------------------------------------------------------
; void cpu_sti(void)
; ---------------------------------------------------------------------------
; Enable interrupts (Set Interrupt Flag).
; ---------------------------------------------------------------------------
global cpu_sti
cpu_sti:
    sti
    ret

; ---------------------------------------------------------------------------
; uint32_t cpu_get_flags(void)
; ---------------------------------------------------------------------------
; Get current EFLAGS register value.
; ---------------------------------------------------------------------------
global cpu_get_flags
cpu_get_flags:
    pushfd                                  ; Push EFLAGS onto stack
    pop eax                                 ; Pop into return register
    ret

; ---------------------------------------------------------------------------
; I/O Port Access Functions
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; void outb(uint16_t port, uint8_t value)
; ---------------------------------------------------------------------------
; Write a byte to an I/O port.
; Arguments: port (esp+4), value (esp+8)
; ---------------------------------------------------------------------------
global outb
outb:
    mov dx, [esp + 4]                       ; Port number
    mov al, [esp + 8]                       ; Value to write
    out dx, al                              ; Write to port
    ret

; ---------------------------------------------------------------------------
; uint8_t inb(uint16_t port)
; ---------------------------------------------------------------------------
; Read a byte from an I/O port.
; Argument: port (esp+4)
; Returns: byte read in AL (zero-extended to EAX)
; ---------------------------------------------------------------------------
global inb
inb:
    mov dx, [esp + 4]                       ; Port number
    xor eax, eax                            ; Clear EAX
    in al, dx                               ; Read from port
    ret

; ---------------------------------------------------------------------------
; void outw(uint16_t port, uint16_t value)
; ---------------------------------------------------------------------------
; Write a word (16 bits) to an I/O port.
; Arguments: port (esp+4), value (esp+8)
; ---------------------------------------------------------------------------
global outw
outw:
    mov dx, [esp + 4]                       ; Port number
    mov ax, [esp + 8]                       ; Value to write
    out dx, ax                              ; Write to port
    ret

; ---------------------------------------------------------------------------
; uint16_t inw(uint16_t port)
; ---------------------------------------------------------------------------
; Read a word (16 bits) from an I/O port.
; Argument: port (esp+4)
; Returns: word read in AX (zero-extended to EAX)
; ---------------------------------------------------------------------------
global inw
inw:
    mov dx, [esp + 4]                       ; Port number
    xor eax, eax                            ; Clear EAX
    in ax, dx                               ; Read from port
    ret

; ---------------------------------------------------------------------------
; void outl(uint16_t port, uint32_t value)
; ---------------------------------------------------------------------------
; Write a double word (32 bits) to an I/O port.
; Arguments: port (esp+4), value (esp+8)
; ---------------------------------------------------------------------------
global outl
outl:
    mov dx, [esp + 4]                       ; Port number
    mov eax, [esp + 8]                      ; Value to write
    out dx, eax                             ; Write to port
    ret

; ---------------------------------------------------------------------------
; uint32_t inl(uint16_t port)
; ---------------------------------------------------------------------------
; Read a double word (32 bits) from an I/O port.
; Argument: port (esp+4)
; Returns: dword read in EAX
; ---------------------------------------------------------------------------
global inl
inl:
    mov dx, [esp + 4]                       ; Port number
    in eax, dx                              ; Read from port
    ret

; ---------------------------------------------------------------------------
; void io_wait(void)
; ---------------------------------------------------------------------------
; Wait for I/O operation to complete.
; Uses a dummy write to port 0x80 (POST code port) which takes ~1 microsecond.
; ---------------------------------------------------------------------------
global io_wait
io_wait:
    mov al, 0
    out 0x80, al                            ; Write to unused port
    ret

; ---------------------------------------------------------------------------
; Descriptor Table Loading Functions
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; void gdt_load(void* gdt_descriptor)
; ---------------------------------------------------------------------------
; Load the Global Descriptor Table.
; Argument: pointer to GDT descriptor (esp+4)
; ---------------------------------------------------------------------------
global gdt_load
gdt_load:
    mov eax, [esp + 4]                      ; Get GDT descriptor pointer
    lgdt [eax]                              ; Load GDTR
    
    ; Reload data segments
    mov ax, 0x10                            ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload CS
    jmp 0x08:.reload_cs                     ; Kernel code segment selector
.reload_cs:
    ret

; ---------------------------------------------------------------------------
; void idt_load(void* idt_descriptor)
; ---------------------------------------------------------------------------
; Load the Interrupt Descriptor Table.
; Argument: pointer to IDT descriptor (esp+4)
; ---------------------------------------------------------------------------
global idt_load
idt_load:
    mov eax, [esp + 4]                      ; Get IDT descriptor pointer
    lidt [eax]                              ; Load IDTR
    ret

; ---------------------------------------------------------------------------
; Memory Barrier Functions
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; void memory_barrier(void)
; ---------------------------------------------------------------------------
; Full memory barrier - ensures all memory operations complete before
; continuing. Used for proper synchronization in driver code.
; ---------------------------------------------------------------------------
global memory_barrier
memory_barrier:
    mfence                                  ; Memory fence (requires SSE2)
    ret

; ---------------------------------------------------------------------------
; void read_barrier(void)
; ---------------------------------------------------------------------------
; Read memory barrier - ensures all reads complete before continuing.
; ---------------------------------------------------------------------------
global read_barrier
read_barrier:
    lfence                                  ; Load fence
    ret

; ---------------------------------------------------------------------------
; void write_barrier(void)
; ---------------------------------------------------------------------------
; Write memory barrier - ensures all writes complete before continuing.
; ---------------------------------------------------------------------------
global write_barrier
write_barrier:
    sfence                                  ; Store fence
    ret
