#ifndef LIBMKPARTICLE_VM_H
#define LIBMKPARTICLE_VM_H

#include "libmkparticle/color.h"
#include "libmkparticle/pfxmath.h"

typedef struct PfxMetrics PfxMetrics;

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
    PfxVec3 position;                   /* +0x000 */
    float lifetime;                     /* +0x00C */
    char pad010[0x30];
    int field_40;                       /* +0x040 */
    char pad44[0x2A4];
    union {
        void* user_data;                /* +0x2E8 - memory-placement phase */
        void* transform;                /* +0x2E8 - bound-render phase */
    };
} PfxVmEmitter;

typedef struct PfxVm {
    PfxVec3 basis0;                    /* +0x000 */
    char pad00C[4];
    PfxVec3 basis1;                    /* +0x010 */
    char pad01C[0x28];
    void* runtime_buffer_a;            /* +0x044 */
    void* runtime_buffer_b;            /* +0x048 */
    char pad04C[4];
    int particle_capacity;             /* +0x050 */
    int particle_cursor;               /* +0x054 */
    int active_transform;              /* +0x058 */
    char pad05C[4];
    unsigned int flags_0x60;           /* +0x060 */
    int particle_user_data_size;       /* +0x064 */
    void* particle_data;               /* +0x068 */
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
    char pad1C8[4];
    int behavior_count;                /* +0x1CC */
    void** behaviors;                  /* +0x1D0 */
    int flags_0x1D4;                   /* +0x1D4 */
    int field_count;                   /* +0x1D8 */
    void* field_descriptions;          /* +0x1DC */
    char pad1E0[4];
    float field1E4;
    float field1E8;
    float field1EC;
    char pad1F0[0x2C];
    char* name;                        /* +0x21C */
    char pad220[4];
    PfxMetrics* metrics;               /* +0x224 */
    char pad228[4];
    int field_0x22C;
    void* emitter_user_data;           /* +0x230 */
    void* effect_allocations;          /* +0x234 -- linked raw allocations */
    float field238;
    char pad23C[4];
} PfxVm;

typedef char PfxVmEmitterSizeCheck[(sizeof(PfxVmEmitter) == 0x2EC) ? 1 : -1];
typedef char PfxVmSizeCheck[(sizeof(PfxVm) == 0x240) ? 1 : -1];

#endif
