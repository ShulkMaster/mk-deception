#ifndef LIBMKPARTICLE_VM_H
#define LIBMKPARTICLE_VM_H

#include "libmkparticle/color.h"
#include "libmkparticle/pfxmath.h"

typedef struct PfxMetrics PfxMetrics;
typedef struct PfxBehavior PfxBehavior;
typedef struct PfxVm PfxVm;
typedef struct PfxParametricState PfxParametricState;
struct PfxSpawnTable;
struct PfxFieldDescription;

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
    int live;                         /* +0x40 */
    int particle_field_stride; /* +0x44 */
} PfxTransform;

typedef struct PfxRuntimeBuffer {
    unsigned char* particle_data;
    int particle_stride;
    unsigned char* render_data;
    int render_stride;
    char pad10[0x10];
} PfxRuntimeBuffer;

typedef struct PfxTextureFrame {
    float u;
    float v;
} PfxTextureFrame;

typedef union PfxEmitterFlags {
    unsigned char value;
    struct {
        unsigned char cycle_paused : 1;
        unsigned char emission_enabled : 1;
        unsigned char flag_0x20 : 1;
        unsigned char constant_rate : 1;
        unsigned char flags_0x0F : 4;
    } bits;
} PfxEmitterFlags;

typedef union PfxSpawnArguments {
    PfxColor color;
    PfxTextureFrame uv;
    PfxVec3 point;
    struct {
        PfxVec3 minimum;
        PfxVec3 extent;
    } box;
    struct {
        struct PfxSpawnTable* table;
        unsigned int field;
        unsigned int source_field;
    } table;
    struct {
        int minimum;
        int maximum;
    } integer_range;
    struct {
        float minimum;
        float maximum;
    } scalar_range;
    struct {
        PfxVec3 axis;
        float argument0;
        float argument1;
        float argument2;
        float argument3;
        int option;
    } shape;
    struct {
        PfxVec3 offset;
        float minimum_length;
        float length_range;
        unsigned int source_field;
        int clamp_y;
        float y;
    } from_position;
    unsigned int words[8];
    float scalars[8];
} PfxSpawnArguments;

typedef struct PfxEmitterInstruction {
    int opcode;
    int field_description;
    char pad08[0x2C];
    union {
        PfxSpawnArguments spawn;
        union {
            struct {
                int start;
                int end;
            } integer;
            struct {
                float start;
                float end;
            } scalar;
        } value;
    };
} PfxEmitterInstruction;

typedef struct PfxVmEmitter {
    PfxVec3 position;                   /* +0x000 */
    float birth_rate;                   /* +0x00C */
    float partial_birth;                /* +0x010 */
    float age;                          /* +0x014 */
    float lifetime;                     /* +0x018 */
    PfxEmitterFlags flags;              /* +0x01C */
    char pad01D[3];
    float cycle_length;                 /* +0x020 */
    float cycle_length_variation;       /* +0x024 */
    int birth_limit;                    /* +0x028 */
    int cycle_limit;                    /* +0x02C */
    float current_cycle_length;         /* +0x030 */
    float cycle_position;               /* +0x034 */
    int birth_count;                    /* +0x038 */
    int cycle_index;                    /* +0x03C */
    int field_40;                       /* +0x040 */
    PfxEmitterInstruction instructions[8]; /* +0x044 */
    int instruction_count;              /* +0x2E4 */
    union {
        void* user_data;                /* +0x2E8 - memory-placement phase */
        void* transform;                /* +0x2E8 - external matrix view */
        PfxMatrix* pfx_transform;        /* +0x2E8 - particle matrix view */
    };
} PfxVmEmitter;

typedef struct PfxEmitterTransfer {
    int particle_count;
    unsigned char* source;
    int source_stride;
    struct PfxEmitterTransfer* next;
} PfxEmitterTransfer;

typedef void (*PfxSpawnCallback)(PfxVm* pfx, int first_particle,
                                 int particle_count);

struct PfxVm {
    PfxVec3 basis0;                    /* +0x000 */
    char pad00C[4];
    PfxVec3 basis1;                    /* +0x010 */
    char pad01C[0x24];
    unsigned char frame_flags;         /* +0x040 */
    char pad041[3];
    union {
        void* runtime_buffer_a; /* +0x044 -- memory-placement view */
        PfxRuntimeBuffer* typed_runtime_buffer_a;
    };
    union {
        void* runtime_buffer_b; /* +0x048 -- memory-placement view */
        PfxRuntimeBuffer* typed_runtime_buffer_b;
    };
    float elapsed_time;                /* +0x04C */
    int particle_capacity;             /* +0x050 */
    int particle_cursor;               /* +0x054 */
    int active_transform;              /* +0x058 */
    int previous_transform;            /* +0x05C */
    unsigned int flags_0x60;           /* +0x060 */
    int particle_user_data_size;       /* +0x064 */
    void* particle_data;               /* +0x068 */
    int particle_vector_stride;        /* +0x06C */
    PfxTransform transforms[3];        /* +0x070 */
    union {
        PfxParametricState* parametric; /* +0x148 -- parametric runtime view */
        void* name_obj;                 /* +0x148 -- memory-placement view */
    };
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
    float cull_radius;                 /* +0x1BC */
    int emitter_count;                 /* +0x1C0 */
    PfxVmEmitter* emitters;            /* +0x1C4 */
    PfxEmitterTransfer* emitter_transfers; /* +0x1C8 */
    int behavior_count;                /* +0x1CC */
    union {
        void** behaviors;              /* +0x1D0 -- memory-placement view */
        PfxBehavior** behavior_list;   /* +0x1D0 -- behavior-runtime view */
    };
    int flags_0x1D4;                   /* +0x1D4 */
    int field_count;                   /* +0x1D8 */
    union {
        void* field_descriptions; /* +0x1DC -- memory-placement view */
        struct PfxFieldDescription* typed_field_descriptions;
    };
    int field_0x1E0;
    union {
        PfxVec3 field_0x1E4;
        struct {
            float field1E4;
            float field1E8;
            float field1EC;
        };
    };
    PfxVec3 world_position;            /* +0x1F0 */
    PfxVec3 field_0x1FC;
    float field_0x208;
    void* tables[2];                   /* +0x20C */
    char pad214[4];
    PfxSpawnCallback spawn_callback;   /* +0x218 */
    char* name;                        /* +0x21C */
    char pad220[4];
    PfxMetrics* metrics;               /* +0x224 */
    char pad228[4];
    int field_0x22C;
    void* emitter_user_data;           /* +0x230 */
    void* effect_allocations;          /* +0x234 -- linked raw allocations */
    float field238;
    int total_birth_count;              /* +0x23C */
};

typedef char PfxEmitterInstructionSizeCheck[
    (sizeof(PfxEmitterInstruction) == 0x54) ? 1 : -1];
typedef char PfxSpawnArgumentsSizeCheck[
    (sizeof(PfxSpawnArguments) == 0x20) ? 1 : -1];
typedef char PfxEmitterTransferSizeCheck[
    (sizeof(PfxEmitterTransfer) == 0x10) ? 1 : -1];
typedef char PfxVmEmitterSizeCheck[(sizeof(PfxVmEmitter) == 0x2EC) ? 1 : -1];
typedef char PfxVmSizeCheck[(sizeof(PfxVm) == 0x240) ? 1 : -1];

#endif
