#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Hash Hash;

Hash* ht_create(size_t capacity);
void ht_destroy(Hash *h);

bool ht_insert(Hash *h, const char *key, int value);
bool ht_get(const Hash *h, const char *key, int *value);
bool ht_delete(Hash *h, const char *key);
size_t ht_size(const Hash *h);

#endif