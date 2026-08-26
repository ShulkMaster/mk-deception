#include "game/game_info.h"
#include "math/gxMat.h"
#include "math/gxMath.h"
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

typedef struct BgndUpdateData BgndUpdateData;
struct BgndUpdateVtable;
typedef void (*BgndSlotUpdateFn)(BgndUpdateData* update, int index);

typedef struct BgndUpdateSlot {
    int blend_divisor; /* +0x00 */
    int blend_numerator; /* +0x04 */
    BgndSlotUpdateFn update_fn; /* +0x08 */
    int enabled; /* +0x0C */
    float start_value; /* +0x10 */
    float end_value; /* +0x14 */
    float shadow_scale; /* +0x18 */
    float initial_speed; /* +0x1C */
    float speed_param; /* +0x20 */
    float speed; /* +0x24 */
    float sin_rate; /* +0x28 */
    float sin_phase; /* +0x2C */
    float fall_acceleration; /* +0x30 */
    float field_34;
    float field_38;
    Vec direction; /* +0x3C */
    char pad48[0x08];
} BgndUpdateSlot; /* 0x50 */

typedef int (*BgndUpdateDestroyFn)(
    BgndUpdateData* update, struct BgndUpdateVtable* vtable);

typedef struct BgndUpdateVtable {
    void* reserved[4];
    BgndUpdateDestroyFn destroy;
} BgndUpdateVtable;

struct BgndUpdateData {
    BgndUpdateVtable* vtbl;
    unsigned int instance;
    Vec origin; /* +0x08 */
    MkSobj* object; /* +0x14 */
    float origin_length; /* +0x18 */
    int group_id;    /* +0x1C */
    int remove_hide; /* +0x20 */
    int active_slot; /* +0x24 */
    char pad28[0x08];
    BgndUpdateSlot slots[1]; /* +0x30 */
};

/* A command prefix begins 0x50 bytes apart; its slot payload starts at +0x30. */
typedef struct BgndUpdateCommandBlock {
    char pad00[0x28];
    int delay;       /* +0x28 */
    int slot_index;  /* +0x2C */
    BgndUpdateSlot slot; /* +0x30 */
} BgndUpdateCommandBlock;

typedef struct RopeSegment {
    Vec velocity; /* +0x00 */
    float pad0C;
    Vec span; /* +0x10 */
    float pad1C;
    Vec offset; /* +0x20 */
    float pad2C;
    float length_scale; /* +0x30 */
    float damping; /* +0x34 */
    float field_38;
    float field_3C;
    float inverse_length_scale; /* +0x40 */
    int mode; /* +0x44 */
    float field_48;
    int bone_tag; /* +0x4C */
    MkBone* bone; /* +0x50 */
    char pad54[0x0C];
} RopeSegment; /* 0x60 */

typedef struct RopeInfo {
    int bone_tag;
    int mode;
    float field_08;
    float inverse_length_scale;
    float field_10;
    float field_14;
} RopeInfo;

typedef struct RopeControllerData {
    MkHdr hdr;
    MkObj* model; /* +0x08 */
    int segment_count; /* +0x0C */
    RopeSegment segments[3]; /* +0x10 */
    char pad130[0x60];
    float damping; /* +0x190 */
    MkObj* attached_model; /* +0x194 */
    int attached_object_id; /* +0x198 */
} RopeControllerData;

static const float update_seconds_per_frame = 1.0f / 60.0f;

static int rope_bones[] = {0x2001, 0x2002, 0x2003};
RopeInfo g_rope_info[] = {
    {0x2001, 1, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x2002, 2, 0.2f, 0.05f, 0.1f, 0.95f},
    {0x2003, 2, 0.2f, 0.05f, 0.1f, 0.95f},
};
static int n_rope_info = 3;

static inline float bgnd_inv_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } guess;
    float product;
    float correction;

    if (value <= 0.0f) {
        return 0.0f;
    }
    guess.f = value;
    guess.u = 0x5F375A00U - (guess.u >> 1);
    product = guess.f * (value * guess.f);
    correction = 3.0f - product;
    return 0.0625f * guess.f * correction *
           -(correction * (product * correction) - 12.0f);
}

static inline float bgnd_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;
    float refined;

    if (value <= 0.0f) {
        return 0.0f;
    }
    input.f = value;
    guess.u =
        (unsigned int)GXMathSqrtTable[(input.u >> 10) & 0x3FFE] << 8;
    guess.u |=
        (((input.u & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    refined = guess.f * (3.0f - (guess.f * guess.f) / value);
    return 0.5f * refined;
}

extern MkObj* g_bgnd_preloaded_models[];
extern int exec_tick_ctr;
RopeProcLatch rope_proc_item;
RopeProcLatch sobj_ctrl_proc_item;
int g_ticks_delay;
int g_delay_rnd;


void build_bones_tbl(MkObj* object, const int* tags);
void update_bone_hierarchy(void* object);
void* get_bone_with_tag(void* object, int tag);

static void rope_controller_init(MkHdr* pdata, MkObj* model);
static void rope_controller_update(MkHdr* pdata);
static float p_watch_shadow(void);
static float p_watch_cliffs(void);
static float p_obj_ctrl(void);
static float p_rope(void);
static void update_func_shadow_scale(BgndUpdateData* update, int index);
static void update_func_blend_start(BgndUpdateData* update, int index);
static void update_func_fall(BgndUpdateData* update, int index);
static void update_func_awayxz(BgndUpdateData* update, int index);
static void update_func_sin(BgndUpdateData* update, int index);
MkSobj* bgnd_fetch_sobj(int model_index, int object_id);
static void insert_obj_ctrl_section(MkSobj* object, int section);
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
    if (proc == 0) {
        return;
    }
    if (pdata.hdr == 0) {
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

/*
 * Soft ceiling: p_watch_cliffs ~90.33% -- flag-load coloring and script
 * destructor call scheduling.
 */
static float p_watch_cliffs(void) {
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

int check_damage_valid_fc(void) {
    BgndDamagePdataRef pdata;
    BgndDamageState* damage;
    MkProc* proc;

    if (g_game_info.bgnd_id == 6) {
        proc = find_mkproc_pid(0xB010);
        if (proc == 0) {
            pdata.hdr = 0;
        } else {
            pdata.hdr = pdata_of_proc(proc);
            if (pdata.hdr == 0) {
                pdata.hdr = 0;
            }
        }
        if (pdata.hdr == 0) {
            damage = 0;
        } else {
            damage = &pdata.damage->state;
        }
        if (damage != 0 && damage->valid != 0) {
            return 1;
        }
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
static float p_watch_shadow(void) {
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
        (g_game_info.plyr0.slot.mirror_a->pos.value.z +
         g_game_info.plyr1.slot.mirror_a->pos.value.z);
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


void mks_set_update_delay(int ticks, int random_ticks) {
    g_delay_rnd = random_ticks;
    g_ticks_delay = ticks;
}

static inline int bgnd_list_is_valid(MkPtr** list) {
    return list != 0;
}

/* Soft ceiling: 97.76% -- validated-process latch branch layout only. */
void mks_removehide_by_group(int group_id, int remove_hide) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!bgnd_list_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                update->remove_hide = remove_hide;
            }
            link = link->next;
        }
    }
}

/* Soft ceiling: 97.11% -- validation-latch and list-loop GPR coloring only. */
void mks_shadow_scale(int group_id, int blend_ticks,
                      float start_scale, float end_scale) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateCommandBlock* previous;
    int slot_index;
    int previous_index;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!bgnd_list_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id) {
                slot_index = update->active_slot;
                command = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                    slot_index * 0x50);
                command->slot.blend_numerator = blend_ticks;
                command->slot.blend_divisor = blend_ticks;
                command->delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                command->slot.field_34 = 0.0f;
                command->slot.update_fn = update_func_shadow_scale;
                command->slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
                previous_index = update->active_slot - 1;
                if (previous_index < 0) {
                    previous_index = 1;
                }
                previous = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                     previous_index * 0x50);
                previous->slot.shadow_scale = start_scale;
                previous->slot.start_value = start_scale;
                previous->slot.end_value = end_scale;
                previous->slot.enabled = 1;
            }
            link = link->next;
        }
    }
}

/* Soft ceiling: 96.51% -- validation-latch and list-loop GPR coloring only. */
void mks_blend_start_update_by_group(int group_id, int blend_ticks) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    int slot_index;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!bgnd_list_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = update->active_slot;
                command = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                    slot_index * 0x50);
                command->slot.blend_numerator = blend_ticks;
                command->slot.blend_divisor = blend_ticks;
                command->delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                command->slot.field_34 = 0.0f;
                command->slot.update_fn = update_func_blend_start;
                command->slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
            }
            link = link->next;
        }
    }
}

/* Soft ceiling: 97.41% -- validation-latch and list-loop GPR coloring only. */
void mks_gravity_update_by_group(int group_id, int blend_ticks,
                                 float velocity_x, float velocity_y,
                                 float velocity_z, float gravity) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateCommandBlock* previous;
    int slot_index;
    int previous_index;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!bgnd_list_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = update->active_slot;
                command = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                    slot_index * 0x50);
                command->slot.blend_numerator = blend_ticks;
                command->slot.blend_divisor = blend_ticks;
                command->delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                command->slot.field_34 = 0.0f;
                command->slot.update_fn = update_func_fall;
                command->slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
                previous_index = update->active_slot - 1;
                if (previous_index < 0) {
                    previous_index = 1;
                }
                previous = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                     previous_index * 0x50);
                previous->slot.fall_acceleration = gravity;
                previous->slot.direction.x = velocity_x;
                previous->slot.direction.y = velocity_y;
                previous->slot.direction.z = velocity_z;
            }
            link = link->next;
        }
    }
}

/* Soft ceiling: 97.86% -- validated-process latch and GPR coloring only. */
void mks_away_vel_update_by_group(int group_id, int blend_ticks,
                                  float speed, float speed_param,
                                  float random_range) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateCommandBlock* previous;
    MkSobj* object;
    float inverse_length;
    float varied_speed;
    float position_x;
    float position_y;
    float position_z;
    int slot_index;
    int previous_index;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!bgnd_list_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = update->active_slot;
                command = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                    slot_index * 0x50);
                command->slot.blend_numerator = blend_ticks;
                command->slot.blend_divisor = blend_ticks;
                command->delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                command->slot.field_34 = 0.0f;
                command->slot.update_fn = update_func_awayxz;
                command->slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
                previous_index = update->active_slot - 1;
                if (previous_index < 0) {
                    previous_index = 1;
                }
                previous = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                     previous_index * 0x50);
                varied_speed = speed +
                               (frand(random_range) - 0.5f * random_range);
                previous->slot.speed = varied_speed;
                previous->slot.initial_speed = varied_speed;
                previous->slot.speed_param = speed_param;

                object = update->object;
                position_y = object->pos.y;
                position_x = object->pos.x;
                position_z = object->pos.z;
                inverse_length = bgnd_inv_sqrt(
                    position_z * position_z +
                    (position_x * position_x + position_y * position_y));
                previous->slot.direction.x = position_x * inverse_length;
                previous->slot.direction.y = position_y * inverse_length;
                previous->slot.direction.z = position_z * inverse_length;
            }
            link = link->next;
        }
    }
}

void mks_set_rotate_update_by_group(void) {
}

/* Soft ceiling: 96.83% -- validation-latch and list-loop GPR coloring only. */
void mks_set_sin_update_by_group(int group_id, int blend_ticks,
                                 int update_flags, int extra_flags,
                                 float start_value, float end_value,
                                 float start_speed, float speed_param,
                                 float sin_rate, float sin_phase) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateCommandBlock* previous;
    int slot_index;
    int previous_index;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!bgnd_list_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = update->active_slot;
                command = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                    slot_index * 0x50);
                command->slot.blend_numerator = blend_ticks;
                command->slot.blend_divisor = blend_ticks;
                command->delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                command->slot.field_34 = 0.0f;
                command->slot.update_fn = update_func_sin;
                command->slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
                previous_index = update->active_slot - 1;
                if (previous_index < 0) {
                    previous_index = 1;
                }
                previous = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                     previous_index * 0x50);
                previous->slot.shadow_scale = start_value;
                previous->slot.start_value = start_value;
                previous->slot.end_value = end_value;
                previous->slot.speed = start_speed;
                previous->slot.initial_speed = start_speed;
                previous->slot.speed_param = speed_param;
                previous->slot.sin_rate = sin_rate;
                previous->slot.sin_phase = sin_phase;
                previous->slot.enabled = update_flags;
                previous->slot.enabled |= extra_flags;
            }
            link = link->next;
        }
    }
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

/*
 * Soft ceiling: 93.43% -- validated-process latch and equivalent inline
 * square-root lookup/temporary scheduling only.
 */
static void insert_obj_ctrl_section(MkSobj* object, int section) {
    MkProc* proc;
    MkHdr* pdata;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    float length_squared;
    int index;

    if (object == 0) {
        return;
    }

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == sobj_ctrl_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return;
    }

    pdata = get_mkpdata_generic(0xC8);
    if (pdata == 0) {
        return;
    }
    mk_insert(pdata, &proc->pdata_list);

    update = (BgndUpdateData*)pdata;
    update->object = object;
    update->group_id = section;
    object->flags_08_bits.bit6 = 1;
    update->origin.x = object->pos.x;
    update->origin.y = object->pos.y;
    update->origin.z = object->pos.z;
    length_squared = update->origin.z * update->origin.z +
                     (update->origin.x * update->origin.x +
                      update->origin.y * update->origin.y);
    update->origin_length = length_squared;
    update->origin_length = bgnd_sqrt(update->origin_length);
    update->remove_hide = -1;
    update->active_slot = 0;

    for (index = 0; index < 2; index++) {
        command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
        command->slot.field_34 = 0.0f;
        command->slot.field_38 = 0.0f;
        command->slot.direction.z = 0.0f;
        command->slot.direction.y = 0.0f;
        command->slot.direction.x = 0.0f;
        command->slot.blend_divisor = 0;
        command->slot.blend_numerator = 0;
        command->slot.update_fn = 0;
        command->slot.enabled = 0;
        command->slot.start_value = 0.0f;
        command->slot.end_value = 0.0f;
        command->slot.shadow_scale = 0.0f;
        command->slot.sin_phase = 0.0f;
        command->slot.sin_rate = 0.0f;
        command->delay = 0;
        command->slot.fall_acceleration = 0.0f;
    }
}

/* Soft ceiling: 99.12% -- GPR coloring and li-zero versus mr-zero only. */
static float p_obj_ctrl(void) {
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateSlot* slot;
    float blend;
    int index;

    while (apdata != 0) {
        update = (BgndUpdateData*)apdata;
        if (update->object != 0) {
            if (update->remove_hide >= 0) {
                update->remove_hide--;
                if (update->remove_hide < 0) {
                    update->remove_hide = -1;
                    if (update->object != 0) {
                        hide_sobj(update->object);
                    }
                    if (update->instance != 0) {
                        update->vtbl->destroy(update, update->vtbl);
                    }
                }
            }

            for (index = 0; index < 2; index++) {
                command = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                                    index * 0x50);
                slot = &command->slot;
                if (slot->update_fn != 0) {
                    if (command->delay > 0) {
                        command->delay--;
                    } else {
                        slot->update_fn(update, index);
                        if (slot->blend_divisor != -1) {
                            if (slot->enabled != 0 &&
                                (slot->enabled & 1) != 0) {
                                if (slot->blend_divisor == 0) {
                                    slot->shadow_scale = 0.0f;
                                } else {
                                    blend = (float)slot->blend_numerator /
                                            (float)slot->blend_divisor;
                                    slot->shadow_scale =
                                        slot->start_value * blend +
                                        slot->end_value * (1.0f - blend);
                                }
                            }

                            if (slot->blend_divisor == 0) {
                                slot->speed = slot->speed_param;
                            } else {
                                blend = (float)slot->blend_numerator /
                                        (float)slot->blend_divisor;
                                slot->speed =
                                    slot->initial_speed * blend +
                                    slot->speed_param * (1.0f - blend);
                            }
                            slot->blend_numerator--;
                            if (slot->blend_numerator < 0) {
                                slot->blend_numerator = 0;
                                slot->update_fn = 0;
                            }
                        }
                    }
                }
            }
        }
        next_apdata();
    }
    return 1.0f;
}

static void update_func_shadow_scale(BgndUpdateData* update, int index) {
    BgndUpdateCommandBlock* command;
    MkSobj* object;
    float shadow_scale;

    object = update->object;
    if (object == 0 ||
        (command = (BgndUpdateCommandBlock*)((unsigned char*)update +
                                              index * 0x50),
         command->slot.blend_divisor <= 0)) {
        if (object != 0) {
            hide_sobj(object);
        }
        if (update->instance != 0) {
            update->vtbl->destroy(update, update->vtbl);
        }
        return;
    }

    shadow_scale = command->slot.shadow_scale;
    object->flags_08_bits.scale_dirty = 1;
    object->scale.x = 1.0f;
    object->scale.y = 1.0f;
    object->scale.z = shadow_scale;
}

/* Soft ceiling: 91.42% -- equivalent blend-expression scheduling/FPR coloring. */
static void update_func_blend_start(BgndUpdateData* update, int index) {
    BgndUpdateSlot* slot;
    MkSobj* object;
    float blend;

    object = update->object;
    if (object == 0) {
        return;
    }

    slot = &update->slots[index];
    if (slot->blend_divisor <= 0) {
        object->pos.y = update->origin.y;
        return;
    }

    blend =
        (float)slot->blend_numerator / (float)slot->blend_divisor;
    object->pos.y =
        object->pos.y * blend + update->origin.y * (1.0f - blend);
}

static void update_func_awayxz(BgndUpdateData* update, int index) {
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

static void update_func_fall(BgndUpdateData* update, int index) {
    BgndUpdateCommandBlock* command;
    MkSobj* object;
    Vec movement;

    object = update->object;
    if (object == 0) {
        return;
    }

    command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
    movement.x = update_seconds_per_frame * command->slot.direction.x;
    movement.y = update_seconds_per_frame * command->slot.direction.y;
    movement.z = update_seconds_per_frame * command->slot.direction.z;
    object->pos.x += movement.x;
    object->pos.y += movement.y;
    object->pos.z += movement.z;
    command->slot.direction.y +=
        update_seconds_per_frame * command->slot.fall_acceleration;
}

/* Soft ceiling: 98.91% -- FPR coloring and float-pool labels only. */
static void update_func_sin(BgndUpdateData* update, int index) {
    MkSobj* object;
    float frame_time;
    float base_angle;
    float random_offset;
    float sine;
    float value;

    object = update->object;
    if (object == 0) {
        return;
    }

    frame_time = (float)(update->slots[index].blend_divisor -
                         update->slots[index].blend_numerator) / 60.0f;

    if ((update->slots[index].enabled & 4) != 0) {
        random_offset = frand(update->slots[index].sin_phase);
        base_angle = update->origin_length *
                     (update->slots[index].sin_rate + random_offset);
    } else if ((update->slots[index].enabled & 8) != 0) {
        random_offset = frand(update->slots[index].sin_phase);
        base_angle = object->pos.x *
                     (update->slots[index].sin_rate + random_offset);
    } else if ((update->slots[index].enabled & 0x20) != 0) {
        base_angle = gxMathArcTanYX(update->origin.x, update->origin.z);
        random_offset = frand(update->slots[index].sin_phase);
        base_angle *= update->slots[index].sin_rate + random_offset;
    } else {
        random_offset = frand(update->slots[index].sin_phase);
        base_angle = object->pos.x *
                     (update->slots[index].sin_rate + random_offset);
    }

    sine = gxMathSin(
        6.28f * update->slots[index].speed * frame_time + base_angle);
    value = update->slots[index].shadow_scale * sine;
    object->pos.y += value - update->slots[index].field_34;
    update->slots[index].field_34 = value;
}

/* Soft ceiling: 94.51% -- validated-process latch and list-loop branch layout. */
void bgnd_detach_rope(int model_index) {
    MkObj* model;
    MkProc* proc;
    RopeControllerData* rope;

    model = g_bgnd_preloaded_models[model_index];
    if (model == 0) {
        return;
    }

    proc = rope_proc_item.proc;
    if (proc != 0) {
        if (proc->instance == rope_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }

    if (proc == 0) {
        rope = 0;
    } else {
        MkPtr* iterator;

        iterator = first_mkptr(&proc->pdata_list);
        if (iterator == 0) {
            rope = 0;
        } else {
            rope = (RopeControllerData*)iterator->hdr;
            while (rope != 0) {
                if (rope->model == model) {
                    break;
                }
                iterator = next_mkptr(iterator);
                if (iterator == 0) {
                    rope = 0;
                } else {
                    rope = (RopeControllerData*)iterator->hdr;
                }
            }
        }
    }

    if (rope != 0) {
        rope->segments[rope->segment_count - 1].mode = 2;
        rope->attached_model = 0;
        rope->attached_object_id = 0;
    }
}

/* Soft ceiling: 96.06% -- validated-process latch and list-loop branch layout. */
void bgnd_rope_adjust_length(int model_index, int preserve_shape, float length) {
    MkObj* model;
    MkProc* proc;
    RopeControllerData* rope;

    model = g_bgnd_preloaded_models[model_index];
    if (model == 0) {
        return;
    }

    proc = rope_proc_item.proc;
    if (proc != 0) {
        if (proc->instance != rope_proc_item.instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }

    if (proc == 0) {
        rope = 0;
    } else {
        MkPtr* iterator;

        iterator = first_mkptr(&proc->pdata_list);
        if (iterator == 0) {
            rope = 0;
        } else {
            rope = (RopeControllerData*)iterator->hdr;
            while (rope != 0) {
                if (rope->model == model) {
                    break;
                }
                iterator = next_mkptr(iterator);
                if (iterator == 0) {
                    rope = 0;
                } else {
                    rope = (RopeControllerData*)iterator->hdr;
                }
            }
        }
    }

    if (rope != 0) {
        float scale;
        int i;

        scale = 0.7f * length;
        for (i = 0; i < rope->segment_count; i++) {
            RopeSegment* segment;

            segment = &rope->segments[i];
            segment->length_scale *= scale / 10.0f;
            if (preserve_shape == 1 && scale > 0.0f) {
                segment->inverse_length_scale *= 10.0f / scale;
            }
        }
    }
}

/*
 * Soft ceiling: 93.55% -- defensive-guard, validation-latch, and loop keep-edge
 * branch layout only.
 */
void bgnd_attach_rope_to_bgnd_obj(
    int rope_model_index, int target_model_index, int object_id) {
    MkObj* target_model;
    MkObj* rope_model;
    MkProc* proc;
    RopeControllerData* rope;

    rope_model = g_bgnd_preloaded_models[rope_model_index];
    if (rope_model == 0) {
        return;
    }
    target_model = g_bgnd_preloaded_models[target_model_index];
    if (target_model == 0) {
        return;
    }
    if (rope_model == 0 || target_model == 0) {
        return;
    }

    proc = rope_proc_item.proc;
    if (proc != 0) {
        if (proc->instance != rope_proc_item.instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }

    if (proc == 0) {
        rope = 0;
    } else {
        MkPtr* iterator;

        iterator = first_mkptr(&proc->pdata_list);
        if (iterator == 0) {
            rope = 0;
        } else {
            rope = (RopeControllerData*)iterator->hdr;
            while (rope != 0) {
                if (rope->model == rope_model) {
                    break;
                }
                iterator = next_mkptr(iterator);
                if (iterator == 0) {
                    rope = 0;
                } else {
                    rope = (RopeControllerData*)iterator->hdr;
                }
            }
        }
    }

    if (rope != 0) {
        rope->segments[rope->segment_count - 1].mode = 3;
        rope->attached_model = target_model;
        rope->attached_object_id = object_id;
    }
}

/* Soft ceiling: 94.08% -- validated-process latch branch/GPR coloring only. */
void bgnd_preload_obj_attach_rope(int model_index) {
    MkHdr* rope_pdata;
    MkObj* model;
    MkProc* rope_proc;

    model = g_bgnd_preloaded_models[model_index];
    if (model == 0) {
        return;
    }

    rope_proc = rope_proc_item.proc;
    if (rope_proc != 0) {
        if (rope_proc->instance == rope_proc_item.instance) {
            /* Keep the validated process. */
        } else {
            rope_proc = 0;
        }
    } else {
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
static float p_rope(void) {
    while (apdata != 0) {
        rope_controller_update(apdata);
        next_apdata();
    }
    return 1.0f;
}

/*
 * Soft ceiling: 90.23% -- aligned-matrix call scheduling and register
 * allocation only; operations, accesses, loop, and frame alignment agree.
 */
static void rope_controller_init(MkHdr* pdata, MkObj* model) {
    RopeControllerData* rope;
    RopeInfo* info_base;
    int segment_count;
    int i;

    rope = (RopeControllerData*)pdata;
    rope->model = model;
    rope->attached_model = 0;
    rope->attached_object_id = 0;
    build_bones_tbl(model, rope_bones);
    segment_count = n_rope_info;
    info_base = g_rope_info;
    update_bone_hierarchy(model != 0 ? as_mkhdr(&model->hdr) : 0);

    rope->segment_count = segment_count;
    rope->damping = 0.975f;
    for (i = 0; i < rope->segment_count; i++) {
        RopeSegment* segment;
        RopeInfo* info;
        MkBone* bone;

        segment = &rope->segments[i];
        info = &info_base[i];
        segment->velocity.x = 0.0f;
        segment->velocity.y = 0.0f;
        segment->velocity.z = 0.0f;
        segment->span.x = 0.0f;
        segment->span.y = 0.0f;
        segment->span.z = 0.0f;
        segment->offset.x = 0.0f;
        segment->offset.y = 0.0f;
        segment->offset.z = 0.0f;
        segment->length_scale = 1.0f;
        segment->bone_tag = info->bone_tag;

        bone = (MkBone*)get_bone_with_tag(model, segment->bone_tag);
        if (bone == 0) {
            break;
        }

        segment->bone = bone;
        bone->flags_54_bits.calculation_locked = 1;
        gxMat33x33(
            (Mat33*)&bone->matrix, (Mat33*)bone->parent_matrix,
            (Mat33*)model->frame);
        gxMatV3MatAddV3(
            (Vec*)&bone->matrix.pos, (Vec*)&bone->parent_matrix->pos,
            (Mat33*)model->frame, (Vec*)&model->frame->modelling.pos);

        if (bone->transform_parent != 0) {
            MkBone* child;
            RwMatrix bone_matrix __attribute__((aligned(16)));
            RwMatrix child_matrix __attribute__((aligned(16)));

            child = bone->transform_parent;
            child->flags_54_bits.calculation_locked = 1;
            gxMat33x33(
                (Mat33*)&child->matrix, (Mat33*)child->parent_matrix,
                (Mat33*)model->frame);
            gxMatV3MatAddV3(
                (Vec*)&child->matrix.pos,
                (Vec*)&child->parent_matrix->pos, (Mat33*)model->frame,
                (Vec*)&model->frame->modelling.pos);

            if (bone->flags_54_bits.calculation_locked) {
                bone_matrix = bone->matrix;
            } else {
                gxMat33x33(
                    (Mat33*)&bone_matrix, (Mat33*)bone->parent_matrix,
                    (Mat33*)model->frame);
                gxMatV3MatAddV3(
                    (Vec*)&bone_matrix.pos,
                    (Vec*)&bone->parent_matrix->pos,
                    (Mat33*)model->frame,
                    (Vec*)&model->frame->modelling.pos);
            }
            if (child->flags_54_bits.calculation_locked) {
                child_matrix = child->matrix;
            } else {
                gxMat33x33(
                    (Mat33*)&child_matrix, (Mat33*)child->parent_matrix,
                    (Mat33*)model->frame);
                gxMatV3MatAddV3(
                    (Vec*)&child_matrix.pos,
                    (Vec*)&child->parent_matrix->pos,
                    (Mat33*)model->frame,
                    (Vec*)&model->frame->modelling.pos);
            }
            PSVECSubtract(
                (Vec*)&bone_matrix.pos, (Vec*)&child_matrix.pos,
                &segment->span);
            segment->length_scale = PSVECMag(&segment->span);
        }

        segment->damping = 0.75f;
        segment->mode = info->mode;
        segment->field_38 = info->field_10;
        segment->field_3C = info->field_14;
        segment->inverse_length_scale = info->inverse_length_scale;
        segment->field_48 = info->field_08;
    }
}

static inline void rope_update_bone_matrix(
    MkBone* bone, const RwMatrix* model_matrix) {
    RwMatrix* parent_matrix;

    parent_matrix = bone->parent_matrix;
    gxMat33x33(
        (Mat33*)&bone->matrix, (const Mat33*)parent_matrix,
        (const Mat33*)model_matrix);
    gxMatV3MatAddV3(
        &bone->matrix.pos_vec, &parent_matrix->pos_vec,
        (Mat33*)model_matrix, (Vec*)&model_matrix->pos_vec);
}

static inline void rope_set_identity(RwMatrix* matrix) {
    matrix->right.x = 1.0f;
    matrix->right.y = 0.0f;
    matrix->right.z = 0.0f;
    matrix->up.x = 0.0f;
    matrix->up.y = 1.0f;
    matrix->up.z = 0.0f;
    matrix->at.x = 0.0f;
    matrix->at.y = 0.0f;
    matrix->at.z = 1.0f;
    matrix->pos.x = 0.0f;
    matrix->pos.y = 0.0f;
    matrix->pos.z = 0.0f;
    matrix->flags |= 0x20003;
}

static inline void rope_point_bone_at(
    MkBone* bone, Vec* direction, RwMatrixPosition* axis,
    Quat* quaternion,
    RwMatrix* source, RwMatrix* rotation) {
    PSVECNormalize(direction, direction);
    *axis = *(RwMatrixPosition*)&bone->parent_matrix->up;
    PSVECNormalize(&axis->value, &axis->value);
    gxVectV3V3ToQuat(quaternion, &axis->value, direction);
    gxQuatQuatToMat((Mat33*)rotation, quaternion);
    *source = *bone->parent_matrix;
    gxMat33x33(
        (Mat33*)bone->parent_matrix, (const Mat33*)source,
        (const Mat33*)rotation);
}

static inline RopeSegment* rope_next_segment(
    RopeControllerData* rope, int index) {
    index++;
    if (index >= rope->segment_count) {
        return 0;
    }
    return &rope->segments[index];
}

static inline RopeSegment* rope_previous_segment(
    RopeControllerData* rope, int index) {
    index--;
    if (index < 0) {
        return 0;
    }
    return &rope->segments[index];
}

/*
 * Soft ceiling: retail aligns the matrix/vector workspace to 16 bytes. Clean
 * portable C uses the TU's natural stack layout, leaving stack offsets,
 * register allocation, and matrix/vector load scheduling as residue.
 */
static void rope_controller_update(MkHdr* pdata) {
    MkObj* model;
    RopeControllerData* rope;
    RwMatrix inverse_model_matrix;
    RwMatrix attached_matrix;
    RwMatrix rotation;
    RwMatrix source;
    Vec acceleration;
    Vec velocity_delta;
    RwMatrixPosition constraint_axis;
    Vec correction;
    Vec local_position;
    Vec attached_position;
    Quat quaternion;
    RwMatrixPosition axis;
    Vec direction;
    Vec midpoint;
    int i;

    rope = (RopeControllerData*)pdata;
    model = rope->model;
    if (model == 0) {
        return;
    }

    RwMatrixInvert(&inverse_model_matrix, &model->frame->modelling);

    for (i = 0; i < rope->segment_count; i++) {
        RopeSegment* segment;
        RopeSegment* next;
        MkBone* bone;

        segment = &rope->segments[i];
        bone = segment->bone;
        if (bone == 0) {
            continue;
        }

        if (segment->mode == 1) {
            if (bone->flags_54_bits.calculation_locked) {
                rope_update_bone_matrix(bone, &model->frame->modelling);
            }
            continue;
        }

        if (bone->transform_parent == 0) {
            continue;
        }

        if (segment->mode == 2) {
            acceleration.x = 0.0f;
            acceleration.y = -segment->field_38 * segment->damping;
            acceleration.z = 0.0f;
            PSVECAdd(&acceleration, &segment->offset, &acceleration);

            next = rope_next_segment(rope, i);
            if (next != 0) {
                PSVECSubtract(&acceleration, &next->offset, &acceleration);
            }

            PSVECScale(
                &acceleration, &velocity_delta,
                update_seconds_per_frame * game_speed / segment->field_38);
            PSVECAdd(&segment->span, &segment->velocity, &segment->span);
            PSVECAdd(
                &segment->velocity, &velocity_delta, &segment->velocity);

            if (segment->field_48 <= 0.0f) {
                float span_length;
                float maximum_length;

                span_length = PSVECMag(&segment->span);
                maximum_length =
                    segment->length_scale + segment->field_48;
                if (span_length > maximum_length) {
                    PSVECScale(
                        &segment->span, &segment->span,
                        (rope->damping *
                             (span_length - maximum_length) +
                         maximum_length) /
                            span_length);
                    constraint_axis =
                        *(RwMatrixPosition*)&segment->span;
                    PSVECNormalize(
                        &constraint_axis.value, &constraint_axis.value);
                    PSVECScale(
                        &constraint_axis.value, &correction,
                        -PSVECDotProduct(
                            &constraint_axis.value, &segment->velocity));
                    PSVECAdd(
                        &segment->velocity, &correction,
                        &segment->velocity);
                }
            }

            segment->velocity.x *= segment->field_3C;
            segment->velocity.y *= segment->field_3C;
            segment->velocity.z *= segment->field_3C;
        }

        bone->parent_matrix->pos.x =
            bone->transform_parent->parent_matrix->pos.x + segment->span.x;
        bone->parent_matrix->pos.y =
            bone->transform_parent->parent_matrix->pos.y + segment->span.y;
        bone->parent_matrix->pos.z =
            bone->transform_parent->parent_matrix->pos.z + segment->span.z;
    }

    for (i = 0; i < rope->segment_count; i++) {
        RopeSegment* segment;
        MkBone* bone;

        segment = &rope->segments[i];
        bone = segment->bone;
        if (bone == 0) {
            continue;
        }

        if (segment->mode == 3 && rope->attached_model != 0) {
            MkBone* attached_bone;
            RwMatrix* attached_model_matrix;

            segment->velocity.x = 0.0f;
            segment->velocity.y = 0.0f;
            segment->velocity.z = 0.0f;
            attached_bone =
                rope->attached_model->bones[rope->attached_object_id];
            if (attached_bone->flags_54_bits.calculation_locked) {
                attached_matrix = attached_bone->matrix;
            } else {
                attached_model_matrix =
                    &rope->attached_model->frame->modelling;
                gxMat33x33(
                    (Mat33*)&attached_matrix,
                    (const Mat33*)attached_bone->parent_matrix,
                    (const Mat33*)attached_model_matrix);
                gxMatV3MatAddV3(
                    &attached_matrix.pos_vec,
                    &attached_bone->parent_matrix->pos_vec,
                    (Mat33*)attached_model_matrix,
                    &attached_model_matrix->pos_vec);
            }
            attached_position.x = attached_matrix.pos.x;
            attached_position.y = attached_matrix.pos.y;
            attached_position.z = attached_matrix.pos.z;
            gxMat33Tx31(
                &local_position, &attached_position,
                (Mat33*)&inverse_model_matrix);
            local_position.x += model->pos.value.x;
            local_position.y += model->pos.value.y;
            local_position.z += model->pos.value.z;
            bone->parent_matrix->pos.x = local_position.x;
            bone->parent_matrix->pos.y = local_position.y;
            bone->parent_matrix->pos.z = local_position.z;

            if (bone->transform_parent != 0) {
                segment->span.x =
                    bone->parent_matrix->pos.x -
                    bone->transform_parent->parent_matrix->pos.x;
                segment->span.y =
                    bone->parent_matrix->pos.y -
                    bone->transform_parent->parent_matrix->pos.y;
                segment->span.z =
                    bone->parent_matrix->pos.z -
                    bone->transform_parent->parent_matrix->pos.z;
            }
        }

        if (bone->flags_54_bits.calculation_locked) {
            rope_update_bone_matrix(bone, &model->frame->modelling);
        }
    }

    for (i = 0; i < rope->segment_count; i++) {
        RopeSegment* segment;
        RopeSegment* previous;
        RopeSegment* next;
        MkBone* bone;

        segment = &rope->segments[i];
        bone = segment->bone;
        if (bone == 0) {
            continue;
        }

        if (bone->transform_parent != 0) {
            float span_length;

            span_length = PSVECMag(&segment->span);
            if (span_length != 0.0f) {
                PSVECScale(
                    &segment->span, &segment->offset,
                    -((span_length - segment->length_scale) *
                      segment->inverse_length_scale) /
                        span_length);
            }
        }

        previous = rope_previous_segment(rope, i);
        if (previous != 0) {
            rope_set_identity(&source);
            rope_set_identity(&rotation);
            PSVECScale(&segment->span, &direction, -1.0f);
            rope_point_bone_at(
                bone, &direction, &axis, &quaternion, &source,
                &rotation);
        } else if (segment->mode == 1) {
            next = rope_next_segment(rope, i);
            if (next != 0) {
                PSVECAdd(&segment->span, &next->span, &midpoint);
                PSVECScale(&midpoint, &midpoint, 0.5f);
                rope_set_identity(&source);
                rope_set_identity(&rotation);
                PSVECSubtract(&segment->span, &midpoint, &direction);
                rope_point_bone_at(
                    bone, &direction, &axis, &quaternion, &source,
                    &rotation);
            }
        }

        if (bone->flags_54_bits.calculation_locked) {
            rope_update_bone_matrix(bone, &model->frame->modelling);
        }
    }
}
#include "rw/rtquat.h"
