# NexaKernel Operating System Report

## 1. System Overview
NexaKernel is a 32-bit x86 monolithic operating system kernel developed from scratch. It demonstrates core operating system concepts including protected mode execution, physical and virtual memory management (partial), preemptive multitasking, and hardware abstraction through device drivers. The project places a strong emphasis on the practical application of Data Structures and Algorithms (DSA) within system-level software.

## 2. Architecture & Boot Process
### Bootloader Integration
The kernel is Multiboot-compliant, allowing it to be loaded by standard bootloaders like GRUB.
- **Entry Point**: `boot/bootloader.asm` handles the initial handoff.
- **Protected Mode**: The boot assembly sets up the Global Descriptor Table (GDT) to define a flat memory model (code/data segments spanning 4GB) and transitions the CPU from 16-bit Real Mode to 32-bit Protected Mode (`CR0` register modification).
- **Stack Setup**: A graphical stack is initialized before jumping to the C entry point `kernel_main`.

## 3. Memory Management
The memory subsystem is divided into two layers:

### Physical Memory Manager (PMM)
- **Algorithm**: Bitmap Allocator.
- **Implementation**: `kernel/memory/frame_allocator.c`.
- **Logic**: Memory is divided into 4KB data blocks ("frames"). A bit array tracks the status of each frame (1=Used, 0=Free). This allows for O(n) verification of contiguous blocks and O(1) status updates.

### Kernel Heap (Dynamic Memory)
- **Algorithm**: Free List Implementation.
- **Implementation**: `kernel/memory/heap_allocator.c`.
- **Logic**: A singly linked list maintains blocks of free memory. `kmalloc` searches this list (First-Fit strategy) to find a suitable block, splitting it if necessary to minimize internal fragmentation. `kfree` returns blocks to the list and attempts to coalesce adjacent free blocks to reduce external fragmentation.

## 4. Process Management & Scheduling
### Task Control
- **Structure**: `task_t` (Task Control Block) stores CPU registers (`esp`, `eip`, etc.), process ID (PID), state, and priority.
- **Context Switching**: Implemented in assembly (`context_switch.asm`), this routine saves the current processor state to the task's kernel stack and loads the state of the next task.

### Scheduler
- **Algorithm**: Preemptive Round-Robin.
- **Structure**: A **Circular Queue** holds `READY` tasks.
- **Logic**: The Programmable Interval Timer (PIT) fires interrupts at 100Hz. On each tick, the current task's time slice is decremented. If it reaches zero, the scheduler preempts the task, moves it to the back of the queue, and context switches to the next available task.

## 5. Interrupt Handling & Device Drivers
### Interrupt Descriptor Table (IDT)
The kernel installs a full 256-entry IDT to handle:
- **CPU Exceptions** (0-31): Page Faults, Divide-by-Zero, etc.
- **Hardware Interrupts** (32-47): Remapped from the defaults to avoid collision with CPU exceptions.
- **System Calls** (128/0x80): User-space to Kernel-space gateway.

### Device Drivers
- **VGA Console**: Memory-mapped I/O at `0xB8000`. Features a **Circular History Buffer** (1000 lines) to support scrolling via Up/Down arrow keys.
- **Keyboard (PS/2)**: Interrupt-driven driver (IRQ 1). Uses a **Ring Buffer** to store incoming scancodes. Supports extended keys (Arrows, Home, End) by mapping them to special internal keycodes (`KEY_UP`, `KEY_DOWN`), decoupling hardware scan codes from ASCII input.
- **Timer (PIT)**: programmed to 100Hz frequency to drive the scheduler.

## 6. Data Structures Used
The kernel explicitly demonstrates the utility of various data structures:
1.  **Bitmap**: Physical Frame Allocation.
2.  **Linked List**: Kernel Heap (Free List) & Task Lists.
3.  **Circular Queue**: Process Scheduler (Round-Robin).
4.  **Ring Buffer**: Keyboard Input Buffer & VGA Scroll History.
5.  **Trie / Hash Map**: Filesystem Indexing (in-memory VFS).

## 7. Future Work
-   Virtual Memory (Paging) implementation.
-   User-space process isolation (Ring 3).
-   Extended filesystem support.

