#ifndef NEXA_TASK_H
#define NEXA_TASK_H

/*
 * kernel/scheduler/task.h
 *
 * Task Structure Definition
 *
 * This header defines the `task_t` struct and associated API. It is included
 * by the scheduler and other modules that need to interact with task objects.
 */

#include <stdint.h>

typedef struct task {
    uint32_t *stack_ptr;
    int pid;
    int priority;
    // Add register save area and other metadata
} task_t;

void task_init(task_t *t);

#endif /* NEXA_TASK_H */
