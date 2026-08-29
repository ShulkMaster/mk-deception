#include "libmkparticle/pfx_memory.h"
#include "libmkparticle/behavior.h"
#include "libmkparticle/config.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/metrics.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/shader.h"
#include "libmkparticle/vm.h"
#include "runtime/mk_mem.h"
#include "runtime/cstring.h"

int pfx_estimate_render_size(PfxVm* vm)
{
    int size;
    int index;
    int field_offset;
    int render_field_count;
    PfxConfig* config;
    PfxFieldDefinition* field_table;
    PfxFieldDefinition* field;

    size = 0;
    index = 0;
    field_offset = 0;
    config = &_pfx_config;
    field_table = render_fields;
    render_field_count = _num_render_fields;
    while (index < render_field_count) {
        field = (PfxFieldDefinition*)((unsigned char*)field_table +
                                      field_offset);
        if ((vm->flags_0x1D4 & field->flag) != 0) {
            if (field->flag == 0x100) {
                size += 0x10;
            } else {
                size += get_size(field->type);
            }
            size = (size + config->align_add) & ~config->align_mask;
        }
        index++;
        field_offset += sizeof(PfxFieldDefinition);
    }
    return size;
}

int pfx_particle_estimate_size(unsigned int flags, PfxEstimate* estimate)
{
    int count;
    int size;
    PfxFieldDefinition* field;

    count = 0;
    size = 0;
    for (field = properties; field->flag != 0; field++) {
        if ((flags & field->flag) != 0) {
            size += get_size(field->type);
            count++;
        }
    }
    estimate->particle_property_count = count;
    size = (size + _pfx_config.align_add) & ~_pfx_config.align_mask;
    estimate->particle_stride = size;
    return size;
}

void pfx_particle_set_memory(PfxParticleMemory* particle,
                             PfxEstimate* estimate, void* memory)
{
    if (estimate->particle_user_data_size != 0) {
        particle->data = memory;
    } else {
        particle->data = 0;
    }
    particle->stride = estimate->particle_stride;
    particle->user_data_size = estimate->particle_user_data_size;
}

void pfx_estimate_size(PfxVm* pfx, PfxEstimate* estimate,
                       PfxBuildInfo* build)
{
    PfxFieldSet fields;

    if (pfx->particle_capacity == 0) {
        return;
    }

    fields.render_flags = pfx->flags_0x1D4;
    fields.particle_flags = pfx->flags_0x60;
    memset(estimate, 0, sizeof(PfxEstimate));
    estimate->particle_user_data_size = build->particle_user_data_size;

    if (pfx->field_0x22C == 0) {
        estimate->particle_memory_size =
            pfx_particle_estimate_size(pfx->flags_0x60, estimate);
    }
    estimate->render_memory_size = pfx_estimate_render_size(pfx);
    if (pfx->field_0x22C != 0) {
        estimate->shader_memory_size = 0;
    } else {
        estimate->shader_memory_size =
            pfx_shader_estimate_size(pfx->flags_0x1D4);
    }
    if (pfx->field_0x22C != 0) {
        estimate->parametric_memory_size = pfx->particle_capacity * 0x28 + 0x358;
    } else {
        estimate->parametric_memory_size = 0;
    }

    estimate->field_count = get_field_count(&fields);
    if (pfx->field_0x22C != 0) {
        estimate->field_count += 4;
    }
    if (estimate->field_count != 0) {
        estimate->field_descriptions_size =
            (estimate->field_count + 1) * 0xC;
    }

    if ((build->flags & 0x80000000U) != 0) {
        estimate->emitter_user_data_size = 0x40;
    }
    if (build->emitter_user_data_size != 0) {
        if ((build->emitter_user_data_size & 3) != 0) {
            build->emitter_user_data_size =
                (build->emitter_user_data_size & ~3) + 4;
        }
        estimate->emitter_user_data_block_size =
            build->emitter_user_data_size + 0xC;
    }

    estimate->metrics_memory_size =
        pfxmetrics_estimate_size(build->metrics_frame_count);
    estimate->metrics_frame_count = build->metrics_frame_count;
    estimate->runtime_buffers_size = 0x40;
    if (build->name != 0) {
        estimate->name_size = strlen(build->name) + 1;
    }
    if (build->behavior_count != 0) {
        estimate->behavior_memory_size = build->behavior_count * 0x38C;
        estimate->behavior_count = build->behavior_count;
    }
    if (build->emitter_count != 0) {
        estimate->emitter_memory_size = build->emitter_count * 0x2EC;
        estimate->emitter_count = build->emitter_count;
    }

    estimate->size = estimate->parametric_memory_size +
                     estimate->emitter_user_data_size +
                     estimate->name_size +
                     estimate->behavior_memory_size +
                     estimate->field_descriptions_size +
                     estimate->emitter_memory_size +
                     estimate->metrics_memory_size +
                     estimate->emitter_user_data_block_size +
                     estimate->particle_user_data_size * pfx->particle_capacity +
                     estimate->runtime_buffers_size + 0x10;
}

typedef struct PfxParametricMemory {
    char pad00[0x348];
    int particle_capacity;
    char pad34C[8];
    float field_0x354;
} PfxParametricMemory;

typedef char PfxParametricMemorySizeCheck[
    (sizeof(PfxParametricMemory) == 0x358) ? 1 : -1];

void pfx_set_memory(PfxVm* pfx, void* memory, PfxEstimate* estimate)
{
    unsigned char* cursor;
    PfxParametricMemory* parametric;
    PfxFieldSet fields;
    int index;

    cursor = (unsigned char*)memory;
    memset(cursor, 0, estimate->size);
    if (pfx->field_0x22C == 0) {
        pfx_particle_set_memory((PfxParticleMemory*)&pfx->particle_capacity,
                                estimate, cursor);
        cursor += estimate->particle_user_data_size * pfx->particle_capacity;
    }

    if (((unsigned int)cursor & 0xF) != 0) {
        cursor = (unsigned char*)(((unsigned int)cursor + 0x10) & ~0xFU);
    }
    for (index = 0; index < 3; index++) {
        pfx->transforms[index].particle_field_stride =
            estimate->render_memory_size;
    }

    if (pfx->field_0x22C != 0) {
        pfx->name_obj = cursor;
        parametric = (PfxParametricMemory*)pfx->name_obj;
        parametric->particle_capacity = pfx->particle_capacity;
        parametric->field_0x354 = -10000.0f;
        cursor += estimate->parametric_memory_size;
    } else {
        pfx->name_obj = 0;
    }

    if (estimate->emitter_memory_size != 0) {
        pfx->emitter_count = estimate->emitter_count;
        pfx->emitters = (PfxVmEmitter*)cursor;
        cursor += estimate->emitter_memory_size;
    } else {
        pfx->emitter_count = 0;
        pfx->emitters = 0;
    }

    if (estimate->emitter_user_data_size != 0) {
        ((PfxVmEmitter*)pfx_get_emitter((PfxEmitterTableView*)pfx, 0))->user_data =
            cursor;
    }
    cursor += estimate->emitter_user_data_size;

    if (estimate->emitter_user_data_block_size != 0) {
        unsigned char* aligned;

        aligned = cursor;
        if (((unsigned int)cursor & 0xF) != 0) {
            aligned += 0x10 - ((unsigned int)cursor & 0xF);
        }
        pfx->emitter_user_data = aligned;
        cursor += estimate->emitter_user_data_block_size;
    }

    if (estimate->behavior_memory_size != 0) {
        pfx->behavior_count = estimate->behavior_count;
        pfx->behaviors = (void**)cursor;
        cursor += estimate->behavior_count * sizeof(void*);
        for (index = 0; index < estimate->behavior_count; index++) {
            pfx->behaviors[index] = cursor;
            cursor += 0x388;
        }
    }

    if (estimate->field_descriptions_size != 0) {
        pfx->field_count = estimate->field_count;
        pfx->field_descriptions = cursor;
        cursor += (estimate->field_count + 1) * 0xC;
        fields.render_flags = pfx->flags_0x1D4;
        fields.particle_flags = pfx->flags_0x60;
        fill_field_description(pfx->field_descriptions, &fields,
                               pfx->field_0x22C);
    }

    pfx->metrics = pfxmetrics_set_mem((PfxMetrics*)cursor,
                                      estimate->metrics_frame_count);
    cursor += estimate->metrics_memory_size;
    if (estimate->runtime_buffers_size != 0) {
        memset(cursor, 0, 0x20);
        pfx->runtime_buffer_a = cursor;
        pfx->runtime_buffer_b = cursor + 0x20;
        cursor += 0x40;
    } else {
        pfx->runtime_buffer_a = 0;
        pfx->runtime_buffer_b = 0;
    }

    if (estimate->name_size != 0) {
        pfx->name = (char*)cursor;
    } else {
        pfx->name = 0;
    }
}

void pfx_copy_behavior_list(void* vm, int count, const void* behaviors)
{
    PfxVm* pfx;
    const PfxBehavior* source;
    int index;

    pfx = (PfxVm*)vm;
    source = (const PfxBehavior*)behaviors;
    if (count == pfx->behavior_count) {
        for (index = 0; index < count; index++) {
            memcpy(pfx_behavior(pfx, index)->segment_0xDC,
                   source[index].segment_0xDC, 0x244);
            memcpy(pfx_behavior(pfx, index)->segment_0x58,
                   source[index].segment_0x58, 0x84);
            memcpy(pfx_behavior(pfx, index)->segment_0x320,
                   source[index].segment_0x320, 0x64);
        }
        if (count != 0) {
            pfx_behavior(pfx, 0)->link_0x384 = source[0].link_0x384;
        }
    }
}

/*
 * Soft ceiling: 93.67647%. A 1,102-iteration authentic-compiler permuter run
 * reached zero only by adding a fake increment/decrement lifetime around the
 * alignment mask; that match-forcing candidate is intentionally rejected.
 */
void* pfx_effect_memory_alloc(PfxVm* vm, int size, int align)
{
    unsigned char* allocation;
    unsigned int align_mask;
    unsigned char* aligned;

    if (align < 4) {
        align = 4;
    }

    allocation = (unsigned char*)get_mem(size + align);
    if (allocation != 0) {
        align_mask = (unsigned int)-align;
        aligned = allocation + align;
        aligned = (unsigned char*)
            (align_mask & (unsigned int)(aligned + 3));
        *(void**)allocation = vm->effect_allocations;
        vm->effect_allocations = allocation;
        if (aligned != allocation + sizeof(void*)) {
            *(void**)(allocation + sizeof(void*)) = aligned;
        }
    } else {
        aligned = 0;
    }
    return aligned;
}
