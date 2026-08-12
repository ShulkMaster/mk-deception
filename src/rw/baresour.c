#include "libmkparticle/rw_engine.h"
#include "rw/rwcore_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwresources.h"

typedef struct RwResHeap RwResHeap;

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

RwModuleInfo resourcesModule;

extern int _rwResHeapInit(RwResHeap* heap, unsigned int size);
extern int _rwResHeapClose(RwResHeap* heap);
extern void _rwResHeapFree(void* memory);
extern void* _rwResHeapAlloc(RwResHeap* heap, unsigned int size);




static RwResourcesGlobals* ResourcesInit(RwResourcesGlobals* globals,
                                         unsigned int size) {
    if (size != 0) {
        globals->arena = RwEngineInstance->fpMalloc(size, 0x4040B);
        if (globals->arena == 0) {
            RwError error;

            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000013, size);
            RwErrorSet(&error);
            return 0;
        }
        if (!_rwResHeapInit(globals->arena, size)) {
            RwError error;

            RwEngineInstance->fpFree(globals->arena);
            error.pluginID = 1;
            error.errorCode = _rwerror(0xC, 0);
            RwErrorSet(&error);
            return 0;
        }
    } else {
        globals->arena = 0;
    }
    globals->entriesA.link.next = &globals->entriesA.link;
    globals->entriesA.link.prev = &globals->entriesA.link;
    globals->entriesB.link.next = &globals->entriesB.link;
    globals->entriesB.link.prev = &globals->entriesB.link;
    globals->allocList = &globals->entriesA.link;
    globals->activeList = &globals->entriesB.link;
    globals->arenaSize = size;
    globals->arenaUsage = 0;
    globals->arenaReusage = 0;
    return globals;
}

void* _rwResourcesOpen(void* object, int offset, int size) {
    unsigned int arenaSize;

    resourcesModule.globalsOffset = offset;
    arenaSize = RwEngineInstance->resArenaInitSize;
    if (ResourcesInit(&(*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)), arenaSize) == 0) {
        return 0;
    }
    resourcesModule.numInstances++;
    return object;
}

void* _rwResourcesClose(void* object, int offset, int size) {
    RwResourcesEmptyArena();
    _rwResHeapClose((*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arena);
    RwEngineInstance->fpFree((*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arena);
    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arena = 0;
    resourcesModule.numInstances--;
    return object;
}

int RwResourcesFreeResEntry(RwResEntry* entry) {
    if (entry->destroyNotify != 0) {
        entry->destroyNotify(entry);
    }
    if (entry->ownerRef != 0) {
        *entry->ownerRef = 0;
    }
    if (entry->link.next != 0) {
        entry->link.prev->next = entry->link.next;
        {
            RwLLLink* previous = entry->link.prev;
            entry->link.next->prev = previous;
        }
        (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arenaUsage -= entry->size;
        _rwResHeapFree(entry);
    } else {
        RwEngineInstance->fpFree(entry);
    }
    return 1;
}




void _rwResourcesPurge(void) {
    RwLLLink* active = (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).activeList;
    RwLLLink* alloc = (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList;

    if (active->next != active) {
        if (alloc->next == alloc) {
            alloc->next = active->next;
            alloc->prev = active->prev;
            active->next->prev = alloc;
            active->prev->next = alloc;
        } else {
            RwLLLink* allocLast = alloc->prev;
            RwLLLink* activeFirst = active->next;
            RwLLLink* activeLast = active->prev;

            allocLast->next = activeFirst;
            activeFirst->prev = allocLast;
            activeLast->next = alloc;
            alloc->prev = activeLast;
        }
        active->next = active;
        active->prev = active;
    }
    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList = active;
    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).activeList = alloc;
    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arenaReusage = 0;
}

RwResEntry* RwResourcesAllocateResEntry(
    void* owner, RwResEntry** ownerRef, int size,
    RwResEntryDestroyNotify destroyNotify) {
    RwResEntry* entry;
    int exhausted = 0;

    while (exhausted == 0) {
        entry = _rwResHeapAlloc((*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arena,
                                size + sizeof(RwResEntry));
        if (entry != 0) {
            entry->link.next = (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->next;
            entry->link.prev = (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList;
            (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->next->prev = &entry->link;
            (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->next = &entry->link;
            entry->owner = owner;
            entry->size = size;
            entry->ownerRef = ownerRef;
            entry->destroyNotify = destroyNotify;
            (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arenaUsage += size;
            if (ownerRef != 0) {
                *ownerRef = entry;
            }
            return entry;
        }
        {
            RwLLLink* tail = (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).activeList->prev;

            if (tail != (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).activeList) {
                entry = (RwResEntry*)tail;
                RwResourcesFreeResEntry(entry);
            } else {
                tail = (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->prev;
                if (tail != (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).allocList) {
                    entry = (RwResEntry*)tail;
                    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arenaReusage += entry->size;
                    RwResourcesFreeResEntry(entry);
                } else {
                    exhausted = 1;
                }
            }
        }
    }
    if (ownerRef != 0) {
        *ownerRef = 0;
    }
    {
        RwError error;

        error.pluginID = 1;
        error.errorCode = _rwerror(0xC, size);
        RwErrorSet(&error);
    }
    return 0;
}

int RwResourcesSetArenaSize(unsigned int size) {
    RwResourcesGlobals* globals;

    if (resourcesModule.numInstances == 0) {
        RwEngineInstance->resArenaInitSize = size;
        return 1;
    }
    globals = &(*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset));
    globals->arenaSize = size;
    RwResourcesEmptyArena();
    _rwResHeapClose(globals->arena);
    RwEngineInstance->fpFree(globals->arena);
    globals->arena = RwEngineInstance->fpMalloc(size, 0x3040B);
    if (globals->arena == 0) {
        RwError error;

        globals->arenaSize = 0;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    if (!_rwResHeapInit(globals->arena, size)) {
        RwError error;

        RwEngineInstance->fpFree(globals->arena);
        error.pluginID = 1;
        error.errorCode = _rwerror(0xC, 0);
        RwErrorSet(&error);
        return 0;
    }
    return 1;
}



int RwResourcesEmptyArena(void) {
    RwLLLink* link;
    RwLLLink* end;

    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.prev->next =
        (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link.next;
    link = (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.next;
    end = &(*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link;
    while (link != end) {
        RwResEntry* entry = (RwResEntry*)link;

        link = link->next;
        RwResourcesFreeResEntry(entry);
    }
    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.next = &(*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link;
    {
        RwLLLink* sentinelA = &(*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link;
        (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.prev = sentinelA;
    }
    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link.next = &(*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link;
    {
        RwLLLink* sentinelB = &(*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link;
        (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link.prev = sentinelB;
    }
    (*(RwResourcesGlobals*)((unsigned char*)RwEngineInstance + resourcesModule.globalsOffset)).arenaReusage = 0;
    return 1;
}
