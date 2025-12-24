/*
 * ===========================================================================
 * kernel/scheduler/dsa_structures/priority_queue.c
 * ===========================================================================
 *
 * Priority Queue Implementation (Min-Heap)
 *
 * This file implements a priority queue using a binary min-heap data structure.
 * It is used for priority-based task scheduling where tasks with lower priority
 * values are considered higher priority (like Unix nice values).
 *
 * Data Structure: Binary Heap (Min-Heap)
 * ┌───────────────────────────────────────────────────────────────────────────┐
 * │  A binary heap is a complete binary tree stored as an array where:       │
 * │  - Parent at index i has children at 2i+1 (left) and 2i+2 (right)        │
 * │  - Child at index i has parent at (i-1)/2                                │
 * │  - Every parent is less than or equal to its children (min-heap)         │
 * │                                                                          │
 * │  Example (tasks by priority):                                            │
 * │                    [0]                                                   │
 * │                   P=1                                                    │
 * │                 /      \                                                 │
 * │              [1]        [2]                                              │
 * │             P=2         P=3                                              │
 * │            /   \       /   \                                             │
 * │         [3]    [4]  [5]    [6]                                           │
 * │         P=5    P=4  P=3    P=7                                           │
 * │                                                                          │
 * │  Array: [P1, P2, P3, P5, P4, P3, P7]                                     │
 * │                                                                          │
 * │  Operations:                                                             │
 * │  - Insert: Add at end, bubble up    O(log n)                             │
 * │  - Extract: Remove root, bubble down O(log n)                            │
 * │  - Peek: Return root                O(1)                                 │
 * │  - Remove: Find, swap, bubble       O(n)                                 │
 * └───────────────────────────────────────────────────────────────────────────┘
 *
 * ===========================================================================
 */

#include "../dsa_structures.h"
#include "../../memory/memory.h"

/* ---------------------------------------------------------------------------
 * Priority Queue Structure
 * --------------------------------------------------------------------------- */
typedef struct priority_queue {
    task_t **buffer;        /* Array of task pointers */
    size_t capacity;        /* Maximum number of tasks */
    size_t size;            /* Current number of tasks */
} priority_queue_t;

/* Static instance of the priority queue */
static priority_queue_t pq = {
    .buffer = NULL,
    .capacity = 0,
    .size = 0
};

/* ---------------------------------------------------------------------------
 * Internal Helper Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Get parent index in the heap
 */
static inline size_t parent(size_t i)
{
    return (i - 1) / 2;
}

/**
 * @brief Get left child index in the heap
 */
static inline size_t left_child(size_t i)
{
    return 2 * i + 1;
}

/**
 * @brief Get right child index in the heap
 */
static inline size_t right_child(size_t i)
{
    return 2 * i + 2;
}

/**
 * @brief Compare two tasks by priority
 * 
 * Returns negative if a has higher priority (lower value),
 * positive if b has higher priority, zero if equal.
 */
static int task_compare(task_t *a, task_t *b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    
    /* Lower priority value = higher priority */
    if (a->priority < b->priority) {
        return -1;  /* a has higher priority */
    } else if (a->priority > b->priority) {
        return 1;   /* b has higher priority */
    }
    
    /* If same priority, prefer task that's been waiting longer (lower PID as tie-breaker) */
    if (a->pid < b->pid) {
        return -1;
    } else if (a->pid > b->pid) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief Swap two elements in the buffer
 */
static void swap(size_t i, size_t j)
{
    task_t *temp = pq.buffer[i];
    pq.buffer[i] = pq.buffer[j];
    pq.buffer[j] = temp;
}

/**
 * @brief Restore heap property by bubbling up
 * 
 * Used after insertion. Moves element up the tree until heap property is satisfied.
 */
static void heapify_up(size_t index)
{
    /*
     * While we're not at the root and parent has lower priority:
     * Swap with parent and continue up
     */
    while (index > 0) {
        size_t p = parent(index);
        
        /* If parent has higher or equal priority, stop */
        if (task_compare(pq.buffer[p], pq.buffer[index]) <= 0) {
            break;
        }
        
        /* Parent has lower priority, swap */
        swap(index, p);
        index = p;
    }
}

/**
 * @brief Restore heap property by bubbling down
 * 
 * Used after extraction. Moves element down the tree until heap property is satisfied.
 */
static void heapify_down(size_t index)
{
    while (true) {
        size_t smallest = index;
        size_t left = left_child(index);
        size_t right = right_child(index);
        
        /* Check if left child has higher priority */
        if (left < pq.size && 
            task_compare(pq.buffer[left], pq.buffer[smallest]) < 0) {
            smallest = left;
        }
        
        /* Check if right child has higher priority */
        if (right < pq.size && 
            task_compare(pq.buffer[right], pq.buffer[smallest]) < 0) {
            smallest = right;
        }
        
        /* If current position is correct, stop */
        if (smallest == index) {
            break;
        }
        
        /* Swap and continue down */
        swap(index, smallest);
        index = smallest;
    }
}

/**
 * @brief Find a task in the priority queue
 * 
 * @return Index of task, or pq.size if not found
 */
static size_t find_task(task_t *task)
{
    for (size_t i = 0; i < pq.size; i++) {
        if (pq.buffer[i] == task) {
            return i;
        }
    }
    return pq.size;  /* Not found */
}

/* ---------------------------------------------------------------------------
 * Public API Implementation
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the priority queue
 */
bool pq_init(size_t capacity)
{
    /* Validate capacity */
    if (capacity == 0) {
        return false;
    }

    /* Clean up any existing queue */
    if (pq.buffer != NULL) {
        pq_destroy();
    }

    /* Allocate buffer */
    pq.buffer = (task_t **)kmalloc(capacity * sizeof(task_t *));
    if (pq.buffer == NULL) {
        return false;  /* Out of memory */
    }

    /* Initialize all slots to NULL */
    for (size_t i = 0; i < capacity; i++) {
        pq.buffer[i] = NULL;
    }

    /* Initialize queue state */
    pq.capacity = capacity;
    pq.size = 0;

    return true;
}

/**
 * @brief Destroy the priority queue
 */
void pq_destroy(void)
{
    if (pq.buffer != NULL) {
        kfree(pq.buffer);
        pq.buffer = NULL;
    }
    pq.capacity = 0;
    pq.size = 0;
}

/**
 * @brief Add a task to the priority queue
 * 
 * Time Complexity: O(log n)
 */
bool pq_enqueue(task_t *task)
{
    /* Validate parameters */
    if (task == NULL || pq.buffer == NULL) {
        return false;
    }

    /* Check if queue is full */
    if (pq.size >= pq.capacity) {
        return false;
    }

    /* Add task at the end */
    pq.buffer[pq.size] = task;
    pq.size++;

    /* Restore heap property by bubbling up */
    heapify_up(pq.size - 1);

    return true;
}

/**
 * @brief Remove and return the highest-priority task
 * 
 * Time Complexity: O(log n)
 */
task_t *pq_dequeue(void)
{
    /* Check if queue is empty */
    if (pq.buffer == NULL || pq.size == 0) {
        return NULL;
    }

    /* Save the root (highest priority task) */
    task_t *task = pq.buffer[0];

    /* Move last element to root */
    pq.size--;
    if (pq.size > 0) {
        pq.buffer[0] = pq.buffer[pq.size];
        pq.buffer[pq.size] = NULL;
        
        /* Restore heap property by bubbling down */
        heapify_down(0);
    } else {
        pq.buffer[0] = NULL;
    }

    return task;
}

/**
 * @brief Look at the highest-priority task without removing it
 * 
 * Time Complexity: O(1)
 */
task_t *pq_peek(void)
{
    if (pq.buffer == NULL || pq.size == 0) {
        return NULL;
    }
    return pq.buffer[0];
}

/**
 * @brief Check if priority queue is empty
 * 
 * Time Complexity: O(1)
 */
bool pq_is_empty(void)
{
    return (pq.buffer == NULL || pq.size == 0);
}

/**
 * @brief Check if priority queue is full
 * 
 * Time Complexity: O(1)
 */
bool pq_is_full(void)
{
    return (pq.buffer != NULL && pq.size >= pq.capacity);
}

/**
 * @brief Get the current number of tasks
 * 
 * Time Complexity: O(1)
 */
size_t pq_count(void)
{
    return pq.size;
}

/**
 * @brief Remove a specific task from the priority queue
 * 
 * Time Complexity: O(n) for search + O(log n) for re-heapify
 */
bool pq_remove(task_t *task)
{
    /* Validate parameters */
    if (task == NULL || pq.buffer == NULL || pq.size == 0) {
        return false;
    }

    /* Find the task */
    size_t index = find_task(task);
    if (index >= pq.size) {
        return false;  /* Not found */
    }

    /* Move last element to this position */
    pq.size--;
    if (index < pq.size) {
        pq.buffer[index] = pq.buffer[pq.size];
        pq.buffer[pq.size] = NULL;

        /*
         * Restore heap property.
         * The replacement element could violate heap property in either direction:
         * - If it has higher priority than parent, bubble up
         * - If it has lower priority than children, bubble down
         */
        if (index > 0 && task_compare(pq.buffer[index], pq.buffer[parent(index)]) < 0) {
            heapify_up(index);
        } else {
            heapify_down(index);
        }
    } else {
        pq.buffer[index] = NULL;
    }

    return true;
}

/**
 * @brief Update a task's position after priority change
 * 
 * Time Complexity: O(n) for search + O(log n) for re-heapify
 */
void pq_update(task_t *task)
{
    /* Validate parameters */
    if (task == NULL || pq.buffer == NULL || pq.size == 0) {
        return;
    }

    /* Find the task */
    size_t index = find_task(task);
    if (index >= pq.size) {
        return;  /* Not found */
    }

    /*
     * Priority may have increased or decreased.
     * Try to bubble up first, then bubble down.
     */
    if (index > 0 && task_compare(pq.buffer[index], pq.buffer[parent(index)]) < 0) {
        heapify_up(index);
    } else {
        heapify_down(index);
    }
}
