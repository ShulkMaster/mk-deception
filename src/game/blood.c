#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"
#include "game/pfxscript.h"
#include "math/mk_math.h"

#define BLOOD_SPLAT_COUNT 24

typedef struct BloodSplat {
    int field_00;
    float reset_height; /* +0x04 */
    char pad08[0x10];
} BloodSplat; /* 0x18 */

typedef void (*BloodProcDestroyFn)(MkProc* proc);

typedef struct BloodProcVtablePrefix {
    void* reserved[4];
    BloodProcDestroyFn destroy;
} BloodProcVtablePrefix;

typedef union BloodProcVtableRef {
    MkVtableMkproc* base;
    BloodProcVtablePrefix* blood;
} BloodProcVtableRef;

typedef struct BloodProcLatch {
    MkProc* proc;
    unsigned int instance;
} BloodProcLatch;

typedef struct DecalEmitterWatcherPdata {
    MkHdr hdr;                /* +0x00 */
    char pad08[8];
    MKMATRIX matrices[10];    /* +0x10 */
    char pad290[0x10];
} DecalEmitterWatcherPdata; /* 0x2A0 */

typedef struct BloodFxFlags {
    unsigned char pad_high : 3;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char pad_low : 3;
} BloodFxFlags;

typedef struct BloodFxUserdata {
    char pad00[0x40];
    union {
        unsigned char flags_40;
        BloodFxFlags flags_40_bits;
    };
} BloodFxUserdata;

static const char* blood_decal_to_reset[] = {
    "blsplat",
    "blsplat2",
    "blpuddle",
    "blpuddle2",
    "blsmash",
    "blfoot",
    0,
};
extern BloodSplat ncs_blood_splat_list[BLOOD_SPLAT_COUNT];
extern BloodProcLatch ncs_pfx_decal_emitter_proc;
extern BloodProcLatch bleed_pfx_proc_item;
extern BloodProcLatch bleed_proc_item;
extern MkPtr* gusher_list;
extern int bleed_startup__fire_off_splat_watcher_func;
extern MkVtable5 vtbl_mkpdata_generic;

void* memset(void* destination, int value, unsigned long size);
unsigned int fx_by_owner(const char* name, int owner);
void spawn_decal_emitter(
    const char* name, MkObj* object, const Vec* position, const Vec* direction,
    float angle);
void start_blood_splat_watcher(void);

static float p_decal_emitter_watcher(void);
static float p_bleed(void);
static float p_pfx_bleed(void);
static void do_pfx_bleed(MkHdr* hdr);

int is_blood_disabled(void) {
    return get_blood_level() < 2;
}

void gusher_destroy_list(void) {
    MkPtr* ptr;

    if (&gusher_list != 0) {
        ptr = gusher_list;
        while (ptr != 0) {
            MkHdr* hdr;

            hdr = ptr->hdr;
            if (ptr->instance != hdr->instance) {
                MkPtr* next;

                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
                continue;
            }

            if ((void*)hdr->vtbl == (void*)&vtbl_mkpdata_generic &&
                hdr->instance != 0U) {
                BloodProcVtableRef vtbl;

                vtbl.base = (MkVtableMkproc*)hdr->vtbl;
                vtbl.blood->destroy((MkProc*)hdr);
            }
            ptr = ptr->next;
        }
    }
    gusher_list = 0;
}

void kill_gusher(MkProc* proc) {
    BloodProcVtableRef vtbl;

    if (proc->instance != 0U) {
        vtbl.base = proc->vtbl;
        vtbl.blood->destroy(proc);
    }
}

static float p_foot_print_wait(void) {
    return 60.0f;
}

void spawn_bld_splat(const char* name, MkObj* object, const Vec* position) {
    spawn_decal_emitter(name, object, position, 0, 0.0f);
}

void start_decal_emitter_watcher(void) {
    DecalEmitterWatcherPdata* pdata;
    MkProc* proc;
    int index;

    proc = _create_mkproc_generic_nostack(
        0x601A, 0x30, p_decal_emitter_watcher,
        sizeof(DecalEmitterWatcherPdata), (MkHdr**)&pdata);
    if (proc != 0) {
        zero_pdata_payload(sizeof(DecalEmitterWatcherPdata), &pdata->hdr);
        ncs_pfx_decal_emitter_proc.proc = proc;
        ncs_pfx_decal_emitter_proc.instance = proc->instance;
        for (index = 0; index < 10; index++) {
            MKMatrixSetIdentity(&pdata->matrices[index]);
        }
    }
}

static float p_decal_emitter_watcher(void) {
    MkPfx* pfx;

    pfx = (MkPfx*)apdata;
    if (pfx == 0) {
        return -1.0f;
    }
    pfx->field_294 = 0;
    return 1.0f;
}

void reset_blood_decals(void) {
    int index;

    index = 0;
    while (blood_decal_to_reset[index] != 0) {
        unsigned int effect;

        effect = fx_by_owner(blood_decal_to_reset[index], 1);
        if (effect != 0) {
            fx_reset(effect);
        }
        effect = fx_by_owner(blood_decal_to_reset[index], 2);
        if (effect != 0) {
            fx_reset(effect);
        }
        index++;
    }

    memset(ncs_blood_splat_list, 0, sizeof(ncs_blood_splat_list));
    for (index = 0; index < BLOOD_SPLAT_COUNT; index++) {
        ncs_blood_splat_list[index].reset_height = -10000.0f;
    }
}

void bleed_startup(void) {
    MkProc* proc;
    int flags;

    flags = 0;
    proc = create_mkproc(
        0x30, get_mkproc_nostack(&flags), 0x5013, p_bleed, 0);
    if (proc != 0) {
        bleed_proc_item.proc = proc;
        bleed_proc_item.instance = proc->instance;
    }

    flags = 0;
    proc = create_mkproc(
        0x2E, get_mkproc_nostack(&flags), 0x5014, p_pfx_bleed, 0);
    if (proc != 0) {
        bleed_pfx_proc_item.proc = proc;
        bleed_pfx_proc_item.instance = proc->instance;
    }

    if (bleed_startup__fire_off_splat_watcher_func != 0) {
        start_blood_splat_watcher();
    }
}

void bleed_init(void) {
    int index;

    bleed_proc_item.proc = 0;
    bleed_proc_item.instance = 0;
    bleed_pfx_proc_item.proc = 0;
    bleed_pfx_proc_item.instance = 0;
    gusher_list = 0;
    bleed_startup__fire_off_splat_watcher_func = 1;

    memset(ncs_blood_splat_list, 0, sizeof(ncs_blood_splat_list));
    for (index = 0; index < BLOOD_SPLAT_COUNT; index++) {
        ncs_blood_splat_list[index].reset_height = -10000.0f;
    }
}

static float p_pfx_bleed(void) {
    apply_to_mklist(do_pfx_bleed, &aproc->pdata_list);
    return 1.0f;
}

static void bloodfx_init(BloodFxUserdata* userdata) {
    userdata->flags_40_bits.bit4 = 1;
    userdata->flags_40_bits.bit3 = 1;
}
