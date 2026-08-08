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

#define RESOURCESGLOBAL \
    RWPLUGINOFFSET(RwResourcesGlobals, RwEngineInstance, \
                   resourcesModule.globalsOffset)

extern RwBool _rwResHeapInit(RwResHeap* heap, RwUInt32 size);
extern RwBool _rwResHeapClose(RwResHeap* heap);
extern void _rwResHeapFree(void* memory);
extern void* _rwResHeapAlloc(RwResHeap* heap, RwUInt32 size);

/* Near miss: allocation, failures, globals, and list topology match. Retail
 * materializes the two list sentinels in r29/r28 and uses savegpr helpers;
 * equivalent typed locals make this compiler select individual saves. */
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
    if (ResourcesInit(&RESOURCESGLOBAL, arenaSize) == 0) {
        return 0;
    }
    resourcesModule.numInstances++;
    return object;
}

void* _rwResourcesClose(void* object, RwInt32 offset, RwInt32 size) {
    RwResourcesEmptyArena();
    _rwResHeapClose(RESOURCESGLOBAL.arena);
    RwEngineInstance->fpFree(RESOURCESGLOBAL.arena);
    RESOURCESGLOBAL.arena = 0;
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
        RESOURCESGLOBAL.arenaUsage -= entry->size;
        _rwResHeapFree(entry);
    } else {
        RwEngineInstance->fpFree(entry);
    }
    return TRUE;
}

void _rwResourcesPurge(void) {
    RwLLLink* active = RESOURCESGLOBAL.activeList;
    RwLLLink* alloc = RESOURCESGLOBAL.allocList;

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
    RESOURCESGLOBAL.allocList = active;
    RESOURCESGLOBAL.activeList = alloc;
    RESOURCESGLOBAL.arenaReusage = 0;
}

/* Near miss: retail retains an unused copy of the allocated entry in r25 but
 * returns the original pointer; reproducing its larger frame requires dead
 * liveness. The allocation, eviction, ownership, and failure paths match. */
RwResEntry* RwResourcesAllocateResEntry(
    void* owner, RwResEntry** ownerRef, RwInt32 size,
    RwResEntryDestroyNotify destroyNotify) {
    RwResEntry* entry;
    RwBool exhausted = FALSE;

    while (exhausted == FALSE) {
        entry = _rwResHeapAlloc(RESOURCESGLOBAL.arena,
                                size + sizeof(RwResEntry));
        if (entry != 0) {
            RwResEntry* result;

            entry->link.next = RESOURCESGLOBAL.allocList->next;
            entry->link.prev = RESOURCESGLOBAL.allocList;
            RESOURCESGLOBAL.allocList->next->prev = &entry->link;
            RESOURCESGLOBAL.allocList->next = &entry->link;
            result = entry;
            entry->owner = owner;
            entry->size = size;
            entry->ownerRef = ownerRef;
            entry->destroyNotify = destroyNotify;
            RESOURCESGLOBAL.arenaUsage += size;
            if (ownerRef != 0) {
                *ownerRef = entry;
            }
            return result;
        }
        {
            RwLLLink* tail = RESOURCESGLOBAL.activeList->prev;

            if (tail != RESOURCESGLOBAL.activeList) {
                entry = (RwResEntry*)tail;
                RwResourcesFreeResEntry(entry);
            } else {
                tail = RESOURCESGLOBAL.allocList->prev;
                if (tail != RESOURCESGLOBAL.allocList) {
                    entry = (RwResEntry*)tail;
                    RESOURCESGLOBAL.arenaReusage += entry->size;
                    RwResourcesFreeResEntry(entry);
                } else {
                    exhausted = TRUE;
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
        return TRUE;
    }
    globals = &RESOURCESGLOBAL;
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
        return FALSE;
    }
    if (!_rwResHeapInit(globals->arena, size)) {
        RwError error;

        RwEngineInstance->fpFree(globals->arena);
        error.pluginID = 1;
        error.errorCode = _rwerror(0xC, 0);
        RwErrorSet(&error);
        return FALSE;
    }
    return TRUE;
}

RwBool RwResourcesEmptyArena(void) {
    RwLLLink* link;
    RwLLLink* end;

    RESOURCESGLOBAL.entriesA.link.prev->next =
        RESOURCESGLOBAL.entriesB.link.next;
    link = RESOURCESGLOBAL.entriesA.link.next;
    end = &RESOURCESGLOBAL.entriesB.link;
    while (link != end) {
        RwResEntry* entry = (RwResEntry*)link;

        link = link->next;
        RwResourcesFreeResEntry(entry);
    }
    RESOURCESGLOBAL.entriesA.link.next = &RESOURCESGLOBAL.entriesA.link;
    RESOURCESGLOBAL.entriesA.link.prev = &RESOURCESGLOBAL.entriesA.link;
    RESOURCESGLOBAL.entriesB.link.next = &RESOURCESGLOBAL.entriesB.link;
    RESOURCESGLOBAL.entriesB.link.prev = &RESOURCESGLOBAL.entriesB.link;
    RESOURCESGLOBAL.arenaReusage = 0;
    return TRUE;
}
