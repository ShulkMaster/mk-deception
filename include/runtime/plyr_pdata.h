#ifndef MKD_PLYR_PDATA_H
#define MKD_PLYR_PDATA_H

#include "msl/msl_types.h"
#include "runtime/image.h"
#include "runtime/plyr_info.h"

/*
 * Fight player pdata (mkpdata_plyr).
 * Pool: _mkpdata_plyrs .bss size 0xE98 = 2 * 0x74C (init_mkpdata_plyrs).
 * Offsets from plyr_pdata_get_* (Ghidra) + get_mkpdata_plyr zeroing.
 */

typedef struct MkObj MkObj;
typedef struct ScriptSlot ScriptSlot;
typedef struct SwitchData SwitchData;
typedef struct AniData AniData;
typedef struct AnimScript AnimScript;
typedef struct MovesetDefinition MovesetDefinition;
typedef struct GlobalMoveset GlobalMoveset;
typedef struct WeaponDefinition WeaponDefinition;

typedef struct PlyrStyleDefinition {
    unsigned int animation_header;
    WeaponDefinition* primary_weapon;
    WeaponDefinition* secondary_weapon;
    char pad0C[0x0C];
    const char* animation_section_name;
} PlyrStyleDefinition;

typedef struct PlyrMoveBlendData {
    char pad00[4];
    void* primary_weapon;   /* +0x04 */
    void* secondary_weapon; /* +0x08 */
    char pad0C[0x10];
    int use_fighting_stance; /* +0x1C */
    char pad20[8];
    union {
        float walk_forward_start_step;
        float step;
    }; /* +0x28 */
    union {
        float walk_forward_start_weight;
        float weight;
    }; /* +0x2C */
    float walk_forward_start_frame;  /* +0x30 */
    char pad34[4];
    float walk_backward_start_step;   /* +0x38 */
    float walk_backward_start_weight; /* +0x3C */
    float walk_backward_start_frame;  /* +0x40 */
    char pad44[4];
    float walk_forward_step; /* +0x48 */
    char pad4C[4];
    float walk_backward_step; /* +0x50 */
    char pad54[4];
    float strafe_start_frame;  /* +0x58 */
    float strafe_start_step;   /* +0x5C */
    float strafe_start_weight; /* +0x60 */
} PlyrMoveBlendData;

typedef struct PlyrWeaponImpactData {
    char pad00[0x20];
    int attack_region; /* +0x20 */
} PlyrWeaponImpactData;

typedef struct PlyrMoveDisplayData {
    char pad00[0x10];
    int display_width; /* +0x10 */
} PlyrMoveDisplayData;

typedef struct PlyrMirrorBoneMapEntry {
    int field_00;
    int bone_index;
} PlyrMirrorBoneMapEntry;

typedef struct PlyrMirrorBoneMap {
    int count;
    PlyrMirrorBoneMapEntry* entries;
} PlyrMirrorBoneMap;

typedef struct PlyrMirrorObjLatch {
    MkObj* obj;
    unsigned int instance;
} PlyrMirrorObjLatch;

typedef struct PlyrWeaponMirrorSlot {
    PlyrMirrorObjLatch primary;
    PlyrMirrorObjLatch mirror; /* +0x08 */
    PlyrMirrorObjLatch secondary; /* +0x10 */
} PlyrWeaponMirrorSlot; /* 0x18 */

typedef struct PlyrMirrorSlots {
    PlyrWeaponMirrorSlot weapon[4];
} PlyrMirrorSlots; /* 0x60 */

typedef struct PlyrFighterDefinition {
    char pad00[4];
    PlyrMoveBlendData* move_blend_data; /* +0x04 */
    ScriptSlot* cmo; /* +0x08 */
    PlyrMirrorSlots mirror_slots; /* +0x0C - default weapon-trail slots */
    char pad6C[8];
    AniData* duck_exit_animation; /* +0x74 */
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
    char pad9C[0x10];
    AniData* duck_block_animation; /* +0xAC */
    AniData* weapon_block_reaction; /* +0xB0 */
    char padB4[0x10];
    AniData* duck_animation; /* +0xC4 */
    char padC8[0x2C];
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
} PlyrFighterDefinition;

typedef struct PlyrStateFlagBits {
    unsigned char pad_bit7 : 1;
    unsigned char frozen : 1; /* bit6 - freeze-light lifetime */
    unsigned char pad_bit5 : 1;
    unsigned char dizzy : 1; /* bit4 - held in the puzzle dizzy state */
    unsigned char bit3 : 1;
    unsigned char pad_bit2 : 1;
    unsigned char projectile_invulnerable : 1; /* bit1 */
    unsigned char projectile_request : 1; /* bit0 - sidekick projectile request */
} PlyrStateFlagBits;

typedef union PlyrStateFlags {
    unsigned char raw;
    PlyrStateFlagBits bits;
} PlyrStateFlags;

typedef struct PlyrDisabledMove {
    unsigned int until_tick;
    unsigned int move;
} PlyrDisabledMove;

typedef struct FatalityDefinition {
    const char* primary_section; /* +0x00 */
    unsigned int primary_script; /* +0x04 */
    unsigned int primary_victim_script; /* +0x08 */
    float primary_min;
    float primary_max;
    char pad14[4];
    const char* secondary_section; /* +0x18 */
    unsigned int secondary_script; /* +0x1C */
    unsigned int secondary_victim_script; /* +0x20 */
    float secondary_min;
    float secondary_max;
    char pad2C[8];
    unsigned int suicide_script; /* +0x34 */
    unsigned int suicide_sidekick_script; /* +0x38 */
} FatalityDefinition;

typedef struct FatalityRadiusCheck {
    int select_farthest;
    float center_x;
    char pad08[4];
    float center_z;
    float radius;
} FatalityRadiusCheck;

typedef FatalityDefinition FatalityDistanceLimits;

typedef struct PlyrStatusData {
    unsigned int flags;
    char pad04[0x80];
    union {
        FatalityDefinition* fatality_definition;
        FatalityDistanceLimits* fatality_limits;
    }; /* +0x84 */
    char pad88[0x2C];
    struct TrialWrapupData* trial_wrapup_data; /* +0xB4 */
    char padB8[0x7C];
    unsigned int reaction_cleanup; /* +0x134 - cleanup script function */
    char pad138[4];
    unsigned int throw_script; /* +0x13C */
} PlyrStatusData;

typedef struct PlyrWeaponStyle {
    unsigned int animation_header;
    union {
        PlyrStyleDefinition* definition;
        unsigned int instance;
    };
    ScriptSlot* script;
    PlyrMirrorSlots mirror_slots; /* +0x0C */
    char pad6C[8];
    int animation_data; /* +0x74 - async animation destination */
    char pad78[0x24];
    MkPtr* object_list; /* +0x9C - owns style weapon/reflection objects */
} PlyrWeaponStyle;

typedef struct PlyrProcLatch {
    struct MkProc* proc;
    unsigned int instance;
} PlyrProcLatch;

typedef struct PlyrPdata {
    void* vtbl; /* +0x00 - vtbl_mkpdata_plyr; free-list next when idle */
    unsigned int instance; /* +0x04 */
    union {
        struct MkProc* player_proc;
        struct MkProc* opponent_proc;
    }; /* +0x08 */
    union {
        unsigned int player_proc_instance;
        unsigned int opponent_proc_instance;
    }; /* +0x0C */
    struct PlyrPdata* his_plyr_pdata; /* +0x10 */
    MkObj* his_obj;                   /* +0x14 */
    PlyrInfo* plyr_info;              /* +0x18 - GameInfo plyr0/plyr1 */
    PlyrStateFlags state_flags; /* +0x1C */
    char pad1D[3];
    struct MkProc* own_player_proc; /* +0x20 */
    unsigned int own_player_proc_instance; /* +0x24 */
    struct MkProc* aux_player_proc; /* +0x28 */
    unsigned int aux_player_proc_instance; /* +0x2C */
    union {
        struct {
            MkObj* tracked_obj; /* +0x30 */
            unsigned int tracked_obj_instance; /* +0x34 */
        };
        PlyrMirrorObjLatch tracked_obj_latch;
    };
    PlyrMirrorObjLatch held_opponent_latch; /* +0x38 */
    PlyrMirrorObjLatch aux_weapon_latch; /* +0x40 */
    PlyrMirrorObjLatch mirror_obj; /* +0x48 */
    struct MkProc* hold_proc; /* +0x50 */
    unsigned int hold_proc_instance; /* +0x54 */
    MkPtr* item_links; /* +0x58 - item attachment/link records */
    struct MkProc* anim_proc; /* +0x5C */
    unsigned int anim_proc_instance; /* +0x60 */
    struct MkProc* left_hand_anim_proc; /* +0x64 */
    unsigned int left_hand_anim_instance; /* +0x68 */
    struct MkProc* right_hand_anim_proc; /* +0x6C */
    unsigned int right_hand_anim_instance; /* +0x70 */
    struct MkProc* face_anim_proc; /* +0x74 */
    unsigned int face_anim_proc_instance; /* +0x78 */
    PlyrProcLatch field_7C;
    PlyrProcLatch field_84;
    PlyrProcLatch field_8C;
    PlyrProcLatch goro_hand_anim[4];
    struct MkProc* transient_proc; /* +0xB4 */
    unsigned int transient_proc_instance; /* +0xB8 */
    unsigned int reserved_BC[8];
    PlyrMirrorObjLatch impaled_item_a;           /* +0xDC */
    PlyrMirrorObjLatch impaled_item_b;           /* +0xE4 */
    PlyrMirrorObjLatch impaled_item_a_secondary; /* +0xEC */
    PlyrMirrorObjLatch impaled_item_b_secondary; /* +0xF4 */
    unsigned int reserved_FC;
    struct MkProc* spear_proc; /* +0x100 */
    unsigned int spear_proc_instance; /* +0x104 */
    unsigned int reserved_108[3];
    AniTextureControlItem facial_texture; /* +0x114 */
    unsigned int reserved_11C[6];
    MkPtr* active_weapon_links; /* +0x134 */
    unsigned int reserved_138[3];
    PlyrMirrorObjLatch reserved_obj_latches[15]; /* +0x144 */
    union {
        PlyrMirrorObjLatch held_by_object_latch;
        struct {
            int held_by_player;
            int hold_state;
        };
    }; /* +0x1BC */
    int (*aux_update_callback)(void); /* +0x1C4 */
    PlyrWeaponImpactData* weapon_impact; /* +0x1C8 */
    union {
        void* pchr;
        int character_id;
    }; /* +0x1CC */
    int plyr_num;   /* +0x1D0 */
    PlyrMirrorBoneMap* mirror_bone_map; /* +0x1D4 */
    char pad1D8[4];
    int collision_result;        /* +0x1DC */
    int hit_flash_enabled;         /* +0x1E0 */
    int blocking_disabled;       /* +0x1E4 */
    int blocking_disabled_2;     /* +0x1E8 */
    unsigned int blocking_disable_tick_1; /* +0x1EC */
    unsigned int blocking_disable_tick_2; /* +0x1F0 */
    union {
        int input_unlock_tick;
        unsigned int action_lock_a;
    }; /* +0x1F4 */
    union {
        int attacks_disabled_until;
        unsigned int action_lock_b;
    }; /* +0x1F8 */
    int attack_disable_tick_p1; /* +0x1FC */
    int attack_disable_tick_p2; /* +0x200 */
    int dodge_sound_played; /* +0x204 */
    char pad208[4];
    PlyrDisabledMove disabled_moves[4]; /* +0x20C */
    int special_move_disabled; /* +0x22C */
    int f_constrained; /* +0x230 */
    int field_234;
    int breaker_strength; /* +0x238 - MK Chess fight setup */
    int throw_restriction; /* +0x23C */
    int secondary_state; /* +0x240 */
    int state;          /* +0x244 */
    int previous_state; /* +0x248 */
    union {
        SwitchData* switch_data;
        int controller_port;
    }; /* +0x24C */
    int hit_count; /* +0x250 */
    int reaction_hit_count; /* +0x254 */
    int hit_streak; /* +0x258 */
    int combo_hit_count; /* +0x25C */
    int combo_flags; /* +0x260 */
    int attack_counter; /* +0x264 */
    int shared_attack_until; /* +0x268 */
    int field_26C;
    char pad270[4];
    unsigned int last_collision_tick; /* +0x274 */
    int last_back_dash_tick; /* +0x278 - switch double-tap timing */
    char pad27C[8];
    unsigned int damage_boost_until; /* +0x284 */
    char pad288[4];
    float taunt_life_scale; /* +0x28C */
    float postround_value; /* +0x290 */
    float combo_damage; /* +0x294 */
    float damage_multiplier; /* +0x298 */
    int his_attack_counter; /* +0x29C */
    int script_exit_value_int; /* +0x2A0 */
    int script_exit_args[2]; /* +0x2A4 */
    union {
        int script_exit_arg_2;
        int duck_loop_counter;
    }; /* +0x2AC */
    int fatality_advance; /* +0x2B0 */
    int facial_damage_complete; /* +0x2B4 */
    float summon_position_x; /* +0x2B8 */
    float summon_position_z; /* +0x2BC */
    float summon_position_y; /* +0x2C0 */
    int drone_request; /* +0x2C4 */
    int drone_handoff_pending; /* +0x2C8 */
    int collision_disabled; /* +0x2CC */
    int duck_wait_ticks; /* +0x2D0 */
    char pad2D4[8];
    unsigned int block_start_tick; /* +0x2DC */
    int opponent_attack_counter; /* +0x2E0 */
    int field_2E4;
    int push_blocked; /* +0x2E8 */
    int field_2EC;
    union {
        int reaction_counter;
        int opponent_attack_counter_copy;
    }; /* +0x2F0 */
    union {
        int block_requirement;
        int pending_hit_strength;
    }; /* +0x2F4 */
    int pending_reaction; /* +0x2F8 */
    PlyrWeaponStyle* weapon_styles[3]; /* +0x2FC */
    int player_slot; /* +0x308 */
    union {
        PlyrFighterDefinition* fighter_definition;
        GlobalMoveset* global_moveset;
    }; /* +0x30C */
    PlyrMirrorSlots* mirror_slots; /* +0x310 */
    union {
        PlyrMoveDisplayData* active_move_display;
        unsigned int fighter_definition_instance;
        MovesetDefinition* global_moveset_definition;
    }; /* +0x314 */
    int animation_data[8]; /* +0x318 - base animation destination */
    AniData* dizzy_animation; /* +0x338 */
    char pad33C[4];
    AniData* big_boss_taunt_animation; /* +0x340 */
    char pad344[4];
    AniData* turn_to_screen_animation; /* +0x348 */
    char pad34C[0x1C];
    AniData* reaction_animation; /* +0x368 */
    void* reaction_animation_a; /* +0x36C */
    void* reaction_animation_b; /* +0x370 */
    void* reaction_animation_c; /* +0x374 */
    void* esp1_reaction_animation; /* +0x378 */
    union {
        AniData* screen_taunt_animation;
        void* scorpion_spear_hit;
    }; /* +0x37C - per-character reaction slot */
    void* scorpion_spear_pull; /* +0x380 */
    union {
        AniData* goro_fold_animation; /* fatality arm-fold script */
        void* scorpion_spear_recover;
    }; /* +0x384 - per-character reaction slot */
    char pad388[4];
    AniData* ice_reaction_animation; /* +0x38C */
    char pad390[0x14];
    union {
        AniData* fatality_animation;
        int fatality_palette;
    }; /* +0x3A4 */
    union {
        struct {
            union {
                AniData* mileena_veil_animation;
                unsigned int suicide_camera_main_ntsc;
            }; /* +0x3A8 */
            char pad3AC[4];
            unsigned int suicide_camera_sidekick_ntsc; /* +0x3B0 */
            unsigned int suicide_camera_main_pal; /* +0x3B4 */
            unsigned int suicide_camera_sidekick_pal; /* +0x3B8 */
            char pad3BC[0x18];
            unsigned int fatality_camera_ntsc; /* +0x3D4 */
            unsigned int fatality_camera_pal;  /* +0x3D8 */
        };
        unsigned int fatality_camera_scripts[13];
    };
    AnimScript* face_animations[37]; /* +0x3DC */
    MkObj* shadowbox; /* +0x470 */
    char pad474[4];
    ScriptSlot* cmo; /* +0x478 */
    unsigned char large_blood_spawn_state[0x10]; /* +0x47C */
    void* blood_model_data; /* +0x48C - enables per-player blood emitters */
    char pad490[0x1C];
    unsigned char left_blood_spawn_state[0x8C];  /* +0x4AC */
    unsigned char right_blood_spawn_state[0x70]; /* +0x538 */
    unsigned int next_large_bleed_tick; /* +0x5A8 */
    unsigned int next_blood_glop_tick;  /* +0x5AC */
    int duck_reaction_active; /* +0x5B0 */
    float saved_position_x; /* +0x5B4 */
    float saved_position_y; /* +0x5B8 */
    float saved_position_z; /* +0x5BC */
    char pad5C0[0x2C];
    unsigned int saved_anim_script_word; /* +0x5EC */
    unsigned int saved_anim_flags;       /* +0x5F0 */
    char pad5F4[8];
    float saved_anim_low_frame;          /* +0x5FC */
    float saved_anim_high_frame;         /* +0x600 */
    char pad604[0xC0];
    float facial_damage; /* +0x6C4 */
    int attack_region; /* +0x6C8 */
    int attack_type; /* +0x6CC */
    unsigned int round_attack_count; /* +0x6D0 - AI round pressure */
    int round_attack_stage; /* +0x6D4 */
    int combo_depth; /* +0x6D8 */
    int field_6DC;
    int strafe_direction; /* +0x6E0 */
    int block_hit_count; /* +0x6E4 */
    MslSoundHandle scream_sound_handle; /* +0x6E8 */
    int repeated_action_count; /* +0x6EC */
    unsigned int previous_action; /* +0x6F0 */
    int death_type; /* +0x6F4 */
    union {
        unsigned int* status_flags;
        PlyrStatusData* status_data;
        FighterRuntimeData* runtime_data;
    }; /* +0x6F8 */
    int fatality_shove_active; /* +0x6FC */
    struct MkProc* jaw_monitor;           /* +0x700 */
    unsigned int jaw_monitor_instance;    /* +0x704 */
    struct MkProc* baraka_blades_monitor; /* +0x708 */
    unsigned int baraka_blades_monitor_instance; /* +0x70C */
    unsigned int reserved_710[2];
    int (*baraka_moveset_callback)(
        struct PlyrPdata*, PlyrMirrorSlots*); /* +0x718 */
    void* active_pickup; /* +0x71C */
    int online_sync_index;            /* +0x720 - -1 when unavailable */
    int impaled_projectile_state;       /* +0x724 */
    int field_728;
    MkObj* sidekick_obj;              /* +0x72C */
    unsigned int sidekick_instance;   /* +0x730 */
    struct MkProc* sidekick_anim_proc; /* +0x734 */
    unsigned int sidekick_anim_instance; /* +0x738 */
    int sidekick_active;              /* +0x73C */
    int sidekick_available;           /* +0x740 */
    int angle_jump_pending; /* +0x744 */
    int taunts_performed;             /* +0x748 */
} PlyrPdata; /* 0x74C */

#define PLYR_PDATA_SIZE 0x74C
#define PLYR_PDATA_POOL_COUNT 2

#ifndef PLYR_PDATA_TYPES_ONLY
PlyrPdata* get_mkpdata_plyr(void);
void init_mkpdata_plyrs(void);

int plyr_pdata_get_previous_state(PlyrPdata* pdata);
int plyr_pdata_get_state(PlyrPdata* pdata);
void* plyr_pdata_get_pchr(PlyrPdata* pdata);
ScriptSlot* plyr_pdata_get_cmo(PlyrPdata* pdata);
int plyr_pdata_get_plyr_num(PlyrPdata* pdata);
MkObj* plyr_pdata_get_his_obj(PlyrPdata* pdata);
/* Returns plyr_info->slot.mirror_a (+0x5C), despite the retail name. */
void* plyr_pdata_get_plyr_obj(PlyrPdata* pdata);
PlyrPdata* plyr_pdata_get_his_plyr_pdata(PlyrPdata* pdata);
PlyrInfo* plyr_pdata_get_plyr_info(PlyrPdata* pdata);
PlyrPdata* get_my_plyr_pdata(void);
PlyrPdata* get_his_plyr_pdata(void);
PlyrPdata* get_plyr_pdata_plyr_num(int player);
MkObj* get_plyr_obj_plyr_num(int player);
int get_my_plyr_num(void);
MkObj* get_my_plyr_obj(void);
MkObj* get_his_plyr_obj(void);
PlyrInfo* get_plyr_info(void);
void cleanup_player_globals(void);
int get_my_particle_player_bank_num(void);
int plyr_pdata_sidekick_active(PlyrPdata* pdata);
MkObj* plyr_pdata_get_sidekick_obj(PlyrPdata* pdata);
MkObj* get_my_sidekick_obj(void);
int is_sidekick_active(PlyrInfo* player);
float player_sleep_forever(void);
void set_attack_type(int attack_type);

extern PlyrPdata* plyr_pdata;
extern PlyrPdata* free_mkpdata_plyrs;
#endif

#endif
