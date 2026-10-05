#ifndef RUNTIME_FONTS_H
#define RUNTIME_FONTS_H

#include "libmkparticle/pfxfont.h"
#include "rw/rwcore_types.h"
#include "runtime/mk_struct.h"

typedef struct StringObjVisBits {
    unsigned char hidden : 1;
    unsigned char keep_when_suppress : 1;
    unsigned char pad : 6;
} StringObjVisBits;

struct StringObj;

typedef struct StringObjVtable {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct StringObj* object);
} StringObjVtable;

typedef struct StringObj {
    union {
        MkVtable5* vtbl;
        StringObjVtable* typed_vtbl;
    };
    unsigned int instance;
    int oid;
    union {
        int flags;
        StringObjVisBits visibility;
    };
    int x;
    int y;
    int wrap_w;
    int y_off;
    int halign;
    int valign;
    int render_x;
    int render_y;
    int text_w;
    int text_h;
    const char* text;
    PfxFontString pfx;
    int priority;
} StringObj;

typedef struct FontTableEntry {
    char* name;
    int tga_arg;
    int binary_id;
    char* path;
    PfxFontSlot slot;
} FontTableEntry;

typedef struct FontStringRow {
    const char* langs[6];
} FontStringRow;

#ifdef __cplusplus
extern "C" {
#endif

extern FontTableEntry font_table[18];
extern FontStringRow string_table[];
extern int string_tbl_size;

PfxFontSlot* load_font(int slot);
PfxFontSlot* load_font_in_slot(int slot, int handle, int tga_arg, int binary_id);
PfxFontSlot* load_named_font(const char* name);
void unload_font(int slot);
void init_font_system(void);
void destroy_fonts(void);

const char* get_string(int id);

const char* get_string_ext(const char** table, int max_id, int id);

void render_string_obj(StringObj* obj);
void destroy_string_obj(StringObj* obj);
void vdestroy_string_obj(StringObj* obj);
void pull_string_obj(StringObj* obj);
void del_string_obj_by_id(int oid);

float get_font_height(int font);
void update_string_obj_pfx(StringObj* obj, PfxFontSlot* font, const char* text);
void update_string_obj(StringObj* obj, int font, const char* text);
float get_string_width_by_font_num(int font, const char* text);
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

#ifdef __cplusplus
}
#endif

#endif
