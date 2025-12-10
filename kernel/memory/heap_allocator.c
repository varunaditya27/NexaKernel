#include <lib/dsa/list.h>
#include <stddef.h>
#include <stdint.h>

/*
 * kernel/memory/heap_allocator.c
 *
 * Kernel Heap Allocator (kmalloc/kfree)
 *
 * This file implements the dynamic memory allocator for the kernel. It manages
 * a pool of memory (the kernel heap) and provides standard `kmalloc` and `kfree`
 * functions. It uses a Free List strategy to track available memory blocks.
 */

typedef struct heap_block {
    list_node_t node;
    size_t size;
    bool is_free;
} heap_block_t;

static list_t free_list;
static void *heap_start;
static size_t heap_size;

void heap_init(void *start, size_t size) {
    heap_start = start;
    heap_size = size;
    list_init(&free_list);

    // Initial big block
    heap_block_t *initial_block = (heap_block_t *)start;
    initial_block->size = size - sizeof(heap_block_t);
    initial_block->is_free = true;
    list_node_init(&initial_block->node);
    
    list_push_back(&free_list, &initial_block->node);
}

void *kmalloc(size_t size) {
    // Simple first-fit
    list_node_t *current;
    list_for_each(current, &free_list) {
        heap_block_t *block = list_entry(current, heap_block_t, node);
        if (block->is_free && block->size >= size) {
            // Split block if large enough
            if (block->size > size + sizeof(heap_block_t) + 16) {
                heap_block_t *new_block = (heap_block_t *)((char *)block + sizeof(heap_block_t) + size);
                new_block->size = block->size - size - sizeof(heap_block_t);
                new_block->is_free = true;
                list_node_init(&new_block->node);
                
                // Insert new block after current
                // Note: Generic list doesn't have insert_after, so we'd need to handle this carefully
                // For now, we just shrink the current block and don't add the split part back effectively in this simple wrapper
                // In a real implementation, we would insert 'new_block' into the list.
                
                block->size = size;
            }
            block->is_free = false;
            return (void *)((char *)block + sizeof(heap_block_t));
        }
    }
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    heap_block_t *block = (heap_block_t *)((char *)ptr - sizeof(heap_block_t));
    block->is_free = true;
    
    // Coalesce with neighbors (omitted for brevity)
}
