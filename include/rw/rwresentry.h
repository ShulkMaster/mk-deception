#ifndef RW_RWRESENTRY_H
#define RW_RWRESENTRY_H

#include "rw/rwcore_types.h"

typedef struct RwResEntry RwResEntry;
typedef void (*RwResEntryDestroyNotify)(RwResEntry* entry);

struct RwResEntry {
    RwLLLink link;
    int size;
    void* owner;
    RwResEntry** ownerRef;
    RwResEntryDestroyNotify destroyNotify;
};
typedef char RwResEntrySizeCheck[sizeof(RwResEntry) == 0x18 ? 1 : -1];

#endif
