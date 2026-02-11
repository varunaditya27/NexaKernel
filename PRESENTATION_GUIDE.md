# 🎓 NexaKernel: Project Presentation Guide

This guide is designed to help you present **NexaKernel** to OS professors and engineering faculty. It focuses on demonstrating both **Systems Engineering** competence (low-level details) and **Data Structures & Algorithms (DSA)** mastery (practical application).

---

## 🕒 Presentation Structure (15-20 Minutes)

1.  **Hook & Introduction (2 mins)** - What is it? Why build it?
2.  **Architecture Overview (3 mins)** - Boot process & High-level design.
3.  **Deep Dive: The "Big Three" Subsystems (6 mins)** - Scheduler, Memory, Filesystem.
4.  **System Internals: Interrupts & Syscalls (4 mins)** - The nervous system.
5.  **Live Demo Walkthrough (3 mins)** - Showing it runs.
6.  **Q&A Prep (Buffer)** - Answering technical questions.

---

## 1. 🎤 Introduction & Problem Statement

**Slide Goal:** Set the stage. You didn't just write code; you solved a systems problem.

*   **Hook:** "Most OS courses stop at theory. I wanted to see how the theory actually translates to bare-metal code."
*   **What is NexaKernel?**
    *   A modular, 32-bit x86 operating system kernel written from scratch in C and Assembly.
    *   It boots from a custom bootloader (GRUB/Multiboot), manages memory, schedules tasks, and handles hardware interrupts.
*   **The Unique Angle (The "Why"):**
    *   "Crucially, this project bridges the gap between **OS Theory** and **Data Structures**. Every subsystem is built around a specific, optimal data structure implementation."

---

## 2. 🏗️ High-Level Architecture & Boot Process

**Slide Goal:** Show you understand how an OS starts and fits together.

### A. The Boot Sequence (`boot/bootloader.asm`)
*   **The Handoff:** "The BIOS loads GRUB, which loads my kernel. My entry point `_start` is in `boot/bootloader.asm`."
*   **Multiboot Magic:** "I first verify the Multiboot magic number (`0x2BADB002`) to ensure a compliant bootloader loaded us."
*   **Global Descriptor Table (GDT):**
    *   "I set up a **Flat Memory Model** using the GDT (`boot/gdt.asm`)."
    *   "Code Segment: Base `0x0`, Limit `4GB`, Execute/Read."
    *   "Data Segment: Base `0x0`, Limit `4GB`, Read/Write."
    *   "This transitions the CPU into **32-bit Protected Mode**, enabling access to the full 4GB address space."

### B. Kernel Initialization (`kernel/kernel.c`)
*   "Once in C (`kernel_main`), I initialize:
    1.  **IDT** (Interrupt Descriptor Table).
    2.  **Physical Memory** (Bitmap Allocator).
    3.  **Heap** (Dynamic Allocator).
    4.  **Drivers** (PIT, Keyboard, VGA).
    5.  **Scheduler** (Multitasking).
    6.  **Syscalls** (Userland interface)."

---

## 3. 🔬 Deep Dive: The "Big Three" (The Meat of the Project)

This is where you impress them. Pick **2-3** of these to explain in depth depending on time.

### A. The Scheduler (`kernel/scheduler/scheduler.c`)

*   **The Concept:** Preemptive Multitasking.
*   **The DSA:**
    *   **Round-Robin:** Implemented using a **Circular Queue** (`kernel/scheduler/dsa_structures/round_robin_queue.c`). Used for standard tasks. O(1) enqueue/dequeue.
    *   **Priority Scheduling:** Implemented using a **Binary Min-Heap** (`kernel/scheduler/dsa_structures/priority_queue.c`). Used for high-priority tasks. O(log n) insertion/extraction.
*   **Code Highlight:**
    *   "I defined a `task_t` struct (Task Control Block) that stores register state (`esp`, `ebp`), stack pointers, and process flags."
    *   "The `schedule()` function is called on every timer interrupt (IRQ0). It saves the current context to the stack and switches to the next task."
    *   "Context switching is handled in pure Assembly (`context_switch.asm`) to manipulate CPU registers directly."

### B. Memory Management (`kernel/memory/`)

*   **The Concept:** Managing Physical RAM and Kernel Heap.
*   **Physical Allocator (`kernel/memory/dsa_structures/bitmap.c`):**
    *   **DSA:** **Bitmap**.
    *   "I divide physical RAM into 4KB page frames. A bitmap tracks availability (1 bit = 1 page). Allocation uses `bitmap_find_first_zero` for O(N) search (optimized with word-scanning)."
*   **Heap Allocator (`kernel/memory/heap_allocator.c`):**
    *   **DSA:** **Doubly Linked Free List** with **Coalescing**.
    *   "Each block has a header (`heap_block_t`) with size and magic number."
    *   "**Allocation:** Uses a **First-Fit** strategy. If a block is too big, it splits it."
    *   "**Deallocation:** `kfree` marks the block as free and checks `prev` and `next` pointers to merge (coalesce) adjacent free blocks, reducing fragmentation."

### C. The File System (`kernel/fs/ramfs.c`)

*   **The Concept:** A Virtual File System (VFS) backed by RAM.
*   **The DSA:**
    *   **Trie (Prefix Tree):** Used for fast **path lookup** (`kernel/fs/dsa_structures/trie.c`). This makes file retrieval O(L) where L is path length, independent of the number of files.
    *   **N-ary Tree:** Represents the **directory hierarchy** (`directory_tree.c`). Folders point to a list of child nodes.
    *   **Hash Map:** Used for the **Open File Descriptor Table** ($O(1)$ access to open file handles).
*   **Code Highlight:**
    *   "Files are nodes in the tree (`ramfs_inode_t`). They contain metadata and a pointer to the data buffer."
    *   "The system supports standard operations like `open`, `read`, `write`, `close`, `mkdir`, and `unlink`."

---

## 4. ⚡ System Internals: Interrupts & Syscalls

**Slide Goal:** Demonstrate low-level control flow.

### A. Interrupt Descriptor Table (IDT) (`kernel/interrupts/idt.c`)
*   "I configured the IDT with 256 entries."
    *   **0-31:** CPU Exceptions (e.g., #0 Divide-by-Zero, #13 GPF, #14 Page Fault).
    *   **32-47:** Hardware IRQs (Remapped from PIC).
    *   **0x80 (128):** System Calls.
*   "The Programmable Interrupt Controller (PIC) is remapped to offset 32 to avoid conflicts with CPU exceptions."

### B. System Calls (`kernel/syscall.c`)
*   "Userland programs interact with the kernel via `INT 0x80`."
*   **Mechanism:**
    *   **EAX:** Syscall Number (e.g., 1=EXIT, 3=READ, 4=WRITE).
    *   **EBX, ECX, EDX:** Arguments.
*   "The `syscall_handler` function uses a jump table (`syscall_table`) to dispatch the correct function based on EAX."

### C. Drivers
*   **Timer (PIT):** Configured in Mode 2 (Rate Generator) at 100Hz. Drives the scheduler preemption.
*   **Keyboard (PS/2):** interrupt-driven. Scancodes are read from port `0x60`, translated to ASCII using a lookup table, and stored in a **Circular Buffer**.

---

## 5. 🖥️ Live Demo Walkthrough

**Step 1: Booting**
*   Run `make run` (or `make run-iso`).
*   **Say:** "Here you see the GRUB bootloader handing control to my kernel entry point. The kernel initializes the GDT, IDT, and Memory Manager."

**Step 2: The Shell**
*   **Action:** Type `help` to show available commands.
*   **Action:** Type `meminfo` (if available) to show the memory map.
    *   **Say:** "This confirms the memory allocator is tracking used vs. free bytes."

**Step 3: Multitasking**
*   **Action:** Run a command that spawns tasks (e.g., `tasks` or a demo program).
    *   **Say:** "You can see multiple tasks printing to the screen simultaneously. This proves the preemptive scheduler is context-switching hundreds of times per second."

**Step 4: Filesystem**
*   **Action:** `touch test.txt`, `write test.txt "Hello OS"`, `cat test.txt`.
    *   **Say:** "This demonstrates the RAMFS. The file data was allocated in the heap, indexed in the Trie, and retrieved."

---

## 6. ❓ Q&A Preparation (Anticipate these!)

**Q: How do you handle concurrency/race conditions?**
*   **A:** "For this version, I use **interrupt disabling** (`cli`/`sti`) as a coarse-grained lock during critical kernel sections (like scheduling or memory allocation) to ensure atomicity on a single-core setup."

**Q: Why a Trie for the filesystem?**
*   **A:** "A Trie is optimal for prefix-based lookups like file paths (`/home/user/file`). It avoids comparing the full string at every node, making path resolution extremely fast compared to a linear search."

**Q: How does the context switch work?**
*   **A:** "It manually pushes all General Purpose Registers (EAX, EBX, etc.) onto the current task's stack, saves the Stack Pointer (ESP) to the `task_t` struct, loads the new task's ESP, and pops the registers back off."

**Q: What was the hardest part?**
*   **A:** "Debugging **Triple Faults** during early boot. Moving from Real Mode to Protected Mode requires setting up the GDT perfectly; one wrong byte causes the CPU to reset. Also, ensuring the stack is 16-byte aligned for GCC assumptions."

**Q: How does the Priority Queue work for scheduling?**
*   **A:** "It's a Min-Heap. The task with the lowest priority value (highest importance) is always at the root. Insertion is O(log n) because we bubble-up. Extraction is O(log n) because we swap the last element to the root and bubble-down."

---

## 📝 Key Files to Have Open During Presentation

1.  `kernel/scheduler/scheduler.c` (The "Brain")
2.  `kernel/memory/heap_allocator.c` (The "Muscle")
3.  `kernel/fs/ramfs.c` (The "Organizer")
4.  `kernel/interrupts/idt.c` (The "Nervous System")
5.  `boot/bootloader.asm` (The "Spark")

Good luck! You have built a real, working system. Be proud of the complexity you have managed.
