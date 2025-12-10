#ifndef NEXA_SCHEDULER_DSA_H
#define NEXA_SCHEDULER_DSA_H

/*
 * kernel/scheduler/dsa_structures.h
 *
 * Scheduler DSA Wrappers
 *
 * This header declares the specific data structure wrappers used by the scheduler.
 * It adapts generic DSA implementations (Queue, Heap) for managing task queues
 * (e.g., Round Robin run queue, Priority-based ready queue).
 */

#include <stddef.h>
#include <stdbool.h>
#include "./task.h"

/* Round Robin Queue Wrapper */
bool rr_queue_init(size_t capacity);
bool rr_enqueue(task_t *task);
task_t *rr_dequeue(void);
bool rr_is_empty(void);
size_t rr_count(void);

/* Priority Queue Wrapper */
bool pq_init(size_t capacity);
bool pq_enqueue(task_t *task);
task_t *pq_dequeue(void);
bool pq_is_empty(void);
size_t pq_count(void);

#endif /* NEXA_SCHEDULER_DSA_H */
