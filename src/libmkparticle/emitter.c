#include "libmkparticle/random.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/metrics.h"
#include "libmkparticle/spawn.h"
#include "libmkparticle/particle.h"

int pfx_emitter_exhausted(PfxVmEmitter* emitter)
{
    int final_cycle;

    final_cycle = 1;
    if (0.0f != emitter->cycle_length &&
        emitter->cycle_limit == 0) {
        final_cycle = 0;
    }
    if (emitter->cycle_limit != 0 &&
        emitter->cycle_index < emitter->cycle_limit - 1) {
        final_cycle = 0;
    }

    if (emitter->lifetime >= 1.0f && emitter->age > emitter->lifetime) {
        return 1;
    }

    if (final_cycle != 0) {
        if (emitter->birth_limit != 0 &&
            emitter->birth_count >= emitter->birth_limit) {
            return 1;
        }
        if (emitter->cycle_length &&
            emitter->cycle_position >= emitter->current_cycle_length) {
            return 1;
        }
    }
    return 0;
}

int pfx_emitter_unused(PfxVmEmitter* emitter)
{
    if (emitter->cycle_position) {
        return 0;
    }
    if (emitter->cycle_index != 0) {
        return 0;
    }
    if (emitter->flags.bits.cycle_paused == 0) {
        return 0;
    }
    return emitter->birth_count == 0;
}

int pfx_emitter_restart_cycle(PfxVmEmitter* emitter)
{
    int cycle_index;

    cycle_index = emitter->cycle_index + 1;
    if (emitter->cycle_limit != 0 && cycle_index >= emitter->cycle_limit) {
        return 0;
    }
    pfx_emitter_reset(emitter);
    emitter->cycle_index = cycle_index;
    emitter->flags.bits.cycle_paused = 0;
    return 1;
}

void pfx_emitter_reset(PfxVmEmitter* emitter)
{
    float variation;
    float cycle_length;

    emitter->cycle_index = 0;
    emitter->cycle_position = 0.0f;
    emitter->birth_count = 0;
    emitter->partial_birth = 0.0f;
    variation = emitter->cycle_length_variation;
    cycle_length = emitter->cycle_length;
    emitter->current_cycle_length =
        rnd_between(cycle_length - variation, cycle_length + variation);
    emitter->flags.bits.cycle_paused = 1;
}

int _pfx_emitter_get_birthcount(PfxVmEmitter* emitter, float frame_time,
                                PfxVm* pfx)
{
    float cycle_time;
    int birth_count;
    int available;

    if (emitter->flags.bits.cycle_paused != 0) {
        return 0;
    }
    if (pfx_emitter_exhausted(emitter) != 0) {
        return 0;
    }

    if (emitter->cycle_length) {
        if (emitter->cycle_position >= emitter->current_cycle_length) {
            emitter->cycle_position = emitter->current_cycle_length;
            if (pfx_emitter_restart_cycle(emitter) == 0) {
                return 0;
            }
        }

        cycle_time =
            emitter->current_cycle_length - emitter->cycle_position;
        if (cycle_time > frame_time) {
            cycle_time = frame_time;
            emitter->cycle_position += frame_time;
        } else {
            emitter->cycle_position = emitter->current_cycle_length;
        }
    } else {
        cycle_time = frame_time;
    }

    emitter->age += cycle_time;
    if (emitter->lifetime >= 1.0f && emitter->age > emitter->lifetime) {
        return 0;
    }

    if (emitter->flags.bits.constant_rate != 0) {
        birth_count = emitter->birth_rate;
    } else {
        emitter->partial_birth += emitter->birth_rate * cycle_time;
        birth_count = emitter->partial_birth;
        emitter->partial_birth -= birth_count;
        if (emitter->partial_birth > 0.00001f) {
            birth_count++;
            emitter->partial_birth -= 1.0f;
        }
    }

    available = pfx->particle_capacity - pfx->particle_cursor;
    if (birth_count > available) {
        birth_count = available;
    }
    if (emitter->birth_limit != 0) {
        available = emitter->birth_limit - emitter->birth_count;
        if (birth_count > available) {
            birth_count = available;
        }
    }
    return birth_count;
}

void pfx_emitter_run_frame(PfxVm* pfx, int emitter_index, float frame_time)
{
    int spawn_index;
    int first_particle;
    PfxFieldBuffer destination;
    int transfer_count;
    PfxVmEmitter* emitter;
    int birth_count;
    int available;

    pfxmetrics_event(pfx->metrics, 0x1001);
    if (pfx->emitter_transfers != 0) {
        destination.data = pfx_get_field(pfx, -2, 0x100);
        destination.stride = pfx_get_struct_size(pfx, 0x100);
    }

    available = pfx->particle_capacity - pfx->particle_cursor;
    emitter = pfx_get_emitter(pfx, emitter_index);
    if (emitter->flags.bits.emission_enabled != 0) {
        PfxEmitterTransfer* transfer;

        birth_count =
            _pfx_emitter_get_birthcount(emitter, frame_time, pfx);
        for (transfer = pfx->emitter_transfers; transfer != 0;
             transfer = transfer->next) {
            birth_count += transfer->particle_count;
        }
        if (birth_count > available) {
            birth_count = available;
        }

        first_particle = pfx->particle_cursor;
        for (spawn_index = 0; spawn_index < birth_count; spawn_index++) {
            _pfxvm_execute_spawn(pfx, emitter_index);
        }

        for (transfer = pfx->emitter_transfers; transfer != 0;
             transfer = transfer->next) {
            transfer_count = transfer->particle_count;
            if (transfer_count != 0) {
                PfxFieldBuffer source;

                if (transfer_count > available) {
                    transfer_count = available;
                }
                source.data = transfer->source;
                source.stride = transfer->source_stride;
                field_copy(&destination, &source, sizeof(PfxVec3),
                           transfer_count);
                destination.data += destination.stride * transfer_count;
                available -= transfer_count;
            }
        }
        emitter->birth_count += birth_count;
    } else {
        birth_count = 0;
        first_particle = 0;
    }

    if (pfx->spawn_callback != 0) {
        pfx->spawn_callback(pfx, first_particle,
                            pfx->particle_cursor - first_particle);
    }
    pfx->emitter_transfers = 0;
    pfx->total_birth_count += birth_count;
    pfxmetrics_event(pfx->metrics, 0x2001);
}

void pfx_emitter_scan_for_fields(PfxVmEmitter* emitter,
                                 unsigned int* fields)
{
    int index;

    for (index = 0; index < emitter->instruction_count; index++) {
        add_field(fields, emitter->instructions[index].field_description);
    }
}

PfxEmitterInstruction* pfx_emitter_find_insn(PfxVmEmitter* emitter,
                                             int description)
{
    PfxEmitterInstruction* instruction;
    int index;

    if (emitter == 0) {
        return 0;
    }
    instruction = emitter->instructions;
    for (index = 0; index < emitter->instruction_count;
         index++, instruction++) {
        if (instruction->field_description == description) {
            return instruction;
        }
    }
    return 0;
}

int pfx_emitter_insn_is_static(PfxEmitterInstruction* instruction)
{
    switch (instruction->opcode) {
    case 0:
        return 1;
    case 8:
        return instruction->value.integer.start ==
               instruction->value.integer.end;
    case 9:
        return instruction->value.scalar.start ==
               instruction->value.scalar.end;
    default:
        return 0;
    }
}
