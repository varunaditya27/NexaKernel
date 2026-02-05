# 📚 NexaKernel Data Structures Study Guide

## A Comprehensive Reference for Understanding Data Structures in Operating System Kernels

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Overview: DSA-Kernel Subsystem Mapping](#2-overview-dsa-kernel-subsystem-mapping)
3. [Memory Management Data Structures](#3-memory-management-data-structures)
   - [3.1 Bitmap (Physical Frame Allocator)](#31-bitmap-physical-frame-allocator)
   - [3.2 Free List (Heap Allocator)](#32-free-list-heap-allocator)
   - [3.3 Buddy System (Power-of-Two Allocator)](#33-buddy-system-power-of-two-allocator)
4. [Process Scheduling Data Structures](#4-process-scheduling-data-structures)
   - [4.1 Circular Queue (Round-Robin Scheduler)](#41-circular-queue-round-robin-scheduler)
   - [4.2 Binary Heap / Priority Queue (Priority Scheduler)](#42-binary-heap--priority-queue-priority-scheduler)
5. [File System Data Structures](#5-file-system-data-structures)
   - [5.1 N-ary Tree (Directory Hierarchy)](#51-n-ary-tree-directory-hierarchy)
   - [5.2 Hash Map (Open File Table)](#52-hash-map-open-file-table)
   - [5.3 Trie (File Indexing)](#53-trie-file-indexing)
6. [Inter-Process Communication Data Structures](#6-inter-process-communication-data-structures)
   - [6.1 Circular Buffer (Message Queues)](#61-circular-buffer-message-queues)
7. [Foundational Data Structures](#7-foundational-data-structures)
   - [7.1 Intrusive Doubly Linked List](#71-intrusive-doubly-linked-list)
   - [7.2 Generic Binary Tree Helpers](#72-generic-binary-tree-helpers)
8. [Time Complexity Summary](#8-time-complexity-summary)
9. [Viva Questions & Answers](#9-viva-questions--answers)
10. [Quick Reference Cheat Sheet](#10-quick-reference-cheat-sheet)
11. [Glossary](#11-glossary)

---

## 1. Introduction

### What is NexaKernel?

NexaKernel is an educational x86 protected-mode operating system kernel designed to demonstrate how **data structures** form the backbone of operating system functionality. Every major kernel subsystem—memory management, process scheduling, file systems, and inter-process communication—relies on carefully chosen data structures to achieve efficiency, correctness, and scalability.

### Why Study DSA in OS Context?

Understanding data structures in the context of operating systems provides:

1. **Real-world Application**: See how theoretical DSA concepts solve actual system problems
2. **Performance Insight**: Understand why certain operations must be O(1) or O(log n)
3. **Design Trade-offs**: Learn to balance memory usage, speed, and complexity
4. **Interview Preparation**: OS + DSA is a common interview topic for systems roles

### Document Organization

This guide is organized by **kernel subsystem**, with each section covering:
- **What**: The data structure and its properties
- **Why**: Rationale for choosing this structure
- **How**: Operations performed during kernel execution
- **When**: Specific scenarios that trigger operations
- **Code**: Key implementation details from NexaKernel

---

## 2. Overview: DSA-Kernel Subsystem Mapping

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        NexaKernel DSA Architecture                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────┐    ┌─────────────────────┐    ┌────────────────┐  │
│  │   MEMORY MANAGER    │    │  PROCESS SCHEDULER  │    │  FILE SYSTEM   │  │
│  ├─────────────────────┤    ├─────────────────────┤    ├────────────────┤  │
│  │ • Bitmap            │    │ • Circular Queue    │    │ • N-ary Tree   │  │
│  │   (Frame Allocator) │    │   (Round-Robin)     │    │   (Dir Tree)   │  │
│  │                     │    │                     │    │                │  │
│  │ • Free List         │    │ • Binary Heap       │    │ • Hash Map     │  │
│  │   (Heap Allocator)  │    │   (Priority Sched)  │    │   (Open Files) │  │
│  │                     │    │                     │    │                │  │
│  │ • Buddy System      │    │                     │    │ • Trie         │  │
│  │   (Block Allocator) │    │                     │    │   (File Index) │  │
│  └─────────────────────┘    └─────────────────────┘    └────────────────┘  │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      IPC (Inter-Process Communication)               │   │
│  ├─────────────────────────────────────────────────────────────────────┤   │
│  │ • Circular Buffer (Message Queues)                                   │   │
│  │ • Reference Counting (Shared Memory)                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        FOUNDATION LAYER                              │   │
│  ├─────────────────────────────────────────────────────────────────────┤   │
│  │ • Intrusive Doubly Linked List (Used by Free List, Buddy, etc.)      │   │
│  │ • Binary Tree Index Helpers (Used by Heap, Buddy Tree)               │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Quick Reference Table

| Kernel Subsystem | Data Structure | Purpose | Key Operations | Time Complexity |
|------------------|----------------|---------|----------------|-----------------|
| Frame Allocator | Bitmap | Track physical memory frames | set, clear, find_first_zero | O(1), O(1), O(n/8) |
| Heap Allocator | Free List | Dynamic kernel memory | alloc, free, coalesce | O(n), O(1), O(1) |
| Buddy Allocator | Binary Tree + Bitmap | Power-of-2 blocks | alloc, free, merge | O(log n) |
| Round-Robin Scheduler | Circular Queue | FIFO task scheduling | enqueue, dequeue | O(1) |
| Priority Scheduler | Min-Heap | Priority-based scheduling | insert, extract_min | O(log n) |
| Directory Tree | N-ary Tree | Hierarchical directories | add_child, find | O(children) |
| Open File Table | Hash Map | File descriptor lookup | put, get, remove | O(1) average |
| File Index | Trie | Prefix-based file search | insert, search | O(key length) |
| Message Queue | Circular Buffer | IPC message passing | send, receive | O(1) |

---

## 3. Memory Management Data Structures

Memory management is the most critical kernel subsystem. It manages physical RAM and provides dynamic allocation services to the rest of the kernel.

### 3.1 Bitmap (Physical Frame Allocator)

#### 3.1.1 Concept Overview

A **bitmap** (also called a **bit array** or **bit vector**) is a space-efficient data structure that uses individual bits to represent boolean states. In memory management, each bit represents one physical memory frame:

- **Bit = 0**: Frame is FREE (available for allocation)
- **Bit = 1**: Frame is ALLOCATED (in use)

```
Physical Memory (simplified):
┌────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┐
│Frame 0 │Frame 1 │Frame 2 │Frame 3 │Frame 4 │Frame 5 │Frame 6 │Frame 7 │
│ 4KB    │ 4KB    │ 4KB    │ 4KB    │ 4KB    │ 4KB    │ 4KB    │ 4KB    │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘
    ↓        ↓        ↓        ↓        ↓        ↓        ↓        ↓
Bitmap:
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 1 │ 1 │ 0 │ 1 │ 0 │ 0 │ 0 │ 1 │  ← 1 byte tracks 8 frames
└───┴───┴───┴───┴───┴───┴───┴───┘
 ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑
Used Used Free Used Free Free Free Used
```

#### 3.1.2 Why Bitmap for Frame Allocation?

| Requirement | Why Bitmap Satisfies It |
|-------------|------------------------|
| **Space Efficiency** | N frames need only N/8 bytes (vs. N×sizeof(pointer) for a linked list) |
| **O(1) Status Check** | Testing if a frame is free is a single bit operation |
| **O(1) Allocation/Free** | Setting/clearing a bit is constant time |
| **Cache Friendly** | Contiguous memory layout for sequential scanning |
| **No Dynamic Memory** | Can be statically allocated at compile time |

**Space Calculation Example:**
- For 256 MB of RAM with 4 KB pages:
- Total frames = 256 MB / 4 KB = 65,536 frames
- Bitmap size = 65,536 / 8 = 8,192 bytes = **8 KB**

#### 3.1.3 Data Structure Definition

```c
/* From lib/dsa/bitmap.h */
typedef struct bitmap {
    uint8_t *buffer;      /* Pointer to the bit storage array */
    size_t size_bits;     /* Total number of bits being tracked */
    size_t size_bytes;    /* Size of buffer in bytes = ceil(size_bits/8) */
} bitmap_t;
```

#### 3.1.4 Operations & Implementation

##### Operation 1: Initialize Bitmap

```c
bool bitmap_init(bitmap_t *bitmap, size_t size_bits, void *buffer)
{
    if (!bitmap || !buffer || size_bits == 0) {
        return false;
    }
    
    bitmap->buffer = (uint8_t *)buffer;
    bitmap->size_bits = size_bits;
    bitmap->size_bytes = (size_bits + 7) / 8;  /* Ceiling division */
    
    /* Clear all bits to zero (all frames free) */
    memset(bitmap->buffer, 0, bitmap->size_bytes);
    
    return true;
}
```

**When Called:** During kernel boot, after memory size is determined from multiboot info.

##### Operation 2: Set Bit (Mark Frame as Allocated)

```c
void bitmap_set(bitmap_t *bitmap, size_t index)
{
    if (!bitmap || index >= bitmap->size_bits) {
        return;
    }
    
    /* Set bit at position (index % 8) in byte (index / 8) */
    bitmap->buffer[index / 8] |= (1 << (index % 8));
}
```

**Bit Manipulation Explanation:**
```
index = 13
byte_index = 13 / 8 = 1
bit_position = 13 % 8 = 5

Before: byte[1] = 0b00000000
Mask:             0b00100000  ← (1 << 5)
After:  byte[1] = 0b00100000  (bit 5 is now set)
```

**When Called:**
- When allocating a frame for page tables
- When allocating a frame for kernel data
- When reserving frames for hardware (DMA, VGA buffer)

##### Operation 3: Clear Bit (Mark Frame as Free)

```c
void bitmap_clear(bitmap_t *bitmap, size_t index)
{
    if (!bitmap || index >= bitmap->size_bits) {
        return;
    }
    
    /* Clear bit using AND with inverted mask */
    bitmap->buffer[index / 8] &= ~(1 << (index % 8));
}
```

**Bit Manipulation Explanation:**
```
index = 13
Before: byte[1] = 0b00100000
Mask:             0b00100000  ← (1 << 5)
~Mask:            0b11011111  ← inverted
After:  byte[1] = 0b00000000  (bit 5 is now clear)
```

**When Called:**
- When freeing a page that was unmapped
- When a process terminates and its pages are released

##### Operation 4: Test Bit (Check if Frame is Allocated)

```c
bool bitmap_test(const bitmap_t *bitmap, size_t index)
{
    if (!bitmap || index >= bitmap->size_bits) {
        return false;
    }
    
    return (bitmap->buffer[index / 8] & (1 << (index % 8))) != 0;
}
```

**When Called:**
- Before allocating a specific frame (e.g., for DMA)
- When validating memory operations

##### Operation 5: Find First Zero (Find Free Frame)

```c
int64_t bitmap_find_first_zero(const bitmap_t *bitmap)
{
    if (!bitmap) return -1;
    
    for (size_t byte_idx = 0; byte_idx < bitmap->size_bytes; byte_idx++) {
        /* Skip fully allocated bytes (0xFF) */
        if (bitmap->buffer[byte_idx] != 0xFF) {
            /* Found a byte with at least one free bit */
            for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
                if (!(bitmap->buffer[byte_idx] & (1 << bit_idx))) {
                    size_t global_index = (byte_idx * 8) + bit_idx;
                    if (global_index < bitmap->size_bits) {
                        return (int64_t)global_index;
                    }
                }
            }
        }
    }
    
    return -1;  /* All frames allocated */
}
```

**Optimization:** By checking entire bytes against 0xFF, we can skip 8 bits at a time.

**When Called:**
- Every time `frame_alloc()` is called
- When the kernel needs a new physical page

##### Operation 6: Find Contiguous Zeros (Allocate Multiple Frames)

```c
int64_t bitmap_find_contiguous_zeros(const bitmap_t *bitmap, size_t count)
{
    size_t consecutive = 0;
    size_t start = 0;
    
    for (size_t i = 0; i < bitmap->size_bits; i++) {
        if (!bitmap_test(bitmap, i)) {
            if (consecutive == 0) start = i;
            consecutive++;
            if (consecutive == count) {
                return (int64_t)start;
            }
        } else {
            consecutive = 0;
        }
    }
    
    return -1;
}
```

**When Called:**
- For DMA buffers requiring physically contiguous memory
- For large kernel allocations (e.g., kernel stacks)

#### 3.1.5 Frame Allocator Integration

```c
/* From kernel/memory/frame_allocator.c */

uintptr_t frame_alloc(void)
{
    int64_t frame_idx = bitmap_find_first_zero(&frame_bitmap);
    
    if (frame_idx < 0) {
        return 0;  /* Out of memory */
    }
    
    bitmap_set(&frame_bitmap, (size_t)frame_idx);
    used_frames++;
    
    /* Convert frame index to physical address */
    return memory_base + ((uintptr_t)frame_idx * PAGE_SIZE);
}

void frame_free(uintptr_t addr)
{
    /* Convert physical address to frame index */
    size_t frame_idx = (addr - memory_base) / PAGE_SIZE;
    
    bitmap_clear(&frame_bitmap, frame_idx);
    used_frames--;
}
```

#### 3.1.6 Time & Space Complexity

| Operation | Time Complexity | Space Complexity |
|-----------|-----------------|------------------|
| `bitmap_init` | O(n/8) | O(n/8) |
| `bitmap_set` | O(1) | O(1) |
| `bitmap_clear` | O(1) | O(1) |
| `bitmap_test` | O(1) | O(1) |
| `bitmap_find_first_zero` | O(n/8) worst case | O(1) |
| `bitmap_find_contiguous_zeros` | O(n) worst case | O(1) |

Where n = number of bits (frames)

---

### 3.2 Free List (Heap Allocator)

#### 3.2.1 Concept Overview

A **free list** is a linked list of free memory blocks used to manage a heap. Unlike the bitmap (which tracks fixed-size frames), the free list handles **variable-size allocations**.

```
Heap Memory Layout:
┌────────────────────────────────────────────────────────────────────────────┐
│ [Header][  Allocated  ][Header][   FREE   ][Header][Allocated][Header][FREE]│
└────────────────────────────────────────────────────────────────────────────┘
                              │                                     │
                              └───────── Free List ─────────────────┘

Free List Structure:
    ┌──────────────┐      ┌──────────────┐
    │ Block A      │ ──→  │ Block B      │ ──→ NULL
    │ size: 256    │      │ size: 128    │
    │ is_free: true│      │ is_free: true│
    └──────────────┘      └──────────────┘
```

#### 3.2.2 Why Free List for Heap Management?

| Requirement | Why Free List Satisfies It |
|-------------|---------------------------|
| **Variable Sizes** | Can allocate any size (within limits) |
| **Coalescing** | Adjacent free blocks can merge |
| **No External Memory** | Block headers stored in-place |
| **Flexibility** | Supports multiple allocation strategies |

#### 3.2.3 Data Structure Definition

```c
/* From kernel/memory/heap_allocator.c */
typedef struct heap_block {
    uint32_t magic;             /* 0xDEADBEEF for corruption detection */
    size_t size;                /* Size of usable data (excluding header) */
    bool is_free;               /* true if block is available */
    struct heap_block *prev;    /* Previous block in memory */
    struct heap_block *next;    /* Next block in memory */
} heap_block_t;
```

#### 3.2.4 Allocation Strategies

NexaKernel's free list supports three allocation strategies:

##### First-Fit (Default)
```c
static freelist_block_t *find_first_fit(size_t size)
{
    list_node_t *current;
    list_for_each(current, &free_list) {
        freelist_block_t *block = list_entry(current, freelist_block_t, node);
        if (block->is_free && block->size >= size) {
            return block;  /* Return first block that fits */
        }
    }
    return NULL;
}
```
- **Pros:** Fast (stops at first match)
- **Cons:** May cause fragmentation at heap start

##### Best-Fit
```c
static freelist_block_t *find_best_fit(size_t size)
{
    freelist_block_t *best = NULL;
    size_t best_size = (size_t)-1;
    
    list_node_t *current;
    list_for_each(current, &free_list) {
        freelist_block_t *block = list_entry(current, freelist_block_t, node);
        if (block->is_free && block->size >= size) {
            if (block->size < best_size) {
                best = block;
                best_size = block->size;
                if (block->size == size) break;  /* Perfect fit */
            }
        }
    }
    return best;
}
```
- **Pros:** Minimizes wasted space per allocation
- **Cons:** Slower (must check all blocks), may create tiny fragments

##### Worst-Fit
```c
static freelist_block_t *find_worst_fit(size_t size)
{
    freelist_block_t *worst = NULL;
    size_t worst_size = 0;
    
    list_node_t *current;
    list_for_each(current, &free_list) {
        freelist_block_t *block = list_entry(current, freelist_block_t, node);
        if (block->is_free && block->size >= size) {
            if (block->size > worst_size) {
                worst = block;
                worst_size = block->size;
            }
        }
    }
    return worst;
}
```
- **Pros:** Leaves large remainders (useful fragments)
- **Cons:** May exhaust large blocks quickly

#### 3.2.5 Operations & Implementation

##### Operation 1: Initialize Heap

```c
void heap_init(void *start, size_t size)
{
    /* Align start address */
    uintptr_t aligned_start = ALIGN_UP((uintptr_t)start, HEAP_ALIGNMENT);
    
    /* Store boundaries */
    heap_start = (void *)aligned_start;
    heap_size = size - (aligned_start - (uintptr_t)start);
    heap_end = (void *)((char *)heap_start + heap_size);
    
    /* Create initial free block spanning entire heap */
    first_block = (heap_block_t *)heap_start;
    first_block->magic = HEAP_MAGIC;
    first_block->size = heap_size - sizeof(heap_block_t);
    first_block->is_free = true;
    first_block->prev = NULL;
    first_block->next = NULL;
}
```

**When Called:** Early in `kernel_main()`, after frame allocator is initialized.

##### Operation 2: Allocate Memory (kmalloc)

```c
void *kmalloc(size_t size)
{
    if (!heap_initialized || size == 0) return NULL;
    
    /* Align requested size */
    size = ALIGN_UP(size, HEAP_ALIGNMENT);
    if (size < HEAP_MIN_ALLOC_SIZE) size = HEAP_MIN_ALLOC_SIZE;
    
    /* Find a suitable block (first-fit) */
    heap_block_t *block = first_block;
    while (block) {
        if (!is_valid_block(block)) {
            PANIC("Heap corruption detected!");
        }
        
        if (block->is_free && block->size >= size) {
            /* Split block if much larger than needed */
            split_block(block, size);
            
            /* Mark as allocated */
            block->is_free = false;
            
            /* Update statistics */
            bytes_allocated += block->size;
            
            /* Return pointer to data area (after header) */
            return (void *)((char *)block + sizeof(heap_block_t));
        }
        
        block = block->next;
    }
    
    return NULL;  /* Out of memory */
}
```

**When Called:**
- Task creation (allocating Task Control Blocks)
- Creating data structures (queues, buffers)
- Device driver buffers

##### Operation 3: Split Block

```c
static void split_block(heap_block_t *block, size_t needed_size)
{
    /* Only split if remainder is large enough */
    if (block->size <= needed_size + HEAP_MIN_SPLIT_SIZE) {
        return;
    }
    
    /* Create new block in remaining space */
    heap_block_t *new_block = (heap_block_t *)
        ((char *)block + sizeof(heap_block_t) + needed_size);
    
    new_block->magic = HEAP_MAGIC;
    new_block->size = block->size - needed_size - sizeof(heap_block_t);
    new_block->is_free = true;
    new_block->prev = block;
    new_block->next = block->next;
    
    /* Update original block */
    block->size = needed_size;
    block->next = new_block;
}
```

**Visual Representation:**
```
Before split (request 100 bytes from 500 byte block):
┌────────────────────────────────────────────────────────────────┐
│ [Header: 24B][          Free Block: 500B                     ] │
└────────────────────────────────────────────────────────────────┘

After split:
┌────────────────────────────────────────────────────────────────┐
│ [Header: 24B][Alloc: 100B][Header: 24B][  Free: 352B        ] │
└────────────────────────────────────────────────────────────────┘
```

##### Operation 4: Free Memory (kfree)

```c
void kfree(void *ptr)
{
    if (!ptr) return;
    
    /* Get block header */
    heap_block_t *block = (heap_block_t *)
        ((char *)ptr - sizeof(heap_block_t));
    
    /* Validate block */
    if (!is_valid_block(block)) {
        PANIC("kfree: invalid block!");
    }
    
    /* Mark as free */
    block->is_free = true;
    bytes_allocated -= block->size;
    
    /* Coalesce with adjacent free blocks */
    coalesce_block(block);
}
```

##### Operation 5: Coalesce (Merge Adjacent Free Blocks)

```c
static void coalesce_block(heap_block_t *block)
{
    /* Merge with next block if free */
    if (block->next && block->next->is_free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    /* Merge with previous block if free */
    if (block->prev && block->prev->is_free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}
```

**Visual Representation:**
```
Before coalescing:
┌─────────────────────────────────────────────────────────────────┐
│ [Free: 100B][Header][Free: 200B][Header][Free: 150B]           │
└─────────────────────────────────────────────────────────────────┘

After coalescing (if middle block freed):
┌─────────────────────────────────────────────────────────────────┐
│ [Free: 100B + 24B + 200B + 24B + 150B = 498B]                   │
└─────────────────────────────────────────────────────────────────┘
```

**When Called:** Automatically after every `kfree()` to reduce fragmentation.

#### 3.2.6 Time & Space Complexity

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `heap_init` | O(1) | Single block creation |
| `kmalloc` (first-fit) | O(n) | n = number of free blocks |
| `kmalloc` (best-fit) | O(n) | Must check all blocks |
| `kfree` | O(1) | Direct access via pointer |
| `split_block` | O(1) | Constant operations |
| `coalesce_block` | O(1) | Only checks neighbors |

---

### 3.3 Buddy System (Power-of-Two Allocator)

#### 3.3.1 Concept Overview

The **buddy system** is a memory allocation algorithm that divides memory into power-of-two sized blocks. Each block has a "buddy" — an adjacent block of the same size that can be merged when both are free.

```
Memory divided into orders (order 0 = smallest):
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Order 3 (32 KB block)                             │
├───────────────────────────────────┬─────────────────────────────────────────┤
│         Order 2 (16 KB)           │              Order 2 (16 KB)            │
├─────────────────┬─────────────────┼─────────────────┬───────────────────────┤
│  Order 1 (8 KB) │  Order 1 (8 KB) │  Order 1 (8 KB) │     Order 1 (8 KB)    │
├────────┬────────┼────────┬────────┼────────┬────────┼────────┬──────────────┤
│O0 (4KB)│O0 (4KB)│O0 (4KB)│O0 (4KB)│O0 (4KB)│O0 (4KB)│O0 (4KB)│  O0 (4KB)   │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┴──────────────┘
   ↑         ↑
   └─ Buddies ┘  (can merge to form Order 1 block)
```

#### 3.3.2 Why Buddy System?

| Requirement | Why Buddy System Satisfies It |
|-------------|------------------------------|
| **O(log n) Allocation** | Binary tree structure |
| **O(log n) Deallocation** | Predictable merge pattern |
| **Low External Fragmentation** | Power-of-two sizes align naturally |
| **Efficient Coalescing** | Buddy relationship makes merging trivial |
| **Contiguous Allocation** | Each block is contiguous |

#### 3.3.3 Key Insight: Buddy Address Calculation

The genius of the buddy system is that you can calculate a block's buddy address using XOR:

```
Buddy Address = Block Address XOR Block Size

Example:
- Block at 0x1000, size 4KB (0x1000)
- Buddy = 0x1000 XOR 0x1000 = 0x0000

- Block at 0x2000, size 8KB (0x2000)  
- Buddy = 0x2000 XOR 0x2000 = 0x0000

- Block at 0x5000, size 4KB (0x1000)
- Buddy = 0x5000 XOR 0x1000 = 0x4000
```

#### 3.3.4 Data Structure Definition

```c
/* From kernel/memory/dsa_structures/buddy.h */

#define BUDDY_MIN_BLOCK_SIZE    4096    /* Minimum block = 4KB (one page) */
#define BUDDY_MAX_ORDER         10      /* Maximum order (4MB blocks) */

typedef struct buddy_block {
    list_node_t node;           /* Link in the free list */
    int order;                  /* Size = MIN_SIZE * 2^order */
} buddy_block_t;

/* Free list for each order */
static list_t free_areas[BUDDY_MAX_ORDER + 1];

/* Bitmap tracks allocation at minimum order */
static bitmap_t block_bitmap;
```

#### 3.3.5 Operations & Implementation

##### Operation 1: Calculate Order for Size

```c
int buddy_size_to_order(size_t size)
{
    if (size == 0) return 0;
    
    int order = 0;
    size_t block_size = BUDDY_MIN_BLOCK_SIZE;
    
    while (block_size < size && order < BUDDY_MAX_ORDER) {
        block_size <<= 1;  /* block_size *= 2 */
        order++;
    }
    
    return (block_size < size) ? -1 : order;
}
```

**Examples:**
| Requested Size | Order | Actual Block Size |
|----------------|-------|-------------------|
| 1 KB | 0 | 4 KB |
| 4 KB | 0 | 4 KB |
| 5 KB | 1 | 8 KB |
| 10 KB | 2 | 16 KB |
| 1 MB | 8 | 1 MB |

##### Operation 2: Get Buddy Address

```c
static void *get_buddy_address(void *block, int order)
{
    size_t block_size = BUDDY_MIN_BLOCK_SIZE << order;
    uintptr_t block_offset = (uintptr_t)block - (uintptr_t)memory_start;
    uintptr_t buddy_offset = block_offset ^ block_size;
    
    return (void *)((uintptr_t)memory_start + buddy_offset);
}
```

##### Operation 3: Allocate Block

```c
void *buddy_alloc(size_t size)
{
    int order = buddy_size_to_order(size);
    if (order < 0) return NULL;
    
    /* Find smallest available order >= requested */
    int current_order = order;
    while (current_order <= BUDDY_MAX_ORDER) {
        if (!list_is_empty(&free_areas[current_order])) {
            /* Found a free block */
            buddy_block_t *block = list_entry(
                list_pop_front(&free_areas[current_order]),
                buddy_block_t, node
            );
            
            /* Split down to requested order */
            while (current_order > order) {
                current_order--;
                /* Create buddy and add to lower order free list */
                void *buddy = (char *)block + (BUDDY_MIN_BLOCK_SIZE << current_order);
                add_to_free_list(buddy, current_order);
            }
            
            mark_allocated(block, order);
            return block;
        }
        current_order++;
    }
    
    return NULL;  /* Out of memory */
}
```

**Allocation Example:**
```
Request: 5 KB (needs order 1 = 8 KB)
Initial state: Only one order 3 block (32 KB) available

Step 1: Take order 3 block, split into two order 2 blocks
        ┌─────────────────────────────────────────────────────────┐
        │ [Order 2: USE] │ [Order 2: FREE → add to free_areas[2]] │
        └─────────────────────────────────────────────────────────┘

Step 2: Split order 2 block into two order 1 blocks
        ┌─────────────────────────────────────────────────────────┐
        │ [Order 1: USE] │ [Order 1: FREE → add to free_areas[1]] │
        └─────────────────────────────────────────────────────────┘

Result: Return order 1 block (8 KB), waste = 8 KB - 5 KB = 3 KB
```

##### Operation 4: Free Block

```c
void buddy_free(void *ptr, int order)
{
    if (!ptr || order < 0 || order > BUDDY_MAX_ORDER) return;
    
    mark_free(ptr, order);
    
    /* Try to merge with buddy */
    while (order < BUDDY_MAX_ORDER) {
        void *buddy = get_buddy_address(ptr, order);
        
        if (!is_buddy_free(buddy, order)) {
            break;  /* Buddy not free, can't merge */
        }
        
        /* Remove buddy from its free list */
        remove_from_free_list(buddy, order);
        
        /* Merged block starts at lower address */
        if ((uintptr_t)buddy < (uintptr_t)ptr) {
            ptr = buddy;
        }
        
        order++;  /* Move up to next order */
    }
    
    /* Add to free list at final order */
    add_to_free_list(ptr, order);
}
```

**Coalescing Example:**
```
Before free(Block B):
┌────────────────────────────────────────────────────────────────────────────┐
│ [Block A: FREE] │ [Block B: ALLOCATED] │ [Block C: ALLOCATED] │ [Block D: FREE] │
└────────────────────────────────────────────────────────────────────────────┘
      Order 0           Order 0                 Order 0              Order 0

After free(Block B) - A and B are buddies:
┌────────────────────────────────────────────────────────────────────────────┐
│ [    Blocks A+B: FREE (Order 1)    ] │ [Block C: ALLOCATED] │ [Block D: FREE] │
└────────────────────────────────────────────────────────────────────────────┘
              Order 1                          Order 0              Order 0
```

#### 3.3.6 Time & Space Complexity

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `buddy_size_to_order` | O(log n) | n = max block size / min block size |
| `buddy_alloc` | O(log n) | May need to split down |
| `buddy_free` | O(log n) | May merge up multiple levels |
| `get_buddy_address` | O(1) | XOR calculation |

**Space Overhead:**
- Free lists: O(number of orders)
- Bitmap: O(total memory / min block size / 8)

---

## 4. Process Scheduling Data Structures

The scheduler decides which task runs on the CPU. Different scheduling policies require different data structures.

### 4.1 Circular Queue (Round-Robin Scheduler)

#### 4.1.1 Concept Overview

A **circular queue** (ring buffer) is a fixed-size array that wraps around at the end. It's perfect for FIFO (First-In-First-Out) scheduling where tasks take turns in order.

```
Circular Queue (capacity=8, 5 tasks):

Logical view:
    ┌───┐    ┌───┐    ┌───┐    ┌───┐    ┌───┐
    │ A │ ─→ │ B │ ─→ │ C │ ─→ │ D │ ─→ │ E │
    └───┘    └───┘    └───┘    └───┘    └───┘
    head                                 tail

Physical array view:
    ┌───┬───┬───┬───┬───┬───┬───┬───┐
    │ A │ B │ C │ D │ E │   │   │   │
    └───┴───┴───┴───┴───┴───┴───┴───┘
      ↑                   ↑
    head                tail

After dequeue(A) and enqueue(F):
    ┌───┬───┬───┬───┬───┬───┬───┬───┐
    │   │ B │ C │ D │ E │ F │   │   │
    └───┴───┴───┴───┴───┴───┴───┴───┘
          ↑               ↑
        head            tail
```

#### 4.1.2 Why Circular Queue for Round-Robin?

| Requirement | Why Circular Queue Satisfies It |
|-------------|--------------------------------|
| **O(1) Enqueue/Dequeue** | No shifting required |
| **FIFO Order** | Tasks run in arrival order |
| **Bounded Memory** | Fixed size, no allocation during scheduling |
| **Fairness** | Every task gets equal CPU time |

#### 4.1.3 Data Structure Definition

```c
/* From kernel/scheduler/dsa_structures/round_robin_queue.c */
typedef struct rr_queue {
    task_t **buffer;        /* Array of task pointers */
    size_t capacity;        /* Maximum number of tasks */
    size_t size;            /* Current number of tasks */
    size_t head;            /* Index of front element */
    size_t tail;            /* Index of next free slot */
} rr_queue_t;
```

#### 4.1.4 Operations & Implementation

##### Operation 1: Initialize Queue

```c
bool rr_queue_init(size_t capacity)
{
    if (capacity == 0) return false;
    
    run_queue.buffer = (task_t **)kmalloc(capacity * sizeof(task_t *));
    if (run_queue.buffer == NULL) return false;
    
    for (size_t i = 0; i < capacity; i++) {
        run_queue.buffer[i] = NULL;
    }
    
    run_queue.capacity = capacity;
    run_queue.size = 0;
    run_queue.head = 0;
    run_queue.tail = 0;
    
    return true;
}
```

##### Operation 2: Enqueue (Add Task to Ready Queue)

```c
bool rr_enqueue(task_t *task)
{
    if (task == NULL || run_queue.size >= run_queue.capacity) {
        return false;
    }
    
    /* Add at tail position */
    run_queue.buffer[run_queue.tail] = task;
    
    /* Advance tail with wraparound */
    run_queue.tail = (run_queue.tail + 1) % run_queue.capacity;
    
    run_queue.size++;
    return true;
}
```

**When Called:**
- When a new task is created
- When a blocked task becomes ready
- When a running task's time slice expires

##### Operation 3: Dequeue (Get Next Task to Run)

```c
task_t *rr_dequeue(void)
{
    if (run_queue.size == 0) return NULL;
    
    /* Get task at head */
    task_t *task = run_queue.buffer[run_queue.head];
    
    /* Clear the slot */
    run_queue.buffer[run_queue.head] = NULL;
    
    /* Advance head with wraparound */
    run_queue.head = (run_queue.head + 1) % run_queue.capacity;
    
    run_queue.size--;
    return task;
}
```

**When Called:**
- On every scheduling decision
- When timer interrupt triggers reschedule

##### Operation 4: Peek (Check Next Task Without Removing)

```c
task_t *rr_peek(void)
{
    if (run_queue.size == 0) return NULL;
    return run_queue.buffer[run_queue.head];
}
```

##### Operation 5: Remove Specific Task

```c
bool rr_remove(task_t *task)
{
    /* Search for the task */
    size_t idx = run_queue.head;
    for (size_t i = 0; i < run_queue.size; i++) {
        if (run_queue.buffer[idx] == task) {
            /* Shift all subsequent elements */
            size_t current = idx;
            size_t next = (idx + 1) % run_queue.capacity;
            
            while (current != (run_queue.tail - 1 + run_queue.capacity) % run_queue.capacity) {
                run_queue.buffer[current] = run_queue.buffer[next];
                current = next;
                next = (next + 1) % run_queue.capacity;
            }
            
            run_queue.buffer[current] = NULL;
            run_queue.tail = (run_queue.tail - 1 + run_queue.capacity) % run_queue.capacity;
            run_queue.size--;
            return true;
        }
        idx = (idx + 1) % run_queue.capacity;
    }
    return false;
}
```

**When Called:**
- When a task terminates
- When a task blocks (waiting for I/O, sleep, etc.)

#### 4.1.5 Round-Robin Scheduling Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Round-Robin Scheduling Flow                          │
└─────────────────────────────────────────────────────────────────────────────┘

Timer Interrupt (every 10ms):
    │
    ▼
┌─────────────────────────────────────┐
│ Decrement current task's time slice │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Time slice = 0?                     │──── No ────→ Return (continue running)
└─────────────────────────────────────┘
    │ Yes
    ▼
┌─────────────────────────────────────┐
│ Reset time slice                    │
│ Enqueue current task (back of queue)│
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Dequeue next task                   │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Context switch to new task          │
└─────────────────────────────────────┘
```

#### 4.1.6 Time & Space Complexity

| Operation | Time Complexity | Space Complexity |
|-----------|-----------------|------------------|
| `rr_queue_init` | O(n) | O(n) |
| `rr_enqueue` | O(1) | O(1) |
| `rr_dequeue` | O(1) | O(1) |
| `rr_peek` | O(1) | O(1) |
| `rr_remove` | O(n) | O(1) |

---

### 4.2 Binary Heap / Priority Queue (Priority Scheduler)

#### 4.2.1 Concept Overview

A **binary heap** is a complete binary tree stored as an array where every parent has higher priority than its children. For scheduling, we use a **min-heap** where lower priority values mean higher importance.

```
Priority Queue (Min-Heap) for Tasks:

Tree view:
                    ┌───────────────┐
                    │ Task A (P=1)  │  ← Highest priority (runs first)
                    └───────┬───────┘
                ┌───────────┴───────────┐
        ┌───────┴───────┐       ┌───────┴───────┐
        │ Task B (P=2)  │       │ Task C (P=3)  │
        └───────┬───────┘       └───────┬───────┘
            ┌───┴───┐               ┌───┴───┐
        ┌───┴───┐ ┌─┴───┐       ┌───┴───┐ ┌─┴───┐
        │ D(P=5)│ │E(P=4)│      │ F(P=7)│ │G(P=6)│
        └───────┘ └─────┘       └───────┘ └─────┘

Array view:
Index:    0     1     2     3     4     5     6
        ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┐
        │  A  │  B  │  C  │  D  │  E  │  F  │  G  │
        │ P=1 │ P=2 │ P=3 │ P=5 │ P=4 │ P=7 │ P=6 │
        └─────┴─────┴─────┴─────┴─────┴─────┴─────┘

Parent-Child Relationships:
- Parent of i: (i-1)/2
- Left child of i: 2i+1
- Right child of i: 2i+2
```

#### 4.2.2 Why Binary Heap for Priority Scheduling?

| Requirement | Why Binary Heap Satisfies It |
|-------------|------------------------------|
| **O(log n) Insert** | Heapify-up restores heap property |
| **O(log n) Extract-Min** | Heapify-down restores heap property |
| **O(1) Peek** | Minimum always at root |
| **Array-Based** | Cache-friendly, no pointers needed |
| **Priority Order** | Highest priority task always accessible |

#### 4.2.3 Data Structure Definition

```c
/* From kernel/scheduler/dsa_structures/priority_queue.c */
typedef struct priority_queue {
    task_t **buffer;        /* Array of task pointers */
    size_t capacity;        /* Maximum number of tasks */
    size_t size;            /* Current number of tasks */
} priority_queue_t;
```

#### 4.2.4 Operations & Implementation

##### Operation 1: Index Calculations

```c
static inline size_t parent(size_t i) {
    return (i - 1) / 2;
}

static inline size_t left_child(size_t i) {
    return 2 * i + 1;
}

static inline size_t right_child(size_t i) {
    return 2 * i + 2;
}
```

##### Operation 2: Task Comparison

```c
static int task_compare(task_t *a, task_t *b)
{
    if (a == NULL || b == NULL) return 0;
    
    /* Lower priority value = higher priority */
    if (a->priority < b->priority) return -1;
    if (a->priority > b->priority) return 1;
    
    /* Tie-breaker: older task (lower PID) wins */
    if (a->pid < b->pid) return -1;
    if (a->pid > b->pid) return 1;
    
    return 0;
}
```

##### Operation 3: Heapify-Up (After Insertion)

```c
static void heapify_up(size_t index)
{
    while (index > 0) {
        size_t p = parent(index);
        
        /* If parent has higher priority, stop */
        if (task_compare(pq.buffer[p], pq.buffer[index]) <= 0) {
            break;
        }
        
        /* Parent has lower priority, swap */
        swap(index, p);
        index = p;
    }
}
```

**Visual Example:**
```
Insert Task D (P=2) into heap:

Step 1: Add at end (index 4)
        ┌─────┐
        │A P=1│
        └──┬──┘
     ┌─────┴─────┐
   ┌─┴─┐       ┌─┴─┐
   │B=3│       │C=4│
   └─┬─┘       └───┘
  ┌──┴──┐
┌─┴─┐ ┌─┴─┐
│E=5│ │D=2│ ← New task
└───┘ └───┘

Step 2: D(2) < B(3), swap
        ┌─────┐
        │A P=1│
        └──┬──┘
     ┌─────┴─────┐
   ┌─┴─┐       ┌─┴─┐
   │D=2│       │C=4│ ← D bubbled up
   └─┬─┘       └───┘
  ┌──┴──┐
┌─┴─┐ ┌─┴─┐
│E=5│ │B=3│
└───┘ └───┘

Step 3: D(2) > A(1), stop (heap property satisfied)
```

##### Operation 4: Heapify-Down (After Extraction)

```c
static void heapify_down(size_t index)
{
    while (true) {
        size_t smallest = index;
        size_t left = left_child(index);
        size_t right = right_child(index);
        
        /* Find smallest among node and children */
        if (left < pq.size && 
            task_compare(pq.buffer[left], pq.buffer[smallest]) < 0) {
            smallest = left;
        }
        
        if (right < pq.size && 
            task_compare(pq.buffer[right], pq.buffer[smallest]) < 0) {
            smallest = right;
        }
        
        /* If current is smallest, heap property satisfied */
        if (smallest == index) break;
        
        /* Swap and continue down */
        swap(index, smallest);
        index = smallest;
    }
}
```

**Visual Example:**
```
Extract min (A) from heap:

Step 1: Replace root with last element, remove last
Before:                    After replacing:
    ┌─────┐                    ┌─────┐
    │A P=1│                    │G P=6│ ← Last moved to root
    └──┬──┘                    └──┬──┘
  ┌────┴────┐                ┌────┴────┐
┌─┴─┐     ┌─┴─┐            ┌─┴─┐     ┌─┴─┐
│B=2│     │C=3│            │B=2│     │C=3│
└─┬─┘     └─┬─┘            └─┬─┘     └─┬─┘
┌─┴─┐     ┌─┴─┐            ┌─┴─┐     ┌─┴─┐
│D=4│     │E=5│            │D=4│     │E=5│
└───┘     └───┘            └───┘     └───┘

Step 2: Heapify-down G(6)
Compare G(6) with children B(2) and C(3)
Smallest = B(2), swap G and B

        ┌─────┐
        │B P=2│ ← B bubbled up
        └──┬──┘
  ┌────┴────┐
┌─┴─┐     ┌─┴─┐
│G=6│     │C=3│
└─┬─┘     └───┘
┌─┴─┐
│D=4│
└───┘

Step 3: Continue heapify-down G(6)
Compare G(6) with child D(4)
Smallest = D(4), swap G and D

        ┌─────┐
        │B P=2│
        └──┬──┘
  ┌────┴────┐
┌─┴─┐     ┌─┴─┐
│D=4│     │C=3│
└─┬─┘     └───┘
┌─┴─┐
│G=6│ ← G reached correct position
└───┘
```

##### Operation 5: Insert Task

```c
bool pq_enqueue(task_t *task)
{
    if (task == NULL || pq.size >= pq.capacity) {
        return false;
    }
    
    /* Add at end */
    pq.buffer[pq.size] = task;
    pq.size++;
    
    /* Restore heap property */
    heapify_up(pq.size - 1);
    
    return true;
}
```

##### Operation 6: Extract Minimum (Highest Priority Task)

```c
task_t *pq_dequeue(void)
{
    if (pq.size == 0) return NULL;
    
    /* Save root (minimum) */
    task_t *min_task = pq.buffer[0];
    
    /* Move last to root */
    pq.buffer[0] = pq.buffer[pq.size - 1];
    pq.size--;
    
    /* Restore heap property */
    if (pq.size > 0) {
        heapify_down(0);
    }
    
    return min_task;
}
```

#### 4.2.5 Priority Scheduling Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       Priority Scheduling Flow                              │
└─────────────────────────────────────────────────────────────────────────────┘

Scheduling Event (timer tick, task blocks, task created):
    │
    ▼
┌─────────────────────────────────────┐
│ Is current task still highest       │
│ priority?                           │
└─────────────────────────────────────┘
    │ Yes               │ No
    ▼                   ▼
┌─────────┐     ┌─────────────────────────────────┐
│ Continue│     │ Add current task back to heap    │
│ running │     │ (if still runnable)              │
└─────────┘     └─────────────────────────────────┘
                    │
                    ▼
                ┌─────────────────────────────────┐
                │ Extract-min from heap            │
                │ (get highest priority task)      │
                └─────────────────────────────────┘
                    │
                    ▼
                ┌─────────────────────────────────┐
                │ Context switch to new task       │
                └─────────────────────────────────┘
```

#### 4.2.6 Time & Space Complexity

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `pq_init` | O(n) | Allocate buffer |
| `pq_enqueue` | O(log n) | Heapify-up |
| `pq_dequeue` | O(log n) | Heapify-down |
| `pq_peek` | O(1) | Return root |
| `pq_remove` | O(n) | Find + heapify |
| `heapify_up` | O(log n) | Height of tree |
| `heapify_down` | O(log n) | Height of tree |

---

## 5. File System Data Structures

The file system organizes and manages files and directories on storage media.

### 5.1 N-ary Tree (Directory Hierarchy)

#### 5.1.1 Concept Overview

An **N-ary tree** is a tree where each node can have any number of children. This perfectly models directory hierarchies where:
- Each directory can contain multiple files and subdirectories
- Files are leaf nodes (no children)
- Directories are internal nodes (can have children)

```
File System Tree:
                        ┌───────┐
                        │   /   │ (root)
                        └───┬───┘
              ┌─────────────┼─────────────┐
          ┌───┴───┐     ┌───┴───┐     ┌───┴───┐
          │  bin  │     │  home │     │  etc  │
          └───┬───┘     └───┬───┘     └───┬───┘
         ┌────┼────┐       ┌┴───┐     ┌───┼────┐
       ┌─┴─┐┌─┴─┐┌─┴─┐   ┌─┴─┐┌─┴─┐ ┌─┴─┐┌─┴──┐
       │ls ││cp ││mv │   │usr││joe│ │pwd││conf│
       └───┘└───┘└───┘   └─┬─┘└─┬─┘ └───┘└────┘
                      ┌────┴┐  ┌┴───┐
                    ┌─┴──┐┌─┴┐│docs│
                    │file││..││    │
                    └────┘└──┘└────┘
```

#### 5.1.2 Why N-ary Tree for Directories?

| Requirement | Why N-ary Tree Satisfies It |
|-------------|----------------------------|
| **Hierarchical Structure** | Natural parent-child relationships |
| **Variable Children** | Directories can have any number of entries |
| **Path Resolution** | Traverse parent→child to resolve "/home/usr/file" |
| **Easy Navigation** | Parent pointers for ".." support |

#### 5.1.3 Data Structure Definition

```c
/* From lib/dsa/tree.h */
typedef struct tree_node {
    void *data;                     /* Pointer to file/directory metadata */
    struct tree_node *parent;       /* Parent directory */
    struct tree_node *first_child;  /* First child (file or subdirectory) */
    struct tree_node *next_sibling; /* Next entry in same directory */
} tree_node_t;
```

**Note:** Children are stored as a linked list (first_child → next_sibling) rather than an array, allowing unlimited children without reallocation.

#### 5.1.4 Operations & Implementation

##### Operation 1: Initialize Node

```c
void tree_node_init(tree_node_t *node, void *data)
{
    if (!node) return;
    node->data = data;
    node->parent = NULL;
    node->first_child = NULL;
    node->next_sibling = NULL;
}
```

##### Operation 2: Add Child (Create File/Directory)

```c
void tree_add_child(tree_node_t *parent, tree_node_t *child)
{
    if (!parent || !child) return;
    
    child->parent = parent;
    child->next_sibling = parent->first_child;  /* Insert at front */
    parent->first_child = child;
}
```

**Visual Example:**
```
Before: /home has children [usr]
        ┌────────┐
        │  home  │
        └────┬───┘
             │
         ┌───┴───┐
         │  usr  │
         └───────┘

After add_child(home, joe):
        ┌────────┐
        │  home  │
        └────┬───┘
             │
         ┌───┴───┐
         │  joe  │ → [usr]  (joe is now first_child)
         └───────┘
```

##### Operation 3: Find Child (Lookup File/Directory)

```c
tree_node_t *tree_find_child(tree_node_t *parent, void *data, 
                              int (*comparator)(void *, void *))
{
    if (!parent || !comparator) return NULL;
    
    tree_node_t *current = parent->first_child;
    while (current) {
        if (comparator(current->data, data) == 0) {
            return current;
        }
        current = current->next_sibling;
    }
    return NULL;
}
```

**When Called:**
- Resolving path components: `/home/usr` → find "home" in "/", then find "usr" in "home"
- Opening a file
- Creating a file (check if exists)

##### Operation 4: Remove Child

```c
void tree_remove_child(tree_node_t *parent, tree_node_t *child)
{
    if (!parent || !child) return;
    
    tree_node_t *current = parent->first_child;
    tree_node_t *prev = NULL;
    
    while (current) {
        if (current == child) {
            if (prev) {
                prev->next_sibling = current->next_sibling;
            } else {
                parent->first_child = current->next_sibling;
            }
            child->parent = NULL;
            child->next_sibling = NULL;
            return;
        }
        prev = current;
        current = current->next_sibling;
    }
}
```

#### 5.1.5 File System Integration

```c
/* From kernel/fs/dsa_structures/directory_tree.c */

static tree_node_t root_node;

void fs_tree_init(void *root_data) {
    tree_node_init(&root_node, root_data);
}

tree_node_t *fs_tree_get_root(void) {
    return &root_node;
}

void fs_tree_add_child(tree_node_t *parent, tree_node_t *child) {
    tree_add_child(parent, child);
}

tree_node_t *fs_tree_find_child(tree_node_t *parent, void *data, 
                                 int (*comparator)(void *, void *)) {
    return tree_find_child(parent, data, comparator);
}
```

#### 5.1.6 Time & Space Complexity

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `tree_add_child` | O(1) | Insert at front |
| `tree_find_child` | O(children) | Linear search through siblings |
| `tree_remove_child` | O(children) | Linear search to find |
| Path resolution | O(depth × avg_children) | Traverse each component |

---

### 5.2 Hash Map (Open File Table)

#### 5.2.1 Concept Overview

A **hash map** (hash table) provides fast key-value lookups using a hash function. In the file system, it maps file paths to file structures (file descriptors).

```
Hash Map for Open Files:

Key (path)            Hash Function      Buckets
─────────────────────────────────────────────────
"/home/file.txt"  ──→ hash() = 3  ──→  Bucket[3] ──→ ["/home/file.txt", fd=5]
"/etc/config"     ──→ hash() = 7  ──→  Bucket[7] ──→ ["/etc/config", fd=8]
"/home/data.txt"  ──→ hash() = 3  ──→  Bucket[3] ──→ collision! chain:
                                                     ["/home/file.txt", fd=5]
                                                           ↓
                                                     ["/home/data.txt", fd=9]
```

#### 5.2.2 Why Hash Map for Open File Table?

| Requirement | Why Hash Map Satisfies It |
|-------------|--------------------------|
| **O(1) Average Lookup** | Direct index calculation |
| **String Keys** | Hash any path string |
| **Dynamic Growth** | Can handle varying file counts |
| **Common Operations** | Insert/find/delete all O(1) average |

#### 5.2.3 Data Structure Definition

```c
/* From lib/dsa/hashmap.h */
typedef struct hashmap_entry {
    char *key;                      /* File path (string key) */
    void *value;                    /* File structure pointer */
    struct hashmap_entry *next;     /* Next entry in chain (collision handling) */
} hashmap_entry_t;

typedef struct hashmap {
    hashmap_entry_t **buckets;      /* Array of bucket pointers */
    size_t bucket_count;            /* Number of buckets */
    size_t size;                    /* Number of entries */
} hashmap_t;
```

#### 5.2.4 Hash Function: DJB2

```c
/* DJB2 hash algorithm - fast and good distribution */
uint32_t hashmap_hash_string(const char *str)
{
    uint32_t hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  /* hash * 33 + c */
    }
    
    return hash;
}
```

**Why DJB2?**
- Simple and fast (only shifts and additions)
- Good distribution for strings
- Low collision rate
- Magic number 5381 provides good initial seed

#### 5.2.5 Operations & Implementation

##### Operation 1: Initialize Hash Map

```c
bool hashmap_init(hashmap_t *map, size_t bucket_count)
{
    if (!map || bucket_count == 0) return false;
    
    map->buckets = (hashmap_entry_t **)kmalloc(bucket_count * sizeof(hashmap_entry_t *));
    if (!map->buckets) return false;
    
    memset(map->buckets, 0, bucket_count * sizeof(hashmap_entry_t *));
    map->bucket_count = bucket_count;
    map->size = 0;
    
    return true;
}
```

##### Operation 2: Put (Insert/Update)

```c
bool hashmap_put(hashmap_t *map, const char *key, void *value)
{
    if (!map || !key) return false;
    
    uint32_t hash = hashmap_hash_string(key);
    size_t index = hash % map->bucket_count;
    
    /* Check if key exists (update) */
    hashmap_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;  /* Update existing */
            return true;
        }
        entry = entry->next;
    }
    
    /* Create new entry (insert) */
    hashmap_entry_t *new_entry = (hashmap_entry_t *)kmalloc(sizeof(hashmap_entry_t));
    if (!new_entry) return false;
    
    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = map->buckets[index];  /* Insert at front */
    map->buckets[index] = new_entry;
    map->size++;
    
    return true;
}
```

**When Called:**
- When opening a file (add to open file table)
- Updating file position/state

##### Operation 3: Get (Lookup)

```c
void *hashmap_get(hashmap_t *map, const char *key)
{
    if (!map || !key) return NULL;
    
    uint32_t hash = hashmap_hash_string(key);
    size_t index = hash % map->bucket_count;
    
    hashmap_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}
```

**When Called:**
- Reading from a file (get file descriptor)
- Writing to a file
- Any file operation by path

##### Operation 4: Remove

```c
void hashmap_remove(hashmap_t *map, const char *key)
{
    if (!map || !key) return;
    
    uint32_t hash = hashmap_hash_string(key);
    size_t index = hash % map->bucket_count;
    
    hashmap_entry_t *entry = map->buckets[index];
    hashmap_entry_t *prev = NULL;
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            kfree(entry->key);
            kfree(entry);
            map->size--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}
```

**When Called:**
- Closing a file
- Process termination (close all files)

#### 5.2.6 Collision Handling

NexaKernel uses **separate chaining** — each bucket is a linked list of entries that hash to that bucket.

```
Collision Example:
                    hash("/a") = hash("/z") = 5 (hypothetically)

Bucket Array:
Index: 0   1   2   3   4   5   6   7
       │   │   │   │   │   │   │   │
       ▼   ▼   ▼   ▼   ▼   ▼   ▼   ▼
     NULL NULL NULL NULL NULL │ NULL NULL
                              ▼
                         ┌─────────┐
                         │ key="/a"│
                         │ val=... │
                         │ next ───┼──→ ┌─────────┐
                         └─────────┘    │ key="/z"│
                                        │ val=... │
                                        │ next=NULL│
                                        └─────────┘
```

#### 5.2.7 Time & Space Complexity

| Operation | Average | Worst Case | Notes |
|-----------|---------|------------|-------|
| `hashmap_put` | O(1) | O(n) | Worst when all keys collide |
| `hashmap_get` | O(1) | O(n) | Worst when all keys collide |
| `hashmap_remove` | O(1) | O(n) | Worst when all keys collide |
| Space | O(n + m) | — | n entries, m buckets |

**Load Factor:** n/m — ideally kept below 0.7 to maintain O(1) performance.

---

### 5.3 Trie (File Indexing)

#### 5.3.1 Concept Overview

A **trie** (prefix tree) is a tree where each path from root to node represents a string prefix. It's ideal for prefix-based searches like filename autocomplete.

```
Trie for filenames: ["cat", "car", "card", "dog", "do"]

                    (root)
                    /    \
                   c      d
                  /        \
                 a          o
                / \          \
               t   r          g
                    \
                     d

Searching "car": root → c → a → r → found!
Searching "cab": root → c → a → b → not found (no 'b' child)
```

#### 5.3.2 Why Trie for File Indexing?

| Requirement | Why Trie Satisfies It |
|-------------|----------------------|
| **O(key length) Search** | Independent of number of entries |
| **Prefix Search** | Find all files starting with "doc" |
| **Autocomplete** | Traverse to prefix, enumerate children |
| **No Collisions** | Unlike hash map, no collision handling needed |

#### 5.3.3 Data Structure Definition

```c
/* From lib/dsa/trie.h */
#define TRIE_ALPHABET_SIZE 256  /* Support all ASCII characters */

typedef struct trie_node {
    struct trie_node *children[TRIE_ALPHABET_SIZE];
    void *data;           /* Associated data (file metadata) */
    bool is_terminal;     /* Is this the end of a valid key? */
} trie_node_t;

typedef struct trie {
    trie_node_t *root;
} trie_t;
```

**Note:** Full ASCII alphabet (256 characters) supports all filename characters.

#### 5.3.4 Operations & Implementation

##### Operation 1: Initialize Trie

```c
static trie_node_t *create_node(void)
{
    trie_node_t *node = (trie_node_t *)kmalloc(sizeof(trie_node_t));
    if (node) {
        node->data = NULL;
        node->is_terminal = false;
        memset(node->children, 0, sizeof(node->children));
    }
    return node;
}

void trie_init(trie_t *trie)
{
    if (!trie) return;
    trie->root = create_node();
}
```

##### Operation 2: Insert (Index a File)

```c
bool trie_insert(trie_t *trie, const char *key, void *data)
{
    if (!trie || !trie->root || !key) return false;
    
    trie_node_t *current = trie->root;
    
    while (*key) {
        unsigned char index = (unsigned char)*key;
        
        if (!current->children[index]) {
            current->children[index] = create_node();
            if (!current->children[index]) return false;
        }
        
        current = current->children[index];
        key++;
    }
    
    current->data = data;
    current->is_terminal = true;
    
    return true;
}
```

**Visual Example:**
```
Insert "cat":
Step 1: root['c'] = new node
Step 2: root['c']['a'] = new node
Step 3: root['c']['a']['t'] = new node, mark terminal

After inserting "cat", "car":
        (root)
           │
       ['c']
           │
       ['a']
        /   \
    ['t']  ['r']
    term   term
```

**When Called:**
- When creating a new file
- Building file index at mount time

##### Operation 3: Search

```c
void *trie_search(trie_t *trie, const char *key)
{
    if (!trie || !trie->root || !key) return NULL;
    
    trie_node_t *current = trie->root;
    
    while (*key) {
        unsigned char index = (unsigned char)*key;
        
        if (!current->children[index]) {
            return NULL;  /* Path doesn't exist */
        }
        
        current = current->children[index];
        key++;
    }
    
    return current->is_terminal ? current->data : NULL;
}
```

**When Called:**
- Looking up a file by name
- Checking if file exists

##### Operation 4: Remove

```c
static bool remove_recursive(trie_node_t *node, const char *key)
{
    if (*key == '\0') {
        if (node->is_terminal) {
            node->is_terminal = false;
            node->data = NULL;
            return !has_children(node);  /* Safe to delete if no children */
        }
        return false;
    }
    
    unsigned char index = (unsigned char)*key;
    if (!node->children[index]) return false;
    
    if (remove_recursive(node->children[index], key + 1)) {
        kfree(node->children[index]);
        node->children[index] = NULL;
        return !node->is_terminal && !has_children(node);
    }
    
    return false;
}

bool trie_remove(trie_t *trie, const char *key)
{
    if (!trie || !trie->root || !key) return false;
    return remove_recursive(trie->root, key);
}
```

**When Called:**
- Deleting a file
- Unmounting filesystem

#### 5.3.5 File System Integration

```c
/* From kernel/fs/dsa_structures/trie.c */

static trie_t file_index;

void fs_index_init(void) {
    trie_init(&file_index);
}

bool fs_index_add(const char *filename, void *inode) {
    return trie_insert(&file_index, filename, inode);
}

void *fs_index_get(const char *filename) {
    return trie_search(&file_index, filename);
}

bool fs_index_remove(const char *filename) {
    return trie_remove(&file_index, filename);
}
```

#### 5.3.6 Time & Space Complexity

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `trie_insert` | O(m) | m = key length |
| `trie_search` | O(m) | m = key length |
| `trie_remove` | O(m) | m = key length |
| Space | O(ALPHABET_SIZE × total nodes) | Can be memory-intensive |

**Trade-off:** Tries use more memory than hash maps but provide prefix operations and guaranteed O(m) time regardless of number of entries.

---

## 6. Inter-Process Communication Data Structures

IPC enables tasks to communicate and synchronize.

### 6.1 Circular Buffer (Message Queues)

#### 6.1.1 Concept Overview

A **circular buffer** (ring buffer) for IPC stores messages in FIFO order. Producers (senders) add messages at the tail, and consumers (receivers) remove from the head.

```
Message Queue Circular Buffer:

┌───────────────────────────────────────────────────────────────┐
│ Message Queue (capacity=8, 4 messages)                        │
├───┬───┬───┬───┬───┬───┬───┬───┬                              │
│ M1│ M2│ M3│ M4│   │   │   │   │                              │
├───┴───┴───┴───┴───┴───┴───┴───┤                              │
│  ↑                   ↑         │                              │
│ head               tail        │                              │
│ (next read)      (next write)  │                              │
└───────────────────────────────────────────────────────────────┘

Message structure:
┌─────────────────────────────────┐
│ sender_pid: 5                   │
│ type: MSG_REQUEST               │
│ size: 64                        │
│ data[256]: "Hello, receiver!"   │
└─────────────────────────────────┘
```

#### 6.1.2 Why Circular Buffer for Message Queues?

| Requirement | Why Circular Buffer Satisfies It |
|-------------|----------------------------------|
| **O(1) Send/Receive** | Direct array access |
| **FIFO Ordering** | Messages processed in order |
| **Bounded Memory** | Fixed-size prevents runaway memory |
| **No Allocation** | Pre-allocated, no per-message alloc |

#### 6.1.3 Data Structure Definition

```c
/* From kernel/ipc/message_queue.c */

#define MAX_MESSAGES    32
#define MAX_MSG_SIZE    256

typedef struct {
    uint32_t sender_pid;
    uint32_t type;
    size_t size;
    uint8_t data[MAX_MSG_SIZE];
} message_t;

typedef struct {
    bool valid;
    uint32_t key;
    uint32_t ref_count;
    
    message_t messages[MAX_MESSAGES];  /* Circular buffer */
    size_t head;                        /* Next to read */
    size_t tail;                        /* Next to write */
    size_t count;                       /* Current message count */
} msgq_t;
```

#### 6.1.4 Operations & Implementation

##### Operation 1: Create Queue

```c
int msgq_create(uint32_t key)
{
    /* Check if queue exists */
    for (size_t i = 0; i < MAX_QUEUES; i++) {
        if (queues[i].valid && queues[i].key == key) {
            queues[i].ref_count++;
            return (int)i;  /* Return existing */
        }
    }
    
    /* Find empty slot */
    for (size_t i = 0; i < MAX_QUEUES; i++) {
        if (!queues[i].valid) {
            queues[i].valid = true;
            queues[i].key = key;
            queues[i].ref_count = 1;
            queues[i].head = 0;
            queues[i].tail = 0;
            queues[i].count = 0;
            return (int)i;
        }
    }
    
    return -1;  /* No slots */
}
```

##### Operation 2: Send Message

```c
int msgq_send(int qid, const void *data, size_t size, uint32_t type)
{
    if (qid < 0 || qid >= MAX_QUEUES) return -1;
    
    msgq_t *q = &queues[qid];
    if (!q->valid) return -1;
    
    /* Check if queue is full */
    if (q->count >= MAX_MESSAGES) {
        return -1;  /* Queue full */
    }
    
    /* Copy message to tail position */
    message_t *msg = &q->messages[q->tail];
    msg->sender_pid = /* current task PID */;
    msg->type = type;
    msg->size = (size > MAX_MSG_SIZE) ? MAX_MSG_SIZE : size;
    memcpy(msg->data, data, msg->size);
    
    /* Advance tail with wraparound */
    q->tail = (q->tail + 1) % MAX_MESSAGES;
    q->count++;
    
    return 0;
}
```

**When Called:**
- Task wants to send data to another task
- Producer-consumer pattern implementation

##### Operation 3: Receive Message

```c
int msgq_receive(int qid, void *buffer, size_t buffer_size, uint32_t *type)
{
    if (qid < 0 || qid >= MAX_QUEUES) return -1;
    
    msgq_t *q = &queues[qid];
    if (!q->valid || q->count == 0) return -1;
    
    /* Get message at head */
    message_t *msg = &q->messages[q->head];
    
    /* Copy data to buffer */
    size_t copy_size = (msg->size < buffer_size) ? msg->size : buffer_size;
    memcpy(buffer, msg->data, copy_size);
    
    if (type) *type = msg->type;
    
    /* Advance head with wraparound */
    q->head = (q->head + 1) % MAX_MESSAGES;
    q->count--;
    
    return (int)copy_size;
}
```

**When Called:**
- Task waiting for messages
- Consumer processing incoming data

#### 6.1.5 IPC Communication Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Message Queue Communication                          │
└─────────────────────────────────────────────────────────────────────────────┘

Producer Task                              Consumer Task
─────────────────                          ────────────────
      │                                           │
      ▼                                           │
┌─────────────────┐                               │
│ msgq_create(42) │ ──→ Creates/gets queue 42     │
└─────────────────┘                               │
      │                                           ▼
      │                                  ┌─────────────────┐
      │                                  │ msgq_get(42)    │
      │                                  └─────────────────┘
      ▼                                           │
┌─────────────────┐                               │
│ msgq_send(...)  │ ──→ Message enters queue ─────┼────┐
└─────────────────┘                               │    │
      │                                           │    │
      │                                           ▼    │
      │                                  ┌─────────────────┐
      │                                  │ msgq_receive()  │ ←──┘
      │                                  └─────────────────┘
      │                                           │
      ▼                                           ▼
    (continue)                                 (process message)
```

#### 6.1.6 Time & Space Complexity

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| `msgq_create` | O(n) | Search for existing/free slot |
| `msgq_send` | O(1) | Direct array access |
| `msgq_receive` | O(1) | Direct array access |
| `msgq_destroy` | O(1) | Just update flags |

---

## 7. Foundational Data Structures

These data structures are building blocks used by higher-level structures.

### 7.1 Intrusive Doubly Linked List

#### 7.1.1 Concept Overview

An **intrusive** linked list embeds the list node directly within the data structure being stored, rather than wrapping data in separate node allocations.

```
Traditional (Non-Intrusive) List:
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ ListNode    │ ─→ │ ListNode    │ ─→ │ ListNode    │
│ data: ptr ──┼──→ │ data: ptr ──┼──→ │ data: ptr ──┼──→ ...
└─────────────┘ │  └─────────────┘ │  └─────────────┘ │
                ▼                  ▼                  ▼
          ┌─────────┐        ┌─────────┐        ┌─────────┐
          │ MyData  │        │ MyData  │        │ MyData  │
          └─────────┘        └─────────┘        └─────────┘

Intrusive List:
┌─────────────────────┐    ┌─────────────────────┐
│ MyData              │    │ MyData              │
│ ...                 │    │ ...                 │
│ ┌─────────────────┐ │ ─→ │ ┌─────────────────┐ │ ─→ ...
│ │ list_node_t     │ │    │ │ list_node_t     │ │
│ │ (embedded)      │ │    │ │ (embedded)      │ │
│ └─────────────────┘ │    │ └─────────────────┘ │
└─────────────────────┘    └─────────────────────┘
```

#### 7.1.2 Why Intrusive Lists?

| Advantage | Explanation |
|-----------|-------------|
| **Zero Allocation** | No separate node allocation |
| **Cache Friendly** | Node and data are contiguous |
| **O(1) Removal** | Given a node, can remove without search |
| **No Memory Fragmentation** | No list node allocations to fragment heap |

#### 7.1.3 Data Structure Definition

```c
/* From lib/dsa/list.h */

typedef struct list_node {
    struct list_node *next;
    struct list_node *prev;
} list_node_t;

typedef struct list {
    list_node_t *head;
    list_node_t *tail;
    size_t size;
} list_t;

/* Magic macro to get container from node */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

#### 7.1.4 The `list_entry` Macro Explained

```c
/* Given a pointer to the list_node inside a struct,
   get a pointer to the containing struct */

struct my_data {
    int value;           /* offset 0 */
    char name[20];       /* offset 4 */
    list_node_t node;    /* offset 24 */
};

/* If we have list_node_t *node_ptr pointing to the node field: */
struct my_data *data = list_entry(node_ptr, struct my_data, node);

/* Calculation:
   data = (struct my_data *)((char *)node_ptr - offsetof(struct my_data, node))
   data = (struct my_data *)((char *)node_ptr - 24)
*/
```

#### 7.1.5 Operations

```c
/* Initialize */
void list_init(list_t *list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

/* Push to front */
void list_push_front(list_t *list, list_node_t *node) {
    node->next = list->head;
    node->prev = NULL;
    if (list->head) list->head->prev = node;
    else list->tail = node;
    list->head = node;
    list->size++;
}

/* Push to back */
void list_push_back(list_t *list, list_node_t *node) {
    node->next = NULL;
    node->prev = list->tail;
    if (list->tail) list->tail->next = node;
    else list->head = node;
    list->tail = node;
    list->size++;
}

/* Remove node */
void list_remove(list_t *list, list_node_t *node) {
    if (node->prev) node->prev->next = node->next;
    else list->head = node->next;
    if (node->next) node->next->prev = node->prev;
    else list->tail = node->prev;
    list->size--;
}
```

#### 7.1.6 Iteration Macro

```c
/* Iterate over all nodes */
#define list_for_each(pos, list) \
    for (pos = (list)->head; pos != NULL; pos = pos->next)

/* Usage example */
list_node_t *current;
list_for_each(current, &my_list) {
    struct my_data *data = list_entry(current, struct my_data, node);
    /* process data */
}
```

---

### 7.2 Generic Binary Tree Helpers

#### 7.2.1 Concept Overview

These helper functions calculate array indices for binary tree operations (used by heaps and buddy system).

```
Array-based Binary Tree:

          [0]              Index:     0
         /   \                      /   \
       [1]   [2]                   1     2
      /  \   /  \                 / \   / \
    [3] [4] [5] [6]              3   4 5   6

Parent of i: (i-1)/2
Left child of i: 2i+1
Right child of i: 2i+2
Sibling of i: i±1 (if i is left/right child)
```

#### 7.2.2 Implementation

```c
/* From lib/dsa/tree.c */

size_t binary_tree_left_child(size_t index) {
    return 2 * index + 1;
}

size_t binary_tree_right_child(size_t index) {
    return 2 * index + 2;
}

size_t binary_tree_parent(size_t index) {
    return (index - 1) / 2;
}

bool binary_tree_is_left_child(size_t index) {
    return index % 2 != 0;  /* Odd indices are left children */
}

bool binary_tree_is_right_child(size_t index) {
    return index % 2 == 0 && index != 0;  /* Even non-zero are right children */
}

size_t binary_tree_sibling(size_t index) {
    if (index == 0) return 0;  /* Root has no sibling */
    return binary_tree_is_left_child(index) ? index + 1 : index - 1;
}
```

---

## 8. Time Complexity Summary

### Complete Complexity Reference

| Data Structure | Operation | Time | Space | Used By |
|----------------|-----------|------|-------|---------|
| **Bitmap** | set/clear/test | O(1) | O(n/8) | Frame Allocator |
| | find_first_zero | O(n/8) | O(1) | |
| **Free List** | alloc (first-fit) | O(n) | O(n) | Heap Allocator |
| | free + coalesce | O(1) | O(1) | |
| **Buddy System** | alloc/free | O(log n) | O(n/8) | Block Allocator |
| **Circular Queue** | enqueue/dequeue | O(1) | O(n) | Round-Robin |
| | remove | O(n) | O(1) | |
| **Binary Heap** | insert/extract | O(log n) | O(n) | Priority Scheduler |
| | peek | O(1) | O(1) | |
| **N-ary Tree** | add_child | O(1) | O(n) | Directory Tree |
| | find_child | O(c) | O(1) | |
| **Hash Map** | put/get/remove | O(1)* | O(n+m) | Open File Table |
| **Trie** | insert/search/remove | O(k) | O(Σ×n) | File Index |

*Average case; worst case O(n) with all collisions
c = children count, k = key length, n = entries, m = buckets, Σ = alphabet size

---

## 9. Viva Questions & Answers

### General Concepts

**Q1: Why do operating systems need specialized data structures instead of using standard libraries?**

**A:** Kernel code runs in a freestanding environment without standard library support. Additionally:
- **No dynamic allocation** during boot (heap not yet initialized)
- **No exceptions** (C doesn't have them, crashes are fatal)
- **Real-time constraints** require predictable performance
- **Memory protection** prevents using user-space libraries
- **Custom requirements** like intrusive lists are specific to kernel programming

---

**Q2: Explain the trade-off between time and space complexity in OS data structures.**

**A:** 
- **Bitmap** trades time (O(n) search) for space (only 1 bit per frame)
- **Hash Map** trades space (extra bucket array) for time (O(1) lookup)
- **Buddy System** trades internal fragmentation for O(log n) coalescing
- **Trie** trades space (large alphabet array per node) for O(key length) operations

---

### Memory Management

**Q3: Why use a bitmap instead of a linked list for frame allocation?**

**A:**
| Aspect | Bitmap | Linked List |
|--------|--------|-------------|
| Space | N/8 bytes | N × sizeof(node) |
| Status check | O(1) | O(n) |
| Contiguous search | Easy (scan bytes) | Complex |
| Cache performance | Excellent (contiguous) | Poor (scattered) |

For 64K frames: Bitmap = 8KB, Linked list ≈ 512KB

---

**Q4: Explain how the buddy system's XOR-based buddy calculation works.**

**A:** For a block at address A of size S:
```
Buddy = A XOR S
```

This works because:
1. Buddy pairs are at addresses that differ only in one bit position
2. XOR toggles that specific bit
3. Example: Block at 0x1000 (4KB), buddy = 0x1000 XOR 0x1000 = 0x0000
4. Example: Block at 0x2000 (4KB), buddy = 0x2000 XOR 0x1000 = 0x3000

---

**Q5: What is internal vs external fragmentation? Which data structures address each?**

**A:**
- **Internal fragmentation**: Wasted space INSIDE allocated blocks
  - Buddy system causes this (allocate 5KB, get 8KB block)
  - Free list with splitting minimizes this

- **External fragmentation**: Free memory exists but not contiguous
  - Free list with coalescing addresses this
  - Buddy system's merge operation addresses this

---

### Scheduling

**Q6: Why is O(1) scheduling important? How do circular queues achieve it?**

**A:** Scheduling happens on EVERY timer interrupt (100+ times/second). O(n) operations would slow the system.

Circular queue achieves O(1) because:
- Enqueue: buffer[tail] = task; tail = (tail+1) % capacity
- Dequeue: task = buffer[head]; head = (head+1) % capacity
- No scanning, no moving elements

---

**Q7: Compare round-robin and priority scheduling. When would you use each?**

**A:**
| Aspect | Round-Robin | Priority |
|--------|-------------|----------|
| Complexity | O(1) | O(log n) |
| Fairness | Guaranteed | May starve low-priority |
| Use case | Time-sharing servers | Real-time systems |
| Responsiveness | Equal for all | High for important tasks |
| Implementation | Circular queue | Binary heap |

---

**Q8: Explain heapify-up and heapify-down operations.**

**A:**
- **Heapify-up** (after insert): New element "bubbles up" by swapping with parent until heap property is satisfied
  ```
  Insert: Add at end, then while (child < parent) swap(child, parent)
  ```

- **Heapify-down** (after extract): Replacement element "sinks down" by swapping with smaller child
  ```
  Extract: Replace root with last, then while (parent > min_child) swap(parent, min_child)
  ```

---

### File System

**Q9: Why use different data structures (tree, hash map, trie) for the same subsystem?**

**A:** Each serves a different purpose:
- **N-ary Tree**: Represents hierarchical directory structure (natural for parent-child relationships)
- **Hash Map**: O(1) lookup for open files by path (fast access)
- **Trie**: Prefix search for autocomplete, finding files with common prefix

---

**Q10: Explain how the DJB2 hash function works and why it's good for strings.**

**A:**
```c
hash = 5381  // Magic initial value
while (*str) {
    hash = hash * 33 + *str  // Or: (hash << 5) + hash + c
    str++
}
```

Properties:
- Simple arithmetic (shift and add = fast)
- Good avalanche effect (small input change → big hash change)
- Low collision rate empirically
- 33 = 32 + 1 → shift + add optimization

---

### IPC

**Q11: Why use a circular buffer for message queues instead of a dynamic list?**

**A:**
| Aspect | Circular Buffer | Dynamic List |
|--------|-----------------|--------------|
| Memory | Fixed, pre-allocated | Grows dynamically |
| Allocation | None during operation | Per message |
| Bounded | Yes (prevents memory exhaustion) | No (can run out) |
| Cache | Excellent (contiguous) | Poor (fragmented) |
| Predictability | Deterministic | Variable latency |

---

### Advanced

**Q12: How would you modify the bitmap to support O(1) first-free-bit finding?**

**A:** Maintain a hierarchical bitmap:
```
Level 2:  [1] (any free in group 0-63?)
          / \
Level 1: [1] [0] (any free in group 0-31 or 32-63?)
         /\ 
Level 0: [10101...] [11111...] (actual frame bits)
```
- If level N has a 0, search children
- Skip entire subtrees when parent indicates "all full"
- Reduces search from O(n) to O(log n)

---

**Q13: How does the kernel handle the "chicken-and-egg" problem of memory allocation?**

**A:** Boot order matters:
1. **Static allocation**: Frame bitmap is a static array
2. **Frame allocator init**: Uses static bitmap, no heap needed
3. **Heap init**: Gets frames from frame allocator
4. **Dynamic structures**: Now kmalloc() works

---

**Q14: What happens if the free list becomes extremely fragmented?**

**A:** Symptoms:
- Allocation fails despite having enough total free memory
- Search time increases (more blocks to scan)
- Memory utilization decreases

Solutions:
- Coalescing on free (immediate merge)
- Periodic compaction (expensive, may require copying)
- Buddy system (guarantees coalescible blocks)
- Different allocation strategies (best-fit reduces small fragments)

---

## 10. Quick Reference Cheat Sheet

### Memory Allocation Decision Tree

```
Need to allocate memory?
         │
         ▼
┌─────────────────────┐
│ Fixed-size blocks?  │
└─────────────────────┘
    │Yes           │No
    ▼              ▼
┌────────┐    ┌─────────────────────┐
│ BITMAP │    │ Power-of-2 sizes OK?│
└────────┘    └─────────────────────┘
                  │Yes         │No
                  ▼            ▼
             ┌──────────┐  ┌───────────┐
             │  BUDDY   │  │ FREE LIST │
             │  SYSTEM  │  │           │
             └──────────┘  └───────────┘
```

### Scheduling Algorithm Selection

```
Scheduling requirements?
         │
         ├─── Need fairness ─────────→ ROUND-ROBIN (Circular Queue)
         │
         ├─── Need priority ──────────→ PRIORITY (Binary Heap)
         │
         └─── Need both ──────────────→ Multilevel Feedback Queue
                                        (Multiple queues + aging)
```

### File System Operation Mapping

```
Operation              Data Structure Used
─────────────────────────────────────────
ls /home/user         N-ary Tree traversal
open("/etc/passwd")   Hash Map lookup
autocomplete "Do"     Trie prefix search
mkdir                 Tree add_child
rm file               Tree remove + Trie remove
close(fd)             Hash Map remove
```

### Key Formulas

```
Binary Heap:
  Parent(i) = (i - 1) / 2
  Left(i)   = 2i + 1
  Right(i)  = 2i + 2

Buddy System:
  Buddy(addr, size) = addr XOR size
  Order(size) = ceil(log2(size / MIN_SIZE))

Hash Map:
  Index = hash(key) % bucket_count
  Load Factor = entries / buckets

Bitmap:
  Byte Index = bit_index / 8
  Bit Position = bit_index % 8
  Set: buffer[byte] |= (1 << bit)
  Clear: buffer[byte] &= ~(1 << bit)
```

---

## 11. Glossary

| Term | Definition |
|------|------------|
| **Bitmap** | Array of bits representing boolean states |
| **Buddy** | Adjacent block of same size in buddy system |
| **Coalescing** | Merging adjacent free blocks |
| **Context Switch** | Saving one task's state and loading another's |
| **DJB2** | Dan J. Bernstein's hash algorithm |
| **FIFO** | First-In-First-Out ordering |
| **Frame** | Fixed-size block of physical memory |
| **Heap** | (1) Data structure for priority queue; (2) Dynamic memory region |
| **Heapify** | Restoring heap property after modification |
| **Intrusive List** | List where node is embedded in data structure |
| **IPC** | Inter-Process Communication |
| **Load Factor** | Ratio of entries to buckets in hash map |
| **Min-Heap** | Heap where parent ≤ children |
| **N-ary Tree** | Tree where each node can have N children |
| **Page** | Virtual memory unit (typically same size as frame) |
| **PID** | Process Identifier |
| **Preemption** | Forcibly stopping a task to run another |
| **Ring Buffer** | Another name for circular queue |
| **Starvation** | Task never getting CPU time |
| **TCB** | Task Control Block (task metadata) |
| **Time Slice** | Maximum CPU time before preemption |
| **Trie** | Tree for string prefix storage |

---

## Final Notes

This document covers all data structures used in NexaKernel with:
- Theoretical foundations
- Implementation details
- Use cases and operations
- Complexity analysis
- Viva preparation

For hands-on experience, explore the actual source code in:
- `lib/dsa/` - Generic data structure implementations
- `kernel/memory/` - Memory management using bitmap, free list, buddy system
- `kernel/scheduler/` - Scheduling using queues and heaps
- `kernel/fs/` - File system using trees, hash maps, tries
- `kernel/ipc/` - IPC using circular buffers

Good luck with your presentation! 🎓
