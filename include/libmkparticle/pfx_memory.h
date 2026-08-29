#ifndef LIBMKPARTICLE_PFX_MEMORY_H
#define LIBMKPARTICLE_PFX_MEMORY_H

typedef struct PfxVm PfxVm;

typedef struct PfxBuildInfo {
    int behavior_count;             /* +0x00 */
    int metrics_frame_count;        /* +0x04 */
    int emitter_count;              /* +0x08 */
    unsigned int flags;             /* +0x0C */
    int emitter_user_data_size;     /* +0x10 */
    int particle_user_data_size;    /* +0x14 */
    char* name;
} PfxBuildInfo;

typedef struct PfxEstimate {
    int particle_memory_size;       /* +0x00 */
    int render_memory_size;         /* +0x04 */
    int shader_memory_size;         /* +0x08 */
    int parametric_memory_size;     /* +0x0C */
    int emitter_user_data_size;     /* +0x10 */
    int name_size;                  /* +0x14 */
    int runtime_buffers_size;       /* +0x18 */
    int metrics_frame_count;        /* +0x1C */
    int metrics_memory_size;        /* +0x20 */
    int field_count;                /* +0x24 */
    int field_descriptions_size;    /* +0x28 */
    int behavior_count;             /* +0x2C */
    int behavior_memory_size;       /* +0x30 */
    int emitter_count;              /* +0x34 */
    int emitter_memory_size;        /* +0x38 */
    int emitter_user_data_block_size; /* +0x3C */
    int size;                       /* +0x40 -- total allocation size */
    int particle_property_count;    /* +0x44 */
    int particle_stride;            /* +0x48 */
    int particle_user_data_size;    /* +0x4C */
} PfxEstimate;

/* Allocation-facing view of PfxVm +0x50. */
typedef struct PfxParticleMemory {
    int capacity;
    int cursor;
    int active_transform;
    int field_0x0C;
    unsigned int flags;
    int user_data_size;
    void* data;
    int stride;
} PfxParticleMemory;

typedef char PfxBuildInfoSizeCheck[(sizeof(PfxBuildInfo) == 0x1C) ? 1 : -1];
typedef char PfxEstimateSizeCheck[(sizeof(PfxEstimate) == 0x50) ? 1 : -1];
typedef char PfxParticleMemorySizeCheck[
    (sizeof(PfxParticleMemory) == 0x20) ? 1 : -1];

void pfx_estimate_size(PfxVm* pfx, PfxEstimate* estimate,
                       PfxBuildInfo* build);
void pfx_set_memory(PfxVm* pfx, void* memory, PfxEstimate* estimate);
int pfx_estimate_render_size(PfxVm* vm);
int pfx_particle_estimate_size(unsigned int flags, PfxEstimate* estimate);
void pfx_particle_set_memory(PfxParticleMemory* particle,
                             PfxEstimate* estimate, void* memory);
void pfx_copy_behavior_list(void* vm, int count, const void* behaviors);
void* pfx_effect_memory_alloc(PfxVm* vm, int size, int align);

#endif
