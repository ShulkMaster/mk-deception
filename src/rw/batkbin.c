#include "rw/rwerror.h"
#include "rw/rwplcore.h"

extern RwBool RwStreamFindChunk(RwStream*, RwUInt32, RwUInt32*, RwUInt32*);
extern RwStream* RwStreamSkip(RwStream*, RwUInt32);
extern RwStream* _rwStreamReadChunkHeader(RwStream*, RwUInt32*, RwInt32*,
                                          RwUInt32*, RwUInt32*);
extern RwStream* _rwStreamWriteVersionedChunkHeader(
    RwStream*, RwUInt32, RwUInt32, RwUInt32, RwUInt32);

RwInt32 _rwPluginRegistryAddPluginStream(
    RwPluginRegistry* registry, RwUInt32 pluginID,
    RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != NULL) {
        entry->readCB = readCB;
        entry->writeCB = writeCB;
        entry->getSizeCB = getSizeCB;
        return entry->offset;
    }
    return -1;
}

RwInt32 _rwPluginRegistryAddPlgnStrmlwysCB(
    RwPluginRegistry* registry, RwUInt32 pluginID,
    RwPluginDataChunkAlwaysCallBack alwaysCB) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != NULL) {
        entry->alwaysCB = alwaysCB;
        return entry->offset;
    }
    return -1;
}

RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(
    RwPluginRegistry* registry, RwUInt32 pluginID,
    RwPluginDataChunkRightsCallBack rightsCB) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != NULL) {
        entry->rightsCB = rightsCB;
        return entry->offset;
    }
    return -1;
}

/* Soft ceiling: only stack-slot and nonvolatile-register allocation differs. */
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(
    const RwPluginRegistry* registry, RwStream* stream, void* object) {
    RwUInt32 version;
    RwUInt32 length;

    if (!RwStreamFindChunk(stream, 3, &length, &version)) {
        return NULL;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        while (length != 0) {
            RwUInt32 pluginID;
            RwInt32 pluginDataLength;
            RwPluginRegEntry* entry;

            if (_rwStreamReadChunkHeader(stream, &pluginID, &pluginDataLength,
                                         NULL, NULL) == NULL) {
                return NULL;
            }
            entry = registry->firstRegEntry;
            while (entry != NULL) {
                if (entry->pluginID == pluginID) {
                    break;
                }
                entry = entry->nextRegEntry;
            }
            if (entry != NULL && entry->readCB != NULL) {
                if (entry->readCB(stream, pluginDataLength, object,
                                  entry->offset, entry->size) == NULL) {
                    return NULL;
                }
            } else if (RwStreamSkip(stream, pluginDataLength) == NULL) {
                return NULL;
            }
            length -= pluginDataLength + 12;
        }
        {
            RwPluginRegEntry* entry = registry->firstRegEntry;
            while (entry != NULL) {
                if (entry->alwaysCB != NULL &&
                    !entry->alwaysCB(object, entry->offset, entry->size)) {
                    return NULL;
                }
                entry = entry->nextRegEntry;
            }
        }
        return registry;
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }
}

/* Soft ceiling: only argument stack-homing and register allocation differs. */
const RwPluginRegistry* _rwPluginRegistryInvokeRights(
    const RwPluginRegistry* registry, RwUInt32 pluginID, void* object,
    RwUInt32 extraData) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != NULL && entry->rightsCB != NULL &&
        entry->rightsCB(object, entry->offset, entry->size, extraData)) {
        return registry;
    }
    return NULL;
}

/* Soft ceiling: only stack homing, register allocation, and add scheduling differ. */
RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry* registry,
                                 const void* object) {
    const void* pluginObject = object;
    RwInt32 size = 0;
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->getSizeCB != NULL) {
            RwInt32 pluginSize = entry->getSizeCB(pluginObject, entry->offset,
                                                   entry->size);
            if (pluginSize > 0) {
                size = pluginSize + size + 12;
            }
        }
        entry = entry->nextRegEntry;
    }
    return size;
}

const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(
    const RwPluginRegistry* registry, RwStream* stream,
    const void* object) {
    RwPluginRegEntry* entry;

    if (_rwStreamWriteVersionedChunkHeader(
            stream, 3, _rwPluginRegistryGetSize(registry, object), 0x36003,
            0xFFFF) == NULL) {
        return NULL;
    }
    entry = registry->firstRegEntry;
    while (entry != NULL) {
        if (entry->getSizeCB != NULL && entry->writeCB != NULL) {
            RwInt32 size = entry->getSizeCB(object, entry->offset, entry->size);
            if (size > 0) {
                if (_rwStreamWriteVersionedChunkHeader(
                        stream, entry->pluginID, size, 0x36003, 0xFFFF) ==
                    NULL) {
                    return NULL;
                }
                if (entry->writeCB(stream, size, object, entry->offset,
                                   entry->size) == NULL) {
                    return NULL;
                }
            }
        }
        entry = entry->nextRegEntry;
    }
    return registry;
}

/* Soft ceiling: only stack-local allocation and equivalent subtraction emission differ. */
const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(
    const RwPluginRegistry* registry, RwStream* stream) {
    RwUInt32 length;

    if (!RwStreamFindChunk(stream, 3, &length, NULL)) {
        return NULL;
    }
    while (length != 0) {
        RwInt32 pluginDataLength;
        if (_rwStreamReadChunkHeader(stream, NULL, &pluginDataLength, NULL,
                                     NULL) == NULL) {
            return NULL;
        }
        if (RwStreamSkip(stream, pluginDataLength) == NULL) {
            return NULL;
        }
        length -= pluginDataLength + 12;
    }
    return registry;
}
