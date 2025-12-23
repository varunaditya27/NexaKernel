/*
 * ===========================================================================
 * kernel/memory/dsa_structures/buddy.h
 * ===========================================================================
 *
 * Buddy System Allocator Interface
 *
 * The buddy system is a memory allocation algorithm that divides memory into
 * partitions of power-of-two sizes. This approach balances allocation speed
 * with external fragmentation reduction.
 *
 * How it works:
 *   1. Memory is organized into "orders" where order N has blocks of size
 *      2^N * MIN_BLOCK_SIZE
 *   2. When allocating, we find a free block of the appropriate order
 *   3. If no block of the right size exists, we split a larger block
 *   4. When freeing, we check if the "buddy" (adjacent block of same size)
 *      is also free; if so, we merge them
 *
 * Example (MIN_BLOCK_SIZE = 4KB):
 *   Order 0: 4KB blocks
 *   Order 1: 8KB blocks
 *   Order 2: 16KB blocks
 *   ...
 *   Order 10: 4MB blocks
 *
 * Advantages:
 *   - Fast allocation: O(log N) where N is max memory size
 *   - Fast deallocation: O(log N)
 *   - Low external fragmentation due to coalescing
 *   - Simple to implement
 *
 * Disadvantages:
 *   - Internal fragmentation (allocations rounded up to power of 2)
 *   - Memory overhead for tracking blocks
 *
 * ===========================================================================
 */

#ifndef NEXA_BUDDY_H
#define NEXA_BUDDY_H

#include "../../../config/os_config.h"
#include "../../../lib/dsa/list.h"

/* ---------------------------------------------------------------------------
 * Configuration Constants
 * --------------------------------------------------------------------------- */

/* Maximum order (order N = blocks of size 2^N * MIN_BLOCK_SIZE) */
#define BUDDY_MAX_ORDER     10

/* Minimum block size (must be power of 2, typically one page) */
#define BUDDY_MIN_BLOCK_SIZE    PAGE_SIZE   /* 4KB */

/* Maximum block size = 2^MAX_ORDER * MIN_BLOCK_SIZE = 4MB with defaults */
#define BUDDY_MAX_BLOCK_SIZE    (BUDDY_MIN_BLOCK_SIZE << BUDDY_MAX_ORDER)

/* ---------------------------------------------------------------------------
 * Buddy Block Structure
 * ---------------------------------------------------------------------------
 * This structure is placed at the beginning of each free block.
 * Allocated blocks don't need headers (we track them via the bitmap).
 * --------------------------------------------------------------------------- */
typedef struct buddy_block {
    list_node_t node;   /* Intrusive list node for free list */
    int order;          /* Block's order (size = 2^order * MIN_BLOCK_SIZE) */
} buddy_block_t;

/* ---------------------------------------------------------------------------
 * Buddy Allocator Statistics
 * --------------------------------------------------------------------------- */
typedef struct buddy_stats {
    size_t total_memory;        /* Total managed memory */
    size_t free_memory;         /* Currently free memory */
    size_t allocated_memory;    /* Currently allocated memory */
    size_t total_allocations;   /* Total allocation count */
    size_t total_frees;         /* Total free count */
    size_t blocks_per_order[BUDDY_MAX_ORDER + 1];  /* Free blocks per order */
} buddy_stats_t;

/* ---------------------------------------------------------------------------
 * Initialization
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the buddy allocator
 * 
 * @param start      Start address of memory region (should be page-aligned)
 * @param total_size Total size of memory region to manage
 * @return true on success, false on failure
 * 
 * @note The memory region should be aligned to BUDDY_MAX_BLOCK_SIZE for best results
 * @note Some memory is reserved for allocator metadata
 */
bool buddy_init(void *start, size_t total_size);

/* ---------------------------------------------------------------------------
 * Allocation Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Allocate memory from the buddy system
 * 
 * @param size Number of bytes needed
 * @return Pointer to allocated memory, or NULL if allocation fails
 * 
 * @note Actual allocation will be rounded up to nearest power of 2 >= MIN_BLOCK_SIZE
 */
void *buddy_alloc(size_t size);

/**
 * @brief Allocate blocks of a specific order
 * 
 * @param order Block order (0 = MIN_BLOCK_SIZE, 1 = 2*MIN_BLOCK_SIZE, etc.)
 * @return Pointer to allocated block, or NULL if allocation fails
 */
void *buddy_alloc_order(int order);

/**
 * @brief Free previously allocated memory
 * 
 * @param ptr  Pointer returned by buddy_alloc
 * @param size Original size requested (needed to determine order)
 * 
 * @note The size parameter is required because allocated blocks don't store
 *       their size - the caller must remember it
 */
void buddy_free(void *ptr, size_t size);

/**
 * @brief Free a block of specific order
 * 
 * @param ptr   Pointer to block
 * @param order Order of the block
 */
void buddy_free_order(void *ptr, int order);

/* ---------------------------------------------------------------------------
 * Utility Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Calculate the order needed for a given size
 * 
 * @param size Requested allocation size
 * @return Order number, or -1 if size is too large
 */
int buddy_size_to_order(size_t size);

/**
 * @brief Calculate the block size for a given order
 * 
 * @param order Block order
 * @return Block size in bytes
 */
size_t buddy_order_to_size(int order);

/**
 * @brief Get current allocator statistics
 * 
 * @param stats Pointer to structure to fill with statistics
 */
void buddy_get_stats(buddy_stats_t *stats);

/**
 * @brief Check if the buddy allocator is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool buddy_is_initialized(void);

#endif /* NEXA_BUDDY_H */
