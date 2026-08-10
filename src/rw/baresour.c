#include "libmkparticle/rw_engine.h"
#include "rw/rwcore_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwresources.h"

typedef struct RwResHeap RwResHeap;

typedef struct RwResourcesGlobals {
    RwUInt32 arenaSize;
    RwUInt32 arenaUsage;
    RwUInt32 arenaReusage;
    RwResHeap* arena;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLLLink* activeList;
    RwLLLink* allocList;
} RwResourcesGlobals;

RwModuleInfo resourcesModule;

extern RwBool _rwResHeapInit(RwResHeap* heap, RwUInt32 size);
extern RwBool _rwResHeapClose(RwResHeap* heap);
extern void _rwResHeapFree(void* memory);
extern void* _rwResHeapAlloc(RwResHeap* heap, RwUInt32 size);




static RwResourcesGlobals* ResourcesInit(RwResourcesGlobals* globals,
                                         RwUInt32 size) {
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

void* _rwResourcesOpen(void* object, RwInt32 offset, RwInt32 size) {
    RwUInt32 arenaSize;

    resourcesModule.globalsOffset = offset;
    arenaSize = RwEngineInstance->resArenaInitSize;
    if (ResourcesInit(&(*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)), arenaSize) == 0) {
        return 0;
    }
    resourcesModule.numInstances++;
    return object;
}

void* _rwResourcesClose(void* object, RwInt32 offset, RwInt32 size) {
    RwResourcesEmptyArena();
    _rwResHeapClose((*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arena);
    RwEngineInstance->fpFree((*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arena);
    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arena = 0;
    resourcesModule.numInstances--;
    return object;
}

RwBool RwResourcesFreeResEntry(RwResEntry* entry) {
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
        (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arenaUsage -= entry->size;
        _rwResHeapFree(entry);
    } else {
        RwEngineInstance->fpFree(entry);
    }
    return 1;
}




void _rwResourcesPurge(void) {
    RwLLLink* active = (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).activeList;
    RwLLLink* alloc = (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList;

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
    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList = active;
    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).activeList = alloc;
    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arenaReusage = 0;
}

RwResEntry* RwResourcesAllocateResEntry(
    void* owner, RwResEntry** ownerRef, RwInt32 size,
    RwResEntryDestroyNotify destroyNotify) {
    RwResEntry* entry;
    RwBool exhausted = 0;

    while (exhausted == 0) {
        entry = _rwResHeapAlloc((*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arena,
                                size + sizeof(RwResEntry));
        if (entry != 0) {
            entry->link.next = (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->next;
            entry->link.prev = (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList;
            (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->next->prev = &entry->link;
            (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->next = &entry->link;
            entry->owner = owner;
            entry->size = size;
            entry->ownerRef = ownerRef;
            entry->destroyNotify = destroyNotify;
            (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arenaUsage += size;
            if (ownerRef != 0) {
                *ownerRef = entry;
            }
            return entry;
        }
        {
            RwLLLink* tail = (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).activeList->prev;

            if (tail != (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).activeList) {
                entry = (RwResEntry*)tail;
                RwResourcesFreeResEntry(entry);
            } else {
                tail = (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList->prev;
                if (tail != (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).allocList) {
                    entry = (RwResEntry*)tail;
                    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arenaReusage += entry->size;
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

RwBool RwResourcesSetArenaSize(RwUInt32 size) {
    RwResourcesGlobals* globals;

    if (resourcesModule.numInstances == 0) {
        RwEngineInstance->resArenaInitSize = size;
        return 1;
    }
    globals = &(*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset));
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



RwBool RwResourcesEmptyArena(void) {
    RwLLLink* link;
    RwLLLink* end;

    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.prev->next =
        (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link.next;
    link = (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.next;
    end = &(*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link;
    while (link != end) {
        RwResEntry* entry = (RwResEntry*)link;

        link = link->next;
        RwResourcesFreeResEntry(entry);
    }
    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.next = &(*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link;
    {
        RwLLLink* sentinelA = &(*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link;
        (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesA.link.prev = sentinelA;
    }
    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link.next = &(*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link;
    {
        RwLLLink* sentinelB = &(*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link;
        (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).entriesB.link.prev = sentinelB;
    }
    (*(RwResourcesGlobals*)((RwUInt8*)RwEngineInstance + resourcesModule.globalsOffset)).arenaReusage = 0;
    return 1;
}
