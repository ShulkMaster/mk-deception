#ifndef LIBMKPARTICLE_PARTICLE_H
#define LIBMKPARTICLE_PARTICLE_H

#include "libmkparticle/vm.h"
#include "libmkparticle/particle_system.h"
#include "libmkparticle/particle_render.h"

typedef struct RwTexture RwTexture;
typedef struct RwCamera RwCamera;

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
    float field_0x500; /* +0xC0 */
    PfxVec3 field_0x501; /* +0xC4 */
    float field_0x502; /* +0xD0 */
    int widescreen_x; /* +0xD4 */
    int widescreen_y; /* +0xD8 */
    RwCamera* camera; /* +0xDC */
} PfxSystemGlobals;

extern PfxSystemGlobals pfxsystem_globals;

void pfx_halt(const char* message);
void pfx_parametric_spawn(PfxVm* pfx, float frame_time);
void pfx_parametric_update(PfxVm* pfx, float frame_time);
void pfx_run(PfxVm* pfx, float frame_time);
void update_live_particles(PfxVm* pfx);
void pfx_reset_renderstate(void);
int pfx_verify(PfxVerifyView* pfx);
int get_field_size(int type);
int pfx_field_get_type(int field);
int pfx_get_struct_size(PfxVm* pfx, int field);

void pfxvm_require_field(PfxVm* pfx, int field);

#endif
