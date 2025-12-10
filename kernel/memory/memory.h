#ifndef NEXA_MEMORY_H
#define NEXA_MEMORY_H

/*
 * kernel/memory/memory.h
 *
 * Memory Management Subsystem Interface
 *
 * This header exposes the public API for the kernel's memory management
 * components, including the Physical Frame Allocator and the Kernel Heap Allocator.
 */

#include <stddef.h>
#include <stdint.h>

/* Frame Allocator */
void frame_init(void *bitmap_buffer, size_t mem_size, uintptr_t start_addr);
uintptr_t frame_alloc(void);
void frame_free(uintptr_t addr);
size_t frame_total_count(void);
size_t frame_used_count(void);

/* Heap Allocator */
void heap_init(void *start, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif /* NEXA_MEMORY_H */
