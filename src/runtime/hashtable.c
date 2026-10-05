#include "runtime/hashtable.h"

#include "mw/mwMem.h"

#include <ctype.h>
#include "runtime/cstring.h"

void hashtable_foreach(Hashtable* ht, HashtableForeachFn fn) {
    Hashtable* ht_local;
    unsigned int i;
    HashtableEntry* entry;

    ht_local = ht;
    for (i = 0; i < (unsigned int)ht_local->bucket_count; i++) {
        entry = ht_local->buckets[i];
        while (entry != 0) {
            fn(entry->value);
            entry = entry->next;
        }
    }
}

void hashtable_destroy(Hashtable* ht) {
    _mwMemFree(ht->entry_pool, 0, 0);
    _mwMemFree(ht->buckets, 0, 0);
    if (ht->owns_keys != 0) {
        _mwMemFree(ht->key_storage, 0, 0);
    }
    ht->entry_pool = 0;
    ht->buckets = 0;
    ht->key_storage = 0;
    ht->initialized = 0;
}

void* hashtable_get(Hashtable* ht, const char* key) {
    HashtableEntry* entry;

    entry = hashtable_get_bucket(ht, key);
    if (entry != 0) {
        return entry->value;
    }
    return 0;
}

HashtableEntry* hashtable_get_bucket(Hashtable* ht, const char* key) {
    unsigned int high;
    int ch;
    unsigned int hash;
    const char* p;
    unsigned int bucket;
    HashtableEntry* entry;
    int cmp;

    if (key == 0) {
        return 0;
    }

    hash = 0;
    for (p = key; *p != 0; p++) {
        ch = *p;
        if (isupper(ch)) {
            ch = _tolower(ch);
        }
        hash = (hash << 4) + ch;
        high = hash & 0xF0000000;
        if (high != 0) {
            hash ^= (int)high >> 24;
            hash ^= high;
        }
    }

    bucket = hash % ht->bucket_count;
    entry = ht->buckets[bucket];
    for (; entry != 0; entry = entry->next) {
        cmp = stricmp(key, entry->key_ptr.key);
        if (cmp == 0) {
            return entry;
        }
    }
    return 0;
}

void hashtable_store(Hashtable* ht, const char* key, void* value) {
    hashtable_store_with_instance(ht, key, value, 0);
}

void hashtable_store_with_instance(Hashtable* ht, const char* key, void* value, int instance) {
    int ch;
    unsigned int hash;
    unsigned int high;
    const char* p;
    unsigned int bucket;
    HashtableEntry* entry;
    int cmp;
    int len;

    if (key == 0) {
        return;
    }

    hash = 0;
    for (p = key; *p != 0; p++) {
        ch = *p;
        if (isupper(ch)) {
            ch = _tolower(ch);
        }
        hash = (hash << 4) + ch;
        high = hash & 0xF0000000;
        if (high != 0) {
            hash ^= (int)high >> 24;
            hash ^= high;
        }
    }

    bucket = hash % ht->bucket_count;
    entry = ht->buckets[bucket];
    while (entry != 0) {
        cmp = stricmp(key, entry->key_ptr.key);
        if (cmp == 0) {
            entry->value = value;
            break;
        }
        entry = entry->next;
    }
    if (entry == 0) {
        entry = &ht->entry_pool[ht->allocation_index];
        if (entry->next != 0) {
            entry = entry->next;
            ht->entry_pool[ht->allocation_index].next = entry->next;
            entry->next = 0;
        } else {
            ht->allocation_index++;
        }
        if (ht->owns_keys != 0) {
            len = strlen(key);
            entry->key_ptr.writable_key = ht->key_storage + ht->key_storage_used;
            strcpy(entry->key_ptr.writable_key, key);
            ht->key_storage_used += len + 1;
        } else {
            entry->key_ptr.key = key;
        }
        entry->value = value;
        entry->next = ht->buckets[bucket];
        ht->buckets[bucket] = entry;
    }
    entry->instance = instance;
}

static inline int hashtable_initialize_storage(Hashtable* ht,
                                                unsigned int bucket_count) {
    unsigned int i;

    for (i = 0; i < bucket_count; i++) {
        ht->entry_pool[i].next = ht->buckets[i] = 0;
    }
    ht->key_storage_used = 0;
    ht->allocation_index = 0;
    ht->initialized = 1;
    return 1;
}

/* TODO: [near miss] 98.29%; storage helper fixes loop setup; final zero
 * register and duplicate success constant remain after honest-form checks. */
int hashtable_dynamic_init(Hashtable* ht, unsigned int bucket_count, _mwMemHeap* heap) {
    ht->owns_keys = 1;
    ht->key_storage_capacity = bucket_count << 6;
    ht->heap = heap;
    ht->key_storage = _mwMemMalloc(heap, ht->key_storage_capacity, 3, 0, 0, 0);
    ht->capacity = bucket_count;
    ht->bucket_count = bucket_count;
    ht->buckets = _mwMemMalloc(ht->heap, bucket_count * sizeof(*ht->buckets), 3, 0, 0, 0);
    ht->entry_pool = _mwMemMalloc(ht->heap, ht->capacity * sizeof(*ht->entry_pool), 3, 0, 0, 0);
    if (ht->buckets == 0 || ht->entry_pool == 0 || ht->key_storage == 0) {
        return 0;
    }
    return hashtable_initialize_storage(ht, bucket_count);
}
