; gdt.asm
;
; Global Descriptor Table setup for protected mode. Expose a symbol for the
; address so that C code can reference segment selectors.
; Keep GDT entries basic: null, code, data.
