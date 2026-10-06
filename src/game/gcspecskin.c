#include "game/specular.h"
#include "dolphin/gx.h"
#include "rw/alphapass.h"
#include "rw/dltextur.h"
#include "gameware/dltoken.h"
#include "rw/gamecube_globals.h"
#include "rw/gcspecular.h"
#include "rw/rpworld_types.h"
#include "rw/rpmatfx.h"
#include "rw/rpskin.h"
#include "rw/rtquat.h"
#include "rw/rwframe.h"
#include "rw/gamecube_texture.h"
#include "rw/rwvector.h"
#include "math/gxMath.h"
#include "game/gcspecskin.h"
#include "runtime/mk_plugins.h"

struct SpecularGeometryData {
    void* field_00;
    int material_index;
};

struct SpecColor4 {
    float red;
    float green;
    float blue;
    float alpha;
};

struct SpecLight {
    unsigned char object_type;
    unsigned char light_type;
    unsigned char flags;
    unsigned char private_flags;
    RwFrame* frame;
    unsigned char pad08[0x0C];
    float radius;
    struct SpecColor4 color;
    unsigned char pad28[0x0C];
    RwLLLink in_world;
};

struct SpecWorld {
    unsigned char pad00[0x34];
    RwLLLink point_lights;
    RwLLLink directional_lights;
};

struct SpecCamera {
    unsigned char pad00[0x20];
    RwMatrix view_matrix;
    unsigned char pad60[0x20];
    float near_plane;
    float far_plane;
    float fog_plane;
    float z_scale;
};

struct SpecMesh {
    unsigned short* indices;
    unsigned int num_indices;
    RpMaterial* material;
};

struct SpecMeshHeader {
    unsigned int flags;
    unsigned short num_meshes;
    unsigned short serial_num;
    unsigned int total_indices;
    unsigned int first_mesh_offset;
    struct SpecMesh meshes[1];
};

struct SpecDisplayList {
    void* data;
    unsigned int size;
};

struct SpecDisplayHeader {
    unsigned short token;
    unsigned short pad02;
    unsigned int pad04;
    unsigned int display_list_count;
    unsigned char pad0C[8];
    struct SpecDisplayList lists[1];
};

struct SpecDisplayResource {
    unsigned char pad00[0x18];
    struct SpecDisplayHeader header;
};

struct SpecResourceEntry {
    struct SpecDisplayResource* display_resource;
    struct SpecMeshHeader* mesh_header;
    unsigned int object_setup_0;
    unsigned char pad0C[0x10];
    unsigned int object_setup_1;
    unsigned int object_setup_2;
};

struct SpecMatrixPalette {
    RwMatrix matrix[30];
    unsigned int valid_bits;
};

struct SpecLightingData {
    unsigned char pad00[0x0C];
    struct SpecColor4 ambient;
    int has_ambient;
    unsigned int light_mask;
    int light_count;
};

typedef void* (*RpSkinInstanceCallback)(void*, RwResEntry**);
typedef RpAtomic* (*RpSkinRenderCallback)(RpAtomic*, struct SpecResourceEntry*);
typedef RpAtomic* (*RpSkinLightingCallback)(RpAtomic*, struct SpecLightingData*);

static RpAtomic* MKReflectionRenderCallback(
    RpAtomic* atomic, struct SpecResourceEntry* resource);
static RpAtomic* MKSpecSkinRenderCallback(
    RpAtomic* atomic, struct SpecResourceEntry* resource);
static void SpecSkinProcessMaterialList(
    RpAtomic* atomic, struct SpecResourceEntry* resource);
static void GCSpecSkinMaterialNoSpecmap(struct SpecMesh* mesh);
static void GCSpecSkinMaterial(struct SpecMesh* mesh, int alpha_pass);
static RpAtomic* GCSpecSkinLighting(
    RpAtomic* atomic, struct SpecLightingData* lighting);

extern GXLightObj _RwGCLightObjs[8];

RxPipeline* _rpDlAtomicPipelineCreate(
    unsigned int plugin_id,
    unsigned int plugin_data,
    RpSkinInstanceCallback instance_callback,
    RpSkinInstanceCallback reinstance_callback,
    RpSkinLightingCallback lighting_callback,
    RpSkinRenderCallback render_callback);
void _rwDlVtxFmtSetup(void*, struct SpecResourceEntry*);
void _rwDlTransformSetup(const RwMatrix*, int);
void _rwDlObjectRenderSetup(unsigned int, unsigned int, unsigned int, int);
void _rwDlRenderStateSetZCompLoc(int);
void _rpSkinLoadMatrix(const RwMatrix*, int, int);

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
struct SpecLight* pDirLight1;
struct SpecLight* pDirLight2;
struct SpecLight* pPointLight1;
struct SpecLight* pPointLight2;
struct SpecLight* pAmbLight;
static float oldZFar;
static float oldZNear;
static float lastZOffset;

static inline void* rw_plugin_data(void* owner, int offset) {
    return (unsigned char*)owner + offset;
}

static inline SpecularMaterialPluginData* specular_data(RpMaterial* material) {
    return rw_plugin_data(
        material, SpecularMaterialOffset);
}

static inline MkmaterialPluginData* mkmaterial_data(RpMaterial* material) {
    return rw_plugin_data(
        material, MkmaterialLocalOffset);
}

static inline struct SpecularGeometryData* specular_geometry(RpGeometry* geometry) {
    return rw_plugin_data(
        geometry, SpecularGeometryOffset);
}

static inline void* geometry_vertex_format(RpGeometry* geometry) {
    return *(void**)rw_plugin_data(geometry, _rpDlGeomVtxFmtOffset);
}

static inline MksobjPluginData* mksobj_data(RpAtomic* atomic) {
    return rw_plugin_data(atomic, MksobjLocalOffset);
}

static inline struct SpecLight* light_from_link(RwLLLink* link) {
    return RW_CONTAINER_OF(link, struct SpecLight, in_world);
}

static inline int color_component(float value) {
    return value;
}

/* TODO: [near miss] 87.60%; TEV/coordinate homes agree; RGB load/conversion interleave remains. */
void ProcessSpecularity(
    RpMaterial* material,
    RwTexture* base_texture,
    RwTexture* alpha_texture,
    int has_specular_map) {
    SpecularMaterialPluginData* specular;
    struct SpecLight* light;
    RwTexture* texture;
    GXColor color;
    float scale;
    float material_scale;
    int initial_stage;
    int tev_stage;
    int tex_coord;

    specular = specular_data(material);
    initial_stage = (has_specular_map != 0) + 1;
    tev_stage = initial_stage;
    if (alpha_texture != 0) {
        tev_stage = initial_stage + 1;
    }
    tex_coord = base_texture != 0;

    scale = 1.0f;
    material_scale = 2.0f * material->surface.specular;
    light = specular->light;
    scale = scale <= material_scale ? scale : material_scale;
    color.r = color_component(
        specular->tint.red * (scale * light->color.red));
    color.g = color_component(
        specular->tint.green * (scale * light->color.green));
    color.b = color_component(
        specular->tint.blue * (scale * light->color.blue));
    color.a = 0xFF;
    GXSetTevColor(3, color);

    GXSetNumTexGens(tex_coord + 1);
    GXSetTexCoordGen2(tex_coord, 1, 1, 0x39, 0, 0x7D);
    texture = specular->texture;
    texture->filter_flags =
        (texture->filter_flags & 0xFFFF00FF) | 0x1100;
    _rwDlTextureSet(texture, specTexNum);
    GXSetTevOrder(tev_stage, tex_coord, specTexNum, 0xFF);
    GXSetTevSwapMode(tev_stage, 0, 0);
    GXSetNumTevStages(
        (unsigned char)tev_stage + 1);
    GXSetTevColorIn(tev_stage, 0xF, 8, 6, 0);
    GXSetTevColorOp(tev_stage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(tev_stage, 7, 7, 7, 0);
    GXSetTevAlphaOp(tev_stage, 0, 0, 0, 1, 0);
}

/* TODO: [near miss] 71.08%; GX byte ABI verified; stage-count sinking changes frame and saved homes; stop at lifetime ceiling. */
void CleanupSpecularity(
    RpMaterial* material, RwTexture* base_texture, RwTexture* alpha_texture) {
    SpecularMaterialPluginData* specular;
    GXBool textured;
    unsigned char stage_count;

    stage_count = (alpha_texture != 0) + 1;
    textured = base_texture != 0;
    specular = specular_data(material);

    GXSetNumTexGens(textured);
    GXSetNumTevStages(stage_count);
    if (alpha_texture != 0 && specular->flags.bits.swapMode != 0) {
        GXSetTevSwapMode(2, 0, 0);
        GXSetTevSwapMode(3, 0, 0);
    }
}

void SetupAtomicSpecularity(RpAtomic* atomic) {
    RpGeometry* geometry = atomic->geometry;
    struct SpecularGeometryData* extension = specular_geometry(geometry);
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
        &combined, RwFrameGetLTM(atomic->object.parent), &inverse);

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

int SpecularCreatePipelines(void) {
    SpecSkinAtomicPipeline = _rpDlAtomicPipelineCreate(
        0xDC, 0, _rpSkinInstanceCallback, _rpSkinAtomicReinstanceCallBack,
        GCSpecSkinLighting, MKSpecSkinRenderCallback);
    if (SpecSkinAtomicPipeline == 0) {
        return 0;
    }

    ReflectionAtomicPipeline = _rpDlAtomicPipelineCreate(
        0xDC, 0, _rpSkinInstanceCallback, _rpSkinAtomicReinstanceCallBack,
        GCSpecSkinLighting, MKReflectionRenderCallback);
    if (ReflectionAtomicPipeline != 0) {
        return 1;
    }
    return 0;
}

void SetupShadowPlayerPipeline(RpClump* clump) {
}

static RpAtomic* MKReflectionRenderCallback(
    RpAtomic* atomic, struct SpecResourceEntry* resource) {
    RpSkin* skin;
    void* vertex_format;
    RwMatrix* atomic_ltm;
    unsigned int bone;

    resource->display_resource->header.token = _RwDlTokenCurrent;
    vertex_format = geometry_vertex_format(atomic->geometry);
    atomic_ltm = RwFrameGetLTM(atomic->object.parent);
    _rwDlVtxFmtSetup(vertex_format, resource);

    skin = RpSkinGeometryGetSkin(atomic->geometry);
    if (skin->maxNumWeights > 1) {
        _rwDlTransformSetup(atomic_ltm, 1);
    } else {
        GXSetVtxDesc(0, 1);
    }

    _rwDlObjectRenderSetup(
        resource->object_setup_0,
        resource->object_setup_2,
        resource->object_setup_1,
        0);
    if (skin->maxNumWeights == 1) {
        for (bone = 0; bone < skin->numUsedBones; bone++) {
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                    [skin->usedBoneList[bone]],
                bone * 3,
                1);
        }
    }
    return atomic;
}

/* TODO: [near miss] 99.56%; only atomic/resource parameter registers are
 * swapped (r30/r31); local declaration order does not move them. */
static RpAtomic* MKSpecSkinRenderCallback(
    RpAtomic* atomic, struct SpecResourceEntry* resource) {
    struct SpecCamera* camera;
    RpSkin* skin;
    RwMatrix* atomic_ltm;
    void* vertex_format;
    unsigned int bone;
    RwMatrix object_to_camera;
    int old_src_blend;
    int old_dst_blend;

    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x14, 1);
    RwEngineInstance->dOpenDevice.fpRenderStateGet(0x0A, &old_src_blend);
    RwEngineInstance->dOpenDevice.fpRenderStateGet(0x0B, &old_dst_blend);

    resource->display_resource->header.token = _RwDlTokenCurrent;
    vertex_format = geometry_vertex_format(atomic->geometry);
    atomic_ltm = RwFrameGetLTM(atomic->object.parent);
    _rwDlVtxFmtSetup(vertex_format, resource);

    skin = RpSkinGeometryGetSkin(atomic->geometry);
    if (skin->maxNumWeights > 1) {
        _rwDlTransformSetup(atomic_ltm, 1);
    } else {
        GXSetVtxDesc(0, 1);
    }

    _rwDlObjectRenderSetup(
        resource->object_setup_0,
        resource->object_setup_2,
        resource->object_setup_1,
        0);
    if (skin->maxNumWeights == 1) {
        for (bone = 0; bone < skin->numUsedBones; bone++) {
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                    [skin->usedBoneList[bone]],
                bone * 3,
                1);
        }
    }

    camera = RwEngineInstance->curCamera;
    RwMatrixMultiply(
        &object_to_camera,
        RwFrameGetLTM(atomic->object.parent),
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

    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x0A, 2);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x0B, 2);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x0A, old_src_blend);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x0B, old_dst_blend);
    return atomic;
}

static inline void upload_material_transform(
    RpAtomic* atomic, struct SpecMesh* mesh) {
    MkSobj* sobj = mksobj_data(atomic)->sobj;
    RpSkin* skin =
        RpSkinGeometryGetSkin(atomic->geometry);

    if (sobj != 0 && skin->maxNumWeights > 1) {
        struct SpecMatrixPalette* palette = (struct SpecMatrixPalette*)sobj->matrices;
        int has_transform = 0;
        unsigned int material_number =
            (mkmaterial_data(mesh->material)->flags & 0xBFF) / 10 - 1;

        if (palette != 0) {
            has_transform = (palette->valid_bits >> material_number) & 1;
        }
        if (has_transform != 0 || bLastMatUploadedRoot != 0) {
            RwMatrix* transform;
            if (has_transform != 0) {
                bLastMatUploadedRoot = 1;
                transform = &palette->matrix[material_number];
            } else {
                transform = RwFrameGetLTM(atomic->object.parent);
                bLastMatUploadedRoot = 0;
            }
            _rwDlTransformSetup(transform, 1);
        }
    }
}

static inline void draw_spec_mesh(
    RpAtomic* atomic,
    struct SpecMesh* first_mesh,
    struct SpecMesh* mesh,
    struct SpecDisplayList* display_lists,
    int alpha_pass) {
    unsigned int display_index;

    upload_material_transform(atomic, mesh);
    if (RpMaterialGetAlphaPassTexture(mesh->material) != 0) {
        GCSpecSkinMaterial(mesh, alpha_pass);
    } else {
        GCSpecSkinMaterialNoSpecmap(mesh);
    }

    display_index = mesh - first_mesh;
    GXCallDisplayList(
        display_lists[display_index].data,
        display_lists[display_index].size);
}

static inline struct SpecMesh** spec_mesh_slot(
    struct SpecMesh** base, unsigned int byte_offset) {
    return (struct SpecMesh**)((unsigned char*)base + byte_offset);
}

static inline int* spec_priority_slot(
    int* base, unsigned int byte_offset) {
    return (int*)((unsigned char*)base + byte_offset);
}

/* TODO: [near miss] 97.35%; declaration order improves coloring; header address staging and inlined material load schedule remain. */
static void SpecSkinProcessMaterialList(
    RpAtomic* atomic, struct SpecResourceEntry* resource) {
    struct SpecMesh* alpha_meshes[64];
    struct SpecMesh* reflection_meshes[64];
    int reflection_priority[64];
    struct SpecMeshHeader* mesh_header;
    struct SpecDisplayHeader* display_header;
    unsigned int num_meshes;
    struct SpecMesh* mesh;
    struct SpecDisplayList* display_lists;
    struct SpecMesh* first_mesh;
    unsigned int mesh_index;
    int alpha_count = 0;
    int reflection_count = 0;
    int i;
    unsigned int reflection_offset = 0;

    display_header = &resource->display_resource->header;
    bLastMatUploadedRoot = 1;
    display_lists =
        &display_header->lists[display_header->display_list_count - 1];
    mesh_header = resource->mesh_header;
    first_mesh = mesh_header->meshes;
    num_meshes = mesh_header->num_meshes;

    for (mesh_index = 0, mesh = first_mesh;
         mesh_index < num_meshes;
         mesh_index++, mesh++) {
        SpecularMaterialPluginData* specular = specular_data(mesh->material);
        unsigned int flags;

        if (specular->flags.bits.hidden != 0) {
            continue;
        }
        flags = mkmaterial_data(mesh->material)->flags;

        if ((flags & 0x40000000) == 0x40000000) {
            int insert = -1;
            int priority = (flags >> 16) & 0xFF;
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
            if (specular->flags.bits.cullFront != 0) {
                GXSetCullMode(0);
            } else {
                GXSetCullMode(1);
            }
            draw_spec_mesh(
                atomic, first_mesh, mesh, display_lists, 0);
        }
    }

    for (i = 0; i < alpha_count; i++) {
        struct SpecMesh* mesh = alpha_meshes[i];
        SpecularMaterialPluginData* specular = specular_data(mesh->material);

        if (specular->flags.bits.cullFront != 0) {
            GXSetCullMode(0);
        } else {
            GXSetCullMode(1);
        }
        draw_spec_mesh(
            atomic, first_mesh, mesh, display_lists, 0);
    }
    if (reflection_count > 0) {
        for (i = reflection_count - 1; i >= 0; i--) {
            struct SpecMesh* mesh = reflection_meshes[i];
            SpecularMaterialPluginData* specular = specular_data(mesh->material);

            if (specular->flags.bits.reflectionPass == 0) {
                GXSetCullMode(2);
                draw_spec_mesh(
                    atomic, first_mesh, mesh, display_lists, 1);
            }
        }
        for (i = 0; i < reflection_count; i++) {
            struct SpecMesh* mesh = reflection_meshes[i];

            GXSetCullMode(1);
            draw_spec_mesh(
                atomic, first_mesh, mesh, display_lists, 1);
        }
    }

    GXSetTevSwapMode(1, 0, 0);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x14, 1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x14, 2);
}

static inline void apply_material_z_bias(float bias) {
    union {
        float value;
        int bits;
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

static inline void setup_material_channels(
    RpMaterial* material, const GXColor* default_ambient) {
    GXColor ambient;
    GXColor diffuse;
    float scale;

    if (pAmbLight != 0) {
        scale = 255.0f * material->surface.ambient;
        ambient.r = color_component(pAmbLight->color.red * scale);
        ambient.g = color_component(pAmbLight->color.green * scale);
        ambient.b = color_component(pAmbLight->color.blue * scale);
        ambient.a = 0;
    } else {
        ambient = *default_ambient;
    }
    GXSetChanAmbColor(0, ambient);

    diffuse.r = color_component(
        material->color.red * material->surface.diffuse);
    diffuse.g = color_component(
        material->color.green * material->surface.diffuse);
    diffuse.b = color_component(
        material->color.blue * material->surface.diffuse);
    diffuse.a = 0;
    GXSetChanMatColor(0, diffuse);
}

static inline void setup_base_z_compare(RwTexture* texture) {
    RwRaster* raster_owner;

    if (texture != 0 && texture->raster != 0) {
        raster_owner = texture->raster->parent;
        _rwDlRenderStateSetZCompLoc(
            (RW_RASTER_PLATFORM_DATA(raster_owner)->hasAlpha & 1) ^ 1);
    }
}

/* TODO: [near miss] 92.36%; stack slot order of the channel/specular colors and material/specular r31/r30 swap remain. */
static void GCSpecSkinMaterialNoSpecmap(struct SpecMesh* mesh) {
    RpMaterial* material = mesh->material;
    SpecularMaterialPluginData* specular;
    struct SpecLight* light;
    RwTexture* base_texture;
    RwTexture* specular_texture;
    GXColor default_ambient = {0, 0, 0, 0xFF};
    GXColor specular_color;
    RwMatrix* base_transform;
    float scale;
    float material_scale;

    specular = specular_data(material);
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
    light = specular_data(material)->light;
    scale = 1.0f <= material_scale ? 1.0f : material_scale;
    specular_color.r = color_component(
        specular->tint.red * (scale * light->color.red));
    specular_color.g = color_component(
        specular->tint.green * (scale * light->color.green));
    specular_color.b = color_component(
        specular->tint.blue * (scale * light->color.blue));
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

/* TODO: [breakthrough] 97.76421%; RGB staging fixed; GXColor stack slots and owner registers remain. */
static void GCSpecSkinMaterial(struct SpecMesh* mesh, int alpha_pass) {
    RpMaterial* material = mesh->material;
    SpecularMaterialPluginData* specular = specular_data(material);
    struct SpecLight* light;
    RwTexture* base_texture;
    RwTexture* specular_texture;
    RwTexture* alpha_texture;
    RwMatrix* base_transform;
    GXColor default_ambient = {0, 0, 0, 0xFF};
    GXColor specular_color;
    float scale;
    float material_scale;
    float red_intensity;
    float red_component;
    float green_component;
    apply_material_z_bias(specular->gloss);
    RpMatFXMaterialGetUVTransformMatrices(
        material, &base_transform, 0);
    setup_material_channels(material, &default_ambient);

    alpha_texture = RpMaterialGetAlphaPassTexture(mesh->material);
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
    light = specular_data(material)->light;
    scale = (1.0f <= material_scale) ? 1.0f : material_scale;
    red_intensity = light->color.red;
    red_intensity = red_intensity * scale;
    red_component = specular->tint.red;
    red_component *= red_intensity;
    specular_color.r = color_component(red_component);
    green_component = light->color.green;
    green_component = green_component * scale;
    green_component = specular->tint.green * green_component;
    specular_color.g = color_component(green_component);
    specular_color.b = color_component(specular->tint.blue * (scale * light->color.blue));
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

static inline void find_spec_lights(RwGlobals* engine) {
    struct SpecWorld* world;
    RwLLLink* link;

    pDirLight1 = 0;
    pDirLight2 = 0;
    pAmbLight = 0;
    world = engine->curWorld;
    if (world != 0) {
        for (link = world->directional_lights.next;
             link != &world->directional_lights;
             link = link->next) {
            struct SpecLight* light = light_from_link(link);

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
    world = engine->curWorld;
    if (world != 0) {
        for (link = world->point_lights.next;
             link != &world->point_lights;
             link = link->next) {
            struct SpecLight* light = light_from_link(link);

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


static inline void upload_point_light(
    struct SpecLight* light,
    struct SpecLightingData* lighting,
    const Vec* delta,
    float inverse_distance,
    float intensity) {
    GXColor color;
    int light_index;
    GXLightObj* light_object;
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
    red = (light->color.red * color_scale);
    green = (light->color.green * color_scale);
    blue = (light->color.blue * color_scale);
    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = 0;
    GXInitLightColor(light_object, color);
    GXLoadLightObjImm(light_object, 1U << light_index);
    lighting->light_mask |= 1U << lighting->light_count;
    lighting->light_count++;
}

static inline void upload_directional_light(
    struct SpecLight** light_slot,
    struct SpecLightingData* lighting,
    float intensity,
    RwV3d* direction) {
    struct SpecLight* light;
    RwMatrix* light_ltm;
    GXColor color;
    int light_index;
    GXLightObj* light_object;
    int red;
    int green;
    int blue;
    float color_scale;

    light_ltm = RwFrameGetLTM((*light_slot)->frame);
    RwV3dTransformVector(
        direction, &light_ltm->at, &_RwDlInvCamLTM);
    light_index = lighting->light_count;
    light_object = &_RwGCLightObjs[light_index];
    light = *light_slot;
    GXInitLightAttn(light_object, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    GXInitLightPos(
        light_object,
        -1048576.0f * -direction->x,
        -1048576.0f * direction->y,
        -1048576.0f * -direction->z);
    color_scale = 255.0f * intensity;
    red = (light->color.red * color_scale);
    green = (light->color.green * color_scale);
    blue = (light->color.blue * color_scale);
    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = 0;
    GXInitLightColor(light_object, color);
    GXLoadLightObjImm(light_object, 1U << light_index);
    lighting->light_mask |= 1U << lighting->light_count;
    lighting->light_count++;
}

/* TODO: [breakthrough] 95.91%; light ownership and intensity order recovered;
 * recover point-metric stack storage and first-distance rounding. */
static RpAtomic* GCSpecSkinLighting(
    RpAtomic* atomic, struct SpecLightingData* lighting) {
    RpMaterial* specular_material;
    RwMatrix* atomic_ltm;
    RwMatrix* light_ltm;
    RwMatrix inverse_specular;
    float texture_matrix[3][4];
    RwMatrix combined;
    RwV3d direction;
    struct SpecLight* point1;
    struct SpecLight* point2;
    Vec point1_delta;
    Vec point2_delta;
    float point1_distance_sq;
    float point2_distance_sq;
    float point1_radius;
    float point2_radius;
    float point1_intensity;
    float point2_intensity;
    float point1_inverse_distance;
    float point2_inverse_distance;
    float strongest_point_intensity;
    float directional_intensity;

    lighting->light_mask = 0;
    lighting->ambient.red = 0.0f;
    lighting->ambient.green = 0.0f;
    lighting->ambient.blue = 0.0f;
    lighting->ambient.alpha = 1.0f;
    lighting->light_count = 0;

    RwMatrixInvert(
        &cachedInverseAtomicLTM,
        RwFrameGetLTM(atomic->object.parent));
    specular_material = atomic->geometry->matList.materials[0];
    if ((atomic->geometry->flags | 0x20U) != 0) {
        find_spec_lights(RwEngineInstance);

        if (pAmbLight != 0) {
            lighting->has_ambient = 1;
            lighting->ambient = pAmbLight->color;
        } else {
            lighting->has_ambient = 0;
        }

        strongest_point_intensity = 0.0f;
        directional_intensity = 1.0f;
        if (pPointLight1 != 0) {
            atomic_ltm = RwFrameGetLTM(atomic->object.parent);
            light_ltm = RwFrameGetLTM(pPointLight1->frame);
            point1 = pPointLight1;
            point1_delta.x = light_ltm->pos.x - atomic_ltm->pos.x;
            point1_delta.y = light_ltm->pos.y - atomic_ltm->pos.y;
            point1_delta.z = light_ltm->pos.z - atomic_ltm->pos.z;
            point1_radius = point1->radius;
            point1_distance_sq =
                point1_delta.x * point1_delta.x +
                point1_delta.y * point1_delta.y +
                point1_delta.z * point1_delta.z;
            if (pPointLight2 != 0) {
                light_ltm = RwFrameGetLTM(pPointLight2->frame);
                point2 = pPointLight2;
                point2_delta.x = light_ltm->pos.x - atomic_ltm->pos.x;
                point2_delta.y = light_ltm->pos.y - atomic_ltm->pos.y;
                point2_delta.z = light_ltm->pos.z - atomic_ltm->pos.z;
                point2_radius = point2->radius;
                point2_distance_sq =
                    point2_delta.x * point2_delta.x +
                    point2_delta.y * point2_delta.y +
                    point2_delta.z * point2_delta.z;
                point1_intensity =
                    1.0f - gxMathFastSqrt(point1_distance_sq) /
                        point1_radius;
                point2_intensity =
                    1.0f - gxMathFastSqrt(point2_distance_sq) / point2_radius;
                if (point1_intensity < 0.0f) {
                    point1_intensity = 0.0f;
                }
                if (point2_intensity < 0.0f) {
                    point2_intensity = 0.0f;
                }
                strongest_point_intensity =
                    point1_intensity >= point2_intensity
                        ? point1_intensity : point2_intensity;
                point2_inverse_distance = gxMathFastInvSqrt(point2_distance_sq);
                upload_point_light(
                    point2, lighting, &point2_delta,
                    point2_inverse_distance, point2_intensity);
            } else {
                point1_intensity =
                    1.0f - gxMathFastSqrt(point1_distance_sq) /
                        point1_radius;
                if (point1_intensity < 0.0f) {
                    point1_intensity = 0.0f;
                }
                strongest_point_intensity = point1_intensity;
            }

            directional_intensity =
                0.5f >= 1.0f - strongest_point_intensity
                    ? 0.5f : 1.0f - strongest_point_intensity;

            point1_inverse_distance = gxMathFastInvSqrt(point1_distance_sq);
            upload_point_light(
                point1, lighting, &point1_delta,
                point1_inverse_distance, point1_intensity);
        }

        if (pDirLight1 != 0) {
            upload_directional_light(
                &pDirLight1, lighting, directional_intensity, &direction);

            SpecularMaterialCalcMatrix(specular_material);
            combined.flags = 0x20003;
            inverse_specular.flags = 0x20003;
            RwMatrixInvert(&inverse_specular, &SpecularMatrix);
            RwMatrixMultiply(
                &combined,
                RwFrameGetLTM(atomic->object.parent),
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
                &pDirLight2, lighting, directional_intensity, &direction);
        }
    }
    return atomic;
}
