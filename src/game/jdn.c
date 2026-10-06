#include "game/game_info.h"
#include "fdlibm.h"
#include "game/jdn.h"
#include "libmkparticle/color.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/texture_anim.h"
#include "libmkparticle/vm.h"
#include "math/gxVect.h"
#include "math/gxMath.h"
#include "runtime/mk_particle.h"
#include "platform/main.h"
#include "runtime/mtRand2.h"
#include "runtime/utils.h"
#include "runtime/cstring.h"

static float pfx_glass_break_run(void);
static void mkpfx_spawnupdate_glass_break(PfxVm* vm);

const unsigned int noch_pfx_table[] = {
    5, 0x272, 3, 0, 0xBB03126F, 0, 0x3E4CCCCD, 0x42F00000,
    0x42F00000, 0x42C80000, 0x437F0000, 0x96, 0x64, 0, 0, 0x100,
    0x40, 0x55, 0xC, 0x40000000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    (unsigned int)mkpfx_spawnupdate_glass_break,
};
static const char stringBase0[] = "C - glass shards\0C.glass_shard";

static PfxColor glass_fragment_alphas[0xB5];
float soul_sine[0x400];
unsigned int g_kill_shard_fx;

MkPfx* create_pfx(int, int, float (*)(void), MkPfx**,
                  const unsigned int*, const char*);

MkPfx* start_pfx_glass_shards(
    int art_id, Vec* position, Vec* center, int bounce_limit,
    unsigned int spawn_count, unsigned int scale_mode, int motion_mode) {
    MkPfx* glass;
    MkPfx* pfx;
    MkObj* emitter_object;
    unsigned int index;

    pfx = create_pfx(0x8023, 0x8023, pfx_glass_break_run, &glass,
                     noch_pfx_table, stringBase0);
    if (pfx != 0 && glass != 0) {
        emitter_object = (MkObj*)pfx_get_emitter_obj(glass, 0);
        if (emitter_object == 0) {
            if (pfx->hdr.instance != 0) {
                pfx->hdr.typed_vtbl->destroy(&pfx->hdr);
            }
            return 0;
        }

        emitter_object->pos.value.x = position->x;
        emitter_object->pos.value.y = position->y;
        emitter_object->pos.value.z = position->z;
        emitter_object->flags_08_bits.airborne = 1;
        update_mkobj(emitter_object != 0 ? as_mkhdr(&emitter_object->hdr) : 0);

        glass->depth_bias = -50.0f;
        glass->effect_center.x = center->x;
        glass->effect_center.y = center->y;
        glass->effect_center.z = center->z;
        glass->effect_state = bounce_limit;
        glass->field_29C = 0.1f;
        glass->field_28C = 0xB4;
        glass->glass_alphas = glass_fragment_alphas;
        glass->field_290 = 1;
        glass->field_298 = 0.5f;
        glass->field_294 = motion_mode;

        if (scale_mode == 0) {
            glass->field_2A0 = 0.15f;
            glass->field_29C = 0.15f;
        } else if (scale_mode == 1) {
            glass->field_2A0 = 0.1f;
            glass->field_29C = 0.1f;
        } else if (scale_mode == 2) {
            glass->field_2A0 = 0.05f;
            glass->field_29C = 0.05f;
        } else if (scale_mode == 3) {
            glass->field_2A0 = 0.2f;
            glass->field_29C = 0.2f;
        } else if (scale_mode == 4) {
            glass->field_2A0 = 0.16f;
            glass->field_29C = 0.03f;
        }

        set_pfx_texture(
            (PfxVm*)glass->matrix, 0x2001E, art_id);
        if ((unsigned int)(art_id - 0x013D0000) == 8) {
            for (index = 0; index <= sizeof(glass_fragment_alphas) / sizeof(glass_fragment_alphas[0]) - 1; index++) {
                pfx_native_set_rgba(&glass_fragment_alphas[index], 90.0f, 130.0f,
                    90.0f, 255.0f - 255.0f * (float)index / 181.0f);
            }
            glass->field_2A0 *= 1.35f;
            glass->field_29C *= 1.35f;
            pfx_texture_animate((PfxVm*)glass->matrix,
                                0x80, 0x20, 0x20, 0x10, 3.0f);
        } else if ((unsigned int)(art_id - 0x013D0000) == 9) {
            pfx_texture_animate((PfxVm*)glass->matrix,
                                0x80, 0x20, 0x20, 0x10, 5.0f);
        } else {
            pfx_texture_animate((PfxVm*)glass->matrix,
                                0x100, 0x40, 0x40, 0x10, 5.0f);
        }
        pfx_get_emitter((PfxVm*)glass->matrix, 0)->birth_rate = spawn_count;
        glass->emitter_enabled = 1;
    } else {
        return 0;
    }
    glass->name_dst = (char*)stringBase0 + 0x11;
    return glass;
}

/* TODO: [near miss] 96.09%; CFG/math/calls recovered; pass-1 spill choice differs (retail spills src_angle, index r31, vm r17); needs spill-cost evidence. */
static float pfx_glass_break_run(void) {
    int bounce_limit = apfx->effect_state;
    float moving_damping = pow(0.99f, game_speed);
    float resting_damping = pow(0.975f, game_speed);
    float gravity = 0.003f * game_speed;
    PfxVm* vm = (PfxVm*)apfx->matrix;
    float* dst_scale = pfx_get_field(vm, -2, 0x102);
    float* src_scale = pfx_get_field(vm, -1, 0x102);
    float* src_time = pfx_get_field(vm, -1, 0x301);
    float* dst_time = pfx_get_field(vm, -2, 0x301);
    Vec* dst_pos = pfx_get_field(vm, -2, 0x100);
    Vec* src_pos = pfx_get_field(vm, -1, 0x100);
    Vec* src_vel = pfx_get_field(vm, -1, 0x300);
    Vec* dst_vel = pfx_get_field(vm, -2, 0x300);
    PfxColor* dst_color = pfx_get_field(vm, -2, 0x101);
    int* src_state = pfx_get_field(vm, -1, 0x307);
    int* dst_state = pfx_get_field(vm, -2, 0x307);
    float* src_life = pfx_get_field(vm, -1, 0x305);
    float* dst_life = pfx_get_field(vm, -2, 0x305);
    float* dst_angle = pfx_get_field(vm, -2, 0x103);
    float* src_angle = pfx_get_field(vm, -1, 0x103);
    int fstride = vm->transforms[0].particle_field_stride;
    int vstride = vm->particle_vector_stride;
    int last = vm->particle_cursor - 1;
    float* last_angle = PFX_FIELD_AT(src_angle, fstride * last);
    Vec* last_pos = PFX_FIELD_AT(src_pos, fstride * last);
    Vec* last_vel = PFX_FIELD_AT(src_vel, vstride * last);
    float* last_time = PFX_FIELD_AT(src_time, vstride * last);
    float* last_scale = PFX_FIELD_AT(src_scale, fstride * last);
    float* last_life = PFX_FIELD_AT(src_life, vstride * last);
    int* last_state = PFX_FIELD_AT(src_state, vstride * last);
    int index = 0;

    while (index < vm->particle_cursor) {
        if (*src_life >= (float)(apfx->field_28C - 1)) {
            vm->particle_cursor--;
            if (index != vm->particle_cursor) {
                memcpy(src_pos, last_pos, fstride);
                memcpy(dst_vel, last_vel, vstride);
                index--;
                last_pos = PFX_FIELD_AT(last_pos, -fstride);
                last_vel = PFX_FIELD_AT(last_vel, -vstride);
                *src_time = *last_time;
                last_time = PFX_FIELD_AT(last_time, -vstride);
                *src_scale = *last_scale;
                last_scale = PFX_FIELD_AT(last_scale, -fstride);
                *src_angle = *last_angle;
                last_angle = PFX_FIELD_AT(last_angle, -fstride);
                *src_state = *last_state;
                last_state = PFX_FIELD_AT(last_state, -vstride);
                *src_life = *last_life;
                last_life = PFX_FIELD_AT(last_life, -vstride);
            }
        } else {
            Vec displacement;
            float old_y;
            dst_vel->x = src_vel->x;
            dst_vel->y = src_vel->y;
            dst_vel->z = src_vel->z;
            *dst_life = *src_life;
            *dst_time = *src_time;
            if (src_pos->y < 0.5f * *src_scale + g_game_info.field_34 &&
                src_vel->y != 0.0f && src_vel->y < 0.0f) {
                *dst_state = *src_state + 1;
                dst_vel->y *= -0.5f;
            } else {
                *dst_state = *src_state;
            }
            if (*dst_state >= bounce_limit && *dst_life == 0.0f) {
                dst_vel->y = 0.0f;
            }
            if (apfx->field_294 == 6) {
                dst_vel->x *= moving_damping;
                dst_vel->z *= moving_damping;
                old_y = dst_vel->y;
                displacement.y = old_y * game_speed;
                displacement.x = dst_vel->x * game_speed;
                displacement.z = dst_vel->z * game_speed;
                dst_vel->y = old_y - gravity;
                dst_pos->x = src_pos->x + displacement.x;
                dst_pos->y = src_pos->y + displacement.y;
                dst_pos->z = src_pos->z + displacement.z;
                *dst_time = *src_time + game_speed;
            } else if (*dst_state < bounce_limit) {
                dst_vel->x *= moving_damping;
                dst_vel->z *= moving_damping;
                old_y = dst_vel->y;
                displacement.y = old_y * game_speed;
                displacement.x = dst_vel->x * game_speed;
                displacement.z = dst_vel->z * game_speed;
                dst_vel->y = old_y - gravity;
                dst_pos->x = src_pos->x + displacement.x;
                dst_pos->y = src_pos->y + displacement.y;
                dst_pos->z = src_pos->z + displacement.z;
            } else {
                dst_vel->x *= resting_damping;
                dst_vel->z *= resting_damping;
                gxVectScale(&displacement, dst_vel, game_speed);
                dst_pos->x = src_pos->x + displacement.x;
                dst_pos->y = src_pos->y + displacement.y;
                dst_pos->z = src_pos->z + displacement.z;
            }
            *dst_scale = *src_scale;
            *dst_angle = *src_angle;
            *dst_color = apfx->glass_alphas[(int)*src_life];
            if (*dst_state < bounce_limit) {
                *dst_time = *src_time + game_speed;
            } else {
                if (((unsigned int)(*src_time / game_speed) & 0xF) != 9) {
                    *dst_time = *src_time + game_speed;
                }
                if (apfx->field_290 != 0) {
                    int max_life = apfx->field_28C;
                    float life = *src_life + game_speed;
                    if ((float)max_life <= life) {
                        life = max_life;
                    }
                    *dst_life = life;
                }
            }
#define ADVANCE(ptr, stride) \
            ((ptr) = PFX_FIELD_AT((ptr), (stride)))
            ADVANCE(dst_time, vstride); ADVANCE(src_time, vstride);
            ADVANCE(dst_pos, fstride); ADVANCE(src_pos, fstride);
            ADVANCE(dst_angle, fstride); ADVANCE(dst_scale, fstride);
            ADVANCE(src_scale, fstride); ADVANCE(dst_vel, vstride);
            ADVANCE(src_vel, vstride); ADVANCE(dst_color, fstride);
            ADVANCE(dst_state, vstride); ADVANCE(src_state, vstride);
            ADVANCE(dst_life, vstride); ADVANCE(src_life, vstride);
            ADVANCE(src_angle, fstride);
        }
        index++;
    }

    if (pfx_get_emitter(vm, 0)->birth_rate != 0.0f) {
        int spawn_index;
        PfxColor color;
        color.r = 0x80;
        color.g = 0x80;
        color.b = 0x80;
        color.a = 0xFF;
        vm->particle_cursor += (int)pfx_get_emitter(vm, 0)->birth_rate;
        for (spawn_index = 0; spawn_index < vm->particle_cursor; spawn_index++) {
            if (reseed_rnd_tbl != 0) reload_rnd_tbl();
            *dst_life = 0.0f;
            if (apfx->field_294 == 9) {
                dst_pos->x = apfx_emitter_obj->pos.value.x + frand(0.8f);
                dst_pos->y = apfx_emitter_obj->pos.value.y + apfx->field_298 + sfrand(0.5f);
                dst_pos->z = apfx_emitter_obj->pos.value.z + frand(0.8f);
            } else {
                dst_pos->x = apfx_emitter_obj->pos.value.x - 0.1f + frand(0.2f);
                dst_pos->y = apfx_emitter_obj->pos.value.y + apfx->field_298 + sfrand(0.5f);
                dst_pos->z = apfx_emitter_obj->pos.value.z - 0.1f + frand(0.2f);
            }
            if (apfx->field_294 == 0) {
                dst_vel->x = apfx->effect_center.x - 0.05f + frand(0.1f);
                dst_vel->y = apfx->effect_center.y - 0.05f + frand(0.15f);
                dst_vel->z = apfx->effect_center.z - 0.05f + frand(0.1f);
            } else if (apfx->field_294 == 1) {
                dst_vel->x = apfx->effect_center.x + sfrand(0.05f);
                dst_vel->y = apfx->effect_center.y + sfrand(0.01f);
                dst_vel->z = apfx->effect_center.z + sfrand(0.05f);
            } else if (apfx->field_294 == 6) {
                dst_vel->x = apfx->effect_center.x + sfrand(0.1f);
                dst_vel->y = apfx->effect_center.y + sfrand(0.05f);
                dst_vel->z = apfx->effect_center.z + sfrand(0.1f);
                *dst_life = 15.0f * game_speed;
            } else if (apfx->field_294 == 9) {
                dst_vel->x = sfrand(0.1f);
                dst_vel->y = apfx->effect_center.y + sfrand(0.04f);
                dst_vel->z = sfrand(0.1f);
            } else {
                dst_vel->x = apfx->effect_center.x + sfrand(0.05f);
                dst_vel->y = apfx->effect_center.y + frand(0.07f);
                dst_vel->z = apfx->effect_center.z + sfrand(0.05f);
            }
            *dst_time = (unsigned short)randu0(0x14);
            *dst_scale = apfx->field_29C + frand(3.0f * apfx->field_2A0);
            *dst_color = color;
            *dst_state = 0;
            *dst_angle = frand(6.2831855f);
            ADVANCE(dst_time, vstride); ADVANCE(dst_pos, fstride);
            ADVANCE(dst_scale, fstride); ADVANCE(dst_vel, vstride);
            ADVANCE(dst_color, fstride); ADVANCE(dst_state, vstride);
            ADVANCE(dst_life, vstride); ADVANCE(dst_angle, fstride);
        }
        pfx_get_emitter(vm, 0)->birth_rate = 0.0f;
    }

#undef ADVANCE
    if ((int)g_kill_shard_fx == 1) {
        return -1.0f;
    }
    if (vm->particle_cursor == 0) {
        return -1.0f;
    }
    return 1.0f;
}

static void mkpfx_spawnupdate_glass_break(PfxVm* vm) {
    int index;
    for (index = 0; index <= (int)(sizeof(glass_fragment_alphas) / sizeof(glass_fragment_alphas[0])) - 1; index++) {
        pfx_native_set_rgba(&glass_fragment_alphas[index], 128.0f, 128.0f,
            128.0f, 255.0f - 255.0f * (float)index / 181.0f);
    }
    pfxvm_require_field(vm, 0x307);
    pfxvm_require_field(vm, 0x305);
}

void allow_shard_pfx_now(void) { g_kill_shard_fx = 0; }
void kill_shard_pfx_now(void) { g_kill_shard_fx = 1; }
float get_soul_sine(int index) { return soul_sine[index]; }

#pragma optimize_for_size on
#pragma use_lmw_stmw on

static inline void init_soul_sine(void) {
    static int table_defined;
    int index;

    if (table_defined == 0) {
        soul_sine[0] = gxMathSin(0.0f);
        for (index = 1; index < 0x400; index++) {
            soul_sine[index] = gxMathSin(3.1415927f * (float)(index * 2) / 1024.0f);
        }
        table_defined = 1;
    }
}

int build_sine_table_for_scripts(void) {
    init_soul_sine();
    return 0x400;
}

void build_sine_table(void) {
    init_soul_sine();
}

#pragma optimize_for_size reset
#pragma use_lmw_stmw reset
