#ifndef HASHTABLE_H
#define HASHTABLE_H

struct _mwMemHeap;

typedef struct HashtableEntry {
    union {
        const char* key;
        char* writable_key;
    } key_ptr;                       /* +0x00 */
    void* value;                     /* +0x04 */
    int instance;                    /* +0x08: caller-owned generation/tag */
    struct HashtableEntry* next;     /* +0x0C: bucket chain or recycled entry */
} HashtableEntry;                    /* 0x10 */

typedef struct Hashtable {
    int initialized;                 /* +0x00 */
    HashtableEntry** buckets;        /* +0x04 */
    int bucket_count;                /* +0x08 */
    HashtableEntry* entry_pool;      /* +0x0C */
    int allocation_index;            /* +0x10 */
    int capacity;                    /* +0x14 */
    struct _mwMemHeap* heap;         /* +0x18 */
    int owns_keys;                   /* +0x1C */
    char* key_storage;               /* +0x20 */
    int key_storage_capacity;        /* +0x24 */
    int key_storage_used;            /* +0x28 */
} Hashtable;                         /* 0x2C */

typedef void (*HashtableForeachFn)(void* value);

void hashtable_foreach(Hashtable* ht, HashtableForeachFn fn);
void hashtable_destroy(Hashtable* ht);
void* hashtable_get(Hashtable* ht, const char* key);
HashtableEntry* hashtable_get_bucket(Hashtable* ht, const char* key);
void hashtable_store(Hashtable* ht, const char* key, void* value);
void hashtable_store_with_instance(Hashtable* ht, const char* key, void* value, int instance);
int hashtable_dynamic_init(Hashtable* ht, unsigned int bucket_count,
                           struct _mwMemHeap* heap);

#endif
