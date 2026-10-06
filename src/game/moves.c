#include "game/konquest_missions.h"
#include "runtime/mk_pdata.h"
#include "runtime/anim_pdata.h"
#include "runtime/anim_api.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/asset.h"
#include "runtime/cam.h"
#include "runtime/sound.h"
#include "runtime/utils.h"
#include "game/game_info.h"
#include "game/constrain.h"
#include "game/bgnd.h"
#include "game/jmt.h"
#include "game/ejb.h"
#include "game/ai.h"
#include "runtime/plyr_anim_pdata.h"
#include "game/moveset.h"
#include "game/moves.h"
#include "game/switch.h"

#include "game/plyr.h"
#include "game/plyr_globals.h"
#include "game/trial.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "platform/io.h"
#include "platform/joy.h"
#include "platform/main.h"

static float p_scorpion_scale(void);

struct MovesSidekickPdata {
    MkHdr hdr;
    PlyrPdata* player;
};

union MovesSidekickPdataRef {
    MkHdr* hdr;
    struct MovesSidekickPdata* sidekick;
};

struct MovesWeaponWatchPdata {
    MkHdr hdr;
    MkProc* player_proc;
    unsigned int player_proc_instance;
    MkProcEntryFn monitor_token;
    int timeout;
};

struct MovesSharedAnimations {
    char pad00[0x1C];
    AniData* flying_land;
    AniData* dash_back;
    char pad24[0x184];
    AniData* wall_dodge_a;
    AniData* wall_dodge_b;
    char pad1B0[0x30];
    AniData* jump_towards;
    AniData* jump_away;
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
    char pad274[4];
    AniData* flying_kick;
    AniData* flying_kick2;
    AniData* flying_punch;
    char pad284[0x24];
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
    AniData* duck_block_intro;
    AniData* duck_block_loop;
    char pad2E8[0x38];
    AniData* sidekick_charge;
    AniData* fall_dead;
    char pad328[4];
    AniData* dizzy;
    char pad330[0x1C];
    AniData* grab_animations[9];
};

struct MovesScalePdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    float timeout;
    int expanding;
    float hold_ticks;
    float scale;
    float scale_rate;
};

struct MovesBlastPdata {
    MkHdr hdr;
    MkObj* object;
    unsigned int object_instance;
    int current_tick;
    int end_tick;
    float start_alpha;
    float end_alpha;
};

struct MovesAttackInfo {
    float attack_frame;
    float collision_frame;
    float blend_rate;
    float step;
    float weight;
    unsigned int voice_event;
    unsigned int whoosh_event;
    int transition;
    int attack_region;
    float collision_x;
    float collision_y;
    float collision_z;
    unsigned int block_requirement;
};

struct MovesPickup {
    MkHdr hdr;
    int type;
    MkObj* primary_object;
    MkObj* secondary_object;
    MkSobj* transform_a;
    MkSobj* transform_b;
    int background_moveset;
    char pad20[4];
    int background_sobj_id;
    char pad28[0x0C];
    Vec primary_position;
    Vec primary_angle;
    Vec secondary_position;
    Vec secondary_angle;
    char pad64[0x18];
    unsigned int pickup_script;
};

struct MovesWeaponGrabEntry {
    float animation_frame;
    float angle;
    float normal_grab_type;
    float flipped_grab_type;
    float weighting;
};

unsigned int scan_freak_4[1] = {(unsigned int)-1};
AiFightstyleAttack temp_throw_switch = {4, 0x8F};

extern PlyrPdata* his_pdata;
extern MkProc* plyr_anim_proc;
extern float debug_x;
extern int round_winner;
extern int f_fatality_available;
extern int f_fatality_was_done;
extern int f_fatality_finished;
int g_min_time_in_block_for_drone;
static int victory_proper_flip_flags;
int g_drone_faked_out;
int g_drone_blocking_in_reaction;
extern float drone_blocking_done(void);
extern struct MovesSharedAnimations shared_ani;
extern int p1_current_switch_bit;
extern int p1_current_switch_time;
extern int p1_last_switch_bit;
extern int p1_last_switch_time;
extern int p2_current_switch_bit;
extern int p2_current_switch_time;
extern int p2_last_switch_bit;
extern int p2_last_switch_time;
extern float aniproc_land(void);

typedef MkProcEntryFn MovesEntryFn;

void bgnd_swap_level(int level);
void bgnd_move_plyrs_to_initial_pos(void);

static float x_pickup(void);
static AniData* fetch_grab_anim_ptr(unsigned int grab_type);
static float p_sidekick_watchdog_launcher(void);
static float drahmin_dash_back(void);
static float p_plyr_sidekick_projectile(void);
static float p_plyr_sidekick_intro(void);
static float p_plyr_smoke_entrance(void);
static float p_plyr_noob_entrance(void);
static float p_plyr_sidekick_charge(void);
static float p_plyr_sidekick_switch(void);
static float p_sidekick_exit_now(void);
void avoid_double_ani(void);
void init_ground_move_no_aniproc(void);
void init_air_move(void);
void face_opponent_now(void);
void ani_loop_more_frames(float frames);
int my_pad_position(void);
void blend_to_fstance(float rate);
int is_my_chest_to_screen();
static float back_rollup_left(void);
static float back_rollup_right(void);
static float front_rollup_left(void);
static float front_rollup_right(void);
void init_3d_move(void);
static float rollup_finish(void);
float start_suicide(void);
float start_fatality(void);
float start_2nd_fatality(void);
static float p_hide_and_die(void);
static float p_watch_weapon(void);
float x_attack_1(void);
float x_attack_2(void);
float x_attack_3(void);
float x_attack_4(void);
float x_attack_5(void);
int is_pX_airborn(int player_number);
int trial_block_callback(int player);
void advance_active_moveset(PlyrPdata* player);
void tightrope_restrictions_on(void);
void set_my_secondary_state(int state);
void ani_x_more_frames(float frames);
void random_voice(int group);
void ani_to_blend_frame(float frames);
int do_i_have_life_left(void);
void stop_me(void);
void player_feet_land_chores(void);
void ani_to_frame_x(float frame);
void ani_to_end(void);
void ani_1_frame(void);
void got_hit_fx(int type, int sound_group, int blood, int arg3, int arg4, float rate, int arg5);
int drone_ai_should_roll(int mode);
void play_sound_1(int sound_id);
void reaction_xfer_him_nohit(int reaction);
void glitch_to_ani(AniData* animation, int transition);
int drone_ai_get_min_time_in_block(void);
int get_his_attack_counter(void);
int get_player_number(MkObj* object);
MkProc* start_scorpion_spear(int field_34);
int trial_change_style_callback(int player);
void start_gore2_update(void);
void set_attackers_attack_region(int region);
void attack_to_frame_x(AniData* animation, float frame, float blend_rate,
                       float step, float weight, unsigned int voice_event,
                       unsigned int whoosh_event, int transition);
void ani_to_frame_x_col(
    float frame, int region, float x, float y, int reaction, float z,
    unsigned int collision_ticks);
void whoosh_fx(int sound);
float p_sc_spear_retract(void);
float p_sc_spear_kill(void);
static float tug_in_spear(void);
static float retract_spear(void);
float p_blend_to_stance_in_10(void);
float p_blend_to_fstance_in_10(void);
float j_blend_to_fstance_in_x(void);
float j_stay_down_dead(void);
float trial_run_loser_animation_script(void);
float fall_dead(void);
float j_block_loop(void);
float x_advance_fatality(void);
float x_advance_moveset(void);
static float blend_to_duck_block(void);
static float block_a_intro(void);
static float block_a_intro_glitch(void);
void set_ani_weight(float weight);
void blend_to_ani_nosleep(AniData* animation, int transition, float blend_rate);
int drone_ai_check_block_fakeout(void);
void random_hit(int group);
int am_i_duck_blocking(void);
int am_i_a_big_character(void);
int is_he_airborn(void);
void nudge_towards_him(float max_step);
void dead_liukang_snd_chain_check(
    PlyrPdata* player, int base_delay, int delay_range,
    unsigned int likelihood);
void transition_to_anim_script(
    AnimPdata* anim, AniData* animation, int transition, float blend_rate);
float sidekick_cool_vanish(PlyrPdata* player);
void init_air_move_no_aniproc(void);
void set_jump_towards_velocities(void);
static void do_pickup(struct MovesPickup* pickup, Vec* offset, int take);
static float j_flying_kick1_early(void);
static float j_flying_kick2_early(void);
static float j_flying_punch_early(void);
float j_flying_kick2(void);
float j_flying_kick(void);
static float j_flying_punch(void);
int collision_2(int attack_region, float radius, float extension);
int reaction_xfer_him(int reaction, float damage_scale, int block_type);
void air_collision_pause(int pause_ticks, float target_frame, float gravity);
void set_collision_made_flag(void);
void start_plyr_attack(float radius);
int does_he_have_life_left(void);
void random_dk_foot(void);
void set_ani_speed(float speed);
int am_i_airborn(void);
static void set_grab_anim_weighting(const Vec* offset, unsigned int grab_type);
void clear_both_face_opponent_flags(void);
void plyr_weapon_hide(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* mirror_slots);
void plyr_weapon_show(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* mirror_slots);
void plyr_weapon_trail_hide(PlyrMirrorSlots* mirror_slots);
void plyr_weapon_trail_show(PlyrMirrorSlots* mirror_slots);
void plyr_weapon_grab(PlyrPdata* player, MkObj* weapon);
void plyr_weapon2_grab(PlyrPdata* player, MkObj* weapon);
void plyr_weapon3_grab(PlyrPdata* player, MkObj* weapon);
void plyr_weapon4_grab(PlyrPdata* player, MkObj* weapon);
MkObj* plyr_weapon_release(PlyrPdata* player);
MkObj* plyr_weapon2_release(PlyrPdata* player);
void enable_bgnd_obj_repel(MkHdr* object);
void disable_bgnd_obj_repel(MkHdr* object);
void switch_to_bgnd_moveset(PlyrPdata* player, int moveset);
float bgnd_call_script_function(void);
int my_joypad_state_5(void);
void weapon_trail_on(void);
void plyr_going_to_attack_with(const AiFightstyleAttack* action);
int is_weapon_style(PlyrFighterDefinition* style);
float r_call_player_char_script_function(void);
void tag_team_activate_player(MkObj* sidekick, int active);
void select_fighter_voice_in_bank(int player, int alternate_voice);
void show_fighting_style(GlobalMoveset* moveset, int player);
float r_call_script_function(void);
int am_i_on_the_left2(MkObj* player, MkObj* opponent);
int plyr_start_script_in_plyr_pdata_proc(
    PlyrPdata* player, int pid, int function);
void obj_set_gravity(MkObj* object, float gravity);
void shake_camera(int strength, float duration);
void clear_my_face_opponent_flag(void);
void move_player(MkObj* fighter, const Vec* position, const Vec* angle);
void random_foot(int type);
void uv_to_opponent(Vec* direction);
void wait_to_land(void);
void setup_to_match_land_frame(
    float vertical_velocity, float gravity, float frames);
void head_tracking_off(void);
void ani_to_frame_x_call(void (*callback)(void), float frame);
void init_3d_move_no_aniproc(void);
int is_local_plyr(void);
int was_button_pressed(int button);
int drone_ai_check_next_block_state(unsigned int tick);
void tightrope_restrictions_off(void);

float rotate_toward_j_exit(void);
static float walk_backward(void);
static float walk_forward(void);
float blend_to_stance_j_exit(void);
static float blend_to_fstance_j_exit(void);
static float weapon_block(void);
static float block_a(void);
static float block_b(void);
static float block_c(void);
static float block_d(void);
static float walk_left(void);
static float walk_right(void);
static float jump_away_opponent_j_exit(void);
static float jump_towards_opponent_j_exit(void);
float r_hit_wall(void);
float throw_spear(void);
float go_into_twitch_death(void);
float go_into_major_pain(void);
float j_ass_rollup(void);
float front_rollup(void);
static float jump_landing_j_exit(void);
float dizzy(void);
float r_chest2_stumble(void);
static float do_my_fatality_remote(void);
static float x_attack_5_remote(void);
static float do_my_suicide_remote(void);
static float do_my_2nd_fatality_remote(void);
float back_to_crouch(void);
float do_my_suicide(void);

#include "src/game/moves_scan_tables.inc"

struct MovesWeaponGrabEntry weapon_grab_table[9] = {
    {15.0f, 0.0f, 3.0f, 3.0f, 1.5f},
    {15.0f, -5.22f, 2.0f, 5.0f, 1.5f},
    {14.0f, -4.2f, 1.0f, 6.0f, 1.5f},
    {14.0f, -3.3f, 0.0f, 7.0f, 1.5f},
    {15.0f, -2.9f, 8.0f, 8.0f, 1.5f},
    {14.0f, -2.2f, 7.0f, 0.0f, 1.5f},
    {14.0f, -1.4f, 6.0f, 1.0f, 1.5f},
    {14.0f, -0.8f, 5.0f, 2.0f, 1.5f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
};

static void check_for_suicide(void);
static float p_blast(void);

static inline float moves_inverse_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } bits;
    float guess;
    float product;
    float correction;

    if (value <= 0.0f) {
        return 0.0f;
    }
    bits.f = value;
    bits.u = 0x5F375A00U - (bits.u >> 1);
    guess = bits.f;
    product = guess * (value * guess);
    correction = 3.0f - product;
    return 0.0625f * guess * correction *
           -(correction * (product * correction) - 12.0f);
}

static inline int moves_his_state_is_not(int state) {
    return !(his_pdata->state == state);
}

static inline void moves_jump(MkProcEntryFn entry) {
    MkVtableMkproc* vtable;

    vtable = aproc->vtbl;
    vtable->jump_sleep(entry, 0.0f);
}

static inline void moves_sleep(float ticks) {
    MkVtableMkproc* vtable;

    _mkproc_sleep_ticks = ticks;
    vtable = aproc->vtbl;
    vtable->sleep();
}

static inline MkObj* moves_resolve_weapon_latch(
    const PlyrMirrorObjLatch* latch) {
    MkObj* object;

    object = latch->obj;
    return MK_HDR_LIVE(object, latch->instance);
}

static inline int moves_is_weapon_style(PlyrFighterDefinition* style) {
    PlyrStyleDefinition* weapon_data;

    if (style != 0 && (weapon_data = style->definition) != 0 &&
        (weapon_data->primary_weapon != 0 ||
         weapon_data->secondary_weapon != 0)) {
        return 1;
    }
    return 0;
}

static inline int moves_find_active_weapon(
    MkPtr** weapon_list, struct MovesPickup** result) {
    struct MovesPickup* pickup;
    MkPtr* link;
    MkPtr* next;

    if (weapon_list != 0) {
        link = *weapon_list;
        while (link != 0) {
            pickup = (struct MovesPickup*)link->hdr;
            if (link->instance != pickup->hdr.instance) {
                next = link->next;
                discard_stale_mkptr(link);
                link = next;
                continue;
            }
            if (pickup->type == 0 || pickup->type == 1) {
                *result = pickup;
                return 1;
            }
            link = link->next;
        }
    }
    return 0;
}

static inline int moves_find_nearby_pickup(
    MkObj* object, MkPtr** pickup_list, struct MovesPickup** result, Vec* offset) {
    MkSobj* transform;
    struct MovesPickup* pickup;
    MkPtr* link;
    MkPtr* next;
    float vertical_distance;

    if (pickup_list != 0) {
        link = *pickup_list;
        while (link != 0) {
            pickup = (struct MovesPickup*)link->hdr;
            if (link->instance != pickup->hdr.instance) {
                next = link->next;
                discard_stale_mkptr(link);
                link = next;
                continue;
            }

            transform = pickup->transform_a;
            if (transform == 0) {
                transform = pickup->transform_b;
            }
            offset->x = object->pos.value.x - transform->pos.x;
            offset->y = object->pos.value.y - transform->pos.y;
            offset->z = object->pos.value.z - transform->pos.z;
            if (offset->x * offset->x + offset->z * offset->z < 2.9f) {
                vertical_distance = offset->y;
                vertical_distance = vertical_distance >= 0.0f
                                        ? vertical_distance
                                        : -vertical_distance;
                if (vertical_distance < 1.5f) {
                    offset->y = 0.0f;
                    *result = pickup;
                    return 1;
                }
            }
            link = link->next;
        }
    }
    return 0;
}

static inline AniData* fetch_grab_anim_ptr_inline(unsigned int grab_type) {
    switch (grab_type) {
    case 0:
        return shared_ani.grab_animations[0];
    case 1:
        return shared_ani.grab_animations[1];
    case 2:
        return shared_ani.grab_animations[2];
    case 3:
        return shared_ani.grab_animations[3];
    case 4:
        return shared_ani.grab_animations[4];
    case 5:
        return shared_ani.grab_animations[5];
    case 6:
        return shared_ani.grab_animations[6];
    case 7:
        return shared_ani.grab_animations[7];
    case 8:
        return shared_ani.grab_animations[8];
    default:
        return shared_ani.grab_animations[0];
    }
}

static inline void moves_dispatch_attack(AiFightstyleAttack* action) {
    plyr_going_to_attack_with(action);
    switch (action->opcode) {
    case 0:
        cmdscript_reset_stack();
        cmdscript_setup_execution(
            plyr_pdata->fighter_definition->cmo, action->argument);
        call_player_script_function(plyr_pdata->fighter_definition->cmo);
        break;
    case 1:
        moves_jump(action->entry);
        break;
    case 2:
        cmdscript_reset_stack();
        cmdscript_setup_execution(
            plyr_pdata->cmo, action->argument);
        call_player_script_function(plyr_pdata->cmo);
        break;
    case 3:
        cmdscript_reset_stack();
        cmdscript_setup_execution(
            plyr_pdata->his_plyr_pdata->cmo, action->argument);
        call_player_script_function(plyr_pdata->his_plyr_pdata->cmo);
        break;
    case 4:
        cmdscript_reset_stack();
        cmdscript_setup_execution(
            reactions_cmo, action->argument);
        call_player_script_function(reactions_cmo);
        break;
    }
}

static inline void moves_prepare_sidekick_entrance(
    AnimPdata* animation, MkObj* sidekick) {
    animation->flags &= ~8U;
    sidekick->hide_flag_bits.bit6 = 0;
}

static inline void victory_wait_for_pre_animation(void) {
    PlyrFighterDefinition* fighter = plyr_pdata->fighter_definition;
    int ticks = 240;

    if (plyr_anim_pdata->script ==
        fighter->duck_exit_animation) {
        return;
    }
    while (plyr_anim_pdata->frame < plyr_anim_pdata->high_frame &&
           plyr_pdata->state != 0x4200 && ticks != 0) {
        ticks--;
        moves_sleep(1.0f);
    }
}

static inline int moves_dead_movement(void) {
    if (plyr_pdata == 0) {
        return 0;
    }
    if (round_winner == 0) {
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

static inline int moves_has_nearby_pickup(MkObj* object, MkPtr** pickup_list) {
    MkSobj* transform;
    struct MovesPickup* pickup;
    MkPtr* link;
    MkPtr* next;
    float delta_z;
    float delta_x;
    float delta_y;

    if (pickup_list != 0) {
        link = *pickup_list;
        while (link != 0) {
            pickup = (struct MovesPickup*)link->hdr;
            if (link->instance != pickup->hdr.instance) {
                next = link->next;
                discard_stale_mkptr(link);
                link = next;
                continue;
            }
            transform = pickup->transform_a;
            if (transform == 0) {
                transform = pickup->transform_b;
            }
            delta_x = object->pos.value.x - transform->pos.x;
            delta_z = object->pos.value.z - transform->pos.z;
            delta_y = object->pos.value.y - transform->pos.y;
            if (delta_x * delta_x + delta_z * delta_z < 2.9f) {
                delta_y = delta_y >= 0.0f ? delta_y : -delta_y;
                if (delta_y < 1.5f) {
                    return 1;
                }
            }
            link = link->next;
        }
    }
    return 0;
}

static inline void j_flying_kick2_airborne(void) {
    while (plyr_anim_pdata->frame < 8.0f) {
        ani_1_frame();
        if (plyr_obj->gravity == 0.0) {
            break;
        }
        moves_sleep(1.0f);
    }
    start_plyr_attack(0.0f);
    while (plyr_anim_pdata->frame < 11.0f) {
        ani_1_frame();
        if (plyr_pdata->collision_result == -1 && collision_2(8, 1.0f, 0.0f) != 0) {
            stop_me();
            set_collision_made_flag();
            reaction_xfer_him(0x36, 0.1f, 0);
        }
        if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
            !plyr_obj->flags_08_bits.moving) {
            return;
        }
        moves_sleep(1.0f);
    }
    if (plyr_pdata->collision_result != -1) {
        air_collision_pause(5, 12.0f, -0.02f);
    }
    while (plyr_anim_pdata->frame < 14.0f) {
        if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
            !plyr_obj->flags_08_bits.moving) {
            return;
        }
        ani_1_frame();
        moves_sleep(1.0f);
    }
    plyr_obj->flags_08_bits.moving = 1;
    while (plyr_obj->pos.value.y > plyr_obj->ground_colls_y + 1.0f &&
           plyr_obj->flags_08_bits.moving == 1 && plyr_obj->gravity != 0.0f) {
        ani_1_frame();
        moves_sleep(1.0f);
    }
}

static inline void j_flying_kick_airborne(void) {
    while (plyr_anim_pdata->frame < 7.0f) {
        ani_1_frame();
        if (plyr_obj->gravity == 0.0) {
            break;
        }
        moves_sleep(1.0f);
    }
    start_plyr_attack(0.0f);
    while (plyr_anim_pdata->frame < 10.0f) {
        ani_1_frame();
        if (plyr_pdata->collision_result == -1 && collision_2(0xD, 1.0f, 0.0f) != 0) {
            stop_me();
            set_collision_made_flag();
            reaction_xfer_him(0x32, 0.1f, 0);
        }
        if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
            !plyr_obj->flags_08_bits.moving) {
            return;
        }
        moves_sleep(1.0f);
    }
    if (plyr_pdata->collision_result != -1) {
        air_collision_pause(5, 12.0f, -0.01f);
    }
    plyr_anim_pdata->step = 0.5f;
    while (plyr_anim_pdata->frame < 13.0f) {
        if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
            !plyr_obj->flags_08_bits.moving) {
            return;
        }
        ani_1_frame();
        moves_sleep(1.0f);
    }
    plyr_obj->flags_08_bits.moving = 1;
    plyr_anim_pdata->high_frame = 20.0f;
    plyr_anim_pdata->step = 1.0f;
    while (plyr_obj->pos.value.y > plyr_obj->ground_colls_y + 1.0f &&
           plyr_obj->flags_08_bits.moving == 1 && plyr_obj->gravity != 0.0f) {
        ani_1_frame();
        moves_sleep(1.0f);
    }
}

static inline void flying_punch_airborne(void) {
    while (plyr_anim_pdata->frame < 10.0f) {
        ani_1_frame();
        if (plyr_pdata->collision_result == -1 &&
            collision_2(7, 1.0f, 0.0f) != 0) {
            stop_me();
            set_collision_made_flag();
            reaction_xfer_him(0x2B, 0.1f, 0);
        }
        if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
            !plyr_obj->flags_08_bits.moving) {
            return;
        }
        moves_sleep(1.0f);
    }
    if (plyr_pdata->collision_result != -1) {
        air_collision_pause(5, 12.0f, -0.01f);
    }
    plyr_anim_pdata->step = 0.5f;
    if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
        !plyr_obj->flags_08_bits.moving) {
        return;
    }
    moves_sleep(1.0f);
    if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
        !plyr_obj->flags_08_bits.moving) {
        return;
    }
    moves_sleep(1.0f);
    while (plyr_anim_pdata->frame < 13.0f) {
        if (plyr_obj->pos.value.y < plyr_obj->ground_colls_y + 1.0f ||
            !plyr_obj->flags_08_bits.moving) {
            return;
        }
        ani_1_frame();
        moves_sleep(1.0f);
    }
    plyr_obj->flags_08_bits.moving = 1;
    plyr_anim_pdata->step = 1.0f;
    while (plyr_obj->pos.value.y > plyr_obj->ground_colls_y + 1.0f &&
           plyr_obj->flags_08_bits.moving == 1 &&
           plyr_obj->gravity != 0.0f) {
        ani_1_frame();
        moves_sleep(1.0f);
    }
}

static inline void moves_transition_block(
    int state, AniData** intro, AniData** loop) {
    int ticks;

    set_my_state(state);
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    blend_to_ani_nosleep(*intro, 3, 0.5f);
    if (aproc->pid == 0x1001) {
        ticks = g_game_info.plyr1.slot.pdata->shared_attack_until -
                exec_tick_ctr;
    } else {
        ticks = g_game_info.plyr0.slot.pdata->shared_attack_until -
                exec_tick_ctr;
    }
    plyr_anim_pdata->step =
        (plyr_anim_pdata->high_frame - plyr_anim_pdata->frame) / ticks;
    if (plyr_anim_pdata->step > 1.0f) {
        plyr_anim_pdata->step = 1.0f;
    } else if (plyr_anim_pdata->step <= 0.0f) {
        plyr_anim_pdata->step = 1.0f;
    }
    while (plyr_anim_pdata->frame < plyr_anim_pdata->high_frame) {
        if (am_i_blocking() == 0) {
            break;
        }
        moves_sleep(1.0f);
    }
    plyr_anim_pdata->frame = plyr_anim_pdata->high_frame;
    plyr_anim_pdata->step = 1.0f;
    blend_to_ani_nosleep(*loop, 0, 0.5f);
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
        if (plyr_anim_pdata->step > 1.0f) { \
            plyr_anim_pdata->step = 1.0f; \
        } else if (plyr_anim_pdata->step <= 0.0f) { \
            plyr_anim_pdata->step = 1.0f; \
        } \
        while (plyr_anim_pdata->frame < plyr_anim_pdata->high_frame) { \
            if (am_i_blocking() == 0) { \
                break; \
            } \
            moves_sleep(1.0f); \
        } \
        plyr_anim_pdata->frame = plyr_anim_pdata->high_frame;               \
        plyr_anim_pdata->step = 1.0f;                                       \
        blend_to_ani_nosleep(loop_animation, 0, 0.5f);                      \
        moves_jump(j_block_loop);                                           \
    } while (0)

static inline void moves_pre_attack_chores(void) {
    float angle_error;
    int state;

    plyr_pdata->blocking_disabled = 1;
    if (get_my_angle_y_error() >= 0.0f) {
        angle_error = get_my_angle_y_error();
    } else {
        angle_error = -get_my_angle_y_error();
    }
    if (angle_error < 1.0f) {
        if (his_pdata->state == 0x2003) {
            plyr_obj->flags_09_bits.face_opponent = 1;
        }
    } else {
        debug_x = angle_error;
    }
    rotate_towards_him(0.4f);
    plyr_obj->flags_09_bits.head_tracking = 0;
    if (should_i_weapon_block() != 0) {
        weapon_trail_on();
    }
    state = plyr_pdata->state;
    if (state != 0x4208 && state != 0x4209 && state != 0x420A &&
        state != 0x120B && state != 0x1219) {
        set_my_state(0x1200);
    }
    if (plyr_pdata->state == 0x120B) {
        trial_increment_state_value(plyr_pdata->plyr_num, 7, 0);
        plyr_pdata->pending_hit_strength = 2;
    }
    xfer_proc(plyr_anim_proc, p_animate);
}

void kobra_teleport_position(void) {
    float delta_x;
    float delta_z;
    float inverse_distance;
    float opponent_x;
    float opponent_z;
    float offset_x;
    float offset_z;
    float teleport_offset = -2.0f;

    delta_x = plyr_obj->pos.value.x - (opponent_x = his_obj->pos.value.x);
    delta_z = plyr_obj->pos.value.z - (opponent_z = his_obj->pos.value.z);
    inverse_distance =
        moves_inverse_sqrt(delta_x * delta_x + delta_z * delta_z);

    delta_x *= inverse_distance;
    delta_z *= inverse_distance;
    offset_x = delta_x * teleport_offset;
    offset_z = delta_z * teleport_offset;
    plyr_obj->pos.value.x = offset_x + opponent_x;
    plyr_obj->pos.value.z = offset_z + opponent_z;
    if (g_game_info.bgnd_id != 2) {
        set_constrain_last_pos_pdata(&his_obj->pos.value);
    }
    clear_my_face_opponent_flag();
    bgnd_clear_danger_zone_callback(plyr_pdata);
}

void mileena_sky_set_position(void) {
    float delta_x;
    float delta_z;
    float distance_squared;
    float inverse_distance;
    float opponent_x;
    float opponent_z;
    float sky_distance = 3.0f;
    float offset_x;
    float offset_z;

    delta_x = plyr_obj->pos.value.x - (opponent_x = his_obj->pos.value.x);
    delta_z = plyr_obj->pos.value.z - (opponent_z = his_obj->pos.value.z);
    distance_squared = delta_x * delta_x + delta_z * delta_z;
    inverse_distance = moves_inverse_sqrt(distance_squared);
    delta_x *= inverse_distance;
    delta_z *= inverse_distance;
    offset_x = delta_x * sky_distance;
    offset_z = delta_z * sky_distance;
    plyr_obj->pos.value.x = offset_x + opponent_x;
    plyr_obj->pos.value.z = offset_z + opponent_z;
    plyr_obj->pos.value.y = g_game_info.field_34 + 4.0f;
}

void disable_mileena_collisions(int disable) {
    if (plyr_pdata == 0 || (plyr_pdata->character_id != 4 &&
                            plyr_pdata->character_id != 0x1B)) {
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

void switch_plyr_positions(void) {
    Vec player0_position;
    Vec player_angle;
    float player0_y;
    float player1_y;

    player0_position.x = g_game_info.plyr0.slot.mirror_a->pos.value.x;
    player0_position.y = g_game_info.plyr0.slot.mirror_a->pos.value.y;
    player0_position.z = g_game_info.plyr0.slot.mirror_a->pos.value.z;
    player0_y = g_game_info.plyr0.slot.mirror_a->pos.value.y;
    player_angle.x = g_game_info.plyr0.slot.mirror_a->ang.x;
    player_angle.y = g_game_info.plyr0.slot.mirror_a->ang.y;
    player_angle.z = g_game_info.plyr0.slot.mirror_a->ang.z;

    move_player(g_game_info.plyr0.slot.mirror_a,
                &g_game_info.plyr1.slot.mirror_a->pos.value, &player_angle);
    g_game_info.plyr0.slot.mirror_a->pos.value.y = player0_y;

    player_angle.x = g_game_info.plyr1.slot.mirror_a->ang.x;
    player_angle.y = g_game_info.plyr1.slot.mirror_a->ang.y;
    player_angle.z = g_game_info.plyr1.slot.mirror_a->ang.z;
    player1_y = g_game_info.plyr1.slot.mirror_a->pos.value.y;
    move_player(g_game_info.plyr1.slot.mirror_a, &player0_position,
                &player_angle);
    g_game_info.plyr1.slot.mirror_a->pos.value.y = player1_y;
}

void start_scorpion_teleport_scale(
    void*, float scale_rate, float hold_ticks) {
    struct MovesScalePdata* data;
    MkProc* proc;

    proc = _create_mkproc_generic_tinystack(
        0xB00D, 0x1F, p_scorpion_scale, sizeof(struct MovesScalePdata),
        (MkHdr**)&data);
    if (proc == 0) {
        return;
    }

    data->object = plyr_obj;
    data->object_instance = plyr_obj->hdr.instance;
    data->timeout = 60.0f;
    data->scale_rate = scale_rate;
    data->scale = 1.0f;
    data->hold_ticks = hold_ticks;
    data->expanding = 0;
    plyr_obj->flags_08_bits.scale_active = 1;
    plyr_obj->scale.x = 1.0f;
    plyr_obj->scale.y = 1.0f;
    plyr_obj->scale.z = 1.0f;
    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        plyr_turn_off_mirrorguy(&g_game_info.plyr0);
    } else {
        plyr_turn_off_mirrorguy(&g_game_info.plyr1);
    }
}

static float p_scorpion_scale(void) {
    struct MovesScalePdata* data;
    MkObj* object;

    data = (struct MovesScalePdata*)pdata_of_proc(aproc);
    object = MK_HDR_LIVE(data->object, data->object_instance);
    if (object == 0) {
        return -1.0f;
    }

    data->timeout -= game_speed;
    if (data->timeout < 0.0f) {
        object->flags_08_bits.scale_active = 0;
        if (object == g_game_info.plyr0.slot.mirror_a) {
            plyr_turn_on_mirrorguy(&g_game_info.plyr0);
        } else {
            plyr_turn_on_mirrorguy(&g_game_info.plyr1);
        }
        return -1.0f;
    }

    if (data->expanding == 0) {
        data->scale -= data->scale_rate;
        if (data->scale < 0.1f) {
            data->scale = 0.1f;
        }
        data->hold_ticks -= game_speed;
        if (data->hold_ticks < 0.0f) {
            data->expanding = 1;
            return 1.0f;
        }
    } else {
        data->scale += data->scale_rate;
        if (data->scale > 1.0f) {
            if (object == g_game_info.plyr0.slot.mirror_a) {
                plyr_turn_on_mirrorguy(&g_game_info.plyr0);
            } else {
                plyr_turn_on_mirrorguy(&g_game_info.plyr1);
            }
            object->flags_08_bits.scale_active = 0;
            return -1.0f;
        }
    }

    object->scale.x = data->scale;
    object->scale.y = data->scale;
    object->scale.z = data->scale;
    return 1.0f;
}

void blast_effect_at_plyr(void) {
    struct MovesBlastPdata* data;
    MkObj* blast;
    MkProc* proc;

    blast = load_named_model_for_player(
        "BLAST", plyr_pdata->plyr_num,
        0x600A, 0);
    if (blast == 0) {
        return;
    }
    blast->pos.value.x = plyr_obj->pos.value.x;
    blast->pos.value.z = plyr_obj->pos.value.z;
    blast->pos.value.y = g_game_info.field_34;
    blast->flags_08_bits.scale_active = 1;
    blast->scale.x = 0.2f;
    blast->scale.y = 0.2f;
    blast->scale.z = 0.2f;
    insert_fgnd_mkobj(blast);

    proc = _create_mkproc_generic_tinystack(
        0x2026, 0x1F, p_blast, sizeof(struct MovesBlastPdata), (MkHdr**)&data);
    if (proc == 0) {
        if (blast->hdr.instance != 0) {
            blast->hdr.typed_vtbl->destroy(&blast->hdr);
        }
        return;
    }
    data->object = blast;
    data->object_instance = blast->hdr.instance;
    mk_insert(&blast->hdr, &proc->pdata_list_b);
    data->end_tick = 0x24;
    data->current_tick = 0x24;
    data->start_alpha = 128.0f;
    data->end_alpha = 0.0f;
}

static float p_blast(void) {
    RwRGBA color = {0x64, 0xFF, 0x64, 0xFF};
    struct MovesBlastPdata* data;
    MkObj* object;
    float interval;
    float fraction;

    data = (struct MovesBlastPdata*)pdata_of_proc(aproc);
    object = MK_HDR_LIVE(data->object, data->object_instance);
    if (object == 0) {
        return -1.0f;
    }

    object->scale.x += 0.1f;
    object->scale.y += 0.1f;
    object->scale.z += 0.1f;
    if (object->scale.x > 8.0f) {
        object->scale.x = 8.0f;
        object->scale.y = 8.0f;
        object->scale.z = 8.0f;
    }

    interval = data->end_tick - data->current_tick;
    if (interval == 0.0f) {
        color.alpha = (signed char)data->start_alpha;
    } else {
        fraction = (float)data->current_tick / interval;
        if (fraction > 1.0f) {
            fraction = 1.0f;
        } else if (fraction < 0.0f) {
            fraction = 0.0f;
        }
        color.alpha = (signed char)(data->start_alpha * fraction +
                                    data->end_alpha * (1.0f - fraction));
    }
    obj_set_color_for_all_materials(object, &color);
    data->current_tick--;
    if (data->current_tick < 0) {
        return -1.0f;
    }
    return 1.0f;
}

void configure_iceball(MkObj* iceball) {
    MkSobj* subobject;

    iceball->flags_08_bits.scale_active = 1;
    iceball->scale.x = 1.3f;
    iceball->scale.y = 1.3f;
    iceball->scale.z = 1.3f;

    subobject = obj_create_sobjs_by_id(iceball, 1);
    if (subobject != 0) {
        subobject->flags_08_bits.bit3 = 1;
        subobject->flags_08_bits.angular_velocity_enabled = 1;
        subobject->ang_vel.x = 0.4f;
        subobject->z_offset = -5.0f;
    }
    subobject = obj_create_sobjs_by_id(iceball, 2);
    if (subobject != 0) {
        subobject->flags_08_bits.bit4 = 1;
        subobject->flags09_bits.bit7 = 1;
        subobject->flags_08_bits.bit0 = 0;
    }
    subobject = obj_create_sobjs_by_id(iceball, 3);
    if (subobject != 0) {
        subobject->flags09_bits.bit5 = 1;
        subobject->flags09_bits.bit7 = 1;
        subobject->z_offset = -10.0f;
    }
}

static inline void moves_select_special_weapon_style(PlyrPdata* player) {
    unsigned int style_index;
    PlyrWeaponStyle* style;

    if (player != 0) {
        for (style_index = 0; style_index < 3; style_index++) {
            style = player->weapon_styles[style_index];
            if (moves_is_weapon_style(style)) {
                player->mirror_slots = &style->mirror_slots;
                break;
            }
        }
    }
}

/* TODO: [near miss] 99.32%; inline checks and callback ownership agree; search register colors and player handoff remain. */
void start_special_weapon_monitor(void) {
    struct MovesWeaponWatchPdata* pdata;
    PlyrMirrorSlots* slots;
    PlyrPdata* player;
    MkProc* monitor;

    pdata = 0;
    if (plyr_pdata == 0) {
        return;
    }

    moves_select_special_weapon_style(plyr_pdata);

    if (plyr_pdata->mirror_slots == &plyr_pdata->fighter_definition->mirror_slots) {
        return;
    }

    monitor = _create_mkproc_generic_tinystack(
        0xB00B, 0x1F, p_watch_weapon, sizeof(struct MovesWeaponWatchPdata),
        (MkHdr**)&pdata);
    if (monitor != 0 && pdata != 0) {
        if (plyr_pdata->player_slot >= 0 &&
            moves_is_weapon_style(plyr_pdata->fighter_definition) != 0) {
            plyr_weapon_hide(plyr_pdata, 0, &plyr_pdata->fighter_definition->mirror_slots);
        }

        player = plyr_pdata;
        slots = player->mirror_slots;
        if (MK_HDR_LIVE(slots->weapon[0].primary.obj, slots->weapon[0].primary.instance) != 0) {
            plyr_weapon_grab(
                player,
                MK_HDR_LIVE(slots->weapon[0].primary.obj, slots->weapon[0].primary.instance));
        }
        if (MK_HDR_LIVE(slots->weapon[1].primary.obj, slots->weapon[1].primary.instance) != 0) {
            plyr_weapon2_grab(
                player,
                MK_HDR_LIVE(slots->weapon[1].primary.obj, slots->weapon[1].primary.instance));
        }
        if (MK_HDR_LIVE(slots->weapon[2].primary.obj, slots->weapon[2].primary.instance) != 0) {
            plyr_weapon3_grab(
                player,
                MK_HDR_LIVE(slots->weapon[2].primary.obj, slots->weapon[2].primary.instance));
        }
        if (MK_HDR_LIVE(slots->weapon[3].primary.obj, slots->weapon[3].primary.instance) != 0) {
            plyr_weapon4_grab(
                player,
                MK_HDR_LIVE(slots->weapon[3].primary.obj, slots->weapon[3].primary.instance));
        }
        plyr_weapon_show(player, 1, slots);
        if (player->baraka_moveset_callback != 0) {
            player->baraka_moveset_callback(player, slots);
        }
        plyr_weapon_trail_show(plyr_pdata->mirror_slots);

        pdata->monitor_token = aproc->entry;
        pdata->player_proc = aproc;
        pdata->player_proc_instance = aproc->instance;
        pdata->timeout = 60;
        return;
    }
    plyr_pdata->mirror_slots = &plyr_pdata->fighter_definition->mirror_slots;
}

static float p_watch_weapon(void) {
    MkProc* player_proc;
    struct MovesWeaponWatchPdata* pdata;

    pdata = (struct MovesWeaponWatchPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    pdata->timeout--;
    if (pdata->timeout < 0) {
        moves_jump(p_hide_and_die);
        return 0.0f;
    }
    player_proc = MK_LIVE(pdata->player_proc, pdata->player_proc_instance);

    if (player_proc != 0 &&
        pdata->monitor_token != player_proc->entry) {
        moves_jump(p_hide_and_die);
        return 0.0f;
    }
    return 1.0f;
}

static float p_hide_and_die(void) {
    struct MovesWeaponWatchPdata* pdata;

    PlyrMirrorSlots* slots;
    PlyrPdata* player;

    MkProc* player_proc;

    pdata = (struct MovesWeaponWatchPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    player_proc = MK_LIVE(pdata->player_proc, pdata->player_proc_instance);
    if (player_proc == 0) {
        return -1.0f;
    }

    player = (PlyrPdata*)pdata_of_proc(player_proc);
    if (player->state == 0xC600) {
        return 1.0f;
    }
    if (&player->fighter_definition->mirror_slots !=
        player->mirror_slots) {
        plyr_weapon_hide(player, 1, player->mirror_slots);
        plyr_weapon_trail_hide(player->mirror_slots);
        player->mirror_slots =
            &player->fighter_definition->mirror_slots;
        slots = player->mirror_slots;
        if (slots != 0) {
            if (moves_is_weapon_style(player->fighter_definition) != 0) {
                if (MK_HDR_LIVE(slots->weapon[0].primary.obj, slots->weapon[0].primary.instance) != 0) {
                    plyr_weapon_grab(
                        player, MK_HDR_LIVE(slots->weapon[0].primary.obj, slots->weapon[0].primary.instance));
                }
                if (MK_HDR_LIVE(slots->weapon[1].primary.obj, slots->weapon[1].primary.instance) != 0) {
                    plyr_weapon2_grab(
                        player, MK_HDR_LIVE(slots->weapon[1].primary.obj, slots->weapon[1].primary.instance));
                }
                if (MK_HDR_LIVE(slots->weapon[2].primary.obj, slots->weapon[2].primary.instance) != 0) {
                    plyr_weapon3_grab(
                        player, MK_HDR_LIVE(slots->weapon[2].primary.obj, slots->weapon[2].primary.instance));
                }
                if (MK_HDR_LIVE(slots->weapon[3].primary.obj, slots->weapon[3].primary.instance) != 0) {
                    plyr_weapon4_grab(
                        player, MK_HDR_LIVE(slots->weapon[3].primary.obj, slots->weapon[3].primary.instance));
                }
                plyr_weapon_show(player, 1, slots);
                if (player->baraka_moveset_callback != 0) {
                    player->baraka_moveset_callback(player, slots);
                }
            }
            if (player->baraka_moveset_callback != 0) {
                player->baraka_moveset_callback(player, slots);
            }
        }
    }
    return -1.0f;
}

void attack_opponent_with(
    AniData* animation, struct MovesAttackInfo* info, int reaction) {
    unsigned int block_requirement;

    set_attackers_attack_region(info->attack_region);
    plyr_pdata->last_collision_tick = game_tick_ctr;
    plyr_pdata->throw_restriction = 1;
    block_requirement = info->block_requirement;
    if (block_requirement == 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 6, 0);
    } else {
        trial_increment_state_value(plyr_pdata->plyr_num, 7, 0);
    }
    plyr_pdata->block_requirement = block_requirement;
    attack_to_frame_x(
        animation, info->attack_frame, info->blend_rate, info->step,
        info->weight, info->voice_event, info->whoosh_event, info->transition);
    plyr_pdata->throw_restriction = 2;
    ani_to_frame_x_col(
        info->collision_frame, info->attack_region, info->collision_x,
        info->collision_y, reaction, info->collision_z, info->block_requirement);
    plyr_pdata->throw_restriction = 3;
}

void scorpion_teleport_position(void) {
    float delta_x;
    float delta_z;
    float inverse_distance;
    float opponent_x;
    float opponent_z;
    float offset_x;
    float offset_z;
    float teleport_offset = -3.0f;

    delta_x = plyr_obj->pos.value.x - (opponent_x = his_obj->pos.value.x);
    delta_z = plyr_obj->pos.value.z - (opponent_z = his_obj->pos.value.z);
    inverse_distance = moves_inverse_sqrt(
        delta_x * delta_x + delta_z * delta_z);
    delta_x *= inverse_distance;
    delta_z *= inverse_distance;
    offset_x = delta_x * teleport_offset;
    offset_z = delta_z * teleport_offset;
    plyr_obj->pos.value.x = offset_x + opponent_x;
    plyr_obj->pos.value.z = offset_z + opponent_z;
    bgnd_clear_danger_zone_callback(plyr_pdata);
    if (g_game_info.bgnd_id != 2) {
        set_constrain_last_pos_pdata(&his_obj->pos.value);
    }
    plyr_obj->pos.value.y = g_game_info.field_34 + 2.2f;
}

void kenshi_teleport_position(void) {
    float delta_x;
    float delta_z;
    float inverse_distance;
    float opponent_x;
    float opponent_z;
    float offset_x;
    float offset_z;
    float teleport_offset = 1.25f;

    delta_x = plyr_obj->pos.value.x - (opponent_x = his_obj->pos.value.x);
    delta_z = plyr_obj->pos.value.z - (opponent_z = his_obj->pos.value.z);
    inverse_distance = moves_inverse_sqrt(
        delta_x * delta_x + delta_z * delta_z);
    delta_x *= inverse_distance;
    delta_z *= inverse_distance;
    offset_x = delta_x * teleport_offset;
    offset_z = delta_z * teleport_offset;
    plyr_obj->pos.value.x = offset_x + opponent_x;
    plyr_obj->pos.value.z = offset_z + opponent_z;
    if (g_game_info.bgnd_id != 2) {
        set_constrain_last_pos_pdata(&his_obj->pos.value);
    }
}

float throw_spear(void) {
    MkProc* spear_proc;
    float timeout;
    float attack_ticks;

    set_my_secondary_state(0x101);
    trial_increment_state_value(plyr_pdata->plyr_num, 6, 0);
    plyr_pdata->block_requirement = 0;
    plyr_pdata->saved_position_x = plyr_obj->pos.value.x;
    plyr_pdata->saved_position_z = plyr_obj->pos.value.z;
    plyr_pdata->duck_reaction_active = 1;
    trial_register_attack(get_player_number(plyr_obj), 3, 0x33);

    spear_proc = MK_LIVE(plyr_pdata->spear_proc, plyr_pdata->spear_proc_instance);
    if (spear_proc != 0) {
        xfer_proc(spear_proc, p_sc_spear_kill);
    }

    if (plyr_pdata->character_id != 0x19 && plyr_pdata->character_id != 0x1A) {
        blend_to_ani(plyr_pdata->base_animations[0], 0x43, 0.1f);
    } else {
        blend_to_ani(plyr_pdata->common_exit_animation, 0x43, 0.1f);
    }
    ani_to_frame_x(12.0f);
    if (plyr_pdata->character_id == 0x19 || plyr_pdata->character_id == 0x1A) {
        play_sound_1(0x30A);
    } else {
        play_sound_1(0x2CC);
    }
    ani_to_end();
    set_my_state(0xD200);
    attack_ticks = 7.1666665f;
    plyr_pdata->shared_attack_until = exec_tick_ctr + (int)attack_ticks;
    plyr_pdata->attack_counter++;
    spear_proc = start_scorpion_spear(9);
    plyr_pdata->spear_proc = spear_proc;
    plyr_pdata->spear_proc_instance = spear_proc->instance;

    if (plyr_pdata->character_id != 0x19 && plyr_pdata->character_id != 0x1A) {
        blend_to_ani(plyr_pdata->base_animations[1], 0, 0.1f);
    } else {
        blend_to_ani(plyr_pdata->turn_to_screen_animation, 0, 0.1f);
    }
    ani_x_more_frames(10.0f);
    ani_to_end();

    timeout = 60.0f;
    while (timeout > 0.0f && his_pdata->state != 0x604 &&
           his_pdata->state != 0x606) {
        moves_sleep(1.0f);
        timeout -= game_speed;
    }
    set_my_state(plyr_pdata->state & ~0x1000);
    plyr_pdata->duck_reaction_active = 0;
    set_my_secondary_state(0);

    if (!g_game_info.flag_bits.lens_flare_enabled) {
        if (g_game_info.pause_flag_bits.fatality_window == 1) {
            spear_proc = MK_LIVE(plyr_pdata->spear_proc, plyr_pdata->spear_proc_instance);
            if (spear_proc != 0) {
                xfer_proc(spear_proc, p_sc_spear_kill);
            }
        }
    } else if (his_pdata->state == 0x604 || his_pdata->state == 0x603) {
        moves_jump(tug_in_spear);
        return 0.0f;
    } else if (his_pdata->state == 0x606 || his_pdata->state == 0x607) {
        moves_jump(retract_spear);
        return 0.0f;
    }
    blend_to_fstance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

static float tug_in_spear(void) {
    if (plyr_pdata->character_id == 0x19 ||
        plyr_pdata->character_id == 0x1A) {
        snd_req(0x30D);
    } else {
        snd_req(0x2CF);
    }
    if (plyr_pdata->character_id != 0x19 &&
        plyr_pdata->character_id != 0x1A) {
        blend_to_ani(plyr_pdata->base_animations[2], 3, 0.1f);
    } else {
        blend_to_ani(
            plyr_pdata->smoke_land_animation, 3, 0.1f);
    }
    plyr_anim_pdata->step = 0.5f;
    if (plyr_pdata->character_id == 0x19) {
        play_sound_1(0x30E);
    } else if (plyr_pdata->character_id == 0x1A) {
        play_sound_1(0x310);
    } else {
        play_sound_1(0x2D4);
    }
    reaction_xfer_him_nohit(0xA4);
    ani_to_blend_frame(10.0f);
    disable_this_move_exec(0x1203, 0xA0);
    blend_to_fstance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

static float retract_spear(void) {
    MkProc* proc = MK_LIVE(plyr_pdata->spear_proc, plyr_pdata->spear_proc_instance);

    if (proc != 0) {
        xfer_proc(proc, p_sc_spear_retract);
    }
    blend_to_fstance(0.1f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

void kill_spear(void) {
    MkProc* proc = MK_LIVE(plyr_pdata->spear_proc, plyr_pdata->spear_proc_instance);

    if (proc != 0) {
        xfer_proc(proc, p_sc_spear_kill);
    }
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
    if (mode_of_play == 8 &&
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
            xfer_proc(player->idle_proc, x_block);
        }
    }
    return -1.0f;
}

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
        xfer_proc(player->idle_proc, x_attack_5);
    }
    return -1.0f;
}

float switch_proc_pickup(void) {
    PlyrInfo* player;
    MkObj* object;
    PlyrPdata* player_data;
    int pad_index;
    int can_pick_up;
    int state;

    player = switch_pdata->player;
    if (player->controller_slot == 0x2C) {
        return -1.0f;
    }
    player_data = player->slot.pdata;
    pad_index = player->pad_index;
    if ((player_data->state & 0x200) == 0 && is_pX_airborn(pad_index) == 0) {
        object = player_data->plyr_info->slot.mirror_a;
        if ((player_data->state & 0x200) != 0 ||
            is_plyr_airborn(object, player_data) == 1) {
            can_pick_up = 0;
        } else {
            can_pick_up = moves_has_nearby_pickup(
                object, &g_game_info.field_64);
        }
        if (can_pick_up != 0) {
            PlyrInfo* target = switch_pdata->player;
            if (target != 0) {
                if (target->slot.pdata->state == 0x6000) {
                    g_game_info.plyr0.slot.pdata->state = 0x6002;
                    mkproc_die();
                }
                if (target->player_state != 2 && target->player_state != 3) {
                    mkproc_die();
                }
                state = target->slot.pdata->state;
                if ((state & 0x200) != 0 && state != 0x420D) {
                    mkproc_die();
                }
                if ((target->slot.pdata->state & 0x800) != 0) {
                    mkproc_die();
                }
                if ((unsigned int)target->slot.pdata->attacks_disabled_until >
                    (unsigned int)game_tick_ctr) {
                    mkproc_die();
                }
                if (target->field_0C == 0.0f) {
                    mkproc_die();
                }
                xfer_proc(target->idle_proc, x_pickup);
            }
        }
    }
    return -1.0f;
}

float switch_proc_attack_4(void) {
    PlyrInfo* player;
    int state;

    player = switch_pdata->player;
    if (player != 0) {
        if (player->slot.pdata->state == 0x6000) {
            g_game_info.plyr0.slot.pdata->state = 0x6002;
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
        xfer_proc(player->idle_proc, x_attack_4);
    }
    return -1.0f;
}

float switch_proc_attack_3(void) {
    PlyrInfo* player;
    int state;

    player = switch_pdata->player;
    if (player != 0) {
        if (player->slot.pdata->state == 0x6000) {
            g_game_info.plyr0.slot.pdata->state = 0x6002;
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
        xfer_proc(player->idle_proc, x_attack_3);
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
        xfer_proc(player->idle_proc, x_attack_2);
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
        xfer_proc(player->idle_proc, x_attack_1);
    }
    return -1.0f;
}

void drop_active_weapon_to_original_position(PlyrPdata* player) {
    struct MovesPickup* pickup;

    if (moves_find_active_weapon(&player->active_weapon_links, &pickup) != 1) {
        return;
    }

    mk_pull_discard(&pickup->hdr, &player->active_weapon_links);
    mk_insert(&pickup->hdr, &g_game_info.field_64);
    enable_bgnd_obj_repel(&pickup->hdr);
    plyr_weapon_release(player);
    plyr_weapon2_release(player);

    if (pickup->primary_object != 0) {
        pickup->primary_object->flags_08_bits.angular_velocity_enabled = 1;
        pickup->primary_object->flags_08_bits.airborne = 1;
        pickup->primary_object->pos.value.x = pickup->primary_position.x;
        pickup->primary_object->pos.value.y = pickup->primary_position.y;
        pickup->primary_object->pos.value.z = pickup->primary_position.z;
        pickup->primary_object->ang.x = pickup->primary_angle.x;
        pickup->primary_object->ang.y = pickup->primary_angle.y;
        pickup->primary_object->ang.z = pickup->primary_angle.z;
        update_mkobj(pickup->primary_object);
        hide_obj(pickup->primary_object);

        pickup->transform_a->pos.x = pickup->primary_position.x;
        pickup->transform_a->pos.y = pickup->primary_position.y;
        pickup->transform_a->pos.z = pickup->primary_position.z;
        pickup->transform_a->ang.x = pickup->primary_angle.x;
        pickup->transform_a->ang.y = pickup->primary_angle.y;
        pickup->transform_a->ang.z = pickup->primary_angle.z;
        update_mkobj(pickup->primary_object);
        if (pickup->transform_a != 0) {
            unhide_sobj(pickup->transform_a);
        }
        update_mksobj(pickup->transform_a);
    }

    if (pickup->secondary_object != 0) {
        pickup->secondary_object->flags_08_bits.angular_velocity_enabled = 1;
        pickup->secondary_object->flags_08_bits.airborne = 1;
        pickup->secondary_object->pos.value.x = pickup->secondary_position.x;
        pickup->secondary_object->pos.value.y = pickup->secondary_position.y;
        pickup->secondary_object->pos.value.z = pickup->secondary_position.z;
        pickup->secondary_object->ang.x = pickup->secondary_angle.x;
        pickup->secondary_object->ang.y = pickup->secondary_angle.y;
        pickup->secondary_object->ang.z = pickup->secondary_angle.z;
        update_mkobj(pickup->secondary_object);
        hide_obj(pickup->secondary_object);

        pickup->transform_b->pos.x = pickup->primary_position.x;
        pickup->transform_b->pos.y = pickup->primary_position.y;
        pickup->transform_b->pos.z = pickup->primary_position.z;
        pickup->transform_b->ang.x = pickup->primary_angle.x;
        pickup->transform_b->ang.y = pickup->primary_angle.y;
        pickup->transform_b->ang.z = pickup->primary_angle.z;
        update_mkobj(pickup->secondary_object);
        if (pickup->transform_b != 0) {
            unhide_sobj(pickup->transform_b);
        }
        update_mksobj(pickup->transform_b);
    }

    if (pickup->background_sobj_id != 0) {
        unhide_sobj(obj_create_sobjs_by_id(
            g_game_info.bgnd_obj, pickup->background_sobj_id));
    }
}

static float x_pickup(void) {
    MkObj* object;
    struct MovesPickup* nearby_pickup;
    Vec offset;
    int found;

    nearby_pickup = 0;
    object = plyr_obj;
    if ((plyr_pdata->state & 0x200) != 0 ||
        is_plyr_airborn(object, plyr_pdata) == 1) {
        found = 0;
    } else {
        found = moves_find_nearby_pickup(
            object, &g_game_info.field_64,
            &nearby_pickup, &offset);
    }

    if (found == 1) {
        do_pickup(nearby_pickup, &offset, 1);
    }
    moves_jump(j_exit);
    return 0.0f;
}

static void do_pickup(struct MovesPickup* pickup, Vec* offset, int take) {
    AniData* animation;
    float angle;
    unsigned int sector;
    int flipped;
    int i;

    sector = 0;
    flipped = am_i_flipped();
    set_my_state(0x4210);
    plyr_pdata->active_pickup = pickup;
    disable_bgnd_obj_repel(&pickup->hdr);
    special_move_cam_setup(
        2.5f, 3.5f, 2.0f, -0.4f, 0.15f, 0x19, 0x46, 0);

    angle = gxMathArcTanYX(offset->x, offset->z) - plyr_obj->ang.y;
    if (angle > 0.0f) {
        angle *= -1.0f;
        flipped = flipped != 1;
    }
    if (angle < -6.2831855f) {
        angle += 6.2831855f;
    }
    if (angle > -5.7f && angle < weapon_grab_table[7].angle) {
        sector = 1;
        for (i = 1; i < 7; i++) {
            if (angle < weapon_grab_table[i].angle) {
                break;
            }
            sector++;
        }
    }

    if (flipped == 1) {
        animation = fetch_grab_anim_ptr_inline(
            weapon_grab_table[sector].flipped_grab_type);
    } else {
        animation = fetch_grab_anim_ptr_inline(
            weapon_grab_table[sector].normal_grab_type);
    }

    xfer_proc(plyr_anim_proc, p_idle);
    init_ground_move();
    tightrope_restrictions_off();
    blend_to_ani(animation, 3, 0.1f);
    if (flipped == 1) {
        set_grab_anim_weighting(
            offset, weapon_grab_table[sector].flipped_grab_type);
    } else {
        set_grab_anim_weighting(
            offset, weapon_grab_table[sector].normal_grab_type);
    }
    set_ani_speed(0.35f);
    ani_to_frame_x(weapon_grab_table[sector].animation_frame);

    if (take == 0) {
        moves_sleep(10.0f);
        if ((flipped == 1 && sector - 1 <= 1) ||
            (flipped == 0 && (sector == 7 || sector == 6))) {
            blend_to_fstance(0.05f);
        } else {
            blend_to_stance(0.05f);
        }
        return;
    }

    if (find_in_mklist(pickup != 0 ? as_mkhdr(&pickup->hdr) : 0,
                       &g_game_info.field_64) == 0) {
        if ((flipped == 1 && sector - 1 <= 1) ||
            (flipped == 0 && (sector == 7 || sector == 6))) {
            blend_to_fstance(0.05f);
        } else {
            blend_to_stance(0.05f);
        }
        return;
    }

    if (pickup->type != 2) {
        set_my_state(0x4211);
        set_ani_speed(0.35f);
        tightrope_restrictions_on();
        mk_pull_discard(&pickup->hdr, &g_game_info.field_64);
        mk_insert(&pickup->hdr, &plyr_pdata->active_weapon_links);
        switch_to_bgnd_moveset(plyr_pdata, pickup->background_moveset);
        if (pickup->background_sobj_id != 0) {
            hide_sobj(obj_create_sobjs_by_id(
                g_game_info.bgnd_obj, pickup->background_sobj_id));
        }
        if (pickup->transform_a != 0) {
            hide_sobj(pickup->transform_a);
        }
        if (pickup->transform_b != 0) {
            hide_sobj(pickup->transform_b);
        }
        set_ani_weight(1.0f);
        blend_to_stance(0.05f);
        return;
    }

    set_ani_speed(0.35f);
    tightrope_restrictions_on();
    mk_pull_discard(&pickup->hdr, &g_game_info.field_64);
    mk_insert(&pickup->hdr, &plyr_pdata->active_weapon_links);
    hide_sobj(pickup->transform_a);
    set_ani_weight(1.0f);
    if (pickup->pickup_script != 0) {
        active_cmdscript->unk28 = pickup->pickup_script;
        moves_jump(bgnd_call_script_function);
    } else {
        blend_to_stance(0.05f);
    }
}

/* TODO: [near miss] 95.80%; retail order restored; prologue guard-constant/offset-component scheduling remains. */
static void set_grab_anim_weighting(const Vec* offset, unsigned int grab_type) {
    float length;
    float weight;

    length = gxMathFastSqrt(
        offset->z * offset->z +
        (offset->x * offset->x + offset->y * offset->y));

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

static AniData* fetch_grab_anim_ptr(unsigned int grab_type) {
    switch (grab_type) {
    case 0:
        return shared_ani.grab_animations[0];
    case 1:
        return shared_ani.grab_animations[1];
    case 2:
        return shared_ani.grab_animations[2];
    case 3:
        return shared_ani.grab_animations[3];
    case 4:
        return shared_ani.grab_animations[4];
    case 5:
        return shared_ani.grab_animations[5];
    case 6:
        return shared_ani.grab_animations[6];
    case 7:
        return shared_ani.grab_animations[7];
    case 8:
        return shared_ani.grab_animations[8];
    default:
        return shared_ani.grab_animations[0];
    }
}

float x_attack_5(void) {
    unsigned int throw_script;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        p1_current_switch_bit = p1_switch_log[p1_log_index].switch_index;
        p1_current_switch_time = p1_switch_log[p1_log_index].tick;
    } else {
        p2_current_log_index = p2_log_index;
        p2_current_switch_bit = p2_switch_log[p2_log_index].switch_index;
        p2_current_switch_time = p2_switch_log[p2_log_index].tick;
    }
    trial_increment_state_value(
        plyr_pdata->plyr_num, plyr_pdata->player_slot + 8, 0);
    plyr_pdata->blocking_disabled = 1;
    if (am_i_airborn() != 0 && plyr_pdata->state == 0x6001) {
        if (am_i_a_big_character() != 0) {
            moves_jump(j_flying_kick);
            return 0.0f;
        }
        moves_jump(j_flying_kick2);
        return 0.0f;
    }

    my_joypad_state_5();
    moves_pre_attack_chores();
    throw_script = plyr_pdata->status_data->throw_script;
    if (throw_script == 0) {
        temp_throw_switch.opcode = 4;
        temp_throw_switch.argument = 0x8F;
        moves_dispatch_attack(&temp_throw_switch);
    } else {
        temp_throw_switch.opcode = 2;
        temp_throw_switch.argument = throw_script;
        moves_dispatch_attack(&temp_throw_switch);
    }
    set_my_state(0);
    blend_to_stance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

static float x_attack_5_remote(void) {
    moves_pre_attack_chores();
    if (plyr_pdata->status_data->throw_script == 0) {
        temp_throw_switch.opcode = 4;
        temp_throw_switch.argument = 0x8F;
        moves_dispatch_attack(&temp_throw_switch);
    } else if (is_big_boss(plyr_pdata) == 0) {
        unsigned int throw_script = plyr_pdata->status_data->throw_script;
        temp_throw_switch.opcode = 2;
        temp_throw_switch.argument = throw_script;
        moves_dispatch_attack(&temp_throw_switch);
    }
    set_my_state(0);
    blend_to_stance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

float x_attack_4(void) {
    AiFightstyleAttack* action;
    unsigned int* sequences;
    int joy_state;

    sequences = &jump_table[0].value;
    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        p1_current_switch_bit = p1_switch_log[p1_log_index].switch_index;
        p1_current_switch_time = p1_switch_log[p1_log_index].tick;
    } else {
        p2_current_log_index = p2_log_index;
        p2_current_switch_bit = p2_switch_log[p2_log_index].switch_index;
        p2_current_switch_time = p2_switch_log[p2_log_index].tick;
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 4, 0);
    trial_increment_state_value(
        plyr_pdata->plyr_num, plyr_pdata->player_slot + 8, 0);
    if (am_i_airborn() != 0 && plyr_pdata->state == 0x6001) {
        if (am_i_a_big_character() != 0) {
            moves_jump(j_flying_kick);
            return 0.0f;
        }
        moves_jump(j_flying_kick2);
        return 0.0f;
    }

    action = &((AiFightstyleAttackTable*)
        plyr_pdata->fighter_definition->move_blend_data)->attacks[15];
    joy_state = my_joypad_state_5();
    if (joy_state == 2) {
        init_ground_move();
        set_my_state(0x1300);
    }

    switch (plyr_pdata->character_id) {
    case 6:
        if (his_pdata->hit_count < 5 && moves_his_state_is_not(0x3203) == 1) {
            scan_switch_sequences(&sequences[0x760 / 4]);
        }
        break;
    case 28:
        scan_switch_sequences(scan_freak_4);
        break;
    case 36:
        scan_switch_sequences(&sequences[0xC68 / 4]);
        break;
    case 5:
        scan_switch_sequences(&sequences[0x6E4 / 4]);
        break;
    case 14:
        scan_switch_sequences(&sequences[0x1218 / 4]);
        break;
    case 4:
        scan_switch_sequences(&sequences[0x7F0 / 4]);
        break;
    case 7:
        scan_switch_sequences(&sequences[0x89C / 4]);
        break;
    case 11:
        scan_switch_sequences(&sequences[0x650 / 4]);
        break;
    case 24:
        scan_switch_sequences(&sequences[0x12CC / 4]);
        break;
    case 9:
        scan_switch_sequences(&sequences[0x5C8 / 4]);
        break;
    case 12:
        scan_switch_sequences(&sequences[0xA44 / 4]);
        break;
    case 3:
        scan_switch_sequences(&sequences[0xAC8 / 4]);
        break;
    case 10:
        scan_switch_sequences(&sequences[0x54C / 4]);
        break;
    case 19:
        if (his_pdata->hit_count < 2 && his_pdata->state != 0x421A) {
            scan_switch_sequences(&sequences[0xCEC / 4]);
        }
        break;
    case 20:
        scan_switch_sequences(&sequences[0xF10 / 4]);
        break;
    case 23:
        scan_switch_sequences(&sequences[0x1108 / 4]);
        break;
    case 18:
        scan_switch_sequences(&sequences[0x116C / 4]);
        break;
    case 27:
        if (plyr_pdata->sidekick_active == 1) {
            scan_switch_sequences(&sequences[0xDC8 / 4]);
        } else {
            scan_switch_sequences(&sequences[0xDFC / 4]);
        }
        break;
    case 21:
        scan_switch_sequences(&sequences[0xB60 / 4]);
        break;
    case 15:
        scan_switch_sequences(&sequences[0xFD8 / 4]);
        break;
    case 16:
        scan_switch_sequences(&sequences[0x1070 / 4]);
        break;
    case 25:
        if (his_pdata->hit_count < 5 && moves_his_state_is_not(0x3203) == 1) {
            scan_switch_sequences(&sequences[0x13A0 / 4]);
        }
        break;
    case 26:
        if (his_pdata->hit_count < 5 && moves_his_state_is_not(0x3203) == 1) {
            scan_switch_sequences(&sequences[0x13A0 / 4]);
        }
        break;
    }

    action += joy_state;
    moves_pre_attack_chores();
    if (joy_state == 2) {
        set_my_state(0x1300);
    }
    moves_dispatch_attack(action);
    return 1.0f;
}

float x_attack_3(void) {
    AiFightstyleAttack* action;
    unsigned int* sequences = &jump_table[0].value;
    int joy_state;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        p1_current_switch_bit = p1_switch_log[p1_log_index].switch_index;
        p1_current_switch_time = p1_switch_log[p1_log_index].tick;
    } else {
        p2_current_log_index = p2_log_index;
        p2_current_switch_bit = p2_switch_log[p2_log_index].switch_index;
        p2_current_switch_time = p2_switch_log[p2_log_index].tick;
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 3, 0);
    trial_increment_state_value(
        plyr_pdata->plyr_num, plyr_pdata->player_slot + 8, 0);
    plyr_pdata->blocking_disabled = 1;
    if (am_i_airborn() != 0 && plyr_pdata->state == 0x6001) {
        if (am_i_a_big_character() != 0) {
            moves_jump(j_flying_kick);
            return 0.0f;
        }
        moves_jump(j_flying_kick2);
        return 0.0f;
    }

    action = &((AiFightstyleAttackTable*)
        plyr_pdata->fighter_definition->move_blend_data)->attacks[10];
    joy_state = my_joypad_state_5();
    if (joy_state == 2) {
        init_ground_move();
        set_my_state(0x1300);
    }

    switch (plyr_pdata->character_id) {
    case 6:
        scan_switch_sequences(&sequences[0x72C / 4]);
        break;
    case 8:
        scan_switch_sequences(&sequences[0x8FC / 4]);
        break;
    case 1:
        scan_switch_sequences(&sequences[0x980 / 4]);
        break;
    case 27:
        scan_switch_sequences(&sequences[0xD9C / 4]);
        break;
    case 3:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0xAB0 / 4]);
        }
        break;
    case 0:
        scan_switch_sequences(&sequences[0xBE0 / 4]);
        break;
    case 28:
        scan_switch_sequences(&sequences[0xC3C / 4]);
        break;
    case 19:
        scan_switch_sequences(&sequences[0xCCC / 4]);
        break;
    case 14:
        scan_switch_sequences(&sequences[0x1200 / 4]);
        break;
    case 24:
        scan_switch_sequences(&sequences[0x1298 / 4]);
        break;
    case 9:
        scan_switch_sequences(&sequences[0x5B0 / 4]);
        break;
    case 12:
        scan_switch_sequences(&sequences[0xA18 / 4]);
        break;
    case 10:
        scan_switch_sequences(&sequences[0x51C / 4]);
        break;
    case 7:
        scan_switch_sequences(&sequences[0x884 / 4]);
        break;
    case 22:
        scan_switch_sequences(&sequences[0xE6C / 4]);
        break;
    case 23:
        scan_switch_sequences(&sequences[0x10F0 / 4]);
        break;
    case 18:
        scan_switch_sequences(&sequences[0x1138 / 4]);
        break;
    case 4:
        scan_switch_sequences(&sequences[0x7D0 / 4]);
        break;
    case 15:
        if (his_pdata->hit_count < 5) {
            scan_switch_sequences(&sequences[0xFC0 / 4]);
        }
        break;
    case 21:
        scan_switch_sequences(&sequences[0xB48 / 4]);
        break;
    case 16:
        scan_switch_sequences(&sequences[0x103C / 4]);
        break;
    case 29:
        scan_switch_sequences(&sequences[0x1428 / 4]);
        break;
    case 25:
        scan_switch_sequences(&sequences[0x1358 / 4]);
        break;
    case 26:
        scan_switch_sequences(&sequences[0x1358 / 4]);
        break;
    case 31:
        scan_switch_sequences(&sequences[0x14D4 / 4]);
        break;
    case 30:
        if ((his_pdata->state & 0x400) == 0) {
            scan_switch_sequences(&sequences[0x1538 / 4]);
        }
        break;
    }

    action += joy_state;
    moves_pre_attack_chores();
    if (joy_state == 2) {
        set_my_state(0x1300);
    }
    moves_dispatch_attack(action);
    return 1.0f;
}

float x_attack_2(void) {
    AiFightstyleAttack* action;
    unsigned int* sequences = &jump_table[0].value;
    int joy_state;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        p1_current_switch_bit = p1_switch_log[p1_log_index].switch_index;
        p1_current_switch_time = p1_switch_log[p1_log_index].tick;
    } else {
        p2_current_log_index = p2_log_index;
        p2_current_switch_bit = p2_switch_log[p2_log_index].switch_index;
        p2_current_switch_time = p2_switch_log[p2_log_index].tick;
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 2, 0);
    trial_increment_state_value(
        plyr_pdata->plyr_num, plyr_pdata->player_slot + 8, 0);
    if (am_i_airborn() != 0 && plyr_pdata->state == 0x6001) {
        moves_jump(j_flying_punch);
        return 0.0f;
    }

    action = &((AiFightstyleAttackTable*)
        plyr_pdata->fighter_definition->move_blend_data)->attacks[5];
    joy_state = my_joypad_state_5();
    if (joy_state == 2) {
        init_ground_move();
        set_my_state(0x1200);
    }

    switch (plyr_pdata->character_id) {
    case 11:
        scan_switch_sequences(&sequences[0x630 / 4]);
        break;
    case 6:
        scan_switch_sequences(&sequences[0x714 / 4]);
        break;
    case 16:
        scan_switch_sequences(&sequences[0x1008 / 4]);
        break;
    case 5:
        scan_switch_sequences(&sequences[0x6B0 / 4]);
        break;
    case 0:
        scan_switch_sequences(&sequences[0xBC8 / 4]);
        break;
    case 31:
        scan_switch_sequences(&sequences[0x1484 / 4]);
        break;
    case 7:
        scan_switch_sequences(&sequences[0x848 / 4]);
        break;
    case 28:
        scan_switch_sequences(&sequences[0xC24 / 4]);
        break;
    case 14:
        scan_switch_sequences(&sequences[0x11CC / 4]);
        break;
    case 24:
        scan_switch_sequences(&sequences[0x1280 / 4]);
        break;
    case 1:
        scan_switch_sequences(&sequences[0x948 / 4]);
        break;
    case 9:
        scan_switch_sequences(&sequences[0x598 / 4]);
        break;
    case 3:
        scan_switch_sequences(&sequences[0xA90 / 4]);
        break;
    case 12:
        scan_switch_sequences(&sequences[0x9C8 / 4]);
        break;
    case 25:
        scan_switch_sequences(&sequences[0x132C / 4]);
        break;
    case 26:
        scan_switch_sequences(&sequences[0x132C / 4]);
        break;
    case 27:
        if (plyr_pdata->sidekick_active == 1) {
            scan_switch_sequences(&sequences[0xD68 / 4]);
        } else {
            scan_switch_sequences(&sequences[0xD34 / 4]);
        }
        break;
    case 10:
        if ((his_pdata->state & 0x400) == 0) {
            scan_switch_sequences(&sequences[0x4E8 / 4]);
        }
        break;
    case 4:
        scan_switch_sequences(&sequences[0x7B8 / 4]);
        break;
    case 19:
        scan_switch_sequences(&sequences[0xC98 / 4]);
        break;
    case 22:
        scan_switch_sequences(&sequences[0xE54 / 4]);
        break;
    case 20:
        scan_switch_sequences(&sequences[0xEC8 / 4]);
        break;
    case 23:
        scan_switch_sequences(&sequences[0x10D8 / 4]);
        break;
    case 15:
        scan_switch_sequences(&sequences[0xF78 / 4]);
        break;
    case 21:
        scan_switch_sequences(&sequences[0xB14 / 4]);
        break;
    case 29:
        scan_switch_sequences(&sequences[0x1410 / 4]);
        break;
    case 30:
        scan_switch_sequences(&sequences[0x1520 / 4]);
        break;
    }

    action += joy_state;
    moves_pre_attack_chores();
    if (joy_state == 2) {
        set_my_state(0x1200);
    }
    moves_dispatch_attack(action);
    return 1.0f;
}

float x_attack_1(void) {
    AiFightstyleAttack* action;
    unsigned int* sequences = &jump_table[0].value;
    int joy_state;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        p1_current_switch_bit = p1_switch_log[p1_log_index].switch_index;
        p1_current_switch_time = p1_switch_log[p1_log_index].tick;
    } else {
        p2_current_log_index = p2_log_index;
        p2_current_switch_bit = p2_switch_log[p2_log_index].switch_index;
        p2_current_switch_time = p2_switch_log[p2_log_index].tick;
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 1, 0);
    trial_increment_state_value(
        plyr_pdata->plyr_num, plyr_pdata->player_slot + 8, 0);
    if (am_i_airborn() != 0 && plyr_pdata->state == 0x6001) {
        moves_jump(j_flying_punch);
        return 0.0f;
    }

    action = ((AiFightstyleAttackTable*)
        plyr_pdata->fighter_definition->move_blend_data)->attacks;
    joy_state = my_joypad_state_5();
    if (joy_state == 2) {
        init_ground_move();
        set_my_state(0x1300);
    }

    switch (plyr_pdata->character_id) {
    case 0:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0xB78 / 4]);
        }
        break;
    case 28:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0xC0C / 4]);
        }
        break;
    case 5:
        scan_switch_sequences(&sequences[0x668 / 4]);
        break;
    case 21:
        scan_switch_sequences(&sequences[0xAE0 / 4]);
        break;
    case 27:
        if (his_pdata->hit_count < 1) {
            if (plyr_pdata->sidekick_active == 1) {
                scan_switch_sequences(&sequences[0xD1C / 4]);
            } else {
                scan_switch_sequences(&sequences[0xD04 / 4]);
            }
        }
        break;
    case 14:
        scan_switch_sequences(&sequences[0x11A0 / 4]);
        break;
    case 24:
        scan_switch_sequences(&sequences[0x124C / 4]);
        break;
    case 6:
        scan_switch_sequences(&sequences[0x6FC / 4]);
        break;
    case 7:
        scan_switch_sequences(&sequences[0x81C / 4]);
        break;
    case 8:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0x8B4 / 4]);
        }
        break;
    case 1:
        scan_switch_sequences(&sequences[0x930 / 4]);
        break;
    case 12:
        if (his_pdata->hit_count < 5 && moves_his_state_is_not(0x3203) == 1) {
            scan_switch_sequences(&sequences[0x9AC / 4]);
        }
        break;
    case 3:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0xA5C / 4]);
        }
        break;
    case 11:
        if (his_pdata->hit_count < 5) {
            scan_switch_sequences(&sequences[0x5FC / 4]);
        }
        break;
    case 9:
        scan_switch_sequences(&sequences[0x564 / 4]);
        break;
    case 4:
        scan_switch_sequences(&sequences[0x798 / 4]);
        break;
    case 10:
        scan_switch_sequences(&sequences[0x4D0 / 4]);
        break;
    case 19:
        scan_switch_sequences(&sequences[0xC80 / 4]);
        break;
    case 22:
        if ((his_pdata->state & 0x400) == 0) {
            scan_switch_sequences(&sequences[0xE1C / 4]);
        }
        break;
    case 20:
        scan_switch_sequences(&sequences[0xE9C / 4]);
        break;
    case 23:
        scan_switch_sequences(&sequences[0x1088 / 4]);
        break;
    case 16:
        scan_switch_sequences(&sequences[0xFF0 / 4]);
        break;
    case 18:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0x1120 / 4]);
        }
        break;
    case 15:
        scan_switch_sequences(&sequences[0xF44 / 4]);
        break;
    case 25:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0x12E4 / 4]);
        }
        break;
    case 26:
        if (his_pdata->hit_count < 2) {
            scan_switch_sequences(&sequences[0x12E4 / 4]);
        }
        break;
    case 29:
        scan_switch_sequences(&sequences[0x13D0 / 4]);
        break;
    case 31:
        scan_switch_sequences(&sequences[0x1440 / 4]);
        if (plyr_pdata->taunts_performed < 3) {
            scan_switch_sequences(&sequences[0x146C / 4]);
        }
        break;
    case 30:
        scan_switch_sequences(&sequences[0x14EC / 4]);
        break;
    }

    action += joy_state;
    moves_pre_attack_chores();
    if (joy_state == 2) {
        set_my_state(0x1300);
    }
    moves_dispatch_attack(action);
    plyr_anim_pdata->step = 1.0f;
    return 1.0f;
}

void pre_attack_chores(void) {
    moves_pre_attack_chores();
}

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

static float do_my_suicide_remote(void) {
    f_fatality_was_done = 1;
    aproc->vtbl->jump_sleep(start_suicide, 0.0f);
    return 0.0f;
}

static float do_my_fatality_remote(void) {
    f_fatality_was_done = 1;
    aproc->vtbl->jump_sleep(start_fatality, 0.0f);
    return 0.0f;
}

static float do_my_2nd_fatality_remote(void) {
    f_fatality_was_done = 1;
    aproc->vtbl->jump_sleep(start_2nd_fatality, 0.0f);
    return 0.0f;
}

void advance_my_current_switch(void) {
    int previous_index;
    int destination;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        previous_index = p1_log_index;
        if (p1_current_log_index != previous_index) {
            p1_current_log_index++;
            if (p1_current_log_index >= 30) {
                p1_current_log_index = 0;
            }
            if (p1_current_log_index <= previous_index) {
                if (previous_index - p1_current_log_index > 1) {
                    destination = p1_current_log_index + 1;
                    p1_log_index = destination;
                    p1_switch_log[destination].joy_state =
                        p1_switch_log[previous_index].joy_state;
                    p1_switch_log[destination].label =
                        p1_switch_log[previous_index].label;
                    p1_switch_log[destination].switch_index =
                        p1_switch_log[previous_index].switch_index;
                    p1_switch_log[destination].tick =
                        p1_switch_log[previous_index].tick;
                }
            } else if (30 - p1_current_log_index + previous_index > 1) {
                p1_log_index = p1_current_log_index + 1;
                if (p1_current_log_index >= 30) {
                    destination = p1_current_log_index - 30;
                    p1_log_index = destination;
                    p1_switch_log[destination].joy_state =
                        p1_switch_log[previous_index].joy_state;
                    p1_switch_log[destination].label =
                        p1_switch_log[previous_index].label;
                    p1_switch_log[destination].switch_index =
                        p1_switch_log[previous_index].switch_index;
                    p1_switch_log[destination].tick =
                        p1_switch_log[previous_index].tick;
                }
            }
        }
        p1_current_switch_bit =
            p1_switch_log[p1_current_log_index].switch_index;
        p1_current_switch_time =
            p1_switch_log[p1_current_log_index].tick;
        return;
    }

    previous_index = p2_log_index;
    if (p2_current_log_index != previous_index) {
        p2_current_log_index++;
        if (p2_current_log_index >= 30) {
            p2_current_log_index = 0;
        }
        if (p2_current_log_index <= previous_index) {
            if (previous_index - p2_current_log_index > 1) {
                destination = p2_current_log_index + 1;
                p2_log_index = destination;
                p2_switch_log[destination].joy_state =
                    p2_switch_log[previous_index].joy_state;
                p2_switch_log[destination].label =
                    p2_switch_log[previous_index].label;
                p2_switch_log[destination].switch_index =
                    p2_switch_log[previous_index].switch_index;
                p2_switch_log[destination].tick =
                    p2_switch_log[previous_index].tick;
            }
        } else if (30 - p2_current_log_index + previous_index > 1) {
            p2_log_index = p2_current_log_index + 1;
            if (p2_current_log_index >= 30) {
                destination = p2_current_log_index - 30;
                p2_log_index = destination;
                p2_switch_log[destination].joy_state =
                    p2_switch_log[previous_index].joy_state;
                p2_switch_log[destination].label =
                    p2_switch_log[previous_index].label;
                p2_switch_log[destination].switch_index =
                    p2_switch_log[previous_index].switch_index;
                p2_switch_log[destination].tick =
                    p2_switch_log[previous_index].tick;
            }
        }
    }
    p2_current_switch_bit = p2_switch_log[p2_current_log_index].switch_index;
    p2_current_switch_time = p2_switch_log[p2_current_log_index].tick;
}

/* TODO: [near miss] 89.69%; retail order restored; shared i*20 indexed base/base+4 loads still differ. */
void update_my_last_switch(void) {
    unsigned int index;
    int switch_id;
    int switch_value;

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        index = p1_log_index;
        p1_current_log_index = index;
        switch_value = p1_switch_log[index].tick;
        switch_id = p1_switch_log[index].switch_index;
        p1_last_switch_bit = switch_id;
        p1_last_switch_time = switch_value;
        p1_current_switch_bit = switch_id;
        p1_current_switch_time = switch_value;
        return;
    }
    index = p2_log_index;
    p2_current_log_index = index;
    switch_value = p2_switch_log[index].tick;
    switch_id = p2_switch_log[index].switch_index;
    p2_last_switch_bit = switch_id;
    p2_last_switch_time = switch_value;
    p2_current_switch_bit = switch_id;
    p2_current_switch_time = switch_value;
}

/* TODO: [near miss] 98.41%; retail indexes field bases; entry staging still folds to lwz. */
static float j_flying_kick2_early(void) {
    MkHdr* object;
    struct SwitchLogEntry* entry;
    float attack_ticks;

    if ((his_pdata->state & 0x400) != 0) {
        moves_jump(j_flying_kick2);
        return 0.0f;
    }

    plyr_pdata->attack_counter++;
    plyr_pdata->pending_reaction = 0x36;
    init_air_move_no_aniproc();
    trial_increment_state_value(plyr_pdata->plyr_num, 0x16, 0);
    trial_increment_state_value(plyr_pdata->plyr_num, 5, 0);
    set_my_state(0x3200);

    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        entry = p1_switch_log;
        entry += p1_log_index;
        p1_last_switch_bit = entry->switch_index;
        p1_last_switch_time = entry->tick;
        p1_current_switch_bit = entry->switch_index;
        p1_current_switch_time = entry->tick;
    } else {
        p2_current_log_index = p2_log_index;
        entry = p2_switch_log;
        entry += p2_log_index;
        p2_last_switch_bit = entry->switch_index;
        p2_last_switch_time = entry->tick;
        p2_current_switch_bit = entry->switch_index;
        p2_current_switch_time = entry->tick;
    }

    plyr_anim_pdata->flags |= 0x40;
    attack_ticks = 20.5f;
    plyr_pdata->shared_attack_until = exec_tick_ctr + (int)attack_ticks;
    plyr_pdata->attack_counter++;
    blend_to_ani(shared_ani.flying_kick2, 0x43, 0.1f);
    plyr_anim_pdata->step = 0.5f;
    plyr_anim_pdata->weight = 1.0f;
    random_hit(7);
    ani_to_frame_x(10.0f);
    start_plyr_attack(0.0f);

    while (plyr_anim_pdata->frame < 12.0f) {
        ani_1_frame();
        if (plyr_pdata->collision_result == -1 && collision_2(8, 1.0f, 0.0f) != 0) {
            stop_me();
            set_collision_made_flag();
            reaction_xfer_him(0x36, 0.1f, 0);
        }
        moves_sleep(1.0f);
    }

    if (plyr_pdata->collision_result != -1) {
        air_collision_pause(5, 13.0f, -0.02f);
    }
    plyr_anim_pdata->step = 0.5f;
    plyr_obj->flags_08_bits.moving = 1;
    while (plyr_obj->pos.value.y > plyr_obj->ground_colls_y + 1.0f &&
           plyr_obj->flags_08_bits.moving == 1 && plyr_obj->gravity != 0.0f) {
        ani_1_frame();
        moves_sleep(1.0f);
    }

    player_feet_land_chores();
    init_ground_move();
    stop_me();
    blend_to_ani(shared_ani.flying_land, 3, 0.1f);
    plyr_anim_pdata->step = 1.33f;
    ani_to_frame_x(15.0f);
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    set_my_state(0);
    moves_jump(p_blend_to_stance_in_10);
    return 0.0f;
}

/* TODO: [near miss] 98.75%; only the inlined update_my_last_switch lwzx addressing differs; fix it there. */
float j_flying_kick2(void) {
    MkHdr* object;
    float attack_ticks;

    plyr_pdata->attack_counter++;
    plyr_pdata->pending_reaction = 0x36;
    init_air_move_no_aniproc();
    trial_increment_state_value(plyr_pdata->plyr_num, 0x16, 0);
    trial_increment_state_value(plyr_pdata->plyr_num, 5, 0);
    set_my_state(0x3200);
    update_my_last_switch();
    plyr_anim_pdata->flags |= 0x40;
    attack_ticks = 11.166667f;
    plyr_pdata->shared_attack_until = exec_tick_ctr + (int)attack_ticks;
    plyr_pdata->attack_counter++;
    blend_to_ani(shared_ani.flying_kick2, 0x43, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    plyr_anim_pdata->weight = 1.0f;
    random_hit(7);
    j_flying_kick2_airborne();
    player_feet_land_chores();
    init_ground_move();
    stop_me();
    blend_to_ani(shared_ani.flying_land, 3, 0.1f);
    plyr_anim_pdata->step = 1.33f;
    ani_to_frame_x(15.0f);
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    set_my_state(0);
    moves_jump(p_blend_to_stance_in_10);
    return 0.0f;
}

/* TODO: [near miss] 98.43%; six switch-log column loads still use element addressing instead of retail indexed field bases. */
static float j_flying_kick1_early(void) {
    MkHdr* object;
    struct SwitchLogEntry* entry;
    float attack_ticks;

    if ((his_pdata->state & 0x400) != 0) {
        moves_jump(j_flying_kick);
        return 0.0f;
    }
    plyr_pdata->attack_counter++;
    plyr_pdata->pending_reaction = 0x32;
    init_air_move_no_aniproc();
    trial_increment_state_value(plyr_pdata->plyr_num, 0x16, 0);
    trial_increment_state_value(plyr_pdata->plyr_num, 5, 0);
    set_my_state(0x3200);
    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        entry = p1_switch_log;
        entry += p1_log_index;
        p1_last_switch_bit = entry->switch_index;
        p1_last_switch_time = entry->tick;
        p1_current_switch_bit = entry->switch_index;
        p1_current_switch_time = entry->tick;
    } else {
        p2_current_log_index = p2_log_index;
        entry = p2_switch_log;
        entry += p2_log_index;
        p2_last_switch_bit = entry->switch_index;
        p2_last_switch_time = entry->tick;
        p2_current_switch_bit = entry->switch_index;
        p2_current_switch_time = entry->tick;
    }
    plyr_anim_pdata->flags |= 0x40;
    attack_ticks = 20.5f;
    plyr_pdata->shared_attack_until = exec_tick_ctr + (int)attack_ticks;
    plyr_pdata->attack_counter++;
    blend_to_ani(shared_ani.flying_kick, 0x43, 0.1f);
    plyr_anim_pdata->step = 0.5f;
    plyr_anim_pdata->weight = 1.0f;
    random_voice(0);
    random_hit(8);
    ani_to_frame_x(10.0f);
    start_plyr_attack(0.0f);
    while (plyr_anim_pdata->frame < 12.0f) {
        ani_1_frame();
        if (plyr_pdata->collision_result == -1 && collision_2(0xD, 1.0f, 0.0f) != 0) {
            stop_me();
            set_collision_made_flag();
            reaction_xfer_him(0x32, 0.1f, 0);
        }
        moves_sleep(1.0f);
    }
    if (plyr_pdata->collision_result != -1) {
        air_collision_pause(5, 13.0f, -0.02f);
    }
    plyr_anim_pdata->step = 0.5f;
    plyr_obj->flags_08_bits.moving = 1;
    while (plyr_obj->pos.value.y > plyr_obj->ground_colls_y + 1.0f &&
           plyr_obj->flags_08_bits.moving == 1 && plyr_obj->gravity != 0.0f) {
        ani_1_frame();
        moves_sleep(1.0f);
    }
    player_feet_land_chores();
    init_ground_move();
    stop_me();
    blend_to_ani(shared_ani.flying_land, 3, 0.1f);
    plyr_anim_pdata->step = 1.33f;
    ani_to_frame_x(15.0f);
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    set_my_state(0);
    moves_jump(p_blend_to_stance_in_10);
    return 0.0f;
}

/* TODO: [near miss] 98.79%; inlined update_my_last_switch needs independent base/base+4 address trees; shared helper source unresolved. */
float j_flying_kick(void) {
    MkHdr* object;
    float attack_ticks;

    plyr_pdata->attack_counter++;
    plyr_pdata->pending_reaction = 0x32;
    init_air_move_no_aniproc();
    trial_increment_state_value(plyr_pdata->plyr_num, 0x16, 0);
    trial_increment_state_value(plyr_pdata->plyr_num, 5, 0);
    set_my_state(0x3200);
    update_my_last_switch();
    plyr_anim_pdata->flags |= 0x40;
    attack_ticks = 9.833333f;
    plyr_pdata->shared_attack_until = exec_tick_ctr + (int)attack_ticks;
    plyr_pdata->attack_counter++;
    blend_to_ani(shared_ani.flying_kick, 0x43, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    plyr_anim_pdata->weight = 1.0f;
    random_voice(0);
    random_hit(8);
    j_flying_kick_airborne();
    player_feet_land_chores();
    init_ground_move();
    stop_me();
    blend_to_ani(shared_ani.flying_land, 3, 0.1f);
    plyr_anim_pdata->step = 1.33f;
    ani_to_frame_x(15.0f);
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    set_my_state(0);
    moves_jump(p_blend_to_stance_in_10);
    return 0.0f;
}

/* TODO: [near miss] 98.43%; real log-base/index staging restored; retail field-base
 * indexed loads differ from folded entry loads; stop without alias/layout evidence. */
static float j_flying_punch_early(void) {
    MkHdr* object;
    struct SwitchLogEntry* entry;
    float attack_ticks;

    if ((his_pdata->state & 0x400) != 0) {
        moves_jump(j_flying_punch);
        return 0.0f;
    }
    plyr_pdata->attack_counter++;
    plyr_pdata->pending_reaction = 0x2B;
    init_air_move_no_aniproc();
    trial_increment_state_value(plyr_pdata->plyr_num, 0x16, 0);
    trial_increment_state_value(plyr_pdata->plyr_num, 5, 0);
    set_my_state(0x3200);
    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        p1_current_log_index = p1_log_index;
        entry = p1_switch_log;
        entry += p1_log_index;
        p1_last_switch_bit = entry->switch_index;
        p1_last_switch_time = entry->tick;
        p1_current_switch_bit = entry->switch_index;
        p1_current_switch_time = entry->tick;
    } else {
        p2_current_log_index = p2_log_index;
        entry = p2_switch_log;
        entry += p2_log_index;
        p2_last_switch_bit = entry->switch_index;
        p2_last_switch_time = entry->tick;
        p2_current_switch_bit = entry->switch_index;
        p2_current_switch_time = entry->tick;
    }
    plyr_anim_pdata->flags |= 0x40;
    attack_ticks = 16.5f;
    plyr_pdata->shared_attack_until = exec_tick_ctr + (int)attack_ticks;
    plyr_pdata->attack_counter++;
    blend_to_ani(shared_ani.flying_punch, 0x43, 0.1f);
    plyr_anim_pdata->step = 0.5f;
    plyr_anim_pdata->weight = 1.0f;
    random_voice(0);
    random_hit(7);
    ani_to_frame_x(8.0f);
    start_plyr_attack(0.0f);
    while (plyr_anim_pdata->frame < 11.0f) {
        ani_1_frame();
        if (plyr_pdata->collision_result == -1 && collision_2(7, 1.0f, 0.0f) != 0) {
            stop_me();
            set_collision_made_flag();
            reaction_xfer_him(0x2B, 0.1f, 0);
        }
        moves_sleep(1.0f);
    }
    if (plyr_pdata->collision_result != -1) {
        air_collision_pause(5, 12.0f, -0.01f);
    }
    plyr_anim_pdata->step = 0.5f;
    plyr_obj->flags_08_bits.moving = 1;
    while (plyr_obj->pos.value.y > plyr_obj->ground_colls_y + 1.0f &&
           plyr_obj->flags_08_bits.moving == 1 && plyr_obj->gravity != 0.0f) {
        ani_1_frame();
        moves_sleep(1.0f);
    }
    player_feet_land_chores();
    init_ground_move();
    stop_me();
    blend_to_ani(shared_ani.flying_land, 3, 0.1f);
    plyr_anim_pdata->step = 1.33f;
    ani_to_frame_x(15.0f);
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    set_my_state(0);
    moves_jump(p_blend_to_stance_in_10);
    return 0.0f;
}

/* TODO: [near miss] 98.84%; airborne landing CFG aligns; typed input-log indexed loads remain. */
static float j_flying_punch(void) {
    MkHdr* object;
    int log_index;
    int switch_time;
    int switch_bit;
    float attack_ticks = 9.833333f;

    plyr_pdata->attack_counter++;
    plyr_pdata->pending_reaction = 0x2B;
    init_air_move_no_aniproc();
    trial_increment_state_value(plyr_pdata->plyr_num, 0x16, 0);
    trial_increment_state_value(plyr_pdata->plyr_num, 5, 0);
    set_my_state(0x3200);
    if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
        log_index = p1_log_index;
        p1_current_log_index = log_index;
        switch_bit = p1_switch_log[log_index].switch_index;
        switch_time = p1_switch_log[log_index].tick;
        p1_last_switch_bit = switch_bit;
        p1_last_switch_time = switch_time;
        p1_current_switch_bit = switch_bit;
        p1_current_switch_time = switch_time;
    } else {
        log_index = p2_log_index;
        p2_current_log_index = log_index;
        switch_bit = p2_switch_log[log_index].switch_index;
        switch_time = p2_switch_log[log_index].tick;
        p2_last_switch_bit = switch_bit;
        p2_last_switch_time = switch_time;
        p2_current_switch_bit = switch_bit;
        p2_current_switch_time = switch_time;
    }
    plyr_pdata->shared_attack_until = exec_tick_ctr + (int)attack_ticks;
    plyr_pdata->attack_counter++;
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(shared_ani.flying_punch, 0x43, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    plyr_anim_pdata->weight = 1.0f;
    random_voice(0);
    random_hit(7);
    while (plyr_anim_pdata->frame < 7.0f) {
        ani_1_frame();
        if (plyr_obj->gravity == 0.0) {
            break;
        }
        moves_sleep(1.0f);
    }
    start_plyr_attack(0.0f);
    flying_punch_airborne();
    player_feet_land_chores();
    init_ground_move();
    stop_me();
    blend_to_ani(shared_ani.flying_land, 3, 0.1f);
    plyr_anim_pdata->step = 1.33f;
    ani_to_frame_x(15.0f);
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    set_my_state(0);
    moves_jump(p_blend_to_stance_in_10);
    return 0.0f;
}

float j_duck_block_loop(void) {
    PlyrPdata* opponent_block;
    int opponent_state;
    unsigned int timeout;
    int requirement;

    timeout = exec_tick_ctr + 120;
    if (plyr_pdata->block_hit_count > 3) {
        plyr_pdata->block_hit_count--;
    } else {
        plyr_pdata->block_hit_count = 0;
    }
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_anim_pdata->step = 0.5f;

    while (am_i_duck_blocking() != 0) {
        init_ground_move();
        set_my_state(0x900);
        nudge_towards_him(0.2f);
        moves_sleep(1.0f);
        if (plyr_pdata->his_attack_counter != get_his_attack_counter()) {
            plyr_pdata->combo_depth++;
        }

        if (plyr_pdata->drone_request != 0) {
            if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
                opponent_state = g_game_info.plyr1.slot.pdata->state;
            } else {
                opponent_state = g_game_info.plyr0.slot.pdata->state;
            }
            if (((opponent_state & 0x1000) == 0 ||
                 his_pdata->throw_restriction == 3) &&
                (unsigned int)g_min_time_in_block_for_drone <
                    (unsigned int)exec_tick_ctr &&
                (plyr_pdata->his_plyr_pdata)
                        ->duck_reaction_active == 0) {
                set_my_state(0x101);
                plyr_pdata->field_6DC = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return 0.0f;
            }
            if (timeout < (unsigned int)exec_tick_ctr) {
                opponent_block =
                    plyr_pdata->his_plyr_pdata;
                opponent_block->duck_reaction_active = 0;
                plyr_pdata->field_6DC = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return 0.0f;
            }
            requirement = his_pdata->block_requirement;
            if (requirement == 0 || requirement == 7) {
                break;
            }
        }
    }

    if (check_switch(plyr_pdata->controller_port, 0xE) != 0 &&
        plyr_pdata->drone_request == 0) {
        set_my_state(0x101);
    } else {
        if ((check_switch(plyr_pdata->controller_port, 1) != 0 &&
             plyr_pdata->drone_request == 0) ||
            (plyr_pdata->drone_request == 1 &&
             ((requirement = his_pdata->block_requirement) == 0 ||
              requirement == 7))) {
            random_hit(7);
            random_voice(9);
            init_ground_move();
            if (should_i_weapon_block() != 0) {
                moves_jump(weapon_block);
                return 0.0f;
            }
            moves_jump(block_a_intro_glitch);
            return 0.0f;
        }
    }

    blend_to_stance(0.1f);
    trial_clear_provision();
    moves_jump(j_exit);
    return 0.0f;
}

float j_block_loop(void) {
    PlyrMoveBlendData* style_data;
    unsigned int timeout;
    int changed_without_weapon;
    int has_weapon;
    int opponent_state;

    timeout = exec_tick_ctr + 120;
    plyr_pdata->block_hit_count++;
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_anim_pdata->step = 1.0f;
    while (am_i_blocking() != 0) {
        nudge_towards_him(0.2f);
        moves_sleep(1.0f);

        if (plyr_pdata->drone_request != 0) {
            if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
                opponent_state = g_game_info.plyr1.slot.pdata->state;
            } else {
                opponent_state = g_game_info.plyr0.slot.pdata->state;
            }
            if (((opponent_state & 0x1000) != 0 &&
                 his_pdata->throw_restriction != 3) ||
                (his_pdata->secondary_state & 0x100) != 0 ||
                plyr_pdata->his_plyr_pdata->duck_reaction_active != 0) {
                g_min_time_in_block_for_drone =
                    exec_tick_ctr + drone_ai_get_min_time_in_block();
            }
            if ((unsigned int)g_min_time_in_block_for_drone <
                (unsigned int)exec_tick_ctr) {
                if (plyr_pdata->state == 0xA00) {
                    plyr_pdata->combo_depth = 0;
                }
                plyr_pdata->field_6DC = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return 0.0f;
            }
            if (timeout < (unsigned int)exec_tick_ctr) {
                if (plyr_pdata->state == 0xA00) {
                    plyr_pdata->combo_depth = 0;
                }
                (plyr_pdata->his_plyr_pdata)
                    ->duck_reaction_active = 0;
                plyr_pdata->field_6DC = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return 0.0f;
            }
        }

        if (plyr_pdata->his_attack_counter != get_his_attack_counter()) {
            plyr_pdata->combo_depth++;
        }
        changed_without_weapon = 0;
        if (plyr_pdata->his_attack_counter != get_his_attack_counter()) {
            if (plyr_pdata->fighter_definition != 0 &&
                (style_data = plyr_pdata->fighter_definition->move_blend_data) != 0 &&
                (style_data->primary_weapon != 0 ||
                 style_data->secondary_weapon != 0)) {
                has_weapon = 1;
            } else {
                has_weapon = 0;
            }
            if (has_weapon == 0 && is_big_boss(plyr_pdata) == 0) {
                changed_without_weapon = 1;
            }
        }
        if (changed_without_weapon != 0) {
            timeout = exec_tick_ctr + 90;
            if (drone_ai_check_next_block_state(exec_tick_ctr) == 1) {
                plyr_pdata->field_6DC = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return 0.0f;
            }
            switch (plyr_pdata->state) {
            case 0xA00:
                moves_transition_block(
                    0xA01, &shared_ani.block_b_intro,
                    &shared_ani.block_b_loop);
                break;
            case 0xA01:
                moves_transition_block(
                    0xA02, &shared_ani.block_c_intro,
                    &shared_ani.block_c_loop);
                break;
            case 0xA02:
                moves_transition_block(
                    0xA03, &shared_ani.block_d_intro,
                    &shared_ani.block_d_loop);
                break;
            case 0xA03:
                moves_transition_block(
                    0xA00, &shared_ani.block_a_intro,
                    &shared_ani.block_loop);
                break;
            }
        } else if (plyr_pdata->his_attack_counter !=
                   get_his_attack_counter()) {
            plyr_pdata->his_attack_counter = get_his_attack_counter();
            timeout = exec_tick_ctr + 90;
            if (drone_ai_check_next_block_state(exec_tick_ctr) == 1) {
                plyr_pdata->field_6DC = exec_tick_ctr;
                moves_jump(drone_blocking_done);
                return 0.0f;
            }
        }

        if ((check_switch(plyr_pdata->controller_port, 0xE) != 0 &&
             plyr_pdata->drone_request == 0) ||
            (plyr_pdata->drone_request == 1 &&
             (his_pdata->block_requirement == 1 ||
              his_pdata->block_requirement == 8) &&
             g_drone_faked_out == 0)) {
            random_hit(7);
            random_voice(9);
            init_ground_move();
            set_my_state(0x901);
            moves_jump(blend_to_duck_block);
            return 0.0f;
        }
    }

    set_my_state(0);
    blend_to_stance(0.1f);
    trial_clear_provision();
    moves_jump(j_exit);
    return 0.0f;
}

float x_block(void) {
    init_ground_move_no_aniproc();
    stop_me();
    random_hit(7);
    dead_liukang_snd_chain_check(plyr_pdata, 4, 8, 0x32);
    if ((unsigned int)(exec_tick_ctr - plyr_pdata->field_6DC) > 10U) {
        plyr_pdata->combo_depth = 0;
    }
    random_voice(9);
    g_min_time_in_block_for_drone =
        exec_tick_ctr + drone_ai_get_min_time_in_block();
    g_drone_faked_out = 0;

    if (plyr_pdata->drone_request == 1 &&
        (his_pdata->block_requirement == 1 ||
         his_pdata->block_requirement == 5 ||
         his_pdata->block_requirement == 8)) {
        if (g_drone_blocking_in_reaction == 0 &&
            drone_ai_check_block_fakeout() == 1) {
            g_drone_faked_out = 1;
        }
        if (plyr_pdata->block_hit_count > 10) {
            plyr_pdata->block_hit_count = plyr_pdata->block_hit_count - 10;
        } else {
            plyr_pdata->block_hit_count = 0;
        }
    }

    g_drone_blocking_in_reaction = 0;
    if ((check_switch(plyr_pdata->controller_port, 0xE) != 0 &&
         plyr_pdata->drone_request == 0) ||
        ((his_pdata->block_requirement == 1 ||
          his_pdata->block_requirement == 5 ||
          his_pdata->block_requirement == 8) &&
         plyr_pdata->drone_request == 1 && g_drone_faked_out == 0)) {
        moves_jump(blend_to_duck_block);
        return 0.0f;
    }

    set_my_state(0xA00);
    if (should_i_weapon_block() != 0) {
        moves_jump(weapon_block);
        return 0.0f;
    }
    moves_jump(block_a_intro);
    return 0.0f;
}

static float block_d(void) {
    MOVES_BLOCK_BODY(0xA03, shared_ani.block_d_intro, shared_ani.block_d_loop);
    return 0.0f;
}

static float block_c(void) {
    MOVES_BLOCK_BODY(0xA02, shared_ani.block_c_intro, shared_ani.block_c_loop);
    return 0.0f;
}

static float block_b(void) {
    MOVES_BLOCK_BODY(0xA01, shared_ani.block_b_intro, shared_ani.block_b_loop);
    return 0.0f;
}

static float block_a(void) {
    MOVES_BLOCK_BODY(0xA00, shared_ani.block_a_intro, shared_ani.block_loop);
    return 0.0f;
}

static float block_a_intro_glitch(void) {
    set_my_state(0xA00);
    blend_to_ani(shared_ani.block_intro, 3, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    glitch_to_ani(shared_ani.block_loop, 0);
    g_min_time_in_block_for_drone =
        exec_tick_ctr + drone_ai_get_min_time_in_block();
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    aproc->vtbl->jump_sleep(j_block_loop, 0.0f);
    return 0.0f;
}

static float block_a_intro(void) {
    set_my_state(0xA00);
    blend_to_ani(shared_ani.block_intro, 3, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    blend_to_ani(shared_ani.block_loop, 0, 0.5f);
    g_min_time_in_block_for_drone =
        exec_tick_ctr + drone_ai_get_min_time_in_block();
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    moves_jump(j_block_loop);
    return 0.0f;
}

static float weapon_block(void) {
    set_my_state(0xA00);
    blend_to_ani(
        plyr_pdata->fighter_definition->weapon_block_animation,
        3, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    glitch_to_ani(
        (plyr_pdata->fighter_definition)
            ->weapon_block_loop,
        0);
    moves_jump(j_block_loop);
    return 0.0f;
}

/* TODO: [near miss] 97.07%; retail order restored; player argument staging around the second weapon latch remains. */
void advance_active_moveset(PlyrPdata* player) {
    PlyrMirrorSlots* slots;
    MkObj* sidekick;

    trial_increment_state_value(player->plyr_num, 0x10, 0);
    trial_register_attack(
        player->plyr_num, player->player_slot, 0x63);
    drop_active_weapon_to_original_position(player);
    if (player->player_slot >= 0 &&
        moves_is_weapon_style(player->fighter_definition) != 0) {
        plyr_weapon_hide(player, 0, player->mirror_slots);
    }

    player->player_slot++;
    if (player->player_slot >= 3) {
        if (player->character_id == 0x1B) {
            if (g_game_info.feature_flags.bits.high_bit != 0) {
                player->player_slot = 0;
                sidekick = moves_resolve_weapon_latch(
                    &player->tracked_obj_latch);
                if (sidekick != 0) {
                    player->sidekick_active = 0;
                    tag_team_activate_player(
                        sidekick, player->sidekick_active == 0);
                    select_fighter_voice_in_bank(
                        player->plyr_num, player->sidekick_active);
                }
            } else if (player->sidekick_active == 1) {
                player->player_slot = 1;
            } else {
                player->player_slot = 0;
            }
        } else {
            player->player_slot = 0;
        }
    }
    if (player->character_id == 0x1B && player->player_slot >= 2) {
        player->player_slot = 0;
    }

    player->fighter_definition = player->weapon_styles[player->player_slot];
    if (player->fighter_definition->move_blend_data == 0) {
        player->player_slot++;
        if (player->player_slot >= 3) {
            player->player_slot = 0;
        }
        player->fighter_definition = player->weapon_styles[player->player_slot];
        if (player->fighter_definition->move_blend_data == 0) {
            player->player_slot = 0;
            player->fighter_definition =
                player->weapon_styles[player->player_slot];
        }
    }

    player->active_move_display =
        player->fighter_definition->move_blend_data;
    player->mirror_slots = &player->fighter_definition->mirror_slots;
    slots = player->mirror_slots;
    if (slots != 0) {
        if (moves_is_weapon_style(player->fighter_definition) != 0) {
            if (moves_resolve_weapon_latch(&slots->weapon[0].primary) != 0) {
                plyr_weapon_grab(
                    player, moves_resolve_weapon_latch(
                                &slots->weapon[0].primary));
            }
            if (moves_resolve_weapon_latch(&slots->weapon[1].primary) != 0) {
                plyr_weapon2_grab(
                    player, moves_resolve_weapon_latch(
                                &slots->weapon[1].primary));
            }
            if (moves_resolve_weapon_latch(&slots->weapon[2].primary) != 0) {
                plyr_weapon3_grab(
                    player, moves_resolve_weapon_latch(
                                &slots->weapon[2].primary));
            }
            if (moves_resolve_weapon_latch(&slots->weapon[3].primary) != 0) {
                plyr_weapon4_grab(
                    player, moves_resolve_weapon_latch(
                                &slots->weapon[3].primary));
            }
            plyr_weapon_show(player, 1, slots);
            if (player->baraka_moveset_callback != 0) {
                player->baraka_moveset_callback(player, slots);
            }
        }
        if (player->baraka_moveset_callback != 0) {
            player->baraka_moveset_callback(player, slots);
        }
    }
    show_fighting_style(player->fighter_definition, player->plyr_num);
}

void sidekick_intro_check(void) {
    MkProc* proc;
    union MovesSidekickPdataRef pdata;

    if (g_game_info.plyr0.slot.pdata->sidekick_available != 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_intro,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
        if (proc != 0 && pdata.hdr != 0) {
            pdata.sidekick->player = g_game_info.plyr0.slot.pdata;
        }
    }

    if (g_game_info.plyr1.slot.pdata->sidekick_available != 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_intro,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
        if (proc != 0 && pdata.hdr != 0) {
            pdata.sidekick->player = g_game_info.plyr1.slot.pdata;
        }
    }
}

/* TODO: [breakthrough] 94.55%; restored live mirror angle/yaw reloads; defined vector Y zeros and pointer/FPR lifetimes remain. */
static float p_plyr_sidekick_intro(void) {
    union {
        float f;
        unsigned int u;
    } inverse_bits;
    PlyrPdata* player;
    PlyrInfo* player_info;
    MkProc* anim_proc;
    MkProc* player_proc;
    MkObj* sidekick;
    MkObj* main_object;
    MkObj* angle_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec direction;
    Vec lateral;
    Vec offset;
    float length_sq;
    float inverse_length;
    float estimate_product;
    float correction;
    float position_x;
    float position_y;
    float position_z;
    float angle_x;
    float angle_y;
    float angle_z;
    float wrapped_angle;
    int function;

    player = ((struct MovesSidekickPdata*)apdata)->player;
    sidekick = player->sidekick_obj;
    if (sidekick != 0) {
        sidekick = sidekick->hdr.instance ==
                           player->sidekick_instance
                       ? sidekick
                       : 0;
    } else {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    player_info = player->plyr_info;
    opponent_object = player->his_obj;
    main_object = player_info->slot.mirror_a;
    if (anim_proc != 0) {
        if (anim_proc->instance != player->sidekick_anim_instance) {
            anim_proc = 0;
        }
    } else {
        anim_proc = 0;
    }
    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    sidekick->flags_09_bits.head_tracking = 1;

    player_proc = player->own_player_proc;
    if (player_proc != 0) {
        if (player_proc->instance != player->own_player_proc_instance) {
            player_proc = 0;
        }
    } else {
        player_proc = 0;
    }
    get_cmdscript_for_proc(player_proc);
    tag_team_activate_player(
        sidekick, player_info->slot.pdata->sidekick_active);
    angle_object = player_info->slot.mirror_a;
    direction.x = main_object->pos.value.x - opponent_object->pos.value.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.value.z - opponent_object->pos.value.z;
    angle_x = angle_object->ang.x;
    angle_y = angle_object->ang.y;
    angle_z = angle_object->ang.z;

    length_sq = direction.x * direction.x + direction.z * direction.z;
    if (length_sq <= 0.0f) {
        inverse_length = 0.0f;
    } else {
        inverse_bits.f = length_sq;
        inverse_bits.u = 0x5F375A00U - (inverse_bits.u >> 1);
        estimate_product = inverse_bits.f * (length_sq * inverse_bits.f);
        correction = 3.0f - estimate_product;
        inverse_length =
            0.0625f * inverse_bits.f * correction *
            -((correction * (estimate_product * correction)) - 12.0f);
    }
    direction.x *= inverse_length;
    direction.z *= inverse_length;
    offset.x = 0.35f * direction.x;
    offset.y = 0.0f;
    offset.z = 0.35f * direction.z;
    lateral.x = direction.z;
    lateral.y = 0.0f;
    lateral.z = -direction.x;
    if (am_i_on_the_left2(
            player->plyr_info->slot.mirror_a,
            player->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        v3_add_v3_scaled(&offset, &offset, &lateral, 0.55f);
    } else {
        v3_add_v3_scaled(&offset, &offset, &lateral, -0.55f);
    }
    position_x = main_object->pos.value.x + offset.x;
    position_y = g_game_info.field_34;
    position_z = main_object->pos.value.z + offset.z;

    sidekick->ground_colls_y = main_object->ground_colls_y;
    set_root_and_obj_movement_weights(anim, 0.0f, 0.5f);
    sidekick->pos.value.x = position_x;
    sidekick->pos.value.y = position_y;
    sidekick->pos.value.z = position_z;
    sidekick->ang.x = angle_x;
    sidekick->ang.y = angle_y;
    sidekick->ang.z = angle_z;
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    ground_me(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);

    if (player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    if (am_i_on_the_left2(
            player->plyr_info->slot.mirror_a,
            player->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        transition_to_anim_script(
            anim, player->weapon_styles[1]->duck_exit_animation, 0, 0.1f);
    } else {
        transition_to_anim_script(
            anim, player->weapon_styles[1]->duck_exit_animation, 8, 0.1f);
    }
    anim->step = 1.0f;
    moves_sleep(1.0f);
    unhide_obj(sidekick);
    wrapped_angle =
        0.000005992112f *
        (float)(((int)(166886.1f * player->plyr_info->slot.mirror_a->ang.y)) & 0xFFFFF);
    gxMathSin(wrapped_angle);
    gxMathCos(wrapped_angle);
    moves_sleep(120.0f + (float)(unsigned short)randu0(30));
    sidekick->flags_09_bits.head_tracking = 0;
    if (am_i_on_the_left2(
            player->plyr_info->slot.mirror_a,
            player->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        transition_to_anim_script(
            anim, player->base_animations[0], 3, 0.25f);
    } else {
        transition_to_anim_script(
            anim, player->base_animations[0], 0xB, 0.25f);
    }
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
    function = get_script_function_by_name(player->cmo, "sidekick_intro_exit");
    plyr_start_script_in_plyr_pdata_proc(player, 0xC025, function);
    moves_sleep(30.0f);
    obj_set_gravity(sidekick, 0.0f);
    sidekick->flags_08_bits.moving = 0;
    hide_obj(sidekick);
    return -1.0f;
}

void noobsmoke_sidekick_projectile(void) {
    MkProc* proc;
    union MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_projectile,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_projectile,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

/* TODO: [breakthrough] 80.28%; sqrt byte-offset indexing corrected;
 * audit the remaining consumer CFG/ABI differences separately. */
static float p_plyr_sidekick_projectile(void) {
    union {
        float f;
        unsigned int u;
    } bits, guess, inverse_bits;
    struct MovesSidekickPdata* pdata;
    PlyrPdata* player;
    PlyrPdata* actions;
    MkProc* anim_proc;
    MkProc* player_proc;
    MkObj* sidekick;
    MkObj* main_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec direction;
    float length_sq;
    float distance;
    float inverse_length;
    float estimate_product;
    float correction;
    float placement_scale;
    float object_weight;
    float wrapped_angle;
    float position_x;
    float position_y;
    float position_z;
    float angle_x;
    float angle_y;
    float angle_z;
    int transition;
    int function;

    pdata = (struct MovesSidekickPdata*)apdata;
    player = pdata->player;
    actions = pdata->player;
    sidekick = player->sidekick_obj;
    object_weight = 2.0f;
    transition = 0;
    if (sidekick != 0 &&
        sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != player->sidekick_anim_instance) {
        anim_proc = 0;
    }
    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    main_object = player->plyr_info->slot.mirror_a;
    opponent_object = player->his_obj;

    direction.x = main_object->pos.value.x - opponent_object->pos.value.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.value.z - opponent_object->pos.value.z;
    length_sq =
        direction.x * direction.x + direction.z * direction.z;
    distance = 0.0f;
    if (length_sq > 0.0f) {
        bits.f = length_sq;
        guess.u =
            (unsigned int)GXMathSqrtTable[(bits.u >> 11) & 0x1FFF] << 8;
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
    moves_prepare_sidekick_entrance(anim, sidekick);
    if (am_i_on_the_left2(
            main_object,
            player->his_plyr_pdata->plyr_info->slot.mirror_a) == 0) {
        transition = 8;
    }

    player_proc = player->own_player_proc;
    if (player_proc != 0 &&
        player_proc->instance != player->own_player_proc_instance) {
        player_proc = 0;
    }
    get_cmdscript_for_proc(player_proc);
    tag_team_activate_player(
        sidekick, player->plyr_info->slot.pdata->sidekick_active);

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
    position_x = main_object->pos.value.x + direction.x * placement_scale;
    position_y = g_game_info.field_34;
    position_z = main_object->pos.value.z + direction.z * placement_scale;
    angle_x = main_object->ang.x;
    angle_y = main_object->ang.y;
    angle_z = main_object->ang.z;
    sidekick->ground_colls_y = main_object->ground_colls_y;
    set_root_and_obj_movement_weights(anim, 0.0f, object_weight);
    sidekick->pos.value.x = position_x;
    sidekick->pos.value.y = position_y;
    sidekick->pos.value.z = position_z;
    sidekick->ang.x = angle_x;
    sidekick->ang.y = angle_y;
    sidekick->ang.z = angle_z;
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    ground_me(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);

    if (pdata->player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    set_anim_script(anim, actions->base_animations[7], transition);
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
    pdata->player->state_flags.bits.projectile_request = 1;
    moves_sleep(1.0f);
    pdata->player->state_flags.bits.projectile_request = 1;
    moves_sleep(1.0f);
    pdata->player->state_flags.bits.projectile_request = 1;
    moves_sleep(1.0f);
    pdata->player->state_flags.bits.projectile_request = 1;
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

int noobsmoke_fire_projectile_request(void) {
    unsigned char flags = plyr_pdata->state_flags.raw;
    int requested = flags & 1;

    plyr_pdata->state_flags.bits.projectile_request = 0;
    return requested;
}

void smoke_victory_entrance(void) {
    MkProc* proc;
    union MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_smoke_entrance,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_smoke_entrance,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

static float p_plyr_smoke_entrance(void) {
    struct MovesSidekickPdata* pdata;
    PlyrInfo* plyr_info;
    MkObj* sidekick;
    PlyrPdata* player;
    MkObj* main_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec lateral;
    Vec offset;
    Vec velocity = {0.0f, 0.0f, 0.0f};
    float direction_x;
    float direction_z;
    float inverse_length;
    float position_x;
    float position_y;
    float position_z;
    float angle_x;
    float angle_y;
    float angle_z;
    int transition;

    pdata = (struct MovesSidekickPdata*)apdata;
    player = pdata->player;
    sidekick = MK_HDR_LIVE(player->sidekick_obj, player->sidekick_instance);
    plyr_info = player->plyr_info;
    opponent_object = player->his_obj;
    main_object = plyr_info->slot.mirror_a;
    anim = (AnimPdata*)pdata_of_proc(MK_LIVE(player->sidekick_anim_proc, player->sidekick_anim_instance));
    moves_prepare_sidekick_entrance(anim, sidekick);
    sidekick->flags_09_bits.head_tracking = 0;

    get_cmdscript_for_proc(MK_LIVE(player->own_player_proc, player->own_player_proc_instance));
    tag_team_activate_player(sidekick, plyr_info->slot.pdata->sidekick_active);
    direction_z = main_object->pos.value.z - opponent_object->pos.value.z;
    direction_x = main_object->pos.value.x - opponent_object->pos.value.x;
    angle_x = plyr_info->slot.mirror_a->ang.x;
    angle_y = plyr_info->slot.mirror_a->ang.y;
    angle_z = plyr_info->slot.mirror_a->ang.z;

    inverse_length =
        moves_inverse_sqrt(direction_x * direction_x + direction_z * direction_z);
    direction_x *= inverse_length;
    direction_z *= inverse_length;
    offset.x = -0.2f * direction_x;
    offset.z = -0.2f * direction_z;
    lateral.x = direction_z;
    lateral.z = -direction_x;
    if (am_i_on_the_left2(
            player->plyr_info->slot.mirror_a,
            player->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        v3_add_v3_scaled(&offset, &offset, &lateral, -0.7f);
        transition = 0;
    } else {
        v3_add_v3_scaled(&offset, &offset, &lateral, 0.7f);
        transition = 8;
    }
    offset.x += main_object->pos.value.x;
    position_x = offset.x;
    offset.z += main_object->pos.value.z;
    position_z = offset.z;

    sidekick->flags_09_bits.bit6 = 0;
    sidekick->flags_09_bits.launched = 0;
    position_y = g_game_info.field_34 - 2.5f;
    sidekick->ground_colls_y = main_object->ground_colls_y;
    velocity.y = 0.06f;
    sidekick->pos_vel.x = velocity.x;
    sidekick->pos_vel.y = velocity.y;
    sidekick->pos_vel.z = velocity.z;
    set_root_and_obj_movement_weights(anim, 0.0f, 1.0f);
    sidekick->pos.value.x = position_x;
    sidekick->pos.value.y = position_y;
    sidekick->pos.value.z = position_z;
    sidekick->ang.x = angle_x;
    sidekick->ang.y = angle_y;
    sidekick->ang.z = angle_z;
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);

    if (player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    snd_req(0x333);
    set_anim_script(
        anim, player->turn_to_screen_animation, transition | 0x40);
    moves_sleep(1.0f);
    unhide_obj(sidekick);
    anim->step = 1.0f;
    while (sidekick->pos.value.y < g_game_info.field_34 + 1.3f) {
        moves_sleep(1.0f);
    }
    obj_set_gravity(sidekick, -0.006f);
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    ground_me(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    transition_to_anim_script(
        anim, player->smoke_land_animation, transition, 0.05f);
    moves_sleep(24.0f);
    sidekick->flags_09_bits.bit6 = 1;
    random_foot(2);
    shake_camera(2, 0.02f);
    anim->step = 0.6f;
    moves_sleep(10000.0f);
    return -1.0f;
}

void noob_victory_entrance(void) {
    MkProc* proc;
    union MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_noob_entrance,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_noob_entrance,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

static float p_plyr_noob_entrance(void) {
    struct MovesSidekickPdata* pdata;
    PlyrPdata* player;
    PlyrInfo* info;
    MkObj* sidekick;
    MkObj* main_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec lateral;
    Vec offset;
    float direction_x;
    float direction_z;
    float length_sq;
    float inverse_length;
    float position_x;
    float position_y;
    float position_z;
    float angle_x;
    float angle_y;
    float angle_z;
    int transition;

    pdata = (struct MovesSidekickPdata*)apdata;
    player = pdata->player;
    sidekick = MK_HDR_LIVE(player->sidekick_obj, player->sidekick_instance);
    info = player->plyr_info;
    opponent_object = player->his_obj;
    main_object = info->slot.mirror_a;
    anim = (AnimPdata*)pdata_of_proc(MK_LIVE(player->sidekick_anim_proc, player->sidekick_anim_instance));
    moves_prepare_sidekick_entrance(anim, sidekick);
    moves_prepare_sidekick_entrance(anim, sidekick);

    get_cmdscript_for_proc(MK_LIVE(player->own_player_proc,
                                           player->own_player_proc_instance));
    tag_team_activate_player(sidekick, info->slot.pdata->sidekick_active);
    direction_z = main_object->pos.value.z - opponent_object->pos.value.z;
    direction_x = main_object->pos.value.x - opponent_object->pos.value.x;
    angle_x = info->slot.mirror_a->ang.x;
    angle_y = info->slot.mirror_a->ang.y;
    angle_z = info->slot.mirror_a->ang.z;

    length_sq = direction_x * direction_x + direction_z * direction_z;
    inverse_length = moves_inverse_sqrt(length_sq);
    direction_x *= inverse_length;
    direction_z *= inverse_length;
    offset.x = -0.2f * direction_x;
    offset.z = -0.2f * direction_z;
    lateral.x = direction_z;
    lateral.z = -direction_x;
    if (am_i_on_the_left2(
            player->plyr_info->slot.mirror_a,
            player->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        v3_add_v3_scaled(&offset, &offset, &lateral, -0.7f);
        transition = 0;
    } else {
        v3_add_v3_scaled(&offset, &offset, &lateral, 0.7f);
        transition = 8;
    }
    offset.x += main_object->pos.value.x;
    offset.z += main_object->pos.value.z;
    position_x = offset.x;
    position_z = offset.z;

    sidekick->flags_09_bits.bit6 = 0;
    sidekick->flags_09_bits.launched = 0;
    position_y = 6.535f + g_game_info.field_34;
    sidekick->ground_colls_y = main_object->ground_colls_y;
    set_root_and_obj_movement_weights(anim, 0.0f, 1.0f);
    sidekick->pos.value.x = position_x;
    sidekick->pos.value.y = position_y;
    sidekick->pos.value.z = position_z;
    sidekick->ang.x = angle_x;
    sidekick->ang.y = angle_y;
    sidekick->ang.z = angle_z;
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);

    if (player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    transition |= 3;
    set_anim_script(anim, player->noob_entrance_animation, transition);
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
    update_bone_hierarchy(
        sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    ground_me(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    random_hit(1);
    shake_camera(2, 0.02f);
    sidekick->flags_09_bits.head_tracking = 0;
    transition_to_anim_script(
        anim, player->common_exit_animation, transition, 0.05f);
    anim->step = 0.6f;
    moves_sleep(10000.0f);
    return -1.0f;
}

void noobsmoke_sidekick_double_charge(void) {
    MkProc* proc;
    union MovesSidekickPdataRef pdata;

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_charge,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_charge,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = plyr_pdata;
    }
}

/* TODO: [near miss] 94.95%; retail order restored; CFG aligns with defined Vec Y zeros; FPR coloring remains. */
static float p_plyr_sidekick_charge(void) {
    union {
        float f;
        unsigned int u;
    } inverse_bits;
    struct MovesSidekickPdata* pdata;
    MkObj* sidekick;
    PlyrInfo* info;
    PlyrPdata* player;
    MkProc* anim_proc;
    MkProc* player_proc;
    MkObj* main_object;
    MkObj* angle_object;
    MkObj* opponent_object;
    AnimPdata* anim;
    Vec direction;
    Vec lateral;
    Vec offset;
    float length_sq;
    float inverse_length;
    float estimate_product;
    float correction;
    float facing_yaw;
    float wrapped_angle;
    float sine;
    float cosine;
    float position_x;
    float position_y;
    float position_z;
    float angle_x;
    float angle_y;
    float angle_z;
    int transition;
    int function;

    pdata = (struct MovesSidekickPdata*)apdata;
    player = pdata->player;
    sidekick = player->sidekick_obj;
    if (sidekick != 0) {
        if (sidekick->hdr.instance != player->sidekick_instance) {
            sidekick = 0;
        }
    } else {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    info = player->plyr_info;
    opponent_object = player->his_obj;
    main_object = info->slot.mirror_a;
    if (anim_proc != 0) {
        if (anim_proc->instance != player->sidekick_anim_instance) {
            anim_proc = 0;
        }
    } else {
        anim_proc = 0;
    }
    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    moves_prepare_sidekick_entrance(anim, sidekick);
    sidekick->flags_09_bits.head_tracking = 0;

    player_proc = player->own_player_proc;
    if (player_proc != 0 &&
        player_proc->instance != player->own_player_proc_instance) {
        player_proc = 0;
    }
    get_cmdscript_for_proc(player_proc);
    tag_team_activate_player(
        sidekick, info->slot.pdata->sidekick_active);
    angle_object = info->slot.mirror_a;
    direction.x = main_object->pos.value.x - opponent_object->pos.value.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.value.z - opponent_object->pos.value.z;
    angle_x = angle_object->ang.x;
    angle_y = angle_object->ang.y;
    angle_z = angle_object->ang.z;

    length_sq =
        direction.x * direction.x + direction.z * direction.z;
    if (length_sq <= 0.0f) {
        inverse_length = 0.0f;
    } else {
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
            player->plyr_info->slot.mirror_a,
            player->his_plyr_pdata->plyr_info->slot.mirror_a) != 0) {
        v3_add_v3_scaled(&offset, &offset, &lateral, 0.45f);
        transition = 0;
    } else {
        v3_add_v3_scaled(&offset, &offset, &lateral, -0.45f);
        transition = 8;
    }
    position_x = main_object->pos.value.x + offset.x;
    position_y = g_game_info.field_34;
    position_z = main_object->pos.value.z + offset.z;

    sidekick->ground_colls_y = main_object->ground_colls_y;
    set_root_and_obj_movement_weights(anim, 0.0f, 1.0f);
    sidekick->pos.value.x = position_x;
    sidekick->pos.value.y = position_y;
    sidekick->pos.value.z = position_z;
    sidekick->ang.x = angle_x;
    sidekick->ang.y = angle_y;
    sidekick->ang.z = angle_z;
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    ground_me(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);

    if (player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    set_anim_script(anim, shared_ani.sidekick_charge, transition);
    moves_sleep(1.0f);
    unhide_obj(sidekick);
    anim->step = 1.0f;
    facing_yaw = player->plyr_info->slot.mirror_a->ang.y;
    wrapped_angle =
        0.000005992112f *
        (float)(((int)(166886.1f * facing_yaw)) & 0xFFFFF);
    sine = gxMathSin(wrapped_angle);
    cosine = gxMathCos(wrapped_angle);
    while (anim->frame < 5.0f) {
        moves_sleep(1.0f);
    }

    sidekick->pos_vel.x = 0.1f * sine;
    sidekick->pos_vel.y = 0.0f;
    sidekick->pos_vel.z = 0.1f * cosine;
    sidekick->flags_08_bits.gravity_enabled = 1;
    while (anim->frame < 24.0f) {
        moves_sleep(1.0f);
        sidekick->pos_vel.x *= 0.99f;
        sidekick->pos_vel.y = 0.0f;
        sidekick->pos_vel.z *= 0.99f;
    }
    while (anim->frame < 24.0f) {
        moves_sleep(1.0f);
    }
    anim->step = 0.1f;
    moves_sleep(2.0f);
    sidekick->pos_vel.z = 0.0f;
    sidekick->pos_vel.y = 0.0f;
    sidekick->pos_vel.x = 0.0f;
    sidekick->flags_08_bits.gravity_enabled = 0;
    while (anim->frame < 26.0f) {
        moves_sleep(1.0f);
    }

    anim->step = 0.75f;
    transition_to_anim_script(
        anim, player->base_animations[0], transition | 3, 0.25f);
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
        get_script_function_by_name(player->cmo, "start_smoke_exit_pfx");
    plyr_start_script_in_plyr_pdata_proc(
        player, 0xC025, function);
    moves_sleep(30.0f);
    obj_set_gravity(sidekick, 0.0f);
    sidekick->flags_08_bits.moving = 0;
    hide_obj(sidekick);
    return -1.0f;
}

void sidekick_switch_style_swap(float unread_arg, unsigned int count) {
    while (count != 0) {
        ani_loop_more_frames(1.0f);
        face_opponent_now();
        count--;
    }
}

static float p_sidekick_exit_now(void) {
    struct MovesSidekickPdata* pdata;
    PlyrPdata* player;
    PlyrFighterDefinition* fighter;
    MkObj* sidekick;
    MkProc* anim_proc;
    AnimPdata* anim;

    pdata = (struct MovesSidekickPdata*)apdata;
    player = pdata->player;

    sidekick = MK_HDR_LIVE(player->sidekick_obj, player->sidekick_instance);
    anim_proc = MK_LIVE(player->sidekick_anim_proc, player->sidekick_anim_instance);

    anim = (AnimPdata*)pdata_of_proc(anim_proc);
    transition_to_anim_script(
        anim,
        (player->fighter_definition)
            ->walk_backward_loop,
        0, 0.2f);
    fighter =
        player->fighter_definition;
    anim->step = 1.2f * fighter->move_blend_data->walk_backward_step;
    sidekick->flags_09_bits.bit6 = 1;
    _mkproc_sleep_ticks = 45.0f;
    aproc->vtbl->sleep();
    sidekick_cool_vanish(player);
    return -1.0f;
}

static inline void moves_create_sidekick_exit_process(void) {
    MkHdr* exit_header;
    PlyrPdata* player;
    MkProc* proc;

    player = plyr_pdata;
    if (player->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_sidekick_exit_now, sizeof(struct MovesSidekickPdata),
            &exit_header);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_sidekick_exit_now, sizeof(struct MovesSidekickPdata),
            &exit_header);
    }
    if (proc != 0 && exit_header != 0) {
        ((struct MovesSidekickPdata*)exit_header)->player = player;
    }
}

/* TODO: [near miss] 99.81%; six player-x/normalized-z FPR rows differ;
 * fresh retail float-pool targets remain a TU layout check. */
int advance_my_sidekick_from_behind_with_moveset(void) {
    MkObj* sidekick;
    MkHdr* object;
    float delta_x;
    float delta_z;
    float player_z;
    float normalized_z;
    float opponent_x;
    float opponent_z;
    float inverse_distance;
    float position_x;
    float position_y;
    float position_z;

    sidekick = MK_HDR_LIVE(plyr_pdata->sidekick_obj, plyr_pdata->sidekick_instance);

    plyr_obj->flags_09_bits.bit6 = 1;
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    stop_me();
    tightrope_restrictions_off();

    player_z = plyr_obj->pos.value.z;
    opponent_z = his_obj->pos.value.z;
    delta_z = player_z - opponent_z;
    delta_x = plyr_obj->pos.value.x;
    opponent_x = his_obj->pos.value.x;
    delta_x -= opponent_x;
    position_y = plyr_obj->pos.value.y;
    inverse_distance =
        moves_inverse_sqrt(delta_x * delta_x + delta_z * delta_z);
    delta_x *= inverse_distance;
    normalized_z = delta_z * inverse_distance;
    position_x = -2.0f * delta_x;
    position_z = -2.0f * normalized_z;
    position_x += opponent_x;
    position_z += opponent_z;
    sidekick->pos.value.x = position_x;
    sidekick->pos.value.y = position_y;
    sidekick->pos.value.z = position_z;
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    unhide_obj(sidekick);
    active_sidekick_swap(plyr_pdata, 0);

    if (plyr_pdata->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    moves_create_sidekick_exit_process();

    set_root_and_obj_movement_weights(plyr_anim_pdata, 0.0f, 1.0f);
    plyr_obj->pos.value.x = position_x;
    plyr_obj->pos.value.y = position_y;
    plyr_obj->pos.value.z = position_z;
    face_opponent_now();
    update_mkobj(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    glitch_to_ani(plyr_pdata->fighter_definition->walk_forward_loop, 0);
    plyr_anim_pdata->step =
        plyr_pdata->fighter_definition->move_blend_data->walk_forward_step;
    tightrope_restrictions_on();
    return 1;
}

void advance_sidekick_with_moveset(PlyrPdata* player) {
    MkProc* proc;
    union MovesSidekickPdataRef pdata;

    if (player->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_plyr_sidekick_switch,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_plyr_sidekick_switch,
            sizeof(struct MovesSidekickPdata), &pdata.hdr);
    }
    if (proc != 0 && pdata.hdr != 0) {
        pdata.sidekick->player = player;
    }
}

/* TODO: [breakthrough needed] 72.37255%; retail order restored; structural and GPR mismatches need CFG/lifetime evidence. */
static float p_plyr_sidekick_switch(void) {
    union {
        float f;
        unsigned int u;
    } inverse_bits;
    struct MovesSidekickPdata* pdata;
    PlyrPdata* player;
    PlyrFighterDefinition* fighter;
    union MovesSidekickPdataRef watchdog_pdata;
    MkProc* player_proc;
    MkProc* anim_proc;
    MkObj* sidekick;
    MkObj* main_object;
    AnimPdata* sidekick_anim;
    CmdScript* script;
    Vec direction;
    float length_sq;
    float inverse_length;
    float estimate_product;
    float correction;
    float object_weight;
    float position_y;
    float angle_x;
    float angle_y;
    float angle_z;
    int state;
    MkProc* proc;

    pdata = (struct MovesSidekickPdata*)apdata;
    player = pdata->player;
    sidekick = player->sidekick_obj;
    object_weight = 2.0f;
    if (sidekick != 0 &&
        sidekick->hdr.instance != player->sidekick_instance) {
        sidekick = 0;
    }
    anim_proc = player->sidekick_anim_proc;
    if (anim_proc != 0 &&
        anim_proc->instance != player->sidekick_anim_instance) {
        anim_proc = 0;
    }
    sidekick_anim = (AnimPdata*)pdata_of_proc(anim_proc);

    state = pdata->player->state;
    if (state != 0 && state != 0x2000 && state != 0x2001) {
        return -1.0f;
    }
    if ((g_game_info.flags & 0x18) != 0) {
        return -1.0f;
    }

    player_proc = player->own_player_proc;
    if (player_proc != 0 &&
        player_proc->instance != player->own_player_proc_instance) {
        player_proc = 0;
    }
    script = get_cmdscript_for_proc(player_proc);
    main_object = player->plyr_info->slot.mirror_a;
    tag_team_activate_player(
        sidekick, player->plyr_info->slot.pdata->sidekick_active);

    direction.x = main_object->pos.value.x - player->his_obj->pos.value.x;
    direction.y = 0.0f;
    direction.z = main_object->pos.value.z - player->his_obj->pos.value.z;
    position_y = main_object->pos.value.y;
    angle_x = main_object->ang.x;
    angle_y = main_object->ang.y;
    angle_z = main_object->ang.z;
    if (is_plyr_airborn(player->his_obj, player->his_plyr_pdata) != 0) {
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
    sidekick->pos.value.x =
        main_object->pos.value.x + direction.x * inverse_length * 2.5f;
    sidekick->pos.value.y = position_y;
    sidekick->pos.value.z =
        main_object->pos.value.z + direction.z * inverse_length * 2.5f;
    sidekick->ang.x = angle_x;
    sidekick->ang.y = angle_y;
    sidekick->ang.z = angle_z;
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    ground_me(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);

    if (pdata->player->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    fighter =
        pdata->player->fighter_definition;
    set_anim_script(sidekick_anim, fighter->walk_forward_loop, 0);
    moves_sleep(1.0f);
    unhide_obj(sidekick);
    sidekick_anim->step = 1.25f;

    if (pdata->player->plyr_num == 0) {
        proc = _create_mkproc_generic_bigstack(
            0xC028, 8, p_sidekick_watchdog_launcher,
            sizeof(struct MovesSidekickPdata), &watchdog_pdata.hdr);
    } else {
        proc = _create_mkproc_generic_bigstack(
            0xC029, 8, p_sidekick_watchdog_launcher,
            sizeof(struct MovesSidekickPdata), &watchdog_pdata.hdr);
    }
    if (proc != 0 && watchdog_pdata.hdr != 0) {
        watchdog_pdata.sidekick->player = pdata->player;
    }

    if ((g_game_info.flags & 0x18) == 0) {
        script->unk28 = 0x7B;
        xfer_player_proc(player_proc, r_call_script_function);
    }
    return -1.0f;
}

static float p_sidekick_watchdog_launcher(void) {
    struct MovesSidekickPdata* pdata;
    PlyrPdata* player;
    MkObj* sidekick;

    pdata = (struct MovesSidekickPdata*)apdata;
    player = pdata->player;
    _mkproc_sleep_ticks = 300.0f;
    aproc->vtbl->sleep();

    sidekick = MK_HDR_LIVE(player->sidekick_obj, player->sidekick_instance);
    if (sidekick != 0 && !sidekick->hide_flag_bits.hidden) {
        hide_obj(sidekick);
    }
    return -1.0f;
}

PlyrFighterDefinition* get_active_moveset_from_pdata(PlyrPdata* player) {
    return player->fighter_definition;
}

void advance_my_moveset(void) {
    snd_req(0xDC1);
    if (aproc->pid == 0x1001) {
        advance_active_moveset(g_game_info.plyr0.slot.pdata);
    } else {
        advance_active_moveset(g_game_info.plyr1.slot.pdata);
    }
}

int is_weapon_style(PlyrFighterDefinition* style) {
    PlyrStyleDefinition* weapon_data;

    if (style != 0) {
        weapon_data = style->definition;
        if (weapon_data != 0 &&
            (weapon_data->primary_weapon != 0 ||
             weapon_data->secondary_weapon != 0)) {
            return 1;
        }
    }
    return 0;
}

float switch_proc_advance_moveset(void) {
    PlyrPdata* player_data;
    union MovesSidekickPdataRef pdata;
    int player_num;
    int fighter_state;
    MkProc* proc;
    int player_state;
    MkProc* idle_proc;
    PlyrInfo* player;
    float life;

    player = switch_pdata->player;
    if (player == 0) {
        return -1.0f;
    }

    player_data = player->slot.pdata;
    idle_proc = player->idle_proc;
    player_num = player->controller_slot;
    player_state = player->player_state;
    life = player->field_0C;
    if (mode_of_play == 8 &&
        trial_change_style_callback(player_num) == 0) {
        return -1.0f;
    }

    if ((player_state == 2 || player_state == 3) && life > 0.0f) {
        fighter_state = player_data->state;
        if ((fighter_state & 0x1800) == 0 &&
            !player_data->state_flags.bits.frozen) {
            if (f_fatality_available != 0 && fighter_state != 0x420D) {
                if ((fighter_state & 0x200) == 0 &&
                    is_pX_airborn(player_num) == 0) {
                    xfer_proc(idle_proc, x_advance_fatality);
                    snd_req(0xDC5);
                }
                player_data->fatality_advance = 0x1E0;
            } else {
                if ((fighter_state & 0x200) == 0 &&
                    is_pX_airborn(player_num) == 0) {
                    xfer_proc(idle_proc, x_advance_moveset);
                }
                if (player_data->state == 0x420D) {
                    xfer_proc(idle_proc, x_advance_moveset);
                }
                if (player->slot.pdata->sidekick_available == 0) {
                    advance_active_moveset(player_data);
                    snd_req(0xDC1);
                } else {
                    if (player_data->plyr_num == 0) {
                        proc = _create_mkproc_generic_bigstack(
                            0xC028, 8, p_plyr_sidekick_switch,
                            sizeof(struct MovesSidekickPdata), &pdata.hdr);
                    } else {
                        proc = _create_mkproc_generic_bigstack(
                            0xC029, 8, p_plyr_sidekick_switch,
                            sizeof(struct MovesSidekickPdata), &pdata.hdr);
                    }
                    if (proc != 0 && pdata.hdr != 0) {
                        pdata.sidekick->player = player_data;
                    }
                }
            }
        }
    }
    return -1.0f;
}

float x_advance_fatality(void) {
    set_my_state(0x420D);
    advance_active_moveset(plyr_pdata);
    plyr_pdata->fatality_advance = 0x1E0;
    blend_to_stance(0.1f);
    plyr_anim_pdata->step = 1.0f;
    init_ground_move();
    while (plyr_pdata->fatality_advance != 0) {
        moves_sleep(1.0f);
        if (f_fatality_available == 0) {
            break;
        }
        plyr_pdata->fatality_advance--;
    }
    blend_to_stance(0.1f);
    plyr_anim_pdata->step = 1.0f;
    moves_jump(j_exit);
    return 0.0f;
}

float x_advance_moveset(void) {
    set_my_state(0);
    plyr_pdata->fatality_advance = 0;
    blend_to_stance(0.1f);
    plyr_anim_pdata->step = 1.0f;
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 99.78%; retail order restored; r3/r4 coloring around animation field stores remains. */
void attack_to_frame_x(AniData* animation, float frame, float blend_rate,
                       float step, float weight, unsigned int voice_event,
                       unsigned int whoosh_event, int transition) {
    float whoosh_frame;
    float voice_frame;

    plyr_pdata->last_collision_tick = game_tick_ctr;
    plyr_pdata->throw_restriction = 1;
    clear_collision_result();
    plyr_pdata->shared_attack_until =
        exec_tick_ctr + (int)(0.5f + frame / step);
    plyr_pdata->attack_counter++;
    plyr_anim_pdata->flags |= 0x40;
    if (animation != 0) {
        blend_to_ani(animation, transition, blend_rate);
    }
    plyr_anim_pdata->step = step;
    voice_frame = voice_event >> 16;
    plyr_anim_pdata->weight = weight;
    whoosh_frame = whoosh_event >> 16;

    if (voice_frame < frame && whoosh_frame < frame) {
        if (voice_frame < whoosh_frame) {
            if (voice_frame > 0.0f) {
                ani_to_frame_x(voice_frame);
            }
            if (exec_tick_ctr - (unsigned int)plyr_pdata->field_26C >
                30U) {
                random_voice((unsigned short)voice_event);
                plyr_pdata->field_26C = exec_tick_ctr;
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
            if (exec_tick_ctr - (unsigned int)plyr_pdata->field_26C >
                30U) {
                random_voice((unsigned short)voice_event);
                plyr_pdata->field_26C = exec_tick_ctr;
            }
        }
        if (voice_frame == whoosh_frame) {
            if (whoosh_frame > 0.0f) {
                ani_to_frame_x(whoosh_frame);
            }
            whoosh_fx((unsigned short)whoosh_event);
            if (exec_tick_ctr - (unsigned int)plyr_pdata->field_26C >
                30U) {
                random_voice((unsigned short)voice_event);
                plyr_pdata->field_26C = exec_tick_ctr;
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

float jump_away_opponent(void) {
    MkHdr* object;
    Vec direction;
    float high_frame;

    rotate_towards_him(0.2f);
    avoid_double_ani();
    init_air_move();
    set_my_state(0x6200);
    uv_to_opponent(&direction);
    plyr_obj->pos_vel.x = 0.008f * -direction.x;
    plyr_obj->pos_vel.z = 0.008f * -direction.z;
    xfer_proc(plyr_anim_proc, p_animate);
    dead_liukang_snd_chain_check(plyr_pdata, 0, 0x1E, 0x50);
    random_voice(9);
    plyr_anim_pdata->flags |= 0x40;
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.jump_away, 0xB, 0.1f);
    plyr_anim_pdata->frame = 8.0f;
    setup_to_match_land_frame(0.07f, -0.0035f, 18.0f);
    snd_req_vol(0xD71, 0.5f);
    wait_to_land();
    player_feet_land_chores();
    stop_me();
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    high_frame = plyr_anim_pdata->high_frame;
    plyr_anim_pdata->landing_start = high_frame - 10.0f;
    plyr_anim_pdata->landing_end = plyr_anim_pdata->high_frame;
    xfer_proc(plyr_anim_proc, aniproc_land);
    return 0.0f;
}

float jump_towards_opponent(void) {
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
            return 0.0f;
        }
        moves_jump(j_flying_kick2_early);
        return 0.0f;
    }
    if (state == 0x6003) {
        moves_jump(j_flying_punch_early);
        return 0.0f;
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
    flight_ticks = flight_ticks >= 0.0f ? flight_ticks : -flight_ticks;
    plyr_anim_pdata->step = 18.0f / flight_ticks;
    remaining_ticks = flight_ticks - 1.0f;
    aproc->flags_bits.use_game_speed = 1;
    while (remaining_ticks > 0.0f) {
        moves_sleep(1.0f);
        remaining_ticks -= 1.0f;
    }
    aproc->flags_bits.use_game_speed = 0;

    player_feet_land_chores();
    stop_me();
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    high_frame = plyr_anim_pdata->high_frame;
    plyr_anim_pdata->landing_start = high_frame - 10.0f;
    plyr_anim_pdata->landing_end = plyr_anim_pdata->high_frame;
    xfer_proc(plyr_anim_proc, aniproc_land);
    return 0.0f;
}

float jump_towards_opponent_bgnd_transition(void) {
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
    flight_ticks = flight_ticks >= 0.0f ? flight_ticks : -flight_ticks;
    plyr_anim_pdata->step = 18.0f / flight_ticks;
    remaining_ticks = flight_ticks - 1.0f;
    aproc->flags_bits.use_game_speed = 1;
    while (remaining_ticks > 0.0f) {
        moves_sleep(1.0f);
        remaining_ticks -= 1.0f;
    }
    aproc->flags_bits.use_game_speed = 0;

    player_feet_land_chores();
    stop_me();
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    high_frame = plyr_anim_pdata->high_frame;
    plyr_anim_pdata->landing_start = high_frame - 10.0f;
    plyr_anim_pdata->landing_end = plyr_anim_pdata->high_frame;
    xfer_proc(plyr_anim_proc, aniproc_land);
    return 0.0f;
}

static float jump_landing_j_exit(void) {
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
    high_frame = plyr_anim_pdata->high_frame;
    plyr_anim_pdata->landing_start = high_frame - 10.0f;
    plyr_anim_pdata->landing_end = plyr_anim_pdata->high_frame;
    xfer_proc(plyr_anim_proc, aniproc_land);
    moves_jump(j_exit);
    return 0.0f;
}

float step_backward(void) {
    int pad_position;

    avoid_double_ani();
    dead_liukang_snd_chain_check(plyr_pdata, 0, 0xA, 0x14);
    trial_increment_state_value(plyr_pdata->plyr_num, 0xF, 0);
    init_ground_move_no_aniproc();
    rotate_towards_him(0.2f);
    pad_position = my_pad_position();
    set_my_state(0x2001);
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(
        plyr_pdata->fighter_definition->walk_backward_start, 3, 0.33f);
    plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                ->walk_backward_start_step;
    plyr_anim_pdata->weight = plyr_pdata->fighter_definition->move_blend_data
                                  ->walk_backward_start_weight;
    ani_to_frame_x_call(
        face_opponent_now,
        plyr_pdata->fighter_definition->move_blend_data
            ->walk_backward_start_frame);
    ani_to_frame_x_call(
        face_opponent_now, plyr_anim_pdata->high_frame - 14.0f);
    random_foot(1);
    if (pad_position == my_pad_position() && pad_position != 0) {
        moves_jump(walk_backward);
        return 0.0f;
    }
    ani_to_frame_x_call(
        face_opponent_now, plyr_anim_pdata->high_frame - 10.0f);
    plyr_anim_pdata->weight = 1.0f;
    if (plyr_pdata->fighter_definition->move_blend_data
            ->use_fighting_stance != 0) {
        blend_to_fstance(0.1f);
    } else {
        blend_to_stance(0.1f);
    }
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

void forced_step_forward(void) {
    avoid_double_ani();
    init_ground_move_no_aniproc();
    face_opponent_now();
    my_pad_position();
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(
        plyr_pdata->fighter_definition->forced_step_animation, 3, 0.33f);
    plyr_anim_pdata->step =
        plyr_pdata->fighter_definition->move_blend_data->step;
    plyr_anim_pdata->weight =
        plyr_pdata->fighter_definition->move_blend_data->weight;
}

float step_forward(void) {
    int pad_position;

    avoid_double_ani();
    trial_increment_state_value(plyr_pdata->plyr_num, 0xE, 0);
    init_ground_move_no_aniproc();
    rotate_towards_him(0.2f);
    pad_position = my_pad_position();
    set_my_state(0x2000);
    dead_liukang_snd_chain_check(plyr_pdata, 0, 0xA, 0xF);
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(
        plyr_pdata->fighter_definition->walk_forward_start, 3, 0.33f);
    plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                ->walk_forward_start_step;
    plyr_anim_pdata->weight = plyr_pdata->fighter_definition->move_blend_data
                                  ->walk_forward_start_weight;
    ani_to_frame_x_call(
        face_opponent_now,
        plyr_pdata->fighter_definition->move_blend_data
            ->walk_forward_start_frame);
    ani_to_frame_x_call(
        face_opponent_now, plyr_anim_pdata->high_frame - 13.0f);
    random_foot(0);
    if (pad_position == my_pad_position() && pad_position != 0) {
        moves_jump(walk_forward);
        return 0.0f;
    }
    ani_to_frame_x_call(
        face_opponent_now, plyr_anim_pdata->high_frame - 10.0f);
    plyr_anim_pdata->weight = 1.0f;
    if (plyr_pdata->fighter_definition->move_blend_data
            ->use_fighting_stance != 0) {
        blend_to_fstance(0.1f);
    } else {
        blend_to_stance(0.1f);
    }
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

static float blend_to_duck_block(void) {
    set_my_state(0x901);
    if (should_i_weapon_block() != 0) {
        blend_to_ani(plyr_pdata->fighter_definition->duck_block_intro, 3, 0.1f);
        plyr_anim_pdata->step = 1.0f;
        ani_to_end();
        glitch_to_ani(plyr_pdata->fighter_definition->duck_block_animation, 0);
    } else {
        blend_to_ani(shared_ani.duck_block_intro, 3, 0.1f);
        plyr_anim_pdata->step = 1.0f;
        ani_to_end();
        glitch_to_ani(shared_ani.duck_block_loop, 0);
    }
    plyr_pdata->his_attack_counter = get_his_attack_counter();
    set_my_state(0x900);
    g_min_time_in_block_for_drone =
        exec_tick_ctr + drone_ai_get_min_time_in_block();
    moves_jump(j_duck_block_loop);
    return 0.0f;
}

static float jump_towards_opponent_j_exit(void) {
    jump_towards_opponent();
    moves_jump(j_exit_blend_stance);
    return 0.0f;
}

static float jump_away_opponent_j_exit(void) {
    MkHdr* object;
    Vec direction;
    float high_frame;

    rotate_towards_him(0.2f);
    avoid_double_ani();
    init_air_move();
    set_my_state(0x6200);
    uv_to_opponent(&direction);
    plyr_obj->pos_vel.x = 0.008f * -direction.x;
    plyr_obj->pos_vel.z = 0.008f * -direction.z;
    xfer_proc(plyr_anim_proc, p_animate);
    dead_liukang_snd_chain_check(plyr_pdata, 0, 0x1E, 0x50);
    random_voice(9);
    plyr_anim_pdata->flags |= 0x40;
    transition_to_anim_script(
        plyr_anim_pdata, shared_ani.jump_away, 0xB, 0.1f);
    plyr_anim_pdata->frame = 8.0f;
    setup_to_match_land_frame(0.07f, -0.0035f, 18.0f);
    snd_req_vol(0xD71, 0.5f);
    wait_to_land();
    player_feet_land_chores();
    stop_me();
    plyr_obj->flags_09_bits.launched = 1;
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    update_bone_hierarchy(object);
    object = plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0;
    ground_me(object);
    plyr_obj->gravity = 0.0f;
    plyr_obj->flags_08_bits.moving = 0;
    high_frame = plyr_anim_pdata->high_frame;
    plyr_anim_pdata->landing_start = high_frame - 10.0f;
    plyr_anim_pdata->landing_end = plyr_anim_pdata->high_frame;
    xfer_proc(plyr_anim_proc, aniproc_land);
    moves_jump(j_exit_blend_stance);
    return 0.0f;
}

float step_right(void) {
    int pad_position;

    init_3d_move_no_aniproc();
    rotate_towards_him(0.2f);
    pad_position = my_pad_position();
    set_my_state(0);
    plyr_pdata->strafe_direction = pad_position;
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(plyr_pdata->fighter_definition->strafe_right_start, 3, 0.2f);
    dead_liukang_snd_chain_check(plyr_pdata, 0, 0xA, 0x19);
    plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                ->strafe_start_step;
    plyr_anim_pdata->weight = plyr_pdata->fighter_definition->move_blend_data
                                  ->strafe_start_weight;
    plyr_pdata->dodge_sound_played = 0;
    while (plyr_anim_pdata->frame <
           plyr_pdata->fighter_definition->move_blend_data
               ->strafe_start_frame) {
        dodge_3d_scan();
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        moves_sleep(1.0f);
    }
    disable_this_move_exec(0x6004, 0x28);
    random_foot(1);
    if (pad_position == my_pad_position() && pad_position != 0) {
        moves_jump(walk_right);
        return 0.0f;
    }
    ani_to_blend_frame(10.0f);
    plyr_anim_pdata->weight = 1.0f;
    if (plyr_pdata->fighter_definition->move_blend_data
            ->use_fighting_stance != 0) {
        blend_to_fstance(0.1f);
    } else {
        blend_to_stance(0.1f);
    }
    plyr_pdata->strafe_direction = 0;
    set_my_state(0);
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

static float walk_right(void) {
    int pad_position = my_pad_position();

    set_my_state(0x2003);
    blend_to_ani(plyr_pdata->fighter_definition->strafe_right_loop, 0, 0.1f);
    plyr_anim_pdata->step = 0.9f;
    while (pad_position == my_pad_position()) {
        if (moves_dead_movement() != 0) {
            break;
        }
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        moves_sleep(1.0f);
    }
    plyr_pdata->strafe_direction = 0;
    set_my_state(0);
    if (plyr_pdata->fighter_definition->move_blend_data
            ->use_fighting_stance != 0) {
        blend_to_fstance(0.1f);
    } else {
        blend_to_stance(0.1f);
    }
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

float step_left(void) {
    int pad_position;

    init_3d_move_no_aniproc();
    rotate_towards_him(0.2f);
    pad_position = my_pad_position();
    set_my_state(0);
    plyr_pdata->strafe_direction = pad_position;
    plyr_anim_pdata->flags |= 0x40;
    blend_to_ani(plyr_pdata->fighter_definition->strafe_left_start, 3, 0.2f);
    dead_liukang_snd_chain_check(plyr_pdata, 0, 0xA, 0x19);
    plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                ->strafe_start_step;
    plyr_anim_pdata->weight = plyr_pdata->fighter_definition->move_blend_data
                                  ->strafe_start_weight;
    plyr_pdata->dodge_sound_played = 0;
    while (plyr_anim_pdata->frame <
           plyr_pdata->fighter_definition->move_blend_data
               ->strafe_start_frame) {
        dodge_3d_scan();
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        moves_sleep(1.0f);
    }
    disable_this_move_exec(0x6004, 0x28);
    random_foot(1);
    if (pad_position == my_pad_position() && pad_position != 0) {
        moves_jump(walk_left);
        return 0.0f;
    }
    ani_to_blend_frame(10.0f);
    plyr_anim_pdata->weight = 1.0f;
    if (plyr_pdata->fighter_definition->move_blend_data
            ->use_fighting_stance != 0) {
        blend_to_fstance(0.1f);
    } else {
        blend_to_stance(0.1f);
    }
    plyr_pdata->strafe_direction = 0;
    set_my_state(0);
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

static float walk_left(void) {
    int pad_position;

    pad_position = my_pad_position();
    set_my_state(0x2003);
    blend_to_ani(plyr_pdata->fighter_definition->strafe_left_loop, 0, 0.1f);
    plyr_anim_pdata->step = 0.9f;
    while (pad_position == my_pad_position()) {
        if (moves_dead_movement() != 0) {
            break;
        }
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        moves_sleep(1.0f);
    }
    plyr_pdata->strafe_direction = 0;
    set_my_state(0);
    if (plyr_pdata->fighter_definition->move_blend_data
            ->use_fighting_stance != 0) {
        blend_to_fstance(0.1f);
    } else {
        blend_to_stance(0.1f);
    }
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

static float walk_forward(void) {
    int pad_position;
    int tracking_disabled;

    tracking_disabled = 0;
    pad_position = my_pad_position();
    blend_to_ani(plyr_pdata->fighter_definition->walk_forward_loop, 0, 0.2f);
    plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                ->walk_forward_step;
    while (pad_position == my_pad_position()) {
        if (moves_dead_movement() != 0) {
            break;
        }
        if (is_he_airborn() == 0 || xz_distance_between_players() >= 1.0f) {
            if (tracking_disabled == 0) {
                face_opponent_now();
            }
        } else {
            tracking_disabled = 1;
            head_tracking_off();
        }
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        moves_sleep(1.0f);
    }
    blend_to_stance(0.1f);
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

static float walk_backward(void) {
    int pad_position;

    pad_position = my_pad_position();
    blend_to_ani(plyr_pdata->fighter_definition->walk_backward_loop, 0, 0.2f);
    plyr_anim_pdata->step = plyr_pdata->fighter_definition->move_blend_data
                                ->walk_backward_step;
    while (pad_position == my_pad_position()) {
        if (moves_dead_movement() != 0) {
            break;
        }
        face_opponent_now();
        advance_anim(plyr_anim_pdata);
        pose_anim(plyr_anim_pdata, 1);
        moves_sleep(1.0f);
    }
    blend_to_stance(0.1f);
    moves_sleep(1.0f);
    moves_jump(j_exit);
    return 0.0f;
}

int check_for_dead_movement(void) {
    if (plyr_pdata == 0) {
        return 0;
    }
    if (round_winner == 0) {
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

static float blend_to_fstance_j_exit(void) {
    blend_to_fstance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

float glitch_to_stance_j_exit(void) {
    PlyrFighterDefinition* fighter;

    fighter = plyr_pdata->fighter_definition;
    set_anim_script(plyr_anim_pdata, fighter->duck_exit_animation, 0x20);
    while (do_i_have_life_left() == 0) {
        moves_sleep(1.0f);
    }
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float blend_to_stance_j_exit(void) {
    blend_to_stance(0.1f);
    moves_jump(j_exit);
    return 0.0f;
}

static float drahmin_dash_back(void) {
    snd_req(0xD71);
    blend_to_ani(plyr_pdata->turn_to_screen_animation, 0xB, 0.1f);
    plyr_anim_pdata->step = 1.4f;
    plyr_anim_pdata->weight_velocity = 0.0f;
    plyr_anim_pdata->weight = 1.5f;
    ani_to_frame_x(13.0f);
    init_air_move();
    ani_to_frame_x(16.0f);
    snd_req(0xD71);
    blend_to_ani(plyr_pdata->turn_to_screen_animation, 3, 0.1f);
    init_ground_move();
    plyr_anim_pdata->step = 1.4f;
    plyr_anim_pdata->weight = 1.5f;
    ani_to_frame_x(13.0f);
    init_air_move();
    ani_to_frame_x(17.0f);
    init_ground_move();
    blend_to_stance(0.1f);
    disable_this_move_exec(0x6208, 0x28);
    moves_jump(j_exit);
    return 0.0f;
}

float joy_dash_back(void) {
    avoid_double_ani();
    init_ground_move_no_aniproc();
    if ((plyr_pdata->fighter_definition)
            ->weapon_rest_animation != 0) {
        plyr_spawn_anim(
            (plyr_pdata->fighter_definition)
                ->weapon_rest_animation,
            p_animate_weapon_rest);
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 0x17, 0);
    random_voice(9);
    rotate_towards_him(0.2f);
    dead_liukang_snd_chain_check(plyr_pdata, 0, 5, 0x14);
    set_my_state(0x6208);
    plyr_anim_pdata->flags |= 0x40;
    if (plyr_pdata->character_id == 0x2B) {
        moves_jump(drahmin_dash_back);
        return 0.0f;
    }
    snd_req(0xD71);
    blend_to_ani(shared_ani.dash_back, 3, 0.2f);
    plyr_anim_pdata->step = 0.9f;
    plyr_anim_pdata->weight_velocity = 0.0f;
    plyr_anim_pdata->weight = 1.5f;
    ani_to_frame_x(12.0f);
    init_air_move();
    ani_to_frame_x(17.0f);
    init_ground_move();
    random_foot(1);
    blend_to_fstance(0.1f);
    disable_this_move_exec(0x6208, 0x28);
    moves_jump(j_exit);
    return 0.0f;
}

float victory(void) {
    int ticks;
    PlyrPdata* opponent;
    AnimPdata* opponent_anim;
    MkProc* opponent_anim_proc;

    if (mode_of_play == 8) {
        blend_to_stance(0.1f);
        for (;;) {
            moves_sleep(60.0f);
        }
    }
    if (g_game_info.flag_bits.level_fatality_active ||
        g_game_info.flag_bits.level_fatality_done) {
        for (;;) {
            moves_sleep(60.0f);
        }
    }

    victory_proper_flip_flags = 0;
    set_my_state(0x4201);
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    plyr_obj->flags_0B_bits.bit6 = 1;
    plyr_obj->flags_09_bits.bit4 = 0;
    plyr_obj->flags_09_bits.head_tracking = 0;

    victory_wait_for_pre_animation();

    plyr_weapon_hide(plyr_pdata, 0, plyr_pdata->mirror_slots);
    if (plyr_pdata->runtime_data->script_138 == 0) {
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

    ticks = 240;
    opponent = plyr_pdata->his_plyr_pdata;
    opponent_anim_proc = MK_LIVE(opponent->anim_proc,
                                         opponent->anim_proc_instance);
    opponent_anim = (AnimPdata*)pdata_of_proc(opponent_anim_proc);
    while (opponent_anim->frame < opponent_anim->high_frame &&
           opponent->state != 0x4200 &&
           opponent->state != 0 &&
           opponent->state != 0x4203 && ticks != 0) {
        ticks--;
        moves_sleep(1.0f);
    }

    clear_both_face_opponent_flags();
    xfer_proc(plyr_anim_proc, p_anim_idle);
    if (is_big_boss(plyr_pdata) != 0) {
        active_cmdscript->unk28 =
            plyr_pdata->runtime_data->script_138;
        moves_jump(r_call_player_char_script_function);
        return 0.0f;
    }
    if (is_my_chest_to_screen() == 0) {
        if ((plyr_anim_pdata->flags & 8) == 0) {
            victory_proper_flip_flags = 8;
        }
        blend_to_ani(
            shared_ani.victory_turn, victory_proper_flip_flags | 3, 0.1f);
        ani_to_blend_frame(10.0f);
    }
    active_cmdscript->unk28 =
        plyr_pdata->runtime_data->script_138;
    moves_jump(r_call_player_char_script_function);
    return 0.0f;
}

int get_victory_flip_flags(void) {
    return victory_proper_flip_flags;
}

/* TODO: [near miss] 98.59%; retail order restored; shared bitfield/call 0/1 register coloring remains. */
float dizzy(void) {
    struct SwitchLogEntry* entry;
    PlyrPdata* player;
    MkObj* opponent;
    if (plyr_pdata->character_id == 1) {
        if (is_local_plyr() != 0) {
            while (is_weapon_style(plyr_pdata->fighter_definition)) {
                advance_active_moveset(plyr_pdata);
            }
        }
    } else {
        plyr_weapon_hide(plyr_pdata, 1, plyr_pdata->mirror_slots);
    }

    init_ground_move_no_aniproc();
    rotate_towards_him(0.2f);
    if (is_my_chest_to_screen() == 0) {
        plyr_anim_pdata->flags ^= 8;
        plyr_obj->hide_flag_bits.bit6 ^= 1;
        xfer_proc(plyr_anim_proc, p_animate);
        blend_to_ani(shared_ani.victory_turn, 3, 0.1f);
        ani_to_end();
    }
    set_my_state(0x4203);
    player = plyr_pdata;
    player->state_flags.bits.dizzy = 1;
    plyr_obj->flags_09_bits.head_tracking = 0;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    opponent = g_game_info.plyr1.slot.mirror_a;
    opponent->flags_09_bits.tightrope_restricted = 0;
    dizzy_kill_pfx(opponent, 0, player, 1);
    if (plyr_pdata->character_id == 0x1E) {
        blend_to_ani(plyr_pdata->dizzy_animation, 0, 0.1f);
    } else {
        blend_to_ani(shared_ani.dizzy, 0, 0.1f);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    while (f_fatality_finished == 0) {
        check_for_suicide();
        if (plyr_obj == g_game_info.plyr0.slot.mirror_a) {
            p1_current_log_index = p1_log_index;
            entry = &p1_switch_log[p1_log_index];
            p1_last_switch_bit = entry->switch_index;
            p1_last_switch_time = entry->tick;
            p1_current_switch_bit = entry->switch_index;
            p1_current_switch_time = entry->tick;
        } else {
            p2_current_log_index = p2_log_index;
            entry = &p2_switch_log[p2_log_index];
            p2_last_switch_bit = entry->switch_index;
            p2_last_switch_time = entry->tick;
            p2_current_switch_bit = entry->switch_index;
            p2_current_switch_time = entry->tick;
        }
        moves_sleep(1.0f);
    }
    moves_jump(fall_dead);
    return 0.0f;
}

void idle_his_anim_proc(void) {
    if (his_pdata != 0) {
        MkProc* proc = MK_LIVE(his_pdata->anim_proc, his_pdata->anim_proc_instance);

        if (proc != 0) {
            xfer_proc(proc, p_anim_idle);
        }
    }
}

void clear_his_f_constrained(void) {
    if (his_pdata != 0) {
        his_pdata->f_constrained = 0;
    }
}

static void check_for_suicide(void) {
    unsigned int* sequences = &jump_table[0].value;

    if (was_button_pressed(7) != 0) {
        switch (plyr_pdata->character_id) {
        case 0:
            scan_switch_sequences(&sequences[0x1D8 / 4]);
            break;
        case 4:
            scan_switch_sequences(&sequences[0x238 / 4]);
            break;
        case 0x16:
            scan_switch_sequences(&sequences[0x3F4 / 4]);
            break;
        case 0x17:
            scan_switch_sequences(&sequences[0x414 / 4]);
            break;
        case 0x1F:
            scan_switch_sequences(&sequences[0x4B0 / 4]);
            break;
        }
    } else if (was_button_pressed(4) != 0) {
        switch (plyr_pdata->character_id) {
        case 11:
            scan_switch_sequences(&sequences[0x2D8 / 4]);
            break;
        case 20:
            scan_switch_sequences(&sequences[0x3B8 / 4]);
            break;
        case 3:
            scan_switch_sequences(&sequences[0x218 / 4]);
            break;
        case 10:
            scan_switch_sequences(&sequences[0x1B8 / 4]);
            break;
        case 14:
            scan_switch_sequences(&sequences[0x318 / 4]);
            break;
        case 19:
            scan_switch_sequences(&sequences[0x398 / 4]);
            break;
        case 24:
            scan_switch_sequences(&sequences[0x434 / 4]);
            break;
        case 21:
            scan_switch_sequences(&sequences[0x3D4 / 4]);
            break;
        case 7:
            scan_switch_sequences(&sequences[0x278 / 4]);
            break;
        }
    } else if (was_button_pressed(6) != 0) {
        switch (plyr_pdata->character_id) {
        case 18:
            scan_switch_sequences(&sequences[0x378 / 4]);
            break;
        case 5:
            scan_switch_sequences(&sequences[0x198 / 4]);
            break;
        case 25:
            scan_switch_sequences(&sequences[0x450 / 4]);
            break;
        case 16:
            scan_switch_sequences(&sequences[0x358 / 4]);
            break;
        case 6:
            scan_switch_sequences(&sequences[0x258 / 4]);
            break;
        case 9:
            scan_switch_sequences(&sequences[0x2B8 / 4]);
            break;
        case 15:
            scan_switch_sequences(&sequences[0x338 / 4]);
            break;
        }
    } else if (was_button_pressed(5) != 0) {
        switch (plyr_pdata->character_id) {
        case 12:
            scan_switch_sequences(&sequences[0x2F8 / 4]);
            break;
        case 1:
            scan_switch_sequences(&sequences[0x1F8 / 4]);
            break;
        case 27:
            scan_switch_sequences(&sequences[0x470 / 4]);
            break;
        case 8:
            scan_switch_sequences(&sequences[0x298 / 4]);
            break;
        case 30:
            scan_switch_sequences(&sequences[0x490 / 4]);
            break;
        }
    }
}

float fall_dead(void) {
    init_ground_move_no_aniproc();
    set_my_state(0x4200);
    plyr_pdata->death_type = 1;
    plyr_obj->flags_09_bits.head_tracking = 0;
    plyr_obj->flags_09_bits.face_opponent = 0;
    if (mode_of_play == 8 &&
        trial_show_standard_fight_messages() == 0) {
        aproc->vtbl->jump_sleep(trial_run_loser_animation_script, 0.0f);
        return 0.0f;
    }
    blend_to_ani(shared_ani.fall_dead, 3, 0.1f);
    ani_to_frame_x(49.0f);
    got_hit_fx(4, 9, 1, 0, 0, 0.0f, 0);
    ani_to_end();
    aproc->vtbl->jump_sleep(j_stay_down_dead, 0.0f);
    return 0.0f;
}

/* TODO: [near miss] 98.06%; function order restored; four retail ani_1_frame
 * calls set r3=1 despite the void prototype; verify the callee ABI. */
float big_boss_end_of_round(void) {
    int ticks;

    if (do_i_have_life_left() == 0 || does_he_have_life_left() == 0) {
        init_ground_move_no_aniproc();
        set_my_state(0x4200);
        if (is_my_chest_to_screen() == 0) {
            ticks = 30;
            while (--ticks > 0) {
                force_midpoint_calculation_update = 1;
                ani_1_frame();
                moves_sleep(1.0f);
            }
            blend_to_ani(plyr_pdata->turn_to_screen_animation, 3, 0.1f);
            set_ani_speed(1.2f);
            ticks = 30;
            while (--ticks > 0) {
                force_midpoint_calculation_update = 1;
                ani_1_frame();
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
            ani_1_frame();
            moves_sleep(1.0f);
        }
        camera_idle();
        if ((unsigned short)randu0(100) < 50) {
            snd_req(0x1B4);
        } else {
            snd_req(0x1B5);
        }
        blend_to_ani(plyr_pdata->screen_taunt_animation, 3, 0.1f);
        ani_to_blend_frame(20.0f);
        xfer_camera(p_camera_proc, 0);

        if (xz_distance_between_players() < 7.5f) {
            blend_to_ani(plyr_pdata->turn_to_screen_animation, 3, 0.1f);
            set_ani_speed(1.2f);
            ticks = 30;
            while (--ticks > 0) {
                force_midpoint_calculation_update = 1;
                ani_1_frame();
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
            return 0.0f;
        }
        blend_to_stance(0.05f);
    }

    while (does_he_have_life_left() == 0 ||
           g_game_info.flag_bits.lens_flare_enabled == 0) {
        ani_loop_more_frames(1.0f);
    }
    plyr_obj->flags_09_bits.tightrope_restricted = 1;
    plyr_obj->flags_0B_bits.bit6 = 0;
    if (plyr_pdata->drone_request != 0) {
        moves_jump(drone_start);
        return 0.0f;
    }
    moves_jump(j_exit);
    return 0.0f;
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

float wall_dodge(void) {
    int side;
    int use_b;

    if (drone_ai_should_roll(1) == 0) {
        moves_jump(p_blend_to_stance_in_10);
        return 0.0f;
    }
    init_3d_move();
    side = 1 - plyr_pdata->script_exit_value_int;
    use_b = side != 0;
    if (is_my_chest_to_screen(side) == 0) {
        use_b ^= 1;
    }
    if (use_b != 0) {
        blend_to_ani(shared_ani.wall_dodge_b, 3, 0.1f);
    } else {
        blend_to_ani(shared_ani.wall_dodge_a, 3, 0.1f);
    }
    plyr_pdata->collision_disabled = 1;
    plyr_anim_pdata->step = 0.8f;
    if (use_b != 0) {
        moves_jump(p_blend_to_fstance_in_10);
        return 0.0f;
    }
    moves_jump(p_blend_to_stance_in_10);
    return 0.0f;
}

float j_front_roll_left(void) {
    if (is_my_chest_to_screen() != 0) {
        aproc->vtbl->jump_sleep(front_rollup_left, 0.0f);
        return 0.0f;
    }
    aproc->vtbl->jump_sleep(front_rollup_right, 0.0f);
    return 0.0f;
}

float j_front_roll_right(void) {
    if (is_my_chest_to_screen() != 0) {
        aproc->vtbl->jump_sleep(front_rollup_right, 0.0f);
        return 0.0f;
    }
    aproc->vtbl->jump_sleep(front_rollup_left, 0.0f);
    return 0.0f;
}

float j_back_rollup_OUT(void) {
    if (is_my_chest_to_screen() != 0) {
        aproc->vtbl->jump_sleep(back_rollup_right, 0.0f);
        return 0.0f;
    }
    aproc->vtbl->jump_sleep(back_rollup_left, 0.0f);
    return 0.0f;
}

float j_back_rollup_IN(void) {
    if (is_my_chest_to_screen() != 0) {
        aproc->vtbl->jump_sleep(back_rollup_left, 0.0f);
        return 0.0f;
    }
    aproc->vtbl->jump_sleep(back_rollup_right, 0.0f);
    return 0.0f;
}

static float front_rollup_right(void) {
    init_3d_move();
    blend_to_ani(shared_ani.front_roll_right, 3, 0.2f);
    aproc->vtbl->jump_sleep(rollup_finish, 0.0f);
    return 0.0f;
}

static float front_rollup_left(void) {
    init_3d_move();
    blend_to_ani(shared_ani.front_roll_left, 3, 0.2f);
    aproc->vtbl->jump_sleep(rollup_finish, 0.0f);
    return 0.0f;
}

float front_rollup(void) {
    tightrope_restrictions_on();
    blend_to_ani(shared_ani.front_rollup, 3, 0.1f);
    plyr_anim_pdata->step = 1.2f;
    aproc->vtbl->jump_sleep(p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

static float back_rollup_right(void) {
    init_3d_move();
    blend_to_ani(shared_ani.back_roll_right, 3, 0.2f);
    aproc->vtbl->jump_sleep(rollup_finish, 0.0f);
    return 0.0f;
}

static float back_rollup_left(void) {
    init_3d_move();
    blend_to_ani(shared_ani.back_roll_left, 3, 0.2f);
    aproc->vtbl->jump_sleep(rollup_finish, 0.0f);
    return 0.0f;
}

static float rollup_finish(void) {
    plyr_anim_pdata->step = 1.0f;
    ani_x_more_frames(15.0f);
    random_voice(9);
    ani_to_blend_frame(15.0f);
    plyr_pdata->summon_position_x = 15.0f;
    aproc->vtbl->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

float j_ass_rollup(void) {
    blend_to_ani(shared_ani.ass_rollup, 3, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    ani_to_blend_frame(10.0f);
    blend_to_fstance(0.05f);
    aproc->vtbl->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

int get_fatality_available_flag(void) {
    return f_fatality_available;
}

float rotate_toward_j_exit(void) {
    rotate_towards_him(0.2f);
    moves_jump(j_exit);
    return 0.0f;
}

#undef MOVES_BLOCK_BODY
