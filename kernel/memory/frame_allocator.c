#include <lib/dsa/bitmap.h>
#include <stddef.h>
#include <stdint.h>

/*
 * kernel/memory/frame_allocator.c
 *
 * Physical Frame Allocator
 *
 * This file manages the allocation and deallocation of physical memory frames
 * (typically 4KB pages). It uses a Bitmap data structure to efficiently track
 * which frames are free and which are in use.
 *
 * It provides the foundation for virtual memory management.
 */

#define PAGE_SIZE 4096

static bitmap_t frame_bitmap;
static size_t total_frames;
static size_t used_frames;
static uintptr_t memory_start;

void frame_init(void *bitmap_buffer, size_t mem_size, uintptr_t start_addr) {
    memory_start = start_addr;
    total_frames = mem_size / PAGE_SIZE;
    used_frames = 0;
    bitmap_init(&frame_bitmap, total_frames, bitmap_buffer);
}

uintptr_t frame_alloc(void) {
    int64_t frame_idx = bitmap_find_first_zero(&frame_bitmap);
    if (frame_idx != -1) {
        bitmap_set(&frame_bitmap, (size_t)frame_idx);
        used_frames++;
        return memory_start + (frame_idx * PAGE_SIZE);
    }
    return 0; // NULL/OOM
}

void frame_free(uintptr_t addr) {
    if (addr < memory_start) return;
    size_t frame_idx = (addr - memory_start) / PAGE_SIZE;
    
    if (bitmap_test(&frame_bitmap, frame_idx)) {
        bitmap_clear(&frame_bitmap, frame_idx);
        used_frames--;
    }
}

size_t frame_total_count(void) {
    return total_frames;
}

size_t frame_used_count(void) {
    return used_frames;
}
