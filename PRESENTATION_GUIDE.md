# 🎓 NexaKernel: Project Presentation Guide

This guide is designed to help you present **NexaKernel** to OS professors and engineering faculty. It focuses on demonstrating both **Systems Engineering** competence and **Data Structures & Algorithms (DSA)** mastery.

---

## 🕒 Presentation Structure (10-15 Minutes)

1.  **Hook & Introduction (2 mins)** - What is it? Why build it?
2.  **Architecture Overview (2 mins)** - High-level design.
3.  **Deep Dive: The "Big Three" Subsystems (5 mins)** - Scheduler, Memory, Filesystem.
4.  **Live Demo Walkthrough (3 mins)** - Showing it runs.
5.  **Q&A Prep (Buffer)** - Answering technical questions.

---

## 1. 🎤 Introduction & Problem Statement

**Slide Goal:** Set the stage. You didn't just write code; you solved a systems problem.

*   **Hook:** "Most OS courses stop at theory. I wanted to see how the theory actually translates to bare-metal code."
*   **What is NexaKernel?**
    *   A modular, 32-bit x86 operating system kernel written from scratch in C and Assembly.
    *   It boots from a custom bootloader, manages memory, schedules tasks, and handles hardware interrupts.
*   **The Unique Angle (The "Why"):**
    *   "Crucially, this project bridges the gap between **OS Theory** and **Data Structures**. Every subsystem is built around a specific, optimal data structure implementation."

---

## 2. 🏗️ High-Level Architecture

**Slide Goal:** Show you understand how an OS fits together.

*   **Visual Aid:** (Point to `docs/architecture_diagram.png` if available, or draw a simple block diagram).
*   **Key Layers:**
    1.  **Hardware Abstraction:** (Drivers for VGA, Keyboard, Timer, PIC).
    2.  **Kernel Core:** (Interrupt handling, Panic system, Utils).
    3.  **Subsystems:** (Scheduler, Memory Manager, VFS).
    4.  **Userland:** (Shell, User programs).
*   **The Build System:**
    *   "I use a custom Makefile with a cross-compiler toolchain (`i686-elf-gcc`) to ensure clean binary generation for the i386 architecture."

---

## 3. 🔬 Deep Dive: The "Big Three" (The Meat of the Project)

This is where you impress them. Pick **2-3** of these to explain in depth depending on time.

### A. The Scheduler (`kernel/scheduler/scheduler.c`)

*   **The Concept:** Preemptive Multitasking.
*   **The DSA:**
    *   **Round-Robin:** Implemented using a **Circular Queue**.
    *   **Priority Scheduling:** Implemented using a **Binary Min-Heap**.
*   **Code Highlight:**
    *   "I defined a `task_t` struct (Task Control Block) that stores register state (`esp`, `ebp`), stack pointers, and process flags."
    *   "The `schedule()` function is called on every timer interrupt (via the PIT driver). It saves the current context to the stack and switches to the next task in the queue."
    *   "Context switching is handled in pure Assembly (`context_switch.asm`) to manipulate CPU registers directly."

### B. Memory Management (`kernel/memory/heap_allocator.c`)

*   **The Concept:** Dynamic Memory Allocation (`kmalloc`/`kfree`).
*   **The DSA:** **Free List** with **First-Fit** Strategy & **Coalescing**.
*   **Code Highlight:**
    *   "I treat the heap as a linked list of memory blocks. Each block has a header (`heap_block_t`) containing its size, a magic number (for corruption detection), and a 'free' flag."
    *   "**Allocation:** The allocator traverses the list to find the first block that fits (`First-Fit`). If the block is too big, it **splits** it into two."
    *   "**Deallocation:** When `kfree` is called, it marks the block as free and attempts to **coalesce** (merge) it with adjacent free blocks to prevent fragmentation."
*   **Why this matters:** "This demonstrates understanding of fragmentation, alignment (8-byte aligned), and pointer arithmetic."

### C. The File System (`kernel/fs/ramfs.c`)

*   **The Concept:** A Virtual File System (VFS) backed by RAM.
*   **The DSA:**
    *   **Trie (Prefix Tree):** Used for fast **path lookup** and indexing. This makes file retrieval $O(L)$ where $L$ is path length, instead of $O(N)$.
    *   **N-ary Tree:** Represents the **directory hierarchy** (Folders containing files/folders).
    *   **Hash Map:** Used for the **Open File Descriptor Table** ($O(1)$ access to open file handles).
*   **Code Highlight:**
    *   "Files are nodes in the tree (`ramfs_inode_t`). They contain metadata (name, size, type) and a pointer to the data buffer."
    *   "The `ramfs_read` and `ramfs_write` functions handle buffer management, dynamically resizing file data buffers in the heap as needed."

---

## 4. 🖥️ Live Demo Walkthrough

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

## 5. ❓ Q&A Preparation (Anticipate these!)

**Q: How do you handle concurrency/race conditions?**
*   **A:** "For this version, I use **interrupt disabling** (`cli`/`sti`) as a coarse-grained lock during critical kernel sections (like scheduling or memory allocation) to ensure atomicity on a single-core setup."

**Q: Why a Trie for the filesystem?**
*   **A:** "A Trie is optimal for prefix-based lookups like file paths (`/home/user/file`). It avoids comparing the full string at every node, making path resolution extremely fast."

**Q: How does the context switch work?**
*   **A:** "It manually pushes all General Purpose Registers (EAX, EBX, etc.) onto the current task's stack, saves the Stack Pointer (ESP) to the `task_t` struct, loads the new task's ESP, and pops the registers back off."

**Q: What was the hardest part?**
*   **A:** "Debugging the **Triple Faults** during early boot. Moving from Real Mode to Protected Mode requires setting up the GDT perfectly; one wrong byte causes the CPU to reset."

---

## 📝 Key Files to Have Open During Presentation

1.  `kernel/scheduler/scheduler.c` (The "Brain")
2.  `kernel/memory/heap_allocator.c` (The "Muscle")
3.  `kernel/fs/ramfs.c` (The "Organizer")
4.  `kernel/interrupts/idt.c` (The "Nervous System")

Good luck! You have built a real, working system. Be proud of the complexity you have managed.
