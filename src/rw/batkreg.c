#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"

static RwFreeList toolkitRegEntriesSpace;
static RwInt32 _rwPluginRegFreeListBlockSize = 0x40;
static RwInt32 _rwPluginRegListPreallocBlocks = 1;
static RwPluginRegistry** toolkitNonFLRegList;
static RwUInt32 numRegToolkits;
static RwFreeList* toolkitRegEntries;

extern RwUInt32 _rwGetNumEngineInstances(void);

RwBool _rwPluginRegistryOpen(void) {
    toolkitRegEntries = RwFreeListCreateAndPreallocateSpace(
        sizeof(RwPluginRegEntry), _rwPluginRegFreeListBlockSize, 4,
        _rwPluginRegListPreallocBlocks, &toolkitRegEntriesSpace, 0x40000);
    if (toolkitRegEntries == NULL) {
        return FALSE;
    }
    numRegToolkits = 0;
    return TRUE;
}

/* Soft ceiling: only callback-argument stack homing and register allocation differ. */
static void rwDestroyEntry(void* memory, void* data) {
    RwPluginRegEntry* entry = memory;
    RwFreeList* freeList = data;

    if (entry->parentRegistry->firstRegEntry != NULL) {
        entry->parentRegistry->sizeOfStruct =
            entry->parentRegistry->origSizeOfStruct;
        entry->parentRegistry->firstRegEntry = NULL;
        entry->parentRegistry->lastRegEntry = NULL;
    }
    RwEngineInstance->fpFreeListFree(freeList, entry);
}

/* Soft ceiling: only local lifetimes and nonvolatile-register allocation differ. */
RwBool _rwPluginRegistryClose(void) {
    if (toolkitRegEntries != NULL) {
        RwFreeListForAllUsed(toolkitRegEntries, rwDestroyEntry,
                             toolkitRegEntries);
        if (RwEngineInstance->fpFreeListAlloc !=
            (RwFreeListAllocCall)_rwFreeListAllocReal) {
            RwUInt32 i;
            for (i = 0; i < numRegToolkits; i++) {
                RwPluginRegEntry* entry =
                    toolkitNonFLRegList[i]->firstRegEntry;
                RwPluginRegistry* parent =
                    entry != NULL ? entry->parentRegistry : NULL;
                while (entry != NULL) {
                    RwPluginRegEntry* next = entry->nextRegEntry;
                    RwEngineInstance->fpFreeListFree(NULL, entry);
                    entry = next;
                }
                if (parent != NULL && parent->firstRegEntry != NULL) {
                    parent->sizeOfStruct = parent->origSizeOfStruct;
                    parent->firstRegEntry = NULL;
                    parent->lastRegEntry = NULL;
                }
            }
        }
        if (toolkitNonFLRegList != NULL) {
            RwEngineInstance->fpFree(toolkitNonFLRegList);
            toolkitNonFLRegList = NULL;
        }
        RwFreeListDestroy(toolkitRegEntries);
        toolkitRegEntries = NULL;
    }
    return TRUE;
}

static void* PluginDefaultConstructor(void* object, RwInt32 offset,
                                      RwInt32 size) {
    return object;
}

static void* PluginDefaultDestructor(void* object, RwInt32 offset,
                                     RwInt32 size) {
    return object;
}

static void* PluginDefaultCopy(void* destination, const void* source,
                               RwInt32 offset, RwInt32 size) {
    return destination;
}

RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* registry,
                                         RwUInt32 pluginID) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->pluginID == pluginID) {
            return entry->offset;
        }
        entry = entry->nextRegEntry;
    }
    return -1;
}

/* Soft ceiling: only register allocation and instruction scheduling differ. */
RwInt32 _rwPluginRegistryAddPlugin(
    RwPluginRegistry* registry, RwInt32 size, RwUInt32 pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    RwPluginRegEntry* entry;
    RwInt32 newSize;

    if (toolkitRegEntries == NULL) {
        return -1;
    }
    if (_rwGetNumEngineInstances() != 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000017);
        RwErrorSet(&error);
        return -1;
    }
    if (RwEngineInstance->fpFreeListAlloc !=
        (RwFreeListAllocCall)_rwFreeListAllocReal) {
        RwUInt32 index;
        for (index = 0; index < numRegToolkits; index++) {
            if (registry == toolkitNonFLRegList[index]) {
                break;
            }
        }
        if (numRegToolkits == index) {
            RwPluginRegistry** newList = RwEngineInstance->fpMalloc(
                (numRegToolkits + 1) * sizeof(*newList), 0x40000);
            RwUInt32 copyIndex = 0;
            if (toolkitNonFLRegList != NULL) {
                while (copyIndex < numRegToolkits) {
                    newList[copyIndex] = toolkitNonFLRegList[copyIndex];
                    copyIndex++;
                }
                RwEngineInstance->fpFree(toolkitNonFLRegList);
                toolkitNonFLRegList = NULL;
            }
            newList[copyIndex] = registry;
            numRegToolkits++;
            toolkitNonFLRegList = newList;
        }
    }
    entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->pluginID == pluginID) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000017);
            RwErrorSet(&error);
            return entry->offset;
        }
        entry = entry->nextRegEntry;
    }
    newSize = registry->sizeOfStruct + ((size + 3) & ~3);
    if (registry->maxSizeOfStruct != 0 &&
        newSize > registry->maxSizeOfStruct) {
        return -1;
    }
    entry = RwEngineInstance->fpFreeListAlloc(toolkitRegEntries, 0x40000);
    if (entry != NULL) {
        entry->offset = registry->sizeOfStruct;
        registry->sizeOfStruct = newSize;
        entry->size = size;
        entry->pluginID = pluginID;
        entry->readCB = NULL;
        entry->writeCB = NULL;
        entry->getSizeCB = NULL;
        entry->alwaysCB = NULL;
        entry->rightsCB = NULL;
        entry->constructCB = constructCB != NULL ? constructCB
                                                 : PluginDefaultConstructor;
        entry->destructCB = destructCB != NULL ? destructCB
                                               : PluginDefaultDestructor;
        entry->copyCB = copyCB != NULL ? copyCB : PluginDefaultCopy;
        entry->errStrCB = NULL;
        entry->nextRegEntry = NULL;
        entry->prevRegEntry = NULL;
        entry->parentRegistry = registry;
        if (registry->firstRegEntry == NULL) {
            registry->firstRegEntry = entry;
            registry->lastRegEntry = entry;
        } else {
            registry->lastRegEntry->nextRegEntry = entry;
            entry->prevRegEntry = registry->lastRegEntry;
            registry->lastRegEntry = entry;
        }
        return entry->offset;
    }
    return -1;
}

/* Soft ceiling: only callback operand loading and register allocation differ. */
const RwPluginRegistry* _rwPluginRegistryInitObject(
    const RwPluginRegistry* registry, void* object) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->constructCB(object, entry->offset, entry->size) == NULL) {
            entry = entry->prevRegEntry;
            while (entry != NULL) {
                entry->destructCB(object, entry->offset, entry->size);
                entry = entry->prevRegEntry;
            }
            return NULL;
        }
        entry = entry->nextRegEntry;
    }
    return registry;
}

/* Soft ceiling: only object stack homing and callback operand scheduling differ. */
const RwPluginRegistry* _rwPluginRegistryDeInitObject(
    const RwPluginRegistry* registry, void* object) {
    RwPluginRegEntry* entry = registry->lastRegEntry;
    while (entry != NULL) {
        entry->destructCB(object, entry->offset, entry->size);
        entry = entry->prevRegEntry;
    }
    return registry;
}
