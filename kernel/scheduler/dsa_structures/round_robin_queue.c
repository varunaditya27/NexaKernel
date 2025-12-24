/*
 * ===========================================================================
 * kernel/scheduler/dsa_structures/round_robin_queue.c
 * ===========================================================================
 *
 * Round-Robin Queue Implementation
 *
 * This file implements a circular queue (ring buffer) specifically designed
 * for round-robin task scheduling. It provides FIFO (First-In-First-Out)
 * semantics where tasks are added to the back and removed from the front.
 *
 * Data Structure: Circular Queue (Ring Buffer)
 * ┌───────────────────────────────────────────────────────────────────────────┐
 * │  A circular queue uses a fixed-size array with two indices:              │
 * │  - head: Points to the front element (next to dequeue)                   │
 * │  - tail: Points to the next free slot (where to enqueue)                 │
 * │                                                                          │
 * │  Example (capacity=8, 3 elements):                                       │
 * │  ┌───┬───┬───┬───┬───┬───┬───┬───┐                                      │
 * │  │   │   │ A │ B │ C │   │   │   │                                      │
 * │  └───┴───┴─▲─┴───┴─▲─┴───┴───┴───┘                                      │
 * │            │       │                                                     │
 * │          head    tail                                                    │
 * │                                                                          │
 * │  Operations:                                                             │
 * │  - Enqueue: Add at tail, advance tail                                    │
 * │  - Dequeue: Remove at head, advance head                                 │
 * │  - Both indices wrap around when reaching capacity                       │
 * │                                                                          │
 * │  Complexity: O(1) for enqueue/dequeue, O(n) for remove                   │
 * └───────────────────────────────────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#include "../dsa_structures.h"
#include "../../memory/memory.h"

/* ---------------------------------------------------------------------------
 * Round-Robin Queue Structure
 * --------------------------------------------------------------------------- */
typedef struct rr_queue {
    task_t **buffer;        /* Array of task pointers */
    size_t capacity;        /* Maximum number of tasks */
    size_t size;            /* Current number of tasks */
    size_t head;            /* Index of front element */
    size_t tail;            /* Index of next free slot */
} rr_queue_t;

/* Static instance of the round-robin queue */
static rr_queue_t run_queue = {
    .buffer = NULL,
    .capacity = 0,
    .size = 0,
    .head = 0,
    .tail = 0
};

/* ---------------------------------------------------------------------------
 * Initialization and Cleanup
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the round-robin queue
 * 
 * Allocates the internal buffer for storing task pointers.
 */
bool rr_queue_init(size_t capacity)
{
    /* Validate capacity */
    if (capacity == 0) {
        return false;
    }

    /* Clean up any existing queue */
    if (run_queue.buffer != NULL) {
        rr_queue_destroy();
    }

    /* Allocate buffer */
    run_queue.buffer = (task_t **)kmalloc(capacity * sizeof(task_t *));
    if (run_queue.buffer == NULL) {
        return false;  /* Out of memory */
    }

    /* Initialize all slots to NULL */
    for (size_t i = 0; i < capacity; i++) {
        run_queue.buffer[i] = NULL;
    }

    /* Initialize queue state */
    run_queue.capacity = capacity;
    run_queue.size = 0;
    run_queue.head = 0;
    run_queue.tail = 0;

    return true;
}

/**
 * @brief Destroy the round-robin queue
 */
void rr_queue_destroy(void)
{
    if (run_queue.buffer != NULL) {
        kfree(run_queue.buffer);
        run_queue.buffer = NULL;
    }
    run_queue.capacity = 0;
    run_queue.size = 0;
    run_queue.head = 0;
    run_queue.tail = 0;
}

/* ---------------------------------------------------------------------------
 * Queue Operations
 * --------------------------------------------------------------------------- */

/**
 * @brief Add a task to the back of the queue
 * 
 * Time Complexity: O(1)
 */
bool rr_enqueue(task_t *task)
{
    /* Validate parameters */
    if (task == NULL || run_queue.buffer == NULL) {
        return false;
    }

    /* Check if queue is full */
    if (run_queue.size >= run_queue.capacity) {
        return false;
    }

    /* Add task at tail position */
    run_queue.buffer[run_queue.tail] = task;

    /* Advance tail (wrap around if needed) */
    run_queue.tail = (run_queue.tail + 1) % run_queue.capacity;

    /* Increment size */
    run_queue.size++;

    return true;
}

/**
 * @brief Remove and return the task at the front
 * 
 * Time Complexity: O(1)
 */
task_t *rr_dequeue(void)
{
    /* Check if queue is empty */
    if (run_queue.buffer == NULL || run_queue.size == 0) {
        return NULL;
    }

    /* Get task at head position */
    task_t *task = run_queue.buffer[run_queue.head];

    /* Clear the slot */
    run_queue.buffer[run_queue.head] = NULL;

    /* Advance head (wrap around if needed) */
    run_queue.head = (run_queue.head + 1) % run_queue.capacity;

    /* Decrement size */
    run_queue.size--;

    return task;
}

/**
 * @brief Look at the front task without removing it
 * 
 * Time Complexity: O(1)
 */
task_t *rr_peek(void)
{
    if (run_queue.buffer == NULL || run_queue.size == 0) {
        return NULL;
    }
    return run_queue.buffer[run_queue.head];
}

/**
 * @brief Check if queue is empty
 * 
 * Time Complexity: O(1)
 */
bool rr_is_empty(void)
{
    return (run_queue.buffer == NULL || run_queue.size == 0);
}

/**
 * @brief Check if queue is full
 * 
 * Time Complexity: O(1)
 */
bool rr_is_full(void)
{
    return (run_queue.buffer != NULL && run_queue.size >= run_queue.capacity);
}

/**
 * @brief Get the current number of tasks
 * 
 * Time Complexity: O(1)
 */
size_t rr_count(void)
{
    return run_queue.size;
}

/**
 * @brief Remove a specific task from anywhere in the queue
 * 
 * This is needed when a task blocks or terminates and must be removed
 * from the ready queue regardless of its position.
 * 
 * Time Complexity: O(n) where n is the number of tasks in queue
 */
bool rr_remove(task_t *task)
{
    /* Validate parameters */
    if (task == NULL || run_queue.buffer == NULL || run_queue.size == 0) {
        return false;
    }

    /*
     * Search for the task in the queue.
     * We need to iterate from head to tail, accounting for wrap-around.
     */
    size_t current = run_queue.head;
    size_t found_index = run_queue.capacity;  /* Invalid index = not found */

    for (size_t i = 0; i < run_queue.size; i++) {
        if (run_queue.buffer[current] == task) {
            found_index = current;
            break;
        }
        current = (current + 1) % run_queue.capacity;
    }

    /* If not found, return false */
    if (found_index == run_queue.capacity) {
        return false;
    }

    /*
     * Shift all elements after the found position one position backward.
     * This maintains the FIFO order of the queue.
     */
    size_t i = found_index;
    size_t next = (i + 1) % run_queue.capacity;
    
    while (next != run_queue.tail) {
        run_queue.buffer[i] = run_queue.buffer[next];
        i = next;
        next = (next + 1) % run_queue.capacity;
    }

    /* Clear the last position and update tail */
    run_queue.buffer[i] = NULL;
    
    /* Move tail back one position */
    if (run_queue.tail == 0) {
        run_queue.tail = run_queue.capacity - 1;
    } else {
        run_queue.tail--;
    }

    /* Decrement size */
    run_queue.size--;

    return true;
}
