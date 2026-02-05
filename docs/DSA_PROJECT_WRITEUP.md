# NexaKernel: Data Structures in Operating System Kernel Design

## An Interactive Visualization and Implementation Study of Core Data Structures in OS Development

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [Objectives](#2-objectives)
3. [Methodology](#3-methodology)
   - 3.1 [System Architecture](#31-system-architecture)
   - 3.2 [Data Structures Implemented](#32-data-structures-implemented)
   - 3.3 [OS Kernel Integration](#33-os-kernel-integration)
   - 3.4 [Web Visualization Platform](#34-web-visualization-platform)
   - 3.5 [Complexity Analysis](#35-complexity-analysis)
4. [Conclusion](#4-conclusion)
5. [Appendix](#5-appendix)

---

## 1. Problem Statement

### Background

Operating System kernels form the foundational layer of modern computing, managing critical resources including CPU time, physical memory, file storage, and inter-process communication. At their core, these systems rely heavily on sophisticated **Data Structures and Algorithms (DSA)** to efficiently manage hardware resources and provide abstractions to user programs.

### The Problem

Despite the fundamental role of DSA in operating systems, traditional computer science education often presents data structures in isolation, divorced from their real-world applications. Students learn about bitmaps, heaps, tries, and hash maps through abstract examples, missing the critical insight into **why** these structures were designed and **how** they solve actual engineering problems.

This creates several challenges:

1. **Conceptual Gap**: Students struggle to connect theoretical DSA knowledge with practical systems programming
2. **Visualization Difficulty**: Understanding complex pointer-based structures (trees, graphs, linked lists) through static diagrams is insufficient
3. **Integration Complexity**: Seeing how multiple data structures work together in a cohesive system is rarely demonstrated
4. **Performance Understanding**: Time and space complexity remains theoretical without concrete examples

### Our Solution

This project addresses these challenges through a **two-pronged approach**:

1. **NexaKernel**: A fully functional x86 protected-mode operating system kernel written in C and Assembly, implementing 10+ data structures in their authentic OS context
2. **NexaKernel DSA Visualizer**: An interactive web application providing step-by-step animated visualizations of each data structure as it operates within OS subsystems

---

## 2. Objectives

### Primary Objectives

| # | Objective | Deliverable |
|---|-----------|-------------|
| 1 | Implement a functional x86 OS kernel demonstrating DSA in systems programming | `boot/`, `kernel/`, `lib/` directories with working C/Assembly code |
| 2 | Create reusable, generic DSA library implementations | `lib/dsa/` with 8+ data structure implementations |
| 3 | Develop interactive visualization tool for educational purposes | `web-app/` with frontend and backend |
| 4 | Document DSA-to-OS subsystem mappings with complexity analysis | This report and inline documentation |
| 5 | Provide step-by-step animation of DSA operations in OS context | Web visualizer with playback controls |

### Secondary Objectives

- **Educational Impact**: Create a teaching resource for DSA and OS courses
- **Practical Demonstration**: Show how theoretical data structures solve real engineering problems
- **Performance Analysis**: Provide concrete complexity analysis in context
- **Open Source Contribution**: Make the project available for academic use

### Scope

The project covers the following OS subsystems and their corresponding DSA:

| OS Subsystem | Data Structures Used | Purpose |
|--------------|---------------------|---------|
| **Memory Management** | Bitmap, Free List, Buddy System | Physical frame allocation, heap management |
| **Process Scheduling** | Circular Queue, Priority Queue (Min-Heap) | Round-robin and priority-based scheduling |
| **File System** | Trie, Hash Map, N-ary Tree | Path lookup, inode caching, directory hierarchy |
| **Inter-Process Communication** | Circular Buffer (Message Queue), Graph | Message passing, process dependency tracking |

---

## 3. Methodology

### 3.1 System Architecture

The project consists of three interconnected components:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           NexaKernel Project                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────────────┐   ┌──────────────────────┐   ┌─────────────────┐  │
│  │    boot/             │   │    kernel/           │   │    lib/dsa/     │  │
│  │  ┌────────────────┐  │   │  ┌────────────────┐  │   │  ┌───────────┐  │  │
│  │  │bootloader.asm  │  │   │  │  memory/       │  │   │  │ bitmap.c  │  │  │
│  │  │gdt.asm         │  │   │  │  scheduler/    │  │   │  │ heap.c    │  │  │
│  │  │startup.asm     │  │   │  │  fs/           │  │   │  │ trie.c    │  │  │
│  │  └────────────────┘  │   │  │  ipc/          │  │   │  │ hashmap.c │  │  │
│  │   Protected Mode     │   │  │  drivers/      │  │   │  │ list.c    │  │  │
│  │   Entry Point        │   │  │  interrupts/   │  │   │  │ queue.c   │  │  │
│  └──────────────────────┘   │  └────────────────┘  │   │  │ tree.c    │  │  │
│             │               │         │            │   │  └───────────┘  │  │
│             └───────────────┴─────────┼────────────┘   │   Generic DSA   │  │
│                                       │                │   Library       │  │
│                    ┌──────────────────┘                └─────────────────┘  │
│                    ▼                                                        │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                        web-app/ (Visualization)                         ││
│  │  ┌──────────────────────────┐    ┌──────────────────────────────────┐   ││
│  │  │       backend/           │    │          frontend/               │   ││
│  │  │  ┌────────────────────┐  │    │  ┌────────────────────────────┐  │   ││
│  │  │  │ dsa/ (TypeScript)  │  │◄───┤  │ renderers/ (D3.js + SVG)   │  │   ││
│  │  │  │ engine/            │  │API │  │ core/ (State Management)   │  │   ││
│  │  │  │ routes/            │  │    │  │ styles/ (Tailwind CSS)     │  │   ││
│  │  │  └────────────────────┘  │    │  └────────────────────────────┘  │   ││
│  │  │   Simulation Engine      │    │    Interactive Visualization     │   ││
│  │  └──────────────────────────┘    └──────────────────────────────────┘   ││
│  └─────────────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Data Structures Implemented

#### 3.2.1 Bitmap (Bit Array)

**Purpose**: Physical memory frame allocation tracking

**Kernel Location**: `lib/dsa/bitmap.c` → `kernel/memory/frame_allocator.c`

**Implementation Details**:
```c
typedef struct bitmap {
    uint8_t *buffer;      // Array of bytes storing bits
    size_t size_bits;     // Total number of bits
    size_t size_bytes;    // (size_bits + 7) / 8
} bitmap_t;
```

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `bitmap_set(index)` | O(1) | Mark frame as allocated |
| `bitmap_clear(index)` | O(1) | Mark frame as free |
| `bitmap_test(index)` | O(1) | Check if frame is allocated |
| `bitmap_find_first_zero()` | O(N/8) | Find first free frame |

**OS Context**: For 256MB of physical memory with 4KB pages:
- 65,536 frames to track
- Only 8KB of memory needed (65,536 bits = 8,192 bytes)
- 99.996% space efficiency vs. storing full addresses

**Bit Manipulation**:
```c
// Set bit at index
buffer[index / 8] |= (1 << (index % 8));

// Clear bit at index  
buffer[index / 8] &= ~(1 << (index % 8));

// Test bit at index
return (buffer[index / 8] & (1 << (index % 8))) != 0;
```

---

#### 3.2.2 Free List (Linked List of Free Blocks)

**Purpose**: Kernel heap dynamic memory allocation

**Kernel Location**: `kernel/memory/dsa_structures/freelist.c` → `kernel/memory/heap_allocator.c`

**Implementation Details**:
```c
typedef struct freelist_block {
    size_t size;              // Usable size (excluding header)
    bool is_free;             // Allocation status
    list_node_t node;         // Intrusive linked list node
} freelist_block_t;
```

**Allocation Strategies**:
| Strategy | Time | Fragmentation | Use Case |
|----------|------|---------------|----------|
| First-Fit | O(n) worst | Moderate | General purpose |
| Best-Fit | O(n) always | Low internal, high external | Memory constrained |
| Worst-Fit | O(n) always | Low external | Large allocations |

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `freelist_alloc(size)` | O(n) | Find and allocate block |
| `freelist_free(ptr)` | O(n) | Free and coalesce |
| `split_block()` | O(1) | Split large block |
| `coalesce()` | O(1) | Merge adjacent free blocks |

**Memory Layout**:
```
Before allocation:
[Header|-------- Large Free Block --------]

After allocation + split:
[Header|Allocated Data][Header|Remaining Free]
```

---

#### 3.2.3 Buddy System (Binary Tree of Power-of-2 Blocks)

**Purpose**: External fragmentation-free memory allocation

**Kernel Location**: `kernel/memory/dsa_structures/buddy_tree.c`

**Implementation Details**:
```c
// Free lists for each order (0 to MAX_ORDER)
static list_t free_areas[BUDDY_MAX_ORDER + 1];

// Order N = block size of 2^N * MIN_BLOCK_SIZE
// Buddy address = block_addr XOR block_size
```

**Algorithm**:
```
Allocation (size = 20KB, MIN_BLOCK = 4KB):
1. Required order = ceil(log2(20KB/4KB)) = 3 (32KB block)
2. Find free block at order 3
3. If not found, split order 4 block into two order 3 buddies
4. Return one buddy, add other to free_areas[3]

Deallocation:
1. Calculate buddy address: buddy = addr XOR (1 << order)
2. If buddy is free, merge into order+1 block
3. Repeat until buddy is not free or max order reached
```

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `buddy_alloc(size)` | O(log N) | Allocate power-of-2 block |
| `buddy_free(addr)` | O(log N) | Free and coalesce with buddy |
| `buddy_size_to_order()` | O(log N) | Calculate required order |

**Buddy Address Calculation**:
```c
// For block at address A of order N:
buddy_address = A ^ (1 << (order + MIN_ORDER_BITS));

// Example: Block at 0x1000, order 0 (4KB)
// Buddy = 0x1000 ^ 0x1000 = 0x0000
```

---

#### 3.2.4 Priority Queue (Binary Min-Heap)

**Purpose**: Priority-based process scheduling

**Kernel Location**: `lib/dsa/heap.c` → `kernel/scheduler/dsa_structures/priority_queue.c`

**Implementation Details**:
```c
typedef struct priority_queue {
    task_t **buffer;    // Array of task pointers
    size_t capacity;    // Maximum tasks
    size_t size;        // Current count
} priority_queue_t;

// Heap property: parent.priority <= children.priority
// Array representation:
//   parent(i) = (i-1)/2
//   left(i)   = 2i + 1
//   right(i)  = 2i + 2
```

**Visual Representation**:
```
              [P=1]              Array: [P1, P2, P3, P5, P4, P6, P7]
             /     \             
          [P=2]   [P=3]          Indices:
          /   \   /   \            - Parent of i: (i-1)/2
       [P=5] [P=4][P=6][P=7]       - Left child: 2i+1
                                   - Right child: 2i+2
```

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `pq_enqueue(task)` | O(log n) | Insert + heapify up |
| `pq_dequeue()` | O(log n) | Extract min + heapify down |
| `pq_peek()` | O(1) | View highest priority |
| `heapify_up(i)` | O(log n) | Restore heap after insert |
| `heapify_down(i)` | O(log n) | Restore heap after extract |

**Heapify Up (Bubble Up)**:
```c
static void heapify_up(size_t index) {
    while (index > 0) {
        size_t p = parent(index);
        if (task_compare(buffer[p], buffer[index]) <= 0) break;
        swap(index, p);
        index = p;
    }
}
```

---

#### 3.2.5 Circular Queue (Ring Buffer)

**Purpose**: Round-robin process scheduling

**Kernel Location**: `lib/dsa/queue.c` → `kernel/scheduler/dsa_structures/round_robin_queue.c`

**Implementation Details**:
```c
typedef struct rr_queue {
    task_t **buffer;    // Fixed-size array
    size_t capacity;    // Maximum size
    size_t size;        // Current count
    size_t head;        // Front (dequeue point)
    size_t tail;        // Back (enqueue point)
} rr_queue_t;
```

**Visual Representation**:
```
Circular Queue (capacity=8, size=3):
┌───┬───┬───┬───┬───┬───┬───┬───┐
│   │   │ A │ B │ C │   │   │   │
└───┴───┴─▲─┴───┴─▲─┴───┴───┴───┘
          │       │
        head    tail

After enqueue(D):
┌───┬───┬───┬───┬───┬───┬───┬───┐
│   │   │ A │ B │ C │ D │   │   │
└───┴───┴─▲─┴───┴───┴─▲─┴───┴───┘
          │           │
        head        tail
```

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `rr_enqueue(task)` | O(1) | Add to tail |
| `rr_dequeue()` | O(1) | Remove from head |
| `rr_peek()` | O(1) | View head |
| `rr_remove(task)` | O(n) | Remove specific task |

**Wrap-around Logic**:
```c
// Enqueue
buffer[tail] = task;
tail = (tail + 1) % capacity;

// Dequeue
task = buffer[head];
head = (head + 1) % capacity;
```

---

#### 3.2.6 Hash Map (Chained Hash Table)

**Purpose**: File descriptor table, inode caching

**Kernel Location**: `lib/dsa/hashmap.c` → `kernel/fs/dsa_structures/hashmap.c`

**Implementation Details**:
```c
typedef struct hashmap_entry {
    char *key;                    // String key
    void *value;                  // Associated value
    struct hashmap_entry *next;   // Collision chain
} hashmap_entry_t;

typedef struct hashmap {
    hashmap_entry_t **buckets;    // Array of chains
    size_t bucket_count;          // Number of buckets
    size_t size;                  // Total entries
} hashmap_t;
```

**DJB2 Hash Function**:
```c
uint32_t hashmap_hash_string(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}
```

**Key Operations**:
| Operation | Average | Worst | Description |
|-----------|---------|-------|-------------|
| `hashmap_put(k,v)` | O(1) | O(n) | Insert/update |
| `hashmap_get(k)` | O(1) | O(n) | Retrieve value |
| `hashmap_remove(k)` | O(1) | O(n) | Delete entry |

**Collision Handling**: Separate chaining with linked lists

---

#### 3.2.7 Trie (Prefix Tree)

**Purpose**: File path lookup and indexing

**Kernel Location**: `lib/dsa/trie.c` → `kernel/fs/dsa_structures/trie.c`

**Implementation Details**:
```c
#define TRIE_ALPHABET_SIZE 256  // Full ASCII support

typedef struct trie_node {
    struct trie_node *children[TRIE_ALPHABET_SIZE];
    void *data;                   // Associated data (inode pointer)
    bool is_terminal;             // End of a valid key
} trie_node_t;
```

**Visual Representation** (File paths /bin, /boot, /etc):
```
         [root]
        /   |   \
      'b'  'e'  ...
       |    |
     [in] [tc]
     / \    |
   'i' 'o' '\0' → inode(/etc)
    |   |
  '\0' 'o'
    |   |
   →i  't'
        |
       '\0' → inode(/boot)
```

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `trie_insert(path, inode)` | O(L) | Add path mapping |
| `trie_search(path)` | O(L) | Find inode by path |
| `trie_remove(path)` | O(L) | Delete path mapping |

Where L = length of the path string

**Advantages for File Systems**:
- Prefix-based lookups for directory listing
- No hash collisions
- Natural alphabetical ordering
- Efficient for paths with common prefixes

---

#### 3.2.8 N-ary Tree (General Tree)

**Purpose**: Directory hierarchy representation

**Kernel Location**: `lib/dsa/tree.c` → `kernel/fs/ramfs.c`

**Implementation Details**:
```c
typedef struct tree_node {
    void *data;                     // Inode pointer
    struct tree_node *parent;       // Parent directory
    struct tree_node *first_child;  // First child (leftmost)
    struct tree_node *next_sibling; // Next sibling
} tree_node_t;
```

**Child-Sibling Representation**:
```
Directory tree:         Internal representation:
      /                       [/]
    / | \                      |
  bin etc home              [bin] → [etc] → [home]
      |                               |
    passwd                        [passwd]
```

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `tree_add_child()` | O(1) | Add file/directory |
| `tree_remove_child()` | O(children) | Remove entry |
| `tree_find_child()` | O(children) | Search by name |

---

#### 3.2.9 Doubly Linked List

**Purpose**: Free block list, task queues, general-purpose

**Kernel Location**: `lib/dsa/list.c`

**Implementation Details** (Intrusive Design):
```c
typedef struct list_node {
    struct list_node *next;
    struct list_node *prev;
} list_node_t;

typedef struct list {
    list_node_t *head;
    list_node_t *tail;
    size_t size;
} list_t;

// Usage: embed list_node_t in your structure
typedef struct my_struct {
    int data;
    list_node_t node;  // Embedded, not pointer
} my_struct_t;

// Get container from node:
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

**Intrusive vs Non-Intrusive**:
| Aspect | Intrusive | Non-Intrusive |
|--------|-----------|---------------|
| Memory | No extra allocation | Node + data allocation |
| Cache | Better locality | Pointer chasing |
| Flexibility | One list per node | Multiple lists possible |
| Complexity | Slightly higher API | Simpler API |

---

#### 3.2.10 Message Queue (Circular Buffer)

**Purpose**: Inter-Process Communication

**Kernel Location**: `kernel/ipc/message_queue.c`

**Implementation Details**:
```c
typedef struct {
    uint32_t sender_pid;
    uint32_t type;           // Message type filter
    size_t size;
    uint8_t data[MAX_MSG_SIZE];
} message_t;

typedef struct {
    bool valid;
    uint32_t key;            // Queue identifier
    message_t messages[MAX_MESSAGES];
    size_t head, tail, count;
} msgq_t;
```

**Key Operations**:
| Operation | Complexity | Description |
|-----------|------------|-------------|
| `msgq_send()` | O(1) | Send message to queue |
| `msgq_receive()` | O(n) | Receive (optionally by type) |
| `msgq_create()` | O(1) | Create new queue |

---

### 3.3 OS Kernel Integration

#### Boot Sequence
```
1. GRUB loads kernel at 0x100000 (1MB mark)
2. bootloader.asm: Verify Multiboot, load GDT, set up stack
3. kernel_main(): Initialize subsystems in dependency order
   └─→ VGA Console (for output)
   └─→ Frame Allocator (bitmap for physical memory)
   └─→ Heap Allocator (free list for kmalloc/kfree)
   └─→ IDT + ISR + IRQ (interrupt handling)
   └─→ Timer Driver (PIT for scheduling ticks)
   └─→ Keyboard Driver (PS/2 input)
   └─→ Scheduler (round-robin + priority queues)
   └─→ File System (trie + tree + hashmap)
   └─→ IPC (message queues)
```

#### Subsystem Dependencies
```
┌────────────────────────────────────────────────────────────────┐
│                      Application Layer                         │
├────────────────────────────────────────────────────────────────┤
│                   System Call Interface                        │
├───────────────┬───────────────┬───────────────┬────────────────┤
│   Scheduler   │  File System  │     IPC       │   Drivers      │
│  ┌─────────┐  │  ┌─────────┐  │  ┌─────────┐  │  ┌──────────┐  │
│  │Priority │  │  │  Trie   │  │  │Msg Queue│  │  │  Timer   │  │
│  │ Queue   │  │  │ HashMap │  │  │(Circ Buf│  │  │ Keyboard │  │
│  │Circ Que │  │  │ N-ary   │  │  │         │  │  │   VGA    │  │
│  └─────────┘  │  └─────────┘  │  └─────────┘  │  └──────────┘  │
├───────────────┴───────────────┴───────────────┴────────────────┤
│                    Memory Management                           │
│         ┌───────────────┐   ┌───────────────┐                  │
│         │Frame Allocator│   │Heap Allocator │                  │
│         │   (Bitmap)    │   │  (Free List)  │                  │
│         └───────────────┘   └───────────────┘                  │
├────────────────────────────────────────────────────────────────┤
│                  Interrupt Handling (IDT/PIC)                  │
├────────────────────────────────────────────────────────────────┤
│                    Hardware Abstraction                        │
└────────────────────────────────────────────────────────────────┘
```

### 3.4 Web Visualization Platform

#### Architecture
```
Frontend (Vite + TypeScript + D3.js)     Backend (Node.js + Express + TypeScript)
┌────────────────────────────────┐       ┌────────────────────────────────┐
│  home.html                     │       │  server.ts                     │
│  └── DSA Card Grid             │       │  └── Express middleware        │
│                                │       │                                │
│  dsa/*.html (10 pages)         │ REST  │  routes/simulate.ts            │
│  └── Visualization Canvas      │◄─────►│  └── /api/simulate/* endpoints │
│  └── Operation Controls        │  API  │                                │
│  └── Playback Controls         │       │  engine/                       │
│                                │       │  └── simulationManager.ts      │
│  src/renderers/                │       │  └── stepGenerator.ts          │
│  └── D3.js SVG rendering       │       │                                │
│  └── Per-DSA color themes      │       │  dsa/ (10 implementations)     │
│                                │       │  └── Step-by-step generators   │
│  src/core/                     │       │                                │
│  └── State management          │       │                                │
│  └── API client                │       │                                │
└────────────────────────────────┘       └────────────────────────────────┘
```

#### Visualization Features

| Feature | Implementation | Purpose |
|---------|----------------|---------|
| **Step-by-Step Animation** | Backend generates `Step[]` array | Show algorithm progression |
| **Playback Controls** | Play/Pause/Step/Speed | User-controlled learning pace |
| **Color Themes** | Per-DSA unique palette | Visual distinction |
| **OS Context Panel** | Static content + dynamic state | Real-world connection |
| **Pseudocode Display** | Prism.js syntax highlighting | Algorithm reference |
| **Keyboard Shortcuts** | Space, Arrow keys | Quick navigation |

#### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/simulate/create` | POST | Create new simulation session |
| `/api/simulate/step` | POST | Execute operation, get steps |
| `/api/simulate/reset` | POST | Reset session state |
| `/api/simulate/types` | GET | List supported DSA types |
| `/api/health` | GET | Server health check |

### 3.5 Complexity Analysis Summary

| Data Structure | Insert | Delete | Search | Space | OS Use Case |
|----------------|--------|--------|--------|-------|-------------|
| **Bitmap** | O(1) | O(1) | O(1) | O(n/8) | Frame allocation |
| **Free List** | O(n) | O(n) | O(n) | O(1) per block | Heap allocation |
| **Buddy System** | O(log n) | O(log n) | - | O(2n) | Memory allocation |
| **Priority Queue** | O(log n) | O(log n) | O(1) peek | O(n) | Priority scheduling |
| **Circular Queue** | O(1) | O(1) | O(n) | O(capacity) | Round-robin scheduling |
| **Hash Map** | O(1) avg | O(1) avg | O(1) avg | O(n + buckets) | File descriptor table |
| **Trie** | O(L) | O(L) | O(L) | O(ALPHABET × keys × L) | Path indexing |
| **N-ary Tree** | O(1) | O(children) | O(children) | O(n) | Directory hierarchy |
| **Linked List** | O(1) | O(1) given node | O(n) | O(n) | General purpose |
| **Message Queue** | O(1) | O(1)/O(n)* | - | O(capacity) | IPC |

*O(n) when filtering by message type

---

## 4. Conclusion

### Achievements

This project successfully demonstrates:

1. **Practical DSA Application**: Implemented 10+ data structures within a working x86 OS kernel, showing their authentic use cases in memory management, scheduling, file systems, and IPC.

2. **Educational Value**: Created an interactive visualization platform that transforms abstract DSA concepts into tangible, animated demonstrations with OS context.

3. **Engineering Depth**: Built a complete boot-to-userland system including:
   - Multiboot-compliant bootloader
   - Protected mode initialization
   - Physical and virtual memory management
   - Preemptive multitasking scheduler
   - In-memory file system
   - Device drivers (VGA, timer, keyboard)

4. **Visualization Innovation**: Developed a web-based tool with:
   - 10 unique DSA visualizations
   - Step-by-step animated operations
   - Playback controls for self-paced learning
   - OS-contextualized explanations

### Key Insights

| Insight | Details |
|---------|---------|
| **DSA Choice Matters** | Bitmap for frame allocation achieves 99.996% space efficiency vs. storing addresses |
| **Trade-offs Are Real** | First-fit allocation is faster but fragments more than best-fit |
| **Complexity Is Contextual** | O(log n) heap operations are worthwhile for priority scheduling fairness |
| **Integration Is Key** | Data structures don't exist in isolation—frame allocator enables heap, heap enables everything else |

### Future Work

1. **Virtual Memory**: Implement paging with page tables (tree/trie-based mapping)
2. **Disk File System**: Add persistent storage with B-tree indexing
3. **Network Stack**: Implement TCP/IP with ring buffers and hash tables
4. **Enhanced Visualizer**: Add multi-DSA comparison views and performance benchmarks

### Impact

This project bridges the gap between theoretical DSA education and systems programming practice. By seeing how bitmaps track memory, how heaps prioritize processes, and how tries index files, students gain appreciation for **why** these structures were invented and **how** to choose the right tool for each problem.

---

## 5. Appendix

### A. Build Instructions

```bash
# Build NexaKernel
cd /path/to/NexaKernel
make clean && make

# Run in QEMU
make run

# Debug with GDB
make debug
# In another terminal: gdb -x scripts/debug_gdb.sh
```

### B. Web Visualizer Setup

```bash
# Backend
cd web-app/backend
npm install
npm run dev  # Runs on http://localhost:3001

# Frontend
cd ../frontend
npm install
npm run dev  # Runs on http://localhost:5173

# Open http://localhost:5173/home.html
```

### C. Project Structure

```
NexaKernel/
├── boot/                    # Assembly bootloader
├── kernel/                  # C kernel code
│   ├── memory/             # Frame allocator, heap
│   ├── scheduler/          # Task management
│   ├── fs/                 # File system
│   ├── ipc/                # Inter-process communication
│   ├── drivers/            # Hardware drivers
│   └── interrupts/         # IDT, ISR, IRQ
├── lib/
│   ├── dsa/                # Generic DSA library
│   └── cstd/               # C standard library
├── web-app/
│   ├── backend/            # Node.js simulation engine
│   └── frontend/           # D3.js visualization
├── config/                 # Build configuration
└── docs/                   # Documentation
```

### D. References

1. Tanenbaum, A. S., & Bos, H. (2014). *Modern Operating Systems* (4th ed.). Pearson.
2. Cormen, T. H., et al. (2009). *Introduction to Algorithms* (3rd ed.). MIT Press.
3. OSDev Wiki. (n.d.). https://wiki.osdev.org/
4. Intel. (2021). *Intel 64 and IA-32 Architectures Software Developer's Manual*.

---

*Document prepared for DSA Course Project Evaluation*

*Project Repository*: [github.com/varunaditya27/NexaKernel](https://github.com/varunaditya27/NexaKernel)

*Visualization Tool*: [github.com/Tanisha-27-12/NexaKernel-DSA-Visualizer](https://github.com/Tanisha-27-12/NexaKernel-DSA-Visualizer)
