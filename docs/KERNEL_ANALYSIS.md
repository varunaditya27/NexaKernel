# NexaKernel Codebase Deep Dive & Demonstration Strategy

## 1. Codebase Overview

**NexaKernel** is an educational x86 kernel (32-bit Protected Mode) designed with a clear, modular structure. It emphasizes transparency by printing detailed status messages during its initialization phases.

### Key Components

*   **Bootloader (`boot/`)**: Uses Multiboot specification (likely GRUB compatible). Sets up the GDT and transfers control to `kernel_main`.
*   **Kernel Core (`kernel/`)**:
    *   **Initialization (`kernel.c`)**: A phased initialization process (Memory -> Interrupts -> Drivers -> Scheduler). It is verbose, printing ASCII art and status tables, which is excellent for demonstration.
    *   **Memory Management (`kernel/memory/`)**: Implements a Physical Frame Allocator (Bitmap based) and a Kernel Heap (Free List). It includes visualization functions (printing the bitmap).
    *   **Scheduling (`kernel/scheduler/`)**: Round-Robin scheduler with preemption. It currently runs demo tasks (Producer/Consumer/HighPriority) before dropping into the shell.
    *   **Interrupts (`kernel/interrupts/`)**: Standard IDT/ISR/IRQ setup with PIC remapping.
    *   **Drivers (`kernel/drivers/`)**:
        *   **VGA Text**: Standard 80x25 driver with **History Buffer** and Scrolling.
        *   **Keyboard**: PS/2 keyboard driver with buffer.
        *   **Serial**: For debugging output.
*   **Userland (`userland/`)**:
    *   **Shell (`shell/main.c`)**: A basic interactive shell running as PID 1 (after kernel init). Supports commands: `help`, `ps`, `mem`, `echo`, `clear`, etc.
    *   **Lib**: Basic wrappers for system calls (`syscall_wrappers.c`).

## 2. Current Demonstrability & Transparency Features

The kernel already has several features designed for demonstration:

1.  **Verbose Boot Sequence**: The `kernel_main` function prints clear "Phase" banners and status reports.
2.  **Visual Memory Tests**: `init_memory` prints a visual representation of the Frame Bitmap and Heap Free List.
3.  **Scheduler Demos**: Hardcoded demo tasks (A, B, C) run briefly to show context switching via text logs ("State: RUNNING -> READY").
4.  **Interactive Shell**: The shell provides a "hands-on" feel, proving the system is responsive.

## 3. Findings & Limitations

While good for a start, there are limitations to its "Demonstrability":

*   **(SOLVED) Transient Information**: ~The boot messages... scroll off...~ **Fixed:** Implemented a 1000-line circular history buffer with scroll support (Up/Down keys).
*   **Text-Only Linear Output**: Everything fights for the same 80x25 text buffer. Kernel logs, task output, and shell input all interleave, which can look messy.
*   **Limited Shell Commands**: The `ps` and `mem` commands in the shell are partially hardcoded or lack "live" depth.
*   **Opaque Scheduler**: Once the shell starts, the scheduler is working in the background, but invisible. You can't "see" the multitasking happening anymore.

## 4. Recommendations for "Transparent & Demonstrable" Kernel

To align with the "Baby Steps" emulator strategy (Dual-Window: QEMU + GDB), we should implement the following "Dual-View" strategy:

### A. The "Monitor" (QEMU Window): Split-Screen HUD
While Window B (GDB) handles the *micro-level* details (registers, assembly), Window A (The Kernel) should show the *macro-level* state.
Instead of a single scrolling log, we will divide the 80x25 VGA screen into regions:
*   **Top Bar (Lines 0-2)**: System Status (PIDs running, Memory Free, Uptime).
*   **Main Workspace (Lines 3-20)**: The Shell / Command output.
*   **Bottom Log (Lines 21-24)**: Rolling kernel debug log (interrupts, context switches).

*Why*: This ensures Window A looks like a "Control Console" matching the "Cyberpunk" aesthetic of your GDB dashboard.

### B. Visual System Calls (The "Matrix" Effect)
Modify `syscall_handler` to briefly flash a specific character or color on the specific status line whenever a syscall occurs.
*   *Why*: When you demo the shell, the audience will see "blips" of activity, visualizing the Userland -> Kernel conversation.

### C. GDB-Ready Data Structures
Ensure our critical data structures (like `current_task` or `free_list_head`) are globally visible symbols so they appear beautifully in the GDB Dashboard's "Memory" or "Watch" panels.

### D. Advanced Shell Commands
1.  `spawn <name>`: Create a dummy background process to populate the "Top Bar" process counter.
2.  `crash`: Deliberately trigger a fault to demonstrate the GDB "Interrupt" workflow.
3.  `map`: A command that clears the screen and draws the *entire* physical memory map (Used vs Free frames).

## 5. Implementation Roadmap

1.  **Modify `vga_text.c`**: Add support for "regions" or "viewports" so `printf` can be directed to the shell area or the log area.
2.  **Update `syscall.c`**: Add hooks to update the "Top Bar" or "Bottom Log" on every syscall.
3.  **Upgrade Shell**: Implement `spawn` and `crash`.

This approach ensures Window A (Kernel) acts as the perfect high-level companion to Window B (Debugger).
