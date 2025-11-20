; startup.asm
;
; Early startup assembly: sets up stack, performs any last-minute initial
; checks, and then calls into `kernel_main()` defined in C.
;
; Avoid heavy logic here; move complex work to C after the kernel heap is
; initialized.
