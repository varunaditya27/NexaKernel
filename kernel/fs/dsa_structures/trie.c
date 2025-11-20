/*
 * trie.c
 *
 * Trie for filename lookup in RAM FS. Keep the trie small and memory efficient —
 * this is for demonstration, not production.
 */

#include <stdint.h>

void trie_init(void) {}
void trie_insert(const char *s, void *value) {}
void *trie_find(const char *s) { return 0; }
