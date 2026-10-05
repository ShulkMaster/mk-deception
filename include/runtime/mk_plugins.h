#ifndef MK_PLUGINS_H
#define MK_PLUGINS_H

#include "runtime/mk_obj.h"
#include "rw/gcspecular.h"

typedef struct ColorSetEntry {
    unsigned int count;
    void** ptr_array;
    void*** arrays;
    int* int_arrays_c;
    int* int_arrays_10;
} ColorSetEntry;

typedef struct ColorSetPluginData {
    unsigned int count;
    void** ptr4;
    ColorSetEntry* entries;
} ColorSetPluginData;

typedef struct MkmaterialExtra {
    int field_00;
    int count;
    int* data;
} MkmaterialExtra;

typedef struct MkmaterialUvScroll {
    float u1;
    float v1;
    float u2;
    float v2;
} MkmaterialUvScroll;

typedef struct MkmaterialPluginData {
    unsigned int flags;
    float field_04;
    union {
        unsigned char bytes_08[4];
        unsigned int word_08;
    };
    float field_0C;
    float z_bias;
    MkmaterialUvScroll* vec4;
    int field_18;
    MkmaterialExtra* extra;
    unsigned int field_20;
} MkmaterialPluginData;

typedef struct MkobjPluginData {
    MkObj* owner;
} MkobjPluginData;

int RpColorSetPluginAttach(void);
int RpMaterialMkmaterialPluginAttach(void);
int RpAtomicMksobjPluginAttach(void);
int RpClumpMkobjPluginAttach(void);

extern int MkobjGlobalOffset;
extern int MkobjLocalOffset;
extern int MksobjGlobalOffset;
extern int MksobjLocalOffset;
extern int MkmaterialGlobalOffset;
extern int MkmaterialLocalOffset;
extern int ColorSetGeometryOffset;
#define MK_MATERIAL_PLUGIN(material)                                      \
    ((MkmaterialPluginData*)((unsigned char*)(material) +                 \
                             MkmaterialLocalOffset))
#define MK_ATOMIC_PLUGIN(atomic)                                         \
    ((MksobjPluginData*)((unsigned char*)(atomic) + MksobjLocalOffset))
#define MK_CLUMP_PLUGIN(clump)                                           \
    ((MkobjPluginData*)((unsigned char*)(clump) + MkobjLocalOffset))

static inline ColorSetPluginData* COLOR_SET_PLUGIN(const void* geometry) {
    return (ColorSetPluginData*)((const unsigned char*)geometry +
                                 ColorSetGeometryOffset);
}

static inline SpecularMaterialPluginData* mk_get_specular_material_plugin(
    RpMaterial* material) {
    return (SpecularMaterialPluginData*)(
        (unsigned char*)material + SpecularMaterialOffset);
}

#endif
