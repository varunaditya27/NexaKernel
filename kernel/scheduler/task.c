/*
 * kernel/scheduler/task.c
 *
 * Task control block implementation. Define structures for task_t: registers,
 * stack pointer, state, priority, and metadata. Expose functions to create and
 * destroy tasks and to store them in scheduler queues.
 */

#include <stdint.h>

typedef struct task {
    uint32_t *stack_ptr;
    int pid;
    int priority;
    // Add register save area and other metadata
} task_t;

void task_init(task_t *t) {
    // Initialize task structure
}
