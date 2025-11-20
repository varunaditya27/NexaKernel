# 📁 NexaKernel Directory Structure — Final Clean Version

This is the **streamlined, necessary-only** directory layout for NexaKernel.
Unnecessary or future-overkill components are removed.
Everything listed here is essential for your semester kernel + DSA integration project.

This structure keeps things modular, simple, and extremely easy for 2 developers to work on.

---

# 🏛️ Root Level

```
NexaKernel/
├── README.md
├── Makefile
├── LICENSE
├── config/
├── docs/
├── boot/
├── kernel/
├── lib/
├── userland/
└── scripts/
```

---

# ⚙️ `config/` — Build & Memory Configuration

```
config/
├── os_config.h
├── memory_layout.ld
└── build_flags.mk
```

### `os_config.h`

Global compile-time settings (debug flags, sizes, scheduling mode).

### `memory_layout.ld`

Linker script: defines kernel load address and section layout.

### `build_flags.mk`

Compiler and linker flags used by Makefile.

---

# 📚 `docs/` — Reports & Diagrams

```
docs/
├── OS_Report.pdf
├── DSA_Report.pdf
└── architecture_diagram.png
```

* Academic documentation.
* Architecture diagram for demo and report.

---

# 🚀 `boot/` — Bootloader + CPU Setup

```
boot/
├── multiboot_header.asm
├── bootloader.asm
├── gdt.asm
└── startup.asm
```

### `multiboot_header.asm`

GRUB-compatible header.

### `bootloader.asm`

Loads kernel, switches to protected mode.

### `gdt.asm`

Defines the Global Descriptor Table.

### `startup.asm`

Jumps into `kernel_main()` (C entry point).

---

# 🧠 `kernel/` — Core OS Logic

```
kernel/
├── kernel.c
├── panic.c
├── syscall.c
├── scheduler/
├── memory/
├── fs/
├── drivers/
├── interrupts/
├── ipc/
└── utils/
```

## `kernel.c`

Kernel entry. Initializes IDT, memory, scheduler, drivers.

## `panic.c`

Handles unrecoverable errors.

## `syscall.c`

System call table + dispatcher.

---

# ⚙️ `kernel/scheduler/`

```
scheduler/
├── scheduler.c
├── task.c
├── context_switch.asm
└── dsa_structures/
```

### `scheduler.c`

Implements scheduling policy (round robin / priority).

### `task.c`

Task creation, management, task control blocks.

### `context_switch.asm`

Assembly-level register switching.

### `dsa_structures/`

* `round_robin_queue.c`
* `priority_queue.c`
* `heap.c`

Used for task scheduling.

---

# 🧱 `kernel/memory/`

```
memory/
├── frame_allocator.c
├── heap_allocator.c
└── dsa_structures/
```

### `frame_allocator.c`

Physical memory manager (bitmap/buddy).

### `heap_allocator.c`

Kernel heap using free list.

### `dsa_structures/`

* `bitmap.c` — physical memory map
* `buddy_tree.c` — buddy system allocator
* `freelist.c` — heap block list

---

# 📁 `kernel/fs/` — Simple RAM Filesystem

```
fs/
├── vfs.c
├── ramfs.c
└── dsa_structures/
```

### `vfs.c`

Virtual File System layer.

### `ramfs.c`

Simple in-memory FS (directories + files).

### `dsa_structures/`

* `trie.c` — file name indexing
* `directory_tree.c` — folder structure
* `hashmap.c` — open file table

---

# 🖥️ `kernel/drivers/`

```
drivers/
├── vga_text.c
├── keyboard.c
└── timer.c
```

### `vga_text.c`

Low-level VGA 80x25 text output.

### `keyboard.c`

PS/2 keyboard input.

### `timer.c`

Sets up PIT timer for scheduling.

---

# ⚡ `kernel/interrupts/`

```
interrupts/
├── idt.c
├── isr.c
└── irq.c
```

### `idt.c`

Creates IDT entries.

### `isr.c`

Software interrupt handlers.

### `irq.c`

Hardware IRQ handlers.

---

# 🔄 `kernel/ipc/`

```
ipc/
├── message_queue.c
└── shared_memory.c
```

### `message_queue.c`

FIFO-based process communication.

### `shared_memory.c`

Memory-mapped shared segments.

---

# 🧰 `kernel/utils/`

```
utils/
├── string.c
├── stdio.c
└── logging.c
```

Utility library shared throughout the kernel.

---

# 📦 `lib/` — Reusable Libraries

```
lib/
├── cstd/
└── dsa/
```

### `cstd/`

Minimal C stdlib for kernel:

* `string.c`
* `stdio.c`
* `memory.c`

### `dsa/`

Reusable data structures used across subsystems:

* lists
* heaps
* hash maps
* tries
* trees
* graphs

---

# 👤 `userland/` — Simple User Programs

```
userland/
├── shell/
├── programs/
└── lib/
```

### `shell/`

Basic user shell (command parsing, printing).

### `programs/`

Test programs for FS, memory, scheduler.

### `lib/`

System call wrappers + crt0.

---

# 🔧 `scripts/`

```
scripts/
├── build.sh
├── run_qemu.sh
└── debug_gdb.sh
```

Automation scripts for building, running, debugging.

---

# ✔️ Final Thoughts

This structure contains **only what you need** to build:

* a clean kernel
* with real DSA integration
* modular subsystems
* easy scalability
