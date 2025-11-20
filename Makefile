# Makefile for NexaKernel (scaffold)
# This is a minimal Makefile scaffold to compile NexaKernel with a cross-compiler
# The real project may need platform-specific flags, assemblers, and linking options.

# Tools
CC=x86_64-elf-gcc
AS=nasm
LD=x86_64-elf-ld
CFLAGS=-m32 -ffreestanding -O2 -nostdlib -fno-builtin -Ilib/cstd -Ikernel
ASFLAGS=-f elf32
LDFLAGS=-T config/memory_layout.ld

# Targets
.PHONY: all clean run debug
all: kernel.bin

kernel.bin: kernel/kernel.c
	@echo "(scaffold) Compile kernel..."
	@echo "Add specific build steps in the Makefile to compile the real kernel." 

clean:
	rm -f kernel.bin

run:
	@echo "(scaffold) Run QEMU - create a working run script at scripts/run_qemu.sh"
	@echo "make run should start qemu: see scripts/run_qemu.sh"

debug:
	@echo "(scaffold) Debug with GDB - implement scripts/debug_gdb.sh with port information"
