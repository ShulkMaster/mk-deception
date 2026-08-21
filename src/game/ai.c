#include "game/ai.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_info.h"
#include "runtime/plyr_pdata.h"
#include "game/constrain.h"
#include "game/game_info.h"
#include "game/settings.h"
#include "math/gxMath.h"
#include "math/mk_math.h"

typedef struct DroneAI {
    int movement_state; /* +0x00 */
    unsigned int match_mode; /* +0x04 */
    char pad008[4];
    float reaction_scale; /* +0x0C */
    unsigned int reaction_ticks; /* +0x10 */
    PlyrPdata* player; /* +0x14 */
    unsigned int match_stage; /* +0x18 */
    unsigned int big_boss_stage; /* +0x1C */
    unsigned int initialization_tick; /* +0x20 */
    unsigned int charge_cooldown_tick; /* +0x24 */
    unsigned int next_style_change_tick; /* +0x28 */
    float opponent_health; /* +0x2C */
    float player_health; /* +0x30 */
    float opponent_distance; /* +0x34 */
    int opponent_out_of_range; /* +0x38 */
    int background_attack_active; /* +0x3C */
    int difficulty_index; /* +0x40 */
    int decision_ready; /* +0x44 */
    int attack_pending; /* +0x48 */
    int jump_attack_pending; /* +0x4C */
    unsigned int block_hold_ticks; /* +0x50 */
    int special_reaction_active; /* +0x54 */
    int block_request; /* +0x58 */
    int block_subtype; /* +0x5C */
    int special_reaction_ticks; /* +0x60 */
    int special_reaction_state; /* +0x64 */
    void* script_attack; /* +0x68 */
    int script_attack_ready; /* +0x6C */
    int request_active; /* +0x70 */
    void (*reaction_watcher)(void); /* +0x74 */
    int attack_type; /* +0x78 */
    int attack_latched; /* +0x7C */
    int movement_attempt; /* +0x80 */
    int force_attack; /* +0x84 */
    unsigned int arena_collision_history; /* +0x88 */
    unsigned int arena_collision_count; /* +0x8C */
    int evade_arena_state; /* +0x90 */
    Vec obstacle_target; /* +0x94 */
    int danger_area_active; /* +0xA0 */
    int danger_area_request; /* +0xA4 */
    int danger_area_ready; /* +0xA8 */
    int danger_side_step; /* +0xAC */
    int taunt_pending; /* +0xB0 */
    int hit_active; /* +0xB4 */
    int fatality_decision; /* +0xB8 */
    int command_active; /* +0xBC */
    int super_combo_active; /* +0xC0 */
    int avoid_position_request; /* +0xC4 */
    Vec avoid_position_target; /* +0xC8 */
    int avoid_position_ready; /* +0xD4 */
    int danger_area_state; /* +0xD8 */
    int big_boss_block_state; /* +0xDC */
    int consecutive_losses; /* +0xE0 */
    int reversal_pending; /* +0xE4 */
    unsigned int opponent_round_attacks; /* +0xE8 */
    int start_state_a; /* +0xEC */
    int start_state_b; /* +0xF0 */
    int attack_disable_request; /* +0xF4 */
    int push_attempts; /* +0xF8 */
    int danger_area_counter; /* +0xFC */
    unsigned int big_boss_stage_hits; /* +0x100 */
    unsigned int damage_transition_tick; /* +0x104 */
    unsigned int duck_reaction_tick; /* +0x108 */
    unsigned int duck_started_tick; /* +0x10C */
    unsigned int next_special_voice_tick; /* +0x110 */
    unsigned int* ai_command; /* +0x114 */
    int ai_command_arg; /* +0x118 */
    int ai_command_target; /* +0x11C */
    float ai_command_value; /* +0x120 */
    float walk_ticks; /* +0x124 */
    int ai_command_flag0; /* +0x128 */
    int ai_command_flag1; /* +0x12C */
    int ai_command_flag2; /* +0x130 */
    char pad134[8];
    float avoidance_area_duration; /* +0x13C */
    float avoidance_position[3]; /* +0x140 */
    unsigned int block_retry_tick; /* +0x14C */
    unsigned int field_150; /* +0x150 */
} DroneAI;

typedef struct AiMovesetTableContainer {
    char pad00[0x80];
    FighterAiTable tables[14];
} AiMovesetTableContainer;

typedef struct AiMoveset {
    char pad00[4];
    AiMovesetTableContainer* ai_tables; /* +0x04 */
    ScriptSlot* cmo;                    /* +0x08 */
} AiMoveset;

typedef struct DroneOverrideInfo {
    float likelihood_scale; /* +0x00 */
    unsigned int flags;     /* +0x04 */
} DroneOverrideInfo;

typedef struct AiProcVtable {
    char pad00[0x18];
    void (*sleep)(void); /* +0x18 */
    char pad1C[8];
    float (*transfer)(float (*entry)(void), float delay); /* +0x24 */
} AiProcVtable;

typedef struct AiRequest {
    char pad00[0x3C];
    int active;
} AiRequest;

typedef struct AiSharedAnimations {
    char pad000[0x370];
    AniData* major_pain_a;
    AniData* twitch_death;
    AniData* major_pain_b;
} AiSharedAnimations;

typedef struct AiWeaponStyleView {
    int style_id;
} AiWeaponStyleView;

typedef struct AiMoveBlendScripts {
    char pad00[0xF0];
    int fatality_scripts[40];
    int push_script;
} AiMoveBlendScripts;

typedef struct AiFightstyleAttack {
    int opcode;
    int argument;
} AiFightstyleAttack;

typedef struct AiFightstyleAttackTable {
    char pad000[0xF0];
    AiFightstyleAttack attacks[18];
} AiFightstyleAttackTable;

typedef struct AiAnimPdataView {
    char pad000[0xA8];
    float twitch_weight; /* +0xA8 */
} AiAnimPdataView;

typedef struct AiSpecialMoveList {
    char pad00[0xC4];
    unsigned int ranged_count;
    unsigned int* ranged_commands;
    unsigned int count;
    unsigned int* commands;
} AiSpecialMoveList;

typedef struct AiExtendedSpecialMoveList {
    char pad000[0x114];
    unsigned int throw_count;
    unsigned int* throw_commands;
} AiExtendedSpecialMoveList;

typedef struct AiStatusSoundView {
    char pad000[0x140];
    unsigned int pain_voice;
} AiStatusSoundView;

typedef struct AiMoveSoundData {
    char pad00[4];
    struct AiMoveSoundFlags* flags;
} AiMoveSoundData;

typedef struct AiMoveSoundFlags {
    char pad00[0x5C];
    unsigned int weapon_flags;
} AiMoveSoundFlags;

typedef struct AiFighterSoundView {
    char pad00[4];
    AiMoveSoundData* move_data;
} AiFighterSoundView;

typedef struct AiComboTable {
    char pad00[0x80];
    unsigned int close_attack_count;
    char pad84[0x4C];
    unsigned int distant_attack_count;
} AiComboTable;

typedef struct AiAirMoveStatus {
    char pad000[0xBC];
    unsigned int close_count;
    unsigned int* close_commands;
    char pad0C4[0x58];
    unsigned int distant_count;
    unsigned int* distant_commands;
} AiAirMoveStatus;

typedef struct AiStyleMoveData {
    char pad000[0xB8];
    int taunt_count;
    char pad0BC[4];
    int charge_count;
} AiStyleMoveData;

typedef struct AiStyleSlot {
    char pad000[4];
    AiStyleMoveData* moves;
} AiStyleSlot;

typedef struct AiWalkMovementData {
    char pad00[0x28];
    float forward_start_step;
    float forward_weight;
    char pad30[8];
    float backward_start_step;
    float backward_weight;
    char pad40[8];
    float forward_loop_step;
    char pad4C[4];
    float backward_loop_step;
} AiWalkMovementData;

typedef struct AiWalkAnimationSet {
    char pad00[4];
    AiWalkMovementData* movement;
    char pad08[0x70];
    AniData* forward_start;
    AniData* backward_start;
    char pad80[8];
    AniData* forward_loop;
    AniData* backward_loop;
} AiWalkAnimationSet;

typedef struct AiFightStyleRestrictionTable AiFightStyleRestrictionTable;

typedef struct AiCharacterStateWeights {
    int character_id;
    int weights[2][9];
} AiCharacterStateWeights; /* 0x4C */

typedef struct AiTauntCameraData {
    MkObj* object;
    unsigned int object_instance;
    int ticks;
    float angle;
    float distance;
    float height;
    float depth;
    int active;
} AiTauntCameraData; /* 0x20 */

typedef struct AiCameraPoseView {
    char pad000[0xA0];
    Vec position;
    char pad0AC[0x24];
    Vec target;
} AiCameraPoseView;

typedef struct AiCameraItem {
    MkHdr* node;
    unsigned int instance;
} AiCameraItem;

typedef struct AiGamePlayerView {
    char pad000[0xFC];
    PlyrPdata* first_player;
    MkObj* first_object;
    char pad104[0x64];
    PlyrPdata* second_player;
} AiGamePlayerView;

struct AiFightStyleRestrictionTable {
    char pad00[0x1C];
    int (*taunt_allowed)(AiFightStyleRestrictionTable* table);
    int (*charge_allowed)(AiFightStyleRestrictionTable* table);
    char pad24[0x398];
    unsigned int attack_chance[9];       /* +0x3BC */
    int close_attack_chance[9];          /* +0x3E0 */
    int alternate_attack_chance[9];      /* +0x404 */
    int evade_attack_chance[9];          /* +0x428 */
    char pad44C[0x24];
    AiCharacterStateWeights character_states[40]; /* +0x470 */
    int idle_state_weights[9];           /* +0x1050 */
    int easy_state_weights[9];           /* +0x1074 */
    int normal_state_weights[9];         /* +0x1098 */
    int boss_state_weights[9];           /* +0x10BC */
};

static unsigned short g_likelihoodOfInAirAttack[9] = {
    5, 10, 20, 40, 50, 60, 70, 80, 95
};

static int g_likelihoodOfRolling[9] = {
    60, 60, 60, 60, 60, 70, 80, 90, 100
};

static int g_likelihoodForComboBreaker[9] = {
    1, 1, 2, 4, 7, 7, 10, 10, 15
};

static int g_likelihoodOfBlockingInReaction[9] = {
    15, 20, 25, 35, 55, 60, 65, 80, 90
};

static unsigned int g_likelihoodOfReactAttack[9] = {
    1, 2, 5, 8, 10, 12, 15, 20, 25
};

static unsigned int g_minExtremeBlockHeldTime[9] = {
    300, 300, 240, 180, 180, 180, 120, 120, 90
};

static unsigned int g_minBlockHeldTime[9] = {
    300, 300, 240, 180, 120, 120, 60, 60, 50
};

static unsigned int g_likelihoodToThrow[9] = {
    15, 15, 20, 25, 28, 33, 38, 40, 45
};

static unsigned int g_likelihoodToAggressiveThrow[9] = {
    5, 5, 10, 10, 20, 20, 25, 30, 35
};

static unsigned int g_minBlockHiHeldTime[9] = {
    300, 300, 240, 180, 120, 120, 60, 50, 40
};

static int g_blockFakeOutPercentage[9] = {
    75, 75, 65, 60, 60, 45, 35, 25, 15
};

static int g_likelihoodOfBlockCounter[9] = {
    5, 10, 30, 40, 65, 65, 75, 90, 90
};

static int g_blockCounterAdjustor[9] = {
    0, -40, -25, 15, 0, -15, 5, 0, 0
};

static int g_likelihoodOfBlocking[9] = {
    1, 2, 7, 12, 14, 15, 18, 28, 32
};

static int g_bigBossBlockAdjuster[9] = {
    5, 5, 5, 5, 5, 5, 10, 10, 15
};

static int g_maximumNumBlocksInARow[9] = {
    2, 2, 3, 3, 3, 3, 4, 4, 5
};

static int g_likelihoodOfNotFacingAttacking[9] = {
    0, 15, 30, 40, 60, 60, 75, 75, 80
};

extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern AiCameraItem camera_item;
extern PlyrPdata* his_pdata;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;
extern AiSharedAnimations shared_ani;
extern DroneAI g_DroneAI1;
extern DroneAI g_DroneAI2;
extern DroneOverrideInfo g_DroneOverrideInfo;
extern int g_droneOverrideActiviated;
extern int g_big_boss_intro_tap_out_f;
extern int force_midpoint_calculation_update;
extern unsigned int game_tick_ctr;
extern unsigned int exec_tick_ctr;
extern ConstrainInfo constrain_info;
extern unsigned short randu0(unsigned int max);
extern void snd_req(int sound_id);
extern void random_snd_req(int sound_id);
extern void shake_camera(int ticks, float strength);
void random_hit(int group);
void fight_fx_blades_clash(PlyrPdata* player);
float frand(float range);
int pan_vol_pitch_snd_req(
    int sound_id, float pan, float volume, float pitch);
float dist_behind_me(void);
void xfer_player_proc(MkProc* proc, MkProcEntryFn entry);
void run_reaction_cleanup_function(PlyrPdata* player);
float r_call_script_function(void);
float joy_duck_loop(void);
int get_his_attack_counter(void);
float j_exit_blend_stance(void);
void init_air_move(void);
int am_i_flipped(void);
void plyr_bleed_large_ext(
    PlyrPdata* player, unsigned int bone, PlyrPdata* source);
void start_blood_particles(
    int effect, unsigned int bone, PlyrPdata* player, MkObj* object);

int get_player_number(void* player);
MkProc* get_player_proc(void* player);
float xz_distance_between_players(void);
void drone_ai_im_dizzy(void);
int drone_ai_check_for_berserker_movement();
int drone_ai_fetch_next_AIState(DroneAI* drone);
static int handicap_get_current_difficulty(DroneAI* drone);
int is_he_airborn(void);
int is_he_duck_blocking(void);
int InAttackRange(void);
int InAttackRange2(void);
int InAttackRange_close(void);
int always_false(void);
float j_exit(void);
float x_block(void);
float side_step_to_center_with_jexit(void);
void step_forward(void);
void step_backward(void);
void jump_towards_opponent(void);
void drone_walk_FB_true(int (*test)(void), int ticks, int forward,
                        int allow_exit);
void handicap_calc_min_time_in_block(DroneAI* drone);
float drone_ai_watcher(void);
int drone_ai_force_change_style(DroneAI* drone, int style);
void drone_ai_initialize(DroneAI* drone);
void jump_away_opponent(void);
float jump_away_opponent_with_j_exit(void);
float drone_entry(void);
int drone_ai_check_attack(DroneAI* drone, int force, int immediate);
int is_weapon_style(PlyrFighterDefinition* fighter);
int drone_ai_check_for_evade_attack(DroneAI* drone);
int drone_ai_check_for_evade_movement(DroneAI* drone);
int drone_ai_check_for_extreme_throw(DroneAI* drone);
int drone_ai_check_for_throw(DroneAI* drone);
int drone_ai_check_continue_combo(void);
int drone_ai_check_for_passive_movement(DroneAI* drone);
int drone_ai_check_change_style(DroneAI* drone);
int drone_ai_check_push(DroneAI* drone);
int drone_ai_check_for_defend_movement(DroneAI* drone);
int drone_ai_check_for_dodge_movement(DroneAI* drone);
int drone_ai_check_for_ducker_movement(DroneAI* drone);
int drone_ai_check_for_aggressive_movement(DroneAI* drone);
int is_big_boss(PlyrPdata* player);
void joy_dash_back(void);
float drone_ai_perform_attack(void);
void set_my_state(int state);
float side_step_to_center_attack_with_jexit(void);
float drone_ai_perform_push(void);
void drone_ai_victim_speared_2(void);
int drone_ai_attacker_defenseless(DroneAI* drone);
float drone_ai_avoid_danger_area_now(void);
int drone_ai_can_push(DroneAI* drone);
float drone_ai_perform_knockdown(void);
int am_i_a_big_character(void);
int am_i_airborn(void);
float j_flying_kick(void);
float j_flying_kick2(void);
void ck_rumble_controller(int pad, int strength, int duration);
void uv_to_opponent(Vec* direction);
void snd_req_delay(int sound, int delay);
void pre_attack_chores(void);
void plyr_going_to_attack_with_action(int action);
void share_my_attack_info(float duration, float divisor);
void init_ground_move_no_aniproc(void);
void face_opponent_now(void);
int random_foot(int group);
void tightrope_restrictions_off(MkObj* object, int clear);
void transition_to_anim_script(
    AnimPdata* anim, AniData* animation, int transition, float blend);
void set_root_and_obj_movement_weights(
    AnimPdata* animation, float root_weight, float object_weight);
void ani_to_frame_x(float frame);
float p_animate(void);
float p_camera_proc(void);
float j_stay_down_dead(void);
float dk_screen_taunt(void);
float drone_ai_perform_script_attack(void);
float drone_ai_scripted_attack(void);
void back_to_normal(void);
void init_ground_move(void);
void rotate_towards_him(float rate);
void end_of_round_check(void);
int handicap_likelihood_for_combo_breaker(DroneAI* drone);
void drone_ai_change_style(void);
void bgnd_restore_player(void);
void enable_all_my_blocking(void);
int is_my_chest_to_screen(void);
void blend_to_ani(AniData* animation, int transition, float blend);
void set_ani_speed(float speed);
void ani_loop_more_frames(float frames);
void blend_to_fstance(float blend);
void blend_to_stance(float blend);
void ani_to_blend_frame(float frame);
void advance_active_moveset(PlyrPdata* player);
void advance_sidekick_with_moveset(PlyrPdata* player);
void* get_special_move(void);
void* get_random_fightstyle_attack(
    PlyrFighterDefinition* fighter, int attack_group, int flags);
void rotate_towards_position(
    const Vec* target, MkObj* object, int flags, float rate);
int drone_ai_enemy_inair_attack(DroneAI* drone);
void* drone_ai_choose_move_from_category(
    int category, int action, int* is_script);
void drone_ai_special_attack_now(void);
int segment_against_obstacle_list(
    const Vec* start, const Vec* end, Vec* hit, ConstrainInfo* info);
void drone_step_LR_true(
    int (*test)(void), int ticks, int move_right);
extern int f_fatality_was_done;
extern int g_game_number;
extern int g_fatality_game_number;
extern int mode_of_play;
extern float game_speed;
extern AiFightStyleRestrictionTable fight_style_restriction_table;
extern int g_minDecisionBaseWaitTime[9];
extern int g_randomDecisionBaseWaitTime[9];
int get_game_state(void);
int get_fatality_available_flag(void);
int can_i_do_fatality_now(int player);
void do_my_suicide(void);
unsigned int handicap_calc_likelihood_of_blocking_in_reaction(
    DroneAI* drone);
int drone_ai_should_be_blocking(int reaction);
int drone_ai_check_projectile_head_on(DroneAI* drone);
int drone_ai_check_projectile_side(DroneAI* drone);
int drone_ai_check_propel_attack(DroneAI* drone);
int drone_ai_check_dont_touch_attack_phase1(DroneAI* drone);
int drone_ai_check_dont_touch_attack_phase2(DroneAI* drone);
int drone_ai_check_from_ground_attack_phase1(DroneAI* drone);
int drone_ai_check_from_ground_attack_phase2(DroneAI* drone);
int drone_ai_check_all_over_ground(DroneAI* drone);
int drone_ai_check_all_over_ground_phase1(DroneAI* drone, int delay);
int drone_ai_check_cant_dodge_attack(DroneAI* drone);
int drone_ai_check_cant_dodge_attack2(DroneAI* drone);
int drone_ai_check_avoid_danger_area(DroneAI* drone);
int drone_ai_check_attack_from_above(DroneAI* drone);
int drone_ai_check_mid_high_spinner(DroneAI* drone);
float drone_ai_dodge_3d_with_counter(void);
int drone_ai_check_for_side_step_counter_attack(DroneAI* drone);
int drone_ai_should_evade_attack(DroneAI* drone);
void drone_ai_charge_up_watcher_defense(DroneAI* drone);
void drone_ai_perform_charge_up(void);
int get_ladder_position(void);
int trial_get_drone_difficulty(void);
int mk_chess_get_current_difficulty_for_ai(int side);
extern int g_GameLossesInARow;
extern float inverse_game_speed;
int am_i_on_the_left(void);
void init_3d_move_no_aniproc(void);
static float p_lookat_cam(void);
static AiTauntCameraData at_cam_data;
float jump_towards_opponent_with_jexit(void);
float walk_forward_attackdist_with_jexit(void);
float walk_forward_attackdist2_with_jexit(void);
float step_forward_with_jexit(void);
float step_backward_with_jexit(void);
int drone_ai_victim_dizzy_2(void);
void drone_ai_victim_dizzy_3(void);
void do_my_fatality(void);
void do_my_2nd_fatality(void);
void look_at_target(const Vec* target);
void show_player(PlyrPdata* player);

#define AI_TRANSFER(entry)                                                   \
    ((AiProcVtable*)aproc->vtbl)                                             \
        ->transfer((float (*)(void))(entry), 0.0f)

#define AI_SLEEP(ticks)                                                      \
    do {                                                                     \
        _mkproc_sleep_ticks = (ticks);                                       \
        ((AiProcVtable*)aproc->vtbl)->sleep();                               \
    } while (0)

static float p_lookat_cam(void) {
    AiCameraPoseView* camera;
    AiTauntCameraData* data;
    AiGamePlayerView* game;
    MkObj* target;
    Vec saved_position;
    Vec saved_target;
    Vec look_target;
    float angle;
    float offset_x;
    float offset_z;

    camera = (AiCameraPoseView*)camera_item.node;
    if (camera != 0 &&
        camera_item.node->instance != camera_item.instance) {
        camera = 0;
    }
    if (camera == 0) {
        AI_TRANSFER(p_camera_proc);
        return 0.0f;
    }

    data = &at_cam_data;
    target = data->object;
    if (target != 0 &&
        target->hdr.instance != data->object_instance) {
        target = 0;
    }
    if (target == 0) {
        AI_TRANSFER(p_camera_proc);
        return 0.0f;
    }

    saved_position = camera->position;
    saved_target = camera->target;
    if (data->ticks != 0) {
        angle = target->ang.y + data->angle;
        angle =
            (float)((int)(angle * 166886.1f) & 0xFFFFF) *
            0.000005992112f;
        offset_x = gxMathSin(angle) * data->distance;
        offset_z = gxMathCos(angle) * data->distance;
        do {
            camera->position.x = target->pos.value.x + offset_x;
            camera->position.y = target->pos.value.y + data->depth;
            camera->position.z = target->pos.value.z + offset_z;
            look_target = target->pos.value;
            look_target.y += data->height;
            look_at_target(&look_target);
            AI_SLEEP(1.0f);
            data->ticks--;
        } while (data->ticks != 0);
    }

    game = (AiGamePlayerView*)&g_game_info;
    if (target == game->first_object) {
        show_player(game->second_player);
    } else {
        show_player(game->first_player);
    }
    if (data->active != 0) {
        camera->position = saved_position;
        camera->target = saved_target;
    }
    force_midpoint_calculation_update = 1;
    AI_TRANSFER(p_camera_proc);
    return 0.0f;
}

static float ai_side_clearance(int right) {
    Vec end;
    Vec hit;
    float sine;
    float cosine;
    float direction;

    if (plyr_obj == 0) {
        return 100.0f;
    }
    direction = right ? 1.0f : -1.0f;
    sine = gxMathSin(plyr_obj->ang.y);
    cosine = gxMathCos(plyr_obj->ang.y);
    end = plyr_obj->pos.value;
    end.x += direction * 100.0f * cosine;
    end.z -= direction * 100.0f * sine;
    if (!segment_against_obstacle_list(
            &plyr_obj->pos.value, &end, &hit, &constrain_info)) {
        return 100.0f;
    }
    return dist_v3_to_v3(&hit, &plyr_obj->pos.value);
}

static float ai_backward_clearance(void) {
    Vec end;
    Vec hit;

    if (plyr_obj == 0) {
        return -1.0f;
    }
    end = plyr_obj->pos.value;
    end.x -= 100.0f * gxMathSin(plyr_obj->ang.y);
    end.z -= 100.0f * gxMathCos(plyr_obj->ang.y);
    if (!segment_against_obstacle_list(
            &plyr_obj->pos.value, &end, &hit, &constrain_info)) {
        return 100.0f;
    }
    return dist_v3_to_v3(&hit, &plyr_obj->pos.value);
}

static void* ai_pick_special_move(
    DroneAI* drone, unsigned int* commands, unsigned int count) {
    if (count == 0) {
        return 0;
    }
    drone->ai_command = commands + randu0(count) * 16;
    drone->ai_command_arg = 0;
    drone->ai_command_target = drone->player->character_id;
    drone->ai_command_flag0 = 0;
    drone->ai_command_flag1 = 0;
    drone->ai_command_flag2 = 0;
    return get_special_move();
}

static int ai_start_throw(DroneAI* drone) {
    void* script;
    int is_script;

    script = drone_ai_choose_move_from_category(11, 75, &is_script);
    if (script == 0) {
        return 0;
    }
    drone->script_attack = script;
    drone->script_attack_ready = is_script != 0 ? 1 : 2;
    xfer_proc(
        get_player_proc(plyr_obj),
        (MkProcEntryFn)drone_ai_perform_script_attack);
    drone->request_active = 1;
    return 1;
}

static int ai_count_charge_moves(void) {
    AiStyleSlot* style;
    int count;
    int i;

    if (!fight_style_restriction_table.charge_allowed(
            &fight_style_restriction_table)) {
        return 0;
    }

    count = 0;
    for (i = 0; i < 3; i++) {
        style = (AiStyleSlot*)plyr_pdata->weapon_styles[i];
        if (style->moves != 0) {
            count += style->moves->charge_count;
        }
    }
    return count;
}

static int ai_find_taunt_style(void) {
    AiStyleSlot* style;
    int i;

    if (!fight_style_restriction_table.taunt_allowed(
            &fight_style_restriction_table)) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        style = (AiStyleSlot*)plyr_pdata->weapon_styles[i];
        if (style->moves != 0 && style->moves->taunt_count != 0) {
            return i;
        }
    }
    return -1;
}

static int ai_count_taunt_moves(void) {
    AiStyleSlot* style;
    int count;
    int i;

    if (!fight_style_restriction_table.taunt_allowed(
            &fight_style_restriction_table)) {
        return 0;
    }
    count = 0;
    for (i = 0; i < 3; i++) {
        style = (AiStyleSlot*)plyr_pdata->weapon_styles[i];
        if (style->moves != 0) {
            count += style->moves->taunt_count;
        }
    }
    return count;
}

static int ai_find_charge_style(void) {
    AiStyleSlot* style;
    int i;

    if (!fight_style_restriction_table.charge_allowed(
            &fight_style_restriction_table)) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        style = (AiStyleSlot*)plyr_pdata->weapon_styles[i];
        if (style->moves != 0 && style->moves->charge_count != 0) {
            return i;
        }
    }
    return -1;
}

static void ai_transfer_active(MkProcEntryFn entry) {
    DroneAI* active_drone;

    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    xfer_proc(get_player_proc(plyr_obj), entry);
    active_drone->request_active = 1;
}

void liukang_in_fight_random_snd_check(void) {
    int character = plyr_pdata->character_id;

    if ((character == 0x10 || character == 0x11) && randu0(100) < 10) {
        random_snd_req(0x23);
    }
}

/* Soft ceiling: big_boss_wait_for_intro ~99.81% - 1.0f pool identity only; stop. */
void big_boss_wait_for_intro(void) {
    int ticks = 300;

    do {
        if (g_big_boss_intro_tap_out_f != 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((AiProcVtable*)aproc->vtbl)->sleep();
    } while (--ticks != 0);
}

void big_boss_taunt_cam_cut(void) {
    unsigned short camera_roll;
    unsigned short sound_roll;
    float camera_angle;
    float camera_distance;
    float camera_depth;

    camera_roll = randu0(100);
    init_3d_move_no_aniproc();
    set_my_state(0x420A);

    if (camera_roll < 66) {
        camera_angle =
            am_i_on_the_left() ? -0.9599311f : 0.9599311f;
        camera_distance = 2.35f;
        camera_depth = -1.1f;
        if (camera_roll >= 33) {
            if (am_i_on_the_left()) {
                camera_distance = 2.85f;
                camera_depth = 2.0f;
            } else {
                camera_depth = 2.1f;
            }
        }

        at_cam_data.object = plyr_obj;
        at_cam_data.object_instance = plyr_obj->hdr.instance;
        at_cam_data.ticks = 0x11D;
        at_cam_data.angle = camera_angle;
        at_cam_data.distance = camera_distance;
        at_cam_data.height = 0.6f;
        at_cam_data.depth = camera_depth;
        at_cam_data.active = 1;
        xfer_camera(p_lookat_cam, 0);
    }

    set_ani_speed(0.8f);
    set_root_and_obj_movement_weights(plyr_anim_pdata, 0.0f, 1.0f);
    if (is_my_chest_to_screen() == 0) {
        blend_to_ani(
            plyr_pdata->big_boss_taunt_animation, 0xB, 0.1f);
    } else {
        blend_to_ani(
            plyr_pdata->big_boss_taunt_animation, 3, 0.1f);
    }

    ani_to_frame_x(15.0f);
    sound_roll = randu0(100);
    if (sound_roll < 33) {
        snd_req(0x1B0);
    } else if (sound_roll < 66) {
        snd_req(0x1B1);
    } else {
        snd_req(0x1B2);
    }
    shake_camera(1, 0.01f);

    ani_to_frame_x(32.0f);
    sound_roll = randu0(100);
    if (sound_roll < 33) {
        snd_req(0x1B0);
    } else if (sound_roll < 66) {
        snd_req(0x1B1);
    } else {
        snd_req(0x1B2);
    }
    shake_camera(1, 0.01f);

    ani_to_frame_x(48.0f);
    snd_req(0x1B6);
    ani_to_blend_frame(10.0f);
    if (is_my_chest_to_screen() == 0) {
        blend_to_fstance(0.05f);
    } else {
        blend_to_stance(0.05f);
    }
    set_my_state(0);
    AI_TRANSFER(j_exit);
}

void set_attackers_attack_region(int region) {
    plyr_pdata->attack_region = region;
}

void setDroneOverrideSwitch(int activated, DroneOverrideInfo* info) {
    g_DroneOverrideInfo.likelihood_scale = 1.0f;
    g_DroneOverrideInfo.flags = 0;
    g_droneOverrideActiviated = activated;
    if (activated == 1) {
        g_DroneOverrideInfo.likelihood_scale = info->likelihood_scale;
        g_DroneOverrideInfo.flags = info->flags;
    }
}

void random_dk_foot(void) {
    unsigned short sound_roll;

    sound_roll = randu0(100);
    if (sound_roll < 33) {
        snd_req(0x1B0);
    } else if (sound_roll < 66) {
        snd_req(0x1B1);
    } else {
        snd_req(0x1B2);
    }
    shake_camera(1, 0.01f);
}

int can_big_boss_make_special_vo_call(unsigned int cooldown_ticks) {
    DroneAI* drone =
        get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;

    if (drone->next_special_voice_tick < game_tick_ctr) {
        drone->next_special_voice_tick =
            game_tick_ctr + cooldown_ticks;
        return 1;
    }
    return 0;
}

float drone_ai_get_big_boss_damage_scale(PlyrPdata* player) {
    DroneAI* drone;

    if (player == g_game_info.plyr0.slot.pdata) {
        drone = &g_DroneAI1;
    } else if (player == g_game_info.plyr1.slot.pdata) {
        drone = &g_DroneAI2;
    } else {
        return 1.0f;
    }

    if (player->drone_request == 0) {
        return 1.0f;
    }

    if (drone->damage_transition_tick < game_tick_ctr) {
        if (drone->big_boss_stage < 3) {
            return 0.42f;
        }
        if (drone->big_boss_stage == 3) {
            return 0.32f;
        }
        return 0.25f;
    }

    if (drone->big_boss_stage < 3) {
        return 0.75f;
    }
    if (drone->big_boss_stage == 3) {
        return 0.65f;
    }
    return 0.55f;
}

void cleanup_drone_ai(void) {
}

static float drone_ai_stupid_watcher(void) {
    return 1.0f;
}

void drone_ai_watcher_calculate_data(void) {
    DroneAI* drone;
    int player;
    int ladder_position;

    player = get_player_number(plyr_obj);
    drone = player == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->match_mode = g_game_info.pselect.field_1f4;

    if (player == 1) {
        drone->reaction_scale =
            g_game_info.plyr1.field_0C - g_game_info.plyr0.field_0C;
        drone->match_stage = g_game_info.plyr1.field_40;
        drone->opponent_health = g_game_info.plyr0.field_0C;
        drone->player_health = g_game_info.plyr1.field_0C;
    } else {
        drone->reaction_scale =
            g_game_info.plyr0.field_0C - g_game_info.plyr1.field_0C;
        drone->match_stage = g_game_info.plyr0.field_40;
        drone->opponent_health = g_game_info.plyr1.field_0C;
        drone->player_health = g_game_info.plyr0.field_0C;
    }

    drone->reaction_ticks = g_game_info.field_204;
    ladder_position = get_ladder_position();
    if (ladder_position > 7) {
        ladder_position = 7;
    }
    if (get_game_state() == 3) {
        ladder_position = 8;
    }
    if (mode_of_play == 8) {
        ladder_position = trial_get_drone_difficulty();
    }
    if (mode_of_play == 10) {
        ladder_position = 4;
    }

    drone->big_boss_stage = ladder_position;
    drone->player = plyr_pdata;
    drone->charge_cooldown_tick = game_settings.kombat_difficulty;
    drone->consecutive_losses = g_GameLossesInARow;
    drone->opponent_round_attacks = his_pdata->round_attack_count;

    if (drone->charge_cooldown_tick > 2) {
        if (drone->opponent_round_attacks >
            (unsigned int)(((int)drone->match_mode - 1) * 10 + 12 -
                           (int)drone->match_mode * 2)) {
            drone->start_state_a = 1;
        }
        if (his_pdata->round_attack_stage > 2) {
            drone->start_state_a = 1;
            if (his_pdata->round_attack_stage > 3) {
                drone->start_state_b = 1;
            }
        }
    }

    drone->opponent_distance = xz_distance_between_players();
    drone->opponent_out_of_range = 0;
    if (drone->opponent_distance > 14.6f ||
        drone->opponent_distance > 5.9457946f) {
        drone->opponent_out_of_range = 1;
    }

    if (exec_tick_ctr - drone->next_style_change_tick > 60) {
        drone->next_style_change_tick = exec_tick_ctr;
        drone->difficulty_index =
            handicap_get_current_difficulty(drone);
        if (get_game_state() == 3) {
            drone->difficulty_index = 8;
        }
        if (mode_of_play == 10) {
            drone->difficulty_index =
                mk_chess_get_current_difficulty_for_ai(
                    drone->player->plyr_num);
            drone->charge_cooldown_tick =
                game_settings.arcade_difficulty;
            if (drone->charge_cooldown_tick < 2) {
                drone->big_boss_stage = 2;
            } else if (drone->charge_cooldown_tick == 2) {
                drone->big_boss_stage = 4;
            } else if (drone->charge_cooldown_tick == 3) {
                drone->big_boss_stage = 6;
            } else {
                drone->big_boss_stage = 8;
            }
        }
    }

    if ((his_pdata->state & 0x800) != 0) {
        drone->block_hold_ticks++;
    } else if (drone->block_hold_ticks > 20) {
        drone->block_hold_ticks -= 5;
    } else {
        drone->block_hold_ticks = 0;
    }

    if (drone->attack_disable_request == 1) {
        drone->attack_disable_request = 0;
        plyr_pdata->attacks_disabled_until =
            game_tick_ctr +
            (int)(30.0f * inverse_game_speed + 0.5f);
    }
}

#pragma dont_inline on
static int handicap_get_current_difficulty(DroneAI* drone) {
    int score;
    int difficulty;
    int opponent_rounds;

    score = ((int)drone->big_boss_stage + 4) * 5;
    if (score > 90) {
        score = 90;
    }
    if (drone->big_boss_stage > 5) {
        score += 10;
    }

    switch (drone->charge_cooldown_tick) {
    case 0:
        score -= 25;
        break;
    case 1:
        score -= 20;
        break;
    case 2:
        if (drone->big_boss_stage < 5) {
            score -= 10;
        }
        break;
    case 3:
        score += 20;
        break;
    case 4:
        score += 50;
        break;
    }

    if (drone->match_mode == 1) {
        score -= 10;
    } else if (drone->match_mode == 3 &&
               drone->big_boss_stage > 2) {
        score += 10;
    }
    if (drone->reaction_scale < -0.45f) {
        score += (int)drone->charge_cooldown_tick * 5 + 5;
    }
    if (drone->reaction_scale > 0.45f) {
        score -= 10;
        if (score > 40) {
            score = 40;
        }
    }
    if (randu0(100) < 10) {
        score += 10;
    }
    if (drone->charge_cooldown_tick > 2 && score < 40) {
        score = 40;
    }
    if (drone->start_state_a == 1 && score < 70) {
        score += 10;
    }

    if (his_pdata == 0) {
        opponent_rounds = 0;
    } else if (his_pdata == g_game_info.plyr0.slot.pdata) {
        opponent_rounds = g_game_info.plyr0.field_44;
    } else {
        opponent_rounds = g_game_info.plyr1.field_44;
    }
    if (opponent_rounds == 2) {
        score += 10;
    } else if (opponent_rounds == 3) {
        score += 15;
    } else if (opponent_rounds >= 4) {
        score += 20;
    }

    if (score < 0) {
        score = 0;
    }
    difficulty = score / 10;
    if (difficulty > 8) {
        difficulty = 8;
    }
    if (drone->charge_cooldown_tick == 0 && difficulty > 4) {
        difficulty = 4;
    } else if (drone->charge_cooldown_tick == 1 && difficulty > 5) {
        difficulty = 5;
    }
    if (get_game_state() == 3) {
        drone->difficulty_index = 8;
        drone->big_boss_stage = 8;
        return 8;
    }
    return difficulty;
}

static int get_random_fightstyle_index(
    int* count_ptr, int selection_mode) {
    DroneAI* drone;
    float upper_scale;
    float lower_scale;
    unsigned int upper;
    unsigned int lower;
    int count;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    count = *count_ptr;
    upper = count;
    lower = 0;
    if (drone->big_boss_stage == 0 ||
        drone->charge_cooldown_tick == 0) {
        if (count > 2) {
            upper = 2;
        }
    } else if (drone->charge_cooldown_tick == 1 &&
               drone->big_boss_stage < 4) {
        if (count > 2) {
            upper = 2;
        }
    } else if (count >= 5) {
        upper_scale = 1.0f;
        lower_scale = 0.0f;
        if (drone->difficulty_index < 2) {
            if (randu0(100) < 3) {
                upper_scale = 0.6f;
                lower_scale = 0.33f;
            } else {
                upper_scale = 0.5f;
            }
        } else if (drone->difficulty_index < 4) {
            if (randu0(100) < 10) {
                upper_scale = 0.75f;
                lower_scale = 0.33f;
            } else {
                upper_scale = 0.5f;
            }
        } else if (drone->difficulty_index < 6) {
            if (randu0(100) < 20) {
                upper_scale = 0.85f;
                lower_scale = 0.4f;
            } else {
                upper_scale = 0.75f;
                lower_scale = 0.2f;
            }
        } else if (drone->difficulty_index < 8) {
            upper_scale = 0.85f;
            lower_scale = 0.4f;
        } else {
            upper_scale = 0.85f;
            lower_scale = 0.2f;
        }
        upper = (unsigned int)((float)count * upper_scale);
        lower = (unsigned int)((float)count * lower_scale);

        if (selection_mode == 1) {
            upper += 3;
            lower += 2;
            if (upper > (unsigned int)count) {
                upper = count;
            }
            if (lower > (unsigned int)count) {
                lower = count;
            }
        } else if (selection_mode == 2) {
            lower = 0;
        } else if (selection_mode == 3) {
            upper = count;
            lower = count > 1 ? count - 2 : 0;
        }
        if (drone->big_boss_stage > 3 &&
            drone->charge_cooldown_tick > 1 &&
            randu0(100) < 5) {
            upper = count;
            lower = 0;
        }
        if (lower == upper) {
            if (lower != 0) {
                lower--;
            }
            if (upper < (unsigned int)count) {
                upper++;
            }
        }
    }

    if (upper <= lower) {
        return randu0((unsigned short)lower);
    }
    return (int)lower +
           randu0((unsigned short)(upper - lower));
}
#pragma dont_inline off

#pragma optimization_level 2
void drone_ai_victim_dizzy(void) {
    DroneAI* drone;
    float delta;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return;
    }
    if (drone->fatality_decision == 0) {
        drone->fatality_decision = 1;
        if (randu0(100) >= 80 &&
            can_i_do_fatality_now(plyr_pdata->plyr_num) &&
            (g_game_info.field_04 & 0x20) == 0 &&
            g_game_number > g_fatality_game_number + 6 &&
            randu0(100) < 30) {
            g_fatality_game_number = g_game_number;
            drone->fatality_decision = 2;
        }
        if (is_big_boss(plyr_pdata)) {
            drone->fatality_decision = 4;
        }
        if (mode_of_play == 10) {
            drone->fatality_decision = 0;
        }
    } else if (drone->fatality_decision == 1) {
        if (drone->opponent_distance > 13.378037f) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
        } else if (xz_distance_between_players() >= 1.4864486f) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
        } else {
            drone->reaction_watcher =
                (void (*)(void))drone_ai_victim_dizzy_2;
        }
    } else if (drone->fatality_decision == 2) {
        delta = drone->opponent_distance - 4.0f;
        if (delta < 2.0f && delta > -2.0f) {
            drone->fatality_decision = 3;
            ai_transfer_active(
                randu0(100) < 95 ? (MkProcEntryFn)do_my_fatality
                                 : (MkProcEntryFn)do_my_2nd_fatality);
        } else if (drone->opponent_distance < 4.0f) {
            ai_transfer_active(step_backward_with_jexit);
        } else if (delta > 5.9457946f) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
        } else {
            ai_transfer_active(step_forward_with_jexit);
        }
    } else if (drone->fatality_decision == 4) {
        if (xz_distance_between_players() >= 3.3445094f) {
            ai_transfer_active(walk_forward_attackdist2_with_jexit);
        } else {
            drone->reaction_watcher =
                (void (*)(void))drone_ai_victim_dizzy_3;
        }
    }
}
#pragma optimization_level 4

/* Soft ceiling: drone_ai_should_ermac_fly_kick ~99.66% - float pool identities only; stop. */
int drone_ai_should_ermac_fly_kick(void) {
    float distance = xz_distance_between_players();

    if (distance > 5.9457946f && randu0(100) < 30) {
        return 0;
    }
    if (distance < 5.9457946f && randu0(100) < 30) {
        return 1;
    }
    if (distance < 14.6f && randu0(100) < 10) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: drone_ai_should_ermac_ground_slam ~99.76% - float pool identities only; stop. */
int drone_ai_should_ermac_ground_slam(void) {
    float distance = xz_distance_between_players();

    if (distance < 14.6f && randu0(100) < 30) {
        return 0;
    }
    if (distance > 5.9457946f && randu0(100) < 30) {
        return 1;
    }
    if (randu0(100) < 10) {
        return 1;
    }
    return 0;
}

static int drone_ai_is_dizzy_watcher(void) {
    if (plyr_pdata->state == 0x4203) {
        drone_ai_im_dizzy();
    }
    return 0;
}

int drone_ai_check_for_knockdown_movement(DroneAI* drone) {
    return drone_ai_check_for_berserker_movement(drone);
}

static void drone_ai_check_next_AIState(DroneAI* drone) {
    drone->movement_state = drone_ai_fetch_next_AIState(drone);
}

int drone_ai_check_taunt_restrictions(void) {
    return 1;
}

/* Soft ceiling: drone_ai_check_throw_restrictions ~99.04% - volatile pointer/state coloring; stop. */
int drone_ai_check_throw_restrictions(void) {
    int allowed = 1;
    PlyrPdata* opponent;
    int state;

    if ((g_DroneOverrideInfo.flags & 0x40) != 0) {
        return 0;
    }
    if (is_he_airborn() != 0) {
        return 0;
    }
    if (is_he_duck_blocking() != 0) {
        allowed = 0;
    }

    opponent = his_pdata;
    state = opponent->state;
    if (state == 0x101) {
        allowed = 0;
    }
    if (state == 0x302) {
        allowed = 0;
    }
    if (state == 0x900) {
        allowed = 0;
    }
    if (state == 0x901) {
        allowed = 0;
    }
    if (opponent->throw_restriction == 3) {
        allowed = 0;
    }
    if ((state & 0x400) != 0) {
        allowed = 0;
    }
    return allowed;
}

int always_true(void) {
    return 1;
}

int always_false(void) {
    return 0;
}

void drone_ai_check_charge_up_restrictions(void) {
}

static int drone_ai_should_be_attacking(
    DroneAI* drone, int* attack_state, int force) {
    unsigned int attack_chance;
    int chance;
    float attack_distance;

    if ((g_DroneOverrideInfo.flags & 0x10) != 0 ||
        plyr_pdata->action_lock_a > game_tick_ctr ||
        plyr_pdata->action_lock_b > game_tick_ctr ||
        plyr_pdata->push_blocked != 0 ||
        (plyr_pdata->state & 0x200) != 0) {
        return 0;
    }

    *attack_state = 0;
    attack_chance =
        fight_style_restriction_table.attack_chance[drone->difficulty_index];
    if (drone->movement_state == 2) {
        attack_chance -= 10;
        if (drone->difficulty_index > 3) {
            attack_chance -= 10;
        }
        if (attack_chance < 60) {
            attack_chance = 60;
        }
    } else if (drone->movement_state == 1 &&
               drone->difficulty_index > 1) {
        attack_chance += 10;
    }
    if (randu0(100) < 50) {
        attack_chance += randu0(10);
    } else {
        attack_chance -= randu0(5);
    }
    if (drone->match_stage == 0) {
        attack_chance = 10;
    }
    if (randu0(1000) >= attack_chance && force != 1) {
        drone->force_attack = 0;
        return 0;
    }

    attack_distance =
        is_big_boss(drone->player) ? 6.5670843f : 5.837408f;
    chance = fight_style_restriction_table
                 .close_attack_chance[drone->difficulty_index];
    if (drone->movement_state == 7) {
        chance = 0;
    } else {
        if (drone->movement_state == 2) {
            chance -= 2;
        } else if (drone->movement_state == 5 ||
                   drone->movement_state == 6) {
            chance += drone->difficulty_index > 5 ? 3 : 1;
        }
        chance += randu0(100) < 50 ? randu0(2) : -randu0(2);
        if (drone->match_stage > 2 && drone->start_state_a == 1) {
            chance += 10;
        }
        if (drone->difficulty_index > 4) {
            if (drone->big_boss_stage == 4) {
                chance = 70;
            } else if (drone->big_boss_stage == 3) {
                chance = 40;
            }
        }
        if (get_game_state() == 3) {
            chance = 40;
        }
        if (chance < 0) {
            chance = 1;
        }
    }
    if (randu0(100) < chance &&
        drone->opponent_distance < attack_distance) {
        *attack_state = 1;
    }

    chance = fight_style_restriction_table
                 .alternate_attack_chance[drone->difficulty_index];
    if (drone->movement_state == 7) {
        chance = 0;
    } else {
        if (drone->movement_state == 2) {
            chance -= 2;
        } else if (drone->movement_state == 5 ||
                   drone->movement_state == 6) {
            chance += drone->difficulty_index > 5 ? 3 : 1;
        }
        chance += randu0(100) < 50 ? randu0(2) : -randu0(2);
        if (drone->match_stage > 2 && drone->start_state_a == 1) {
            chance += 2;
        }
        if (get_game_state() == 3) {
            chance += 3;
        }
        if (chance < 0) {
            chance = 1;
        }
    }
    if (randu0(100) < chance &&
        drone->opponent_distance < attack_distance) {
        *attack_state = 5;
    }

    chance = fight_style_restriction_table
                 .evade_attack_chance[drone->difficulty_index];
    if ((g_DroneOverrideInfo.flags & 4) != 0) {
        chance = 0;
    } else {
        if (drone->movement_state == 2) {
            chance -= 5;
        } else if (drone->movement_state == 1) {
            chance += 3;
        }
        if (chance < 0) {
            chance = 1;
        }
        chance += randu0(100) < 50 ? randu0(3) : -randu0(3);
        if (chance < 0) {
            chance = 1;
        }
        if (drone->big_boss_stage == 4) {
            chance -= 4;
        }
    }
    if ((randu0(100) < chance || drone->force_attack == 1) &&
        ((his_pdata->state & 0x800) == 0 || randu0(100) < 20)) {
        *attack_state = 2;
        if (drone->difficulty_index < 2 && drone->match_stage == 0) {
            *attack_state = 0;
        }
    }
    if (drone->opponent_distance > 6.74f &&
        drone->opponent_distance < 11.0f &&
        !is_big_boss(drone->player)) {
        attack_chance = drone->movement_state == 3 ? 2 : 7;
        if (randu0(100) < attack_chance &&
            drone->difficulty_index < 7) {
            *attack_state = 3;
        }
    }
    return 1;
}

int drone_ai_fetch_next_AIState(DroneAI* drone) {
    const int* weights;
    AiCharacterStateWeights* character;
    unsigned int total;
    unsigned int delay;
    unsigned short roll;
    int state;
    int selected;

    if (g_game_info.field_204 < 10 &&
        drone->reaction_scale < 0.05f &&
        (drone->movement_state == 0 ||
         drone->movement_state == 2 ||
         drone->movement_state == 3) &&
        drone->difficulty_index > 2) {
        drone->charge_cooldown_tick = exec_tick_ctr;
    }
    if (drone->charge_cooldown_tick > exec_tick_ctr) {
        return drone->movement_state;
    }

    if (drone->match_stage == 0) {
        weights = fight_style_restriction_table.idle_state_weights;
    } else if (drone->difficulty_index == 0) {
        weights = fight_style_restriction_table.easy_state_weights;
    } else if (drone->difficulty_index < 3) {
        weights = fight_style_restriction_table.normal_state_weights;
    } else if (drone->difficulty_index > 4 &&
               drone->big_boss_stage == 4) {
        weights = fight_style_restriction_table.boss_state_weights;
    } else {
        character = fight_style_restriction_table.character_states;
        while (character->character_id != drone->player->character_id &&
               character->character_id != -1) {
            character++;
        }
        weights = character->weights[drone->difficulty_index > 5];
    }

    roll = randu0(100);
    total = 0;
    selected = drone->movement_state;
    for (state = 0; state < 9; state++) {
        total += weights[state];
        if (roll < total) {
            selected = state;
            delay = randu0(3) * 60 + 120;
            if (state == 0 && drone->difficulty_index > 2) {
                delay -= randu0(60);
            } else if (state == 7 || state == 1 ||
                       state == 5 || state == 6) {
                delay += randu0(60) + 60;
                if (drone->difficulty_index > 5) {
                    delay += randu0(2) * 60 + 30;
                }
            }
            if (delay < 60) {
                delay = 60;
            }
            drone->charge_cooldown_tick = exec_tick_ctr + delay;
            if (g_game_info.field_204 < 15 &&
                selected != 1 && selected != 5 &&
                selected != 6 && selected != 7 &&
                drone->reaction_scale < 0.05f &&
                drone->difficulty_index > 2) {
                selected = 1;
            }
            if (get_game_state() == 3 &&
                (selected == 0 || selected == 2)) {
                selected = 1;
            }
            return selected;
        }
    }
    return drone->movement_state;
}

void drone_ai_clear_avoidance_area_duration(int player) {
    DroneAI* drone;

    drone = player == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->avoidance_area_duration = 0.0f;
}

float drone_ai_avoid_position_now(void) {
    ((AiProcVtable*)aproc->vtbl)
        ->transfer(side_step_to_center_with_jexit, 0.0f);
    return 0.0f;
}

float step_forward_with_jexit(void) {
    step_forward();
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float step_backward_with_jexit(void) {
    step_backward();
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float jump_towards_opponent_with_jexit(void) {
    jump_towards_opponent();
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

void drone_ai_hit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->hit_active = 0;
}

void drone_ai_finished_request(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->request_active = 0;
}

void drone_ai_initialize(DroneAI* drone) {
    int ladder_position;
    unsigned short random_ticks;

    drone->charge_cooldown_tick = exec_tick_ctr;
    drone->difficulty_index = 4;
    ladder_position = get_ladder_position();
    if (ladder_position > 7) {
        ladder_position = 7;
    }
    if (get_game_state() == 3) {
        ladder_position = 8;
    }
    if (mode_of_play == 8) {
        ladder_position = trial_get_drone_difficulty();
    }
    if (mode_of_play == 10) {
        ladder_position = 4;
    }

    if (ladder_position < 3) {
        drone->movement_state = 0;
        random_ticks = randu0(60);
        drone->charge_cooldown_tick = exec_tick_ctr + random_ticks + 120;
        drone->difficulty_index = 0;
    } else if (randu0(100) < 50) {
        drone->movement_state = 3;
    } else {
        drone->movement_state = 1;
    }

    random_ticks = randu0(120);
    drone->next_style_change_tick = exec_tick_ctr + random_ticks + 180;
    drone->initialization_tick = exec_tick_ctr;
    drone->background_attack_active = 1;
    drone->block_subtype = 0;
    drone->script_attack = 0;
    drone->script_attack_ready = 0;
    drone->decision_ready = 1;
    if (get_game_state() == 3) {
        drone->difficulty_index = 7;
    }
    drone->special_reaction_active = 0;
    drone->block_request = 1;
    drone->big_boss_stage_hits = 0;
    drone->damage_transition_tick = 0;
    drone->block_hold_ticks = 0;
    drone->special_reaction_ticks = 0;
    drone->special_reaction_state = 0;
    drone->request_active = 0;
    drone->reaction_watcher = 0;
    drone->attack_type = 0;
    drone->force_attack = 0;
    drone->evade_arena_state = 0;
    drone->danger_area_request = 0;
    drone->attack_pending = 0;
    drone->jump_attack_pending = 0;
    drone->request_active = 0;
    drone->attack_latched = 0;
    drone->movement_attempt = 0;
    drone->arena_collision_history = 0;
    drone->arena_collision_count = 0;
    drone->danger_area_active = 0;
    drone->danger_area_request = 0;
    drone->danger_area_ready = 0;
    drone->taunt_pending = 0;
    drone->hit_active = 0;
    drone->fatality_decision = 0;
    drone->command_active = 0;
    drone->super_combo_active = 0;
    drone->avoid_position_request = 0;
    drone->avoid_position_ready = 0;
    drone->danger_area_state = 0;
    drone->reversal_pending = 0;
    drone->opponent_round_attacks = 0;
    drone->attack_disable_request = 1;
    drone->push_attempts = 4;
    drone->danger_area_counter = 0;
    drone->duck_reaction_tick = 0;
    drone->next_special_voice_tick = 0;
    drone->ai_command = 0;
    drone->ai_command_arg = 0;
    drone->ai_command_target = -1;
    drone->ai_command_value = 0.0f;
    drone->walk_ticks = 0.0f;
    drone->ai_command_flag0 = 0;
    drone->ai_command_flag1 = 0;
    drone->ai_command_flag2 = 0;
    drone->avoidance_area_duration = 0.0f;
    drone->avoidance_position[2] = 0.0f;
    drone->avoidance_position[1] = 0.0f;
    drone->avoidance_position[0] = 0.0f;
    drone->block_retry_tick = 0;
    drone->field_150 = 0;
}

void drone_super_combo_refresh(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->super_combo_active = 0;
}

void drone_ai_perform_block(void) {
    plyr_pdata->state = 0xA00;
    plyr_pdata->block_start_tick = exec_tick_ctr;
    ((AiProcVtable*)aproc->vtbl)->transfer(x_block, 0.0f);
}

void drone_ai_get_min_time_in_block(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    handicap_calc_min_time_in_block(drone);
}

float walk_forward_attackdist_with_jexit(void) {
    drone_walk_FB_true(InAttackRange, 0x3C, 1, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float walk_forward_attackdist2_with_jexit(void) {
    drone_walk_FB_true(InAttackRange2, 0x3C, 1, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float walk_forward_attackdist_close_with_jexit(void) {
    drone_walk_FB_true(InAttackRange_close, 0x3C, 1, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float drone_walk_backwards_further_with_jexit(void) {
    drone_walk_FB_true(always_false, 0x19, 0, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float drone_walk_backwards_with_jexit(void) {
    drone_walk_FB_true(always_false, (randu0(3) + 1) * 10, 0, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static void ai_walk_footstep(void) {
    unsigned short sound;

    if (is_big_boss(plyr_pdata)) {
        sound = randu0(100);
        if (sound < 33) {
            snd_req(0x1B0);
        } else if (sound < 66) {
            snd_req(0x1B1);
        } else {
            snd_req(0x1B2);
        }
        shake_camera(1, 0.01f);
    } else {
        random_foot(1);
    }
}

void drone_walk_FB_true(
    int (*test)(void), int ticks, int forward, int finish_start) {
    AiWalkAnimationSet* animations;
    AiWalkMovementData* movement;
    float elapsed;
    float duration;

    elapsed = 0.0f;
    duration = (float)ticks;
    animations = (AiWalkAnimationSet*)plyr_pdata->fighter_definition;
    movement = animations->movement;
    init_ground_move_no_aniproc();
    rotate_towards_him(0.2f);
    if (plyr_pdata->character_id == 0x10 &&
        (plyr_pdata->plyr_info->field_14 & 1) == 0 &&
        randu0(100) < 15) {
        snd_req_delay(randu0(5) + 0x27B, randu0(20) + 1);
    }
    if (ticks == 0 || test() == 1) {
        blend_to_stance(0.1f);
        return;
    }

    plyr_anim_pdata->flags |= 0x40;
    if (forward) {
        set_my_state(0x2000);
        blend_to_ani(animations->forward_start, 0x23, 0.2f);
        plyr_anim_pdata->step = movement->forward_start_step;
        plyr_anim_pdata->weight = movement->forward_weight;
    } else {
        set_my_state(0x2001);
        blend_to_ani(animations->backward_start, 0x23, 0.2f);
        plyr_anim_pdata->step = movement->backward_start_step;
        plyr_anim_pdata->weight = movement->backward_weight;
    }

    while (test() == 0 &&
           (finish_start ||
            elapsed < duration) &&
           plyr_anim_pdata->frame <=
               plyr_anim_pdata->high_frame - 13.0f) {
        face_opponent_now();
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        AI_SLEEP(1.0f);
        elapsed += game_speed;
    }
    if (plyr_anim_pdata->frame > 5.0f) {
        ai_walk_footstep();
    }

    if (test() == 0 && elapsed < duration) {
        if (forward) {
            blend_to_ani(animations->forward_loop, 0, 0.2f);
            plyr_anim_pdata->step = movement->forward_loop_step;
        } else {
            blend_to_ani(animations->backward_loop, 0, 0.2f);
            plyr_anim_pdata->step = movement->backward_loop_step;
        }
    }
    while (test() == 0 && elapsed < duration) {
        if (is_big_boss(plyr_pdata) &&
            ((plyr_anim_pdata->frame > 15.8f &&
              plyr_anim_pdata->frame < 16.5f) ||
             (plyr_anim_pdata->frame > 36.8f &&
              plyr_anim_pdata->frame < 37.5f))) {
            ai_walk_footstep();
        }
        face_opponent_now();
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        AI_SLEEP(1.0f);
        elapsed += game_speed;
    }
    if (is_big_boss(plyr_pdata)) {
        ai_walk_footstep();
    }
    blend_to_stance(0.1f);
}

void drone_ai_ok_to_think(void) {
    if (g_game_info.plyr0.player_state == 0) {
        xfer_proc(g_game_info.plyr0.field_68, drone_ai_watcher);
    }
    if (g_game_info.plyr1.player_state == 0) {
        xfer_proc(g_game_info.plyr1.field_68, drone_ai_watcher);
    }
}

void drone_ai_dont_think(void) {
    if (g_game_info.plyr0.player_state == 0) {
        xfer_proc(g_game_info.plyr0.field_68, drone_ai_stupid_watcher);
    }
    if (g_game_info.plyr1.player_state == 0) {
        xfer_proc(g_game_info.plyr1.field_68, drone_ai_stupid_watcher);
    }
}

void drone_ai_perform_reversal(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void drone_ai_perform_impale_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void drone_ai_perform_low_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->attack_type = 3;
    ((AiProcVtable*)aproc->vtbl)
        ->transfer(drone_ai_perform_attack, 0.0f);
}

float change_to_weapon_style_with_j_exit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, 2);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

void drone_ai_reset_all(void) {
    DroneAI* drone;

    drone = get_player_number(g_game_info.plyr0.slot.mirror_a) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_initialize(drone);
    drone = get_player_number(g_game_info.plyr1.slot.mirror_a) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_initialize(drone);
}

void jump_away_opponent_with_jexit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    jump_away_opponent();
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

int drone_ai_check_button_press(int button) {
    DroneAI* drone;
    int command_button;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        drone =
            get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
        drone->ai_command = 0;
        drone->ai_command_arg = 0;
        drone->ai_command_target = -1;
        drone->ai_command_value = 0.0f;
        drone->ai_command_flag0 = 0;
        drone->ai_command_flag1 = 0;
        drone->ai_command_flag2 = 0;
        drone->command_active = 0;
        return 0;
    }
    if (drone->ai_command != 0) {
        command_button = -2;
        switch (button) {
        case 7:
            command_button = 0;
            break;
        case 4:
            command_button = 5;
            break;
        case 6:
            command_button = 10;
            break;
        case 5:
            command_button = 15;
            break;
        case 3:
            command_button = 20;
            break;
        }
        if (command_button ==
            ((int)drone->ai_command[drone->ai_command_arg] / 5) * 5) {
            if (drone_ai_check_continue_combo() != 0) {
                return 1;
            }
            drone = get_player_number(plyr_obj) == 0
                        ? &g_DroneAI1
                        : &g_DroneAI2;
            drone->ai_command = 0;
            drone->ai_command_arg = 0;
            drone->ai_command_target = -1;
            drone->ai_command_value = 0.0f;
            drone->ai_command_flag0 = 0;
            drone->ai_command_flag1 = 0;
            drone->ai_command_flag2 = 0;
            drone->command_active = 0;
        }
    }
    return 0;
}

int drone_ai_check_button_direction(int direction) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        drone =
            get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
        drone->ai_command = 0;
        drone->ai_command_arg = 0;
        drone->ai_command_target = -1;
        drone->ai_command_value = 0.0f;
        drone->ai_command_flag0 = 0;
        drone->ai_command_flag1 = 0;
        drone->ai_command_flag2 = 0;
        drone->command_active = 0;
        return 0;
    }
    if (drone->ai_command != 0 &&
        direction == (int)drone->ai_command[drone->ai_command_arg] % 5) {
        if (drone_ai_check_continue_combo() != 0) {
            return 1;
        }
        drone =
            get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
        drone->ai_command = 0;
        drone->ai_command_arg = 0;
        drone->ai_command_target = -1;
        drone->ai_command_value = 0.0f;
        drone->ai_command_flag0 = 0;
        drone->ai_command_flag1 = 0;
        drone->ai_command_flag2 = 0;
        drone->command_active = 0;
    }
    return 0;
}

void advance_cur_cmd_idx(void) {
    DroneAI* drone;
    int index;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->ai_command_arg++;
    index = drone->ai_command_arg;
    if (index >= 16) {
        drone =
            get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
        drone->ai_command = 0;
        drone->ai_command_arg = 0;
        drone->ai_command_target = -1;
        drone->ai_command_value = 0.0f;
        drone->ai_command_flag0 = 0;
        drone->ai_command_flag1 = 0;
        drone->ai_command_flag2 = 0;
        drone->command_active = 0;
        return;
    }
    if ((int)drone->ai_command[index] == -1) {
        drone =
            get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
        drone->ai_command = 0;
        drone->ai_command_arg = 0;
        drone->ai_command_target = -1;
        drone->ai_command_value = 0.0f;
        drone->ai_command_flag0 = 0;
        drone->ai_command_flag1 = 0;
        drone->ai_command_flag2 = 0;
        drone->command_active = 0;
    }
}

void drone_ai_reset_ai_cmd(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->ai_command = 0;
    drone->ai_command_arg = 0;
    drone->ai_command_target = -1;
    drone->ai_command_value = 0.0f;
    drone->ai_command_flag0 = 0;
    drone->ai_command_flag1 = 0;
    drone->ai_command_flag2 = 0;
    drone->command_active = 0;
}

void jump_towards_opponent_with_j_exit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    jump_towards_opponent();
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void drone_ai_scripted_change_style(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, -1);
    ((AiProcVtable*)aproc->vtbl)->transfer(drone_entry, 0.0f);
}

void jump_towards_opponent_with_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    jump_towards_opponent();
    drone->jump_attack_pending = 1;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void drone_ai_special_attack_now(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->force_attack = 1;
    drone_ai_check_attack(drone, 1, 1);
    drone->force_attack = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void drone_ai_change_style(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, randu0(3));
    ((AiProcVtable*)aproc->vtbl)->transfer(drone_entry, 0.0f);
}

int drone_ai_check_reversal_restrictions(void) {
    int state;

    state = his_pdata->state;
    if (state == 0x120B || state == 0x120C ||
        state == 0x1210 || state == 0x1211 ||
        state == 0x3204 || state == 0x3205) {
        return 0;
    }
    if (is_he_airborn() == 1) {
        return 0;
    }
    return is_weapon_style(his_pdata->fighter_definition) != 0;
}

void drone_ai_set_avoidance_area(const float* position, float duration) {
    PlyrPdata* opponent;
    DroneAI* drone;

    if (plyr_pdata != 0) {
        opponent = plyr_pdata->his_plyr_pdata;
        if (opponent->drone_request != 0) {
            drone = opponent->plyr_num == 0 ? &g_DroneAI1 : &g_DroneAI2;
            if (duration != 0.0f) {
                drone->avoidance_area_duration = duration;
                drone->avoidance_position[0] = position[0];
                drone->avoidance_position[1] = position[1];
                drone->avoidance_position[2] = position[2];
            }
        }
    }
}

void drone_ai_perform_jump_attack(AiRequest* request) {
    DroneAI* drone;
    MkProc* player_proc;

    request->active = 0;
    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    player_proc = get_player_number(plyr_obj) == 0
                      ? g_game_info.plyr0.idle_proc
                      : g_game_info.plyr1.idle_proc;
    xfer_proc(player_proc, jump_towards_opponent_with_jexit);
    drone->request_active = 1;
    set_my_state(0);
}

void drone_ai_evade(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_for_evade_attack(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_evade_movement(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return;
    }
    drone_ai_check_for_throw(drone);
}

void dash_back_with_jexit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (plyr_pdata != 0 && is_big_boss(plyr_pdata) != 0) {
        step_backward();
    } else {
        joy_dash_back();
    }
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void drone_ai_counter_attack_now(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_attack(drone, 1, 1) == 1) {
        ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
        return;
    }
    jump_towards_opponent();
    drone->jump_attack_pending = 1;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void walk_backward_walk_ticks_jexit(void) {
    DroneAI* drone;
    int ticks;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    ticks = (int)drone->walk_ticks;
    if (ticks > 0) {
        if (ticks > 200) {
            ticks = 200;
        }
        drone_walk_FB_true(InAttackRange, ticks, 0, 0);
        drone->walk_ticks = 0.0f;
    }
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
}

void drone_ai_perform_weapon_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, 2);
    if (plyr_pdata->player_slot != 2) {
        ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
        return;
    }
    drone->attack_type = 1;
    ((AiProcVtable*)aproc->vtbl)
        ->transfer(drone_ai_perform_attack, 0.0f);
}

void drone_ai_passive(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_check_for_passive_movement(drone);
}

void drone_ai_defend(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (randu0(100) < 60) {
        if (drone_ai_check_attack(drone, 0, 0) == 1) {
            return;
        }
    } else if (drone_ai_check_push(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return;
    }
    drone_ai_check_for_defend_movement(drone);
}

void drone_ai_dodge_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_change_style(drone) == 1) {
        return;
    }
    if (randu0(8) == 0 &&
        drone_ai_check_attack(drone, 1, 0) == 1) {
        return;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return;
    }
    drone_ai_check_for_dodge_movement(drone);
}

void drone_ai_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_change_style(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return;
    }
    if (drone_ai_check_attack(drone, 0, 0) == 1) {
        return;
    }
    if (drone_ai_check_for_throw(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_aggressive_movement(drone) == 1) {
        return;
    }
    drone_ai_check_attack(drone, 0, 0);
}

void drone_ai_berserk(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_for_berserker_movement(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_throw(drone) == 1) {
        return;
    }
    if (randu0(100) < 50 && drone_ai_check_push(drone) == 1) {
        return;
    }
    drone_ai_check_attack(drone, 1, 0);
}

int drone_ai_check_for_dodge_movement(DroneAI* request) {
    DroneAI* drone;
    MkProc* player_proc;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    player_proc = get_player_number(plyr_obj) == 0
                      ? g_game_info.plyr0.idle_proc
                      : g_game_info.plyr1.idle_proc;
    if (request->opponent_distance > 5.9457946f) {
        xfer_proc(player_proc, walk_forward_attackdist_with_jexit);
    } else {
        xfer_proc(player_proc, side_step_to_center_attack_with_jexit);
    }
    drone->request_active = 1;
    return 1;
}

void drone_ai_check_evade_arena(DroneAI* request) {
    DroneAI* drone;
    MkProc* player_proc;

    if (request->evade_arena_state == 1) {
        drone = get_player_number(plyr_obj) == 0
                    ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? g_game_info.plyr0.idle_proc
                          : g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, side_step_to_center_with_jexit);
        drone->request_active = 1;
        request->evade_arena_state = 2;
        return;
    }
    if (request->evade_arena_state == 2) {
        request->evade_arena_state = 0;
    }
}

int drone_ai_check_push(DroneAI* request) {
    DroneAI* drone;
    MkProc* player_proc;

    if (drone_ai_can_push(request) == 1 &&
        request->opponent_distance < 4.378056f &&
        randu0(100) < 8) {
        drone = get_player_number(plyr_obj) == 0
                    ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? g_game_info.plyr0.idle_proc
                          : g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, drone_ai_perform_push);
        drone->request_active = 1;
        return 1;
    }
    return 0;
}

void drone_ai_victim_speared(void) {
    DroneAI* drone;
    MkProc* player_proc;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4204) {
        drone->reaction_watcher = 0;
        return;
    }
    if (randu0(3) == 0) {
        player_proc = get_player_number(plyr_obj) == 0
                          ? g_game_info.plyr0.idle_proc
                          : g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, change_to_weapon_style_with_j_exit);
        drone->request_active = 1;
    }
    drone->reaction_watcher = drone_ai_victim_speared_2;
}

void drone_ai_victim_speared_2(void) {
    DroneAI* drone;
    MkProc* player_proc;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4204) {
        drone->reaction_watcher = 0;
        return;
    }
    player_proc = get_player_number(plyr_obj) == 0
                      ? g_game_info.plyr0.idle_proc
                      : g_game_info.plyr1.idle_proc;
    if (drone->opponent_distance > 13.378037f) {
        xfer_proc(player_proc, jump_towards_opponent_with_jexit);
        drone->request_active = 1;
        return;
    }
    if (xz_distance_between_players() >= 1.4864486f) {
        xfer_proc(player_proc, walk_forward_attackdist_with_jexit);
        drone->request_active = 1;
        return;
    }
    drone_ai_attacker_defenseless(drone);
}

void drone_ai_victim_slipping_on_vomit(void) {
    DroneAI* drone;
    MkProc* player_proc;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4207) {
        drone->reaction_watcher = 0;
        return;
    }
    player_proc = get_player_number(plyr_obj) == 0
                      ? g_game_info.plyr0.idle_proc
                      : g_game_info.plyr1.idle_proc;
    if (drone->opponent_distance > 13.378037f) {
        xfer_proc(player_proc, jump_towards_opponent_with_jexit);
        drone->request_active = 1;
        return;
    }
    if (xz_distance_between_players() >= 1.4864486f) {
        xfer_proc(player_proc, walk_forward_attackdist_with_jexit);
        drone->request_active = 1;
        return;
    }
    drone_ai_attacker_defenseless(drone);
}

void drone_ai_check_external_requests(DroneAI* request) {
    DroneAI* drone;
    MkProc* player_proc;

    player_proc = get_player_number(plyr_obj) == 0
                      ? g_game_info.plyr0.idle_proc
                      : g_game_info.plyr1.idle_proc;
    if (request->danger_area_request == 1 &&
        request->danger_area_ready == 1) {
        drone = get_player_number(plyr_obj) == 0
                    ? &g_DroneAI1 : &g_DroneAI2;
        xfer_proc(player_proc, drone_ai_avoid_danger_area_now);
        drone->request_active = 1;
        request->danger_area_request = 0;
        return;
    }
    if (request->avoid_position_request == 1 &&
        request->avoid_position_ready == 1) {
        drone = get_player_number(plyr_obj) == 0
                    ? &g_DroneAI1 : &g_DroneAI2;
        xfer_proc(player_proc, drone_ai_avoid_position_now);
        drone->request_active = 1;
        request->avoid_position_request = 0;
    }
}

void drone_ai_knockdown(void) {
    DroneAI* drone;
    MkProc* player_proc;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_for_knockdown_movement(drone) == 1) {
        return;
    }
    if ((plyr_pdata->his_plyr_pdata->state & 0x100) != 0) {
        return;
    }
    if (drone->opponent_distance > 5.9457946f) {
        return;
    }
    if (drone->opponent_distance > 2.8103173f &&
        randu0(100) < 90) {
        return;
    }
    if (randu0(100) < 50 && drone_ai_check_push(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return;
    }
    if (drone_ai_check_for_throw(drone) == 1) {
        return;
    }
    player_proc = get_player_number(plyr_obj) == 0
                      ? g_game_info.plyr0.idle_proc
                      : g_game_info.plyr1.idle_proc;
    xfer_proc(player_proc, drone_ai_perform_knockdown);
    drone->request_active = 1;
}

void drone_ai_process_background_states(DroneAI* request) {
    DroneAI* drone;
    MkProc* player_proc;
    float distance;

    if (request->background_attack_active != 0) {
        return;
    }
    distance = randu0(100) < 50 ? 2.5f : 1.5f;
    if (request->opponent_distance < distance * distance &&
        am_i_airborn() != 0) {
        request->background_attack_active = 1;
        drone = get_player_number(plyr_obj) == 0
                    ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? g_game_info.plyr0.idle_proc
                          : g_game_info.plyr1.idle_proc;
        if (am_i_a_big_character() != 0) {
            xfer_proc(player_proc, j_flying_kick);
        } else {
            xfer_proc(player_proc, j_flying_kick2);
        }
        drone->request_active = 1;
    }
}

void execute_rumble(int reaction, int flags) {
    int strength;
    int scale;
    int pad;

    strength = 0;
    scale = 0;
    if ((flags & 4) != 0) {
        strength = 5;
        scale = 1;
    }
    if ((flags & 8) != 0) {
        strength = 10;
        scale = 3;
    }
    if (strength == 0) {
        switch (reaction) {
        case 1:
            strength = 8;
            scale = 2;
            break;
        case 2:
            strength = 10;
            scale = 2;
            break;
        case 4:
            strength = 8;
            scale = 3;
            break;
        case 5:
        case 6:
            strength = 4;
            scale = 1;
            break;
        default:
            break;
        }
    }
    if (scale != 0) {
        if (strength > 10) {
            strength = 10;
        }
        if (aproc->pid == 0x1001) {
            pad = g_game_info.plyr0.pad_index;
        } else if (aproc->pid == 0x1002) {
            pad = g_game_info.plyr1.pad_index;
        } else {
            return;
        }
        ck_rumble_controller(pad, strength, scale * 15);
    }
}

void force_ai_style(int style) {
}

void generate_ai_table_player(FighterMirror* player) {
    FighterAiTable* table;
    int index;

    table = player->ai_tables->tables;
    for (index = 0; index < 14; index++, table++) {
        unsigned int row_count;

        table->usable_row_count = 0;
        if (table->rows == 0) {
            continue;
        }
        row_count = get_row_count_for_table_by_pointer(
            player->cmo, table->rows);
        if (row_count == 0) {
            table->usable_row_count = 0;
        } else {
            table->usable_row_count = row_count - 1;
        }
        if (table->usable_row_count > 0 &&
            table->rows[table->usable_row_count].move_id != -1) {
            table->usable_row_count++;
        }
    }
}

void generate_ai_table_moveset(AiMoveset* moveset) {
    FighterAiTable* table;
    int index;

    table = moveset->ai_tables->tables;
    for (index = 0; index < 14; index++, table++) {
        unsigned int row_count;

        table->usable_row_count = 0;
        if (table->rows == 0) {
            continue;
        }
        row_count = get_row_count_for_table_by_pointer(
            moveset->cmo, table->rows);
        if (row_count == 0) {
            table->usable_row_count = 0;
        } else {
            table->usable_row_count = row_count - 1;
        }
        if (table->usable_row_count > 0 &&
            table->rows[table->usable_row_count].move_id != -1) {
            table->usable_row_count++;
        }
    }
}

int InAttackRange2(void) {
    return xz_distance_between_players() < 3.3445094f;
}

int InAttackRange(void) {
    return xz_distance_between_players() < 1.4864486f;
}

int InAttackRange_close(void) {
    return xz_distance_between_players() < 1.0f;
}

MkProc* get_player_proc(void* player) {
    if (get_player_number(player) == 0) {
        return (MkProc*)g_game_info.plyr0.idle_proc;
    }
    return (MkProc*)g_game_info.plyr1.idle_proc;
}

int HeIsNotFacing(void) {
    Vec to_opponent;
    Vec facing;
    float dot;

    uv_to_opponent(&to_opponent);
    uv_from_angle_y(&facing, his_obj->ang.y);
    dot = to_opponent.x * facing.x + to_opponent.z * facing.z;
    return dot > -0.86f;
}

void dead_liukang_snd_chain_check(
    PlyrPdata* player, int base_delay, unsigned short delay_range,
    unsigned int likelihood) {
    int delay;

    if (player->character_id == 0x10 &&
        !player->plyr_info->flags_14_bits.alternate_costume &&
        randu0(100) < likelihood) {
        delay = base_delay + randu0(delay_range);
        snd_req_delay(randu0(5) + 0x27B, delay + 1);
    }
}

void dk_taunt_at_screen(void) {
    PlyrPdata* player;
    MkProc* proc;

    player = plyr_pdata;
    proc = player->player_proc;
    if (proc != 0 && proc->instance != player->player_proc_instance) {
        proc = 0;
    }
    xfer_proc(proc, dk_screen_taunt);
}

int drone_ai_check_combo_breaker(void) {
    DroneAI* drone;
    int likelihood;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    likelihood = handicap_likelihood_for_combo_breaker(drone);
    return randu0(100) < likelihood;
}

void drone_ai_scripted_special_attack(void) {
    pre_attack_chores();
    plyr_going_to_attack_with_action(active_cmdscript->unk28);
    share_my_attack_info(2.0f, 0.3f);
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->cmo);
    AI_TRANSFER(j_exit);
}

int drone_ai_can_push(DroneAI* drone) {
    PlyrPdata* action;

    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr ||
        action->action_lock_a > game_tick_ctr ||
        action->push_blocked != 0 || (action->state & 0x200) != 0) {
        return 0;
    }
    return drone->player->character_id == 0;
}

void go_into_twitch_death(void) {
    PlyrPdata* death;
    AiAnimPdataView* anim_view;

    death = plyr_pdata;
    init_ground_move_no_aniproc();
    if (death->death_type == 4) {
        plyr_obj->flags_09_bits.head_tracking = 0;
        tightrope_restrictions_off(plyr_obj, 0);
        plyr_anim_pdata->step = 1.0f;
        anim_view = (AiAnimPdataView*)plyr_anim_pdata;
        anim_view->twitch_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.twitch_death, 0, 0.05f);
        AI_SLEEP(1.0f);
        ani_to_frame_x(2.0f);
        plyr_obj->flags_09_bits.launched = 0;
        xfer_proc(plyr_anim_proc, p_animate);
    }
    AI_TRANSFER(j_stay_down_dead);
}

float drone_ai_perform_push(void) {
    DroneAI* drone;
    AiWeaponStyleView* style;
    AiMoveBlendScripts* scripts;
    int style_index;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    style = (AiWeaponStyleView*)plyr_pdata->weapon_styles[0];
    if (style->style_id == 5) {
        style_index = 0;
    } else {
        style = (AiWeaponStyleView*)plyr_pdata->weapon_styles[1];
        style_index = style->style_id == 5 ? 1 : -1;
    }
    if (style_index < 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    drone_ai_force_change_style(drone, style_index);
    if (plyr_pdata->player_slot != style_index) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    scripts =
        (AiMoveBlendScripts*)plyr_pdata->fighter_definition->move_blend_data;
    drone->script_attack = &scripts->push_script;
    AI_TRANSFER(drone_ai_perform_script_attack);
    return 0.0f;
}

int drone_ai_check_change_style(DroneAI* drone) {
    MkObj* player_object;
    MkProc* player_proc;
    DroneAI* active_drone;

    if (drone->next_style_change_tick > exec_tick_ctr ||
        (g_DroneOverrideInfo.flags & 2) != 0) {
        return 0;
    }
    drone->next_style_change_tick =
        exec_tick_ctr + 120 + randu0(4) * 60;
    player_object = drone->player->plyr_info->slot.mirror_a;
    if (player_object->hide_flag_bits.hidden) {
        return 0;
    }
    if (drone->player->character_id == 0x1B) {
        if (randu0(100) < 10) {
            drone->next_style_change_tick =
                exec_tick_ctr + 60 + randu0(2) * 60;
            return 0;
        }
        drone->next_style_change_tick += 60 + randu0(2) * 60;
    }

    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    player_proc = get_player_proc(plyr_obj);
    xfer_proc(player_proc, (MkProcEntryFn)drone_ai_change_style);
    active_drone->request_active = 1;
    return 1;
}

int drone_ai_victim_dizzy_2(void) {
    DroneAI* drone;
    AiMoveBlendScripts* scripts;
    MkProc* player_proc;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return 0;
    }

    scripts =
        (AiMoveBlendScripts*)plyr_pdata->fighter_definition->move_blend_data;
    drone->script_attack = &scripts->fatality_scripts[14];
    player_proc = get_player_proc(plyr_obj);
    xfer_proc(player_proc, drone_ai_scripted_attack);
    drone->request_active = 1;
    return 1;
}

float drone_loop(void) {
    DroneAI* drone;
    PlyrPdata* action;
    int difficulty;
    int ticks;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    end_of_round_check();
    if ((g_game_info.flags & 1) != 0) {
        AI_TRANSFER(drone_entry);
    }
    action = plyr_pdata;
    if (action->action_lock_a > game_tick_ctr ||
        action->action_lock_b > game_tick_ctr ||
        action->push_blocked != 0 || (action->state & 0x200) != 0) {
        return 1.0f;
    }
    difficulty = drone->difficulty_index;
    ticks = g_minDecisionBaseWaitTime[difficulty] +
            randu0(g_randomDecisionBaseWaitTime[difficulty]);
    return (float)ticks;
}

float drone_start(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (g_game_info.pselect.field_1f4 > 1) {
        init_ground_move();
        back_to_normal();
        if (his_obj != 0 && plyr_obj != 0) {
            rotate_towards_him(0.2f);
        }
    }
    g_game_number++;
    drone->start_state_a = 0;
    drone->start_state_b = 0;
    drone_ai_initialize(drone);
    plyr_pdata->drone_request = 1;
    while (!g_game_info.flag_bits.lens_flare_enabled) {
        AI_SLEEP(1.0f);
    }
    AI_TRANSFER(drone_entry);
    return 0.0f;
}

void catch_opponent(void) {
    DroneAI* drone;
    AiSpecialMoveList* moves;
    void* script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
    script = 0;
    if (moves->count != 0) {
        drone->ai_command =
            moves->commands + (randu0(moves->count) * 16);
        drone->ai_command_arg = 0;
        drone->ai_command_target = drone->player->character_id;
        drone->ai_command_flag0 = 0;
        drone->ai_command_flag1 = 0;
        drone->ai_command_flag2 = 0;
        script = get_special_move();
    }
    if (script != 0) {
        drone->script_attack = script;
        drone->script_attack_ready = 1;
        drone_ai_perform_script_attack();
    }
    AI_TRANSFER(j_exit);
}

float dk_screen_taunt(void) {
    int frames;

    bgnd_restore_player();
    init_ground_move_no_aniproc();
    set_my_state(0x600);
    enable_all_my_blocking();
    if (!is_my_chest_to_screen()) {
        blend_to_ani(plyr_pdata->turn_to_screen_animation, 3, 0.2f);
        set_ani_speed(2.0f);
        for (frames = 29; frames != 0; --frames) {
            force_midpoint_calculation_update = 1;
            ani_loop_more_frames(1.0f);
        }
        set_ani_speed(1.0f);
        blend_to_fstance(0.1f);
    }
    snd_req(randu0(100) < 50 ? 0x1B4 : 0x1B5);
    blend_to_ani(plyr_pdata->screen_taunt_animation, 3, 0.1f);
    set_ani_speed(1.0f);
    ani_to_blend_frame(20.0f);
    set_my_state(0);
    blend_to_stance(0.1f);
    AI_TRANSFER(j_exit);
    return 0.0f;
}

void drone_ai_perform_combo_attack(void) {
    DroneAI* drone;
    AiComboTable* combo_table;
    void* script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    combo_table =
        (AiComboTable*)plyr_pdata->fighter_definition->move_blend_data;
    if ((his_pdata->state & 0x100) != 0) {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 3, 1);
    } else if (combo_table->close_attack_count > 6 &&
               xz_distance_between_players() < 7.1883736f &&
               randu0(100) < 30) {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 0, 1);
    } else if (combo_table->distant_attack_count > 6 &&
               randu0(100) < 30) {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 10, 1);
    } else {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 1, 1);
    }
    drone->command_active = 1;
    if (script == 0) {
        AI_TRANSFER(j_exit);
        return;
    }
    drone->script_attack = script;
    AI_TRANSFER(drone_ai_perform_script_attack);
}

int drone_ai_force_change_style(DroneAI* drone, int style) {
    if (is_big_boss(drone->player) ||
        (g_DroneOverrideInfo.flags & 2) != 0 ||
        drone->super_combo_active == 1 ||
        drone->command_active == 1) {
        return 0;
    }
    if ((plyr_pdata->state & 0x200) != 0 && style != 3) {
        return 0;
    }
    if (style == 3) {
        style = 0;
    }
    if (style == plyr_pdata->player_slot) {
        return 0;
    }
    if (plyr_pdata->sidekick_available != 0) {
        advance_sidekick_with_moveset(plyr_pdata);
        blend_to_stance(0.1f);
        AI_SLEEP(1.0f);
        return 1;
    }

    advance_active_moveset(plyr_pdata);
    if (style != -1 && style != plyr_pdata->player_slot) {
        advance_active_moveset(plyr_pdata);
        if (style != plyr_pdata->player_slot) {
            advance_active_moveset(plyr_pdata);
        }
    }
    snd_req(0xDC1);
    blend_to_stance(0.1f);
    AI_SLEEP(1.0f);
    return 1;
}

int drone_ai_reversal_watcher(void) {
    DroneAI* drone;
    MkProc* player_proc;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (plyr_pdata->state != 0x4000) {
        return 0;
    }
    if (drone->match_stage > 6 && randu0(100) < 80) {
        drone->reversal_pending = 1;
    } else if (drone->difficulty_index < 4 || randu0(100) < 20) {
        drone->jump_attack_pending = 1;
    } else if (drone->difficulty_index < 6 && randu0(100) < 20) {
        drone->jump_attack_pending = 1;
    } else {
        drone->reversal_pending = 1;
    }

    player_proc = get_player_proc(plyr_obj);
    xfer_proc(player_proc, (MkProcEntryFn)j_exit);
    drone->request_active = 1;
    return 1;
}

int drone_ai_attacker_defenseless_watcher(void) {
    DroneAI* drone;
    PlyrPdata* action;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4206 &&
        (unsigned int)his_pdata->state != 0xFFFFC601U) {
        return 0;
    }
    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr ||
        action->action_lock_a > game_tick_ctr ||
        action->push_blocked != 0 ||
        (action->state & 0x200) != 0 ||
        am_i_airborn() == 1) {
        return 0;
    }
    return drone_ai_attacker_defenseless(drone);
}

int drone_ai_victim_frozen(void) {
    DroneAI* drone;
    MkProc* player_proc;
    unsigned int state;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    state = (unsigned int)his_pdata->state;
    if (state != 0xFFFFC600U && state != 0x202U && state != 0x421AU) {
        drone->reaction_watcher = 0;
        return 0;
    }
    player_proc = get_player_proc(plyr_obj);
    if (drone->opponent_distance > 13.378037f) {
        xfer_proc(player_proc, jump_towards_opponent_with_jexit);
        drone->request_active = 1;
        return 1;
    }
    if (xz_distance_between_players() >= 1.4864486f) {
        xfer_proc(player_proc, walk_forward_attackdist_with_jexit);
        drone->request_active = 1;
        return 1;
    }
    return drone_ai_attacker_defenseless(drone);
}

int drone_ai_handle_arena_collisions(DroneAI* request) {
    DroneAI* drone;
    int in_wall_collision;

    if (request->evade_arena_state != 0) {
        return 0;
    }
    if (request->movement_state < 0 ||
        request->movement_state == 3 ||
        request->movement_state == 4 ||
        request->movement_state > 5) {
        request->arena_collision_count = 0;
        request->arena_collision_history = 0;
        return 0;
    }

    if ((request->arena_collision_history & 0x80000000U) != 0) {
        --request->arena_collision_count;
    }
    request->arena_collision_history <<= 1;
    in_wall_collision =
        (plyr_obj->flags_0B & 0x10) != 0 ||
        (plyr_obj->flags_09_bits.wall_restricted &&
         (plyr_pdata->state & 0x2000) != 0);
    if (in_wall_collision) {
        ++request->arena_collision_count;
        request->arena_collision_history |= 1;
    }
    if (request->arena_collision_count <= 24) {
        return 0;
    }

    request->arena_collision_count = 0;
    request->arena_collision_history = 0;
    request->evade_arena_state = 1;
    if ((plyr_pdata->state & 0x2000) != 0) {
        drone = get_player_number(plyr_obj) == 0
                    ? &g_DroneAI1 : &g_DroneAI2;
        xfer_proc(get_player_proc(plyr_obj), (MkProcEntryFn)j_exit);
        drone->request_active = 1;
        return 1;
    }
    return 0;
}

void drone_ai_attack_obstacle_now(void) {
    DroneAI* drone;
    PlyrPdata* action;
    void* script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr ||
        action->action_lock_a > game_tick_ctr ||
        action->push_blocked != 0 ||
        (action->state & 0x200) != 0) {
        AI_TRANSFER(j_exit);
        return;
    }

    plyr_obj->flags_0B |= 8;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    rotate_towards_position(&drone->obstacle_target, plyr_obj, 0, 0.2f);
    script = get_random_fightstyle_attack(
        plyr_pdata->fighter_definition, 9, 0);
    if (script == 0) {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 10, 0);
    }
    if (script == 0) {
        AI_TRANSFER(j_exit);
        return;
    }
    drone->script_attack = script;
    drone_ai_perform_script_attack();
    AI_TRANSFER(j_exit);
}

void drone_ai_check_external_request_breakouts(DroneAI* request) {
    DroneAI* drone;
    float dx;
    float dz;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (request->danger_area_request == 1 &&
        request->danger_area_ready == 0) {
        request->danger_area_ready = 1;
        if (request->request_active == 1) {
            xfer_proc(get_player_proc(plyr_obj), (MkProcEntryFn)j_exit);
            drone->request_active = 1;
        }
        return;
    }

    if (request->avoid_position_request == 1 &&
        request->avoid_position_ready == 0) {
        dx = request->avoid_position_target.x - plyr_obj->pos.value.x;
        dz = request->avoid_position_target.z - plyr_obj->pos.value.z;
        if (dx * dx + dz * dz < 1.5f) {
            request->avoid_position_ready = 1;
            if (request->request_active == 1) {
                xfer_proc(get_player_proc(plyr_obj), (MkProcEntryFn)j_exit);
                drone->request_active = 1;
            }
        }
        return;
    }

    if (request->avoid_position_ready == 1 &&
        request->request_active == 0) {
        request->avoid_position_ready = 0;
        request->avoid_position_request = 0;
    }
}

int drone_ai_opponent_inair_watcher(void) {
    DroneAI* drone;
    PlyrPdata* action;
    unsigned int likelihood;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr ||
        action->action_lock_a > game_tick_ctr ||
        action->push_blocked != 0 ||
        (action->state & 0x200) != 0 ||
        am_i_airborn() == 1 ||
        is_he_airborn() == 0 ||
        his_obj->pos.value.y < 1.0668f) {
        return 0;
    }

    likelihood = g_likelihoodOfInAirAttack[drone->difficulty_index];
    if (drone->big_boss_stage == 0) {
        likelihood = 5;
    }
    if (randu0(100) < likelihood &&
        drone_ai_enemy_inair_attack(drone) != 0) {
        return 1;
    }
    return 0;
}

float side_step_to_center_attack_with_jexit(void) {
    float right_clearance;
    float left_clearance;
    int direction;

    right_clearance = ai_side_clearance(1);
    left_clearance = ai_side_clearance(0);
    direction = right_clearance > left_clearance ? 0 : 1;
    drone_step_LR_true(
        HeIsNotFacing, (randu0(3) + 1) * 30, direction);
    AI_TRANSFER(j_exit);
    return 0.0f;
}

float side_step_to_center_long_with_jexit(void) {
    float right_clearance;
    float left_clearance;
    int direction;

    right_clearance = ai_side_clearance(1);
    left_clearance = ai_side_clearance(0);
    direction = right_clearance > left_clearance ? 0 : 1;
    drone_step_LR_true(
        always_false, (randu0(3) + 1) * 60, direction);
    AI_TRANSFER(j_exit);
    return 0.0f;
}

float side_step_to_center_with_jexit(void) {
    float right_clearance;
    float left_clearance;
    int direction;

    right_clearance = ai_side_clearance(1);
    left_clearance = ai_side_clearance(0);
    direction = right_clearance > left_clearance ? 0 : 1;
    drone_step_LR_true(
        always_false, (randu0(3) + 1) * 25, direction);
    AI_TRANSFER(j_exit);
    return 0.0f;
}

float force_some_distance(void) {
    if (ai_backward_clearance() > 2.1336f &&
        !is_big_boss(plyr_pdata)) {
        AI_TRANSFER(joy_dash_back);
    } else {
        AI_TRANSFER(j_exit);
    }
    return 0.0f;
}

float give_some_distance(void) {
    PlyrPdata* opponent;
    float maximum_distance;

    opponent = his_pdata;
    maximum_distance =
        opponent->death_type == 1 ? 9.290304f : 5.9457946f;
    if (xz_distance_between_players() > maximum_distance) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    if (ai_backward_clearance() > 2.1336f) {
        if (plyr_pdata != 0 && is_big_boss(plyr_pdata)) {
            step_backward();
            AI_TRANSFER(j_exit);
        } else {
            AI_TRANSFER(joy_dash_back);
        }
        return 0.0f;
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

int drone_ai_check_block_at_reactions(void) {
    DroneAI* drone;
    PlyrPdata* player;
    unsigned int likelihood;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (g_DroneOverrideInfo.likelihood_scale == 0.0f) {
        return 0;
    }
    if (his_pdata->repeated_action_count >
        (int)(randu0(3) + 2)) {
        if (drone->big_boss_stage == 0) {
            if (drone->match_stage > 4 && randu0(100) < 40) {
                return 1;
            }
        } else if (drone->match_stage < 3 &&
                   drone->big_boss_stage <= 2) {
            if (randu0(100) < 30) {
                return 1;
            }
        } else {
            return 1;
        }
    }

    likelihood =
        handicap_calc_likelihood_of_blocking_in_reaction(drone);
    if (randu0(100) >= likelihood) {
        return 0;
    }
    player = plyr_pdata;
    if (player->reaction_counter == his_pdata->attack_counter) {
        return 0;
    }
    drone->block_request = 1;
    drone->block_subtype = 0;
    drone->attack_pending = 1;
    drone->request_active = 1;
    return 1;
}

int drone_ai_should_roll(int aggressive) {
    DroneAI* drone;
    int likelihood;

    if (plyr_pdata->drone_request == 0) {
        return 1;
    }
    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    likelihood = g_likelihoodOfRolling[drone->difficulty_index];
    if (drone->movement_state == 2 || drone->movement_state == 3) {
        likelihood += 15;
    }
    if (aggressive == 1) {
        likelihood += 10;
    }
    if (randu0(100) < 50) {
        likelihood += randu0(10);
    } else {
        likelihood -= randu0(20);
    }
    if (likelihood < 0) {
        likelihood = 0;
    }
    return randu0(100) < (unsigned int)likelihood;
}

int handicap_likelihood_for_combo_breaker(DroneAI* drone) {
    int likelihood;

    likelihood = g_likelihoodForComboBreaker[drone->difficulty_index];
    if (get_game_state() == 3 || drone->big_boss_stage < 2) {
        return 1;
    }
    if (mode_of_play == 10) {
        if (drone->big_boss_stage == 2) {
            return 20;
        }
        if (drone->big_boss_stage == 3) {
            return 35;
        }
        return 50;
    }
    if (drone->match_stage == 0) {
        return 0;
    }
    if (drone->big_boss_stage == 2 && drone->match_stage < 3) {
        return 2;
    }
    if (drone->reaction_scale > 0.45f) {
        likelihood /= 2;
    }
    if (drone->reaction_ticks > 30) {
        likelihood /= 2;
    }
    if (drone->movement_state == 2) {
        likelihood += 3;
    }
    if (drone->start_state_a != 0 || drone->start_state_b != 0) {
        likelihood += 20;
    }
    if (drone->match_mode == 1 && drone->player->breaker_strength == 1) {
        likelihood = 0;
    }
    if (drone->player->his_plyr_pdata->plyr_info->slot.mirror_a
            ->hide_flag_bits.hidden) {
        likelihood += 10;
    }
    return likelihood;
}

int drone_ai_check_escape_restrictions(void) {
    return ai_backward_clearance() > 2.1336f;
}

unsigned int handicap_calc_likelihood_of_blocking_in_reaction(
    DroneAI* drone) {
    int likelihood;

    likelihood =
        g_likelihoodOfBlockingInReaction[drone->difficulty_index];
    if (drone->movement_state == 1) {
        likelihood -= 2;
    } else if (drone->movement_state == 2) {
        likelihood += 6;
    } else if (drone->movement_state == 3) {
        likelihood += 3;
    }
    if (randu0(100) < 50) {
        likelihood += randu0(3);
    } else {
        likelihood -= randu0(3);
    }
    if (likelihood < 0) {
        likelihood = 0;
    }
    if (drone->match_stage == 0 && drone->difficulty_index < 3) {
        likelihood = 5;
    }
    if (get_game_state() == 3) {
        likelihood += 5;
    } else if (his_pdata->state == 0x1219) {
        if (drone->big_boss_stage == 0) {
            likelihood = 2;
        } else if (drone->match_stage < 3) {
            likelihood +=
                (drone->big_boss_stage > 2 ? 5 : 2) *
                (drone->match_stage + 1);
        } else if (drone->difficulty_index < 4) {
            likelihood += 35;
        } else {
            likelihood += 55;
        }
    }
    if (drone->player->his_plyr_pdata->plyr_info->slot.mirror_a
            ->hide_flag_bits.hidden) {
        if (drone->big_boss_stage == 4) {
            likelihood -= 8;
        } else {
            likelihood /= 2;
        }
    }
    return (unsigned int)(
        (float)likelihood * g_DroneOverrideInfo.likelihood_scale);
}

int drone_ai_attacker_reacting_watcher(void) {
    DroneAI* drone;
    PlyrPdata* action;
    unsigned int likelihood;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x600 || drone->movement_state != 1) {
        return 0;
    }
    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr ||
        action->action_lock_a > game_tick_ctr ||
        action->push_blocked != 0 ||
        (action->state & 0x200) != 0) {
        return 0;
    }
    likelihood =
        g_likelihoodOfReactAttack[drone->difficulty_index];
    if (drone->big_boss_stage > 2) {
        likelihood += 5;
    }
    if (drone->match_stage == 0) {
        likelihood = 0;
    }
    if (randu0(100) < likelihood) {
        xfer_proc(
            get_player_proc(plyr_obj),
            (MkProcEntryFn)drone_ai_special_attack_now);
        drone->request_active = 1;
        return 1;
    }
    return 0;
}

void drone_ai_perform_range_attack(void) {
    DroneAI* drone;
    AiSpecialMoveList* moves;
    void* script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
    if (drone->opponent_distance > 14.6f) {
        script =
            ai_pick_special_move(drone, moves->commands, moves->count);
    } else {
        script = ai_pick_special_move(
            drone, moves->ranged_commands, moves->ranged_count);
    }
    if (script != 0) {
        drone->script_attack = script;
        drone->script_attack_ready = 1;
        drone_ai_perform_script_attack();
    }
    AI_TRANSFER(j_exit);
}

int drone_ai_check_for_special_move_reaction(DroneAI* drone) {
    int opponent_state;

    if (drone->attack_pending == 1 ||
        drone->special_reaction_active == 1 ||
        !drone_ai_should_be_blocking(1)) {
        return 0;
    }
    opponent_state = his_pdata->secondary_state;
    switch (opponent_state) {
    case 0x101:
        return drone_ai_check_projectile_head_on(drone);
    case 0x102:
        return drone_ai_check_projectile_side(drone);
    case 0x513:
        return drone_ai_check_propel_attack(drone);
    case 0x103:
        return drone_ai_check_dont_touch_attack_phase1(drone);
    case 0x10A:
        return drone_ai_check_dont_touch_attack_phase2(drone);
    case 0x10B:
        return 1;
    case 0x104:
        return 0;
    case 0x105:
        if ((unsigned int)drone->block_subtype != 0x107U) {
            return drone_ai_check_from_ground_attack_phase1(drone);
        }
        break;
    case 0x106:
        if ((unsigned int)drone->block_subtype == 0x107U) {
            return drone_ai_check_from_ground_attack_phase2(drone);
        }
        break;
    case 0x109:
        return drone_ai_check_all_over_ground(drone);
    case 0x108:
        return drone_ai_check_all_over_ground_phase1(drone, 10);
    case 0x10F:
        return drone_ai_check_all_over_ground_phase1(drone, 5);
    case 0x10C:
        return drone_ai_check_cant_dodge_attack(drone);
    case 0x10D:
        return drone_ai_check_cant_dodge_attack2(drone);
    case 0x10E:
        return drone_ai_check_avoid_danger_area(drone);
    case 0x111:
        drone->special_reaction_ticks = randu0(10) + 5;
        drone->special_reaction_state = 0x110;
        drone->block_subtype = 0x201;
        drone->special_reaction_active = 1;
        break;
    case 0x112:
        return drone_ai_check_mid_high_spinner(drone);
    }
    if (plyr_pdata->secondary_state == 0x109) {
        return drone_ai_check_all_over_ground(drone);
    }
    if (plyr_pdata->secondary_state == 0x110) {
        return drone_ai_check_attack_from_above(drone);
    }
    return 0;
}

int drone_ai_check_for_extreme_throw(DroneAI* drone) {
    if (drone->opponent_distance > 3.1252584f ||
        !drone_ai_check_throw_restrictions() ||
        drone->block_hold_ticks <=
            g_minExtremeBlockHeldTime[drone->difficulty_index]) {
        return 0;
    }
    return ai_start_throw(drone);
}

int drone_ai_check_for_throw(DroneAI* drone) {
    unsigned int minimum_ticks;
    unsigned int likelihood;

    if (drone->opponent_distance > 3.1252584f ||
        !drone_ai_check_throw_restrictions() ||
        drone->big_boss_stage == 0 ||
        randu0(100) < 70) {
        return 0;
    }
    minimum_ticks = g_minBlockHeldTime[drone->difficulty_index];
    likelihood = g_likelihoodToThrow[drone->difficulty_index];
    if (drone->block_hold_ticks <= minimum_ticks &&
        randu0(100) >= likelihood) {
        return 0;
    }
    return ai_start_throw(drone);
}

int drone_ai_push_watcher(DroneAI* drone) {
    unsigned int minimum_ticks;

    minimum_ticks = g_minBlockHiHeldTime[drone->difficulty_index];
    if (!drone_ai_can_push(drone) ||
        drone->opponent_distance >= 4.378056f) {
        return 0;
    }
    if (drone->block_hold_ticks > minimum_ticks &&
        (his_pdata->state & 0x100) == 0 &&
        randu0(100) < 60) {
        xfer_proc(get_player_proc(plyr_obj), drone_ai_perform_push);
        drone->request_active = 1;
        return 1;
    }
    if (drone->reaction_scale <= 0.65f &&
        drone->difficulty_index < 8 &&
        drone->push_attempts > 0 &&
        randu0(100) < 15) {
        --drone->push_attempts;
        if (drone->push_attempts < 0) {
            drone->push_attempts = 0;
        }
        xfer_proc(get_player_proc(plyr_obj), drone_ai_perform_push);
        drone->request_active = 1;
        return 1;
    }
    return 0;
}

int drone_ai_check_from_ground_attack_phase2(DroneAI* drone) {
    DroneAI* active_drone;
    unsigned int roll;

    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    if (drone->opponent_distance < 2.8103173f &&
        randu0(100) < 75) {
        xfer_proc(
            get_player_proc(plyr_obj),
            drone_ai_dodge_3d_with_counter);
        active_drone->request_active = 1;
        drone->block_subtype = 0;
        return 1;
    }
    if (drone->opponent_distance >= 3.3445094f) {
        roll = randu0(100);
        if (roll < 50 ||
            (roll < 75 && drone->difficulty_index < 3)) {
            xfer_proc(
                get_player_proc(plyr_obj),
                (MkProcEntryFn)jump_towards_opponent_with_j_exit);
            active_drone->request_active = 1;
        } else {
            drone_ai_perform_jump_attack((AiRequest*)drone);
        }
        drone->block_subtype = 0;
        return 1;
    }
    drone->block_subtype = 0;
    return 0;
}

int drone_ai_check_for_evade_attack(DroneAI* drone) {
    DroneAI* active_drone;

    if (drone->opponent_distance < 5.9457946f) {
        if (ai_backward_clearance() <= 2.1336f) {
            return 0;
        }
        if (randu0(3) == 0) {
            return drone_ai_check_for_side_step_counter_attack(drone) == 1;
        }
        if (drone_ai_should_evade_attack(drone) == 1) {
            drone->attack_type = 0;
            active_drone = get_player_number(plyr_obj) == 0
                               ? &g_DroneAI1 : &g_DroneAI2;
            xfer_proc(get_player_proc(plyr_obj), drone_ai_perform_attack);
            active_drone->request_active = 1;
            return 1;
        }
        return 0;
    }
    if (randu0(2) != 0) {
        return drone_ai_check_attack(drone, 0, 0) == 1;
    }
    return 0;
}

int drone_ai_check_block_fakeout(void) {
    DroneAI* drone;
    PlyrPdata* player;
    int likelihood;
    int block_hits;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    player = plyr_pdata;
    block_hits = player->block_hit_count;
    likelihood = g_blockFakeOutPercentage[drone->difficulty_index];
    if (block_hits < 4) {
        if (drone->big_boss_stage == 0) {
            likelihood = 20;
        } else if (drone->difficulty_index < 2) {
            likelihood = 5;
        } else {
            likelihood = 0;
        }
    } else {
        if (block_hits > 6) {
            likelihood += 15;
        } else if (block_hits > 8) {
            likelihood += 25;
        }
        if (drone->movement_state == 2) {
            likelihood += 10;
        }
        if (randu0(100) < 50) {
            likelihood += randu0(10);
        }
        if (drone->big_boss_stage == 4) {
            likelihood = 5;
        } else if (drone->big_boss_stage == 3) {
            likelihood -= 10;
        }
        if (likelihood < 0) {
            likelihood = 0;
        }
    }
    return randu0(100) <= (unsigned int)likelihood;
}

void drone_ai_charge_up_watcher(DroneAI* drone) {
    DroneAI* active_drone;

    if (randu0(100) < 20 &&
        (his_pdata->state & 0x1000) == 0 &&
        ai_count_charge_moves() > 0 &&
        drone->opponent_distance > 14.6f) {
        /*
         * Retail consumes this random value even though every possible result
         * passes its comparison against 100.
         */
        randu0(20);
        if (drone->difficulty_index > 2) {
            active_drone = get_player_number(plyr_obj) == 0
                               ? &g_DroneAI1 : &g_DroneAI2;
            xfer_proc(
                get_player_proc(plyr_obj),
                (MkProcEntryFn)drone_ai_perform_charge_up);
            active_drone->request_active = 1;
            return;
        }
    }
    drone_ai_charge_up_watcher_defense(drone);
}

void drone_ai_taunt_watcher_defense(DroneAI* drone) {
    DroneAI* active_drone;

    if (his_pdata->state != 0x420A) {
        return;
    }
    if (drone->difficulty_index <= 2 && randu0(100) >= 50) {
        return;
    }
    if (ai_count_charge_moves() > 0 &&
        randu0(100) < 75 &&
        drone->opponent_distance > 2.8103173f) {
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        xfer_proc(
            get_player_proc(plyr_obj),
            (MkProcEntryFn)drone_ai_perform_charge_up);
        active_drone->request_active = 1;
        return;
    }
    his_pdata->state = 0x4206;
}

void drone_ai_im_dizzy(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (plyr_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return;
    }

    if (drone->fatality_decision == 0) {
        if (get_fatality_available_flag() == 1) {
            if (g_game_number > g_fatality_game_number + 6 &&
                randu0(100) < 30) {
                g_fatality_game_number = g_game_number;
                drone->fatality_decision = 2;
            } else {
                drone->fatality_decision = 1;
            }
        }
        if (is_big_boss(plyr_pdata) || mode_of_play == 10) {
            drone->fatality_decision = 1;
        }
    } else if (drone->fatality_decision == 2) {
        xfer_proc(
            get_player_proc(plyr_obj),
            (MkProcEntryFn)do_my_suicide);
        drone->request_active = 1;
    }
}

void drone_ai_perform_charge_up(void) {
    DroneAI* drone;
    int style;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    style = ai_find_charge_style();
    if (style < 0) {
        AI_TRANSFER(j_exit);
        return;
    }
    drone_ai_force_change_style(drone, style);
    if (plyr_pdata->player_slot != style) {
        AI_TRANSFER(j_exit);
        return;
    }

    drone->jump_attack_pending = 1;
    drone->charge_cooldown_tick = exec_tick_ctr + 240;
    if (drone->movement_state != 1 && drone->movement_state != 5) {
        drone->movement_state = 1;
    }
    drone->attack_type = 8;
    AI_TRANSFER(drone_ai_perform_attack);
}

void drone_ai_perform_taunt(void) {
    DroneAI* drone;
    int style;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    style = ai_find_taunt_style();
    if (style < 0) {
        AI_TRANSFER(j_exit);
        return;
    }
    drone_ai_force_change_style(drone, style);
    if (plyr_pdata->player_slot != style) {
        AI_TRANSFER(j_exit);
        return;
    }

    drone->attack_type = 7;
    AI_TRANSFER(drone_ai_perform_attack);
}

int drone_ai_check_for_berserker_movement(DroneAI* drone) {
    drone->movement_attempt = 0;
    if (drone->opponent_distance > 5.9457946f) {
        if (drone->opponent_distance > 14.6f) {
            drone->charge_cooldown_tick += randu0(25) + 5;
        } else {
            drone->charge_cooldown_tick += randu0(3);
        }
        if (((AiSpecialMoveList*)plyr_pdata->status_flags)->count > 0 &&
            randu0(10) == 0) {
            ai_transfer_active((MkProcEntryFn)catch_opponent);
        } else {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
        }
        return 1;
    }
    if (drone->opponent_distance > 2.8103173f) {
        if (randu0(100) < 20) {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
        } else {
            ai_transfer_active(step_forward_with_jexit);
        }
        return 1;
    }
    if (randu0(100) < 20) {
        ai_transfer_active(side_step_to_center_attack_with_jexit);
        return 1;
    }
    return 0;
}

float drone_ai_avoid_danger_area_now(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    drone->danger_area_request = 0;
    if (ai_backward_clearance() > 1.524f) {
        if (drone->opponent_distance < 14.6f) {
            if (randu0(100) < 35) {
                AI_TRANSFER(side_step_to_center_with_jexit);
            } else if (randu0(100) < 40) {
                AI_TRANSFER(dash_back_with_jexit);
            } else {
                AI_TRANSFER(drone_walk_backwards_with_jexit);
            }
            return 0.0f;
        }
        if (randu0(100) < 15) {
            jump_towards_opponent();
            AI_TRANSFER(j_exit);
        } else {
            AI_TRANSFER(side_step_to_center_with_jexit);
        }
        return 0.0f;
    }
    if (drone->danger_side_step == 1) {
        AI_TRANSFER(side_step_to_center_with_jexit);
    } else {
        jump_towards_opponent();
        AI_TRANSFER(j_exit);
    }
    return 0.0f;
}

void drone_ai_charge_up_watcher_defense(DroneAI* drone) {
    if (his_pdata->state != 0x4209) {
        return;
    }
    if (drone->difficulty_index <= 2 && randu0(100) >= 50) {
        return;
    }

    his_pdata->state = 0x4206;
    if (ai_count_taunt_moves() > 0 &&
        drone->opponent_distance > 13.378037f &&
        drone->reaction_scale >= 0.65f &&
        randu0(100) < 90) {
        ai_transfer_active((MkProcEntryFn)drone_ai_perform_taunt);
        return;
    }
    if (ai_count_charge_moves() > 0 &&
        randu0(100) < 30 &&
        drone->opponent_distance > 5.9457946f) {
        ai_transfer_active((MkProcEntryFn)drone_ai_perform_charge_up);
    }
}

void drone_ai_taunt_watcher(DroneAI* drone) {
    if (randu0(100) < 20 &&
        (his_pdata->state & 0x1000) == 0 &&
        ai_count_taunt_moves() > 0) {
        if (drone->opponent_distance > 26.84898f &&
            is_he_airborn() &&
            drone->difficulty_index < 6) {
            ai_transfer_active((MkProcEntryFn)drone_ai_perform_taunt);
            return;
        }
        if (drone->opponent_distance > 23.783178f &&
            drone->reaction_scale >= 0.65f &&
            drone->difficulty_index < 8 &&
            plyr_pdata != 0) {
            ai_transfer_active((MkProcEntryFn)drone_ai_perform_taunt);
            return;
        }
    }
    drone_ai_taunt_watcher_defense(drone);
}

int drone_ai_check_for_big_boss_aggressive_movement(DroneAI* drone) {
    drone->movement_attempt = 0;
    if (drone->opponent_distance > 14.6f) {
        if (randu0(6) == 0 &&
            drone_ai_check_attack(drone, 1, 0) == 1) {
            return 1;
        }
        if (randu0(8) == 0) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 5.9457946f) {
        if (randu0(4) == 0 &&
            drone_ai_check_attack(drone, 1, 0) == 1) {
            return 1;
        }
        if (randu0(50) == 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
        if (randu0(50) == 0) {
            ai_transfer_active(side_step_to_center_with_jexit);
            return 1;
        }
    } else if (randu0(50) == 0) {
        ai_transfer_active(side_step_to_center_with_jexit);
        return 1;
    }

    if ((his_pdata->state & 0x400) != 0 && randu0(60) == 0) {
        ai_transfer_active(side_step_to_center_with_jexit);
        return 1;
    }
    return 0;
}

int drone_ai_check_for_big_boss_passive_movement(DroneAI* drone) {
    drone->movement_attempt = 0;
    if (drone->opponent_distance < 5.225796f) {
        if (randu0(100) < 90) {
            return 0;
        }
        if (ai_backward_clearance() > 2.1336f) {
            if (randu0(2) != 0) {
                ai_transfer_active(drone_walk_backwards_with_jexit);
                return 1;
            }
        } else {
            ai_transfer_active(side_step_to_center_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 28.451557f) {
        if (randu0(100) < 5) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
    } else {
        if (randu0(100) < 90) {
            return 0;
        }
        if (randu0(100) < 5) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
    }
    return 0;
}

int drone_ai_check_for_aggressive_movement(DroneAI* drone) {
    int retry;

    retry = drone->movement_attempt;
    drone->movement_attempt = 0;
    if (drone->opponent_distance > 28.451557f) {
        drone->charge_cooldown_tick += randu0(25) + 5;
        if ((randu0(25) == 0 && drone->difficulty_index < 6) ||
            randu0(100) == 0) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        }
        if (randu0(30) == 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
        if (randu0(30) == 0) {
            drone->jump_attack_pending = 1;
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
        if (retry == 1) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 5.225796f) {
        if (drone->opponent_distance > 14.6f) {
            drone->charge_cooldown_tick += randu0(25) + 5;
        } else if (drone->opponent_distance > 5.9457946f) {
            drone->charge_cooldown_tick += randu0(3);
        }
        if (randu0(30) == 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
        if (randu0(30) == 0 || retry == 1) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
        if (randu0(60) == 0 &&
            (drone->match_stage != 0 || drone->difficulty_index >= 3)) {
            ai_transfer_active(side_step_to_center_with_jexit);
            return 1;
        }
    } else if (ai_backward_clearance() > 2.1336f &&
               randu0(22) == 0) {
        if (drone->difficulty_index > 5 && randu0(100) < 50) {
            ai_transfer_active((MkProcEntryFn)dash_back_with_jexit);
            drone->jump_attack_pending = 1;
        } else {
            ai_transfer_active(step_backward_with_jexit);
        }
        return 1;
    }
    if ((drone->match_stage != 0 || drone->difficulty_index >= 3) &&
        (his_pdata->state & 0x400) != 0 && randu0(60) == 0) {
        ai_transfer_active(side_step_to_center_with_jexit);
        return 1;
    }
    return 0;
}

int drone_ai_check_for_passive_movement(DroneAI* drone) {
    drone->movement_attempt = 0;
    if (randu0(100) < 60) {
        return 0;
    }
    if (drone->opponent_distance < 5.225796f) {
        if (randu0(16) != 0) {
            return 0;
        }
        if (ai_backward_clearance() <= 2.1336f) {
            ai_transfer_active(side_step_to_center_with_jexit);
            return 1;
        }
        if (randu0(2) != 0) {
            ai_transfer_active(drone_walk_backwards_with_jexit);
            return 1;
        }
        if (randu0(3) == 0) {
            ai_transfer_active(
                randu0(2) == 0
                    ? jump_away_opponent_with_j_exit
                    : (MkProcEntryFn)dash_back_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 28.451557f) {
        ai_transfer_active(
            randu0(4) != 0
                ? jump_towards_opponent_with_jexit
                : step_forward_with_jexit);
        return 1;
    } else {
        if (randu0(14) == 0) {
            return 0;
        }
        if (randu0(15) == 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
        if (ai_backward_clearance() > 2.1336f &&
            randu0(30) == 0) {
            ai_transfer_active(
                randu0(100) < 75
                    ? step_backward_with_jexit
                    : (MkProcEntryFn)dash_back_with_jexit);
            return 1;
        }
    }
    return 0;
}

int drone_ai_check_for_defend_movement(DroneAI* drone) {
    drone->movement_attempt = 0;
    if (drone->opponent_distance < 5.9457946f) {
        if (randu0(4) == 0) {
            return 0;
        }
        if (ai_backward_clearance() <= 2.1336f) {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
            return 1;
        }
        if (randu0(10) == 0) {
            ai_transfer_active(drone_walk_backwards_with_jexit);
            if (randu0(2) == 0) {
                drone->jump_attack_pending = 1;
            }
            return 1;
        }
        if (randu0(25) == 0) {
            ai_transfer_active(jump_away_opponent_with_j_exit);
            return 1;
        }
    } else if (drone->opponent_distance > 28.451557f) {
        if ((randu0(10) != 0 && drone->difficulty_index < 6) ||
            randu0(50) == 0) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
        } else {
            ai_transfer_active(step_forward_with_jexit);
        }
        return 1;
    } else if (drone->opponent_distance > 14.6f) {
        if (randu0(12) != 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
    } else if (randu0(50) == 0) {
        ai_transfer_active(step_forward_with_jexit);
        return 1;
    } else if (randu0(200) == 0 &&
               (drone->match_stage != 0 ||
                drone->difficulty_index >= 3)) {
        ai_transfer_active(side_step_to_center_long_with_jexit);
        return 1;
    } else if (randu0(700) == 0) {
        ai_transfer_active(jump_towards_opponent_with_jexit);
        return 1;
    }
    return 0;
}

int drone_ai_check_for_evade_movement(DroneAI* drone) {
    drone->movement_attempt = 0;
    if (drone->opponent_distance < 5.9457946f) {
        if (randu0(4) == 0) {
            return 0;
        }
        if (ai_backward_clearance() <= 2.1336f) {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
            return 1;
        }
        if (randu0(10) == 0) {
            ai_transfer_active(drone_walk_backwards_with_jexit);
            return 1;
        }
        if (randu0(4) == 0) {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
            return 1;
        }
        if (randu0(10) == 0) {
            ai_transfer_active(jump_away_opponent_with_j_exit);
            return 1;
        }
    } else if (drone->opponent_distance > 28.451557f) {
        drone->charge_cooldown_tick += randu0(25) + 5;
        ai_transfer_active(
            randu0(10) != 0
                ? jump_towards_opponent_with_jexit
                : step_forward_with_jexit);
        return 1;
    } else if (drone->opponent_distance > 14.6f) {
        if (randu0(12) != 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
    } else if (randu0(50) == 0) {
        ai_transfer_active(step_forward_with_jexit);
        return 1;
    } else if (randu0(200) == 0) {
        ai_transfer_active(side_step_to_center_long_with_jexit);
        return 1;
    } else if (randu0(700) == 0) {
        ai_transfer_active(jump_towards_opponent_with_jexit);
        return 1;
    } else if (randu0(500) == 0 &&
               ai_backward_clearance() > 2.1336f) {
        ai_transfer_active(jump_away_opponent_with_j_exit);
        return 1;
    }
    return 0;
}

int drone_ai_change_attack_to_low(DroneAI* drone) {
    unsigned int stage;
    unsigned short roll;

    roll = randu0(100);
    if (drone->movement_state == 8 &&
        (drone->player->state & 0x100) != 0 &&
        randu0(100) < 80) {
        return 1;
    }

    stage = drone->big_boss_stage;
    if (stage < 2 ||
        (stage == 2 && drone->match_stage < 2) ||
        drone->opponent_distance > 2.0f) {
        return 0;
    }
    if ((his_pdata->state & 0x800) != 0) {
        if ((stage == 2 && roll < 15) ||
            (stage == 3 && roll < 25) ||
            (stage == 4 && roll < 35)) {
            return 1;
        }
    }
    if ((stage == 2 && roll < 5) ||
        (stage == 3 && roll < 10) ||
        (stage == 4 && roll < 15)) {
        return 1;
    }
    return 0;
}

int drone_ai_check_all_over_ground(DroneAI* drone) {
    unsigned short roll;

    drone->special_reaction_active = 1;
    drone->attack_pending = 1;
    if (drone->opponent_distance < 2.8103173f) {
        if (randu0(100) < 50) {
            ai_transfer_active((MkProcEntryFn)drone_ai_counter_attack_now);
        } else {
            ai_transfer_active(jump_away_opponent_with_j_exit);
        }
        return 1;
    }
    if (drone->opponent_distance < 5.9457946f) {
        if (randu0(100) < 50) {
            ai_transfer_active(
                (MkProcEntryFn)jump_towards_opponent_with_j_exit);
        } else {
            ai_transfer_active(jump_away_opponent_with_j_exit);
        }
        return 1;
    }

    roll = randu0(100);
    if (roll < 50 ||
        (roll < 75 && drone->difficulty_index < 3)) {
        ai_transfer_active(jump_away_opponent_with_j_exit);
    } else {
        ai_transfer_active(
            (MkProcEntryFn)jump_towards_opponent_with_j_exit);
    }
    return 1;
}

int drone_ai_check_avoid_danger_area(DroneAI* drone) {
    drone->special_reaction_active = 1;
    drone->danger_area_active = 0;
    drone->danger_area_counter = 0;
    if (randu0(100) < 5 || drone->difficulty_index < 2) {
        return 0;
    }

    drone->danger_area_active = 1;
    if (drone->opponent_distance < 2.8103173f) {
        ai_transfer_active((MkProcEntryFn)drone_ai_counter_attack_now);
        return 1;
    }
    if (drone->opponent_distance < 14.6f) {
        drone->reversal_pending = 1;
        ai_transfer_active(side_step_to_center_with_jexit);
        return 1;
    }
    if (ai_count_taunt_moves() > 0 && randu0(100) < 75) {
        ai_transfer_active((MkProcEntryFn)drone_ai_perform_taunt);
    } else {
        ai_transfer_active(side_step_to_center_long_with_jexit);
    }
    return 1;
}

int drone_ai_check_for_aggressive_throw(DroneAI* drone) {
    AiExtendedSpecialMoveList* moves;
    void* script;

    if (drone->opponent_distance > 4.2958374f ||
        randu0(100) < 70 ||
        drone->big_boss_stage == 0 ||
        !drone_ai_check_throw_restrictions()) {
        return 0;
    }
    if (randu0(100) < 80) {
        if (drone->block_hold_ticks <=
            g_minBlockHeldTime[drone->difficulty_index]) {
            return 0;
        }
    } else if (randu0(100) >=
               g_likelihoodToAggressiveThrow[drone->difficulty_index]) {
        return 0;
    }

    script = get_random_fightstyle_attack(
        plyr_pdata->fighter_definition, 11, 0);
    if (script == 0) {
        moves = (AiExtendedSpecialMoveList*)plyr_pdata->status_flags;
        script = ai_pick_special_move(
            drone, moves->throw_commands, moves->throw_count);
        if (script == 0) {
            return 0;
        }
        drone->script_attack_ready = 1;
    }
    drone->script_attack = script;
    ai_transfer_active(
        (MkProcEntryFn)drone_ai_perform_script_attack);
    return 1;
}

void drone_ai_victim_avoid(void) {
    DroneAI* drone;
    Vec target;
    Vec enemy_direction;
    float target_distance;
    float enemy_distance;
    float cross;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (drone->avoidance_area_duration > 0.0f &&
        his_obj != 0 && plyr_obj != 0) {
        target.x = drone->avoidance_position[0];
        target.y = plyr_obj->pos.value.y;
        target.z = drone->avoidance_position[2];
        target_distance = dist_v3_to_v3(&target, &plyr_obj->pos.value);
        enemy_distance = dist_v3_to_v3(&his_obj->pos.value, &plyr_obj->pos.value);
        if (target_distance == 0.0f || enemy_distance == 0.0f) {
            return;
        }
        if (enemy_distance >= target_distance - 1.0f) {
            uv_v3_to_v3_dist(
                &enemy_direction, &plyr_obj->pos.value, &his_obj->pos.value);
            cross =
                (target.x - plyr_obj->pos.value.x) * enemy_direction.z -
                (target.z - plyr_obj->pos.value.z) * enemy_direction.x;
            if (cross < 0.0f) {
                cross = -cross;
            }
            if (cross < 1.25f) {
                ai_transfer_active(side_step_to_center_with_jexit);
                return;
            }
        }
    }
    drone->reaction_watcher = 0;
}

void drone_ai_victim_ducking(void) {
    DroneAI* drone;
    void* script;
    int is_script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if ((his_pdata->state & 0x100) == 0) {
        drone->duck_reaction_tick = 0;
        drone->reaction_watcher = 0;
        return;
    }
    if (drone->opponent_distance < 7.3f &&
        randu0(100) < 25 &&
        dist_behind_me() > 1.0f) {
        ai_transfer_active(drone_walk_backwards_further_with_jexit);
        return;
    }
    if (drone->opponent_distance > 14.6f &&
        ai_count_taunt_moves() > 0 &&
        randu0(100) < 15) {
        ai_transfer_active((MkProcEntryFn)drone_ai_perform_taunt);
        return;
    }
    if (drone->opponent_distance < 5.9457946f) {
        script = drone_ai_choose_move_from_category(6, 75, &is_script);
        if (script != 0) {
            drone->script_attack = script;
            drone->script_attack_ready = is_script ? 1 : 2;
            ai_transfer_active(
                (MkProcEntryFn)drone_ai_perform_script_attack);
            return;
        }
    }
    drone->reaction_watcher = 0;
}

void drone_ai_ducker(void) {
    DroneAI* drone;
    AiFightstyleAttackTable* attacks;
    AiFightstyleAttack* script;
    PlyrPdata* opponent;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    opponent = drone->player->his_plyr_pdata;
    if (drone->opponent_distance < 2.8103173f &&
        (opponent->state & 0x1000) != 0 &&
        opponent->block_requirement == 0 &&
        randu0(100) < 70 &&
        ((drone->player->state & 0x100) != 0 || randu0(100) < 10)) {
        script = (AiFightstyleAttack*)get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 3, 0);
        attacks = (AiFightstyleAttackTable*)
            plyr_pdata->fighter_definition->move_blend_data;
        if (script != 0 && script->opcode == 0 &&
            (script == &attacks->attacks[2] ||
             script == &attacks->attacks[12] ||
             script == &attacks->attacks[17])) {
            drone->script_attack = script;
            drone->script_attack_ready = 2;
            ai_transfer_active(
                (MkProcEntryFn)drone_ai_perform_script_attack);
            return;
        }
    }
    if (drone_ai_check_change_style(drone) == 1 ||
        drone_ai_check_for_ducker_movement(drone) == 1) {
        return;
    }
    if (randu0(100) < 20 &&
        drone_ai_check_attack(drone, 0, 0) == 1) {
        return;
    }
    if (randu0(1000) < 65) {
        drone_ai_check_for_throw(drone);
    }
}

void drone_ai_increase_big_boss_stage(PlyrPdata* victim) {
    DroneAI* drone;
    PlyrPdata* opponent;
    MkProc* opponent_proc;
    CmdScript* script;

    drone = &g_DroneAI1;
    if (g_game_info.plyr1.slot.pdata->character_id == 0x1D) {
        drone = &g_DroneAI2;
    }
    drone->big_boss_stage_hits++;
    if (drone->big_boss_stage_hits > 5) {
        drone->damage_transition_tick = 600000;
    } else {
        drone->damage_transition_tick = game_tick_ctr + 240;
    }

    if (victim == g_game_info.plyr1.slot.pdata) {
        opponent = g_game_info.plyr0.slot.pdata;
        opponent_proc = (MkProc*)g_game_info.plyr0.idle_proc;
    } else if (victim == g_game_info.plyr0.slot.pdata) {
        opponent = g_game_info.plyr1.slot.pdata;
        opponent_proc = (MkProc*)g_game_info.plyr1.idle_proc;
    } else {
        return;
    }
    script = get_cmdscript_for_proc(opponent_proc);
    opponent->blocking_disabled = 1;
    opponent->blocking_disabled_2 = 1;
    snd_req(0xCF);
    run_reaction_cleanup_function(victim->his_plyr_pdata);
    if (script != 0) {
        script->unk28 = 0x6F;
    }
    xfer_player_proc(opponent_proc, r_call_script_function);
}

int random_block_hit(int attacker_style, int defender_style, int heavy) {
    int sound_id;

    if (attacker_style < 0 || attacker_style >= 6 ||
        defender_style < 0 || defender_style >= 6) {
        return 0;
    }
    sound_id = -1;
    if (attacker_style <= 2 && defender_style <= 2) {
        if (heavy == 1 || attacker_style == 1 || defender_style == 1) {
            sound_id = randu0(100) < 50 ? 0xD95 : 0xD9A;
        } else {
            sound_id = 0xD94;
        }
        fight_fx_blades_clash(plyr_pdata);
        if (sound_id != -1) {
            pan_vol_pitch_snd_req(
                sound_id, 0.0f, 1.0f, 0.95f + frand(0.11f));
        }
        return sound_id != -1;
    }
    if ((attacker_style == 3 && defender_style <= 1) ||
        (defender_style == 3 && attacker_style <= 1)) {
        sound_id = heavy == 1
                       ? (randu0(100) < 50 ? 0xD95 : 0xD9A)
                       : 0xD96;
    }
    if (attacker_style == 3 && defender_style == 3) {
        sound_id = heavy == 1 ? 0xD99 : 0xD98;
    }
    if (sound_id != -1) {
        pan_vol_pitch_snd_req(
            sound_id, 0.0f, 1.0f, 0.85f + frand(0.21f));
    }
    return sound_id != -1;
}

void drone_ai_victim_dizzy_3(void) {
    DroneAI* drone;
    AiSpecialMoveList* moves;
    void* script;
    unsigned short roll;
    int is_script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    script = drone_ai_choose_move_from_category(11, 75, &is_script);
    roll = randu0(100);
    if (his_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return;
    }
    if (roll < 40) {
        moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
        if (moves->ranged_count > 0) {
            script = ai_pick_special_move(
                drone, moves->ranged_commands, moves->ranged_count);
            drone->script_attack_ready = 1;
        }
    } else if (script != 0 && roll < 80) {
        drone->script_attack_ready = is_script ? 1 : 2;
    } else {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 10, 3);
        if (script == 0) {
            return;
        }
    }
    if (script != 0) {
        drone->script_attack = script;
        ai_transfer_active(
            (MkProcEntryFn)drone_ai_perform_script_attack);
    }
}

float drone_entry(void) {
    DroneAI* drone;
    PlyrPdata* action;
    int previous_state;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    previous_state = plyr_pdata->state;
    init_ground_move();
    back_to_normal();
    rotate_towards_him(0.2f);
    end_of_round_check();
    drone->force_attack = 0;
    drone->command_active = 0;
    drone->super_combo_active = 0;
    if (drone->attack_latched == 1) {
        drone->attack_pending = 0;
    }
    if (drone->jump_attack_pending == 1) {
        drone->jump_attack_pending = 0;
        if (drone->difficulty_index > 0) {
            drone_ai_check_attack(drone, 1, 1);
        }
    } else if (drone->reversal_pending == 1) {
        drone->reversal_pending = 0;
        if (drone->opponent_distance < 5.9457946f) {
            AI_TRANSFER(drone_ai_perform_combo_attack);
            return 0.0f;
        }
    }
    if (drone->taunt_pending == 1) {
        drone->taunt_pending = 0;
        drone_ai_perform_taunt();
    }
    if (drone->movement_state == 8 &&
        (previous_state & 0x100) != 0) {
        set_my_state(0x100);
        drone->player->field_728 = 0;
        drone->duck_started_tick =
            game_tick_ctr - 20 - randu0(40);
        AI_TRANSFER(joy_duck_loop);
        return 0.0f;
    }

    drone->decision_ready = 1;
    drone->request_active = 0;
    action = plyr_pdata;
    if (action->action_lock_a <= game_tick_ctr &&
        action->action_lock_b <= game_tick_ctr &&
        action->push_blocked == 0 &&
        (action->state & 0x200) == 0 &&
        (g_game_info.flags & 0x20) != 0 &&
        (g_game_info.pause_flags & 2) == 0) {
        AI_TRANSFER(drone_loop);
    }
    return 0.0f;
}

void drone_blocking_done(void) {
    DroneAI* drone;
    PlyrPdata* player;
    int likelihood;
    int counter;
    int combo_depth;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    player = plyr_pdata;
    plyr_pdata->previous_state = plyr_pdata->state;
    plyr_pdata->state = 0;
    blend_to_stance(0.1f);
    drone->attack_pending = 0;
    counter = 0;
    if ((g_DroneOverrideInfo.flags & 0x20) == 0) {
        likelihood = g_likelihoodOfBlockCounter[drone->difficulty_index];
        if (drone->attack_latched == 1) {
            likelihood += 10;
        }
        likelihood += g_blockCounterAdjustor[drone->movement_state];
        if (drone->difficulty_index > 1) {
            if (randu0(100) < 50) {
                likelihood += randu0(15);
            } else {
                likelihood -= randu0(5);
            }
        }
        if (is_big_boss(drone->player)) {
            likelihood -= 20;
        }
        if (likelihood < 0) {
            likelihood = 0;
        }
        counter = randu0(100) < (unsigned int)likelihood;
    }
    if (counter) {
        combo_depth = player->combo_depth;
        if (drone->difficulty_index < 2) {
            drone->jump_attack_pending = 1;
        } else if (drone->difficulty_index < 4) {
            if ((randu0(100) < 40 && combo_depth > 4) ||
                (randu0(100) < 15 && combo_depth > 1)) {
                drone->reversal_pending = 1;
            } else {
                drone->jump_attack_pending = 1;
            }
        } else if (drone->difficulty_index < 6) {
            if ((randu0(100) < 70 && combo_depth > 3) ||
                (randu0(100) < 35 && combo_depth > 1)) {
                drone->reversal_pending = 1;
            } else {
                drone->jump_attack_pending = 1;
            }
        } else if (combo_depth > 5 ||
                   (combo_depth > 3 && randu0(100) < 90) ||
                   (combo_depth > 1 && randu0(100) < 55)) {
            drone->reversal_pending = 1;
        } else {
            drone->jump_attack_pending = 1;
        }
    }
    AI_TRANSFER(drone_entry);
}

void whoosh_fx(int hit_type) {
    AiFighterSoundView* fighter;
    unsigned int move_flags;
    int region;

    region = plyr_pdata->attack_region;
    if (region == 0x10 && plyr_pdata->weapon_impact != 0) {
        region = plyr_pdata->weapon_impact->attack_region;
    }
    fighter = (AiFighterSoundView*)plyr_pdata->fighter_definition;
    move_flags = 0;
    if (fighter->move_data != 0 &&
        fighter->move_data->flags != 0) {
        move_flags = fighter->move_data->flags->weapon_flags;
    }
    if (plyr_pdata->character_id == 0x10 &&
        !plyr_pdata->plyr_info->flags_14_bits.alternate_costume &&
        ((region >= 0 && region < 3) ||
         (region >= 6 && region < 8) ||
         (region >= 10 && region < 13))) {
        snd_req(0x280);
    }
    if (hit_type == 0) {
        return;
    }
    if (region == 0x10 &&
        is_weapon_style(plyr_pdata->fighter_definition)) {
        region = 0;
    }
    if (region == 0) {
        if ((move_flags & 8) != 0 || (move_flags & 0x100) != 0) {
            random_hit(0x12);
        } else if ((move_flags & 2) != 0) {
            random_hit(0x11);
        } else {
            random_hit(0x10);
        }
    } else if (hit_type == 8) {
        random_hit(8);
    } else {
        random_hit(7);
    }
}

static void ai_finish_duck_reaction(DroneAI* drone, int throw_attack) {
    AiMoveBlendScripts* scripts;
    void* script;
    unsigned int exposed_ticks;
    int is_script;

    plyr_pdata->his_attack_counter = get_his_attack_counter();
    xfer_proc(plyr_anim_proc, p_animate);
    if (is_big_boss(drone->player)) {
        AI_TRANSFER(x_block);
        return;
    }
    init_ground_move();
    set_my_state(0x100);
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(
        plyr_pdata->fighter_definition->duck_animation, 0, 0.1f);
    set_my_state(0x101);
    AI_SLEEP(1.0f);
    plyr_pdata->duck_wait_ticks = 10;
    while (plyr_pdata->duck_wait_ticks > 0) {
        AI_SLEEP(1.0f);
        plyr_pdata->duck_wait_ticks--;
    }

    exposed_ticks = 0;
    while ((his_pdata->state & 0x1000) != 0 ||
           his_pdata->throw_restriction == 3 ||
           his_pdata->duck_reaction_active == 1) {
        exposed_ticks++;
        if (exposed_ticks >= 25) {
            break;
        }
        AI_SLEEP(1.0f);
        if (his_pdata->secondary_state != 0x101 &&
            plyr_pdata->his_attack_counter != get_his_attack_counter()) {
            if (his_pdata->block_requirement != 0 &&
                his_pdata->block_requirement != 6) {
                AI_TRANSFER(x_block);
                return;
            }
            plyr_pdata->his_attack_counter = get_his_attack_counter();
            exposed_ticks = 0;
        }
    }
    AI_SLEEP(2.0f + (float)randu0(10));
    drone->attack_pending = 0;
    if (throw_attack) {
        script =
            drone_ai_choose_move_from_category(11, 75, &is_script);
        if (script != 0) {
            drone->script_attack = script;
            drone->script_attack_ready = is_script ? 1 : 2;
            AI_TRANSFER(drone_ai_perform_script_attack);
            return;
        }
    } else if (randu0(100) < 10) {
        scripts = (AiMoveBlendScripts*)
            plyr_pdata->fighter_definition->move_blend_data;
        drone->script_attack = &scripts->fatality_scripts[14];
        AI_TRANSFER(drone_ai_scripted_attack);
        return;
    }
    if (!throw_attack) {
        drone_ai_check_attack(drone, 1, 1);
    }
    set_my_state(0);
    AI_TRANSFER(j_exit_blend_stance);
}

void drone_ai_duck_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    ai_finish_duck_reaction(drone, 0);
}

void drone_ai_duck_throw_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    ai_finish_duck_reaction(drone, 1);
}

void go_into_major_pain(void) {
    PlyrPdata* death;
    AiAnimPdataView* anim_view;
    AiStatusSoundView* status;

    death = plyr_pdata;
    anim_view = (AiAnimPdataView*)plyr_anim_pdata;
    status = (AiStatusSoundView*)plyr_pdata->status_flags;
    back_to_normal();
    plyr_obj->flags_09_bits.head_tracking = 0;
    tightrope_restrictions_off(plyr_obj, 0);
    if (death->death_type == 4) {
        init_ground_move_no_aniproc();
        if (randu0(100) >= 75 && (g_game_info.field_04 & 0x80) == 0) {
            plyr_anim_pdata->step = 1.0f;
            anim_view->twitch_weight = 0.5f;
            plyr_anim_pdata->flags |= 0x40;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.twitch_death,
                0, 0.05f);
            ani_to_frame_x(2.0f);
        } else {
            plyr_anim_pdata->step = 0.6f;
            anim_view->twitch_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.major_pain_a,
                0, 0.05f);
            AI_SLEEP(1.0f);
            ani_to_frame_x(7.0f);
        }
        plyr_obj->flags_09_bits.launched = 0;
        if (status->pain_voice != 0) {
            random_snd_req(status->pain_voice);
        }
        plyr_anim_pdata->step = 0.6f;
        xfer_proc(plyr_anim_proc, p_animate);
    } else if (death->death_type == 1) {
        init_ground_move_no_aniproc();
        if (!is_weapon_style(plyr_pdata->fighter_definition)) {
            if (!is_big_boss(his_pdata)) {
                xfer_player_proc(
                    get_player_proc(his_obj),
                    force_some_distance);
            }
            plyr_obj->flags_09_bits.head_tracking = 0;
            tightrope_restrictions_off(plyr_obj, 0);
            plyr_anim_pdata->step = 0.6f;
            if (status->pain_voice != 0) {
                random_snd_req(status->pain_voice);
            }
            anim_view->twitch_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.major_pain_b,
                0, 0.05f);
            AI_SLEEP(1.0f);
            ani_to_frame_x(2.0f);
            init_air_move();
            plyr_anim_pdata->step = 0.6f;
            xfer_proc(plyr_anim_proc, p_animate);
        }
    }
    AI_TRANSFER(j_stay_down_dead);
}

int drone_ai_check_for_ducker_movement(DroneAI* drone) {
    AiSpecialMoveList* moves;
    void* script;
    int is_script;

    drone->movement_attempt = 0;
    if (drone->opponent_distance > 16.218693f &&
        randu0(100) < 33) {
        script = drone_ai_choose_move_from_category(2, 50, &is_script);
        if (script == 0) {
            return 0;
        }
        drone->script_attack = script;
        drone->script_attack_ready = is_script ? 1 : 2;
        ai_transfer_active(
            (MkProcEntryFn)drone_ai_perform_script_attack);
        return 1;
    }
    if (drone->opponent_distance > 5.9457946f) {
        if (drone->opponent_distance > 14.6f) {
            drone->charge_cooldown_tick += randu0(25) + 5;
        } else {
            drone->charge_cooldown_tick += randu0(3);
        }
        moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
        if (moves->count > 0 && randu0(10) == 0) {
            ai_transfer_active((MkProcEntryFn)catch_opponent);
        } else {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
        }
        return 1;
    }
    if (drone->opponent_distance > 5.729021f) {
        if (randu0(100) < 20) {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
        } else {
            ai_transfer_active(step_forward_with_jexit);
        }
        return 1;
    }
    if ((drone->player->state & 0x100) != 0) {
        if (game_tick_ctr - drone->duck_started_tick > 120 ||
            (randu0(1000) < 2 &&
             game_tick_ctr - drone->duck_started_tick > 20)) {
            if ((drone->player->his_plyr_pdata->state & 0x1000) == 0) {
                drone->player->field_728 = 1;
            }
        }
    } else if (randu0(1000) < 20) {
        drone->duck_started_tick = game_tick_ctr;
        drone->player->field_728 = 0;
        ai_transfer_active(joy_duck_loop);
        return 1;
    }
    return 0;
}

int drone_ai_enemy_inair_attack(DroneAI* drone) {
    AiAirMoveStatus* moves;
    void* script;

    drone->script_attack_ready = 0;
    if (drone->opponent_distance > 14.6f) {
        return 0;
    }
    moves = (AiAirMoveStatus*)plyr_pdata->status_flags;
    script = 0;
    if (drone->opponent_distance > 5.9457946f) {
        if (moves->distant_count > 0) {
            script = ai_pick_special_move(
                drone, moves->distant_commands, moves->distant_count);
            drone->script_attack_ready = 1;
        }
    } else if (drone->opponent_distance > 2.8103173f) {
        if (moves->distant_count != 0 && randu0(2) != 0) {
            script = ai_pick_special_move(
                drone, moves->distant_commands, moves->distant_count);
            drone->script_attack_ready = 1;
        } else {
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, 12, 0);
        }
    } else if (moves->close_count != 0 && randu0(2) != 0) {
        script = ai_pick_special_move(
            drone, moves->close_commands, moves->close_count);
        drone->script_attack_ready = 1;
    } else {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 12, 1);
        if (script == 0) {
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, 10, 1);
        }
    }
    if (script == 0) {
        return 0;
    }
    drone->script_attack = script;
    ai_transfer_active(
        (MkProcEntryFn)drone_ai_perform_script_attack);
    return 1;
}

void drone_ai_beating_the_snot_out_of_him_watcher(void) {
    DroneAI* drone;
    PlyrPdata* opponent;
    PlyrPdata* player;
    unsigned int likelihood;
    int can_act;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    opponent = his_pdata;
    if (opponent->hit_streak < 3 ||
        drone->super_combo_active == 1 ||
        drone->command_active == 1) {
        return;
    }
    player = plyr_pdata;
    can_act =
        player->action_lock_a <= game_tick_ctr &&
        player->action_lock_b <= game_tick_ctr &&
        player->push_blocked == 0 &&
        (player->state & 0x200) == 0;
    if (!can_act && am_i_airborn()) {
        return;
    }
    if (randu0(100) < 75 && drone->difficulty_index < 5) {
        opponent->hit_streak = 0;
        drone->movement_state = 0;
        drone->charge_cooldown_tick = exec_tick_ctr + 120;
    }
    if (drone->difficulty_index == 8 ||
        drone->opponent_distance > 9.290304f ||
        ai_backward_clearance() <= 2.1336f) {
        return;
    }
    likelihood = (opponent->hit_streak - 3) * 10 + 30;
    if (drone->big_boss_stage == 4) {
        likelihood -= 20;
    }
    if (randu0(100) < likelihood) {
        ai_transfer_active((MkProcEntryFn)dash_back_with_jexit);
        opponent->hit_streak = 0;
    } else if (randu0(100) < 75) {
        opponent->hit_streak = 0;
    }
}

void drone_ai_attacker_not_facing_watcher(void) {
    DroneAI* drone;
    PlyrPdata* player;
    int likelihood;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    player = plyr_pdata;
    if (player->action_lock_a > game_tick_ctr ||
        player->action_lock_b > game_tick_ctr ||
        player->push_blocked != 0 ||
        (player->state & 0x200) != 0 ||
        am_i_airborn() == 1 ||
        !HeIsNotFacing() ||
        (g_DroneOverrideInfo.flags & 0x20) != 0) {
        return;
    }
    likelihood =
        g_likelihoodOfNotFacingAttacking[drone->difficulty_index];
    if (drone->movement_state == 6) {
        likelihood += 10;
    } else if (drone->movement_state == 2) {
        likelihood -= 10;
    }
    if (randu0(100) < 50) {
        likelihood += randu0(20);
    }
    if (likelihood < 0) {
        likelihood = 0;
    }
    if (randu0(100) >= (unsigned int)likelihood) {
        if (drone_ai_can_push(drone) &&
            drone->opponent_distance < 4.378056f &&
            randu0(500) < 5 &&
            drone->reaction_scale > 0.05f) {
            ai_transfer_active(drone_ai_perform_push);
        }
        return;
    }
    if (drone_ai_can_push(drone) &&
        drone->opponent_distance < 4.378056f &&
        randu0(100) < 3 &&
        drone->reaction_scale > 0.05f) {
        ai_transfer_active(drone_ai_perform_push);
        return;
    }
    if ((plyr_pdata->his_plyr_pdata->state & 0x100) == 0) {
        drone_ai_attacker_defenseless(drone);
    }
}

unsigned int handicap_calc_likelihood_of_blocking(DroneAI* drone) {
    int likelihood;
    unsigned int stage;

    likelihood = g_likelihoodOfBlocking[drone->difficulty_index];
    if (drone->movement_state == 1) {
        likelihood -= 5;
    } else if (drone->movement_state == 2) {
        likelihood += 20;
        if (drone->difficulty_index < 2) {
            likelihood -= 15;
        }
    } else if (drone->movement_state == 3) {
        likelihood += 10;
    }
    if (is_big_boss(plyr_pdata)) {
        if (drone->big_boss_block_state == 0) {
            likelihood +=
                g_bigBossBlockAdjuster[drone->difficulty_index] - 5;
        } else {
            likelihood -= drone->big_boss_stage == 4 ? 5 : 12;
        }
        if (likelihood < 0) {
            likelihood = 0;
        }
    }
    if (randu0(100) < 50) {
        likelihood += randu0(5);
    } else {
        likelihood -= randu0(5);
    }
    if (likelihood < 0) {
        likelihood = 0;
    }
    if (drone->match_stage == 0 && drone->difficulty_index < 3) {
        likelihood = 0;
    }
    stage = drone->big_boss_stage;
    if (stage == 3) {
        likelihood += 2;
    } else if (stage == 4) {
        likelihood += 8;
    }
    if (get_game_state() == 3) {
        likelihood -= 6;
    }
    if (his_pdata->state == 0x1219) {
        if (stage == 0) {
            likelihood++;
        } else if (drone->match_stage < 3) {
            likelihood +=
                (stage <= 2 ? 2 : 5) * (drone->match_stage + 1);
        } else if (drone->difficulty_index < 4) {
            likelihood += 25;
        } else {
            likelihood = 90;
        }
    }
    if (is_big_boss(drone->player) &&
        drone->player->plyr_info->field_0C < 0.35f) {
        likelihood += 3;
    }
    if ((drone->player->state & 0x100) != 0 &&
        drone->movement_state == 8 &&
        drone->player->his_plyr_pdata->block_requirement == 1) {
        likelihood += 3;
    }
    if (drone->player->his_plyr_pdata->plyr_info
            ->slot.mirror_a->hide_flag_bits.hidden) {
        if (stage == 4) {
            likelihood -= 8;
        } else {
            likelihood /= 2;
        }
    }
    if (likelihood < 0) {
        likelihood = 0;
    }
    return (unsigned int)(
        (float)likelihood * g_DroneOverrideInfo.likelihood_scale);
}

int drone_ai_should_be_blocking(int reaction) {
    DroneAI* drone;
    PlyrPdata* player;
    unsigned int likelihood;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (plyr_pdata->blocking_disable_tick_1 > game_tick_ctr ||
        plyr_pdata->blocking_disable_tick_2 > game_tick_ctr ||
        plyr_pdata->blocking_disabled == 1 ||
        am_i_airborn() ||
        (plyr_pdata->state & 0x200) != 0) {
        return 0;
    }
    if ((his_pdata->state & 0x1000) == 0 &&
        (his_pdata->throw_restriction != 3 ||
         (his_pdata->secondary_state & 0x100) == 0)) {
        return 0;
    }
    player = plyr_pdata;
    player->opponent_attack_counter = his_pdata->attack_counter;
    player->opponent_attack_counter_copy = his_pdata->attack_counter;
    if (drone->big_boss_stage < 4 &&
        drone->block_retry_tick > game_tick_ctr) {
        return 0;
    }
    if (drone->opponent_distance >= 9.0f &&
        (his_pdata->secondary_state & 0x100) == 0) {
        return 0;
    }
    if (g_DroneOverrideInfo.likelihood_scale == 0.0f) {
        return 0;
    }
    if (reaction == 1) {
        return 1;
    }
    if (his_pdata->repeated_action_count >
        (int)(randu0(3) + 2)) {
        if (drone->big_boss_stage == 0 &&
            drone->match_stage > 5 &&
            randu0(100) < 50) {
            return 0;
        }
        if (drone->match_stage < 3 &&
            drone->big_boss_stage <= 2 &&
            randu0(100) < 30) {
            return 0;
        }
        if (drone->difficulty_index > 1 &&
            randu0(100) < 90) {
            return 0;
        }
        if (randu0(100) < 50) {
            return 0;
        }
    }
    likelihood = handicap_calc_likelihood_of_blocking(drone);
    if (randu0(100) >= likelihood) {
        return 0;
    }
    if ((unsigned int)drone->hit_active >=
        (unsigned int)(
            g_maximumNumBlocksInARow[drone->difficulty_index] +
            randu0(2))) {
        return 0;
    }
    drone->hit_active++;
    return 1;
}

static int ai_weapon_material(PlyrFighterDefinition* fighter) {
    AiFighterSoundView* view;
    unsigned int flags;

    if (!is_weapon_style(fighter)) {
        return 5;
    }
    view = (AiFighterSoundView*)fighter;
    flags = 0;
    if (view->move_data != 0 && view->move_data->flags != 0) {
        flags = view->move_data->flags->weapon_flags;
    }
    if ((flags & 0x40) != 0) {
        return (flags & 2) != 0 ? 1 : 0;
    }
    if ((flags & 8) != 0) {
        return 2;
    }
    if ((flags & 0x20) != 0) {
        return 3;
    }
    if ((flags & 0x10) != 0) {
        return 4;
    }
    return 5;
}

void blocked_fx(int reaction, int camera_strength, int rumble_flags) {
    AiFighterSoundView* player_fighter;
    unsigned int player_flags;
    unsigned int blood_bone;
    int bleeding_weapon;
    int opponent_material;
    int player_material;
    int region;

    region = his_pdata->attack_region;
    if (region == 0x10 && his_pdata->weapon_impact != 0) {
        region = his_pdata->weapon_impact->attack_region;
    }
    opponent_material = region == 0
                            ? ai_weapon_material(
                                  his_pdata->fighter_definition)
                            : 5;
    bleeding_weapon = 0;
    if (region == 0) {
        player_fighter =
            (AiFighterSoundView*)his_pdata->fighter_definition;
        if (player_fighter->move_data != 0 &&
            player_fighter->move_data->flags != 0 &&
            (player_fighter->move_data->flags->weapon_flags & 4) != 0) {
            bleeding_weapon = 1;
        }
    }
    player_material = ai_weapon_material(plyr_pdata->fighter_definition);
    player_fighter =
        (AiFighterSoundView*)plyr_pdata->fighter_definition;
    player_flags = 0;
    if (player_fighter->move_data != 0 &&
        player_fighter->move_data->flags != 0) {
        player_flags = player_fighter->move_data->flags->weapon_flags;
    }
    if ((player_flags & 0x80) == 0 &&
        (player_material == 0 || player_material == 1)) {
        player_material = 3;
    }
    if (opponent_material == 5 && player_material == 5) {
        if (reaction != 5) {
            random_hit(6);
        }
    } else if (!random_block_hit(
                   opponent_material, player_material,
                   reaction == 11)) {
        random_hit(6);
    }
    if (bleeding_weapon && player_material == 5) {
        blood_bone = 0x18;
        if (plyr_pdata->previous_state == 0xA01 ||
            plyr_pdata->previous_state == 0xA03) {
            blood_bone = 0x19;
        } else if (plyr_pdata->previous_state == 0xA02) {
            blood_bone = 0x14;
        }
        if (!am_i_flipped()) {
            if (blood_bone == 0x19) {
                blood_bone = 0x18;
            } else if (blood_bone == 0x18) {
                blood_bone = 0x19;
            }
        }
        plyr_bleed_large_ext(plyr_pdata, blood_bone, plyr_pdata);
        start_blood_particles(
            0x18, blood_bone, plyr_pdata, plyr_obj);
    }
    switch (camera_strength) {
    case 1:
        shake_camera(2, 0.02f);
        break;
    case 2:
        shake_camera(3, 0.03f);
        break;
    case 3:
        shake_camera(2, 0.03f);
        break;
    case 4:
        shake_camera(3, 0.02f);
        break;
    case 5:
        shake_camera(1, 0.01f);
        break;
    case 6:
        shake_camera(2, 0.01f);
        break;
    }
    execute_rumble(camera_strength, rumble_flags);
}
