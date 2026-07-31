#ifndef LIBMKPARTICLE_COLOR_H
#define LIBMKPARTICLE_COLOR_H

/**
 * Native particle color stored as four unsigned channels.
 *
 * Retail layout: 0x04 bytes. The type/member names are descriptive; retail
 * code proves the byte offsets and unsigned loads.
 */
typedef struct PfxColor {
    unsigned char r; /**< Retail offset 0x00: red channel. */
    unsigned char g; /**< Retail offset 0x01: green channel. */
    unsigned char b; /**< Retail offset 0x02: blue channel. */
    unsigned char a; /**< Retail offset 0x03: alpha channel. */
} PfxColor;

void pfx_native_set_rgba(PfxColor* color, float r, float g, float b, float a);
void pfx_native_get_rgba(const PfxColor* color, float* r, float* g, float* b,
                         float* a);

#endif
