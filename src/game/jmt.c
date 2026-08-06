#include "game/game_info.h"
#include "game/pfxscript.h"
#include "math/gxMath.h"
#include "runtime/anim_pdata.h"
#include "runtime/cam.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/plyr_pdata.h"

static const char ermac_eye_effects[] = "eyelt\0eyert";

typedef union JmtFloatBits {
    float f;
    unsigned int u;
} JmtFloatBits;

typedef struct JmtProcVtable {
    void* reserved[6];
    void (*sleep)(struct JmtProcVtable* vtbl);
    void* reserved_after_sleep[2];
    int (*jump_sleep)(MkProcEntryFn entry, float ticks);
} JmtProcVtable;

typedef struct JmtDecoyPdata {
    MkHdr hdr;
    char pad08[0x10];
    MkObj* object;
    unsigned int object_instance;
    char pad20[0x518];
    float shrink_ticks;
} JmtDecoyPdata;

typedef struct JmtSharedAnimations {
    char pad000[0x324];
    void* kabal_falldown;
} JmtSharedAnimations;

typedef struct JmtKabalAnimations {
    char pad000[0x378];
    void* collide_start;
    void* collide_recover;
} JmtKabalAnimations;

typedef struct JmtBloodTypeList {
    char pad00[0x2C];
    int minimum_level;
} JmtBloodTypeList;

typedef struct JmtPfxEffectView {
    char pad00[0x40];
    char emitter_vm[1];
} JmtPfxEffectView;

typedef struct JmtPfxEmitterView {
    char pad00[0x1C];
    unsigned char flags;
} JmtPfxEmitterView;

typedef struct JmtDestroyable JmtDestroyable;
typedef struct JmtDestroyVtable {
    void* reserved[4];
    int (*destroy)(JmtDestroyable* object);
} JmtDestroyVtable;

struct JmtDestroyable {
    JmtDestroyVtable* vtbl;
    unsigned int instance;
};

extern AnimPdata* plyr_anim_pdata;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern PlyrPdata* his_pdata;
extern JmtSharedAnimations shared_ani;
extern MkObj* g_bgnd_preloaded_models[];
extern JmtBloodTypeList blood_type_list;
extern MkPtr* gusher_list;
extern int heart_beat;
extern int mode_of_play;
extern float game_speed;
extern void check_release_other_player(void);
extern void swap_active_plyr_proc(void);
extern void adjust_player_life(float amount);
void set_constrain_last_pos(int player, int position);
void plyr_bleed_large_ext(PlyrPdata* player);

int local_collision_allowed(PlyrPdata* player);
int player_area_collision_check(
    int region, int flags, float radius, float height, float depth);
int collision_2(int region, float radius, float height);
void check_to_register_miss(void);
void ani_1_frame(void);
void start_plyr_attack(float radius);
void set_collision_made_flag(void);
void reaction_xfer_him(int reaction, float rate, int strength);
void blend_to_ani(void* animation, int transition, float blend);
void ani_to_frame_x(float frame);
void slow_ani_x(float speed, float frame);
void stop_me(void);
void got_hit_fx(
    int type, int region, int arg2, int arg3, int arg4, int arg5,
    float scale);
void set_ani_speed(float speed);
void random_hit(int type);
void ani_to_blend_frame(float frame);
void face_opponent_now(void);
void init_air_move(void);
void force_away(
    int duration, int animation, float force, float damping);
void blend_to_stance(float blend);
float j_exit(void);
void idle_victim(MkObj* object, int mode);
void xfer_player_proc(MkProc* proc, MkProcEntryFn entry);
int build_bones_tbl(MkObj* object, int flags);
int get_blood_level(void);
MkHdr* start_gusher(
    int* heartbeat, void* player, MkObj* object, int bone,
    const Vec* direction, const Vec* velocity);
float p_decoy_shrink(void);
float kabal_collide_victim(void);
float j_getup_back_6(void);
void* pfx_get_emitter(void* vm, int index);
void* find_pfx_by_name(const char* name);
void restart_effect_ppfx(void* effect);
static float kabal_collide_victim_falldown(void);

static inline float jmt_fast_inverse_sqrt(float squared) {
    JmtFloatBits bits;
    float estimate;
    float product;
    float correction;

    if (squared <= 0.0f) {
        return 0.0f;
    }
    bits.f = squared;
    bits.u = 0x5F375A00 - (bits.u >> 1);
    estimate = bits.f;
    product = estimate * (squared * estimate);
    correction = 3.0f - product;
    return 0.0625f * estimate * correction *
           -(correction * (product * correction) - 12.0f);
}

static inline float jmt_fast_sqrt(float squared) {
    JmtFloatBits bits;

    if (squared <= 0.0f) {
        return 0.0f;
    }
    bits.f = squared;
    bits.u =
        ((unsigned int)GXMathSqrtTable[(bits.u >> 10) & 0x3FFE] << 8) |
        ((((bits.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000);
    return 0.5f * (bits.f * (3.0f - (bits.f * bits.f) / squared));
}

void player_area_collision_ticks(
    int region, int flags, void* script_args,
    float radius, float height, float depth, float ticks) {
    JmtProcVtable* proc_vtbl;
    float elapsed;

    (void)script_args;
    elapsed = 0.0f;
    plyr_pdata->collision_result = -1;
    while (elapsed < ticks) {
        if (local_collision_allowed(plyr_pdata) != 0) {
            if (player_area_collision_check(
                    region, flags, radius, height, depth) != 0) {
                break;
            }
        } else if (plyr_pdata->collision_result != -1 &&
                   plyr_pdata->collision_result != 3) {
            break;
        }

        _mkproc_sleep_ticks = 1.0f;
        proc_vtbl = (JmtProcVtable*)aproc->vtbl;
        proc_vtbl->sleep(proc_vtbl);
        ani_1_frame();
        elapsed += game_speed;
    }
    check_to_register_miss();
}

void flying_collision(
    int region, int reaction, int strength, void* script_args,
    float radius, float height, float reaction_rate,
    float exit_height, float collision_height, float max_frame,
    float max_ticks) {
    JmtProcVtable* proc_vtbl;
    float object_y;
    float ground_y;
    float elapsed;

    (void)script_args;
    plyr_pdata->collision_result = -1;
    start_plyr_attack(0.0f);
    object_y = plyr_obj->pos.y;
    ground_y = g_game_info.field_34;
    elapsed = 0.0f;
    while (object_y > ground_y + exit_height &&
           elapsed < max_ticks) {
        _mkproc_sleep_ticks = 1.0f;
        proc_vtbl = (JmtProcVtable*)aproc->vtbl;
        proc_vtbl->sleep(proc_vtbl);
        ani_1_frame();
        ground_y = g_game_info.field_34;
        object_y = plyr_obj->pos.y;

        if (object_y > ground_y + collision_height &&
            plyr_anim_pdata->frame < max_frame) {
            if (local_collision_allowed(plyr_pdata) != 0) {
                if (collision_2(region, radius, height) != 0) {
                    set_collision_made_flag();
                    reaction_xfer_him(
                        reaction, reaction_rate, strength);
                    break;
                }
            } else if (plyr_pdata->collision_result != -1 &&
                       plyr_pdata->collision_result != 3) {
                break;
            }
        }
        elapsed += game_speed;
    }
    check_to_register_miss();
}

void kabal_collision_control_victim(int falldown) {
    MkProc* victim_proc;

    if (plyr_pdata == 0) {
        return;
    }
    plyr_obj->flags_09 |= 0x20;
    his_obj->flags_09 |= 0x20;
    idle_victim(his_obj, 1);
    if (plyr_pdata->plyr_num == 0) {
        victim_proc = g_game_info.plyr1.idle_proc;
    } else {
        victim_proc = g_game_info.plyr0.idle_proc;
    }
    if (falldown == 0) {
        xfer_player_proc(victim_proc, kabal_collide_victim);
    } else {
        xfer_player_proc(victim_proc, kabal_collide_victim_falldown);
    }
}

static float kabal_collide_victim_falldown(void) {
    JmtProcVtable* proc_vtbl;

    blend_to_ani(his_pdata->screen_taunt_animation, 3, 0.2f);
    ani_to_frame_x(6.0f);
    slow_ani_x(0.8f, 12.0f);
    stop_me();
    got_hit_fx(0, 2, 4, 3, 2, 0, 0.0f);
    ani_to_frame_x(20.0f);
    set_ani_speed(1.0f);
    blend_to_ani(shared_ani.kabal_falldown, 3, 0.1f);
    ani_to_frame_x(50.0f);
    random_hit(5);
    got_hit_fx(4, 9, 1, 0, 0, 0, 0.0f);
    ani_to_blend_frame(10.0f);
    proc_vtbl = (JmtProcVtable*)aproc->vtbl;
    proc_vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

float kabal_collide_victim(void) {
    JmtProcVtable* proc_vtbl;
    JmtKabalAnimations* animations;
    int ticks;

    face_opponent_now();
    init_air_move();
    animations = (JmtKabalAnimations*)his_pdata;
    blend_to_ani(animations->collide_start, 3, 0.1f);
    plyr_anim_pdata->step = 2.0f;
    force_away(15, 6, 0.11f, 0.975f);
    ani_to_frame_x(15.0f);
    force_away(15, 20, -0.18f, 0.975f);
    ani_to_blend_frame(15.0f);

    proc_vtbl = (JmtProcVtable*)aproc->vtbl;
    for (ticks = 0; ticks < 50; ticks++) {
        _mkproc_sleep_ticks = 2.0f;
        proc_vtbl->sleep(proc_vtbl);
        if (his_pdata != 0 && his_pdata->state != 0x1200) {
            blend_to_stance(0.1f);
            proc_vtbl->jump_sleep(j_exit, 0.0f);
            return 0.0f;
        }
    }

    animations = (JmtKabalAnimations*)his_pdata;
    blend_to_ani(animations->collide_recover, 3, 0.2f);
    ani_to_frame_x(20.0f);
    blend_to_ani(shared_ani.kabal_falldown, 3, 0.1f);
    ani_to_blend_frame(10.0f);
    proc_vtbl->jump_sleep(j_getup_back_6, 0.0f);
    return 0.0f;
}

void destroy_subzero_decoy(void) {
    MkProc* proc;
    JmtDecoyPdata* pdata;
    MkObj* object;

    if (plyr_pdata == 0) {
        return;
    }
    if (plyr_pdata->plyr_num == 0) {
        proc = find_mkproc_pid(0xB00F);
    } else {
        proc = find_mkproc_pid(0xB00E);
    }
    if (proc == 0) {
        return;
    }

    pdata = (JmtDecoyPdata*)pdata_of_proc(proc);
    if (pdata == 0) {
        return;
    }
    object = pdata->object;
    if (object != 0 && object->hdr.instance != pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        if (proc->instance != 0) {
            ((JmtDestroyable*)proc)->vtbl->destroy(
                (JmtDestroyable*)proc);
        }
        return;
    }

    object->flags_08 |= 2;
    object->scale.x = 1.0f;
    object->scale.y = 1.0f;
    object->scale.z = 1.0f;
    pdata->shrink_ticks = 15.0f;
    xfer_proc(proc, p_decoy_shrink);
}

int is_drone(void) {
    if (plyr_pdata == 0) {
        return 0;
    }
    return plyr_pdata->drone_request == 1;
}

void remove_impaled_projectiles(void) {
    destroy_mkprocs_pid(0x2026);
    g_game_info.plyr0.slot.pdata->impaled_projectile_state = 0;
    g_game_info.plyr1.slot.pdata->impaled_projectile_state = 0;
}

void set_constrain_last_pos_pdata(int position) {
    if (plyr_pdata != 0) {
        set_constrain_last_pos(plyr_pdata->plyr_num, position);
    }
}

void plyr_set_gravity(float gravity) {
    plyr_obj->gravity = gravity;
    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->flags_09_bits.launched = 1;
}

void mks_victim_bleed(int player) {
    PlyrPdata* victim;

    if (player == 1) {
        victim = g_game_info.plyr1.slot.pdata;
    } else {
        victim = g_game_info.plyr0.slot.pdata;
    }
    if (victim != 0) {
        plyr_bleed_large_ext(victim);
    }
}

void mks_plyr_stop(int player) {
    MkObj* object;

    if (player == 1) {
        object = g_game_info.plyr1.slot.mirror_a;
    } else {
        object = g_game_info.plyr0.slot.mirror_a;
    }
    if (object != 0) {
        object->flags_08_bits.moving = 0;
        object->gravity = 0.0f;
        object->pos_vel.x = 0.0f;
        object->pos_vel.y = 0.0f;
        object->pos_vel.z = 0.0f;
    }
}

void increment_taunts_performed(void) {
    plyr_pdata->taunts_performed++;
}

int get_taunts_performed(void) {
    return plyr_pdata->taunts_performed;
}

int check_for_online_condition(PlyrPdata* pdata) {
    if (g_game_info.feature_flags.bits.high_bit == 0) {
        return 0;
    }
    if (pdata == 0) {
        return 0;
    }

    pdata = pdata->his_plyr_pdata;
    if (pdata != 0 && pdata->online_sync_index != -1) {
        return 1;
    }
    return 0;
}

void online_sync_reset(void) {
}

void release_both_players(void) {
    check_release_other_player();
    swap_active_plyr_proc();
    check_release_other_player();
    swap_active_plyr_proc();
}

void adjust_kabal_position(void) {
}

float get_adjusted_speed(MkObj* object, float speed, float adjustment) {
    return speed;
}

int get_current_bgnd(void) {
    return g_game_info.bgnd_id;
}

void kill_ermac_eyes(void) {
    if (plyr_pdata != 0 && plyr_pdata->character_id == 6) {
        fx_reset(fx(ermac_eye_effects));
        fx_reset(fx(ermac_eye_effects + 6));
    }
}

void jmt_debug_script(int command, int value, const void* args, float scalar) {
}

float animpdata_get_anim_hiframe(const AnimPdata* pdata) {
    return pdata->high_frame;
}

float plyr_get_anim_hiframe(void) {
    return plyr_anim_pdata->high_frame;
}

float plyr_get_anim_frame(void) {
    return plyr_anim_pdata->frame;
}

void plyr_set_vel_xz_y(float xz_velocity, float y_velocity) {
    float sine;
    float z_velocity;

    sine = gxMathSin(plyr_obj->ang.y);
    z_velocity = gxMathCos(plyr_obj->ang.y) * xz_velocity;
    plyr_obj->pos_vel.x = sine * xz_velocity;
    plyr_obj->pos_vel.y = y_velocity;
    plyr_obj->pos_vel.z = z_velocity;
}

void plyr_scale_pos_vel(float x_scale, float y_scale, float z_scale) {
    plyr_obj->pos_vel.x *= x_scale;
    plyr_obj->pos_vel.y *= y_scale;
    plyr_obj->pos_vel.z *= z_scale;
}

int get_collision_result(void) {
    return plyr_pdata->collision_result;
}

void collision_result_dont_care(void) {
    plyr_pdata->collision_result = 3;
}

void clear_collision_result(void) {
    plyr_pdata->collision_result = -1;
}

void online_combo_record(void) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void online_combo_adjust(void) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void enable_no_sync_anim_f(void) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void enable_no_adjustment_f(void) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void kill_plyr_life(void) {
    adjust_player_life(-1.0f);
}

int is_local_plyr(void) {
    if (plyr_pdata == 0) {
        return 1;
    }
    return 1;
}

void dizzy_kill_pfx(void) {
    if (plyr_pdata != 0) {
        switch (plyr_pdata->character_id) {
        case 6:
            fx_reset(fx(ermac_eye_effects));
            fx_reset(fx(ermac_eye_effects + 6));
            break;
        }
    }
}

int single_frame_collision_check(
    int region, int reaction, int strength, void* script_args,
    float radius, float height, float reaction_rate) {
    (void)script_args;
    if (collision_2(region, radius, height) == 0) {
        return 0;
    }
    set_collision_made_flag();
    reaction_xfer_him(reaction, reaction_rate, strength);
    return 1;
}

float mks_get_victim_to_tr_dot(int player) {
    MkObj* victim;
    MkObj* target;
    float victim_x;
    float victim_z;
    float target_x;
    float target_z;
    float victim_inverse_length;
    float target_inverse_length;

    if (player == 0) {
        victim = g_game_info.plyr1.slot.mirror_a;
        target = g_game_info.plyr0.slot.mirror_a;
    } else {
        victim = g_game_info.plyr0.slot.mirror_a;
        target = g_game_info.plyr1.slot.mirror_a;
    }
    if (victim == 0 || target == 0) {
        return 0.0f;
    }

    victim_x = victim->pos.x;
    victim_z = victim->pos.z;
    victim_inverse_length =
        jmt_fast_inverse_sqrt(victim_x * victim_x + victim_z * victim_z);
    target_x = target->pos.x - victim_x;
    target_z = target->pos.z - victim_z;
    target_inverse_length =
        jmt_fast_inverse_sqrt(target_x * target_x + target_z * target_z);
    return (victim_x * victim_inverse_length) *
               (target_x * target_inverse_length) +
           (victim_z * victim_inverse_length) *
               (target_z * target_inverse_length);
}

MkHdr* mks_start_gusher(
    int player, int bone, void* script_args,
    float velocity_x, float velocity_y, float velocity_z,
    float direction_x, float direction_y, float direction_z) {
    PlyrInfo* info;
    MkHdr* gusher;
    Vec direction;
    Vec velocity;

    (void)script_args;
    if (get_blood_level() < blood_type_list.minimum_level) {
        return 0;
    }

    direction.x = direction_x;
    direction.y = direction_y;
    direction.z = direction_z;
    velocity.x = velocity_x;
    velocity.y = velocity_y;
    velocity.z = velocity_z;
    if (player == 1) {
        info = &g_game_info.plyr1;
    } else {
        info = &g_game_info.plyr0;
    }
    gusher = start_gusher(
        &heart_beat, info->slot.fighter, info->slot.mirror_a, bone,
        &direction, &velocity);
    if (gusher != 0) {
        mk_insert(gusher, &gusher_list);
    }
    return gusher;
}

void mks_set_plyr_to_center_ang_offset(
    int player, void* script_args, float angle_offset) {
    MkObj* object;
    float length;
    float inverse_length;
    float normalized_x;
    float normalized_z;
    int angle_bits;

    (void)script_args;
    if (player == 1) {
        object = g_game_info.plyr1.slot.mirror_a;
    } else {
        object = g_game_info.plyr0.slot.mirror_a;
    }
    if (object == 0) {
        return;
    }

    length = jmt_fast_sqrt(
        object->pos.x * object->pos.x +
        object->pos.z * object->pos.z);
    if (length <= 0.0f) {
        inverse_length = length;
    } else {
        inverse_length = 1.0f / length;
    }
    if (length == 0.0f) {
        return;
    }

    normalized_x = -object->pos.x * inverse_length;
    normalized_z = -object->pos.z * inverse_length;
    angle_bits = (int)(166886.1f *
        (angle_offset + gxMathArcTanYX(normalized_x, normalized_z)));
    angle_bits &= 0xFFFFF;
    object->ang.y = 0.000005992112f * (float)angle_bits;
}

void mks_bgnd_cam_offset_away(
    void* script_args, float distance, float height) {
    MkObj* victim;
    float length;
    float inverse_length;

    (void)script_args;
    victim = (MkObj*)camera_get_victim();
    if (victim == 0) {
        return;
    }

    length = jmt_fast_sqrt(
        victim->pos.x * victim->pos.x +
        victim->pos.z * victim->pos.z);
    if (length <= 0.0f) {
        inverse_length = length;
    } else {
        inverse_length = 1.0f / length;
    }
    if (length != 0.0f) {
        camera_set_movement_offset_explicit(
            victim->pos.x * inverse_length * distance,
            height,
            victim->pos.z * inverse_length * distance);
    }
}

void mks_bgnd_pfx_bind_to_sobj(
    const char* effect_name, unsigned int sobj_id) {
    MkSobj* sobj;
    void* effect;
    JmtPfxEffectView* effect_view;
    JmtPfxEmitterView* emitter;

    sobj = (MkSobj*)obj_find_sobj_by_id(g_game_info.bgnd_obj, sobj_id);
    if (sobj == 0) {
        return;
    }
    effect = find_pfx_by_name(effect_name);
    if (effect == 0) {
        return;
    }

    restart_effect_ppfx(effect);
    pfx_bind_emitter_to_sobj((MkPfx*)effect, sobj, 0);
    effect_view = (JmtPfxEffectView*)effect;
    emitter = (JmtPfxEmitterView*)pfx_get_emitter(
        effect_view->emitter_vm, 0);
    emitter->flags &= (unsigned char)~0x80;
}

void mks_npc_build_bones_tbl(int model_index, int flags) {
    MkObj* model;

    model = g_bgnd_preloaded_models[model_index];
    if (model != 0) {
        build_bones_tbl(model, flags);
    }
}

void check_bgnd_effect(void) {
    CmdScript* script;
    CmdScript* saved_script;

    if (g_game_info.bgnd_id != 6 || mode_of_play == 6) {
        return;
    }

    script = alloc_cmdscript();
    saved_script = active_cmdscript;
    active_cmdscript = script;
    cmdscript_setup_execution(g_game_info.cmdscript, 0x2B);
    cmdscript_execute(g_game_info.cmdscript);
    active_cmdscript = saved_script;
    if (script->instance != 0) {
        ((JmtDestroyable*)script)->vtbl->destroy(
            (JmtDestroyable*)script);
    }
}

int rotate_towards_sync(float angle) {
    float magnitude;

    if (angle >= 0.0f) {
        magnitude = angle;
    } else {
        magnitude = -angle;
    }
    return magnitude > 2.7f;
}

int is_reaction_xfer_him_allowed(void) {
    if (g_game_info.feature_flags.bits.high_bit == 0) {
        return 1;
    }
    return 0;
}
