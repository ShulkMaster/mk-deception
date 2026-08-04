#include "game/game_info.h"
#include "math/gxVect.h"
#include "math/gxMath.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/light.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_pdata.h"

/*
 * Common projectile process data. Fields are named incrementally as the
 * projectile TU is lifted; keeping one typed layout avoids repeating raw
 * offsets across the script-facing setup functions.
 */
typedef struct ProjectilePdata {
    MkHdr hdr;
    PlyrPdata* owner;    /* +0x08 */
    PlyrPdata* opponent; /* +0x0C */
    union {
        unsigned int end_script_index;
        PlyrPdata* impaled_target;
        struct ProjectilePdata* retarget_source;
    }; /* +0x10 */
    MkObj* retarget_object;              /* +0x14 */
    MkProc* process;                    /* +0x18 */
    unsigned int process_instance;      /* +0x1C */
    char pad20[8];
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
    float target_ground_height;         /* +0x5C */
    char pad60[4];
    Vec velocity_damping;               /* +0x64 */
    Vec target_position; /* +0x70 */
    int impale_bone;                     /* +0x7C */
    Vec random_position;                /* +0x80 */
    Vec random_rotation;                /* +0x8C */
    int sound_handle;                   /* +0x98 */
    MkObj* tracking_light;              /* +0x9C */
    unsigned int tracking_light_instance; /* +0xA0 */
    int flight_sound;                   /* +0xA4 */
    int impact_sound;                   /* +0xA8 */
    int down_sound;      /* +0xAC */
    unsigned char setup_flags;          /* +0xB0 */
    unsigned char behavior_flags;       /* +0xB1 */
} ProjectilePdata;

typedef struct ProjectileScriptPdata {
    MkHdr hdr;
    PlyrPdata* owner;
    PlyrPdata* opponent;
    PlyrPdata* target;
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

typedef struct ProjectileFlippedBoneMap {
    unsigned int count;
    unsigned int* bone_ids;
} ProjectileFlippedBoneMap;

static ProjectilePdata* proj_pdata;

int snd_req(int sound);
float p_ground_target(void);
float p_projectile_launch_upward(void);
float p_projectile_downward(void);
float p_projectile_die(void);
void set_active_projectile_velocity_to_hit_gnd(float ticks);
int build_bones_tbl(MkObj* object, const int* tags);
MkObj* start_projectile_from_specific_plyr_bone(
    int model_id, int bone_id, const char* model_name,
    float x_offset, float y_offset, MkProcEntryFn process,
    int use_sidekick);
extern float game_speed;
extern MkPtr* point_light_list;
extern MkObj* plyr_obj;
extern GameInfo g_game_info;
extern unsigned short GXMathSqrtTable[];

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

int get_bid_with_flip(MkObj* object, unsigned int bone_id) {
    if ((object->hide_flags & 0x40) != 0) {
        ProjectileFlippedBoneMap* flipped =
            (ProjectileFlippedBoneMap*)object->flipped_bone_map;

        if (bone_id < flipped->count) {
            bone_id = flipped->bone_ids[bone_id];
        }
    }
    return bone_id;
}

void active_projectile_setup_done(void) {
    proj_pdata = 0;
}

void ps_projectile(void) {
    proj_pdata = 0;
}

void pw_projectile(void) {
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

    pdata->owner = source->owner;
    pdata->opponent = source->opponent;
    pdata->retarget_source = source->retarget_source;
    pdata->retarget_object = source->retarget_object;

    speed = projectile_fast_sqrt(
        object->pos_vel.x * object->pos_vel.x +
        object->pos_vel.y * object->pos_vel.y +
        object->pos_vel.z * object->pos_vel.z);
    dx = pdata->retarget_object->pos.x - object->pos.x;
    dz = pdata->retarget_object->pos.z - object->pos.z;
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
    target_y = g_game_info.field_34 - object->pos.y;
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

void set_active_projectile_velocity(const Vec* velocity) {
    MkObj* object;
    float inverse_length;

    if (proj_pdata == 0) {
        return;
    }
    object = proj_pdata->object;
    if (object == 0 ||
        object->hdr.instance != proj_pdata->object_instance) {
        return;
    }

    object->flags_08 |= 0x20;
    object->pos_vel = *velocity;
    if (object->pos_vel.x == 0.0f && object->pos_vel.y == 0.0f) {
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

void set_active_projectile_max_ticks(int ticks) {
    if (proj_pdata != 0) {
        proj_pdata->max_ticks = (float)ticks;
    }
}

void set_active_projectile_target_pos(const Vec* position) {
    if (proj_pdata != 0) {
        proj_pdata->target_position.x = position->x;
        proj_pdata->target_position.y = position->y;
        proj_pdata->target_position.z = position->z;
    }
}

void set_active_projectile_dn_sound(int sound) {
    if (proj_pdata != 0 && sound != 0) {
        proj_pdata->down_sound = sound;
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

PlyrPdata* get_projectile_script_plyr_pdata(void) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        return pdata->owner;
    }
    return 0;
}

int get_projectile_his_plyr_num(void) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        PlyrPdata* opponent = pdata->opponent;

        if (opponent != 0) {
            return opponent->plyr_num;
        }
    }
    return 3;
}

int get_projectile_script_plyr_num(void) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        PlyrPdata* owner = pdata->owner;

        if (owner != 0) {
            return owner->plyr_num;
        }
    }
    return 3;
}

void get_projectile_script_last_pos(Vec* position) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        position->x = pdata->last_position.x;
        position->y = pdata->last_position.y;
        position->z = pdata->last_position.z;
    }
}

void get_projectile_script_velocity(Vec* velocity) {
    ProjectileScriptPdata* pdata = (ProjectileScriptPdata*)apdata;

    if (pdata != 0) {
        velocity->x = pdata->velocity.x;
        velocity->y = pdata->velocity.y;
        velocity->z = pdata->velocity.z;
    }
}

int check_for_throw(ProjectilePdata* pdata) {
    PlyrPdata* target = pdata->impaled_target;
    MkProc* hold_proc = target->hold_proc;

    if (hold_proc != 0 &&
        hold_proc->instance == target->hold_proc_instance) {
        return 1;
    }
    return 0;
}

float p_proj_end_run_script(void) {
    ProjectilePdata* pdata =
        (ProjectilePdata*)pdata_of_proc(aproc);

    if (pdata->end_script_index == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(
        pdata->owner->cmo, pdata->end_script_index);
    cmdscript_execute(pdata->owner->cmo);
    return -1.0f;
}

void set_active_projectile_velocity_damp(const Vec* damping) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_flags |= 0x80;
        proj_pdata->velocity_damping.x = damping->x;
        proj_pdata->velocity_damping.y = damping->y;
        proj_pdata->velocity_damping.z = damping->z;
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

void set_active_projectile_hit_gnd_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->ground_script_index = script_index;
        proj_pdata->setup_flags |= 1;
    }
}

void set_active_projectile_end_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->end_callback_index = script_index;
        proj_pdata->setup_flags |= 4;
    }
}

void set_active_projectile_block_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->block_script_index = script_index;
        proj_pdata->setup_flags |= 2;
    }
}

void set_active_projectile_hit_script(unsigned int script_index) {
    if (proj_pdata != 0) {
        proj_pdata->hit_script_index = script_index;
        proj_pdata->setup_flags |= 8;
    }
}

void set_active_projectile_collision_info(
    int enabled, float radius, float height, float depth) {
    if (proj_pdata != 0) {
        if (enabled != 0) {
            proj_pdata->setup_flags |= 0x10;
        } else {
            proj_pdata->setup_flags &= (unsigned char)~0x10;
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
        proj_pdata->setup_flags |= 0x20;
    }
}

void set_active_projectile_random_pos(float x, float y, float z) {
    if (proj_pdata != 0) {
        proj_pdata->random_position.x = x;
        proj_pdata->random_position.y = y;
        proj_pdata->random_position.z = z;
        proj_pdata->setup_flags |= 0x40;
    }
}

void set_active_projectile_continue_thru_hit(void) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_flags |= 8;
    }
}

void set_active_projectile_3d_track(void) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_flags |= 0x10;
    }
}

void set_active_projectile_2d_track(void) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_flags |= 0x20;
    }
}

void set_active_projectile_not_duckable(void) {
    if (proj_pdata != 0) {
        proj_pdata->behavior_flags |= 0x40;
    }
}

void set_active_projectile_p_handler(MkProcEntryFn handler) {
    MkProc* process;

    if (proj_pdata == 0) {
        return;
    }
    process = proj_pdata->process;
    if (process != 0 &&
        process->instance == proj_pdata->process_instance) {
        xfer_proc(process, handler);
    }
}

void set_active_projectile_target_ground(
    float ticks, float target_height, float collision_radius) {
    MkProc* process;

    if (proj_pdata == 0) {
        return;
    }
    proj_pdata->target_ground_height = target_height;
    proj_pdata->collision_radius = collision_radius;
    process = proj_pdata->process;
    if (process != 0 &&
        process->instance == proj_pdata->process_instance) {
        xfer_proc(process, p_ground_target);
    }
    set_active_projectile_velocity_to_hit_gnd(ticks);
}

void set_active_projectile_upward_attack(const Vec* target) {
    MkProc* process;

    if (proj_pdata != 0) {
        process = proj_pdata->process;
        if (process != 0 &&
            process->instance == proj_pdata->process_instance) {
            xfer_proc(process, p_projectile_launch_upward);
        }
    }
    if (proj_pdata != 0) {
        proj_pdata->target_position.x = target->x;
        proj_pdata->target_position.y = target->y;
        proj_pdata->target_position.z = target->z;
    }
}

void set_active_add_ang_y(float angle) {
    MkObj* object;
    int fixed;

    if (proj_pdata == 0) {
        return;
    }
    object = proj_pdata->object;
    if (object == 0 ||
        object->hdr.instance != proj_pdata->object_instance) {
        return;
    }
    object->ang.y += angle;
    fixed = (int)(166886.1f * object->ang.y) & 0xFFFFF;
    object->ang.y = 0.000005992112f * (float)fixed;
}

float p_projectile_launch_upward(void) {
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
        object->pos.x = proj_pdata->target_position.x;
        object->pos.z = proj_pdata->target_position.z;
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

float p_projectile_impaled(void) {
    proj_pdata->max_ticks -= game_speed;
    if (proj_pdata->max_ticks < 0.0f) {
        proj_pdata->impaled_target->impaled_projectile_state--;
        ((ProjectileProcVtable*)aproc->vtbl)
            ->jump_sleep(p_projectile_die, 0.0f);
        return 0.0f;
    }
    return 1.0f;
}

void set_active_projectile_impale_info(int bone) {
    MkObj* object;

    if (proj_pdata == 0) {
        return;
    }
    object = proj_pdata->object;
    if (object == 0 ||
        object->hdr.instance != proj_pdata->object_instance) {
        return;
    }
    build_bones_tbl(object, 0);
    proj_pdata->impale_bone = bone;
    proj_pdata->setup_flags |= 0x80;
}

float p_point_light_follower(void) {
    ProjectileFollowerPdata* follower =
        (ProjectileFollowerPdata*)apdata;
    ProjectilePdata* projectile = follower->projectile;
    MkObj* object;
    MkObj* light;

    if (projectile == 0 ||
        projectile->hdr.instance != follower->projectile_instance) {
        return -1.0f;
    }
    object = projectile->object;
    if (object == 0 ||
        object->hdr.instance != projectile->object_instance) {
        return -1.0f;
    }
    light = projectile->tracking_light;
    if (light == 0 ||
        light->hdr.instance != projectile->tracking_light_instance) {
        return -1.0f;
    }
    light->pos.x = object->pos.x;
    light->pos.y = object->pos.y;
    light->pos.z = object->pos.z;
    update_obj_pos(light);
    return 1.0f;
}

MkObj* set_active_projectile_tracking_light(LightDef* definition) {
    ProjectileFollowerPdata* follower;
    MkHdr* raw_pdata = 0;
    MkObj* light;
    MkProc* process;

    if (proj_pdata == 0) {
        return 0;
    }
    light = load_light(definition, &point_light_list, 0);
    if (light == 0) {
        return 0;
    }
    process = _create_mkproc_generic_tinystack(
        0x2026, 0x1F, p_point_light_follower,
        sizeof(ProjectileFollowerPdata), &raw_pdata);
    if (process == 0) {
        if (light->hdr.instance != 0) {
            ((void (*)(MkHdr*))light->hdr.vtbl->destroy)(&light->hdr);
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

MkObj* start_projectile_from_plyr_bone(
    int model_id, int bone_id, const char* model_name,
    float x_offset, float y_offset, MkProcEntryFn process) {
    return start_projectile_from_specific_plyr_bone(
        model_id, bone_id, model_name, x_offset, y_offset, process, 0);
}

MkObj* start_projectile_from_sidekick_bone(
    int model_id, int bone_id, const char* model_name, int flags,
    float x_offset, float y_offset) {
    return start_projectile_from_specific_plyr_bone(
        model_id, bone_id, model_name, x_offset, y_offset,
        (MkProcEntryFn)flags, 1);
}
