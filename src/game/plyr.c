#include "game/game_info.h"
#include "game/cloth.h"
#include "math/mk_math.h"
#include "runtime/plyr_info.h"

#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/anim_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"

#define LOAD_PLYR_MODEL_PID 0x9032

typedef AnimPdata PlyrAnimPdata;

typedef struct LoadPlyrModelPdata {
    MkHdr hdr;
    int player;
    int char_id;
    int flags;
} LoadPlyrModelPdata;

static PlyrPdata _mkpdata_plyrs[PLYR_PDATA_POOL_COUNT];
void* memset(void* destination, int value, unsigned long size);

extern float p_load_plyr_model_async(void);
extern PlyrPdata* plyr_pdata;
extern MkObj* plyr_obj;
extern AnimPdata* plyr_anim_pdata;
extern MkProc* plyr_anim_proc;
extern MkProc* plyr_lefthand_anim_proc;
extern MkProc* plyr_righthand_anim_proc;
extern PlyrPdata* his_pdata;
extern MkObj* his_obj;
extern MkHdr* apdata_save;
extern int g_perform_validation;
extern MkVtable5 vtbl_mkpdata_plyr;
extern MkVtable5 vtbl_mkobj;
extern MslSoundHandle plyr_snd_req(int sound);
extern void snd_stop(MslSoundHandle handle);
extern int baraka_advance_active_moveset(void* moveset, void* context);
extern int destroy_mkpdata_plyr(void);
extern AnimPdata* anim_pdata;
extern void pose_anim(AnimPdata* animation, int update_object);
extern unsigned int create_mkproc_anim2(
    int proc_id, MkProcEntryFn entry, AnimPdata** pdata_out);
extern void* get_bone_with_tag(MkObj* object, int tag);
extern float p_baraka_jaw_controller(void);
extern float p_baraka_blades_controller(void);
extern void set_constrain_last_pos(int player, const Vec* position);
extern int is_weapon_style(PlyrFighterDefinition* style);
extern void plyr_weapon_show(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* slots);
extern void plyr_weapon_hide(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* slots);
extern void* p1_profile;
extern void* p2_profile;
extern int is_mark_as_unlocked(void* profile, int mark);
extern int is_load_meter_active(void);
extern void snd_req(int sound);
extern int damage_p1(float damage);
extern int damage_p2(float damage);
extern void drone_ai_watcher(void);
extern void setup_sound_banks(int bank);
extern void create_player(int player, PlyrInfo* info);
extern float p_plyr_start(void);
extern MkObj* load_weapon(MkObj* owner);
extern void plyr_aux_weapon_grab(PlyrPdata* pdata, MkObj* weapon);
extern void set_anim_script(
    AnimPdata* animation, MkProcEntryFn script, unsigned int flags);
extern unsigned int randu0(unsigned int range);
extern int is_char_locked(unsigned short character, int alternate);
extern void resolve_alternate_palettes(
    PlyrInfo* player2, PlyrInfo* player1);
extern MkHdr* start_bone_matcher(
    MkObj* source, int source_bone, MkObj* target, int target_bone,
    float weight);
extern void active_sidekick_swap(PlyrPdata* pdata, int mode);
extern float r_call_script_function(void);
extern float sqrtf(float value);
extern void fxbanks_unload_by_owner(int owner);
extern void TearDownShadow(PlyrPdata* pdata);
extern void kill_fstyle_signs_for_plyr(PlyrInfo* player);
extern void term_player_collision(PlyrInfo* player);
extern void unload_script(int slot);
extern void transition_to_anim_script(
    AnimPdata* animation, AniData* script, int flags, float blend);
extern void obj_set_gravity(MkObj* object, float gravity);
extern void snd_req_delay(int sound, int delay);

typedef struct BarakaJawPdata {
    MkHdr hdr;
    AnimPdata* animation;
    unsigned int animation_instance;
    int state;
    void* jaw_bone;
    char pad18[0x10];
    int command;
    char pad2C[8];
} BarakaJawPdata;

typedef struct BarakaBladesPdata {
    MkHdr hdr;
    MkBone* blade_a;
    MkBone* blade_b;
    float blade_a_target;
    float blade_b_target;
    float blade_a_step;
    float blade_b_step;
    int command;
} BarakaBladesPdata;

typedef struct PlyrScriptProcPdata {
    MkHdr hdr;
    unsigned int func_index;
    ScriptSlot* script;
} PlyrScriptProcPdata;

typedef struct PlyrProcVtable {
    void* reserved[6];
    void (*sleep)(MkProc* proc);
    void* stack_ops[2];
    int (*jump_sleep)(MkProcEntryFn entry, float ticks);
} PlyrProcVtable;

typedef struct PlyrClumpLink {
    struct PlyrClumpLink* next;
    struct PlyrClumpLink* previous;
} PlyrClumpLink;

typedef struct PlyrClumpNode {
    void* geometry;
    char pad04[0x24];
    PlyrClumpLink link;
} PlyrClumpNode;

typedef struct PlyrClumpView {
    char pad00[8];
    PlyrClumpLink geometry_list;
} PlyrClumpView;

typedef struct PlyrGeometryView {
    char pad00[0x20];
    RpMaterial** materials;
    unsigned int material_count;
} PlyrGeometryView;

typedef struct PlyrSpecularMaterialView {
    char pad00[0x2C];
    unsigned char flags_2C;
} PlyrSpecularMaterialView;

extern int MkmaterialLocalOffset;
extern int SpecularMaterialOffset;

static inline MkObj* resolve_player_object(
    MkObj* object, unsigned int instance) {
    if (object == 0 || object->hdr.instance != instance) {
        return 0;
    }
    return object;
}

void tag_team_activate_player(MkObj* object, int active) {
    PlyrClumpView* clump;
    PlyrClumpLink* link;

    clump = (PlyrClumpView*)object->clump;
    if (clump == 0) {
        return;
    }

    link = clump->geometry_list.next;
    while (link != &clump->geometry_list) {
        PlyrClumpNode* node =
            (PlyrClumpNode*)((char*)link - 0x28);
        PlyrGeometryView* geometry =
            (PlyrGeometryView*)node->geometry;
        unsigned int i;

        for (i = 0; i < geometry->material_count; i++) {
            RpMaterial* material = geometry->materials[i];
            unsigned int local =
                *(unsigned int*)((char*)material +
                                 MkmaterialLocalOffset);
            PlyrSpecularMaterialView* specular =
                (PlyrSpecularMaterialView*)
                    ((char*)material + SpecularMaterialOffset);
            int primary = ((local >> 10) & 1) ^ 1;
            int alternate = ((local & 0xBFF) % 10) > 5;
            int hidden;

            if (active != 0) {
                hidden = !primary || alternate;
            } else {
                hidden = primary || alternate;
            }
            specular->flags_2C =
                (specular->flags_2C & 0x7F) |
                (hidden << 7);
        }
        link = link->next;
    }
}

void init_mkpdata_plyrs(void) {
    memset(_mkpdata_plyrs, 0, sizeof(_mkpdata_plyrs));
    free_mkpdata_plyrs = &_mkpdata_plyrs[0];
    _mkpdata_plyrs[0].vtbl = &_mkpdata_plyrs[1];
    _mkpdata_plyrs[1].vtbl = 0;
}

void xfer_player_proc(MkProc* proc, MkProcEntryFn entry) {
    CmdScript* script;

    pdata_of_proc(proc);
    script = get_cmdscript_for_proc(proc);
    script->state = 0;
    xfer_proc(proc, entry);
}

static float p_plyr_script_in_proc(void);

void plyr_start_script_in_proc(int proc_id, unsigned int function) {
    MkProc* proc;
    PlyrScriptProcPdata* pdata;

    pdata = 0;
    proc = _create_mkproc_generic_tinystack(
        proc_id, 0x1F, p_plyr_script_in_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    if (proc != 0 && pdata != 0) {
        pdata->script = plyr_pdata->cmo;
        pdata->func_index = function;
        set_process_as_scriptable(proc);
    }
}

static float p_plyr_aux2(void);

static int fetch_shujinko_special_number_for(unsigned int move_id) {
    switch (move_id) {
    case 0x421B: return 0;
    case 0x4219: return 1;
    case 0x421E: return 2;
    case 0x4241: return 3;
    case 0x1203: return 4;
    case 0x1205: return 5;
    case 0x4242: return 6;
    case 0x4239: return 7;
    case 0x4243: return 8;
    case 0x4244: return 9;
    case 0x4245: return 10;
    default: return -2;
    }
}

void is_special_move_available(PlyrPdata* pdata, int move_id) {
    void* profile;

    if (pdata->character_id != 0x19 && pdata->character_id != 0x1A) {
        return;
    }
    profile = pdata->plyr_num == 1 ? p2_profile : p1_profile;
    if (fetch_shujinko_special_number_for(move_id) >= 0) {
        is_mark_as_unlocked(profile, 4);
    }
}

void plyr_start_script_in_plyr_pdata_proc(
    FighterMirror* fighter, int proc_id, unsigned int function) {
    PlyrScriptProcPdata* pdata = 0;
    MkProc* proc;

    proc = _create_mkproc_generic_tinystack(
        proc_id, 0x1F, p_plyr_script_in_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    if (proc != 0 && pdata != 0) {
        pdata->script = fighter->cmo;
        pdata->func_index = function;
        set_process_as_scriptable(proc);
    }
}

void show_baraka_one_blade_only(PlyrPdata* pdata, int first_blade) {
    MkProc* proc = pdata->baraka_blades_monitor;
    BarakaBladesPdata* blades;

    if (proc == 0 || proc->instance !=
        (int)pdata->baraka_blades_monitor_instance) {
        return;
    }
    blades = (BarakaBladesPdata*)pdata_of_proc(proc);
    if (blades == 0) {
        return;
    }
    if (first_blade) {
        blades->blade_a_target = 0.0f;
        blades->blade_a_step = -0.1f;
        blades->blade_b_target = 1.0f;
        blades->blade_b_step = 0.1f;
    } else {
        blades->blade_a_target = 1.0f;
        blades->blade_a_step = 0.1f;
        blades->blade_b_target = 0.0f;
        blades->blade_b_step = -0.1f;
    }
    if (!is_load_meter_active()) {
        snd_req(0x163);
    }
}

static float p_plyr_aux(void) {
    MkObj* object;

    if (aproc->pid == 0x100A) {
        object = g_game_info.plyr1.slot.mirror_a;
    } else {
        object = g_game_info.plyr0.slot.mirror_a;
    }
    if (object == 0) {
        return 0.0f;
    }
    ((PlyrProcVtable*)aproc->vtbl)->jump_sleep(p_plyr_aux2, 0.0f);
    return 0.0f;
}

void plyr_turn_on_shadowbox(PlyrInfo* player) {
    FighterMirror* fighter;

    if ((g_game_info.section->flags70 & 1) == 0) {
        return;
    }
    fighter = player->slot.fighter;
    if (fighter != 0 && fighter->flag_obj != 0) {
        unhide_obj(fighter->flag_obj);
    }
}

void plyr_turn_on_mirrorguy(PlyrInfo* player) {
    if ((g_game_info.section->flags70 & 8) != 0 &&
        player->slot.mirror_b != 0) {
        unhide_obj(player->slot.mirror_b);
    }
}

float plyr_get_pos(unsigned int axis) {
    if (axis == 1) {
        return plyr_obj->pos.y;
    }
    if (axis == 2) {
        return plyr_obj->pos.z;
    }
    return plyr_obj->pos.x;
}

void plyr_invulnerable_to_projectiles(PlyrPdata* pdata, int enabled) {
    pdata->state_flags.bits.projectile_invulnerable = enabled;
}

static float p_plyr_script_in_proc(void) {
    PlyrScriptProcPdata* pdata;

    pdata = (PlyrScriptProcPdata*)pdata_of_proc(aproc);
    if (pdata->func_index == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(pdata->script, pdata->func_index);
    cmdscript_execute(pdata->script);
    return -1.0f;
}

static float p_animate_weapon_rest_lp(void) {
    int rest_ticks;

    pose_anim(anim_pdata, 1);
    rest_ticks = anim_pdata->rest_ticks - 1;
    anim_pdata->rest_ticks = rest_ticks;
    return rest_ticks < 0 ? -1.0f : 1.0f;
}

void plyr_spawn_anim(MkProcEntryFn hand_script, MkProcEntryFn entry) {
    AnimPdata* animation;

    if (create_mkproc_anim2(0x5002, entry, &animation) != 0) {
        animation->obj = plyr_obj;
        animation->obj_instance = plyr_obj->hdr.instance;
        animation->owner = plyr_pdata;
        animation->owner_instance = plyr_pdata->instance;
        animation->hand_script = hand_script;
    }
}

void stop_vomit_slip_sound(void) {
    if (plyr_pdata->scream_sound_handle != 0) {
        snd_stop(plyr_pdata->scream_sound_handle);
    }
    plyr_pdata->scream_sound_handle = 0;
}

void setup_vomit_slip_sound(void) {
    if (plyr_pdata->scream_sound_handle == 0) {
        plyr_pdata->scream_sound_handle = plyr_snd_req(0x43);
    }
}

int plyr_pdata_is_alt_costume(PlyrInfo* player) {
    return player->flags_14_bits.alternate_costume;
}

void register_baraka_cb_functions(void) {
    plyr_pdata->baraka_moveset_callback = baraka_advance_active_moveset;
}

/* Soft ceiling: 99.94% - typed retail Baraka blade monitor setup. */
void start_baraka_blades_monitor(void) {
    BarakaBladesPdata* controller = 0;
    MkProc* proc = _create_mkproc_generic_nostack(
        0xC003, 0x1F, p_baraka_blades_controller,
        sizeof(BarakaBladesPdata), (MkHdr**)&controller);

    if (proc != 0) {
        plyr_pdata->baraka_blades_monitor = proc;
        plyr_pdata->baraka_blades_monitor_instance = proc->instance;
        controller->blade_a = plyr_obj->bones[58];
        controller->blade_b = plyr_obj->bones[59];
        controller->blade_a->flags_55_bits.scale_controlled = 1;
        controller->blade_a->scale.x = 0.0f;
        controller->blade_a->scale.y = 1.0f;
        controller->blade_a->scale.z = 1.0f;
        controller->blade_b->flags_55_bits.scale_controlled = 1;
        controller->blade_b->scale.x = 0.0f;
        controller->blade_b->scale.y = 1.0f;
        controller->blade_b->scale.z = 1.0f;
        controller->blade_a_target = 0.0f;
        controller->blade_a_step = 0.0f;
        controller->blade_a_target = 0.0f;
        controller->blade_b_step = 0.0f;
        mk_insert((MkHdr*)proc, &plyr_pdata->fighter_definition->cmo->pdata_list);
        mk_insert((MkHdr*)proc, &plyr_obj->child_list);
    }
}

void start_baraka_jaw_monitor(void) {
    union {
        MkHdr* hdr;
        BarakaJawPdata* jaw;
    } pdata;
    union {
        MkProc* proc;
        MkHdr* hdr;
    } monitor;

    pdata.hdr = 0;
    destroy_mkprocs_pid(0xC002);
    monitor.proc = _create_mkproc_generic_bigstack(
        0xC002, 0x12, p_baraka_jaw_controller, sizeof(BarakaJawPdata),
        &pdata.hdr);
    if (monitor.proc != 0) {
        pdata.jaw->animation = plyr_anim_pdata;
        pdata.jaw->animation_instance = plyr_anim_pdata->hdr.instance;
        pdata.jaw->state = 0;
        pdata.jaw->command = 0;
        plyr_pdata->jaw_monitor = monitor.proc;
        plyr_pdata->jaw_monitor_instance = monitor.proc->instance;
        pdata.jaw->jaw_bone = get_bone_with_tag(plyr_obj, 1);
        mk_insert(monitor.hdr, &plyr_obj->child_list);
    }
}

/* Soft ceiling: 96.86% - facial damage and texture stages recovered. */
void add_facial_damage(float amount) {
    PlyrPdata* player = plyr_pdata;
    AniTextureControl* texture;
    float damage;

    if (player == 0) {
        return;
    }
    if (has_sidekick(player) != 0) {
        return;
    }
    if (get_blood_level() == 0) {
        return;
    }

    player->facial_damage += amount;
    damage = player->facial_damage;
    if (damage > 1.0f) {
        damage = 1.0f;
    }
    player->facial_damage = damage;
    if (player->facial_damage_complete == 0 &&
        player->facial_damage == 1.0f) {
        player->facial_damage_complete = 1;
    }

    texture = ck_ani_texture_control_item(&player->facial_texture);
    if (texture == 0) {
        return;
    }
    if (get_ani_texture_numframes(texture) - 1 > 3 &&
        player->facial_damage >= 1.0f) {
        set_ani_texture_frame(texture, 4);
    } else if (player->facial_damage > 0.85f) {
        set_ani_texture_frame(texture, 3);
    } else if (player->facial_damage > 0.5f) {
        set_ani_texture_frame(texture, 2);
    } else if (player->facial_damage > 0.25f) {
        set_ani_texture_frame(texture, 1);
    } else {
        set_ani_texture_frame(texture, 0);
    }
}

int get_player_number(MkObj* object) {
    if (object->oid == 0x1001) {
        return 0;
    }
    if (object->oid == 0x1002) {
        return 1;
    }
    return -1;
}

void plyr_turn_off_shadowbox(PlyrInfo* player) {
    if (player->slot.mirror_b != 0) {
        hide_obj(player->slot.pdata->shadowbox);
    }
}

void plyr_turn_off_mirrorguy(PlyrInfo* player) {
    if (player->slot.mirror_b != 0) {
        hide_obj(player->slot.mirror_b);
    }
}

void ps_plyr(void) {
    plyr_pdata->saved_anim_high_frame = plyr_anim_pdata->high_frame;
    plyr_pdata->saved_anim_low_frame = plyr_anim_pdata->low_frame;
    plyr_pdata->saved_anim_flags = plyr_anim_pdata->flags;
    plyr_pdata->saved_anim_script_word = plyr_anim_pdata->script_word;
    g_perform_validation = 1;
    plyr_pdata = 0;
    plyr_obj = 0;
    plyr_anim_proc = 0;
    plyr_anim_pdata = 0;
    his_obj = 0;
    his_pdata = 0;
}

void vdestroy_mkpdata_plyr(void) {
    destroy_mkpdata_plyr();
}

void exit_plyr_proc(void) {
    plyr_pdata->saved_anim_high_frame = plyr_anim_pdata->high_frame;
    plyr_pdata->saved_anim_low_frame = plyr_anim_pdata->low_frame;
    plyr_pdata->saved_anim_flags = plyr_anim_pdata->flags;
    plyr_pdata->saved_anim_script_word = plyr_anim_pdata->script_word;
    g_perform_validation = 1;
    plyr_pdata = 0;
    plyr_obj = 0;
    plyr_anim_proc = 0;
    plyr_anim_pdata = 0;
    his_obj = 0;
    his_pdata = 0;
    apdata = apdata_save;
    apdata_save = 0;
}

/* Soft ceiling: 77.64% - visibility and owned-child filtering recovered. */
void show_player(PlyrPdata* player) {
    MkObj* object;
    MkObj* fighter;
    MkPtr* link;

    if (player == 0) {
        return;
    }
    object = resolve_player_object(
        player->tracked_obj, player->tracked_obj_instance);
    if (object != 0) {
        unhide_obj(object);
    }
    if (is_weapon_style(player->fighter_definition) != 0) {
        plyr_weapon_show(player, 1, player->mirror_slots);
    }

    fighter = resolve_player_object(
        player->tracked_obj, player->tracked_obj_instance);
    if (fighter != 0) {
        object = resolve_player_object(
            player->aux_weapon_latch.obj,
            player->aux_weapon_latch.instance);
        if (object != 0) {
            unhide_obj(object);
        }
        object = resolve_player_object(
            player->mirror_obj.obj, player->mirror_obj.instance);
        if (object != 0) {
            unhide_obj(object);
        }

        link = fighter->list_44;
        while (link != 0) {
            if (link->hdr->instance != link->instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                object = link->hdr->vtbl == &vtbl_mkobj
                    ? (MkObj*)link->hdr : 0;
                if (object != 0 && object->oid != 0x1008) {
                    unhide_obj(object);
                }
                link = link->next;
            }
        }
    }

    if ((g_game_info.section->flags70 & 8) != 0 &&
        player->plyr_info->slot.mirror_b != 0) {
        unhide_obj(player->plyr_info->slot.mirror_b);
    }
    if ((g_game_info.section->flags70 & 1) != 0 &&
        player->plyr_info->slot.fighter != 0 &&
        player->plyr_info->slot.fighter->flag_obj != 0) {
        unhide_obj(player->plyr_info->slot.fighter->flag_obj);
    }
}

/* Soft ceiling: 80.60% - hide propagation recovered with typed latches. */
void hide_player(PlyrPdata* player, int hide_weapons) {
    MkObj* object;
    MkObj* fighter;
    MkPtr* link;
    int i;

    object = resolve_player_object(
        player->tracked_obj, player->tracked_obj_instance);
    if (object != 0) {
        hide_obj(object);
    }
    if (hide_weapons != 0) {
        for (i = 0; i < 3; i++) {
            plyr_weapon_hide(
                player, 1, &player->weapon_styles[i]->mirror_slots);
        }
    }

    fighter = resolve_player_object(
        player->tracked_obj, player->tracked_obj_instance);
    if (fighter != 0) {
        object = resolve_player_object(
            player->aux_weapon_latch.obj,
            player->aux_weapon_latch.instance);
        if (object != 0) {
            hide_obj(object);
        }
        object = resolve_player_object(
            player->mirror_obj.obj, player->mirror_obj.instance);
        if (object != 0) {
            hide_obj(object);
        }

        link = fighter->list_44;
        while (link != 0) {
            if (link->hdr->instance != link->instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                object = link->hdr->vtbl == &vtbl_mkobj
                    ? (MkObj*)link->hdr : 0;
                if (object != 0) {
                    hide_obj(object);
                }
                link = link->next;
            }
        }
    }

    if (player->plyr_info->slot.mirror_b != 0) {
        hide_obj(player->plyr_info->slot.mirror_b);
    }
    if (player->plyr_info->slot.mirror_b != 0) {
        hide_obj(player->plyr_info->slot.fighter->flag_obj);
    }
}

/* Soft ceiling: 94.80% - teleport and constraint latch update recovered. */
void move_player(
    MkObj* object,
    const Vec* position,
    const Vec* angles) {
    Vec translation;

    v3_sub_v3(&translation, position, &object->pos);
    object->pos.x = position->x;
    object->pos.y = position->y;
    object->pos.z = position->z;
    object->ang.x = angles->x;
    object->ang.y = angles->y;
    object->ang.z = angles->z;
    update_obj_pos(object);
    obj_translate_cloth(object, &translation);
    if (object == g_game_info.plyr0.slot.mirror_a) {
        set_constrain_last_pos(g_game_info.plyr0.slot.pdata->plyr_num, position);
    } else {
        set_constrain_last_pos(g_game_info.plyr1.slot.pdata->plyr_num, position);
    }
}

/* Soft ceiling: 92.53% - clean typed movement; float scheduling differs. */
void move_player_no_constrain_update(
    MkObj* object,
    const Vec* position,
    const Vec* angles) {
    Vec translation;

    v3_sub_v3(&translation, position, &object->pos);
    object->pos.x = position->x;
    object->pos.y = position->y;
    object->pos.z = position->z;
    object->ang.x = angles->x;
    object->ang.y = angles->y;
    object->ang.z = angles->z;
    update_obj_pos(object);
    obj_translate_cloth(object, &translation);
}

int plyr_pdata_get_previous_state(PlyrPdata* pdata) {
    return pdata->previous_state;
}

int plyr_pdata_get_state(PlyrPdata* pdata) {
    return pdata->state;
}

void* plyr_pdata_get_pchr(PlyrPdata* pdata) {
    return pdata->pchr;
}

ScriptSlot* plyr_pdata_get_cmo(PlyrPdata* pdata) {
    return pdata->cmo;
}

int plyr_pdata_get_plyr_num(PlyrPdata* pdata) {
    return pdata->plyr_num;
}

MkObj* plyr_pdata_get_his_obj(PlyrPdata* pdata) {
    return pdata->his_obj;
}

void* plyr_pdata_get_plyr_obj(PlyrPdata* pdata) {
    return pdata->plyr_info->slot.mirror_a;
}

PlyrPdata* plyr_pdata_get_his_plyr_pdata(PlyrPdata* pdata) {
    return pdata->his_plyr_pdata;
}

PlyrInfo* plyr_pdata_get_plyr_info(PlyrPdata* pdata) {
    return pdata->plyr_info;
}

float plyr_anim_get_frame(PlyrAnimPdata* pdata) {
    return pdata->frame;
}

PlyrAnimPdata* get_my_plyr_anim_pdata(void) {
    return plyr_anim_pdata;
}

PlyrPdata* get_my_plyr_pdata(void) {
    return plyr_pdata;
}

PlyrPdata* get_his_plyr_pdata(void) {
    return plyr_pdata->his_plyr_pdata;
}

PlyrPdata* get_plyr_pdata_plyr_num(int player) {
    if (player == 0) {
        return (PlyrPdata*)g_game_info.plyr0.slot.fighter;
    }
    if (player == 1) {
        return (PlyrPdata*)g_game_info.plyr1.slot.fighter;
    }
    return 0;
}

MkObj* get_plyr_obj_plyr_num(int player) {
    if (player == 0) {
        return (MkObj*)g_game_info.plyr0.slot.mirror_a;
    }
    if (player == 1) {
        return (MkObj*)g_game_info.plyr1.slot.mirror_a;
    }
    return 0;
}

int get_my_plyr_num(void) {
    return plyr_pdata->plyr_num;
}

MkObj* get_my_plyr_obj(void) {
    return plyr_obj;
}

MkObj* get_his_plyr_obj(void) {
    return plyr_pdata->his_obj;
}

PlyrInfo* get_plyr_info(void) {
    return plyr_pdata->plyr_info;
}

int get_my_particle_player_bank_num(void) {
    return plyr_pdata->plyr_num == 1 ? 2 : 1;
}

int plyr_pdata_sidekick_active(PlyrPdata* pdata) {
    return pdata->sidekick_active;
}

MkObj* plyr_pdata_get_sidekick_obj(PlyrPdata* pdata) {
    MkObj* obj;

    obj = pdata->sidekick_obj;
    if (obj != 0) {
        if (((MkHdr*)obj)->instance != pdata->sidekick_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    return obj;
}

MkObj* get_my_sidekick_obj(void) {
    MkObj* obj;

    obj = plyr_pdata->sidekick_obj;
    if (obj != 0) {
        if (((MkHdr*)obj)->instance == plyr_pdata->sidekick_instance) {
            return obj;
        }
        return 0;
    }
    return 0;
}

int is_sidekick_active(PlyrInfo* player) {
    PlyrPdata* pdata;

    pdata = (PlyrPdata*)player->slot.fighter;
    if (pdata->sidekick_available != 0) {
        return pdata->sidekick_active;
    }
    return 0;
}

void cleanup_player_globals(void) {
}

void set_player_state(PlyrInfo* player, int state) {
    player->player_state = state;
}

float player_sleep_forever(void) {
    return 1.0f;
}

void set_attack_type(int attack_type) {
    plyr_pdata->attack_type = attack_type;
}

void init_plyr_info_struct(PlyrInfo* player) {
    player->player_index = 0x2C;
    player->field_0C = 1.0f;
    player->slot.fighter = 0;
    player->field_14 = 0;
    player->slot.mirror_a = 0;
    player->slot.mirror_b = 0;
    player->idle_proc = 0;
    player->field_68 = 0;
    player->collision_data = 0;
    player->field_40 = 0;
    player->field_44 = 0;
    player->field_48 = 0;
    player->pad_index = -1;
    player->field_04 = 3;
}

int load_plyr_model_async(int player, int char_id, int* flags) {
    MkHdr* pdata_out;
    LoadPlyrModelPdata* pdata;
    int pid;

    pid = player + LOAD_PLYR_MODEL_PID;
    destroy_mkprocs_pid(pid);
    if (_create_mkproc_generic_bigstack(
            pid, 0x1F, p_load_plyr_model_async, sizeof(LoadPlyrModelPdata), &pdata_out) == 0) {
        return 0;
    }

    pdata = (LoadPlyrModelPdata*)pdata_out;
    pdata->player = player;
    pdata->char_id = char_id;
    pdata->flags = *flags;
    return 1;
}

static float p_plyr_aux2(void) {
    if (plyr_pdata->postround_value != 0.0f &&
        (g_game_info.flags & 1) == 0 &&
        !g_game_info.pause_flag_bits.fatality_window) {
        if (plyr_pdata->plyr_num == 0) {
            damage_p1(-plyr_pdata->postround_value);
        } else {
            damage_p2(-plyr_pdata->postround_value);
        }
    }
    if (plyr_pdata->drone_request == 1) {
        drone_ai_watcher();
    } else if (plyr_pdata->aux_update_callback != 0) {
        plyr_pdata->aux_update_callback();
    }
    return 0.0f;
}

void start_plyrs(void) {
    create_player(0, &g_game_info.plyr0);
    setup_sound_banks(0xC);
    create_player(1, &g_game_info.plyr1);
    setup_sound_banks(0xD);
    xfer_proc((MkProc*)g_game_info.plyr0.idle_proc, p_plyr_start);
    xfer_proc((MkProc*)g_game_info.plyr1.idle_proc, p_plyr_start);
}

void load_aux_weapon(void) {
    MkObj* weapon = load_weapon(plyr_obj);

    if (weapon != 0) {
        plyr_aux_weapon_grab(plyr_pdata, weapon);
        weapon->hide_flags &= ~0x20;
    }
}

float p_animate_weapon_rest(void) {
    set_anim_script(anim_pdata, anim_pdata->hand_script, 0x40);
    anim_pdata->rest_ticks = 10;
    ((PlyrProcVtable*)aproc->vtbl)
        ->jump_sleep(p_animate_weapon_rest_lp, 1.0f);
    return 0.0f;
}

static void move_blade_toward(
    MkBone* blade, float target, float step, int* command) {
    float scale = blade->scale.x + step;

    if ((step > 0.0f && scale > target) ||
        (step < 0.0f && scale < target)) {
        scale = target;
        if (target != 0.0f) {
            *command = 1;
        }
    }
    blade->scale.x = scale;
}

float p_baraka_blades_controller(void) {
    BarakaBladesPdata* blades = (BarakaBladesPdata*)apdata;

    if (blades->blade_a->scale.x != blades->blade_a_target) {
        move_blade_toward(
            blades->blade_a, blades->blade_a_target,
            blades->blade_a_step, &blades->command);
    }
    if (blades->blade_b->scale.x != blades->blade_b_target) {
        move_blade_toward(
            blades->blade_b, blades->blade_b_target,
            blades->blade_b_step, &blades->command);
    }
    return 1.0f;
}

PlyrPdata* get_mkpdata_plyr(void) {
    PlyrPdata* pdata = free_mkpdata_plyrs;

    if (pdata != 0) {
        free_mkpdata_plyrs = (PlyrPdata*)pdata->vtbl;
        pdata->vtbl = &vtbl_mkpdata_plyr;
        mk_set_instance(&pdata->instance);
        memset(
            &pdata->state_flags, 0,
            sizeof(*pdata) - 0x1C);
        pdata->online_sync_index = -1;
    }
    return pdata;
}

void pw_plyr(void) {
    plyr_pdata = (PlyrPdata*)apdata;
    plyr_obj = resolve_player_object(
        plyr_pdata->tracked_obj, plyr_pdata->tracked_obj_instance);
    plyr_anim_proc = plyr_pdata->anim_proc;
    if (plyr_anim_proc != 0 &&
        plyr_anim_proc->instance == (int)plyr_pdata->anim_proc_instance) {
        plyr_anim_pdata = (AnimPdata*)pdata_of_proc(plyr_anim_proc);
        if (g_perform_validation != 0) {
            get_game_state();
        }
        g_perform_validation = 0;
    } else {
        plyr_anim_proc = 0;
    }
    plyr_lefthand_anim_proc = plyr_pdata->left_hand_anim_proc;
    if (plyr_lefthand_anim_proc != 0 &&
        plyr_lefthand_anim_proc->instance !=
            (int)plyr_pdata->left_hand_anim_instance) {
        plyr_lefthand_anim_proc = 0;
    }
    plyr_righthand_anim_proc = plyr_pdata->right_hand_anim_proc;
    if (plyr_righthand_anim_proc != 0 &&
        plyr_righthand_anim_proc->instance !=
            (int)plyr_pdata->right_hand_anim_instance) {
        plyr_righthand_anim_proc = 0;
    }
    if (g_game_info.plyr0.slot.pdata != 0 &&
        g_game_info.plyr1.slot.pdata != 0) {
        his_obj = plyr_pdata->his_obj;
        his_pdata = plyr_pdata->his_plyr_pdata;
    }
}

void become_plyr1_proc(void) {
    apdata_save = apdata;
    apdata = (MkHdr*)g_game_info.plyr0.slot.pdata;
    pw_plyr();
}

void become_plyr2_proc(void) {
    apdata_save = apdata;
    apdata = (MkHdr*)g_game_info.plyr1.slot.pdata;
    pw_plyr();
}

void swap_active_plyr_proc(void) {
    PlyrPdata* current = (PlyrPdata*)apdata;

    apdata = (MkHdr*)current->his_plyr_pdata;
    pw_plyr();
}

static void randomize_player(PlyrInfo* player) {
    int attempts = 0;
    int alternate = 0;
    unsigned short character = 0;

    do {
        character = randu0(0x2C);
        alternate = randu0(4) == 0;
        attempts++;
    } while (is_char_locked(character, alternate) && attempts < 50);
    if (attempts >= 50) {
        character = 0;
        alternate = 0;
    }
    player->player_index = character;
    player->flags_14_bits.alternate_costume = alternate;
}

void rnd_plyrs(void) {
    randomize_player(&g_game_info.plyr0);
    randomize_player(&g_game_info.plyr1);
    resolve_alternate_palettes(
        &g_game_info.plyr1, &g_game_info.plyr0);
}

static void destroy_header(MkHdr* object) {
    if (object != 0 && object->instance != 0) {
        ((void (*)(MkHdr*))object->vtbl->destroy)(object);
    }
}

void release_other_player(void) {
    MkObj* held = plyr_pdata->held_opponent_latch.obj;
    MkProc* hold_proc = plyr_pdata->hold_proc;
    PlyrPdata* opponent = plyr_pdata->his_plyr_pdata;

    plyr_obj->flags_09 |= 0x18;
    if (held != 0 &&
        held->hdr.instance ==
            plyr_pdata->held_opponent_latch.instance) {
        held->flags_09 |= 0xBA;
    }
    plyr_pdata->held_opponent_latch.obj = 0;
    plyr_pdata->held_opponent_latch.instance = 0;
    if (opponent != 0) {
        opponent->held_by_player = 0;
        opponent->hold_state = 0;
    }
    if (hold_proc != 0 &&
        hold_proc->instance == (int)plyr_pdata->hold_proc_instance) {
        plyr_pdata->hold_proc = 0;
        plyr_pdata->hold_proc_instance = 0;
        destroy_header((MkHdr*)hold_proc);
        if (opponent != 0 && opponent->anim_proc != 0 &&
            opponent->anim_proc->instance ==
                (int)opponent->anim_proc_instance) {
            AnimPdata* animation =
                (AnimPdata*)pdata_of_proc(opponent->anim_proc);
            if (animation != 0) {
                animation->transition_weight = 1.0f;
                animation->transition_step = 0.0f;
            }
        }
    }
}

void check_release_other_player(void) {
    MkProc* hold_proc = plyr_pdata->hold_proc;

    if (hold_proc != 0 &&
        hold_proc->instance == (int)plyr_pdata->hold_proc_instance) {
        release_other_player();
    }
}

typedef struct GrabBoneMatcher {
    MkHdr hdr;
    unsigned int flags;
    float weight;
    char pad10[0x0C];
    Vec source_offset;
    char pad28[0x14];
    Vec target_offset;
    char pad48[4];
} GrabBoneMatcher;

static void set_object_flip(
    MkObj* object, AnimPdata* animation, int flip_state) {
    if (object == 0 || animation == 0) {
        return;
    }
    if (flip_state == 1 && (object->hide_flags & 0x40) != 0) {
        object->hide_flags &= ~0x40;
        animation->flags ^= 8;
    } else if (flip_state == 2 &&
               (object->hide_flags & 0x40) == 0) {
        object->hide_flags |= 0x40;
        animation->flags ^= 8;
    }
}

void plyr_grab_other_flip_states(
    int player_flip, int opponent_flip) {
    MkObj* opponent = plyr_pdata->his_obj;
    AnimPdata* opponent_animation = 0;
    GrabBoneMatcher* matcher;

    set_object_flip(plyr_obj, plyr_anim_pdata, player_flip);
    if (plyr_pdata->held_opponent_latch.obj != 0 ||
        plyr_pdata->hold_proc != 0 || opponent == 0) {
        return;
    }
    if (opponent_flip != 0 &&
        plyr_pdata->his_plyr_pdata != 0 &&
        plyr_pdata->his_plyr_pdata->anim_proc != 0) {
        opponent_animation = (AnimPdata*)pdata_of_proc(
            plyr_pdata->his_plyr_pdata->anim_proc);
    }
    set_object_flip(opponent, opponent_animation, opponent_flip);
    opponent->flags_09 &= ~0xBA;
    plyr_obj->flags_09 &= ~0x18;
    plyr_pdata->held_opponent_latch.obj = opponent;
    plyr_pdata->held_opponent_latch.instance =
        opponent->hdr.instance;
    matcher = (GrabBoneMatcher*)start_bone_matcher(
        plyr_obj, plyr_obj->fallback_bone_index,
        opponent, opponent->fallback_bone_index, 5.0f);
    if (matcher != 0) {
        matcher->source_offset.x = 0.0f;
        matcher->source_offset.y = 0.0f;
        matcher->source_offset.z = 0.0f;
        matcher->target_offset.x = 0.0f;
        matcher->target_offset.y = 0.0f;
        matcher->target_offset.z = 0.0f;
        matcher->flags |= 0x16000000;
        matcher->weight = 0.75f;
        plyr_pdata->hold_proc = (MkProc*)matcher;
        plyr_pdata->hold_proc_instance = matcher->hdr.instance;
    }
}

typedef struct SidekickStyleRequest {
    MkHdr hdr;
    char pad08[0x18];
    MkProc* process;
    unsigned int process_instance;
} SidekickStyleRequest;

void active_sidekick_swap_change_style(
    SidekickStyleRequest* request) {
    MkProc* process = request->process;
    CmdScript* script;

    if (process == 0 ||
        process->instance != (int)request->process_instance) {
        return;
    }
    active_sidekick_swap(plyr_pdata, 2);
    script = get_cmdscript_for_proc(process);
    script->unk28 = 0x7C;
    ((PlyrProcVtable*)aproc->vtbl)
        ->jump_sleep(r_call_script_function, 0.0f);
}

void active_sidekick_swap_from_sky(PlyrPdata* pdata) {
    float dx;
    float dz;
    float length;

    destroy_mkprocs_pid(pdata->plyr_num == 0 ? 0xC028 : 0xC029);
    dx = plyr_obj->pos.x - his_obj->pos.x;
    dz = plyr_obj->pos.z - his_obj->pos.z;
    length = sqrtf(dx * dx + dz * dz);
    active_sidekick_swap(pdata, 1);
    if (length != 0.0f) {
        plyr_obj->pos.x = his_obj->pos.x + dx * 3.0f / length;
        plyr_obj->pos.z = his_obj->pos.z + dz * 3.0f / length;
    }
    plyr_obj->pos.y = g_game_info.field_34 + 4.0f;
}

void delete_player(int player_index) {
    PlyrInfo* player;
    PlyrPdata* pdata;
    int script;

    if (player_index < 0 || player_index > 1) {
        return;
    }
    player = player_index == 0
        ? &g_game_info.plyr0 : &g_game_info.plyr1;
    pdata = player->slot.pdata;
    fxbanks_unload_by_owner(player_index + 1);
    if (pdata != 0) {
        MkObj* object = resolve_player_object(
            pdata->tracked_obj, pdata->tracked_obj_instance);
        if (object != 0) {
            if (object == player->slot.mirror_a) {
                destroy_header((MkHdr*)object);
                player->slot.mirror_a = 0;
            }
            pdata->tracked_obj = 0;
            pdata->tracked_obj_instance = 0;
        }
        if (player->slot.mirror_b != 0) {
            destroy_header((MkHdr*)player->slot.mirror_b);
            player->slot.mirror_b = 0;
        }
        TearDownShadow(pdata);
        destroy_mkprocs_pid(player_index == 0 ? 0x6002 : 0x6003);
        destroy_mkprocs_pid(pdata->plyr_num == 0 ? 0xC028 : 0xC029);
        if (pdata->sidekick_obj != 0 &&
            pdata->sidekick_obj->hdr.instance ==
                pdata->sidekick_instance) {
            destroy_header((MkHdr*)pdata->sidekick_obj);
            pdata->sidekick_obj = 0;
            pdata->sidekick_instance = 0;
        }
        destroy_header((MkHdr*)pdata);
        player->slot.pdata = 0;
    }
    if (player->idle_proc != 0) {
        destroy_mkprocs_pid(((MkProc*)player->idle_proc)->pid);
    }
    destroy_mkprocs_pid(0x1003);
    kill_fstyle_signs_for_plyr(player);
    term_player_collision(player);
    for (script = player_index == 0 ? 3 : 7;
         script < (player_index == 0 ? 7 : 11); script++) {
        unload_script(script);
    }
}

typedef struct SidekickActionView {
    char pad00[0x318];
    AniData* charge_exit_animation;
    char pad31C[0x28];
    AniData* common_exit_animation;
    char pad348[0x130];
    ScriptSlot* cmo;
} SidekickActionView;

static void plyr_sleep(float ticks) {
    _mkproc_sleep_ticks = ticks;
    ((PlyrProcVtable*)aproc->vtbl)->sleep(aproc);
}

void sidekick_cool_vanish(PlyrPdata* pdata) {
    MkObj* sidekick = pdata->sidekick_obj;
    MkProc* anim_proc = pdata->sidekick_anim_proc;
    AnimPdata* animation;
    SidekickActionView* actions = (SidekickActionView*)pdata;
    int function;

    if (sidekick == 0 || sidekick->hdr.instance != pdata->sidekick_instance ||
        anim_proc == 0 ||
        anim_proc->instance != (int)pdata->sidekick_anim_instance) {
        return;
    }
    animation = (AnimPdata*)pdata_of_proc(anim_proc);
    if (animation == 0) {
        return;
    }
    if (pdata->sidekick_active == 0) {
        transition_to_anim_script(
            animation, actions->charge_exit_animation, 3, 0.25f);
        animation->step = 1.2f;
        sidekick->flags_09 |= 0x40;
        while (animation->frame < 5.0f) {
            plyr_sleep(1.0f);
        }
        sidekick->flags_09 &= ~0xC0;
        while (animation->frame < 26.0f) {
            plyr_sleep(1.0f);
        }
        snd_req(0x32C);
        obj_set_gravity(sidekick, -0.01f);
    } else {
        transition_to_anim_script(
            animation, actions->common_exit_animation, 3, 0.1f);
        animation->step = 2.0f;
        sidekick->flags_09 |= 0x40;
        while (animation->frame < 5.0f) {
            plyr_sleep(1.0f);
        }
        sidekick->flags_09 &= ~0xC0;
        snd_req_delay(0x33B, 0x10);
    }
    if ((sidekick->hide_flags & 0x20) == 0) {
        function = get_script_function_by_name(
            actions->cmo, "start_smoke_exit_pfx");
        plyr_start_script_in_plyr_pdata_proc(
            pdata->plyr_info->slot.fighter, 0xC025, function);
    }
    plyr_sleep(30.0f);
    obj_set_gravity(sidekick, 0.0f);
    sidekick->flags_08 &= ~1;
    hide_obj(sidekick);
}

void plyr_in_spin_react(PlyrPdata* pdata) {
    MkProc* proc = pdata->anim_proc;

    if (proc != 0 && proc->instance == (int)pdata->anim_proc_instance) {
        (void)pdata_of_proc(proc);
    }
}
