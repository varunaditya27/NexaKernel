# NexaKernel: Project Write-up Reference

This document serves as the foundational material for your formal project write-up. It is structured according to academic requirements.

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
    *   **Circular Queues** (Scheduler)
    *   **Binary Heaps** (Priority Scheduling)
    *   **Free Lists** (Memory Allocation)
    *   **Tries & N-ary Trees** (File System Indexing)
    *   **Hash Maps** (File Descriptor Tables)

---

# 4. Methodology

The development of NexaKernel followed a modular, layered approach. The system was built in C (for logic) and Assembly (for hardware interaction), compiled using a cross-compiler toolchain (`i686-elf-gcc`).

## 4.1 Booting and Initialization
The boot process begins with a custom bootloader (or GRUB) that loads the kernel into memory.
*   **GDT Setup:** The Global Descriptor Table is configured to define code and data segments, transitioning the CPU from 16-bit Real Mode to 32-bit Protected Mode.
*   **IDT Setup:** The Interrupt Descriptor Table is populated with Interrupt Service Routines (ISRs) to handle exceptions (like Divide-by-Zero) and hardware interrupts (IRQs).

## 4.2 Memory Management Subsystem
The memory manager is the backbone of the OS, responsible for tracking available RAM.
*   **Physical Memory:** Uses a **Bitmap** to track free/used 4KB physical frames.
*   **Heap Allocator (`kmalloc`/`kfree`):**
    *   **Data Structure:** A **Doubly Linked Free List**.
    *   **Algorithm:** **First-Fit** allocation strategy. When a block is requested, the allocator traverses the list to find the first block large enough. If the block is significantly larger, it is **split** into two.
    *   **Coalescing:** When memory is freed, the allocator checks adjacent blocks. If they are also free, they are merged (**coalesced**) into a single larger block to reduce external fragmentation.

## 4.3 Process Scheduler
The scheduler enables multitasking by rapidly switching the CPU between tasks.
*   **Task Control Block (TCB):** A `task_t` struct holds the task's state (registers `ESP`, `EBP`, `EIP`), stack pointer, and status flags.
*   **Scheduling Algorithms:**
    1.  **Round-Robin:** Uses a **Circular Queue**. Tasks are dequeued, run for a time slice (handled by the PIT timer), and re-queued. This ensures fairness ($O(1)$ complexity).
    2.  **Priority Scheduling:** Uses a **Binary Min-Heap**. Tasks with higher priority (lower numerical value) are always at the root, ensuring the most critical task runs next ($O(\log N)$ complexity).
*   **Context Switching:** Implemented in Assembly, this routine manually saves the current task's registers to its stack and loads the new task's stack pointer.

## 4.4 Virtual File System (RAMFS)
A non-persistent, in-memory file system was implemented to demonstrate file management.
*   **Directory Structure:** An **N-ary Tree** represents the hierarchy. Each directory node points to a list of child nodes (subdirectories or files).
*   **Path Indexing:** A **Trie (Prefix Tree)** is used to index file paths. This allows for $O(L)$ lookup time (where $L$ is path length), which is significantly faster than linear search for deep directory structures.
*   **File Descriptors:** An **Open File Table** uses a **Hash Map** to map integer file descriptors (fd) to open file objects, allowing $O(1)$ access during `read`/`write` operations.
*   **Storage:** File data is stored in dynamically allocated heap buffers that grow as data is written.

## 4.5 Inter-Process Communication (IPC)
Basic IPC mechanisms allow tasks to coordinate.
*   **Message Passing:** Implemented using **FIFO Message Queues**, allowing tasks to send and receive data packets safely.

---

# 5. Conclusion

NexaKernel successfully demonstrates the construction of a functional, modular operating system kernel. By integrating core Data Structures and Algorithms directly into the kernel's subsystems, the project provides a tangible connection between abstract computer science concepts and low-level systems engineering.

The system achieves its objectives of managing memory, scheduling tasks, and organizing files efficiently. The use of specific data structures—such as Tries for the filesystem and Heaps for the scheduler—resulted in a system that is not only functional but also theoretically sound and optimized.

This project serves as a comprehensive educational platform, showcasing the complexity and beauty of operating system design. Future work could extend NexaKernel with virtual memory paging, a persistent disk-based filesystem, and user-space process isolation.
