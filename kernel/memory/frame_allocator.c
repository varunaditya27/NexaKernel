/*
 * ===========================================================================
 * kernel/memory/frame_allocator.c
 * ===========================================================================
 *
 * Physical Frame Allocator
 *
 * This module manages the allocation and deallocation of physical memory
 * frames (pages). It provides the foundation for virtual memory management
 * by tracking which physical memory frames are available and which are in use.
 *
 * Design:
 *   - Uses a bitmap where each bit represents one physical frame
 *   - Bit value 0 = frame is free, 1 = frame is allocated
 *   - Supports frame sizes defined by PAGE_SIZE (typically 4KB)
 *
 * Memory Map (x86 typical):
 *   0x00000000 - 0x000FFFFF: Low memory (BIOS, VGA, etc.) - often unusable
 *   0x00100000 - onwards:    Extended memory - available for use
 *
 * Integration:
 *   This allocator is initialized early in kernel_main() with information
 *   from the multiboot memory map. It provides frames to:
 *   - Kernel heap allocator
 *   - Page table allocations
 *   - DMA buffers
 *
 * ===========================================================================
 */

#include "memory.h"
#include "../../lib/dsa/bitmap.h"
#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * Configuration and Constants
 * --------------------------------------------------------------------------- */

/* Maximum physical memory we can track (256MB by default) */
#define MAX_PHYSICAL_MEMORY_SUPPORTED   MAX_PHYSICAL_MEMORY

/* Number of bits in the bitmap = max frames we can track */
#define MAX_FRAMES  (MAX_PHYSICAL_MEMORY_SUPPORTED / PAGE_SIZE)

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* The bitmap tracking frame allocation */
static bitmap_t frame_bitmap;

/* Statically allocated bitmap buffer (enough for MAX_FRAMES bits) */
/* For 256MB with 4KB pages = 65536 frames = 8192 bytes (8KB) */
static uint8_t bitmap_buffer[(MAX_FRAMES + 7) / 8];

/* Total number of frames being managed */
static size_t total_frames = 0;

/* Number of frames currently in use */
static size_t used_frames = 0;

/* Base address of the managed memory region */
static uintptr_t memory_base = 0;

/* End address of the managed memory region */
static uintptr_t memory_end = 0;

/* Initialization flag */
static bool initialized = false;

/* ---------------------------------------------------------------------------
 * frame_init - Initialize the physical frame allocator
 * ---------------------------------------------------------------------------
 * Parameters:
 *   bitmap_buffer_ignored - Ignored (we use our static buffer)
 *   mem_size              - Total physical memory size in bytes
 *   start_addr            - Base address of usable memory region
 *
 * This function sets up the bitmap and marks all frames as initially free.
 * The caller should subsequently mark reserved regions (kernel, hardware)
 * as allocated using frame_reserve().
 * --------------------------------------------------------------------------- */
void frame_init(void *bitmap_buffer_ignored, size_t mem_size, uintptr_t start_addr)
{
    /* Ignore the passed buffer - we use our static buffer */
    UNUSED(bitmap_buffer_ignored);

    /* Validate parameters */
    if (mem_size == 0 || mem_size > MAX_PHYSICAL_MEMORY_SUPPORTED) {
        mem_size = MAX_PHYSICAL_MEMORY_SUPPORTED;
    }

    /* Store memory region info */
    memory_base = start_addr;
    memory_end = start_addr + mem_size;
    total_frames = mem_size / PAGE_SIZE;
    used_frames = 0;

    /* Cap at maximum supported */
    if (total_frames > MAX_FRAMES) {
        total_frames = MAX_FRAMES;
        memory_end = memory_base + (total_frames * PAGE_SIZE);
    }

    /* Initialize the bitmap - all frames start as free */
    bitmap_init(&frame_bitmap, total_frames, bitmap_buffer);

    initialized = true;
}

/* ---------------------------------------------------------------------------
 * frame_alloc - Allocate a single physical frame
 * ---------------------------------------------------------------------------
 * Returns:
 *   Physical address of the allocated frame, or 0 if no frames available
 *
 * This function finds the first free frame, marks it as allocated, and
 * returns its physical address. Returns 0 (NULL) on failure.
 *
 * Time complexity: O(n/8) worst case, where n is total number of frames
 * --------------------------------------------------------------------------- */
uintptr_t frame_alloc(void)
{
    if (!initialized) {
        return 0;  /* Not initialized */
    }

    /* Find the first free frame */
    int64_t frame_idx = bitmap_find_first_zero(&frame_bitmap);
    
    if (frame_idx < 0) {
        return 0;  /* Out of memory - no free frames */
    }

    /* Mark the frame as allocated */
    bitmap_set(&frame_bitmap, (size_t)frame_idx);
    used_frames++;

    /* Calculate and return the physical address */
    return memory_base + ((uintptr_t)frame_idx * PAGE_SIZE);
}

/* ---------------------------------------------------------------------------
 * frame_alloc_at - Allocate a specific physical frame
 * ---------------------------------------------------------------------------
 * Parameters:
 *   addr - Physical address of the frame to allocate (must be page-aligned)
 *
 * Returns:
 *   The same address on success, or 0 if the frame is already allocated
 *   or out of range
 *
 * This is useful for allocating specific frames needed for hardware
 * (e.g., DMA buffers at specific addresses).
 * --------------------------------------------------------------------------- */
uintptr_t frame_alloc_at(uintptr_t addr)
{
    if (!initialized) {
        return 0;
    }

    /* Validate address is in our managed region and page-aligned */
    if (addr < memory_base || addr >= memory_end || (addr % PAGE_SIZE) != 0) {
        return 0;
    }

    /* Calculate frame index */
    size_t frame_idx = (addr - memory_base) / PAGE_SIZE;

    /* Check if already allocated */
    if (bitmap_test(&frame_bitmap, frame_idx)) {
        return 0;  /* Already in use */
    }

    /* Mark as allocated */
    bitmap_set(&frame_bitmap, frame_idx);
    used_frames++;

    return addr;
}

/* ---------------------------------------------------------------------------
 * frame_alloc_contiguous - Allocate multiple contiguous frames
 * ---------------------------------------------------------------------------
 * Parameters:
 *   count - Number of contiguous frames needed
 *
 * Returns:
 *   Physical address of first frame, or 0 if not enough contiguous memory
 *
 * This is useful for DMA buffers and large kernel allocations that require
 * physically contiguous memory.
 * --------------------------------------------------------------------------- */
uintptr_t frame_alloc_contiguous(size_t count)
{
    if (!initialized || count == 0) {
        return 0;
    }

    /* Find contiguous free frames */
    int64_t start_idx = bitmap_find_contiguous_zeros(&frame_bitmap, count);
    
    if (start_idx < 0) {
        return 0;  /* Not enough contiguous memory */
    }

    /* Mark all frames as allocated */
    bitmap_set_range(&frame_bitmap, (size_t)start_idx, count);
    used_frames += count;

    return memory_base + ((uintptr_t)start_idx * PAGE_SIZE);
}

/* ---------------------------------------------------------------------------
 * frame_free - Free a previously allocated frame
 * ---------------------------------------------------------------------------
 * Parameters:
 *   addr - Physical address of the frame to free
 *
 * If the address is invalid or the frame is already free, this function
 * does nothing (safe to call with any value).
 * --------------------------------------------------------------------------- */
void frame_free(uintptr_t addr)
{
    if (!initialized) {
        return;
    }

    /* Validate address */
    if (addr < memory_base || addr >= memory_end) {
        return;  /* Address out of range */
    }

    /* Check alignment */
    if (addr % PAGE_SIZE != 0) {
        return;  /* Not page-aligned */
    }

    /* Calculate frame index */
    size_t frame_idx = (addr - memory_base) / PAGE_SIZE;

    /* Check if frame is actually allocated (prevent double-free issues) */
    if (bitmap_test(&frame_bitmap, frame_idx)) {
        bitmap_clear(&frame_bitmap, frame_idx);
        used_frames--;
    }
}

/* ---------------------------------------------------------------------------
 * frame_free_contiguous - Free multiple contiguous frames
 * ---------------------------------------------------------------------------
 * Parameters:
 *   addr  - Physical address of first frame
 *   count - Number of frames to free
 * --------------------------------------------------------------------------- */
void frame_free_contiguous(uintptr_t addr, size_t count)
{
    if (!initialized || count == 0) {
        return;
    }

    /* Validate start address */
    if (addr < memory_base || addr >= memory_end || (addr % PAGE_SIZE) != 0) {
        return;
    }

    size_t start_idx = (addr - memory_base) / PAGE_SIZE;

    /* Free each frame */
    for (size_t i = 0; i < count && (start_idx + i) < total_frames; i++) {
        if (bitmap_test(&frame_bitmap, start_idx + i)) {
            bitmap_clear(&frame_bitmap, start_idx + i);
            used_frames--;
        }
    }
}

/* ---------------------------------------------------------------------------
 * frame_reserve - Mark a range of frames as reserved (allocated)
 * ---------------------------------------------------------------------------
 * Parameters:
 *   addr  - Start address (will be aligned down to page boundary)
 *   size  - Size in bytes (will be aligned up to page boundary)
 *
 * This function marks frames as allocated without returning them. It's used
 * during initialization to reserve:
 *   - Kernel code and data regions
 *   - Hardware-reserved memory
 *   - BIOS data areas
 * --------------------------------------------------------------------------- */
void frame_reserve(uintptr_t addr, size_t size)
{
    if (!initialized || size == 0) {
        return;
    }

    /* Align address down and size up to page boundaries */
    uintptr_t start = ALIGN_DOWN(addr, PAGE_SIZE);
    uintptr_t end = ALIGN_UP(addr + size, PAGE_SIZE);

    /* Clamp to our managed region */
    if (start < memory_base) {
        start = memory_base;
    }
    if (end > memory_end) {
        end = memory_end;
    }

    if (start >= end) {
        return;
    }

    /* Calculate frame indices */
    size_t start_idx = (start - memory_base) / PAGE_SIZE;
    size_t end_idx = (end - memory_base) / PAGE_SIZE;

    /* Mark frames as allocated */
    for (size_t i = start_idx; i < end_idx && i < total_frames; i++) {
        if (!bitmap_test(&frame_bitmap, i)) {
            bitmap_set(&frame_bitmap, i);
            used_frames++;
        }
    }
}

/* ---------------------------------------------------------------------------
 * frame_is_free - Check if a frame is free
 * ---------------------------------------------------------------------------
 * Parameters:
 *   addr - Physical address of the frame to check
 *
 * Returns:
 *   true if the frame is free, false if allocated or invalid
 * --------------------------------------------------------------------------- */
bool frame_is_free(uintptr_t addr)
{
    if (!initialized) {
        return false;
    }

    if (addr < memory_base || addr >= memory_end || (addr % PAGE_SIZE) != 0) {
        return false;
    }

    size_t frame_idx = (addr - memory_base) / PAGE_SIZE;
    return !bitmap_test(&frame_bitmap, frame_idx);
}

/* ---------------------------------------------------------------------------
 * Statistics Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Get total number of frames being managed
 */
size_t frame_total_count(void)
{
    return total_frames;
}

/**
 * @brief Get number of frames currently allocated
 */
size_t frame_used_count(void)
{
    return used_frames;
}

/**
 * @brief Get number of frames currently free
 */
size_t frame_free_count(void)
{
    return total_frames - used_frames;
}

/**
 * @brief Get total managed memory in bytes
 */
size_t frame_total_memory(void)
{
    return total_frames * PAGE_SIZE;
}

/**
 * @brief Get used memory in bytes
 */
size_t frame_used_memory(void)
{
    return used_frames * PAGE_SIZE;
}

/**
 * @brief Get free memory in bytes
 */
size_t frame_free_memory(void)
{
    return (total_frames - used_frames) * PAGE_SIZE;
}

/**
 * @brief Get the base address of managed memory
 */
uintptr_t frame_get_base(void)
{
    return memory_base;
}

/**
 * @brief Check if the frame allocator is initialized
 */
bool frame_is_initialized(void)
{
    return initialized;
}
