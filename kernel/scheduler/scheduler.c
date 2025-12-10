/*
 * kernel/scheduler/scheduler.c
 *
 * Process Scheduler
 *
 * This file implements the kernel's scheduling logic. It decides which task
 * runs next on the CPU. It uses DSA wrappers (Round Robin Queue, Priority Queue)
 * to manage the ready state of tasks.
 */

#include "task.h"
#include "dsa_structures.h"
#include <stddef.h>

// Simple round robin for now
void init_scheduler(void) {
    rr_queue_init(100); // Capacity 100
    pq_init(100);       // Capacity 100
}

void schedule(void) {
    if (rr_is_empty()) return;

    task_t *next_task = rr_dequeue();
    // Context switch to next_task (assembly stub would be called here)
    // For now, just re-enqueue to simulate round robin
    rr_enqueue(next_task);
}

void add_task(task_t *task) {
    rr_enqueue(task);
    pq_enqueue(task); // Also add to priority queue for demo
}
