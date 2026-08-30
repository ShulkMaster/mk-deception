#ifndef LIBMKPARTICLE_CONFIG_H
#define LIBMKPARTICLE_CONFIG_H

typedef int (*PfxShaderEstimateSizeFn)(unsigned int render_flags);
typedef void (*PfxHaltFn)(const char* message);

/**
 * Particle-system configuration block (`_pfx_config`).
 *
 * Retail layout: 0x20 bytes. Field names are inferred; offsets are recovered
 * from retail code and are not sourced from DWARF.
 */
typedef struct PfxConfig {
    int normalized_texture_coords; /**< Retail offset 0x00. */
    int align_add;                 /**< Retail offset 0x04. */
    int align_mask;                /**< Retail offset 0x08. */
    PfxShaderEstimateSizeFn estimate_size; /**< Retail offset 0x0C. */
    char pad10[4]; /**< Retail offsets 0x10-0x13; purpose unknown. */
    PfxHaltFn halt; /**< Retail offset 0x14; optional fatal-error callback. */
    char pad18[8]; /**< Retail offsets 0x18-0x1F; purpose unknown. */
} PfxConfig;

extern PfxConfig _pfx_config;

#endif
