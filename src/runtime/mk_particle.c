#include "runtime/mk_particle.h"

#include "game/game_info.h"
#include "libmkparticle/metrics.h"
#include "libmkparticle/particle_system.h"
#include "libmkparticle/particle_render.h"
#include "libmkparticle/supported.h"
#include "libmkparticle/init.h"
#include "libmkparticle/emitter.h"
#include "libmkparticle/behavior.h"
#include "libmkparticle/gc_render.h"
#include "runtime/mk_mem.h"
#include "runtime/asset.h"
#include "runtime/cstring.h"
#include "runtime/mk_render.h"
#include "runtime/utils.h"
#include "platform/main.h"
#include "platform/display.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwengine.h"
#include "rw/rwframe.h"

void pfx_reset_renderstate(void* vm);

static float* inverse_camera_matrix;
static void* old_ltm_415;

MkPfx* apfx;
MkObj* apfx_render_obj;
MkObj* apfx_emitter_obj;
MkSobj* apfx_render_sobj;
MkSobj* apfx_emitter_sobj;
MkPtr* pfx_render_list;
MkPtr* pfx_clone_render_list;

static void apfx_set_transform_matrix(void);
static int pfxmetrics_stop_timer(int id);
static void pfxmetrics_start_timer(int id);

static const float kZero = 0.0f;
static const float kOne = 1.0f;
static const float kDefaultPfxScale = -10000.0f;

#define MKPFX_MKOBJ_FROM_HDR(hdr_) ((MkObj*)(hdr_))

static inline MkObj* as_mkobj(MkHdr* hdr) {
    if (hdr == 0) {
        return 0;
    }
    if (hdr->vtbl != MK_VTABLE_ADDRESS(vtbl_mkobj)) {
        return 0;
    }
    return (MkObj*)hdr;
}

static inline MkSobj* vtbl_call_get_sobj(MkHdr* hdr) {
    MkVtable5* vtbl;

    if (hdr == 0) {
        return 0;
    }
    vtbl = hdr->vtbl;
    return (MkSobj*)vtbl->fn2(hdr);
}

static inline void vtbl_call_destroy(MkHdr* hdr) {
    MkHdrVtable* vtbl;

    if (hdr == 0 || hdr->instance == 0) {
        return;
    }
    vtbl = hdr->typed_vtbl;
    vtbl->destroy(hdr);
}

static inline int flag_msb(unsigned char byte) {
    return (signed char)(byte & 0x80);
}

static inline PfxVm* pfx_vm(MkPfx* pfx) {
    return (PfxVm*)pfx->matrix;
}

void mkpfx_get_origin(MkPfx* pfx, float origin[3]) {
    int slot;
    MkHdr* bound;
    MkObj* mkobj;
    MkSobj* sobj;
    MkVtable5* vtbl;
    RwMatrix* ltm;
    PfxTransform* transform;

    slot = pfx->active_slot;
    transform = &pfx->transforms[slot];
    bound = MK_LIVE(pfx->slot_table->hdr, pfx->slot_table->instance);

    if (bound != 0) {

        vtbl = bound->vtbl;
        if (vtbl == MK_VTABLE_ADDRESS(vtbl_mkobj)) {
            mkobj = MKPFX_MKOBJ_FROM_HDR(bound);
        } else {
            mkobj = 0;
        }
        if (bound != 0) {
            sobj = (MkSobj*)vtbl->fn2(bound);
        } else {
            sobj = 0;
        }
        if (mkobj != 0) {
            ltm = RwFrameGetLTM(mkobj->frame);
            origin[0] = ltm->pos.x;
            origin[1] = ltm->pos.y;
            origin[2] = ltm->pos.z;
            return;
        }
        if (sobj != 0) {
            ltm = RwFrameGetLTM(sobj->frame);
            origin[0] = ltm->pos.x;
            origin[1] = ltm->pos.y;
            origin[2] = ltm->pos.z;
        } else {
            return;
        }
    } else {
        origin[0] = transform->position.x;
        origin[1] = transform->position.y;
        origin[2] = transform->position.z;
    }
}

void mkpfx_camera_end(void) {
    pfx_count_end();
}

void mkpfx_camera_begin(void) {
    pfxsystem_set_frame_info(0, 0, (const float*)&camera_facing_matrix_ay, Camera);
    pfx_count_begin();
}

void mkpfx_set_environment(void) {
    pfxsystem_set_global(0x500, g_game_info.field_34);
}

MkHdr* pfx_get_emitter_obj(MkPfx* pfx, int index) {
    PfxSlot* table;
    int count;
    MkHdr* emitter;
    MkHdr* result;

    if (pfx == 0) {
        return 0;
    }
    count = pfx->slot_count;
    if (index >= count || index < 0) {
        return 0;
    }
    table = pfx->slot_table;
    emitter = MK_LIVE(table[index].hdr, table[index].instance);
    result = 0;
    if (emitter != 0) {
        result = emitter;
    }
    return result;
}

void vdestroy_pfx_clone(PfxClone* clone) {
    MkHdr* hdr;

    if (clone->flag_bits.destroyed) {
        return;
    }

    clone->hdr.instance = 0;
    clone->flag_bits.destroyed = 1;

    if (clone->flag_bits.owns_bind) {
        hdr = MK_LIVE(clone->bind_hdr, clone->bind_inst);
        vtbl_call_destroy(hdr);
    }
    if (clone->flag_bits.owns_bind2) {
        hdr = MK_LIVE(clone->bind2_hdr, clone->bind2_inst);
        vtbl_call_destroy(hdr);
    }

    clone->hdr.instance = 0;
    mkhdr_memfree(&clone->hdr);
}

static inline void destroy_owned_pfx_slots(MkPfx* pfx) {
    if (pfx->slot_table != 0) {
        int i = 0;
        while (i < pfx->slot_count) {
            PfxSlot* slot = &pfx->slot_table[i];

            if (slot->flag_bits.owns_bind) {
                MkHdr* hdr = MK_LIVE(slot->hdr, slot->instance);
                vtbl_call_destroy(hdr);
            }
            i++;
        }
    }
}

void vdestroy_pfx(MkPfx* pfx) {
    void* mem;
    if (pfx->flag_bits.destroyed) {
        return;
    }

    pfx->hdr.instance = 0;
    pfx->flag_bits.destroyed = 1;

    if (pfx->flag_bits.owns_bind) {
        MkHdr* hdr = MK_LIVE(pfx->bind_hdr, pfx->bind_inst);
        vtbl_call_destroy(hdr);
    }

    destroy_owned_pfx_slots(pfx);

    mem = pfx->mem;
    if (mem != 0) {
        free_mem_delayed(mem, 3);
    }

    pfx->hdr.instance = 0;
    mkhdr_memfree(&pfx->hdr);
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
    mat = pfx->transforms[pfx->active_slot].matrix.elements;
    memcpy(save, mat, sizeof(save));
    memcpy(mat, clone->matrix_copy, sizeof(save));

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
        pfx_set_renderstate((struct PfxRenderView*)pfx_vm(pfx));
        particle_render(pfx_vm(pfx));
        pfx_reset_renderstate(pfx_vm(pfx));
        pfxmetrics_event(pfx->metrics_handle, 0x4005);
    }

    memcpy(mat, save, sizeof(save));
}

void render_pfx(MkPfx* pfx) {
    MkHdr* bound;
    MkObj* mkobj;
    MkHdr* parent;
    float* inv;
    float zero;
    float one;

    bound = MK_LIVE(pfx->slot_table->hdr, pfx->slot_table->instance);
    if (bound != 0) {
        mkobj = bound->vtbl == MK_VTABLE_ADDRESS(vtbl_mkobj) ? (MkObj*)bound : 0;
        if (mkobj != 0) {
            if (((mkobj->flags_0C >> 1) & 1) != 0) {
                parent = MK_LIVE(mkobj->parent_hdr, mkobj->parent_inst);
                if (parent == 0 || ((((MkObj*)parent)->hide_flags >> 5) & 1)) {
                    return;
                }
            } else if ((mkobj->hide_flags >> 5) & 1) {
                return;
            }
        }
    }

    if (((pfx->flags >> 4) & 1) == 0) {
        return;
    }

    if (pfx->transform_cb != 0) {
        apfx = pfx;
        pfx->transform_cb();
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
        pfx_set_renderstate((struct PfxRenderView*)pfx_vm(pfx));
        particle_render(pfx_vm(pfx));
        pfx_reset_renderstate(pfx_vm(pfx));
        pfxmetrics_event(pfx->metrics_handle, 0x4005);
    }
}

void hide_pfx(MkPfx* pfx, int hide) {
    pfx->f80.hide = hide;
}

void pfx_end_batch(void) {
}

void pfx_start_batch(void) {
}

void insert_PFXlist_in_transl_tree(void) {
    RwCamera* camera;
    void* frame;

    camera = RwEngineInstance->curCamera;
    if (camera == 0) {
        return;
    }
    frame = camera->object.object.parent;
    inverse_camera_matrix = (float*)RwFrameGetLTM(frame);
    apply_to_mklist(InsertPFXInTranslTree, &pfx_render_list);
    apply_to_mklist(InsertPFXCloneInTranslTree, &pfx_clone_render_list);
}

void set_pfx_texture(PfxVm* pfx, int handle, unsigned int art_oid) {
    RwTexture* tex;

    tex = load_tga(handle, art_oid);
    if (tex != 0) {
        pfx_set_texture((struct PfxRenderView*)pfx, tex);
    }
}

MkObj* pfx_clone_bind_render_to_new_obj(PfxClone* clone, int object_type) {
    MkObj* obj;

    obj = get_mkobj_frame(object_type, 0);
    if (obj != 0) {
        clone->flag_bits.owns_bind = 1;
        clone->bind_hdr = &obj->hdr;
        clone->bind_inst = obj->hdr.instance;
        clone->matrix_copy = &obj->frame->modelling;
        insert_particle_mkobj(obj);
    }
    return obj;
}

void pfx_bind_emitter_num_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone, int emitter) {
    PfxEmitter* emitter_vm;
    void* bone_mat;

    pfx->slot_table[emitter].flag_bits.owns_bind = 0;
    pfx->slot_table[emitter].hdr = &obj->hdr;
    pfx->slot_table[emitter].instance = obj->hdr.instance;

    bone_mat = obj->bones[bone];
    if (bone_mat == 0) {
        bone_mat = obj->field_24;
        emitter_vm = pfx_get_emitter(pfx_vm(pfx), emitter);
        emitter_vm->transform = bone_mat;
        return;
    }

    emitter_vm = pfx_get_emitter(pfx_vm(pfx), emitter);
    emitter_vm->transform = bone_mat;
    calc_bone_world_mat(obj, bone);
    obj_set_bone_calc_world_mat_flag(obj, bone);
}

void pfx_bind_emitter_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone) {
    PfxSlot* slot;
    PfxEmitter* emitter_vm;
    void* bone_mat;

    slot = pfx->slot_table;
    slot->flag_bits.owns_bind = 0;
    pfx->slot_table->hdr = &obj->hdr;
    pfx->slot_table->instance = obj->hdr.instance;

    bone_mat = obj->bones[bone];
    if (bone_mat == 0) {
        bone_mat = obj->field_24;
        emitter_vm = pfx_get_emitter(pfx_vm(pfx), 0);
        emitter_vm->transform = bone_mat;
        return;
    }

    emitter_vm = pfx_get_emitter(pfx_vm(pfx), 0);
    emitter_vm->transform = bone_mat;
    calc_bone_world_mat(obj, bone);
    obj_set_bone_calc_world_mat_flag(obj, bone);
}

/* TODO: [near miss] 88.61%; native matrix selection agrees; bone index is retained before the success branch and callback staging differs. */
void pfx_bind_render_to_obj_bone(MkPfx* pfx, MkObj* obj, int bone) {
    MkBone* bone_mat;

    pfx->flag_bits.owns_bind = 0;
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
    void* ltm;
    PfxEmitter* emitter_vm;

    pfx->slot_table[emitter].flag_bits.owns_bind = flag;
    pfx->slot_table[emitter].hdr = &sobj->hdr;
    pfx->slot_table[emitter].instance = sobj->hdr.instance;
    ltm = RwFrameGetLTM(sobj->frame);
    emitter_vm = pfx_get_emitter(pfx_vm(pfx), emitter);
    emitter_vm->transform = ltm;
}

void pfx_bind_emitter_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag) {
    void* ltm;
    PfxEmitter* emitter_vm;

    pfx->slot_table->flag_bits.owns_bind = flag;
    pfx->slot_table->hdr = &sobj->hdr;
    pfx->slot_table->instance = sobj->hdr.instance;
    ltm = RwFrameGetLTM(sobj->frame);
    emitter_vm = pfx_get_emitter(pfx_vm(pfx), 0);
    emitter_vm->transform = ltm;
}

void pfx_bind_render_to_sobj(MkPfx* pfx, MkSobj* sobj, int flag) {
    pfx->flag_bits.owns_bind = flag;
    pfx->bind_hdr = &sobj->hdr;
    pfx->bind_inst = sobj->hdr.instance;
    pfx->transform_cb = apfx_set_transform_matrix;
    pfx->bone_mat = RwFrameGetLTM(sobj->frame);
}

static inline MkObj* pfx_slot_bound_object(const PfxSlot* slot)
{
    MkObj* obj = 0;
    MkHdr* bound = slot->hdr;

    if (bound != 0) {
        bound = bound->instance == slot->instance ? bound : 0;
    } else {
        bound = 0;
    }
    if (bound != 0) {
        obj = (MkObj*)bound;
    }
    return obj;
}

/* TODO: [near miss] 99.15%; binding operations agree; stop at bound-object result register coloring. */
MkObj* pfx_bind_to_new_obj(MkPfx* pfx, int object_type) {
    PfxSlot* slot;
    MkObj* obj;
    PfxEmitter* emitter_vm;
    RwMatrix* ltm;

    if (pfx == 0 || pfx->slot_count <= 0) {
        obj = 0;
    } else {
        slot = pfx->slot_table;
        if (slot->flag_bits.owns_bind != 0) {
            obj = pfx_slot_bound_object(slot);
            if (obj != 0) {
                return obj;
            }
        }
        obj = get_mkobj_frame(object_type, 0);
        if (obj != 0) {
            if (pfx != 0 && obj != 0 && pfx->slot_count > 0) {
                pfx->slot_table->flag_bits.owns_bind = 1;
                pfx->slot_table->hdr = &obj->hdr;
                pfx->slot_table->instance = obj->hdr.instance;
                ltm = &obj->frame->modelling;
                emitter_vm = pfx_get_emitter(pfx_vm(pfx), 0);
                emitter_vm->transform = ltm;
            }
            insert_particle_mkobj(obj);
        }
    }
    return obj;
}

static inline void pfx_bind_owned_emitter(MkPfx* pfx, MkObj* obj, int emitter)
{
    PfxEmitter* emitter_vm;
    RwMatrix* ltm;

    if (pfx != 0 && obj != 0 && emitter >= 0 && emitter < pfx->slot_count) {
        pfx->slot_table[emitter].flag_bits.owns_bind = 1;
        pfx->slot_table[emitter].hdr = &obj->hdr;
        pfx->slot_table[emitter].instance = obj->hdr.instance;
        ltm = &obj->frame->modelling;
        emitter_vm = pfx_get_emitter(pfx_vm(pfx), emitter);
        emitter_vm->transform = ltm;
    }
}

/* TODO: [near miss] 98.50%; native branches and operands agree;
 * two null assignments use moves instead of immediates. */
MkObj* pfx_bind_emitter_num_to_new_obj(MkPfx* pfx, int object_type, int emitter) {
    PfxSlot* slot;
    MkObj* existing;
    MkObj* obj;

    if (pfx == 0 || emitter < 0 || emitter >= pfx->slot_count) {
        return 0;
    }

    slot = &pfx->slot_table[emitter];
    if (pfx->slot_table[emitter].flag_bits.owns_bind) {
        existing = pfx_slot_bound_object(slot);
        if (existing != 0) {
            return existing;
        }
    }

    obj = get_mkobj_frame(object_type, 0);
    if (obj != 0) {
        pfx_bind_owned_emitter(pfx, obj, emitter);
        insert_particle_mkobj(obj);
    }
    return obj;
}

void pfx_bind_emitter_to_obj(MkPfx* pfx, MkObj* obj, int flag) {
    PfxEmitter* emitter_vm;
    RwMatrix* ltm;

    if (pfx != 0 && obj != 0 && pfx->slot_count > 0) {
        pfx->slot_table->flag_bits.owns_bind = flag;
        pfx->slot_table->hdr = &obj->hdr;
        pfx->slot_table->instance = obj->hdr.instance;
        ltm = &obj->frame->modelling;
        emitter_vm = pfx_get_emitter(pfx_vm(pfx), 0);
        emitter_vm->transform = ltm;
    }
}

void pfx_bind_emitter_num_to_obj(MkPfx* pfx, MkObj* obj, int flag, int emitter) {
    PfxEmitter* emitter_vm;
    void* ltm;

    if (pfx == 0 || obj == 0 || emitter < 0 || emitter >= pfx->slot_count) {
        return;
    }
    pfx->slot_table[emitter].flag_bits.owns_bind = flag;
    pfx->slot_table[emitter].hdr = &obj->hdr;
    pfx->slot_table[emitter].instance = obj->hdr.instance;
    ltm = &obj->frame->modelling;
    emitter_vm = pfx_get_emitter(pfx_vm(pfx), emitter);
    emitter_vm->transform = ltm;
}

void pfx_bind_render_to_obj(MkPfx* pfx, MkObj* obj, int flag) {
    pfx->flag_bits.owns_bind = flag;
    pfx->bind_hdr = &obj->hdr;
    pfx->bind_inst = obj->hdr.instance;
    pfx->transform_cb = apfx_set_transform_matrix;
    pfx->bone_mat = &obj->frame->modelling;
}

PfxClone* pfx_create_clone(MkPfx* pfx) {
    PfxClone* clone;
    float zero;

    clone = (PfxClone*)get_mkhdr(&vtbl_pfx_clone, sizeof(PfxClone));
    if (clone != 0) {
        zero = kZero;
        clone->parent = pfx;
        clone->matrix_copy = 0;
        clone->bind_hdr = 0;
        clone->bind_inst = 0;
        clone->bind2_hdr = 0;
        clone->bind2_inst = 0;
        clone->flags_word = 0;
        clone->depth_bias = zero;
        clone->priority = 0x12;
        mk_insert(&clone->hdr, &pfx_clone_render_list);
    }
    return clone;
}

void* pfx_create_raw_userdata(int extra_size, int userdata_size, int field_90,
                              int field_214, int field_a0, PfxInitCb init_cb,
                              int pid, MkProcEntryFn entry, void** out_pfx) {
    static PfxBuildInfo empty_build_info = {0};

    empty_build_info.particle_user_data_size = userdata_size;
    empty_build_info.emitter_count = 1;
    return new_pfx_create_raw_userdata(&empty_build_info, extra_size, field_90,
                                       field_214, field_a0, init_cb, pid, entry,
                                       out_pfx);
}

static inline PfxVm* scan_pfx_emitter_fields(MkPfx* pfx, PfxEmitter* emitter)
{
    unsigned int field_pair[2] = {0, 0};
    PfxVm* vm;

    pfx_emitter_scan_for_fields(emitter, field_pair);
    vm = pfx_vm(pfx);
    pfx->field_214 |= field_pair[0];
    pfx->field_A0 |= field_pair[1];
    return vm;
}

static inline int initialize_pfx_memory(MkPfx* pfx, PfxVm* vm,
                                        PfxBuildInfo* build)
{
    PfxEstimate est_buf;
    int ready;
    int pad_raw;
    int pad_align;
    unsigned int alloc_size;
    int est_size;
    int pad_extra;
    void* mem;
    void* aligned;
    PfxNameObj* name_obj;
    char* name_dst;

    if (pfx_native_is_supported_type(pfx->field_214) == 0) {
        ready = 0;
    } else {
        pfx_estimate_size(vm, &est_buf, build);
        est_size = est_buf.size;
        pad_raw = build->emitter_count;
        pad_raw *= sizeof(PfxSlot);
        pad_align = (pad_raw + 0xF) & ~0xF;
        pad_extra = pad_align - pad_raw;
        alloc_size = (pad_raw + pad_extra) + (est_size + 0x10);
        mem = get_mem(alloc_size);
        if (mem == 0) {
            ready = 0;
        } else {
            pfx->mem = mem;
            memset(mem, 0, alloc_size);
            aligned = (void*)(((unsigned long)mem + 0xFUL) & ~0xFUL);
            if (build->emitter_count != 0) {
                pfx->slot_table = aligned;
                aligned = (char*)aligned + pad_raw + pad_extra;
            }
            pfx_set_memory(vm, aligned, &est_buf);
            name_obj = vm->name_obj;
            if (name_obj != 0) {
                name_obj->scale = vm->effect_scale;
            }
            if (pfx_frame_begin(vm) != 0) {
                pfx_frame_end(vm);
                ready = 0;
            } else {
                pfx_frame_end(vm);
                if (vm->behavior_count != 0) {
                    pfx_behaviors_frame_begin(vm);
                    pfx_behaviors_frame_end(vm);
                }
                name_dst = vm->name;
                if (name_dst != 0) {
                    strcpy(name_dst, build->name);
                }
                ready = 1;
            }
        }
    }
    return ready;
}

/* TODO: [near miss] 99.70%; allocation scratch and emitter-save homes remain. */
void* new_pfx_create_raw_userdata(PfxBuildInfo* build, int extra_size, int field_90,
                                  int field_214, int field_a0, PfxInitCb init_cb,
                                  int pid, MkProcEntryFn entry, void** out_pfx) {
    MkProc* created_proc;
    MkPfx* pfx;
    PfxVm* vm;

    MkProcInitFlags proc_nostack_slot;
    PfxEmitter emitter_buf;
    int ready;

    float zero;

    *out_pfx = created_proc = 0;
    pfx = (MkPfx*)get_mkhdr(&vtbl_pfx, extra_size + sizeof(MkPfx));
    if (pfx == 0) {
        return 0;
    }

    zero = kZero;

    pfx->proc = 0;
    pfx->proc_inst = 0;
    pfx->flags_word = 0;
    pfx->bone_mat = 0;
    pfx->bind_hdr = 0;
    pfx->bind_inst = 0;
    pfx->slot_table = 0;
    pfx->depth_bias = zero;
    pfx->priority = 0x12;
    pfx->accum_34 = zero;
    pfx->accum_38 = zero;
    pfx->mem = 0;
    pfx->bound_obj = 0;

    pfxvm_init(pfx_vm(pfx));

    pfx->field_90 = field_90;
    pfx->field_214 = field_214;
    pfx->field_A0 = field_a0;
    pfx->tracked_object = 0;
    pfx->tracked_object_instance = 0;
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

    memset(&emitter_buf, 0, sizeof(emitter_buf));
    if (init_cb != 0) {
        pfx->emitter_scratch = &emitter_buf;
        pfx->slot_count = 1;
        init_cb(pfx_vm(pfx));
    }

    vm = scan_pfx_emitter_fields(pfx, &emitter_buf);

    ready = initialize_pfx_memory(pfx, vm, build);

    if (ready == 0) {
        if (pfx->hdr.instance != 0) {
            pfx->hdr.typed_vtbl->destroy(&pfx->hdr);
        }
        return 0;
    }

    if (pfx->slot_count != 0 && init_cb != 0) {
        PfxEmitter* ltm_src = pfx->emitter_scratch;

        old_ltm_415 = ltm_src->transform;
        memcpy(ltm_src, &emitter_buf, sizeof(emitter_buf));
        ltm_src->transform = old_ltm_415;
    }

    if (entry != 0) {
        proc_nostack_slot.value = 0;
        proc_nostack_slot.bits.has_pdata = 1;

        created_proc = create_mkproc(0x2E, get_mkproc_nostack(proc_nostack_slot), pid, entry,
                             &pfx->hdr);
        if (created_proc != 0) {
            created_proc->pre_destroy = pfx_pre_wake;
            created_proc->destroy_cb = pfx_post_sleep;
            pfx->proc = &created_proc->hdr;
            pfx->proc_inst = created_proc->instance;
            mk_insert(&pfx->hdr, &pfx_render_list);
        } else {
            pfx = 0;
        }
    } else {
        mk_insert(&pfx->hdr, &pfx_render_list);
    }

    *out_pfx = pfx;
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
    slot = vm->active_transform;
    dst = vm->transforms[slot].matrix.elements;
    src = pfx->bone_mat;
    if (src == 0) {
        return;
    }
    memcpy(dst, src, sizeof(vm->transforms[slot].matrix));
    zero = kZero;
    one = kOne;
    dst[3] = zero;
    dst[7] = zero;
    dst[11] = zero;
    dst[15] = one;
}

void pfx_post_sleep(void) {
    float ticks;
    float accum;

    if (apfx != 0) {
        if (apfx->behaviors_active != 0) {
            pfx_behaviors_frame_end(pfx_vm(apfx));
        }
        pfx_frame_end(pfx_vm(apfx));
        pfx_frame_end_check(pfx_vm(apfx));
        pfxmetrics_event(apfx->metrics_handle, 0x2000);

        ticks = aproc->sleep_ticks;
        accum = apfx->accum_34;
        apfx->accum_34 = accum + (float)(int)ticks;
        apfx->accum_38 += game_speed * aproc->sleep_ticks;
        apfx = 0;
    }
    apfx_render_obj = 0;
    apfx_emitter_obj = 0;
    apfx_render_sobj = 0;
    apfx_emitter_sobj = 0;
}

/* TODO: [breakthrough needed] 69.63%; resolve binding-validation and wake/failure CFG against retail. */
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

    render_hdr = MK_LIVE(pfx->bind_hdr, pfx->bind_inst);
    if (pfx->bind_hdr != 0 && render_hdr == 0) {
        mkproc_die();
    }
    if (render_hdr != 0) {
        apfx_render_obj = as_mkobj(render_hdr);
        apfx_render_sobj = vtbl_call_get_sobj(render_hdr);
    }

    emitter_slot = pfx->slot_table;
    if (emitter_slot != 0) {
        emitter_hdr = MK_LIVE(emitter_slot->hdr, emitter_slot->instance);
        if (emitter_hdr != 0) {
            apfx_emitter_obj = as_mkobj(emitter_hdr);
            apfx_emitter_sobj = vtbl_call_get_sobj(emitter_hdr);
        }
    }

    begin_rc = pfx_frame_begin(pfx_vm(pfx));
    if (begin_rc != 0) {
        pfx_frame_end(pfx_vm(pfx));
        proc_hdr = MK_LIVE(pfx->proc, pfx->proc_inst);
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
    static const PfxMetricsInterface interface = {
        0, 0, 0, 0, pfxmetrics_start_timer, pfxmetrics_stop_timer,
    };
    PfxMetricsInterface interface_copy = interface;

    pfxmetrics_set_interface(&interface_copy);
    return 1;
}

static int pfxmetrics_stop_timer(int id) {
    return stop_usec_timer(id + 5);
}

static void pfxmetrics_start_timer(int id) {
    start_usec_timer(id + 5);
}
