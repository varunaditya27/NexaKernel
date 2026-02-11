# NexaKernel: Project Write-up Reference

This document serves as the foundational material for your formal project write-up. It is structured according to academic standards and provides an in-depth analysis of the **NexaKernel** operating system project, covering theoretical background, detailed implementation, data structures, and future directions.

---

# 1. Introduction

## 1.1 Overview
An Operating System (OS) is the fundamental software layer that acts as an intermediary between computer hardware and the user. It manages resources such as the Central Processing Unit (CPU), Random Access Memory (RAM), storage devices, and input/output peripherals. While commercial operating systems like Linux and Windows are vast ecosystems, their core functionality relies on a set of well-defined algorithms and data structures.

**NexaKernel** is a custom-built, 32-bit x86 monolithic operating system kernel developed from scratch. It is designed not merely as a functional kernel but as an educational platform to demonstrate the direct application of **Data Structures and Algorithms (DSA)** in systems programming.

## 1.2 Theoretical Context
Operating systems are typically classified by their kernel architecture:
*   **Monolithic Kernels:** (e.g., Linux, Unix) All services (drivers, filesystem, scheduler) run in the same kernel address space for maximum performance.
*   **Microkernels:** (e.g., Minix, L4) Only minimal services run in kernel mode; others run as user processes, improving stability but adding Inter-Process Communication (IPC) overhead.

NexaKernel adopts a **Monolithic Architecture** for simplicity and performance, allowing direct function calls between subsystems without the context-switching overhead of a microkernel.

## 1.3 Scope of the Project
The project encompasses the entire boot-to-shell lifecycle:
1.  **Bootstrapping:** Transitioning the CPU from 16-bit Real Mode to 32-bit Protected Mode.
2.  **Kernel Initialization:** Setting up the Global Descriptor Table (GDT), Interrupt Descriptor Table (IDT), and memory managers.
3.  **Core Services:** Implementing a preemptive scheduler, physical frame allocator, dynamic heap allocator, and virtual file system.
4.  **User Interaction:** Providing a shell environment via a PS/2 keyboard driver and VGA text output.

---

# 2. Problem Statement

Undergraduate operating systems curricula often face a pedagogical gap. Students learn **Data Structures** (heaps, trees, graphs) in one course and **OS Theory** (scheduling, paging, inodes) in another, rarely seeing how the two intersect in a bare-metal environment.

Standard approaches to teaching OS include:
*   **High-Level Simulations:** Writing a scheduler in Python/Java. (Lacks hardware reality).
*   **Modifying Existing Kernels:** Adding a syscall to Minix/xv6. (Obscures the initialization process).

These approaches fail to demonstrate the challenges of:
1.  **Bare-metal Concurrency:** Handling asynchronous hardware interrupts (timer, keyboard) while managing shared data structures.
2.  **Memory Constraints:** Implementing `malloc`/`free` without an underlying OS to manage the heap.
3.  **Hardware Abstraction:** Directly interfacing with I/O ports and memory-mapped devices.
4.  **Algorithm Selection:** Understanding *why* a Red-Black Tree might be better than a Linked List for a scheduler.

**NexaKernel** solves this by building the entire system from scratch, requiring the implementation of every core data structure (Bitmap, Linked List, Queue, Heap, Trie) to make the OS functional.

---

# 3. Objectives

The primary objectives of the NexaKernel project are:

1.  **Kernel Architecture:** To design and implement a monolithic kernel capable of booting on standard x86 hardware (or QEMU emulator) using the Multiboot standard.
2.  **Memory Management:**
    *   Implement a **Physical Frame Allocator** to track 4KB page usage.
    *   Implement a **Dynamic Heap Allocator** (`kmalloc`/`kfree`) with fragmentation handling.
3.  **Process Management:**
    *   Develop a **Preemptive Multitasking Scheduler** capable of context switching.
    *   Implement scheduling policies: **Round-Robin** (fairness) and **Priority** (real-time).
4.  **File System:**
    *   Create an **In-Memory Virtual File System (VFS)** supporting hierarchical directories and file operations.
    *   Implement efficient path indexing using **Tries**.
5.  **Inter-Process Communication (IPC):**
    *   Implement **Message Queues** and **Shared Memory** for task coordination.
6.  **DSA Integration:** To demonstrate the practical application of:
    *   **Bitmaps:** Physical memory management (Space efficiency).
    *   **Doubly Linked Lists:** Heap memory blocks (O(1) insertion/deletion).
    *   **Circular Queues:** Round-Robin scheduling (FIFO).
    *   **Binary Min-Heaps:** Priority scheduling (O(log N) access to highest priority).
    *   **Tries (Prefix Trees):** Filesystem path lookup (O(L) performance).
    *   **Hash Maps:** File descriptor tables (O(1) lookup).

---

# 4. Methodology

The development followed a layered approach, building from the hardware up. The system was written in **C** (for logic) and **NASM Assembly** (for CPU-specific instructions), compiled with a cross-compiler toolchain (`i686-elf-gcc`).

## 4.1 Phase 1: Booting and CPU Initialization
The boot process is the sequence of events from power-on to kernel execution.
*   **Multiboot Header (`boot/multiboot_header.asm`):** A magic number (`0x1BADB002`) and flags tell a Multiboot-compliant bootloader (GRUB) how to load the kernel.
*   **Global Descriptor Table (GDT) (`boot/gdt.asm`):** The x86 CPU starts in 16-bit Real Mode (1MB addressable memory). We switch to **32-bit Protected Mode** by loading a GDT that defines a **Flat Memory Model**:
    *   **Code Segment:** Base 0x0, Limit 4GB, Execute/Read.
    *   **Data Segment:** Base 0x0, Limit 4GB, Read/Write.
*   **Stack Initialization:** The bootloader does not set up a stack. We reserve 16KB in the `.bss` section and point the `ESP` register to it.

## 4.2 Phase 2: Interrupt Handling (The Nervous System)
The OS must respond to asynchronous hardware events and synchronous software exceptions.
*   **IDT Setup (`kernel/interrupts/idt.c`):** The **Interrupt Descriptor Table** is a vector table with 256 entries.
    *   **0-31 (ISR):** CPU Exceptions (e.g., #0 Divide-by-Zero, #14 Page Fault).
    *   **32-47 (IRQ):** Hardware Interrupts (remapped from the PIC).
    *   **128 (0x80):** System Calls.
*   **PIC Remapping (`kernel/interrupts/irq.c`):** The Intel 8259A Programmable Interrupt Controller defaults to vectors 8-15, conflicting with CPU exceptions. We remap the Master PIC to offset 32 and Slave to 40.
*   **Context Saving (`kernel/interrupts/isr_stubs.asm`):** Before calling a C handler, we must save the CPU state (`pusha` pushes EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI) so execution can resume seamlessly.

## 4.3 Phase 3: Memory Management (The Backbone)
The memory manager tracks physical RAM and provides dynamic allocation for the kernel.
*   **Physical Memory Manager (PMM):**
    *   **Data Structure:** **Bitmap** (`kernel/memory/dsa_structures/bitmap.c`).
    *   **Logic:** RAM is divided into 4KB page frames. Each bit in the bitmap represents one frame (0=Free, 1=Used).
    *   **Algorithm:** `bitmap_find_first_zero` scans 32 bits at a time (word-aligned) for performance. Complexity: O(N/32).
*   **Kernel Heap Allocator (Kheap):**
    *   **Data Structure:** **Doubly Linked Free List** (`kernel/memory/heap_allocator.c`).
    *   **Metadata:** Each block has a header (`heap_block_t`) containing `size`, `is_free`, `next`, `prev`, and a `magic` number for corruption detection.
    *   **Allocation Algorithm (First-Fit):** Iterate through the list; take the first free block that fits. If it's too large, **split** it into a used block and a smaller free block.
    *   **Deallocation Algorithm (Coalescing):** When `kfree` runs, it checks adjacent blocks (`block->prev` and `block->next`). If they are free, they are merged into one contiguous block. This eliminates **external fragmentation**.

## 4.4 Phase 4: Process Scheduling (The Heartbeat)
The scheduler enables multitasking by switching the CPU between tasks (context switching).
*   **Task Control Block (TCB):** A `task_t` struct holds the task's register state (`ESP`, `EBP`, `EIP`), stack pointer, process ID (PID), and status.
*   **Algorithm 1: Round-Robin (Fairness):**
    *   **Data Structure:** **Circular Queue** (`kernel/scheduler/dsa_structures/round_robin_queue.c`).
    *   **Logic:** Tasks are dequeued from the head, run for a fixed time slice (10ms), and re-queued at the tail. Complexity: O(1).
*   **Algorithm 2: Priority Scheduling (Real-Time):**
    *   **Data Structure:** **Binary Min-Heap** (`kernel/scheduler/dsa_structures/priority_queue.c`).
    *   **Logic:** The task with the highest priority (lowest numerical value) is always at the root.
    *   **Operations:** Insertion is O(log N) (bubble-up). Extraction is O(log N) (bubble-down).
*   **Context Switching:** Implemented in `kernel/scheduler/context_switch.asm`. It manually saves the current stack pointer and loads the new one. The `ret` instruction then "returns" into the new task's execution path.

## 4.5 Phase 5: File System (Data Organization)
A Virtual File System (VFS) abstracts storage.
*   **Directory Structure:** An **N-ary Tree** (`kernel/fs/directory_tree.c`) represents the hierarchy. Each directory node contains a linked list of children.
*   **Path Indexing:** A **Trie (Prefix Tree)** (`kernel/fs/dsa_structures/trie.c`) indexes file paths (e.g., `/home/user/doc.txt`).
    *   **Benefit:** Lookup time depends on path length (L), not the number of files (N). Complexity: O(L).
*   **File Descriptors:** An **Open File Table** uses a **Hash Map** (`kernel/fs/dsa_structures/hashmap.c`) to map integer FDs to file objects for O(1) access.

## 4.6 Phase 6: Inter-Process Communication (IPC)
Tasks must coordinate and share data.
*   **Message Queues (`kernel/ipc/message_queue.c`):** Asynchronous FIFO buffers. Tasks send variable-sized messages. If the queue is empty, receivers block.
*   **Shared Memory (`kernel/ipc/shared_memory.c`):** A physical memory region mapped into multiple tasks. Allows zero-copy data exchange.

---

# 5. Userland Architecture

NexaKernel provides a basic userland environment to demonstrate interaction.

## 5.1 The Shell (`userland/shell/shell.c`)
The shell acts as the command interpreter (CLI).
1.  **Input Loop:** Reads characters from the keyboard driver buffer (`stdin`).
2.  **Tokenization:** Splits the input string into arguments (e.g., `echo "hello"` -> `["echo", "hello"]`).
3.  **Command Execution:**
    *   **Built-in:** `help`, `clear`, `ls`, `cat`. Executed directly in the shell's process.
    *   **External:** Uses `fork()` and `exec()` (simulated) to spawn new tasks.

## 5.2 System Calls (`kernel/syscall.c`)
User programs execute privileged operations via the `INT 0x80` software interrupt.
*   **Mechanism:**
    *   **EAX:** Syscall Number (1=EXIT, 3=READ, 4=WRITE).
    *   **EBX, ECX, EDX:** Arguments.
*   **Dispatcher:** The kernel's `syscall_handler` verifies the syscall number and calls the corresponding C function from `syscall_table`.

---

# 6. Design Decisions & Analysis

## 6.1 Monolithic vs. Microkernel
We chose a monolithic design. While microkernels are more modular, they suffer from performance overhead due to frequent context switching and IPC message passing. For an educational OS where understanding the direct interaction between memory and scheduling is key, a monolithic design offers clarity and performance.

## 6.2 Data Structure Selection

| Component | Data Structure | Rationale | Complexity |
| :--- | :--- | :--- | :--- |
| **Physical Memory** | **Bitmap** | Extremely space-efficient (1 bit per 4KB page). Fast contiguous allocation scanning. | O(N/32) scan |
| **Kernel Heap** | **Linked List** | Supports variable-sized blocks. Easy to implement coalescing to stop fragmentation. | O(N) alloc, O(1) free |
| **Scheduler (RR)** | **Circular Queue** | Fairness (FIFO). Constant time operations are critical for the tick handler. | O(1) |
| **Scheduler (Prio)**| **Min-Heap** | Ensures highest priority task is always available immediately (Root). | O(log N) |
| **Filesystem Index**| **Trie** | Optimal for prefix-based strings (paths). Performance is independent of file count. | O(L) |

## 6.3 Challenges Faced
1.  **The "Triple Fault" Loop:** Early in development, incorrect GDT entries caused the CPU to reset immediately upon booting. This was resolved by meticulously verifying segment descriptors and offsets.
2.  **Stack Alignment:** GCC assumes the stack is 16-byte aligned. Failure to align the stack before calling C functions led to subtle crashes in optimized code.
3.  **Concurrency:** Without atomic primitives (Spinlocks), race conditions were possible. We mitigated this by disabling interrupts (`cli`) during critical kernel operations.

---

# 7. Future Improvements

1.  **Virtual Memory & Paging:** Currently, all tasks share the same address space. Implementing Paging (CR3 register) would provide memory protection and virtual addressing for user processes.
2.  **Multicore Support (SMP):** Adapting the scheduler to load-balance across multiple CPU cores using APIC.
3.  **ELF Loading:** Replacing the simulated `exec` with a true ELF binary loader to run compiled programs from disk.
4.  **Persistent Filesystem:** Implementing a FAT32 or ext2 driver to read/write to a physical hard drive instead of RAM.

---

# 8. Conclusion

NexaKernel successfully demonstrates the construction of a functional, modular operating system kernel. By integrating core Data Structures and Algorithms directly into the kernel's subsystems, the project provides a tangible connection between abstract computer science concepts and low-level systems engineering.

The system achieves its objectives of managing memory, scheduling tasks, organizing files, and facilitating IPC efficiently. The deliberate choice of data structures—such as Tries for the filesystem and Heaps for the scheduler—resulted in a system that is not only functional but also theoretically sound and optimized.

This project serves as a comprehensive educational platform, showcasing the complexity, beauty, and logic of operating system design.
