#include "runtime/cstring.h"
#include "rw/batextur.h"
#include "rw/rwcore_types.h"
#include "rw/rwerror.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

extern RwPluginRegistry textureTKList;


RwUInt32 RwTextureStreamGetSize(const RwTexture *texture) {
    RwInt32 size = 0x10;

    size = _rwStringStreamGetSize(texture->name) + size;
    size += 0xC;
    size = _rwStringStreamGetSize(texture->mask) + size;
    size += 0xC;
    size = _rwPluginRegistryGetSize(&textureTKList, texture) + size;
    size += 0xC;
    return size;
}

const RwTexture *RwTextureStreamWrite(const RwTexture *texture, RwStream *stream) {
    RwUInt32 filterAddressing;
    RwUInt8 noAutoMip;

    if (_rwStreamWriteVersionedChunkHeader(stream, 6, RwTextureStreamGetSize(texture), 0x36003,
                                           0xFFFF) == 0) {
        return 0;
    }
    if (_rwStreamWriteVersionedChunkHeader(stream, 1, 4, 0x36003, 0xFFFF) == 0) {
        return 0;
    }
    if (texture->raster != 0 && !(texture->raster->format & 0x10)) {
        noAutoMip = 1;
    } else {
        noAutoMip = 0;
    }
    filterAddressing = (RwUInt16)texture->filter_flags | ((RwUInt32)noAutoMip << 16);
    RwMemLittleEndian32(&filterAddressing, sizeof(filterAddressing));
    if (RwStreamWrite(stream, &filterAddressing, sizeof(filterAddressing)) == 0) {
        return 0;
    }
    if (_rwStringStreamWrite(texture->name, stream) == 0) {
        return 0;
    }
    if (_rwStringStreamWrite(texture->mask, stream) == 0) {
        return 0;
    }
    if (_rwPluginRegistryWriteDataChunks(&textureTKList, stream, texture) == 0) {
        return 0;
    }
    return texture;
}

RwTexture *RwTextureStreamRead(RwStream *stream) {
    RwChar name[128];
    RwChar mask[128];
    RwError error;
    RwUInt32 length;
    RwUInt32 version;
    RwUInt32 packed;
    RwInt32 filterMode;
    RwInt32 addressingV;
    RwInt32 addressingU;
    RwInt32 autoMipMap;
    RwBool mipMapping;
    RwBool autoMipMapping;
    RwTexture *texture;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return 0;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        memset(&packed, 0, sizeof(packed));
        if (RwStreamRead(stream, &packed, length) != length) {
            return 0;
        }
        RwMemNative32(&packed, sizeof(packed));
        filterMode = (RwUInt8)packed;
        addressingU = (packed >> 8) & 0xF;
        addressingV = (packed >> 12) & 0xF;
        if (addressingV == 0) {
            addressingV = addressingU;
            packed |= ((RwUInt32)addressingV & 0xF) << 12;
        }
        autoMipMap = (packed >> 16) & 0xFF;

        mipMapping = RwTextureGetMipmapping();
        autoMipMapping = RwTextureGetAutoMipmapping();
        if (filterMode == 3 || filterMode == 4 ||
            filterMode == 5 || filterMode == 6) {
            RwTextureSetMipmapping(1);
            if (autoMipMap & 1) {
                RwTextureSetAutoMipmapping(0);
            } else {
                RwTextureSetAutoMipmapping(1);
            }
        } else {
            RwTextureSetMipmapping(0);
            RwTextureSetAutoMipmapping(0);
        }

        if (_rwStringStreamFindAndRead(name, stream) == 0) {
            RwTextureSetMipmapping(mipMapping);
            RwTextureSetAutoMipmapping(autoMipMapping);
            return 0;
        }
        if (_rwStringStreamFindAndRead(mask, stream) == 0) {
            RwTextureSetMipmapping(mipMapping);
            RwTextureSetAutoMipmapping(autoMipMapping);
            return 0;
        }
        texture = RwTextureRead(name, mask);
        if (texture == 0) {
            _rwPluginRegistrySkipDataChunks(&textureTKList, stream);
            RwTextureSetMipmapping(mipMapping);
            RwTextureSetAutoMipmapping(autoMipMapping);
            return 0;
        }

        RwTextureSetMipmapping(mipMapping);
        RwTextureSetAutoMipmapping(autoMipMapping);
        if (texture->ref_count == 1) {
            texture->filter_flags = (RwUInt16)packed;
            if (_rwPluginRegistryReadDataChunks(&textureTKList, stream, texture) == 0) {
                return 0;
            }
        } else if (_rwPluginRegistrySkipDataChunks(&textureTKList, stream) == 0) {
            return 0;
        }
        return texture;
    } else {
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
}
