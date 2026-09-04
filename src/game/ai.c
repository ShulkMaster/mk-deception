#include "game/ai.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/anim_pdata.h"
#include "runtime/cam.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_info.h"
#include "runtime/plyr_pdata.h"
#include "game/constrain.h"
#include "game/collision.h"
#include "game/ejb.h"
#include "game/game_info.h"
#include "game/plyr.h"
#include "game/settings.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "platform/io.h"
#include "runtime/image.h"

typedef struct AiFightstyleAttack {
    int opcode;
    union {
        int argument;
        float (*entry)(void);
    };
} AiFightstyleAttack;

typedef struct DroneAI {
    int movement_state; /* +0x00 */
    unsigned int match_mode; /* +0x04 */
    unsigned int handicap_match_stage; /* +0x08 */
    float reaction_scale; /* +0x0C */
    unsigned int reaction_ticks; /* +0x10 */
    PlyrPdata* player; /* +0x14 */
    union {
        unsigned int match_stage;
        unsigned int handicap_stage;
    }; /* +0x18 */
    union {
        unsigned int big_boss_stage;
        unsigned int handicap_setting;
    }; /* +0x1C */
    unsigned int difficulty_update_tick; /* +0x20 */
    unsigned int charge_cooldown_tick; /* +0x24 */
    unsigned int next_style_change_tick; /* +0x28 */
    float opponent_health; /* +0x2C */
    float player_health; /* +0x30 */
    float opponent_distance; /* +0x34 */
    int opponent_out_of_range; /* +0x38 */
    int background_attack_active; /* +0x3C */
    int difficulty_index; /* +0x40 */
    unsigned int decision_ready; /* +0x44 */
    int attack_pending; /* +0x48 */
    int jump_attack_pending; /* +0x4C */
    unsigned int block_hold_ticks; /* +0x50 */
    int special_reaction_active; /* +0x54 */
    int block_request; /* +0x58 */
    unsigned int block_subtype; /* +0x5C */
    unsigned int special_reaction_ticks; /* +0x60 */
    int special_reaction_state; /* +0x64 */
    void* script_attack; /* +0x68 */
    int script_attack_ready; /* +0x6C */
    int request_active; /* +0x70 */
    int (*reaction_watcher)(void); /* +0x74 */
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
    AiFightstyleAttack special_move; /* +0x134 */
    float avoidance_area_duration; /* +0x13C */
    float avoidance_position[3]; /* +0x140 */
    unsigned int block_retry_tick; /* +0x14C */
    unsigned int field_150; /* +0x150 */
} DroneAI;

/* Retail ELF functions called before their definitions in this unit. */
static float drone_ai_perform_range_attack(void);
static int drone_ai_check_obstacles(DroneAI* request);
static int drone_ai_is_dizzy_watcher(void);
static void drone_ai_check_next_AIState(DroneAI* drone);
static int drone_ai_process_scripted_cmd(void);
int drone_ai_check_for_normal_blocking(DroneAI* drone);
int drone_ai_check_for_special_move_reaction(DroneAI* drone);
int drone_ai_reversal_watcher(void);
int drone_ai_check_external_request_breakouts(DroneAI* drone);
int drone_ai_handle_arena_collisions(DroneAI* drone);
int drone_ai_process_background_states(DroneAI* drone);
int drone_ai_check_external_requests(DroneAI* drone);
int drone_ai_check_evade_arena(DroneAI* drone);
int drone_ai_check_for_aggressive_throw(DroneAI* drone);
int drone_ai_check_for_big_boss_aggressive_movement(DroneAI* drone);
int drone_ai_check_for_big_boss_passive_movement(DroneAI* drone);
int drone_ai_should_passive_state_switch(DroneAI* drone);
float drone_ai_passive(void);
float drone_ai_attack(void);
float drone_ai_defend(void);
float drone_ai_evade(void);
float drone_ai_mass_attack(void);
float drone_ai_berserk(void);
float drone_ai_dodge_attack(void);
float drone_ai_knockdown(void);
float drone_ai_ducker(void);
static float walk_forward_attackdist_close_with_jexit(void);
static float walk_backward_walk_ticks_jexit(void);
static float drone_ai_scripted_special_attack(void);
static float drone_ai_scripted_change_style(void);
static float drone_ai_perform_block(void);
float drone_ai_perform_reversal(void);
static float jump_towards_opponent_with_attack(void);
static float jump_away_opponent_with_jexit(void);
float p_idle(void);
float p_plyr_aux2(void);
void set_my_secondary_state(int state);
int drone_ai_attacker_reacting_watcher(void);
int drone_ai_attacker_defenseless_watcher(void);
int drone_ai_attacker_not_facing_watcher(void);
int drone_ai_push_watcher(DroneAI* drone);
int drone_ai_opponent_inair_watcher(void);
int drone_ai_beating_the_snot_out_of_him_watcher(void);
int drone_ai_victim_dizzy(void);
int drone_ai_victim_frozen(void);
int drone_ai_victim_speared(void);
int drone_ai_victim_slipping_on_vomit(void);
int drone_ai_victim_ducking(void);
static int drone_ai_victim_avoid(void);
void drone_ai_watcher_calculate_data(void);
static int always_true(void);
int drone_ai_check_reversal_restrictions(void);
int drone_ai_check_escape_restrictions(void);
int drone_ai_check_taunt_restrictions(void);
int drone_ai_check_charge_up_restrictions(void);
int drone_ai_check_throw_restrictions(void);
void execute_rumble(int reaction, int flags);
void execute_hit_voice_sound(int hit_type, int hit_group, int flags);

typedef struct DroneOverrideInfo {
    float likelihood_scale; /* +0x00 */
    unsigned int flags;     /* +0x04 */
} DroneOverrideInfo;

typedef union AiFloatBits {
    float f;
    unsigned int u;
} AiFloatBits;

typedef struct AiProcVtable {
    char pad00[0x18];
    void (*sleep)(void); /* +0x18 */
    char pad1C[8];
    float (*transfer)(float (*entry)(void), float delay); /* +0x24 */
} AiProcVtable;

typedef struct AiSharedAnimations {
    char pad000[0x210];
    AniData* back_getup_3; /* +0x210 */
    char pad214[0x0C];
    AniData* back_getup_9; /* +0x220 */
    char pad224[0x28];
    AniData* sit_getup_6; /* +0x24C */
    AniData* sit_getup_12; /* +0x250 */
    char pad254[0xE4];
    AniData* field_338;
    AniData* field_33C;
    AniData* field_340;
    AniData* field_344;
    AniData* field_348;
    char pad34C[0x24];
    AniData* major_pain_a; /* +0x370 */
    AniData* twitch_death; /* +0x374 */
    AniData* major_pain_b; /* +0x378 */
} AiSharedAnimations;

typedef struct AiWeaponStyleView {
    int style_id;
} AiWeaponStyleView;

typedef struct AiMoveBlendScripts {
    char pad00[0xF0];
    int fatality_scripts[40];
    int push_script;
} AiMoveBlendScripts;

typedef struct AiFightstyleAttackTable {
    char pad000[0xF0];
    AiFightstyleAttack attacks[25];
} AiFightstyleAttackTable;

typedef struct AiSpecialMoveList {
    char pad00[0xC4];
    int ranged_count;
    unsigned int* ranged_commands;
    int count;
    unsigned int* commands;
} AiSpecialMoveList;

typedef struct AiExtendedSpecialMoveList {
    char pad000[0x114];
    int throw_count;
    unsigned int* throw_commands;
} AiExtendedSpecialMoveList;

typedef struct AiStatusSoundView {
    char pad000[0x12C];
    unsigned int field_12C;
    unsigned int field_130;
    char pad134[0x0C];
    unsigned int pain_voice; /* +0x140 */
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
    int close_count;
    unsigned int* close_commands;
    char pad0C4[0x58];
    int distant_count;
    unsigned int* distant_commands;
} AiAirMoveStatus;

typedef struct AiStyleMoveData {
    char pad000[0xA8];
    int reversal_count;
    char pad0AC[0x0C];
    int taunt_count;
    char pad0BC[4];
    int charge_count;
    char pad0C4[4];
    int knockdown_count;
} AiStyleMoveData;

typedef struct AiStyleSlot {
    char pad000[4];
    AiStyleMoveData* moves;
} AiStyleSlot;

typedef struct AiFightStyleRestrictionTable AiFightStyleRestrictionTable;

typedef struct AiCharacterStateWeights {
    int character_id;
    int weights[2][9];
} AiCharacterStateWeights; /* 0x4C */

typedef struct AiTauntCameraData {
    MkObj* object;
    unsigned int object_instance;
    unsigned int ticks;
    float angle;
    float distance;
    float height;
    float depth;
    int active;
} AiTauntCameraData; /* 0x20 */

struct AiFightStyleRestrictionTable {
    int (*always_allowed[5])(void);
    int (*reversal_allowed)(void);
    int (*escape_allowed)(void);
    int (*taunt_allowed)(void);
    int (*charge_allowed)(void);
    int (*knockdown_allowed)(void);
    int (*always_allowed_10)(void);
    int (*throw_allowed)(void);
    int (*always_allowed_12)(void);
    int (*sentinel)(void);
};

const int big_boss_reaction_tbl[0x139] = {
#include "src/game/ai_big_boss_reaction_table.inc"
};

AiFightStyleRestrictionTable fight_style_restriction_table = {
    {always_true, always_true, always_true, always_true, always_true},
    drone_ai_check_reversal_restrictions,
    drone_ai_check_escape_restrictions,
    drone_ai_check_taunt_restrictions,
    drone_ai_check_charge_up_restrictions,
    always_true,
    always_true,
    drone_ai_check_throw_restrictions,
    always_true,
    0
};

float (*g_DroneAIJumpTable[9])(void) = {
    drone_ai_passive,
    drone_ai_attack,
    drone_ai_defend,
    drone_ai_evade,
    drone_ai_mass_attack,
    drone_ai_berserk,
    drone_ai_dodge_attack,
    drone_ai_knockdown,
    drone_ai_ducker
};

static int g_likelihoodForSuperCombo[9] = {
    0, 0, 0, 0, 1, 1, 5, 10, 20
};

static int g_likelihoodForComboBreaker[9] = {
    1, 1, 2, 4, 7, 7, 10, 10, 15
};

static int g_likelihoodForComboBreakout[9] = {
    0, 0, 0, 10, 20, 30, 40, 50, 60
};

static unsigned int g_minBlockHeldTime[9] = {
    300, 300, 240, 180, 120, 120, 60, 60, 50
};

static unsigned int g_likelihoodToThrow[9] = {
    15, 15, 20, 25, 28, 33, 38, 40, 45
};

static unsigned int g_minExtremeBlockHeldTime[9] = {
    300, 300, 240, 180, 180, 180, 120, 120, 90
};

static unsigned int g_likelihoodToAggressiveThrow[9] = {
    5, 5, 10, 10, 20, 20, 25, 30, 35
};

static int g_minDecisionBaseWaitTime[9] = {
    4, 3, 2, 2, 2, 2, 1, 1, 1
};

static int g_randomDecisionBaseWaitTime[9] = {
    4, 3, 2, 2, 2, 2, 2, 1, 1
};

static int g_likelihoodOfBlockCounter[9] = {
    5, 10, 30, 40, 65, 65, 75, 90, 90
};

static int g_blockCounterAdjustor[9] = {
    0, -40, -25, 15, 0, -15, 5, 0, 0
};

static int g_minTimeInBlock[9] = {
    15, 15, 15, 10, 5, 5, 3, 1, 1
};

static int g_blockFakeOutPercentage[9] = {
    75, 75, 65, 60, 60, 45, 35, 25, 15
};

static int g_likelihoodOfRolling[9] = {
    60, 60, 60, 60, 60, 70, 80, 90, 100
};

static int g_likelihoodForSuperDefense[9] = {
    5, 5, 10, 10, 13, 13, 13, 15, 17
};

static unsigned int g_likelihoodForSuperMoveBlocking[9] = {
    30, 40, 60, 70, 70, 70, 80, 85, 85
};

static unsigned int g_likelihoodOfReactAttack[9] = {
    1, 2, 5, 8, 10, 12, 15, 20, 25
};

static int g_maximumNumBlocksInARow[9] = {
    2, 2, 3, 3, 3, 3, 4, 4, 5
};

int g_bigBossBlockAdjuster[9] = {
    5, 5, 5, 5, 5, 5, 10, 10, 15
};

static int g_likelihoodOfBlocking[9] = {
    1, 2, 7, 12, 14, 15, 18, 28, 32
};

static int g_likelihoodOfBlockingInReaction[9] = {
    15, 20, 25, 35, 55, 60, 65, 80, 90
};

static int g_likelihoodOfDuckCounter[9] = {
    1, 2, 5, 6, 10, 12, 14, 15, 20
};

static int g_likelihoodOfMinOppDuckingInTicks[9] = {
    120, 120, 80, 60, 30, 30, 30, 30, 20
};

static int g_likelihoodOfMaxOppDuckingInTicks[9] = {
    240, 240, 120, 120, 120, 100, 80, 80, 70
};

static unsigned int g_likelihoodOfAttacking[9] = {
    7, 14, 36, 44, 50, 62, 70, 79, 88
};

static int g_likelihoodOfComboAttacking[9] = {
    0, 0, 0, 3, 5, 7, 11, 20, 30
};

static int g_likelihoodOfPopUpAttacking[9] = {
    0, 0, 1, 2, 3, 4, 4, 5, 5
};

static int g_likelihoodOfSpecialAttacking[9] = {
    3, 4, 4, 5, 6, 7, 7, 8, 8
};

static int g_likelihoodOfEvadeAttacking[9] = {
    50, 50, 100, 150, 150, 150, 175, 225, 300
};

static AiCharacterStateWeights g_likelihoodOfPCHRChangingState[40] = {
#include "src/game/ai_pchr_state_weights.inc"
};

static int g_likelihoodOfChangingStateE3FingEasyLevel[9] = {
    70, 30, 0, 0, 0, 0, 0, 0, 0
};

static int g_likelihoodOfChangingStateFingEasyLevel[9] = {
    45, 25, 30, 0, 0, 0, 0, 0, 0
};

static int g_likelihoodOfChangingStateEasyLevel[9] = {
    30, 35, 35, 0, 0, 0, 0, 0, 0
};

static int g_likelihoodOfChangingStateMAXLevel[9] = {
    0, 0, 10, 0, 0, 40, 30, 20, 0
};

static int g_likelihoodOfNotFacingAttacking[9] = {
    0, 15, 30, 40, 60, 60, 75, 75, 80
};

static unsigned short g_ProbabilityKatanaSlice[9] = {
    15, 30, 50, 60, 80, 100, 125, 250, 450
};

static unsigned short g_baseSpecialMoveWait[9] = {
    8, 7, 6, 5, 4, 3, 4, 4, 4
};

static unsigned short g_randSpecialMoveWait[9] = {
    6, 5, 4, 4, 3, 3, 3, 2, 2
};

static unsigned short g_likelihoodOfInAirAttack[9] = {
    5, 10, 20, 40, 50, 60, 70, 80, 95
};

static unsigned short g_likelihoodOfPassiveSwitch[9] = {
    90, 80, 70, 60, 50, 40, 30, 20, 10
};

static unsigned int g_minBlockHiHeldTime[9] = {
    300, 300, 240, 180, 120, 120, 60, 50, 40
};

extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern PlyrPdata* his_pdata;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;
extern AiSharedAnimations shared_ani;
DroneAI g_DroneAI2;
DroneAI g_DroneAI1;
int g_game_number = 10;
int g_big_boss_intro_tap_out_f;
int g_droneOverrideActiviated;
int go_into_major_pain_please;
int go_into_twitch_death_please;
int g_fatality_game_number;
DroneOverrideInfo g_DroneOverrideInfo;
extern int force_midpoint_calculation_update;
extern ConstrainInfo constrain_info;
extern unsigned short randu0(unsigned int max);
extern void snd_req(int sound_id);
extern void random_snd_req(int sound_id);
extern void shake_camera(int ticks, float strength);
MslSoundHandle random_hit(int group);
MslSoundHandle random_voice(int group);
void fight_fx_blades_clash(PlyrPdata* player);
float frand(float range);
MslSoundHandle pan_vol_pitch_snd_req(
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
void start_sweat_particles(
    int effect, unsigned int bone, PlyrPdata* player, MkObj* object);
void gut_bleed_me(int size);
void face_bleed_me(int size);
void plyr_bleed_mouth(PlyrPdata* player);
void add_facial_damage(float amount);

int get_player_number(MkObj* player);
MkProc* get_player_proc(MkObj* player);
float xz_distance_between_players(void);
static int drone_ai_im_dizzy(void);
int drone_ai_check_for_berserker_movement();
int drone_ai_fetch_next_AIState(DroneAI* drone);
int handicap_get_current_difficulty(DroneAI* drone);
int is_he_airborn(void);
int is_he_duck_blocking(void);
static int InAttackRange(void);
static int InAttackRange2(void);
static int InAttackRange_close(void);
static int always_false(void);
void advance_cur_cmd_idx(void);
void drone_ai_reset_ai_cmd(void);
float j_exit(void);
float x_block(void);
float side_step_to_center_with_jexit(void);
void step_forward(void);
void step_backward(void);
void jump_towards_opponent(void);
void drone_walk_FB_true(int (*test)(void), unsigned int ticks, int forward,
                        int allow_exit);
void ani_to_frame_x_call(void (*callback)(void), float frame);
int handicap_calc_min_time_in_block(DroneAI* drone);
float drone_ai_watcher(void);
static int drone_ai_force_change_style(DroneAI* drone, int style);
void drone_ai_initialize(DroneAI* drone);
void jump_away_opponent(void);
float jump_away_opponent_with_j_exit(void);
float drone_entry(void);
static float drone_loop(void);
float drone_ai_perform_combo_attack(void);
int drone_ai_check_attack(DroneAI* drone, int force, int immediate);
int is_weapon_style(PlyrFighterDefinition* fighter);
int drone_ai_check_for_evade_attack(DroneAI* drone);
int drone_ai_check_for_evade_movement(DroneAI* drone);
int drone_ai_check_for_extreme_throw(DroneAI* drone);
int drone_ai_check_for_throw(DroneAI* drone);
int drone_ai_check_continue_combo(void);
int drone_ai_check_for_passive_movement(DroneAI* drone);
static int drone_ai_check_change_style(DroneAI* drone);
int drone_ai_check_push(DroneAI* drone);
int drone_ai_check_for_defend_movement(DroneAI* drone);
int drone_ai_check_for_dodge_movement(DroneAI* drone);
int drone_ai_check_for_ducker_movement(DroneAI* drone);
int drone_ai_check_for_aggressive_movement(DroneAI* drone);
int drone_ai_taunt_watcher(DroneAI* drone);
int drone_ai_taunt_watcher_defense(DroneAI* drone);
int is_big_boss(PlyrPdata* player);
void joy_dash_back(void);
float drone_ai_perform_attack(void);
void set_my_state(int state);
static float side_step_to_center_attack_with_jexit(void);
static float drone_ai_perform_push(void);
static float drone_ai_perform_low_attack(void);
static float drone_ai_perform_weapon_attack(void);
static float drone_ai_perform_impale_attack(void);
static float drone_ai_stupid_watcher(void);
static int HeIsNotFacing(void);
static float drone_ai_avoid_position_now(void);
static float catch_opponent(void);
static float drone_ai_attack_obstacle_now(void);
static float drone_ai_counter_attack_now(void);
static float side_step_to_center_long_with_jexit(void);
float dash_back_with_jexit(void);
float drone_walk_backwards_with_jexit(void);
float drone_walk_backwards_further_with_jexit(void);
float jump_towards_opponent_with_j_exit(void);
float change_to_weapon_style_with_j_exit(void);
void drone_ai_perform_jump_attack(DroneAI* request);
unsigned int handicap_calc_likelihood_of_blocking(DroneAI* drone);
int drone_ai_victim_speared_2(void);
int drone_ai_attacker_defenseless(DroneAI* drone);
static float drone_ai_avoid_danger_area_now(void);
int drone_ai_can_push(DroneAI* drone);
static float drone_ai_perform_knockdown(void);
int am_i_a_big_character(void);
int am_i_airborn(void);
float j_flying_kick(void);
float j_flying_kick2(void);
void ck_rumble_controller(int pad, int strength, int duration);
void uv_to_opponent(Vec* direction);
void snd_req_delay(int sound, int delay);
void pre_attack_chores(void);
void plyr_going_to_attack_with_action(unsigned int action);
void share_my_attack_info(float duration, float divisor);
void init_ground_move_no_aniproc(void);
void face_opponent_now(void);
int random_foot(int group);
void tightrope_restrictions_off(void);
void transition_to_anim_script(
    AnimPdata* anim, AniData* animation, int transition, float blend);
void set_root_and_obj_movement_weights(
    AnimPdata* animation, float root_weight, float object_weight);
void ani_to_frame_x(float frame);
float p_animate(void);
float p_camera_proc(void);
float j_stay_down_dead(void);
static float dk_screen_taunt(void);
float drone_ai_perform_script_attack(void);
static float drone_ai_scripted_attack(void);
void back_to_normal(void);
void init_ground_move(void);
int do_i_have_life_left(void);
float r_call_player_char_script_function(void);
void p_blend_to_stance_in_10(void);
void rotate_towards_him(float rate);
float end_of_round_check(void);
static int handicap_likelihood_for_combo_breaker(DroneAI* drone);
float drone_ai_change_style(void);
void bgnd_restore_player(void);
void enable_all_my_blocking(void);
int is_my_chest_to_screen(void);
void blend_to_ani(AniData* animation, int transition, float blend);
void set_ani_speed(float speed);
void ani_loop_more_frames(float frames);
void blend_to_fstance(float blend);
int blend_to_stance(float blend);
void ani_to_blend_frame(float frame);
void advance_active_moveset(PlyrPdata* player);
void advance_sidekick_with_moveset(PlyrPdata* player);
static AiFightstyleAttack* get_special_move(void);
static int get_random_fightstyle_index(
    int attack_group, FighterAiTable* table, int selection_mode);
static AiFightstyleAttack* get_random_fightstyle_attack(
    PlyrFighterDefinition* fighter, int attack_group, int flags);
int drone_ai_enemy_inair_attack(DroneAI* drone);
static AiFightstyleAttack* drone_ai_choose_move_from_category(
    int category, unsigned int likelihood, int* is_script);
static float drone_ai_special_attack_now(void);
int segment_against_obstacle_list(
    const Vec* start, const Vec* end, Vec* hit, ConstrainInfo* info);
void drone_step_LR_true(
    int (*test)(void), unsigned int ticks, int move_right);
ScreenObj* display_image_by_plyr(
    int slot, const char* image_name, PlyrInfo* source,
    int unused, float y_offset);
void fight_fx_im_hit_with_breaker_flash(
    int player, MkObj* object, int bone, int use_bone, float y_offset);
extern int f_fatality_was_done;
extern int mode_of_play;
extern float game_speed;
extern AiFightStyleRestrictionTable fight_style_restriction_table;
int get_game_state(void);
int get_fatality_available_flag(void);
int can_i_do_fatality_now(int player);
float do_my_suicide(void);
static unsigned int handicap_calc_likelihood_of_blocking_in_reaction(
    DroneAI* drone);
int drone_ai_should_be_blocking(DroneAI* drone, int reaction);
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
static float drone_ai_dodge_3d_with_counter(void);
static float drone_ai_duck_attack(void);
static float drone_ai_duck_throw_attack(void);
int drone_ai_check_for_side_step_counter_attack(DroneAI* drone);
int drone_ai_should_evade_attack(DroneAI* drone);
int drone_ai_charge_up_watcher(DroneAI* drone);
int drone_ai_charge_up_watcher_defense(DroneAI* drone);
float drone_ai_perform_charge_up(void);
float drone_ai_perform_taunt(void);
int get_ladder_position(void);
int trial_get_drone_difficulty(void);
int mk_chess_get_current_difficulty_for_ai(int side);
extern int g_GameLossesInARow;
extern float inverse_game_speed;
int am_i_on_the_left(void);
void init_3d_move_no_aniproc(void);
static float p_lookat_cam(void);
static void ai_side_clearances(float* right, float* left);
static float ai_backward_clearance(void);
static int drone_ai_should_evade_attack(DroneAI* drone);
static AiTauntCameraData at_cam_data;
static float jump_towards_opponent_with_jexit(void);
float walk_forward_attackdist_with_jexit(void);
static float walk_forward_attackdist2_with_jexit(void);
static float step_forward_with_jexit(void);
static float step_backward_with_jexit(void);
int drone_ai_victim_dizzy_2(void);
static int drone_ai_victim_dizzy_3(void);
static int drone_ai_victim_throw_attempt(void);
float do_my_fatality(void);
float do_my_2nd_fatality(void);
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

/*
 * Retail repeats this complete collision sequence in five callers. Their
 * current emissions match retail size and opcode counts; remaining differences
 * are FP/GPR allocation and scheduling within the inlined sequence.
 */
static inline void ai_side_clearances(float* right, float* left) {
    Vec origin;
    Vec end;
    Vec hit;
    Vec unit;
    float delta_x;
    float delta_z;

    if (plyr_obj == 0) {
        *right = 100.0f;
        *left = 100.0f;
    } else {
        origin.x = plyr_obj->pos.value.x;
        origin.y = plyr_obj->pos.value.y;
        origin.z = plyr_obj->pos.value.z;
        unit.x = gxMathSin(plyr_obj->ang.y);
        unit.y = 0.0f;
        unit.z = gxMathCos(plyr_obj->ang.y);
        delta_x = 100.0f * unit.z;
        delta_z = 100.0f * -unit.x;

        end.x = origin.x + delta_x;
        end.y = origin.y + unit.y;
        end.z = origin.z + delta_z;
        if (segment_against_obstacle_list(
                &origin, &end, &hit, &constrain_info)) {
            *right = uv_v3_to_v3_dist(&unit, &hit, &origin);
        } else {
            *right = 100.0f;
        }

        end.x = origin.x - delta_x;
        end.y = origin.y - unit.y;
        end.z = origin.z - delta_z;
        if (segment_against_obstacle_list(
                &origin, &end, &hit, &constrain_info)) {
            *left = uv_v3_to_v3_dist(&unit, &hit, &origin);
        } else {
            *left = 100.0f;
        }
    }
}

static inline float ai_sqrt_table(float squared) {
    AiFloatBits bits;
    unsigned int exponent;

    bits.f = squared;
    if (squared <= 0.0f) {
        return 0.0f;
    }
    exponent =
        (((bits.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000;
    bits.u =
        (unsigned int)GXMathSqrtTable[(bits.u >> 11) & 0x1FFF] << 8;
    bits.u |= exponent;
    return 0.5f * (bits.f * (3.0f - (bits.f * bits.f) / squared));
}

static inline float ai_backward_clearance(void) {
    Vec origin;
    Vec direction;
    Vec hit;
    Vec unit;

    if (plyr_obj == 0) {
        return -1.0f;
    }
    origin.x = plyr_obj->pos.value.x;
    origin.y = plyr_obj->pos.value.y;
    origin.z = plyr_obj->pos.value.z;
    direction.x = gxMathSin(plyr_obj->ang.y);
    direction.y = 0.0f;
    direction.z = gxMathCos(plyr_obj->ang.y);
    direction.x = -100.0f * direction.x;
    direction.z = -100.0f * direction.z;
    direction.x += origin.x;
    direction.y += origin.y;
    direction.z += origin.z;
    if (segment_against_obstacle_list(
            &origin, &direction, &hit, &constrain_info)) {
        return uv_v3_to_v3_dist(&unit, &hit, &origin);
    }
    return 100.0f;
}

void liukang_in_fight_random_snd_check(void) {
    int character = plyr_pdata->character_id;

    if ((character == 0x10 || character == 0x11) && randu0(100) < 10) {
        random_snd_req(0x23);
    }
}

/* Soft ceiling: pointer/instance validation, call ABI, and operations match.
 * Retail retains one unconditional branch around the nulling block that MWCC
 * folds out of the equivalent structured condition below. */
void dk_taunt_at_screen(void) {
    PlyrPdata* player;
    MkProc* proc;

    player = plyr_pdata;
    proc = player->player_proc;
    if (proc != 0) {
        if (proc->instance != player->player_proc_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    xfer_player_proc(proc, dk_screen_taunt);
}

static float dk_screen_taunt(void) {
    int frames;

    bgnd_restore_player();
    init_ground_move_no_aniproc();
    set_my_state(0x600);
    enable_all_my_blocking();
    if (!is_my_chest_to_screen()) {
        blend_to_ani(plyr_pdata->turn_to_screen_animation, 3, 0.2f);
        set_ani_speed(2.0f);
        frames = 30;
        while (--frames != 0) {
            force_midpoint_calculation_update = 1;
            ani_loop_more_frames(1.0f);
        }
        set_ani_speed(1.0f);
        blend_to_fstance(0.1f);
    }
    if (randu0(100) < 50) {
        snd_req(0x1B4);
    } else {
        snd_req(0x1B5);
    }
    blend_to_ani(plyr_pdata->screen_taunt_animation, 3, 0.1f);
    set_ani_speed(1.0f);
    ani_to_blend_frame(20.0f);
    set_my_state(0);
    blend_to_stance(0.1f);
    AI_TRANSFER(j_exit);
    return 0.0f;
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

float big_boss_taunt_cam_cut(void) {
    unsigned int camera_roll;
    unsigned short sound_roll;

    camera_roll = randu0(100);
    init_3d_move_no_aniproc();
    set_my_state(0x420A);

    if (camera_roll < 33) {
        if (am_i_on_the_left()) {
            at_cam_data.ticks = 0x11D;
            at_cam_data.angle = -0.9599311f;
            at_cam_data.distance = 2.35f;
            at_cam_data.height = 0.6f;
            at_cam_data.depth = -1.1f;
            at_cam_data.active = 1;
            at_cam_data.object = plyr_obj;
            at_cam_data.object_instance = plyr_obj->hdr.instance;
            xfer_camera(p_lookat_cam, 0);
        } else {
            at_cam_data.ticks = 0x11D;
            at_cam_data.angle = 0.9599311f;
            at_cam_data.distance = 2.35f;
            at_cam_data.height = 0.6f;
            at_cam_data.depth = -1.1f;
            at_cam_data.active = 1;
            at_cam_data.object = plyr_obj;
            at_cam_data.object_instance = plyr_obj->hdr.instance;
            xfer_camera(p_lookat_cam, 0);
        }
    } else if (camera_roll < 66) {
        if (am_i_on_the_left()) {
            at_cam_data.ticks = 0x11D;
            at_cam_data.angle = -0.9599311f;
            at_cam_data.distance = 2.85f;
            at_cam_data.height = 0.6f;
            at_cam_data.depth = 2.0f;
            at_cam_data.active = 1;
            at_cam_data.object = plyr_obj;
            at_cam_data.object_instance = plyr_obj->hdr.instance;
            xfer_camera(p_lookat_cam, 0);
        } else {
            at_cam_data.ticks = 0x11D;
            at_cam_data.angle = 0.9599311f;
            at_cam_data.distance = 2.35f;
            at_cam_data.height = 0.6f;
            at_cam_data.depth = 2.1f;
            at_cam_data.active = 1;
            at_cam_data.object = plyr_obj;
            at_cam_data.object_instance = plyr_obj->hdr.instance;
            xfer_camera(p_lookat_cam, 0);
        }
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
    return 0.0f;
}


/* Soft ceiling: exact size, frame, save/restore, calls, layout, and math.
 * Residue is confined to two validated-pointer selection diamonds. */
static float p_lookat_cam(void) {
    CameraObj* camera;
    MkObj* target;
    float saved_position_x;
    float saved_position_y;
    float saved_position_z;
    float saved_target_x;
    float saved_target_y;
    float saved_target_z;
    Vec look_target;
    float angle;
    float offset_x;
    float position_y_offset;
    float offset_z;
    float sine;
    float cosine;

    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }
    if (camera == 0) {
        AI_TRANSFER(p_camera_proc);
        return 0.0f;
    }

    target = at_cam_data.object;
    if (target != 0) {
        if (target->hdr.instance != at_cam_data.object_instance) {
            target = 0;
        }
    } else {
        target = 0;
    }
    if (target == 0) {
        AI_TRANSFER(p_camera_proc);
        return 0.0f;
    }

    saved_position_x = camera->pos.x;
    saved_position_y = camera->pos.y;
    saved_position_z = camera->pos.z;
    saved_target_x = camera->ang.x;
    saved_target_y = camera->ang.y;
    saved_target_z = camera->ang.z;
    if (at_cam_data.ticks != 0) {
        angle = target->ang.y + at_cam_data.angle;
        angle =
            0.000005992112f *
            (float)((int)(166886.1f * angle) & 0xFFFFF);
        sine = gxMathSin(angle);
        cosine = gxMathCos(angle);
        position_y_offset = at_cam_data.depth;
        offset_x = sine * at_cam_data.distance;
        offset_z = cosine * at_cam_data.distance;
        while (at_cam_data.ticks != 0) {
            camera->pos.x = target->pos.value.x + offset_x;
            camera->pos.y = target->pos.value.y + position_y_offset;
            camera->pos.z = target->pos.value.z + offset_z;
            look_target.x = target->pos.value.x;
            look_target.y = target->pos.value.y;
            look_target.z = target->pos.value.z;
            look_target.y += at_cam_data.height;
            look_at_target(&look_target);
            AI_SLEEP(1.0f);
            at_cam_data.ticks--;
        }
    }

    if (target == g_game_info.plyr0.slot.mirror_a) {
        show_player(g_game_info.plyr1.slot.pdata);
    } else {
        show_player(g_game_info.plyr0.slot.pdata);
    }
    if (at_cam_data.active != 0) {
        camera->pos.x = saved_position_x;
        camera->pos.y = saved_position_y;
        camera->pos.z = saved_position_z;
        camera->ang.x = saved_target_x;
        camera->ang.y = saved_target_y;
        camera->ang.z = saved_target_z;
    }
    force_midpoint_calculation_update = 1;
    AI_TRANSFER(p_camera_proc);
    return 0.0f;
}

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

unsigned int big_boss_reaction_remap(unsigned int reaction) {
    ScreenObj* image;
    int mapped_reaction;
    int i;

    if (reaction == 0x7A) {
        fight_fx_im_hit_with_breaker_flash(
            his_pdata->plyr_num, his_obj, 0x10, 0, 1.7f);
        image = display_image_by_plyr(
            0x10005, "BREAKER", his_pdata->plyr_info, 0, 1.7f);
        if (image != 0 && plyr_pdata->breaker_strength == 0) {
            for (i = 0; i < 4; i++) {
                image->pfx2d->verts[i].r = 0x80;
                image->pfx2d->verts[i].g = 0;
                image->pfx2d->verts[i].b = 0;
                image->pfx2d->verts[i].a = 0xFF;
            }
        }
    }

    if (reaction >= 0x139) {
        return reaction;
    }
    mapped_reaction = big_boss_reaction_tbl[reaction];
    if (mapped_reaction == -1) {
        mapped_reaction = reaction;
    }
    return mapped_reaction;
}


void set_attackers_attack_region(int region) {
    plyr_pdata->attack_region = region;
}


float force_some_distance(void) {
    if (ai_backward_clearance() > 2.1336f &&
        !is_big_boss(plyr_pdata)) {
        AI_TRANSFER(joy_dash_back);
        return 0.0f;
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}


float give_some_distance(void) {
    float distance;
    float maximum_distance;

    distance = xz_distance_between_players();
    maximum_distance = 5.9457946f;
    if (his_pdata->death_type == 1) {
        maximum_distance = 9.290304f;
    }
    if (distance > maximum_distance) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    if (ai_backward_clearance() > 2.1336f) {
        if (plyr_pdata != 0 && is_big_boss(plyr_pdata)) {
            step_backward();
        } else {
            AI_TRANSFER(joy_dash_back);
            return 0.0f;
        }
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}


/* Soft ceiling: all operations, operands, calls, and switch outcomes match.
 * Retail retains one redundant branch between the explicit empty case groups. */
float go_into_twitch_death(void) {
    init_ground_move_no_aniproc();
    switch (plyr_pdata->death_type) {
    case 0:
    case 1:
    case 2:
        break;
    case 3:
        break;
    case 4:
        plyr_obj->flags_09_bits.head_tracking = 0;
        tightrope_restrictions_off();
        plyr_anim_pdata->step = 1.0f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.twitch_death, 0, 0.05f);
        AI_SLEEP(1.0f);
        ani_to_frame_x(2.0f);
        plyr_obj->flags_09_bits.launched = 0;
        xfer_proc(plyr_anim_proc, p_animate);
        break;
    default:
        break;
    }
    AI_TRANSFER(j_stay_down_dead);
    return 0.0f;
}


/* Soft ceiling: every case body, call, typed access, and side effect is exact.
 * Retail retains two redundant branches in the empty/default switch dispatch;
 * current MWCC folds them, producing the entire eight-byte size difference. */
float go_into_major_pain(void) {
    back_to_normal();
    plyr_obj->flags_09_bits.head_tracking = 0;
    tightrope_restrictions_off();
    switch (plyr_pdata->death_type) {
    case 4:
        if (randu0(100) < 75 ||
            g_game_info.feature_flags.bits.high_bit) {
            init_ground_move_no_aniproc();
            plyr_anim_pdata->step = 0.6f;
            plyr_anim_pdata->transition_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.major_pain_a,
                0, 0.05f);
            AI_SLEEP(1.0f);
            ani_to_frame_x(7.0f);
            plyr_obj->flags_09_bits.launched = 0;
        } else {
            init_ground_move_no_aniproc();
            plyr_anim_pdata->step = 1.0f;
            plyr_anim_pdata->transition_weight = 0.5f;
            plyr_anim_pdata->flags |= 0x40;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.twitch_death,
                0, 0.05f);
            ani_to_frame_x(2.0f);
            plyr_obj->flags_09_bits.launched = 0;
        }
        if (((AiStatusSoundView*)plyr_pdata->status_flags)->pain_voice != 0) {
            random_snd_req(
                ((AiStatusSoundView*)plyr_pdata->status_flags)->pain_voice);
        }
        plyr_anim_pdata->step = 0.6f;
        xfer_proc(plyr_anim_proc, p_animate);
        break;
    case 1:
        init_ground_move_no_aniproc();
        if (!is_weapon_style(plyr_pdata->fighter_definition)) {
            if (!is_big_boss(his_pdata)) {
                MkProc* opponent_proc;

                if (get_player_number(his_obj) == 0) {
                    opponent_proc = (MkProc*)g_game_info.plyr0.idle_proc;
                } else {
                    opponent_proc = (MkProc*)g_game_info.plyr1.idle_proc;
                }
                xfer_player_proc(opponent_proc, force_some_distance);
            }
            plyr_obj->flags_09_bits.head_tracking = 0;
            tightrope_restrictions_off();
            plyr_anim_pdata->step = 0.6f;
            if (((AiStatusSoundView*)plyr_pdata->status_flags)->pain_voice != 0) {
                random_snd_req(
                    ((AiStatusSoundView*)plyr_pdata->status_flags)->pain_voice);
            }
            plyr_anim_pdata->transition_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.major_pain_b,
                0, 0.05f);
            AI_SLEEP(1.0f);
            ani_to_frame_x(2.0f);
            init_air_move();
            plyr_anim_pdata->step = 0.6f;
            xfer_proc(plyr_anim_proc, p_animate);
        }
        break;
    default:
        break;
    }
    AI_TRANSFER(j_stay_down_dead);
    return 0.0f;
}

float getup_from_ground(void) {
    DroneAI* drone;
    MkObj* object;
    unsigned int script;
    int use_random_voice = 0;

    while (!do_i_have_life_left()) {
        AI_SLEEP(1.0f);
    }

    back_to_normal();
    object = plyr_obj;
    object->flags_09_bits.head_tracking = 0;
    tightrope_restrictions_off();

    if (plyr_pdata->death_type == 1) {
        init_ground_move_no_aniproc();
        if (is_weapon_style(plyr_pdata->fighter_definition)) {
            drone = get_player_number(plyr_obj) == 0
                        ? &g_DroneAI1
                        : &g_DroneAI2;
            drone_ai_force_change_style(drone, 3);
        }

        script =
            ((AiStatusSoundView*)plyr_pdata->status_data)->field_12C;
        if (script != 0) {
            active_cmdscript->unk28 = script;
            AI_TRANSFER(r_call_player_char_script_function);
            return 0.0f;
        }

        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.field_338, 3, 0.05f);
        AI_SLEEP(1.0f);
        ani_to_frame_x(2.0f);
        plyr_obj->flags_09_bits.launched = 0;
        plyr_anim_pdata->step = 0.6f;
        ani_to_frame_x(20.0f);
        random_hit(7);

        switch (plyr_pdata->character_id) {
        case 11:
            snd_req(0x223);
            break;
        case 19:
            snd_req(0x24E);
            break;
        case 16:
        case 17:
            snd_req(0x29C);
            break;
        case 25:
            snd_req(0x319);
            break;
        default:
            use_random_voice = 1;
            break;
        }

        ani_to_frame_x(24.0f);
        if (use_random_voice) {
            random_voice(0);
        }
        ani_to_frame_x(30.0f);
        random_hit(0);
        ani_to_frame_x(106.0f);
        init_ground_move();
    } else if (plyr_pdata->death_type == 2) {
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 0.6f;
        if (randu0(100) < 50 ||
            g_game_info.feature_flags.bits.high_bit) {
            plyr_anim_pdata->transition_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.field_33C, 3, 0.05f);
            AI_SLEEP(1.0f);
            ani_to_frame_x(80.0f);
            init_ground_move();
        } else {
            plyr_anim_pdata->transition_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.field_340, 3, 0.05f);
            AI_SLEEP(1.0f);
            ani_to_frame_x(100.0f);
            init_ground_move();
        }
    } else if (plyr_pdata->death_type == 7) {
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.field_344, 3, 0.05f);
        AI_SLEEP(1.0f);
    } else if (plyr_pdata->death_type == 4) {
        init_ground_move_no_aniproc();
        script =
            ((AiStatusSoundView*)plyr_pdata->status_data)->field_130;
        if (script != 0) {
            active_cmdscript->unk28 = script;
            AI_TRANSFER(r_call_player_char_script_function);
            return 0.0f;
        }

        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.field_348, 3, 0.05f);
        AI_SLEEP(1.0f);
        ani_to_frame_x(2.0f);
        ani_to_frame_x(88.0f);
        init_ground_move();
    } else if (plyr_pdata->death_type == 0) {
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.back_getup_9, 3, 0.05f);
        AI_SLEEP(1.0f);
    } else if (plyr_pdata->death_type == 3) {
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 0.6f;
        if ((plyr_anim_pdata->flags & 8) != 0) {
            plyr_anim_pdata->transition_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.back_getup_9,
                3, 0.05f);
            AI_SLEEP(1.0f);
        } else {
            plyr_anim_pdata->transition_weight = 0.5f;
            transition_to_anim_script(
                plyr_anim_pdata, shared_ani.back_getup_3,
                3, 0.05f);
            AI_SLEEP(1.0f);
        }
    } else if (plyr_pdata->death_type == 6) {
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.sit_getup_12, 3, 0.05f);
        AI_SLEEP(1.0f);
    } else if (plyr_pdata->death_type == 5) {
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 0.6f;
        plyr_anim_pdata->transition_weight = 0.5f;
        transition_to_anim_script(
            plyr_anim_pdata, shared_ani.sit_getup_6, 3, 0.05f);
        AI_SLEEP(1.0f);
    } else if (plyr_pdata->death_type == 10) {
        init_ground_move_no_aniproc();
        plyr_anim_pdata->step = 1.0f;
        blend_to_stance(0.1f);
    } else {
        init_ground_move();
        AI_TRANSFER(p_blend_to_stance_in_10);
        return 0.0f;
    }

    if (plyr_pdata->death_type != 10) {
        ani_to_blend_frame(10.0f);
    }
    back_to_normal();
    rotate_towards_him(0.1f);
    if (plyr_pdata->death_type != 10) {
        plyr_anim_pdata->step = 1.2f;
        AI_TRANSFER(p_blend_to_stance_in_10);
        return 0.0f;
    }

    AI_TRANSFER(j_exit);
    return 0.0f;
}

/* Soft ceiling: exact behavior, calls, widths, frame, and switch lowering.
 * Residue is the equivalent region-selection branch polarity, one coalesced
 * join move, and GPR scheduling at the fighter-data load. */
void whoosh_fx(int hit_type) {
    AiFighterSoundView* fighter;
    unsigned int move_flags;
    int region;
    int is_dead_liukang;

    if (plyr_pdata->attack_region != 0x10 ||
        plyr_pdata->weapon_impact == 0) {
        region = plyr_pdata->attack_region;
    } else {
        region = plyr_pdata->weapon_impact->attack_region;
    }
    fighter = (AiFighterSoundView*)plyr_pdata->fighter_definition;
    if (fighter->move_data->flags != 0) {
        move_flags = fighter->move_data->flags->weapon_flags;
    } else {
        move_flags = 0;
    }
    if (plyr_pdata->character_id == 0x10 &&
        !plyr_pdata->plyr_info->flags_14_bits.alternate_costume) {
        is_dead_liukang = 1;
    } else {
        is_dead_liukang = 0;
    }
    if (is_dead_liukang) {
        switch (region) {
        case 0:
        case 1:
        case 2:
        case 6:
        case 7:
        case 10:
        case 11:
        case 12:
            snd_req(0x280);
            break;
        default:
            break;
        }
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

void dead_liukang_snd_chain_check(
    PlyrPdata* player, int base_delay, unsigned short delay_range,
    unsigned int likelihood) {
    int delay;
    int is_dead_liukang;

    if (player->character_id == 0x10 &&
        !player->plyr_info->flags_14_bits.alternate_costume) {
        is_dead_liukang = 1;
    } else {
        is_dead_liukang = 0;
    }
    if (is_dead_liukang && randu0(100) < likelihood) {
        delay = base_delay + randu0(delay_range);
        snd_req_delay(randu0(5) + 0x27B, delay + 1);
    }
}

MslSoundHandle random_block_hit(
    int attacker_style, int defender_style, int heavy) {
    unsigned int sound_id;
    MslSoundHandle result;

    sound_id = -1;
    result = 0;
    if (attacker_style >= 0 && attacker_style < 6 &&
        defender_style >= 0 && defender_style < 6) {
        if (((unsigned int)attacker_style <= 1U || attacker_style == 2) &&
            (defender_style == 0 || defender_style == 2 ||
             defender_style == 1)) {
            if (heavy == 1 || defender_style == 1 || attacker_style == 1) {
                if (randu0(100) < 50) {
                    sound_id = 0xD95;
                } else {
                    sound_id = 0xD9A;
                }
            } else {
                sound_id = 0xD94;
            }
            fight_fx_blades_clash(plyr_pdata);
            if (sound_id == -1) {
                return 0;
            }
            return pan_vol_pitch_snd_req(
                sound_id, 0.0f, 1.0f, 0.95f + frand(0.11f));
        }
        if ((attacker_style == 3 &&
             (unsigned int)defender_style <= 1U) ||
            (defender_style == 3 &&
             (attacker_style == 0 || attacker_style == 1))) {
            if (heavy == 1) {
                if (randu0(100) < 50) {
                    sound_id = 0xD95;
                } else {
                    sound_id = 0xD9A;
                }
            } else {
                sound_id = 0xD96;
            }
        }
        if (attacker_style == 3 && defender_style == 3) {
            if (heavy == 1) {
                sound_id = 0xD99;
            } else {
                sound_id = 0xD98;
            }
        }
        if (sound_id == -1) {
            return 0;
        }
        result = pan_vol_pitch_snd_req(
            sound_id, 0.0f, 1.0f, 0.85f + frand(0.21f));
    }
    return result;
}

void got_hit_fx(
    int hit_type, int hit_group, int camera_strength, int blood_level,
    int sweat, int flags, float facial_damage) {
    AiMoveSoundFlags* move_flags;
    unsigned int weapon_flags;
    int resolved_region;
    int uses_face_blood;
    int blood_size;

    uses_face_blood = 0;
    move_flags = ((AiFighterSoundView*)his_pdata->fighter_definition)
                     ->move_data->flags;
    blood_size = 0;
    weapon_flags = move_flags != 0 ? move_flags->weapon_flags : 0;
    resolved_region = his_pdata->attack_region;
    if (resolved_region == 0x10 && his_pdata->weapon_impact != 0) {
        resolved_region = his_pdata->weapon_impact->attack_region;
    }
    if (resolved_region == 0 && (weapon_flags & 4) != 0) {
        uses_face_blood = 1;
    }

    execute_hit_voice_sound(hit_type, hit_group, flags);
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

    if (blood_level == 0 && uses_face_blood == 1 &&
        (flags & 0x10) == 0) {
        if (hit_type == 2) {
            blood_level = 14;
        } else if (hit_type == 0) {
            blood_level = 5;
        }
    }

    if (blood_level != 0) {
        if (uses_face_blood == 1) {
            switch (blood_level) {
            case 12:
            case 15:
                blood_size = 1;
                /* fall through */
            case 11:
            case 14:
                blood_size++;
                /* fall through */
            case 10:
            case 13:
            case 16:
                blood_size += 2;
                if (blood_size > 3) {
                    blood_size = 3;
                }
                if (his_pdata->attack_type == 1) {
                    start_blood_particles(
                        0x18, 9, plyr_pdata, plyr_obj);
                } else if (his_pdata->attack_type == 2) {
                    start_blood_particles(
                        0x18, 9, plyr_pdata, plyr_obj);
                } else if (blood_level <= 12) {
                    start_blood_particles(
                        0x29, 9, plyr_pdata, plyr_obj);
                } else if (blood_level == 16) {
                    start_blood_particles(
                        0x18, 9, plyr_pdata, plyr_obj);
                } else {
                    start_blood_particles(
                        0x18, 9, plyr_pdata, plyr_obj);
                }
                gut_bleed_me(blood_size);
                break;
            case 3:
            case 6:
            case 9:
                blood_size = 1;
                /* fall through */
            case 2:
            case 5:
            case 8:
                blood_size++;
                /* fall through */
            case 1:
            case 4:
            case 7:
                blood_size++;
                if (blood_size > 3) {
                    blood_size = 3;
                }
                if (blood_level <= 3) {
                    start_blood_particles(
                        0x39, 9, plyr_pdata, plyr_obj);
                } else if (blood_level <= 6) {
                    start_blood_particles(
                        0x18, 0x10, plyr_pdata, plyr_obj);
                } else {
                    start_blood_particles(
                        1, 0x10, plyr_pdata, plyr_obj);
                }
                face_bleed_me(blood_size);
                break;
            }
        } else if ((unsigned int)hit_type <= 1U || hit_type == 4) {
            switch (blood_level) {
            case 3:
            case 6:
            case 9:
                blood_size = 1;
                /* fall through */
            case 2:
            case 5:
            case 8:
                blood_size++;
                /* fall through */
            case 1:
            case 4:
            case 7:
                blood_size++;
                if (blood_size > 3) {
                    blood_size = 3;
                }
                if (blood_level <= 3) {
                    start_blood_particles(
                        0x39, 9, plyr_pdata, plyr_obj);
                } else if (blood_level <= 6) {
                    start_blood_particles(
                        0x18, 0x10, plyr_pdata, plyr_obj);
                } else {
                    start_blood_particles(
                        1, 0x10, plyr_pdata, plyr_obj);
                }
                face_bleed_me(blood_size);
                break;
            }
        }

        if ((hit_group == 4 || hit_group == 5) &&
            uses_face_blood == 0 && hit_type == 2 &&
            (flags & 0x10) == 0) {
            plyr_bleed_mouth(plyr_pdata);
            start_blood_particles(0x31, 0x10, plyr_pdata, plyr_obj);
        }
    }
    if (sweat > 0) {
        start_sweat_particles(7, 0x10, plyr_pdata, plyr_obj);
    }
    execute_rumble(camera_strength, flags);
    add_facial_damage(facial_damage);
}

static inline int ai_weapon_material(PlyrFighterDefinition* fighter) {
    AiMoveSoundFlags* move_flags;
    unsigned int flags;

    if (!is_weapon_style(fighter)) {
        return 5;
    }
    move_flags = ((AiFighterSoundView*)fighter)->move_data->flags;
    flags = move_flags != 0 ? move_flags->weapon_flags : 0;
    if ((flags & 0x40) != 0) {
        if ((flags & 2) != 0) {
            return 1;
        }
        return 0;
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

void blocked_fx(
    int reaction, int camera_strength, int unused3, int unused4,
    int rumble_flags) {
    AiFighterSoundView* opponent_fighter;
    AiFighterSoundView* player_fighter;
    AiMoveSoundFlags* move_flags;
    unsigned int opponent_flags;
    unsigned int player_flags;
    unsigned int blood_bone;
    int bleeding_weapon;
    int opponent_material;
    int player_material;
    int region;

    bleeding_weapon = 0;

    opponent_fighter =
        (AiFighterSoundView*)his_pdata->fighter_definition;
    move_flags = opponent_fighter->move_data->flags;
    opponent_flags =
        move_flags != 0 ? move_flags->weapon_flags : 0;

    player_fighter =
        (AiFighterSoundView*)plyr_pdata->fighter_definition;
    move_flags = player_fighter->move_data->flags;
    player_flags = move_flags != 0 ? move_flags->weapon_flags : 0;

    region = his_pdata->attack_region;
    if (region == 0x10 && his_pdata->weapon_impact != 0) {
        region = his_pdata->weapon_impact->attack_region;
    }
    if (region == 0) {
        if ((opponent_flags & 4) != 0) {
            bleeding_weapon = 1;
        }
        opponent_material =
            ai_weapon_material(his_pdata->fighter_definition);
    } else {
        opponent_material = 5;
    }

    if (is_weapon_style(plyr_pdata->fighter_definition)) {
        player_material =
            ai_weapon_material(plyr_pdata->fighter_definition);
        if ((player_flags & 0x80) == 0 &&
            (player_material == 0 || player_material == 1)) {
            player_material = 3;
        }
    } else {
        player_material = 5;
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
    if (bleeding_weapon == 1 && player_material == 5) {
        blood_bone = 0x18;
        if (plyr_pdata->previous_state == 0xA01) {
            blood_bone = 0x19;
        } else if (plyr_pdata->previous_state == 0xA02) {
            blood_bone = 0x14;
        } else if (plyr_pdata->previous_state == 0xA03) {
            blood_bone = 0x19;
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

void execute_rumble(int reaction, int flags) {
    int strength;
    int scale;

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
            scale = 3;
            break;
        case 3:
            strength = 10;
            scale = 2;
            break;
        case 4:
            strength = 8;
            scale = 3;
            break;
        case 5:
            strength = 4;
            scale = 1;
            break;
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
            ck_rumble_controller(
                g_game_info.plyr0.controller_slot,
                strength, scale * 15);
        } else if (aproc->pid == 0x1002) {
            ck_rumble_controller(
                g_game_info.plyr1.controller_slot,
                strength, scale * 15);
        }
    }
}

/* Soft ceiling: material selection, switches, calls, widths, and ABI match.
 * MWCC folds one equivalent impact-selection join, leaving a four-byte size
 * difference plus temporary-pointer coloring. */
void execute_hit_voice_sound(int hit_type, int hit_group, int flags) {
    int material;
    int attack_region;
    int sound_group;

    material = ai_weapon_material(his_pdata->fighter_definition);

    if (his_pdata->attack_region != 0x10 ||
        his_pdata->weapon_impact == 0) {
        attack_region = his_pdata->attack_region;
    } else {
        attack_region = his_pdata->weapon_impact->attack_region;
    }
    if (attack_region != 0) {
        material = 5;
    }

    if ((flags & 1) == 0) {
        switch (hit_group) {
        case 0:
            sound_group = 2;
            break;
        case 1:
            sound_group = 2;
            break;
        case 2:
            sound_group = 3;
            break;
        case 3:
            sound_group = 4;
            break;
        case 4:
            sound_group = 4;
            break;
        case 6:
            sound_group = 0x14;
            break;
        case 5:
            sound_group = 5;
            break;
        case 7:
            sound_group = 6;
            break;
        case 8:
            sound_group = 7;
            break;
        case 9:
            sound_group = 8;
            break;
        case 12:
            sound_group = 3;
            break;
        case 10:
            sound_group = 4;
            break;
        case 11:
            sound_group = 4;
            break;
        case 13:
            sound_group = 0xD;
            break;
        case 14:
            sound_group = 0xE;
            break;
        case 15:
            sound_group = 0x13;
            break;
        case 16:
            sound_group = 0x16;
            break;
        default:
            sound_group = 2;
            break;
        }
        random_voice(sound_group);
    }

    if ((flags & 2) == 0) {
        if (material == 1 || material == 0) {
            sound_group = 0xD;
        } else {
            switch (hit_type) {
            case 0:
                sound_group = 2;
                if (hit_group == 2) {
                    sound_group = 4;
                } else if (hit_group == 12) {
                    sound_group = 12;
                }
                break;
            case 1:
                sound_group = 3;
                if (hit_group == 12) {
                    sound_group = 12;
                }
                break;
            case 3:
                sound_group = 5;
                break;
            case 2:
                sound_group = 0;
                if (hit_group == 5) {
                    sound_group = 1;
                }
                if (hit_group == 6) {
                    sound_group = 0xD;
                }
                break;
            case 4:
                sound_group = 9;
                break;
            default:
                sound_group = 2;
                break;
            }
        }
        random_hit(sound_group);
    }
}

void cleanup_drone_ai(void) {
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

static float drone_ai_stupid_watcher(void) {
    return 1.0f;
}

static inline void ai_transfer_active(MkProcEntryFn entry) {
    DroneAI* active_drone;
    MkProc* player_proc;

    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    player_proc = get_player_number(plyr_obj) == 0
                      ? (MkProc*)g_game_info.plyr0.idle_proc
                      : (MkProc*)g_game_info.plyr1.idle_proc;
    xfer_proc(player_proc, entry);
    active_drone->request_active = 1;
}

static inline int ai_watcher_can_act(void) {
    if (plyr_pdata->action_lock_a > game_tick_ctr) {
        return 0;
    }
    if (plyr_pdata->action_lock_b > game_tick_ctr) {
        return 0;
    }
    if (plyr_pdata->push_blocked != 0 ||
        (plyr_pdata->state & 0x200) != 0) {
        return 0;
    }
    return 1;
}

static inline int ai_watcher_postround_check(DroneAI* drone) {
    if (plyr_pdata->postround_value == 0.0f) {
        return 0;
    }

    if (drone->difficulty_index > 3) {
        switch (drone->movement_state) {
        case 0:
        case 2:
        case 3:
            if (((drone->reaction_scale <= 0.55f &&
                  drone->player_health < 0.65f) ||
                 drone->player_health < 0.25f) &&
                randu0(100) < 30) {
                drone->charge_cooldown_tick = exec_tick_ctr;
            }
            break;
        }
    }
    return 0;
}

static inline DroneAI* ai_watcher_active_drone(void) {
    return get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
}

static inline int ai_watcher_victim_throw(void) {
    DroneAI* drone = ai_watcher_active_drone();
    int result;

    if (!ai_watcher_can_act()) {
        result = 0;
    } else if (am_i_airborn() == 1) {
        result = 0;
    } else if (his_pdata->state == 0x120C) {
        if (plyr_pdata->opponent_attack_counter_copy ==
            his_pdata->attack_counter) {
            result = 0;
        } else {
            result = 1;
            drone->reaction_watcher = drone_ai_victim_throw_attempt;
        }
    } else {
        result = 0;
    }
    return result;
}

static inline int ai_watcher_victim_dizzy(void) {
    DroneAI* drone = ai_watcher_active_drone();
    int result;

    if (!ai_watcher_can_act()) {
        result = 0;
    } else if (his_pdata->state == 0x4203) {
        result = 1;
        drone->reaction_watcher = drone_ai_victim_dizzy;
    } else {
        result = 0;
    }
    return result;
}

static inline int ai_watcher_victim_duck(void) {
    DroneAI* drone = ai_watcher_active_drone();
    unsigned int likelihood;
    unsigned int elapsed;
    int result = 0;

    if (!ai_watcher_can_act()) {
        result = 0;
    } else if (am_i_airborn() == 1) {
        result = 0;
    } else if ((his_pdata->state & 0x100) == 0) {
        drone->duck_reaction_tick = 0;
        result = 0;
    } else {
        if (drone->duck_reaction_tick == 0) {
            drone->duck_reaction_tick = game_tick_ctr;
        }
        likelihood = g_likelihoodOfDuckCounter[drone->difficulty_index];
        elapsed = game_tick_ctr - drone->duck_reaction_tick;
        if (drone->duck_reaction_tick == 0) {
            elapsed = 0;
        }
        if (elapsed < g_likelihoodOfMinOppDuckingInTicks[
                          drone->difficulty_index]) {
            likelihood = 0;
        } else if (elapsed > g_likelihoodOfMaxOppDuckingInTicks[
                                 drone->difficulty_index]) {
            likelihood = 100;
        }
        if (randu0(100) >= likelihood) {
            result = 0;
        } else if (drone->opponent_distance < 9.733334f) {
            drone->duck_reaction_tick = 0;
            drone->reaction_watcher = drone_ai_victim_ducking;
            result = 1;
        } else {
            result = 0;
        }
    }
    return result;
}

static inline int ai_watcher_victim_avoidance(void) {
    DroneAI* drone = ai_watcher_active_drone();
    int result;

    if (!ai_watcher_can_act()) {
        result = 0;
    } else if (am_i_airborn() == 1) {
        result = 0;
    } else {
        drone->avoidance_area_duration -= 1.0f;
        if (drone->avoidance_area_duration < 0.0f) {
            drone->avoidance_area_duration = 0.0f;
            result = 0;
        } else {
            result = 1;
            drone->reaction_watcher = drone_ai_victim_avoid;
        }
    }
    return result;
}

static inline int ai_watcher_victim_frozen_check(void) {
    DroneAI* drone = ai_watcher_active_drone();
    int state;
    int result;

    if (!ai_watcher_can_act()) {
        result = 0;
    } else {
        state = his_pdata->state;
        if (state == 0xC600 || state == 0x202 ||
            state == 0x421A) {
            result = 1;
            drone->reaction_watcher = drone_ai_victim_frozen;
        } else {
            result = 0;
        }
    }
    return result;
}

static inline int ai_watcher_victim_spear(void) {
    DroneAI* drone = ai_watcher_active_drone();
    int result;

    if (plyr_pdata->character_id != 0) {
        result = 0;
    } else if (!ai_watcher_can_act()) {
        result = 0;
    } else if (am_i_airborn() == 1) {
        result = 0;
    } else if (his_pdata->state == 0x4204) {
        result = 1;
        drone->reaction_watcher = drone_ai_victim_speared;
    } else {
        result = 0;
    }
    return result;
}

static inline int ai_watcher_victim_slip(void) {
    DroneAI* drone = ai_watcher_active_drone();
    int result;

    if (!ai_watcher_can_act()) {
        result = 0;
    } else if (am_i_airborn() == 1) {
        result = 0;
    } else if (his_pdata->state == 0x4207) {
        result = 1;
        drone->reaction_watcher = drone_ai_victim_slipping_on_vomit;
    } else {
        result = 0;
    }
    return result;
}

float drone_ai_watcher(void) {
    DroneAI* drone;
    DroneAI* active_drone;
    MkProc* player_proc;
    int player;
    int command_result;

    player = get_player_number(plyr_obj);
    drone = player == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (plyr_pdata->drone_handoff_pending != 0) {
        plyr_pdata->drone_handoff_pending = 0;
        plyr_pdata->drone_request = 0;
        ai_transfer_active(j_exit);
        AI_TRANSFER(p_plyr_aux2);
        return 0.0f;
    }

    drone->avoidance_area_duration -= 1.0f;
    if (drone->avoidance_area_duration < 0.0f) {
        drone->avoidance_area_duration = 0.0f;
    }
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        active_drone->ai_command = 0;
        active_drone->ai_command_arg = 0;
        active_drone->ai_command_target = -1;
        active_drone->ai_command_value = 0.0f;
        active_drone->ai_command_flag0 = 0;
        active_drone->ai_command_flag1 = 0;
        active_drone->ai_command_flag2 = 0;
        active_drone->command_active = 0;
        return 1.0f;
    }
    if (plyr_pdata->state_flags.bits.bit3 != 0) {
        return 1.0f;
    }
    player_proc = get_player_number(plyr_obj) == 0
                      ? (MkProc*)g_game_info.plyr0.idle_proc
                      : (MkProc*)g_game_info.plyr1.idle_proc;
    if (player_proc->entry == p_idle) {
        return 1.0f;
    }

    drone_ai_watcher_calculate_data();
    if (drone_ai_is_dizzy_watcher() != 0) {
        return 1.0f;
    }
    if (drone->player_health == 0.0f) {
        return 1.0f;
    }
    if (drone->block_subtype == 0x201) {
        drone->special_reaction_ticks--;
        if (drone->special_reaction_ticks == 0) {
            set_my_secondary_state(drone->special_reaction_state);
            drone->special_reaction_active = 0;
        }
    }
    if (plyr_pdata->action_lock_b > game_tick_ctr) {
        return 1.0f;
    }
    if (plyr_pdata->push_blocked != 0 ||
        (plyr_pdata->state & 0x200) != 0) {
        return 1.0f;
    }
    if ((his_pdata->state & 0x1000) == 0) {
        drone->attack_pending = 0;
        drone->attack_latched = 0;
    }
    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    if (active_drone->danger_area_active == 1 &&
        active_drone->danger_area_counter > 0) {
        active_drone->danger_area_counter--;
        if (active_drone->danger_area_counter == 0) {
            active_drone->danger_area_active = 0;
        }
    }

    if ((his_pdata->secondary_state & 0x100) > 0 &&
        !is_big_boss(drone->player)) {
        if (drone_ai_check_for_special_move_reaction(drone) == 1) {
            return 1.0f;
        }
    } else {
        drone->special_reaction_active = 0;
        if (drone_ai_check_for_normal_blocking(drone) == 1) {
            return 1.0f;
        }
    }
    if (drone_ai_reversal_watcher() != 0) {
        return 1.0f;
    }
    if (drone_ai_check_external_request_breakouts(drone) == 1) {
        return 1.0f;
    }
    if (drone_ai_handle_arena_collisions(drone) == 1) {
        return 1.0f;
    }
    if (drone_ai_process_background_states(drone) == 1) {
        return 1.0f;
    }

    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    if (active_drone->ai_command == 0) {
        command_result = 0;
    } else {
        active_drone->ai_command_value -= game_speed;
        if (active_drone->ai_command_value > 0.0f) {
            command_result = 1;
        } else {
            active_drone->ai_command_value = 0.0f;
            command_result = drone_ai_process_scripted_cmd();
        }
    }
    if (command_result != 0) {
        return 1.0f;
    }
    if (!ai_watcher_can_act()) {
        return 1.0f;
    }

    if (drone->decision_ready != 0) {
        if (ai_watcher_victim_throw() != 0) {
            drone->decision_ready = 0;
            drone->request_active = 0;
        }
    }
    if (drone->request_active == 1) {
        return 1.0f;
    }
    if (drone_ai_check_external_requests(drone) == 1) {
        return 1.0f;
    }
    drone->danger_area_ready = 0;
    drone->danger_area_request = 0;
    if (drone_ai_check_evade_arena(drone) == 1) {
        return 1.0f;
    }
    if (drone_ai_check_obstacles(drone) == 1) {
        return 1.0f;
    }
    if (drone->reaction_watcher != 0 &&
        drone->reaction_watcher() == 1) {
        return 1.0f;
    }

    if (drone->decision_ready != 0) {
        if (ai_watcher_victim_dizzy() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (((g_game_info.flags >> 5) & 1) == 0) {
            return 1.0f;
        }

        if (ai_watcher_victim_duck() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }

        if (ai_watcher_victim_avoidance() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }

        if (ai_watcher_victim_frozen_check() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (drone_ai_attacker_reacting_watcher() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (drone_ai_attacker_defenseless_watcher() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (drone_ai_attacker_not_facing_watcher() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (drone_ai_push_watcher(drone) != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (ai_watcher_postround_check(drone) != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (drone_ai_taunt_watcher(drone) != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (drone_ai_charge_up_watcher(drone) != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }
        if (drone_ai_opponent_inair_watcher() != 0) {
            drone->decision_ready = 0;
            return 1.0f;
        }

        switch (plyr_pdata->character_id) {
        case 0:
            if (ai_watcher_victim_spear() != 0) {
                drone->decision_ready = 0;
                return 1.0f;
            }
            /* fall through */
        case 8:
        case 10:
            if (ai_watcher_victim_slip() != 0) {
                drone->decision_ready = 0;
                return 1.0f;
            }
            /* fall through */
        default:
            if (drone_ai_beating_the_snot_out_of_him_watcher() == 1) {
                drone->decision_ready = 0;
                return 1.0f;
            }
            break;
        }
    }

    drone_ai_check_next_AIState(drone);
    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    if (active_drone->movement_state >= 9) {
        drone_ai_initialize(active_drone);
    }
    if (active_drone->big_boss_stage > 1 &&
        active_drone->match_stage == 4 && mode_of_play != 10) {
        active_drone->movement_state = 8;
    }
    if (get_game_state() == 3 &&
        (active_drone->movement_state == 0 ||
         active_drone->movement_state == 8)) {
        active_drone->movement_state = 1;
    }
    if (is_big_boss(active_drone->player)) {
        active_drone->movement_state = 4;
    }
    g_DroneAIJumpTable[active_drone->movement_state]();
    return 1.0f;
}

/* Soft ceiling: exact size, instructions, calls, and typed accesses. The sole
 * diff is null-script branching to a later identical 0.0f return block rather
 * than the earlier invalid-script return block. */
float drone_ai_ducker(void) {
    DroneAI* drone;
    AiFightstyleAttack* script;
    unsigned int attack_index;
    int valid_script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (drone->opponent_distance < 2.8103173f &&
        (drone->player->his_plyr_pdata->state & 0x1000) != 0 &&
        drone->player->his_plyr_pdata->block_requirement == 0 &&
        randu0(100) < 70) {
        if ((drone->player->state & 0x100) != 0 || randu0(100) < 10) {
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, 3, 0);
            if (script != 0) {
                if (script->opcode == 0 &&
                    ((attack_index =
                          script -
                          ((AiFightstyleAttackTable*)
                               plyr_pdata->fighter_definition->move_blend_data)
                              ->attacks) == 12 ||
                     attack_index == 17 || attack_index == 2)) {
                    valid_script = 1;
                } else {
                    valid_script = 0;
                }
                if (valid_script == 0) {
                    return 0.0f;
                }
                drone->script_attack = script;
                drone->script_attack_ready = 2;
                ai_transfer_active(drone_ai_perform_script_attack);
            }
        }
        return 0.0f;
    }
    if (drone_ai_check_change_style(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_ducker_movement(drone) == 1) {
        return 0.0f;
    }
    if (randu0(100) < 20 &&
        drone_ai_check_attack(drone, 0, 0) == 1) {
        return 0.0f;
    }
    if (randu0(1000) < 65) {
        if (drone_ai_check_for_throw(drone) == 1) {
            return 0.0f;
        }
    }
    return 0.0f;
}

static inline int ai_has_ranged_move(unsigned int category) {
    return plyr_pdata->ai_tables->tables[category].usable_row_count != 0;
}

/* Soft ceiling: exact behavior, calls, table widths, and size. Residue is
 * category-2 address constant folding, Boolean-normalization placement,
 * temporary coloring, and merging of identical zero-return tails. */
float drone_ai_mass_attack(void) {
    DroneAI* drone;
    int ranged_move_available;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_should_passive_state_switch(drone);
    if (drone->big_boss_block_state == 1) {
        if (drone_ai_check_for_big_boss_passive_movement(drone) == 1) {
            return 0.0f;
        }

        ranged_move_available = 0;
        if (drone->opponent_distance > 14.6f) {
            ranged_move_available = ai_has_ranged_move(2);
        } else if (drone->opponent_distance > 5.9457946f) {
            ranged_move_available = ai_has_ranged_move(1);
        }
        if (ranged_move_available == 1 && randu0(100) < 2) {
            ai_transfer_active(drone_ai_perform_range_attack);
            return 1.0f;
        }
        return 0.0f;
    }

    if (drone->big_boss_stage != 0) {
        if (drone->big_boss_stage == 4) {
            if (drone_ai_check_attack(drone, 0, 0) == 1) {
                return 0.0f;
            }
        } else if (randu0(100) < 75 &&
                   drone_ai_check_attack(drone, 0, 0) == 1) {
            return 0.0f;
        }
    } else if (randu0(100) < 55 &&
               drone_ai_check_attack(drone, 0, 0) == 1) {
        return 0.0f;
    }

    if (drone->big_boss_stage != 0) {
        if (randu0(100) < 90 &&
            drone_ai_check_for_aggressive_throw(drone) == 1) {
            return 0.0f;
        }
    } else if (randu0(100) < 70 &&
               drone_ai_check_for_aggressive_throw(drone) == 1) {
        return 0.0f;
    }

    ranged_move_available = 0;
    if (drone->opponent_distance > 14.6f) {
        ranged_move_available = ai_has_ranged_move(2);
    } else if (drone->opponent_distance > 5.9457946f) {
        ranged_move_available = ai_has_ranged_move(1);
    }
    if (ranged_move_available == 1 && randu0(100) < 5) {
        ai_transfer_active(drone_ai_perform_range_attack);
        return 1.0f;
    }
    if (drone_ai_check_for_big_boss_aggressive_movement(drone) == 1) {
        return 0.0f;
    }
    return 0.0f;
}

float drone_ai_evade(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_for_evade_attack(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_evade_movement(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_throw(drone) == 1) {
        return 0.0f;
    }
    return 0.0f;
}

float drone_ai_defend(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (randu0(100) < 60) {
        if (drone_ai_check_attack(drone, 0, 0) == 1) {
            return 0.0f;
        }
    } else if (drone_ai_check_push(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_defend_movement(drone) == 1) {
        return 0.0f;
    }
    return 0.0f;
}

float drone_ai_dodge_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_change_style(drone) == 1) {
        return 0.0f;
    }
    if (randu0(8) == 0 &&
        drone_ai_check_attack(drone, 1, 0) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_dodge_movement(drone) == 1) {
        return 0.0f;
    }
    return 0.0f;
}

float drone_ai_berserk(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_for_berserker_movement(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_throw(drone) == 1) {
        return 0.0f;
    }
    if (randu0(100) < 50 && drone_ai_check_push(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_attack(drone, 1, 0) == 1) {
        return 0.0f;
    }
    return 0.0f;
}

float drone_ai_knockdown(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_for_knockdown_movement(drone) == 1) {
        return 0.0f;
    }
    if ((plyr_pdata->his_plyr_pdata->state & 0x100) != 0) {
        return 0.0f;
    }
    if (drone->opponent_distance > 5.9457946f) {
        return 0.0f;
    }
    if (drone->opponent_distance > 2.8103173f &&
        randu0(100) < 90) {
        return 0.0f;
    }
    if (randu0(100) < 50 && drone_ai_check_push(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_throw(drone) == 1) {
        return 0.0f;
    }
    ai_transfer_active(drone_ai_perform_knockdown);
    return 0.0f;
}

float drone_ai_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_change_style(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_extreme_throw(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_attack(drone, 0, 0) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_throw(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_for_aggressive_movement(drone) == 1) {
        return 0.0f;
    }
    if (drone_ai_check_attack(drone, 0, 0) == 1) {
        return 0.0f;
    }
    return 0.0f;
}

float drone_ai_passive(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_for_passive_movement(drone) == 1) {
        return 0.0f;
    }
    return 0.0f;
}

int drone_ai_check_external_requests(DroneAI* request) {
    if (request->danger_area_request == 1 &&
        request->danger_area_ready == 1) {
        ai_transfer_active(drone_ai_avoid_danger_area_now);
        request->danger_area_request = 0;
        return 1;
    }
    if (request->avoid_position_request == 1 &&
        request->avoid_position_ready == 1) {
        ai_transfer_active(drone_ai_avoid_position_now);
        request->avoid_position_request = 0;
        return 1;
    }
    return 0;
}

int drone_ai_check_external_request_breakouts(DroneAI* request) {
    float dx;
    float dz;

    if (request->danger_area_request == 1 &&
        request->danger_area_ready == 0) {
        request->danger_area_ready = 1;
        if (request->request_active == 1) {
            ai_transfer_active(j_exit);
            return 1;
        }
    } else if (request->avoid_position_request == 1 &&
               request->avoid_position_ready == 0) {
        dx = request->avoid_position_target.x - plyr_obj->pos.value.x;
        dz = request->avoid_position_target.z - plyr_obj->pos.value.z;
        if (dx * dx + dz * dz < 1.5f) {
            request->avoid_position_ready = 1;
            if (request->request_active == 1) {
                ai_transfer_active(j_exit);
                return 1;
            }
        }
    } else if (request->avoid_position_ready == 1 &&
               request->request_active == 0) {
        request->avoid_position_ready = 0;
        request->avoid_position_request = 0;
    }
    return 0;
}

int drone_ai_check_block_at_reactions(void) {
    DroneAI* drone;
    PlyrPdata* player;
    unsigned int action_lock_b;
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
    if (player->reaction_counter != his_pdata->attack_counter) {
        drone->block_request = 1;
        drone->block_subtype = 0;
        drone->attack_pending = 1;
        drone->request_active = 1;
        return 1;
    }
    return 0;
}

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
        return drone_ai_im_dizzy();
    }
    return 0;
}

int drone_ai_reversal_watcher(void) {
    DroneAI* drone;

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

    ai_transfer_active(j_exit);
    return 1;
}

/* Soft ceiling: exact size, CFG, cached action lock, and materialized
 * probability result. Residue is only r3/r4 coloring for pdata/tick loads. */
int drone_ai_opponent_inair_watcher(void) {
    DroneAI* drone;
    PlyrPdata* action;
    unsigned int action_lock_b;
    unsigned short likelihood;
    int can_attack;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    action = plyr_pdata;
    action_lock_b = action->action_lock_b;
    if (action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else {
        if (action->action_lock_a > game_tick_ctr) {
            can_attack = 0;
        } else if (action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else if (action->push_blocked != 0 ||
                   (action->state & 0x200) != 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
        if (can_attack == 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
    }
    if (can_attack == 0) {
        return 0;
    }
    if (action->action_lock_a > game_tick_ctr) {
        can_attack = 0;
    } else if (action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else if (action->push_blocked != 0 ||
               (action->state & 0x200) != 0) {
        can_attack = 0;
    } else {
        can_attack = 1;
    }
    if (can_attack == 0) {
        return 0;
    }
    if (am_i_airborn() == 1) {
        return 0;
    }
    if (is_he_airborn() == 0) {
        return 0;
    }
    if (his_obj->pos.value.y < 1.0668f) {
        return 0;
    }

    likelihood = g_likelihoodOfInAirAttack[drone->difficulty_index];
    if (drone->big_boss_stage == 0) {
        likelihood = 5;
    }
    can_attack = randu0(100) < likelihood;
    if (can_attack != 0 && drone_ai_enemy_inair_attack(drone) != 0) {
        return 1;
    }
    return 0;
}

int drone_ai_enemy_inair_attack(DroneAI* drone) {
    void* script;

    drone->script_attack_ready = 0;
    script = 0;
    if (drone->opponent_distance > 14.6f) {
        return 0;
    }
    if (drone->opponent_distance > 5.9457946f) {
        if (((AiAirMoveStatus*)plyr_pdata->status_flags)->distant_count > 0) {
            DroneAI* active;
            AiAirMoveStatus* selection_moves;

            active = get_player_number(plyr_obj) == 0
                         ? &g_DroneAI1 : &g_DroneAI2;
            selection_moves =
                (AiAirMoveStatus*)plyr_pdata->status_flags;
            if (selection_moves->distant_count == 0) {
                script = 0;
            } else {
                active->ai_command = selection_moves->distant_commands +
                    randu0((unsigned short)selection_moves->distant_count) * 16;
                active->ai_command_arg = 0;
                active->ai_command_target = active->player->character_id;
                active->ai_command_flag0 = 0;
                active->ai_command_flag1 = 0;
                active->ai_command_flag2 = 0;
                script = get_special_move();
            }
            drone->script_attack_ready = 1;
        }
    } else if (drone->opponent_distance > 2.8103173f) {
        if (((AiAirMoveStatus*)plyr_pdata->status_flags)->distant_count == 0 ||
            randu0(2) == 0) {
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, 12, 0);
            if (script == 0) {
                return 0;
            }
        } else {
            DroneAI* active;
            AiAirMoveStatus* selection_moves;

            active = get_player_number(plyr_obj) == 0
                         ? &g_DroneAI1 : &g_DroneAI2;
            selection_moves =
                (AiAirMoveStatus*)plyr_pdata->status_flags;
            if (selection_moves->distant_count == 0) {
                script = 0;
            } else {
                active->ai_command = selection_moves->distant_commands +
                    randu0((unsigned short)selection_moves->distant_count) * 16;
                active->ai_command_arg = 0;
                active->ai_command_target = active->player->character_id;
                active->ai_command_flag0 = 0;
                active->ai_command_flag1 = 0;
                active->ai_command_flag2 = 0;
                script = get_special_move();
            }
            drone->script_attack_ready = 1;
        }
    } else {
        if (((AiAirMoveStatus*)plyr_pdata->status_flags)->close_count == 0 ||
            randu0(2) == 0) {
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, 12, 1);
            if (script == 0) {
                script = get_random_fightstyle_attack(
                    plyr_pdata->fighter_definition, 10, 1);
                if (script == 0) {
                    return 0;
                }
            }
        } else {
            DroneAI* active;
            AiAirMoveStatus* selection_moves;

            active = get_player_number(plyr_obj) == 0
                         ? &g_DroneAI1 : &g_DroneAI2;
            selection_moves = (AiAirMoveStatus*)plyr_pdata->status_flags;
            if (selection_moves->close_count == 0) {
                script = 0;
            } else {
                active->ai_command = selection_moves->close_commands +
                    randu0((unsigned short)selection_moves->close_count) * 16;
                active->ai_command_arg = 0;
                active->ai_command_target = active->player->character_id;
                active->ai_command_flag0 = 0;
                active->ai_command_flag1 = 0;
                active->ai_command_flag2 = 0;
                script = get_special_move();
            }
            drone->script_attack_ready = 1;
        }
    }
    if (script != 0) {
        drone->script_attack = script;
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    return 0;
}

static inline int ai_throw_restrictions_allow(void) {
    int allowed = 1;
    PlyrPdata* opponent;
    int state;

    if ((g_DroneOverrideInfo.flags & 0x40) != 0) {
        allowed = 0;
    } else if (is_he_airborn() != 0) {
        allowed = 0;
    } else {
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
    }
    return allowed;
}

/* Soft ceiling: exact size, CFG, operations, calls, and memory accesses.
 * Residue is a whole-lifetime r29/r30 swap for the guard and tick limit. */
int drone_ai_check_for_throw(DroneAI* drone) {
    unsigned int minimum_ticks;
    int can_throw;
    void* script;
    int is_script;

    if (drone->opponent_distance > 3.1252584f) {
        return 0;
    }
    minimum_ticks = g_minBlockHeldTime[drone->difficulty_index];
    can_throw = ai_throw_restrictions_allow();
    if (can_throw == 0) {
        can_throw = 0;
    } else if (drone->big_boss_stage == 0) {
        can_throw = 0;
    } else if (randu0(100) < 70) {
        can_throw = 0;
    } else if (drone->block_hold_ticks > minimum_ticks) {
        can_throw = 1;
    } else if (randu0(100) <
               (unsigned int)g_likelihoodToThrow[drone->difficulty_index]) {
        can_throw = 1;
    } else {
        can_throw = 0;
    }
    if (can_throw != 0) {
        script = drone_ai_choose_move_from_category(11, 75, &is_script);
        if (script == 0) {
            return 0;
        }
        drone->script_attack = script;
        if (is_script != 0) {
            drone->script_attack_ready = 1;
        } else {
            drone->script_attack_ready = 2;
        }
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    return 0;
}

static inline int ai_start_throw(DroneAI* drone) {
    void* script;
    int is_script;

    script = drone_ai_choose_move_from_category(11, 75, &is_script);
    if (script == 0) {
        return 0;
    }
    drone->script_attack = script;
    if (is_script != 0) {
        drone->script_attack_ready = 1;
    } else {
        drone->script_attack_ready = 2;
    }
    ai_transfer_active(drone_ai_perform_script_attack);
    return 1;
}

/* Soft ceiling: restrictions, calls, stores, and CFG agree. Residue is a
 * consistent r29/r30 lifetime swap and one retail-preserved constant one. */
int drone_ai_check_for_extreme_throw(DroneAI* drone) {
    unsigned int minimum_ticks;
    int can_throw;

    if (drone->opponent_distance > 3.1252584f) {
        return 0;
    }
    minimum_ticks = g_minExtremeBlockHeldTime[drone->difficulty_index];
    can_throw = ai_throw_restrictions_allow();
    if (can_throw == 0) {
        can_throw = 0;
    } else if (drone->block_hold_ticks > minimum_ticks) {
        can_throw = 1;
    } else {
        can_throw = 0;
    }
    if (can_throw != 0) {
        return ai_start_throw(drone);
    }
    return 0;
}

/* Soft ceiling: exact size, CFG, operations, calls, and memory accesses.
 * The remaining 14 objdiff rows are register operands in the fallback special
 * move path: MWCC swaps the command-drone and move-list lifetimes and colors
 * their arithmetic temporaries differently. */
int drone_ai_check_for_aggressive_throw(DroneAI* drone) {
    DroneAI* command_drone;
    AiExtendedSpecialMoveList* moves;
    unsigned int min_hold_ticks;
    int throw_count;
    void* script;
    int can_throw;
    int opponent_state;
    int should_throw;

    if (drone->opponent_distance > 4.2958374f) {
        return 0;
    }
    if (randu0(100) < 70) {
        return 0;
    }
    min_hold_ticks = g_minBlockHeldTime[drone->difficulty_index];
    if (drone->big_boss_stage == 0) {
        should_throw = 0;
    } else {
        can_throw = 1;
        if ((g_DroneOverrideInfo.flags & 0x40) != 0) {
            can_throw = 0;
        } else if (plyr_pdata->action_lock_b > game_tick_ctr) {
            can_throw = 0;
        } else if (is_he_airborn() != 0) {
            can_throw = 0;
        } else {
            opponent_state = his_pdata->state;
            if (opponent_state == 0x101) {
                can_throw = 0;
            }
            if (opponent_state == 0x302) {
                can_throw = 0;
            }
            if (opponent_state == 0x900) {
                can_throw = 0;
            }
            if (opponent_state == 0x901) {
                can_throw = 0;
            }
            if (his_pdata->throw_restriction == 3) {
                can_throw = 0;
            }
        }
        if (can_throw == 0) {
            should_throw = 0;
        } else if (randu0(100) < 80 &&
                   drone->block_hold_ticks > min_hold_ticks) {
            should_throw = 1;
        } else if (randu0(100) <
                   g_likelihoodToAggressiveThrow[drone->difficulty_index]) {
            should_throw = 1;
        } else {
            should_throw = 0;
        }
    }
    if (should_throw != 0) {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 11, 0);
        if (script == 0) {
            command_drone = get_player_number(plyr_obj) == 0
                                ? &g_DroneAI1 : &g_DroneAI2;
            moves = (AiExtendedSpecialMoveList*)plyr_pdata->status_flags;
            throw_count = moves->throw_count;
            if (throw_count == 0) {
                script = 0;
            } else {
                command_drone->ai_command =
                    moves->throw_commands +
                    randu0((unsigned short)throw_count) * 16;
                command_drone->ai_command_arg = 0;
                command_drone->ai_command_target =
                    command_drone->player->character_id;
                command_drone->ai_command_flag0 = 0;
                command_drone->ai_command_flag1 = 0;
                command_drone->ai_command_flag2 = 0;
                script = get_special_move();
            }
            if (script == 0) {
                return 0;
            }
            drone->script_attack_ready = 1;
        }
        drone->script_attack = script;
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    return 0;
}

/* Soft ceiling: exact size, CFG, operations, and cached lock semantics.
 * Residue is confined to r3/r6 coloring of the lock and tick operands. */
int drone_ai_attacker_reacting_watcher(void) {
    DroneAI* drone;
    PlyrPdata* action;
    unsigned int likelihood;
    int can_attack;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x600) {
        return 0;
    }
    if (drone->movement_state == 1) {
        action = plyr_pdata;
        if (action->action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else {
            if (action->action_lock_a > game_tick_ctr) {
                can_attack = 0;
            } else if (action->action_lock_b > game_tick_ctr) {
                can_attack = 0;
            } else if (action->push_blocked != 0 ||
                       (action->state & 0x200) != 0) {
                can_attack = 0;
            } else {
                can_attack = 1;
            }
            if (can_attack == 0) {
                can_attack = 0;
            } else {
                can_attack = 1;
            }
        }
        if (can_attack == 1) {
            likelihood =
                g_likelihoodOfReactAttack[drone->difficulty_index];
            if (drone->big_boss_stage > 2) {
                likelihood += 5;
            }
            if (drone->match_stage == 0) {
                likelihood = 0;
            }
            can_attack = randu0(100) < likelihood;
            if (can_attack == 1) {
                ai_transfer_active(drone_ai_special_attack_now);
            }
        }
        return 1;
    }
    return 0;
}

int drone_ai_check_for_evade_attack(DroneAI* drone) {
    int has_clearance;

    if (drone->opponent_distance < 5.9457946f) {
        if (ai_backward_clearance() > 2.1336f) {
            has_clearance = 1;
        } else {
            has_clearance = 0;
        }
        if (has_clearance == 1) {
            if (randu0(3) == 0) {
                if (drone_ai_check_for_side_step_counter_attack(drone) == 1) {
                    return 1;
                }
            } else if (drone_ai_should_evade_attack(drone) == 1) {
                drone->attack_type = 0;
                ai_transfer_active(drone_ai_perform_attack);
                return 1;
            }
        }
    } else if (randu0(2) != 0 &&
               drone_ai_check_attack(drone, 0, 0) == 1) {
        return 1;
    }
    return 0;
}

int drone_ai_check_for_side_step_counter_attack(DroneAI* drone) {
    DroneAI* active_drone;
    MkProc* player_proc;
    Vec facing;
    Vec to_opponent;
    int likelihood;
    int should_counter;

    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    player_proc = get_player_number(plyr_obj) == 0
                      ? (MkProc*)g_game_info.plyr0.idle_proc
                      : (MkProc*)g_game_info.plyr1.idle_proc;
    xfer_proc(player_proc, side_step_to_center_attack_with_jexit);
    active_drone->request_active = 1;

    if ((g_DroneOverrideInfo.flags & 0x20) != 0) {
        should_counter = 0;
    } else {
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
        if (randu0(100) < (unsigned int)likelihood) {
            should_counter = 1;
        } else {
            should_counter = 0;
        }
    }

    if (should_counter == 1) {
        uv_to_opponent(&to_opponent);
        uv_from_angle_y(&facing, his_obj->ang.y);
        if (to_opponent.x * facing.x +
                to_opponent.z * facing.z > -0.86f) {
            drone->jump_attack_pending = 1;
        }
    }
    return 1;
}

int drone_ai_check_for_big_boss_aggressive_movement(DroneAI* drone) {
    drone->movement_attempt = 0;
    if (drone->opponent_distance > 14.6f) {
        if (randu0(6) == 0) {
            if (drone_ai_check_attack(drone, 1, 0) == 1) {
                return 1;
            }
        } else if (randu0(8) == 0) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 5.9457946f) {
        if (randu0(4) == 0) {
            if (drone_ai_check_attack(drone, 1, 0) == 1) {
                return 1;
            }
        } else {
            if (randu0(50) == 0) {
                ai_transfer_active(step_forward_with_jexit);
                return 1;
            }
            if (randu0(50) == 0) {
                ai_transfer_active(side_step_to_center_with_jexit);
                return 1;
            }
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

int drone_ai_check_for_aggressive_movement(DroneAI* drone) {
    int has_clearance;
    int has_special_moves;
    int retry;

    retry = drone->movement_attempt;
    drone->movement_attempt = 0;
    if (drone->opponent_distance > 28.451557f) {
        if (drone->opponent_distance > 14.6f) {
            drone->charge_cooldown_tick += randu0(25) + 5;
        } else if (drone->opponent_distance > 5.9457946f) {
            drone->charge_cooldown_tick += randu0(3);
        }
        if (((AiSpecialMoveList*)plyr_pdata->status_flags)->count > 0) {
            has_special_moves = 1;
        } else {
            has_special_moves = 0;
        }
        if (has_special_moves == 1 && randu0(10) == 0) {
            ai_transfer_active(catch_opponent);
            return 1;
        }
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
        if (randu0(30) == 0) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
        if (randu0(60) == 0 || retry == 1) {
            if (drone->match_stage == 0 &&
                drone->difficulty_index < 3) {
                return 0;
            }
            ai_transfer_active(side_step_to_center_with_jexit);
            return 1;
        }
        /* Retail retains this second retry path after the side-step test. */
        if (retry == 1) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
        }
    } else {
        if (ai_backward_clearance() > 2.1336f) {
            has_clearance = 1;
        } else {
            has_clearance = 0;
        }
        if (has_clearance == 1 && randu0(22) == 0) {
            if (drone->difficulty_index > 5 && randu0(100) < 50) {
                ai_transfer_active(dash_back_with_jexit);
                drone->jump_attack_pending = 1;
            } else {
                ai_transfer_active(step_backward_with_jexit);
            }
            return 1;
        }
    }
    if (drone->match_stage == 0 && drone->difficulty_index < 3) {
        return 0;
    }
    if ((his_pdata->state & 0x400) != 0 && randu0(60) == 0) {
        ai_transfer_active(side_step_to_center_with_jexit);
        return 1;
    }
    return 0;
}

/* Soft ceiling: exact size, instructions, traversal, stale-node cleanup,
 * calls, widths, stack Vec slots, and NaN-sensitive inverse-length CFG.
 * All remaining differences are equivalent iterator and FPR coloring. */
static int drone_ai_check_obstacles(DroneAI* request) {
    AiFloatBits input;
    AiFloatBits estimate;
    MkPtr* obstacle_item;
    MkPtr* shape_item;
    MkPtr* next;
    ArenaObstacle* obstacle;
    CollisionObj* shape;
    DroneAI* drone;
    MkProc* player_proc;
    Vec to_opponent;
    Vec center;
    float delta_z;
    float delta_x;
    float squared_distance;
    float normalization_squared;
    float inverse_length;
    float estimate_product;
    float correction;

    if (&constrain_info != 0) {
        obstacle_item = constrain_info.obstacles;
        while (obstacle_item != 0) {
            obstacle = (ArenaObstacle*)obstacle_item->hdr;
            if (obstacle_item->instance != obstacle->hdr.instance) {
                next = obstacle_item->next;
                obstacle_item->hdr = 0;
                destroy_mkptr(obstacle_item);
                obstacle_item = next;
                continue;
            }
            if (!obstacle->flags.bits.disabled && &obstacle->shapes != 0) {
                shape_item = obstacle->shapes;
                while (shape_item != 0) {
                    shape = (CollisionObj*)shape_item->hdr;
                    if (shape_item->instance != shape->hdr.instance) {
                        next = shape_item->next;
                        shape_item->hdr = 0;
                        destroy_mkptr(shape_item);
                        shape_item = next;
                        continue;
                    }
                    delta_z = 0.0f;
                    squared_distance = 10000.0f;
                    delta_x = 0.0f;
                    if (get_shape_center_for_collision_obstacle(
                            shape, &center) != 0) {
                        delta_z = center.z - plyr_obj->pos.value.z;
                        delta_x = center.x - plyr_obj->pos.value.x;
                        squared_distance =
                            delta_x * delta_x + delta_z * delta_z;
                    }
                    if (squared_distance <= 5.9457946f) {
                        uv_to_opponent(&to_opponent);
                        normalization_squared =
                            delta_x * delta_x + delta_z * delta_z;
                        if (normalization_squared <= 0.0f) {
                            inverse_length = 0.0f;
                        } else {
                            input.f = normalization_squared;
                            estimate.u = 0x5F375A00U - (input.u >> 1);
                            estimate_product =
                                estimate.f *
                                (normalization_squared * estimate.f);
                            correction = 3.0f - estimate_product;
                            inverse_length =
                                0.0625f * estimate.f * correction *
                                -(correction *
                                      (estimate_product * correction) -
                                  12.0f);
                        }
                        delta_z *= inverse_length;
                        delta_x *= inverse_length;
                        if (to_opponent.x * delta_x +
                                    to_opponent.z * delta_z >
                                0.86f &&
                            request->opponent_distance > squared_distance) {
                            if (obstacle->type == 5) {
                                request->obstacle_target.x = center.x;
                                request->obstacle_target.z = center.z;
                                drone = get_player_number(plyr_obj) == 0
                                            ? &g_DroneAI1
                                            : &g_DroneAI2;
                                if (get_player_number(plyr_obj) == 0) {
                                    player_proc = g_game_info.plyr0.idle_proc;
                                } else {
                                    player_proc = g_game_info.plyr1.idle_proc;
                                }
                                xfer_proc(
                                    player_proc,
                                    drone_ai_attack_obstacle_now);
                                drone->request_active = 1;
                                return 1;
                            }
                            if (obstacle->type != 3) {
                                drone = get_player_number(plyr_obj) == 0
                                            ? &g_DroneAI1
                                            : &g_DroneAI2;
                                if (get_player_number(plyr_obj) == 0) {
                                    player_proc = g_game_info.plyr0.idle_proc;
                                } else {
                                    player_proc = g_game_info.plyr1.idle_proc;
                                }
                                xfer_proc(
                                    player_proc,
                                    side_step_to_center_with_jexit);
                                drone->request_active = 1;
                                return 1;
                            }
                        }
                    }
                    shape_item = shape_item->next;
                }
            }
            obstacle_item = obstacle_item->next;
        }
    }
    return 0;
}

int drone_ai_handle_arena_collisions(DroneAI* request) {
    if (request->evade_arena_state == 0) {
        switch (request->movement_state) {
        case 0:
        case 1:
        case 2:
        case 5:
            if ((request->arena_collision_history & 0x80000000U) != 0) {
                --request->arena_collision_count;
            }
            request->arena_collision_history <<= 1;
            if ((((unsigned int)plyr_obj->flags_0B >> 4) & 1) == 1 ||
                ((((unsigned int)plyr_obj->flags_09 >> 2) & 1) == 1 &&
                 (plyr_pdata->state & 0x2000) != 0)) {
                ++request->arena_collision_count;
                request->arena_collision_history |= 1;
            }
            if (request->arena_collision_count > 24) {
                request->arena_collision_count = 0;
                request->arena_collision_history = 0;
                request->evade_arena_state = 1;
                if ((plyr_pdata->state & 0x2000) != 0) {
                    ai_transfer_active(j_exit);
                    return 1;
                }
                return 0;
            }
            break;
        default:
            request->arena_collision_count = 0;
            request->arena_collision_history = 0;
            break;
        }
    }
    return 0;
}

int drone_ai_check_evade_arena(DroneAI* request) {
    if (request->evade_arena_state == 1) {
        ai_transfer_active(side_step_to_center_with_jexit);
        request->evade_arena_state = 2;
        return 1;
    }
    if (request->evade_arena_state == 2) {
        request->evade_arena_state = 0;
    }
    return 0;
}

#pragma dont_inline on
int drone_ai_check_for_knockdown_movement(DroneAI* drone) {
    return drone_ai_check_for_berserker_movement(drone);
}
#pragma dont_inline reset

int drone_ai_check_for_ducker_movement(DroneAI* drone) {
    AiSpecialMoveList* moves;
    void* script;
    int has_special_moves;
    int is_script;

    drone->movement_attempt = 0;
    if (drone->opponent_distance > 16.218693f &&
        randu0(100) < 33) {
        script = drone_ai_choose_move_from_category(2, 50, &is_script);
        if (script == 0) {
            return 0;
        }
        drone->script_attack = script;
        if (is_script != 0) {
            drone->script_attack_ready = 1;
        } else {
            drone->script_attack_ready = 2;
        }
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    if (drone->opponent_distance > 5.9457946f) {
        if (drone->opponent_distance > 14.6f) {
            drone->charge_cooldown_tick += randu0(25) + 5;
        } else if (drone->opponent_distance > 5.9457946f) {
            drone->charge_cooldown_tick += randu0(3);
        }
        moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
        if (moves->count > 0) {
            has_special_moves = 1;
        } else {
            has_special_moves = 0;
        }
        if (has_special_moves == 1 && randu0(10) == 0) {
            ai_transfer_active(catch_opponent);
            return 1;
        } else {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
    }
    if (drone->opponent_distance > 5.729021f) {
        if (randu0(100) < 20) {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
            return 1;
        } else {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
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

int drone_ai_check_for_berserker_movement(DroneAI* drone) {
    int has_special_moves;

    drone->movement_attempt = 0;
    if (drone->opponent_distance > 5.9457946f) {
        if (drone->opponent_distance > 14.6f) {
            drone->charge_cooldown_tick += randu0(25) + 5;
        } else if (drone->opponent_distance > 5.9457946f) {
            drone->charge_cooldown_tick += randu0(3);
        }
        if (((AiSpecialMoveList*)plyr_pdata->status_flags)->count > 0) {
            has_special_moves = 1;
        } else {
            has_special_moves = 0;
        }
        if (has_special_moves == 1 && randu0(10) == 0) {
            ai_transfer_active(catch_opponent);
            return 1;
        } else {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
    }
    if (drone->opponent_distance > 2.8103173f) {
        if (randu0(100) < 20) {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
            return 1;
        } else {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
    }
    if (randu0(100) < 20) {
        ai_transfer_active(side_step_to_center_attack_with_jexit);
        return 1;
    }
    return 0;
}

int drone_ai_check_for_dodge_movement(DroneAI* request) {
    DroneAI* drone;
    MkProc* player_proc;

    if (request->opponent_distance > 5.9457946f) {
        drone = get_player_number(plyr_obj) == 0
                    ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? (MkProc*)g_game_info.plyr0.idle_proc
                          : (MkProc*)g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, walk_forward_attackdist_with_jexit);
        drone->request_active = 1;
        return 1;
    }

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    player_proc = get_player_number(plyr_obj) == 0
                      ? (MkProc*)g_game_info.plyr0.idle_proc
                      : (MkProc*)g_game_info.plyr1.idle_proc;
    xfer_proc(player_proc, side_step_to_center_attack_with_jexit);
    drone->request_active = 1;
    return 1;
}

int drone_ai_check_for_big_boss_passive_movement(DroneAI* drone) {
    int has_clearance;

    drone->movement_attempt = 0;
    if (drone->opponent_distance < 5.225796f) {
        if (randu0(100) < 90) {
            return 0;
        }
        if (ai_backward_clearance() > 2.1336f) {
            has_clearance = 1;
        } else {
            has_clearance = 0;
        }
        if (has_clearance == 1) {
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

int drone_ai_check_for_passive_movement(DroneAI* drone) {
    int has_clearance;

    drone->movement_attempt = 0;
    if (randu0(100) < 60) {
        return 0;
    }
    if (drone->opponent_distance < 5.225796f) {
        if (randu0(16) != 0) {
            return 0;
        }
        if (ai_backward_clearance() > 2.1336f) {
            has_clearance = 1;
        } else {
            has_clearance = 0;
        }
        if (has_clearance == 1) {
            if (randu0(2) != 0) {
                ai_transfer_active(drone_walk_backwards_with_jexit);
                return 1;
            }
            if (randu0(3) == 0) {
                if (randu0(2) == 0) {
                    ai_transfer_active(jump_away_opponent_with_jexit);
                    return 1;
                }
                ai_transfer_active(dash_back_with_jexit);
                return 1;
            }
        } else {
            ai_transfer_active(side_step_to_center_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 28.451557f) {
        if (randu0(4) != 0) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        } else {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
    } else {
        if (randu0(14) == 0) {
            return 0;
        }
        if (randu0(15) == 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
        if (ai_backward_clearance() > 2.1336f) {
            has_clearance = 1;
        } else {
            has_clearance = 0;
        }
        if (has_clearance == 1 && randu0(30) == 0) {
            if (randu0(100) < 75) {
                ai_transfer_active(step_backward_with_jexit);
                return 1;
            }
            ai_transfer_active(dash_back_with_jexit);
            return 1;
        }
    }
    return 0;
}

int drone_ai_check_for_defend_movement(DroneAI* drone) {
    int has_clearance;
    int has_special_moves;

    drone->movement_attempt = 0;
    if (drone->opponent_distance < 5.9457946f) {
        if (randu0(4) == 0) {
            return 0;
        }
        if (ai_backward_clearance() > 2.1336f) {
            has_clearance = 1;
        } else {
            has_clearance = 0;
        }
        if (has_clearance == 1) {
            if (randu0(10) == 0) {
                ai_transfer_active(drone_walk_backwards_with_jexit);
                if (randu0(2) == 0) {
                    drone->jump_attack_pending = 1;
                }
                return 1;
            }
            if (randu0(25) == 0) {
                ai_transfer_active(jump_away_opponent_with_jexit);
                return 1;
            }
        } else {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 28.451557f) {
        if (((AiSpecialMoveList*)plyr_pdata->status_flags)->count > 0) {
            has_special_moves = 1;
        } else {
            has_special_moves = 0;
        }
        if (has_special_moves == 1 && randu0(25) == 0) {
            ai_transfer_active(catch_opponent);
            return 1;
        }
        if ((randu0(10) != 0 && drone->difficulty_index < 6) ||
            randu0(50) == 0) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        } else {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 14.6f) {
        if (randu0(12) == 0) {
            return 0;
        }
        ai_transfer_active(step_forward_with_jexit);
        return 1;
    } else {
        if (randu0(50) == 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
        if (randu0(200) == 0) {
            if (drone->match_stage == 0 &&
                drone->difficulty_index < 3) {
                return 0;
            }
            ai_transfer_active(side_step_to_center_long_with_jexit);
            return 1;
        }
        if (randu0(700) == 0) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        }
        if (randu0(500) == 0) {
            if (ai_backward_clearance() > 2.1336f) {
                has_clearance = 1;
            } else {
                has_clearance = 0;
            }
            if (has_clearance == 1) {
                ai_transfer_active(jump_away_opponent_with_jexit);
                return 1;
            }
        }
    }
    return 0;
}

int drone_ai_check_for_evade_movement(DroneAI* drone) {
    int has_clearance;

    drone->movement_attempt = 0;
    if (drone->opponent_distance < 5.9457946f) {
        if (randu0(4) == 0) {
            return 0;
        }
        if (ai_backward_clearance() > 2.1336f) {
            has_clearance = 1;
        } else {
            has_clearance = 0;
        }
        if (has_clearance == 1) {
            if (randu0(10) == 0) {
                ai_transfer_active(drone_walk_backwards_with_jexit);
                return 1;
            }
            if (randu0(4) == 0) {
                ai_transfer_active(side_step_to_center_attack_with_jexit);
                return 1;
            }
            if (randu0(10) == 0) {
                ai_transfer_active(jump_away_opponent_with_jexit);
                return 1;
            }
        } else {
            ai_transfer_active(side_step_to_center_attack_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 28.451557f) {
        if (drone->opponent_distance > 14.6f) {
            drone->charge_cooldown_tick += randu0(25) + 5;
        } else if (drone->opponent_distance > 5.9457946f) {
            drone->charge_cooldown_tick += randu0(3);
        }
        if (randu0(10) != 0) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        }
        ai_transfer_active(step_forward_with_jexit);
        return 1;
    } else if (drone->opponent_distance > 14.6f) {
        if (randu0(12) == 0) {
            return 0;
        }
        ai_transfer_active(step_forward_with_jexit);
        return 1;
    } else {
        if (randu0(50) == 0) {
            ai_transfer_active(step_forward_with_jexit);
            return 1;
        }
        if (randu0(200) == 0) {
            ai_transfer_active(side_step_to_center_long_with_jexit);
            return 1;
        }
        if (randu0(700) == 0) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        }
        if (randu0(500) == 0) {
            if (ai_backward_clearance() > 2.1336f) {
                has_clearance = 1;
            } else {
                has_clearance = 0;
            }
            if (has_clearance == 1) {
                ai_transfer_active(jump_away_opponent_with_jexit);
                return 1;
            }
        }
    }
    return 0;
}

int drone_ai_check_for_special_move_reaction(DroneAI* drone) {
    int opponent_state;
    int result;

    result = 0;
    if (drone->attack_pending == 1) {
        return 0;
    }
    if (drone->special_reaction_active == 1) {
        return 0;
    }
    if (drone_ai_should_be_blocking(drone, 1) == 0) {
        return 0;
    }
    opponent_state = his_pdata->secondary_state;
    if (opponent_state == 0x101) {
        result = drone_ai_check_projectile_head_on(drone);
    } else if (opponent_state == 0x102) {
        result = drone_ai_check_projectile_side(drone);
    } else if (opponent_state == 0x513) {
        result = drone_ai_check_propel_attack(drone);
    } else if (opponent_state == 0x103) {
        result = drone_ai_check_dont_touch_attack_phase1(drone);
    } else if (opponent_state == 0x10A) {
        result = drone_ai_check_dont_touch_attack_phase2(drone);
    } else if (opponent_state == 0x10B) {
        result = 1;
    } else if (opponent_state == 0x104) {
        result = 0;
    } else if (opponent_state == 0x105 &&
               (unsigned int)drone->block_subtype != 0x107U) {
        result = drone_ai_check_from_ground_attack_phase1(drone);
    } else if (opponent_state == 0x106 &&
               (unsigned int)drone->block_subtype == 0x107U) {
        result = drone_ai_check_from_ground_attack_phase2(drone);
    } else if (opponent_state == 0x109) {
        result = drone_ai_check_all_over_ground(drone);
    } else {
        if (plyr_pdata->secondary_state == 0x109) {
            result = drone_ai_check_all_over_ground(drone);
        } else if (opponent_state == 0x108) {
            result = drone_ai_check_all_over_ground_phase1(drone, 10);
        } else if (opponent_state == 0x10F) {
            result = drone_ai_check_all_over_ground_phase1(drone, 5);
        } else if (opponent_state == 0x10C) {
            result = drone_ai_check_cant_dodge_attack(drone);
        } else if (opponent_state == 0x10D) {
            result = drone_ai_check_cant_dodge_attack2(drone);
        } else if (opponent_state == 0x10E) {
            result = drone_ai_check_avoid_danger_area(drone);
        } else if (plyr_pdata->secondary_state == 0x110) {
            result = drone_ai_check_attack_from_above(drone);
        } else if (opponent_state == 0x111) {
            drone->special_reaction_ticks = randu0(10) + 5;
            drone->special_reaction_state = 0x110;
            drone->block_subtype = 0x201;
            drone->special_reaction_active = 1;
        } else if (opponent_state == 0x112) {
            result = drone_ai_check_mid_high_spinner(drone);
        }
    }
    return result;
}

int drone_ai_check_all_over_ground(DroneAI* drone) {
    unsigned short roll;

    drone->special_reaction_active = 1;
    drone->attack_pending = 1;
    if (drone->opponent_distance < 2.8103173f) {
        if (randu0(100) < 50) {
            ai_transfer_active(drone_ai_counter_attack_now);
            return 1;
        } else {
            ai_transfer_active(jump_away_opponent_with_j_exit);
            return 1;
        }
    }
    /* Retail retains both sides of this non-NaN exhaustive test. */
    if (drone->opponent_distance >= 2.8103173f ||
        drone->opponent_distance < 5.9457946f) {
        if (randu0(100) < 50) {
            ai_transfer_active(jump_towards_opponent_with_j_exit);
            return 1;
        } else {
            ai_transfer_active(jump_away_opponent_with_j_exit);
            return 1;
        }
    }

    if (drone->opponent_distance >= 5.9457946f) {
        roll = randu0(100);
        if (roll < 50 ||
            (roll < 75 && drone->difficulty_index < 3)) {
            ai_transfer_active(jump_away_opponent_with_j_exit);
            return 1;
        } else {
            ai_transfer_active(jump_towards_opponent_with_j_exit);
            return 1;
        }
    }
    return 0;
}

static inline int ai_count_taunt_moves(void) {
    AiStyleSlot* style;
    int count;

    if (!fight_style_restriction_table.taunt_allowed()) {
        return 0;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[0];
    count = style->moves != 0 ? style->moves->taunt_count : 0;
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[1];
    count += style->moves != 0 ? style->moves->taunt_count : 0;
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[2];
    count += style->moves != 0 ? style->moves->taunt_count : 0;
    return count;
}

/* Soft ceiling: retail/current state stores, taunt count, transfers, and CFG
 * agree. Residue is scheduling of two constant-one results and one branch. */
int drone_ai_check_avoid_danger_area(DroneAI* drone) {
    drone->special_reaction_active = 1;
    drone->danger_area_active = 0;
    drone->danger_area_counter = 0;
    if (randu0(100) < 5 || drone->difficulty_index < 2) {
        return 0;
    }

    drone->danger_area_active = 1;
    if (drone->opponent_distance < 2.8103173f) {
        ai_transfer_active(drone_ai_counter_attack_now);
        return 1;
    }
    if (drone->opponent_distance < 14.6f) {
        drone->reversal_pending = 1;
        ai_transfer_active(side_step_to_center_with_jexit);
        return 1;
    }
    if (ai_count_taunt_moves() > 0 && randu0(100) < 75) {
        ai_transfer_active(drone_ai_perform_taunt);
    } else {
        ai_transfer_active(side_step_to_center_long_with_jexit);
    }
    return 1;
}

/*
 * Retail repeats this decision in ten callers and branches to explicit 1/0
 * results for the final likelihood test. The helper emits no ELF symbol.
 */
static inline int ai_should_block_super_move(DroneAI* drone) {
    unsigned int likelihood;

    if ((g_DroneOverrideInfo.flags & 8) != 0) {
        return 0;
    }
    if (g_DroneOverrideInfo.likelihood_scale == 0.0f) {
        return 0;
    }
    if (drone->big_boss_stage < 4 &&
        drone->block_retry_tick > game_tick_ctr) {
        return 0;
    }
    if (his_pdata->repeated_action_count >
        (int)(randu0(3) + 2)) {
        return 1;
    }

    likelihood =
        g_likelihoodForSuperMoveBlocking[drone->difficulty_index];
    if (drone->movement_state != 1) {
        likelihood += 5;
    }
    if (drone->match_stage < 2 && drone->difficulty_index < 3) {
        likelihood = 1;
    }
    likelihood = (unsigned int)(
        (float)likelihood * g_DroneOverrideInfo.likelihood_scale);
    if (randu0(100) < likelihood) {
        return 1;
    }
    return 0;
}

static inline void ai_update_block_retry(DroneAI* drone) {
    if (drone->player->his_plyr_pdata->field_234 == 0) {
        return;
    }
    if (drone->big_boss_stage == 4) {
        drone->block_retry_tick = 0;
    } else if (drone->difficulty_index > 5 && randu0(100) < 70) {
        drone->block_retry_tick = 0;
    } else if (drone->difficulty_index > 2 && randu0(100) < 50) {
        drone->block_retry_tick = 0;
    } else {
        drone->block_retry_tick = 800;
        drone->block_retry_tick += game_tick_ctr;
    }
}

int drone_ai_check_attack_from_above(DroneAI* drone) {
    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    ai_transfer_active(drone_ai_perform_block);
    return 1;
}

int drone_ai_check_mid_high_spinner(DroneAI* drone) {
    if (drone->opponent_distance > 5.225796f) {
        return 0;
    }
    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    ai_transfer_active(drone_ai_perform_block);
    return 1;
}

int drone_ai_check_all_over_ground_phase1(
    DroneAI* drone, int delay) {
    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    if (drone->opponent_distance < 5.9457946f &&
        (drone->difficulty_index > 5 || randu0(100) < 20)) {
        ai_transfer_active(drone_ai_counter_attack_now);
        return 1;
    }
    drone->special_reaction_ticks = delay;
    drone->special_reaction_state = 0x109;
    drone->block_subtype = 0x201;
    drone->special_reaction_active = 1;
    drone->attack_pending = 0;
    return 0;
}

static inline int ai_not_facing(void) {
    Vec to_opponent;
    Vec facing;
    float dot;

    uv_to_opponent(&to_opponent);
    uv_from_angle_y(&facing, his_obj->ang.y);
    dot = to_opponent.x * facing.x + to_opponent.z * facing.z;
    if (dot > -0.86f) {
        return 1;
    }
    return 0;
}

int drone_ai_check_cant_dodge_attack(DroneAI* drone) {
    if (drone->opponent_distance > 9.0f || ai_not_facing() == 1) {
        return 0;
    }
    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    ai_transfer_active(drone_ai_perform_block);
    return 1;
}

int drone_ai_check_cant_dodge_attack2(DroneAI* drone) {
    if (drone->opponent_distance > 12.0f) {
        return 0;
    }
    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    ai_transfer_active(drone_ai_perform_block);
    return 1;
}

int drone_ai_check_propel_attack(DroneAI* drone) {
    if (drone->opponent_distance > 12.0f) {
        return 0;
    }
    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    if (randu0(100) < 85) {
        ai_transfer_active(drone_ai_perform_block);
    } else {
        ai_transfer_active(drone_ai_dodge_3d_with_counter);
    }
    return 1;
}

int drone_ai_check_projectile_side(DroneAI* drone) {
    float distance;

    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    distance = drone->opponent_distance;
    if (distance < 3.3445094f) {
        ai_transfer_active(drone_ai_perform_block);
        return 1;
    }
    if (distance > 23.783178f) {
        if (randu0(100) < 50) {
            ai_transfer_active(drone_ai_perform_taunt);
        } else {
            ai_transfer_active(drone_ai_perform_charge_up);
        }
        return 1;
    }
    drone->jump_attack_pending = 1;
    ai_transfer_active(walk_forward_attackdist_with_jexit);
    return 1;
}

int drone_ai_check_from_ground_attack_phase2(DroneAI* drone) {
    unsigned int roll;

    if (drone->opponent_distance < 2.8103173f &&
        randu0(100) < 75) {
        ai_transfer_active(drone_ai_dodge_3d_with_counter);
        drone->block_subtype = 0;
        return 1;
    }
    if (drone->opponent_distance >= 3.3445094f) {
        roll = randu0(100);
        if (roll < 50 ||
            (roll < 75 && drone->difficulty_index < 3)) {
            ai_transfer_active(jump_towards_opponent_with_j_exit);
            drone->block_subtype = 0;
            return 1;
        }
        drone_ai_perform_jump_attack(drone);
        drone->block_subtype = 0;
        return 1;
    }
    drone->block_subtype = 0;
    return 0;
}

int drone_ai_check_from_ground_attack_phase1(DroneAI* drone) {
    drone->special_reaction_active = 1;
    if (!ai_should_block_super_move(drone)) {
        return 0;
    }
    ai_update_block_retry(drone);
    drone->attack_pending = 1;
    if (drone->opponent_distance < 5.9457946f &&
        (drone->difficulty_index > 5 || randu0(100) < 20)) {
        ai_transfer_active(drone_ai_counter_attack_now);
        return 1;
    }
    drone->block_subtype = 0x107;
    drone->special_reaction_active = 0;
    drone->attack_pending = 0;
    return 0;
}

/* Soft ceiling: 99.8996%; only the x/z distance temporary FPRs differ. */
int drone_ai_check_projectile_head_on(DroneAI* drone) {
    int duck_reaction_active;
    unsigned int roll;
    int should_block;
    float projectile_distance;
    float delta_z;
    float delta_x;

    duck_reaction_active = his_pdata->duck_reaction_active;
    if (duck_reaction_active == 0) {
        projectile_distance = 10000.0f;
    } else {
        delta_z = plyr_obj->pos.value.z - his_pdata->saved_position_z;
        delta_x = plyr_obj->pos.value.x - his_pdata->saved_position_x;
        projectile_distance = delta_x * delta_x + delta_z * delta_z;
    }

    if (projectile_distance < 9.0f) {
        roll = randu0(100);
        should_block = 0;
        drone->special_reaction_active = 1;
        if (!ai_should_block_super_move(drone)) {
            return 0;
        }
        ai_update_block_retry(drone);
        drone->attack_pending = 1;
        if (drone->movement_state == 3) {
            roll += 10;
        }
        if (roll < 50 || drone->opponent_distance < 5.9457946f) {
            should_block = 1;
        }
        if (ai_not_facing() == 1 &&
            (drone->difficulty_index > 5 || randu0(100) < 20)) {
            ai_transfer_active(drone_ai_counter_attack_now);
            return 1;
        }
        if (should_block == 1) {
            ai_transfer_active(drone_ai_perform_block);
            return 1;
        }
        if (randu0(100) < 40) {
            ai_transfer_active(drone_ai_duck_attack);
        } else {
            ai_transfer_active(drone_ai_dodge_3d_with_counter);
        }
        return 1;
    }

    if (duck_reaction_active == 0 &&
        drone->opponent_distance < 5.9457946f) {
        drone->special_reaction_active = 1;
        if (!ai_should_block_super_move(drone)) {
            return 0;
        }
        ai_update_block_retry(drone);
        drone->attack_pending = 1;
        if (randu0(100) < 25) {
            ai_transfer_active(drone_ai_counter_attack_now);
            return 1;
        }
        if (randu0(100) < 25) {
            ai_transfer_active(drone_ai_duck_attack);
            return 1;
        }
        ai_transfer_active(drone_ai_perform_block);
        return 1;
    }
    return 0;
}

static inline void ai_close_dont_touch_attack(DroneAI* drone) {
    if (drone->difficulty_index < 6) {
        ai_transfer_active(drone_ai_perform_low_attack);
    } else {
        ai_transfer_active(drone_ai_perform_weapon_attack);
    }
    his_pdata->secondary_state = 0x10B;
}

/* Soft ceiling: four-byte condition-code lowering residue (extrwi./mfcr in
 * retail versus cror currently); algorithm, calls, and typed accesses match. */
int drone_ai_check_dont_touch_attack_phase1(DroneAI* drone) {
    float clearance;

    drone->special_reaction_active = 1;
    if (ai_should_block_super_move(drone) == 0) {
        return 0;
    }

    drone->special_reaction_active = 0;
    drone->attack_pending = 0;
    his_pdata->secondary_state = 0x10A;
    ai_update_block_retry(drone);

    if (drone->opponent_distance < 5.9457946f) {
        clearance = ai_backward_clearance();
        if (clearance >= 2.1336f || randu0(100) < 5) {
            ai_close_dont_touch_attack(drone);
            return 1;
        }
        if (drone->opponent_distance < 1.1380622522339916) {
            ai_close_dont_touch_attack(drone);
            return 1;
        }
        ai_transfer_active(dash_back_with_jexit);
        return 1;
    }
    return drone_ai_check_dont_touch_attack_phase2(drone);
}

static inline int ai_count_charge_moves(void) {
    AiStyleSlot* style;
    int count;

    if (!fight_style_restriction_table.charge_allowed()) {
        return 0;
    }

    style = (AiStyleSlot*)plyr_pdata->weapon_styles[0];
    count = style->moves != 0 ? style->moves->charge_count : 0;
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[1];
    count += style->moves != 0 ? style->moves->charge_count : 0;
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[2];
    count += style->moves != 0 ? style->moves->charge_count : 0;
    return count;
}

static inline int ai_move_category_count(
    PlyrPdata* player, int category) {
    return player->ai_tables->tables[category].usable_row_count;
}

static inline int ai_range_move_count(const DroneAI* drone) {
    int count;

    count = 0;
    if (drone->opponent_distance > 14.6f) {
        count = ai_move_category_count(plyr_pdata, 2);
    } else if (drone->opponent_distance > 5.9457946f) {
        count = ai_move_category_count(plyr_pdata, 1);
    }
    return count;
}

/* Soft ceiling: algorithm, CFG, callers, typed counts, and final Boolean
 * lifetime match. The sole residue is two equivalent category-2 address
 * expansions: retail materializes 2 and indexes from +0xBC; current MWCC
 * folds each to +0xCC. */
int drone_ai_check_dont_touch_attack_phase2(DroneAI* drone) {
    unsigned short roll;
    int taunt_count;
    int charge_count;
    int range_count;
    int has_range_moves;

    taunt_count = ai_count_taunt_moves();
    charge_count = ai_count_charge_moves();
    roll = randu0(100);
    his_pdata->secondary_state = 0x10B;

    if ((int)roll < 10) {
        if (drone->difficulty_index < 3 || (int)roll < 20) {
            return 1;
        }
    }

    if ((int)roll < 65 && charge_count + taunt_count > 0) {
        if ((charge_count > 0 && (int)roll < 25) || taunt_count == 0) {
            ai_transfer_active(drone_ai_perform_charge_up);
            return 1;
        }
        if (taunt_count > 0) {
            ai_transfer_active(drone_ai_perform_taunt);
            return 1;
        }
    }

    range_count = ai_range_move_count(drone);
    if ((range_count == 0 || randu0(100) < 20) &&
        drone->opponent_distance < 5.225796f) {
        ai_transfer_active(drone_ai_perform_weapon_attack);
        return 1;
    }
    if (randu0(100) < 10) {
        return 1;
    }
    range_count = ai_range_move_count(drone);
    if (range_count != 0) {
        has_range_moves = 1;
    } else {
        has_range_moves = 0;
    }
    if (has_range_moves == 1) {
        ai_transfer_active(drone_ai_perform_range_attack);
        return 1;
    }
    return 1;
}

int drone_ai_charge_up_watcher(DroneAI* drone) {
    if (randu0(100) < 20 &&
        (his_pdata->state & 0x1000) == 0) {
        if (ai_count_charge_moves() > 0 &&
            drone->opponent_distance > 14.6f &&
            randu0(20) < 100 &&
            drone->difficulty_index > 2) {
            ai_transfer_active(drone_ai_perform_charge_up);
            return 1;
        }
    }
    return drone_ai_charge_up_watcher_defense(drone);
}

int drone_ai_charge_up_watcher_defense(DroneAI* drone) {
    if (his_pdata->state != 0x4209) {
        return 0;
    }
    if (drone->difficulty_index > 2 || randu0(100) < 50) {
        his_pdata->state = 0x4206;
        if (ai_count_taunt_moves() > 0 &&
            drone->opponent_distance > 13.378037f &&
            drone->reaction_scale >= 0.65f &&
            randu0(100) < 90) {
            ai_transfer_active(drone_ai_perform_taunt);
            return 1;
        }
        if (ai_count_charge_moves() > 0 &&
            randu0(100) < 30 &&
            drone->opponent_distance > 5.9457946f) {
            ai_transfer_active(drone_ai_perform_charge_up);
            return 1;
        }
    }
    return 0;
}

int drone_ai_taunt_watcher(DroneAI* drone) {
    if (randu0(100) < 20 &&
        (his_pdata->state & 0x1000) == 0 &&
        ai_count_taunt_moves() > 0) {
        if (drone->opponent_distance > 26.84898f &&
            is_he_airborn() &&
            drone->difficulty_index < 6) {
            ai_transfer_active(drone_ai_perform_taunt);
            return 1;
        }
        if (drone->opponent_distance > 23.783178f &&
            drone->reaction_scale >= 0.65f &&
            drone->difficulty_index < 8 &&
            plyr_pdata != 0) {
            ai_transfer_active(drone_ai_perform_taunt);
            return 1;
        }
    }
    return drone_ai_taunt_watcher_defense(drone);
}

int drone_ai_push_watcher(DroneAI* drone) {
    unsigned int minimum_ticks;

    minimum_ticks = g_minBlockHiHeldTime[drone->difficulty_index];
    if (drone_ai_can_push(drone) == 1 &&
        drone->opponent_distance < 4.378056f) {
        if (drone->block_hold_ticks > minimum_ticks &&
            (his_pdata->state & 0x100) == 0 &&
            randu0(100) < 60) {
            ai_transfer_active(drone_ai_perform_push);
            return 1;
        }
        if (drone->reaction_scale >= 0.65f &&
            drone->difficulty_index < 8 &&
            drone->push_attempts > 0 &&
            randu0(100) < 15) {
            --drone->push_attempts;
            if (drone->push_attempts < 0) {
                drone->push_attempts = 0;
            }
            ai_transfer_active(drone_ai_perform_push);
            return 1;
        }
    }
    return 0;
}

int drone_ai_taunt_watcher_defense(DroneAI* drone) {
    if (his_pdata->state != 0x420A) {
        return 0;
    }
    if (drone->difficulty_index > 2 || randu0(100) < 50) {
        if (ai_count_charge_moves() > 0 &&
            randu0(100) < 75 &&
            drone->opponent_distance > 2.8103173f) {
            ai_transfer_active(drone_ai_perform_charge_up);
            return 1;
        }
        his_pdata->state = 0x4206;
    }
    return 0;
}

static inline int ai_find_reversal_style(void) {
    AiStyleSlot* style;
    int count;

    if (!fight_style_restriction_table.reversal_allowed()) {
        return -1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[0];
    count = style->moves != 0 ? style->moves->reversal_count : 0;
    if (count != 0) {
        return 0;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[1];
    count = style->moves != 0 ? style->moves->reversal_count : 0;
    if (count != 0) {
        return 1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[2];
    count = style->moves != 0 ? style->moves->reversal_count : 0;
    if (count != 0) {
        return 2;
    }
    return -1;
}

int drone_ai_check_for_normal_blocking(DroneAI* drone) {
    PlyrMoveBlendData* move_data;
    unsigned int reversal_likelihood;
    unsigned int roll;
    int reversal_count;
    int do_reversal;
    int hit_strength;

    if (drone->attack_pending == 1) {
        return 0;
    }
    if (his_pdata->state == 0x120B || his_pdata->state == 0x120C) {
        return 0;
    }
    if (drone_ai_should_be_blocking(drone, 1) == 0) {
        drone->block_request = 1;
        drone->block_subtype = 0;
        return 0;
    }

    if (drone->block_request == 1) {
        drone->block_request = 0;
        reversal_likelihood =
            g_likelihoodForSuperDefense[drone->difficulty_index];
        if (drone->movement_state != 1) {
            reversal_likelihood += 5;
        }
        if (drone->match_stage == 0) {
            reversal_likelihood = 1;
        }
        if ((g_DroneOverrideInfo.flags & 8) != 0) {
            reversal_likelihood = 0;
        } else {
            reversal_likelihood = (unsigned int)(
                (float)reversal_likelihood *
                g_DroneOverrideInfo.likelihood_scale);
        }
        do_reversal =
            (unsigned short)randu0(100) < reversal_likelihood;
        if (do_reversal == 1) {
            move_data =
                plyr_pdata->fighter_definition->move_blend_data;
            reversal_count = move_data != 0
                                 ? move_data->ai_tables[5]
                                       .usable_row_count
                                 : 0;
            if (randu0(100) < 20 ||
                (drone->difficulty_index > 4 && randu0(100) < 60)) {
                if (ai_find_reversal_style() >= 0) {
                    reversal_count = 1;
                }
            }
            if (reversal_count > 0) {
                drone->attack_pending = 1;
                drone->block_request = 0;
            }
            if (reversal_count > 0) {
                ai_transfer_active(drone_ai_perform_reversal);
                return 1;
            }
        }
    }

    if (drone_ai_should_be_blocking(drone, 0) == 1) {
    roll = (unsigned short)randu0(100);
    drone->attack_pending = 1;
    if (drone->player->his_plyr_pdata->field_234 != 0) {
        if (drone->big_boss_stage == 4) {
            drone->block_retry_tick = 0;
        } else if (drone->difficulty_index > 5 && randu0(100) < 70) {
            drone->block_retry_tick = 0;
        } else if (drone->difficulty_index > 2 && randu0(100) < 50) {
            drone->block_retry_tick = 0;
        } else {
            drone->block_retry_tick = 800;
            drone->block_retry_tick += game_tick_ctr;
        }
    }

    if (his_pdata->state == 0x1219) {
        if (drone->opponent_distance < 2.8103173f &&
            drone->difficulty_index > 3 && randu0(100) < 50) {
            ai_transfer_active(drone_ai_dodge_3d_with_counter);
        } else {
            ai_transfer_active(drone_ai_perform_block);
        }
        return 1;
    }

    if (drone->movement_state == 8 &&
        (drone->player->state & 0x100) != 0) {
        hit_strength = his_pdata->pending_hit_strength;
        if (hit_strength == 1 || hit_strength == 8 ||
            hit_strength == 9 || hit_strength == 5) {
            if (randu0(100) < 80) {
                plyr_pdata->field_728 = 2;
                return 1;
            }
        } else if (hit_strength == 4 && randu0(100) < 60) {
            plyr_pdata->field_728 = 3;
            return 1;
        }
        return 0;
    }

    hit_strength = his_pdata->pending_hit_strength;
    if (hit_strength == 1) {
        if (roll < 15 && drone->difficulty_index < 6 &&
            !is_big_boss(drone->player)) {
            if ((his_pdata->state & 0x100) > 0 &&
                drone->opponent_distance < 4.0f) {
                ai_transfer_active(jump_towards_opponent_with_attack);
                return 1;
            }
            if (ai_backward_clearance() > 1.524f) {
                ai_transfer_active(jump_away_opponent_with_jexit);
                return 1;
            }
        }
        if (drone->movement_state == 3) {
            roll += 10;
        }
        if (roll < 85 || drone->difficulty_index < 2) {
            ai_transfer_active(drone_ai_perform_block);
        } else {
            ai_transfer_active(drone_ai_dodge_3d_with_counter);
        }
        return 1;
    }
    if (hit_strength == 2) {
        ai_transfer_active(drone_ai_dodge_3d_with_counter);
        return 1;
    }
    if (hit_strength == 6) {
        ai_transfer_active(drone_ai_perform_block);
        return 1;
    }
    if (hit_strength == 8) {
        ai_transfer_active(drone_ai_perform_block);
        return 1;
    }
    if (hit_strength == 7) {
        ai_transfer_active(drone_ai_perform_block);
        return 1;
    }
    if (drone->movement_state == 3) {
        roll += 10;
    }
    if (roll < 20 && hit_strength == 0) {
        ai_transfer_active(drone_ai_duck_attack);
        return 1;
    }
    if (roll < 50 && hit_strength == 5) {
        ai_transfer_active(drone_ai_duck_attack);
        return 1;
    }
    if (roll < 75 || drone->difficulty_index < 2) {
        ai_transfer_active(drone_ai_perform_block);
    } else {
        ai_transfer_active(drone_ai_dodge_3d_with_counter);
    }
        return 1;
    }
    return 0;
}

/* Soft ceiling: exact size, CFG, calls, cached action lock, and facing-result
 * materialization. Residue is only r3/r4 coloring for pdata/tick loads. */
int drone_ai_attacker_not_facing_watcher(void) {
    DroneAI* drone;
    PlyrPdata* player;
    unsigned int action_lock_b;
    int likelihood;
    int can_attack;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    player = plyr_pdata;
    action_lock_b = player->action_lock_b;
    if (action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else {
        if (player->action_lock_a > game_tick_ctr) {
            can_attack = 0;
        } else if (action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else if (player->push_blocked != 0 ||
                   (player->state & 0x200) != 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
        if (can_attack == 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
    }
    if (can_attack == 0) {
        return 0;
    }
    if (player->action_lock_a > game_tick_ctr) {
        can_attack = 0;
    } else if (action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else if (player->push_blocked != 0 ||
               (player->state & 0x200) != 0) {
        can_attack = 0;
    } else {
        can_attack = 1;
    }
    if (can_attack == 0) {
        return 0;
    }
    if (am_i_airborn() == 1) {
        return 0;
    }
    can_attack = ai_not_facing();
    if (can_attack == 0) {
        return 0;
    }
    if ((g_DroneOverrideInfo.flags & 0x20) != 0) {
        return 0;
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
        if (drone_ai_can_push(drone) == 1 &&
            drone->opponent_distance < 4.378056f &&
            randu0(500) < 5 &&
            drone->reaction_scale > 0.05f) {
            ai_transfer_active(drone_ai_perform_push);
            return 1;
        }
        return 0;
    }
    if (drone_ai_can_push(drone) == 1 &&
        drone->opponent_distance < 4.378056f &&
        randu0(100) < 3 &&
        drone->reaction_scale > 0.05f) {
        ai_transfer_active(drone_ai_perform_push);
        return 1;
    }
    if ((plyr_pdata->his_plyr_pdata->state & 0x100) != 0) {
        return 0;
    }
    return drone_ai_attacker_defenseless(drone);
}

/* Soft ceiling: exact size, opcodes, state tests, and availability logic.
 * Remaining differences are GPR allocation and load scheduling. */
int drone_ai_attacker_defenseless_watcher(void) {
    DroneAI* drone;
    PlyrPdata* action;
    unsigned int action_lock_b;
    int can_attack;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4206 &&
        (unsigned int)his_pdata->state != 0xFFFFC601U) {
        return 0;
    }
    action = plyr_pdata;
    action_lock_b = action->action_lock_b;
    if (action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else {
        if (action->action_lock_a > game_tick_ctr) {
            can_attack = 0;
        } else if (action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else if (action->push_blocked != 0 ||
                   (action->state & 0x200) != 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
        if (can_attack == 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
    }
    if (can_attack == 0) {
        return 0;
    }
    if (action->action_lock_a > game_tick_ctr) {
        can_attack = 0;
    } else if (action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else if (action->push_blocked != 0 ||
               (action->state & 0x200) != 0) {
        can_attack = 0;
    } else {
        can_attack = 1;
    }
    if (can_attack == 0) {
        return 0;
    }
    if (am_i_airborn() == 1) {
        return 0;
    }
    return drone_ai_attacker_defenseless(drone);
}

/* Soft ceiling: exact size, CFG, operations, and cached lock semantics.
 * Residue is confined to r3/r5 coloring of the lock and tick operands. */
int drone_ai_beating_the_snot_out_of_him_watcher(void) {
    DroneAI* drone;
    PlyrPdata* player;
    unsigned int likelihood;
    int can_act;
    int can_retreat;
    int result;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    result = 0;
    if (his_pdata->hit_streak < 3) {
        return 0;
    }
    if (drone->super_combo_active == 1 ||
        drone->command_active == 1) {
        return 0;
    }
    player = plyr_pdata;
    if (player->action_lock_b > game_tick_ctr) {
        can_act = 0;
    } else {
        if (player->action_lock_a > game_tick_ctr) {
            can_act = 0;
        } else if (player->action_lock_b > game_tick_ctr) {
            can_act = 0;
        } else if (player->push_blocked != 0 ||
                   (player->state & 0x200) != 0) {
            can_act = 0;
        } else {
            can_act = 1;
        }
        if (can_act == 0) {
            can_act = 0;
        } else {
            can_act = 1;
        }
    }
    if (can_act == 0 && am_i_airborn() != 0) {
        return 0;
    }
    if (randu0(100) < 75 && drone->difficulty_index < 5) {
        result = 1;
        his_pdata->hit_streak = 0;
        drone->movement_state = 0;
        drone->charge_cooldown_tick = exec_tick_ctr + 120;
    }
    if (drone->difficulty_index == 8) {
        return 0;
    }
    if (drone->opponent_distance > 9.290304f) {
        return 0;
    }
    if (ai_backward_clearance() > 2.1336f)
        can_retreat = 1;
    else
        can_retreat = 0;
    if (can_retreat == 1) {
        likelihood = (his_pdata->hit_streak - 3) * 10 + 30;
        if (drone->big_boss_stage == 4) {
            likelihood -= 20;
        }
        if (randu0(100) < likelihood) {
            ai_transfer_active(dash_back_with_jexit);
            result = 1;
            his_pdata->hit_streak = 0;
        } else if (randu0(100) < 75) {
            his_pdata->hit_streak = 0;
        }
    }
    return result;
}

static inline AiFightstyleAttack* ai_pick_status_special_move(void) {
    DroneAI* active;
    AiSpecialMoveList* moves;

    active = get_player_number(plyr_obj) == 0
                 ? &g_DroneAI1 : &g_DroneAI2;
    moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
    if (moves->count == 0) {
        return 0;
    }
    active->ai_command =
        moves->commands + randu0((unsigned short)moves->count) * 16;
    active->ai_command_arg = 0;
    active->ai_command_target = active->player->character_id;
    active->ai_command_flag0 = 0;
    active->ai_command_flag1 = 0;
    active->ai_command_flag2 = 0;
    return get_special_move();
}

/* Soft ceiling: retail call/branch structure and table-selection diamonds agree.
 * The residue is one move-data reload plus nonvolatile GPR coloring. */
int drone_ai_attacker_defenseless(DroneAI* drone) {
    void* category_move;
    void* script;
    unsigned int category;
    unsigned int roll;
    int is_script;
    int knockdown_count;
    int distant_count;
    int special_count;
    int close_count;

    knockdown_count =
        plyr_pdata->fighter_definition->move_blend_data != 0
            ? plyr_pdata->fighter_definition->move_blend_data->ai_tables[9]
                                .usable_row_count
            : 0;
    distant_count =
        plyr_pdata->fighter_definition->move_blend_data != 0
            ? plyr_pdata->fighter_definition->move_blend_data->ai_tables[10]
                              .usable_row_count
            : 0;
    category_move =
        drone_ai_choose_move_from_category(11, 75, &is_script);
    script = 0;
    roll = (unsigned short)randu0(100);
    drone->script_attack_ready = 0;

    if (drone->opponent_distance > 14.6f) {
        special_count =
            ((AiSpecialMoveList*)plyr_pdata->status_flags)->count;
        if (special_count > 0 &&
            (roll < 10 ||
             (drone->difficulty_index > 5 && roll < 20))) {
            script = ai_pick_status_special_move();
            drone->script_attack_ready = 1;
        } else if (is_big_boss(drone->player) != 0) {
            if (special_count > 0) {
                script = ai_pick_status_special_move();
                drone->script_attack_ready = 1;
            }
        } else {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        }
    } else if (drone->opponent_distance > 5.9457946f) {
        special_count =
            ((AiSpecialMoveList*)plyr_pdata->status_flags)->count;
        if (special_count > 0 &&
            (roll < 10 ||
             (drone->difficulty_index > 5 && roll < 20))) {
            script = ai_pick_status_special_move();
            drone->script_attack_ready = 1;
        } else if ((close_count =
                        plyr_pdata->fighter_definition->move_blend_data != 0
                            ? plyr_pdata->fighter_definition->move_blend_data
                                  ->ai_tables[1]
                                      .usable_row_count
                            : 0) > 0 &&
                   roll < 10) {
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, 1, 1);
            if (script == 0) {
                return 0;
            }
            drone->script_attack_ready = 2;
        } else if (!(xz_distance_between_players() < 1.4864486f)) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        }
    } else if (roll < 60 ||
               (drone->difficulty_index >= 5 && roll < 90)) {
        category = 14;
        if (distant_count > 0) {
            category = 10;
        }
        if (category == 14 && knockdown_count > 0) {
            category = 9;
        }
        if (knockdown_count > 0 && randu0(100) < 30) {
            category = 9;
        }
        if (category == 14 && category_move != 0) {
            category = 11;
        }
        if (category_move != 0 && randu0(100) < 30 &&
            drone->opponent_distance < 4.378056f) {
            category = 11;
        }

        if (drone->difficulty_index >= 4 && randu0(100) < 40 &&
            drone->opponent_distance < 4.378056f &&
            his_pdata->state != 0x4203) {
            PlyrFighterDefinition* combo_fighter;
            void* combo_script;

            combo_fighter = plyr_pdata->fighter_definition;
            distant_count = combo_fighter->move_blend_data != 0
                                ? combo_fighter->move_blend_data->ai_tables[10]
                                      .usable_row_count
                                : 0;
            close_count = combo_fighter->move_blend_data != 0
                              ? combo_fighter->move_blend_data->ai_tables[0]
                                    .usable_row_count
                              : 0;
            if ((his_pdata->state & 0x100) != 0) {
                combo_script = get_random_fightstyle_attack(
                    combo_fighter, 3, 1);
            } else if ((unsigned int)close_count > 6U &&
                       xz_distance_between_players() < 7.1883736f &&
                       randu0(100) < 30) {
                combo_script = get_random_fightstyle_attack(
                    combo_fighter, 0, 1);
            } else if ((unsigned int)distant_count > 6U &&
                       randu0(100) < 30) {
                combo_script = get_random_fightstyle_attack(
                    combo_fighter, 10, 1);
            } else {
                combo_script = get_random_fightstyle_attack(
                    combo_fighter, 1, 1);
            }
            drone->command_active = 1;
            script = combo_script;
            if (script == 0) {
                return 0;
            }
        } else if (category == 11) {
            script = category_move;
            if (script == 0) {
                return 0;
            }
            if (is_script != 0) {
                drone->script_attack_ready = 1;
            } else {
                drone->script_attack_ready = 2;
            }
        } else if (category != 14) {
            if (category == 10 && randu0(100) < 50 &&
                drone->difficulty_index > 4) {
                drone->jump_attack_pending = 1;
            }
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, category, 1);
            if (script == 0) {
                return 0;
            }
        }
    }

    if (script != 0) {
        drone->script_attack = script;
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    return 0;
}

void drone_ai_watcher_calculate_data(void) {
    DroneAI* drone;
    int player;
    int ladder_position;

    player = get_player_number(plyr_obj);
    drone = player == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->match_mode = g_game_info.pselect.field_1f4;

    if (get_player_number(plyr_obj) == 1) {
        drone->reaction_scale =
            g_game_info.plyr1.field_0C - g_game_info.plyr0.field_0C;
        drone->handicap_match_stage = g_game_info.plyr1.field_40;
        drone->opponent_health = g_game_info.plyr0.field_0C;
        drone->player_health = g_game_info.plyr1.field_0C;
    } else {
        drone->reaction_scale =
            g_game_info.plyr0.field_0C - g_game_info.plyr1.field_0C;
        drone->handicap_match_stage = g_game_info.plyr0.field_40;
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

    drone->match_stage = ladder_position;
    drone->player = plyr_pdata;
    drone->big_boss_stage = game_settings.kombat_difficulty;
    drone->consecutive_losses = g_GameLossesInARow;
    drone->opponent_round_attacks = his_pdata->round_attack_count;

    if (drone->big_boss_stage > 2) {
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
    if (drone->opponent_distance > 14.6f) {
        drone->opponent_out_of_range = 1;
    } else if (drone->opponent_distance > 5.9457946f) {
        drone->opponent_out_of_range = 1;
    }

    if (exec_tick_ctr - drone->difficulty_update_tick > 60) {
        drone->difficulty_update_tick = exec_tick_ctr;
        drone->difficulty_index =
            handicap_get_current_difficulty(drone);
        if (get_game_state() == 3) {
            drone->difficulty_index = 8;
        }
        if (mode_of_play == 10) {
            drone->difficulty_index =
                mk_chess_get_current_difficulty_for_ai(
                    drone->player->plyr_num);
            drone->big_boss_stage =
                game_settings.arcade_difficulty;
            if (drone->big_boss_stage < 2) {
                drone->match_stage = 2;
            } else if (drone->big_boss_stage == 2) {
                drone->match_stage = 4;
            } else if (drone->big_boss_stage == 3) {
                drone->match_stage = 6;
            } else {
                drone->match_stage = 8;
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

/* Soft ceiling: exact behavior, calls, and opcode set except one extra zero
 * materialization; retail keeps the guard branch but shares the final li. */
int drone_ai_process_background_states(DroneAI* request) {
    float distance;

    if (request->background_attack_active != 0) {
        return 0;
    }
    distance = 1.5f;
    if (randu0(100) < 50) {
        distance = 2.5f;
    }
    if (request->opponent_distance < distance * distance &&
        am_i_airborn() != 0) {
        request->background_attack_active = 1;
        if (am_i_a_big_character() != 0) {
            ai_transfer_active(j_flying_kick);
        } else {
            ai_transfer_active(j_flying_kick2);
        }
        return 1;
    }
    return 0;
}

static inline void* ai_pick_special_move(unsigned int category) {
    DroneAI* drone;
    FighterAiTable* table;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    table = &plyr_pdata->ai_tables->tables[category];
    if (table->usable_row_count == 0) {
        return 0;
    }
    drone->ai_command = (unsigned int*)&table->rows[
        randu0((unsigned short)table->usable_row_count)];
    drone->ai_command_arg = 0;
    drone->ai_command_target = drone->player->character_id;
    drone->ai_command_flag0 = 0;
    drone->ai_command_flag1 = 0;
    drone->ai_command_flag2 = 0;
    return get_special_move();
}

static inline unsigned int ai_table_row_count(
    FighterAiTable* tables, unsigned int category) {
    return tables[category].usable_row_count;
}

static inline unsigned int ai_move_table_row_count(
    PlyrMoveBlendData* move_data, unsigned int category) {
    if (move_data == 0) {
        return 0;
    }
    return ai_table_row_count(move_data->ai_tables, category);
}

/* Soft ceiling: the scoped propagation mode recovers retail's dynamic indexed
 * table loads and 104 of the former 108 missing bytes. The remaining four-byte
 * deficit is one reused move_blend_data pointer in the low-attack override;
 * other residue is GPR coloring. */
#pragma opt_propagation off
int drone_ai_check_attack(DroneAI* drone, int force, int immediate) {
    PlyrMoveBlendData* move_data;
    AiFightstyleAttack* script;
    unsigned int special_count;
    unsigned int fightstyle_count;
    unsigned int attack_flags;
    unsigned int stage;
    unsigned int elapsed;
    int attack_state;
    int low_attack;
    int allow_special;

    low_attack = 0;
    if (!drone_ai_should_be_attacking(
            drone, &attack_state, force)) {
        return 0;
    }
    if (immediate == 1 && attack_state == 3) {
        attack_state = 0;
    }

    if ((his_pdata->state & 0x100) != 0) {
        stage = drone->big_boss_stage;
        elapsed = game_tick_ctr - drone->duck_reaction_tick;
        if (stage < 2 || (stage == 2 && drone->match_stage < 4)) {
            allow_special = 0;
        } else if (elapsed < 15) {
            if (stage == 4 && randu0(100) < 70) {
                allow_special = 1;
            } else {
                allow_special = 0;
            }
        } else if (elapsed > 120) {
            allow_special = 1;
        } else if (drone->big_boss_stage == 2 && randu0(100) < 20) {
            allow_special = 1;
        } else if (drone->big_boss_stage == 3 && randu0(100) < 40) {
            allow_special = 1;
        } else if (drone->big_boss_stage == 4 && randu0(100) < 50) {
            allow_special = 1;
        } else {
            allow_special = 0;
        }
        if (allow_special == 1) {
            low_attack = 1;
            attack_state = 0;
        }
    }
    if (drone_ai_change_attack_to_low(drone)) {
        low_attack = 1;
    }

    if (attack_state == 4) {
        ai_transfer_active(drone_ai_perform_impale_attack);
        return 1;
    }
    if (attack_state == 1) {
        PlyrFighterDefinition* fighter;
        unsigned int state1_fightstyle_count;

        fighter = plyr_pdata->fighter_definition;
        move_data = fighter->move_blend_data;
        state1_fightstyle_count =
            move_data != 0
                ? move_data->ai_tables[10].usable_row_count
                : 0;
        special_count = move_data != 0
                            ? move_data->ai_tables[0].usable_row_count
                            : 0;
        if ((his_pdata->state & 0x100) != 0) {
            script = get_random_fightstyle_attack(fighter, 3, 1);
        } else if (special_count > 6 &&
                   xz_distance_between_players() < 7.1883736f &&
                   randu0(100) < 30) {
            script = get_random_fightstyle_attack(fighter, 0, 1);
        } else if (state1_fightstyle_count > 6 && randu0(100) < 30) {
            script = get_random_fightstyle_attack(fighter, 10, 1);
        } else {
            script = get_random_fightstyle_attack(fighter, 1, 1);
        }
        if (script == 0) {
            return 0;
        }
        drone->script_attack = script;
        drone->script_attack_ready = 2;
        if (immediate == 1) {
            drone_ai_perform_script_attack();
            return 1;
        }
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    if (attack_state == 5) {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 10, 2);
        if (script == 0) {
            return 0;
        }
        drone->script_attack = script;
        drone->script_attack_ready = 2;
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    if (attack_state == 0 || attack_state == 2) {
        int category;

        attack_flags = 0;
        if (his_pdata->state != 0x600 &&
            (drone->difficulty_index >= 2 || randu0(100) < 65)) {
            attack_flags |= 0x100;
        }
        if (drone->opponent_distance > 14.6f) {
            category = 2;
            attack_flags |= 8;
            fightstyle_count = 0;
            special_count = ai_table_row_count(
                plyr_pdata->ai_tables->tables, category);
        } else if (drone->opponent_distance > 5.9457946f) {
            category = 1;
            attack_flags |= 4;
            special_count = ai_table_row_count(
                plyr_pdata->ai_tables->tables, category);
            move_data = plyr_pdata->fighter_definition->move_blend_data;
            fightstyle_count =
                ai_move_table_row_count(move_data, category);
        } else if (drone->opponent_distance > 2.8103173f) {
            category = 1;
            attack_flags |= 2;
            special_count = ai_table_row_count(
                plyr_pdata->ai_tables->tables, category);
            move_data = plyr_pdata->fighter_definition->move_blend_data;
            fightstyle_count =
                ai_move_table_row_count(move_data, category);
            if (drone->opponent_distance < 4.378056f &&
                low_attack == 1) {
                category = 3;
                attack_flags |= 1;
                special_count = ai_table_row_count(
                    plyr_pdata->ai_tables->tables, category);
                move_data =
                    plyr_pdata->fighter_definition->move_blend_data;
                fightstyle_count = ai_move_table_row_count(
                    move_data, category);
            }
        } else if ((his_pdata->state & 0x900) != 0) {
            category = 3;
            attack_flags |= 1;
            special_count = ai_table_row_count(
                plyr_pdata->ai_tables->tables, category);
            move_data = plyr_pdata->fighter_definition->move_blend_data;
            fightstyle_count =
                ai_move_table_row_count(move_data, category);
        } else if (his_obj->pos.value.y < 0.4f + g_game_info.field_34) {
            category = 3;
            attack_flags |= 1;
            special_count = ai_table_row_count(
                plyr_pdata->ai_tables->tables, category);
            move_data = plyr_pdata->fighter_definition->move_blend_data;
            fightstyle_count =
                ai_move_table_row_count(move_data, category);
        } else {
            attack_flags |= 1;
            category = 0;
            if (randu0(100) < 10) {
                attack_flags |= 2;
                category = 1;
            } else if (low_attack == 1 &&
                       (drone->movement_state == 8 ||
                        randu0(100) < 30)) {
                attack_flags |= 1;
                category = 3;
            }
            special_count = ai_table_row_count(
                plyr_pdata->ai_tables->tables, category);
            move_data = plyr_pdata->fighter_definition->move_blend_data;
            fightstyle_count =
                ai_move_table_row_count(move_data, category);
        }
        if (is_he_airborn() && (attack_flags & 3) != 0) {
            category = 12;
            special_count = ai_table_row_count(
                plyr_pdata->ai_tables->tables, category);
            move_data = plyr_pdata->fighter_definition->move_blend_data;
            fightstyle_count =
                ai_move_table_row_count(move_data, category);
        }

        if (attack_state == 2) {
            if (special_count != 0) {
                if (fightstyle_count == 0 && special_count != 0 &&
                    his_pdata->state != 0x600 &&
                    randu0(3) == 0) {
                    allow_special = 0;
                } else {
                    allow_special = 1;
                }
                if (allow_special == 1) {
                    script = ai_pick_special_move(category);
                    if (script != 0) {
                        drone->script_attack = script;
                        drone->script_attack_ready = 1;
                        if (immediate == 1) {
                            drone_ai_perform_script_attack();
                            return 1;
                        }
                        ai_transfer_active(drone_ai_perform_script_attack);
                        return 1;
                    }
                } else if (fightstyle_count == 0) {
                    drone->movement_attempt = 1;
                    return 0;
                }
            }
        }
        if (fightstyle_count != 0) {
            script = get_random_fightstyle_attack(
                plyr_pdata->fighter_definition, category, 0);
            if (script == 0) {
                return 0;
            }
            drone->script_attack = script;
            drone->script_attack_ready = 2;
            if (immediate == 1) {
                drone_ai_perform_script_attack();
                return 1;
            }
            ai_transfer_active(drone_ai_perform_script_attack);
            return 1;
        }
    } else if (attack_state == 3) {
        drone_ai_perform_jump_attack(drone);
        return 1;
    }
    return 0;
}
#pragma opt_propagation reset

float drone_ai_perform_combo_attack(void) {
    DroneAI* drone;
    PlyrFighterDefinition* fighter;
    AiComboTable* combo_table;
    unsigned int distant_count;
    unsigned int close_count;
    void* script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    fighter = plyr_pdata->fighter_definition;
    combo_table = (AiComboTable*)fighter->move_blend_data;
    distant_count = combo_table != 0
                         ? combo_table->distant_attack_count : 0;
    close_count = combo_table != 0
                      ? combo_table->close_attack_count : 0;
    if ((his_pdata->state & 0x100) != 0) {
        script = get_random_fightstyle_attack(fighter, 3, 1);
    } else if (close_count > 6 &&
               xz_distance_between_players() < 7.1883736f &&
               randu0(100) < 30) {
        script = get_random_fightstyle_attack(fighter, 0, 1);
    } else if (distant_count > 6 &&
               randu0(100) < 30) {
        script = get_random_fightstyle_attack(fighter, 10, 1);
    } else {
        script = get_random_fightstyle_attack(fighter, 1, 1);
    }
    drone->command_active = 1;
    if (script == 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    drone->script_attack = script;
    AI_TRANSFER(drone_ai_perform_script_attack);
    return 0.0f;
}

int drone_ai_check_combo_breaker(void) {
    DroneAI* drone;
    int likelihood;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    likelihood = handicap_likelihood_for_combo_breaker(drone);
    if ((int)randu0(100) < likelihood) {
        return 1;
    }
    return 0;
}

int drone_super_combo_refresh(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->super_combo_active = 0;
    return 0;
}

#pragma dont_inline on
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
#pragma dont_inline reset

#pragma dont_inline on
void drone_ai_perform_jump_attack(DroneAI* request) {
    DroneAI* drone;
    MkProc* player_proc;

    request->background_attack_active = 0;
    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    player_proc = get_player_number(plyr_obj) == 0
                      ? g_game_info.plyr0.idle_proc
                      : g_game_info.plyr1.idle_proc;
    xfer_proc(player_proc, jump_towards_opponent_with_jexit);
    drone->request_active = 1;
    set_my_state(0);
}
#pragma dont_inline reset


int drone_ai_victim_ducking(void) {
    DroneAI* drone;
    void* script;
    int is_script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if ((his_pdata->state & 0x100) == 0) {
        drone->duck_reaction_tick = 0;
        drone->reaction_watcher = 0;
        return 0;
    }
    if (drone->opponent_distance >= 7.3f &&
        drone->opponent_distance < 14.6f &&
        randu0(100) < 25 &&
        dist_behind_me() > 1.0f) {
        ai_transfer_active(drone_walk_backwards_further_with_jexit);
        return 1;
    }
    if (drone->opponent_distance > 14.6f &&
        ai_count_taunt_moves() > 0 &&
        randu0(100) < 15) {
        ai_transfer_active(drone_ai_perform_taunt);
        return 1;
    }
    if (drone->opponent_distance < 5.9457946f) {
        script = drone_ai_choose_move_from_category(6, 75, &is_script);
        if (script != 0) {
            drone->script_attack = script;
            if (is_script != 0) {
                drone->script_attack_ready = 1;
            } else {
                drone->script_attack_ready = 2;
            }
            ai_transfer_active(drone_ai_perform_script_attack);
            return 1;
        }
    }
    drone->reaction_watcher = 0;
    return 0;
}

int drone_ai_victim_slipping_on_vomit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4207) {
        drone->reaction_watcher = 0;
        return 0;
    }
    if (drone->opponent_distance > 13.378037f) {
        ai_transfer_active(jump_towards_opponent_with_jexit);
        return 1;
    }
    if (!(xz_distance_between_players() < 1.4864486f)) {
        ai_transfer_active(walk_forward_attackdist_with_jexit);
        return 1;
    }
    drone_ai_attacker_defenseless(drone);
    return 1;
}

static int drone_ai_victim_throw_attempt(void) {
    DroneAI* drone;
    void* script;
    unsigned int likelihood;
    unsigned short roll;
    int is_script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    roll = randu0(100);
    if (his_pdata->state != 0x120C) {
        drone->decision_ready = 1;
        drone->reaction_watcher = 0;
        return 0;
    }

    plyr_pdata->opponent_attack_counter = his_pdata->attack_counter;
    plyr_pdata->reaction_counter = his_pdata->attack_counter;
    if (drone->big_boss_stage < 2) {
        likelihood = 30;
        if (drone->match_stage == 0) {
            likelihood = 100;
        } else if (drone->difficulty_index < 4) {
            likelihood = 60;
        }
    } else if (drone->big_boss_stage == 2) {
        likelihood = 30;
        if (drone->match_stage == 0) {
            likelihood = 100;
        } else if (drone->difficulty_index < 3) {
            likelihood = 40;
        } else if (drone->difficulty_index == 5) {
            likelihood = 18;
        } else if (drone->difficulty_index > 5) {
            likelihood = 3;
        }
    } else {
        likelihood = 12;
        if (drone->match_stage == 0) {
            likelihood = 40;
        } else if (drone->difficulty_index < 3) {
            likelihood = 24;
        } else if (drone->difficulty_index > 5) {
            likelihood = 3;
        }
    }

    if (likelihood < 50 && drone->field_150 == 0) {
        likelihood = randu0(30) + 50;
    } else if (likelihood < 50 && likelihood > 5 &&
               drone->field_150 > 3) {
        likelihood = 5;
    }
    if (roll < likelihood) {
        drone->field_150++;
        if (randu0(100) < 10) {
            drone->attack_pending = 1;
            drone->reaction_watcher = 0;
            ai_transfer_active(drone_ai_perform_block);
            return 1;
        }
        drone->decision_ready = 1;
        drone->reaction_watcher = 0;
        return 0;
    }

    if (drone->big_boss_stage < 4 && randu0(100) < 7) {
        drone->field_150 = 0;
    }
    if (xz_distance_between_players() < 4.84f) {
        if (drone->difficulty_index > 3 && randu0(100) < 33) {
            script =
                drone_ai_choose_move_from_category(11, 75, &is_script);
            if (script != 0) {
                ai_transfer_active(drone_ai_duck_throw_attack);
            } else {
                ai_transfer_active(drone_ai_duck_attack);
            }
        } else {
            ai_transfer_active(drone_ai_duck_attack);
        }
        return 1;
    }

    drone->decision_ready = 1;
    drone->reaction_watcher = 0;
    return 0;
}

/* Soft ceiling: exact algorithm, CFG, calls, widths, and non-load operations.
 * Retail reloads player X/Z after the first square root; current MWCC CSE
 * retains those values, with downstream register/stack-slot coloring only. */
static int drone_ai_victim_avoid(void) {
    DroneAI* drone;
    float target_x;
    float target_z;
    float enemy_x;
    float enemy_z;
    float target_distance;
    float enemy_distance;
    float inverse_distance;
    float unit_x;
    float unit_z;
    float cross;
    int should_avoid;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    should_avoid = 0;
    if (drone->avoidance_area_duration > 0.0f) {
        if (his_obj == 0 || plyr_obj == 0) {
            return 0;
        }
        target_x =
            drone->avoidance_position[0] - plyr_obj->pos.value.x;
        target_z =
            drone->avoidance_position[2] - plyr_obj->pos.value.z;
        target_distance =
            ai_sqrt_table(target_x * target_x + target_z * target_z);
        if (target_distance == 0.0f) {
            return 0;
        }
        enemy_x = his_obj->pos.value.x - plyr_obj->pos.value.x;
        enemy_z = his_obj->pos.value.z - plyr_obj->pos.value.z;
        enemy_distance =
            ai_sqrt_table(enemy_x * enemy_x + enemy_z * enemy_z);
        inverse_distance = enemy_distance > 0.0f
                               ? 1.0f / enemy_distance
                               : enemy_distance;
        unit_x = enemy_x * inverse_distance;
        unit_z = enemy_z * inverse_distance;
        if (enemy_distance == 0.0f) {
            return 0;
        }
        if (enemy_distance < target_distance - 1.0f) {
            return 0;
        }
        cross = target_x * unit_z + target_z * -unit_x;
        if (cross < 0.0f) {
            cross = -cross;
        }
        if (cross < 1.25f) {
            should_avoid = 1;
        }
    }
    if (should_avoid != 0) {
        ai_transfer_active(side_step_to_center_with_jexit);
        return 1;
    }
    drone->reaction_watcher = 0;
    return 0;
}

/*
 * Retail supports this function's local O2 mode: using the TU's O4,s mode
 * regresses the diff. Exact size and CFG now agree; residue is FP constant
 * folding, register coloring, and scheduling in the distance calculations.
 */
#pragma optimization_level 2
int drone_ai_victim_dizzy(void) {
    DroneAI* drone;
    float range;
    float desired_distance;
    float delta;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return 0;
    }
    switch (drone->fatality_decision) {
    case 0:
        if (randu0(100) < 80 ||
            !can_i_do_fatality_now(plyr_pdata->plyr_num) ||
            g_game_info.feature_flags.bits.powerbars_locked != 0) {
            drone->fatality_decision = 1;
        } else if (g_game_number > g_fatality_game_number + 6 &&
                   randu0(100) < 30) {
            g_fatality_game_number = g_game_number;
            drone->fatality_decision = 2;
        } else {
            drone->fatality_decision = 1;
        }
        if (is_big_boss(plyr_pdata)) {
            drone->fatality_decision = 4;
        }
        if (mode_of_play == 10) {
            drone->fatality_decision = 0;
        }
        break;
    case 4:
        if (!(xz_distance_between_players() < 3.3445094f)) {
            ai_transfer_active(walk_forward_attackdist2_with_jexit);
            return 1;
        } else {
            drone->reaction_watcher = drone_ai_victim_dizzy_3;
            break;
        }
    case 1:
        if (drone->opponent_distance > 13.378037f) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        } else if (!(xz_distance_between_players() < 1.4864486f)) {
            ai_transfer_active(walk_forward_attackdist_with_jexit);
            return 1;
        } else {
            drone->reaction_watcher = drone_ai_victim_dizzy_2;
            break;
        }
    case 2:
        range = 2.0f;
        desired_distance = range * range;
        delta = drone->opponent_distance - desired_distance;
        if (delta < range) {
            if (delta > -2.0f) {
                drone->fatality_decision = 3;
                if (randu0(100) < 95) {
                    ai_transfer_active(do_my_fatality);
                } else {
                    ai_transfer_active(do_my_2nd_fatality);
                }
                return 1;
            }
        }
        if (drone->opponent_distance < desired_distance) {
            ai_transfer_active(step_backward_with_jexit);
            return 1;
        }
        if (delta > 5.9457946f) {
            ai_transfer_active(jump_towards_opponent_with_jexit);
            return 1;
        }
        ai_transfer_active(step_forward_with_jexit);
        return 1;
    default:
        break;
    }
    return 1;
}
#pragma optimization_level 4


static int drone_ai_im_dizzy(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (plyr_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return 0;
    }

    switch (drone->fatality_decision) {
    case 0:
        if (get_fatality_available_flag() == 1) {
            if (g_game_number > g_fatality_game_number + 6 &&
                randu0(100) < 30) {
                g_fatality_game_number = g_game_number;
                drone->fatality_decision = 2;
            } else {
                drone->fatality_decision = 1;
            }
        }
        if (is_big_boss(plyr_pdata)) {
            drone->fatality_decision = 1;
        }
        if (mode_of_play == 10) {
            drone->fatality_decision = 1;
        }
        break;
    case 1:
        return 0;
    case 2:
        ai_transfer_active(do_my_suicide);
        return 1;
    default:
        break;
    }
    return 1;
}

/* Soft ceiling: exact size, CFG, instructions, calls, stores, and selection.
 * The remaining differences are operand/register coloring only. */
static int drone_ai_victim_dizzy_3(void) {
    DroneAI* drone;
    DroneAI* command_drone;
    AiSpecialMoveList* moves;
    void* category_script;
    void* script;
    void* selected_script;
    unsigned int roll_value;
    unsigned short roll;
    int ranged_count;
    int is_script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    category_script =
        drone_ai_choose_move_from_category(11, 75, &is_script);
    script = 0;
    roll_value = randu0(100);
    roll = (unsigned short)roll_value;
    if (his_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return 0;
    }
    moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
    ranged_count = moves->ranged_count;
    if (roll < 40 && ranged_count > 0) {
        command_drone = get_player_number(plyr_obj) == 0
                            ? &g_DroneAI1 : &g_DroneAI2;
        moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
        ranged_count = moves->ranged_count;
        if (ranged_count == 0) {
            selected_script = 0;
        } else {
            command_drone->ai_command =
                moves->ranged_commands +
                randu0((unsigned short)ranged_count) * 16;
            command_drone->ai_command_arg = 0;
            command_drone->ai_command_target =
                command_drone->player->character_id;
            command_drone->ai_command_flag0 = 0;
            command_drone->ai_command_flag1 = 0;
            command_drone->ai_command_flag2 = 0;
            selected_script = get_special_move();
        }
        script = selected_script;
        drone->script_attack_ready = 1;
    } else if (category_script != 0 && roll < 80) {
        if (is_script != 0) {
            drone->script_attack_ready = 1;
        } else {
            drone->script_attack_ready = 2;
        }
    } else {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 10, 3);
        if (script == 0) {
            return 0;
        }
    }
    if (script != 0) {
        drone->script_attack = script;
        ai_transfer_active(drone_ai_perform_script_attack);
        return 1;
    }
    return 0;
}

int drone_ai_victim_dizzy_2(void) {
    DroneAI* drone;
    AiMoveBlendScripts* scripts;
    int* fatality_scripts;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4203 || f_fatality_was_done != 0) {
        drone->reaction_watcher = 0;
        return 0;
    }

    scripts =
        (AiMoveBlendScripts*)plyr_pdata->fighter_definition->move_blend_data;
    fatality_scripts = scripts->fatality_scripts;
    if (fatality_scripts == 0) {
        return drone_ai_attacker_defenseless(drone);
    }
    drone->script_attack = &fatality_scripts[14];
    ai_transfer_active(drone_ai_scripted_attack);
    return 1;
}

int drone_ai_victim_frozen(void) {
    DroneAI* drone;
    int state;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    state = his_pdata->state;
    if (state != 0xC600 &&
        state != 0x202 && state != 0x421A) {
        drone->reaction_watcher = 0;
        return 0;
    }
    if (drone->opponent_distance > 13.378037f) {
        ai_transfer_active(jump_towards_opponent_with_jexit);
        return 1;
    }
    if (!(xz_distance_between_players() < 1.4864486f)) {
        ai_transfer_active(walk_forward_attackdist_with_jexit);
        return 1;
    }
    return drone_ai_attacker_defenseless(drone);
}

int drone_ai_victim_speared_2(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4204) {
        drone->reaction_watcher = 0;
        return 0;
    }
    if (drone->opponent_distance > 13.378037f) {
        ai_transfer_active(jump_towards_opponent_with_jexit);
        return 1;
    }
    if (!(xz_distance_between_players() < 1.4864486f)) {
        ai_transfer_active(walk_forward_attackdist_with_jexit);
        return 1;
    }
    return drone_ai_attacker_defenseless(drone);
}

int drone_ai_victim_speared(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->state != 0x4204) {
        drone->reaction_watcher = 0;
        return 0;
    }
    if (randu0(3) == 0) {
        ai_transfer_active(change_to_weapon_style_with_j_exit);
    }
    drone->reaction_watcher = drone_ai_victim_speared_2;
    return 1;
}

float drone_ai_perform_script_attack(void) {
    DroneAI* drone;
    AiFightstyleAttackTable* attacks;
    AiFightstyleAttack* attack;
    unsigned int attack_index;
    int input_direction;
    int should_side_step;
    int is_special_attack;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (his_pdata->drone_request != 0) {
        input_direction = 0;
    } else if (check_switch(his_pdata->controller_port, 0xF) != 0) {
        if (check_switch(his_pdata->controller_port, 0xE) != 0) {
            input_direction = 5;
        } else {
            input_direction = 1;
        }
    } else if (check_switch(his_pdata->controller_port, 0xD) != 0) {
        if (check_switch(his_pdata->controller_port, 0xE) != 0) {
            input_direction = 5;
        } else {
            input_direction = 2;
        }
    } else if (check_switch(his_pdata->controller_port, 0xC) != 0) {
        input_direction = 3;
    } else if (check_switch(his_pdata->controller_port, 0xE) != 0) {
        input_direction = 4;
    } else {
        input_direction = 0;
    }

    if (drone->script_attack == 0) {
        blend_to_stance(0.1f);
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    if ((drone->script_attack_ready & 1) != 0) {
        AI_SLEEP((float)(unsigned short)(
            g_baseSpecialMoveWait[drone->difficulty_index] +
            randu0(g_randSpecialMoveWait[drone->difficulty_index])));
    } else if (his_pdata->drone_request == 0 &&
               (his_pdata->state & 0x1500) == 0 &&
               g_game_info.flag_bits.field_bit0 == 0 &&
               g_game_info.pause_flag_bits.fatality_window == 0 &&
               (his_pdata->strafe_direction != 0 ||
                input_direction != 0)) {
        if (drone->big_boss_stage < 2) {
            should_side_step = 0;
        } else if (drone->big_boss_stage == 2 &&
                   drone->match_stage > 3 && randu0(100) < 35) {
            should_side_step = 1;
        } else if (drone->big_boss_stage == 3 &&
                   randu0(100) < 60) {
            should_side_step = 1;
        } else if (drone->big_boss_stage == 4 &&
                   randu0(100) < 85) {
            should_side_step = 1;
        } else {
            should_side_step = 0;
        }
        if (should_side_step == 1) {
            int opponent_is_right;

            opponent_is_right = is_a_to_the_right_of_b(his_obj, plyr_obj);
            if (his_pdata->strafe_direction == 3 ||
                input_direction == 3) {
                drone_step_LR_true(always_true, 5, opponent_is_right);
            } else if (his_pdata->strafe_direction == 4 ||
                       input_direction == 4) {
                drone_step_LR_true(
                    always_true, 5, opponent_is_right == 0);
            }
        }
    }

    attack = (AiFightstyleAttack*)drone->script_attack;
    if (am_i_airborn() == 0) {
        pre_attack_chores();
        if (attack->opcode == 0) {
            attacks = (AiFightstyleAttackTable*)
                plyr_pdata->fighter_definition->move_blend_data;
            attack_index = attack - attacks->attacks;
            if (attack_index == 12 || attack_index == 17 ||
                attack_index == 2) {
                is_special_attack = 1;
            } else {
                is_special_attack = 0;
            }
        } else {
            is_special_attack = 0;
        }
        if (is_special_attack != 0) {
            set_my_state(0x1300);
        }
        switch (attack->opcode) {
        case 0:
            cmdscript_reset_stack();
            cmdscript_setup_execution(
                plyr_pdata->fighter_definition->cmo, attack->argument);
            call_player_script_function(
                plyr_pdata->fighter_definition->cmo);
            break;
        case 1:
            AI_TRANSFER(attack->entry);
            break;
        case 2:
            cmdscript_reset_stack();
            cmdscript_setup_execution(plyr_pdata->cmo, attack->argument);
            call_player_script_function(plyr_pdata->cmo);
            break;
        case 4:
            cmdscript_reset_stack();
            cmdscript_setup_execution(reactions_cmo, attack->argument);
            call_player_script_function(reactions_cmo);
            break;
        }
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

float jump_away_opponent_with_j_exit(void) {
    DroneAI* drone;
    float right_clearance;
    float left_clearance;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (ai_backward_clearance() > 1.524f) {
        jump_away_opponent();
    } else {
        ai_side_clearances(&right_clearance, &left_clearance);
        if (right_clearance > left_clearance) {
            drone_step_LR_true(
                always_false, (randu0(3) + 1) * 30, 0);
        } else {
            drone_step_LR_true(
                always_false, (randu0(3) + 1) * 30, 1);
        }
    }
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float jump_towards_opponent_with_j_exit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    jump_towards_opponent();
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float drone_ai_perform_reversal(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float drone_ai_perform_impale_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static inline int ai_find_knockdown_style(void) {
    AiStyleSlot* style;
    int count;

    if (!fight_style_restriction_table.knockdown_allowed()) {
        return -1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[0];
    count = style->moves != 0 ? style->moves->knockdown_count : 0;
    if (count != 0) {
        return 0;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[1];
    count = style->moves != 0 ? style->moves->knockdown_count : 0;
    if (count != 0) {
        return 1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[2];
    count = style->moves != 0 ? style->moves->knockdown_count : 0;
    if (count != 0) {
        return 2;
    }
    return -1;
}

static float drone_ai_perform_knockdown(void) {
    DroneAI* drone;
    PlyrPdata* action;
    AiFightstyleAttackTable* attacks;
    AiFightstyleAttack* attack;
    int style;
    int can_attack;
    unsigned int attack_index;
    int is_special_attack;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else {
        if (action->action_lock_a > game_tick_ctr) {
            can_attack = 0;
        } else if (action->action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else if (action->push_blocked != 0 ||
                   (action->state & 0x200) != 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
        if (can_attack == 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
    }
    if (can_attack == 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }

    style = ai_find_knockdown_style();
    if (style < 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    drone_ai_force_change_style(drone, style);
    if (plyr_pdata->player_slot != style) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    attack = get_random_fightstyle_attack(
        plyr_pdata->fighter_definition, 9, 0);
    if (attack == 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }

    if (am_i_airborn() == 0) {
        pre_attack_chores();
        if (attack->opcode == 0) {
            attacks = (AiFightstyleAttackTable*)
                plyr_pdata->fighter_definition->move_blend_data;
            attack_index = attack - attacks->attacks;
            if (attack_index == 12 || attack_index == 17 ||
                attack_index == 2) {
                is_special_attack = 1;
            } else {
                is_special_attack = 0;
            }
        } else {
            is_special_attack = 0;
        }
        if (is_special_attack != 0) {
            set_my_state(0x1300);
        }
        switch (attack->opcode) {
        case 0:
            cmdscript_reset_stack();
            cmdscript_setup_execution(
                plyr_pdata->fighter_definition->cmo, attack->argument);
            call_player_script_function(
                plyr_pdata->fighter_definition->cmo);
            break;
        case 1:
            AI_TRANSFER(attack->entry);
            break;
        case 2:
            cmdscript_reset_stack();
            cmdscript_setup_execution(plyr_pdata->cmo, attack->argument);
            call_player_script_function(plyr_pdata->cmo);
            break;
        case 4:
            cmdscript_reset_stack();
            cmdscript_setup_execution(reactions_cmo, attack->argument);
            call_player_script_function(reactions_cmo);
            break;
        }
    }
    drone->attack_pending = 0;
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static inline int ai_find_charge_style(void) {
    AiStyleSlot* style;
    int count;

    if (!fight_style_restriction_table.charge_allowed()) {
        return -1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[0];
    count = style->moves != 0 ? style->moves->charge_count : 0;
    if (count != 0) {
        return 0;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[1];
    count = style->moves != 0 ? style->moves->charge_count : 0;
    if (count != 0) {
        return 1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[2];
    count = style->moves != 0 ? style->moves->charge_count : 0;
    if (count != 0) {
        return 2;
    }
    return -1;
}

/* Soft ceiling: exact size, CFG, helper expansion, calls, and field accesses.
 * Residue is a whole-lifetime r30/r31 swap between drone and style. */
float drone_ai_perform_charge_up(void) {
    DroneAI* drone;
    int style;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    style = ai_find_charge_style();
    if (style < 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    drone_ai_force_change_style(drone, style);
    if (plyr_pdata->player_slot != style) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }

    drone->jump_attack_pending = 1;
    drone->charge_cooldown_tick = exec_tick_ctr + 240;
    if (drone->movement_state != 1 && drone->movement_state != 5) {
        drone->movement_state = 1;
    }
    drone->attack_type = 8;
    AI_TRANSFER(drone_ai_perform_attack);
    return 0.0f;
}

static inline int ai_find_taunt_style(void) {
    AiStyleSlot* style;
    int count;

    if (!fight_style_restriction_table.taunt_allowed()) {
        return -1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[0];
    count = style->moves != 0 ? style->moves->taunt_count : 0;
    if (count != 0) {
        return 0;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[1];
    count = style->moves != 0 ? style->moves->taunt_count : 0;
    if (count != 0) {
        return 1;
    }
    style = (AiStyleSlot*)plyr_pdata->weapon_styles[2];
    count = style->moves != 0 ? style->moves->taunt_count : 0;
    if (count != 0) {
        return 2;
    }
    return -1;
}

/* Soft ceiling: exact size, CFG, helper expansion, calls, and field accesses.
 * Residue is a whole-lifetime r30/r31 swap between drone and style. */
float drone_ai_perform_taunt(void) {
    DroneAI* drone;
    int style;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    style = ai_find_taunt_style();
    if (style < 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    drone_ai_force_change_style(drone, style);
    if (plyr_pdata->player_slot != style) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }

    drone->attack_type = 7;
    AI_TRANSFER(drone_ai_perform_attack);
    return 0.0f;
}

/* Soft ceiling: exact size, CFG, operations, calls, and data accesses.
 * The four remaining operand differences are temporary-register coloring. */
static float drone_ai_perform_push(void) {
    DroneAI* drone;
    AiWeaponStyleView* style;
    AiMoveBlendScripts* scripts;
    int style_index;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    style = (AiWeaponStyleView*)plyr_pdata->weapon_styles[0];
    switch (style->style_id) {
    case 5:
        style_index = 0;
        break;
    default:
        style = (AiWeaponStyleView*)plyr_pdata->weapon_styles[1];
        switch (style->style_id) {
        case 5:
            style_index = 1;
            break;
        default:
            style_index = -1;
            break;
        }
        break;
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

static float drone_ai_perform_weapon_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, 2);
    if (plyr_pdata->player_slot != 2) {
        ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
        return 0.0f;
    }
    drone->attack_type = 1;
    ((AiProcVtable*)aproc->vtbl)
        ->transfer(drone_ai_perform_attack, 0.0f);
    return 0.0f;
}

static float drone_ai_perform_low_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->attack_type = 3;
    ((AiProcVtable*)aproc->vtbl)
        ->transfer(drone_ai_perform_attack, 0.0f);
    return 0.0f;
}

static float walk_backward_walk_ticks_jexit(void) {
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
    return 0.0f;
}

float dash_back_with_jexit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (plyr_pdata != 0 && is_big_boss(plyr_pdata) != 0) {
        step_backward();
    } else {
        joy_dash_back();
    }
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float jump_away_opponent_with_jexit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    jump_away_opponent();
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float side_step_to_center_long_with_jexit(void) {
    float right_clearance;
    float left_clearance;

    ai_side_clearances(&right_clearance, &left_clearance);
    if (right_clearance > left_clearance) {
        drone_step_LR_true(
            always_false, (randu0(3) + 1) * 60, 0);
    } else {
        drone_step_LR_true(
            always_false, (randu0(3) + 1) * 60, 1);
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static float side_step_to_center_attack_with_jexit(void) {
    float right_clearance;
    float left_clearance;

    ai_side_clearances(&right_clearance, &left_clearance);
    if (right_clearance > left_clearance) {
        drone_step_LR_true(
            HeIsNotFacing, (randu0(3) + 1) * 30, 0);
    } else {
        drone_step_LR_true(
            HeIsNotFacing, (randu0(3) + 1) * 30, 1);
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

float side_step_to_center_with_jexit(void) {
    float right_clearance;
    float left_clearance;

    ai_side_clearances(&right_clearance, &left_clearance);
    if (right_clearance > left_clearance) {
        drone_step_LR_true(
            always_false, (randu0(3) + 1) * 25, 0);
    } else {
        drone_step_LR_true(
            always_false, (randu0(3) + 1) * 25, 1);
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static float step_backward_with_jexit(void) {
    step_backward();
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float walk_forward_attackdist2_with_jexit(void) {
    drone_walk_FB_true(InAttackRange2, 0x3C, 1, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float walk_forward_attackdist_close_with_jexit(void) {
    drone_walk_FB_true(InAttackRange_close, 0x3C, 1, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float walk_forward_attackdist_with_jexit(void) {
    drone_walk_FB_true(InAttackRange, 0x3C, 1, 1);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

float change_to_weapon_style_with_j_exit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, 2);
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float step_forward_with_jexit(void) {
    step_forward();
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: exact size, operations, CFG, calls, widths, and save/restore.
 * All remaining differences are equivalent r29-r31 operand coloring. */
static float catch_opponent(void) {
    DroneAI* active_drone;
    AiFightstyleAttack* script;

    active_drone = get_player_number(plyr_obj) == 0
                       ? &g_DroneAI1 : &g_DroneAI2;
    script = ai_pick_status_special_move();
    if (script != 0) {
        active_drone->script_attack = script;
        active_drone->script_attack_ready = 1;
        drone_ai_perform_script_attack();
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static float jump_towards_opponent_with_jexit(void) {
    jump_towards_opponent();
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float jump_towards_opponent_with_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    jump_towards_opponent();
    drone->jump_attack_pending = 1;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float drone_ai_special_attack_now(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->force_attack = 1;
    drone_ai_check_attack(drone, 1, 1);
    drone->force_attack = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

static float drone_ai_counter_attack_now(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (drone_ai_check_attack(drone, 1, 1) == 1) {
        ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
        return 0.0f;
    }
    jump_towards_opponent();
    drone->jump_attack_pending = 1;
    drone->attack_pending = 0;
    ((AiProcVtable*)aproc->vtbl)->transfer(j_exit, 0.0f);
    return 0.0f;
}

/* Soft ceiling: ABI, guards, bitfield writes, calls, and transfer CFG match.
 * Residue is GPR coloring plus one retail-retained redundant script null test. */
static float drone_ai_attack_obstacle_now(void) {
    DroneAI* drone;
    PlyrPdata* action;
    void* script;
    int can_attack;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else {
        if (action->action_lock_a > game_tick_ctr) {
            can_attack = 0;
        } else if (action->action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else if (action->push_blocked != 0 ||
                   (action->state & 0x200) != 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
        if (can_attack == 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
    }
    if (can_attack == 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }

    plyr_obj->flags_0B_bits.bit3 = 1;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    rotate_towards_position(&drone->obstacle_target, 0.2f);
    script = get_random_fightstyle_attack(
        plyr_pdata->fighter_definition, 9, 0);
    if (script == 0) {
        script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, 10, 0);
    }
    if (script == 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    if (script != 0) {
        drone->script_attack = script;
        drone_ai_perform_script_attack();
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static float drone_ai_dodge_3d_with_counter(void) {
    DroneAI* drone;
    Vec to_opponent;
    Vec facing;
    float right_clearance;
    float left_clearance;
    int likelihood;
    int counter;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    ai_side_clearances(&right_clearance, &left_clearance);
    if (right_clearance > left_clearance) {
        drone_step_LR_true(
            HeIsNotFacing, (randu0(3) + 1) * 20, 0);
    } else {
        drone_step_LR_true(
            HeIsNotFacing, (randu0(3) + 1) * 20, 1);
    }

    if ((g_DroneOverrideInfo.flags & 0x20) != 0) {
        counter = 0;
    } else {
        likelihood =
            g_likelihoodOfBlockCounter[drone->difficulty_index];
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
        if (randu0(100) < (unsigned int)likelihood) {
            counter = 1;
        } else {
            counter = 0;
        }
    }

    if (counter == 1 || drone->difficulty_index > 6) {
        uv_to_opponent(&to_opponent);
        uv_from_angle_y(&facing, his_obj->ang.y);
        if (to_opponent.x * facing.x +
                to_opponent.z * facing.z >
            -0.86f) {
            drone->jump_attack_pending = 1;
        }
    }
    drone->attack_pending = 0;
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static float drone_ai_avoid_position_now(void) {
    ((AiProcVtable*)aproc->vtbl)
        ->transfer(side_step_to_center_with_jexit, 0.0f);
    return 0.0f;
}

/*
 * Retail shares the final j_exit transfer between both jump paths. The
 * remaining diff is limited to redundant zero-result loads and branch
 * placement around the transfer returns.
 */
static float drone_ai_avoid_danger_area_now(void) {
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
        } else {
            AI_TRANSFER(side_step_to_center_with_jexit);
            return 0.0f;
        }
    } else if (drone->danger_side_step == 1) {
        AI_TRANSFER(side_step_to_center_with_jexit);
        return 0.0f;
    } else {
        jump_towards_opponent();
    }
    AI_TRANSFER(j_exit);
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

static float drone_ai_perform_block(void) {
    plyr_pdata->state = 0xA00;
    plyr_pdata->block_start_tick = exec_tick_ctr;
    ((AiProcVtable*)aproc->vtbl)->transfer(x_block, 0.0f);
    return 0.0f;
}

static inline float ai_finish_duck_reaction(
    DroneAI* drone, int throw_attack) {
    AiMoveBlendScripts* scripts;
    int* fatality_scripts;
    void* script;
    unsigned int exposed_ticks = 0;
    int is_script;

    plyr_pdata->his_attack_counter = get_his_attack_counter();
    xfer_proc(plyr_anim_proc, p_animate);
    if (is_big_boss(drone->player)) {
        AI_TRANSFER(x_block);
        return 0.0f;
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

    while ((((int)(his_pdata->state & 0x1000) > 0) ||
            his_pdata->throw_restriction == 3 ||
            his_pdata->duck_reaction_active == 1) &&
           exposed_ticks++ < 25) {
        AI_SLEEP(1.0f);
        if (his_pdata->secondary_state != 0x101 &&
            plyr_pdata->his_attack_counter != get_his_attack_counter()) {
            if (his_pdata->block_requirement != 0 &&
                his_pdata->block_requirement != 6) {
                AI_TRANSFER(x_block);
                return 0.0f;
            }
            exposed_ticks = 0;
            plyr_pdata->his_attack_counter = get_his_attack_counter();
        }
    }
    if (!throw_attack && randu0(100) < 10) {
        scripts = (AiMoveBlendScripts*)
            plyr_pdata->fighter_definition->move_blend_data;
        fatality_scripts = scripts->fatality_scripts;
        if (fatality_scripts != 0) {
            drone->script_attack = &fatality_scripts[14];
            AI_TRANSFER(drone_ai_scripted_attack);
            return 0.0f;
        }
    }
    AI_SLEEP(2.0f + (float)randu0(10));
    drone->attack_pending = 0;
    if (throw_attack) {
        script =
            drone_ai_choose_move_from_category(11, 75, &is_script);
        if (script != 0) {
            drone->script_attack = script;
            if (is_script) {
                drone->script_attack_ready = 1;
            } else {
                drone->script_attack_ready = 2;
            }
            AI_TRANSFER(drone_ai_perform_script_attack);
            return 0.0f;
        }
    } else {
        drone_ai_check_attack(drone, 1, 1);
    }
    set_my_state(0);
    AI_TRANSFER(j_exit_blend_stance);
    return 0.0f;
}

static float drone_ai_duck_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    return ai_finish_duck_reaction(drone, 0);
}

/*
 * The throw specialization matches retail size and opcode counts. Remaining
 * differences are instruction scheduling and GPR allocation in the inlined
 * reaction helper.
 */
static float drone_ai_duck_throw_attack(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    return ai_finish_duck_reaction(drone, 1);
}

/* Soft ceiling: exact size, CFG, calls, and opcodes. The only differences are
 * the operand order of two commutative adds in randomized tick deadlines. */
static int drone_ai_check_change_style(DroneAI* drone) {
    MkObj* player_object;

    if (drone->next_style_change_tick > exec_tick_ctr) {
        return 0;
    }
    if ((g_DroneOverrideInfo.flags & 2) != 0) {
        return 0;
    }
    drone->next_style_change_tick =
        exec_tick_ctr + 120 + randu0(4) * 60;
    player_object = drone->player->plyr_info->slot.mirror_a;
    if (player_object->hide_flag_bits.hidden == 1) {
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

    ai_transfer_active(drone_ai_change_style);
    return 1;
}

static void drone_ai_check_next_AIState(DroneAI* drone) {
    drone->movement_state = drone_ai_fetch_next_AIState(drone);
}

/* Soft ceiling: exact size, CFG, instructions, calls, and typed command stores.
 * The remaining differences are GPR operand coloring only. */
static float drone_ai_perform_range_attack(void) {
    DroneAI* drone;
    void* script;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (drone->opponent_distance > 14.6f) {
        DroneAI* distant_drone;
        AiSpecialMoveList* distant_moves;

        distant_drone = get_player_number(plyr_obj) == 0
                            ? &g_DroneAI1 : &g_DroneAI2;
        distant_moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
        if (distant_moves->count == 0) {
            script = 0;
        } else {
            distant_drone->ai_command =
                distant_moves->commands +
                randu0((unsigned short)distant_moves->count) * 16;
            distant_drone->ai_command_arg = 0;
            distant_drone->ai_command_target =
                distant_drone->player->character_id;
            distant_drone->ai_command_flag0 = 0;
            distant_drone->ai_command_flag1 = 0;
            distant_drone->ai_command_flag2 = 0;
            script = get_special_move();
        }
    } else {
        DroneAI* ranged_drone;
        AiSpecialMoveList* ranged_moves;

        ranged_drone = get_player_number(plyr_obj) == 0
                            ? &g_DroneAI1 : &g_DroneAI2;
        ranged_moves = (AiSpecialMoveList*)plyr_pdata->status_flags;
        if (ranged_moves->ranged_count == 0) {
            script = 0;
        } else {
            ranged_drone->ai_command =
                ranged_moves->ranged_commands +
                randu0((unsigned short)ranged_moves->ranged_count) * 16;
            ranged_drone->ai_command_arg = 0;
            ranged_drone->ai_command_target =
                ranged_drone->player->character_id;
            ranged_drone->ai_command_flag0 = 0;
            ranged_drone->ai_command_flag1 = 0;
            ranged_drone->ai_command_flag2 = 0;
            script = get_special_move();
        }
    }
    if (script != 0) {
        drone->script_attack = script;
        drone->script_attack_ready = 1;
        drone_ai_perform_script_attack();
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

float drone_ai_change_style(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, randu0(3));
    ((AiProcVtable*)aproc->vtbl)->transfer(drone_entry, 0.0f);
    return 0.0f;
}

float drone_ai_perform_attack(void) {
    DroneAI* drone;
    PlyrPdata* action;
    AiFightstyleAttackTable* attacks;
    AiFightstyleAttack* attack;
    unsigned int attack_index;
    int can_attack;
    int is_special_attack;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else {
        if (action->action_lock_a > game_tick_ctr) {
            can_attack = 0;
        } else if (action->action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else if (action->push_blocked != 0 ||
                   (action->state & 0x200) != 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
        if (can_attack == 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
    }
    if (can_attack == 0) {
        return 1.0f;
    }
    pre_attack_chores();
    attack = get_random_fightstyle_attack(
        plyr_pdata->fighter_definition, drone->attack_type, 0);
    if (attack == 0) {
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    if (am_i_airborn() == 0) {
        pre_attack_chores();
        if (attack->opcode == 0) {
            attacks = (AiFightstyleAttackTable*)
                plyr_pdata->fighter_definition->move_blend_data;
            attack_index = attack - attacks->attacks;
            if (attack_index == 12 || attack_index == 17 ||
                attack_index == 2) {
                is_special_attack = 1;
            } else {
                is_special_attack = 0;
            }
        } else {
            is_special_attack = 0;
        }
        if (is_special_attack) {
            set_my_state(0x1300);
        }
        switch (attack->opcode) {
        case 0:
            cmdscript_reset_stack();
            cmdscript_setup_execution(
                plyr_pdata->fighter_definition->cmo, attack->argument);
            call_player_script_function(plyr_pdata->fighter_definition->cmo);
            break;
        case 1:
            AI_TRANSFER(attack->entry);
            break;
        case 2:
            cmdscript_reset_stack();
            cmdscript_setup_execution(plyr_pdata->cmo, attack->argument);
            call_player_script_function(plyr_pdata->cmo);
            break;
        case 4:
            cmdscript_reset_stack();
            cmdscript_setup_execution(reactions_cmo, attack->argument);
            call_player_script_function(reactions_cmo);
            break;
        }
    }
    AI_TRANSFER(j_exit);
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

void drone_ai_finished_request(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->request_active = 0;
}

/*
 * Retail/current have identical size, opcode multiset, CFG, calls, and typed
 * field-store order. Residue is zero-extension/load/add/immediate scheduling
 * and one caller-saved GPR in the two randomized tick expressions.
 */
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
        drone->charge_cooldown_tick = exec_tick_ctr + randu0(60) + 120;
        drone->difficulty_index = 0;
    } else if (randu0(100) < 50) {
        drone->movement_state = 3;
    } else {
        drone->movement_state = 1;
    }

    random_ticks = randu0(120);
    drone->next_style_change_tick = exec_tick_ctr + random_ticks + 180;
    drone->difficulty_update_tick = exec_tick_ctr;
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

#pragma dont_inline on
/* Soft ceiling: typed flags, behavior, and CFG match retail; current code has
 * three zero materializations versus retail's shared value/GPR scheduling. */
float drone_entry(void) {
    DroneAI* drone;
    PlyrPdata* action;
    int previous_state;
    int can_enter_loop;

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
    if (action->action_lock_a > game_tick_ctr) {
        can_enter_loop = 0;
    } else if (action->action_lock_b > game_tick_ctr) {
        can_enter_loop = 0;
    } else if (action->push_blocked != 0 ||
               (action->state & 0x200) != 0) {
        can_enter_loop = 0;
    } else {
        can_enter_loop = 1;
    }
    if (can_enter_loop == 0 ||
        g_game_info.flag_bits.lens_flare_enabled == 0 ||
        action->push_blocked != 0 ||
        g_game_info.pause_flag_bits.controllers_disabled == 1 ||
        (action->state & 0x200) != 0) {
        return 1.0f;
    }
    AI_TRANSFER(drone_loop);
    return 0.0f;
}
#pragma dont_inline reset


/* Soft ceiling: exact size, CFG, calls, table indices, and result conversion.
 * Residue is ordering of the two table loads and one GPR base choice. */
static float drone_loop(void) {
    DroneAI* drone;
    unsigned int ticks;
    unsigned int random_ticks;
    int difficulty;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    end_of_round_check();
    if ((g_game_info.flags & 1) != 0) {
        AI_TRANSFER(drone_entry);
    }
    if (!ai_watcher_can_act()) {
        return 1.0f;
    }
    difficulty = drone->difficulty_index;
    ticks = g_minDecisionBaseWaitTime[difficulty];
    random_ticks = g_randomDecisionBaseWaitTime[difficulty];
    ticks += randu0((unsigned short)random_ticks);
    return (float)ticks;
}

float drone_blocking_done(void) {
    DroneAI* drone;
    int likelihood;
    int counter;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
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
        if (randu0(100) < (unsigned int)likelihood) {
            counter = 1;
        } else {
            counter = 0;
        }
    }
    if (counter == 1) {
        if (drone->difficulty_index < 2) {
            drone->jump_attack_pending = 1;
        } else if (drone->difficulty_index < 4) {
            if (randu0(100) < 40 && plyr_pdata->combo_depth > 4) {
                drone->reversal_pending = 1;
            } else if (randu0(100) < 15 &&
                       plyr_pdata->combo_depth > 1) {
                drone->reversal_pending = 1;
            } else {
                drone->jump_attack_pending = 1;
            }
        } else if (drone->difficulty_index < 6) {
            if (randu0(100) < 70 && plyr_pdata->combo_depth > 3) {
                drone->reversal_pending = 1;
            } else if (randu0(100) < 35 &&
                       plyr_pdata->combo_depth > 1) {
                drone->reversal_pending = 1;
            } else {
                drone->jump_attack_pending = 1;
            }
        } else {
            if (plyr_pdata->combo_depth > 5) {
                drone->reversal_pending = 1;
            } else if (plyr_pdata->combo_depth > 3) {
                if (randu0(100) < 90) {
                    drone->reversal_pending = 1;
                } else {
                    drone->jump_attack_pending = 1;
                }
            } else if (randu0(100) < 55 &&
                       plyr_pdata->combo_depth > 1) {
                drone->reversal_pending = 1;
            } else {
                drone->jump_attack_pending = 1;
            }
        }
    }
    AI_TRANSFER(drone_entry);
    return 0.0f;
}

static int drone_ai_force_change_style(DroneAI* drone, int style) {
    if (is_big_boss(drone->player)) {
        return 0;
    }
    if ((g_DroneOverrideInfo.flags & 2) != 0) {
        return 0;
    }
    if (drone->super_combo_active == 1 ||
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

    if (style == -1) {
        advance_active_moveset(plyr_pdata);
    } else {
        advance_active_moveset(plyr_pdata);
        if (style != plyr_pdata->player_slot) {
            advance_active_moveset(plyr_pdata);
            if (style != plyr_pdata->player_slot) {
                advance_active_moveset(plyr_pdata);
            }
        }
    }
    snd_req(0xDC1);
    blend_to_stance(0.1f);
    AI_SLEEP(1.0f);
    return 1;
}

int drone_ai_check_next_block_state(void) {
    DroneAI* drone;
    Vec facing;
    Vec to_opponent;
    unsigned int likelihood;
    int facing_opponent;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    uv_to_opponent(&to_opponent);
    uv_from_angle_y(&facing, his_obj->ang.y);
    if (plyr_pdata->drone_request == 0) {
        return 0;
    }

    if (to_opponent.x * facing.x +
            to_opponent.z * facing.z > -0.86f) {
        facing_opponent = 1;
    } else {
        facing_opponent = 0;
    }
    if (facing_opponent == 1) {
        likelihood = (drone->difficulty_index - 4) * 5 + 55;
        if (drone->big_boss_stage == 4 || get_game_state() == 3) {
            likelihood = 70;
        }
        if (drone->opponent_distance > 14.6f) {
            likelihood += 20;
        }
        if (randu0(100) < likelihood) {
            drone->attack_latched = 1;
            plyr_pdata->blocking_disable_tick_1 =
                game_tick_ctr +
                (int)(10.0f * inverse_game_speed + 0.5f);
            return 1;
        }
        return 0;
    }

    likelihood = (drone->difficulty_index - 3) * 3 + 76;
    if (drone->difficulty_index < 2) {
        likelihood -= 20;
    }
    if (drone->movement_state == 2) {
        likelihood += 5;
    }
    if (drone->big_boss_stage == 3) {
        likelihood += 5;
    }
    if (randu0(100) < likelihood) {
        return 0;
    }
    if (drone->big_boss_stage == 4) {
        return 0;
    }
    drone->attack_latched = 1;
    plyr_pdata->blocking_disable_tick_1 =
        game_tick_ctr +
        (int)(10.0f * inverse_game_speed + 0.5f);
    return 1;
}

static inline int ai_reset_command_state(void) {
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
    return 0;
}

static AiFightstyleAttack* get_special_move(void) {
    DroneAI* drone;
    unsigned int command;
    int function_index;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    command = drone->ai_command[drone->ai_command_arg];
    if ((command & 0xFFF00000) == 0x40000000) {
        drone->ai_command_flag0 = command & 0xFFFFF;
        advance_cur_cmd_idx();
        command = drone->ai_command[drone->ai_command_arg];
    }
    if ((command & 0xFFF00000) == 0x80000000) {
        drone->ai_command_flag1 = command & 0xFFFFF;
        advance_cur_cmd_idx();
        command = drone->ai_command[drone->ai_command_arg];
    }
    if ((command & 0xFFF00000) == 0x00100000) {
        drone->ai_command_flag2 = command & 0xFFFFF;
        advance_cur_cmd_idx();
        command = drone->ai_command[drone->ai_command_arg];
    }
    if ((command & 0xFFF00000) != 0x04000000) {
        ai_reset_command_state();
        return 0;
    }
    if (drone->ai_command_flag0 != 0 &&
        is_this_move_disabled_exec(drone->ai_command_flag0) != 0) {
        ai_reset_command_state();
        return 0;
    }
    if (drone->ai_command_flag1 != 0 &&
        drone->player->his_plyr_pdata->state == drone->ai_command_flag1) {
        ai_reset_command_state();
        return 0;
    }
    if (drone->ai_command_flag2 != 0 &&
        drone->player->his_plyr_pdata->hit_count >=
            drone->ai_command_flag2) {
        ai_reset_command_state();
        return 0;
    }

    advance_cur_cmd_idx();
    function_index = command & 0xFFFFF;
    if (function_index < 1 ||
        function_index > (int)plyr_pdata->cmo->func_count) {
        ai_reset_command_state();
        return 0;
    }
    drone->special_move.opcode = 2;
    drone->special_move.argument = function_index;
    return &drone->special_move;
}

static int drone_ai_change_attack_to_low(DroneAI* drone) {
    unsigned int stage;
    unsigned int random_value;
    unsigned short roll;

    random_value = randu0(100);
    roll = (unsigned short)random_value;
    if (drone->movement_state == 8 &&
        (drone->player->state & 0x100) != 0 &&
        randu0(100) < 80) {
        return 1;
    }

    stage = drone->big_boss_stage;
    if (stage < 2 ||
        (stage == 2 && drone->match_stage < 2)) {
        return 0;
    }
    if (drone->opponent_distance > 2.0f) {
        return 0;
    }
    if ((his_pdata->state & 0x800) != 0) {
        if (stage == 2 && roll < 15) {
            return 1;
        }
        if (stage == 3 && roll < 25) {
            return 1;
        }
        if (stage == 4 && roll < 35) {
            return 1;
        }
    }
    if (stage == 2 && roll < 5) {
        return 1;
    }
    if (stage == 3 && roll < 10) {
        return 1;
    }
    if (stage == 4 && roll < 15) {
        return 1;
    }
    return 0;
}

int drone_ai_check_escape_restrictions(void) {
    int has_clearance;

    /* Retail performs this lookup before evaluating the clearance helper. */
    get_player_number(plyr_obj);
    if (ai_backward_clearance() > 2.1336f) {
        has_clearance = 1;
    } else {
        has_clearance = 0;
    }
    if (has_clearance == 1) {
        return 1;
    }
    return 0;
}

int drone_ai_check_reversal_restrictions(void) {
    int state;

    state = his_pdata->state;
    if (state == 0x120B) {
        return 0;
    }
    if (state == 0x120C) {
        return 0;
    }
    if (state == 0x1210) {
        return 0;
    }
    if (state == 0x1211) {
        return 0;
    }
    if (state == 0x3204) {
        return 0;
    }
    if (state == 0x3205) {
        return 0;
    }
    if (is_he_airborn() == 1) {
        return 0;
    }
    return is_weapon_style(his_pdata->fighter_definition) != 0;
}

int drone_ai_check_taunt_restrictions(void) {
    return 1;
}

/* Soft ceiling: exact size, CFG, state tests, widths, and return behavior.
 * Residue is a consistent r3/r4 swap for the opponent pointer and state. */
int drone_ai_check_throw_restrictions(void) {
    int allowed = 1;
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
    state = his_pdata->state;
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
    if (his_pdata->throw_restriction == 3) {
        allowed = 0;
    }
    if ((state & 0x400) != 0) {
        allowed = 0;
    }
    return allowed;
}

int drone_ai_check_charge_up_restrictions(void) {
    return plyr_pdata->charge_up_disabled_until <= game_tick_ctr;
}

/* Soft ceiling: algorithm, CFG, signedness, tables, calls, and every retail
 * opcode match. MWCC emits two extra result moves around random adjustments;
 * the remaining diff is GPR coloring and relocation labeling. */
static int drone_ai_should_be_attacking(DroneAI* drone, int* attack_state,
                                        int force) {
    unsigned int attack_chance;
    int can_attack;

    if ((g_DroneOverrideInfo.flags & 0x10) != 0) {
        return 0;
    }
    if (plyr_pdata->action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else if (ai_watcher_can_act() == 0) {
        can_attack = 0;
    } else {
        can_attack = 1;
    }
    if (can_attack == 0) {
        return 0;
    }

    *attack_state = 0;
    attack_chance = g_likelihoodOfAttacking[drone->difficulty_index];
    if (drone->movement_state == 2) {
        attack_chance -= 10;
        if (drone->difficulty_index > 3) {
            attack_chance -= 10;
        }
        attack_chance = attack_chance < 60 ? 60 : attack_chance;
    } else if (drone->movement_state == 1 && drone->difficulty_index > 1) {
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
    if (randu0(1000) < attack_chance || force == 1) {
        unsigned int distance_chance;
        int chance;
        float attack_distance;

        attack_distance = is_big_boss(drone->player) ? 6.5670843f : 5.837408f;
        chance = g_likelihoodOfComboAttacking[drone->difficulty_index];
        if (drone->movement_state == 7) {
            chance = 0;
        } else {
            if (drone->movement_state == 2) {
                chance -= 2;
            }
            if (drone->movement_state == 5 || drone->movement_state == 6) {
                chance += 1;
                if (drone->difficulty_index > 5) {
                    chance += 2;
                }
            }
            if (randu0(100) < 50) {
                chance += randu0(2);
            } else {
                chance -= randu0(2);
            }
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

        chance = g_likelihoodOfPopUpAttacking[drone->difficulty_index];
        if (drone->movement_state == 7) {
            chance = 0;
        } else {
            if (drone->movement_state == 2) {
                chance -= 2;
            }
            if (drone->movement_state == 5 || drone->movement_state == 6) {
                chance += 1;
                if (drone->difficulty_index > 5) {
                    chance += 2;
                }
            }
            if (randu0(100) < 50) {
                chance += randu0(2);
            } else {
                chance -= randu0(2);
            }
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

        chance = g_likelihoodOfSpecialAttacking[drone->difficulty_index];
        if ((g_DroneOverrideInfo.flags & 4) != 0) {
            chance = 0;
        } else {
            if (drone->movement_state == 2) {
                chance -= 5;
            }
            if (drone->movement_state == 1) {
                chance += 3;
            }
            if (chance < 0) {
                chance = 1;
            }
            if (randu0(100) < 50) {
                chance += randu0(3);
            } else {
                chance -= randu0(3);
            }
            if (chance < 0) {
                chance = 1;
            }
            if (drone->big_boss_stage == 4) {
                chance -= 4;
            }
        }
        if ((randu0(100) < (unsigned int)chance ||
             drone->force_attack == 1) &&
            ((his_pdata->state & 0x800) == 0 || randu0(100) < 20)) {
            *attack_state = 2;
            if (drone->difficulty_index < 2 && drone->match_stage == 0) {
                *attack_state = 0;
            }
        }
        if (drone->opponent_distance > 6.74f &&
            drone->opponent_distance < 11.0f && !is_big_boss(drone->player)) {
            distance_chance = drone->movement_state == 3 ? 2 : 7;
            if (randu0(100) < distance_chance && drone->difficulty_index < 7) {
                *attack_state = 3;
            }
        }
        return 1;
    }
    drone->force_attack = 0;
    return 0;
}

static int drone_ai_should_evade_attack(DroneAI* drone) {
    unsigned int likelihood;
    int can_attack;

    if (plyr_pdata->action_lock_b > game_tick_ctr) {
        can_attack = 0;
    } else {
        if (plyr_pdata->action_lock_a > game_tick_ctr) {
            can_attack = 0;
        } else if (plyr_pdata->action_lock_b > game_tick_ctr) {
            can_attack = 0;
        } else if (plyr_pdata->push_blocked != 0 ||
                   (plyr_pdata->state & 0x200) != 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
        if (can_attack == 0) {
            can_attack = 0;
        } else {
            can_attack = 1;
        }
    }
    if (can_attack == 0) {
        return 0;
    }
    likelihood =
        g_likelihoodOfEvadeAttacking[drone->difficulty_index];
    if (randu0(100) < 50) {
        likelihood += randu0(25);
    } else {
        likelihood -= randu0(25);
    }
    if (is_big_boss(drone->player)) {
        likelihood += 100;
    }
    if (randu0(1000) < likelihood) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: state weighting, 0x4C character-row search, sentinel handling,
 * calls, and later CFG agree. Residue is r6/r8 coloring and MWCC's inline
 * character-id preheader versus retail's out-of-line rotated preheader; current
 * code is 12 bytes shorter. */
int drone_ai_fetch_next_AIState(DroneAI* drone) {
    GameInfo* game;
    unsigned int total;
    unsigned short roll;
    int state;

    game = &g_game_info;
    total = 0;

    if (game->field_204 < 10 &&
        drone->reaction_scale < 0.05f &&
        (drone->movement_state == 0 ||
         drone->movement_state == 3 ||
         drone->movement_state == 2) &&
        drone->difficulty_index > 2) {
        drone->charge_cooldown_tick = exec_tick_ctr;
    }
    if (drone->charge_cooldown_tick > exec_tick_ctr) {
        return drone->movement_state;
    }

    roll = randu0(100);
    for (state = 0; state < 9; state++) {
        int difficulty_group;
        int character_state_index;
        int weight;

        difficulty_group = 0;
        character_state_index = 0;
        if (drone->match_stage == 0) {
            weight = g_likelihoodOfChangingStateE3FingEasyLevel[state];
        } else if (drone->difficulty_index == 0) {
            weight = g_likelihoodOfChangingStateFingEasyLevel[state];
        } else if (drone->difficulty_index < 3) {
            weight = g_likelihoodOfChangingStateEasyLevel[state];
        } else if (drone->difficulty_index > 4 &&
                   drone->big_boss_stage == 4) {
            weight = g_likelihoodOfChangingStateMAXLevel[state];
        } else {
            if (drone->difficulty_index > 5) {
                difficulty_group = 1;
            }
            for (;;) {
                if (g_likelihoodOfPCHRChangingState[character_state_index]
                        .character_id ==
                    drone->player->character_id) {
                    weight =
                        g_likelihoodOfPCHRChangingState[character_state_index]
                            .weights[difficulty_group][state];
                    break;
                }
                if (g_likelihoodOfPCHRChangingState[character_state_index]
                        .character_id == -1) {
                    weight =
                        g_likelihoodOfPCHRChangingState[character_state_index]
                            .weights[difficulty_group][state];
                    break;
                }
                character_state_index++;
            }
        }
        total += weight;
        if (roll < total) {
            drone->charge_cooldown_tick = randu0(3) * 60 + 120;
            if (state == 0 && drone->difficulty_index > 2) {
                drone->charge_cooldown_tick -= randu0(60);
            } else if (state == 7) {
                drone->charge_cooldown_tick += randu0(60) + 60;
            } else if (state == 1 || state == 6 || state == 5) {
                drone->charge_cooldown_tick += randu0(60) + 60;
                if (drone->difficulty_index > 5) {
                    drone->charge_cooldown_tick += randu0(2) * 60 + 30;
                }
            }
            if (drone->charge_cooldown_tick < 60) {
                drone->charge_cooldown_tick = 60;
            }
            drone->charge_cooldown_tick += exec_tick_ctr;
            if (game->field_204 < 15 &&
                state != 1 && state != 5 &&
                state != 7 && state != 6 &&
                drone->reaction_scale < 0.05f &&
                drone->difficulty_index > 2) {
                state = 1;
            }
            if (get_game_state() == 3 &&
                (state == 0 || state == 2)) {
                state = 1;
            }
            return state;
        }
    }
    return drone->movement_state;
}

/* Soft ceiling: algorithm, guard semantics, types, calls, and all substantive
 * operations match. Current code retains one extra li/b for a local zero
 * return where retail branches directly to the shared final zero return. */
int drone_ai_should_be_blocking(DroneAI* drone, int reaction) {
    Vec facing;
    Vec to_opponent;
    unsigned int likelihood;
    int is_facing_away;
    int facing_opponent;

    facing_opponent = 0;
    if (plyr_pdata->blocking_disable_tick_1 > game_tick_ctr ||
        plyr_pdata->blocking_disable_tick_2 > game_tick_ctr ||
        plyr_pdata->blocking_disabled == 1) {
        return 0;
    }
    if (am_i_airborn() != 0) {
        return 0;
    }
    if ((plyr_pdata->state & 0x200) != 0) {
        return 0;
    }
    if ((his_pdata->state & 0x1000) == 0 ||
        his_pdata->throw_restriction == 3) {
        if ((his_pdata->secondary_state & 0x100) == 0) {
            return 0;
        }
    }
    plyr_pdata->opponent_attack_counter = his_pdata->attack_counter;
    plyr_pdata->opponent_attack_counter_copy = his_pdata->attack_counter;
    if (drone->big_boss_stage < 4 &&
        drone->block_retry_tick > game_tick_ctr) {
        return 0;
    }
    if (drone->opponent_distance < 9.0f) {
        uv_to_opponent(&to_opponent);
        uv_from_angle_y(&facing, his_obj->ang.y);
        is_facing_away =
            to_opponent.x * facing.x +
                to_opponent.z * facing.z >
            -0.86f;
        if (is_facing_away != 0) {
            facing_opponent = 0;
        } else {
            facing_opponent = 1;
        }
    }
    if (facing_opponent == 1 ||
        (his_pdata->secondary_state & 0x100) != 0) {
        if (g_DroneOverrideInfo.likelihood_scale == 0.0f) {
            return 0;
        }
        if (reaction == 1) {
            return 1;
        }
        if (his_pdata->repeated_action_count >
            (int)(randu0(3) + 2)) {
            if (drone->big_boss_stage == 0) {
                if (drone->match_stage > 5 && randu0(100) < 50) {
                    return 1;
                }
            } else if (drone->match_stage < 3 &&
                       drone->big_boss_stage <= 2) {
                if (randu0(100) < 30) {
                    return 1;
                }
            } else {
                if (drone->difficulty_index > 1 &&
                    randu0(100) < 90) {
                    return 1;
                }
                if (randu0(100) < 50) {
                    return 1;
                }
            }
        }
        likelihood = handicap_calc_likelihood_of_blocking(drone);
        if (randu0(100) < likelihood) {
            if ((unsigned int)drone->hit_active >=
                (unsigned int)(
                    g_maximumNumBlocksInARow[drone->difficulty_index] +
                    randu0(2))) {
                return 0;
            }
            drone->hit_active++;
            return 1;
        }
    }
    return 0;
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
        unsigned short adjustment;

        adjustment = randu0(10);
        likelihood += adjustment;
    } else {
        unsigned short adjustment;

        adjustment = randu0(20);
        likelihood -= adjustment;
    }
    if (likelihood < 0) {
        likelihood = 0;
    }
    if (randu0(100) < (unsigned int)likelihood) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: exact size, CFG, calls, accesses, and computed attack address.
 * The sole remaining difference is the final pointer store using r0 instead
 * of retail's r3. */
static int drone_ai_process_scripted_cmd(void) {
    DroneAI* drone;
    DroneAI* active_drone;
    MkProc* player_proc;
    CmdScript* script;
    AiFightstyleAttackTable* attacks;
    AiFightstyleAttack* fightstyle_attacks;
    unsigned int command_kind;
    int command;
    int argument;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (aproc->pid != 0x100A && aproc->pid != 0x100B) {
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        active_drone->ai_command = 0;
        active_drone->ai_command_arg = 0;
        active_drone->ai_command_target = -1;
        active_drone->ai_command_value = 0.0f;
        active_drone->ai_command_flag0 = 0;
        active_drone->ai_command_flag1 = 0;
        active_drone->ai_command_flag2 = 0;
        active_drone->command_active = 0;
        return 0;
    }

    command = (int)drone->ai_command[drone->ai_command_arg];
    command_kind = command & 0xFFF00000;
    if (drone->request_active == 1) {
        return 0;
    }

    switch (command_kind) {
    case 0x20000000:
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        active_drone->ai_command = 0;
        active_drone->ai_command_arg = 0;
        active_drone->ai_command_target = -1;
        active_drone->ai_command_value = 0.0f;
        active_drone->ai_command_flag0 = 0;
        active_drone->ai_command_flag1 = 0;
        active_drone->ai_command_flag2 = 0;
        active_drone->command_active = 0;
        return 0;
    case 0x02000000:
        drone->ai_command_value = (float)(command & 0xFFFFF);
        break;
    case 0x08000000:
        drone->walk_ticks = (float)(command & 0xFFFFF);
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? (MkProc*)g_game_info.plyr0.idle_proc
                          : (MkProc*)g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, walk_forward_attackdist_close_with_jexit);
        active_drone->request_active = 1;
        break;
    case 0x10000000:
        drone->walk_ticks = (float)(command & 0xFFFFF);
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? (MkProc*)g_game_info.plyr0.idle_proc
                          : (MkProc*)g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, walk_backward_walk_ticks_jexit);
        active_drone->request_active = 1;
        break;
    case 0x01000000:
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? (MkProc*)g_game_info.plyr0.idle_proc
                          : (MkProc*)g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, drone_ai_scripted_change_style);
        active_drone->request_active = 1;
        break;
    case 0x40000000:
        drone->ai_command_flag0 = command & 0xFFFFF;
        advance_cur_cmd_idx();
        return 1;
    case 0x80000000:
        drone->ai_command_flag1 = command & 0xFFFFF;
        advance_cur_cmd_idx();
        return 1;
    case 0x00100000:
        drone->ai_command_flag2 = command & 0xFFFFF;
        advance_cur_cmd_idx();
        return 1;
    case 0x04000000:
        player_proc = get_player_number(plyr_obj) == 0
                          ? (MkProc*)g_game_info.plyr0.idle_proc
                          : (MkProc*)g_game_info.plyr1.idle_proc;
        script = get_cmdscript_for_proc(player_proc);
        if (script == 0) {
            active_drone = get_player_number(plyr_obj) == 0
                               ? &g_DroneAI1 : &g_DroneAI2;
            active_drone->ai_command = 0;
            active_drone->ai_command_arg = 0;
            active_drone->ai_command_target = -1;
            active_drone->ai_command_value = 0.0f;
            active_drone->ai_command_flag0 = 0;
            active_drone->ai_command_flag1 = 0;
            active_drone->ai_command_flag2 = 0;
            active_drone->command_active = 0;
            return 0;
        }
        if ((drone->ai_command_flag0 == 0 ||
             is_this_move_disabled_exec(drone->ai_command_flag0) == 0) &&
            (drone->ai_command_flag1 == 0 ||
             drone->player->his_plyr_pdata->state ==
                 drone->ai_command_flag1) &&
            (drone->ai_command_flag2 == 0 ||
             drone->player->his_plyr_pdata->hit_count <
                 drone->ai_command_flag2)) {
            argument =
                drone->ai_command[drone->ai_command_arg] & 0xFFFFF;
            if (argument < 1 ||
                argument > (int)plyr_pdata->cmo->func_count) {
                active_drone = get_player_number(plyr_obj) == 0
                                   ? &g_DroneAI1 : &g_DroneAI2;
                active_drone->ai_command = 0;
                active_drone->ai_command_arg = 0;
                active_drone->ai_command_target = -1;
                active_drone->ai_command_value = 0.0f;
                active_drone->ai_command_flag0 = 0;
                active_drone->ai_command_flag1 = 0;
                active_drone->ai_command_flag2 = 0;
                active_drone->command_active = 0;
                return 0;
            }
            script->unk28 = argument;
            active_drone = get_player_number(plyr_obj) == 0
                               ? &g_DroneAI1 : &g_DroneAI2;
            player_proc = get_player_number(plyr_obj) == 0
                              ? (MkProc*)g_game_info.plyr0.idle_proc
                              : (MkProc*)g_game_info.plyr1.idle_proc;
            xfer_proc(player_proc, drone_ai_scripted_special_attack);
            active_drone->request_active = 1;
        }
        break;
    case 0:
        if (command < 0 || (unsigned int)command >= 25) {
            active_drone = get_player_number(plyr_obj) == 0
                               ? &g_DroneAI1 : &g_DroneAI2;
            active_drone->ai_command = 0;
            active_drone->ai_command_arg = 0;
            active_drone->ai_command_target = -1;
            active_drone->ai_command_value = 0.0f;
            active_drone->ai_command_flag0 = 0;
            active_drone->ai_command_flag1 = 0;
            active_drone->ai_command_flag2 = 0;
            active_drone->command_active = 0;
            return 0;
        }
        if (drone_ai_check_continue_combo() == 0) {
            active_drone = get_player_number(plyr_obj) == 0
                               ? &g_DroneAI1 : &g_DroneAI2;
            active_drone->ai_command = 0;
            active_drone->ai_command_arg = 0;
            active_drone->ai_command_target = -1;
            active_drone->ai_command_value = 0.0f;
            active_drone->ai_command_flag0 = 0;
            active_drone->ai_command_flag1 = 0;
            active_drone->ai_command_flag2 = 0;
            active_drone->command_active = 0;
            return 0;
        }
        attacks = (AiFightstyleAttackTable*)
            plyr_pdata->fighter_definition->move_blend_data;
        fightstyle_attacks = attacks->attacks;
        drone->script_attack = &fightstyle_attacks[command];
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        player_proc = get_player_number(plyr_obj) == 0
                          ? (MkProc*)g_game_info.plyr0.idle_proc
                          : (MkProc*)g_game_info.plyr1.idle_proc;
        xfer_proc(player_proc, drone_ai_scripted_attack);
        active_drone->request_active = 1;
        break;
    default:
        active_drone = get_player_number(plyr_obj) == 0
                           ? &g_DroneAI1 : &g_DroneAI2;
        active_drone->ai_command = 0;
        active_drone->ai_command_arg = 0;
        active_drone->ai_command_target = -1;
        active_drone->ai_command_value = 0.0f;
        active_drone->ai_command_flag0 = 0;
        active_drone->ai_command_flag1 = 0;
        active_drone->ai_command_flag2 = 0;
        active_drone->command_active = 0;
        return 0;
    }
    advance_cur_cmd_idx();
    return 1;
}

static float drone_ai_scripted_special_attack(void) {
    pre_attack_chores();
    plyr_going_to_attack_with_action(active_cmdscript->unk28);
    share_my_attack_info(2.0f, 0.3f);
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->cmo);
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static float drone_ai_scripted_change_style(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone_ai_force_change_style(drone, -1);
    ((AiProcVtable*)aproc->vtbl)->transfer(drone_entry, 0.0f);
    return 0.0f;
}

static float drone_ai_scripted_attack(void) {
    DroneAI* drone;
    AiFightstyleAttack* attack;
    unsigned int attack_index;
    int is_special_attack;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    attack = (AiFightstyleAttack*)drone->script_attack;
    if (attack == 0) {
        blend_to_stance(0.1f);
        AI_TRANSFER(j_exit);
        return 0.0f;
    }
    if (am_i_airborn() == 0) {
        pre_attack_chores();
        if (attack->opcode == 0 &&
            ((attack_index =
                  attack -
                  ((AiFightstyleAttackTable*)plyr_pdata->fighter_definition
                       ->move_blend_data)
                      ->attacks) == 12 ||
             attack_index == 17 || attack_index == 2)) {
            is_special_attack = 1;
        } else {
            is_special_attack = 0;
        }
        if (is_special_attack) {
            set_my_state(0x1300);
        }
        switch (attack->opcode) {
        case 0:
            cmdscript_reset_stack();
            cmdscript_setup_execution(
                plyr_pdata->fighter_definition->cmo, attack->argument);
            call_player_script_function(plyr_pdata->fighter_definition->cmo);
            break;
        case 1:
            AI_TRANSFER(attack->entry);
            break;
        case 2:
            cmdscript_reset_stack();
            cmdscript_setup_execution(plyr_pdata->cmo, attack->argument);
            call_player_script_function(plyr_pdata->cmo);
            break;
        case 4:
            cmdscript_reset_stack();
            cmdscript_setup_execution(reactions_cmo, attack->argument);
            call_player_script_function(reactions_cmo);
            break;
        }
    }
    AI_TRANSFER(j_exit);
    return 0.0f;
}

static inline int ai_combo_breakout_likelihood(DroneAI* drone) {
    int likelihood;

    likelihood = g_likelihoodForComboBreakout[drone->difficulty_index];
    if (drone->movement_state == 5) {
        likelihood -= 10;
    }
    if (drone->big_boss_stage == 3) {
        likelihood += 10;
    }
    if (drone->big_boss_stage == 4) {
        likelihood += 25;
    }
    if (drone->match_stage == 0) {
        likelihood = 0;
    }
    if (get_game_state() == 3) {
        likelihood += 25;
    }
    if (likelihood < 0) {
        likelihood = 0;
    }
    return likelihood;
}

/* Soft ceiling: reset behavior, likelihood calculations, command masking, and
 * ABI match. Current structured C materializes/rechecks one false breakout
 * value where retail branches directly into the second-distance attempt. */
int drone_ai_check_switching_to(int command) {
    DroneAI* drone;
    Vec facing_direction;
    Vec opponent_direction;
    int likelihood;
    int should_break_out;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        ai_reset_command_state();
        drone->super_combo_active = 0;
        return 0;
    }
    if ((g_DroneOverrideInfo.flags & 2) != 0) {
        ai_reset_command_state();
        drone->super_combo_active = 0;
        return 0;
    }

    if ((his_pdata->state & 0x800) != 0 &&
        his_pdata->state != 0xA00 && randu0(100) < 5) {
        should_break_out = 1;
    } else {
        uv_to_opponent(&opponent_direction);
        uv_from_angle_y(&facing_direction, plyr_obj->ang.y);
        if (opponent_direction.x * facing_direction.x +
                opponent_direction.z * facing_direction.z <
            0.86f) {
            should_break_out = 1;
        } else {
            should_break_out = 0;
        }
        if (should_break_out == 1) {
            likelihood = ai_combo_breakout_likelihood(drone);
            if ((int)randu0(100) < likelihood) {
                should_break_out = 1;
            } else {
                should_break_out = 0;
            }
        }
        if (should_break_out == 0 &&
            drone->opponent_distance > 5.9457946f) {
            likelihood = ai_combo_breakout_likelihood(drone);
            if ((int)randu0(100) < likelihood + 30) {
                should_break_out = 1;
            }
        }
    }

    if (should_break_out == 1) {
        ai_reset_command_state();
        drone->super_combo_active = 0;
        return 0;
    }
    if (drone->ai_command != 0 &&
        command ==
            (int)(drone->ai_command[drone->ai_command_arg] & 0xFEFFFFFF)) {
        drone->super_combo_active = 0;
        advance_cur_cmd_idx();
        return 1;
    }
    return 0;
}

/* Soft ceiling: signed base-5 division, guards, reset stores, and ABI match.
 * The typed inline reset result recovers retail's return-register lifetime;
 * current MWCC retains one extra branch after the second reset expansion. */
int drone_ai_check_button_direction(int direction) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        return ai_reset_command_state();
    }
    if (drone->ai_command != 0 &&
        direction == (int)drone->ai_command[drone->ai_command_arg] % 5) {
        if (drone_ai_check_continue_combo() != 0) {
            return 1;
        }
        return ai_reset_command_state();
    }
    return 0;
}

/* Soft ceiling: exact size, switch CFG, signed division, reset stores, and ABI.
 * Residue is caller-saved GPR coloring plus address/result scheduling. */
int drone_ai_check_button_press(int button) {
    DroneAI* drone;
    int command_button;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        return ai_reset_command_state();
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
        default:
            command_button = -2;
            break;
        }
        if (command_button ==
            ((int)drone->ai_command[drone->ai_command_arg] / 5) * 5) {
            if (drone_ai_check_continue_combo() != 0) {
                return 1;
            }
            return ai_reset_command_state();
        }
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

void drone_ai_get_min_time_in_block(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    handicap_calc_min_time_in_block(drone);
}

/* Soft ceiling: exact size, CFG, operations, and signed likelihood handling.
 * The residual diff is an r3/r4 allocation swap and relocation labels. */
int drone_ai_check_block_fakeout(void) {
    DroneAI* drone;
    int likelihood;
    int block_hits;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    block_hits = plyr_pdata->block_hit_count;
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
        }
        if (drone->big_boss_stage == 3) {
            likelihood -= 10;
        }
        if (likelihood < 0) {
            likelihood = 0;
        }
    }
    if (randu0(100) < (unsigned int)likelihood) {
        return 1;
    }
    return 0;
}

void drone_ai_hit(void) {
    DroneAI* drone;

    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->hit_active = 0;
}

#pragma dont_inline on
/* Soft ceiling: exact size, instructions, CFG, and cached action-lock use.
 * Residue is only r4/r6 coloring for the lock and tick values. */
int drone_ai_can_push(DroneAI* drone) {
    PlyrPdata* action;
    int can_push;

    action = plyr_pdata;
    if (action->action_lock_b > game_tick_ctr) {
        can_push = 0;
    } else {
        if (action->action_lock_a > game_tick_ctr) {
            can_push = 0;
        } else if (action->action_lock_b > game_tick_ctr) {
            can_push = 0;
        } else if (action->push_blocked != 0 ||
                   (action->state & 0x200) != 0) {
            can_push = 0;
        } else {
            can_push = 1;
        }
        if (can_push == 0) {
            can_push = 0;
        } else {
            can_push = 1;
        }
    }
    if (can_push == 0) {
        return 0;
    }
    switch (drone->player->character_id) {
    case 0:
        return 1;
    default:
        return 0;
    }
}
#pragma dont_inline reset

























#pragma dont_inline on
/* Soft ceiling: algorithm, CFG, calls, widths, ABI, and typed layouts match.
 * Current MWCC coalesces retail's table-base addi/lwz into lwzu; remaining
 * differences are base/index scheduling and GPR coloring. */
static AiFightstyleAttack* get_random_fightstyle_attack(
    PlyrFighterDefinition* fighter, int attack_group, int flags) {
    DroneAI* drone;
    FighterAiTable* tables;
    FighterAiTable* table;
    FighterAiMoveRow* row;
    AiFightstyleAttack* attacks;
    unsigned int command_kind;
    int command_index;
    int command_target;
    int command;
    int accepted;
    int attempts;
    DroneAI* reset_drone;

    command_index = 0;
    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    tables = fighter->move_blend_data->ai_tables;
    table = &tables[attack_group];
    command_target = -1;
    if (table->usable_row_count == 0) {
        return 0;
    }

    accepted = 0;
    attempts = 0;
    for (;;) {
        row = &table->rows[
            get_random_fightstyle_index(attack_group, table, flags)];
        command = row->commands[0];
        command_kind = command & 0xFFF00000;
        if (command_kind == 0) {
            accepted = 1;
            break;
        } else if (command_kind == 0x20000000 &&
                   (command & 0xFFFFF) == drone->player->character_id) {
            command_target = command & 0xFFFFF;
            command_index = 1;
            accepted = 1;
            break;
        }
        attempts++;
        if (attempts >= 3) {
            break;
        }
    }

    if (!accepted) {
        reset_drone = get_player_number(plyr_obj) == 0
                          ? &g_DroneAI1 : &g_DroneAI2;
        reset_drone->ai_command = 0;
        reset_drone->ai_command_arg = 0;
        reset_drone->ai_command_target = -1;
        reset_drone->ai_command_value = 0.0f;
        reset_drone->ai_command_flag0 = 0;
        reset_drone->ai_command_flag1 = 0;
        reset_drone->ai_command_flag2 = 0;
        reset_drone->command_active = 0;
        return 0;
    }
    if (row->commands[command_index] == -1) {
        reset_drone = get_player_number(plyr_obj) == 0
                          ? &g_DroneAI1 : &g_DroneAI2;
        reset_drone->ai_command = 0;
        reset_drone->ai_command_arg = 0;
        reset_drone->ai_command_target = -1;
        reset_drone->ai_command_value = 0.0f;
        reset_drone->ai_command_flag0 = 0;
        reset_drone->ai_command_flag1 = 0;
        reset_drone->ai_command_flag2 = 0;
        reset_drone->command_active = 0;
        return 0;
    }

    drone->ai_command = (unsigned int*)row;
    drone->ai_command_arg = command_index;
    drone->ai_command_target = command_target;
    drone->ai_command_flag0 = 0;
    drone->ai_command_flag1 = 0;
    drone->ai_command_flag2 = 0;
    command = row->commands[command_index];
    command_kind = command & 0xFFF00000;
    if (command_kind == 0x04000000 ||
        command_kind == 0x40000000 ||
        command_kind == 0x80000000 ||
        command_kind == 0x00100000) {
        return get_special_move();
    }
    if (command_kind != 0) {
        reset_drone = get_player_number(plyr_obj) == 0
                          ? &g_DroneAI1 : &g_DroneAI2;
        reset_drone->ai_command = 0;
        reset_drone->ai_command_arg = 0;
        reset_drone->ai_command_target = -1;
        reset_drone->ai_command_value = 0.0f;
        reset_drone->ai_command_flag0 = 0;
        reset_drone->ai_command_flag1 = 0;
        reset_drone->ai_command_flag2 = 0;
        reset_drone->command_active = 0;
        return 0;
    }
    command = (int)drone->ai_command[command_index];
    if (command < 0 || command >= 25) {
        reset_drone = get_player_number(plyr_obj) == 0
                          ? &g_DroneAI1 : &g_DroneAI2;
        reset_drone->ai_command = 0;
        reset_drone->ai_command_arg = 0;
        reset_drone->ai_command_target = -1;
        reset_drone->ai_command_value = 0.0f;
        reset_drone->ai_command_flag0 = 0;
        reset_drone->ai_command_flag1 = 0;
        reset_drone->ai_command_flag2 = 0;
        reset_drone->command_active = 0;
        return 0;
    }
    advance_cur_cmd_idx();
    attacks = ((AiFightstyleAttackTable*)
                   plyr_pdata->fighter_definition->move_blend_data)
                  ->attacks;
    return &attacks[command];
}

static int get_random_fightstyle_index(
    int attack_group, FighterAiTable* table, int selection_mode) {
    DroneAI* drone;
    float lower_scale;
    float upper_scale;
    unsigned int roll;
    int count;
    unsigned int lower;
    int upper;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    upper = table->usable_row_count;
    lower = 0;
    lower_scale = 0.0f;
    upper_scale = 1.0f;
    roll = randu0(100);
    if (drone->match_stage == 0 ||
        drone->big_boss_stage == 0 ||
        (drone->big_boss_stage == 1 &&
         drone->match_stage < 4)) {
        if (table->usable_row_count > 2) {
            upper = 2;
        }
    } else {
        count = table->usable_row_count;
        if (count < 5) {
            upper = count;
        } else {
            if (drone->difficulty_index < 2) {
                if (roll < 3) {
                    lower_scale = 0.33f;
                    upper_scale = 0.6f;
                } else {
                    upper_scale = 0.5f;
                }
            } else if (drone->difficulty_index < 4) {
                if (roll < 10) {
                    lower_scale = 0.33f;
                    upper_scale = 0.75f;
                } else {
                    upper_scale = 0.5f;
                }
            } else if (drone->difficulty_index < 6) {
                if (roll < 20) {
                    lower_scale = 0.4f;
                    upper_scale = 0.85f;
                } else {
                    lower_scale = 0.2f;
                    upper_scale = 0.75f;
                }
            } else if (drone->difficulty_index < 8) {
                if (roll < 40) {
                    lower_scale = 0.4f;
                } else if (roll < 45) {
                    upper_scale = 0.85f;
                } else {
                    lower_scale = 0.4f;
                    upper_scale = 0.85f;
                }
            } else {
                if (roll < 60) {
                    lower_scale = 0.6f;
                } else if (roll < 65) {
                    upper_scale = 0.85f;
                } else {
                    lower_scale = 0.2f;
                    upper_scale = 0.85f;
                }
            }
            lower = (unsigned int)((float)count * lower_scale);
            upper = (unsigned int)((float)count * upper_scale);

            if (selection_mode == 1) {
                upper += 3;
                lower += 2;
                if (upper > count) {
                    upper = count;
                }
                if ((int)lower > count) {
                    lower = count;
                }
            } else if (selection_mode == 2) {
                lower = 0;
            }
            if (drone->match_stage > 3 &&
                drone->big_boss_stage > 1 &&
                randu0(100) < 5) {
                lower = 0;
                upper = table->usable_row_count;
            }
            if (lower == (unsigned int)upper) {
                if (lower != 0) {
                    lower--;
                }
                if (upper < table->usable_row_count) {
                    upper++;
                }
            }
            if (selection_mode == 3) {
                int final_count;

                final_count = table->usable_row_count;
                upper = final_count;
                if (final_count > 1) {
                    lower = final_count - 2;
                } else {
                    lower = 0;
                }
            }
        }
    }

    if ((unsigned int)upper < lower) {
        return randu0((unsigned short)lower);
    }
    return (int)lower +
           randu0((unsigned short)((unsigned int)upper - lower));
}
#pragma dont_inline off

void drone_ai_increase_big_boss_stage(PlyrPdata* victim) {
    DroneAI* drone;
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

    if (g_game_info.plyr1.slot.pdata == victim) {
        opponent_proc = get_player_number(g_game_info.plyr0.slot.mirror_a) == 0
                            ? (MkProc*)g_game_info.plyr0.idle_proc
                            : (MkProc*)g_game_info.plyr1.idle_proc;
        script = get_cmdscript_for_proc(opponent_proc);
        g_game_info.plyr0.slot.pdata->blocking_disabled = 1;
        g_game_info.plyr0.slot.pdata->blocking_disabled_2 = 1;
    } else if (g_game_info.plyr0.slot.pdata == victim) {
        opponent_proc = get_player_number(g_game_info.plyr1.slot.mirror_a) == 0
                            ? (MkProc*)g_game_info.plyr0.idle_proc
                            : (MkProc*)g_game_info.plyr1.idle_proc;
        script = get_cmdscript_for_proc(opponent_proc);
        g_game_info.plyr1.slot.pdata->blocking_disabled = 1;
        g_game_info.plyr1.slot.pdata->blocking_disabled_2 = 1;
    } else {
        return;
    }
    snd_req(0xCF);
    run_reaction_cleanup_function(victim->his_plyr_pdata);
    script->unk28 = 0x6F;
    xfer_player_proc(opponent_proc, r_call_script_function);
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

int drone_ai_should_passive_state_switch(DroneAI* drone) {
    unsigned short likelihood;

    drone->danger_area_state--;
    if (drone->danger_area_state < 0) {
        if (drone->big_boss_block_state == 1) {
            drone->big_boss_block_state = 0;
            drone->danger_area_state = randu0(5) * 15 + 180;
            drone->decision_ready = 1;
            return 1;
        }
        if (drone->reaction_scale > 0.45f) {
            likelihood =
                g_likelihoodOfPassiveSwitch[drone->difficulty_index];
            if (randu0(100) < likelihood) {
                drone->danger_area_state = randu0(5) * 15 + 120;
                drone->big_boss_block_state = 1;
                drone->decision_ready = 0;
            } else {
                drone->big_boss_block_state = 0;
                drone->danger_area_state = randu0(5) * 15 + 120;
                drone->decision_ready = 1;
            }
            return 1;
        }
        if (drone->difficulty_index < 7 &&
            (drone->reaction_scale > -0.3f ||
             drone->player_health > 0.5f)) {
            if (randu0(100) < 4) {
                drone->danger_area_state = randu0(5) * 15 + 80;
                drone->big_boss_block_state = 1;
                drone->decision_ready = 0;
            }
        } else if (drone->difficulty_index < 5 &&
                   randu0(100) < 3) {
            drone->danger_area_state = randu0(5) * 10 + 80;
            drone->big_boss_block_state = 1;
            drone->decision_ready = 0;
        }
    }
    return 0;
}

/* Soft ceiling: exact size, CFG, arithmetic, field accesses, and calls.
 * Residue is one r3/r4 swap between match_stage and its multiplier. */
static unsigned int handicap_calc_likelihood_of_blocking_in_reaction(
    DroneAI* drone) {
    int likelihood;

    likelihood =
        g_likelihoodOfBlockingInReaction[drone->difficulty_index];
    if (drone->movement_state == 1) {
        likelihood -= 2;
    }
    if (drone->movement_state == 2) {
        likelihood += 6;
    }
    if (drone->movement_state == 3) {
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
            int multiplier;

            multiplier = 5;
            if (drone->big_boss_stage <= 2) {
                multiplier = 2;
            }
            likelihood += multiplier * (drone->match_stage + 1);
        } else if (drone->difficulty_index < 4) {
            likelihood += 35;
        } else {
            likelihood += 55;
        }
    }
    if (drone->player->his_plyr_pdata->plyr_info->slot.mirror_a
            ->hide_flag_bits.hidden == 1) {
        if (drone->big_boss_stage == 4) {
            likelihood -= 8;
        } else {
            likelihood /= 2;
        }
    }
    return (unsigned int)(
        (float)likelihood * g_DroneOverrideInfo.likelihood_scale);
}

/* Soft ceiling: exact size, CFG, arithmetic, field accesses, and calls.
 * Residue is one r3/r4 swap between match_stage and its multiplier. */
unsigned int handicap_calc_likelihood_of_blocking(DroneAI* drone) {
    int likelihood;

    likelihood = g_likelihoodOfBlocking[drone->difficulty_index];
    if (drone->movement_state == 1) {
        likelihood -= 5;
    }
    if (drone->movement_state == 2) {
        likelihood += 20;
        if (drone->difficulty_index < 2) {
            likelihood -= 15;
        }
    }
    if (drone->movement_state == 3) {
        likelihood += 10;
    }
    if (is_big_boss(plyr_pdata)) {
        if (drone->big_boss_block_state == 0) {
            likelihood +=
                g_bigBossBlockAdjuster[drone->difficulty_index];
            likelihood -= 5;
            if (likelihood < 0) {
                likelihood = 0;
            }
        } else {
            if (drone->big_boss_stage == 4) {
                likelihood -= 5;
            } else {
                likelihood -= 12;
            }
            if (likelihood < 0) {
                likelihood = 0;
            }
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
    if (drone->big_boss_stage == 3) {
        likelihood += 2;
    }
    if (drone->big_boss_stage == 4) {
        likelihood += 8;
    }
    if (get_game_state() == 3) {
        likelihood -= 6;
    }
    if (his_pdata->state == 0x1219) {
        if (drone->big_boss_stage == 0) {
            likelihood++;
        } else if (drone->match_stage < 3) {
            int stage_multiplier;

            stage_multiplier = 5;
            if (drone->big_boss_stage <= 2) {
                stage_multiplier = 2;
            }
            likelihood += stage_multiplier * (drone->match_stage + 1);
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
            ->slot.mirror_a->hide_flag_bits.hidden == 1) {
        if (drone->big_boss_stage == 4) {
            likelihood -= 8;
        } else {
            likelihood /= 2;
        }
    }
    return (unsigned int)(
        (float)likelihood * g_DroneOverrideInfo.likelihood_scale);
}

int handicap_calc_min_time_in_block(DroneAI* drone) {
    unsigned int roll;
    int minimum_ticks;

    minimum_ticks = g_minTimeInBlock[drone->difficulty_index];
    roll = randu0(100);
    if (drone->big_boss_stage == 0) {
        if (randu0(100) < 33) {
            minimum_ticks += randu0(20) + 30;
        }
    } else if (drone->big_boss_stage == 1) {
        if (randu0(100) < 20) {
            minimum_ticks += randu0(15) + 25;
        }
    } else if (((drone->match_stage < 4 &&
                 drone->big_boss_stage == 2) ||
                drone->match_stage < 2) &&
               randu0(100) < 10) {
        minimum_ticks += randu0(10) + 20;
    }

    if (drone->movement_state == 2 || drone->movement_state == 3) {
        minimum_ticks += randu0(3) + 5;
    }
    if (roll < 25) {
        minimum_ticks += randu0(8) + 4;
    } else if (roll < 50) {
        minimum_ticks += randu0(4) + 8;
        if (minimum_ticks < 12) {
            minimum_ticks = 12;
        }
    }
    if (drone->big_boss_stage > 2) {
        minimum_ticks = (unsigned int)(0.75f * minimum_ticks);
    }
    if (is_big_boss(drone->player)) {
        minimum_ticks += 12;
    }
    if (minimum_ticks < 0) {
        minimum_ticks = 0;
    }
    return minimum_ticks;
}

/* Soft ceiling: retail CFG/types/offsets match; residue is signed divide-by-2
 * lowering, one scheduled table-index load, and stmw/lmw versus scalar saves. */
static int handicap_likelihood_for_combo_breaker(DroneAI* drone) {
    int likelihood;

    likelihood = g_likelihoodForComboBreaker[drone->difficulty_index];
    if (get_game_state() == 3) {
        return 1;
    }
    if (drone->big_boss_stage < 2) {
        return 1;
    }
    if (mode_of_play == 10) {
        if (drone->big_boss_stage == 2) {
            return 20;
        }
        likelihood = 50;
        if (drone->big_boss_stage == 3) {
            likelihood = 35;
        }
        return likelihood;
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
            ->hide_flag_bits.hidden == 1) {
        likelihood += 10;
    }
    return likelihood;
}

/* Soft ceiling: combo-stop CFG, likelihood calls, state guards, and return
 * thresholds match. Current code retains one extra li/b on a zero-result path. */
int drone_ai_check_continue_combo(void) {
    DroneAI* drone;
    Vec facing;
    Vec to_opponent;
    int combo_roll;
    int likelihood;
    int facing_away;
    int stop_combo;

    drone = get_player_number(plyr_obj) == 0
                ? &g_DroneAI1 : &g_DroneAI2;
    combo_roll = randu0(100);
    if (plyr_pdata->drone_request == 0) {
        return 0;
    }
    if ((his_pdata->state & 0x800) != 0 &&
        his_pdata->state != 0xA00 && randu0(100) < 5) {
        stop_combo = 1;
    } else {
        uv_to_opponent(&to_opponent);
        uv_from_angle_y(&facing, plyr_obj->ang.y);
        if (to_opponent.x * facing.x + to_opponent.z * facing.z < 0.86f) {
            facing_away = 1;
        } else {
            facing_away = 0;
        }
        if (facing_away == 1 &&
            (int)randu0(100) < ai_combo_breakout_likelihood(drone)) {
            stop_combo = 1;
        } else if (drone->opponent_distance > 5.9457946f) {
            likelihood = ai_combo_breakout_likelihood(drone);
            if ((int)randu0(100) < likelihood + 30) {
                stop_combo = 1;
            } else {
                stop_combo = 0;
            }
        } else {
            stop_combo = 0;
        }
    }
    if (stop_combo == 1) {
        return 0;
    }
    if (drone->player_health == 0.0f) {
        return 0;
    }
    if (drone->difficulty_index == 0) {
        return 0;
    }
    if (drone->match_stage == 0 && drone->difficulty_index < 5) {
        return 0;
    }
    if (drone->command_active == 1) {
        if (drone->big_boss_stage >= 3) {
            return 1;
        }
        return drone->difficulty_index >= 4;
    }
    if (randu0(100) < 5) {
        return 0;
    }
    if (drone->big_boss_stage == 4) {
        return 1;
    }
    if (drone->difficulty_index <= 2) {
        return (unsigned int)combo_roll < 65U;
    }
    if (drone->difficulty_index <= 5) {
        return (unsigned int)combo_roll < 80U;
    }
    if (drone->difficulty_index <= 7) {
        return (unsigned int)combo_roll < 95U;
    }
    return (unsigned int)combo_roll >= 5U;
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

#pragma dont_inline on
int handicap_get_current_difficulty(DroneAI* drone) {
    short score;
    int difficulty;
    int opponent_rounds;
    unsigned int random_value;
    unsigned short random_roll;

    random_value = randu0(100);
    random_roll = (unsigned short)random_value;
    score = ((int)drone->handicap_stage + 4) * 5;
    if (score > 90) {
        score = 90;
    }
    if (drone->handicap_stage > 5) {
        score += 10;
    }

    if (drone->handicap_setting == 0) {
        score -= 25;
    } else if (drone->handicap_setting == 1) {
        score -= 20;
    } else if (drone->handicap_setting == 2) {
        if (drone->handicap_stage < 5) {
            score -= 10;
        }
    } else if (drone->handicap_setting == 3) {
        score += 20;
    } else if (drone->handicap_setting == 4) {
        score += 50;
    }

    if (drone->match_mode == 1) {
        score -= 10;
    } else if (drone->match_mode == 3 &&
               drone->handicap_stage > 2) {
        score += 10;
    }
    if (drone->reaction_scale < -0.45f) {
        score += (short)(drone->handicap_setting * 5 + 5);
    }
    if (drone->reaction_scale > 0.45f) {
        score -= 10;
        if (score > 40) {
            score = 40;
        }
    }
    if (random_roll < 10) {
        score += 10;
    }
    if (drone->handicap_setting > 2 && score < 40) {
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
    if (drone->handicap_setting == 0 && difficulty > 4) {
        difficulty = 4;
    } else if (drone->handicap_setting == 1 && difficulty > 5) {
        difficulty = 5;
    }
    if (drone->handicap_setting < 4 && difficulty != 0) {
        switch (difficulty) {
        case 1:
        case 2:
            if (drone->consecutive_losses > 1) {
                difficulty = 0;
            }
            break;
        case 3:
        case 4:
        case 5:
            if (drone->consecutive_losses > 8) {
                difficulty = 0;
            } else if (drone->consecutive_losses > 4) {
                difficulty = 1;
            } else if (drone->consecutive_losses > 2) {
                difficulty -= 2;
            } else if (drone->consecutive_losses > 0) {
                difficulty -= 1;
            }
            break;
        case 6:
        case 7:
        case 8:
            if (drone->consecutive_losses > 8) {
                difficulty = 1;
            } else if (drone->consecutive_losses > 5) {
                difficulty -= 4;
            } else if (drone->consecutive_losses > 3) {
                difficulty -= 3;
            } else if (drone->consecutive_losses > 1) {
                difficulty -= 2;
            }
            break;
        }
    }
    if (get_game_state() == 3) {
        drone->difficulty_index = 8;
        drone->handicap_stage = 8;
    }
    return difficulty;
}


#pragma dont_inline off



static inline void ai_big_boss_walk_footstep(void) {
    unsigned short sound;

    sound = randu0(100);
    if (sound < 33) {
        snd_req(0x1B0);
    } else if (sound < 66) {
        snd_req(0x1B1);
    } else {
        snd_req(0x1B2);
    }
    shake_camera(1, 0.01f);
}

static inline void ai_walk_footstep(void) {
    if (is_big_boss(plyr_pdata)) {
        ai_big_boss_walk_footstep();
    } else {
        random_foot(1);
    }
}

void drone_walk_FB_true(
    int (*test)(void), unsigned int ticks, int forward, int finish_start) {
    float elapsed;
    float duration;
    int walk_voice_eligible;

    elapsed = 0.0f;
    duration = (float)ticks;
    init_ground_move_no_aniproc();
    rotate_towards_him(0.2f);
    if (plyr_pdata->character_id == 0x10 &&
        plyr_pdata->plyr_info->flags_14_bits.alternate_costume == 0) {
        walk_voice_eligible = 1;
    } else {
        walk_voice_eligible = 0;
    }
    if (walk_voice_eligible && randu0(100) < 15) {
        snd_req_delay(randu0(5) + 0x27B, randu0(20) + 1);
    }
    if (ticks == 0) {
        blend_to_stance(0.1f);
        return;
    }
    if (test() == 1) {
        blend_to_stance(0.1f);
        return;
    }

    if (forward) {
        set_my_state(0x2000);
        plyr_anim_pdata->flags |= 0x40;
        blend_to_ani(plyr_pdata->fighter_definition->walk_forward_start, 0x23,
                     0.2f);
        plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                   ->walk_forward_start_step;
        plyr_anim_pdata->weight = plyr_pdata->fighter_definition->move_blend_data
                                     ->walk_forward_start_weight;
    } else {
        set_my_state(0x2001);
        plyr_anim_pdata->flags |= 0x40;
        blend_to_ani(plyr_pdata->fighter_definition->walk_backward_start, 0x23,
                     0.2f);
        plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                   ->walk_backward_start_step;
        plyr_anim_pdata->weight = plyr_pdata->fighter_definition->move_blend_data
                                     ->walk_backward_start_weight;
        ani_to_frame_x_call(face_opponent_now,
                            plyr_anim_pdata->high_frame - 14.0f);
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
            blend_to_ani(plyr_pdata->fighter_definition->walk_forward_loop, 0,
                         0.2f);
            plyr_anim_pdata->step = plyr_pdata->fighter_definition
                                       ->move_blend_data->walk_forward_step;
        } else {
            blend_to_ani(plyr_pdata->fighter_definition->walk_backward_loop, 0,
                         0.2f);
            plyr_anim_pdata->step = plyr_pdata->fighter_definition
                                       ->move_blend_data->walk_backward_step;
        }
    }
    while (test() == 0 && elapsed < duration) {
        if (is_big_boss(plyr_pdata) &&
            ((plyr_anim_pdata->frame > 15.8f &&
              plyr_anim_pdata->frame < 16.5f) ||
             (plyr_anim_pdata->frame > 36.8f &&
              plyr_anim_pdata->frame < 37.5f))) {
            ai_big_boss_walk_footstep();
        }
        face_opponent_now();
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        AI_SLEEP(1.0f);
        elapsed += game_speed;
    }
    if (is_big_boss(plyr_pdata)) {
        ai_big_boss_walk_footstep();
    }
    blend_to_stance(0.1f);
}

static inline void ai_big_boss_strafe_footstep(void) {
    unsigned short sound;

    sound = randu0(100);
    if (sound < 33) {
        snd_req(0x1B0);
    } else if (sound < 66) {
        snd_req(0x1B1);
    } else {
        snd_req(0x1B2);
    }
    shake_camera(1, 0.01f);
}

void drone_step_LR_true(
    int (*test)(void), unsigned int ticks, int move_right) {
    unsigned int start_tick;
    int walk_voice_eligible;

    start_tick = exec_tick_ctr;
    rotate_towards_him(0.2f);
    if (plyr_pdata->character_id == 0x10 &&
        !plyr_pdata->plyr_info->flags_14_bits.alternate_costume) {
        walk_voice_eligible = 1;
    } else {
        walk_voice_eligible = 0;
    }
    if (walk_voice_eligible && randu0(100) < 15) {
        unsigned int delay;

        delay = (unsigned short)randu0(20);
        snd_req_delay(randu0(5) + 0x27B, delay + 1);
    }

    init_3d_move_no_aniproc();
    set_my_state(0);
    plyr_anim_pdata->flags |= 0x40;
    if (move_right) {
        if (am_i_flipped() == 0) {
            blend_to_ani(
                plyr_pdata->fighter_definition->strafe_right_start, 3, 0.2f);
        } else {
            blend_to_ani(
                plyr_pdata->fighter_definition->strafe_left_start, 3, 0.2f);
        }
    } else {
        if (am_i_flipped() == 0) {
            blend_to_ani(
                plyr_pdata->fighter_definition->strafe_left_start, 3, 0.2f);
        } else {
            blend_to_ani(
                plyr_pdata->fighter_definition->strafe_right_start, 3, 0.2f);
        }
    }
    plyr_anim_pdata->step =
        plyr_pdata->fighter_definition->move_blend_data->strafe_start_step;
    plyr_anim_pdata->obj_movement_weight =
        plyr_pdata->fighter_definition->move_blend_data->strafe_start_weight;
    plyr_pdata->dodge_sound_played = 0;

    while (test() == 0 &&
           plyr_anim_pdata->frame <
               plyr_pdata->fighter_definition->move_blend_data
                   ->strafe_start_frame) {
        dodge_3d_scan();
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        AI_SLEEP(1.0f);
    }

    if (is_big_boss(plyr_pdata)) {
        disable_this_move_exec(0x6004, 120);
    } else {
        disable_this_move_exec(0x6004, 40);
    }
    if (is_big_boss(plyr_pdata)) {
        ai_big_boss_strafe_footstep();
    } else {
        random_foot(1);
    }

    if (test() == 0 && exec_tick_ctr - start_tick < ticks) {
        set_my_state(0x2003);
        if (move_right) {
            if (am_i_flipped() == 0) {
                blend_to_ani(
                    plyr_pdata->fighter_definition->strafe_right_loop, 0, 0.1f);
            } else {
                blend_to_ani(
                    plyr_pdata->fighter_definition->strafe_left_loop, 0, 0.1f);
            }
        } else {
            if (am_i_flipped() == 0) {
                blend_to_ani(
                    plyr_pdata->fighter_definition->strafe_left_loop, 0, 0.1f);
            } else {
                blend_to_ani(
                    plyr_pdata->fighter_definition->strafe_right_loop, 0, 0.1f);
            }
        }
        plyr_anim_pdata->step = 0.9f;

        while (test() == 0 && exec_tick_ctr - start_tick < ticks) {
            if (is_big_boss(plyr_pdata) &&
                ((plyr_anim_pdata->frame > 11.8f &&
                  plyr_anim_pdata->frame < 12.5f) ||
                 (plyr_anim_pdata->frame > 32.8f &&
                  plyr_anim_pdata->frame < 33.5f))) {
                ai_big_boss_strafe_footstep();
            }
            advance_anim(plyr_anim_pdata);
            pose_anim(plyr_anim_pdata, 1);
            AI_SLEEP(1.0f);
        }
    } else {
        ani_to_blend_frame(10.0f);
        plyr_anim_pdata->obj_movement_weight = 1.0f;
    }

    plyr_pdata->strafe_direction = 0;
    set_my_state(0);
    if (plyr_pdata->fighter_definition->move_blend_data
            ->use_fighting_stance != 0) {
        blend_to_fstance(0.05f);
    } else {
        blend_to_stance(0.05f);
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

static int HeIsNotFacing(void) {
    Vec to_opponent;
    Vec facing;
    float dot;

    uv_to_opponent(&to_opponent);
    uv_from_angle_y(&facing, his_obj->ang.y);
    dot = to_opponent.x * facing.x + to_opponent.z * facing.z;
    return dot > -0.86f;
}

static int always_true(void) {
    return 1;
}

static int always_false(void) {
    return 0;
}

static int InAttackRange2(void) {
    return xz_distance_between_players() < 3.3445094f;
}

static int InAttackRange(void) {
    return xz_distance_between_players() < 1.4864486f;
}

static int InAttackRange_close(void) {
    return xz_distance_between_players() < 1.0f;
}

MkProc* get_player_proc(MkObj* player) {
    if (get_player_number(player) == 0) {
        return (MkProc*)g_game_info.plyr0.idle_proc;
    }
    return (MkProc*)g_game_info.plyr1.idle_proc;
}

void force_ai_style(int style) {
}

void drone_ai_set_avoidance_area(Vec* position, float duration) {
    PlyrPdata* opponent;
    DroneAI* drone;

    if (plyr_pdata != 0) {
        opponent = plyr_pdata->his_plyr_pdata;
        if (opponent->drone_request != 0) {
            drone = opponent->plyr_num == 0 ? &g_DroneAI1 : &g_DroneAI2;
            if (duration != 0.0f) {
                drone->avoidance_area_duration = duration;
                drone->avoidance_position[0] = position->x;
                drone->avoidance_position[1] = position->y;
                drone->avoidance_position[2] = position->z;
            }
        }
    }
}

/* Soft ceiling: exact size and the same nine instructions; MWCC schedules the
 * zero-float load before rather than after the player-pointer selection. */
void drone_ai_clear_avoidance_area_duration(int player) {
    DroneAI* drone;

    drone = player == 0 ? &g_DroneAI1 : &g_DroneAI2;
    drone->avoidance_area_duration = 0.0f;
}

/* Soft ceiling: table selection, result join, calls, stores, and CFG agree.
 * Retail retains one category-preservation move; the remaining differences
 * are nonvolatile GPR coloring in the row-address calculation. */
static AiFightstyleAttack* drone_ai_choose_move_from_category(
    int category, unsigned int likelihood, int* is_script) {
    DroneAI* drone;
    FighterAiTable* table;
    AiFightstyleAttack* script;
    AiFightstyleAttack* selected_script;
    AiFightstyleAttack* fightstyle_script;

    *is_script = 1;
    drone = get_player_number(plyr_obj) == 0 ? &g_DroneAI1 : &g_DroneAI2;
    table = plyr_pdata->ai_tables->tables;
    table += category;
    if (table->usable_row_count == 0) {
        selected_script = 0;
    } else {
        drone->ai_command = (unsigned int*)
            &table->rows[randu0((unsigned short)table->usable_row_count)];
        drone->ai_command_arg = 0;
        drone->ai_command_target = drone->player->character_id;
        drone->ai_command_flag0 = 0;
        drone->ai_command_flag1 = 0;
        drone->ai_command_flag2 = 0;
        selected_script = get_special_move();
    }
    script = selected_script;
    if (script == 0 || randu0(100) < likelihood) {
        fightstyle_script = get_random_fightstyle_attack(
            plyr_pdata->fighter_definition, category, 0);
        if (fightstyle_script != 0) {
            script = fightstyle_script;
            *is_script = 0;
        }
    }
    return script;
}

/* Soft ceiling: both wrappers have exact size, CFG, operations, calls,
 * table/index registers, and stmw/lmw. Residue is only an r28/r29 swap
 * between rows and cmo. */
static inline void generate_ai_table(
    FighterAiTable* source_table, ScriptSlot* source_cmo) {
    FighterAiMoveRow* rows;
    FighterAiTable* table;
    ScriptSlot* cmo;
    int index;

    index = 0;
    table = source_table;
    cmo = source_cmo;
    do {
        unsigned int row_count;

        table->usable_row_count = 0;
        rows = table->rows;
        if (rows != 0) {
            row_count = get_row_count_for_table_by_pointer(cmo, rows);
            if (row_count == 0) {
                table->usable_row_count = 0;
            } else {
                table->usable_row_count = row_count - 1;
            }
            if (table->usable_row_count > 0 &&
                rows[table->usable_row_count].move_id != -1) {
                table->usable_row_count++;
            }
        }
        index++;
        table++;
    } while (index < 14);
}

void generate_ai_table_player(PlyrPdata* player) {
    generate_ai_table(player->ai_tables->tables, player->cmo);
}

void generate_ai_table_moveset(PlyrWeaponStyle* moveset) {
    generate_ai_table(moveset->definition->ai_tables, moveset->script);
}
