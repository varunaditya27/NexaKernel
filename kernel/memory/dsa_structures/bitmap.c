#include <lib/dsa/bitmap.h>
#include <stddef.h>

/*
 * ===========================================================================
 * kernel/memory/dsa_structures/bitmap.c
 * ===========================================================================
 *
 * Physical Frame Allocator - Bitmap Based Implementation
 *
 * This module wraps the generic bitmap data structure for use as a physical
 * frame allocator. Each bit in the bitmap represents one physical memory
 * frame (typically 4KB), where:
 *   - 0 = frame is free
 *   - 1 = frame is allocated
 *
 * This implementation is an alternative to using the frame allocator directly.
 * It's kept here for modularity and to demonstrate how DSA structures are
 * integrated into kernel subsystems.
 *
 * ===========================================================================
 */

#include "../../../lib/dsa/bitmap.h"
#include "../../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* The bitmap tracking frame allocation status */
static bitmap_t frame_bitmap;

/* Statistics */
static size_t total_frames = 0;
static size_t used_frames = 0;

/* Memory region info */
static uintptr_t memory_base = 0;
static size_t page_size = PAGE_SIZE;

/* Initialization flag */
static bool is_initialized = false;

/* ---------------------------------------------------------------------------
 * frame_allocator_init - Initialize the bitmap-based frame allocator
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap_buffer - Pre-allocated buffer for the bitmap data
 *   mem_size      - Total physical memory size in bytes
 *   frame_size    - Size of each frame in bytes (typically PAGE_SIZE)
 *
 * Returns:
 *   true on success, false on failure
 *
 * Notes:
 *   - The bitmap_buffer must be at least (mem_size/frame_size + 7)/8 bytes
 *   - All frames start as free (bit = 0)
 * --------------------------------------------------------------------------- */
bool frame_allocator_init(void *bitmap_buffer, size_t mem_size, size_t frame_size)
{
    /* Validate parameters */
    if (!bitmap_buffer || mem_size == 0 || frame_size == 0) {
        return false;
    }

    /* Calculate total number of frames */
    page_size = frame_size;
    total_frames = mem_size / frame_size;
    used_frames = 0;
    memory_base = 0;

    /* Initialize the bitmap */
    if (!bitmap_init(&frame_bitmap, total_frames, bitmap_buffer)) {
        return false;
    }

    is_initialized = true;
    return true;
}

/* ---------------------------------------------------------------------------
 * frame_allocator_set_base - Set the base address for frame calculations
 * ---------------------------------------------------------------------------
 * Parameters:
 *   base - Physical address of the first frame
 *
 * This allows the allocator to return actual physical addresses rather
 * than just frame indices.
 * --------------------------------------------------------------------------- */
void frame_allocator_set_base(uintptr_t base)
{
    memory_base = base;
}

/* ---------------------------------------------------------------------------
 * frame_alloc - Allocate a single physical frame
 * ---------------------------------------------------------------------------
 * Returns:
 *   Frame index on success (>= 0), or -1 if no frames available
 *
 * Notes:
 *   - Use frame_alloc_address() to get the physical address directly
 * --------------------------------------------------------------------------- */
int64_t frame_alloc(void)
{
    if (!is_initialized) {
        return -1;
    }

    /* Find the first free frame */
    int64_t frame_idx = bitmap_find_first_zero(&frame_bitmap);
    
    if (frame_idx >= 0) {
        /* Mark the frame as allocated */
        bitmap_set(&frame_bitmap, (size_t)frame_idx);
        used_frames++;
    }

    return frame_idx;
}

/* ---------------------------------------------------------------------------
 * frame_alloc_address - Allocate a frame and return its physical address
 * ---------------------------------------------------------------------------
 * Returns:
 *   Physical address of allocated frame, or 0 if allocation fails
 * --------------------------------------------------------------------------- */
uintptr_t frame_alloc_address(void)
{
    int64_t frame_idx = frame_alloc();
    
    if (frame_idx < 0) {
        return 0;  /* Allocation failed */
    }

    return memory_base + ((uintptr_t)frame_idx * page_size);
}

/* ---------------------------------------------------------------------------
 * frame_alloc_contiguous - Allocate multiple contiguous frames
 * ---------------------------------------------------------------------------
 * Parameters:
 *   count - Number of contiguous frames needed
 *
 * Returns:
 *   Starting frame index on success (>= 0), or -1 if not enough contiguous frames
 * --------------------------------------------------------------------------- */
int64_t frame_alloc_contiguous(size_t count)
{
    if (!is_initialized || count == 0) {
        return -1;
    }

    /* Find contiguous free frames */
    int64_t start_idx = bitmap_find_contiguous_zeros(&frame_bitmap, count);
    
    if (start_idx >= 0) {
        /* Mark all frames as allocated */
        bitmap_set_range(&frame_bitmap, (size_t)start_idx, count);
        used_frames += count;
    }

    return start_idx;
}

/* ---------------------------------------------------------------------------
 * frame_free - Free a previously allocated frame
 * ---------------------------------------------------------------------------
 * Parameters:
 *   frame_index - Index of the frame to free
 *
 * Notes:
 *   - Double-free is detected and ignored
 * --------------------------------------------------------------------------- */
void frame_free(size_t frame_index)
{
    if (!is_initialized || frame_index >= total_frames) {
        return;
    }

    /* Check if frame is actually allocated */
    if (bitmap_test(&frame_bitmap, frame_index)) {
        bitmap_clear(&frame_bitmap, frame_index);
        used_frames--;
    }
}

/* ---------------------------------------------------------------------------
 * frame_free_address - Free a frame by its physical address
 * ---------------------------------------------------------------------------
 * Parameters:
 *   address - Physical address of the frame to free
 * --------------------------------------------------------------------------- */
void frame_free_address(uintptr_t address)
{
    if (!is_initialized || address < memory_base) {
        return;
    }

    size_t frame_index = (address - memory_base) / page_size;
    frame_free(frame_index);
}

/* ---------------------------------------------------------------------------
 * frame_free_contiguous - Free multiple contiguous frames
 * ---------------------------------------------------------------------------
 * Parameters:
 *   start_index - Index of first frame to free
 *   count       - Number of frames to free
 * --------------------------------------------------------------------------- */
void frame_free_contiguous(size_t start_index, size_t count)
{
    if (!is_initialized) {
        return;
    }

    for (size_t i = 0; i < count && (start_index + i) < total_frames; i++) {
        if (bitmap_test(&frame_bitmap, start_index + i)) {
            bitmap_clear(&frame_bitmap, start_index + i);
            used_frames--;
        }
    }
}

/* ---------------------------------------------------------------------------
 * frame_is_allocated - Check if a frame is allocated
 * ---------------------------------------------------------------------------
 * Parameters:
 *   frame_index - Index of frame to check
 *
 * Returns:
 *   true if allocated, false if free or invalid index
 * --------------------------------------------------------------------------- */
bool frame_is_allocated(size_t frame_index)
{
    if (!is_initialized || frame_index >= total_frames) {
        return false;
    }

    return bitmap_test(&frame_bitmap, frame_index);
}

/* ---------------------------------------------------------------------------
 * Getter Functions
 * --------------------------------------------------------------------------- */

size_t frame_total_count(void)
{
    return total_frames;
}

size_t frame_used_count(void)
{
    return used_frames;
}

size_t frame_free_count(void)
{
    return total_frames - used_frames;
}

size_t frame_get_page_size(void)
{
    return page_size;
}

bool frame_allocator_is_initialized(void)
{
    return is_initialized;
}
