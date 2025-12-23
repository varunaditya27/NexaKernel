# ===========================================================================
# Makefile for NexaKernel
# ===========================================================================
#
# This Makefile compiles the NexaKernel operating system from source.
# It handles assembly files (NASM), C files (native GCC), and linking.
#
# Usage:
#   make          - Build the kernel (kernel.bin)
#   make iso      - Create bootable ISO image
#   make run      - Run in QEMU
#   make debug    - Run in QEMU with GDB server
#   make clean    - Remove build artifacts
#
# Prerequisites (Linux):
#   - gcc with 32-bit support (gcc-multilib on Debian/Ubuntu)
#   - nasm
#   - ld (binutils)
#   - grub-mkrescue, xorriso (for ISO creation)
#   - qemu-system-i386
#
# Install on Debian/Ubuntu:
#   sudo apt install build-essential gcc-multilib nasm qemu-system-x86 grub-pc-bin xorriso
#
# ===========================================================================

# ---------------------------------------------------------------------------
# Tool Configuration (Native Linux GCC)
# ---------------------------------------------------------------------------
CC      = gcc
LD      = ld
AS      = nasm
OBJCOPY = objcopy

# ---------------------------------------------------------------------------
# Directory Structure
# ---------------------------------------------------------------------------
BOOT_DIR    = boot
KERNEL_DIR  = kernel
LIB_DIR     = lib
CONFIG_DIR  = config
BUILD_DIR   = build
ISO_DIR     = $(BUILD_DIR)/isofiles

# ---------------------------------------------------------------------------
# Compiler Flags
# ---------------------------------------------------------------------------
# -m32:           Generate 32-bit code
# -ffreestanding: No standard library, no startup files
# -fno-builtin:   Don't use built-in functions
# -fno-pic:       No position-independent code
# -nostdlib:      Don't link standard libraries
# -nostdinc:      Don't search standard include paths
# -Wall -Wextra:  Enable warnings
# -O2:            Optimization level 2
# ---------------------------------------------------------------------------
CFLAGS = -m32 \
         -ffreestanding \
         -fno-builtin \
         -fno-pic \
         -nostdlib \
         -nostdinc \
         -fno-stack-protector \
         -Wall \
         -Wextra \
         -Werror \
         -O2 \
         -g \
         -I$(KERNEL_DIR) \
         -I$(LIB_DIR)/cstd \
         -I$(CONFIG_DIR)

# Assembler flags
ASFLAGS = -f elf32 -g

# Linker flags
LDFLAGS = -m elf_i386 \
          -T $(CONFIG_DIR)/memory_layout.ld \
          -nostdlib

# ---------------------------------------------------------------------------
# Source Files
# ---------------------------------------------------------------------------
# Assembly sources (boot)
ASM_SOURCES = $(BOOT_DIR)/multiboot_header.asm \
              $(BOOT_DIR)/gdt.asm \
              $(BOOT_DIR)/bootloader.asm \
              $(BOOT_DIR)/startup.asm

# C sources (kernel core)
C_SOURCES = $(KERNEL_DIR)/kernel.c \
            $(KERNEL_DIR)/panic.c

# TODO: Add more source files as they are implemented
# C_SOURCES += $(KERNEL_DIR)/drivers/vga_text.c
# C_SOURCES += $(KERNEL_DIR)/interrupts/idt.c

# ---------------------------------------------------------------------------
# Object Files
# ---------------------------------------------------------------------------
ASM_OBJECTS = $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS   = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS     = $(ASM_OBJECTS) $(C_OBJECTS)

# ---------------------------------------------------------------------------
# Output Files
# ---------------------------------------------------------------------------
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = kernel.bin
ISO_FILE   = nexakernel.iso

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
.PHONY: all clean run debug iso dirs

# Default target: build kernel binary
all: dirs $(KERNEL_BIN)
	@echo "========================================"
	@echo "NexaKernel built successfully!"
	@echo "Binary: $(KERNEL_BIN)"
	@echo "========================================"

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR)/$(BOOT_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/interrupts
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/memory
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/scheduler
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/fs
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/ipc
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/utils
	@mkdir -p $(BUILD_DIR)/$(LIB_DIR)/cstd
	@mkdir -p $(BUILD_DIR)/$(LIB_DIR)/dsa

# Link kernel ELF
$(KERNEL_ELF): $(OBJECTS)
	@echo "[LD] Linking $@"
	$(LD) $(LDFLAGS) -o $@ $^

# Create flat binary from ELF
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[OBJCOPY] Creating $@"
	$(OBJCOPY) -O binary $< $@

# ---------------------------------------------------------------------------
# Compilation Rules
# ---------------------------------------------------------------------------

# Compile assembly files
$(BUILD_DIR)/%.o: %.asm
	@echo "[AS] $<"
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@echo "[CC] $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# ---------------------------------------------------------------------------
# ISO Image Creation
# ---------------------------------------------------------------------------
iso: all
	@echo "[ISO] Creating bootable ISO..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	@echo 'menuentry "NexaKernel" {' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.elf' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)
	@echo "ISO created: $(ISO_FILE)"

# ---------------------------------------------------------------------------
# QEMU Targets
# ---------------------------------------------------------------------------

# Run kernel in QEMU
run: all
	@echo "[QEMU] Starting NexaKernel..."
	qemu-system-i386 -kernel $(KERNEL_ELF) \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# Run with ISO (if GRUB is needed)
run-iso: iso
	@echo "[QEMU] Starting NexaKernel from ISO..."
	qemu-system-i386 -cdrom $(ISO_FILE) \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# Debug with GDB
debug: all
	@echo "[QEMU] Starting NexaKernel in debug mode..."
	@echo "Connect GDB with: target remote localhost:1234"
	qemu-system-i386 -kernel $(KERNEL_ELF) \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-s -S

# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------
clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(KERNEL_BIN)
	rm -f $(ISO_FILE)
	@echo "Clean complete."

# ---------------------------------------------------------------------------
# Help
# ---------------------------------------------------------------------------
help:
	@echo "NexaKernel Build System (Native Linux)"
	@echo "======================================"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build kernel binary (default)"
	@echo "  iso     - Create bootable ISO image"
	@echo "  run     - Run in QEMU (direct kernel boot)"
	@echo "  run-iso - Run in QEMU from ISO"
	@echo "  debug   - Run in QEMU with GDB server"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  sudo apt install build-essential gcc-multilib nasm qemu-system-x86 grub-pc-bin xorriso"
