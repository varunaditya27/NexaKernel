/*
 * ===========================================================================
 * kernel/memory/dsa_structures/freelist.c
 * ===========================================================================
 *
 * Free List Data Structure for Heap Memory Management
 *
 * This file implements a free list - a linked list of free memory blocks
 * that forms the backbone of the kernel heap allocator. The free list
 * enables efficient dynamic memory allocation within the kernel.
 *
 * Design:
 *   - Each free block contains a header with size and linking information
 *   - Headers are stored at the beginning of each block
 *   - The list is ordered by address to facilitate coalescing
 *   - Supports first-fit, best-fit, and worst-fit allocation strategies
 *
 * Block Structure:
 *   +------------------+
 *   | Block Header     |  <- sizeof(freelist_block_t)
 *   | - size           |
 *   | - is_free        |
 *   | - list_node      |
 *   +------------------+
 *   | Usable Memory    |  <- Returned to caller
 *   | (size bytes)     |
 *   +------------------+
 *
 * Memory Layout:
 *   [Header][Data][Header][Data][Header][Data]...
 *
 * ===========================================================================
 */

#include "freelist.h"

/* ---------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------- */

/* Minimum allocation size (prevents tiny fragments) */
#define MIN_ALLOC_SIZE      16

/* Minimum remaining size after a split (header + MIN_ALLOC_SIZE) */
#define MIN_SPLIT_SIZE      (sizeof(freelist_block_t) + MIN_ALLOC_SIZE)

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* The free list containing all available blocks */
static list_t free_list;

/* Heap region boundaries */
static void *heap_start = NULL;
static void *heap_end = NULL;
static size_t heap_total_size = 0;

/* Statistics */
static size_t total_allocations = 0;
static size_t total_frees = 0;
static size_t bytes_allocated = 0;

/* ---------------------------------------------------------------------------
 * freelist_init - Initialize the free list allocator
 * ---------------------------------------------------------------------------
 * Parameters:
 *   start - Start address of the heap memory region
 *   size  - Total size of the heap in bytes
 *
 * Returns:
 *   true on success, false on failure
 *
 * Notes:
 *   - The entire heap initially becomes one large free block
 *   - The heap must be at least large enough for one header + MIN_ALLOC_SIZE
 * --------------------------------------------------------------------------- */
bool freelist_init(void *start, size_t size)
{
    /* Validate parameters */
    if (!start || size < sizeof(freelist_block_t) + MIN_ALLOC_SIZE) {
        return false;
    }

    /* Initialize the free list */
    list_init(&free_list);

    /* Store heap boundaries */
    heap_start = start;
    heap_end = (char *)start + size;
    heap_total_size = size;

    /* Create the initial free block spanning the entire heap */
    freelist_block_t *initial_block = (freelist_block_t *)start;
    initial_block->size = size - sizeof(freelist_block_t);
    initial_block->is_free = true;
    list_node_init(&initial_block->node);

    /* Add to free list */
    list_push_back(&free_list, &initial_block->node);

    /* Reset statistics */
    total_allocations = 0;
    total_frees = 0;
    bytes_allocated = 0;

    return true;
}

/* ---------------------------------------------------------------------------
 * find_best_fit - Find the smallest free block that fits the request
 * ---------------------------------------------------------------------------
 * This implements the best-fit allocation strategy, which minimizes wasted
 * space within each allocation but may lead to more small fragments.
 * --------------------------------------------------------------------------- */
static freelist_block_t *find_best_fit(size_t size)
{
    freelist_block_t *best = NULL;
    size_t best_size = (size_t)-1;  /* Start with maximum possible size */

    list_node_t *current;
    list_for_each(current, &free_list) {
        freelist_block_t *block = list_entry(current, freelist_block_t, node);
        
        if (block->is_free && block->size >= size) {
            /* This block fits - check if it's better than current best */
            if (block->size < best_size) {
                best = block;
                best_size = block->size;
                
                /* Perfect fit - no need to look further */
                if (block->size == size) {
                    break;
                }
            }
        }
    }

    return best;
}

/* ---------------------------------------------------------------------------
 * find_first_fit - Find the first free block that fits the request
 * ---------------------------------------------------------------------------
 * This implements the first-fit allocation strategy, which is fast but may
 * lead to fragmentation at the beginning of the heap.
 * --------------------------------------------------------------------------- */
static freelist_block_t *find_first_fit(size_t size)
{
    list_node_t *current;
    list_for_each(current, &free_list) {
        freelist_block_t *block = list_entry(current, freelist_block_t, node);
        
        if (block->is_free && block->size >= size) {
            return block;
        }
    }

    return NULL;
}

/* ---------------------------------------------------------------------------
 * split_block - Split a block if it's significantly larger than needed
 * ---------------------------------------------------------------------------
 * If a block is large enough to be split, this function creates a new
 * free block from the remaining space.
 *
 * Before split:
 *   [Header|-------- Large Block --------]
 *
 * After split:
 *   [Header|Allocated][Header|Free Block]
 * --------------------------------------------------------------------------- */
static void split_block(freelist_block_t *block, size_t needed_size)
{
    /* Only split if the remaining space is large enough to be useful */
    if (block->size <= needed_size + MIN_SPLIT_SIZE) {
        return;  /* Don't split - would create too small a fragment */
    }

    /* Calculate where the new block will start */
    char *new_block_addr = (char *)block + sizeof(freelist_block_t) + needed_size;
    freelist_block_t *new_block = (freelist_block_t *)new_block_addr;

    /* Initialize the new block */
    new_block->size = block->size - needed_size - sizeof(freelist_block_t);
    new_block->is_free = true;
    list_node_init(&new_block->node);

    /* Shrink the original block */
    block->size = needed_size;

    /* Insert the new block into the free list after the current block */
    list_insert_after(&free_list, &block->node, &new_block->node);
}

/* ---------------------------------------------------------------------------
 * freelist_alloc - Allocate memory from the heap
 * ---------------------------------------------------------------------------
 * Parameters:
 *   size - Number of bytes to allocate
 *
 * Returns:
 *   Pointer to allocated memory, or NULL if allocation fails
 *
 * Notes:
 *   - Uses first-fit strategy by default (faster than best-fit)
 *   - Returned pointer is to the usable memory, not the header
 *   - Allocations are aligned to at least 8 bytes
 * --------------------------------------------------------------------------- */
void *freelist_alloc(size_t size)
{
    /* Validate request */
    if (size == 0) {
        return NULL;
    }

    /* Align size to 8 bytes for better performance */
    size = ALIGN_UP(size, 8);

    /* Enforce minimum allocation size */
    if (size < MIN_ALLOC_SIZE) {
        size = MIN_ALLOC_SIZE;
    }

    /* Find a suitable block (using first-fit for speed) */
    freelist_block_t *block = find_first_fit(size);
    
    if (!block) {
        /* No suitable block found - out of memory */
        return NULL;
    }

    /* Try to split the block if it's much larger than needed */
    split_block(block, size);

    /* Mark the block as allocated */
    block->is_free = false;

    /* Update statistics */
    total_allocations++;
    bytes_allocated += block->size;

    /* Return pointer to usable memory (after the header) */
    return (void *)((char *)block + sizeof(freelist_block_t));
}

/* ---------------------------------------------------------------------------
 * freelist_alloc_best_fit - Allocate using best-fit strategy
 * ---------------------------------------------------------------------------
 * Same as freelist_alloc but uses best-fit instead of first-fit.
 * Better memory utilization but slower allocation.
 * --------------------------------------------------------------------------- */
void *freelist_alloc_best_fit(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    size = ALIGN_UP(size, 8);
    if (size < MIN_ALLOC_SIZE) {
        size = MIN_ALLOC_SIZE;
    }

    freelist_block_t *block = find_best_fit(size);
    
    if (!block) {
        return NULL;
    }

    split_block(block, size);
    block->is_free = false;

    total_allocations++;
    bytes_allocated += block->size;

    return (void *)((char *)block + sizeof(freelist_block_t));
}

/* ---------------------------------------------------------------------------
 * coalesce_with_next - Merge a block with its next neighbor if both are free
 * ---------------------------------------------------------------------------
 * This reduces fragmentation by combining adjacent free blocks.
 * --------------------------------------------------------------------------- */
static void coalesce_with_next(freelist_block_t *block)
{
    /* Get the next block in memory (not necessarily in the list) */
    char *next_addr = (char *)block + sizeof(freelist_block_t) + block->size;
    
    /* Check if next block is within heap bounds */
    if (next_addr >= (char *)heap_end) {
        return;
    }

    freelist_block_t *next_block = (freelist_block_t *)next_addr;

    /* Check if the next block is free and valid */
    if (!next_block->is_free) {
        return;
    }

    /* Merge: absorb the next block's space and remove it from the free list */
    block->size += sizeof(freelist_block_t) + next_block->size;
    list_remove(&free_list, &next_block->node);
}

/* ---------------------------------------------------------------------------
 * freelist_free - Free previously allocated memory
 * ---------------------------------------------------------------------------
 * Parameters:
 *   ptr - Pointer returned by freelist_alloc (or NULL, which is ignored)
 *
 * Notes:
 *   - Freeing NULL is safe and does nothing
 *   - Double-free is detected and ignored
 *   - Adjacent free blocks are coalesced automatically
 * --------------------------------------------------------------------------- */
void freelist_free(void *ptr)
{
    /* Freeing NULL is always safe */
    if (!ptr) {
        return;
    }

    /* Get the block header */
    freelist_block_t *block = (freelist_block_t *)((char *)ptr - sizeof(freelist_block_t));

    /* Validate the pointer is within our heap */
    if ((void *)block < heap_start || (void *)block >= heap_end) {
        /* Pointer not from our heap - ignore */
        return;
    }

    /* Check for double-free */
    if (block->is_free) {
        return;  /* Already free - ignore */
    }

    /* Mark as free */
    block->is_free = true;

    /* Update statistics */
    total_frees++;
    bytes_allocated -= block->size;

    /* Try to coalesce with the next block */
    coalesce_with_next(block);

    /*
     * Note: Coalescing with the previous block would require either:
     * - A back pointer in each block header
     * - Walking the entire list to find the previous block
     * - Using a footer at the end of each block
     * 
     * For simplicity, we only coalesce forward here. A more sophisticated
     * implementation would use boundary tags (header + footer) for O(1)
     * bidirectional coalescing.
     */
}

/* ---------------------------------------------------------------------------
 * freelist_get_stats - Get allocator statistics
 * ---------------------------------------------------------------------------
 * Fills the provided structure with current allocator statistics.
 * --------------------------------------------------------------------------- */
void freelist_get_stats(freelist_stats_t *stats)
{
    if (!stats) {
        return;
    }

    stats->total_size = heap_total_size;
    stats->bytes_allocated = bytes_allocated;
    stats->bytes_free = heap_total_size - bytes_allocated - 
                        (list_size(&free_list) * sizeof(freelist_block_t));
    stats->allocation_count = total_allocations;
    stats->free_count = total_frees;
    stats->free_block_count = 0;
    stats->largest_free_block = 0;

    /* Walk the free list to count blocks and find largest */
    list_node_t *current;
    list_for_each(current, &free_list) {
        freelist_block_t *block = list_entry(current, freelist_block_t, node);
        if (block->is_free) {
            stats->free_block_count++;
            if (block->size > stats->largest_free_block) {
                stats->largest_free_block = block->size;
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * freelist_debug_dump - Dump the heap state for debugging
 * ---------------------------------------------------------------------------
 * This function walks through all blocks (free and allocated) and could
 * print their information. For now, it just validates heap integrity.
 *
 * Returns the number of blocks found, or -1 if corruption is detected.
 * --------------------------------------------------------------------------- */
int freelist_debug_dump(void)
{
    int block_count = 0;
    char *current_addr = (char *)heap_start;

    while (current_addr < (char *)heap_end) {
        freelist_block_t *block = (freelist_block_t *)current_addr;
        
        /* Basic sanity check */
        if (block->size == 0 || block->size > heap_total_size) {
            return -1;  /* Corruption detected */
        }

        block_count++;
        current_addr += sizeof(freelist_block_t) + block->size;
    }

    return block_count;
}
