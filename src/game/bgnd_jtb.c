#include "game/game_info.h"
#include "runtime/anim_transition.h"
#include "game/plyr_globals.h"
#include "runtime/sound.h"
#include "math/gxMath.h"
#include "math/gxVect.h"
#include "math/mk_math.h"
#include "platform/gcutils.h"
#include "platform/main.h"
#include "runtime/anim_pdata.h"
#include "runtime/plyr_anim_pdata.h"
#include "game/plyr.h"
#include "runtime/cam.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"


void ani_to_frame_x(float frame);
void launch_me_up(float velocity, float gravity);
void land_chores(int sound, int flags, float velocity, float gravity);

struct NbNpcState {
    MkHdr hdr;
    int npc_id;
    MkObj* object;
    char pad10[0x14];
    Vec anchor;
    Vec momentum;
    float last_hit_id[2];
    float field_44;
    char pad48[0x3C];
    float rope_length;
    float acceleration_divisor;
    float field_8C;
    float acceleration_scale;
    float swing_angle;
    float phase;
    char pad9C[8];
    int active;
    int swing_ticks;
};

struct NbNpcProcPdata {
    MkHdr hdr;
    struct NbNpcState* npc;
};

struct NbFighterObjectSlot {
    char pad00[0x5C];
    MkObj* object;
};

struct NbFighterHurtView {
    char pad00[0x14];
    MkObj* opponent_object;
    struct NbFighterObjectSlot* object_slot;
    char pad1C[0x5A4];
    AnimPdata anim_pdata;
};

static float p_npc_on_pendulum_rope(void);
static void nb_get_desired_acceleration(
    struct NbNpcState* state, Vec* acceleration, const Vec* surface_normal);
int bgnd_preload_named_model(const char* model_name, int slot);
void bgnd_set_active_sobj_in_obj(int model_index, int object_id);
void bgnd_unhide_preload_obj(int model_index);
void bgnd_unhide_active_sobj(void);
void bgnd_set_active_sobj_pos(float x, float y, float z);
void bgnd_preload_obj_attach_rope(int model_index);
void bgnd_create_named_npc_in_slot(
    int npc_id, const char* model_name, int model_id, int flags);
void bgnd_add_brains_to_npc(int npc_id, MkProcEntryFn brains);
MkObj* bgnd_fetch_obj(int object_id);
struct NbNpcState* bgnd_fetch_npc(int npc_id);
void bgnd_attach_rope_to_bgnd_obj(
    int rope_model_index, int target_model_index, int object_id);
void bgnd_rope_adjust_length(
    int model_index, int preserve_shape, float length);
void bgnd_npc_add_collision_shape(
    int npc_id, int shape_id, int shape_type, float radius, float height,
    float offset_y, float offset_z);
unsigned long random_hit(int group);
void xfer_player_proc_to_script_manual_messaging(
    FighterMirror* fighter, MkObj* object, int message);
MkProc* get_player_proc(MkObj* object);
int is_my_chest_to_screen(void);
void bgnd_collision_if_disable_col(int list_id, unsigned int collision_id);
void bgnd_collision_if_enable_col(int list_id, unsigned int collision_id);
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
void bgnd_launch_fx_at_bid_of_mkobj(
    const char* effect_name, MkObj* object, int bone);
int bgnd_pebble_set_current_pebble(int pebble, int index);
int bgnd_pebble_set_current_info(int info, void* object, float value);

float r_chest2_stumble(void);

static void nb_npc_slave_hit_by_plyr(int npc_id);
static int nb_npc_hurt_player(
    struct NbNpcState* hit, unsigned int player_index, float impact);


void lower_mines_ani_to_point(
    void* script, float start_frame, float animation_step, float end_frame,
    int landing_sound, float vertical_velocity, float gravity,
    float transition, Vec* target, unsigned int frame_offset) {
    MkHdr* object_header;
    float root;
    float radicand;
    float frames;
    float root_b;
    float inverse_frames;
    float delta_x;
    float delta_z;
    float height_term;

    plyr_anim_pdata->flags |= 0x40;
    transition_to_anim_script(transition, plyr_anim_pdata, script, 0x43);

    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();

    if (start_frame != 0.0f) {
        plyr_anim_pdata->step = animation_step;
        ani_to_frame_x(start_frame);
        plyr_anim_pdata->step = 1.0f;
    }

    launch_me_up(vertical_velocity, gravity);
    plyr_obj->flags_09_bits.launched = 0;

    radicand = vertical_velocity * vertical_velocity;
    height_term = (2.0f * gravity) *
        ((plyr_obj->pos.value.y - 0.19f) - plyr_obj->ground_colls_y);
    radicand -= height_term;
    root = 0.001f;
    if (radicand >= root) {
        root = radicand;
    }
    root = gxMathFastSqrt(root);

    radicand = (root - vertical_velocity) / gravity;
    root_b = (-root - vertical_velocity) / gravity;
    if (radicand < 0.0f ||
        (root_b > 0.0f && root_b < radicand)) {
        radicand = root_b;
    }

    frames = 1.0f;
    radicand = radicand - (float)frame_offset;
    if (radicand >= frames) {
        frames = radicand;
    }

    plyr_anim_pdata->step = (end_frame - start_frame) / frames;
    inverse_frames = 1.0f / frames;
    delta_x = target->x;
    delta_x -= plyr_obj->pos.value.x;
    delta_z = target->z - plyr_obj->pos.value.z;
    delta_x *= inverse_frames;
    plyr_obj->pos_vel.x = delta_x;
    plyr_obj->pos_vel.z = delta_z * inverse_frames;
    ani_to_frame_x(end_frame);

    plyr_obj->flags_09_bits.launched = 1;
    if (plyr_obj != 0) {
        object_header = as_mkhdr(&plyr_obj->hdr);
    } else {
        object_header = 0;
    }
    update_bone_hierarchy(object_header);
    if (plyr_obj != 0) {
        object_header = as_mkhdr(&plyr_obj->hdr);
    } else {
        object_header = 0;
    }
    ground_me(object_header);
    land_chores(landing_sound, 0, 0.0f, 0.0f);
}

static const Vec nb_collision_x_axis = {1.0f, 0.0f, 0.0f};
static const Vec nb_collision_z_axis = {0.0f, 0.0f, 1.0f};
static const Vec nb_impact_zero = {0.0f, 0.0f, 0.0f};
static const Vec nb_rope_preload_rotation = {1.0f, 0.0f, 0.0f};
static const Vec nb_world_up = {0.0f, 1.0f, 0.0f};
static const Vec nb_hit_zero = {0.0f, 0.0f, 0.0f};
static const Vec nb_collision_zero = {0.0f, 0.0f, 0.0f};

/* TODO: [breakthrough needed] 98.15044%; side-X priority and coordinate-helper
 * ownership remain; projection and basis accumulators preserve the wrong order. */
void nb_npc_slave_plyr_process_collision(unsigned int npc_id) {
    static unsigned int last_sound_time;
    struct NbNpcState* npc;
    Vec push_direction;
    Vec facing;
    Vec side = nb_collision_zero;
    Vec local_z;
    Vec local_x;
    float player_index;
    float speed;
    float separation;
    float impact_scale;
    float alignment;
    float momentum_x;
    float momentum_y;
    float momentum_z;
    int attack_flags;
    int play_impact_sound;
    float x_on_facing;
    float x_on_side;
    float z_on_facing;
    float z_on_side;

    npc = bgnd_fetch_npc(npc_id);
    spad_set_vector(0, 0x1A);
    player_index = spad_get_pos(0, 0);
    spad_set_vector(0, 0x15);
    spad_set_vector_setting(
        1, npc->object->pos.value.x, npc->object->pos.value.y, npc->object->pos.value.z);
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

    momentum_y = npc->momentum.y;
    momentum_x = npc->momentum.x;
    momentum_z = npc->momentum.z;
    speed = gxMathFastSqrt(
        momentum_z * momentum_z +
        (momentum_x * momentum_x + momentum_y * momentum_y));
    spad_set_vector(0, 0x1C);
    attack_flags = spad_get_pos(0, 0);

    if (speed < 0.03f) {
        spad_set_vector(0, 0x15);
        spad_set_vector_setting(
            1, npc->object->pos.value.x, npc->object->pos.value.y, npc->object->pos.value.z);
        spad_sub_vectors(2, 1, 0);
        separation = spad_xz_length_vector(2);
        spad_norm_vector(2);
        if (separation < 0.35f) {
            spad_scale_vector(3, 2, -1.0f * (0.45f - separation));
            spad_sub_vectors(1, 1, 3);
            npc->object->pos.value.x = spad_get_pos(1, 0);
            npc->object->pos.value.z = spad_get_pos(1, 2);
        }
        if (npc->swing_angle != 0.0f || (unsigned short)randu0(100) < 80) {
            npc->swing_angle = 0.035f + frand(0.03f);
        }
        if (attack_flags != 0) {
            spad_scale_vector(0, 2, 0.065f);
        } else {
            spad_scale_vector(0, 2, 0.02f);
        }
        push_direction.x = spad_get_pos(0, 0);
        push_direction.y = 0.0f;
        push_direction.z = spad_get_pos(0, 2);
        rotate_xz(&push_direction, &push_direction, 0.5235988f);
        npc->momentum.x = push_direction.x;
        npc->momentum.y = push_direction.y;
        npc->momentum.z = push_direction.z;
        bgnd_collision_if_enable_col(5, npc_id + 0x12C);
        return;
    }

    spad_set_vector(0, 0x15);
    spad_set_vector_setting(
        1, npc->object->pos.value.x, npc->object->pos.value.y, npc->object->pos.value.z);
    spad_sub_vectors(0, 0, 1);
    spad_set_vector_y(0, 0.0f);
    spad_norm_vector(0);
    spad_set_vector_setting(
        1, npc->momentum.x, 0.0f, npc->momentum.z);
    spad_norm_vector(1);
    if (spad_xz_dot_xz(0, 1) < -0.1f) {
        spad_set_vector(0, 0x15);
        spad_set_vector_setting(
            1, npc->object->pos.value.x, npc->object->pos.value.y, npc->object->pos.value.z);
        spad_sub_vectors(2, 1, 0);
        if (spad_xz_length_vector(2) < 0.35f) {
            npc->momentum.x = 1.1f * npc->momentum.x;
            npc->momentum.z = 1.1f * npc->momentum.z;
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
        int index = player_index;

        if (is_pX_airborn(index) == 0 && speed > 0.11f) {
            impact_scale = 0.2f;
            play_impact_sound = 0;
            if (nb_npc_hurt_player(
                    npc, index, speed) == 1) {
                impact_scale = -0.05f;
            }
        }
    }
    if (play_impact_sound == 1 &&
        last_sound_time < (unsigned int)exec_tick_ctr && speed > 0.06f) {
        last_sound_time = (unsigned int)exec_tick_ctr + 30;
        random_hit(1);
    }

    spad_set_vector(0, 0x1E);
    uv_from_angle_y(&facing, spad_get_pos(0, 1));
    side.x = facing.z;
    side.z = -facing.x;
    spad_set_vector(0, 0x15);
    spad_set_vector_setting(
        1, npc->object->pos.value.x, npc->object->pos.value.y, npc->object->pos.value.z);
    spad_sub_vectors(0, 0, 1);
    spad_set_vector_setting(1, facing.x, facing.y, facing.z);
    alignment = spad_xz_dot_xz(0, 1);
    if (alignment > -0.4f && alignment < 0.4f) {
        float swap_x;
        float swap_y;
        float swap_z;

        if (side.x * npc->object->pos.value.x +
                side.z * npc->object->pos.value.z <
            0.0f) {
            side.x = -1.0f * side.x;
            side.z = -1.0f * side.z;
        }
        swap_x = side.x;
        swap_y = side.y;
        swap_z = side.z;

        side.x = facing.x;
        side.y = facing.y;
        side.z = facing.z;
        facing.x = swap_x;
        facing.y = swap_y;
        facing.z = swap_z;
    } else if (alignment < 0.0f) {
        facing.x = -1.0f * facing.x;
        facing.z = -1.0f * facing.z;
    }

    local_x = nb_collision_x_axis;
    local_z = nb_collision_z_axis;
    momentum_x = npc->momentum.x;
    momentum_z = npc->momentum.z;
    x_on_facing = local_x.x * facing.x + local_x.z * facing.z;
    x_on_side = local_x.x * side.x + local_x.z * side.z;
    z_on_facing = local_z.x * facing.x + local_z.z * facing.z;
    z_on_side = local_z.x * side.x + local_z.z * side.z;
    npc->momentum.x =
        -1.0f * impact_scale * (momentum_x * facing.x + momentum_z * facing.z);
    npc->momentum.z = momentum_x * side.x + momentum_z * side.z;
    momentum_y = npc->momentum.y;
    momentum_x = npc->momentum.x;
    momentum_z = npc->momentum.z;
    npc->momentum.x = momentum_x * x_on_facing + momentum_z * x_on_side;
    npc->momentum.z = momentum_x * z_on_facing + momentum_z * z_on_side;
    npc->momentum.y = momentum_y;
    bgnd_collision_if_enable_col(5, npc_id + 0x12C);
}

/* TODO: [near miss] 99.32806%; weighted-sum schedule agrees; mass-add direction
 * and FPR homes remain; denominator/reciprocal accumulators rotate earlier values. */
static void nb_npc_slave_hit_by_plyr(int npc_id) {
    struct NbNpcState* npc;
    Vec target = nb_hit_zero;
    Vec delta;
    float player_side;
    float previous_hit;
    float force;
    float inverse_length;
    float inverse_mass;
    float old_x;
    float old_y;
    float old_z;
    Vec blended_momentum;
    float target_weight;
    float correction_weight;
    float weighted_sum;
    float weighted_target;
    float correction;
    int player_index;
    unsigned int collision_id;
    int hit_id;

    npc = bgnd_fetch_npc(npc_id);
    collision_id = npc_id + 0x12CU;
    bgnd_collision_if_disable_col(5, collision_id);

    spad_set_vector(0, 0x1A);
    player_side = spad_get_pos(0, 0);
    previous_hit = npc->last_hit_id[0];
    if (player_side == 1.0f) {
        previous_hit = npc->last_hit_id[1];
    }

    spad_set_vector(0, 0x1B);
    hit_id = spad_get_pos(0, 0);
    if ((float)hit_id != previous_hit) {
        int power;

        if (player_side == 0.0f) {
            npc->last_hit_id[0] = hit_id;
        } else {
            npc->last_hit_id[1] = hit_id;
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

        player_index = player_side;
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

        inverse_length = gxMathFastInvSqrt(
            delta.x * delta.x + delta.z * delta.z);
        target.x = delta.x * inverse_length;
        target.z = delta.z * inverse_length;
        target.x *= force;
        target.z *= force;
        inverse_mass = 1.0f / (npc->acceleration_divisor + 1.0f);
        target_weight = 1.0f;
        correction_weight = 0.9f;
        weighted_sum = old_x * npc->acceleration_divisor;
        weighted_target = target.x * target_weight;
        correction = target.x - old_x;
        weighted_sum += weighted_target;
        correction *= correction_weight;
        blended_momentum.x = weighted_sum + correction;
        weighted_sum = old_y * npc->acceleration_divisor;
        weighted_target = target.y * target_weight;
        correction = target.y - old_y;
        weighted_sum += weighted_target;
        correction *= correction_weight;
        blended_momentum.y = weighted_sum + correction;
        weighted_sum = old_z * npc->acceleration_divisor;
        weighted_target = target.z * target_weight;
        correction = target.z - old_z;
        weighted_sum += weighted_target;
        correction *= correction_weight;
        blended_momentum.z = weighted_sum + correction;
        blended_momentum.x *= inverse_mass;
        blended_momentum.y *= inverse_mass;
        blended_momentum.z *= inverse_mass;

        if ((reaction_fetch_current_flags(player_index) & 0x80) != 0) {
            blended_momentum.x = 0.205f * blended_momentum.x;
            blended_momentum.y = 0.2f;
            blended_momentum.z = 0.205f * blended_momentum.z;
            npc->active |= 1;
            npc->swing_angle = 0.02f + frand(0.02f);
        } else if (npc->swing_angle != 0.0f || (unsigned short)randu0(100) < 80) {
            npc->swing_angle = 0.08f + frand(0.09f);
            if ((unsigned short)randu0(100) < 50) {
                npc->swing_angle *= -1.0f;
            }
        }

        snd_req(0x110);
        random_hit(1);
        npc->momentum.x = blended_momentum.x;
        npc->momentum.y = blended_momentum.y;
        npc->momentum.z = blended_momentum.z;
    }

    bgnd_collision_if_enable_col(5, collision_id);
}

static inline int nb_npc_side_reaction_is_left(
    struct NbFighterHurtView* fighter_view, struct NbNpcState* hit) {
    CameraObj* camera;
    MkObj* current_object;
    MkObj* npc_object;
    int use_left_reaction;
    float camera_to_player_z;
    float camera_x;
    float camera_to_player_x;
    float camera_to_npc_z;
    float camera_to_npc_x;

    use_left_reaction = 0;
    current_object = fighter_view->object_slot->object;
    plyr_obj = current_object;
    his_obj = fighter_view->opponent_object;
    plyr_anim_pdata = &fighter_view->anim_pdata;
    npc_object = hit->object;
    camera = MK_HDR_LIVE(camera_item.node, camera_item.instance);

    camera_to_npc_z = camera->pos.z - npc_object->pos.value.z;
    camera_to_player_z = camera->pos.z - current_object->pos.value.z;
    camera_x = camera->pos.x;
    camera_to_npc_x = camera_x - npc_object->pos.value.x;
    camera_to_player_x = camera_x - current_object->pos.value.x;
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

    return use_left_reaction;
}

static int nb_npc_hurt_player(
    struct NbNpcState* hit, unsigned int player_index, float impact) {
    MkObj* player_object;
    FighterMirror* fighter;
    struct NbFighterHurtView* fighter_view;
    Vec facing;
    Vec direction;
    float hit_length_inverse;
    float facing_length_inverse;
    float alignment;

    player_object = g_game_info.plyr0.slot.mirror_a;
    fighter = g_game_info.plyr0.slot.fighter;
    if (player_index == 1) {
        player_object = g_game_info.plyr1.slot.mirror_a;
        fighter = g_game_info.plyr1.slot.fighter;
    }
    fighter_view = (struct NbFighterHurtView*)fighter;

    random_hit(0xD);
    uv_from_angle_y(&facing, player_object->ang.y);
    hit_length_inverse = gxMathFastInvSqrt(
        hit->momentum.x * hit->momentum.x +
        hit->momentum.z * hit->momentum.z);
    direction.x = hit->momentum.x * hit_length_inverse;
    direction.z = hit->momentum.z * hit_length_inverse;
    facing_length_inverse = gxMathFastInvSqrt(
        facing.x * facing.x + facing.z * facing.z);
    facing.x *= facing_length_inverse;
    facing.z *= facing_length_inverse;
    alignment = direction.x * facing.x + direction.z * facing.z;

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
        int use_left_reaction = nb_npc_side_reaction_is_left(fighter_view, hit);
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
        xfer_player_proc(get_player_proc(player_object), r_chest2_stumble);
    }
    return 0;
}

static inline void nb_apply_tangent_momentum(
    struct NbNpcState* npc, const Vec* tangent, float speed_squared) {
    float speed;

    speed = gxMathFastSqrt(speed_squared);
    if (tangent->x * npc->momentum.x +
            tangent->y * npc->momentum.y +
            tangent->z * npc->momentum.z <
        0.0f) {
        speed *= -1.0f;
    }
    npc->momentum.x = tangent->x * speed;
    npc->momentum.y = tangent->y * speed;
    npc->momentum.z = tangent->z * speed;
}

static inline void nb_get_rope_angle(
    struct NbNpcState* npc, const Vec* displacement, const Vec* world_up,
    Vec* angle_vector) {
    Vec horizontal_direction;
    float inverse_length;
    float angle;

    horizontal_direction.x = displacement->x;
    horizontal_direction.y = 0.0f;
    horizontal_direction.z = displacement->z;
    inverse_length = gxMathFastInvSqrt(
        horizontal_direction.x * horizontal_direction.x +
        horizontal_direction.y * horizontal_direction.y +
        horizontal_direction.z * horizontal_direction.z);
    horizontal_direction.x = horizontal_direction.x * inverse_length;
    horizontal_direction.y = horizontal_direction.y * inverse_length;
    horizontal_direction.z = horizontal_direction.z * inverse_length;
    angle_vector->x =
        horizontal_direction.y * world_up->z -
        horizontal_direction.z * world_up->y;
    angle_vector->y =
        horizontal_direction.z * world_up->x -
        horizontal_direction.x * world_up->z;
    angle_vector->z =
        horizontal_direction.x * world_up->y -
        horizontal_direction.y * world_up->x;
    inverse_length = gxMathFastInvSqrt(
        angle_vector->x * angle_vector->x +
        angle_vector->y * angle_vector->y +
        angle_vector->z * angle_vector->z);
    angle_vector->x *= inverse_length;
    angle_vector->y *= inverse_length;
    angle_vector->z *= inverse_length;
    angle = gxMathArcCos(-1.0f * displacement->y / npc->rope_length);
    angle *= 1.25f;
    if (horizontal_direction.x * displacement->x +
            horizontal_direction.y * displacement->y +
            horizontal_direction.z * displacement->z <
        0.0f) {
        angle *= -1.0f;
    }
    angle_vector->x *= angle;
    angle_vector->y *= angle;
    angle_vector->z *= angle;
}

/* TODO: [breakthrough needed] 97.41940%; PS2 also republishes acceleration;
 * recover the original vector operation behind six missing stores and FRSPs. */
static float p_npc_on_pendulum_rope(void) {
    MkObj* object;
    struct NbNpcState* npc;
    Vec acceleration;
    Vec normal;
    Vec angle_vector;
    Vec world_up = nb_world_up;
    Vec displacement;
    Vec velocity_direction;
    Vec tangent;
    float distance;
    float inverse_length;
    float speed_squared;
    float response;

    npc = ((struct NbNpcProcPdata*)apdata)->npc;
    object = npc->object;
    npc->momentum.z = 0.0f;
    npc->momentum.y = 0.0f;
    npc->momentum.x = 0.0f;

    for (;;) {
        npc->swing_ticks++;
        npc->phase += 0.018f;

        displacement.x = object->pos.value.x - npc->anchor.x;
        displacement.y = object->pos.value.y - npc->anchor.y;
        displacement.z = object->pos.value.z - npc->anchor.z;
        distance = gxMathFastSqrt(
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
                        "slave_blood_burst", npc->object, 8);
                    bgnd_launch_fx_at_bid_of_mkobj(
                        "slave_blood_spurt", npc->object, 8);
                    snd_req(0x10F);
                    snd_req(0xD5D);
                } else {
                    npc->momentum.y = 0.0f;
                    npc->active &= ~1;
                }
            }
            acceleration.z = 0.0f;
            acceleration.y = 0.0f;
            acceleration.x = 0.0f;
            acceleration.y = -npc->acceleration_scale;
            npc->momentum.x = acceleration.x + npc->momentum.x;
            npc->momentum.y += acceleration.y;
            npc->momentum.z += acceleration.z;
        } else {
            if (distance > npc->rope_length + 0.002f ||
                distance < npc->rope_length - 0.002f) {
                inverse_length = gxMathFastInvSqrt(
                    displacement.x * displacement.x +
                    displacement.y * displacement.y +
                    displacement.z * displacement.z);
                displacement.x *= inverse_length;
                displacement.y *= inverse_length;
                displacement.z *= inverse_length;
                displacement.x *= npc->rope_length;
                displacement.y *= npc->rope_length;
                displacement.z *= npc->rope_length;
                object->pos.value.x = npc->anchor.x + displacement.x;
                object->pos.value.y = npc->anchor.y + displacement.y;
                object->pos.value.z = npc->anchor.z + displacement.z;
            }

            acceleration.z = 0.0f;
            acceleration.y = 0.0f;
            acceleration.x = 0.0f;
            inverse_length = gxMathFastInvSqrt(
                displacement.x * displacement.x +
                displacement.y * displacement.y +
                displacement.z * displacement.z);
            normal.x = displacement.x * inverse_length;
            normal.y = displacement.y * inverse_length;
            normal.z = displacement.z * inverse_length;
            normal.x = -1.0f * normal.x;
            normal.y = -1.0f * normal.y;
            normal.z = -1.0f * normal.z;
            nb_get_desired_acceleration(
                npc, &acceleration, &normal);

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
                Vec cross;

                speed_squared =
                    npc->momentum.x * npc->momentum.x +
                    npc->momentum.y * npc->momentum.y +
                    npc->momentum.z * npc->momentum.z;
                inverse_length = gxMathFastInvSqrt(speed_squared);
                velocity_direction.x = npc->momentum.x * inverse_length;
                velocity_direction.y = npc->momentum.y * inverse_length;
                velocity_direction.z = npc->momentum.z * inverse_length;

                cross.x = velocity_direction.y * normal.z - velocity_direction.z * normal.y;
                cross.y = velocity_direction.z * normal.x - velocity_direction.x * normal.z;
                cross.z = velocity_direction.x * normal.y - velocity_direction.y * normal.x;
                tangent.x = cross.y * normal.z - cross.z * normal.y;
                tangent.y = cross.z * normal.x - cross.x * normal.z;
                tangent.z = cross.x * normal.y - cross.y * normal.x;
                inverse_length = gxMathFastInvSqrt(
                    tangent.x * tangent.x + tangent.y * tangent.y +
                    tangent.z * tangent.z);
                tangent.x *= inverse_length;
                tangent.y *= inverse_length;
                tangent.z *= inverse_length;
                nb_apply_tangent_momentum(npc, &tangent, speed_squared);
            }
        }

        object->pos.value.x += npc->momentum.x;
        object->pos.value.y += npc->momentum.y;
        object->pos.value.z += npc->momentum.z;
        bgnd_pebble_set_current_pebble(8, npc->npc_id - 1);
        bgnd_pebble_set_current_info(
            9, npc->object, npc->object->pos.value.x);
        bgnd_pebble_set_current_info(
            0xB, npc->object, npc->object->pos.value.z);

        if (npc->swing_angle != 0.0f) {
            npc->swing_angle *= 0.995f;
            if (npc->swing_angle < 0.008f &&
                npc->swing_angle > -0.008f) {
                npc->swing_angle = 0.0f;
            }
            npc->object->ang.y += npc->swing_angle;
        }

        nb_get_rope_angle(npc, &displacement, &world_up, &angle_vector);
        rotate_xz(&angle_vector, &angle_vector, -1.0f * npc->object->ang.y);

        if (angle_vector.x < 0.0f && npc->object->ang.x > 3.1415927f) {
            angle_vector.x = norm_angle_inline(angle_vector.x);
        }
        if (angle_vector.x >= 0.0f && angle_vector.x < 3.1415927f &&
            npc->object->ang.x > 3.1415927f) {
            npc->object->ang.x -= 6.2831855f;
        }
        if (angle_vector.z < 0.0f && npc->object->ang.z > 3.1415927f) {
            angle_vector.z = norm_angle_inline(angle_vector.z);
        }
        if (angle_vector.z >= 0.0f && angle_vector.z < 3.1415927f &&
            npc->object->ang.z > 3.1415927f) {
            npc->object->ang.z -= 6.2831855f;
        }

        response = (npc->active & 1) != 0 ? 60.0f : 4.0f;
        npc->object->ang.x -=
            (npc->object->ang.x - angle_vector.x) / response;
        npc->object->ang.z -=
            (npc->object->ang.z - angle_vector.z) / response;
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
}

static inline void nb_scale_desired_force(Vec* force, float scale)
{
    force->y *= scale;
    force->x *= scale;
    force->z *= scale;
}

static void nb_get_desired_acceleration(
    struct NbNpcState* state, Vec* acceleration, const Vec* surface_normal) {
    Vec force;
    float swing_angle;
    float scale;
    float inv_length;
    float squared_length;
    float normal_component;
    Vec normal;
    float tangent_force_y;
    float tangent_force_x;
    float tangent_force_z;

    force.z = 0.0f;
    acceleration->z = 0.0f;
    force.y = 0.0f;
    force.x = 0.0f;
    acceleration->y = 0.0f;
    acceleration->x = 0.0f;
    swing_angle = state->phase;

    if (state->swing_ticks < 0) {
        force.y = -1.0f;
    } else {
        scale = 24.0f * (float)refresh_rate();
        if (scale) {
            scale = (float)state->swing_ticks / scale;
            if (scale > 1.0f) {
                scale = 1.0f;
            }
            force.x = scale * gxMathSin(swing_angle);
            force.z = scale * gxMathCos(swing_angle);
            force.y = -6.0f;
        }
    }

    squared_length =
        force.z * force.z + (force.x * force.x + force.y * force.y);
    inv_length = gxMathFastInvSqrt(squared_length);

    force.y *= inv_length;
    force.x *= inv_length;
    force.z *= inv_length;
    nb_scale_desired_force(&force, state->acceleration_scale);

    normal.x = surface_normal->x;
    normal.y = surface_normal->y;
    normal.z = surface_normal->z;
    normal_component =
        -(float)(force.z * normal.z +
                 (force.x * normal.x + force.y * normal.y));

    tangent_force_x = normal.x * normal_component;
    tangent_force_y = normal.y * normal_component;
    tangent_force_z = normal.z * normal_component;
    tangent_force_x += force.x;
    tangent_force_y += force.y;
    tangent_force_z += force.z;

    acceleration->x = tangent_force_x / state->acceleration_divisor;
    acceleration->y = tangent_force_y / state->acceleration_divisor;
    acceleration->z = tangent_force_z / state->acceleration_divisor;
}

void nb_place_slave_in_bgnd(
    int npc_id, int rope_model_index, const char* model_name, int model_id,
    float anchor_x, float anchor_y, float anchor_z, float rope_length,
    float local_angle_x, float local_angle_y, float local_angle_z,
    float object_angle_x, float object_angle_y, float object_angle_z,
    float acceleration_divisor, float acceleration_scale, float field_8C) {
    struct NbNpcState* npc;
    MkObj* preload_object;
    Vec* object_position;
    Vec local_angles;
    Vec rope_offset;
    MKMATRIX rotation;
    float collision_offset_z;

    rope_offset = nb_rope_preload_rotation;
    bgnd_preload_named_model("ROPE", rope_model_index);
    bgnd_set_active_sobj_in_obj(rope_model_index, 0);
    bgnd_unhide_preload_obj(rope_model_index);
    bgnd_unhide_active_sobj();
    bgnd_set_active_sobj_pos(anchor_x, anchor_y, anchor_z);
    bgnd_preload_obj_attach_rope(rope_model_index);

    bgnd_create_named_npc_in_slot(npc_id, model_name, model_id, 0);
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
    npc->phase = 0.17444445f * (float)(unsigned int)npc_id;
    npc->swing_ticks = 3000;

    npc->object->ang_vel.z = 0.0f;
    npc->object->ang_vel.y = 0.0f;
    npc->object->ang_vel.x = 0.0f;
    npc->object->ang.x = object_angle_x;
    npc->object->ang.y = object_angle_y;
    npc->object->ang.z = object_angle_z;

    object_position = &npc->object->pos.value;
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

void rd_set_impact_vector(float scale) {
    Vec impact = nb_impact_zero;
    float squared_length;
    float inverse_length;

    impact.x =
        g_game_info.player_objects[1]->pos.value.x -
        g_game_info.player_objects[0]->pos.value.x;
    impact.z =
        g_game_info.player_objects[1]->pos.value.z -
        g_game_info.player_objects[0]->pos.value.z;

    squared_length = impact.x * impact.x + impact.z * impact.z;
    inverse_length = gxMathFastInvSqrt(squared_length);
    impact.x *= inverse_length;
    impact.z *= inverse_length;

    g_game_info.impact_vector.y = impact.y;
    g_game_info.impact_vector.x = impact.x;
    g_game_info.impact_vector.z = impact.z;
    g_game_info.impact_vector.x *= scale;
    g_game_info.impact_vector.y *= scale;
    g_game_info.impact_vector.z *= scale;
}

void bgnd_jtb_debug_info(void) {
}
