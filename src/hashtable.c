#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hashtable.h"

struct Entry {

    char *key;
    int value;
    struct Entry *next;

};

struct Hash {

    struct Entry **buckets;
    size_t capacity;
    size_t count;

};

/*
haskKey()

Hashes the key and returns to user.

Parameters
----------
    const char *key:
        Key entered in the (key, value) pair

Return
------
    unsigned long:
        Converted key to an unsigned long

*/
static unsigned long hashKey(const char *key) {

    if (key == NULL) {
        return 0;
    }

    unsigned long hash = 5381;

    while (*key != '\0') {
        hash = (hash * 33) + (unsigned char)*key;
        key++;
    }

    return hash;

}

/*
ht_resize()

Resizes the current hash table. Entries are moved, not copied. No keys or entries are allocated or feeed. Called internally by ht_insert() when the load factor would exceed 0.75.

Parameters
----------
    Hash *h:
        Hash table to be resized

    size_t new_capacity:
        Number of buckets in the resized table. Must be greater than 0.

Return
------
    bool:
        True if the table was resized
        False if the new bucket array could not be allocated; the table is left unchanged and still valid.
*/
static bool ht_resize(Hash *h, size_t new_capacity) {

    if (h == NULL || new_capacity < 1) {
        return false;
    }

    struct Entry **new_buckets = calloc(new_capacity, sizeof(struct Entry*));
    if (new_buckets == NULL) {
        return false;
    }

    for (size_t i = 0; i < h->capacity; i++) {
        struct Entry *current = h->buckets[i];

        while (current != NULL) {
            struct Entry *next = current->next;

            size_t new_index = hashKey(current->key) % new_capacity;
            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;

            current = next;
        }
    }

    free(h->buckets);
    h->buckets = new_buckets;
    h->capacity = new_capacity; 
    return true;

}

//
//
//
//
// Functions Below
//
//
//
//

/*
ht_create()

Initialize a hash table.

Parameters
----------
    size_t capacity: 
        Size of the hash table; Must be a multiple of 2.

Return
------
    Hash*: 
        Return pointer to the hash table object.
*/
Hash* ht_create(size_t capacity) {

    if (capacity == 0) {
        printf("HashTable error: Expected hash table size greater than 0\n");
        return NULL;
    }

    Hash *hash = malloc(sizeof(Hash));
    if (hash == NULL) {
        return NULL;
    }

    hash->buckets = calloc(capacity, sizeof(struct Entry*));
    if (hash->buckets == NULL) {
        free(hash);
        return NULL;
    }

    hash->capacity = capacity;
    hash->count = 0;
    return hash;

}

/*
ht_destroy()

Removes the hash table from memory.

Parameters
----------
    Hash *h: 
        Hash table entity to be destroyed

Return
------
    void
*/
void ht_destroy(Hash *h) {

    if (h == NULL) {
        return;
    }

    for (size_t i = 0; i < h->capacity; i++) {

        struct Entry *current = h->buckets[i];

        while (current != NULL) {
            struct Entry *next = current->next;
            free(current->key);
            free(current);
            current = next;
        }

    }

    free(h->buckets);
    free(h);

}

/*
ht_insert()

Inserts a (key, value) pair into the hash table.

Parameters
----------
    Hash *h:
        Hash table entity

    const char *key:
        Key to be entered

    int value:
        Value to be associated with that key

Return
------
    bool:
        True if successfully inserted.
        False if not inserted.
*/
bool ht_insert(Hash *h, const char *key, int value) {

    if (h == NULL || key == NULL) {
        return false;
    }

    // iterate through loop
    size_t index = hashKey(key) % h->capacity;
    struct Entry *current = h->buckets[index];

    while (current != NULL) {

        if (strcmp(current->key, key) == 0) {

            current->value = value;
            return true;

        }
        current = current->next;

    }

    // check if resize needed
    if ((h->count + 1) * 4 > h->capacity * 3) {

        ht_resize(h, h->capacity*2);
        index = hashKey(key) % h->capacity;

    }

    struct Entry *entry = malloc(sizeof(struct Entry));
    if (entry == NULL) {
        return false;
    }

    entry->key = malloc(strlen(key) + 1);
    if (entry->key == NULL) {
        free(entry);
        return false;
    }

    strcpy(entry->key, key);
    entry->value = value;

    entry->next = h->buckets[index];
    h->buckets[index] = entry;

    h->count++;
    return true;

}

/*
ht_get()

Retrieves a value from a key in the hash table. Return value indicates whether the object exists in the table or not.

Parameters
----------
    const Hash *h:
        Hash table to me looked at

    const char *key:
        Key of the value to get

    int *value:
        Pointer to the value retrieved

Return
------
    bool:
        True if object was found
        False if object was not found
*/
bool ht_get(const Hash *h, const char *key, int *value) {

    if (h == NULL || key == NULL || value == NULL) {
        return false;
    }

    size_t index = hashKey(key) % h->capacity;

    struct Entry *current = h->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            *value = current->value;
            return true;
        }
        current = current->next;
    }

    return false;

}

/*
ht_delete()

Deletes an existing hash table entry. Do nothing if it doesn't exist.

Parameters
----------
    Hash *h:
        Hash table to find the deleted element in.

    const char *key:
        Key of the pair to be destroyed.

Return
------
    bool:
        True if the key, value pair was in the table and deleted.
        False if the key did not exist in the hash table.
*/
bool ht_delete(Hash *h, const char *key) {

    if (h == NULL || key == NULL) {
        return false;
    }

    size_t index = hashKey(key) % h->capacity;

    struct Entry *prev = NULL;
    struct Entry *current = h->buckets[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {

            if (prev == NULL) {
                h->buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }

            free(current->key);
            free(current);

            h->count--;
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;

}

/*
ht_size()

Parameters
----------
    const Hash *h:
        Hash table to determine the size of

Return
------
    size_t:
        Returns the size of the table, via the number of elements, as size_t.
*/
size_t ht_size(const Hash *h) {

    if (h == NULL) {
        return 0;
    }

    return h->count;

}