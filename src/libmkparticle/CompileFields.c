#include "libmkparticle/compile_fields.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/particle.h"

/* TODO: [near miss] 98.41096%; canonical instruction base adds one address
 * adjustment versus retail prefix-relative accesses; retain typed pointer storage. */
void _pfx_emitter_compile(PfxVmEmitter* emitter,
                          PfxTableRegistry* registry) {
    PfxEmitterInstruction* instruction;
    int i;

    if (emitter->instruction_count == 0) {
        return;
    }

    i = 0;
    while (i < emitter->instruction_count) {
        unsigned int storage;

        if (has_field_description(
                registry,
                (instruction = &emitter->instructions[i])->field_description) == 0) {
            return;
        }

        storage = instruction->field_description & 0xF00;
        switch (storage) {
        case 0x100:
            instruction->storage_type = 0;
            break;
        case 0x300:
            instruction->storage_type = 1;
            break;
        case 0x400:
            instruction->storage_type = 2;
            break;
        }

        instruction->field_offset =
            get_field_offset(registry, instruction->field_description);
        switch (instruction->opcode) {
        case 5:
        case 6:
        case 7: {
            PfxSpawnTable* table;

            table = (PfxSpawnTable*)
                registry->tables[instruction->spawn.table.table_index];
            if (table->type !=
                pfx_field_get_type(instruction->spawn.table.field)) {
                return;
            }
            instruction->spawn.table.table = table;
            break;
        }
        }
        i++;
    }
    emitter->flags.bits.emission_enabled = 1;
}
