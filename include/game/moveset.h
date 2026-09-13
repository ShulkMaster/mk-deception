#ifndef MKD_GAME_MOVESET_H
#define MKD_GAME_MOVESET_H

#include "runtime/plyr_info.h"
#include "runtime/mk_struct.h"

typedef struct ScreenObj ScreenObj;
typedef struct MkObj MkObj;
typedef struct MkPtr MkPtr;
typedef struct ScriptSlot ScriptSlot;
typedef struct WeaponDefinition WeaponDefinition;
typedef struct AniData AniData;

typedef struct MovesetReflectionOwner {
    char pad00[0x94];
    MkPtr* reflections; /* +0x94 */
} MovesetReflectionOwner;

/* Shared style MKO prefix through the fourteen AI selection tables. */
typedef struct PlyrStyleDefinition {
    unsigned int animation_header; /* +0x00 */
    WeaponDefinition* primary_weapon; /* +0x04 */
    WeaponDefinition* secondary_weapon; /* +0x08 */
    union { const char* section_name_0C; const char* style_sign_name; }; /* +0x0C - MKO tag 4 */
    union { int display_width; int style_sign_width; }; /* +0x10 */
    union { const char* section_name_14; const char* style_section_name; }; /* +0x14 - MKO tag 4 */
    const char* animation_section_name; /* +0x18 */
    int use_fighting_stance; /* +0x1C */
    float fields_20[2]; /* +0x20 */
    union {
        float walk_forward_start_step;
        float step;
    }; /* +0x28 */
    union {
        float walk_forward_start_weight;
        float weight;
    }; /* +0x2C */
    float walk_forward_start_frame; /* +0x30 */
    unsigned int field_34;
    float walk_backward_start_step; /* +0x38 */
    float walk_backward_start_weight; /* +0x3C */
    float walk_backward_start_frame; /* +0x40 */
    unsigned int field_44;
    float walk_forward_step; /* +0x48 */
    float field_4C;
    float walk_backward_step; /* +0x50 */
    float field_54;
    float strafe_start_frame; /* +0x58 */
    float strafe_start_step; /* +0x5C */
    float strafe_start_weight; /* +0x60 */
    float field_64;
    unsigned int field_68;
    union { float fields_6C[5]; struct { float fields_6C_78[4]; float camera_distance_offset; }; }; /* +0x6C */
    FighterAiTable ai_tables[14]; /* +0x80 */
} PlyrStyleDefinition;

typedef PlyrStyleDefinition PlyrMoveBlendData;

typedef PlyrStyleDefinition MovesetDefinition;

typedef struct PlyrMirrorObjLatch {
    MkObj* obj;
    unsigned int instance;
} PlyrMirrorObjLatch;

typedef struct PlyrWeaponMirrorSlot {
    PlyrMirrorObjLatch primary;
    PlyrMirrorObjLatch mirror; /* +0x08 */
    union {
        PlyrMirrorObjLatch secondary;
        MkHdrLatch secondary_hdr;
    }; /* +0x10 */
} PlyrWeaponMirrorSlot; /* 0x18 */

typedef struct PlyrMirrorSlots {
    PlyrWeaponMirrorSlot weapon[4];
} PlyrMirrorSlots; /* 0x60 */

/* The active fighter definition and the three loaded weapon styles select
 * entries of global_movesets. Animation palettes are embedded pointer arrays;
 * load_player_anim_files passes their addresses, not their first entries. */
typedef struct GlobalMoveset {
    union { unsigned int animation_header; int fighter_id; };
    union { PlyrStyleDefinition* definition; PlyrMoveBlendData* move_blend_data; unsigned int instance; };
    union { ScriptSlot* script; ScriptSlot* cmo; MovesetReflectionOwner* reflection_owner; };
    union {
        PlyrMirrorSlots mirror_slots; /* +0x0C */
        struct {
            MkObj* primary_weapon;
            unsigned int primary_weapon_instance;
            MkObj* reflection_weapon;
            unsigned int reflection_weapon_instance;
            PlyrMirrorObjLatch weapon_latch_1C;
            MkObj* secondary_weapon;
            unsigned int secondary_weapon_instance;
            PlyrMirrorObjLatch weapon_latches_2C[8];
        };
    };
    ScreenObj* style_sign; /* +0x6C */
    unsigned int style_sign_instance; /* +0x70 */
    union {
        AniData* animation_data[78]; /* +0x74 - complete loaded palette */
        struct {
            union {
                AniData* primary_animations[57]; /* +0x74 */
                struct {
                    union { AniData* duck_exit_animation; AniData* standing_animation_script; }; /* +0x74 */
                    union {
                        AniData* forced_step_animation; /* +0x78 */
                        AniData* walk_forward_start;
                    };
                    AniData* walk_backward_start; /* +0x7C */
                    AniData* strafe_left_start; /* +0x80 */
                    AniData* strafe_right_start; /* +0x84 */
                    AniData* walk_forward_loop; /* +0x88 */
                    AniData* walk_backward_loop; /* +0x8C */
                    AniData* strafe_left_loop; /* +0x90 */
                    AniData* strafe_right_loop; /* +0x94 */
                    AniData* weapon_block_animation; /* +0x98 */
                    union { AniData* weapon_block_loop; MkPtr* object_list; }; /* +0x9C */
                    AniData* animations_A0[2];
                    AniData* duck_block_intro; /* +0xA8 */
                    AniData* duck_block_animation; /* +0xAC */
                    AniData* weapon_block_reaction; /* +0xB0 */
                    AniData* animations_B4[4];
                    AniData* duck_animation; /* +0xC4 */
                    AniData* animations_C8[11];
                    AniData* spear_throw_start; /* +0xF4 */
                    AniData* spear_throw_loop; /* +0xF8 */
                    AniData* spear_throw_end; /* +0xFC */
                    union {
                        AniData* spear_hit;
                        AniData* projectile_return_loop;
                    }; /* +0x100 */
                    union {
                        AniData* spear_tug_start;
                        AniData* projectile_return_end;
                    }; /* +0x104 */
                    AniData* spear_tug_loop; /* +0x108 */
                    AniData* spear_tug_end; /* +0x10C */
                    AniData* animations_110[18];
                };
            };
            union {
                AniData* alternate_animations[21]; /* +0x158 */
                struct {
                    AniData* judo_throw_reaction; /* +0x158 */
                    AniData* animations_15C[19];
                    AniData* weapon_rest_animation; /* +0x1A8 */
                };
            };
        };
    };
} GlobalMoveset; /* 0x1AC */

typedef GlobalMoveset PlyrFighterDefinition;
typedef GlobalMoveset PlyrWeaponStyle;

typedef char GlobalMovesetSizeCheck[sizeof(GlobalMoveset) == 0x1AC ? 1 : -1];
#ifndef __cplusplus
typedef char GlobalMovesetPrimaryCheck[(unsigned int)&((GlobalMoveset*)0)->primary_animations == 0x74 ? 1 : -1];
typedef char GlobalMovesetAlternateCheck[(unsigned int)&((GlobalMoveset*)0)->alternate_animations == 0x158 ? 1 : -1];
#endif

#define GLOBAL_MOVESET_COUNT 8
extern GlobalMoveset global_movesets[GLOBAL_MOVESET_COUNT];

#endif
