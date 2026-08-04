/*
 * Port readiness:
 *   Structs: PARTIAL
 *   Fuzzy: 16.98% (.text)
 *   Linked: NO
 *   Status: SCAFFOLD
 *   Gaps: attacks, reactions, movement, presentation, and scripts remain
 */

#include "runtime/plyr_pdata.h"
#include "game/pz_fatality.h"

#define PZ_ENGINE_SPECIAL_MOVE_4 0x40
#define PZ_ENGINE_SPECIAL_MOVE_5 0x20
#define PZ_ENGINE_EASY_CONTINUATION 0x02
#define PZ_ENGINE_CONTINUATION_RESET 0x04
#define PZ_ENGINE_CONTINUATION_ALLOWED 0x08
#define PZ_ENGINE2_CONTINUATION_BLOCKED 0x80

typedef float (*PuzzleMoveEntry)(void);
typedef float (*PuzzleProcessTransfer)(PuzzleMoveEntry entry, float delay);
typedef float (*PuzzleProcessTransferWithExit)(
    PuzzleMoveEntry entry, int exit_value, float delay);
typedef float (*PuzzleProcessSleep)(void);
typedef float (*PuzzleFighterFunction)(void);
typedef struct ScriptSlot ScriptSlot;

typedef struct PuzzleCmdScript {
    char pad00[0x28];
    int shared_function; /* +0x28 */
} PuzzleCmdScript;

typedef struct PuzzleProcessVtable {
    char pad00[0x18];
    PuzzleProcessSleep sleep; /* +0x18 */
    char pad1C[8];
    union {
        PuzzleProcessTransfer transfer; /* +0x24 */
        PuzzleProcessTransferWithExit transfer_with_exit;
    };
} PuzzleProcessVtable;

typedef struct PuzzleProcess {
    PuzzleProcessVtable* vtbl;
} PuzzleProcess;

typedef struct PuzzlePresentState {
    char pad00[8];
    int state; /* +0x08 */
    PlyrPdata* owner; /* +0x0C */
} PuzzlePresentState;

typedef struct PuzzleFighterMove {
    char pad00[0x20];
    unsigned int active_flags; /* +0x20 */
} PuzzleFighterMove;

typedef struct PuzzleFightersEngine {
    char pad00[0x74];
    int peak_mode; /* +0x74 */
    int special_move_enabled; /* +0x78 */
    char pad7C[0x0C];
    int peak_active; /* +0x88 */
    unsigned char flags; /* +0x8C */
    unsigned char flags2; /* +0x8D */
    char pad8E[0xD6];
    int breakout; /* +0x164 */
    char pad168[0x54];
    PuzzlePresentState* present; /* +0x1BC */
} PuzzleFightersEngine;

typedef struct PuzzleProjectile {
    char pad00[0x10];
    int state; /* +0x10 */
} PuzzleProjectile;

typedef struct PuzzleFighterObject {
    char pad00[8];
    union {
        unsigned char flags_08; /* +0x08 */
        struct {
            unsigned char pad08_high : 7;
            unsigned char gravity_enabled : 1; /* bit0 */
        };
    };
    union {
        unsigned char flags_09; /* +0x09 */
        struct {
            unsigned char pad_high : 4;
            unsigned char reaction_locked : 1; /* bit3 */
            unsigned char pad_low : 3;
        } action_flags;
        struct {
            unsigned char pad_high : 6;
            unsigned char unk_bit1 : 1;
            unsigned char pad_low : 1;
        } presentation_flags;
    };
    char pad0A[0xA6];
    float external_force_x; /* +0xB0 */
    float vertical_velocity; /* +0xB4 */
    float external_force_z; /* +0xB8 */
    char padBC[0x18];
    float angle_y; /* +0xD4 */
} PuzzleFighterObject;

typedef AniData PuzzleAnimation;
typedef struct PuzzleAnimPdata {
    char pad00[0x44];
    float step; /* +0x44 */
} PuzzleAnimPdata;

typedef struct PuzzleSharedCombatAnimations {
    char pad000[0x30];
    PuzzleAnimation* step_throw; /* +0x30 */
    char pad034[0x278];
    PuzzleAnimation* block_high; /* +0x2AC */
    char pad2B0[0x7C];
    PuzzleAnimation* dizzy; /* +0x32C */
} PuzzleSharedCombatAnimations;

typedef struct PuzzleReactionDelayPdata {
    char header[8];
    int ticks; /* +0x08 */
    int reaction; /* +0x0C */
    void* saved_pdata; /* +0x10 */
} PuzzleReactionDelayPdata;

typedef struct PuzzleCameraShakePdata {
    char header[8];
    int duration; /* +0x08 */
    float strength; /* +0x0C */
} PuzzleCameraShakePdata;

typedef struct PuzzleSharedAnimations {
    char pad000[0x10];
    PuzzleAnimation* dizzy_punch; /* +0x10 */
    char pad014[4];
    PuzzleAnimation* double_arm_victory; /* +0x18 */
    char pad01C[8];
    PuzzleAnimation* uppercut_brush_back; /* +0x24 */
    char pad028[0x10];
    PuzzleAnimation* backflip; /* +0x38 */
    char pad03C[0x14];
    PuzzleAnimation* shove; /* +0x50 */
    PuzzleAnimation* won2; /* +0x54 */
    char pad058[0x18];
    PuzzleAnimation* gaydance; /* +0x70 */
    PuzzleAnimation* whatever2; /* +0x74 */
    PuzzleAnimation* workthecrowd_start; /* +0x78 */
    PuzzleAnimation* workthecrowd_loop; /* +0x7C */
    PuzzleAnimation* go_get_him; /* +0x80 */
    char pad084[0x0C];
    PuzzleAnimation* dont_get_me; /* +0x90 */
    PuzzleAnimation* taunt1; /* +0x94 */
    PuzzleAnimation* taunt2; /* +0x98 */
    PuzzleAnimation* taunt3; /* +0x9C */
    char pad0A0[0x48];
    PuzzleAnimation* fast_look; /* +0xE8 */
    PuzzleAnimation* one_arm_swing; /* +0xEC */
    char pad0F0[0x3C];
    PuzzleAnimation* wipe_blood; /* +0x12C */
    PuzzleAnimation* disgusted_with_grinding; /* +0x130 */
    PuzzleAnimation* round_ground_pound; /* +0x134 */
    PuzzleAnimation* round_whew; /* +0x138 */
    PuzzleAnimation* wtf2; /* +0x13C */
    PuzzleAnimation* wtf; /* +0x140 */
    char pad144[0x0C];
    PuzzleAnimation* backflip_point; /* +0x150 */
    char pad154[0x14];
    PuzzleAnimation* round_failure; /* +0x168 */
    char pad16C[4];
    PuzzleAnimation* bow_warmup; /* +0x170 */
    char pad174[0x30];
    PuzzleAnimation* active_warmup1; /* +0x1A4 */
    PuzzleAnimation* active_warmup2; /* +0x1A8 */
    PuzzleAnimation* showoff_warmup1; /* +0x1AC */
    PuzzleAnimation* showoff_warmup2; /* +0x1B0 */
    char pad1B4[8];
    PuzzleAnimation* superman; /* +0x1BC */
    char pad1C0[8];
    PuzzleAnimation* propell_start; /* +0x1C8 */
    PuzzleAnimation* propell_air; /* +0x1CC */
    PuzzleAnimation* propell_end; /* +0x1D0 */
    char pad1D4[0x1C];
    PuzzleAnimation* peak; /* +0x1F0 */
    char pad1F4[0x3C];
    PuzzleAnimation* round_victory; /* +0x230 */
} PuzzleSharedAnimations;

extern PuzzleFightersEngine g_pz_fighters_engine;
extern PuzzleProcess* aproc;
extern PuzzleProcess* plyr_anim_proc;
extern PuzzleProjectile* g_global_projectile;
extern PuzzleFighterObject* plyr_obj;
extern PuzzleFighterObject* his_obj;
extern PuzzleAnimPdata* plyr_anim_pdata;
extern PlyrPdata* his_pdata;
extern PuzzleSharedAnimations pz_shared_ani;
extern PuzzleSharedCombatAnimations shared_ani;
extern ScriptSlot* pz_shared_cmo;
extern PuzzleCmdScript* active_cmdscript;
extern void* apdata;
extern float _mkproc_sleep_ticks;
extern int g_pz_cam_already_shaking;

float pz_fighter_laugh(void);
float pz_fighter_whatever2(void);
float pz_fighter_beg(void);
float pz_fighter_workthecrowd(void);
float pz_fighter_select_taunt_1(void);
float pz_fighter_select_taunt_2(void);
float pz_fighter_select_taunt_3(void);
float pz_fighter_peak(void);
float pz_fighter_fast_look(void);
float pz_fighter_big_time_happy(void);
float pz_fighter_round_whew(void);
float pz_fighter_WTF(void);
float pz_fighter_WTF2(void);
float pz_fighter_execute_distracts_and_hit(void);
float pz_fighter_execute_point_reaction_no_space(void);
float pz_fighter_far_propell(void);
float pz_fighter_go_get_him(void);
float pz_fighter_one_arm_swing(void);
float pz_fighter_gaydance(void);
float pz_fighter_dont_get_me(void);
float pz_fighter_light_propell(void);
float pz_fighter_superman_move(void);
float pz_fighter_propell(void);
float r_pz_call_script_function(void);
float pz_fighter_one_arm_victory(void);
float pz_fighter_one_arm_victory2(void);
float pz_fighter_double_arm_victory(void);
void p_anim_idle(void);
void set_my_state(int state);
float p_plyr_pz_fighter_entry(void);
float pz_fighter_exit(void);
float pz_fighter_long_exit(void);
float j_exit(void);
float j_exit_6(void);
void face_opponent_now(void);
void head_tracking_off(void);
void head_tracking_on(void);
void cmdscript_reset_stack(void);
void cmdscript_setup_execution(ScriptSlot* slot, unsigned int function);
float call_player_script_function(ScriptSlot* slot);
void toggle_obj_and_ani_flips(PuzzleAnimPdata* animation);
void release_other_player(void);
void pz_fighter_reaction_xfer_him(int reaction);
float p_force_reaction(void);
float p_pz_shake_camera(void);
void* _create_mkproc_generic_tinystack(
    int pid, int priority, float (*entry)(void), int pdata_size,
    void* pdata_out);
void xfer_proc();
void set_ani_weight(float weight);
void blend_to_ani(PuzzleAnimation* animation, int flags, float blend);
void set_ani_speed(float speed);
void stop_me();
void init_ground_move_no_aniproc(void);
void init_ground_move(void);
void init_air_move(void);
void ani_loop_more_frames(float frames);
void ani_1_frame(void);
void pz_fighter_set_y_constrain(
    PuzzleFighterObject* fighter, int enabled, float height);
void pz_fighter_dont_fudge_desired_distance(void);
void pz_fighter_startup_attack(
    PuzzleAnimation* animation, unsigned int start_flags,
    unsigned int attack_flags, int blend_flags, int reaction,
    float attack_frame, float blend, float speed, float force, float damping);
void player_feet_land_chores(void);
void random_hit(int sound);
void random_voice(int sound);
void snd_req(int sound);
void glitch_to_ani(PuzzleAnimation* animation, int frame);
void p_animate(void);
void pz_fighter_ani_attack(
    int attack, int reaction, float active_frame, float hit_frame,
    float damage);
void set_both_face_opponent_flags(void);
void got_hit_fx();
void myvel_his_angle_y(float y, float x, float z);
void init_air_move_no_aniproc(void);
void update_bone_hierarchy(void* object);
void ground_me(void* object);
void rotate_towards_him(float rate);
int get_his_attack_counter(void);
void force_forward();
void nudge_towards_him(float distance);
void ani_to_blend_frame(float frame);
void ani_to_frame_x(float frame);
void blend_to_stance(float blend);
void plyr_bleed_medium_cycle(PlyrPdata* pdata, int bone);
void force_away();
void pz_fighter_attack(
    PuzzleAnimation* animation, PuzzleAttackParameters* attack, int reaction);
PuzzleFighterMove* pz_get_fighter_move(void);
void slow_ani_x(float speed, float frame);
void ani_to_end(void);
void pz_fighter_check_breakout(void);
float xz_distance_between_players(void);
int pz_fighter_close_enough_to_super_move(int player);
int pz_fighter_is_winning_big(int player);
int pz_fighter_is_losing_big(int player);
PuzzleProcess* get_player_proc(PuzzleFighterObject* fighter);
float pz_fighter_fetch_plyr_to_home_post_distance(int player);
void pz_fighters_calc_distance_to_desired_idle_pos_abs(
    float* player1_distance, float* player2_distance,
    float* player1_absolute, float* player2_absolute);
void pz_fighters_calc_distance_to_desired_idle_pos(
    float* player1_distance, float* player2_distance);
unsigned int randu0(unsigned int max);
void shake_camera(int duration, float strength);
void minigame_get_bgnd_y_value(int* first, int* second);
void minigame_set_bgnd_y_value(int first, int second);
float pz_fighter_present_explode(void);
float pz_fighter_present_given(void);
float pz_fighter_present_on_attackers_hand(void);
float p_present_control(void);
static float pz_fighter_scorpion_attack_start(void);
static float pz_fighter_jax_attack_start(void);
static float r_call_character_cmo_function(void);

PuzzleFighterFunction pz_fighter_tbl[3] = {
    pz_fighter_present_on_attackers_hand,
    pz_fighter_present_given,
    pz_fighter_present_explode,
};

int pz_fighter_should_handle_special_move(void* fighter, unsigned int move) {
    (void)fighter;

    if (move == 4) {
        g_pz_fighters_engine.flags |= PZ_ENGINE_SPECIAL_MOVE_4;
    } else if (move == 5) {
        g_pz_fighters_engine.flags |= PZ_ENGINE_SPECIAL_MOVE_5;
    } else if (move == 1) {
        return 1;
    }
    return 0;
}

float pz_fighter_perform_special_move(void) {
    PuzzleMoveEntry entry;
    int script_function;

    entry = pz_fighter_exit;
    script_function = -1;
    if (g_pz_fighters_engine.special_move_enabled == 1) {
        switch (plyr_pdata->character_id) {
        case 0:
            entry = pz_fighter_scorpion_attack_start;
            break;
        case 1:
            script_function = 0x0D;
            break;
        case 2:
        case 6:
        case 7:
        case 9:
        case 11:
        case 19:
        case 33:
            entry = pz_fighter_jax_attack_start;
            break;
        case 3:
            script_function = 0x13;
            break;
        case 4:
        case 10:
            script_function = 0x0F;
            break;
        case 5:
            script_function = 0x17;
            break;
        case 8:
            script_function = 0x11;
            break;
        case 12:
            script_function = 0x12;
            break;
        case 21:
            script_function = 0x16;
            break;
        case 23:
            script_function = 0x18;
            break;
        case 29:
            script_function = 0x14;
            break;
        default:
            break;
        }
    }

    if (script_function >= 0) {
        active_cmdscript->shared_function = script_function;
        entry = r_call_character_cmo_function;
    }
    aproc->vtbl->transfer(entry, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_finish_him_request ~98.46% - emit-order island. */
float pz_finish_him_request(void) {
    if (xz_distance_between_players() > 2.0f) {
        aproc->vtbl->transfer(pz_fighter_far_propell, 0.0f);
    } else {
        active_cmdscript->shared_function = 0x2B;
        cmdscript_reset_stack();
        cmdscript_setup_execution(
            pz_shared_cmo, active_cmdscript->shared_function);
        call_player_script_function(pz_shared_cmo);
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    }
    return 0.0f;
}

float pz_fighters_react_to_bomb_explosion(void) {
    if (pz_fighter_fetch_plyr_to_home_post_distance(
            ((PlyrPdata*)plyr_pdata)->plyr_num) >
        6.5f) {
        active_cmdscript->shared_function = 0x44;
    } else {
        active_cmdscript->shared_function = 0x45;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_perform_ohyeah_move ~90.47% - NV coloring. */
float pz_fighter_perform_ohyeah_move(void) {
    unsigned short choice = randu0(100);

    if (choice < 50) {
        aproc->vtbl->transfer(pz_fighter_one_arm_swing, 0.0f);
    } else if (choice < 70) {
        aproc->vtbl->transfer(pz_fighter_big_time_happy, 0.0f);
    } else {
        aproc->vtbl->transfer(pz_fighter_gaydance, 0.0f);
    }
    return 0.0f;
}

/* Soft ceiling: pz_fighter_perform_super_move_just_enabled ~90.64% - NV coloring. */
float pz_fighter_perform_super_move_just_enabled(void) {
    unsigned short choice = randu0(100);

    if (choice < 50) {
        aproc->vtbl->transfer(pz_fighter_go_get_him, 0.0f);
    } else if (choice < 60) {
        aproc->vtbl->transfer(pz_fighter_one_arm_swing, 0.0f);
    } else if (choice < 75) {
        aproc->vtbl->transfer(pz_fighter_big_time_happy, 0.0f);
    } else {
        aproc->vtbl->transfer(pz_fighter_gaydance, 0.0f);
    }
    return 0.0f;
}

float pz_fighter_perform_other_guy_super_move_just_enabled(void) {
    aproc->vtbl->transfer(pz_fighter_dont_get_me, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_ohno_move(void) {
    unsigned short choice = randu0(100);

    if (choice < 30) {
        aproc->vtbl->transfer(pz_fighter_WTF, 0.0f);
    } else if (choice < 70) {
        aproc->vtbl->transfer(pz_fighter_WTF2, 0.0f);
    } else if (
        choice < 88 &&
        pz_fighter_is_losing_big(plyr_pdata->plyr_num) != 0) {
        aproc->vtbl->transfer(pz_fighter_beg, 0.0f);
    } else {
        xfer_proc(
            get_player_proc(his_obj), pz_fighter_execute_distracts_and_hit);
        aproc->vtbl->transfer(
            pz_fighter_execute_point_reaction_no_space, 0.0f);
    }
    return 0.0f;
}

/* Soft ceiling: center_pos_minor_adjustement ~90.91% - branch scheduling. */
float pz_fighter_perform_center_pos_minor_adjustement(void) {
    if (xz_distance_between_players() > 1.2f) {
        aproc->vtbl->transfer(pz_fighter_light_propell, 0.0f);
    } else {
        active_cmdscript->shared_function = 8;
        aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    }
    return 0.0f;
}

float pz_fighter_perform_center_pos_single_close_move(void) {
    aproc->vtbl->transfer(pz_fighter_light_propell, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_smart_flippy ~99.94% - pool label only. */
float pz_fighter_smart_flippy(void) {
    float player1_distance;
    float player2_distance;
    float player1_absolute;
    float player2_absolute;
    float distance;

    pz_fighters_calc_distance_to_desired_idle_pos_abs(
        &player1_distance, &player2_distance, &player1_absolute,
        &player2_absolute);
    if (plyr_pdata->plyr_num == 0) {
        distance = player1_distance;
    } else {
        distance = player2_distance;
    }

    if (distance < 0.1f) {
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
        return 0.0f;
    }
    if (distance < 0.9f) {
        active_cmdscript->shared_function = 4;
    } else {
        active_cmdscript->shared_function = 3;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_center_pos_single_range_move(void) {
    if (randu0(100) < 30) {
        aproc->vtbl->transfer(pz_fighter_light_propell, 0.0f);
    } else {
        active_cmdscript->shared_function = 3;
        aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    }
    return 0.0f;
}

/* Soft ceiling: center_pos_range_attack ~91.09% - NV coloring. */
float pz_fighter_perform_center_pos_range_attack(void) {
    unsigned short choice = randu0(100);

    if (choice < 40) {
        aproc->vtbl->transfer(pz_fighter_superman_move, 0.0f);
    } else if (choice < 55) {
        aproc->vtbl->transfer(pz_fighter_propell, 0.0f);
    } else {
        active_cmdscript->shared_function = 1;
        aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    }
    return 0.0f;
}

float pz_fighter_perform_dist_attack(void) {
    unsigned short choice = randu0(100);

    if (choice < 20) {
        active_cmdscript->shared_function = 2;
    } else if (choice < 60) {
        active_cmdscript->shared_function = 6;
    } else {
        active_cmdscript->shared_function = 7;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

/* Soft ceiling: off_wall_attack ~90.70% - MWCC branch scheduling. */
float pz_fighter_perform_off_wall_attack(void) {
    unsigned short choice = randu0(100);

    if (choice < 40) {
        aproc->vtbl->transfer(pz_fighter_superman_move, 0.0f);
        return 0.0f;
    }
    active_cmdscript->shared_function = 2;
    if (choice < 80) {
        active_cmdscript->shared_function = 1;
    }
    aproc->vtbl->transfer(r_pz_call_script_function, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_other_guy_ohno(void) {
    aproc->vtbl->transfer(pz_fighter_laugh, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_other_guy_ohyeah(void) {
    aproc->vtbl->transfer(pz_fighter_whatever2, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_other_guy_holding_onto_super_move(void) {
    aproc->vtbl->transfer(pz_fighter_beg, 0.0f);
    return 0.0f;
}

float pz_fighter_perform_holding_onto_super_move(void) {
    aproc->vtbl->transfer(pz_fighter_workthecrowd, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_random_taunt ~90.47% - broad policy pass. */
float pz_fighter_random_taunt(void) {
    unsigned short roll = randu0(100);

    if (roll < 30) {
        aproc->vtbl->transfer(pz_fighter_select_taunt_1, 0.0f);
    } else if (roll < 60) {
        aproc->vtbl->transfer(pz_fighter_select_taunt_2, 0.0f);
    } else {
        aproc->vtbl->transfer(pz_fighter_select_taunt_3, 0.0f);
    }
    return 0.0f;
}

/* Soft ceiling: pz_fighter_perform_peak_move ~90.32% - broad policy pass. */
float pz_fighter_perform_peak_move(void) {
    if ((unsigned short)randu0(100) < 65) {
        aproc->vtbl->transfer(pz_fighter_peak, 0.0f);
    } else {
        aproc->vtbl->transfer(pz_fighter_fast_look, 0.0f);
    }
    return 0.0f;
}

/* Soft ceiling: pz_fighter_perform_relief_move ~91.67% - broad policy pass. */
float pz_fighter_perform_relief_move(void) {
    if ((unsigned short)randu0(100) < 65 &&
        pz_fighter_is_winning_big(plyr_pdata->plyr_num)) {
        aproc->vtbl->transfer(pz_fighter_big_time_happy, 0.0f);
    } else {
        aproc->vtbl->transfer(pz_fighter_round_whew, 0.0f);
    }
    return 0.0f;
}

float pz_fighter_dummy_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(20, 4, -0.03f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 20; ticks++) {
        if (xz_distance_between_players() <= 0.65f) {
            plyr_pdata->collision_result = 1;
            stop_me(plyr_pdata);
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(j_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_light_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(20, 4, -0.03f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 20; ticks++) {
        if (xz_distance_between_players() <= 0.65f) {
            plyr_pdata->collision_result = 1;
            pz_fighter_reaction_xfer_him(0x2B);
            stop_me();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(j_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(20, 4, -0.03f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 20; ticks++) {
        if (xz_distance_between_players() <= 0.75f) {
            plyr_pdata->collision_result = 1;
            pz_fighter_reaction_xfer_him(0x29);
            stop_me();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_far_propell(void) {
    int ticks;

    init_ground_move_no_aniproc();
    head_tracking_off();
    blend_to_ani(pz_shared_ani.propell_start, 3, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(10.0f);
    snd_req(0xD7E);
    ani_to_end();
    force_away(30, 4, -0.065f, 0.9f);
    set_ani_speed(1.5f);
    glitch_to_ani(pz_shared_ani.propell_air, 0);
    xfer_proc(plyr_anim_proc, p_animate);
    for (ticks = 0; ticks < 30; ticks++) {
        if (xz_distance_between_players() <= 0.75f) {
            plyr_pdata->collision_result = 1;
            pz_fighter_reaction_xfer_him(0x29);
            stop_me();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.propell_end, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/* pz_fighter_shake_camera: 100% via typed out-pdata and spawn ownership. */
void pz_fighter_shake_camera(int duration, float strength) {
    PuzzleCameraShakePdata* pdata;

    shake_camera(duration, strength);
    if (g_pz_cam_already_shaking == 1) {
        return;
    }

    g_pz_cam_already_shaking = 1;
    if (_create_mkproc_generic_tinystack(
            0x1007, 0x1E, p_pz_shake_camera,
            sizeof(PuzzleCameraShakePdata), &pdata) != 0) {
        pdata->duration = duration;
        pdata->strength = strength;
    }
}

/* Soft ceiling: p_pz_shake_camera ~91.86% - broad background-offset pass. */
float p_pz_shake_camera(void) {
    PuzzleCameraShakePdata* pdata = apdata;
    int first;
    int second;
    int offset;
    int i;

    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    minigame_get_bgnd_y_value(&first, &second);

    for (i = 0; i < pdata->duration; i++) {
        offset = (int)(340.0f * pdata->strength);
        minigame_set_bgnd_y_value(first + offset, second + offset);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        minigame_set_bgnd_y_value(first, second);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    g_pz_cam_already_shaking = 0;
    return -1.0f;
}

int pz_fighter_walk_until_danger_or_in_wrong_direction(void) {
    float player1_distance;
    float player2_distance;
    float distance;

    pz_fighters_calc_distance_to_desired_idle_pos(
        &player1_distance, &player2_distance);
    if (plyr_pdata->plyr_num == 0) {
        distance = player1_distance;
    } else {
        distance = player2_distance;
    }
    if (distance > -0.01f && distance < 0.01f) {
        return 1;
    }
    if (g_pz_fighters_engine.breakout == 1) {
        return 1;
    }
    switch (plyr_pdata->state) {
    case 0x2000:
        return distance < 0.0f;
    case 0x2001:
        return distance > 0.0f;
    default:
        return 0;
    }
}

/* Soft ceiling: pz_fighter_walk_until_fight_distance ~99.58% - pool label only. */
int pz_fighter_walk_until_fight_distance(void) {
    return xz_distance_between_players() < 0.95f;
}

/*
 * Soft ceilings: pz_fighter_showoff_warmup2/1 and
 * pz_fighter_active_warmup2/1 ~99.29%; pz_fighter_bow_warmup ~99.39%.
 * The remaining deltas are float-pool symbol identities only.
 */
float pz_fighter_showoff_warmup2(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.showoff_warmup2, 3, 0.1f);
    set_ani_speed(0.15f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_showoff_warmup1(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.showoff_warmup1, 3, 0.1f);
    set_ani_speed(0.2f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_active_warmup2(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.active_warmup2, 3, 0.1f);
    set_ani_speed(0.2f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_active_warmup1(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    set_ani_weight(0.5f);
    blend_to_ani(pz_shared_ani.active_warmup1, 3, 0.1f);
    set_ani_speed(0.2f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_bow_warmup(void) {
    face_opponent_now();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.bow_warmup, 3, 0.1f);
    set_ani_speed(0.25f);
    ani_to_blend_frame(1.0f);
    blend_to_stance(0.15f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

void pz_fighter_kill_global_projectile(void) {
    if (g_global_projectile != 0) {
        g_global_projectile->state = 2;
    }
}

/* Soft ceiling: pz_fighter_completely_prone ~97.50% - pool label only. */
float pz_fighter_completely_prone(void) {
    return 1.0f;
}

/* Soft ceiling: pz_fighter_won2 ~99.67% - float pool labels only. */
float pz_fighter_won2(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags = 11;
    }

    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.won2, flags, 0.1f);
    set_ani_speed(0.65f);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    ani_to_frame_x(131.0f);
    aproc->vtbl->transfer(pz_fighter_one_arm_victory, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_wipe_blood_off ~99.60% - float pool labels only. */
float pz_fighter_wipe_blood_off(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags = 11;
    }

    xfer_proc(plyr_anim_proc, p_anim_idle);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    blend_to_ani(pz_shared_ani.wipe_blood, flags, 0.1f);
    set_ani_speed(0.75f);
    ani_to_frame_x(22.0f);
    plyr_bleed_medium_cycle(plyr_pdata, 9);
    plyr_bleed_medium_cycle(plyr_pdata, 0x18);
    plyr_bleed_medium_cycle(plyr_pdata, 0x1A);
    ani_to_frame_x(80.0f);
    plyr_bleed_medium_cycle(plyr_pdata, 0x18);
    plyr_bleed_medium_cycle(plyr_pdata, 0x19);
    ani_to_frame_x(128.0f);
    aproc->vtbl->transfer(pz_fighter_double_arm_victory, 0.0f);
    return 0.0f;
}

void pz_fighter_wipe_blood_off_hands(void) {
    plyr_bleed_medium_cycle(plyr_pdata, 0x18);
    plyr_bleed_medium_cycle(plyr_pdata, 0x19);
}

/* Soft ceiling: pz_fighter_double_arm_victory ~99.63% - float pool labels only. */
float pz_fighter_double_arm_victory(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }

    set_my_state(0x4201);
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.double_arm_victory, flags, 0.05f);
    set_ani_speed(0.5f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_whatever2 ~99.43% - float pool labels only. */
float pz_fighter_whatever2(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags = 11;
    }

    xfer_proc(plyr_anim_proc, p_anim_idle);
    force_away(0.01f, 5, 0.9f, 5);
    head_tracking_on();
    blend_to_ani(pz_shared_ani.whatever2, flags, 0.1f);
    set_ani_speed(0.7f);
    ani_to_blend_frame(10.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_disgusted_with_grinding ~99.65% - float pool labels only. */
float pz_fighter_disgusted_with_grinding(void) {
    int flags = 3;

    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }

    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.disgusted_with_grinding, flags, 0.1f);
    set_ani_speed(0.75f);
    plyr_obj->presentation_flags.unk_bit1 = 0;
    ani_to_frame_x(163.0f);
    aproc->vtbl->transfer(pz_fighter_one_arm_victory2, 0.0f);
    return 0.0f;
}

float pz_fighter_round_ground_pound(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    ani_loop_more_frames(10.0f);
    pz_fighter_set_y_constrain(plyr_obj, 1, 0.1f);
    blend_to_ani(pz_shared_ani.round_ground_pound, flags, 0.1f);
    set_ani_speed(0.65f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_WTF2(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    ani_loop_more_frames(10.0f);
    blend_to_ani(pz_shared_ani.wtf2, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_gaydance(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.gaydance, flags, 0.1f);
    set_ani_speed(1.35f);
    ani_to_frame_x(2.0f);
    init_air_move();
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_dont_get_me(void) {
    int flags = 3;

    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.dont_get_me, flags, 0.1f);
    set_ani_speed(0.75f);
    ani_to_blend_frame(20.0f);
    blend_to_stance(0.2f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_go_get_him(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.go_get_him, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_one_arm_swing(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move_no_aniproc();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    blend_to_ani(pz_shared_ani.one_arm_swing, flags, 0.1f);
    set_ani_speed(1.45f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_select_taunt_3(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.taunt3, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_select_taunt_2(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.taunt2, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_select_taunt_1(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.taunt1, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_WTF(void) {
    int flags = 3;

    head_tracking_off();
    stop_me();
    init_ground_move();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    ani_loop_more_frames(10.0f);
    blend_to_ani(pz_shared_ani.wtf, flags, 0.1f);
    set_ani_speed(0.9f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_round_failure(void) {
    unsigned int frame;

    head_tracking_off();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.round_failure, 3, 0.1f);
    set_ani_speed(0.75f);
    for (frame = 0; frame < 32; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += 0.049087387f;
        } else {
            plyr_obj->angle_y -= 0.049087387f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    ani_to_frame_x(154.0f);
    for (frame = 0; frame < 16; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += -0.09817477f;
        } else {
            plyr_obj->angle_y -= -0.09817477f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_workthecrowd(void) {
    int flags = 0;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags = 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.workthecrowd_start, flags, 0.05f);
    set_ani_speed(0.8f);
    blend_to_ani(pz_shared_ani.workthecrowd_loop, flags, 0.1f);
    ani_loop_more_frames(220.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_fast_look(void) {
    int flags = 3;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    init_ground_move_no_aniproc();
    blend_to_ani(pz_shared_ani.fast_look, flags, 0.33f);
    set_ani_speed(2.15f);
    ani_to_blend_frame(5.0f);
    blend_to_stance(0.2f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_peak(void) {
    int flags = 3;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    init_ground_move_no_aniproc();
    blend_to_ani(pz_shared_ani.peak, flags, 0.1f);
    set_ani_speed(1.1f);
    ani_to_frame_x(46.0f);
    g_pz_fighters_engine.peak_active = 1;
    g_pz_fighters_engine.peak_mode = 2;
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_round_whew(void) {
    int flags = 3;

    head_tracking_off();
    if (plyr_pdata->plyr_num == 1) {
        flags |= 8;
    }
    xfer_proc(plyr_anim_proc, p_anim_idle);
    blend_to_ani(pz_shared_ani.round_whew, flags, 0.1f);
    set_ani_speed(0.55f);
    ani_to_blend_frame(10.0f);
    blend_to_stance(0.05f);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_round_victory(void) {
    unsigned int frame;

    xfer_proc(plyr_anim_proc, p_anim_idle);
    head_tracking_off();
    blend_to_ani(pz_shared_ani.round_victory, 3, 0.1f);
    set_ani_speed(0.75f);
    for (frame = 0; frame < 32; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += 0.049087387f;
        } else {
            plyr_obj->angle_y -= 0.049087387f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    ani_to_frame_x(184.0f);
    for (frame = 0; frame < 16; frame++) {
        if (plyr_pdata->plyr_num == 1) {
            plyr_obj->angle_y += -0.09817477f;
        } else {
            plyr_obj->angle_y -= -0.09817477f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_give_present(void) {
    PuzzleReactionDelayPdata* pdata;

    head_tracking_off();
    init_ground_move();
    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_force_reaction,
            sizeof(PuzzleReactionDelayPdata), &pdata) != 0 &&
        pdata != 0) {
        pdata->ticks = 30;
        pdata->reaction = 0x2D;
        pdata->saved_pdata = apdata;
    }
    active_cmdscript->shared_function = 0x37;
    cmdscript_reset_stack();
    cmdscript_setup_execution(
        pz_shared_cmo, active_cmdscript->shared_function);
    call_player_script_function(pz_shared_cmo);
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_distance_check_wo_super_check ~99.69% - pool label only. */
int pz_fighter_distance_check_wo_super_check(void) {
    float distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);

    if (distance < 2.45f) {
        return 1;
    }
    return 2;
}

/* Soft ceiling: pz_fighter_distance_check ~99.82% - pool label only. */
int pz_fighter_distance_check(void) {
    float distance =
        pz_fighter_fetch_plyr_to_home_post_distance(plyr_pdata->plyr_num);

    if (pz_fighter_close_enough_to_super_move(his_pdata->plyr_num) == 1) {
        return 0;
    }
    if (distance < 2.45f) {
        return 1;
    }
    return 2;
}

#define PZ_RUN_SHARED_FIGHTER_SCRIPT(script_index)                              \
    do {                                                                        \
        active_cmdscript->shared_function = (script_index);                     \
        cmdscript_reset_stack();                                                \
        cmdscript_setup_execution(pz_shared_cmo,                                \
                                  active_cmdscript->shared_function);            \
        call_player_script_function(pz_shared_cmo);                             \
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);                           \
    } while (0)

float pz_fighter_execute_point_reaction_no_space(void) {
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x36);
    return 0.0f;
}

float pz_fighter_execute_distracts_and_hit(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x32);
    return 0.0f;
}

float pz_fighter_execute_R_coming_down(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x34);
    return 0.0f;
}

float pz_fighter_execute_point_no_space_check(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x33);
    return 0.0f;
}

float pz_fighter_execute_point(void) {
    head_tracking_off();
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x31);
    return 0.0f;
}

float pz_fighter_perform_taunt(void) {
    PZ_RUN_SHARED_FIGHTER_SCRIPT(0x30);
    return 0.0f;
}

#undef PZ_RUN_SHARED_FIGHTER_SCRIPT

void pz_fighter_check_to_toggle_obj_and_ani_flips(
    unsigned int expected_player) {
    if ((unsigned int)plyr_pdata->plyr_num == expected_player) {
        toggle_obj_and_ani_flips(plyr_anim_pdata);
    }
}

void pz_fighter_release_other_player(int reaction) {
    int reaction_locked = 0;

    release_other_player();
    his_obj->action_flags.reaction_locked = reaction_locked;
    plyr_obj->action_flags.reaction_locked = reaction_locked;
    pz_fighter_reaction_xfer_him(reaction);
}

float pz_fighter_step_throw_into_check(void) {
    init_ground_move();
    random_voice(9);
    random_hit(0xE);
    set_my_state(0x120C);
    blend_to_ani(shared_ani.step_throw, 3, 0.1f);
    plyr_anim_pdata->step = 1.6f;
    ani_to_frame_x(8.0f);
    pz_fighter_ani_attack(0x12, 2, 10.0f, 9.0f, 1.0f);
    set_both_face_opponent_flags();
    return 0.0f;
}

float pz_fighter_backflip_and_point(void) {
    set_my_state(0x4208);
    init_air_move();
    head_tracking_off();
    pz_fighter_dont_fudge_desired_distance();
    pz_fighter_startup_attack(
        pz_shared_ani.backflip, 0x10000, 0x10008, 3, 2, 14.0f,
        0.1f, 0.6f, 0.3f, 0.8f);
    ani_to_frame_x(24.0f);
    player_feet_land_chores();
    init_ground_move();
    ani_to_frame_x(32.0f);
    init_air_move();
    ani_to_frame_x(38.0f);
    init_ground_move();
    ani_to_end();
    set_my_state(0x420A);
    blend_to_ani(pz_shared_ani.backflip_point, 3, 0.1f);
    set_ani_speed(0.8f);
    ani_to_frame_x(18.0f);
    random_hit(7);
    ani_to_frame_x(30.0f);
    random_hit(7);
    ani_to_blend_frame(20.0f);
    set_my_state(0);
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_just_backflip(void) {
    set_my_state(0x4208);
    init_air_move();
    head_tracking_off();
    pz_fighter_dont_fudge_desired_distance();
    pz_fighter_startup_attack(
        pz_shared_ani.backflip, 0x10000, 0x10008, 3, 2, 14.0f,
        0.1f, 0.6f, 0.3f, 0.8f);
    ani_to_frame_x(24.0f);
    player_feet_land_chores();
    init_ground_move();
    ani_to_frame_x(32.0f);
    init_air_move();
    ani_to_frame_x(38.0f);
    init_ground_move();
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_long_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_superman_move(void) {
    int reached = 0;
    int ticks;
    void* object;

    init_air_move_no_aniproc();
    head_tracking_off();
    plyr_obj->flags_09 &= ~0x80;
    blend_to_ani(pz_shared_ani.superman, 3, 0.2f);
    set_ani_speed(0.6f);
    ani_to_frame_x(17.0f);
    set_ani_speed(0.75f);
    myvel_his_angle_y(0.0f, -0.035f, -0.035f);
    for (ticks = 0; ticks < 15; ticks++) {
        if (xz_distance_between_players() < 1.25f) {
            reached = 1;
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    stop_me();
    if (reached == 1) {
        pz_fighter_reaction_xfer_him(0x27);
    }
    ani_to_frame_x(37.0f);
    plyr_obj->flags_09 |= 0x80;
    object = plyr_obj != 0 ? as_mkhdr((MkHdr*)plyr_obj) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr((MkHdr*)plyr_obj) : 0;
    ground_me(object);
    ani_to_end();
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_dizzy(void) {
    int flags = 0;

    if (plyr_pdata->plyr_num == 1) {
        flags = 8;
    }
    init_ground_move_no_aniproc();
    rotate_towards_him(0.1f);
    set_my_state(0x4203);
    plyr_pdata->state_flags.raw |= 0x10;
    plyr_obj->flags_09 &= ~2;
    blend_to_ani(shared_ani.dizzy, flags, 0.1f);
    xfer_proc(plyr_anim_proc, p_animate);
    _mkproc_sleep_ticks = 10.0f;
    aproc->vtbl->sleep();
    set_my_state(0);
    for (;;) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
}

float pz_fighter_exit(void) {
    plyr_pdata->script_exit_value_int = 10;
    plyr_pdata->script_exit_args[0] = 0;
    plyr_pdata->script_exit_args[1] = 0;
    plyr_pdata->script_exit_args[2] = 0;
    plyr_pdata->input_unlock_tick = 0;
    plyr_pdata->blocking_disable_tick_1 = 0;
    plyr_pdata->blocking_disable_tick_2 = 0;
    aproc->vtbl->transfer_with_exit(j_exit_6, 10, 0.0f);
    return 0.0f;
}

float pz_fighter_long_exit(void) {
    plyr_pdata->script_exit_value_int = 20;
    plyr_pdata->script_exit_args[0] = 0;
    plyr_pdata->script_exit_args[1] = 0;
    plyr_pdata->script_exit_args[2] = 0;
    plyr_pdata->input_unlock_tick = 0;
    plyr_pdata->blocking_disable_tick_1 = 0;
    plyr_pdata->blocking_disable_tick_2 = 0;
    aproc->vtbl->transfer_with_exit(j_exit_6, 20, 0.0f);
    return 0.0f;
}

/* Soft ceiling: pz_fighter_force_reaction_in_ticks ~84.67% - broad process pass. */
void pz_fighter_force_reaction_in_ticks(int reaction, int ticks) {
    PuzzleReactionDelayPdata* pdata;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_force_reaction,
            sizeof(PuzzleReactionDelayPdata), &pdata) != 0 &&
        pdata != 0) {
        pdata->ticks = ticks;
        pdata->reaction = reaction;
        pdata->saved_pdata = apdata;
    }
}

/* Soft ceiling: p_force_reaction ~85.79% - broad typed process-data pass. */
float p_force_reaction(void) {
    PuzzleReactionDelayPdata* pdata = apdata;

    pdata->ticks--;
    if (pdata->ticks > 0) {
        return 1.0f;
    }

    apdata = pdata->saved_pdata;
    pz_fighter_reaction_xfer_him(pdata->reaction);
    return -1.0f;
}

float pz_fighter_r_null(void) {
    _mkproc_sleep_ticks = 8.0f;
    aproc->vtbl->sleep();
    blend_to_stance(0.1f);
    aproc->vtbl->transfer(j_exit, 0.0f);
    return 0.0f;
}

float r_pz_fighter_spear_tug(void) {
    int ticks;

    blend_to_ani(
        his_pdata->fighter_definition->spear_tug_start, 3, 0.1f);
    ani_to_end();
    snd_req(0xD70);
    myvel_his_angle_y(0.0f, -0.05f, -0.05f);
    blend_to_ani(
        his_pdata->fighter_definition->spear_tug_loop, 0, 0.1f);
    plyr_anim_pdata->step = 0.45f;
    for (ticks = 0; ticks < 120; ticks++) {
        if (xz_distance_between_players() <= 1.0f) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    init_ground_move();
    stop_me();
    set_my_state(0x4204);
    blend_to_ani(
        his_pdata->fighter_definition->spear_tug_end, 3, 0.2f);
    plyr_anim_pdata->step = 1.3f;
    ani_loop_more_frames(120.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float r_pz_fighter_spear_hit(void) {
    stop_me();
    init_air_move();
    set_my_state(0x603);
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    blend_to_ani(his_pdata->fighter_definition->spear_hit, 3, 0.1f);
    ani_to_frame_x(83.0f);
    set_my_state(0x604);
    ani_to_end();
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

void suspend_in_midair(float ticks) {
    float saved_velocity = plyr_obj->vertical_velocity;

    plyr_obj->gravity_enabled = 0;
    plyr_obj->vertical_velocity = 0.0f;
    _mkproc_sleep_ticks = ticks;
    aproc->vtbl->sleep();
    plyr_obj->vertical_velocity = saved_velocity;
    plyr_obj->gravity_enabled = 1;
}

float r_pz_fighter_block_hi(void) {
    stop_me();
    init_ground_move();
    random_hit(1);
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    set_my_state(0xA00);
    blend_to_ani(shared_ani.block_high, 0, 0.5f);
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_anim_pdata->step = 1.0f;
    force_forward(6, 3, plyr_anim_pdata, 0.01f, 0.5f);
    for (;;) {
        nudge_towards_him(0.2f);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        if ((plyr_pdata->state & 0x800) == 0 || his_pdata->state == 0) {
            break;
        }
    }
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

void pz_fighter_function(unsigned int function) {
    if (function < 3) {
        pz_fighter_tbl[function]();
    }
}

float pz_fighter_present_explode(void) {
    if (g_pz_fighters_engine.present != 0) {
        g_pz_fighters_engine.present->state = 2;
    }
    return 0.0f;
}

float pz_fighter_present_given(void) {
    if (g_pz_fighters_engine.present != 0) {
        g_pz_fighters_engine.present->state = 3;
    }
    return 0.0f;
}

float pz_fighter_present_on_attackers_hand(void) {
    PuzzlePresentState* present = 0;

    if (_create_mkproc_generic_tinystack(
            0xC001, 0x1F, p_present_control,
            sizeof(PuzzlePresentState), &present) != 0 &&
        present != 0) {
        present->state = 0;
        g_pz_fighters_engine.present = present;
        present->owner = plyr_pdata;
    }
    return 0.0f;
}

void pz_fighter_kill_present(void) {
    if (g_pz_fighters_engine.present == 0) {
        return;
    }
    g_pz_fighters_engine.present->state = 5;
}

void pz_fighter_allow_easy_continuation(void) {
    g_pz_fighters_engine.flags |=
        PZ_ENGINE_EASY_CONTINUATION |
        PZ_ENGINE_CONTINUATION_ALLOWED;
}

void pz_fighter_reset_continuation(void) {
    g_pz_fighters_engine.flags &=
        ~PZ_ENGINE_CONTINUATION_RESET;
}

void pz_fighter_disallow_continuation(void) {
    g_pz_fighters_engine.flags &=
        ~PZ_ENGINE_CONTINUATION_ALLOWED;
    g_pz_fighters_engine.flags2 &=
        ~PZ_ENGINE2_CONTINUATION_BLOCKED;
}

void pz_fighter_allow_continuation(void) {
    g_pz_fighters_engine.flags &=
        ~PZ_ENGINE_EASY_CONTINUATION;
    g_pz_fighters_engine.flags |=
        PZ_ENGINE_CONTINUATION_ALLOWED;
}

/* Soft ceiling: pz_fighter_clear_out_external_forces ~97.50% - pool label only. */
void pz_fighter_clear_out_external_forces(void) {
    plyr_obj->external_force_x = 0.0f;
    plyr_obj->external_force_z = 0.0f;
}

void pz_fighter_clear_out_all_external_forces(
    PuzzleFighterObject* fighter) {
    fighter->external_force_x = 0.0f;
    fighter->external_force_z = 0.0f;
}

/* Exact: repeated typed slot access preserves retail post-setup reloads. */
static float r_call_other_pz_player_char_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->his_plyr_pdata->cmo,
                              active_cmdscript->shared_function);
    call_player_script_function(plyr_pdata->his_plyr_pdata->cmo);
    return 0.0f;
}

static float r_call_character_cmo_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->cmo,
                              active_cmdscript->shared_function);
    call_player_script_function(plyr_pdata->cmo);
    return 0.0f;
}

static float r_call_player_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->fighter_definition->cmo,
                              active_cmdscript->shared_function);
    call_player_script_function(plyr_pdata->fighter_definition->cmo);
    return 0.0f;
}

float r_pz_call_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(pz_shared_cmo,
                              active_cmdscript->shared_function);
    call_player_script_function(pz_shared_cmo);
    return 0.0f;
}

float pz_fighter_shove_brush_back(void) {
    PuzzleAttackParameters attack = {
        45.0f, 0.1f, 1.3f, 0x00010001, 0x00070007, 3,
        0.4f, 0.8f, 1.1f, 55.0f, 47.0f, 1, 0, 0, 0,
    };
    PuzzleFighterMove* move;

    move = pz_get_fighter_move();
    move->active_flags |= 1;
    pz_fighter_attack(pz_shared_ani.shove, &attack, 0x14);
    ani_to_end();
    return 0.0f;
}

float pz_fighter_uppercut_brush_back(void) {
    PuzzleAttackParameters attack = {
        9.0f, 0.1f, 0.9f, 0x00010000, 0x00060008, 3,
        0.2f, 0.6f, 0.85f, 14.0f, 11.0f, 0, 1, 0, 0,
    };

    pz_fighter_attack(pz_shared_ani.uppercut_brush_back, &attack, 8);
    slow_ani_x(0.3f, 17.0f);
    ani_to_end();
    return 0.0f;
}

float pz_fighter_punch_dizzyfall(void) {
    PuzzleAttackParameters attack = {
        9.0f, 0.1f, 1.15f, 0x00010001, 0x00070007, 3,
        0.2f, 0.75f, 0.92f, 13.0f, 11.0f, 1, 1, 0, 0,
    };

    head_tracking_off();
    pz_fighter_attack(pz_shared_ani.dizzy_punch, &attack, 0x11);
    pz_fighter_check_breakout();
    ani_to_frame_x(22.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

float pz_fighter_back_and_forth_showoff(void) {
    PuzzleAttackParameters attack = {
        9.0f, 0.1f, 1.15f, 0x00010001, 0x00070007, 3,
        0.2f, 0.75f, 0.92f, 13.0f, 11.0f, 1, 1, 0, 0,
    };

    head_tracking_off();
    pz_fighter_attack(pz_shared_ani.dizzy_punch, &attack, 0x1D);
    pz_fighter_check_breakout();
    ani_to_frame_x(22.0f);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}
