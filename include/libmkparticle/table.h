#ifndef LIBMKPARTICLE_TABLE_H
#define LIBMKPARTICLE_TABLE_H

typedef struct PfxFieldDescription PfxFieldDescription;

/** Partial particle table/field registry layout recovered from retail users. */
typedef struct PfxTableRegistry {
    char pad00[0x1D8]; /**< Retail offsets 0x000-0x1D7; fields unknown. */
    int field_count;                         /**< Retail offset 0x1D8. */
    PfxFieldDescription* field_descriptions; /**< Retail offset 0x1DC. */
    char pad1E0[0x1C]; /**< Retail offsets 0x1E0-0x1FB; fields unknown. */
    unsigned char field_0x1FC[0x10];
    void* tables[1];   /**< Retail offset 0x20C; variable-length pointer region. */
} PfxTableRegistry;

void pfx_register_table(PfxTableRegistry* registry, int index, void* table);

#endif
