#include "libmkparticle/pfxfont.h"

#include "libmkparticle/gc_font.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/config.h"
#include "libmkparticle/rw_engine.h"

void* memset(void* dst, int c, unsigned long n);
void* memcpy(void* dst, const void* src, unsigned long n);
unsigned long strlen(const char* s);
int strncmp(const char* a, const char* b, unsigned long n);

/* EABI: dest in r3, floats in f1-f4. Dest-first helps addi-before-lfs at calls. */
void pfx_native_set_rgba(void* dest, float r, float g, float b, float a);

void RwRenderStateSet_rwRENDERSTATETEXTUREFILTER(int filter);
void RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(int enable);
void RwRenderStateSet_rwRENDERSTATEZTESTENABLE(int enable);
void RwRenderStateSet_rwRENDERSTATECULLMODE(int mode);
void RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(int enable);

#define COLOR_TAG "<COLOR=0x"

static const float s_zero = 0.0f;
static const float s_one = 1.0f;
/* Retail @360 = 255.0f -- rgba white in 0..255 float space (not 1.0). */
static const float s_255 = 255.0f;
static const float s_half = 0.5f;

static PfxFontAllocFn font_memory_alloc;
static PfxFontFreeFn font_memory_free;
static int cull_mode;

/* Retail emission order: system_init ... string_init, hex_char, find_*, string_set,
 * set_transform, string_render, get_render_state, begin/end, cleanup. */

void pfxfont_system_init(PfxFontAllocFn alloc_fn, PfxFontFreeFn free_fn) {
    font_memory_alloc = alloc_fn;
    font_memory_free = free_fn;
    nativefont_system_init();
}

void pfxfont_system_shutdown(void) {
    font_memory_alloc = 0;
    font_memory_free = 0;
}

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
int pfxfont_get_width(FontMetrics* metrics, const char* text) {
    float line;
    float widest;
    unsigned int ch;

    /* Retail: lfs line, fmr widest, then null checks (inside frame). */
    line = s_zero;
    widest = line;
    if (text == 0) {
        return 0;
    }
    if (metrics == 0) {
        return 0;
    }

    /* Retail falls through newline into the >=0x20 check (no else). */
    while ((ch = (unsigned char)*text) != 0) {
        if (ch == '\n') {
            if (line > widest) {
                widest = line;
            }
            line = s_zero;
        }
        if (ch >= 0x20 && (int)ch < 0x100) {
            /* advance then letter_spacing - retail lfsx then lfs 0x28. */
            line += metrics->glyphs[ch - 0x20].advance;
            line += metrics->letter_spacing;
        }
        text++;
    }
    if (line > widest) {
        widest = line;
    }
    return (int)widest;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
/* Retail keeps extsb + cmpwi at both signed-byte zero tests. */
#pragma peephole off
#pragma scheduling off
#pragma opt_common_subs off
#endif
int pfxfont_get_height(FontMetrics* metrics, const char* text) {
    float h;
    float max_glyph_h;
    float line_h;
    float spacing;
    unsigned char b;
    int ch;

    /* Combined || -> shared early-out (retail: cmplwi/cmplwi/lbz+extsb+cmpwi). */
    if (metrics == 0 || text == 0 || (signed char)*text == 0) {
        return 0;
    }

    /* Retail FPR: f3=line_h, f2=max, f0=spacing via lfs/fmr/lfs. */
    line_h = s_zero;
    max_glyph_h = line_h;
    spacing = s_one;

    /* Bottom-tested: raw byte in b so body/newline can re-extsb (retail r6). */
    for (; (b = *(unsigned char*)text, ch = (signed char)b, ch != 0); text++) {
        ch = (signed char)b; /* retail re-extsb at body entry before mulli */
        h = metrics->glyphs[ch - 0x20].box_h;
        if (max_glyph_h < h) {
            max_glyph_h = h;
        }
        if ((signed char)b == '\n') {
            line_h += max_glyph_h;
            line_h += spacing;
            max_glyph_h = s_zero;
        }
    }
    line_h += max_glyph_h;
    return (int)line_h;
}
#if !defined(TARGET_PC)
#pragma opt_common_subs reset
#pragma scheduling reset
#pragma peephole reset
#endif

void pfxfont_set_string_color(PfxFontString* dest, unsigned int* color) {
    PfxFontInstance* inst;

    inst = &dest->instance0;
    while (inst != 0) {
        if (inst->color_override == 0) {
            inst->color = *color;
        }
        inst = inst->next;
    }
}

#if !defined(TARGET_PC)
#pragma dont_inline on
#pragma scheduling off
#endif
void pfxfont_string_init(PfxFontString* ctx) {
    float one;

    /* Retail: stw lr / save r31 / mr, then li memset args (scheduling off). */
    memset(ctx, 0, 0x90);
    /*
     * Dest-first prototype so MWCC emits addi r3,+0x78 before lfs @360.
     * Same EABI regs as float-first (f1-f4 + r3); jdn.c uses this shape too.
     */
    pfx_native_set_rgba(&ctx->instance0.color, s_255, s_255, s_255, s_255);

    /* Align transform into pad at +0x10; identity diagonal via reloaded ptr. */
    ctx->transform = (PfxFontTransform*)(((unsigned long)ctx + 0x13) & ~0xFul);
    one = s_one;
    ctx->transform->rx = one;
    ctx->transform->uy = one;
    ctx->transform->az = one;
    ctx->transform->field_0x3C = one;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#pragma dont_inline reset
#endif

#if !defined(TARGET_PC)
#pragma dont_inline on
#pragma scheduling off
/* Retail keeps clrlwi + slwi + clrlwi instead of clrlslwi. */
#pragma peephole off
#endif
static unsigned char hex_char(const char* p) {
    unsigned char value;
    int i;
    int c;

    /* Retail: li value, li ctr=2, mtctr; shift via clrlwi/slwi/clrlwi. */
    value = 0;
    for (i = 0; i < 2; i++) {
        value = (unsigned char)((value & 0xff) << 4);
        c = (int)(signed char)*p;
        if (c >= '0' && c <= '9') {
            value = (unsigned char)((c + value) - '0');
        } else if (c >= 'a' && c <= 'f') {
            value = (unsigned char)((c + value) - 0x57);
        } else if (c >= 'A' && c <= 'F') {
            value = (unsigned char)((c + value) - 0x37);
        }
        p++;
    }
    return value;
}
#if !defined(TARGET_PC)
#pragma peephole reset
#pragma scheduling reset
#pragma dont_inline reset
#endif

/*
 * Scan from text; if a COLOR tag opens a new run, write RGBA into inst and
 * advance *pos. Returns drawable char count until next tag / NUL / cap 0x78.
 */
#if !defined(TARGET_PC)
#pragma dont_inline on
#pragma scheduling off
#pragma peephole off
#endif
static int find_drawable_boundary(const char* text, int* pos, PfxFontInstance* inst) {
    int count;
    int c;
    unsigned char* rgba;

    count = 0;
    while (*text != '\0') {
        c = (int)(signed char)*text;
        if (c == '<' && strncmp(text, COLOR_TAG, 9) == 0) {
            if (count != 0) {
                break;
            }
            *pos += 0x12;
            rgba = inst->rgba;
            rgba[0] = hex_char(text + 9);
            rgba[1] = hex_char(text + 0xB);
            rgba[2] = hex_char(text + 0xD);
            rgba[3] = hex_char(text + 0xF);
            text += 0x12;
            inst->color_override = 1;
            continue;
        }
        if (count >= 0x78) {
            break;
        }
        if (c != '\n') {
            count++;
        }
        text++;
    }
    return count;
}
#if !defined(TARGET_PC)
#pragma peephole reset
#pragma scheduling reset
#pragma dont_inline reset
#endif

#if !defined(TARGET_PC)
#pragma scheduling off
#pragma peephole off
#endif
void pfxfont_string_set(PfxFontString* ctx, PfxFontSlot* font, const char* text, float wrap_w,
                        int halign) {
    /* pen_x/pen_y before max_w -> retail f30/f29/f28 NV homes. */
    float pen_x;
    float pen_y;
    float max_w;
    float zero;
    float line_w;
    float line_w_at_space;
    NativeFontQuad quad;
    /* High NV first: cfg/remaining/cur -> r31/r30/r29; prev/char_i/run_len -> r28/r27/r26. */
    PfxConfig* cfg;
    int remaining;
    PfxFontInstance* cur;
    PfxFontInstance* prev;
    int char_i;
    int run_len;
    int line_end;
    int last_space;
    int pos;
    int est;
    int alloc_size;
    GlyphMetrics* g;
    void* raw;
    unsigned char ch;

    /*
     * Retail prologue: lfs max_w / li prev,char_i,run_len / fmr pen_x then pen_y.
     * Avoid a separate zero temp (extra fmr cascade).
     */
    max_w = s_zero;
    prev = 0;
    char_i = 0;
    run_len = 0;
    pen_x = max_w;
    pen_y = max_w;

    if (font->face == 0) {
        return;
    }

    remaining = (int)strlen(text);
    pfxfont_string_cleanup(ctx);
    pfxfont_string_init(ctx);
    ctx->face = font->face;
    cur = &ctx->instance0;

    if (remaining <= 0) {
        return;
    }

    cfg = &_pfx_config;
    /* Retail keeps a second zero in f31 for the align fcmpo pair. */
    zero = s_zero;

    while (remaining > 0) {
        pos = 0;
        line_end = 0;
        last_space = 0;
        line_w = s_zero;
        line_w_at_space = line_w;

        while (line_end < remaining) {
            ch = (unsigned char)text[line_end];
            if (ch == 0 || ch == '\n') {
                break;
            }
            if (ch == ' ') {
                line_w_at_space = line_w;
                line_w += font->metrics->space_width;
                last_space = line_end;
            } else if (ch >= 0x20) {
                if (ch == '<' && strncmp(text + line_end, COLOR_TAG, 9) == 0) {
                    line_end += 0x12;
                    continue;
                }
                /* Wrap measure uses glyph advance (not box_w). */
                line_w += font->metrics->glyphs[ch - 0x20].advance;
                line_w += font->metrics->letter_spacing;
            }

            /* Retail reloads @320 for these compares (not the align zero). */
            if (wrap_w > s_zero && line_w > wrap_w && last_space > 0) {
                line_end = last_space;
                line_w = line_w_at_space;
                break;
            }
            line_end++;
        }

        /*
         * Retail switch tree writes pen_x (f30) directly. When wrap is off /
         * line empty /halign out of range, pen_x is left at the EOL zero.
         */
        if (wrap_w > zero && line_w > zero) {
            switch (halign) {
            case 0:
                pen_x = zero;
                break;
            case 1: {
                float center_delta = wrap_w;
                center_delta -= line_w;
                pen_x = center_delta * s_half;
                break;
            }
            case 2:
                pen_x = wrap_w - line_w;
                break;
            }
        }

        while (pos < line_end) {
            if (cur->char_count == 0 || char_i >= run_len) {
                if (line_w > s_zero) {
                    if (prev != 0) {
                        nativefont_instance_unlock(prev);
                        cur = font_memory_alloc(0x30);
                        if (cur == 0) {
                            return;
                        }
                        memset(cur, 0, 0x30);
                        cur->color = prev->color;
                        cur->color_override = prev->color_override;
                        prev->next = cur;
                    }

                    run_len = find_drawable_boundary(text + pos, &pos, cur);
                    est = nativefont_estimate_geometry_size(run_len);
                    if (est != 0) {
                        alloc_size = est + cfg->align_add;
                        raw = font_memory_alloc(alloc_size);
                        cur->verts_raw = raw;
                        if (cur->verts_raw == 0) {
                            cur->verts = 0;
                            return;
                        }
                        /* Retail: store raw+add, reload, then andc with mask. */
                        cur->verts = (void*)((unsigned long)cur->verts_raw +
                                             (unsigned long)cfg->align_add);
                        cur->verts = (void*)((unsigned long)cur->verts &
                                             ~(unsigned long)cfg->align_mask);
                        cur->verts_bytes = alloc_size;
                    }
                    cur->char_count = run_len;
                    prev = cur;
                    char_i = 0;
                    nativefont_instance_lock(cur);
                }
            }

            if (pos < line_end) {
                ch = (unsigned char)text[pos];
                if (ch == ' ') {
                    if (pen_x > s_zero) {
                        pen_x += font->metrics->space_width;
                        pen_x += font->metrics->letter_spacing;
                    }
                } else if (ch >= 0x20) {
                    g = &font->metrics->glyphs[ch - 0x20];
                    /* Retail pack: x0,y0,x1,y1,u0,v0,u1,v1 */
                    quad.x0 = pen_x - g->off_x;
                    quad.y0 = pen_y - g->off_y;
                    quad.x1 = quad.x0 + g->box_w;
                    quad.y1 = quad.y0 + g->box_h;
                    quad.u0 = g->u0;
                    quad.v0 = g->v0;
                    quad.u1 = g->u1;
                    quad.v1 = g->v1;
                    nativefont_instance_addglyph(ctx, cur, &quad);
                    pen_x += g->advance;
                    pen_x += font->metrics->letter_spacing;
                    if (pen_x > max_w) {
                        max_w = pen_x;
                    }
                }

                char_i++;
                pos++;
            }
        }

        pos++;
        char_i++;
        text += pos;
        remaining -= pos;
        pen_x = s_zero;
        pen_y += font->metrics->line_height;
    }

    nativefont_instance_unlock(cur);
    ctx->width = (int)max_w;
    ctx->height = (int)pen_y;
}
#if !defined(TARGET_PC)
#pragma peephole reset
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
void pfxfont_set_transform(PfxFontString* ctx, const void* matrix44) {
    /* Retail: stw lr, then li r5,0x40, lwz dest, memcpy. */
    memcpy(ctx->transform, matrix44, 0x40);
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif


#if !defined(TARGET_PC)
#pragma scheduling off
#endif
void pfxfont_string_render(PfxFontString* ctx, float x, float y) {
    PfxSystemGlobals* g;
    int wx;
    int wy;

    /* Retail: i2f both widescreen offsets, then check instance0.char_count. */
    g = &pfxsystem_globals;
    wx = g->widescreen_x;
    x += (float)wx;
    wy = g->widescreen_y;
    y -= (float)wy;
    if (ctx->instance0.char_count != 0) {
        nativefont_string_render(ctx, x, y);
    }
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
#pragma dont_inline on
#endif
static void pfxfont_get_render_state(int state, void* out) {
    RwEngineInstance->fpRenderStateGet(state, out);
}
#if !defined(TARGET_PC)
#pragma dont_inline reset
#endif

void pfxfont_begin_render(void) {
    nativefont_begin_render();
    pfxfont_get_render_state(0x14, &cull_mode);
    RwRenderStateSet_rwRENDERSTATETEXTUREFILTER(1);
    RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(0);
    RwRenderStateSet_rwRENDERSTATEZTESTENABLE(0);
    RwRenderStateSet_rwRENDERSTATECULLMODE(1);
    RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(1);
}

#if !defined(TARGET_PC)
#pragma scheduling off
#endif
void pfxfont_end_render(void) {
    /* Retail: stw lr, then li r3,1 for ZWrite. */
    RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(1);
    RwRenderStateSet_rwRENDERSTATEZTESTENABLE(1);
    RwRenderStateSet_rwRENDERSTATECULLMODE(cull_mode);
    nativefont_end_render();
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#endif

#if !defined(TARGET_PC)
#pragma dont_inline on
#pragma scheduling off
#endif
void pfxfont_string_cleanup(PfxFontString* ctx) {
    /* High NV first: first/inst/next -> r31/r29/r28 (retail). */
    PfxFontInstance* first;
    PfxFontInstance* inst;
    PfxFontInstance* next;

    /* Retail: addi inst, li null, mr first=inst. */
    inst = &ctx->instance0;
    first = inst;
    while (inst != 0) {
        nativefont_string_cleanup(ctx);
        if (inst->verts_raw != 0) {
            font_memory_free(inst->verts_raw);
            inst->verts_raw = 0;
            inst->verts = 0;
        }
        next = inst->next;
        if (inst != first) {
            font_memory_free(inst);
        }
        inst = next;
    }
    ctx->instance0.next = 0;
}
#if !defined(TARGET_PC)
#pragma scheduling reset
#pragma dont_inline reset
#endif
