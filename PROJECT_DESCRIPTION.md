# 📘 NexaKernel — Comprehensive Project Description

A fully from-scratch operating system kernel integrated with real-world data structure implementations

---

# 1. Introduction

NexaKernel is a lightweight, modular operating system kernel designed and implemented entirely from scratch. The project serves as a combined academic submission for both the **Operating Systems Laboratory** and **Data Structures & Applications Laboratory**.

The core objective of NexaKernel is twofold:

1. **To implement the essential components of an operating system**, including bootloading, memory management, interrupt handling, task scheduling, system calls, device drivers, and basic userland support.
2. **To demonstrate practical, meaningful usage of core data structures** such as queues, heaps, trees, tries, hash maps, linked lists, bitmaps, and graphs within the internal subsystems of the OS.

This makes NexaKernel a unique educational OS project that is simple enough for academic evaluation yet architecturally powerful enough for long-term expansion into advanced systems concepts.

---

# 2. Project Vision

The vision behind NexaKernel is to build a clean, comprehensible, and extensible operating system that mirrors the structure of real kernels without unnecessary complexity. Along with core OS functionality, the kernel intentionally incorporates **DSA-based subsystem designs** to show how complex data structures form the backbone of low-level systems.

The design philosophy is based on:

* **Minimalism:** Only implement features necessary for understanding OS internals.
* **Modularity:** Each subsystem is isolated and replaceable.
* **DSA Traceability:** Every data structure serves a real, practical purpose.
* **Extensibility:** Future modules (filesystems, networking, paging, AI-based scheduling) can be added without restructuring the kernel.

---

# 3. Core Components of NexaKernel

NexaKernel is composed of the following foundational subsystems:

## 3.1 Bootloader and Kernel Initialization

The system begins with a custom-written bootloader that executes in real mode and transitions the CPU into protected mode. It loads the NexaKernel binary into memory and transfers control to the main kernel entry function.

During early initialization, the kernel:

* Sets up a Global Descriptor Table.
* Initializes basic memory regions.
* Loads interrupt descriptors.
* Configures essential hardware.
* Starts the system timer.
* Initializes device drivers.
* Prepares the scheduler.

This sequence establishes the minimal environment required for higher-level kernel components to operate.

---

## 3.2 Interrupt Handling and System Calls

The kernel defines and installs an Interrupt Descriptor Table that manages both software-generated interrupts and hardware IRQs.

Interrupts handle:

* CPU exceptions
* Timer ticks
* Keyboard input
* Future device events

A system call interface allows user programs to request privileged operations such as writing to the screen or terminating execution.

---

## 3.3 Memory Management

NexaKernel’s memory management subsystem consists of:

### Physical Memory Allocation

A bitmap-based or buddy-tree-based frame allocator tracks available physical memory. This allocator is responsible for giving out memory blocks in fixed-size chunks.

### Kernel Heap

A kernel heap uses a free-list structure to dynamically allocate memory for kernel subsystems, allowing them to request variable-sized memory blocks at runtime.

These structures demonstrate real-world allocator design and allow later expansion into virtual memory and paging.

---

## 3.4 Scheduler and Multitasking

The scheduler manages the execution of multiple tasks in a cooperative or preemptive manner using timer interrupts.

It uses DSA-backed structures such as:

* **Circular queues** for round-robin scheduling.
* **Priority queues** for priority-based scheduling.
* **Heaps** for efficient priority lookups.

Tasks are represented through task-control structures that store their register states, stack location, and process information. Context switching is performed through low-level assembly routines.

This provides hands-on insight into how real kernels manage multitasking and CPU time allocation.

---

## 3.5 Device Drivers

NexaKernel includes minimal drivers required for early kernel interaction:

* VGA text driver for screen output.
* PS/2 keyboard driver for user input.
* System timer driver to generate periodic interrupts.

These drivers showcase how kernels communicate with hardware and how interrupts integrate with device handling.

---

## 3.6 Simple RAM-Based File System

The kernel includes a lightweight, memory-resident filesystem designed for ease of implementation and demonstration.

This filesystem includes:

* A directory hierarchy represented as a tree.
* File name indexing via a trie structure.
* A hash map-based file table for quick open-file lookups.

This demonstrates how filesystems fundamentally rely on data structures for performance and organization.

---

## 3.7 Interprocess Communication (IPC)

Basic mechanisms for communication between tasks are implemented through:

* **Message queues** for FIFO-based communication.
* **Shared memory regions** for high-speed data sharing.

These lay the groundwork for future expansion into pipes, signals, and sockets.

---

## 3.8 Userland Environment

NexaKernel supports a basic userland consisting of:

* A simple shell with minimal command parsing.
* Basic programs that interact with kernel system calls.
* A lightweight C library for essential user functions.

This allows demonstration of kernel-to-user interactions and system-call-driven application execution.

---

# 4. Data Structures in NexaKernel

A major goal of the project is showcasing realistic data-structure usage inside an operating system. NexaKernel integrates multiple DSAs across its subsystems:

* **Circular Queues:** Task rotation in round-robin scheduling.
* **Priority Queues and Heaps:** Priority-based scheduling and efficient task selection.
* **Bitmaps:** Physical memory frame tracking.
* **Buddy Tree:** Memory block allocation.
* **Free Lists:** Kernel heap block management.
* **Tries:** Fast file name lookup.
* **N-ary Trees:** Directory hierarchy representation.
* **Hash Maps:** File descriptor tables and system-call mapping.
* **Linked Lists:** Task state lists and dynamic kernel structures.
* **Graphs (optional module):** Mapping system components or process dependencies.

Each of these structures is implemented from scratch and tied into a subsystem with clear practical benefits.

---

# 5. Educational Value

NexaKernel offers strong educational value for multiple domains:

### Operating Systems

Students understand:

* Boot process fundamentals
* Protected mode switching
* Memory maps and allocation
* Scheduling strategies
* Interrupt handling
* Kernel-user separation

### Data Structures

Students apply:

* Realistic DSA usage beyond textbook examples
* Performance-aware data structure selection
* Structures used in production-grade OS kernels

### Systems Programming

The project reinforces:

* Low-level C
* Assembly integration
* Hardware-software interfacing
* Modular architecture design

---

# 6. Scope for Future Work

NexaKernel is intentionally designed with expansion in mind. Future enhancements may include:

* Virtual memory and paging
* Demand paging and memory protection
* Process isolation and privilege levels
* System call extensions
* Networking stack (TCP-lite, packet handler)
* On-disk filesystem implementation
* Multicore (SMP) support
* AI-driven scheduling or predictive resource allocation
* Advanced security modules

The modular architecture ensures new features can be added without major rewrites.

---

# 7. Conclusion

NexaKernel is more than just a minimal kernel—it is an educational platform and a systems engineering playground. It bridges conceptual learning with practical implementation, demonstrating precisely how data structures underpin core operating system functionality.

The project is designed to be simple enough for academic submission, yet deep, scalable, and engineered to grow into an advanced OS framework. This balance of clarity and complexity makes NexaKernel a powerful demonstration of OS fundamentals, DSA application, and clean software architecture.

---

**NexaKernel — where OS engineering meets data structure mastery.**
