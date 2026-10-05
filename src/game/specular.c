#include "game/gcspecskin.h"
#include "game/specular.h"
#include "math/mk_math.h"
#include "platform/display.h"
#include "platform/gcdisplay.h"
#include "runtime/light.h"
#include "runtime/mk_plugins.h"
#include "rw/rplight.h"
#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwengine.h"
#include "rw/rwframe.h"
#include "rw/rpskin.h"
#include "rw/rpmatfx.h"

union FloatBits {
    float value;
    unsigned int bits;
};

struct SpecularFlags {
    unsigned char unused_7 : 1;
    signed char reflective : 1;
    signed char flag_5 : 1;
    signed char flag_4 : 1;
    signed char flag_3 : 1;
    unsigned char unused_2_0 : 3;
};

struct SpecularTint {
    unsigned char component[4];
};

struct SpecularMaterialExt {
    RpLight* light;
    void* frame;
    void* phong_texture;
    unsigned int saved_tex_c;
    RpSurfaceProperties saved_surface;
    int clip_value;
    float shininess;
    struct SpecularTint tint;
    float gloss;
    struct SpecularFlags flags;
    char pad_2D[3];
};

struct MkMaterialExt {
    unsigned int flags;
    float shininess;
    struct SpecularTint tint;
    int field_0xC;
    float gloss;
};

struct SpecularGeometryExt {
    int field_0x00;
    int material_index;
};

struct GxLightBlock {
    RwMatrix primary[15];
    RwMatrix secondary[15];
    int field_0x780;
};

void material_restore_reflection_texture(void);
void material_cache_reflection_texture(void);
void material_set_reflection_texture(void* material, void* texture);
int SpecularCreatePipelines(void);

extern void* PhongTextures[3];
extern float PhongCoefficients[3];

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
static RpAtomic* specskin_atomic_setup(RpAtomic* atomic, void* data);
static void* MKSpecularOpen(void* instance, int offset, int size);
static void* MKSpecularClose(void* instance, int offset, int size);

static inline struct SpecularMaterialExt* specular_material_ext(RpMaterial* material) {
    return (struct SpecularMaterialExt*)((char*)material + SpecularMaterialOffset);
}

static inline struct MkMaterialExt* mk_material_ext(RpMaterial* material) {
    return (struct MkMaterialExt*)((char*)material + MkmaterialLocalOffset);
}

static inline struct SpecularGeometryExt* specular_geometry_ext(
    RpGeometry* geometry) {
    return (struct SpecularGeometryExt*)((char*)geometry + SpecularGeometryOffset);
}

static inline RpAtomic* atomic_from_clump_link(RwLLLink* link) {
    return RW_CONTAINER_OF(link, RpAtomic, inClumpLink);
}

static inline MkSobj* atomic_mksobj(RpAtomic* atomic) {
    return MK_ATOMIC_PLUGIN(atomic)->sobj;
}

static inline RpMaterial* material_at_index(
    const RpMaterialList* list, int index) {
    return list->materials[index];
}

static inline float fast_inverse_sqrt(float length_squared) {
    union FloatBits inverse;
    float product;
    float correction;
    float result;

    if (length_squared <= 0.0f) {
        result = 0.0f;
    } else {
        inverse.value = length_squared;
        inverse.bits = 0x5F375A00U - (inverse.bits >> 1);
        product = inverse.value * (length_squared * inverse.value);
        correction = 3.0f - product;
        result = 0.0625f * inverse.value * correction *
                 -(correction * (product * correction) - 12.0f);
    }
    return result;
}

static inline void specular_normalize(RwV3d* vector) {
    float x = vector->x;
    float x_squared = x * x;
    float y_squared = vector->y * vector->y;
    float z_squared = vector->z * vector->z;
    float inverse_length =
        fast_inverse_sqrt(z_squared + (x_squared + y_squared));

    vector->x = x * inverse_length;
    vector->y *= inverse_length;
    vector->z *= inverse_length;
}

static RpMaterial* restore_specular_texture_material_callback(RpMaterial* material, void* data) {
    struct SpecularMaterialExt* spec;

    spec = specular_material_ext(material);
    if (spec->light != 0 && spec->saved_tex_c != 0) {
        material_restore_reflection_texture();
        material->surface = spec->saved_surface;
    }
    return material;
}

static RpMaterial* swap_specular_texture_material_callback(RpMaterial* material, void* texture) {
    struct SpecularMaterialExt* spec;
    void* reflection_texture;
    RpSurfaceProperties surface;

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

RpAtomic* force_specular_texture_atomic_callback(RpAtomic* atomic,
                                                 void* texture) {
    RpAtomic* atom;
    void* reflection_texture;
    RpGeometry* geometry;
    int index;
    int material_count;

    atom = atomic;
    reflection_texture = texture;
    geometry = atom->geometry;
    material_count = geometry->matList.numMaterials;
    index = 0;
    while (index < material_count) {
        if (specular_material_ext(material_at_index(
                &geometry->matList, index))->light != 0) {
            material_set_reflection_texture(
                material_at_index(&geometry->matList, index),
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

/* TODO: [near miss] 99.98%; the two inlined inverse-sqrt input slots (0x10/0x14) are swapped vs retail. */
void SpecularMaterialCalcMatrix(RpMaterial* material) {
    struct SpecularMaterialExt* spec;
    RwMatrix* light_matrix;
    RwMatrix* frame_matrix;
    RwV3d reflected;
    RwMatrix matrix;
    float dot;
    float reflection_scale;
    float cross_length_squared;
    float inverse_cross_length;
    float scaled_x;
    float scaled_y;
    float scaled_z;

    spec = specular_material_ext(material);
    if (spec->light != 0 && spec->light->object.object.parent != 0) {
        light_matrix = RwFrameGetLTM(spec->light->object.object.parent);
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
        specular_normalize(&matrix.at);

        matrix.right.x = Yaxis.y * matrix.at.z - Yaxis.z * matrix.at.y;
        matrix.right.y = Yaxis.z * matrix.at.x - Yaxis.x * matrix.at.z;
        matrix.right.z = Yaxis.x * matrix.at.y - Yaxis.y * matrix.at.x;
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
        SpecularMatrix = matrix;
    }
}

void specskin_initialize_clump(void* clump) {
    RpClumpForAllAtomics(clump, specskin_atomic_setup, 0);
}

void specskin_force_clipping_clump(void* clump, int value) {
    RpClump* clump_ptr = clump;
    RwLLLink* end = &clump_ptr->atomicList;
    RwLLLink* link;

    link = clump_ptr->atomicList.next;
    while (link != end) {
        RwLLLink* next;
        RpGeometry* geometry;
        RpMaterialList* material_list;
        unsigned int material_count;
        unsigned int index;

        geometry = atomic_from_clump_link(link)->geometry;
        next = link->next;
        material_list = &geometry->matList;
        material_count = material_list->numMaterials;
        index = 0;
        while (index < material_count) {
            specular_material_ext(
                _rpMaterialListGetMaterial(material_list, index))->clip_value = value;
            index++;
        }
        link = next;
    }
}

static RpAtomic* specskin_atomic_setup(RpAtomic* atomic, void* data) {
    MkSobj* mksobj;
    RpGeometry* geometry;
    RpAtomic* atom;
    struct GxLightBlock* light_block;
    int count;
    RwMatrix* slot0;
    RwMatrix* slot1;
    unsigned int flags;

    mksobj = atomic_mksobj(atomic);
    geometry = atomic->geometry;
    RpMatFXAtomicEnableEffects(atomic);
    atom = atomic;
    atom->pipeline = SpecSkinAtomicPipeline;
    RpGeometryForAllMaterials(geometry, specskin_material_setup, 0);
    if (mksobj != 0 && mksobj->matrices == 0) {
        light_block = RwEngineInstance->fpMalloc(0x790, 0x30000);
        mksobj->matrices = (RwMatrix*)light_block;
        light_block->field_0x780 = 0;
        for (count = 0; count < 0xF; count++) {
            slot0 = &light_block->primary[count];
            slot1 = &light_block->secondary[count];
            slot0->at.z = kOne;
            slot0->up.y = kOne;
            slot0->right.x = kOne;
            slot0->up.x = kZero;
            slot0->right.z = kZero;
            slot0->right.y = kZero;
            slot0->at.y = kZero;
            slot0->at.x = kZero;
            slot0->up.z = kZero;
            slot0->pos.z = kZero;
            slot0->pos.y = kZero;
            slot0->pos.x = kZero;
            flags = slot0->flags;
            flags = (flags | 0x20000) | 3;
            slot0->flags = flags;
            slot1->at.z = kOne;
            slot1->up.y = kOne;
            slot1->right.x = kOne;
            slot1->up.x = kZero;
            slot1->right.z = kZero;
            slot1->right.y = kZero;
            slot1->at.y = kZero;
            slot1->at.x = kZero;
            slot1->up.z = kZero;
            slot1->pos.z = kZero;
            slot1->pos.y = kZero;
            slot1->pos.x = kZero;
            flags = slot1->flags;
            flags = (flags | 0x20000) | 3;
            slot1->flags = flags;
        }
    }
    return atom;
}

RpMaterial* specskin_material_setup(RpMaterial* material,
                                    void* is_player) {
    int phong_index;
    RpMaterial* mat;
    struct SpecularMaterialExt* spec;
    RpLight* light;
    int coeff_count;
    float threshold;
    float shininess;
    int use_player;
    float* coeff_pair;
    void* camera_frame;
    void* selected_texture;
    RpSurfaceProperties surface;

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

/* TODO: [near miss] 93.88%; extension-offset/flags GPRs and final bit-extract schedule remain; source staging exhausted. */
void specular_condition_clump(void* clump) {
    RpClump* clump_ptr;
    RwLLLink* link;
    RpGeometry* geometry;
    RwLLLink* end;
    RwLLLink* next;
    RpSkin* skin;
    unsigned int material_count;
    unsigned int material_index;
    struct MkMaterialExt* mkmat;
    struct SpecularMaterialExt* spec;
    RpMaterial* material;
    unsigned int flags;
    float material_shininess;
    struct SpecularTint material_tint;
    float material_gloss;

    clump_ptr = clump;
    link = clump_ptr->atomicList.next;
    end = &clump_ptr->atomicList;
    while (link != end) {
        geometry = atomic_from_clump_link(link)->geometry;
        next = link->next;
        skin = RpSkinGeometryGetSkin(geometry);
        material_count = geometry->matList.numMaterials;
        material_index = 0;
        while (material_index < material_count) {
            material = material_at_index(
                &geometry->matList, material_index);
            mkmat = mk_material_ext(material);
            flags = mkmat->flags;
            material_shininess = mkmat->shininess;
            material_tint = mkmat->tint;
            material_gloss = mkmat->gloss;
            spec = specular_material_ext(material);
            spec->shininess = material_shininess;
            spec->tint = material_tint;
            spec->gloss = material_gloss;
            spec->flags.reflective = flags >> 31;
            spec->flags.flag_5 = (flags >> 27) & 1;
            spec->flags.flag_4 = (flags >> 29) & 1;
            spec->flags.flag_3 = (flags >> 30) & 1;
            if (material_shininess > kZero) {
                specular_geometry_ext(geometry)->material_index =
                    material_index;
                if (skin == 0) {
                    specskin_material_setup(material, (void*)1);
                }
            }
            material_index++;
        }
        link = next;
    }
}

int specskin_plugin_attach(void) {
    unsigned int result;

    result = RwEngineRegisterPlugin(0, 0xDC, MKSpecularOpen, MKSpecularClose);
    return (result >> 31) ^ 1;
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
