/*
 * lib/dsa/list.c
 *
 * Linked List Implementation
 *
 * This file implements the operations for the generic doubly linked list,
 * including initialization, insertion (front/back), removal, and size tracking.
 */

#include "list.h"

void list_init(list_t *list) {
    if (!list) return;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void list_node_init(list_node_t *node) {
    if (!node) return;
    node->next = NULL;
    node->prev = NULL;
}

bool list_is_empty(list_t *list) {
    return list == NULL || list->size == 0;
}

void list_push_front(list_t *list, list_node_t *node) {
    if (!list || !node) return;

    node->next = list->head;
    node->prev = NULL;

    if (list->head) {
        list->head->prev = node;
    } else {
        list->tail = node;
    }

    list->head = node;
    list->size++;
}

void list_push_back(list_t *list, list_node_t *node) {
    if (!list || !node) return;

    node->next = NULL;
    node->prev = list->tail;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }

    list->tail = node;
    list->size++;
}

list_node_t *list_pop_front(list_t *list) {
    if (!list || !list->head) return NULL;

    list_node_t *node = list->head;
    list->head = node->next;

    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    node->next = NULL;
    node->prev = NULL;
    list->size--;

    return node;
}

list_node_t *list_pop_back(list_t *list) {
    if (!list || !list->tail) return NULL;

    list_node_t *node = list->tail;
    list->tail = node->prev;

    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }

    node->next = NULL;
    node->prev = NULL;
    list->size--;

    return node;
}

void list_remove(list_t *list, list_node_t *node) {
    if (!list || !node) return;

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    node->next = NULL;
    node->prev = NULL;
    list->size--;
}

size_t list_size(list_t *list) {
    return list ? list->size : 0;
}
