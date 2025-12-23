/*
 * ===========================================================================
 * lib/dsa/list.c
 * ===========================================================================
 *
 * Generic Intrusive Doubly Linked List Implementation
 *
 * This file implements all operations for the intrusive doubly linked list.
 * All operations are O(1) since we maintain both head and tail pointers
 * and the list is doubly linked.
 *
 * Memory safety notes:
 *   - All public functions check for NULL pointers
 *   - Nodes are reset after removal to prevent dangling references
 *   - Size is always kept in sync with actual node count
 *
 * ===========================================================================
 */

#include "list.h"

/* ---------------------------------------------------------------------------
 * list_init - Initialize a list structure
 * ---------------------------------------------------------------------------
 * Sets the list to an empty state with no nodes.
 * --------------------------------------------------------------------------- */
void list_init(list_t *list)
{
    if (!list) {
        return;
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

/* ---------------------------------------------------------------------------
 * list_node_init - Initialize a list node
 * ---------------------------------------------------------------------------
 * Sets the node's pointers to NULL, ensuring it's in a known clean state.
 * This should be called before the first use of a node, especially for
 * stack-allocated or embedded nodes.
 * --------------------------------------------------------------------------- */
void list_node_init(list_node_t *node)
{
    if (!node) {
        return;
    }
    
    node->next = NULL;
    node->prev = NULL;
}

/* ---------------------------------------------------------------------------
 * list_is_empty - Check if a list is empty
 * ---------------------------------------------------------------------------
 * Returns true if the list has no nodes, or if the list pointer is NULL.
 * --------------------------------------------------------------------------- */
bool list_is_empty(const list_t *list)
{
    return (list == NULL) || (list->size == 0);
}

/* ---------------------------------------------------------------------------
 * list_size - Get the number of nodes in the list
 * ---------------------------------------------------------------------------
 * Returns the current count of nodes. This is O(1) since we track size.
 * --------------------------------------------------------------------------- */
size_t list_size(const list_t *list)
{
    return list ? list->size : 0;
}

/* ---------------------------------------------------------------------------
 * list_push_front - Add a node to the front of the list
 * ---------------------------------------------------------------------------
 * The new node becomes the head. If the list was empty, it also becomes
 * the tail.
 *
 * Before: [head] -> [A] -> [B] -> [tail]
 * After:  [head] -> [new] -> [A] -> [B] -> [tail]
 * --------------------------------------------------------------------------- */
void list_push_front(list_t *list, list_node_t *node)
{
    if (!list || !node) {
        return;
    }

    /* Set up the new node's pointers */
    node->next = list->head;
    node->prev = NULL;

    /* Update the old head's prev pointer, if it exists */
    if (list->head) {
        list->head->prev = node;
    } else {
        /* List was empty - new node is also the tail */
        list->tail = node;
    }

    /* Update head to point to new node */
    list->head = node;
    list->size++;
}

/* ---------------------------------------------------------------------------
 * list_push_back - Add a node to the back of the list
 * ---------------------------------------------------------------------------
 * The new node becomes the tail. If the list was empty, it also becomes
 * the head.
 *
 * Before: [head] -> [A] -> [B] -> [tail]
 * After:  [head] -> [A] -> [B] -> [new] -> [tail]
 * --------------------------------------------------------------------------- */
void list_push_back(list_t *list, list_node_t *node)
{
    if (!list || !node) {
        return;
    }

    /* Set up the new node's pointers */
    node->next = NULL;
    node->prev = list->tail;

    /* Update the old tail's next pointer, if it exists */
    if (list->tail) {
        list->tail->next = node;
    } else {
        /* List was empty - new node is also the head */
        list->head = node;
    }

    /* Update tail to point to new node */
    list->tail = node;
    list->size++;
}

/* ---------------------------------------------------------------------------
 * list_insert_after - Insert a node after another node
 * ---------------------------------------------------------------------------
 * Inserts 'node' immediately after 'after' in the list.
 *
 * Before: [A] -> [after] -> [B]
 * After:  [A] -> [after] -> [node] -> [B]
 * --------------------------------------------------------------------------- */
void list_insert_after(list_t *list, list_node_t *after, list_node_t *node)
{
    if (!list || !after || !node) {
        return;
    }

    /* Set up the new node's pointers */
    node->prev = after;
    node->next = after->next;

    /* Update surrounding nodes */
    if (after->next) {
        after->next->prev = node;
    } else {
        /* 'after' was the tail - 'node' is now the tail */
        list->tail = node;
    }
    
    after->next = node;
    list->size++;
}

/* ---------------------------------------------------------------------------
 * list_insert_before - Insert a node before another node
 * ---------------------------------------------------------------------------
 * Inserts 'node' immediately before 'before' in the list.
 *
 * Before: [A] -> [before] -> [B]
 * After:  [A] -> [node] -> [before] -> [B]
 * --------------------------------------------------------------------------- */
void list_insert_before(list_t *list, list_node_t *before, list_node_t *node)
{
    if (!list || !before || !node) {
        return;
    }

    /* Set up the new node's pointers */
    node->next = before;
    node->prev = before->prev;

    /* Update surrounding nodes */
    if (before->prev) {
        before->prev->next = node;
    } else {
        /* 'before' was the head - 'node' is now the head */
        list->head = node;
    }
    
    before->prev = node;
    list->size++;
}

/* ---------------------------------------------------------------------------
 * list_pop_front - Remove and return the first node
 * ---------------------------------------------------------------------------
 * Removes the head node and returns it. The caller is responsible for
 * the returned node's memory.
 *
 * Before: [head] -> [A] -> [B] -> [tail]
 * After:  [head] -> [B] -> [tail]
 * Returns: [A] (with next/prev set to NULL)
 * --------------------------------------------------------------------------- */
list_node_t *list_pop_front(list_t *list)
{
    if (!list || !list->head) {
        return NULL;
    }

    /* Save the node we're removing */
    list_node_t *node = list->head;
    
    /* Update head to the next node */
    list->head = node->next;

    /* Update the new head's prev pointer, or clear tail if list is now empty */
    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    /* Clean up the removed node's pointers */
    node->next = NULL;
    node->prev = NULL;
    list->size--;

    return node;
}

/* ---------------------------------------------------------------------------
 * list_pop_back - Remove and return the last node
 * ---------------------------------------------------------------------------
 * Removes the tail node and returns it. The caller is responsible for
 * the returned node's memory.
 *
 * Before: [head] -> [A] -> [B] -> [tail]
 * After:  [head] -> [A] -> [tail]
 * Returns: [B] (with next/prev set to NULL)
 * --------------------------------------------------------------------------- */
list_node_t *list_pop_back(list_t *list)
{
    if (!list || !list->tail) {
        return NULL;
    }

    /* Save the node we're removing */
    list_node_t *node = list->tail;
    
    /* Update tail to the previous node */
    list->tail = node->prev;

    /* Update the new tail's next pointer, or clear head if list is now empty */
    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }

    /* Clean up the removed node's pointers */
    node->next = NULL;
    node->prev = NULL;
    list->size--;

    return node;
}

/* ---------------------------------------------------------------------------
 * list_remove - Remove a specific node from the list
 * ---------------------------------------------------------------------------
 * Removes the specified node from the list. The node must actually be in
 * this list - behavior is undefined if it's not.
 *
 * Note: This function cannot verify that the node is actually in this list.
 * The caller must ensure this is the case.
 * --------------------------------------------------------------------------- */
void list_remove(list_t *list, list_node_t *node)
{
    if (!list || !node) {
        return;
    }

    /* Update the previous node's next pointer, or head if this is the first node */
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        /* Node is the head */
        list->head = node->next;
    }

    /* Update the next node's prev pointer, or tail if this is the last node */
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        /* Node is the tail */
        list->tail = node->prev;
    }

    /* Clean up the removed node's pointers */
    node->next = NULL;
    node->prev = NULL;
    list->size--;
}
