/*
 * GameCube skin/specularity pipeline.
 *
 * The plugin offset helpers are the only byte-addressed accesses in this TU:
 * RenderWare assigns those extension offsets at runtime. Everything behind an
 * extension is represented by a typed retail-layout view.
 */
#include "rw/rpworld_types.h"
#include "rw/gamecube.h"
#include "rw/rtquat.h"
#include "math/gxMath.h"
#include "runtime/mk_plugins.h"

typedef struct RxPipeline RxPipeline;

typedef union SpecularMaterialFlags {
    unsigned char value;
    struct {
        signed char hidden : 1;
        signed char reflection_pass : 1;
        signed char cull_front : 1;
        signed char swap_mode : 1;
        unsigned char pad4 : 4;
    } bits;
} SpecularMaterialFlags;

typedef struct SpecularMaterialData {
    struct SpecLight* light;        /* +0x00 */
    RwFrame* frame;                 /* +0x04 */
    RwTexture* texture;             /* +0x08 */
    RwTexture* saved_texture;       /* +0x0C */
    unsigned char pad10[0x14];
    unsigned char tint[3];          /* +0x24 */
    unsigned char pad27;
    float gloss;                    /* +0x28 */
    SpecularMaterialFlags flags;    /* +0x2C */
    unsigned char pad2D[3];
} SpecularMaterialData;

typedef struct SpecularGeometryData {
    void* field_00;
    int material_index;             /* +0x04 */
} SpecularGeometryData;

typedef union SpecColor4 {
    struct {
        float red;
        float green;
        float blue;
        float alpha;
    } channel;
    float value[4];
} SpecColor4;

typedef struct SpecLight {
    unsigned char object_type;
    unsigned char light_type;       /* +0x01 */
    unsigned char flags;            /* +0x02 */
    unsigned char private_flags;
    RwFrame* frame;                 /* +0x04 */
    unsigned char pad08[0x0C];
    float radius;                   /* +0x14 */
    SpecColor4 color;               /* +0x18 */
    unsigned char pad28[0x0C];
    RwLLLink in_world;              /* +0x34 */
} SpecLight;

typedef struct SpecWorld {
    unsigned char pad00[0x34];
    RwLLLink point_lights;          /* +0x34 */
    RwLLLink directional_lights;    /* +0x3C */
} SpecWorld;

typedef struct SpecCamera {
    unsigned char pad00[0x20];
    RwMatrix view_matrix;           /* +0x20 */
    unsigned char pad60[0x20];
    float near_plane;               /* +0x80 */
    float far_plane;                /* +0x84 */
    float fog_plane;                /* +0x88 */
    float z_scale;                  /* +0x8C */
} SpecCamera;

typedef struct SpecRwEngine {
    SpecCamera* camera;              /* +0x00 */
    SpecWorld* world;                /* +0x04 */
    unsigned char pad08[0x18];
    int (*render_state_set)(int state, int value); /* +0x20 */
    int (*render_state_get)(int state, void* value); /* +0x24 */
} SpecRwEngine;

typedef struct SpecSkinData {
    unsigned int field_00;
    unsigned int num_bones;         /* +0x04 */
    unsigned char* bone_indices;    /* +0x08 */
    unsigned char pad0C[4];
    unsigned int num_used_bones;    /* +0x10 */
} SpecSkinData;

typedef struct SpecSkinGlobals {
    unsigned char pad00[0x0C];
    RwMatrix* matrix_cache;         /* +0x0C */
} SpecSkinGlobals;

typedef struct SpecMesh {
    unsigned short* indices;
    unsigned int num_indices;
    RpMaterial* material;
} SpecMesh;

typedef struct SpecMeshHeader {
    unsigned int flags;
    unsigned short num_meshes;
    unsigned short serial_num;
    unsigned int total_indices;
    unsigned int first_mesh_offset;
    SpecMesh meshes[1];             /* +0x10 */
} SpecMeshHeader;

typedef struct SpecDisplayList {
    void* data;
    unsigned int size;
} SpecDisplayList;

typedef struct SpecDisplayHeader {
    unsigned short token;            /* +0x00 */
    unsigned short pad02;
    unsigned int pad04;
    unsigned int display_list_count; /* +0x08 */
    unsigned char pad0C[8];
    SpecDisplayList lists[1];        /* +0x14 */
} SpecDisplayHeader;

typedef struct SpecDisplayResource {
    unsigned char pad00[0x18];
    SpecDisplayHeader header;        /* +0x18 */
} SpecDisplayResource;

typedef struct SpecResourceEntry {
    SpecDisplayResource* display_resource; /* +0x00 */
    SpecMeshHeader* mesh_header;       /* +0x04 */
    unsigned int object_setup_0;       /* +0x08 */
    unsigned char pad0C[0x10];
    unsigned int object_setup_1;       /* +0x1C */
    unsigned int object_setup_2;       /* +0x20 */
} SpecResourceEntry;

typedef struct SpecMatrixPalette {
    RwMatrix matrix[30];
    unsigned int valid_bits;         /* +0x780 */
} SpecMatrixPalette;

typedef struct SpecLightingData {
    unsigned char pad00[0x0C];
    SpecColor4 ambient;               /* +0x0C */
    int has_ambient;                 /* +0x1C */
    unsigned int light_mask;         /* +0x20 */
    int light_count;                 /* +0x24 */
} SpecLightingData;

typedef RpAtomic* (*RpSkinInstanceCallback)(RpAtomic*, SpecResourceEntry*);
typedef RpAtomic* (*RpSkinRenderCallback)(RpAtomic*, SpecResourceEntry*);
typedef RpAtomic* (*RpSkinLightingCallback)(RpAtomic*, SpecLightingData*);

static RpAtomic* MKReflectionRenderCallback(
    RpAtomic* atomic, SpecResourceEntry* resource);
static RpAtomic* MKSpecSkinRenderCallback(
    RpAtomic* atomic, SpecResourceEntry* resource);
static void SpecSkinProcessMaterialList(
    RpAtomic* atomic, SpecResourceEntry* resource);
static void GCSpecSkinMaterialNoSpecmap(SpecMesh* mesh);
static void GCSpecSkinMaterial(SpecMesh* mesh, int alpha_pass);
static RpAtomic* GCSpecSkinLighting(
    RpAtomic* atomic, SpecLightingData* lighting);

extern int SpecularGeometryOffset;
extern int _rpDlGeomVtxFmtOffset;
extern int _RwGameCubeRasterExtOffset;
extern unsigned short _RwDlTokenCurrent;
extern SpecRwEngine* RwEngineInstance;
extern SpecSkinGlobals _rpSkinGlobals;
extern GXLightObj _RwGCLightObjs[8];
extern RwMatrix SpecularMatrix;
extern RwMatrix _RwDlInvCamLTM;

RxPipeline* _rpDlAtomicPipelineCreate(
    unsigned int plugin_id,
    unsigned int plugin_data,
    RpSkinInstanceCallback instance_callback,
    RpSkinInstanceCallback reinstance_callback,
    RpSkinLightingCallback lighting_callback,
    RpSkinRenderCallback render_callback);
RpAtomic* _rpSkinInstanceCallback(RpAtomic*, SpecResourceEntry*);
RpAtomic* _rpSkinAtomicReinstanceCallBack(RpAtomic*, SpecResourceEntry*);
SpecSkinData* RpSkinGeometryGetSkin(RpGeometry*);
RwMatrix* RwFrameGetLTM(RwFrame*);
RwMatrix* RwMatrixInvert(RwMatrix*, const RwMatrix*);
RwMatrix* RwMatrixMultiply(RwMatrix*, const RwMatrix*, const RwMatrix*);
void RwV3dTransformVector(RwV3d*, const RwV3d*, const RwMatrix*);
void SpecularMaterialCalcMatrix(void*);
RwTexture* RpMaterialGetAlphaPassTexture(RpMaterial*);
void RpMatFXMaterialGetUVTransformMatrices(
    RpMaterial*, RwMatrix**, RwMatrix**);

void _rwDlTransformSetup(const RwMatrix*, int);
void _rwDlTextureSet(RwTexture*, int);
void _rwDlRenderStateSetZCompLoc(int);
void _rpSkinLoadMatrix(const RwMatrix*, int, int);

void GXGetViewportv();
RxPipeline* SpecSkinAtomicPipeline;
RxPipeline* SpecSkinMaterialPipeline;
RxPipeline* ReflectionAtomicPipeline;

RwMatrix cachedInverseAtomicLTM;
static float viewPort[6];
static int bLastMatUploadedRoot = 1;
static int specTexNum = 3;
static float base_z_buff;
static float z_near;
static float z_scale;
static float z_dist;
SpecLight* pDirLight1;
SpecLight* pDirLight2;
SpecLight* pPointLight1;
SpecLight* pPointLight2;
SpecLight* pAmbLight;
static float oldZFar;
static float oldZNear;
static float lastZOffset;

/*
 * RenderWare plugin offsets are assigned at runtime. Centralize the portable
 * byte-based lookup so material/geometry/atomic consumers remain typed.
 */
static inline void* rw_plugin_data(void* owner, int offset) {
    return (unsigned char*)owner + offset;
}

static inline SpecularMaterialData* specular_data(RpMaterial* material) {
    return (SpecularMaterialData*)rw_plugin_data(
        material, SpecularMaterialOffset);
}

static inline MkmaterialPluginData* mkmaterial_data(RpMaterial* material) {
    return (MkmaterialPluginData*)rw_plugin_data(
        material, MkmaterialLocalOffset);
}

static inline SpecularGeometryData* specular_geometry(RpGeometry* geometry) {
    return (SpecularGeometryData*)rw_plugin_data(
        geometry, SpecularGeometryOffset);
}

static inline void* geometry_vertex_format(RpGeometry* geometry) {
    return *(void**)rw_plugin_data(geometry, _rpDlGeomVtxFmtOffset);
}

static inline MksobjPluginData* mksobj_data(RpAtomic* atomic) {
    return (MksobjPluginData*)rw_plugin_data(atomic, MksobjLocalOffset);
}

static inline SpecLight* light_from_link(RwLLLink* link) {
    return (SpecLight*)((unsigned char*)link -
        (sizeof(SpecLight) - sizeof(RwLLLink)));
}

static inline signed char color_component(float value) {
    return (signed char)value;
}

static inline GXColor scaled_light_color(
    const SpecLight* light,
    const unsigned char tint[3],
    float scale) {
    GXColor color;

    color.r = color_component(
        tint[0] * (scale * light->color.value[0]));
    color.g = color_component(
        tint[1] * (scale * light->color.value[1]));
    color.b = color_component(
        tint[2] * (scale * light->color.value[2]));
    color.a = 0xFF;
    return color;
}

/* Soft ceiling: 87.11% -- color-conversion FPR and aggregate scheduling remain. */
void ProcessSpecularity(
    RpMaterial* material,
    int has_texture,
    unsigned int has_specularity,
    unsigned int has_specular_map) {
    SpecularMaterialData* specular;
    SpecLight* light;
    GXColor color;
    float scale;
    float material_scale;
    int initial_stage;
    int tex_coord;
    int tev_stage;

    specular = specular_data(material);
    initial_stage = (has_specular_map != 0) + 1;
    tev_stage = initial_stage;
    if (has_specularity != 0) {
        tev_stage = initial_stage + 1;
    }
    tex_coord = has_texture != 0;

    scale = 1.0f;
    material_scale = 2.0f * material->surface.specular;
    light = specular->light;
    scale = scale <= material_scale ? scale : material_scale;
    color.r = color_component(
        specular->tint[0] * (scale * light->color.value[0]));
    color.g = color_component(
        specular->tint[1] * (scale * light->color.value[1]));
    color.b = color_component(
        specular->tint[2] * (scale * light->color.value[2]));
    color.a = 0xFF;
    GXSetTevColor(3, color);

    GXSetNumTexGens((unsigned char)(tex_coord + 1));
    GXSetTexCoordGen2(tex_coord, 1, 1, 0x39, 0, 0x7D);
    {
        RwTexture* texture = specular->texture;
        texture->filter_flags =
            (texture->filter_flags & 0xFFFF00FF) | 0x1100;
        _rwDlTextureSet(texture, specTexNum);
    }
    GXSetTevOrder(tev_stage, tex_coord, specTexNum, 0xFF);
    GXSetTevSwapMode(tev_stage, 0, 0);
    GXSetNumTevStages(
        (unsigned char)((unsigned char)tev_stage + 1));
    GXSetTevColorIn(tev_stage, 0xF, 8, 6, 0);
    GXSetTevColorOp(tev_stage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(tev_stage, 7, 7, 7, 0);
    GXSetTevAlphaOp(tev_stage, 0, 0, 0, 1, 0);
}

/* Soft ceiling: 73.44% -- boolean scheduling changes only the frame and NV homes. */
void CleanupSpecularity(
    RpMaterial* material, int has_texture, unsigned int has_specularity) {
    SpecularMaterialData* specular;
    int textured;
    unsigned int stage_count;

    stage_count = (has_specularity != 0) + 1;
    textured = has_texture != 0;
    specular = specular_data(material);

    GXSetNumTexGens(textured);
    GXSetNumTevStages(stage_count);
    if (has_specularity != 0 && specular->flags.bits.swap_mode != 0) {
        GXSetTevSwapMode(2, 0, 0);
        GXSetTevSwapMode(3, 0, 0);
    }
}

void SetupAtomicSpecularity(RpAtomic* atomic) {
    RpGeometry* geometry = atomic->geometry;
    SpecularGeometryData* extension = specular_geometry(geometry);
    RwMatrix inverse;
    float texture_matrix[3][4];
    RwMatrix combined;

    if (extension->material_index == -1) {
        return;
    }

    SpecularMaterialCalcMatrix(geometry->matList.materials[0]);
    combined.flags = 0x20003;
    inverse.flags = 0x20003;
    RwMatrixInvert(&inverse, &SpecularMatrix);
    RwMatrixMultiply(
        &combined, RwFrameGetLTM((RwFrame*)atomic->object.parent), &inverse);

    texture_matrix[0][0] = -0.5f * -combined.right.x;
    texture_matrix[0][1] = -0.5f * -combined.up.x;
    texture_matrix[0][2] = -0.5f * -combined.at.x;
    texture_matrix[0][3] = -0.5f;
    texture_matrix[1][0] = -0.5f * combined.right.y;
    texture_matrix[1][1] = -0.5f * combined.up.y;
    texture_matrix[1][2] = -0.5f * combined.at.y;
    texture_matrix[1][3] = 0.5f;
    GXLoadTexMtxImm(texture_matrix, 0x39, 1);
}

/* Soft ceiling: 97.16% -- pipeline-result boolean scheduling remains. */
int SpecularCreatePipelines(void) {
    unsigned char created;

    SpecSkinAtomicPipeline = _rpDlAtomicPipelineCreate(
        0xDC, 0, _rpSkinInstanceCallback, _rpSkinAtomicReinstanceCallBack,
        GCSpecSkinLighting, MKSpecSkinRenderCallback);
    if (SpecSkinAtomicPipeline == 0) {
        return 0;
    }

    ReflectionAtomicPipeline = _rpDlAtomicPipelineCreate(
        0xDC, 0, _rpSkinInstanceCallback, _rpSkinAtomicReinstanceCallBack,
        GCSpecSkinLighting, MKReflectionRenderCallback);
    created = ReflectionAtomicPipeline != 0;
    return created;
}

void SetupShadowPlayerPipeline(RpGeometry* geometry) {
    (void)geometry;
}

static inline SpecSkinData* prepare_skin_render(
    RpAtomic* atomic, SpecResourceEntry* resource) {
    SpecSkinData* skin;
    void* vertex_format;
    RwMatrix* atomic_ltm;
    unsigned int bone;

    resource->display_resource->header.token = _RwDlTokenCurrent;
    vertex_format = geometry_vertex_format(atomic->geometry);
    atomic_ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);
    _rwDlVtxFmtSetup((RpGameCubeVtxFmt*)vertex_format,
                     (RpGameCubeVtxFmtSetupData*)resource);

    skin = RpSkinGeometryGetSkin(atomic->geometry);
    if (skin->num_used_bones > 1) {
        _rwDlTransformSetup(atomic_ltm, 1);
    } else {
        GXSetVtxDesc(0, 1);
    }

    _rwDlObjectRenderSetup(
        resource->object_setup_0,
        resource->object_setup_2,
        resource->object_setup_1,
        0);
    if (skin->num_used_bones == 1) {
        for (bone = 0; bone < skin->num_bones; bone++) {
            _rpSkinLoadMatrix(
                &_rpSkinGlobals.matrix_cache[skin->bone_indices[bone]],
                bone * 3,
                1);
        }
    }
    return skin;
}

/* Soft ceiling: 97.74% -- callback nonvolatile allocation remains. */
static RpAtomic* MKReflectionRenderCallback(
    RpAtomic* atomic, SpecResourceEntry* resource) {
    prepare_skin_render(atomic, resource);
    return atomic;
}

/* Soft ceiling: 98.65% -- viewport/state scheduling remains. */
static RpAtomic* MKSpecSkinRenderCallback(
    RpAtomic* atomic, SpecResourceEntry* resource) {
    SpecCamera* camera;
    RwMatrix object_to_camera;
    int old_src_blend;
    int old_dst_blend;

    RwEngineInstance->render_state_set(0x14, 1);
    RwEngineInstance->render_state_get(0x0A, &old_src_blend);
    RwEngineInstance->render_state_get(0x0B, &old_dst_blend);

    prepare_skin_render(atomic, resource);

    camera = RwEngineInstance->camera;
    RwMatrixMultiply(
        &object_to_camera,
        RwFrameGetLTM((RwFrame*)atomic->object.parent),
        &camera->view_matrix);
    z_dist = object_to_camera.pos.z;
    z_near = camera->near_plane;
    z_scale = camera->near_plane *
        (camera->z_scale * camera->far_plane) /
        (camera->far_plane - camera->near_plane);
    if (z_dist < z_near) {
        z_dist = z_near;
    }
    base_z_buff = z_scale / z_dist;

    GXGetViewportv(viewPort);
    oldZNear = viewPort[4];
    oldZFar = viewPort[5];
    lastZOffset = 0.0f;
    SpecSkinProcessMaterialList(atomic, resource);

    if (lastZOffset != 0.0f) {
        viewPort[4] = oldZNear;
        viewPort[5] = oldZFar;
        GXSetViewport(
            viewPort[0], viewPort[1], viewPort[2], viewPort[3],
            viewPort[4], viewPort[5]);
    }

    RwEngineInstance->render_state_set(0x0A, 2);
    RwEngineInstance->render_state_set(0x0B, 2);
    RwEngineInstance->render_state_set(0x0A, old_src_blend);
    RwEngineInstance->render_state_set(0x0B, old_dst_blend);
    return atomic;
}

static inline void upload_material_transform(
    RpAtomic* atomic, SpecMesh* mesh) {
    MkSobj* sobj = mksobj_data(atomic)->sobj;
    SpecSkinData* skin = RpSkinGeometryGetSkin(atomic->geometry);

    if (sobj != 0 && skin->num_used_bones > 1) {
        int has_transform = 0;
        SpecMatrixPalette* palette = (SpecMatrixPalette*)sobj->matrices;
        unsigned int material_id =
            mkmaterial_data(mesh->material)->flags & 0xBFF;
        unsigned int material_number = material_id / 10 - 1;

        if (palette != 0) {
            has_transform = (palette->valid_bits >> material_number) & 1;
        }
        if (has_transform != 0 || bLastMatUploadedRoot != 0) {
            RwMatrix* transform;
            if (has_transform != 0) {
                bLastMatUploadedRoot = 1;
                transform = &palette->matrix[material_number];
            } else {
                transform = RwFrameGetLTM((RwFrame*)atomic->object.parent);
                bLastMatUploadedRoot = 0;
            }
            _rwDlTransformSetup(transform, 1);
        }
    }
}

static inline void draw_spec_mesh(
    RpAtomic* atomic,
    SpecMesh* first_mesh,
    SpecMesh* mesh,
    SpecDisplayList* display_lists,
    int alpha_pass) {
    unsigned int display_index;

    upload_material_transform(atomic, mesh);
    if (RpMaterialGetAlphaPassTexture(mesh->material) != 0) {
        GCSpecSkinMaterial(mesh, alpha_pass);
    } else {
        GCSpecSkinMaterialNoSpecmap(mesh);
    }

    display_index = (unsigned int)(mesh - first_mesh);
    GXCallDisplayList(
        display_lists[display_index].data,
        display_lists[display_index].size);
}

static inline SpecMesh** spec_mesh_slot(
    SpecMesh** base, unsigned int byte_offset) {
    return (SpecMesh**)((unsigned char*)base + byte_offset);
}

static inline int* spec_priority_slot(
    int* base, unsigned int byte_offset) {
    return (int*)((unsigned char*)base + byte_offset);
}

/*
 * Soft ceiling: 94.21% -- retail pass/sort/loop CFG and accesses are aligned;
 * the residue is repeated inlined-helper register and load scheduling.
 */
static void SpecSkinProcessMaterialList(
    RpAtomic* atomic, SpecResourceEntry* resource) {
    SpecMesh* alpha_meshes[64];
    SpecMesh* reflection_meshes[64];
    int reflection_priority[64];
    SpecMeshHeader* mesh_header;
    SpecDisplayList* display_lists;
    SpecMesh* first_mesh;
    int reflection_count = 0;
    SpecMesh* mesh;
    unsigned int mesh_index;
    unsigned int num_meshes;
    int alpha_count = 0;
    unsigned int reflection_offset = 0;

    bLastMatUploadedRoot = 1;
    mesh_header = resource->mesh_header;
    first_mesh = mesh_header->meshes;
    num_meshes = mesh_header->num_meshes;
    display_lists = &resource->display_resource->header.lists[
        resource->display_resource->header.display_list_count - 1];

    for (mesh_index = 0, mesh = first_mesh;
         mesh_index < num_meshes;
         mesh_index++, mesh++) {
        SpecularMaterialData* specular = specular_data(mesh->material);
        unsigned int flags = mkmaterial_data(mesh->material)->flags;

        if (specular->flags.bits.hidden != 0) {
            continue;
        }

        if ((flags & 0x40000000) == 0x40000000) {
            int priority = (flags >> 16) & 0xFF;
            int insert = -1;
            int scan;

            for (scan = 0;
                 scan < reflection_count && insert < 0;
                 scan++) {
                if (reflection_priority[scan] > priority) {
                    insert = scan;
                }
            }
            if (insert >= 0) {
                for (scan = reflection_count; scan > insert; scan--) {
                    reflection_meshes[scan] = reflection_meshes[scan - 1];
                    reflection_priority[scan] =
                        reflection_priority[scan - 1];
                }
                reflection_meshes[insert] = mesh;
                reflection_priority[insert] = priority;
            } else {
                *spec_mesh_slot(reflection_meshes, reflection_offset) = mesh;
                *spec_priority_slot(reflection_priority, reflection_offset) =
                    priority;
            }
            reflection_count++;
            reflection_offset += sizeof(reflection_meshes[0]);
        } else if ((int)(flags & 0xFFF) > 0) {
            alpha_meshes[alpha_count++] = mesh;
        } else {
            if (specular->flags.bits.cull_front != 0) {
                GXSetCullMode(0);
            } else {
                GXSetCullMode(1);
            }
            draw_spec_mesh(
                atomic, first_mesh, mesh, display_lists, 0);
        }
    }

    for (mesh_index = 0; mesh_index < alpha_count; mesh_index++) {
        SpecMesh* mesh = alpha_meshes[mesh_index];
        SpecularMaterialData* specular = specular_data(mesh->material);

        if (specular->flags.bits.cull_front != 0) {
            GXSetCullMode(0);
        } else {
            GXSetCullMode(1);
        }
        draw_spec_mesh(
            atomic, first_mesh, mesh, display_lists, 0);
    }
    if (reflection_count > 0) {
        for (mesh_index = reflection_count - 1;
             (int)mesh_index >= 0;
             mesh_index--) {
            SpecMesh* mesh = reflection_meshes[mesh_index];
            SpecularMaterialData* specular = specular_data(mesh->material);

            if (specular->flags.bits.reflection_pass == 0) {
                GXSetCullMode(2);
                draw_spec_mesh(
                    atomic, first_mesh, mesh, display_lists, 1);
            }
        }
    }
    for (mesh_index = 0; mesh_index < reflection_count; mesh_index++) {
        SpecMesh* mesh = reflection_meshes[mesh_index];

        GXSetCullMode(1);
        draw_spec_mesh(
            atomic, first_mesh, mesh, display_lists, 1);
    }

    GXSetTevSwapMode(1, 0, 0);
    RwEngineInstance->render_state_set(0x14, 1);
    RwEngineInstance->render_state_set(0x14, 2);
}

static inline void apply_material_z_bias(float bias) {
    union {
        float value;
        unsigned int bits;
    } encoded_bias;
    float offset;

    encoded_bias.value = bias;
    if (encoded_bias.bits != 0) {
        offset = z_scale / (z_dist + bias) - base_z_buff;
        viewPort[4] = oldZNear + offset;
        viewPort[5] = oldZFar + offset;
        GXSetViewport(
            viewPort[0], viewPort[1], viewPort[2], viewPort[3],
            viewPort[4], viewPort[5]);
        lastZOffset = offset;
    } else if (lastZOffset != 0.0f) {
        viewPort[4] = oldZNear;
        viewPort[5] = oldZFar;
        GXSetViewport(
            viewPort[0], viewPort[1], viewPort[2], viewPort[3],
            viewPort[4], viewPort[5]);
        lastZOffset = 0.0f;
    }
}

static inline void setup_uv_transform(RwMatrix* base_transform) {
    float texture_matrix[3][4];

    if (base_transform != 0) {
        texture_matrix[0][0] = base_transform->right.x;
        texture_matrix[0][1] = base_transform->up.x;
        texture_matrix[0][2] = base_transform->at.x;
        texture_matrix[0][3] = base_transform->pos.x;
        texture_matrix[1][0] = base_transform->right.y;
        texture_matrix[1][1] = base_transform->up.y;
        texture_matrix[1][2] = base_transform->at.y;
        texture_matrix[1][3] = base_transform->pos.y;
        GXLoadTexMtxImm(texture_matrix, 0x21, 1);
        GXSetTexCoordGen2(0, 1, 4, 0x21, 0, 0x7D);
    } else {
        GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    }
}

static inline signed char float_color_component(float value) {
    return (signed char)(int)value;
}

static inline void setup_material_channels(
    RpMaterial* material, const GXColor* default_ambient) {
    GXColor ambient;
    GXColor diffuse;
    float scale;

    if (pAmbLight != 0) {
        scale = 255.0f * material->surface.ambient;
        ambient.r = float_color_component(pAmbLight->color.value[0] * scale);
        ambient.g = float_color_component(pAmbLight->color.value[1] * scale);
        ambient.b = float_color_component(pAmbLight->color.value[2] * scale);
        ambient.a = 0;
    } else {
        ambient = *default_ambient;
    }
    GXSetChanAmbColor(0, ambient);

    diffuse.r = float_color_component(
        material->color.red * material->surface.diffuse);
    diffuse.g = float_color_component(
        material->color.green * material->surface.diffuse);
    diffuse.b = float_color_component(
        material->color.blue * material->surface.diffuse);
    diffuse.a = 0;
    GXSetChanMatColor(0, diffuse);
}

static inline void setup_base_z_compare(RwTexture* texture) {
    void* raster_owner;

    if (texture != 0 && texture->raster != 0) {
        raster_owner = *(void**)texture->raster;
        _rwDlRenderStateSetZCompLoc(
            (*(unsigned int*)((char*)raster_owner +
                              _RwGameCubeRasterExtOffset + 0x14) & 1) ^ 1);
    }
}

/* Soft ceiling: 90.39% -- channel aggregates and specular-color scheduling remain. */
static void GCSpecSkinMaterialNoSpecmap(SpecMesh* mesh) {
    RpMaterial* material = mesh->material;
    SpecularMaterialData* specular = specular_data(material);
    RwTexture* base_texture;
    RwTexture* specular_texture;
    RwMatrix* base_transform;
    GXColor default_ambient = {0, 0, 0, 0xFF};
    GXColor specular_color;
    float scale;
    float material_scale;

    GXSetBlendMode(0, 4, 5, 5);
    apply_material_z_bias(specular->gloss);

    specular_texture = specular->texture;
    base_texture = mesh->material->texture;
    _rwDlTextureSet(base_texture, 0);
    specular_texture->filter_flags =
        (specular_texture->filter_flags & 0xFFFF00FF) | 0x1100;
    _rwDlTextureSet(specular_texture, 1);
    setup_base_z_compare(base_texture);

    GXSetNumTexGens(2);
    RpMatFXMaterialGetUVTransformMatrices(
        material, &base_transform, 0);
    setup_uv_transform(base_transform);
    GXSetTexCoordGen2(1, 1, 1, 0x39, 0, 0x7D);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOrder(1, 1, 1, 0xFF);
    GXSetNumTevStages(2);

    setup_material_channels(material, &default_ambient);
    material_scale = 2.0f * material->surface.specular;
    scale = 1.0f <= material_scale ? 1.0f : material_scale;
    specular_color.r = float_color_component(
        specular->tint[0] *
        (scale * specular->light->color.value[0]));
    specular_color.g = float_color_component(
        specular->tint[1] *
        (scale * specular->light->color.value[1]));
    specular_color.b = float_color_component(
        specular->tint[2] *
        (scale * specular->light->color.value[2]));
    specular_color.a = 0xFF;
    GXSetTevColor(3, specular_color);

    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    GXSetTevColorIn(0, 0xF, 8, 0xA, 0xF);
    GXSetTevAlphaIn(0, 7, 7, 7, 6);
    GXSetTevSwapMode(1, 0, 0);
    GXSetTevColorOp(1, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
    GXSetTevColorIn(1, 0xF, 6, 8, 0);
    GXSetTevAlphaIn(1, 7, 7, 7, 0);
}

/* Soft ceiling: 93.01% -- retail alpha-pass TEV programs are aligned; scheduling remains. */
static void GCSpecSkinMaterial(SpecMesh* mesh, int alpha_pass) {
    RpMaterial* material = mesh->material;
    SpecularMaterialData* specular = specular_data(material);
    RwTexture* base_texture;
    RwTexture* specular_texture;
    RwTexture* alpha_texture;
    RwMatrix* base_transform;
    GXColor default_ambient = {0, 0, 0, 0xFF};
    GXColor specular_color;
    float scale;
    float material_scale;

    apply_material_z_bias(specular->gloss);
    RpMatFXMaterialGetUVTransformMatrices(
        material, &base_transform, 0);
    setup_material_channels(material, &default_ambient);

    alpha_texture = RpMaterialGetAlphaPassTexture(material);
    if (alpha_texture == 0) {
        return;
    }

    specular_texture = specular->texture;
    base_texture = mesh->material->texture;
    _rwDlTextureSet(base_texture, 0);
    _rwDlTextureSet(alpha_texture, 1);
    specular_texture->filter_flags =
        (specular_texture->filter_flags & 0xFFFF00FF) | 0x1100;
    _rwDlTextureSet(specular_texture, 2);
    setup_base_z_compare(base_texture);

    GXSetNumTexGens(2);
    setup_uv_transform(base_transform);
    GXSetTexCoordGen2(1, 1, 1, 0x39, 0, 0x7D);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOrder(1, 0, 1, 4);
    GXSetTevOrder(2, 1, 2, 0xFF);
    GXSetTevSwapModeTable(3, 0, 3, 2, 1);
    GXSetNumTevStages(3);

    material_scale = 2.0f * material->surface.specular;
    scale = 1.0f <= material_scale ? 1.0f : material_scale;
    specular_color.r = float_color_component(
        specular->tint[0] *
        (scale * specular->light->color.value[0]));
    specular_color.g = float_color_component(
        specular->tint[1] *
        (scale * specular->light->color.value[1]));
    specular_color.b = float_color_component(
        specular->tint[2] *
        (scale * specular->light->color.value[2]));
    specular_color.a = 0xFF;
    GXSetTevColor(3, specular_color);

    if (alpha_pass != 0) {
        GXSetBlendMode(1, 4, 5, 5);
        GXSetTevColorOp(0, 0, 0, 0, 1, 0);
        GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
        GXSetTevColorIn(0, 0xF, 8, 0xA, 0xF);
        GXSetTevAlphaIn(0, 7, 7, 7, 7);
        GXSetTevSwapMode(1, 0, 3);
        GXSetTevColorOp(1, 0, 0, 0, 1, 0);
        GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
        GXSetTevColorIn(1, 0xF, 0xF, 0xF, 0);
        GXSetTevAlphaIn(1, 7, 7, 7, 4);
        GXSetTevColorOp(2, 0, 0, 0, 1, 0);
        GXSetTevAlphaOp(2, 0, 0, 0, 1, 0);
        GXSetTevColorIn(2, 0xF, 6, 8, 0);
        GXSetTevAlphaIn(2, 7, 7, 7, 0);
        return;
    }

    GXSetBlendMode(0, 4, 5, 5);
    GXSetTevColorIn(0, 0xF, 8, 0xA, 0xF);
    GXSetTevColorOp(0, 0, 0, 0, 1, 1);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 1);
    GXSetTevAlphaIn(0, 7, 7, 7, 7);
    GXSetTevSwapMode(1, 0, 3);
    GXSetTevColorIn(1, 0xF, 6, 9, 0xF);
    GXSetTevColorOp(1, 0, 0, 0, 1, 2);
    GXSetTevAlphaOp(1, 0, 0, 0, 1, 2);
    GXSetTevAlphaIn(1, 7, 7, 7, 4);
    GXSetTevColorOp(2, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(2, 0, 0, 0, 1, 0);
    GXSetTevColorIn(2, 0xF, 4, 8, 2);
    GXSetTevAlphaIn(2, 7, 7, 7, 6);
}

static inline void find_spec_lights(SpecRwEngine* engine) {
    SpecWorld* world;
    RwLLLink* link;

    pDirLight1 = 0;
    pDirLight2 = 0;
    pAmbLight = 0;
    world = engine->world;
    if (world != 0) {
        for (link = world->directional_lights.next;
             link != &world->directional_lights;
             link = link->next) {
            SpecLight* light = light_from_link(link);

            if (light == 0) {
                continue;
            }
            if ((light->flags & 1) == 0 &&
                (light->flags & 2) == 0) {
                continue;
            }
            switch (light->light_type) {
            case 1:
                if (pDirLight1 == 0) {
                    pDirLight1 = light;
                } else {
                    pDirLight2 = light;
                }
                break;
            case 2:
                pAmbLight = light;
                break;
            }
        }
    }

    pPointLight1 = 0;
    pPointLight2 = 0;
    world = engine->world;
    if (world != 0) {
        for (link = world->point_lights.next;
             link != &world->point_lights;
             link = link->next) {
            SpecLight* light = light_from_link(link);

            if (light == 0) {
                continue;
            }
            if ((light->flags & 1) == 0 &&
                (light->flags & 2) == 0) {
                continue;
            }
            switch (light->light_type) {
            case 0x80:
                break;
            default:
                continue;
            }
            if (pPointLight1 == 0) {
                pPointLight1 = light;
            } else {
                pPointLight2 = light;
                break;
            }
        }
    }
}

static inline float spec_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;
    float result;

    if (value <= 0.0f) {
        result = 0.0f;
    } else {
        input.f = value;
        guess.u =
            (unsigned int)GXMathSqrtTable[(input.u >> 11) & 0x1FFF] << 8;
        guess.u |=
            (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
        result = 0.5f * guess.f *
            (3.0f - (guess.f * guess.f) / value);
    }
    return result;
}

static inline float spec_inv_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } guess;
    float product;
    float correction;
    float result;

    if (value <= 0.0f) {
        result = 0.0f;
    } else {
        guess.f = value;
        guess.u = 0x5F375A00U - (guess.u >> 1);
        product = guess.f * (value * guess.f);
        correction = 3.0f - product;
        result = 0.0625f * guess.f * correction *
            -(correction * (product * correction) - 12.0f);
    }
    return result;
}

static inline void upload_point_light(
    SpecLight* light,
    SpecLightingData* lighting,
    const Vec* delta,
    float inverse_distance,
    float intensity) {
    GXColor color;
    GXLightObj* light_object;
    int light_index;
    int red;
    int green;
    int blue;
    float color_scale;

    light_index = lighting->light_count;
    light_object = &_RwGCLightObjs[light_index];
    GXInitLightAttn(light_object, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    GXInitLightPos(
        light_object,
        -1048576.0f * -(delta->x * inverse_distance),
        -1048576.0f * (delta->y * inverse_distance),
        -1048576.0f * -(delta->z * inverse_distance));
    color_scale = 255.0f * intensity;
    red = (int)(light->color.value[0] * color_scale);
    green = (int)(light->color.value[1] * color_scale);
    blue = (int)(light->color.value[2] * color_scale);
    color.r = (signed char)red;
    color.g = (signed char)green;
    color.b = (signed char)blue;
    color.a = 0;
    GXInitLightColor(light_object, color);
    GXLoadLightObjImm(light_object, 1U << light_index);
    lighting->light_mask |= 1U << lighting->light_count;
    lighting->light_count++;
}

static inline void upload_directional_light(
    SpecLight* light,
    SpecLightingData* lighting,
    float intensity) {
    RwMatrix* light_ltm;
    RwV3d direction;
    GXColor color;
    GXLightObj* light_object;
    int light_index;
    int red;
    int green;
    int blue;
    float color_scale;

    light_ltm = RwFrameGetLTM(light->frame);
    RwV3dTransformVector(
        &direction, &light_ltm->at, &_RwDlInvCamLTM);
    light_index = lighting->light_count;
    light_object = &_RwGCLightObjs[light_index];
    GXInitLightAttn(light_object, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    GXInitLightPos(
        light_object,
        -1048576.0f * -direction.x,
        -1048576.0f * direction.y,
        -1048576.0f * -direction.z);
    color_scale = 255.0f * intensity;
    red = (int)(light->color.value[0] * color_scale);
    green = (int)(light->color.value[1] * color_scale);
    blue = (int)(light->color.value[2] * color_scale);
    color.r = (signed char)red;
    color.g = (signed char)green;
    color.b = (signed char)blue;
    color.a = 0;
    GXInitLightColor(light_object, color);
    GXLoadLightObjImm(light_object, 1U << light_index);
    lighting->light_mask |= 1U << lighting->light_count;
    lighting->light_count++;
}

/* Soft ceiling: 91.50% -- repeated light-upload FPR/GPR scheduling remains. */
static RpAtomic* GCSpecSkinLighting(
    RpAtomic* atomic, SpecLightingData* lighting) {
    RpMaterial* specular_material;
    RwMatrix* atomic_ltm;
    RwMatrix* light_ltm;
    RwMatrix inverse_specular;
    float texture_matrix[3][4];
    RwMatrix combined;
    SpecLight* point1;
    SpecLight* point2;
    Vec point1_delta;
    Vec point2_delta;
    float point1_distance_sq;
    float point2_distance_sq;
    float point1_intensity;
    float point2_intensity;
    float strongest_point_intensity;
    float directional_intensity;

    lighting->light_mask = 0;
    lighting->ambient.channel.red = 0.0f;
    lighting->ambient.channel.green = 0.0f;
    lighting->ambient.channel.blue = 0.0f;
    lighting->ambient.channel.alpha = 1.0f;
    lighting->light_count = 0;

    RwMatrixInvert(
        &cachedInverseAtomicLTM,
        RwFrameGetLTM((RwFrame*)atomic->object.parent));
    specular_material = atomic->geometry->matList.materials[0];
    if (((unsigned int)atomic->geometry->flags | 0x20U) != 0) {
    find_spec_lights(RwEngineInstance);

    if (pAmbLight != 0) {
        lighting->has_ambient = 1;
        lighting->ambient = pAmbLight->color;
    } else {
        lighting->has_ambient = 0;
    }

    strongest_point_intensity = 0.0f;
    directional_intensity = 1.0f;
    point1 = pPointLight1;
    if (point1 != 0) {
        atomic_ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);
        light_ltm = RwFrameGetLTM(point1->frame);
        point1_delta.x = light_ltm->pos.x - atomic_ltm->pos.x;
        point1_delta.y = light_ltm->pos.y - atomic_ltm->pos.y;
        point1_delta.z = light_ltm->pos.z - atomic_ltm->pos.z;
        point1_distance_sq =
            point1_delta.x * point1_delta.x +
            point1_delta.y * point1_delta.y +
            point1_delta.z * point1_delta.z;
        point2 = pPointLight2;
        if (point2 != 0) {
            light_ltm = RwFrameGetLTM(point2->frame);
            point2_delta.x = light_ltm->pos.x - atomic_ltm->pos.x;
            point2_delta.y = light_ltm->pos.y - atomic_ltm->pos.y;
            point2_delta.z = light_ltm->pos.z - atomic_ltm->pos.z;
            point2_distance_sq =
                point2_delta.x * point2_delta.x +
                point2_delta.y * point2_delta.y +
                point2_delta.z * point2_delta.z;
            point1_intensity =
                1.0f - spec_sqrt(point1_distance_sq) /
                    point1->radius;
            if (point1_intensity < 0.0f) {
                point1_intensity = 0.0f;
            }
            point2_intensity =
                1.0f - spec_sqrt(point2_distance_sq) / point2->radius;
            if (point2_intensity < 0.0f) {
                point2_intensity = 0.0f;
            }
            strongest_point_intensity =
                point1_intensity >= point2_intensity
                    ? point1_intensity : point2_intensity;
            upload_point_light(
                point2, lighting, &point2_delta,
                spec_inv_sqrt(point2_distance_sq), point2_intensity);
        } else {
            point1_intensity =
                1.0f - spec_sqrt(point1_distance_sq) /
                    point1->radius;
            if (point1_intensity < 0.0f) {
                point1_intensity = 0.0f;
            }
            strongest_point_intensity = point1_intensity;
        }

        directional_intensity = 1.0f - strongest_point_intensity;
        if (directional_intensity < 0.5f) {
            directional_intensity = 0.5f;
        }

        upload_point_light(
            point1, lighting, &point1_delta,
            spec_inv_sqrt(point1_distance_sq), point1_intensity);
    }

    if (pDirLight1 != 0) {
        upload_directional_light(
            pDirLight1, lighting, directional_intensity);

        SpecularMaterialCalcMatrix(specular_material);
        inverse_specular.flags = 0x20003;
        combined.flags = 0x20003;
        RwMatrixInvert(&inverse_specular, &SpecularMatrix);
        RwMatrixMultiply(
            &combined,
            RwFrameGetLTM((RwFrame*)atomic->object.parent),
            &inverse_specular);
        texture_matrix[0][0] = -0.5f * -combined.right.x;
        texture_matrix[0][1] = -0.5f * -combined.up.x;
        texture_matrix[0][2] = -0.5f * -combined.at.x;
        texture_matrix[0][3] = -0.5f;
        texture_matrix[1][0] = -0.5f * combined.right.y;
        texture_matrix[1][1] = -0.5f * combined.up.y;
        texture_matrix[1][2] = -0.5f * combined.at.y;
        texture_matrix[1][3] = 0.5f;
        GXLoadTexMtxImm(texture_matrix, 0x39, 1);
    }
    if (pDirLight2 != 0) {
        upload_directional_light(
            pDirLight2, lighting, directional_intensity);
    }
    }
    return atomic;
}
