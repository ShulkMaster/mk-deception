#ifndef LIBMKPARTICLE_FIELDS_H
#define LIBMKPARTICLE_FIELDS_H

#include "libmkparticle/table.h"

typedef struct PfxFieldDefinition {
    unsigned int description;
    unsigned int flag;
    int type;
} PfxFieldDefinition;

typedef struct PfxFieldSet {
    unsigned int render_flags;
    unsigned int particle_flags;
} PfxFieldSet;

extern PfxFieldDefinition properties[];
extern PfxFieldDefinition render_fields[];
extern int _num_render_fields;

int get_size(int type);
int get_field_count(PfxFieldSet* fields);
void fill_field_description(void* descriptions, PfxFieldSet* fields,
                            int parametric);

int get_field_offset(PfxTableRegistry* registry, unsigned int description);
int has_field_description(PfxTableRegistry* registry,
                          unsigned int description);

#endif
