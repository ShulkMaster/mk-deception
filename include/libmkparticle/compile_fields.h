#ifndef LIBMKPARTICLE_COMPILE_FIELDS_H
#define LIBMKPARTICLE_COMPILE_FIELDS_H

#include "libmkparticle/table.h"

typedef struct PfxCompileField {
    char pad00[0x44];
    int operation;
    unsigned int description;
    int storage_type;
    int field_offset;
    char pad54[0x24];
    int table_index;
    unsigned int field;
} PfxCompileField;

typedef struct PfxEmitterCompileView {
    char pad00[0x1C];
    union {
        unsigned char flags1C;
        struct {
            unsigned char flag1C_80 : 1;
            unsigned char flag1C_40 : 1;
            unsigned char flags1C_low : 6;
        };
    };
    char pad1D[0x2C7];
    int field_count;
} PfxEmitterCompileView;

typedef struct PfxFieldTableHeader { int field_type; } PfxFieldTableHeader;

void _pfx_emitter_compile(PfxEmitterCompileView* emitter, PfxTableRegistry* registry);
#endif
