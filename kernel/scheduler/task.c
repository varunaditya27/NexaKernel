/*
 * kernel/scheduler/task.c
 *
 * Task Control Block (TCB) Management
 *
 * This file handles the creation, initialization, and destruction of tasks
 * (processes/threads). It defines the `task_t` structure which holds all
 * information necessary to save and restore a task's execution context.
 */

#include "task.h"
#include <stddef.h>

void task_init(task_t *t) {
    if (!t) return;
    t->stack_ptr = NULL;
    t->pid = 0;
    t->priority = 0;
}
