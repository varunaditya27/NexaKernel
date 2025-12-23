#ifndef NEXA_HASHMAP_H
#define NEXA_HASHMAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * lib/dsa/hashmap.h
 *
 * Generic Hash Map Interface
 *
 * This header defines a generic hash map (dictionary) that maps string keys to
 * void pointers. It uses chaining (linked lists) to resolve hash collisions.
 */

typedef struct hashmap_entry {
    char *key;
    void *value;
    struct hashmap_entry *next;
} hashmap_entry_t;

typedef struct hashmap {
    hashmap_entry_t **buckets;
    size_t bucket_count;
    size_t size;
} hashmap_t;

/* Initialize a hash map */
bool hashmap_init(hashmap_t *map, size_t bucket_count);

/* Destroy the hash map */
void hashmap_destroy(hashmap_t *map);

/* Insert a key-value pair */
bool hashmap_put(hashmap_t *map, const char *key, void *value);

/* Get a value by key */
void *hashmap_get(hashmap_t *map, const char *key);

/* Remove a key */
void hashmap_remove(hashmap_t *map, const char *key);

/* Simple string hash function */
uint32_t hashmap_hash_string(const char *str);

#endif /* NEXA_HASHMAP_H */
