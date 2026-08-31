#include "runtime/image.h"

#include "runtime/asset.h"
#include "runtime/cmath.h"
#include "runtime/cstring.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "rw/alphapass.h"

extern float game_speed;
extern MkVtable5 vtbl_mkpdata_string_obj;
extern MkVtable5 vtbl_screen_engine;

static void update_atc_block(AniTextureControl* atc);
static void _destroy_screen_obj_oid_mask(ScreenObj* obj);

RwTexture* material_get_texture_pointer(RpMaterial* mat, int flag);
void material_set_texture_pointer(RpMaterial* mat, RwTexture* tex, int flag);
int RwRasterGetNumLevels(RwRaster* raster);
void set_render_state(int state, int value);
Pfx2dObj* pfx2d_alloc_obj(void);
void pfx2d_free_obj(Pfx2dObj* obj);
void pfx2d_build_default_geometry(Pfx2dObj* obj);
void pfx2d_begin_render(void);
void pfx2d_end_render(void);
void pfx2d_render(Pfx2dObj* obj);
void render_string_obj(StringObj* obj);
void screen_engine_render(void);

int suppress_normal_2d_items;
MkPtr* ani_texture_control_list;
MkPtr* screen_obj_list;

static int oid_to_kill_mask;
static int oid_to_kill;

static const float kZero = 0.0f;
static const float kOne = 1.0f;
static const float kHalf = 0.5f;
static const float kNegOne = -1.0f;

/* MWCC int->float helpers (sdata2 doubles). */
static float u32_to_float(unsigned int v) {
    return (float)v;
}

static float s32_to_float(int v) {
    return (float)v;
}

/*
 * StringObj-compatible priority view. Using the complete StringObj here
 * changes MWCC alias scheduling in the three RTTI ladders below.
 */
typedef struct ImageStringObjView {
    char pad[0xCC];
    int priority; /* +0xCC */
} ImageStringObjView;

/* Typed plugin accessors - same codegen as (char*)+LocalOffset. */
#define mkmaterial_plugin(mat) \
    ((MkmaterialPluginData*)((char*)(mat) + MkmaterialLocalOffset))
#define mkobj_clump_ext(clump) \
    (*(ImageClumpExt**)((char*)(clump) + MkobjLocalOffset))

void toggle_normal_2d_rendering(int enable) {
    suppress_normal_2d_items = enable == 0;
}

void insert_ani_texture_control_item(AniTextureControl* atc, AniTextureControlItem* item) {
    item->atc = atc;
    item->instance = atc->instance;
}

/* Soft ceiling: ck_ani_texture_control_item (~91.5%) --
 * retail joins fail paths via r5 + extra b; branch peephole leftover; stop.
 * (Q28 two-var keep does NOT work for directly-returned latches: MWCC
 * tail-duplicates the returns instead of joining.) */
AniTextureControl* ck_ani_texture_control_item(AniTextureControlItem* item) {
    AniTextureControl* atc;

    atc = item->atc;
    if (atc != 0) {
        if ((unsigned int)atc->instance != (unsigned int)item->instance) {
            atc = 0;
        }
    } else {
        atc = 0;
    }
    return atc;
}

int is_raster_power_of_two(RwRaster* raster) {
    int lw;
    int lh;
    int w;
    int h;
    int t;

    w = raster->width;
    lw = -1;
    for (t = w; t != 0; t >>= 1) {
        lw += 1;
    }
    h = raster->height;
    lh = -1;
    for (t = h; t != 0; t >>= 1) {
        lh += 1;
    }
    if ((1 << lw) != w) {
        return 0;
    }
    return (h - (1 << lh)) == 0;
}

void set_ani_texture_screen_obj(AniTextureControl* atc, ScreenObj* obj) {
    atc->screen_obj = obj;
    atc->screen_obj_instance = obj->instance;
}

void ani_texture_has_alpha_frames(AniTextureControl* atc) {
    atc->alpha_flag_bits.alpha_frames = 1;
}

void set_ani_texture_framerate(AniTextureControl* atc, float rate) {
    atc->framerate = rate;
}

void set_ani_texture_frame(AniTextureControl* atc, int frame) {
    atc->frame_f = u32_to_float((unsigned int)frame);
}

void set_ani_texture_rwtexture_a(AniTextureControl* atc, int index, RwTexture* tex) {
    atc->alpha_textures[index] = tex;
}

void set_ani_texture_rwtexture(AniTextureControl* atc, int index, RwTexture* tex) {
    atc->textures[index] = tex;
}

RwTexture* get_ani_texture_rwtexture(AniTextureControl* atc, int index) {
    return atc->textures[index];
}

void set_ani_texture_numframes(AniTextureControl* atc, int n) {
    atc->numframes = n;
}

int get_ani_texture_numframes(AniTextureControl* atc) {
    return atc->numframes;
}

void stop_ani_texture_control(void) {
    destroy_list(&ani_texture_control_list);
    destroy_mkprocs_pid(0x4002);
}

void start_ani_texture_control(void) {
    int flags[2];
    MkProc* proc;

    ani_texture_control_list = 0;
    flags[1] = 0;
    flags[0] = 0;
    proc = get_mkproc_nostack(flags);
    create_mkproc(0x10, proc, 0x4002, (MkProcEntryFn)p_animate_textures, 0);
}

void pull_ani_texture_control(AniTextureControl* atc) {
    mk_pull((MkHdr*)atc, &ani_texture_control_list);
}

void insert_ani_texture_control(AniTextureControl* atc) {
    mk_insert((MkHdr*)atc, &ani_texture_control_list);
}

int vdestroy_ani_texture_control(AniTextureControl* atc) {
    atc->instance = 0;
    mkhdr_memfree((MkHdr*)atc);
}

int destroy_ani_texture_control(AniTextureControl* atc) {
    atc->instance = 0;
    mkhdr_memfree((MkHdr*)atc);
}

void unhide_screen_obj(ScreenObj* obj) {
    obj->flag_bits.hidden = 0;
}

void hide_screen_obj(ScreenObj* obj) {
    obj->flag_bits.hidden = 1;
}

void init_2d_obj_lists(void) {
    screen_obj_list = 0;
}

AniTextureControl* get_ani_texture_control(void) {
    AniTextureControl* atc;

    atc = (AniTextureControl*)get_mkhdr(&vtbl_ani_texture_control, 0x198);
    if (atc != 0) {
        atc->frame = 0;
        /* Single stw clears flags + flags_hi (adjacent ushorts @ +0x0C). */
        atc->flags_word = 0;
        atc->frame_f = kZero;
        atc->numframes = 0;
        atc->framerate = kZero;
        atc->name = 0;
        memset(atc->materials, 0, 0xc);
        atc->atomic = 0;
        atc->screen_obj = 0;
        atc->screen_obj_instance = 0;
        memset(atc->textures, 0, 0xb0);
        memset(atc->alpha_textures, 0, 0xb0);
    }
    return atc;
}

float p_animate_textures(void) {
    apply_to_mklist((MkListApplyFn)update_atc_block, &ani_texture_control_list);
    return kOne;
}

void delete_screen_obj_oid(int oid) {
    /* Retail stores oid then mask. */
    oid_to_kill = oid;
    oid_to_kill_mask = -1;
    apply_to_mklist((MkListApplyFn)_destroy_screen_obj_oid_mask, &screen_obj_list);
}

static void _destroy_screen_obj_oid_mask(ScreenObj* obj) {
    ScreenObj* screen;

    if (obj->vtbl == &vtbl_mkpdata_screen_obj) {
        screen = obj;
    } else {
        screen = 0;
    }
    if (screen != 0 && (unsigned int)oid_to_kill == (screen->oid & oid_to_kill_mask)) {
        if (screen->pfx2d != 0) {
            pfx2d_free_obj(screen->pfx2d);
        }
        screen->instance = 0;
        mkhdr_memfree((MkHdr*)screen);
    }
}

int vdestroy_screen_obj(ScreenObj* obj) {
    ScreenObj* screen;

    /* Retail inlines as_* without null-guarding the cast result. */
    if (obj->vtbl == &vtbl_mkpdata_screen_obj) {
        screen = obj;
    } else {
        screen = 0;
    }
    if (screen->pfx2d != 0) {
        pfx2d_free_obj(screen->pfx2d);
    }
    screen->instance = 0;
    mkhdr_memfree((MkHdr*)screen);
    /* Retail leaves r3 from mkhdr_memfree (no li r3,0). */
}

int destroy_screen_obj(ScreenObj* obj) {
    if (obj->pfx2d != 0) {
        pfx2d_free_obj(obj->pfx2d);
    }
    obj->instance = 0;
    mkhdr_memfree((MkHdr*)obj);
    /* Retail leaves r3 from mkhdr_memfree (no li r3,0). */
}

void pull_screen_obj(ScreenObj* obj) {
    MkHdr* hdr;
    MkPtr* ptr;

    /* Retail: beq-to-null / fallthrough as_mkhdr (not bne early-return). */
    if (obj != 0) {
        hdr = as_mkhdr((MkHdr*)obj);
    } else {
        hdr = 0;
    }
    ptr = find_in_mklist(hdr, &screen_obj_list);
    if (ptr != 0) {
        ptr->hdr = 0;
        destroy_mkptr(ptr);
    }
}

/* Keep these out of xy wrappers (retail calls, does not inline). */
#if !defined(TARGET_PC)
#pragma dont_inline on
#endif
ScreenObj* load_named_2d_pfxobj(int slot, int oid, const char* name, int flags, int priority) {
    RwTexture* tex;
    ScreenObj* obj;
    int saved_oid;
    int saved_flags;
    int saved_pri;

    /* Decl/assign order -> stmw r29..r31 then mr r29/r30/r31 (retail). */
    saved_oid = oid;
    saved_flags = flags;
    saved_pri = priority;
    tex = load_named_tga_from_slot(slot, name);
    if (tex != 0) {
        obj = load_2d_pfxobj_with_texture(saved_oid, tex, saved_flags, saved_pri);
    } else {
        obj = 0;
    }
    return obj;
}

ScreenObj* load_2d_pfxobj(int slot, int oid, char* name, int flags, int priority) {
    RwTexture* tex;
    ScreenObj* obj;
    int saved_oid;
    int saved_flags;
    int saved_pri;

    saved_oid = oid;
    saved_flags = flags;
    saved_pri = priority;
    tex = load_tga(slot, (unsigned int)name);
    if (tex != 0) {
        obj = load_2d_pfxobj_with_texture(saved_oid, tex, saved_flags, saved_pri);
    } else {
        obj = 0;
    }
    return obj;
}
#if !defined(TARGET_PC)
#pragma dont_inline reset
#endif

ScreenObj* load_named_2d_pfxobj_xy(int slot, int oid, const char* name, int flags, int x, int y,
                                    int priority) {
    ScreenObj* obj;
    int saved_x;
    int saved_y;
    int saved_pri;

    /* Retail: mr r31,r9; mr r29,r7; mr r30,r8; mr r7,r31 */
    saved_pri = priority;
    saved_x = x;
    saved_y = y;
    obj = load_named_2d_pfxobj(slot, oid, name, flags, saved_pri);
    if (obj != 0) {
        obj->x = saved_x;
        obj->y = saved_y;
        obj->priority = saved_pri;
    } else {
        obj = 0;
    }
    return obj;
}

ScreenObj* load_2d_pfxobj_xy(int slot, int oid, char* name, int flags, int x, int y, int priority) {
    ScreenObj* obj;
    int saved_x;
    int saved_y;

    /* Retail: mr r30,r7; mr r31,r8; mr r7,r9 - only saves x/y, not priority. */
    saved_x = x;
    saved_y = y;
    obj = load_2d_pfxobj(slot, oid, name, flags, priority);
    if (obj != 0) {
        obj->x = saved_x;
        obj->y = saved_y;
    } else {
        obj = 0;
    }
    return obj;
}

RpMaterial* MaterialFindAniTexture(RpMaterial* material, void* data) {
    AniTextureControl* atc = data;
    AtcFlagBits* fbits;
    unsigned short mid;
    unsigned int mat_id;
    RwTexture* tex;
    MkmaterialPluginData* mat_plugin;

    tex = material->texture;
    if (tex != 0) {
        if (atc->name != 0) {
            if (stricmp(tex->name, atc->name) != 0) {
                return material;
            }
        } else {
            mid = (unsigned short)((atc->flags >> 3) & 0xff);
            if (mid != 0) {
                mat_plugin = mkmaterial_plugin(material);
                mat_id = mat_plugin->flags & 0xfff;
                if (mat_id != mid) {
                    return material;
                }
            }
        }
fbits = &atc->flag_bits;
        atc->materials[fbits->count] = material;
        fbits->count = fbits->count + 1;
        /* Prefer return 0 (li r3) over material=0 (li r31). */
        if (fbits->multi == 0 || fbits->count >= 3) {
            return 0;
        }
    }
    return material;
}

RpAtomic* AtomicFindAniTexture(RpAtomic* atomic, void* data) {
    AniTextureControl* atc = data;
    AtcFlagBits* fbits;
    RpGeometry* geom;

    geom = atomic->geometry;
    if (geom != 0) {
        RpGeometryForAllMaterials(geom, MaterialFindAniTexture, atc);
        fbits = &atc->flag_bits;
        if (fbits->multi == 0) {
            if (fbits->count != 0) {
                atc->atomic = atomic;
                return 0;
            }
        } else if (fbits->count >= 3) {
            return 0;
        }
    }
    return atomic;
}

AniTextureControl* find_atc_for_atomic_material_id(RpAtomic* atomic, unsigned int material_id) {
    /* Decl mat before atc -> retail atc=r6, mat=r5 (was swapped). */
    MkPtr* ptr;
    MkPtr* next;
    RpMaterial* mat;
    AniTextureControl* atc;
    unsigned int mid;
    MkmaterialPluginData* mat_plugin;

    if (&ani_texture_control_list != 0) {
        ptr = ani_texture_control_list;
        while (ptr != 0) {
            atc = (AniTextureControl*)ptr->hdr;
            if (ptr->instance != atc->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                mat = atc->materials[0];
                if (mat != 0) {
                    mat_plugin = mkmaterial_plugin(mat);
                    mid = mat_plugin->flags & 0xfff;
                    if (atc->atomic == atomic && mid == material_id) {
                        return atc;
                    }
                }
                ptr = ptr->next;
            }
        }
    }
    return 0;
}

/* Soft ceiling: update_atc_block (~92%+) -- loop NV coloring leftover. */
static void update_atc_block(AniTextureControl* atc) {
    int i;
    int count;
    AtcFlagBits* fbits;
    ScreenObj* raw;
    ScreenObj* screen;
    RwTexture* tex;
    RwTexture* alpha;
    int frame;

    /* Soft ceiling: ~94.4% -- lfs pair emission order, alpha NV color,
     * li vs mr zero; stop. */
    atc->frame_f = atc->framerate * game_speed + atc->frame_f;
    if (atc->frame_f >= s32_to_float(atc->numframes)) {
        atc->frame_f = (float)fmod((double)atc->frame_f, (double)s32_to_float(atc->numframes));
    }
    atc->frame = (int)atc->frame_f;
    frame = atc->frame;
    tex = atc->textures[frame];
    raw = atc->screen_obj;
    if (raw != 0) {
        /* Retail inlines the live-check helper; its own null check is dead
         * here (second beq reuses CR0) but still emitted. */
        if (raw != 0) {
            if ((unsigned int)raw->instance == (unsigned int)atc->screen_obj_instance) {
                screen = raw;
            } else {
                screen = 0;
            }
        } else {
            screen = 0;
        }
        if (screen == 0) {
            atc->instance = 0;
            mkhdr_memfree((MkHdr*)atc);
        } else if (screen->pfx2d != 0) {
            screen->pfx2d->texture = tex;
if (atc->flag_bits.alpha) {
                screen->pfx2d->alpha_texture = atc->alpha_textures[atc->frame];
            }
        }
    } else {
        fbits = &atc->flag_bits;
        count = fbits->count;
        if (count != 0) {
            i = 0;
            alpha = atc->alpha_textures[frame];
            while (i < count) {
        fbits = &atc->flag_bits;
                material_set_texture_pointer(atc->materials[i], tex, fbits->filter);
                if (fbits->alpha) {
                    RpMaterialSetAlphaPassTexture(atc->materials[i], alpha);
                }
                i += 1;
            }
        }
    }
}

/* Draw-path core for legal/logo/PRESS START (via load_2d_pfxobj*).
 * Soft ceiling: load_2d_pfxobj_with_texture (~97.8%) -- dead pfx2d_free on null path; stop. */
ScreenObj* load_2d_pfxobj_with_texture(int oid, RwTexture* texture, int flags, int priority) {
    ScreenObj* obj;
    Pfx2dObj* pfx;
    float tmp;
    unsigned int ff;
    ScreenObjDrawFlags* dflags;

    obj = (ScreenObj*)get_mkhdr(&vtbl_mkpdata_screen_obj, 0x38);
    if (obj != 0) {
        obj->flags_word = 0;
        obj->oid = 0;
        obj->texture = 0;
        obj->x = 0;
        obj->y = 0;
        obj->priority = 0;
        obj->field_0x20 = 0;
        obj->field_0x24 = 0;
        obj->scale_x = kOne;
        obj->scale_y = kOne;
        obj->blend = 0;
        obj->pfx2d = 0;
        mk_insert((MkHdr*)obj, &master_clean_up_list);
    }
    if (obj == 0) {
        return 0;
    }
    obj->oid = oid;
    obj->flags_word = flags;
    dflags = &obj->draw_flags;
    dflags->on = 1;
    dflags->bit6 = 0;
    obj->priority = priority;
    obj->texture = texture->raster;
    ff = texture->filter_flags;
    ff = (ff & 0xffff00ff) | 0x3300;
    texture->filter_flags = ff;
    pfx = pfx2d_alloc_obj();
    obj->pfx2d = pfx;
    if (obj->pfx2d == 0) {
        /* Retail inlines a cleanup helper whose own null check is dead here
         * (beq reuses CR0) but still emitted. */
        if (obj->pfx2d != 0) {
            pfx2d_free_obj(obj->pfx2d);
        }
        obj->instance = 0;
        mkhdr_memfree((MkHdr*)obj);
        return 0;
    }
    obj->pfx2d->texture = texture;
    pfx2d_build_default_geometry(obj->pfx2d);
    if (dflags->flip_u) {
        /* Flip U of verts 1<->2 and 0<->3 (retail +0x1C/+0x30, +0x08/+0x44). */
        pfx = obj->pfx2d;
        tmp = pfx->verts[2].u;
        pfx->verts[2].u = pfx->verts[1].u;
        obj->pfx2d->verts[1].u = tmp;
        pfx = obj->pfx2d;
        tmp = pfx->verts[3].u;
        pfx->verts[3].u = pfx->verts[0].u;
        obj->pfx2d->verts[0].u = tmp;
    }
    insert_screen_obj(obj);
    return obj;
}

ScreenObj* load_wiff_screen_pfxobj(int a, int b, int oid, AniTextureControl** out_atc, int flags,
                                   int priority) {
    ScreenObj* obj;
    int saved_pri;
    AniTextureControl* atc;

    /* Retail: mr r30,r8 (pri) then mr. r29,r3 (atc); r30 later reused for obj.
     * Soft ceiling: ~99.9% -- sdata2 reloc label only (kNegOne). */
    saved_pri = priority;
    atc = get_wiff_atc_block(a, b);
    if (atc == 0) {
        return 0;
    }
    obj = load_2d_pfxobj_with_texture(oid, atc->textures[0], flags, saved_pri);
    if (atc->flag_bits.alpha) {
        obj->pfx2d->alpha_texture = atc->alpha_textures[atc->frame];
    }
    if (obj == 0) {
        atc->instance = 0;
        mkhdr_memfree((MkHdr*)atc);
        return 0;
    }
    atc->screen_obj = obj;
    atc->screen_obj_instance = obj->instance;
    mk_insert((MkHdr*)atc, &ani_texture_control_list);
    if ((obj->flags >> 5) & 1) {
        obj->pfx2d->scale_x = kNegOne;
        obj->pfx2d->mirror = 1;
    }
    *out_atc = atc;
    return obj;
}

ScreenObj* insert_2d_obj(ScreenObj* obj) {
    /* Soft ceiling: insert_2d_obj (~99.29%) -- pri/ptr r29/r31 NV unreproducible; stop.
     * Tried: pri-before-ptr decl (-0.5%), unsigned cmplw (-1%), null-first diamond (-18%),
     * invert blt branch (-2.5%). Keep if/else RTTI ladder + signed cmp. */
    MkPtr* ptr;
    MkPtr* next;
    ScreenObj* cur;
    ScreenObj* as_screen;
    ImageStringObjView* as_string;
    ScreenObj* as_engine;
    MkVtable5* vtbl;
    int pri;
    int cur_pri;
    MkPtr* insert;

    vtbl = obj->vtbl;
    if (vtbl == &vtbl_mkpdata_screen_obj) {
        as_screen = obj;
    } else {
        as_screen = 0;
    }
    if (as_screen != 0) {
        pri = as_screen->priority;
    } else {
        if (vtbl == &vtbl_mkpdata_string_obj) {
            as_string = (ImageStringObjView*)obj;
        } else {
            as_string = 0;
        }
        if (as_string != 0) {
            pri = as_string->priority;
        } else {
            if (vtbl == &vtbl_screen_engine) {
                as_engine = obj;
            } else {
                as_engine = 0;
            }
            if (as_engine != 0) {
                pri = 0x11;
            } else {
                pri = 0;
            }
        }
    }
    if (pri == 0) {
        mk_append((MkHdr*)obj, &screen_obj_list);
        return obj;
    }
    if (&screen_obj_list != 0) {
        ptr = screen_obj_list;
        while (ptr != 0) {
            cur = (ScreenObj*)ptr->hdr;
            if (ptr->instance != cur->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                vtbl = cur->vtbl;
                if (vtbl == &vtbl_mkpdata_screen_obj) {
                    as_screen = cur;
                } else {
                    as_screen = 0;
                }
                if (as_screen != 0) {
                    cur_pri = as_screen->priority;
                } else {
                    if (vtbl == &vtbl_mkpdata_string_obj) {
                        as_string = (ImageStringObjView*)cur;
                    } else {
                        as_string = 0;
                    }
                    if (as_string != 0) {
                        cur_pri = as_string->priority;
                    } else {
                        if (vtbl == &vtbl_screen_engine) {
                            as_engine = cur;
                        } else {
                            as_engine = 0;
                        }
                        if (as_engine != 0) {
                            cur_pri = 0x11;
                        } else {
                            cur_pri = 0;
                        }
                    }
                }
                /* Retail: cmpw pri,cur_pri; blt continue; else insert. */
                if (pri >= cur_pri) {
                    insert = get_mkptr_owns_mkhdr((MkHdr*)obj);
                    insert_mkptr_before(insert, ptr);
                    return obj;
                }
                ptr = ptr->next;
            }
        }
    }
    mk_append((MkHdr*)obj, &screen_obj_list);
    return obj;
}

ScreenObj* insert_string_obj(ScreenObj* obj) {
    /* Soft ceiling: insert_string_obj (~98.5%+) -- NV coloring; stop. */
    MkPtr* ptr;
    MkPtr* next;
    ScreenObj* cur;
    ScreenObj* as_screen;
    ImageStringObjView* as_string;
    MkVtable5* vtbl;
    int pri;
    int cur_pri;
    MkPtr* insert;

    vtbl = obj->vtbl;
    if (vtbl == &vtbl_mkpdata_screen_obj) {
        as_screen = obj;
    } else {
        as_screen = 0;
    }
    if (as_screen != 0) {
        pri = as_screen->priority;
    } else {
        if (vtbl == &vtbl_mkpdata_string_obj) {
            as_string = (ImageStringObjView*)obj;
        } else {
            as_string = 0;
        }
        if (as_string != 0) {
            pri = as_string->priority;
        } else {
            if (vtbl == &vtbl_screen_engine) {
                as_screen = obj;
            } else {
                as_screen = 0;
            }
            if (as_screen != 0) {
                pri = 0x11;
            } else {
                pri = 0;
            }
        }
    }
    if (pri == 0) {
        mk_append((MkHdr*)obj, &screen_obj_list);
        return obj;
    }
    if (&screen_obj_list != 0) {
        ptr = screen_obj_list;
        while (ptr != 0) {
            cur = (ScreenObj*)ptr->hdr;
            if (ptr->instance != cur->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                vtbl = cur->vtbl;
                if (vtbl == &vtbl_mkpdata_screen_obj) {
                    as_screen = cur;
                } else {
                    as_screen = 0;
                }
                if (as_screen != 0) {
                    cur_pri = as_screen->priority;
                } else {
                    if (vtbl == &vtbl_mkpdata_string_obj) {
                        as_string = (ImageStringObjView*)cur;
                    } else {
                        as_string = 0;
                    }
                    if (as_string != 0) {
                        cur_pri = as_string->priority;
                    } else {
                        /* Retail zeros cur when vtbl is not screen_engine. */
                        if (vtbl != &vtbl_screen_engine) {
                            cur = 0;
                        }
                        if (cur != 0) {
                            cur_pri = 0x11;
                        } else {
                            cur_pri = 0;
                        }
                    }
                }
                if (pri >= cur_pri) {
                    insert = get_mkptr_owns_mkhdr((MkHdr*)obj);
                    insert_mkptr_before(insert, ptr);
                    return obj;
                }
                ptr = ptr->next;
            }
        }
    }
    mk_append((MkHdr*)obj, &screen_obj_list);
    return obj;
}

ScreenObj* insert_screen_obj(ScreenObj* obj) {
    /* Soft ceiling: insert_screen_obj (~98.5%+) -- NV coloring; stop. */
    MkPtr* ptr;
    MkPtr* next;
    ScreenObj* cur;
    ScreenObj* as_screen;
    ImageStringObjView* as_string;
    MkVtable5* vtbl;
    int pri;
    int cur_pri;
    MkPtr* insert;

    vtbl = obj->vtbl;
    if (vtbl == &vtbl_mkpdata_screen_obj) {
        as_screen = obj;
    } else {
        as_screen = 0;
    }
    if (as_screen != 0) {
        pri = as_screen->priority;
    } else {
        if (vtbl == &vtbl_mkpdata_string_obj) {
            as_string = (ImageStringObjView*)obj;
        } else {
            as_string = 0;
        }
        if (as_string != 0) {
            pri = as_string->priority;
        } else {
            if (vtbl == &vtbl_screen_engine) {
                as_screen = obj;
            } else {
                as_screen = 0;
            }
            if (as_screen != 0) {
                pri = 0x11;
            } else {
                pri = 0;
            }
        }
    }
    if (pri == 0) {
        mk_append((MkHdr*)obj, &screen_obj_list);
        return obj;
    }
    if (&screen_obj_list != 0) {
        ptr = screen_obj_list;
        while (ptr != 0) {
            cur = (ScreenObj*)ptr->hdr;
            if (ptr->instance != cur->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                vtbl = cur->vtbl;
                if (vtbl == &vtbl_mkpdata_screen_obj) {
                    as_screen = cur;
                } else {
                    as_screen = 0;
                }
                if (as_screen != 0) {
                    cur_pri = as_screen->priority;
                } else {
                    if (vtbl == &vtbl_mkpdata_string_obj) {
                        as_string = (ImageStringObjView*)cur;
                    } else {
                        as_string = 0;
                    }
                    if (as_string != 0) {
                        cur_pri = as_string->priority;
                    } else {
                        if (vtbl != &vtbl_screen_engine) {
                            cur = 0;
                        }
                        if (cur != 0) {
                            cur_pri = 0x11;
                        } else {
                            cur_pri = 0;
                        }
                    }
                }
                if (pri >= cur_pri) {
                    insert = get_mkptr_owns_mkhdr((MkHdr*)obj);
                    insert_mkptr_before(insert, ptr);
                    return obj;
                }
                ptr = ptr->next;
            }
        }
    }
    mk_append((MkHdr*)obj, &screen_obj_list);
    return obj;
}

/* Layered ScreenObj / string / screen-engine draw (legal, logo, PRESS START). */
void render_2d_objs(int layer) {
    ScreenObj* obj;
    MkPtr* ptr;
    MkPtr* next;
    ScreenObj* screen;
    StringObj* str;
    ScreenObj* eng;
    MkVtable5* vtbl;
    ScreenObjFlags* hflags;

    if (&screen_obj_list != 0) {
        ptr = screen_obj_list;
        while (ptr != 0) {
            obj = (ScreenObj*)ptr->hdr;
            if (ptr->instance != obj->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                vtbl = obj->vtbl;
                if (vtbl == &vtbl_mkpdata_screen_obj) {
                    screen = obj;
                } else {
                    screen = 0;
                }
                if (screen != 0) {
                    if (layer == ((obj->flags >> 2) & 1)) {
                        set_render_state(6, 0);
                        set_render_state(8, 0);
                        set_render_state(7, 2);
                        pfx2d_begin_render();
hflags = &obj->flag_bits;
                        if (hflags->hidden == 0
                            && (suppress_normal_2d_items == 0 || ((obj->flags >> 1) & 1) != 0)
                            && obj->pfx2d != 0) {
                            if (obj->blend != 0) {
                                set_render_state(0xa, (int)(obj->blend >> 16));
                                set_render_state(0xb, (int)(obj->blend & 0xffff));
                            }
                            obj->pfx2d->x = obj->x;
                            obj->pfx2d->y = obj->y;
                            obj->pfx2d->scale_x = obj->scale_x;
                            obj->pfx2d->scale_y = obj->scale_y;
                            pfx2d_render(obj->pfx2d);
                            if (obj->blend != 0) {
                                set_render_state(0xa, 5);
                                set_render_state(0xb, 6);
                            }
                        }
                        pfx2d_end_render();
                    }
                } else {
                    if (vtbl == &vtbl_mkpdata_string_obj) {
                        str = (StringObj*)obj;
                    } else {
                        str = 0;
                    }
                    if (str != 0) {
                        render_string_obj((StringObj*)obj);
                    } else {
                        if (vtbl == &vtbl_screen_engine) {
                            eng = obj;
                        } else {
                            eng = 0;
                        }
                        if (eng != 0) {
                            screen_engine_render();
                        }
                    }
                }
                ptr = ptr->next;
            }
        }
    }
}

/* Soft ceiling: ~95.2% -- retail homes slot/name in r25/r31 (ours swapped);
 * both single-use at one call site, decl/copy levers do not move param
 * coloring; plus float pool label diffs. */
AniTextureControl* append_texture_by_name_to_atomic_material_id(int slot, char* name,
                                                                 RpAtomic* atomic,
                                                                 int material_id,
                                                                 int flag) {
    RpClump* clump;
    ImageClumpExt* clump_ext;
    AniTextureControl* atc;
    RwTexture* tex;
    RwRaster* raster;
    int levels;
    int pot;
    AtcFlagBits* fbits;


    clump = atomic->clump;
    if (clump == 0) {
        return 0;
    }
    clump_ext = mkobj_clump_ext(clump);
    if (clump_ext == 0) {
        return 0;
    }
    atc = get_ani_texture_control();
    if (atc == 0) {
        return 0;
    }
    atc->framerate = kZero;
    atc->numframes = 2;
    tex = load_named_tga_from_slot(slot, name);
    if (tex == 0) {
        atc->instance = 0;
        mkhdr_memfree((MkHdr*)atc);
        return 0;
    }
    raster = tex->raster;
    if (raster == 0 || (levels = RwRasterGetNumLevels(raster), levels <= 1)) {
        tex->filter_flags = (tex->filter_flags & 0xffffff00) | 2;
    } else {
        tex->filter_flags = (tex->filter_flags & 0xffffff00) | 4;
    }
    pot = is_raster_power_of_two(raster);
    if (pot != 0) {
        tex->filter_flags = (tex->filter_flags & 0xffff00ff) | 0x1100;
    } else {
        tex->filter_flags = (tex->filter_flags & 0xffff00ff) | 0x3300;
    }
    atc->textures[1] = tex;
    atc->flags = (unsigned short)(((material_id & 0xff) << 3) | (atc->flags & 0xf807));
    if (flag != 0) {
atc->flag_bits.filter = 1;
    }
    if (atomic->geometry != 0) {
        RpGeometryForAllMaterials(atomic->geometry, MaterialFindAniTexture, atc);
        fbits = &atc->flag_bits;
        if (fbits->multi == 0 && fbits->count != 0) {
            atc->atomic = atomic;
        }
    }
    if (atc->materials[0] == 0) {
        atc->instance = 0;
        mkhdr_memfree((MkHdr*)atc);
        return 0;
    }
    atc->textures[0] = material_get_texture_pointer(atc->materials[0], flag);
    mk_insert((MkHdr*)atc, &ani_texture_control_list);
    mk_insert((MkHdr*)atc, &clump_ext->atc_list);
    return atc;
}

AniTextureControl* attach_named_wiff_to_first_material(int slot, char* name, ImageMkSobj* mkobj) {
    AniTextureControl* atc;
    ImageClumpExt* clump_ext;
    AtcFlagBits* fbits;
    int i;
    int count;
    RwTexture* tex;
    RwTexture* alpha;
    RpAtomic* atomic;

    clump_ext = mkobj->clump_ext;
    atomic = mkobj->atomic;
    if (clump_ext == 0) {
        return 0;
    }
    atc = load_named_wiff_from_slot(slot, name);
    if (atc == 0) {
        return 0;
    }
    atc->framerate = kHalf;
    if (atomic->geometry != 0) {
        RpGeometryForAllMaterials(atomic->geometry, MaterialFindAniTexture, atc);
        fbits = &atc->flag_bits;
        if (fbits->multi == 0 && fbits->count != 0) {
            atc->atomic = atomic;
        }
    }
    if (atc->materials[0] == 0) {
        atc->instance = 0;
        mkhdr_memfree((MkHdr*)atc);
        return 0;
    }
        fbits = &atc->flag_bits;
    count = fbits->count;
    tex = atc->textures[atc->frame];
    alpha = atc->alpha_textures[atc->frame];
    /* Retail: li i,0; mr off,i -- share one zero. */
    i = 0;
    for (; i < count; i += 1) {
        fbits = &atc->flag_bits;
        material_set_texture_pointer(atc->materials[i], tex, fbits->filter);
        if (fbits->alpha) {
            RpMaterialSetAlphaPassTexture(atc->materials[i], alpha);
        }
    }
    mk_insert((MkHdr*)atc, &ani_texture_control_list);
    mk_insert((MkHdr*)atc, &clump_ext->atc_list);
    return atc;
}

AniTextureControl* attach_wiff_to_atomic_material(
    int slot, unsigned int art_oid, RpAtomic* atomic, char* tex_name) {
    AniTextureControl* atc;
    ImageClumpExt* clump_ext;
    AtcFlagBits* fbits;
    int i;
    int count;
    RwTexture* tex;
    RwTexture* alpha;
    RpClump* clump;

    clump = atomic->clump;
    if (clump == 0) {
        return 0;
    }
    clump_ext = mkobj_clump_ext(clump);
    if (clump_ext == 0) {
        return 0;
    }
    atc = get_wiff_atc_block(slot, art_oid);
    if (atc == 0) {
        return 0;
    }
    atc->name = tex_name;
    atc->framerate = kHalf;
    if (atomic->geometry != 0) {
        RpGeometryForAllMaterials(atomic->geometry, MaterialFindAniTexture, atc);
        fbits = &atc->flag_bits;
        if (fbits->multi == 0 && fbits->count != 0) {
            atc->atomic = atomic;
        }
    }
    if (atc->materials[0] == 0) {
        atc->instance = 0;
        mkhdr_memfree((MkHdr*)atc);
        return 0;
    }
        fbits = &atc->flag_bits;
    count = fbits->count;
    tex = atc->textures[atc->frame];
    alpha = atc->alpha_textures[atc->frame];
    i = 0;
    while (i < count) {
        fbits = &atc->flag_bits;
        material_set_texture_pointer(atc->materials[i], tex, fbits->filter);
        if (fbits->alpha) {
            RpMaterialSetAlphaPassTexture(atc->materials[i], alpha);
        }
        i += 1;
    }
    mk_insert((MkHdr*)atc, &ani_texture_control_list);
    mk_insert((MkHdr*)atc, &clump_ext->atc_list);
    return atc;
}

AniTextureControl* append_wiff_to_clump_material(int slot, char* name, RpClump* clump,
                                                 char* tex_name) {
    /* Soft ceiling: append_wiff_to_clump_material (~83%) -- bdnz shift loop; stop. */
    ImageClumpExt* clump_ext;
    AniTextureControl* atc;
    int n;
    int i;
    RpMaterial* mat;

    clump_ext = mkobj_clump_ext(clump);
    if (clump_ext == 0) {
        return 0;
    }
    atc = get_wiff_atc_block(slot, (int)name);
    if (atc == 0) {
        return 0;
    }
atc->flag_bits.multi = 1;
    atc->name = tex_name;
    atc->framerate = kZero;
    RpClumpForAllAtomics(clump, AtomicFindAniTexture, atc);
    mat = atc->materials[0];
    if (mat == 0) {
        atc->instance = 0;
        mkhdr_memfree((MkHdr*)atc);
        return 0;
    }
    n = atc->numframes;
    i = n - 1;
    atc->numframes = n + 1;
    if (i >= 0) {
        do {
            atc->textures[i + 1] = atc->textures[i];
            i -= 1;
        } while (i >= 0);
    }
    atc->textures[0] = mat->texture;
    mk_insert((MkHdr*)atc, &ani_texture_control_list);
    mk_insert((MkHdr*)atc, &clump_ext->atc_list);
    return atc;
}

/* Soft ceiling: append_wiff_to_clump_material_id (~83%) -- 3D WIFF helper;
 * not on legal/logo 2D path; stop Matching-grind. */
AniTextureControl* append_wiff_to_clump_material_id(int slot, char* name, RpClump* clump,
                                                    unsigned short material_id) {
    ImageClumpExt* clump_ext;
    AniTextureControl* atc;
    int n;
    int i;
    RpMaterial* mat;
    RwTexture* alpha;
    unsigned short flags_u;

    clump_ext = mkobj_clump_ext(clump);
    if (clump_ext == 0) {
        return 0;
    }
    atc = get_wiff_atc_block(slot, (int)name);
    if (atc == 0) {
        return 0;
    }
flags_u = atc->flags;
atc->flags =
        (unsigned short)((flags_u & 0xf807) | ((material_id & 0xff) << 3));
    atc->framerate = kZero;
    RpClumpForAllAtomics(clump, AtomicFindAniTexture, atc);
    mat = atc->materials[0];
    if (mat == 0) {
        atc->instance = 0;
        mkhdr_memfree((MkHdr*)atc);
        return 0;
    }
    alpha = RpMaterialGetAlphaPassTexture(mat);
    if (alpha != 0) {
atc->flag_bits.alpha = 1;
    }
    n = atc->numframes;
    i = n - 1;
    atc->numframes = n + 1;
    for (; i >= 0; i--) {
        atc->textures[i + 1] = atc->textures[i];
    if (atc->flag_bits.alpha) {
            atc->alpha_textures[i + 1] = atc->alpha_textures[i];
        }
    }
    atc->textures[0] = mat->texture;
    if (atc->flag_bits.alpha) {
        atc->alpha_textures[0] = alpha;
    }
    mk_insert((MkHdr*)atc, &ani_texture_control_list);
    mk_insert((MkHdr*)atc, &clump_ext->atc_list);
    return atc;
}
