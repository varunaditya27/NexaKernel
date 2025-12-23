; ===========================================================================
; gdt.asm
; ===========================================================================
;
; Global Descriptor Table (GDT) Definitions
;
; This file defines the GDT required for x86 Protected Mode. The GDT describes
; memory segments that the CPU uses for memory protection and access control.
;
; We implement a FLAT MEMORY MODEL where:
;   - Code and Data segments span the entire 4GB address space
;   - Base address = 0x00000000
;   - Limit = 0xFFFFFFFF (with 4KB granularity)
;
; Segment Selectors (offsets into GDT):
;   - 0x00: Null Descriptor (required by CPU)
;   - 0x08: Kernel Code Segment (Ring 0, Execute/Read)
;   - 0x10: Kernel Data Segment (Ring 0, Read/Write)
;
; ===========================================================================

section .data
align 8                                     ; GDT should be 8-byte aligned for performance

; ---------------------------------------------------------------------------
; GDT Structure
; ---------------------------------------------------------------------------
; Each GDT entry is 8 bytes with the following format:
;
;   Bytes 0-1: Limit (bits 0-15)
;   Bytes 2-4: Base (bits 0-23)
;   Byte 5:    Access byte
;   Byte 6:    Flags (4 bits) + Limit (bits 16-19)
;   Byte 7:    Base (bits 24-31)
;
; Access Byte Format:
;   Bit 7: Present (P) - Must be 1 for valid segment
;   Bits 5-6: Privilege Level (DPL) - 0 = Ring 0 (kernel)
;   Bit 4: Descriptor Type (S) - 1 = code/data, 0 = system
;   Bits 0-3: Type - segment type and access rights
;
; Flags Format (upper 4 bits of byte 6):
;   Bit 7: Granularity (G) - 0 = byte, 1 = 4KB pages
;   Bit 6: Size (D/B) - 0 = 16-bit, 1 = 32-bit
;   Bit 5: Long mode (L) - 0 for 32-bit protected mode
;   Bit 4: Reserved (AVL) - Available for system use
; ---------------------------------------------------------------------------

global gdt_start
global gdt_end
global gdt_descriptor
global GDT_CODE_SEG
global GDT_DATA_SEG

gdt_start:

; ---------------------------------------------------------------------------
; Null Descriptor (Offset 0x00)
; ---------------------------------------------------------------------------
; The first entry MUST be null. Any attempt to use this selector triggers a
; General Protection Fault, catching bugs like uninitialized segment registers.
; ---------------------------------------------------------------------------
gdt_null:
    dq 0x0000000000000000                   ; 8 bytes of zeros

; ---------------------------------------------------------------------------
; Kernel Code Segment Descriptor (Offset 0x08)
; ---------------------------------------------------------------------------
; Base: 0x00000000, Limit: 0xFFFFF (with 4KB granularity = 4GB)
; Access: Present=1, DPL=0, S=1, Type=1010 (Execute/Read)
; Flags: G=1, D=1, L=0, AVL=0
; ---------------------------------------------------------------------------
gdt_code:
    dw 0xFFFF                               ; Limit (bits 0-15)
    dw 0x0000                               ; Base (bits 0-15)
    db 0x00                                 ; Base (bits 16-23)
    db 10011010b                            ; Access: P=1, DPL=00, S=1, Type=1010
    db 11001111b                            ; Flags=1100, Limit(16-19)=1111
    db 0x00                                 ; Base (bits 24-31)

; ---------------------------------------------------------------------------
; Kernel Data Segment Descriptor (Offset 0x10)
; ---------------------------------------------------------------------------
; Base: 0x00000000, Limit: 0xFFFFF (with 4KB granularity = 4GB)
; Access: Present=1, DPL=0, S=1, Type=0010 (Read/Write)
; Flags: G=1, D=1, L=0, AVL=0
; ---------------------------------------------------------------------------
gdt_data:
    dw 0xFFFF                               ; Limit (bits 0-15)
    dw 0x0000                               ; Base (bits 0-15)
    db 0x00                                 ; Base (bits 16-23)
    db 10010010b                            ; Access: P=1, DPL=00, S=1, Type=0010
    db 11001111b                            ; Flags=1100, Limit(16-19)=1111
    db 0x00                                 ; Base (bits 24-31)

gdt_end:

; ---------------------------------------------------------------------------
; GDT Descriptor (GDTR)
; ---------------------------------------------------------------------------
; This structure is loaded into the GDTR register using the `lgdt` instruction.
; Format: 2-byte limit (size-1), 4-byte base address
; ---------------------------------------------------------------------------
gdt_descriptor:
    dw gdt_end - gdt_start - 1              ; GDT size minus 1
    dd gdt_start                            ; GDT base address

; ---------------------------------------------------------------------------
; Segment Selector Constants
; ---------------------------------------------------------------------------
; These are offsets into the GDT, used when loading segment registers.
; ---------------------------------------------------------------------------
GDT_CODE_SEG equ gdt_code - gdt_start       ; 0x08
GDT_DATA_SEG equ gdt_data - gdt_start       ; 0x10