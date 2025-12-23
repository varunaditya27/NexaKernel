/*
 * lib/dsa/trie.c
 *
 * Trie Implementation
 *
 * This file implements Trie operations: insertion, search, and removal.
 * It manages a tree of nodes where each edge represents a character in the key string.
 */

#include "trie.h"

extern void *kmalloc(size_t size);
extern void kfree(void *ptr);
void *memset(void *s, int c, size_t n); // Assuming we have this or need to include it

static trie_node_t *create_node(void) {
    trie_node_t *node = (trie_node_t *)kmalloc(sizeof(trie_node_t));
    if (node) {
        node->data = NULL;
        node->is_terminal = false;
        memset(node->children, 0, sizeof(node->children));
    }
    return node;
}

void trie_init(trie_t *trie) {
    if (!trie) return;
    trie->root = create_node();
}

bool trie_insert(trie_t *trie, const char *key, void *data) {
    if (!trie || !trie->root || !key) return false;

    trie_node_t *current = trie->root;
    while (*key) {
        unsigned char index = (unsigned char)*key;
        if (!current->children[index]) {
            current->children[index] = create_node();
            if (!current->children[index]) return false; // Allocation failed
        }
        current = current->children[index];
        key++;
    }
    current->data = data;
    current->is_terminal = true;
    return true;
}

void *trie_search(trie_t *trie, const char *key) {
    if (!trie || !trie->root || !key) return NULL;

    trie_node_t *current = trie->root;
    while (*key) {
        unsigned char index = (unsigned char)*key;
        if (!current->children[index]) {
            return NULL;
        }
        current = current->children[index];
        key++;
    }

    return current->is_terminal ? current->data : NULL;
}

static bool has_children(trie_node_t *node) {
    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        if (node->children[i]) return true;
    }
    return false;
}

static bool remove_recursive(trie_node_t *node, const char *key) {
    if (*key == '\0') {
        if (node->is_terminal) {
            node->is_terminal = false;
            node->data = NULL;
            return !has_children(node); // Return true if node should be deleted
        }
        return false;
    }

    unsigned char index = (unsigned char)*key;
    if (!node->children[index]) return false;

    if (remove_recursive(node->children[index], key + 1)) {
        kfree(node->children[index]);
        node->children[index] = NULL;
        return !node->is_terminal && !has_children(node);
    }

    return false;
}

bool trie_remove(trie_t *trie, const char *key) {
    if (!trie || !trie->root || !key) return false;
    return remove_recursive(trie->root, key); // Note: Root is never deleted
}
