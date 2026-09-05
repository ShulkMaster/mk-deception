#ifndef LIBMKPARTICLE_FIELDS_H
#define LIBMKPARTICLE_FIELDS_H

#include "libmkparticle/table.h"

typedef struct PfxVm PfxVm;

typedef struct PfxFieldDefinition {
    int description;
    int flag;
    int type;
} PfxFieldDefinition;

typedef struct PfxFieldSet {
    unsigned int render_flags;
    unsigned int particle_flags;
} PfxFieldSet;

typedef struct PfxFieldBuffer {
    unsigned char* data;
    int stride;
} PfxFieldBuffer;

typedef struct PfxFieldDescription {
    int description;
    int stream;
    int offset;
} PfxFieldDescription;

extern PfxFieldDefinition properties[];
extern PfxFieldDefinition render_fields[];
extern PfxFieldDefinition parametric_fields[];
extern const int _num_render_fields;

int get_size(int type);
unsigned int map_field_to_propertyflag(int description);
unsigned int map_field_to_renderflag(int description);
void add_field(unsigned int* fields, unsigned int description);
int map_field_to_stream(int description);
int get_field_count(PfxFieldSet* fields);
void field_copy(PfxFieldBuffer* destination, PfxFieldBuffer* source,
                unsigned int field_size, int count);
void fill_field_description(PfxFieldDescription* descriptions,
                            PfxFieldSet* fields, int parametric);

int get_field_offset(PfxTableRegistry* registry, int description);
int has_field_description(PfxTableRegistry* registry, int description);
void* pfx_get_field(PfxVm* pfx, int particle, unsigned int description);

#endif
