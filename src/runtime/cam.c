#include "runtime/cam.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/anim_types.h"
#include "runtime/plyr_pdata.h"
#include "runtime/image.h"
#include "runtime/sound.h"
#include "runtime/anim_pdata.h"
#include "runtime/asset.h"

#include "math/gxMath.h"
#include "math/gxQuat.h"
#include "math/mk_math.h"
#include "game/game_info.h"
#include "game/collision.h"
#include "game/moveset.h"
#include "game/mk_chess.h"
#include "platform/display.h"
#include "platform/display_metrics.h"
#include "platform/io.h"
#include "rw/rwcore_types.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwdevice.h"
#include "rw/rwframe.h"
#include "rw/rpworld_types.h"
#include "rw/rwtypehf.h"

/* Camera runtime reconstructed from retail control flow, layouts, and callers. */

typedef struct ScriptedCameraData {
    int movement_mode;       /* +0x00 */
    int look_mode;           /* +0x04 */
    Vec movement_offset;     /* +0x08 */
    Vec lookat_offset;       /* +0x14 */
    int glitch;              /* +0x20 */
    float rotation_rate;     /* +0x24 */
    float movement_rate;     /* +0x28 */
    MkObj* movement_focus; /* +0x2C */
    MkObj* lookat_focus;  /* +0x30 */
    int mirror;           /* +0x34 */
    float focus_angle;    /* +0x38 */
    Vec focus_direction;  /* +0x3C */
    int pos_move_done;       /* +0x48 */
    int ang_move_done;       /* +0x4C */
    int check_collisions;    /* +0x50 */
    int custom_movement;     /* +0x54 */
    Vec center_of_rotation;  /* +0x58 */
    union {
        struct {
            float travel_time; /* +0x64 */
            union {
                int rotation_direction;
                float rotation_step;
            };                 /* +0x68 */
            float initial_speed; /* +0x6C */
        };
        Vec radial_vector;    /* +0x64 */
    };
    float final_speed;       /* +0x70 */
    union {
        int radial_movement;
        float radial_step;
    };                       /* +0x74 */
    float radial_distance;   /* +0x78 */
} ScriptedCameraData;

typedef struct FadeBoxItem FadeBoxItem;

extern CameraObj* camera_obj;
extern float DEFAULT_FIELD_OF_VIEW;
extern float DEFAULT_ASPECTRATIO;
extern float _mkproc_sleep_ticks;
extern float inverse_game_speed;
extern float game_speed;
extern float cam_fov;
extern float camera_speed;
extern float cam_rot_speed;
extern RwMatrix* camera_mat;
extern float field_of_view_ratio;
extern int end_round_cam_done;
extern int camera_mode;
extern int mode_of_play;
extern MkObj* plyr_obj;
extern Vec tightrope_uv;
extern Vec tightrope_perp_uv;
extern Vec conversation_midpoint;
extern float conversation_interaction_angle;
extern float right_frustum_plane_dist;
extern float left_frustum_plane_dist;
extern FadeBoxItem lower_fade_box_item;
extern FadeBoxItem upper_fade_box_item;
/* Saved camera entry while intro anim runs (cam.o .sbss). */
extern MkProcEntryFn old_camera_function;
void reset_game_speed(void);
void adj_cam_pos(void);
float get_constrain_player_distance(void);
void* memset(void* destination, int value, unsigned long size);
int get_game_state(void);
int get_konquest_game_mode(void);
void set_process_as_scriptable(MkProc* proc);
void atomic_set_transl_flag(RpAtomic* atomic);
void render_col_shape(const CollisionShape* shape, const unsigned int* color);
extern unsigned int rgba_red;
extern unsigned int rgba_green;
extern unsigned short GXMathSqrtTable[];
extern int MksobjLocalOffset;
double atan2(double y, double x);
void hide_atomic(void* atomic);
void unhide_atomic(void* atomic);
float frand(float maximum);
unsigned short randu0(unsigned int maximum);
int build_bones_tbl(MkObj* object, const int* tags);
void set_root_and_obj_movement_weights(float root_weight, float object_weight,
                                       AnimPdata* animation);
void get_bone_offset_world_pos(MkObj* object, int bone, const Vec* offset,
                               Vec* position);
MslSoundHandle pan_vol_snd_req(int sound_id, float pan, float volume);

Vec cam_ang_offset = {0.0f, 0.0f, 0.0f};
Vec cam_pos_offset = {0.0f, 0.0f, 0.0f};
Vec old_cam_ang_offset = {0.0f, 0.0f, 0.0f};
Vec old_cam_pos_offset = {0.0f, 0.0f, 0.0f};
static int camera_bones[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
static Vec uv_y = { 0.0f, 1.0f, 0.0f };
static Vec cam_unit_vector = {0.0f, 0.0f, 1.0f};
static Vec camera_midpoint = {0.0f, 1.55f, 0.0f};
static Vec right_frustum_plane_normal;
static Vec left_frustum_plane_normal;
static Vec right_frustum_vector;
static Vec left_frustum_vector;
static Vec cam_up_uv;
static Vec cam_forward_uv;
static Vec cam_right_uv;
static Vec midpoint_to_cam_vector;
static float last_camera_distance = 2.6f;

float gxMathTan(float x);
RwRaster* RwRasterSubRaster(RwRaster* sub_raster, RwRaster* raster, RwRect* rect);
/* Must stay external so xfer_camera emits bl (local stub was inlined away). */
void CameraSize(RwCamera* camera, RwRect* rect, float view_window, float aspect_ratio);
void CameraDestroy(RwCamera* camera);
MkObj* get_mkobj_frame(int type, RwFrame* frame);
float xz_ray_circle_intersection_dist(const Vec* ray_origin,
                                      const Vec* ray_direction,
                                      float radius);
void xz_unit_vector(Vec* out, const Vec* from, const Vec* to);
void normalize_v3(Vec* value);
float p_camera_proc(void);
float p_attract_camera(void);
float p_idle_camera(void);
float p_scripted_camera(void);
static float p_run_camera_script(void);
static float p_run_interaction_camera(void);
static float p_interaction_cam(void);
static float p_special_move_cam(void);
static float p_move_widescreen_bars(void);
float p_hold_camera_in_place(void);
static void look_at_interaction_target(const Vec* target, int snap_angles);
static void check_reverse_interaction_cam_targets(void);
void get_play_camera_position(Vec* position);
float get_game_speed(void);
int player_is_stationary(PlyrPdata* player);
int am_i_on_the_left(void);
int am_i_on_the_left2(MkObj* target, MkObj* reference_object);
int move_to_end_point(const Vec* endpoint, float* initial_speed,
                      float* final_speed, int reset, float time);
int orbit_position_to_end_point(const Vec* center, const Vec* endpoint,
                                float* initial_speed, float* final_speed,
                                unsigned int direction, int reset, float time);

static const float kZero = 0.0f;
static const float kNegOne = -1.0f;
static const float kOne = 1.0f;
static const float kHalf = 0.5f;
static const float kThree = 3.0f;
static const float kInvSqrtScale = 0.0625f;
static const float kNewton12 = 12.0f;
static const float kSnapDist = 0.01f;
static const float kMoveScale = 0.1f;
static const float kPi = 3.1415927410125732f;
static const float kTwoPi = 6.2831854820251465f;
static const float kNegPi = -3.1415927410125732f;
static const float kAngEpsSq = 1.0e-6f;
static const float kDegPerTurn = 360.0f;

static inline float camera_inv_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } pun;
    float guess;
    float first_step;
    float correction;

    if (value <= kZero) {
        return kZero;
    }
    pun.f = value;
    pun.u = 0x5F375A00U - (pun.u >> 1);
    guess = pun.f;
    first_step = guess * (value * guess);
    correction = kThree - first_step;
    return kInvSqrtScale * guess * correction *
           (kNewton12 - first_step * correction * correction);
}

static inline float normalize_interaction_angle(float angle) {
    return 0.000005992112f *
           (float)((int)(166886.1f * angle) & 0xFFFFF);
}

static inline void camera_scale_v3(Vec* out, const Vec* value, float scale) {
    out->x = value->x * scale;
    out->y = value->y * scale;
    out->z = value->z * scale;
}

static inline void camera_matrix_set_identity(MKMATRIX* matrix) {
    matrix->at.z = kOne;
    matrix->up.y = kOne;
    matrix->right.x = kOne;
    matrix->up.x = kZero;
    matrix->right.z = kZero;
    matrix->right.y = kZero;
    matrix->at.y = kZero;
    matrix->at.x = kZero;
    matrix->up.z = kZero;
    matrix->pos.z = kZero;
    matrix->pos.y = kZero;
    matrix->pos.x = kZero;
    matrix->flags |= 0x20003U;
}

static inline void camera_turn_on_impl(void) {
    if (Camera == 0) {
        return;
    }
    if (World != 0 && RwCameraGetWorld(Camera) == 0) {
        RpWorldAddCamera(World, Camera);
    }
    turn_display_on();
}

static inline void make_interaction_orbit_offset(Vec* out, float angle,
                                                 float radius,
                                                 float height) {
    Vec unit_z = {0.0f, 0.0f, 1.0f};
    float inverse_length;

    rotate_xz(out, &unit_z, angle);
    inverse_length = camera_inv_sqrt(out->x * out->x + out->z * out->z);
    out->x *= inverse_length;
    out->z *= inverse_length;
    out->x *= radius;
    out->z *= radius;
    out->y = height;
}

static inline float camera_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } pun;
    unsigned int bits;
    float guess;
    float correction;

    if (value <= kZero) {
        return kZero;
    }
    pun.f = value;
    bits = pun.u;
    pun.u = (unsigned int)GXMathSqrtTable[(bits >> 11) & 0x1FFF] << 8;
    pun.u |=
        (((bits & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    guess = pun.f;
    correction = kThree - (guess * guess) / value;
    guess *= correction;
    return kHalf * guess;
}

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

BezierCamera g_bezier_cam;
static int BezierCamera_GetNextPoint(BezierCamera* camera, Vec* point);

static inline void chess_camera_look_at(const Vec* target) {
    CameraObj* camera;
    Vec direction;
    Vec angles;
    float inverse_length;

    camera = camera_item.node;
    if (camera != 0 && camera->hdr.instance != camera_item.instance) {
        camera = 0;
    }
    direction.x = target->x - camera->pos.x;
    direction.y = target->y - camera->pos.y;
    direction.z = target->z - camera->pos.z;
    inverse_length = camera_inv_sqrt(
        direction.x * direction.x + direction.y * direction.y +
        direction.z * direction.z);
    direction.x *= inverse_length;
    direction.y *= inverse_length;
    direction.z *= inverse_length;
    v3_to_xy_ang_high_freq(&angles, &direction);
    camera->ang.x = angles.x;
    camera->ang.y = angles.y;
    camera->ang.z = angles.z;
}

typedef struct BackgroundDangerZone {
    CollisionShape shape;
    RpAtomic* object;
    int action;
    int test_mode;
    int cooldown;
    int enabled; /* +0xA0 */
    char padA4[0x0C];
} BackgroundDangerZone;

typedef struct InteractionCameraData {
    MkObj* hero;
    unsigned int hero_instance;
    MkObj* target;
    unsigned int target_instance;
    float camera_yaw_offset;
    float camera_radius;
    float camera_height;
    float look_yaw_offset;
    float look_radius;
    float look_height;
    int ticks;
    int created_process;
    int reversed;
    int glitched;
    int blocked;
} InteractionCameraData;

typedef struct InteractionCameraProcData {
    MkHdr hdr;
    void* owner;
    void* script;
    int field_10;
} InteractionCameraProcData;

typedef struct SpecialMoveCameraData {
    MkObj* target;
    unsigned int target_instance;
    float orbit_yaw_offset;
    float orbit_radius;
    float camera_height;
    float look_yaw_offset;
    float look_pitch;
    int ease_ticks;
    int total_ticks;
} SpecialMoveCameraData;

typedef struct ActiveNpcCameraView {
    char pad00[0x1D];
    unsigned char field_1D;
} ActiveNpcCameraView;

typedef struct KonquestCameraView {
    char pad00[0x24];
    void* camera_script;
    char pad28[0x1C];
    int widescreen_bars_active;
    char pad48[0xB0];
    MkObj* hero_object;
    unsigned int hero_instance;
    char pad100[0xF8];
    struct InteractionNpc* movement_npc;
    unsigned int movement_npc_instance;
    char pad200[0x0C];
    int conversation_mode_b;
} KonquestCameraView;

struct FadeBoxItem {
    ScreenObj* node;
    unsigned int instance;
};

typedef struct WidescreenBarPdata {
    MkHdr hdr;
    float step;
    int direction;
} WidescreenBarPdata;

typedef struct InteractionNpcTargetInfo {
    char pad00[0x0C];
    MkObj* object;
} InteractionNpcTargetInfo;

typedef struct InteractionNpc {
    MkHdr hdr;
    char pad08[0x0C];
    InteractionNpcTargetInfo* target_info;
    char pad1C[0x84];
    Vec pos;
} InteractionNpc;

typedef struct AttractCameraState {
    union {
        Vec center;
        MkObj* target;
    };
    float field_0C;
    float field_10;
    float field_14;
    float field_18;
    CameraObj* camera;
    Vec target_position;
    Vec current_position;
    Vec current_angles;
    float field_44;
    float field_48;
    int mode;
    unsigned int countdown;
} AttractCameraState;

typedef void (*AttractCameraCallback)(AttractCameraState* state);

typedef struct AttractCameraSetup {
    AttractCameraCallback setup;
    AttractCameraCallback update;
    AttractCameraCallback move;
    AttractCameraCallback glitch_move;
} AttractCameraSetup;

static void attract_setup_radial_sweep(AttractCameraState* state);
static void attract_update_radial_sweep(AttractCameraState* state);
static void attract_default_move(AttractCameraState* state);
static void attract_default_glitch_move(AttractCameraState* state);
static void attract_setup_gamecam(AttractCameraState* state);
static void attract_move_gamecam(AttractCameraState* state);
static void attract_glitch_move_gamecam(AttractCameraState* state);
static void attract_setup_chase_cam(AttractCameraState* state);
static void attract_update_chase_cam(AttractCameraState* state);
static void attract_setup_flyby(AttractCameraState* state);
static void attract_update_flyby(AttractCameraState* state);
static void attract_move_flyby(AttractCameraState* state);

extern AttractCameraSetup attract_cam_setup_table[4];

typedef struct CameraShakePdata {
    MkHdr hdr;
    int count;
    float strength;
} CameraShakePdata;

typedef struct CameraScriptPdata {
    MkHdr hdr;
    int script;
    int argument;
    int flags;
} CameraScriptPdata;

typedef struct CameraScriptMonitorItem {
    MkProc* node;
    unsigned int instance;
} CameraScriptMonitorItem;

typedef struct CameraCmdScriptView {
    char pad00[0x20];
    int active;
} CameraCmdScriptView;

typedef struct DangerZoneAtomicView {
    char pad00[0x18];
    struct {
        char pad00[8];
        unsigned int flags;
    }* field_18;
} DangerZoneAtomicView;

static BackgroundDangerZone background_danger_zones[37];
static int number_of_danger_zones;
InteractionCameraData g_ic_data;
ScriptedCameraData scripted_camera_data;
static SpecialMoveCameraData smc_data;
int force_midpoint_calculation_update;
extern CameraScriptMonitorItem camera_script_monitor_item;
extern ActiveNpcCameraView* g_active_npc;
extern KonquestCameraView* konquest_pdata;
void cmdscript_setup_execution(int script, int argument);
void cmdscript_execute(int script);
CameraCmdScriptView* get_cmdscript_for_proc(MkProc* proc);

typedef union CameraVecBits {
    float values[3];
    unsigned int words[3];
} CameraVecBits;

/* Default krypt camera pose (@2947 / @2948 in cam.o rodata). */
static const CameraVecBits kDefaultPos = {{-28.5f, 4.4f, 55.0f}};
static const CameraVecBits kDefaultAng =
    {{0.42f, 3.1415927410125732f, 0.0f}};
static Vec unit_z;
static RwMatrix camera_anim_fixup_matrix;

typedef struct CameraAnimEvent {
    int type;
    float frame;
    unsigned int argument;
    float volume;
    float pan;
} CameraAnimEvent;

/* Retail's bne/b instance diamond peepholes to beq in this compiler build. */
#define RESOLVE_CAMERA_OBJ(cam_)                                                               \
    do {                                                                                       \
        (cam_) = camera_item.node;                                                             \
        if ((cam_) != 0) {                                                                     \
            if ((cam_)->hdr.instance != camera_item.instance) {                                \
                (cam_) = 0;                                                                    \
            }                                                                                  \
        } else {                                                                               \
            (cam_) = 0;                                                                        \
        }                                                                                      \
    } while (0)

static inline void remove_camera_offsets_impl(void) {
    CameraObj* camera;

    RESOLVE_CAMERA_OBJ(camera);
    camera->pos.x -= old_cam_pos_offset.x;
    camera->pos.y -= old_cam_pos_offset.y;
    camera->pos.z -= old_cam_pos_offset.z;
    camera->ang.x -= old_cam_ang_offset.x;
    camera->ang.y -= old_cam_ang_offset.y;
    camera->ang.z -= old_cam_ang_offset.z;
}

static inline void add_camera_offsets_impl(void) {
    CameraObj* camera;

    RESOLVE_CAMERA_OBJ(camera);
    old_cam_ang_offset.x = cam_ang_offset.x;
    old_cam_ang_offset.y = cam_ang_offset.y;
    old_cam_ang_offset.z = cam_ang_offset.z;
    old_cam_pos_offset.x = cam_pos_offset.x;
    old_cam_pos_offset.y = cam_pos_offset.y;
    old_cam_pos_offset.z = cam_pos_offset.z;
    camera->pos.x += cam_pos_offset.x;
    camera->pos.y += cam_pos_offset.y;
    camera->pos.z += cam_pos_offset.z;
    camera->ang.x += cam_ang_offset.x;
    camera->ang.y += cam_ang_offset.y;
    camera->ang.z += cam_ang_offset.z;
}

static inline void get_target_movement_vector_impl(
    const Vec* current_position, const Vec* target_position, Vec* movement,
    float duration) {
    static float units_per_tick;
    Vec delta;
    float length_squared;
    float inverse_length;
    float scaled_duration;
    float denominator;

    delta.x = target_position->x - current_position->x;
    delta.y = target_position->y - current_position->y;
    delta.z = target_position->z - current_position->z;
    length_squared = delta.x * delta.x + delta.y * delta.y +
                     delta.z * delta.z;
    scaled_duration = 0.6f * duration;
    denominator = 60.0f * scaled_duration;
    units_per_tick = camera_sqrt(length_squared) / denominator;
    inverse_length = camera_inv_sqrt(length_squared);
    delta.x *= inverse_length;
    delta.y *= inverse_length;
    delta.z *= inverse_length;
    movement->x = delta.x;
    movement->y = delta.y;
    movement->z = delta.z;
    movement->x *= units_per_tick;
    movement->y *= units_per_tick;
    movement->z *= units_per_tick;
}

static inline InteractionNpc* get_konquest_movement_npc(void) {
    InteractionNpc* movement_npc = konquest_pdata->movement_npc;

    if (movement_npc != 0) {
        if (movement_npc->hdr.instance !=
            konquest_pdata->movement_npc_instance) {
            movement_npc = 0;
        }
    } else {
        movement_npc = 0;
    }
    return movement_npc;
}

static void mkproc_jump_sleep(MkProcEntryFn entry) {
    ((MkProcEntryVtable*)aproc->vtbl)->jump_sleep(entry, kZero);
}

static void mkproc_sleep(void) {
    MkVtableMkproc* vtbl;

    vtbl = aproc->vtbl;
    vtbl->sleep();
}

static inline void xfer_camera_impl(MkProcEntryFn entry, int reset_projection) {
    float tan_half_fov;
    CameraObjFlags* flags;

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
        flags = &camera_obj->flags;
        flags->bit04 = 1;
        flags = &camera_obj->flags;
        flags->bit20 = 1;
    }
}

static float p_animate_camera_move(void);
static float kick_camera(void);
static float p_shake_camera(void);

void kick_the_camera(void) {
    xfer_proc(camera_info.proc, kick_camera);
}

static inline int kick_camera_move_to_position(const Vec* target) {
    CameraObj* camera;
    float dx;
    float dy;
    float dz;
    float distance;

    RESOLVE_CAMERA_OBJ(camera);
    dx = target->x - camera->pos.x;
    dy = target->y - camera->pos.y;
    dz = target->z - camera->pos.z;
    distance = camera_sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < 0.01f) {
        camera->pos.x = target->x;
        camera->pos.y = target->y;
        camera->pos.z = target->z;
        return 1;
    }

    dx *= 0.1f;
    dy *= 0.1f;
    dz *= 0.1f;
    camera->pos.x += dx;
    camera->pos.y += dy;
    camera->pos.z += dz;
    return 0;
}

/* Near miss: validated-latch branch polarity and late FPR scheduling only. */
static float kick_camera(void) {
    CameraObj* camera;
    CameraObj* camera_check;
    CameraPdata* pdata;
    MkObj* attacker;
    CameraShakePdata* shake;
    Vec target_position;
    Vec bone_offset = {0.05f, 0.0f, 0.8f};
    Vec facing;
    Vec angles;
    Vec delta;
    Vec look_position;
    float inverse_length;
    float target_scale;
    float target_offset_x;
    float target_offset_y;
    float target_offset_z;

    RESOLVE_CAMERA_OBJ(camera);
    if (camera == 0) {
        mkproc_jump_sleep(p_camera_proc);
        return kZero;
    }
    pdata = camera_info.pdata;
    if (pdata == 0) {
        mkproc_jump_sleep(p_camera_proc);
        return kZero;
    }
    attacker = pdata->attacker;
    {
        int flip_bone_offset;

        facing = (Vec){0.0f, 0.0f, 1.0f};
        RESOLVE_CAMERA_OBJ(camera_check);
        if (camera_check == 0) {
            flip_bone_offset = 0;
        } else {
            rotate_xz(&facing, &facing, attacker->ang.y);
            if (cam_forward_uv.z * facing.x -
                    cam_forward_uv.x * facing.z <
                kZero) {
                flip_bone_offset = 1;
            } else {
                flip_bone_offset = 0;
            }
        }
        if (flip_bone_offset != 0) {
            bone_offset.x *= kNegOne;
        }
    }
    if (_create_mkproc_generic_tinystack(
            0x1007, 0x1E, p_shake_camera, sizeof(CameraShakePdata),
            (MkHdr**)&shake) != 0) {
        shake->count = 5;
        shake->strength = 0.02f;
    }

    target_scale = 5.0f;
    target_scale = -target_scale;
    target_offset_x = cam_forward_uv.x * target_scale;
    target_offset_y = cam_forward_uv.y * target_scale;
    target_offset_z = cam_forward_uv.z * target_scale;
    target_position.x = camera->pos.x + target_offset_x;
    target_position.y = camera->pos.y + target_offset_y;
    target_position.z = camera->pos.z + target_offset_z;
    RESOLVE_CAMERA_OBJ(camera);
    if (camera != 0) {
        look_position.x = 2.0f * cam_forward_uv.x;
        look_position.y = 2.0f * cam_forward_uv.y;
        look_position.z = 2.0f * cam_forward_uv.z;
        look_position.x += camera->pos.x;
        look_position.y += camera->pos.y;
        look_position.z += camera->pos.z;
    }

    while (kick_camera_move_to_position(&target_position) == 0) {
        RESOLVE_CAMERA_OBJ(camera);
        delta.x = look_position.x - camera->pos.x;
        delta.y = look_position.y - camera->pos.y;
        delta.z = look_position.z - camera->pos.z;
        inverse_length = camera_inv_sqrt(
            delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
        delta.x *= inverse_length;
        delta.y *= inverse_length;
        delta.z *= inverse_length;
        v3_to_xy_ang_high_freq(&angles, &delta);
        camera->ang.x = angles.x;
        camera->ang.y = angles.y;
        camera->ang.z = angles.z;
        RESOLVE_CAMERA_OBJ(camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        camera->pos.x += cam_pos_offset.x;
        camera->pos.y += cam_pos_offset.y;
        camera->pos.z += cam_pos_offset.z;
        camera->ang.x += cam_ang_offset.x;
        camera->ang.y += cam_ang_offset.y;
        camera->ang.z += cam_ang_offset.z;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }

    get_bone_offset_world_pos(attacker, 0x10, &bone_offset,
                              &target_position);
    while (kick_camera_move_to_position(&target_position) == 0) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }

    _mkproc_sleep_ticks = 120.0f;
    mkproc_sleep();
    end_round_cam_done = 1;
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
    }
}

static float generic_victory_camera(void);

void do_victory_camera(VictoryCameraConfig* config) {
    MkProc* camera_proc;
    CameraPdata* pdata;
    MkObj* opponent;

    camera_proc = camera_info.proc;
    pdata = camera_info.pdata;
    if (camera_proc != 0) {
        pdata = (CameraPdata*)pdata_of_proc(camera_proc);
    }
    if (pdata != 0 && mode_of_play != 8) {
        end_round_cam_done = 0;
        pdata->attacker = plyr_obj;
        opponent = g_game_info.plyr0.slot.mirror_a;
        if (plyr_obj == opponent) {
            opponent = g_game_info.plyr1.slot.mirror_a;
        }
        pdata->victim = opponent;
        pdata->victory_camera_config = config;
        xfer_proc(camera_proc, generic_victory_camera);
    }
}

static float generic_victory_camera(void) {
    CameraPdata* pdata = camera_info.pdata;
    CameraObj* initial_camera;
    CameraObj* camera_node;
    CameraObj* camera;
    MkObj* attacker;
    VictoryCameraConfig* config;
    Vec zero;
    Vec side_axis;
    Vec radial_source;
    Vec side_source;
    Vec radial;
    Vec side_offset;
    Vec destination;
    Vec look_target;
    Vec moving_look_target;
    Vec forward_step;
    Vec look_step;
    Vec direction;
    Vec desired_angles;
    float initial_speed = kZero;
    float final_speed = kZero;
    float radius;
    float side_angle;
    float configured_side_angle;
    float forward_offset;
    float camera_angle;
    float look_height;
    float travel_time;
    float pitch_delta;
    float yaw_delta;
    float roll_delta;
    float step_length;
    float remaining_length;
    float inverse_length;
    int rotation_ticks;
    int side_is_left;

    RESOLVE_CAMERA_OBJ(initial_camera);
    if (initial_camera == 0) {
        mkproc_jump_sleep(p_camera_proc);
        return kZero;
    }
    if (camera_info.proc != 0) {
        pdata = (CameraPdata*)pdata_of_proc(camera_info.proc);
    }
    if (pdata == 0) {
        mkproc_jump_sleep(p_camera_proc);
        return kZero;
    }

    attacker = pdata->attacker;
    zero = (Vec){0.0f, 0.0f, 0.0f};
    move_to_end_point(&zero, &initial_speed, &final_speed, 1, kZero);
    orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1, 1,
                                kZero);
    initial_camera->velocity.z = kZero;
    initial_camera->velocity.y = kZero;
    initial_camera->velocity.x = kZero;
    initial_camera->ang_velocity.z = kZero;
    initial_camera->ang_velocity.y = kZero;
    initial_camera->ang_velocity.x = kZero;
    config = pdata->victory_camera_config;
    radius = config->radius;
    configured_side_angle = config->side_angle;
    forward_offset = config->forward_offset;
    camera_angle = config->camera_angle;
    look_height = config->look_height;
    rotation_ticks = config->rotation_ticks;
    travel_time = config->travel_time;
    end_round_cam_done = 0;

    camera_node = camera_item.node;
    camera = 0;
    if (camera_node != 0) {
        if (camera_node->hdr.instance == camera_item.instance) {
            camera = camera_node;
        }
    }
    if (camera != 0) {
        camera_scale_v3(&forward_step, &cam_forward_uv, 2.0f);
        moving_look_target = camera->pos;
        moving_look_target.x += forward_step.x;
        moving_look_target.y += forward_step.y;
        moving_look_target.z += forward_step.z;
    }

    if (camera_node != 0) {
        if (camera_node->hdr.instance != camera_item.instance) {
            camera_node = 0;
        }
    }

    if (camera_node == 0) {
        side_is_left = 0;
    } else {
        side_axis = (Vec){1.0f, 0.0f, 0.0f};
        rotate_xz(&side_axis, &side_axis, attacker->ang.y);
        if (cam_forward_uv.z * side_axis.x -
                cam_forward_uv.x * side_axis.z <
            kZero) {
            side_is_left = 1;
        } else {
            side_is_left = 0;
        }
    }
    if (side_is_left != 0) {
        side_angle = attacker->ang.y - 1.5707964f - camera_angle;
    } else {
        configured_side_angle *= kNegOne;
        side_angle = 1.5707964f + attacker->ang.y + camera_angle;
    }

    radial_source = (Vec){0.0f, 0.0f, 1.0f};
    camera_scale_v3(&radial, &radial_source, radius);
    rotate_xz(&radial, &radial, attacker->ang.y - configured_side_angle);
    side_source = (Vec){0.0f, 0.0f, 1.0f};
    camera_scale_v3(&side_offset, &side_source, kOne);
    rotate_xz(&side_offset, &side_offset, side_angle);

    destination.x = attacker->pos.value.x + radial.x;
    destination.y = attacker->pos.value.y + radial.y;
    destination.z = attacker->pos.value.z + radial.z;
    look_target = destination;
    destination.y = g_game_info.field_34 + forward_offset;
    look_target.y += look_height;
    destination.x += side_offset.x;
    destination.y += side_offset.y;
    destination.z += side_offset.z;

    if (rotation_ticks != 0) {
        direction.x = attacker->pos.value.x - initial_camera->pos.x;
        direction.y = attacker->pos.value.y - initial_camera->pos.y;
        direction.z = attacker->pos.value.z - initial_camera->pos.z;
        inverse_length = camera_inv_sqrt(direction.x * direction.x +
                                         direction.y * direction.y +
                                         direction.z * direction.z);
        direction.x *= inverse_length;
        direction.y *= inverse_length;
        direction.z *= inverse_length;
        v3_to_xy_ang(&desired_angles, &direction);
        desired_angles.x = initial_camera->ang.x;
        while (rotation_ticks != 0) {
            RESOLVE_CAMERA_OBJ(camera);
            pitch_delta = desired_angles.x - camera->ang.x;
            if (pitch_delta > kPi) {
                pitch_delta -= kTwoPi;
            } else if (pitch_delta < kNegPi) {
                pitch_delta += kTwoPi;
            }
            yaw_delta = desired_angles.y - camera->ang.y;
            if (yaw_delta > kPi) {
                yaw_delta -= kTwoPi;
            } else if (yaw_delta < kNegPi) {
                yaw_delta += kTwoPi;
            }
            if (pitch_delta * pitch_delta + yaw_delta * yaw_delta <
                1.0000001e-6f) {
                camera->ang.x = desired_angles.x;
                camera->ang.y = desired_angles.y;
                camera->ang.z = desired_angles.z;
            } else {
                pitch_delta *= 0.1f;
                yaw_delta *= 0.1f;
                roll_delta = kZero * 0.1f;
                camera->ang.x = camera->ang.x + pitch_delta;
                camera->ang.y = camera->ang.y + yaw_delta;
                camera->ang.z = camera->ang.z + roll_delta;
            }
            remove_camera_offsets_impl();
            add_camera_offsets_impl();
            _mkproc_sleep_ticks = kOne;
            mkproc_sleep();
            rotation_ticks--;
        }
        RESOLVE_CAMERA_OBJ(camera);
        if (camera != 0) {
            camera_scale_v3(&forward_step, &cam_forward_uv, 2.0f);
            moving_look_target = camera->pos;
            moving_look_target.x += forward_step.x;
            moving_look_target.y += forward_step.y;
            moving_look_target.z += forward_step.z;
        }
    }

    get_target_movement_vector_impl(&moving_look_target, &look_target,
                                    &look_step, travel_time);

    while (move_to_end_point(&destination, &initial_speed, &final_speed, 0,
                             travel_time) == 0) {
        step_length = camera_sqrt(look_step.x * look_step.x +
                                  look_step.y * look_step.y +
                                  look_step.z * look_step.z);
        direction.x = look_target.x - moving_look_target.x;
        direction.y = look_target.y - moving_look_target.y;
        direction.z = look_target.z - moving_look_target.z;
        remaining_length = camera_sqrt(direction.x * direction.x +
                                       direction.y * direction.y +
                                       direction.z * direction.z);
        if (remaining_length < step_length) {
            moving_look_target.x = look_target.x;
            moving_look_target.y = look_target.y;
            moving_look_target.z = look_target.z;
        } else {
            moving_look_target.x += look_step.x;
            moving_look_target.y += look_step.y;
            moving_look_target.z += look_step.z;
        }

        RESOLVE_CAMERA_OBJ(camera);
        direction.x = moving_look_target.x - camera->pos.x;
        direction.y = moving_look_target.y - camera->pos.y;
        direction.z = moving_look_target.z - camera->pos.z;
        inverse_length = camera_inv_sqrt(direction.x * direction.x +
                                         direction.y * direction.y +
                                         direction.z * direction.z);
        direction.x *= inverse_length;
        direction.y *= inverse_length;
        direction.z *= inverse_length;
        v3_to_xy_ang_high_freq(&desired_angles, &direction);
        camera->ang.x = desired_angles.x;
        camera->ang.y = desired_angles.y;
        camera->ang.z = desired_angles.z;
        remove_camera_offsets_impl();
        add_camera_offsets_impl();
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }

    end_round_cam_done = 1;
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
    }
}

float p_mk_chess_cam_bezier_controller(void) {
    ChessCameraInfo* chess_camera = mk_chess_fetch_camera_info();
    CameraObj* camera;
    float remaining_ticks = chess_camera->look_at_ticks;
    float look_step_x;
    float look_step_y;
    float look_step_z;
    float inverse_ticks;
    float previous_ticks;

    RESOLVE_CAMERA_OBJ(camera);
    if (chess_camera->zoom_sound_enabled == 1) {
        mk_chess_enable_cam_zoom_sound(1);
    }
    inverse_ticks = kOne / remaining_ticks;
    look_step_x = (chess_camera->desired_look_at.x -
                   chess_camera->current_look_at.x) *
                  inverse_ticks;
    look_step_y = (chess_camera->desired_look_at.y -
                   chess_camera->current_look_at.y) *
                  inverse_ticks;
    look_step_z = (chess_camera->desired_look_at.z -
                   chess_camera->current_look_at.z) *
                  inverse_ticks;

    while (BezierCamera_GetNextPoint(&g_bezier_cam, &camera->pos) == 0) {
        previous_ticks = remaining_ticks;
        remaining_ticks -= kOne;
        if (previous_ticks > kZero) {
            chess_camera->current_look_at.x += look_step_x;
            chess_camera->current_look_at.y += look_step_y;
            chess_camera->current_look_at.z += look_step_z;
            chess_camera_look_at(&chess_camera->current_look_at);
        } else {
            if (chess_camera->look_at_completion != 0) {
                chess_camera->look_at_completion();
            }
            chess_camera->current_look_at.x =
                chess_camera->desired_look_at.x;
            chess_camera->current_look_at.y =
                chess_camera->desired_look_at.y;
            chess_camera->current_look_at.z =
                chess_camera->desired_look_at.z;
            chess_camera_look_at(&chess_camera->desired_look_at);
        }

        RESOLVE_CAMERA_OBJ(camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        camera->pos.x += cam_pos_offset.x;
        camera->pos.y += cam_pos_offset.y;
        camera->pos.z += cam_pos_offset.z;
        camera->ang.x += cam_ang_offset.x;
        camera->ang.y += cam_ang_offset.y;
        camera->ang.z += cam_ang_offset.z;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }

    if (remaining_ticks < kZero) {
        chess_camera_look_at(&chess_camera->desired_look_at);
    } else {
        chess_camera->desired_look_at.x =
            chess_camera->current_look_at.x;
        chess_camera->desired_look_at.y =
            chess_camera->current_look_at.y;
        chess_camera->desired_look_at.z =
            chess_camera->current_look_at.z;
    }
    mk_chess_enable_cam_zoom_sound(0);
    if (chess_camera->completion != 0) {
        chess_camera->completion();
    }
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
    }
}

/* Near miss: only the pooled 1.0f relocation label differs. */
void BezierCamera_SetOverallCameraTimeInTicks(BezierCamera* camera, float ticks) {
    ticks *= inverse_game_speed;
    camera->step = kOne / ticks;
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
        t = kOne;
        t2 = t * t;
        t3 = t2 * t;
        p2_weight = (-3.0f * t3) + (3.0f * t2);
        p1_weight = (3.0f * t3) - (6.0f * t2) + (3.0f * t);
        p0_weight =
            kOne + (((-kOne * t3) + (3.0f * t2)) - (3.0f * t));
        point->x = camera->control[0].x * p0_weight +
                   camera->control[1].x * p1_weight +
                   camera->control[2].x * p2_weight +
                   camera->control[3].x * t3;
        point->y = camera->control[0].y * p0_weight +
                   camera->control[1].y * p1_weight +
                   camera->control[2].y * p2_weight +
                   camera->control[3].y * t3;
        point->z = camera->control[0].z * p0_weight +
                   camera->control[1].z * p1_weight +
                   camera->control[2].z * p2_weight +
                   camera->control[3].z * t3;
        return 1;
    }

    t2 = t * t;
    t3 = t2 * t;
    p2_weight = (-3.0f * t3) + (3.0f * t2);
    p1_weight = (3.0f * t3) - (6.0f * t2) + (3.0f * t);
    p0_weight = kOne + (((-kOne * t3) + (3.0f * t2)) - (3.0f * t));

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
    ChessCameraInfo* chess_camera = mk_chess_fetch_camera_info();
    ChessManagerInfo* manager = mk_chess_fetch_manager_info();
    ChessPiece* piece = mk_chess_fetch_piece_at_cursor();
    CameraObj* camera;
    Vec destination;
    Vec movement;
    Vec direction;
    Vec angles;
    float distance;
    float inverse_length;
    float camera_x;
    int side;

    if (piece == 0 || manager->input_state != 0) {
        return kOne;
    }
    if (piece == chess_camera->viewing_camera) {
        return kOne;
    }

    RESOLVE_CAMERA_OBJ(camera);
    if (mk_chess_allow_setting_of_viewing_quadrant() != 0) {
        mk_chess_set_viewing_quadrant(camera);
    }

    destination.x = piece->object->pos.value.x;
    destination.y = piece->object->pos.value.y;
    destination.z = piece->object->pos.value.z;
    side = mk_chess_fetch_current_side_based_on_ones(manager->active_side);
    destination.y = destination.y + 11.0f;
    destination.z = 14.0f * -(float)side + destination.z;
    distance = dist_v3_to_v3(&destination, &camera->pos);
    if (distance > 0.25f) {
        distance = 0.25f;
    } else if (distance < 0.1f) {
        chess_camera->viewing_camera = piece;
        return kOne;
    }

    chess_camera->viewing_camera = 0;
    camera_x = camera->pos.x;
    movement.x = destination.x - camera_x;
    movement.y = destination.y - camera->pos.y;
    movement.z = destination.z - camera->pos.z;
    inverse_length = camera_inv_sqrt(
        movement.x * movement.x + movement.y * movement.y +
        movement.z * movement.z);
    movement.x *= inverse_length;
    movement.x *= distance;
    movement.y *= inverse_length;
    movement.y *= distance;
    movement.z *= inverse_length;
    movement.z *= distance;
    camera->pos.x = camera_x + movement.x;
    camera->pos.y += movement.y;
    camera->pos.z += movement.z;
    chess_camera->current_look_at.x += movement.x;
    chess_camera->current_look_at.y += movement.y;
    chess_camera->current_look_at.z += movement.z;

    RESOLVE_CAMERA_OBJ(camera);
    direction.x = chess_camera->current_look_at.x - camera->pos.x;
    direction.y = chess_camera->current_look_at.y - camera->pos.y;
    direction.z = chess_camera->current_look_at.z - camera->pos.z;
    inverse_length = camera_inv_sqrt(
        direction.x * direction.x + direction.y * direction.y +
        direction.z * direction.z);
    direction.x *= inverse_length;
    direction.y *= inverse_length;
    direction.z *= inverse_length;
    v3_to_xy_ang_high_freq(&angles, &direction);
    camera->ang.x = angles.x;
    camera->ang.y = angles.y;
    camera->ang.z = angles.z;
    return kOne;
}

float p_mk_chess_cam_control(void) {
    Vec origin = {0.0f, 0.0f, 0.0f};
    Vec rotated_position;
    Vec direction;
    Vec angles;
    Vec pitch_angles;
    RwMatrix yaw_matrix __attribute__((aligned(16)));
    RwMatrix inverse_matrix __attribute__((aligned(16)));
    RwMatrix pitch_matrix __attribute__((aligned(16)));
    RwMatrix temporary_matrix __attribute__((aligned(16)));
    RwMatrix result_matrix __attribute__((aligned(16)));
    RwMatrix* active_matrix;
    CameraObj* camera;
    float horizontal;
    float vertical;
    float inverse_length;
    float pitch_delta;
    int moved;

    for (;;) {
        if ((mk_chess_allow_cam_control() == 1 &&
             mk_chess_allow_setting_of_viewing_quadrant() != 0) ||
            (g_game_info.feature_flags.raw & 0x20) != 0) {
            moved = get_stick_pos(mk_chess_return_active_pad(), 1,
                                  &horizontal, &vertical);
            if ((g_game_info.feature_flags.raw & 0x20) != 0) {
                moved = mk_chess_fake_demo_cam(&horizontal);
            }
            if (moved != 0) {
                if (horizontal != kZero) {
                    y_angle_to_MKMATRIX(&yaw_matrix, 0.02f * horizontal);
                    v3_x_mat(&rotated_position, &camera_obj->pos, &yaw_matrix);
                    camera_obj->pos.x = rotated_position.x;
                    camera_obj->pos.y = rotated_position.y;
                    camera_obj->pos.z = rotated_position.z;
                    RESOLVE_CAMERA_OBJ(camera);
                    direction.x = origin.x - camera->pos.x;
                    direction.y = origin.y - camera->pos.y;
                    direction.z = origin.z - camera->pos.z;
                    inverse_length = camera_inv_sqrt(
                        direction.x * direction.x +
                        direction.y * direction.y +
                        direction.z * direction.z);
                    direction.x *= inverse_length;
                    direction.y *= inverse_length;
                    direction.z *= inverse_length;
                    v3_to_xy_ang_high_freq(&angles, &direction);
                    camera->ang.x = angles.x;
                    camera->ang.y = angles.y;
                    camera->ang.z = angles.z;
                }
                mk_chess_set_viewing_quadrant(camera_obj);
            }

            moved = get_stick_pos(mk_chess_return_active_pad(), 1,
                                  &horizontal, &vertical);
            if ((g_game_info.feature_flags.raw & 0x20) != 0) {
                moved = 0;
            }
            if (moved != 0) {
                if (vertical != kZero) {
                    pitch_delta = -0.02f * vertical;
                    active_matrix = camera_obj->field_24;
                    if ((vertical < kZero && active_matrix->up.y > 0.1f) ||
                        (vertical > kZero && active_matrix->up.y < 0.9f)) {
                        pitch_angles.x = pitch_delta;
                        pitch_angles.y = kZero;
                        pitch_angles.z = kZero;
                        RwMatrixInvert(&inverse_matrix, active_matrix);
                        YXZ_angles_to_MKMATRIX(&pitch_angles, &pitch_matrix);
                        mat_x_mat(&temporary_matrix, &inverse_matrix,
                                  &pitch_matrix);
                        mat_x_mat(&result_matrix, &temporary_matrix,
                                  active_matrix);
                        v3_x_mat(&rotated_position, &camera_obj->pos,
                                 &result_matrix);
                        camera_obj->pos.x = rotated_position.x;
                        camera_obj->pos.y = rotated_position.y;
                        camera_obj->pos.z = rotated_position.z;
                    }
                    RESOLVE_CAMERA_OBJ(camera);
                    direction.x = origin.x - camera->pos.x;
                    direction.y = origin.y - camera->pos.y;
                    direction.z = origin.z - camera->pos.z;
                    inverse_length = camera_inv_sqrt(
                        direction.x * direction.x +
                        direction.y * direction.y +
                        direction.z * direction.z);
                    direction.x *= inverse_length;
                    direction.y *= inverse_length;
                    direction.z *= inverse_length;
                    v3_to_xy_ang_high_freq(&angles, &direction);
                    camera->ang.x = angles.x;
                    camera->ang.y = angles.y;
                    camera->ang.z = angles.z;
                }
                mk_chess_set_viewing_quadrant(camera_obj);
            }
        }

        RESOLVE_CAMERA_OBJ(camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        camera->pos.x += cam_pos_offset.x;
        camera->pos.y += cam_pos_offset.y;
        camera->pos.z += cam_pos_offset.z;
        camera->ang.x += cam_ang_offset.x;
        camera->ang.y += cam_ang_offset.y;
        camera->ang.z += cam_ang_offset.z;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
        RESOLVE_CAMERA_OBJ(camera);
        camera->pos.x -= old_cam_pos_offset.x;
        camera->pos.y -= old_cam_pos_offset.y;
        camera->pos.z -= old_cam_pos_offset.z;
        camera->ang.x -= old_cam_ang_offset.x;
        camera->ang.y -= old_cam_ang_offset.y;
        camera->ang.z -= old_cam_ang_offset.z;
    }
}

/* Soft ceiling: exact size/math; only FPR and int-to-double scheduling differs. */
void camera_get_screen_pos_from_world_pos(const Vec* world, RwV2d* screen) {
    RwRaster* raster = Camera->frameBuffer;
    float y = world->y;
    float x = world->x;
    float z = world->z;
    int height = raster->height;
    float reciprocal_depth;
    float output_x;
    float output_y;

    reciprocal_depth =
        kOne / (Camera->viewMatrix.pos.z +
                (z * Camera->viewMatrix.at.z +
                 (x * Camera->viewMatrix.right.z +
                  y * Camera->viewMatrix.up.z)));
    output_x =
        (float)raster->width *
            ((Camera->viewMatrix.pos.x +
              (z * Camera->viewMatrix.at.x +
               (x * Camera->viewMatrix.right.x +
                y * Camera->viewMatrix.up.x))) *
             reciprocal_depth) +
        (float)raster->offsetX;
    output_y =
        (float)height -
        ((float)height *
             ((Camera->viewMatrix.pos.y +
               (z * Camera->viewMatrix.at.y +
                (x * Camera->viewMatrix.right.y +
                 y * Camera->viewMatrix.up.y))) *
              reciprocal_depth) +
         (float)raster->offsetY);
    screen->x = output_x;
    screen->y = output_y;
}

void remove_widescreen_bars(void) {
    ScreenObj* upper;
    ScreenObj* lower;
    WidescreenBarPdata* pdata = 0;

    upper = upper_fade_box_item.node;
    if (upper != 0) {
        if (upper->instance != upper_fade_box_item.instance) {
            upper = 0;
        }
    } else {
        upper = 0;
    }
    lower = lower_fade_box_item.node;
    if (lower != 0) {
        if (lower->instance != lower_fade_box_item.instance) {
            lower = 0;
        }
    } else {
        lower = 0;
    }
    if (find_mkproc_pid(0x8229) != 0) {
        destroy_mkprocs_pid(0x8229);
    }
    if (mode_of_play == 7) {
        konquest_pdata->widescreen_bars_active = 0;
    }
    if (upper != 0) {
        if (lower == 0) {
            return;
        }
        _create_mkproc_generic_tinystack(
            0x8229, 0x1F, p_move_widescreen_bars,
            sizeof(WidescreenBarPdata), (MkHdr**)&pdata);
        if (pdata != 0) {
            pdata->step = kZero;
            pdata->direction = 0;
        }
    }
}

/* Soft ceiling: exact size; latch diamonds and Pfx2d load/FPR scheduling only. */
static float p_move_widescreen_bars(void) {
    ScreenObj* upper;
    ScreenObj* lower;
    WidescreenBarPdata* pdata;
    float left_edge;
    float step;

    pdata = (WidescreenBarPdata*)pdata_of_proc(aproc);
    left_edge = (float)-(screen_width - 0x280) * kHalf;
    upper = upper_fade_box_item.node;
    if (upper != 0) {
        if (upper->instance != upper_fade_box_item.instance) {
            upper = 0;
        }
    } else {
        upper = 0;
    }
    lower = lower_fade_box_item.node;
    if (lower != 0) {
        if (lower->instance != lower_fade_box_item.instance) {
            lower = 0;
        }
    } else {
        lower = 0;
    }
    if (upper == 0 || lower == 0) {
        return kNegOne;
    }

    if (pdata != 0) {
        if (pdata->step != kZero) {
            step = pdata->step / 60.0f;
            lower->pfx2d->verts[0].x = left_edge;
            lower->pfx2d->verts[1].x = left_edge;
            lower->pfx2d->verts[2].x = (float)screen_width;
            lower->pfx2d->verts[3].x = (float)screen_width;
            lower->pfx2d->verts[0].y = kZero;
            lower->pfx2d->verts[1].y = kZero;
            lower->pfx2d->verts[2].y = kZero;
            lower->pfx2d->verts[3].y = kZero;
            upper->pfx2d->verts[0].x = left_edge;
            upper->pfx2d->verts[1].x = left_edge;
            upper->pfx2d->verts[2].x = (float)screen_width;
            upper->pfx2d->verts[3].x = (float)screen_width;
            upper->pfx2d->verts[0].y = 480.0f;
            upper->pfx2d->verts[1].y = 480.0f;
            upper->pfx2d->verts[2].y = 480.0f;
            upper->pfx2d->verts[3].y = 480.0f;
            while (lower->pfx2d->verts[0].y < pdata->step) {
                lower->pfx2d->verts[0].y += step;
                lower->pfx2d->verts[3].y += step;
                lower->pfx2d->mirror = 1;
                upper->pfx2d->verts[1].y -= 1.1666666f;
                upper->pfx2d->verts[2].y -= 1.1666666f;
                upper->pfx2d->mirror = 1;
                _mkproc_sleep_ticks = kOne;
                mkproc_sleep();
            }
        } else {
            step = lower->pfx2d->verts[0].y / 60.0f;
            while (lower->pfx2d->verts[0].y > kZero) {
                lower->pfx2d->verts[0].y -= step;
                lower->pfx2d->verts[3].y -= step;
                lower->pfx2d->mirror = 1;
                upper->pfx2d->verts[1].y += 1.1666666f;
                upper->pfx2d->verts[2].y += 1.1666666f;
                upper->pfx2d->mirror = 1;
                _mkproc_sleep_ticks = kOne;
                mkproc_sleep();
            }
        }
    }

    if (pdata->direction == 0) {
        if (lower != 0) {
            if (lower->instance != 0) {
                lower->typed_vtbl->destroy(lower);
            }
            lower_fade_box_item.node = 0;
            lower_fade_box_item.instance = 0;
        }
        if (upper != 0) {
            if (upper->instance != 0) {
                upper->typed_vtbl->destroy(upper);
            }
            upper_fade_box_item.node = 0;
            upper_fade_box_item.instance = 0;
        }
    } else if (mode_of_play == 7 && konquest_pdata != 0) {
        konquest_pdata->widescreen_bars_active = 1;
    }
    return kNegOne;
}

void add_widescreen_bars(float height) {
    ScreenObj* upper;
    ScreenObj* lower;
    WidescreenBarPdata* pdata = 0;

    upper = upper_fade_box_item.node;
    if (upper != 0) {
        if (upper->instance != upper_fade_box_item.instance) {
            upper = 0;
        }
    } else {
        upper = 0;
    }
    lower = lower_fade_box_item.node;
    if (lower != 0) {
        if (lower->instance != lower_fade_box_item.instance) {
            lower = 0;
        }
    } else {
        lower = 0;
    }

    if (find_mkproc_pid(0x8229) != 0) {
        destroy_mkprocs_pid(0x8229);
        if (lower != 0) {
            if (lower->instance != 0) {
                lower->typed_vtbl->destroy(lower);
            }
            lower_fade_box_item.node = 0;
            lower_fade_box_item.instance = 0;
        }
        if (upper != 0) {
            if (upper->instance != 0) {
                upper->typed_vtbl->destroy(upper);
            }
            upper_fade_box_item.node = 0;
            upper_fade_box_item.instance = 0;
        }
    }

    upper = load_2d_pfxobj(0, 0x2098, (char*)0x10017, 0, 0xF);
    lower = load_2d_pfxobj(0, 0x2098, (char*)0x10017, 0, 0xF);
    if (upper != 0) {
        if (lower == 0) {
            return;
        }
        upper_fade_box_item.node = upper;
        upper_fade_box_item.instance = upper->instance;
        lower_fade_box_item.node = lower;
        lower_fade_box_item.instance = lower->instance;
        snd_req(0x15A4);
        _create_mkproc_generic_tinystack(
            0x8229, 0x1F, p_move_widescreen_bars,
            sizeof(WidescreenBarPdata), (MkHdr**)&pdata);
        if (pdata != 0) {
            pdata->step = height;
            pdata->direction = 1;
        }
        if (mode_of_play == 7 && konquest_pdata != 0) {
            konquest_pdata->widescreen_bars_active = 0;
        }
    }
}

void cam_set_intro_cam_speed(float speed) {
    camera_info.pdata->speed = speed;
}

void cam_set_intro_cam_pause_ticks(float ticks) {
    camera_info.pdata->pause_ticks = ticks;
}

void camera_set_anim_aux_data(CameraAnimEvent* data) {
    camera_info.pdata->aux_data = data;
}

void camera_set_animation_mirror_plane(int mode) {
    CameraInfo* info = &camera_info;
    CameraPdata* pdata = info->pdata;

    pdata->mat_mirror.at.z = kOne;
    pdata->mat_mirror.up.y = kOne;
    pdata->mat_mirror.right.x = kOne;
    pdata->mat_mirror.up.x = kZero;
    pdata->mat_mirror.right.z = kZero;
    pdata->mat_mirror.right.y = kZero;
    pdata->mat_mirror.at.y = kZero;
    pdata->mat_mirror.at.x = kZero;
    pdata->mat_mirror.up.z = kZero;
    pdata->mat_mirror.pos.z = kZero;
    pdata->mat_mirror.pos.y = kZero;
    pdata->mat_mirror.pos.x = kZero;
    pdata->mat_mirror.flags |= 0x20003;

    switch (mode) {
    case 0:
        return;
    case 1:
        pdata->mat_mirror.up.y = kNegOne;
        break;
    case 2:
        pdata->mat_mirror.at.z = kNegOne;
        break;
    case 3:
        pdata->mat_mirror.right.x = kNegOne;
        break;
    }

    pdata->mat_mirror.flags &= ~0x20000;
    info->pdata->flags_bits.animation_mirror = 1;
}

/* Retail keeps these three component stores in source order. */
void camera_set_animation_parent_position(CamVec3* position) {
    camera_info.pdata->mat_parent.pos.x = position->x;
    camera_info.pdata->mat_parent.pos.y = position->y;
    camera_info.pdata->mat_parent.pos.z = position->z;
}

void camera_set_animation_parent_angle(const CamVec3* angle, int relative) {
    camera_info.pdata->flags_bits.parent_relative = relative;
    YXZ_angles_to_MKMATRIX((const Vec*)angle, &camera_info.pdata->mat_parent);
}

void camera_wait_for_animation_completion(void) {
    while (camera_info.proc->entry == p_animate_camera_move) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
}

void camera_run_animation_start_end(float start_frame, float end_frame, int wait_flag,
                                    int use_frame_range) {
    CameraInfo* info = &camera_info;
    MkObj* bone;
    AnimPdata* anim;

    bone = info->pdata->bone_obj;
    if (bone != 0 && bone->hdr.instance != info->pdata->bone_instance) {
        bone = 0;
    }
    if (bone == 0) {
        bone = (MkObj*)load_named_model_from_slot(0, "CAM_BONE", 0x900D, 0);
        if (bone != 0) {
            mk_insert(&bone->hdr, &camera_obj->child_list);
            info->pdata->bone_obj = bone;
            info->pdata->bone_instance = bone->hdr.instance;
            build_bones_tbl(bone, camera_bones);
            bone->flags_0B_bits.force_anim_speed = 1;
        }
    }

    if (bone != 0) {
        anim = info->pdata->anim_pdata;
        if (anim != 0 && anim->hdr.instance != info->pdata->anim_instance) {
            anim = 0;
        }
        if (anim == 0) {
            anim = get_mkpdata_anim();
            anim->obj = bone;
            anim->obj_instance = bone->hdr.instance;
            set_root_and_obj_movement_weights(kZero, kZero, anim);
            anim->step = info->pdata->speed;
            mk_insert(&anim->hdr, &bone->child_list);
            info->pdata->anim_pdata = anim;
            info->pdata->anim_instance = anim->hdr.instance;
        }
    }

    anim = info->pdata->anim_pdata;
    if (anim != 0 && anim->hdr.instance != info->pdata->anim_instance) {
        anim = 0;
    }
    if (use_frame_range == 0) {
        start_frame = kZero;
    }
    set_anim_script_frame(start_frame, anim, info->pdata->anim_path, 0x23);
    if (use_frame_range != 0) {
        anim->low_frame = start_frame;
        anim->high_frame = end_frame;
    }
    xfer_camera_impl(p_animate_camera_move, 1);
    if (wait_flag != 0) {
        camera_wait_for_animation_completion();
    }
}

void camera_run_animation(int wait_flag) {
    camera_run_animation_start_end(kZero, kZero, wait_flag, 0);
}

void camera_init_animation(AniData* anim_path, MkProcEntryFn override_entry) {
    MkProcEntryFn saved;

    camera_info.pdata->anim_path = anim_path;
    if (override_entry != 0) {
        saved = override_entry;
    } else {
        saved = camera_info.proc->entry;
    }
    old_camera_function = saved;
    MKMatrixSetIdentity(&camera_info.pdata->mat_parent);
    MKMatrixSetIdentity(&camera_info.pdata->mat_mirror);
    camera_info.pdata->flags_bits.animation_mirror = 0;
    camera_info.pdata->speed = kOne;
    camera_info.pdata->aux_data = 0;
}

float p_animate_and_freeze(void) {
    ((MkObj*)camera_obj)->flags_word_08 = 0;
    while (1) {
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
    }
}

float p_animated_intro_done(void) {
    CameraObj* camera;

    end_round_cam_done = 0;
    g_game_info.pause_flag_bits.pad_bit6 = 1;
    camera_mode = 1;
    force_midpoint_calculation_update = 0;
    {
        float initial_speed;
        float final_speed;
        Vec endpoint = {0.0f, 0.0f, 0.0f};

        initial_speed = kZero;
        final_speed = kZero;

        move_to_end_point(&endpoint, &initial_speed, &final_speed, 1, kZero);
        orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1, 1,
                                    kZero);
    }

    camera = camera_obj;
    camera_info.pdata->target_pos.x = camera->pos.x;
    camera_info.pdata->target_pos.y = camera->pos.y;
    camera_info.pdata->target_pos.z = camera->pos.z;
    camera_info.pdata->flags_bits.pos_done = 0;
    camera = camera_obj;
    camera_info.pdata->target_ang.x = camera->ang.x;
    camera_info.pdata->target_ang.y = camera->ang.y;
    camera_info.pdata->target_ang.z = camera->ang.z;

    while (1) {
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
    }
}

static float p_animate_camera_move(void) {
    CameraInfo* info = &camera_info;
    MkObj* bone;
    AnimPdata* animation;
    CameraObj* active_camera;
    CameraAnimEvent* event;
    RwMatrix bone_matrix __attribute__((aligned(16)));
    RwMatrix final_matrix __attribute__((aligned(16)));
    Vec transformed_offset;
    Vec world_offset;
    Vec forward;
    unsigned int saved_flags;
    float pause_ticks;
    float event_frame;
    int pose_frame;

    bone = info->pdata->bone_obj;
    if (bone != 0 && bone->hdr.instance != info->pdata->bone_instance) {
        bone = 0;
    }
    if (bone == 0) {
        mkproc_jump_sleep(old_camera_function);
        return kZero;
    }

    animation = info->pdata->anim_pdata;
    if (animation != 0 &&
        animation->hdr.instance != info->pdata->anim_instance) {
        animation = 0;
    }
    if (animation == 0) {
        mkproc_jump_sleep(old_camera_function);
        return kZero;
    }

    bone->pos.value = camera_obj->pos;
    bone->ang = camera_obj->ang;
    animation->frame = kZero;
    saved_flags = ((MkObj*)camera_obj)->flags_word_08;
    camera_obj->flags.pad3 = 0;
    camera_obj->flags.bit04 = 0;
    camera_obj->flags.pad6 = 0;
    camera_obj->flags.bit20 = 0;
    pause_ticks = info->pdata->pause_ticks;
    info->pdata->pause_ticks = kZero;

    camera_matrix_set_identity(&camera_anim_fixup_matrix);
    camera_anim_fixup_matrix.right.x = kNegOne;
    camera_anim_fixup_matrix.at.z = kNegOne;
    camera_anim_fixup_matrix.flags = 0;
    camera_turn_on_impl();

    for (;;) {
        quat_to_mat(&bone_matrix, &bone->bones[0]->rotation);
        mat_x_mat(&final_matrix, &camera_anim_fixup_matrix, &bone_matrix);
        if (info->pdata->flags_bits.animation_mirror) {
            mat_x_mat(&bone_matrix, &final_matrix, &info->pdata->mat_mirror);
            bone_matrix.right.x = -bone_matrix.right.x;
            bone_matrix.right.y = -bone_matrix.right.y;
            bone_matrix.right.z = -bone_matrix.right.z;
        } else {
            set_mat(&bone_matrix, &final_matrix);
        }

        if (info->pdata->flags_bits.parent_relative) {
            mat_x_mat(&final_matrix, &info->pdata->mat_parent, &bone_matrix);
        } else {
            mat_x_mat(&final_matrix, &bone_matrix, &info->pdata->mat_parent);
        }

        v3_x_mat(&transformed_offset, &animation->root_offset,
                 &info->pdata->mat_mirror);
        v3_x_mat(&world_offset, &transformed_offset, &info->pdata->mat_parent);
        world_offset.x += info->pdata->mat_parent.pos.x;
        world_offset.y += info->pdata->mat_parent.pos.y;
        world_offset.z += info->pdata->mat_parent.pos.z;
        camera_mat->pos.x = world_offset.x;
        camera_mat->pos.y = world_offset.y;
        camera_mat->pos.z = world_offset.z;

        active_camera = camera_item.node;
        if (active_camera != 0 &&
            active_camera->hdr.instance != camera_item.instance) {
            active_camera = 0;
        }
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        if (active_camera != 0) {
            info->pdata->mat_offset.pos.x = cam_pos_offset.x;
            info->pdata->mat_offset.pos.y = cam_pos_offset.y;
            info->pdata->mat_offset.pos.z = cam_pos_offset.z;
            YXZ_angles_to_MKMATRIX(&cam_ang_offset,
                                   &info->pdata->mat_offset);
        }

        mat_x_mat(camera_mat, &final_matrix, &info->pdata->mat_offset);
        camera_obj->pos.x = camera_mat->pos.x;
        camera_obj->pos.y = camera_mat->pos.y;
        camera_obj->pos.z = camera_mat->pos.z;
        v3_x_mat(&forward, &unit_z, camera_mat);
        v3_to_xy_ang_high_freq(&camera_obj->ang, &forward);
        animation->step = info->pdata->speed;

        if (pause_ticks > kZero && animation->frame == kZero) {
            _mkproc_sleep_ticks = pause_ticks;
            mkproc_sleep();
            pause_ticks = kZero;
            continue;
        }

        pose_frame = 1;
        event = info->pdata->aux_data;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
        advance_anim(animation);
        if (event != 0) {
            event_frame = (float)(int)event->frame * inverse_game_speed;
            if (animation->frame > event_frame) {
                if (event->type == 4) {
                    info->pdata->aux_data = 0;
                } else {
                    switch (event->type) {
                    case 0:
                        if (animation->frame <= (double)event_frame + 1.0) {
                            pose_frame = 0;
                        }
                        break;
                    case 1:
                        pan_vol_snd_req(event->argument, event->pan,
                                        event->volume);
                        break;
                    case 2:
                        one_shot_script_func(g_game_info.cmdscript,
                                             event->argument, 0);
                        break;
                    case 3:
                        one_shot_script_func(konquest_pdata->camera_script,
                                             event->argument, 0);
                        break;
                    }
                    info->pdata->aux_data++;
                }
            }
        }
        if (pose_frame != 0) {
            pose_anim(animation, 1);
        }
        if (animation->frame + animation->step * game_speed >
            animation->high_frame) {
            break;
        }
    }

    ((MkObj*)camera_obj)->flags_word_08 = saved_flags;
    mkproc_jump_sleep(old_camera_function);
    return kZero;
}

void* get_intro_camera_path(void) {
    return camera_info.pdata->intro_path;
}

void set_intro_camera_path(void* path) {
    camera_info.pdata->intro_path = path;
}

static RpMaterial* set_material_alpha(RpMaterial* material,
                                      const unsigned char* alpha) {
    unsigned char value = *alpha;
    if (material != 0) {
        material->color.alpha = value;
    }
    return material;
}

static int is_shape_in_frustum(const Vec* position,
                               const CollisionShape* shape) {
    Vec from_center_to_position = {0.0f, 0.0f, 0.0f};
    Vec from_player_1_to_position = {0.0f, 0.0f, 0.0f};
    Vec from_player_2_to_position = {0.0f, 0.0f, 0.0f};
    Vec center = {0.0f, 0.0f, 0.0f};
    Vec closest = {0.0f, 0.0f, 0.0f};
    Vec from_center_to_closest = {0.0f, 0.0f, 0.0f};
    float projection;
    float player_dot;
    float player_1_dot;
    float player_2_dot;

    switch (shape->type & 7) {
    case 3:
        return 0;
    case 1:
        center.x = shape->sphere_center.x;
        center.y = shape->sphere_center.y;
        center.z = shape->sphere_center.z;
        break;
    case 2:
        center.x = shape->cylinder_center.x;
        center.y = shape->cylinder_center.y;
        center.z = shape->cylinder_center.z;
        break;
    default:
        break;
    }

    from_center_to_position.x = position->x - center.x;
    from_center_to_position.z = position->z - center.z;
    from_player_1_to_position.x =
        position->x - g_game_info.plyr0.slot.mirror_a->pos.value.x;
    from_player_1_to_position.z =
        position->z - g_game_info.plyr0.slot.mirror_a->pos.value.z;
    if (xz_dot_xz(&from_center_to_position,
                  &from_player_1_to_position) <= kZero) {
        return 0;
    }

    projection =
        ((center.x - g_game_info.plyr0.slot.mirror_a->pos.value.x) *
             (g_game_info.plyr1.slot.mirror_a->pos.value.x -
              g_game_info.plyr0.slot.mirror_a->pos.value.x) +
         (center.z - g_game_info.plyr0.slot.mirror_a->pos.value.z) *
             (g_game_info.plyr1.slot.mirror_a->pos.value.z -
              g_game_info.plyr0.slot.mirror_a->pos.value.z));
    projection /=
        dist2_xz_to_xz(&g_game_info.plyr1.slot.mirror_a->pos.value,
                       &g_game_info.plyr0.slot.mirror_a->pos.value);
    if (projection > 1.0) {
        return 0;
    }

    closest.x = g_game_info.plyr0.slot.mirror_a->pos.value.x +
                projection * (g_game_info.plyr1.slot.mirror_a->pos.value.x -
                              g_game_info.plyr0.slot.mirror_a->pos.value.x);
    closest.z = g_game_info.plyr0.slot.mirror_a->pos.value.z +
                projection * (g_game_info.plyr1.slot.mirror_a->pos.value.z -
                              g_game_info.plyr0.slot.mirror_a->pos.value.z);
    from_center_to_closest.x = closest.x - center.x;
    from_center_to_closest.z = closest.z - center.z;
    if (xz_dot_xz(&from_center_to_closest,
                  &from_player_1_to_position) >= kOne) {
        return 0;
    }

    from_player_2_to_position.x =
        position->x - g_game_info.plyr1.slot.mirror_a->pos.value.x;
    from_player_2_to_position.z =
        position->z - g_game_info.plyr1.slot.mirror_a->pos.value.z;
    normalize_xz(&from_center_to_position);
    normalize_xz(&from_player_1_to_position);
    normalize_xz(&from_player_2_to_position);
    player_dot = xz_dot_xz(&from_player_1_to_position,
                           &from_player_2_to_position);
    player_1_dot = xz_dot_xz(&from_player_1_to_position,
                             &from_center_to_position);
    player_2_dot = xz_dot_xz(&from_player_2_to_position,
                             &from_center_to_position);
    if (player_dot < player_1_dot && player_dot < player_2_dot &&
        xz_dot_xz(&from_center_to_closest,
                  &from_center_to_closest) > kOne) {
        return 1;
    }
    return 0;
}

void toggle_danger_zone(int index) {
    if (index < 0) {
        return;
    }
    if (index > number_of_danger_zones) {
        return;
    }
    background_danger_zones[index].enabled ^= 1;
}

int keep_camera_out_of_danger_zones(Vec* position) {
    /* Retail clears this complete 16-byte result block before the pause gate. */
    typedef struct DangerZoneScratch {
        float field_00;
        float radius_squared;
        float distance_squared;
        float push_height;
    } DangerZoneScratch;

    int adjusted = 0;
    int inside = 0;
    int index;
    BackgroundDangerZone* zone;
    MkSobj* linked_sobj = 0;
    unsigned char alpha;
    float radius_squared;
    DangerZoneScratch scratch;

    memset(&scratch, 0, sizeof(scratch));

    if (!g_game_info.pause_flag_bits.pad_bit6) {
        return 0;
    }
    for (index = 0; index < number_of_danger_zones; index++) {
        zone = &background_danger_zones[index];
        if (zone->enabled != 0) {
            continue;
        }
        if (zone->object != 0) {
            linked_sobj =
                *(MkSobj**)((char*)zone->object + MksobjLocalOffset + 8);
        }

        switch (zone->shape.type & 7) {
        case 3:
            if (zone->test_mode == 0) {
                inside = is_point_inside_shape(&zone->shape, position);
            }
            break;
        case 2:
            switch (zone->test_mode) {
            case 1:
                inside = is_shape_in_frustum(position, &zone->shape);
                break;
            case 0:
                inside = is_point_inside_shape(&zone->shape, position);
                break;
            }
            break;
        case 1:
            inside = 0;
            if (zone->shape.type != 0) {
                radius_squared = zone->shape.sphere_radius *
                                 zone->shape.sphere_radius;
                scratch.distance_squared =
                    dist2_v3_to_v3(position, &zone->shape.sphere_center);
                scratch.radius_squared = radius_squared;
                if (scratch.distance_squared < scratch.radius_squared) {
                    inside = 1;
                }
            }
            if (zone->test_mode == 1 && !inside) {
                inside = is_shape_in_frustum(position, &zone->shape);
            }
            if (inside && zone->action == 0) {
                scratch.push_height = camera_sqrt(
                    scratch.radius_squared - scratch.distance_squared);
                scratch.push_height += zone->shape.sphere_center.y;
            }
            break;
        }

        if (inside) {
            switch (zone->action) {
            case 0:
                if (position->y < scratch.push_height) {
                    adjusted = 1;
                    zone->cooldown = 1;
                    position->y = scratch.push_height;
                }
                break;
            case 1:
                if (zone->cooldown == 0) {
                    alpha = 0x60;
                    if (zone->object->geometry != 0) {
                        RpGeometryForAllMaterials(
                            zone->object->geometry,
                            (RpMaterialCallBack)set_material_alpha, &alpha);
                    }
                    zone->cooldown = 60;
                    if (linked_sobj != 0) {
                        linked_sobj->flags09_bits.bit7 = 1;
                    }
                }
                break;
            case 2:
                if (zone->cooldown == 0) {
                    hide_atomic(zone->object);
                    zone->cooldown = 60;
                }
                break;
            }
        } else if (zone->cooldown != 0) {
            zone->cooldown--;
            if (zone->cooldown <= 0) {
                zone->cooldown = 0;
                if (zone->action == 1) {
                    alpha = 0xFF;
                    if (zone->object->geometry != 0) {
                        RpGeometryForAllMaterials(
                            zone->object->geometry,
                            (RpMaterialCallBack)set_material_alpha, &alpha);
                    }
                    if (linked_sobj != 0) {
                        linked_sobj->flags09_bits.bit7 = 0;
                    }
                } else if (zone->action == 2) {
                    unhide_atomic(zone->object);
                }
            }
        }
    }
    return adjusted;
}

BackgroundDangerZone* add_background_danger_zone(RpAtomic* atomic, int type,
                                                  int mode) {
    BackgroundDangerZone* zone;
    RwSphere* sphere;
    DangerZoneAtomicView* atomic_view;

    if (number_of_danger_zones >= 37) {
        return 0;
    }
    if ((type == 1 || type == 2) && atomic == 0) {
        return 0;
    }

    zone = &background_danger_zones[number_of_danger_zones];
    zone->object = atomic;
    zone->action = type;
    zone->test_mode = mode;
    zone->enabled = 0;

    if (atomic != 0) {
        sphere = RpAtomicGetWorldBoundingSphere(atomic);
        zone->shape.sphere_center.x = sphere->center.x;
        zone->shape.sphere_center.y = sphere->center.y;
        zone->shape.sphere_center.z = sphere->center.z;
        zone->shape.sphere_radius = sphere->radius;
        zone->shape.type = 1;
        if (type == 1) {
            atomic_view = (DangerZoneAtomicView*)atomic;
            if (atomic_view->field_18 != 0) {
                atomic_view->field_18->flags |= 0x40;
            }
            atomic_set_transl_flag(atomic);
        }
    }

    number_of_danger_zones++;
    return zone;
}

void hide_sobj_if_camera_is_in_rectangle(MkSobj* object, const Vec* center, float min_x,
                                         float min_z, float max_x,
                                         float max_z) {
    BackgroundDangerZone* zone = add_background_danger_zone(object->atomic, 1, 0);

    build_col_shape_vertical_box(&zone->shape, center, min_x, min_z, max_x,
                                 max_z);
    zone->shape.type = 3;
}

void turn_off_sobj_if_camera_is_in_rectangle(MkSobj* object, const Vec* center,
                                             float min_x, float min_z,
                                             float max_x, float max_z) {
    BackgroundDangerZone* zone = add_background_danger_zone(object->atomic, 2, 0);

    build_col_shape_vertical_box(&zone->shape, center, min_x, min_z, max_x,
                                 max_z);
    zone->shape.type = 3;
}

void hide_sobj_if_camera_is_in_cylinder(MkSobj* object, const Vec* center, float radius,
                                        float height) {
    BackgroundDangerZone* zone = add_background_danger_zone(object->atomic, 1, 0);

    build_col_shape_vertical_cylinder(&zone->shape, center, radius, height);
    zone->shape.type = 2;
}

void turn_off_sobj_if_camera_is_in_cylinder(MkSobj* object, const Vec* center,
                                            float radius, float height) {
    BackgroundDangerZone* zone = add_background_danger_zone(object->atomic, 2, 0);

    build_col_shape_vertical_cylinder(&zone->shape, center, radius, height);
    zone->shape.type = 2;
}

void set_danger_zone_properties(BackgroundDangerZone* zone, float scale,
                                float y_offset) {
    zone->shape.sphere_radius *= scale;
    zone->shape.sphere_center.y += y_offset;
}

void render_background_danger_areas(void) {
    int index;

    for (index = 0; index < number_of_danger_zones; index++) {
        if (background_danger_zones[index].cooldown != 0) {
            render_col_shape(&background_danger_zones[index].shape, &rgba_red);
        } else {
            render_col_shape(&background_danger_zones[index].shape, &rgba_green);
        }
    }
}

void initialize_background_danger_zones(void) {
    number_of_danger_zones = 0;
    memset(background_danger_zones, 0, sizeof(background_danger_zones));
}

float p_krypt_camera_loop(void) {
    union {
        float f;
        unsigned int u;
    } value_bits, guess_bits;
    CameraPdata* pdata;
    CameraObj* cam;
    CameraPdataFlags* flag_bits;
    CameraVecBits default_pos;
    CameraVecBits default_ang;
    float speed_scale;
    float dx;
    float dy;
    float dz;
    float dist_sq;
    float dist;
    float d_ang_x;
    float d_ang_y;
    float roll_delta;
    float ang_err_sq;
    unsigned int bits;
    float guess;
    int pos_done;

    /* Word copies of @2947 / @2948 onto stack (retail lwz/stw shape). */
    default_pos.words[0] = kDefaultPos.words[0];
    default_pos.words[1] = kDefaultPos.words[1];
    default_pos.words[2] = kDefaultPos.words[2];
    default_ang.words[0] = kDefaultAng.words[0];
    default_ang.words[1] = kDefaultAng.words[1];
    default_ang.words[2] = kDefaultAng.words[2];

    pdata = camera_info.pdata;
    if (pdata == 0) {
        return kNegOne;
    }

    /* Unconditional stores after resolve (retail may write through NULL). */
    RESOLVE_CAMERA_OBJ(cam);
    cam->pos.x = default_pos.values[0];
    cam->pos.y = default_pos.values[1];
    cam->pos.z = default_pos.values[2];

    RESOLVE_CAMERA_OBJ(cam);
    cam->ang.x = default_ang.values[0];
    cam->ang.y = default_ang.values[1];
    cam->ang.z = default_ang.values[2];

    pdata->target_pos.x = default_pos.values[0];
    pdata->target_pos.y = default_pos.values[1];
    pdata->target_pos.z = default_pos.values[2];
    pdata->target_ang.x = default_ang.values[0];
    pdata->target_ang.y = default_ang.values[1];
    pdata->target_ang.z = default_ang.values[2];

    for (;;) {
        speed_scale = kMoveScale * pdata->speed;
        RESOLVE_CAMERA_OBJ(cam);

        dy = pdata->target_pos.y - cam->pos.y;
        dx = pdata->target_pos.x - cam->pos.x;
        dz = pdata->target_pos.z - cam->pos.z;
        dist_sq = dx * dx + dy * dy + dz * dz;

        /* Inlined gxMathSqrt (retail embeds table Newton step). */
        if (dist_sq <= kZero) {
            dist = kZero;
        } else {
            value_bits.f = dist_sq;
            bits = value_bits.u;
            guess_bits.u =
                (unsigned int)GXMathSqrtTable[(bits >> 10) & 0x3FFE] << 8;
            guess_bits.u |=
                (((bits & 0x7F800000U) + 0x3F800000U) >> 1) &
                0x7F800000U;
            guess = guess_bits.f;
            guess *= kThree - (guess * guess) / dist_sq;
            dist = kHalf * guess;
        }

        if (dist < kSnapDist) {
            cam->pos.x = pdata->target_pos.x;
            cam->pos.y = pdata->target_pos.y;
            cam->pos.z = pdata->target_pos.z;
            pos_done = 1;
        } else {
            dx *= speed_scale;
            dy *= speed_scale;
            dz *= speed_scale;
            cam->pos.x += dx;
            cam->pos.y += dy;
            cam->pos.z += dz;
            pos_done = 0;
        }

        flag_bits = &pdata->flags_bits;
        if (pos_done != 0) {
            flag_bits->pos_done = 1;
        } else {
            flag_bits->pos_done = 0;
        }

        speed_scale = kMoveScale * pdata->speed;
        RESOLVE_CAMERA_OBJ(cam);

        d_ang_x = pdata->target_ang.x - cam->ang.x;
        if (d_ang_x > kPi) {
            d_ang_x = d_ang_x - kTwoPi;
        } else if (d_ang_x < kNegPi) {
            d_ang_x = d_ang_x + kTwoPi;
        }

        d_ang_y = pdata->target_ang.y - cam->ang.y;
        if (d_ang_y > kPi) {
            d_ang_y = d_ang_y - kTwoPi;
        } else if (d_ang_y < kNegPi) {
            d_ang_y = d_ang_y + kTwoPi;
        }

        ang_err_sq = d_ang_x * d_ang_x + d_ang_y * d_ang_y;

        if (ang_err_sq < kAngEpsSq) {
            cam->ang.x = pdata->target_ang.x;
            cam->ang.y = pdata->target_ang.y;
            cam->ang.z = pdata->target_ang.z;
        } else {
            d_ang_x *= speed_scale;
            d_ang_y *= speed_scale;
            roll_delta = kZero * speed_scale;
            cam->ang.x += d_ang_x;
            cam->ang.y += d_ang_y;
            cam->ang.z += roll_delta;
        }

        RESOLVE_CAMERA_OBJ(cam);

        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;

        cam->pos.x = cam->pos.x + cam_pos_offset.x;
        cam->pos.y = cam->pos.y + cam_pos_offset.y;
        cam->pos.z = cam->pos.z + cam_pos_offset.z;
        cam->ang.x = cam->ang.x + cam_ang_offset.x;
        cam->ang.y = cam->ang.y + cam_ang_offset.y;
        cam->ang.z = cam->ang.z + cam_ang_offset.z;

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
    CameraObj* camera;
    Vec target_position = {0.0f, 0.0f, 0.0f};
    Vec target_angles = {0.0f, 0.0f, 0.0f};
    float dx;
    float dy;
    float dz;
    float distance;
    float pitch_delta;
    float yaw_delta;

    RESOLVE_CAMERA_OBJ(camera);
    target_position.x = camera->pos.x;
    target_position.y = camera->pos.y;
    target_position.z = camera->pos.z;
    RESOLVE_CAMERA_OBJ(camera);
    target_angles.x = camera->ang.x;
    target_angles.y = camera->ang.y;
    target_angles.z = camera->ang.z;

    while (get_game_state() == 20) {
        RESOLVE_CAMERA_OBJ(camera);
        dx = target_position.x - camera->pos.x;
        dy = target_position.y - camera->pos.y;
        dz = target_position.z - camera->pos.z;
        distance = camera_sqrt(dx * dx + dy * dy + dz * dz);
        if (distance < 0.01f) {
            camera->pos.x = target_position.x;
            camera->pos.y = target_position.y;
            camera->pos.z = target_position.z;
        } else {
            dx *= 0.1f;
            dy *= 0.1f;
            dz *= 0.1f;
            camera->pos.x += dx;
            camera->pos.y += dy;
            camera->pos.z += dz;
        }

        RESOLVE_CAMERA_OBJ(camera);
        pitch_delta = target_angles.x - camera->ang.x;
        if (pitch_delta > kPi) {
            pitch_delta -= kTwoPi;
        } else if (pitch_delta < kNegPi) {
            pitch_delta += kTwoPi;
        }
        yaw_delta = target_angles.y - camera->ang.y;
        if (yaw_delta > kPi) {
            yaw_delta -= kTwoPi;
        } else if (yaw_delta < kNegPi) {
            yaw_delta += kTwoPi;
        }
        if (pitch_delta * pitch_delta + yaw_delta * yaw_delta <
            kAngEpsSq) {
            camera->ang.x = target_angles.x;
            camera->ang.y = target_angles.y;
            camera->ang.z = target_angles.z;
        } else {
            pitch_delta *= 0.1f;
            yaw_delta *= 0.1f;
            camera->ang.x += pitch_delta;
            camera->ang.y += yaw_delta;
            camera->ang.z = camera->ang.z + kZero * 0.1f;
        }

        RESOLVE_CAMERA_OBJ(camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        camera->pos.x += cam_pos_offset.x;
        camera->pos.y += cam_pos_offset.y;
        camera->pos.z += cam_pos_offset.z;
        camera->ang.x += cam_ang_offset.x;
        camera->ang.y += cam_ang_offset.y;
        camera->ang.z += cam_ang_offset.z;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
    camera_info.pdata->flags_bits.konquest_mode = 1;
    return kOne;
}

float p_konquest_interior_camera_proc(void) {
    if (camera_info.pdata == 0) {
        return kNegOne;
    }
    mkproc_jump_sleep(konquest_interior_camera_loop);
    return kZero;
}

float konquest_camera_loop(void) {
    Vec orbit_vector = {0.0f, 2.0f, -3.0f};
    Vec forward = {0.0f, 0.0f, 1.0f};
    Vec focus_position = {0.0f, 0.0f, 0.0f};
    Vec collision_start = {0.0f, 0.0f, 0.0f};
    Vec collision_point = {0.0f, 0.0f, 0.0f};
    MkObj* focus;
    CameraObj* camera;
    float speed;
    float dx;
    float dy;
    float dz;
    float distance;
    float pitch_delta;
    float yaw_delta;
    float roll_delta;
    int position_done;

    if (camera_info.pdata == 0) {
        return kNegOne;
    }
    focus = camera_info.pdata->movement_focus;
    if (focus == 0) {
        return kNegOne;
    }
    RESOLVE_CAMERA_OBJ(camera);
    if (camera == 0) {
        return kNegOne;
    }

    focus_position.x = focus->pos.value.x;
    focus_position.y = focus->pos.value.y;
    focus_position.z = focus->pos.value.z;
    camera_info.pdata->target_ang.z = kZero;
    rotate_xz(&camera_info.pdata->target_pos, &forward,
              camera_info.pdata->target_ang.y);
    camera_info.pdata->target_pos.x *= orbit_vector.z;
    camera_info.pdata->target_pos.y *= orbit_vector.z;
    camera_info.pdata->target_pos.z *= orbit_vector.z;
    camera_info.pdata->target_pos.x += focus_position.x;
    camera_info.pdata->target_pos.y += focus_position.y;
    camera_info.pdata->target_pos.z += focus_position.z;
    camera_info.pdata->target_pos.y = focus->ground_colls_y + 2.0f;

    if ((camera->pos.x - camera_info.pdata->target_pos.x) *
                camera->field_24->at.x +
            (camera->pos.z - camera_info.pdata->target_pos.z) *
                camera->field_24->at.z <
        kZero) {
        camera_info.pdata->flags_bits.konquest_mode = 1;
    }
    if (camera_info.pdata->flags_bits.konquest_mode) {
        RESOLVE_CAMERA_OBJ(camera);
        camera->pos.x = camera_info.pdata->target_pos.x;
        camera->pos.y = camera_info.pdata->target_pos.y;
        camera->pos.z = camera_info.pdata->target_pos.z;
        RESOLVE_CAMERA_OBJ(camera);
        camera->ang.x = camera_info.pdata->target_ang.x;
        camera->ang.y = camera_info.pdata->target_ang.y;
        camera->ang.z = camera_info.pdata->target_ang.z;
        update_mkobj(camera != 0 ? as_mkhdr(&camera->hdr) : 0);
        camera_info.pdata->flags_bits.konquest_mode = 0;
    }

    for (;;) {
        if (camera_info.pdata->flags_bits.konquest_mode) {
            return kOne;
        }
        focus = camera_info.pdata->movement_focus;
        if (focus == 0) {
            return kNegOne;
        }
        if (!focus->hide_flag_bits.still_move ||
            !camera_info.pdata->flags_bits.pos_done) {
            focus_position.x = focus->pos.value.x;
            focus_position.y = focus->pos.value.y;
            focus_position.z = focus->pos.value.z;
        }

        rotate_xz(&camera_info.pdata->target_pos, &forward,
                  camera_info.pdata->target_ang.y);
        camera_info.pdata->target_pos.x *= orbit_vector.z;
        camera_info.pdata->target_pos.y *= orbit_vector.z;
        camera_info.pdata->target_pos.z *= orbit_vector.z;
        camera_info.pdata->target_pos.x += focus_position.x;
        camera_info.pdata->target_pos.y += focus_position.y;
        camera_info.pdata->target_pos.z += focus_position.z;
        camera_info.pdata->target_pos.y = focus->ground_colls_y + 2.0f;

        collision_start.x = focus_position.x;
        collision_start.y = focus_position.y;
        collision_start.z = focus_position.z;
        collision_start.y = focus->ground_colls_y + 2.0f;
        if (get_konquest_game_mode() != 8) {
            if (collide_segment_against_global_collision_list(
                    &collision_start, &camera_info.pdata->target_pos,
                    &collision_point,
                    0x10002) != 0) {
                camera_info.pdata->target_pos.x = collision_point.x;
                camera_info.pdata->target_pos.y = collision_point.y;
                camera_info.pdata->target_pos.z = collision_point.z;
            }

            speed = kMoveScale * camera_info.pdata->speed;
            RESOLVE_CAMERA_OBJ(camera);
            dy = camera_info.pdata->target_pos.y - camera->pos.y;
            dx = camera_info.pdata->target_pos.x - camera->pos.x;
            dz = camera_info.pdata->target_pos.z - camera->pos.z;
            distance = camera_sqrt(dx * dx + dy * dy + dz * dz);
            if (distance < kSnapDist) {
                position_done = 1;
                camera->pos.x = camera_info.pdata->target_pos.x;
                camera->pos.y = camera_info.pdata->target_pos.y;
                camera->pos.z = camera_info.pdata->target_pos.z;
            } else {
                position_done = 0;
                dx *= speed;
                dy *= speed;
                dz *= speed;
                camera->pos.x += dx;
                camera->pos.y += dy;
                camera->pos.z += dz;
            }
            if (position_done != 0) {
                camera_info.pdata->flags_bits.pos_done = 1;
            } else {
                camera_info.pdata->flags_bits.pos_done = 0;
            }

            speed = kMoveScale * camera_info.pdata->speed;
            RESOLVE_CAMERA_OBJ(camera);
            pitch_delta = camera_info.pdata->target_ang.x - camera->ang.x;
            if (pitch_delta > kPi) {
                pitch_delta -= kTwoPi;
            } else if (pitch_delta < kNegPi) {
                pitch_delta += kTwoPi;
            }
            yaw_delta = camera_info.pdata->target_ang.y - camera->ang.y;
            if (yaw_delta > kPi) {
                yaw_delta -= kTwoPi;
            } else if (yaw_delta < kNegPi) {
                yaw_delta += kTwoPi;
            }
            if (!(pitch_delta * pitch_delta + yaw_delta * yaw_delta <
                  1.0000001e-6f)) {
                pitch_delta *= speed;
                yaw_delta *= speed;
                roll_delta = kZero * speed;
                camera->ang.x += pitch_delta;
                camera->ang.y += yaw_delta;
                camera->ang.z += roll_delta;
            }
        } else {
            RESOLVE_CAMERA_OBJ(camera);
            dy = camera_info.pdata->target_pos.y - camera->pos.y;
            dx = camera_info.pdata->target_pos.x - camera->pos.x;
            dz = camera_info.pdata->target_pos.z - camera->pos.z;
            distance = camera_sqrt(dx * dx + dy * dy + dz * dz);
            if (distance < kSnapDist) {
                camera->pos.x = camera_info.pdata->target_pos.x;
                camera->pos.y = camera_info.pdata->target_pos.y;
                camera->pos.z = camera_info.pdata->target_pos.z;
            } else {
                speed = kOne;
                dx *= speed;
                dy *= speed;
                dz *= speed;
                camera->pos.x += dx;
                camera->pos.y += dy;
                camera->pos.z += dz;
            }
            camera_info.pdata->flags_bits.pos_done = 0;

            RESOLVE_CAMERA_OBJ(camera);
            pitch_delta = camera_info.pdata->target_ang.x - camera->ang.x;
            if (pitch_delta > kPi) {
                pitch_delta -= kTwoPi;
            } else if (pitch_delta < kNegPi) {
                pitch_delta += kTwoPi;
            }
            yaw_delta = camera_info.pdata->target_ang.y - camera->ang.y;
            if (yaw_delta > kPi) {
                yaw_delta -= kTwoPi;
            } else if (yaw_delta < kNegPi) {
                yaw_delta += kTwoPi;
            }
            if (!(pitch_delta * pitch_delta + yaw_delta * yaw_delta <
                  1.0000001e-6f)) {
                speed = kOne;
                pitch_delta *= speed;
                yaw_delta *= speed;
                roll_delta = kZero * speed;
                camera->ang.x += pitch_delta;
                camera->ang.y += yaw_delta;
                camera->ang.z += roll_delta;
            }
        }

        RESOLVE_CAMERA_OBJ(camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        camera->pos.x += cam_pos_offset.x;
        camera->pos.y += cam_pos_offset.y;
        camera->pos.z += cam_pos_offset.z;
        camera->ang.x += cam_ang_offset.x;
        camera->ang.y += cam_ang_offset.y;
        camera->ang.z += cam_ang_offset.z;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
}

float p_konquest_camera_proc(void) {
    if (camera_info.pdata == 0) {
        return kNegOne;
    }
    camera_info.pdata->target_ang.x = 0.17f;
    camera_info.pdata->speed = kOne;
    camera_info.pdata->flags_bits.konquest_mode = 1;
    mkproc_jump_sleep(konquest_camera_loop);
    return kZero;
}

void run_interaction_camera_script(void* owner, void* script) {
    MkProc* process;
    MkHdr* process_data_header;

    if (g_active_npc != 0 &&
        ((g_active_npc->field_1D >> 6) & 1) != 0) {
        return;
    }

    memset(&g_ic_data, 0, sizeof(g_ic_data));
    process = find_mkproc_pid(0x9006);
    if (process != 0) {
        g_ic_data.created_process = 0;
        process_data_header = pdata_of_proc(process);
        ((InteractionCameraProcData*)process_data_header)->script = script;
        xfer_proc(process, p_run_interaction_camera);
        return;
    }

    g_ic_data.created_process = 1;
    process = _create_mkproc_generic_bigstack(
        0x9006, 0x1F, p_run_interaction_camera,
        sizeof(InteractionCameraProcData), &process_data_header);
    if (process != 0) {
        set_process_as_scriptable(process);
        ((InteractionCameraProcData*)process_data_header)->owner = owner;
        ((InteractionCameraProcData*)process_data_header)->script = script;
    }
}

/* Soft ceiling: validated-latch branch peephole and register coloring only. */
static float p_run_interaction_camera(void) {
    InteractionCameraProcData* data;

    data = (InteractionCameraProcData*)pdata_of_proc(aproc);
    cmdscript_setup_execution((int)data->owner, (int)data->script);
    cmdscript_execute((int)data->owner);
    while (get_konquest_movement_npc() != 0) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
    return kNegOne;
}

/* Soft ceiling: two validated-latch branch peepholes and register coloring. */
void interaction_cam_set_target_info(int duration, float angle_a,
                                     float field_14, float field_18,
                                     float angle_b, float field_20,
                                     float field_24) {
    MkObj* hero;
    InteractionNpc* movement_npc;
    MkObj* target;
    float normalized_angle_a;
    float normalized_angle_b;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    movement_npc = konquest_pdata->movement_npc;
    if (movement_npc != 0) {
        if (movement_npc->hdr.instance !=
            konquest_pdata->movement_npc_instance) {
            movement_npc = 0;
        }
    } else {
        movement_npc = 0;
    }

    while (g_ic_data.ticks != 0) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
    g_ic_data.camera_radius = field_14;
    g_ic_data.camera_height = field_18;
    g_ic_data.look_radius = field_20;
    g_ic_data.look_height = field_24;
    normalized_angle_a = 0.000005992112f *
                         (float)((int)(166886.1f * angle_a) & 0xFFFFF);
    normalized_angle_b = 0.000005992112f *
                         (float)((int)(166886.1f * angle_b) & 0xFFFFF);
    g_ic_data.camera_yaw_offset = normalized_angle_a;
    g_ic_data.look_yaw_offset = normalized_angle_b;
    g_ic_data.ticks = (int)((float)duration / get_game_speed());
    if (g_ic_data.ticks <= 1) {
        g_ic_data.ticks = 1;
        g_ic_data.glitched = 1;
    }
    target = movement_npc->target_info->object;
    g_ic_data.target = target;
    g_ic_data.target_instance = target->hdr.instance;
    g_ic_data.hero = hero;
    g_ic_data.hero_instance = hero->hdr.instance;
    if (camera_info.proc != 0) {
        xfer_proc(camera_info.proc, p_interaction_cam);
    }
}

/* Soft ceiling: inline orbit stack placement, latch peepholes, and FPR coloring. */
static float p_interaction_cam(void) {
    CameraObj* entry_camera;
    CameraObj* active_camera;
    MkObj* hero;
    MkObj* target;
    Vec desired_camera_position;
    Vec look_target;
    Vec hit_point;
    float entry_dx;
    float entry_dz;
    float entry_radius;
    float entry_height;
    float entry_world_yaw;
    float entry_relative_yaw;
    float ease_phase;
    float ease_step;
    float blend;
    float yaw_delta;
    float camera_yaw;
    float camera_radius;
    float camera_height;
    float half_dx;
    float half_dz;
    int track_targets;
    int snap_angles;

    track_targets = g_ic_data.created_process;
    check_reverse_interaction_cam_targets();
    entry_camera = camera_item.node;
    g_ic_data.created_process = 0;
    snap_angles = g_ic_data.ticks == 1;
    if (entry_camera != 0) {
        if (entry_camera->hdr.instance != camera_item.instance) {
            entry_camera = 0;
        }
    } else {
        entry_camera = 0;
    }

    if (entry_camera == 0) {
        return kNegOne;
    }
    if (g_ic_data.blocked != 0) {
        while (1) {
            _mkproc_sleep_ticks = 60.0f;
            mkproc_sleep();
        }
    }

    entry_dx = entry_camera->pos.x - conversation_midpoint.x;
    entry_dz = entry_camera->pos.z - conversation_midpoint.z;
    entry_height = entry_camera->pos.y - conversation_midpoint.y;
    ease_phase = kZero;
    ease_step = kPi * (kOne / (float)g_ic_data.ticks);
    entry_world_yaw = gxMathArcTanYX(entry_dx, entry_dz);
    entry_radius = camera_sqrt(entry_dx * entry_dx + entry_dz * entry_dz);
    entry_relative_yaw = normalize_interaction_angle(
        entry_world_yaw - conversation_interaction_angle);

    make_interaction_orbit_offset(
        &look_target,
        conversation_interaction_angle + g_ic_data.look_yaw_offset,
        g_ic_data.look_radius, g_ic_data.look_height);
    look_target.x += conversation_midpoint.x;
    look_target.y += conversation_midpoint.y;
    look_target.z += conversation_midpoint.z;
    desired_camera_position.x = entry_camera->pos.x;
    desired_camera_position.y = entry_camera->pos.y;
    desired_camera_position.z = entry_camera->pos.z;

    while (1) {
        if (g_ic_data.ticks != 0 && g_ic_data.blocked == 0) {
            ease_phase += ease_step;
            blend = kHalf * (kOne - gxMathCos(ease_phase));

            if (track_targets != 0) {
                hero = g_ic_data.hero;
                if (hero != 0) {
                    if (hero->hdr.instance != g_ic_data.hero_instance) {
                        hero = 0;
                    }
                } else {
                    hero = 0;
                }
                if (hero != 0) {
                    target = g_ic_data.target;
                    if (target != 0) {
                        if (target->hdr.instance !=
                            g_ic_data.target_instance) {
                            target = 0;
                        }
                    } else {
                        target = 0;
                    }
                    if (target != 0) {
                        conversation_midpoint.x =
                            hero->pos.value.x - target->pos.value.x;
                        conversation_midpoint.x *= kHalf;
                        conversation_midpoint.x += target->pos.value.x;
                        conversation_midpoint.z =
                            hero->pos.value.z - target->pos.value.z;
                        conversation_midpoint.z *= kHalf;
                        conversation_midpoint.z += target->pos.value.z;
                        conversation_midpoint.y =
                            kHalf * (hero->ground_colls_y +
                                     target->ground_colls_y);
                    }
                }

                hero = g_ic_data.hero;
                if (hero != 0) {
                    if (hero->hdr.instance != g_ic_data.hero_instance) {
                        hero = 0;
                    }
                } else {
                    hero = 0;
                }
                if (hero == 0) {
                    conversation_interaction_angle = kZero;
                } else {
                    target = g_ic_data.target;
                    if (target != 0) {
                        if (target->hdr.instance !=
                            g_ic_data.target_instance) {
                            target = 0;
                        }
                    } else {
                        target = 0;
                    }
                    if (target == 0) {
                        conversation_interaction_angle = kZero;
                    } else if (hero == target) {
                        conversation_interaction_angle = hero->ang.y;
                    } else {
                        half_dx = kHalf * (hero->pos.value.x - target->pos.value.x);
                        half_dz = kHalf * (hero->pos.value.z - target->pos.value.z);
                        conversation_interaction_angle =
                            (float)atan2(half_dx, half_dz);
                    }
                }
            }

            yaw_delta = g_ic_data.camera_yaw_offset - entry_relative_yaw;
            if (yaw_delta > kPi) {
                yaw_delta -= kTwoPi;
            } else if (yaw_delta < kNegPi) {
                yaw_delta += kTwoPi;
            }
            camera_yaw = conversation_interaction_angle +
                         (yaw_delta * blend + entry_relative_yaw);
            camera_yaw = normalize_interaction_angle(camera_yaw);
            camera_height =
                blend * (g_ic_data.camera_height - entry_height) +
                entry_height;
            camera_radius =
                blend * (g_ic_data.camera_radius - entry_radius) +
                entry_radius;

            make_interaction_orbit_offset(&desired_camera_position,
                                          camera_yaw, camera_radius,
                                          camera_height);
            desired_camera_position.x += conversation_midpoint.x;
            desired_camera_position.y += conversation_midpoint.y;
            desired_camera_position.z += conversation_midpoint.z;
            make_interaction_orbit_offset(
                &look_target,
                conversation_interaction_angle + g_ic_data.look_yaw_offset,
                g_ic_data.look_radius, g_ic_data.look_height);
            look_target.x += conversation_midpoint.x;
            look_target.y += conversation_midpoint.y;
            look_target.z += conversation_midpoint.z;

            if (collide_segment_against_global_collision_list(
                    &entry_camera->pos, &desired_camera_position,
                    &hit_point, 0x10002) != 0) {
                desired_camera_position.x = hit_point.x;
                desired_camera_position.y = hit_point.y;
                desired_camera_position.z = hit_point.z;
                g_ic_data.blocked = 1;
            }
            g_ic_data.ticks--;
        }

        RESOLVE_CAMERA_OBJ(active_camera);
        active_camera->pos.x = desired_camera_position.x;
        active_camera->pos.y = desired_camera_position.y;
        active_camera->pos.z = desired_camera_position.z;
        look_at_interaction_target(&look_target, snap_angles);
        RESOLVE_CAMERA_OBJ(active_camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        active_camera->pos.x += cam_pos_offset.x;
        active_camera->pos.y += cam_pos_offset.y;
        active_camera->pos.z += cam_pos_offset.z;
        active_camera->ang.x += cam_ang_offset.x;
        active_camera->ang.y += cam_ang_offset.y;
        active_camera->ang.z += cam_ang_offset.z;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
}

/* Soft ceiling: camera-latch lowering and inline inverse-sqrt FPU scheduling. */
static void look_at_interaction_target(const Vec* target, int snap_angles) {
    CameraObj* camera;
    Vec direction;
    Vec target_angles;
    float inverse_length;
    float delta_x;
    float delta_y;
    float delta_z;

    RESOLVE_CAMERA_OBJ(camera);
    direction.x = target->x - camera->pos.x;
    direction.y = target->y - camera->pos.y;
    direction.z = target->z - camera->pos.z;
    inverse_length = camera_inv_sqrt(direction.x * direction.x +
                                     direction.y * direction.y +
                                     direction.z * direction.z);
    direction.x *= inverse_length;
    direction.y *= inverse_length;
    direction.z *= inverse_length;
    v3_to_xy_ang_high_freq(&target_angles, &direction);
    norm_angles_v3(&target_angles);

    if (snap_angles != 0) {
        camera->ang.x = target_angles.x;
        camera->ang.y = target_angles.y;
        camera->ang.z = target_angles.z;
    } else {
        delta_x = target_angles.x - camera->ang.x;
        if (delta_x > kPi) {
            delta_x -= kTwoPi;
        } else if (delta_x < kNegPi) {
            delta_x += kTwoPi;
        }
        delta_y = target_angles.y - camera->ang.y;
        if (delta_y > kPi) {
            delta_y -= kTwoPi;
        } else if (delta_y < kNegPi) {
            delta_y += kTwoPi;
        }
        delta_z = target_angles.z - camera->ang.z;
        if (delta_z > kPi) {
            delta_z -= kTwoPi;
        } else if (delta_z < kNegPi) {
            delta_z += kTwoPi;
        }
        camera->ang.x = kMoveScale * delta_x + camera->ang.x;
        camera->ang.y = kMoveScale * delta_y + camera->ang.y;
        camera->ang.z = kMoveScale * delta_z + camera->ang.z;
    }
}

/* Soft ceiling: six inline invsqrt stack/FPR layouts and latch peepholes. */
static void check_reverse_interaction_cam_targets(void) {
    MkObj* hero;
    MkObj* target;
    MkObj* hero_item;
    MkObj* target_item;
    CameraObj* camera;
    Vec candidate_1;
    Vec candidate_2;
    Vec candidate_3;
    Vec candidate_0;
    Vec hit_point;
    Vec camera_delta;
    float half_dx;
    float half_dz;
    float best_distance;
    float distance;
    int selection;

    hero_item = g_ic_data.hero;
    target_item = g_ic_data.target;
    hero = hero_item;
    if (hero != 0) {
        if (hero->hdr.instance != g_ic_data.hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    target = target_item;
    if (target != 0) {
        if (target->hdr.instance != g_ic_data.target_instance) {
            target = 0;
        }
    } else {
        target = 0;
    }
    RESOLVE_CAMERA_OBJ(camera);
    selection = 0;
    if (hero == 0 || target == 0 || camera == 0) {
        return;
    }

    if (g_ic_data.ticks == 1 || g_ic_data.created_process != 0) {
        hero = hero_item;
        if (hero != 0) {
            if (hero->hdr.instance != g_ic_data.hero_instance) {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero != 0) {
            target = target_item;
            if (target != 0) {
                if (target->hdr.instance != g_ic_data.target_instance) {
                    target = 0;
                }
            } else {
                target = 0;
            }
            if (target != 0) {
                conversation_midpoint.x = hero->pos.value.x - target->pos.value.x;
                conversation_midpoint.x *= kHalf;
                conversation_midpoint.x += target->pos.value.x;
                conversation_midpoint.z = hero->pos.value.z - target->pos.value.z;
                conversation_midpoint.z *= kHalf;
                conversation_midpoint.z += target->pos.value.z;
                conversation_midpoint.y =
                    (hero->ground_colls_y + target->ground_colls_y) * kHalf;
            }
        }

        hero = hero_item;
        if (hero != 0) {
            if (hero->hdr.instance != g_ic_data.hero_instance) {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero == 0) {
            conversation_interaction_angle = kZero;
        } else {
            target = target_item;
            if (target != 0) {
                if (target->hdr.instance != g_ic_data.target_instance) {
                    target = 0;
                }
            } else {
                target = 0;
            }
            if (target == 0) {
                conversation_interaction_angle = kZero;
            } else if (hero == target) {
                conversation_interaction_angle = hero->ang.y;
            } else {
                half_dx = kHalf * (hero->pos.value.x - target->pos.value.x);
                half_dz = kHalf * (hero->pos.value.z - target->pos.value.z);
                conversation_interaction_angle =
                    (float)atan2(half_dx, half_dz);
            }
        }
    }

    if (konquest_pdata->conversation_mode_b == 2) {
        g_ic_data.camera_yaw_offset += kPi;
        g_ic_data.look_yaw_offset += kPi;
    }
    if (g_ic_data.reversed != 0) {
        g_ic_data.camera_yaw_offset =
            kTwoPi - g_ic_data.camera_yaw_offset;
        g_ic_data.look_yaw_offset = kTwoPi - g_ic_data.look_yaw_offset;
    }
    g_ic_data.camera_yaw_offset =
        normalize_interaction_angle(g_ic_data.camera_yaw_offset);
    g_ic_data.look_yaw_offset =
        normalize_interaction_angle(g_ic_data.look_yaw_offset);

    if (g_ic_data.created_process != 0) {
        make_interaction_orbit_offset(
            &candidate_0,
            conversation_interaction_angle + g_ic_data.camera_yaw_offset,
            g_ic_data.camera_radius, g_ic_data.camera_height);
        make_interaction_orbit_offset(
            &candidate_1,
            conversation_interaction_angle +
                (kTwoPi - g_ic_data.camera_yaw_offset),
            g_ic_data.camera_radius, g_ic_data.camera_height);
        make_interaction_orbit_offset(
            &candidate_2,
            conversation_interaction_angle +
                (kPi - g_ic_data.camera_yaw_offset),
            g_ic_data.camera_radius, g_ic_data.camera_height);
        make_interaction_orbit_offset(
            &candidate_3,
            conversation_interaction_angle +
                (kPi + g_ic_data.camera_yaw_offset),
            g_ic_data.camera_radius, g_ic_data.camera_height);

        candidate_0.x += conversation_midpoint.x;
        candidate_0.z += conversation_midpoint.z;
        candidate_1.x += conversation_midpoint.x;
        candidate_1.z += conversation_midpoint.z;
        candidate_2.x += conversation_midpoint.x;
        candidate_2.z += conversation_midpoint.z;
        candidate_3.x += conversation_midpoint.x;
        candidate_3.z += conversation_midpoint.z;

        camera_delta.x = candidate_0.x - camera->pos.x;
        camera_delta.z = candidate_0.z - camera->pos.z;
        best_distance = camera_delta.x * camera_delta.x +
                        camera_delta.z * camera_delta.z;

        camera_delta.x = candidate_2.x - camera->pos.x;
        camera_delta.z = candidate_2.z - camera->pos.z;
        distance = camera_delta.x * camera_delta.x +
                   camera_delta.z * camera_delta.z;
        if (distance < best_distance) {
            best_distance = distance;
            selection = 2;
        }
        camera_delta.x = candidate_3.x - camera->pos.x;
        camera_delta.z = candidate_3.z - camera->pos.z;
        distance = camera_delta.x * camera_delta.x +
                   camera_delta.z * camera_delta.z;
        if (distance < best_distance) {
            best_distance = distance;
            selection = 3;
        }
        camera_delta.x = candidate_1.x - camera->pos.x;
        camera_delta.z = candidate_1.z - camera->pos.z;
        distance = camera_delta.x * camera_delta.x +
                   camera_delta.z * camera_delta.z;
        if (distance < best_distance) {
            selection = 1;
            g_ic_data.camera_yaw_offset =
                kTwoPi - g_ic_data.camera_yaw_offset;
            g_ic_data.look_yaw_offset =
                kTwoPi - g_ic_data.look_yaw_offset;
            g_ic_data.reversed = 1;
        }
        g_ic_data.camera_yaw_offset =
            normalize_interaction_angle(g_ic_data.camera_yaw_offset);
        g_ic_data.look_yaw_offset =
            normalize_interaction_angle(g_ic_data.look_yaw_offset);
        if ((selection & 2) != 0) {
            g_ic_data.ticks = 1;
            g_ic_data.glitched = 1;
        }
    }

    make_interaction_orbit_offset(
        &candidate_0,
        conversation_interaction_angle + g_ic_data.camera_yaw_offset,
        g_ic_data.camera_radius, g_ic_data.camera_height);
    candidate_0.x += conversation_midpoint.x;
    candidate_0.y += conversation_midpoint.y;
    candidate_0.z += conversation_midpoint.z;
    if (g_ic_data.glitched != 0) {
        candidate_1.x = target->pos.value.x;
        candidate_1.y = target->pos.value.y;
        candidate_1.z = target->pos.value.z;
    } else {
        candidate_1.x = camera->pos.x;
        candidate_1.y = camera->pos.y;
        candidate_1.z = camera->pos.z;
    }
    if (collide_segment_against_global_collision_list(
            &candidate_1, &candidate_0, &hit_point, 0x10002) == 0) {
        return;
    }

    g_ic_data.camera_yaw_offset = kTwoPi - g_ic_data.camera_yaw_offset;
    g_ic_data.look_yaw_offset = kTwoPi - g_ic_data.look_yaw_offset;
    make_interaction_orbit_offset(
        &candidate_0,
        conversation_interaction_angle + g_ic_data.camera_yaw_offset,
        g_ic_data.camera_radius, g_ic_data.camera_height);
    candidate_0.x += conversation_midpoint.x;
    candidate_0.y += conversation_midpoint.y;
    candidate_0.z += conversation_midpoint.z;
    if (g_ic_data.glitched != 0) {
        candidate_1.x = target->pos.value.x;
        candidate_1.y = target->pos.value.y;
        candidate_1.z = target->pos.value.z;
    } else {
        candidate_1.x = camera->pos.x;
        candidate_1.y = camera->pos.y;
        candidate_1.z = camera->pos.z;
    }
    if (collide_segment_against_global_collision_list(
            &candidate_1, &candidate_0, &hit_point, 0x10002) != 0) {
        g_ic_data.blocked = 1;
        g_ic_data.glitched = 0;
    } else {
        g_ic_data.reversed = !g_ic_data.reversed;
    }
}

int interaction_cam_glitched(void) {
    return g_ic_data.glitched;
}

void reset_camera_paths(void) {
    float initial_speed = kZero;
    float final_speed = kZero;
    Vec endpoint = {0.0f, 0.0f, 0.0f};

    move_to_end_point(&endpoint, &initial_speed, &final_speed, 1, kZero);
    orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1, 1,
                                kZero);
}

void special_move_cam_end(void) {
    xfer_camera_impl(p_camera_proc, 1);
}

/* Soft ceiling: folded GameInfo base load, register timing, constant labels. */
void special_move_cam_setup2(int ease_ticks, int total_ticks, int unused,
                             MkObj* target, MkObj* reference_object,
                             float orbit_yaw_offset, float orbit_radius,
                             float camera_height, float look_yaw_offset,
                             float look_pitch) {
    if (!g_game_info.flag_bits.lens_flare_enabled) {
        if (am_i_on_the_left2(target, reference_object) != 0) {
            orbit_yaw_offset = -orbit_yaw_offset;
            look_yaw_offset = -look_yaw_offset;
        }
        smc_data.orbit_yaw_offset = orbit_yaw_offset;
        smc_data.look_pitch = look_pitch;
        smc_data.look_yaw_offset = look_yaw_offset;
        smc_data.orbit_radius = orbit_radius;
        smc_data.camera_height = camera_height + g_game_info.field_34;
        smc_data.ease_ticks = ease_ticks;
        smc_data.total_ticks = total_ticks;
        smc_data.target = target;
        smc_data.target_instance = target->hdr.instance;
        xfer_camera_impl(p_special_move_cam, 1);
    }
}

/* Soft ceiling: folded GameInfo base load, register timing, constant labels. */
void special_move_cam_setup(int ease_ticks, int total_ticks, int unused,
                            float orbit_yaw_offset, float orbit_radius,
                            float camera_height, float look_yaw_offset,
                            float look_pitch) {
    if (!g_game_info.flag_bits.lens_flare_enabled) {
        if (am_i_on_the_left() != 0) {
            orbit_yaw_offset = -orbit_yaw_offset;
            look_yaw_offset = -look_yaw_offset;
        }
        smc_data.orbit_yaw_offset = orbit_yaw_offset;
        smc_data.look_pitch = look_pitch;
        smc_data.look_yaw_offset = look_yaw_offset;
        smc_data.orbit_radius = orbit_radius;
        smc_data.camera_height = camera_height + g_game_info.field_34;
        smc_data.ease_ticks = ease_ticks;
        smc_data.total_ticks = total_ticks;
        smc_data.target = plyr_obj;
        smc_data.target_instance = plyr_obj->hdr.instance;
        xfer_camera_impl(p_special_move_cam, 1);
    }
}

/* Soft ceiling: validated-latch branches, offset-base folding, FPR coloring. */
static float p_special_move_cam(void) {
    MkObj* target;
    CameraObj* camera;
    Vec target_angles;
    float start_pos_x;
    float start_pos_y;
    float start_pos_z;
    float start_ang_x;
    float start_ang_y;
    float start_ang_z;
    float delta_pos_x;
    float delta_pos_y;
    float delta_pos_z;
    float delta_ang_x;
    float delta_ang_y;
    float delta_ang_z;
    float current_pos_x;
    float current_pos_y;
    float current_pos_z;
    float current_ang_x;
    float current_ang_y;
    float current_ang_z;
    float desired_pos_x;
    float desired_pos_z;
    float orbit_angle;
    float orbit_sin;
    float orbit_cos;
    float phase;
    float phase_step;
    float blend;

    target = smc_data.target;
    if (target != 0) {
        if (target->hdr.instance != smc_data.target_instance) {
            target = 0;
        }
    } else {
        target = 0;
    }
    if (target == 0) {
        mkproc_jump_sleep(p_camera_proc);
        return kZero;
    }

    RESOLVE_CAMERA_OBJ(camera);
    if (camera == 0) {
        mkproc_jump_sleep(p_camera_proc);
        return kZero;
    }

    orbit_angle =
        0.000005992112f *
        (float)((int)(166886.1f *
                      (target->ang.y + smc_data.orbit_yaw_offset)) &
                0xFFFFF);
    orbit_sin = gxMathSin(orbit_angle);
    orbit_cos = gxMathCos(orbit_angle);
    desired_pos_x = orbit_sin * smc_data.orbit_radius + target->pos.value.x;
    desired_pos_z = orbit_cos * smc_data.orbit_radius + target->pos.value.z;

    target_angles.x = smc_data.look_pitch;
    target_angles.y = target->ang.y + smc_data.look_yaw_offset;
    target_angles.z = kZero;
    norm_angles_v3(&target_angles);

    start_pos_x = camera->pos.x;
    start_pos_y = camera->pos.y;
    start_pos_z = camera->pos.z;
    start_ang_x = camera->ang.x;
    start_ang_y = camera->ang.y;
    start_ang_z = camera->ang.z;
    delta_pos_x = desired_pos_x - start_pos_x;
    delta_pos_y = smc_data.camera_height - start_pos_y;
    delta_pos_z = desired_pos_z - start_pos_z;

    delta_ang_x = target_angles.x - start_ang_x;
    if (delta_ang_x > kPi) {
        delta_ang_x -= kTwoPi;
    } else if (delta_ang_x < kNegPi) {
        delta_ang_x += kTwoPi;
    }
    delta_ang_y = target_angles.y - start_ang_y;
    if (delta_ang_y > kPi) {
        delta_ang_y -= kTwoPi;
    } else if (delta_ang_y < kNegPi) {
        delta_ang_y += kTwoPi;
    }
    delta_ang_z = target_angles.z - start_ang_z;
    if (delta_ang_z > kPi) {
        delta_ang_z -= kTwoPi;
    } else if (delta_ang_z < kNegPi) {
        delta_ang_z += kTwoPi;
    }

    phase = kZero;
    phase_step = kPi * (kOne / (float)smc_data.ease_ticks);
    do {
        if (smc_data.ease_ticks != 0) {
            phase += phase_step;
            blend = kHalf * (kOne - gxMathCos(phase));
            smc_data.ease_ticks--;
            current_pos_x = delta_pos_x * blend + start_pos_x;
            current_pos_y = delta_pos_y * blend + start_pos_y;
            current_pos_z = delta_pos_z * blend + start_pos_z;
            current_ang_x = delta_ang_x * blend + start_ang_x;
            current_ang_y = delta_ang_y * blend + start_ang_y;
            current_ang_z = delta_ang_z * blend + start_ang_z;
        }

        RESOLVE_CAMERA_OBJ(camera);
        camera->pos.x = current_pos_x;
        camera->pos.y = current_pos_y;
        camera->pos.z = current_pos_z;

        RESOLVE_CAMERA_OBJ(camera);
        camera->ang.x = current_ang_x;
        camera->ang.y = current_ang_y;
        camera->ang.z = current_ang_z;

        RESOLVE_CAMERA_OBJ(camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        camera->pos.x += cam_pos_offset.x;
        camera->pos.y += cam_pos_offset.y;
        camera->pos.z += cam_pos_offset.z;
        camera->ang.x += cam_ang_offset.x;
        camera->ang.y += cam_ang_offset.y;
        camera->ang.z += cam_ang_offset.z;

        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    } while (--smc_data.total_ticks >= 0);

    force_midpoint_calculation_update = 1;
    mkproc_jump_sleep(p_camera_proc);
    return kZero;
}

void cam_calc_right_at_up_offsets(const Vec* position, float* forward_offset,
                                  float* right_offset, float* up_offset) {
    CameraObj* camera;
    float dx;
    float dy;
    float dz;

    RESOLVE_CAMERA_OBJ(camera);
    dy = position->y - camera->pos.y;
    dx = position->x - camera->pos.x;
    dz = position->z - camera->pos.z;

    *right_offset =
        cam_right_uv.x * dx + cam_right_uv.y * dy + cam_right_uv.z * dz;
    *forward_offset = cam_forward_uv.x * dx + cam_forward_uv.y * dy +
                      cam_forward_uv.z * dz;
    *up_offset = cam_up_uv.x * dx + cam_up_uv.y * dy + cam_up_uv.z * dz;
}

float get_volume_from_distance(const Vec* position, float far_distance,
                               float near_distance) {
    CameraObj* camera;
    float volume = kZero;
    float distance;

    RESOLVE_CAMERA_OBJ(camera);
    if (camera == 0) {
        return kNegOne;
    }
    distance = dist_v3_to_v3(position, &camera->frame->modelling.pos_vec);
    if (distance < far_distance) {
        volume = kOne -
                 ((distance - near_distance) / (far_distance - near_distance));
    }
    return volume;
}

float get_pan_value(const Vec* position) {
    CameraObj* camera;
    Vec direction;
    Vec forward = {0.0f, 0.0f, 0.0f};
    Vec right = {0.0f, 0.0f, 0.0f};
    float inverse_length;
    float forward_dot;
    float right_dot;

    RESOLVE_CAMERA_OBJ(camera);
    if (camera == 0) {
        return 0.0f;
    }

    xz_unit_vector(&direction, &camera->field_24->pos_vec, position);

    forward.x = cam_forward_uv.x;
    forward.z = cam_forward_uv.z;
    inverse_length = camera_inv_sqrt(
        forward.x * forward.x + forward.y * forward.y +
        forward.z * forward.z);
    forward.x *= inverse_length;
    forward.y *= inverse_length;
    forward.z *= inverse_length;

    right.x = cam_right_uv.x;
    right.z = cam_right_uv.z;
    inverse_length = camera_inv_sqrt(
        right.x * right.x + right.y * right.y + right.z * right.z);
    right.x *= inverse_length;
    right.y *= inverse_length;
    right.z *= inverse_length;

    forward_dot =
        forward.x * direction.x + forward.z * direction.z;
    right_dot = right.x * direction.x + right.z * direction.z;

    if (right_dot > kZero) {
        return -gxMathArcCos(forward_dot) / 1.5707964f;
    }
    return gxMathArcCos(forward_dot) / 1.5707964f;
}

void camera_exit_script(void) {
    memset(&scripted_camera_data, 0, sizeof(scripted_camera_data));
    {
        float initial_speed = kZero;
        float final_speed = kZero;
        Vec endpoint = {0.0f, 0.0f, 0.0f};

        move_to_end_point(&endpoint, &initial_speed, &final_speed, 1, kZero);
        orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1, 1,
                                    kZero);
    }
    if (camera_info.proc != 0) {
        xfer_proc(camera_info.proc, old_camera_function);
        if (Camera != 0) {
            CameraSize(Camera, 0,
                       gxMathTan((kPi * DEFAULT_FIELD_OF_VIEW) / kDegPerTurn),
                       DEFAULT_ASPECTRATIO);
            RwCameraSetProjection(Camera, 1);
        }
        if (camera_obj != 0) {
            camera_obj->flags.bit04 = 1;
            camera_obj->flags.bit20 = 1;
        }
    }
    destroy_mkprocs_pid(0x9006);
}

void run_camera_script(int script, int argument, int flags) {
    MkHdr* pdata_hdr;
    MkProc* monitor = camera_script_monitor_item.node;
    MkProc* process;

    if (monitor != 0) {
        if (monitor->instance != camera_script_monitor_item.instance) {
            monitor = 0;
        } else {
            /* keep the validated monitor */
        }
    } else {
        monitor = 0;
    }
    if (monitor != 0 && monitor->instance != 0) {
        monitor->vtbl->destroy(monitor);
    }
    process = _create_mkproc_generic_bigstack(
        0x9006, 0x1D, p_run_camera_script, sizeof(CameraScriptPdata),
        &pdata_hdr);
    if (process != 0) {
        camera_script_monitor_item.node = process;
        camera_script_monitor_item.instance = process->instance;
        ((CameraScriptPdata*)pdata_hdr)->script = script;
        ((CameraScriptPdata*)pdata_hdr)->argument = argument;
        ((CameraScriptPdata*)pdata_hdr)->flags = flags;
        old_camera_function = camera_info.proc->entry;
        set_process_as_scriptable(process);
        memset(&scripted_camera_data, 0, sizeof(scripted_camera_data));
        {
            float initial_speed = kZero;
            float final_speed = kZero;
            Vec endpoint = {0.0f, 0.0f, 0.0f};

            move_to_end_point(&endpoint, &initial_speed, &final_speed, 1,
                              kZero);
            orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1,
                                        1, kZero);
        }
        if (camera_info.proc != 0) {
            xfer_proc(camera_info.proc, p_scripted_camera);
            if (Camera != 0) {
                CameraSize(
                    Camera, 0,
                    gxMathTan((kPi * DEFAULT_FIELD_OF_VIEW) / kDegPerTurn),
                    DEFAULT_ASPECTRATIO);
                RwCameraSetProjection(Camera, 1);
            }
            if (camera_obj != 0) {
                camera_obj->flags.bit04 = 1;
                camera_obj->flags.bit20 = 1;
            }
        }
    }
}

static float p_run_camera_script(void) {
    cmdscript_setup_execution(((CameraScriptPdata*)apdata)->script,
                              ((CameraScriptPdata*)apdata)->argument);
    cmdscript_execute(((CameraScriptPdata*)apdata)->script);
    if (((CameraScriptPdata*)apdata)->flags == 0) {
        memset(&scripted_camera_data, 0, sizeof(scripted_camera_data));
        {
            float initial_speed = kZero;
            float final_speed = kZero;
            Vec endpoint = {0.0f, 0.0f, 0.0f};

            move_to_end_point(&endpoint, &initial_speed, &final_speed, 1,
                              kZero);
            orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1,
                                        1, kZero);
        }
        while (get_cmdscript_for_proc(aproc)->active != 0) {
            _mkproc_sleep_ticks = kOne;
            mkproc_sleep();
        }
        if (camera_info.proc != 0) {
            xfer_proc(camera_info.proc, old_camera_function);
            if (Camera != 0) {
                CameraSize(
                    Camera, 0,
                    gxMathTan((kPi * DEFAULT_FIELD_OF_VIEW) / kDegPerTurn),
                    DEFAULT_ASPECTRATIO);
                RwCameraSetProjection(Camera, 1);
            }
            if (camera_obj != 0) {
                camera_obj->flags.bit04 = 1;
                camera_obj->flags.bit20 = 1;
            }
        }
        camera_script_monitor_item.node = 0;
        camera_script_monitor_item.instance = 0;
        return kNegOne;
    }
    while (1) {
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
    }
}

float p_scripted_camera(void) {
    Vec target_position = {0.0f, 0.0f, 0.0f};
    Vec look_target = {0.0f, 0.0f, 0.0f};
    Vec target_angles = {0.0f, 0.0f, 0.0f};
    Vec collision_point = {0.0f, 0.0f, 0.0f};
    Vec desired_angles;
    Vec direct_angles;
    Vec direction;
    CameraObj* camera;
    MkObj* focus;
    float distance;
    float inverse_length;
    float pitch_delta;
    float yaw_delta;
    float rate;
    float roll_delta;
    int done;

    cam_ang_offset.x = kZero;
    cam_ang_offset.y = kZero;
    cam_ang_offset.z = kZero;
    cam_pos_offset.x = kZero;
    cam_pos_offset.y = kZero;
    cam_pos_offset.z = kZero;
    _mkproc_sleep_ticks = kOne;
    mkproc_sleep();

    for (;;) {
        remove_camera_offsets_impl();

        focus = scripted_camera_data.movement_focus;
        switch (scripted_camera_data.movement_mode) {
        case 0:
            break;
        case 1:
            target_position.x =
                focus->pos.value.x + scripted_camera_data.movement_offset.x;
            target_position.y =
                focus->pos.value.y + scripted_camera_data.movement_offset.y;
            target_position.z =
                focus->pos.value.z + scripted_camera_data.movement_offset.z;
            target_position.y = scripted_camera_data.movement_offset.y;
            break;
        case 2:
            target_position.x =
                focus->pos.value.x + scripted_camera_data.movement_offset.x;
            target_position.y =
                focus->pos.value.y + scripted_camera_data.movement_offset.y;
            target_position.z =
                focus->pos.value.z + scripted_camera_data.movement_offset.z;
            break;
        case 3:
            rotate_xz(&direction, &scripted_camera_data.focus_direction,
                      scripted_camera_data.center_of_rotation.z);
            direction.x *= scripted_camera_data.center_of_rotation.y;
            direction.y *= scripted_camera_data.center_of_rotation.y;
            direction.z *= scripted_camera_data.center_of_rotation.y;
            target_position.x = focus->pos.value.x + direction.x;
            target_position.y = focus->pos.value.y + direction.y;
            target_position.z = focus->pos.value.z + direction.z;
            target_position.y = scripted_camera_data.movement_offset.y;
            break;
        case 4:
            target_position.x = scripted_camera_data.movement_offset.x;
            target_position.y = scripted_camera_data.movement_offset.y;
            target_position.z = scripted_camera_data.movement_offset.z;
            break;
        case 5:
            v3_x_mat_add_v3(&target_position,
                            &scripted_camera_data.movement_offset,
                            focus->field_24, &focus->pos.value);
            break;
        case 6:
            v3_x_mat_add_v3(&target_position,
                            &scripted_camera_data.movement_offset,
                            focus->field_24, &focus->pos.value);
            target_position.y = scripted_camera_data.movement_offset.y;
            break;
        case 7:
            rotate_xz(&scripted_camera_data.radial_vector,
                      &scripted_camera_data.radial_vector,
                      scripted_camera_data.radial_step);
            target_position.x = scripted_camera_data.center_of_rotation.x +
                                scripted_camera_data.radial_vector.x;
            target_position.y = scripted_camera_data.center_of_rotation.y +
                                scripted_camera_data.radial_vector.y;
            target_position.z = scripted_camera_data.center_of_rotation.z +
                                scripted_camera_data.radial_vector.z;
            target_position.y = scripted_camera_data.movement_offset.y;
            break;
        }

        focus = scripted_camera_data.lookat_focus;
        switch (scripted_camera_data.look_mode) {
        case 0:
            break;
        case 1:
        case 2:
            target_angles.x = scripted_camera_data.lookat_offset.x;
            target_angles.y = scripted_camera_data.lookat_offset.y;
            target_angles.z = scripted_camera_data.lookat_offset.z;
            break;
        case 3:
            target_angles.x = scripted_camera_data.lookat_offset.x;
            target_angles.y = scripted_camera_data.focus_angle +
                              scripted_camera_data.lookat_offset.y;
            break;
        case 4:
            look_target.x = scripted_camera_data.lookat_offset.x;
            look_target.y = scripted_camera_data.lookat_offset.y;
            look_target.z = scripted_camera_data.lookat_offset.z;
            break;
        case 5:
        case 6:
            look_target.x = focus->pos.value.x;
            look_target.y = focus->pos.value.y;
            look_target.z = focus->pos.value.z;
            break;
        case 7:
        case 8:
            look_target.x = focus->pos.value.x;
            look_target.y = focus->pos.value.y;
            look_target.z = focus->pos.value.z;
            look_target.y = scripted_camera_data.lookat_offset.y;
            break;
        case 9:
            v3_x_mat_add_v3(&look_target,
                            &scripted_camera_data.lookat_offset,
                            focus->field_24, &focus->pos.value);
            break;
        }

        if (scripted_camera_data.movement_mode != 0) {
            if (scripted_camera_data.glitch != 0) {
                RESOLVE_CAMERA_OBJ(camera);
                camera->pos.x = target_position.x;
                camera->pos.y = target_position.y;
                camera->pos.z = target_position.z;
                scripted_camera_data.pos_move_done = 1;
            } else {
                if (scripted_camera_data.check_collisions != 0 &&
                    collide_segment_against_global_collision_list(
                        &look_target, &target_position, &collision_point,
                        0x10002) != 0) {
                    target_position.x = collision_point.x;
                    target_position.y = collision_point.y;
                    target_position.z = collision_point.z;
                    scripted_camera_data.movement_mode = 0;
                    scripted_camera_data.look_mode = 0;
                }
                if (scripted_camera_data.custom_movement != 0) {
                    if (scripted_camera_data.radial_movement != 0) {
                        scripted_camera_data.pos_move_done =
                            orbit_position_to_end_point(
                                &scripted_camera_data.center_of_rotation,
                                &target_position,
                                &scripted_camera_data.initial_speed,
                                &scripted_camera_data.final_speed,
                                scripted_camera_data.rotation_direction, 0,
                                scripted_camera_data.travel_time);
                    } else {
                        scripted_camera_data.pos_move_done = move_to_end_point(
                            &target_position,
                            &scripted_camera_data.initial_speed,
                            &scripted_camera_data.final_speed, 0,
                            scripted_camera_data.travel_time);
                    }
                } else {
                    rate = scripted_camera_data.movement_rate;
                    RESOLVE_CAMERA_OBJ(camera);
                    direction.x = target_position.x - camera->pos.x;
                    direction.y = target_position.y - camera->pos.y;
                    direction.z = target_position.z - camera->pos.z;
                    distance = camera_sqrt(direction.x * direction.x +
                                           direction.y * direction.y +
                                           direction.z * direction.z);
                    if (distance < kSnapDist) {
                        camera->pos.x = target_position.x;
                        camera->pos.y = target_position.y;
                        camera->pos.z = target_position.z;
                        done = 1;
                    } else {
                        direction.x *= rate;
                        direction.y *= rate;
                        direction.z *= rate;
                        camera->pos.x += direction.x;
                        camera->pos.y += direction.y;
                        camera->pos.z += direction.z;
                        done = 0;
                    }
                    scripted_camera_data.pos_move_done = done;
                }
            }
        }

        if (scripted_camera_data.look_mode == 1 ||
            scripted_camera_data.look_mode == 2 ||
            scripted_camera_data.look_mode == 3) {
            RESOLVE_CAMERA_OBJ(camera);
            if (scripted_camera_data.glitch != 0) {
                camera->ang.x = target_angles.x;
                camera->ang.y = target_angles.y;
                camera->ang.z = target_angles.z;
                scripted_camera_data.ang_move_done = 1;
            } else {
                pitch_delta = target_angles.x - camera->ang.x;
                if (pitch_delta > kPi) {
                    pitch_delta -= kTwoPi;
                } else if (pitch_delta < kNegPi) {
                    pitch_delta += kTwoPi;
                }
                yaw_delta = target_angles.y - camera->ang.y;
                if (yaw_delta > kPi) {
                    yaw_delta -= kTwoPi;
                } else if (yaw_delta < kNegPi) {
                    yaw_delta += kTwoPi;
                }
                if (pitch_delta * pitch_delta + yaw_delta * yaw_delta <
                    1.0000001e-6f) {
                    if (scripted_camera_data.look_mode == 2) {
                        camera->ang.x = target_angles.x;
                        camera->ang.y = target_angles.y;
                        camera->ang.z = target_angles.z;
                    }
                    done = 1;
                } else {
                    rate = scripted_camera_data.rotation_rate;
                    pitch_delta *= rate;
                    yaw_delta *= rate;
                    roll_delta = kZero * rate;
                    camera->ang.x += pitch_delta;
                    camera->ang.y += yaw_delta;
                    camera->ang.z += roll_delta;
                    done = 0;
                }
                scripted_camera_data.ang_move_done = done;
            }
        } else if (scripted_camera_data.look_mode != 0) {
            if (scripted_camera_data.glitch != 0 ||
                scripted_camera_data.look_mode == 8 ||
                scripted_camera_data.look_mode == 6) {
                RESOLVE_CAMERA_OBJ(camera);
                direction.x = look_target.x - camera->pos.x;
                direction.y = look_target.y - camera->pos.y;
                direction.z = look_target.z - camera->pos.z;
                inverse_length = camera_inv_sqrt(
                    direction.x * direction.x + direction.y * direction.y +
                    direction.z * direction.z);
                direction.x *= inverse_length;
                direction.y *= inverse_length;
                direction.z *= inverse_length;
                v3_to_xy_ang_high_freq(&direct_angles, &direction);
                camera->ang.x = direct_angles.x;
                camera->ang.y = direct_angles.y;
                camera->ang.z = direct_angles.z;
            } else {
                if (scripted_camera_data.custom_movement != 0) {
                    direction.x = look_target.x - camera_obj->pos.x;
                    direction.y = look_target.y - camera_obj->pos.y;
                    direction.z = look_target.z - camera_obj->pos.z;
                } else {
                    direction.x = look_target.x - target_position.x;
                    direction.y = look_target.y - target_position.y;
                    direction.z = look_target.z - target_position.z;
                }
                v3_to_xy_ang_high_freq(&desired_angles, &direction);
                RESOLVE_CAMERA_OBJ(camera);
                pitch_delta = desired_angles.x - camera->ang.x;
                if (pitch_delta > kPi) {
                    pitch_delta -= kTwoPi;
                } else if (pitch_delta < kNegPi) {
                    pitch_delta += kTwoPi;
                }
                yaw_delta = desired_angles.y - camera->ang.y;
                if (yaw_delta > kPi) {
                    yaw_delta -= kTwoPi;
                } else if (yaw_delta < kNegPi) {
                    yaw_delta += kTwoPi;
                }
                if (pitch_delta * pitch_delta + yaw_delta * yaw_delta <
                    1.0000001e-6f) {
                    done = 1;
                } else {
                    rate = scripted_camera_data.rotation_rate;
                    pitch_delta *= rate;
                    yaw_delta *= rate;
                    roll_delta = kZero * rate;
                    camera->ang.x += pitch_delta;
                    camera->ang.y += yaw_delta;
                    camera->ang.z += roll_delta;
                    done = 0;
                }
                scripted_camera_data.ang_move_done = done;
            }
        }

        add_camera_offsets_impl();
        scripted_camera_data.glitch = 0;
        update_mkobj(camera_obj);
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
}

MkObj* camera_get_victim(void) {
    return camera_info.pdata->victim;
}

void camera_set_victim(MkObj* object) {
    camera_info.pdata->victim = object;
}

MkObj* camera_get_attacker(void) {
    return camera_info.pdata->attacker;
}

void camera_set_attacker(MkObj* object) {
    camera_info.pdata->attacker = object;
}

void camera_special_function(int function) {
    CameraObj* camera;
    Vec player_delta;
    Vec side;
    Vec position;
    Vec direction;
    Vec angles;
    float inverse_length;

    switch (function) {
    case 0: {
        MkObj* attacker = camera_info.pdata->attacker;
        MkObj* victim = camera_info.pdata->victim;

        player_delta.x = victim->pos.value.x - attacker->pos.value.x;
        player_delta.y = victim->pos.value.y - attacker->pos.value.y;
        player_delta.z = victim->pos.value.z - attacker->pos.value.z;
        side.x = player_delta.y * uv_y.z - player_delta.z * uv_y.y;
        side.y = player_delta.z * uv_y.x - player_delta.x * uv_y.z;
        side.z = player_delta.x * uv_y.y - player_delta.y * uv_y.x;
        if (xz_dot_xz(&side, &cam_forward_uv) > kZero) {
            scripted_camera_data.mirror = 1;
        } else {
            scripted_camera_data.mirror = 0;
        }
        xz_unit_vector(&scripted_camera_data.focus_direction,
                       &camera_info.pdata->attacker->pos.value,
                       &camera_info.pdata->victim->pos.value);
        scripted_camera_data.focus_angle =
            gxMathArcTanYX(scripted_camera_data.focus_direction.x,
                           scripted_camera_data.focus_direction.z);
        scripted_camera_data.focus_angle = normalize_interaction_angle(
            scripted_camera_data.focus_angle);
        break;
    }
    case 1:
        scripted_camera_data.mirror = 0;
        break;
    case 2:
        camera_obj->velocity.x =
            scripted_camera_data.movement_focus->pos.value.x - camera_obj->pos.x;
        camera_obj->velocity.y =
            scripted_camera_data.movement_focus->pos.value.y - camera_obj->pos.y;
        camera_obj->velocity.z =
            scripted_camera_data.movement_focus->pos.value.z - camera_obj->pos.z;
        camera_obj->velocity.x *= scripted_camera_data.movement_rate;
        camera_obj->velocity.y *= scripted_camera_data.movement_rate;
        camera_obj->velocity.z *= scripted_camera_data.movement_rate;
        break;
    case 3:
        camera_obj->velocity.z = kZero;
        camera_obj->velocity.y = kZero;
        camera_obj->velocity.x = kZero;
        break;
    case 4:
        get_play_camera_position(&position);
        RESOLVE_CAMERA_OBJ(camera);
        camera->pos.x = position.x;
        camera->pos.y = position.y;
        camera->pos.z = position.z;
        if (g_game_info.plyr0.slot.mirror_a != 0 &&
            g_game_info.plyr1.slot.mirror_a != 0) {
            RESOLVE_CAMERA_OBJ(camera);
            if (g_game_info.plyr0.slot.mirror_a != 0 &&
                g_game_info.plyr1.slot.mirror_a != 0) {
                position.x = g_game_info.plyr1.slot.mirror_a->pos.value.x +
                             g_game_info.plyr0.slot.mirror_a->pos.value.x;
                position.y = g_game_info.plyr1.slot.mirror_a->pos.value.y +
                             g_game_info.plyr0.slot.mirror_a->pos.value.y;
                position.z = g_game_info.plyr1.slot.mirror_a->pos.value.z +
                             g_game_info.plyr0.slot.mirror_a->pos.value.z;
                position.x *= kHalf;
                position.y *= kHalf;
                position.z *= kHalf;
            }
            position.y = -((3.0f * gxMathTan(0.1f)) -
                           (1.55f + camera->ground_plane));
        }
        RESOLVE_CAMERA_OBJ(camera);
        direction.x = position.x - camera->pos.x;
        direction.y = position.y - camera->pos.y;
        direction.z = position.z - camera->pos.z;
        inverse_length = camera_inv_sqrt(
            direction.x * direction.x + direction.y * direction.y +
            direction.z * direction.z);
        direction.x *= inverse_length;
        direction.y *= inverse_length;
        direction.z *= inverse_length;
        v3_to_xy_ang_high_freq(&angles, &direction);
        camera->ang.x = angles.x;
        camera->ang.y = angles.y;
        camera->ang.z = angles.z;
        turn_display_off();
        if (World != 0 && Camera != 0 && RwCameraGetWorld(Camera) != 0) {
            RpWorldRemoveCamera(World, Camera);
        }
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
        if (Camera != 0) {
            if (World != 0 && RwCameraGetWorld(Camera) == 0) {
                RpWorldAddCamera(World, Camera);
            }
            turn_display_on();
        }
        break;
    }
}

void camera_unpause_player(void) {
    reset_game_speed();
}

void camera_pause_player(void) {
    game_speed = kZero;
}

void camera_set_lookat_focus(MkObj* object) {
    scripted_camera_data.lookat_focus = object;
}

void camera_set_movement_focus_obj(MkObj* object) {
    scripted_camera_data.movement_focus = object;
}

int camera_get_mirror_flag(void) {
    return scripted_camera_data.mirror;
}

int is_a_to_the_right_of_b(MkObj* a, MkObj* b) {
    Vec delta;
    Vec right;

    delta.x = b->pos.value.x - a->pos.value.x;
    delta.y = b->pos.value.y - a->pos.value.y;
    delta.z = b->pos.value.z - a->pos.value.z;
    right.x = delta.y * uv_y.z - delta.z * uv_y.y;
    right.y = delta.z * uv_y.x - delta.x * uv_y.z;
    right.z = delta.x * uv_y.y - delta.y * uv_y.x;
    return xz_dot_xz(&right, &cam_forward_uv) > kZero;
}

void camera_setup_simple_rotation(int ticks, float rotation) {
    Vec direction;
    Vec angle;
    float radial_distance;
    float radial_step = (rotation * game_speed) / (float)ticks;

    if (scripted_camera_data.mirror != 0) {
        radial_step *= -1.0f;
    }
    if (scripted_camera_data.movement_focus != 0) {
        if (scripted_camera_data.lookat_focus != 0) {
            radial_distance = dist_xz_to_xz(
                &camera_obj->pos,
                &scripted_camera_data.movement_focus->pos.value);
            xz_unit_vector(
                &direction, &scripted_camera_data.lookat_focus->pos.value,
                &camera_obj->pos);
            v3_to_xy_ang_high_freq(&angle, &direction);
            scripted_camera_data.radial_vector.x =
                direction.x * radial_distance;
            scripted_camera_data.radial_vector.y =
                direction.y * radial_distance;
            scripted_camera_data.radial_vector.z =
                direction.z * radial_distance;
            scripted_camera_data.center_of_rotation.x =
                scripted_camera_data.lookat_focus->pos.value.x;
            scripted_camera_data.center_of_rotation.y =
                scripted_camera_data.lookat_focus->pos.value.y;
            scripted_camera_data.center_of_rotation.z =
                scripted_camera_data.lookat_focus->pos.value.z;
            scripted_camera_data.radial_distance = radial_distance;
            scripted_camera_data.radial_step = radial_step;
        }
    }
}

void camera_setup_tightrope_angle_offset(void* script_args, float height,
                                         float angle) {
    if (scripted_camera_data.mirror != 0) {
        angle = kTwoPi - angle;
        angle = 0.000005992112f *
                (float)((int)(166886.1f * angle) & 0xFFFFF);
    }
    scripted_camera_data.lookat_offset.x = height;
    scripted_camera_data.lookat_offset.y = angle;
}

void camera_setup_radial_position(void* script_args, float distance,
                                  float angle, float height) {
    if (scripted_camera_data.mirror != 0) {
        angle = kTwoPi - angle;
        angle = 0.000005992112f *
                (float)((int)(166886.1f * angle) & 0xFFFFF);
    }
    scripted_camera_data.center_of_rotation.z = angle;
    scripted_camera_data.center_of_rotation.y = distance;
    scripted_camera_data.movement_offset.y = height;
}

void camera_setup_radial_sweep(void* script_args, float travel_time,
                               float rotation_step, float initial_speed,
                               float final_speed, float radial_step,
                               float radial_distance, float center_x,
                               float center_y, float start_angle) {
    if (start_angle >= kZero) {
        scripted_camera_data.center_of_rotation.z = start_angle;
    } else {
        scripted_camera_data.center_of_rotation.z = camera_obj->ang.y;
    }
    scripted_camera_data.initial_speed = initial_speed;
    scripted_camera_data.center_of_rotation.x = center_x;
    scripted_camera_data.rotation_step = rotation_step;
    scripted_camera_data.radial_distance = radial_distance;
    scripted_camera_data.travel_time = travel_time;
    scripted_camera_data.radial_step = radial_step;
    scripted_camera_data.final_speed = final_speed;
    scripted_camera_data.center_of_rotation.y = center_y;
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

void camera_set_lookat_offset_obj_rel(const Vec* offset, void* script_args) {
    MkObj* focus = scripted_camera_data.lookat_focus;

    if (focus != 0) {
        v3_x_mat_add_v3(&scripted_camera_data.lookat_offset, offset,
                        focus->field_24, &focus->pos.value);
    }
}

void camera_set_lookat_offset(Vec* offset, void* script_args) {
    offset->y += g_game_info.field_34;
    scripted_camera_data.lookat_offset.x = offset->x;
    scripted_camera_data.lookat_offset.y = offset->y;
    scripted_camera_data.lookat_offset.z = offset->z;
}

void cam_recalc_midpoint(void) {
    force_midpoint_calculation_update = 1;
    if ((g_game_info.field_04 >> 5) & 1) {
        xfer_camera_impl(p_attract_camera, 1);
    } else {
        xfer_camera_impl(p_camera_proc, 1);
    }
}

float camera_get_pos(unsigned int axis) {
    CameraObj* camera;
    float position;

    RESOLVE_CAMERA_OBJ(camera);
    position = camera->pos.x;
    if (axis == 1) {
        position = camera->pos.y;
    } else if (axis == 2) {
        position = camera->pos.z;
    }
    return position;
}

/* Soft ceiling: exact XZ selection; Vec stack/FPR and latch scheduling differ. */
void find_best_conversation_camera_position(void) {
    MkObj* focus = scripted_camera_data.lookat_focus;
    InteractionNpc* npc = get_konquest_movement_npc();
    Vec focus_to_npc = {0.0f, 0.0f, 0.0f};
    Vec camera_to_focus = {0.0f, 0.0f, 0.0f};
    Vec right_offset;
    Vec left_offset;
    Vec right_hit;
    Vec left_hit;
    Vec right_position;
    Vec left_position;

    if (focus != 0 || npc != 0) {
        uv_v3_to_v3(&focus_to_npc, &focus->pos.value, &npc->pos);
        focus_to_npc.y = kZero;
        uv_v3_to_v3(&camera_to_focus, &camera_obj->pos, &focus->pos.value);
        camera_to_focus.y = kZero;

        focus_to_npc.x *= 1.15f;
        focus_to_npc.y *= 1.15f;
        focus_to_npc.z *= 1.15f;
        rotate_xz(&right_offset, &focus_to_npc, 0.45f);
        rotate_xz(&left_offset, &focus_to_npc, -0.45f);

        right_position.x = focus->pos.value.x + right_offset.x;
        right_position.z = focus->pos.value.z + right_offset.z;
        left_position.x = focus->pos.value.x + left_offset.x;
        left_position.z = focus->pos.value.z + left_offset.z;

        if (repel_point_against_global_collision_list_toward_target(
                &right_position, &focus->pos.value, &right_hit, 0x10002)) {
            right_position = right_hit;
        }
        if (repel_point_against_global_collision_list_toward_target(
                &left_position, &focus->pos.value, &left_hit, 0x10002)) {
            left_position = left_hit;
        }

        if (dist_xz_to_xz(&camera_obj->pos, &right_position) <
            dist_xz_to_xz(&camera_obj->pos, &left_position)) {
            right_position.x -= focus->pos.value.x;
            right_position.y = 0.7f;
            right_position.y += g_game_info.field_34;
            right_position.z -= focus->pos.value.z;
            scripted_camera_data.movement_offset.x = right_position.x;
            scripted_camera_data.movement_offset.y = right_position.y;
            scripted_camera_data.movement_offset.z = right_position.z;
        } else {
            left_position.x -= focus->pos.value.x;
            left_position.y = 0.7f;
            left_position.y += g_game_info.field_34;
            left_position.z -= focus->pos.value.z;
            scripted_camera_data.movement_offset.x = left_position.x;
            scripted_camera_data.movement_offset.y = left_position.y;
            scripted_camera_data.movement_offset.z = left_position.z;
        }

        if (camera_to_focus.x * focus_to_npc.x +
                camera_to_focus.z * focus_to_npc.z >
            kZero) {
            scripted_camera_data.glitch = 1;
            camera_info.pdata->flags_bits.konquest_mode = 1;
        }
    }
}

void camera_check_reverse_move_offset(int expected_mask, int skip_mask) {
    int reverse_x = 0;
    int reverse_z = 0;
    int detected_mask = 0;
    Vec offset;
    Vec reverse_x_offset;
    Vec reverse_z_offset;
    Vec world_offset;
    Vec world_reverse;
    float original_distance;

    offset.x = scripted_camera_data.movement_offset.x;
    offset.y = scripted_camera_data.movement_offset.y;
    offset.z = scripted_camera_data.movement_offset.z;
    reverse_x_offset.x = offset.x;
    reverse_x_offset.y = offset.y;
    reverse_x_offset.z = offset.z;
    reverse_z_offset.x = offset.x;
    reverse_z_offset.y = offset.y;
    reverse_z_offset.z = offset.z;
    reverse_x_offset.x = -reverse_x_offset.x;
    reverse_z_offset.z = -reverse_z_offset.z;
    if (scripted_camera_data.movement_focus != 0) {
        v3_x_mat_add_v3(
            &world_offset, &offset,
            scripted_camera_data.movement_focus->field_24,
            &scripted_camera_data.movement_focus->pos.value);
        v3_x_mat_add_v3(
            &world_reverse, &reverse_x_offset,
            scripted_camera_data.movement_focus->field_24,
            &scripted_camera_data.movement_focus->pos.value);
        original_distance =
            dist_xz_to_xz(&world_offset, &camera_obj->pos);
        if (original_distance >
            dist_xz_to_xz(&world_reverse, &camera_obj->pos)) {
            reverse_x = 1;
        }
        v3_x_mat_add_v3(
            &world_reverse, &reverse_z_offset,
            scripted_camera_data.movement_focus->field_24,
            &scripted_camera_data.movement_focus->pos.value);
        if (original_distance >
            dist_xz_to_xz(&world_reverse, &camera_obj->pos)) {
            reverse_z = 1;
        }
    } else {
        if (dist_xz_to_xz(&offset, &camera_obj->pos) >
            dist_xz_to_xz(&reverse_x_offset, &camera_obj->pos)) {
            reverse_x = 1;
        }
        if (dist_xz_to_xz(&offset, &camera_obj->pos) >
            dist_xz_to_xz(&reverse_z_offset, &camera_obj->pos)) {
            reverse_z = 1;
        }
    }

    if (reverse_x != 0 && (skip_mask & 1) == 0) {
        detected_mask |= 1;
        if ((expected_mask & 1) != 0) {
            scripted_camera_data.movement_offset.x =
                -scripted_camera_data.movement_offset.x;
        }
    }
    if (reverse_z != 0 && (skip_mask & 2) == 0) {
        detected_mask |= 2;
        if ((expected_mask & 2) != 0) {
            scripted_camera_data.movement_offset.z =
                -scripted_camera_data.movement_offset.z;
        }
    }
    if (detected_mask != 0 && expected_mask != detected_mask) {
        scripted_camera_data.glitch = 1;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
        scripted_camera_data.movement_mode = 0;
        scripted_camera_data.look_mode = 0;
    }
}

void camera_set_custom_camera_movement_flag(int enabled) {
    scripted_camera_data.custom_movement = enabled;
}

void camera_set_radial_movement(int enabled) {
    scripted_camera_data.radial_movement = enabled;
}

/* Soft ceiling: only the global-base addi schedules one instruction early. */
void camera_set_center_of_rotation(const CamVec3* center) {
    Vec* rotation_center = &scripted_camera_data.center_of_rotation;

    rotation_center->x = center->x;
    rotation_center->y = center->y;
    rotation_center->z = center->z;
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

void camera_set_movement_offset_obj_rel(const Vec* offset, void* script_args) {
    MkObj* focus = scripted_camera_data.movement_focus;

    if (focus != 0) {
        v3_x_mat_add_v3(&scripted_camera_data.movement_offset, offset,
                        focus->field_24, &focus->pos.value);
    }
}

void camera_set_movement_offset(Vec* offset, void* script_args) {
    offset->y += g_game_info.field_34;
    scripted_camera_data.movement_offset.x = offset->x;
    scripted_camera_data.movement_offset.y = offset->y;
    scripted_camera_data.movement_offset.z = offset->z;
}

void camera_set_look_mode(int mode) {
    scripted_camera_data.look_mode = mode;
}

void camera_set_movement_mode(int mode) {
    scripted_camera_data.movement_mode = mode;
}

float camera_wait_for_pos_and_ang_move_done(void) {
    while (scripted_camera_data.pos_move_done == 0 ||
           scripted_camera_data.ang_move_done == 0) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
    return kZero;
}

float camera_wait_for_ang_move_done(void) {
    while (scripted_camera_data.ang_move_done == 0) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
    return kZero;
}

float camera_wait_for_pos_move_done(void) {
    while (scripted_camera_data.pos_move_done == 0) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
    return kZero;
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
    memset(&scripted_camera_data, 0, sizeof(scripted_camera_data));
    {
        float initial_speed = kZero;
        float final_speed = kZero;
        Vec endpoint = { 0.0f, 0.0f, 0.0f };

        move_to_end_point(&endpoint, &initial_speed, &final_speed, 1, kZero);
        orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1, 1,
                                    kZero);
    }
}

float p_attract_camera(void) {
    AttractCameraState state;
    AttractCameraSetup* callbacks = 0;
    int use_glitch_move = 1;
    int next_mode;
    CameraObj* camera;

    RESOLVE_CAMERA_OBJ(camera);
    state.camera = camera;
    state.countdown = 0;
    state.mode = -1;
    g_game_info.pause_flag_bits.pad_bit6 = 1;
    if (camera == 0) {
        return kNegOne;
    }

    for (;;) {
        if (state.countdown == 0) {
            use_glitch_move = 1;
            state.countdown = (unsigned short)randu0(360) + 240;
            if (state.mode == -1) {
                state.mode = 1;
            } else {
                do {
                    next_mode = (unsigned short)randu0(4);
                } while (next_mode == state.mode);
                state.mode = next_mode;
            }
            callbacks = &attract_cam_setup_table[state.mode];
            if (callbacks->setup != 0) {
                callbacks->setup(&state);
            }
        }
        if (callbacks->update != 0) {
            callbacks->update(&state);
        }
        if (use_glitch_move != 0) {
            if (callbacks->glitch_move != 0) {
                callbacks->glitch_move(&state);
            }
            use_glitch_move = 0;
        } else if (callbacks->move != 0) {
            callbacks->move(&state);
        }
        _mkproc_sleep_ticks = kOne;
        state.countdown--;
        mkproc_sleep();
    }
}

/* Soft ceiling: typed-object reload/CSE and validated-latch scheduling only. */
static void attract_glitch_move_gamecam(AttractCameraState* state) {
    CameraObj* camera;
    Vec direction;

    get_play_camera_position(&state->current_position);
    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        RESOLVE_CAMERA_OBJ(camera);
        if (g_game_info.plyr0.slot.mirror_a != 0 &&
            g_game_info.plyr1.slot.mirror_a != 0) {
            state->target_position.x =
                g_game_info.plyr1.slot.mirror_a->pos.value.x +
                g_game_info.plyr0.slot.mirror_a->pos.value.x;
            state->target_position.y =
                g_game_info.plyr1.slot.mirror_a->pos.value.y +
                g_game_info.plyr0.slot.mirror_a->pos.value.y;
            state->target_position.z =
                g_game_info.plyr1.slot.mirror_a->pos.value.z +
                g_game_info.plyr0.slot.mirror_a->pos.value.z;
            state->target_position.x *= kHalf;
            state->target_position.y *= kHalf;
            state->target_position.z *= kHalf;
        }
        state->target_position.y =
            -((3.0f * gxMathTan(0.1f)) -
              (1.55f + camera->ground_plane));
    }

    direction.x = state->target_position.x - state->current_position.x;
    direction.y = state->target_position.y - state->current_position.y;
    direction.z = state->target_position.z - state->current_position.z;
    v3_to_xy_ang(&state->current_angles, &direction);
    state->camera->pos.x = state->current_position.x;
    state->camera->pos.y = state->current_position.y;
    state->camera->pos.z = state->current_position.z;
    state->camera->ang.x = state->current_angles.x;
    state->camera->ang.y = state->current_angles.y;
    state->camera->ang.z = state->current_angles.z;
}

/* Soft ceiling: exact instructions; only the -0.15f relocation label differs. */
static void attract_default_glitch_move(AttractCameraState* state) {
    Vec direction;

    direction.x = state->target_position.x - state->current_position.x;
    direction.y = state->target_position.y - state->current_position.y;
    direction.z = state->target_position.z - state->current_position.z;
    v3_to_xy_ang(&state->current_angles, &direction);
    if (state->mode == 0) {
        state->current_angles.y += -0.15f;
    }
    state->camera->pos.x = state->current_position.x;
    state->camera->pos.y = state->current_position.y;
    state->camera->pos.z = state->current_position.z;
    state->camera->ang.x = state->current_angles.x;
    state->camera->ang.y = state->current_angles.y;
    state->camera->ang.z = state->current_angles.z;
}

static void attract_move_flyby(AttractCameraState* state) {
    CameraObj* camera;
    Vec direction;
    float delta_x;
    float delta_y;
    float angle_rate;
    float roll_delta;

    direction.x = state->target_position.x - state->camera->pos.x;
    direction.y = state->target_position.y - state->camera->pos.y;
    direction.z = state->target_position.z - state->camera->pos.z;
    v3_to_xy_ang(&state->current_angles, &direction);

    angle_rate = state->field_48;
    RESOLVE_CAMERA_OBJ(camera);
    camera->pos.x = state->current_position.x;
    camera->pos.y = state->current_position.y;
    camera->pos.z = state->current_position.z;

    RESOLVE_CAMERA_OBJ(camera);
    delta_x = state->current_angles.x - camera->ang.x;
    if (delta_x > kPi) {
        delta_x -= kTwoPi;
    } else if (delta_x < kNegPi) {
        delta_x += kTwoPi;
    }
    delta_y = state->current_angles.y - camera->ang.y;
    if (delta_y > kPi) {
        delta_y -= kTwoPi;
    } else if (delta_y < kNegPi) {
        delta_y += kTwoPi;
    }
    if (!(delta_x * delta_x + delta_y * delta_y < kAngEpsSq)) {
        delta_x *= angle_rate;
        delta_y *= angle_rate;
        roll_delta = kZero * angle_rate;
        camera->ang.x = camera->ang.x + delta_x;
        camera->ang.y = camera->ang.y + delta_y;
        camera->ang.z = camera->ang.z + roll_delta;
    }
}

/* Soft ceiling: validated-latch and inline sqrt FPR/stack allocation only. */
static void attract_default_move(AttractCameraState* state) {
    CameraObj* camera;
    Vec direction;
    float dx;
    float dy;
    float dz;
    float distance;
    float delta_x;
    float delta_y;
    float move_rate;
    float angle_rate;
    float roll_delta;

    direction.x = state->target_position.x - state->camera->pos.x;
    direction.y = state->target_position.y - state->camera->pos.y;
    direction.z = state->target_position.z - state->camera->pos.z;
    v3_to_xy_ang(&state->current_angles, &direction);
    if (state->mode == 0) {
        state->current_angles.y += -0.15f;
    }

    move_rate = state->field_44;
    RESOLVE_CAMERA_OBJ(camera);
    dx = state->current_position.x - camera->pos.x;
    dy = state->current_position.y - camera->pos.y;
    dz = state->current_position.z - camera->pos.z;
    distance = camera_sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < 0.01f) {
        camera->pos.x = state->current_position.x;
        camera->pos.y = state->current_position.y;
        camera->pos.z = state->current_position.z;
    } else {
        dx *= move_rate;
        dy *= move_rate;
        dz *= move_rate;
        camera->pos.x = camera->pos.x + dx;
        camera->pos.y = camera->pos.y + dy;
        camera->pos.z = camera->pos.z + dz;
    }

    angle_rate = state->field_48;
    RESOLVE_CAMERA_OBJ(camera);
    delta_x = state->current_angles.x - camera->ang.x;
    if (delta_x > kPi) {
        delta_x -= kTwoPi;
    } else if (delta_x < kNegPi) {
        delta_x += kTwoPi;
    }
    delta_y = state->current_angles.y - camera->ang.y;
    if (delta_y > kPi) {
        delta_y -= kTwoPi;
    } else if (delta_y < kNegPi) {
        delta_y += kTwoPi;
    }
    if (!(delta_x * delta_x + delta_y * delta_y < kAngEpsSq)) {
        delta_x *= angle_rate;
        delta_y *= angle_rate;
        roll_delta = kZero * angle_rate;
        camera->ang.x = camera->ang.x + delta_x;
        camera->ang.y = camera->ang.y + delta_y;
        camera->ang.z = camera->ang.z + roll_delta;
    }
}

/* Soft ceiling: exact size; latch branch and inline sqrt FPR scheduling only. */
static void attract_update_flyby(AttractCameraState* state) {
    CameraObj* camera;
    float distance;

    RESOLVE_CAMERA_OBJ(camera);
    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        state->target_position.x =
            g_game_info.plyr1.slot.mirror_a->pos.value.x +
            g_game_info.plyr0.slot.mirror_a->pos.value.x;
        state->target_position.y =
            g_game_info.plyr1.slot.mirror_a->pos.value.y +
            g_game_info.plyr0.slot.mirror_a->pos.value.y;
        state->target_position.z =
            g_game_info.plyr1.slot.mirror_a->pos.value.z +
            g_game_info.plyr0.slot.mirror_a->pos.value.z;
        state->target_position.x *= kHalf;
        state->target_position.y *= kHalf;
        state->target_position.z *= kHalf;
    }
    state->target_position.y = camera->ground_plane + 1.55f;
    state->current_position.x += state->center.x;
    state->current_position.y += state->center.y;
    state->current_position.z += state->center.z;
    distance = camera_sqrt(
        state->current_position.x * state->current_position.x +
        state->current_position.z * state->current_position.z);
    if (distance > 12.0f) {
        state->countdown = 1;
    }
}

static void attract_update_chase_cam(AttractCameraState* state) {
    Vec forward = {0.0f, 0.0f, 1.0f};
    Vec* current_position;

    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        state->target_position.x =
            g_game_info.plyr1.slot.mirror_a->pos.value.x +
            g_game_info.plyr0.slot.mirror_a->pos.value.x;
        state->target_position.y =
            g_game_info.plyr1.slot.mirror_a->pos.value.y +
            g_game_info.plyr0.slot.mirror_a->pos.value.y;
        state->target_position.z =
            g_game_info.plyr1.slot.mirror_a->pos.value.z +
            g_game_info.plyr0.slot.mirror_a->pos.value.z;
        state->target_position.x *= kHalf;
        state->target_position.y *= kHalf;
        state->target_position.z *= kHalf;
    }
    state->target_position.y += kHalf;
    current_position = &state->current_position;
    if (current_position != 0) {
        current_position->x = forward.x;
        current_position->y = forward.y;
        current_position->z = forward.z;
        current_position->x *= 2.0f;
        current_position->y *= 2.0f;
        current_position->z *= 2.0f;
        rotate_xz(current_position, current_position,
                  state->target->ang.y + 2.6f);
    }
    state->current_position.x += state->target->pos.value.x;
    state->current_position.y += state->target->pos.value.y;
    state->current_position.z += state->target->pos.value.z;
    state->current_position.y = camera_obj->ground_plane + 2.0f;
}

static void attract_move_gamecam(AttractCameraState* state) {
    adj_cam_pos();
}

/* Soft ceiling: latch diamonds, repeated GameInfo loads, and FPR scheduling. */
static void attract_update_radial_sweep(AttractCameraState* state) {
    CameraObj* camera;

    RESOLVE_CAMERA_OBJ(camera);
    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        state->target_position.x =
            g_game_info.plyr1.slot.mirror_a->pos.value.x +
            g_game_info.plyr0.slot.mirror_a->pos.value.x;
        state->target_position.y =
            g_game_info.plyr1.slot.mirror_a->pos.value.y +
            g_game_info.plyr0.slot.mirror_a->pos.value.y;
        state->target_position.z =
            g_game_info.plyr1.slot.mirror_a->pos.value.z +
            g_game_info.plyr0.slot.mirror_a->pos.value.z;
        state->target_position.x *= kHalf;
        state->target_position.y *= kHalf;
        state->target_position.z *= kHalf;
    }
    state->target_position.x =
        kHalf * (state->center.x + state->target_position.x);
    state->target_position.y =
        kHalf * (state->center.y + state->target_position.y);
    state->target_position.z =
        kHalf * (state->center.z + state->target_position.z);
    state->target_position.y = camera->ground_plane + 1.55f;
    state->center.x = state->target_position.x;
    state->center.y = state->target_position.y;
    state->center.z = state->target_position.z;

    state->field_0C += 0.001f;
    if (state->field_0C >= 0.01f) {
        state->field_0C = 0.01f;
    } else if (state->field_0C <= 0.005f) {
        state->field_0C = 0.005f;
    }
    state->field_10 += 0.03f;
    if (state->field_10 >= 7.0f) {
        state->field_10 = 7.0f;
    } else if (state->field_10 <= 2.6f) {
        state->field_10 = 2.6f;
    }
    state->field_18 += 0.01f;
    if (state->field_18 >= 4.0f) {
        state->field_18 = 4.0f;
    } else if (state->field_18 <= 0.5f) {
        state->field_18 = 0.5f;
    }
    state->field_14 += state->field_0C;
    state->current_position.x =
        state->field_10 * gxMathCos(state->field_14) +
        state->target_position.x;
    state->current_position.z =
        state->field_10 * gxMathSin(state->field_14) +
        state->target_position.z;
    state->current_position.y =
        camera_obj->ground_plane + state->field_18;
}

static void attract_setup_flyby(AttractCameraState* state) {
    MkObj* player;
    Vec start;
    Vec travel;
    Vec offset;
    float offset_distance;
    float signed_distance;
    float player_dot;
    float forward_distance;
    float reverse_distance;
    float end_x;
    float end_y;
    float end_z;
    float length_squared;
    float inverse_length;

    offset_distance = 2.0f + frand(3.0f);
    player = g_game_info.plyr0.slot.mirror_a;
    player_dot = player->pos.value.z * tightrope_perp_uv.z +
                 (player->pos.value.x * tightrope_perp_uv.x +
                  player->pos.value.y * tightrope_perp_uv.y);
    if (player_dot > kZero) {
        signed_distance = kNegOne * offset_distance;
        offset.x = tightrope_perp_uv.x * signed_distance;
        offset.y = tightrope_perp_uv.y * signed_distance;
        offset.z = tightrope_perp_uv.z * signed_distance;
    } else {
        offset.x = tightrope_perp_uv.x * offset_distance;
        offset.y = tightrope_perp_uv.y * offset_distance;
        offset.z = tightrope_perp_uv.z * offset_distance;
    }
    start.x = player->pos.value.x + offset.x;
    start.y = player->pos.value.y + offset.y;
    start.z = player->pos.value.z + offset.z;
    start.y = camera_obj->ground_plane + 2.5f;

    forward_distance =
        xz_ray_circle_intersection_dist(&start, &tightrope_uv, 11.5f);
    travel.x = -tightrope_uv.x;
    travel.y = -tightrope_uv.y;
    travel.z = -tightrope_uv.z;
    reverse_distance =
        xz_ray_circle_intersection_dist(&start, &travel, 11.5f);
    if (forward_distance > reverse_distance) {
        travel.x = tightrope_uv.x * forward_distance;
        travel.y = tightrope_uv.y * forward_distance;
        travel.z = tightrope_uv.z * forward_distance;
    } else {
        travel.x *= reverse_distance;
        travel.y *= reverse_distance;
        travel.z *= reverse_distance;
    }

    end_x = start.x + travel.x;
    end_y = start.y + travel.y;
    end_z = start.z + travel.z;
    length_squared = travel.z * travel.z +
                     (travel.x * travel.x + travel.y * travel.y);
    inverse_length = camera_inv_sqrt(length_squared);
    state->center.x = travel.x * inverse_length;
    state->center.y = travel.y * inverse_length;
    state->center.z = travel.z * inverse_length;
    state->field_44 = 0.1f;
    state->field_48 = 0.2f;
    state->center.x *= -state->field_44;
    state->center.y *= -state->field_44;
    state->center.z *= -state->field_44;
    state->current_position.x = end_x;
    state->current_position.y = end_y;
    state->current_position.z = end_z;
}

static void attract_setup_gamecam(AttractCameraState* state) {
    force_midpoint_calculation_update = 1;
}

static void attract_setup_chase_cam(AttractCameraState* state) {
    state->field_44 = 0.2f;
    state->field_48 = 0.2f;
    if (randu0(2) == 0) {
        state->target = g_game_info.plyr0.slot.mirror_a;
    } else {
        state->target = g_game_info.plyr1.slot.mirror_a;
    }
}

static void attract_setup_radial_sweep(AttractCameraState* state) {
    state->field_0C = 0.005f;
    state->field_10 = 3.0f;
    state->field_14 = 0.0f;
    state->field_18 = 2.0f;
    state->field_44 = 0.025f;
    state->field_48 = 0.05f;

    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        state->center.x =
            g_game_info.plyr1.slot.mirror_a->pos.value.x +
            g_game_info.plyr0.slot.mirror_a->pos.value.x;
        state->center.y =
            g_game_info.plyr1.slot.mirror_a->pos.value.y +
            g_game_info.plyr0.slot.mirror_a->pos.value.y;
        state->center.z =
            g_game_info.plyr1.slot.mirror_a->pos.value.z +
            g_game_info.plyr0.slot.mirror_a->pos.value.z;
        state->center.x = kHalf * state->center.x;
        state->center.y = kHalf * state->center.y;
        state->center.z = kHalf * state->center.z;
    }
}

float p_puzzle_game_camera_proc(void) {
    CameraObj* camera;
    Vec target_position;
    float target_pitch;
    float target_yaw;
    float dx;
    float dy;
    float dz;
    float distance;
    float pitch_delta;
    float yaw_delta;
    float roll_delta;

    RESOLVE_CAMERA_OBJ(camera);
    target_position.x = camera->pos.x;
    target_position.y = camera->pos.y;
    target_position.z = camera->pos.z;
    target_pitch = camera->ang.x;
    target_yaw = camera->ang.y;

    for (;;) {
        RESOLVE_CAMERA_OBJ(camera);
        camera->pos.x -= old_cam_pos_offset.x;
        camera->pos.y -= old_cam_pos_offset.y;
        camera->pos.z -= old_cam_pos_offset.z;
        camera->ang.x -= old_cam_ang_offset.x;
        camera->ang.y -= old_cam_ang_offset.y;
        camera->ang.z -= old_cam_ang_offset.z;

        RESOLVE_CAMERA_OBJ(camera);
        dx = target_position.x - camera->pos.x;
        dy = target_position.y - camera->pos.y;
        dz = target_position.z - camera->pos.z;
        distance = camera_sqrt(dx * dx + dy * dy + dz * dz);
        if (distance < 0.01f) {
            camera->pos.x = target_position.x;
            camera->pos.y = target_position.y;
            camera->pos.z = target_position.z;
        } else {
            dx *= 0.1f;
            dy *= 0.1f;
            dz *= 0.1f;
            camera->pos.x = camera->pos.x + dx;
            camera->pos.y = camera->pos.y + dy;
            camera->pos.z = camera->pos.z + dz;
        }

        RESOLVE_CAMERA_OBJ(camera);
        pitch_delta = target_pitch - camera->ang.x;
        if (pitch_delta > kPi) {
            pitch_delta -= kTwoPi;
        } else if (pitch_delta < kNegPi) {
            pitch_delta += kTwoPi;
        }
        yaw_delta = target_yaw - camera->ang.y;
        if (yaw_delta > kPi) {
            yaw_delta -= kTwoPi;
        } else if (yaw_delta < kNegPi) {
            yaw_delta += kTwoPi;
        }
        if (!(pitch_delta * pitch_delta + yaw_delta * yaw_delta <
              kAngEpsSq)) {
            pitch_delta *= 0.1f;
            yaw_delta *= 0.1f;
            roll_delta = kZero * 0.1f;
            camera->ang.x = camera->ang.x + pitch_delta;
            camera->ang.y = camera->ang.y + yaw_delta;
            camera->ang.z = camera->ang.z + roll_delta;
        }

        RESOLVE_CAMERA_OBJ(camera);
        old_cam_ang_offset.x = cam_ang_offset.x;
        old_cam_ang_offset.y = cam_ang_offset.y;
        old_cam_ang_offset.z = cam_ang_offset.z;
        old_cam_pos_offset.x = cam_pos_offset.x;
        old_cam_pos_offset.y = cam_pos_offset.y;
        old_cam_pos_offset.z = cam_pos_offset.z;
        camera->pos.x += cam_pos_offset.x;
        camera->pos.y += cam_pos_offset.y;
        camera->pos.z += cam_pos_offset.z;
        camera->ang.x += cam_ang_offset.x;
        camera->ang.y += cam_ang_offset.y;
        camera->ang.z += cam_ang_offset.z;
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
    }
}

/* Soft ceiling: inline sqrt/invsqrt stack allocation and FPR scheduling. */
void get_target_movement_vector(const Vec* current_position,
                                const Vec* target_position, Vec* movement,
                                float duration) {
    if (current_position != 0 && target_position != 0) {
        if (movement == 0) {
            return;
        }
        get_target_movement_vector_impl(current_position, target_position,
                                        movement, duration);
    }
}

void set_camera_target_angle(CamVec3* angle) {
    camera_info.pdata->target_ang.x = angle->x;
    camera_info.pdata->target_ang.y = angle->y;
    camera_info.pdata->target_ang.z = angle->z;
}

void set_camera_destination(const CamVec3* position) {
    int pos_done = 0;
    CameraPdata** pdata = &camera_info.pdata;

    (*pdata)->target_pos.x = position->x;
    (*pdata)->target_pos.y = position->y;
    (*pdata)->target_pos.z = position->z;
    (*pdata)->flags_bits.pos_done = pos_done;
}

void skip_camera_intro(void) {
    CameraObj* camera;
    Vec endpoint = {0.0f, 0.0f, 0.0f};
    Vec position;
    Vec direction;
    Vec angles;
    float initial_speed = kZero;
    float final_speed = kZero;
    float inverse_length;

    move_to_end_point(&endpoint, &initial_speed, &final_speed, 1, kZero);
    orbit_position_to_end_point(0, 0, &initial_speed, &final_speed, 1, 1,
                                kZero);
    get_play_camera_position(&position);
    RESOLVE_CAMERA_OBJ(camera);
    camera->pos.x = position.x;
    camera->pos.y = position.y;
    camera->pos.z = position.z;

    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        RESOLVE_CAMERA_OBJ(camera);
        if (g_game_info.plyr0.slot.mirror_a != 0 &&
            g_game_info.plyr1.slot.mirror_a != 0) {
            position.x = g_game_info.plyr1.slot.mirror_a->pos.value.x +
                         g_game_info.plyr0.slot.mirror_a->pos.value.x;
            position.y = g_game_info.plyr1.slot.mirror_a->pos.value.y +
                         g_game_info.plyr0.slot.mirror_a->pos.value.y;
            position.z = g_game_info.plyr1.slot.mirror_a->pos.value.z +
                         g_game_info.plyr0.slot.mirror_a->pos.value.z;
            position.x *= kHalf;
            position.y *= kHalf;
            position.z *= kHalf;
        }
        position.y =
            -(3.0f * gxMathTan(0.1f) -
              (1.55f + camera->ground_plane));
    }

    RESOLVE_CAMERA_OBJ(camera);
    direction.x = position.x - camera->pos.x;
    direction.y = position.y - camera->pos.y;
    direction.z = position.z - camera->pos.z;
    inverse_length = camera_inv_sqrt(
        direction.x * direction.x + direction.y * direction.y +
        direction.z * direction.z);
    direction.x *= inverse_length;
    direction.y *= inverse_length;
    direction.z *= inverse_length;
    v3_to_xy_ang_high_freq(&angles, &direction);
    camera->ang.x = angles.x;
    camera->ang.y = angles.y;
    camera->ang.z = angles.z;
    update_mkobj(camera_obj);
    camera_mode = 1;
    g_game_info.pause_flags |= 0x40;
    force_midpoint_calculation_update = 1;
    end_round_cam_done = 0;
}

int intro_done(void) {
    return (g_game_info.pause_flags >> 6) & 1;
}

static int radial_move_to_game_position(const Vec* target_position,
                                        const Vec* center, float rate) {
    CameraObj* camera;
    Vec current;
    Vec target;
    Vec radial;
    float current_radius;
    float target_radius;
    float current_angle;
    float angle_delta;
    float radius_delta;
    float absolute_angle_delta;
    float absolute_radius_delta;
    float next_radius;
    float next_height;

    RESOLVE_CAMERA_OBJ(camera);
    if (camera == 0) {
        return 0;
    }

    current.x = camera->pos.x - center->x;
    current.y = camera->pos.y - center->y;
    current.z = camera->pos.z - center->z;
    current_radius =
        camera_sqrt(current.x * current.x + current.z * current.z);
    current_angle = (float)atan2((double)current.x, (double)current.z);

    target.x = target_position->x - center->x;
    target.y = target_position->y - center->y;
    target.z = target_position->z - center->z;
    target_radius = camera_sqrt(target.x * target.x + target.z * target.z);
    angle_delta =
        (float)atan2((double)target.x, (double)target.z) - current_angle;
    radius_delta = target_radius - current_radius;
    next_radius = current_radius + radius_delta * rate;

    if (angle_delta < kNegPi) {
        angle_delta += kTwoPi;
    } else if (angle_delta > kPi) {
        angle_delta -= kTwoPi;
    }
    absolute_angle_delta =
        angle_delta >= kZero ? angle_delta : -angle_delta;
    if (absolute_angle_delta < 0.001f) {
        absolute_radius_delta =
            radius_delta >= kZero ? radius_delta : -radius_delta;
        if (absolute_radius_delta < 0.01f) {
            return 1;
        }
    }

    next_height = camera->pos.y +
                  (target_position->y - camera->pos.y) * rate;
    radial = (Vec){0.0f, 0.0f, 0.0f};
    rotate_xz(&radial, &Zaxis, current_angle + angle_delta * rate);
    radial.x *= next_radius;
    radial.y *= next_radius;
    radial.z *= next_radius;
    camera->pos.x = radial.x + center->x;
    camera->pos.y = radial.y + center->y;
    camera->pos.z = radial.z + center->z;
    camera->pos.y = next_height;
    return 0;
}

int orbit_position_to_end_point(const Vec* center, const Vec* endpoint,
                                float* initial_speed, float* final_speed,
                                unsigned int direction, int reset, float time) {
    static unsigned int num_ticks;
    static Vec swing_vector = {0.0f, 0.0f, 0.0f};
    static float desired_speed;
    static int initializing = 1;
    static float speed_increment;
    static float current_speed;
    static unsigned int blending_ticks;
    static float angle_rotated;
    static float radius_increment;
    static float height_increment;
    static float decelerate_angle;
    static float final_speed_increment;
    static int decelerating;
    static float initial_height;
    static float initial_radius;
    static float angle = 10.0f;
    static float final_radius;
    CameraObj* camera;
    Vec initial_angles;
    Vec final_angles;
    Vec final_vector;
    float dx;
    float dz;
    float radius_squared;
    float inverse_radius;
    float radius;
    float deceleration_sum = 0.0f;
    float angle_difference;
    float absolute_angle_difference;
    float absolute_desired_speed;
    float absolute_angle_rotated;
    float requested_final_speed;
    unsigned int tick;
    unsigned int ticks_remaining;

    RESOLVE_CAMERA_OBJ(camera);

    angle_difference = angle_rotated - angle;
    absolute_angle_difference =
        angle_difference >= kZero ? angle_difference : -angle_difference;
    absolute_desired_speed =
        desired_speed >= kZero ? desired_speed : -desired_speed;
    if ((decelerating != 0 && blending_ticks == 0) ||
        absolute_angle_difference < absolute_desired_speed ||
        reset != 0) {
        *final_speed = current_speed * final_radius;
        initializing = 1;
        angle_rotated = kZero;
        angle = 10.0f;
        decelerating = 0;
        return 1;
    }

    if (initializing != 0) {
        num_ticks = (unsigned int)(60.0f * time);
        dx = camera->pos.x - center->x;
        dz = camera->pos.z - center->z;
        radius_squared = dx * dx + kZero * kZero;
        radius_squared += dz * dz;
        initial_radius = radius_squared;
        initial_radius = camera_sqrt(radius_squared);
        inverse_radius = camera_inv_sqrt(radius_squared);
        swing_vector.x = dx * inverse_radius;
        swing_vector.y = kZero;
        swing_vector.z = dz * inverse_radius;
        v3_to_xy_ang(&initial_angles, &swing_vector);
        initial_height = camera->pos.y;

        final_vector.x = endpoint->x - center->x;
        final_vector.y = endpoint->y - center->y;
        final_vector.z = endpoint->z - center->z;
        final_vector.y = kZero;
        radius_squared = final_vector.x * final_vector.x +
                         final_vector.y * final_vector.y;
        radius_squared += final_vector.z * final_vector.z;
        final_radius = radius_squared;
        final_radius = camera_sqrt(radius_squared);
        inverse_radius = camera_inv_sqrt(radius_squared);
        final_vector.x *= inverse_radius;
        final_vector.y *= inverse_radius;
        final_vector.z *= inverse_radius;
        v3_to_xy_ang(&final_angles, &final_vector);

        angle = final_angles.y - initial_angles.y;
        if (angle < kZero) {
            if (direction == 1U) {
                angle += kTwoPi;
            }
        } else if (direction == 0U) {
            angle -= kTwoPi;
        }
        requested_final_speed = *final_speed;

        if (*initial_speed == kNegOne && requested_final_speed == kNegOne) {
            desired_speed = angle / (float)num_ticks;
        } else if (*initial_speed == kNegOne ||
                   requested_final_speed == kNegOne) {
            desired_speed = angle / (0.8f * (float)num_ticks);
        } else {
            desired_speed = angle / (0.6f * (float)num_ticks);
        }
        blending_ticks = (unsigned int)(0.2f * (float)num_ticks);

        if (*initial_speed == kNegOne) {
            current_speed = desired_speed;
            speed_increment = kZero;
        } else {
            current_speed = *initial_speed / initial_radius;
            speed_increment =
                (desired_speed - current_speed) / (float)blending_ticks;
        }

        if (requested_final_speed == kNegOne) {
            decelerate_angle = kTwoPi;
            final_speed_increment = kZero;
        } else {
            final_speed_increment =
                ((requested_final_speed / final_radius) - desired_speed) /
                (float)blending_ticks;
            tick = 0;
            ticks_remaining = blending_ticks;
            if (ticks_remaining != 0) {
                do {
                    deceleration_sum +=
                        final_speed_increment * (float)tick + desired_speed;
                    tick++;
                    ticks_remaining--;
                } while (ticks_remaining != 0);
            }
            decelerate_angle = angle - deceleration_sum;
            if (!(decelerate_angle >= kZero)) {
                decelerate_angle = -decelerate_angle;
            }
        }

        initializing = 0;
        height_increment = (endpoint->y - initial_height) / angle;
        radius_increment = (final_radius - initial_radius) / angle;
    }

    absolute_angle_rotated =
        angle_rotated >= kZero ? angle_rotated : -angle_rotated;
    if (absolute_angle_rotated >= decelerate_angle &&
        decelerating == 0) {
        decelerating = 1;
        blending_ticks = (unsigned int)(0.2f * (float)num_ticks);
        speed_increment = final_speed_increment;
    }

    if (blending_ticks != 0) {
        if (decelerating != 0) {
            desired_speed = current_speed;
        }
        blending_ticks--;
        current_speed += speed_increment;
    } else {
        current_speed = desired_speed;
    }

    angle_rotated += current_speed;
    radius = radius_increment * angle_rotated + initial_radius;
    inverse_radius = camera_inv_sqrt(
        swing_vector.x * swing_vector.x + swing_vector.y * swing_vector.y +
        swing_vector.z * swing_vector.z);
    swing_vector.x *= inverse_radius;
    swing_vector.y *= inverse_radius;
    swing_vector.z *= inverse_radius;
    swing_vector.x *= radius;
    swing_vector.y *= radius;
    swing_vector.z *= radius;
    rotate_xz(&swing_vector, &swing_vector, current_speed);
    camera->pos.x = center->x + swing_vector.x;
    camera->pos.y = center->y + swing_vector.y;
    camera->pos.z = center->z + swing_vector.z;
    camera->pos.y = height_increment * angle_rotated + initial_height;
    return 0;
}

int move_to_end_point(const Vec* endpoint, float* initial_speed,
                      float* final_speed, int reset, float time) {
    static unsigned int num_ticks;
    static float desired_speed;
    static int initializing = 1;
    static float speed_increment;
    static float current_speed;
    static unsigned int blending_ticks;
    static float decelerate_distance;
    static float final_speed_increment;
    static int decelerating;
    static float distance_traveled;
    static Vec unit_vector = {0.0f, 0.0f, 0.0f};
    CameraObj* camera;
    float dx;
    float dy;
    float dz;
    float distance_squared;
    float remaining_distance;
    float distance;
    float inverse_distance;
    float deceleration_distance_sum = 0.0f;
    float move_x;
    float move_y;
    float move_z;
    unsigned int tick;
    unsigned int ticks_remaining;

    RESOLVE_CAMERA_OBJ(camera);
    dy = endpoint->y - camera->pos.y;
    dx = endpoint->x - camera->pos.x;
    dz = endpoint->z - camera->pos.z;
    distance_squared = dz * dz + (dx * dx + dy * dy);
    remaining_distance = camera_sqrt(distance_squared);

    if ((decelerating != 0 && blending_ticks == 0) ||
        remaining_distance < desired_speed || reset != 0) {
        initializing = 1;
        desired_speed = kZero;
        decelerating = 0;
        *final_speed = current_speed;
        distance_traveled = kZero;
        return 1;
    }

    if (initializing != 0) {
        num_ticks = (unsigned int)(60.0f * time);
        distance = camera_sqrt(distance_squared);
        inverse_distance = camera_inv_sqrt(distance_squared);
        unit_vector.x = dx * inverse_distance;
        unit_vector.y = dy * inverse_distance;
        unit_vector.z = dz * inverse_distance;

        if (*initial_speed == kNegOne && *final_speed == kNegOne) {
            desired_speed = distance / (float)num_ticks;
        } else if (*initial_speed == kNegOne || *final_speed == kNegOne) {
            desired_speed = distance / (0.8f * (float)num_ticks);
        } else {
            desired_speed = distance / (0.6f * (float)num_ticks);
        }
        blending_ticks = (unsigned int)(0.2f * (float)num_ticks);

        if (*initial_speed == kNegOne) {
            current_speed = desired_speed;
            speed_increment = kZero;
        } else {
            current_speed = *initial_speed;
            speed_increment =
                (desired_speed - *initial_speed) / (float)blending_ticks;
        }

        if (*final_speed == kNegOne) {
            final_speed_increment = kZero;
            decelerate_distance = 2.0f * distance;
        } else {
            final_speed_increment =
                (*final_speed - desired_speed) / (float)blending_ticks;
            tick = 0;
            ticks_remaining = blending_ticks;
            if (ticks_remaining != 0) {
                do {
                    deceleration_distance_sum +=
                        final_speed_increment * (float)tick + desired_speed;
                    tick++;
                    ticks_remaining--;
                } while (ticks_remaining != 0);
            }
            distance -= deceleration_distance_sum;
            decelerate_distance = distance < kZero ? -distance : distance;
        }
        initializing = 0;
    }

    if (distance_traveled >= decelerate_distance && decelerating == 0) {
        decelerating = 1;
        blending_ticks = (unsigned int)(0.2f * (float)num_ticks);
        speed_increment = final_speed_increment;
    }
    if (blending_ticks != 0) {
        if (decelerating != 0) {
            desired_speed = current_speed;
        }
        blending_ticks--;
        current_speed += speed_increment;
    } else {
        current_speed = desired_speed;
    }
    move_x = unit_vector.x * current_speed;
    move_y = unit_vector.y * current_speed;
    move_z = unit_vector.z * current_speed;
    distance_traveled += current_speed;
    camera->pos.x += move_x;
    camera->pos.y += move_y;
    camera->pos.z += move_z;
    return 0;
}

AttractCameraSetup attract_cam_setup_table[4] = {
    {attract_setup_radial_sweep, attract_update_radial_sweep,
     attract_default_move, attract_default_glitch_move},
    {attract_setup_gamecam, 0, attract_move_gamecam,
     attract_glitch_move_gamecam},
    {attract_setup_chase_cam, attract_update_chase_cam,
     attract_default_move, attract_default_glitch_move},
    {attract_setup_flyby, attract_update_flyby, attract_move_flyby,
     attract_default_glitch_move},
};
Vec conversation_midpoint = {0.0f, 0.0f, 0.0f};
static Vec unit_z = {0.0f, 0.0f, 1.0f};
static Vec unit_x = {1.0f, 0.0f, 0.0f};

/* Soft ceiling: camera-latch peephole and inline invsqrt FPR scheduling. */
void look_at_target(const Vec* target) {
    CameraObj* camera;
    Vec direction;
    Vec angle;
    float inverse_length;

    RESOLVE_CAMERA_OBJ(camera);
    direction.x = target->x - camera->pos.x;
    direction.y = target->y - camera->pos.y;
    direction.z = target->z - camera->pos.z;
    inverse_length = camera_inv_sqrt(direction.x * direction.x +
                                     direction.y * direction.y +
                                     direction.z * direction.z);
    direction.x *= inverse_length;
    direction.y *= inverse_length;
    direction.z *= inverse_length;
    v3_to_xy_ang_high_freq(&angle, &direction);
    camera->ang.x = angle.x;
    camera->ang.y = angle.y;
    camera->ang.z = angle.z;
}

void set_camera_focal_length(float focal_length) {
    float view_window;

    view_window = 20.7f / (2.0f * focal_length);
    cam_fov = 114.59156f * gxMathArcTan(view_window);
    CameraSize(Camera, 0, view_window, DEFAULT_ASPECTRATIO);
}

void set_camera_velocity(const CamVec3* velocity) {
    CameraObj* camera;

    RESOLVE_CAMERA_OBJ(camera);
    camera->flags.bit20 = 1;
    camera->velocity.x = velocity->x;
    camera->velocity.y = velocity->y;
    camera->velocity.z = velocity->z;
}

void get_camera_velocity(CamVec3* velocity) {
    CameraObj* camera;

    RESOLVE_CAMERA_OBJ(camera);
    velocity->x = camera->velocity.x;
    velocity->y = camera->velocity.y;
    velocity->z = camera->velocity.z;
}

/* Soft ceiling: resolve peephole; typed CameraObj ~88% (was ~94% with (char*)+4). */
void get_camera_angle(CamVec3* ang) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    ang->x = cam->ang.x;
    ang->y = cam->ang.y;
    ang->z = cam->ang.z;
}

void get_camera_position(CamVec3* pos) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    pos->x = cam->pos.x;
    pos->y = cam->pos.y;
    pos->z = cam->pos.z;
}

void set_camera_angle(const CamVec3* ang) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    cam->ang.x = ang->x;
    cam->ang.y = ang->y;
    cam->ang.z = ang->z;
}

void set_camera_position(const CamVec3* pos) {
    CameraObj* cam;

    RESOLVE_CAMERA_OBJ(cam);
    cam->pos.x = pos->x;
    cam->pos.y = pos->y;
    cam->pos.z = pos->z;
}

static float p_shake_camera_y(void);
static float p_shake_camera(void);

void shake_camera_y(int count, float strength) {
    MkHdr* pdata;

    if (_create_mkproc_generic_tinystack(0x1007, 0x1E, p_shake_camera_y,
                                         sizeof(CameraShakePdata), &pdata) != 0) {
        ((CameraShakePdata*)pdata)->count = count;
        ((CameraShakePdata*)pdata)->strength = strength;
    }
}

static float p_shake_camera_y(void) {
    int index;
    CameraShakePdata* shake = (CameraShakePdata*)apdata;

    for (index = 0; index < shake->count; index++) {
        cam_ang_offset.y = shake->strength;
        _mkproc_sleep_ticks = kThree;
        mkproc_sleep();
        cam_ang_offset.y = kZero;
        _mkproc_sleep_ticks = kThree;
        mkproc_sleep();
    }
    return kNegOne;
}

void shake_camera(int count, void* script_args, float strength) {
    MkHdr* pdata;

    if (_create_mkproc_generic_tinystack(0x1007, 0x1E, p_shake_camera,
                                         sizeof(CameraShakePdata), &pdata) != 0) {
        ((CameraShakePdata*)pdata)->count = count;
        ((CameraShakePdata*)pdata)->strength = strength;
    }
}

static float p_shake_camera(void) {
    int index;
    CameraShakePdata* shake = (CameraShakePdata*)apdata;

    for (index = 0; index < shake->count; index++) {
        cam_ang_offset.x = shake->strength;
        _mkproc_sleep_ticks = kThree;
        mkproc_sleep();
        cam_ang_offset.x = kZero;
        _mkproc_sleep_ticks = kThree;
        mkproc_sleep();
    }
    return kNegOne;
}

void CameraDestroy(RwCamera* camera) {
    {
        CameraObj* object;

        RESOLVE_CAMERA_OBJ(object);
        if (object != 0) {
            if (camera_item.node->hdr.instance != 0) {
                camera_item.node->hdr.typed_vtbl->destroy(&camera_item.node->hdr);
            }
            camera_item.node = 0;
            camera_item.instance = 0;
        }
    }
    camera_obj = 0;

    if (camera != 0) {
        RpWorld* world;
        RwFrame* frame;
        RwRaster* raster;
        RwRaster* parent;

        world = RwCameraGetWorld(camera);
        if (world != 0) {
            RpWorldRemoveCamera(world, camera);
        }
        frame = RwCameraGetFrame(camera);
        if (frame != 0) {
            _rwObjectHasFrameSetFrame(camera, 0);
            RwFrameDestroy(frame);
        }
        raster = camera->frameBuffer;
        if (raster != 0) {
            parent = raster->parent;
            RwRasterDestroy(raster);
            if (parent != 0 && parent != raster) {
                RwRasterDestroy(parent);
            }
            camera->frameBuffer = 0;
        }
        raster = camera->zBuffer;
        if (raster != 0) {
            parent = raster->parent;
            RwRasterDestroy(raster);
            if (parent != 0 && parent != raster) {
                RwRasterDestroy(parent);
            }
            camera->zBuffer = 0;
        }
        RwCameraDestroy(camera);
    }
}

/* Soft ceiling: CameraSize ~99.91% -- pooled int-to-double relocation label only. */
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

void vdestroy_mkpdata_camera(MkHdr* pdata) {
    pdata->instance = 0;
    mkhdr_memfree(pdata);
}

void camera_set_speed_scalar(float speed) {
    camera_info.pdata->speed = speed;
}

static inline int player_is_stationary_impl(PlyrPdata* player) {
    MkProc* animation_proc;
    AnimPdata* animation;

    if (player->state == 0x4200) {
        return 1;
    }
    animation_proc = player->anim_proc;
    if (animation_proc != 0) {
        if (animation_proc->instance == player->anim_proc_instance) {
            /* keep the validated animation process */
        } else {
            animation_proc = 0;
        }
    } else {
        animation_proc = 0;
    }
    if (animation_proc == 0) {
        return 0;
    }
    animation = (AnimPdata*)pdata_of_proc(animation_proc);
    if (animation == 0) {
        return 0;
    }
    if (animation->script_word ==
        player->global_moveset->standing_animation_script) {
        return 1;
    }
    return 0;
}

void adj_cam_pos(void) {
    CameraObj* camera;
    Vec delta;
    Vec candidate;
    float player_distance;
    float half_span;
    float desired_distance;
    float distance_delta;
    float rate;
    float pitch_delta;
    float yaw_delta;
    float roll_delta;
    float t;
    float current_distance_sq;
    float inverse_length;

    RESOLVE_CAMERA_OBJ(camera);
    camera_midpoint.y += camera->ground_plane;
    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) {
        return;
    }
    if (tightrope_uv.x * tightrope_uv.x +
            tightrope_uv.y * tightrope_uv.y +
            tightrope_uv.z * tightrope_uv.z ==
        kZero) {
        return;
    }
    if (camera_mode != 1) {
        return;
    }

    player_distance = get_constrain_player_distance();
    if (force_midpoint_calculation_update != 0 ||
        !player_is_stationary_impl(g_game_info.plyr0.slot.pdata) ||
        !player_is_stationary_impl(g_game_info.plyr1.slot.pdata)) {
        if (player_distance > 0.1225f) {
            force_midpoint_calculation_update = 0;
            half_span = player_distance * kHalf;
            half_span += g_game_info.plyr0.slot.pdata->global_moveset
                             ->definition->camera_distance_offset;
            half_span += g_game_info.plyr1.slot.pdata->global_moveset
                             ->definition->camera_distance_offset;

            if (g_game_info.plyr0.slot.mirror_a != 0 &&
                g_game_info.plyr1.slot.mirror_a != 0) {
                camera_midpoint.x =
                    g_game_info.plyr1.slot.mirror_a->pos.value.x +
                    g_game_info.plyr0.slot.mirror_a->pos.value.x;
                camera_midpoint.y =
                    g_game_info.plyr1.slot.mirror_a->pos.value.y +
                    g_game_info.plyr0.slot.mirror_a->pos.value.y;
                camera_midpoint.z =
                    g_game_info.plyr1.slot.mirror_a->pos.value.z +
                    g_game_info.plyr0.slot.mirror_a->pos.value.z;
                camera_midpoint.x *= kHalf;
                camera_midpoint.y *= kHalf;
                camera_midpoint.z *= kHalf;
            }

            cam_unit_vector.x = -tightrope_perp_uv.x;
            cam_unit_vector.y = -tightrope_perp_uv.y;
            cam_unit_vector.z = -tightrope_perp_uv.z;
            v3_to_xy_ang(&camera_info.pdata->target_ang, &cam_unit_vector);
            camera_info.pdata->target_ang.y += kPi;

            desired_distance =
                1.03f * (0.75f + half_span) * field_of_view_ratio;
            if (!(desired_distance >= 2.6f)) {
                desired_distance = 2.6f;
            }
            if (!(desired_distance <= 7.5f)) {
                desired_distance = 7.5f;
            }
            if (desired_distance < 3.5f &&
                last_camera_distance < desired_distance) {
                desired_distance = 2.6f;
            }
            distance_delta = desired_distance - last_camera_distance;
            if (!(distance_delta >= kZero)) {
                distance_delta = -distance_delta;
            }
            if (distance_delta < 0.1f) {
                desired_distance = last_camera_distance;
            }
            last_camera_distance = desired_distance;

            midpoint_to_cam_vector.x = cam_unit_vector.x * desired_distance;
            midpoint_to_cam_vector.y = cam_unit_vector.y * desired_distance;
            midpoint_to_cam_vector.z = cam_unit_vector.z * desired_distance;
            camera_info.pdata->target_pos.x =
                camera_midpoint.x + midpoint_to_cam_vector.x;
            camera_info.pdata->target_pos.y =
                camera_midpoint.y + midpoint_to_cam_vector.y;
            camera_info.pdata->target_pos.z =
                camera_midpoint.z + midpoint_to_cam_vector.z;

            if (desired_distance < 3.9f) {
                camera_info.pdata->target_ang.x = 0.1f;
                camera_info.pdata->target_pos.y =
                    1.55f + camera->ground_plane;
            } else {
                t = (desired_distance - 3.9f) / 3.6000001f;
                camera_info.pdata->target_ang.x =
                    0.1f * (3.0f * t + kOne);
                camera_info.pdata->target_pos.y =
                    1.55f * (1.5f * t + kOne) + camera->ground_plane;
            }

            delta.x = camera_info.pdata->target_pos.x - camera->pos.x;
            delta.y = camera_info.pdata->target_pos.y - camera->pos.y;
            delta.z = camera_info.pdata->target_pos.z - camera->pos.z;
            current_distance_sq = xz_dot_xz(&delta, &delta);
            candidate.x = midpoint_to_cam_vector.x;
            candidate.y = midpoint_to_cam_vector.y;
            candidate.z = midpoint_to_cam_vector.z;
            candidate.x *= kNegOne;
            candidate.y *= kNegOne;
            candidate.z *= kNegOne;
            candidate.x += camera_midpoint.x;
            candidate.y += camera_midpoint.y;
            candidate.z += camera_midpoint.z;
            candidate.y = camera_info.pdata->target_pos.y;
            delta.x = candidate.x - camera->pos.x;
            delta.y = candidate.y - camera->pos.y;
            delta.z = candidate.z - camera->pos.z;
            if (xz_dot_xz(&delta, &delta) < current_distance_sq) {
                camera_info.pdata->target_pos.x = candidate.x;
                camera_info.pdata->target_pos.y = candidate.y;
                camera_info.pdata->target_pos.z = candidate.z;
                camera_info.pdata->target_ang.y += kPi;
            }
        }
    }

    if (keep_camera_out_of_danger_zones(&camera_info.pdata->target_pos)) {
        midpoint_to_cam_vector.x =
            camera_info.pdata->target_pos.x - camera_midpoint.x;
        midpoint_to_cam_vector.y =
            camera_info.pdata->target_pos.y - camera_midpoint.y;
        midpoint_to_cam_vector.z =
            camera_info.pdata->target_pos.z - camera_midpoint.z;
        inverse_length = camera_inv_sqrt(
            midpoint_to_cam_vector.x * midpoint_to_cam_vector.x +
            midpoint_to_cam_vector.y * midpoint_to_cam_vector.y +
            midpoint_to_cam_vector.z * midpoint_to_cam_vector.z);
        midpoint_to_cam_vector.x *= inverse_length;
        midpoint_to_cam_vector.y *= inverse_length;
        midpoint_to_cam_vector.z *= inverse_length;
        v3_to_xy_ang_high_freq(&camera_info.pdata->target_ang,
                               &midpoint_to_cam_vector);
        camera_info.pdata->target_ang.y += kPi;
    }

    rate = 0.1f / (last_camera_distance / 2.6f);
    RESOLVE_CAMERA_OBJ(camera);
    camera->pos.x -= old_cam_pos_offset.x;
    camera->pos.y -= old_cam_pos_offset.y;
    camera->pos.z -= old_cam_pos_offset.z;
    camera->ang.x -= old_cam_ang_offset.x;
    camera->ang.y -= old_cam_ang_offset.y;
    camera->ang.z -= old_cam_ang_offset.z;
    radial_move_to_game_position(&camera_info.pdata->target_pos,
                                 &camera_midpoint, rate);

    RESOLVE_CAMERA_OBJ(camera);
    pitch_delta = camera_info.pdata->target_ang.x - camera->ang.x;
    if (pitch_delta > kPi) {
        pitch_delta -= kTwoPi;
    } else if (pitch_delta < kNegPi) {
        pitch_delta += kTwoPi;
    }
    yaw_delta = camera_info.pdata->target_ang.y - camera->ang.y;
    if (yaw_delta > kPi) {
        yaw_delta -= kTwoPi;
    } else if (yaw_delta < kNegPi) {
        yaw_delta += kTwoPi;
    }
    if (!(pitch_delta * pitch_delta + yaw_delta * yaw_delta < kAngEpsSq)) {
        pitch_delta *= rate;
        yaw_delta *= rate;
        roll_delta = kZero * rate;
        camera->ang.x += pitch_delta;
        camera->ang.y += yaw_delta;
        camera->ang.z += roll_delta;
    }

    RESOLVE_CAMERA_OBJ(camera);
    old_cam_ang_offset.x = cam_ang_offset.x;
    old_cam_ang_offset.y = cam_ang_offset.y;
    old_cam_ang_offset.z = cam_ang_offset.z;
    old_cam_pos_offset.x = cam_pos_offset.x;
    old_cam_pos_offset.y = cam_pos_offset.y;
    old_cam_pos_offset.z = cam_pos_offset.z;
    camera->pos.x += cam_pos_offset.x;
    camera->pos.y += cam_pos_offset.y;
    camera->pos.z += cam_pos_offset.z;
    camera->ang.x += cam_ang_offset.x;
    camera->ang.y += cam_ang_offset.y;
    camera->ang.z += cam_ang_offset.z;
}

int player_is_stationary(PlyrPdata* player) {
    return player_is_stationary_impl(player);
}

/* Soft ceiling: exact size; inlined camera-latch branch and GPR coloring only. */
void go_to_camera_cut_with_angle(const CamVec3* position,
                                 const CamVec3* angle) {
    if (camera_info.proc != 0) {
        xfer_proc(camera_info.proc, p_hold_camera_in_place);
    }
    set_camera_position(position);
    set_camera_angle(angle);
}

/* Soft ceiling: exact size; inline invsqrt stack/FPR and latch scheduling only. */
void go_to_camera_cut(const CamVec3* position, const Vec* target) {
    if (camera_info.proc != 0) {
        xfer_proc(camera_info.proc, p_hold_camera_in_place);
    }
    set_camera_position(position);
    look_at_target(target);
}

/* Soft ceiling: inlined paired latch CSE and contiguous-offset coloring only. */
float p_hold_camera_in_place(void) {
    remove_camera_offsets_impl();
    add_camera_offsets_impl();
    return kOne;
}

void remove_camera_offsets(void) {
    remove_camera_offsets_impl();
}

void add_camera_offsets(void) {
    add_camera_offsets_impl();
}

#pragma fp_contract off
void get_current_target(Vec* target) {
    CameraObj* camera;
    float x_offset;
    float y_offset;
    float z_offset;

    RESOLVE_CAMERA_OBJ(camera);
    if (camera != 0) {
        target->x = camera->pos.x;
        target->y = camera->pos.y;
        target->z = camera->pos.z;
        x_offset = 2.0f * cam_forward_uv.x;
        y_offset = 2.0f * cam_forward_uv.y;
        z_offset = 2.0f * cam_forward_uv.z;
        target->x += x_offset;
        target->y += y_offset;
        target->z += z_offset;
    }
}
#pragma fp_contract reset

/* Soft ceiling: typed MkObj aliasing retains coordinates across moveset loads. */
void get_play_camera_position(Vec* position) {
    CameraObj* camera;
    Vec camera_direction = {0.0f, 0.0f, 0.0f};
    float midpoint_x;
    float midpoint_y;
    float midpoint_z;
    float dx;
    float dy;
    float dz;
    float distance_squared;
    float player_distance;
    float half_span;
    float inverse_distance;
    float camera_distance;
    float height_scale;

    RESOLVE_CAMERA_OBJ(camera);
    dy = g_game_info.plyr0.slot.mirror_a->pos.value.y -
         g_game_info.plyr1.slot.mirror_a->pos.value.y;
    dx = g_game_info.plyr0.slot.mirror_a->pos.value.x -
         g_game_info.plyr1.slot.mirror_a->pos.value.x;
    dz = g_game_info.plyr0.slot.mirror_a->pos.value.z -
         g_game_info.plyr1.slot.mirror_a->pos.value.z;
    distance_squared = dx * dx + dy * dy + dz * dz;
    player_distance = camera_sqrt(distance_squared);
    half_span = player_distance * kHalf;
    half_span += g_game_info.plyr0.slot.pdata->global_moveset->definition
                     ->camera_distance_offset;
    half_span += g_game_info.plyr1.slot.pdata->global_moveset->definition
                     ->camera_distance_offset;

    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        midpoint_x =
            kHalf * (g_game_info.plyr1.slot.mirror_a->pos.value.x +
                     g_game_info.plyr0.slot.mirror_a->pos.value.x);
        midpoint_y =
            kHalf * (g_game_info.plyr1.slot.mirror_a->pos.value.y +
                     g_game_info.plyr0.slot.mirror_a->pos.value.y);
        midpoint_z =
            kHalf * (g_game_info.plyr1.slot.mirror_a->pos.value.z +
                     g_game_info.plyr0.slot.mirror_a->pos.value.z);
    }

    inverse_distance = camera_inv_sqrt(distance_squared);
    camera_direction.x = dz * inverse_distance;
    camera_direction.z = -(dx * inverse_distance);
    camera_distance =
        1.03f * (0.75f + half_span) * field_of_view_ratio;
    if (!(camera_distance >= 2.6f)) {
        camera_distance = 2.6f;
    }
    if (!(camera_distance <= 7.5f)) {
        camera_distance = 7.5f;
    }

    position->x = midpoint_x + camera_direction.x * camera_distance;
    position->y = midpoint_y + camera_direction.y * camera_distance;
    position->z = midpoint_z + camera_direction.z * camera_distance;
    if (camera_distance < 3.9f) {
        position->y = 1.55f + camera->ground_plane;
    } else {
        height_scale = (camera_distance - 3.9f) / 3.6f;
        position->y =
            1.55f * (1.0f + 1.5f * height_scale) + camera->ground_plane;
    }
}

void cam_set_ground_plane(float ground_plane) {
    CameraObj* camera;

    RESOLVE_CAMERA_OBJ(camera);
    if (camera != 0) {
        camera->ground_plane = ground_plane;
    }
}

float p_camera_proc(void) {
    switch (get_game_state()) {
    case 0:
    case 3:
    case 4:
    case 9:
        return kOne;
    case 7:
    case 15:
    case 18:
        if (!g_game_info.flag_bits.high_res_path) {
            adj_cam_pos();
        }
        break;
    default:
        break;
    }
    return kOne;
}

/* Soft ceiling: frustum cross-product scheduling and folded global labels only. */
static void prewake_camera(void) {
    RwFrame* frame;
    float dot;

    frame = RwCameraGetFrame(Camera);
    cam_right_uv.x = frame->modelling.right.x;
    cam_right_uv.y = frame->modelling.right.y;
    cam_right_uv.z = frame->modelling.right.z;
    cam_forward_uv.x = frame->modelling.at.x;
    cam_forward_uv.y = frame->modelling.at.y;
    cam_forward_uv.z = frame->modelling.at.z;
    cam_up_uv.x = frame->modelling.up.x;
    cam_up_uv.y = frame->modelling.up.y;
    cam_up_uv.z = frame->modelling.up.z;

    left_frustum_plane_normal.x =
        Camera->frustumPlanes[4].plane.normal.x;
    left_frustum_plane_normal.y =
        Camera->frustumPlanes[4].plane.normal.y;
    left_frustum_plane_normal.z =
        Camera->frustumPlanes[4].plane.normal.z;
    left_frustum_vector.x =
        Camera->frustumPlanes[4].plane.normal.y * cam_up_uv.z -
        Camera->frustumPlanes[4].plane.normal.z * cam_up_uv.y;
    left_frustum_vector.y =
        Camera->frustumPlanes[4].plane.normal.z * cam_up_uv.x -
        Camera->frustumPlanes[4].plane.normal.x * cam_up_uv.z;
    left_frustum_vector.z =
        Camera->frustumPlanes[4].plane.normal.x * cam_up_uv.y -
        Camera->frustumPlanes[4].plane.normal.y * cam_up_uv.x;
    left_frustum_plane_dist = Camera->frustumPlanes[4].plane.distance;

    right_frustum_plane_normal.x =
        Camera->frustumPlanes[2].plane.normal.x;
    right_frustum_plane_normal.y =
        Camera->frustumPlanes[2].plane.normal.y;
    right_frustum_plane_normal.z =
        Camera->frustumPlanes[2].plane.normal.z;
    right_frustum_vector.x =
        cam_up_uv.y * Camera->frustumPlanes[2].plane.normal.z -
        cam_up_uv.z * Camera->frustumPlanes[2].plane.normal.y;
    right_frustum_vector.y =
        cam_up_uv.z * Camera->frustumPlanes[2].plane.normal.x -
        cam_up_uv.x * Camera->frustumPlanes[2].plane.normal.z;
    right_frustum_vector.z =
        cam_up_uv.x * Camera->frustumPlanes[2].plane.normal.y -
        cam_up_uv.y * Camera->frustumPlanes[2].plane.normal.x;
    right_frustum_plane_dist = Camera->frustumPlanes[2].plane.distance;

    dot = left_frustum_vector.x * right_frustum_vector.x +
          left_frustum_vector.y * right_frustum_vector.y +
          left_frustum_vector.z * right_frustum_vector.z;
    if (dot < kNegOne) {
        dot = kNegOne;
    }
    if (dot > kOne) {
        dot = kOne;
    }
    cam_fov = gxMathArcCos(dot);
    field_of_view_ratio = kOne / gxMathTan(cam_fov * kHalf);
}

void camera_idle(void) {
    xfer_camera_impl(p_idle_camera, 1);
}

float p_idle_camera(void) {
    while (1) {
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
    }
}

/* Soft ceiling: xfer_camera ~99.78% -- pooled float relocation labels only. */
void xfer_camera(MkProcEntryFn entry, int reset_projection) {
    xfer_camera_impl(entry, reset_projection);
}

int init_camera(void) {
    union {
        int word;
        struct {
            unsigned char scheduling;
            unsigned char pad[3];
        };
    } flags;
    RwCamera* camera;
    RwFrame* frame;
    MkProc* process;
    CameraPdata* pdata;
    int width;
    int height;
    int proc_flags;

    memset(&camera_info, 0, sizeof(camera_info));
    camera_speed = 0.05f;
    cam_rot_speed = 0.05f;
    if (Camera != 0) {
        return 0;
    }

    height = screen_height;
    width = screen_width;
    camera = RwCameraCreate();
    if (camera != 0) {
        frame = RwFrameCreate();
        if (frame != 0) {
            _rwObjectHasFrameSetFrame(camera, frame);
        }
        camera->frameBuffer = RwRasterCreate(width, height, 0, 2);
        camera->zBuffer = RwRasterCreate(width, height, 0, 1);
        if (camera->object.object.parent == 0 ||
            camera->frameBuffer == 0 || camera->frameBuffer->parent == 0 ||
            camera->zBuffer == 0 || camera->zBuffer->parent == 0) {
            CameraDestroy(camera);
            camera = 0;
        }
    } else {
        CameraDestroy(camera);
    }
    Camera = camera;
    if (camera == 0) {
        debug_error_message("Cannot create camera.");
        return 0;
    }

    RwCameraSetNearClipPlane(camera, 0.25f);
    RwCameraSetFarClipPlane(Camera, 125.0f);
    if (Camera != 0) {
        CameraSize(Camera, 0,
                   gxMathTan((kPi * DEFAULT_FIELD_OF_VIEW) /
                             kDegPerTurn),
                   DEFAULT_ASPECTRATIO);
        RwCameraSetProjection(Camera, 1);
    }

    if (camera_obj != 0) {
        camera_obj->flags.bit04 = 1;
        camera_obj->flags.bit20 = 1;
    }
    camera_obj =
        (CameraObj*)get_mkobj_frame(0x1003, RwCameraGetFrame(Camera));
    if (camera_obj != 0) {
        camera_item.node = camera_obj;
        camera_item.instance = camera_obj->hdr.instance;
        camera_mat = camera_obj->field_24;
        insert_fgnd_mkobj(camera_obj);
        camera_obj->flags.bit04 = 1;
        camera_obj->flags.bit20 = 1;
        update_mkobj(camera_obj);
    }

    flags.word = 0;
    flags.scheduling |= 0x40;
    proc_flags = flags.word;
    process = get_mkproc_bigstack(&proc_flags);
    pdata = (CameraPdata*)get_mkhdr(&vtbl_mkpdata_camera,
                                    sizeof(CameraPdata));
    camera_info.pdata = pdata;
    camera_info.proc = create_mkproc(0x1D, process, 0x5005,
                                     p_camera_proc, &pdata->hdr);
    camera_info.proc->pre_destroy = prewake_camera;
    pdata->camera = Camera;
    zero_pdata_payload(sizeof(CameraPdata), &pdata->hdr);
    camera_mode = 1;
    camera_script_monitor_item.node = 0;
    camera_script_monitor_item.instance = 0;
    return 1;
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
