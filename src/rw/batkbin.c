#include "rw/rwerror.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

RwInt32 _rwPluginRegistryAddPluginStream(
    RwPluginRegistry* registry, RwUInt32 pluginID,
    RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != 0) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != 0) {
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
    while (entry != 0) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != 0) {
        entry->alwaysCB = alwaysCB;
        return entry->offset;
    }
    return -1;
}

RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(
    RwPluginRegistry* registry, RwUInt32 pluginID,
    RwPluginDataChunkRightsCallBack rightsCB) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != 0) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != 0) {
        entry->rightsCB = rightsCB;
        return entry->offset;
    }
    return -1;
}


const RwPluginRegistry* _rwPluginRegistryReadDataChunks(
    const RwPluginRegistry* registry, RwStream* stream, void* object) {
    RwUInt32 version;
    RwUInt32 length;

    if (!RwStreamFindChunk(stream, 3, &length, &version)) {
        return 0;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        while (length != 0) {
            RwUInt32 pluginID;
            RwUInt32 pluginDataLength;
            RwPluginRegEntry* entry;

            if (_rwStreamReadChunkHeader(stream, &pluginID, &pluginDataLength,
                                         0, 0) == 0) {
                return 0;
            }
            entry = registry->firstRegEntry;
            while (entry != 0) {
                if (entry->pluginID == pluginID) {
                    break;
                }
                entry = entry->nextRegEntry;
            }
            if (entry != 0 && entry->readCB != 0) {
                if (entry->readCB(stream, pluginDataLength, object,
                                  entry->offset, entry->size) == 0) {
                    return 0;
                }
            } else if (RwStreamSkip(stream, pluginDataLength) == 0) {
                return 0;
            }
            length -= pluginDataLength + 12;
        }
        {
            RwPluginRegEntry* entry = registry->firstRegEntry;
            while (entry != 0) {
                if (entry->alwaysCB != 0 &&
                    !entry->alwaysCB(object, entry->offset, entry->size)) {
                    return 0;
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
        return 0;
    }
}

const RwPluginRegistry* _rwPluginRegistryInvokeRights(
    const RwPluginRegistry* registry, RwUInt32 pluginID, void* object,
    RwUInt32 extraData) {
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != 0) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }
    if (entry != 0 && entry->rightsCB != 0) {
        if (!entry->rightsCB(object, entry->offset, entry->size, extraData)) {
            return 0;
        }
        return registry;
    }
    return 0;
}

RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry* registry,
                                 const void* object) {

    const void* pluginObject = object;
    RwInt32 size = 0;
    RwPluginRegEntry* entry = registry->firstRegEntry;
    while (entry != 0) {
        if (entry->getSizeCB != 0) {
            RwInt32 pluginSize = entry->getSizeCB(pluginObject, entry->offset,
                                                   entry->size);
            if (pluginSize > 0) {
                size += pluginSize;
                size += 12;
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
            0xFFFF) == 0) {
        return 0;
    }
    entry = registry->firstRegEntry;
    while (entry != 0) {
        if (entry->getSizeCB != 0 && entry->writeCB != 0) {
            RwInt32 size = entry->getSizeCB(object, entry->offset, entry->size);
            if (size > 0) {
                if (_rwStreamWriteVersionedChunkHeader(
                        stream, entry->pluginID, size, 0x36003, 0xFFFF) ==
                    0) {
                    return 0;
                }
                if (entry->writeCB(stream, size, object, entry->offset,
                                   entry->size) == 0) {
                    return 0;
                }
            }
        }
        entry = entry->nextRegEntry;
    }
    return registry;
}


const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(
    const RwPluginRegistry* registry, RwStream* stream) {
    RwUInt32 length;

    if (!RwStreamFindChunk(stream, 3, &length, 0)) {
        return 0;
    }
    while (length != 0) {
        RwUInt32 pluginDataLength;
        if (_rwStreamReadChunkHeader(stream, 0, &pluginDataLength, 0,
                                     0) == 0) {
            return 0;
        }
        if (RwStreamSkip(stream, pluginDataLength) == 0) {
            return 0;
        }
        length -= pluginDataLength + 12;
    }
    return registry;
}
