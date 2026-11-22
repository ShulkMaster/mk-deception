#ifndef MK_OBJ_FLAGS_H
#define MK_OBJ_FLAGS_H

typedef struct MkObjFlags {
    unsigned int bit08_7 : 1;
    unsigned int airborne : 1;
    unsigned int gravity_enabled : 1;
    unsigned int transform_dirty : 1;
    unsigned int angular_velocity_enabled : 1;
    unsigned int rotation_enabled : 1;
    unsigned int scale_active : 1;
    unsigned int moving : 1;
    unsigned int launched : 1;
    unsigned int bit09_6 : 1;
    unsigned int tightrope_restricted : 1;
    unsigned int bit09_4 : 1;
    unsigned int face_opponent : 1;
    unsigned int wall_restricted : 1;
    unsigned int head_tracking : 1;
    unsigned int bit09_0 : 1;
    unsigned int still_move : 1;
    unsigned int bit0A_6 : 1;
    unsigned int hidden : 1;
    unsigned int bit0A_4 : 1;
    unsigned int bit0A_3 : 1;
    unsigned int weapon_effect : 1;
    unsigned int pin_animation : 1;
    unsigned int bit0A_0 : 1;
    unsigned int root_transform_pending : 1;
    unsigned int bit0B_6 : 1;
    unsigned int force_anim_speed : 1;
    unsigned int bit0B_4 : 1;
    unsigned int bit0B_3 : 1;
    unsigned int pivot_enabled : 1;
    unsigned int special_texture : 1;
    unsigned int bit0B_0 : 1;
} MkObjFlags;

#endif
