#ifndef RUNTIME_FONTS_H
#define RUNTIME_FONTS_H

#include "libmkparticle/pfxfont.h"
#include "runtime/mk_struct.h"

/* MSB-first byte at +0x0C: hidden=0x80, keep_when_suppress=0x40 -> rlwimi/extrwi. */
typedef struct StringObjVisBits {
    unsigned char hidden : 1;
    unsigned char keep_when_suppress : 1;
    unsigned char pad : 6;
} StringObjVisBits;

struct StringObj;

typedef struct StringObjVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    int (*destroy)(struct StringObj* object);
} StringObjVtable;

/*
 * StringObj is 0xD0. pfx (PfxFontString, 0x90) sits at +0x3C through +0xCB;
 * priority at +0xCC. text_w/text_h copy from pfx.width/height after string_set.
 */
typedef struct StringObj {
    union {
        MkVtable5* vtbl;       /* +0x00 */
        StringObjVtable* typed_vtbl;
    };
    unsigned int instance; /* +0x04 */
    int oid;               /* +0x08 */
    union {
        int flags;                  /* +0x0C */
        StringObjVisBits visibility;
    };
    int x;                 /* +0x10 */
    int y;                 /* +0x14 */
    int wrap_w;            /* +0x18 */
    int y_off;             /* +0x1C */
    int halign;            /* +0x20 */
    int valign;            /* +0x24 */
    int render_x;          /* +0x28 */
    int render_y;          /* +0x2C */
    int text_w;            /* +0x30 */
    int text_h;            /* +0x34 */
    const char* text;      /* +0x38 */
    PfxFontString pfx;     /* +0x3C .. +0xCB */
    int priority;          /* +0xCC */
} StringObj;

/*
 * fonts.o - UI strings + pfxfont StringObj helpers.
 *
 * Retail: PRESS START string path (atm_setup_press_start_flasher /
 *   atm_old_mkda_logo):
 *   1. load_font(0)            - fill font_table[0].slot
 *   2. get_string(1)           - "PRESS START"
 *   3. string_center_xy(...)   - alloc StringObj, insert_2d_obj (oid 0x2010)
 *   4. Frame draw: render_2d_objs -> render_string_obj -> pfxfont_string_render
 *   5. p_flash_atm_text / hide_string_obj / unhide_string_obj toggle +0xC hidden
 * Soft ceiling: create_wrapped_string ~94%; load_named_font ~88.6%;
 *   render_string_obj 100%; get_string_width_by_font_num 100%;
 *   rewrite_button_string ~95%; update_string_obj ~88%; update_string_obj_pfx ~91%;
 *   set_valign ~84%.
 * Matched: load_font / load_font_in_slot / unload_font / init_font_system;
 *   string_center/right/left_xy; get_string / get_string_ext / vdestroy /
 *   del_by_id / pull; set_halign; _destroy oid mask; load_font returns PfxFontSlot*.
 */
typedef struct FontFace {
    char pad[0x50];
    unsigned int flags_50; /* +0x50; load_font sets low bytes 0x01 / 0x33 */
} FontFace;

/*
 * One font_table[] row (0x18). The face + metrics pair is a typed PfxFontSlot
 * at +0x10, matching the object returned by load_font.
 */
typedef struct FontTableEntry {
    char* name;            /* +0x00 */
    int tga_arg;           /* +0x04; second arg to load_tga */
    int binary_id;         /* +0x08 */
    char* path;            /* +0x0C; often 0; cast to handle for load_tga */
    PfxFontSlot slot;      /* +0x10: face + metrics */
} FontTableEntry;

/*
 * One string_table / get_string_ext row (0x18). Six language slots; get_language_setting()
 * indexes langs[]. Flat char* string_table[] stores rows packed back-to-back.
 */
typedef struct FontStringRow {
    const char* langs[6]; /* +0x00 -- EN/ES/DE/IT/FR/... */
} FontStringRow; /* 0x18 */

extern FontTableEntry font_table[18];
extern FontStringRow string_table[];
extern int string_tbl_size;

PfxFontSlot* load_font(int slot);
PfxFontSlot* load_font_in_slot(int slot, char* path, int tga_arg, int binary_id);
PfxFontSlot* load_named_font(const char* name);
void unload_font(int slot);
void init_font_system(void);
void destroy_fonts(void);

const char* get_string(int id);
/* table = FontStringRow*; max_id bounds id (misnamed lang_count in callers). */
const char* get_string_ext(const char** table, int max_id, int id);

void render_string_obj(StringObj* obj);
void destroy_string_obj(StringObj* obj);
int vdestroy_string_obj(StringObj* obj);
void pull_string_obj(StringObj* obj);
void del_string_obj_by_id(int oid);

float get_font_height(int font);
void update_string_obj_pfx(StringObj* obj, PfxFontSlot* font, const char* text);
void update_string_obj(StringObj* obj, int font, const char* text);
double get_string_width_by_font_num(int font, const char* text);
void string_obj_set_valign(StringObj* obj, PfxFontSlot* font, int valign);
void string_obj_set_halign(StringObj* obj, int halign);

StringObj* create_wrapped_string(int oid, PfxFontSlot* font, const char* text, int x, int y,
                                 int wrap_w, int y_off, int halign, int valign);
StringObj* string_center_xy(int oid, int font, const char* text, int x, int y, int priority);
StringObj* string_right_xy(int oid, int font, const char* text, int x, int y, int priority);
StringObj* string_left_xy(int oid, int font, const char* text, int x, int y, int priority);

void unhide_string_obj(StringObj* obj);
void hide_string_obj(StringObj* obj);

void rewrite_button_string(const char* keys, char* text, int swap, int* map);

#endif
