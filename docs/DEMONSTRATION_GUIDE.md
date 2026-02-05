# NexaKernel Demonstration Guide

## For OS Course Project Presentation

This guide provides a structured walkthrough for demonstrating NexaKernel to your Operating Systems professor. The kernel now features **extensive visual ASCII diagrams** that explain each OS concept as it executes, making it easy to understand what's happening and why.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Boot Sequence Demonstration](#1-boot-sequence-demonstration)
3. [Memory Management](#2-memory-management)
4. [Interrupt Handling](#3-interrupt-handling)
5. [Device Drivers](#4-device-drivers)
6. [Process Scheduling](#5-process-scheduling)
7. [Interactive Commands](#6-interactive-commands)
8. [Key Talking Points](#7-key-talking-points)
9. [Source Code Walkthrough](#8-source-code-walkthrough)

---

## Quick Start

### Prerequisites
```bash
# Install required packages (Ubuntu/Debian)
sudo apt install build-essential gcc-multilib nasm qemu-system-x86

# Navigate to project directory
cd /path/to/NexaKernel

# Build and run
make clean && make
make run
```

### Two Display Modes
When running `make run`, output appears in **two places**:

1. **Terminal (Serial Console)**: Shows all kernel messages via COM1 serial port - **BEST FOR READING**
2. **QEMU VGA Window**: Shows the same output on the emulated display

**Tip**: For your presentation, position both windows side-by-side. The **terminal output is easier to read** and can be scrolled back to review earlier messages.

### What Makes This Demonstration Special

The kernel now includes **inline visual explanations** that appear during boot:
- ASCII diagrams of data structures (bitmap, free list, queues)
- Flowcharts showing interrupt and scheduling flows
- Memory layout visualizations
- Step-by-step explanations of algorithms (with before/after states)

---

## 1. Boot Sequence Demonstration

### What You'll See

When the kernel boots, you'll see a beautifully formatted sequence with the NexaKernel ASCII art banner, followed by four clearly labeled phases:

```
+========================================================+
|     _   _                _  __                   _     |
|    | \ | | _____  ____ _| |/ /___ _ __ _ __   ___| |    |
|    |  \| |/ _ \ \/ / _` | ' // _ \ '__| '_ \ / _ \ |    |
|    | |\  |  __/>  < (_| | . \  __/ |  | | | |  __/ |    |
|    |_| \_|\___/_/\_\__,_|_|\_\___|_|  |_| |_|\___|_|    |
|                                                        |
|          Educational x86 Operating System Kernel       |
+========================================================+

[BOOT] Kernel loaded at: 0x00100000
[BOOT] Kernel ends at:   0x00139000
[BOOT] Kernel size:      228 KB
```

Each phase is clearly marked with a header box, making it easy to follow the initialization sequence.

### Key Boot Information to Explain

| Message | What It Means |
| ------- | ------------- |
| `Kernel loaded at: 0x00100000` | Kernel loads at 1MB mark (protected mode convention, avoids BIOS area) |
| `Multiboot flags: 0x0000024F` | GRUB passes system info (memory map, boot device) via Multiboot |
| `Lower memory: 639 KB` | Conventional memory below 1MB (legacy DOS compatibility area) |
| `Upper memory: 126 MB` | Extended memory above 1MB available for kernel use |

### Key Source Files

- `boot/multiboot_header.asm` - Multiboot magic numbers (tells GRUB this is a kernel)
- `boot/bootloader.asm` - Entry point, GDT setup, jump to C code
- `boot/gdt.asm` - Global Descriptor Table (flat memory model for 32-bit)

---

## 2. Memory Management

### Visual Demonstrations During Boot

The kernel now shows **inline ASCII diagrams** explaining memory management:

**Frame Allocator Bitmap Visualization:**
```
  BITMAP VISUALIZATION (first 64 frames):
  [0=free, 1=used]
  Frame 0         16        32        48        64
        |          |         |         |         |
        v          v         v         v         v
        0000000000000000000000000000000000000000000000000000000000000000
        ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^
        All frames initially FREE (ready for allocation)

  After reserving kernel region:
        1111111111111111111111111100000000000000000000000000000000000000
        ^~~~~~~~~~~~~~~~~~~~~~~~^
        Kernel code/data (RESERVED)
```

**Heap Memory Allocation Visualization (during memory test):**
```
  STEP 1: kmalloc(256) - Allocate 256 bytes
  +--------+--------------------------------------------------+
  |  A:256 |             FREE (remaining)                    |
  +--------+--------------------------------------------------+
     ^~~~~ Block A = 256 bytes (USED)

  STEP 3: kfree(A) - Free the first 256-byte block
  +--------+-------------+------------------------------------+
  | FREE   |   B:1024    |          FREE (remaining)          |
  | (256)  |   (USED)    |                                    |
  +--------+-------------+------------------------------------+
     ^~~~~~ Now on free list!

  STEP 4: kmalloc(128) - Request 128 bytes (will REUSE freed block)
  >>> SAME ADDRESS AS A! Free list reuse confirmed! <<<
```

### Interactive Memory Demo (Press 'M')

Press `M` in QEMU to see a comprehensive memory map:

```
+============================================================+
|              MEMORY SUBSYSTEM STATISTICS                   |
+============================================================+
|                                                            |
| PHYSICAL MEMORY LAYOUT:                                    |
|                                                            |
|  0x00000000 +------------------+                           |
|             | Real Mode IVT    | 1 KB                      |
|  0x00000400 +------------------+                           |
|             | BIOS Data        | 256 bytes                 |
|  0x00000500 +------------------+                           |
|             | Conventional Mem | ~638 KB                   |
|  0x000A0000 +------------------+                           |
|             | Video Memory     | 128 KB                    |
|  0x000C0000 +------------------+                           |
|             | ROM / Reserved   | 256 KB                    |
|  0x00100000 +------------------+ <- KERNEL STARTS HERE     |
|             | Kernel Code/Data |                           |
|  0x00200000 +------------------+ <- HEAP STARTS HERE       |
|             | Kernel Heap      | 16 MB                     |
|             +------------------+                           |
|             | Free Frames      | (managed by bitmap)       |
|             +------------------+                           |
+============================================================+
```

### What to Explain

**Physical Frame Allocator:**
- **Data Structure**: Bitmap (1 bit per 4KB frame)
- **Algorithm**: First-fit scan through bitmap for free frame
- **Purpose**: Manages physical memory at page granularity (4KB chunks)
- **Complexity**: O(n) allocation, O(1) free

**Kernel Heap Allocator:**
- **Data Structure**: Free list (singly linked list of free blocks)
- **Algorithm**: First-fit with block splitting and coalescing
- **Purpose**: Provides `kmalloc()`/`kfree()` for variable-size allocations
- **Key Feature**: Coalescing prevents fragmentation

### Key Source Files

- `kernel/memory/frame_allocator.c` - Physical frame allocation (bitmap)
- `kernel/memory/heap_allocator.c` - Dynamic heap (free list)
- `kernel/memory/dsa_structures/bitmap.c` - Bitmap data structure

---

## 3. Interrupt Handling

### Visual Demonstrations During Boot

The kernel shows detailed ASCII diagrams explaining interrupts:

**IDT Vector Allocation Table:**
```
  IDT VECTOR ALLOCATION:
  +----------+---------+----------------------------------------+
  | Vectors  | Type    | Description                            |
  +----------+---------+----------------------------------------+
  | 0-31     | ISR     | CPU Exceptions (Divide, GPF, PF, etc.) |
  | 32-47    | IRQ     | Hardware Interrupts (Timer, Kbd, etc.) |
  | 48-127   | Reserved| Available for future use               |
  | 128(0x80)| Syscall | System call interface (INT 0x80)       |
  | 129-255  | Reserved| Available for future use               |
  +----------+---------+----------------------------------------+
```

**PIC Remapping Explanation:**
```
  PIC (PROGRAMMABLE INTERRUPT CONTROLLER) SETUP:
  +----------------------------------------------------------+
  | Chip: Dual 8259A (Master + Slave, cascaded via IRQ2)     |
  +----------------------------------------------------------+
  | REMAPPING COMPLETE:                                      |
  |   Master PIC: IRQ0-7  -> Vectors 32-39 (was 8-15)        |
  |   Slave PIC:  IRQ8-15 -> Vectors 40-47 (was 112-119)     |
  +----------------------------------------------------------+
  | WHY REMAP? Default vectors 8-15 conflict with CPU        |
  | exceptions! (e.g., IRQ0 = vector 8 = Double Fault)       |
  +----------------------------------------------------------+
```

**Keyboard Interrupt Flow Diagram:**
```
  KEYBOARD INTERRUPT FLOW:
  +-------+     +---------+     +-----+     +--------+
  | KEY   | --> |  8042   | --> | PIC | --> |  CPU   |
  | PRESS |     | scancode|     | IRQ1|     | INT 33 |
  +-------+     +---------+     +-----+     +--------+
                                              |
                                              v
                                     +----------------+
                                     | kbd_handler:   |
                                     | read 0x60      |
                                     | translate->buf |
                                     +----------------+
```

### Interactive Interrupt Demo (Press 'I')

Press `I` in QEMU to see interrupt statistics:

```
+============================================================+
|              INTERRUPT SUBSYSTEM STATISTICS                |
+============================================================+
| INTERRUPT DESCRIPTOR TABLE (IDT):                          |
|   - 256 entries (gates)                                    |
|   - Vectors 0-31:  CPU Exceptions                          |
|   - Vectors 32-47: Hardware IRQs (remapped)                |
|   - Vector 0x80:   System calls                            |
+------------------------------------------------------------+
| IRQ | VECTOR | DEVICE         | COUNT       | RATE        |
+-----+--------+----------------+-------------+-------------+
|  0  |   32   | PIT Timer      |   5000     | ~100/sec    |
|  1  |   33   | PS/2 Keyboard  |     15     | on keypress |
+-----+--------+----------------+-------------+-------------+
```

### What to Explain

**Interrupt Descriptor Table (IDT):**
- 256 entries, 8 bytes each = 2KB total
- Each entry: segment selector + handler offset + type/attributes
- Vectors 0-31: CPU exceptions (reserved by Intel)
- Vectors 32-47: Hardware IRQs (we remap here)
- Vector 0x80: System calls (like Linux)

**Why PIC Remapping?**
- Default: IRQ0-7 map to vectors 8-15
- Problem: Vectors 8-15 are CPU exceptions (Double Fault, etc.)
- Solution: Remap IRQs to 32-47 (unused vector range)

**Interrupt Flow:**
1. Device triggers IRQ line → PIC
2. PIC sends interrupt signal to CPU
3. CPU saves state, looks up handler in IDT
4. Handler executes, sends EOI to PIC
5. IRET restores state and continues

### Key Source Files

- `kernel/interrupts/idt.c` - IDT initialization
- `kernel/interrupts/isr.c` - CPU exception handlers
- `kernel/interrupts/irq.c` - Hardware IRQ handlers + PIC driver
- `kernel/interrupts/isr_stubs.asm` - Low-level assembly entry points

---

## 4. Device Drivers

### Visual Demonstrations During Boot

**VGA Text Mode Explanation:**
```
  VGA TEXT MODE DRIVER:
  +----------------------------------------------------------+
  | Type: Memory-Mapped I/O                                  |
  | Buffer Address: 0xB8000 (video memory)                   |
  | Resolution: 80 columns x 25 rows = 2000 characters       |
  +----------------------------------------------------------+
  | CHARACTER FORMAT (2 bytes per cell):                     |
  |   Byte 0: ASCII character code                           |
  |   Byte 1: Attribute (fg color | bg color << 4)           |
  +----------------------------------------------------------+
```

**PIT Timer Flow Diagram:**
```
  TIMER INTERRUPT FLOW:
  +--------+     +-----+     +--------+     +------------+
  |  PIT   | --> | PIC | --> |  CPU   | --> | IRQ0       |
  | 100 Hz |     | IRQ0|     | INT 32 |     | Handler    |
  +--------+     +-----+     +--------+     +------------+
                                              |
                                              v
                                     +----------------+
                                     | scheduler tick |
                                     | (preemption)   |
                                     +----------------+
```

**PS/2 Keyboard Details:**
```
  PS/2 KEYBOARD DRIVER:
  +----------------------------------------------------------+
  | Interface: PS/2 (Intel 8042 keyboard controller)         |
  | I/O Ports:                                               |
  |   0x60: Data port (read scancodes, send commands)        |
  |   0x64: Status/Command port                              |
  | IRQ: 1 (mapped to vector 33)                             |
  +----------------------------------------------------------+
```

### What to Explain

**VGA Text Mode:**
- Memory-mapped at 0xB8000 (not port I/O)
- 80x25 = 2000 cells × 2 bytes = 4000 bytes of video RAM
- Each cell: character byte + attribute byte
- No graphics mode - pure text for educational simplicity

**PIT (Programmable Interval Timer):**
- Intel 8254 chip, base frequency 1.193182 MHz
- We set divisor to get 100 Hz (10ms ticks)
- Drives preemptive multitasking via scheduler callback

**PS/2 Keyboard:**
- Uses port I/O (0x60 for data, 0x64 for status)
- Interrupt-driven (IRQ1)
- Scancode Set 1 translation to ASCII

### Interactive Keyboard Demo

Press any key to see the keyboard driver in action:

```
[KEYBOARD DEMO] Key pressed: 'a'  |  ASCII code: 97  |  Hex: 0x61
[KEYBOARD DEMO] Key pressed: 'B'  |  ASCII code: 66  |  Hex: 0x42
```

### Key Source Files

- `kernel/drivers/vga_text.c` - VGA display driver
- `kernel/drivers/timer.c` - PIT timer driver
- `kernel/drivers/keyboard.c` - PS/2 keyboard driver
- `kernel/drivers/serial.c` - Serial port (for terminal output)

---

## 5. Process Scheduling

### Visual Demonstrations During Boot

**Round-Robin Queue Visualization:**
```
  ROUND-ROBIN SCHEDULING ALGORITHM:
  +----------------------------------------------------------+
  |                                                          |
  |    +---------+   +---------+   +---------+               |
  |    | Task A  |-->| Task B  |-->| Task C  |--+            |
  |    +---------+   +---------+   +---------+  |            |
  |         ^                                   |            |
  |         +-----------------------------------+            |
  |                   (circular queue)                       |
  |                                                          |
  | Each task runs for TIME_SLICE ticks, then moves to back  |
  +----------------------------------------------------------+
```

**Task State Machine:**
```
  TASK STATE TRANSITIONS:
  +----------------------------------------------------------+
  |                                                          |
  |   CREATING -----> READY <-----> RUNNING                  |
  |                     ^             |                      |
  |                     |             v                      |
  |                  SLEEPING     BLOCKED                    |
  |                     |             |                      |
  |                     v             v                      |
  |                   TERMINATED <----+                      |
  |                                                          |
  +----------------------------------------------------------+
```

**Context Switch Explanation:**
```
  CONTEXT SWITCH PROCESS:
  +----------------------------------------------------------+
  | 1. Timer interrupt fires (IRQ0 @ 100Hz)                  |
  | 2. Save current task registers (EAX-EDI, ESP, EIP)       |
  | 3. Store ESP in current_task->stack_pointer              |
  | 4. Select next task from ready queue                     |
  | 5. Load ESP from new_task->stack_pointer                 |
  | 6. Restore registers and IRET (return from interrupt)    |
  +----------------------------------------------------------+
```

### Task Execution Demo (Watch the interleaving!)

After boot, you'll see tasks executing in an interleaved fashion:

```
  +--- TASK A STARTED (Producer) ---------------------------+
  | PID: 2 | Priority: 3 (NORMAL) | State: RUNNING          |
  | Role: Simulates a data-producing thread                 |
  +----------------------------------------------------------+
  | [A] Producing item #0  -->  State: RUNNING -> READY (yield)
  | [B] Consuming item #0  -->  State: RUNNING -> READY (yield)
  | [C***] Critical op #0  -->  State: RUNNING -> READY (yield)
  | [A] Producing item #1  -->  State: RUNNING -> READY (yield)
  ...
```

**Key observation**: The interleaved A, B, C output proves context switching is working!

### Interactive Scheduler Demo (Press 'S')

Press `S` in QEMU for detailed scheduler statistics:

```
+============================================================+
|              SCHEDULER SUBSYSTEM STATISTICS                |
+============================================================+
| SCHEDULING ALGORITHM: Round-Robin with Preemptive         |
|                       Time Slicing                        |
| DATA STRUCTURE: Circular Queue (FIFO)                     |
+------------------------------------------------------------+
| METRIC                        | VALUE                     |
+-------------------------------+---------------------------+
| Context Switches              |  2345                     |
| Schedule() Invocations        |  4567                     |
| Ready Queue Size              |     3                     |
| Idle Task Time (ticks)        |  1200                     |
| Total Active Tasks            |     5                     |
| Timer Tick Rate               |   100 Hz                  |
| Time Slice Duration           |    10 ticks (100ms)       |
+-------------------------------+---------------------------+
| CONTEXT SWITCH EXPLANATION:                                |
| Each switch saves/restores 8 registers (32 bytes)         |
| + Updates task->stack_pointer with current ESP            |
+============================================================+
```

### Interactive Task List (Press 'T')

Press `T` to see all active tasks:

```
+============================================================+
|                    ACTIVE TASK LIST                        |
+============================================================+
| TASK STATES:                                               |
|   READY    - In queue, waiting for CPU time                |
|   RUNNING  - Currently executing on CPU                    |
|   BLOCKED  - Waiting for I/O or event                      |
|   SLEEPING - Voluntarily sleeping (timer-based)            |
+-----+------------+----------+----------+------------------+
| PID | Name       | State    | Priority | Role             |
+-----+------------+----------+----------+------------------+
|  0  | idle       | READY    |    7     | CPU idle loop    |
|  1  | main       | RUNNING  |    3     | Interactive shell|
|  2  | demo-A     | READY    |    3     | Producer task    |
|  3  | demo-B     | READY    |    3     | Consumer task    |
|  4  | demo-C     | READY    |    1     | High-priority    |
+-----+------------+----------+----------+------------------+
| PRIORITY LEVELS (0=highest, 7=lowest):                     |
|   0-1: HIGH     - Critical system tasks                    |
|   2-4: NORMAL   - Regular user/kernel tasks                |
|   5-6: LOW      - Background tasks                         |
|   7:   IDLE     - Only runs when nothing else to do        |
+============================================================+
```

### What to Explain

**Task Control Block (TCB):**
- Contains: stack_pointer, pid, state, priority, time_slice, name
- Stack pointer saves the task's execution context
- Each task has its own stack (default 4KB)

**Round-Robin Algorithm:**
- FIFO queue of READY tasks
- Each task runs for `time_slice` ticks (default 10 = 100ms)
- On time slice expiry or yield, task moves to back of queue
- Next task from front of queue runs

**Context Switch (Assembly):**
```asm
; Save old task
pusha                      ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
mov [eax + STACK_OFF], esp ; Save stack pointer

; Load new task  
mov esp, [ebx + STACK_OFF] ; Restore stack pointer
popa                       ; Pop all registers
ret                        ; Return (jumps to saved EIP)
```

### Key Source Files

- `kernel/scheduler/task.c` - Task creation and management
- `kernel/scheduler/scheduler.c` - Scheduling logic
- `kernel/scheduler/context_switch.asm` - Low-level context switch
- `kernel/scheduler/dsa_structures/round_robin_queue.c` - FIFO queue

---

## 6. Interactive Commands

Once the kernel is running, press these keys in the QEMU window:

| Key | Command | What It Shows |
| --- | ------- | ------------- |
| `S` | Scheduler Stats | Context switches, ready queue, scheduling policy details |
| `I` | Interrupt Stats | IRQ counts, timer ticks, PIC info, uptime |
| `M` | Memory Stats | Physical memory map, frame allocator, heap stats |
| `T` | Task List | All tasks with PIDs, states, priorities, roles |
| `D` | DSA Info | All data structures used in the kernel |
| `H` | Help | Command reference |
| Any | Key Echo | Shows ASCII code (keyboard driver demo) |

### New: Data Structures Command (Press 'D')

Press `D` to see all data structures used in the kernel:

```
+============================================================+
|           DATA STRUCTURES USED IN NEXAKERNEL               |
+============================================================+
|                                                            |
| 1. BITMAP (kernel/memory/dsa_structures/bitmap.c)         |
|    Used by: Physical Frame Allocator                      |
|    Purpose: Track 4KB page allocation (1 bit per frame)   |
|    Ops: O(n) scan for alloc, O(1) for free                |
|                                                            |
| 2. FREE LIST (kernel/memory/dsa_structures/freelist.c)    |
|    Used by: Kernel Heap Allocator                         |
|    Purpose: Manage variable-size memory blocks            |
|    Ops: O(n) first-fit alloc, O(1) free + coalescing      |
|                                                            |
| 3. CIRCULAR QUEUE (scheduler/dsa_structures/)             |
|    Used by: Round-Robin Scheduler                         |
|    Purpose: FIFO task queue for fair scheduling           |
|    Ops: O(1) enqueue/dequeue                              |
|                                                            |
| 4. PRIORITY QUEUE/HEAP (lib/dsa/heap.c)                   |
|    Used by: Priority Scheduler (alternative)              |
|    Purpose: Always schedule highest-priority task         |
|    Ops: O(log n) insert/extract-min                       |
|                                                            |
| 5. TRIE (kernel/fs/dsa_structures/trie.c)                 |
|    Used by: Filesystem path lookup                        |
|    Purpose: Fast prefix-based path resolution             |
|    Ops: O(k) lookup where k = path length                 |
|                                                            |
| 6. HASH MAP (kernel/fs/dsa_structures/hashmap.c)          |
|    Used by: Open file table, inode cache                  |
|    Purpose: O(1) average lookup for file descriptors      |
|                                                            |
+============================================================+
```

### Heartbeat Messages

Every 10 seconds, the kernel prints a status heartbeat:

```
+---------------------- HEARTBEAT [30s] ----------------------+
| Tasks: 5 | Timer IRQs: 3000 | Context Switches: 450 |
+------------------------------------------------------------+
```

This proves:
- Timer interrupts are firing (IRQ count increases by ~1000 per 10s)
- Scheduler is running (context switch count increases)
- Tasks are alive and yielding

---

## 7. Key Talking Points

### Architecture Highlights

1. **x86 Protected Mode**: 32-bit flat memory model with GDT
2. **Multiboot Compliant**: Loads via GRUB bootloader
3. **Modular Design**: Clean separation of concerns (memory, interrupts, drivers, scheduler)
4. **Educational Focus**: Implements core OS concepts with clear, commented code

### Data Structures Used

| Component | Data Structure | Purpose |
|-----------|---------------|---------|
| Frame Allocator | Bitmap | Track 4KB physical pages |
| Heap Allocator | Free List | Dynamic memory blocks |
| Scheduler | Circular Queue | Round-robin task queue |
| Scheduler | Min-Heap | Priority queue (alternative) |
| Filesystem (future) | Trie, Tree | Directory/file lookup |

### OS Concepts Demonstrated

1. **Bootstrap Process**
   - Multiboot header recognition
   - Protected mode transition
   - GDT/IDT setup

2. **Memory Management**
   - Physical memory allocation (bitmap)
   - Dynamic heap allocation (free list)
   - Block splitting and coalescing

3. **Interrupt Handling**
   - Hardware interrupt flow
   - PIC programming
   - Interrupt service routines

4. **Device I/O**
   - Memory-mapped I/O (VGA)
   - Port-mapped I/O (keyboard, timer)
   - Interrupt-driven input

5. **Process Management**
   - Task creation and destruction
   - Context switching
   - Preemptive scheduling

---

## 8. Source Code Walkthrough

For a deeper dive, show these key code sections:

### Boot Entry Point
```c
// kernel/kernel.c - kernel_main()
void kernel_main(multiboot_info_t *multiboot_info)
{
    cpu_cli();              // Disable interrupts
    serial_init();          // Init debug output
    clear_bss();            // Zero uninitialized data
    early_console_init();   // Init display
    
    // Phase 1-4: Initialize subsystems
    init_memory(multiboot_info);
    init_interrupts();
    init_drivers();
    init_scheduler_subsystem();
    
    cpu_sti();              // Enable interrupts
    scheduler_start();      // Never returns
}
```

### Context Switch (Assembly)
```assembly
; kernel/scheduler/context_switch.asm
task_switch_asm:
    ; Save old task's registers
    pusha
    mov [eax + TASK_STACK_OFFSET], esp
    
    ; Load new task's registers
    mov esp, [ebx + TASK_STACK_OFFSET]
    popa
    ret
```

### IRQ Handler
```c
// kernel/interrupts/irq.c
void irq_handler(interrupt_frame_t *frame)
{
    uint8_t irq = frame->int_no - IRQ_BASE;
    
    irq_counts[irq]++;           // Statistics
    pic_send_eoi(irq);           // Acknowledge interrupt
    
    if (irq_handlers[irq])
        irq_handlers[irq](frame); // Call registered handler
}
```

---

## Presentation Tips

1. **Start with the big picture**: Explain what the kernel does before diving into code
2. **Use the interactive demo**: Let your professor press keys and see live responses
3. **Point out the terminal output**: It shows everything happening in real-time
4. **Highlight data structures**: Bitmap for frames, free list for heap, queue for scheduler
5. **Show the source code**: The code is well-commented and educational

### Suggested Demo Flow (10 minutes)

| Time | Activity |
|------|----------|
| 0:00 | Run `make run`, explain boot sequence |
| 2:00 | Point out memory initialization |
| 3:00 | Show interrupt setup and PIC remapping |
| 4:00 | Demonstrate keyboard input (press keys) |
| 5:00 | Show task switching (observe interleaved output) |
| 6:00 | Press `s` for scheduler stats |
| 7:00 | Press `m` for memory stats |
| 8:00 | Walk through a key source file |
| 10:00 | Q&A |

---

## Troubleshooting

### QEMU Window Shows "Guest has not initialized display"
- Wait a few seconds - the kernel is booting
- Check the terminal for serial output

### Keyboard Not Responding
- Click inside the QEMU window to give it focus
- The keyboard only works when QEMU has focus

### Build Fails
```bash
# Install 32-bit libraries
sudo apt install gcc-multilib

# Clean rebuild
make clean && make
```

---

## Summary

NexaKernel demonstrates these core OS concepts with **inline visual explanations**:

✅ **Bootloader Integration** - Multiboot protocol, protected mode transition, GDT setup  
✅ **Memory Management** - Bitmap-based frame allocator, free-list heap with coalescing  
✅ **Interrupt Handling** - IDT (256 entries), ISR/IRQ handlers, PIC remapping  
✅ **Device Drivers** - VGA text mode, PIT timer, PS/2 keyboard, serial port  
✅ **Process Scheduling** - Task Control Blocks, context switching, round-robin  
✅ **Data Structures** - Bitmap, Free List, Circular Queue, Priority Heap, Trie, HashMap  

### What Makes This Demonstration Special

1. **Visual ASCII Diagrams** - Every major concept has inline diagrams
2. **Step-by-Step Explanations** - Algorithms shown with before/after states
3. **Interactive Commands** - 6 commands to explore the running kernel
4. **Dual Output** - Both VGA and serial terminal for easy viewing
5. **Real-Time Statistics** - Watch counters increase as the kernel runs

### Quick Reference Card for Presentation

```
Commands:  S=Scheduler  I=Interrupts  M=Memory  T=Tasks  D=DSA  H=Help

Key Concepts:
- Frame Allocator: Bitmap, O(n) alloc, O(1) free
- Heap Allocator: Free list, first-fit, coalescing
- Scheduler: Round-robin, time-sliced, preemptive
- Interrupts: PIC remapped 8-15 → 32-47, EOI required
- Context Switch: pusha/popa + ESP swap in assembly
```

**Good luck with your presentation!** 🎓
