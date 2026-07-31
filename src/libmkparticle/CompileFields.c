#include "libmkparticle/compile_fields.h"

int has_field_description(PfxTableRegistry* registry, unsigned int description);
int get_field_offset(PfxTableRegistry* registry, unsigned int description);
int pfx_field_get_type(unsigned int field);

/* Soft ceiling: _pfx_emitter_compile ~95.68% -- equivalent range-branch
 * and call scheduling differences. */
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

        compile_field = (PfxCompileField*)((char*)emitter + offset);
        if (has_field_description(registry, compile_field->description) == 0) {
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
        if (compile_field->operation < 8) {
            if (compile_field->operation >= 5) {
                PfxFieldTableHeader* table;

                table = (PfxFieldTableHeader*)
                    registry->tables[compile_field->table_index];
                if (table->field_type !=
                    pfx_field_get_type(compile_field->field)) {
                    return;
                }
                compile_field->table_index = (int)table;
            }
        }
        i++;
        offset += 0x54;
    }
    emitter->flag1C_40 = 1;
}
