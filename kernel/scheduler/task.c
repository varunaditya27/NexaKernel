/*
 * ===========================================================================
 * kernel/scheduler/task.c
 * ===========================================================================
 *
 * Task Control Block (TCB) Management Implementation
 *
 * This file implements task (process/thread) management including creation,
 * destruction, and state tracking. Each task has:
 *
 * - A unique PID
 * - A kernel stack for execution context
 * - CPU register state saved during context switches
 * - Scheduling metadata (priority, state, time accounting)
 *
 * The implementation uses a fixed-size task table where each slot is either
 * UNUSED (free) or contains an active task.
 *
 * ===========================================================================
 */

#include "task.h"
#include "../memory/memory.h"
#include "../drivers/drivers.h"

/* ---------------------------------------------------------------------------
 * External Functions (from assembly)
 * --------------------------------------------------------------------------- */
extern void cpu_cli(void);          /* Disable interrupts */
extern void cpu_sti(void);          /* Enable interrupts */

/* Forward declaration of scheduler functions */
extern void schedule(void);
extern void scheduler_add_task(task_t *task);
extern void scheduler_remove_task(task_t *task);

/* ---------------------------------------------------------------------------
 * Static Variables
 * --------------------------------------------------------------------------- */

/* Task table - holds all TCBs */
static task_t task_table[MAX_TASKS];

/* Current running task */
static task_t *current_task = NULL;

/* Next available PID */
static uint32_t next_pid = 0;

/* Total number of active tasks */
static uint32_t active_task_count = 0;

/* Flag indicating task system is initialized */
static bool task_system_initialized = false;

/* ---------------------------------------------------------------------------
 * Internal Helper Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Find a free slot in the task table
 * 
 * @return Pointer to free task slot, or NULL if table is full
 */
static task_t *find_free_task_slot(void)
{
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].state == TASK_STATE_UNUSED) {
            return &task_table[i];
        }
    }
    return NULL;
}

/**
 * @brief Generate the next unique PID
 * 
 * @return New PID value
 */
static uint32_t allocate_pid(void)
{
    return next_pid++;
}

/**
 * @brief Copy a string safely (like strncpy but ensures null termination)
 * 
 * @param dest   Destination buffer
 * @param src    Source string
 * @param n      Maximum characters to copy
 */
static void safe_strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/**
 * @brief Entry wrapper for new tasks
 * 
 * This function is called when a task first starts executing. It calls
 * the actual entry point and handles task termination when it returns.
 * 
 * Note: This runs in the context of the new task.
 */
static void task_entry_wrapper(void)
{
    /*
     * Interrupts were disabled during context switch.
     * Re-enable them now that we're in the task's context.
     */
    cpu_sti();

    /* Get current task and call its entry point */
    task_t *task = current_task;
    
    if (task != NULL && task->entry_point != NULL) {
        /* Clear the first-run flag */
        task->flags &= ~TASK_FLAG_FIRST_RUN;
        
        /* Call the task's actual entry function */
        task->entry_point(task->arg);
    }

    /* If entry point returns, terminate the task */
    task_exit(0);
}

/**
 * @brief Set up the initial stack for a new task
 * 
 * Creates a fake stack frame that looks like the task was interrupted,
 * so context_switch can "resume" it.
 * 
 * @param task Task to set up stack for
 */
static void setup_task_stack(task_t *task)
{
    /*
     * Initial stack layout (growing downward, low address at top):
     *
     * [stack_base + stack_size]  <- Stack bottom (highest address)
     *   ...
     * [entry_wrapper address]    <- EIP: where task will start
     * [EAX]                      <- General purpose registers
     * [ECX]                      
     * [EDX]                      
     * [EBX]                      
     * [ESP dummy]                <- Ignored
     * [EBP = 0]                  <- NULL for clean stack trace
     * [ESI]                      
     * [EDI]                      
     * [stack_pointer]            <- Task's ESP after setup (lowest address)
     *
     * When context_switch restores this task, it will:
     * 1. Load ESP from task->stack_pointer
     * 2. POPA (restore EDI, ESI, EBP, skip ESP, EBX, EDX, ECX, EAX)
     * 3. RET (pop EIP and jump to task_entry_wrapper)
     */

    /* Calculate the top of the stack (highest valid address) */
    uint32_t *stack_top = (uint32_t *)((uint8_t *)task->stack_base + task->stack_size);
    
    /* Align stack to 16 bytes (required by some calling conventions) */
    stack_top = (uint32_t *)((uintptr_t)stack_top & ~0xF);
    
    /* Build the stack frame from top to bottom */
    uint32_t *sp = stack_top;
    
    /* Push the entry point address (will be popped by RET) */
    *(--sp) = (uint32_t)task_entry_wrapper;
    
    /* Push initial register values (POPA order: EDI, ESI, EBP, skip, EBX, EDX, ECX, EAX) */
    /* But PUSHA pushes: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI */
    /* So stack from low to high: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX */
    /* POPA pops in reverse: EDI, ESI, EBP, (skip ESP), EBX, EDX, ECX, EAX */
    
    *(--sp) = 0;                    /* EAX */
    *(--sp) = 0;                    /* ECX */
    *(--sp) = 0;                    /* EDX */
    *(--sp) = 0;                    /* EBX */
    *(--sp) = 0;                    /* ESP (ignored by POPA) */
    *(--sp) = 0;                    /* EBP = 0 for clean backtrace */
    *(--sp) = 0;                    /* ESI */
    *(--sp) = 0;                    /* EDI */
    
    /* Save the stack pointer */
    task->stack_pointer = sp;
}

/* ---------------------------------------------------------------------------
 * Public API Implementation
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the task subsystem
 */
bool task_system_init(void)
{
    if (task_system_initialized) {
        return true;  /* Already initialized */
    }

    /* Clear all task slots */
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        task_init(&task_table[i]);
    }

    /* Reset counters */
    next_pid = 0;
    active_task_count = 0;
    current_task = NULL;

    task_system_initialized = true;
    return true;
}

/**
 * @brief Initialize a task structure to default values
 */
void task_init(task_t *task)
{
    if (task == NULL) {
        return;
    }

    /* Clear all fields */
    task->stack_pointer = NULL;
    task->stack_base = NULL;
    task->stack_size = 0;

    task->pid = 0;
    task->name[0] = '\0';

    task->state = TASK_STATE_UNUSED;
    task->priority = TASK_PRIORITY_DEFAULT;
    task->base_priority = TASK_PRIORITY_DEFAULT;
    task->flags = 0;
    task->time_slice = SCHEDULER_TIME_SLICE;

    task->cpu_time = 0;
    task->start_time = 0;
    task->sleep_until = 0;

    task->entry_point = NULL;
    task->arg = NULL;

    task->exit_code = 0;

    task->next = NULL;
    task->prev = NULL;
}

/**
 * @brief Create a new task
 */
task_t *task_create(const char *name, 
                    void (*entry_point)(void *),
                    void *arg,
                    uint8_t priority,
                    size_t stack_size)
{
    /* Validate parameters */
    if (entry_point == NULL) {
        return NULL;
    }

    if (!task_system_initialized) {
        return NULL;
    }

    /* Find a free task slot */
    task_t *task = find_free_task_slot();
    if (task == NULL) {
        return NULL;  /* Task table full */
    }

    /* Mark as creating (prevents race conditions) */
    task->state = TASK_STATE_CREATING;

    /* Set up task identification */
    task->pid = allocate_pid();
    if (name != NULL) {
        safe_strncpy(task->name, name, sizeof(task->name));
    } else {
        /* Generate a default name */
        task->name[0] = 'T';
        task->name[1] = 'a';
        task->name[2] = 's';
        task->name[3] = 'k';
        task->name[4] = '-';
        /* Simple number to string for PID */
        uint32_t pid = task->pid;
        int pos = 5;
        if (pid == 0) {
            task->name[pos++] = '0';
        } else {
            char digits[10];
            int d = 0;
            while (pid > 0 && d < 10) {
                digits[d++] = '0' + (pid % 10);
                pid /= 10;
            }
            while (d > 0 && pos < 31) {
                task->name[pos++] = digits[--d];
            }
        }
        task->name[pos] = '\0';
    }

    /* Set scheduling parameters */
    if (priority >= MAX_PRIORITY_LEVELS) {
        priority = MAX_PRIORITY_LEVELS - 1;
    }
    task->priority = priority;
    task->base_priority = priority;
    task->time_slice = SCHEDULER_TIME_SLICE;

    /* Set flags */
    task->flags = TASK_FLAG_KERNEL | TASK_FLAG_PREEMPTIBLE | TASK_FLAG_FIRST_RUN;

    /* Allocate stack */
    if (stack_size == 0) {
        stack_size = TASK_STACK_SIZE;
    }
    if (stack_size < TASK_MIN_STACK_SIZE) {
        stack_size = TASK_MIN_STACK_SIZE;
    }
    
    /* Align stack size to page boundary */
    stack_size = ALIGN_UP(stack_size, PAGE_SIZE);
    
    task->stack_base = kmalloc(stack_size);
    if (task->stack_base == NULL) {
        task->state = TASK_STATE_UNUSED;
        return NULL;  /* Out of memory */
    }
    task->stack_size = stack_size;

    /* Set entry point and argument */
    task->entry_point = entry_point;
    task->arg = arg;

    /* Initialize time accounting */
    task->cpu_time = 0;
    task->start_time = pit_get_ticks();
    task->sleep_until = 0;

    /* Set up the initial stack frame */
    setup_task_stack(task);

    /* Task is now ready to run */
    task->state = TASK_STATE_READY;
    active_task_count++;

    return task;
}

/**
 * @brief Terminate the current task
 */
void task_exit(int32_t exit_code)
{
    /* Disable interrupts during state change */
    cpu_cli();

    task_t *task = current_task;
    if (task == NULL) {
        /* No current task - shouldn't happen */
        cpu_sti();
        return;
    }

    /* Store exit code */
    task->exit_code = exit_code;

    /* Mark task as terminated */
    task->state = TASK_STATE_ZOMBIE;
    task->flags |= TASK_FLAG_NEEDS_CLEANUP;

    /* Remove from scheduler queues */
    scheduler_remove_task(task);

    /* Decrement active task count */
    if (active_task_count > 0) {
        active_task_count--;
    }

    /* Re-enable interrupts and schedule next task */
    cpu_sti();
    
    /* This will never return - scheduler will pick another task */
    schedule();
    
    /* Should never reach here */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/**
 * @brief Destroy a task and free its resources
 */
void task_destroy(task_t *task)
{
    if (task == NULL) {
        return;
    }

    /* Only destroy terminated tasks */
    if (task->state != TASK_STATE_TERMINATED && 
        task->state != TASK_STATE_ZOMBIE) {
        return;
    }

    /* Free the stack */
    if (task->stack_base != NULL) {
        kfree(task->stack_base);
        task->stack_base = NULL;
    }

    /* Reset the task structure */
    task_init(task);
}

/**
 * @brief Get the currently running task
 */
task_t *task_current(void)
{
    return current_task;
}

/**
 * @brief Set the current task (called by scheduler)
 * 
 * Note: This is an internal function used by the scheduler.
 * 
 * @param task Task to set as current
 */
void task_set_current(task_t *task)
{
    current_task = task;
}

/**
 * @brief Set a task's state
 */
void task_set_state(task_t *task, task_state_t state)
{
    if (task != NULL) {
        task->state = state;
    }
}

/**
 * @brief Get a task's current state
 */
task_state_t task_get_state(task_t *task)
{
    if (task == NULL) {
        return TASK_STATE_UNUSED;
    }
    return task->state;
}

/**
 * @brief Set a task's priority
 */
void task_set_priority(task_t *task, uint8_t priority)
{
    if (task != NULL) {
        if (priority >= MAX_PRIORITY_LEVELS) {
            priority = MAX_PRIORITY_LEVELS - 1;
        }
        task->priority = priority;
    }
}

/**
 * @brief Get a task's current priority
 */
uint8_t task_get_priority(task_t *task)
{
    if (task == NULL) {
        return TASK_PRIORITY_LOWEST;
    }
    return task->priority;
}

/**
 * @brief Find a task by PID
 */
task_t *task_get_by_pid(uint32_t pid)
{
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].state != TASK_STATE_UNUSED &&
            task_table[i].pid == pid) {
            return &task_table[i];
        }
    }
    return NULL;
}

/**
 * @brief Get the total number of active tasks
 */
uint32_t task_count(void)
{
    return active_task_count;
}

/**
 * @brief Get a string representation of a task state
 */
const char *task_state_name(task_state_t state)
{
    switch (state) {
        case TASK_STATE_UNUSED:     return "UNUSED";
        case TASK_STATE_CREATING:   return "CREATING";
        case TASK_STATE_READY:      return "READY";
        case TASK_STATE_RUNNING:    return "RUNNING";
        case TASK_STATE_BLOCKED:    return "BLOCKED";
        case TASK_STATE_SLEEPING:   return "SLEEPING";
        case TASK_STATE_TERMINATED: return "TERMINATED";
        case TASK_STATE_ZOMBIE:     return "ZOMBIE";
        default:                    return "UNKNOWN";
    }
}

/**
 * @brief Yield the CPU voluntarily
 */
void task_yield(void)
{
    /* Request a reschedule */
    schedule();
}

/**
 * @brief Put the current task to sleep
 */
void task_sleep(uint32_t ticks)
{
    if (current_task == NULL || ticks == 0) {
        return;
    }

    /* Disable interrupts during state change */
    cpu_cli();

    /* Calculate wake-up time */
    current_task->sleep_until = pit_get_ticks() + ticks;
    current_task->state = TASK_STATE_SLEEPING;

    /* Re-enable interrupts and schedule another task */
    cpu_sti();
    schedule();
}

/**
 * @brief Wake up a sleeping/blocked task
 */
void task_wakeup(task_t *task)
{
    if (task == NULL) {
        return;
    }

    /* Only wake sleeping or blocked tasks */
    if (task->state == TASK_STATE_SLEEPING || 
        task->state == TASK_STATE_BLOCKED) {
        task->state = TASK_STATE_READY;
        task->sleep_until = 0;
        
        /* Re-add to scheduler ready queue */
        scheduler_add_task(task);
    }
}

/**
 * @brief Get a task from the task table by index
 * 
 * Used by scheduler for iterating tasks.
 * 
 * @param index Index into task table (0 to MAX_TASKS-1)
 * @return Pointer to task, or NULL if index out of range
 */
task_t *task_get_by_index(uint32_t index)
{
    if (index >= MAX_TASKS) {
        return NULL;
    }
    return &task_table[index];
}

/**
 * @brief Check if task system is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool task_system_is_initialized(void)
{
    return task_system_initialized;
}
