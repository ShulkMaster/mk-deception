#ifndef MK_PARTICLE_H
#define MK_PARTICLE_H

#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "libmkparticle/pfx_memory.h"
#include "libmkparticle/vm.h"

typedef void (*PfxInitCb)(void* vm);
typedef void (*PfxTransformCb)(void);
typedef struct FighterMirror FighterMirror;
typedef struct PfxColor PfxColor;
typedef struct PfxMetrics PfxMetrics;

/* PFX VM name object - scale written during create @ +0x354. */
typedef struct PfxNameObj {
    char pad00[0x354];
    float scale; /* +0x354 */
} PfxNameObj;

/* Emitter/slot bind entry -- stride 0xC in MkPfx.slot_table. */
typedef struct PfxSlot {
    MkHdr* hdr;            /* +0x00 */
    unsigned int instance; /* +0x04 */
    unsigned char flags;   /* +0x08 -- bit7 = owns / destroy on teardown */
    unsigned char pad09[3];
} PfxSlot;

/* Per-slot transform matrix block -- stride 0x48 from MkPfx.mats. */
typedef struct PfxSlotMat {
    float m[16];      /* +0x00 */
    char pad40[4];
    int particle_stride; /* +0x44 */
} PfxSlotMat;

/* Emitter VM blob (stack/scratch size 0x2EC); transform @ +0x2E8. */
typedef struct PfxEmitter {
    Vec position; /* +0x00 */
    float lifetime; /* +0x0C */
    char pad10[0x30];
    int field_40;
    char pad44[0x2A4];
    void* transform; /* +0x2E8 -- bone mat / LTM */
} PfxEmitter;

typedef struct MkPfx MkPfx;

/* PFX clone -- size 0x2C (get_mkhdr). */
typedef struct PfxClone {
    MkHdr hdr;              /* +0x00 */
    unsigned char flags;    /* +0x08 -- bit7 destroyed, bit6 owns bind, bit5 owns bind2 */
    unsigned char pad09[3];
    MkPfx* parent;          /* +0x0C */
    void* matrix_copy;      /* +0x10 */
    MkHdr* bind_hdr;        /* +0x14 */
    unsigned int bind_inst; /* +0x18 */
    MkHdr* bind2_hdr;       /* +0x1C */
    unsigned int bind2_inst;/* +0x20 */
    float field_24;         /* +0x24 */
    int field_28;           /* +0x28 = 0x12 */
} PfxClone;

/*
 * Main particle object -- base size 0x2C0 (+ extra_size from create).
 * Embeds MkHdr @ +0x00; PFX VM / camera matrix starts @ +0x40.
 */
struct MkPfx {
    MkHdr hdr;                    /* +0x00 */
    union {
        unsigned char flags;
        struct {
            unsigned char destroyed : 1;
            unsigned char owns_bind : 1;
            unsigned char flags_bit5 : 1;
            unsigned char visible : 1;
            unsigned char flags_low : 4;
        } flag_bits;
    };                            /* +0x08 */
    unsigned char pad09[3];
    MkHdr* proc;                  /* +0x0C */
    unsigned int proc_inst;       /* +0x10 */
    void* bone_mat;               /* +0x14 */
    MkHdr* bind_hdr;              /* +0x18 */
    unsigned int bind_inst;       /* +0x1C */
    PfxSlot* slot_table;          /* +0x20 */
    MkObj* bound_obj;             /* +0x24 */
    float field_28;               /* +0x28 */
    int field_2C;                 /* +0x2C = 0x12 */
    int tick;                     /* +0x30 */
    float accum_34;               /* +0x34 */
    float accum_38;               /* +0x38 */
    void* mem;                    /* +0x3C */
    float matrix[16];             /* +0x40 -- also PFX VM base */
    unsigned char flags80;        /* +0x80 -- bit5 camera-facing, bit7 hide */
    char pad81[0x0F];
    int field_90;                 /* +0x90 */
    int field_94;                 /* +0x94 -- emitter/active gate (mk_render InsertPFX*) */
    int active_slot;              /* +0x98 */
    char pad9C[4];
    int field_A0;                 /* +0xA0 */
    char padA4[0x0C];
    PfxSlotMat mats[1];           /* +0xB0 -- indexed; stride 0x48 */
    char padF8[0xCA];
    unsigned short emitter_enabled; /* +0x1C2 */
    char pad1C4[0x3C];
    int slot_count;               /* +0x200 */
    PfxEmitter* emitter_scratch;  /* +0x204 */
    char pad208[4];
    int behaviors_active;         /* +0x20C */
    char pad210[4];
    int field_214;                /* +0x214 */
    char pad218[0x3C];
    PfxTransformCb transform_cb;  /* +0x254 */
    char pad258[4];
    char* name_dst;               /* +0x25C -- set from VM during create */
    char pad260[4];
    PfxMetrics* metrics_handle;   /* +0x264 */
    float scale;                  /* +0x268 */
    char pad26C[0x14];
    union {
        int field_280;
        MkObj* tracked_object;
    };                            /* +0x280 */
    union {
        int field_284;
        unsigned int tracked_object_instance;
    };
    union {
        int field_288;
        int effect_state;
    };
    int field_28C;
    int field_290;
    int field_294;
    float field_298;
    float field_29C;
    float field_2A0;
    float field_2A4;
    float field_2A8;
    union {
        char pad2AC[0x0C];
        Vec glass_center;
    };
    union {
        int field_2B8;
        FighterMirror* decal_owner;
        PfxColor* glass_alphas;
    };                            /* +0x2B8 */
    int field_2BC;                /* +0x2BC -- end of 0x2C0 base */
};

/* RwFrame modelling matrix @ +0x10 (stock RW: object + dirty link). */
typedef struct RwFrameModelling {
    char pad00[0x10];
    float modelling[16]; /* +0x10 */
} RwFrameModelling;

void mkpfx_get_origin(MkPfx* pfx, float* origin);
void mkpfx_camera_end(void);
void mkpfx_camera_begin(void);
void mkpfx_set_environment(void);
MkHdr* pfx_get_emitter_obj(MkPfx* pfx, int index);
int vdestroy_pfx_clone(PfxClone* clone);
int vdestroy_pfx(MkPfx* pfx);
void render_pfx_clone(PfxClone* clone);
void render_pfx(MkPfx* pfx);
void hide_pfx(MkPfx* pfx, int hide);
void pfx_end_batch(void);
void pfx_start_batch(void);
void insert_PFXlist_in_transl_tree(void);
void set_pfx_texture(PfxVm* vm, void* path, void* name);
MkObj* pfx_clone_bind_render_to_new_obj(PfxClone* clone, void* frame_src);
void pfx_bind_emitter_num_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone, int emitter);
void pfx_bind_emitter_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone);
void pfx_bind_render_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone);
void pfx_bind_emitter_num_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag, int emitter);
void pfx_bind_emitter_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag);
void pfx_bind_render_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag);
MkObj* pfx_bind_to_new_obj(MkPfx* pfx, void* frame_src);
MkObj* pfx_bind_emitter_num_to_new_obj(MkPfx* pfx, void* frame_src, int emitter);
void pfx_bind_emitter_to_obj(MkPfx* pfx, MkObj* obj, int flag);
void pfx_bind_emitter_num_to_obj(MkPfx* pfx, MkObj* obj, int flag, int emitter);
void pfx_bind_render_to_obj(MkPfx* pfx, MkObj* obj, int flag);
PfxClone* pfx_create_clone(MkPfx* pfx);

/*
 * Thin wrapper: stamps empty_build_info (flag=1, userdata) then calls
 * new_pfx_create_raw_userdata. Used by krypt tombstone letter/number/koin pfx.
 */
void* pfx_create_raw_userdata(int extra_size, void* userdata, int field_90,
                              int field_214, int field_a0, PfxInitCb init_cb,
                              int pid, MkProcEntryFn entry, void** out_pfx);

void* new_pfx_create_raw_userdata(PfxBuildInfo* build, int extra_size, int field_90,
                                  int field_214, int field_a0, PfxInitCb init_cb,
                                  int pid, MkProcEntryFn entry, void** out_pfx);

void pfx_post_sleep(void);
void pfx_pre_wake(void);
int mkpfx_init(void);

extern MkPfx* apfx;
extern MkObj* apfx_render_obj;
extern MkObj* apfx_emitter_obj;
extern MkSobj* apfx_render_sobj;
extern MkSobj* apfx_emitter_sobj;
extern MkPtr* pfx_render_list;
extern MkPtr* pfx_clone_render_list;

#endif
