#ifndef LIBMKPARTICLE_PFXFONT_H
#define LIBMKPARTICLE_PFXFONT_H

#include "libmkparticle/color.h"

/*
 * Midway pfxfont (libmkparticle pfxfont.o) - PRESS START / legal string path.
 * Called from fonts.c and disc_error.c.
 */

typedef void* (*PfxFontAllocFn)(unsigned int size);
typedef void (*PfxFontFreeFn)(void* ptr);

/* Defined in runtime/fonts.h (TGA face object); incomplete here for PfxFontSlot. */
typedef struct FontFace FontFace;
typedef struct RwTexture RwTexture;

/* Per-glyph metrics; table at FontMetrics+0x34, stride 0x24, index (ch-0x20). */
typedef struct GlyphMetrics {
    float box_w;   /* +0x00 */
    float box_h;   /* +0x04 */
    float off_x;   /* +0x08 */
    float off_y;   /* +0x0C */
    float u0;      /* +0x10 */
    float u1;      /* +0x14 */
    float v0;      /* +0x18 */
    float v1;      /* +0x1C */
    float advance; /* +0x20 */
} GlyphMetrics;

/* Glyph metrics header in the binary block loaded beside the TGA. */
typedef struct FontMetrics {
    char pad00[0x24];
    float cell_height;    /* +0x24 - fonts valign / get_font_height */
    float letter_spacing; /* +0x28 */
    float line_height;    /* +0x2C */
    float space_width;    /* +0x30 */
    GlyphMetrics glyphs[1]; /* +0x34 - variable; index via (ch - 0x20) */
} FontMetrics;

/* View of FontTableEntry.face + .metrics (return of load_font). */
typedef struct PfxFontSlot {
    FontFace* face;       /* +0x00 TGA / FontFace* */
    FontMetrics* metrics; /* +0x04 glyph metrics binary */
} PfxFontSlot;

/* 4x4/RwMatrix-style transform consumed by the native font renderer. */
typedef struct PfxFontTransform {
    float rx, ry, rz, field_0x0C;
    float ux, uy, uz, field_0x1C;
    float ax, ay, az, field_0x2C;
    float tx, ty, tz, field_0x3C;
} PfxFontTransform;

/* One drawable run / color span (0x30). Linked from PfxFontString+0x60. */
typedef struct PfxFontInstance {
    void* dl;                     /* +0x00 aligned native display list */
    unsigned int dl_size;         /* +0x04 native display-list byte count */
    unsigned int size;            /* +0x08 native display-list capacity */
    unsigned int pad0c;           /* +0x0C */
    int locked;                   /* +0x10 native display-list build state */
    int color_override;           /* +0x14 */
    /* +0x18 -- word or per-channel RGBA (pfxfont tags / ScreenText). */
    union {
        unsigned int color;
        PfxColor native_color;
        unsigned char rgba[4];
    };
    int char_count;               /* +0x1C */
    void* verts;                  /* +0x20 aligned */
    int verts_bytes;              /* +0x24 */
    void* verts_raw;              /* +0x28 alloc base */
    struct PfxFontInstance* next; /* +0x2C */
} PfxFontInstance;

/*
 * String draw context (0x90). Embedded as StringObj.pfx at +0x3C.
 * Init writes float rgba[4] at +0x78 (overlays instance0.color..verts_bytes).
 * Cleanup clears +0x8C (instance0.next).
 * Transform pointer at +0x00 is 16-byte-aligned into pad at +0x10.
 */
typedef struct PfxFontString {
    PfxFontTransform* transform; /* +0x00 - aligned into pad04 */
    char pad04[0x50]; /* +0x04 - holds 4x4 matrix when transform points here */
    int height;                /* +0x54 */
    int width;                 /* +0x58 */
    union {
        FontFace* face;        /* +0x5C pfx font face */
        RwTexture* texture;    /* +0x5C native texture upload view */
    };
    PfxFontInstance instance0; /* +0x60 .. +0x8F */
} PfxFontString; /* 0x90 */

void pfxfont_system_init(PfxFontAllocFn alloc_fn, PfxFontFreeFn free_fn);
void pfxfont_system_shutdown(void);
/* Retail callers pass FontTableEntry.slot.metrics / PfxFontSlot.metrics (+0x14 / +0x04). */
int pfxfont_get_width(FontMetrics* metrics, const char* text);
int pfxfont_get_height(FontMetrics* metrics, const char* text);
void pfxfont_set_string_color(PfxFontString* dest, unsigned int* color);
void pfxfont_string_init(PfxFontString* ctx);
/* Retail source order: float wrap_w then int halign (prologue fmr then mr). */
void pfxfont_string_set(PfxFontString* ctx, PfxFontSlot* font, const char* text, float wrap_w,
                        int halign);
void pfxfont_set_transform(PfxFontString* ctx, const void* matrix44);
void pfxfont_string_render(PfxFontString* ctx, float x, float y);
void pfxfont_begin_render(void);
void pfxfont_end_render(void);
void pfxfont_string_cleanup(PfxFontString* ctx);

#endif
