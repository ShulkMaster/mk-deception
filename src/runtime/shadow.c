#include "runtime/shadow.h"
#include "runtime/plyr_pdata.h"

#include "math/gxQuat.h"
#include "math/mk_math.h"
#include "math/gxVect.h"
#include "platform/display.h"
#include "platform/gcutils.h"
#include "runtime/mk_obj.h"
#include "runtime/asset.h"
#include "runtime/mk_struct.h"
#include "rw/gamecube.h"
#include "rw/rpworld_types.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwengine.h"
#include "rw/rwframe.h"
#include "rw/rwvector.h"


typedef struct ShadowboxObject {
    MkObj object;
} ShadowboxObject;

static const char stringBase0[] = "Shadow2\0SHADOWBOX";

static const float kShadowScaleDefault = 1.7f;
static const float kShadowScaleAlt = 2.5f;
static const float kFarClipMul = 2.0f;
static const float kNearClipMul = 0.001f;
static const float kViewWindowBias = -0.5f;
static const float kHalf = 0.5f;
static const float kOne = 1.0f;
static const float kZero = 0.0f;
static const float kPi = 3.1415927f;
static const float kGroundOffset = 0.005f;
static const float kAlphaScale = 255.0f;

static int ShadowResolutionIndex = 8;
static int ShadowBlur = 1;
static int ShadowAA = 1;

RwMatrix ShadowDirectionMatrix;

float ShadowStrength;
static unsigned int save_res_for_shadowbox;
static RwRaster* ShadowRasterAA;
static RwCamera* ShadowIPCamera;
static RwRaster* ShadowCameraRaster;
static RwCamera* ShadowCamera;

static RpAtomic* shadow_getFirstAtomic(RpAtomic* atomic, void* out);
static int Im2DRenderQuad(int alpha, float p1, float p2, float p3,
                          float p4, float depth, float p6, float p7);
static inline void shadow_destroy_shadowbox(MkObj** box_ptr);

struct Im2DVertex {
    float u;
    float v;
    float z;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    float x;
    float y;
};

static inline void shadow_destroy_shadowbox(MkObj** box_ptr) {
    MkObj* box;

    box = *box_ptr;
    if (box == NULL) {
        return;
    }
    if (box->hdr.instance != 0) {
        box->hdr.typed_vtbl->destroy(&box->hdr);
    }
    *box_ptr = NULL;
}

static int Im2DRenderQuad(int alpha, float p1, float p2, float p3,
                          float p4, float depth, float p6, float p7) {
    static unsigned char colorgray;
    struct Im2DVertex vertices[4];
    float v_top;

    v_top = kOne + p7;
    vertices[0].u = p1;
    vertices[0].v = p2;
    vertices[0].z = depth;
    vertices[0].r = colorgray;
    vertices[0].g = colorgray;
    vertices[0].b = colorgray;
    vertices[0].a = alpha;
    vertices[0].x = p7;
    vertices[0].y = p7;
    vertices[1].u = p1;
    vertices[1].v = p4;
    vertices[1].z = depth;
    vertices[1].r = colorgray;
    vertices[1].g = colorgray;
    vertices[1].b = colorgray;
    vertices[1].a = alpha;
    vertices[1].x = p7;
    vertices[1].y = v_top;
    vertices[2].u = p3;
    vertices[2].v = p2;
    vertices[2].z = depth;
    vertices[2].r = colorgray;
    vertices[2].g = colorgray;
    vertices[2].b = colorgray;
    vertices[2].a = alpha;
    vertices[2].x = v_top;
    vertices[2].y = p7;
    vertices[3].u = p3;
    vertices[3].v = p4;
    vertices[3].z = depth;
    vertices[3].r = colorgray;
    vertices[3].g = colorgray;
    vertices[3].b = colorgray;
    vertices[3].a = alpha;
    vertices[3].x = v_top;
    vertices[3].y = v_top;
    RwEngineInstance->dOpenDevice.fpIm2DRenderPrimitive(4, vertices, 4);
    return 1;
}

static void ShadowCameraDestroy(RwCamera* camera) {
    RwFrame* frame;
    RwRaster* raster;

    if (camera == NULL) {
        return;
    }
    frame = rwCameraParentFrame(camera);
    if (frame != NULL) {
        _rwObjectHasFrameSetFrame(camera, NULL);
        RwFrameDestroy(frame);
    }
    raster = camera->zBuffer;
    if (raster != NULL) {
        camera->zBuffer = NULL;
        RwRasterDestroy(raster);
    }
    if (camera->frameBuffer != NULL) {
        camera->frameBuffer = NULL;
    }
    RwCameraDestroy(camera);
}

static RwCamera* ShadowCameraCreate(int resolution) {
    RwCamera* camera;
    RwFrame* frame;
    RwRaster* raster;
    camera = RwCameraCreate();
    if (camera != NULL) {
        frame = RwFrameCreate();
        _rwObjectHasFrameSetFrame(camera, frame);
        if (rwCameraParentFrame(camera) != NULL) {
            raster = RwRasterCreate(resolution, resolution, 0, 1);
            if (raster != NULL) {
                camera->zBuffer = raster;
                RwCameraSetProjection(camera, 2);
                return camera;
            }
        }
    }
    ShadowCameraDestroy(camera);
    return NULL;
}

static RwCamera* ShadowCameraSetFrustum(RwCamera* camera, float radius) {
    RwV2d view_window;

    RwCameraSetFarClipPlane(camera, kFarClipMul * radius);
    RwCameraSetNearClipPlane(camera, kNearClipMul * radius);
    view_window.x = radius;
    view_window.y = radius;
    RwCameraSetViewWindow(camera, &view_window);
    return camera;
}

static RwCamera* ShadowCameraSetLight(RwCamera* camera, RwMatrix* light_matrix) {
    RwFrame* frame;

    frame = rwCameraParentFrame(camera);
    frame->modelling.right = light_matrix->right;
    frame->modelling.up = light_matrix->up;
    frame->modelling.at = light_matrix->at;
    RwMatrixUpdate(&frame->modelling);
    RwFrameUpdateObjects(frame);
    return camera;
}

static RwCamera* ShadowCameraSetCenter(RwCamera* camera, Vec* center) {
    RwFrame* frame;

    frame = rwCameraParentFrame(camera);
    frame->modelling.pos_vec = *center;
    frame->modelling.pos.x = frame->modelling.pos.x + frame->modelling.at.x * (kViewWindowBias * camera->farPlane);
    frame->modelling.pos.y = frame->modelling.pos.y + frame->modelling.at.y * (kViewWindowBias * camera->farPlane);
    frame->modelling.pos.z = frame->modelling.pos.z + frame->modelling.at.z * (kViewWindowBias * camera->farPlane);
    RwMatrixUpdate(&frame->modelling);
    RwFrameUpdateObjects(frame);
    return camera;
}

int ShadowCameraUpdate_flag;

/* TODO: [near miss] 99.32098%; clear color and camera return match; node/atomic saved registers remain swapped. */
RwCamera* ShadowCameraUpdate(RwCamera* camera, RpClump* clump, int clear) {
    RwRGBA clear_color = {255, 255, 255, 0};
    RpAtomic* atomic;
    RwLLLink* end;
    RwLLLink* node;
    RpGeometry* geometry;
    unsigned int saved_flags;

    if (clear != 0) {
        RwCameraClear(camera, &clear_color, 3);
    }
    RwFrameOrthoNormalize(rwCameraParentFrame(camera));
    if (RwCameraBeginUpdate(camera) != 0) {
        set_render_state(0xA, 5);
        set_render_state(0xB, 6);
        set_render_state(0x6, 0);
        set_render_state(0x8, 0);
        set_render_state(0xC, 0);
        set_render_state(0xA, 2);
        set_render_state(0xB, 1);
        set_render_state(0x6, 1);
        set_render_state(0x8, 1);
        set_render_state(0xC, 1);
        node = clump->atomicList.next;
        end = &clump->atomicList;
        while (node != end) {
            atomic = rpAtomicFromClumpNode(node);
            if (atomic->object.flags & 4) {
                geometry = atomic->geometry;
                saved_flags = geometry->flags;
                geometry->flags = saved_flags & ~0x20;
                RwFrameGetLTM(atomic->object.parent);
                atomic->renderCallBack(atomic);
                geometry->flags = saved_flags;
            }
            node = node->next;
        }
        RwCameraEndUpdate(camera);
        RwGameCubeCameraTextureFlush(camera->frameBuffer, 0);
    }
    return camera;
}

static RwRaster* ShadowRasterCreate(int size) {
    return RwRasterCreate(size, size, 0x20, 0x505);
}

static void ShadowRasterDestroy(RwRaster* raster) {
    RwRasterDestroy(raster);
}

static int ShadowRasterResample(RwRaster* dest_raster, RwRaster* source_raster, RwCamera* camera) {
    RwRGBA clear_color = {0xFF, 0xFF, 0xFF, 0x00};
    float raster_width;
    float half_texel;
    float inv_far;

    raster_width = dest_raster->width;
    half_texel = kHalf / raster_width;
    inv_far = kOne / camera->farPlane;
    camera->frameBuffer = dest_raster;
    RwCameraClear(camera, &clear_color, 3);
    if (RwCameraBeginUpdate(camera) != 0) {
        set_render_state(0xA, 2);
        set_render_state(0xB, 1);
        set_render_state(0x6, 0);
        set_render_state(0x2, 3);
        set_render_state(0x9, 2);
        set_render_state(1, (int)source_raster);
        Im2DRenderQuad(0xFF, kZero, kZero, raster_width, raster_width,
                       RwEngineInstance->dOpenDevice.zBufferFar, inv_far,
                       half_texel);
        set_render_state(0x6, 1);
        set_render_state(0xA, 5);
        set_render_state(0xB, 6);
        RwCameraEndUpdate(camera);
        RwGameCubeCameraTextureFlush(camera->frameBuffer, 0);
    }
    camera->frameBuffer = NULL;
    return 1;
}

int ShadowRasterBlur(RwRaster* src_raster, RwRaster* dst_raster,
                     RwCamera* ip_camera, unsigned int pass_count) {
    RwRGBA clear_color = {255, 255, 255, 0};
    unsigned int pass;
    float raster_width;
    float inv_width;
    float inv_far;
    int alpha;

    raster_width = src_raster->width;
    inv_width = kOne / raster_width;
    inv_far = kOne / ip_camera->farPlane;
    for (pass = 0; pass < pass_count; pass++) {
        ip_camera->frameBuffer = dst_raster;
        RwCameraClear(ip_camera, &clear_color, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(0xA, 2);
            set_render_state(0xB, 1);
            set_render_state(0x6, 0);
            set_render_state(0x9, 2);
            set_render_state(0x2, 3);
            set_render_state(1, (int)src_raster);
            Im2DRenderQuad(0xFF, kZero, kZero, raster_width,
                           raster_width, RwEngineInstance->dOpenDevice.zBufferFar,
                           inv_far,
                           inv_width);
            RwCameraEndUpdate(ip_camera);
            RwGameCubeCameraTextureFlush(ip_camera->frameBuffer, 0);
        }
        ip_camera->frameBuffer = src_raster;
        RwCameraClear(ip_camera, &clear_color, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(1, (int)dst_raster);
            if (pass < pass_count - 1) {
                Im2DRenderQuad(0xFF, kZero, kZero, raster_width,
                               raster_width,
                               RwEngineInstance->dOpenDevice.zBufferFar,
                               inv_far, kZero);
            } else {
                alpha = (kAlphaScale * ShadowStrength);
                Im2DRenderQuad(alpha, kZero, kZero,
                               raster_width, raster_width,
                               RwEngineInstance->dOpenDevice.zBufferFar,
                               inv_far, kZero);
            }
            set_render_state(0x6, 1);
            set_render_state(0xA, 5);
            set_render_state(0xB, 6);
            RwCameraEndUpdate(ip_camera);
            RwGameCubeCameraTextureFlush(ip_camera->frameBuffer, 0);
        }
    }
    ip_camera->frameBuffer = NULL;
    return 1;
}

static RpAtomic* shadow_getFirstAtomic(RpAtomic* atomic, void* out) {
    RpAtomic** result = out;

    *result = atomic;
    return 0;
}

/* TODO: [near miss] 99.67%; SetLight frame takes r28 (retail r29): ShadowCameraDestroy frame
 * web is coloured first and claims r28; helper object order is the open lead. */
int init_shadow_system(void) {
    unsigned int resolution;
    unsigned int aa_resolution;
    RwRaster* raster;

    if (ShadowCamera != NULL) {
        return 1;
    }
    resolution = 1U << ShadowResolutionIndex;
    if (ShadowAA != 0) {
        aa_resolution = resolution >> 1;
    } else {
        aa_resolution = resolution;
    }
    ShadowCamera = ShadowCameraCreate(resolution);
    if (ShadowCamera == NULL) {
        return 0;
    }
    ShadowCameraSetLight(ShadowCamera, &ShadowDirectionMatrix);
    ShadowIPCamera = ShadowCameraCreate(aa_resolution);
    if (ShadowIPCamera == NULL) {
        return 0;
    }
    raster = ShadowRasterCreate(resolution);
    ShadowCameraRaster = raster;
    if (raster == NULL) {
        return 0;
    }
    ShadowCamera->frameBuffer = raster;
    if (ShadowAA != 0 && ShadowRasterAA == NULL) {
        raster = ShadowRasterCreate(aa_resolution);
        ShadowRasterAA = raster;
        if (raster == NULL) {
            return 0;
        }
    }
    save_res_for_shadowbox = aa_resolution;
    return 1;
}

int SetupShadow(ShadowObject* shadow) {
    PlyrPdata* owner = (PlyrPdata*)shadow;
    RpMaterial* material;
    MkSobj* sobj;
    unsigned int filter;

    owner->shadow_raster = RwRasterCreate(save_res_for_shadowbox,
                                            save_res_for_shadowbox, 0x20,
                                            0x505);
    if (owner->shadow_raster == NULL) {
        return 0;
    }
    if (owner->character_id == 0x1D) {
        if (owner->plyr_num == 0) {
            owner->shadowbox = load_model_from_slot_transl(
                0x0003000B, 0x008F0002, 0x5012);
        } else {
            owner->shadowbox = load_model_from_slot_transl(
                0x0004000B, 0x008F0002, 0x5012);
        }
    } else {
        owner->shadowbox = load_model_from_slot_transl(
            0, 0x0001000A, 0x5012);
    }
    if (owner->shadowbox == NULL) {
        return 0;
    }
    owner->shadowbox->flags_08_bits.airborne = 1;
    owner->shadowbox->flags_08_bits.angular_velocity_enabled = 1;
    owner->shadowbox->flags_08_bits.scale_active = 1;
    insert_fgnd_mkobj(owner->shadowbox);
    if (owner->character_id == 0x1D) {
        material = obj_find_material_with_texture(owner->shadowbox, stringBase0);
    } else {
        material = obj_find_material_with_texture(owner->shadowbox, stringBase0 + 8);
    }
    if (material != NULL) {
        material->texture = RwTextureCreate(owner->shadow_raster);
        if (material->texture != NULL) {
            filter = material->texture->filter_flags;
            filter = (filter & ~0xFF) | 2;
            material->texture->filter_flags = filter;
            filter = material->texture->filter_flags;
            filter = (filter & 0xFFFF00FF) | 0x3300;
            material->texture->filter_flags = filter;
            owner->shadow_texture = material->texture;
        }
    }
    obj_create_sobjs(owner->shadowbox);
    sobj = obj_first_sobj(owner->shadowbox);
    if (sobj != NULL) {
        sobj->flags09_bits.bit7 = 1;
        sobj->render_flags = 0x10006;
        sobj_set_priority(sobj, 0xC);
    }
    owner->shadowbox->pos.value.y = kGroundOffset;
    owner->shadowbox->scale.y = kOne;
    return 1;
}

void shadow_set_new_ground_plane(ShadowObject* shadow, ShadowboxObject* ground,
                                 float y) {
    PlyrPdata* owner = (PlyrPdata*)shadow;
    MkObj* box;

    box = owner->shadowbox;
    if (box != 0) {
        box->pos.value.y = kGroundOffset + y;
    }
    if (ground == 0) {
        return;
    }
    ground->object.pos.value.y = y;
}

void TearDownShadow(ShadowObject* shadow) {
    PlyrPdata* owner = (PlyrPdata*)shadow;
    MkObj* box;

    if (owner->shadow_raster != 0) {
        RwRasterDestroy(owner->shadow_raster);
        owner->shadow_raster = 0;
    }
    if (owner->shadow_texture != 0) {
        owner->shadow_texture->raster = NULL;
        RwTextureDestroy(owner->shadow_texture);
        owner->shadow_texture = 0;
    }
    box = owner->shadowbox;
    if (box != 0) {
        if (box->hdr.instance != 0) {
            box->hdr.typed_vtbl->destroy(&box->hdr);
        }
        owner->shadowbox = 0;
    }
}

void destroy_shadow_system(void) {
    if (ShadowCamera != NULL) {
        ShadowCameraDestroy(ShadowCamera);
        ShadowCamera = NULL;
    }
    if (ShadowIPCamera != NULL) {
        ShadowCameraDestroy(ShadowIPCamera);
        ShadowIPCamera = NULL;
    }
    if (ShadowCameraRaster != NULL) {
        ShadowRasterDestroy(ShadowCameraRaster);
        ShadowCameraRaster = NULL;
    }
    if (ShadowRasterAA != NULL) {
        ShadowRasterDestroy(ShadowRasterAA);
        ShadowRasterAA = NULL;
    }
}

int UpdateShadowCameraLightSource(const float* angles) {
    YXZ_angles_to_MKMATRIX((const Vec*)angles, &ShadowDirectionMatrix);
    return 1;
}

/* TODO: [near miss] 99.90%; Resample camera/source homes r27/r30 swapped: SetFrustum camera
 * param coalesces with the propagated ShadowCamera load temp (replay: uncoalesced = retail). */
void UpdateShadow(MkObj* fighter_object, PlyrPdata* owner, MkObj* object) {
    MkObj* fighter;
    RwMatrix* dir_matrix;
    MkObj* validated;
    MKVECTOR center;
    MKVECTOR light_pos;
    MKVECTOR delta_pos;
    MKVECTOR light_dir;
    MKVECTOR corner;
    MKVECTOR corner_a;
    MKVECTOR corner_b;
    MKVECTOR corner_c;
    MKVECTOR plane_normal;
    MKVECTOR edge_a;
    MKVECTOR edge_b;
    Vec center_offset;
    Vec corner_a_spread;
    Vec corner_a_offset;
    Vec corner_b_spread;
    Vec corner_b_offset;
    Vec corner_c_spread;
    Vec corner_c_offset;
    Vec plane_point;
    float right_x;
    float right_y;
    float right_z;
    float up_x;
    float up_y;
    float up_z;
    float shadow_scale;
    float proj_scale;
    float angle;
    float mag_a;
    float mag_b;
    int clear_flags;

    fighter = fighter_object;
    shadow_scale = kShadowScaleDefault;
    if (owner->character_id == 0x1D) {
        shadow_scale = kShadowScaleAlt;
    }
    gc_enable_alpha_writes(1);
    RwV3dTransformPoints(
        (RwV3d*)&owner->shadow_ground_point, (const RwV3d*)&object->pos.value, 1,
        &object->frame->modelling);
    ShadowCameraSetLight(ShadowCamera, &ShadowDirectionMatrix);
    ShadowCameraSetFrustum(ShadowCamera, shadow_scale);
    ShadowCameraSetCenter(ShadowCamera, &fighter->pos.value);
    ShadowCameraUpdate_flag = 1;
    ShadowCameraUpdate(ShadowCamera, object->clump, 1);
    if (owner->mirror_slots != NULL) {
        validated = MK_HDR_LIVE(owner->mirror_slots->weapon[0].primary.obj, owner->mirror_slots->weapon[0].primary.instance);
        if (validated != NULL && !validated->hide_flag_bits.hidden) {
            validated = MK_HDR_LIVE(owner->mirror_slots->weapon[0].mirror.obj, owner->mirror_slots->weapon[0].mirror.instance);
            if (validated != NULL) {
                ShadowCameraUpdate(ShadowCamera, validated->clump, 0);
            }
        }
        validated = MK_HDR_LIVE(owner->mirror_slots->weapon[1].primary.obj, owner->mirror_slots->weapon[1].primary.instance);
        if (validated != NULL && !validated->hide_flag_bits.hidden) {
            validated = MK_HDR_LIVE(owner->mirror_slots->weapon[1].mirror.obj, owner->mirror_slots->weapon[1].mirror.instance);
            if (validated != NULL) {
                ShadowCameraUpdate(ShadowCamera, validated->clump, 0);
            }
        }
    }
    validated = MK_HDR_LIVE(owner->aux_weapon_latch.obj, owner->aux_weapon_latch.instance);
    if (validated != NULL && !validated->hide_flag_bits.hidden) {
        validated = MK_HDR_LIVE(owner->mirror_obj.obj, owner->mirror_obj.instance);
        if (validated != NULL) {
            ShadowCameraUpdate(ShadowCamera, validated->clump, 0);
        }
    }
    clear_flags = ShadowAA;
    ShadowCameraUpdate_flag = 0;
    if (clear_flags != 0) {
        ShadowRasterResample(owner->shadow_raster, ShadowCameraRaster, ShadowIPCamera);
        owner->shadow_blur_raster = ShadowRasterAA;
    } else {
        owner->shadow_blur_raster = ShadowCameraRaster;
    }
    if (ShadowBlur != 0) {
        ShadowRasterBlur(owner->shadow_raster, owner->shadow_blur_raster,
                         ShadowIPCamera, ShadowBlur);
    }
    dir_matrix = &ShadowDirectionMatrix;
    plane_normal.z = plane_normal.y = plane_normal.x = kZero;
    plane_normal.y = kOne;
    plane_point.x = kZero;
    plane_point.z = kZero;
    right_x = dir_matrix->right.x;
    light_pos.x = fighter->pos.value.x;
    right_y = dir_matrix->right.y;
    right_z = dir_matrix->right.z;
    light_pos.y = fighter->pos.value.y;
    up_x = dir_matrix->up.x;
    up_y = dir_matrix->up.y;
    light_pos.z = fighter->pos.value.z;
    up_z = dir_matrix->up.z;
    plane_point.y = fighter->ground_colls_y;
    delta_pos.x = light_pos.x - plane_point.x;
    delta_pos.y = light_pos.y - plane_point.y;
    delta_pos.z = light_pos.z - plane_point.z;
    light_dir.x = dir_matrix->at.x;
    light_dir.y = dir_matrix->at.y;
    light_dir.z = dir_matrix->at.z;
    proj_scale = kOne / PSVECDotProduct(&plane_normal, &light_dir);
    angle = -PSVECDotProduct(&plane_normal, &delta_pos) * proj_scale;
    center_offset.x = angle * light_dir.x;
    center_offset.y = angle * light_dir.y;
    center_offset.z = angle * light_dir.z;
    PSVECAdd(&light_pos, &center_offset, &center);
    owner->shadowbox->pos.value.x = center.x;
    owner->shadowbox->pos.value.z = center.z;
    owner->shadowbox->ang.y = gxVectAngleZX(&light_dir) - kPi;
    corner_a_spread.x = (up_x - right_x) * shadow_scale;
    corner_a_spread.y = (up_y - right_y) * shadow_scale;
    corner_a_spread.z = (up_z - right_z) * shadow_scale;
    PSVECAdd(&light_pos, &corner_a_spread, &corner);
    angle = -PSVECDotProduct(&plane_normal, &corner) * proj_scale;
    corner_a_offset.x = angle * light_dir.x;
    corner_a_offset.y = angle * light_dir.y;
    corner_a_offset.z = angle * light_dir.z;
    PSVECAdd(&corner, &corner_a_offset, &corner_a);
    corner_b_spread.x = (right_x + up_x) * shadow_scale;
    corner_b_spread.y = (right_y + up_y) * shadow_scale;
    corner_b_spread.z = (right_z + up_z) * shadow_scale;
    PSVECAdd(&light_pos, &corner_b_spread, &corner);
    angle = -PSVECDotProduct(&plane_normal, &corner) * proj_scale;
    corner_b_offset.x = angle * light_dir.x;
    corner_b_offset.y = angle * light_dir.y;
    corner_b_offset.z = angle * light_dir.z;
    PSVECAdd(&corner, &corner_b_offset, &corner_b);
    corner_c_spread.x = (-up_x - right_x) * shadow_scale;
    corner_c_spread.y = (-up_y - right_y) * shadow_scale;
    corner_c_spread.z = (-up_z - right_z) * shadow_scale;
    PSVECAdd(&light_pos, &corner_c_spread, &corner);
    angle = -PSVECDotProduct(&plane_normal, &corner) * proj_scale;
    corner_c_offset.x = angle * light_dir.x;
    corner_c_offset.y = angle * light_dir.y;
    corner_c_offset.z = angle * light_dir.z;
    PSVECAdd(&corner, &corner_c_offset, &corner_c);
    PSVECSubtract(&corner_b, &corner_a, &edge_a);
    PSVECSubtract(&corner_c, &corner_a, &edge_b);
    mag_a = PSVECMag(&edge_a);
    owner->shadowbox->scale.x = mag_a;
    mag_b = PSVECMag(&edge_b);
    owner->shadowbox->scale.z = kHalf * mag_b;
    gc_enable_alpha_writes(0);
}

void init_shadow(ShadowObject* shadow, MkObj* object) {
    PlyrPdata* owner = (PlyrPdata*)shadow;
    RpAtomic* atomic;

    if (shadow != NULL) {
        RpClumpForAllAtomics(object->clump, shadow_getFirstAtomic, &atomic);
        if (atomic->interpolator.flags & 2) {
            _rpAtomicResyncInterpolatedSphere(atomic);
        }
        owner->shadow_sphere = atomic->boundingSphere;
        owner->shadow_ground_radius = owner->shadow_sphere.radius;
        RwV3dTransformPoints(
            (RwV3d*)&owner->shadow_ground_point, (const RwV3d*)&object->pos.value, 1,
            &object->frame->modelling);
    }
    if (SetupShadow(shadow) == 0) {
        if (owner->shadow_raster != NULL) {
            RwRasterDestroy(owner->shadow_raster);
            owner->shadow_raster = NULL;
        }
        if (owner->shadow_texture != NULL) {
            owner->shadow_texture->raster = NULL;
            RwTextureDestroy(owner->shadow_texture);
            owner->shadow_texture = NULL;
        }
        shadow_destroy_shadowbox(&owner->shadowbox);
    }
}
