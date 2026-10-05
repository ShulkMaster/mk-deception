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
struct BloodParticleDefinition;

typedef struct PfxNameObj {
    char pad00[0x354];
    float scale;
} PfxNameObj;

struct PfxSlotFlagBits {
    signed char owns_bind : 1;
    unsigned char flags_rest : 7;
};

typedef struct PfxSlot {
    MkHdr* hdr;
    unsigned int instance;
    union {
        unsigned char flags;
        struct PfxSlotFlagBits flag_bits;
    };
    unsigned char pad09[3];
} PfxSlot;

typedef PfxVmEmitter PfxEmitter;

typedef struct MkPfx MkPfx;

struct PfxCloneFlagBits {
    unsigned char destroyed : 1;
    unsigned char owns_bind : 1;
    unsigned char owns_bind2 : 1;
    unsigned char flags_rest : 5;
};

typedef struct PfxClone {
    MkHdr hdr;
    union {
        unsigned int flags_word;
        unsigned char flags;
        struct PfxCloneFlagBits flag_bits;
    };
    MkPfx* parent;
    void* matrix_copy;
    MkHdr* bind_hdr;
    unsigned int bind_inst;
    MkHdr* bind2_hdr;
    unsigned int bind2_inst;
    float depth_bias;
    int priority;
} PfxClone;

struct MkPfxFlagBits {
    unsigned char destroyed : 1;
    unsigned char owns_bind : 1;
    unsigned char flags_bit5 : 1;
    unsigned char visible : 1;
    unsigned char skip_translucent_sort : 1;
    unsigned char flags_low : 3;
};

struct MkPfxRenderFlagBits {
    unsigned char hide : 1;
    unsigned char field_6 : 1;
    unsigned char camera_facing : 1;
    unsigned char flags_rest : 5;
};

struct MkPfx {
    MkHdr hdr;
    union {
        unsigned int flags_word;
        unsigned char flags;
        struct MkPfxFlagBits flag_bits;
    };
    MkHdr* proc;
    unsigned int proc_inst;
    void* bone_mat;
    MkHdr* bind_hdr;
    unsigned int bind_inst;
    PfxSlot* slot_table;
    MkObj* bound_obj;
    float depth_bias;
    int priority;
    int tick;
    float accum_34;
    float accum_38;
    void* mem;
    float matrix[16];
    union {
        unsigned char flags80;
        struct MkPfxRenderFlagBits f80;
    };
    char pad81[0x0F];
    int field_90;
    int field_94;
    int active_slot;
    char pad9C[4];
    int field_A0;
    char padA4[0x0C];
    PfxTransform transforms[3];
    char pad188[0x1C];
    Vec camera_follow_position;
    char pad1B0[0x12];
    unsigned short emitter_enabled;
    char pad1C4[0x3C];
    int slot_count;
    PfxEmitter* emitter_scratch;
    char pad208[4];
    int behaviors_active;
    char pad210[4];
    int field_214;
    char pad218[0x3C];
    PfxTransformCb transform_cb;
    char pad258[4];
    char* name_dst;
    char pad260[4];
    PfxMetrics* metrics_handle;
    float scale;
    char pad26C[0x14];
    MkObj* tracked_object;
    unsigned int tracked_object_instance;
    union {
        int field_288;
        int effect_state;
    };

    union {
        int field_28C;
        struct BloodParticleDefinition* blood_definition;
    };
    int field_290;
    int field_294;
    float field_298;
    float field_29C;
    float field_2A0;
    float field_2A4;
    float field_2A8;
    Vec effect_center;
    union {
        int field_2B8;
        FighterMirror* decal_owner;
        PfxColor* glass_alphas;
    };
    int field_2BC;
};

typedef char PfxNameObjPrefixSizeCheck[
    sizeof(PfxNameObj) == 0x358 ? 1 : -1];
typedef char PfxSlotSizeCheck[sizeof(PfxSlot) == 0x0C ? 1 : -1];
typedef char PfxCloneSizeCheck[sizeof(PfxClone) == 0x2C ? 1 : -1];
#ifndef __cplusplus
typedef char MkPfxSizeCheck[sizeof(MkPfx) == 0x2C0 ? 1 : -1];
#endif

void mkpfx_get_origin(MkPfx* pfx, float origin[3]);
void mkpfx_camera_end(void);
void mkpfx_camera_begin(void);
void mkpfx_set_environment(void);
MkHdr* pfx_get_emitter_obj(MkPfx* pfx, int index);
void vdestroy_pfx_clone(PfxClone* clone);
void vdestroy_pfx(MkPfx* pfx);
void render_pfx_clone(PfxClone* clone);
void render_pfx(MkPfx* pfx);
void hide_pfx(MkPfx* pfx, int hide);
void pfx_end_batch(void);
void pfx_start_batch(void);
void insert_PFXlist_in_transl_tree(void);
void set_pfx_texture(PfxVm* vm, int handle, unsigned int art_oid);
MkObj* pfx_clone_bind_render_to_new_obj(PfxClone* clone, int object_type);
void pfx_bind_emitter_num_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone, int emitter);
void pfx_bind_emitter_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone);
void pfx_bind_render_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone);
void pfx_bind_emitter_num_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag, int emitter);
void pfx_bind_emitter_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag);
void pfx_bind_render_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag);
MkObj* pfx_bind_to_new_obj(MkPfx* pfx, int object_type);
MkObj* pfx_bind_emitter_num_to_new_obj(MkPfx* pfx, int object_type, int emitter);
void pfx_bind_emitter_to_obj(MkPfx* pfx, MkObj* obj, int flag);
void pfx_bind_emitter_num_to_obj(MkPfx* pfx, MkObj* obj, int flag, int emitter);
void pfx_bind_render_to_obj(MkPfx* pfx, MkObj* obj, int flag);
PfxClone* pfx_create_clone(MkPfx* pfx);
MkPfx* pfx_from_handle(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
MkPfx* find_pfx_by_handle(unsigned int handle);
MkPfx* find_pfx_by_name_by_bankowner(const char* name, unsigned int owner);

void* pfx_create_raw_userdata(int extra_size, int userdata_size, int field_90,
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
