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

typedef struct PfxParametricParticle {
    PfxVec3 position;
    PfxVec3 velocity;
    float birth_time;
    float size;
    float texture;
    PfxColor color;
} PfxParametricParticle;

struct PfxParametricState {
    float texture_curve[64];
    int texture_curve_count;
    char pad104[0x0C];
    float size_curve[64];
    int size_curve_count;
    char pad214[0x0C];
    PfxColor color_curve[64];
    int color_curve_count;
    char pad324[0x0C];
    PfxVec3 acceleration;
    float vertical_acceleration;
    float damping;
    float texture_rate;
    int particle_capacity;
    int particle_cursor;
    float lifetime;
    float minimum_y;
    PfxParametricParticle particles[];
};

typedef struct PfxTransform {
    union {
        PfxMatrix matrix;
        struct {
            char pad00[0x30];
            PfxVec3 position;
            char pad3C[4];
        };
    };
    int live;
    int particle_field_stride;
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
        union {
            struct PfxSpawnTable* table;
            int table_index;
        };
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
        PfxVec3 origin;
        float minimum_radius;
        float maximum_radius;
        int quadratic_radius;
    } sphere;
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
    int storage_type;
    int field_offset;
    char pad10[0x24];
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
    PfxVec3 position;
    float birth_rate;
    float partial_birth;
    float age;
    float lifetime;
    PfxEmitterFlags flags;
    char pad01D[3];
    float cycle_length;
    float cycle_length_variation;
    int birth_limit;
    int cycle_limit;
    float current_cycle_length;
    float cycle_position;
    int birth_count;
    int cycle_index;
    int field_40;
    PfxEmitterInstruction instructions[8];
    int instruction_count;
    union {
        void* user_data;
        void* transform;
        PfxMatrix* pfx_transform;
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
    union {
        PfxMatrix matrix;
        struct {
            PfxVec3 basis0;
            char pad00C[4];
            PfxVec3 basis1;
            char pad01C[0x24];
        };
    };
    struct {
        unsigned char disabled : 1;
        unsigned char cull_enabled : 1;
        unsigned char visible : 1;
        unsigned char bit4 : 1;
        unsigned char frame_end_pending : 1;
        unsigned char low_bits : 3;
    } frame_flag_bits;
    char pad041[3];
    union {
        void* runtime_buffer_a;
        PfxRuntimeBuffer* typed_runtime_buffer_a;
    };
    union {
        void* runtime_buffer_b;
        PfxRuntimeBuffer* typed_runtime_buffer_b;
    };
    float elapsed_time;
    int particle_capacity;
    int particle_cursor;
    int active_transform;
    int previous_transform;
    int flags_0x60;
    int particle_user_data_size;
    void* particle_data;
    int particle_vector_stride;
    PfxTransform transforms[3];
    union {
        PfxParametricState* parametric;
        void* name_obj;
    };
    char pad14C[4];
    unsigned char flag150_80 : 1;
    unsigned char flag150_40 : 1;
    unsigned char flag150_20 : 1;
    unsigned char flag150_10 : 1;
    unsigned char flag150_08 : 1;
    unsigned char flag150_04 : 1;
    unsigned char flag150_02 : 1;
    unsigned char flag150_01 : 1;
    unsigned char flag151_80 : 1;
    unsigned char flag151_40 : 1;
    unsigned char flag151_20 : 1;
    unsigned char flag151_10 : 1;
    unsigned char : 2;
    unsigned char flag151_02 : 1;
    unsigned char : 1;
    char pad152[2];
    float light_attenuation_k0;
    float light_attenuation_k1;
    float light_attenuation_k2;
    PfxColor light_color;
    PfxVec3 light_position;
    float z_bias;
    struct RwTexture* render_texture;
    struct RwTexture* alpha_texture;
    int render_blend_mode;
    short texture_frame_count;
    short texture_mode;
    float texture_frame_time;
    float texture_u_step;
    float texture_v_step;
    PfxTextureFrame* texture_frames;
    PfxVec3 geometry_axis0;
    PfxVec3 geometry_axis1;
    float geometry_scale0;
    float geometry_scale1;
    PfxColor color1B4;
    float billboard_size;
    float cull_radius;
    int emitter_count;
    PfxVmEmitter* emitters;
    PfxEmitterTransfer* emitter_transfers;
    int behavior_count;
    union {
        void** behaviors;
        PfxBehavior** behavior_list;
    };
    int flags_0x1D4;
    int field_count;
    union {
        void* field_descriptions;
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
    PfxVec3 world_position;
    PfxVec3 field_0x1FC;
    float field_0x208;
    void* tables[2];
    void (*transform_callback)(void);
    PfxSpawnCallback spawn_callback;
    char* name;
    int effect_id;
    PfxMetrics* metrics;
    float effect_scale;
    int field_0x22C;
    void* emitter_user_data;
    void* effect_allocations;
    float field238;
    int total_birth_count;
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
