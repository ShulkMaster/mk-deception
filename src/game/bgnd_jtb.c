#include "game/game_info.h"
#include "math/gxMath.h"
#include "math/gxVect.h"
#include "math/mk_math.h"
#include "platform/gcutils.h"
#include "runtime/anim_pdata.h"
#include "runtime/cam.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"

typedef struct BgndJtbProcVtable {
    void* reserved[6];
    void (*sleep)(void);
} BgndJtbProcVtable;

typedef union NbFloatBits {
    float f;
    unsigned int u;
} NbFloatBits;

static inline float nb_sqrt(float value) {
    NbFloatBits input;
    NbFloatBits estimate;
    float refined;

    if (value <= 0.0f) {
        return 0.0f;
    }
    input.f = value;
    /* The SDK masks a byte offset into its packed halfword lookup table. */
    estimate.u =
        (unsigned int)*(unsigned short*)((char*)GXMathSqrtTable +
                                        ((input.u >> 10) & 0x3FFE)) <<
        8;
    estimate.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    refined = estimate.f * (3.0f - (estimate.f * estimate.f) / value);
    return 0.5f * refined;
}

extern AnimPdata* plyr_anim_pdata;
extern MkObj* plyr_obj;
extern MkProc* aproc;
extern float _mkproc_sleep_ticks;

void transition_to_anim_script(
    AnimPdata* pdata, void* script, int flags, float transition);
void ani_to_frame_x(float frame);
void launch_me_up(float velocity, float gravity);
void land_chores(int sound, int flags, float velocity, float gravity);
void update_bone_hierarchy(MkHdr* object);
void ground_me(MkHdr* object);

/* Soft ceiling: 91.70% -- equivalent FPR scheduling and fused arithmetic. */
void lower_mines_ani_to_point(
    void* script, int landing_sound, Vec* target, unsigned int frame_offset,
    float start_frame, float animation_step, float end_frame,
    float vertical_velocity, float gravity, float transition) {
    MkHdr* object_header;
    float root;
    float radicand;
    float frames;
    float root_a;
    float root_b;
    float inverse_frames;

    plyr_anim_pdata->flags |= 0x40;
    transition_to_anim_script(
        plyr_anim_pdata, script, 0x43, transition);

    _mkproc_sleep_ticks = 1.0f;
    ((BgndJtbProcVtable*)aproc->vtbl)->sleep();

    if (start_frame != 0.0f) {
        plyr_anim_pdata->step = animation_step;
        ani_to_frame_x(start_frame);
        plyr_anim_pdata->step = 1.0f;
    }

    launch_me_up(vertical_velocity, gravity);
    plyr_obj->flags_09_bits.launched = 0;

    radicand = vertical_velocity * vertical_velocity -
        (2.0f * gravity) *
            ((plyr_obj->pos.y - 0.19f) - plyr_obj->ground_colls_y);
    root = 0.001f;
    if (radicand >= root) {
        root = radicand;
    }
    root = nb_sqrt(root);

    root_a = (root - vertical_velocity) / gravity;
    root_b = (-root - vertical_velocity) / gravity;
    if (root_a < 0.0f ||
        (root_b > 0.0f && root_b < root_a)) {
        root_a = root_b;
    }

    frames = 1.0f;
    radicand = root_a - (float)frame_offset;
    if (radicand >= frames) {
        frames = radicand;
    }

    inverse_frames = 1.0f / frames;
    plyr_anim_pdata->step = (end_frame - start_frame) / frames;
    plyr_obj->pos_vel.x =
        (target->x - plyr_obj->pos.x) * inverse_frames;
    plyr_obj->pos_vel.z =
        (target->z - plyr_obj->pos.z) * inverse_frames;
    ani_to_frame_x(end_frame);

    plyr_obj->flags_09_bits.launched = 1;
    if (plyr_obj != 0) {
        object_header = as_mkhdr((MkHdr*)plyr_obj);
    } else {
        object_header = 0;
    }
    update_bone_hierarchy(object_header);
    if (plyr_obj != 0) {
        object_header = as_mkhdr((MkHdr*)plyr_obj);
    } else {
        object_header = 0;
    }
    ground_me(object_header);
    land_chores(landing_sound, 0, 0.0f, 0.0f);
}

typedef struct NbPendulumState {
    char pad00[0x88];
    float acceleration_divisor; /* +0x88 */
    char pad8C[4];
    float acceleration_scale;   /* +0x90 */
    char pad94[4];
    float swing_angle;          /* +0x98 */
    char pad9C[0xC];
    int swing_ticks;            /* +0xA8 */
} NbPendulumState;

typedef struct NbNpcState {
    MkHdr hdr;
    int npc_id; /* +0x08 */
    MkObj* object; /* +0x0C */
    char pad10[0x14];
    Vec anchor; /* +0x24 */
    Vec momentum; /* +0x30 */
    float last_hit_id[2]; /* +0x3C */
    float field_44;
    char pad48[0x3C];
    float rope_length; /* +0x84 */
    float acceleration_divisor; /* +0x88 */
    float field_8C;
    float acceleration_scale; /* +0x90 */
    float swing_angle; /* +0x94 */
    float phase; /* +0x98 */
    char pad9C[8];
    int active; /* +0xA4 */
    int swing_ticks; /* +0xA8 */
} NbNpcState;

typedef struct NbNpcHitState {
    MkHdr hdr;
    char pad08[4];
    MkObj* object; /* +0x0C */
    char pad10[0x20];
    float direction_x; /* +0x30 */
    char pad34[4];
    float direction_z; /* +0x38 */
} NbNpcHitState;

typedef struct NbNpcProcPdata {
    MkHdr hdr;
    NbNpcState* npc;
} NbNpcProcPdata;

typedef struct NbFighterObjectSlot {
    char pad00[0x5C];
    MkObj* object;
} NbFighterObjectSlot;

typedef struct NbFighterHurtView {
    char pad00[0x14];
    MkObj* opponent_object;
    NbFighterObjectSlot* object_slot;
    char pad1C[0x5A4];
    AnimPdata anim_pdata; /* +0x5C0 */
} NbFighterHurtView;

float p_npc_on_pendulum_rope(void);
int bgnd_preload_named_model(
    const char* model_name, const char* instance_name, int flags,
    float scale, const Vec* rotation);
void bgnd_set_active_sobj_in_obj(int model_index, int object_id);
void bgnd_unhide_preload_obj(int model_index);
void bgnd_unhide_active_sobj(void);
void bgnd_set_active_sobj_pos(float x, float y, float z);
void bgnd_preload_obj_attach_rope(int model_index);
void bgnd_create_named_npc_in_slot(
    int npc_id, int model_slot, int model_id, int flags);
void bgnd_add_brains_to_npc(int npc_id, MkProcEntryFn brains);
MkObj* bgnd_fetch_obj(int object_id);
NbNpcState* bgnd_fetch_npc(int npc_id);
void bgnd_attach_rope_to_bgnd_obj(
    int rope_model_index, int target_model_index, int object_id);
void bgnd_rope_adjust_length(
    int model_index, int preserve_shape, float length);
void bgnd_npc_add_collision_shape(
    int npc_id, int shape_id, int shape_type, float radius, float height,
    float offset_y, float offset_z);
unsigned long random_hit(int group);
void uv_from_angle_y(Vec* out, float angle);
void xfer_player_proc_to_script_manual_messaging(
    FighterMirror* fighter, MkObj* object, int message);
void get_player_proc(MkObj* object);
void xfer_player_proc(void* script);
int is_my_chest_to_screen(void);
int bgnd_collision_if_disable_col(int list_id, int collision_id);
int bgnd_collision_if_enable_col(int list_id, int collision_id);
int spad_set_vector(int index, int source);
float spad_get_pos(int index, int component);
int spad_sub_vectors(int lhs, int rhs, int out);
int spad_norm_vector(int index);
int spad_scale_vector(int destination, int source, float scale);
int spad_set_vector_y(int index, float value);
int spad_set_vector_setting(
    int index, float x, float y, float z);
float spad_xz_dot_xz(int lhs, int rhs);
float spad_xz_length_vector(int index);
int reaction_fetch_current_power_level(int player_index);
int reaction_fetch_current_flags(int player_index);
int is_pX_airborn(int player_index);
float frand(float range);
unsigned short randu0(unsigned int maximum);
unsigned long snd_req(int sound_id);
void* bgnd_launch_fx_at_bid_of_mkobj(
    const char* effect_name, MkObj* object, int bone);
int bgnd_pebble_set_current_pebble(int pebble, int index);
int bgnd_pebble_set_current_info(int info, void* object, float value);

extern MkObj* his_obj;
extern unsigned char r_chest2_stumble[];
extern unsigned int exec_tick_ctr;

static unsigned int last_slave_hit_sound_time;

static inline float nb_fast_inverse_sqrt(float squared) {
    NbFloatBits bits;
    float estimate;
    float product;
    float correction;

    if (squared <= 0.0f) {
        return 0.0f;
    }

    bits.f = squared;
    bits.u = 0x5F375A00U - (bits.u >> 1);
    estimate = bits.f;
    product = estimate * (squared * estimate);
    correction = 3.0f - product;
    return 0.0625f * estimate * correction *
           -(correction * (product * correction) - 12.0f);
}

/*
 * Soft ceiling: nb_get_desired_acceleration ~91.14% -- remaining differences
 * are FPR load/operand scheduling and fused tangent-plane projection math.
 */
static void nb_get_desired_acceleration(
    NbPendulumState* state, Vec* acceleration, const Vec* surface_normal) {
    float force_z;
    float force_y;
    float force_x;
    float swing_angle;
    float scale;
    float inv_length;
    float squared_length;
    float normal_component;

    force_z = 0.0f;
    acceleration->z = 0.0f;
    force_y = 0.0f;
    force_x = 0.0f;
    acceleration->y = 0.0f;
    acceleration->x = 0.0f;
    swing_angle = state->swing_angle;

    if (state->swing_ticks < 0) {
        force_y = -1.0f;
    } else {
        scale = 24.0f * (float)refresh_rate();
        if (scale != 0.0f) {
            scale = (float)state->swing_ticks / scale;
            if (scale > 1.0f) {
                scale = 1.0f;
            }
            force_x = scale * gxMathSin(swing_angle);
            force_z = scale * gxMathCos(swing_angle);
            force_y = -6.0f;
        }
    }

    squared_length =
        force_z * force_z + (force_x * force_x + force_y * force_y);
    inv_length = nb_fast_inverse_sqrt(squared_length);

    force_y *= inv_length;
    force_x *= inv_length;
    force_z *= inv_length;
    force_y *= state->acceleration_scale;
    force_x *= state->acceleration_scale;
    force_z *= state->acceleration_scale;

    normal_component =
        -(force_z * surface_normal->z +
          (force_x * surface_normal->x + force_y * surface_normal->y));

    acceleration->x =
        (surface_normal->x * normal_component + force_x) /
        state->acceleration_divisor;
    acceleration->y =
        (surface_normal->y * normal_component + force_y) /
        state->acceleration_divisor;
    acceleration->z =
        (surface_normal->z * normal_component + force_z) /
        state->acceleration_divisor;
}

void bgnd_jtb_debug_info(void) {
}

/* Soft ceiling: 90.37% -- string-pool placement and NV register lifetimes. */
void nb_place_slave_in_bgnd(
    int npc_id, int rope_model_index, int model_slot, int model_id,
    float anchor_x, float anchor_y, float anchor_z, float rope_length,
    float local_angle_x, float local_angle_y, float local_angle_z,
    float object_angle_x, float object_angle_y, float object_angle_z,
    float acceleration_divisor, float acceleration_scale, float field_8C) {
    static const Vec zero_vector = {0.0f, 0.0f, 0.0f};
    NbNpcState* npc;
    MkObj* preload_object;
    Vec* object_position;
    Vec local_angles;
    Vec rope_offset;
    MKMATRIX rotation __attribute__((aligned(16)));
    float collision_offset_z;

    rope_offset = zero_vector;
    bgnd_preload_named_model(
        "ROPE", "slave_blood_burst", 0, 0.0f, &zero_vector);
    bgnd_set_active_sobj_in_obj(rope_model_index, 0);
    bgnd_unhide_preload_obj(rope_model_index);
    bgnd_unhide_active_sobj();
    bgnd_set_active_sobj_pos(anchor_x, anchor_y, anchor_z);
    bgnd_preload_obj_attach_rope(rope_model_index);

    bgnd_create_named_npc_in_slot(npc_id, model_slot, model_id, 0);
    bgnd_add_brains_to_npc(npc_id, p_npc_on_pendulum_rope);
    preload_object = bgnd_fetch_obj(npc_id);
    preload_object->light_flags = 4;
    bgnd_attach_rope_to_bgnd_obj(rope_model_index, npc_id, 8);
    bgnd_rope_adjust_length(rope_model_index, 1, rope_length);

    npc = bgnd_fetch_npc(npc_id);
    npc->anchor.x = anchor_x;
    npc->anchor.y = anchor_y;
    npc->anchor.z = anchor_z;
    npc->rope_length = rope_length;
    npc->acceleration_divisor = acceleration_divisor;
    npc->field_8C = field_8C;
    npc->acceleration_scale = acceleration_scale;
    npc->active = 0;
    npc->swing_angle = 0.0f;
    npc->field_44 = 0.0f;
    npc->last_hit_id[1] = 0.0f;
    npc->last_hit_id[0] = 0.0f;
    npc->phase = 0.17444445f * (float)npc_id;
    npc->swing_ticks = 3000;

    npc->object->ang_vel.z = 0.0f;
    npc->object->ang_vel.y = 0.0f;
    npc->object->ang_vel.x = 0.0f;
    npc->object->ang.x = object_angle_x;
    npc->object->ang.y = object_angle_y;
    npc->object->ang.z = object_angle_z;

    object_position = &npc->object->pos;
    local_angles.x = local_angle_x;
    local_angles.y = local_angle_y;
    local_angles.z = local_angle_z;
    rope_offset.x = rope_length;
    XYZ_angles_to_MKMATRIX(&local_angles, &rotation);
    v3_x_mat(object_position, &rope_offset, &rotation);
    object_position->x += npc->anchor.x;
    object_position->y += npc->anchor.y;
    collision_offset_z = object_position->z;
    object_position->z += npc->anchor.z;

    bgnd_npc_add_collision_shape(
        npc_id, npc_id + 0x12C, 1, 0.3f, 2.0f, -1.2f,
        collision_offset_z);
}

/* Soft ceiling: 92.90% -- normalized-vector FPR scheduling only. */
void rd_set_impact_vector(float scale) {
    Vec impact = {0.0f, 0.0f, 0.0f};
    float squared_length;
    float inverse_length;

    impact.x =
        g_game_info.player_objects[1]->pos.x -
        g_game_info.player_objects[0]->pos.x;
    impact.z =
        g_game_info.player_objects[1]->pos.z -
        g_game_info.player_objects[0]->pos.z;

    squared_length = impact.x * impact.x + impact.z * impact.z;
    inverse_length = nb_fast_inverse_sqrt(squared_length);
    impact.x *= inverse_length;
    impact.z *= inverse_length;

    g_game_info.impact_vector.y = impact.y;
    g_game_info.impact_vector.x = impact.x;
    g_game_info.impact_vector.z = impact.z;
    g_game_info.impact_vector.x = impact.x * scale;
    g_game_info.impact_vector.y = impact.y * scale;
    g_game_info.impact_vector.z = impact.z * scale;
}

static void nb_npc_slave_hit_by_plyr(int npc_id);
/* Soft ceiling: 91.44% -- inverse-sqrt FPR allocation and branch coloring. */
int nb_npc_hurt_player(
    NbNpcHitState* hit, unsigned int player_index, float impact);

void nb_npc_slave_plyr_process_collision(int npc_id) {
    NbNpcState* npc = bgnd_fetch_npc(npc_id);
    MkObj* object = npc->object;
    Vec facing;
    Vec side;
    float player_index;
    float speed;
    float separation;
    float impact_scale;
    float alignment;
    float old_x;
    float old_z;
    int attack_flags;
    int play_impact_sound;

    spad_set_vector(0, 0x1A);
    player_index = spad_get_pos(0, 0);
    spad_set_vector(0, 0x15);
    spad_set_vector_setting(
        1, object->pos.x, object->pos.y, object->pos.z);
    spad_sub_vectors(0, 1, 0);
    if (spad_get_pos(0, 1) > 1.8f) {
        return;
    }

    npc->swing_ticks = refresh_rate() * -15;
    bgnd_collision_if_disable_col(5, npc_id + 0x12C);
    spad_set_vector(0, 0x1D);
    if (((int)spad_get_pos(0, 0) & 1) == 0) {
        nb_npc_slave_hit_by_plyr(npc_id);
        return;
    }

    speed = nb_sqrt(
        npc->momentum.x * npc->momentum.x +
        npc->momentum.y * npc->momentum.y +
        npc->momentum.z * npc->momentum.z);
    spad_set_vector(0, 0x1C);
    attack_flags = (int)spad_get_pos(0, 0);

    if (speed < 0.03f) {
        spad_set_vector(0, 0x15);
        spad_set_vector_setting(
            1, object->pos.x, object->pos.y, object->pos.z);
        spad_sub_vectors(2, 1, 0);
        separation = spad_xz_length_vector(2);
        spad_norm_vector(2);
        if (separation < 0.35f) {
            spad_scale_vector(3, 2, -(0.45f - separation));
            spad_sub_vectors(1, 1, 3);
            object->pos.x = spad_get_pos(1, 0);
            object->pos.z = spad_get_pos(1, 2);
        }
        if (npc->swing_angle != 0.0f || randu0(100) < 80) {
            npc->swing_angle = 0.035f + frand(0.03f);
        }
        if (attack_flags != 0) {
            spad_scale_vector(0, 2, 0.065f);
        } else {
            spad_scale_vector(0, 2, 0.02f);
        }
        facing.x = spad_get_pos(0, 0);
        facing.y = 0.0f;
        facing.z = spad_get_pos(0, 2);
        rotate_xz(&facing, &facing, 0.5235988f);
        npc->momentum = facing;
        bgnd_collision_if_enable_col(5, npc_id + 0x12C);
        return;
    }

    spad_set_vector(0, 0x15);
    spad_set_vector_setting(
        1, object->pos.x, object->pos.y, object->pos.z);
    spad_sub_vectors(0, 0, 1);
    spad_set_vector_y(0, 0.0f);
    spad_norm_vector(0);
    spad_set_vector_setting(
        1, npc->momentum.x, 0.0f, npc->momentum.z);
    spad_norm_vector(1);
    if (spad_xz_dot_xz(0, 1) < -0.1f) {
        spad_set_vector(0, 0x15);
        spad_set_vector_setting(
            1, object->pos.x, object->pos.y, object->pos.z);
        spad_sub_vectors(2, 1, 0);
        if (spad_xz_length_vector(2) < 0.35f) {
            npc->momentum.x *= 1.1f;
            npc->momentum.z *= 1.1f;
        }
        bgnd_collision_if_enable_col(5, npc_id + 0x12C);
        return;
    }

    if (attack_flags == 0 || (attack_flags & 0x800) != 0) {
        impact_scale = 0.45f;
    } else if ((attack_flags & 0x400) != 0) {
        impact_scale = 0.05f;
    } else {
        impact_scale = 0.7f;
    }
    play_impact_sound = 1;
    if ((attack_flags & 0x2000) != 0 || attack_flags == 0) {
        int index = (int)player_index;

        if (is_pX_airborn(index) == 0 && speed > 0.11f) {
            impact_scale = 0.2f;
            play_impact_sound = 0;
            if (nb_npc_hurt_player(
                    (NbNpcHitState*)npc, (unsigned int)index, speed) == 1) {
                impact_scale = -0.05f;
            }
        }
    }
    if (play_impact_sound != 0 &&
        last_slave_hit_sound_time < exec_tick_ctr && speed > 0.06f) {
        last_slave_hit_sound_time = exec_tick_ctr + 30;
        random_hit(1);
    }

    spad_set_vector(0, 0x1E);
    uv_from_angle_y(&facing, spad_get_pos(0, 1));
    side.x = facing.z;
    side.y = 0.0f;
    side.z = -facing.x;
    spad_set_vector(0, 0x15);
    spad_set_vector_setting(
        1, object->pos.x, object->pos.y, object->pos.z);
    spad_sub_vectors(0, 0, 1);
    spad_set_vector_setting(1, facing.x, facing.y, facing.z);
    alignment = spad_xz_dot_xz(0, 1);
    if (alignment > -0.4f && alignment < 0.4f) {
        if (side.x * object->pos.x + side.z * object->pos.z < 0.0f) {
            side.x = -side.x;
            side.z = -side.z;
        }
        {
            Vec swap = facing;
            facing = side;
            side = swap;
        }
    } else if (alignment < 0.0f) {
        facing.x = -facing.x;
        facing.z = -facing.z;
    }

    old_x = npc->momentum.x;
    old_z = npc->momentum.z;
    npc->momentum.x =
        -impact_scale * (old_x * facing.x + old_z * facing.z);
    npc->momentum.z = old_x * side.x + old_z * side.z;
    old_x = npc->momentum.x;
    old_z = npc->momentum.z;
    npc->momentum.x =
        old_x * facing.x + old_z * side.x;
    npc->momentum.z =
        old_x * facing.z + old_z * side.z;
    bgnd_collision_if_enable_col(5, npc_id + 0x12C);
}

static void nb_npc_slave_hit_by_plyr(int npc_id) {
    NbNpcState* npc;
    Vec delta;
    float player_side;
    float previous_hit;
    float force;
    float inverse_length;
    float inverse_mass;
    float target_x;
    float target_y;
    float target_z;
    float old_x;
    float old_y;
    float old_z;
    float new_x;
    float new_y;
    float new_z;
    int collision_id;
    int hit_id;
    int player_index;
    int power;

    collision_id = npc_id + 0x12C;
    npc = bgnd_fetch_npc(npc_id);
    bgnd_collision_if_disable_col(5, collision_id);

    spad_set_vector(0, 0x1A);
    player_side = spad_get_pos(0, 0);
    previous_hit = npc->last_hit_id[0];
    if (player_side == 1.0f) {
        previous_hit = npc->last_hit_id[1];
    }

    spad_set_vector(0, 0x1B);
    hit_id = (int)spad_get_pos(0, 0);
    if ((float)hit_id != previous_hit) {
        if (player_side == 0.0f) {
            npc->last_hit_id[0] = (float)hit_id;
        } else {
            npc->last_hit_id[1] = (float)hit_id;
        }

        spad_set_vector(0, 0x15);
        spad_set_vector(1, 0x14);
        spad_sub_vectors(0, 1, 0);
        delta.x = spad_get_pos(0, 0);
        delta.y = spad_get_pos(0, 1);
        delta.z = spad_get_pos(0, 2);
        old_x = npc->momentum.x;
        old_y = npc->momentum.y;
        old_z = npc->momentum.z;

        player_index = (int)player_side;
        power = reaction_fetch_current_power_level(player_index);
        if (power > 3) {
            force = 0.095f + frand(0.08f);
        } else if (power > 2) {
            force = 0.07f + frand(0.08f);
        } else if (power > 1) {
            force = 0.05f + frand(0.08f);
        } else {
            force = 0.05f + frand(0.05f);
        }

        inverse_length = nb_fast_inverse_sqrt(
            delta.x * delta.x + delta.z * delta.z);
        target_x = delta.x * inverse_length * force;
        target_y = 0.0f;
        target_z = delta.z * inverse_length * force;
        inverse_mass = 1.0f / (npc->acceleration_divisor + 1.0f);
        new_x =
            (old_x * npc->acceleration_divisor + target_x +
             (target_x - old_x) * 0.9f) *
            inverse_mass;
        new_y =
            (old_y * npc->acceleration_divisor + target_y +
             (target_y - old_y) * 0.9f) *
            inverse_mass;
        new_z =
            (old_z * npc->acceleration_divisor + target_z +
             (target_z - old_z) * 0.9f) *
            inverse_mass;

        if ((reaction_fetch_current_flags(player_index) & 0x80) != 0) {
            new_x *= 0.205f;
            new_y = 0.2f;
            new_z *= 0.205f;
            npc->active |= 1;
            npc->swing_angle = 0.02f + frand(0.02f);
        } else if (npc->swing_angle != 0.0f || randu0(100) < 80) {
            npc->swing_angle = 0.08f + frand(0.09f);
            if (randu0(100) < 50) {
                npc->swing_angle *= -1.0f;
            }
        }

        snd_req(0x110);
        random_hit(1);
        npc->momentum.x = new_x;
        npc->momentum.y = new_y;
        npc->momentum.z = new_z;
    }

    bgnd_collision_if_enable_col(5, collision_id);
}

int nb_npc_hurt_player(
    NbNpcHitState* hit, unsigned int player_index, float impact) {
    MkObj* player_object;
    FighterMirror* fighter;
    NbFighterHurtView* fighter_view;
    CameraObj* camera;
    Vec facing;
    float hit_length_inverse;
    float facing_length_inverse;
    float alignment;

    player_object = g_game_info.plyr0.slot.mirror_a;
    fighter = g_game_info.plyr0.slot.fighter;
    if (player_index == 1) {
        player_object = g_game_info.plyr1.slot.mirror_a;
        fighter = g_game_info.plyr1.slot.fighter;
    }
    fighter_view = (NbFighterHurtView*)fighter;

    random_hit(0xD);
    uv_from_angle_y(&facing, player_object->ang.y);
    hit_length_inverse = nb_fast_inverse_sqrt(
        hit->direction_x * hit->direction_x +
        hit->direction_z * hit->direction_z);
    facing_length_inverse = nb_fast_inverse_sqrt(
        facing.x * facing.x + facing.z * facing.z);
    facing.x *= facing_length_inverse;
    facing.z *= facing_length_inverse;
    alignment =
        hit->direction_x * hit_length_inverse * facing.x +
        hit->direction_z * hit_length_inverse * facing.z;

    if (impact > 0.115f && alignment > 0.7f && alignment < 1.3f) {
        xfer_player_proc_to_script_manual_messaging(
            fighter, player_object, 0xA);
        return 1;
    }
    if (impact > 0.125f && alignment > -1.15f && alignment < -0.85f) {
        xfer_player_proc_to_script_manual_messaging(
            fighter, player_object, 9);
        return 1;
    }
    if (alignment > -0.45f && alignment < 0.45f) {
        MkObj* current_object;
        int use_left_reaction;
        float camera_to_npc_x;
        float camera_to_npc_z;
        float camera_to_player_x;
        float camera_to_player_z;

        current_object = fighter_view->object_slot->object;
        plyr_obj = current_object;
        his_obj = fighter_view->opponent_object;
        plyr_anim_pdata = &fighter_view->anim_pdata;

        camera = camera_item.node;
        if (camera != 0 && camera->instance != camera_item.instance) {
            camera = 0;
        }

        camera_to_npc_x = camera->pos_x - hit->object->pos.x;
        camera_to_npc_z = camera->pos_z - hit->object->pos.z;
        camera_to_player_x = camera->pos_x - current_object->pos.x;
        camera_to_player_z = camera->pos_z - current_object->pos.z;
        use_left_reaction = 0;
        if (camera_to_npc_x * camera_to_npc_x +
                camera_to_npc_z * camera_to_npc_z >
            camera_to_player_x * camera_to_player_x +
                camera_to_player_z * camera_to_player_z) {
            if (is_my_chest_to_screen() == 0) {
                use_left_reaction = 1;
            }
        } else if (is_my_chest_to_screen() != 0) {
            use_left_reaction = 1;
        }

        plyr_obj = 0;
        his_obj = 0;
        plyr_anim_pdata = 0;
        if (use_left_reaction != 0) {
            xfer_player_proc_to_script_manual_messaging(
                fighter, player_object, 0xB);
        } else {
            xfer_player_proc_to_script_manual_messaging(
                fighter, player_object, 0xC);
        }
        return 1;
    }
    if (alignment > -1.55f && alignment < -0.55f) {
        get_player_proc(player_object);
        xfer_player_proc(r_chest2_stumble);
    }
    return 0;
}

float p_npc_on_pendulum_rope(void) {
    static const Vec world_up = {0.0f, 1.0f, 0.0f};
    NbNpcState* npc = ((NbNpcProcPdata*)apdata)->npc;
    MkObj* object = npc->object;
    Vec displacement;
    Vec normal;
    Vec acceleration;
    Vec velocity_direction;
    Vec tangent;
    Vec angle_vector;
    float distance;
    float inverse_length;
    float speed;
    float angle;
    float response;

    npc->momentum.x = 0.0f;
    npc->momentum.y = 0.0f;
    npc->momentum.z = 0.0f;

    for (;;) {
        npc->swing_ticks++;
        npc->phase += 0.018f;

        displacement.x = object->pos.x - npc->anchor.x;
        displacement.y = object->pos.y - npc->anchor.y;
        displacement.z = object->pos.z - npc->anchor.z;
        distance = nb_sqrt(
            displacement.x * displacement.x +
            displacement.y * displacement.y +
            displacement.z * displacement.z);

        if ((npc->active & 1) != 0) {
            if (npc->momentum.y < 0.0f &&
                distance > npc->rope_length + 0.002f) {
                if (npc->momentum.y < -0.1f) {
                    npc->momentum.y = 0.08f;
                    npc->momentum.x *= 0.2f;
                    npc->momentum.z *= 0.3f;
                    bgnd_launch_fx_at_bid_of_mkobj(
                        "slave_blood_burst", object, 8);
                    bgnd_launch_fx_at_bid_of_mkobj(
                        "slave_blood_spurt", object, 8);
                    snd_req(0x10F);
                    snd_req(0xD5D);
                } else {
                    npc->momentum.y = 0.0f;
                    npc->active &= ~1;
                }
            }
            npc->momentum.y -= npc->acceleration_scale;
        } else {
            if (distance > npc->rope_length + 0.002f ||
                distance < npc->rope_length - 0.002f) {
                inverse_length = nb_fast_inverse_sqrt(
                    displacement.x * displacement.x +
                    displacement.y * displacement.y +
                    displacement.z * displacement.z);
                displacement.x *= inverse_length * npc->rope_length;
                displacement.y *= inverse_length * npc->rope_length;
                displacement.z *= inverse_length * npc->rope_length;
                object->pos.x = npc->anchor.x + displacement.x;
                object->pos.y = npc->anchor.y + displacement.y;
                object->pos.z = npc->anchor.z + displacement.z;
            }

            inverse_length = nb_fast_inverse_sqrt(
                displacement.x * displacement.x +
                displacement.y * displacement.y +
                displacement.z * displacement.z);
            normal.x = -displacement.x * inverse_length;
            normal.y = -displacement.y * inverse_length;
            normal.z = -displacement.z * inverse_length;
            nb_get_desired_acceleration(
                (NbPendulumState*)npc, &acceleration, &normal);

            acceleration.x += npc->momentum.x * npc->field_8C;
            acceleration.y += npc->momentum.y * npc->field_8C;
            acceleration.z += npc->momentum.z * npc->field_8C;
            npc->momentum.x += acceleration.x;
            npc->momentum.y += acceleration.y;
            npc->momentum.z += acceleration.z;

            if (npc->momentum.x * normal.x +
                    npc->momentum.y * normal.y +
                    npc->momentum.z * normal.z <
                0.0f) {
                speed = nb_sqrt(
                    npc->momentum.x * npc->momentum.x +
                    npc->momentum.y * npc->momentum.y +
                    npc->momentum.z * npc->momentum.z);
                inverse_length = nb_fast_inverse_sqrt(
                    npc->momentum.x * npc->momentum.x +
                    npc->momentum.y * npc->momentum.y +
                    npc->momentum.z * npc->momentum.z);
                velocity_direction.x = npc->momentum.x * inverse_length;
                velocity_direction.y = npc->momentum.y * inverse_length;
                velocity_direction.z = npc->momentum.z * inverse_length;

                tangent.x =
                    (velocity_direction.x * normal.y -
                     velocity_direction.y * normal.x) *
                        normal.y -
                    (velocity_direction.y * normal.z -
                     velocity_direction.z * normal.y) *
                        normal.z;
                tangent.y =
                    (velocity_direction.z * normal.x -
                     velocity_direction.x * normal.z) *
                        normal.z -
                    (velocity_direction.x * normal.y -
                     velocity_direction.y * normal.x) *
                        normal.x;
                tangent.z =
                    (velocity_direction.y * normal.z -
                     velocity_direction.z * normal.y) *
                        normal.x -
                    (velocity_direction.z * normal.x -
                     velocity_direction.x * normal.z) *
                        normal.y;
                inverse_length = nb_fast_inverse_sqrt(
                    tangent.x * tangent.x + tangent.y * tangent.y +
                    tangent.z * tangent.z);
                tangent.x *= inverse_length;
                tangent.y *= inverse_length;
                tangent.z *= inverse_length;
                if (tangent.x * npc->momentum.x +
                        tangent.y * npc->momentum.y +
                        tangent.z * npc->momentum.z <
                    0.0f) {
                    speed = -speed;
                }
                npc->momentum.x = tangent.x * speed;
                npc->momentum.y = tangent.y * speed;
                npc->momentum.z = tangent.z * speed;
            }
        }

        object->pos.x += npc->momentum.x;
        object->pos.y += npc->momentum.y;
        object->pos.z += npc->momentum.z;
        bgnd_pebble_set_current_pebble(8, npc->npc_id - 1);
        bgnd_pebble_set_current_info(9, object, object->pos.x);
        bgnd_pebble_set_current_info(0xB, object, object->pos.z);

        if (npc->swing_angle != 0.0f) {
            npc->swing_angle *= 0.995f;
            if (npc->swing_angle < 0.008f &&
                npc->swing_angle > -0.008f) {
                npc->swing_angle = 0.0f;
            }
            object->ang.y += npc->swing_angle;
        }

        inverse_length = nb_fast_inverse_sqrt(
            displacement.x * displacement.x +
            displacement.z * displacement.z);
        velocity_direction.x = displacement.x * inverse_length;
        velocity_direction.y = 0.0f;
        velocity_direction.z = displacement.z * inverse_length;
        angle_vector.x =
            velocity_direction.z * world_up.x -
            velocity_direction.x * world_up.z;
        angle_vector.y =
            velocity_direction.y * world_up.z -
            velocity_direction.z * world_up.y;
        angle_vector.z =
            velocity_direction.x * world_up.y -
            velocity_direction.y * world_up.x;
        inverse_length = nb_fast_inverse_sqrt(
            angle_vector.x * angle_vector.x +
            angle_vector.y * angle_vector.y +
            angle_vector.z * angle_vector.z);
        angle_vector.x *= inverse_length;
        angle_vector.y *= inverse_length;
        angle_vector.z *= inverse_length;
        angle = gxMathArcCos(-displacement.y / npc->rope_length) * 1.25f;
        if (velocity_direction.x * displacement.x +
                velocity_direction.y * displacement.y +
                velocity_direction.z * displacement.z <
            0.0f) {
            angle = -angle;
        }
        angle_vector.x *= angle;
        angle_vector.y *= angle;
        angle_vector.z *= angle;
        rotate_xz(&angle_vector, &angle_vector, -object->ang.y);

        if (angle_vector.y < 0.0f && object->ang.x > 3.1415927f) {
            angle_vector.y =
                0.000005992112f *
                (float)((int)(166886.1f * angle_vector.y) & 0xFFFFF);
        }
        if (angle_vector.y > 0.0f && angle_vector.y < 3.1415927f &&
            object->ang.x > 3.1415927f) {
            object->ang.x -= 6.2831855f;
        }
        if (angle_vector.z < 0.0f && object->ang.z > 3.1415927f) {
            angle_vector.z =
                0.000005992112f *
                (float)((int)(166886.1f * angle_vector.z) & 0xFFFFF);
        }
        if (angle_vector.z > 0.0f && angle_vector.z < 3.1415927f &&
            object->ang.z > 3.1415927f) {
            object->ang.z -= 6.2831855f;
        }

        response = (npc->active & 1) != 0 ? 60.0f : 4.0f;
        object->ang.x -= (object->ang.x - angle_vector.y) / response;
        object->ang.z -= (object->ang.z - angle_vector.z) / response;
        _mkproc_sleep_ticks = 1.0f;
        ((BgndJtbProcVtable*)aproc->vtbl)->sleep();
    }
}
