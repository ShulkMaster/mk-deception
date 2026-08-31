#include "game/game_info.h"
#include "game/jmt.h"
#include "game/blood.h"
#include "game/constrain.h"
#include "game/pfxscript.h"
#include "libmkparticle/particle.h"
#include "math/gxMath.h"
#include "runtime/asset.h"
#include "runtime/anim_pdata.h"
#include "runtime/cam.h"
#include "runtime/cstring.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"

static const Vec kabal_smoke_angles = {-1.57079637f, 0.0f, 0.0f};
static const Vec subzero_decoy_angles = {-1.57079637f, 0.0f, 0.0f};

typedef struct JmtEffectNames {
    char ermac_eye_left[6];
    char ermac_eye_right[6];
    char kabal_smoke[11];
    char decoy_model[6];
    char decoy_mist[11];
    char decoy_mist_head[16];
    char decoy_mist_left[14];
    char decoy_mist_right[14];
    char decoy_mist_shin[16];
    char bow_model[4];
    char bow_magic[9];
    char bow_magic_sparks[15];
} JmtEffectNames;

static const JmtEffectNames jmt_effect_names = {
    "eyelt", "eyert", "kabalsmoke", "DECOY", "decoy_mist",
    "decoy_mist_head", "decoy_mist_lt", "decoy_mist_rt",
    "decoy_mist_shin", "BOW", "bowmagic", "bowmagicsparks"
};

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
    MkProc* player_proc;
    unsigned int player_proc_instance;
    PlyrPdata* his_plyr_pdata;
    MkObj* his_obj;
    MkObj* decoy_object;
    unsigned int decoy_instance;
    MkObj* source_object;
    PlyrInfo* owner_info;
    char zeroed_payload[0x508];
    int bone_copy_count;
    const int (*bone_map)[2];
    float lifetime;
    unsigned int field_53C;
    int flash_toggle;
    float flash_timer;
    int flash_count;
    unsigned int effect_handles[9];
} JmtDecoyPdata;

int subzero_clone_bones[20][2] = {
    {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {9, 9},
    {12, 10}, {13, 11}, {14, 12}, {15, 13}, {16, 14}, {17, 15},
    {18, 16}, {19, 17}, {20, 18}, {21, 19}, {22, 20}, {23, 21}
};

int clone_bones[] = {
    0x1000, 0x1001, 0x1002, 0x1003, 0x1004, 0x1005, 0x1006,
    0x1009, 0x100C, 0x100D, 0x100E, 0x100F, 0x1010, 0x1011,
    0x1012, 0x1013, 0x1014, 0x1015, 0x1016, 0x1017, 0
};

typedef struct JmtKabalSmokePdata {
    MkHdr hdr;
    MkProc* player_proc;
    unsigned int player_proc_instance;
    PlyrPdata* his_plyr_pdata;
    MkObj* his_obj;
    MkObj* owner;
    Vec origin;
    float duration;
    unsigned int emitters[10];
} JmtKabalSmokePdata;

typedef struct JmtBowPdata {
    MkHdr hdr;
    MkObj* bow;
    unsigned int bow_instance;
    MkObj* owner;
    int bone;
    float duration;
    float scale;
} JmtBowPdata;

typedef struct JmtSharedAnimations {
    char pad000[0x324];
    void* kabal_falldown;
} JmtSharedAnimations;

typedef struct JmtKabalAnimations {
    char pad000[0x378];
    void* collide_start;
    void* collide_recover;
} JmtKabalAnimations;

extern AnimPdata* plyr_anim_pdata;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern PlyrPdata* his_pdata;
extern JmtSharedAnimations shared_ani;
extern MkObj* g_bgnd_preloaded_models[];
extern int blood_type_list[12];
extern MkPtr* gusher_list;
extern int mode_of_play;
extern float game_speed;
extern void check_release_other_player(void);
extern void swap_active_plyr_proc(void);
extern void adjust_player_life(float amount);
void plyr_bleed_large_ext(PlyrPdata* player, int type, PlyrPdata* owner);
void drone_ai_set_avoidance_area(const float* position, float duration);
void drone_ai_clear_avoidance_area_duration(int player);
unsigned int fx_next_emitter(unsigned int effect);
void fx_reset_emit(unsigned int effect);
void fx_resume_emit(unsigned int effect);
void fx_set_param_v3(
    unsigned int effect, int parameter, float x, float y, float z);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
int build_bones_tbl(MkObj* object, const int* tags);
void pull_bone_hierarchy_mkobj(MkObj* object);
void obj_set_all_sobjs_priority(MkObj* object, int priority);
int pfx_plyr_bankowner(PlyrInfo* player);
unsigned int fx_by_owner(const char* name, unsigned int owner);
void fx_reset(unsigned int effect);
void fx_pause_emit(unsigned int effect);
unsigned int pfxhandle_spawn_at_bid_next(
    unsigned int effect, MkObj* object, int bone);
void RwFrameUpdateObjects(RwFrame* frame);
void obj_for_all_atomics_set_material_alpha(MkObj* object, unsigned int alpha);
int check_for_throw(PlyrPdata* player);
int collide_cylinder_vs_plyr(
    PlyrInfo* player, const Vec* center, const Vec* angles,
    float radius, float height);
void trial_state_collision_check(int collision_result, int player);
int is_big_boss(PlyrPdata* player);

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
void idle_victim(void);
void xfer_player_proc(MkProc* proc, MkProcEntryFn entry);
int get_blood_level(void);
static float p_decoy_shrink(void);
static float p_bow_ctrl(void);
static float p_bow_retract(void);
static float kabal_collide_victim(void);
float j_getup_back_6(void);
void* find_pfx_by_name(const char* name);
void restart_effect_ppfx(void* effect);
static float kabal_collide_victim_falldown(void);
static void start_kabal_smoke_pfx(JmtKabalSmokePdata* pdata);
static float p_kabal_smoke(void);
static float p_create_decoy(void);
static float p_decoy(void);

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
    JmtFloatBits input;
    JmtFloatBits estimate;
    float refined;

    if (!(squared > 0.0f)) {
        return 0.0f;
    }
    input.f = squared;
    estimate.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 10) & 0x3FFE] << 8;
    estimate.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    refined = estimate.f *
        (3.0f - (estimate.f * estimate.f) / squared);
    return 0.5f * refined;
}

static inline int jmt_is_local_plyr(void) {
    if (plyr_pdata == 0) {
        return 1;
    }
    return 1;
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

void remove_impaled_projectiles(void) {
    destroy_mkprocs_pid(0x2026);
    g_game_info.plyr0.slot.pdata->impaled_projectile_state = 0;
    g_game_info.plyr1.slot.pdata->impaled_projectile_state = 0;
}

int is_drone(void) {
    if (plyr_pdata == 0) {
        return 0;
    }
    return plyr_pdata->drone_request != 0;
}

void adjust_kabal_position(void) {
}

float get_adjusted_speed(float speed, float adjustment) {
    return speed;
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
    object_y = plyr_obj->pos.value.y;
    ground_y = g_game_info.field_34;
    elapsed = 0.0f;
    while (object_y > ground_y + exit_height &&
           elapsed < max_ticks) {
        _mkproc_sleep_ticks = 1.0f;
        proc_vtbl = (JmtProcVtable*)aproc->vtbl;
        proc_vtbl->sleep(proc_vtbl);
        ani_1_frame();
        ground_y = g_game_info.field_34;
        object_y = plyr_obj->pos.value.y;

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

int get_current_bgnd(void) {
    return g_game_info.bgnd_id;
}

void set_constrain_last_pos_pdata(const Vec* position) {
    if (plyr_pdata != 0) {
        set_constrain_last_pos(plyr_pdata->plyr_num, position);
    }
}

void kill_ermac_eyes(void) {
    if (plyr_pdata != 0 && plyr_pdata->character_id == 6) {
        fx_reset(fx(jmt_effect_names.ermac_eye_left));
        fx_reset(fx(jmt_effect_names.ermac_eye_right));
    }
}

void dizzy_kill_pfx(
    MkObj* opponent, int unused, PlyrPdata* player, int enabled) {
    if (plyr_pdata != 0) {
        switch (plyr_pdata->character_id) {
        case 6:
            fx_reset(fx(jmt_effect_names.ermac_eye_left));
            fx_reset(fx(jmt_effect_names.ermac_eye_right));
            break;
        }
    }
}

void kabal_collision_control_victim(int falldown) {
    if (plyr_pdata == 0) {
        return;
    }
    plyr_obj->flags_09_bits.tightrope_restricted = 1;
    his_obj->flags_09_bits.tightrope_restricted = 1;
    idle_victim();
    if (falldown == 0) {
        if (plyr_pdata->plyr_num == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr1.idle_proc,
                kabal_collide_victim);
        } else {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr0.idle_proc,
                kabal_collide_victim);
        }
    } else {
        if (plyr_pdata->plyr_num == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr1.idle_proc,
                kabal_collide_victim_falldown);
        } else {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr0.idle_proc,
                kabal_collide_victim_falldown);
        }
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

static float kabal_collide_victim(void) {
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

void jmt_debug_script(int command, int value, const void* args, float scalar) {
}

void start_kabal_smoke(void* script_args, float duration) {
    JmtKabalSmokePdata* pdata;
    MkProc* proc;
    unsigned int* emitter;
    int index;

    (void)script_args;
    pdata = 0;
    if (plyr_pdata == 0) {
        return;
    }
    if (plyr_pdata->plyr_num == 0) {
        if (find_mkproc_pid(0xB011) != 0) {
            return;
        }
    } else if (plyr_pdata->plyr_num == 1) {
        if (find_mkproc_pid(0xB012) != 0) {
            return;
        }
    } else {
        return;
    }

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_tinystack(
            0xB011, 0x1F, p_kabal_smoke,
            sizeof(*pdata), (MkHdr**)&pdata);
    } else {
        proc = _create_mkproc_generic_tinystack(
            0xB012, 0x1F, p_kabal_smoke,
            sizeof(*pdata), (MkHdr**)&pdata);
    }
    if (proc == 0) {
        return;
    }

    pdata->player_proc = plyr_pdata->player_proc;
    pdata->player_proc_instance = plyr_pdata->player_proc_instance;
    pdata->his_plyr_pdata = plyr_pdata->his_plyr_pdata;
    pdata->his_obj = plyr_pdata->his_obj;
    pdata->owner = plyr_obj;
    pdata->duration = duration;
    pdata->origin.x = plyr_obj->pos.value.x;
    pdata->origin.y = plyr_obj->pos.value.y;
    pdata->origin.z = plyr_obj->pos.value.z;
    emitter = pdata->emitters;
    for (index = 0; index < 10; index++) {
        *emitter++ = 0;
    }
    start_kabal_smoke_pfx(pdata);
    drone_ai_set_avoidance_area(&plyr_obj->pos.value.x, duration);
}

static void start_kabal_smoke_pfx(JmtKabalSmokePdata* pdata) {
    MkObj* object;
    unsigned int effect;
    int emitter_count;
    Vec position;

    emitter_count = 0;
    object = plyr_obj;
    if (object == 0) {
        return;
    }
    effect = fx(jmt_effect_names.kabal_smoke);
    if (effect != 0) {
        fx_reset_emit(effect);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 3, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 9, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 1, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 2, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 4, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 5, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 8, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 7, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 0x15, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
    effect = fx_next_emitter(effect);
    if (effect != 0) {
        pdata->emitters[emitter_count++] = effect;
        fx_resume_emit(effect);
        get_bone_world_pos(object, 0x14, &position);
        fx_set_param_v3(effect, 0x202, position.x, position.y, position.z);
    }
}

void destroy_kabal_smoke(void) {
    JmtKabalSmokePdata* pdata;
    MkProc* proc;
    int index;

    if (plyr_pdata == 0) {
        return;
    }
    if (plyr_pdata->plyr_num == 0) {
        proc = find_mkproc_pid(0xB012);
    } else {
        proc = find_mkproc_pid(0xB011);
    }
    if (proc == 0) {
        return;
    }
    pdata = (JmtKabalSmokePdata*)pdata_of_proc(proc);
    if (pdata != 0) {
        for (index = 0; index < 10; index++) {
            if (pdata->emitters[index] != 0) {
                fx_reset_emit(pdata->emitters[index]);
                pdata->emitters[index] = 0;
            }
        }
    }
    if (pdata->owner == g_game_info.plyr0.slot.mirror_a) {
        drone_ai_clear_avoidance_area_duration(1);
    } else if (pdata->owner == g_game_info.plyr1.slot.mirror_a) {
        drone_ai_clear_avoidance_area_duration(0);
    }
    if (proc->instance != 0) {
        ((MkHdr*)proc)->typed_vtbl->destroy((MkHdr*)proc);
    }
}

static float p_kabal_smoke(void) {
    Vec angles = kabal_smoke_angles;
    Vec center;
    JmtKabalSmokePdata* pdata;
    PlyrInfo* player;

    pdata = (JmtKabalSmokePdata*)pdata_of_proc(aproc);
    if (pdata->owner == 0) {
        return -1.0f;
    }
    player = 0;
    if (pdata->owner == g_game_info.plyr0.slot.mirror_a) {
        player = &g_game_info.plyr1;
    } else if (pdata->owner == g_game_info.plyr1.slot.mirror_a) {
        player = &g_game_info.plyr0;
    }
    pdata->duration -= game_speed;
    if (pdata->duration < 0.0f || g_game_info.flag_bits.field_bit0) {
        trial_state_collision_check(0, player->controller_slot);
        return -1.0f;
    }
    if (check_for_throw(player->slot.pdata) != 0) {
        return 1.0f;
    }
    if (g_game_info.feature_flags.bits.high_bit == 0) {
        center.x = pdata->origin.x;
        center.y = pdata->origin.y;
        center.z = pdata->origin.z;
        center.y -= 1.0f;
        if (collide_cylinder_vs_plyr(
                player, &center, &angles, 0.25f, 1.75f) != 0) {
            trial_state_collision_check(1, player->controller_slot);
            if (is_big_boss(player->slot.pdata) == 0 &&
                !player->slot.pdata->state_flags.bits.projectile_invulnerable) {
                reaction_xfer_him(0xF8, 0.0f, 0);
            }
        }
    }
    return 1.0f;
}

void start_subzero_decoy(void* script_args, float duration) {
    JmtDecoyPdata* pdata;
    MkObj* decoy;
    MkProc* proc;
    int art_slot;

    (void)script_args;
    pdata = 0;
    if (plyr_pdata == 0) {
        return;
    }
    if (plyr_pdata->plyr_num == 0) {
        if (find_mkproc_pid(0xB00E) != 0) {
            return;
        }
    } else if (plyr_pdata->plyr_num == 1) {
        if (find_mkproc_pid(0xB00F) != 0) {
            return;
        }
    } else {
        return;
    }

    art_slot = 0x4000A;
    if (plyr_pdata->plyr_num == 0) {
        art_slot = 0x3000A;
    }
    decoy = (MkObj*)load_named_model_from_slot(
        art_slot, jmt_effect_names.decoy_model, 0xD003, 0);
    if (decoy == 0) {
        return;
    }
    if (build_bones_tbl(decoy, clone_bones) == 0) {
        if (decoy->hdr.instance != 0) {
            ((MkHdr*)decoy)->typed_vtbl->destroy((MkHdr*)decoy);
        }
        return;
    }

    pull_bone_hierarchy_mkobj(decoy);
    decoy->pos.value.x = plyr_obj->pos.value.x;
    decoy->pos.value.y = plyr_obj->pos.value.y;
    decoy->pos.value.z = plyr_obj->pos.value.z;
    decoy->ang.x = plyr_obj->ang.x;
    decoy->ang.y = plyr_obj->ang.y;
    decoy->ang.z = plyr_obj->ang.z;
    hide_obj(decoy);
    decoy->light_flags = 0x2000;
    obj_create_sobjs(decoy);
    obj_set_all_sobjs_priority(decoy, 0x13);
    insert_fgnd_mkobj(decoy);

    if (plyr_pdata->plyr_num == 0) {
        proc = _create_mkproc_generic_tinystack(
            0xB00E, 0x1F, p_create_decoy, sizeof(*pdata), (MkHdr**)&pdata);
    } else {
        proc = _create_mkproc_generic_tinystack(
            0xB00F, 0x1F, p_create_decoy, sizeof(*pdata), (MkHdr**)&pdata);
    }
    if (proc == 0) {
        if (decoy->hdr.instance != 0) {
            ((MkHdr*)decoy)->typed_vtbl->destroy((MkHdr*)decoy);
        }
        return;
    }

    zero_pdata_payload(sizeof(*pdata), &pdata->hdr);
    pdata->decoy_object = decoy;
    pdata->decoy_instance = decoy->hdr.instance;
    mk_insert(&decoy->hdr, &proc->pdata_list);
    pdata->player_proc = plyr_pdata->player_proc;
    pdata->player_proc_instance = plyr_pdata->player_proc_instance;
    pdata->his_plyr_pdata = plyr_pdata->his_plyr_pdata;
    pdata->his_obj = plyr_pdata->his_obj;
    pdata->source_object = plyr_obj;
    pdata->owner_info = plyr_pdata->plyr_info;
    pdata->lifetime = duration;
    pdata->flash_toggle = 1;
    pdata->flash_timer = 0.0f;
    pdata->flash_count = 22;
    pdata->bone_copy_count = 20;
    pdata->bone_map = subzero_clone_bones;
    drone_ai_set_avoidance_area(&plyr_obj->pos.value.x, duration);
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
    object = pdata->decoy_object;
    if (object != 0) {
        if (object->hdr.instance == pdata->decoy_instance) {
            /* Keep the validated decoy object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object == 0) {
        if (proc->instance != 0) {
            ((MkHdr*)proc)->typed_vtbl->destroy((MkHdr*)proc);
        }
        return;
    }

    object->flags_08 |= 2;
    object->scale.x = 1.0f;
    object->scale.y = 1.0f;
    object->scale.z = 1.0f;
    pdata->lifetime = 15.0f;
    xfer_proc(proc, p_decoy_shrink);
}

static float p_create_decoy(void) {
    JmtDecoyPdata* pdata;
    JmtProcVtable* proc_vtbl;
    MkObj* decoy;
    MkObj* source;
    MkBone* source_bone;
    MkBone* decoy_bone;
    unsigned int effect;
    int index;
    int source_index;

    pdata = (JmtDecoyPdata*)pdata_of_proc(aproc);
    decoy = pdata->decoy_object;
    if (decoy != 0) {
        if (decoy->hdr.instance == pdata->decoy_instance) {
            /* Keep the validated decoy object. */
        } else {
            decoy = 0;
        }
    } else {
        decoy = 0;
    }
    if (decoy == 0) {
        return -1.0f;
    }
    source = pdata->source_object;
    if (source == 0) {
        return -1.0f;
    }
    if (pdata->bone_copy_count > 20) {
        return -1.0f;
    }

    for (index = 0; index < pdata->bone_copy_count; index++) {
        source_index = pdata->bone_map[index][0];
        if (source_index < (int)source->bone_count &&
            index < (int)decoy->bone_count) {
            source_bone = source->bones[source_index];
            decoy_bone = decoy->bones[index];
            if (source_bone != 0 && decoy_bone != 0 &&
                source_bone->parent_matrix != 0 &&
                decoy_bone->parent_matrix != 0) {
                memcpy(
                    decoy_bone->parent_matrix,
                    source_bone->parent_matrix,
                    sizeof(*decoy_bone->parent_matrix));
            }
        }
    }
    memcpy(decoy->field_24, source->field_24, sizeof(*decoy->field_24));
    RwFrameUpdateObjects(decoy->frame);

    effect = fx_by_owner(
        jmt_effect_names.decoy_mist,
        pfx_plyr_bankowner(pdata->owner_info));
    fx_reset(effect);
    pdata->effect_handles[0] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 1);
    pdata->effect_handles[1] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 2);

    effect = fx_by_owner(
        jmt_effect_names.decoy_mist_head,
        pfx_plyr_bankowner(pdata->owner_info));
    fx_reset(effect);
    pdata->effect_handles[2] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 9);

    effect = fx_by_owner(
        jmt_effect_names.decoy_mist_left,
        pfx_plyr_bankowner(pdata->owner_info));
    fx_reset(effect);
    pdata->effect_handles[3] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 0xE);
    pdata->effect_handles[4] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 0x12);

    effect = fx_by_owner(
        jmt_effect_names.decoy_mist_right,
        pfx_plyr_bankowner(pdata->owner_info));
    fx_reset(effect);
    pdata->effect_handles[5] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 0xF);
    pdata->effect_handles[6] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 0x13);

    effect = fx_by_owner(
        jmt_effect_names.decoy_mist_shin,
        pfx_plyr_bankowner(pdata->owner_info));
    fx_reset(effect);
    pdata->effect_handles[7] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 4);
    pdata->effect_handles[8] =
        pfxhandle_spawn_at_bid_next(effect, decoy, 5);

    unhide_obj(decoy);
    proc_vtbl = (JmtProcVtable*)aproc->vtbl;
    proc_vtbl->jump_sleep(p_decoy, 0.0f);
    return 0.0f;
}

static float p_decoy(void) {
    Vec angles = subzero_decoy_angles;
    JmtDecoyPdata* pdata;
    JmtProcVtable* proc_vtbl;
    MkObj* decoy;
    PlyrInfo* player;
    Vec center;
    RwRGBA dark_color = {0x80, 0x80, 0xFF, 0xC8};
    RwRGBA light_color = {0xE1, 0xE1, 0xFF, 0xC8};

    player = 0;
    pdata = (JmtDecoyPdata*)pdata_of_proc(aproc);
    decoy = pdata->decoy_object;
    if (decoy != 0) {
        if (decoy->hdr.instance == pdata->decoy_instance) {
            /* Keep the validated decoy object. */
        } else {
            decoy = 0;
        }
    } else {
        decoy = 0;
    }
    if (decoy == 0 || pdata->source_object == 0) {
        return -1.0f;
    }
    if (pdata->source_object == g_game_info.plyr0.slot.mirror_a) {
        player = &g_game_info.plyr1;
    } else if (pdata->source_object == g_game_info.plyr1.slot.mirror_a) {
        player = &g_game_info.plyr0;
    }

    pdata->lifetime -= game_speed;
    if (pdata->lifetime < 0.0f || g_game_info.flag_bits.field_bit0) {
        trial_state_collision_check(0, player->controller_slot);
        decoy->flags_08 |= 2;
        decoy->scale.x = 1.0f;
        decoy->scale.y = 1.0f;
        decoy->scale.z = 1.0f;
        pdata->lifetime = 15.0f;
        proc_vtbl = (JmtProcVtable*)aproc->vtbl;
        proc_vtbl->jump_sleep(p_decoy_shrink, 0.0f);
        return 0.0f;
    }

    pdata->flash_timer -= game_speed;
    if (pdata->flash_timer < 0.0f && pdata->flash_count > 0) {
        pdata->flash_timer = 1.0f;
        if (pdata->flash_toggle == 0) {
            pdata->flash_toggle = 1;
            obj_set_color_for_all_materials(decoy, &dark_color);
            pdata->flash_count--;
        } else if (pdata->flash_toggle == 1) {
            pdata->flash_toggle = 0;
            obj_set_color_for_all_materials(decoy, &light_color);
            pdata->flash_count--;
        }
        if (pdata->flash_count <= 0) {
            pdata->flash_count = 0;
            obj_set_color_for_all_materials(decoy, &dark_color);
        }
    }

    if (check_for_throw(player->slot.pdata) != 0) {
        return 1.0f;
    }
    if (g_game_info.feature_flags.bits.high_bit == 0) {
        center.x = decoy->pos.value.x;
        center.y = decoy->pos.value.y;
        center.z = decoy->pos.value.z;
        center.y -= 1.0f;
        if (collide_cylinder_vs_plyr(
                player, &center, &angles, 0.25f, 1.75f) != 0) {
            trial_state_collision_check(1, player->controller_slot);
            if (is_big_boss(player->slot.pdata) == 0 &&
                !player->slot.pdata->state_flags.bits.projectile_invulnerable) {
                reaction_xfer_him(0xA0, 0.0f, 0);
            }
        }
    }
    return 1.0f;
}

static float p_decoy_shrink(void) {
    JmtDecoyPdata* pdata;
    JmtProcVtable* proc_vtbl;
    MkObj* decoy;
    unsigned int effect;
    int index;

    pdata = (JmtDecoyPdata*)pdata_of_proc(aproc);
    for (index = 0; index < 9; index++) {
        effect = pdata->effect_handles[index];
        if (effect != 0) {
            fx_pause_emit(effect);
            fx_reset_emit(effect);
        }
    }

    while (pdata->lifetime > 0.0f) {
        decoy = pdata->decoy_object;
        if (decoy != 0) {
            if (decoy->hdr.instance == pdata->decoy_instance) {
                /* Keep the validated decoy object. */
            } else {
                decoy = 0;
            }
        } else {
            decoy = 0;
        }
        if (decoy == 0 || pdata->source_object == 0) {
            return -1.0f;
        }
        pdata->lifetime -= game_speed;
        obj_for_all_atomics_set_material_alpha(
            decoy, (unsigned int)(200.0f * (pdata->lifetime / 15.0f)));
        _mkproc_sleep_ticks = 1.0f;
        proc_vtbl = (JmtProcVtable*)aproc->vtbl;
        proc_vtbl->sleep(proc_vtbl);
    }

    if (pdata->source_object == g_game_info.plyr0.slot.mirror_a) {
        drone_ai_clear_avoidance_area_duration(1);
    } else if (pdata->source_object == g_game_info.plyr1.slot.mirror_a) {
        drone_ai_clear_avoidance_area_duration(0);
    }
    return -1.0f;
}

void start_bow(int bone, float duration) {
    JmtBowPdata* pdata;
    MkObj* bow;
    MkProc* proc;
    MkPfx* effect;
    unsigned int handle;
    Vec position;

    pdata = 0;
    bow = load_named_model_for_player(
        jmt_effect_names.bow_model, plyr_pdata->plyr_num, 0xD002, 0);
    if (bow == 0) {
        return;
    }
    bow->flags_08 |= 0x40;
    bow->flags_08 |= 8;
    bow->flags_08 |= 2;
    bow->scale.x = 1.0f;
    bow->scale.y = 0.0f;
    bow->scale.z = 1.0f;
    insert_fgnd_mkobj(bow);

    proc = _create_mkproc_generic_tinystack(
        0xB009, 0x1F, p_bow_ctrl, sizeof(*pdata), (MkHdr**)&pdata);
    if (proc == 0) {
        if (bow->hdr.instance != 0) {
            ((MkHdr*)bow)->typed_vtbl->destroy((MkHdr*)bow);
        }
        return;
    }
    pdata->bow = bow;
    pdata->bow_instance = bow->hdr.instance;
    mk_insert(&bow->hdr, &proc->pdata_list);
    pdata->owner = plyr_obj;
    pdata->bone = bone;
    pdata->duration = duration;
    get_bone_world_pos(pdata->owner, pdata->bone, &position);
    bow->pos.value.x = position.x;
    bow->pos.value.y = position.y;
    bow->pos.value.z = position.z;
    bow->ang.y = pdata->owner->ang.y;
    pdata->scale = 0.0f;

    handle = fx(jmt_effect_names.bow_magic);
    effect = find_pfx_by_handle(handle);
    if (effect != 0) {
        pfx_bind_emitter_to_obj(effect, bow, 0);
        fx_reset(handle);
        fx_resume_emit(handle);
    }
    handle = fx(jmt_effect_names.bow_magic_sparks);
    effect = find_pfx_by_handle(handle);
    if (effect != 0) {
        pfx_bind_emitter_to_obj(effect, bow, 0);
        fx_reset(handle);
        fx_resume_emit(handle);
    }
}

static float p_bow_ctrl(void) {
    JmtBowPdata* pdata;
    JmtProcVtable* proc_vtbl;
    MkObj* bow;
    Vec position;

    pdata = (JmtBowPdata*)pdata_of_proc(aproc);
    bow = pdata->bow;
    if (bow != 0) {
        if (bow->hdr.instance == pdata->bow_instance) {
            /* Keep the validated bow object. */
        } else {
            bow = 0;
        }
    } else {
        bow = 0;
    }
    if (bow == 0 || pdata->owner == 0) {
        return -1.0f;
    }
    pdata->duration -= game_speed;
    if (pdata->duration < 0.0f) {
        proc_vtbl = (JmtProcVtable*)aproc->vtbl;
        proc_vtbl->jump_sleep(p_bow_retract, 0.0f);
        return 0.0f;
    }
    get_bone_world_pos(pdata->owner, pdata->bone, &position);
    bow->pos.value.x = position.x;
    bow->pos.value.y = position.y;
    bow->pos.value.z = position.z;
    bow->ang.y = pdata->owner->ang.y;
    pdata->scale += 0.1f;
    if (pdata->scale > 1.0f) {
        pdata->scale = 1.0f;
    }
    bow->scale.x = 1.0f;
    bow->scale.y = pdata->scale;
    bow->scale.z = 1.0f;
    return 1.0f;
}

static float p_bow_retract(void) {
    JmtBowPdata* pdata;
    MkObj* bow;
    Vec position;

    pdata = (JmtBowPdata*)pdata_of_proc(aproc);
    bow = pdata->bow;
    if (bow != 0) {
        if (bow->hdr.instance == pdata->bow_instance) {
            /* Keep the validated bow object. */
        } else {
            bow = 0;
        }
    } else {
        bow = 0;
    }
    if (bow == 0 || pdata->owner == 0) {
        return -1.0f;
    }
    get_bone_world_pos(pdata->owner, pdata->bone, &position);
    bow->pos.value.x = position.x;
    bow->pos.value.y = position.y;
    bow->pos.value.z = position.z;
    bow->ang.y = pdata->owner->ang.y;
    pdata->scale -= 0.2f;
    if (pdata->scale < 0.0f) {
        return -1.0f;
    }
    bow->scale.x = 1.0f;
    bow->scale.y = pdata->scale;
    bow->scale.z = 1.0f;
    return 1.0f;
}

int single_frame_collision_check(
    int region, int reaction, int strength, void* script_args,
    float radius, float height, float reaction_rate) {
    (void)script_args;
    if (collision_2(region, radius, height) != 0) {
        set_collision_made_flag();
        reaction_xfer_him(reaction, reaction_rate, strength);
        return 1;
    }
    return 0;
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

void plyr_set_gravity(float gravity) {
    plyr_obj->gravity = gravity;
    plyr_obj->flags_08_bits.moving = 1;
    plyr_obj->flags_09_bits.launched = 1;
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
    float result;

    result = 0.0f;
    if (player == 0) {
        victim = g_game_info.plyr1.slot.mirror_a;
        target = g_game_info.plyr0.slot.mirror_a;
    } else {
        victim = g_game_info.plyr0.slot.mirror_a;
        target = g_game_info.plyr1.slot.mirror_a;
    }
    if (victim != 0 && target != 0) {
        victim_x = victim->pos.value.x;
        victim_z = victim->pos.value.z;
        victim_inverse_length =
            jmt_fast_inverse_sqrt(victim_x * victim_x + victim_z * victim_z);
        target_z = target->pos.value.z - victim_z;
        target_x = target->pos.value.x - victim_x;
        target_inverse_length =
            jmt_fast_inverse_sqrt(target_x * target_x + target_z * target_z);
        result = (target_x * target_inverse_length) *
                     (victim_x * victim_inverse_length) +
                 (target_z * target_inverse_length) *
                     (victim_z * victim_inverse_length);
    }
    return result;
}

void resume_effect_at_plyr_num_bid(
    int player_num, int bone, unsigned int handle, int bind_mode,
    int requires_blood) {
    MkObj* object;
    MkPfx* effect;
    Vec position;

    if (player_num == g_game_info.plyr0.slot.pdata->plyr_num) {
        object = g_game_info.plyr0.slot.mirror_a;
        if ((requires_blood != 1 ||
             get_blood_level() >= blood_type_list[11]) &&
            (effect = find_pfx_by_handle(handle)) != 0) {
            if (bind_mode == 1) {
                get_bone_world_pos(object, bone, &position);
                fx_set_param_v3(
                    handle, 0x202, position.x, position.y, position.z);
            } else if (bind_mode == 3) {
                pfx_bind_emitter_to_obj_bone(effect, object, bone);
            } else {
                pfx_bind_emitter_to_obj(effect, object, 0);
            }
            fx_reset(handle);
            fx_resume_emit(handle);
        }
    } else {
        object = g_game_info.plyr1.slot.mirror_a;
        if ((requires_blood != 1 ||
             get_blood_level() >= blood_type_list[11]) &&
            (effect = find_pfx_by_handle(handle)) != 0) {
            if (bind_mode == 1) {
                get_bone_world_pos(object, bone, &position);
                fx_set_param_v3(
                    handle, 0x202, position.x, position.y, position.z);
            } else if (bind_mode == 3) {
                pfx_bind_emitter_to_obj_bone(effect, object, bone);
            } else {
                pfx_bind_emitter_to_obj(effect, object, 0);
            }
            fx_reset(handle);
            fx_resume_emit(handle);
        }
    }
}

void resume_effect_at_obj_bid(
    MkObj* object, int bone, unsigned int handle, int bind_mode,
    int requires_blood) {
    MkPfx* effect;
    Vec position;

    if ((requires_blood != 1 ||
         get_blood_level() >= blood_type_list[11]) &&
        (effect = find_pfx_by_handle(handle)) != 0) {
        if (bind_mode == 1) {
            get_bone_world_pos(object, bone, &position);
            fx_set_param_v3(
                handle, 0x202, position.x, position.y, position.z);
        } else if (bind_mode == 3) {
            pfx_bind_emitter_to_obj_bone(effect, object, bone);
        } else {
            pfx_bind_emitter_to_obj(effect, object, 0);
        }
        fx_reset(handle);
        fx_resume_emit(handle);
    }
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
    if (get_blood_level() < blood_type_list[11]) {
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
    gusher = (MkHdr*)start_gusher(
        heart_beat, info->slot.fighter, info->slot.mirror_a, bone,
        &direction, &velocity);
    if (gusher != 0) {
        mk_insert(gusher, &gusher_list);
    }
    return gusher;
}

void mks_victim_bleed(int player, int type) {
    PlyrPdata* victim;

    victim = g_game_info.plyr0.slot.pdata;
    if (player == 1) {
        victim = g_game_info.plyr1.slot.pdata;
    }
    if (victim != 0) {
        plyr_bleed_large_ext(victim, type, victim);
    }
}

void mks_plyr_stop(int player) {
    MkObj* object;

    object = g_game_info.plyr0.slot.mirror_a;
    if (player == 1) {
        object = g_game_info.plyr1.slot.mirror_a;
    }
    if (object != 0) {
        object->flags_08_bits.moving = 0;
        object->gravity = 0.0f;
        object->pos_vel.z = 0.0f;
        object->pos_vel.y = 0.0f;
        object->pos_vel.x = 0.0f;
    }
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
    object = g_game_info.plyr0.slot.mirror_a;
    if (player == 1) {
        object = g_game_info.plyr1.slot.mirror_a;
    }
    if (object == 0) {
        return;
    }

    length = jmt_fast_sqrt(
        object->pos.value.x * object->pos.value.x +
        object->pos.value.z * object->pos.value.z);
    if (length <= 0.0f) {
        inverse_length = length;
    } else {
        inverse_length = 1.0f / length;
    }
    if (length != 0.0f) {
        normalized_x = -object->pos.value.x * inverse_length;
        normalized_z = -object->pos.value.z * inverse_length;
        angle_bits = (int)(166886.1f *
            (angle_offset + gxMathArcTanYX(normalized_x, normalized_z)));
        angle_bits &= 0xFFFFF;
        object->ang.y = 0.000005992112f * (float)angle_bits;
    }
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
        victim->pos.value.x * victim->pos.value.x +
        victim->pos.value.z * victim->pos.value.z);
    if (length <= 0.0f) {
        inverse_length = length;
    } else {
        inverse_length = 1.0f / length;
    }
    if (length != 0.0f) {
        camera_set_movement_offset_explicit(
            victim->pos.value.x * inverse_length * distance,
            height,
            victim->pos.value.z * inverse_length * distance);
    }
}

void mks_bgnd_pfx_bind_to_sobj(
    const char* effect_name, unsigned int sobj_id) {
    MkSobj* sobj;
    void* effect;
    MkPfx* pfx;
    PfxEmitterFlagsView* emitter;

    sobj = (MkSobj*)obj_find_sobj_by_id(g_game_info.bgnd_obj, sobj_id);
    if (sobj != 0) {
        effect = find_pfx_by_name(effect_name);
        if (effect != 0) {
            restart_effect_ppfx(effect);
            pfx = (MkPfx*)effect;
            pfx_bind_emitter_to_sobj(pfx, sobj, 0);
            emitter = (PfxEmitterFlagsView*)pfx_get_emitter(
                (PfxEmitterTableView*)pfx->matrix, 0);
            emitter->high_bit = 0;
        }
    }
}

void mks_npc_build_bones_tbl(int model_index, const int* bone_tags) {
    MkObj* model;

    model = g_bgnd_preloaded_models[model_index];
    if (model != 0) {
        build_bones_tbl(model, bone_tags);
    }
}

void check_bgnd_effect(void) {
    CmdScript* script;
    CmdScript* saved_script;

    if (g_game_info.bgnd_id == 6 && mode_of_play != 6) {
        script = alloc_cmdscript();
        saved_script = active_cmdscript;
        active_cmdscript = script;
        cmdscript_setup_execution(g_game_info.cmdscript, 0x2B);
        cmdscript_execute(g_game_info.cmdscript);
        active_cmdscript = saved_script;
        if (script->instance != 0) {
            ((MkHdr*)script)->typed_vtbl->destroy((MkHdr*)script);
        }
    }
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

int rotate_towards_sync(float angle) {
    float magnitude;

    if (angle >= 0.0f) {
        magnitude = angle;
    } else {
        magnitude = -angle;
    }
    if (magnitude > 2.7f) {
        return 1;
    }
    return 0;
}

void online_combo_record(void) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void online_combo_adjust(float* horizontal, float* vertical) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void enable_no_sync_anim_f(int enabled) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void enable_no_adjustment_f(int enabled) {
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        return;
    }
}

void kill_plyr_life(int player) {
    adjust_player_life(-1.0f);
}

int is_reaction_xfer_him_allowed(void) {
    if (g_game_info.feature_flags.bits.high_bit == 0) {
        return 1;
    }
    if (jmt_is_local_plyr() == 1) {
        return 0;
    }
    return 1;
}

int is_local_plyr(void) {
    return jmt_is_local_plyr();
}
