#include "runtime/hashtable.h"

#include "mw/mwMem.h"

extern unsigned char __ctype_map[];
extern unsigned char __lower_map[];

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

/* Soft ceiling: hashtable_get_bucket ~98.25% -- hash-loop volatile coloring
 * and bucket-count register swap; algorithm and size match retail. */
HashtableEntry* hashtable_get_bucket(Hashtable* ht, const char* key) {
    unsigned char raw;
    unsigned int hash;
    unsigned int high;
    unsigned int idx;
    int ch;
    const unsigned char* p;
    unsigned char* ctype_map;
    unsigned char* lower_map;
    unsigned int bucket;
    HashtableEntry* entry;
    int cmp;

    if (key == 0) {
        return 0;
    }

    ctype_map = __ctype_map;
    lower_map = __lower_map;
    p = (const unsigned char*)key;
    hash = 0;
    while ((signed char)(raw = *p) != 0) {
        ch = (signed char)raw;
        idx = (unsigned char)ch;
        if ((ctype_map[idx] & 0x80) != 0) {
            if (ch == -1) {
                ch = -1;
            } else {
                ch = lower_map[idx];
            }
        }
        hash = (hash << 4) + (unsigned int)ch;
        high = hash & 0xF0000000;
        if (high != 0) {
            hash ^= (int)high >> 24;
            hash ^= high;
        }
        p++;
    }

    bucket = hash - (hash / ht->bucket_count) * ht->bucket_count;
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

/* Soft ceiling: hashtable_store_with_instance ~94.38% -- shared hash-loop
 * coloring plus recycled-entry and key-storage scheduling; size matches retail. */
void hashtable_store_with_instance(Hashtable* ht, const char* key, void* value, int instance) {
    unsigned char raw;
    unsigned int hash;
    unsigned int high;
    unsigned int idx;
    int ch;
    const unsigned char* p;
    unsigned char* ctype_map;
    unsigned char* lower_map;
    unsigned int bucket;
    HashtableEntry* entry;
    HashtableEntry* allocation_slot;
    HashtableEntry* recycled;
    int cmp;
    int len;
    int allocation_index;

    if (key == 0) {
        return;
    }

    ctype_map = __ctype_map;
    lower_map = __lower_map;
    p = (const unsigned char*)key;
    hash = 0;
    while ((signed char)(raw = *p) != 0) {
        ch = (signed char)raw;
        idx = (unsigned char)ch;
        if ((ctype_map[idx] & 0x80) != 0) {
            if (ch == -1) {
                ch = -1;
            } else {
                ch = lower_map[idx];
            }
        }
        hash = (hash << 4) + (unsigned int)ch;
        high = hash & 0xF0000000;
        if (high != 0) {
            hash ^= (int)high >> 24;
            hash ^= high;
        }
        p++;
    }

    bucket = hash - (hash / ht->bucket_count) * ht->bucket_count;
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
        allocation_index = ht->allocation_index;
        allocation_slot = &ht->entry_pool[allocation_index];
        recycled = allocation_slot->next;
        if (recycled != 0) {
            allocation_slot->next = recycled->next;
            recycled->next = 0;
            entry = recycled;
        } else {
            ht->allocation_index = allocation_index + 1;
            entry = allocation_slot;
        }
        if (ht->owns_keys != 0) {
            len = strlen(key);
            entry->key_ptr.writable_key = ht->key_storage + ht->key_storage_used;
            strcpy(entry->key_ptr.writable_key, key);
            ht->key_storage_used = len + ht->key_storage_used + 1;
        } else {
            entry->key_ptr.key = key;
        }
        entry->value = value;
        entry->next = ht->buckets[bucket];
        ht->buckets[bucket] = entry;
    }
    entry->instance = instance;
}

/* Soft ceiling: hashtable_dynamic_init ~96.58% -- zero-register allocation
 * and unsigned loop-exit branch scheduling; stop. */
int hashtable_dynamic_init(Hashtable* ht, unsigned int bucket_count, _mwMemHeap* heap) {
    Hashtable* ht_local;
    unsigned int count;
    int i;

    ht_local = ht;
    count = bucket_count;
    ht_local->owns_keys = 1;
    ht_local->key_storage_capacity = count << 6;
    ht_local->heap = heap;
    ht_local->key_storage = _mwMemMalloc(heap, ht_local->key_storage_capacity, 3, 0, 0, 0);
    ht_local->capacity = count;
    ht_local->bucket_count = count;
    ht_local->buckets = _mwMemMalloc(ht_local->heap, count << 2, 3, 0, 0, 0);
    ht_local->entry_pool = _mwMemMalloc(ht_local->heap, ht_local->capacity << 4, 3, 0, 0, 0);
    if (ht_local->buckets == 0 || ht_local->entry_pool == 0 || ht_local->key_storage == 0) {
        return 0;
    }
    i = 0;
    while (count > 0) {
        ht_local->buckets[i] = 0;
        ht_local->entry_pool[i].next = 0;
        i++;
        count--;
    }
    ht_local->key_storage_used = 0;
    ht_local->allocation_index = 0;
    ht_local->initialized = 1;
    return 1;
}
