/*
 * lib/dsa/heap.c
 *
 * Binary Heap Implementation
 *
 * This file implements binary heap operations: insertion, extraction (pop top),
 * and peeking. It maintains the heap property using `heapify_up` and `heapify_down`
 * internal helpers.
 */

#include "heap.h"

extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

bool dsa_heap_init(heap_t *heap, size_t capacity, heap_comparator_t comparator) {
    if (!heap || capacity == 0 || !comparator) return false;

    heap->buffer = (void **)kmalloc(capacity * sizeof(void *));
    if (!heap->buffer) return false;

    heap->capacity = capacity;
    heap->size = 0;
    heap->comparator = comparator;

    return true;
}

void dsa_heap_destroy(heap_t *heap) {
    if (!heap) return;
    if (heap->buffer) {
        kfree(heap->buffer);
        heap->buffer = NULL;
    }
    heap->size = 0;
    heap->capacity = 0;
}

static void swap(void **a, void **b) {
    void *temp = *a;
    *a = *b;
    *b = temp;
}

static void heapify_up(heap_t *heap, size_t index) {
    if (index == 0) return;

    size_t parent = (index - 1) / 2;

    // For a min-heap behavior: if current < parent, swap.
    // The comparator should return < 0 if a has higher priority (comes first) than b.
    if (heap->comparator(heap->buffer[index], heap->buffer[parent]) < 0) {
        swap(&heap->buffer[index], &heap->buffer[parent]);
        heapify_up(heap, parent);
    }
}

static void heapify_down(heap_t *heap, size_t index) {
    size_t left = 2 * index + 1;
    size_t right = 2 * index + 2;
    size_t smallest = index;

    if (left < heap->size && heap->comparator(heap->buffer[left], heap->buffer[smallest]) < 0) {
        smallest = left;
    }

    if (right < heap->size && heap->comparator(heap->buffer[right], heap->buffer[smallest]) < 0) {
        smallest = right;
    }

    if (smallest != index) {
        swap(&heap->buffer[index], &heap->buffer[smallest]);
        heapify_down(heap, smallest);
    }
}

bool heap_insert(heap_t *heap, void *data) {
    if (!heap || heap->size >= heap->capacity) return false;

    heap->buffer[heap->size] = data;
    heapify_up(heap, heap->size);
    heap->size++;

    return true;
}

void *heap_extract(heap_t *heap) {
    if (dsa_heap_is_empty(heap)) return NULL;

    void *data = heap->buffer[0];
    heap->buffer[0] = heap->buffer[heap->size - 1];
    heap->size--;
    heapify_down(heap, 0);

    return data;
}

void *dsa_heap_peek(heap_t *heap) {
    if (dsa_heap_is_empty(heap)) return NULL;
    return heap->buffer[0];
}

bool dsa_heap_is_empty(heap_t *heap) {
    return heap == NULL || heap->size == 0;
}

size_t dsa_heap_size(heap_t *heap) {
    return heap ? heap->size : 0;
}
