#include "game/gcspecskin.h"
#include "rw/rtquat.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwframe.h"

typedef union FloatBits {
    float value;
    unsigned int bits;
} FloatBits;

typedef struct RwEngineInstanceType {
    char pad[0x134];
    void* (*fpMalloc)(unsigned long size, unsigned long hint);
} RwEngineInstanceType;

typedef struct SpecularLight {
    char pad[0x4];
    void* frame;
} SpecularLight;

typedef struct RwSurfaceProperties {
    float ambient;
    float specular;
    float diffuse;
} RwSurfaceProperties;

typedef struct SpecularFlags {
    unsigned char unused_7 : 1;
    signed char reflective : 1;
    signed char flag_5 : 1;
    signed char flag_4 : 1;
    signed char flag_3 : 1;
    unsigned char unused_2_0 : 3;
} SpecularFlags;

typedef union SpecularTint {
    unsigned int value;
    unsigned char component[4];
} SpecularTint;

typedef struct SpecularMaterialExt {
    SpecularLight* light;
    void* frame;
    void* phong_texture;
    unsigned int saved_tex_c;
    RwSurfaceProperties saved_surface;
    int clip_value;
    float shininess;
    SpecularTint tint;
    float gloss;
    SpecularFlags flags;
    char pad_2D[3];
} SpecularMaterialExt;

typedef struct RpMaterial {
    char pad[0x8];
    void* pipeline;
    RwSurfaceProperties surface;
} RpMaterial;

typedef struct RpMaterialList {
    RpMaterial** materials;
    int count;
} RpMaterialList;

typedef struct RpGeometry {
    char pad[0x20];
    RpMaterialList material_list;
} RpGeometry;

typedef struct RpAtomic {
    char pad[0x18];
    RpGeometry* geometry;
    char pad2[0x50];
    void* pipeline;
} RpAtomic;

typedef struct RpClump {
    char pad[0x8];
    RwLinkList atomic_list;
} RpClump;

typedef struct MkMaterialExt {
    unsigned int flags;
    float shininess;
    unsigned int tint;
    int field_0xC;
    float gloss;
} MkMaterialExt;

typedef struct SpecularGeometryExt {
    int field_0x00;
    int material_index;
} SpecularGeometryExt;

typedef struct GxLightSlot {
    float field00;
    float field04;
    float field08;
    unsigned int field0C;
    float field10;
    float field14;
    float field18;
    float field1C;
    float field20;
    float field24;
    float field28;
    float field2C;
    float field30;
    float field34;
    float field38;
    float field3C;
} GxLightSlot;

typedef struct GxLightBlock {
    GxLightSlot primary[15];
    GxLightSlot secondary[15];
    int field_0x780;
} GxLightBlock;

typedef struct MkSObj {
    char pad[0x80];
    GxLightBlock* light_block;
} MkSObj;

typedef struct RpSkin RpSkin;

void material_restore_reflection_texture(void);
void material_cache_reflection_texture(void);
void material_set_reflection_texture(void* material, void* texture);
void* RpGeometryForAllMaterials(void* geometry, void* callback, void* data);
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, int effects);
void* get_specular_light(void);
void* create_default_specular_light(void);
void* get_bgnd_specular_light(void);
void* create_default_bgnd_specular_light(void);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
void* _rpMaterialListGetMaterial(RpMaterialList* material_list, int index);
void SpecularCreatePipelines(void);

extern int SpecularMaterialOffset;
extern int SpecularGeometryOffset;
extern int MkmaterialLocalOffset;
extern int MksobjLocalOffset;
extern RwCamera* Camera;
extern RwEngineInstanceType* RwEngineInstance;
extern void* PhongTextures[3];
extern float PhongCoefficients[3];
extern RwV3d Yaxis;
extern char loading_image[];

static const float kOne = 1.0f;
static const float kZero = 0.0f;
static const float kNegTwo = -2.0f;
static const float kThree = 3.0f;
static const float kInvSqrtCoeffA = 0.0625f;
static const float kInvSqrtCoeffB = 12.0f;
static const float kHalf = 0.5f;

RwMatrix SpecularMatrix;
void* ImagePixels = loading_image;
int lbl_80510AC4;
int MKSpecularInstances;

static RpMaterial* restore_specular_texture_material_callback(RpMaterial* material, void* data);
static RpMaterial* swap_specular_texture_material_callback(RpMaterial* material, void* texture);
static void* specskin_atomic_setup(void* atomic);
static void* MKSpecularOpen(void* instance, int offset, int size);
static void* MKSpecularClose(void* instance, int offset, int size);
void* specskin_material_setup(void* material, unsigned int is_player);

static inline SpecularMaterialExt* specular_material_ext(RpMaterial* material) {
    return (SpecularMaterialExt*)((char*)material + SpecularMaterialOffset);
}

static inline MkMaterialExt* mk_material_ext(RpMaterial* material) {
    return (MkMaterialExt*)((char*)material + MkmaterialLocalOffset);
}

static inline SpecularGeometryExt* specular_geometry_ext(
    RpGeometry* geometry) {
    return (SpecularGeometryExt*)((char*)geometry + SpecularGeometryOffset);
}

static inline RpAtomic* atomic_from_clump_link(RwLLLink* link) {
    return (RpAtomic*)((char*)link - 0x40);
}

static inline MkSObj* atomic_mksobj(RpAtomic* atomic) {
    return *(MkSObj**)((char*)atomic + MksobjLocalOffset + 8);
}

static inline RpMaterial* material_at_index(
    const RpMaterialList* list, int index) {
    return list->materials[index];
}

static inline float fast_inverse_sqrt(float length_squared) {
    FloatBits inverse;
    float product;
    float correction;
    float result;

    if (length_squared <= kZero) {
        result = kZero;
    } else {
        inverse.value = length_squared;
        inverse.bits = 0x5F375A00U - (inverse.bits >> 1);
        product = inverse.value * (length_squared * inverse.value);
        correction = kThree - product;
        result = kInvSqrtCoeffA * inverse.value * correction *
                 -(correction * (product * correction) - kInvSqrtCoeffB);
    }
    return result;
}

static RpMaterial* restore_specular_texture_material_callback(RpMaterial* material, void* data) {
    SpecularMaterialExt* spec;

    spec = specular_material_ext(material);
    if (spec->light != 0 && spec->saved_tex_c != 0) {
        material_restore_reflection_texture();
        material->surface = spec->saved_surface;
    }
    return material;
}

static RpMaterial* swap_specular_texture_material_callback(RpMaterial* material, void* texture) {
    SpecularMaterialExt* spec;
    void* reflection_texture;
    RwSurfaceProperties surface;

    reflection_texture = texture;
    spec = specular_material_ext(material);
    if (spec->light != 0) {
        material_cache_reflection_texture();
        material_set_reflection_texture(material, reflection_texture);
        surface = material->surface;
        spec->saved_surface = surface;
        surface.specular = kOne;
        material->surface = surface;
    }
    return material;
}

void* force_specular_texture_atomic_callback(void* atomic, void* texture) {
    RpAtomic* atom;
    void* reflection_texture;
    RpGeometry* geometry;
    int index;
    int material_count;

    atom = atomic;
    reflection_texture = texture;
    geometry = atom->geometry;
    material_count = geometry->material_list.count;
    index = 0;
    while (index < material_count) {
        if (specular_material_ext(material_at_index(
                &geometry->material_list, index))->light != 0) {
            material_set_reflection_texture(
                material_at_index(&geometry->material_list, index),
                reflection_texture);
        }
        index++;
    }
    return atomic;
}

RpAtomic* restore_specular_texture_atomic_callback(RpAtomic* atomic,
                                                   void* data) {
    RpGeometryForAllMaterials(atomic->geometry,
                              restore_specular_texture_material_callback,
                              data);
    return atomic;
}

RpAtomic* swap_specular_texture_atomic_callback(RpAtomic* atomic,
                                                void* texture) {
    RpGeometryForAllMaterials(atomic->geometry,
                              swap_specular_texture_material_callback,
                              texture);
    return atomic;
}

/* Soft ceiling: 90.03% -- FPR scheduling and matrix-copy addressing remain. */
void SpecularMaterialCalcMatrix(void* material) {
    SpecularMaterialExt* spec;
    RwMatrix* light_matrix;
    RwMatrix* frame_matrix;
    RwV3d reflected;
    RwMatrix matrix;
    float dot;
    float reflection_scale;
    float length_squared;
    float inverse_length;
    float cross_length_squared;
    float inverse_cross_length;
    float scaled_x;
    float scaled_y;
    float scaled_z;
    unsigned int* source;
    unsigned int* destination;
    int copy_count;

    spec = specular_material_ext(material);
    if (spec->light != 0 && spec->light->frame != 0) {
        light_matrix = RwFrameGetLTM(spec->light->frame);
        frame_matrix = RwFrameGetLTM(spec->frame);
        reflected = frame_matrix->at;

        dot = reflected.z * light_matrix->at.z +
              (reflected.x * light_matrix->at.x + reflected.y * light_matrix->at.y);
        if (dot < kZero) {
            reflection_scale = kNegTwo * dot;
            scaled_x = light_matrix->at.x * reflection_scale;
            scaled_y = light_matrix->at.y * reflection_scale;
            scaled_z = light_matrix->at.z * reflection_scale;
            reflected.x += scaled_x;
            reflected.y += scaled_y;
            reflected.z += scaled_z;
        }

        matrix.at.x = reflected.x + light_matrix->at.x;
        matrix.at.y = reflected.y + light_matrix->at.y;
        matrix.at.z = reflected.z + light_matrix->at.z;
        length_squared = matrix.at.z * matrix.at.z +
                         (matrix.at.x * matrix.at.x +
                          matrix.at.y * matrix.at.y);
        inverse_length = fast_inverse_sqrt(length_squared);
        scaled_x = matrix.at.x * inverse_length;
        scaled_y = matrix.at.y * inverse_length;
        scaled_z = matrix.at.z * inverse_length;
        matrix.at.x = scaled_x;
        matrix.at.y = scaled_y;
        matrix.at.z = scaled_z;

        matrix.right.x = Yaxis.y * scaled_z - Yaxis.z * scaled_y;
        matrix.right.y = Yaxis.z * scaled_x - Yaxis.x * scaled_z;
        matrix.right.z = Yaxis.x * scaled_y - Yaxis.y * scaled_x;
        cross_length_squared = matrix.right.z * matrix.right.z +
                               (matrix.right.x * matrix.right.x +
                                matrix.right.y * matrix.right.y);
        inverse_cross_length = fast_inverse_sqrt(cross_length_squared);
        matrix.right.x *= inverse_cross_length;
        matrix.right.y *= inverse_cross_length;
        matrix.right.z *= inverse_cross_length;

        matrix.up.x = matrix.at.y * matrix.right.z - matrix.at.z * matrix.right.y;
        matrix.up.y = matrix.at.z * matrix.right.x - matrix.at.x * matrix.right.z;
        matrix.up.z = matrix.at.x * matrix.right.y - matrix.at.y * matrix.right.x;
        RwMatrixUpdate(&matrix);
        source = (unsigned int*)&matrix;
        destination = (unsigned int*)&SpecularMatrix;
        for (copy_count = 0; copy_count < 8; copy_count++) {
            destination[0] = source[0];
            destination[1] = source[1];
            source += 2;
            destination += 2;
        }
    }
}

void specskin_initialize_clump(void* clump) {
    RpClumpForAllAtomics(clump, specskin_atomic_setup, 0);
}

/* Soft ceiling: 99.19% -- the list cursor occupies r4 instead of retail r5. */
void specskin_force_clipping_clump(void* clump, int value) {
    int clip_value;
    RpClump* clump_ptr;
    RwLLLink* link;
    RwLLLink* end;
    RwLLLink* next;
    RpGeometry* geometry;
    RpMaterialList* material_list;
    unsigned int material_count;
    unsigned int index;

    clip_value = value;
    clump_ptr = clump;
    end = &clump_ptr->atomic_list.link;
    link = end->next;
    while (link != end) {
        geometry = atomic_from_clump_link(link)->geometry;
        next = link->next;
        material_list = &geometry->material_list;
        material_count = material_list->count;
        index = 0;
        while (index < material_count) {
            specular_material_ext(
                _rpMaterialListGetMaterial(material_list, index))->clip_value =
                clip_value;
            index++;
        }
        link = next;
    }
}

static void* specskin_atomic_setup(void* atomic) {
    MkSObj* mksobj;
    RpGeometry* geometry;
    RpAtomic* atom;
    GxLightBlock* light_block;
    int count;
    GxLightSlot* slot0;
    GxLightSlot* slot1;
    unsigned int flags;

    mksobj = atomic_mksobj(atomic);
    geometry = ((RpAtomic*)atomic)->geometry;
    RpMatFXAtomicEnableEffects(atomic);
    atom = atomic;
    atom->pipeline = SpecSkinAtomicPipeline;
    RpGeometryForAllMaterials(geometry, specskin_material_setup, 0);
    if (mksobj != 0 && mksobj->light_block == 0) {
        light_block = RwEngineInstance->fpMalloc(0x790, 0x30000);
        mksobj->light_block = light_block;
        light_block->field_0x780 = 0;
        for (count = 0; count < 0xF; count++) {
            slot0 = &light_block->primary[count];
            slot1 = &light_block->secondary[count];
            slot0->field28 = kOne;
            slot0->field14 = kOne;
            slot0->field00 = kOne;
            slot0->field10 = kZero;
            slot0->field08 = kZero;
            slot0->field04 = kZero;
            slot0->field24 = kZero;
            slot0->field20 = kZero;
            slot0->field18 = kZero;
            slot0->field38 = kZero;
            slot0->field34 = kZero;
            slot0->field30 = kZero;
            flags = slot0->field0C;
            flags = (flags | 0x20000) | 3;
            slot0->field0C = flags;
            slot1->field28 = kOne;
            slot1->field14 = kOne;
            slot1->field00 = kOne;
            slot1->field10 = kZero;
            slot1->field08 = kZero;
            slot1->field04 = kZero;
            slot1->field24 = kZero;
            slot1->field20 = kZero;
            slot1->field18 = kZero;
            slot1->field38 = kZero;
            slot1->field34 = kZero;
            slot1->field30 = kZero;
            flags = slot1->field0C;
            flags = (flags | 0x20000) | 3;
            slot1->field0C = flags;
        }
    }
    return atom;
}

void* specskin_material_setup(void* material, unsigned int is_player) {
    int phong_index;
    RpMaterial* mat;
    SpecularMaterialExt* spec;
    void* light;
    int coeff_count;
    float threshold;
    float shininess;
    int use_player;
    float* coeff_pair;
    void* camera_frame;
    void* selected_texture;
    RwSurfaceProperties surface;

    phong_index = 0;
    mat = material;
    use_player = 1;
    if (is_player != 0) {
        use_player = 0;
    }
    if (use_player != 0) {
        RpMatFXMaterialSetEffects(material, 5);
        light = get_specular_light();
        if (light == 0) {
            light = create_default_specular_light();
        }
    } else {
        light = get_bgnd_specular_light();
        if (light == 0) {
            light = create_default_bgnd_specular_light();
        }
    }
    if (light == 0) {
        return 0;
    }
    shininess = specular_material_ext(mat)->shininess;
    if (shininess != kZero) {
        for (coeff_count = 0; coeff_count < 2; coeff_count++) {
            coeff_pair = &PhongCoefficients[coeff_count];
            threshold = kHalf * (coeff_pair[0] + coeff_pair[1]);
            if (!(shininess >= threshold)) {
                break;
            }
            phong_index++;
        }
    } else {
        surface = mat->surface;
        surface.specular = kZero;
        mat->surface = surface;
    }
    selected_texture = PhongTextures[phong_index];
    camera_frame = Camera->object.object.parent;
    spec = specular_material_ext(mat);
    if (mat->surface.specular > kOne) {
        mat->surface.specular = kOne;
    }
    if (mat->surface.ambient > kOne) {
        mat->surface.ambient = kOne;
    }
    if (mat->surface.diffuse > kOne) {
        mat->surface.diffuse = kOne;
    }
    spec->light = light;
    spec->frame = camera_frame;
    spec->phong_texture = selected_texture;
    if (spec->tint.component[0] < 0x40) {
        spec->tint.component[0] = 0x40;
    }
    if (spec->tint.component[1] < 0x40) {
        spec->tint.component[1] = 0x40;
    }
    if (spec->tint.component[2] < 0x40) {
        spec->tint.component[2] = 0x40;
    }
    mat->pipeline = SpecSkinMaterialPipeline;
    return material;
}

/* Soft ceiling: 91.71% -- flag extraction and adjacent store scheduling differ. */
void specular_condition_clump(void* clump) {
    RpClump* clump_ptr;
    RwLLLink* link;
    RpGeometry* geometry;
    RwLLLink* end;
    RwLLLink* next;
    void* skin;
    unsigned int material_count;
    unsigned int material_index;
    MkMaterialExt* mkmat;
    SpecularMaterialExt* spec;
    void* material;
    signed char reflective;
    signed char flag_5;
    signed char flag_4;
    signed char flag_3;
    unsigned int flags;
    float material_shininess;
    unsigned int material_tint;
    float material_gloss;

    clump_ptr = clump;
    link = clump_ptr->atomic_list.link.next;
    end = &clump_ptr->atomic_list.link;
    while (link != end) {
        geometry = atomic_from_clump_link(link)->geometry;
        next = link->next;
        skin = RpSkinGeometryGetSkin(geometry);
        material_count = geometry->material_list.count;
        material_index = 0;
        while (material_index < material_count) {
            material = material_at_index(
                &geometry->material_list, material_index);
            mkmat = mk_material_ext(material);
            flags = mkmat->flags;
            material_shininess = mkmat->shininess;
            material_tint = mkmat->tint;
            material_gloss = mkmat->gloss;
            spec = specular_material_ext(material);
            reflective = (signed char)(flags >> 31);
            flag_5 = (signed char)((flags >> 27) & 1);
            flag_4 = (signed char)((flags >> 29) & 1);
            flag_3 = (signed char)((flags >> 30) & 1);
            spec->shininess = material_shininess;
            spec->tint.value = material_tint;
            spec->gloss = material_gloss;
            spec->flags.reflective = reflective;
            spec->flags.flag_5 = flag_5;
            spec->flags.flag_4 = flag_4;
            spec->flags.flag_3 = flag_3;
            if (material_shininess > kZero) {
                specular_geometry_ext(geometry)->material_index =
                    material_index;
                if (skin == 0) {
                    specskin_material_setup(material, 1);
                }
            }
            material_index++;
        }
        link = next;
    }
}

int specskin_plugin_attach(void) {
    unsigned int result;

    /* Unsigned >> so MWCC emits srwi (retail), not srawi. */
    result = (unsigned int)RwEngineRegisterPlugin(0, 0xDC, MKSpecularOpen, MKSpecularClose);
    return (int)((result >> 31) ^ 1);
}

static void* MKSpecularOpen(void* instance, int offset, int size) {
    void* saved;
    int count;

    saved = instance;
    count = MKSpecularInstances;
    MKSpecularInstances = count + 1;
    if (count == 0) {
        SpecularCreatePipelines();
    }
    return saved;
}

static void* MKSpecularClose(void* instance, int offset, int size) {
    MKSpecularInstances = MKSpecularInstances - 1;
    return instance;
}
