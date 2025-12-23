; ===========================================================================
; multiboot_header.asm
; ===========================================================================
;
; Multiboot Specification Header (Version 1)
;
; This file defines the magic numbers and flags required by Multiboot-compliant
; bootloaders (like GRUB) to recognize and load the kernel.
;
; The Multiboot header MUST appear within the first 8KB of the kernel image
; and MUST be 32-bit (4-byte) aligned.
;
; Reference: https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
;
; ===========================================================================

; ---------------------------------------------------------------------------
; Multiboot Constants
; ---------------------------------------------------------------------------
MULTIBOOT_MAGIC         equ 0x1BADB002      ; Magic number for bootloader recognition
MULTIBOOT_ALIGN         equ 1 << 0          ; Align loaded modules on page boundaries
MULTIBOOT_MEMINFO       equ 1 << 1          ; Request memory map from bootloader
MULTIBOOT_FLAGS         equ MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; ---------------------------------------------------------------------------
; Multiboot Header Section
; ---------------------------------------------------------------------------
; This section is placed at the beginning of the kernel binary by the linker.
; The bootloader scans for this header to validate and load the kernel.
; ---------------------------------------------------------------------------

section .multiboot
align 4                                     ; Multiboot spec requires 4-byte alignment

multiboot_header:
    dd MULTIBOOT_MAGIC                      ; Magic number - identifies as Multiboot
    dd MULTIBOOT_FLAGS                      ; Flags - what we want from bootloader
    dd MULTIBOOT_CHECKSUM                   ; Checksum - must sum to zero with above

; ---------------------------------------------------------------------------
; End of Multiboot Header
; ---------------------------------------------------------------------------
; After this header, control transfers to _start (defined in startup.asm)
; with:
;   - EAX = Multiboot magic value (0x2BADB002) for verification
;   - EBX = Physical address of Multiboot information structure
; ---------------------------------------------------------------------------