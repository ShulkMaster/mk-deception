#ifndef MKD_GAME_MOVESET_H
#define MKD_GAME_MOVESET_H

#include "runtime/plyr_info.h"
#include "runtime/mk_struct.h"

typedef struct ScreenObj ScreenObj;
typedef struct MkObj MkObj;
typedef struct MkPtr MkPtr;
typedef struct ScriptSlot ScriptSlot;
typedef struct WeaponDefinition WeaponDefinition;
typedef struct AnimScript AniData;

typedef struct MovesetReflectionOwner {
    char pad00[0x94];
    MkPtr* reflections;
} MovesetReflectionOwner;

typedef struct PlyrStyleDefinition {
    unsigned int animation_header;
    WeaponDefinition* primary_weapon;
    WeaponDefinition* secondary_weapon;
    const char* style_sign_name;
    union { int display_width; int style_sign_width; };
    const char* style_section_name;
    const char* animation_section_name;
    int use_fighting_stance;
    float fields_20[2];
    union {
        float walk_forward_start_step;
        float step;
    };
    union {
        float walk_forward_start_weight;
        float weight;
    };
    float walk_forward_start_frame;
    unsigned int field_34;
    float walk_backward_start_step;
    float walk_backward_start_weight;
    float walk_backward_start_frame;
    unsigned int field_44;
    float walk_forward_step;
    float field_4C;
    float walk_backward_step;
    float field_54;
    float strafe_start_frame;
    float strafe_start_step;
    float strafe_start_weight;
    float field_64;
    unsigned int field_68;
    float fields_6C_78[4];
    float camera_distance_offset;
    FighterAiTable ai_tables[14];
} PlyrStyleDefinition;

typedef PlyrStyleDefinition PlyrMoveBlendData;

typedef PlyrStyleDefinition MovesetDefinition;

typedef struct PlyrMirrorObjLatch {
    MkObj* obj;
    unsigned int instance;
} PlyrMirrorObjLatch;

typedef struct PlyrWeaponMirrorSlot {
    PlyrMirrorObjLatch primary;
    PlyrMirrorObjLatch mirror;
    union {
        PlyrMirrorObjLatch secondary;
        MkHdrLatch secondary_hdr;
    };
} PlyrWeaponMirrorSlot;

typedef struct PlyrMirrorSlots {
    PlyrWeaponMirrorSlot weapon[4];
} PlyrMirrorSlots;

typedef struct GlobalMoveset {
    union { unsigned int animation_header; int fighter_id; };
    union { PlyrStyleDefinition* definition; PlyrMoveBlendData* move_blend_data; unsigned int instance; };
    union { ScriptSlot* script; ScriptSlot* cmo; MovesetReflectionOwner* reflection_owner; };
    union {
        PlyrMirrorSlots mirror_slots;
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
    ScreenObj* style_sign;
    unsigned int style_sign_instance;
    union {
        AniData* animation_data[78];
        struct {
            union {
                AniData* primary_animations[57];
                struct {
                    union { AniData* duck_exit_animation; AniData* standing_animation_script; };
                    union {
                        AniData* forced_step_animation;
                        AniData* walk_forward_start;
                    };
                    AniData* walk_backward_start;
                    AniData* strafe_left_start;
                    AniData* strafe_right_start;
                    AniData* walk_forward_loop;
                    AniData* walk_backward_loop;
                    AniData* strafe_left_loop;
                    AniData* strafe_right_loop;
                    AniData* weapon_block_animation;
                    AniData* weapon_block_loop;
                    AniData* animations_A0[2];
                    AniData* duck_block_intro;
                    AniData* duck_block_animation;
                    AniData* weapon_block_reaction;
                    AniData* animations_B4[4];
                    AniData* duck_animation;
                    AniData* animations_C8[11];
                    AniData* spear_throw_start;
                    AniData* spear_throw_loop;
                    AniData* spear_throw_end;
                    union {
                        AniData* spear_hit;
                        AniData* projectile_return_loop;
                    };
                    union {
                        AniData* spear_tug_start;
                        AniData* projectile_return_end;
                    };
                    AniData* spear_tug_loop;
                    AniData* spear_tug_end;
                    AniData* animations_110[18];
                };
            };
            union {
                AniData* alternate_animations[21];
                struct {
                    AniData* judo_throw_reaction;
                    AniData* animations_15C[19];
                    AniData* weapon_rest_animation;
                };
            };
        };
    };
} GlobalMoveset;

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
