/*
 * ===========================================================================
 * kernel/scheduler/dsa_structures.h
 * ===========================================================================
 *
 * Scheduler Data Structure Wrappers
 *
 * This header declares the data structure wrappers used by the scheduler
 * for managing task queues. These wrappers adapt generic DSA implementations
 * (circular queue, heap) for task scheduling purposes.
 *
 * Two scheduling policies are supported:
 *
 * 1. Round-Robin (RR)
 *    - FIFO queue of ready tasks
 *    - Tasks rotate through, each getting a time slice
 *    - Simple, fair, prevents starvation
 *
 * 2. Priority-Based
 *    - Min-heap of ready tasks, ordered by priority
 *    - Lower priority value = higher priority (like nice values)
 *    - Higher priority tasks always run first
 *    - Can cause starvation without aging
 *
 * The scheduler can use either policy (configured in os_config.h) or
 * combine them (priority queues within round-robin).
 *
 * ===========================================================================
 */

#ifndef NEXA_SCHEDULER_DSA_H
#define NEXA_SCHEDULER_DSA_H

#include "../../config/os_config.h"
#include "task.h"

/* ---------------------------------------------------------------------------
 * Scheduler Policy Configuration
 * ---------------------------------------------------------------------------
 * Scheduling policy constants are defined in scheduler.h to avoid
 * duplicate definitions. Include scheduler.h for SCHED_POLICY_* macros.
 * 
 * Policies:
 *   SCHED_POLICY_ROUND_ROBIN (0) - FIFO queue with time slicing
 *   SCHED_POLICY_PRIORITY    (1) - Min-heap by priority value
 *   SCHED_POLICY_MLFQ        (2) - Multi-Level Feedback Queue (future)
 * --------------------------------------------------------------------------- */

/* Only define if not already defined (avoid redefinition) */
#ifndef SCHED_POLICY_ROUND_ROBIN
#define SCHED_POLICY_ROUND_ROBIN    0
#define SCHED_POLICY_PRIORITY       1
#define SCHED_POLICY_MLFQ           2   /* Multi-Level Feedback Queue (future) */
#endif

/* Default scheduling policy */
#ifndef SCHEDULER_POLICY
#define SCHEDULER_POLICY    SCHED_POLICY_ROUND_ROBIN
#endif

/* ---------------------------------------------------------------------------
 * Round-Robin Queue API
 * ---------------------------------------------------------------------------
 * Implements a circular queue (FIFO) for round-robin task scheduling.
 * Tasks are added to the back and removed from the front.
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the round-robin queue
 * 
 * @param capacity Maximum number of tasks the queue can hold
 * @return true on success, false on failure (e.g., out of memory)
 */
bool rr_queue_init(size_t capacity);

/**
 * @brief Destroy the round-robin queue and free resources
 */
void rr_queue_destroy(void);

/**
 * @brief Add a task to the back of the queue
 * 
 * @param task Task to enqueue
 * @return true on success, false if queue is full
 */
bool rr_enqueue(task_t *task);

/**
 * @brief Remove and return the task at the front of the queue
 * 
 * @return Task pointer, or NULL if queue is empty
 */
task_t *rr_dequeue(void);

/**
 * @brief Look at the task at the front without removing it
 * 
 * @return Task pointer, or NULL if queue is empty
 */
task_t *rr_peek(void);

/**
 * @brief Check if the round-robin queue is empty
 * 
 * @return true if empty, false otherwise
 */
bool rr_is_empty(void);

/**
 * @brief Check if the round-robin queue is full
 * 
 * @return true if full, false otherwise
 */
bool rr_is_full(void);

/**
 * @brief Get the number of tasks in the queue
 * 
 * @return Number of tasks
 */
size_t rr_count(void);

/**
 * @brief Remove a specific task from the queue
 * 
 * Searches the queue for the task and removes it.
 * Used when a task blocks or terminates.
 * 
 * @param task Task to remove
 * @return true if found and removed, false otherwise
 */
bool rr_remove(task_t *task);

/* ---------------------------------------------------------------------------
 * Priority Queue API
 * ---------------------------------------------------------------------------
 * Implements a min-heap for priority-based task scheduling.
 * Tasks with lower priority values are considered higher priority
 * and will be dequeued first.
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the priority queue
 * 
 * @param capacity Maximum number of tasks the queue can hold
 * @return true on success, false on failure
 */
bool pq_init(size_t capacity);

/**
 * @brief Destroy the priority queue and free resources
 */
void pq_destroy(void);

/**
 * @brief Add a task to the priority queue
 * 
 * The task is inserted in the correct position based on its priority.
 * 
 * @param task Task to enqueue
 * @return true on success, false if queue is full
 */
bool pq_enqueue(task_t *task);

/**
 * @brief Remove and return the highest-priority task
 * 
 * @return Task pointer, or NULL if queue is empty
 */
task_t *pq_dequeue(void);

/**
 * @brief Look at the highest-priority task without removing it
 * 
 * @return Task pointer, or NULL if queue is empty
 */
task_t *pq_peek(void);

/**
 * @brief Check if the priority queue is empty
 * 
 * @return true if empty, false otherwise
 */
bool pq_is_empty(void);

/**
 * @brief Check if the priority queue is full
 * 
 * @return true if full, false otherwise
 */
bool pq_is_full(void);

/**
 * @brief Get the number of tasks in the priority queue
 * 
 * @return Number of tasks
 */
size_t pq_count(void);

/**
 * @brief Remove a specific task from the priority queue
 * 
 * @param task Task to remove
 * @return true if found and removed, false otherwise
 */
bool pq_remove(task_t *task);

/**
 * @brief Update a task's position after priority change
 * 
 * Call this after changing a task's priority to maintain heap property.
 * 
 * @param task Task whose priority was changed
 */
void pq_update(task_t *task);

#endif /* NEXA_SCHEDULER_DSA_H */
