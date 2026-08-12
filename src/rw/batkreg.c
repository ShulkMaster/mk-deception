#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"

static RwFreeList toolkitRegEntriesSpace;
static int _rwPluginRegFreeListBlockSize = 0x40;
static int _rwPluginRegListPreallocBlocks = 1;
static RwPluginRegistry** toolkitNonFLRegList;
static unsigned int numRegToolkits;
static RwFreeList* toolkitRegEntries;

extern unsigned int _rwGetNumEngineInstances(void);

int _rwPluginRegistryOpen(void) {
    toolkitRegEntries = RwFreeListCreateAndPreallocateSpace(
        sizeof(RwPluginRegEntry), _rwPluginRegFreeListBlockSize, 4,
        _rwPluginRegListPreallocBlocks, &toolkitRegEntriesSpace, 0x40000);
    if (toolkitRegEntries == 0) {
        return 0;
    }
    numRegToolkits = 0;
    return 1;
}

static void rwDestroyEntry(void* memory, void* data) {
    RwPluginRegEntry* entry = memory;

    if (entry->parentRegistry->firstRegEntry != 0) {
        entry->parentRegistry->sizeOfStruct =
            entry->parentRegistry->origSizeOfStruct;
        entry->parentRegistry->firstRegEntry = 0;
        entry->parentRegistry->lastRegEntry = 0;
    }
    RwEngineInstance->fpFreeListFree(data, entry);
}




int _rwPluginRegistryClose(void) {
    if (toolkitRegEntries != 0) {
        RwFreeListForAllUsed(toolkitRegEntries, rwDestroyEntry,
                             toolkitRegEntries);
        if (RwEngineInstance->fpFreeListAlloc !=
            (RwFreeListAllocCall)_rwFreeListAllocReal) {
            unsigned int i;
            for (i = 0; i < numRegToolkits; i++) {
                RwPluginRegEntry* entry =
                    toolkitNonFLRegList[i]->firstRegEntry;
                RwPluginRegistry* parent =
                    entry != 0 ? entry->parentRegistry : 0;
                while (entry != 0) {
                    RwPluginRegEntry* next = entry->nextRegEntry;
                    RwEngineInstance->fpFreeListFree(0, entry);
                    entry = next;
                }
                if (parent != 0 && parent->firstRegEntry != 0) {
                    parent->sizeOfStruct = parent->origSizeOfStruct;
                    parent->firstRegEntry = 0;
                    parent->lastRegEntry = 0;
                }
            }
            if (toolkitNonFLRegList != 0) {
                RwEngineInstance->fpFree(toolkitNonFLRegList);
                toolkitNonFLRegList = 0;
            }
        }
        RwFreeListDestroy(toolkitRegEntries);
        toolkitRegEntries = 0;
    }
    return 1;
}

static void* PluginDefaultConstructor(void* object, int offset,
                                      int size) {
    return object;
}

static void* PluginDefaultDestructor(void* object, int offset,
                                     int size) {
    return object;
}

static void* PluginDefaultCopy(void* destination, const void* source,
                               int offset, int size) {
    return destination;
}

int _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* registry,
                                         unsigned int pluginID) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != 0) {
        if (entry->pluginID == pluginID) {
            return entry->offset;
        }
        entry = entry->nextRegEntry;
    }
    return -1;
}

int _rwPluginRegistryAddPlugin(
    RwPluginRegistry* registry, int size, unsigned int pluginID,
    RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    RwPluginRegEntry* entry;
    int newSize;

    if (toolkitRegEntries == 0) {
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
        unsigned int index;
        for (index = 0; index < numRegToolkits; index++) {
            if (registry == toolkitNonFLRegList[index]) {
                break;
            }
        }
        if (numRegToolkits == index) {
            RwPluginRegistry** newList = RwEngineInstance->fpMalloc(
                (numRegToolkits + 1) * sizeof(*newList), 0x40000);
            unsigned int copyIndex = 0;
            if (toolkitNonFLRegList != 0) {
                while (copyIndex < numRegToolkits) {
                    newList[copyIndex] = toolkitNonFLRegList[copyIndex];
                    copyIndex++;
                }
                RwEngineInstance->fpFree(toolkitNonFLRegList);
                toolkitNonFLRegList = 0;
            }
            newList[copyIndex] = registry;
            numRegToolkits++;
            toolkitNonFLRegList = newList;
        }
    }
    entry = registry->firstRegEntry;
    while (entry != 0) {
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
    if (entry != 0) {
        entry->offset = registry->sizeOfStruct;
        registry->sizeOfStruct = newSize;
        entry->size = size;
        entry->pluginID = pluginID;
        entry->readCB = 0;
        entry->writeCB = 0;
        entry->getSizeCB = 0;
        entry->alwaysCB = 0;
        entry->rightsCB = 0;
        entry->constructCB = constructCB != 0 ? constructCB
                                                 : PluginDefaultConstructor;
        entry->destructCB = destructCB != 0 ? destructCB
                                               : PluginDefaultDestructor;
        entry->copyCB = copyCB != 0 ? copyCB : PluginDefaultCopy;
        entry->errStrCB = 0;
        entry->nextRegEntry = 0;
        entry->prevRegEntry = 0;
        entry->parentRegistry = registry;
        if (registry->firstRegEntry == 0) {
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




const RwPluginRegistry* _rwPluginRegistryInitObject(
    const RwPluginRegistry* registry, void* object) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != 0) {
        if (entry->constructCB(object, entry->offset, entry->size) == 0) {
            entry = entry->prevRegEntry;
            while (entry != 0) {
                entry->destructCB(object, entry->offset, entry->size);
                entry = entry->prevRegEntry;
            }
            return 0;
        }
        entry = entry->nextRegEntry;
    }
    return registry;
}

const RwPluginRegistry* _rwPluginRegistryDeInitObject(
    const RwPluginRegistry* registry, void* object) {
    RwPluginRegEntry* entry = registry->lastRegEntry;
    while (entry != 0) {
        RwPluginObjectDestructor destructCB = entry->destructCB;
        int offset = entry->offset;
        int size = entry->size;
        destructCB(object, offset, size);
        entry = entry->prevRegEntry;
    }
    return registry;
}
