#include <lib/dsa/heap.h>
#include "../task.h"

/*
 * kernel/scheduler/dsa_structures/priority_queue.c
 *
 * Priority Queue Wrapper
 *
 * This file adapts the generic Binary Heap data structure for use by the scheduler.
 * It allows the scheduler to efficiently retrieve the highest-priority task
 * (O(1) peek, O(log n) dequeue).
 */

static heap_t priority_queue;

/* Comparator for tasks: Higher priority value comes first (or lower, depending on OS design)
 * Let's assume lower value = higher priority (like Unix nice values or simple ranking 0=highest)
 */
static int task_comparator(const void *a, const void *b) {
    const task_t *task_a = (const task_t *)a;
    const task_t *task_b = (const task_t *)b;

    if (task_a->priority < task_b->priority) return -1;
    if (task_a->priority > task_b->priority) return 1;
    return 0;
}

bool pq_init(size_t capacity) {
    return heap_init(&priority_queue, capacity, task_comparator);
}

bool pq_enqueue(task_t *task) {
    return heap_insert(&priority_queue, (void *)task);
}

task_t *pq_dequeue(void) {
    return (task_t *)heap_extract(&priority_queue);
}

bool pq_is_empty(void) {
    return heap_is_empty(&priority_queue);
}

size_t pq_count(void) {
    return heap_size(&priority_queue);
}
