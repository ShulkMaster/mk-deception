#include "libmkparticle/table.h"

void pfx_register_table(PfxTableRegistry* registry, int index, void* table) {
    registry->tables[index] = table;
}
