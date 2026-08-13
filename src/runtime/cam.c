#include "runtime/cam.h"

#include "math/gxMath.h"
#include "math/gxQuat.h"
#include "math/mk_math.h"
#include "platform/display.h"
#include "rw/rwcore_types.h"
#include "rw/rwdevice.h"
#include "rw/rpworld_types.h"

/*
 * NonMatching scaffold: this TU exposes many retail camera symbols while only
 * the typed krypt/setup and camera-control paths below are reconstructed.
 * Empty bodies are unresolved placeholders, not claimed retail algorithms.
 */

/* MSB-first bitfields -> rlwimi (retail). */
typedef struct CamPdataFlags {
    unsigned char pos_done : 1;
    unsigned char pad6_3 : 4;
    unsigned char parent_relative : 1;
    unsigned char pad1_0 : 2;
} CamPdataFlags;

typedef struct CamObjFlags {
    unsigned char pad7 : 1;
    unsigned char pad6 : 1;
    unsigned char bit20 : 1;
    unsigned char pad4 : 1;
    unsigned char pad3 : 1;
    unsigned char bit04 : 1;
    unsigned char pad1 : 1;
    unsigned char pad0 : 1;
} CamObjFlags;

typedef struct RwRect {
    int x;
    int y;
    int w;
    int h;
} RwRect;

typedef struct ScriptedCameraData {
    int movement_mode;       /* +0x00 */
    int look_mode;           /* +0x04 */
    Vec movement_offset;     /* +0x08 */
    Vec lookat_offset;       /* +0x14 */
    int glitch;              /* +0x20 */
    float rotation_rate;     /* +0x24 */
    float movement_rate;     /* +0x28 */
    void* movement_focus; /* +0x2C */
    void* lookat_focus;   /* +0x30 */
    int mirror;           /* +0x34 */
    char pad38[0x10];
    int pos_move_done;       /* +0x48 */
    int ang_move_done;       /* +0x4C */
    int check_collisions;    /* +0x50 */
    int custom_movement;     /* +0x54 */
    Vec center_of_rotation;  /* +0x58 */
    float travel_time;       /* +0x64 */
    int rotation_direction;  /* +0x68 */
    float initial_speed;     /* +0x6C */
    float final_speed;       /* +0x70 */
    int radial_movement;     /* +0x74 */
    char pad78[4];
} ScriptedCameraData;

extern CameraObj* camera_obj;
extern float DEFAULT_FIELD_OF_VIEW;
extern float DEFAULT_ASPECTRATIO;
extern float _mkproc_sleep_ticks;
extern float inverse_game_speed;
extern float game_speed;
/* Retail layout: ang[3], pos[3], old_ang[3], old_pos[3] at one base. */
extern float cam_ang_offset[12];
/* Saved camera entry while intro anim runs (cam.o .sbss). */
extern MkProcEntryFn old_camera_function;
extern ScriptedCameraData scripted_camera_data;
void reset_game_speed(void);

Vec cam_forward_uv;
Vec cam_right_uv;

float gxMathTan(float x);
RwCamera* RwCameraSetProjection(RwCamera* camera, int projection);
RwRaster* RwRasterSubRaster(RwRaster* sub_raster, RwRaster* raster, RwRect* rect);
/* Must stay external so xfer_camera emits bl (local stub was inlined away). */
void CameraSize(RwCamera* camera, RwRect* rect, float view_window, float aspect_ratio);
void xz_unit_vector(Vec* out, const Vec* from, const Vec* to);
void normalize_v3(Vec* value);

static const float kZero = 0.0f;
static const float kNegOne = -1.0f;
static const float kOne = 1.0f;
static const float kHalf = 0.5f;
static const float kThree = 3.0f;
static const float kSnapDist = 0.01f;
static const float kMoveScale = 0.1f;
static const float kPi = 3.1415927410125732f;
static const float kTwoPi = 6.2831854820251465f;
static const float kNegPi = -3.1415927410125732f;
static const float kAngEpsSq = 1.0e-6f;
static const float kDegPerTurn = 360.0f;

typedef struct BezierCamera {
    Vec control[4];       /* +0x00 */
    float step;           /* +0x30 */
    float acceleration;   /* +0x34 */
    float time;           /* +0x38 */
    unsigned int phase;   /* +0x3C */
    float decel_step;     /* +0x40 */
    float accel_end;      /* +0x44 */
    float decel_start;    /* +0x48 */
    float minimum_step;   /* +0x4C */
    float maximum_step;   /* +0x50 */
} BezierCamera;

typedef struct BackgroundDangerZone {
    char pad00[0xA0];
    int enabled; /* +0xA0 */
    char padA4[0x0C];
} BackgroundDangerZone;

static BackgroundDangerZone background_danger_zones[37];
static int number_of_danger_zones;

/* Default krypt camera pose (@2947 / @2948 in cam.o rodata). */
static const float kDefaultPos[3] = {-28.5f, 4.4f, 55.0f};
static const float kDefaultAng[3] = {0.42f, 3.1415927410125732f, 0.0f};

/*
 * Soft ceiling: retail bne/b on instance match; MWCC 2.7 peepholes to beq.
 * Keep != clear + empty keep branch (same polarity as bgnd_nbc / display).
 */
#define RESOLVE_CAMERA_OBJ(cam_)                                                               \
    do {                                                                                       \
        (cam_) = camera_item.node;                                                             \
        if ((cam_) != 0) {                                                                     \
            if ((cam_)->instance != camera_item.instance) {                                    \
                (cam_) = 0;                                                                    \
            } else {                                                                           \
                /* keep cam */                                                                 \
            }                                                                                  \
        } else {                                                                               \
            (cam_) = 0;                                                                        \
        }                                                                                      \
    } while (0)

static void mkproc_jump_sleep(MkProcEntryFn entry) {
    MkVtableMkproc* vtbl;
    union {
        MkProcEntryFn entry;
        int address;
    } continuation;

    vtbl = aproc->vtbl;
    continuation.entry = entry;
    vtbl->jump_sleep(continuation.address);
}

static void mkproc_sleep(void) {
    MkVtableMkproc* vtbl;

    vtbl = aproc->vtbl;
    vtbl->sleep();
}

static float p_animate_camera_move(void);
static float kick_camera(void);

void kick_the_camera(void) {
    xfer_proc(camera_info.proc, kick_camera);
}

static float kick_camera(void) {
    return kZero;
}

void do_victory_camera(void) {
}

static void generic_victory_camera(void) {
}

float p_mk_chess_cam_bezier_controller(void) {
    return kZero;
}

/* Soft ceiling: BezierCamera_SetOverallCameraTimeInTicks ~95% -- float emit order; stop. */
void BezierCamera_SetOverallCameraTimeInTicks(BezierCamera* camera, float ticks) {
    camera->step = kOne / (ticks * inverse_game_speed);
}

void BezierCamera_LinearAccelerateDeccelerate(BezierCamera* camera, float acceleration,
                                               float decel_step, float accel_end,
                                               float decel_start, float minimum_step,
                                               float maximum_step) {
    camera->phase = 1;
    camera->decel_step = decel_step * game_speed;
    camera->accel_end = accel_end * game_speed;
    camera->decel_start = decel_start * game_speed;
    camera->minimum_step = minimum_step * game_speed;
    camera->maximum_step = maximum_step * game_speed;
    camera->acceleration = acceleration * game_speed;
}

/* Soft ceiling: BezierCamera_Init ~93.39% -- structure copy scheduling; stop. */
void BezierCamera_Init(BezierCamera* camera, float step, const Vec* p0, const Vec* p1,
                       const Vec* p2, const Vec* p3) {
    camera->control[0].x = p0->x;
    camera->control[0].y = p0->y;
    camera->control[0].z = p0->z;
    camera->control[1].x = p1->x;
    camera->control[1].y = p1->y;
    camera->control[1].z = p1->z;
    camera->control[2].x = p2->x;
    camera->control[2].y = p2->y;
    camera->control[2].z = p2->z;
    camera->control[3].x = p3->x;
    camera->control[3].y = p3->y;
    camera->control[3].z = p3->z;
    camera->step = step;
    camera->acceleration = kZero;
    camera->time = kZero;
    camera->phase = 0;
}

static int BezierCamera_GetNextPoint(BezierCamera* camera, Vec* point) {
    float t;
    float t2;
    float t3;
    float p0_weight;
    float p1_weight;
    float p2_weight;

    t = camera->time;
    if (t > kOne) {
        point->x = camera->control[0].x * kZero + camera->control[1].x * kZero +
                   camera->control[2].x * kZero + camera->control[3].x * kOne;
        point->y = camera->control[0].y * kZero + camera->control[1].y * kZero +
                   camera->control[2].y * kZero + camera->control[3].y * kOne;
        point->z = camera->control[0].z * kZero + camera->control[1].z * kZero +
                   camera->control[2].z * kZero + camera->control[3].z * kOne;
        return 1;
    }

    t2 = t * t;
    t3 = t2 * t;
    p2_weight = -3.0f * t3 + 3.0f * t2;
    p1_weight = 3.0f * t3 - 6.0f * t2 + 3.0f * t;
    p0_weight = 1.0f + (-3.0f * t3 + 3.0f * t2 - 3.0f * t);

    point->x = camera->control[0].x * p0_weight + camera->control[1].x * p1_weight +
               camera->control[2].x * p2_weight + camera->control[3].x * t3;
    point->y = camera->control[0].y * p0_weight + camera->control[1].y * p1_weight +
               camera->control[2].y * p2_weight + camera->control[3].y * t3;
    point->z = camera->control[0].z * p0_weight + camera->control[1].z * p1_weight +
               camera->control[2].z * p2_weight + camera->control[3].z * t3;

    camera->time += camera->step;
    if (camera->phase == 1) {
        if (camera->time < camera->accel_end) {
            camera->step += camera->acceleration;
            if (camera->step > camera->maximum_step) {
                camera->step = camera->maximum_step;
            }
        } else {
            camera->acceleration = kZero;
        }
        if (camera->time >= camera->decel_start) {
            camera->acceleration = camera->decel_step;
            camera->phase = 2;
        }
    }
    if (camera->phase == 2 && camera->time >= camera->decel_start) {
        camera->step += camera->acceleration;
        if (camera->step < camera->minimum_step) {
            camera->step = camera->minimum_step;
        }
    }
    return 0;
}

float p_mk_chess_cam_chase_cursor(void) {
    return kZero;
}

float p_mk_chess_cam_control(void) {
    return kZero;
}

float camera_get_screen_pos_from_world_pos(void) {
    return kZero;
}

void remove_widescreen_bars(void) {
}

static float p_move_widescreen_bars(void) {
    return kZero;
}

void add_widescreen_bars(void) {
}

void cam_set_intro_cam_speed(float speed) {
    camera_info.pdata->speed = speed;
}

void cam_set_intro_cam_pause_ticks(float ticks) {
    camera_info.pdata->pause_ticks = ticks;
}

void camera_set_anim_aux_data(void* data) {
    camera_info.pdata->aux_data = data;
}

void camera_set_animation_mirror_plane(void) {
}

void camera_set_animation_parent_position(const CamVec3* position) {
    camera_info.pdata->mat_parent.pos.x = position->x;
    camera_info.pdata->mat_parent.pos.y = position->y;
    camera_info.pdata->mat_parent.pos.z = position->z;
}

void camera_set_animation_parent_angle(const CamVec3* angle, int relative) {
    camera_info.pdata->flags =
        (camera_info.pdata->flags & (unsigned char)~4) | ((relative & 1) << 2);
    YXZ_angles_to_MKMATRIX((const Vec*)angle, &camera_info.pdata->mat_parent);
}

void camera_wait_for_animation_completion(void) {
    MkProc* proc;

    while (1) {
        proc = camera_info.proc;
        if (proc == 0 || proc->entry != p_animate_camera_move) {
            break;
        }
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
}

/*
 * Full retail path (~668 B): CAM_BONE load, anim pdata, xfer to
 * p_animate_camera_move, optional wait. MVP skips bone/anim setup so
 * wait() does not spin - static camera pose from set_camera_* is enough
 * to see the krypt. Lift when Wave 4 intro-anim depth is needed.
 */
#pragma dont_inline on
void camera_run_animation_start_end(float start_frame, float end_frame, int wait_flag,
                                    int unused) {
    (void)start_frame;
    (void)end_frame;
    (void)wait_flag;
    (void)unused;
}
#pragma dont_inline reset

void camera_run_animation(int wait_flag) {
    camera_run_animation_start_end(kZero, kZero, wait_flag, 0);
}

void camera_init_animation(void* anim_path, MkProcEntryFn override_entry) {
    CameraPdata* pdata;
    MkProcEntryFn saved;

    pdata = camera_info.pdata;
    pdata->anim_path = anim_path;
    if (override_entry != 0) {
        saved = override_entry;
    } else {
        saved = camera_info.proc->entry;
    }
    old_camera_function = saved;
    MKMatrixSetIdentity(&pdata->mat_parent);
    MKMatrixSetIdentity(&pdata->mat_mirror);
    pdata->flags = (unsigned char)(pdata->flags & 0xDFu);
    pdata->speed = kZero;
    pdata->aux_data = 0;
}

float p_animate_and_freeze(void) {
    return kZero;
}

float p_animated_intro_done(void) {
    return kZero;
}

/*
 * Retail is ~1604 B (bone follow + advance_anim). MVP: restore prior
 * camera entry so wait_for_animation_completion always exits if somehow
 * transferred here before start_end is fully lifted.
 */
static float p_animate_camera_move(void) {
    mkproc_jump_sleep(old_camera_function);
    return kZero;
}

void* get_intro_camera_path(void) {
    return camera_info.pdata->intro_path;
}

void set_intro_camera_path(void* path) {
    camera_info.pdata->intro_path = path;
}

static void set_material_alpha(RpMaterial* material, const unsigned char* alpha) {
    unsigned char value = *alpha;
    if (material != 0) {
        material->color.alpha = value;
    }
}

static void is_shape_in_frustum(void) {
}

void toggle_danger_zone(int index) {
    if (index >= 0) {
        if (index > number_of_danger_zones) {
            return;
        }
        background_danger_zones[index].enabled ^= 1;
    }
}

void keep_camera_out_of_danger_zones(void) {
}

void add_background_danger_zone(void) {
}

void hide_sobj_if_camera_is_in_rectangle(void* object, int mode, float min_x,
                                         float min_z, float max_x,
                                         float max_z) {
    (void)object;
    (void)mode;
    (void)min_x;
    (void)min_z;
    (void)max_x;
    (void)max_z;
}

void turn_off_sobj_if_camera_is_in_rectangle(void* object, int mode,
                                             float min_x, float min_z,
                                             float max_x, float max_z) {
    (void)object;
    (void)mode;
    (void)min_x;
    (void)min_z;
    (void)max_x;
    (void)max_z;
}

void hide_sobj_if_camera_is_in_cylinder(void* object, int mode, float radius,
                                        float height) {
    (void)object;
    (void)mode;
    (void)radius;
    (void)height;
}

void turn_off_sobj_if_camera_is_in_cylinder(void* object, int mode,
                                            float radius, float height) {
    (void)object;
    (void)mode;
    (void)radius;
    (void)height;
}

void set_danger_zone_properties(void) {
}

void render_background_danger_areas(void) {
}

void initialize_background_danger_zones(void) {
}

float p_krypt_camera_loop(void) {
    CameraPdata* pdata;
    CameraObj* cam;
    CamPdataFlags* flag_bits;
    float default_pos[3];
    float default_ang[3];
    float speed_scale;
    float dx;
    float dy;
    float dz;
    float dist_sq;
    float dist;
    float d_ang_x;
    float d_ang_y;
    float ang_err_sq;
    unsigned int bits;
    unsigned int mantissa_exp;
    float guess;
    int pos_done;
    unsigned int* pos_words;
    unsigned int* ang_words;
    unsigned int* src_pos;
    unsigned int* src_ang;

    /* Word copies of @2947 / @2948 onto stack (retail lwz/stw shape). */
    pos_words = (unsigned int*)default_pos;
    ang_words = (unsigned int*)default_ang;
    src_pos = (unsigned int*)kDefaultPos;
    src_ang = (unsigned int*)kDefaultAng;
    pos_words[0] = src_pos[0];
    pos_words[1] = src_pos[1];
    pos_words[2] = src_pos[2];
    ang_words[0] = src_ang[0];
    ang_words[1] = src_ang[1];
    ang_words[2] = src_ang[2];

    pdata = camera_info.pdata;
    if (pdata == 0) {
        return kNegOne;
    }

    /* Unconditional stores after resolve (retail may write through NULL). */
    RESOLVE_CAMERA_OBJ(cam);
    cam->pos_x = default_pos[0];
    cam->pos_y = default_pos[1];
    cam->pos_z = default_pos[2];

    RESOLVE_CAMERA_OBJ(cam);
    cam->ang_x = default_ang[0];
    cam->ang_y = default_ang[1];
    cam->ang_z = default_ang[2];

    pdata->target_pos_x = default_pos[0];
    pdata->target_pos_y = default_pos[1];
    pdata->target_pos_z = default_pos[2];
    pdata->target_ang_x = default_ang[0];
    pdata->target_ang_y = default_ang[1];
    pdata->target_ang_z = default_ang[2];

    for (;;) {
        speed_scale = kMoveScale * pdata->speed;
        RESOLVE_CAMERA_OBJ(cam);

        dy = pdata->target_pos_y - cam->pos_y;
        dx = pdata->target_pos_x - cam->pos_x;
        dz = pdata->target_pos_z - cam->pos_z;
        dist_sq = dx * dx + dy * dy + dz * dz;

        /* Inlined gxMathSqrt (retail embeds table Newton step). */
        if (!(kZero < dist_sq)) {
            dist = kZero;
        } else {
            bits = *(unsigned int*)&dist_sq;
            mantissa_exp = (unsigned int)GXMathSqrtTable[(bits >> 10) & 0x3FFE] << 8;
            mantissa_exp |= (((bits & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
            guess = *(float*)&mantissa_exp;
            dist = kHalf * guess * (kThree - (guess * guess) / dist_sq);
        }

        if (dist < kSnapDist) {
            cam->pos_x = pdata->target_pos_x;
            cam->pos_y = pdata->target_pos_y;
            cam->pos_z = pdata->target_pos_z;
            pos_done = 1;
        } else {
            cam->pos_x = cam->pos_x + dx * speed_scale;
            cam->pos_y = cam->pos_y + dy * speed_scale;
            cam->pos_z = cam->pos_z + dz * speed_scale;
            pos_done = 0;
        }

        flag_bits = (CamPdataFlags*)&pdata->flags;
        if (pos_done != 0) {
            flag_bits->pos_done = 1;
        } else {
            flag_bits->pos_done = 0;
        }

        speed_scale = kMoveScale * pdata->speed;
        RESOLVE_CAMERA_OBJ(cam);

        d_ang_x = pdata->target_ang_x - cam->ang_x;
        if (d_ang_x > kPi) {
            d_ang_x = d_ang_x - kTwoPi;
        } else if (d_ang_x < kNegPi) {
            d_ang_x = d_ang_x + kTwoPi;
        }

        d_ang_y = pdata->target_ang_y - cam->ang_y;
        if (d_ang_y > kPi) {
            d_ang_y = d_ang_y - kTwoPi;
        } else if (d_ang_y < kNegPi) {
            d_ang_y = d_ang_y + kTwoPi;
        }

        ang_err_sq = d_ang_x * d_ang_x + d_ang_y * d_ang_y;

        if (ang_err_sq < kAngEpsSq) {
            cam->ang_x = pdata->target_ang_x;
            cam->ang_y = pdata->target_ang_y;
            cam->ang_z = pdata->target_ang_z;
        } else {
            cam->ang_x = cam->ang_x + d_ang_x * speed_scale;
            cam->ang_y = cam->ang_y + d_ang_y * speed_scale;
            cam->ang_z = cam->ang_z + kZero * speed_scale;
        }

        RESOLVE_CAMERA_OBJ(cam);

        /* Contiguous offset block: ang@0, pos@3, old_ang@6, old_pos@9. */
        cam_ang_offset[6] = cam_ang_offset[0];
        cam_ang_offset[7] = cam_ang_offset[1];
        cam_ang_offset[8] = cam_ang_offset[2];
        cam_ang_offset[9] = cam_ang_offset[3];
        cam_ang_offset[10] = cam_ang_offset[4];
        cam_ang_offset[11] = cam_ang_offset[5];

        cam->pos_x = cam->pos_x + cam_ang_offset[3];
        cam->pos_y = cam->pos_y + cam_ang_offset[4];
        cam->pos_z = cam->pos_z + cam_ang_offset[5];
        cam->ang_x = cam->ang_x + cam_ang_offset[0];
        cam->ang_y = cam->ang_y + cam_ang_offset[1];
        cam->ang_z = cam->ang_z + cam_ang_offset[2];

        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
}

/* Soft ceiling: p_krypt_camera_proc ~95.83% -- MWCC emit coloring; stop. */
float p_krypt_camera_proc(void) {
    CameraPdata* pdata;

    pdata = camera_info.pdata;
    if (pdata == 0) {
        return kNegOne;
    }
    pdata->speed = kOne;
    mkproc_jump_sleep(p_krypt_camera_loop);
    return kZero;
}

static float konquest_interior_camera_loop(void) {
    return kZero;
}

float p_konquest_interior_camera_proc(void) {
    return kZero;
}

float konquest_camera_loop(void) {
    return kZero;
}

float p_konquest_camera_proc(void) {
    return kZero;
}

void run_interaction_camera_script(void) {
}

static float p_run_interaction_camera(void) {
    return kZero;
}

void interaction_cam_set_target_info(void) {
}

static float p_interaction_cam(void) {
    return kZero;
}

static void look_at_interaction_target(void) {
}

static void check_reverse_interaction_cam_targets(void) {
}

float interaction_cam_glitched(void) {
    return kZero;
}

void reset_camera_paths(void) {
}

void special_move_cam_end(void) {
}

void special_move_cam_setup2(void) {
}

void special_move_cam_setup(void) {
}

static float p_special_move_cam(void) {
    return kZero;
}

float cam_calc_right_at_up_offsets(void) {
    return kZero;
}

float get_volume_from_distance(const Vec* position, float far_distance,
                               float near_distance) {
    (void)position;
    (void)far_distance;
    (void)near_distance;
    return kZero;
}

/*
 * Recovered retail spatial-pan algorithm. The surrounding camera TU remains
 * a scaffold, so this function is retained for semantics rather than Matching.
 */
float get_pan_value(float* position) {
    typedef struct SoundCameraFrame {
        char pad00[0x30];
        Vec position;
    } SoundCameraFrame;
    typedef struct SoundCameraObj {
        char pad00[0x24];
        SoundCameraFrame* frame;
    } SoundCameraObj;
    CameraObj* camera;
    Vec direction;
    Vec forward;
    Vec right;
    float forward_dot;
    float right_dot;

    RESOLVE_CAMERA_OBJ(camera);
    if (camera == 0) {
        return 0.0f;
    }

    xz_unit_vector(
        &direction,
        &((SoundCameraObj*)camera)->frame->position,
        (const Vec*)position);

    forward = cam_forward_uv;
    right = cam_right_uv;
    normalize_v3(&forward);
    normalize_v3(&right);

    forward_dot =
        forward.x * direction.x + forward.z * direction.z;
    right_dot = right.x * direction.x + right.z * direction.z;

    if (right_dot <= 0.0f) {
        return gxMathArcCos(forward_dot) / kPi;
    }
    return -gxMathArcCos(forward_dot) / kPi;
}

void camera_exit_script(void) {
}

void run_camera_script(void) {
}

static float p_run_camera_script(void) {
    return kZero;
}

float p_scripted_camera(void) {
    return kZero;
}

void* camera_get_victim(void) {
    return camera_info.pdata->victim;
}

void camera_set_victim(void* object) {
    camera_info.pdata->victim = object;
}

void* camera_get_attacker(void) {
    return camera_info.pdata->attacker;
}

void camera_set_attacker(void* object) {
    camera_info.pdata->attacker = object;
}

void camera_special_function(void) {
}

void camera_unpause_player(void) {
    reset_game_speed();
}

void camera_pause_player(void) {
    game_speed = kZero;
}

void camera_set_lookat_focus(void* object) {
    scripted_camera_data.lookat_focus = object;
}

void camera_set_movement_focus_obj(void* object) {
    scripted_camera_data.movement_focus = object;
}

int camera_get_mirror_flag(void) {
    return scripted_camera_data.mirror;
}

float is_a_to_the_right_of_b(void) {
    return kZero;
}

void camera_setup_simple_rotation(void) {
}

void camera_setup_tightrope_angle_offset(void) {
}

void camera_setup_radial_position(void) {
}

void camera_setup_radial_sweep(void) {
}

void camera_set_rotation_rate(float rate) {
    scripted_camera_data.rotation_rate = rate;
}

void camera_set_movement_rate(float rate) {
    scripted_camera_data.movement_rate = rate;
}

void camera_set_check_konquest_collisions_flag(int enabled) {
    scripted_camera_data.check_collisions = enabled;
}

void camera_set_glitch_flag(void) {
    scripted_camera_data.glitch = 1;
}

void camera_set_lookat_offset_explicit(float x, float y, float z) {
    scripted_camera_data.lookat_offset.x = x;
    scripted_camera_data.lookat_offset.z = z;
    scripted_camera_data.lookat_offset.y = y;
}

void camera_set_lookat_offset_obj_rel(void) {
}

void camera_set_lookat_offset(void) {
}

void cam_recalc_midpoint(void) {
}

float camera_get_pos(void) {
    return kZero;
}

void find_best_conversation_camera_position(void) {
}

void camera_check_reverse_move_offset(void) {
}

void camera_set_custom_camera_movement_flag(int enabled) {
    scripted_camera_data.custom_movement = enabled;
}

void camera_set_radial_movement(int enabled) {
    scripted_camera_data.radial_movement = enabled;
}

void camera_set_center_of_rotation(const CamVec3* center) {
    scripted_camera_data.center_of_rotation.x = center->x;
    scripted_camera_data.center_of_rotation.y = center->y;
    scripted_camera_data.center_of_rotation.z = center->z;
}

void camera_set_travel_time(float time) {
    scripted_camera_data.travel_time = time;
}

void camera_set_rotation_direction(int direction) {
    scripted_camera_data.rotation_direction = direction;
}

void camera_set_final_speed(float speed) {
    scripted_camera_data.final_speed = speed;
}

void camera_set_initial_speed(float speed) {
    scripted_camera_data.initial_speed = speed;
}

void camera_set_movement_offset_explicit(float x, float y, float z) {
    scripted_camera_data.movement_offset.x = x;
    scripted_camera_data.movement_offset.z = z;
    scripted_camera_data.movement_offset.y = y;
}

void camera_set_movement_offset_obj_rel(void) {
}

void camera_set_movement_offset(void) {
}

void camera_set_look_mode(int mode) {
    scripted_camera_data.look_mode = mode;
}

void camera_set_movement_mode(int mode) {
    scripted_camera_data.movement_mode = mode;
}

void camera_wait_for_pos_and_ang_move_done(void) {
}

void camera_wait_for_ang_move_done(void) {
}

void camera_wait_for_pos_move_done(void) {
}

int camera_is_ang_move_done(void) {
    return scripted_camera_data.ang_move_done != 0;
}

int camera_is_pos_move_done(void) {
    return scripted_camera_data.pos_move_done != 0;
}

void camera_reset_ang_done_flag(void) {
    scripted_camera_data.ang_move_done = 0;
}

void camera_reset_pos_done_flag(void) {
    scripted_camera_data.pos_move_done = 0;
}

void init_scripted_camera(void) {
}

float p_attract_camera(void) {
    return kZero;
}

static void attract_glitch_move_gamecam(void) {
}

static void attract_default_glitch_move(void) {
}

static void attract_move_flyby(void) {
}

static void attract_default_move(void) {
}

static void attract_update_flyby(void) {
}

static void attract_update_chase_cam(void) {
}

static void attract_move_gamecam(void) {
}

static void attract_update_radial_sweep(void) {
}

static void attract_setup_flyby(void) {
}

static void attract_setup_gamecam(void) {
}

static void attract_setup_chase_cam(void) {
}

static void attract_setup_radial_sweep(void) {
}

float p_puzzle_game_camera_proc(void) {
    return kZero;
}

float get_target_movement_vector(void) {
    return kZero;
}

void set_camera_target_angle(void) {
}

void set_camera_destination(void) {
}

float skip_camera_intro(void) {
    return kZero;
}

float intro_done(void) {
    return kZero;
}

static void radial_move_to_game_position(void) {
}

void orbit_position_to_end_point(void) {
}

void move_to_end_point(void) {
}

void look_at_target(void) {
}

void set_camera_focal_length(void) {
}

void set_camera_velocity(void) {
}

float get_camera_velocity(void) {
    return kZero;
}

/* Soft ceiling: resolve peephole; typed CameraObj ~88% (was ~94% with (char*)+4). */
void get_camera_angle(CamVec3* ang) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    ang->x = cam->ang_x;
    ang->y = cam->ang_y;
    ang->z = cam->ang_z;
}

void get_camera_position(CamVec3* pos) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    pos->x = cam->pos_x;
    pos->y = cam->pos_y;
    pos->z = cam->pos_z;
}

void set_camera_angle(CamVec3* ang) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    cam->ang_x = ang->x;
    cam->ang_y = ang->y;
    cam->ang_z = ang->z;
}

void set_camera_position(CamVec3* pos) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    cam->pos_x = pos->x;
    cam->pos_y = pos->y;
    cam->pos_z = pos->z;
}

void shake_camera_y(void) {
}

static float p_shake_camera_y(void) {
    return kZero;
}

void shake_camera(void) {
}

static float p_shake_camera(void) {
    return kZero;
}

void CameraDestroy(void) {
}

/* Soft ceiling: CameraSize ~99.91% -- pooled int-to-double relocation label only. */
#pragma optimize_for_size on
void CameraSize(RwCamera* camera, RwRect* rect, float view_window, float aspect_ratio) {
    RwV2d view;
    RwVideoMode video_mode;
    RwRect rect_storage;
    RwRaster* raster;

    if (camera != 0) {
        RwEngineGetVideoModeInfo(&video_mode, RwEngineGetCurrentVideoMode());
        if ((video_mode.flags & 1) != 0) {
            rect = &rect_storage;
            rect_storage.x = rect_storage.y = 0;
            rect_storage.w = video_mode.width;
            rect_storage.h = video_mode.height;
        } else if (rect == 0) {
            rect = &rect_storage;
            raster = camera->frameBuffer;
            rect_storage.w = raster->width;
            rect_storage.h = raster->height;
            rect_storage.x = rect_storage.y = 0;
        }

        if (rect->w > 0 && rect->h > 0) {
            if ((video_mode.flags & 1) != 0) {
                view.x = view_window;
                view.y = view_window / aspect_ratio;
            } else if (rect->w > rect->h) {
                view.x = view_window;
                view.y = ((float)rect->h * view_window) / (float)rect->w;
            } else {
                view.y = view_window;
                view.x = ((float)rect->w * view_window) / (float)rect->h;
            }

            raster = camera->frameBuffer;
            if (raster != 0) {
                RwRasterSubRaster(raster, raster->parent, rect);
            }

            raster = camera->zBuffer;
            if (raster != 0) {
                RwRasterSubRaster(raster, raster->parent, rect);
            }

            RwCameraSetViewWindow(camera, &view);
        }
    }
}
#pragma optimize_for_size reset

void vdestroy_mkpdata_camera(MkHdr* pdata) {
    pdata->instance = 0;
    mkhdr_memfree(pdata);
}

void camera_set_speed_scalar(float speed) {
    camera_info.pdata->speed = speed;
}

void adj_cam_pos(void) {
}

float player_is_stationary(void) {
    return kZero;
}

void go_to_camera_cut_with_angle(void) {
}

void go_to_camera_cut(void) {
}

float p_hold_camera_in_place(void) {
    return kZero;
}

void remove_camera_offsets(void) {
}

void add_camera_offsets(void) {
}

void get_current_target(void) {
}

void get_play_camera_position(void) {
}

void cam_set_ground_plane(void) {
}

float p_camera_proc(void) {
    return kZero;
}

static void prewake_camera(void) {
}

void camera_idle(void) {
}

float p_idle_camera(void) {
    return kZero;
}

/* Soft ceiling: xfer_camera ~99.78% -- pooled float relocation labels only. */
void xfer_camera(MkProcEntryFn entry, int reset_projection) {
    float tan_half_fov;
    CamObjFlags* flags;

    if (camera_info.proc == 0) {
        return;
    }
    xfer_proc(camera_info.proc, entry);
    if (reset_projection == 0) {
        return;
    }
    if (Camera != 0) {
        tan_half_fov = gxMathTan((kPi * DEFAULT_FIELD_OF_VIEW) / kDegPerTurn);
        CameraSize(Camera, 0, tan_half_fov, DEFAULT_ASPECTRATIO);
        RwCameraSetProjection(Camera, 1);
    }
    if (camera_obj != 0) {
        flags = (CamObjFlags*)&camera_obj->flags;
        flags->bit04 = 1;
        flags = (CamObjFlags*)&camera_obj->flags;
        flags->bit20 = 1;
    }
}

void init_camera(void) {
}

MkProc* get_camera_proc(void) {
    return camera_info.proc;
}

CameraPdata* get_pdata_of_camera(void) {
    return camera_info.pdata;
}

void turn_camera_off(void) {
    turn_display_off();
    if (World != 0 && Camera != 0 && RwCameraGetWorld(Camera) != 0) {
        RpWorldRemoveCamera(World, Camera);
    }
}

void turn_camera_on(void) {
    if (Camera == 0) {
        return;
    }
    if (World != 0) {
        if (RwCameraGetWorld(Camera) == 0) {
            RpWorldAddCamera(World, Camera);
        }
    }
    turn_display_on();
}
