/*
 * lib/dsa/hashmap.c
 *
 * Hash Map Implementation
 *
 * This file implements the hash map operations: initialization, insertion (put),
 * retrieval (get), removal, and destruction. It uses the DJB2 hash algorithm
 * for string hashing.
 */

#include "hashmap.h"

extern void *kmalloc(size_t size);
extern void kfree(void *ptr);
extern size_t strlen(const char *s);
extern char *strcpy(char *dest, const char *src);
extern int strcmp(const char *s1, const char *s2);
void *memset(void *s, int c, size_t n);

// Simple DJB2 hash function
uint32_t hashmap_hash_string(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

static char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *new_s = (char *)kmalloc(len);
    if (new_s) {
        strcpy(new_s, s);
    }
    return new_s;
}

bool hashmap_init(hashmap_t *map, size_t bucket_count) {
    if (!map || bucket_count == 0) return false;

    map->buckets = (hashmap_entry_t **)kmalloc(bucket_count * sizeof(hashmap_entry_t *));
    if (!map->buckets) return false;

    memset(map->buckets, 0, bucket_count * sizeof(hashmap_entry_t *));
    map->bucket_count = bucket_count;
    map->size = 0;

    return true;
}

void hashmap_destroy(hashmap_t *map) {
    if (!map) return;

    for (size_t i = 0; i < map->bucket_count; i++) {
        hashmap_entry_t *entry = map->buckets[i];
        while (entry) {
            hashmap_entry_t *next = entry->next;
            kfree(entry->key);
            kfree(entry);
            entry = next;
        }
    }
    kfree(map->buckets);
    map->buckets = NULL;
    map->size = 0;
}

bool hashmap_put(hashmap_t *map, const char *key, void *value) {
    if (!map || !key) return false;

    uint32_t hash = hashmap_hash_string(key);
    size_t index = hash % map->bucket_count;

    hashmap_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value; // Update existing
            return true;
        }
        entry = entry->next;
    }

    // Create new entry
    hashmap_entry_t *new_entry = (hashmap_entry_t *)kmalloc(sizeof(hashmap_entry_t));
    if (!new_entry) return false;

    new_entry->key = strdup(key);
    if (!new_entry->key) {
        kfree(new_entry);
        return false;
    }
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;

    return true;
}

void *hashmap_get(hashmap_t *map, const char *key) {
    if (!map || !key) return NULL;

    uint32_t hash = hashmap_hash_string(key);
    size_t index = hash % map->bucket_count;

    hashmap_entry_t *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}

void hashmap_remove(hashmap_t *map, const char *key) {
    if (!map || !key) return;

    uint32_t hash = hashmap_hash_string(key);
    size_t index = hash % map->bucket_count;

    hashmap_entry_t *entry = map->buckets[index];
    hashmap_entry_t *prev = NULL;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            kfree(entry->key);
            kfree(entry);
            map->size--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}
