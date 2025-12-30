<div align="center">

# 🧬 NexaKernel

## A Modular, From-Scratch Operating System Kernel with Deep Data Structure Integration

**Built from the ground up. Architected for extensibility. Powered by clean systems design & real DSA fundamentals.**

</div>

---

# 🚀 Overview

**NexaKernel** is a fully modular operating system kernel implemented from scratch, designed as a fusion between low-level OS engineering and high-performance data structure applications.

This project serves two purposes:

- **OS Laboratory Project:** A complete, custom-built kernel featuring bootloader, memory manager, interrupt handling, scheduler, and userland.
- **DSA Laboratory Project:** Real-world, production-level usage of advanced data structures such as trees, heaps, tries, hash tables, circular queues, graphs, bitmaps, and more—implemented directly inside core kernel subsystems.

NexaKernel is built to be **minimal today**, with **massive future scope** for expansion into filesystems, networking, process isolation, and even AI-enhanced scheduling.

---

# 🏗️ Features

## ✔️ **Core Kernel Features**

- Custom bootloader (x86)
- Protected mode initialization
- Interrupt Descriptor Table (IDT)
- Hardware IRQ handling
- Software interrupts & syscall interface
- VGA text-mode display driver
- Programmable Interval Timer (PIT)
- PS/2 keyboard driver
- Basic kernel memory manager
- Heap allocator (`kmalloc`/`kfree`)
- Physical frame allocator (bitmap or buddy tree)
- Round-robin scheduler with context switching
- Optional priority scheduler

---

# 📚 Data Structures Used (DSA Integration)

NexaKernel is intentionally designed to showcase **real DSA usage** within operating system internals.

## 🔹 Scheduler

- **Circular Queue** – Round Robin scheduling
- **Binary Heap / Priority Queue** – Priority-based scheduling
- **Linked Lists** – Task lists and run queues

## 🔹 Memory Manager

- **Bitmap** – Physical frame allocation
- **Binary Buddy Tree** – Block allocation
- **Free List** – Heap block management

## 🔹 File System (RAM-based for now)

- **Trie** – File name indexing
- **N-ary Tree** – Directory structure
- **Hash Map** – File descriptor tables

## 🔹 IPC (Interprocess Communication)

- **Message Queues** – FIFO structures
- **Shared Memory Maps** – Hash lookup

## 🔹 Optional DSA Module Extensions

- KD-Tree for process resource mapping
- Graph-based interrupt routing (Dijkstra)
- Trie-based text search engine module

---

# 🧩 Repository Structure

A clean, extensible structure designed for future kernel modules.

```
NexaKernel/
├── README.md
├── LICENSE
├── Makefile
├── config/
├── docs/
├── boot/
├── kernel/
│   ├── scheduler/
│   ├── memory/
│   ├── fs/
│   ├── drivers/
│   ├── interrupts/
│   ├── ipc/
│   ├── utils/
│   └── modules/
├── lib/
│   ├── cstd/
│   └── dsa/
├── userland/
└── scripts/
```

Full expanded version is available in the repo.

---

# 🧱 Build Instructions

NexaKernel is designed to run on **QEMU**, making it easy to test without hardware flashing.

> **Note:** This project is designed for **Linux systems only**. Native 32-bit ELF compilation requires `gcc-multilib`.

## 🔧 Prerequisites

### Debian/Ubuntu

```bash
sudo apt update
sudo apt install build-essential gcc-multilib nasm qemu-system-x86 grub-pc-bin xorriso gdb
```

### Fedora/RHEL

```bash
sudo dnf install gcc gcc-c++ glibc-devel.i686 libgcc.i686 nasm qemu-system-x86 grub2-tools-extra xorriso gdb
```

### Arch Linux

```bash
sudo pacman -S base-devel lib32-glibc lib32-gcc-libs nasm qemu grub xorriso gdb
```

**Required tools:**

- `gcc` with 32-bit support (`gcc-multilib` / `glibc-devel.i686` / `lib32-glibc`)
- `nasm` (assembler)
- `ld` (linker from binutils)
- `qemu-system-i386` (emulator)
- `grub-mkrescue`, `xorriso` (for ISO creation)
- `gdb` (debugger, optional)

## ▶️ Build & Run

```bash
make          # Build the kernel
make run      # Run in QEMU
make iso      # Create bootable ISO image
make run-iso  # Run ISO in QEMU
```

## 🐛 Debugging with GDB

```bash
make debug    # Start QEMU with GDB server on port 1234
# In another terminal:
gdb build/kernel.elf -ex 'target remote localhost:1234'
```

## 🧹 Clean Build

```bash
make clean    # Remove all build artifacts
```

---

# 🌱 Future Features & Expansion

NexaKernel is intentionally modular so new features can be plugged in easily.

## 🔮 Planned Future Modules

- Full virtual memory with paging
- User-space process loader (ELF)
- Custom filesystem (on-disk)
- Networking stack (ARP, ICMP, TCP-lite)
- SMP support (multicore)
- AI-assisted scheduling module
- Predictive file caching layer (DSA + ML)

---

# 📄 Documentation

Detailed documentation is available inside `/docs`.
Includes:

- Kernel architecture
- DSA usage analysis
- Memory layout
- Bootloader flow
- Scheduler design
- FS design

---

# 🤝 Contributing

This is a personal academic + systems engineering project, but contributors are welcome.
Feel free to fork the repo and submit PRs for fixes, new modules, or optimizations.

---

# ⚖️ License

MIT License – free to use, modify, fork, and build upon.

---

# ⭐ Acknowledgements

Inspired by:

- Linux Kernel
- MikeOS
- xv6
- OSDev Wiki

---

# 🙌 Final Note

NexaKernel is both a learning journey and a long-term engineering platform. It starts small, but the architecture allows it to grow into:

- A full-fledged OS
- A DSA teaching tool
- A modular kernel playground
- A systems research platform

**This is the first stone in the foundation of a future systems engineer’s toolkit.**

---

### 🚀 Let's build something legendary.
