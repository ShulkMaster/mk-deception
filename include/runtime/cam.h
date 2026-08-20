#ifndef RUNTIME_CAM_H
#define RUNTIME_CAM_H

#include "math/gxVect.h"
#include "rw/rtquat.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/anim_types.h"

typedef struct MkSobj MkSobj;
typedef struct MkObj MkObj;
typedef struct RwFrame RwFrame;
typedef struct RwCamera RwCamera;
typedef struct CameraAnimEvent CameraAnimEvent;

typedef struct VictoryCameraConfig {
    float radius;
    float side_angle;
    float forward_offset;
    float camera_angle;
    float look_height;
    int rotation_ticks;
    float travel_time;
} VictoryCameraConfig;

typedef Vec CamVec3;

typedef struct CameraObjFlags {
    unsigned char pad7 : 1;
    unsigned char pad6 : 1;
    unsigned char bit20 : 1;
    unsigned char pad4 : 1;
    unsigned char pad3 : 1;
    unsigned char bit04 : 1;
    unsigned char pad1 : 1;
    unsigned char pad0 : 1;
} CameraObjFlags;

/*
 * Camera mkobj body (cam / display / bgnd_nbc agree):
 * instance @ +0x04, flags @ +0x08, position @ +0xA0, angles @ +0xD0.
 *
 * Retail: p_main_menu calls turn_camera_on() (Matching 100%):
 *   if Camera: RpWorldAddCamera(World, Camera) when unbound; turn_display_on().
 * Optional 3D after menu 2D: add_clump_to_world(World, clump) then Render()
 *   [display.h; soft ceiling ~73%]. Fighter/skin/MatFX Matching out of scope.
 */
typedef struct CameraObj {
    union {
        MkHdr hdr;
        struct {
            MkVtable5* vtbl;
            unsigned int instance; /* +0x04 - matched to CameraItem.instance */
        };
    };
    union {
        unsigned int flags_word;
        struct {
            union {
                unsigned char flags;
                CameraObjFlags flags_bits;
            }; /* +0x08 */
            unsigned char pad09[3];
        };
    };
    char pad0C[0x14];
    union {
        RwFrame* frame;
        RwMatrix* matrix;
    }; /* +0x20 - frame or transform view, both evidenced by callers */
    RwMatrix* field_24; /* +0x24 - active camera transform */
    MkPtr* child_list; /* +0x28 */
    char pad2C[0x44];
    float ground_plane; /* +0x70 */
    char pad74[0x2C];
    union {
        Vec pos; /* +0xA0 */
        struct {
            float pos_x;
            float pos_y;
            float pos_z;
        };
    };
    char padAC[4];
    Vec velocity; /* +0xB0 */
    char padBC[0x14];
    union {
        Vec ang; /* +0xD0 */
        struct {
            float ang_x;
            float ang_y;
            float ang_z;
        };
    };
    char padDC[4];
    Vec ang_velocity; /* +0xE0 */
} CameraObj;

/* Validated camera_item latch: node live iff node && node->instance == instance. */
typedef struct CameraItem {
    CameraObj* node;         /* +0x00 */
    unsigned int instance;   /* +0x04 */
} CameraItem;

typedef struct CameraPdataFlags {
    unsigned char pos_done : 1;
    unsigned char konquest_mode : 1;
    unsigned char animation_mirror : 1;
    unsigned char pad4_3 : 2;
    unsigned char parent_relative : 1;
    unsigned char pad1_0 : 2;
} CameraPdataFlags;

/*
 * camera_info.pdata: targets @ 0x0C/0x18, speed @ 0x50, flags @ 0x6C,
 * pause @ 0x70, parent/mirror mats @ 0x80/0xC0.
 */
typedef struct CameraPdata {
    MkHdr hdr;
    RwCamera* camera; /* +0x08 */
    union {
        Vec target_pos; /* +0x0C */
        struct {
            float target_pos_x;
            float target_pos_y;
            float target_pos_z;
        };
    };
    union {
        Vec target_ang; /* +0x18 */
        struct {
            float target_ang_x;
            float target_ang_y;
            float target_ang_z;
        };
    };
    char pad24[0x18];
    MkObj* movement_focus; /* +0x3C */
    MkObj* attacker; /* +0x40 */
    MkObj* victim;   /* +0x44 */
    char pad48[4];
    VictoryCameraConfig* victory_camera_config; /* +0x4C */
    float speed; /* +0x50 */
    AnimPdata* anim_pdata;
    unsigned int anim_instance;
    MkObj* bone_obj;
    unsigned int bone_instance;
    AniData* anim_path;
    struct CameraAnimEvent* aux_data;
    union {
        unsigned char flags; /* +0x6C */
        CameraPdataFlags flags_bits;
    };
    char pad_6d[3];
    float pause_ticks; /* +0x70 */
    void* intro_path; /* +0x74 */
    char pad_78[8];
    RwMatrix mat_parent; /* +0x80 */
    RwMatrix mat_mirror; /* +0xC0 */
    RwMatrix mat_offset; /* +0x100 - per-frame camera offsets */
} CameraPdata;

typedef struct CameraInfo {
    MkProc* proc;
    CameraPdata* pdata;
} CameraInfo;

/* Krypt / setup camera path */
float p_krypt_camera_loop(void);
float p_krypt_camera_proc(void);
float p_konquest_camera_proc(void);
void set_camera_angle(const CamVec3* ang);
void set_camera_position(const CamVec3* pos);
void get_camera_angle(CamVec3* ang);
void get_camera_position(CamVec3* pos);
void xfer_camera(MkProcEntryFn entry, int reset_projection);
void turn_camera_on(void);
void turn_camera_off(void);
void camera_idle(void);
void set_camera_destination(const CamVec3* position);
void set_camera_target_angle(const CamVec3* angle);
void set_camera_velocity(const CamVec3* velocity);
void get_camera_velocity(CamVec3* velocity);
void look_at_target(const Vec* target);
void go_to_camera_cut_with_angle(const CamVec3* position,
                                 const CamVec3* angle);
void go_to_camera_cut(const CamVec3* position, const Vec* target);
void add_widescreen_bars(float height);
float p_hold_camera_in_place(void);
void remove_camera_offsets(void);
void add_camera_offsets(void);
int init_camera(void);
void skip_camera_intro(void);
void do_victory_camera(VictoryCameraConfig* config);
void camera_exit_script(void);
float get_pan_value(const Vec* position);
void get_target_movement_vector(const Vec* current_position,
                                const Vec* target_position, Vec* movement,
                                float duration);
void interaction_cam_set_target_info(int duration, float angle_a,
                                     float field_14, float field_18,
                                     float angle_b, float field_20,
                                     float field_24);
void special_move_cam_setup2(int ease_ticks, int total_ticks, int unused,
                             MkObj* target, MkObj* reference_object,
                             float orbit_yaw_offset, float orbit_radius,
                             float camera_height, float look_yaw_offset,
                             float look_pitch);
void special_move_cam_setup(int ease_ticks, int total_ticks, int unused,
                            float orbit_yaw_offset, float orbit_radius,
                            float camera_height, float look_yaw_offset,
                            float look_pitch);
void cam_set_ground_plane(float ground_plane);
float camera_get_pos(unsigned int axis);
float camera_wait_for_pos_and_ang_move_done(void);
float camera_wait_for_ang_move_done(void);
float camera_wait_for_pos_move_done(void);
MkProc* get_camera_proc(void);
CameraPdata* get_pdata_of_camera(void);

/* Intro / background anim path used by p_setup_krypt */
void cam_set_intro_cam_speed(float speed);
void cam_set_intro_cam_pause_ticks(float ticks);
void camera_init_animation(AniData* anim_path, MkProcEntryFn override_entry);
void camera_run_animation(int wait_flag);
void camera_wait_for_animation_completion(void);
void camera_run_animation_start_end(float start_frame, float end_frame,
                                    int wait_flag, int use_frame_range);
void camera_set_anim_aux_data(CameraAnimEvent* data);
void camera_set_animation_mirror_plane(int mode);
void camera_set_animation_parent_position(const CamVec3* position);
void camera_set_animation_parent_angle(const CamVec3* angle, int relative);
void* get_intro_camera_path(void);
void set_intro_camera_path(void* path);
MkObj* camera_get_victim(void);
void camera_set_victim(MkObj* object);
MkObj* camera_get_attacker(void);
void camera_set_attacker(MkObj* object);
void camera_unpause_player(void);
void camera_pause_player(void);
void camera_set_lookat_focus(MkObj* object);
void camera_set_movement_focus_obj(MkObj* object);
void camera_check_reverse_move_offset(int expected_mask, int skip_mask);
int camera_get_mirror_flag(void);
void hide_sobj_if_camera_is_in_rectangle(MkSobj* object, const Vec* center, float min_x,
                                         float min_z, float max_x,
                                         float max_z);
void hide_sobj_if_camera_is_in_cylinder(MkSobj* object, const Vec* center, float radius,
                                        float height);
void turn_off_sobj_if_camera_is_in_rectangle(MkSobj* object, const Vec* center,
                                             float min_x, float min_z,
                                             float max_x, float max_z);
void turn_off_sobj_if_camera_is_in_cylinder(MkSobj* object, const Vec* center,
                                            float radius, float height);
float get_volume_from_distance(const Vec* position, float far_distance,
                               float near_distance);
void camera_set_speed_scalar(float speed);
void vdestroy_mkpdata_camera(MkHdr* pdata);
void camera_set_rotation_rate(float rate);
void camera_set_movement_rate(float rate);
void camera_set_check_konquest_collisions_flag(int enabled);
void camera_set_glitch_flag(void);
void camera_set_lookat_offset_explicit(float x, float y, float z);
void camera_set_lookat_offset(Vec* offset, void* script_args);
void camera_set_lookat_offset_obj_rel(const Vec* offset, void* script_args);
void camera_set_custom_camera_movement_flag(int enabled);
void camera_set_radial_movement(int enabled);
void camera_set_center_of_rotation(const CamVec3* center);
void camera_set_travel_time(float time);
void camera_set_rotation_direction(int direction);
void camera_set_final_speed(float speed);
void camera_set_initial_speed(float speed);
void camera_set_movement_offset_explicit(float x, float y, float z);
void camera_set_movement_offset(Vec* offset, void* script_args);
void camera_set_movement_offset_obj_rel(const Vec* offset, void* script_args);
void camera_set_look_mode(int mode);
void camera_set_movement_mode(int mode);
int camera_is_ang_move_done(void);
int camera_is_pos_move_done(void);
void camera_reset_ang_done_flag(void);
void camera_reset_pos_done_flag(void);
void camera_special_function(int mode);
void camera_setup_simple_rotation(int ticks, float rotation);
void camera_setup_tightrope_angle_offset(void* script_args, float height,
                                         float angle);
void camera_setup_radial_position(void* script_args, float distance,
                                  float angle, float height);
void camera_setup_radial_sweep(void* script_args, float travel_time,
                               float rotation_step, float initial_speed,
                               float final_speed, float radial_step,
                               float radial_distance, float center_x,
                               float center_y, float start_angle);
void find_best_conversation_camera_position(void);

extern CameraInfo camera_info;
extern CameraItem camera_item;

#endif
