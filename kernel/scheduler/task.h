/*
 * ===========================================================================
 * kernel/scheduler/task.h
 * ===========================================================================
 *
 * Task Control Block (TCB) Structure and Task Management API
 *
 * This header defines the Task Control Block structure which contains all
 * information needed to save and restore a task's execution state. It also
 * provides the API for task creation, destruction, and state management.
 *
 * Task Lifecycle:
 * ┌─────────────┐     create      ┌─────────────┐
 * │   (none)    │ ───────────────>│    READY    │
 * └─────────────┘                 └──────┬──────┘
 *                                        │ schedule
 *                                        v
 *                                 ┌─────────────┐
 *       ┌────────────────────────>│   RUNNING   │<───────┐
 *       │ I/O complete            └──────┬──────┘        │
 *       │                                │               │
 *       │                   ┌────────────┴────────────┐  │
 *       │                   │                         │  │
 *       │                   v (preempt)     (block)   v  │
 *       │            ┌─────────────┐          ┌──────────┴──┐
 *       │            │    READY    │          │   BLOCKED   │
 *       │            └─────────────┘          └─────────────┘
 *       │                                            │
 *       └────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#ifndef NEXA_TASK_H
#define NEXA_TASK_H

#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * Forward Declarations
 * --------------------------------------------------------------------------- */
struct task;
typedef struct task task_t;

/* ---------------------------------------------------------------------------
 * Task State Enumeration
 * ---------------------------------------------------------------------------
 * Defines all possible states a task can be in during its lifecycle.
 * --------------------------------------------------------------------------- */
typedef enum task_state {
    TASK_STATE_UNUSED = 0,      /* TCB slot is free (not a valid task) */
    TASK_STATE_CREATING,        /* Task is being initialized */
    TASK_STATE_READY,           /* Task is ready to run, waiting in queue */
    TASK_STATE_RUNNING,         /* Task is currently executing on CPU */
    TASK_STATE_BLOCKED,         /* Task is waiting for an event (I/O, sleep) */
    TASK_STATE_SLEEPING,        /* Task is sleeping (will wake after timeout) */
    TASK_STATE_TERMINATED,      /* Task has finished execution */
    TASK_STATE_ZOMBIE           /* Task terminated but not yet cleaned up */
} task_state_t;

/* ---------------------------------------------------------------------------
 * Task Priority Levels
 * ---------------------------------------------------------------------------
 * Lower values indicate higher priority (like Unix nice values).
 * Priority 0 is highest, MAX_PRIORITY_LEVELS-1 is lowest.
 * --------------------------------------------------------------------------- */
#define TASK_PRIORITY_HIGHEST       0
#define TASK_PRIORITY_HIGH          1
#define TASK_PRIORITY_ABOVE_NORMAL  2
#define TASK_PRIORITY_NORMAL        3
#define TASK_PRIORITY_BELOW_NORMAL  4
#define TASK_PRIORITY_LOW           5
#define TASK_PRIORITY_LOWEST        6
#define TASK_PRIORITY_IDLE          7

/* Default priority for new tasks */
#define TASK_PRIORITY_DEFAULT       TASK_PRIORITY_NORMAL

/* ---------------------------------------------------------------------------
 * Task Flags
 * ---------------------------------------------------------------------------
 * Bit flags for special task behaviors and states.
 * --------------------------------------------------------------------------- */
#define TASK_FLAG_KERNEL        (1 << 0)    /* Kernel task (ring 0) */
#define TASK_FLAG_USER          (1 << 1)    /* User task (ring 3) - future */
#define TASK_FLAG_PREEMPTIBLE   (1 << 2)    /* Can be preempted */
#define TASK_FLAG_IDLE          (1 << 3)    /* Idle task (lowest priority) */
#define TASK_FLAG_FIRST_RUN     (1 << 4)    /* Task hasn't run yet */
#define TASK_FLAG_NEEDS_CLEANUP (1 << 5)    /* Resources need cleanup */

/* ---------------------------------------------------------------------------
 * CPU Register Context
 * ---------------------------------------------------------------------------
 * This structure holds all CPU registers that must be saved/restored
 * during a context switch. The layout must match context_switch.asm.
 *
 * Stack layout after context save (from bottom/high address to top/low):
 *   [high address]
 *   SS (if ring change)
 *   ESP (if ring change)
 *   EFLAGS
 *   CS
 *   EIP            <- return address
 *   EAX
 *   ECX
 *   EDX
 *   EBX
 *   ESP (original) <- ignored on restore
 *   EBP
 *   ESI
 *   EDI
 *   [low address]  <- saved ESP points here
 * --------------------------------------------------------------------------- */
typedef struct cpu_context {
    /* Saved by context_switch (PUSHA order, but we pop in reverse) */
    uint32_t edi;               /* Offset 0 */
    uint32_t esi;               /* Offset 4 */
    uint32_t ebp;               /* Offset 8 */
    uint32_t esp_dummy;         /* Offset 12 - ESP from PUSHA, not used */
    uint32_t ebx;               /* Offset 16 */
    uint32_t edx;               /* Offset 20 */
    uint32_t ecx;               /* Offset 24 */
    uint32_t eax;               /* Offset 28 */

    /* Return address (pushed by call or manually set for first run) */
    uint32_t eip;               /* Offset 32 */
} __attribute__((packed)) cpu_context_t;

/* Size of the CPU context on the stack */
#define CPU_CONTEXT_SIZE    sizeof(cpu_context_t)

/* ---------------------------------------------------------------------------
 * Task Control Block (TCB)
 * ---------------------------------------------------------------------------
 * The central data structure for representing a task. Contains:
 * - Execution context (CPU registers, stack)
 * - Scheduling information (priority, state, time accounting)
 * - Task identification (PID, name)
 * - Linkage for scheduler queues
 * --------------------------------------------------------------------------- */
struct task {
    /*
     * Stack and Context
     * -----------------
     * stack_pointer: Points to the top of the task's kernel stack.
     *                On context switch, this is saved/restored.
     * stack_base:    Points to the bottom (start) of allocated stack memory.
     * stack_size:    Size of allocated stack in bytes.
     */
    uint32_t *stack_pointer;        /* Current stack pointer (saved on switch) */
    void *stack_base;               /* Base of allocated stack memory */
    size_t stack_size;              /* Stack size in bytes */

    /*
     * Task Identification
     * -------------------
     * pid:           Unique process/task identifier
     * name:          Human-readable task name (for debugging)
     */
    uint32_t pid;                   /* Process ID (unique identifier) */
    char name[32];                  /* Task name (null-terminated) */

    /*
     * Scheduling Information
     * ----------------------
     * state:         Current task state (READY, RUNNING, etc.)
     * priority:      Task priority level (lower = higher priority)
     * base_priority: Original priority (used after temporary boosts)
     * flags:         Task behavior flags
     * time_slice:    Remaining time slice in ticks
     */
    task_state_t state;             /* Current task state */
    uint8_t priority;               /* Current priority (0-7) */
    uint8_t base_priority;          /* Base priority level */
    uint16_t flags;                 /* Task flags (TASK_FLAG_*) */
    uint32_t time_slice;            /* Remaining time slice (ticks) */

    /*
     * Time Accounting
     * ---------------
     * cpu_time:      Total CPU time used by this task (in ticks)
     * start_time:    System tick when task was created
     * sleep_until:   System tick when sleeping task should wake
     */
    uint32_t cpu_time;              /* Total CPU ticks consumed */
    uint32_t start_time;            /* Creation time (tick count) */
    uint32_t sleep_until;           /* Wake-up time for sleeping tasks */

    /*
     * Task Entry Point
     * ----------------
     * entry_point:   Function to execute when task starts
     * arg:           Argument passed to entry point function
     */
    void (*entry_point)(void *);    /* Task entry function */
    void *arg;                      /* Argument to entry function */

    /*
     * Exit Information
     * ----------------
     * exit_code:     Value returned when task terminates
     */
    int32_t exit_code;              /* Task exit code */

    /*
     * Queue Linkage (for intrusive lists)
     * -----------------------------------
     * These pointers allow the task to be linked into scheduler queues
     * without additional memory allocation.
     */
    task_t *next;                   /* Next task in queue */
    task_t *prev;                   /* Previous task in queue (if doubly-linked) */
};

/* ---------------------------------------------------------------------------
 * Task Table
 * ---------------------------------------------------------------------------
 * The kernel maintains a fixed-size table of all tasks. Tasks are created
 * by finding a free slot (TASK_STATE_UNUSED) and initializing it.
 * --------------------------------------------------------------------------- */

/* Maximum number of concurrent tasks (from os_config.h) */
#ifndef MAX_TASKS
#define MAX_TASKS 64
#endif

/* Default stack size for tasks (16 KB) */
#define TASK_STACK_SIZE     (16 * 1024)

/* Minimum stack size (4 KB) */
#define TASK_MIN_STACK_SIZE (4 * 1024)

/* ---------------------------------------------------------------------------
 * Task Management API
 * --------------------------------------------------------------------------- */

/*
 * Initialization
 */

/**
 * @brief Initialize the task subsystem
 * 
 * Must be called before creating any tasks. Sets up the task table
 * and creates the initial kernel task (PID 0).
 * 
 * @return true on success, false on failure
 */
bool task_system_init(void);

/**
 * @brief Initialize a task structure to default values
 * 
 * @param task Pointer to task structure to initialize
 */
void task_init(task_t *task);

/*
 * Task Creation and Destruction
 */

/**
 * @brief Create a new task
 * 
 * Allocates a TCB and stack, initializes the task's context so it will
 * begin execution at the entry point when scheduled.
 * 
 * @param name        Human-readable name (max 31 chars)
 * @param entry_point Function to execute when task starts
 * @param arg         Argument passed to entry point (can be NULL)
 * @param priority    Task priority (0=highest, 7=lowest)
 * @param stack_size  Stack size in bytes (0 for default)
 * 
 * @return Pointer to created task, or NULL on failure
 */
task_t *task_create(const char *name, 
                    void (*entry_point)(void *),
                    void *arg,
                    uint8_t priority,
                    size_t stack_size);

/**
 * @brief Terminate the calling task
 * 
 * The current task exits with the given exit code. Control is transferred
 * to the next ready task. This function does not return.
 * 
 * @param exit_code Value to store as task's exit code
 */
void task_exit(int32_t exit_code);

/**
 * @brief Destroy a task and free its resources
 * 
 * Frees the task's stack and marks the TCB slot as unused.
 * Should only be called on terminated/zombie tasks.
 * 
 * @param task Task to destroy
 */
void task_destroy(task_t *task);

/*
 * Task State Management
 */

/**
 * @brief Get the currently running task
 * 
 * @return Pointer to current task, or NULL if no task is running
 */
task_t *task_current(void);

/**
 * @brief Set a task's state
 * 
 * @param task  Task to modify
 * @param state New state
 */
void task_set_state(task_t *task, task_state_t state);

/**
 * @brief Get a task's current state
 * 
 * @param task Task to query
 * @return Current task state
 */
task_state_t task_get_state(task_t *task);

/**
 * @brief Set a task's priority
 * 
 * @param task     Task to modify
 * @param priority New priority (0-7)
 */
void task_set_priority(task_t *task, uint8_t priority);

/**
 * @brief Get a task's current priority
 * 
 * @param task Task to query
 * @return Current priority level
 */
uint8_t task_get_priority(task_t *task);

/*
 * Task Lookup
 */

/**
 * @brief Find a task by PID
 * 
 * @param pid Process ID to search for
 * @return Pointer to task, or NULL if not found
 */
task_t *task_get_by_pid(uint32_t pid);

/**
 * @brief Get the total number of tasks
 * 
 * @return Number of tasks (including current task)
 */
uint32_t task_count(void);

/*
 * Task Utilities
 */

/**
 * @brief Get a string representation of a task state
 * 
 * @param state Task state to convert
 * @return Const string name (do not free)
 */
const char *task_state_name(task_state_t state);

/**
 * @brief Yield the CPU voluntarily
 * 
 * The current task gives up its remaining time slice and allows
 * the scheduler to run another task.
 */
void task_yield(void);

/**
 * @brief Put the current task to sleep
 * 
 * @param ticks Number of timer ticks to sleep
 */
void task_sleep(uint32_t ticks);

/**
 * @brief Wake up a sleeping/blocked task
 * 
 * @param task Task to wake up
 */
void task_wakeup(task_t *task);

#endif /* NEXA_TASK_H */
