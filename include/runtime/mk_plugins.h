#ifndef MK_PLUGINS_H
#define MK_PLUGINS_H

#include "runtime/mk_obj.h"
#include "rw/gcspecular.h"

/*
 * Midway RW plugin userdata layouts (offsets relative to *LocalOffset /
 * ColorSetGeometryOffset on the host Rp* object). See docs/renderware_re.md.
 */

/* ColorSet geometry entry - stride 0x14 (one per mesh / material slot). */
typedef struct ColorSetEntry {
    unsigned int count;  /* +0x00 */
    void** ptr_array;    /* +0x04 */
    void*** arrays;      /* +0x08 - [color] -> void*[] freed per slot */
    int* int_arrays_c;   /* +0x0C - int[count], zeroed per slot */
    int* int_arrays_10;  /* +0x10 */
} ColorSetEntry;

/* ColorSet geometry plugin - 0xC bytes (id 0x1BA). */
typedef struct ColorSetPluginData {
    unsigned int count;     /* +0x00 */
    void** ptr4;            /* +0x04 */
    ColorSetEntry* entries; /* +0x08 */
} ColorSetPluginData;

/* Optional Mkmaterial heap block: flag + count + inline int[count]. */
typedef struct MkmaterialExtra {
    int field_00; /* +0x00 - stream flag bit */
    int count;    /* +0x04 */
    int* data;    /* +0x08 - points at inline ints after header */
} MkmaterialExtra;

/* Mkmaterial material plugin - 0x24 bytes (id 0x895303). */
typedef struct MkmaterialPluginData {
    unsigned int flags;        /* +0x00 - low 12 bits material id */
    float field_04;            /* +0x04 */
    union {
        unsigned char bytes_08[4]; /* +0x08 - default 0xFF... */
        unsigned int word_08;
    };
    float field_0C;            /* +0x0C - default 5.0 */
    float z_bias;              /* +0x10 */
    union {
        float* vec4;               /* +0x14 - optional 4xfloat heap */
        unsigned int* vec4_words;
    };
    int field_18;              /* +0x18 - zeroed; not streamed */
    MkmaterialExtra* extra;    /* +0x1C */
    unsigned int field_20;     /* +0x20 */
} MkmaterialPluginData;

/* Mkobj clump plugin - 4 bytes (id 0x895301). */
typedef struct MkobjPluginData {
    MkObj* owner; /* +0x00 - owning Midway object */
} MkobjPluginData;

/* MksobjPluginData (atomic, 0x10 B, id 0x895302) - defined in mk_obj.h */

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
