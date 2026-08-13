#ifndef RW_RWRESOURCES_H
#define RW_RWRESOURCES_H

#include "rw/rwcore_types.h"
#include "rw/gamecube.h"
#include "rw/rwplcore.h"
#include "rw/rwresentry.h"

typedef struct RwGameCubeResEntryHeader {
    RwResEntry entry;
    union {
        struct {
            unsigned short token;
            unsigned short meshSerialNum;
        } sync;
        RwGameCubeVertexBuffer vertexBuffer;
    } data;
} RwGameCubeResEntryHeader;

typedef struct RwResourcesGlobalsPrefix {
    unsigned int arenaSize;
    unsigned int arenaUsage;
    unsigned int arenaReusage;
    void* arena;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLLLink* activeList;
} RwResourcesGlobalsPrefix;

extern RwModuleInfo resourcesModule;

int RwResourcesFreeResEntry(RwResEntry* entry);
RwResEntry* RwResourcesAllocateResEntry(
    void* owner, RwResEntry** ownerRef, int size,
    RwResEntryDestroyNotify destroyNotify);
int RwResourcesSetArenaSize(unsigned int size);
int RwResourcesEmptyArena(void);
void _rwResourcesPurge(void);
void* _rwResourcesOpen(void* instance, int offset, int size);
void* _rwResourcesClose(void* instance, int offset, int size);

#endif
