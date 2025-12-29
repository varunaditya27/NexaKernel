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
# Tool Configuration (Auto-detect cross-compiler on macOS)
# ---------------------------------------------------------------------------
# On macOS, we need a cross-compiler for ELF output
# On Linux, native GCC with -m32 works fine
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS - use cross-compiler
    CROSS_PREFIX = x86_64-elf-
    CC      = $(CROSS_PREFIX)gcc
    LD      = $(CROSS_PREFIX)ld
    OBJCOPY = $(CROSS_PREFIX)objcopy
else
    # Linux - use native tools
    CC      = gcc
    LD      = ld
    OBJCOPY = objcopy
endif

AS      = nasm

# ---------------------------------------------------------------------------
# Directory Structure
# ---------------------------------------------------------------------------
BOOT_DIR     = boot
KERNEL_DIR   = kernel
LIB_DIR      = lib
CONFIG_DIR   = config
USERLAND_DIR = userland
BUILD_DIR    = build
ISO_DIR      = $(BUILD_DIR)/isofiles

# ---------------------------------------------------------------------------
# Include additional build configuration
# ---------------------------------------------------------------------------
-include $(CONFIG_DIR)/build_flags.mk

# ---------------------------------------------------------------------------
# GCC Built-in Include Path (for stdarg.h, etc.)
# ---------------------------------------------------------------------------
GCC_INCLUDE = $(shell $(CC) -print-file-name=include)

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
# -MMD -MP:       Generate dependency files
# ---------------------------------------------------------------------------
CFLAGS = -m32 \
         -ffreestanding \
         -fno-builtin \
         -fno-pic \
         -nostdlib \
         -nostdinc \
         -isystem $(GCC_INCLUDE) \
         -fno-stack-protector \
         -fno-exceptions \
         -fno-unwind-tables \
         -fno-asynchronous-unwind-tables \
         -Wall \
         -Wextra \
         -Wno-unused-parameter \
         -O2 \
         -g \
         -I. \
         -I$(KERNEL_DIR) \
         -I$(LIB_DIR) \
         -I$(CONFIG_DIR) \
         -MMD -MP

# Assembler flags
ASFLAGS = -f elf32 -g

# Linker flags
LDFLAGS = -m elf_i386 \
          -T $(CONFIG_DIR)/memory_layout.ld \
          -nostdlib

# ---------------------------------------------------------------------------
# Source Files - Assembly
# ---------------------------------------------------------------------------
# Boot assembly sources
ASM_SOURCES = $(BOOT_DIR)/multiboot_header.asm \
              $(BOOT_DIR)/gdt.asm \
              $(BOOT_DIR)/bootloader.asm \
              $(BOOT_DIR)/startup.asm

# Kernel interrupt stubs
ASM_SOURCES += $(KERNEL_DIR)/interrupts/isr_stubs.asm

# Scheduler context switch
ASM_SOURCES += $(KERNEL_DIR)/scheduler/context_switch.asm

# ---------------------------------------------------------------------------
# Source Files - Kernel Core
# ---------------------------------------------------------------------------
C_SOURCES = $(KERNEL_DIR)/kernel.c \
            $(KERNEL_DIR)/panic.c \
            $(KERNEL_DIR)/syscall.c

# ---------------------------------------------------------------------------
# Source Files - Memory Management
# ---------------------------------------------------------------------------
C_SOURCES += $(KERNEL_DIR)/memory/frame_allocator.c \
             $(KERNEL_DIR)/memory/heap_allocator.c

# Memory DSA structures (bitmap.c is integrated into frame_allocator.c)
# freelist.c and buddy_tree.c provide alternative allocator implementations
C_SOURCES += $(KERNEL_DIR)/memory/dsa_structures/freelist.c \
             $(KERNEL_DIR)/memory/dsa_structures/buddy_tree.c

# ---------------------------------------------------------------------------
# Source Files - Interrupt Handling
# ---------------------------------------------------------------------------
C_SOURCES += $(KERNEL_DIR)/interrupts/idt.c \
             $(KERNEL_DIR)/interrupts/isr.c \
             $(KERNEL_DIR)/interrupts/irq.c

# ---------------------------------------------------------------------------
# Source Files - Device Drivers
# ---------------------------------------------------------------------------
C_SOURCES += $(KERNEL_DIR)/drivers/vga_text.c \
             $(KERNEL_DIR)/drivers/timer.c \
             $(KERNEL_DIR)/drivers/keyboard.c

# ---------------------------------------------------------------------------
# Source Files - Scheduler
# ---------------------------------------------------------------------------
C_SOURCES += $(KERNEL_DIR)/scheduler/task.c \
             $(KERNEL_DIR)/scheduler/scheduler.c

# Scheduler DSA structures
C_SOURCES += $(KERNEL_DIR)/scheduler/dsa_structures/round_robin_queue.c \
             $(KERNEL_DIR)/scheduler/dsa_structures/priority_queue.c

# ---------------------------------------------------------------------------
# Source Files - Filesystem
# ---------------------------------------------------------------------------
C_SOURCES += $(KERNEL_DIR)/fs/vfs.c \
             $(KERNEL_DIR)/fs/ramfs.c

# Filesystem DSA structures
C_SOURCES += $(KERNEL_DIR)/fs/dsa_structures/trie.c \
             $(KERNEL_DIR)/fs/dsa_structures/directory_tree.c \
             $(KERNEL_DIR)/fs/dsa_structures/hashmap.c

# ---------------------------------------------------------------------------
# Source Files - IPC
# ---------------------------------------------------------------------------
C_SOURCES += $(KERNEL_DIR)/ipc/message_queue.c \
             $(KERNEL_DIR)/ipc/shared_memory.c

# ---------------------------------------------------------------------------
# Source Files - Utilities
# ---------------------------------------------------------------------------
C_SOURCES += $(KERNEL_DIR)/utils/logging.c \
             $(KERNEL_DIR)/utils/test_dsa.c

# ---------------------------------------------------------------------------
# Source Files - Data Structure Library
# ---------------------------------------------------------------------------
C_SOURCES += $(LIB_DIR)/dsa/bitmap.c \
             $(LIB_DIR)/dsa/list.c \
             $(LIB_DIR)/dsa/queue.c \
             $(LIB_DIR)/dsa/heap.c \
             $(LIB_DIR)/dsa/tree.c \
             $(LIB_DIR)/dsa/trie.c \
             $(LIB_DIR)/dsa/hashmap.c

# ---------------------------------------------------------------------------
# Source Files - C Standard Library
# ---------------------------------------------------------------------------
C_SOURCES += $(LIB_DIR)/cstd/string.c \
             $(LIB_DIR)/cstd/memory.c \
             $(LIB_DIR)/cstd/stdio.c

# ---------------------------------------------------------------------------
# Source Files - Userland (compiled separately for now)
# ---------------------------------------------------------------------------
USERLAND_SOURCES = $(USERLAND_DIR)/shell/main.c \
                   $(USERLAND_DIR)/lib/syscall_wrappers.c \
                   $(USERLAND_DIR)/programs/example.c

# ---------------------------------------------------------------------------
# Object Files
# ---------------------------------------------------------------------------
ASM_OBJECTS = $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS   = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS     = $(ASM_OBJECTS) $(C_OBJECTS)

# Dependency files (generated by -MMD)
DEPS = $(C_OBJECTS:.o=.d)

# ---------------------------------------------------------------------------
# Output Files
# ---------------------------------------------------------------------------
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = kernel.bin
ISO_FILE   = nexakernel.iso

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
.PHONY: all clean run debug iso dirs help info userland

# Default target: build kernel binary
all: dirs $(KERNEL_BIN)
	@echo "========================================"
	@echo "NexaKernel built successfully!"
	@echo "Binary: $(KERNEL_BIN)"
	@echo "ELF:    $(KERNEL_ELF)"
	@echo "========================================"

# Include dependency files
-include $(DEPS)

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR)/$(BOOT_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/interrupts
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/memory
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/memory/dsa_structures
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/scheduler
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/scheduler/dsa_structures
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/fs
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/fs/dsa_structures
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/ipc
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/utils
	@mkdir -p $(BUILD_DIR)/$(LIB_DIR)/cstd
	@mkdir -p $(BUILD_DIR)/$(LIB_DIR)/dsa
	@mkdir -p $(BUILD_DIR)/$(USERLAND_DIR)/shell
	@mkdir -p $(BUILD_DIR)/$(USERLAND_DIR)/lib
	@mkdir -p $(BUILD_DIR)/$(USERLAND_DIR)/programs

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
	@echo 'set timeout=3' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "NexaKernel" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.elf' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR) 2>/dev/null
	@echo "========================================"
	@echo "ISO created: $(ISO_FILE)"
	@echo "========================================"

# ---------------------------------------------------------------------------
# QEMU Targets
# ---------------------------------------------------------------------------

# Run kernel in QEMU (direct multiboot)
run: all
	@echo "[QEMU] Starting NexaKernel..."
	qemu-system-i386 -kernel $(KERNEL_ELF) \
		-m 128M \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-vga std

# Run with ISO (uses GRUB bootloader)
run-iso: iso
	@echo "[QEMU] Starting NexaKernel from ISO..."
	qemu-system-i386 -cdrom $(ISO_FILE) \
		-m 128M \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-vga std

# Debug with GDB (paused at start)
debug: all
	@echo "[QEMU] Starting NexaKernel in debug mode..."
	@echo ""
	@echo "Connect GDB with:"
	@echo "  gdb $(KERNEL_ELF) -ex 'target remote localhost:1234'"
	@echo ""
	qemu-system-i386 -kernel $(KERNEL_ELF) \
		-m 128M \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-vga std \
		-s -S

# Debug from ISO
debug-iso: iso
	@echo "[QEMU] Starting NexaKernel (ISO) in debug mode..."
	qemu-system-i386 -cdrom $(ISO_FILE) \
		-m 128M \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-vga std \
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
# Information
# ---------------------------------------------------------------------------
info:
	@echo "NexaKernel Build Information"
	@echo "============================"
	@echo ""
	@echo "Compiler:     $(CC)"
	@echo "Linker:       $(LD)"
	@echo "Assembler:    $(AS)"
	@echo ""
	@echo "ASM Sources:  $(words $(ASM_SOURCES)) files"
	@echo "C Sources:    $(words $(C_SOURCES)) files"
	@echo "Total:        $(words $(OBJECTS)) object files"
	@echo ""
	@echo "Output:"
	@echo "  ELF:    $(KERNEL_ELF)"
	@echo "  Binary: $(KERNEL_BIN)"
	@echo "  ISO:    $(ISO_FILE)"

# ---------------------------------------------------------------------------
# Help
# ---------------------------------------------------------------------------
help:
	@echo ""
	@echo "  _   _                 _  __                    _ "
	@echo " | \\ | | _____  ____ _| |/ /___ _ __ _ __   ___| |"
	@echo " |  \\| |/ _ \\ \\/ / _\` | ' // _ \\ '__| '_ \\ / _ \\ |"
	@echo " | |\\  |  __/>  < (_| | . \\  __/ |  | | | |  __/ |"
	@echo " |_| \\_|\\___/_/\\_\\__,_|_|\\_\\___|_|  |_| |_|\\___|_|"
	@echo ""
	@echo "NexaKernel Build System"
	@echo "======================="
	@echo ""
	@echo "Build Targets:"
	@echo "  all       - Build kernel binary (default)"
	@echo "  iso       - Create bootable ISO image"
	@echo "  clean     - Remove build artifacts"
	@echo ""
	@echo "Run Targets:"
	@echo "  run       - Run in QEMU (direct kernel boot)"
	@echo "  run-iso   - Run in QEMU from ISO (with GRUB)"
	@echo "  debug     - Run in QEMU with GDB server (port 1234)"
	@echo "  debug-iso - Debug from ISO"
	@echo ""
	@echo "Info Targets:"
	@echo "  info      - Show build information"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Prerequisites (Debian/Ubuntu):"
	@echo "  sudo apt install build-essential gcc-multilib nasm \\"
	@echo "                   qemu-system-x86 grub-pc-bin xorriso gdb"
	@echo ""
	@echo "Quick Start:"
	@echo "  make && make run"
	@echo ""
