#include "runtime/anim_pdata.h"
#include "game/nis.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/cam.h"
#include "runtime/image.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/section.h"
#include "runtime/mk_struct.h"
#include "rw/rwcamera_internal.h"

typedef struct KonquestPathData {
    MkHdr hdr;
    void* waypoints; /* +0x08 */
    int script_state; /* +0x0C */
    int travel_state; /* +0x10 */
    int travel_mode; /* +0x14 */
    int path_id; /* +0x18 */
    Vec destination; /* +0x1C */
    int use_animation_override; /* +0x28 */
    int animation_override; /* +0x2C */
    union {
        int current_waypoint;
        int reaction_state;
    }; /* +0x30 */
    int target_waypoint; /* +0x34 */
    int previous_waypoint; /* +0x38 */
    int step_direction; /* +0x3C */
    float speed; /* +0x40 */
} KonquestPathData;

typedef struct KonquestNpcData {
    char pad00[0x4C];
    Vec position; /* +0x4C */
    float angle_y; /* +0x58 */
} KonquestNpcData;

typedef struct KonquestTileOrigin {
    char pad00[4];
    int loaded; /* +0x04 */
    Vec origin; /* +0x08 */
} KonquestTileOrigin;

typedef struct KonquestNpcAnimState {
    char pad00[0x0C];
    MkObj* object; /* +0x0C */
    char pad10[4];
    AniTextureControl* lip_texture; /* +0x14 */
    unsigned int lip_texture_instance; /* +0x18 */
    int dialog_anim; /* +0x1C */
    MkProc* proc; /* +0x20 */
} KonquestNpcAnimState;

typedef struct KonquestNpcEvent {
    int enabled;
    char pad04[8];
} KonquestNpcEvent;

typedef struct KonquestTime {
    int year;
    int month;
    int day_of_month;
    int day_of_week;
    int hour;
    int minute;
} KonquestTime;

typedef struct KonquestEventSchedule {
    int year;
    int month;
    int day_of_month;
    int day_of_week;
    int hour;
    int minute;
} KonquestEventSchedule;

typedef struct AniData AniData;
typedef RpMaterial* (*KonquestMaterialCallback)(
    RpMaterial* material, void* data);

typedef struct KonquestNpc {
    MkHdr hdr;
    char pad08[4];
    KonquestNpcData* data; /* +0x0C */
    KonquestPathData* path; /* +0x10 */
    KonquestNpcAnimState* animation; /* +0x14 */
    char pad18[4];
    union {
        int flags; /* +0x1C */
        struct {
            union {
                unsigned char flags_1C;
                struct {
                    unsigned char flags_1C_pad_high : 1;
                    unsigned char reaction_active : 1;
                    unsigned char flags_1C_pad_mid : 2;
                    unsigned char ignore_events : 1;
                    unsigned char flags_1C_pad_low : 3;
                };
            };
            union {
                unsigned char flags_1D;
                struct {
                    unsigned char flags_1D_pad_high : 3;
                    unsigned char skip_visibility : 1;
                    unsigned char flags_1D_pad_mid : 1;
                    unsigned char reaction_mode : 1;
                    unsigned char flags_1D_pad_low : 2;
                };
            };
            union {
                unsigned char timed_event_flags; /* +0x1E */
                struct {
                    unsigned char animation_override : 1;
                    unsigned char wait_for_animation : 1;
                    unsigned char timed_event_pad_bit5 : 1;
                    unsigned char reset_timed_events : 1;
                    unsigned char timed_event_pad_low : 4;
                };
            };
            unsigned char flags_1F;
        };
    };
    char pad20[0x20];
    int tile_index; /* +0x40 */
    char pad44[0x18];
    AniData* queued_animation; /* +0x5C */
    float queued_animation_frame; /* +0x60 */
    unsigned int animation_flags; /* +0x64 */
    int conversation_count; /* +0x68 */
    int punch_count; /* +0x6C */
    float wait_ticks; /* +0x70 */
    float animation_speed; /* +0x74 */
    unsigned int saved_script_position; /* +0x78 */
    char saved_script_state[0x140]; /* +0x7C */
    unsigned int saved_script_stack_depth; /* +0x1BC */
    char pad1C0[4];
    float camera_distance_squared; /* +0x1C4 */
    char pad1C8[8];
    KonquestNpcEvent events[7]; /* +0x1D0 */
    char pad224[0x20];
    KonquestTime wake_time; /* +0x244 */
    MkProc* turn_proc; /* +0x25C */
    unsigned int turn_proc_instance; /* +0x260 */
} KonquestNpc;

typedef struct KonquestNpcLoadConfig {
    char pad00[0x0C];
    char* art_section_name; /* +0x0C */
    char* animation_section_name; /* +0x10 */
    char pad14[0x18];
    char* string_bank_name; /* +0x2C */
} KonquestNpcLoadConfig;

typedef struct KonquestNpcPdata {
    char pad00[0x24];
    ScriptSlot* waypoint_script; /* +0x24 */
    KonquestNpcLoadConfig* load_config; /* +0x28 */
    char pad2C[0x10];
    MkPtr* npc_list; /* +0x3C */
    char pad40[4];
    int dialog_ready; /* +0x44 */
    char pad48[0xB0];
    MkObj* monk; /* +0xF8 */
    unsigned int monk_instance; /* +0xFC */
    char pad100[0x38];
    KonquestTime current_time; /* +0x138 */
    char pad150[0xB0];
    int conversation_state_a; /* +0x200 */
    int conversation_state_b; /* +0x204 */
    int conversation_mode_a; /* +0x208 */
    int conversation_mode_b; /* +0x20C */
    int conversation_event; /* +0x210 */
    char pad214[0x22C];
    MkObj* camera_target; /* +0x440 */
    unsigned int camera_target_instance; /* +0x444 */
} KonquestNpcPdata;

typedef struct NpcManagerPdata NpcManagerPdata;
typedef struct NpcManagerObject NpcManagerObject;

typedef void (*NpcManagerDestroyFn)(
    NpcManagerPdata* manager, NpcManagerObject* object);

struct NpcManagerObject {
    char pad00[0x10];
    NpcManagerDestroyFn destroy; /* +0x10 */
};

struct NpcManagerPdata {
    NpcManagerObject* object;
    unsigned int has_object;
};

typedef struct KonquestTriggerData {
    char pad00[0x20];
    int source_type; /* +0x20 */
    KonquestNpc* source_npc; /* +0x24 */
} KonquestTriggerData;

typedef struct KonquestTrigger {
    char pad00[8];
    KonquestTriggerData* data; /* +0x08 */
} KonquestTrigger;

typedef struct AnimState AnimState;
typedef struct GroundCollTable GroundCollTable;

typedef struct KonquestReactionPdata {
    MkHdr hdr;
    char pad08[4];
    MkObj* object; /* +0x0C */
    char pad10[0x10];
    MkProc* animation_proc; /* +0x20 */
    KonquestNpc* npc; /* +0x24 */
} KonquestReactionPdata;

typedef struct KonquestModelLoadPdata {
    MkHdr hdr;
    char pad08[0x1C];
    KonquestNpc* npc; /* +0x24 */
} KonquestModelLoadPdata;

typedef struct KonquestAnimPdata {
    MkHdr hdr;
    char pad08[0x28];
    unsigned int flags; /* +0x30 */
    char pad34[4];
    float frame; /* +0x38 */
    float low_frame; /* +0x3C */
    float high_frame; /* +0x40 */
    float step; /* +0x44 */
    char pad48[8];
    Vec root_offset; /* +0x50 */
} KonquestAnimPdata;

typedef struct KonquestNpcProcSleepVtable {
    char pad00[0x18];
    void (*sleep)(void); /* +0x18 */
} KonquestNpcProcSleepVtable;

typedef struct KonquestProcDestroyVtable KonquestProcDestroyVtable;

struct KonquestProcDestroyVtable {
    char pad00[0x10];
    void (*destroy)(
        MkProc* proc, KonquestProcDestroyVtable* vtable); /* +0x10 */
};

typedef struct LipSyncKeyframe {
    float time;
    int frame;
} LipSyncKeyframe;

typedef struct KonquestLipSyncPdata {
    MkHdr hdr;
    int mode; /* +0x08 */
    KonquestNpc* npc; /* +0x0C */
    AniTextureControl* texture; /* +0x10 */
    unsigned int texture_instance; /* +0x14 */
    LipSyncKeyframe* keyframes; /* +0x18 */
    unsigned int sound_handle; /* +0x1C */
    float elapsed; /* +0x20 */
    int stop_requested; /* +0x24 */
} KonquestLipSyncPdata;

typedef struct KonquestWaypointScriptPdata {
    MkHdr hdr;
    unsigned int function_index; /* +0x08 */
    KonquestNpc* npc; /* +0x0C */
} KonquestWaypointScriptPdata;

typedef struct TurnAndFacePdata {
    MkHdr hdr;
    float angle; /* +0x08 */
    Vec position; /* +0x0C */
    int use_angle; /* +0x18 */
    int saved_pin_animation; /* +0x1C */
    int target_kind; /* +0x20 */
    union {
        KonquestNpc* npc;
        MkObj* object;
    } target; /* +0x24 */
} TurnAndFacePdata;

typedef union ObliqueMatrixCell {
    float value;
    unsigned int flags;
} ObliqueMatrixCell;

typedef union KonquestFloatBits {
    float value;
    unsigned int bits;
} KonquestFloatBits;

typedef struct KonquestDestroyVtable {
    char pad00[0x10];
    void (*destroy)(void* object); /* +0x10 */
} KonquestDestroyVtable;

typedef struct KonquestNpcShadows {
    char pad00[0x60];
    MkHdr* objects[15]; /* +0x60 */
    union {
        unsigned char flags; /* +0x9C */
        struct {
            unsigned char initialized : 1;
            unsigned char pad_flags : 7;
        };
    };
    char pad9D[0x7F];
    int active_count; /* +0x11C */
} KonquestNpcShadows;

typedef ObliqueMatrixCell ObliqueMatrix[16];

typedef struct KonquestCmdScriptView {
    char pad00[0x14];
    unsigned int position;
    char pad18[8];
    int state;
    char pad24[0x40];
    char execution_state[0x140];
} KonquestCmdScriptView;

extern KonquestNpc* g_active_npc;
extern NpcManagerPdata* npc_manager_pdata;
extern KonquestNpcPdata* konquest_pdata;
extern GroundCollTable npc_ground_colls;
extern GroundCollTable npc_punched_ground_colls;
extern int npc_fast_anims[];
extern int konquest_editor_mode_on;
extern CameraObj* camera_obj;
extern void* Camera;
extern KonquestNpcShadows npc_shadows;
extern float p_anim_idle(void);
extern MkVtable5 vtbl_path_data_struct;

KonquestTrigger* find_trigger_by_id(void);
void execute_trigger(KonquestTrigger* trigger);
void destroy_mkproc_nostack(MkProc* proc);
void set_root_and_obj_movement_weights(
    float root_weight, float object_weight, AnimState* animation);
void npc_play_dialog_and_anim_sequence(int dialog, int animation);
void resume_hero_state_process(void);
AniData* get_animation(int animation_id);
void transition_to_anim_script(
    KonquestAnimPdata* animation, AniData* script, int flags, float blend);
void set_anim_script(
    KonquestAnimPdata* animation, AniData* script, int flags);
float anim_script_lastframe(AniData* script);
void npc_travel_path(int path_id, int path_arg, int travel_mode);
KonquestTileOrigin* get_nth_tile_struct(int index);
int get_tile_from_position(const Vec* position);
void npc_set_his_flags(KonquestNpcData* data, int flags, int enabled);
void* get_door_path(int door_id);
RpGeometry* RpGeometryForAllMaterials(
    RpGeometry* geometry, KonquestMaterialCallback callback, void* data);
int get_konquest_game_mode(void);
void npc_ani_1_frame(void);
void add_npc(int npc_id);
void* memcpy(void* dst, const void* src, unsigned long size);
int check_skip_conversation_flag(void);
void snd_stop(unsigned int sound_handle);
float p_wait_for_dialog(void);
float p_do_lip_synch(void);
int is_time_a_greater_than_time_b(
    const KonquestTime* time_a, const KonquestTime* time_b);
void add_minutes_to_time(KonquestTime* time, int minutes);
void add_hours_to_time(KonquestTime* time, int hours);
void add_days_to_time(KonquestTime* time, int days);
void add_months_to_time(KonquestTime* time, int months);
void add_years_to_time(KonquestTime* time, int years);

void npc_force_state_for_npc(KonquestNpc* npc, int event_index);
static void npc_set_state_for_npc(KonquestNpc* npc, int event_index);
static void npc_set_path(
    KonquestNpc* npc, void* path, int table_index, int row_count, int flags,
    int travel_mode);
static RpMaterial* MaterialFindTextureWithRootString(
    RpMaterial* material, void* root_string);
static void npc_shadow_update(void);
static void load_model_for_npc(KonquestNpc* npc);
static void npc_notify_nearby_npcs_that_player_hit_someone(KonquestNpc* npc);

void npc_sleep_until_model_loaded(void) {
    KonquestCmdScriptView* script;
    CmdScript* saved_script;
    KonquestNpc* npc;

    npc = g_active_npc;
    if (npc == 0 || aproc->pid != 0xA014) {
        return;
    }

    npc->wait_ticks = 2.0f;
    npc = g_active_npc;
    if (npc == 0) {
        return;
    }

    npc->wait_ticks -= 1.0f;
    if (npc->wait_ticks <= 0.0f) {
        npc->wait_ticks = 0.0f;
        return;
    }

    script = (KonquestCmdScriptView*)active_cmdscript;
    script->state = 2;
    saved_script = active_cmdscript;
    npc = g_active_npc;
    cmdscript_step_backward();
    memcpy(
        npc->saved_script_state,
        ((KonquestCmdScriptView*)active_cmdscript)->execution_state,
        sizeof(npc->saved_script_state));
    npc->saved_script_position =
        ((KonquestCmdScriptView*)active_cmdscript)->position;
    npc->saved_script_stack_depth = get_script_stack_depth();
    active_cmdscript = saved_script;
}

void cleanup_npc_manager(void) {
    if (npc_manager_pdata != 0) {
        if (npc_manager_pdata->has_object != 0) {
            NpcManagerObject* object = npc_manager_pdata->object;

            object->destroy(npc_manager_pdata, object);
        }
        npc_manager_pdata = 0;
    }
    g_active_npc = 0;
}

/*
 * Soft ceiling: 82.921875% - the typed body is exact; MWCC uses split
 * saves/restores and different fixed-size-copy/GPR scheduling from retail.
 */
void npc_set_wake_up_time(int unit, int amount) {
    if (is_time_a_greater_than_time_b(
            &konquest_pdata->current_time,
            &g_active_npc->wake_time) != 0) {
        memcpy(
            &g_active_npc->wake_time, &konquest_pdata->current_time,
            sizeof(KonquestTime));
        switch (unit) {
        case 0:
            add_minutes_to_time(&g_active_npc->wake_time, amount);
            return;
        case 1:
            add_hours_to_time(&g_active_npc->wake_time, amount);
            return;
        case 2:
            add_days_to_time(&g_active_npc->wake_time, amount);
            return;
        case 3:
            add_months_to_time(&g_active_npc->wake_time, amount);
            return;
        case 4:
            add_years_to_time(&g_active_npc->wake_time, amount);
            break;
        }
    }
}

void npc_set_my_conversation_counter(int count) {
    if (g_active_npc != 0) {
        g_active_npc->conversation_count = count;
    }
}

void npc_set_my_punch_counter(int count) {
    if (g_active_npc != 0) {
        g_active_npc->punch_count = count;
    }
}

void npc_reset_my_timed_events(void) {
    if (g_active_npc != 0) {
        g_active_npc->reset_timed_events = 1;
    }
}

void npc_reset_all_timed_events(void) {
    MkPtr* link;

    link = konquest_pdata->npc_list;
    while (link != 0) {
        KonquestNpc* npc;

        npc = (KonquestNpc*)link->hdr;
        if (link->instance != npc->hdr.instance) {
            MkPtr* next;

            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (npc != 0) {
                npc->reset_timed_events = 1;
            }
            link = link->next;
        }
    }
}

/* Matching: 95.52631% - zero-float relocation and equivalent latch branch
 * scheduling remain. */
void npc_switch_camera_focus(int focus_mode) {
    MkObj* focus;

    camera_set_movement_rate(0.0f);
    camera_set_rotation_rate(0.0f);
    switch (focus_mode) {
    case 2:
        focus = konquest_pdata->monk;
        if (focus != 0) {
            if (focus->hdr.instance != konquest_pdata->monk_instance) {
                focus = 0;
            }
        } else {
            focus = 0;
        }
        camera_set_lookat_focus(focus);
        camera_set_movement_focus_obj(g_active_npc->animation->object);
        break;
    case 0:
        camera_set_lookat_focus(g_active_npc->animation->object);
        focus = konquest_pdata->monk;
        if (focus != 0) {
            if (focus->hdr.instance != konquest_pdata->monk_instance) {
                focus = 0;
            }
        } else {
            focus = 0;
        }
        camera_set_movement_focus_obj(focus);
        break;
    case 1:
        focus = konquest_pdata->camera_target;
        if (focus != 0) {
            if (focus->hdr.instance !=
                konquest_pdata->camera_target_instance) {
                focus = 0;
            }
        } else {
            focus = 0;
        }
        camera_set_lookat_focus(focus);
        camera_set_movement_focus_obj(g_active_npc->animation->object);
        break;
    }
    camera_set_glitch_flag();
}

/*
 * Soft ceiling: 78.548386% - the typed body is exact; MWCC uses split
 * nonvolatile saves/restores and different GPR coloring from retail.
 */
void npc_glitch_to_ani(int animation_id, int flags) {
    AniData* animation;
    KonquestNpc* npc;

    npc = g_active_npc;
    animation = get_animation(animation_id);

    if (aproc->pid == 0xA014) {
        npc->queued_animation = animation;
        npc->animation_flags = flags;
        npc->queued_animation_frame = 0.0f;
    } else if (npc->animation != 0) {
        KonquestAnimPdata* animation_pdata =
            (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);

        set_anim_script(animation_pdata, animation, flags);
    }
}

/* Soft ceiling: 94.117645% - retail retains a redundant repeated null branch. */
void npc_set_anim_proc(MkProcEntryFn entry) {
    if (g_active_npc != 0 && g_active_npc->animation != 0) {
        xfer_proc(g_active_npc->animation->proc, entry);
    }
}

int npc_get_punch_count(void) {
    if (g_active_npc != 0) {
        return g_active_npc->punch_count;
    }
    return 0;
}

int npc_get_conversation_count(void) {
    if (g_active_npc != 0) {
        return g_active_npc->conversation_count;
    }
    return 0;
}

void npc_fire_trigger(void) {
    KonquestTrigger* trigger = find_trigger_by_id();

    if (g_active_npc != 0) {
        trigger->data->source_npc = g_active_npc;
        trigger->data->source_type = 3;
        execute_trigger(trigger);
    }
}

/* Soft ceiling: 99.82758% - only the -1.0f relocation differs. */
static float p_npc_waypoint_script(void) {
    KonquestWaypointScriptPdata* pdata =
        (KonquestWaypointScriptPdata*)pdata_of_proc(aproc);

    if (pdata->function_index != 0) {
        cmdscript_set_parameters(
            active_cmdscript, 1, pdata->npc->data);
        cmdscript_setup_execution(
            konquest_pdata->waypoint_script, pdata->function_index);
        cmdscript_execute(konquest_pdata->waypoint_script);
    }
    return -1.0f;
}

KonquestNpcData* get_active_npc_data(void) {
    KonquestNpcData* data = 0;

    if (g_active_npc != 0) {
        data = g_active_npc->data;
    }
    return data;
}

/*
 * Matching: 99.86842% - instructions are exact; only the 1.0f relocation
 * differs.
 */
static int npc_dialog_wait_for_widescreen_bars(void) {
    if (konquest_pdata->dialog_ready != 0) {
        return 1;
    }
    if (find_mkproc_pid(0x8229) == 0) {
        return 0;
    }
    while (konquest_pdata->dialog_ready == 0) {
        npc_ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return 1;
}

/* Soft ceiling: 88.42105% - typed active-animation boolean lowers differently. */
void npc_set_gravity(float gravity) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->gravity = gravity;
    }
}

void npc_set_my_ground_level(float ground_level) {
    g_active_npc->animation->object->ground_colls_y = ground_level;
}

int npc_get_flag_state(int flags) {
    return g_active_npc->flags & flags;
}

/*
 * Soft ceiling: 76.0% - the typed body is exact; MWCC splits the r30/r31
 * saves instead of using retail's stmw/lmw pair.
 */
void npc_at_waypoint_set_flags(int flags, int enabled) {
    KonquestWaypointScriptPdata* pdata =
        (KonquestWaypointScriptPdata*)pdata_of_proc(aproc);

    if (pdata != 0) {
        npc_set_his_flags(pdata->npc->data, flags, enabled);
    }
}

void npc_ignore_events(int enabled) {
    if (enabled != 0) {
        g_active_npc->ignore_events = 1;
        return;
    }
    g_active_npc->ignore_events = 0;
}

void npc_set_flags(int flags, int enabled) {
    if (enabled != 0) {
        g_active_npc->flags |= flags;
        return;
    }
    g_active_npc->flags &= ~flags;
}

void npc_set_dialog_anim(int animation) {
    if (g_active_npc->animation != 0) {
        g_active_npc->animation->dialog_anim = animation;
    }
}

void hero_handle_conversation(void) {
    for (;;) {
        KonquestNpcPdata* pdata = konquest_pdata;

        nis_wait_for_event(pdata->conversation_event, -1);
        if (pdata->conversation_mode_a != 2) {
            nis_wait_for_event(konquest_pdata->conversation_event + 1, -1);
            continue;
        }

        xfer_proc(g_active_npc->animation->proc, p_anim_idle);
        npc_play_dialog_and_anim_sequence(
            pdata->conversation_state_a, pdata->conversation_state_b);
        nis_signal_event(konquest_pdata->conversation_event + 1);
        konquest_pdata->conversation_event += 2;
    }
}

void conversation_term(void) {
    konquest_pdata->conversation_state_a = 0;
    konquest_pdata->conversation_state_b = 0;
    konquest_pdata->conversation_mode_a = 0;
    konquest_pdata->conversation_mode_b = 0;
    konquest_pdata->conversation_event = 0;
    resume_hero_state_process();
}

void conversation_init(int mode) {
    konquest_pdata->conversation_state_a = 0;
    konquest_pdata->conversation_state_b = 0;
    konquest_pdata->conversation_mode_a = mode;
    konquest_pdata->conversation_mode_b = mode;
    konquest_pdata->conversation_event = 0;
}

/*
 * Soft ceiling: 90.75572% - the typed body is exact; the proc-pointer boolean
 * lowering, split nonvolatile saves/restores, and 1.0f relocation differ.
 */
void npc_wait_for_dialog(void) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;
    int skip_wait;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation == 1) {
        skip_wait = !g_active_npc->wait_for_animation;
    } else {
        skip_wait = 1;
    }

    if (skip_wait == 0) {
        MkProc* wait_proc = _create_mkproc_generic_bigstack(
            0x9025, 0x1F, p_wait_for_dialog, 0, 0);

        if (wait_proc != 0) {
            mk_insert_no_own(
                &g_active_npc->hdr, &wait_proc->pdata_list);
            while (find_mkproc_pid(0x9025) != 0) {
                if (check_skip_conversation_flag() != 0) {
                    if (&active_proc_list != 0) {
                        MkPtr* link = active_proc_list;

                        while (link != 0) {
                            MkProc* proc = (MkProc*)link->hdr;

                            if (link->instance !=
                                (unsigned int)proc->instance) {
                                MkPtr* next = link->next;

                                link->hdr = 0;
                                destroy_mkptr(link);
                                link = next;
                            } else {
                                if (proc->pid == 0x8232) {
                                    KonquestLipSyncPdata* lip =
                                        (KonquestLipSyncPdata*)
                                            pdata_of_proc(proc);
                                    AniTextureControl* texture = lip->texture;
                                    unsigned int sound_handle =
                                        lip->sound_handle;

                                    if (texture != 0) {
                                        if ((unsigned int)texture->instance ==
                                            lip->texture_instance) {
                                            /* The latch is still live. */
                                        } else {
                                            texture = 0;
                                        }
                                    } else {
                                        texture = 0;
                                    }
                                    if (sound_handle != 0) {
                                        snd_stop(sound_handle);
                                    }
                                    if (lip->mode == 1 && texture != 0) {
                                        set_ani_texture_frame(texture, 0);
                                    }
                                    if ((unsigned int)proc->instance != 0) {
                                        KonquestProcDestroyVtable* vtable =
                                            (KonquestProcDestroyVtable*)
                                                proc->vtbl;

                                        vtable->destroy(proc, vtable);
                                    }
                                }
                                link = link->next;
                            }
                        }
                    }
                    destroy_mkprocs_pid(0x9025);
                    destroy_mkprocs_pid(0xA012);
                    return;
                }
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            }
        }
    }
}

void npc_stop_goro_bone_match(void) {
    MkProc* proc = find_mkproc_pid(0x500F);

    if (proc != 0) {
        destroy_mkproc_nostack(proc);
    }
}

void vdestroy_path_data_struct(KonquestPathData* path) {
    path->hdr.instance = 0;
    mkhdr_memfree(&path->hdr);
}

/* Matching: 99.8% - code is exact; only the 1.0f relocation differs. */
KonquestPathData* get_new_path_data_struct(void) {
    KonquestPathData* path = (KonquestPathData*)get_mkhdr(
        &vtbl_path_data_struct, sizeof(KonquestPathData));

    if (path != 0) {
        zero_pdata_payload(sizeof(KonquestPathData), &path->hdr);
        path->travel_mode = 0;
        path->speed = 1.0f;
        path->script_state = -2;
    }
    return path;
}

void vdestroy_konquest_npc_struct(KonquestNpc* npc) {
    npc->hdr.instance = 0;
    mkhdr_memfree(&npc->hdr);
}

void npc_signal_event(KonquestNpc* npc, int event_index) {
    if (!npc->ignore_events) {
        npc_set_state_for_npc(npc, event_index);
    }
}

/* Soft ceiling: 92.413795% - typed active-animation boolean lowers differently. */
void npc_set_my_ang_y(float angle) {
    KonquestNpcAnimState* state;
    int has_active_animation;

    g_active_npc->data->angle_y = angle;
    state = g_active_npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkObj* object = state->object;

        object->hide_flag_bits.pin_animation = 0;
        g_active_npc->animation->object->ang.y = angle;
    }
}

/*
 * Soft ceiling: 96.14035% - the typed body is exact; proc-pointer boolean
 * lowering and the nonvolatile save form differ.
 */
void npc_set_my_world_pos(float x, float y, float z) {
    Vec position;
    KonquestNpc* npc = g_active_npc;
    KonquestNpcAnimState* state;
    int has_active_animation;

    position.x = x;
    position.y = y;
    position.z = z;
    npc->data->position.x = position.x;
    npc->data->position.y = position.y;
    npc->data->position.z = position.z;
    npc->tile_index = get_tile_from_position(&position);

    state = npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->hide_flag_bits.pin_animation = 0;
        npc->animation->object->pos.x = position.x;
        npc->animation->object->pos.y = position.y;
        npc->animation->object->pos.z = position.z;
    }
}

/* Soft ceiling: 97.25% - typed active-animation boolean lowers differently. */
void npc_set_my_pos(float x, float y, float z) {
    KonquestTileOrigin* tile =
        get_nth_tile_struct(g_active_npc->tile_index);
    KonquestNpc* npc = g_active_npc;
    Vec position;
    KonquestNpcAnimState* state;
    int has_active_animation;

    position.x = x + tile->origin.x;
    position.y = y + tile->origin.y;
    position.z = z + tile->origin.z;
    npc->data->position.x = position.x;
    npc->data->position.y = position.y;
    npc->data->position.z = position.z;
    npc->tile_index = get_tile_from_position(&position);

    state = npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkObj* object = state->object;

        object->hide_flag_bits.pin_animation = 0;
        npc->animation->object->pos.x = position.x;
        npc->animation->object->pos.y = position.y;
        npc->animation->object->pos.z = position.z;
    }
}

/*
 * Soft ceiling: 83.731346% - the typed body is exact; nonvolatile GPR
 * coloring, split saves/restores, and an equivalent latch branch differ.
 */
void kill_lip_sync_procs(void) {
    MkPtr* link;

    if (&active_proc_list != 0) {
        link = active_proc_list;
        while (link != 0) {
            MkHdr* hdr = link->hdr;

            if (link->instance != hdr->instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                MkProc* proc = (MkProc*)hdr;

                if (proc->pid == 0x8232) {
                    KonquestLipSyncPdata* lip =
                        (KonquestLipSyncPdata*)pdata_of_proc(proc);
                    AniTextureControl* texture = lip->texture;
                    unsigned int sound_handle = lip->sound_handle;

                    if (texture != 0) {
                        if ((unsigned int)texture->instance ==
                            lip->texture_instance) {
                            /* The latched texture is still live. */
                        } else {
                            texture = 0;
                        }
                    } else {
                        texture = 0;
                    }
                    if (sound_handle != 0) {
                        snd_stop(sound_handle);
                    }
                    if (lip->mode == 1 && texture != 0) {
                        set_ani_texture_frame(texture, 0);
                    }
                    if ((unsigned int)proc->instance != 0) {
                        KonquestProcDestroyVtable* vtable =
                            (KonquestProcDestroyVtable*)proc->vtbl;

                        vtable->destroy(proc, vtable);
                    }
                }
                link = link->next;
            }
        }
    }
}

/*
 * Soft ceiling: 89.84127% - the typed body is exact; local GPR coloring,
 * split saves/restores, and an equivalent latch branch differ.
 */
void npc_lip_synch(int sound_id, LipSyncKeyframe* keyframes) {
    KonquestLipSyncPdata* lip;

    if (g_active_npc->animation != 0 && sound_id != -1 &&
        _create_mkproc_generic_nostack(
            0x8232, 0x1F, p_do_lip_synch, sizeof(*lip),
            (MkHdr**)&lip) != 0) {
        AniTextureControl* texture;
        KonquestNpcAnimState* animation;
        KonquestLipSyncPdata* target;
        unsigned int texture_instance;

        zero_pdata_payload(sizeof(*lip), &lip->hdr);
        lip->mode = 1;
        lip->npc = g_active_npc;
        target = lip;
        animation = g_active_npc->animation;
        texture = animation->lip_texture;
        texture_instance = animation->lip_texture_instance;
        target->texture = texture;
        target->texture_instance = texture_instance;
        lip->sound_handle = sound_id;

        texture = lip->texture;
        if (texture != 0) {
            if ((unsigned int)texture->instance ==
                lip->texture_instance) {
                /* The latched texture is still live. */
            } else {
                texture = 0;
            }
        } else {
            texture = 0;
        }
        if (texture != 0) {
            lip->keyframes = keyframes;
        }
        lip->stop_requested = 0;
    }
}

/*
 * Soft ceiling: 98.98305% - the 2.0f relocation and equivalent FPR coloring
 * differ.
 */
void npc_travel_to_world_position(Vec* position, int travel_mode) {
    if (g_active_npc != 0 && g_active_npc->path != 0) {
        KonquestPathData* path = g_active_npc->path;
        float delta_z;
        float delta_x;

        path->destination.x = position->x;
        g_active_npc->path->destination.y = position->y;
        g_active_npc->path->destination.z = position->z;
        delta_z = g_active_npc->data->position.z - position->z;
        delta_x = g_active_npc->data->position.x - position->x;
        if (delta_x * delta_x + delta_z * delta_z > 2.0f) {
            g_active_npc->path->current_waypoint = -1;
            g_active_npc->path->target_waypoint = 0;
            g_active_npc->path->previous_waypoint = -1;
            g_active_npc->path->travel_state = 1;
            npc_travel_path(0x80000007, 0, travel_mode);
        }
        g_active_npc->path->script_state = -1;
    }
}

void npc_travel_path_anim_override(
    int path_id, int path_arg, int animation, int travel_mode) {
    if (g_active_npc != 0) {
        g_active_npc->path->use_animation_override = 1;
        g_active_npc->path->animation_override = animation;
        npc_travel_path(path_id, path_arg, travel_mode);
    }
}

void npc_set_my_movement_weight(float root_weight, float object_weight) {
    if (g_active_npc->animation != 0) {
        AnimState* animation = (AnimState*)pdata_of_proc(
            g_active_npc->animation->proc);

        set_root_and_obj_movement_weights(
            root_weight, object_weight, animation);
    }
}

/* Soft ceiling: 89.52381% - typed active-animation boolean lowers differently. */
void npc_set_snap_to_ground(int enabled) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->flags_09_bits.bit6 = enabled;
    }
}

/* Matching: 99.0% - code is exact; only the TU-local float relocation differs. */
void npc_set_wait_ticks(float ticks) {
    g_active_npc->wait_ticks = ticks + 1.0f;
}

/* Soft ceiling: 92.97298% - typed validity lowering and GPR coloring differ. */
void npc_set_ani_frame(float frame) {
    KonquestNpc* npc = g_active_npc;
    KonquestNpcAnimState* state = npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }

    if (has_active_animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(state->proc);

        animation->frame = frame;
        if (frame > animation->high_frame) {
            animation->frame = animation->high_frame;
        }
    } else {
        npc->queued_animation_frame = frame;
    }
}

void npc_set_ani_flags(unsigned int flags) {
    if (g_active_npc->animation != 0 &&
        g_active_npc->animation->proc != 0) {
        AnimPdata* animation =
            (AnimPdata*)pdata_of_proc(g_active_npc->animation->proc);

        animation->flags |= flags;
        return;
    }
    g_active_npc->animation_flags |= flags;
}

/*
 * Soft ceiling: 91.74775% - typed validity lowering, GPR save form, and the
 * zero-float relocation differ.
 */
void npc_blend_to_ani_with_offset(
    int animation_id, int flags, float blend, float step) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }

    if (has_active_animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(state->proc);
        MkObj* object;
        int skip_sleep;

        animation->step = step;
        transition_to_anim_script(
            animation, get_animation(animation_id), flags, blend);
        object = g_active_npc->animation->object;
        object->pos.x += animation->root_offset.x;
        object = g_active_npc->animation->object;
        object->pos.y += animation->root_offset.y;
        object = g_active_npc->animation->object;
        object->pos.z += animation->root_offset.z;

        state = g_active_npc->animation;
        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation == 1) {
            skip_sleep = !g_active_npc->wait_for_animation;
        } else {
            skip_sleep = 1;
        }
        if (skip_sleep == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    } else {
        g_active_npc->queued_animation = get_animation(animation_id);
        g_active_npc->animation_flags = flags;
        g_active_npc->queued_animation_frame = 0.0f;
    }
}

/*
 * Soft ceiling: 88.5812% - nonvolatile GPR coloring/save form, typed
 * active-animation boolean lowering, and float relocations differ.
 */
static float p_turn_and_face(void) {
    MkObj* target_object;
    int use_npc_position;
    TurnAndFacePdata* pdata;
    float target_angle;

    pdata = (TurnAndFacePdata*)pdata_of_proc(aproc);
    target_object = 0;
    use_npc_position = 0;
    if (pdata->target_kind == 0x9003) {
        target_object = pdata->target.object;
    } else {
        KonquestNpc* target_npc = pdata->target.npc;
        KonquestNpcAnimState* state = target_npc->animation;
        int has_active_animation;
        int use_data;

        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation == 1) {
            use_data = !target_npc->wait_for_animation;
        } else {
            use_data = 1;
        }
        if (use_data != 0) {
            use_npc_position = 1;
        } else {
            target_object = state->object;
        }
    }

    if (pdata->use_angle != 0) {
        target_angle = pdata->angle;
    } else {
        float destination_x = pdata->position.x;
        float destination_z = pdata->position.z;
        float target_x;
        float target_z;

        if (use_npc_position != 0) {
            target_x = pdata->target.npc->data->position.x;
            target_z = pdata->target.npc->data->position.z;
        } else {
            target_x = target_object->pos.x;
            target_z = target_object->pos.z;
        }
        target_angle = gxMathArcTanYX(
            destination_x - target_x, destination_z - target_z);
    }

    if (use_npc_position != 0) {
        pdata->target.npc->data->angle_y = target_angle;
        return -1.0f;
    }
    if (get_konquest_game_mode() == 4) {
        target_object->ang.y = target_angle;
        return -1.0f;
    }

    {
        float difference = ang_sub_ang(target_angle, target_object->ang.y);
        float magnitude;

        target_object->hide_flag_bits.pin_animation = 0;
        if (difference >= 0.0f) {
            magnitude = difference;
        } else {
            magnitude = -difference;
        }
        if (magnitude > 0.1f) {
            if (difference < 0.0f) {
                target_object->ang.y -= 0.1f;
            } else {
                target_object->ang.y += 0.1f;
            }
            return 1.0f;
        }
    }
    target_object->ang.y = target_angle;
    return -1.0f;
}

/* Soft ceiling: 89.52381% - typed active-animation boolean lowers differently. */
void npc_set_pinanim_flag(int enabled) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkObj* object = state->object;

        object->hide_flag_bits.pin_animation = enabled;
    }
}

/*
 * Matching: 99.479164% - instructions are exact; only TU-local float
 * relocations differ.
 */
static float p_hero_turn_and_face(void) {
    TurnAndFacePdata* pdata = (TurnAndFacePdata*)pdata_of_proc(aproc);
    float target_angle = pdata->angle;
    MkObj* object = pdata->target.object;
    float difference = ang_sub_ang(target_angle, object->ang.y);
    float magnitude;

    object->hide_flag_bits.pin_animation = 0;
    if (difference >= 0.0f) {
        magnitude = difference;
    } else {
        magnitude = -difference;
    }
    if (magnitude > 0.1f) {
        if (difference < 0.0f) {
            object->ang.y -= 0.1f;
        } else {
            object->ang.y += 0.1f;
        }
        return 1.0f;
    }
    object->ang.y = target_angle;
    return -1.0f;
}

/*
 * Soft ceiling: 87.396225% - typed latch joins, active-animation boolean
 * lowering, pdata reload scheduling, and the 1.0f relocation differ.
 */
void npc_turn_and_face_angle(KonquestNpc* npc, float angle) {
    MkProc* turn_proc = npc->turn_proc;

    if (turn_proc != 0) {
        if ((unsigned int)turn_proc->instance ==
            npc->turn_proc_instance) {
            /* The latched process is still live. */
        } else {
            turn_proc = 0;
        }
    } else {
        turn_proc = 0;
    }
    if (turn_proc == 0) {
        KonquestNpcAnimState* state = npc->animation;
        int has_active_animation;
        int use_data;

        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation == 1) {
            use_data = !npc->wait_for_animation;
        } else {
            use_data = 1;
        }
        if (use_data != 0) {
            npc->data->angle_y = angle;
            return;
        }

        {
            TurnAndFacePdata* pdata;

            turn_proc = _create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_turn_and_face,
                sizeof(TurnAndFacePdata), (MkHdr**)&pdata);
            if (turn_proc != 0) {
                pdata->angle = angle;
                pdata->saved_pin_animation =
                    state->object->hide_flag_bits.pin_animation;
                pdata->target_kind = 0xA002;
                pdata->target.npc = npc;
                pdata->use_angle = 1;
                npc->turn_proc = turn_proc;
                npc->turn_proc_instance = turn_proc->instance;
            }
        }

        while ((turn_proc = npc->turn_proc) != 0 &&
               (unsigned int)turn_proc->instance ==
                   npc->turn_proc_instance) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/*
 * Soft ceiling: 83.08187% - typed latch joins/reloads, active-animation
 * boolean lowering, nonvolatile save form, and the 1.0f relocation differ.
 */
void npc_turn_and_face_player(int turn_player) {
    MkObj* monk = konquest_pdata->monk;
    KonquestNpcAnimState* state;
    int has_active_animation;

    if (monk != 0) {
        if ((unsigned int)monk->hdr.instance ==
            konquest_pdata->monk_instance) {
            /* The latched object is still live. */
        } else {
            monk = 0;
        }
    } else {
        monk = 0;
    }

    state = g_active_npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkProc* turn_proc = g_active_npc->turn_proc;
        TurnAndFacePdata* pdata;

        if (turn_proc != 0) {
            if ((unsigned int)turn_proc->instance ==
                g_active_npc->turn_proc_instance) {
                /* The latched process is still live. */
            } else {
                turn_proc = 0;
            }
        } else {
            turn_proc = 0;
        }
        if (turn_proc == 0) {
            turn_proc = _create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_turn_and_face,
                sizeof(TurnAndFacePdata), (MkHdr**)&pdata);
            if (turn_proc != 0) {
                pdata->target_kind = 0xA002;
                pdata->target.npc = g_active_npc;
                pdata->saved_pin_animation =
                    state->object->hide_flag_bits.pin_animation;
                state->object->hide_flag_bits.pin_animation = 0;
                pdata->use_angle = 0;
                pdata->position.x = monk->pos.x;
                pdata->position.y = monk->pos.y;
                pdata->position.z = monk->pos.z;
                g_active_npc->turn_proc = turn_proc;
                g_active_npc->turn_proc_instance = turn_proc->instance;
            }
        }

        if (turn_player != 0) {
            turn_proc = _create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_turn_and_face,
                sizeof(TurnAndFacePdata), (MkHdr**)&pdata);
            if (turn_proc != 0) {
                pdata->target.object = monk;
                pdata->saved_pin_animation =
                    monk->hide_flag_bits.pin_animation;
                monk->hide_flag_bits.pin_animation = 0;
                pdata->target_kind = 0x9003;
                pdata->use_angle = 0;
                pdata->position.x = state->object->pos.x;
                pdata->position.y = state->object->pos.y;
                pdata->position.z = state->object->pos.z;
            }
        }

        while ((turn_proc = g_active_npc->turn_proc) != 0 &&
               (unsigned int)turn_proc->instance ==
                   g_active_npc->turn_proc_instance) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

void npc_shove_reaction_standard_shutdown(void) {
    g_active_npc->reaction_active = 0;
    g_active_npc->ignore_events = 0;
    g_active_npc->reaction_mode = 0;
    if (g_active_npc->path != 0) {
        g_active_npc->path->reaction_state = -1;
    }
}

/*
 * Soft ceiling: 91.163635% - the typed body is exact; MWCC uses split
 * nonvolatile saves/restores, and the float relocations differ.
 */
void npc_run_shove_animation(int animation_id) {
    KonquestAnimPdata* animation_pdata =
        (KonquestAnimPdata*)pdata_of_proc(g_active_npc->animation->proc);
    AniData* animation = get_animation(animation_id);

    if (animation != 0) {
        float final_frame = anim_script_lastframe(animation) - 10.0f;

        transition_to_anim_script(animation_pdata, animation, 3, 0.1f);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        while (animation_pdata->frame < final_frame) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/*
 * Matching: 99.88372% - code is exact; only the zero-float relocation differs.
 */
void npc_shove_reaction_standard_setup(void) {
    g_active_npc->reaction_active = 1;
    g_active_npc->ignore_events = 1;
    g_active_npc->reaction_mode = 1;
    if (g_active_npc->animation != 0 &&
        g_active_npc->animation->object != 0) {
        g_active_npc->animation->object->pos_vel.z = 0.0f;
        g_active_npc->animation->object->pos_vel.y = 0.0f;
        g_active_npc->animation->object->pos_vel.x = 0.0f;
        g_active_npc->animation->object->ang_vel.z = 0.0f;
        g_active_npc->animation->object->ang_vel.y = 0.0f;
        g_active_npc->animation->object->ang_vel.x = 0.0f;
    }
}

/*
 * Matching: 96.94444% - the validated monk latch uses an equivalent branch
 * schedule.
 */
int npc_punch_reaction_check_data(void) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);
    MkObj* monk = konquest_pdata->monk;

    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (reaction == 0 || monk == 0) {
        return 0;
    }
    if (reaction->npc == 0 || reaction->object == 0) {
        return 0;
    }
    return 1;
}

/*
 * Matching: 99.71591% - instructions are exact; only float relocations differ.
 */
void npc_run_punch_animation(
    int animation_id, int flags, int unused, int use_blend,
    void* script_args, float gravity) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);

    if (reaction != 0) {
        KonquestAnimPdata* animation_pdata =
            (KonquestAnimPdata*)pdata_of_proc(reaction->animation_proc);

        if (animation_pdata != 0) {
            AniData* animation = get_animation(animation_id);

            if (animation != 0) {
                float final_frame =
                    anim_script_lastframe(animation) - 10.0f;

                if (use_blend != 0) {
                    transition_to_anim_script(
                        animation_pdata, animation, flags, 0.05f);
                } else {
                    set_anim_script(animation_pdata, animation, 3);
                }
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                reaction->object->pos.y = animation_pdata->root_offset.y;
                reaction->object->gravity = gravity;

                while (animation_pdata->frame < final_frame) {
                    if (gravity != 0.0f) {
                        reaction->object->flags_08_bits.moving = 1;
                    }
                    npc_ani_1_frame();
                    _mkproc_sleep_ticks = 1.0f;
                    ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                }
            }
        }
    }

    (void)unused;
    (void)script_args;
}

/*
 * Matching: 97.25% - the validated monk latch uses an equivalent branch
 * schedule.
 */
void npc_snap_to_face_monk(void) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);
    MkObj* monk = konquest_pdata->monk;

    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (monk != 0 && reaction != 0 && reaction->object != 0) {
        reaction->object->ang.y = gxMathArcTanYX(
            monk->pos.x - reaction->object->pos.x,
            monk->pos.z - reaction->object->pos.z);
    }
}

/*
 * Matching: 94.5% - the animation-presence boolean uses an equivalent
 * three-instruction normalization sequence.
 */
void npc_punch_reaction_standard_shutdown(void) {
    KonquestNpcAnimState* animation;
    int has_animation;

    g_active_npc->reaction_active = 0;
    g_active_npc->ignore_events = 0;
    g_active_npc->reaction_mode = 0;
    if (g_active_npc->path != 0) {
        g_active_npc->path->reaction_state = -1;
    }

    animation = g_active_npc->animation;
    if (animation == 0) {
        has_animation = 0;
    } else if (animation->object == 0) {
        has_animation = 0;
    } else {
        has_animation = animation->proc != 0;
    }
    if (has_animation != 0) {
        animation->object->ground_colls = &npc_ground_colls;
    }
}

void npc_prepare_for_unconscious_state(void) {
    g_active_npc->reaction_active = 0;
    g_active_npc->ignore_events = 1;
    g_active_npc->reaction_mode = 0;
    if (g_active_npc->path != 0) {
        g_active_npc->path->reaction_state = -1;
    }
}

/*
 * Matching: 99.88372% - code is exact; only the zero-float relocation differs.
 */
void npc_punch_reaction_standard_setup(void) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);

    if (reaction->object != 0) {
        reaction->object->pos_vel.z = 0.0f;
        reaction->object->pos_vel.y = 0.0f;
        reaction->object->pos_vel.x = 0.0f;
        reaction->object->ang_vel.z = 0.0f;
        reaction->object->ang_vel.y = 0.0f;
        reaction->object->ang_vel.x = 0.0f;
        reaction->object->ground_colls = &npc_punched_ground_colls;
    }
    g_active_npc->reaction_active = 1;
    g_active_npc->ignore_events = 1;
    g_active_npc->reaction_mode = 1;
    npc_notify_nearby_npcs_that_player_hit_someone(g_active_npc);
}

void npc_set_ani_speed(float speed) {
    g_active_npc->animation_speed = speed;
    if (g_active_npc->animation != 0) {
        AnimPdata* animation =
            (AnimPdata*)pdata_of_proc(g_active_npc->animation->proc);

        animation->step = speed;
    }
}

/*
 * Matching: 99.72222% - code is exact; only the integer-conversion constant
 * relocation differs in this partial TU.
 */
void npc_change_path_speed(float speed) {
    if (g_active_npc->path != 0) {
        g_active_npc->path->speed = (float)(int)speed;
    }
}

/* Matching: 97.5% - code is exact; only the return float relocation differs. */
float p_npc_idle(void) {
    return 1.0f;
}

/*
 * Matching: 99.545456% - code is exact; only the -1.0f relocation differs.
 */
static float p_npc_load_model(void) {
    KonquestModelLoadPdata* pdata = (KonquestModelLoadPdata*)apdata;

    load_model_for_npc(pdata->npc);
    return -1.0f;
}

static void npc_set_state_for_npc(KonquestNpc* npc, int event_index) {
    if (npc->events[event_index].enabled != 0) {
        npc_force_state_for_npc(npc, event_index);
    }
}

/* Matching: 99.7619% - code is exact; only the 1.0f relocation differs. */
static void npc_shoved_setup(KonquestNpc* npc) {
    if (npc->animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);

        npc->ignore_events = 1;
        animation->step = 1.0f;
    }
}

static void npc_shoved_cleanup(KonquestNpc* npc) {
    npc->ignore_events = 0;
    npc->reaction_active = 0;
}

static void npc_override_cleanup(KonquestNpc* npc) {
    npc->animation_override = 0;
    npc->ignore_events = 0;
    npc->reaction_active = 0;
}

static void npc_override_setup(KonquestNpc* npc) {
    npc->animation_override = 1;
    npc->ignore_events = 1;
    npc->reaction_active = 1;
}

/*
 * Soft ceiling: 94.23077% - typed active-animation boolean lowering and the
 * 1.0f relocation differ.
 */
static void npc_punched_setup(KonquestNpc* npc) {
    KonquestNpcAnimState* state = npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(state->proc);

        npc->animation->object->flags_09_bits.bit6 = 0;
        npc->ignore_events = 1;
        animation->step = 1.0f;
    }
}

static void npc_plyr_violent_setup(void) {
}

void npc_enable_event(int event_index, int enabled) {
    if (g_active_npc != 0 && event_index >= 0 && event_index < 7) {
        g_active_npc->events[event_index].enabled = enabled;
    }
}

/*
 * Soft ceiling: 72.5% - the typed loop is exact; nonvolatile save form and
 * load/update scheduling differ from retail.
 */
void add_npc_list_to_world(int* npc_ids) {
    unsigned int count = get_row_count_for_table_by_pointer(
        konquest_pdata->waypoint_script, npc_ids);
    unsigned int index = 0;
    int npc_id = *npc_ids;

    while (index < count) {
        add_npc(npc_id);
        npc_id = npc_ids[1];
        npc_ids++;
        index++;
    }
}

/*
 * Soft ceiling: 89.5% - the typed comparisons are exact; equivalent shared
 * current-time addressing and branch scheduling differ.
 */
static int is_this_a_current_event_for_today(
    const KonquestEventSchedule* event) {
    const KonquestTime* current = &konquest_pdata->current_time;

    if (event->year != -1 && event->year != current->year) {
        return 0;
    }
    if (event->month != -1 && event->month != current->month) {
        return 0;
    }
    if (event->day_of_month == -1 && event->day_of_week == -1) {
        return 1;
    }
    if (event->day_of_month == -1 &&
        event->day_of_week == current->day_of_week) {
        if (current->hour > event->hour) {
            return 1;
        }
        if (current->hour == event->hour &&
            current->minute >= event->minute) {
            return 1;
        }
        return 0;
    }
    if (event->day_of_month == current->day_of_month &&
        event->day_of_week == -1) {
        if (current->hour > event->hour) {
            return 1;
        }
        if (current->hour == event->hour &&
            current->minute >= event->minute) {
            return 1;
        }
        return 0;
    }
    if (event->day_of_month == current->day_of_month &&
        event->day_of_week == current->day_of_week) {
        if (current->hour > event->hour) {
            return 1;
        }
        if (current->hour == event->hour &&
            current->minute >= event->minute) {
            return 1;
        }
    }
    return 0;
}

/*
 * Soft ceiling: 73.89474% - the typed body is exact; array iteration GPR
 * coloring/address scheduling and split saves/restores differ.
 */
void npc_shadow_teardown(void) {
    int index;

    for (index = 0; index < 15; index++) {
        MkHdr* object = npc_shadows.objects[index];

        if (object != 0 && object->instance != 0) {
            KonquestDestroyVtable* vtable =
                (KonquestDestroyVtable*)object->vtbl;

            vtable->destroy(object);
        }
        npc_shadows.objects[index] = 0;
    }
    npc_shadows.active_count = 0;
    npc_shadows.initialized = 0;
}

/*
 * Matching: 99.44444% - code is exact; only the return-value float relocation
 * differs in this partial TU.
 */
static float p_update_npc_shadows(void) {
    npc_shadow_update();
    return 1.0f;
}

/* Matching: 99.96183% - the 0.01f literal relocation differs. */
static void append_oblique_projection(
    ObliqueMatrix result, ObliqueMatrix left, ObliqueMatrix right) {
    result[0].value =
        left[2].value * right[8].value +
        (left[0].value * right[0].value +
         left[1].value * right[4].value);
    result[1].value =
        left[2].value * right[9].value +
        (left[0].value * right[1].value +
         left[1].value * right[5].value);
    result[2].value =
        left[2].value * right[10].value +
        (left[0].value * right[2].value +
         left[1].value * right[6].value);

    result[4].value =
        left[6].value * right[8].value +
        (left[4].value * right[0].value +
         left[5].value * right[4].value);
    result[5].value =
        left[6].value * right[9].value +
        (left[4].value * right[1].value +
         left[5].value * right[5].value);
    result[6].value =
        left[6].value * right[10].value +
        (left[4].value * right[2].value +
         left[5].value * right[6].value);

    result[8].value =
        left[10].value * right[8].value +
        (left[8].value * right[0].value +
         left[9].value * right[4].value);
    result[9].value =
        left[10].value * right[9].value +
        (left[8].value * right[1].value +
         left[9].value * right[5].value);
    result[10].value =
        left[10].value * right[10].value +
        (left[8].value * right[2].value +
         left[9].value * right[6].value);

    result[3].flags = left[3].flags & right[3].flags;
    result[12].value =
        left[14].value * right[8].value +
        (left[12].value * right[0].value +
         left[13].value * right[4].value);
    result[13].value =
        left[14].value * right[9].value +
        (left[12].value * right[1].value +
         left[13].value * right[5].value);
    result[14].value =
        left[14].value * right[10].value +
        (left[12].value * right[2].value +
         left[13].value * right[6].value);
    result[13].value += 0.01f;
    result[3].flags = 3;
}

/*
 * Soft ceiling: 82.67857% - the typed body is exact; nonvolatile GPR coloring
 * and split saves differ from retail.
 */
void npc_assign_door_path(int door_id, int travel_mode) {
    void* path = get_door_path(door_id);

    if (path == 0) {
        npc_set_path(g_active_npc, 0, 0, 0, 0, travel_mode);
        return;
    }
    npc_set_path(
        g_active_npc, path, door_id, 4, 0x40000000, travel_mode);
}

/*
 * Soft ceiling: 74.85714% - the typed body is exact; MWCC splits the r28-r31
 * save/restore sequence instead of using retail's stmw/lmw pair.
 */
void npc_assign_path(void* path, int flags, int travel_mode) {
    if (g_active_npc != 0) {
        int row_count = 0;
        int table_index = 0;

        if (path != 0) {
            row_count = get_row_count_for_table_by_pointer(
                konquest_pdata->waypoint_script, path);
            table_index = get_table_index_by_pointer(
                konquest_pdata->waypoint_script, path);
        }
        npc_set_path(
            g_active_npc, path, table_index, row_count, flags, travel_mode);
    }
}

static RpAtomic* AtomicFindTextureWithRootString(
    RpAtomic* atomic, void* root_string) {
    if (atomic->geometry != 0) {
        RpGeometryForAllMaterials(
            atomic->geometry, MaterialFindTextureWithRootString, root_string);
    }
    return atomic;
}

static void material_restore_texture_pointer(RpMaterial* material) {
    SpecularMaterialPluginData* spec =
        mk_get_specular_material_plugin(material);

    if (spec != 0) {
        material->texture = spec->saved_texture;
    }
}

static void material_store_texture_pointer(RpMaterial* material) {
    SpecularMaterialPluginData* spec =
        mk_get_specular_material_plugin(material);

    if (spec != 0) {
        spec->saved_texture = material->texture;
    }
}

/*
 * Soft ceiling: 86.07692% - the typed visibility and distance algorithm is
 * exact; FPR/stack scheduling, save form, and float relocations differ.
 */
static int npc_check_visibility_and_calc_dist(KonquestNpc* npc) {
    if (npc->reaction_active) {
        KonquestNpcData* data = npc->data;
        CameraObj* camera = camera_obj;
        float delta_x;
        float delta_y;
        float delta_z;

        delta_y = data->position.y - camera->pos_y;
        delta_x = data->position.x - camera->pos_x;
        delta_z = data->position.z - camera->pos_z;
        npc->camera_distance_squared =
            delta_z * delta_z +
            (delta_x * delta_x + delta_y * delta_y);
        return 1;
    }
    if (npc->skip_visibility) {
        return 0;
    }

    {
        KonquestTileOrigin* tile = get_nth_tile_struct(npc->tile_index);
        KonquestNpcData* data;
        RwSphere sphere;
        RwMatrix* matrix;
        float direction_x;
        float direction_y;
        float direction_z;
        float inverse_length = 0.0f;
        float length_squared;

        if (tile == 0 || tile->loaded == 0) {
            return 0;
        }

        data = npc->data;
        sphere.center.x = data->position.x;
        sphere.center.y = data->position.y;
        sphere.center.z = data->position.z;
        matrix = camera_obj->field_24;
        direction_y = matrix->at.y;
        direction_x = matrix->at.x;
        direction_z = matrix->at.z;
        length_squared =
            direction_z * direction_z +
            (direction_x * direction_x + direction_y * direction_y);
        if (length_squared > 0.0f) {
            KonquestFloatBits estimate;
            float product;
            float correction;

            estimate.value = length_squared;
            estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
            product = estimate.value *
                      (length_squared * estimate.value);
            correction = 3.0f - product;
            inverse_length =
                0.0625f * estimate.value * correction *
                -((correction * (product * correction)) - 12.0f);
        }

        direction_x *= inverse_length;
        direction_y *= inverse_length;
        direction_z *= inverse_length;
        sphere.center.x += 3.0f * direction_x;
        sphere.center.y += 3.0f * direction_y;
        sphere.center.z += 3.0f * direction_z;
        sphere.radius = 1.0f;
        if (RwCameraFrustumTestSphere(Camera, &sphere) == 0) {
            return 0;
        }
    }

    {
        KonquestNpcData* data = npc->data;
        CameraObj* camera = camera_obj;
        float delta_x = data->position.x - camera->pos_x;
        float delta_y = data->position.y - camera->pos_y;
        float delta_z = data->position.z - camera->pos_z;
        float distance_squared =
            delta_z * delta_z +
            (delta_x * delta_x + delta_y * delta_y);

        npc->camera_distance_squared = distance_squared;
        if (distance_squared > 2500.0f) {
            return 0;
        }
        return 1;
    }
}

void initialize_npc_data(void) {
    g_active_npc = 0;
    if (konquest_pdata->load_config->art_section_name != 0) {
        load_art_section_by_name(
            0x6002B, konquest_pdata->load_config->art_section_name);
        unload_section_slot(0x6002E);
        add_anim_section_by_name_async(
            0x6002E,
            konquest_pdata->load_config->animation_section_name,
            npc_fast_anims, 0, 1);
        wait_for_slot_load(0x6002E);
        if (konquest_editor_mode_on == 0 &&
            konquest_pdata->load_config->string_bank_name != 0) {
            load_string_bank(
                0x20000, konquest_pdata->load_config->string_bank_name);
        }
    }
}
