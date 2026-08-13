#ifndef RW_RWRESOURCES_H
#define RW_RWRESOURCES_H

#include "rw/rwcore_types.h"
#include "rw/gamecube.h"
#include "rw/rwplcore.h"
#include "rw/rwresentry.h"

typedef struct RwResHeap RwResHeap;
typedef struct RwResHeapBlock RwResHeapBlock;

struct RwResHeap {
    RwResHeapBlock* firstBlock;
    RwResHeapBlock* firstFreeBlock;
};

struct RwResHeapBlock {
    RwResHeap* heap;
    RwResHeapBlock* next;
    RwResHeapBlock* prev;
    unsigned int size;
    unsigned int flags;
    unsigned int reserved[3];
};

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

typedef struct RwResourcesGlobals {
    unsigned int arenaSize;
    unsigned int arenaUsage;
    unsigned int arenaReusage;
    RwResHeap* arena;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLLLink* activeList;
    RwLLLink* allocList;
} RwResourcesGlobals;

extern RwModuleInfo resourcesModule;

int _rwResHeapInit(RwResHeap* heap, unsigned int size);
int _rwResHeapClose(RwResHeap* heap);
void _rwResHeapFree(void* memory);
void* _rwResHeapAlloc(RwResHeap* heap, unsigned int size);
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
