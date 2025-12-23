#include <lib/dsa/list.h>
#include <lib/dsa/queue.h>
#include <lib/dsa/heap.h>
#include <lib/dsa/bitmap.h>
#include <lib/dsa/tree.h>
#include <lib/dsa/trie.h>
#include <lib/dsa/hashmap.h>
#include <stddef.h>

/*
 * kernel/utils/test_dsa.c
 *
 * Data Structure Verification Suite
 *
 * This file contains unit tests for the kernel's Data Structures and Algorithms (DSA)
 * library. It verifies the correctness of Lists, Queues, Heaps, Trees, etc.,
 * ensuring they function as expected before being used by critical kernel subsystems.
 */

extern int printf(const char *format, ...); // Placeholder

void test_list(void) {
    list_t list;
    list_init(&list);
    
    struct my_node {
        int value;
        list_node_t node;
    } n1, n2, n3;
    
    n1.value = 1; list_node_init(&n1.node);
    n2.value = 2; list_node_init(&n2.node);
    n3.value = 3; list_node_init(&n3.node);
    
    list_push_back(&list, &n1.node);
    list_push_back(&list, &n2.node);
    list_push_front(&list, &n3.node);
    
    // Expected: 3 -> 1 -> 2
    if (list_size(&list) != 3) printf("List size error\n");
    
    list_node_t *head = list.head;
    if (list_entry(head, struct my_node, node)->value != 3) printf("List head error\n");
    
    list_pop_back(&list);
    if (list_size(&list) != 2) printf("List pop error\n");
}

void test_queue(void) {
    queue_t q;
    if (!queue_init(&q, 5)) return;
    
    int a = 10, b = 20;
    queue_enqueue(&q, &a);
    queue_enqueue(&q, &b);
    
    if (*(int*)queue_dequeue(&q) != 10) printf("Queue dequeue error\n");
    if (*(int*)queue_dequeue(&q) != 20) printf("Queue dequeue error\n");
    
    queue_destroy(&q);
}

int int_cmp(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

void test_heap(void) {
    heap_t h;
    heap_init(&h, 10, int_cmp);
    
    int a = 30, b = 10, c = 20;
    heap_insert(&h, &a);
    heap_insert(&h, &b);
    heap_insert(&h, &c);
    
    // Min heap: 10, 20, 30
    if (*(int*)heap_extract(&h) != 10) printf("Heap extract error 1\n");
    if (*(int*)heap_extract(&h) != 20) printf("Heap extract error 2\n");
    if (*(int*)heap_extract(&h) != 30) printf("Heap extract error 3\n");
    
    heap_destroy(&h);
}

void test_dsa_all(void) {
    test_list();
    test_queue();
    test_heap();
    // Add others...
    printf("DSA Tests Completed.\n");
}
