#ifndef NEXA_LIST_H
#define NEXA_LIST_H

#include <stddef.h>
#include <stdbool.h>

/*
 * lib/dsa/list.h
 *
 * Generic Doubly Linked List
 *
 * This header defines a generic, intrusive doubly linked list. "Intrusive" means
 * the list node structure (`list_node_t`) is embedded within the data structure
 * it holds, allowing for efficient, allocation-free manipulation of list items.
 *
 * It is a core primitive used throughout the kernel (e.g., for task queues).
 */

typedef struct list_node {
    struct list_node *next;
    struct list_node *prev;
} list_node_t;

typedef struct list {
    list_node_t *head;
    list_node_t *tail;
    size_t size;
} list_t;

/* Initialize a list */
void list_init(list_t *list);

/* Initialize a node (optional, but good practice) */
void list_node_init(list_node_t *node);

/* Check if list is empty */
bool list_is_empty(list_t *list);

/* Add to the front of the list */
void list_push_front(list_t *list, list_node_t *node);

/* Add to the back of the list */
void list_push_back(list_t *list, list_node_t *node);

/* Remove from the front of the list */
list_node_t *list_pop_front(list_t *list);

/* Remove from the back of the list */
list_node_t *list_pop_back(list_t *list);

/* Remove a specific node from the list */
void list_remove(list_t *list, list_node_t *node);

/* Get the size of the list */
size_t list_size(list_t *list);

/* Macro to get the container struct from the list node */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* Helper macro for offsetof if not available (though stddef.h should have it) */
#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

/* Iterate over a list */
#define list_for_each(pos, list) \
    for (pos = (list)->head; pos != NULL; pos = pos->next)

/* Iterate safely (allows removal during iteration) */
#define list_for_each_safe(pos, n, list) \
    for (pos = (list)->head, n = (pos ? pos->next : NULL); pos != NULL; pos = n, n = (pos ? pos->next : NULL))

#endif /* NEXA_LIST_H */
