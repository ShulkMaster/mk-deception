#include "game/game_info.h"
#include "game/projectile.h"
#include "game/jmt.h"
#include "math/gxVect.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "runtime/asset.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/light.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"

/*
 * Common projectile process data. Fields are named incrementally as the
 * projectile TU is lifted; keeping one typed layout avoids repeating raw
 * offsets across the script-facing setup functions.
 */
typedef struct ProjectilePdata {
    MkHdr hdr;
    MkProc* player_process;                 /* +0x08 */
    unsigned int player_process_instance;   /* +0x0C */
    union {
        unsigned int end_script_index;
        PlyrPdata* impaled_target;
        struct ProjectilePdata* retarget_source;
    }; /* +0x10 */
    MkObj* retarget_object;              /* +0x14 */
    MkProc* process;                    /* +0x18 */
    unsigned int process_instance;      /* +0x1C */
    MkObj* source_object;               /* +0x20 */
    unsigned int source_object_instance; /* +0x24 */
    MkObj* object;                      /* +0x28 */
    unsigned int object_instance;       /* +0x2C */
    float max_ticks;     /* +0x30 */
    unsigned int hit_script_index;      /* +0x34 */
    unsigned int end_callback_index;    /* +0x38 */
    unsigned int block_script_index;    /* +0x3C */
    unsigned int ground_script_index;   /* +0x40 */
    int reaction;        /* +0x44 */
    float reaction_scale; /* +0x48 */
    int reaction_flags;  /* +0x4C */
    float collision_radius;             /* +0x50 */
    float collision_height;             /* +0x54 */
    float collision_depth;              /* +0x58 */
    float ground_collision_ticks;       /* +0x5C */
    float ground_height;                /* +0x60 */
    Vec velocity_damping;               /* +0x64 */
    Vec target_position; /* +0x70 */
    ProjectileImpaleInfo* impale_info;   /* +0x7C */
    Vec random_position;                /* +0x80 */
    Vec random_rotation;                /* +0x8C */
    int sound_handle;                   /* +0x98 */
    MkObj* tracking_light;              /* +0x9C */
    unsigned int tracking_light_instance; /* +0xA0 */
    int flight_sound;                   /* +0xA4 */
    int impact_sound;                   /* +0xA8 */
    int down_sound;      /* +0xAC */
    union {
        unsigned char setup_flags;      /* +0xB0 */
        struct {
            unsigned char impale_info_set : 1;
            unsigned char random_position_set : 1;
            unsigned char random_rotation_set : 1;
            unsigned char collision_info_set : 1;
            unsigned char hit_script_set : 1;
            unsigned char end_script_set : 1;
            unsigned char block_script_set : 1;
            unsigned char ground_script_set : 1;
        } setup_bits;
    };
    union {
        unsigned char behavior_flags;   /* +0xB1 */
        struct {
            unsigned char velocity_damping_set : 1;
            unsigned char not_duckable : 1;
            unsigned char track_2d : 1;
            unsigned char track_3d : 1;
            unsigned char continue_through_hit : 1;
            unsigned char behavior_unknown_2_0 : 3;
        } behavior_bits;
    };
} ProjectilePdata;

typedef struct ProjectileScriptPdata {
    MkHdr hdr;
    PlyrPdata* owner;
    PlyrPdata* opponent;
    union {
        PlyrPdata* target;
        unsigned int script_index;
    };
    Vec last_position;
    Vec velocity;
} ProjectileScriptPdata;

typedef struct ProjectileProcVtable {
    void* reserved[9];
    float (*jump_sleep)(MkProcEntryFn entry, float ticks);
} ProjectileProcVtable;

typedef struct ProjectileFollowerPdata {
    MkHdr hdr;
    ProjectilePdata* projectile;
    unsigned int projectile_instance;
} ProjectileFollowerPdata;

typedef union ProjectileFloatBits {
    float f;
    unsigned int u;
} ProjectileFloatBits;

typedef struct ProjectileBoneMatcher {
    MkHdr hdr;
    unsigned char flags_08;
    char pad09[0x13];
    Vec parent_offset;
    char pad28[0x14];
    Vec child_offset;
} ProjectileBoneMatcher;

static ProjectilePdata* proj_pdata;

void set_active_projectile_target_ground(
    float ticks, float collision_ticks, float collision_radius);
void set_active_projectile_upward_attack(const Vec* target);
int get_bid_with_flip(MkObj* object, unsigned int bone_id);
void active_projectile_setup_done(void);
void set_active_projectile_velocity_damp(const Vec* damping);
void set_active_projectile_max_ticks(int ticks);
void set_active_projectile_target_pos(const Vec* position);
void set_active_projectile_p_handler(MkProcEntryFn handler);
void set_active_projectile_velocity_to_hit_gnd(float ticks);
void set_active_projectile_dn_sound(int sound);
void set_active_projectile_sound(
    int start_sound, int flight_sound, int impact_sound);
void set_active_projectile_velocity(const Vec* velocity);
void set_active_add_ang_y(float angle);
void set_active_projectile_hit_gnd_script(unsigned int script_index);
void set_active_projectile_end_script(unsigned int script_index);
void set_active_projectile_block_script(unsigned int script_index);
void set_active_projectile_hit_script(unsigned int script_index);
void set_active_projectile_collision_info(
    int enabled, float radius, float height, float depth);
void set_active_projectile_random_rot(float x, float y, float z);
void set_active_projectile_random_pos(float x, float y, float z);
void set_active_projectile_impale_info(
    ProjectileImpaleInfo* info, const int* bone_tags);
void set_active_projectile_continue_thru_hit(void);
void set_active_projectile_3d_track(void);
MkObj* set_active_projectile_tracking_light(LightDef* definition);
static float p_point_light_follower(void);
void set_active_projectile_2d_track(void);
void set_active_projectile_not_duckable(void);
void set_active_projectile_rx_info(
    int reaction, int flags, float scale);
static MkObj* start_projectile_from_specific_plyr_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset,
    int use_sidekick);
MkObj* start_projectile_from_sidekick_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset);
MkObj* start_projectile_from_plyr_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset);
static void ps_projectile(void);
static void pw_projectile(void);
void retarget_projectile(ProjectilePdata* pdata);
static void projectile_set_velocity_angy_tol(
    MkObj* object, float speed, float tolerance);
static void projectile_impale(ProjectilePdata* pdata, MkObj* victim);
static float p_projectile_handler(void);
static float p_projectile_continue(void);
static float p_ground_target(void);
static float p_ground_target_collide(void);
int check_for_throw(PlyrPdata* player);
static float p_projectile_launch_upward(void);
static float p_projectile_downward(void);
static float p_projectile_impaled(void);
float p_projectile_die(void);
void get_projectile_script_velocity(Vec* velocity);
void get_projectile_script_last_pos(Vec* position);
PlyrPdata* get_projectile_script_plyr_pdata(void);
int get_projectile_his_plyr_num(void);
int get_projectile_script_plyr_num(void);
static float p_proj_end_run_script(void);

int snd_req(int sound);
int build_bones_tbl(MkObj* object, const int* tags);
extern float game_speed;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern GameInfo g_game_info;
extern unsigned short GXMathSqrtTable[];
extern void trial_state_collision_check(
    int collision_result, int player);
extern int collide_cylinder_vs_plyr(
    PlyrInfo* player, const Vec* center, const Vec* angles,
    float radius, float height);
extern int reaction_xfer_him(
    int reaction, float damage_scale, int block_type);
extern int collide_sphere_vs_plyr(
    PlyrInfo* player, const Vec* center, float radius);
extern void pz_fighter_reaction_xfer_him(int reaction);
extern int mode_of_play;
extern ProjectileBoneMatcher* start_bone_matcher(
    float blend_ticks, MkObj* parent, int parent_bone,
    MkObj* child, int child_bone);
extern void obj_set_all_sobjs_priority(MkObj* object, int priority);
extern void get_bone_offset_world_pos(
    MkObj* object, int bone, const Vec* offset, Vec* out);

static const Vec projectile_ground_collision_angles = {
    -1.57079637f, 0.0f, 0.0f
};

static inline float projectile_fast_inverse_sqrt(float squared) {
    ProjectileFloatBits bits;
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

static inline float projectile_fast_sqrt(float squared) {
    ProjectileFloatBits bits;

    if (squared <= 0.0f) {
        return 0.0f;
    }
    bits.f = squared;
    bits.u =
        ((unsigned int)GXMathSqrtTable[(bits.u >> 10) & 0x3FFE] << 8) |
        ((((bits.u & 0x7F800000) + 0x3F800000) >> 1) & 0x7F800000);
    return 0.5f * (bits.f * (3.0f - (bits.f * bits.f) / squared));
}

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
        process = proj_pdata->process;
        if (process != 0) {
            if ((unsigned int)process->instance ==
                proj_pdata->process_instance) {
                /* The instance latch still identifies this process. */
            } else {
                process = 0;
            }
        } else {
            process = 0;
        }
        if (process != 0) {
            xfer_proc(process, handler);
        }
    }
}

static inline MkProc* projectile_start_end_script(
    PlyrPdata* owner, PlyrPdata* opponent,
    unsigned int script_index) {
    ProjectileScriptPdata* script_data;
    MkProc* process;

    script_data = 0;
    process = _create_mkproc_generic_tinystack(
        0xB00A, 0x1F, p_proj_end_run_script,
        sizeof(ProjectileScriptPdata), (MkHdr**)&script_data);
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
    ProjectilePdata* projectile, unsigned int script_index) {
    ProjectileScriptPdata* script_data;
    PlyrPdata* owner;
    MkObj* source;
    MkObj* object;
    MkProc* process;

    source = projectile->source_object;
    if (source != 0 &&
        source->hdr.instance != projectile->source_object_instance) {
        source = 0;
    }
    if (source == 0) {
        return 0;
    }
    if (source == g_game_info.plyr0.slot.mirror_a) {
        owner = g_game_info.plyr0.slot.pdata;
    } else {
        owner = g_game_info.plyr1.slot.pdata;
    }
    process = projectile_start_end_script(
        owner, projectile->impaled_target, script_index);
    if (process == 0) {
        return 0;
    }

    script_data = (ProjectileScriptPdata*)pdata_of_proc(process);
    object = projectile->object;
    if (object != 0 &&
        object->hdr.instance != projectile->object_instance) {
        object = 0;
    }
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

void set_active_projectile_upward_attack(const Vec* target) {
    projectile_set_process_handler(p_projectile_launch_upward);
    projectile_set_target_position(target);
}

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

void set_active_projectile_velocity_damp(const Vec* damping) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_bits.velocity_damping_set = 1;
        proj_pdata->velocity_damping.x = damping->x;
        proj_pdata->velocity_damping.y = damping->y;
        proj_pdata->velocity_damping.z = damping->z;
    }
}

void set_active_projectile_max_ticks(int ticks) {
    if (proj_pdata != 0) {
        proj_pdata->max_ticks = (float)ticks;
    }
}

void set_active_projectile_target_pos(const Vec* position) {
    projectile_set_target_position(position);
}

void set_active_projectile_p_handler(MkProcEntryFn handler) {
    projectile_set_process_handler(handler);
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
    object = proj_pdata->object;
    if (object == 0 ||
        object->hdr.instance != proj_pdata->object_instance) {
        return;
    }

    object->flags_08 |= 0x20;
    speed = projectile_fast_sqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.y * object->pos_vel.y +
        object->pos_vel.z * object->pos_vel.z);
    horizontal_inverse_length = projectile_fast_inverse_sqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.z * object->pos_vel.z);
    target_x = object->pos_vel.x * horizontal_inverse_length * ticks;
    target_z = object->pos_vel.z * horizontal_inverse_length * ticks;
    target_y = g_game_info.field_34 - object->pos.value.y;
    if (target_y > 0.0f) {
        target_y = 0.0f;
    }
    target_inverse_length = projectile_fast_inverse_sqrt(
        target_x * target_x + target_y * target_y + target_z * target_z);
    object->pos_vel.x = target_x * target_inverse_length * speed;
    object->pos_vel.y = target_y * target_inverse_length * speed;
    object->pos_vel.z = target_z * target_inverse_length * speed;

    if (object->pos_vel.x == 0.0f && object->pos_vel.y == 0.0f) {
        object->ang.y = plyr_obj->ang.y;
        return;
    }
    angle_inverse_length = projectile_fast_inverse_sqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.z * object->pos_vel.z);
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

void set_active_projectile_velocity(const Vec* velocity) {
    MkObj* object;
    float inverse_length;

    if (proj_pdata != 0) {
        object = proj_pdata->object;
        if (object != 0) {
            if ((unsigned int)object->hdr.instance ==
                proj_pdata->object_instance) {
                /* The instance latch still identifies this object. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            object->flags_08 |= 0x20;
            object->pos_vel = *velocity;
            if (object->pos_vel.x == 0.0f &&
                object->pos_vel.y == 0.0f) {
                object->ang.y = plyr_obj->ang.y;
                return;
            }
            inverse_length = projectile_fast_inverse_sqrt(
                object->pos_vel.x * object->pos_vel.x +
                object->pos_vel.z * object->pos_vel.z);
            object->ang.y = gxMathArcTanYX(
                object->pos_vel.x * inverse_length,
                object->pos_vel.z * inverse_length);
        }
    }
}

static inline MkObj* projectile_live_object(ProjectilePdata* owner) {
    MkObj* object = owner->object;
    if (object != 0) {
        if ((unsigned int)object->hdr.instance == owner->object_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

void set_active_add_ang_y(float angle) {
    MkObj* object;
    int fixed;

    if (proj_pdata != 0) {
        object = projectile_live_object(proj_pdata);

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
        proj_pdata->setup_bits.ground_script_set = 1;
    }
}

void set_active_projectile_end_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->end_callback_index = script_index;
        proj_pdata->setup_bits.end_script_set = 1;
    }
}

void set_active_projectile_block_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->block_script_index = script_index;
        proj_pdata->setup_bits.block_script_set = 1;
    }
}

void set_active_projectile_hit_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->hit_script_index = script_index;
        proj_pdata->setup_bits.hit_script_set = 1;
    }
}

void set_active_projectile_collision_info(
    int enabled, float radius, float height, float depth) {
    if (proj_pdata != 0) {
        if (enabled != 0) {
            proj_pdata->setup_bits.collision_info_set = 1;
        } else {
            proj_pdata->setup_bits.collision_info_set = 0;
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
        proj_pdata->setup_bits.random_rotation_set = 1;
    }
}

void set_active_projectile_random_pos(float x, float y, float z) {
    if (proj_pdata != 0) {
        proj_pdata->random_position.x = x;
        proj_pdata->random_position.y = y;
        proj_pdata->random_position.z = z;
        proj_pdata->setup_bits.random_position_set = 1;
    }
}

void set_active_projectile_impale_info(
    ProjectileImpaleInfo* info, const int* bone_tags) {
    MkObj* object;

    if (proj_pdata != 0) {
        object = projectile_live_object(proj_pdata);

        if (object != 0) {
            build_bones_tbl(object, bone_tags);
            proj_pdata->impale_info = info;
            proj_pdata->setup_bits.impale_info_set = 1;
        }
    }
}

void set_active_projectile_continue_thru_hit(void) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_bits.continue_through_hit = 1;
    }
}

void set_active_projectile_3d_track(void) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_bits.track_3d = 1;
    }
}

MkObj* set_active_projectile_tracking_light(LightDef* definition) {
    ProjectileFollowerPdata* follower;
    MkHdr* raw_pdata;
    MkObj* light;
    MkProc* process;

    if (proj_pdata != 0) {
        light = load_light(definition, &point_light_list, 0);
        if (light != 0) {
            process = _create_mkproc_generic_tinystack(
                0x2026, 0x1F, p_point_light_follower,
                sizeof(ProjectileFollowerPdata), &raw_pdata);
            if (process == 0) {
                if (light->hdr.instance != 0) {
                    ((void (*)(MkHdr*))light->hdr.vtbl->destroy)(
                        &light->hdr);
                }
                return 0;
            }

            proj_pdata->tracking_light = light;
            proj_pdata->tracking_light_instance = light->hdr.instance;
            follower = (ProjectileFollowerPdata*)raw_pdata;
            follower->projectile = proj_pdata;
            follower->projectile_instance = proj_pdata->hdr.instance;
            mk_insert(&light->hdr, &process->pdata_list);
            return light;
        }
    }
    return 0;
}

static float p_point_light_follower(void) {
    ProjectileFollowerPdata* follower =
        (ProjectileFollowerPdata*)apdata;
    ProjectilePdata* projectile = follower->projectile;
    MkObj* object;
    MkObj* light;

    if (projectile != 0) {
        if (projectile->hdr.instance == follower->projectile_instance) {
            /* The instance latch still identifies this projectile. */
        } else {
            projectile = 0;
        }
    } else {
        projectile = 0;
    }
    if (projectile != 0) {
        object = projectile->object;
        if (object != 0) {
            if ((unsigned int)object->hdr.instance ==
                projectile->object_instance) {
                /* The instance latch still identifies this object. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            light = projectile->tracking_light;
            if (light != 0) {
                if ((unsigned int)light->hdr.instance ==
                    projectile->tracking_light_instance) {
                    /* The instance latch still identifies this light. */
                } else {
                    light = 0;
                }
            } else {
                light = 0;
            }
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
        proj_pdata->behavior_bits.track_2d = 1;
    }
}

void set_active_projectile_not_duckable(void) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_bits.not_duckable = 1;
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

static MkObj* start_projectile_from_specific_plyr_bone(
    int bone_id, MkObj* existing_object, const char* model_name,
    float speed, float tolerance, const Vec* bone_offset,
    int use_sidekick) {
    ProjectilePdata* projectile;
    MkObj* object;
    MkObj* launch_object;
    MkProc* process;
    Vec position;
    float inverse_length;
    int player;

    proj_pdata = 0;
    if (plyr_obj == 0) {
        return 0;
    }

    object = existing_object;
    if (object == 0) {
        player = get_player_number(plyr_obj);
        if (model_name == 0) {
            object = get_mkobj_frame(0xD000, 0);
        } else {
            object = load_named_model_for_player(
                model_name, player, 0xD000, 0);
        }
        if (object != 0) {
            insert_fgnd_mkobj(object);
        }
        if (object == 0) {
            return 0;
        }
        obj_set_all_sobjs_priority(object, 0x13);
    }

    object->hide_flags |= 0x20;
    object->flags_08 |= 8;
    object->flags_08 |= 0x20;
    projectile = 0;
    process = _create_mkproc_generic_tinystack(
        0x2026, 0x1F, p_projectile_handler,
        sizeof(ProjectilePdata), (MkHdr**)&projectile);
    if (process == 0) {
        if (object != existing_object && object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy((MkHdr*)object);
        }
        return 0;
    }

    projectile->process = process;
    projectile->process_instance = process->instance;
    process->pre_destroy = pw_projectile;
    process->destroy_cb = ps_projectile;
    if (object != existing_object) {
        mk_insert((MkHdr*)object, &process->pdata_list);
    }

    launch_object = plyr_obj;
    if (use_sidekick) {
        launch_object = plyr_pdata->sidekick_obj;
        if (launch_object != 0 &&
            launch_object->hdr.instance != plyr_pdata->sidekick_instance) {
            launch_object = 0;
        }
        if (launch_object == 0) {
            launch_object = plyr_obj;
        }
    }
    get_bone_offset_world_pos(
        launch_object, bone_id, bone_offset, &position);
    object->pos.value.x = position.x;
    object->pos.value.y = position.y;
    object->pos.value.z = position.z;
    projectile_set_velocity_angy_tol(
        object, get_adjusted_speed(speed, 0.8f), tolerance);
    if (object->pos_vel.x == 0.0f && object->pos_vel.y == 0.0f) {
        object->ang.y = plyr_obj->ang.y;
    } else {
        inverse_length = projectile_fast_inverse_sqrt(
            object->pos_vel.x * object->pos_vel.x +
            object->pos_vel.z * object->pos_vel.z);
        object->ang.y = gxMathArcTanYX(
            object->pos_vel.x * inverse_length,
            object->pos_vel.z * inverse_length);
    }
    if (model_name != 0 || existing_object != 0) {
        update_mkobj(object);
    }
    object->hide_flags &= (unsigned char)~0x20;

    projectile->source_object = plyr_obj;
    projectile->source_object_instance = plyr_obj->hdr.instance;
    projectile->object = object;
    projectile->object_instance = object->hdr.instance;
    projectile->player_process = plyr_pdata->player_proc;
    projectile->player_process_instance = plyr_pdata->player_proc_instance;
    projectile->impaled_target = plyr_pdata->his_plyr_pdata;
    projectile->retarget_object = plyr_pdata->his_obj;
    projectile->impaled_target->his_plyr_pdata->duck_reaction_active = 0;
    projectile->setup_flags = 0;
    projectile->behavior_flags = 0;
    projectile->max_ticks = 240.0f;
    projectile->reaction = -1;
    projectile->reaction_scale = 0.0f;
    projectile->reaction_flags = 0;
    projectile->collision_radius = 0.2f;
    projectile->collision_height = 0.0f;
    projectile->collision_depth = 100.0f;
    projectile->impale_info = 0;
    projectile->random_position.x = 0.0f;
    projectile->random_position.y = 0.0f;
    projectile->random_position.z = 0.0f;
    projectile->random_rotation.x = 0.0f;
    projectile->random_rotation.y = 0.0f;
    projectile->random_rotation.z = 0.0f;
    projectile->target_position.x = 0.0f;
    projectile->target_position.y = 0.0f;
    projectile->target_position.z = 0.0f;
    projectile->velocity_damping.x = 0.0f;
    projectile->velocity_damping.y = 0.0f;
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
    proj_pdata = projectile;
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
    proj_pdata = (ProjectilePdata*)pdata_of_proc(aproc);
}

void retarget_projectile(ProjectilePdata* pdata) {
    ProjectilePdata* source;
    MkObj* object;
    float speed;
    float dx;
    float dz;
    float inverse_length;

    if (pdata == 0) {
        return;
    }
    object = pdata->object;
    source = pdata->retarget_source;
    if (object == 0 || object->hdr.instance != pdata->object_instance) {
        return;
    }

    pdata->player_process = source->player_process;
    pdata->player_process_instance = source->player_process_instance;
    pdata->retarget_source = source->retarget_source;
    pdata->retarget_object = source->retarget_object;

    speed = projectile_fast_sqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.y * object->pos_vel.y +
        object->pos_vel.z * object->pos_vel.z);
    dx = pdata->retarget_object->pos.value.x - object->pos.value.x;
    dz = pdata->retarget_object->pos.value.z - object->pos.value.z;
    inverse_length = projectile_fast_inverse_sqrt(dx * dx + dz * dz);
    object->pos_vel.x = dx * inverse_length;
    object->pos_vel.z = dz * inverse_length;
    object->pos_vel.y = 0.0f;
    object->pos_vel.x *= speed;
    object->pos_vel.y *= speed;
    object->pos_vel.z *= speed;

    if (object->pos_vel.x == 0.0f && object->pos_vel.y == 0.0f) {
        object->ang.y = plyr_obj->ang.y;
    } else {
        inverse_length = projectile_fast_inverse_sqrt(
            object->pos_vel.x * object->pos_vel.x +
            object->pos_vel.z * object->pos_vel.z);
        object->ang.y = gxMathArcTanYX(
            object->pos_vel.x * inverse_length,
            object->pos_vel.z * inverse_length);
    }
    pdata->max_ticks = 300.0f;
}

static void projectile_set_velocity_angy_tol(
    MkObj* object, float speed, float tolerance) {
    float cone_cos;
    float dx;
    float dz;
    float inverse_length;
    float direction_x;
    float direction_z;
    float forward_x;
    float forward_z;
    float dot;
    float side;
    float side_x;
    float side_z;
    float cone_sin;

    cone_cos = gxMathCos((3.1415f * tolerance) / 360.0f);
    dx = his_obj->pos.value.x - object->pos.value.x;
    dz = his_obj->pos.value.z - object->pos.value.z;
    inverse_length = projectile_fast_inverse_sqrt(dx * dx + dz * dz);
    direction_x = dx * inverse_length;
    direction_z = dz * inverse_length;
    forward_x = gxMathSin(plyr_obj->ang.y);
    forward_z = gxMathCos(plyr_obj->ang.y);
    dot = forward_x * direction_x + forward_z * direction_z;

    if (dot > cone_cos) {
        object->pos_vel.x = direction_x * speed;
        object->pos_vel.y = 0.0f * speed;
        object->pos_vel.z = direction_z * speed;
        return;
    }

    cone_sin = projectile_fast_sqrt(1.0f - cone_cos * cone_cos);
    side = forward_z * direction_x - forward_x * direction_z;
    side_x = side * forward_z;
    side_z = -(side * forward_x);
    inverse_length = projectile_fast_inverse_sqrt(
        side_x * side_x + side_z * side_z);
    object->pos_vel.x = side_x * inverse_length * cone_sin;
    object->pos_vel.z = side_z * inverse_length * cone_sin;
    object->pos_vel.x += forward_x * cone_cos;
    object->pos_vel.z += forward_z * cone_cos;
    inverse_length = projectile_fast_inverse_sqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.z * object->pos_vel.z);
    object->pos_vel.x *= inverse_length;
    object->pos_vel.z *= inverse_length;
    object->pos_vel.x *= speed;
    object->pos_vel.y *= speed;
    object->pos_vel.z *= speed;
}

static void projectile_impale(ProjectilePdata* pdata, MkObj* victim) {
    ProjectileImpaleInfo* info = pdata->impale_info;
    ProjectileBoneMatcher* matcher;
    MkBone* victim_bone;
    Vec angles;
    float random_x;
    float random_y;
    float random_z;

    if (info == 0) {
        return;
    }

    matcher = start_bone_matcher(
        0.0f, pdata->retarget_object, info->parent_bone,
        victim, info->child_bone);
    if (matcher == 0) {
        return;
    }

    matcher->flags_08 |= 0x40;
    matcher->flags_08 |= 0x10;
    matcher->child_offset.x = info->child_offset.x;
    matcher->child_offset.y = info->child_offset.y;
    matcher->child_offset.z = info->child_offset.z;

    if (pdata->setup_bits.random_position_set) {
        random_x = frand(pdata->random_position.x) -
                   0.5f * pdata->random_position.x;
        random_y = frand(pdata->random_position.y) -
                   0.5f * pdata->random_position.y;
        random_z = frand(pdata->random_position.z) -
                   0.5f * pdata->random_position.z;
        matcher->parent_offset.x = info->parent_offset.x + random_x;
        matcher->parent_offset.y = info->parent_offset.y + random_y;
        matcher->parent_offset.z = info->parent_offset.z + random_z;
    } else {
        matcher->parent_offset.x = info->parent_offset.x;
        matcher->parent_offset.y = info->parent_offset.y;
        matcher->parent_offset.z = info->parent_offset.z;
    }

    victim_bone = victim->bones[victim->fallback_bone_index];
    if (victim_bone == 0) {
        if (matcher->hdr.instance != 0) {
            matcher->hdr.typed_vtbl->destroy((MkHdr*)matcher);
        }
        return;
    }

    victim_bone->flags_54_bits.pose_matrix_applied = 1;
    if (pdata->setup_bits.random_rotation_set) {
        random_x = frand(pdata->random_rotation.x) -
                   0.5f * pdata->random_rotation.x;
        random_y = frand(pdata->random_rotation.y) -
                   0.5f * pdata->random_rotation.y;
        random_z = frand(pdata->random_rotation.z) -
                   0.5f * pdata->random_rotation.z;
        angles.x = info->rotation.x + random_x;
        angles.y = info->rotation.y + random_y;
        angles.z = info->rotation.z + random_z;
        YXZ_angles_to_MKMATRIX(&angles, victim_bone->parent_matrix);
    } else {
        YXZ_angles_to_MKMATRIX(&info->rotation, victim_bone->parent_matrix);
    }

    pdata->max_ticks = 1800.0f;
    victim->flags_08 &= (unsigned char)~8;
    victim->flags_08 &= (unsigned char)~0x20;
}

static float p_projectile_handler(void) {
    ProjectilePdata* projectile = proj_pdata;
    PlyrPdata* owner;
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
    int player;

    object = projectile->object;
    if (object != 0 &&
        object->hdr.instance != projectile->object_instance) {
        object = 0;
    }
    if (object == 0) {
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }
    projectile->max_ticks -= game_speed;
    if (projectile->max_ticks < 0.0f) {
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    if (projectile->behavior_bits.velocity_damping_set) {
        object->pos_vel.x *= projectile->velocity_damping.x;
        object->pos_vel.y *= projectile->velocity_damping.y;
        object->pos_vel.z *= projectile->velocity_damping.z;
    }
    if (projectile->behavior_bits.track_2d) {
        get_bone_world_pos(
            projectile->retarget_object, 0x10, &bone_position);
        dx = projectile->retarget_object->pos.value.x - object->pos.value.x;
        dz = projectile->retarget_object->pos.value.z - object->pos.value.z;
        if (dx * dx + dz * dz < 3.0f) {
            speed = projectile_fast_sqrt(
                object->pos_vel.x * object->pos_vel.x +
                object->pos_vel.y * object->pos_vel.y +
                object->pos_vel.z * object->pos_vel.z);
            object->pos_vel.y = (bone_position.y - object->pos.value.y) / 5.0f;
            inverse_length = projectile_fast_inverse_sqrt(
                object->pos_vel.x * object->pos_vel.x +
                object->pos_vel.y * object->pos_vel.y +
                object->pos_vel.z * object->pos_vel.z);
            object->pos_vel.x *= inverse_length;
            object->pos_vel.y *= inverse_length;
            object->pos_vel.z *= inverse_length;
            object->pos_vel.x *= speed;
            object->pos_vel.y *= speed;
            object->pos_vel.z *= speed;
        }
    }
    if (projectile->behavior_bits.track_3d) {
        get_bone_world_pos(
            projectile->retarget_object, 0x10, &bone_position);
        dx = projectile->retarget_object->pos.value.x - object->pos.value.x;
        dz = projectile->retarget_object->pos.value.z - object->pos.value.z;
        distance_squared = dx * dx + dz * dz;
        if (distance_squared < 0.25f) {
            projectile->behavior_bits.track_3d = 0;
        } else if (distance_squared < 3.0f) {
            speed = projectile_fast_sqrt(
                object->pos_vel.x * object->pos_vel.x +
                object->pos_vel.y * object->pos_vel.y +
                object->pos_vel.z * object->pos_vel.z);
            object->pos_vel.x = dx;
            object->pos_vel.z = dz;
            inverse_length = projectile_fast_inverse_sqrt(
                object->pos_vel.x * object->pos_vel.x +
                object->pos_vel.y * object->pos_vel.y +
                object->pos_vel.z * object->pos_vel.z);
            object->pos_vel.x *= inverse_length;
            object->pos_vel.y *= inverse_length;
            object->pos_vel.z *= inverse_length;
            object->pos_vel.x *= speed;
            object->pos_vel.y *= speed;
            object->pos_vel.z *= speed;
        }
    }

    owner = projectile->impaled_target;
    victim = owner->his_plyr_pdata;
    target = owner->his_obj;
    collision = simple_3d_projectile_collision(
        &target->pos.value, &projectile->retarget_object->pos.value, &object->pos.value,
        projectile->setup_bits.collision_info_set != 0,
        projectile->collision_radius, projectile->collision_depth,
        projectile->collision_height);
    victim->duck_reaction_active = 1;
    victim->saved_position_x = object->pos.value.x;
    victim->saved_position_y = object->pos.value.y;
    victim->saved_position_z = object->pos.value.z;
    player = target == g_game_info.plyr0.slot.mirror_a;

    if (object->pos.value.y < 0.2f + g_game_info.field_34) {
        trial_state_collision_check(0, player);
        victim->duck_reaction_active = 0;
        if (projectile->sound_handle != 0) {
            snd_stop(projectile->sound_handle);
            projectile->sound_handle = 0;
        }
        if (projectile->setup_bits.ground_script_set) {
            projectile_start_script_snapshot(
                projectile, projectile->ground_script_index);
        }
        if (projectile->flight_sound != 0) {
            snd_req(projectile->flight_sound);
        }
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    if (collision == 0 && !projectile->behavior_bits.not_duckable &&
        (owner->state == 0x101 || owner->state == 0x302 ||
         owner->state == 0x900 || owner->state == 0x1300) &&
        projectile->reaction_flags != 1) {
        collision = 4;
    }
    if (victim->state_flags.bits.projectile_invulnerable && collision == 0) {
        collision = 1;
    } else if (collision == 0 &&
               collide_sphere_vs_plyr(
                   owner->plyr_info, &object->pos.value,
                   projectile->collision_radius) == 0) {
        collision = 4;
    }
    switch (collision) {
    case 1:
    case 2:
        trial_state_collision_check(0, player);
        break;
    case 0:
        trial_state_collision_check(1, player);
        break;
    }

    switch (collision) {
    case 0:
        if (owner->state == 0x1222) {
            retarget_projectile(projectile);
            return 1.0f;
        }
        projectile->behavior_bits.track_2d = 0;
        projectile->behavior_bits.track_3d = 0;
        victim->duck_reaction_active = 0;
        if (projectile->sound_handle != 0) {
            snd_stop(projectile->sound_handle);
            projectile->sound_handle = 0;
        }
        if (projectile->setup_bits.hit_script_set) {
            projectile_start_script_snapshot(
                projectile, projectile->hit_script_index);
        }

        collision_result = 0;
        if (projectile->reaction != -1) {
            if (mode_of_play != 6) {
                reaction_xfer_him(
                    projectile->reaction, projectile->reaction_scale,
                    projectile->reaction_flags);
            } else {
                pz_fighter_reaction_xfer_him(projectile->reaction);
            }
            collision_result = victim->collision_result;
        }
        impale = collision_result == 1 &&
                 projectile->setup_bits.impale_info_set &&
                 victim->his_plyr_pdata->impaled_projectile_state < 3;
        if (collision_result == 1) {
            if (projectile->flight_sound != 0) {
                snd_req(projectile->flight_sound);
            }
        } else if (collision_result == 2) {
            if (projectile->impact_sound != 0) {
                snd_req(projectile->impact_sound);
            }
            victim->duck_reaction_active = 0;
            if (projectile->sound_handle != 0) {
                snd_stop(projectile->sound_handle);
                projectile->sound_handle = 0;
            }
            if (projectile->setup_bits.block_script_set) {
                projectile_start_script_snapshot(
                    projectile, projectile->block_script_index);
            }
        }
        if (impale) {
            projectile_impale(projectile, object);
            victim->his_plyr_pdata->impaled_projectile_state++;
            ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
                p_projectile_impaled, 0.0f);
            return 0.0f;
        }
        if (projectile->behavior_bits.continue_through_hit) {
            ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
                p_projectile_continue, 0.0f);
            return 0.0f;
        }
        break;
    case 4:
        return 1.0f;
    case 1:
        victim->duck_reaction_active = 0;
        return 1.0f;
    }

    ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
        p_projectile_die, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: the three-way collision result mapping and terminal transfer
 * are recovered directly from retail; residue is latch/register scheduling.
 */
static float p_projectile_continue(void) {
    MkObj* object;
    MkObj* target;
    int collision;
    int player;

    object = proj_pdata->object;
    if (object != 0 &&
        object->hdr.instance != proj_pdata->object_instance) {
        object = 0;
    }
    if (object != 0) {
        target = proj_pdata->impaled_target->his_obj;
        collision = simple_3d_projectile_collision(
            &target->pos.value, &proj_pdata->retarget_object->pos.value,
            &object->pos.value,
            proj_pdata->setup_bits.collision_info_set != 0,
            proj_pdata->collision_radius,
            proj_pdata->collision_depth,
            proj_pdata->collision_height);
        player = target == g_game_info.plyr0.slot.mirror_a;
        switch (collision) {
        case 1:
        case 2:
            trial_state_collision_check(0, player);
            break;
        case 0:
            trial_state_collision_check(1, player);
            break;
        }
        if (collision != 2) {
            return 1.0f;
        }
    }
    ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
        p_projectile_die, 0.0f);
    return 0.0f;
}

static float p_ground_target(void) {
    ProjectilePdata* projectile = proj_pdata;
    ProjectileScriptPdata* script_data;
    PlyrPdata* target;
    PlyrPdata* owner;
    MkObj* object;
    MkObj* source;
    MkProc* process;

    object = projectile->object;
    if (object != 0 &&
        object->hdr.instance != projectile->object_instance) {
        object = 0;
    }
    if (object == 0) {
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    projectile->max_ticks -= game_speed;
    if (projectile->max_ticks < 0.0f) {
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    target = projectile->impaled_target->his_plyr_pdata;
    target->duck_reaction_active = 1;
    target->saved_position_x = object->pos.value.x;
    target->saved_position_y = object->pos.value.y;
    target->saved_position_z = object->pos.value.z;
    if (object->pos.value.y >= g_game_info.field_34) {
        return 1.0f;
    }

    if (projectile->flight_sound != 0) {
        snd_req(projectile->flight_sound);
    }
    target->duck_reaction_active = 0;
    if (projectile->sound_handle != 0) {
        snd_stop(projectile->sound_handle);
        projectile->sound_handle = 0;
    }

    process = 0;
    if (projectile->setup_bits.ground_script_set) {
        source = projectile->source_object;
        if (source != 0 &&
            source->hdr.instance != projectile->source_object_instance) {
            source = 0;
        }
        if (source != 0) {
            if (source == g_game_info.plyr0.slot.mirror_a) {
                owner = g_game_info.plyr0.slot.pdata;
            } else {
                owner = g_game_info.plyr1.slot.pdata;
            }
            process = projectile_start_end_script(
                owner, projectile->impaled_target,
                projectile->ground_script_index);
        }
    }
    if (process != 0) {
        script_data = (ProjectileScriptPdata*)pdata_of_proc(process);
        object = projectile->object;
        if (object != 0 &&
            object->hdr.instance != projectile->object_instance) {
            object = 0;
        }
        if (script_data != 0 && object != 0) {
            script_data->last_position.x = object->pos.value.x;
            script_data->last_position.y = object->pos.value.y;
            script_data->last_position.z = object->pos.value.z;
            script_data->velocity.x = object->pos_vel.x;
            script_data->velocity.y = object->pos_vel.y;
            script_data->velocity.z = object->pos_vel.z;
        }
    }

    object->pos_vel.z = 0.0f;
    object->pos_vel.y = 0.0f;
    object->pos_vel.x = 0.0f;
    projectile->max_ticks = projectile->ground_collision_ticks;
    ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
        p_ground_target_collide, 0.0f);
    return 0.0f;
}

/*
 * Soft ceiling: retail's ground-target latch, timeout, hold-process gate,
 * cylinder collision, trial result, and reaction transfer are preserved.
 */
static float p_ground_target_collide(void) {
    PlyrPdata* target;
    PlyrInfo* player_info;
    MkObj* object;
    MkProc* hold_proc;

    object = proj_pdata->object;
    if (object != 0 &&
        object->hdr.instance != proj_pdata->object_instance) {
        object = 0;
    }
    if (object == 0) {
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    target = proj_pdata->impaled_target->his_plyr_pdata;
    target->duck_reaction_active = 1;
    target->saved_position_x = object->pos.value.x;
    target->saved_position_y = object->pos.value.y;
    target->saved_position_z = object->pos.value.z;
    player_info = &g_game_info.plyr1;
    if (proj_pdata->impaled_target == g_game_info.plyr0.slot.pdata) {
        player_info = &g_game_info.plyr0;
    }
    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        trial_state_collision_check(0, player_info->controller_slot);
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    hold_proc = target->hold_proc;
    if (hold_proc != 0 &&
        hold_proc->hdr.instance != target->hold_proc_instance) {
        hold_proc = 0;
    }
    if (hold_proc != 0) {
        return 1.0f;
    }
    if (collide_cylinder_vs_plyr(
            player_info, &object->pos.value,
            &projectile_ground_collision_angles,
            proj_pdata->collision_radius, 0.3f) != 0) {
        if (proj_pdata->reaction != -1 &&
            !proj_pdata->impaled_target->state_flags.bits
                 .projectile_invulnerable) {
            trial_state_collision_check(
                1, player_info->controller_slot);
            reaction_xfer_him(
                proj_pdata->reaction, proj_pdata->reaction_scale,
                proj_pdata->reaction_flags);
        }
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

int check_for_throw(PlyrPdata* player) {
    PlyrPdata* target = player->his_plyr_pdata;
    MkProc* hold_proc = target->hold_proc;

    if (hold_proc != 0) {
        if ((unsigned int)hold_proc->instance ==
            target->hold_proc_instance) {
            /* The instance latch still identifies this process. */
        } else {
            hold_proc = 0;
        }
    } else {
        hold_proc = 0;
    }
    if (hold_proc == 0) {
        return 0;
    }
    return 1;
}

static float p_projectile_launch_upward(void) {
    MkObj* object;

    proj_pdata->impaled_target->his_plyr_pdata->duck_reaction_active = 0;
    object = proj_pdata->object;
    if (object == 0 ||
        object->hdr.instance != proj_pdata->object_instance) {
        ((ProjectileProcVtable*)aproc->vtbl)
            ->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }

    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        object->pos.value.x = proj_pdata->target_position.x;
        object->pos.value.z = proj_pdata->target_position.z;
        object->pos_vel.x = -object->pos_vel.x;
        object->pos_vel.y = -object->pos_vel.y;
        object->pos_vel.z = -object->pos_vel.z;
        proj_pdata->max_ticks = 300.0f;
        if (proj_pdata->down_sound != 0) {
            snd_req(proj_pdata->down_sound);
        }
        ((ProjectileProcVtable*)aproc->vtbl)
            ->jump_sleep(p_projectile_downward, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

static float p_projectile_downward(void) {
    ProjectilePdata* projectile = proj_pdata;
    ProjectileScriptPdata* script_data;
    PlyrPdata* victim;
    PlyrPdata* victim_state;
    PlyrPdata* owner;
    MkObj* object;
    MkObj* source;
    MkProc* process;
    Vec target_position;
    float dx;
    float dy;
    float dz;

    object = projectile->object;
    if (object != 0 &&
        object->hdr.instance != projectile->object_instance) {
        object = 0;
    }
    if (object == 0) {
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    victim = projectile->impaled_target;
    victim_state = victim->his_plyr_pdata;
    victim_state->duck_reaction_active = 1;
    victim_state->saved_position_x = object->pos.value.x;
    victim_state->saved_position_y = object->pos.value.y;
    victim_state->saved_position_z = object->pos.value.z;
    projectile->max_ticks -= game_speed;
    if (projectile->max_ticks < 0.0f ||
        projectile->ground_height != g_game_info.field_34) {
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    if (object->pos.value.y < (float)(0.2 + (double)g_game_info.field_34)) {
        trial_state_collision_check(0, victim->plyr_num);
        victim_state->duck_reaction_active = 0;
        if (projectile->sound_handle != 0) {
            snd_stop(projectile->sound_handle);
            projectile->sound_handle = 0;
        }

        process = 0;
        if (projectile->setup_bits.ground_script_set) {
            source = projectile->source_object;
            if (source != 0 &&
                source->hdr.instance != projectile->source_object_instance) {
                source = 0;
            }
            if (source != 0) {
                if (source == g_game_info.plyr0.slot.mirror_a) {
                    owner = g_game_info.plyr0.slot.pdata;
                } else {
                    owner = g_game_info.plyr1.slot.pdata;
                }
                process = projectile_start_end_script(
                    owner, victim, projectile->ground_script_index);
            }
        }
        if (process != 0) {
            script_data = (ProjectileScriptPdata*)pdata_of_proc(process);
            object = projectile->object;
            if (object != 0 &&
                object->hdr.instance != projectile->object_instance) {
                object = 0;
            }
            if (script_data != 0 && object != 0) {
                script_data->last_position.x = object->pos.value.x;
                script_data->last_position.y = object->pos.value.y;
                script_data->last_position.z = object->pos.value.z;
                script_data->velocity.x = object->pos_vel.x;
                script_data->velocity.y = object->pos_vel.y;
                script_data->velocity.z = object->pos_vel.z;
            }
        }
        if (projectile->flight_sound != 0) {
            snd_req(projectile->flight_sound);
        }
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }

    get_bone_world_pos(
        projectile->retarget_object, 0x10, &target_position);
    dx = target_position.x - object->pos.value.x;
    dy = target_position.y - object->pos.value.y;
    dz = target_position.z - object->pos.value.z;
    if ((double)(dx * dx + dy * dy + dz * dz) < 0.2 &&
        !victim->state_flags.bits.projectile_invulnerable &&
        victim->state != 0x1222 && projectile->reaction != -1) {
        victim_state->duck_reaction_active = 0;
        if (projectile->sound_handle != 0) {
            snd_stop(projectile->sound_handle);
            projectile->sound_handle = 0;
        }

        process = 0;
        if (projectile->setup_bits.hit_script_set) {
            source = projectile->source_object;
            if (source != 0 &&
                source->hdr.instance != projectile->source_object_instance) {
                source = 0;
            }
            if (source != 0) {
                if (source == g_game_info.plyr0.slot.mirror_a) {
                    owner = g_game_info.plyr0.slot.pdata;
                } else {
                    owner = g_game_info.plyr1.slot.pdata;
                }
                process = projectile_start_end_script(
                    owner, victim, projectile->hit_script_index);
            }
        }
        if (process != 0) {
            script_data = (ProjectileScriptPdata*)pdata_of_proc(process);
            object = projectile->object;
            if (object != 0 &&
                object->hdr.instance != projectile->object_instance) {
                object = 0;
            }
            if (script_data != 0 && object != 0) {
                script_data->last_position.x = object->pos.value.x;
                script_data->last_position.y = object->pos.value.y;
                script_data->last_position.z = object->pos.value.z;
                script_data->velocity.x = object->pos_vel.x;
                script_data->velocity.y = object->pos_vel.y;
                script_data->velocity.z = object->pos_vel.z;
            }
        }

        trial_state_collision_check(1, victim->plyr_num);
        reaction_xfer_him(
            projectile->reaction, projectile->reaction_scale,
            projectile->reaction_flags);
        if (victim_state->collision_result == 1) {
            if (projectile->flight_sound != 0) {
                snd_req(projectile->flight_sound);
            }
        } else if (victim_state->collision_result == 2 &&
                   projectile->impact_sound != 0) {
            snd_req(projectile->impact_sound);
        }
        ((ProjectileProcVtable*)aproc->vtbl)->jump_sleep(
            p_projectile_die, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

static float p_projectile_impaled(void) {
    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        proj_pdata->impaled_target->impaled_projectile_state--;
        ((ProjectileProcVtable*)aproc->vtbl)
            ->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

/*
 * Soft ceiling: the sound teardown, victim state reset, source latch, owner
 * selection, script-process payload, and final transform snapshot follow
 * retail. Remaining differences are helper inlining and register allocation.
 */
float p_projectile_die(void) {
    ProjectilePdata* projectile;
    ProjectileScriptPdata* script_data;
    PlyrPdata* owner;
    MkObj* source;
    MkObj* object;
    MkProc* process;

    projectile = proj_pdata;
    if (projectile->sound_handle != 0) {
        snd_stop(projectile->sound_handle);
        projectile->sound_handle = 0;
    }
    projectile->impaled_target->his_plyr_pdata->duck_reaction_active = 0;
    process = 0;
    if (projectile->setup_bits.end_script_set) {
        source = projectile->source_object;
        if (source != 0 &&
            source->hdr.instance != projectile->source_object_instance) {
            source = 0;
        }
        if (source != 0) {
            if (source == g_game_info.plyr0.slot.mirror_a) {
                owner = g_game_info.plyr0.slot.pdata;
            } else {
                owner = g_game_info.plyr1.slot.pdata;
            }
            process = projectile_start_end_script(
                owner, projectile->impaled_target,
                projectile->end_callback_index);
        }
    }
    if (process != 0) {
        script_data = (ProjectileScriptPdata*)pdata_of_proc(process);
        object = projectile->object;
        if (object != 0 &&
            object->hdr.instance != projectile->object_instance) {
            object = 0;
        }
        if (script_data != 0 && object != 0) {
            script_data->last_position.x = object->pos.value.x;
            script_data->last_position.y = object->pos.value.y;
            script_data->last_position.z = object->pos.value.z;
            script_data->velocity.x = object->ang.x;
            script_data->velocity.y = object->ang.y;
            script_data->velocity.z = object->ang.z;
        }
    }
    return -1.0f;
}

void get_projectile_script_velocity(Vec* velocity) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        velocity->x = pdata->velocity.x;
        velocity->y = pdata->velocity.y;
        velocity->z = pdata->velocity.z;
    }
}

void get_projectile_script_last_pos(Vec* position) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        position->x = pdata->last_position.x;
        position->y = pdata->last_position.y;
        position->z = pdata->last_position.z;
    }
}

PlyrPdata* get_projectile_script_plyr_pdata(void) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        return pdata->owner;
    }
    return 0;
}

int get_projectile_his_plyr_num(void) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;
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
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;
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
    ProjectileScriptPdata* pdata =
        (ProjectileScriptPdata*)pdata_of_proc(aproc);

    if (pdata->script_index == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(
        pdata->owner->cmo, pdata->script_index);
    cmdscript_execute(pdata->owner->cmo);
    return -1.0f;
}
