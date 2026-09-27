#include "platform/gcinstance.h"
#include "runtime/cstring.h"
#include "dolphin/gx.h"
#include "rw/dltextur.h"
#include "rw/dltoken.h"
#include "rw/gamecube_globals.h"
#include "rw/gamecube_texture.h"
#include "rw/native_internal.h"
#include "rw/rpskin.h"
#include "rw/rwresentry.h"

typedef struct GcInstanceEngine {
    unsigned char pad_0x00[0x134];
    void* (*allocate)(unsigned int size, unsigned int hint);
    unsigned char pad_0x138[0x0C];
    void* (*free_list_allocate)(void* free_list, unsigned int hint);
} GcInstanceEngine;

extern GcInstanceEngine* RwEngineInstance;
extern void _rxGCResEntryWaitDone(RwResEntry* entry);

extern int RwStreamFindChunk(RwStream* stream, unsigned int type,
                             unsigned int* length, unsigned int* version);
extern unsigned int RwStreamReadInt32(RwStream* stream, void* values,
                                      unsigned int length);
extern unsigned int RwStreamRead(RwStream* stream, void* destination,
                                 unsigned int length);
extern RwStream* RwStreamSkip(RwStream* stream, unsigned int offset);
extern int PadSize32(unsigned int value);
extern int _rwerror(int code, ...);
extern RwError* RwErrorSet(RwError* error);
extern void DCFlushRange(void* address, unsigned int length);
extern RwTexture* RwTextureSetMaskName(RwTexture* texture, const char* name);
/* RenderWare publishes this plugin at a runtime-selected offset. */
static unsigned char* AlignPointer4(const void* pointer) {
    return (unsigned char*)(((unsigned int)pointer + 3) & ~3U);
}

static void* _rpNativeRead(RwStream* stream, void* owner, RwResEntry** entry,
                           unsigned int mesh_count);

RwStream* inplaceSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry) {
    unsigned int version;
    unsigned int chunk_length;
    unsigned int packed_counts;
    int platform;
    unsigned int vertex_count;
    RpSkin* skin;

    if (!RwStreamFindChunk(stream, 1, &chunk_length, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        RwError error;

        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (version < 0x34002) {
        RwError error;

        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (!RwStreamReadInt32(stream, &platform, 4)) {
        return 0;
    }
    if (platform != 6) {
        return 0;
    }

    skin = RwEngineInstance->free_list_allocate(_rpSkinGlobals.skinFreeList,
                                                 0x30116);
    memset(skin, 0, sizeof(RpSkin));
    if (!RwStreamReadInt32(stream, &packed_counts, 4)) {
        return 0;
    }
    skin->numBones = (unsigned char)packed_counts;
    skin->numUsedBones = (unsigned char)(packed_counts >> 8);
    skin->maxNumWeights = (unsigned char)(packed_counts >> 16);
    vertex_count = geometry->numVertices;
    chunk_length -= 8;

    if (skin->maxNumWeights > 1) {
        skin->platformWeights =
            RwEngineInstance->allocate(chunk_length + 5, 0x30116);
        skin->platformIndices =
            (unsigned char*)skin->platformWeights +
            skin->maxNumWeights * vertex_count;
        skin->platformIndices = AlignPointer4(skin->platformIndices);
        skin->skinToBoneMatrices =
            (RwMatrix*)((unsigned char*)skin->platformIndices +
                        skin->maxNumWeights * vertex_count);
        skin->skinToBoneMatrices =
            (RwMatrix*)AlignPointer4(skin->skinToBoneMatrices);
        skin->usedBoneList =
            (unsigned char*)skin->skinToBoneMatrices + skin->numBones * 64;

        chunk_length = skin->numUsedBones;
        if (RwStreamRead(stream, skin->usedBoneList, chunk_length) != chunk_length) {
            return 0;
        }
        chunk_length = skin->maxNumWeights * vertex_count;
        if (RwStreamRead(stream, skin->platformIndices, chunk_length) !=
            chunk_length) {
            return 0;
        }
        chunk_length = skin->maxNumWeights * vertex_count;
        if (RwStreamRead(stream, skin->platformWeights, chunk_length) !=
            chunk_length) {
            return 0;
        }
        chunk_length = skin->numBones * 64;
        if (RwStreamRead(stream, skin->skinToBoneMatrices, chunk_length) !=
            chunk_length) {
            return 0;
        }
    } else {
        skin->platformWeights =
            RwEngineInstance->allocate(chunk_length + 3, 0x30116);
        skin->skinToBoneMatrices = (RwMatrix*)skin->platformWeights;
        skin->skinToBoneMatrices =
            (RwMatrix*)AlignPointer4(skin->skinToBoneMatrices);
        skin->usedBoneList =
            (unsigned char*)skin->skinToBoneMatrices + skin->numBones * 64;

        chunk_length = skin->numUsedBones;
        if (RwStreamRead(stream, skin->usedBoneList, chunk_length) != chunk_length) {
            return 0;
        }
        chunk_length = skin->numBones * 64;
        if (RwStreamRead(stream, skin->skinToBoneMatrices, chunk_length) !=
            chunk_length) {
            return 0;
        }
    }
    RpSkinGeometrySetSkin(geometry, skin);
    return stream;
}

RpGeometry* inplaceGeometryNativeRead(RwStream* stream, RpGeometry* geometry) {
    return _rpNativeRead(stream, geometry, &geometry->repEntry,
                         geometry->meshHeader->numMeshes);
}

static void _inplaceNativeOffset2Pointer(GameCubeNativeMeshHeader* native,
                                        GameCubeNativeMesh* meshes,
                                        unsigned int numMeshes,
                                        unsigned char* vertexData) {
    unsigned int i;

    for (i = 0; i < native->numMeshes; i++) {
        native->meshes[i].displayList.pointer =
            vertexData + native->meshes[i].displayList.offset;
    }
    for (i = 0; i < numMeshes; i++) {
        meshes[i].displayList.pointer =
            vertexData + meshes[i].displayList.offset;
    }
}

static void* _rpNativeRead(RwStream* stream, void* owner, RwResEntry** entry,
                           unsigned int mesh_count) {
    unsigned int version;
    unsigned int chunk_length;
    int resource_size;
    int display_list_size;
    int platform;
    GameCubeNativeMeshHeader* native_header;
    unsigned int padding;
    GameCubeNativeMesh* extra_meshes;
    unsigned char* stream_data;

    if (!RwStreamFindChunk(stream, 1, &chunk_length, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        RwError error;

        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (version <= 0x34004) {
        RwError error;

        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (!RwStreamReadInt32(stream, &platform, 4)) {
        return 0;
    }
    if (platform != 6) {
        return 0;
    }
    if (!RwStreamReadInt32(stream, &resource_size, 4)) {
        return 0;
    }
    if (!RwStreamReadInt32(stream, &display_list_size, 4)) {
        return 0;
    }

    *entry = RwEngineInstance->allocate(resource_size + sizeof(RwResEntry),
                                         0x3050d);
    native_header = (GameCubeNativeMeshHeader*)((*entry) + 1);
    if (RwStreamRead(stream, native_header, resource_size) != resource_size) {
        return 0;
    }

    padding = PadSize32(stream->data.memory.position);
    RwStreamSkip(stream, padding);
    stream_data = stream->data.memory.start + stream->data.memory.position;
    RwStreamSkip(stream, display_list_size - padding);

    extra_meshes = (GameCubeNativeMesh*)(native_header + 1) +
                   (native_header->numMeshes - 1);
    _inplaceNativeOffset2Pointer(native_header, extra_meshes, mesh_count,
                                 stream_data);

    (*entry)->link.next = 0;
    (*entry)->link.prev = 0;
    (*entry)->owner = owner;
    (*entry)->size = chunk_length;
    (*entry)->ownerRef = entry;
    (*entry)->destroyNotify = _rxGCResEntryWaitDone;
    native_header->token = _RwDlTokenCurrent;
    DCFlushRange((*entry) + 1, (*entry)->size);
    GXInvalidateVtxCache();
    return owner;
}

int _inplaceNativeTextureRead(RwStream* stream, RwTexture** texture) {
    RwTexture* result;
    unsigned int chunk_length;
    unsigned int version;
    GameCubeNativeTextureHeader texture_header;
    GameCubeNativeRasterHeader raster_header;
    unsigned int image_size;
    unsigned int padding;
    unsigned int raster_format_bit;
    unsigned char* stream_data;
    RwRaster* raster;
    RwGameCubeRasterExt* extension;

    if (!RwStreamFindChunk(stream, 1, &chunk_length, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        return 0;
    }
    if (RwStreamRead(stream, &texture_header, sizeof(texture_header)) !=
        sizeof(texture_header)) {
        return 0;
    }
    if (texture_header.platform != 6) {
        return 0;
    }
    if (RwStreamRead(stream, &raster_header, sizeof(raster_header)) !=
        sizeof(raster_header)) {
        return 0;
    }

    raster = RwRasterCreate(raster_header.width, raster_header.height,
                            raster_header.depth, raster_header.format | 0x80);
    if (raster == 0) {
        return 0;
    }
    extension = (RwGameCubeRasterExt*)((unsigned char*)raster +
                                       _RwGameCubeRasterExtOffset);
    extension->format = raster_header.tileMode;
    extension->paletteFormat = raster_header.paletteFormat;
    extension->hasAlpha = raster_header.hasAlpha != 0;

    raster_format_bit = raster->format & 0x10;
    raster->format &= ~raster_format_bit;
    if (RwStreamRead(stream, &image_size, sizeof(image_size)) !=
        sizeof(image_size)) {
        return 0;
    }
    padding = PadSize32(stream->data.memory.position);
    RwStreamSkip(stream, padding);
    image_size -= padding;
    stream_data = stream->data.memory.start + stream->data.memory.position;
    RwStreamSkip(stream, image_size);
    extension->imageData = stream_data;
    DCFlushRange(extension->imageData, image_size);

    if ((raster->format << 8) & 0x6000) {
        stream_data = stream->data.memory.start + stream->data.memory.position;
        RwStreamSkip(stream, (1 << raster->depth) * 2);
        extension->paletteData = stream_data;
        DCFlushRange(extension->paletteData, (1 << raster->depth) * 2);
        /* The TLUT object is the extension's first member and retail passes
         * the extension base directly here. */
        GXInitTlutObj((GXTlutObj*)extension, extension->paletteData,
                      extension->paletteFormat,
                      (unsigned short)(1 << raster->depth));
    }
    GXInvalidateTexAll();
    raster->format |= raster_format_bit;

    result = RwTextureCreate(raster);
    if (result == 0) {
        RwRasterDestroy(raster);
        return 0;
    }
    /* RwTextureSetFilterMode/SetAddressingU/SetAddressingV, expanded. */
    result->filter_flags = (result->filter_flags & ~0xff) |
                           ((unsigned char)texture_header.filterAddressing & 0xff);
    result->filter_flags = (result->filter_flags & ~0xf00) |
                           (texture_header.filterAddressing & 0xf00);
    result->filter_flags = (result->filter_flags & ~0xf000) |
                           (texture_header.filterAddressing & 0xf000);
    RwTextureSetName(result, texture_header.name);
    RwTextureSetMaskName(result, texture_header.mask);
    RwGameCubeTextureSetLOD(result, texture_header.lodBias,
                            texture_header.biasClamp,
                            texture_header.edgeLod,
                            texture_header.maxAnisotropy);
    *texture = result;
    return 1;
}
