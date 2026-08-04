/*
 * Port readiness:
 *   Structs: CLEAN
 *   Matching: PARTIAL
 *   Linked: NO
 *   Status: REVIEW
 *   Gaps: reaction transfer, damage, animation, and cleanup implementations
 */
#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/plyr_pdata.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"
#include "game/game_info.h"

extern MkObj* his_obj;
extern MkObj* plyr_obj;
extern PlyrPdata* his_pdata;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;

typedef struct ReactionProcVtable ReactionProcVtable;
struct ReactionProcVtable {
    void* reserved[6];
    void (*sleep)(ReactionProcVtable* vtable);
    void* reserved_after_sleep[2];
    int (*jump_sleep)(MkProcEntryFn entry, float ticks);
};

typedef struct LoadableReactionScript {
    int slot_count;
    int script;
    int flags;
    int power_level;
    int state;
} LoadableReactionScript;

typedef struct ReactionDamagePdata {
    char pad00[0x284];
    unsigned int damage_boost_until;
    float damage_boost;
    char pad28C[8];
    float accumulated_damage;
} ReactionDamagePdata;

typedef struct ReactionCurrentPdata {
    char pad00[0x2F8];
    unsigned int reaction_index;
} ReactionCurrentPdata;

typedef struct ReactionXferAddress {
    int reaction;
    int script;
    int power_level;
    int state;
    int flags;
} ReactionXferAddress;

typedef struct ReactionFighterDefinitionView {
    char pad00[0x158];
    int judo_throw_reaction;
} ReactionFighterDefinitionView;

typedef struct ReactionSharedAnimations {
    void* jax_piston_high;       /* +0x000 */
    int chest_stumble;           /* +0x004 */
    char pad008[0x54];
    void* jax_piston_low;        /* +0x05C */
    char pad060[0x2C];
    void* gut_on_butt;           /* +0x08C */
    char pad090[4];
    void* gut_on_feet;           /* +0x094 */
    char pad098[0x34];
    void* enough_air;            /* +0x0CC */
    char pad0D0[0x2C];
    void* cyrus_stomp;           /* +0x0FC */
    char pad100[0x30];
    void* feet_hit;              /* +0x130 */
    char pad134[8];
    int swept_in;                /* +0x13C */
    int swept_reverse;           /* +0x140 */
    int swept_out;               /* +0x144 */
    char pad148[0x0C];
    void* falling_back;          /* +0x154 */
    void* side_head_spin;        /* +0x158 */
    char pad15C[0x0C];
    void* side_head_dive;        /* +0x168 */
    char pad16C[0x0C];
    void* top_of_head_slam;      /* +0x178 */
    void* head_slam_fall;        /* +0x17C */
    char pad180[0x24];
    void* wall_hit;              /* +0x1A4 */
    char pad1A8[0x0C];
    void* jump_chin;             /* +0x1B4 */
    void* jump_slambounce;       /* +0x1B8 */
    char pad1BC[0x5C];
    int post_surf_getup;         /* +0x218 */
    char pad21C[0x0C];
    int throw_getup;             /* +0x228 */
    char pad22C[0x60];
    void* throw_fall;            /* +0x28C */
    char pad290[0x60];
    void* counter_caught;        /* +0x2F0 */
    char pad2F4[0x1C];
    void* counter_caught_6;      /* +0x310 */
    void* counter_caught_7;      /* +0x314 */
    void* counter_caught_8;      /* +0x318 */
    void* counter_caught_9;      /* +0x31C */
    int combo_breaker;           /* +0x320 */
} ReactionSharedAnimations;

typedef struct ReactionExtendedPdata {
    char pad000[0x368];
    void* reaction_animation;
    void* reaction_animation_a;
    void* reaction_animation_b;
    void* reaction_animation_c;
    void* esp1_reaction_animation;
    void* scorpion_spear_hit;
    void* scorpion_spear_pull;
    void* scorpion_spear_recover;
} ReactionExtendedPdata;

typedef struct ReactionPostSurfPdata {
    char pad000[0x6F4];
    int stay_down;
} ReactionPostSurfPdata;

typedef struct ReactionWallPdata {
    char pad000[0x258];
    int wall_hit_count;
} ReactionWallPdata;

typedef struct ReactionStatusFlagsView {
    char pad000[0x134];
    unsigned int cleanup_function;
} ReactionStatusFlagsView;

typedef struct ReactionImageFaderPdata {
    MkHdr hdr;
    ScreenObj* object;
    unsigned int object_instance;
    int direction;
    int alpha;
    int delay;
} ReactionImageFaderPdata;

typedef struct ReactionImageSource {
    MkHdr hdr;
    char pad08[0x54];
    MkObj* object;
} ReactionImageSource;

typedef struct ReactionBladeTransform {
    char pad000[0x60];
    Vec position;
} ReactionBladeTransform;

typedef struct ReactionBladeRoot {
    char pad000[4];
    ReactionBladeTransform* transform;
} ReactionBladeRoot;

typedef struct ReactionBladeFighterDefinition {
    char pad000[4];
    ReactionBladeRoot* blade_root;
    char pad008[4];
    MkObj* blade_object;
    unsigned int blade_object_instance;
} ReactionBladeFighterDefinition;

static LoadableReactionScript g_loadable_reaction_scripts[8] = {
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
    {5, 0, 0, 0, 0}, {5, 0, 0, 0, 0},
};

extern const ReactionXferAddress tbl_xfer_addresses[];

unsigned int fx_by_owner(const char* name, int owner);
unsigned int fx_next_emitter(unsigned int effect);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void* mk_chess_launch_fx_at_pos_with_obj_emit_based(
    unsigned int effect, float x, float y, float z);
void trial_increment_state_value(int player, int state, int amount);
void low_flash_check(void);
void face_opponent_now(void);
void random_voice(int group);
void blend_to_ani(void* animation, int transition, float blend);
void ani_to_end(void);
float j_blend_to_stance_in_x(void);
void reaction_xfer_him(int reaction, float rate, int strength);
void stop_me(void);
void init_ground_move(void);
void blocked_fx(int type, int bone, int third, int fourth, int fifth);
void force_away(int direction, int ticks, float speed, float damping);
void disable_my_attacks(int ticks);
void adjust_my_damage_multiplier(float multiplier);
void got_hit_fx(
    int first, int second, int third, int fourth, int fifth, int sixth,
    float value);
void random_hit(int group);
float j_block_common_reaction(void);
float chest_stumble_both(void);
float j_exit(void);
float j_blend_to_fstance_in_x(void);
float j_getup_back_12(void);
void blend_to_stance(float rate);
int blend_to_fstance(float rate);
void freeze_player(void);
void unfreeze_player(void);
void set_my_state(int state);
void glitch_to_ani(int animation, int transition);
void adjust_p1_life(float amount);
void adjust_p2_life(float amount);
int should_weapon_block(PlyrPdata* player);

extern int game_tick_ctr;
extern ScriptSlot* reactions_cmo;
extern ReactionSharedAnimations shared_ani;

void ani_to_frame_x(float frame);
void ani_to_blend_frame(float frame);
void ani_to_frame_x_call(void (*callback)(void), float frame);
void add_facial_damage(float amount);
void check_for_combo_message(void);
void disable_both_repel_flags(void);
void head_tracking_on(void);
void init_air_move(void);
void p_blend_to_stance_in_10(void);
float p_animate(void);
float r_complete_ermac_slam(void);
float r_face3_onback(void);
void set_ani_speed(float speed);
void set_anim_hiframe(float frame);
void special_move_cam_setup(
    int mode, int ticks, int flags, float x, float y, float z,
    float distance, float speed);
void update_bone_hierarchy(MkHdr* object);
void ground_me(MkHdr* object);
void wall_eligible_on(void);
void blend_to_ani_frame(
    int animation, int transition, float blend, float frame);
float j_getup_back_6(void);
float j_getup_front_12(void);
float j_getup_back_3(void);
float j_getup_back_9(void);
float j_getup_back_12(void);
float j_getup_sit_12(void);
float j_exit_blend_stance(void);
float j_stay_down_dead(void);
void ani_to_fall_to_frame(
    int sound_id, float landing_frame, float target_frame);
void back_rollup_check(void);
void danger_zone_eligible_on(void);
void face_bleed_me(int size);
void init_air_move_no_aniproc(void);
void land_chores(
    int land_sound, int second_sound,
    float shake_ticks, float shake_strength);
void launch_n_land_ani(
    void* animation, int flags, float x, float z, float frame,
    float speed, float gravity, float blend);
void myvel_his_angle_y(float y, float x, float z);
void newani_to_frame_x(
    void* animation, int flags, float frame, float x, float z, float blend);
void player_feet_land_chores(void);
void set_anim_script(AnimPdata* animation, int script, int transition);
void shake_camera(int ticks, float strength);
void shake_hit_voice(
    int shake_ticks, int hit_voice, int fighter_voice, float rumble_scale);
void start_blood_particles(
    int script, int bone, PlyrPdata* player, MkObj* object);
int stay_down_check(void);
void tightrope_restrictions_off(void);
void tightrope_restrictions_on(void);
extern void (*large_ground_fx)(void);
void ani_1_frame(void);
void ani_loop_more_frames(float frames);
void blend_to_ani_INOUT(
    int in_animation, int out_animation, float blend_rate,
    float in_speed, float out_speed);
void disable_blocking(void);
void ejb_call(int command);
float fpick_a_float(float normal, float flipped_value);
int is_he_airborn(void);
void launch_me_up(float vertical_velocity, float gravity);
void myvel_his_angle_y_inout(float y, float x, float z);
int my_joypad_state_5(void);
void* start_blood_particles_scripts(int script, int bone);
void wait_to_land(void);
float xz_distance_between_players(void);
float p_joy_loop(void);
float wall_dodge(void);
float r_counter_caught_abort(void);
MslSoundHandle snd_req(int sound_id);
int emitter_id_from_handle(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
void fx_set_param_v3(
    unsigned int effect, int parameter, float x, float y, float z);
void fx_resume_emit(unsigned int effect);
float p_image_fader(void);
void get_bone_offset_world_pos(
    MkObj* object, int bone, const Vec* offset, Vec* position);
void camera_get_screen_pos_from_world_pos(
    const Vec* world, Vec* screen);

ScreenObj* display_image_by_plyr(
    int slot, const char* image_name,
    ReactionImageSource* source, float y_offset) {
    ReactionImageFaderPdata* fader;
    ScreenObj* image;
    Vec bone_offset = {0.0f, 0.0f, 0.0f};
    Vec world_position;
    Vec screen_position;

    image = load_named_2d_pfxobj(
        slot, 0xC021, image_name, 0, 0x2F);
    get_bone_offset_world_pos(
        source->object, 9, &bone_offset, &world_position);
    world_position.y = y_offset + g_game_info.field_34;
    camera_get_screen_pos_from_world_pos(
        &world_position, &screen_position);
    image->x =
        (int)screen_position.x - image->pfx2d->tex_w / 2;
    image->y = (int)screen_position.y;

    fader = 0;
    if (_create_mkproc_generic_nostack(
            0xC02A, 0x1F, p_image_fader,
            sizeof(ReactionImageFaderPdata),
            (MkHdr**)&fader) != 0) {
        fader->object = image;
        fader->object_instance = image->instance;
        fader->direction = source->hdr.instance;
        fader->alpha = 0xFF;
        fader->delay = 60;
    }
    return image;
}

void flash_hit_at_bid_with_y(float y_offset) {
    unsigned int effect;
    Vec position;

    if (plyr_pdata->plyr_num == 0) {
        effect = fx_by_owner("hit_fx", 1);
    } else {
        effect = fx_by_owner("hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    get_bone_world_pos(plyr_obj, 0, &position);
    position.y = y_offset + g_game_info.field_34;
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void flash_hit_at_bid(int bone) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    object = plyr_obj;
    if (plyr_pdata->plyr_num == 0) {
        effect = fx_by_owner("hit_fx", 1);
    } else {
        effect = fx_by_owner("hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    get_bone_world_pos(object, bone, &position);
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void low_flash_check(void) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    if (plyr_pdata->hit_flash_enabled != 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x1F, 1);
        object = plyr_obj;
        if (plyr_pdata->plyr_num == 0) {
            effect = fx_by_owner("hit_fx", 1);
        } else {
            effect = fx_by_owner("hit_fx", 2);
        }
        effect = fx_next_emitter(effect);
        get_bone_world_pos(object, 0, &position);
        position.y = 0.5f + g_game_info.field_34;
        mk_chess_launch_fx_at_pos_with_obj_emit_based(
            effect, position.x, position.y, position.z);
    }
}

void medium_flash_check(void) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    if (plyr_pdata->hit_flash_enabled != 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x1E, 1);
        object = plyr_obj;
        if (plyr_pdata->plyr_num == 0) {
            effect = fx_by_owner("hit_fx", 1);
        } else {
            effect = fx_by_owner("hit_fx", 2);
        }
        effect = fx_next_emitter(effect);
        get_bone_world_pos(object, 0, &position);
        position.y = 1.15f + g_game_info.field_34;
        mk_chess_launch_fx_at_pos_with_obj_emit_based(
            effect, position.x, position.y, position.z);
    }
}

void high_flash_check(void) {
    unsigned int effect;
    MkObj* object;
    Vec position;

    if (plyr_pdata->hit_flash_enabled != 0) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x1D, 1);
        object = plyr_obj;
        if (plyr_pdata->plyr_num == 0) {
            effect = fx_by_owner("hit_fx", 1);
        } else {
            effect = fx_by_owner("hit_fx", 2);
        }
        effect = fx_next_emitter(effect);
        get_bone_world_pos(object, 0, &position);
        position.y = 1.7f + g_game_info.field_34;
        mk_chess_launch_fx_at_pos_with_obj_emit_based(
            effect, position.x, position.y, position.z);
    }
}

void fight_fx_im_hit_with_breaker_flash(
    int player,
    MkObj* object,
    int bone,
    int use_bone,
    void* script_args,
    float y_offset) {
    unsigned int effect;
    Vec position;

    (void)script_args;
    if (player == 0) {
        effect = fx_by_owner("breaker_hit_fx", 1);
    } else {
        effect = fx_by_owner("breaker_hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    if (use_bone == 0) {
        get_bone_world_pos(object, 0, &position);
        position.y = y_offset + g_game_info.field_34;
    } else {
        get_bone_world_pos(object, bone, &position);
    }
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void fight_fx_im_hit_flash(
    int player,
    MkObj* object,
    int bone,
    int use_bone,
    void* script_args,
    float y_offset) {
    unsigned int effect;
    Vec position;

    (void)script_args;
    if (player == 0) {
        effect = fx_by_owner("hit_fx", 1);
    } else {
        effect = fx_by_owner("hit_fx", 2);
    }
    effect = fx_next_emitter(effect);
    if (use_bone == 0) {
        get_bone_world_pos(object, 0, &position);
        position.y = y_offset + g_game_info.field_34;
    } else {
        get_bone_world_pos(object, bone, &position);
    }
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void general_flash_fx(
    int player,
    MkObj* object,
    const char* effect_name,
    int bone,
    int use_bone,
    float y_offset) {
    unsigned int effect;
    Vec position;

    if (player == 0) {
        effect = fx_by_owner(effect_name, 1);
    } else {
        effect = fx_by_owner(effect_name, 2);
    }
    effect = fx_next_emitter(effect);
    if (use_bone == 0) {
        get_bone_world_pos(object, 0, &position);
        position.y = y_offset + g_game_info.field_34;
    } else {
        get_bone_world_pos(object, bone, &position);
    }
    mk_chess_launch_fx_at_pos_with_obj_emit_based(
        effect, position.x, position.y, position.z);
}

void load_script_as_reaction(unsigned int slot, int script) {
    if (slot <= 7U) {
        g_loadable_reaction_scripts[slot].script = script;
    }
}

/* Soft ceiling: reaction_xfer_him_nohit ~99.71% -- pool-label noise only. */
void reaction_xfer_him_nohit(int reaction) {
    int hit_count;

    if (his_pdata->blocking_disabled_2 != 0) {
        hit_count = his_pdata->hit_count;
        his_pdata->hit_count = hit_count - 1;
        reaction_xfer_him(reaction, 0.0f, 2);
    }
}

/*
 * Near miss: the two current-reaction accessors are semantically complete.
 * MWCC keeps the table index in r0 here instead of retail's r3 and omits a
 * redundant addis-by-zero before the 0xFFFF sentinel test.
 */
int reaction_fetch_current_flags(int player) {
    unsigned int reaction_index;

    reaction_index =
        ((ReactionCurrentPdata*)g_game_info.plyr0.slot.pdata)->reaction_index;
    if (player == 1) {
        reaction_index =
            ((ReactionCurrentPdata*)g_game_info.plyr1.slot.pdata)
                ->reaction_index;
    }
    if (reaction_index == 0xFFFFU) {
        return 0;
    }
    return tbl_xfer_addresses[reaction_index].flags;
}

int reaction_fetch_current_power_level(int player) {
    unsigned int reaction_index;

    reaction_index =
        ((ReactionCurrentPdata*)g_game_info.plyr0.slot.pdata)->reaction_index;
    if (player == 1) {
        reaction_index =
            ((ReactionCurrentPdata*)g_game_info.plyr1.slot.pdata)
                ->reaction_index;
    }
    if (reaction_index == 0xFFFFU) {
        return 2;
    }
    return tbl_xfer_addresses[reaction_index].power_level;
}

/*
 * Soft ceiling: the five damage leaves are opcode-identical apart from
 * TU-local float-pool labels and the commutative fmuls operand order.
 */
void damage_player(PlyrPdata* source, float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    if (source->plyr_num == 0) {
        boost_source =
            (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
        damage = amount * 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
            ->accumulated_damage += damage;
        return;
    }

    boost_source = (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
    damage = amount * 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p2_life(-damage);
    ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)->accumulated_damage +=
        damage;
}

void damage_him(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
            damage = amount * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
            return;
        }

        boost_source =
            (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
        damage = amount * 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p1_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
            ->accumulated_damage += damage;
    }
}

void damage_me(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
            damage = amount * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
            return;
        }

        boost_source =
            (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
        damage = amount * 0.8f;
        if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
            damage *= boost_source->damage_boost;
        }
        if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
            damage *= 1.15f;
        }
        adjust_p2_life(-damage);
        ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
            ->accumulated_damage += damage;
    }
}

void damage_p2(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    boost_source = (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
    damage = amount * 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p2_life(-damage);
    ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)->accumulated_damage +=
        damage;
}

void damage_p1(float amount) {
    ReactionDamagePdata* boost_source;
    float damage;

    boost_source = (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
    damage = amount * 0.8f;
    if (boost_source->damage_boost_until > (unsigned int)game_tick_ctr) {
        damage *= boost_source->damage_boost;
    }
    if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
        damage *= 1.15f;
    }
    adjust_p1_life(-damage);
    ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)->accumulated_damage +=
        damage;
}

/*
 * Soft ceiling: these three script-call wrappers are opcode-identical to
 * retail; objdiff only distinguishes their TU-local zero-float labels.
 */
float r_call_other_player_char_script_function(void) {
    PlyrPdata* other;

    cmdscript_reset_stack();
    other = plyr_pdata->his_plyr_pdata;
    cmdscript_setup_execution(other->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->his_plyr_pdata->cmo);
    return 0.0f;
}

float r_call_player_char_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(plyr_pdata->cmo, active_cmdscript->unk28);
    call_player_script_function(plyr_pdata->cmo);
    return 0.0f;
}

float r_call_script_function(void) {
    cmdscript_reset_stack();
    cmdscript_setup_execution(reactions_cmo, active_cmdscript->unk28);
    call_player_script_function(reactions_cmo);
    return 0.0f;
}

/*
 * Soft ceiling: these terminal reaction leaves are opcode-identical to retail;
 * objdiff only distinguishes their TU-local float-pool labels.
 */
float r_iceball_reversal(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    set_my_state(0xC600);
    plyr_pdata->blocking_disabled = 1;
    plyr_pdata->blocking_disabled_2 = 1;
    plyr_obj->flags_09_bits.face_opponent = 0;
    freeze_player();
    _mkproc_sleep_ticks = 180.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    unfreeze_player();
    set_my_state(0);
    plyr_pdata->summon_position_x = 25.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

float r_null(void) {
    ReactionProcVtable* vtable;

    _mkproc_sleep_ticks = 8.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_stance(0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_judo_throw1(void) {
    ReactionProcVtable* vtable;
    ReactionFighterDefinitionView* fighter;

    fighter = (ReactionFighterDefinitionView*)his_pdata->fighter_definition;
    glitch_to_ani(fighter->judo_throw_reaction, 3);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_12, 0.0f);
    return 0.0f;
}

float r_counter_caught_abort(void) {
    ReactionProcVtable* vtable;

    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    blend_to_fstance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

#pragma dont_inline on
void r_top_of_head_slam(void) {
    high_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.05f);
    random_hit(3);
    blend_to_ani(shared_ani.top_of_head_slam, 3, 0.5f);
    plyr_anim_pdata->step = 1.5f;
    ani_to_frame_x(9.0f);
    plyr_anim_pdata->step = 2.5f;
    ani_to_blend_frame(4.0f);
}
#pragma dont_inline reset

#pragma dont_inline on

float j_counter_caught(void) {
    ReactionProcVtable* vtable;
    int reaction;

    init_ground_move_no_aniproc();
    stop_me();
    random_voice(9);
    shake_hit_voice(2, 6, 5, 0.02f);
    disable_blocking();
    reaction = plyr_pdata->script_exit_value_int;
    if (reaction == 8) {
        blend_to_ani(shared_ani.counter_caught_8, 0, 0.1f);
    } else if (reaction < 8) {
        if (reaction == 6) {
            blend_to_ani(shared_ani.counter_caught_6, 0, 0.1f);
        } else if (reaction >= 6) {
            blend_to_ani(shared_ani.counter_caught_7, 0, 0.1f);
        }
    } else if (reaction < 0xA) {
        blend_to_ani(shared_ani.counter_caught_9, 0, 0.1f);
    }
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    _mkproc_sleep_ticks = 22.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    while (his_pdata->state == 0x4000) {
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
    }
    _mkproc_sleep_ticks = 15.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_counter_caught_abort, 0.0f);
    return 0.0f;
}

float r_combo_breaker(void) {
    ReactionProcVtable* vtable;

    trial_increment_state_value(plyr_pdata->plyr_num, 0x20, 0);
    adjust_my_damage_multiplier(0.75f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    plyr_anim_pdata->weight = 1.3f;
    plyr_anim_pdata->step = 0.7f;
    face_opponent_now();
    got_hit_fx(2, 5, 2, 0, 0, 0, 0.0f);
    reaction_xfer_him(0x79, 0.0f, 2);
    _mkproc_sleep_ticks = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    plyr_obj->flags_09_bits.bit6 = 1;
    blend_to_ani_frame(shared_ani.combo_breaker, 3, 0.33f, 7.0f);
    set_ani_speed(1.0f);
    ani_to_frame_x(11.0f);
    reaction_xfer_him(0x7A, 0.0f, 2);
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    set_ani_speed(0.5f);
    ani_to_frame_x(25.0f);
    ani_to_blend_frame(3.0f);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_mileena_hit(void) {
    ReactionProcVtable* vtable;
    ReactionDamagePdata* boost_source;
    float damage;

    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
            damage = 0.12f * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
        } else {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
            damage = 0.12f * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
        }
    }
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_face3_onback, 0.0f);
    return 0.0f;
}

float r_popup_final_hitter(void) {
    ReactionProcVtable* vtable;
    int ticks;

    head_tracking_on();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    if (xz_distance_between_players() < 1.0f) {
        force_away(9, 8, 0.1f, 0.9f);
    }
    xfer_proc(plyr_anim_proc, p_animate);
    set_ani_speed(0.33f);
    _mkproc_sleep_ticks = 30.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_fstance(0.05f);
    ticks = 0x28;
    while (is_he_airborn() == 1 && ticks > 0) {
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
        ticks--;
    }
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_jump_chin3_final_hit(void) {
    ReactionProcVtable* vtable;

    special_move_cam_setup(
        0xA, 0x3C, 0, 1.47f, 4.1f, 1.0f, -1.75f, -0.15f);
    reaction_xfer_him(0xDC, 0.0f, 2);
    face_opponent_now();
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_obj->pos_y = his_obj->pos_y;
    stop_me();
    init_air_move();
    plyr_obj->flags_09_bits.launched = 0;
    shake_hit_voice(2, 4, 3, 0.02f);
    start_blood_particles_scripts(0x39, 0x10);
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    set_my_state(0x605);
    blend_to_ani(shared_ani.jump_chin, 3, 0.2f);
    set_ani_speed(0.7f);
    ani_to_frame_x(43.0f);
    plyr_obj->gravity = -0.1f;
    wait_to_land();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    got_hit_fx(4, 0, 1, 0, 0, 1, 0.0f);
    set_my_state(0x600);
    back_rollup_check();
    ani_to_frame_x(60.0f);
    set_ani_speed(2.0f);
    init_ground_move();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_sidehead3_spin(void) {
    ReactionProcVtable* vtable;
    float flight_ticks;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    myvel_his_angle_y(-0.71f, 0.03f, 0.03f);
    plyr_anim_pdata->step = 0.7f;
    blend_to_ani(shared_ani.side_head_spin, 3, 0.33f);
    launch_me_up(0.06f, -0.003f);
    flight_ticks = 2.0f * (plyr_obj->pos_vel.y / plyr_obj->gravity);
    if (flight_ticks < 0.0f) {
        flight_ticks = -flight_ticks;
    }
    plyr_anim_pdata->step = 27.0f / flight_ticks;
    ani_to_frame_x(27.0f);
    wait_to_land();
    tightrope_restrictions_on();
    shake_camera(3, 0.03f);
    ani_to_end();
    blend_to_stance(0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(p_joy_loop, 0.0f);
    return 0.0f;
}

float r_feet3_sweptout_rev(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.face_opponent = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    blend_to_ani_INOUT(
        shared_ani.swept_reverse, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    myvel_his_angle_y_inout(-1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

float r_feet3_swept_in(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
    plyr_obj->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    blend_to_ani_INOUT(
        shared_ani.swept_in, shared_ani.swept_out,
        0.2f, 0.8f, 1.0f);
    myvel_his_angle_y_inout(1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

float r_feet3_swept_out(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
    plyr_obj->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.tightrope_restricted = 0;
    blend_to_ani_INOUT(
        shared_ani.swept_out, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    myvel_his_angle_y_inout(-1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

float r_feet3_sweptin_rev(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 7, 0, 0, 0, 0x10, 0.0f);
    tightrope_restrictions_off();
    blend_to_ani_INOUT(
        shared_ani.swept_reverse, shared_ani.swept_in,
        0.2f, 1.0f, 0.8f);
    myvel_his_angle_y_inout(1.57f, 0.08f, 0.08f);
    ani_to_frame_x(fpick_a_float(20.0f, 20.0f));
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_9, 0.0f);
    return 0.0f;
}

float r_esp1_B(void) {
    ReactionProcVtable* vtable;
    ReactionExtendedPdata* opponent;
    ReactionDamagePdata* boost_source;
    float damage;
    int ticks;

    face_opponent_now();
    plyr_obj->gravity = 0.0f;
    plyr_obj->pos_vel.y = 0.0f;
    plyr_obj->flags_09_bits.face_opponent = 0;
    random_voice(0xD);
    wall_eligible_on();
    myvel_his_angle_y(0.0f, -0.16f, -0.16f);
    opponent = (ReactionExtendedPdata*)his_pdata;
    blend_to_ani(opponent->reaction_animation_a, 3, 0.1f);
    ani_to_end();
    blend_to_ani(opponent->reaction_animation_b, 0, 0.1f);
    ticks = 0x1E;
    while (xz_distance_between_players() > 2.25f && ticks > 0) {
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
        ticks--;
    }
    ani_loop_more_frames(7.0f);
    blend_to_ani(opponent->reaction_animation_c, 3, 0.1f);
    ani_to_frame_x(19.0f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
            damage = 0.14f * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
        } else {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
            damage = 0.14f * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
        }
    }
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

float r_scorpion_spear_1(void) {
    ReactionProcVtable* vtable;
    ReactionExtendedPdata* opponent;

    high_flash_check();
    face_opponent_now();
    stop_me();
    init_air_move();
    ejb_call(0x31);
    set_my_state(0x603);
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    start_blood_particles(0x20, 9, plyr_pdata, plyr_obj);
    opponent = (ReactionExtendedPdata*)his_pdata;
    blend_to_ani(opponent->scorpion_spear_hit, 3, 0.1f);
    ani_to_frame_x(83.0f);
    set_my_state(0x604);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float r_scorpion_spear_2(void) {
    ReactionProcVtable* vtable;
    ReactionExtendedPdata* opponent;
    int ticks;

    opponent = (ReactionExtendedPdata*)his_pdata;
    blend_to_ani(opponent->scorpion_spear_pull, 3, 0.1f);
    ani_to_end();
    snd_req(0xD70);
    myvel_his_angle_y(0.0f, -0.13f, -0.13f);
    blend_to_ani(opponent->scorpion_spear_recover, 0, 0.1f);
    plyr_anim_pdata->step = 0.75f;
    ticks = 0;
    while (xz_distance_between_players() > 1.0f && ticks < 0x50) {
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
        ticks++;
    }
    init_ground_move();
    stop_me();
    set_my_state(0x4204);
    if (his_pdata->character_id == 0x19 ||
        his_pdata->character_id == 0x1A) {
        blend_to_ani(opponent->reaction_animation_c, 3, 0.2f);
    } else {
        blend_to_ani(opponent->reaction_animation, 3, 0.2f);
    }
    plyr_anim_pdata->step = 1.2f;
    plyr_pdata->summon_position_x = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_fstance_in_x, 0.0f);
    return 0.0f;
}

float r_hit_wall(void) {
    ReactionProcVtable* vtable;
    ReactionDamagePdata* boost_source;
    ReactionWallPdata* player;
    ReactionWallPdata* opponent;
    float damage;

    trial_increment_state_value(plyr_pdata->plyr_num, 0x1C, 1);
    adjust_my_damage_multiplier(0.6f);
    set_my_state(0x601);
    init_air_move_no_aniproc();
    if (plyr_pdata != 0) {
        if (aproc->pid == 0x1001) {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr1.slot.pdata;
            damage = 0.06f * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr0.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p1_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr0.slot.pdata)
                ->accumulated_damage += damage;
        } else {
            boost_source =
                (ReactionDamagePdata*)g_game_info.plyr0.slot.pdata;
            damage = 0.06f * 0.8f;
            if (boost_source->damage_boost_until >
                (unsigned int)game_tick_ctr) {
                damage *= boost_source->damage_boost;
            }
            if (should_weapon_block(g_game_info.plyr1.slot.pdata) != 0) {
                damage *= 1.15f;
            }
            adjust_p2_life(-damage);
            ((ReactionDamagePdata*)g_game_info.plyr1.slot.pdata)
                ->accumulated_damage += damage;
        }
    }
    player = (ReactionWallPdata*)plyr_pdata;
    opponent = (ReactionWallPdata*)plyr_pdata->his_plyr_pdata;
    plyr_pdata->hit_count++;
    player->wall_hit_count++;
    opponent->wall_hit_count = 0;
    shake_hit_voice(2, 9, 8, 0.02f);
    blend_to_ani(shared_ani.wall_hit, 3, 0.5f);
    plyr_anim_pdata->step = 0.8f;
    ani_to_frame_x(7.0f);
    plyr_obj->gravity = -0.0075f;
    if (plyr_pdata->drone_request != 0) {
        plyr_pdata->script_exit_value_int = (randu0(2) & 0xFFFF) + 1;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(wall_dodge, 0.0f);
        return 0.0f;
    }
    while (plyr_anim_pdata->frame < 23.0f) {
        plyr_pdata->script_exit_value_int = my_joypad_state_5();
        if (plyr_pdata->script_exit_value_int == 1 ||
            plyr_pdata->script_exit_value_int == 2) {
            vtable = (ReactionProcVtable*)aproc->vtbl;
            vtable->jump_sleep(wall_dodge, 0.0f);
            return 0.0f;
        }
        ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->sleep(vtable);
    }
    ani_to_blend_frame(10.0f);
    plyr_obj->flags_09_bits.wall_restricted = 0;
    check_for_combo_message();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

#pragma dont_inline reset

void run_reaction_cleanup_function(PlyrPdata* player) {
    ReactionStatusFlagsView* status;
    CmdScript* saved_script;
    PlyrPdata* saved_player;
    PlyrPdata* saved_opponent;
    MkObj* saved_object;
    MkObj* saved_opponent_object;
    MkObj* object;
    MkObj* opponent_object;

    if (player == 0) {
        return;
    }
    status = (ReactionStatusFlagsView*)player->status_flags;
    if (status->cleanup_function == 0) {
        return;
    }

    saved_player = plyr_pdata;
    saved_opponent = his_pdata;
    plyr_pdata = player;
    saved_object = plyr_obj;
    saved_opponent_object = his_obj;
    his_pdata = player->his_plyr_pdata;

    object = player->tracked_obj;
    if (object != 0 &&
        object->hdr.instance != player->tracked_obj_instance) {
        object = 0;
    }
    plyr_obj = object;

    opponent_object = his_pdata->tracked_obj;
    if (opponent_object != 0 &&
        opponent_object->hdr.instance != his_pdata->tracked_obj_instance) {
        opponent_object = 0;
    }
    his_obj = opponent_object;

    if (object == 0 || opponent_object == 0) {
        return;
    }

    saved_script = active_cmdscript;
    active_cmdscript = &global_script_interpreter;
    cmdscript_set_parameters(&global_script_interpreter, 1, player);
    cmdscript_setup_execution(player->cmo, status->cleanup_function);
    cmdscript_execute(player->cmo);
    active_cmdscript = saved_script;
    plyr_pdata = saved_player;
    his_pdata = saved_opponent;
    plyr_obj = saved_object;
    his_obj = saved_opponent_object;
}

float p_image_fader(void) {
    ReactionImageFaderPdata* pdata;
    ScreenObj* object;

    pdata = (ReactionImageFaderPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    if (pdata->delay > 0) {
        object = pdata->object;
        if (object != 0 && object->instance != pdata->object_instance) {
            object = 0;
        }
        if (object != 0) {
            if (pdata->direction == 0) {
                object->x--;
                object->y++;
            } else {
                object->x++;
                object->y++;
            }
        }
        pdata->delay--;
        return 2.0f;
    }

    if (pdata->alpha > 0) {
        pdata->alpha -= 8;
        object = pdata->object;
        if (object != 0 && object->instance != pdata->object_instance) {
            object = 0;
        }
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, (unsigned char)pdata->alpha);
            if (pdata->direction == 0) {
                object->x--;
                object->y++;
            } else {
                object->x++;
                object->y++;
            }
        }
        if (pdata->alpha - 8 < 0) {
            pdata->alpha = 0;
        }
        return 2.0f;
    }

    object = pdata->object;
    if (object != 0 && object->instance != pdata->object_instance) {
        object = 0;
    }
    if (object != 0 && object->instance != 0) {
        object->vtbl->destroy();
    }
    return -1.0f;
}

void fight_fx_blades_clash(PlyrPdata* player) {
    ReactionBladeFighterDefinition* fighter;
    ReactionBladeTransform* transform;
    MkObj* blade;
    MkPfx* particle;
    unsigned int effect;
    int bone;

    fighter =
        (ReactionBladeFighterDefinition*)player->fighter_definition;
    blade = fighter->blade_object;
    if (blade != 0 &&
        blade->hdr.instance != fighter->blade_object_instance) {
        blade = 0;
    }
    bone = 0;
    if (blade == 0 || blade->hide_flag_bits.hidden == 1) {
        bone = 0x1C;
        blade = player->plyr_info->slot.mirror_a;
    }
    if (player->plyr_num == 0) {
        effect = fx_by_owner("blade_flash", 1);
    } else {
        effect = fx_by_owner("blade_flash", 2);
    }
    effect = fx_next_emitter(effect);
    transform = fighter->blade_root->transform;
    if (effect != 0) {
        particle = pfx_from_emitter(effect);
        pfx_bind_emitter_num_to_obj_bone(
            particle, blade, bone, emitter_id_from_handle(effect));
        fx_set_param_v3(
            effect, 0x202, transform->position.x,
            transform->position.y, transform->position.z);
        fx_resume_emit(effect);
    }

    if (player->plyr_num == 0) {
        effect = fx_by_owner("blade_sparks", 1);
    } else {
        effect = fx_by_owner("blade_sparks", 2);
    }
    effect = fx_next_emitter(effect);
    transform = fighter->blade_root->transform;
    if (effect != 0) {
        particle = pfx_from_emitter(effect);
        pfx_bind_emitter_num_to_obj_bone(
            particle, blade, bone, emitter_id_from_handle(effect));
        fx_set_param_v3(
            effect, 0x202, transform->position.x,
            transform->position.y, transform->position.z);
        fx_resume_emit(effect);
    }

    if (player->plyr_num == 0) {
        effect = fx_by_owner("blade_bouncy_sparks", 1);
    } else {
        effect = fx_by_owner("blade_bouncy_sparks", 2);
    }
    effect = fx_next_emitter(effect);
    transform = fighter->blade_root->transform;
    if (effect != 0) {
        particle = pfx_from_emitter(effect);
        pfx_bind_emitter_num_to_obj_bone(
            particle, blade, bone, emitter_id_from_handle(effect));
        fx_set_param_v3(
            effect, 0x202, transform->position.x,
            transform->position.y, transform->position.z);
        fx_resume_emit(effect);
    }
}

#pragma dont_inline on

float r_obstacle_falldown(void) {
    ReactionProcVtable* vtable;

    init_air_move_no_aniproc();
    stop_me();
    face_opponent_now();
    danger_zone_eligible_on();
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    myvel_his_angle_y(0.0f, 0.045f, 0.045f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.004f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_counter_catch_med(void) {
    ReactionProcVtable* vtable;

    blend_to_ani(shared_ani.counter_caught, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    set_my_state(0x4202);
    _mkproc_sleep_ticks = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    set_my_state(0x4000);
    _mkproc_sleep_ticks = 70.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    reaction_xfer_him(0xBF, 0.0f, 2);
    plyr_pdata->summon_position_x = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

float r_post_surf_throw(void) {
    ReactionProcVtable* vtable;
    ReactionPostSurfPdata* player;

    if (stay_down_check() != 0) {
        player = (ReactionPostSurfPdata*)plyr_pdata;
        player->stay_down = 1;
        vtable = (ReactionProcVtable*)aproc->vtbl;
        vtable->jump_sleep(j_stay_down_dead, 0.0f);
        return 0.0f;
    }
    force_away(0xA, 0x10, 0.1f, 0.9f);
    _mkproc_sleep_ticks = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    tightrope_restrictions_on();
    glitch_to_ani(shared_ani.post_surf_getup, 3);
    plyr_anim_pdata->step = 1.2f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float r_nightwolf_lightning(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    wall_eligible_on();
    plyr_obj->flags_09_bits.launched = 0;
    got_hit_fx(0, 2, 1, 3, 0, 1, 0.05f);
    random_voice(0x13);
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_nightwolf_charge(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    wall_eligible_on();
    plyr_obj->flags_09_bits.launched = 0;
    if (his_pdata->character_id == 1 && his_pdata->attack_region == 0) {
        random_voice(3);
        random_hit(0xD);
        shake_camera(2, 0.02f);
        start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
        face_bleed_me(3);
    } else {
        got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    }
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_face3_onback(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    face_opponent_now();
    wall_eligible_on();
    plyr_obj->flags_09_bits.launched = 0;
    if (his_pdata->character_id == 1 && his_pdata->attack_region == 0) {
        random_voice(3);
        random_hit(0xD);
        shake_camera(2, 0.02f);
        start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
        face_bleed_me(3);
    } else {
        got_hit_fx(0, 2, 1, 3, 0, 0, 0.05f);
    }
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.falling_back, 0, 0.0f, 0.0f, 24.0f,
        0.1f, -0.007f, 0.2f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    plyr_anim_pdata->step = 2.0f;
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_head3_onback(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 0, 0, 3, 0, 3, 0.05f);
    r_top_of_head_slam();
    blend_to_ani(shared_ani.head_slam_fall, 3, 0.2f);
    plyr_anim_pdata->step = 0.8f;
    ani_to_frame_x(27.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    got_hit_fx(4, 8, 1, 0, 0, 0, 0.0f);
    back_rollup_check();
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_enough_air_already(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    plyr_pdata->blocking_disabled = 0;
    plyr_pdata->blocking_disabled_2 = 0;
    plyr_obj->flags_09_bits.launched = 0;
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    face_bleed_me(3);
    shake_hit_voice(2, 1, 0xD, 0.02f);
    random_hit(4);
    myvel_his_angle_y(0.0f, 0.07f, 0.07f);
    launch_n_land_ani(
        shared_ani.enough_air, 0xCA1, 0.0f, 0.0f, 28.0f,
        0.05f, -0.008f, 0.33f);
    shake_hit_voice(2, 9, 8, 0.02f);
    back_rollup_check();
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

float r_sidehead3_dive_opposite(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 0, 0, 0, 0.05f);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    face_bleed_me(2);
    newani_to_frame_x(
        shared_ani.side_head_dive, 2, 22.0f, 1.0f, 1.0f, 0.2f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0, 0, 0.0f, 0.0f);
    shake_hit_voice(2, 9, 7, 0.02f);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_3, 0.0f);
    return 0.0f;
}

float r_sidehead3_dive(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 2, 1, 0, 0, 0, 0.05f);
    tightrope_restrictions_off();
    plyr_obj->flags_09_bits.launched = 0;
    face_bleed_me(2);
    start_blood_particles(0x39, 0x10, plyr_pdata, plyr_obj);
    newani_to_frame_x(
        shared_ani.side_head_dive, 1, 22.0f, 1.0f, 1.0f, 0.2f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    land_chores(0xCA1, 0xCA1, 2.0f, 0.02f);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    ani_to_end();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_3, 0.0f);
    return 0.0f;
}

float r_gut3_onfeet_hard(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    plyr_obj->flags_09_bits.launched = 0;
    blend_to_ani(shared_ani.gut_on_feet, 3, 0.33f);
    plyr_anim_pdata->step = 0.85f;
    plyr_anim_pdata->weight = 1.6f;
    ani_to_frame_x(24.0f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    player_feet_land_chores();
    ani_to_blend_frame(10.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float r_gut3_onfeet(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    plyr_obj->flags_09_bits.launched = 0;
    blend_to_ani(shared_ani.gut_on_feet, 3, 0.33f);
    plyr_anim_pdata->step = 0.85f;
    ani_to_frame_x(24.0f);
    init_ground_move();
    blend_to_stance(0.04f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_gut3_onbutt(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 5, 1, 0, 0, 0, 0.0f);
    plyr_obj->flags_09_bits.launched = 0;
    force_away(3, 8, 0.1f, 0.9f);
    blend_to_ani(shared_ani.gut_on_butt, 3, 0.33f);
    ani_to_frame_x(18.0f);
    if (large_ground_fx != 0) {
        large_ground_fx();
    }
    ani_to_fall_to_frame(0xD7F, 24.0f, plyr_anim_pdata->high_frame);
    _mkproc_sleep_ticks = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_sit_12, 0.0f);
    return 0.0f;
}

float r_throw(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    blend_to_ani(shared_ani.throw_fall, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_frame_x(39.0f);
    land_chores(0xD7F, 0xCB8, 3.0f, 0.03f);
    ani_to_end();
    plyr_obj->ang.y += 3.14f;
    set_anim_script(plyr_anim_pdata, shared_ani.throw_getup, 3);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_12, 0.0f);
    return 0.0f;
}

float r_raiden_shocker_fall(void) {
    ReactionProcVtable* vtable;
    ReactionExtendedPdata* opponent;

    opponent = (ReactionExtendedPdata*)his_pdata;
    blend_to_ani(opponent->reaction_animation, 0, 0.1f);
    plyr_anim_pdata->step = 1.0f;
    xfer_proc(plyr_anim_proc, p_animate);
    _mkproc_sleep_ticks = 60.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    init_ground_move();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit_blend_stance, 0.0f);
    return 0.0f;
}

#pragma dont_inline reset

/* Soft ceiling: r_ZZZZZZZ ~97.50% -- pool-label noise only. */
float r_ZZZZZZZ(void) {
    return 1.0f;
}

/* Retail TU-local; referenced by remaining split reaction code. */
void same_xz(void) {
    plyr_obj->pos_x = his_obj->pos_x;
    plyr_obj->pos_z = his_obj->pos_z;
}

/* Soft ceiling: r_block_hit_projectile ~99.31% -- pool-label noise only. */
float r_block_hit_projectile(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(5, 0, 0, 0, 0);
    force_away(2, 5, 0.25f, 0.4f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p5 ~99.31% -- pool-label noise only. */
float r_block_hit_p5(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xB, 5, 0, 0, 0);
    force_away(3, 8, 0.2f, 0.85f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p3 ~99.31% -- pool-label noise only. */
float r_block_hit_p3(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(2, 5, 0.25f, 0.4f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p1 ~99.35% -- pool-label noise only. */
float r_block_hit_p1(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(2, 4, 0.15f, 0.5f);
    disable_my_attacks(6);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_block_hit_p0 ~99.35% -- pool-label noise only. */
float r_block_hit_p0(void) {
    ReactionProcVtable* vtable;

    stop_me();
    init_ground_move();
    blocked_fx(0xA, 0, 0, 0, 0);
    force_away(2, 5, 0.25f, 0.4f);
    disable_my_attacks(6);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_block_common_reaction, 0.0f);
    return 0.0f;
}

#pragma dont_inline on
/* Soft ceiling: r_chest2_stumble_shake ~99.31% -- pool-label noise only. */
float r_chest2_stumble_shake(void) {
    ReactionProcVtable* vtable;

    adjust_my_damage_multiplier(0.75f);
    face_opponent_now();
    got_hit_fx(2, 2, 1, 0xB, 0, 0, 0.0f);
    random_hit(5);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(chest_stumble_both, 0.0f);
    return 0.0f;
}

/* Soft ceiling: r_chest2_stumble ~99.42% -- pool-label noise only. */
float r_chest2_stumble(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0, 0.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(chest_stumble_both, 0.0f);
    return 0.0f;
}
#pragma dont_inline reset

float r_summon_flames(void) {
    low_flash_check();
    face_opponent_now();
    random_voice(0x14);
    blend_to_ani(his_pdata->reaction_animation_a, 3, 0.2f);
    plyr_anim_pdata->step = 1.0f;
    ani_to_end();
    blend_to_ani(his_pdata->reaction_animation_b, 3, 0.5f);
    ani_to_end();
    blend_to_ani(his_pdata->reaction_animation_c, 3, 0.1f);
    ani_to_end();
    plyr_pdata->summon_position_x = 15.0f;
    ((ReactionProcVtable*)aproc->vtbl)
        ->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

#pragma dont_inline on

float r_chest2_separate(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    face_opponent_now();
    wall_eligible_on();
    got_hit_fx(2, 4, 0, 0xA, 0, 0, 0.025f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(9, 8, 0.1f, 0.9f);
    plyr_anim_pdata->weight = 1.0f;
    plyr_anim_pdata->step = 0.7f;
    glitch_to_ani(shared_ani.chest_stumble, 3);
    set_anim_hiframe(47.0f);
    ani_to_blend_frame(15.0f);
    blend_to_stance(0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_cyrus_stomp(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    disable_both_repel_flags();
    got_hit_fx(2, 5, 1, 0xA, 0, 0, 0.05f);
    blend_to_ani(shared_ani.cyrus_stomp, 3, 0.2f);
    ani_to_frame_x_call(same_xz, 8.0f);
    got_hit_fx(4, 8, 1, 0, 0, 0, 0.0f);
    init_ground_move();
    ani_to_end();
    _mkproc_sleep_ticks = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_front_12, 0.0f);
    return 0.0f;
}

float r_shujinko_slam(void) {
    ReactionProcVtable* vtable;

    got_hit_fx(2, 0xD, 4, 0, 0, 2, 0.0f);
    init_air_move();
    face_opponent_now();
    stop_me();
    plyr_obj->gravity = 0.0018f;
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_ani(his_pdata->reaction_animation_a, 0, 0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_complete_ermac_slam, 0.0f);
    return 0.0f;
}

float r_ermac_slam(void) {
    ReactionProcVtable* vtable;
    ReactionExtendedPdata* opponent;

    got_hit_fx(2, 0xD, 4, 0, 0, 2, 0.0f);
    init_air_move();
    face_opponent_now();
    stop_me();
    plyr_obj->gravity = 0.0018f;
    xfer_proc(plyr_anim_proc, p_animate);
    opponent = (ReactionExtendedPdata*)his_pdata;
    blend_to_ani(opponent->reaction_animation, 0, 0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(r_complete_ermac_slam, 0.0f);
    return 0.0f;
}

float r_combo_broken_part1(void) {
    ReactionProcVtable* vtable;

    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    _mkproc_sleep_ticks = 40.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_jump_slambounce_final_hit(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    stop_me();
    init_air_move();
    special_move_cam_setup(
        0xA, 0x3C, 0, 1.47f, 4.1f, 1.0f, -1.75f, -0.15f);
    reaction_xfer_him(0xDB, 0.0f, 2);
    got_hit_fx(0, 2, 1, 1, 0, 0, 0.05f);
    blend_to_ani(shared_ani.jump_slambounce, 3, 0.2f);
    set_ani_speed(0.7f);
    ani_to_frame_x(6.0f);
    init_ground_move();
    ani_to_frame_x(50.0f);
    got_hit_fx(4, 0, 1, 0, 0, 1, 0.0f);
    ani_to_end();
    check_for_combo_message();
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float r_slamdown_final_hitter(void) {
    ReactionProcVtable* vtable;

    head_tracking_on();
    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    set_ani_speed(0.25f);
    ani_to_blend_frame(3.0f);
    xfer_proc(plyr_anim_proc, p_animate);
    blend_to_stance(0.05f);
    _mkproc_sleep_ticks = 6.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_corner_repell_ani(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(0x12, 8, 0.15f, 0.9f);
    ani_to_blend_frame(2.0f);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_corner_repell(void) {
    ReactionProcVtable* vtable;

    face_opponent_now();
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    force_away(9, 8, 0.1f, 0.9f);
    _mkproc_sleep_ticks = 20.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->sleep(vtable);
    blend_to_stance(0.05f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_feet1a(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0x10, 0.0f);
    force_away(3, 8, 0.1f, 0.9f);
    blend_to_ani(shared_ani.feet_hit, 3, 0.33f);
    ani_to_blend_frame(10.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float r_feet1_stay_close(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 0x10, 0.0f);
    force_away(3, 8, 0.04f, 0.9f);
    blend_to_ani(shared_ani.feet_hit, 3, 0.33f);
    ani_to_blend_frame(10.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float r_jax_piston_hi(void) {
    ReactionProcVtable* vtable;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(0, 1, 5, 4, 0, 0, 0.025f);
    blend_to_ani(shared_ani.jax_piston_high, 3, 0.33f);
    plyr_anim_pdata->weight = 0.0f;
    ani_to_blend_frame(10.0f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep((MkProcEntryFn)p_blend_to_stance_in_10, 0.0f);
    return 0.0f;
}

float r_jax_piston_lo(void) {
    ReactionProcVtable* vtable;

    low_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 5, 0, 0, 0, 0.0f);
    adjust_my_damage_multiplier(0.75f);
    blend_to_ani(shared_ani.jax_piston_low, 3, 0.2f);
    plyr_anim_pdata->weight = 0.0f;
    ani_to_frame_x(20.0f);
    plyr_pdata->summon_position_x = 15.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

float chest_stumble_both(void) {
    ReactionProcVtable* vtable;

    medium_flash_check();
    add_facial_damage(0.025f);
    face_opponent_now();
    wall_eligible_on();
    adjust_my_damage_multiplier(0.75f);
    plyr_obj->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    ground_me(plyr_obj != 0 ? as_mkhdr(&plyr_obj->hdr) : 0);
    plyr_anim_pdata->weight = 1.3f;
    plyr_anim_pdata->step = 0.7f;
    blend_to_ani_frame(shared_ani.chest_stumble, 3, 0.33f, 7.0f);
    set_anim_hiframe(47.0f);
    ani_to_blend_frame(15.0f);
    blend_to_stance(0.1f);
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_exit, 0.0f);
    return 0.0f;
}

float r_esp1_A(void) {
    ReactionProcVtable* vtable;
    ReactionExtendedPdata* opponent;

    high_flash_check();
    face_opponent_now();
    got_hit_fx(2, 4, 0, 0, 0, 2, 0.0f);
    opponent = (ReactionExtendedPdata*)his_pdata;
    blend_to_ani(opponent->esp1_reaction_animation, 3, 0.1f);
    plyr_obj->gravity = -0.0075f;
    plyr_anim_pdata->step = 0.8f;
    ani_to_blend_frame(10.0f);
    plyr_pdata->summon_position_x = 10.0f;
    vtable = (ReactionProcVtable*)aproc->vtbl;
    vtable->jump_sleep(j_blend_to_stance_in_x, 0.0f);
    return 0.0f;
}

#pragma dont_inline reset
