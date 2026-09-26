#include "libmkparticle/compile_fields.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/particle.h"

void _pfx_emitter_compile(PfxVmEmitter* emitter,
                          PfxTableRegistry* registry) {
    int i;

    if (emitter->instruction_count == 0) {
        return;
    }

    for (i = 0; i < emitter->instruction_count; i++) {
        if (has_field_description(
                registry, emitter->instructions[i].field_description) == 0) {
            return;
        }

        switch (emitter->instructions[i].field_description & 0xF00) {
        case 0x100:
            emitter->instructions[i].storage_type = 0;
            break;
        case 0x300:
            emitter->instructions[i].storage_type = 1;
            break;
        case 0x400:
            emitter->instructions[i].storage_type = 2;
            break;
        }

        emitter->instructions[i].field_offset = get_field_offset(
            registry, emitter->instructions[i].field_description);
        switch (emitter->instructions[i].opcode) {
        case 5:
        case 6:
        case 7: {
            PfxSpawnTable* table;

            table = (PfxSpawnTable*)registry->tables
                [emitter->instructions[i].spawn.table.table_index];
            if (table->type !=
                pfx_field_get_type(emitter->instructions[i].spawn.table.field)) {
                return;
            }
            emitter->instructions[i].spawn.table.table = table;
            break;
        }
        }
    }
    emitter->flags.bits.emission_enabled = 1;
}
