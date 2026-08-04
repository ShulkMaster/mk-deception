/*
 * GameCube skin/specularity pipeline.
 *
 * The plugin offset helpers are the only byte-addressed accesses in this TU:
 * RenderWare assigns those extension offsets at runtime. Everything behind an
 * extension is represented by a typed retail-layout view.
 */
#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "math/gxMath.h"
#include "runtime/mk_plugins.h"

typedef struct RxPipeline RxPipeline;

typedef struct GXColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} GXColor;

typedef struct GXLightObj {
    unsigned char data[0x40];
} GXLightObj;

typedef struct SpecularMaterialData {
    struct SpecLight* light;        /* +0x00 */
    RwFrame* frame;                 /* +0x04 */
    RwTexture* texture;             /* +0x08 */
    RwTexture* saved_texture;       /* +0x0C */
    unsigned char pad10[0x14];
    unsigned char tint[3];          /* +0x24 */
    unsigned char pad27;
    float gloss;                    /* +0x28 */
    unsigned char flags;            /* +0x2C */
    unsigned char pad2D[3];
} SpecularMaterialData;

typedef struct SpecularGeometryData {
    void* field_00;
    int material_index;             /* +0x04 */
} SpecularGeometryData;

typedef struct SpecLight {
    unsigned char object_type;
    unsigned char light_type;       /* +0x01 */
    unsigned char flags;            /* +0x02 */
    unsigned char private_flags;
    RwFrame* frame;                 /* +0x04 */
    unsigned char pad08[0x0C];
    float radius;                   /* +0x14 */
    float color[4];                 /* +0x18 */
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
    unsigned char pad00[0x18];
    unsigned short token;           /* +0x18 */
    unsigned short pad1A;
    unsigned int pad1C;
    unsigned int display_list_count; /* +0x20 */
    unsigned char pad24[8];
    SpecDisplayList lists[1];       /* +0x2C */
} SpecDisplayHeader;

typedef struct SpecResourceEntry {
    SpecDisplayHeader* display_header; /* +0x00 */
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
    float ambient_red;               /* +0x0C */
    float ambient_green;             /* +0x10 */
    float ambient_blue;              /* +0x14 */
    float ambient_alpha;             /* +0x18 */
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
void RwV3dTransformVector(Vec*, const Vec*, const RwMatrix*);
void SpecularMaterialCalcMatrix(void*);
RwTexture* RpMaterialGetAlphaPassTexture(RpMaterial*);
void RpMatFXMaterialGetUVTransformMatrices(
    RpMaterial*, RwMatrix**, RwMatrix**);

void _rwDlVtxFmtSetup(void*, SpecResourceEntry*);
void _rwDlTransformSetup(const RwMatrix*, int);
void _rwDlObjectRenderSetup(unsigned int, unsigned int, unsigned int, int);
void _rwDlTextureSet(RwTexture*, int);
void _rwDlRenderStateSetZCompLoc(int);
void _rpSkinLoadMatrix(const RwMatrix*, int, int);

void GXSetTevColor();
void GXSetNumTexGens();
void GXSetTexCoordGen2();
void GXSetTevOrder();
void GXSetTevSwapMode();
void GXSetTevSwapModeTable();
void GXSetNumTevStages();
void GXSetTevColorIn();
void GXSetTevColorOp();
void GXSetTevAlphaIn();
void GXSetTevAlphaOp();
void GXSetCullMode();
void GXSetVtxDesc();
void GXCallDisplayList();
void GXSetBlendMode();
void GXSetChanAmbColor();
void GXSetChanMatColor();
void GXLoadTexMtxImm();
void GXGetViewportv();
void GXSetViewport();
void GXInitLightAttn();
void GXInitLightPos();
void GXInitLightColor();
void GXLoadLightObjImm();

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

static inline unsigned char color_component(float value) {
    int component = (int)value;
    if (component < 0) {
        component = 0;
    } else if (component > 255) {
        component = 255;
    }
    return (unsigned char)component;
}

static inline GXColor scaled_light_color(
    const SpecLight* light,
    const unsigned char tint[3],
    float scale) {
    GXColor color;

    color.r = color_component(tint[0] * scale * light->color[0]);
    color.g = color_component(tint[1] * scale * light->color[1]);
    color.b = color_component(tint[2] * scale * light->color[2]);
    color.a = 0xFF;
    return color;
}

/* Soft ceiling: 48.21% - typed TEV setup is recovered; scheduling differs. */
void ProcessSpecularity(
    RpMaterial* material,
    int has_texture,
    int has_specularity,
    int has_specular_map) {
    SpecularMaterialData* specular;
    GXColor color;
    float scale;
    unsigned char tex_coord;
    unsigned char tev_stage;

    specular = specular_data(material);
    tev_stage = (has_specular_map == 0) + 1;
    if (has_specularity != 0) {
        tev_stage++;
    }
    tex_coord = has_texture != 0;

    scale = 2.0f * material->surface.specular;
    if (scale > 1.0f) {
        scale = 1.0f;
    }
    color = scaled_light_color(
        specular->light, specular->tint, scale);
    GXSetTevColor(3, &color);

    GXSetNumTexGens(tex_coord + 1);
    GXSetTexCoordGen2(tex_coord, 1, 1, 0x39, 0, 0x7D);
    specular->texture->filter_flags =
        (specular->texture->filter_flags & 0xFFFF00FF) | 0x1100;
    _rwDlTextureSet(specular->texture, specTexNum);
    GXSetTevOrder(tev_stage, tex_coord, specTexNum, 0xFF);
    GXSetTevSwapMode(tev_stage, 0, 0);
    GXSetNumTevStages(tev_stage + 1);
    GXSetTevColorIn(tev_stage, 0xF, 8, 6, 0);
    GXSetTevColorOp(tev_stage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(tev_stage, 7, 7, 7, 0);
    GXSetTevAlphaOp(tev_stage, 0, 0, 0, 1, 0);
}

/* Soft ceiling: 64.69% - narrow boolean/codegen ordering remains. */
void CleanupSpecularity(
    RpMaterial* material, int has_texture, int has_specularity) {
    SpecularMaterialData* specular = specular_data(material);

    GXSetNumTexGens(has_texture != 0);
    GXSetNumTevStages((has_specularity != 0) + 1);
    if (has_specularity != 0 && (specular->flags & 0x10) != 0) {
        GXSetTevSwapMode(2, 0, 0);
        GXSetTevSwapMode(3, 0, 0);
    }
}

/* Soft ceiling: 92.90% - matrix expression scheduling remains. */
void SetupAtomicSpecularity(RpAtomic* atomic) {
    RpGeometry* geometry = atomic->geometry;
    SpecularGeometryData* extension = specular_geometry(geometry);
    RwMatrix inverse;
    RwMatrix combined;
    float texture_matrix[2][4];

    if (extension->material_index == -1) {
        return;
    }

    SpecularMaterialCalcMatrix(geometry->matList.materials[0]);
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

/* Soft ceiling: final narrow-bool clrlwi/NV emit only. */
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
    RwMatrix* atomic_ltm;
    unsigned int bone;

    resource->display_header->token = _RwDlTokenCurrent;
    atomic_ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);
    _rwDlVtxFmtSetup(geometry_vertex_format(atomic->geometry), resource);

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

/* Soft ceiling: 84.42% - callback NV allocation remains. */
static RpAtomic* MKReflectionRenderCallback(
    RpAtomic* atomic, SpecResourceEntry* resource) {
    prepare_skin_render(atomic, resource);
    return atomic;
}

/* Soft ceiling: 93.28% - viewport/state scheduling remains. */
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
    z_scale = camera->near_plane * camera->z_scale * camera->far_plane /
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
    MksobjPluginData* atomic_data = mksobj_data(atomic);
    SpecSkinData* skin = RpSkinGeometryGetSkin(atomic->geometry);
    RwMatrix* transform = 0;
    unsigned int material_number;

    if (atomic_data->sobj != 0 && skin->num_used_bones > 1) {
        SpecMatrixPalette* palette =
            (SpecMatrixPalette*)atomic_data->sobj->matrices;
        unsigned int material_id =
            mkmaterial_data(mesh->material)->flags & 0xBFF;

        material_number = material_id / 10;
        if (material_number != 0) {
            material_number--;
        }
        if (palette != 0 && material_id >= 10 && material_number < 30 &&
            (palette->valid_bits & (1U << material_number)) != 0) {
            transform = &palette->matrix[material_number];
        }
    }

    if (transform != 0) {
        if (bLastMatUploadedRoot == 0 ||
            transform != RwFrameGetLTM((RwFrame*)atomic->object.parent)) {
            _rwDlTransformSetup(transform, 1);
        }
        bLastMatUploadedRoot = 1;
    } else if (bLastMatUploadedRoot != 0) {
        _rwDlTransformSetup(
            RwFrameGetLTM((RwFrame*)atomic->object.parent), 1);
        bLastMatUploadedRoot = 0;
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

/*
 * Soft ceiling: 10.67% - recovered retail pass order and reflection priority;
 * the three typed sort queues still produce substantially different codegen.
 */
static void SpecSkinProcessMaterialList(
    RpAtomic* atomic, SpecResourceEntry* resource) {
    SpecMesh* reflection_meshes[64];
    SpecMesh* alpha_meshes[64];
    unsigned int reflection_priority[64];
    SpecMeshHeader* mesh_header;
    SpecDisplayList* display_lists;
    SpecMesh* first_mesh;
    unsigned int reflection_count = 0;
    unsigned int alpha_count = 0;
    unsigned int mesh_index;

    bLastMatUploadedRoot = 1;
    mesh_header = resource->mesh_header;
    first_mesh = mesh_header->meshes;
    display_lists = &resource->display_header->lists[
        resource->display_header->display_list_count - 1];

    for (mesh_index = 0; mesh_index < mesh_header->num_meshes; mesh_index++) {
        SpecMesh* mesh = &first_mesh[mesh_index];
        SpecularMaterialData* specular = specular_data(mesh->material);
        unsigned int flags = mkmaterial_data(mesh->material)->flags;

        if ((specular->flags & 0x80) != 0) {
            continue;
        }

        if ((flags & 0x40000000) != 0) {
            unsigned int priority = (flags >> 16) & 0xFF;
            unsigned int insert = reflection_count;

            while (insert != 0 &&
                reflection_priority[insert - 1] > priority) {
                reflection_meshes[insert] = reflection_meshes[insert - 1];
                reflection_priority[insert] =
                    reflection_priority[insert - 1];
                insert--;
            }
            reflection_meshes[insert] = mesh;
            reflection_priority[insert] = priority;
            reflection_count++;
        } else if ((flags & 0xFFF) != 0) {
            alpha_meshes[alpha_count++] = mesh;
        } else {
            GXSetCullMode((specular->flags & 0x20) != 0 ? 0 : 1);
            draw_spec_mesh(
                atomic, first_mesh, mesh, display_lists, 0);
        }
    }

    for (mesh_index = 0; mesh_index < alpha_count; mesh_index++) {
        SpecMesh* mesh = alpha_meshes[mesh_index];
        SpecularMaterialData* specular = specular_data(mesh->material);

        GXSetCullMode((specular->flags & 0x20) != 0 ? 0 : 1);
        draw_spec_mesh(
            atomic, first_mesh, mesh, display_lists, 0);
    }
    for (mesh_index = reflection_count; mesh_index != 0; mesh_index--) {
        SpecMesh* mesh = reflection_meshes[mesh_index - 1];
        SpecularMaterialData* specular = specular_data(mesh->material);

        if ((specular->flags & 0x40) == 0) {
            GXSetCullMode(2);
            draw_spec_mesh(
                atomic, first_mesh, mesh, display_lists, 1);
        }
    }
    for (mesh_index = 0; mesh_index < reflection_count; mesh_index++) {
        SpecMesh* mesh = reflection_meshes[mesh_index];

        GXSetCullMode(1);
        draw_spec_mesh(
            atomic, first_mesh, mesh, display_lists, 1);
    }

    GXSetTevSwapMode(2, 0, 0);
    GXSetTevSwapMode(3, 0, 0);
    RwEngineInstance->render_state_set(0x14, 1);
    RwEngineInstance->render_state_set(0x14, 2);
}

static inline void apply_material_z_bias(float bias) {
    float offset;

    if (bias != 0.0f) {
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
    float texture_matrix[2][4];

    if (base_transform == 0) {
        GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
        return;
    }

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
}

static inline unsigned char float_color_component(float value) {
    return (unsigned char)(int)value;
}

static inline void setup_material_channels(RpMaterial* material) {
    GXColor ambient;
    GXColor diffuse;
    float scale;

    if (pAmbLight != 0) {
        scale = 255.0f * material->surface.ambient;
        ambient.r = float_color_component(pAmbLight->color[0] * scale);
        ambient.g = float_color_component(pAmbLight->color[1] * scale);
        ambient.b = float_color_component(pAmbLight->color[2] * scale);
        ambient.a = 0;
    } else {
        ambient.r = 0;
        ambient.g = 0;
        ambient.b = 0;
        ambient.a = 0xFF;
    }

    diffuse.r = float_color_component(
        material->color.red * material->surface.diffuse);
    diffuse.g = float_color_component(
        material->color.green * material->surface.diffuse);
    diffuse.b = float_color_component(
        material->color.blue * material->surface.diffuse);
    diffuse.a = 0;
    GXSetChanAmbColor(0, &ambient);
    GXSetChanMatColor(0, &diffuse);
}

static inline void setup_base_z_compare(RwTexture* texture) {
    void* raster_owner;
    unsigned int* raster_flags;

    if (texture != 0 && texture->raster != 0) {
        raster_owner = *(void**)texture->raster;
        raster_flags = (unsigned int*)rw_plugin_data(
            raster_owner, _RwGameCubeRasterExtOffset + 0x14);
        _rwDlRenderStateSetZCompLoc((*raster_flags & 1) ^ 1);
    }
}

/* Soft ceiling: retail TEV program recovered; local scheduling remains. */
static void GCSpecSkinMaterialNoSpecmap(SpecMesh* mesh) {
    RpMaterial* material = mesh->material;
    SpecularMaterialData* specular = specular_data(material);
    RwMatrix* base_transform;
    GXColor specular_color;
    float scale;

    GXSetBlendMode(0, 4, 5, 5);
    apply_material_z_bias(specular->gloss);

    _rwDlTextureSet(material->texture, 0);
    specular->texture->filter_flags =
        (specular->texture->filter_flags & 0xFFFF00FF) | 0x1100;
    _rwDlTextureSet(specular->texture, 1);
    setup_base_z_compare(material->texture);

    GXSetNumTexGens(2);
    RpMatFXMaterialGetUVTransformMatrices(
        material, &base_transform, 0);
    setup_uv_transform(base_transform);
    GXSetTexCoordGen2(1, 1, 1, 0x39, 0, 0x7D);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOrder(1, 1, 1, 0xFF);
    GXSetNumTevStages(2);

    setup_material_channels(material);
    scale = 2.0f * material->surface.specular;
    if (scale > 1.0f) {
        scale = 1.0f;
    }
    specular_color.r = float_color_component(
        specular->tint[0] *
        (scale * specular->light->color[0]));
    specular_color.g = float_color_component(
        specular->tint[1] *
        (scale * specular->light->color[1]));
    specular_color.b = float_color_component(
        specular->tint[2] *
        (scale * specular->light->color[2]));
    specular_color.a = 0xFF;
    GXSetTevColor(3, &specular_color);

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

/* Soft ceiling: retail alpha-pass TEV programs recovered; scheduling remains. */
static void GCSpecSkinMaterial(SpecMesh* mesh, int alpha_pass) {
    RpMaterial* material = mesh->material;
    SpecularMaterialData* specular = specular_data(material);
    RwTexture* alpha_texture;
    RwMatrix* base_transform;
    GXColor specular_color;
    float scale;

    apply_material_z_bias(specular->gloss);
    RpMatFXMaterialGetUVTransformMatrices(
        material, &base_transform, 0);
    setup_material_channels(material);

    alpha_texture = RpMaterialGetAlphaPassTexture(material);
    if (alpha_texture == 0) {
        return;
    }

    _rwDlTextureSet(material->texture, 0);
    _rwDlTextureSet(alpha_texture, 1);
    specular->texture->filter_flags =
        (specular->texture->filter_flags & 0xFFFF00FF) | 0x1100;
    _rwDlTextureSet(specular->texture, 2);
    setup_base_z_compare(material->texture);

    GXSetNumTexGens(2);
    setup_uv_transform(base_transform);
    GXSetTexCoordGen2(1, 1, 1, 0x39, 0, 0x7D);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOrder(1, 0, 1, 4);
    GXSetTevOrder(2, 1, 2, 0xFF);
    GXSetTevSwapModeTable(3, 0, 3, 2, 1);
    GXSetNumTevStages(3);

    scale = 2.0f * material->surface.specular;
    if (scale > 1.0f) {
        scale = 1.0f;
    }
    specular_color.r = float_color_component(
        specular->tint[0] *
        (scale * specular->light->color[0]));
    specular_color.g = float_color_component(
        specular->tint[1] *
        (scale * specular->light->color[1]));
    specular_color.b = float_color_component(
        specular->tint[2] *
        (scale * specular->light->color[2]));
    specular_color.a = 0xFF;
    GXSetTevColor(3, &specular_color);

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
    } else {
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
}

static inline int light_is_active(const SpecLight* light) {
    return (light->flags & 3) != 0;
}

static inline void find_spec_lights(SpecWorld* world) {
    RwLLLink* link;

    pDirLight1 = 0;
    pDirLight2 = 0;
    pPointLight1 = 0;
    pPointLight2 = 0;
    pAmbLight = 0;
    if (world == 0) {
        return;
    }

    for (link = world->directional_lights.next;
         link != &world->directional_lights;
         link = link->next) {
        SpecLight* light = light_from_link(link);

        if (!light_is_active(light)) {
            continue;
        }
        if (light->light_type == 1) {
            if (pDirLight1 == 0) {
                pDirLight1 = light;
            } else {
                pDirLight2 = light;
            }
        } else if (light->light_type == 2) {
            pAmbLight = light;
        }
    }

    for (link = world->point_lights.next;
         link != &world->point_lights;
         link = link->next) {
        SpecLight* light = light_from_link(link);

        if (!light_is_active(light) || light->light_type != 0x80) {
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

static inline float spec_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;
    unsigned int mantissa_exp;

    if (!(value > 0.0f)) {
        return 0.0f;
    }

    input.f = value;
    mantissa_exp =
        (unsigned int)GXMathSqrtTable[(input.u >> 10) & 0x3FFE] << 8;
    mantissa_exp |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    guess.u = mantissa_exp;
    return 0.5f * guess.f *
        (3.0f - (guess.f * guess.f) / value);
}

static inline float spec_inv_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } guess;
    float product;
    float correction;

    if (!(value > 0.0f)) {
        return 0.0f;
    }

    guess.f = value;
    guess.u = 0x5F375A00U - (guess.u >> 1);
    product = guess.f * (value * guess.f);
    correction = 3.0f - product;
    return 0.0625f * guess.f * correction *
        -(correction * (product * correction) - 12.0f);
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
    float color_scale;

    light_index = lighting->light_count;
    light_object = &_RwGCLightObjs[light_index];
    GXInitLightAttn(light_object, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    GXInitLightPos(
        light_object,
        1048576.0f * delta->x * inverse_distance,
        -1048576.0f * delta->y * inverse_distance,
        1048576.0f * delta->z * inverse_distance);
    color_scale = 255.0f * intensity;
    color.r = float_color_component(light->color[0] * color_scale);
    color.g = float_color_component(light->color[1] * color_scale);
    color.b = float_color_component(light->color[2] * color_scale);
    color.a = 0;
    GXInitLightColor(light_object, &color);
    GXLoadLightObjImm(light_object, 1U << light_index);
    lighting->light_mask |= 1U << light_index;
    lighting->light_count++;
}

static inline void upload_directional_light(
    SpecLight* light,
    SpecLightingData* lighting,
    float intensity) {
    RwMatrix* light_ltm;
    Vec source_direction;
    Vec direction;
    GXColor color;
    GXLightObj* light_object;
    int light_index;
    float color_scale;

    light_ltm = RwFrameGetLTM(light->frame);
    source_direction.x = light_ltm->at.x;
    source_direction.y = light_ltm->at.y;
    source_direction.z = light_ltm->at.z;
    RwV3dTransformVector(
        &direction, &source_direction, &_RwDlInvCamLTM);
    light_index = lighting->light_count;
    light_object = &_RwGCLightObjs[light_index];
    GXInitLightAttn(light_object, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    GXInitLightPos(
        light_object,
        1048576.0f * direction.x,
        -1048576.0f * direction.y,
        1048576.0f * direction.z);
    color_scale = 255.0f * intensity;
    color.r = float_color_component(light->color[0] * color_scale);
    color.g = float_color_component(light->color[1] * color_scale);
    color.b = float_color_component(light->color[2] * color_scale);
    color.a = 0;
    GXInitLightColor(light_object, &color);
    GXLoadLightObjImm(light_object, 1U << light_index);
    lighting->light_mask |= 1U << light_index;
    lighting->light_count++;
}

/* Soft ceiling: retail light selection, attenuation and uploads recovered. */
static RpAtomic* GCSpecSkinLighting(
    RpAtomic* atomic, SpecLightingData* lighting) {
    RpMaterial* specular_material;
    RwMatrix* atomic_ltm;
    RwMatrix* light_ltm;
    RwMatrix inverse_specular;
    RwMatrix combined;
    float texture_matrix[2][4];
    Vec point1_delta;
    Vec point2_delta;
    float point1_distance_sq;
    float point2_distance_sq;
    float point1_intensity;
    float point2_intensity;
    float strongest_point_intensity;
    float directional_intensity;

    lighting->light_mask = 0;
    lighting->light_count = 0;
    lighting->ambient_red = 0.0f;
    lighting->ambient_green = 0.0f;
    lighting->ambient_blue = 0.0f;
    lighting->ambient_alpha = 1.0f;

    RwMatrixInvert(
        &cachedInverseAtomicLTM,
        RwFrameGetLTM((RwFrame*)atomic->object.parent));
    specular_material = atomic->geometry->matList.materials[0];
    find_spec_lights(RwEngineInstance->world);

    if (pAmbLight != 0) {
        lighting->ambient_red = pAmbLight->color[0];
        lighting->ambient_green = pAmbLight->color[1];
        lighting->ambient_blue = pAmbLight->color[2];
        lighting->ambient_alpha = pAmbLight->color[3];
        lighting->has_ambient = 1;
    } else {
        lighting->has_ambient = 0;
    }

    strongest_point_intensity = 0.0f;
    if (pPointLight1 != 0) {
        atomic_ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);
        light_ltm = RwFrameGetLTM(pPointLight1->frame);
        point1_delta.x = light_ltm->pos.x - atomic_ltm->pos.x;
        point1_delta.y = light_ltm->pos.y - atomic_ltm->pos.y;
        point1_delta.z = light_ltm->pos.z - atomic_ltm->pos.z;
        point1_distance_sq =
            point1_delta.x * point1_delta.x +
            point1_delta.y * point1_delta.y +
            point1_delta.z * point1_delta.z;
        point1_intensity =
            1.0f - spec_sqrt(point1_distance_sq) / pPointLight1->radius;
        if (point1_intensity < 0.0f) {
            point1_intensity = 0.0f;
        }

        if (pPointLight2 != 0) {
            light_ltm = RwFrameGetLTM(pPointLight2->frame);
            point2_delta.x = light_ltm->pos.x - atomic_ltm->pos.x;
            point2_delta.y = light_ltm->pos.y - atomic_ltm->pos.y;
            point2_delta.z = light_ltm->pos.z - atomic_ltm->pos.z;
            point2_distance_sq =
                point2_delta.x * point2_delta.x +
                point2_delta.y * point2_delta.y +
                point2_delta.z * point2_delta.z;
            point2_intensity =
                1.0f - spec_sqrt(point2_distance_sq) / pPointLight2->radius;
            if (point2_intensity < 0.0f) {
                point2_intensity = 0.0f;
            }
            strongest_point_intensity =
                point1_intensity >= point2_intensity
                    ? point1_intensity : point2_intensity;
            upload_point_light(
                pPointLight2, lighting, &point2_delta,
                spec_inv_sqrt(point2_distance_sq), point2_intensity);
        } else {
            strongest_point_intensity = point1_intensity;
        }

        upload_point_light(
            pPointLight1, lighting, &point1_delta,
            spec_inv_sqrt(point1_distance_sq), point1_intensity);
    }

    directional_intensity = 1.0f - strongest_point_intensity;
    if (directional_intensity < 0.5f) {
        directional_intensity = 0.5f;
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
    return atomic;
}
