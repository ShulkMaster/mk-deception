#include "runtime/fonts.h"

#include "runtime/image.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_vtbl.h"
#include "runtime/utils.h"


#ifndef NULL
#define NULL ((void*)0)
#endif


int stricmp(const char* a, const char* b);
FontFace* load_tga(int handle, unsigned int art_oid);
void* load_binary_block(int handle, unsigned int art_oid, int* out_size);
static const float kZeroHeight = 0.0f;

static int oid_to_kill_mask;
static int oid_to_kill;

static const double kFloat689 = 4503601774854144.0;

#include "runtime/fonts_data.inc"

static const char* fonts_default_text(void) {
    return &stringBase0[0x2E78];
}

static int fonts_half(int v) {
    return v / 2;
}

static float fonts_metrics_height(FontMetrics* metrics) {
    return metrics->cell_height;
}

static int fonts_find_key(const char* keys, char key) {
    int index;
    const char* walk;

    index = 0;
    walk = keys;
    while (*walk != '\0') {
        if (key == *walk) {
            return index;
        }
        index++;
        walk++;
    }
    return -1;
}


/* TODO: [near miss] 95.43%; CFG agrees; GPR coloring remains around the key-index helper. */
void rewrite_button_string(const char* keys, char* text, int swap, int* map) {
    char key;
    int key_idx;
    int bit;
    int code;
    int out_bit;

    while ((key = *text) != '\0') {
        key_idx = fonts_find_key(keys, key);
        if (key_idx >= 0) {
            bit = 1;
            out_bit = 0;
            code = map[key_idx * 3];
            if (swap != 0 && code == 0x2000) {
                code = 0x8000;
            } else if (swap != 0 && (unsigned int)code == 0x8000u) {
                code = 0x2000;
            }
            while (bit != code) {
                bit <<= 1;
                out_bit++;
            }
            *text = keys[out_bit];
        }
        text++;
    }
}

const char* get_string(int id) {
    int size;
    int lang;
    size = string_tbl_size;
    lang = get_language_setting();
    if (id < 0 || id > size) {
        return string_table[0].langs[0];
    }
    return string_table[id].langs[lang];
}

const char* get_string_ext(const char** table, int max_id, int id) {
    int lang;
    FontStringRow* rows;

    lang = get_language_setting();
    if (id < 0 || id > max_id) {
        return string_table[0].langs[0];
    }
    rows = (FontStringRow*)table;
    return rows[id].langs[lang];
}

void render_string_obj(StringObj* obj) {
    StringObjVisBits* bits;

    if (obj == NULL) {
        return;
    }
    bits = &obj->visibility;
    if (bits->hidden) {
        return;
    }
    if (suppress_normal_2d_items != 0 && bits->keep_when_suppress == 0) {
        return;
    }
    if (obj->pfx.face == NULL) {
        return;
    }
    pfxfont_begin_render();
    pfxfont_string_render(&obj->pfx, obj->render_x, obj->render_y);
    pfxfont_end_render();
}

static void _destroy_string_obj_oid_mask(MkHdr* hdr) {
    StringObj* obj;
    int oid;
    int mask;

    if (hdr->vtbl == &vtbl_mkpdata_string_obj) {
        obj = (StringObj*)hdr;
    } else {
        obj = NULL;
    }
    if (obj == NULL) {
        return;
    }
    oid = obj->oid;
    mask = oid_to_kill_mask;
    if ((unsigned int)oid_to_kill != (unsigned int)(oid & mask)) {
        return;
    }
    if (obj->pfx.face != NULL) {
        pfxfont_string_cleanup(&obj->pfx);
    }
    obj->instance = 0;
    mkhdr_memfree((MkHdr*)obj);
}

void del_string_obj_by_id(int oid) {
    MkPtr* head;
    int mask;

    mask = -1;
    head = screen_obj_list;
    oid_to_kill = oid;
    oid_to_kill_mask = mask;
    if (head != NULL && head->hdr != NULL) {
        apply_to_mklist(_destroy_string_obj_oid_mask, &screen_obj_list);
    }
}

int vdestroy_string_obj(StringObj* obj) {
    if (obj->pfx.face != NULL) {
        pfxfont_string_cleanup(&obj->pfx);
    }
    obj->instance = 0;
    mkhdr_memfree((MkHdr*)obj);
}

void destroy_string_obj(StringObj* obj) {
    if (obj->pfx.face != NULL) {
        pfxfont_string_cleanup(&obj->pfx);
    }
    obj->instance = 0;
    mkhdr_memfree((MkHdr*)obj);
}

void pull_string_obj(StringObj* obj) {
    MkHdr* hdr;
    MkPtr* node;

    if (obj != NULL) {
        hdr = as_mkhdr((MkHdr*)obj);
    } else {
        hdr = NULL;
    }
    node = find_in_mklist(hdr, &screen_obj_list);
    if (node != NULL) {
        node->hdr = NULL;
        destroy_mkptr(node);
    }
}

void destroy_fonts(void) {
    int i;

    for (i = 0; i < 18; i++) {
        font_table[i].slot.face = NULL;
        font_table[i].slot.metrics = NULL;
    }
}

float get_font_height(int font) {
    FontMetrics* metrics;

    metrics = font_table[font].slot.metrics;
    if (metrics != NULL) {
        return fonts_metrics_height(metrics);
    }
    return kZeroHeight;
}

void update_string_obj_pfx(StringObj* obj, PfxFontSlot* font, const char* text) {
    int halign;
    int valign;
    int font_height;
    int y_off;
    float h;

    if (text == NULL) {
        text = fonts_default_text();
    }
    obj->text = text;
    pfxfont_string_set(&obj->pfx, font, text, obj->wrap_w, obj->halign);
    obj->text_w = obj->pfx.width;
    obj->text_h = obj->pfx.height;
    halign = obj->halign;
    obj->render_x = obj->x;
    if (obj->wrap_w == 0) {
        switch (halign) {
        case 1:
            obj->render_x = obj->render_x - (obj->text_w / 2);
            break;
        case 2:
            obj->render_x = obj->render_x - obj->text_w;
            break;
        }
    }
    valign = obj->valign;
    h = font->metrics->cell_height;
    obj->render_y = obj->y;
    font_height = h;
    y_off = obj->y_off;
    if (y_off != 0) {
        switch (valign) {
        case 0:
            obj->render_y = obj->render_y - font_height;
            break;
        case 1:
            obj->render_y =
                (obj->text_h / 2) + (obj->y - (y_off / 2)) - font_height;
            break;
        case 2:
            obj->render_y = obj->render_y - y_off;
            break;
        }
    } else {
        switch (valign) {
        case 0:
            obj->render_y = obj->render_y - font_height;
            break;
        case 1:
            obj->render_y = obj->render_y - (font_height / 2);
            break;
        }
    }
}

/* TODO: [near miss] 96.47%; switch lattice matches; font_table rematerialization (addi r0) and render_y store slot remain. */
void update_string_obj(StringObj* obj, int font, const char* text) {
    int halign;
    int valign;
    int font_height;
    int y_off;
    float h;
    FontMetrics* metrics;

    if (text == NULL) {
        text = fonts_default_text();
    }
    obj->text = text;
    pfxfont_string_set(&obj->pfx, &font_table[font].slot, text, obj->wrap_w,
                       obj->halign);
    obj->text_w = obj->pfx.width;
    obj->text_h = obj->pfx.height;
    halign = obj->halign;
    obj->render_x = obj->x;
    if (obj->wrap_w == 0) {
        switch (halign) {
        case 1:
            obj->render_x = obj->render_x - (obj->text_w / 2);
            break;
        case 2:
            obj->render_x = obj->render_x - obj->text_w;
            break;
        }
    }
    valign = obj->valign;
    metrics = font_table[font].slot.metrics;
    h = metrics->cell_height;
    obj->render_y = obj->y;
    font_height = h;
    y_off = obj->y_off;
    if (y_off != 0) {
        switch (valign) {
        case 0:
            obj->render_y = obj->render_y - font_height;
            break;
        case 1:
            obj->render_y =
                (obj->text_h / 2) + (obj->y - (y_off / 2)) - font_height;
            break;
        case 2:
            obj->render_y = obj->render_y - y_off;
            break;
        }
    } else {
        switch (valign) {
        case 0:
            obj->render_y = obj->render_y - font_height;
            break;
        case 1:
            obj->render_y = obj->render_y - (font_height / 2);
            break;
        }
    }
}

float get_string_width_by_font_num(int font, const char* text) {
    int w;

    w = pfxfont_get_width(font_table[font].slot.metrics, text);
    return w;
}

void string_obj_set_valign(StringObj* obj, PfxFontSlot* font, int valign) {
    int y_off;
    int font_height;
    float h;

    h = font->metrics->cell_height;
    obj->valign = valign;
    obj->render_y = obj->y;
    font_height = h;
    y_off = obj->y_off;
    if (y_off != 0) {
        switch (valign) {
        case 0:
            obj->render_y = obj->render_y - font_height;
            break;
        case 1:
            obj->render_y =
                (obj->text_h / 2) + (obj->y - (y_off / 2)) - font_height;
            break;
        case 2:
            obj->render_y = obj->render_y - y_off;
            break;
        }
    } else {
        switch (valign) {
        case 0:
            obj->render_y = obj->render_y - font_height;
            break;
        case 1:
            obj->render_y = obj->render_y - (font_height / 2);
            break;
        }
    }
}

void string_obj_set_halign(StringObj* obj, int halign) {
    obj->halign = halign;
    obj->render_x = obj->x;
    if (obj->wrap_w == 0) {
        switch (halign) {
        case 1:
            obj->render_x = obj->render_x - obj->text_w / 2;
            break;
        case 2:
            obj->render_x = obj->render_x - obj->text_w;
            break;
        }
    }
}

StringObj* create_wrapped_string(int oid, PfxFontSlot* font, const char* text, int x, int y,
                                 int wrap_w, int y_off, int halign, int valign) {
    StringObj* obj;

    obj = (StringObj*)get_mkhdr(&vtbl_mkpdata_string_obj, 0xD0);
    if (obj != NULL) {
        mk_insert((MkHdr*)obj, &master_clean_up_list);
        obj->flags = 0;
        obj->oid = 0;
    }
    if (obj == NULL) {
        return NULL;
    }
    if (text == NULL) {
        text = fonts_default_text();
    }
    if (font == NULL || font->face == NULL || font->metrics == NULL) {
        return NULL;
    }
    obj->oid = oid;
    obj->text = text;
    pfxfont_string_init(&obj->pfx);
    pfxfont_string_set(&obj->pfx, font, text, wrap_w, halign);
    obj->wrap_w = wrap_w;
    obj->y_off = y_off;
    obj->x = x;
    obj->y = y;
    obj->text_w = obj->pfx.width;
    obj->text_h = obj->pfx.height;
    obj->halign = halign;
    obj->render_x = obj->x;
    if (obj->wrap_w == 0) {
        switch (halign) {
        case 1:
            obj->render_x = obj->render_x - obj->text_w / 2;
            break;
        case 2:
            obj->render_x = obj->render_x - obj->text_w;
            break;
        }
    }
    string_obj_set_valign(obj, font, valign);
    return obj;
}

StringObj* string_center_xy(int oid, int font, const char* text, int x, int y, int priority) {
    StringObj* obj;
    const char* str;
    PfxFontSlot* slot;
    FontTableEntry* entry;

    str = text;
    obj = (StringObj*)get_mkhdr(&vtbl_mkpdata_string_obj, 0xD0);
    if (obj != NULL) {
        mk_insert((MkHdr*)obj, &master_clean_up_list);
        obj->flags = 0;
        obj->oid = 0;
    }
    if (obj == NULL) {
        obj = NULL;
    } else {
        if (text == NULL) {
            str = fonts_default_text();
        }
        entry = &font_table[font];
        slot = &entry->slot;
        if (slot == NULL || entry->slot.face == NULL || entry->slot.metrics == NULL) {
            obj = NULL;
        } else {
            obj->oid = oid;
            obj->text = str;
            pfxfont_string_init(&obj->pfx);
            pfxfont_string_set(&obj->pfx, slot, str, 0.0f, 1);
            obj->wrap_w = 0;
            obj->y_off = 0;
            obj->x = x;
            obj->y = y;
            obj->text_w = obj->pfx.width;
            obj->text_h = obj->pfx.height;
            obj->halign = 1;
            obj->render_x = obj->x;
            if (obj->wrap_w == 0) {
                obj->render_x = obj->render_x - fonts_half(obj->text_w);
            }
            obj->valign = 2;
            obj->render_y = obj->y;
            if (obj->y_off != 0) {
                obj->render_y = obj->render_y - obj->y_off;
            }
        }
    }
    if (obj == NULL) {
        return NULL;
    }
    obj->priority = priority;
    insert_2d_obj((ScreenObj*)obj);
    return obj;
}

StringObj* string_right_xy(int oid, int font, const char* text, int x, int y, int priority) {
    StringObj* obj;
    const char* str;
    PfxFontSlot* slot;
    FontTableEntry* entry;

    str = text;
    obj = (StringObj*)get_mkhdr(&vtbl_mkpdata_string_obj, 0xD0);
    if (obj != NULL) {
        mk_insert((MkHdr*)obj, &master_clean_up_list);
        obj->flags = 0;
        obj->oid = 0;
    }
    if (obj == NULL) {
        obj = NULL;
    } else {
        if (text == NULL) {
            str = fonts_default_text();
        }
        entry = &font_table[font];
        slot = &entry->slot;
        if (slot == NULL || entry->slot.face == NULL || entry->slot.metrics == NULL) {
            obj = NULL;
        } else {
            obj->oid = oid;
            obj->text = str;
            pfxfont_string_init(&obj->pfx);
            pfxfont_string_set(&obj->pfx, slot, str, 0.0f, 2);
            obj->wrap_w = 0;
            obj->y_off = 0;
            obj->x = x;
            obj->y = y;
            obj->text_w = obj->pfx.width;
            obj->text_h = obj->pfx.height;
            obj->halign = 2;
            obj->render_x = obj->x;
            if (obj->wrap_w == 0) {
                obj->render_x = obj->render_x - obj->text_w;
            }
            obj->valign = 2;
            obj->render_y = obj->y;
            if (obj->y_off != 0) {
                obj->render_y = obj->render_y - obj->y_off;
            }
        }
    }
    if (obj == NULL) {
        return NULL;
    }
    obj->priority = priority;
    insert_2d_obj((ScreenObj*)obj);
    return obj;
}

StringObj* string_left_xy(int oid, int font, const char* text, int x, int y, int priority) {
    StringObj* obj;
    const char* str;
    PfxFontSlot* slot;
    FontTableEntry* entry;

    str = text;
    obj = (StringObj*)get_mkhdr(&vtbl_mkpdata_string_obj, 0xD0);
    if (obj != NULL) {
        mk_insert((MkHdr*)obj, &master_clean_up_list);
        obj->flags = 0;
        obj->oid = 0;
    }
    if (obj == NULL) {
        obj = NULL;
    } else {
        if (text == NULL) {
            str = fonts_default_text();
        }
        entry = &font_table[font];
        slot = &entry->slot;
        if (slot == NULL || entry->slot.face == NULL || entry->slot.metrics == NULL) {
            obj = NULL;
        } else {
            obj->oid = oid;
            obj->text = str;
            pfxfont_string_init(&obj->pfx);
            pfxfont_string_set(&obj->pfx, slot, str, 0.0f, 0);
            obj->wrap_w = 0;
            obj->y_off = 0;
            obj->x = x;
            obj->y = y;
            obj->text_w = obj->pfx.width;
            obj->text_h = obj->pfx.height;
            obj->halign = 0;
            obj->render_x = obj->x;
            obj->valign = 2;
            obj->render_y = obj->y;
            if (obj->y_off != 0) {
                obj->render_y = obj->render_y - obj->y_off;
            }
        }
    }
    if (obj == NULL) {
        return NULL;
    }
    obj->priority = priority;
    insert_2d_obj((ScreenObj*)obj);
    return obj;
}

/* TODO: [breakthrough needed] 89.39%; recover destructive mulli/stwu loop addressing and nonvolatile coloring. */
PfxFontSlot* load_named_font(const char* name) {
    int i;
    int offset;
    FontTableEntry* entry;
    FontTableEntry* walk;
    FontFace* face;
    int binary_id;
    int tga_arg;
    int handle;
    FontFace* tga;
    FontMetrics* bin;
    int flag;
    PfxFontSlot* dest;

    i = 0;
    offset = 0;
    do {
        walk = (FontTableEntry*)((unsigned char*)font_table + offset);
        if (stricmp(name, walk->name) == 0) {
            entry = &font_table[i];
            face = entry->slot.face;
            binary_id = entry->binary_id;
            tga_arg = entry->tga_arg;
            handle = (int)entry->path;
            if (face == NULL) {
                flag = 0;
                tga = load_tga(handle, tga_arg);
                bin = load_binary_block(handle, binary_id, &flag);
                dest = &font_table[i].slot;
                tga->flags_50 = (tga->flags_50 & 0xFFFFFF00u) | 1u;
                tga->flags_50 = (tga->flags_50 & 0xFFFF00FFu) | 0x3300u;
                dest->face = tga;
                dest->metrics = bin;
            }
            return &walk->slot;
        }
        i += 1;
        offset += sizeof(FontTableEntry);
    } while (i < 18);
    return NULL;
}

void unload_font(int slot) {
    font_table[slot].slot.face = NULL;
    font_table[slot].slot.metrics = NULL;
}

PfxFontSlot* load_font_in_slot(int slot, int handle, int tga_arg, int binary_id) {
    FontFace* tga;
    FontMetrics* bin;
    int flag;
    PfxFontSlot* dest;

    if (font_table[slot].slot.face == NULL) {
        flag = 0;
        tga = load_tga(handle, tga_arg);
        bin = load_binary_block(handle, binary_id, &flag);
        dest = &font_table[slot].slot;
        tga->flags_50 = (tga->flags_50 & 0xFFFFFF00u) | 1u;
        tga->flags_50 = (tga->flags_50 & 0xFFFF00FFu) | 0x3300u;
        dest->face = tga;
        dest->metrics = bin;
    }
    return &font_table[slot].slot;
}

PfxFontSlot* load_font(int slot) {
    FontFace* face;
    int handle;
    int binary_id;
    int tga_arg;
    FontFace* tga;
    FontMetrics* bin;
    int flag;
    PfxFontSlot* dest;

    face = font_table[slot].slot.face;
    binary_id = font_table[slot].binary_id;
    tga_arg = font_table[slot].tga_arg;
    handle = (int)font_table[slot].path;
    if (face == NULL) {
        flag = 0;
        tga = load_tga(handle, tga_arg);
        bin = load_binary_block(handle, binary_id, &flag);
        dest = &font_table[slot].slot;
        tga->flags_50 = (tga->flags_50 & 0xFFFFFF00u) | 1u;
        tga->flags_50 = (tga->flags_50 & 0xFFFF00FFu) | 0x3300u;
        dest->face = tga;
        dest->metrics = bin;
    }
    return &font_table[slot].slot;
}

static void delayed_free(void* mem) {
    free_mem_delayed(mem, 4);
}

void init_font_system(void) {
    pfxfont_system_init(get_mem, delayed_free);
}

void unhide_string_obj(StringObj* obj) {
    obj->visibility.hidden = 0;
}

void hide_string_obj(StringObj* obj) {
    obj->visibility.hidden = 1;
}
