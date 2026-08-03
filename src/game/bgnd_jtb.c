#include "math/gxMath.h"
#include "math/gxVect.h"
#include "platform/gcutils.h"
#include "runtime/anim_pdata.h"
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
