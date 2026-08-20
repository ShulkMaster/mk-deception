#ifndef LIBMKPARTICLE_PARTICLE_H
#define LIBMKPARTICLE_PARTICLE_H

#include "libmkparticle/vm.h"

typedef struct RwTexture RwTexture;
typedef struct RwCamera RwCamera;

typedef struct PfxLiveSlot {
    int live;
    char pad04[0x44];
} PfxLiveSlot; /* 0x48 */

typedef struct PfxRuntimeView {
    char pad00[0x54];
    int live; /* +0x54 */
    int active_slot; /* +0x58 */
    char pad5C[0x54];
    PfxLiveSlot slots[1]; /* +0xB0 -- trailing slot table */
} PfxRuntimeView;

typedef struct PfxRenderView {
    char pad00[0x30];
    float source_x; /* +0x30 */
    float source_y; /* +0x34 */
    float source_z; /* +0x38 */
    char pad3C[0x114];
    union {
        unsigned char flags; /* +0x150 */
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
    float render_x; /* +0x164 */
    float render_y; /* +0x168 */
    float render_z; /* +0x16C */
    char pad170[0x04];
    RwTexture* texture; /* +0x174 */
    char pad178[0x04];
    int blend_mode; /* +0x17C */
} PfxRenderView;

typedef struct PfxEmitterView {
    char bytes[0x2EC];
} PfxEmitterView;

typedef struct PfxEmitterFlagsView {
    char pad00[0x1C];
    unsigned char high_bit : 1;
    unsigned char : 7;
} PfxEmitterFlagsView;

typedef struct PfxEmitterTableView {
    char pad00[0x1C0];
    int emitter_count; /* +0x1C0 */
    PfxEmitterView* emitters; /* +0x1C4; stride 0x2EC */
} PfxEmitterTableView;

typedef struct PfxVerifyView {
    char pad00[0x150];
    unsigned char byte_flags; /* +0x150 */
    char pad151[0x83];
    unsigned int flags; /* +0x1D4 */
} PfxVerifyView;

/*
 * libmkparticle particle.o - thin frame helpers for boot Render path.
 * VM / emitter body left as NonMatching stubs (see particle.c).
 */

typedef struct PfxSystemGlobals {
    char pad00[0x80];
    union {
        float camera_facing[16]; /* +0x80 4x4 */
        struct {
            PfxVec3 billboard_axis0;
            float camera_facing_0C;
            PfxVec3 billboard_axis1;
            float camera_facing_1C;
            float camera_facing_tail[8];
        };
    };
    char padC0[0x14];
    int widescreen_x; /* +0xD4 */
    int widescreen_y; /* +0xD8 */
    RwCamera* camera; /* +0xDC */
} PfxSystemGlobals;

extern PfxSystemGlobals pfxsystem_globals;

void pfxsystem_frame_begin(void);
void pfxsystem_skip_render_frame(void);
void pfxsystem_set_frame_info(int unused0, int unused1, const float* matrix,
                              RwCamera* camera);
void pfxsystem_widescreen_offset(int x, int y);
void get_pfxsystem_widescreen_offset(int* out_x, int* out_y);
void pfxsystem_init(void);
void pfxsystem_set_global(int id, float value);
int pfx_frame_begin(void* pfx);
void pfx_frame_end(void* pfx);
void pfx_frame_end_check(void* pfx);
void pfx_count_begin(void);
void pfx_count_end(void);
void pfx_count_add(void* pfx);
void pfx_set_texture(PfxRenderView* pfx, RwTexture* texture);
void update_live_particles(PfxRuntimeView* pfx);
void pfx_set_renderstate(PfxRenderView* pfx);
void pfx_reset_renderstate(void);
void pfx_render_set_blendmode(PfxRenderView* pfx, int mode);
PfxEmitterView* pfx_get_emitter(PfxEmitterTableView* pfx, int index);
int pfx_verify(PfxVerifyView* pfx);
int pfx_field_get_type(unsigned int field);

#endif
