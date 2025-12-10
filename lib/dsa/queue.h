#ifndef NEXA_QUEUE_H
#define NEXA_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

/*
 * lib/dsa/queue.h
 *
 * Generic Circular Queue Interface
 *
 * This header defines a generic fixed-size circular queue (FIFO). It stores
 * pointers to elements, allowing it to hold any data type by reference.
 */

typedef struct queue {
    void **buffer;
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
} queue_t;

/* Initialize a queue with a given capacity */
bool queue_init(queue_t *queue, size_t capacity);

/* Destroy the queue (frees the internal buffer, not the elements) */
void queue_destroy(queue_t *queue);

/* Check if queue is empty */
bool queue_is_empty(queue_t *queue);

/* Check if queue is full */
bool queue_is_full(queue_t *queue);

/* Enqueue an element */
bool queue_enqueue(queue_t *queue, void *data);

/* Dequeue an element */
void *queue_dequeue(queue_t *queue);

/* Peek at the front element */
void *queue_peek(queue_t *queue);

/* Get current size */
size_t queue_size(queue_t *queue);

#endif /* NEXA_QUEUE_H */
