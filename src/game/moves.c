#include "runtime/mk_pdata.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "game/game_info.h"
#include "math/gxMath.h"
#include "platform/io.h"
#include "platform/main.h"

typedef struct MovesSidekickPdata {
    MkHdr hdr;
    PlyrPdata* player;
} MovesSidekickPdata;

typedef union MovesSidekickPdataRef {
    MkHdr* hdr;
    MovesSidekickPdata* sidekick;
} MovesSidekickPdataRef;

typedef struct MovesSwitchPdata {
    MkHdr hdr;
    PlyrInfo* player;
} MovesSwitchPdata;

typedef struct MovesWeaponWatchPdata {
    MkHdr hdr;
    PlyrPdata* player;
    unsigned int player_instance;
    unsigned int transient_proc_instance;
    int timeout;
} MovesWeaponWatchPdata;

typedef struct MovesWeaponStyleData {
    char pad00[4];
    void* primary_weapon;
    void* secondary_weapon;
} MovesWeaponStyleData;

typedef struct MovesStyle {
    char pad00[4];
    MovesWeaponStyleData* weapon_data;
} MovesStyle;

typedef struct MovesSharedAnimations {
    char pad00[0x1A8];
    AniData* wall_dodge_a;
    AniData* wall_dodge_b;
    char pad1B0[0x30];
    AniData* jump_towards;
    char pad1E4[4];
    AniData* ass_rollup;
    char pad1EC[4];
    AniData* front_roll_left;
    AniData* front_roll_right;
    AniData* back_roll_left;
    AniData* back_roll_right;
    char pad200[0x48];
    AniData* front_rollup;
    char pad24C[0x24];
    AniData* victory_turn;
    char pad274[0x34];
    AniData* block_intro;
    AniData* block_loop;
    char pad2B0[4];
    AniData* block_b_intro;
    AniData* block_b_loop;
    char pad2BC[8];
    AniData* block_c_intro;
    AniData* block_c_loop;
    char pad2CC[4];
    AniData* block_d_intro;
    AniData* block_d_loop;
    char pad2D8[4];
    AniData* block_a_intro;
    char pad2E0[0x40];
    AniData* sidekick_charge;
    AniData* fall_dead;
} MovesSharedAnimations;

typedef struct MovesFighterDefinitionView {
    char pad00[0x74];
    AniData* stance;
    char pad78[0x20];
    AniData* weapon_block_intro;
    AniData* weapon_block_loop;
} MovesFighterDefinitionView;

typedef struct MovesSidekickBlendData {
    char pad00[0x50];
    float exit_step;
} MovesSidekickBlendData;

typedef struct MovesSidekickFighterDefinition {
    char pad00[4];
    MovesSidekickBlendData* blend_data;
    char pad08[0x80];
    AniData* switch_animation;
    AniData* exit_animation;
} MovesSidekickFighterDefinition;

typedef struct MovesSidekickStateView {
    char pad00[0x72C];
    MkObj* sidekick_obj;
    unsigned int sidekick_instance;
    MkProc* sidekick_anim_proc;
    unsigned int sidekick_anim_proc_instance;
    int sidekick_active;
    int sidekick_available;
} MovesSidekickStateView;

typedef struct MovesSidekickSwitchState {
    char pad00[0x10];
    PlyrPdata* opponent;
    MkObj* opponent_obj;
    PlyrInfo* player_info;
    char pad1C[4];
    MkProc* player_proc;
    unsigned int player_proc_instance;
    char pad28[0x704];
    MkObj* sidekick_obj;
    unsigned int sidekick_instance;
    MkProc* sidekick_anim_proc;
    unsigned int sidekick_anim_proc_instance;
} MovesSidekickSwitchState;

typedef struct MovesSidekickActionView {
    char pad00[0x318];
    AniData* charge_exit_animation;
    char pad31C[0x18];
    AniData* projectile_animation;
    char pad338[0x0C];
    AniData* common_exit_animation;
    char pad348[0x14];
    AniData* noob_entrance_animation;
    char pad360[0x118];
    ScriptSlot* cmo;
} MovesSidekickActionView;

typedef struct MovesAnimPdataView {
    AnimPdata base;
    float landing_start;
    float landing_end;
} MovesAnimPdataView;

typedef struct MovesProcessLatchView {
    char pad00[0x5C];
    MkProc* anim_proc;
    unsigned int anim_proc_instance;
} MovesProcessLatchView;

typedef struct MovesSpearLatchView {
    char pad00[0x100];
    MkProc* spear_proc;
    unsigned int spear_proc_instance;
} MovesSpearLatchView;

typedef struct MovesMoveDataView {
    char pad00[0x2B0];
    int move_advance_latch;
    char pad2B4[0x6C];
    AniData* spear_tug;
    char pad324[0x28];
    AniData* boss_spear_tug;
} MovesMoveDataView;

typedef struct MovesDeathDataView {
    char pad00[0x6F4];
    int death_animation_active;
} MovesDeathDataView;

typedef struct MovesAttackStateView {
    char pad00[0x23C];
    int attack_phase;
    char pad240[0x24];
    int attack_counter;
    int shared_attack_until;
    unsigned int last_voice_tick;
    char pad270[4];
    int attack_start_tick;
} MovesAttackStateView;

typedef struct MovesBlockStateView {
    char pad00[0x5B0];
    int drone_block_latch;          /* +0x5B0 */
    char pad5B4[0x124];
    int block_counter;             /* +0x6D8 */
    unsigned int previous_block_tick; /* +0x6DC */
    char pad6E0[4];
    int block_reserve;             /* +0x6E4 */
} MovesBlockStateView;

typedef struct MovesAttackInfo {
    float attack_frame;
    float collision_frame;
    float attack_x;
    float attack_y;
    float attack_z;
    int attack_arg1;
    int attack_arg2;
    int attack_arg3;
    int attack_region;
    float collision_x;
    float collision_y;
    float collision_z;
    unsigned int block_requirement;
} MovesAttackInfo;

typedef struct MovesBossAnimationView {
    char pad00[0x348];
    AniData* walk_animation;
    char pad34C[0x30];
    AniData* end_round_animation;
} MovesBossAnimationView;

typedef struct MovesVictoryData {
    char pad00[0x138];
    int victory_script;
} MovesVictoryData;

typedef struct MovesSwitchLogEntry {
    int switch_id;
    int switch_value;
    const char* label;
    unsigned int pad_state;
    int mapped_index;
} MovesSwitchLogEntry;

typedef struct MovesGameInfoView {
    char pad00[0x64];
    MkPtr* pickup_list;
} MovesGameInfoView;

typedef struct MovesGameStateView {
    char pad00[0x18];
    int state;
} MovesGameStateView;

typedef struct MovesPickupTransform {
    char pad00[0x30];
    Vec position;
} MovesPickupTransform;

typedef struct MovesPickup {
    MkHdr hdr;
    char pad08[0x0C];
    MovesPickupTransform* transform_a;
    MovesPickupTransform* transform_b;
} MovesPickup;

extern PlyrPdata* plyr_pdata;
extern PlyrPdata* his_pdata;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern int exec_tick_ctr;
extern int game_tick_ctr;
extern int round_winner;
extern int victory_proper_flip_flags;
extern int f_fatality_available;
extern int f_fatality_was_done;
extern int f_fatality_finished;
extern int g_min_time_in_block_for_drone;
extern int g_drone_blocking_in_reaction;
extern int g_drone_faked_out;
extern float drone_blocking_done(void);
extern float drone_start(void);
extern int force_midpoint_calculation_update;
extern float p_camera_proc(void);
extern MovesSwitchPdata* switch_pdata;
extern MovesSharedAnimations shared_ani;
extern int p1_log_index;
extern int p2_log_index;
extern int p1_current_log_index;
extern int p2_current_log_index;
extern int p1_current_switch_bit;
extern int p1_current_switch_time;
extern int p1_last_switch_bit;
extern int p1_last_switch_time;
extern int p2_current_switch_bit;
extern int p2_current_switch_time;
extern int p2_last_switch_bit;
extern int p2_last_switch_time;
extern MovesSwitchLogEntry p1_switch_log[30];
extern MovesSwitchLogEntry p2_switch_log[30];
extern float aniproc_land(void);

typedef float (*MovesEntryFn)(void);
typedef struct MovesProcVtable MovesProcVtable;
typedef void (*MovesSleepFn)(MovesEntryFn, MovesProcVtable*, float);
struct MovesProcVtable {
    void* functions[9];
    MovesSleepFn sleep;
};

typedef struct MovesYieldVtable {
    void* functions[6];
    void (*yield)(struct MovesYieldVtable*);
} MovesYieldVtable;

void bgnd_swap_level(int level);
void bgnd_move_plyrs_to_initial_pos(void);

float p_plyr_sidekick_projectile(void);
float p_plyr_sidekick_intro(void);
float p_plyr_smoke_entrance(void);
float p_plyr_noob_entrance(void);
float p_plyr_sidekick_charge(void);
float p_plyr_sidekick_switch(void);
float p_sidekick_exit_now(void);
void trial_increment_state_value(int player, int state, int amount);
void avoid_double_ani(void);
void init_ground_move_no_aniproc(void);
void face_opponent_now(void);
void ani_loop_more_frames(float frames);
void my_pad_position(void);
void blend_to_ani(AniData* animation, int transition, float rate);
void blend_to_stance(float rate);
void blend_to_fstance(float rate);
void rotate_towards_him(float rate);
int is_my_chest_to_screen();
void back_rollup_left(void);
void back_rollup_right(void);
void front_rollup_left(void);
void front_rollup_right(void);
void init_3d_move(void);
void rollup_finish(void);
float j_exit(void);
float j_exit_blend_stance(void);
float start_suicide(void);
float start_fatality(void);
float start_2nd_fatality(void);
float p_hide_and_die(void);
float x_block(void);
float x_attack_1(void);
float x_attack_2(void);
float x_attack_5(void);
int is_pX_airborn(int player_number);
int is_plyr_airborn(MkObj* object, PlyrPdata* player);
int trial_block_callback(int player);
void snd_req(int sound_id);
void advance_active_moveset(PlyrPdata* player);
void tightrope_restrictions_on(void);
void set_my_state(int state);
void ani_x_more_frames(float frames);
void random_voice(int group);
void ani_to_blend_frame(float frames);
void set_anim_script(AnimPdata* anim, AniData* animation, int transition);
int do_i_have_life_left(void);
void stop_me(void);
void update_bone_hierarchy(MkHdr* object);
void ground_me(MkHdr* object);
void player_feet_land_chores(void);
void ani_to_frame_x(float frame);
void ani_to_end(void);
void ani_1_frame(int update);
void got_hit_fx(int type, int sound_group, int blood, int arg3, int arg4,
                int arg5, float rate);
int trial_show_standard_fight_messages(void);
int drone_ai_should_roll(int mode);
void play_sound_1(int sound_id);
void reaction_xfer_him_nohit(int reaction);
void disable_this_move_exec(unsigned int move, int ticks);
void glitch_to_ani(AniData* animation, int transition);
int drone_ai_get_min_time_in_block(void);
int get_his_attack_counter(void);
int trial_change_style_callback(int player);
void start_gore2_update(void);
void set_attackers_attack_region(int region);
void attack_to_frame_x(unsigned int animation, unsigned int voice_event,
                       unsigned int whoosh_event, int transition, float frame,
                       float blend_rate, float step, float weight);
void ani_to_frame_x_col(int region, int reaction, unsigned int collision_ticks,
                        float frame, float x, float y, float z);
void clear_collision_result(void);
int whoosh_fx(int sound);
float p_sc_spear_retract(void);
float p_sc_spear_kill(void);
float p_anim_idle(void);
float p_blend_to_stance_in_10(void);
float p_blend_to_fstance_in_10(void);
float j_blend_to_fstance_in_x(void);
float j_stay_down_dead(void);
float trial_run_loser_animation_script(void);
float j_block_loop(void);
float x_advance_fatality(void);
void x_advance_moveset(void);
float blend_to_duck_block(void);
float block_a_intro(void);
void block_a_intro_glitch(void);
void set_ani_weight(float weight);
void blend_to_ani_nosleep(AniData* animation, int transition, float blend_rate);
int am_i_blocking(void);
int should_i_weapon_block(void);
int drone_ai_check_block_fakeout(void);
void random_hit(int group);
float p_animate(void);
int am_i_duck_blocking(void);
int am_i_a_big_character(void);
int is_he_airborn(void);
float xz_distance_between_players(void);
void init_ground_move(void);
void nudge_towards_him(float max_step);
void trial_clear_provision(void);
void dead_liukang_snd_chain_check(
    PlyrPdata* player, int sound_chain, int minimum, int maximum);
void transition_to_anim_script(
    AnimPdata* anim, AniData* animation, int transition, float blend_rate);
void sidekick_cool_vanish(PlyrPdata* player);
void init_air_move_no_aniproc(void);
void set_jump_towards_velocities(void);
void jump_towards_opponent(void);
void do_pickup(MovesPickup* pickup, Vec* offset, int take);
float j_flying_kick1_early(void);
float j_flying_kick2_early(void);
float j_flying_punch_early(void);
int does_he_have_life_left(void);
void random_dk_foot(void);
unsigned short randu0(unsigned int maximum);
void camera_idle(void);
void xfer_camera(MkProcEntryFn entry, int transition);
void set_ani_speed(float speed);
void clear_both_face_opponent_flags(void);
int is_big_boss(PlyrPdata* player);
void plyr_weapon_hide(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* mirror_slots);
float r_call_player_char_script_function(void);
CmdScript* get_cmdscript_for_proc(MkProc* proc);
void tag_team_activate_player(MkObj* sidekick, int active);
void set_root_and_obj_movement_weights(
    AnimPdata* animation, float root_weight, float object_weight);
void xfer_player_proc(MkProc* proc, MkProcEntryFn entry);
float r_call_script_function(void);
int am_i_on_the_left2(MkObj* player, MkObj* opponent);
int plyr_start_script_in_plyr_pdata_proc(
    PlyrPdata* player, int pid, int function);
void obj_set_gravity(MkObj* object, float gravity);
void shake_camera(int strength, float duration);
void set_constrain_last_pos_pdata(Vec* position);
void clear_my_face_opponent_flag(void);
void bgnd_clear_danger_zone_callback(PlyrPdata* player);
float sqrtf(float value);

void kobra_teleport_position(void) {
    float delta_z;
    float delta_x;
    float distance_squared;
    float inverse_distance;

    inverse_distance = 0.0f;
    delta_z = plyr_obj->pos.z - his_obj->pos.z;
    delta_x = plyr_obj->pos.x - his_obj->pos.x;
    distance_squared = delta_z * delta_z + delta_x * delta_x;
    if (distance_squared > 0.0f) {
        inverse_distance = 1.0f / sqrtf(distance_squared);
    }

    plyr_obj->pos.x =
        his_obj->pos.x + delta_x * inverse_distance * -2.0f;
    plyr_obj->pos.z =
        his_obj->pos.z + delta_z * inverse_distance * -2.0f;
    if (((MovesGameStateView*)&g_game_info)->state != 2) {
        set_constrain_last_pos_pdata(&his_obj->pos);
    }
    clear_my_face_opponent_flag();
    bgnd_clear_danger_zone_callback(plyr_pdata);
}

static void moves_jump(MovesEntryFn entry) {
    MovesProcVtable* vtable;

    vtable = (MovesProcVtable*)aproc->vtbl;
    vtable->sleep(entry, vtable, 0.0f);
}

static void moves_sleep(float ticks) {
    MovesYieldVtable* vtable;

    _mkproc_sleep_ticks = ticks;
    vtable = (MovesYieldVtable*)aproc->vtbl;
    vtable->yield(vtable);
}

/*
 * Near miss: p_watch_weapon is semantically complete. Remaining differences
 * are float-pool labels and MWCC's equivalent valid-latch branch layout.
 */
float p_watch_weapon(void) {
    PlyrPdata* player;
    MovesWeaponWatchPdata* pdata;

    pdata = (MovesWeaponWatchPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    pdata->timeout--;
    if (pdata->timeout < 0) {
        moves_jump(p_hide_and_die);
        return 0.0f;
    }
    player = pdata->player;
    if (player != 0) {
        if (player->instance == pdata->player_instance) {
            /* The player latch is still live. */
        } else {
            player = 0;
        }
    } else {
        player = 0;
    }
    if (player != 0 &&
        pdata->transient_proc_instance != player->transient_proc_instance) {
        moves_jump(p_hide_and_die);
        return 0.0f;
    }
    return 1.0f;
}

/*
 * Soft ceiling: the three local fatality dispatchers are opcode-identical to
 * retail; objdiff only distinguishes their TU-local zero-float pool labels.
 */
float do_my_suicide(void) {
    if (f_fatality_available != 0 ||
        g_game_info.feature_flags.bits.high_bit != 0) {
        f_fatality_was_done = 1;
        moves_jump(start_suicide);
        return 0.0f;
    }
    moves_jump(j_exit);
    return 0.0f;
}

float do_my_2nd_fatality(void) {
    if (((f_fatality_available != 0 && his_pdata->state == 0x4203) ||
         g_game_info.feature_flags.bits.high_bit != 0) &&
        (plyr_pdata->character_id != 0x1B ||
         plyr_pdata->sidekick_active != 0)) {
        f_fatality_was_done = 1;
        moves_jump(start_2nd_fatality);
        return 0.0f;
    }
    moves_jump(j_exit);
    return 0.0f;
}

float do_my_fatality(void) {
    if (((f_fatality_available != 0 && his_pdata->state == 0x4203) ||
         g_game_info.feature_flags.bits.high_bit != 0) &&
        (plyr_pdata->character_id != 0x1B ||
         plyr_pdata->sidekick_active == 0)) {
        f_fatality_was_done = 1;
        moves_jump(start_fatality);
        return 0.0f;
    }
    moves_jump(j_exit);
    return 0.0f;
}

void x_pickup(void) {
    MovesGameInfoView* game;
    MovesPickupTransform* transform;
    MovesPickup* pickup;
    MovesPickup* nearby_pickup;
    MkPtr* link;
    MkPtr* next;
    Vec offset;
    float vertical_distance;
    int found;

    nearby_pickup = 0;
    found = 0;
    if ((plyr_pdata->state & 0x200) == 0 &&
        is_plyr_airborn(plyr_obj, plyr_pdata) != 1) {
        game = (MovesGameInfoView*)&g_game_info;
        link = game->pickup_list;
        while (link != 0) {
            pickup = (MovesPickup*)link->hdr;
            if (link->instance != pickup->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }

            transform = pickup->transform_a;
            if (transform == 0) {
                transform = pickup->transform_b;
            }
            offset.x = plyr_obj->pos.x - transform->position.x;
            offset.y = plyr_obj->pos.y - transform->position.y;
            offset.z = plyr_obj->pos.z - transform->position.z;
            if (offset.x * offset.x + offset.z * offset.z < 2.9f) {
                vertical_distance = offset.y;
                if (vertical_distance < 0.0f) {
                    vertical_distance = -vertical_distance;
                }
                if (vertical_distance < 1.5f) {
                    offset.y = 0.0f;
                    nearby_pickup = pickup;
                    found = 1;
                    break;
                }
            }
            link = link->next;
        }
    }

    if (found == 1) {
        do_pickup(nearby_pickup, &offset, 1);
    }
    moves_jump(j_exit);
}

/*
 * Soft ceiling: p_block's executable body is opcode-identical to retail.
 * MWCC selects scalar r30/r31 saves here instead of retail's stmw/lmw pair.
 */
float p_block(void) {
    PlyrInfo* player;
    int player_number;
    PlyrPdata* player_data;
    int state;

    player = switch_pdata->player;
    if (player == 0) {
        return -1.0f;
    }
    player_data = player->slot.pdata;
    player_number = player_data->plyr_num;
    if ((int)mode_of_play == 8 &&
        trial_block_callback(player_number) == 0) {
        return -1.0f;
    }
    if ((player_data->state & 0x200) == 0 &&
        is_pX_airborn(player_number) == 0) {
        player = switch_pdata->player;
        if (player != 0) {
            if (player->slot.pdata->state == 0x6000) {
                mkproc_die();
            }
            if (player->player_state != 2 && player->player_state != 3) {
                mkproc_die();
            }
            state = player->slot.pdata->state;
            if ((state & 0x200) != 0 && state != 0x420D) {
                mkproc_die();
            }
            if ((player->slot.pdata->state & 0x800) != 0) {
                mkproc_die();
            }
            if ((unsigned int)player->slot.pdata->attacks_disabled_until >
                (unsigned int)game_tick_ctr) {
                mkproc_die();
            }
            if (player->field_0C == 0.0f) {
                mkproc_die();
            }
            xfer_proc((MkProc*)player->idle_proc, x_block);
        }
    }
    return -1.0f;
}

/*
 * Soft ceiling: switch_proc_attack_5/2/1 are opcode-identical to retail;
 * objdiff only distinguishes their TU-local zero/-one float-pool labels.
 */
float switch_proc_attack_5(void) {
    PlyrInfo* player;
    int state;

    player = switch_pdata->player;
    if (player != 0) {
        if (player->slot.pdata->state == 0x6000) {
            mkproc_die();
        }
        if (player->player_state != 2 && player->player_state != 3) {
            mkproc_die();
        }
        state = player->slot.pdata->state;
        if ((state & 0x200) != 0 && state != 0x420D) {
            mkproc_die();
        }
        if ((player->slot.pdata->state & 0x800) != 0) {
            mkproc_die();
        }
        if ((unsigned int)player->slot.pdata->attacks_disabled_until >
            (unsigned int)game_tick_ctr) {
            mkproc_die();
        }
        if (player->field_0C == 0.0f) {
            mkproc_die();
        }
        xfer_proc((MkProc*)player->idle_proc, x_attack_5);
    }
    return -1.0f;
}

float switch_proc_attack_2(void) {
    PlyrInfo* player;
    int state;

    player = switch_pdata->player;
    if (player != 0) {
        if (player->slot.pdata->state == 0x6000) {
            player->slot.pdata->state = 0x6003;
            mkproc_die();
        }
        if (player->player_state != 2 && player->player_state != 3) {
            mkproc_die();
        }
        state = player->slot.pdata->state;
        if ((state & 0x200) != 0 && state != 0x420D) {
            mkproc_die();
        }
        if ((player->slot.pdata->state & 0x800) != 0) {
            mkproc_die();
        }
        if ((unsigned int)player->slot.pdata->attacks_disabled_until >
            (unsigned int)game_tick_ctr) {
            mkproc_die();
        }
        if (player->field_0C == 0.0f) {
            mkproc_die();
        }
        xfer_proc((MkProc*)player->idle_proc, x_attack_2);
    }
    return -1.0f;
}

float switch_proc_attack_1(void) {
    PlyrInfo* player;
    int state;

    player = switch_pdata->player;
    if (player != 0) {
        if (player->slot.pdata->state == 0x6000) {
            player->slot.pdata->state = 0x6003;
            mkproc_die();
        }
        if (player->player_state != 2 && player->player_state != 3) {
            mkproc_die();
        }
        state = player->slot.pdata->state;
        if ((state & 0x200) != 0 && state != 0x420D) {
            mkproc_die();
        }
        if ((player->slot.pdata->state & 0x800) != 0) {
            mkproc_die();
        }
        if ((unsigned int)player->slot.pdata->attacks_disabled_until >
            (unsigned int)game_tick_ctr) {
            mkproc_die();
        }
        if (player->field_0C == 0.0f) {
            mkproc_die();
        }
        xfer_proc((MkProc*)player->idle_proc, x_attack_1);
    }
    return -1.0f;
}

void switch_proc_advance_moveset(void) {
    PlyrInfo* player;
    PlyrPdata* player_data;
    MkProc* idle_proc;
    MkProc* proc;
    MovesSidekickPdataRef pdata;
    int state;

    player = switch_pdata->player;
    if (player == 0) {
        return;
    }
    player_data = player->slot.pdata;
    idle_proc = (MkProc*)player->idle_proc;
    if ((int)mode_of_play == 8 &&
        trial_change_style_callback(player->controller_slot) == 0) {
        return;
    }
    if ((player->player_state != 2 && player->player_state != 3) ||
        player->field_0C <= 0.0f) {
        return;
    }

    state = player_data->state;
    if ((state & 0x1800) != 0 || player_data->state_flags.bits.frozen) {
        return;
    }
    if (f_fatality_available != 0 && state != 0x420D) {
        if ((state & 0x200) == 0 && is_pX_airborn(player->controller_slot) == 0) {
            xfer_proc(idle_proc, x_advance_fatality);
            snd_req(0xDC5);
        }
        ((MovesMoveDataView*)player_data)->move_advance_latch = 0x1E0;
        return;
    }

    if ((state & 0x200) == 0 && is_pX_airborn(player->controller_slot) == 0) {
        xfer_proc(idle_proc, (MkProcEntryFn)x_advance_moveset);
    }
    if (player_data->state == 0x420D) {
        xfer_proc(idle_proc, (MkProcEntryFn)x_advance_moveset);
    }
    if (player_data->sidekick_available == 0) {
        advance_active_moveset(player_data);
        snd_req(0xDC1);
        return;
    }

    if (player_data->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_switch,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_switch,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = player_data;
    }
}

void set_grab_anim_weighting(const Vec* offset, unsigned int grab_type) {
    union {
        float f;
        unsigned int u;
    } length_bits, guess_bits;
    float length_sq;
    float length;
    float weight;

    length_sq =
        offset->z * offset->z +
        (offset->x * offset->x + offset->y * offset->y);
    length = 0.0f;
    if (length_sq > 0.0f) {
        length_bits.f = length_sq;
        guess_bits.u =
            (unsigned int)GXMathSqrtTable[(length_bits.u >> 10) & 0x3FFE] << 8;
        guess_bits.u |=
            (((length_bits.u & 0x7F800000U) + 0x3F800000U) >> 1) &
            0x7F800000U;
        length =
            0.5f * (guess_bits.f *
                    (3.0f - (guess_bits.f * guess_bits.f) / length_sq));
    }

    if (grab_type == 3) {
        if (length < 0.85f) {
            weight = 0.15f;
        } else if (length < 1.0f) {
            weight = 0.35f * ((length - 0.75f) / 0.2f) + 0.15f;
        } else if (length < 1.25f) {
            weight = 0.15f * ((length - 1.0f) / 0.3f) + 0.35f;
        } else if (length < 1.4f) {
            weight = 0.8f;
        } else {
            weight = 0.9f;
        }
    } else if (grab_type == 8) {
        if (length < 1.1f) {
            weight = 0.1f;
        } else if (length < 1.4f) {
            weight = 0.35f;
        } else {
            weight = 0.45f;
        }
    } else if (length < 1.0f) {
        weight = 0.15f;
    } else {
        weight = 0.35f;
    }
    set_ani_weight(weight);
}

void sidekick_switch_style_swap(unsigned int count) {
    while (count != 0) {
        ani_loop_more_frames(1.0f);
        face_opponent_now();
        count--;
    }
}

void j_back_rollup_IN(void) {
    if (is_my_chest_to_screen() != 0) {
        moves_jump((MovesEntryFn)back_rollup_left);
        return;
    }
    moves_jump((MovesEntryFn)back_rollup_right);
}

void j_back_rollup_OUT(void) {
    if (is_my_chest_to_screen() != 0) {
        moves_jump((MovesEntryFn)back_rollup_right);
        return;
    }
    moves_jump((MovesEntryFn)back_rollup_left);
}

void j_front_roll_left(void) {
    if (is_my_chest_to_screen() != 0) {
        moves_jump((MovesEntryFn)front_rollup_left);
        return;
    }
    moves_jump((MovesEntryFn)front_rollup_right);
}

void j_front_roll_right(void) {
    if (is_my_chest_to_screen() != 0) {
        moves_jump((MovesEntryFn)front_rollup_right);
        return;
    }
    moves_jump((MovesEntryFn)front_rollup_left);
}

void front_rollup_left(void) {
    init_3d_move();
    blend_to_ani(shared_ani.front_roll_left, 3, 0.2f);
    moves_jump((MovesEntryFn)rollup_finish);
}

void front_rollup_right(void) {
    init_3d_move();
    blend_to_ani(shared_ani.front_roll_right, 3, 0.2f);
    moves_jump((MovesEntryFn)rollup_finish);
}

void back_rollup_left(void) {
    init_3d_move();
    blend_to_ani(shared_ani.back_roll_left, 3, 0.2f);
    moves_jump((MovesEntryFn)rollup_finish);
}

void back_rollup_right(void) {
    init_3d_move();
    blend_to_ani(shared_ani.back_roll_right, 3, 0.2f);
    moves_jump((MovesEntryFn)rollup_finish);
}

int noobsmoke_fire_projectile_request(void) {
    unsigned char flags = plyr_pdata->state_flags.raw;
    int requested = flags & 1;

    plyr_pdata->state_flags.bits.projectile_request = 0;
    return requested;
}

void do_my_suicide_remote(void) {
    f_fatality_was_done = 1;
    moves_jump(start_suicide);
}

void do_my_fatality_remote(void) {
    f_fatality_was_done = 1;
    moves_jump(start_fatality);
}

void do_my_2nd_fatality_remote(void) {
    f_fatality_was_done = 1;
    moves_jump(start_2nd_fatality);
}

float blend_to_stance_j_exit(void) {
    blend_to_stance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

float blend_to_fstance_j_exit(void) {
    blend_to_fstance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

float rotate_toward_j_exit(void) {
    rotate_towards_him(0.2f);
    moves_jump(j_exit);
    return 0.0f;
}

float jump_towards_opponent_j_exit(void) {
    jump_towards_opponent();
    moves_jump(j_exit_blend_stance);
    return 0.0f;
}

void sidekick_intro_check(void) {
    MkProc* proc;
    MovesSidekickPdataRef pdata;
    PlyrPdata* player;

    player = g_game_info.plyr0.slot.pdata;
    if (player->sidekick_available != 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_intro,
            sizeof(MovesSidekickPdata), &pdata.hdr);
        if (proc != 0 && pdata.hdr != 0) {
            pdata.sidekick->player = player;
        }
    }

    player = g_game_info.plyr1.slot.pdata;
    if (player->sidekick_available != 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_intro,
            sizeof(MovesSidekickPdata), &pdata.hdr);
        if (proc != 0 && pdata.hdr != 0) {
            pdata.sidekick->player = player;
        }
    }
}

void noobsmoke_sidekick_projectile(void) {
    MkProc* proc;
    MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_projectile,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_projectile,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

void smoke_victory_entrance(void) {
    MkProc* proc;
    MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_smoke_entrance,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_smoke_entrance,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

void noob_victory_entrance(void) {
    MkProc* proc;
    MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_noob_entrance,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_noob_entrance,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

void noobsmoke_sidekick_double_charge(void) {
    MkProc* proc;
    MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_charge,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_charge,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

void advance_sidekick_with_moveset(PlyrPdata* player) {
    MkProc* proc;
    MovesSidekickPdataRef pdata;

    if (player->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_switch,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_switch,
            sizeof(MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = player;
    }
}

float p_sidekick_watchdog_launcher(void) {
    MovesSidekickPdata* pdata;
    PlyrPdata* player;
    MkObj* sidekick;

    pdata = (MovesSidekickPdata*)apdata;
    player = pdata->player;
    moves_sleep(300.0f);

    sidekick = player->sidekick_obj;
    if (sidekick != 0 && sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    if (sidekick != 0 && !sidekick->hide_flag_bits.hidden) {
        hide_obj(sidekick);
    }
    return -1.0f;
}

float p_sidekick_exit_now(void) {
    MovesSidekickPdata* pdata;
    MovesSidekickStateView* player;
    MovesSidekickFighterDefinition* fighter;
    MkObj* sidekick;
    MkProc* anim_proc;
    AnimPdata* anim;

    pdata = (MovesSidekickPdata*)apdata;
    player = (MovesSidekickStateView*)pdata->player;

    sidekick = player->sidekick_obj;
    if (sidekick != 0 &&
        sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != player->sidekick_anim_proc_instance) {
        anim_proc = 0;
    }

    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    fighter =
        (MovesSidekickFighterDefinition*)pdata->player->fighter_definition;
    transition_to_anim_script(anim, fighter->exit_animation, 0, 0.2f);
    anim->step = 1.2f * fighter->blend_data->exit_step;
    sidekick->flags_09_bits.bit6 = 1;
    moves_sleep(45.0f);
    sidekick_cool_vanish(pdata->player);
    return 0.0f;
}

float p_plyr_sidekick_switch(void) {
    union {
        float f;
        unsigned int u;
    } inverse_bits;
    MovesSidekickPdata* pdata;
    MovesSidekickSwitchState* player;
    MovesSidekickFighterDefinition* fighter;
    MovesSidekickPdataRef watchdog_pdata;
    MkProc* player_proc;
    MkProc* anim_proc;
    MkObj* sidekick;
    MkObj* main_object;
    AnimPdata* sidekick_anim;
    CmdScript* script;
    Vec direction;
    Vec main_angle;
    float length_sq;
    float inverse_length;
    float estimate_product;
    float correction;
    float object_weight;
    int state;
    MkProc* proc;

    pdata = (MovesSidekickPdata*)apdata;
    player = (MovesSidekickSwitchState*)pdata->player;
    sidekick = player->sidekick_obj;
    object_weight = 2.0f;
    if (sidekick != 0 &&
        sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != player->sidekick_anim_proc_instance) {
        anim_proc = 0;
    }
    sidekick_anim = (AnimPdata*)pdata_of_proc(anim_proc);

    state = pdata->player->state;
    if (state != 0 && state != 0x2000 && state != 0x2001) {
        return 0.0f;
    }
    if ((g_game_info.flags & 0x18) != 0) {
        return 0.0f;
    }

    player_proc = player->player_proc;
    if (player_proc != 0 &&
        player_proc->instance != player->player_proc_instance) {
        player_proc = 0;
    }
    script = get_cmdscript_for_proc(player_proc);
    main_object = player->player_info->slot.mirror_a;
    tag_team_activate_player(
        sidekick, player->player_info->slot.pdata->sidekick_active);

    direction.x = main_object->pos.x - player->opponent_obj->pos.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.z - player->opponent_obj->pos.z;
    main_angle = main_object->ang;
    if (is_plyr_airborn(player->opponent_obj, player->opponent) != 0) {
        object_weight = 3.0f;
    }

    length_sq =
        direction.x * direction.x + direction.z * direction.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse_bits.f = length_sq;
        inverse_bits.u = 0x5F375A00U - (inverse_bits.u >> 1);
        estimate_product =
            inverse_bits.f * (length_sq * inverse_bits.f);
        correction = 3.0f - estimate_product;
        inverse_length =
            0.0625f * inverse_bits.f * correction *
            -((correction * (estimate_product * correction)) - 12.0f);
    }

    set_root_and_obj_movement_weights(
        sidekick_anim, 0.0f, object_weight);
    sidekick->pos.x =
        main_object->pos.x + direction.x * inverse_length * 2.5f;
    sidekick->pos.y = main_object->pos.y;
    sidekick->pos.z =
        main_object->pos.z + direction.z * inverse_length * 2.5f;
    sidekick->ang = main_angle;
    update_mkobj(sidekick);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(as_mkhdr(&sidekick->hdr));
    ground_me(as_mkhdr(&sidekick->hdr));

    if (pdata->player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    fighter =
        (MovesSidekickFighterDefinition*)pdata->player->fighter_definition;
    set_anim_script(sidekick_anim, fighter->switch_animation, 0);
    moves_sleep(1.0f);
    unhide_obj(sidekick);
    sidekick_anim->step = 1.25f;

    if (pdata->player->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_sidekick_watchdog_launcher,
            sizeof(MovesSidekickPdata), &watchdog_pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_sidekick_watchdog_launcher,
            sizeof(MovesSidekickPdata), &watchdog_pdata.hdr);
    }
    if (proc != 0 && watchdog_pdata.hdr != 0) {
        watchdog_pdata.sidekick->player = pdata->player;
    }

    if ((g_game_info.flags & 0x18) == 0) {
        script->unk28 = 0x7B;
        xfer_player_proc(player_proc, r_call_script_function);
    }
    return 0.0f;
}

float p_plyr_sidekick_projectile(void) {
    union {
        float f;
        unsigned int u;
    } bits, guess;
    union {
        float f;
        unsigned int u;
    } inverse_bits;
    MovesSidekickPdata* pdata;
    MovesSidekickSwitchState* player;
    MovesSidekickActionView* actions;
    MkProc* anim_proc;
    MkProc* player_proc;
    MkObj* sidekick;
    MkObj* main_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec direction;
    Vec main_angle;
    float length_sq;
    float distance;
    float inverse_length;
    float estimate_product;
    float correction;
    float placement_scale;
    float object_weight;
    float wrapped_angle;
    int transition;
    int function;

    pdata = (MovesSidekickPdata*)apdata;
    player = (MovesSidekickSwitchState*)pdata->player;
    actions = (MovesSidekickActionView*)pdata->player;
    sidekick = player->sidekick_obj;
    object_weight = 2.0f;
    transition = 0;
    if (sidekick != 0 &&
        sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != player->sidekick_anim_proc_instance) {
        anim_proc = 0;
    }
    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    main_object = player->player_info->slot.mirror_a;
    opponent_object = player->opponent_obj;

    direction.x = main_object->pos.x - opponent_object->pos.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.z - opponent_object->pos.z;
    length_sq =
        direction.x * direction.x + direction.z * direction.z;
    distance = 0.0f;
    if (length_sq > 0.0f) {
        bits.f = length_sq;
        guess.u =
            (unsigned int)GXMathSqrtTable[(bits.u >> 10) & 0x3FFE] << 8;
        guess.u |=
            (((bits.u & 0x7F800000U) + 0x3F800000U) >> 1) &
            0x7F800000U;
        distance =
            0.5f * guess.f *
            (3.0f - (guess.f * guess.f) / length_sq);
    }
    if (distance < 3.2f) {
        object_weight = 0.75f;
    }
    anim->flags &= ~8U;
    sidekick->hide_flag_bits.bit6 = 0;
    if (am_i_on_the_left2(
            main_object,
            player->opponent->plyr_info->slot.mirror_a) == 0) {
        transition = 8;
    }

    player_proc = player->player_proc;
    if (player_proc != 0 &&
        player_proc->instance != player->player_proc_instance) {
        player_proc = 0;
    }
    get_cmdscript_for_proc(player_proc);
    tag_team_activate_player(
        sidekick, player->player_info->slot.pdata->sidekick_active);

    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse_bits.f = length_sq;
        inverse_bits.u = 0x5F375A00U - (inverse_bits.u >> 1);
        estimate_product =
            inverse_bits.f * (length_sq * inverse_bits.f);
        correction = 3.0f - estimate_product;
        inverse_length =
            0.0625f * inverse_bits.f * correction *
            -((correction * (estimate_product * correction)) - 12.0f);
    }
    direction.x *= inverse_length;
    direction.z *= inverse_length;
    placement_scale = distance < 3.2f ? 1.2f : 1.5f;
    main_angle = main_object->ang;
    sidekick->ground_colls_y = main_object->ground_colls_y;
    set_root_and_obj_movement_weights(anim, 0.0f, object_weight);
    sidekick->pos.x =
        main_object->pos.x + direction.x * placement_scale;
    sidekick->pos.y = g_game_info.field_34;
    sidekick->pos.z =
        main_object->pos.z + direction.z * placement_scale;
    sidekick->ang = main_angle;
    update_mkobj(sidekick);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(as_mkhdr(&sidekick->hdr));
    ground_me(as_mkhdr(&sidekick->hdr));

    destroy_mkprocs_pid(
        pdata->player->plyr_num == 0 ? 0xC028 : 0xC029);
    set_anim_script(anim, actions->projectile_animation, transition);
    anim->step = 1.25f;
    moves_sleep(1.0f);
    unhide_obj(sidekick);

    wrapped_angle =
        0.000005992112f *
        (float)(((int)(166886.1f * main_object->ang.y)) & 0xFFFFF);
    gxMathSin(wrapped_angle);
    gxMathCos(wrapped_angle);
    moves_sleep(8.0f);
    anim->step = 0.75f;
    random_hit(8);
    moves_sleep(5.0f);
    pdata->player->state_flags.raw |= 1;
    moves_sleep(1.0f);
    pdata->player->state_flags.raw |= 1;
    moves_sleep(1.0f);
    pdata->player->state_flags.raw |= 1;
    moves_sleep(1.0f);
    pdata->player->state_flags.raw |= 1;
    moves_sleep(12.0f);

    set_root_and_obj_movement_weights(anim, 0.0f, 0.5f);
    transition_to_anim_script(
        anim, actions->common_exit_animation, transition | 3, 0.1f);
    anim->step = 2.0f;
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.head_tracking = 0;
    while (anim->frame < 5.0f) {
        moves_sleep(1.0f);
    }
    sidekick->flags_09_bits.bit6 = 0;
    sidekick->flags_09_bits.launched = 0;
    moves_sleep(25.0f);

    function =
        get_script_function_by_name(actions->cmo, "start_noob_exit_pfx");
    plyr_start_script_in_plyr_pdata_proc(
        pdata->player, 0xC025, function);
    snd_req(0x33B);
    moves_sleep(5.0f);
    hide_obj(sidekick);
    return -1.0f;
}

float p_plyr_noob_entrance(void) {
    union {
        float f;
        unsigned int u;
    } inverse_bits;
    MovesSidekickPdata* pdata;
    MovesSidekickSwitchState* player;
    MovesSidekickActionView* actions;
    MkProc* anim_proc;
    MkProc* player_proc;
    MkObj* sidekick;
    MkObj* main_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec direction;
    Vec lateral;
    Vec offset;
    Vec main_angle;
    float length_sq;
    float inverse_length;
    float estimate_product;
    float correction;
    float lateral_scale;
    int transition;

    pdata = (MovesSidekickPdata*)apdata;
    player = (MovesSidekickSwitchState*)pdata->player;
    actions = (MovesSidekickActionView*)pdata->player;
    sidekick = player->sidekick_obj;
    if (sidekick != 0 &&
        sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != player->sidekick_anim_proc_instance) {
        anim_proc = 0;
    }
    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    anim->flags &= ~8U;
    sidekick->hide_flag_bits.bit6 = 0;

    player_proc = player->player_proc;
    if (player_proc != 0 &&
        player_proc->instance != player->player_proc_instance) {
        player_proc = 0;
    }
    get_cmdscript_for_proc(player_proc);
    tag_team_activate_player(
        sidekick, player->player_info->slot.pdata->sidekick_active);
    main_object = player->player_info->slot.mirror_a;
    opponent_object = player->opponent_obj;
    direction.x = main_object->pos.x - opponent_object->pos.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.z - opponent_object->pos.z;
    main_angle = main_object->ang;

    length_sq =
        direction.x * direction.x + direction.z * direction.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse_bits.f = length_sq;
        inverse_bits.u = 0x5F375A00U - (inverse_bits.u >> 1);
        estimate_product =
            inverse_bits.f * (length_sq * inverse_bits.f);
        correction = 3.0f - estimate_product;
        inverse_length =
            0.0625f * inverse_bits.f * correction *
            -((correction * (estimate_product * correction)) - 12.0f);
    }
    direction.x *= inverse_length;
    direction.z *= inverse_length;
    offset.x = -0.2f * direction.x;
    offset.y = 0.0f;
    offset.z = -0.2f * direction.z;
    lateral.x = direction.z;
    lateral.y = 0.0f;
    lateral.z = -direction.x;
    if (am_i_on_the_left2(
            main_object,
            player->opponent->plyr_info->slot.mirror_a) != 0) {
        lateral_scale = -0.7f;
        transition = 0;
    } else {
        lateral_scale = 0.7f;
        transition = 8;
    }
    offset.x += lateral.x * lateral_scale;
    offset.z += lateral.z * lateral_scale;

    sidekick->flags_09_bits.bit6 = 0;
    sidekick->flags_09_bits.launched = 0;
    sidekick->ground_colls_y = main_object->ground_colls_y;
    set_root_and_obj_movement_weights(anim, 0.0f, 1.0f);
    sidekick->pos.x = main_object->pos.x + offset.x;
    sidekick->pos.y = g_game_info.field_34 + 6.535f;
    sidekick->pos.z = main_object->pos.z + offset.z;
    sidekick->ang = main_angle;
    update_mkobj(sidekick);

    destroy_mkprocs_pid(
        pdata->player->plyr_num == 0 ? 0xC028 : 0xC029);
    transition |= 3;
    set_anim_script(anim, actions->noob_entrance_animation, transition);
    moves_sleep(1.0f);
    unhide_obj(sidekick);
    anim->step = 0.6f;
    moves_sleep(4.0f);
    snd_req(0xD7B);
    while (anim->frame < 26.0f) {
        moves_sleep(1.0f);
    }

    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(as_mkhdr(&sidekick->hdr));
    ground_me(as_mkhdr(&sidekick->hdr));
    random_hit(1);
    shake_camera(2, 0.02f);
    sidekick->flags_09_bits.head_tracking = 0;
    transition_to_anim_script(
        anim, actions->common_exit_animation, transition, 0.05f);
    anim->step = 0.6f;
    moves_sleep(10000.0f);
    return -1.0f;
}

float p_plyr_sidekick_charge(void) {
    union {
        float f;
        unsigned int u;
    } inverse_bits;
    MovesSidekickPdata* pdata;
    MovesSidekickSwitchState* player;
    MovesSidekickActionView* actions;
    MkProc* anim_proc;
    MkProc* player_proc;
    MkObj* sidekick;
    MkObj* main_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec direction;
    Vec lateral;
    Vec offset;
    Vec main_angle;
    float length_sq;
    float inverse_length;
    float estimate_product;
    float correction;
    float lateral_scale;
    float wrapped_angle;
    float sine;
    float cosine;
    int transition;
    int function;

    pdata = (MovesSidekickPdata*)apdata;
    player = (MovesSidekickSwitchState*)pdata->player;
    actions = (MovesSidekickActionView*)pdata->player;
    sidekick = player->sidekick_obj;
    if (sidekick != 0 &&
        sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != player->sidekick_anim_proc_instance) {
        anim_proc = 0;
    }
    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    anim->flags &= ~8U;
    sidekick->hide_flag_bits.bit6 = 0;
    sidekick->flags_09_bits.head_tracking = 0;

    player_proc = player->player_proc;
    if (player_proc != 0 &&
        player_proc->instance != player->player_proc_instance) {
        player_proc = 0;
    }
    get_cmdscript_for_proc(player_proc);
    tag_team_activate_player(
        sidekick, player->player_info->slot.pdata->sidekick_active);
    main_object = player->player_info->slot.mirror_a;
    opponent_object = player->opponent_obj;
    direction.x = main_object->pos.x - opponent_object->pos.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.z - opponent_object->pos.z;
    main_angle = main_object->ang;

    length_sq =
        direction.x * direction.x + direction.z * direction.z;
    inverse_length = 0.0f;
    if (length_sq > 0.0f) {
        inverse_bits.f = length_sq;
        inverse_bits.u = 0x5F375A00U - (inverse_bits.u >> 1);
        estimate_product =
            inverse_bits.f * (length_sq * inverse_bits.f);
        correction = 3.0f - estimate_product;
        inverse_length =
            0.0625f * inverse_bits.f * correction *
            -((correction * (estimate_product * correction)) - 12.0f);
    }
    direction.x *= inverse_length;
    direction.z *= inverse_length;
    offset.x = 0.5f * direction.x;
    offset.y = 0.0f;
    offset.z = 0.5f * direction.z;
    lateral.x = direction.z;
    lateral.y = 0.0f;
    lateral.z = -direction.x;
    if (am_i_on_the_left2(
            main_object,
            player->opponent->plyr_info->slot.mirror_a) != 0) {
        lateral_scale = 0.45f;
        transition = 0;
    } else {
        lateral_scale = -0.45f;
        transition = 8;
    }
    offset.x += lateral.x * lateral_scale;
    offset.z += lateral.z * lateral_scale;

    sidekick->ground_colls_y = main_object->ground_colls_y;
    set_root_and_obj_movement_weights(anim, 0.0f, 1.0f);
    sidekick->pos.x = main_object->pos.x + offset.x;
    sidekick->pos.y = g_game_info.field_34;
    sidekick->pos.z = main_object->pos.z + offset.z;
    sidekick->ang = main_angle;
    update_mkobj(sidekick);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(as_mkhdr(&sidekick->hdr));
    ground_me(as_mkhdr(&sidekick->hdr));

    destroy_mkprocs_pid(
        pdata->player->plyr_num == 0 ? 0xC028 : 0xC029);
    set_anim_script(anim, shared_ani.sidekick_charge, transition);
    moves_sleep(1.0f);
    unhide_obj(sidekick);
    anim->step = 1.0f;
    wrapped_angle =
        0.000005992112f *
        (float)(((int)(166886.1f * main_object->ang.y)) & 0xFFFFF);
    sine = gxMathSin(wrapped_angle);
    cosine = gxMathCos(wrapped_angle);
    while (anim->frame < 5.0f) {
        moves_sleep(1.0f);
    }

    sidekick->pos_vel.x = 0.1f * sine;
    sidekick->pos_vel.y = 0.0f;
    sidekick->pos_vel.z = 0.1f * cosine;
    sidekick->flags_08 |= 0x20;
    while (anim->frame < 24.0f) {
        moves_sleep(1.0f);
        sidekick->pos_vel.x *= 0.99f;
        sidekick->pos_vel.y = 0.0f;
        sidekick->pos_vel.z *= 0.99f;
    }
    anim->step = 0.1f;
    moves_sleep(2.0f);
    sidekick->pos_vel.x = 0.0f;
    sidekick->pos_vel.y = 0.0f;
    sidekick->pos_vel.z = 0.0f;
    sidekick->flags_08 &= ~0x20;
    while (anim->frame < 26.0f) {
        moves_sleep(1.0f);
    }

    anim->step = 0.75f;
    transition_to_anim_script(
        anim, actions->charge_exit_animation, transition | 3, 0.25f);
    anim->step = 1.2f;
    sidekick->flags_09_bits.bit6 = 1;
    while (anim->frame < 5.0f) {
        moves_sleep(1.0f);
    }
    sidekick->flags_09_bits.bit6 = 0;
    sidekick->flags_09_bits.launched = 0;
    while (anim->frame < 26.0f) {
        moves_sleep(1.0f);
    }

    snd_req(0x32C);
    obj_set_gravity(sidekick, -0.01f);
    function =
        get_script_function_by_name(actions->cmo, "start_smoke_exit_pfx");
    plyr_start_script_in_plyr_pdata_proc(
        pdata->player, 0xC025, function);
    moves_sleep(30.0f);
    obj_set_gravity(sidekick, 0.0f);
    sidekick->flags_08_bits.moving = 0;
    hide_obj(sidekick);
    return -1.0f;
}

PlyrFighterDefinition* get_active_moveset_from_pdata(PlyrPdata* player) {
    return player->fighter_definition;
}

int is_weapon_style(MovesStyle* style) {
    MovesWeaponStyleData* weapon_data;

    if (style != 0) {
        weapon_data = style->weapon_data;
        if (weapon_data != 0 &&
            (weapon_data->primary_weapon != 0 ||
             weapon_data->secondary_weapon != 0)) {
            return 1;
        }
    }
    return 0;
}

void attack_to_frame_x(unsigned int animation, unsigned int voice_event,
                       unsigned int whoosh_event, int transition, float frame,
                       float blend_rate, float step, float weight) {
    MovesAttackStateView* attack_state;
    float voice_frame;
    float whoosh_frame;

    attack_state = (MovesAttackStateView*)plyr_pdata;
    attack_state->attack_start_tick = game_tick_ctr;
    attack_state->attack_phase = 1;
    clear_collision_result();
    attack_state->shared_attack_until =
        exec_tick_ctr + (int)(0.5f + frame / step);
    attack_state->attack_counter++;
    plyr_anim_pdata->flags |= 0x40;
    if (animation != 0) {
        blend_to_ani((AniData*)animation, transition, blend_rate);
    }
    plyr_anim_pdata->step = step;
    voice_frame = (float)(voice_event >> 16);
    plyr_anim_pdata->weight = weight;
    whoosh_frame = (float)(whoosh_event >> 16);

    if (voice_frame < frame && whoosh_frame < frame) {
        if (voice_frame < whoosh_frame) {
            if (voice_frame > 0.0f) {
                ani_to_frame_x(voice_frame);
            }
            if ((unsigned int)(exec_tick_ctr - attack_state->last_voice_tick) >
                30U) {
                random_voice((unsigned short)voice_event);
                attack_state->last_voice_tick = exec_tick_ctr;
            }
            if (whoosh_frame > 0.0f) {
                ani_to_frame_x(whoosh_frame);
            }
            whoosh_fx((unsigned short)whoosh_event);
        }
        if (voice_frame > whoosh_frame) {
            if (whoosh_frame > 0.0f) {
                ani_to_frame_x(whoosh_frame);
            }
            whoosh_fx((unsigned short)whoosh_event);
            if (voice_frame > 0.0f) {
                ani_to_frame_x(voice_frame);
            }
            if ((unsigned int)(exec_tick_ctr - attack_state->last_voice_tick) >
                30U) {
                random_voice((unsigned short)voice_event);
                attack_state->last_voice_tick = exec_tick_ctr;
            }
        }
        if (voice_frame == whoosh_frame) {
            if (whoosh_frame > 0.0f) {
                ani_to_frame_x(whoosh_frame);
            }
            whoosh_fx((unsigned short)whoosh_event);
            if ((unsigned int)(exec_tick_ctr - attack_state->last_voice_tick) >
                30U) {
                random_voice((unsigned short)voice_event);
                attack_state->last_voice_tick = exec_tick_ctr;
            }
        }
    }
    ani_to_frame_x(frame);
}

void set_block_requirement(int requirement) {
    if (requirement == 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 6, 0);
    } else {
        trial_increment_state_value(plyr_pdata->plyr_num, 7, 0);
    }
    plyr_pdata->block_requirement = requirement;
}

void share_my_attack_info(float duration, float divisor) {
    plyr_pdata->shared_attack_until =
        exec_tick_ctr + (int)(0.5f + duration / divisor);
    plyr_pdata->attack_counter++;
}

void forced_step_forward(void) {
    PlyrMoveBlendData* blend_data;

    avoid_double_ani();
    init_ground_move_no_aniproc();
    face_opponent_now();
    my_pad_position();
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(
        plyr_pdata->fighter_definition->forced_step_animation, 3, 0.33f);
    blend_data = plyr_pdata->fighter_definition->move_blend_data;
    plyr_anim_pdata->step = blend_data->step;
    plyr_anim_pdata->weight = blend_data->weight;
}

int get_victory_flip_flags(void) {
    return victory_proper_flip_flags;
}

void clear_his_f_constrained(void) {
    if (his_pdata != 0) {
        his_pdata->f_constrained = 0;
    }
}

int get_fatality_available_flag(void) {
    return f_fatality_available;
}

float p_swap_levels(void) {
    static unsigned int current_level;
    unsigned int row_count;

    if (mode_of_play == 9 || mode_of_play == 6) {
        return -1.0f;
    }

    row_count = get_row_count_for_table_by_pointer(
        g_game_info.cmdscript, g_game_info.section->misc);
    current_level++;
    if (current_level >= row_count - 1) {
        current_level = 0;
    }

    bgnd_swap_level(current_level);
    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        bgnd_move_plyrs_to_initial_pos();
    }
    g_game_info.active_level = current_level;
    return -1.0f;
}

void advance_my_moveset(void) {
    snd_req(0xDC1);
    if (aproc->pid == 0x1001) {
        advance_active_moveset(g_game_info.plyr0.slot.pdata);
    } else {
        advance_active_moveset(g_game_info.plyr1.slot.pdata);
    }
}

void front_rollup(void) {
    tightrope_restrictions_on();
    blend_to_ani(shared_ani.front_rollup, 3, 0.1f);
    plyr_anim_pdata->step = 1.2f;
    moves_jump(p_blend_to_stance_in_10);
}

void x_advance_moveset(void) {
    MovesMoveDataView* move_data;

    move_data = (MovesMoveDataView*)plyr_pdata;
    set_my_state(0);
    move_data->move_advance_latch = 0;
    blend_to_stance(0.1f);
    plyr_anim_pdata->step = 1.0f;
    moves_jump(j_exit);
}

void j_ass_rollup(void) {
    blend_to_ani(shared_ani.ass_rollup, 3, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    ani_to_blend_frame(10.0f);
    blend_to_fstance(0.05f);
    moves_jump(j_exit);
}

void rollup_finish(void) {
    plyr_anim_pdata->step = 1.0f;
    ani_x_more_frames(15.0f);
    random_voice(9);
    ani_to_blend_frame(15.0f);
    plyr_pdata->summon_position_x = 15.0f;
    moves_jump(j_blend_to_fstance_in_x);
}

void glitch_to_stance_j_exit(void) {
    MovesFighterDefinitionView* fighter;

    fighter = (MovesFighterDefinitionView*)plyr_pdata->fighter_definition;
    set_anim_script(plyr_anim_pdata, fighter->stance, 0x20);
    while (do_i_have_life_left() == 0) {
        moves_sleep(1.0f);
    }
    moves_jump(j_exit);
}

int check_for_dead_movement(void) {
    if (plyr_pdata == 0 || round_winner == 0) {
        return 0;
    }
    if (round_winner == 2 && plyr_pdata->plyr_num == 0 &&
        g_game_info.plyr0.field_0C <= 0.0f) {
        return 1;
    }
    if (round_winner == 1 && plyr_pdata->plyr_num == 1 &&
        g_game_info.plyr1.field_0C <= 0.0f) {
        return 1;
    }
    return 0;
}

void fall_dead(void) {
    MovesDeathDataView* death_data;

    death_data = (MovesDeathDataView*)plyr_pdata;
    init_ground_move_no_aniproc();
    set_my_state(0x4200);
    death_data->death_animation_active = 1;
    plyr_obj->flags_09_bits.head_tracking = 0;
    plyr_obj->flags_09_bits.face_opponent = 0;
    if ((int)mode_of_play == 8 &&
        trial_show_standard_fight_messages() == 0) {
        moves_jump(trial_run_loser_animation_script);
        return;
    }
    blend_to_ani(shared_ani.fall_dead, 3, 0.1f);
    ani_to_frame_x(49.0f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    ani_to_end();
    moves_jump(j_stay_down_dead);
}

void victory(void) {
    MovesFighterDefinitionView* fighter;
    MovesVictoryData* victory_data;
    MovesProcessLatchView* opponent_latch;
    AnimPdata* opponent_anim;
    MkProc* opponent_anim_proc;
    int ticks;
    int opponent_state;

    if ((int)mode_of_play == 8) {
        blend_to_stance(0.1f);
        for (;;) {
            moves_sleep(60.0f);
        }
    }
    if ((g_game_info.flags & 0x0C) != 0) {
        for (;;) {
            moves_sleep(60.0f);
        }
    }

    victory_proper_flip_flags = 0;
    set_my_state(0x4201);
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    plyr_obj->flags_0B |= 0x40;
    plyr_obj->flags_09_bits.bit4 = 0;
    plyr_obj->flags_09_bits.head_tracking = 0;

    fighter = (MovesFighterDefinitionView*)plyr_pdata->fighter_definition;
    ticks = 240;
    if (plyr_anim_pdata->script_word !=
        (unsigned int)fighter->stance) {
        while (plyr_anim_pdata->frame < plyr_anim_pdata->high_frame &&
               plyr_pdata->state == 0x4200 && ticks != 0) {
            moves_sleep(1.0f);
            ticks--;
        }
    }

    plyr_weapon_hide(plyr_pdata, 0, plyr_pdata->mirror_slots);
    victory_data = (MovesVictoryData*)plyr_pdata->status_flags;
    if (victory_data->victory_script == 0) {
        clear_both_face_opponent_flags();
        xfer_proc(plyr_anim_proc, p_animate);
        blend_to_stance(0.1f);
        set_my_state(0x4253);
        for (;;) {
            moves_sleep(60.0f);
        }
    }

    set_ani_speed(1.0f);
    if (f_fatality_finished != 0) {
        face_opponent_now();
    } else {
        blend_to_stance(0.1f);
        moves_sleep(20.0f);
        rotate_towards_him(0.2f);
    }

    opponent_latch =
        (MovesProcessLatchView*)plyr_pdata->his_plyr_pdata;
    opponent_anim_proc = opponent_latch->anim_proc;
    if (opponent_anim_proc != 0 &&
        opponent_anim_proc->instance != opponent_latch->anim_proc_instance) {
        opponent_anim_proc = 0;
    }
    opponent_anim = (AnimPdata*)pdata_of_proc(opponent_anim_proc);
    ticks = 240;
    while (opponent_anim->frame < opponent_anim->high_frame) {
        opponent_state = plyr_pdata->his_plyr_pdata->state;
        if (opponent_state == 0x4200 || opponent_state == 0 ||
            opponent_state == 0x4203 || ticks == 0) {
            break;
        }
        moves_sleep(1.0f);
        ticks--;
    }

    clear_both_face_opponent_flags();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    active_cmdscript->unk28 = victory_data->victory_script;
    if (is_big_boss(plyr_pdata) != 0) {
        moves_jump(r_call_player_char_script_function);
        return;
    }
    if (is_my_chest_to_screen() == 0) {
        if ((plyr_anim_pdata->flags & 8) == 0) {
            victory_proper_flip_flags = 8;
        }
        blend_to_ani(
            shared_ani.victory_turn, victory_proper_flip_flags | 3, 0.1f);
        ani_to_blend_frame(10.0f);
    }
    active_cmdscript->unk28 = victory_data->victory_script;
    moves_jump(r_call_player_char_script_function);
}

void big_boss_end_of_round(void) {
    MovesBossAnimationView* animations;
    int ticks;

    animations = (MovesBossAnimationView*)plyr_pdata;
    if (do_i_have_life_left() != 0 && does_he_have_life_left() != 0) {
        while (does_he_have_life_left() == 0 ||
               (g_game_info.flags & 0x20) == 0) {
            ani_loop_more_frames(1.0f);
        }
    } else {
        init_ground_move_no_aniproc();
        set_my_state(0x4200);
        if (is_my_chest_to_screen() == 0) {
            ticks = 30;
            while (--ticks > 0) {
                force_midpoint_calculation_update = 1;
                ani_1_frame(1);
                moves_sleep(1.0f);
            }
            blend_to_ani(animations->walk_animation, 3, 0.1f);
            set_ani_speed(1.2f);
            ticks = 30;
            while (--ticks > 0) {
                force_midpoint_calculation_update = 1;
                ani_1_frame(1);
                moves_sleep(1.0f);
            }
            ani_to_blend_frame(40.0f);
            random_dk_foot();
            ani_to_blend_frame(10.0f);
            set_ani_speed(1.0f);
            blend_to_fstance(0.05f);
        }

        ticks = 30;
        while (--ticks > 0) {
            force_midpoint_calculation_update = 1;
            ani_1_frame(1);
            moves_sleep(1.0f);
        }
        camera_idle();
        if (randu0(100) < 50) {
            snd_req(0x1B4);
        } else {
            snd_req(0x1B5);
        }
        blend_to_ani(animations->end_round_animation, 3, 0.1f);
        ani_to_blend_frame(20.0f);
        xfer_camera(p_camera_proc, 0);

        if (xz_distance_between_players() < 7.5f) {
            blend_to_ani(animations->walk_animation, 3, 0.1f);
            set_ani_speed(1.2f);
            ticks = 30;
            while (--ticks > 0) {
                force_midpoint_calculation_update = 1;
                ani_1_frame(1);
                moves_sleep(1.0f);
            }
            ani_to_blend_frame(40.0f);
            random_dk_foot();
            ani_to_blend_frame(10.0f);
            set_ani_speed(1.0f);
            blend_to_fstance(0.05f);
        }
        if (do_i_have_life_left() == 0 && (g_game_info.flags & 1) != 0) {
            moves_jump(j_stay_down_dead);
            return;
        }
        blend_to_stance(0.05f);

        while (does_he_have_life_left() == 0 ||
               (g_game_info.flags & 0x20) == 0) {
            ani_loop_more_frames(1.0f);
        }
    }

    plyr_obj->flags_09_bits.tightrope_restricted = 1;
    plyr_obj->flags_0B &= ~0x40;
    if (plyr_pdata->drone_request != 0) {
        moves_jump(drone_start);
    } else {
        moves_jump(j_exit);
    }
}

void jump_towards_opponent(void) {
    MovesAnimPdataView* anim;
    MkHdr* object;
    float flight_ticks;
    float remaining_ticks;
    float high_frame;
    int state;

    rotate_towards_him(0.2f);
    init_air_move_no_aniproc();
    set_my_state(0x6000);
    random_voice(9);
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(shared_ani.jump_towards, 3, 0.1f);
    plyr_anim_pdata->step = 2.0f;
    ani_to_frame_x(11.0f);
    dead_liukang_snd_chain_check(plyr_pdata, 0, 0x1E, 0x50);
    plyr_obj->flags_08_bits.moving = 1;
    set_jump_towards_velocities();

    state = plyr_pdata->state;
    if (state == 0x6002) {
        if (am_i_a_big_character() != 0) {
            moves_jump(j_flying_kick1_early);
        } else {
            moves_jump(j_flying_kick2_early);
        }
        return;
    }
    if (state == 0x6003) {
        moves_jump(j_flying_punch_early);
        return;
    }

    set_my_state(0x6001);
    if (xz_distance_between_players() < 2.25f && is_he_airborn() == 0) {
        plyr_obj->pos_vel.x *= 1.2f;
        plyr_obj->pos_vel.z *= 1.2f;
        plyr_obj->pos_vel.y *= 1.3f;
        plyr_obj->gravity *= 1.1f;
        plyr_obj->flags_09_bits.face_opponent = 0;
    }

    flight_ticks = 2.0f * (plyr_obj->pos_vel.y / plyr_obj->gravity);
    if (flight_ticks < 0.0f) {
        flight_ticks = -flight_ticks;
    }
    plyr_anim_pdata->step = 18.0f / flight_ticks;
    remaining_ticks = flight_ticks - 1.0f;
    aproc->flags |= MKPROC_FLAG_USE_GAME_SPEED;
    while (remaining_ticks > 0.0f) {
        moves_sleep(1.0f);
        remaining_ticks -= 1.0f;
    }
    aproc->flags &= ~MKPROC_FLAG_USE_GAME_SPEED;

    player_feet_land_chores();
    stop_me();
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    anim = (MovesAnimPdataView*)plyr_anim_pdata;
    high_frame = anim->base.high_frame;
    anim->landing_start = high_frame - 10.0f;
    anim->landing_end = anim->base.high_frame;
    xfer_proc(plyr_anim_proc, (MkProcEntryFn)aniproc_land);
}

float jump_towards_opponent_bgnd_transition(void) {
    MovesAnimPdataView* anim;
    MkHdr* object;
    float flight_ticks;
    float remaining_ticks;
    float high_frame;

    face_opponent_now();
    init_air_move_no_aniproc();
    set_my_state(0x6000);
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(shared_ani.jump_towards, 3, 0.1f);
    plyr_anim_pdata->step = 2.0f;
    ani_to_frame_x(11.0f);
    plyr_obj->flags_08_bits.moving = 1;
    set_jump_towards_velocities();
    set_my_state(0x6001);

    flight_ticks = 2.0f * (plyr_obj->pos_vel.y / plyr_obj->gravity);
    if (flight_ticks < 0.0f) {
        flight_ticks = -flight_ticks;
    }
    plyr_anim_pdata->step = 18.0f / flight_ticks;
    remaining_ticks = flight_ticks - 1.0f;
    aproc->flags |= MKPROC_FLAG_USE_GAME_SPEED;
    while (remaining_ticks > 0.0f) {
        moves_sleep(1.0f);
        remaining_ticks -= 1.0f;
    }
    aproc->flags &= ~MKPROC_FLAG_USE_GAME_SPEED;

    player_feet_land_chores();
    stop_me();
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    anim = (MovesAnimPdataView*)plyr_anim_pdata;
    high_frame = anim->base.high_frame;
    anim->landing_start = high_frame - 10.0f;
    anim->landing_end = anim->base.high_frame;
    xfer_proc(plyr_anim_proc, (MkProcEntryFn)aniproc_land);
    return 0.0f;
}

void jump_landing_j_exit(void) {
    MovesAnimPdataView* anim;
    MkHdr* object;
    float high_frame;

    player_feet_land_chores();
    stop_me();
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    anim = (MovesAnimPdataView*)plyr_anim_pdata;
    high_frame = anim->base.high_frame;
    anim->landing_start = high_frame - 10.0f;
    anim->landing_end = anim->base.high_frame;
    xfer_proc(plyr_anim_proc, (MkProcEntryFn)aniproc_land);
    moves_jump(j_exit);
}

void wall_dodge(void) {
    int direction;
    int script_direction;

    if (drone_ai_should_roll(1) == 0) {
        moves_jump(p_blend_to_stance_in_10);
        return;
    }
    init_3d_move();
    script_direction = plyr_pdata->script_exit_value_int;
    direction = script_direction != 1;
    if (is_my_chest_to_screen(1 - script_direction) == 0) {
        direction = script_direction == 1;
    }
    if (direction != 0) {
        blend_to_ani(shared_ani.wall_dodge_b, 3, 0.1f);
    } else {
        blend_to_ani(shared_ani.wall_dodge_a, 3, 0.1f);
    }
    plyr_pdata->collision_disabled = 1;
    plyr_anim_pdata->step = 0.8f;
    if (direction != 0) {
        moves_jump(p_blend_to_fstance_in_10);
    } else {
        moves_jump(p_blend_to_stance_in_10);
    }
}

void update_my_last_switch(void) {
    MovesSwitchLogEntry* entry;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        entry = &p1_switch_log[p1_log_index];
        p1_last_switch_bit = entry->switch_id;
        p1_last_switch_time = entry->switch_value;
        p1_current_switch_bit = entry->switch_id;
        p1_current_switch_time = entry->switch_value;
        return;
    }
    p2_current_log_index = p2_log_index;
    entry = &p2_switch_log[p2_log_index];
    p2_last_switch_bit = entry->switch_id;
    p2_last_switch_time = entry->switch_value;
    p2_current_switch_bit = entry->switch_id;
    p2_current_switch_time = entry->switch_value;
}

void kill_spear(void) {
    MovesSpearLatchView* latch;
    MkProc* proc;

    latch = (MovesSpearLatchView*)plyr_pdata;
    proc = latch->spear_proc;
    if (proc != 0 && proc->instance != latch->spear_proc_instance) {
        proc = 0;
    }
    if (proc != 0) {
        xfer_proc(proc, p_sc_spear_kill);
    }
}

void retract_spear(void) {
    MovesSpearLatchView* latch;
    MkProc* proc;

    latch = (MovesSpearLatchView*)plyr_pdata;
    proc = latch->spear_proc;
    if (proc != 0 && proc->instance != latch->spear_proc_instance) {
        proc = 0;
    }
    if (proc != 0) {
        xfer_proc(proc, p_sc_spear_retract);
    }
    blend_to_fstance(0.1f);
    moves_jump(j_exit);
}

void tug_in_spear(void) {
    MovesMoveDataView* move_data;
    int character;

    move_data = (MovesMoveDataView*)plyr_pdata;
    character = plyr_pdata->character_id;
    if (character == 0x19 || character == 0x1A) {
        snd_req(0x30D);
        blend_to_ani(move_data->boss_spear_tug, 3, 0.1f);
    } else {
        snd_req(0x2CF);
        blend_to_ani(move_data->spear_tug, 3, 0.1f);
    }
    plyr_anim_pdata->step = 0.5f;
    if (character == 0x19) {
        play_sound_1(0x30E);
    } else if (character == 0x1A) {
        play_sound_1(0x310);
    } else {
        play_sound_1(0x2D4);
    }
    reaction_xfer_him_nohit(0xA4);
    ani_to_blend_frame(10.0f);
    disable_this_move_exec(0x1203, 0xA0);
    blend_to_fstance(0.1f);
    moves_jump(j_exit);
}

void weapon_block(void) {
    MovesFighterDefinitionView* fighter;

    fighter = (MovesFighterDefinitionView*)plyr_pdata->fighter_definition;
    set_my_state(0xA00);
    blend_to_ani(fighter->weapon_block_intro, 3, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    glitch_to_ani(fighter->weapon_block_loop, 0);
    moves_jump(j_block_loop);
}

float x_block(void) {
    MovesBlockStateView* block;
    int opponent_requirement;

    block = (MovesBlockStateView*)plyr_pdata;
    init_ground_move_no_aniproc();
    stop_me();
    random_hit(7);
    dead_liukang_snd_chain_check(plyr_pdata, 4, 8, 0x32);
    if ((unsigned int)(exec_tick_ctr - block->previous_block_tick) > 10U) {
        block->block_counter = 0;
    }
    random_voice(9);
    g_drone_faked_out = 0;
    g_min_time_in_block_for_drone =
        exec_tick_ctr + drone_ai_get_min_time_in_block();

    opponent_requirement = his_pdata->block_requirement;
    if (plyr_pdata->drone_request == 1 &&
        (opponent_requirement == 1 || opponent_requirement == 5 ||
         opponent_requirement == 8)) {
        if (g_drone_blocking_in_reaction == 0 &&
            drone_ai_check_block_fakeout() == 1) {
            g_drone_faked_out = 1;
        }
        if (block->block_reserve > 10) {
            block->block_reserve -= 10;
        } else {
            block->block_reserve = 0;
        }
    }

    g_drone_blocking_in_reaction = 0;
    opponent_requirement = his_pdata->block_requirement;
    if ((check_switch(plyr_pdata->controller_port, 0xE) != 0 &&
         plyr_pdata->drone_request == 0) ||
        ((opponent_requirement == 1 || opponent_requirement == 5 ||
          opponent_requirement == 8) &&
         plyr_pdata->drone_request == 1 && g_drone_faked_out == 0)) {
        moves_jump(blend_to_duck_block);
        return 0.0f;
    }

    set_my_state(0xA00);
    if (should_i_weapon_block() != 0) {
        moves_jump((MovesEntryFn)weapon_block);
    } else {
        moves_jump(block_a_intro);
    }
    return 0.0f;
}

void j_duck_block_loop(void) {
    MovesBlockStateView* block;
    MovesBlockStateView* opponent_block;
    MovesAttackStateView* opponent_attack;
    PlyrPdata* opponent;
    unsigned int timeout;
    int requirement;

    block = (MovesBlockStateView*)plyr_pdata;
    timeout = exec_tick_ctr + 120;
    if (block->block_reserve > 3) {
        block->block_reserve--;
    } else {
        block->block_reserve = 0;
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_anim_pdata->step = 0.5f;

    while (am_i_duck_blocking() != 0) {
        init_ground_move();
        set_my_state(0x900);
        nudge_towards_him(0.2f);
        moves_sleep(1.0f);
        if (plyr_pdata->his_attack_counter != get_his_attack_counter()) {
            block->block_counter++;
        }

        if (plyr_pdata->drone_request != 0) {
            if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
                opponent = g_game_info.plyr1.slot.pdata;
            } else {
                opponent = g_game_info.plyr0.slot.pdata;
            }
            opponent_attack = (MovesAttackStateView*)his_pdata;
            if (((opponent->state & 0x1000) == 0 ||
                 opponent_attack->attack_phase == 3) &&
                (unsigned int)g_min_time_in_block_for_drone <
                    (unsigned int)exec_tick_ctr &&
                ((MovesBlockStateView*)plyr_pdata->his_plyr_pdata)
                        ->drone_block_latch == 0) {
                set_my_state(0x101);
                block->previous_block_tick = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return;
            }
            if (timeout < (unsigned int)exec_tick_ctr) {
                opponent_block =
                    (MovesBlockStateView*)plyr_pdata->his_plyr_pdata;
                opponent_block->drone_block_latch = 0;
                block->previous_block_tick = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return;
            }
            requirement = his_pdata->block_requirement;
            if (requirement != 0 && requirement != 7) {
                continue;
            }
        }
        break;
    }

    if (check_switch(plyr_pdata->controller_port, 0xE) != 0 &&
        plyr_pdata->drone_request == 0) {
        set_my_state(0x101);
    } else {
        requirement = his_pdata->block_requirement;
        if ((check_switch(plyr_pdata->controller_port, 1) != 0 &&
             plyr_pdata->drone_request == 0) ||
            (plyr_pdata->drone_request == 1 &&
             (requirement == 0 || requirement == 7))) {
            random_hit(7);
            random_voice(9);
            init_ground_move();
            if (should_i_weapon_block() != 0) {
                moves_jump((MovesEntryFn)weapon_block);
            } else {
                moves_jump((MovesEntryFn)block_a_intro_glitch);
            }
            return;
        }
    }

    blend_to_stance(0.1f);
    trial_clear_provision();
    moves_jump(j_exit);
}

#define MOVES_BLOCK_BODY(block_state, intro_animation, loop_animation)       \
    do {                                                                    \
        int ticks;                                                          \
        set_my_state(block_state);                                          \
        plyr_pdata->his_attack_counter = get_his_attack_counter();          \
        blend_to_ani_nosleep(intro_animation, 3, 0.5f);                     \
        if (aproc->pid == 0x1001) {                                         \
            ticks = g_game_info.plyr1.slot.pdata->shared_attack_until -     \
                    exec_tick_ctr;                                          \
        } else {                                                            \
            ticks = g_game_info.plyr0.slot.pdata->shared_attack_until -     \
                    exec_tick_ctr;                                          \
        }                                                                   \
        plyr_anim_pdata->step =                                             \
            (plyr_anim_pdata->high_frame - plyr_anim_pdata->frame) / ticks; \
        if (plyr_anim_pdata->step > 1.0f ||                                 \
            plyr_anim_pdata->step <= 0.0f) {                                \
            plyr_anim_pdata->step = 1.0f;                                   \
        }                                                                   \
        while (plyr_anim_pdata->frame < plyr_anim_pdata->high_frame &&      \
               am_i_blocking() != 0) {                                      \
            moves_sleep(1.0f);                                              \
        }                                                                   \
        plyr_anim_pdata->frame = plyr_anim_pdata->high_frame;               \
        plyr_anim_pdata->step = 1.0f;                                       \
        blend_to_ani_nosleep(loop_animation, 0, 0.5f);                      \
        moves_jump(j_block_loop);                                           \
    } while (0)

float block_d(void) {
    MOVES_BLOCK_BODY(0xA03, shared_ani.block_d_intro, shared_ani.block_d_loop);
    return 0.0f;
}

float block_c(void) {
    MOVES_BLOCK_BODY(0xA02, shared_ani.block_c_intro, shared_ani.block_c_loop);
    return 0.0f;
}

float block_b(void) {
    MOVES_BLOCK_BODY(0xA01, shared_ani.block_b_intro, shared_ani.block_b_loop);
    return 0.0f;
}

float block_a(void) {
    MOVES_BLOCK_BODY(0xA00, shared_ani.block_a_intro, shared_ani.block_loop);
    return 0.0f;
}

#undef MOVES_BLOCK_BODY

void block_a_intro_glitch(void) {
    set_my_state(0xA00);
    blend_to_ani(shared_ani.block_intro, 3, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    glitch_to_ani(shared_ani.block_loop, 0);
    g_min_time_in_block_for_drone =
        exec_tick_ctr + drone_ai_get_min_time_in_block();
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    moves_jump(j_block_loop);
}

void big_boss_death(void) {
    int function;

    start_gore2_update();
    function = get_script_function_by_name(
        plyr_pdata->cmo, "dragonking_death");
    cmdscript_setup_execution(plyr_pdata->cmo, function);
    cmdscript_execute(plyr_pdata->cmo);
    f_fatality_finished = 1;
    while (1) {
        moves_sleep(60.0f);
    }
}

void disable_mileena_collisions(int disable) {
    int character;

    if (plyr_pdata == 0) {
        return;
    }
    character = plyr_pdata->character_id;
    if (character != 4 && character != 0x1B) {
        return;
    }
    if (disable == 1) {
        plyr_pdata->collision_disabled = 1;
        plyr_obj->flags_09_bits.launched = 0;
        plyr_obj->flags_09_bits.bit6 = 0;
    } else {
        plyr_pdata->collision_disabled = 0;
        plyr_obj->flags_09_bits.launched = 1;
        plyr_obj->flags_09_bits.bit6 = 1;
    }
}

void idle_his_anim_proc(void) {
    MovesProcessLatchView* latch;
    MkProc* proc;

    if (his_pdata == 0) {
        return;
    }
    latch = (MovesProcessLatchView*)his_pdata;
    proc = latch->anim_proc;
    if (proc != 0 && proc->instance != latch->anim_proc_instance) {
        proc = 0;
    }
    if (proc != 0) {
        xfer_proc(proc, p_anim_idle);
    }
}

void attack_opponent_with(
    int attack, MovesAttackInfo* info, int reaction) {
    MovesAttackStateView* attack_state;
    unsigned int block_requirement;

    attack_state = (MovesAttackStateView*)plyr_pdata;
    set_attackers_attack_region(info->attack_region);
    attack_state->attack_start_tick = game_tick_ctr;
    attack_state->attack_phase = 1;
    block_requirement = info->block_requirement;
    if (block_requirement == 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 6, 0);
    } else {
        trial_increment_state_value(plyr_pdata->plyr_num, 7, 0);
    }
    plyr_pdata->block_requirement = block_requirement;
    attack_to_frame_x(
        attack, info->attack_arg1, info->attack_arg2, info->attack_arg3,
        info->attack_frame, info->attack_x, info->attack_y, info->attack_z);
    attack_state->attack_phase = 2;
    ani_to_frame_x_col(
        info->attack_region, reaction, block_requirement,
        info->collision_frame, info->collision_x, info->collision_y,
        info->collision_z);
    attack_state->attack_phase = 3;
}
