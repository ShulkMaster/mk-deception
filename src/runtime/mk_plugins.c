#include "runtime/mk_plugins.h"
#include "rw/bamateri.h"
#include "rw/rwstream.h"

typedef struct PluginEngineView {
    char pad00[0x134];
    void* (*allocate)(unsigned int size, unsigned int flags);
    void (*free)(void* memory, struct PluginEngineView* engine);
} PluginEngineView;

typedef struct MkmaterialExtraAllocation {
    MkmaterialExtra extra;
    int inline_data[1];
} MkmaterialExtraAllocation;

extern PluginEngineView* RwEngineInstance;

int MkobjGlobalOffset = -1;
int MkobjLocalOffset = -1;
int MksobjGlobalOffset = -1;
int MksobjLocalOffset = -1;
int MkmaterialGlobalOffset = -1;
int MkmaterialLocalOffset = -1;
int ColorSetGeometryOffset = -1;
int ColorSetInstances;

static void* ColorSetGeometryCopy(void* destination, const void* source, int offset, int size);
static void* ColorSetGeometryDestructor(void* object, int offset, int size);
static void* ColorSetGeometryConstructor(void* object, int offset, int size);
static void* ColorSetOpen(void* object, int offset, int size);
static void* ColorSetClose(void* object, int offset, int size);

static RwStream* MkmaterialDataWriteStream(RwStream* stream, int length, const void* object,
                                           int offset, int size);
static RwStream* MkmaterialDataReadStream(RwStream* stream, int length, void* object, int offset,
                                          int size);
static int MkmaterialDataGetStreamSize(const void* object, int offset, int size);
static void* MkmaterialDataCopier(void* destination, const void* source, int offset, int size);
static void* MkmaterialDataDestructor(void* object, int offset, int size);
static void* MkmaterialDataConstructor(void* object, int offset, int size);
static void* MkmaterialGlobalDataDestructor(void* object, int offset, int size);
static void* MkmaterialGlobalDataConstructor(void* object, int offset, int size);

static RwStream* MksobjDataWriteStream(RwStream* stream, int length, const void* object, int offset,
                                       int size);
static RwStream* MksobjDataReadStream(RwStream* stream, int length, void* object, int offset,
                                      int size);
static int MksobjDataGetStreamSize(const void* object, int offset, int size);
static void* MksobjDataCopier(void* destination, const void* source, int offset, int size);
static void* MksobjDataDestructor(void* object, int offset, int size);
static void* MksobjDataConstructor(void* object, int offset, int size);
static void* MksobjGlobalDataDestructor(void* object, int offset, int size);
static void* MksobjGlobalDataConstructor(void* object, int offset, int size);

static RwStream* MkobjDataWriteStream(RwStream* stream, int length, const void* object, int offset,
                                      int size);
static RwStream* MkobjDataReadStream(RwStream* stream, int length, void* object, int offset,
                                     int size);
static int MkobjDataGetStreamSize(const void* object, int offset, int size);
static void* MkobjDataCopier(void* destination, const void* source, int offset, int size);
static void* MkobjDataDestructor(void* object, int offset, int size);
static void* MkobjDataConstructor(void* object, int offset, int size);
static void* MkobjGlobalDataDestructor(void* object, int offset, int size);
static void* MkobjGlobalDataConstructor(void* object, int offset, int size);

int RpColorSetPluginAttach(void) {
    if (RwEngineRegisterPlugin(0, 0xBA, ColorSetOpen, ColorSetClose) < 0) {
        return 0;
    }
    return (ColorSetGeometryOffset =
                RpGeometryRegisterPlugin(
                    sizeof(ColorSetPluginData), 0x1BA,
                    ColorSetGeometryConstructor, ColorSetGeometryDestructor,
                    ColorSetGeometryCopy)) >= 0;
}

static void* ColorSetGeometryCopy(void* destination, const void* source, int offset, int size) {
    return destination;
}

static void* ColorSetGeometryDestructor(void* object, int offset, int size) {
    RpGeometry* geometry = object;
    ColorSetPluginData* data = COLOR_SET_PLUGIN(geometry);
    unsigned int morph_index;
    unsigned int slot_index;
    unsigned int color_index;

    if (data->count != 0) {
        if (data->entries != 0) {
            for (morph_index = 0; morph_index < (unsigned int)geometry->numMorphTargets;
                 morph_index++) {
                ColorSetEntry* entry = &data->entries[morph_index];
                if (entry->count != 0) {
                    for (slot_index = 0; slot_index < entry->count; slot_index++) {
                        RwEngineInstance->free(entry->ptr_array[slot_index], RwEngineInstance);
                        for (color_index = 0; color_index < data->count; color_index++) {
                            RwEngineInstance->free(entry->arrays[color_index][slot_index],
                                                   RwEngineInstance);
                        }
                        entry->int_arrays_c[slot_index] = 0;
                        entry->int_arrays_10[slot_index] = 0;
                    }
                }
                if (entry->ptr_array != 0) {
                    RwEngineInstance->free(entry->ptr_array, RwEngineInstance);
                }
                if (entry->arrays != 0) {
                    RwEngineInstance->free(entry->arrays, RwEngineInstance);
                }
                if (entry->arrays != 0) {
                    RwEngineInstance->free(entry->int_arrays_c, RwEngineInstance);
                }
                if (entry->arrays != 0) {
                    RwEngineInstance->free(entry->int_arrays_10, RwEngineInstance);
                }
            }
            RwEngineInstance->free(data->entries, RwEngineInstance);
        }
        for (color_index = 0; color_index < data->count; color_index++) {
            RwEngineInstance->free(data->ptr4[color_index], RwEngineInstance);
        }
        RwEngineInstance->free(data->ptr4, RwEngineInstance);
    }
    return object;
}

static void* ColorSetGeometryConstructor(void* object, int offset, int size) {
    COLOR_SET_PLUGIN(object)->count = 0;
    COLOR_SET_PLUGIN(object)->ptr4 = 0;
    COLOR_SET_PLUGIN(object)->entries = 0;
    return object;
}

static void* ColorSetOpen(void* object, int offset, int size) {
    ColorSetInstances++;
    return object;
}

static void* ColorSetClose(void* object, int offset, int size) {
    ColorSetInstances--;
    return object;
}

int RpMaterialMkmaterialPluginAttach(void) {
    MkmaterialGlobalOffset = RwEngineRegisterPlugin(
        4, 0x895303, MkmaterialGlobalDataConstructor, MkmaterialGlobalDataDestructor);
    if (MkmaterialGlobalOffset < 0) {
        return 0;
    }
    MkmaterialLocalOffset = RpMaterialRegisterPlugin(
        sizeof(MkmaterialPluginData), 0x895303, MkmaterialDataConstructor,
        MkmaterialDataDestructor, MkmaterialDataCopier);
    if (MkmaterialLocalOffset < 0) {
        return 0;
    }
    return MkmaterialLocalOffset -
               RpMaterialRegisterPluginStream(
                   0x895303, MkmaterialDataReadStream,
                   MkmaterialDataWriteStream, MkmaterialDataGetStreamSize) ==
           0;
}

static void* MkmaterialDataConstructor(void* object, int offset, int size) {
    if (MkmaterialLocalOffset > 0) {
        MK_MATERIAL_PLUGIN(object)->flags = 0;
        MK_MATERIAL_PLUGIN(object)->field_04 = 0.0f;
        MK_MATERIAL_PLUGIN(object)->bytes_08[0] = 0xFF;
        MK_MATERIAL_PLUGIN(object)->bytes_08[1] = 0xFF;
        MK_MATERIAL_PLUGIN(object)->bytes_08[2] = 0xFF;
        MK_MATERIAL_PLUGIN(object)->bytes_08[3] = 0xFF;
        MK_MATERIAL_PLUGIN(object)->field_0C = 5.0f;
        MK_MATERIAL_PLUGIN(object)->z_bias = 0.0f;
        MK_MATERIAL_PLUGIN(object)->vec4 = 0;
        MK_MATERIAL_PLUGIN(object)->field_18 = 0;
        MK_MATERIAL_PLUGIN(object)->extra = 0;
        MK_MATERIAL_PLUGIN(object)->field_20 = 0;
    }
    return object;
}

static RwStream* MkmaterialDataWriteStream(RwStream* stream, int length, const void* object,
                                           int offset, int size) {
    const MkmaterialExtra* extra;
    const float* vec4;
    int version = 5;
    int index;

    if (stream == 0 || object == 0) {
        return 0;
    }
    extra = MK_MATERIAL_PLUGIN(object)->extra;
    if (extra != 0) {
        version = 0x40000005;
        if (extra->field_00 != 0) {
            version |= 0x20000000;
        }
    }
    vec4 = MK_MATERIAL_PLUGIN(object)->vec4;
    if (vec4 != 0) {
        version |= 0x80000000;
    }
    RwStreamWriteInt32(stream, &version, 4);
    RwStreamWriteInt32(stream, &MK_MATERIAL_PLUGIN(object)->flags, 4);
    RwStreamWriteReal(stream, &MK_MATERIAL_PLUGIN(object)->field_04, 4);
    if (extra != 0) {
        RwStreamWriteInt32(stream, &extra->count, 4);
        for (index = 0; index < extra->count; index++) {
            int value = extra->data[index];
            RwStreamWriteInt32(stream, &value, 4);
        }
    }
    RwStreamWriteInt32(stream, &MK_MATERIAL_PLUGIN(object)->word_08, 4);
    RwStreamWriteReal(stream, &MK_MATERIAL_PLUGIN(object)->field_0C, 4);
    RwStreamWriteReal(stream, &MK_MATERIAL_PLUGIN(object)->z_bias, 4);
    if (vec4 != 0) {
        RwStreamWriteReal(stream, &vec4[0], 4);
        RwStreamWriteReal(stream, &vec4[1], 4);
        RwStreamWriteReal(stream, &vec4[2], 4);
        RwStreamWriteReal(stream, &vec4[3], 4);
    }
    RwStreamWriteInt32(stream, &MK_MATERIAL_PLUGIN(object)->field_20, 4);
    return stream;
}

static RwStream* MkmaterialDataReadStream(RwStream* stream, int length, void* object, int offset,
                                          int size) {
    union {
        float reals[4];
        unsigned int words[4];
    } vec4_values;
    MkmaterialPluginData* data;
    MkmaterialExtra* extra;
    MkmaterialExtraAllocation* extra_allocation;
    int version;
    int extra_count;
    int value;
    int stream_version;
    unsigned int version_flags;
    unsigned int index;
    unsigned int* word_08;
    float field_0C;
    unsigned int* vec4;
    int consumed;

    if (stream == 0 || object == 0) {
        return 0;
    }
    RwStreamReadInt32(stream, &version, 4);
    RwStreamReadInt32(stream, &MK_MATERIAL_PLUGIN(object)->flags, 4);
    RwStreamReadReal(stream, &MK_MATERIAL_PLUGIN(object)->field_04, 4);
    stream_version = (unsigned short)version;
    version_flags = (unsigned int)version >> 16;
    if (stream_version > 1 && (version_flags & 0x4000) != 0) {
        RwStreamReadInt32(stream, &extra_count, 4);
        extra_allocation = RwEngineInstance->allocate(extra_count * 4 + 0xC, 0x30000);
        extra = &extra_allocation->extra;
        extra->field_00 = (version_flags >> 13) & 1;
        extra->count = extra_count;
        extra->data = extra_allocation->inline_data;
        for (index = 0; index < (unsigned int)extra_count; index++) {
            RwStreamReadInt32(stream, &value, 4);
            extra->data[index] = value;
        }
        MK_MATERIAL_PLUGIN(object)->extra = extra;
    }
    if (stream_version > 2) {
        word_08 = &MK_MATERIAL_PLUGIN(object)->word_08;
        RwStreamReadInt32(stream, word_08, 4);
        RwMemNative32(word_08, 4);
    }
    if (stream_version > 3) {
        RwStreamReadReal(stream, &field_0C, 4);
        MK_MATERIAL_PLUGIN(object)->field_0C = field_0C;
        RwStreamReadReal(stream, &MK_MATERIAL_PLUGIN(object)->z_bias, 4);
        if ((version_flags & 0x8000) != 0) {
            RwStreamReadReal(stream, &vec4_values.reals[0], 4);
            RwStreamReadReal(stream, &vec4_values.reals[1], 4);
            RwStreamReadReal(stream, &vec4_values.reals[2], 4);
            RwStreamReadReal(stream, &vec4_values.reals[3], 4);
            vec4 = RwEngineInstance->allocate(0x10, 0x30000);
            if (vec4 != 0) {
                vec4[0] = vec4_values.words[0];
                vec4[1] = vec4_values.words[1];
                vec4[2] = vec4_values.words[2];
                vec4[3] = vec4_values.words[3];
                MK_MATERIAL_PLUGIN(object)->vec4_words = vec4;
            }
        }
    }
    if (stream_version > 4) {
        RwStreamReadInt32(stream, &MK_MATERIAL_PLUGIN(object)->field_20, 4);
    }
    data = MK_MATERIAL_PLUGIN(object);
    consumed = 0x1C;
    if (data->extra != 0) {
        consumed = data->extra->count * 4 + 0x20;
    }
    if (data->vec4 != 0) {
        consumed += 0x10;
    }
    consumed = length - consumed;
    if (consumed > 0) {
        RwStreamSkip(stream, consumed);
    }
    return stream;
}

static int MkmaterialDataGetStreamSize(const void* object, int offset, int size) {
    const MkmaterialPluginData* data = MK_MATERIAL_PLUGIN(object);
    int stream_size = 0x1C;
    if (data->extra != 0) {
        stream_size = data->extra->count * 4 + 0x20;
    }
    if (data->vec4 != 0) {
        stream_size += 0x10;
    }
    return stream_size;
}

static void* MkmaterialDataCopier(void* destination, const void* source, int offset, int size) {
    const MkmaterialPluginData* source_data = MK_MATERIAL_PLUGIN(source);
    MkmaterialExtra* extra_copy;
    MkmaterialExtraAllocation* extra_allocation;
    unsigned int* vec4_copy;
    unsigned int index;
    MK_MATERIAL_PLUGIN(destination)->flags =
        MK_MATERIAL_PLUGIN(source)->flags;
    MK_MATERIAL_PLUGIN(destination)->field_04 =
        MK_MATERIAL_PLUGIN(source)->field_04;
    MK_MATERIAL_PLUGIN(destination)->word_08 =
        MK_MATERIAL_PLUGIN(source)->word_08;
    MK_MATERIAL_PLUGIN(destination)->field_0C =
        MK_MATERIAL_PLUGIN(source)->field_0C;
    MK_MATERIAL_PLUGIN(destination)->z_bias =
        MK_MATERIAL_PLUGIN(source)->z_bias;
    MK_MATERIAL_PLUGIN(destination)->field_20 =
        MK_MATERIAL_PLUGIN(source)->field_20;
    if (source_data->extra != 0) {
        extra_allocation =
            RwEngineInstance->allocate(source_data->extra->count * 4 + 0xC, 0x30000);
        extra_copy = &extra_allocation->extra;
        if (extra_copy != 0) {
            MK_MATERIAL_PLUGIN(destination)->extra = extra_copy;
            extra_copy->field_00 = source_data->extra->field_00;
            extra_copy->count = source_data->extra->count;
            extra_copy->data = extra_allocation->inline_data;
            for (index = 0; index < (unsigned int)source_data->extra->count; index++) {
                extra_copy->data[index] = source_data->extra->data[index];
            }
        }
    }
    if (source_data->vec4_words != 0) {
        vec4_copy = RwEngineInstance->allocate(0x10, 0x30000);
        if (vec4_copy != 0) {
            vec4_copy[0] = source_data->vec4_words[0];
            vec4_copy[1] = source_data->vec4_words[1];
            vec4_copy[2] = source_data->vec4_words[2];
            vec4_copy[3] = source_data->vec4_words[3];
            MK_MATERIAL_PLUGIN(destination)->vec4_words = vec4_copy;
        }
    }
    return destination;
}

static void* MkmaterialDataDestructor(void* object, int offset, int size) {
    if (MK_MATERIAL_PLUGIN(object)->extra != 0) {
        RwEngineInstance->free(MK_MATERIAL_PLUGIN(object)->extra,
                               RwEngineInstance);
        MK_MATERIAL_PLUGIN(object)->extra = 0;
    }
    if (MK_MATERIAL_PLUGIN(object)->vec4 != 0) {
        RwEngineInstance->free(MK_MATERIAL_PLUGIN(object)->vec4,
                               RwEngineInstance);
        MK_MATERIAL_PLUGIN(object)->vec4 = 0;
    }
    return object;
}

static void* MkmaterialGlobalDataDestructor(void* object, int offset, int size) {
    return object;
}

static void* MkmaterialGlobalDataConstructor(void* object, int offset, int size) {
    return object;
}

int RpAtomicMksobjPluginAttach(void) {
    MksobjGlobalOffset = RwEngineRegisterPlugin(
        4, 0x895302, MksobjGlobalDataConstructor, MksobjGlobalDataDestructor);
    if (MksobjGlobalOffset < 0) {
        return 0;
    }
    MksobjLocalOffset = RpAtomicRegisterPlugin(sizeof(MksobjPluginData), 0x895302,
                                               MksobjDataConstructor, MksobjDataDestructor,
                                               MksobjDataCopier);
    if (MksobjLocalOffset < 0) {
        return 0;
    }
    return MksobjLocalOffset -
               RpAtomicRegisterPluginStream(0x895302, MksobjDataReadStream,
                                            MksobjDataWriteStream,
                                            MksobjDataGetStreamSize) ==
           0;
}

static int MksobjDataGetStreamSize(const void* object, int offset, int size) {
    return 0x10;
}

static RwStream* MksobjDataWriteStream(RwStream* stream, int length, const void* object, int offset,
                                       int size) {
    int version = 3;
    int field_0C;
    if (stream == 0 || object == 0) {
        return 0;
    }
    RwStreamWriteInt32(stream, &version, 4);
    RwStreamWriteInt32(stream, &MK_ATOMIC_PLUGIN(object)->flags, 4);
    RwStreamWriteReal(stream, &MK_ATOMIC_PLUGIN(object)->field_04, 4);
    field_0C = MK_ATOMIC_PLUGIN(object)->field_0C;
    RwStreamWriteInt32(stream, &field_0C, 4);
    return stream;
}

static RwStream* MksobjDataReadStream(RwStream* stream, int length, void* object, int offset,
                                      int size) {
    int version;
    int stream_version;
    int field_0C;
    if (stream == 0 || object == 0) {
        return 0;
    }
    RwStreamReadInt32(stream, &version, 4);
    stream_version = (unsigned short)version;
    RwStreamReadInt32(stream, &MK_ATOMIC_PLUGIN(object)->flags, 4);
    if (stream_version > 1) {
        RwStreamReadReal(stream, &MK_ATOMIC_PLUGIN(object)->field_04, 4);
    }
    if (stream_version > 2) {
        RwStreamReadInt32(stream, &field_0C, 4);
        MK_ATOMIC_PLUGIN(object)->field_0C = field_0C;
    }
    if (length > 0x10) {
        RwStreamSkip(stream, length - 0x10);
    }
    return stream;
}

static void* MksobjDataCopier(void* destination, const void* source, int offset, int size) {
    MK_ATOMIC_PLUGIN(destination)->flags =
        MK_ATOMIC_PLUGIN(source)->flags;
    MK_ATOMIC_PLUGIN(destination)->field_04 =
        MK_ATOMIC_PLUGIN(source)->field_04;
    MK_ATOMIC_PLUGIN(destination)->field_0C =
        MK_ATOMIC_PLUGIN(source)->field_0C;
    return destination;
}

static void* MksobjDataDestructor(void* object, int offset, int size) {
    return object;
}

static void* MksobjDataConstructor(void* object, int offset, int size) {
    if (MksobjLocalOffset > 0) {
        MK_ATOMIC_PLUGIN(object)->flags = 0;
        MK_ATOMIC_PLUGIN(object)->field_04 = 0.0f;
        MK_ATOMIC_PLUGIN(object)->sobj = 0;
        MK_ATOMIC_PLUGIN(object)->field_0C = 0;
    }
    return object;
}

static void* MksobjGlobalDataDestructor(void* object, int offset, int size) {
    return object;
}

static void* MksobjGlobalDataConstructor(void* object, int offset, int size) {
    return object;
}

int RpClumpMkobjPluginAttach(void) {
    MkobjGlobalOffset = RwEngineRegisterPlugin(4, 0x895301, MkobjGlobalDataConstructor,
                                               MkobjGlobalDataDestructor);
    if (MkobjGlobalOffset < 0) {
        return 0;
    }
    MkobjLocalOffset = RpClumpRegisterPlugin(sizeof(MkobjPluginData), 0x895301,
                                             MkobjDataConstructor, MkobjDataDestructor,
                                             MkobjDataCopier);
    if (MkobjLocalOffset < 0) {
        return 0;
    }
    return MkobjLocalOffset -
               RpClumpRegisterPluginStream(
                   0x895301, MkobjDataReadStream, MkobjDataWriteStream,
                   MkobjDataGetStreamSize) ==
           0;
}

static RwStream* MkobjDataWriteStream(RwStream* stream, int length, const void* object, int offset,
                                      int size) {
    return stream;
}

static RwStream* MkobjDataReadStream(RwStream* stream, int length, void* object, int offset,
                                     int size) {
    if (length > 0) {
        RwStreamSkip(stream, length);
    }
    return stream;
}

static int MkobjDataGetStreamSize(const void* object, int offset, int size) {
    return 0;
}

static void* MkobjDataCopier(void* destination, const void* source, int offset, int size) {
    MK_CLUMP_PLUGIN(destination)->owner = MK_CLUMP_PLUGIN(source)->owner;
    return destination;
}

static void* MkobjDataDestructor(void* object, int offset, int size) {
    return object;
}

static void* MkobjDataConstructor(void* object, int offset, int size) {
    if (MkobjLocalOffset > 0) {
        MK_CLUMP_PLUGIN(object)->owner = 0;
    }
    return object;
}

static void* MkobjGlobalDataDestructor(void* object, int offset, int size) {
    return object;
}

static void* MkobjGlobalDataConstructor(void* object, int offset, int size) {
    return object;
}
