# NexaKernel - Comprehensive Manual Testing Guide

This guide walks you through **everything** you need to build, run, test, and debug NexaKernel on a Linux system. Follow these baby steps to get the kernel running in QEMU.

---

## Table of Contents

1. [Prerequisites Overview](#1-prerequisites-overview)
2. [Step-by-Step Tool Installation](#2-step-by-step-tool-installation)
3. [Verify Your Installation](#3-verify-your-installation)
4. [Understanding the Build System](#4-understanding-the-build-system)
5. [Building the Kernel](#5-building-the-kernel)
6. [Running the Kernel](#6-running-the-kernel)
7. [Expected Output & What to Look For](#7-expected-output--what-to-look-for)
8. [Debugging with GDB](#8-debugging-with-gdb)
9. [Creating a Bootable ISO](#9-creating-a-bootable-iso)
10. [Troubleshooting Common Issues](#10-troubleshooting-common-issues)
11. [Testing Checklist](#11-testing-checklist)
12. [File Reference](#12-file-reference)

---

## 1. Prerequisites Overview

NexaKernel is a 32-bit x86 operating system kernel. To build and run it, you need:

| Tool | Purpose | Required |
|------|---------|----------|
| **GCC** | C compiler | ✅ Yes |
| **gcc-multilib** | 32-bit compilation support on 64-bit systems | ✅ Yes |
| **NASM** | Assembler for x86 assembly files | ✅ Yes |
| **LD** (binutils) | Linker to combine object files | ✅ Yes |
| **Make** | Build automation | ✅ Yes |
| **QEMU** | x86 emulator to run the kernel | ✅ Yes |
| **GDB** | Debugger for stepping through code | ⭕ Optional |
| **GRUB** | Bootloader for ISO creation | ⭕ Optional |
| **xorriso** | ISO creation tool | ⭕ Optional |

---

## 2. Step-by-Step Tool Installation

### For Debian/Ubuntu (and derivatives like Linux Mint, Pop!_OS)

```bash
# Step 1: Update package lists
sudo apt update

# Step 2: Install essential build tools (GCC, Make, binutils)
sudo apt install build-essential

# Step 3: Install 32-bit GCC support (CRITICAL for x86 kernel)
sudo apt install gcc-multilib

# Step 4: Install NASM assembler
sudo apt install nasm

# Step 5: Install QEMU x86 emulator
sudo apt install qemu-system-x86

# Step 6: (Optional) Install debugger
sudo apt install gdb

# Step 7: (Optional) Install GRUB and ISO tools for bootable image
sudo apt install grub-pc-bin xorriso

# ALL IN ONE COMMAND:
sudo apt install build-essential gcc-multilib nasm qemu-system-x86 grub-pc-bin xorriso gdb
```

### For Fedora/RHEL/CentOS

```bash
# Install all required packages
sudo dnf install gcc gcc-c++ glibc-devel.i686 nasm qemu-system-x86 grub2-tools-extra xorriso gdb
```

### For Arch Linux/Manjaro

```bash
# Install all required packages
sudo pacman -S base-devel lib32-glibc nasm qemu grub xorriso gdb
```

### For openSUSE

```bash
# Install all required packages
sudo zypper install gcc gcc-32bit nasm qemu-x86 grub2 xorriso gdb
```

---

## 3. Verify Your Installation

Before building, verify all tools are installed correctly:

```bash
# Navigate to the project directory
cd /home/varunaditya/Projects/NexaKernel

# Run verification commands
echo "=== Checking GCC ==="
gcc --version

echo ""
echo "=== Checking 32-bit GCC support ==="
gcc -m32 -E -x c /dev/null -o /dev/null && echo "✅ 32-bit support OK" || echo "❌ 32-bit support MISSING"

echo ""
echo "=== Checking NASM ==="
nasm -v

echo ""
echo "=== Checking LD (linker) ==="
ld --version | head -1

echo ""
echo "=== Checking Make ==="
make --version | head -1

echo ""
echo "=== Checking QEMU ==="
qemu-system-i386 --version

echo ""
echo "=== Checking GDB (optional) ==="
gdb --version | head -1 2>/dev/null || echo "GDB not installed (optional)"

echo ""
echo "=== Checking GRUB (optional) ==="
grub-mkrescue --version 2>/dev/null || echo "GRUB not installed (optional - needed for ISO)"
```

### Expected Output

You should see version numbers for each tool. If any shows "command not found", install that package.

---

## 4. Understanding the Build System

### Directory Structure

```
NexaKernel/
├── boot/               # Assembly boot code
│   ├── multiboot_header.asm   # GRUB multiboot header
│   ├── gdt.asm                # Global Descriptor Table
│   ├── bootloader.asm         # Protected mode entry
│   └── startup.asm            # CPU utilities (I/O, halt)
├── kernel/             # C kernel code
│   ├── kernel.c               # Main entry point
│   ├── interrupts/            # IDT, ISR, IRQ handling
│   ├── memory/                # Frame allocator, heap
│   ├── drivers/               # VGA, timer, keyboard
│   └── scheduler/             # Task management
├── lib/                # Libraries
│   ├── cstd/                  # C standard library (string, memory)
│   └── dsa/                   # Data structures
├── config/             # Configuration files
│   ├── memory_layout.ld       # Linker script
│   ├── os_config.h            # Kernel configuration
│   └── build_flags.mk         # Build flags
├── build/              # Generated object files (created during build)
├── scripts/            # Helper scripts
│   ├── build.sh               # Build wrapper
│   ├── run_qemu.sh            # QEMU launcher
│   └── debug_gdb.sh           # GDB helper
└── Makefile            # Main build file
```

### Key Files Involved in Build

| File | Role |
|------|------|
| `Makefile` | Main build orchestrator |
| `config/memory_layout.ld` | Tells linker where to place code in memory |
| `boot/*.asm` | Assembly files compiled first (entry point) |
| `kernel/kernel.c` | Main C entry point (`kernel_main`) |

---

## 5. Building the Kernel

### Method 1: Using Make (Recommended)

```bash
# Step 1: Navigate to project root
cd /home/varunaditya/Projects/NexaKernel

# Step 2: Clean any previous builds (optional but recommended)
make clean

# Step 3: Build the kernel
make

# OR build with verbose output to see all commands:
make V=1
```

### Method 2: Using the Build Script

```bash
# Make the script executable (first time only)
chmod +x scripts/build.sh

# Build the kernel
./scripts/build.sh

# Clean and rebuild
./scripts/build.sh rebuild
```

### What Happens During Build

1. **Assembly Phase**: NASM compiles `.asm` files to object files (`.o`)
   - `boot/multiboot_header.asm` → Multiboot magic numbers
   - `boot/gdt.asm` → GDT segment descriptors
   - `boot/bootloader.asm` → Entry point `_start`
   - `boot/startup.asm` → CPU utilities

2. **Compilation Phase**: GCC compiles `.c` files with `-m32` (32-bit mode)
   - All kernel C code → object files

3. **Linking Phase**: LD combines all objects using `memory_layout.ld`
   - Output: `build/kernel.elf` (debug symbols)
   - Output: `kernel.bin` (flat binary)

### Expected Build Output

```
[AS] boot/multiboot_header.asm
[AS] boot/gdt.asm
[AS] boot/bootloader.asm
[AS] boot/startup.asm
[AS] kernel/interrupts/isr_stubs.asm
[AS] kernel/scheduler/context_switch.asm
[CC] kernel/kernel.c
[CC] kernel/panic.c
... (more files)
[LD] Linking build/kernel.elf
[OBJCOPY] Creating kernel.bin
========================================
NexaKernel built successfully!
Binary: kernel.bin
ELF:    build/kernel.elf
========================================
```

### Build Artifacts

After successful build, you'll have:

| File | Description |
|------|-------------|
| `build/kernel.elf` | Kernel with debug symbols (for GDB) |
| `kernel.bin` | Flat binary (for direct boot) |
| `build/*.o` | Object files (intermediate) |
| `build/*.d` | Dependency files (for incremental builds) |

---

## 6. Running the Kernel

### Method 1: Quick Run with Make

```bash
# Build and run in one command
make run
```

This launches QEMU with the kernel directly (no ISO needed).

### Method 2: Using the Run Script

```bash
# Make executable (first time)
chmod +x scripts/run_qemu.sh

# Run the kernel
./scripts/run_qemu.sh
```

### Method 3: Manual QEMU Command

```bash
# Direct kernel boot (fastest)
qemu-system-i386 -kernel build/kernel.elf -m 128M -serial stdio -no-reboot -no-shutdown

# With VGA display
qemu-system-i386 -kernel build/kernel.elf -m 128M -serial stdio -no-reboot -no-shutdown -vga std
```

### QEMU Options Explained

| Option | Purpose |
|--------|---------|
| `-kernel build/kernel.elf` | Load kernel directly (Multiboot) |
| `-m 128M` | Allocate 128MB RAM to VM |
| `-serial stdio` | Send serial output to terminal |
| `-no-reboot` | Don't reboot on crash (see error) |
| `-no-shutdown` | Keep VM open after halt |
| `-vga std` | Standard VGA display |
| `-s -S` | Enable GDB server, pause at start |

### Exiting QEMU

- Press `Ctrl+A` then `X` to quit
- Or close the QEMU window

---

## 7. Expected Output & What to Look For

When the kernel boots successfully, you should see:

### VGA Display (QEMU Window)

```
========================================
  NexaKernel v0.1.0 - Booting...
========================================

[BOOT] Kernel loaded at: 0x00100000
[BOOT] Kernel ends at:   0x00XXXXXX
[BOOT] Kernel size:      XX KB
[BOOT] Multiboot flags:  0xXXXXXXXX
[BOOT] Lower memory:     640 KB
[BOOT] Upper memory:     XXXXX KB (XX MB)

[INIT] Phase 1: Memory Management
  - Frame allocator initialized
  - Heap allocator initialized

[INIT] Phase 2: Interrupt Handling
  - IDT initialized (256 entries)
  - ISR handlers installed
  - IRQ handlers installed
  - PIC remapped

[INIT] Phase 3: Device Drivers
  - VGA text mode initialized
  - PIT timer initialized (100 Hz)
  - PS/2 keyboard initialized

[INIT] Phase 4: Scheduler
  - Task system initialized
  - Scheduler initialized

[TEST] Memory Allocation Test
  - kmalloc test: PASSED
  - kfree test: PASSED

[TEST] Interrupt System Test
  - Interrupts enabled

[TEST] Scheduler System Test
  ...
```

### Success Indicators

✅ **Kernel boots** - You see the welcome banner  
✅ **Memory detected** - Upper/lower memory values shown  
✅ **Phases complete** - All 4 init phases pass  
✅ **Tests pass** - Memory allocation works  
✅ **Interrupts work** - Timer ticks appear (if implemented)  
✅ **Keyboard works** - Keypresses detected (if implemented)  

### Failure Indicators

❌ **Black screen** - Boot failed, check assembly code  
❌ **Triple fault/reboot** - CPU exception, use GDB to debug  
❌ **Hang at phase** - Initialization stuck, check that module  
❌ **Page fault** - Memory access issue  

---

## 8. Debugging with GDB

If something goes wrong, use GDB to step through the kernel.

### Step 1: Start QEMU in Debug Mode

```bash
# Terminal 1: Start QEMU paused with GDB server
make debug
```

QEMU will start but wait for GDB to connect (notice `-s -S` flags).

### Step 2: Connect GDB

```bash
# Terminal 2: Start GDB and connect
gdb build/kernel.elf -ex "target remote localhost:1234"
```

### Step 3: Basic GDB Commands

```gdb
# Continue execution
(gdb) c

# Set breakpoint at kernel_main
(gdb) break kernel_main
(gdb) c

# Step one instruction
(gdb) si

# Step one C line
(gdb) n

# Print registers
(gdb) info registers

# Print variable
(gdb) print vga_row

# Examine memory at address
(gdb) x/10x 0xB8000

# Disassemble current function
(gdb) disas

# Quit
(gdb) quit
```

### Using the Debug Script

```bash
# Terminal 1: Start QEMU with GDB server
./scripts/debug_gdb.sh qemu

# Terminal 2: Connect GDB
./scripts/debug_gdb.sh gdb
```

### Common Debug Scenarios

**Kernel doesn't boot:**
```gdb
break _start
c
si
# Step through assembly to find where it fails
```

**Crash at specific function:**
```gdb
break init_memory
c
n
# Step through to find the failing line
```

**Page fault:**
```gdb
# Check CR2 register for faulting address
info registers cr2
```

---

## 9. Creating a Bootable ISO

To test with GRUB (like a real computer would boot):

### Build ISO

```bash
# Build kernel and create ISO
make iso
```

This creates `nexakernel.iso` in the project root.

### Run from ISO

```bash
# Boot from ISO (uses GRUB)
make run-iso

# Or manually
qemu-system-i386 -cdrom nexakernel.iso -m 128M
```

### What's in the ISO

```
nexakernel.iso
└── boot/
    ├── kernel.elf
    └── grub/
        └── grub.cfg    # GRUB configuration
```

### Testing on Real Hardware (Advanced)

```bash
# Write ISO to USB (DANGEROUS - replace /dev/sdX with your USB)
# MAKE SURE /dev/sdX IS YOUR USB, NOT YOUR HARD DRIVE!
sudo dd if=nexakernel.iso of=/dev/sdX bs=4M status=progress
sync
```

---

## 10. Troubleshooting Common Issues

### Issue: "gcc: command not found"

**Solution:**
```bash
sudo apt install build-essential
```

### Issue: "cannot find -lgcc: No such file" or 32-bit errors

**Solution:**
```bash
sudo apt install gcc-multilib
```

### Issue: "nasm: command not found"

**Solution:**
```bash
sudo apt install nasm
```

### Issue: "qemu-system-i386: command not found"

**Solution:**
```bash
sudo apt install qemu-system-x86
# On some systems:
sudo apt install qemu-system-i386
```

### Issue: Build fails with undefined reference

**Cause:** A function is declared but not implemented.

**Solution:** Check that all `.c` and `.asm` files have implementations for declared functions.

### Issue: QEMU shows black screen then exits

**Causes:**
1. Multiboot header invalid
2. Entry point `_start` not found
3. Triple fault (CPU exception)

**Debug:**
```bash
make debug
# In another terminal:
gdb build/kernel.elf -ex "target remote localhost:1234" -ex "break _start" -ex "c"
```

### Issue: Kernel hangs (no output)

**Causes:**
1. VGA driver not initialized
2. Infinite loop somewhere
3. Interrupts disabled indefinitely

**Debug:** Add breakpoints at each initialization phase.

### Issue: grub-mkrescue fails

**Solution:**
```bash
sudo apt install grub-pc-bin xorriso mtools
```

### Issue: "No multiboot header found"

**Cause:** Multiboot header not in first 8KB of binary.

**Solution:** Check `config/memory_layout.ld` places `.multiboot` section first.

---

## 11. Testing Checklist

Use this checklist to verify the kernel works:

### Build Tests

- [ ] `make clean` completes without errors
- [ ] `make` builds successfully
- [ ] `build/kernel.elf` file exists
- [ ] `kernel.bin` file exists
- [ ] No compiler warnings (or expected warnings only)

### Boot Tests

- [ ] `make run` launches QEMU
- [ ] Welcome banner appears
- [ ] Kernel address is 0x00100000
- [ ] Memory info is displayed
- [ ] All 4 init phases complete

### Memory Tests

- [ ] Frame allocator initializes
- [ ] Heap allocator initializes
- [ ] kmalloc test passes
- [ ] kfree test passes

### Interrupt Tests

- [ ] IDT initialized message appears
- [ ] ISR handlers installed
- [ ] IRQ handlers installed
- [ ] PIC remapped message appears
- [ ] Interrupts enabled without crash

### Driver Tests

- [ ] VGA text mode works (text visible)
- [ ] Timer initializes (if implemented)
- [ ] Keyboard initializes (if implemented)
- [ ] Keyboard input works (if implemented)

### Scheduler Tests

- [ ] Task system initializes
- [ ] Scheduler initializes
- [ ] Context switch works (if implemented)

### ISO Tests

- [ ] `make iso` creates ISO file
- [ ] `make run-iso` boots with GRUB menu
- [ ] Kernel loads from GRUB

### Debug Tests

- [ ] `make debug` starts QEMU in paused state
- [ ] GDB can connect
- [ ] Breakpoints work
- [ ] Can step through code

---

## 12. File Reference

### Critical Boot Files

| File | Purpose | What to Check |
|------|---------|---------------|
| `boot/multiboot_header.asm` | GRUB recognition | Magic number 0x1BADB002 |
| `boot/gdt.asm` | Memory segments | Code/Data selectors 0x08/0x10 |
| `boot/bootloader.asm` | Entry point | `_start` symbol exists |
| `boot/startup.asm` | CPU utilities | `cpu_halt`, `inb`, `outb` |

### Kernel Entry

| File | Purpose | What to Check |
|------|---------|---------------|
| `kernel/kernel.c` | Main entry | `kernel_main` function |
| `kernel/panic.c` | Error handling | `kernel_panic` function |

### Configuration

| File | Purpose | What to Check |
|------|---------|---------------|
| `config/memory_layout.ld` | Memory layout | Kernel at 0x00100000 |
| `config/os_config.h` | Kernel config | Version, options |
| `Makefile` | Build system | CFLAGS, LDFLAGS |

### Key Commands Summary

```bash
# Build
make                    # Build kernel
make clean              # Clean build
make iso                # Create ISO

# Run
make run                # Run in QEMU
make run-iso            # Run from ISO
make debug              # Debug mode

# Info
make help               # Show all targets
make info               # Show build info
```

---

## Quick Start (TL;DR)

```bash
# 1. Install tools (Ubuntu/Debian)
sudo apt install build-essential gcc-multilib nasm qemu-system-x86 gdb

# 2. Navigate to project
cd /home/varunaditya/Projects/NexaKernel

# 3. Build
make clean && make

# 4. Run
make run

# 5. (If issues) Debug
make debug
# In another terminal:
gdb build/kernel.elf -ex "target remote localhost:1234"
```

---

**Last Updated:** December 30, 2025  
**NexaKernel Version:** 0.1.0  
**Supported Platforms:** Linux (Debian, Ubuntu, Fedora, Arch)
