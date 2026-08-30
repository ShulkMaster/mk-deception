#ifndef LIBMKPARTICLE_BEHAVIOR_H
#define LIBMKPARTICLE_BEHAVIOR_H

#include "libmkparticle/fields.h"
#include "libmkparticle/range.h"

typedef struct PfxVm PfxVm;

typedef struct PfxVmField {
    unsigned int description;
    int stream;
    int offset;
} PfxVmField;

typedef union PfxUpdateArguments {
    PfxVmField field;
    struct {
        float x;
        float y;
        float z;
        float scale;
    } vector;
    struct {
        void* table;
        PfxVmField index_field;
    } table;
    struct {
        PfxVmField age_field;
        float start_time;
        float duration;
        int start_alpha;
        int end_alpha;
    } fade;
    struct {
        PfxVmField age_field;
        float duration;
        int last_color;
        int first_color;
        void* colors;
    } color_lerp;
    struct {
        PfxVmField age_field;
        float frame_time;
        short frame_count;
        short mode;
        int first_frame;
        void* frame_source;
    } texture_anim;
    unsigned int words[7];
    float scalars[7];
} PfxUpdateArguments;

typedef struct PfxKillInstruction {
    int opcode;
    PfxVmField field;
    int field_10; /* component byte offset within the selected field */
    struct PfxBehavior* target;
    int field_18; /* threshold comes from reference_field when nonzero */
    union {
        int field_1C;
        int integer;
        float scalar;
        unsigned int reference_field;
    };
} PfxKillInstruction;

typedef struct PfxUpdateInstruction {
    int opcode;
    union {
        int field_04;
        float scalar_04;
    };
    PfxVmField field;
    union {
        PfxUpdateArguments arguments;
        struct {
            unsigned int argument_0x14;
            unsigned int argument_0x18;
            int argument_offset;
            char pad20[0xC];
            void* target;
        };
    };
} PfxUpdateInstruction;

typedef struct PfxInitInstruction {
    int opcode;
    PfxVmField field;
    union {
        PfxFloatRange range;
        unsigned int source_field;
    } argument;
} PfxInitInstruction;

typedef struct PfxBehavior {
    int particle_count;                    /* +0x000 */
    int active_particle_count;             /* +0x004 */
    union {
        struct {
            unsigned char* previous_stream_100; /* +0x008 */
            int stream_100_stride;              /* +0x00C */
            unsigned char* previous_stream_300; /* +0x010 */
            int stream_300_stride;              /* +0x014 */
        };
        PfxFieldBuffer previous_streams[2];
    };
    void* auxiliary_stream_100;            /* +0x018 */
    int auxiliary_stream_100_stride;       /* +0x01C */
    char pad020[8];
    union {
        struct {
            unsigned char* stream_100;     /* +0x028 */
            int current_stream_100_stride; /* +0x02C */
            unsigned char* stream_300;     /* +0x030 */
            int current_stream_300_stride; /* +0x034 */
        };
        PfxFieldBuffer current_streams[2];
    };
    void* auxiliary_stream_300;            /* +0x038 */
    int auxiliary_stream_300_stride;       /* +0x03C */
    char pad040[8];
    PfxVm* effect;                         /* +0x048 */
    unsigned int age_field;                /* +0x04C */
    int has_age_field;                     /* +0x050 */
    int age_field_offset;                  /* +0x054 */
    union {
        unsigned char segment_0x58[0x84];
        struct {
            int kill_instruction_count;
            PfxKillInstruction kill_instructions[4];
        };
    };
    union {
        unsigned char segment_0xDC[0x244];
        struct {
            int update_instruction_count;
            PfxUpdateInstruction update_instructions[12];
        };
    };
    union {
        unsigned char segment_0x320[0x64];
        struct {
            int init_instruction_count;
            PfxInitInstruction init_instructions[4];
        };
    };
    void* link_0x384;
} PfxBehavior;

typedef char PfxVmFieldSizeCheck[(sizeof(PfxVmField) == 0xC) ? 1 : -1];
typedef char PfxKillInstructionSizeCheck[
    (sizeof(PfxKillInstruction) == 0x20) ? 1 : -1];
typedef char PfxUpdateInstructionSizeCheck[
    (sizeof(PfxUpdateInstruction) == 0x30) ? 1 : -1];
typedef char PfxUpdateArgumentsSizeCheck[
    (sizeof(PfxUpdateArguments) == 0x1C) ? 1 : -1];
typedef char PfxInitInstructionSizeCheck[
    (sizeof(PfxInitInstruction) == 0x18) ? 1 : -1];
typedef char PfxBehaviorSizeCheck[(sizeof(PfxBehavior) == 0x388) ? 1 : -1];

int pfx_num_behaviors(PfxVm* pfx);
PfxBehavior* pfx_behavior(PfxVm* pfx, int index);
void bind_behavior_to_effect(PfxBehavior* behavior, PfxVm* pfx);
void behavior_adjust_streams(PfxBehavior* source, PfxBehavior* destination);
PfxKillInstruction* add_kill_insn(PfxBehavior* behavior, int opcode,
                                  unsigned int field);
void pfx_behaviors_frame_begin(PfxVm* pfx);
void pfx_behaviors_frame_end(PfxVm* pfx);
void behavior_fixup_targets(PfxBehavior* behavior,
                            PfxBehavior* const* old_targets,
                            PfxBehavior* const* new_targets, int target_count);
void pfx_behaviors_fixup_targets(PfxBehavior** behaviors,
                                 PfxBehavior** old_targets, int count);
PfxInitInstruction* add_init_insn(PfxBehavior* behavior, int opcode,
                                  unsigned int field);
void pfx_behavior_scan_fields(PfxBehavior* behavior,
                              unsigned int* particle_fields,
                              unsigned int* render_fields);
void pfxvm_update_make_last_insn_first(PfxBehavior* behavior);
void _pfxvm_execute_initial_behavior(PfxBehavior* behavior, float frame_time);
void pfxvm_initial_reflect(PfxBehavior* behavior, unsigned int field);
void pfxvm_initial_set_float_range(PfxBehavior* behavior, unsigned int field,
                                   PfxFloatRange* range);
void pfxvm_initial_multiply_float_range(PfxBehavior* behavior,
                                        unsigned int field,
                                        PfxFloatRange* range);
void pfxvm_initial_add_v3(PfxBehavior* behavior, unsigned int field,
                          unsigned int source_field);
void pfxvm_initial_divert(PfxBehavior* behavior, unsigned int field,
                          PfxFloatRange* range);

void move_particle_to_behavior(PfxBehavior* source, int particle,
                               PfxBehavior* destination);
void kill_behavior_particle(PfxBehavior* behavior, int particle);
void pfxvm_kill_on_y_less_than_field(PfxBehavior* behavior,
                                      unsigned int field,
                                      unsigned int reference_field);
void pfxvm_kill_on_intersect_plane_y(PfxBehavior* behavior, float plane);
void pfxvm_kill_on_intersect_plane_z(PfxBehavior* behavior, float plane);
void pfxvm_kill_on_intersect_plane_x(PfxBehavior* behavior, float plane);
void pfxvm_kill_on_greater(PfxBehavior* behavior, unsigned int field,
                           float value);
void pfxvm_kill_percent(PfxBehavior* behavior, float percent);
void pfxvm_kill_roundrobin(PfxBehavior* behavior, unsigned int field);
void pfxvm_change_on_greater(PfxBehavior* behavior, unsigned int field,
                             float value, PfxBehavior* target);
void pfxvm_change_on_less(PfxBehavior* behavior, unsigned int field,
                          float value, PfxBehavior* target);
void pfxvm_change_on_y_less(PfxBehavior* behavior, unsigned int field,
                            float value, PfxBehavior* target);
void pfxvm_change_on_y_less_than_field(PfxBehavior* behavior,
                                       unsigned int field,
                                       unsigned int reference_field,
                                       PfxBehavior* target);
void pfxvm_execute_behavior_kill(PfxBehavior* behavior);

#endif
