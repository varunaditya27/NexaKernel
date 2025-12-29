/*
 * ===========================================================================
 * kernel/scheduler/scheduler.c
 * ===========================================================================
 *
 * Process Scheduler Implementation
 *
 * This file implements the kernel's scheduling logic, which decides which
 * task runs next on the CPU. The scheduler supports two policies:
 *
 * 1. Round-Robin (default)
 *    - Each task gets a fixed time slice
 *    - Tasks rotate in FIFO order
 *    - Fair, prevents starvation
 *    - O(1) scheduling decisions
 *
 * 2. Priority-Based
 *    - Tasks run in priority order (lower value = higher priority)
 *    - Higher priority tasks preempt lower priority ones
 *    - O(log n) scheduling decisions
 *    - Can cause starvation without aging
 *
 * The scheduler is invoked:
 * - On timer interrupts (preemptive scheduling)
 * - When a task blocks, sleeps, or yields
 * - When a task terminates
 *
 * Key Data Structures:
 * - Round-Robin Queue: Circular buffer of ready tasks
 * - Priority Queue: Min-heap of ready tasks by priority
 *
 * ===========================================================================
 */

#include "task.h"
#include "dsa_structures.h"
#include "../memory/memory.h"
#include "../interrupts/interrupts.h"
#include "../drivers/drivers.h"

/* ---------------------------------------------------------------------------
 * External Functions
 * --------------------------------------------------------------------------- */

/* From startup.asm */
extern void cpu_cli(void);              /* Disable interrupts */
extern void cpu_sti(void);              /* Enable interrupts */

/* From context_switch.asm */
extern void context_switch(uint32_t **old_sp, uint32_t *new_sp);
extern void switch_to_task(task_t *new_task);
extern void task_switch_asm(task_t *old_task, task_t *new_task);

/* From task.c */
extern void task_set_current(task_t *task);
extern task_t *task_get_by_index(uint32_t index);
extern bool task_system_is_initialized(void);

/* ---------------------------------------------------------------------------
 * Scheduler Configuration
 * --------------------------------------------------------------------------- */

/* Scheduling policy (from os_config.h or use default) */
#ifndef SCHEDULER_POLICY
#define SCHEDULER_POLICY    SCHED_POLICY_ROUND_ROBIN
#endif

/* Debug output (enable for scheduler debugging) */
#ifndef DEBUG_SCHEDULER
#define DEBUG_SCHEDULER     0
#endif

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Scheduler state */
static bool scheduler_initialized = false;
static bool scheduler_running = false;
static bool scheduling_in_progress = false;

/* Scheduling policy in use */
static uint8_t current_policy = SCHEDULER_POLICY;

/* Idle task (runs when no other task is ready) */
static task_t *idle_task = NULL;

/* Statistics */
static uint32_t context_switch_count = 0;
static uint32_t schedule_call_count = 0;
static uint32_t idle_time = 0;

/* ---------------------------------------------------------------------------
 * Forward Declarations
 * --------------------------------------------------------------------------- */
void schedule(void);
void scheduler_add_task(task_t *task);
void scheduler_remove_task(task_t *task);

/* ---------------------------------------------------------------------------
 * Idle Task Implementation
 * ---------------------------------------------------------------------------
 * The idle task runs when no other task is ready. It puts the CPU into
 * a low-power state while waiting for interrupts.
 * --------------------------------------------------------------------------- */

/**
 * @brief Idle task entry point
 * 
 * This task runs at the lowest priority and simply halts the CPU
 * until an interrupt occurs. This saves power and allows the scheduler
 * to run when a higher-priority task becomes ready.
 */
static void idle_task_entry(void *arg)
{
    UNUSED(arg);
    
    while (1) {
        /*
         * HLT instruction puts CPU in low-power state until next interrupt.
         * Interrupts are enabled, so timer/keyboard/etc. will wake us.
         */
        idle_time++;
        __asm__ volatile("hlt");
    }
}

/**
 * @brief Create the idle task
 */
static bool create_idle_task(void)
{
    idle_task = task_create(
        "idle",                     /* Name */
        idle_task_entry,            /* Entry point */
        NULL,                       /* Argument */
        TASK_PRIORITY_IDLE,         /* Lowest priority */
        TASK_MIN_STACK_SIZE         /* Minimal stack */
    );
    
    if (idle_task == NULL) {
        return false;
    }
    
    /* Set idle task flags */
    idle_task->flags |= TASK_FLAG_IDLE;
    
    return true;
}

/* ---------------------------------------------------------------------------
 * Timer Callback for Preemptive Scheduling
 * ---------------------------------------------------------------------------
 * Called by the PIT driver on each timer tick (SCHEDULER_TICK_HZ per second).
 * This implements time slicing for preemptive scheduling.
 * --------------------------------------------------------------------------- */

/**
 * @brief Timer tick callback
 * 
 * Called from interrupt context. Decrements current task's time slice
 * and triggers a reschedule if the time slice is exhausted.
 */
static void scheduler_tick_handler(void)
{
    task_t *current = task_current();
    
    if (current == NULL || !scheduler_running) {
        return;
    }
    
    /* Increment CPU time for current task */
    current->cpu_time++;
    
    /* Decrement time slice */
    if (current->time_slice > 0) {
        current->time_slice--;
    }
    
    /* Check sleeping tasks and wake them if needed */
    uint32_t current_tick = pit_get_ticks();
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        task_t *task = task_get_by_index(i);
        if (task != NULL && task->state == TASK_STATE_SLEEPING) {
            if (current_tick >= task->sleep_until) {
                task_wakeup(task);
            }
        }
    }
    
    /* Check if preemption is needed */
    if (current->time_slice == 0 && 
        (current->flags & TASK_FLAG_PREEMPTIBLE)) {
        /*
         * Time slice exhausted. Set time slice for next round and reschedule.
         * Note: We don't call schedule() directly from interrupt context.
         * Instead, we rely on the scheduler being called after the interrupt.
         */
        current->time_slice = SCHEDULER_TIME_SLICE;
        
        /* In preemptive mode, we schedule from the timer interrupt */
        if (SCHEDULER_PREEMPTIVE) {
            schedule();
        }
    }
}

/* ---------------------------------------------------------------------------
 * Scheduler Initialization
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the scheduler
 * 
 * Sets up the ready queues, creates the idle task, and registers
 * the timer callback for preemptive scheduling.
 * 
 * @return true on success, false on failure
 */
bool scheduler_init(void)
{
    if (scheduler_initialized) {
        return true;  /* Already initialized */
    }
    
    /* Initialize the task system first */
    if (!task_system_is_initialized()) {
        if (!task_system_init()) {
            return false;
        }
    }
    
    /* Initialize the round-robin queue */
    if (!rr_queue_init(MAX_TASKS)) {
        return false;
    }
    
    /* Initialize the priority queue */
    if (!pq_init(MAX_TASKS)) {
        rr_queue_destroy();
        return false;
    }
    
    /* Create the idle task */
    if (!create_idle_task()) {
        pq_destroy();
        rr_queue_destroy();
        return false;
    }
    
    /* Add idle task to the appropriate queue based on policy */
    scheduler_add_task(idle_task);
    
    /* Reset statistics */
    context_switch_count = 0;
    schedule_call_count = 0;
    idle_time = 0;
    
    /* Register timer callback for preemptive scheduling */
    pit_register_callback(scheduler_tick_handler);
    
    scheduler_initialized = true;
    return true;
}

/**
 * @brief Start the scheduler
 * 
 * Begins task execution by switching to the first ready task.
 * This function does not return until the scheduler is stopped.
 */
void scheduler_start(void)
{
    if (!scheduler_initialized) {
        PANIC("Scheduler not initialized");
    }
    
    /* Find the first task to run */
    task_t *first_task = NULL;
    
    if (current_policy == SCHED_POLICY_PRIORITY) {
        first_task = pq_peek();
    } else {
        first_task = rr_peek();
    }
    
    if (first_task == NULL) {
        first_task = idle_task;
    }
    
    /* Mark scheduler as running */
    scheduler_running = true;
    
    /* Set first task as current and switch to it */
    first_task->state = TASK_STATE_RUNNING;
    
    /* Enable interrupts before switching to first task */
    cpu_sti();
    
    /* Switch to the first task (never returns) */
    switch_to_task(first_task);
    
    /* Should never reach here */
    PANIC("scheduler_start returned unexpectedly");
}

/**
 * @brief Stop the scheduler
 */
void scheduler_stop(void)
{
    scheduler_running = false;
    pit_register_callback(NULL);  /* Unregister timer callback */
}

/* ---------------------------------------------------------------------------
 * Task Queue Management
 * --------------------------------------------------------------------------- */

/**
 * @brief Add a task to the scheduler's ready queue
 * 
 * @param task Task to add
 */
void scheduler_add_task(task_t *task)
{
    if (task == NULL) {
        return;
    }
    
    /* Add to appropriate queue based on policy */
    switch (current_policy) {
        case SCHED_POLICY_PRIORITY:
            pq_enqueue(task);
            break;
            
        case SCHED_POLICY_ROUND_ROBIN:
        default:
            rr_enqueue(task);
            break;
    }
}

/**
 * @brief Remove a task from all scheduler queues
 * 
 * @param task Task to remove
 */
void scheduler_remove_task(task_t *task)
{
    if (task == NULL) {
        return;
    }
    
    /* Remove from both queues (task might be in either) */
    rr_remove(task);
    pq_remove(task);
}

/* ---------------------------------------------------------------------------
 * Main Scheduling Function
 * --------------------------------------------------------------------------- */

/**
 * @brief Pick the next task to run
 * 
 * Selects the next task based on the current scheduling policy.
 * 
 * @return Pointer to next task, or idle_task if no tasks are ready
 */
static task_t *pick_next_task(void)
{
    task_t *next = NULL;
    
    switch (current_policy) {
        case SCHED_POLICY_PRIORITY:
            /* Get highest-priority task */
            next = pq_dequeue();
            break;
            
        case SCHED_POLICY_ROUND_ROBIN:
        default:
            /* Get next task in FIFO order */
            next = rr_dequeue();
            break;
    }
    
    /* If no task is ready, use the idle task */
    if (next == NULL) {
        next = idle_task;
    }
    
    return next;
}

/**
 * @brief Main scheduling function
 * 
 * Called to switch from the current task to another ready task.
 * This function implements the core scheduling logic.
 */
void schedule(void)
{
    /* Prevent recursive scheduling */
    if (scheduling_in_progress) {
        return;
    }
    
    /* Check if scheduler is running */
    if (!scheduler_running || !scheduler_initialized) {
        return;
    }
    
    scheduling_in_progress = true;
    schedule_call_count++;
    
    /* Disable interrupts during scheduling */
    cpu_cli();
    
    task_t *current = task_current();
    task_t *next = NULL;
    
    /* Handle current task */
    if (current != NULL && current->state == TASK_STATE_RUNNING) {
        /*
         * Current task is still runnable (preempted or yielded).
         * Put it back in the ready queue.
         */
        current->state = TASK_STATE_READY;
        scheduler_add_task(current);
    }
    
    /* Pick the next task to run */
    next = pick_next_task();
    
    /* If same task, just return */
    if (next == current) {
        if (current != NULL) {
            current->state = TASK_STATE_RUNNING;
        }
        scheduling_in_progress = false;
        cpu_sti();
        return;
    }
    
    /* Update statistics */
    context_switch_count++;
    
    /* Set next task as running */
    next->state = TASK_STATE_RUNNING;
    
    /* Reset time slice for next task if needed */
    if (next->time_slice == 0) {
        next->time_slice = SCHEDULER_TIME_SLICE;
    }
    
    /* Perform the context switch */
    if (current != NULL) {
        /* Save current context, switch to next */
        task_switch_asm(current, next);
    } else {
        /* No current task (first switch), just load next */
        switch_to_task(next);
    }
    
    /* Execution continues here when this task is scheduled again */
    scheduling_in_progress = false;
    cpu_sti();
}

/* ---------------------------------------------------------------------------
 * Scheduler Utility Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Check if the scheduler is running
 * 
 * @return true if scheduler is active
 */
bool scheduler_is_running(void)
{
    return scheduler_running;
}

/**
 * @brief Check if the scheduler is initialized
 * 
 * @return true if scheduler is initialized
 */
bool scheduler_is_initialized(void)
{
    return scheduler_initialized;
}

/**
 * @brief Get the current scheduling policy
 * 
 * @return Current policy (SCHED_POLICY_*)
 */
uint8_t scheduler_get_policy(void)
{
    return current_policy;
}

/**
 * @brief Set the scheduling policy
 * 
 * @param policy New policy to use
 */
void scheduler_set_policy(uint8_t policy)
{
    if (policy <= SCHED_POLICY_MLFQ) {
        current_policy = policy;
    }
}

/**
 * @brief Get the number of ready tasks
 * 
 * @return Number of tasks in ready queue
 */
uint32_t scheduler_ready_count(void)
{
    if (current_policy == SCHED_POLICY_PRIORITY) {
        return pq_count();
    } else {
        return rr_count();
    }
}

/**
 * @brief Get context switch count
 * 
 * @return Total number of context switches since boot
 */
uint32_t scheduler_get_context_switches(void)
{
    return context_switch_count;
}

/**
 * @brief Get schedule call count
 * 
 * @return Total number of schedule() calls
 */
uint32_t scheduler_get_schedule_calls(void)
{
    return schedule_call_count;
}

/**
 * @brief Get idle time (ticks the idle task has run)
 * 
 * @return Idle time in ticks
 */
uint32_t scheduler_get_idle_time(void)
{
    return idle_time;
}

/**
 * @brief Get the idle task
 * 
 * @return Pointer to idle task
 */
task_t *scheduler_get_idle_task(void)
{
    return idle_task;
}
