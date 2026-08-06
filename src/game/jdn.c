#include "libmkparticle/color.h"

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;

typedef struct PfxSpawnResult {
    void* pfx;
    void* emitter;
} PfxSpawnResult;

typedef struct JdnDestroyVtable {
    void* reserved[4];
    void (*destroy)(void* object);
} JdnDestroyVtable;

typedef struct JdnDestroyable {
    JdnDestroyVtable* vtable;
    unsigned int instance;
} JdnDestroyable;

typedef struct GlassEmitterObject {
    char pad00[8];
    unsigned char flags;
    char pad09[0x97];
    Vec3 position;
} GlassEmitterObject;

typedef struct GlassEmitter {
    char pad000[0x28];
    float vertical_speed;
    char pad02C[0x14];
    char texture_state;
    char pad041[0x181];
    short active;
    char pad1C4[0x98];
    const char* effect_name;
    char pad260[0x28];
    unsigned long owner;
    unsigned long lifetime;
    unsigned long enabled;
    unsigned long shard_count;
    float mode_scale_a;
    float mode_scale_b;
    char pad2A4[8];
    Vec3 center;
    PfxColor* alphas;
} GlassEmitter;

static const char stringBase0[] = "C - glass shards\0C.glass_shard";

static void mkpfx_spawnupdate_glass_break(void* vm);
void pfx_glass_break_run(void);

static const unsigned long noch_pfx_table[] = {
    0x00000005, 0x00000272, 0x00000003, 0x00000000, 0xBB03126F, 0x00000000, 0x3E4CCCCD,
    0x42F00000, 0x42F00000, 0x42C80000, 0x437F0000, 0x00000096, 0x00000064, 0x00000000,
    0x00000000, 0x00000100, 0x00000040, 0x00000055, 0x0000000C, 0x40000000, 0x00000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, (unsigned long)mkpfx_spawnupdate_glass_break,
};

static PfxColor glass_fragment_alphas[0xB5];
static float soul_sine[0x400];
static unsigned long table_defined_242;
static unsigned long g_kill_shard_fx;

static const float kNeg50 = -50.0f;
static const float k01 = 0.1f;
static const float k05 = 0.5f;
static const float k015 = 0.15f;
static const float k005 = 0.05f;
static const float k02 = 0.2f;
static const float k016 = 0.16f;
static const float k003 = 0.03f;
static const float k90 = 90.0f;
static const float k130 = 130.0f;
static const float k255 = 255.0f;
static const float k181 = 181.0f;
static const float k135 = 1.35f;
static const float k3 = 3.0f;
static const float k5 = 5.0f;
static const double kIeeeBase = 4503599627370496.0;
static const double kPowA = 0.99;
static const double kPowB = 0.975;
static const float kGravity = 0.003f;
static const float kZero = 0.0f;
static const float kNegHalf = -0.5f;
static const float k08 = 0.8f;
static const float k001 = 0.01f;
static const float k15 = 15.0f;
static const float k004 = 0.04f;
static const float k007 = 0.07f;
static const float kTwoPi = 6.2831855f;
static const float kNeg1 = -1.0f;
static const float k1 = 1.0f;
static const double kIeeeBase2 = 4503601774854144.0;
static const float k128 = 128.0f;
static const float kPi = 3.1415927f;
static const float kInv1024 = 0.0009765625f;

extern void* apfx;
extern void* apfx_emitter_obj;
extern float game_speed;
extern unsigned long reseed_rnd_tbl;

void* create_pfx(int id, void* runFn, void* out, void* table, const char* name);
void* pfx_get_emitter_obj(void* pfx, int index);
void as_mkhdr(void* obj);
void update_mkobj(void* obj);
void set_pfx_texture(void* texState, int texId, int artId);
void pfx_texture_animate(void* texState, int a, float b, int c, int d, int e);
void* pfx_get_emitter(void* vm, int index);
void* pfx_get_field(void* vm, int sign, int field);
void pfxvm_require_field(void* vm, int field);
void memcpy(void* dst, const void* src, unsigned long n);
double pow(double x, double y);
float frand(float scale);
float sfrand(float scale);
unsigned long randu0(unsigned long max);
unsigned long __cvt_fp2unsigned(float val);
void reload_rnd_tbl(void);
float gxMathSin(float angle);

static float glass_alpha_for_index(int idx) {
    double v;

    v = (double)idx;
    v = v + kIeeeBase2;
    v = v - kIeeeBase2;
    v = k255 * v;
    v = v / k181;
    return (float)(k255 - v);
}

void* start_pfx_glass_shards(Vec3* pos, int artId, Vec3* center, void* owner, int shardMode,
                             int shardCount) {
    PfxSpawnResult spawn;
    GlassEmitterObject* emitterObj;
    JdnDestroyable* pfx;
    GlassEmitter* emitter;
    float modeScaleA;
    float modeScaleB;
    int i;

    spawn.pfx = 0;
    spawn.emitter = 0;
    pfx = (JdnDestroyable*)create_pfx(
        -0x7FDD, pfx_glass_break_run, &spawn, (void*)noch_pfx_table, stringBase0);
    if (pfx == 0 || spawn.emitter == 0) {
        return 0;
    }
    emitterObj = (GlassEmitterObject*)pfx_get_emitter_obj(pfx, 0);
    if (emitterObj == 0) {
        if (pfx->instance != 0) {
            if (pfx->vtable->destroy != 0) {
                pfx->vtable->destroy(pfx);
            }
        }
        return 0;
    }
    emitterObj->position = *pos;
    emitterObj->flags = (emitterObj->flags & 0xDF) | 0x40;
    if (emitterObj->flags & 0x40) {
        as_mkhdr(emitterObj);
    }
    update_mkobj(emitterObj);
    emitter = (GlassEmitter*)spawn.emitter;
    emitter->vertical_speed = kNeg50;
    emitter->center = *center;
    emitter->owner = (unsigned long)owner;
    emitter->mode_scale_b = k05;
    emitter->lifetime = 0xB4;
    emitter->alphas = glass_fragment_alphas;
    emitter->enabled = 1;
    emitter->mode_scale_a = k015;
    emitter->shard_count = (unsigned long)shardCount;
    modeScaleA = k015;
    modeScaleB = k05;
    if (shardMode == 0) {
        modeScaleA = k015;
        modeScaleB = k015;
    } else if (shardMode == 1) {
        modeScaleA = k05;
        modeScaleB = k05;
    } else if (shardMode == 2) {
        modeScaleA = k005;
        modeScaleB = k005;
    } else if (shardMode == 3) {
        modeScaleA = k02;
        modeScaleB = k02;
    } else if (shardMode == 4) {
        modeScaleA = k016;
        modeScaleB = k003;
    }
    emitter->mode_scale_a = modeScaleA;
    emitter->mode_scale_b = modeScaleB;
    set_pfx_texture(&emitter->texture_state, 0x21E, artId);
    if ((unsigned long)(artId - 0x13D) <= 8U) {
        for (i = 0; i <= 0xB4; i++) {
            float a = glass_alpha_for_index(i);
            pfx_native_set_rgba(&glass_fragment_alphas[i], k90, k130, k90, a);
        }
        emitter->mode_scale_a *= k135;
        emitter->mode_scale_b *= k135;
        pfx_texture_animate(&emitter->texture_state, 0x80, k3, 0x20, 0x20, 0x10);
    } else if ((unsigned long)(artId - 0x13D) == 9U) {
        pfx_texture_animate(&emitter->texture_state, 0x80, k5, 0x20, 0x20, 0x10);
    } else {
        pfx_texture_animate(&emitter->texture_state, 0x100, k5, 0x40, 0x40, 0x10);
    }
    {
        float* emitterTime = pfx_get_emitter(&emitter->texture_state, 0);
        emitterTime[3] = (float)((double)(unsigned long)owner - kIeeeBase);
    }
    emitter->active = 1;
    emitter->effect_name = stringBase0 + 0x11;
    return spawn.emitter;
}

void pfx_glass_break_run(void) {
    void* vm;
    void* emitterBase;
    float powA;
    float powB;
    float gravityStep;
    int particleCount;
    int particleStride;
    int i;
    float returnVal;

    vm = apfx;
    powA = (float)pow((double)game_speed, kPowA);
    powB = (float)pow((double)game_speed, kPowB);
    gravityStep = kGravity * game_speed;
    emitterBase = (char*)vm + 0x40;
    particleCount = ((int*)emitterBase)[0x15];
    particleStride = ((int*)emitterBase)[0x2D];
    for (i = 0; i < particleCount; i++) {
        float* life = (float*)pfx_get_field(emitterBase, -2, 0x305);
        float* posX = (float*)pfx_get_field(emitterBase, -1, 0x100);
        float* vel = (float*)pfx_get_field(emitterBase, -2, 0x103);
        float* alpha = (float*)pfx_get_field(emitterBase, -1, 0x307);
        (void)life;
        (void)posX;
        (void)vel;
        (void)alpha;
        (void)powA;
        (void)powB;
        (void)gravityStep;
    }
    returnVal = kZero;
    if (g_kill_shard_fx == 1) {
        returnVal = kNeg1;
    } else if (((int*)emitterBase)[0x15] == 0) {
        returnVal = kNeg1;
    } else {
        returnVal = k1;
    }
    {
        float* emitterTime = pfx_get_emitter(emitterBase, 0);
        emitterTime[3] = returnVal;
    }
}

static void mkpfx_spawnupdate_glass_break(void* vm) {
    PfxColor* color;
    int i;

    color = glass_fragment_alphas;
    for (i = 0; i <= 0xB4; i++, color++) {
        pfx_native_set_rgba(color, k128, k128, k128,
                            k255 - ((k255 * (float)i) / k181));
    }
    pfxvm_require_field(vm, 0x307);
    pfxvm_require_field(vm, 0x305);
}

void allow_shard_pfx_now(void) {
    g_kill_shard_fx = 0;
}

void kill_shard_pfx_now(void) {
    g_kill_shard_fx = 1;
}

float get_soul_sine(int index) {
    return soul_sine[index];
}

static void fill_sine_table(void) {
    int i;
    int angle;

    if (table_defined_242 != 0) {
        return;
    }
    soul_sine[0] = gxMathSin(kZero);
    for (i = 1, angle = 2; i < 0x400; i++, angle += 2) {
        double v = (double)angle + kIeeeBase;
        float rad;

        v = v - kIeeeBase;
        v = v * kPi;
        rad = (float)(v * kInv1024);
        soul_sine[i] = gxMathSin(rad);
    }
    table_defined_242 = 1;
}

int build_sine_table_for_scripts(void) {
    fill_sine_table();
    return 0x400;
}

void build_sine_table(void) {
    fill_sine_table();
}
