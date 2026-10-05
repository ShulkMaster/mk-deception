#ifndef IMAGE_H
#define IMAGE_H

#include "runtime/mk_struct.h"
#include "libmkparticle/pfx2d.h"
#include "rw/rpworld_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern MkVtableAniTextureControl vtbl_ani_texture_control;
extern struct ScreenObjVtable vtbl_mkpdata_screen_obj;

typedef struct AniTextureControl AniTextureControl;
typedef struct AniTextureControlItem AniTextureControlItem;
typedef struct ScreenObj ScreenObj;
typedef struct ImageClumpExt ImageClumpExt;
typedef struct ImageMkSobj ImageMkSobj;
typedef struct StringObj StringObj;

typedef struct ScreenObjVtable {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(ScreenObj* object);
} ScreenObjVtable;

typedef struct ScreenObjFlags {
    unsigned char pad0 : 2;
    unsigned char bit5 : 1;
    unsigned char hidden : 1;
    unsigned char scaled : 1;
    unsigned char bit2 : 1;
    unsigned char bit1 : 1;
    unsigned char bit0 : 1;
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

typedef struct AtcMaterialIdBits {
    unsigned short : 5;
    unsigned short material_id : 8;
    unsigned short : 3;
} AtcMaterialIdBits;

typedef struct AtcAlphaFlag {
    unsigned char pad0 : 1;
    unsigned char alpha_frames : 1;
    unsigned char pad1 : 6;
} AtcAlphaFlag;

struct AniTextureControlItem {
    AniTextureControl* atc;
    int instance;
};

struct AniTextureControl {
    MkVtable5* vtbl;
    unsigned int instance;
    int frame;
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
            union {
                unsigned short flags;
                AtcMaterialIdBits material_id_bits;
            };
            unsigned short flags_hi_word;
        };
    };
    float frame_f;
    int numframes;
    float framerate;
    const char* name;
    RpMaterial* materials[3];
    RpAtomic* atomic;
    ScreenObj* screen_obj;
    int screen_obj_instance;
    RwTexture* textures[44];
    RwTexture* alpha_textures[44];
};

struct ImageClumpExt {
    char pad00[0x28];
    MkPtr* atc_list;
};

struct ImageMkSobj {
    MkHdr hdr;
    char pad08[0x0C];
    RpAtomic* atomic;
    void* frame;
    ImageClumpExt* clump_ext;
};

struct ScreenObj {
    union {
        MkVtable5* vtbl;
        ScreenObjVtable* typed_vtbl;
    };
    unsigned int instance;
    int oid;
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
    };
    RwRaster* texture;
    int x;
    int y;
    int priority;
    int field_0x20;
    int field_0x24;
    float scale_x;
    float scale_y;
    unsigned int blend;
    Pfx2dObj* pfx2d;
};

extern int suppress_normal_2d_items;
extern MkPtr* ani_texture_control_list;
extern MkPtr* screen_obj_list;

void toggle_normal_2d_rendering(int enable);
void insert_ani_texture_control_item(AniTextureControl* atc, AniTextureControlItem* item);
AniTextureControl* ck_ani_texture_control_item(AniTextureControlItem* item);

AniTextureControl* append_texture_by_name_to_atomic_material_id(
    int slot, char* name, RpAtomic* atomic, int material_id, int flag);
AniTextureControl* attach_named_wiff_to_first_material(int slot, char* name, ImageMkSobj* mkobj);
AniTextureControl* attach_wiff_to_atomic_material(
    int slot, unsigned int art_oid, RpAtomic* atomic, char* tex_name);
AniTextureControl* append_wiff_to_clump_material(int slot, unsigned int art_oid, RpClump* clump, const char* tex_name);
AniTextureControl* append_wiff_to_clump_material_id(int slot, char* name, RpClump* clump, unsigned short material_id);

RpAtomic* AtomicFindAniTexture(RpAtomic* atomic, void* data);
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
void vdestroy_ani_texture_control(AniTextureControl* atc);
int destroy_ani_texture_control(AniTextureControl* atc);

void render_2d_objs(int layer);
ScreenObj* load_named_2d_pfxobj(int slot, int oid, const char* name, int flags, int priority);
ScreenObj* load_2d_pfxobj(int slot, int oid, unsigned int art_oid, int flags, int priority);
ScreenObj* load_named_2d_pfxobj_xy(int slot, int oid, const char* name, int flags, int x, int y,
                                    int priority);
ScreenObj* load_2d_pfxobj_xy(int slot, int oid, unsigned int art_oid, int flags, int x, int y, int priority);
ScreenObj* load_2d_pfxobj_with_texture(int oid, RwTexture* texture, int flags, int priority);

void delete_screen_obj_oid(int oid);
void vdestroy_screen_obj(ScreenObj* obj);
int destroy_screen_obj(ScreenObj* obj);
void pull_screen_obj(ScreenObj* obj);
ScreenObj* insert_2d_obj(ScreenObj* obj);
ScreenObj* insert_string_obj(ScreenObj* obj);
ScreenObj* insert_screen_obj(ScreenObj* obj);
void unhide_screen_obj(ScreenObj* obj);
void hide_screen_obj(ScreenObj* obj);
void init_2d_obj_lists(void);

#ifdef __cplusplus
}
#endif

#endif
