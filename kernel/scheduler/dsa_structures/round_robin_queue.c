#include <lib/dsa/queue.h>
#include "../task.h" // Assuming task_t is defined here

/*
 * kernel/scheduler/dsa_structures/round_robin_queue.c
 *
 * Round Robin Queue Wrapper
 *
 * This file adapts the generic Circular Queue data structure for use by the scheduler.
 * It provides a FIFO (First-In-First-Out) mechanism for managing tasks in a
 * round-robin scheduling policy.
 */

static queue_t run_queue;

bool rr_queue_init(size_t capacity) {
    return queue_init(&run_queue, capacity);
}

bool rr_enqueue(task_t *task) {
    return queue_enqueue(&run_queue, (void *)task);
}

task_t *rr_dequeue(void) {
    return (task_t *)queue_dequeue(&run_queue);
}

bool rr_is_empty(void) {
    return queue_is_empty(&run_queue);
}

size_t rr_count(void) {
    return queue_size(&run_queue);
}
