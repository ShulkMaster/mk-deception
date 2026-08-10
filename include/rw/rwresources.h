#ifndef RW_RWRESOURCES_H
#define RW_RWRESOURCES_H

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RwResEntry RwResEntry;
typedef void (*RwResEntryDestroyNotify)(RwResEntry* entry);

struct RwResEntry {
    RwLLLink link;
    RwInt32 size;
    void* owner;
    RwResEntry** ownerRef;
    RwResEntryDestroyNotify destroyNotify;
};

RwBool RwResourcesFreeResEntry(RwResEntry* entry);
RwResEntry* RwResourcesAllocateResEntry(
    void* owner, RwResEntry** ownerRef, RwInt32 size,
    RwResEntryDestroyNotify destroyNotify);
RwBool RwResourcesSetArenaSize(RwUInt32 size);
RwBool RwResourcesEmptyArena(void);
void _rwResourcesPurge(void);

#endif
