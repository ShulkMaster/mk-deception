#include "runtime/hashtable.h"

#include "mw/mwMem.h"

#include <ctype.h>

int stricmp(const char* a, const char* b);
unsigned long strlen(const char* s);
char* strcpy(char* dst, const char* src);

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

/* TODO: [near miss] 99.71%; entry-as-slot with in-branch slot copy matches the CFG and copies;
 * only volatile regs differ (retail pool base r6, slot copy r3, next r4). */
void hashtable_store_with_instance(Hashtable* ht, const char* key, void* value, int instance) {
    int ch;
    unsigned int hash;
    unsigned int high;
    const char* p;
    unsigned int bucket;
    HashtableEntry* entry;
    HashtableEntry* allocation_slot;
    HashtableEntry* recycled;
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
        recycled = entry->next;
        if (recycled != 0) {
            allocation_slot = entry;
            entry = recycled;
            allocation_slot->next = recycled->next;
            recycled->next = 0;
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

/* TODO: [near miss] 98.08%; chained loop store shares retail's single zero; residue is the zero/offset
 * register swap at loop setup and retail's single r3 = 1 shared by the initialized store and return. */
int hashtable_dynamic_init(Hashtable* ht, unsigned int bucket_count, _mwMemHeap* heap) {
    unsigned int i;

    ht->owns_keys = 1;
    ht->key_storage_capacity = bucket_count << 6;
    ht->heap = heap;
    ht->key_storage = _mwMemMalloc(heap, ht->key_storage_capacity, 3, 0, 0, 0);
    ht->capacity = bucket_count;
    ht->bucket_count = bucket_count;
    ht->buckets = _mwMemMalloc(ht->heap, bucket_count << 2, 3, 0, 0, 0);
    ht->entry_pool = _mwMemMalloc(ht->heap, ht->capacity << 4, 3, 0, 0, 0);
    if (ht->buckets == 0 || ht->entry_pool == 0 || ht->key_storage == 0) {
        return 0;
    }
    for (i = 0; i < bucket_count; i++) {
        ht->entry_pool[i].next = ht->buckets[i] = 0;
    }
    ht->key_storage_used = 0;
    ht->allocation_index = 0;
    ht->initialized = 1;
    return 1;
}
