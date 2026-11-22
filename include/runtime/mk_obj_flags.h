#ifndef MK_OBJ_FLAGS_H
#define MK_OBJ_FLAGS_H

typedef struct MkObjFlags {
    unsigned char bit08_7 : 1;
    unsigned char airborne : 1;
    unsigned char gravity_enabled : 1;
    unsigned char transform_dirty : 1;
    unsigned char angular_velocity_enabled : 1;
    unsigned char rotation_enabled : 1;
    unsigned char scale_active : 1;
    unsigned char moving : 1;
    unsigned char launched : 1;
    unsigned char bit09_6 : 1;
    unsigned char tightrope_restricted : 1;
    unsigned char bit09_4 : 1;
    unsigned char face_opponent : 1;
    unsigned char wall_restricted : 1;
    unsigned char head_tracking : 1;
    unsigned char bit09_0 : 1;
    unsigned char still_move : 1;
    unsigned char bit0A_6 : 1;
    unsigned char hidden : 1;
    unsigned char bit0A_4 : 1;
    unsigned char bit0A_3 : 1;
    unsigned char weapon_effect : 1;
    unsigned char pin_animation : 1;
    unsigned char bit0A_0 : 1;
    unsigned char root_transform_pending : 1;
    unsigned char bit0B_6 : 1;
    unsigned char force_anim_speed : 1;
    unsigned char bit0B_4 : 1;
    unsigned char bit0B_3 : 1;
    unsigned char pivot_enabled : 1;
    unsigned char special_texture : 1;
    unsigned char bit0B_0 : 1;
} MkObjFlags;

#endif
