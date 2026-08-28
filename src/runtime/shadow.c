#include "runtime/shadow.h"

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
#include "rw/rwframe.h"
#include "rw/rwvector.h"

typedef struct ShadowFighterObject ShadowFighterObject;

typedef struct ShadowLightPair {
    ShadowFighterObject* primary;  /* +0x00 */
    unsigned int primary_id;       /* +0x04 */
    ShadowFighterObject* secondary; /* +0x08 */
    unsigned int secondary_id;     /* +0x0C */
    unsigned char unknown_10[8];   /* +0x10 */
    ShadowFighterObject* pair_c;   /* +0x18 */
    unsigned int pair_c_id;        /* +0x1C */
    ShadowFighterObject* pair_d;   /* +0x20 */
    unsigned int pair_d_id;        /* +0x24 */
} ShadowLightPair;

typedef struct ShadowObject {
    char pad_00[0x40];
    ShadowFighterObject* fighter_a; /* +0x40 */
    unsigned int fighter_a_id;
    ShadowFighterObject* fighter_b;
    unsigned int fighter_b_id;
    char pad_50[0x17C]; /* +0x50 -> +0x1CC */
    int mode; /* +0x1CC */
    int alt_shadowbox; /* +0x1D0 */
    char pad_1D4[0x13C]; /* +0x1D4 -> +0x310 */
    ShadowLightPair* light_pair; /* +0x310 */
    char pad_314[0x130]; /* +0x314 -> +0x444 */
    float sphere_x; /* +0x444 */
    float sphere_y;
    float sphere_z;
    float sphere_w; /* +0x450 */
    RwV3d ground_point; /* +0x454 */
    float ground_w; /* +0x460 */
    RwRaster* shadow_raster; /* +0x464 */
    RwRaster* blur_raster; /* +0x468 */
    RwTexture* shadow_texture; /* +0x46c */
    ShadowboxObject* shadowbox; /* +0x470 */
} ShadowObject;

typedef struct ShadowboxObject {
    MkObj object;
} ShadowboxObject;

struct ShadowFighterObject {
    MkObj object; /* +0x00 */
    char field_100[0xCC];
    int mode; /* +0x1CC */
};

extern RwEngineInstanceType* RwEngineInstance;

static const char stringBase0[] = "Shadow2\0SHADOWBOX\0";

static RwRGBA clear_color_white = {0xFF, 0xFF, 0xFF, 0x00};
static RwRGBA clear_color_black = {0x00, 0x00, 0x00, 0xFF};

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
static const double kFloatConvBias = 4503601774854144.0;

int ShadowAA = 1;
int ShadowBlur = 1;
int ShadowResolutionIndex = 8;

RwMatrix ShadowDirectionMatrix;

int ShadowCameraUpdate_flag;
static unsigned char colorgray;
RwCamera* ShadowCamera;
RwRaster* ShadowCameraRaster;
RwCamera* ShadowIPCamera;
RwRaster* ShadowRasterAA;
unsigned int save_res_for_shadowbox;

float ShadowStrength;

static RpAtomic* shadow_getFirstAtomic(RpAtomic* atomic, void* out);
static int Im2DRenderQuad(unsigned char alpha, float p1, float p2, float p3,
                          float p4, float depth, float p6, float p7);
static inline ShadowFighterObject* shadow_validate_fighter(
    ShadowFighterObject* fighter, unsigned int expected_id);
static inline int shadow_fighter_visible(ShadowFighterObject* fighter);
static inline void shadow_destroy_shadowbox(ShadowboxObject** box_ptr);

typedef struct Im2DVertex {
    float u;
    float v;
    float z;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    float x;
    float y;
} Im2DVertex;

static inline ShadowFighterObject* shadow_validate_fighter(
    ShadowFighterObject* fighter, unsigned int expected_id) {
    if (fighter == NULL) {
        return NULL;
    }
    if (fighter->object.hdr.instance != expected_id) {
        return NULL;
    }
    return fighter;
}

static inline int shadow_fighter_visible(ShadowFighterObject* fighter) {
    if (fighter == NULL) {
        return 0;
    }
    if (fighter->object.hide_flag_bits.hidden) {
        return 0;
    }
    return 1;
}

static inline void shadow_destroy_shadowbox(ShadowboxObject** box_ptr) {
    ShadowboxObject* box;

    box = *box_ptr;
    if (box == NULL) {
        return;
    }
    if (box->object.hdr.instance != 0) {
        box->object.hdr.typed_vtbl->destroy((MkHdr*)box);
    }
    *box_ptr = NULL;
}

void init_shadow(ShadowObject* shadow, MkObj* object) {
    RpAtomic* atomic;

    if (shadow != NULL) {
        RpClumpForAllAtomics(object->clump, shadow_getFirstAtomic, &atomic);
        if (atomic->interpolator.flags & 2) {
            _rpAtomicResyncInterpolatedSphere(atomic);
        }
        shadow->sphere_x = atomic->boundingSphere.center.x;
        shadow->sphere_y = atomic->boundingSphere.center.y;
        shadow->sphere_z = atomic->boundingSphere.center.z;
        shadow->sphere_w = atomic->boundingSphere.radius;
        shadow->ground_w = shadow->sphere_w;
        RwV3dTransformPoints(
            &shadow->ground_point, (const RwV3d*)&object->pos.value, 1,
            &object->frame->modelling);
    }
    if (SetupShadow(shadow) == 0) {
        if (shadow->shadow_raster != NULL) {
            RwRasterDestroy(shadow->shadow_raster);
            shadow->shadow_raster = NULL;
        }
        if (shadow->shadow_texture != NULL) {
            shadow->shadow_texture->raster = NULL;
            RwTextureDestroy(shadow->shadow_texture);
            shadow->shadow_texture = NULL;
        }
        shadow_destroy_shadowbox(&shadow->shadowbox);
    }
}

void UpdateShadow(MkObj* fighter_object, ShadowObject* shadow, MkObj* object) {
    ShadowFighterObject* fighter;
    RwCamera* camera;
    RwMatrix* frame_matrix;
    RwMatrix* dir_matrix;
    ShadowLightPair* lights;
    ShadowFighterObject* validated;
    float shadow_scale;
    RwV2d view_window;
    RwCamera* ip_camera;
    RwRaster* src_raster;
    RwRaster* dst_raster;
    ShadowboxObject* box;
    Vec plane_normal;
    Vec plane_point;
    Vec light_dir;
    Vec light_pos;
    Vec light_at;
    Vec delta_pos;
    Vec delta_at;
    Vec work_a;
    Vec work_b;
    Vec work_c;
    Vec corner_a;
    Vec corner_b;
    Vec corner_c;
    Vec corner_d;
    Vec offset;
    float proj_scale;
    float angle;
    float mag_a;
    float mag_b;
    int clear_flags;
    union {
        double d;
        int i[2];
    } conv;
    float aspect;
    float inv_height;

    fighter = (ShadowFighterObject*)fighter_object;
    shadow_scale = kShadowScaleDefault;
    if (fighter->mode == 0x1D) {
        shadow_scale = kShadowScaleAlt;
    }
    gc_enable_alpha_writes(1);
    RwV3dTransformPoints(
        &shadow->ground_point, (const RwV3d*)&object->pos.value, 1,
        &object->frame->modelling);
    camera = ShadowCamera;
    dir_matrix = &ShadowDirectionMatrix;
    frame_matrix = &RwCameraGetFrame(camera)->modelling;
    frame_matrix->right = dir_matrix->right;
    frame_matrix->up = dir_matrix->up;
    frame_matrix->at = dir_matrix->at;
    RwMatrixUpdate(frame_matrix);
    RwFrameUpdateObjects(RwCameraGetFrame(camera));
    RwCameraSetFarClipPlane(camera, kFarClipMul * shadow_scale);
    RwCameraSetNearClipPlane(camera, kNearClipMul * shadow_scale);
    view_window.x = shadow_scale;
    view_window.y = shadow_scale;
    RwCameraSetViewWindow(camera, &view_window);
    frame_matrix = &RwCameraGetFrame(camera)->modelling;
    frame_matrix->pos.x = fighter->object.pos.value.x;
    frame_matrix->pos.y = fighter->object.pos.value.y;
    frame_matrix->pos.z = fighter->object.pos.value.z;
    frame_matrix->pos.x = frame_matrix->pos.x + frame_matrix->at.x * (kViewWindowBias * camera->farPlane);
    frame_matrix->pos.y = frame_matrix->pos.y + frame_matrix->at.y * (kViewWindowBias * camera->farPlane);
    frame_matrix->pos.z = frame_matrix->pos.z + frame_matrix->at.z * (kViewWindowBias * camera->farPlane);
    RwMatrixUpdate(frame_matrix);
    RwFrameUpdateObjects(RwCameraGetFrame(camera));
    ShadowCameraUpdate_flag = 1;
    ShadowCameraUpdate(camera, object->clump, 1);
    lights = shadow->light_pair;
    if (lights != NULL) {
        validated = shadow_validate_fighter(lights->primary, lights->primary_id);
        if (validated != NULL && shadow_fighter_visible(validated)) {
            validated = shadow_validate_fighter(lights->secondary, lights->secondary_id);
            if (validated != NULL) {
                ShadowCameraUpdate(ShadowCamera, validated->object.clump, 0);
            }
        }
        validated = shadow_validate_fighter(lights->pair_c, lights->pair_c_id);
        if (validated != NULL && shadow_fighter_visible(validated)) {
            validated = shadow_validate_fighter(lights->pair_d, lights->pair_d_id);
            if (validated != NULL) {
                ShadowCameraUpdate(ShadowCamera, validated->object.clump, 0);
            }
        }
    }
    validated = shadow_validate_fighter(shadow->fighter_a, shadow->fighter_a_id);
    if (validated != NULL && shadow_fighter_visible(validated)) {
        validated = shadow_validate_fighter(shadow->fighter_b, shadow->fighter_b_id);
        if (validated != NULL) {
            ShadowCameraUpdate(ShadowCamera, validated->object.clump, 0);
        }
    }
    clear_flags = ShadowAA;
    ShadowCameraUpdate_flag = 0;
    if (clear_flags != 0) {
        ip_camera = ShadowIPCamera;
        src_raster = shadow->shadow_raster;
        conv.i[0] = 0x43300000;
        conv.i[1] = src_raster->height ^ 0x80000000;
        inv_height = (float)(conv.d - kFloatConvBias);
        aspect = kOne / ip_camera->farPlane;
        ip_camera->frameBuffer = src_raster;
        RwCameraClear(ip_camera, &clear_color_black, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(0xA, 2);
            set_render_state(0xB, 1);
            set_render_state(0x6, 0);
            set_render_state(0x2, 3);
            set_render_state(0x9, 2);
            set_render_state(1, (int)ShadowCameraRaster);
            Im2DRenderQuad(0xFF, kZero, kZero, inv_height, inv_height,
                           RwEngineInstance->im2d_depth, aspect, kHalf / inv_height);
            set_render_state(0x6, 1);
            set_render_state(0xA, 5);
            set_render_state(0xB, 6);
            RwCameraEndUpdate(ip_camera);
            RwGameCubeCameraTextureFlush(ip_camera->frameBuffer, 0);
        }
        ip_camera->frameBuffer = NULL;
        shadow->blur_raster = ShadowRasterAA;
    } else {
        shadow->blur_raster = ShadowCameraRaster;
    }
    if (ShadowBlur != 0) {
        ShadowRasterBlur(shadow->shadow_raster, shadow->blur_raster,
                         ShadowIPCamera, ShadowBlur);
    }
    dir_matrix = &ShadowDirectionMatrix;
    plane_normal.x = kZero;
    plane_normal.y = kOne;
    plane_normal.z = kZero;
    plane_point.x = kZero;
    plane_point.y = fighter->object.ground_colls_y;
    plane_point.z = kZero;
    light_pos.x = fighter->object.pos.value.x;
    light_pos.y = fighter->object.pos.value.y;
    light_pos.z = fighter->object.pos.value.z;
    light_dir.x = dir_matrix->at.x;
    light_dir.y = dir_matrix->at.y;
    light_dir.z = dir_matrix->at.z;
    delta_pos.x = light_pos.x - plane_point.x;
    delta_pos.y = light_pos.y - plane_point.y;
    delta_pos.z = light_pos.z - plane_point.z;
    proj_scale = kOne / PSVECDotProduct(&plane_normal, &light_dir);
    angle = -PSVECDotProduct(&plane_normal, &delta_pos) * proj_scale;
    work_a.x = angle * light_dir.x;
    work_a.y = angle * light_dir.y;
    work_a.z = angle * light_dir.z;
    PSVECAdd(&light_pos, &work_a, &work_b);
    box = shadow->shadowbox;
    box->object.pos.value.x = work_b.x;
    box->object.pos.value.z = work_b.z;
    box->object.ang.y = gxVectAngleZX(&light_dir) - kPi;
    work_c.x = (dir_matrix->up.x - dir_matrix->right.x) * shadow_scale;
    work_c.y = (dir_matrix->up.y - dir_matrix->right.y) * shadow_scale;
    work_c.z = (dir_matrix->up.z - dir_matrix->right.z) * shadow_scale;
    PSVECAdd(&light_pos, &work_c, &corner_a);
    angle = -PSVECDotProduct(&plane_normal, &corner_a) * proj_scale;
    work_a.x = angle * light_dir.x;
    work_a.y = angle * light_dir.y;
    work_a.z = angle * light_dir.z;
    PSVECAdd(&corner_a, &work_a, &corner_b);
    corner_c.x = (dir_matrix->right.x + dir_matrix->up.x) * shadow_scale;
    corner_c.y = (dir_matrix->right.y + dir_matrix->up.y) * shadow_scale;
    corner_c.z = (dir_matrix->right.z + dir_matrix->up.z) * shadow_scale;
    PSVECAdd(&light_pos, &corner_c, &corner_d);
    angle = -PSVECDotProduct(&plane_normal, &corner_d) * proj_scale;
    work_a.x = angle * light_dir.x;
    work_a.y = angle * light_dir.y;
    work_a.z = angle * light_dir.z;
    PSVECAdd(&corner_d, &work_a, &offset);
    work_c.x = (-dir_matrix->up.x - dir_matrix->right.x) * shadow_scale;
    work_c.y = (-dir_matrix->up.y - dir_matrix->right.y) * shadow_scale;
    work_c.z = (-dir_matrix->up.z - dir_matrix->right.z) * shadow_scale;
    PSVECAdd(&light_pos, &work_c, &corner_a);
    angle = -PSVECDotProduct(&plane_normal, &corner_a) * proj_scale;
    work_a.x = angle * light_dir.x;
    work_a.y = angle * light_dir.y;
    work_a.z = angle * light_dir.z;
    PSVECAdd(&corner_a, &work_a, &corner_c);
    PSVECSubtract(&corner_b, &offset, &work_a);
    PSVECSubtract(&corner_c, &offset, &work_b);
    mag_a = PSVECMag(&work_a);
    box->object.scale.x = mag_a;
    mag_b = PSVECMag(&work_b);
    box->object.scale.z = kHalf * mag_b;
    gc_enable_alpha_writes(0);
}

int UpdateShadowCameraLightSource(const float* angles) {
    YXZ_angles_to_MKMATRIX((const Vec*)angles, &ShadowDirectionMatrix);
    return 1;
}

static inline void shadow_destroy_camera(RwCamera** camera_ptr) {
    RwCamera* camera;
    void* frame;

    camera = *camera_ptr;
    if (camera == NULL) {
        return;
    }
    frame = RwCameraGetFrame(camera);
    if (frame != NULL) {
        _rwObjectHasFrameSetFrame(camera, NULL);
        RwFrameDestroy(frame);
    }
    if (camera->zBuffer != NULL) {
        camera->zBuffer = NULL;
        RwRasterDestroy(camera->zBuffer);
    }
    if (camera->frameBuffer != NULL) {
        camera->frameBuffer = NULL;
    }
    RwCameraDestroy(camera);
    *camera_ptr = NULL;
}

static inline RwCamera* shadow_create_camera(int resolution) {
    RwCamera* camera;
    void* frame;
    RwRaster* raster;

    camera = RwCameraCreate();
    if (camera != NULL) {
        frame = RwFrameCreate();
        _rwObjectHasFrameSetFrame(camera, frame);
        if (RwCameraGetFrame(camera) != NULL) {
            raster = RwRasterCreate(resolution, resolution, 0, 1);
            if (raster != NULL) {
                camera->zBuffer = raster;
                RwCameraSetProjection(camera, 2);
                return camera;
            }
        }
        frame = RwCameraGetFrame(camera);
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
    return NULL;
}

void destroy_shadow_system(void) {
    shadow_destroy_camera(&ShadowCamera);
    shadow_destroy_camera(&ShadowIPCamera);
    if (ShadowCameraRaster != NULL) {
        RwRasterDestroy(ShadowCameraRaster);
        ShadowCameraRaster = NULL;
    }
    if (ShadowRasterAA != NULL) {
        RwRasterDestroy(ShadowRasterAA);
        ShadowRasterAA = NULL;
    }
}

void TearDownShadow(ShadowObject* shadow) {
    ShadowboxObject* box;

    if (shadow->shadow_raster != 0) {
        RwRasterDestroy(shadow->shadow_raster);
        shadow->shadow_raster = 0;
    }
    if (shadow->shadow_texture != 0) {
        shadow->shadow_texture->raster = NULL;
        RwTextureDestroy(shadow->shadow_texture);
        shadow->shadow_texture = 0;
    }
    box = shadow->shadowbox;
    if (box != 0) {
        if (box->object.hdr.instance != 0) {
            box->object.hdr.typed_vtbl->destroy((MkHdr*)box);
        }
        shadow->shadowbox = 0;
    }
}

void shadow_set_new_ground_plane(ShadowObject* shadow, ShadowboxObject* ground,
                                 float y) {
    ShadowboxObject* box;

    box = shadow->shadowbox;
    if (box != 0) {
        box->object.pos.value.y = kGroundOffset + y;
    }
    if (ground == 0) {
        return;
    }
    ground->object.pos.value.y = y;
}

int SetupShadow(ShadowObject* shadow) {
    RpMaterial* material;
    RwTexture* texture;
    MkSobj* sobj;
    unsigned int filter;
    unsigned int flags;
    ShadowboxObject* box;

    shadow->shadow_raster = RwRasterCreate(save_res_for_shadowbox,
                                            save_res_for_shadowbox, 0x20,
                                            0x505);
    if (shadow->shadow_raster == NULL) {
        return 0;
    }
    if (shadow->mode == 0x1D) {
        if (shadow->alt_shadowbox == 0) {
            shadow->shadowbox = (ShadowboxObject*)load_model_from_slot_transl(
                0x0003000B, 0x008F0002, 0x5012);
        } else {
            shadow->shadowbox = (ShadowboxObject*)load_model_from_slot_transl(
                0x0004000B, 0x008F0002, 0x5012);
        }
    } else {
        shadow->shadowbox = (ShadowboxObject*)load_model_from_slot_transl(
            0, 0x0001000A, 0x5012);
    }
    if (shadow->shadowbox == NULL) {
        return 0;
    }
    box = shadow->shadowbox;
    flags = box->object.flags_08;
    flags = (flags & ~(1 << 6)) | (1 << 6);
    box->object.flags_08 = (unsigned char)flags;
    flags = box->object.flags_08;
    flags = (flags & ~(1 << 3)) | (1 << 3);
    box->object.flags_08 = (unsigned char)flags;
    flags = box->object.flags_08;
    flags = (flags & ~(1 << 1)) | (1 << 1);
    box->object.flags_08 = (unsigned char)flags;
    insert_fgnd_mkobj(box);
    if (shadow->mode == 0x1D) {
        material = obj_find_material_with_texture((MkObj*)box, stringBase0);
    } else {
        material = obj_find_material_with_texture((MkObj*)box, stringBase0 + 8);
    }
    if (material != NULL) {
        texture = RwTextureCreate(shadow->shadow_raster);
        material->texture = texture;
        if (material->texture != NULL) {
            filter = texture->filter_flags;
            filter = (filter & ~0xFF) | 2;
            texture->filter_flags = filter;
            filter = texture->filter_flags;
            filter = (filter & 0xFFFF00FF) | 0x3300;
            texture->filter_flags = filter;
            shadow->shadow_texture = material->texture;
        }
    }
    obj_create_sobjs((MkObj*)box);
    sobj = obj_first_sobj((MkObj*)box);
    if (sobj != NULL) {
        flags = sobj->flags09;
        flags = (flags & ~(1 << 7)) | (1 << 7);
        sobj->flags09 = (unsigned char)flags;
        sobj->render_flags = 0x10006;
        sobj_set_priority(sobj, 0xC);
    }
    box->object.pos.value.y = kGroundOffset;
    box->object.scale.y = kOne;
    return 1;
}

int init_shadow_system(void) {
    RwCamera* camera;
    RwMatrix* frame_matrix;
    RwMatrix* dir_matrix;
    int resolution;
    int aa_resolution;
    RwRaster* raster;

    if (ShadowCamera != NULL) {
        return 1;
    }
    resolution = 1 << ShadowResolutionIndex;
    aa_resolution = resolution;
    if (ShadowAA != 0) {
        aa_resolution = resolution >> 1;
    }
    camera = shadow_create_camera(resolution);
    ShadowCamera = camera;
    if (camera == NULL) {
        return 0;
    }
    dir_matrix = &ShadowDirectionMatrix;
    frame_matrix = &RwCameraGetFrame(camera)->modelling;
    frame_matrix->right = dir_matrix->right;
    frame_matrix->up = dir_matrix->up;
    frame_matrix->at = dir_matrix->at;
    RwMatrixUpdate(frame_matrix);
    RwFrameUpdateObjects(RwCameraGetFrame(camera));
    camera = shadow_create_camera(aa_resolution);
    ShadowIPCamera = camera;
    if (camera == NULL) {
        return 0;
    }
    raster = RwRasterCreate(resolution, resolution, 0x20, 0x505);
    ShadowCameraRaster = raster;
    if (raster == NULL) {
        return 0;
    }
    ShadowCamera->frameBuffer = raster;
    if (ShadowAA != 0 && ShadowRasterAA == NULL) {
        raster = RwRasterCreate(aa_resolution, 0x20, 0, 0x505);
        ShadowRasterAA = raster;
        if (raster == NULL) {
            return 0;
        }
    }
    save_res_for_shadowbox = aa_resolution;
    return 1;
}

static RpAtomic* shadow_getFirstAtomic(RpAtomic* atomic, void* out) {
    *(RpAtomic**)out = atomic;
    return 0;
}

int ShadowRasterBlur(RwRaster* src_raster, RwRaster* dst_raster,
                     RwCamera* ip_camera, unsigned int pass_count) {
    int pass;
    int last_pass;
    union {
        double d;
        int i[2];
    } conv;
    float raster_height;
    float inv_height;
    float inv_far;
    int alpha;

    conv.i[0] = 0x43300000;
    conv.i[1] = src_raster->height ^ 0x80000000;
    raster_height = (float)(conv.d - kFloatConvBias);
    inv_height = kOne / raster_height;
    inv_far = kOne / ip_camera->farPlane;
    last_pass = pass_count - 1;
    for (pass = 0; pass < pass_count; pass++) {
        ip_camera->frameBuffer = dst_raster;
        RwCameraClear(ip_camera, &clear_color_white, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(0xA, 2);
            set_render_state(0xB, 1);
            set_render_state(0x6, 0);
            set_render_state(0x9, 2);
            set_render_state(0x2, 3);
            set_render_state(1, (int)src_raster);
            Im2DRenderQuad(0xFF, kZero, kZero, raster_height,
                           raster_height, RwEngineInstance->im2d_depth, inv_far,
                           inv_height);
            RwCameraEndUpdate(ip_camera);
            RwGameCubeCameraTextureFlush(ip_camera->frameBuffer, 0);
        }
        ip_camera->frameBuffer = src_raster;
        RwCameraClear(ip_camera, &clear_color_white, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(1, (int)dst_raster);
            if (pass < last_pass) {
                Im2DRenderQuad(0xFF, kZero, kZero, raster_height,
                               raster_height, RwEngineInstance->im2d_depth,
                               inv_far, kZero);
            } else {
                alpha = (int)(kAlphaScale * ShadowStrength);
                Im2DRenderQuad((unsigned char)alpha, kZero, kZero,
                               raster_height, raster_height,
                               RwEngineInstance->im2d_depth, inv_far, kZero);
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

void ShadowCameraUpdate(RwCamera* camera, RpClump* clump, int clear) {
    RwLLLink* node;
    RwLLLink* end;
    RpAtomic* atomic;
    RpGeometry* geometry;
    unsigned int saved_flags;

    if (clear != 0) {
        RwCameraClear(camera, &clear_color_black, 3);
    }
    RwFrameOrthoNormalize(RwCameraGetFrame(camera));
    if (RwCameraBeginUpdate(camera) == 0) {
        return;
    }
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
        atomic = RpAtomicFromClumpLink(node);
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

static int Im2DRenderQuad(unsigned char alpha, float p1, float p2, float p3,
                          float p4, float depth, float p6, float p7) {
    Im2DVertex vertices[4];
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
    RwEngineInstance->fpIm2DRenderIndexedPrimitive(4, vertices, 4);
    return 1;
}
