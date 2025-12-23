/*
 * ===========================================================================
 * kernel/memory/memory.h
 * ===========================================================================
 *
 * Memory Management Subsystem Interface
 *
 * This header defines the public API for the kernel's memory management
 * subsystem. It provides two main components:
 *
 * 1. Physical Frame Allocator
 *    - Manages physical memory at the page (frame) level
 *    - Uses a bitmap to track which frames are free/allocated
 *    - Provides the foundation for virtual memory
 *
 * 2. Kernel Heap Allocator  
 *    - Provides dynamic memory allocation (kmalloc/kfree)
 *    - Uses a free list for efficient block management
 *    - Supports allocation, freeing, reallocation, and zeroed allocation
 *
 * Usage Order:
 *   1. Call frame_init() early in boot with memory map info
 *   2. Reserve kernel regions with frame_reserve()
 *   3. Call heap_init() with a region for the heap
 *   4. Use kmalloc/kfree for dynamic allocations
 *
 * ===========================================================================
 */

#ifndef NEXA_MEMORY_H
#define NEXA_MEMORY_H

#include "../../config/os_config.h"

/* ===========================================================================
 * PHYSICAL FRAME ALLOCATOR
 * ===========================================================================
 * Manages allocation of physical memory pages (frames). Each frame is
 * PAGE_SIZE bytes (typically 4KB). This allocator tracks which physical
 * frames are in use and provides allocation/deallocation services.
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * Initialization
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the physical frame allocator
 * 
 * @param bitmap_buffer Unused (kept for API compatibility, uses internal buffer)
 * @param mem_size      Total physical memory size in bytes
 * @param start_addr    Base address of usable memory region
 * 
 * Call this early in kernel initialization with information from the
 * multiboot memory map.
 */
void frame_init(void *bitmap_buffer, size_t mem_size, uintptr_t start_addr);

/**
 * @brief Mark a memory region as reserved (already in use)
 * 
 * @param addr Start address of region to reserve
 * @param size Size of region in bytes
 * 
 * Use this to mark kernel code, data, and hardware-reserved regions.
 */
void frame_reserve(uintptr_t addr, size_t size);

/* ---------------------------------------------------------------------------
 * Single Frame Allocation
 * --------------------------------------------------------------------------- */

/**
 * @brief Allocate a single physical frame
 * 
 * @return Physical address of allocated frame, or 0 if out of memory
 */
uintptr_t frame_alloc(void);

/**
 * @brief Allocate a specific physical frame
 * 
 * @param addr Physical address to allocate (must be page-aligned)
 * @return The same address on success, or 0 if unavailable
 */
uintptr_t frame_alloc_at(uintptr_t addr);

/**
 * @brief Free a previously allocated frame
 * 
 * @param addr Physical address of frame to free
 */
void frame_free(uintptr_t addr);

/* ---------------------------------------------------------------------------
 * Contiguous Frame Allocation
 * --------------------------------------------------------------------------- */

/**
 * @brief Allocate multiple contiguous physical frames
 * 
 * @param count Number of contiguous frames needed
 * @return Physical address of first frame, or 0 if not enough contiguous memory
 * 
 * Use this for DMA buffers or large allocations requiring physical contiguity.
 */
uintptr_t frame_alloc_contiguous(size_t count);

/**
 * @brief Free multiple contiguous frames
 * 
 * @param addr  Physical address of first frame
 * @param count Number of frames to free
 */
void frame_free_contiguous(uintptr_t addr, size_t count);

/* ---------------------------------------------------------------------------
 * Query Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Check if a frame is free
 * 
 * @param addr Physical address to check (must be page-aligned)
 * @return true if free, false if allocated or invalid
 */
bool frame_is_free(uintptr_t addr);

/**
 * @brief Check if frame allocator is initialized
 */
bool frame_is_initialized(void);

/* ---------------------------------------------------------------------------
 * Statistics
 * --------------------------------------------------------------------------- */

/** @brief Get total number of frames being managed */
size_t frame_total_count(void);

/** @brief Get number of allocated frames */
size_t frame_used_count(void);

/** @brief Get number of free frames */
size_t frame_free_count(void);

/** @brief Get total managed memory in bytes */
size_t frame_total_memory(void);

/** @brief Get used memory in bytes */
size_t frame_used_memory(void);

/** @brief Get free memory in bytes */
size_t frame_free_memory(void);

/** @brief Get the base address of managed memory */
uintptr_t frame_get_base(void);


/* ===========================================================================
 * KERNEL HEAP ALLOCATOR (kmalloc/kfree)
 * ===========================================================================
 * Provides dynamic memory allocation within the kernel. This is the kernel's
 * equivalent of the standard library's malloc/free.
 * =========================================================================== */

/* ---------------------------------------------------------------------------
 * Initialization
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the kernel heap
 * 
 * @param start Start address of heap memory region
 * @param size  Size of heap region in bytes
 * 
 * The heap region should be pre-allocated (e.g., from the frame allocator
 * or reserved in the linker script).
 */
void heap_init(void *start, size_t size);

/**
 * @brief Check if heap is initialized
 */
bool heap_is_initialized(void);

/* ---------------------------------------------------------------------------
 * Core Allocation Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Allocate memory from the kernel heap
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if allocation fails
 * 
 * The returned memory is NOT zero-initialized. Use kcalloc() for zeroed memory.
 */
void *kmalloc(size_t size);

/**
 * @brief Free previously allocated memory
 * 
 * @param ptr Pointer returned by kmalloc (NULL is safely ignored)
 */
void kfree(void *ptr);

/**
 * @brief Resize an allocation
 * 
 * @param ptr  Pointer to existing allocation (NULL = kmalloc(size))
 * @param size New size in bytes (0 = kfree(ptr))
 * @return Pointer to resized allocation, or NULL on failure
 * 
 * If the allocation is moved, the old pointer becomes invalid.
 * If realloc fails, the original allocation remains valid.
 */
void *krealloc(void *ptr, size_t size);

/**
 * @brief Allocate and zero-initialize memory
 * 
 * @param count Number of elements
 * @param size  Size of each element
 * @return Pointer to zero-initialized memory, or NULL on failure
 */
void *kcalloc(size_t count, size_t size);

/* ---------------------------------------------------------------------------
 * Aligned Allocation
 * --------------------------------------------------------------------------- */

/**
 * @brief Allocate memory with specific alignment
 * 
 * @param size      Number of bytes to allocate
 * @param alignment Required alignment (must be power of 2)
 * @return Aligned pointer, or NULL on failure
 * 
 * IMPORTANT: Must be freed with kfree_aligned(), not kfree()!
 */
void *kmalloc_aligned(size_t size, size_t alignment);

/**
 * @brief Free memory allocated with kmalloc_aligned
 * 
 * @param ptr Pointer returned by kmalloc_aligned
 */
void kfree_aligned(void *ptr);

/* ---------------------------------------------------------------------------
 * Statistics and Debugging
 * --------------------------------------------------------------------------- */

/** @brief Get total heap size */
size_t heap_total_size(void);

/** @brief Get currently allocated bytes */
size_t heap_used_size(void);

/** @brief Get currently free bytes (approximate) */
size_t heap_free_size(void);

/** @brief Get peak memory usage */
size_t heap_peak_usage(void);

/** @brief Get total allocation count */
size_t heap_allocation_count(void);

/** @brief Get total free count */
size_t heap_free_count(void);

/**
 * @brief Validate heap integrity
 * 
 * @return Number of blocks if valid, -1 if corruption detected
 * 
 * Use this for debugging memory corruption issues.
 */
int heap_validate(void);


/* ===========================================================================
 * CONVENIENCE MACROS
 * =========================================================================== */

/**
 * @brief Allocate memory for a specific type
 * 
 * Usage: struct task *t = KMALLOC(struct task);
 */
#define KMALLOC(type) ((type *)kmalloc(sizeof(type)))

/**
 * @brief Allocate zeroed memory for a specific type
 * 
 * Usage: struct task *t = KCALLOC(struct task);
 */
#define KCALLOC(type) ((type *)kcalloc(1, sizeof(type)))

/**
 * @brief Allocate an array of a specific type
 * 
 * Usage: int *arr = KMALLOC_ARRAY(int, 100);
 */
#define KMALLOC_ARRAY(type, count) ((type *)kmalloc(sizeof(type) * (count)))

/**
 * @brief Allocate a zeroed array of a specific type
 * 
 * Usage: int *arr = KCALLOC_ARRAY(int, 100);
 */
#define KCALLOC_ARRAY(type, count) ((type *)kcalloc((count), sizeof(type)))

#endif /* NEXA_MEMORY_H */
