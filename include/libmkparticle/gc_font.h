#ifndef LIBMKPARTICLE_GC_FONT_H
#define LIBMKPARTICLE_GC_FONT_H

#include "libmkparticle/pfxfont.h"

/*
 * GC native font geometry / GX display-list path (libmkparticle gc_font.o).
 * Called from pfxfont for PRESS START / legal / mode-select ScreenText labels.
 *
 * Menu TEXT chain:
 *   ScreenText::Render -> pfxfont_string_render -> nativefont_string_render
 *   -> nativefont_instance_addglyph / begin_render / end_render
 *
 * Soft ceiling: nativefont_instance_addglyph ~92.72% (branch/sth coloring;
 * per-vert WGPIPE reload + opt-level 2).
 */

/* Native GX code consumes the same retail 0x30 run and 0x90 string context. */
typedef PfxFontInstance NativeFontInstance;
typedef PfxFontString NativeFontString;

/* Glyph quad: x0,y0,x1,y1,u0,v0,u1,v1 */
typedef struct NativeFontQuad {
    float x0;
    float y0;
    float x1;
    float y1;
    float u0;
    float v0;
    float u1;
    float v1;
} NativeFontQuad;

int nativefont_system_init(void);
int nativefont_estimate_geometry_size(int glyph_count);
void nativefont_string_render(NativeFontString* ctx, float x, float y);
void nativefont_instance_lock(NativeFontInstance* inst);
void nativefont_instance_unlock(NativeFontInstance* inst);
void nativefont_instance_addglyph(NativeFontString* ctx, NativeFontInstance* inst,
                                  NativeFontQuad* quad);
void nativefont_end_render(void);
void nativefont_begin_render(void);
void nativefont_string_cleanup(NativeFontString* ctx);
void pfxfont_release_delayed_vertex_buffers(void);

#endif
