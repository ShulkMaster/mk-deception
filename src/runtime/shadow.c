#include "runtime/shadow.h"

#include "math/gxQuat.h"
#include "math/gxVect.h"
#include "platform/gcutils.h"
#include "runtime/mk_struct.h"
#include "rw/rpworld_types.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct ShadowMatrix {
    float right[4];
    float up[4];
    float at[4];
    float pos[4];
} ShadowMatrix;

/* RwFrame modelling matrix @ +0x10 (stock RW). */
typedef struct ShadowFrame {
    char pad[0x10];
    ShadowMatrix modelling; /* +0x10 */
} ShadowFrame;

typedef struct RwRGBA {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} RwRGBA;

typedef struct RwEngineInstanceType {
    char pad[0x1C];
    float field_1C;
    char pad2[0x10];
    void (*fpIm2DRenderIndexedPrimitive)(int primType, void* vertices, int numVertices);
} RwEngineInstanceType;

typedef struct RwCamera {
    char object[0x4];
    ShadowFrame* frame; /* +0x04 */
    char pad[0x58];
    void* raster_target; /* +0x60 */
    void* z_raster;      /* +0x64 */
    char pad68[0x1C];
    float view_window_scale; /* +0x84 */
} RwCamera;

/* Raster height @ +0x0C (used for Im2D aspect). */
typedef struct ShadowRaster {
    char pad[0x0C];
    int height; /* +0x0C */
} ShadowRaster;

typedef struct FighterState FighterState;

typedef struct ShadowLightPair {
    FighterState* primary;         /* +0x00 */
    unsigned int primary_id;       /* +0x04 */
    FighterState* secondary;       /* +0x08 */
    unsigned int secondary_id;     /* +0x0C */
    FighterState* tertiary;        /* +0x10 */
    unsigned int tertiary_id;      /* +0x14 */
    FighterState* pair_c;          /* +0x18 */
    unsigned int pair_c_id;        /* +0x1C */
    FighterState* pair_d;          /* +0x20 */
    unsigned int pair_d_id;        /* +0x24 */
} ShadowLightPair;

typedef struct ShadowObject {
    char pad_00[0x40];
    FighterState* fighter_a; /* +0x40 */
    unsigned int fighter_a_id;
    FighterState* fighter_b;
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
    Vec ground_point; /* +0x454 */
    float ground_w; /* +0x460 */
    void* shadow_raster; /* +0x464 */
    void* blur_raster; /* +0x468 */
    RwTexture* shadow_texture; /* +0x46c */
    ShadowboxObject* shadowbox; /* +0x470 */
} ShadowObject;

typedef struct ShadowboxObject {
    MkHdr hdr; /* +0x00 */
    unsigned char flags; /* +0x08 */
    char pad9;
    unsigned char sobj_flags;
    char padB[0x21];
    int sobj_priority;
    char pad30[0x18];
    void* clump;
    char pad4C[0x54]; /* +0x4C -> +0xA0 */
    union {
        Vec position;
        struct {
            float pos_x;
            float ground_y;
            float pos_z;
        };
    }; /* +0xA0 */
    char padAC[0x28];
    float field_D4;
    char padD8[0x18];
    float field_F0;
    float field_F4;
    float field_F8;
} ShadowboxObject;

typedef struct ShadowboxVtable {
    void* field_00;
    void* field_04;
    void* field_08;
    void* field_0C;
    void (*destroy)(void* self);
} ShadowboxVtable;

typedef struct ClumpRenderContext {
    char pad[0x18];
    void* clump;         /* +0x18 */
    void* matrix_holder; /* +0x1C */
    char pad20[0x80];
    Vec world_anchor; /* +0xA0 */
} ClumpRenderContext;

struct FighterState {
    MkHdr hdr; /* +0x00 */
    char pad08[2];
    unsigned char hide_flags; /* +0x0A */
    char pad0B[0x0D];
    void* clump; /* +0x18 -- ShadowCameraUpdate arg */
    char pad1C[0x54];
    float field_70;
    char pad74[0x2C];
    Vec position; /* +0xA0 */
    char padAC[0x120];
    int mode; /* +0x1CC */
};

/* Sobj flags09 @ +0x09, field @ +0x2C (SetupShadow). */
typedef struct ShadowSobj {
    char pad[0x09];
    unsigned char flags09; /* +0x09 */
    char pad0A[0x22];
    int field_2C; /* +0x2C */
} ShadowSobj;

void set_render_state(int state, int value);
void* RpClumpForAllAtomics(void* clump, void* callback, void* data);
void _rpAtomicResyncInterpolatedSphere(void* atomic);
void RwV3dTransformPoints(void* dst, void* src, int count, ShadowMatrix* matrix);
int RwRasterDestroy(RwRaster* raster);
void RwMatrixUpdate(ShadowMatrix* matrix);
void RwFrameUpdateObjects(void* frame);
void RwCameraSetFarClipPlane(void* camera, float distance);
void RwCameraSetNearClipPlane(void* camera, float distance);
void RwCameraSetViewWindow(void* camera, float* width, float* height);
int RwCameraClear(void* camera, RwRGBA* color, int flags);
int RwCameraBeginUpdate(void* camera);
void RwCameraEndUpdate(void* camera);
void RwGameCubeCameraTextureFlush(void* camera, int flags);
void* RwCameraCreate(void);
void* RwFrameCreate(void);
void _rwObjectHasFrameSetFrame(void* object, void* frame);
void RwCameraSetProjection(void* camera, int projection);
void RwFrameDestroy(void* frame);
void RwCameraDestroy(void* camera);
void RwFrameOrthoNormalize(void* frame);
void* RwFrameGetLTM(void* frame);
void YXZ_angles_to_MKMATRIX(const float* angles, ShadowMatrix* matrix);
void* load_model_from_slot_transl(int slot_hi, int slot_lo, int flags);
void insert_fgnd_mkobj(void* model);
RpMaterial* obj_find_material_with_texture(void* model, const char* name);
void obj_create_sobjs(void* model);
void* obj_first_sobj(void* model);
void sobj_set_priority(void* sobj, int priority);
void PSVECAdd(const Vec* a, const Vec* b, Vec* dst);

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

ShadowMatrix ShadowDirectionMatrix;

int ShadowCameraUpdate_flag;
static unsigned char colorgray;
RwCamera* ShadowCamera;
void* ShadowCameraRaster;
RwCamera* ShadowIPCamera;
void* ShadowRasterAA;
unsigned int save_res_for_shadowbox;

float ShadowStrength;

static int shadow_getFirstAtomic(void* atomic, void* out);
static void Im2DRenderQuad(unsigned char alpha, float p1, float p2, float p3, float p4, float depth, float p6,
                           float p7);
static FighterState* shadow_validate_fighter(FighterState* fighter, unsigned int expected_id);
static int shadow_fighter_visible(FighterState* fighter);
static void shadow_destroy_shadowbox(ShadowboxObject** box_ptr);

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
    float w;
} Im2DVertex;

static FighterState* shadow_validate_fighter(FighterState* fighter, unsigned int expected_id) {
    if (fighter == NULL) {
        return NULL;
    }
    if (fighter->hdr.instance != expected_id) {
        return NULL;
    }
    return fighter;
}

static int shadow_fighter_visible(FighterState* fighter) {
    if (fighter == NULL) {
        return 0;
    }
    if ((fighter->hide_flags >> 1) & 1) {
        return 0;
    }
    return 1;
}

static void shadow_destroy_shadowbox(ShadowboxObject** box_ptr) {
    ShadowboxObject* box;
    ShadowboxVtable* vtable;

    box = *box_ptr;
    if (box == NULL) {
        return;
    }
    if (box->hdr.instance != 0) {
        vtable = (ShadowboxVtable*)box->hdr.vtbl;
        vtable->destroy(box);
    }
    *box_ptr = NULL;
}

int init_shadow(void* shadow_ptr, void* clump_ptr) {
    ShadowObject* shadow;
    RpClump* clump;
    RpAtomic* atomic;
    int result;

    shadow = shadow_ptr;
    clump = clump_ptr;
    if (clump != NULL) {
        atomic = NULL;
        RpClumpForAllAtomics(clump->atomics, shadow_getFirstAtomic, &atomic);
        if (atomic != NULL) {
            if (atomic->interpolatorFlags & 2) {
                _rpAtomicResyncInterpolatedSphere(atomic);
            }
            shadow->sphere_x = atomic->boundingSphereX;
            shadow->sphere_y = atomic->boundingSphereY;
            shadow->sphere_z = atomic->boundingSphereZ;
            shadow->sphere_w = atomic->boundingSphereRadius;
            shadow->ground_w = shadow->sphere_w;
            RwV3dTransformPoints(&shadow->ground_point, &clump->worldAnchorX, 1,
                                 &((ShadowFrame*)clump->modellingFrame)->modelling);
        }
    }
    result = SetupShadow(shadow);
    if (result == 0) {
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
    return result;
}

void UpdateShadow(void* fighter_ptr, void* shadow_ptr, void* ltm_ptr) {
    FighterState* fighter;
    ShadowObject* shadow;
    ClumpRenderContext* ctx;
    RwCamera* camera;
    ShadowMatrix* frame_matrix;
    ShadowMatrix* dir_matrix;
    ShadowLightPair* lights;
    FighterState* validated;
    float shadow_scale;
    float view_window;
    RwCamera* ip_camera;
    ShadowRaster* src_raster;
    void* dst_raster;
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

    fighter = fighter_ptr;
    shadow = shadow_ptr;
    ctx = ltm_ptr;
    shadow_scale = kShadowScaleDefault;
    if (fighter->mode == 0x1D) {
        shadow_scale = kShadowScaleAlt;
    }
    gc_enable_alpha_writes(1);
    RwV3dTransformPoints(&shadow->ground_point, &ctx->world_anchor, 1,
                         &((ShadowFrame*)ctx->matrix_holder)->modelling);
    camera = ShadowCamera;
    dir_matrix = &ShadowDirectionMatrix;
    frame_matrix = &camera->frame->modelling;
    frame_matrix->right[0] = dir_matrix->right[0];
    frame_matrix->right[1] = dir_matrix->right[1];
    frame_matrix->right[2] = dir_matrix->right[2];
    frame_matrix->up[0] = dir_matrix->up[0];
    frame_matrix->up[1] = dir_matrix->up[1];
    frame_matrix->up[2] = dir_matrix->up[2];
    frame_matrix->at[0] = dir_matrix->at[0];
    frame_matrix->at[1] = dir_matrix->at[1];
    frame_matrix->at[2] = dir_matrix->at[2];
    RwMatrixUpdate(frame_matrix);
    RwFrameUpdateObjects(camera->frame);
    RwCameraSetFarClipPlane(camera, kFarClipMul * shadow_scale);
    RwCameraSetNearClipPlane(camera, kNearClipMul * shadow_scale);
    view_window = shadow_scale;
    RwCameraSetViewWindow(camera, &view_window, &view_window);
    frame_matrix = &camera->frame->modelling;
    frame_matrix->pos[0] = fighter->position.x;
    frame_matrix->pos[1] = fighter->position.y;
    frame_matrix->pos[2] = fighter->position.z;
    frame_matrix->pos[0] = frame_matrix->pos[0] + frame_matrix->at[0] * (kViewWindowBias * camera->view_window_scale);
    frame_matrix->pos[1] = frame_matrix->pos[1] + frame_matrix->up[1] * (kViewWindowBias * camera->view_window_scale);
    frame_matrix->pos[2] = frame_matrix->pos[2] + frame_matrix->at[2] * (kViewWindowBias * camera->view_window_scale);
    RwMatrixUpdate(frame_matrix);
    RwFrameUpdateObjects(camera->frame);
    ShadowCameraUpdate_flag = 1;
    ShadowCameraUpdate(camera, ctx->clump, 1);
    lights = shadow->light_pair;
    if (lights != NULL) {
        validated = shadow_validate_fighter(lights->primary, lights->primary_id);
        if (validated != NULL && shadow_fighter_visible(validated)) {
            validated = shadow_validate_fighter(lights->secondary, lights->secondary_id);
            if (validated != NULL) {
                ShadowCameraUpdate(ShadowCamera, validated->clump, 0);
            }
        }
        validated = shadow_validate_fighter(lights->pair_c, lights->pair_c_id);
        if (validated != NULL && shadow_fighter_visible(validated)) {
            validated = shadow_validate_fighter(lights->pair_d, lights->pair_d_id);
            if (validated != NULL) {
                ShadowCameraUpdate(ShadowCamera, validated->clump, 0);
            }
        }
    }
    validated = shadow_validate_fighter(shadow->fighter_a, shadow->fighter_a_id);
    if (validated != NULL && shadow_fighter_visible(validated)) {
        validated = shadow_validate_fighter(shadow->fighter_b, shadow->fighter_b_id);
        if (validated != NULL) {
            ShadowCameraUpdate(ShadowCamera, validated->clump, 0);
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
        aspect = kOne / ip_camera->view_window_scale;
        ip_camera->raster_target = src_raster;
        RwCameraClear(ip_camera, &clear_color_black, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(0xA, 2);
            set_render_state(0xB, 1);
            set_render_state(0x6, 0);
            set_render_state(0x2, 3);
            set_render_state(0x9, 2);
            set_render_state(1, (int)ShadowCameraRaster);
            Im2DRenderQuad(0xFF, kZero, kZero, inv_height, inv_height,
                           RwEngineInstance->field_1C, aspect, kHalf / inv_height);
            set_render_state(0x6, 1);
            set_render_state(0xA, 5);
            set_render_state(0xB, 6);
            RwCameraEndUpdate(ip_camera);
            RwGameCubeCameraTextureFlush(ip_camera->raster_target, 0);
        }
        ip_camera->raster_target = NULL;
        shadow->blur_raster = ShadowRasterAA;
    } else {
        shadow->blur_raster = ShadowCameraRaster;
    }
    if (ShadowBlur != 0) {
        ShadowRasterBlur(shadow->shadow_raster, shadow->blur_raster, ShadowIPCamera);
    }
    dir_matrix = &ShadowDirectionMatrix;
    plane_normal.x = kZero;
    plane_normal.y = kOne;
    plane_normal.z = kZero;
    plane_point.x = kZero;
    plane_point.y = fighter->field_70;
    plane_point.z = kZero;
    light_pos.x = fighter->position.x;
    light_pos.y = fighter->position.y;
    light_pos.z = fighter->position.z;
    light_dir.x = dir_matrix->at[0];
    light_dir.y = dir_matrix->at[1];
    light_dir.z = dir_matrix->at[2];
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
    box->pos_x = work_b.x;
    box->pos_z = work_b.z;
    box->field_D4 = gxVectAngleZX(&light_dir) - kPi;
    work_c.x = (dir_matrix->up[0] - dir_matrix->right[0]) * shadow_scale;
    work_c.y = (dir_matrix->up[1] - dir_matrix->right[1]) * shadow_scale;
    work_c.z = (dir_matrix->up[2] - dir_matrix->right[2]) * shadow_scale;
    PSVECAdd(&light_pos, &work_c, &corner_a);
    angle = -PSVECDotProduct(&plane_normal, &corner_a) * proj_scale;
    work_a.x = angle * light_dir.x;
    work_a.y = angle * light_dir.y;
    work_a.z = angle * light_dir.z;
    PSVECAdd(&corner_a, &work_a, &corner_b);
    corner_c.x = (dir_matrix->right[0] + dir_matrix->up[0]) * shadow_scale;
    corner_c.y = (dir_matrix->right[1] + dir_matrix->up[1]) * shadow_scale;
    corner_c.z = (dir_matrix->right[2] + dir_matrix->up[2]) * shadow_scale;
    PSVECAdd(&light_pos, &corner_c, &corner_d);
    angle = -PSVECDotProduct(&plane_normal, &corner_d) * proj_scale;
    work_a.x = angle * light_dir.x;
    work_a.y = angle * light_dir.y;
    work_a.z = angle * light_dir.z;
    PSVECAdd(&corner_d, &work_a, &offset);
    work_c.x = (-dir_matrix->up[0] - dir_matrix->right[0]) * shadow_scale;
    work_c.y = (-dir_matrix->up[1] - dir_matrix->right[1]) * shadow_scale;
    work_c.z = (-dir_matrix->up[2] - dir_matrix->right[2]) * shadow_scale;
    PSVECAdd(&light_pos, &work_c, &corner_a);
    angle = -PSVECDotProduct(&plane_normal, &corner_a) * proj_scale;
    work_a.x = angle * light_dir.x;
    work_a.y = angle * light_dir.y;
    work_a.z = angle * light_dir.z;
    PSVECAdd(&corner_a, &work_a, &corner_c);
    PSVECSubtract(&corner_b, &offset, &work_a);
    PSVECSubtract(&corner_c, &offset, &work_b);
    mag_a = PSVECMag(&work_a);
    box->field_F0 = mag_a;
    mag_b = PSVECMag(&work_b);
    box->field_F8 = kHalf * mag_b;
    gc_enable_alpha_writes(0);
}

int UpdateShadowCameraLightSource(const float* angles) {
    YXZ_angles_to_MKMATRIX(angles, &ShadowDirectionMatrix);
    return 1;
}

static void shadow_destroy_camera(RwCamera** camera_ptr) {
    RwCamera* camera;
    void* frame;

    camera = *camera_ptr;
    if (camera == NULL) {
        return;
    }
    frame = camera->frame;
    if (frame != NULL) {
        _rwObjectHasFrameSetFrame(camera, NULL);
        RwFrameDestroy(frame);
    }
    if (camera->z_raster != NULL) {
        camera->z_raster = NULL;
        RwRasterDestroy(camera->z_raster);
    }
    if (camera->raster_target != NULL) {
        camera->raster_target = NULL;
    }
    RwCameraDestroy(camera);
    *camera_ptr = NULL;
}

static RwCamera* shadow_create_camera(int resolution) {
    RwCamera* camera;
    void* frame;
    void* raster;

    camera = RwCameraCreate();
    if (camera != NULL) {
        frame = RwFrameCreate();
        _rwObjectHasFrameSetFrame(camera, frame);
        if (camera->frame != NULL) {
            raster = RwRasterCreate(resolution, resolution, 0, 1);
            if (raster != NULL) {
                camera->z_raster = raster;
                RwCameraSetProjection(camera, 2);
                return camera;
            }
        }
        frame = camera->frame;
        if (frame != NULL) {
            _rwObjectHasFrameSetFrame(camera, NULL);
            RwFrameDestroy(frame);
        }
        raster = camera->z_raster;
        if (raster != NULL) {
            camera->z_raster = NULL;
            RwRasterDestroy(raster);
        }
        if (camera->raster_target != NULL) {
            camera->raster_target = NULL;
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
    ShadowboxVtable* vtable;

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
        if (box->hdr.instance != 0) {
            vtable = (ShadowboxVtable*)box->hdr.vtbl;
            vtable->destroy(box);
        }
        shadow->shadowbox = 0;
    }
}

void shadow_set_new_ground_plane(ShadowObject* shadow, ShadowboxObject* ground,
                                 float y) {
    ShadowboxObject* box;

    box = shadow->shadowbox;
    if (box != 0) {
        box->ground_y = kGroundOffset + y;
    }
    if (ground == 0) {
        return;
    }
    ground->ground_y = y;
}

int SetupShadow(void* shadow_ptr) {
    ShadowObject* shadow;
    RpMaterial* material;
    RwTexture* texture;
    ShadowSobj* sobj;
    unsigned int filter;
    unsigned int flags;
    ShadowboxObject* box;

    shadow = shadow_ptr;
    shadow->shadow_raster = RwRasterCreate(save_res_for_shadowbox, 0x20, 0, 0x505);
    if (shadow->shadow_raster == NULL) {
        return 0;
    }
    if (shadow->mode == 0x1D) {
        if (shadow->alt_shadowbox == 0) {
            shadow->shadowbox = load_model_from_slot_transl(0x0003000B, 0x008F0002, 0x5012);
        } else {
            shadow->shadowbox = load_model_from_slot_transl(0x0004000B, 0x008F0002, 0x5012);
        }
    } else {
        shadow->shadowbox = load_model_from_slot_transl(0, 0x0001000A, 0x5012);
    }
    if (shadow->shadowbox == NULL) {
        return 0;
    }
    box = shadow->shadowbox;
    flags = box->flags;
    flags = (flags & ~(1 << 6)) | (1 << 6);
    box->flags = (unsigned char)flags;
    flags = box->flags;
    flags = (flags & ~(1 << 3)) | (1 << 3);
    box->flags = (unsigned char)flags;
    flags = box->flags;
    flags = (flags & ~(1 << 1)) | (1 << 1);
    box->flags = (unsigned char)flags;
    insert_fgnd_mkobj(box);
    if (shadow->mode == 0x1D) {
        material = obj_find_material_with_texture(box, stringBase0);
    } else {
        material = obj_find_material_with_texture(box, stringBase0 + 8);
    }
    if (material != NULL) {
        texture = RwTextureCreate(shadow->shadow_raster);
        material->texture = texture;
        if (material->texture != NULL) {
            filter = texture->filter_flags;
            filter = (filter & ~0xFF) | 2;
            texture->filter_flags = filter;
            filter = texture->filter_flags;
            filter = (filter & 0xFF00FFFF) | 0x330000;
            texture->filter_flags = filter;
            shadow->shadow_texture = material->texture;
        }
    }
    obj_create_sobjs(box);
    sobj = obj_first_sobj(box);
    if (sobj != NULL) {
        flags = sobj->flags09;
        flags = (flags & ~(1 << 7)) | (1 << 7);
        sobj->flags09 = (unsigned char)flags;
        sobj->field_2C = 0x10006;
        sobj_set_priority(sobj, 0xC);
    }
    box->ground_y = kGroundOffset;
    box->field_F4 = kOne;
    return 1;
}

int init_shadow_system(void) {
    RwCamera* camera;
    ShadowMatrix* frame_matrix;
    ShadowMatrix* dir_matrix;
    int resolution;
    int aa_resolution;
    void* raster;

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
    frame_matrix = &camera->frame->modelling;
    frame_matrix->right[0] = dir_matrix->right[0];
    frame_matrix->right[1] = dir_matrix->right[1];
    frame_matrix->right[2] = dir_matrix->right[2];
    frame_matrix->up[0] = dir_matrix->up[0];
    frame_matrix->up[1] = dir_matrix->up[1];
    frame_matrix->up[2] = dir_matrix->up[2];
    frame_matrix->at[0] = dir_matrix->at[0];
    frame_matrix->at[1] = dir_matrix->at[1];
    frame_matrix->at[2] = dir_matrix->at[2];
    RwMatrixUpdate(frame_matrix);
    RwFrameUpdateObjects(camera->frame);
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
    ShadowCamera->raster_target = raster;
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

static int shadow_getFirstAtomic(void* atomic, void* out) {
    *(void**)out = atomic;
    return 0;
}

void ShadowRasterBlur(void* src_raster, void* dst_raster, void* ip_camera_ptr) {
    RwCamera* ip_camera;
    ShadowRaster* src;
    int pass;
    int last_pass;
    union {
        double d;
        int i[2];
    } conv;
    float inv_height;
    float aspect;
    int alpha;

    ip_camera = ip_camera_ptr;
    src = src_raster;
    conv.i[0] = 0x43300000;
    conv.i[1] = src->height ^ 0x80000000;
    inv_height = (float)(conv.d - kFloatConvBias);
    aspect = kOne / inv_height;
    last_pass = ShadowBlur - 1;
    for (pass = 0; pass < ShadowBlur; pass++) {
        ip_camera->raster_target = dst_raster;
        RwCameraClear(ip_camera, &clear_color_white, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(0xA, 2);
            set_render_state(0xB, 1);
            set_render_state(0x6, 0);
            set_render_state(0x9, 2);
            set_render_state(0x2, 3);
            set_render_state(1, (int)src_raster);
            Im2DRenderQuad(0xFF, kZero, kZero, aspect, aspect, RwEngineInstance->field_1C, kOne, kOne);
            RwCameraEndUpdate(ip_camera);
            RwGameCubeCameraTextureFlush(ip_camera, 0);
        }
        ip_camera->raster_target = src_raster;
        RwCameraClear(ip_camera, &clear_color_white, 3);
        if (RwCameraBeginUpdate(ip_camera) != 0) {
            set_render_state(1, (int)dst_raster);
            if (pass < last_pass) {
                Im2DRenderQuad(0xFF, kZero, kZero, aspect, aspect, RwEngineInstance->field_1C, kOne, kOne);
            } else {
                alpha = (int)(kAlphaScale * ShadowStrength);
                Im2DRenderQuad((unsigned char)alpha, kZero, kZero, aspect, aspect, RwEngineInstance->field_1C, kOne,
                               kOne);
            }
            set_render_state(0x6, 1);
            set_render_state(0xA, 5);
            set_render_state(0xB, 6);
            RwCameraEndUpdate(ip_camera);
            RwGameCubeCameraTextureFlush(ip_camera, 0);
        }
    }
    ip_camera->raster_target = NULL;
}

void ShadowCameraUpdate(void* camera_ptr, void* clump_ptr, int clear) {
    RwCamera* camera;
    RpClump* clump;
    RwLLLink* node;
    RwLLLink* end;
    RpAtomic* atomic;
    RpGeometry* geometry;
    unsigned int saved_flags;

    camera = camera_ptr;
    clump = clump_ptr;
    if (clear != 0) {
        RwCameraClear(camera, &clear_color_black, 3);
    }
    RwFrameOrthoNormalize(camera->frame);
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
        atomic = RP_ATOMIC_FROM_CLUMP_LINK(node);
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
    RwGameCubeCameraTextureFlush(camera, 0);
}

static void Im2DRenderQuad(unsigned char alpha, float p1, float p2, float p3, float p4, float depth, float p6,
                           float p7) {
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
    vertices[0].x = p3;
    vertices[0].y = p4;
    vertices[0].w = p7;
    vertices[1].u = p1;
    vertices[1].v = v_top;
    vertices[1].z = depth;
    vertices[1].r = colorgray;
    vertices[1].g = colorgray;
    vertices[1].b = colorgray;
    vertices[1].a = alpha;
    vertices[1].x = p3;
    vertices[1].y = p6;
    vertices[1].w = p7;
    vertices[2].u = p6;
    vertices[2].v = p2;
    vertices[2].z = depth;
    vertices[2].r = colorgray;
    vertices[2].g = colorgray;
    vertices[2].b = colorgray;
    vertices[2].a = alpha;
    vertices[2].x = p4;
    vertices[2].y = p4;
    vertices[2].w = p7;
    vertices[3].u = p6;
    vertices[3].v = v_top;
    vertices[3].z = depth;
    vertices[3].r = colorgray;
    vertices[3].g = colorgray;
    vertices[3].b = colorgray;
    vertices[3].a = alpha;
    vertices[3].x = p4;
    vertices[3].y = p6;
    vertices[3].w = p7;
    RwEngineInstance->fpIm2DRenderIndexedPrimitive(4, vertices, 4);
}
