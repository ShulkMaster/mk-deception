/*
 * fonts.o - UI string table, pfxfont text objects, and font slot loading.
 *
 * Retail .rodata/.data generated in fonts_data.inc (tools/gen_fonts_data.py).
 *
 * Retail call contract (B16 P0 - see fonts.h): PRESS START path is
 * load_font(0) -> string_center_xy(get_string(1), ...) -> render_string_obj
 * via render_2d_objs. string_*_xy / load_font / load_font_in_slot 100%;
 * load_named_font soft-ceiling; TU NonMatching.
 */
#include "runtime/fonts.h"

#include "runtime/image.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_struct.h"
#include "runtime/utils.h"

extern MkVtable5 vtbl_mkpdata_string_obj;

#ifndef NULL
#define NULL ((void*)0)
#endif

extern int suppress_normal_2d_items;

int stricmp(const char* a, const char* b);
/* Same symbol as asset load_tga(handle, art_oid); font_table path field is often 0. */
FontFace* load_tga(int handle, unsigned int art_oid);
FontMetrics* load_binary_block(char* path, int id, int* out);
static const float kZeroHeight = 0.0f;

#if !defined(TARGET_PC)
#pragma section sdata_type ".sdata" ".sbss" data_mode=sda_rel
__declspec(section ".sdata") static int gap_07_8050F9E4_sdata;
#pragma section sdata_type
#else
static int gap_07_8050F9E4_sdata;
#endif

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

/* --- retail order below --- */

/* Soft ceiling: rewrite_button_string (~95.43%). The inlined key-index helper and
 * repeated swap guard reproduce retail CFG; remaining differences are pure GPR coloring. */
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
            /* Retail remaps only when swap != 0 (0x2000 <-> 0x8000 via cmplwi). */
            if (swap != 0 && code == 0x2000) {
                code = (int)0x8000;
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
    /* Load size before the call so MWCC emits stmw r30 (id + size). */
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
        /* Retail returns global string_table[0], not table[0]. */
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
    pfxfont_string_render(&obj->pfx, (float)obj->render_x, (float)obj->render_y);
    pfxfont_end_render();
}

static void _destroy_string_obj_oid_mask(MkHdr* hdr) {
    StringObj* obj;
    int oid;
    int mask;

    /* Retail: vtbl match -> keep ptr in r31, else NULL; then cmplwi. */
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

    /* Retail: mask in r0, load head, store oid, cmplwi head, store mask, then
     * also require head->hdr != NULL before apply_to_mklist. */
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
    /* Retail leaves r3 from mkhdr_memfree (no li r3,0). */
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

    /* Retail reads font_table[slot].slot.metrics (+0x14). */
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
    pfxfont_string_set(&obj->pfx, font, text, (float)obj->wrap_w, obj->halign);
    obj->text_w = obj->pfx.width;
    obj->text_h = obj->pfx.height;
    /* Retail: keep halign in r3; beq to right case, bge skip lattice. */
    halign = obj->halign;
    obj->render_x = obj->x;
    if (obj->wrap_w == 0) {
        if (halign != 2) {
            if (halign < 2) {
                if (halign < 1) {
                    /* skip */
                } else {
                    obj->render_x = obj->render_x - (obj->text_w / 2);
                }
            }
        } else {
            obj->render_x = obj->render_x - obj->text_w;
        }
    }
    /*
     * Retail: lwz valign, lfs/fctiwz height, stw render_y, lwz y_off;
     * y_off!=0 block first (beq to y_off==0 tail); valign==1 via beq to tail.
     */
    valign = obj->valign;
    h = font->metrics->cell_height;
    obj->render_y = obj->y;
    font_height = (int)h;
    y_off = obj->y_off;
    if (y_off != 0) {
        if (valign != 1) {
            if (valign < 1) {
                if (valign >= 0) {
                    obj->render_y = obj->render_y - font_height;
                }
            } else if (valign < 3) {
                obj->render_y = obj->render_y - y_off;
            }
        } else {
            obj->render_y =
                (obj->text_h / 2) + (obj->y - (y_off / 2)) - font_height;
        }
    } else if (valign != 1) {
        if (valign < 1) {
            if (valign >= 0) {
                obj->render_y = obj->render_y - font_height;
            }
        }
    } else {
        obj->render_y = obj->render_y - (font_height / 2);
    }
}

/* Soft ceiling: update_string_obj ~87.6% -- font_table rematerialize
 * (addi r0 vs r3) + valign bge/blt peephole; algo OK. Stop. */
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
    pfxfont_string_set(&obj->pfx, &font_table[font].slot, text, (float)obj->wrap_w,
                       obj->halign);
    obj->text_w = obj->pfx.width;
    obj->text_h = obj->pfx.height;
    halign = obj->halign;
    obj->render_x = obj->x;
    if (obj->wrap_w == 0) {
        if (halign != 2) {
            if (halign < 2) {
                if (halign < 1) {
                    /* skip */
                } else {
                    obj->render_x = obj->render_x - (obj->text_w / 2);
                }
            }
        } else {
            obj->render_x = obj->render_x - obj->text_w;
        }
    }
    valign = obj->valign;
    metrics = font_table[font].slot.metrics;
    h = metrics->cell_height;
    obj->render_y = obj->y;
    font_height = (int)h;
    y_off = obj->y_off;
    if (y_off != 0) {
        if (valign != 1) {
            if (valign < 1) {
                if (valign >= 0) {
                    obj->render_y = obj->render_y - font_height;
                }
            } else if (valign < 3) {
                obj->render_y = obj->render_y - y_off;
            }
        } else {
            obj->render_y =
                (obj->text_h / 2) + (obj->y - (y_off / 2)) - font_height;
        }
    } else if (valign != 1) {
        if (valign < 1) {
            if (valign >= 0) {
                obj->render_y = obj->render_y - font_height;
            }
        }
    } else {
        obj->render_y = obj->render_y - (font_height / 2);
    }
}

float get_string_width_by_font_num(int font, const char* text) {
    int w;

    w = pfxfont_get_width(font_table[font].slot.metrics, text);
    return (float)w;
}

void string_obj_set_valign(StringObj* obj, PfxFontSlot* font, int valign) {
    int y_off;
    int font_height;
    float h;

    /* Retail: lfs, stw valign, fctiwz, stw render_y; height in r7, y_off in r6. */
    h = font->metrics->cell_height;
    obj->valign = valign;
    obj->render_y = obj->y;
    font_height = (int)h;
    y_off = obj->y_off;
    if (y_off != 0) {
        if (valign != 1) {
            if (valign < 1) {
                if (valign >= 0) {
                    obj->render_y = obj->render_y - font_height;
                }
            } else if (valign < 3) {
                obj->render_y = obj->render_y - y_off;
            }
        } else {
            obj->render_y =
                (obj->text_h / 2) + (obj->y - (y_off / 2)) - font_height;
        }
    } else if (valign != 1) {
        if (valign < 1) {
            if (valign >= 0) {
                obj->render_y = obj->render_y - font_height;
            }
        }
    } else {
        obj->render_y = obj->render_y - (font_height / 2);
    }
}

void string_obj_set_halign(StringObj* obj, int halign) {
    obj->halign = halign;
    obj->render_x = obj->x;
    /* Retail: bnelr; beq to right; bgelr; bltlr; center fallthrough. */
    if (obj->wrap_w != 0) {
        return;
    }
    if (halign != 2) {
        if (halign >= 2) {
            return;
        }
        if (halign < 1) {
            return;
        }
        obj->render_x = obj->render_x - (obj->text_w / 2);
        return;
    }
    obj->render_x = obj->render_x - obj->text_w;
}

/* Soft ceiling: create_wrapped_string (~94%). Leftover: r5/r6 height vs
 * y_off coloring / halign switch layout; stop - no trash C.
 * Title PRESS START uses string_center_xy, not this. */
StringObj* create_wrapped_string(int oid, PfxFontSlot* font, const char* text, int x, int y,
                                 int wrap_w, int y_off, int halign, int valign) {
    StringObj* obj;
    int font_height;
    FontMetrics* metrics;

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
    pfxfont_string_set(&obj->pfx, font, text, (float)wrap_w, halign);
    obj->wrap_w = wrap_w;
    obj->y_off = y_off;
    obj->x = x;
    obj->y = y;
    obj->text_w = obj->pfx.width;
    obj->text_h = obj->pfx.height;
    obj->halign = halign;
    obj->render_x = obj->x;
    /* Retail adjusts X only when wrap_w == 0 (switch lattice on halign). */
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
    metrics = font->metrics;
    {
        float h;

        h = metrics->cell_height;
        obj->valign = valign;
        font_height = (int)h;
    }
    obj->render_y = obj->y;
    /* Retail: y_off!=0 block first; valign==1 via beq to case body. */
    if (obj->y_off != 0) {
        if (valign != 1) {
            if (valign < 1) {
                if (valign >= 0) {
                    obj->render_y = obj->render_y - font_height;
                }
            } else if (valign < 3) {
                obj->render_y = obj->render_y - obj->y_off;
            }
        } else {
            obj->render_y =
                obj->text_h / 2 + (obj->y - obj->y_off / 2) - font_height;
        }
    } else if (valign != 1) {
        if (valign < 1) {
            if (valign >= 0) {
                obj->render_y = obj->render_y - font_height;
            }
        }
    } else {
        obj->render_y = obj->render_y - font_height / 2;
    }
    return obj;
}

StringObj* string_center_xy(int oid, int font, const char* text, int x, int y, int priority) {
    /* PRESS START: oid 0x2010, font 0, text=get_string(1), y=0x41, pri=0x1D. */
    StringObj* obj;
    const char* str;
    PfxFontSlot* slot;
    FontTableEntry* entry;

    /* Early copy keeps text live for the NULL check while str is the working
     * pointer - retail mr r26,r25 then reuses the text NV for &entry->slot. */
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

/*
 * Soft ceiling: load_named_font ~77.0% -- typed walk++ + indexed table access;
 * leftover loop NV coloring blocks the retail destructive mulli + stwu.
 * Stop -- no void-star-star or register coax.
 */
PfxFontSlot* load_named_font(const char* name) {
    int i;
    int offset;
    FontTableEntry* entry;
    FontTableEntry* walk;
    FontFace* face;
    int binary_id;
    int tga_arg;
    char* path;
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
            path = entry->path;
            if (face == NULL) {
                flag = 0;
                tga = load_tga((int)(unsigned long)path, (unsigned int)tga_arg);
                bin = load_binary_block(path, binary_id, &flag);
                /* Rematerialize the indexed slot for the retail store schedule. */
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

/*
 * load_font / load_font_in_slot: fill font_table[slot].slot when NULL.
 * Install via PfxFontSlot* dest live across flag writes (emits retail stwu pair).
 */
PfxFontSlot* load_font_in_slot(int slot, char* path, int tga_arg, int binary_id) {
    FontFace* tga;
    FontMetrics* bin;
    int flag;
    PfxFontSlot* dest;

    if (font_table[slot].slot.face == NULL) {
        flag = 0;
        tga = load_tga((int)(unsigned long)path, (unsigned int)tga_arg);
        bin = load_binary_block(path, binary_id, &flag);
        /* Keep &slot live across flag writes so schedule tracks retail. */
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
    /* Decl order: later of path/binary_id gets lower NV (r28). Retail: r28=binary_id, r29=path. */
    char* path;
    int binary_id;
    int tga_arg;
    FontFace* tga;
    FontMetrics* bin;
    int flag;
    PfxFontSlot* dest;

    face = font_table[slot].slot.face;
    binary_id = font_table[slot].binary_id;
    tga_arg = font_table[slot].tga_arg;
    path = font_table[slot].path;
    if (face == NULL) {
        flag = 0;
        tga = load_tga((int)(unsigned long)path, (unsigned int)tga_arg);
        bin = load_binary_block(path, binary_id, &flag);
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
