#include "game/game_info.h"
#include "runtime/anim_api_ext.h"
#include "game/bgnd_jmt.h"
#include "math/gxMat.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/shadow.h"
#include "runtime/utils.h"
#include "runtime/cstring.h"
#include "runtime/mk_vtbl.h"

struct BgndDamageState {
    float wait_ticks;
    int cliff_index;
    int attempt_count;
    int field_0C;
    int valid;
    int trigger_tick;
    char pad18[0x14];
};

struct BgndDamagePdata {
    MkHdr hdr;
    int watcher_round;
    struct BgndDamageState state;
};

struct BgndUpdateData;
struct BgndUpdateVtable;
typedef void (*BgndSlotUpdateFn)(struct BgndUpdateData* update, int index);

struct BgndUpdateSlot {
    int blend_divisor;
    int blend_numerator;
    BgndSlotUpdateFn update_fn;
    int enabled;
    float start_value;
    float end_value;
    float shadow_scale;
    float initial_speed;
    float speed_param;
    float speed;
    float sin_rate;
    float sin_phase;
    float fall_acceleration;
    float field_34;
    float field_38;
    Vec direction;
};

struct BgndUpdateCommand {
    int delay;
    int slot_index;
    struct BgndUpdateSlot slot;
};

typedef int (*BgndUpdateDestroyFn)(
    struct BgndUpdateData* update, struct BgndUpdateVtable* vtable);

struct BgndUpdateVtable {
    void* reserved[4];
    BgndUpdateDestroyFn destroy;
};

struct BgndUpdateData {
    struct BgndUpdateVtable* vtbl;
    unsigned int instance;
    Vec origin;
    MkSobj* object;
    float origin_length;
    int group_id;
    int remove_hide;
    int active_slot;
    struct BgndUpdateCommand commands[2];
};

struct BgndUpdateCommandBlock {
    char pad00[0x28];
    int delay;
    int slot_index;
    struct BgndUpdateSlot slot;
};

struct RopeSegment {
    Vec velocity;
    float pad0C;
    Vec span;
    float pad1C;
    Vec offset;
    float pad2C;
    float length_scale;
    float damping;
    float field_38;
    float field_3C;
    float inverse_length_scale;
    int mode;
    float field_48;
    int bone_tag;
    MkBone* bone;
    char pad54[0x0C];
};

struct RopeInfo {
    int bone_tag;
    int mode;
    float field_08;
    float inverse_length_scale;
    float field_10;
    float field_14;
};

struct RopeControllerData {
    MkHdr hdr;
    MkObj* model;
    int segment_count;
    struct RopeSegment segments[3];
    char pad130[0x60];
    float damping;
    MkObj* attached_model;
    int attached_object_id;
};

static const float update_seconds_per_frame = 1.0f / 60.0f;

static int rope_bones[] = {0x2001, 0x2002, 0x2003};
struct RopeInfo g_rope_info[] = {
    {0x2001, 1, 0.0f, 0.0f, 0.0f, 0.0f},
    {0x2002, 2, 0.2f, 0.05f, 0.1f, 0.95f},
    {0x2003, 2, 0.2f, 0.05f, 0.1f, 0.95f},
};
static int n_rope_info = sizeof(g_rope_info) / sizeof(g_rope_info[0]);

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

extern MkObj* g_bgnd_preloaded_models[];
RopeProcLatch rope_proc_item;
RopeProcLatch sobj_ctrl_proc_item;
int g_ticks_delay;
int g_delay_rnd;

void* get_bone_with_tag(void* object, int tag);

static void rope_controller_init(MkHdr* pdata, MkObj* model);
static void rope_controller_update(MkHdr* pdata);
static float p_watch_shadow(void);
static float p_watch_cliffs(void);
static float p_obj_ctrl(void);
static float p_rope(void);
static void update_func_shadow_scale(struct BgndUpdateData* update, int index);
static void update_func_blend_start(struct BgndUpdateData* update, int index);
static void update_func_fall(struct BgndUpdateData* update, int index);
static void update_func_awayxz(struct BgndUpdateData* update, int index);
static void update_func_sin(struct BgndUpdateData* update, int index);
MkSobj* bgnd_fetch_sobj(int model_index, int object_id);
static void insert_obj_ctrl_section(MkSobj* object, int section);
void bgnd_start_script_in_proc(int proc_id, int function_index);

void start_cliff_watcher(float wait_ticks) {
    struct BgndDamagePdata* pdata;
    MkProc* proc;

    pdata = 0;
    if (find_mkproc_pid(0xB010) != 0) {
        return;
    }

    proc = _create_mkproc_generic_tinystack(
        0xB010, 0x1F, p_watch_cliffs, sizeof(*pdata), (MkHdr**)&pdata);
    if (proc == 0) {
        return;
    }
    if (pdata == 0) {
        return;
    }

    if (g_game_info.bgnd_obj != 0) {
        mk_insert((MkHdr*)proc, &g_game_info.bgnd_obj->child_list);
    }

    memset(&pdata->state, 0, sizeof(pdata->state));
    pdata->state.wait_ticks = wait_ticks;
    pdata->watcher_round = -1;
}

void set_cliff_watcher_round(int round) {
    MkProc* proc;
    struct BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (struct BgndDamagePdata*)pdata_of_proc(proc);
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
    struct BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (struct BgndDamagePdata*)pdata_of_proc(proc);
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
    struct BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (struct BgndDamagePdata*)pdata_of_proc(proc);
        if (pdata == 0) {
            pdata = 0;
        }
    }
    if (pdata != 0) {
        memset(&pdata->state, 0, sizeof(pdata->state));
    }
}

struct BgndDamageState* get_cliff_data(void) {
    MkProc* proc;
    struct BgndDamagePdata* pdata;

    proc = find_mkproc_pid(0xB010);
    if (proc == 0) {
        pdata = 0;
    } else {
        pdata = (struct BgndDamagePdata*)pdata_of_proc(proc);
        if (pdata == 0) {
            pdata = 0;
        }
    }
    if (pdata == 0) {
        return 0;
    }
    return &pdata->state;
}

static float p_watch_cliffs(void) {
    struct BgndDamagePdata* pdata;
    CmdScript* previous_script;
    CmdScript* script;
    int can_fall;

    pdata = (struct BgndDamagePdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    if (g_game_info.flag_bits.level_fatality_active == 1) {
        return 1.0f;
    }
    if (pdata->state.cliff_index >= 5) {
        return 1.0f;
    }

    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        can_fall = 0;
    } else if (!g_game_info.flag_bits.lens_flare_enabled) {
        can_fall = 0;
    } else if (g_game_info.flag_bits.field_bit0 == 1) {
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
    if (pdata->state.wait_ticks >= 0.0f) {
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
    struct BgndDamagePdata* pdata;
    struct BgndDamageState* damage;
    MkProc* proc;

    if (g_game_info.bgnd_id == 6) {
        proc = find_mkproc_pid(0xB010);
        if (proc == 0) {
            pdata = 0;
        } else {
            pdata = (struct BgndDamagePdata*)pdata_of_proc(proc);
            if (pdata == 0) {
                pdata = 0;
            }
        }
        if (pdata == 0) {
            damage = 0;
        } else {
            damage = &pdata->state;
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

int can_fallingcliff_fall(void) {
    if (g_game_info.pause_flag_bits.controllers_disabled == 1) {
        return 0;
    }
    if (!g_game_info.flag_bits.lens_flare_enabled) {
        return 0;
    }
    if (g_game_info.flag_bits.field_bit0 == 1) {
        return 0;
    }
    if (g_game_info.pause_flag_bits.fatality_window == 1) {
        return 0;
    }
    if (g_game_info.plyr0.field_0C == 0.0f ||
        g_game_info.plyr1.field_0C == 0.0f) {
        return 0;
    }
    return are_death_traps_on() != 0;
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

static float p_watch_shadow(void) {
    Vec angles;
    float height;
    float blend;

    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) {
        return 1.0f;
    }

    angles.z = 0.0f;
    angles.y = 0.0f;
    angles.x = 0.0f;
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
    return 1.0f;
}

void mks_set_update_delay(int ticks, int random_ticks) {
    g_delay_rnd = random_ticks;
    g_ticks_delay = ticks;
}

static inline int bgnd_update_active_slot(struct BgndUpdateData* update) {
    return update->active_slot;
}

void mks_removehide_by_group(int group_id, int remove_hide) {
    MkProc* proc;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    struct BgndUpdateData* update;

    proc = MK_LIVE(sobj_ctrl_proc_item.proc, sobj_ctrl_proc_item.instance);

    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!mklist_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (struct BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            discard_stale_mkptr(link);
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
    MkPtr** list;
    MkProc* proc;
    struct BgndUpdateData* update;
    int slot_index;
    int previous_index;
    MkPtr* link;
    MkPtr* next;

    proc = MK_LIVE(sobj_ctrl_proc_item.proc, sobj_ctrl_proc_item.instance);

    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!mklist_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (struct BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            discard_stale_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id) {
                slot_index = bgnd_update_active_slot(update);
                update->commands[slot_index].slot.blend_numerator = blend_ticks;
                update->commands[slot_index].slot.blend_divisor = blend_ticks;
                update->commands[slot_index].delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                update->commands[slot_index].slot.field_34 = 0.0f;
                update->commands[slot_index].slot.update_fn = update_func_shadow_scale;
                update->commands[slot_index].slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
                previous_index = update->active_slot - 1;
                if (previous_index < 0) {
                    previous_index = 1;
                }
                update->commands[previous_index].slot.shadow_scale = start_scale;
                update->commands[previous_index].slot.start_value = start_scale;
                update->commands[previous_index].slot.end_value = end_scale;
                update->commands[previous_index].slot.enabled = 1;
            }
            link = link->next;
        }
    }
}

void mks_blend_start_update_by_group(int group_id, int blend_ticks) {
    struct BgndUpdateData* update;
    int slot_index;
    MkPtr** list;
    MkPtr* link;
    MkPtr* next;
    MkProc* proc;

    proc = MK_LIVE(sobj_ctrl_proc_item.proc, sobj_ctrl_proc_item.instance);

    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!mklist_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (struct BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            discard_stale_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = bgnd_update_active_slot(update);
                update->commands[slot_index].slot.blend_numerator = blend_ticks;
                update->commands[slot_index].slot.blend_divisor = blend_ticks;
                update->commands[slot_index].delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                update->commands[slot_index].slot.field_34 = 0.0f;
                update->commands[slot_index].slot.update_fn = update_func_blend_start;
                update->commands[slot_index].slot_index = slot_index;

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
    MkPtr** list;
    MkProc* proc;
    struct BgndUpdateData* update;
    int slot_index;
    int previous_index;
    MkPtr* link;
    MkPtr* next;

    proc = MK_LIVE(sobj_ctrl_proc_item.proc, sobj_ctrl_proc_item.instance);

    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!mklist_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (struct BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            discard_stale_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = bgnd_update_active_slot(update);
                update->commands[slot_index].slot.blend_numerator = blend_ticks;
                update->commands[slot_index].slot.blend_divisor = blend_ticks;
                update->commands[slot_index].delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                update->commands[slot_index].slot.field_34 = 0.0f;
                update->commands[slot_index].slot.update_fn = update_func_fall;
                update->commands[slot_index].slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
                previous_index = update->active_slot - 1;
                if (previous_index < 0) {
                    previous_index = 1;
                }
                update->commands[previous_index].slot.fall_acceleration = gravity;
                update->commands[previous_index].slot.direction.x = velocity_x;
                update->commands[previous_index].slot.direction.y = velocity_y;
                update->commands[previous_index].slot.direction.z = velocity_z;
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
    struct BgndUpdateData* update;
    MkPtr* link;
    int slot_index;
    struct BgndUpdateCommandBlock* command;
    MkPtr* next;
    struct BgndUpdateCommandBlock* previous;
    MkSobj* object;
    float inverse_length;
    float varied_speed;
    float position_x;
    float position_y;
    float position_z;
    int previous_index;

    proc = MK_LIVE(sobj_ctrl_proc_item.proc, sobj_ctrl_proc_item.instance);

    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!mklist_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (struct BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            discard_stale_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = update->active_slot;
                command = (struct BgndUpdateCommandBlock*)((unsigned char*)update +
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
                previous = (struct BgndUpdateCommandBlock*)((unsigned char*)update +
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

void mks_set_rotate_update_by_group(
    int arg0, int arg1, float arg2, float arg3, float arg4, int arg5) {
}

void mks_set_sin_update_by_group(
    int group_id, int blend_ticks, int update_flags, float start_value,
    float end_value, float start_speed, float speed_param, float sin_rate,
    float sin_phase, int extra_flags) {
    MkPtr** list;
    MkProc* proc;
    struct BgndUpdateData* update;
    int slot_index;
    int previous_index;
    MkPtr* link;
    MkPtr* next;

    proc = MK_LIVE(sobj_ctrl_proc_item.proc, sobj_ctrl_proc_item.instance);

    if (proc == 0) {
        return;
    }

    list = &proc->pdata_list;
    if (!mklist_is_valid(list)) {
        return;
    }
    link = proc->pdata_list;
    while (link != 0) {
        update = (struct BgndUpdateData*)link->hdr;
        if (link->instance != update->instance) {
            next = link->next;
            discard_stale_mkptr(link);
            link = next;
        } else {
            if (update->group_id == group_id || group_id == -1) {
                slot_index = bgnd_update_active_slot(update);
                update->commands[slot_index].slot.blend_numerator = blend_ticks;
                update->commands[slot_index].slot.blend_divisor = blend_ticks;
                update->commands[slot_index].delay = g_ticks_delay +
                    (unsigned short)randu0(
                        (unsigned short)(g_delay_rnd + 1));
                update->commands[slot_index].slot.field_34 = 0.0f;
                update->commands[slot_index].slot.update_fn = update_func_sin;
                update->commands[slot_index].slot_index = slot_index;

                update->active_slot++;
                if (update->active_slot >= 2) {
                    update->active_slot = 0;
                }
                previous_index = update->active_slot - 1;
                if (previous_index < 0) {
                    previous_index = 1;
                }
                update->commands[previous_index].slot.shadow_scale = start_value;
                update->commands[previous_index].slot.start_value = start_value;
                update->commands[previous_index].slot.end_value = end_value;
                update->commands[previous_index].slot.speed = start_speed;
                update->commands[previous_index].slot.initial_speed = start_speed;
                update->commands[previous_index].slot.speed_param = speed_param;
                update->commands[previous_index].slot.sin_rate = sin_rate;
                update->commands[previous_index].slot.sin_phase = sin_phase;
                update->commands[previous_index].slot.enabled = update_flags;
                update->commands[previous_index].slot.enabled |= extra_flags;
            }
            link = link->next;
        }
    }
}

void start_sobj_ctrl_proc(void) {
    MkProcInitFlags flags;
    MkProc* proc;

    if (sobj_ctrl_proc_item.proc != 0) {
        return;
    }
    flags.value = 0;

    proc = get_mkproc_nostack(flags);
    proc = create_mkproc(0x1F, proc, 0xB007, p_obj_ctrl, 0);
    if (proc != 0) {
        g_delay_rnd = 0;
        g_ticks_delay = 0;
        sobj_ctrl_proc_item.proc = proc;
        sobj_ctrl_proc_item.instance = proc->instance;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&proc->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}

void destroy_sobj_ctrl_proc(void) {
    MkProc* proc;

    proc = sobj_ctrl_proc_item.proc;
    if (proc != 0) {
        proc = proc->instance == sobj_ctrl_proc_item.instance ? proc : 0;
    } else {
        proc = 0;
    }
    if (proc != 0 && proc->instance != 0) {
        proc->vtbl->destroy(proc);
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
        object = obj_find_sobj_by_id(
            g_game_info.bgnd_obj, object_id);
        if (object != 0) {
            insert_obj_ctrl_section(object, section);
        }
    }
}

static void insert_obj_ctrl_section(MkSobj* object, int section) {
    MkProc* proc;
    MkHdr* pdata;
    struct BgndUpdateData* update;
    float length_squared;
    int index;

    if (object == 0) {
        return;
    }

    proc = MK_LIVE(sobj_ctrl_proc_item.proc, sobj_ctrl_proc_item.instance);

    if (proc == 0) {
        return;
    }

    pdata = get_mkpdata_generic(sizeof(struct BgndUpdateData));
    if (pdata == 0) {
        return;
    }
    mk_insert(pdata, &proc->pdata_list);

    update = (struct BgndUpdateData*)pdata;
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
    update->origin_length = gxMathFastSqrt(update->origin_length);
    update->remove_hide = -1;
    update->active_slot = 0;

    for (index = 0; index < 2; index++) {
        update->commands[index].slot.field_34 = 0.0f;
        update->commands[index].slot.field_38 = 0.0f;
        update->commands[index].slot.direction.z = 0.0f;
        update->commands[index].slot.direction.y = 0.0f;
        update->commands[index].slot.direction.x = 0.0f;
        update->commands[index].slot.blend_divisor = 0;
        update->commands[index].slot.blend_numerator = 0;
        update->commands[index].slot.update_fn = 0;
        update->commands[index].slot.enabled = 0;
        update->commands[index].slot.start_value = 0.0f;
        update->commands[index].slot.end_value = 0.0f;
        update->commands[index].slot.shadow_scale = 0.0f;
        update->commands[index].slot.sin_phase = 0.0f;
        update->commands[index].slot.sin_rate = 0.0f;
        update->commands[index].delay = 0;
        update->commands[index].slot.fall_acceleration = 0.0f;
    }
}

static inline void update_bgnd_command_slots(struct BgndUpdateData* update) {
    float blend;
    int index;

    for (index = 0; index < 2; index++) {
        if (update->commands[index].slot.update_fn != 0) {
            if (update->commands[index].delay > 0) {
                update->commands[index].delay--;
            } else {
                update->commands[index].slot.update_fn(update, index);
                if (update->commands[index].slot.blend_divisor != -1) {
                    if (update->commands[index].slot.enabled != 0 &&
                        (update->commands[index].slot.enabled & 1) != 0) {
                        if (update->commands[index].slot.blend_divisor == 0) {
                            update->commands[index].slot.shadow_scale = 0.0f;
                        } else {
                            blend =
                                (float)update->commands[index].slot.blend_numerator /
                                (float)update->commands[index].slot.blend_divisor;
                            update->commands[index].slot.shadow_scale =
                                update->commands[index].slot.start_value * blend +
                                update->commands[index].slot.end_value * (1.0f - blend);
                        }
                    }

                    if (update->commands[index].slot.blend_divisor == 0) {
                        update->commands[index].slot.speed =
                            update->commands[index].slot.speed_param;
                    } else {
                        blend =
                            (float)update->commands[index].slot.blend_numerator /
                            (float)update->commands[index].slot.blend_divisor;
                        update->commands[index].slot.speed =
                            update->commands[index].slot.initial_speed * blend +
                            update->commands[index].slot.speed_param * (1.0f - blend);
                    }
                    update->commands[index].slot.blend_numerator--;
                    if (update->commands[index].slot.blend_numerator < 0) {
                        update->commands[index].slot.blend_numerator = 0;
                        update->commands[index].slot.update_fn = 0;
                    }
                }
            }
        }
    }
}

static float p_obj_ctrl(void) {
    struct BgndUpdateData* update;

    while (apdata != 0) {
        update = (struct BgndUpdateData*)apdata;
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

            update_bgnd_command_slots(update);
        }
        next_apdata();
    }
    return 1.0f;
}

static void update_func_shadow_scale(struct BgndUpdateData* update, int index) {
    MkSobj* object;
    float shadow_scale;

    object = update->object;
    if (object == 0 || update->commands[index].slot.blend_divisor <= 0) {
        if (object != 0) {
            hide_sobj(object);
        }
        if (update->instance != 0) {
            update->vtbl->destroy(update, update->vtbl);
        }
        return;
    }

    shadow_scale = update->commands[index].slot.shadow_scale;
    object->flags_08_bits.scale_dirty = 1;
    object->scale.x = 1.0f;
    object->scale.y = 1.0f;
    object->scale.z = shadow_scale;
}

static void update_func_blend_start(struct BgndUpdateData* update, int index) {
    MkSobj* object;
    float blend;
    float object_part;
    float origin_part;

    object = update->object;
    if (object == 0) {
        return;
    }
    if (update->commands[index].slot.blend_divisor <= 0) {
        object->pos.y = update->origin.y;
        return;
    }

    blend =
        (float)update->commands[index].slot.blend_numerator / (float)update->commands[index].slot.blend_divisor;
    object_part = object->pos.y * blend;
    origin_part = update->origin.y * (1.0f - blend);
    object->pos.y = object_part + origin_part;
}

static void update_func_awayxz(struct BgndUpdateData* update, int index) {
    MkSobj* object;
    float distance;
    Vec delta;

    object = update->object;
    if (object == 0) {
        return;
    }

    distance = update_seconds_per_frame * update->commands[index].slot.speed;
    delta.x = update->commands[index].slot.direction.x * distance;
    delta.y = 0.0f;
    delta.z = update->commands[index].slot.direction.z * distance;
    object->pos.x += delta.x;
    object->pos.y += delta.y;
    object->pos.z += delta.z;
}

static void update_func_fall(struct BgndUpdateData* update, int index) {
    MkSobj* object;
    Vec movement;

    object = update->object;
    if (object == 0) {
        return;
    }

    movement.x = update_seconds_per_frame * update->commands[index].slot.direction.x;
    movement.y = update_seconds_per_frame * update->commands[index].slot.direction.y;
    movement.z = update_seconds_per_frame * update->commands[index].slot.direction.z;
    object->pos.x += movement.x;
    object->pos.y += movement.y;
    object->pos.z += movement.z;
    update->commands[index].slot.direction.y +=
        update_seconds_per_frame * update->commands[index].slot.fall_acceleration;
}

static void update_func_sin(struct BgndUpdateData* update, int index) {
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

    frame_time = (float)(update->commands[index].slot.blend_divisor -
                         update->commands[index].slot.blend_numerator) / 60.0f;

    if ((update->commands[index].slot.enabled & 4) != 0) {
        base_angle = update->origin_length *
                     (update->commands[index].slot.sin_rate +
                      frand(update->commands[index].slot.sin_phase));
    } else if ((update->commands[index].slot.enabled & 8) != 0) {
        base_angle = object->pos.x *
                     (update->commands[index].slot.sin_rate +
                      frand(update->commands[index].slot.sin_phase));
    } else if ((update->commands[index].slot.enabled & 0x20) != 0) {
        base_angle = gxMathArcTanYX(update->origin.x, update->origin.z);
        random_offset = frand(update->commands[index].slot.sin_phase);
        base_angle *= update->commands[index].slot.sin_rate + random_offset;
    } else {
        base_angle = object->pos.x *
                     (update->commands[index].slot.sin_rate +
                      frand(update->commands[index].slot.sin_phase));
    }

    sine = gxMathSin(
        6.28f * update->commands[index].slot.speed * frame_time + base_angle);
    value = update->commands[index].slot.shadow_scale * sine;
    object->pos.y += value - update->commands[index].slot.field_34;
    update->commands[index].slot.field_34 = value;
}

static inline struct RopeControllerData* bgnd_find_rope_for_model(MkProc* proc,
                                                          MkObj* model)
{
    MkPtr* iterator;
    struct RopeControllerData* rope;

    if (proc == 0) {
        return 0;
    }
    iterator = first_mkptr(&proc->pdata_list);
    if (iterator == 0) {
        return 0;
    }
    rope = (struct RopeControllerData*)iterator->hdr;
    while (rope != 0) {
        if (rope->model == model) {
            return rope;
        }
        iterator = next_mkptr(iterator);
        if (iterator == 0) {
            rope = 0;
        } else {
            rope = (struct RopeControllerData*)iterator->hdr;
        }
    }
    return rope;
}

void bgnd_detach_rope(int model_index) {
    MkObj* model;
    MkProc* proc;
    struct RopeControllerData* rope;

    model = g_bgnd_preloaded_models[model_index];
    if (model == 0) {
        return;
    }

    proc = MK_LIVE(rope_proc_item.proc, rope_proc_item.instance);

    rope = bgnd_find_rope_for_model(proc, model);

    if (rope != 0) {
        rope->segments[rope->segment_count - 1].mode = 2;
        rope->attached_model = 0;
        rope->attached_object_id = 0;
    }
}

void bgnd_rope_adjust_length(int model_index, int preserve_shape, float length) {
    MkObj* model;
    MkProc* proc;
    struct RopeControllerData* rope;

    model = g_bgnd_preloaded_models[model_index];
    if (model == 0) {
        return;
    }

    proc = MK_LIVE(rope_proc_item.proc, rope_proc_item.instance);

    rope = bgnd_find_rope_for_model(proc, model);

    if (rope != 0) {
        float scale;
        int i;

        scale = 0.7f * length;
        for (i = 0; i < rope->segment_count; i++) {
            struct RopeSegment* segment;

            segment = &rope->segments[i];
            segment->length_scale *= scale / 10.0f;
            if (preserve_shape == 1 && scale > 0.0f) {
                segment->inverse_length_scale *= 10.0f / scale;
            }
        }
    }
}

static inline struct RopeControllerData* find_rope_for_model(MkProc* proc, MkObj* rope_model) {
    struct RopeControllerData* rope;
    if (proc == 0) {
        rope = 0;
    } else {
        MkPtr* iterator;

        iterator = first_mkptr(&proc->pdata_list);
        if (iterator == 0) {
            rope = 0;
        } else {
            rope = (struct RopeControllerData*)iterator->hdr;
            while (rope != 0) {
                if (rope->model == rope_model) {
                    return rope;
                }
                iterator = next_mkptr(iterator);
                if (iterator == 0) {
                    rope = 0;
                } else {
                    rope = (struct RopeControllerData*)iterator->hdr;
                }
            }
        }
    }

    return rope;
}

void bgnd_attach_rope_to_bgnd_obj(
    int rope_model_index, int target_model_index, int object_id) {
    MkObj* target_model;
    MkObj* rope_model;
    MkProc* proc;
    struct RopeControllerData* rope;

    rope_model = g_bgnd_preloaded_models[rope_model_index];
    if (rope_model == 0) {
        return;
    }
    target_model = g_bgnd_preloaded_models[target_model_index];
    if (target_model == 0) {
        return;
    }
    if (rope_model != 0 && target_model != 0) {
        proc = MK_LIVE(rope_proc_item.proc, rope_proc_item.instance);

        rope = find_rope_for_model(proc, rope_model);

        if (rope != 0) {
            rope->segments[rope->segment_count - 1].mode = 3;
            rope->attached_model = target_model;
            rope->attached_object_id = object_id;
        }
    }
}

/* TODO: [near miss] 98.82%; typed rope owner and latch agree; process/rope saved-register pair remains. */
void bgnd_preload_obj_attach_rope(int model_index) {
    MkProc* rope_proc;
    MkObj* model;
    struct RopeControllerData* rope;

    model = g_bgnd_preloaded_models[model_index];
    if (model != 0) {
        rope_proc = MK_LIVE(rope_proc_item.proc, rope_proc_item.instance);
        if (rope_proc != 0) {
            rope = (struct RopeControllerData*)get_mkpdata_generic(0x1A0);
            if (rope != 0) {
                mk_insert(&rope->hdr, &rope_proc->pdata_list);
                rope_controller_init(&rope->hdr, model);
            }
        }
    }
}

void start_rope_proc(void) {
    MkProcInitFlags flags;
    MkProc* proc;

    if (rope_proc_item.proc != 0) {
        return;
    }
    flags.value = 0;

    proc = get_mkproc_nostack(flags);
    proc = create_mkproc(0x16, proc, 0xB003, p_rope, 0);
    if (proc != 0) {
        rope_proc_item.proc = proc;
        rope_proc_item.instance = proc->instance;
        if (g_game_info.bgnd_obj != 0) {
            mk_insert(&proc->hdr, &g_game_info.bgnd_obj->child_list);
        }
    }
}

static float p_rope(void) {
    while (apdata != 0) {
        rope_controller_update(apdata);
        next_apdata();
    }
    return 1.0f;
}

static inline void rope_bone_world_matrix(
    RwMatrix* out, RwMatrix* parent, MkObj* model) {
    RwMatrix* modelling = &model->frame->modelling;

    gxMat33x33((Mat33*)out, (Mat33*)parent, (Mat33*)modelling);
    gxMatV3MatAddV3(
        (Vec*)&out->pos, (Vec*)&parent->pos, (Mat33*)modelling,
        (Vec*)&modelling->pos);
}

/* TODO: [near miss] 96.80%; rope info/model owner coloring and stack-matrix address registers remain. */
static void rope_controller_init(MkHdr* pdata, MkObj* model) {
    struct RopeControllerData* rope;
    struct RopeInfo* info_base;
    int segment_count;
    int i;

    rope = (struct RopeControllerData*)pdata;
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
        struct RopeInfo* info;
        struct RopeSegment* segment;
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

        bone = get_bone_with_tag(model, segment->bone_tag);
        if (bone == 0) {
            break;
        }

        segment->bone = bone;
        bone->flags_54_bits.calculation_locked = 1;
        rope_bone_world_matrix(
            &bone->matrix, bone->parent_matrix, model);

        if (bone->transform_parent != 0) {
            MkBone* child;
            MKMATRIX bone_matrix;
            MKMATRIX child_matrix;

            child = bone->transform_parent;
            child->flags_54_bits.calculation_locked = 1;
            rope_bone_world_matrix(
                &child->matrix, child->parent_matrix,
                model);

            if (bone->flags_54_bits.calculation_locked) {
                bone_matrix = bone->matrix;
            } else {
                rope_bone_world_matrix(
                    &bone_matrix, bone->parent_matrix,
                    model);
            }
            if (child->flags_54_bits.calculation_locked) {
                child_matrix = child->matrix;
            } else {
                rope_bone_world_matrix(
                    &child_matrix, child->parent_matrix,
                    model);
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

static inline void rope_point_bone_at(
    MkBone* bone, const RwMatrix* initial_matrix, Vec* direction,
    RwMatrixPosition* axis, Quat* quaternion,
    RwMatrix* source, RwMatrix* rotation) {
    PSVECNormalize(direction, direction);
    *axis = *(const RwMatrixPosition*)&initial_matrix->up;
    PSVECNormalize(&axis->value, &axis->value);
    gxVectV3V3ToQuat(quaternion, &axis->value, direction);
    gxQuatQuatToMat((Mat33*)rotation, quaternion);
    *source = *bone->parent_matrix;
    gxMat33x33((Mat33*)bone->parent_matrix, (const Mat33*)source,
              (const Mat33*)rotation);
}

static inline struct RopeSegment* rope_next_segment(
    struct RopeControllerData* rope, int index) {
    index++;
    if (index >= rope->segment_count) {
        return 0;
    }
    return &rope->segments[index];
}

static inline struct RopeSegment* rope_previous_segment(
    struct RopeControllerData* rope, int index) {
    index--;
    if (index < 0) {
        return 0;
    }
    return &rope->segments[index];
}

/* TODO: [near miss] 99.05%; frame and operations agree; owner GPR coloring and zero-compare operand order remain. */
static void rope_controller_update(MkHdr* pdata) {
    struct RopeControllerData* rope = (struct RopeControllerData*)pdata;
    MkObj* model = rope->model;
    MKMATRIX inverse_model_matrix;
    MKMATRIX attached_matrix;
    MKMATRIX rotation;
    MKMATRIX source;
    MKVECTOR acceleration;
    MKVECTOR velocity_delta;
    RwMatrixPosition constraint_axis;
    MKVECTOR correction;
    MKVECTOR local_position;
    MKVECTOR attached_position;
    Quat quaternion;
    RwMatrixPosition axis;
    MKVECTOR direction;
    MKVECTOR midpoint;
    int i;

    if (model == 0) {
        return;
    }

    RwMatrixInvert(&inverse_model_matrix, &model->frame->modelling);

    for (i = 0; i < rope->segment_count; i++) {
        struct RopeSegment* segment;
        struct RopeSegment* next;
        MkBone* bone;
        MkBone* parent;

        segment = &rope->segments[i];
        bone = segment->bone;
        if (bone == 0) {
            continue;
        }

        if (segment->mode == 1) {
            if (bone->flags_54_bits.calculation_locked) {
                const RwMatrix* model_matrix = &model->frame->modelling;
                rope_update_bone_matrix(bone, model_matrix);
            }
            continue;
        }

        parent = bone->transform_parent;
        if (parent == 0) {
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

            if (segment->field_48 >= 0.0f) {
                float span_length;
                float maximum_length;

                span_length = PSVECMag(&segment->span);
                maximum_length =
                    segment->length_scale + segment->field_48;
                if (span_length > maximum_length) {
                    float scale;
                    scale = rope->damping * (span_length - maximum_length) +
                            maximum_length;
                    scale /= span_length;
                    PSVECScale(&segment->span, &segment->span, scale);
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
            segment->span.x + parent->parent_matrix->pos.x;
        bone->parent_matrix->pos.y =
            segment->span.y + parent->parent_matrix->pos.y;
        bone->parent_matrix->pos.z =
            segment->span.z + parent->parent_matrix->pos.z;
    }

    for (i = 0; i < rope->segment_count; i++) {
        struct RopeSegment* segment;
        MkBone* bone;

        segment = &rope->segments[i];
        bone = segment->bone;
        if (bone == 0) {
            continue;
        }

        if (segment->mode == 3 && rope->attached_model != 0) {
            MkBone* attached_bone;
            MkBone* parent;
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
            local_position.x += inverse_model_matrix.pos.x;
            local_position.y += inverse_model_matrix.pos.y;
            local_position.z += inverse_model_matrix.pos.z;
            bone->parent_matrix->pos.x = local_position.x;
            bone->parent_matrix->pos.y = local_position.y;
            bone->parent_matrix->pos.z = local_position.z;

            parent = bone->transform_parent;
            if (parent != 0) {
                segment->span.x =
                    bone->parent_matrix->pos.x -
                    parent->parent_matrix->pos.x;
                segment->span.y =
                    bone->parent_matrix->pos.y -
                    parent->parent_matrix->pos.y;
                segment->span.z =
                    bone->parent_matrix->pos.z -
                    parent->parent_matrix->pos.z;
            }
        }

        if (bone->flags_54_bits.calculation_locked) {
            const RwMatrix* model_matrix = &model->frame->modelling;
            rope_update_bone_matrix(bone, model_matrix);
        }
    }

    for (i = 0; i < rope->segment_count; i++) {
        struct RopeSegment* segment;
        struct RopeSegment* previous;
        struct RopeSegment* next;
        MkBone* bone;
        RwMatrix* parent_matrix;

        segment = &rope->segments[i];
        bone = segment->bone;
        if (bone == 0) {
            continue;
        }

        if (bone->transform_parent != 0) {
            float span_length;

            span_length = PSVECMag(&segment->span);
            if (span_length) {
                float extension = span_length - segment->length_scale;
                PSVECScale(&segment->span, &segment->offset,
                           -(extension * segment->inverse_length_scale) /
                               span_length);
            }
        }

        previous = rope_previous_segment(rope, i);
        if (previous != 0) {
            RwMatrixSetIdentityMacro(&rotation);
            RwMatrixSetIdentityMacro(&source);
            parent_matrix = bone->parent_matrix;
            PSVECScale(&segment->span, &direction, -1.0f);
            rope_point_bone_at(
                bone, parent_matrix, &direction, &axis, &quaternion, &source,
                &rotation);
        } else if (segment->mode == 1) {
            next = rope_next_segment(rope, i);
            if (next != 0) {
                PSVECAdd(&segment->span, &next->span, &midpoint);
                PSVECScale(&midpoint, &midpoint, 0.5f);
                RwMatrixSetIdentityMacro(&rotation);
                RwMatrixSetIdentityMacro(&source);
                parent_matrix = bone->parent_matrix;
                PSVECSubtract(&segment->span, &midpoint, &direction);
                rope_point_bone_at(
                    bone, parent_matrix, &direction, &axis, &quaternion, &source,
                    &rotation);
            }
        }

        if (bone->flags_54_bits.calculation_locked) {
            const RwMatrix* model_matrix = &model->frame->modelling;
            rope_update_bone_matrix(bone, model_matrix);
        }
    }
}
#include "rw/rtquat.h"
