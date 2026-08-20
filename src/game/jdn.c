#include "game/game_info.h"
#include "game/jdn.h"
#include "libmkparticle/color.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/vm.h"
#include "math/gxVect.h"
#include "runtime/mk_particle.h"
#include "runtime/cstring.h"

typedef struct JdnGlassPfx {
    MkPfx pfx;
} JdnGlassPfx;

typedef struct JdnEmitterObject {
    MkHdr hdr;
    union {
        unsigned char flags;
        MkObjFlags08 flags_bits;
    };
    char pad09[0x97];
    Vec position;
} JdnEmitterObject;

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
static unsigned int table_defined_242;

extern float game_speed;
extern unsigned int reseed_rnd_tbl;

MkPfx* create_pfx(int, int, float (*)(void), JdnGlassPfx**,
                  const unsigned int*, const char*);
void* pfx_get_field(PfxVm*, int, int);
void pfxvm_require_field(PfxVm*, int);
float frand(float);
float sfrand(float);
unsigned int randu0(unsigned int);
void reload_rnd_tbl(void);
double pow(double, double);
float gxMathSin(float);

MkPfx* start_pfx_glass_shards(
    int art_id, const Vec* position, const Vec* center, int bounce_limit,
    unsigned int spawn_count, unsigned int scale_mode, int motion_mode) {
    JdnGlassPfx* glass = 0;
    MkPfx* pfx;
    JdnEmitterObject* emitter_object;
    int index;

    pfx = create_pfx(0x8023, 0x8023, pfx_glass_break_run, &glass,
                     noch_pfx_table, stringBase0);
    if (pfx == 0 || glass == 0) {
        return 0;
    }
    emitter_object = (JdnEmitterObject*)pfx_get_emitter_obj(pfx, 0);
    if (emitter_object == 0) {
        if (pfx->hdr.instance != 0) {
            pfx->hdr.typed_vtbl->destroy(&pfx->hdr);
        }
        return 0;
    }

    emitter_object->position = *position;
    emitter_object->flags_bits.airborne = 1;
    as_mkhdr(&emitter_object->hdr);
    update_mkobj(&emitter_object->hdr);

    glass->pfx.field_28 = -50.0f;
    glass->pfx.glass_center = *center;
    glass->pfx.effect_state = bounce_limit;
    glass->pfx.field_29C = 0.1f;
    glass->pfx.field_28C = 0xB4;
    glass->pfx.glass_alphas = glass_fragment_alphas;
    glass->pfx.field_290 = 1;
    glass->pfx.field_298 = 0.5f;
    glass->pfx.field_294 = motion_mode;

    switch (scale_mode) {
    case 0: glass->pfx.field_2A0 = glass->pfx.field_29C = 0.15f; break;
    case 1: glass->pfx.field_2A0 = glass->pfx.field_29C = 0.1f; break;
    case 2: glass->pfx.field_2A0 = glass->pfx.field_29C = 0.05f; break;
    case 3: glass->pfx.field_2A0 = glass->pfx.field_29C = 0.2f; break;
    case 4:
        glass->pfx.field_2A0 = 0.16f;
        glass->pfx.field_29C = 0.03f;
        break;
    }

    set_pfx_texture(
        (PfxVm*)glass->pfx.matrix, (void*)0x2001E, (void*)art_id);
    if ((unsigned int)(art_id - 0x013D0000) == 8) {
        for (index = 0; index <= 0xB4; index++) {
            pfx_native_set_rgba(&glass_fragment_alphas[index], 90.0f, 130.0f,
                90.0f, 255.0f - 255.0f * (float)index / 181.0f);
        }
        glass->pfx.field_2A0 *= 1.35f;
        glass->pfx.field_29C *= 1.35f;
        pfx_texture_animate((PfxVm*)glass->pfx.matrix, 3.0f,
                            0x80, 0x20, 0x20, 0x10);
    } else if ((unsigned int)(art_id - 0x013D0000) == 9) {
        pfx_texture_animate((PfxVm*)glass->pfx.matrix, 5.0f,
                            0x80, 0x20, 0x20, 0x10);
    } else {
        pfx_texture_animate((PfxVm*)glass->pfx.matrix, 5.0f,
                            0x100, 0x40, 0x40, 0x10);
    }
    *(float*)&pfx_get_emitter((PfxEmitterTableView*)glass->pfx.matrix, 0)
                   ->bytes[0xC] = (float)spawn_count;
    glass->pfx.emitter_enabled = 1;
    glass->pfx.name_dst = (char*)stringBase0 + 0x11;
    return &glass->pfx;
}

static float pfx_glass_break_run(void) {
    JdnGlassPfx* glass = (JdnGlassPfx*)apfx;
    PfxVm* vm = (PfxVm*)glass->pfx.matrix;
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
    Vec* last_pos = (Vec*)((unsigned char*)src_pos + fstride * last);
    Vec* last_vel = (Vec*)((unsigned char*)src_vel + vstride * last);
    float* last_time = (float*)((unsigned char*)src_time + vstride * last);
    float* last_scale = (float*)((unsigned char*)src_scale + fstride * last);
    float* last_angle = (float*)((unsigned char*)src_angle + fstride * last);
    float* last_life = (float*)((unsigned char*)src_life + vstride * last);
    int* last_state = (int*)((unsigned char*)src_state + vstride * last);
    float moving_damping = (float)pow(0.99, game_speed);
    float resting_damping = (float)pow(0.975, game_speed);
    float gravity = 0.003f * game_speed;
    int index = 0;

    while (index < vm->particle_cursor) {
        if (*src_life == (float)(glass->pfx.field_28C - 1)) {
            vm->particle_cursor--;
            if (index != vm->particle_cursor) {
                memcpy(src_pos, last_pos, fstride);
                memcpy(dst_vel, last_vel, vstride);
                *src_time = *last_time;
                *src_scale = *last_scale;
                *src_angle = *last_angle;
                *src_life = *last_life;
                *src_state = *last_state;
                index--;
                last_pos = (Vec*)((unsigned char*)last_pos - fstride);
                last_scale = (float*)((unsigned char*)last_scale - fstride);
                last_angle = (float*)((unsigned char*)last_angle - fstride);
                last_vel = (Vec*)((unsigned char*)last_vel - vstride);
                last_time = (float*)((unsigned char*)last_time - vstride);
                last_life = (float*)((unsigned char*)last_life - vstride);
                last_state = (int*)((unsigned char*)last_state - vstride);
            }
        } else {
            float old_y;
            *dst_vel = *src_vel;
            *dst_life = *src_life;
            *dst_time = *src_time;
            if (src_pos->y < 0.5f * *src_scale + g_game_info.field_34 &&
                src_vel->y != 0.0f && src_vel->y < 0.0f) {
                *dst_state = *src_state + 1;
                dst_vel->y *= -0.5f;
            } else {
                *dst_state = *src_state;
            }
            if (*dst_state >= glass->pfx.effect_state && *dst_life == 0.0f) {
                dst_vel->y = 0.0f;
            }
            if (glass->pfx.field_294 == 6 || *dst_state < glass->pfx.effect_state) {
                dst_vel->x *= moving_damping;
                dst_vel->z *= moving_damping;
                old_y = dst_vel->y;
                dst_vel->y = old_y - gravity;
                dst_pos->x = src_pos->x + dst_vel->x * game_speed;
                dst_pos->y = src_pos->y + old_y * game_speed;
                dst_pos->z = src_pos->z + dst_vel->z * game_speed;
                if (glass->pfx.field_294 == 6) {
                    *dst_time = *src_time + game_speed;
                }
            } else {
                dst_vel->x *= resting_damping;
                dst_vel->z *= resting_damping;
                dst_pos->x = src_pos->x + dst_vel->x * game_speed;
                dst_pos->y = src_pos->y + dst_vel->y * game_speed;
                dst_pos->z = src_pos->z + dst_vel->z * game_speed;
            }
            *dst_scale = *src_scale;
            *dst_angle = *src_angle;
            *dst_color = glass->pfx.glass_alphas[(int)*src_life];
            if (*dst_state < glass->pfx.effect_state) {
                *dst_time = *src_time + game_speed;
            } else {
                if (((unsigned int)(*src_time / game_speed) & 0xF) != 9) {
                    *dst_time = *src_time + game_speed;
                }
                if (glass->pfx.field_290 != 0) {
                    float life = *src_life + game_speed;
                    if ((float)glass->pfx.field_28C <= life) {
                        life = (float)glass->pfx.field_28C;
                    }
                    *dst_life = life;
                }
            }
#define ADVANCE(ptr, stride) \
            ((ptr) = (void*)((unsigned char*)(ptr) + (stride)))
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

    {
        float* pending = (float*)&pfx_get_emitter(
            (PfxEmitterTableView*)vm, 0)->bytes[0xC];
        if (*pending != 0.0f) {
            int count = (int)*pending;
            PfxColor color = {0x80, 0x80, 0x80, 0xFF};
            vm->particle_cursor += count;
            for (index = 0; index < vm->particle_cursor; index++) {
                if (reseed_rnd_tbl != 0) reload_rnd_tbl();
                *dst_life = 0.0f;
                if (glass->pfx.field_294 == 9) {
                    dst_pos->x = apfx_emitter_obj->pos.x + frand(0.8f);
                    dst_pos->y = apfx_emitter_obj->pos.y + glass->pfx.field_298 + sfrand(0.5f);
                    dst_pos->z = apfx_emitter_obj->pos.z + frand(0.8f);
                } else {
                    dst_pos->x = apfx_emitter_obj->pos.x - 0.1f + frand(0.2f);
                    dst_pos->y = apfx_emitter_obj->pos.y + glass->pfx.field_298 + sfrand(0.5f);
                    dst_pos->z = apfx_emitter_obj->pos.z - 0.1f + frand(0.2f);
                }
                switch (glass->pfx.field_294) {
                case 0:
                    dst_vel->x = glass->pfx.glass_center.x - 0.05f + frand(0.1f);
                    dst_vel->y = glass->pfx.glass_center.y - 0.05f + frand(0.15f);
                    dst_vel->z = glass->pfx.glass_center.z - 0.05f + frand(0.1f);
                    break;
                case 1:
                    dst_vel->x = glass->pfx.glass_center.x + sfrand(0.05f);
                    dst_vel->y = glass->pfx.glass_center.y + sfrand(0.01f);
                    dst_vel->z = glass->pfx.glass_center.z + sfrand(0.05f);
                    break;
                case 6:
                    dst_vel->x = glass->pfx.glass_center.x + sfrand(0.1f);
                    dst_vel->y = glass->pfx.glass_center.y + sfrand(0.05f);
                    dst_vel->z = glass->pfx.glass_center.z + sfrand(0.1f);
                    *dst_life = 15.0f * game_speed;
                    break;
                case 9:
                    dst_vel->x = sfrand(0.1f);
                    dst_vel->y = glass->pfx.glass_center.y + sfrand(0.04f);
                    dst_vel->z = sfrand(0.1f);
                    break;
                default:
                    dst_vel->x = glass->pfx.glass_center.x + sfrand(0.05f);
                    dst_vel->y = glass->pfx.glass_center.y + frand(0.07f);
                    dst_vel->z = glass->pfx.glass_center.z + sfrand(0.05f);
                    break;
                }
                *dst_time = (float)randu0(0x14);
                *dst_scale = glass->pfx.field_29C + frand(3.0f * glass->pfx.field_2A0);
                *dst_color = color;
                *dst_state = 0;
                *dst_angle = frand(6.2831855f);
                ADVANCE(dst_time, vstride); ADVANCE(dst_pos, fstride);
                ADVANCE(dst_scale, fstride); ADVANCE(dst_vel, vstride);
                ADVANCE(dst_color, fstride); ADVANCE(dst_state, vstride);
                ADVANCE(dst_life, vstride); ADVANCE(dst_angle, fstride);
            }
            *pending = 0.0f;
        }
    }
#undef ADVANCE
    if (g_kill_shard_fx == 1 || vm->particle_cursor == 0) return -1.0f;
    return 1.0f;
}

static void mkpfx_spawnupdate_glass_break(PfxVm* vm) {
    int index;
    for (index = 0; index <= 0xB4; index++) {
        pfx_native_set_rgba(&glass_fragment_alphas[index], 128.0f, 128.0f,
            128.0f, 255.0f - 255.0f * (float)index / 181.0f);
    }
    pfxvm_require_field(vm, 0x307);
    pfxvm_require_field(vm, 0x305);
}

void allow_shard_pfx_now(void) { g_kill_shard_fx = 0; }
void kill_shard_pfx_now(void) { g_kill_shard_fx = 1; }
float get_soul_sine(int index) { return soul_sine[index]; }

int build_sine_table_for_scripts(void) {
    int index;
    int angle;
    if (table_defined_242 == 0) {
        soul_sine[0] = gxMathSin(0.0f);
        for (index = 1, angle = 2; index < 0x400; index++, angle += 2) {
            soul_sine[index] = gxMathSin(3.1415927f * (float)angle * 0.0009765625f);
        }
        table_defined_242 = 1;
    }
    return 0x400;
}

void build_sine_table(void) {
    int index;
    int angle;
    if (table_defined_242 == 0) {
        soul_sine[0] = gxMathSin(0.0f);
        for (index = 1, angle = 2; index < 0x400; index++, angle += 2) {
            soul_sine[index] = gxMathSin(3.1415927f * (float)angle * 0.0009765625f);
        }
        table_defined_242 = 1;
    }
}
