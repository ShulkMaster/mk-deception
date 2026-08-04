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
    int mode; /* +0x10 */
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
                    unsigned char attack_policy_flags; /* +0x8C */
                    unsigned char attack_runtime_flags; /* +0x8D */
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
    char pad108[4];
    int positioning_active; /* +0x10C */
    int fighter_state[2]; /* +0x110 */
    int force_repel; /* +0x118 */
    int fatality_abort; /* +0x11C */
    int fatality_active; /* +0x120 */
    int fatality_ready; /* +0x124 */
    int fatality_victim; /* +0x128 */
    int fatality_attacker; /* +0x12C */
    char pad130[0x2C];
    int fatality_index; /* +0x15C */
    char pad160[4];
    int breakout; /* +0x164 */
    int y_constraint_enabled[2]; /* +0x168 */
    float y_constraint[2]; /* +0x170 */
    int fatality_timer; /* +0x178 */
    char pad17C[4];
    ScreenObj* screen_objects[2]; /* +0x180 */
    AniTextureControl* texture_controls[2]; /* +0x188 */
    int balance_update_timer; /* +0x190 */
    float pending_balance; /* +0x194 */
    unsigned char start_flags; /* +0x198 - bit7 enables fighter startup */
    char pad199[3];
    unsigned int immediate_request_player; /* +0x19C */
    unsigned int immediate_request_type; /* +0x1A0 */
    int immediate_request_timer; /* +0x1A4 */
    int immediate_request_active; /* +0x1A8 */
    unsigned int event_block_count; /* +0x1AC */
    char pad1B0[4];
    int field_1B4;
    int balance_out_of_range; /* +0x1B8 */
    char pad1BC[0x1C];
    int field_1D8;
    int super_move_request_pending; /* +0x1DC */
    int constraint_timer; /* +0x1E0 */
} PuzzleFightersEngine;

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

extern PuzzleFightersEngine g_pz_fighters_engine;
extern PuzzleProcess* aproc;
extern MkHdr* apdata;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern PlyrPdata* his_pdata;
extern PuzzleAnimPdata* plyr_anim_pdata;
extern float _mkproc_sleep_ticks;
extern int screen_width;
extern PuzzleProcess* _create_mkproc_generic_tinystack(
    int proc_id, int priority, PuzzleFighterEntry entry, int pdata_size,
    MkHdr** out_pdata);

double sqrt(double value);
int pz_fighter_fatality_during_round_stuff_over(void);
float pz_fighters_fatality_prep_chores(void);
float pz_fighters_fatality_in_progress(void);
void check_fighter_constraints(void);
void pz_fighter_process_immediate_request(void);
int pz_fighter_check_for_player_to_center_position_control(void);
void pz_fighter_individual_plyr_do_something(int player, int state);
void pz_fighters_inside_super_move_scenerio(void);
float pz_fighters_handle_next_pending_move(void);
void pz_fighters_idle_process(void);
float pz_fighters_handle_next_pending_move_simplified(void);
void pz_fighter_calculate_start_pos(void);
float pz_fighter_exit(void);
float p_plyr_pz_fighter_entry(void);
float pz_fighter_dizzy(void);
static float p_plyr_pz_fighter_loop(void);
float pz_fighter_move_into_desired_position(void);
float pz_fighter_laugh_small(void);
float pz_fighter_random_taunt(void);
float pz_fighter_round_victory(void);
float pz_fighter_round_failure(void);
float pz_fighter_round_whew(void);
float pz_fighter_round_ground_pound(void);
float pz_fighter_WTF(void);
float r_pz_call_script_function(void);
void xfer_player_proc(PuzzleProcess* proc, PuzzleFighterEntry entry);
PuzzleCmdScriptView* get_cmdscript_for_proc(PuzzleProcess* proc);
float pz_fighter_move_into_fighting_position_now(void);
int pz_fighter_walk_until_fight_distance(void);
int pz_fighter_walk_until_danger_or_in_wrong_direction(void);
int pz_fighter_walk_FB_true(
    int (*test)(void), int distance, int forward);
float xz_distance_between_players(void);
void pz_fighter_snap_to_distance(
    float desired_distance_squared, float current_distance_squared);
void pz_fighter_reaction_xfer_him(int reaction);
PuzzleProcess* pz_fighter_get_player_proc(unsigned int player);
void advance_anim(void);
void pose_anim(PuzzleAnimPdata* animation, int update_object);
void attack_to_frame_x(
    float frame1, float frame2, float frame3, float frame4,
    AniScript* animation, int field0C, int field10, int field14);
void force_forward(float force, int duration, float damping, int animation);
void force_away(float force, int duration, float damping, int animation);
void move_player(MkObj* fighter, const Vec* position, const Vec* angle);
unsigned int randu0(unsigned int max);
void face_opponent_now(void);
void blend_to_stance(float blend);
void init_ground_move(void);
void back_to_normal(MkObj* fighter, int enabled);
void rotate_towards_him(float rate);
void glitch_to_stance(float blend);
void xfer_proc(PuzzleProcess* proc, PuzzleFighterEntry entry);
float pz_fighter_handle_dual_off_center_Move(PuzzleFighterMove* move);
float pz_fighter_handle_center_pos_minor_adjustment(PuzzleFighterMove* move);
float pz_fighter_handle_center_pos_single_close_move(PuzzleFighterMove* move);
float pz_fighter_handle_center_pos_single_range_move(PuzzleFighterMove* move);
float pz_fighter_handle_center_pos_range_attack(PuzzleFighterMove* move);
float pz_fighter_handle_distance_attack(PuzzleFighterMove* move);
float pz_fighter_handle_off_wall_attack(PuzzleFighterMove* move);
float pz_fighter_handle_winning_big_based_on_score_move(
    PuzzleFighterMove* move);
float pz_fighter_handle_super_move_available(PuzzleFighterMove* move);
float pz_fighter_handle_ohyeah_move(PuzzleFighterMove* move);
float pz_fighter_handle_in_super_move(PuzzleFighterMove* move);
float pz_fighter_handle_ohno_move(PuzzleFighterMove* move);
float pz_fighter_handle_peak_move(PuzzleFighterMove* move);
float pz_fighter_handle_relief_move(PuzzleFighterMove* move);
float pz_fighter_handle_special_move(PuzzleFighterMove* move);
float pz_fighter_handle_move(PuzzleFighterMove* move);
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
float pz_fighters_react_to_bomb_explosion(void);
float pz_fighter_big_time_happy(void);
float pz_fighter_whatever2(void);
void pz_fighter_shake_camera(float strength, int duration);
void pz_fighter_perform_end_of_round_anims(
    unsigned int player, int other_player);
void p_objects_moving(double conversion_bias);
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
void pz_fighter_buffer_new_move(
    unsigned int event_type, unsigned int player, unsigned int move,
    unsigned int priority);
void pz_fighter_fight_request(
    unsigned int player, unsigned int block_count, int chain_count,
    unsigned int event_type);
void pz_fighter_first_block_has_been_placed(unsigned int player);
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

static PlyrPdata* puzzle_player_pdata(unsigned int player) {
    if (player == 0) {
        return (PlyrPdata*)g_game_info.plyr0.slot.fighter;
    }
    return (PlyrPdata*)g_game_info.plyr1.slot.fighter;
}

static MkObj* puzzle_fighter_object(int player) {
    if (player == 0) {
        return (MkObj*)g_game_info.plyr0.slot.mirror_a;
    }
    return (MkObj*)g_game_info.plyr1.slot.mirror_a;
}

void p_objects_moving(double conversion_bias) {
    PuzzleObjectMotion* motion;
    ScreenObj* object;

    motion = (PuzzleObjectMotion*)apdata;
    if (motion->complete == 1) {
        return;
    }

    motion->lifetime--;
    if (motion->lifetime <= 0) {
        pull_screen_obj(motion->object);
        pull_ani_texture_control(motion->texture_control);
        return;
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
}

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
    ScreenObj* object;
    AniTextureControl* texture_control;
    PuzzleProcess* proc;

    if (player >= 2) {
        return;
    }

    object = g_pz_fighters_engine.screen_objects[player];
    texture_control = g_pz_fighters_engine.texture_controls[player];
    insert_screen_obj(object);
    set_ani_texture_frame(texture_control, frame);
    insert_ani_texture_control(texture_control);
    set_ani_texture_framerate(texture_control, frame_rate);

    proc = _create_mkproc_generic_tinystack(
        0xC001, 0x1F, (PuzzleFighterEntry)p_objects_moving,
        sizeof(PuzzleObjectMotion), (MkHdr**)&motion);
    if (proc == 0 || motion == 0) {
        return;
    }

    motion->start = *start;
    motion->target = *target;
    motion->arrival = arrival;
    motion->object = object;
    motion->texture_control = texture_control;
    if (velocity == 0) {
        motion->velocity_x =
            (target->x - start->x) / (float)target_ticks;
        motion->velocity_y =
            (target->y - start->y) / (float)target_ticks;
    } else {
        motion->velocity_x = velocity->x;
        motion->velocity_y = velocity->y;
    }

    object->flags =
        (object->flags & ~0x20) | ((mirror << 5) & 0x20);
    if (mirror == 1 && object->scale_x > 0.0f) {
        object->scale_x *= -1.0f;
        object->pfx2d->mirror = 1;
    } else if (mirror == 0 && object->scale_x < 0.0f) {
        object->scale_x *= -1.0f;
        object->pfx2d->mirror = 1;
    }

    motion->gravity_step = gravity_step;
    motion->gravity_accumulator = 0.0f;
    object->x = (int)start->x;
    object->y = (int)start->y;
    motion->minimum_velocity_y = minimum_velocity_y;
    motion->bounce_enabled = bounce;
    motion->rise_ticks = -1;
    motion->fall_ticks = 0;
    motion->lifetime = 1200;
    motion->target_ticks = target_ticks;
    motion->complete = 0;
    motion->arrived = 0;
}

void pz_fighter_individual_plyr_do_something(
    int player, int other_state) {
    MkObj* fighter;
    PuzzleProcess* proc;
    float dx;
    float dz;
    float signed_distance;

    fighter = puzzle_fighter_object(player);
    proc = pz_fighter_get_player_proc(player);
    g_pz_fighters_engine.fighters_positioned = 0;
    if (player == 0) {
        dx = fighter->pos.x - g_pz_fighters_engine.player1_idle_x;
        dz = fighter->pos.z - g_pz_fighters_engine.player1_idle_z;
        signed_distance = dx * dx + dz * dz;
        if (dx > 0.0f) {
            signed_distance = -signed_distance;
        }
    } else {
        dx = fighter->pos.x - g_pz_fighters_engine.player2_idle_x;
        dz = fighter->pos.z - g_pz_fighters_engine.player2_idle_z;
        signed_distance = dx * dx + dz * dz;
        if (dx < 0.0f) {
            signed_distance = -signed_distance;
        }
    }

    if (xz_distance_between_players() > 1.45f &&
        signed_distance > -1.0f && signed_distance < 1.0f) {
        if (g_pz_fighters_engine.constraint_timer <= 0 &&
            other_state == 4 && randu0(100) < 50) {
            g_pz_fighters_engine.fighter_state[player] = 2;
            xfer_proc(proc, pz_fighter_laugh_small);
            g_pz_fighters_engine.constraint_timer = 360;
            return;
        }
        if (g_pz_fighters_engine.constraint_timer <= 60) {
            g_pz_fighters_engine.fighter_state[player] = 2;
            xfer_proc(proc, pz_fighter_random_taunt);
            g_pz_fighters_engine.constraint_timer = 300;
            return;
        }
    }

    if ((other_state == 4 && signed_distance < -0.1f) ||
        (other_state != 4 &&
         (signed_distance < -0.1f || signed_distance > 0.1f))) {
        g_pz_fighters_engine.fighter_state[player] = 1;
        xfer_proc(proc, pz_fighter_move_into_desired_position);
    }
}

static void pz_start_round_animation(
    unsigned int player, int fighter_state, PuzzleFighterEntry entry) {
    PlyrPdata* pdata;

    pdata = puzzle_player_pdata(player);
    if ((pdata->state & 0x200) == 0) {
        g_pz_fighters_engine.fighter_state[player] = fighter_state;
        pdata->state |= 0x4201;
        xfer_proc(pz_fighter_get_player_proc(player), entry);
    }
}

void pz_fighter_perform_end_of_round_anims(
    unsigned int winner_player, int loser_player) {
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
        proc = pz_fighter_get_player_proc(winner_player);
        g_pz_fighters_engine.fighter_state[winner_player] = 6;
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

void pz_fighter_event(PuzzleFighterEvent* event) {
    PuzzleFightersEngine* engine;
    unsigned int block_count;
    unsigned int chain_count;

    engine = &g_pz_fighters_engine;
    switch (event->type) {
    case 0:
        engine->round_running = 1;
        engine->super_move_active = 0;
        engine->balance_update_timer = 180;
        pz_fighters_fatality_normal_fighting(1);
        engine->start_flags &= (unsigned char)~0x60;
        engine->field_1B4 = 0;
        engine->field_1D8 = 0;
        engine->constraint_timer = 0;
        break;
    case 1:
        engine->balance = 0.0f;
        engine->immediate_request_active = 0;
        break;
    case 2:
        engine->round_running = 0;
        pz_fighters_fatality_normal_fighting(0);
        break;
    case 3:
        pz_fighters_fatality_unload();
        break;
    case 4:
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
    case 6:
        block_count = (unsigned int)event->block_count;
        chain_count = (unsigned int)event->chain_count;
        pz_fighter_fight_request(
            event->player, block_count, chain_count, 1);
        engine->start_flags |= 0x60;
        break;
    case 8:
        pz_fighter_fight_request(event->player, 0, 0, 6);
        break;
    case 9:
        pz_fighter_fight_request(event->player, 0, 0, 5);
        break;
    case 10:
        engine->round_running = 0;
        engine->fatality_abort = 1;
        engine->fatality_active = 0;
        engine->fatality_ready = 0;
        engine->fatality_victim = event->player;
        pz_fighters_fatality_start(
            event->player, engine->fatality_attacker);
        break;
    case 11:
        engine->round_running = 1;
        engine->pending_move_count = 0;
        engine->super_move_active = 1;
        engine->super_move_request_pending =
            (randu0(100) & 0xFFFF) < 75;
        pz_fighter_kill_present();
        pz_fighter_kill_global_projectile();
        break;
    case 14:
        engine->pending_move_count = 0;
        engine->super_move_active = 1;
        engine->super_move_request_pending =
            (randu0(100) & 0xFFFF) < 75;
        pz_fighter_kill_present();
        pz_fighter_kill_global_projectile();
        break;
    case 15:
        engine->start_flags &= (unsigned char)~0x80;
        pz_fighters_fatality_unload();
        break;
    case 18:
        pz_fighters_fatality_preround_event();
        break;
    case 19:
        pz_fighter_load_place_fatality_elements(
            (unsigned int)event->block_count);
        break;
    case 21:
        if (engine->immediate_request_active != 1) {
            engine->immediate_request_active = 1;
            engine->immediate_request_player = event->player;
            engine->immediate_request_type = 8;
            engine->immediate_request_timer = 1;
            engine->breakout = 1;
        }
        break;
    case 22:
        if (engine->immediate_request_active != 1) {
            engine->immediate_request_active = 1;
            engine->immediate_request_player = event->player;
            engine->immediate_request_type = 9;
            engine->immediate_request_timer = 1;
            engine->breakout = 1;
        }
        break;
    case 23:
        pz_fighter_fight_request(event->player, 0, 0, 7);
        break;
    case 24:
        if ((engine->start_flags & 0x60) == 0) {
            pz_fighter_first_block_has_been_placed(event->player);
        } else {
            pz_fighter_fight_request(event->player, 0, 0, 0);
        }
        engine->start_flags |= 0x40;
        break;
    default:
        break;
    }
}

/*
 * Queue a board event in descending priority order. Retail keeps at most two
 * pending moves; a newly inserted third entry drops the lowest-priority tail.
 */
void pz_fighter_buffer_new_move(
    unsigned int event_type, unsigned int player, unsigned int move,
    unsigned int priority) {
    PuzzleFighterMove pending;
    unsigned int insert;
    unsigned int index;

    pending.event_type = event_type;
    pending.player = player;
    pending.script_move = move;
    pending.policy_word = priority;

    if (g_pz_fighters_engine.pending_move_count == 0) {
        memcpy(
            &g_pz_fighters_engine.pending_moves[0], &pending,
            sizeof(PuzzleFighterMove));
        g_pz_fighters_engine.pending_move_count++;
        return;
    }

    insert = g_pz_fighters_engine.pending_move_count;
    for (index = 0; index < g_pz_fighters_engine.pending_move_count; index++) {
        if (g_pz_fighters_engine.pending_moves[index].policy_word < priority) {
            insert = index;
            break;
        }
    }

    if (insert < g_pz_fighters_engine.pending_move_count) {
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

void check_fighter_constraints(void) {
    unsigned int player;

    for (player = 0; player < 2; player++) {
        MkObj* fighter = puzzle_fighter_object(player);
        float distance =
            g_pz_fighters_engine.constraint_axis_x *
                (g_pz_fighters_engine.fighter_posts[0].x - fighter->pos.x) +
            g_pz_fighters_engine.constraint_axis_z *
                (g_pz_fighters_engine.fighter_posts[0].z - fighter->pos.z);

        if (distance > 0.01f) {
            distance /= 5.0f;
            fighter->pos.x +=
                g_pz_fighters_engine.constraint_axis_x * distance;
            fighter->pos.z +=
                g_pz_fighters_engine.constraint_axis_z * distance;
        } else if (distance < 0.002f) {
            fighter->pos.x +=
                g_pz_fighters_engine.constraint_axis_x * distance;
            fighter->pos.z +=
                g_pz_fighters_engine.constraint_axis_z * distance;
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

        puzzle_fighter_object(0)->pos.x = puzzle_fighter_object(1)->pos.x;
        puzzle_fighter_object(0)->pos.y = puzzle_fighter_object(1)->pos.y;
        puzzle_fighter_object(0)->pos.z = puzzle_fighter_object(1)->pos.z;
        puzzle_fighter_object(1)->pos.x = first_x;
        puzzle_fighter_object(1)->pos.y = puzzle_fighter_object(0)->pos.y;
        puzzle_fighter_object(1)->pos.z = puzzle_fighter_object(0)->pos.z;
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
    if ((event_type >= 5 && event_type <= 8) || event_type == 9) {
        *move = 0;
        *priority = 15;
        return;
    }

    if (chain_count == 0) {
        *move = 1;
        if (block_count == 0) {
            *move = 0;
            *priority = 1;
            return;
        }
        if (block_count < 12) {
            roll = (unsigned short)randu0(100);
            if (block_count < 5) {
                *priority = 2;
            } else if (block_count < 9) {
                *priority = 3;
            } else {
                *priority = 4;
            }
            if (roll < 30) {
                *move = 2;
            } else if (roll < 60 && *priority < 3) {
                *move = 3;
            } else if (roll < 60 && *priority >= 3) {
                *move = 4;
            }
            return;
        }
        if (block_count < 13) {
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
        if (block_count < 21) {
            *move = 8;
            if ((unsigned short)randu0(100) < 25) {
                (*move)++;
            }
            *priority = 8;
            return;
        }
        if (block_count < 31) {
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
    }

    if (chain_count == 1) {
        *move = 5;
        if (block_count < 6) {
            *priority = 5;
        } else if (block_count < 8) {
            *priority = 6;
        } else if (block_count < 10) {
            *priority = 7;
            (*move)++;
        } else {
            *move += 2;
            *priority = 10;
        }
        return;
    }
    if (chain_count == 2) {
        *move = 7;
        if (block_count < 9) {
            *priority = 8;
        } else if (block_count < 11) {
            *priority = 8;
            (*move)++;
        } else {
            *priority = 11;
            *move += 2;
        }
        return;
    }
    if (chain_count == 3) {
        *move = 9;
        if (block_count < 12) {
            *priority = 9;
        } else {
            *priority = 10;
            (*move)++;
        }
        return;
    }
    if (chain_count == 4) {
        *move = 10;
        *priority = 12;
        return;
    }
    if (chain_count == 5) {
        *move = 11;
        *priority = 13;
        return;
    }
    *move = 12;
    *priority = 15;
}

/*
 * Broad pass: central Puzzle fighter scheduler for fatality timing, delayed
 * requests, balance changes, paired fighter states, and idle/super routing.
 * Soft ceiling: p_puzzle_fighter_master 62.18% -- first algorithm pass.
 */
float p_puzzle_fighter_master(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PlyrPdata* player1 = puzzle_player_pdata(0);
    PlyrPdata* player2 = puzzle_player_pdata(1);
    int state1;
    int state2;
    int pending_active;
    int i;

    if (fighters->round_running == 0) {
        if (fighters->fatality_abort == 1) {
            if (fighters->fatality_active == 0) {
                pz_fighters_fatality_prep_chores();
            } else {
                fighters->fatality_timer--;
                if (fighters->fatality_timer == 0) {
                    fighters->fatality_abort = 0;
                    return -1.0f;
                }
                pz_fighters_fatality_in_progress();
            }
        }

        if (player2->state == 0 &&
            player2->fatality_shove_active == 1) {
            xfer_proc(
                pz_fighter_get_player_proc(1),
                p_plyr_pz_fighter_entry);
        }
        if (player1->state == 0 &&
            player1->fatality_shove_active == 1) {
            xfer_proc(
                pz_fighter_get_player_proc(0),
                p_plyr_pz_fighter_entry);
        }
        return 1.0f;
    }

    fighters->fatality_timer = 0;
    fighters->constraint_timer--;
    check_fighter_constraints();

    if (fighters->immediate_request_active == 1) {
        if (player1->state == 0 && player2->state == 0) {
            pz_fighter_process_immediate_request();
        } else {
            fighters->immediate_request_timer--;
            if (fighters->immediate_request_timer == 0) {
                pz_fighter_process_immediate_request();
            }
        }
        return 1.0f;
    }

    fighters->balance_update_timer--;
    if (fighters->balance_update_timer == 0) {
        int balance_step =
            (int)(fighters->balance - fighters->pending_balance);

        fighters->balance_update_timer = 360;
        fighters->balance_out_of_range =
            (float)balance_step > 0.1f ||
            (float)balance_step < -0.1f;
        fighters->balance = fighters->pending_balance;
        pz_fighter_calculate_start_pos();
    }

    if (pz_fighter_check_for_player_to_center_position_control() == 1) {
        return 1.0f;
    }

    state1 = fighters->fighter_state[0];
    state2 = fighters->fighter_state[1];
    if (state1 != 0 || state2 != 0) {
        if (player1->state == 0 && player2->state == 0) {
            fighters->fighter_state[0] = 0;
            fighters->fighter_state[1] = 0;
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
        for (i = 0; i < (int)fighters->pending_move_count; i++) {
            if (fighters->pending_moves[i].event_type == 1) {
                pending_active = 1;
                break;
            }
        }
        if (pending_active == 1 &&
            (state1 == 0 || state1 == 1 || state1 == 5) &&
            (state2 == 0 || state2 == 1 || state2 == 5)) {
            xfer_proc(
                pz_fighter_get_player_proc(0),
                p_plyr_pz_fighter_entry);
            xfer_proc(
                pz_fighter_get_player_proc(1),
                p_plyr_pz_fighter_entry);
            fighters->fighter_state[0] = 9;
            fighters->fighter_state[1] = 9;
        } else if (state1 == 0 && state2 == 1) {
            pz_fighter_individual_plyr_do_something(0, state2);
        } else if (state1 == 1 && state2 == 0) {
            pz_fighter_individual_plyr_do_something(1, state1);
        }
        return 1.0f;
    }

    fighters->random_fatality_active = 0;
    fighters->breakout = 0;
    fighters->balance_out_of_range = 0;
    fighters->attack_policy_word = 0;

    if ((puzzle_fighter_get_super_bar_level(0) > 0.95f ||
         puzzle_fighter_get_super_bar_level(1) > 0.95f) &&
        fighters->super_move_active == 0) {
        pz_fighters_inside_super_move_scenerio();
    } else if (fighters->pending_move_count != 0) {
        pz_fighters_handle_next_pending_move();
    } else {
        pz_fighters_idle_process();
    }
    return 1.0f;
}

/*
 * Broad pass: route off-center fighters into the appropriate paired,
 * single-fighter, wall, or spacing correction event.
 */
int pz_fighter_check_for_player_to_center_position_control(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    MkObj* player1 = puzzle_fighter_object(0);
    MkObj* player2 = puzzle_fighter_object(1);
    PlyrPdata* pdata1 = puzzle_player_pdata(0);
    PlyrPdata* pdata2 = puzzle_player_pdata(1);
    float player1_distance;
    float player2_distance;
    float player1_absolute;
    float player2_absolute;
    float player_distance;
    float player1_wall;
    float player2_wall;
    float dx;
    float dz;
    int player1_busy;
    int player2_busy;

    if (fighters->fighter_state[0] == 8 ||
        fighters->fighter_state[1] == 8 ||
        fighters->fighter_state[0] == 2 ||
        fighters->fighter_state[1] == 2 ||
        puzzle_fighter_get_super_bar_level(0) > 0.98f ||
        puzzle_fighter_get_super_bar_level(1) > 0.98f ||
        fighters->super_move_active == 1 ||
        pdata1->state == 0x605 || pdata2->state == 0x605) {
        return 0;
    }

    player1_busy =
        (fighters->fighter_state[0] == 0 ||
         fighters->fighter_state[0] == 1) ? 0 : 10;
    player2_busy =
        (fighters->fighter_state[1] == 0 ||
         fighters->fighter_state[1] == 1) ? 0 : 10;
    if (player1_busy != 0 && player2_busy != 0) {
        return 0;
    }

    pz_fighters_calc_distance_to_desired_idle_pos_abs(
        &player1_distance, &player2_distance,
        &player1_absolute, &player2_absolute);

    if (player1_absolute > 0.8f && player2_absolute > 0.8f) {
        player_distance = xz_distance_between_players();
        if (player_distance >= 2.0f) {
            if (player1_busy == 0 && player1_distance > 0.0f) {
                fighters->fighter_move.player = 0;
                pz_fighter_handle_center_pos_range_attack(
                    &fighters->fighter_move);
                return 1;
            }
            if (player2_busy == 0 && player2_distance > 0.0f) {
                fighters->fighter_move.player = 1;
                pz_fighter_handle_center_pos_range_attack(
                    &fighters->fighter_move);
                return 1;
            }
            return 0;
        }

        dx = fighters->fighter_posts[0].x - player1->pos.x;
        dz = fighters->fighter_posts[0].z - player1->pos.z;
        player1_wall = dx * dx + dz * dz;
        dx = fighters->fighter_posts[1].x - player2->pos.x;
        dz = fighters->fighter_posts[1].z - player2->pos.z;
        player2_wall = dx * dx + dz * dz;

        if (player1_wall > 6.8f && player1_busy == 0 &&
            player1_distance > 0.0f) {
            fighters->fighter_move.player = 0;
            pz_fighter_handle_off_wall_attack(&fighters->fighter_move);
            return 1;
        }
        if (player2_wall > 6.8f && player2_busy == 0 &&
            player2_distance > 0.0f) {
            fighters->fighter_move.player = 1;
            pz_fighter_handle_off_wall_attack(&fighters->fighter_move);
            return 1;
        }
        if (player1_wall <= 6.8f && player1_busy == 0 &&
            player1_distance > 0.0f) {
            fighters->fighter_move.player = 0;
            pz_fighter_handle_distance_attack(&fighters->fighter_move);
            return 1;
        }
        if (player2_wall <= 6.8f && player2_busy == 0 &&
            player2_distance > 0.0f) {
            fighters->fighter_move.player = 1;
            pz_fighter_handle_distance_attack(&fighters->fighter_move);
            return 1;
        }
        return 0;
    }

    if (player1_absolute > 1.45f) {
        fighters->fighter_move.player = 0;
        if (player1_busy != 0) {
            return 0;
        }
        if (xz_distance_between_players() > 1.7f) {
            pz_fighter_handle_center_pos_single_range_move(
                &fighters->fighter_move);
        } else {
            pz_fighter_handle_center_pos_single_close_move(
                &fighters->fighter_move);
        }
        return 1;
    }
    if (player2_absolute > 1.45f) {
        fighters->fighter_move.player = 1;
        if (player2_busy != 0) {
            return 0;
        }
        if (xz_distance_between_players() > 1.7f) {
            pz_fighter_handle_center_pos_single_range_move(
                &fighters->fighter_move);
        } else {
            pz_fighter_handle_center_pos_single_close_move(
                &fighters->fighter_move);
        }
        return 1;
    }

    if (player2_distance > 0.4f &&
        fighters->balance_out_of_range == 1 && player2_busy == 0) {
        fighters->fighter_move.player = 1;
        pz_fighter_handle_center_pos_minor_adjustment(
            &fighters->fighter_move);
        fighters->balance_out_of_range = 0;
        return 1;
    }
    if (player1_distance > 0.4f &&
        fighters->balance_out_of_range == 1 && player1_busy == 0) {
        fighters->fighter_move.player = 0;
        pz_fighter_handle_center_pos_minor_adjustment(
            &fighters->fighter_move);
        fighters->balance_out_of_range = 0;
        return 1;
    }
    if (((player1_distance > 0.08f && player2_distance > 0.15f) ||
         (player1_distance > 0.15f && player2_distance > 0.08f)) &&
        player1_busy == 0 && player2_busy == 0) {
        fighters->fighter_move.player = randu0(1) & 0xffff;
        pz_fighter_handle_dual_off_center_Move(&fighters->fighter_move);
        return 1;
    }
    return 0;
}

/* Broad pass: restore idle spacing before re-entering the mode master. */
void pz_fighters_idle_process(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    float player1_distance;
    float player2_distance;
    int moved = 0;

    if (fighters->positioning_active == 0) {
        fighters->fighters_positioned = 0;
        pz_fighters_calc_distance_to_desired_idle_pos(
            &player1_distance, &player2_distance);
        if (player1_distance < -0.1f || player1_distance > 0.1f) {
            fighters->fighter_state[0] = 1;
            xfer_proc(
                pz_fighter_get_player_proc(0),
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (player2_distance < -0.1f || player2_distance > 0.1f) {
            fighters->fighter_state[1] = 1;
            xfer_proc(
                pz_fighter_get_player_proc(1),
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (moved != 0) {
            fighters->positioning_active = 1;
            return;
        }
    }
    fighters->positioning_active = 0;
    fighters->fighters_positioned = 1;
    p_puzzle_fighter_master();
}

/*
 * Broad pass: settle spacing, then dispatch a pending super move for the
 * only fighter whose meter is full enough to own the sequence.
 */
void pz_fighters_inside_super_move_scenerio(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    float player1_distance;
    float player2_distance;
    int moved = 0;
    int player = -1;

    if (fighters->positioning_active == 0) {
        fighters->fighters_positioned = 0;
        pz_fighters_calc_distance_to_desired_idle_pos(
            &player1_distance, &player2_distance);
        if (player1_distance < -0.1f || player1_distance > 0.1f) {
            fighters->fighter_state[0] = 1;
            xfer_proc(
                pz_fighter_get_player_proc(0),
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (player2_distance < -0.1f || player2_distance > 0.1f) {
            fighters->fighter_state[1] = 1;
            xfer_proc(
                pz_fighter_get_player_proc(1),
                pz_fighter_move_into_desired_position);
            moved = 1;
        }
        if (moved != 0) {
            fighters->positioning_active = 1;
            return;
        }
    }

    fighters->positioning_active = 0;
    fighters->fighters_positioned = 1;
    if (p_puzzle_fighter_master() == 1.0f) {
        return;
    }
    if (fighters->pending_move_count != 0) {
        pz_fighters_handle_next_pending_move_simplified();
        return;
    }
    if (fighters->super_move_request_pending != 1) {
        return;
    }

    if (puzzle_fighter_get_super_bar_level(0) > 0.98f &&
        puzzle_fighter_get_super_bar_level(1) <= 0.95f) {
        player = 0;
    } else if (puzzle_fighter_get_super_bar_level(1) > 0.98f &&
               puzzle_fighter_get_super_bar_level(0) <= 0.95f) {
        player = 1;
    }
    if (player >= 0) {
        fighters->random_fatality_active = 1;
        fighters->fighter_move.mode = 2;
        fighters->fighter_move.player = player;
        pz_fighter_handle_in_super_move(&fighters->fighter_move);
        fighters->super_move_request_pending = 0;
    }
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
        PlyrPdata* bomb_player1 = puzzle_player_pdata(0);                 \
        PlyrPdata* bomb_player2 = puzzle_player_pdata(1);                 \
        pz_fighter_shake_camera(0.02f, 3);                                \
        (fighters)->fighter_move.mode = 15;                               \
        if (bomb_player1->state != 0x605 ||                               \
            ((fighters)->attack_policy_flags & 0x10) != 0) {             \
            (fighters)->fighter_state[0] = 3;                             \
            bomb_player1->state |= 0x1200;                               \
            xfer_proc(                                                    \
                pz_fighter_get_player_proc(0),                            \
                pz_fighters_react_to_bomb_explosion);                    \
        }                                                                 \
        if (bomb_player2->state != 0x605 ||                               \
            ((fighters)->attack_policy_flags & 0x10) != 0) {             \
            (fighters)->fighter_state[1] = 3;                             \
            bomb_player2->state |= 0x1200;                               \
            xfer_proc(                                                    \
                pz_fighter_get_player_proc(1),                            \
                pz_fighters_react_to_bomb_explosion);                    \
        }                                                                 \
    } while (0)

#define PZ_DISPATCH_HAPPY_REACTION(fighters)                              \
    do {                                                                  \
        unsigned int happy_player = (fighters)->fighter_move.player;      \
        PlyrPdata* happy_pdata = puzzle_player_pdata(happy_player);       \
        PlyrPdata* unhappy_pdata = happy_pdata->his_plyr_pdata;           \
        pz_fighter_shake_camera(0.02f, 3);                                \
        (fighters)->fighter_move.mode = 15;                               \
        if (happy_pdata->state != 0x605) {                                \
            (fighters)->fighter_state[0] = 2;                             \
            happy_pdata->state |= 0x1200;                                \
            xfer_proc(                                                    \
                pz_fighter_get_player_proc(happy_player),                 \
                pz_fighter_big_time_happy);                              \
        }                                                                 \
        if (unhappy_pdata->state != 0x605) {                              \
            (fighters)->fighter_state[1] = 4;                             \
            unhappy_pdata->state |= 0x1200;                              \
            xfer_proc(                                                    \
                pz_fighter_get_player_proc(unhappy_pdata->plyr_num),      \
                pz_fighter_whatever2);                                   \
        }                                                                 \
    } while (0)

/* Soft ceiling: 48.84% -- broad pending-event dequeue and dispatch pass. */
float pz_fighters_handle_next_pending_move(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFighterMove* move;

    PZ_POP_PENDING_EVENT(fighters);
    move = &fighters->fighter_move;
    switch (move->event_type) {
    case 1:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_special_move(move);
        break;
    case 2:
        fighters->random_fatality_active = 1;
        pz_fighter_perform_end_of_round_anims(
            move->player, move->player == 0);
        fighters->balance = 0.0f;
        pz_fighters_fatality_round_over();
        pz_fighter_calculate_start_pos();
        fighters->round_running = 0;
        break;
    case 3:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_move(move);
        break;
    case 6:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_relief_move(move);
        break;
    case 7:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohno_move(move);
        break;
    case 8:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_BOMB_REACTION(fighters);
        /* Retail intentionally continues into the paired happy reaction. */
    case 9:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_HAPPY_REACTION(fighters);
        break;
    case 10:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_peak_move(move);
        break;
    case 11:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohyeah_move(move);
        break;
    case 19:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_winning_big_based_on_score_move(move);
        break;
    }
    return 0.0f;
}

/*
 * Soft ceiling: 43.25% -- broad simplified dequeue pass; scripted moves are
 * normalized to variants that remain valid during a super-move scenario.
 */
float pz_fighters_handle_next_pending_move_simplified(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PuzzleFighterMove* move;

    PZ_POP_PENDING_EVENT(fighters);
    move = &fighters->fighter_move;
    switch (move->event_type) {
    case 1:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_special_move(move);
        /* Retail intentionally also dispatches the relief reaction. */
    case 6:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_relief_move(move);
        break;
    case 2:
        fighters->random_fatality_active = 1;
        pz_fighter_perform_end_of_round_anims(
            move->player, move->player == 0);
        fighters->balance = 0.0f;
        pz_fighters_fatality_round_over();
        pz_fighter_calculate_start_pos();
        fighters->round_running = 0;
        break;
    case 3:
        fighters->random_fatality_active = 1;
        if (move->script_move == 3 || move->script_move == 4) {
            move->script_move = 5;
        } else if (move->script_move == 6) {
            move->script_move = 7;
        } else if (move->script_move == 8) {
            move->script_move = 9;
        } else if (move->script_move == 10 ||
                   move->script_move == 11) {
            move->script_move = 12;
        }
        pz_fighter_handle_move(move);
        break;
    case 7:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohno_move(move);
        break;
    case 8:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_BOMB_REACTION(fighters);
        /* Retail intentionally continues into the paired happy reaction. */
    case 9:
        fighters->random_fatality_active = 1;
        PZ_DISPATCH_HAPPY_REACTION(fighters);
        break;
    case 10:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_peak_move(move);
        break;
    case 11:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_ohyeah_move(move);
        break;
    case 19:
        fighters->random_fatality_active = 1;
        pz_fighter_handle_winning_big_based_on_score_move(move);
        break;
    }
    return 0.0f;
}

/*
 * Soft ceiling: 34.77% -- consume bomb, celebration, super-enable, and
 * character-special requests once both fighter processes can be moved.
 */
void pz_fighter_process_immediate_request(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    PlyrPdata* player1 = puzzle_player_pdata(0);
    PlyrPdata* player2 = puzzle_player_pdata(1);
    MkObj* object1 = puzzle_fighter_object(0);
    MkObj* object2 = puzzle_fighter_object(1);
    int handled = 0;

    if (fighters->random_fatality_active != 0 &&
        (fighters->attack_policy_flags & 0x10) != 0 &&
        fighters->immediate_request_type != 8 &&
        fighters->immediate_request_type != 9 &&
        player1->state == 0x605 && player2->state == 0x605) {
        return;
    }

    fighters->immediate_request_active = 0;
    if (fighters->immediate_request_type == 8) {
        pz_fighter_shake_camera(0.02f, 3);
        fighters->fighter_move.mode = 15;
        if (player1->state != 0x605 ||
            (fighters->attack_policy_flags & 0x10) != 0) {
            fighters->fighter_state[0] = 3;
            player1->state |= 0x1200;
            xfer_proc(
                pz_fighter_get_player_proc(0),
                pz_fighters_react_to_bomb_explosion);
            handled = 1;
        }
        if (player2->state != 0x605 ||
            (fighters->attack_policy_flags & 0x10) != 0) {
            fighters->fighter_state[1] = 3;
            player2->state |= 0x1200;
            xfer_proc(
                pz_fighter_get_player_proc(1),
                pz_fighters_react_to_bomb_explosion);
            handled = 1;
        }
        if (handled != 0) {
            fighters->random_fatality_active = 1;
            fighters->attack_policy_word = 0x10000000;
        }
        return;
    }

    if (fighters->immediate_request_type == 9) {
        if ((fighters->attack_policy_flags & 0x10) == 0) {
            unsigned int happy_player =
                fighters->immediate_request_player;
            PlyrPdata* happy_pdata =
                puzzle_player_pdata(happy_player);
            PlyrPdata* unhappy_pdata = happy_pdata->his_plyr_pdata;

            pz_fighter_shake_camera(0.02f, 3);
            fighters->fighter_move.mode = 15;
            if (happy_pdata->state != 0x605) {
                fighters->fighter_state[0] = 2;
                happy_pdata->state |= 0x1200;
                xfer_proc(
                    pz_fighter_get_player_proc(happy_player),
                    pz_fighter_big_time_happy);
                handled = 1;
            }
            if (unhappy_pdata->state != 0x605) {
                fighters->fighter_state[1] = 4;
                unhappy_pdata->state |= 0x1200;
                xfer_proc(
                    pz_fighter_get_player_proc(unhappy_pdata->plyr_num),
                    pz_fighter_whatever2);
                handled = 1;
            }
        } else {
            pz_fighter_shake_camera(0.02f, 3);
            fighters->fighter_move.mode = 15;
            if (player1->state != 0x605 ||
                (fighters->attack_policy_flags & 0x10) != 0) {
                fighters->fighter_state[0] = 3;
                player1->state |= 0x1200;
                xfer_proc(
                    pz_fighter_get_player_proc(0),
                    pz_fighters_react_to_bomb_explosion);
                handled = 1;
            }
            if (player2->state != 0x605 ||
                (fighters->attack_policy_flags & 0x10) != 0) {
                fighters->fighter_state[1] = 3;
                player2->state |= 0x1200;
                xfer_proc(
                    pz_fighter_get_player_proc(1),
                    pz_fighters_react_to_bomb_explosion);
                handled = 1;
            }
        }
        if (handled != 0) {
            fighters->random_fatality_active = 1;
            fighters->attack_policy_word = 0x10000000;
        }
        return;
    }

    if (fighters->immediate_request_type == 18) {
        object1->flags_09 |= 0x80;
        fighters->y_constraint_enabled[object1->oid == 0x1002] = 0;
        object2->flags_09 |= 0x80;
        fighters->y_constraint_enabled[object2->oid == 0x1002] = 0;
        fighters->random_fatality_active = 1;
        fighters->attack_policy_word = 0;
        fighters->fighter_move.mode = 6;
        fighters->fighter_move.player =
            fighters->immediate_request_player;
        pz_fighter_handle_super_move_available(&fighters->fighter_move);
        return;
    }

    if (pz_fighter_should_handle_special_move(
            fighters->immediate_request_player) == 1) {
        object1->flags_09 |= 0x80;
        fighters->y_constraint_enabled[object1->oid == 0x1002] = 0;
        object2->flags_09 |= 0x80;
        fighters->y_constraint_enabled[object2->oid == 0x1002] = 0;
        fighters->fighter_move.mode = 15;
        fighters->attack_policy_word = 0x80000000;
        fighters->fighter_move.script_move =
            fighters->immediate_request_type;
        fighters->fighter_move.player =
            fighters->immediate_request_player;
        fighters->random_fatality_active = 1;
        pz_fighter_handle_special_move(&fighters->fighter_move);
    }
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
        MkObj* event_fighter = puzzle_fighter_object((move_ptr)->player);  \
        PlyrPdata* event_pdata =                                          \
            puzzle_player_pdata((move_ptr)->player);                      \
        float event_x =                                                   \
            (move_ptr)->player == 0                                      \
                ? g_pz_fighters_engine.player1_idle_x                    \
                : g_pz_fighters_engine.player2_idle_x;                   \
        float event_z =                                                   \
            (move_ptr)->player == 0                                      \
                ? g_pz_fighters_engine.player1_idle_z                    \
                : g_pz_fighters_engine.player2_idle_z;                   \
        float event_dx = event_x - event_fighter->pos.x;                  \
        float event_dz = event_z - event_fighter->pos.z;                  \
        float event_distance =                                            \
            event_dx * event_dx + event_dz * event_dz;                   \
        if (event_distance >= 2.45f) {                                    \
            (move_ptr)->distance_class =                                  \
                event_distance >= 6.2f ? 4 : 2;                          \
        } else {                                                          \
            (move_ptr)->distance_class = 1;                               \
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
 * Soft ceiling family: 30.52-38.11% -- first broad event-dispatch pass;
 * common setup is semantically complete but still has MWCC ordering drift.
 */
float pz_fighter_handle_dual_off_center_Move(PuzzleFighterMove* move) {
    PuzzleProcess* active_proc;
    PuzzleProcess* other_proc;
    unsigned short roll;

    PZ_PREPARE_FIGHTER_EVENT(move);
    active_proc = pz_fighter_get_player_proc(move->player);
    other_proc = pz_fighter_get_player_proc(move->player == 0 ? 1 : 0);
    roll = (unsigned short)randu0(100);
    if (roll < 50) {
        xfer_proc(active_proc, pz_fighter_light_propell);
        xfer_proc(other_proc, pz_fighter_dummy_propell);
    } else if (roll < 75) {
        xfer_proc(active_proc, pz_fighter_light_propell);
        xfer_proc(other_proc, pz_fighter_smart_flippy);
    } else {
        xfer_proc(active_proc, pz_fighter_smart_flippy);
        xfer_proc(other_proc, pz_fighter_smart_flippy);
    }
    return 0.0f;
}

float pz_fighter_handle_center_pos_minor_adjustment(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_center_pos_minor_adjustement);
    return 0.0f;
}

float pz_fighter_handle_center_pos_single_close_move(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_center_pos_single_close_move);
    return 0.0f;
}

float pz_fighter_handle_center_pos_single_range_move(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_center_pos_single_range_move);
    return 0.0f;
}

float pz_fighter_handle_center_pos_range_attack(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_center_pos_range_attack);
    return 0.0f;
}

float pz_fighter_handle_distance_attack(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_dist_attack);
    return 0.0f;
}

float pz_fighter_handle_off_wall_attack(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_off_wall_attack);
    return 0.0f;
}

float pz_fighter_handle_winning_big_based_on_score_move(
    PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    g_pz_fighters_engine.attack_has_followup = 0;
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_give_present);
    return 0.0f;
}

float pz_fighter_handle_super_move_available(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_super_move_just_enabled);
    xfer_proc(
        pz_fighter_get_player_proc(move->player == 0 ? 1 : 0),
        pz_fighter_perform_other_guy_super_move_just_enabled);
    return 0.0f;
}

float pz_fighter_handle_ohyeah_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_ohyeah_move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player == 0 ? 1 : 0),
        pz_fighter_perform_other_guy_ohyeah);
    return 0.0f;
}

float pz_fighter_handle_in_super_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_holding_onto_super_move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player == 0 ? 1 : 0),
        pz_fighter_perform_other_guy_holding_onto_super_move);
    return 0.0f;
}

float pz_fighter_handle_ohno_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_ohno_move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player == 0 ? 1 : 0),
        pz_fighter_perform_other_guy_ohno);
    return 0.0f;
}

float pz_fighter_handle_peak_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_peak_move);
    return 0.0f;
}

float pz_fighter_handle_relief_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_relief_move);
    return 0.0f;
}

float pz_fighter_handle_special_move(PuzzleFighterMove* move) {
    PZ_PREPARE_FIGHTER_EVENT(move);
    xfer_proc(
        pz_fighter_get_player_proc(move->player),
        pz_fighter_perform_special_move);
    return 0.0f;
}

float pz_fighter_handle_move(PuzzleFighterMove* move) {
    unsigned int script_move = move->script_move;

    PZ_PREPARE_FIGHTER_EVENT(move);
    if (script_move == 3 &&
        puzzle_fighter_get_super_bar_level(move->player == 0) > 0.95f) {
        script_move = 5;
    }
    if (script_move < 14) {
        move->script_move = script_move;
        xfer_proc(
            pz_fighter_get_player_proc(move->player),
            pz_fighter_perform_scripted_move);
    }
    return 0.0f;
}

#undef PZ_PREPARE_FIGHTER_EVENT

void pz_fighter_set_y_constrain(float y, MkObj* fighter, int enabled) {
    int player;

    player = 0;
    if (fighter->oid == 0x1002) {
        player = 1;
    }
    if (enabled == 0) {
        fighter->flags_09 |= 0x80;
        g_pz_fighters_engine.y_constraint_enabled[player] = 0;
        return;
    }

    fighter->flags_09 &= ~0x80;
    g_pz_fighters_engine.y_constraint_enabled[player] = 1;
    g_pz_fighters_engine.y_constraint[player] = y;
}

void pz_fighter_force_repel_during_attack(void) {
    g_pz_fighters_engine.force_repel = 1;
}

/*
 * Broad pass: shared Puzzle attack setup, distance policy, reaction window,
 * and animation completion pipeline.
 */
void pz_fighter_ani_attack(
    int reaction, float end_frame, float reaction_frame,
    float hit_distance, int reaction_mode);
void pz_fighter_startup_attack(
    AniScript* animation, float frame1, float frame2, float frame3,
    int field0C, int field10, int field14, float desired_distance,
    float frame4, int reaction_mode);

/* Soft ceiling: pz_fighter_attack 66.34% -- broad descriptor dispatch. */
void pz_fighter_attack(
    AniScript* animation, PuzzleAttackParameters* attack, int reaction) {
    float desired_distance = attack->desired_distance;

    if (attack->walk_to_range == 1) {
        if (pz_fighter_walk_until_fight_distance() != 1) {
            if (his_pdata->state == 0 &&
                his_pdata->fatality_shove_active == 0 &&
                his_pdata->previous_state != 0x4203) {
                xfer_proc(
                    pz_fighter_get_player_proc(his_pdata->plyr_num),
                    pz_fighter_move_into_fighting_position_now);
            }
            pz_fighter_walk_FB_true(
                pz_fighter_walk_until_fight_distance, 120, 1);
        }
        if (g_pz_fighters_engine.breakout == 1) {
            aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
        }
    }

    g_pz_fighters_engine.attack_has_followup =
        attack->has_followup != 0;
    if ((g_pz_fighters_engine.attack_policy_flags & 8) != 0) {
        g_pz_fighters_engine.attack_runtime_flags |= 0x80;
    } else {
        g_pz_fighters_engine.attack_runtime_flags &= ~0x80;
    }
    if (attack->ignore_distance == 1) {
        desired_distance = 0.0f;
    }

    pz_fighter_startup_attack(
        animation, attack->field_00, attack->field_04,
        attack->field_08, attack->field_0C, attack->field_10,
        attack->field_14, desired_distance, attack->field_1C,
        attack->reaction_mode);
    pz_fighter_ani_attack(
        reaction, attack->end_frame, attack->reaction_frame,
        attack->hit_distance, attack->reaction_mode);
    g_pz_fighters_engine.force_repel = 0;
}

void pz_fighter_dont_fudge_desired_distance(void) {
    g_pz_fighters_engine.distance_flags |= PZ_FIGHTER_DISTANCE_FIXED;
}

/* Soft ceiling: pz_fighter_ani_attack 74.21% -- broad frame/reaction loop. */
void pz_fighter_ani_attack(
    int reaction, float end_frame, float reaction_frame,
    float hit_distance, int reaction_mode) {
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

    if (reaction_started != 0 ||
        (g_pz_fighters_engine.distance_flags & 1) != 0) {
        if (his_pdata->plyr_num == 0) {
            g_pz_fighters_engine.fighter_state[0] =
                reaction_started != 0 ? 4 : 0;
        } else {
            g_pz_fighters_engine.fighter_state[1] =
                reaction_started != 0 ? 4 : 0;
        }
    }
    his_pdata->state &= ~0x800;
}

/* Soft ceiling: pz_fighter_startup_attack 61.53% -- broad distance policy. */
void pz_fighter_startup_attack(
    AniScript* animation, float frame1, float frame2, float frame3,
    int field0C, int field10, int field14, float desired_distance,
    float frame4, int reaction_mode) {
    float distance;
    float correction;

    if ((g_pz_fighters_engine.distance_flags &
         PZ_FIGHTER_DISTANCE_FIXED) == 0) {
        distance = xz_distance_between_players();
        if (desired_distance < 0.5f &&
            g_pz_fighters_engine.force_repel == 0) {
            ((MkObj*)g_game_info.plyr0.slot.mirror_a)->flags_09 &= ~0x10;
            ((MkObj*)g_game_info.plyr1.slot.mirror_a)->flags_09 &= ~0x10;
        }

        if (distance > desired_distance - 0.05f &&
            distance < desired_distance + 0.05f) {
            pz_fighter_snap_to_distance(desired_distance, distance);
        } else if (distance > desired_distance - 1.0f &&
                   distance < desired_distance + 0.3f) {
            correction = (distance - desired_distance) / 3.0f;
            if (correction < 0.0f) {
                force_away(-correction, 5, 0.3f, 3);
            } else {
                force_forward(correction, 5, 0.3f, 3);
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
        g_pz_fighters_engine.fighter_state[his_pdata->plyr_num] = 5;
    }

    attack_to_frame_x(
        frame1, frame2, frame3, frame4, animation,
        field0C, field10, field14);
}

/* Soft ceiling: pz_fighter_move_into_desired_position 80.20%. */
float pz_fighter_move_into_desired_position(void) {
    MkObj* player1 = puzzle_fighter_object(0);
    MkObj* player2 = puzzle_fighter_object(1);
    float dx;
    float dz;
    float distance1;
    float distance2;
    float distance;

    dz = player1->pos.z - g_pz_fighters_engine.player1_idle_z;
    dx = player1->pos.x - g_pz_fighters_engine.player1_idle_x;
    distance1 = dx * dx + dz * dz;
    if (dx > 0.0f) {
        distance1 = -distance1;
    }

    dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
    dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
    distance2 = dx * dx + dz * dz;
    if (dx < 0.0f) {
        distance2 = -distance2;
    }

    distance = plyr_pdata->plyr_num == 0 ? distance1 : distance2;
    if (distance < -0.005f) {
        pz_fighter_walk_FB_true(
            pz_fighter_walk_until_danger_or_in_wrong_direction, 120, 0);
    } else if (distance > 0.005f) {
        pz_fighter_walk_FB_true(
            pz_fighter_walk_until_danger_or_in_wrong_direction, 120, 1);
    }
    return aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
}

/* Soft ceiling: pz_fighter_move_into_fighting_position 78.55%. */
void pz_fighter_move_into_fighting_position(void) {
    if (pz_fighter_walk_until_fight_distance() != 1) {
        if (his_pdata->state == 0 &&
            his_pdata->fatality_shove_active == 0 &&
            his_pdata->previous_state != 0x4203) {
            xfer_proc(
                pz_fighter_get_player_proc(his_pdata->plyr_num),
                pz_fighter_move_into_fighting_position_now);
        }
        pz_fighter_walk_FB_true(
            pz_fighter_walk_until_fight_distance, 120, 1);
    }
}

/* Soft ceiling: pz_fighter_move_into_fighting_position_now 99.52% - pool. */
float pz_fighter_move_into_fighting_position_now(void) {
    pz_fighter_walk_FB_true(
        pz_fighter_walk_until_fight_distance, 120, 1);
    aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    return 0.0f;
}

void pz_fighters_calc_distance_to_desired_idle_pos_abs(
    float* player1_distance, float* player2_distance,
    float* player1_absolute, float* player2_absolute) {
    MkObj* player1 = puzzle_fighter_object(0);
    MkObj* player2 = puzzle_fighter_object(1);
    float dx;
    float dz;

    dz = player1->pos.z - g_pz_fighters_engine.player1_idle_z;
    dx = player1->pos.x - g_pz_fighters_engine.player1_idle_x;
    *player1_distance = dx * dx + dz * dz;
    *player1_absolute = *player1_distance;
    if (dx > 0.0f) {
        *player1_distance = -*player1_distance;
    }

    dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
    dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
    *player2_distance = dx * dx + dz * dz;
    *player2_absolute = *player2_distance;
    if (dx < 0.0f) {
        *player2_distance = -*player2_distance;
    }
}

void pz_fighters_calc_distance_to_desired_idle_pos(
    float* player1_distance, float* player2_distance) {
    MkObj* player1 = puzzle_fighter_object(0);
    MkObj* player2 = puzzle_fighter_object(1);
    float dx;
    float dz;

    dz = player1->pos.z - g_pz_fighters_engine.player1_idle_z;
    dx = player1->pos.x - g_pz_fighters_engine.player1_idle_x;
    *player1_distance = dx * dx + dz * dz;
    if (dx > 0.0f) {
        *player1_distance = -*player1_distance;
    }

    dz = player2->pos.z - g_pz_fighters_engine.player2_idle_z;
    dx = player2->pos.x - g_pz_fighters_engine.player2_idle_x;
    *player2_distance = dx * dx + dz * dz;
    if (dx < 0.0f) {
        *player2_distance = -*player2_distance;
    }
}

/*
 * Broad pass: split a requested spacing correction across both fighters,
 * preserve their vertical positions, and make them face each other.
 * Soft ceiling: pz_fighter_snap_to_distance 22.30% -- clean math lift.
 */
void pz_fighter_snap_to_distance(
    float desired_distance_squared, float current_distance_squared) {
    Vec direction;
    Vec my_position = plyr_obj->pos;
    Vec his_position = his_obj->pos;
    Vec my_angle = plyr_obj->ang;
    Vec his_angle = his_obj->ang;
    float desired_distance;
    float current_distance;
    float correction;

    desired_distance =
        desired_distance_squared > 0.0f
            ? (float)sqrt((double)desired_distance_squared)
            : 0.0f;
    current_distance =
        current_distance_squared > 0.0f
            ? (float)sqrt((double)current_distance_squared)
            : 0.0f;
    correction = (current_distance - desired_distance) * 0.5f;

    xz_unit_vector(&direction, &my_position, &his_position);
    my_position.x += direction.x * correction;
    my_position.z += direction.z * correction;
    his_position.x -= direction.x * correction;
    his_position.z -= direction.z * correction;

    my_angle.y = gxMathArcTanYX(direction.x, direction.z);
    his_angle.y = my_angle.y + 3.1415927f;
    if (his_angle.y > 3.1415927f) {
        his_angle.y -= 6.2831855f;
    }
    move_player(plyr_obj, &my_position, &my_angle);
    move_player(his_obj, &his_position, &his_angle);
    xz_distance_between_players();
}

/*
 * Broad pass: derive center, idle, and post positions from the arena axis.
 * Soft ceiling: pz_fighter_calculate_start_pos 37.61% -- clean math lift.
 */
void pz_fighter_calculate_start_pos(void) {
    PuzzleFightersEngine* fighters = &g_pz_fighters_engine;
    MkObj* player1 = puzzle_fighter_object(0);
    MkObj* player2 = puzzle_fighter_object(1);
    float screen_scale = screen_width > 650 ? 1.1f : 1.0f;
    float center_x = fighters->arena_axis_x * fighters->balance;
    float center_z = fighters->arena_axis_z * fighters->balance;
    float post1_scale = -1.6f * screen_scale;
    float post2_scale = 1.6f * screen_scale;

    fighters->center_x = center_x;
    fighters->center_y = 0.0f;
    fighters->center_z = center_z;

    fighters->player1_idle_x =
        center_x + 0.5f * fighters->arena_axis_x;
    fighters->player1_idle_y = player1->pos.y;
    fighters->player1_idle_z =
        center_z + 0.5f * fighters->arena_axis_z;
    fighters->player2_idle_x =
        center_x - 0.5f * fighters->arena_axis_x;
    fighters->player2_idle_y = player2->pos.y;
    fighters->player2_idle_z =
        center_z - 0.5f * fighters->arena_axis_z;

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
    MkObj* fighter = puzzle_fighter_object(player);
    float dx = g_pz_fighters_engine.center_x - fighter->pos.x;
    float dz = g_pz_fighters_engine.center_z - fighter->pos.z;
    float distance = dx * dx + dz * dz;

    if ((player == 0) &&
        (g_pz_fighters_engine.center_x < fighter->pos.x)) {
        distance = -distance;
    }
    if ((player == 1) &&
        (g_pz_fighters_engine.center_x > fighter->pos.x)) {
        distance = -distance;
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

/* Soft ceiling: retail returns 0.0f after the non-returning transfers. */
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
    if (plyr_pdata->state != 0x2000 ||
        g_pz_fighters_engine.fighter_state[player] != 3) {
        g_pz_fighters_engine.fighter_state[player] = 0;
    }

    init_ground_move();
    plyr_obj->flags_09 |= 0x10;
    back_to_normal(plyr_obj, 1);
    player = plyr_obj->oid == 0x1002;
    plyr_obj->flags_09 |= 0x80;
    g_pz_fighters_engine.y_constraint_enabled[player] = 0;
    aproc->vtbl->transfer(p_plyr_pz_fighter_loop, 0.0f);
    return 0.0f;
}

/* Soft ceiling: 99.00%; only shared 0.8f/1.0f pool relocations differ. */
static float p_plyr_pz_fighter_loop(void) {
    rotate_towards_him(0.8f);
    return 1.0f;
}

/* Soft ceiling: pz_fighter_is_losing_big ~96.11% - branch emit shape. */
int pz_fighter_is_losing_big(int player) {
    if (player == 0) {
        if (g_pz_fighters_engine.balance > 0.5f) {
            return 1;
        }
    } else if (g_pz_fighters_engine.balance < -0.5f) {
        return 1;
    }
    return 0;
}

/* Soft ceiling: pz_fighter_is_winning_big ~96.11% - branch emit shape. */
int pz_fighter_is_winning_big(int player) {
    if (player == 1) {
        if (g_pz_fighters_engine.balance > 0.5f) {
            return 1;
        }
    } else if (g_pz_fighters_engine.balance < -0.5f) {
        return 1;
    }
    return 0;
}

float p_plyr_pz_fighter_start(void) {
    if ((g_pz_fighters_engine.start_flags & 0x80) == 0) {
        return 1.0f;
    }

    face_opponent_now();
    if (plyr_pdata->plyr_num == 1 &&
        (plyr_obj->hide_flags & 0x40) != 0) {
        plyr_obj->hide_flags &= ~0x40;
    }
    glitch_to_stance(1.0f);
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
    aproc->vtbl->transfer(p_plyr_pz_fighter_entry, 0.0f);
    return 0.0f;
}

float pz_fighter_check_breakout(void) {
    if (g_pz_fighters_engine.breakout == 1) {
        aproc->vtbl->transfer(pz_fighter_exit, 0.0f);
    }
    return 0.0f;
}

int pz_fighter_should_he_breakout(void) {
    return g_pz_fighters_engine.breakout;
}

/* Soft ceiling: pz_fighter_close_enough_to_super_move ~99.58% - pool label. */
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
    if (player == 0) {
        return g_game_info.plyr0.idle_proc;
    }
    return g_game_info.plyr1.idle_proc;
}

PlyrPdata* pz_get_pdata_by_id(int player) {
    return puzzle_player_pdata(player);
}

PuzzleFighterMove* pz_get_fighter_move(void) {
    return &g_pz_fighters_engine.fighter_move;
}

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
    xfer_proc(pz_fighter_get_player_proc(event->side), reaction);
    return 0.0f;
}

/*
 * Soft ceiling: pz_fighter_first_block_has_been_placed ~86.08% - the
 * retail table-offset cursor reloads its base after selection; this typed
 * row walk leaves only cursor scheduling and nonvolatile allocation drift.
 */
void pz_fighter_first_block_has_been_placed(unsigned int player) {
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
                pz_fighter_get_player_proc(
                    g_pz_fighters_engine.fighter_move.player),
                reaction);
            return;
        }
    }
}

#define PZ_CANCEL_ACTIVE_MOVE()                                      \
    do {                                                             \
        g_pz_fighters_engine.random_fatality_active = 0;             \
        g_pz_fighters_engine.attack_policy_word = 0;                 \
        g_pz_fighters_engine.fighter_state[0] = 0;                   \
        g_pz_fighters_engine.fighter_state[1] = 0;                   \
    } while (0)

/*
 * Soft ceiling: pz_fighter_fight_request ~57.27% - the request policy,
 * continuation rules, pending-queue pruning, and breakout thresholds are
 * recovered; retail's deeply split predicate and duplicated clear regions
 * still produce substantially different control-flow scheduling.
 */
void pz_fighter_fight_request(
    unsigned int player, unsigned int block_count, int chain_count,
    unsigned int event_type) {
    PuzzleFighterMove* current;
    PuzzleFighterMove* pending;
    unsigned int move;
    unsigned int priority;
    unsigned int index;
    unsigned int copy_index;
    int continuation;
    int accepted;

    pz_fighter_classify_move_8012260C(
        block_count, chain_count, &move, &priority, event_type);
    current = &g_pz_fighters_engine.fighter_move;
    pending = g_pz_fighters_engine.pending_moves;

    if (g_pz_fighters_engine.random_fatality_active == 1 &&
        priority < 15 &&
        puzzle_fighter_get_super_bar_level(0) <= 0.95f &&
        puzzle_fighter_get_super_bar_level(1) <= 0.95f &&
        g_pz_fighters_engine.immediate_request_active == 0) {
        pz_fighter_disallow_continuation();
        continuation = 0;
    } else {
        continuation =
            g_pz_fighters_engine.random_fatality_active == 1 &&
            current->player == player &&
            (current->runtime_flags & 0x80) != 0 &&
            event_type <= 1 &&
            (priority > 1 || move == 1 ||
             (current->policy_flags & 0x02) != 0);
    }

    if (continuation) {
        current->policy_flags |= 0x04;
        g_pz_fighters_engine.continuation_move = move;
        g_pz_fighters_engine.continuation_priority = priority;
        return;
    }

    accepted = 1;
    if (move == 13) {
        if (g_pz_fighters_engine.random_fatality_active == 1 &&
            current->script_move == 13) {
            accepted = 0;
        } else {
            for (index = 0;
                 index < g_pz_fighters_engine.pending_move_count;
                 index++) {
                if (pending[index].script_move == 13) {
                    accepted = 0;
                    break;
                }
            }
        }
    } else if (
        g_pz_fighters_engine.random_fatality_active == 1 &&
        current->mode != 15 &&
        priority + 3 < (unsigned int)current->mode) {
        accepted = 0;
    }

    if (!accepted) {
        return;
    }

    if (g_pz_fighters_engine.random_fatality_active == 1) {
        if ((priority > 1 &&
             current->has_followup == 1 &&
             current->player != player &&
             (current->mode == 1 ||
              priority > (unsigned int)current->mode + 1)) ||
            ((event_type > 4 || priority > 2) &&
             current->has_followup == 1 &&
             current->player == player &&
             current->mode == 1)) {
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
            if (pending[index].mode == 1) {
                for (copy_index = index;
                     copy_index <
                     g_pz_fighters_engine.pending_move_count;
                     copy_index++) {
                    memcpy(
                        &pending[copy_index],
                        &pending[copy_index + 1],
                        sizeof(PuzzleFighterMove));
                }
                g_pz_fighters_engine.pending_move_count--;
            } else {
                index++;
            }
        }
    }

    if (g_pz_fighters_engine.random_fatality_active == 1) {
        if (current->player == player) {
            if ((priority == 14 || priority == 15) &&
                current->mode < 14) {
                g_pz_fighters_engine.breakout = 1;
            }
        } else if (
            (priority == 11 && current->mode < 10) ||
            ((priority == 12 || priority == 13) &&
             current->mode < 12) ||
            ((priority == 14 || priority == 15) &&
             current->mode < 14)) {
            g_pz_fighters_engine.breakout = 1;
        }
    }

    switch (event_type) {
    case 3:
        pz_fighter_buffer_new_move(1, player, move, priority);
        break;
    case 8:
        pz_fighter_buffer_new_move(11, player, 8, 15);
        break;
    case 9:
        pz_fighter_buffer_new_move(19, player, 9, 15);
        break;
    case 6:
        pz_fighter_buffer_new_move(7, player, 6, 15);
        break;
    case 7:
        pz_fighter_buffer_new_move(10, player, 7, 15);
        break;
    case 5:
        pz_fighter_buffer_new_move(6, player, 5, 15);
        break;
    default:
        pz_fighter_buffer_new_move(3, player, move, priority);
        break;
    }
}

#undef PZ_CANCEL_ACTIVE_MOVE
