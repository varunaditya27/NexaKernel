#ifndef NEXA_TRIE_H
#define NEXA_TRIE_H

#include "../../config/os_config.h"


/*
 * lib/dsa/trie.h
 *
 * Generic Trie (Prefix Tree) Interface
 *
 * This header defines a generic Trie data structure, optimized for string keys.
 * It is useful for tasks like file indexing, autocomplete, or symbol tables.
 */

#define TRIE_ALPHABET_SIZE 256

typedef struct trie_node {
    struct trie_node *children[TRIE_ALPHABET_SIZE];
    void *data; // NULL if not a terminal node (or if no data associated)
    bool is_terminal;
} trie_node_t;

typedef struct trie {
    trie_node_t *root;
} trie_t;

/* Initialize a trie */
void trie_init(trie_t *trie);

/* Insert a key-value pair */
bool trie_insert(trie_t *trie, const char *key, void *data);

/* Search for a key */
void *trie_search(trie_t *trie, const char *key);

/* Remove a key */
bool trie_remove(trie_t *trie, const char *key);

#endif /* NEXA_TRIE_H */
