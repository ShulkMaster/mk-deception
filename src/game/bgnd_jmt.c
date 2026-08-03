#include "game/game_info.h"
#include "platform/main.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/shadow.h"
#include "runtime/utils.h"

typedef struct MkProcDestroyVtable {
    void* reserved[4];
    int (*destroy)(MkProc* proc);
} MkProcDestroyVtable;

typedef struct BgndDamageState {
    float wait_ticks;       /* +0x00 */
    int cliff_index;        /* +0x04 */
    int attempt_count;      /* +0x08 */
    int field_0C;
    int valid;              /* +0x10 - last triggered cliff index */
    int trigger_tick;       /* +0x14 */
    char pad18[0x14];
} BgndDamageState; /* 0x2C */

typedef struct BgndDamagePdata {
    MkHdr hdr;
    int watcher_round; /* +0x08 */
    BgndDamageState state; /* +0x0C */
} BgndDamagePdata;

typedef union BgndDamagePdataRef {
    MkHdr* hdr;
    BgndDamagePdata* damage;
} BgndDamagePdataRef;

typedef struct RopeProcLatch {
    MkProc* proc;
    unsigned int instance;
} RopeProcLatch;

typedef struct BgndUpdateSlot {
    int blend_divisor; /* +0x00 */
    int blend_numerator; /* +0x04 */
    char pad08[0x10];
    float shadow_scale; /* +0x18 */
    char pad1C[0x08];
    float speed; /* +0x24 */
    char pad28[0x08];
    float fall_acceleration; /* +0x30 */
    char pad34[0x08];
    Vec direction; /* +0x3C */
    char pad48[0x08];
} BgndUpdateSlot; /* 0x50 */

typedef struct BgndUpdateData BgndUpdateData;
typedef int (*BgndUpdateDestroyFn)(BgndUpdateData* update);

typedef struct BgndUpdateVtable {
    void* reserved[4];
    BgndUpdateDestroyFn destroy;
} BgndUpdateVtable;

struct BgndUpdateData {
    BgndUpdateVtable* vtbl;
    unsigned int instance;
    char pad08[4];
    float target_y; /* +0x0C */
    char pad10[4];
    MkSobj* object; /* +0x14 */
    char pad18[0x18];
    BgndUpdateSlot slots[1]; /* +0x30 */
};

static const float update_seconds_per_frame = 1.0f / 60.0f;

extern MkObj* g_bgnd_preloaded_models[];
extern RopeProcLatch rope_proc_item;
extern RopeProcLatch sobj_ctrl_proc_item;
extern int exec_tick_ctr;
extern int g_delay_rnd;
extern int g_ticks_delay;

void rope_controller_init(MkHdr* pdata, MkObj* model);
void rope_controller_update(MkHdr* pdata);
float p_watch_shadow(void);
float p_watch_cliffs(void);
float p_obj_ctrl(void);
float p_rope(void);
MkSobj* bgnd_fetch_sobj(int model_index, int object_id);
void insert_obj_ctrl_section(MkSobj* object, int section);
void bgnd_start_script_in_proc(int proc_id, int function_index);

void start_cliff_watcher(float wait_ticks) {
    BgndDamagePdataRef pdata;
    MkProc* proc;

    pdata.hdr = 0;
    if (find_mkproc_pid(0xB010) != 0) {
        return;
    }

    proc = _create_mkproc_generic_tinystack(
        0xB010, 0x1F, p_watch_cliffs, 0x38, &pdata.hdr);
    if (proc == 0 || pdata.hdr == 0) {
        return;
    }

    if (g_game_info.bgnd_obj != 0) {
        mk_insert((MkHdr*)proc, &g_game_info.bgnd_obj->child_list);
    }

    memset(&pdata.damage->state, 0, sizeof(pdata.damage->state));
    pdata.damage->state.wait_ticks = wait_ticks;
    pdata.damage->watcher_round = -1;
}

void set_cliff_watcher_round(int round) {
    MkProc* proc;
    BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (BgndDamagePdata*)pdata_of_proc(proc);
        if (pdata == 0) {
            pdata = 0;
        }
    }
    if (pdata != 0) {
        pdata->watcher_round = round;
    }
}

int get_cliff_watcher_round(void) {
    MkProc* proc;
    BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (BgndDamagePdata*)pdata_of_proc(proc);
        if (pdata == 0) {
            pdata = 0;
        }
    }
    if (pdata == 0) {
        return 0;
    }
    return pdata->watcher_round;
}

void clear_cliff_data(void) {
    MkProc* proc;
    BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (BgndDamagePdata*)pdata_of_proc(proc);
        if (pdata == 0) {
            pdata = 0;
        }
    }
    if (pdata != 0) {
        memset(&pdata->state, 0, sizeof(pdata->state));
    }
}

BgndDamageState* get_cliff_data(void) {
    MkProc* proc;
    BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (BgndDamagePdata*)pdata_of_proc(proc);
        if (pdata == 0) {
            pdata = 0;
        }
    }
    if (pdata == 0) {
        return 0;
    }
    return &pdata->state;
}

int check_damage_valid_fc(void) {
    BgndDamagePdataRef pdata;
    BgndDamageState* damage;
    MkProc* proc;

    if (g_game_info.bgnd_id != 6) {
        return 0;
    }

    proc = find_mkproc_pid(0xB010);
    pdata.hdr = proc != 0 ? pdata_of_proc(proc) : 0;
    damage = pdata.hdr != 0 ? &pdata.damage->state : 0;
    if (damage != 0 && damage->valid != 0) {
        return 1;
    }
    return 0;
}

int get_exec_tick_ctr(void) {
    return exec_tick_ctr;
}

/* Soft ceiling: can_fallingcliff_fall ~90.60% -- flag-load coloring and bool emit. */
int can_fallingcliff_fall(void) {
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        return 0;
    }
    if ((g_game_info.flags & 0x20) == 0) {
        return 0;
    }
    if ((g_game_info.flags & 1) == 1) {
        return 0;
    }
    if (g_game_info.pause_flag_bits.fatality_window == 1) {
        return 0;
    }
    if (g_game_info.plyr0.field_0C == 0.0f ||
        g_game_info.plyr1.field_0C == 0.0f) {
        return 0;
    }
    return are_death_traps_on() == 1;
}

void start_shadow_watcher(void) {
    MkHdr* pdata;
    MkProc* proc;

    pdata = 0;
    proc = _create_mkproc_generic_tinystack(
        0xB00C, 0x1F, p_watch_shadow, 8, &pdata);
    if (proc != 0 && pdata != 0 && g_game_info.bgnd_obj != 0) {
        mk_insert((MkHdr*)proc, &g_game_info.bgnd_obj->child_list);
    }
}

/* Soft ceiling: p_watch_shadow ~98.96% -- float-pool labels only. */
float p_watch_shadow(void) {
    Vec angles;
    float height;
    float blend;

    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) {
        return 0.0f;
    }

    angles.x = 0.0f;
    angles.y = 0.0f;
    angles.z = 0.0f;
    height = 0.5f *
        (g_game_info.plyr0.slot.mirror_a->pos.z +
         g_game_info.plyr1.slot.mirror_a->pos.z);
    if (height < 0.0f) {
        height = 0.0f;
    }
    if (height > 4.5f) {
        height = 4.5f;
    }
    blend = height / 4.5f;
    angles.x =
        1.5707964f * blend + 0.7853982f * (1.0f - blend);
    UpdateShadowCameraLightSource(&angles.x);
    return 0.0f;
}

/*
 * Soft ceiling: p_watch_cliffs ~90.33% -- flag-load coloring and script
 * destructor call scheduling.
 */
float p_watch_cliffs(void) {
    BgndDamagePdata* pdata;
    CmdScript* script;
    CmdScript* previous_script;
    int can_fall;

    pdata = (BgndDamagePdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    if ((g_game_info.flags & 0x08) != 0) {
        return 1.0f;
    }
    if (pdata->state.cliff_index >= 5) {
        return 1.0f;
    }

    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        can_fall = 0;
    } else if ((g_game_info.flags & 0x20) == 0) {
        can_fall = 0;
    } else if ((g_game_info.flags & 1) == 1) {
        can_fall = 0;
    } else if (g_game_info.pause_flag_bits.fatality_window == 1) {
        can_fall = 0;
    } else if (g_game_info.plyr0.field_0C == 0.0f ||
               g_game_info.plyr1.field_0C == 0.0f) {
        can_fall = 0;
    } else if (are_death_traps_on() == 0) {
        can_fall = 0;
    } else {
        can_fall = 1;
    }
    if (can_fall == 0) {
        pdata->state.wait_ticks = 300.0f;
        return 1.0f;
    }

    pdata->state.wait_ticks -= game_speed;
    if (pdata->state.wait_ticks > 0.0f) {
        return 1.0f;
    }
    pdata->state.wait_ticks = 300.0f;
    pdata->state.attempt_count++;
    if (pdata->state.attempt_count > 2 ||
        pdata->state.cliff_index == 1) {
        pdata->state.valid = pdata->state.cliff_index;
        pdata->state.trigger_tick = exec_tick_ctr;
        bgnd_start_script_in_proc(0xB008, 0x29);
        pdata->state.attempt_count = 0;
    } else {
        script = alloc_cmdscript();
        previous_script = active_cmdscript;
        active_cmdscript = script;
        cmdscript_setup_execution(g_game_info.cmdscript, 0x25);
        cmdscript_execute(g_game_info.cmdscript);
        active_cmdscript = previous_script;
        if (script->instance != 0) {
            ((int (*)(CmdScript*, void*))script->vtbl->destroy)(
                script, script->vtbl);
        }
    }
    return 1.0f;
}

void mks_set_update_delay(int ticks, int random_ticks) {
    g_delay_rnd = random_ticks;
    g_ticks_delay = ticks;
}

void mks_set_rotate_update_by_group(void) {
}

/* Soft ceiling: start_sobj_ctrl_proc ~89.05% -- spawn-argument scheduling only. */
void start_sobj_ctrl_proc(void) {
    int flags[2];
    MkProc* proc;

    if (sobj_ctrl_proc_item.proc != 0) {
        return;
    }
    flags[0] = 0;
    flags[1] = 0;
    proc = create_mkproc(
        0x1F, get_mkproc_nostack(&flags[0]), 0xB007, p_obj_ctrl, 0);
    if (proc != 0) {
        g_delay_rnd = 0;
        g_ticks_delay = 0;
        sobj_ctrl_proc_item.proc = proc;
        sobj_ctrl_proc_item.instance = proc->instance;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert((MkHdr*)proc, &g_game_info.bgnd_obj->child_list);
        }
    }
}

/* Soft ceiling: destroy_sobj_ctrl_proc ~96.56% -- latch branch layout only. */
void destroy_sobj_ctrl_proc(void) {
    MkProc* proc;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if ((unsigned int)proc->instance ==
            sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc != 0 && (unsigned int)proc->instance != 0) {
        ((MkProcDestroyVtable*)proc->vtbl)->destroy(proc);
    }
    sobj_ctrl_proc_item.proc = 0;
    sobj_ctrl_proc_item.instance = 0;
}

void bgnd_obj_insert_obj_ctrl_section(int model_index, int section) {
    MkSobj* object;

    if (g_bgnd_preloaded_models[model_index] != 0) {
        object = bgnd_fetch_sobj(model_index, 0);
        if (object != 0) {
            insert_obj_ctrl_section(object, section);
        }
    }
}

void bgnd_insert_obj_ctrl_section(int object_id, int section) {
    MkSobj* object;

    if (g_game_info.bgnd_obj != 0) {
        object = (MkSobj*)obj_find_sobj_by_id(
            g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            insert_obj_ctrl_section(object, section);
        }
    }
}

void update_func_blend_start(BgndUpdateData* update, int index) {
    BgndUpdateSlot* slot;
    MkSobj* object;
    float blend;

    object = update->object;
    if (object == 0) {
        return;
    }

    slot = &update->slots[index];
    if (slot->blend_divisor <= 0) {
        object->pos.y = update->target_y;
        return;
    }

    blend =
        (float)slot->blend_numerator / (float)slot->blend_divisor;
    object->pos.y =
        object->pos.y * blend + update->target_y * (1.0f - blend);
}

void update_func_shadow_scale(BgndUpdateData* update, int index) {
    BgndUpdateSlot* slot;
    MkSobj* object;

    object = update->object;
    slot = &update->slots[index];
    if (object == 0 || slot->blend_divisor <= 0) {
        if (object != 0) {
            hide_sobj(object);
        }
        if (update->instance != 0) {
            update->vtbl->destroy(update);
        }
        return;
    }

    object->flags_08 |= 0x02;
    object->scale.x = 1.0f;
    object->scale.y = 1.0f;
    object->scale.z = slot->shadow_scale;
}

void update_func_awayxz(BgndUpdateData* update, int index) {
    MkSobj* object;
    float distance;
    Vec delta;

    object = update->object;
    if (object == 0) {
        return;
    }

    distance = update_seconds_per_frame * update->slots[index].speed;
    delta.x = update->slots[index].direction.x * distance;
    delta.y = 0.0f;
    delta.z = update->slots[index].direction.z * distance;
    object->pos.x += delta.x;
    object->pos.y += delta.y;
    object->pos.z += delta.z;
}

void update_func_fall(BgndUpdateData* update, int index) {
    BgndUpdateSlot* slot;
    MkSobj* object;

    object = update->object;
    if (object == 0) {
        return;
    }

    slot = &update->slots[index];
    object->pos.x += update_seconds_per_frame * slot->direction.x;
    object->pos.y += update_seconds_per_frame * slot->direction.y;
    object->pos.z += update_seconds_per_frame * slot->direction.z;
    slot->direction.y +=
        update_seconds_per_frame * slot->fall_acceleration;
}

void bgnd_preload_obj_attach_rope(int model_index) {
    MkHdr* rope_pdata;
    MkObj* model;
    MkProc* rope_proc;

    model = g_bgnd_preloaded_models[model_index];
    if (model == 0) {
        return;
    }

    rope_proc = rope_proc_item.proc;
    if (rope_proc != 0 &&
        rope_proc->instance != rope_proc_item.instance) {
        rope_proc = 0;
    }
    if (rope_proc == 0) {
        return;
    }

    rope_pdata = get_mkpdata_generic(0x1A0);
    if (rope_pdata != 0) {
        mk_insert(rope_pdata, &rope_proc->pdata_list);
        rope_controller_init(rope_pdata, model);
    }
}

/* Soft ceiling: start_rope_proc ~88.14% -- spawn-argument scheduling only. */
void start_rope_proc(void) {
    int flags[2];
    MkProc* proc;

    if (rope_proc_item.proc != 0) {
        return;
    }
    flags[0] = 0;
    flags[1] = 0;
    proc = create_mkproc(
        0x16, get_mkproc_nostack(&flags[0]), 0xB003, p_rope, 0);
    if (proc != 0) {
        rope_proc_item.proc = proc;
        rope_proc_item.instance = proc->instance;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert((MkHdr*)proc, &g_game_info.bgnd_obj->child_list);
        }
    }
}

/* Soft ceiling: p_rope ~99.64% -- float-pool label only. */
float p_rope(void) {
    while (apdata != 0) {
        rope_controller_update(apdata);
        next_apdata();
    }
    return 1.0f;
}
