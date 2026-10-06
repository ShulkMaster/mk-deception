#include "game/collision.h"
#include "game/plyr_globals.h"
#include "platform/main.h"
#include "runtime/sound.h"
#include "runtime/anim_api_ext.h"
#include "game/game_info.h"
#include "game/projectile.h"
#include "game/jmt.h"
#include "math/gxVect.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "runtime/bone_matcher.h"
#include "runtime/asset.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/light.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"

union ProjectileTarget {
    PlyrPdata* impaled_target;
    struct ProjectilePdata* retarget_source;
};

struct ProjectileSetupBits {
    unsigned char impale_info_set : 1;
    unsigned char random_position_set : 1;
    unsigned char random_rotation_set : 1;
    unsigned char collision_info_set : 1;
    unsigned char hit_script_set : 1;
    unsigned char end_script_set : 1;
    unsigned char block_script_set : 1;
    unsigned char ground_script_set : 1;
};

struct ProjectileBehaviorBits {
    unsigned char velocity_damping_set : 1;
    unsigned char not_duckable : 1;
    unsigned char track_2d : 1;
    unsigned char track_3d : 1;
    unsigned char continue_through_hit : 1;
    unsigned char behavior_unknown_2_0 : 3;
};

union ProjectileSetupFlags {
    unsigned char raw;
    struct ProjectileSetupBits bits;
};

union ProjectileBehaviorFlags {
    unsigned char raw;
    struct ProjectileBehaviorBits bits;
};

struct ProjectileFlagBytes {
    union ProjectileSetupFlags setup;
    union ProjectileBehaviorFlags behavior;
};

union ProjectileFlags {
    unsigned int word;
    struct ProjectileFlagBytes bytes;
};

struct ProjectilePdata {
    MkHdr hdr;
    PlyrProcLatch player_process_latch;
    union ProjectileTarget target;
    MkObj* retarget_object;
    MkProc* process;
    unsigned int process_instance;
    MkObj* source_object;
    unsigned int source_object_instance;
    MkObj* object;
    unsigned int object_instance;
    float max_ticks;
    unsigned int hit_script_index;
    unsigned int end_callback_index;
    unsigned int block_script_index;
    unsigned int ground_script_index;
    int reaction;
    float reaction_scale;
    int reaction_flags;
    float collision_radius;
    float collision_height;
    float collision_depth;
    float ground_collision_ticks;
    float ground_height;
    Vec velocity_damping;
    Vec target_position;
    ProjectileImpaleInfo* impale_info;
    Vec random_position;
    Vec random_rotation;
    MslSoundHandle sound_handle;
    MkObj* tracking_light;
    unsigned int tracking_light_instance;
    int flight_sound;
    int impact_sound;
    int down_sound;
    union ProjectileFlags flags;
};

struct ProjectileScriptPdata {
    MkHdr hdr;
    PlyrPdata* owner;
    PlyrPdata* opponent;
    unsigned int script_index;
    Vec last_position;
    Vec velocity;
};

struct ProjectileFollowerPdata {
    MkHdr hdr;
    struct ProjectilePdata* projectile;
    unsigned int projectile_instance;
};


static struct ProjectilePdata* proj_pdata;

void set_active_projectile_velocity_to_hit_gnd(float ticks);
MkObj* set_active_projectile_tracking_light(LightDef* definition);
static float p_point_light_follower(void);
void set_active_projectile_rx_info(
    int reaction, int flags, float scale);
static MkObj* start_projectile_from_specific_plyr_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset,
    int use_sidekick);
static void ps_projectile(void);
static void pw_projectile(void);
void retarget_projectile(struct ProjectilePdata* pdata);
static void projectile_set_velocity_angy_tol(
    MkObj* object, float speed, float tolerance);
static void projectile_impale(struct ProjectilePdata* pdata, MkObj* victim);
static float p_projectile_handler(void);
static float p_projectile_continue(void);
static float p_ground_target(void);
static float p_ground_target_collide(void);
static float p_projectile_launch_upward(void);
static float p_projectile_downward(void);
static float p_projectile_impaled(void);
static float p_proj_end_run_script(void);

extern void trial_state_collision_check(
    int collision_result, int player);

extern int reaction_xfer_him(
    int reaction, float damage_scale, int block_type);
extern int collide_sphere_vs_plyr(
    PlyrInfo* player, const Vec* center, float radius);
extern void pz_fighter_reaction_xfer_him(int reaction);
extern void obj_set_all_sobjs_priority(MkObj* object, int priority);


static inline void projectile_set_target_position(const Vec* position) {
    if (proj_pdata != 0) {
        proj_pdata->target_position.x = position->x;
        proj_pdata->target_position.y = position->y;
        proj_pdata->target_position.z = position->z;
    }
}


static inline void projectile_set_process_handler(MkProcEntryFn handler) {
    MkProc* process;

    if (proj_pdata != 0) {
        process = MK_LIVE(proj_pdata->process, proj_pdata->process_instance);
        if (process != 0) {
            xfer_proc(process, handler);
        }
    }
}

static inline MkProc* projectile_start_end_script(
    PlyrPdata* owner, PlyrPdata* opponent,
    unsigned int script_index) {
    struct ProjectileScriptPdata* script_data;
    MkProc* process;

    script_data = 0;
    process = _create_mkproc_generic_tinystack(
        0xB00A, 0x1F, p_proj_end_run_script,
        sizeof(struct ProjectileScriptPdata), (MkHdr**)&script_data);
    if (process != 0 && script_data != 0) {
        script_data->owner = owner;
        script_data->opponent = opponent;
        script_data->script_index = script_index;
        set_process_as_scriptable(process);
        script_data->last_position.z = 0.0f;
        script_data->last_position.y = 0.0f;
        script_data->last_position.x = 0.0f;
        script_data->velocity.z = 0.0f;
        script_data->velocity.y = 0.0f;
        script_data->velocity.x = 0.0f;
    }
    return process;
}

static inline MkProc* projectile_start_script_snapshot(
    struct ProjectilePdata* projectile, const unsigned int* script_index) {
    struct ProjectileScriptPdata* script_data;
    MkObj* source;
    MkObj* object;
    MkProc* process;

    source = MK_HDR_LIVE(
        projectile->source_object, projectile->source_object_instance);
    if (source == 0) {
        return 0;
    }
    if (source == g_game_info.plyr0.slot.mirror_a) {
        process = projectile_start_end_script(
            g_game_info.plyr0.slot.pdata, projectile->target.impaled_target,
            *script_index);
    } else {
        process = projectile_start_end_script(
            g_game_info.plyr1.slot.pdata, projectile->target.impaled_target,
            *script_index);
    }
    if (process == 0) {
        return 0;
    }

    script_data = (struct ProjectileScriptPdata*)pdata_of_proc(process);
    object = MK_HDR_LIVE(projectile->object, projectile->object_instance);
    if (script_data != 0 && object != 0) {
        script_data->last_position.x = object->pos.value.x;
        script_data->last_position.y = object->pos.value.y;
        script_data->last_position.z = object->pos.value.z;
        script_data->velocity.x = object->pos_vel.x;
        script_data->velocity.y = object->pos_vel.y;
        script_data->velocity.z = object->pos_vel.z;
    }
    return process;
}

void set_active_projectile_target_ground(
    float ticks, float collision_ticks, float collision_radius) {
    if (proj_pdata != 0) {
        proj_pdata->ground_collision_ticks = collision_ticks;
        proj_pdata->collision_radius = collision_radius;
        projectile_set_process_handler(p_ground_target);
        set_active_projectile_velocity_to_hit_gnd(ticks);
    }
}

#pragma scheduling off
/* TODO: [near miss] 99.7%; body matches; saved-LR/r31 epilogue load order remains. */
void set_active_projectile_upward_attack(const Vec* target) {
    projectile_set_process_handler(p_projectile_launch_upward);
    projectile_set_target_position(target);
}

#pragma scheduling reset

int get_bid_with_flip(MkObj* object, unsigned int bone_id) {
    if (object->hide_flag_bits.bit6) {
        MkFlippedBoneMap* flipped = object->flipped_bone_map;

        if (bone_id < flipped->count) {
            bone_id = flipped->bone_indices[bone_id];
        }
    }
    return bone_id;
}

void active_projectile_setup_done(void) {
    proj_pdata = 0;
}

#pragma scheduling off
/* TODO: [near miss] 88.23529%; bit7 li/lbz order remains; interleaved vector stores and owner reloads agree. */
void set_active_projectile_velocity_damp(const Vec* damping) {
    if (proj_pdata != 0) {
        proj_pdata->flags.bytes.behavior.bits.velocity_damping_set = 1;
        proj_pdata->velocity_damping.x = damping->x;
        proj_pdata->velocity_damping.y = damping->y;
        proj_pdata->velocity_damping.z = damping->z;
    }
}

#pragma scheduling reset

void set_active_projectile_max_ticks(int ticks) {
    if (proj_pdata != 0) {
        proj_pdata->max_ticks = ticks;
    }
}

#pragma scheduling off
void set_active_projectile_target_pos(const Vec* position) {
    projectile_set_target_position(position);
}

#pragma scheduling reset

void set_active_projectile_p_handler(MkProcEntryFn handler) {
    projectile_set_process_handler(handler);
}

static inline float projectile_horizontal_length_squared(const Vec* velocity) {
    return velocity->x * velocity->x + velocity->z * velocity->z;
}

void set_active_projectile_velocity_to_hit_gnd(float ticks) {
    MkObj* object;
    float speed;
    float horizontal_inverse_length;
    float target_x;
    float target_y;
    float target_z;
    float target_inverse_length;
    float angle_inverse_length;

    if (proj_pdata == 0) {
        return;
    }
    object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);
    if (object == 0) {
        return;
    }

    object->flags_08_bits.gravity_enabled = 1;
    speed = gxMathFastSqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.y * object->pos_vel.y +
        object->pos_vel.z * object->pos_vel.z);
    target_z = object->pos_vel.z;
    target_x = object->pos_vel.x;
    horizontal_inverse_length = gxMathFastInvSqrt(
        target_x * target_x + target_z * target_z);
    target_x *= horizontal_inverse_length;
    target_z *= horizontal_inverse_length;
    target_x *= ticks;
    target_z *= ticks;
    target_y = g_game_info.field_34 - object->pos.value.y;
    if (target_y > 0.0f) {
        target_y = 0.0f;
    }
    target_inverse_length = gxMathFastInvSqrt(
        target_x * target_x + target_y * target_y + target_z * target_z);
    target_x *= target_inverse_length;
    target_y *= target_inverse_length;
    target_z *= target_inverse_length;
    object->pos_vel.x = target_x * speed;
    object->pos_vel.y = target_y * speed;
    object->pos_vel.z = target_z * speed;

    if (object->pos_vel.x == 0.0f && object->pos_vel.y == 0.0f) {
        object->ang.y = plyr_obj->ang.y;
        return;
    }
    angle_inverse_length = gxMathFastInvSqrt(
        projectile_horizontal_length_squared(&object->pos_vel));
    object->ang.y = gxMathArcTanYX(
        object->pos_vel.x * angle_inverse_length,
        object->pos_vel.z * angle_inverse_length);
}

void set_active_projectile_dn_sound(int sound) {
    if (proj_pdata != 0 && sound != 0) {
        proj_pdata->down_sound = sound;
    }
}

void set_active_projectile_sound(
    int start_sound, int flight_sound, int impact_sound) {
    if (proj_pdata != 0) {
        if (start_sound != 0) {
            proj_pdata->sound_handle = snd_req(start_sound);
        }
        if (flight_sound != 0) {
            proj_pdata->flight_sound = flight_sound;
        }
        if (impact_sound != 0) {
            proj_pdata->impact_sound = impact_sound;
        }
    }
}



/* TODO: [near miss] 97.70%; velocity y preload and inverse-length f2/f3 plus squared-length f4/f2 allocation remain. */
void set_active_projectile_velocity(const Vec* velocity) {
    MkObj* object;
    float inverse_length;

    if (proj_pdata != 0) {
        object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);

        if (object != 0) {
            object->flags_08_bits.gravity_enabled = 1;
            object->pos_vel.x = velocity->x;
            object->pos_vel.y = velocity->y;
            object->pos_vel.z = velocity->z;
            if (object->pos_vel.x == 0.0f &&
                object->pos_vel.y == 0.0f) {
                object->ang.y = plyr_obj->ang.y;
                return;
            }
            inverse_length = gxMathFastInvSqrt(
                object->pos_vel.x * object->pos_vel.x +
                object->pos_vel.z * object->pos_vel.z);
            object->ang.y = gxMathArcTanYX(
                object->pos_vel.x * inverse_length,
                object->pos_vel.z * inverse_length);
        }
    }
}

void set_active_add_ang_y(float angle) {
    MkObj* object;
    int fixed;

    if (proj_pdata != 0) {
        object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);

        if (object != 0) {
            object->ang.y += angle;
            fixed = (int)(166886.1f * object->ang.y) & 0xFFFFF;
            object->ang.y = 0.000005992112f * (float)fixed;
        }
    }
}

void set_active_projectile_hit_gnd_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->ground_script_index = script_index;
        proj_pdata->flags.bytes.setup.bits.ground_script_set = 1;
    }
}

void set_active_projectile_end_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->end_callback_index = script_index;
        proj_pdata->flags.bytes.setup.bits.end_script_set = 1;
    }
}

void set_active_projectile_block_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->block_script_index = script_index;
        proj_pdata->flags.bytes.setup.bits.block_script_set = 1;
    }
}

void set_active_projectile_hit_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->hit_script_index = script_index;
        proj_pdata->flags.bytes.setup.bits.hit_script_set = 1;
    }
}

void set_active_projectile_collision_info(
    float radius, int enabled, float height, float depth) {
    if (proj_pdata != 0) {
        if (enabled != 0) {
            proj_pdata->flags.bytes.setup.bits.collision_info_set = 1;
        } else {
            proj_pdata->flags.bytes.setup.bits.collision_info_set = 0;
        }
        proj_pdata->collision_height = height;
        proj_pdata->collision_depth = depth;
        proj_pdata->collision_radius = radius;
    }
}

void set_active_projectile_random_rot(float x, float y, float z) {
    if (proj_pdata != 0) {
        proj_pdata->random_rotation.x = x;
        proj_pdata->random_rotation.y = y;
        proj_pdata->random_rotation.z = z;
        proj_pdata->flags.bytes.setup.bits.random_rotation_set = 1;
    }
}

void set_active_projectile_random_pos(float x, float y, float z) {
    if (proj_pdata != 0) {
        proj_pdata->random_position.x = x;
        proj_pdata->random_position.y = y;
        proj_pdata->random_position.z = z;
        proj_pdata->flags.bytes.setup.bits.random_position_set = 1;
    }
}

void set_active_projectile_impale_info(
    ProjectileImpaleInfo* info, const int* bone_tags) {
    MkObj* object;

    if (proj_pdata != 0) {
        object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);

        if (object != 0) {
            build_bones_tbl(object, bone_tags);
            proj_pdata->impale_info = info;
            proj_pdata->flags.bytes.setup.bits.impale_info_set = 1;
        }
    }
}

void set_active_projectile_continue_thru_hit(void) {
    if (proj_pdata != 0) {
        proj_pdata->flags.bytes.behavior.bits.continue_through_hit = 1;
    }
}

void set_active_projectile_3d_track(void) {
    if (proj_pdata != 0) {
        proj_pdata->flags.bytes.behavior.bits.track_3d = 1;
    }
}

MkObj* set_active_projectile_tracking_light(LightDef* definition) {
    struct ProjectileFollowerPdata* follower;
    MkObj* light;
    MkProc* process;

    if (proj_pdata != 0) {
        light = load_light(definition, &point_light_list, 0);
        if (light != 0) {
            process = _create_mkproc_generic_tinystack(
                0x2026, 0x1F, p_point_light_follower,
                sizeof(struct ProjectileFollowerPdata), (MkHdr**)&follower);
            if (process == 0) {
                if (light->hdr.instance != 0) {
                    light->hdr.typed_vtbl->destroy(&light->hdr);
                }
                return 0;
            }

            proj_pdata->tracking_light = light;
            proj_pdata->tracking_light_instance = light->hdr.instance;
            follower->projectile = proj_pdata;
            follower->projectile_instance = proj_pdata->hdr.instance;
            mk_insert(&light->hdr, &process->pdata_list_b);
            return light;
        }
    }
    return 0;
}



static float p_point_light_follower(void) {
    struct ProjectileFollowerPdata* follower =
        (struct ProjectileFollowerPdata*)apdata;
    struct ProjectilePdata* projectile = follower->projectile;
    MkObj* object;
    MkObj* light;

    projectile = MK_HDR_LIVE(projectile, follower->projectile_instance);
    if (projectile != 0) {
        object = MK_HDR_LIVE(projectile->object, projectile->object_instance);

        if (object != 0) {
            light = MK_HDR_LIVE(projectile->tracking_light, projectile->tracking_light_instance);

            if (light != 0) {
                light->pos.value.x = object->pos.value.x;
                light->pos.value.y = object->pos.value.y;
                light->pos.value.z = object->pos.value.z;
                update_obj_pos(light);
                return 1.0f;
            }
        }
    }
    return -1.0f;
}

void set_active_projectile_2d_track(void) {
    if (proj_pdata != 0) {
        proj_pdata->flags.bytes.behavior.bits.track_2d = 1;
    }
}

void set_active_projectile_not_duckable(void) {
    if (proj_pdata != 0) {
        proj_pdata->flags.bytes.behavior.bits.not_duckable = 1;
    }
}

void set_active_projectile_rx_info(
    int reaction, int flags, float scale) {
    if (proj_pdata != 0) {
        proj_pdata->reaction = reaction;
        proj_pdata->reaction_scale = scale;
        proj_pdata->reaction_flags = flags;
    }
}

static inline MkObj* projectile_load_foreground_object(
    const char* model_name, int player) {
    MkObj* object;

    if (model_name == 0) {
        object = get_mkobj_frame(0xD000, 0);
    } else {
        object = load_named_model_for_player(
            model_name, player, 0xD000, 0);
    }
    if (object == 0) {
        return 0;
    }
    insert_fgnd_mkobj(object);
    return object;
}

static inline MkObj* projectile_create_attached_process(
    MkObj* object, MkObj* existing_object, MkHdr** allocated_data) {
    MkProc* process;

    if (object == 0) {
        return 0;
    }
    process = _create_mkproc_generic_tinystack(
        0x2026, 0x1F, p_projectile_handler,
        sizeof(struct ProjectilePdata), allocated_data);
    if (process == 0) {
        if (object != existing_object && object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy(&object->hdr);
        }
        return 0;
    }

    ((struct ProjectilePdata*)*allocated_data)->process = process;
    ((struct ProjectilePdata*)*allocated_data)->process_instance = process->instance;
    process->pre_destroy = pw_projectile;
    process->destroy_cb = ps_projectile;
    if (object != existing_object) {
        mk_insert(&object->hdr, &process->pdata_list_b);
    }

    return object;
}

/* TODO: [near miss] 98.34%; equivalent attached-object return join and
 * inverse-length FPR coloring remain; recover the factory exit shape. */
static MkObj* start_projectile_from_specific_plyr_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset,
    int use_sidekick) {
    struct ProjectilePdata* projectile;
    MkHdr* allocated_data;
    MkObj* object;
    MkObj* launch_object;
    Vec position;
    float squared_length;
    float inverse_length;
    int player;

    proj_pdata = 0;
    if (plyr_obj == 0) {
        return 0;
    }

    if (existing_object != 0) {
        object = existing_object;
    } else {
        player = get_player_number(plyr_obj);
        object = projectile_load_foreground_object(model_name, player);
        if (object == 0) {
            return 0;
        }
        obj_set_all_sobjs_priority(object, 0x13);
    }

    object->hide_flag_bits.hidden = 1;
    object->flags_08_bits.angular_velocity_enabled = 1;
    object->flags_08_bits.gravity_enabled = 1;
    object = projectile_create_attached_process(
        object, existing_object, &allocated_data);
    if (object == 0) {
        return 0;
    }

    if (use_sidekick == 0) {
        get_bone_offset_world_pos(
            plyr_obj, bone_id, (Vec*)bone_offset, &position);
    } else {
        launch_object = MK_HDR_LIVE(
            plyr_pdata->sidekick_obj, plyr_pdata->sidekick_instance);
        if (launch_object == 0) {
            get_bone_offset_world_pos(
                plyr_obj, bone_id, (Vec*)bone_offset, &position);
        } else {
            get_bone_offset_world_pos(
                launch_object, bone_id, (Vec*)bone_offset, &position);
        }
    }
    object->pos.value.x = position.x;
    object->pos.value.y = position.y;
    object->pos.value.z = position.z;
    projectile_set_velocity_angy_tol(
        object, get_adjusted_speed(speed, 0.8f), tolerance);
    if (object->pos_vel.x == 0.0f && object->pos_vel.y == 0.0f) {
        object->ang.y = plyr_obj->ang.y;
    } else {
        squared_length =
            object->pos_vel.x * object->pos_vel.x +
            object->pos_vel.z * object->pos_vel.z;
        inverse_length = gxMathFastInvSqrt(squared_length);
        object->ang.y = gxMathArcTanYX(
            object->pos_vel.x * inverse_length,
            object->pos_vel.z * inverse_length);
    }
    if (model_name != 0 || existing_object != 0) {
        update_mkobj(object);
    }
    object->hide_flag_bits.hidden = 0;

    ((struct ProjectilePdata*)allocated_data)->source_object = plyr_obj;
    ((struct ProjectilePdata*)allocated_data)->source_object_instance =
        plyr_obj->hdr.instance;
    ((struct ProjectilePdata*)allocated_data)->object = object;
    ((struct ProjectilePdata*)allocated_data)->object_instance =
        object->hdr.instance;
    projectile = (struct ProjectilePdata*)allocated_data;
    projectile->player_process_latch = plyr_pdata->player_proc_latch;
    projectile->target.impaled_target = plyr_pdata->his_plyr_pdata;
    projectile->retarget_object = plyr_pdata->his_obj;
    projectile->target.impaled_target->his_plyr_pdata->duck_reaction_active = 0;
    projectile->flags.word = 0;
    projectile->max_ticks = 240.0f;
    projectile->reaction = -1;
    projectile->reaction_scale = 0.0f;
    projectile->reaction_flags = 0;
    projectile->collision_radius = 0.2f;
    projectile->collision_height = 0.0f;
    projectile->collision_depth = 100.0f;
    projectile->impale_info = 0;
    projectile->random_position.x = projectile->random_position.y =
        projectile->random_position.z = 0.0f;
    projectile->random_rotation.x = projectile->random_rotation.y =
        projectile->random_rotation.z = 0.0f;
    projectile->target_position.x = projectile->target_position.y =
        projectile->target_position.z = 0.0f;
    projectile->velocity_damping.x = projectile->velocity_damping.y =
        projectile->velocity_damping.z = 0.0f;
    projectile->hit_script_index = 0;
    projectile->end_callback_index = 0;
    projectile->ground_collision_ticks = 0.0f;
    projectile->sound_handle = 0;
    projectile->flight_sound = 0;
    projectile->impact_sound = 0;
    projectile->tracking_light = 0;
    projectile->tracking_light_instance = 0;
    projectile->ground_height = g_game_info.field_34;
    proj_pdata = (struct ProjectilePdata*)allocated_data;
    return object;
}

MkObj* start_projectile_from_sidekick_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset) {
    return start_projectile_from_specific_plyr_bone(
        bone_id, existing_object, model_name, speed, tolerance,
        bone_offset, 1);
}

MkObj* start_projectile_from_plyr_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset) {
    return start_projectile_from_specific_plyr_bone(
        bone_id, existing_object, model_name, speed, tolerance,
        bone_offset, 0);
}

static void ps_projectile(void) {
    proj_pdata = 0;
}

static void pw_projectile(void) {
    proj_pdata = (struct ProjectilePdata*)pdata_of_proc(aproc);
}

/* TODO: [near miss] 99.72%; heading inverse-sqrt FP register allocation remains. */
void retarget_projectile(struct ProjectilePdata* pdata) {
    struct ProjectilePdata* source;
    MkObj* object;
    float speed;
    float dx;
    float dz;
    float inverse_length;

    if (pdata == 0) {
        return;
    }
    source = pdata->target.retarget_source;
    object = MK_HDR_LIVE(pdata->object, pdata->object_instance);
    if (object == 0) {
        return;
    }

    pdata->player_process_latch = source->player_process_latch;
    pdata->target.retarget_source = source->target.retarget_source;
    pdata->retarget_object = source->retarget_object;

    speed = gxMathFastSqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.y * object->pos_vel.y +
        object->pos_vel.z * object->pos_vel.z);
    dx = pdata->retarget_object->pos.value.x - object->pos.value.x;
    dz = pdata->retarget_object->pos.value.z - object->pos.value.z;
    inverse_length = gxMathFastInvSqrt(dx * dx + dz * dz);
    object->pos_vel.x = dx * inverse_length;
    object->pos_vel.z = dz * inverse_length;
    object->pos_vel.y = 0.0f;
    object->pos_vel.x *= speed;
    object->pos_vel.y *= speed;
    object->pos_vel.z *= speed;

    if (object->pos_vel.x == 0.0f && object->pos_vel.y == 0.0f) {
        object->ang.y = plyr_obj->ang.y;
    } else {
        inverse_length = gxMathFastInvSqrt(
            object->pos_vel.x * object->pos_vel.x +
            object->pos_vel.z * object->pos_vel.z);
        object->ang.y = gxMathArcTanYX(
            object->pos_vel.x * inverse_length,
            object->pos_vel.z * inverse_length);
    }
    pdata->max_ticks = 300.0f;
}


static inline void projectile_cross_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

static void projectile_set_velocity_angy_tol(
    MkObj* object, float speed, float tolerance) {
    float cone_cos;
    float dx;
    float dz;
    float inverse_length;
    float direction_x;
    float direction_z;
    Vec forward;
    Vec side_axis;
    Vec side_direction;
    Vec forward_component;
    float dot;
    float side;
    float cone_sin;
    float normalized_side_x;
    float normalized_side_z;

    cone_cos = gxMathCos((3.1415f * tolerance) / 360.0f);
    dx = his_obj->pos.value.x - object->pos.value.x;
    dz = his_obj->pos.value.z - object->pos.value.z;
    inverse_length = gxMathFastInvSqrt(dx * dx + dz * dz);
    direction_x = dx * inverse_length;
    direction_z = dz * inverse_length;
    forward.x = gxMathSin(plyr_obj->ang.y);
    forward.y = 0.0f;
    forward.z = gxMathCos(plyr_obj->ang.y);
    dot = forward.x * direction_x + forward.z * direction_z;

    if (dot > cone_cos) {
        Vec launch_direction;

        launch_direction.x = direction_x;
        launch_direction.y = 0.0f;
        launch_direction.z = direction_z;
        gxVectScale(&object->pos_vel, &launch_direction, speed);
        return;
    }

    side = forward.z * direction_x - forward.x * direction_z;
    side_axis.x = 0.0f;
    side_axis.y = side;
    side_axis.z = 0.0f;
    projectile_cross_v3(&side_direction, &side_axis, &forward);
    cone_sin = gxMathFastSqrt(1.0f - cone_cos * cone_cos);
    inverse_length = gxMathFastInvSqrt(
        side_direction.x * side_direction.x + side_direction.z * side_direction.z);
    normalized_side_x = side_direction.x * inverse_length;
    object->pos_vel.x = normalized_side_x * cone_sin;
    normalized_side_z = side_direction.z * inverse_length;
    object->pos_vel.z = normalized_side_z * cone_sin;
    gxVectScale(&forward_component, &forward, cone_cos);
    object->pos_vel.x += forward_component.x;
    object->pos_vel.z += forward_component.z;
    inverse_length = gxMathFastInvSqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.z * object->pos_vel.z);
    object->pos_vel.x = object->pos_vel.x * inverse_length;
    object->pos_vel.z *= inverse_length;
    object->pos_vel.x *= speed;
    object->pos_vel.y *= speed;
    object->pos_vel.z *= speed;
}

static void projectile_impale(struct ProjectilePdata* pdata, MkObj* victim) {
    BoneMatcherState* matcher;
    MkBone* victim_bone;
    Vec angles;

    if (pdata->impale_info == 0) {
        return;
    }

    matcher = start_bone_matcher(
        pdata->retarget_object,
        pdata->impale_info->parent_bone,
        victim,
        pdata->impale_info->child_bone,
        0.0f);
    if (matcher == 0) {
        return;
    }

    matcher->flags_08.bits.copy_bone_matrix = 1;
    matcher->flags_08.bits.preserve_bone_matrix = 1;
    matcher->child_offset.x = pdata->impale_info->child_offset.x;
    matcher->child_offset.y = pdata->impale_info->child_offset.y;
    matcher->child_offset.z = pdata->impale_info->child_offset.z;

    if (pdata->flags.bytes.setup.bits.random_position_set == 1U) {
        float random_x;
        float random_y;
        float random_z;
        random_x = frand(pdata->random_position.x) -
                   0.5f * pdata->random_position.x;
        random_y = frand(pdata->random_position.y) -
                   0.5f * pdata->random_position.y;
        random_z = frand(pdata->random_position.z) -
                   0.5f * pdata->random_position.z;
        matcher->parent_offset.x = pdata->impale_info->parent_offset.x + random_x;
        matcher->parent_offset.y = pdata->impale_info->parent_offset.y + random_y;
        matcher->parent_offset.z = pdata->impale_info->parent_offset.z + random_z;
    } else {
        matcher->parent_offset.x = pdata->impale_info->parent_offset.x;
        matcher->parent_offset.y = pdata->impale_info->parent_offset.y;
        matcher->parent_offset.z = pdata->impale_info->parent_offset.z;
    }

    victim_bone = victim->bones[victim->fallback_bone_index];
    if (victim_bone == 0) {
        if (matcher->hdr.instance != 0) {
            matcher->hdr.typed_vtbl->destroy(&matcher->hdr);
        }
        return;
    }

    victim_bone->flags_54_bits.pose_matrix_applied = 1;
    if (pdata->flags.bytes.setup.bits.random_rotation_set == 1U) {
        float random_x;
        float random_y;
        float random_z;
        random_x = frand(pdata->random_rotation.x) -
                   0.5f * pdata->random_rotation.x;
        random_y = frand(pdata->random_rotation.y) -
                   0.5f * pdata->random_rotation.y;
        random_z = frand(pdata->random_rotation.z) -
                   0.5f * pdata->random_rotation.z;
        angles.x = pdata->impale_info->rotation.x + random_x;
        angles.y = pdata->impale_info->rotation.y + random_y;
        angles.z = pdata->impale_info->rotation.z + random_z;
        YXZ_angles_to_MKMATRIX(&angles, victim_bone->parent_matrix);
    } else {
        YXZ_angles_to_MKMATRIX(&pdata->impale_info->rotation, victim_bone->parent_matrix);
    }

    pdata->max_ticks = 1800.0f;
    victim->flags_08_bits.angular_velocity_enabled = 0;
    victim->flags_08_bits.gravity_enabled = 0;
}

/* TODO: [near miss] 98.82481%; script snapshots and velocity-X retention agree;
 * square-root FP homes and collision-call staging remain. */
static float p_projectile_handler(void) {
    struct ProjectilePdata* projectile;
    PlyrPdata* victim;
    MkObj* object;
    MkObj* target;
    Vec bone_position;
    float speed;
    float inverse_length;
    float dx;
    float dz;
    float distance_squared;
    int collision;
    int collision_result;
    int impale;

    object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);
    if (object != 0) {
        proj_pdata->max_ticks -= game_speed;
        if (!(proj_pdata->max_ticks < 0.0f)) {
            if (proj_pdata->flags.bytes.behavior.bits.velocity_damping_set) {
                object->pos_vel.x *= proj_pdata->velocity_damping.x;
                object->pos_vel.y *= proj_pdata->velocity_damping.y;
                object->pos_vel.z *= proj_pdata->velocity_damping.z;
            }
            if (proj_pdata->flags.bytes.behavior.bits.track_2d) {
                get_bone_world_pos(
                    proj_pdata->retarget_object, 0x10, &bone_position);
                dx = proj_pdata->retarget_object->pos.value.x - object->pos.value.x;
                dz = proj_pdata->retarget_object->pos.value.z - object->pos.value.z;
                if (dx * dx + dz * dz < 3.0f) {
                    speed = gxMathFastSqrt(
                        object->pos_vel.x * object->pos_vel.x +
                        object->pos_vel.y * object->pos_vel.y +
                        object->pos_vel.z * object->pos_vel.z);
                    object->pos_vel.y =
                        (bone_position.y - object->pos.value.y) / 5.0f;
                    inverse_length = gxMathFastInvSqrt(
                        object->pos_vel.x * object->pos_vel.x +
                        object->pos_vel.y * object->pos_vel.y +
                        object->pos_vel.z * object->pos_vel.z);
                    object->pos_vel.x = object->pos_vel.x * inverse_length;
                    object->pos_vel.y *= inverse_length;
                    object->pos_vel.z *= inverse_length;
                    object->pos_vel.x *= speed;
                    object->pos_vel.y *= speed;
                    object->pos_vel.z *= speed;
                }
            }
            if (proj_pdata->flags.bytes.behavior.bits.track_3d) {
                get_bone_world_pos(
                    proj_pdata->retarget_object, 0x10, &bone_position);
                dx = proj_pdata->retarget_object->pos.value.x - object->pos.value.x;
                dz = proj_pdata->retarget_object->pos.value.z - object->pos.value.z;
                distance_squared = dx * dx + dz * dz;
                if (distance_squared < 0.25f) {
                    proj_pdata->flags.bytes.behavior.bits.track_3d = 0;
                } else if (distance_squared < 3.0f) {
                    speed = gxMathFastSqrt(
                        object->pos_vel.x * object->pos_vel.x +
                        object->pos_vel.y * object->pos_vel.y +
                        object->pos_vel.z * object->pos_vel.z);
                    object->pos_vel.x = dx;
                    object->pos_vel.z = dz;
                    inverse_length = gxMathFastInvSqrt(
                        object->pos_vel.x * object->pos_vel.x +
                        object->pos_vel.y * object->pos_vel.y +
                        object->pos_vel.z * object->pos_vel.z);
                    object->pos_vel.x = object->pos_vel.x * inverse_length;
                    object->pos_vel.y *= inverse_length;
                    object->pos_vel.z *= inverse_length;
                    object->pos_vel.x *= speed;
                    object->pos_vel.y *= speed;
                    object->pos_vel.z *= speed;
                }
            }

            target = proj_pdata->target.impaled_target->his_obj;
            victim = proj_pdata->target.impaled_target->his_plyr_pdata;
            collision = simple_3d_projectile_collision(
                &target->pos.value, &proj_pdata->retarget_object->pos.value,
                &object->pos.value,
                proj_pdata->flags.bytes.setup.bits.collision_info_set != 0,
                proj_pdata->collision_radius, proj_pdata->collision_depth,
                proj_pdata->collision_height);
            proj_pdata->target.impaled_target->his_plyr_pdata->duck_reaction_active = 1;
            proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_x =
                object->pos.value.x;
            proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_y =
                object->pos.value.y;
            proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_z =
                object->pos.value.z;

            if (object->pos.value.y < 0.2f + g_game_info.field_34) {
                trial_state_collision_check(
                    0, target == g_game_info.plyr0.slot.mirror_a);
                projectile = proj_pdata;
                projectile->target.impaled_target->his_plyr_pdata
                    ->duck_reaction_active = 0;
                if (projectile->sound_handle != 0) {
                    snd_stop(projectile->sound_handle);
                    projectile->sound_handle = 0;
                }
                if (projectile->flags.bytes.setup.bits.ground_script_set) {
                    projectile_start_script_snapshot(
                        projectile, &projectile->ground_script_index);
                }
                if (proj_pdata->flight_sound != 0) {
                    snd_req(proj_pdata->flight_sound);
                }
            } else {
                if (collision == 0 &&
                    !proj_pdata->flags.bytes.behavior.bits.not_duckable &&
                    (proj_pdata->target.impaled_target->state == 0x101 ||
                     proj_pdata->target.impaled_target->state == 0x302 ||
                     proj_pdata->target.impaled_target->state == 0x900 ||
                     proj_pdata->target.impaled_target->state == 0x1300) &&
                    proj_pdata->reaction_flags != 1) {
                    collision = 4;
                }
                if (proj_pdata->target.impaled_target->state_flags.bits
                        .projectile_invulnerable &&
                    collision == 0) {
                    collision = 1;
                } else if (collision == 0 &&
                           collide_sphere_vs_plyr(
                               proj_pdata->target.impaled_target->plyr_info,
                               &object->pos.value,
                               proj_pdata->collision_radius) == 0) {
                    collision = 4;
                }
                if (collision == 1 || collision == 2) {
                    trial_state_collision_check(
                        0, target == g_game_info.plyr0.slot.mirror_a);
                } else if (collision == 0) {
                    trial_state_collision_check(
                        1, target == g_game_info.plyr0.slot.mirror_a);
                }

                if (collision == 0) {
                    impale = 0;
                    collision_result = 0;
                    if (proj_pdata->target.impaled_target->state == 0x1222) {
                        retarget_projectile(proj_pdata);
                        return 1.0f;
                    }
                    proj_pdata->flags.bytes.behavior.bits.track_2d = 0;
                    proj_pdata->flags.bytes.behavior.bits.track_3d = 0;
                    projectile = proj_pdata;
                    projectile->target.impaled_target->his_plyr_pdata
                        ->duck_reaction_active = 0;
                    if (projectile->sound_handle != 0) {
                        snd_stop(projectile->sound_handle);
                        projectile->sound_handle = 0;
                    }
                    if (projectile->flags.bytes.setup.bits.hit_script_set) {
                        projectile_start_script_snapshot(
                            projectile, &projectile->hit_script_index);
                    }
                    if (proj_pdata->reaction != -1) {
                        if (mode_of_play != 6) {
                            reaction_xfer_him(
                                proj_pdata->reaction,
                                proj_pdata->reaction_scale,
                                proj_pdata->reaction_flags);
                        } else {
                            pz_fighter_reaction_xfer_him(proj_pdata->reaction);
                        }
                        collision_result = victim->collision_result;
                    }
                    if (collision_result == 1 &&
                        proj_pdata->flags.bytes.setup.bits.impale_info_set == 1 &&
                        victim->his_plyr_pdata->impaled_projectile_state < 3) {
                        impale = 1;
                    }
                    if (collision_result == 1) {
                        if (proj_pdata->flight_sound != 0) {
                            snd_req(proj_pdata->flight_sound);
                        }
                    } else if (collision_result == 2) {
                        if (proj_pdata->impact_sound != 0) {
                            snd_req(proj_pdata->impact_sound);
                        }
                        projectile = proj_pdata;
                        projectile->target.impaled_target->his_plyr_pdata
                            ->duck_reaction_active = 0;
                        if (projectile->sound_handle != 0) {
                            snd_stop(projectile->sound_handle);
                            projectile->sound_handle = 0;
                        }
                        if (projectile->flags.bytes.setup.bits.block_script_set) {
                            projectile_start_script_snapshot(
                                projectile, &projectile->block_script_index);
                        }
                    }
                    if (impale) {
                        projectile_impale(proj_pdata, object);
                        victim->his_plyr_pdata->impaled_projectile_state++;
                        aproc->vtbl->jump_sleep(p_projectile_impaled, 0.0f);
                        return 0.0f;
                    }
                    if (proj_pdata->flags.bytes.behavior.bits.continue_through_hit) {
                        aproc->vtbl->jump_sleep(p_projectile_continue, 0.0f);
                        return 0.0f;
                    }
                } else if (collision == 4) {
                    return 1.0f;
                } else if (collision == 1) {
                    proj_pdata->target.impaled_target->his_plyr_pdata
                        ->duck_reaction_active = 0;
                    return 1.0f;
                }
            }
        }
    }

    aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
    return 0.0f;
}

static float p_projectile_continue(void) {
    MkObj* object;
    MkObj* target;
    int collision;

    object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);
    if (object != 0) {
        target = proj_pdata->target.impaled_target->his_obj;
        collision = simple_3d_projectile_collision(
            &target->pos.value, &proj_pdata->retarget_object->pos.value,
            &object->pos.value,
            proj_pdata->flags.bytes.setup.bits.collision_info_set != 0,
            proj_pdata->collision_radius,
            proj_pdata->collision_depth,
            proj_pdata->collision_height);
        if (collision == 1 || collision == 2) {
            trial_state_collision_check(0, target == g_game_info.plyr0.slot.mirror_a);
        } else if (collision == 0) {
            trial_state_collision_check(1, target == g_game_info.plyr0.slot.mirror_a);
        }
        if (collision != 2) {
            return 1.0f;
        }
    }
    aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
    return 0.0f;
}

static float p_ground_target(void) {
    struct ProjectilePdata* projectile;
    MkObj* object;

    object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);
    if (object == 0) {
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    proj_pdata->target.impaled_target->his_plyr_pdata->duck_reaction_active = 1;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_x =
        object->pos.value.x;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_y =
        object->pos.value.y;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_z =
        object->pos.value.z;
    if (object->pos.value.y < g_game_info.field_34) {
        if (proj_pdata->flight_sound != 0) {
            snd_req(proj_pdata->flight_sound);
        }
        projectile = proj_pdata;
        projectile->target.impaled_target->his_plyr_pdata->duck_reaction_active = 0;
        if (projectile->sound_handle != 0) {
            snd_stop(projectile->sound_handle);
            projectile->sound_handle = 0;
        }

        if (projectile->flags.bytes.setup.bits.hit_script_set) {
            projectile_start_script_snapshot(
                projectile, &projectile->hit_script_index);
        }

        object->pos_vel.z = 0.0f;
        object->pos_vel.y = 0.0f;
        object->pos_vel.x = 0.0f;
        proj_pdata->max_ticks = proj_pdata->ground_collision_ticks;
        aproc->vtbl->jump_sleep(p_ground_target_collide, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

static float p_ground_target_collide(void) {
    Vec collision_angles = {-1.57079637f, 0.0f, 0.0f};
    PlyrInfo* player_info;
    MkObj* object;
    MkProc* hold_proc;

    object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);
    if (object == 0) {
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    proj_pdata->target.impaled_target->his_plyr_pdata->duck_reaction_active = 1;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_x = object->pos.value.x;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_y = object->pos.value.y;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_z = object->pos.value.z;
    if (proj_pdata->target.impaled_target == g_game_info.plyr0.slot.pdata) {
        player_info = &g_game_info.plyr0;
    } else {
        player_info = &g_game_info.plyr1;
    }
    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        trial_state_collision_check(0, player_info->controller_slot);
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    hold_proc = MK_LIVE(
        proj_pdata->target.impaled_target->his_plyr_pdata->hold_proc,
        proj_pdata->target.impaled_target->his_plyr_pdata->hold_proc_instance);
    if (hold_proc != 0) {
        return 1.0f;
    }
    if (collide_cylinder_vs_plyr(
            player_info, &object->pos.value,
            &collision_angles,
            proj_pdata->collision_radius, 0.3f) != 0) {
        if (proj_pdata->reaction != -1 &&
            !proj_pdata->target.impaled_target->state_flags.bits
                 .projectile_invulnerable) {
            trial_state_collision_check(
                1, player_info->controller_slot);
            reaction_xfer_him(
                proj_pdata->reaction, proj_pdata->reaction_scale,
                proj_pdata->reaction_flags);
        }
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}


int check_for_throw(PlyrPdata* player) {
    MkProc* hold_proc = MK_LIVE(player->his_plyr_pdata->hold_proc, player->his_plyr_pdata->hold_proc_instance);

    if (hold_proc != 0) {
        return 1;
    }
    return 0;
}

static float p_projectile_launch_upward(void) {
    MkObj* object;

    proj_pdata->target.impaled_target->his_plyr_pdata->duck_reaction_active = 0;
    object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);
    if (object == 0) {
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        object->pos.value.x = proj_pdata->target_position.x;
        object->pos.value.z = proj_pdata->target_position.z;
        object->pos_vel.x = -1.0f * object->pos_vel.x;
        object->pos_vel.y = -1.0f * object->pos_vel.y;
        object->pos_vel.z = -1.0f * object->pos_vel.z;
        proj_pdata->max_ticks = 300.0f;
        if (proj_pdata->down_sound != 0) {
            snd_req(proj_pdata->down_sound);
        }
        aproc->vtbl->jump_sleep(p_projectile_downward, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

/* TODO: [near miss] 98.87%; effects and four script expansions agree; pdata/process
 * coloring and join copies remain. */
static float p_projectile_downward(void) {
    struct ProjectilePdata* projectile;
    struct ProjectileScriptPdata* script_data;
    PlyrPdata* victim;
    MkObj* object;
    MkObj* source;
    MkProc* process;
    Vec target_position;
    float dx;
    float dy;
    float dz;

    object = MK_HDR_LIVE(proj_pdata->object, proj_pdata->object_instance);
    if (object == 0) {
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    proj_pdata->target.impaled_target->his_plyr_pdata->duck_reaction_active = 1;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_x =
        object->pos.value.x;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_y =
        object->pos.value.y;
    proj_pdata->target.impaled_target->his_plyr_pdata->saved_position_z =
        object->pos.value.z;
    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }
    if (proj_pdata->ground_height != g_game_info.field_34) {
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    if (object->pos.value.y < 0.2 + g_game_info.field_34) {
        trial_state_collision_check(0, proj_pdata->target.impaled_target->plyr_num);
        projectile = proj_pdata;
        projectile->target.impaled_target->his_plyr_pdata->duck_reaction_active = 0;
        if (projectile->sound_handle != 0) {
            snd_stop(projectile->sound_handle);
            projectile->sound_handle = 0;
        }

        if (projectile->flags.bytes.setup.bits.ground_script_set) {
            source = MK_HDR_LIVE(projectile->source_object,
                                 projectile->source_object_instance);
            if (source != 0) {
                if (source == g_game_info.plyr0.slot.mirror_a) {
                    process = projectile_start_end_script(
                        g_game_info.plyr0.slot.pdata, projectile->target.impaled_target,
                        projectile->ground_script_index);
                } else {
                    process = projectile_start_end_script(
                        g_game_info.plyr1.slot.pdata, projectile->target.impaled_target,
                        projectile->ground_script_index);
                }
                if (process != 0) {
                    script_data = (struct ProjectileScriptPdata*)pdata_of_proc(process);
                    object =
                        MK_HDR_LIVE(projectile->object, projectile->object_instance);
                    if (script_data != 0 && object != 0) {
                        script_data->last_position.x = object->pos.value.x;
                        script_data->last_position.y = object->pos.value.y;
                        script_data->last_position.z = object->pos.value.z;
                        script_data->velocity.x = object->pos_vel.x;
                        script_data->velocity.y = object->pos_vel.y;
                        script_data->velocity.z = object->pos_vel.z;
                    }
                }
            }
        }

        if (proj_pdata->flight_sound != 0) {
            snd_req(proj_pdata->flight_sound);
        }
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    get_bone_world_pos(proj_pdata->retarget_object, 0x10, &target_position);
    dx = target_position.x - object->pos.value.x;
    dy = target_position.y - object->pos.value.y;
    dz = target_position.z - object->pos.value.z;
    if (dx * dx + dy * dy + dz * dz < 0.2) {
        projectile = proj_pdata;
        victim = projectile->target.impaled_target;
        if (!victim->state_flags.bits.projectile_invulnerable &&
            victim->state != 0x1222 && projectile->reaction != -1) {
            victim->his_plyr_pdata->duck_reaction_active = 0;
            if (projectile->sound_handle != 0) {
                snd_stop(projectile->sound_handle);
                projectile->sound_handle = 0;
            }

            if (projectile->flags.bytes.setup.bits.hit_script_set) {
                source = MK_HDR_LIVE(projectile->source_object,
                                     projectile->source_object_instance);
                if (source != 0) {
                    if (source == g_game_info.plyr0.slot.mirror_a) {
                        process = projectile_start_end_script(
                            g_game_info.plyr0.slot.pdata,
                            projectile->target.impaled_target,
                            projectile->hit_script_index);
                    } else {
                        process = projectile_start_end_script(
                            g_game_info.plyr1.slot.pdata,
                            projectile->target.impaled_target,
                            projectile->hit_script_index);
                    }
                    if (process != 0) {
                        script_data =
                            (struct ProjectileScriptPdata*)pdata_of_proc(process);
                        object = MK_HDR_LIVE(projectile->object,
                                             projectile->object_instance);
                        if (script_data != 0 && object != 0) {
                            script_data->last_position.x = object->pos.value.x;
                            script_data->last_position.y = object->pos.value.y;
                            script_data->last_position.z = object->pos.value.z;
                            script_data->velocity.x = object->pos_vel.x;
                            script_data->velocity.y = object->pos_vel.y;
                            script_data->velocity.z = object->pos_vel.z;
                        }
                    }
                }
            }

            trial_state_collision_check(1, proj_pdata->target.impaled_target->plyr_num);
            reaction_xfer_him(proj_pdata->reaction, proj_pdata->reaction_scale,
                              proj_pdata->reaction_flags);
            projectile = proj_pdata;
            if (projectile->target.impaled_target->his_plyr_pdata->collision_result ==
                1) {
                if (projectile->flight_sound != 0) {
                    snd_req(projectile->flight_sound);
                }
            } else if (projectile->target.impaled_target->his_plyr_pdata
                               ->collision_result == 2 &&
                       projectile->impact_sound != 0) {
                snd_req(projectile->impact_sound);
            }
            aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
            return 0.0f;
        }
    }
    return 1.0f;
}


static float p_projectile_impaled(void) {
    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        proj_pdata->target.impaled_target->impaled_projectile_state--;
        aproc->vtbl->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

float p_projectile_die(void) {
    struct ProjectilePdata* projectile;

    if (proj_pdata->sound_handle != 0) {
        snd_stop(proj_pdata->sound_handle);
        proj_pdata->sound_handle = 0;
    }
    projectile = proj_pdata;
    projectile->target.impaled_target->his_plyr_pdata->duck_reaction_active = 0;
    if (projectile->flags.bytes.setup.bits.end_script_set) {
        projectile_start_script_snapshot(
            projectile, &projectile->end_callback_index);
    }
    return -1.0f;
}

void get_projectile_script_velocity(Vec* velocity) {
    struct ProjectileScriptPdata* pdata = (struct ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        velocity->x = pdata->velocity.x;
        velocity->y = pdata->velocity.y;
        velocity->z = pdata->velocity.z;
    }
}

void get_projectile_script_last_pos(Vec* position) {
    struct ProjectileScriptPdata* pdata = (struct ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        position->x = pdata->last_position.x;
        position->y = pdata->last_position.y;
        position->z = pdata->last_position.z;
    }
}

PlyrPdata* get_projectile_script_plyr_pdata(void) {
    struct ProjectileScriptPdata* pdata = (struct ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        return pdata->owner;
    }
    return 0;
}

int get_projectile_his_plyr_num(void) {
    struct ProjectileScriptPdata* pdata = (struct ProjectileScriptPdata*)apdata;
    PlyrPdata* opponent;

    if (pdata == 0) {
        return 3;
    }
    opponent = pdata->opponent;
    if (opponent != 0) {
        return opponent->plyr_num;
    }
    return 3;
}

int get_projectile_script_plyr_num(void) {
    struct ProjectileScriptPdata* pdata = (struct ProjectileScriptPdata*)apdata;
    PlyrPdata* owner;

    if (pdata == 0) {
        return 3;
    }
    owner = pdata->owner;
    if (owner != 0) {
        return owner->plyr_num;
    }
    return 3;
}

static float p_proj_end_run_script(void) {
    struct ProjectileScriptPdata* pdata =
        (struct ProjectileScriptPdata*)pdata_of_proc(aproc);

    if (pdata->script_index == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(
        pdata->owner->cmo, pdata->script_index);
    cmdscript_execute(pdata->owner->cmo);
    return -1.0f;
}
