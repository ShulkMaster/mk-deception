#include "libmkparticle/particle.h"
#include "libmkparticle/rw_engine.h"

void* memcpy(void* dst, const void* src, unsigned long n);

void pfxmetrics_begin_frame(void);
void streampool_nextframe(void);
void streampool_skiprenderstream(void);
void streampool_init(void);

int RwRenderStateSet_SRCBLEND_DESTBLEND(int src, int dst);
void RwRenderStateSet_rwRENDERSTATECULLMODE(int mode);

static const float s_zero = 0.0f;

static int cullmode;
static int dstBlend;
static int srcBlend;

typedef struct PfxLiveSlot {
    int live;
    char pad04[0x44];
} PfxLiveSlot; /* 0x48 */

typedef struct PfxRuntimeView {
    char pad00[0x54];
    int live;              /* +0x54 */
    int active_slot;       /* +0x58 */
    char pad5C[0x54];
    PfxLiveSlot slots[1];  /* +0xB0 */
} PfxRuntimeView;

typedef struct PfxRenderView {
    char pad00[0x30];
    float source_x;       /* +0x30 */
    float source_y;       /* +0x34 */
    float source_z;       /* +0x38 */
    char pad3C[0x114];
    union {
        unsigned char flags;  /* +0x150 */
        struct {
            unsigned char field_0x150_80 : 1;
            unsigned char field_0x150_40 : 1;
            unsigned char has_texture : 1; /* 0x20 */
            unsigned char field_0x150_10 : 1;
            unsigned char field_0x150_08 : 1;
            unsigned char field_0x150_04 : 1;
            unsigned char field_0x150_02 : 1;
            unsigned char field_0x150_01 : 1;
        };
    };
    char pad151[0x13];
    float render_x;       /* +0x164 */
    float render_y;       /* +0x168 */
    float render_z;       /* +0x16C */
    char pad170[0x04];
    void* texture;        /* +0x174 */
    char pad178[0x04];
    int blend_mode;       /* +0x17C */
} PfxRenderView;

typedef struct PfxEmitterView {
    char bytes[0x2EC];
} PfxEmitterView;

typedef struct PfxEmitterTableView {
    char pad00[0x1C0];
    int emitter_count;           /* +0x1C0 */
    PfxEmitterView* emitters;    /* +0x1C4; stride 0x2EC */
} PfxEmitterTableView;

typedef struct PfxVerifyView {
    char pad00[0x150];
    unsigned char byte_flags; /* +0x150 */
    char pad151[0x83];
    unsigned int flags;       /* +0x1D4 */
} PfxVerifyView;

/* --- retail order: deferred VM stubs, then lifted frame helpers --- */

int get_propfield_size(int type) {
    (void)type;
    return 0;
}

static int get_renderfield_size(int type) {
    (void)type;
    return 0;
}

int get_field_size(int type) {
    return get_propfield_size(type) + get_renderfield_size(type);
}

int pfx_field_get_type(void* field) {
    (void)field;
    return 0;
}

void pfx_set_texture(PfxRenderView* pfx, void* texture) {
    pfx->texture = texture;
    if (pfx->texture == 0) {
        return;
    }
    pfx->has_texture = 1;
}

void pfx_frame_begin(void) {
}

void pfx_frame_end(void) {
}

void pfx_frame_end_check(void) {
}

void update_live_particles(PfxRuntimeView* pfx) {
    int live;
    int index;

    /* Soft ceiling: 98% -- retail loads live before the slot index. */
    live = pfx->live;
    index = pfx->active_slot;
    pfx->slots[index].live = live;
}

void* pfx_get_field(void* pfx, int index, int type) {
    (void)pfx;
    (void)index;
    (void)type;
    /* Soft ceiling: full field lookup deferred with particle VM. */
    return 0;
}

static int pfx_memory_is_set(void) {
    return 0;
}

void pfxvm_require_field(void* pfx, int type) {
    (void)pfx;
    (void)type;
    (void)pfx_memory_is_set;
}

int pfx_get_struct_size(void* pfx, int type) {
    (void)pfx;
    (void)type;
    return 0;
}

void pfx_halt(void) {
}

void pfx_count_begin(void) {
}

void pfx_count_end(void) {
}

void pfx_count_add(int n) {
    (void)n;
}

static void v3_x_mat_4(float* out, float* v, float* m) {
    (void)out;
    (void)v;
    (void)m;
}

void pfx_parametric_spawn(void* pfx) {
    (void)pfx;
}

void pfx_parametric_update(void* pfx) {
    (void)pfx;
}

void pfx_run(void* pfx) {
    (void)pfx;
}

void pfxsystem_frame_begin(void) {
    pfxmetrics_begin_frame();
    streampool_nextframe();
}

void pfxsystem_skip_render_frame(void) {
    streampool_skiprenderstream();
}

void pfxsystem_set_frame_info(int unused0, int unused1, void* matrix, void* camera) {
    int i;
    float* row;

    (void)unused0;
    (void)unused1;
    memcpy(pfxsystem_globals.camera_facing, matrix, 0x40);
    for (i = 0; i < 4; i++) {
        row = &pfxsystem_globals.camera_facing[i * 4];
        /* Retail zeros translation column slots at +0x8C stride 0x10. */
        row[3] = s_zero;
    }
    /* camera_facing is at +0x80; +0x8C is element [0][3] of first row --- retail
     * writes four floats at +0x8C,+0x9C,+0xAC,+0xBC (column 3 of each row). */
    pfxsystem_globals.camera = camera;
}

void pfxsystem_widescreen_offset(int x, int y) {
    pfxsystem_globals.widescreen_x = x;
    pfxsystem_globals.widescreen_y = y;
}

void get_pfxsystem_widescreen_offset(int* out_x, int* out_y) {
    *out_x = pfxsystem_globals.widescreen_x;
    *out_y = pfxsystem_globals.widescreen_y;
}

void pfxsystem_init(void) {
    streampool_init();
}

void pfxsystem_set_global(int id, float value) {
    float* slot;

    if ((id & 0xF00) == 0x500) {
        slot = (float*)pfx_get_field(0, -2, id);
        if (slot != 0) {
            *slot = value;
        }
    }
}

void pfx_set_renderstate(PfxRenderView* pfx) {
    unsigned char flags;

    flags = pfx->flags;
    if (((flags >> 3) & 1) != 0 && ((flags >> 2) & 1) != 0) {
        pfx->render_x = pfx->source_x;
        pfx->render_y = pfx->source_y;
        pfx->render_z = pfx->source_z;
    }

    RwEngineInstance->fpRenderStateGet(0x14, &cullmode);
    RwRenderStateSet_rwRENDERSTATECULLMODE(1);
    RwEngineInstance->fpRenderStateGet(0xA, &srcBlend);
    RwEngineInstance->fpRenderStateGet(0xB, &dstBlend);

    if (pfx->blend_mode == 1) {
        RwRenderStateSet_SRCBLEND_DESTBLEND(5, 2);
    } else {
        RwRenderStateSet_SRCBLEND_DESTBLEND(5, 6);
    }
}

void pfx_reset_renderstate(void) {
    RwRenderStateSet_SRCBLEND_DESTBLEND(srcBlend, dstBlend);
    RwRenderStateSet_rwRENDERSTATECULLMODE(cullmode);
}

void pfx_render_set_blendmode(PfxRenderView* pfx, int mode) {
    pfx->blend_mode = mode;
}

void* pfx_get_emitter(PfxEmitterTableView* pfx, int index) {
    int count;

    if (index < 0) {
        return 0;
    }
    count = pfx->emitter_count;
    if (index >= count) {
        return 0;
    }
    return &pfx->emitters[index];
}

int pfx_verify(PfxVerifyView* pfx) {
    unsigned int flags;
    unsigned char b;

    flags = pfx->flags;
    if ((flags & 0x100) != 0 && (flags & 0x200) != 0) {
        return 0;
    }
    b = pfx->byte_flags;
    if (((b >> 5) & 1) == 0) {
        return 0;
    }
    return 1;
}
