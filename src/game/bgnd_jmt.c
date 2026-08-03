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

static float bgnd_inv_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } guess;
    float product;
    float correction;

    if (!(value > 0.0f)) {
        return 0.0f;
    }
    guess.f = value;
    guess.u = 0x5F375A00U - (guess.u >> 1);
    product = guess.f * (value * guess.f);
    correction = 3.0f - product;
    return 0.0625f * guess.f * correction *
           -(correction * (product * correction) - 12.0f);
}

static float bgnd_sqrt(float value) {
    union {
        float f;
        unsigned int u;
    } input, guess;
    float refined;

    if (!(value > 0.0f)) {
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
extern RopeProcLatch rope_proc_item;
extern RopeProcLatch sobj_ctrl_proc_item;
extern int exec_tick_ctr;
extern int g_delay_rnd;
extern int g_ticks_delay;

void build_bones_tbl(MkObj* object, const int* tags);
void update_bone_hierarchy(void* object);
void* get_bone_with_tag(void* object, int tag);

void rope_controller_init(MkHdr* pdata, MkObj* model);
void rope_controller_update(MkHdr* pdata);
float p_watch_shadow(void);
float p_watch_cliffs(void);
static float p_obj_ctrl(void);
float p_rope(void);
void update_func_shadow_scale(BgndUpdateData* update, int index);
void update_func_blend_start(BgndUpdateData* update, int index);
void update_func_fall(BgndUpdateData* update, int index);
void update_func_awayxz(BgndUpdateData* update, int index);
void update_func_sin(BgndUpdateData* update, int index);
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
    if (list == 0) {
        return;
    }
    link = *list;
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

void mks_shadow_scale(int group_id, int blend_ticks,
                      float start_scale, float end_scale) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateSlot* previous;
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
    if (list == 0) {
        return;
    }
    link = *list;
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
                                 randu0((unsigned short)(g_delay_rnd + 1));
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
                previous = &update->slots[previous_index];
                previous->shadow_scale = start_scale;
                previous->start_value = start_scale;
                previous->end_value = end_scale;
                previous->enabled = 1;
            }
            link = link->next;
        }
    }
}

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
    if (list == 0) {
        return;
    }
    link = *list;
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
                                 randu0((unsigned short)(g_delay_rnd + 1));
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

void mks_gravity_update_by_group(int group_id, int blend_ticks,
                                 float velocity_x, float velocity_y,
                                 float velocity_z, float gravity) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateSlot* previous;
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
    if (list == 0) {
        return;
    }
    link = *list;
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
                                 randu0((unsigned short)(g_delay_rnd + 1));
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
                previous = &update->slots[previous_index];
                previous->fall_acceleration = gravity;
                previous->direction.x = velocity_x;
                previous->direction.y = velocity_y;
                previous->direction.z = velocity_z;
            }
            link = link->next;
        }
    }
}

void mks_away_vel_update_by_group(int group_id, int blend_ticks,
                                  float speed, float speed_param,
                                  float random_range) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    BgndUpdateData* update;
    BgndUpdateCommandBlock* command;
    BgndUpdateSlot* previous;
    MkSobj* object;
    float inverse_length;
    float varied_speed;
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
    if (list == 0) {
        return;
    }
    link = *list;
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
                                 randu0((unsigned short)(g_delay_rnd + 1));
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
                previous = &update->slots[previous_index];
                varied_speed = speed +
                               (frand(random_range) - 0.5f * random_range);
                previous->speed = varied_speed;
                previous->initial_speed = varied_speed;
                previous->speed_param = speed_param;

                object = update->object;
                inverse_length = bgnd_inv_sqrt(
                    object->pos.x * object->pos.x +
                    object->pos.y * object->pos.y +
                    object->pos.z * object->pos.z);
                previous->direction.x = object->pos.x * inverse_length;
                previous->direction.y = object->pos.y * inverse_length;
                previous->direction.z = object->pos.z * inverse_length;
            }
            link = link->next;
        }
    }
}

void mks_set_rotate_update_by_group(void) {
}

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
    BgndUpdateSlot* previous;
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
    if (list == 0) {
        return;
    }
    link = *list;
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
                                 randu0((unsigned short)(g_delay_rnd + 1));
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
                previous = &update->slots[previous_index];
                previous->shadow_scale = start_value;
                previous->start_value = start_value;
                previous->end_value = end_value;
                previous->speed = start_speed;
                previous->initial_speed = start_speed;
                previous->speed_param = speed_param;
                previous->sin_rate = sin_rate;
                previous->sin_phase = sin_phase;
                previous->enabled = update_flags;
                previous->enabled |= extra_flags;
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
    object->flags_08 |= 0x40;
    update->origin.x = object->pos.x;
    update->origin.y = object->pos.y;
    update->origin.z = object->pos.z;
    length_squared = update->origin.x * update->origin.x +
                     update->origin.y * update->origin.y +
                     update->origin.z * update->origin.z;
    update->origin_length = bgnd_sqrt(length_squared);
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

void update_func_shadow_scale(BgndUpdateData* update, int index) {
    BgndUpdateCommandBlock* command;
    MkSobj* object;

    object = update->object;
    command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
    if (object == 0 || command->slot.blend_divisor <= 0) {
        if (object != 0) {
            hide_sobj(object);
        }
        if (update->instance != 0) {
            update->vtbl->destroy(update, update->vtbl);
        }
        return;
    }

    object->flags_08_bits.scale_dirty = 1;
    object->scale.x = 1.0f;
    object->scale.y = 1.0f;
    object->scale.z = command->slot.shadow_scale;
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
        object->pos.y = update->origin.y;
        return;
    }

    blend =
        (float)slot->blend_numerator / (float)slot->blend_divisor;
    object->pos.y =
        object->pos.y * blend + update->origin.y * (1.0f - blend);
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

void update_func_sin(BgndUpdateData* update, int index) {
    BgndUpdateCommandBlock* command;
    BgndUpdateSlot* slot;
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

    command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
    slot = &command->slot;
    frame_time = (float)(slot->blend_divisor - slot->blend_numerator) / 60.0f;

    if ((slot->enabled & 4) != 0) {
        random_offset = frand(slot->sin_phase);
        command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
        slot = &command->slot;
        base_angle = update->origin_length *
                     (slot->sin_rate + random_offset);
    } else if ((slot->enabled & 8) != 0) {
        random_offset = frand(slot->sin_phase);
        command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
        slot = &command->slot;
        base_angle = object->pos.x *
                     (slot->sin_rate + random_offset);
    } else if ((slot->enabled & 0x20) != 0) {
        base_angle = gxMathArcTanYX(update->origin.x, update->origin.z);
        command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
        random_offset = frand(command->slot.sin_phase);
        command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
        base_angle *= command->slot.sin_rate + random_offset;
    } else {
        random_offset = frand(slot->sin_phase);
        command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
        slot = &command->slot;
        base_angle = object->pos.x *
                     (slot->sin_rate + random_offset);
    }

    command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
    sine = gxMathSin(6.28f * command->slot.speed * frame_time + base_angle);
    command = (BgndUpdateCommandBlock*)((unsigned char*)update + index * 0x50);
    slot = &command->slot;
    value = slot->shadow_scale * sine;
    object->pos.y += value - slot->field_34;
    slot->field_34 = value;
}

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

void bgnd_attach_rope_to_bgnd_obj(
    int rope_model_index, int target_model_index, int object_id) {
    MkObj* rope_model;
    MkObj* target_model;
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

void rope_controller_init(MkHdr* pdata, MkObj* model) {
    RopeControllerData* rope;
    int i;

    rope = (RopeControllerData*)pdata;
    rope->model = model;
    rope->attached_model = 0;
    rope->attached_object_id = 0;
    build_bones_tbl(model, rope_bones);
    update_bone_hierarchy(model != 0 ? as_mkhdr(&model->hdr) : 0);

    rope->segment_count = n_rope_info;
    rope->damping = 0.975f;
    for (i = 0; i < rope->segment_count; i++) {
        RopeSegment* segment;
        RopeInfo* info;
        MkBone* bone;

        segment = &rope->segments[i];
        info = &g_rope_info[i];
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
        bone->flags_54 |= 0x10;
        gxMat33x33(
            (Mat33*)&bone->matrix, (Mat33*)bone->parent_matrix,
            (Mat33*)model->frame);
        gxMatV3MatAddV3(
            (Vec*)&bone->matrix.pos, (Vec*)&bone->parent_matrix->pos,
            (Mat33*)model->frame, (Vec*)&model->frame->modelling.pos);

        if (bone->transform_parent != 0) {
            MkBone* child;
            RwMatrix bone_matrix;
            RwMatrix child_matrix;

            child = bone->transform_parent;
            child->flags_54 |= 0x10;
            gxMat33x33(
                (Mat33*)&child->matrix, (Mat33*)child->parent_matrix,
                (Mat33*)model->frame);
            gxMatV3MatAddV3(
                (Vec*)&child->matrix.pos,
                (Vec*)&child->parent_matrix->pos, (Mat33*)model->frame,
                (Vec*)&model->frame->modelling.pos);

            if ((bone->flags_54 & 0x10) != 0) {
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
            if ((child->flags_54 & 0x10) != 0) {
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
