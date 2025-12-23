/*
 * ===========================================================================
 * kernel/memory/dsa_structures/freelist.h
 * ===========================================================================
 *
 * Free List Data Structure Interface for Heap Memory Management
 *
 * This header defines the interface for a free list based memory allocator.
 * The free list is a linked list of free memory blocks that allows for
 * dynamic memory allocation within the kernel heap.
 *
 * Key Features:
 *   - First-fit and best-fit allocation strategies
 *   - Automatic block splitting for efficient space usage
 *   - Automatic coalescing to reduce fragmentation
 *   - Detailed statistics for debugging and monitoring
 *
 * Usage:
 *   char heap_memory[1024 * 1024];  // 1MB heap
 *   freelist_init(heap_memory, sizeof(heap_memory));
 *   
 *   void *ptr = freelist_alloc(256);
 *   // use ptr...
 *   freelist_free(ptr);
 *
 * ===========================================================================
 */

#ifndef NEXA_FREELIST_H
#define NEXA_FREELIST_H

#include "../../../config/os_config.h"
#include "../../../lib/dsa/list.h"

/* ---------------------------------------------------------------------------
 * Free List Block Structure
 * ---------------------------------------------------------------------------
 * This structure serves as a header for each memory block in the heap.
 * It is placed at the beginning of each block (both free and allocated).
 *
 * Memory Layout:
 *   [freelist_block_t header][usable memory region]
 * --------------------------------------------------------------------------- */
typedef struct freelist_block {
    list_node_t node;   /* Intrusive list node for linking free blocks */
    size_t size;        /* Size of usable memory (excluding this header) */
    bool is_free;       /* true if block is free, false if allocated */
} freelist_block_t;

/* ---------------------------------------------------------------------------
 * Allocator Statistics Structure
 * ---------------------------------------------------------------------------
 * Contains information about the current state of the heap allocator.
 * Useful for debugging, monitoring, and testing.
 * --------------------------------------------------------------------------- */
typedef struct freelist_stats {
    size_t total_size;          /* Total heap size in bytes */
    size_t bytes_allocated;     /* Currently allocated bytes */
    size_t bytes_free;          /* Currently free bytes (excluding headers) */
    size_t allocation_count;    /* Total number of allocations made */
    size_t free_count;          /* Total number of frees made */
    size_t free_block_count;    /* Current number of free blocks */
    size_t largest_free_block;  /* Size of largest contiguous free block */
} freelist_stats_t;

/* ---------------------------------------------------------------------------
 * Initialization
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the free list allocator
 * 
 * @param start Start address of the heap memory region
 * @param size  Total size of the heap in bytes
 * @return true on success, false if parameters are invalid
 * 
 * @note The start address should be aligned to at least 8 bytes
 * @note The size must be at least large enough for one block header
 */
bool freelist_init(void *start, size_t size);

/* ---------------------------------------------------------------------------
 * Allocation Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Allocate memory using first-fit strategy
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if allocation fails
 * 
 * First-fit finds the first block that's large enough. This is fast but
 * may lead to fragmentation at the beginning of the heap.
 */
void *freelist_alloc(size_t size);

/**
 * @brief Allocate memory using best-fit strategy
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if allocation fails
 * 
 * Best-fit finds the smallest block that's large enough. This minimizes
 * wasted space but is slower and may create many small fragments.
 */
void *freelist_alloc_best_fit(size_t size);

/**
 * @brief Free previously allocated memory
 * 
 * @param ptr Pointer returned by freelist_alloc (NULL is safely ignored)
 * 
 * @note Double-free is detected and ignored
 * @note Adjacent free blocks are automatically coalesced
 */
void freelist_free(void *ptr);

/* ---------------------------------------------------------------------------
 * Statistics and Debugging
 * --------------------------------------------------------------------------- */

/**
 * @brief Get current allocator statistics
 * 
 * @param stats Pointer to structure to fill with statistics
 */
void freelist_get_stats(freelist_stats_t *stats);

/**
 * @brief Validate heap integrity and count blocks
 * 
 * @return Number of blocks in the heap, or -1 if corruption is detected
 * 
 * This function walks through all blocks and validates their structure.
 * Useful for detecting memory corruption during debugging.
 */
int freelist_debug_dump(void);

#endif /* NEXA_FREELIST_H */
