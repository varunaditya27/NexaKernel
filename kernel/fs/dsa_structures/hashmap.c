/*
 * hashmap.c
 *
 * A small hashmap for the RAM FS open-file table and other kernel maps.
 * Implement chaining or open addressing as needed for simple lookups.
 */

#include <stdint.h>

void hashmap_init(void) {}
void *hashmap_get(const char *key) { return 0; }
void hashmap_set(const char *key, void *val) {}
