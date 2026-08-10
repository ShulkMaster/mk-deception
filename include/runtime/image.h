#ifndef IMAGE_H
#define IMAGE_H

#include "runtime/mk_struct.h"
#include "libmkparticle/pfx2d.h"
#include "rw/rpworld_types.h"

extern MkVtable5 vtbl_ani_texture_control;
extern MkVtable5 vtbl_mkpdata_screen_obj;

typedef struct AniTextureControl AniTextureControl;
typedef struct AniTextureControlItem AniTextureControlItem;
typedef struct ScreenObj ScreenObj;
typedef struct ImageClumpExt ImageClumpExt;
typedef struct ImageMkSobj ImageMkSobj;
typedef struct StringObj StringObj;

/* ScreenObj +0x0C flags; the hide bit is 0x10. */
typedef struct ScreenObjFlags {
    unsigned char pad0 : 3;
    unsigned char hidden : 1;
    unsigned char pad1 : 4;
} ScreenObjFlags;

typedef struct ScreenObjDrawFlags {
    unsigned char on : 1;
    unsigned char bit6 : 1;
    unsigned char flip_u : 1;
    unsigned char pad : 5;
} ScreenObjDrawFlags;

typedef struct AtcFlagBits {
    unsigned char filter : 1;
    unsigned char alpha : 1;
    unsigned char multi : 1;
    unsigned char count : 2;
    unsigned char pad : 3;
} AtcFlagBits;

typedef struct AtcAlphaFlag {
    unsigned char pad0 : 1;
    unsigned char alpha_frames : 1;
    unsigned char pad1 : 6;
} AtcAlphaFlag;

/* ATC list handle: pointer + cached instance (ck_ani_texture_control_item). */
struct AniTextureControlItem {
    AniTextureControl* atc; /* +0x00 */
    int instance;           /* +0x04 */
};

/*
 * Animated texture control (0x198). Flags at +0x0c are packed:
 * bit7 owns/filter, bit6 alpha frames, bit5 multi-material mode,
 * bits4-3 material count (0..3), bits10-3 material id (ushort overlay).
 */
struct AniTextureControl {
    MkVtable5* vtbl;                 /* +0x00 */
    int instance;                    /* +0x04 */
    int frame;                       /* +0x08 */
    union {
        unsigned int flags_word;
        struct {
            union {
                unsigned char flags_byte;
                AtcFlagBits flag_bits;
                AtcAlphaFlag alpha_flag_bits;
            };
            unsigned char flags_byte_0D;
            unsigned short flags_hi;
        };
        struct {
            unsigned short flags;
            unsigned short flags_hi_word;
        };
    };                               /* +0x0C */
    float frame_f;                   /* +0x10 */
    int numframes;                   /* +0x14 */
    float framerate;                 /* +0x18 */
    char* name;                      /* +0x1C */
    RpMaterial* materials[3];        /* +0x20 */
    RpAtomic* atomic;                /* +0x2C */
    ScreenObj* screen_obj;           /* +0x30 */
    int screen_obj_instance;         /* +0x34 */
    RwTexture* textures[44];         /* +0x38 */
    RwTexture* alpha_textures[44];   /* +0xE8 */
};

/*
 * AniTextureControl screen link + ATC list (see AniTextureControl above).
 * Pfx2dObj lives in libmkparticle/pfx2d.h (0xD0 pool objects).
 */

/* Clump/mkobj extension blob: ATC list head at +0x28 (mk_insert target). */
struct ImageClumpExt {
    char pad00[0x28];
    MkPtr* atc_list; /* +0x28 */
};

/*
 * MkSobj-shaped object used by attach_named_wiff_to_first_material.
 * atomic @ +0x14 matches MkSobj; clump_ext @ +0x1C is image-local.
 */
struct ImageMkSobj {
    MkHdr hdr;                 /* +0x00 */
    char pad08[0x0C];
    RpAtomic* atomic;          /* +0x14 */
    void* frame;               /* +0x18 */
    ImageClumpExt* clump_ext;  /* +0x1C */
};

/*
 * 2D screen object (0x38). Priority at +0x1c; Pfx2dObj* at +0x34.
 * +0x20/+0x24 zeroed on create; no other image.o readers yet.
 *
 * Retail: 2D draw contract:
 *   Frame: display.Render() -> render_2d_objs(0)
 *   Per ScreenObj (vtbl_mkpdata_screen_obj) on matching layer bit:
 *     pfx2d_begin_render -> copy x/y/scale -> pfx2d_render -> pfx2d_end_render
 *     (end_render batches native2d_draw)
 *   ScreenEngine marker (vtbl_screen_engine):
 *     screen_engine_render -> ScreenMgr::Render
 *   load_*_2d_pfxobj* return NULL if load_tga fails -- no list entry.
 * Soft ceiling: load_2d_pfxobj_with_texture ~97.8%; insert_* ~98.6-99.3%;
 *   find_atc ~99.1%; update_atc_block ~92.9%; ck_ani ~91.5%; append_wiff ~83%.
 * Matched: render_2d_objs; MaterialFindAniTexture; load_wiff_screen_pfxobj.
 */
struct ScreenObj {
    MkVtable5* vtbl;       /* +0x00 */
    int instance;          /* +0x04 */
    int oid;               /* +0x08 */
    union {
        unsigned int flags_word;
        struct {
            union {
                unsigned char flags;
                ScreenObjFlags flag_bits;
                ScreenObjDrawFlags draw_flags;
            };
            unsigned char flags_pad[3];
        };
    };                     /* +0x0C */
    RwRaster* texture;     /* +0x10; raster from RwTexture* */
    int x;                 /* +0x14 */
    int y;                 /* +0x18 */
    int priority;          /* +0x1C */
    int field_0x20;        /* +0x20 */
    int field_0x24;        /* +0x24 */
    float scale_x;         /* +0x28 */
    float scale_y;         /* +0x2C */
    unsigned int blend;    /* +0x30 */
    Pfx2dObj* pfx2d;       /* +0x34 */
};

extern int suppress_normal_2d_items;
extern MkPtr* ani_texture_control_list;
extern MkPtr* screen_obj_list;

void toggle_normal_2d_rendering(int enable);
void insert_ani_texture_control_item(AniTextureControl* atc, AniTextureControlItem* item);
AniTextureControl* ck_ani_texture_control_item(AniTextureControlItem* item);

AniTextureControl* append_texture_by_name_to_atomic_material_id(
    int slot, char* name, RpAtomic* atomic, unsigned short material_id, int flag);
AniTextureControl* attach_named_wiff_to_first_material(int slot, char* name, ImageMkSobj* mkobj);
AniTextureControl* attach_wiff_to_atomic_material(int slot, char* name, RpAtomic* atomic, char* tex_name);
AniTextureControl* append_wiff_to_clump_material(int slot, char* name, RpClump* clump, char* tex_name);
AniTextureControl* append_wiff_to_clump_material_id(int slot, char* name, RpClump* clump, unsigned short material_id);

RpAtomic* AtomicFindAniTexture(RpAtomic* atomic, AniTextureControl* atc);
int is_raster_power_of_two(RwRaster* raster);
ScreenObj* load_wiff_screen_pfxobj(int a, int b, int oid, AniTextureControl** out_atc, int flags, int priority);
void set_ani_texture_screen_obj(AniTextureControl* atc, ScreenObj* obj);
RpMaterial* MaterialFindAniTexture(RpMaterial* material, void* data);

void ani_texture_has_alpha_frames(AniTextureControl* atc);
void set_ani_texture_framerate(AniTextureControl* atc, float rate);
void set_ani_texture_frame(AniTextureControl* atc, int frame);
void set_ani_texture_rwtexture_a(AniTextureControl* atc, int index, RwTexture* tex);
void set_ani_texture_rwtexture(AniTextureControl* atc, int index, RwTexture* tex);
RwTexture* get_ani_texture_rwtexture(AniTextureControl* atc, int index);
void set_ani_texture_numframes(AniTextureControl* atc, int n);
int get_ani_texture_numframes(AniTextureControl* atc);

void stop_ani_texture_control(void);
void start_ani_texture_control(void);
AniTextureControl* find_atc_for_atomic_material_id(RpAtomic* atomic, unsigned int material_id);
float p_animate_textures(void);
AniTextureControl* get_ani_texture_control(void);
void pull_ani_texture_control(AniTextureControl* atc);
void insert_ani_texture_control(AniTextureControl* atc);
int vdestroy_ani_texture_control(AniTextureControl* atc);
int destroy_ani_texture_control(AniTextureControl* atc);

void render_2d_objs(int layer);
ScreenObj* load_named_2d_pfxobj(int slot, int oid, const char* name, int flags, int priority);
ScreenObj* load_2d_pfxobj(int slot, int oid, char* name, int flags, int priority);
ScreenObj* load_named_2d_pfxobj_xy(int slot, int oid, const char* name, int flags, int x, int y,
                                    int priority);
ScreenObj* load_2d_pfxobj_xy(int slot, int oid, char* name, int flags, int x, int y, int priority);
ScreenObj* load_2d_pfxobj_with_texture(int oid, RwTexture* texture, int flags, int priority);

void delete_screen_obj_oid(int oid);
int vdestroy_screen_obj(ScreenObj* obj);
int destroy_screen_obj(ScreenObj* obj);
void pull_screen_obj(ScreenObj* obj);
ScreenObj* insert_2d_obj(ScreenObj* obj);
ScreenObj* insert_string_obj(ScreenObj* obj);
ScreenObj* insert_screen_obj(ScreenObj* obj);
void unhide_screen_obj(ScreenObj* obj);
void hide_screen_obj(ScreenObj* obj);
void init_2d_obj_lists(void);

#endif
