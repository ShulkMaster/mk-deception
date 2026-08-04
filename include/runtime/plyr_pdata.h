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

typedef struct PlyrMoveBlendData {
    char pad00[0x28];
    float step;   /* +0x28 */
    float weight; /* +0x2C */
} PlyrMoveBlendData;

typedef struct PlyrWeaponImpactData {
    char pad00[0x20];
    int attack_region; /* +0x20 */
} PlyrWeaponImpactData;

typedef struct PlyrMoveDisplayData {
    char pad00[0x10];
    int display_width; /* +0x10 */
} PlyrMoveDisplayData;

typedef struct PlyrFighterDefinition {
    char pad00[4];
    PlyrMoveBlendData* move_blend_data; /* +0x04 */
    ScriptSlot* cmo; /* +0x08 */
    char pad0C[0x68];
    AniData* duck_exit_animation; /* +0x74 */
    union {
        AniData* forced_step_animation; /* +0x78 */
        AniData* walk_forward_start;
    };
    AniData* walk_backward_start; /* +0x7C */
    char pad80[8];
    AniData* walk_forward_loop; /* +0x88 */
    AniData* walk_backward_loop; /* +0x8C */
    char pad90[0x1C];
    AniData* duck_block_animation; /* +0xAC */
    char padB0[0x14];
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

typedef struct PlyrStateFlagBits {
    unsigned char pad_bit7 : 1;
    unsigned char frozen : 1; /* bit6 - freeze-light lifetime */
    unsigned char pad_bit5 : 1;
    unsigned char dizzy : 1; /* bit4 - held in the puzzle dizzy state */
    unsigned char pad_bits3_2 : 2;
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

typedef struct PlyrWeaponMirrorSlot {
    PlyrMirrorObjLatch primary;
    PlyrMirrorObjLatch mirror; /* +0x08 */
    PlyrMirrorObjLatch secondary; /* +0x10 */
} PlyrWeaponMirrorSlot; /* 0x18 */

typedef struct PlyrMirrorSlots {
    PlyrWeaponMirrorSlot weapon[4];
} PlyrMirrorSlots; /* 0x60 */

typedef struct PlyrWeaponStyle {
    char pad00[0x0C];
    PlyrMirrorSlots mirror_slots; /* +0x0C */
    char pad6C[0x30];
    MkPtr* object_list; /* +0x9C - owns style weapon/reflection objects */
} PlyrWeaponStyle;

typedef struct PlyrPdata {
    void* vtbl; /* +0x00 - vtbl_mkpdata_plyr; free-list next when idle */
    unsigned int instance; /* +0x04 */
    struct MkProc* player_proc; /* +0x08 */
    unsigned int player_proc_instance; /* +0x0C */
    struct PlyrPdata* his_plyr_pdata; /* +0x10 */
    MkObj* his_obj;                   /* +0x14 */
    PlyrInfo* plyr_info;              /* +0x18 - GameInfo plyr0/plyr1 */
    PlyrStateFlags state_flags; /* +0x1C */
    char pad1D[0x13];
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
    char pad74[0x40];
    struct MkProc* transient_proc; /* +0xB4 */
    unsigned int transient_proc_instance; /* +0xB8 */
    char padBC[0x20];
    PlyrMirrorObjLatch impaled_item_a;           /* +0xDC */
    PlyrMirrorObjLatch impaled_item_b;           /* +0xE4 */
    PlyrMirrorObjLatch impaled_item_a_secondary; /* +0xEC */
    PlyrMirrorObjLatch impaled_item_b_secondary; /* +0xF4 */
    char padFC[4];
    struct MkProc* spear_proc; /* +0x100 */
    unsigned int spear_proc_instance; /* +0x104 */
    char pad108[0x0C];
    AniTextureControlItem facial_texture; /* +0x114 */
    char pad11C[0xA0];
    int held_by_player; /* +0x1BC */
    int hold_state; /* +0x1C0 */
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
    char pad26C[0x0C];
    int last_back_dash_tick; /* +0x278 - switch double-tap timing */
    char pad27C[0x14];
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
    char pad2B0[4];
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
    int block_requirement; /* +0x2F4 */
    char pad2F8[4];
    PlyrWeaponStyle* weapon_styles[3]; /* +0x2FC */
    int player_slot; /* +0x308 */
    PlyrFighterDefinition* fighter_definition; /* +0x30C */
    PlyrMirrorSlots* mirror_slots; /* +0x310 */
    PlyrMoveDisplayData* active_move_display; /* +0x314 */
    char pad318[0x28];
    AniData* big_boss_taunt_animation; /* +0x340 */
    char pad344[4];
    AniData* turn_to_screen_animation; /* +0x348 */
    char pad34C[0x1C];
    AniData* reaction_animation; /* +0x368 */
    void* reaction_animation_a; /* +0x36C */
    void* reaction_animation_b; /* +0x370 */
    void* reaction_animation_c; /* +0x374 */
    char pad378[4];
    AniData* screen_taunt_animation; /* +0x37C */
    char pad380[4];
    AniData* goro_fold_animation; /* +0x384 - fatality arm-fold script */
    char pad388[0xE8];
    MkObj* shadowbox; /* +0x470 */
    char pad474[4];
    ScriptSlot* cmo; /* +0x478 */
    unsigned char large_blood_spawn_state[0x10]; /* +0x47C */
    void* blood_model_data; /* +0x48C - enables per-player blood emitters */
    char pad490[0x1C];
    unsigned char left_blood_spawn_state[0x8C];  /* +0x4AC */
    unsigned char right_blood_spawn_state[0x70]; /* +0x538 */
    unsigned int next_large_bleed_tick; /* +0x5A8 */
    char pad5AC[4];
    int duck_reaction_active; /* +0x5B0 */
    float saved_position_x; /* +0x5B4 */
    char pad5B8[4];
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
    char pad6DC[8];
    int block_hit_count; /* +0x6E4 */
    MslSoundHandle scream_sound_handle; /* +0x6E8 */
    int repeated_action_count; /* +0x6EC */
    unsigned int previous_action; /* +0x6F0 */
    int death_type; /* +0x6F4 */
    unsigned int* status_flags; /* +0x6F8 - is_blind / is_big_boss / etc. */
    int fatality_shove_active; /* +0x6FC */
    struct MkProc* jaw_monitor;           /* +0x700 */
    unsigned int jaw_monitor_instance;    /* +0x704 */
    struct MkProc* baraka_blades_monitor; /* +0x708 */
    unsigned int baraka_blades_monitor_instance; /* +0x70C */
    char pad710[8];
    int (*baraka_moveset_callback)(void*, void*); /* +0x718 */
    char pad71C[4];
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
