# NexaKernel: Project Write-up Reference

This document serves as the foundational material for your formal project write-up. It is structured according to academic requirements and provides a comprehensive overview of the **NexaKernel** operating system project.

---

# 1. Introduction

Operating Systems (OS) form the fundamental layer of software that manages computer hardware and software resources. While modern operating systems like Linux and Windows are complex and feature-rich, understanding their core mechanisms requires peeling back layers of abstraction.

**NexaKernel** is a custom-built, 32-bit x86 operating system kernel developed from scratch. It is designed to bridge the gap between theoretical operating system concepts and practical implementation. Unlike typical OS projects that focus solely on system calls or process management, NexaKernel integrates **advanced Data Structures and Algorithms (DSA)** directly into its kernel subsystems.

This project demonstrates the construction of a functional OS kernel, including a bootloader, memory management unit, preemptive scheduler, and a virtual file system. By implementing these components from the ground up, the project highlights the critical role of efficient data structures in low-level systems programming.

---

# 2. Problem Statement

Undergraduate operating systems courses often rely on high-level simulations or modifying existing kernels (like Minix or xv6) to teach core concepts. While valuable, these approaches can obscure the challenges of:

1.  **Bare-metal interaction:** Setting up the Global Descriptor Table (GDT), Interrupt Descriptor Table (IDT), and switching from Real Mode to Protected Mode.
2.  **Memory constraints:** Implementing a dynamic memory allocator (`malloc`/`free`) without relying on an underlying OS.
3.  **Concurrency at the hardware level:** Handling asynchronous hardware interrupts (timer, keyboard) and context switching manually.
4.  **Data Structure Application:** Seeing how textbook data structures (Heaps, Tries, Hash Maps) are actually used to optimize system performance.

The goal of this project was to address these gaps by building a kernel where every major subsystem is backed by a specific, optimal data structure implementation, proving that theoretical DSA knowledge is essential for systems engineering.

---

# 3. Objectives

The primary objectives of the NexaKernel project are:

1.  **Kernel Development:** To design and implement a monolithic kernel capable of booting on standard x86 hardware (or QEMU emulator).
2.  **Memory Management:** To implement a physical frame allocator and a dynamic heap allocator with fragmentation handling.
3.  **Process Scheduling:** To develop a preemptive multitasking scheduler that supports both Round-Robin and Priority-based scheduling policies.
4.  **File System Design:** To create an in-memory Virtual File System (VFS) that supports file creation, reading, writing, and hierarchical directory management.
5.  **DSA Integration:** To demonstrate the practical application of:
    *   **Circular Queues** (Scheduler - Round Robin)
    *   **Binary Heaps** (Scheduler - Priority)
    *   **Free Lists** (Memory Allocation)
    *   **Bitmaps** (Physical Frame Allocation)
    *   **Tries & N-ary Trees** (File System Indexing)
    *   **Hash Maps** (File Descriptor Tables)

---

# 4. Methodology

The development of NexaKernel followed a modular, layered approach. The system was built in C (for logic) and Assembly (for hardware interaction), compiled using a cross-compiler toolchain (`i686-elf-gcc`).

## 4.1 Booting and Initialization
The boot process begins with a custom bootloader (or GRUB) that loads the kernel into memory.
*   **Multiboot Compliance:** The kernel includes a Multiboot header (`boot/multiboot_header.asm`) to ensure compatibility with standard bootloaders like GRUB.
*   **GDT Setup (`boot/gdt.asm`):** The Global Descriptor Table is configured to define code and data segments, transitioning the CPU from 16-bit Real Mode to 32-bit Protected Mode. This establishes a "Flat Memory Model" (Base 0x0, Limit 4GB).
*   **Stack Initialization:** A 16KB kernel stack is reserved in the BSS section, and the Stack Pointer (ESP) is set up before calling `kernel_main`.

## 4.2 Interrupt Handling (IDT & ISRs)
The kernel must respond to asynchronous hardware events and software exceptions.
*   **IDT Setup (`kernel/interrupts/idt.c`):** The Interrupt Descriptor Table is populated with 256 entries.
    *   **0-31:** CPU Exceptions (e.g., Divide-by-Zero, Page Fault).
    *   **32-47:** Hardware IRQs (remapped from the PIC).
    *   **128 (0x80):** System Calls.
*   **PIC Remapping:** The Programmable Interrupt Controller (8259A) is remapped to offset 32 to avoid conflicts with CPU exceptions.
*   **ISR Stubs:** Assembly stubs (`kernel/interrupts/isr_stubs.asm`) save the CPU state (push `pusha`) before calling the C handler.

## 4.3 Memory Management Subsystem
The memory manager is the backbone of the OS, responsible for tracking available RAM.
*   **Physical Memory (Bitmap Allocator):**
    *   **Data Structure:** **Bitmap** (`kernel/memory/dsa_structures/bitmap.c`).
    *   **Logic:** RAM is divided into 4KB page frames. Each bit in the bitmap represents one frame (0=Free, 1=Used).
    *   **Algorithm:** Allocation uses a word-scanning approach (`bitmap_find_first_zero`) to find free frames efficiently.
*   **Heap Allocator (`kmalloc`/`kfree`):**
    *   **Data Structure:** **Doubly Linked Free List** (`kernel/memory/heap_allocator.c`).
    *   **Header:** Each block has a header containing size, status, and magic number.
    *   **Algorithm:** **First-Fit** allocation strategy. When a block is requested, the allocator traverses the list to find the first block large enough. If the block is significantly larger, it is **split** into two.
    *   **Coalescing:** When memory is freed, the allocator checks adjacent blocks (`prev` and `next`). If they are also free, they are merged (**coalesced**) into a single larger block to reduce external fragmentation.

## 4.4 Process Scheduler
The scheduler enables multitasking by rapidly switching the CPU between tasks.
*   **Task Control Block (TCB):** A `task_t` struct holds the task's state (registers `ESP`, `EBP`, `EIP`), stack pointer, and status flags.
*   **Scheduling Algorithms:**
    1.  **Round-Robin:** Uses a **Circular Queue** (`kernel/scheduler/dsa_structures/round_robin_queue.c`). Tasks are dequeued from the head, run for a time slice (handled by the PIT timer), and re-queued at the tail. This ensures fairness and prevents starvation ($O(1)$ complexity).
    2.  **Priority Scheduling:** Uses a **Binary Min-Heap** (`kernel/scheduler/dsa_structures/priority_queue.c`). Tasks with higher priority (lower numerical value) are always bubbled to the root.
        *   **Insertion:** Add at end, `heapify_up` (swap with parent until heap property satisfied). $O(\log N)$.
        *   **Extraction:** Remove root, move last element to root, `heapify_down` (swap with smallest child). $O(\log N)$.
*   **Context Switching:** Implemented in Assembly (`context_switch.asm`), this routine manually saves the current task's registers to its stack and loads the new task's stack pointer.

## 4.5 Virtual File System (RAMFS)
A non-persistent, in-memory file system was implemented to demonstrate file management.
*   **Directory Structure:** An **N-ary Tree** (`kernel/fs/directory_tree.c`) represents the hierarchy. Each directory node points to a list of child nodes (subdirectories or files).
*   **Path Indexing:** A **Trie (Prefix Tree)** (`kernel/fs/dsa_structures/trie.c`) is used to index file paths. This allows for $O(L)$ lookup time (where $L$ is path length), which is significantly faster than linear search for deep directory structures.
*   **File Descriptors:** An **Open File Table** uses a **Hash Map** (`kernel/fs/dsa_structures/hashmap.c`) to map integer file descriptors (fd) to open file objects, allowing $O(1)$ access during `read`/`write` operations.
*   **Storage:** File data is stored in dynamically allocated heap buffers that grow as data is written.

## 4.6 Drivers
*   **Timer (PIT):** The Programmable Interval Timer is configured in Mode 2 (Rate Generator) to fire interrupts at 100Hz (`SCHEDULER_TICK_HZ`). This drives the preemptive scheduler.
*   **Keyboard (PS/2):** The driver reads scancodes from I/O port `0x60`, translates them to ASCII using a lookup table, and stores them in a **Circular Buffer**.

## 4.7 System Calls
Userland programs interact with the kernel via a unified interface.
*   **Mechanism:** Software Interrupt `INT 0x80`.
*   **Dispatcher:** The `syscall_handler` (`kernel/syscall.c`) reads the syscall number from the EAX register and dispatches to the appropriate C function using a function pointer table (`syscall_table`).
*   **Implemented Calls:** `exit`, `fork`, `read`, `write`, `open`, `close`, `getpid`, `sleep`, `yield`.

---

# 5. Design Decisions

1.  **Monolithic Architecture:** Chosen for simplicity and performance. All subsystems share the same address space, eliminating the overhead of message passing found in microkernels.
2.  **Bitmap for Physical Memory:** A bitmap is space-efficient (1 bit per 4KB page = ~32KB overhead for 1GB RAM) and allows fast contiguous allocation searches.
3.  **Trie for Filesystem:** While Hash Maps are O(1), they don't support hierarchical listing or prefix matching naturally. A Trie is optimal for path-based lookups (`/home/user/file`) and directory traversal.
4.  **Min-Heap for Priority Scheduling:** Ensures that the highest-priority task is always accessible in O(1) time (root), with efficient updates. This is superior to a sorted list (O(N) insertion) for dynamic priority systems.

---

# 6. Conclusion

NexaKernel successfully demonstrates the construction of a functional, modular operating system kernel. By integrating core Data Structures and Algorithms directly into the kernel's subsystems, the project provides a tangible connection between abstract computer science concepts and low-level systems engineering.

The system achieves its objectives of managing memory, scheduling tasks, and organizing files efficiently. The use of specific data structures—such as Tries for the filesystem and Heaps for the scheduler—resulted in a system that is not only functional but also theoretically sound and optimized.

This project serves as a comprehensive educational platform, showcasing the complexity and beauty of operating system design. Future work could extend NexaKernel with virtual memory paging, a persistent disk-based filesystem, and user-space process isolation.
