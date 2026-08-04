/*
 * Port readiness:
 *   Structs: MISSING
 *   Matching: 60.42% (.text)
 *   Linked: NO
 *   Status: SCAFFOLD
 *   Gaps: material, atomic-list, geometry-plugin, light, and callback payload casts remain
 */
#include "game/specular.h"

typedef struct RwMatrix {
    float right[4];
    float up[4];
    float at[4];
    float pos[4];
} RwMatrix;

typedef struct RwEngineInstanceType {
    char pad[0x134];
    void* (*fpMalloc)(unsigned long hint, unsigned long size);
} RwEngineInstanceType;

typedef struct SpecularLight {
    char pad[0x4];
    void* frame;
} SpecularLight;

typedef struct SpecularMaterialExt {
    SpecularLight* light;
    void* frame;
    void* phong_texture;
    int saved_tex_c;
    int saved_mat_c;
    int saved_mat_10;
    int saved_mat_14;
    int clip_value;
    float shininess;
    void* mk_texture;
    float gloss;
    unsigned char flags;
    char pad_2D[3];
} SpecularMaterialExt;

typedef struct RpMaterial {
    char pad[0x8];
    void* pipeline;
    float tex_coeff_c;
    float tex_coeff_10;
    float tex_coeff_14;
} RpMaterial;

typedef struct RpGeometry {
    char pad[0x20];
    void** material_list;
    int material_count;
} RpGeometry;

typedef struct RpAtomic {
    char pad[0x18];
    RpGeometry* geometry;
    char pad2[0xC];
    void* in_clump_link;
    void* next_atomic;
} RpAtomic;

typedef struct RpClump {
    char pad[0x8];
    void* atomic_list_end;
} RpClump;

typedef struct CameraType {
    char pad[0x4];
    void* frame;
} CameraType;

typedef struct MkMaterialExt {
    unsigned int flags;
    float shininess;
    void* texture;
    float gloss;
} MkMaterialExt;

typedef struct GxLightSlot {
    float field00;
    float field04;
    float field08;
    unsigned int field0C;
    float field10;
    float field14;
    float field18;
    float field20;
    float field24;
    float field28;
    float field30;
    float field34;
    float field38;
} GxLightSlot;

void material_restore_reflection_texture(void);
void material_cache_reflection_texture(void);
void material_set_reflection_texture(void* material, void* texture);
void* RpGeometryForAllMaterials(void* geometry, void* callback, void* data);
void* RpClumpForAllAtomics(void* clump, void* callback, void* data);
void* RpMatFXAtomicEnableEffects(void* atomic);
void RpMatFXMaterialSetEffects(void* material, int effects);
void* get_specular_light(void);
void* create_default_specular_light(void);
void* get_bgnd_specular_light(void);
void* create_default_bgnd_specular_light(void);
void* RwFrameGetLTM(void* frame);
void RwMatrixUpdate(RwMatrix* matrix);
void* RpSkinGeometryGetSkin(void* geometry);
void* _rpMaterialListGetMaterial(void* material_list, int index);
void SpecularCreatePipelines(void);
int RwEngineRegisterPlugin(int size, unsigned long plugin_id, void* open, void* close);

extern int SpecularMaterialOffset;
extern int SpecSkinAtomicPipeline;
extern int SpecSkinMaterialPipeline;
extern int SpecularGeometryOffset;
extern int MkmaterialLocalOffset;
extern int MksobjLocalOffset;
extern CameraType* Camera;
extern RwEngineInstanceType* RwEngineInstance;
extern void* PhongTextures[3];
extern float PhongCoefficients[3];
extern float Yaxis[3];
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

static void* restore_specular_texture_material_callback(void* material, void* data);
static void* swap_specular_texture_material_callback(void* material, void* texture);
static void* specskin_atomic_setup(void* atomic);
static void* MKSpecularOpen(void* instance, int offset, int size);
static void MKSpecularClose(void* instance, int offset, int size);

static void* restore_specular_texture_material_callback(void* material, void* data) {
    char* mat;
    char* spec;
    int val;

    mat = material;
    spec = mat + SpecularMaterialOffset;
    if (*(int*)spec == 0) {
        return material;
    }
    if (*(int*)(spec + 0xC) == 0) {
        return material;
    }
    material_restore_reflection_texture();
    val = *(int*)(spec + 0x10);
    *(int*)(mat + 0xC) = val;
    val = *(int*)(spec + 0x14);
    *(int*)(mat + 0x10) = val;
    val = *(int*)(spec + 0x18);
    *(int*)(mat + 0x14) = val;
    return material;
}

static void* swap_specular_texture_material_callback(void* material, void* texture) {
    char* mat;
    char* spec;
    char* tex;
    int mat_c;
    int mat_10;
    int mat_14;
    int stack_c;

    mat = material;
    tex = texture;
    spec = mat + SpecularMaterialOffset;
    if (*(int*)spec == 0) {
        return material;
    }
    material_cache_reflection_texture();
    material_set_reflection_texture(mat, tex);
    mat_c = *(int*)(mat + 0xC);
    mat_10 = *(int*)(mat + 0x10);
    mat_14 = *(int*)(mat + 0x14);
    stack_c = mat_10;
    *(int*)(spec + 0x10) = mat_c;
    *(int*)(spec + 0x14) = mat_10;
    *(int*)(spec + 0x18) = mat_14;
    *(float*)&stack_c = kOne;
    *(int*)(mat + 0xC) = mat_c;
    *(int*)(mat + 0x10) = stack_c;
    *(int*)(mat + 0x14) = mat_14;
    return material;
}

void force_specular_texture_atomic_callback(void* atomic, void* texture) {
    char* atom;
    char* geometry;
    void** materials;
    int material_count;
    int index;
    int offset;
    char* spec;

    atom = atomic;
    geometry = *(char**)(atom + 0x18);
    materials = *(void***)(geometry + 0x20);
    material_count = *(int*)(geometry + 0x24);
    index = 0;
    offset = 0;
    while (index < material_count) {
        spec = materials[offset / 4];
        spec = spec + SpecularMaterialOffset;
        if (*(int*)spec != 0) {
            material_set_reflection_texture(materials[offset / 4], texture);
        }
        index++;
        offset += 4;
    }
}

void restore_specular_texture_atomic_callback(void* atomic, void* data) {
    char* saved;

    saved = atomic;
    RpGeometryForAllMaterials(*(void**)(saved + 0x18), restore_specular_texture_material_callback,
                              data);
    atomic = saved;
}

void swap_specular_texture_atomic_callback(void* atomic, void* texture) {
    char* saved;

    saved = atomic;
    RpGeometryForAllMaterials(*(void**)(saved + 0x18), swap_specular_texture_material_callback,
                              texture);
    atomic = saved;
}

/* Soft ceiling: SpecularMaterialCalcMatrix + texture/clump helpers --
 * beyond AttachPlugins; defer full specular / gcspecskin Matching. */
void SpecularMaterialCalcMatrix(void* material) {
    SpecularMaterialExt* spec;
    RwMatrix* ltm_camera;
    RwMatrix* ltm_frame;
    float vec_x;
    float vec_y;
    float vec_z;
    float at_dot;
    float len_sq;
    float inv_len;
    float axis_x;
    float axis_y;
    float axis_z;
    float cross_len_sq;
    float inv_cross_len;
    RwMatrix matrix;
    int len_bits;
    int cross_bits;
    int copy_count;
    int* src;
    int* dst;

    spec = (SpecularMaterialExt*)((char*)material + SpecularMaterialOffset);
    if (spec->light == 0) {
        return;
    }
    if (spec->light->frame == 0) {
        return;
    }
    ltm_camera = RwFrameGetLTM(spec->light->frame);
    ltm_frame = RwFrameGetLTM(spec->frame);
    vec_x = ltm_frame->at[0];
    vec_y = ltm_frame->at[1];
    vec_z = ltm_frame->at[2];
    at_dot = vec_x * ltm_camera->at[0] + vec_y * ltm_camera->at[1] + vec_z * ltm_camera->at[2];
    if (at_dot < kZero) {
        inv_len = kNegTwo * at_dot;
        vec_x = ltm_camera->at[0] + vec_x * inv_len;
        vec_y = ltm_camera->at[1] + vec_y * inv_len;
        vec_z = ltm_camera->at[2] + vec_z * inv_len;
    }
    vec_x = vec_x + ltm_camera->at[0];
    vec_y = vec_y + ltm_camera->at[1];
    vec_z = vec_z + ltm_camera->at[2];
    len_sq = vec_x * vec_x + vec_y * vec_y + vec_z * vec_z;
    if (len_sq < kZero) {
        inv_len = kOne;
    } else {
        len_bits = 0x5F375A00 - ((*(int*)&len_sq) >> 1);
        inv_len = *(float*)&len_bits;
        inv_len = len_sq * inv_len;
        inv_len = kInvSqrtCoeffA * inv_len;
        inv_len = inv_len * inv_len;
        inv_len = kThree - len_sq * inv_len;
        inv_len = kInvSqrtCoeffA * inv_len;
        inv_len = inv_len * (*(float*)&len_bits);
        inv_len = inv_len * kInvSqrtCoeffB;
    }
    axis_x = vec_x * inv_len;
    axis_y = vec_y * inv_len;
    axis_z = vec_z * inv_len;
    matrix.at[0] = Yaxis[2] * (vec_y * inv_len) - Yaxis[1] * (vec_z * inv_len);
    matrix.at[1] = Yaxis[0] * (vec_z * inv_len) - Yaxis[2] * axis_x;
    matrix.at[2] = Yaxis[1] * axis_x - Yaxis[0] * (vec_y * inv_len);
    cross_len_sq = matrix.at[0] * matrix.at[0] + matrix.at[1] * matrix.at[1] +
                   matrix.at[2] * matrix.at[2];
    if (cross_len_sq < kZero) {
        inv_cross_len = kOne;
    } else {
        cross_bits = 0x5F375A00 - ((*(int*)&cross_len_sq) >> 1);
        inv_cross_len = *(float*)&cross_bits;
        inv_cross_len = cross_len_sq * inv_cross_len;
        inv_cross_len = kInvSqrtCoeffA * inv_cross_len;
        inv_cross_len = inv_cross_len * inv_cross_len;
        inv_cross_len = kThree - cross_len_sq * inv_cross_len;
        inv_cross_len = kInvSqrtCoeffA * inv_cross_len;
        inv_cross_len = inv_cross_len * (*(float*)&cross_bits);
        inv_cross_len = inv_cross_len * kInvSqrtCoeffB;
    }
    matrix.right[0] = matrix.at[0] * inv_cross_len;
    matrix.right[1] = matrix.at[1] * inv_cross_len;
    matrix.right[2] = matrix.at[2] * inv_cross_len;
    matrix.up[0] = axis_x;
    matrix.up[1] = axis_y;
    matrix.up[2] = axis_z;
    matrix.at[0] = axis_x;
    matrix.at[1] = axis_y;
    matrix.at[2] = axis_z;
    matrix.right[0] = matrix.up[1] * matrix.at[2] - matrix.up[2] * matrix.at[1];
    matrix.right[1] = matrix.up[2] * matrix.at[0] - matrix.up[0] * matrix.at[2];
    matrix.right[2] = matrix.up[0] * matrix.at[1] - matrix.up[1] * matrix.at[0];
    matrix.up[0] = matrix.at[1] * matrix.right[2] - matrix.at[2] * matrix.right[1];
    matrix.up[1] = matrix.at[2] * matrix.right[0] - matrix.at[0] * matrix.right[2];
    matrix.up[2] = matrix.at[0] * matrix.right[1] - matrix.at[1] * matrix.right[0];
    RwMatrixUpdate(&matrix);
    src = (int*)&matrix.right[1];
    dst = (int*)&SpecularMatrix.right[1];
    copy_count = 8;
    do {
        dst[1] = src[1];
        src += 2;
        dst[0] = src[-2];
        dst += 2;
        copy_count--;
    } while (copy_count != 0);
}

void specskin_initialize_clump(void* clump) {
    RpClumpForAllAtomics(clump, specskin_atomic_setup, 0);
}

void specskin_force_clipping_clump(void* clump, int value) {
    RpClump* clump_ptr;
    void* link;
    void* end;
    RpAtomic* atomic;
    RpGeometry* geometry;
    int material_count;
    int index;

    clump_ptr = clump;
    end = clump_ptr->atomic_list_end;
    link = *(void**)((char*)clump_ptr + 8);
    while (link != end) {
        atomic = (RpAtomic*)((char*)link - 0x28);
        geometry = atomic->geometry;
        material_count = geometry->material_count;
        index = 0;
        while (index < material_count) {
            *(int*)((char*)_rpMaterialListGetMaterial(geometry->material_list, index) +
                    SpecularMaterialOffset + 0x1C) = value;
            index++;
        }
        link = *(void**)link;
    }
}

static void* specskin_atomic_setup(void* atomic) {
    RpAtomic* atom;
    void* mksobj;
    RpGeometry* geometry;
    void* light_block;
    char* base;
    int offset;
    int count;
    GxLightSlot* slot0;
    GxLightSlot* slot1;
    unsigned int flags;

    atom = atomic;
    mksobj = *(void**)((char*)atom + MksobjLocalOffset + 8);
    geometry = atom->geometry;
    RpMatFXAtomicEnableEffects(atom);
    *(void**)((char*)atom + 0x6C) = (void*)SpecSkinAtomicPipeline;
    RpGeometryForAllMaterials(geometry, specskin_material_setup, 0);
    if (mksobj == 0) {
        return atomic;
    }
    if (*(void**)((char*)mksobj + 0x80) != 0) {
        return atomic;
    }
    light_block = RwEngineInstance->fpMalloc(0x790, 0x30000000);
    *(void**)((char*)mksobj + 0x80) = light_block;
    base = light_block;
    offset = 0;
    count = 0xF;
    do {
        slot0 = (GxLightSlot*)(base + offset);
        slot1 = (GxLightSlot*)(base + offset + 0x3C0);
        slot0->field00 = kOne;
        slot0->field14 = kOne;
        slot0->field28 = kOne;
        slot0->field04 = kZero;
        slot0->field08 = kZero;
        slot0->field10 = kZero;
        slot0->field18 = kZero;
        slot0->field20 = kZero;
        slot0->field24 = kZero;
        slot0->field30 = kZero;
        slot0->field34 = kZero;
        slot0->field38 = kZero;
        flags = slot0->field0C;
        flags = (flags | 0x20000) | 3;
        slot0->field0C = flags;
        slot1->field00 = kOne;
        slot1->field14 = kOne;
        slot1->field28 = kOne;
        slot1->field04 = kZero;
        slot1->field08 = kZero;
        slot1->field10 = kZero;
        slot1->field18 = kZero;
        slot1->field20 = kZero;
        slot1->field24 = kZero;
        slot1->field30 = kZero;
        slot1->field34 = kZero;
        slot1->field38 = kZero;
        flags = slot1->field0C;
        flags = (flags | 0x20000) | 3;
        slot1->field0C = flags;
        offset += 0x40;
        count--;
    } while (count != 0);
    return atomic;
}

void* specskin_material_setup(void* material, int is_player) {
    char* mat;
    char* spec;
    void* light;
    int phong_index;
    int coeff_byte;
    int coeff_count;
    float threshold;
    float shininess;
    int stack_slot;
    int use_player;
    float* coeff_pair;

    mat = material;
    phong_index = 0;
    use_player = 1;
    if (is_player == 0) {
        use_player = 1;
    } else {
        use_player = 0;
    }
    if (use_player == 0) {
        light = get_bgnd_specular_light();
        if (light == 0) {
            light = create_default_bgnd_specular_light();
        }
    } else {
        RpMatFXMaterialSetEffects(material, 5);
        light = get_specular_light();
        if (light == 0) {
            light = create_default_specular_light();
        }
    }
    if (light == 0) {
        return 0;
    }
    spec = mat + SpecularMaterialOffset;
    shininess = *(float*)(spec + 0x20);
    if (shininess != kZero) {
        coeff_byte = 0;
        coeff_count = 2;
        do {
            coeff_pair = (float*)((char*)PhongCoefficients + coeff_byte);
            threshold = kHalf * (coeff_pair[0] + coeff_pair[1]);
            if (shininess > threshold) {
                break;
            }
            phong_index++;
            coeff_byte += 4;
            coeff_count--;
        } while (coeff_count != 0);
    } else {
        stack_slot = *(int*)(mat + 0x10);
        *(float*)&stack_slot = kZero;
        *(int*)(mat + 0x10) = stack_slot;
    }
    if (*(float*)(mat + 0x10) > kOne) {
        *(float*)(mat + 0x10) = kOne;
    }
    if (*(float*)(mat + 0xC) > kOne) {
        *(float*)(mat + 0xC) = kOne;
    }
    if (*(float*)(mat + 0x14) > kOne) {
        *(float*)(mat + 0x14) = kOne;
    }
    *(void**)spec = light;
    *(void**)(spec + 4) = Camera->frame;
    *(void**)(spec + 8) = PhongTextures[phong_index];
    if (*(unsigned char*)(spec + 0x24) < 0x40) {
        *(unsigned char*)(spec + 0x24) = 0x40;
    }
    if (*(unsigned char*)(spec + 0x25) < 0x40) {
        *(unsigned char*)(spec + 0x25) = 0x40;
    }
    if (*(unsigned char*)(spec + 0x26) < 0x40) {
        *(unsigned char*)(spec + 0x26) = 0x40;
    }
    *(void**)(mat + 8) = (void*)SpecSkinMaterialPipeline;
    return material;
}

void specular_condition_clump(void* clump) {
    RpClump* clump_ptr;
    void* link;
    void* end;
    RpAtomic* atomic;
    RpGeometry* geometry;
    void* skin;
    void** materials;
    int material_count;
    int material_index;
    MkMaterialExt* mkmat;
    SpecularMaterialExt* spec;
    void* material;
    unsigned int flags;
    unsigned char packed;

    clump_ptr = clump;
    end = clump_ptr->atomic_list_end;
    link = *(void**)((char*)clump_ptr + 8);
    while (link != end) {
        atomic = (RpAtomic*)((char*)link - 0x28);
        geometry = atomic->geometry;
        skin = RpSkinGeometryGetSkin(geometry);
        materials = geometry->material_list;
        material_count = geometry->material_count;
        material_index = 0;
        while (material_index < material_count) {
            material = materials[material_index];
            mkmat = (MkMaterialExt*)((char*)material + MkmaterialLocalOffset);
            spec = (SpecularMaterialExt*)((char*)material + SpecularMaterialOffset);
            flags = mkmat->flags;
            spec->shininess = mkmat->shininess;
            spec->mk_texture = mkmat->texture;
            spec->gloss = mkmat->gloss;
            packed = spec->flags;
            packed = (unsigned char)((packed & 0xBF) | ((flags >> 31) & 1) << 6);
            packed = (unsigned char)((packed & 0xDF) | ((flags >> 4) & 1) << 5);
            packed = (unsigned char)((packed & 0xEF) | ((flags >> 2) & 1) << 4);
            packed = (unsigned char)((packed & 0xF7) | ((flags >> 1) & 1) << 3);
            spec->flags = packed;
            if (mkmat->shininess > kZero) {
                *(int*)((char*)geometry + SpecularGeometryOffset + 4) = material_index;
                if (skin == 0) {
                    specskin_material_setup(material, 1);
                }
            }
            material_index++;
        }
        link = *(void**)link;
    }
}

/*
 * AttachPlugins Midway gate only (B12). Readable body; do not Matching-grind
 * SpecularMaterialCalcMatrix / texture-swap callbacks / gcspecskin pipelines.
 */
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

/* Soft ceiling: MKSpecularClose ~97.5% -- retail lwz r4 vs our r3; stop. */
static void MKSpecularClose(void* instance, int offset, int size) {
    MKSpecularInstances = MKSpecularInstances - 1;
}
