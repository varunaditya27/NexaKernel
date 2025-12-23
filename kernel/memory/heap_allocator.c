#include <lib/dsa/list.h>
#include <stddef.h>
#include <stdint.h>

/*
 * ===========================================================================
 * kernel/memory/heap_allocator.c
 * ===========================================================================
 *
 * Kernel Heap Allocator (kmalloc/kfree)
 *
 * This module provides dynamic memory allocation for the kernel using the
 * classic malloc/free interface. It manages a heap region and provides
 * the kmalloc() and kfree() functions that kernel code uses for runtime
 * memory allocation.
 *
 * Design:
 *   - Uses a free list data structure (linked list of free blocks)
 *   - Each block has a header with size and status information
 *   - First-fit allocation strategy (fast, good enough for kernel use)
 *   - Automatic block splitting when allocating from large blocks
 *   - Automatic coalescing of adjacent free blocks on free
 *
 * Memory Layout:
 *   [Block Header][Usable Data][Block Header][Usable Data]...
 *
 * Block Header:
 *   - size: Size of usable data area (not including header)
 *   - is_free: Whether this block is available
 *   - prev/next: Links for the free list (when free) or validation (when allocated)
 *
 * Alignment:
 *   - All allocations are aligned to 8 bytes for performance
 *   - This ensures proper alignment for all basic types
 *
 * Usage:
 *   void *ptr = kmalloc(256);  // Allocate 256 bytes
 *   // use memory...
 *   kfree(ptr);                // Release the memory
 *
 * Thread Safety:
 *   This implementation is NOT thread-safe. In a multi-threaded kernel,
 *   callers must hold a lock when calling kmalloc/kfree.
 *
 * ===========================================================================
 */

#include "memory.h"
#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------- */

/* Minimum allocation size (prevents excessive fragmentation) */
#define HEAP_MIN_ALLOC_SIZE     16

/* Alignment for all allocations (must be power of 2) */
#define HEAP_ALIGNMENT          8

/* Minimum size to consider splitting a block */
#define HEAP_MIN_SPLIT_SIZE     (sizeof(heap_block_t) + HEAP_MIN_ALLOC_SIZE)

/* Magic number for detecting corruption */
#define HEAP_MAGIC              0xDEADBEEF

/* ---------------------------------------------------------------------------
 * Block Header Structure
 * ---------------------------------------------------------------------------
 * This structure is placed at the beginning of every block (free or allocated).
 * It allows us to:
 *   - Navigate between blocks
 *   - Determine if a block is free
 *   - Validate block integrity
 * --------------------------------------------------------------------------- */
typedef struct heap_block {
    uint32_t magic;             /* Magic number for corruption detection */
    size_t size;                /* Size of usable data (excluding header) */
    bool is_free;               /* true if block is available */
    struct heap_block *prev;    /* Previous block in memory (for coalescing) */
    struct heap_block *next;    /* Next block in memory (for coalescing) */
} heap_block_t;

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Heap boundaries */
static void *heap_start = NULL;
static void *heap_end = NULL;
static size_t heap_size = 0;

/* First block in the heap */
static heap_block_t *first_block = NULL;

/* Statistics */
static size_t total_allocations = 0;
static size_t total_frees = 0;
static size_t bytes_allocated = 0;
static size_t peak_usage = 0;

/* Initialization flag */
static bool heap_initialized = false;

/* ---------------------------------------------------------------------------
 * Helper: Align a size up to HEAP_ALIGNMENT
 * --------------------------------------------------------------------------- */
static inline size_t align_size(size_t size)
{
    return (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
}

/* ---------------------------------------------------------------------------
 * Helper: Get the data pointer from a block
 * --------------------------------------------------------------------------- */
static inline void *block_to_data(heap_block_t *block)
{
    return (void *)((char *)block + sizeof(heap_block_t));
}

/* ---------------------------------------------------------------------------
 * Helper: Get the block from a data pointer
 * --------------------------------------------------------------------------- */
static inline heap_block_t *data_to_block(void *data)
{
    return (heap_block_t *)((char *)data - sizeof(heap_block_t));
}

/* ---------------------------------------------------------------------------
 * Helper: Validate a block's magic number
 * --------------------------------------------------------------------------- */
static inline bool is_valid_block(heap_block_t *block)
{
    return block && block->magic == HEAP_MAGIC;
}

/* ---------------------------------------------------------------------------
 * Helper: Calculate the end address of a block (including data)
 * --------------------------------------------------------------------------- */
static inline void *block_end(heap_block_t *block)
{
    return (void *)((char *)block + sizeof(heap_block_t) + block->size);
}

/* ---------------------------------------------------------------------------
 * heap_init - Initialize the kernel heap allocator
 * ---------------------------------------------------------------------------
 * Parameters:
 *   start - Start address of heap memory region
 *   size  - Size of heap region in bytes
 *
 * The heap region must be pre-allocated (typically from the frame allocator
 * or reserved in the memory layout). This function sets up the initial
 * free block spanning the entire heap.
 * --------------------------------------------------------------------------- */
void heap_init(void *start, size_t size)
{
    /* Validate parameters */
    if (!start || size < sizeof(heap_block_t) + HEAP_MIN_ALLOC_SIZE) {
        return;
    }

    /* Align the start address */
    uintptr_t aligned_start = ALIGN_UP((uintptr_t)start, HEAP_ALIGNMENT);
    size_t lost = aligned_start - (uintptr_t)start;
    
    if (lost >= size) {
        return;  /* Not enough space after alignment */
    }

    size -= lost;
    start = (void *)aligned_start;

    /* Store heap boundaries */
    heap_start = start;
    heap_size = size;
    heap_end = (void *)((char *)start + size);

    /* Create the initial free block */
    first_block = (heap_block_t *)start;
    first_block->magic = HEAP_MAGIC;
    first_block->size = size - sizeof(heap_block_t);
    first_block->is_free = true;
    first_block->prev = NULL;
    first_block->next = NULL;

    /* Reset statistics */
    total_allocations = 0;
    total_frees = 0;
    bytes_allocated = 0;
    peak_usage = 0;

    heap_initialized = true;
}

/* ---------------------------------------------------------------------------
 * split_block - Split a block if it's larger than needed
 * ---------------------------------------------------------------------------
 * If the block is significantly larger than the requested size, split it
 * into two blocks: one for the allocation and one free block with the
 * remainder.
 *
 * Parameters:
 *   block        - Block to potentially split
 *   needed_size  - Size actually needed (aligned)
 * --------------------------------------------------------------------------- */
static void split_block(heap_block_t *block, size_t needed_size)
{
    /* Only split if the remainder is large enough to be useful */
    if (block->size <= needed_size + HEAP_MIN_SPLIT_SIZE) {
        return;  /* Don't split - would create too small a fragment */
    }

    /* Calculate where the new block starts */
    heap_block_t *new_block = (heap_block_t *)((char *)block + sizeof(heap_block_t) + needed_size);

    /* Initialize the new block */
    new_block->magic = HEAP_MAGIC;
    new_block->size = block->size - needed_size - sizeof(heap_block_t);
    new_block->is_free = true;
    new_block->prev = block;
    new_block->next = block->next;

    /* Update the next block's prev pointer */
    if (block->next) {
        block->next->prev = new_block;
    }

    /* Update the original block */
    block->size = needed_size;
    block->next = new_block;
}

/* ---------------------------------------------------------------------------
 * kmalloc - Allocate memory from the kernel heap
 * ---------------------------------------------------------------------------
 * Parameters:
 *   size - Number of bytes to allocate
 *
 * Returns:
 *   Pointer to allocated memory, or NULL if allocation fails
 *
 * This function uses first-fit allocation: it finds the first free block
 * that is large enough to satisfy the request. This is fast but may lead
 * to fragmentation over time.
 * --------------------------------------------------------------------------- */
void *kmalloc(size_t size)
{
    if (!heap_initialized || size == 0) {
        return NULL;
    }

    /* Align the requested size */
    size = align_size(size);

    /* Enforce minimum allocation size */
    if (size < HEAP_MIN_ALLOC_SIZE) {
        size = HEAP_MIN_ALLOC_SIZE;
    }

    /* First-fit search: find the first block that fits */
    heap_block_t *block = first_block;
    
    while (block) {
        /* Skip invalid blocks (corruption detection) */
        if (!is_valid_block(block)) {
            /* Heap corruption detected! */
            PANIC("Heap corruption detected in kmalloc");
            return NULL;
        }

        /* Check if this block is free and large enough */
        if (block->is_free && block->size >= size) {
            /* Found a suitable block */
            
            /* Try to split if the block is much larger than needed */
            split_block(block, size);

            /* Mark as allocated */
            block->is_free = false;

            /* Update statistics */
            total_allocations++;
            bytes_allocated += block->size;
            if (bytes_allocated > peak_usage) {
                peak_usage = bytes_allocated;
            }

            /* Return pointer to usable data area */
            return block_to_data(block);
        }

        block = block->next;
    }

    /* No suitable block found */
    return NULL;
}

/* ---------------------------------------------------------------------------
 * kmalloc_aligned - Allocate memory with specific alignment
 * ---------------------------------------------------------------------------
 * Parameters:
 *   size      - Number of bytes to allocate
 *   alignment - Required alignment (must be power of 2)
 *
 * Returns:
 *   Aligned pointer, or NULL if allocation fails
 *
 * Note: This wastes some memory for alignment padding.
 * --------------------------------------------------------------------------- */
void *kmalloc_aligned(size_t size, size_t alignment)
{
    if (!heap_initialized || size == 0 || alignment == 0) {
        return NULL;
    }

    /* Ensure alignment is a power of 2 */
    if (alignment & (alignment - 1)) {
        return NULL;  /* Not a power of 2 */
    }

    /* Allocate extra space for alignment */
    size_t total_size = size + alignment - 1 + sizeof(void *);
    void *raw_ptr = kmalloc(total_size);
    
    if (!raw_ptr) {
        return NULL;
    }

    /* Calculate aligned address */
    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + sizeof(void *) + alignment - 1) & ~(alignment - 1);

    /* Store the original pointer just before the aligned address */
    *((void **)(aligned_addr - sizeof(void *))) = raw_ptr;

    return (void *)aligned_addr;
}

/* ---------------------------------------------------------------------------
 * kfree_aligned - Free memory allocated with kmalloc_aligned
 * --------------------------------------------------------------------------- */
void kfree_aligned(void *ptr)
{
    if (!ptr) {
        return;
    }

    /* Retrieve the original pointer */
    void *raw_ptr = *((void **)((uintptr_t)ptr - sizeof(void *)));
    kfree(raw_ptr);
}

/* ---------------------------------------------------------------------------
 * coalesce_forward - Merge a block with the next block if both are free
 * --------------------------------------------------------------------------- */
static void coalesce_forward(heap_block_t *block)
{
    if (!block->next || !block->next->is_free) {
        return;  /* Can't merge */
    }

    if (!is_valid_block(block->next)) {
        return;  /* Next block is corrupt */
    }

    heap_block_t *next = block->next;

    /* Absorb the next block */
    block->size += sizeof(heap_block_t) + next->size;
    block->next = next->next;

    /* Update the block after next */
    if (next->next) {
        next->next->prev = block;
    }

    /* Invalidate the absorbed block's magic (optional, for debugging) */
    next->magic = 0;
}

/* ---------------------------------------------------------------------------
 * coalesce_backward - Merge a block with the previous block if both are free
 * --------------------------------------------------------------------------- */
static void coalesce_backward(heap_block_t *block)
{
    if (!block->prev || !block->prev->is_free) {
        return;  /* Can't merge */
    }

    if (!is_valid_block(block->prev)) {
        return;  /* Previous block is corrupt */
    }

    /* Let the previous block absorb this one */
    coalesce_forward(block->prev);
}

/* ---------------------------------------------------------------------------
 * kfree - Free previously allocated memory
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ptr - Pointer returned by kmalloc (NULL is safely ignored)
 *
 * This function marks the block as free and attempts to merge it with
 * adjacent free blocks to reduce fragmentation.
 * --------------------------------------------------------------------------- */
void kfree(void *ptr)
{
    /* Freeing NULL is always safe */
    if (!ptr || !heap_initialized) {
        return;
    }

    /* Validate the pointer is within our heap */
    if (ptr < heap_start || ptr >= heap_end) {
        return;  /* Pointer not from our heap */
    }

    /* Get the block header */
    heap_block_t *block = data_to_block(ptr);

    /* Validate the block */
    if (!is_valid_block(block)) {
        /* Invalid block - could be double-free or corruption */
        PANIC("Invalid block in kfree (possible double-free or corruption)");
        return;
    }

    /* Check for double-free */
    if (block->is_free) {
        /* Already free - this is a bug in the calling code */
        return;
    }

    /* Mark as free */
    block->is_free = true;

    /* Update statistics */
    total_frees++;
    bytes_allocated -= block->size;

    /* Try to coalesce with adjacent free blocks */
    coalesce_forward(block);
    coalesce_backward(block);
}

/* ---------------------------------------------------------------------------
 * krealloc - Resize an allocation
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ptr  - Pointer to existing allocation (NULL = kmalloc)
 *   size - New size (0 = kfree)
 *
 * Returns:
 *   Pointer to resized allocation, or NULL on failure
 *
 * Note: The returned pointer may be different from the input pointer.
 * --------------------------------------------------------------------------- */
void *krealloc(void *ptr, size_t size)
{
    /* NULL ptr = just allocate */
    if (!ptr) {
        return kmalloc(size);
    }

    /* Size 0 = free */
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    /* Get current block */
    heap_block_t *block = data_to_block(ptr);
    
    if (!is_valid_block(block)) {
        return NULL;  /* Invalid block */
    }

    /* If current block is large enough, just return it */
    size = align_size(size);
    if (block->size >= size) {
        /* Could split here if block is much larger, but skip for simplicity */
        return ptr;
    }

    /* Need to allocate a larger block */
    void *new_ptr = kmalloc(size);
    
    if (!new_ptr) {
        return NULL;  /* Allocation failed - original still valid */
    }

    /* Copy data to new location */
    /* Note: We'd use memcpy here, but implementing inline for freestanding */
    char *src = (char *)ptr;
    char *dst = (char *)new_ptr;
    size_t copy_size = block->size;  /* Copy old size worth of data */
    
    for (size_t i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }

    /* Free the old block */
    kfree(ptr);

    return new_ptr;
}

/* ---------------------------------------------------------------------------
 * kcalloc - Allocate and zero-initialize memory
 * ---------------------------------------------------------------------------
 * Parameters:
 *   count - Number of elements
 *   size  - Size of each element
 *
 * Returns:
 *   Pointer to zero-initialized memory, or NULL on failure
 * --------------------------------------------------------------------------- */
void *kcalloc(size_t count, size_t size)
{
    /* Check for overflow */
    size_t total = count * size;
    if (count != 0 && total / count != size) {
        return NULL;  /* Overflow */
    }

    void *ptr = kmalloc(total);
    
    if (ptr) {
        /* Zero the memory */
        char *p = (char *)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }

    return ptr;
}

/* ---------------------------------------------------------------------------
 * Statistics and Debugging Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Get total heap size
 */
size_t heap_total_size(void)
{
    return heap_size;
}

/**
 * @brief Get currently allocated bytes
 */
size_t heap_used_size(void)
{
    return bytes_allocated;
}

/**
 * @brief Get currently free bytes (approximate)
 */
size_t heap_free_size(void)
{
    return heap_size - bytes_allocated - 
           ((total_allocations - total_frees) * sizeof(heap_block_t));
}

/**
 * @brief Get peak memory usage
 */
size_t heap_peak_usage(void)
{
    return peak_usage;
}

/**
 * @brief Get allocation count
 */
size_t heap_allocation_count(void)
{
    return total_allocations;
}

/**
 * @brief Get free count
 */
size_t heap_free_count(void)
{
    return total_frees;
}

/**
 * @brief Check if heap is initialized
 */
bool heap_is_initialized(void)
{
    return heap_initialized;
}

/**
 * @brief Validate heap integrity (for debugging)
 * 
 * Walks through all blocks and verifies magic numbers and linkage.
 * Returns the number of blocks if valid, or -1 if corruption is detected.
 */
int heap_validate(void)
{
    if (!heap_initialized) {
        return -1;
    }

    int block_count = 0;
    heap_block_t *block = first_block;
    heap_block_t *prev = NULL;

    while (block) {
        /* Check magic number */
        if (!is_valid_block(block)) {
            return -1;  /* Corruption */
        }

        /* Check linkage */
        if (block->prev != prev) {
            return -1;  /* Linkage error */
        }

        /* Check bounds */
        if (block_end(block) > heap_end) {
            return -1;  /* Block extends past heap end */
        }

        block_count++;
        prev = block;
        block = block->next;

        /* Sanity check to prevent infinite loops */
        if (block_count > 1000000) {
            return -1;  /* Probable infinite loop */
        }
    }

    return block_count;
}
