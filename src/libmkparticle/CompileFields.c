#include "libmkparticle/compile_fields.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/particle.h"

void _pfx_emitter_compile(PfxEmitterCompileView* emitter,
                          PfxTableRegistry* registry) {
    int offset;
    PfxCompileField* compile_field;
    int i;

    if (emitter->field_count == 0) {
        return;
    }

    i = 0;
    offset = 0;
    while (i < emitter->field_count) {
        unsigned int storage;

        if (has_field_description(
                registry,
                (compile_field = (PfxCompileField*)((char*)emitter + offset))->description) == 0) {
            return;
        }

        storage = compile_field->description & 0xF00;
        switch (storage) {
        case 0x100:
            compile_field->storage_type = 0;
            break;
        case 0x300:
            compile_field->storage_type = 1;
            break;
        case 0x400:
            compile_field->storage_type = 2;
            break;
        }

        compile_field->field_offset =
            get_field_offset(registry, compile_field->description);
        switch (compile_field->operation) {
        case 5:
        case 6:
        case 7: {
            PfxFieldTableHeader* table;

            table = (PfxFieldTableHeader*)
                registry->tables[compile_field->table_index];
            if (table->field_type !=
                pfx_field_get_type(compile_field->field)) {
                return;
            }
            compile_field->table_index = (int)table;
            break;
        }
        }
        i++;
        offset += 0x54;
    }
    emitter->flag1C_40 = 1;
}
