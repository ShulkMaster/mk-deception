#ifndef RUNTIME_CAM_H
#define RUNTIME_CAM_H

#include "math/gxVect.h"
#include "rw/rtquat.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

typedef struct CamVec3 {
    float x;
    float y;
    float z;
} CamVec3;

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
    char pad00[4];
    unsigned int instance; /* +0x04 - matched to CameraItem.instance */
    unsigned char flags;   /* +0x08 - CamObjFlags bitfield overlay */
    char pad09[0x17];
    RwMatrix* matrix; /* +0x20 - camera world matrix */
    RwMatrix* field_24; /* +0x24 - active camera transform */
    char pad28[0x78];
    float pos_x; /* +0xA0 */
    float pos_y;
    float pos_z;
    char padAC[0x24];
    float ang_x; /* +0xD0 */
    float ang_y;
    float ang_z;
} CameraObj;

/* Validated camera_item latch: node live iff node && node->instance == instance. */
typedef struct CameraItem {
    CameraObj* node;         /* +0x00 */
    unsigned int instance;   /* +0x04 */
} CameraItem;

typedef struct CameraPdataFlags {
    unsigned char bit7 : 1;
    unsigned char pad : 7;
} CameraPdataFlags;

typedef CameraItem CameraItemList;

/*
 * camera_info.pdata: targets @ 0x0C/0x18, speed @ 0x50, flags @ 0x6C,
 * pause @ 0x70, parent/mirror mats @ 0x80/0xC0.
 */
typedef struct CameraPdata {
    char pad0[0x0C];
    float target_pos_x; /* +0x0C */
    float target_pos_y;
    float target_pos_z;
    float target_ang_x; /* +0x18 */
    float target_ang_y;
    float target_ang_z;
    char pad1[0x1C];
    void* attacker; /* +0x40 */
    void* victim;   /* +0x44 */
    char pad48[8];
    float speed; /* +0x50 */
    void* anim_pdata;
    unsigned int anim_instance;
    void* bone_obj;
    unsigned int bone_instance;
    void* anim_path;
    void* aux_data;
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
} CameraPdata;

typedef struct CameraInfo {
    MkProc* proc;
    CameraPdata* pdata;
} CameraInfo;

/* Krypt / setup camera path */
float p_krypt_camera_loop(void);
float p_krypt_camera_proc(void);
void set_camera_angle(CamVec3* ang);
void set_camera_position(CamVec3* pos);
void get_camera_angle(CamVec3* ang);
void get_camera_position(CamVec3* pos);
void xfer_camera(MkProcEntryFn entry, int reset_projection);
void turn_camera_on(void);
void turn_camera_off(void);
void camera_idle(void);
MkProc* get_camera_proc(void);
CameraPdata* get_pdata_of_camera(void);

/* Intro / background anim path used by p_setup_krypt */
void cam_set_intro_cam_speed(float speed);
void cam_set_intro_cam_pause_ticks(float ticks);
void camera_init_animation(void* anim_path, MkProcEntryFn override_entry);
void camera_run_animation(int wait_flag);
void camera_wait_for_animation_completion(void);
void camera_set_anim_aux_data(void* data);
void camera_set_animation_parent_position(const CamVec3* position);
void camera_set_animation_parent_angle(const CamVec3* angle, int relative);
void* get_intro_camera_path(void);
void set_intro_camera_path(void* path);
void* camera_get_victim(void);
void camera_set_victim(void* object);
void* camera_get_attacker(void);
void camera_set_attacker(void* object);
void camera_unpause_player(void);
void camera_pause_player(void);
void camera_set_lookat_focus(void* object);
void camera_set_movement_focus_obj(void* object);
int camera_get_mirror_flag(void);
void hide_sobj_if_camera_is_in_rectangle(void* object, int mode, float min_x,
                                         float min_z, float max_x,
                                         float max_z);
void hide_sobj_if_camera_is_in_cylinder(void* object, int mode, float radius,
                                        float height);
void turn_off_sobj_if_camera_is_in_rectangle(void* object, int mode,
                                             float min_x, float min_z,
                                             float max_x, float max_z);
void turn_off_sobj_if_camera_is_in_cylinder(void* object, int mode,
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
void camera_set_custom_camera_movement_flag(int enabled);
void camera_set_radial_movement(int enabled);
void camera_set_center_of_rotation(const CamVec3* center);
void camera_set_travel_time(float time);
void camera_set_rotation_direction(int direction);
void camera_set_final_speed(float speed);
void camera_set_initial_speed(float speed);
void camera_set_movement_offset_explicit(float x, float y, float z);
void camera_set_look_mode(int mode);
void camera_set_movement_mode(int mode);
int camera_is_ang_move_done(void);
int camera_is_pos_move_done(void);
void camera_reset_ang_done_flag(void);
void camera_reset_pos_done_flag(void);

extern CameraInfo camera_info;
extern CameraItem camera_item;

#endif
