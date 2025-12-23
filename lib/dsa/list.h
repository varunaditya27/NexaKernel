/*
 * ===========================================================================
 * lib/dsa/list.h
 * ===========================================================================
 *
 * Generic Intrusive Doubly Linked List
 *
 * An intrusive linked list embeds the list node directly within the data
 * structure being stored, rather than allocating wrapper nodes. This provides:
 *
 *   - Zero memory allocation overhead for list operations
 *   - O(1) insertion and removal
 *   - Cache-friendly memory layout
 *   - No memory fragmentation from list node allocations
 *
 * This is the same approach used by the Linux kernel (list.h) and is ideal
 * for OS kernel programming where dynamic memory allocation may be unavailable
 * or expensive.
 *
 * Usage Pattern:
 *   struct my_data {
 *       int value;
 *       list_node_t node;  // Embed the list node
 *   };
 *
 *   list_t my_list;
 *   list_init(&my_list);
 *
 *   struct my_data item = { .value = 42 };
 *   list_push_back(&my_list, &item.node);
 *
 *   // Later, to get the data from a node:
 *   struct my_data *ptr = list_entry(node, struct my_data, node);
 *
 * This list implementation is used by:
 *   - Free list heap allocator (kernel/memory/heap_allocator.c)
 *   - Task scheduler queues (kernel/scheduler/)
 *   - Buddy allocator free areas
 *
 * ===========================================================================
 */

#ifndef NEXA_LIST_H
#define NEXA_LIST_H

#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * List Node Structure
 * ---------------------------------------------------------------------------
 * This structure is embedded within data structures that need to be linked.
 * It contains only prev/next pointers - no data payload.
 * --------------------------------------------------------------------------- */
typedef struct list_node {
    struct list_node *next;  /* Pointer to next node (NULL if last) */
    struct list_node *prev;  /* Pointer to previous node (NULL if first) */
} list_node_t;

/* ---------------------------------------------------------------------------
 * List Head Structure
 * ---------------------------------------------------------------------------
 * Contains pointers to the first and last nodes, plus a size counter.
 * The size counter makes list_size() O(1) instead of O(n).
 * --------------------------------------------------------------------------- */
typedef struct list {
    list_node_t *head;  /* First node in the list (NULL if empty) */
    list_node_t *tail;  /* Last node in the list (NULL if empty) */
    size_t size;        /* Number of nodes in the list */
} list_t;

/* ---------------------------------------------------------------------------
 * Initialization Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize a list structure
 * 
 * @param list Pointer to list structure to initialize
 * 
 * Sets head and tail to NULL, size to 0.
 */
void list_init(list_t *list);

/**
 * @brief Initialize a list node
 * 
 * @param node Pointer to node to initialize
 * 
 * Sets prev and next to NULL. This is optional but good practice
 * to ensure nodes start in a known state.
 */
void list_node_init(list_node_t *node);

/* ---------------------------------------------------------------------------
 * Query Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Check if a list is empty
 * 
 * @param list Pointer to list
 * @return true if list is empty or NULL, false otherwise
 */
bool list_is_empty(const list_t *list);

/**
 * @brief Get the number of nodes in the list
 * 
 * @param list Pointer to list
 * @return Number of nodes, or 0 if list is NULL
 */
size_t list_size(const list_t *list);

/* ---------------------------------------------------------------------------
 * Insertion Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Add a node to the front of the list
 * 
 * @param list Pointer to list
 * @param node Node to add (must not already be in a list)
 * 
 * Time complexity: O(1)
 */
void list_push_front(list_t *list, list_node_t *node);

/**
 * @brief Add a node to the back of the list
 * 
 * @param list Pointer to list
 * @param node Node to add (must not already be in a list)
 * 
 * Time complexity: O(1)
 */
void list_push_back(list_t *list, list_node_t *node);

/**
 * @brief Insert a node after another node
 * 
 * @param list  Pointer to list
 * @param after Node to insert after (must be in list)
 * @param node  New node to insert
 * 
 * Time complexity: O(1)
 */
void list_insert_after(list_t *list, list_node_t *after, list_node_t *node);

/**
 * @brief Insert a node before another node
 * 
 * @param list   Pointer to list
 * @param before Node to insert before (must be in list)
 * @param node   New node to insert
 * 
 * Time complexity: O(1)
 */
void list_insert_before(list_t *list, list_node_t *before, list_node_t *node);

/* ---------------------------------------------------------------------------
 * Removal Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Remove and return the first node
 * 
 * @param list Pointer to list
 * @return Removed node, or NULL if list is empty
 * 
 * Time complexity: O(1)
 */
list_node_t *list_pop_front(list_t *list);

/**
 * @brief Remove and return the last node
 * 
 * @param list Pointer to list
 * @return Removed node, or NULL if list is empty
 * 
 * Time complexity: O(1)
 */
list_node_t *list_pop_back(list_t *list);

/**
 * @brief Remove a specific node from the list
 * 
 * @param list Pointer to list
 * @param node Node to remove (must be in this list)
 * 
 * Time complexity: O(1)
 */
void list_remove(list_t *list, list_node_t *node);

/* ---------------------------------------------------------------------------
 * Utility Macros
 * --------------------------------------------------------------------------- */

/**
 * @brief Get the containing structure from a list node
 * 
 * Given a pointer to a list_node_t embedded within a structure, this macro
 * returns a pointer to the containing structure.
 * 
 * @param ptr    Pointer to the list_node_t member
 * @param type   Type of the containing structure
 * @param member Name of the list_node_t member within the structure
 * @return Pointer to the containing structure
 * 
 * Example:
 *   struct task { int id; list_node_t node; };
 *   struct task *t = list_entry(node_ptr, struct task, node);
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* Provide offsetof if not available */
#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

/**
 * @brief Iterate over all nodes in a list
 * 
 * @param pos  Loop variable (list_node_t *)
 * @param list Pointer to list
 * 
 * WARNING: Do not modify the list during iteration!
 * Use list_for_each_safe for removal during iteration.
 */
#define list_for_each(pos, list) \
    for ((pos) = (list)->head; (pos) != NULL; (pos) = (pos)->next)

/**
 * @brief Iterate over all nodes safely (allows removal)
 * 
 * @param pos  Loop variable (list_node_t *)
 * @param n    Temporary variable (list_node_t *)
 * @param list Pointer to list
 * 
 * This version stores the next pointer before the loop body executes,
 * allowing the current node to be safely removed.
 */
#define list_for_each_safe(pos, n, list) \
    for ((pos) = (list)->head, (n) = ((pos) ? (pos)->next : NULL); \
         (pos) != NULL; \
         (pos) = (n), (n) = ((pos) ? (pos)->next : NULL))

/**
 * @brief Iterate over all entries (containing structures)
 * 
 * @param pos    Loop variable (pointer to containing struct type)
 * @param list   Pointer to list
 * @param type   Type of the containing structure
 * @param member Name of the list_node_t member
 */
#define list_for_each_entry(pos, list, type, member) \
    for ((pos) = ((list)->head ? list_entry((list)->head, type, member) : NULL); \
         (pos) != NULL; \
         (pos) = ((pos)->member.next ? list_entry((pos)->member.next, type, member) : NULL))

#endif /* NEXA_LIST_H */
