#ifndef GAME_NCS_H
#define GAME_NCS_H

struct MkObj;
struct PlyrInfo;
struct Vec;
struct NcsLimbUpdatePdata;

typedef struct NcsCameraWallRegion NcsCameraWallRegion;

unsigned int start_blood_particles_scripts(int particle_mask, unsigned int bone);
void ncs_camera_wall_show_hide_alpha(NcsCameraWallRegion* regions);
float mkobj_pos_pos_dot_normal_xz(
    struct MkObj* from, const struct MkObj* to, const struct Vec* normal);
struct MkObj* limb_sever_set_motion(
    struct MkObj* owner, int limb, struct Vec* velocity, float gravity,
    struct NcsLimbUpdatePdata* motion, int enable_ground, float ground_offset,
    int ground_value, float vertical_bounce_scale, int field_18,
    int include_children);

void limb_sever_bone_attach(
    struct PlyrInfo* target_player, int owner_bone,
    struct Vec* offset, struct Vec* rotation,
    struct PlyrInfo* owner_player, int limb, int target_bone,
    int include_children);

void limb_sever_explode_apart_plyr_num(int a, float b, float c, float d, int e);
void stop_prison_grab_proc(void);
void done_prison_grab_proc(struct PrisonGrabPdata* pdata);
void mkscripts_set_anim_check_flag(int zone_check, int grab_check);
void destroy_gore2_obj(unsigned int object_id, int particle_index);
void limb_sever_update_slide_end_coeff(struct NcsLimbUpdatePdata* limb, float coefficient);
void start_sweat_particles_scripts(int a, int b);
void start_gore2_update(void);
extern void start_mkpfx_FadeSnapShot(void);
float p_sc_spear_retract(void);
float p_sc_spear_kill(void);
#endif
