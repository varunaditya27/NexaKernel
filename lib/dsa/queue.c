/*
 * lib/dsa/queue.c
 *
 * Circular Queue Implementation
 *
 * This file implements circular queue operations: enqueue, dequeue, peek,
 * and status checks (empty/full). It manages a ring buffer of pointers.
 */

#include "queue.h"
#include <lib/cstd/memory.h> // Assuming kalloc/kfree or similar are wrapped here, or we use a placeholder

// Forward declaration for memory allocation functions if not yet available in cstd
// In a real kernel, these would come from the heap allocator.
// For now, we will assume standard malloc/free signatures exist or we need to define them.
// Since we are in lib/dsa, we should probably depend on a kernel header or a provided allocator.
// Let's assume <stdlib.h> is NOT available and we need to use kernel allocators.
// However, for this generic lib, we might need to pass an allocator or use a global one.
// Let's check `lib/cstd/memory.c` later. For now, we'll assume `kmalloc` and `kfree` are available globally or via header.

extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

bool queue_init(queue_t *queue, size_t capacity) {
    if (!queue || capacity == 0) return false;

    queue->buffer = (void **)kmalloc(capacity * sizeof(void *));
    if (!queue->buffer) return false;

    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;

    return true;
}

void queue_destroy(queue_t *queue) {
    if (!queue) return;
    if (queue->buffer) {
        kfree(queue->buffer);
        queue->buffer = NULL;
    }
    queue->size = 0;
    queue->capacity = 0;
}

bool queue_is_empty(queue_t *queue) {
    return queue == NULL || queue->size == 0;
}

bool queue_is_full(queue_t *queue) {
    return queue != NULL && queue->size == queue->capacity;
}

bool queue_enqueue(queue_t *queue, void *data) {
    if (queue_is_full(queue)) return false;

    queue->buffer[queue->tail] = data;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;

    return true;
}

void *queue_dequeue(queue_t *queue) {
    if (queue_is_empty(queue)) return NULL;

    void *data = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;

    return data;
}

void *queue_peek(queue_t *queue) {
    if (queue_is_empty(queue)) return NULL;
    return queue->buffer[queue->head];
}

size_t queue_size(queue_t *queue) {
    return queue ? queue->size : 0;
}
