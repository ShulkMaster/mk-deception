#ifndef HASHTABLE_H
#define HASHTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

struct _mwMemHeap;

typedef struct HashtableEntry {
    union {
        const char* key;
        char* writable_key;
    } key_ptr;
    void* value;
    int instance;
    struct HashtableEntry* next;
} HashtableEntry;

typedef struct Hashtable {
    int initialized;
    HashtableEntry** buckets;
    int bucket_count;
    HashtableEntry* entry_pool;
    int allocation_index;
    int capacity;
    struct _mwMemHeap* heap;
    int owns_keys;
    char* key_storage;
    int key_storage_capacity;
    int key_storage_used;
} Hashtable;

typedef void (*HashtableForeachFn)(void* value);

void hashtable_foreach(Hashtable* ht, HashtableForeachFn fn);
void hashtable_destroy(Hashtable* ht);
void* hashtable_get(Hashtable* ht, const char* key);
HashtableEntry* hashtable_get_bucket(Hashtable* ht, const char* key);
void hashtable_store(Hashtable* ht, const char* key, void* value);
void hashtable_store_with_instance(Hashtable* ht, const char* key, void* value, int instance);
int hashtable_dynamic_init(Hashtable* ht, unsigned int bucket_count,
                           struct _mwMemHeap* heap);

#ifdef __cplusplus
}
#endif

#endif
