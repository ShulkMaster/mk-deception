#include "platform/gcinstance.h"

typedef void (*RwResEntryDestroyNotify)(RwResEntry* entry);

struct RwResEntry {
    RwResEntry* next;                 /* +0x00 */
    RwResEntry* prev;                 /* +0x04 */
    unsigned int size;                /* +0x08 */
    void* owner;                      /* +0x0C */
    RwResEntry** owner_ref;           /* +0x10 */
    RwResEntryDestroyNotify destroy;  /* +0x14 */
};

typedef struct GcNativeMesh {
    union {
        unsigned int display_list_offset;
        unsigned char* display_list;
    };
    unsigned int display_list_size;
} GcNativeMesh;

typedef struct GcNativeMeshHeader {
    unsigned short display_list_token;
    unsigned short field_0x02;
    unsigned int field_0x04;
    unsigned int num_meshes;
    GcNativeMesh meshes[1];
} GcNativeMeshHeader;

typedef struct GcInstanceEngine {
    unsigned char pad_0x00[0x134];
    void* (*allocate)(unsigned int size, unsigned int hint);
    unsigned char pad_0x138[0x0C];
    void* (*free_list_allocate)(void* free_list, unsigned int hint);
} GcInstanceEngine;

typedef struct GcSkinGlobals {
    unsigned char pad_0x00[0x18];
    void* skin_free_list;
} GcSkinGlobals;

typedef struct RpSkin {
    unsigned int num_bones;
    unsigned int data_size;
    unsigned char* bone_remap;
    unsigned char* skin_to_bone_matrices;
    unsigned int max_vertex_weights;
    unsigned char pad_0x14[0x10];
    unsigned char* vertex_indices;
    unsigned char* vertex_weights;
    unsigned char pad_0x2C[0x20];
} RpSkin; /* 0x4C */

typedef struct GcNativeTextureHeader {
    int platform;
    unsigned int filter_addressing;
    int lod_field_0x08;
    int lod_field_0x0C;
    int lod_field_0x10;
    float lod_bias;
    char name[32];
    char mask[32];
} GcNativeTextureHeader;

typedef struct GcNativeRasterHeader {
    int format;
    unsigned short width;
    unsigned short height;
    unsigned char depth;
    unsigned char field_0x09;
    unsigned char tile_mode;
    unsigned char palette_format;
    unsigned int has_alpha;
} GcNativeRasterHeader;

typedef struct GcRasterExt {
    unsigned char pad_0x00[0x0C];
    int tile_mode;
    int palette_format;
    int has_alpha;
    unsigned char pad_0x18[4];
    unsigned char* image_data;
    unsigned char* palette_data;
} GcRasterExt;

extern GcInstanceEngine* RwEngineInstance;
extern GcSkinGlobals _rpSkinGlobals;
extern unsigned short _RwDlTokenCurrent;
extern int _RwGameCubeRasterExtOffset;
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
extern void RwErrorSet(int* error);
extern void DCFlushRange(void* address, unsigned int length);
extern void GXInvalidateVtxCache(void);
extern void GXInvalidateTexAll(void);
extern void GXInitTlutObj(GcRasterExt* extension, void* palette,
                          int palette_format, unsigned short entries);
extern RwTexture* RwTextureCreate(RwRaster* raster);
extern RwRaster* RwRasterDestroy(RwRaster* raster);
extern RwTexture* RwTextureSetName(RwTexture* texture, const char* name);
extern RwTexture* RwTextureSetMaskName(RwTexture* texture, const char* name);
extern void RwGameCubeTextureSetLOD(RwTexture* texture, int field_0x0C,
                                    int field_0x10, int field_0x08,
                                    float lod_bias);
extern void* memset(void* destination, int value, unsigned int size);
extern RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);

/* RenderWare publishes this plugin at a runtime-selected offset. */
#define GC_RASTER_EXTENSION(raster) \
    ((GcRasterExt*)((unsigned char*)(raster) + _RwGameCubeRasterExtOffset))
#define ALIGN_POINTER_4(pointer) \
    ((unsigned char*)(((unsigned int)(pointer) + 3) & ~3U))

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
        int error[2];

        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
    if (version < 0x34002) {
        int error[2];

        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
    if (!RwStreamReadInt32(stream, &platform, 4)) {
        return 0;
    }
    if (platform != 6) {
        return 0;
    }

    skin = RwEngineInstance->free_list_allocate(_rpSkinGlobals.skin_free_list,
                                                 0x30116);
    memset(skin, 0, sizeof(RpSkin));
    if (!RwStreamReadInt32(stream, &packed_counts, 4)) {
        return 0;
    }
    skin->num_bones = (unsigned char)packed_counts;
    skin->data_size = (unsigned char)(packed_counts >> 8);
    skin->max_vertex_weights = (unsigned char)(packed_counts >> 16);
    vertex_count = geometry->numVertices;
    chunk_length -= 8;

    if (skin->max_vertex_weights > 1) {
        skin->vertex_indices =
            RwEngineInstance->allocate(chunk_length + 5, 0x30116);
        skin->vertex_weights =
            skin->vertex_indices + skin->max_vertex_weights * vertex_count;
        skin->vertex_weights = ALIGN_POINTER_4(skin->vertex_weights);
        skin->skin_to_bone_matrices =
            skin->vertex_weights + skin->max_vertex_weights * vertex_count;
        skin->skin_to_bone_matrices =
            ALIGN_POINTER_4(skin->skin_to_bone_matrices);
        skin->bone_remap =
            skin->skin_to_bone_matrices + skin->num_bones * 64;

        chunk_length = skin->data_size;
        if (RwStreamRead(stream, skin->bone_remap, chunk_length) != chunk_length) {
            return 0;
        }
        chunk_length = skin->max_vertex_weights * vertex_count;
        if (RwStreamRead(stream, skin->vertex_weights, chunk_length) !=
            chunk_length) {
            return 0;
        }
        chunk_length = skin->max_vertex_weights * vertex_count;
        if (RwStreamRead(stream, skin->vertex_indices, chunk_length) !=
            chunk_length) {
            return 0;
        }
        chunk_length = skin->num_bones * 64;
        if (RwStreamRead(stream, skin->skin_to_bone_matrices, chunk_length) !=
            chunk_length) {
            return 0;
        }
    } else {
        skin->vertex_indices =
            RwEngineInstance->allocate(chunk_length + 3, 0x30116);
        skin->skin_to_bone_matrices = skin->vertex_indices;
        skin->skin_to_bone_matrices =
            ALIGN_POINTER_4(skin->skin_to_bone_matrices);
        skin->bone_remap =
            skin->skin_to_bone_matrices + skin->num_bones * 64;

        chunk_length = skin->data_size;
        if (RwStreamRead(stream, skin->bone_remap, chunk_length) != chunk_length) {
            return 0;
        }
        chunk_length = skin->num_bones * 64;
        if (RwStreamRead(stream, skin->skin_to_bone_matrices, chunk_length) !=
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

#pragma dont_inline on
static void* _rpNativeRead(RwStream* stream, void* owner, RwResEntry** entry,
                           unsigned int mesh_count) {
    unsigned int version;
    unsigned int chunk_length;
    int resource_size;
    int display_list_size;
    int platform;
    int error[2];
    GcNativeMeshHeader* native_header;
    unsigned char* stream_data;
    unsigned int padding;
    unsigned int index;
    GcNativeMesh* extra_meshes;

    if (!RwStreamFindChunk(stream, 1, &chunk_length, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
    if (version <= 0x34004) {
        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
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
    native_header = (GcNativeMeshHeader*)((*entry) + 1);
    if (RwStreamRead(stream, native_header, resource_size) != resource_size) {
        return 0;
    }

    padding = PadSize32(stream->bufferPosition);
    RwStreamSkip(stream, padding);
    stream_data = stream->data + stream->bufferPosition;
    RwStreamSkip(stream, display_list_size - padding);

    extra_meshes =
        &native_header->meshes[native_header->num_meshes - 1] + 1;
    for (index = 0; index < native_header->num_meshes; index++) {
        native_header->meshes[index].display_list =
            stream_data + native_header->meshes[index].display_list_offset;
    }
    for (index = 0; index < mesh_count; index++) {
        extra_meshes[index].display_list =
            stream_data + extra_meshes[index].display_list_offset;
    }

    (*entry)->next = 0;
    (*entry)->prev = 0;
    (*entry)->owner = owner;
    (*entry)->size = chunk_length;
    (*entry)->owner_ref = entry;
    (*entry)->destroy = _rxGCResEntryWaitDone;
    native_header->display_list_token = _RwDlTokenCurrent;
    DCFlushRange((*entry) + 1, (*entry)->size);
    GXInvalidateVtxCache();
    return owner;
}
#pragma dont_inline reset

/*
 * Soft ceilings: _rpNativeRead 96.23%, _inplaceNativeTextureRead 94.20%.
 * Their remaining differences are MWCC register coloring and load scheduling;
 * stream layout, relocation loops, access widths, and function sizes agree.
 */

int _inplaceNativeTextureRead(RwStream* stream, RwTexture** texture) {
    unsigned int chunk_length;
    unsigned int version;
    GcNativeTextureHeader texture_header;
    GcNativeRasterHeader raster_header;
    unsigned int image_size;
    unsigned int padding;
    unsigned int raster_format_bit;
    unsigned char* stream_data;
    RwRaster* raster;
    GcRasterExt* extension;
    RwTexture* result;

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
    extension = GC_RASTER_EXTENSION(raster);
    extension->tile_mode = raster_header.tile_mode;
    extension->palette_format = raster_header.palette_format;
    extension->has_alpha = raster_header.has_alpha != 0;

    raster_format_bit = raster->format & 0x10;
    raster->format &= ~raster_format_bit;
    if (RwStreamRead(stream, &image_size, sizeof(image_size)) !=
        sizeof(image_size)) {
        return 0;
    }
    padding = PadSize32(stream->bufferPosition);
    RwStreamSkip(stream, padding);
    image_size -= padding;
    stream_data = stream->data + stream->bufferPosition;
    RwStreamSkip(stream, image_size);
    extension->image_data = stream_data;
    DCFlushRange(extension->image_data, image_size);

    if ((raster->format << 8) & 0x6000) {
        stream_data = stream->data + stream->bufferPosition;
        RwStreamSkip(stream, (1 << raster->depth) * 2);
        extension->palette_data = stream_data;
        DCFlushRange(extension->palette_data, (1 << raster->depth) * 2);
        GXInitTlutObj(extension, extension->palette_data,
                      extension->palette_format,
                      (unsigned short)(1 << raster->depth));
    }
    GXInvalidateTexAll();
    raster->format |= raster_format_bit;

    result = RwTextureCreate(raster);
    if (result == 0) {
        RwRasterDestroy(raster);
        return 0;
    }
    result->filter_flags =
        (result->filter_flags & ~0xff) |
        (unsigned char)texture_header.filter_addressing;
    result->filter_flags = (result->filter_flags & ~0xf00) |
                           (texture_header.filter_addressing & 0xf00);
    result->filter_flags = (result->filter_flags & ~0xf000) |
                           (texture_header.filter_addressing & 0xf000);
    RwTextureSetName(result, texture_header.name);
    RwTextureSetMaskName(result, texture_header.mask);
    RwGameCubeTextureSetLOD(result, texture_header.lod_field_0x0C,
                            texture_header.lod_field_0x10,
                            texture_header.lod_field_0x08,
                            texture_header.lod_bias);
    *texture = result;
    return 1;
}
