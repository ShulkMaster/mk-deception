#include "runtime/mk_particle.h"

#include "game/game_info.h"
#include "runtime/mk_mem.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwframe.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

/* ---- externals (other TUs / SDK) ---- */

extern MkVtable5 vtbl_mkobj;
extern MkVtable5 vtbl_pfx;
extern MkVtable5 vtbl_pfx_clone;

extern void* Camera;
extern void* RwEngineInstance;
extern float camera_facing_matrix_ay[];
extern float game_speed;
extern int exec_tick_ctr;
extern MkHdr* apdata;
extern MkProc* aproc;

void memcpy(void* dst, const void* src, int size);
void memset(void* dst, int c, int size);
char* strcpy(char* dst, const char* src);
void* load_tga(void* path, void* name);
void* get_mkobj_frame(void* src, int unused);
void insert_particle_mkobj(void* obj);
void calc_bone_world_mat(void* obj, int bone);
void obj_set_bone_calc_world_mat_flag(void* obj, int bone);
void mkproc_die(void);

void pfx_count_end(void);
void pfx_count_begin(void);
void pfx_count_add(void* vm);
void pfxsystem_set_frame_info(int a, int b, void* matrix, void* camera);
void pfxsystem_set_global(int id, float value);
void pfxvm_init(void* vm);
void pfx_emitter_scan_for_fields(void* emitter, unsigned int* out_pair);
int pfx_native_is_supported_type(int type);
void pfx_estimate_size(void* vm, PfxEstimate* out_est, PfxBuildInfo* build);
void pfx_set_memory(void* vm, void* mem, PfxEstimate* est);
int pfx_frame_begin(void* vm);
void pfx_frame_end(void* vm);
void pfx_frame_end_check(void* vm);
void pfx_behaviors_frame_begin(void* vm);
void pfx_behaviors_frame_end(void* vm);
void* pfx_get_emitter(void* vm, int index);
void pfx_set_texture(void* pfx, void* texture);
void pfx_set_renderstate(void* vm);
void particle_render(void* vm);
void pfx_reset_renderstate(void* vm);
void pfxmetrics_event(void* handle, int event);
void pfxmetrics_set_interface(void* iface);
/* Retail usec timers return elapsed u64 in r3:r4. */
unsigned long long stop_usec_timer(int id);
void start_usec_timer(int id);

void InsertPFXInTranslTree(MkHdr* hdr);
void InsertPFXCloneInTranslTree(MkHdr* hdr);

/* ---- file-local / BSS ---- */

static float* inverse_camera_matrix;
static void* old_ltm_415;

/* Retail mk_particle.o .sbss2 @499 -- zero-init pair for pfx_emitter_scan_for_fields. */
static unsigned int pfx_emitter_field_pair_seed[2];

typedef struct PfxNostackFlagBits {
    unsigned char pad[3];
    unsigned char nostack : 1;
    unsigned char pad7 : 7;
} PfxNostackFlagBits;

MkPfx* apfx;
MkObj* apfx_render_obj;
MkObj* apfx_emitter_obj;
MkSobj* apfx_render_sobj;
MkSobj* apfx_emitter_sobj;
MkPtr* pfx_render_list;
MkPtr* pfx_clone_render_list;

static void apfx_set_transform_matrix(void);
static unsigned int pfxmetrics_stop_timer(int id);
static void pfxmetrics_start_timer(int id);

static const float kZero = 0.0f;
static const float kOne = 1.0f;
static const float kDefaultPfxScale = -10000.0f; /* retail @1400 */

#define MKPFX_MKOBJ_FROM_HDR(hdr_) ((MkObj*)(hdr_))
#define MKPFX_VTBL_GET_SOBJ(vtbl_, hdr_)                                  \
    (((MkSobj* (*)(MkHdr*))(vtbl_)->fn2)(hdr_))

/* Resolve MkHdr* against a stored instance id (retail inline pattern). */
static MkHdr* resolve_mkptr(MkHdr* hdr, unsigned int expected_instance) {
    if (hdr == 0) {
        return 0;
    }
    if (hdr->instance != expected_instance) {
        return 0;
    }
    return hdr;
}

static MkObj* as_mkobj(MkHdr* hdr) {
    if (hdr == 0) {
        return 0;
    }
    if (hdr->vtbl != &vtbl_mkobj) {
        return 0;
    }
    return (MkObj*)hdr;
}

static MkSobj* vtbl_call_get_sobj(MkHdr* hdr) {
    MkVtable5* vtbl;

    if (hdr == 0) {
        return 0;
    }
    vtbl = hdr->vtbl;
    return ((MkSobj* (*)(MkHdr*))vtbl->fn2)(hdr);
}

static void vtbl_call_destroy(MkHdr* hdr) {
    MkVtable5* vtbl;

    if (hdr == 0 || hdr->instance == 0) {
        return;
    }
    vtbl = hdr->vtbl;
    ((void (*)(MkHdr*))vtbl->destroy)(hdr);
}

static int flag_msb(unsigned char byte) {
    return (int)((signed char)(byte & 0x80));
}

static PfxVm* pfx_vm(MkPfx* pfx) {
    return (PfxVm*)pfx->matrix;
}

/* ======================================================================== */
/* Retail function order                                                     */
/* ======================================================================== */

void mkpfx_get_origin(MkPfx* pfx, float* origin) {
    int slot;
    PfxSlot* slot_base;
    MkHdr* bound;
    MkObj* mkobj;
    MkSobj* sobj;
    MkVtable5* vtbl;
    float* ltm;
    float* mat;

    slot = pfx->active_slot;
    slot_base = pfx->slot_table;
    mat = pfx->mats[slot].m;
    bound = slot_base->hdr;
    if (bound != 0) {
        if (bound->instance != slot_base->instance) {
            bound = 0;
        }
    } else {
        bound = 0;
    }

    if (bound == 0) {
        origin[0] = mat[0xC];
        origin[1] = mat[0xD];
        origin[2] = mat[0xE];
        return;
    }

    vtbl = bound->vtbl;
    if (vtbl == &vtbl_mkobj) {
        mkobj = MKPFX_MKOBJ_FROM_HDR(bound);
    } else {
        mkobj = 0;
    }
    if (bound != 0) {
        sobj = MKPFX_VTBL_GET_SOBJ(vtbl, bound);
    } else {
        sobj = 0;
    }
    if (mkobj != 0) {
        ltm = (float*)RwFrameGetLTM(mkobj->frame);
        origin[0] = ltm[0xC];
        origin[1] = ltm[0xD];
        origin[2] = ltm[0xE];
        return;
    }
    if (sobj != 0) {
        ltm = (float*)RwFrameGetLTM(sobj->frame);
        origin[0] = ltm[0xC];
        origin[1] = ltm[0xD];
        origin[2] = ltm[0xE];
    }
}

void mkpfx_camera_end(void) {
    pfx_count_end();
}

void mkpfx_camera_begin(void) {
    pfxsystem_set_frame_info(0, 0, camera_facing_matrix_ay, Camera);
    pfx_count_begin();
}

void mkpfx_set_environment(void) {
    pfxsystem_set_global(0x500, g_game_info.field_34);
}

MkHdr* pfx_get_emitter_obj(MkPfx* pfx, int index) {
    PfxSlot* table;
    int count;

    if (pfx == 0) {
        return 0;
    }
    count = pfx->slot_count;
    if (index >= count || index < 0) {
        return 0;
    }
    table = pfx->slot_table;
    return resolve_mkptr(table[index].hdr, table[index].instance);
}

int vdestroy_pfx_clone(PfxClone* clone) {
    unsigned char flags;
    MkHdr* hdr;

    flags = clone->flags;
    if ((flags >> 7) & 1) {
        return 0;
    }

    clone->hdr.instance = 0;
    flags = clone->flags;
    clone->flags = (unsigned char)((flags & 0x7F) | 0x80);

    if ((clone->flags >> 6) & 1) {
        hdr = resolve_mkptr(clone->bind_hdr, clone->bind_inst);
        vtbl_call_destroy(hdr);
    }
    if ((clone->flags >> 5) & 1) {
        hdr = resolve_mkptr(clone->bind2_hdr, clone->bind2_inst);
        vtbl_call_destroy(hdr);
    }

    clone->hdr.instance = 0;
    mkhdr_memfree(&clone->hdr);
    return 0;
}

int vdestroy_pfx(MkPfx* pfx) {
    unsigned char flags;
    MkHdr* hdr;
    int i;
    int count;
    PfxSlot* table;
    PfxSlot* slot;

    flags = pfx->flags;
    if ((flags >> 7) & 1) {
        return 0;
    }

    pfx->hdr.instance = 0;
    flags = pfx->flags;
    pfx->flags = (unsigned char)((flags & 0x7F) | 0x80);

    if ((pfx->flags >> 6) & 1) {
        hdr = resolve_mkptr(pfx->bind_hdr, pfx->bind_inst);
        vtbl_call_destroy(hdr);
    }

    table = pfx->slot_table;
    if (table != 0) {
        count = pfx->slot_count;
        for (i = 0; i < count; i++) {
            slot = &table[i];
            if (flag_msb(slot->flags) < 0) {
                hdr = resolve_mkptr(slot->hdr, slot->instance);
                vtbl_call_destroy(hdr);
            }
        }
    }

    hdr = (MkHdr*)pfx->mem;
    if (hdr != 0) {
        free_mem_delayed(hdr, 3);
    }

    pfx->hdr.instance = 0;
    mkhdr_memfree(&pfx->hdr);
    return 0;
}

void render_pfx_clone(PfxClone* clone) {
    MkPfx* pfx;
    float* mat;
    float save[16];
    float* inv;
    float zero;
    float one;

    if (clone->matrix_copy == 0) {
        return;
    }

    pfx = clone->parent;
    mat = pfx->mats[pfx->active_slot].m;
    memcpy(save, mat, 0x40);
    memcpy(mat, clone->matrix_copy, 0x40);

    zero = kZero;
    one = kOne;
    mat[3] = zero;
    mat[7] = zero;
    mat[11] = zero;
    mat[15] = one;

    if (((pfx->flags80 >> 5) & 1) != 0) {
        inv = inverse_camera_matrix;
        pfx->matrix[0] = inv[0];
        pfx->matrix[1] = inv[1];
        pfx->matrix[2] = inv[2];
        pfx->matrix[3] = zero;
        pfx->matrix[4] = inv[4];
        pfx->matrix[5] = inv[5];
        pfx->matrix[6] = inv[6];
        pfx->matrix[7] = zero;
        pfx->matrix[8] = inv[8];
        pfx->matrix[9] = inv[9];
        pfx->matrix[10] = inv[10];
        pfx->matrix[11] = zero;
        pfx->matrix[12] = inv[12];
        pfx->matrix[13] = inv[13];
        pfx->matrix[14] = inv[14];
        pfx->matrix[15] = one;
        pfx_count_add(pfx_vm(pfx));
        pfx_set_renderstate(pfx_vm(pfx));
        particle_render(pfx_vm(pfx));
        pfx_reset_renderstate(pfx_vm(pfx));
        pfxmetrics_event(pfx->metrics_handle, 0x4005);
    }

    memcpy(mat, save, 0x40);
}

void render_pfx(MkPfx* pfx) {
    MkHdr* bound;
    MkObj* mkobj;
    MkHdr* parent;
    unsigned char hide_byte;
    float* inv;
    float zero;
    float one;
    PfxTransformCb cb;

    bound = resolve_mkptr(pfx->slot_table->hdr, pfx->slot_table->instance);
    if (bound != 0) {
        mkobj = as_mkobj(bound);
        if (mkobj != 0) {
            if (((mkobj->flags_0C >> 1) & 1) == 0) {
                hide_byte = mkobj->hide_flags;
            } else {
                parent = resolve_mkptr(mkobj->parent_hdr, mkobj->parent_inst);
                if (parent == 0) {
                    return;
                }
                hide_byte = ((MkObj*)parent)->hide_flags;
            }
            if ((hide_byte >> 5) & 1) {
                return;
            }
        }
    }

    if (((pfx->flags >> 4) & 1) == 0) {
        return;
    }

    cb = pfx->transform_cb;
    if (cb != 0) {
        apfx = pfx;
        cb();
        apfx = 0;
    }

    zero = kZero;
    one = kOne;
    if (((pfx->flags80 >> 5) & 1) != 0) {
        inv = inverse_camera_matrix;
        pfx->matrix[0] = inv[0];
        pfx->matrix[1] = inv[1];
        pfx->matrix[2] = inv[2];
        pfx->matrix[3] = zero;
        pfx->matrix[4] = inv[4];
        pfx->matrix[5] = inv[5];
        pfx->matrix[6] = inv[6];
        pfx->matrix[7] = zero;
        pfx->matrix[8] = inv[8];
        pfx->matrix[9] = inv[9];
        pfx->matrix[10] = inv[10];
        pfx->matrix[11] = zero;
        pfx->matrix[12] = inv[12];
        pfx->matrix[13] = inv[13];
        pfx->matrix[14] = inv[14];
        pfx->matrix[15] = one;
        pfx_count_add(pfx_vm(pfx));
        pfx_set_renderstate(pfx_vm(pfx));
        particle_render(pfx_vm(pfx));
        pfx_reset_renderstate(pfx_vm(pfx));
        pfxmetrics_event(pfx->metrics_handle, 0x4005);
    }
}

void hide_pfx(MkPfx* pfx, int hide) {
    unsigned char flags;

    flags = pfx->flags80;
    pfx->flags80 = (unsigned char)((flags & 0x7F) | ((hide & 1) << 7));
}

void pfx_end_batch(void) {
}

void pfx_start_batch(void) {
}

void insert_PFXlist_in_transl_tree(void) {
    RwCamera* camera;
    void* frame;

    camera = *(RwCamera**)RwEngineInstance;
    if (camera == 0) {
        return;
    }
    frame = camera->object.object.parent;
    inverse_camera_matrix = (float*)RwFrameGetLTM(frame);
    apply_to_mklist(InsertPFXInTranslTree, &pfx_render_list);
    apply_to_mklist(InsertPFXCloneInTranslTree, &pfx_clone_render_list);
}

void set_pfx_texture(PfxVm* pfx, void* path, void* name) {
    void* tex;

    tex = load_tga(path, name);
    if (tex != 0) {
        pfx_set_texture(pfx, tex);
    }
}

MkObj* pfx_clone_bind_render_to_new_obj(PfxClone* clone, void* frame_src) {
    MkObj* obj;
    unsigned char flags;

    obj = (MkObj*)get_mkobj_frame(frame_src, 0);
    if (obj != 0) {
        flags = clone->flags;
        clone->flags = (unsigned char)((flags & 0xBF) | 0x40);
        clone->bind_hdr = &obj->hdr;
        clone->bind_inst = obj->hdr.instance;
        clone->matrix_copy = ((RwFrameModelling*)obj->frame)->modelling;
        insert_particle_mkobj(obj);
    }
    return obj;
}

void pfx_bind_emitter_num_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone, int emitter) {
    PfxSlot* slot;
    PfxEmitter* emitter_vm;
    void* bone_mat;

    slot = &pfx->slot_table[emitter];
    slot->flags = (unsigned char)(slot->flags & 0x7F);
    slot->hdr = &obj->hdr;
    slot->instance = obj->hdr.instance;

        bone_mat = obj->bones[bone];
    if (bone_mat == 0) {
        bone_mat = obj->field_24;
        emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), emitter);
        emitter_vm->transform = bone_mat;
        return;
    }

    emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), emitter);
    emitter_vm->transform = bone_mat;
    calc_bone_world_mat(obj, bone);
    obj_set_bone_calc_world_mat_flag(obj, bone);
}

void pfx_bind_emitter_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone) {
    PfxSlot* slot;
    PfxEmitter* emitter_vm;
    void* bone_mat;

    slot = pfx->slot_table;
    slot->flags = (unsigned char)(slot->flags & 0x7F);
    slot->hdr = &obj->hdr;
    slot->instance = obj->hdr.instance;

        bone_mat = obj->bones[bone];
    if (bone_mat == 0) {
        bone_mat = obj->field_24;
        emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), 0);
        emitter_vm->transform = bone_mat;
        return;
    }

    emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), 0);
    emitter_vm->transform = bone_mat;
    calc_bone_world_mat(obj, bone);
    obj_set_bone_calc_world_mat_flag(obj, bone);
}

void pfx_bind_render_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone) {
    void* bone_mat;
    unsigned char flags;

    flags = pfx->flags;
    pfx->flags = (unsigned char)(flags & 0xBF);
    pfx->bind_hdr = &obj->hdr;
    pfx->bind_inst = obj->hdr.instance;
    pfx->transform_cb = apfx_set_transform_matrix;

        bone_mat = obj->bones[bone];
    if (bone_mat == 0) {
        pfx->bone_mat = obj->field_24;
        return;
    }
    pfx->bone_mat = bone_mat;
    calc_bone_world_mat(obj, bone);
    obj_set_bone_calc_world_mat_flag(obj, bone);
}

void pfx_bind_emitter_num_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag, int emitter) {
    PfxSlot* slot;
    void* ltm;
    PfxEmitter* emitter_vm;
    signed char f;

    f = (signed char)flag;
    slot = &pfx->slot_table[emitter];
    slot->flags = (unsigned char)((slot->flags & 0x7F) | ((f & 1) << 7));
    slot->hdr = &sobj->hdr;
    slot->instance = sobj->hdr.instance;
    ltm = RwFrameGetLTM(sobj->frame);
    emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), emitter);
    emitter_vm->transform = ltm;
}

void pfx_bind_emitter_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag) {
    PfxSlot* slot;
    void* ltm;
    PfxEmitter* emitter_vm;
    signed char f;

    f = (signed char)flag;
    slot = pfx->slot_table;
    slot->flags = (unsigned char)((slot->flags & 0x7F) | ((f & 1) << 7));
    slot->hdr = &sobj->hdr;
    slot->instance = sobj->hdr.instance;
    ltm = RwFrameGetLTM(sobj->frame);
    emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), 0);
    emitter_vm->transform = ltm;
}

void pfx_bind_render_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag) {
    unsigned char flags;
    unsigned int bit;

    bit = (unsigned int)flag & 0xFF;
    flags = pfx->flags;
    pfx->flags = (unsigned char)((flags & 0xBF) | ((bit & 1) << 6));
    pfx->bind_hdr = &sobj->hdr;
    pfx->bind_inst = sobj->hdr.instance;
    pfx->transform_cb = apfx_set_transform_matrix;
    pfx->bone_mat = RwFrameGetLTM(sobj->frame);
}

MkObj* pfx_bind_to_new_obj(MkPfx* pfx, void* frame_src) {
    PfxSlot* slot;
    MkHdr* existing;
    MkObj* obj;
    PfxEmitter* emitter_vm;
    void* ltm;

    if (pfx == 0 || pfx->slot_count < 1) {
        return 0;
    }

    slot = pfx->slot_table;
    if (flag_msb(slot->flags) < 0) {
        existing = resolve_mkptr(slot->hdr, slot->instance);
        if (existing != 0) {
            return (MkObj*)existing;
        }
    }

    obj = (MkObj*)get_mkobj_frame(frame_src, 0);
    if (obj == 0) {
        return 0;
    }

    if (pfx != 0 && pfx->slot_count > 0) {
        slot = pfx->slot_table;
        slot->flags = (unsigned char)((slot->flags & 0x7F) | 0x80);
        slot->hdr = &obj->hdr;
        slot->instance = obj->hdr.instance;
        ltm = ((RwFrameModelling*)obj->frame)->modelling;
        emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), 0);
        emitter_vm->transform = ltm;
    }
    insert_particle_mkobj(obj);
    return obj;
}

MkObj* pfx_bind_emitter_num_to_new_obj(MkPfx* pfx, void* frame_src, int emitter) {
    PfxSlot* slot;
    MkHdr* existing;
    MkObj* obj;
    PfxEmitter* emitter_vm;
    void* ltm;

    if (pfx == 0 || emitter < 0 || emitter >= pfx->slot_count) {
        return 0;
    }

    slot = &pfx->slot_table[emitter];
    if (flag_msb(slot->flags) < 0) {
        existing = resolve_mkptr(slot->hdr, slot->instance);
        if (existing != 0) {
            return (MkObj*)existing;
        }
    }

    obj = (MkObj*)get_mkobj_frame(frame_src, 0);
    if (obj == 0) {
        return 0;
    }

    if (pfx != 0 && emitter >= 0 && emitter < pfx->slot_count) {
        slot = &pfx->slot_table[emitter];
        slot->flags = (unsigned char)((slot->flags & 0x7F) | 0x80);
        slot->hdr = &obj->hdr;
        slot->instance = obj->hdr.instance;
        ltm = ((RwFrameModelling*)obj->frame)->modelling;
        emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), emitter);
        emitter_vm->transform = ltm;
    }
    insert_particle_mkobj(obj);
    return obj;
}

void pfx_bind_emitter_to_obj(MkPfx* pfx, MkObj* obj, int flag) {
    PfxSlot* slot;
    PfxEmitter* emitter_vm;
    void* ltm;
    signed char f;

    if (pfx == 0 || obj == 0 || pfx->slot_count <= 0) {
        return;
    }
    f = (signed char)flag;
    slot = pfx->slot_table;
    slot->flags = (unsigned char)((slot->flags & 0x7F) | ((f & 1) << 7));
    slot->hdr = &obj->hdr;
    slot->instance = obj->hdr.instance;
    ltm = ((RwFrameModelling*)obj->frame)->modelling;
    emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), 0);
    emitter_vm->transform = ltm;
}

void pfx_bind_emitter_num_to_obj(MkPfx* pfx, MkObj* obj, int flag, int emitter) {
    PfxSlot* slot;
    PfxEmitter* emitter_vm;
    void* ltm;
    signed char f;

    if (pfx == 0 || obj == 0 || emitter < 0 || emitter >= pfx->slot_count) {
        return;
    }
    f = (signed char)flag;
    slot = &pfx->slot_table[emitter];
    slot->flags = (unsigned char)((slot->flags & 0x7F) | ((f & 1) << 7));
    slot->hdr = &obj->hdr;
    slot->instance = obj->hdr.instance;
    ltm = ((RwFrameModelling*)obj->frame)->modelling;
    emitter_vm = (PfxEmitter*)pfx_get_emitter(pfx_vm(pfx), emitter);
    emitter_vm->transform = ltm;
}

void pfx_bind_render_to_obj(MkPfx* pfx, MkObj* obj, int flag) {
    unsigned char flags;

    flags = pfx->flags;
    pfx->flags = (unsigned char)((flags & 0xBF) | ((flag & 1) << 6));
    pfx->bind_hdr = &obj->hdr;
    pfx->bind_inst = obj->hdr.instance;
    pfx->transform_cb = apfx_set_transform_matrix;
    pfx->bone_mat = ((RwFrameModelling*)obj->frame)->modelling;
}

PfxClone* pfx_create_clone(MkPfx* pfx) {
    PfxClone* clone;
    float zero;

    clone = (PfxClone*)get_mkhdr(&vtbl_pfx_clone, 0x2C);
    if (clone == 0) {
        return 0;
    }
    zero = kZero;
    clone->parent = pfx;
    clone->matrix_copy = 0;
    clone->bind_hdr = 0;
    clone->bind_inst = 0;
    clone->bind2_hdr = 0;
    clone->bind2_inst = 0;
    *(unsigned int*)&clone->flags = 0;
    clone->field_24 = zero;
    clone->field_28 = 0x12;
    mk_insert(&clone->hdr, &pfx_clone_render_list);
    return clone;
}

void* pfx_create_raw_userdata(int extra_size, void* userdata, int field_90,
                              int field_214, int field_a0, PfxInitCb init_cb,
                              int pid, MkProcEntryFn entry, void** out_pfx) {
    /* Retail: .data static empty_build_info$522 (zero-init -> .data, not .bss). */
    static PfxBuildInfo empty_build_info = {0};

    empty_build_info.userdata = userdata;
    empty_build_info.flag = 1;
    return new_pfx_create_raw_userdata(&empty_build_info, extra_size, field_90,
                                       field_214, field_a0, init_cb, pid, entry,
                                       out_pfx);
}

void* new_pfx_create_raw_userdata(PfxBuildInfo* build, int extra_size, int field_90,
                                  int field_214, int field_a0, PfxInitCb init_cb,
                                  int pid, MkProcEntryFn entry, void** out_pfx) {
    MkProc* created_proc;
    MkPfx* pfx;
    void* vm;
    int proc_nostack_arg;
    unsigned int proc_nostack_slot;
    unsigned int field_pair[2];
    PfxEstimate est_buf;
    unsigned char emitter_buf[0x2EC];
    int ready;
    int pad_raw;
    int pad_align;
    int pad_extra;
    int est_size;
    int alloc_size;
    void* mem;
    void* aligned;
    PfxNameObj* name_obj;
    char* name_dst;
    PfxEmitter* ltm_src;
    MkProc* proc;
    PfxNostackFlagBits* nostack_bits;
    float zero;

    created_proc = 0;
    pfx = (MkPfx*)get_mkhdr(&vtbl_pfx, extra_size + 0x2C0);
    if (pfx == 0) {
        return 0;
    }

    zero = kZero;

    pfx->proc = 0;
    pfx->proc_inst = 0;
    *(unsigned int*)&pfx->flags = 0;
    pfx->bone_mat = 0;
    pfx->bind_hdr = 0;
    pfx->bind_inst = 0;
    pfx->slot_table = 0;
    pfx->field_28 = zero;
    pfx->field_2C = 0x12;
    pfx->accum_34 = zero;
    pfx->accum_38 = zero;
    pfx->mem = 0;
    pfx->bound_obj = 0;

    vm = pfx_vm(pfx);
    pfxvm_init(vm);

    pfx->field_90 = field_90;
    pfx->field_214 = field_214;
    pfx->field_A0 = field_a0;
    pfx->field_280 = 0;
    pfx->field_284 = 0;
    pfx->field_288 = 0;
    pfx->field_28C = 0;
    pfx->field_290 = 0;
    pfx->field_294 = 0;
    pfx->field_298 = zero;
    pfx->field_29C = zero;
    pfx->field_2A0 = zero;
    pfx->field_2A4 = zero;
    pfx->field_2A8 = zero;
    pfx->field_2B8 = 0;
    pfx->field_2BC = 0;
    pfx->scale = kDefaultPfxScale;

    memset(emitter_buf, 0, 0x2EC);
    if (init_cb != 0) {
        pfx->emitter_scratch = (PfxEmitter*)emitter_buf;
        pfx->slot_count = 1;
        init_cb(vm);
    }

    field_pair[0] = pfx_emitter_field_pair_seed[0];
    field_pair[1] = pfx_emitter_field_pair_seed[1];
    pfx_emitter_scan_for_fields(emitter_buf, field_pair);
    pfx->field_214 |= (int)field_pair[0];
    pfx->field_A0 |= (int)field_pair[1];

    ready = 0;
    if (pfx_native_is_supported_type(pfx->field_214) == 0) {
        ready = 0;
    } else {
        pfx_estimate_size(vm, &est_buf, build);
        pad_raw = build->flag * 0xC;
        pad_align = (pad_raw + 0xF) & ~0xF;
        pad_extra = pad_align - pad_raw;
        est_size = est_buf.size;
        alloc_size = pad_raw + pad_extra + est_size + 0x10;
        mem = get_mem(alloc_size);
        if (mem == 0) {
            ready = 0;
        } else {
            pfx->mem = mem;
            memset(mem, 0, alloc_size);
            aligned = (void*)(((unsigned int)mem + 0xF) & ~0xFU);
            if (build->flag != 0) {
                pfx->slot_table = (PfxSlot*)aligned;
                aligned = (char*)aligned + pad_raw + pad_extra;
            }
            pfx_set_memory(vm, aligned, &est_buf);
            name_obj = ((PfxVm*)vm)->name_obj;
            if (name_obj != 0) {
                name_obj->scale = pfx->scale;
            }
            if (pfx_frame_begin(vm) != 0) {
                pfx_frame_end(vm);
                ready = 0;
            } else {
                pfx_frame_end(vm);
                if (pfx->behaviors_active != 0) {
                    pfx_behaviors_frame_begin(vm);
                    pfx_behaviors_frame_end(vm);
                }
                name_dst = pfx->name_dst;
                if (name_dst != 0) {
                    strcpy(name_dst, build->name);
                }
                ready = 1;
            }
        }
    }

    if (ready == 0) {
        if (pfx->hdr.instance != 0) {
            vtbl_call_destroy(&pfx->hdr);
        }
        return 0;
    }

    if (pfx->slot_count != 0 && init_cb != 0) {
        ltm_src = (PfxEmitter*)pfx->emitter_scratch;
        old_ltm_415 = ltm_src->transform;
        memcpy(ltm_src, emitter_buf, 0x2EC);
        ltm_src->transform = old_ltm_415;
    }

    if (entry != 0) {
        proc_nostack_slot = 0;
        nostack_bits = (PfxNostackFlagBits*)&proc_nostack_slot;
        nostack_bits->nostack = 1;
        proc_nostack_arg = (int)proc_nostack_slot;
        proc = create_mkproc(0x2E, get_mkproc_nostack(&proc_nostack_arg), pid, entry,
                             &pfx->hdr);
        if (proc == 0) {
            pfx = 0;
        } else {
            created_proc = proc;
            proc->pre_destroy = pfx_pre_wake;
            proc->destroy_cb = pfx_post_sleep;
            pfx->proc = (MkHdr*)proc;
            pfx->proc_inst = (unsigned int)proc->instance;
            mk_insert(&pfx->hdr, &pfx_render_list);
        }
    } else {
        mk_insert(&pfx->hdr, &pfx_render_list);
    }

    *out_pfx = (void*)pfx;
    return created_proc;
}

static void apfx_set_transform_matrix(void) {
    MkPfx* pfx;
    PfxVm* vm;
    float* dst;
    void* src;
    int slot;
    float zero;
    float one;

    pfx = apfx;
    vm = pfx_vm(pfx);
    if (vm == 0) {
        return;
    }
    slot = vm->active_slot;
    dst = vm->mats[slot].m;
    src = pfx->bone_mat;
    if (src == 0) {
        return;
    }
    memcpy(dst, src, 0x40);
    zero = kZero;
    one = kOne;
    dst[3] = zero;
    dst[7] = zero;
    dst[11] = zero;
    dst[15] = one;
}

void pfx_post_sleep(void) {
    MkPfx* pfx;
    float ticks;
    float accum;

    pfx = apfx;
    if (pfx != 0) {
        if (pfx->behaviors_active != 0) {
            pfx_behaviors_frame_end(pfx_vm(pfx));
        }
        pfx_frame_end(pfx_vm(pfx));
        pfx_frame_end_check(pfx_vm(pfx));
        pfxmetrics_event(pfx->metrics_handle, 0x2000);

        ticks = aproc->sleep_ticks;
        accum = pfx->accum_34;
        pfx->accum_34 = accum + (float)(int)ticks;
        pfx->accum_38 = game_speed * aproc->sleep_ticks + pfx->accum_38;
        apfx = 0;
    }
    apfx_render_obj = 0;
    apfx_emitter_obj = 0;
    apfx_render_sobj = 0;
    apfx_emitter_sobj = 0;
}

void pfx_pre_wake(void) {
    MkPfx* pfx;
    MkHdr* render_hdr;
    PfxSlot* emitter_slot;
    MkHdr* emitter_hdr;
    MkHdr* proc_hdr;
    int begin_rc;

    pfx = (MkPfx*)apdata;
    apfx = pfx;
    if (pfx == 0) {
        return;
    }

    apfx_render_obj = 0;
    apfx_render_sobj = 0;

    render_hdr = resolve_mkptr(pfx->bind_hdr, pfx->bind_inst);
    if (pfx->bind_hdr != 0 && render_hdr == 0) {
        mkproc_die();
    }
    if (render_hdr != 0) {
        apfx_render_obj = as_mkobj(render_hdr);
        apfx_render_sobj = vtbl_call_get_sobj(render_hdr);
    }

    emitter_slot = pfx->slot_table;
    if (emitter_slot != 0) {
        emitter_hdr = resolve_mkptr(emitter_slot->hdr, emitter_slot->instance);
        if (emitter_hdr != 0) {
            apfx_emitter_obj = as_mkobj(emitter_hdr);
            apfx_emitter_sobj = vtbl_call_get_sobj(emitter_hdr);
        }
    }

    begin_rc = pfx_frame_begin(pfx_vm(pfx));
    if (begin_rc != 0) {
        pfx_frame_end(pfx_vm(pfx));
        proc_hdr = resolve_mkptr(pfx->proc, pfx->proc_inst);
        if (pfx->hdr.instance != 0) {
            vtbl_call_destroy(&pfx->hdr);
        }
        if (proc_hdr != 0) {
            vtbl_call_destroy(proc_hdr);
        }
        apfx = 0;
        apdata = 0;
        return;
    }

    if (pfx->behaviors_active != 0) {
        pfx_behaviors_frame_begin(pfx_vm(pfx));
    }
    pfx->tick = exec_tick_ctr;
}

int mkpfx_init(void) {
    void* iface[6];

    iface[0] = 0;
    iface[1] = 0;
    iface[2] = 0;
    iface[3] = 0;
    iface[4] = (void*)pfxmetrics_start_timer;
    iface[5] = (void*)pfxmetrics_stop_timer;
    pfxmetrics_set_interface(iface);
    return 1;
}

static unsigned int pfxmetrics_stop_timer(int id) {
    return stop_usec_timer(id + 5);
}

static void pfxmetrics_start_timer(int id) {
    start_usec_timer(id + 5);
}
