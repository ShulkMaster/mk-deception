#ifndef GAME_NCS_H
#define GAME_NCS_H

struct MkObj;
struct Vec;
struct NcsLimbUpdatePdata;

typedef struct NcsCameraWallRegion NcsCameraWallRegion;

unsigned int start_blood_particles_scripts(int particle_mask, unsigned int bone);
void ncs_camera_wall_show_hide_alpha(NcsCameraWallRegion* regions);
struct MkObj* limb_sever_set_motion(
    struct MkObj* owner, int limb, struct Vec* velocity, float gravity,
    struct NcsLimbUpdatePdata* motion, int enable_ground, float ground_offset,
    int ground_value, float vertical_bounce_scale, int field_18,
    int include_children);

#endif
