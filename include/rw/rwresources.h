#ifndef RW_RWRESOURCES_H
#define RW_RWRESOURCES_H

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RwResEntry RwResEntry;
typedef void (*RwResEntryDestroyNotify)(RwResEntry* entry);

struct RwResEntry {
    RwLLLink link;
    int size;
    void* owner;
    RwResEntry** ownerRef;
    RwResEntryDestroyNotify destroyNotify;
};

int RwResourcesFreeResEntry(RwResEntry* entry);
RwResEntry* RwResourcesAllocateResEntry(
    void* owner, RwResEntry** ownerRef, int size,
    RwResEntryDestroyNotify destroyNotify);
int RwResourcesSetArenaSize(unsigned int size);
int RwResourcesEmptyArena(void);
void _rwResourcesPurge(void);

#endif
