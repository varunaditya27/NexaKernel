# NexaKernel: The Comprehensive Engineering Guide

**Version:** 0.1.0
**Target Architecture:** x86 (32-bit Protected Mode)
**Language:** C (GNU C99) & NASM Assembly
**Build System:** GNU Make

---

# 1. Executive Summary

**NexaKernel** is a monolithic operating system kernel designed from first principles to demonstrate the intersection of **Systems Engineering** and **Data Structures & Algorithms (DSA)**. Unlike commercial kernels optimized for legacy support or extreme throughput, NexaKernel is optimized for **pedagogical clarity** and **architectural purity**.

It implements a complete OS lifecycle:
1.  **Booting** from bare metal (via Multiboot).
2.  **Managing** physical and virtual resources (Memory, CPU).
3.  **Abstacting** hardware complexity (Drivers).
4.  **Providing** user-space services (Syscalls, Shell).

The core philosophy is **"DSA-Driven Design"**: every major subsystem is backed by a specific, well-known data structure (e.g., Tries for the filesystem, Min-Heaps for scheduling), making the abstract concrete.

---

# 2. System Architecture

NexaKernel follows a **Monolithic Architecture**. All core services—scheduler, memory manager, filesystem, and drivers—reside in the same address space (`0x00100000` and above) and run in **Ring 0** (Supervisor Mode).

### 2.1 The Flat Memory Model
The CPU is placed in **32-bit Protected Mode** with a Flat Memory Model:
*   **Segment Base:** `0x00000000`
*   **Segment Limit:** `0xFFFFFFFF` (4GB)
*   **Paging:** Currently disabled (Physical Address = Linear Address).
*   **Kernel Load Address:** `0x100000` (1MB mark) to avoid BIOS/Video RAM conflicts.

### 2.2 The Boot Flow
1.  **BIOS:** Power-on, POST, loads Bootloader (GRUB).
2.  **GRUB:** Loads `kernel.elf` into memory. Transitions to Protected Mode (PE bit). Jumps to `_start`.
3.  **Bootstrap (ASM):** `boot/bootloader.asm` sets up the stack and GDT. Jumps to `kernel_main`.
4.  **Kernel Init (C):** Initializes IDT, PMM, VMM, Drivers, Scheduler.
5.  **Multitasking Start:** `scheduler_start()` enables interrupts and launches the first task (Shell).

---

# 3. Directory Structure & Codebase Navigation

A "Map of the Territory" for developers.

*   **`boot/`**: The "Spark". Assembly code for system startup.
    *   `bootloader.asm`: Entry point `_start`.
    *   `gdt.asm`: Global Descriptor Table definitions.
    *   `multiboot_header.asm`: Magic numbers for GRUB.
*   **`config/`**: Global settings.
    *   `os_config.h`: System constants (Tick rate, Heap size).
    *   `memory_layout.ld`: Linker script (defines memory sections).
*   **`kernel/`**: The "Brain".
    *   `kernel.c`: Main initialization logic.
    *   `syscall.c`: The API for userland.
    *   `panic.c`: Kernel panic handler (Blue Screen of Death).
    *   **`drivers/`**: Hardware Abstraction Layer (HAL).
        *   `vga_text.c`: Screen output.
        *   `keyboard.c`: PS/2 Input.
        *   `timer.c`: PIT (8254) driver.
    *   **`interrupts/`**: The "Nervous System".
        *   `idt.c`: Interrupt Descriptor Table setup.
        *   `isr.c`: Exception handlers (#0 - #31).
        *   `irq.c`: Hardware interrupt handlers (#32 - #47).
    *   **`memory/`**: The "Backbone".
        *   `frame_allocator.c`: Physical RAM manager (Bitmap).
        *   `heap_allocator.c`: Kernel Heap manager (Free List).
    *   **`scheduler/`**: The "Heartbeat".
        *   `scheduler.c`: Core logic (Round-Robin/Priority).
        *   `context_switch.asm`: Register saving/restoring magic.
    *   **`fs/`**: The "Filing Cabinet".
        *   `ramfs.c`: Virtual Filesystem implementation.
        *   `vfs.c`: Abstract file operations.
    *   **`ipc/`**: The "Phone Lines".
        *   `message_queue.c`: Task-to-task messaging.
        *   `shared_memory.c`: Shared RAM regions.
*   **`lib/`**: The "Toolbox".
    *   `cstd/`: Freestanding C library (memcpy, strlen, printf).
    *   `dsa/`: Generic Data Structures (List, Bitmap, Trie, Heap).
*   **`userland/`**: The "Client".
    *   `shell/`: Command Line Interface.
    *   `programs/`: User applications.

---

# 4. Detailed Subsystem Analysis

## 4.1 The Boot Subsystem
*   **Goal:** Survive the transition from 16-bit real mode to 32-bit C execution.
*   **Mechanism:**
    *   **Multiboot:** We rely on GRUB to handle the A20 line and basic mode switching.
    *   **Stack:** We reserve 16KB in `.bss` (`resb 16384`) because C functions need a stack immediately.
    *   **GDT:** We reload the GDT in `bootloader.asm` to ensure we own the memory segmentation, setting Code (0x08) and Data (0x10) selectors.

## 4.2 The Interrupt Subsystem
*   **Goal:** Handle asynchronous events without crashing.
*   **Components:**
    *   **IDT (Interrupt Descriptor Table):** A 256-entry array. Each entry is an 8-byte "Gate Descriptor".
    *   **PIC (Programmable Interrupt Controller):** The 8259A chip defaults to vectors 8-15, which conflicts with CPU exceptions (e.g., Double Fault is vector 8). We **remap** the PIC to offsets 32-47.
    *   **ISR Stubs (`isr_stubs.asm`):** Small assembly wrappers that:
        1.  Push a dummy error code (if needed).
        2.  Push the interrupt number.
        3.  Jump to `isr_common_stub`.
        4.  Save all registers (`pusha`).
        5.  Call the C handler.
        6.  Restore registers (`popa`) and return (`iret`).

## 4.3 The Memory Subsystem
*   **Goal:** Provide `kmalloc` and prevent memory leaks.
*   **Physical Memory (PMM):**
    *   Uses a **Bitmap**. 1 bit = 4KB page.
    *   Optimized to scan 32 bits (128KB RAM) in one CPU cycle.
*   **Virtual Memory (VMM/Heap):**
    *   Uses a **Free List**.
    *   **Header:** `struct heap_block { size, is_free, next, prev, magic }`.
    *   **Allocation:** Traverses list -> Finds block >= size -> Splits if too big -> Returns pointer.
    *   **Deallocation:** Sets `is_free = true` -> Checks `prev` and `next` -> Merges if free -> Eliminates fragmentation.

## 4.4 The Scheduler Subsystem
*   **Goal:** Multitasking.
*   **Algorithm:**
    *   **Round-Robin:** Default. Tasks sit in a Circular Queue. The PIT timer fires every 10ms (100Hz). The handler rotates the queue.
    *   **Priority:** Optional. Tasks sit in a Binary Min-Heap. The root (lowest number) runs first.
*   **Context Switch:**
    *   The "Magic" happens in `switch_to_task`.
    *   We manually swap the **ESP** (Stack Pointer).
    *   When the function returns, it pops values from the *new* stack, effectively "returning" into a different function in a different thread.

## 4.5 The Filesystem Subsystem
*   **Goal:** Hierarchical data storage.
*   **Storage Backend:** **RamFS** (Volatile, in-memory).
*   **Indexing:** **Trie (Prefix Tree)**.
    *   Files `/bin/ls` and `/bin/cat` share the `/bin` node.
    *   Lookup complexity is O(L) (length of path), vastly superior to O(N) linear search.
*   **File Descriptors:** A **Hash Map** maps integer FDs (0, 1, 2...) to internal inode pointers.

## 4.6 The IPC Subsystem
*   **Goal:** Task communication.
*   **Message Queues:**
    *   Implemented as **Circular Buffers** of fixed-size structs.
    *   Supports `msgq_send` (non-blocking) and `msgq_receive` (blocking).
*   **Shared Memory:**
    *   Allocates a page-aligned buffer in kernel heap.
    *   Multiple tasks hold a pointer to it.
    *   Requires external synchronization (e.g., user-mode spinlocks) to be safe.

---

# 5. Data Structures & Algorithms Inventory

NexaKernel is an applied DSA laboratory. Here is the definitive mapping:

| Data Structure | Kernel Component | Why this structure? |
| :--- | :--- | :--- |
| **Bitmap** | Physical Frame Allocator | Space efficiency. 1 bit per page (overhead ~0.003%). |
| **Linked List** | Kernel Heap (Free List) | O(1) insertion/deletion. Supports variable block sizes. |
| **Circular Queue** | Scheduler (Round Robin) | FIFO fairness. O(1) enqueue/dequeue. Fixed memory usage. |
| **Binary Min-Heap**| Scheduler (Priority) | O(log N) access to highest priority. Better than sorted list (O(N)). |
| **Trie (Prefix Tree)**| Filesystem Index | Optimal for hierarchical string keys (paths). O(Length) lookup. |
| **N-ary Tree** | Directory Structure | Natural representation of folders containing N children. |
| **Hash Map** | File Descriptor Table | O(1) lookup for `read(fd)`. Integer keys map perfectly to buckets. |
| **Circular Buffer**| Keyboard Driver | Producer-Consumer model. Decouples ISR speed from shell read speed. |

---

# 6. Build & Test Infrastructure

## 6.1 The Toolchain
*   **Compiler:** `i686-elf-gcc`. A cross-compiler is mandatory to avoid linking against the host OS (Linux/Windows) standard libraries (`glibc`).
*   **Assembler:** `nasm`. Preferred for Intel syntax readability over GAS (AT&T).
*   **Linker:** `ld` with `memory_layout.ld`.
    *   Forces the kernel to start at `0x100000`.
    *   Aligns sections (text, data, bss) to 4KB boundaries.

## 6.2 The Build Process
1.  **Compile ASM:** `nasm -f elf32 boot/bootloader.asm -o build/bootloader.o`
2.  **Compile C:** `gcc -m32 -ffreestanding -c kernel/kernel.c -o build/kernel.o`
3.  **Link:** `ld -T config/memory_layout.ld -o build/kernel.elf ...`
4.  **ISO Generation:** `grub-mkrescue` wraps the ELF in a CD-ROM image with a Multiboot header.

## 6.3 Verification
*   **QEMU:** Emulates a full x86 PC.
    *   `make run`: Boots the kernel.
    *   `make debug`: Pauses CPU at startup and opens a GDB server on port 1234.
*   **Unit Tests:** `kernel/utils/test_dsa.c` runs internal consistency checks on lists, queues, and heaps during boot.

---

# 7. Conclusion

NexaKernel demonstrates that an Operating System is not magic; it is a collection of data structures managing hardware state. By peeling away the layers of abstraction—implementing our own `malloc`, our own scheduler, and our own filesystem—we gain a mastery of systems engineering that high-level programming cannot provide.

This codebase serves as a reference implementation for:
1.  **Systems Programming:** Bare-metal C, Inline Assembly, Memory Management.
2.  **Algorithm Design:** Practical application of theoretical CS concepts.
3.  **Architecture:** Understanding the x86 boot and interrupt sequence.

---

**End of Document**
