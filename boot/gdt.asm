; gdt.asm
;
; Global Descriptor Table (GDT) Definitions
;
; This file defines the GDT required for x86 Protected Mode. It sets up:
; - Null Descriptor (Required by CPU)
; - Kernel Code Segment (Execute/Read)
; - Kernel Data Segment (Read/Write)
;
; The GDT is loaded by `bootloader.asm` using the `lgdt` instruction.
; These descriptors define the flat memory model used by the kernel.
