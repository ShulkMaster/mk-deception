#include "runtime/instance.h"
#include "platform/gcinstance.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

typedef struct RwEngineInstanceType {
    unsigned char pad_0x00[0x134];
    void* (*fpMalloc)(unsigned int size, unsigned int hint);
} RwEngineInstanceType;

typedef struct RpClumpChunkInfo {
    int num_atomics;
    int num_lights;
    int num_cameras;
} RpClumpChunkInfo;

typedef struct RpAtomicChunkInfo {
    int frame_index;
    int geometry_index;
    unsigned int flags;
    int unused;
} RpAtomicChunkInfo;

typedef struct RpGeometryChunkInfo {
    unsigned int format;
    int num_triangles;
    int num_vertices;
    int num_morph_targets;
} RpGeometryChunkInfo;

typedef struct RpMorphTargetChunkInfo {
    RwSphere sphere;
    int has_vertices;
    int has_normals;
} RpMorphTargetChunkInfo;

extern RwEngineInstanceType* RwEngineInstance;
extern RwPluginRegistry atomicTKList;
extern RwPluginRegistry clumpTKList;
extern RwPluginRegistry geometryTKList;
extern int lastSeenExtraData;
extern unsigned int lastSeenRightsPluginId;

extern int _rwerror(int code, ...);
extern void RwErrorSet(int* error);
extern void* memset(void* destination, int value, unsigned int length);

extern RpClump* RpClumpCreate(void);
extern int RpClumpDestroy(RpClump* clump);
extern RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic);
extern RpAtomic* RpAtomicCreate(void);
extern int RpAtomicDestroy(RpAtomic* atomic);
extern RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame);
extern RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry,
                                     unsigned int flags);
extern RpGeometry* RpGeometryStreamRead(RwStream* stream);
extern int RpGeometryDestroy(RpGeometry* geometry);
extern RpGeometry* RpGeometryUnlock(RpGeometry* geometry);

extern int _inplaceNativeTextureRead(RwStream* stream, RwTexture** texture);

static RpAtomic* inplaceClumpAtomicStreamRead(RwStream* stream,
                                              RwFrameList* frame_list,
                                              RpGeometryList* geometry_list);
static RpGeometryList* inplaceGeometryListStreamRead(
    RwStream* stream, RpGeometryList* geometry_list);
static RpGeometry* inplaceGeometryStreamRead(RwStream* stream);
static int inplaceGeometryAddMorphTargets(RpGeometry* geometry, int count);

RpClump* inplaceClumpStreamRead(RwStream* input_stream) {
    RpClumpChunkInfo chunk_info;
    unsigned int length;
    unsigned int version;
    unsigned int chunk_version;
    RpClump* clump;
    int index;
    RwStream* stream = input_stream;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        int error[2];
        error[0] = 0x116;
        error[1] = _rwerror(0x8000001A);
        RwErrorSet(error);
        return 0;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        RwFrameList frame_list;
        RpGeometryList geometry_list;
        int read_ok =
            RwStreamRead(stream, &chunk_info, sizeof(chunk_info)) ==
            sizeof(chunk_info);

        if (read_ok == 0) {
            int error[2];
            error[0] = 0x116;
            error[1] = _rwerror(0x8000001A);
            RwErrorSet(error);
            return 0;
        }
        RwMemNative32(&chunk_info, sizeof(chunk_info));
        clump = RpClumpCreate();
        if (clump == 0) {
            return 0;
        }
        if (!RwStreamFindChunk(stream, 0xE, 0, &chunk_version)) {
            int error[2];
            RpClumpDestroy(clump);
            error[0] = 0x116;
            error[1] = _rwerror(0x8000001A);
            RwErrorSet(error);
            return 0;
        }
        {
            int read_ok = _rwFrameListStreamRead(stream, &frame_list) != 0;
            if (read_ok == 0) {
                int error[2];
                RpClumpDestroy(clump);
                error[0] = 0x116;
                error[1] = _rwerror(0x8000001A);
                RwErrorSet(error);
                return 0;
            }
        }
        clump->object.parent = frame_list.frames[0];
        if (!RwStreamFindChunk(stream, 0x1A, 0, &chunk_version)) {
            int error[2];
            _rwFrameListDeinitialize(&frame_list);
            RpClumpDestroy(clump);
            error[0] = 0x116;
            error[1] = _rwerror(0x8000001A);
            RwErrorSet(error);
            return 0;
        }
        {
            int read_ok =
                inplaceGeometryListStreamRead(stream, &geometry_list) != 0;
            if (read_ok == 0) {
                int error[2];
                _rwFrameListDeinitialize(&frame_list);
                RpClumpDestroy(clump);
                error[0] = 0x116;
                error[1] = _rwerror(0x8000001A);
                RwErrorSet(error);
                return 0;
            }
        }

        index = 0;
        while (index < chunk_info.num_atomics) {
            RpAtomic* atomic;

            if (RwStreamFindChunk(stream, 0x14, 0, &version)) {
                atomic = inplaceClumpAtomicStreamRead(
                    stream, &frame_list, &geometry_list);
            } else {
                int error[2];
                GeometryListDeinitialize(&geometry_list);
                _rwFrameListDeinitialize(&frame_list);
                RpClumpDestroy(clump);
                error[0] = 0x116;
                error[1] = _rwerror(0x8000001A);
                RwErrorSet(error);
                return 0;
            }
            RpClumpAddAtomic(clump, atomic);
            index++;
        }
        GeometryListDeinitialize(&geometry_list);
        _rwFrameListDeinitialize(&frame_list);
        {
            int read_ok =
                _rwPluginRegistryReadDataChunks(&clumpTKList, stream, clump) !=
                0;
            if (read_ok == 0) {
                int error[2];
                RpClumpDestroy(clump);
                error[0] = 0x116;
                error[1] = _rwerror(0x8000001A);
                RwErrorSet(error);
                return 0;
            }
        }
        return clump;
    } else {
        int error[2];
        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
}

static RpAtomic* inplaceClumpAtomicStreamRead(RwStream* stream,
                                              RwFrameList* frame_list,
                                              RpGeometryList* geometry_list) {
    RpAtomicChunkInfo chunk_info;
    unsigned int length;
    unsigned int version;
    RpAtomic* atomic;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        int error[2];
        error[0] = 0x116;
        error[1] = _rwerror(0x8000001A);
        RwErrorSet(error);
        return 0;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        int read_ok;

        memset(&chunk_info, 0, sizeof(chunk_info));
        read_ok = RwStreamRead(stream, &chunk_info, length) == length;
        if (read_ok == 0) {
            int error[2];
            error[0] = 0x116;
            error[1] = _rwerror(0x8000001A);
            RwErrorSet(error);
            return 0;
        }
        RwMemNative32(&chunk_info, sizeof(chunk_info));
        atomic = RpAtomicCreate();
        if (atomic == 0) {
            return 0;
        }
        atomic->object.flags = (unsigned char)chunk_info.flags;
        if (frame_list->numFrames != 0) {
            RpAtomicSetFrame(atomic,
                             frame_list->frames[chunk_info.frame_index]);
        }
        if (geometry_list->numGeometries != 0) {
            RpAtomicSetGeometry(
                atomic, geometry_list->geometries[chunk_info.geometry_index],
                0);
        } else {
            RpGeometry* geometry;

            if (!RwStreamFindChunk(stream, 0xF, 0, &version)) {
                int error[2];
                RpAtomicDestroy(atomic);
                error[0] = 0x116;
                error[1] = _rwerror(0x8000001A);
                RwErrorSet(error);
                return 0;
            }
            if (version >= 0x34000 && version <= 0x36003) {
                geometry = RpGeometryStreamRead(stream);
                if (geometry == 0) {
                    int error[2];
                    RpAtomicDestroy(atomic);
                    error[0] = 0x116;
                    error[1] = _rwerror(0x8000001A);
                    RwErrorSet(error);
                    return 0;
                }
            } else {
                int error[2];
                RpAtomicDestroy(atomic);
                error[0] = 0x116;
                error[1] = _rwerror(0x80000004);
                RwErrorSet(error);
                return 0;
            }
            RpAtomicSetGeometry(atomic, geometry, 0);
            RpGeometryDestroy(geometry);
        }

        lastSeenRightsPluginId = 0;
        lastSeenExtraData = 0;
        {
            int read_ok =
                _rwPluginRegistryReadDataChunks(&atomicTKList, stream, atomic) !=
                0;
            if (read_ok == 0) {
                int error[2];
                error[0] = 0x116;
                error[1] = _rwerror(0x8000001A);
                RwErrorSet(error);
                return 0;
            }
        }
        if (lastSeenRightsPluginId != 0) {
            _rwPluginRegistryInvokeRights(
                &atomicTKList, lastSeenRightsPluginId, atomic,
                lastSeenExtraData);
        }
        return atomic;
    } else {
        int error[2];
        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
}

static RpGeometryList* inplaceGeometryListStreamRead(
    RwStream* stream, RpGeometryList* geometry_list) {
    int count;
    unsigned int length;
    unsigned int version;
    int index;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return 0;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        if (RwStreamRead(stream, &count, sizeof(count)) != sizeof(count)) {
            return 0;
        }
        RwMemNative32(&count, sizeof(count));
        geometry_list->numGeometries = 0;
        if (count > 0) {
            geometry_list->geometries = RwEngineInstance->fpMalloc(
                count * sizeof(*geometry_list->geometries), 0x3000F);
            if (geometry_list->geometries == 0) {
                int error[2];
                error[0] = 0x116;
                error[1] = _rwerror(
                    0x80000013, count * sizeof(*geometry_list->geometries));
                RwErrorSet(error);
                return 0;
            }
        } else {
            geometry_list->geometries = 0;
        }

        index = 0;
        while (index < count) {
            if (!RwStreamFindChunk(stream, 0xF, 0, &version)) {
                GeometryListDeinitialize(geometry_list);
                return 0;
            }
            if (version >= 0x34000 && version <= 0x36003) {
                geometry_list->geometries[index] =
                    inplaceGeometryStreamRead(stream);
                if (geometry_list->geometries[index] == 0) {
                    GeometryListDeinitialize(geometry_list);
                    return 0;
                }
                geometry_list->numGeometries++;
            } else {
                GeometryListDeinitialize(geometry_list);
                return 0;
            }
            index++;
        }
    } else {
        int error[2];
        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
    return geometry_list;
}

static RpGeometry* inplaceGeometryStreamRead(RwStream* stream) {
    RpGeometryChunkInfo chunk_info;
    unsigned int version;
    RpGeometry* geometry;
    int morph_result;
    int morph_index;
    RpMorphTargetChunkInfo morph_info;
    RpMorphTarget* morph_target;
    const RwPluginRegistry* plugin_result;
    unsigned char* inplace_pointer;

    if (!RwStreamFindChunk(stream, 1, 0, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        int error[2];
        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
    if (RwStreamRead(stream, &chunk_info, sizeof(chunk_info)) !=
        sizeof(chunk_info)) {
        return 0;
    }
    RwMemNative32(&chunk_info, sizeof(chunk_info));
    geometry = inplaceGeometryCreate_80056E98(
        chunk_info.num_vertices, chunk_info.num_triangles, chunk_info.format);
    if (geometry == 0) {
        return 0;
    }
    morph_result = inplaceGeometryAddMorphTargets(
        geometry, chunk_info.num_morph_targets);
    if (morph_result < 0) {
        RpGeometryDestroy(geometry);
        return 0;
    }

    if ((geometry->flags & 0x01000000) == 0) {
        int vertex_count = geometry->numVertices;

        if (vertex_count != 0) {
            if (chunk_info.format & 8) {
                inplace_pointer = stream->data.memory.start +
                                  stream->data.memory.position;
                geometry->preLitLum = inplace_pointer;
                RwStreamSkip(stream, vertex_count << 2);
            }
            if (geometry->numTexCoordSets > 0) {
                unsigned int tex_coord_size = geometry->numVertices << 3;

                morph_index = 0;
                while (morph_index < geometry->numTexCoordSets) {
                    inplace_pointer = stream->data.memory.start +
                                      stream->data.memory.position;
                    geometry->texCoords[morph_index] = inplace_pointer;
                    RwStreamSkip(stream, tex_coord_size);
                    morph_index++;
                }
            }
            if (geometry->numTriangles != 0) {
                int triangle_count = geometry->numTriangles;

                inplace_pointer = stream->data.memory.start +
                                  stream->data.memory.position;
        geometry->triangles = (RpTriangle*)inplace_pointer;
                RwStreamSkip(stream, triangle_count << 3);
            }
        }
    }

    morph_index = 0;
    while (morph_index < geometry->numMorphTargets) {
        morph_target = &geometry->morphTarget[morph_index];
        if (RwStreamRead(stream, &morph_info, sizeof(morph_info)) !=
            sizeof(morph_info)) {
            RpGeometryDestroy(geometry);
            return 0;
        }
        RwMemNative32(&morph_info, sizeof(morph_info));
        morph_target->sphere = morph_info.sphere;
        if (morph_info.has_vertices != 0) {
            inplace_pointer = stream->data.memory.start +
                              stream->data.memory.position;
            morph_target->verts = inplace_pointer;
            RwStreamSkip(stream, geometry->numVertices * 12);
        }
        if (morph_info.has_normals != 0) {
            inplace_pointer = stream->data.memory.start +
                              stream->data.memory.position;
            morph_target->normals = inplace_pointer;
            RwStreamSkip(stream, geometry->numVertices * 12);
        }
        morph_index++;
    }

    if (!RwStreamFindChunk(stream, 8, 0, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        int error[2];
        RpGeometryDestroy(geometry);
        error[0] = 0x116;
        error[1] = _rwerror(0x80000004);
        RwErrorSet(error);
        return 0;
    }
    if (!_rpMaterialListStreamRead(stream, &geometry->matList)) {
        RpGeometryDestroy(geometry);
        return 0;
    }
    plugin_result =
        _rwPluginRegistryReadDataChunks(&geometryTKList, stream, geometry);
    if (plugin_result == 0) {
        RpGeometryDestroy(geometry);
        return 0;
    }
    if (geometry->flags & 0x01000000) {
        if (inplaceGeometryNativeRead(stream, geometry) == 0) {
            RpGeometryDestroy(geometry);
            return 0;
        }
        if (chunk_info.format & 0x100) {
            if (inplaceSkinGeometryNativeRead(stream, geometry) == 0) {
                RpGeometryDestroy(geometry);
                return 0;
            }
        }
    }
    if (RpGeometryUnlock(geometry) == 0) {
        RpGeometryDestroy(geometry);
        return 0;
    }
    return geometry;
}

RpGeometry* inplaceGeometryCreate_80056E98(int num_vertices, int num_triangles,
                                           unsigned int format) {
    RwEngineInstanceType* engine;
    int plugin_size;
    unsigned int num_tex_coord_sets;
    unsigned int tex_coord_flags;
    unsigned int format_flags;
    unsigned int combined_flags;
    RpGeometry* geometry;

    if (num_vertices < 0 || num_vertices >= 0x10000 || num_triangles < 0) {
        if (num_vertices >= 0) {
            if (num_vertices >= 0x10000) {
                int error[2];
                error[0] = 0x116;
                error[1] = _rwerror(6);
                RwErrorSet(error);
            }
        }
        return 0;
    }
    plugin_size = geometryTKList.sizeOfStruct;
    format_flags = format & 0xFF;
    if ((format & 0xFF0000) != 0) {
        num_tex_coord_sets = (format & 0xFF0000) >> 16;
    } else if (format & 0x80) {
        num_tex_coord_sets = 2;
    } else {
        num_tex_coord_sets = (format >> 2) & 1;
    }
    if (num_tex_coord_sets == 1) {
        tex_coord_flags = 4;
    } else {
        tex_coord_flags = num_tex_coord_sets > 1 ? 0x80 : 0;
    }
    format_flags &= ~0x84;
    combined_flags = format_flags | tex_coord_flags;

    engine = RwEngineInstance;
    geometry = engine->fpMalloc(plugin_size, 0x3000F);
    if (geometry == 0) {
        return 0;
    }
    if (_rpMaterialListInitialize(&geometry->matList) == 0U) {
        return 0;
    }
    geometry->morphTarget = 0;
    geometry->numMorphTargets = 0;
    geometry->object.type = 8;
    geometry->object.subType = 0;
    geometry->object.flags = 0;
    geometry->object.privateFlags = 0;
    geometry->object.parent = 0;
    geometry->repEntry = 0;
    geometry->lockedSinceLastInst = 0;
    geometry->refCount = 1;
    geometry->meshHeader = 0;
    geometry->numTexCoordSets = num_tex_coord_sets;
    memset(geometry->texCoords, 0, sizeof(geometry->texCoords));
    geometry->preLitLum = 0;
    geometry->triangles = 0;
    geometry->numTriangles = num_triangles;
    geometry->flags = combined_flags | (format & 0x0F000000);
    geometry->numVertices = num_vertices;
    _rwPluginRegistryInitObject(&geometryTKList, geometry);
    return geometry;
}

static int inplaceGeometryAddMorphTargets(RpGeometry* geometry, int count) {
    RwEngineInstanceType* engine;
    RpMorphTarget* morph_data;
    RpMorphTarget* morph;
    unsigned int allocation_size;
    int morph_target_size;
    int total_targets;
    int index;
    float zero;

    /* Native and non-native streams share the same morph-target header size. */
    morph_target_size = (geometry->flags & 0x01000000)
                            ? sizeof(*morph)
                            : sizeof(*morph);
    total_targets = geometry->numMorphTargets + count;
    allocation_size = morph_target_size * total_targets;
    if (geometry->morphTarget != 0) {
        return -1;
    }
    engine = RwEngineInstance;
    morph_data = engine->fpMalloc(allocation_size, 0x3000F);
    if (morph_data == 0) {
        int error[2];
        error[0] = 0x116;
        error[1] = _rwerror(0x80000013, allocation_size);
        RwErrorSet(error);
        return -1;
    }
    geometry->numMorphTargets += count;
    geometry->morphTarget = morph_data;
    for (index = 0; index < geometry->numMorphTargets; index++) {
        morph = &geometry->morphTarget[index];
        morph->verts = 0;
        morph->normals = 0;
    }
    zero = 0.0f;
    for (index = geometry->numMorphTargets - count;
         index < geometry->numMorphTargets; index++) {
        morph = &geometry->morphTarget[index];
        morph->sphere.x = zero;
        morph->sphere.y = zero;
        morph->sphere.z = zero;
        morph->sphere.radius = zero;
        morph->parentGeom = geometry;
    }
    return geometry->numMorphTargets - count;
}

unsigned int PadSize32(unsigned int value) {
    return ((value + 0x1F) & ~0x1FU) - value;
}

int inplaceNativeTextureRead(RwStream* stream, RwTexture** texture) {
    return _inplaceNativeTextureRead(stream, texture);
}
