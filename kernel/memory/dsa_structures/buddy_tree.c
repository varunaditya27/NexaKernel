/*
 * ===========================================================================
 * kernel/memory/dsa_structures/buddy_tree.c
 * ===========================================================================
 *
 * Buddy System Memory Allocator Implementation
 *
 * The buddy system is a classic memory allocation algorithm that divides
 * memory into power-of-two sized blocks. It provides:
 *
 *   - O(log N) allocation time
 *   - O(log N) deallocation time
 *   - Automatic coalescing of adjacent free blocks
 *   - Low external fragmentation
 *
 * Algorithm Overview:
 *   - Maintain free lists for each block "order" (size = 2^order * MIN_SIZE)
 *   - To allocate: find smallest order that fits, split if necessary
 *   - To free: check if buddy is free, merge if so, repeat up the orders
 *
 * Buddy Address Calculation:
 *   For a block at address A of order N:
 *   Buddy address = A XOR (block_size)
 *   
 *   Example: Block at 0x1000, order 0 (4KB), buddy at 0x1000 ^ 0x1000 = 0x0
 *            Block at 0x2000, order 1 (8KB), buddy at 0x2000 ^ 0x2000 = 0x0
 *
 * ===========================================================================
 */

#include "buddy.h"
#include "../../../lib/dsa/bitmap.h"

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Free lists for each order (0 to BUDDY_MAX_ORDER) */
static list_t free_areas[BUDDY_MAX_ORDER + 1];

/* Bitmap to track allocated blocks (1 = allocated, 0 = free) */
static bitmap_t block_bitmap;
static uint8_t *bitmap_buffer = NULL;

/* Memory region being managed */
static void *memory_start = NULL;
static void *memory_end = NULL;
static size_t total_memory = 0;

/* Statistics */
static size_t free_memory = 0;
static size_t allocated_memory = 0;
static size_t total_allocations = 0;
static size_t total_frees = 0;

/* Initialization flag */
static bool initialized = false;

/* ---------------------------------------------------------------------------
 * Helper Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Calculate the order needed for a given size
 * 
 * Finds the smallest order N such that 2^N * MIN_BLOCK_SIZE >= size
 */
int buddy_size_to_order(size_t size)
{
    if (size == 0) {
        return 0;
    }

    int order = 0;
    size_t block_size = BUDDY_MIN_BLOCK_SIZE;

    while (block_size < size && order < BUDDY_MAX_ORDER) {
        block_size <<= 1;  /* block_size *= 2 */
        order++;
    }

    if (block_size < size) {
        return -1;  /* Size too large */
    }

    return order;
}

/**
 * @brief Calculate block size for a given order
 */
size_t buddy_order_to_size(int order)
{
    if (order < 0 || order > BUDDY_MAX_ORDER) {
        return 0;
    }
    return BUDDY_MIN_BLOCK_SIZE << order;
}

/**
 * @brief Get the buddy address for a block
 * 
 * The buddy of a block is the adjacent block of the same size that can be
 * merged with this block to form a block of the next order up.
 */
static void *get_buddy_address(void *block, int order)
{
    size_t block_size = buddy_order_to_size(order);
    uintptr_t block_offset = (uintptr_t)block - (uintptr_t)memory_start;
    uintptr_t buddy_offset = block_offset ^ block_size;
    
    return (void *)((uintptr_t)memory_start + buddy_offset);
}

/**
 * @brief Get the block index in the bitmap for a given address and order
 * 
 * The bitmap tracks blocks at the minimum order level (order 0).
 */
static size_t get_block_index(void *block)
{
    uintptr_t offset = (uintptr_t)block - (uintptr_t)memory_start;
    return offset / BUDDY_MIN_BLOCK_SIZE;
}

/**
 * @brief Check if a block's buddy is free and can be merged
 */
static bool is_buddy_free(void *buddy, int order)
{
    /* Check if buddy is within our managed region */
    if (buddy < memory_start || buddy >= memory_end) {
        return false;
    }

    /* Check the bitmap - all sub-blocks must be free */
    size_t start_idx = get_block_index(buddy);
    size_t block_count = 1 << order;  /* Number of order-0 blocks */

    for (size_t i = 0; i < block_count; i++) {
        if (bitmap_test(&block_bitmap, start_idx + i)) {
            return false;  /* Some part is allocated */
        }
    }

    /* Buddy is free - but we need to verify it's actually in the free list
     * at this order (not split into smaller blocks) */
    list_node_t *current;
    list_for_each(current, &free_areas[order]) {
        buddy_block_t *block = list_entry(current, buddy_block_t, node);
        if ((void *)block == buddy) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Remove a block from its free list
 */
static void remove_from_free_list(void *ptr, int order)
{
    list_node_t *current;
    list_for_each(current, &free_areas[order]) {
        buddy_block_t *block = list_entry(current, buddy_block_t, node);
        if ((void *)block == ptr) {
            list_remove(&free_areas[order], &block->node);
            return;
        }
    }
}

/**
 * @brief Add a block to its free list
 */
static void add_to_free_list(void *ptr, int order)
{
    buddy_block_t *block = (buddy_block_t *)ptr;
    block->order = order;
    list_node_init(&block->node);
    list_push_back(&free_areas[order], &block->node);
}

/**
 * @brief Mark blocks as allocated in the bitmap
 */
static void mark_allocated(void *ptr, int order)
{
    size_t start_idx = get_block_index(ptr);
    size_t block_count = 1 << order;

    for (size_t i = 0; i < block_count; i++) {
        bitmap_set(&block_bitmap, start_idx + i);
    }
}

/**
 * @brief Mark blocks as free in the bitmap
 */
static void mark_free(void *ptr, int order)
{
    size_t start_idx = get_block_index(ptr);
    size_t block_count = 1 << order;

    for (size_t i = 0; i < block_count; i++) {
        bitmap_clear(&block_bitmap, start_idx + i);
    }
}

/* ---------------------------------------------------------------------------
 * Public API Implementation
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the buddy allocator
 */
bool buddy_init(void *start, size_t total_size)
{
    /* Validate parameters */
    if (!start || total_size < BUDDY_MIN_BLOCK_SIZE * 2) {
        return false;
    }

    /* Align start address to MIN_BLOCK_SIZE */
    uintptr_t aligned_start = ALIGN_UP((uintptr_t)start, BUDDY_MIN_BLOCK_SIZE);
    size_t lost_to_alignment = aligned_start - (uintptr_t)start;
    
    if (lost_to_alignment >= total_size) {
        return false;
    }
    
    total_size -= lost_to_alignment;
    start = (void *)aligned_start;

    /* Calculate how many minimum-size blocks we can manage */
    size_t num_blocks = total_size / BUDDY_MIN_BLOCK_SIZE;
    
    /* Reserve space for the bitmap at the beginning of the region */
    size_t bitmap_bytes = (num_blocks + 7) / 8;
    size_t bitmap_blocks = (bitmap_bytes + BUDDY_MIN_BLOCK_SIZE - 1) / BUDDY_MIN_BLOCK_SIZE;
    
    if (bitmap_blocks >= num_blocks) {
        return false;  /* Not enough space */
    }

    /* Set up the bitmap in the reserved area */
    bitmap_buffer = (uint8_t *)start;
    
    /* Adjust the usable memory region */
    memory_start = (void *)((uintptr_t)start + (bitmap_blocks * BUDDY_MIN_BLOCK_SIZE));
    num_blocks -= bitmap_blocks;
    total_memory = num_blocks * BUDDY_MIN_BLOCK_SIZE;
    memory_end = (void *)((uintptr_t)memory_start + total_memory);

    /* Initialize the bitmap */
    if (!bitmap_init(&block_bitmap, num_blocks, bitmap_buffer)) {
        return false;
    }

    /* Initialize all free lists */
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        list_init(&free_areas[i]);
    }

    /* Add all memory as free blocks of the largest possible order */
    void *current_addr = memory_start;
    size_t remaining = total_memory;

    while (remaining >= BUDDY_MIN_BLOCK_SIZE) {
        /* Find the largest order that fits in the remaining space
         * and is properly aligned */
        int order = BUDDY_MAX_ORDER;
        
        while (order >= 0) {
            size_t block_size = buddy_order_to_size(order);
            uintptr_t offset = (uintptr_t)current_addr - (uintptr_t)memory_start;
            
            /* Check if block fits and is aligned for this order */
            if (block_size <= remaining && (offset % block_size) == 0) {
                break;
            }
            order--;
        }

        if (order < 0) {
            break;  /* Can't fit any more blocks */
        }

        /* Add this block to the free list */
        add_to_free_list(current_addr, order);
        
        size_t block_size = buddy_order_to_size(order);
        current_addr = (void *)((uintptr_t)current_addr + block_size);
        remaining -= block_size;
    }

    /* Initialize statistics */
    free_memory = total_memory;
    allocated_memory = 0;
    total_allocations = 0;
    total_frees = 0;

    initialized = true;
    return true;
}

/**
 * @brief Allocate a block of a specific order
 */
void *buddy_alloc_order(int order)
{
    if (!initialized || order < 0 || order > BUDDY_MAX_ORDER) {
        return NULL;
    }

    /* Find a free block at this order or higher */
    int current_order = order;
    
    while (current_order <= BUDDY_MAX_ORDER) {
        if (!list_is_empty(&free_areas[current_order])) {
            break;
        }
        current_order++;
    }

    if (current_order > BUDDY_MAX_ORDER) {
        return NULL;  /* No memory available */
    }

    /* Remove a block from the free list at current_order */
    list_node_t *node = list_pop_front(&free_areas[current_order]);
    buddy_block_t *block = list_entry(node, buddy_block_t, node);
    void *block_addr = (void *)block;

    /* Split the block down to the requested order */
    while (current_order > order) {
        current_order--;
        
        /* Calculate the address of the buddy (second half) */
        size_t half_size = buddy_order_to_size(current_order);
        void *buddy = (void *)((uintptr_t)block_addr + half_size);
        
        /* Add the buddy to the free list at the lower order */
        add_to_free_list(buddy, current_order);
    }

    /* Mark the block as allocated */
    mark_allocated(block_addr, order);

    /* Update statistics */
    size_t block_size = buddy_order_to_size(order);
    free_memory -= block_size;
    allocated_memory += block_size;
    total_allocations++;

    return block_addr;
}

/**
 * @brief Allocate memory (size-based interface)
 */
void *buddy_alloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    int order = buddy_size_to_order(size);
    if (order < 0) {
        return NULL;  /* Size too large */
    }

    return buddy_alloc_order(order);
}

/**
 * @brief Free a block of a specific order
 */
void buddy_free_order(void *ptr, int order)
{
    if (!initialized || !ptr || order < 0 || order > BUDDY_MAX_ORDER) {
        return;
    }

    /* Validate pointer is within our region and properly aligned */
    if (ptr < memory_start || ptr >= memory_end) {
        return;
    }

    size_t block_size = buddy_order_to_size(order);
    if (((uintptr_t)ptr - (uintptr_t)memory_start) % block_size != 0) {
        return;  /* Not properly aligned for this order */
    }

    /* Mark as free in bitmap */
    mark_free(ptr, order);

    /* Update statistics */
    free_memory += block_size;
    allocated_memory -= block_size;
    total_frees++;

    /* Try to coalesce with buddy, moving up orders */
    void *current_block = ptr;
    int current_order = order;

    while (current_order < BUDDY_MAX_ORDER) {
        void *buddy = get_buddy_address(current_block, current_order);

        /* Check if buddy is free and at the same order */
        if (!is_buddy_free(buddy, current_order)) {
            break;  /* Can't merge - stop here */
        }

        /* Remove buddy from its free list */
        remove_from_free_list(buddy, current_order);

        /* The merged block starts at the lower address */
        if (buddy < current_block) {
            current_block = buddy;
        }

        current_order++;
    }

    /* Add the (possibly merged) block to the appropriate free list */
    add_to_free_list(current_block, current_order);
}

/**
 * @brief Free memory (size-based interface)
 */
void buddy_free(void *ptr, size_t size)
{
    if (size == 0 || !ptr) {
        return;
    }

    int order = buddy_size_to_order(size);
    if (order < 0) {
        return;
    }

    buddy_free_order(ptr, order);
}

/**
 * @brief Get allocator statistics
 */
void buddy_get_stats(buddy_stats_t *stats)
{
    if (!stats || !initialized) {
        return;
    }

    stats->total_memory = total_memory;
    stats->free_memory = free_memory;
    stats->allocated_memory = allocated_memory;
    stats->total_allocations = total_allocations;
    stats->total_frees = total_frees;

    /* Count free blocks at each order */
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        stats->blocks_per_order[i] = list_size(&free_areas[i]);
    }
}

/**
 * @brief Check if allocator is initialized
 */
bool buddy_is_initialized(void)
{
    return initialized;
}
