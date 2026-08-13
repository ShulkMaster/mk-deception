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

#endif
