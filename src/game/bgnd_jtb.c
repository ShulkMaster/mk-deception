#include "game/game_info.h"
#include "math/gxMath.h"
#include "math/gxVect.h"
#include "math/mk_math.h"
#include "platform/gcutils.h"
#include "runtime/anim_pdata.h"
#include "runtime/cam.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"

typedef struct BgndJtbObjView {
    char pad00[9];
    unsigned char flags_09;
    char pad0A[0x66];
    float ground_y;
    char pad74[0x2C];
    Vec pos;
    char padAC[4];
    Vec velocity;
} BgndJtbObjView;

typedef struct BgndJtbProcVtable {
    void* reserved[6];
    void (*sleep)(void);
} BgndJtbProcVtable;

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
float sqrtf(float value);

void lower_mines_ani_to_point(
    void* script, int landing_sound, Vec* target, unsigned int frame_offset,
    float start_frame, float animation_step, float end_frame,
    float vertical_velocity, float gravity, float transition) {
    BgndJtbObjView* object;
    float root;
    float frames;
    float root_a;
    float root_b;

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
    object = (BgndJtbObjView*)plyr_obj;
    object->flags_09 &= ~0x80;

    root = vertical_velocity * vertical_velocity -
        (2.0f * gravity) *
            (object->pos.y - 0.19f - object->ground_y);
    if (root < 0.001f) {
        root = 0.001f;
    }
    root = sqrtf(root);

    root_a = (root - vertical_velocity) / gravity;
    root_b = (-root - vertical_velocity) / gravity;
    if (root_a < 0.0f ||
        (root_b > 0.0f && root_b < root_a)) {
        root_a = root_b;
    }

    frames = root_a - (float)frame_offset;
    if (frames < 1.0f) {
        frames = 1.0f;
    }

    plyr_anim_pdata->step = (end_frame - start_frame) / frames;
    object->velocity.x = (target->x - object->pos.x) / frames;
    object->velocity.z = (target->z - object->pos.z) / frames;
    ani_to_frame_x(end_frame);

    object->flags_09 |= 0x80;
    update_bone_hierarchy(as_mkhdr((MkHdr*)plyr_obj));
    ground_me(as_mkhdr((MkHdr*)plyr_obj));
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
    char pad08[4];
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
int reaction_fetch_current_power_level(int player_index);
int reaction_fetch_current_flags(int player_index);
float frand(float range);
unsigned short randu0(unsigned int maximum);
unsigned long snd_req(int sound_id);

extern MkObj* his_obj;
extern void* r_chest2_stumble;

typedef union NbFloatBits {
    float f;
    unsigned int u;
} NbFloatBits;

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

void nb_place_slave_in_bgnd(
    int npc_id, int rope_model_index, int model_slot, int model_id,
    float anchor_x, float anchor_y, float anchor_z, float rope_length,
    float local_angle_x, float local_angle_y, float local_angle_z,
    float object_angle_x, float object_angle_y, float object_angle_z,
    float acceleration_divisor, float acceleration_scale, float field_8C) {
    static const Vec zero_vector = {0.0f, 0.0f, 0.0f};
    NbNpcState* npc;
    MkObj* object;
    Vec local_angles;
    Vec rope_offset;
    MKMATRIX rotation __attribute__((aligned(16)));
    float collision_offset_z;

    bgnd_preload_named_model(
        "ROPE", "slave_blood_burst", 0, 0.0f, &zero_vector);
    bgnd_set_active_sobj_in_obj(rope_model_index, 0);
    bgnd_unhide_preload_obj(rope_model_index);
    bgnd_unhide_active_sobj();
    bgnd_set_active_sobj_pos(anchor_x, anchor_y, anchor_z);
    bgnd_preload_obj_attach_rope(rope_model_index);

    bgnd_create_named_npc_in_slot(npc_id, model_slot, model_id, 0);
    bgnd_add_brains_to_npc(npc_id, p_npc_on_pendulum_rope);
    bgnd_fetch_obj(npc_id)->light_flags = 4;
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
    npc->last_hit_id[0] = 0.0f;
    npc->last_hit_id[1] = 0.0f;
    npc->field_44 = 0.0f;
    npc->phase = 0.17444445f * (float)npc_id;
    npc->swing_ticks = 3000;

    object = npc->object;
    object->ang_vel.x = 0.0f;
    object->ang_vel.y = 0.0f;
    object->ang_vel.z = 0.0f;
    object->ang.x = object_angle_x;
    object->ang.y = object_angle_y;
    object->ang.z = object_angle_z;

    local_angles.x = local_angle_x;
    local_angles.y = local_angle_y;
    local_angles.z = local_angle_z;
    rope_offset = zero_vector;
    rope_offset.x = rope_length;
    XYZ_angles_to_MKMATRIX(&local_angles, &rotation);
    v3_x_mat(&object->pos, &rope_offset, &rotation);
    object->pos.x += npc->anchor.x;
    object->pos.y += npc->anchor.y;
    collision_offset_z = object->pos.z;
    object->pos.z += npc->anchor.z;

    bgnd_npc_add_collision_shape(
        npc_id, npc_id + 0x12C, 1, 0.3f, 2.0f, -1.2f,
        collision_offset_z);
}

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

    g_game_info.impact_vector = impact;
    g_game_info.impact_vector.x *= scale;
    g_game_info.impact_vector.y *= scale;
    g_game_info.impact_vector.z *= scale;
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
    FighterMirror* fighter;
    NbFighterHurtView* fighter_view;
    MkObj* player_object;
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

        camera = camera_item.node;
        if (camera != 0 && camera->instance != camera_item.instance) {
            camera = 0;
        }

        current_object = fighter_view->object_slot->object;
        plyr_obj = current_object;
        his_obj = fighter_view->opponent_object;
        plyr_anim_pdata = &fighter_view->anim_pdata;

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
        xfer_player_proc(&r_chest2_stumble);
    }
    return 0;
}
