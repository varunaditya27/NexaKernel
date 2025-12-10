#include <lib/dsa/bitmap.h>
#include <stddef.h>

/*
 * Physical Frame Allocator Wrapper
 *
 * Uses the generic bitmap to track physical memory frames.
 */

static bitmap_t frame_bitmap;
static size_t total_frames;
static size_t used_frames;

bool frame_allocator_init(void *bitmap_buffer, size_t mem_size, size_t page_size) {
    if (page_size == 0) return false;
    total_frames = mem_size / page_size;
    used_frames = 0;
    return bitmap_init(&frame_bitmap, total_frames, bitmap_buffer);
}

int64_t frame_alloc(void) {
    int64_t frame = bitmap_find_first_zero(&frame_bitmap);
    if (frame != -1) {
        bitmap_set(&frame_bitmap, (size_t)frame);
        used_frames++;
    }
    return frame;
}

void frame_free(size_t frame_index) {
    if (bitmap_test(&frame_bitmap, frame_index)) {
        bitmap_clear(&frame_bitmap, frame_index);
        used_frames--;
    }
}

size_t frame_total_count(void) {
    return total_frames;
}

size_t frame_used_count(void) {
    return used_frames;
}
