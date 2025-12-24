/*
 * ===========================================================================
 * kernel/scheduler/scheduler.h
 * ===========================================================================
 *
 * Task Scheduler Public Interface
 *
 * This header provides the public API for the kernel's task scheduler.
 * The scheduler manages multitasking by:
 *
 * 1. Maintaining queues of ready-to-run tasks
 * 2. Selecting which task runs next based on scheduling policy
 * 3. Performing context switches between tasks
 * 4. Handling preemption via timer interrupts
 *
 * Supported Scheduling Policies:
 * - Round-Robin: FIFO with time slicing
 * - Priority: Lower priority value = runs first
 *
 * Usage:
 *   1. Call scheduler_init() to initialize
 *   2. Create tasks with task_create()
 *   3. Call scheduler_start() to begin multitasking
 *
 * ===========================================================================
 */

#ifndef NEXA_SCHEDULER_H
#define NEXA_SCHEDULER_H

#include "../../config/os_config.h"
#include "task.h"

/* ---------------------------------------------------------------------------
 * Scheduling Policies
 * --------------------------------------------------------------------------- */
#define SCHED_POLICY_ROUND_ROBIN    0   /* First-in-first-out with time slicing */
#define SCHED_POLICY_PRIORITY       1   /* Min priority value runs first */
#define SCHED_POLICY_MLFQ           2   /* Multi-Level Feedback Queue (future) */

/* ---------------------------------------------------------------------------
 * Scheduler Initialization
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the scheduler
 * 
 * Sets up the ready queues (round-robin and priority), creates the idle task,
 * and registers the timer callback for preemptive scheduling.
 * 
 * Must be called after memory management is initialized.
 * 
 * @return true on success, false on failure
 */
bool scheduler_init(void);

/**
 * @brief Start the scheduler
 * 
 * Begins multitasking by switching to the first ready task. This function
 * does not return unless the scheduler is stopped.
 * 
 * Prerequisites:
 * - scheduler_init() must have been called
 * - At least one task should be created (idle task always exists)
 * - Interrupts should be enabled before calling this
 */
void scheduler_start(void);

/**
 * @brief Stop the scheduler
 * 
 * Disables task switching. Used during shutdown or for debugging.
 */
void scheduler_stop(void);

/* ---------------------------------------------------------------------------
 * Task Scheduling
 * --------------------------------------------------------------------------- */

/**
 * @brief Request a reschedule
 * 
 * The scheduler selects the next ready task and performs a context switch.
 * Called when:
 * - A task yields voluntarily
 * - A task blocks (I/O, sleep, etc.)
 * - A task terminates
 * - A timer interrupt preempts the current task
 * 
 * This function may switch to a different task. When it returns, the
 * calling task will resume execution.
 */
void schedule(void);

/**
 * @brief Add a task to the scheduler's ready queue
 * 
 * @param task Task to add (must be in READY state)
 */
void scheduler_add_task(task_t *task);

/**
 * @brief Remove a task from all scheduler queues
 * 
 * @param task Task to remove
 */
void scheduler_remove_task(task_t *task);

/* ---------------------------------------------------------------------------
 * Scheduler Configuration
 * --------------------------------------------------------------------------- */

/**
 * @brief Check if the scheduler is running
 * 
 * @return true if scheduler is active
 */
bool scheduler_is_running(void);

/**
 * @brief Check if the scheduler is initialized
 * 
 * @return true if scheduler has been initialized
 */
bool scheduler_is_initialized(void);

/**
 * @brief Get the current scheduling policy
 * 
 * @return Current policy (SCHED_POLICY_*)
 */
uint8_t scheduler_get_policy(void);

/**
 * @brief Set the scheduling policy
 * 
 * Changes how tasks are selected for execution.
 * 
 * @param policy SCHED_POLICY_ROUND_ROBIN or SCHED_POLICY_PRIORITY
 */
void scheduler_set_policy(uint8_t policy);

/* ---------------------------------------------------------------------------
 * Scheduler Statistics
 * --------------------------------------------------------------------------- */

/**
 * @brief Get the number of tasks in the ready queue
 * 
 * @return Number of ready tasks
 */
uint32_t scheduler_ready_count(void);

/**
 * @brief Get the total number of context switches
 * 
 * @return Context switch count since boot
 */
uint32_t scheduler_get_context_switches(void);

/**
 * @brief Get the total number of schedule() calls
 * 
 * @return Schedule call count
 */
uint32_t scheduler_get_schedule_calls(void);

/**
 * @brief Get idle time
 * 
 * @return Ticks the idle task has run
 */
uint32_t scheduler_get_idle_time(void);

/**
 * @brief Get the idle task
 * 
 * @return Pointer to the idle task
 */
task_t *scheduler_get_idle_task(void);

#endif /* NEXA_SCHEDULER_H */
