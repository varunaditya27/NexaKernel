#ifndef NEXA_HEAP_H
#define NEXA_HEAP_H

#include "../../config/os_config.h"


/*
 * lib/dsa/heap.h
 *
 * Generic Binary Heap Interface
 *
 * This header defines a generic binary heap (priority queue). It supports both
 * Min-Heap and Max-Heap behavior through a user-provided comparator function.
 */

typedef int (*heap_comparator_t)(const void *a, const void *b);

typedef struct heap {
    void **buffer;
    size_t capacity;
    size_t size;
    heap_comparator_t comparator; // Returns < 0 if a < b, > 0 if a > b, 0 if equal
} heap_t;

/* Initialize a heap */
bool dsa_heap_init(heap_t *heap, size_t capacity, heap_comparator_t comparator);

/* Destroy the heap */
void dsa_heap_destroy(heap_t *heap);

/* Insert an element */
bool heap_insert(heap_t *heap, void *data);

/* Extract the top element (min or max depending on comparator) */
void *heap_extract(heap_t *heap);

/* Peek at the top element */
void *dsa_heap_peek(heap_t *heap);

/* Check if heap is empty */
bool dsa_heap_is_empty(heap_t *heap);

/* Get current size */
size_t dsa_heap_size(heap_t *heap);

#endif /* NEXA_HEAP_H */
