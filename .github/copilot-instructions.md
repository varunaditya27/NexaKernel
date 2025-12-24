# Copilot instructions for NexaKernel

These are concise, actionable guidelines to help AI agents (and maintainers) be productive in the NexaKernel repository.

## 1. Big Picture Architecture

NexaKernel is an educational x86 protected mode OS kernel with DSA-driven subsystems.

**Boot Sequence:**
1. GRUB loads kernel via Multiboot → `boot/multiboot_header.asm`
2. Entry point `_start` in `boot/bootloader.asm` sets up GDT, stack
3. Calls `kernel_main()` in `kernel/kernel.c`
4. Memory management initialization (frame allocator, heap)
5. Interrupt handling (IDT, PIC, ISRs, IRQs)
6. Device drivers (VGA, timer, keyboard)

**Key Entry Points:**
- `boot/bootloader.asm` - Protected mode entry, GDT loading, jumps to C
- `boot/gdt.asm` - Global Descriptor Table (flat memory model)
- `kernel/kernel.c` - Main kernel initialization
- `config/memory_layout.ld` - Linker script (kernel loads at 0x100000)
- `config/os_config.h` - Compile-time configuration

**Subsystems:** `kernel/{scheduler,memory,fs,drivers,interrupts,ipc,utils}/`

## 2. Build & Debug (Native Linux)

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install build-essential gcc-multilib nasm qemu-system-x86 grub-pc-bin xorriso gdb

# Build and run
make              # Build kernel.bin and build/kernel.elf
make run          # Run in QEMU
make debug        # QEMU with GDB server on port 1234
make iso          # Create bootable ISO
make clean        # Remove build artifacts
```

**Requirements:** `gcc` with `gcc-multilib`, `nasm`, `ld`, `qemu-system-i386`

## 3. Memory Management

**Physical Frame Allocator** (`kernel/memory/frame_allocator.c`):
- Uses bitmap DSA to track 4KB physical frames
- `frame_alloc()` - allocate single frame, returns physical address
- `frame_free()` - free a frame
- `frame_alloc_contiguous()` - allocate N contiguous frames
- `frame_reserve()` - mark region as reserved (kernel, hardware)

**Kernel Heap** (`kernel/memory/heap_allocator.c`):
- Uses free list DSA with first-fit allocation
- `kmalloc(size)` - allocate memory
- `kfree(ptr)` - free memory
- `krealloc(ptr, size)` - resize allocation
- `kcalloc(count, size)` - allocate zeroed memory
- Block splitting and coalescing for fragmentation reduction

**DSA Structures** (`kernel/memory/dsa_structures/`):
- `bitmap.c` - Physical frame tracking
- `freelist.c` - Alternative heap implementation
- `buddy_tree.c` - Buddy system allocator (power-of-2 blocks)

**Memory API** (`kernel/memory/memory.h`):
- Convenience macros: `KMALLOC(type)`, `KCALLOC(type)`, `KMALLOC_ARRAY(type, n)`

## 4. Interrupt Handling

**Interrupt Descriptor Table** (`kernel/interrupts/idt.c`):
- 256 IDT entries for all x86 interrupts
- `idt_init()` - Initialize and load IDT
- `idt_set_gate()` - Set individual IDT entry

**CPU Exceptions (ISR)** (`kernel/interrupts/isr.c`):
- Handles vectors 0-31 (divide error, page fault, GPF, etc.)
- `isr_init()` - Install exception handlers
- `isr_register_handler()` - Register custom exception handler
- Default handler displays register dump and halts

**Hardware Interrupts (IRQ)** (`kernel/interrupts/irq.c`):
- Handles vectors 32-47 (remapped PIC)
- `irq_init()` - Initialize PIC and IRQ handlers
- `irq_register_handler()` - Register device IRQ handler
- `irq_enable()`/`irq_disable()` - Control IRQ masking
- Spurious IRQ detection (IRQ7/IRQ15)

**PIC Driver** (in `kernel/interrupts/irq.c`):
- `pic_init()` - Remap 8259 PIC (IRQ0-7→32-39, IRQ8-15→40-47)
- `pic_send_eoi()` - Send End-Of-Interrupt

**Interrupt API** (`kernel/interrupts/interrupts.h`):
- `interrupts_enable()`/`interrupts_disable()` - Global interrupt control
- `interrupts_save_and_disable()` - Critical section helpers

## 5. Task Scheduling

**Task Control Block** (`kernel/scheduler/task.h`, `task.c`):
- Task states: UNUSED, CREATING, READY, RUNNING, BLOCKED, SLEEPING, TERMINATED, ZOMBIE
- Priority levels: 0 (highest) to 7 (lowest, idle)
- `task_system_init()` - Initialize task subsystem
- `task_create(name, entry, arg, priority, stack_size)` - Create new task
- `task_exit(code)` - Terminate current task
- `task_current()` - Get currently running task
- `task_yield()` - Voluntarily give up CPU
- `task_sleep(ticks)` - Sleep for specified timer ticks

**Scheduler** (`kernel/scheduler/scheduler.h`, `scheduler.c`):
- Supports Round-Robin and Priority-based scheduling policies
- `scheduler_init()` - Initialize scheduler and create idle task
- `scheduler_start()` - Begin multitasking (does not return)
- `schedule()` - Trigger reschedule (pick next task)
- `scheduler_add_task(task)` - Add task to ready queue
- `scheduler_remove_task(task)` - Remove task from queues
- `scheduler_get_policy()` - Get current scheduling policy
- `scheduler_set_policy(policy)` - Set scheduling policy

**Context Switch** (`kernel/scheduler/context_switch.asm`):
- `context_switch(old_sp_ptr, new_sp)` - Switch between tasks
- `switch_to_task(task)` - Switch to a task (first time)
- `task_switch_asm(old_task, new_task)` - Full context switch with C integration
- Saves/restores: EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP

**Scheduler DSA** (`kernel/scheduler/dsa_structures/`):
- `round_robin_queue.c` - Circular queue (FIFO) for round-robin scheduling
- `priority_queue.c` - Min-heap for priority-based scheduling
- Operations: enqueue, dequeue, peek, remove, count

**Configuration** (`config/os_config.h`):
- `SCHEDULER_PREEMPTIVE` - 1 for preemptive, 0 for cooperative
- `SCHEDULER_TICK_HZ` - Timer frequency (default 100 Hz)
- `SCHEDULER_TIME_SLICE` - Ticks per time slice (default 10)
- `MAX_TASKS` - Maximum concurrent tasks (default 64)
- `MAX_PRIORITY_LEVELS` - Priority levels (default 8)

## 6. Device Drivers

**VGA Text Mode** (`kernel/drivers/vga_text.c`):
- Direct 0xB8000 buffer access, 80x25 text
- `vga_init()`, `vga_clear()`, `vga_putchar()`, `vga_write_string()`
- Color support: `vga_set_color()`, `VGA_ENTRY_COLOR()` macro

**Timer (PIT)** (`kernel/drivers/timer.c`):
- 8254 PIT Channel 0 for system tick (default 100 Hz)
- `pit_init()` - Initialize timer and register IRQ0 handler
- `pit_get_ticks()` - Get tick count since boot
- `pit_sleep_ms()` - Busy-wait delay
- `pit_register_callback()` - Hook for scheduler

**Keyboard** (`kernel/drivers/keyboard.c`):
- PS/2 keyboard via IRQ1
- `keyboard_init()` - Initialize driver
- `keyboard_getchar()` - Non-blocking character read
- `keyboard_getchar_blocking()` - Blocking read
- Scancode Set 1, shift/ctrl/alt/caps lock support

**Driver API** (`kernel/drivers/drivers.h`):
- Complete API for VGA, timer, keyboard

## 7. Coding Conventions

- **Memory:** Use `kmalloc()`/`kfree()` for kernel heap (not available in early boot)
- **I/O Ports:** Use `inb()`, `outb()`, etc. from `boot/startup.asm`
- **Interrupts:** Use `interrupts_save_and_disable()`/`interrupts_restore()` for critical sections
- **Panic:** Use `PANIC("message")` macro for fatal errors
- **ASM↔C:** Assembly functions are `global`, C externs use plain C linkage
- **Types:** Use `uint32_t`, `size_t`, `uintptr_t` from `os_config.h`

## 8. Critical Cross-Component Notes

- **Boot code:** NO dynamic memory. Stack and GDT are statically allocated.
- **GDT segments:** Code=0x08, Data=0x10 (see `boot/gdt.asm`)
- **Multiboot info:** Passed to `kernel_main()`, contains memory map
- **VGA buffer:** Direct access at 0xB8000 for early console output
- **Frame allocator:** Must be initialized before heap
- **Heap:** Allocates contiguous frames from frame allocator at init
- **IDT:** Must be initialized before enabling interrupts
- **PIC:** Remapped to avoid conflicts with CPU exceptions (vectors 0-31)
- **Interrupts:** Disabled during early boot, enabled after all init complete
- **Scheduler:** Must be initialized after heap (needs kmalloc for task stacks)
- **Context switch:** Saves/restores registers via PUSHA/POPA, uses task->stack_pointer
- **Timer callback:** Drives preemptive scheduling via `pit_register_callback()`
- **Idle task:** Created automatically by scheduler, runs at lowest priority

## 9. Library Organization

**Generic DSA Library** (`lib/dsa/`):
- Reusable data structure implementations (bitmap, list, queue, heap, tree, trie, hashmap)
- Used by kernel subsystems via wrappers in `kernel/*/dsa_structures/`

**C Standard Library** (`lib/cstd/`):
- Freestanding implementations of C library functions
- `string.h/c` - String manipulation (strlen, strcpy, strcmp, etc.)
- `memory.h/c` - Memory operations (memcpy, memset, memmove, memcmp)
- `stdio.h/c` - Kernel printf (kprintf, kvprintf)

**Kernel Utilities** (`kernel/utils/`):
- `logging.h/c` - Kernel logging facility (log_info, log_error, log_debug)
- `test_dsa.h/c` - DSA verification test suite

**DSA Wrapper Pattern:**
- Generic DSA in `lib/dsa/` → Module-specific wrapper in `kernel/*/dsa_structures/`
- Example: `lib/dsa/bitmap.h` → `kernel/memory/dsa_structures/bitmap.c` (frame allocator)
- Example: `lib/dsa/hashmap.h` → `kernel/fs/dsa_structures/hashmap.c` (open file table)

## 10. Key Files Reference

| File | Purpose |
|------|---------|
| `boot/multiboot_header.asm` | GRUB multiboot magic/flags |
| `boot/gdt.asm` | GDT with null, code, data segments |
| `boot/bootloader.asm` | Entry point, stack setup, calls kernel |
| `boot/startup.asm` | CPU utilities (halt, I/O, interrupts) |
| `kernel/kernel.c` | Kernel entry, initialization sequence |
| `kernel/panic.c` | Kernel panic with VGA output |
| `kernel/memory/memory.h` | Memory management API |
| `kernel/memory/frame_allocator.c` | Physical frame allocation |
| `kernel/memory/heap_allocator.c` | kmalloc/kfree implementation |
| `kernel/interrupts/interrupts.h` | Interrupt handling API |
| `kernel/interrupts/idt.c` | IDT initialization |
| `kernel/interrupts/isr.c` | CPU exception handlers |
| `kernel/interrupts/irq.c` | Hardware IRQ handlers, PIC driver |
| `kernel/interrupts/isr_stubs.asm` | Low-level interrupt entry points |
| `kernel/drivers/drivers.h` | Device driver API |
| `kernel/drivers/vga_text.c` | VGA text mode console |
| `kernel/drivers/timer.c` | PIT timer driver |
| `kernel/drivers/keyboard.c` | PS/2 keyboard driver |
| `kernel/scheduler/task.h` | Task control block and task API |
| `kernel/scheduler/task.c` | Task management implementation |
| `kernel/scheduler/scheduler.h` | Scheduler public API |
| `kernel/scheduler/scheduler.c` | Scheduler core logic |
| `kernel/scheduler/context_switch.asm` | Low-level context switch |
| `kernel/scheduler/dsa_structures.h` | Scheduler queue interfaces |
| `kernel/scheduler/dsa_structures/round_robin_queue.c` | Round-robin FIFO queue |
| `kernel/scheduler/dsa_structures/priority_queue.c` | Priority min-heap |
| `lib/dsa/bitmap.c` | Bitmap data structure |
| `lib/dsa/list.c` | Intrusive linked list |
| `lib/cstd/string.c` | String functions (strlen, strcpy, etc.) |
| `lib/cstd/memory.c` | Memory functions (memcpy, memset, etc.) |
| `lib/cstd/stdio.c` | Kernel printf (kprintf) |
| `kernel/utils/logging.c` | Kernel logging facility |
| `config/os_config.h` | Types, macros, configuration |

## 11. Do NOT

- Add dynamic allocation in boot code (heap not ready)
- Use libc functions in kernel (use `lib/cstd/` or implement your own)
- Forget to disable interrupts during critical sections
- Assume segment registers are set (reload after GDT changes)
- Free memory not allocated by kmalloc (undefined behavior)
- Use kmalloc before `heap_init()` completes
- Enable interrupts before IDT is loaded
- Duplicate implementations (use lib/dsa and lib/cstd as base)

