#ifndef LIBMKPARTICLE_VM_H
#define LIBMKPARTICLE_VM_H

#include "libmkparticle/color.h"
#include "libmkparticle/pfxmath.h"

typedef struct PfxVec3 {
    float x;
    float y;
    float z;
} PfxVec3;

typedef struct PfxTransform {
    union {
        PfxMatrix matrix;
        struct {
            char pad00[0x30];
            PfxVec3 position;
            char pad3C[4];
        };
    };
    char pad40[4];
    int particle_field_stride; /* +0x44 */
} PfxTransform;

typedef struct PfxTextureFrame {
    float u;
    float v;
} PfxTextureFrame;

typedef struct PfxVmEmitter {
    char pad00[0x40];
    void* field40;
    char pad44[0x2EC - 0x44];
} PfxVmEmitter;

typedef struct PfxVm {
    PfxVec3 basis0;                    /* +0x000 */
    char pad00C[4];
    PfxVec3 basis1;                    /* +0x010 */
    char pad01C[0x34];
    int particle_capacity;             /* +0x050 */
    int particle_cursor;               /* +0x054 */
    int active_transform;              /* +0x058 */
    char pad05C[4];
    unsigned int flags_0x60;           /* +0x060 */
    char pad064[8];
    int particle_vector_stride;        /* +0x06C */
    PfxTransform transforms[3];        /* +0x070 */
    void* name_obj;                      /* +0x148 */
    char pad14C[4];
    union {
        unsigned char flags150;
        struct {
            unsigned char flag150_80 : 1;
            unsigned char flag150_40 : 1;
            unsigned char flag150_20 : 1;
            unsigned char flag150_10 : 1;
            unsigned char flag150_08 : 1;
            unsigned char flag150_04 : 1;
            unsigned char flag150_02 : 1;
            unsigned char flag150_01 : 1;
        };
    };
    union {
        unsigned char flags151;
        struct {
            unsigned char flag151_80 : 1;
            unsigned char flags151_low : 7;
        };
    };
    char pad152[0x2E];
    short texture_frame_count;         /* +0x180 */
    char pad182[2];
    float texture_frame_time;          /* +0x184 */
    float texture_u_step;              /* +0x188 */
    float texture_v_step;              /* +0x18C */
    PfxTextureFrame* texture_frames;   /* +0x190 */
    PfxVec3 geometry_axis0;            /* +0x194 */
    PfxVec3 geometry_axis1;            /* +0x1A0 */
    float geometry_scale0;             /* +0x1AC */
    float geometry_scale1;             /* +0x1B0 */
    PfxColor color1B4;                 /* +0x1B4 */
    float billboard_size;              /* +0x1B8 */
    char pad1BC[4];
    int emitter_count;                 /* +0x1C0 */
    PfxVmEmitter* emitters;            /* +0x1C4 */
    char pad1C8[0x0C];
    int flags_0x1D4;                   /* +0x1D4 */
    char pad1D8[0x0C];
    float field1E4;
    float field1E8;
    float field1EC;
    char pad1F0[0x3C];
    int field_0x22C;
    char pad230[8];
    float field238;
    char pad23C[4];
} PfxVm;

void* pfx_effect_memory_alloc(PfxVm* vm, int size, int align);

#endif
