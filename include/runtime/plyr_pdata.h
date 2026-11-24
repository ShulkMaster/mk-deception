#ifndef MKD_PLYR_PDATA_H
#define MKD_PLYR_PDATA_H

#include "msl/msl_types.h"
#include "runtime/image.h"
#include "game/moveset.h"
#include "game/blood_asset.h"
#include "runtime/plyr_info.h"
#include "runtime/plyr_num.h"

typedef struct MkObj MkObj;
typedef struct ScriptSlot ScriptSlot;
typedef struct SwitchData SwitchData;
typedef struct AnimScript AniData;
typedef struct AnimScript AnimScript;
typedef struct GlobalMoveset GlobalMoveset;
typedef struct WeaponDefinition WeaponDefinition;

typedef struct AiFightstyleAttack {
    int opcode;
    union {
        int argument;
        float (*entry)(void);
    };
} AiFightstyleAttack;

typedef struct AiFightstyleAttackTable {
    PlyrMoveBlendData move_data;
    AiFightstyleAttack attacks[25];
} AiFightstyleAttackTable;

typedef char PlyrStyleDefinitionSizeCheck[sizeof(PlyrStyleDefinition) == 0xF0 ? 1 : -1];
typedef char AiFightstyleAttackSizeCheck[sizeof(AiFightstyleAttack) == 0x08 ? 1 : -1];
typedef char AiFightstyleAttackTableSizeCheck[sizeof(AiFightstyleAttackTable) == 0x1B8 ? 1 : -1];

typedef struct PlyrWeaponImpactData {
    char pad00[0x20];
    int attack_region;
} PlyrWeaponImpactData;

typedef PlyrStyleDefinition PlyrMoveDisplayData;

typedef struct PlyrMirrorBoneMapEntry {
    int field_00;
    int bone_index;
} PlyrMirrorBoneMapEntry;

typedef struct PlyrMirrorBoneMap {
    int count;
    PlyrMirrorBoneMapEntry* entries;
} PlyrMirrorBoneMap;

typedef struct PlyrStateFlagBits {
    unsigned char pad_bit7 : 1;
    unsigned char frozen : 1;
    unsigned char pad_bit5 : 1;
    unsigned char dizzy : 1;
    unsigned char bit3 : 1;
    unsigned char pad_bit2 : 1;
    unsigned char projectile_invulnerable : 1;
    unsigned char projectile_request : 1;
} PlyrStateFlagBits;

typedef union PlyrStateFlags {
    unsigned char raw;
    unsigned int raw_word;
    PlyrStateFlagBits bits;
} PlyrStateFlags;

typedef struct PlyrDisabledMove {
    unsigned int until_tick;
    unsigned int move;
} PlyrDisabledMove;

typedef struct FatalityDefinition {
    const char* primary_section;
    unsigned int primary_script;
    unsigned int primary_victim_script;
    float primary_min;
    float primary_max;
    char pad14[4];
    const char* secondary_section;
    unsigned int secondary_script;
    unsigned int secondary_victim_script;
    float secondary_min;
    float secondary_max;
    char pad2C[8];
    unsigned int suicide_script;
    unsigned int suicide_sidekick_script;
} FatalityDefinition;

typedef struct FatalityRadiusCheck {
    int select_farthest;
    float center_x;
    char pad08[4];
    float center_z;
    float radius;
} FatalityRadiusCheck;

typedef FatalityDefinition FatalityDistanceLimits;

typedef FighterRuntimeData PlyrStatusData;

typedef struct PlyrProcLatch {
    struct MkProc* proc;
    unsigned int instance;
} PlyrProcLatch;

typedef struct PlyrPdata {
    void* vtbl;
    unsigned int instance;
    union {
        struct {
            union {
                struct MkProc* player_proc;
                struct MkProc* opponent_proc;
            };
            union {
                unsigned int player_proc_instance;
                unsigned int opponent_proc_instance;
            };
        };
        PlyrProcLatch player_proc_latch;
    };
    struct PlyrPdata* his_plyr_pdata;
    MkObj* his_obj;
    PlyrInfo* plyr_info;
    PlyrStateFlags state_flags;
    struct MkProc* own_player_proc;
    unsigned int own_player_proc_instance;
    struct MkProc* aux_player_proc;
    unsigned int aux_player_proc_instance;
    union {
        struct {
            MkObj* tracked_obj;
            unsigned int tracked_obj_instance;
        };
        PlyrMirrorObjLatch tracked_obj_latch;
    };
    PlyrMirrorObjLatch held_opponent_latch;
    PlyrMirrorObjLatch aux_weapon_latch;
    PlyrMirrorObjLatch mirror_obj;
    union {
        struct {
            struct MkProc* hold_proc;
            unsigned int hold_proc_instance;
        };
        MkHdrLatch hold_hdr_latch;
    };
    MkPtr* item_links;
    struct MkProc* anim_proc;
    unsigned int anim_proc_instance;
    union {
        struct {
            struct MkProc* left_hand_anim_proc;
            unsigned int left_hand_anim_instance;
        };
        PlyrProcLatch left_hand_anim_latch;
    };
    union {
        struct {
            struct MkProc* right_hand_anim_proc;
            unsigned int right_hand_anim_instance;
        };
        PlyrProcLatch right_hand_anim_latch;
    };
    union {
        struct {
            struct MkProc* face_anim_proc;
            unsigned int face_anim_proc_instance;
        };
        PlyrProcLatch face_anim_latch;
    };
    PlyrProcLatch field_7C;
    PlyrProcLatch field_84;
    PlyrProcLatch field_8C;
    PlyrProcLatch goro_hand_anim[4];
    struct MkProc* transient_proc;
    unsigned int transient_proc_instance;
    unsigned int reserved_BC[8];
    PlyrMirrorObjLatch impaled_item_a;
    PlyrMirrorObjLatch impaled_item_b;
    PlyrMirrorObjLatch impaled_item_a_secondary;
    PlyrMirrorObjLatch impaled_item_b_secondary;
    unsigned int reserved_FC;
    struct MkProc* spear_proc;
    unsigned int spear_proc_instance;
    unsigned int reserved_108;
    struct ScalePdata* scale_pdata;
    unsigned int scale_pdata_instance;
    AniTextureControlItem facial_texture;
    struct MkProc* foot_print_proc;
    unsigned int foot_print_proc_instance;
    unsigned int reserved_124[4];
    MkPtr* active_weapon_links;
    unsigned int reserved_138[3];
    PlyrMirrorObjLatch reserved_obj_latches[15];
    union {
        PlyrMirrorObjLatch held_by_object_latch;
        struct {
            int held_by_player;
            int hold_state;
        };
    };
    int (*aux_update_callback)(void);
    PlyrWeaponImpactData* weapon_impact;
    union {
        void* pchr;
        int character_id;
    };
    PLYR_NUM plyr_num;
    PlyrMirrorBoneMap* mirror_bone_map;
    char pad1D8[4];
    int collision_result;
    int hit_flash_enabled;
    int blocking_disabled;
    int blocking_disabled_2;
    unsigned int blocking_disable_tick_1;
    unsigned int blocking_disable_tick_2;
    union {
        int input_unlock_tick;
        unsigned int action_lock_a;
    };
    union {
        int attacks_disabled_until;
        unsigned int action_lock_b;
    };
    int attack_disable_tick_p1;
    int attack_disable_tick_p2;
    int dodge_sound_played;
    char pad208[4];
    PlyrDisabledMove disabled_moves[4];
    int special_move_disabled;
    int f_constrained;
    int field_234;
    int breaker_strength;
    int throw_restriction;
    int secondary_state;
    int state;
    int previous_state;
    union {
        SwitchData* switch_data;
        int controller_port;
    };
    int hit_count;
    int reaction_hit_count;
    int hit_streak;
    int combo_hit_count;
    int combo_flags;
    int attack_counter;
    int shared_attack_until;
    int field_26C;
    char pad270[4];
    unsigned int last_collision_tick;
    int last_back_dash_tick;
    char pad27C[4];
    unsigned int charge_up_disabled_until;
    unsigned int damage_boost_until;
    float damage_boost;
    float taunt_life_scale;
    float postround_value;
    float combo_damage;
    float damage_multiplier;
    int his_attack_counter;
    int script_exit_value_int;
    int script_exit_args[2];
    union {
        int script_exit_arg_2;
        int duck_loop_counter;
    };
    int fatality_advance;
    int facial_damage_complete;
    float summon_position_x;
    float summon_position_z;
    float summon_position_y;
    int drone_request;
    int drone_handoff_pending;
    int collision_disabled;
    int duck_wait_ticks;
    char pad2D4[8];
    unsigned int block_start_tick;
    int opponent_attack_counter;
    int field_2E4;
    int push_blocked;
    int field_2EC;
    union {
        int reaction_counter;
        int opponent_attack_counter_copy;
    };
    union {
        int block_requirement;
        int pending_hit_strength;
    };
    int pending_reaction;
    PlyrWeaponStyle* weapon_styles[3];
    int player_slot;
    union {
        PlyrFighterDefinition* fighter_definition;
        GlobalMoveset* global_moveset;
        struct FatalityWeaponReflectionSet* weapon_reflections;
    };
    PlyrMirrorSlots* mirror_slots;
    union {
        PlyrMoveDisplayData* active_move_display;
        MovesetDefinition* global_moveset_definition;
    };
    union {
        struct {
            AniData* base_animations[8];
            AniData* dizzy_animation;
            char pad33C[4];
            AniData* big_boss_taunt_animation;
            AniData* common_exit_animation;
            AniData* turn_to_screen_animation;
            AniData* smoke_land_animation;
            char pad350[0x0C];
            AniData* noob_entrance_animation;
            char pad360[8];
            AniData* reaction_animation;
            void* reaction_animation_a;
            void* reaction_animation_b;
            void* reaction_animation_c;
            void* esp1_reaction_animation;
            union {
                AniData* screen_taunt_animation;
                void* scorpion_spear_hit;
            };
            void* scorpion_spear_pull;
            union {
                AniData* goro_fold_animation;
                void* scorpion_spear_recover;
            };
            char pad388[4];
            AniData* ice_reaction_animation;
            char pad390[0x14];
            union {
                AniData* fatality_animation;
                int fatality_palette;
            };
            AniData* mileena_veil_animation;
            char pad3AC[4];
            unsigned int suicide_camera_sidekick_ntsc;
            unsigned int suicide_camera_main_pal;
            unsigned int suicide_camera_sidekick_pal;
            char pad3BC[0x18];
            unsigned int fatality_camera_ntsc;
            unsigned int fatality_camera_pal;
        };
        int animation_data[49];
    };
    AnimScript* face_animations[26];
    RwSphere shadow_sphere;
    Vec shadow_ground_point;
    float shadow_ground_radius;
    struct RwRaster* shadow_raster;
    struct RwRaster* shadow_blur_raster;
    struct RwTexture* shadow_texture;
    MkObj* shadowbox;
    struct MkProc* shadow_proc;
    ScriptSlot* cmo;
    BloodModelData blood_model;
    unsigned int next_large_bleed_tick;
    unsigned int next_blood_glop_tick;
    int duck_reaction_active;
    float saved_position_x;
    float saved_position_y;
    float saved_position_z;
    char pad5C0[0x2C];
    unsigned int saved_anim_script_word;
    unsigned int saved_anim_flags;
    char pad5F4[8];
    float saved_anim_low_frame;
    float saved_anim_high_frame;
    char pad604[0xC0];
    float facial_damage;
    int attack_region;
    int attack_type;
    unsigned int round_attack_count;
    int round_attack_stage;
    int combo_depth;
    int field_6DC;
    int strafe_direction;
    int block_hit_count;
    MslSoundHandle scream_sound_handle;
    int repeated_action_count;
    unsigned int previous_action;
    int death_type;
    union {
        unsigned int* status_flags;
        PlyrStatusData* status_data;
        FighterRuntimeData* runtime_data;
        FighterAiTableContainer* ai_tables;
    };
    int fatality_shove_active;
    struct MkProc* jaw_monitor;
    unsigned int jaw_monitor_instance;
    struct MkProc* baraka_blades_monitor;
    unsigned int baraka_blades_monitor_instance;
    unsigned int reserved_710[2];
    int (*baraka_moveset_callback)(
        struct PlyrPdata*, PlyrMirrorSlots*);
    void* active_pickup;
    int danger_zone_callback;
    int impaled_projectile_state;
    int field_728;
    MkObj* sidekick_obj;
    unsigned int sidekick_instance;
    struct MkProc* sidekick_anim_proc;
    unsigned int sidekick_anim_instance;
    int sidekick_active;
    int sidekick_available;
    int angle_jump_pending;
    int taunts_performed;
} PlyrPdata;

#define PLYR_PDATA_SIZE 0x74C
#define PLYR_PDATA_POOL_COUNT 2

extern PlyrPdata* plyr_pdata;
extern PlyrPdata* free_mkpdata_plyrs;

#endif
