#ifndef LIBMKPARTICLE_TABLE_H
#define LIBMKPARTICLE_TABLE_H

/**
 * Partial particle table registry layout.
 *
 * Retail proves that the indexed table-pointer region begins at offset 0x20C.
 * The complete object size and original type/member names remain unknown.
 */
typedef struct PfxTableRegistry {
    char pad00[0x20C]; /**< Retail offsets 0x000-0x20B; fields unknown. */
    void* tables[1];   /**< Retail offset 0x20C; variable-length pointer region. */
} PfxTableRegistry;

void pfx_register_table(PfxTableRegistry* registry, int index, void* table);

#endif
