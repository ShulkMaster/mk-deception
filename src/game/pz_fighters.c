#include "game/game_info.h"
#include "game/minigames.h"
#include "game/pz_fatality.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "runtime/mk_obj.h"
#include "runtime/image.h"
#include "runtime/plyr_pdata.h"

#define PZ_FIGHTER_DISTANCE_FIXED 0x04

typedef float (*PuzzleFighterEntry)(void);
typedef float (*PuzzleProcessTransfer)(PuzzleFighterEntry entry, float delay);
typedef struct AniScript AniScript;
typedef struct PuzzlePresentState PuzzlePresentState;
typedef struct PuzzleProjectile PuzzleProjectile;

typedef struct PuzzleProcessVtable {
    char pad00[0x18];
    int (*sleep)(void); /* +0x18 */
    char pad1C[8];
    PuzzleProcessTransfer transfer; /* +0x24 */
} PuzzleProcessVtable;

typedef struct PuzzleProcess {
    PuzzleProcessVtable* vtbl;
} PuzzleProcess;

typedef struct PuzzleFighterMove {
    unsigned int event_type; /* +0x00 */
    char pad04[0x0C];
    unsigned int mode; /* +0x10 */
    unsigned int script_move; /* +0x14 */
    unsigned int player; /* +0x18 */
    int distance_class; /* +0x1C */
    unsigned int active_flags; /* +0x20 */
    int has_followup; /* +0x24 */
    union {
        unsigned int policy_word; /* +0x28 */
        struct {
            unsigned char policy_flags; /* +0x28 */
            unsigned char runtime_flags; /* +0x29 */
            unsigned char policy_pad[2];
        };
    };
    char pad2C[8];
} PuzzleFighterMove; /* 0x34 */

typedef struct FirstMoveMadeRow {
    unsigned int type;
    PuzzleFighterEntry reaction;
    unsigned int percent;
} FirstMoveMadeRow;

typedef struct FirstMoveMadeTable {
    FirstMoveMadeRow rows[15];
    unsigned int pad;
} FirstMoveMadeTable; /* 0xB8 */

typedef struct PuzzleAnimPdata {
    char pad00[0x38];
    float frame; /* +0x38 */
} PuzzleAnimPdata;

typedef struct PuzzleFighterStartFlags {
    signed char enabled : 1; /* bit7 */
    unsigned char player_flags : 2; /* bits6-5 */
    unsigned char unused : 5;
} PuzzleFighterStartFlags;

typedef union PuzzleFloatBits {
    float value;
    unsigned int bits;
} PuzzleFloatBits;

typedef struct PuzzleAttackPolicyFlags {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char distance_reaction : 1; /* bit3 */
    unsigned char bit2 : 1;
    unsigned char bit1 : 1;
    unsigned char bit0 : 1;
} PuzzleAttackPolicyFlags;

typedef struct PuzzleAttackRuntimeFlags {
    unsigned char enabled : 1; /* bit7 */
    unsigned char low_bits : 7;
} PuzzleAttackRuntimeFlags;

static inline float pz_fast_sqrt(float squared) {
    PuzzleFloatBits input;
    PuzzleFloatBits estimate;

    if (squared <= 0.0f) {
        return 0.0f;
    }
    input.value = squared;
    estimate.bits =
        (unsigned int)*(unsigned short*)(
            (unsigned char*)GXMathSqrtTable +
            ((input.bits >> 10) & 0x3FFE)) << 8;
    estimate.bits |=
        (((input.bits & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000;
    return 0.5f *
           (estimate.value *
            (3.0f - (estimate.value * estimate.value) / squared));
}

typedef struct PuzzleFightersEngine {
    float balance; /* +0x00 */
    float arena_axis_x; /* +0x04 */
    char pad08[4];
    float arena_axis_z; /* +0x0C */
    float constraint_axis_x; /* +0x10 */
    char pad14[4];
    float constraint_axis_z; /* +0x18 */
    float center_x; /* +0x1C */
    float center_y; /* +0x20 */
    float center_z; /* +0x24 */
    float player1_idle_x; /* +0x28 */
    float player1_idle_y; /* +0x2C */
    float player1_idle_z; /* +0x30 */
    float player2_idle_x; /* +0x34 */
    float player2_idle_y; /* +0x38 */
    float player2_idle_z; /* +0x3C */
    Vec fighter_posts[2]; /* +0x40 - home/grinder posts */
    int round_running; /* +0x58 */
    int super_move_active; /* +0x5C */
    int random_fatality_active; /* +0x60 */
    union {
        PuzzleFighterMove fighter_move; /* +0x64 */
        struct {
            char pad_move64[0x20];
            unsigned int distance_flags; /* +0x84 */
            int attack_has_followup; /* +0x88 */
            union {
                unsigned int attack_policy_word; /* +0x8C */
                struct {
                    union {
                        unsigned char attack_policy_flags;
                        PuzzleAttackPolicyFlags attack_policy_bits;
                    }; /* +0x8C */
                    union {
                        unsigned char attack_runtime_flags;
                        PuzzleAttackRuntimeFlags attack_runtime_bits;
                    }; /* +0x8D */
                    unsigned char attack_policy_pad[2];
                };
            };
            unsigned int continuation_move; /* +0x90 */
            unsigned int continuation_priority; /* +0x94 */
        };
    };
    unsigned int pending_move_count; /* +0x98 */
    PuzzleFighterMove pending_moves[2]; /* +0x9C */
    int fighters_positioned; /* +0x104 */
    PuzzleProcess* master_proc; /* +0x108 */
    int positioning_active; /* +0x10C */
    int fighter_state[2]; /* +0x110 */
    int force_repel; /* +0x118 */
    int fatality_abort; /* +0x11C */
    int fatality_active; /* +0x120 */
    int fatality_ready; /* +0x124 */
    int fatality_victim; /* +0x128 */
    int fatality_attacker; /* +0x12C */
    MkObj* fatality_objects[8]; /* +0x130 */
    MkObj* present_object; /* +0x150 */
    MkObj* projectile_objects[2]; /* +0x154 */
    int fatality_index; /* +0x15C */
    unsigned int random_event_cooldown; /* +0x160 */
    int breakout; /* +0x164 */
    int y_constraint_enabled[2]; /* +0x168 */
    float y_constraint[2]; /* +0x170 */
    unsigned int fatality_timer; /* +0x178 */
    float fatality_motion; /* +0x17C */
    ScreenObj* screen_objects[2]; /* +0x180 */
    AniTextureControl* texture_controls[2]; /* +0x188 */
    int balance_update_timer; /* +0x190 */
    float pending_balance; /* +0x194 */
    union {
        unsigned char start_flags;
        PuzzleFighterStartFlags start_flag_bits;
    }; /* +0x198 */
    char pad199[3];
    unsigned int immediate_request_player; /* +0x19C */
    unsigned int immediate_request_type; /* +0x1A0 */
    int immediate_request_timer; /* +0x1A4 */
    int immediate_request_active; /* +0x1A8 */
    unsigned int event_block_count; /* +0x1AC */
    char pad1B0[4];
    int field_1B4;
    int balance_out_of_range; /* +0x1B8 */
    PuzzlePresentState* present; /* +0x1BC */
    int reaction_slots[6]; /* +0x1C0 */
    int field_1D8;
    int super_move_request_pending; /* +0x1DC */
    int constraint_timer; /* +0x1E0 */
    int reactions_disabled; /* +0x1E4 */
} PuzzleFightersEngine;

typedef struct PuzzleEffectBankContext {
    int slot;
    int field_04;
    int field_08;
} PuzzleEffectBankContext;

typedef void (*PuzzleObjectArrivalFn)(int y, double conversion_bias);

typedef struct PuzzleObjectMotion {
    MkHdr hdr;
    int complete; /* +0x08 */
    Vec start; /* +0x0C */
    Vec target; /* +0x18 */
    float velocity_x; /* +0x24 */
    float velocity_y; /* +0x28 */
    int field_2C;
    PuzzleObjectArrivalFn arrival; /* +0x30 */
    ScreenObj* object; /* +0x34 */
    float gravity_step; /* +0x38 */
    float gravity_accumulator; /* +0x3C */
    int minimum_velocity_y; /* +0x40 */
    int bounce_enabled; /* +0x44 */
    int rise_ticks; /* +0x48 */
    int fall_ticks; /* +0x4C */
    int lifetime; /* +0x50 */
    int target_ticks; /* +0x54 */
    AniTextureControl* texture_control; /* +0x58 */
    int arrived; /* +0x5C */
} PuzzleObjectMotion; /* 0x60 */

typedef struct PuzzleCmdScriptView {
    char pad00[0x28];
    int function_index;
} PuzzleCmdScriptView;

typedef struct PuzzleFighterEvent {
    unsigned int player;
    unsigned int type;
    float block_count;
    float chain_count;
} PuzzleFighterEvent;

typedef struct PuzzleSharedAnimations {
    AniScript* entries[149];
} PuzzleSharedAnimations; /* 0x254 */

typedef struct PuzzleRegisteredMove {
    int script_move;
    unsigned int chance;
    unsigned int conditions;
} PuzzleRegisteredMove;

typedef struct PuzzleCharacterMoveTable {
    unsigned int count;
    PuzzleRegisteredMove moves[15];
} PuzzleCharacterMoveTable; /* 0xB8 */

typedef struct PuzzleFighterMoveTables {
    unsigned int common_count;
    int common_moves[15];
    PuzzleCharacterMoveTable characters[14];
} PuzzleFighterMoveTables; /* 0xA50 */

PuzzleSharedAnimations pz_shared_ani;
PuzzleFightersEngine g_pz_fighters_engine;
PuzzleFighterMoveTables g_pz_fighter_tables;
extern PuzzleProcess* aproc;
extern MkHdr* apdata;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern PlyrPdata* his_pdata;
extern PuzzleAnimPdata* plyr_anim_pdata;
extern float _mkproc_sleep_ticks;
extern int screen_width;
extern PuzzleProjectile* g_global_projectile;
static ScriptSlot* pz_shared_cmo;
extern PuzzleProcess* _create_mkproc_generic_tinystack(
    int proc_id, int priority, PuzzleFighterEntry entry, int pdata_size,
    MkHdr** out_pdata);
extern PuzzleProcess* _create_mkproc_generic_bigstack(
    int proc_id, int priority, PuzzleFighterEntry entry, int pdata_size,
    MkHdr** out_pdata);

double sqrt(double value);
float puzzle_fighter_get_super_bar_level(int player);
int pz_fighter_fatality_during_round_stuff_over(void);
float pz_fighters_fatality_prep_chores(void);
float pz_fighters_fatality_in_progress(void);
static void check_fighter_constraints(void);
static void pz_fighter_process_immediate_request(void);
static int pz_fighter_check_for_player_to_center_position_control(void);
static int pz_fighter_individual_plyr_do_something(
    unsigned int player, unsigned int state);
static int pz_fighters_inside_super_move_scenerio(void);
static float pz_fighters_handle_next_pending_move(void);
static int pz_fighters_idle_process(void);
static float pz_fighters_handle_next_pending_move_simplified(void);
static void pz_fighter_calculate_start_pos(void);
static float p_puzzle_fighter_master(void);
void load_reduced_shared_and_hand_anims(void);
void load_pz_shared_anims(void);
void set_process_as_scriptable(PuzzleProcess* proc);
ScriptSlot* cmdscript_loadfile_by_name(int language, char* name);
void cmdscript_setup_execution(ScriptSlot* slot, unsigned int function_index);
void cmdscript_execute(ScriptSlot* slot);
MkObj* load_model_from_slot(int handle, unsigned int art_oid, int heap_id);
void obj_create_sobjs(MkObj* object);
void load_effect_bank_with_context(
    char* name, PuzzleEffectBankContext* context);
float pz_fighter_exit(void);
float p_plyr_pz_fighter_entry(void);
float pz_fighter_dizzy(void);
static float p_plyr_pz_fighter_loop(void);
static float pz_fighter_move_into_desired_position(void);
float pz_fighter_laugh_small(void);
float pz_fighter_random_taunt(void);
float pz_fighter_round_victory(void);
float pz_fighter_round_failure(void);
float pz_fighter_round_whew(void);
float pz_fighter_round_ground_pound(void);
float pz_fighter_WTF(void);
float pz_fighter_showoff_warmup1(void);
float pz_fighter_showoff_warmup2(void);
float pz_fighter_active_warmup1(void);
float pz_fighter_active_warmup2(void);
float pz_fighter_bow_warmup(void);
float r_pz_call_script_function(void);
void xfer_player_proc(PuzzleProcess* proc, PuzzleFighterEntry entry);
PuzzleCmdScriptView* get_cmdscript_for_proc(PuzzleProcess* proc);
static float pz_fighter_move_into_fighting_position_now(void);
int pz_fighter_walk_until_fight_distance(void);
int pz_fighter_walk_until_danger_or_in_wrong_direction(void);
void pz_fighter_walk_FB_true(
    int (*test)(void), unsigned int duration, int forward);
float xz_distance_between_players(void);
static void pz_fighter_snap_to_distance(
    float desired_distance_squared, float current_distance_squared);
void pz_fighter_reaction_xfer_him(int reaction);
PuzzleProcess* pz_fighter_get_player_proc(unsigned int player);
void pz_fighter_set_y_constrain(MkObj* fighter, int enabled, float y);
float pz_fighter_process_random_fatality_event(
    PuzzleFatalityRandomEvent* event, PuzzleFatalityProcessFn reaction);
float p_plyr_pz_fighter_start(void);

static inline PuzzleProcess* pz_fighter_player_proc(unsigned int player) {
    if (player == 0) {
        return g_game_info.plyr0.idle_proc;
    }
    return g_game_info.plyr1.idle_proc;
}
void advance_anim(void);
void pose_anim(PuzzleAnimPdata* animation, int update_object);
void attack_to_frame_x(
    AniScript* animation, int field0C, int field10, int field14,
    float frame1, float frame2, float frame3, float frame4);
void force_forward(int duration, int interval, float velocity, float damping);
void force_away(int duration, int interval, float velocity, float damping);
void move_player(MkObj* fighter, const Vec* position, const Vec* angle);
unsigned int randu0(unsigned int max);
void puzzle_fighter_get_num_blocks_on_screen(
    unsigned int* player1_blocks, unsigned int* player2_blocks);
unsigned int puzzle_fighter_plyr_winning_big_based_on_points(void);
void face_opponent_now(void);
void blend_to_stance(float blend);
void init_ground_move(void);
void back_to_normal(void);
void rotate_towards_him(float rate);
void glitch_to_stance(float blend);
void xfer_proc(PuzzleProcess* proc, PuzzleFighterEntry entry);
static float pz_fighter_handle_dual_off_center_Move(PuzzleFighterMove* move);
static float pz_fighter_handle_center_pos_minor_adjustment(PuzzleFighterMove* move);
static float pz_fighter_handle_center_pos_single_close_move(PuzzleFighterMove* move);
static float pz_fighter_handle_center_pos_single_range_move(PuzzleFighterMove* move);
static float pz_fighter_handle_center_pos_range_attack(PuzzleFighterMove* move);
float pz_fighter_handle_distance_attack(PuzzleFighterMove* move);
static float pz_fighter_handle_off_wall_attack(PuzzleFighterMove* move);
static float pz_fighter_handle_winning_big_based_on_score_move(
    PuzzleFighterMove* move);
static float pz_fighter_handle_super_move_available(PuzzleFighterMove* move);
static float pz_fighter_handle_ohyeah_move(PuzzleFighterMove* move);
static float pz_fighter_handle_in_super_move(PuzzleFighterMove* move);
static float pz_fighter_handle_ohno_move(PuzzleFighterMove* move);
static float pz_fighter_handle_peak_move(PuzzleFighterMove* move);
static float pz_fighter_handle_relief_move(PuzzleFighterMove* move);
static float pz_fighter_handle_special_move(PuzzleFighterMove* move);
static float pz_fighter_handle_move(PuzzleFighterMove* move);
float pz_fighter_light_propell(void);
float pz_fighter_dummy_propell(void);
float pz_fighter_smart_flippy(void);
float pz_fighter_perform_center_pos_minor_adjustement(void);
float pz_fighter_perform_center_pos_single_close_move(void);
float pz_fighter_perform_center_pos_single_range_move(void);
float pz_fighter_perform_center_pos_range_attack(void);
float pz_fighter_perform_dist_attack(void);
float pz_fighter_perform_off_wall_attack(void);
float pz_fighter_give_present(void);
float pz_fighter_perform_super_move_just_enabled(void);
float pz_fighter_perform_other_guy_super_move_just_enabled(void);
float pz_fighter_perform_ohyeah_move(void);
float pz_fighter_perform_other_guy_ohyeah(void);
float pz_fighter_perform_holding_onto_super_move(void);
float pz_fighter_perform_other_guy_holding_onto_super_move(void);
float pz_fighter_perform_ohno_move(void);
float pz_fighter_perform_other_guy_ohno(void);
float pz_fighter_perform_peak_move(void);
float pz_fighter_perform_relief_move(void);
float pz_fighter_perform_special_move(void);
float pz_fighter_perform_scripted_move(void);
int pz_fighter_should_handle_special_move(unsigned int player);
void pz_fighter_disallow_continuation(void);
int pz_fighter_check_fatality_random_event(
    PuzzleFightersEngine* engine, int force);
float pz_fighters_react_to_bomb_explosion(void);
float pz_fighter_big_time_happy(void);
float pz_fighter_whatever2(void);
void pz_fighter_shake_camera(int duration, float strength);
static void pz_fighter_perform_end_of_round_anims(
    unsigned int player, unsigned int other_player);
static float p_objects_moving(double conversion_bias);
/*
 * Soft ceiling: complete object-motion construction; remaining differences
 * are pointer-slot CSE, float scheduling, conversion slots, and labels.
 */
void pz_fighter_anim_object_to(
    unsigned int player, int mirror, int frame, const Vec* start,
    const Vec* target, const Vec* velocity, int minimum_velocity_y,
    unsigned int target_ticks, float frame_rate, float gravity_step,
    int bounce, PuzzleObjectArrivalFn arrival);
void pz_fighters_fatality_round_over(void);
void pz_fighters_fatality_unload(void);
void pz_fighters_fatality_normal_fighting(int enabled);
void pz_fighters_fatality_start(
    unsigned int victim, unsigned int attacker);
void pz_fighters_fatality_preround_event(void);
void pz_fighter_load_place_fatality_elements(unsigned int fatality);
void pz_fighter_kill_present(void);
void pz_fighter_kill_global_projectile(void);
void* memcpy(void* destination, const void* source, unsigned long size);
void pz_fighters_calc_distance_to_desired_idle_pos_abs(
    float* player1_distance, float* player2_distance,
    float* player1_absolute, float* player2_absolute);
void pz_fighters_calc_distance_to_desired_idle_pos(
    float* player1_distance, float* player2_distance);
void pz_fighter_classify_move_8012260C(
    unsigned int block_count, int chain_count, unsigned int* move,
    unsigned int* priority, unsigned int event_type);
static void pz_fighter_buffer_new_move(
    unsigned int event_type, unsigned int player, unsigned int move,
    unsigned int priority);
static void pz_fighter_fight_request(
    unsigned int player, unsigned int block_count, int chain_count,
    unsigned int event_type);
static void pz_fighter_first_block_has_been_placed(unsigned int player);
float pz_fighter_back_and_forth_showoff(void);
float pz_fighter_punch_dizzyfall(void);
float pz_fighter_footstomp(void);

static FirstMoveMadeTable fistMoveMadeTable = {
    {
        {3, pz_fighter_back_and_forth_showoff, 30},
        {6, pz_fighter_punch_dizzyfall, 60},
        {6, pz_fighter_footstomp, 100},
        {0xFF, 0, 0},
    },
    0,
};

static inline PlyrPdata* puzzle_player_pdata(unsigned int player) {
    if (player == 0) {
        return (PlyrPdata*)g_game_info.plyr0.slot.fighter;
    }
    return (PlyrPdata*)g_game_info.plyr1.slot.fighter;
}

static inline MkObj* puzzle_fighter_object(int player) {
    if (player == 0) {
        return (MkObj*)g_game_info.plyr0.slot.mirror_a;
    }
    return (MkObj*)g_game_info.plyr1.slot.mirror_a;
}



static inline float pz_fighter_signed_idle_distance(unsigned int player) {
    MkObj* player1;
    MkObj* player2;
    float dx;
    float dz;
    float player1_distance;
    float player2_distance;

    g_pz_fighters_engine.fighters_positioned = 0;
    player1 = (MkObj*)g_game_info.plyr0.slot.mirror_a;
    dz = player1->pos.z - g_pz_fighters_engine.player1_idle_z;
    dx = player1->pos.x - g_pz_fighters_engine.player1_idle_x;
    player1_distance = dx * dx + dz * dz;
    if (dx > 0.0f) {
        player1_distance *= -1.0f;
    }
    player2 = (MkObj*)g_game_info.plyr1.slot.mirror_a;
    dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
    dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
    player2_distance = dx * dx + dz * dz;
    if (dx < 0.0f) {
        player2_distance *= -1.0f;
    }
    if (player == 0) {
        return player1_distance;
    }
    return player2_distance;
}


static inline void pz_start_round_animation(
    unsigned int player, int fighter_state, PuzzleFighterEntry entry) {
    PlyrPdata* pdata;

    pdata = puzzle_player_pdata(player);
    if ((pdata->state & 0x200) == 0) {
        if (player == 0) {
            g_pz_fighters_engine.fighter_state[0] = fighter_state;
        } else {
            g_pz_fighters_engine.fighter_state[1] = fighter_state;
        }
        pdata->state |= 0x4201;
        xfer_proc(pz_fighter_player_proc(player), entry);
    }
}


int pz_fighter_fatality_over(void) {
    return g_pz_fighters_engine.fatality_abort == 0;
}

int pz_fighter_is_round_over(void) {
    if (puzzle_player_pdata(0)->state == 0 &&
        puzzle_player_pdata(1)->state == 0 &&
        pz_fighter_fatality_during_round_stuff_over() == 1) {
        return 1;
    }
    return 0;
}

static inline void pz_fighter_start_immediate_request(
    unsigned int player, unsigned int type, int timer) {
    if (g_pz_fighters_engine.immediate_request_active != 1) {
        g_pz_fighters_engine.immediate_request_active = 1;
        g_pz_fighters_engine.immediate_request_player = player;
        g_pz_fighters_engine.immediate_request_type = type;
        g_pz_fighters_engine.immediate_request_timer = timer;
        g_pz_fighters_engine.breakout = 1;
    }
}

static unsigned int block_line;
static int old_position_state_in_center;

static inline void pz_fighter_check_board_spread(
    unsigned int player, unsigned int block_count,
    unsigned int chain_count, int use_chain_count) {
    unsigned int player1_blocks;
    unsigned int player2_blocks;
    unsigned int favored_player;
    int difference;

    favored_player = 0;
    puzzle_fighter_get_num_blocks_on_screen(
        &player1_blocks, &player2_blocks);
    if (player2_blocks < player1_blocks) {
        favored_player = 1;
    }
    difference = (int)player1_blocks - (int)player2_blocks;
    if (difference < 0) {
        difference = -difference;
    }
    if (difference < 8) {
        block_line = player1_blocks;
        old_position_state_in_center = 1;
    } else if (difference >= 12 && old_position_state_in_center == 1) {
        old_position_state_in_center = 0;
        if (block_line > 25 && block_line < 65 &&
            player == favored_player &&
            (block_count > 6 ||
             (use_chain_count != 0 && chain_count > 2))) {
            old_position_state_in_center = 0;
            pz_fighter_fight_request(player, 0, 0, 8);
        }
    }
}

static inline void pz_fighter_breakout_of_random_fatality(void) {
    if (g_pz_fighters_engine.random_fatality_active == 1 &&
        g_pz_fighters_engine.fighter_move.player != 0 &&
        g_pz_fighters_engine.fighter_move.mode <= 9) {
        g_pz_fighters_engine.breakout = 1;
    }
    if (g_pz_fighters_engine.random_fatality_active == 1 &&
        g_pz_fighters_engine.fighter_move.player != 1 &&
        g_pz_fighters_engine.fighter_move.mode <= 9) {
        g_pz_fighters_engine.breakout = 1;
    }
}

static inline void pz_fighter_begin_super_move(void) {
    pz_fighter_breakout_of_random_fatality();
    g_pz_fighters_engine.super_move_active = 1;
    g_pz_fighters_engine.super_move_request_pending =
        (unsigned short)randu0(100) < 75;
    pz_fighter_kill_present();
    pz_fighter_kill_global_projectile();
}

/*
 * Soft ceiling: all 26 retail event cases, persistent board-spread state,
 * model/effect setup, super/fatality transitions, and immediate requests are
 * recovered. The remaining delta is MWCC emission: stack/GPR allocation,
 * typed indexed queue and reaction-slot induction, equivalent bitfield masks,
 * branch scheduling around shared inline policy, and local relocation labels.
 */
void pz_fighter_event(PuzzleFighterEvent* event) {
    PuzzleFightersEngine* engine;
    unsigned int block_count;
    unsigned int chain_count;

    switch (event->type) {
    case 15:
        engine = &g_pz_fighters_engine;
        engine->start_flags &= (unsigned char)~0x80;
        pz_fighters_fatality_unload();
        break;
    case 1:
    {
        PuzzleEffectBankContext effect_context;
        unsigned int index;

        g_pz_fighters_engine.balance = 0.0f;
        g_pz_fighters_engine.immediate_request_active = 0;
        if (g_pz_fighters_engine.start_flag_bits.enabled == 0) {
            load_reduced_shared_and_hand_anims();
            load_pz_shared_anims();
            set_process_as_scriptable(aproc);
            pz_shared_cmo =
                cmdscript_loadfile_by_name(17, "pz_moves.mko");
            cmdscript_setup_execution(pz_shared_cmo, 91);
            cmdscript_execute(pz_shared_cmo);
            g_pz_fighters_engine.start_flag_bits.enabled = 1;
        }

        g_pz_fighters_engine.projectile_objects[0] =
            load_model_from_slot(0x10005, 0x20002, 0x6026);
        hide_obj(g_pz_fighters_engine.projectile_objects[0]);
        g_pz_fighters_engine.projectile_objects[0]->light_flags = 20;
        g_pz_fighters_engine.projectile_objects[1] =
            load_model_from_slot(0x10005, 0x20002, 0x6026);
        hide_obj(g_pz_fighters_engine.projectile_objects[1]);
        g_pz_fighters_engine.projectile_objects[1]->light_flags = 20;
        g_pz_fighters_engine.reactions_disabled = 0;
        obj_create_sobjs(g_pz_fighters_engine.projectile_objects[0]);
        obj_create_sobjs(g_pz_fighters_engine.projectile_objects[1]);

        g_pz_fighters_engine.y_constraint_enabled[0] = 0;
        g_pz_fighters_engine.y_constraint_enabled[1] = 0;
        g_pz_fighters_engine.pending_move_count = 0;
        g_pz_fighters_engine.random_fatality_active = 0;
        g_pz_fighters_engine.breakout = 0;
        g_pz_fighters_engine.fighters_positioned = 0;
        xz_unit_vector(
            (Vec*)&g_pz_fighters_engine.arena_axis_x,
            &g_game_info.plyr1.slot.mirror_a->pos,
            &g_game_info.plyr0.slot.mirror_a->pos);
        g_pz_fighters_engine.constraint_axis_x = -g_pz_fighters_engine.arena_axis_z;
        g_pz_fighters_engine.constraint_axis_z = g_pz_fighters_engine.arena_axis_x;
        g_pz_fighters_engine.random_event_cooldown = 0;
        g_pz_fighters_engine.master_proc = _create_mkproc_generic_bigstack(
            0x600E, 31, p_puzzle_fighter_master, 0, 0);
        g_pz_fighters_engine.positioning_active = 0;
        g_pz_fighters_engine.force_repel = 0;
        g_pz_fighters_engine.fatality_abort = 0;
        g_pz_fighters_engine.constraint_timer = 180;
        g_pz_fighters_engine.fatality_objects[0] =
            load_model_from_slot(0x10005, 0x20006, 0x8227);
        if (g_pz_fighters_engine.fatality_objects[0] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[0]->light_flags = 4;
        g_pz_fighters_engine.fatality_objects[0]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[0]);
        hide_obj(g_pz_fighters_engine.fatality_objects[0]);
        effect_context.slot = 0x70038;
        effect_context.field_04 = 0;
        effect_context.field_08 = 0;
        load_effect_bank_with_context("pz_mini_fx.mko", &effect_context);

        g_pz_fighters_engine.fatality_objects[1] =
            load_model_from_slot(0x10005, 0x20006, 0x8227);
        if (g_pz_fighters_engine.fatality_objects[1] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[1]->light_flags = 20;
        g_pz_fighters_engine.fatality_objects[1]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[1]);
        hide_obj(g_pz_fighters_engine.fatality_objects[1]);
        g_pz_fighters_engine.fatality_objects[2] =
            load_model_from_slot(0x10005, 0x20002, 0x6026);
        if (g_pz_fighters_engine.fatality_objects[2] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[2]->light_flags = 20;
        g_pz_fighters_engine.fatality_objects[2]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[2]);
        hide_obj(g_pz_fighters_engine.fatality_objects[2]);
        g_pz_fighters_engine.fatality_objects[3] =
            load_model_from_slot(0x10005, 0x20003, 0x6027);
        if (g_pz_fighters_engine.fatality_objects[3] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[3]->light_flags = 20;
        g_pz_fighters_engine.fatality_objects[3]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[3]);
        hide_obj(g_pz_fighters_engine.fatality_objects[3]);
        g_pz_fighters_engine.fatality_objects[4] =
            load_model_from_slot(0x10005, 0x20004, 0x6028);
        if (g_pz_fighters_engine.fatality_objects[4] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[4]->light_flags = 20;
        g_pz_fighters_engine.fatality_objects[4]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[4]);
        hide_obj(g_pz_fighters_engine.fatality_objects[4]);
        g_pz_fighters_engine.fatality_objects[5] =
            load_model_from_slot(0x10005, 0x20005, 0x6029);
        if (g_pz_fighters_engine.fatality_objects[5] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[5]->light_flags = 20;
        g_pz_fighters_engine.fatality_objects[5]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[5]);
        hide_obj(g_pz_fighters_engine.fatality_objects[5]);
        g_pz_fighters_engine.fatality_objects[6] =
            load_model_from_slot(0x70038, 0x08160001, 0x6029);
        if (g_pz_fighters_engine.fatality_objects[6] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[6]->light_flags = 20;
        g_pz_fighters_engine.fatality_objects[6]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[6]);
        hide_obj(g_pz_fighters_engine.fatality_objects[6]);
        g_pz_fighters_engine.fatality_objects[7] =
            load_model_from_slot(0x70038, 0x08160001, 0x6029);
        if (g_pz_fighters_engine.fatality_objects[7] == 0) {
            break;
        }
        g_pz_fighters_engine.fatality_objects[7]->light_flags = 20;
        g_pz_fighters_engine.fatality_objects[7]->flags_08_bits.airborne = 1;
        insert_fgnd_mkobj(g_pz_fighters_engine.fatality_objects[7]);
        hide_obj(g_pz_fighters_engine.fatality_objects[7]);

        g_pz_fighters_engine.screen_objects[0] = load_wiff_screen_pfxobj(
            0x70038, 0x08160000, 0x601F,
            &g_pz_fighters_engine.texture_controls[0], 0, 57);
        set_ani_texture_framerate(g_pz_fighters_engine.texture_controls[0], 1.0f);
        pull_screen_obj(g_pz_fighters_engine.screen_objects[0]);
        pull_ani_texture_control(g_pz_fighters_engine.texture_controls[0]);
        g_pz_fighters_engine.screen_objects[0]->flag_bits.hidden = 1;
        g_pz_fighters_engine.screen_objects[0]->scale_x = 0.5f;
        g_pz_fighters_engine.screen_objects[0]->scale_y = 0.5f;

        g_pz_fighters_engine.present_object =
            load_model_from_slot(0x70038, 0x08160002, 0x6029);
        if (g_pz_fighters_engine.present_object == 0) {
            break;
        }
        g_pz_fighters_engine.present_object->light_flags = 20;
        g_pz_fighters_engine.present_object->flags_08_bits.airborne = 1;
        g_pz_fighters_engine.present_object->flags_08_bits.scale_active = 1;
        g_pz_fighters_engine.present_object->scale.x = 0.045f;
        g_pz_fighters_engine.present_object->scale.y = 0.045f;
        g_pz_fighters_engine.present_object->scale.z = 0.045f;
        insert_fgnd_mkobj(g_pz_fighters_engine.present_object);
        hide_obj(g_pz_fighters_engine.present_object);

        g_pz_fighters_engine.pending_balance = 0.0f;
        g_pz_fighters_engine.balance_update_timer = 180;
        g_pz_fighters_engine.field_1B4 = 0;
        g_pz_fighters_engine.present = 0;
        g_global_projectile = 0;
        g_pz_fighters_engine.super_move_request_pending = 1;
        for (index = 0; index < 6; index++) {
            g_pz_fighters_engine.reaction_slots[index] = 0;
        }
        pz_fighter_calculate_start_pos();
        move_player(
            g_game_info.plyr0.slot.mirror_a,
            (Vec*)&g_pz_fighters_engine.player1_idle_x,
            &g_game_info.plyr0.slot.mirror_a->ang);
        move_player(
            g_game_info.plyr1.slot.mirror_a,
            (Vec*)&g_pz_fighters_engine.player2_idle_x,
            &g_game_info.plyr1.slot.mirror_a->ang);
        g_pz_fighters_engine.start_flags &= (unsigned char)~0x78;
        g_pz_fighters_engine.round_running = 0;
        break;
    }
    case 19:
        pz_fighter_load_place_fatality_elements(
            (unsigned int)event->block_count);
        break;
    case 0:
        engine = &g_pz_fighters_engine;
        engine->round_running = 1;
        engine->super_move_active = 0;
        engine->balance_update_timer = 180;
        pz_fighters_fatality_normal_fighting(1);
        engine->start_flags &= (unsigned char)~0x60;
        engine->field_1B4 = 0;
        engine->field_1D8 = 0;
        engine->constraint_timer = 0;
        break;
    case 2:
        engine = &g_pz_fighters_engine;
        engine->round_running = 0;
        pz_fighters_fatality_normal_fighting(0);
        break;
    case 3:
        pz_fighters_fatality_unload();
        _mkproc_sleep_ticks = 2.0f;
        aproc->vtbl->sleep();
        break;
    case 11:
    {
        PuzzleFighterMove pending = {0};

        engine = &g_pz_fighters_engine;
        engine->round_running = 1;
        engine->pending_move_count = 0;
        pending.event_type = 2;
        pending.mode = 16;
        pending.player = event->player;
        if (engine->pending_move_count < 2) {
            memcpy(
                &engine->pending_moves[engine->pending_move_count],
                &pending, sizeof(pending));
            engine->pending_move_count++;
        }
        pz_fighter_begin_super_move();
        break;
    }
    case 14:
        engine = &g_pz_fighters_engine;
        engine->pending_move_count = 0;
        pz_fighter_begin_super_move();
        if (event->player == 0) {
            engine->fatality_attacker = 1;
            puzzle_player_pdata(1)->fatality_shove_active = 1;
        } else {
            engine->fatality_attacker = 0;
            puzzle_player_pdata(0)->fatality_shove_active = 1;
        }
        break;
    case 10:
        engine = &g_pz_fighters_engine;
        pz_fighter_breakout_of_random_fatality();
        engine->round_running = 0;
        engine->fatality_abort = 1;
        engine->fatality_active = 0;
        engine->fatality_ready = 0;
        engine->fatality_victim = event->player;
        pz_fighters_fatality_start(
            event->player, engine->fatality_attacker);
        break;
    case 4:
        engine = &g_pz_fighters_engine;
        engine->pending_balance = event->block_count;
        if (event->block_count - engine->balance > 0.05f ||
            event->block_count - engine->balance < -0.05f) {
            engine->balance_update_timer = 1;
        }
        break;
    case 5:
        if ((randu0(100) & 0xFFFF) < 20) {
            pz_fighter_fight_request(
                event->player, randu0(10) & 0xFFFF,
                (randu0(4) & 0xFFFF) + 1, 1);
        } else {
            pz_fighter_fight_request(event->player, 0, 0, 2);
        }
        break;
    case 8:
        pz_fighter_fight_request(event->player, 0, 0, 6);
        break;
    case 9:
        pz_fighter_fight_request(event->player, 0, 0, 5);
        break;
    case 23:
        pz_fighter_fight_request(event->player, 0, 0, 7);
        break;
    case 6:
        engine = &g_pz_fighters_engine;
        block_count = (unsigned int)event->block_count;
        chain_count = (unsigned int)event->chain_count;
        pz_fighter_check_board_spread(
            event->player, block_count, chain_count, 1);
        pz_fighter_fight_request(
            event->player, block_count, chain_count, 1);
        engine->start_flags |= 0x60;
        break;
    case 12:
        engine = &g_pz_fighters_engine;
        if (event->block_count > 0.0f) {
            unsigned int winner;

            winner = puzzle_fighter_plyr_winning_big_based_on_points();
            if (event->player == winner && event->player == 0 &&
                (engine->start_flags & 0x10) == 0) {
                pz_fighter_fight_request(event->player, 0, 0, 9);
                engine->start_flags |= 0x10;
                break;
            }
            if (event->player == winner && event->player == 1 &&
                (engine->start_flags & 0x08) == 0) {
                pz_fighter_fight_request(event->player, 0, 0, 9);
                engine->start_flags |= 0x08;
                break;
            }
            block_count = (unsigned int)event->block_count;
            pz_fighter_check_board_spread(
                event->player, block_count, 0, 0);
            pz_fighter_fight_request(
                event->player, block_count, 0, 0);
            engine->start_flags |= 0x60;
        }
        break;
    case 25:
        pz_fighter_start_immediate_request(event->player, 18, 1);
        break;
    case 7:
        pz_fighter_start_immediate_request(event->player, 1, 6);
        break;
    case 16:
        pz_fighter_start_immediate_request(event->player, 4, 1);
        break;
    case 17:
        pz_fighter_start_immediate_request(event->player, 5, 1);
        break;
    case 13:
        engine = &g_pz_fighters_engine;
        engine->fighter_state[0] = 8;
        engine->fighter_state[1] = 8;
        puzzle_player_pdata(0)->state |= 0x4201;
        puzzle_player_pdata(1)->state |= 0x4201;
        if (engine->event_block_count == 0) {
            xfer_proc(g_game_info.plyr0.idle_proc, pz_fighter_bow_warmup);
            xfer_proc(g_game_info.plyr1.idle_proc, pz_fighter_bow_warmup);
        } else if (engine->event_block_count == 1) {
            if ((unsigned short)randu0(100) < 50) {
                xfer_proc(
                    g_game_info.plyr0.idle_proc,
                    pz_fighter_active_warmup1);
                xfer_proc(
                    g_game_info.plyr1.idle_proc,
                    pz_fighter_active_warmup2);
            } else {
                xfer_proc(
                    g_game_info.plyr0.idle_proc,
                    pz_fighter_active_warmup2);
                xfer_proc(
                    g_game_info.plyr1.idle_proc,
                    pz_fighter_active_warmup1);
            }
        } else if (engine->event_block_count == 2) {
            if ((unsigned short)randu0(100) < 50) {
                xfer_proc(
                    g_game_info.plyr0.idle_proc,
                    pz_fighter_showoff_warmup1);
                xfer_proc(
                    g_game_info.plyr1.idle_proc,
                    pz_fighter_showoff_warmup2);
            } else {
                xfer_proc(
                    g_game_info.plyr0.idle_proc,
                    pz_fighter_showoff_warmup2);
                xfer_proc(
                    g_game_info.plyr1.idle_proc,
                    pz_fighter_showoff_warmup1);
            }
        }
        pz_fighters_fatality_preround_event();
        break;
    case 20:
        engine = &g_pz_fighters_engine;
        engine->event_block_count = (unsigned int)event->block_count;
        if (engine->event_block_count != 0) {
            Vec player1_position;
            Vec player2_position;
            const Vec separation = {0.46f, 0.0f, 0.0f};
            MkObj* player1 = g_game_info.plyr0.slot.mirror_a;
            MkObj* player2 = g_game_info.plyr1.slot.mirror_a;

            player1_position.x = player1->pos.x - separation.x;
            player1_position.y = player1->pos.y - separation.y;
            player1_position.z = player1->pos.z - separation.z;
            player2_position.x = player2->pos.x + separation.x;
            player2_position.y = player2->pos.y + separation.y;
            player2_position.z = player2->pos.z + separation.z;
            move_player(player1, &player1_position, &player1->ang);
            move_player(player2, &player2_position, &player2->ang);
        }
        break;
    case 18:
        pz_fighters_fatality_preround_event();
        break;
    case 24:
        engine = &g_pz_fighters_engine;
        if ((engine->start_flags & 0x60) == 0) {
            pz_fighter_first_block_has_been_placed(event->player);
        } else {
            pz_fighter_fight_request(event->player, 0, 0, 0);
        }
        engine->start_flags |= 0x40;
        break;
    case 21:
        pz_fighter_start_immediate_request(event->player, 8, 1);
        break;
    case 22:
        pz_fighter_start_immediate_request(event->player, 9, 1);
        break;
    default:
        break;
    }
}

/*
 * Soft ceiling: complete request/continuation/queue policy. Remaining
 * differences are structured join emission, queue pointer induction, GPR
 * allocation, and local labels.
 */
static void pz_fighter_fight_request(
    unsigned int player, unsigned int block_count, int chain_count,
    unsigned int event_type) {
    unsigned int move;
    unsigned int priority;
    unsigned int index;
    unsigned int copy_index;
    int continuation;
    int accepted;
    int disallow;
    int super_ready;

    pz_fighter_classify_move_8012260C(
        block_count, chain_count, &move, &priority, event_type);
    disallow = 0;
    if (g_pz_fighters_engine.random_fatality_active == 1) {
        if (priority <= 14) {
            if (puzzle_fighter_get_super_bar_level(0) > 0.95f) {
                super_ready = 1;
            } else {
                super_ready = 0;
            }
            if (super_ready == 0) {
                if (puzzle_fighter_get_super_bar_level(1) > 0.95f) {
                    super_ready = 1;
                } else {
                    super_ready = 0;
                }
                if (super_ready != 0 ||
                    g_pz_fighters_engine.immediate_request_active != 0) {
                    disallow = 1;
                }
            } else {
                disallow = 1;
            }
        } else {
            disallow = 1;
        }
    }
    if (disallow != 0) {
        pz_fighter_disallow_continuation();
        continuation = 0;
    } else {
        continuation =
            g_pz_fighters_engine.random_fatality_active == 1 &&
            g_pz_fighters_engine.fighter_move.player == player &&
            g_pz_fighters_engine.attack_runtime_bits.enabled == 1 &&
            event_type <= 1 &&
            (priority > 1 || move == 1 ||
             g_pz_fighters_engine.attack_policy_bits.bit1);
    }

    if (continuation) {
        g_pz_fighters_engine.continuation_move = move;
        g_pz_fighters_engine.continuation_priority = priority;
        g_pz_fighters_engine.attack_policy_bits.bit2 = 1;
        return;
    }

    if (move == 13) {
        if (g_pz_fighters_engine.random_fatality_active == 1 &&
            g_pz_fighters_engine.fighter_move.script_move == 13) {
            accepted = 0;
        } else {
            accepted = 1;
            for (index = 0;
                 index < g_pz_fighters_engine.pending_move_count;
                 index++) {
                if (g_pz_fighters_engine.pending_moves[index].script_move ==
                    13) {
                    accepted = 0;
                    break;
                }
            }
        }
    } else if (
        g_pz_fighters_engine.random_fatality_active == 1 &&
        g_pz_fighters_engine.fighter_move.mode != 15 &&
        priority + 3 < g_pz_fighters_engine.fighter_move.mode) {
        accepted = 0;
    } else {
        accepted = 1;
    }

    if (!accepted) {
        return;
    }

    if (g_pz_fighters_engine.random_fatality_active == 1) {
        if (priority > 1 &&
            g_pz_fighters_engine.fighter_move.has_followup == 1 &&
            g_pz_fighters_engine.fighter_move.player != player &&
            (g_pz_fighters_engine.fighter_move.mode == 1 ||
             priority > g_pz_fighters_engine.fighter_move.mode + 1)) {
            PZ_CANCEL_ACTIVE_MOVE();
        } else if ((event_type > 4 || priority > 2) &&
                   g_pz_fighters_engine.fighter_move.has_followup == 1 &&
                   g_pz_fighters_engine.fighter_move.player == player &&
                   g_pz_fighters_engine.fighter_move.mode == 1) {
            PZ_CANCEL_ACTIVE_MOVE();
        }
    }

    if (g_pz_fighters_engine.random_fatality_active == 1 &&
        event_type > 4) {
        return;
    }

    if (priority == 15) {
        g_pz_fighters_engine.pending_move_count = 0;
    } else {
        index = 0;
        while (index < g_pz_fighters_engine.pending_move_count) {
            if (g_pz_fighters_engine.pending_moves[index].mode == 1) {
                for (copy_index = index;
                     copy_index <
                     g_pz_fighters_engine.pending_move_count;
                     copy_index++) {
                    memcpy(
                        &g_pz_fighters_engine.pending_moves[copy_index],
                        &g_pz_fighters_engine.pending_moves[copy_index + 1],
                        sizeof(PuzzleFighterMove));
                }
                g_pz_fighters_engine.pending_move_count--;
            } else {
                index++;
            }
        }
    }

    if (g_pz_fighters_engine.random_fatality_active == 1) {
        if (g_pz_fighters_engine.fighter_move.player != player) {
            if (priority == 11) {
                if (g_pz_fighters_engine.fighter_move.mode <= 9) {
                    g_pz_fighters_engine.breakout = 1;
                }
            } else if (priority == 12 || priority == 13) {
                if (g_pz_fighters_engine.fighter_move.mode <= 11) {
                    g_pz_fighters_engine.breakout = 1;
                }
            } else if (priority == 14 || priority == 15) {
                if (g_pz_fighters_engine.fighter_move.mode <= 13) {
                    g_pz_fighters_engine.breakout = 1;
                }
            }
        } else if ((priority == 14 || priority == 15) &&
                   g_pz_fighters_engine.fighter_move.mode <= 13) {
            g_pz_fighters_engine.breakout = 1;
        }
    }

    if (event_type == 3) {
        pz_fighter_buffer_new_move(1, player, move, priority);
    } else if (event_type == 8) {
        pz_fighter_buffer_new_move(11, player, 8, 15);
    } else if (event_type == 9) {
        pz_fighter_buffer_new_move(19, player, 9, 15);
    } else if (event_type == 6) {
        pz_fighter_buffer_new_move(7, player, 6, 15);
    } else if (event_type == 7) {
        pz_fighter_buffer_new_move(10, player, 7, 15);
    } else if (event_type == 5) {
        pz_fighter_buffer_new_move(6, player, 5, 15);
    } else {
        pz_fighter_buffer_new_move(3, player, move, priority);
    }
}

/*
 * Queue a board event in descending priority order. Retail keeps at most two
 * pending moves; a newly inserted third entry drops the lowest-priority tail.
 */
static void pz_fighter_buffer_new_move(
    unsigned int event_type, unsigned int player, unsigned int move,
    unsigned int priority) {
    PuzzleFighterMove pending;
    int found;
    unsigned int insert;
    unsigned int index;

    found = 0;
    pending.event_type = event_type;
    pending.player = player;
    pending.mode = priority;
    pending.script_move = move;

    if (g_pz_fighters_engine.pending_move_count == 0) {
        memcpy(
            &g_pz_fighters_engine.pending_moves[
                g_pz_fighters_engine.pending_move_count],
            &pending, sizeof(PuzzleFighterMove));
        g_pz_fighters_engine.pending_move_count++;
        return;
    }

    insert = 0;
    for (index = 0; index < g_pz_fighters_engine.pending_move_count; index++) {
        if (g_pz_fighters_engine.pending_moves[index].mode < priority) {
            found = 1;
            break;
        }
        insert++;
    }

    if (found == 1) {
        index = g_pz_fighters_engine.pending_move_count - 1;
        while (index > insert) {
            memcpy(
                &g_pz_fighters_engine.pending_moves[index],
                &g_pz_fighters_engine.pending_moves[index - 1],
                sizeof(PuzzleFighterMove));
            index--;
        }
        memcpy(
            &g_pz_fighters_engine.pending_moves[insert], &pending,
            sizeof(PuzzleFighterMove));
        g_pz_fighters_engine.pending_move_count++;
        if (g_pz_fighters_engine.pending_move_count > 2) {
            g_pz_fighters_engine.pending_move_count = 2;
        }
    } else if (g_pz_fighters_engine.pending_move_count < 2) {
        memcpy(
            &g_pz_fighters_engine.pending_moves[
                g_pz_fighters_engine.pending_move_count],
            &pending, sizeof(PuzzleFighterMove));
        g_pz_fighters_engine.pending_move_count++;
    }
}


/*
 * Soft ceiling: 85.81% -- classify a board event by chain depth and
 * cleared-block count, producing the scripted move and its queue priority.
 */
void pz_fighter_classify_move_8012260C(
    unsigned int block_count, int chain_count, unsigned int* move,
    unsigned int* priority, unsigned int event_type) {
    unsigned short roll;

    if (event_type == 2) {
        *move = 13;
        *priority = 16;
        return;
    }
    if (event_type == 3) {
        *move = 0;
        *priority = 13;
        return;
    }
    if ((event_type - 5 <= 3) || event_type == 9) {
        *move = 0;
        *priority = 15;
        return;
    }

    switch (chain_count) {
    case 0:
        *move = 1;
        if (block_count == 0) {
            *move = 0;
            *priority = 1;
            return;
        }
        if (block_count <= 11) {
            roll = (unsigned short)randu0(100);
            if (block_count <= 4) {
                *priority = 2;
            } else if (block_count <= 8) {
                *priority = 3;
            } else {
                *priority = 4;
            }
            if (roll < 30) {
                *move = 2;
            } else if (roll < 60 && *priority <= 2) {
                *move = 3;
            } else if (roll < 60 && *priority >= 3) {
                *move = 4;
            }
            return;
        }
        if (block_count <= 12) {
            roll = (unsigned short)randu0(100);
            if (roll < 60) {
                *move = 4;
            } else {
                *move = 8;
                if ((unsigned short)randu0(100) < 25) {
                    (*move)++;
                }
            }
            *priority = 6;
            return;
        }
        if (block_count <= 20) {
            *move = 8;
            if ((unsigned short)randu0(100) < 25) {
                (*move)++;
            }
            *priority = 8;
            return;
        }
        if (block_count <= 30) {
            *move = 10;
            if ((unsigned short)randu0(100) < 40) {
                (*move)++;
            }
            *priority = 9;
            return;
        }
        *priority = 13;
        *move = 11;
        if ((unsigned short)randu0(100) < 70) {
            (*move)++;
        }
        return;
    case 1:
        *move = 5;
        if (block_count <= 5) {
            *priority = 5;
        } else if (block_count <= 7) {
            *priority = 6;
        } else if (block_count <= 9) {
            *priority = 7;
            (*move)++;
        } else {
            *move += 2;
            *priority = 10;
        }
        return;
    case 2:
        *move = 7;
        if (block_count <= 8) {
            *priority = 8;
        } else if (block_count <= 10) {
            *priority = 8;
            (*move)++;
        } else {
            *priority = 11;
            *move += 2;
        }
        return;
    case 3:
        *move = 9;
        if (block_count <= 11) {
            *priority = 9;
        } else {
            *priority = 10;
            (*move)++;
        }
        return;
    case 4:
        *move = 10;
        *priority = 12;
        return;
    case 5:
        *move = 11;
        *priority = 13;
        return;
    default:
        *move = 12;
        *priority = 15;
        return;
    }
}

/*
 * Soft ceiling: complete master scheduler. Remaining differences are the
 * clean validity/queue expressions versus retail compare/address emission,
 * stack/FPR scheduling, common-return shaping, and local labels.
 */
static float p_puzzle_fighter_master(void) {
    int state1;
    int state2;
    int pending_active;
    int super_ready;
    int balance_timer;
    unsigned int i;

    if (g_pz_fighters_engine.round_running == 0) {
        if (g_pz_fighters_engine.fatality_abort == 1) {
            if (g_pz_fighters_engine.fatality_active == 0) {
                pz_fighters_fatality_prep_chores();
            } else {
                g_pz_fighters_engine.fatality_timer--;
                if (g_pz_fighters_engine.fatality_timer == 0) {
                    g_pz_fighters_engine.fatality_abort = 0;
                    return -1.0f;
                }
                pz_fighters_fatality_in_progress();
            }
        }

        if (((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state == 0 &&
            ((PlyrPdata*)g_game_info.plyr1.slot.fighter)
                    ->fatality_shove_active == 1) {
            xfer_proc(
                g_game_info.plyr1.idle_proc,
                p_plyr_pz_fighter_entry);
        }
        if (((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state == 0 &&
            ((PlyrPdata*)g_game_info.plyr0.slot.fighter)
                    ->fatality_shove_active == 1) {
            xfer_proc(
                g_game_info.plyr0.idle_proc,
                p_plyr_pz_fighter_entry);
        }
        return 1.0f;
    }

    g_pz_fighters_engine.fatality_timer = 0;
    g_pz_fighters_engine.constraint_timer--;
    check_fighter_constraints();

    if (g_pz_fighters_engine.immediate_request_active == 1) {
        if (((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state == 0 &&
            ((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state == 0) {
            pz_fighter_process_immediate_request();
        } else {
            g_pz_fighters_engine.immediate_request_timer--;
            if (g_pz_fighters_engine.immediate_request_timer == 0) {
                pz_fighter_process_immediate_request();
            }
        }
        return 1.0f;
    }

    balance_timer = g_pz_fighters_engine.balance_update_timer - 1;
    g_pz_fighters_engine.balance_update_timer = balance_timer;
    if ((float)balance_timer == 0.0f) {
        int balance_step =
            (int)(g_pz_fighters_engine.balance -
                  g_pz_fighters_engine.pending_balance);

        g_pz_fighters_engine.balance_update_timer = 360;
        g_pz_fighters_engine.balance_out_of_range =
            (float)balance_step > 0.1f ||
            (float)balance_step < -0.1f;
        g_pz_fighters_engine.balance = g_pz_fighters_engine.pending_balance;
        pz_fighter_calculate_start_pos();
    }

    if (pz_fighter_check_for_player_to_center_position_control() == 1) {
        return 1.0f;
    }

    state1 = g_pz_fighters_engine.fighter_state[0];
    state2 = g_pz_fighters_engine.fighter_state[1];
    if (state1 != 0 || state2 != 0) {
        if (((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state == 0 &&
            ((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state == 0) {
            g_pz_fighters_engine.fighter_state[0] = 0;
            g_pz_fighters_engine.fighter_state[1] = 0;
            return 1.0f;
        }

        if (state1 == 0 && state2 == 4) {
            pz_fighter_individual_plyr_do_something(0, state2);
            return 1.0f;
        }
        if (state2 == 0 && state1 == 4) {
            pz_fighter_individual_plyr_do_something(1, state1);
            return 1.0f;
        }

        pending_active = 0;
        for (i = 0; i < g_pz_fighters_engine.pending_move_count; i++) {
            if (g_pz_fighters_engine.pending_moves[i].event_type == 1) {
                pending_active = 1;
                break;
            }
        }
        if (pending_active == 1 &&
            (state1 == 0 || state1 == 1 || state1 == 5) &&
            (state2 == 0 || state2 == 1 || state2 == 5)) {
            xfer_proc(
                g_game_info.plyr0.idle_proc,
                p_plyr_pz_fighter_entry);
            xfer_proc(
                g_game_info.plyr1.idle_proc,
                p_plyr_pz_fighter_entry);
            g_pz_fighters_engine.fighter_state[0] = 9;
            g_pz_fighters_engine.fighter_state[1] = 9;
        } else if (state1 == 0 && state2 == 1) {
            pz_fighter_individual_plyr_do_something(0, state2);
        } else if (state1 == 1 && state2 == 0) {
            pz_fighter_individual_plyr_do_something(1, state1);
        }
        return 1.0f;
    }

    g_pz_fighters_engine.random_fatality_active = 0;
    g_pz_fighters_engine.breakout = 0;
    g_pz_fighters_engine.balance_out_of_range = 0;
    g_pz_fighters_engine.attack_policy_word = 0;

    if (puzzle_fighter_get_super_bar_level(0) > 0.95f) {
        super_ready = 1;
    } else {
        super_ready = 0;
    }
    if (super_ready != 1) {
        if (puzzle_fighter_get_super_bar_level(1) > 0.95f) {
            super_ready = 1;
        } else {
            super_ready = 0;
        }
    }
    if (super_ready == 1 &&
        g_pz_fighters_engine.super_move_active == 0) {
        pz_fighters_inside_super_move_scenerio();
    } else if (g_pz_fighters_engine.pending_move_count != 0) {
        pz_fighters_handle_next_pending_move();
    } else {
        pz_fighters_idle_process();
    }
    return 1.0f;
}

/*
 * Soft ceiling: complete center-control policy. Remaining differences are
 * equivalent short-circuit/return emission, GPR/FPR allocation, and labels.
 */
static int pz_fighter_check_for_player_to_center_position_control(void) {
    float player1_distance;
    float player2_distance;
    float player1_absolute;
    float player2_absolute;
    float player_distance;
    float player1_wall;
    float player2_wall;
    float dx;
    float dz;
    unsigned int player1_busy;
    unsigned int player2_busy;
    int super_ready;

    if (g_pz_fighters_engine.fighter_state[0] == 8 ||
        g_pz_fighters_engine.fighter_state[1] == 8 ||
        g_pz_fighters_engine.fighter_state[0] == 2 ||
        g_pz_fighters_engine.fighter_state[1] == 2) {
        return 0;
    }
    if (puzzle_fighter_get_super_bar_level(0) > 0.98f) {
        super_ready = 1;
    } else {
        super_ready = 0;
    }
    if (super_ready == 1) {
        return 0;
    }
    if (puzzle_fighter_get_super_bar_level(1) > 0.98f) {
        super_ready = 1;
    } else {
        super_ready = 0;
    }
    if (super_ready == 1 ||
        g_pz_fighters_engine.super_move_active == 1 ||
        ((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state == 0x605 ||
        ((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state == 0x605) {
        return 0;
    }

    if (g_pz_fighters_engine.fighter_state[0] == 0 ||
        g_pz_fighters_engine.fighter_state[0] == 1) {
        player1_busy = 0;
    } else {
        player1_busy = 10;
    }
    if (g_pz_fighters_engine.fighter_state[1] == 0 ||
        g_pz_fighters_engine.fighter_state[1] == 1) {
        player2_busy = 0;
    } else {
        player2_busy = 10;
    }
    if (player1_busy != 0 && player2_busy != 0) {
        return 0;
    }

    dz = ((MkObj*)g_game_info.plyr0.slot.mirror_a)->pos.z -
         g_pz_fighters_engine.player1_idle_z;
    dx = ((MkObj*)g_game_info.plyr0.slot.mirror_a)->pos.x -
         g_pz_fighters_engine.player1_idle_x;
    player1_distance = dx * dx + dz * dz;
    player1_absolute = player1_distance;
    if (dx > 0.0f) {
        player1_distance *= -1.0f;
    }
    dz = ((MkObj*)g_game_info.plyr1.slot.mirror_a)->pos.z -
         g_pz_fighters_engine.player2_idle_z;
    dx = ((MkObj*)g_game_info.plyr1.slot.mirror_a)->pos.x -
         g_pz_fighters_engine.player2_idle_x;
    player2_distance = dx * dx + dz * dz;
    player2_absolute = player2_distance;
    if (dx < 0.0f) {
        player2_distance *= -1.0f;
    }

    if (player1_absolute > 0.8f && player2_absolute > 0.8f) {
        player_distance = xz_distance_between_players();
        if (player_distance < 2.0f) {
            dx = g_pz_fighters_engine.fighter_posts[0].x -
                 ((MkObj*)g_game_info.plyr0.slot.mirror_a)->pos.x;
            dz = g_pz_fighters_engine.fighter_posts[0].z -
                 ((MkObj*)g_game_info.plyr0.slot.mirror_a)->pos.z;
            player1_wall = dx * dx + dz * dz;
            dx = g_pz_fighters_engine.fighter_posts[1].x -
                 ((MkObj*)g_game_info.plyr1.slot.mirror_a)->pos.x;
            dz = g_pz_fighters_engine.fighter_posts[1].z -
                 ((MkObj*)g_game_info.plyr1.slot.mirror_a)->pos.z;
            player2_wall = dx * dx + dz * dz;

            if (player1_wall > 6.8f && player1_busy == 0 &&
                player1_distance > 0.0f) {
                g_pz_fighters_engine.fighter_move.player = 0;
                pz_fighter_handle_off_wall_attack(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
            if (player2_wall > 6.8f && player2_busy == 0 &&
                player2_distance > 0.0f) {
                g_pz_fighters_engine.fighter_move.player = 1;
                pz_fighter_handle_off_wall_attack(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
            if (player1_wall <= 6.8f && player1_busy == 0 &&
                player1_distance > 0.0f) {
                g_pz_fighters_engine.fighter_move.player = 0;
                pz_fighter_handle_distance_attack(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
            if (player2_wall <= 6.8f && player2_busy == 0 &&
                player2_distance > 0.0f) {
                g_pz_fighters_engine.fighter_move.player = 1;
                pz_fighter_handle_distance_attack(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
        } else {
            if (player1_busy == 0 && player1_distance > 0.0f) {
                g_pz_fighters_engine.fighter_move.player = 0;
                pz_fighter_handle_center_pos_range_attack(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
            if (player2_busy == 0 && player2_distance > 0.0f) {
                g_pz_fighters_engine.fighter_move.player = 1;
                pz_fighter_handle_center_pos_range_attack(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
        }
    } else if (player1_absolute > 1.45f) {
        if (xz_distance_between_players() > 1.7f) {
            if (player1_busy == 0) {
                g_pz_fighters_engine.fighter_move.player = 0;
                pz_fighter_handle_center_pos_single_range_move(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
        } else if (player1_busy == 0) {
            g_pz_fighters_engine.fighter_move.player = 0;
            pz_fighter_handle_center_pos_single_close_move(
                &g_pz_fighters_engine.fighter_move);
            return 1;
        }
    } else if (player2_absolute > 1.45f) {
        if (xz_distance_between_players() > 1.7f) {
            if (player2_busy == 0) {
                g_pz_fighters_engine.fighter_move.player = 1;
                pz_fighter_handle_center_pos_single_range_move(
                    &g_pz_fighters_engine.fighter_move);
                return 1;
            }
        } else if (player2_busy == 0) {
            g_pz_fighters_engine.fighter_move.player = 1;
            pz_fighter_handle_center_pos_single_close_move(
                &g_pz_fighters_engine.fighter_move);
            return 1;
        }
    } else if (player2_distance > 0.4f &&
               g_pz_fighters_engine.balance_out_of_range == 1) {
        if (player2_busy == 0) {
            g_pz_fighters_engine.fighter_move.player = 1;
            pz_fighter_handle_center_pos_minor_adjustment(
                &g_pz_fighters_engine.fighter_move);
            g_pz_fighters_engine.balance_out_of_range = 0;
            return 1;
        }
    } else if (player1_distance > 0.4f &&
               g_pz_fighters_engine.balance_out_of_range == 1) {
        if (player1_busy == 0) {
            g_pz_fighters_engine.fighter_move.player = 0;
            pz_fighter_handle_center_pos_minor_adjustment(
                &g_pz_fighters_engine.fighter_move);
            g_pz_fighters_engine.balance_out_of_range = 0;
            return 1;
        }
    } else if ((player1_distance > 0.08f && player2_distance > 0.15f) ||
               (player1_distance > 0.15f && player2_distance > 0.08f)) {
        if (player1_busy == 0 && player2_busy == 0) {
            g_pz_fighters_engine.fighter_move.player = randu0(1) & 0xffff;
            pz_fighter_handle_dual_off_center_Move(
                &g_pz_fighters_engine.fighter_move);
            return 1;
        }
    }
    return 0;
}

/*
 * Soft ceiling: retail behavior and ABI are complete. Remaining differences
 * are FPR allocation, equivalent bound/return scheduling, and float labels.
 */
static int pz_fighter_individual_plyr_do_something(
    unsigned int player, unsigned int other_state) {
    int result;
    float player_distance;
    float signed_distance;

    player_distance = xz_distance_between_players();
    signed_distance = pz_fighter_signed_idle_distance(player);

    if (player_distance > 1.45f &&
        signed_distance > -1.0f && signed_distance < 1.0f) {
        if (g_pz_fighters_engine.constraint_timer <= 0 &&
            other_state == 4 &&
            (unsigned short)randu0(100) < 50) {
            if (player == 0) {
                g_pz_fighters_engine.fighter_state[0] = 2;
            } else {
                g_pz_fighters_engine.fighter_state[1] = 2;
            }
            xfer_proc(
                pz_fighter_player_proc(player), pz_fighter_laugh_small);
            g_pz_fighters_engine.constraint_timer = 360;
            return 1;
        }
        if (g_pz_fighters_engine.constraint_timer <= 60) {
            if (player == 0) {
                g_pz_fighters_engine.fighter_state[0] = 2;
            } else {
                g_pz_fighters_engine.fighter_state[1] = 2;
            }
            xfer_proc(
                pz_fighter_player_proc(player), pz_fighter_random_taunt);
            g_pz_fighters_engine.constraint_timer = 300;
            return 1;
        }
    }

    if (other_state == 4) {
        signed_distance = pz_fighter_signed_idle_distance(player);
        if (signed_distance < -0.1f) {
            if (player == 0) {
                g_pz_fighters_engine.fighter_state[0] = 1;
            } else if (player == 1) {
                g_pz_fighters_engine.fighter_state[1] = 1;
            }
            xfer_proc(
                pz_fighter_player_proc(player),
                pz_fighter_move_into_desired_position);
            return 1;
        }
        return 0;
    }

    signed_distance = pz_fighter_signed_idle_distance(player);
    result = 0;
    if (signed_distance < -0.1f || signed_distance > 0.1f) {
        if (player == 0) {
            g_pz_fighters_engine.fighter_state[0] = 1;
        } else if (player == 1) {
            g_pz_fighters_engine.fighter_state[1] = 1;
        }
        xfer_proc(
            pz_fighter_player_proc(player),
            pz_fighter_move_into_desired_position);
        result = 1;
    }
    return result;
}

/* Broad pass: restore idle spacing before re-entering the mode master. */
/*
 * Soft ceiling: complete positioning/random-event state machine; remaining
 * differences are GPR/FPR allocation, final-call scheduling, and float labels.
 */
static int pz_fighters_idle_process(void) {
    PuzzleFightersEngine* fighters;
    MkObj* player1;
    MkObj* player2;
    float dx;
    float dz;
    float player1_distance;
    float player2_distance;
    int moved;

    fighters = &g_pz_fighters_engine;
    if (fighters->positioning_active == 0) {
        moved = 0;
        fighters->fighters_positioned = 0;
        player1 = (MkObj*)g_game_info.plyr0.slot.mirror_a;
        dz = player1->pos.z - fighters->player1_idle_z;
        dx = player1->pos.x - fighters->player1_idle_x;
        player1_distance = dx * dx + dz * dz;
        if (dx > 0.0f) {
            player1_distance *= -1.0f;
        }
        player2 = (MkObj*)g_game_info.plyr1.slot.mirror_a;
        dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
        dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
        player2_distance = dx * dx + dz * dz;
        if (dx < 0.0f) {
            player2_distance *= -1.0f;
        }
        if (player1_distance < -0.1f || player1_distance > 0.1f) {
            g_pz_fighters_engine.fighter_state[0] = 1;
            xfer_proc(
                g_game_info.plyr0.idle_proc,
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (player2_distance < -0.1f || player2_distance > 0.1f) {
            g_pz_fighters_engine.fighter_state[1] = 1;
            xfer_proc(
                g_game_info.plyr1.idle_proc,
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (moved != 0) {
            fighters->positioning_active = 1;
            return 1;
        }
    }
    fighters->positioning_active = 0;
    g_pz_fighters_engine.fighters_positioned = 1;
    if (pz_fighter_check_fatality_random_event(fighters, 0) == 1) {
        return 1;
    }
    return 1;
}

/*
 * Soft ceiling: complete positioning and super-owner state machine; remaining
 * differences are saved-GPR selection, boolean scheduling, and float labels.
 */
static int pz_fighters_inside_super_move_scenerio(void) {
    PuzzleFightersEngine* fighters;
    MkObj* player1;
    MkObj* player2;
    float dx;
    float dz;
    float player1_distance;
    float player2_distance;
    int moved;
    int player;
    int ready;
    int other_ready;
    int dispatch;

    fighters = &g_pz_fighters_engine;
    if (fighters->positioning_active == 0) {
        moved = 0;
        fighters->fighters_positioned = 0;
        player1 = (MkObj*)g_game_info.plyr0.slot.mirror_a;
        dz = player1->pos.z - fighters->player1_idle_z;
        dx = player1->pos.x - fighters->player1_idle_x;
        player1_distance = dx * dx + dz * dz;
        if (dx > 0.0f) {
            player1_distance *= -1.0f;
        }
        player2 = (MkObj*)g_game_info.plyr1.slot.mirror_a;
        dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
        dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
        player2_distance = dx * dx + dz * dz;
        if (dx < 0.0f) {
            player2_distance *= -1.0f;
        }
        if (player1_distance < -0.1f || player1_distance > 0.1f) {
            g_pz_fighters_engine.fighter_state[0] = 1;
            xfer_proc(
                g_game_info.plyr0.idle_proc,
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (player2_distance < -0.1f || player2_distance > 0.1f) {
            g_pz_fighters_engine.fighter_state[1] = 1;
            xfer_proc(
                g_game_info.plyr1.idle_proc,
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (moved != 0) {
            fighters->positioning_active = 1;
            return 1;
        }
    }

    fighters->positioning_active = 0;
    g_pz_fighters_engine.fighters_positioned = 1;
    if (pz_fighter_check_fatality_random_event(fighters, 0) == 1) {
        return 1;
    }
    if (fighters->pending_move_count != 0) {
        pz_fighters_handle_next_pending_move_simplified();
        return 1;
    }

    if (fighters->super_move_request_pending == 1) {
        if (puzzle_fighter_get_super_bar_level(0) > 0.98f) {
            ready = 1;
        } else {
            ready = 0;
        }
        if (ready != 0) {
            if (puzzle_fighter_get_super_bar_level(1) > 0.95f) {
                other_ready = 1;
            } else {
                other_ready = 0;
            }
            if (other_ready == 0) {
                player = 0;
                dispatch = 1;
            } else {
                dispatch = 0;
            }
        } else {
            dispatch = 0;
        }
        if (dispatch == 0) {
            if (puzzle_fighter_get_super_bar_level(1) > 0.98f) {
                ready = 1;
            } else {
                ready = 0;
            }
            if (ready != 0) {
                if (puzzle_fighter_get_super_bar_level(0) > 0.95f) {
                    other_ready = 1;
                } else {
                    other_ready = 0;
                }
                if (other_ready == 0) {
                    player = 1;
                    dispatch = 1;
                }
            }
        }
        if (dispatch != 0) {
            fighters->random_fatality_active = 1;
            fighters->fighter_move.mode = 2;
            fighters->fighter_move.player = player;
            pz_fighter_handle_in_super_move(&fighters->fighter_move);
            fighters->super_move_request_pending = 0;
        }
    }
    return 0;
}

#define PZ_POP_PENDING_EVENT(fighters)                                    \
    do {                                                                  \
        unsigned int pending_index;                                       \
        (fighters)->fighters_positioned = 0;                              \
        memcpy(                                                           \
            &(fighters)->fighter_move, &(fighters)->pending_moves[0],     \
            sizeof(PuzzleFighterMove));                                   \
        for (pending_index = 1;                                           \
             pending_index < (fighters)->pending_move_count;              \
             pending_index++) {                                           \
            memcpy(                                                       \
                &(fighters)->pending_moves[pending_index - 1],            \
                &(fighters)->pending_moves[pending_index],                \
                sizeof(PuzzleFighterMove));                               \
        }                                                                 \
        (fighters)->pending_move_count--;                                 \
        (fighters)->attack_has_followup = 1;                              \
        (fighters)->random_fatality_active = 0;                           \
        (fighters)->breakout = 0;                                         \
        (fighters)->attack_policy_word = 0;                               \
    } while (0)

#define PZ_DISPATCH_BOMB_REACTION(fighters)                               \
    do {                                                                  \
        pz_fighter_shake_camera(3, 0.02f);                                \
        g_pz_fighters_engine.fighter_move.mode = 15;                      \
        if (((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state != 0x605 || \
            g_pz_fighters_engine.attack_policy_bits.bit4) {               \
            g_pz_fighters_engine.fighter_state[0] = 3;                    \
            ((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state |= 0x1200; \
            xfer_proc(                                                    \
                g_game_info.plyr0.idle_proc,                              \
                pz_fighters_react_to_bomb_explosion);                    \
        }                                                                 \
        if (((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state != 0x605 || \
            g_pz_fighters_engine.attack_policy_bits.bit4) {               \
            g_pz_fighters_engine.fighter_state[1] = 3;                    \
            ((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state |= 0x1200; \
            xfer_proc(                                                    \
                g_game_info.plyr1.idle_proc,                              \
                pz_fighters_react_to_bomb_explosion);                    \
        }                                                                 \
    } while (0)

#define PZ_DISPATCH_HAPPY_REACTION(fighters)                              \
    do {                                                                  \
        unsigned int happy_player =                                      \
            g_pz_fighters_engine.fighter_move.player;                    \
        PlyrPdata* happy_pdata = puzzle_player_pdata(happy_player);       \
        PlyrPdata* unhappy_pdata = happy_pdata->his_plyr_pdata;           \
        pz_fighter_shake_camera(3, 0.02f);                                \
        g_pz_fighters_engine.fighter_move.mode = 15;                      \
        if (happy_pdata->state != 0x605) {                                \
            g_pz_fighters_engine.fighter_state[0] = 2;                    \
            happy_pdata->state |= 0x1200;                                \
            xfer_proc(                                                    \
                pz_fighter_player_proc(happy_player),                    \
                pz_fighter_big_time_happy);                              \
        }                                                                 \
        if (unhappy_pdata->state != 0x605) {                              \
            g_pz_fighters_engine.fighter_state[1] = 4;                    \
            unhappy_pdata->state |= 0x1200;                              \
            xfer_proc(                                                    \
                pz_fighter_player_proc(unhappy_pdata->plyr_num),         \
                pz_fighter_whatever2);                                   \
        }                                                                 \
    } while (0)

void pz_fighter_anim_object_to(
    unsigned int player,
    int mirror,
    int frame,
    const Vec* start,
    const Vec* target,
    const Vec* velocity,
    int minimum_velocity_y,
    unsigned int target_ticks,
    float frame_rate,
    float gravity_step,
    int bounce,
    PuzzleObjectArrivalFn arrival) {
    PuzzleObjectMotion* motion;
    ScreenObj** object_slot;
    AniTextureControl** texture_slot;
    PuzzleProcess* proc;

    if (player >= 2) {
        return;
    }

    object_slot = &g_pz_fighters_engine.screen_objects[player];
    insert_screen_obj(*object_slot);
    texture_slot = &g_pz_fighters_engine.texture_controls[player];
    set_ani_texture_frame(*texture_slot, frame);
    insert_ani_texture_control(*texture_slot);
    set_ani_texture_framerate(*texture_slot, frame_rate);

    proc = _create_mkproc_generic_tinystack(
        0xC001, 0x1F, (PuzzleFighterEntry)p_objects_moving,
        sizeof(PuzzleObjectMotion), (MkHdr**)&motion);
    if (proc != 0 && motion != 0) {
        motion->start.x = start->x;
        motion->start.y = start->y;
        motion->start.z = start->z;
        motion->target.x = target->x;
        motion->target.y = target->y;
        motion->target.z = target->z;
        motion->arrival = arrival;
        motion->object = *object_slot;
        motion->texture_control = *texture_slot;
        if (velocity == 0) {
            motion->velocity_x =
                (target->x - start->x) / (float)target_ticks;
            motion->velocity_y =
                (target->y - start->y) / (float)target_ticks;
        } else {
            motion->velocity_x = velocity->x;
            motion->velocity_y = velocity->y;
        }

        motion->object->flags =
            (motion->object->flags & ~0x20) | ((mirror << 5) & 0x20);
        if (mirror == 1 && motion->object->scale_x > 0.0f) {
            motion->object->scale_x *= -1.0f;
            motion->object->pfx2d->mirror = 1;
        } else if (mirror == 0 && motion->object->scale_x < 0.0f) {
            motion->object->scale_x *= -1.0f;
            motion->object->pfx2d->mirror = 1;
        }

        motion->gravity_step = gravity_step;
        motion->gravity_accumulator = 0.0f;
        motion->object->x = (int)start->x;
        motion->object->y = (int)start->y;
        motion->minimum_velocity_y = minimum_velocity_y;
        motion->bounce_enabled = bounce;
        motion->rise_ticks = -1;
        motion->fall_ticks = 0;
        motion->lifetime = 1200;
        motion->target_ticks = target_ticks;
        motion->complete = 0;
        motion->arrived = 0;
    }
}

static float p_objects_moving(double conversion_bias) {
    PuzzleObjectMotion* motion;
    ScreenObj* object;

    motion = (PuzzleObjectMotion*)apdata;
    if (motion->complete == 1) {
        return -1.0f;
    }

    motion->lifetime--;
    if (motion->lifetime <= 0) {
        pull_screen_obj(motion->object);
        pull_ani_texture_control(motion->texture_control);
        return -1.0f;
    }

    object = motion->object;
    if (motion->target_ticks > 0) {
        motion->velocity_x =
            (motion->target.x - (float)object->x) /
            (float)motion->target_ticks;
        motion->velocity_y =
            (motion->target.y - (float)object->y) /
            (float)motion->target_ticks;
    }
    object->x += (int)motion->velocity_x;
    object->y += (int)motion->velocity_y;
    motion->target_ticks--;

    if (motion->bounce_enabled == 1) {
        if (motion->rise_ticks > 0) {
            motion->rise_ticks--;
            object->y++;
        } else if (motion->rise_ticks == 0) {
            motion->fall_ticks = 8;
            motion->rise_ticks = -1;
        }
        if (motion->fall_ticks > 0) {
            motion->fall_ticks--;
            object->y--;
        } else if (motion->fall_ticks == 0) {
            motion->rise_ticks = 10;
            motion->fall_ticks = -1;
        }
    }

    motion->gravity_accumulator += motion->gravity_step;
    if (motion->gravity_accumulator > 100.0f) {
        motion->gravity_accumulator = 0.0f;
        motion->velocity_y -= 1.0f;
    }
    if (motion->velocity_y < (float)motion->minimum_velocity_y) {
        motion->velocity_y = (float)motion->minimum_velocity_y;
    }

    if (motion->arrived == 0 &&
        (((float)object->x >= motion->target.x &&
          motion->velocity_x > 0.0f) ||
         ((float)object->x <= motion->target.x &&
          motion->velocity_x < 0.0f) ||
         ((float)object->y >= motion->target.y &&
          motion->velocity_y > 0.0f) ||
         ((float)object->y <= motion->target.y &&
          motion->velocity_y < 0.0f))) {
        if (motion->arrival != 0) {
            motion->arrival(object->y, conversion_bias);
        }
        motion->lifetime = 30;
        motion->arrived = 1;
    }

    return 1.0f;
}

static void check_fighter_constraints(void) {
    unsigned int player;

    for (player = 0; player < 2; player++) {
        MkObj* fighter = puzzle_fighter_object(player);
        float distance =
            g_pz_fighters_engine.constraint_axis_x *
                (g_pz_fighters_engine.fighter_posts[0].x - fighter->pos.x) +
            g_pz_fighters_engine.constraint_axis_z *
                (g_pz_fighters_engine.fighter_posts[0].z - fighter->pos.z);

        if (distance > 0.01) {
            float move_x;
            float move_z;

            distance /= 5.0f;
            move_x = g_pz_fighters_engine.constraint_axis_x * distance;
            move_z = g_pz_fighters_engine.constraint_axis_z * distance;
            fighter->pos.x = fighter->pos.x + move_x;
            fighter->pos.z = fighter->pos.z + move_z;
        } else if (distance < 0.002) {
            float move_x =
                g_pz_fighters_engine.constraint_axis_x * distance;
            float move_z =
                g_pz_fighters_engine.constraint_axis_z * distance;

            fighter->pos.x = fighter->pos.x + move_x;
            fighter->pos.z = fighter->pos.z + move_z;
        }

        if (g_pz_fighters_engine.y_constraint_enabled[player] == 1 &&
            fighter->pos.y > g_pz_fighters_engine.y_constraint[player]) {
            fighter->pos.y -= 0.02f;
        }
    }

    if (puzzle_fighter_object(0)->pos.x -
            puzzle_fighter_object(1)->pos.x >
        1.0f) {
        float first_x = puzzle_fighter_object(0)->pos.x;
        float first_y = puzzle_fighter_object(0)->pos.y;
        float first_z = puzzle_fighter_object(0)->pos.z;

        puzzle_fighter_object(0)->pos.x = puzzle_fighter_object(1)->pos.x;
        puzzle_fighter_object(0)->pos.y = puzzle_fighter_object(1)->pos.y;
        puzzle_fighter_object(0)->pos.z = puzzle_fighter_object(1)->pos.z;
        puzzle_fighter_object(1)->pos.x = first_x;
        puzzle_fighter_object(1)->pos.y = first_y;
        puzzle_fighter_object(1)->pos.z = first_z;
    }
}

void pz_fighter_set_y_constrain(MkObj* fighter, int enabled, float y) {
    int player;

    player = 0;
    if ((int)fighter->oid == 0x1002) {
        player = 1;
    }
    if (enabled == 0) {
        fighter->flags_09_bits.launched = 1;
        g_pz_fighters_engine.y_constraint_enabled[player] = 0;
        return;
    }

    fighter->flags_09_bits.launched = 0;
    g_pz_fighters_engine.y_constraint_enabled[player] = 1;
    g_pz_fighters_engine.y_constraint[player] = y;
}

/*
 * Soft ceiling: retail policy and control flow are complete. Remaining
 * differences are handled-flag/GPR allocation, equivalent return and
 * bitfield-store scheduling, and float relocation labels.
 */
static void pz_fighter_process_immediate_request(void) {
    unsigned int happy_player;
    PlyrPdata* happy_pdata;
    PlyrPdata* unhappy_pdata;
    int handled;
    int constraint_index1;
    int constraint_index2;

    if (g_pz_fighters_engine.random_fatality_active != 0 &&
        g_pz_fighters_engine.attack_policy_bits.bit4 &&
        g_pz_fighters_engine.immediate_request_type != 8 &&
        g_pz_fighters_engine.immediate_request_type != 9 &&
        (((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state == 0x605 ||
         ((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state == 0x605)) {
        return;
    }

    handled = 0;
    g_pz_fighters_engine.immediate_request_active = 0;
    if (g_pz_fighters_engine.immediate_request_type == 8) {
        pz_fighter_shake_camera(3, 0.02f);
        g_pz_fighters_engine.fighter_move.mode = 15;
        if (((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state != 0x605 ||
            g_pz_fighters_engine.attack_policy_bits.bit4) {
            g_pz_fighters_engine.fighter_state[0] = 3;
            ((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state |= 0x1200;
            xfer_proc(
                g_game_info.plyr0.idle_proc,
                pz_fighters_react_to_bomb_explosion);
            handled = 1;
        }
        if (((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state != 0x605 ||
            g_pz_fighters_engine.attack_policy_bits.bit4) {
            g_pz_fighters_engine.fighter_state[1] = 3;
            ((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state |= 0x1200;
            xfer_proc(
                g_game_info.plyr1.idle_proc,
                pz_fighters_react_to_bomb_explosion);
            handled = 1;
        }
        if (handled == 1) {
            g_pz_fighters_engine.attack_policy_word = 0;
            g_pz_fighters_engine.random_fatality_active = 1;
            g_pz_fighters_engine.attack_policy_bits.bit4 = 1;
        }
        return;
    }

    if (g_pz_fighters_engine.immediate_request_type == 9) {
        if (g_pz_fighters_engine.attack_policy_bits.bit4) {
            pz_fighter_shake_camera(3, 0.02f);
            g_pz_fighters_engine.fighter_move.mode = 15;
            if (((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state != 0x605 ||
                g_pz_fighters_engine.attack_policy_bits.bit4) {
                g_pz_fighters_engine.fighter_state[0] = 3;
                ((PlyrPdata*)g_game_info.plyr0.slot.fighter)->state |= 0x1200;
                xfer_proc(
                    g_game_info.plyr0.idle_proc,
                    pz_fighters_react_to_bomb_explosion);
                handled = 1;
            }
            if (((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state != 0x605 ||
                g_pz_fighters_engine.attack_policy_bits.bit4) {
                g_pz_fighters_engine.fighter_state[1] = 3;
                ((PlyrPdata*)g_game_info.plyr1.slot.fighter)->state |= 0x1200;
                xfer_proc(
                    g_game_info.plyr1.idle_proc,
                    pz_fighters_react_to_bomb_explosion);
                handled = 1;
            }
            if (handled == 1) {
                g_pz_fighters_engine.attack_policy_word = 0;
                g_pz_fighters_engine.random_fatality_active = 1;
                g_pz_fighters_engine.attack_policy_bits.bit4 = 1;
                return;
            }
        } else {
            happy_player = g_pz_fighters_engine.immediate_request_player;
            happy_pdata = puzzle_player_pdata(happy_player);
            unhappy_pdata = happy_pdata->his_plyr_pdata;

            pz_fighter_shake_camera(3, 0.02f);
            g_pz_fighters_engine.fighter_move.mode = 15;
            if (happy_pdata->state != 0x605) {
                g_pz_fighters_engine.fighter_state[0] = 2;
                happy_pdata->state |= 0x1200;
                xfer_proc(
                    pz_fighter_player_proc(happy_player),
                    pz_fighter_big_time_happy);
                handled = 1;
            }
            if (unhappy_pdata->state != 0x605) {
                g_pz_fighters_engine.fighter_state[1] = 4;
                unhappy_pdata->state |= 0x1200;
                xfer_proc(
                    pz_fighter_player_proc(unhappy_pdata->plyr_num),
                    pz_fighter_whatever2);
                handled = 1;
            }
            if (handled == 1) {
                g_pz_fighters_engine.attack_policy_word = 0;
                g_pz_fighters_engine.random_fatality_active = 1;
                g_pz_fighters_engine.attack_policy_bits.bit4 = 1;
                return;
            }
        }
        return;
    }

    if (g_pz_fighters_engine.immediate_request_type == 18) {
        constraint_index1 = 0;
        if ((int)((MkObj*)g_game_info.plyr0.slot.mirror_a)->oid == 0x1002) {
            constraint_index1 = 1;
        }
        ((MkObj*)g_game_info.plyr0.slot.mirror_a)->flags_09_bits.launched = 1;
        g_pz_fighters_engine.y_constraint_enabled[constraint_index1] = 0;
        constraint_index2 = 0;
        if ((int)((MkObj*)g_game_info.plyr1.slot.mirror_a)->oid == 0x1002) {
            constraint_index2 = 1;
        }
        ((MkObj*)g_game_info.plyr1.slot.mirror_a)->flags_09_bits.launched = 1;
        g_pz_fighters_engine.y_constraint_enabled[constraint_index2] = 0;
        g_pz_fighters_engine.random_fatality_active = 1;
        g_pz_fighters_engine.attack_policy_word = 0;
        g_pz_fighters_engine.fighter_move.mode = 6;
        g_pz_fighters_engine.fighter_move.player =
            g_pz_fighters_engine.immediate_request_player;
        pz_fighter_handle_super_move_available(&g_pz_fighters_engine.fighter_move);
        return;
    }

    if (pz_fighter_should_handle_special_move(
            g_pz_fighters_engine.immediate_request_player) == 1) {
        constraint_index1 = 0;
        if ((int)((MkObj*)g_game_info.plyr0.slot.mirror_a)->oid == 0x1002) {
            constraint_index1 = 1;
        }
        ((MkObj*)g_game_info.plyr0.slot.mirror_a)->flags_09_bits.launched = 1;
        g_pz_fighters_engine.y_constraint_enabled[constraint_index1] = 0;
        constraint_index2 = 0;
        if ((int)((MkObj*)g_game_info.plyr1.slot.mirror_a)->oid == 0x1002) {
            constraint_index2 = 1;
        }
        ((MkObj*)g_game_info.plyr1.slot.mirror_a)->flags_09_bits.launched = 1;
        g_pz_fighters_engine.y_constraint_enabled[constraint_index2] = 0;
        g_pz_fighters_engine.fighter_move.mode = 15;
        g_pz_fighters_engine.attack_policy_word = 0;
        g_pz_fighters_engine.attack_policy_bits.bit7 = 1;
        g_pz_fighters_engine.fighter_move.script_move =
            g_pz_fighters_engine.immediate_request_type;
        g_pz_fighters_engine.fighter_move.player =
            g_pz_fighters_engine.immediate_request_player;
        g_pz_fighters_engine.random_fatality_active = 1;
        pz_fighter_handle_special_move(&g_pz_fighters_engine.fighter_move);
    }
}

/*
 * Soft ceiling: complete dequeue and retail-ordered dispatch. Remaining
 * differences are queue-loop GPR/address scheduling, saves, and labels.
 */
static float pz_fighters_handle_next_pending_move(void) {
    PuzzleFighterMove* move;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;

    move = &fighters->fighter_move;
    PZ_POP_PENDING_EVENT(fighters);
    switch (move->event_type) {
    case 2:
        {
            unsigned int other_player;

            fighters->random_fatality_active = 1;
            other_player = 0;
            if (move->player == 0) {
                other_player = 1;
            }
            pz_fighter_perform_end_of_round_anims(
                move->player, other_player);
            fighters->balance = 0.0f;
            pz_fighters_fatality_round_over();
            pz_fighter_calculate_start_pos();
            fighters->round_running = 0;
            break;
        }
    case 3:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_move(move);
        break;
    case 1:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_special_move(move);
        break;
    case 6:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_relief_move(move);
        break;
    case 10:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_peak_move(move);
        break;
    case 7:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohno_move(move);
        break;
    case 11:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohyeah_move(move);
        break;
    case 19:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_winning_big_based_on_score_move(move);
        break;
    case 8:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_BOMB_REACTION(fighters);
        /* Retail intentionally continues into the paired happy reaction. */
    case 9:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_HAPPY_REACTION(fighters);
        break;
    }
    return 0.0f;
}

/*
 * Soft ceiling: complete simplified dequeue and normalized dispatch.
 * Remaining differences are queue-loop GPR/address scheduling and labels.
 */
static float pz_fighters_handle_next_pending_move_simplified(void) {
    PuzzleFighterMove* move;
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;

    move = &fighters->fighter_move;
    PZ_POP_PENDING_EVENT(fighters);
    switch (move->event_type) {
    case 3:
        {
            unsigned int script_move;

            script_move = move->script_move;
            fighters->random_fatality_active = 1;
            if (script_move > 2 && script_move < 5) {
                move->script_move = 5;
            } else if (script_move == 6) {
                move->script_move = script_move + 1;
            } else if (script_move == 8) {
                move->script_move = script_move + 1;
            } else if (script_move == 10) {
                move->script_move = script_move + 2;
            } else if (script_move == 11) {
                move->script_move = script_move + 1;
            }
            pz_fighter_handle_move(move);
            break;
        }
    case 2:
        {
            unsigned int other_player;

            fighters->random_fatality_active = 1;
            other_player = 0;
            if (move->player == 0) {
                other_player = 1;
            }
            pz_fighter_perform_end_of_round_anims(
                move->player, other_player);
            fighters->balance = 0.0f;
            pz_fighters_fatality_round_over();
            pz_fighter_calculate_start_pos();
            fighters->round_running = 0;
            break;
        }
    case 1:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_special_move(move);
        /* Retail intentionally also dispatches the relief reaction. */
    case 6:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_relief_move(move);
        break;
    case 10:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_peak_move(move);
        break;
    case 7:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohno_move(move);
        break;
    case 11:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohyeah_move(move);
        break;
    case 19:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_winning_big_based_on_score_move(move);
        break;
    case 8:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_BOMB_REACTION(fighters);
        /* Retail intentionally continues into the paired happy reaction. */
    case 9:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_HAPPY_REACTION(fighters);
        break;
    }
    return 0.0f;
}


#undef PZ_DISPATCH_HAPPY_REACTION
#undef PZ_DISPATCH_BOMB_REACTION
#undef PZ_POP_PENDING_EVENT

/*
 * Common retail event setup. The macro intentionally open-codes this sequence:
 * every handler in the retail TU repeats it before transferring a fighter.
 */
#define PZ_PREPARE_FIGHTER_EVENT(move_ptr)                                \
    do {                                                                  \
        MkObj* event_fighter;                                             \
        PlyrPdata* event_pdata;                                           \
        float event_x;                                                    \
        float event_z;                                                    \
        float event_dx;                                                   \
        float event_dz;                                                   \
        float event_distance;                                             \
        if ((move_ptr)->player == 0) {                                    \
            event_fighter = (MkObj*)g_game_info.plyr0.slot.mirror_a;      \
        } else {                                                          \
            event_fighter = (MkObj*)g_game_info.plyr1.slot.mirror_a;      \
        }                                                                 \
        if ((int)(move_ptr)->player == 0) {                               \
            event_x = g_pz_fighters_engine.fighter_posts[0].x;            \
            event_z = g_pz_fighters_engine.fighter_posts[0].z;            \
        } else {                                                          \
            event_x = g_pz_fighters_engine.fighter_posts[1].x;            \
            event_z = g_pz_fighters_engine.fighter_posts[1].z;            \
        }                                                                 \
        event_dx = event_x - event_fighter->pos.x;                        \
        event_dz = event_z - event_fighter->pos.z;                        \
        event_distance = event_dx * event_dx + event_dz * event_dz;       \
        if ((move_ptr)->player == 0) {                                    \
            event_pdata = (PlyrPdata*)g_game_info.plyr0.slot.fighter;     \
        } else {                                                          \
            event_pdata = (PlyrPdata*)g_game_info.plyr1.slot.fighter;     \
        }                                                                 \
        if (event_distance < 2.45f) {                                     \
            (move_ptr)->distance_class = 1;                               \
        } else if (event_distance < 6.2f) {                              \
            (move_ptr)->distance_class = 2;                               \
        } else {                                                          \
            (move_ptr)->distance_class = 4;                               \
        }                                                                 \
        if ((move_ptr)->player == 0) {                                    \
            g_pz_fighters_engine.fighter_state[0] = 2;                   \
            g_pz_fighters_engine.fighter_state[1] = 3;                   \
        } else {                                                          \
            g_pz_fighters_engine.fighter_state[1] = 2;                   \
            g_pz_fighters_engine.fighter_state[0] = 3;                   \
        }                                                                 \
        event_pdata->state |= 0x1200;                                     \
        (move_ptr)->active_flags = 1;                                     \
    } while (0)

/*
 * Soft ceiling: complete four-way round-end animation policy; remaining
 * differences are saved-GPR selection, middle-branch lifetimes, and labels.
 */
static void pz_fighter_perform_end_of_round_anims(
    unsigned int winner_player, unsigned int loser_player) {
    unsigned int selection;

    selection = randu0(100);
    if (selection < 20) {
        pz_start_round_animation(
            winner_player, 6, pz_fighter_round_victory);
        pz_start_round_animation(
            loser_player, 7, pz_fighter_round_failure);
    } else if (selection < 65 &&
               xz_distance_between_players() < 1.2f) {
        PlyrPdata* pdata;
        PuzzleProcess* proc;
        PuzzleCmdScriptView* script;

        pdata = puzzle_player_pdata(winner_player);
        proc = g_game_info.plyr0.idle_proc;
        if (winner_player == 1) {
            proc = g_game_info.plyr1.idle_proc;
        }
        if (winner_player == 0) {
            g_pz_fighters_engine.fighter_state[0] = 6;
        } else {
            g_pz_fighters_engine.fighter_state[1] = 6;
        }
        pdata->state |= 0x4201;
        g_pz_fighters_engine.distance_flags = 1;
        g_pz_fighters_engine.fighter_move.player = winner_player;
        script = get_cmdscript_for_proc(proc);
        script->function_index = 0x2E;
        xfer_player_proc(proc, r_pz_call_script_function);
    } else if (selection < 85) {
        pz_start_round_animation(
            winner_player, 6, pz_fighter_round_whew);
        pz_start_round_animation(
            loser_player, 7, pz_fighter_round_ground_pound);
    } else {
        pz_start_round_animation(
            winner_player, 6, pz_fighter_big_time_happy);
        pz_start_round_animation(
            loser_player, 7, pz_fighter_WTF);
    }
}

/*
 * Soft ceiling: pz_fighter_first_block_has_been_placed ~86.08% - the
 * retail table-offset cursor reloads its base after selection; this typed
 * row walk leaves only cursor scheduling and nonvolatile allocation drift.
 */
static void pz_fighter_first_block_has_been_placed(unsigned int player) {
    FirstMoveMadeRow* row;
    PlyrPdata* fighter;
    PuzzleFighterEntry reaction;
    unsigned int selected_player;
    unsigned short roll;
    unsigned int index;

    roll = (unsigned short)randu0(100);
    g_pz_fighters_engine.fighter_move.player = player;
    g_pz_fighters_engine.fighter_move.mode = 15;

    for (index = 0; index < fistMoveMadeTable.rows[0].type; index++) {
        row = &fistMoveMadeTable.rows[index];
        if (roll < row->percent) {
            reaction = row->reaction;
            selected_player = g_pz_fighters_engine.fighter_move.player;
            fighter = puzzle_player_pdata(selected_player);
            if ((fighter->state & 0x200) != 0) {
                return;
            }

            if (selected_player == 0) {
                g_pz_fighters_engine.fighter_state[0] = 2;
                g_pz_fighters_engine.fighter_state[1] = 3;
            } else {
                g_pz_fighters_engine.fighter_state[1] = 2;
                g_pz_fighters_engine.fighter_state[0] = 3;
            }

            fighter->state |= 0x1200;
            g_pz_fighters_engine.fighter_move.active_flags = 1;
            g_pz_fighters_engine.random_fatality_active = 1;
            xfer_proc(
                pz_fighter_player_proc(
                    g_pz_fighters_engine.fighter_move.player),
                reaction);
            return;
        }
    }
}

/*
 * Retail open-codes this setup in every handler. Keeping the player loads at
 * each decision point reproduces its branch and register lifetimes.
 */
/*
 * Soft ceiling: complete dual-transfer policy; remaining differences are
 * GPR/FPR allocation, multi-register saves, and local float labels.
 */
static float pz_fighter_handle_dual_off_center_Move(PuzzleFighterMove* move) {
    MkObj* fighter;
    PlyrPdata* pdata;
    float target_x;
    float target_z;
    float dx;
    float dz;
    float distance;
    unsigned short roll;
    unsigned int other_player;

    if (move->player == 0) {
        fighter = (MkObj*)g_game_info.plyr0.slot.mirror_a;
    } else {
        fighter = (MkObj*)g_game_info.plyr1.slot.mirror_a;
    }
    if ((int)move->player == 0) {
        target_x = g_pz_fighters_engine.fighter_posts[0].x;
        target_z = g_pz_fighters_engine.fighter_posts[0].z;
    } else {
        target_x = g_pz_fighters_engine.fighter_posts[1].x;
        target_z = g_pz_fighters_engine.fighter_posts[1].z;
    }
    dz = target_z - fighter->pos.z;
    dx = target_x - fighter->pos.x;
    distance = dx * dx + dz * dz;
    if (move->player == 0) {
        pdata = (PlyrPdata*)g_game_info.plyr0.slot.fighter;
    } else {
        pdata = (PlyrPdata*)g_game_info.plyr1.slot.fighter;
    }
    roll = (unsigned short)randu0(100);
    other_player = 0;
    if (distance < 2.45f) {
        move->distance_class = 1;
    } else if (distance < 6.2f) {
        move->distance_class = 2;
    } else {
        move->distance_class = 4;
    }
    if (move->player == 0) {
        g_pz_fighters_engine.fighter_state[0] = 2;
        other_player = 1;
        g_pz_fighters_engine.fighter_state[1] = 3;
    } else {
        g_pz_fighters_engine.fighter_state[1] = 2;
        g_pz_fighters_engine.fighter_state[0] = 3;
    }
    pdata->state |= 0x1200;
    move->active_flags = 1;

    if (roll < 50) {
        xfer_proc(
            pz_fighter_player_proc(move->player),
            pz_fighter_light_propell);
        xfer_proc(
            pz_fighter_player_proc(other_player),
            pz_fighter_dummy_propell);
    } else if (roll < 75) {
        xfer_proc(
            pz_fighter_player_proc(move->player),
            pz_fighter_light_propell);
        xfer_proc(
            pz_fighter_player_proc(other_player),
            pz_fighter_smart_flippy);
    } else {
        xfer_proc(
            pz_fighter_player_proc(move->player),
            pz_fighter_smart_flippy);
        xfer_proc(
            pz_fighter_player_proc(other_player),
            pz_fighter_smart_flippy);
    }
    return 0.0f;
}

static float pz_fighter_handle_center_pos_minor_adjustment(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_center_pos_minor_adjustement);
    return 0.0f;
}

static float pz_fighter_handle_center_pos_single_close_move(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_center_pos_single_close_move);
    return 0.0f;
}

static float pz_fighter_handle_center_pos_single_range_move(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_center_pos_single_range_move);
    return 0.0f;
}

static float pz_fighter_handle_center_pos_range_attack(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_center_pos_range_attack);
    return 0.0f;
}

float pz_fighter_handle_distance_attack(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_dist_attack);
    return 0.0f;
}

static float pz_fighter_handle_off_wall_attack(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_off_wall_attack);
    return 0.0f;
}

static float pz_fighter_handle_winning_big_based_on_score_move(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    g_pz_fighters_engine.attack_has_followup = 0;
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_give_present);
    return 0.0f;
}

static float pz_fighter_handle_super_move_available(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_super_move_just_enabled);
    if (move->player == 0) {
        xfer_proc(
            g_game_info.plyr1.idle_proc,
            pz_fighter_perform_other_guy_super_move_just_enabled);
    } else {
        xfer_proc(
            g_game_info.plyr0.idle_proc,
            pz_fighter_perform_other_guy_super_move_just_enabled);
    }
    return 0.0f;
}

static float pz_fighter_handle_ohyeah_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_ohyeah_move);
    if (move->player == 0) {
        xfer_proc(
            g_game_info.plyr1.idle_proc,
            pz_fighter_perform_other_guy_ohyeah);
    } else {
        xfer_proc(
            g_game_info.plyr0.idle_proc,
            pz_fighter_perform_other_guy_ohyeah);
    }
    return 0.0f;
}

static float pz_fighter_handle_in_super_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_holding_onto_super_move);
    if (move->player == 0) {
        xfer_proc(
            g_game_info.plyr1.idle_proc,
            pz_fighter_perform_other_guy_holding_onto_super_move);
    } else {
        xfer_proc(
            g_game_info.plyr0.idle_proc,
            pz_fighter_perform_other_guy_holding_onto_super_move);
    }
    return 0.0f;
}

static float pz_fighter_handle_ohno_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_ohno_move);
    if (move->player == 0) {
        xfer_proc(
            g_game_info.plyr1.idle_proc,
            pz_fighter_perform_other_guy_ohno);
    } else {
        xfer_proc(
            g_game_info.plyr0.idle_proc,
            pz_fighter_perform_other_guy_ohno);
    }
    return 0.0f;
}

static float pz_fighter_handle_peak_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_peak_move);
    return 0.0f;
}

static float pz_fighter_handle_relief_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_relief_move);
    return 0.0f;
}

static float pz_fighter_handle_special_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_special_move);
    return 0.0f;
}

/*
 * Soft ceiling: complete scripted-move dispatch; remaining differences are
 * GPR/FPR allocation, multi-register saves, and local float labels.
 */
static float pz_fighter_handle_move(PuzzleFighterMove* move) {
    unsigned int script_move;
    MkObj* fighter;
    PlyrPdata* pdata;
    float target_x;
    float target_z;
    float dx;
    float dz;
    float distance;
    int opponent;
    int super_ready;

    script_move = move->script_move;
    if (move->player == 0) {
        fighter = (MkObj*)g_game_info.plyr0.slot.mirror_a;
    } else {
        fighter = (MkObj*)g_game_info.plyr1.slot.mirror_a;
    }
    if ((int)move->player == 0) {
        target_x = g_pz_fighters_engine.fighter_posts[0].x;
        target_z = g_pz_fighters_engine.fighter_posts[0].z;
    } else {
        target_x = g_pz_fighters_engine.fighter_posts[1].x;
        target_z = g_pz_fighters_engine.fighter_posts[1].z;
    }
    dz = target_z - fighter->pos.z;
    dx = target_x - fighter->pos.x;
    distance = dx * dx + dz * dz;
    if (move->player == 0) {
        pdata = (PlyrPdata*)g_game_info.plyr0.slot.fighter;
    } else {
        pdata = (PlyrPdata*)g_game_info.plyr1.slot.fighter;
    }
    if (distance < 2.45f) {
        move->distance_class = 1;
    } else if (distance < 6.2f) {
        move->distance_class = 2;
    } else {
        move->distance_class = 4;
    }
    if (script_move == 3) {
        opponent = 0;
        if (move->player == 0) {
            opponent = 1;
        }
        if (puzzle_fighter_get_super_bar_level(opponent) > 0.95f) {
            super_ready = 1;
        } else {
            super_ready = 0;
        }
        if (super_ready == 1) {
            script_move = 5;
        }
    }
    if (move->player == 0) {
        g_pz_fighters_engine.fighter_state[0] = 2;
        g_pz_fighters_engine.fighter_state[1] = 3;
    } else {
        g_pz_fighters_engine.fighter_state[1] = 2;
        g_pz_fighters_engine.fighter_state[0] = 3;
    }
    pdata->state |= 0x1200;
    move->active_flags = 1;
    if (script_move >= 14) {
        return 0.0f;
    }
    move->script_move = script_move;
    xfer_proc(
        pz_fighter_player_proc(move->player),
        pz_fighter_perform_scripted_move);
    return 0.0f;
}

#undef PZ_PREPARE_FIGHTER_EVENT


/*
 * Soft ceiling: pz_fighter_process_random_fatality_event ~99.82% -
 * zero-float pool identity only.
 */
float pz_fighter_process_random_fatality_event(
    PuzzleFatalityRandomEvent* event, PuzzleFatalityProcessFn reaction) {
    PlyrPdata* fighter = puzzle_player_pdata(event->side);

    if ((fighter->state & 0x200) != 0) {
        return 0.0f;
    }

    if (event->side == 0) {
        g_pz_fighters_engine.fighter_state[0] = 2;
        g_pz_fighters_engine.fighter_state[1] = 3;
    } else {
        g_pz_fighters_engine.fighter_state[1] = 2;
        g_pz_fighters_engine.fighter_state[0] = 3;
    }

    fighter->state |= 0x1200;
    event->started = 1;
    g_pz_fighters_engine.random_fatality_active = 1;
    xfer_proc(pz_fighter_player_proc(event->side), reaction);
    return 0.0f;
}

void pz_fighter_force_repel_during_attack(void) {
    g_pz_fighters_engine.force_repel = 1;
}

/*
 * Broad pass: shared Puzzle attack setup, distance policy, reaction window,
 * and animation completion pipeline.
 */
float pz_fighter_ani_attack(
    int reaction, unsigned int reaction_mode, float end_frame,
    float reaction_frame, float hit_distance);
void pz_fighter_startup_attack(
    AniScript* animation, int field0C, int field10, int field14,
    unsigned int reaction_mode, float frame1, float frame2, float frame3,
    float frame4, float desired_distance);

/*
 * Soft ceiling: complete descriptor dispatch; remaining differences are
 * GPR save selection and argument-load scheduling around the two callees.
 */
void pz_fighter_attack(
    AniScript* animation, PuzzleAttackParameters* attack, int reaction) {
    float frame4 = attack->field_18;

    if (attack->walk_to_range == 1) {
        if (pz_fighter_walk_until_fight_distance() != 1) {
            if (his_pdata->state == 0 &&
                his_pdata->fatality_shove_active == 0 &&
                his_pdata->previous_state != 0x4203) {
                xfer_proc(
                    pz_fighter_player_proc(his_pdata->plyr_num),
                    pz_fighter_move_into_fighting_position_now);
            }
            pz_fighter_walk_FB_true(
                pz_fighter_walk_until_fight_distance, 120, 1);
        }
        if (g_pz_fighters_engine.breakout == 1) {
            aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
        }
    }

    if (attack->has_followup == 0) {
        g_pz_fighters_engine.attack_has_followup = 0;
    } else {
        g_pz_fighters_engine.attack_has_followup = 1;
    }
    if (g_pz_fighters_engine.attack_policy_bits.distance_reaction) {
        g_pz_fighters_engine.attack_runtime_bits.enabled = 1;
    } else {
        g_pz_fighters_engine.attack_runtime_bits.enabled = 0;
    }
    if (attack->ignore_distance == 1) {
        frame4 = 0.0f;
    }

    pz_fighter_startup_attack(
        animation, attack->field_0C, attack->field_10, attack->field_14,
        attack->reaction_mode, attack->field_00, attack->field_04,
        attack->field_08, frame4, attack->desired_distance);
    pz_fighter_ani_attack(
        reaction, attack->reaction_mode, attack->end_frame,
        attack->reaction_frame, attack->hit_distance);
    g_pz_fighters_engine.force_repel = 0;
}

void pz_fighter_dont_fudge_desired_distance(void) {
    g_pz_fighters_engine.distance_flags |= PZ_FIGHTER_DISTANCE_FIXED;
}

/*
 * Soft ceiling: exact 468-byte frame/reaction loop; remaining differences are
 * saved-register allocation, one boolean-store schedule, and float labels.
 */
float pz_fighter_ani_attack(
    int reaction, unsigned int reaction_mode, float end_frame,
    float reaction_frame, float hit_distance) {
    int frame_clamped = 0;
    int reaction_started = 0;

    while (plyr_anim_pdata->frame <= end_frame && frame_clamped == 0) {
        advance_anim();
        pose_anim(plyr_anim_pdata, 1);
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();

        if (plyr_anim_pdata->frame >= end_frame) {
            frame_clamped = 1;
            plyr_anim_pdata->frame = end_frame;
        }

        if (reaction_mode != 2 &&
            (g_pz_fighters_engine.distance_flags & 1) != 0 &&
            reaction_started == 0 &&
            plyr_anim_pdata->frame >= reaction_frame &&
            xz_distance_between_players() < hit_distance) {
            pz_fighter_reaction_xfer_him(reaction);
            reaction_started = 1;
        }
    }

    if (reaction_started == 0 &&
        (g_pz_fighters_engine.distance_flags & 1) != 0) {
        if (his_pdata->plyr_num == 0) {
            g_pz_fighters_engine.fighter_state[0] = 0;
        } else {
            g_pz_fighters_engine.fighter_state[1] = 0;
        }
    } else if (reaction_started == 1) {
        if (his_pdata->plyr_num == 0) {
            g_pz_fighters_engine.fighter_state[0] = 4;
        } else {
            g_pz_fighters_engine.fighter_state[1] = 4;
        }
    }
    his_pdata->state &= ~0x800;
    return 0.0f;
}

/*
 * Soft ceiling: exact 772-byte startup pipeline; remaining differences are
 * fast-sqrt stack-slot/register scheduling and local float labels.
 */
void pz_fighter_startup_attack(
    AniScript* animation, int field0C, int field10, int field14,
    unsigned int reaction_mode, float frame1, float frame2, float frame3,
    float frame4, float desired_distance) {
    float distance;
    float correction;

    if ((g_pz_fighters_engine.distance_flags &
         PZ_FIGHTER_DISTANCE_FIXED) == 0) {
        distance = xz_distance_between_players();
        if (desired_distance < 0.65f &&
            g_pz_fighters_engine.force_repel == 0) {
            ((MkObj*)g_game_info.plyr0.slot.mirror_a)
                ->flags_09_bits.bit4 = 0;
            ((MkObj*)g_game_info.plyr1.slot.mirror_a)
                ->flags_09_bits.bit4 = 0;
        }

        if (distance > desired_distance - 0.2f &&
            distance < desired_distance + 0.2f) {
            pz_fighter_snap_to_distance(desired_distance, distance);
        } else if (distance > desired_distance - 1.0f &&
                   distance < desired_distance + 1.75f) {
            correction =
                (pz_fast_sqrt(distance) -
                 pz_fast_sqrt(desired_distance)) /
                5.0f;
            if (correction < 0.0f) {
                force_away(5, 3, -1.0f * correction, 0.9f);
            } else {
                force_forward(5, 3, correction, 0.9f);
            }
        }
    }

    if ((g_pz_fighters_engine.distance_flags & 2) != 0 &&
        reaction_mode != 2) {
        if (reaction_mode == 1 ||
            (reaction_mode == 4 &&
             (randu0(100) & 0xFFFF) < 25)) {
            pz_fighter_reaction_xfer_him(6);
        } else {
            pz_fighter_reaction_xfer_him(5);
        }
        if (his_pdata->plyr_num == 0) {
            g_pz_fighters_engine.fighter_state[0] = 5;
        } else {
            g_pz_fighters_engine.fighter_state[1] = 5;
        }
    }

    attack_to_frame_x(
        animation, field0C, field10, field14,
        frame1, frame2, frame3, frame4);
}

static float pz_fighter_move_into_desired_position(void) {
    MkObj* player1 = puzzle_fighter_object(0);
    float dx;
    float dz;
    float distance1;
    float distance2;
    float distance;

    dz = player1->pos.z - g_pz_fighters_engine.player1_idle_z;
    dx = player1->pos.x - g_pz_fighters_engine.player1_idle_x;
    distance1 = dx * dx + dz * dz;
    if (dx > 0.0f) {
        distance1 = -1.0f * distance1;
    }

    {
        MkObj* player2 = puzzle_fighter_object(1);
        dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
        dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
        distance2 = dx * dx + dz * dz;
        if (dx < 0.0f) {
            distance2 = -1.0f * distance2;
        }
    }

    distance = distance2;
    if (plyr_pdata->plyr_num == 0) {
        distance = distance1;
    }
    if (distance < -0.005f) {
        pz_fighter_walk_FB_true(
            pz_fighter_walk_until_danger_or_in_wrong_direction, 120, 0);
    } else if (distance > 0.005f) {
        pz_fighter_walk_FB_true(
            pz_fighter_walk_until_danger_or_in_wrong_direction, 120, 1);
    }
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

void pz_fighter_move_into_fighting_position(void) {
    if (pz_fighter_walk_until_fight_distance() != 1) {
        if (his_pdata->state == 0 &&
            his_pdata->fatality_shove_active == 0 &&
            his_pdata->previous_state != 0x4203) {
            xfer_proc(
                pz_fighter_player_proc(his_pdata->plyr_num),
                pz_fighter_move_into_fighting_position_now);
        }
        pz_fighter_walk_FB_true(
            pz_fighter_walk_until_fight_distance, 120, 1);
    }
}

/* Soft ceiling: pz_fighter_move_into_fighting_position_now 99.52% - pool. */
static float pz_fighter_move_into_fighting_position_now(void) {
    pz_fighter_walk_FB_true(
        pz_fighter_walk_until_fight_distance, 120, 1);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

void pz_fighters_calc_distance_to_desired_idle_pos_abs(
    float* player1_distance, float* player2_distance,
    float* player1_absolute, float* player2_absolute) {
    MkObj* player1 = puzzle_fighter_object(0);
    float dz;
    float dx;

    dz = player1->pos.z - g_pz_fighters_engine.player1_idle_z;
    dx = player1->pos.x - g_pz_fighters_engine.player1_idle_x;
    *player1_distance = dx * dx + dz * dz;
    *player1_absolute = *player1_distance;
    if (dx > 0.0f) {
        *player1_distance = -1.0f * *player1_distance;
    }

    {
        MkObj* player2 = puzzle_fighter_object(1);
        dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
        dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
        *player2_distance = dx * dx + dz * dz;
        *player2_absolute = *player2_distance;
        if (dx < 0.0f) {
            *player2_distance = -1.0f * *player2_distance;
        }
    }
}

void pz_fighters_calc_distance_to_desired_idle_pos(
    float* player1_distance, float* player2_distance) {
    MkObj* player1 = puzzle_fighter_object(0);
    float dz;
    float dx;

    dz = player1->pos.z - g_pz_fighters_engine.player1_idle_z;
    dx = player1->pos.x - g_pz_fighters_engine.player1_idle_x;
    *player1_distance = dx * dx + dz * dz;
    if (dx > 0.0f) {
        *player1_distance = -1.0f * *player1_distance;
    }

    {
        MkObj* player2 = puzzle_fighter_object(1);
        dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
        dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
        *player2_distance = dx * dx + dz * dz;
        if (dx < 0.0f) {
            *player2_distance = -1.0f * *player2_distance;
        }
    }
}

/*
 * Soft ceiling: complete spacing and wrapped-facing math; remaining
 * differences are fast-sqrt stack slots, FPR scheduling, and float labels.
 */
static void pz_fighter_snap_to_distance(
    float desired_distance_squared, float current_distance_squared) {
    Vec my_position;
    Vec my_angle;
    Vec his_position;
    Vec his_angle;
    Vec direction;
    float desired_distance;
    float current_distance;
    float distance_delta;
    float correction;
    float opposite_correction;
    float facing;
    float opposite_facing;
    float my_x_offset;
    float my_z_offset;
    float his_x_offset;
    float his_z_offset;

    current_distance = pz_fast_sqrt(current_distance_squared);
    desired_distance = pz_fast_sqrt(desired_distance_squared);
    distance_delta = current_distance - desired_distance;
    correction = distance_delta * 0.5f;

    my_position.x = plyr_obj->pos.x;
    my_position.y = plyr_obj->pos.y;
    my_position.z = plyr_obj->pos.z;
    my_angle.x = plyr_obj->ang.x;
    my_angle.y = plyr_obj->ang.y;
    my_angle.z = plyr_obj->ang.z;
    his_position.x = his_obj->pos.x;
    his_position.y = his_obj->pos.y;
    his_position.z = his_obj->pos.z;
    his_angle.x = his_obj->ang.x;
    his_angle.y = his_obj->ang.y;
    his_angle.z = his_obj->ang.z;

    xz_unit_vector(&direction, &my_position, &his_position);
    facing = (float)gxMathArcTanYX(direction.x, direction.z);
    opposite_correction = -1.0f * correction;
    my_x_offset = direction.x * correction;
    my_z_offset = direction.z * correction;
    my_position.x += my_x_offset;
    my_position.z += my_z_offset;
    his_x_offset = direction.x * opposite_correction;
    his_z_offset = direction.z * opposite_correction;
    his_position.x += his_x_offset;
    his_position.z += his_z_offset;

    facing = 0.000005992112f *
             (float)(((int)(166886.1f * facing)) & 0xFFFFF);
    my_angle.y = facing;
    opposite_facing = 3.1415927f + facing;
    his_angle.y = 0.000005992112f *
                  (float)(((int)(166886.1f * opposite_facing)) & 0xFFFFF);
    move_player(plyr_obj, &my_position, &my_angle);
    move_player(his_obj, &his_position, &his_angle);
    xz_distance_between_players();
}

/*
 * Exact retail algorithm and 276-byte size. The remaining mismatch is MWCC
 * FPR scheduling/contraction across the calculate-then-store scalar batch.
 */
static void pz_fighter_calculate_start_pos(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    float screen_scale = 1.0f;
    float post1_scale;
    float post2_scale;
    float center_x;
    float center_z;
    float player1_idle_x;
    float player1_idle_y;
    float player1_idle_z;
    float player2_idle_x;
    float player2_idle_y;
    float player2_idle_z;
    float player1_x_offset;
    float player1_z_offset;
    float player2_x_offset;
    float player2_z_offset;

    if (screen_width > 650) {
        screen_scale = 1.1f;
    }

    post1_scale = -1.6f * screen_scale;
    post2_scale = 1.6f * screen_scale;
    center_x = 0.0f;
    center_z = 0.0f;
    center_x += fighters->arena_axis_x * fighters->balance;
    center_z += fighters->arena_axis_z * fighters->balance;

    player1_x_offset = 0.5f * fighters->arena_axis_x;
    player1_z_offset = 0.5f * fighters->arena_axis_z;
    player2_x_offset = -0.5f * fighters->arena_axis_x;
    player2_z_offset = -0.5f * fighters->arena_axis_z;
    player1_idle_x = player1_x_offset + center_x;
    player1_idle_y = g_game_info.plyr0.slot.mirror_a->pos.y;
    player1_idle_z = player1_z_offset + center_z;
    player2_idle_x = player2_x_offset + center_x;
    player2_idle_y = g_game_info.plyr1.slot.mirror_a->pos.y;
    player2_idle_z = player2_z_offset + center_z;
    fighters->center_x = 0.0f;
    fighters->center_z = 0.0f;
    fighters->center_y = 0.0f;
    fighters->center_x = center_x;
    fighters->center_z = center_z;

    fighters->player1_idle_x = player1_idle_x;
    fighters->player1_idle_y = player1_idle_y;
    fighters->player1_idle_z = player1_idle_z;
    fighters->player2_idle_x = player2_idle_x;
    fighters->player2_idle_y = player2_idle_y;
    fighters->player2_idle_z = player2_idle_z;
    fighters->fighter_posts[0].x =
        fighters->arena_axis_x * post1_scale;
    fighters->fighter_posts[0].z =
        fighters->arena_axis_z * post1_scale;
    fighters->fighter_posts[1].x =
        fighters->arena_axis_x * post2_scale;
    fighters->fighter_posts[1].z =
        fighters->arena_axis_z * post2_scale;
}

float pz_fighter_fetch_distance_to_center_pos(void) {
    int player = plyr_pdata->plyr_num;
    MkObj* fighter = puzzle_fighter_object((unsigned int)player);
    float dx = g_pz_fighters_engine.center_x - fighter->pos.x;
    float dz = g_pz_fighters_engine.center_z - fighter->pos.z;
    float distance = dx * dx + dz * dz;

    if ((player == 0) &&
        (g_pz_fighters_engine.center_x < fighter->pos.x)) {
        distance *= -1.0f;
    }
    if ((player == 1) &&
        (g_pz_fighters_engine.center_x > fighter->pos.x)) {
        distance *= -1.0f;
    }
    return distance;
}

float pz_fighter_fetch_plyr_to_home_post_distance(int player) {
    MkObj* fighter;
    float home_x;
    float home_z;
    float dx;
    float dz;

    if ((unsigned int)player == 0) {
        fighter = puzzle_fighter_object(0);
    } else {
        fighter = puzzle_fighter_object(1);
    }

    if (player == 0) {
        home_x = g_pz_fighters_engine.fighter_posts[0].x;
        home_z = g_pz_fighters_engine.fighter_posts[0].z;
    } else {
        home_x = g_pz_fighters_engine.fighter_posts[1].x;
        home_z = g_pz_fighters_engine.fighter_posts[1].z;
    }

    dx = home_x - fighter->pos.x;
    dz = home_z - fighter->pos.z;
    return dx * dx + dz * dz;
}

float p_plyr_pz_fighter_start(void) {
    if (g_pz_fighters_engine.start_flag_bits.enabled == 0) {
        return 1.0f;
    }

    face_opponent_now();
    if (plyr_pdata->plyr_num == 1 &&
        plyr_obj->hide_flag_bits.bit6 == 1) {
        plyr_obj->hide_flag_bits.bit6 ^= 1;
    }
    glitch_to_stance(1.0f);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

/* Soft ceiling: exact 388-byte body; only two float-pool labels differ. */
float p_plyr_pz_fighter_entry(void) {
    int player;

    if (plyr_pdata->fatality_shove_active == 1) {
        plyr_pdata->fatality_shove_active = 0;
        aproc->vtbl->transfer(pz_fighter_dizzy, 0.0f);
        return 0.0f;
    }

    blend_to_stance(0.1f);
    if ((plyr_pdata->state & 0x400) != 0) {
        _mkproc_sleep_ticks = 10.0f;
        aproc->vtbl->sleep();
    }

    player = plyr_pdata->plyr_num;
    if (player == 0) {
        if (plyr_pdata->state != 0x2000 ||
            g_pz_fighters_engine.fighter_state[0] != 3) {
            g_pz_fighters_engine.fighter_state[0] = 0;
        }
    } else if (plyr_pdata->state != 0x2000 ||
               g_pz_fighters_engine.fighter_state[1] != 3) {
        g_pz_fighters_engine.fighter_state[1] = 0;
    }

    init_ground_move();
    plyr_obj->flags_09_bits.bit4 = 1;
    back_to_normal();
    player = 0;
    if ((int)plyr_obj->oid == 0x1002) {
        player = 1;
    }
    plyr_obj->flags_09_bits.launched = 1;
    g_pz_fighters_engine.y_constraint_enabled[player] = 0;
    aproc->vtbl->transfer(p_plyr_pz_fighter_loop, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 99.00%; only shared 0.8f/1.0f pool relocations differ. */
static float p_plyr_pz_fighter_loop(void) {
    rotate_towards_him(0.8f);
    return 1.0f;
}

int pz_fighter_is_losing_big(unsigned int player) {
    if (player == 0) {
        if (g_pz_fighters_engine.balance > 0.5f) {
            return 1;
        }
    } else if (g_pz_fighters_engine.balance < -0.5f) {
        return 1;
    }
    return 0;
}

int pz_fighter_is_winning_big(unsigned int player) {
    if (player == 1) {
        if (g_pz_fighters_engine.balance > 0.5f) {
            return 1;
        }
    } else if (g_pz_fighters_engine.balance < -0.5f) {
        return 1;
    }
    return 0;
}


float pz_fighter_check_breakout(void) {
    if (g_pz_fighters_engine.breakout == 1) {
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
        return 0.0f;
    }
    return 0.0f;
}

int pz_fighter_should_he_breakout(void) {
    return g_pz_fighters_engine.breakout;
}

int pz_fighter_close_enough_to_super_move(int player) {
    return puzzle_fighter_get_super_bar_level(player) > -0.75f;
}

MkObj* pz_fighter_get_player_obj(unsigned int player) {
    if (player == 0) {
        return g_game_info.plyr0.slot.mirror_a;
    }
    return g_game_info.plyr1.slot.mirror_a;
}

PuzzleProcess* pz_fighter_get_player_proc(unsigned int player) {
    return pz_fighter_player_proc(player);
}

PlyrPdata* pz_get_pdata_by_id(int player) {
    return puzzle_player_pdata(player);
}

PuzzleFighterMove* pz_get_fighter_move(void) {
    return &g_pz_fighters_engine.fighter_move;
}



#define PZ_CANCEL_ACTIVE_MOVE()                                      \
    do {                                                             \
        g_pz_fighters_engine.random_fatality_active = 0;             \
        g_pz_fighters_engine.attack_policy_word = 0;                 \
        g_pz_fighters_engine.fighter_state[0] = 0;                   \
        g_pz_fighters_engine.fighter_state[1] = 0;                   \
    } while (0)


#undef PZ_CANCEL_ACTIVE_MOVE
