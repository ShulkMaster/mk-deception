#ifndef LIBMKPARTICLE_FIELDS_H
#define LIBMKPARTICLE_FIELDS_H

#include "libmkparticle/table.h"

int get_field_offset(PfxTableRegistry* registry, unsigned int description);
int has_field_description(PfxTableRegistry* registry,
                          unsigned int description);

#endif
