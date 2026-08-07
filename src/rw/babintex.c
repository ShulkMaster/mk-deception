#include "runtime/cstring.h"
#include "rw/rwcore_types.h"
#include "rw/rwerror.h"
#include "rw/rwstream_internal.h"

extern RwPluginRegistry textureTKList;
extern RwInt32 _rwStringStreamGetSize(const RwChar *string);
extern const RwChar *_rwStringStreamWrite(const RwChar *string, RwStream *stream);
extern RwChar *_rwStringStreamFindAndRead(RwChar *string, RwStream *stream);
extern RwStream *_rwStreamWriteVersionedChunkHeader(RwStream *, RwUInt32, RwUInt32, RwUInt32,
                                                    RwUInt32);
extern RwStream *RwStreamWrite(RwStream *, const void *, RwUInt32);
extern RwBool RwStreamFindChunk(RwStream *, RwUInt32, RwUInt32 *, RwUInt32 *);
extern RwUInt32 RwStreamRead(RwStream *, void *, RwUInt32);
extern void RwMemLittleEndian32(void *, RwUInt32);
extern void RwMemNative32(void *, RwUInt32);

/* Soft ceiling: only the commutative add operand order differs from retail. */
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
                                           0xFFFF) == NULL) {
        return NULL;
    }
    if (_rwStreamWriteVersionedChunkHeader(stream, 1, 4, 0x36003, 0xFFFF) == NULL) {
        return NULL;
    }
    if (texture->raster != NULL && !(texture->raster->format & 0x10)) {
        noAutoMip = TRUE;
    } else {
        noAutoMip = FALSE;
    }
    filterAddressing = (RwUInt16)texture->filter_flags | ((RwUInt32)noAutoMip << 16);
    RwMemLittleEndian32(&filterAddressing, sizeof(filterAddressing));
    if (RwStreamWrite(stream, &filterAddressing, sizeof(filterAddressing)) == NULL) {
        return NULL;
    }
    if (_rwStringStreamWrite(texture->name, stream) == NULL) {
        return NULL;
    }
    if (_rwStringStreamWrite(texture->mask, stream) == NULL) {
        return NULL;
    }
    if (_rwPluginRegistryWriteDataChunks(&textureTKList, stream, texture) == NULL) {
        return NULL;
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
    RwTextureFilterMode filterMode;
    RwTextureAddressMode addressingV;
    RwTextureAddressMode addressingU;
    RwInt32 autoMipMap;
    RwBool mipMapping;
    RwBool autoMipMapping;
    RwTexture *texture;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return NULL;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        memset(&packed, 0, sizeof(packed));
        if (RwStreamRead(stream, &packed, length) != length) {
            return NULL;
        }
        RwMemNative32(&packed, sizeof(packed));
        filterMode = (RwTextureFilterMode)(RwUInt8)packed;
        addressingU = (RwTextureAddressMode)((packed >> 8) & 0xF);
        addressingV = (RwTextureAddressMode)((packed >> 12) & 0xF);
        if (addressingV == 0) {
            addressingV = addressingU;
            packed |= ((RwUInt32)addressingV & 0xF) << 12;
        }
        autoMipMap = (packed >> 16) & 0xFF;

        mipMapping = RwTextureGetMipmapping();
        autoMipMapping = RwTextureGetAutoMipmapping();
        if (filterMode == rwFILTERMIPNEAREST || filterMode == rwFILTERMIPLINEAR ||
            filterMode == rwFILTERLINEARMIPNEAREST || filterMode == rwFILTERLINEARMIPLINEAR) {
            RwTextureSetMipmapping(TRUE);
            if (autoMipMap & 1) {
                RwTextureSetAutoMipmapping(FALSE);
            } else {
                RwTextureSetAutoMipmapping(TRUE);
            }
        } else {
            RwTextureSetMipmapping(FALSE);
            RwTextureSetAutoMipmapping(FALSE);
        }

        if (_rwStringStreamFindAndRead(name, stream) == NULL) {
            RwTextureSetMipmapping(mipMapping);
            RwTextureSetAutoMipmapping(autoMipMapping);
            return NULL;
        }
        if (_rwStringStreamFindAndRead(mask, stream) == NULL) {
            RwTextureSetMipmapping(mipMapping);
            RwTextureSetAutoMipmapping(autoMipMapping);
            return NULL;
        }
        texture = RwTextureRead(name, mask);
        if (texture == NULL) {
            _rwPluginRegistrySkipDataChunks(&textureTKList, stream);
            RwTextureSetMipmapping(mipMapping);
            RwTextureSetAutoMipmapping(autoMipMapping);
            return NULL;
        }

        RwTextureSetMipmapping(mipMapping);
        RwTextureSetAutoMipmapping(autoMipMapping);
        if (texture->ref_count == 1) {
            texture->filter_flags = (RwUInt16)packed;
            if (_rwPluginRegistryReadDataChunks(&textureTKList, stream, texture) == NULL) {
                return NULL;
            }
        } else if (_rwPluginRegistrySkipDataChunks(&textureTKList, stream) == NULL) {
            return NULL;
        }
        return texture;
    } else {
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }
}
