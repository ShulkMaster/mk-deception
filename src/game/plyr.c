#include "game/game_info.h"
#include "game/bgnd.h"
#include "game/cloth.h"
#include "game/moveset.h"
#include "game/plyr.h"
#include "game/specular.h"
#include "game/weapon.h"
#include "math/mk_math.h"
#include "platform/main.h"
#include "runtime/plyr_info.h"

#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_obj.h"
#include "runtime/section.h"
#include "runtime/shadow.h"
#include "runtime/anim_pdata.h"
#include "runtime/anim_api.h"
#include "runtime/anims.h"
#include "runtime/cstring.h"
#include "runtime/sound.h"
#include "runtime/asset.h"
#include "runtime/image.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"

#define LOAD_PLYR_MODEL_PID 0x9032

typedef union LoadPlyrFlags {
    unsigned int word;
    struct {
        unsigned char alternate : 1;
        unsigned char alternate_costume : 1;
        unsigned char pad_bits : 6;
        unsigned char pad_bytes[3];
    } bits;
} LoadPlyrFlags;

typedef struct LoadPlyrModelPdata {
    MkHdr hdr;
    int player;
    int char_id;
    LoadPlyrFlags flags;
} LoadPlyrModelPdata;

typedef struct PlyrModelDataTable {
    char pad00[4];
    const char* primary_art;
    char pad08[0x20];
    const char* alternate_art;
    char pad2C[0x20];
    const char* primary_costume_art;
    char pad50[4];
    const char* alternate_costume_art;
    char pad58[4];
    const char* shared_art;
} PlyrModelDataTable;

unsigned char shared_ani[0x480];
static PlyrPdata _mkpdata_plyrs[PLYR_PDATA_POOL_COUNT];
int g_perform_validation = 1;
MkHdr* apdata_save;
ScriptSlot* reactions_cmo;
int p2_current_switch_time;
int p2_current_switch_bit;
int p1_current_switch_time;
int p1_current_switch_bit;
int p2_last_switch_time;
int p2_last_switch_bit;
int p1_last_switch_time;
int p1_last_switch_bit;
AnimPdata* plyr_anim_pdata;
MkProc* plyr_righthand_anim_proc;
MkProc* plyr_lefthand_anim_proc;
MkProc* plyr_anim_proc;
PlyrPdata* his_pdata;
MkObj* his_obj;
MkObj* plyr_obj;
PlyrPdata* plyr_pdata;
PlyrPdata* free_mkpdata_plyrs;

static float p_load_plyr_model_async(void);
static float p_animate_weapon_rest_lp(void);
static void load_player_anim_files(PlyrPdata* pdata);
static void create_sidekick(int player_index, PlyrInfo* player);
extern MkVtable5 vtbl_mkpdata_plyr;
extern MkVtable5 vtbl_mkobj;
extern MslSoundHandle plyr_snd_req(int sound);
extern void snd_stop(MslSoundHandle handle);
static int baraka_advance_active_moveset(
    PlyrPdata* pdata, PlyrMirrorSlots* context);
extern AnimPdata* anim_pdata;
extern void* get_bone_with_tag(MkObj* object, int tag);
static float p_baraka_jaw_controller(void);
static inline int set_plyr_anim_script_frame(
    AnimPdata* animation, AniData* script,
    unsigned int flags, float frame) {
    return set_anim_script_frame(frame, animation, script, flags);
}
extern float p_baraka_blades_controller(void);
extern int get_current_bgnd(void);
extern void clear_my_face_opponent_flag(void);
extern int is_weapon_style(PlyrFighterDefinition* style);
extern void plyr_weapon_show(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* slots);
extern void plyr_weapon_hide(
    PlyrPdata* player, int show_aux, PlyrMirrorSlots* slots);
extern MkObj* plyr_weapon_release(PlyrPdata* player);
extern MkObj* plyr_weapon2_release(PlyrPdata* player);
extern void plyr_weapon_grab(PlyrPdata* player, MkObj* weapon);
extern void plyr_weapon2_grab(PlyrPdata* player, MkObj* weapon);
extern MkObj* load_bgnd_weapon_reflection(WeaponDefinition* definition);
extern void show_fighting_style(GlobalMoveset* moveset, int player);
extern void generate_ai_table_moveset(void* moveset);
extern char p1_profile[];
extern char p2_profile[];
extern int is_mark_as_unlocked(void* profile, int mark);
extern int is_load_meter_active(void);
extern void damage_p1(float damage);
extern void damage_p2(float damage);
extern void drone_ai_watcher(void);
extern void setup_sound_banks(int bank);
extern float p_plyr_start(void);
extern void plyr_aux_weapon_grab(PlyrPdata* pdata, MkObj* weapon);
extern int is_char_locked(int character, int alternate);
extern void resolve_alternate_palettes(PlyrInfo* player);
extern MkHdr* start_bone_matcher(
    MkObj* source, int source_bone, MkObj* target, int target_bone,
    float weight);
extern void select_fighter_voice_in_bank(int player, int alternate_voice);
extern int is_local_plyr(void);
extern void advance_active_moveset(PlyrPdata* pdata);
extern void load_player_fstyle_signs(PlyrPdata* pdata);
extern MkFileEntry misc_anims_list_file_table[5];
extern MkFileInfo cmo_script_reactions;
extern void generate_ai_table_player(FighterMirror* player);
extern int build_bones_tbl(MkObj* object, const int* tags);
extern void limb_sever_hide_z_meat_chunks_all(MkObj* object);
extern void plyr_obj_load_bld_data(
    FighterMirror* pdata, void* blood_data, MkObj* object,
    const char* path_name);
extern int load_effect_bank(char* name);
extern void start_constrain_proc(void);
extern void init_debug_variables(void);
extern void pull_bone_hierarchy_mkobj(void* object);
extern void create_shadow_proc(
    int pid, PlyrPdata* controller, MkObj* source, MkObj* shadow);
extern MkProc* create_mkproc_headtracking(
    int pid, MkObj* object, PlyrPdata* target);
extern void player_initialize_chores(void);
extern int shadow_bones[37];
extern float p_plyr_pz_fighter_start(void);
extern float drone_start(void);
extern float p_joy_start(void);

#define DECLARE_PLAYER_FILES(name_)           \
    extern MkFileEntry name_##_file_table[];  \
    extern MkFileEntry name_##_alt_file_table[]

DECLARE_PLAYER_FILES(scorpion);
DECLARE_PLAYER_FILES(baraka);
DECLARE_PLAYER_FILES(noob);
DECLARE_PLAYER_FILES(subzero);
DECLARE_PLAYER_FILES(mileena);
DECLARE_PLAYER_FILES(nightwolf);
DECLARE_PLAYER_FILES(ermac);
DECLARE_PLAYER_FILES(ashrah);
DECLARE_PLAYER_FILES(sindel);
DECLARE_PLAYER_FILES(limei);
DECLARE_PLAYER_FILES(boraicho);
DECLARE_PLAYER_FILES(hotaru);
DECLARE_PLAYER_FILES(kenshi);
DECLARE_PLAYER_FILES(smoke);
DECLARE_PLAYER_FILES(skab);
DECLARE_PLAYER_FILES(tanya);
DECLARE_PLAYER_FILES(liukang);
DECLARE_PLAYER_FILES(kira);
DECLARE_PLAYER_FILES(kabal);
DECLARE_PLAYER_FILES(kobra);
DECLARE_PLAYER_FILES(jade);
DECLARE_PLAYER_FILES(dairou);
DECLARE_PLAYER_FILES(raiden);
DECLARE_PLAYER_FILES(cassius);
DECLARE_PLAYER_FILES(shujinko);
DECLARE_PLAYER_FILES(noobsmoke);
DECLARE_PLAYER_FILES(goro);
DECLARE_PLAYER_FILES(shaokahn);
DECLARE_PLAYER_FILES(mkda_jax);

extern MkFileEntry liukang_ghost_file_table[];
extern MkFileEntry shujinko_13_file_table[];
extern MkFileEntry freak_file_table[];
extern MkFileEntry dragonking_file_table[];
extern MkFileEntry mkda_rayden_file_table[];
extern MkFileEntry mkda_quanchi_file_table[];
extern MkFileEntry mkda_kunglao_file_table[];
extern MkFileEntry mkda_cage_file_table[];
extern MkFileEntry mkda_sonya_file_table[];
extern MkFileEntry mkda_nitara_file_table[];
extern MkFileEntry mkda_shang_file_table[];
extern MkFileEntry mkda_frost_file_table[];
extern MkFileEntry mkda_kitana_file_table[];
extern MkFileEntry mkda_drahmin_file_table[];

#undef DECLARE_PLAYER_FILES

GlobalPlayerEntry global_player_data[44] = {
    {"SCORPION", scorpion_file_table, scorpion_alt_file_table,
     "scorpion.mko"},
    {"BARAKA", baraka_file_table, baraka_alt_file_table, "baraka.mko"},
    {"NOOB", noob_file_table, noob_alt_file_table, "noob.mko"},
    {"SUB-ZERO", subzero_file_table, subzero_alt_file_table, "subzero.mko"},
    {"MILEENA", mileena_file_table, mileena_alt_file_table, "mileena.mko"},
    {"NIGHTWOLF", nightwolf_file_table, nightwolf_alt_file_table,
     "nightwolf.mko"},
    {"ERMAC", ermac_file_table, ermac_alt_file_table, "ermac.mko"},
    {"ASHRAH", ashrah_file_table, ashrah_alt_file_table, "ashrah.mko"},
    {"SINDEL", sindel_file_table, sindel_alt_file_table, "sindel.mko"},
    {"LI MEI", limei_alt_file_table, limei_file_table, "limei.mko"},
    {"BO' RAI CHO", boraicho_file_table, boraicho_alt_file_table,
     "boraicho.mko"},
    {"HOTARU", hotaru_file_table, hotaru_alt_file_table, "hotaru.mko"},
    {"KENSHI", kenshi_file_table, kenshi_alt_file_table, "kenshi.mko"},
    {"SMOKE", smoke_file_table, smoke_alt_file_table, "smoke.mko"},
    {"HAVIK", skab_file_table, skab_alt_file_table, "skab.mko"},
    {"TANYA", tanya_file_table, tanya_alt_file_table, "tanya.mko"},
    {"LIU KANG", liukang_alt_file_table, liukang_file_table, "liukang.mko"},
    {"GHOST", liukang_ghost_file_table, liukang_ghost_file_table,
     "liukang_ghost.mko"},
    {"KIRA", kira_file_table, kira_alt_file_table, "kira.mko"},
    {"KABAL", kabal_file_table, kabal_alt_file_table, "kabal.mko"},
    {"KOBRA", kobra_file_table, kobra_alt_file_table, "kobra.mko"},
    {"JADE", jade_file_table, jade_alt_file_table, "jade.mko"},
    {"DAIROU", dairou_file_table, dairou_alt_file_table, "dairou.mko"},
    {"RAIDEN", raiden_file_table, raiden_alt_file_table, "raiden.mko"},
    {"DARRIUS", cassius_file_table, cassius_alt_file_table, "cassius.mko"},
    {"SHUJINKO", shujinko_file_table, shujinko_alt_file_table,
     "shujinko.mko"},
    {"SHUJINKO", shujinko_13_file_table, shujinko_13_file_table,
     "shujinko_13.mko"},
    {"NOOB - SMOKE", noobsmoke_alt_file_table, noobsmoke_file_table,
     "noobsmoke.mko"},
    {"MONSTER", freak_file_table, freak_file_table, "freak.mko"},
    {"ONAGA", dragonking_file_table, dragonking_file_table,
     "dragonking.mko"},
    {"GORO", goro_file_table, goro_alt_file_table, "goro.mko"},
    {"SHAO KAHN", shaokahn_file_table, shaokahn_alt_file_table,
     "shaokahn.mko"},
    {"RANDOM", shaokahn_file_table, shaokahn_alt_file_table,
     "shaokahn.mko"},
    {"JAX", mkda_jax_file_table, mkda_jax_alt_file_table, "mkda_jax.mko"},
    {"RAIDEN", mkda_rayden_file_table, mkda_rayden_file_table,
     "mkda_rayden.mko"},
    {"QUAN CHI", mkda_quanchi_file_table, mkda_quanchi_file_table,
     "mkda_quanchi.mko"},
    {"KUNG LAO", mkda_kunglao_file_table, mkda_kunglao_file_table,
     "mkda_kunglao.mko"},
    {"JOHNNY CAGE", mkda_cage_file_table, mkda_cage_file_table,
     "mkda_cage.mko"},
    {"SONYA", mkda_sonya_file_table, mkda_sonya_file_table,
     "mkda_sonya.mko"},
    {"NITARA", mkda_nitara_file_table, mkda_nitara_file_table,
     "mkda_nitara.mko"},
    {"SHANG TSUNG", mkda_shang_file_table, mkda_shang_file_table,
     "mkda_shang.mko"},
    {"FROST", mkda_frost_file_table, mkda_frost_file_table,
     "mkda_frost.mko"},
    {"KITANA", mkda_kitana_file_table, mkda_kitana_file_table,
     "mkda_kitana.mko"},
    {"DRAHMIN", mkda_drahmin_file_table, mkda_drahmin_file_table,
     "mkda_drahmin.mko"},
};

unsigned char goro_hand_to_hand2_remapping[0x56] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x49, 0x56, 0x1C, 0x1D, 0x4B, 0x58,
    0x20, 0x21, 0x4D, 0x5A, 0x4A, 0x57, 0x26, 0x27,
    0x4C, 0x59, 0x2A, 0x2B, 0x4E, 0x5B, 0x4F, 0x5C,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x42, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
};
extern float r_call_script_function(void);
extern void fxbanks_unload_by_owner(int owner);
extern void kill_fstyle_signs_for_plyr(PlyrInfo* player);
extern void term_player_collision(PlyrInfo* player);
extern void unload_script(int slot);
extern void transition_to_anim_script(
    AnimPdata* animation, AniData* script, int flags, float blend);
extern void obj_set_gravity(MkObj* object, float gravity);
extern void snd_req_delay(int sound, int delay, int flags);
extern void animpdata_ani_to_frame_x(
    AnimPdata* animation, float frame);
extern void animpdata_ani_loop_more_frames(
    AnimPdata* animation, float frames);
extern void update_bone_hierarchy(MkHdr* object);
extern void ground_me(MkHdr* object);
extern MslSoundHandle random_foot(int group);
extern float p_anim_idle(void);

typedef struct JawMovement {
    float delta;
    int ticks;
} JawMovement;

typedef struct BarakaJawPdata {
    MkHdr hdr;
    AnimPdata* animation;
    unsigned int animation_instance;
    const JawMovement* pending_movement;
    MkBone* jaw_bone;
    Quat saved_rotation;
    const JawMovement* active_movement;
    int movement_index;
    int movement_ticks;
} BarakaJawPdata;

JawMovement idle_jaw_movement[5] = {
    {-0.03f, 30},
    {-0.05f, 15},
    {0.05f, 30},
    {0.01f, 15},
    {0.0f, -2},
};

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

typedef struct PlyrSidekickProcPdata {
    MkHdr hdr;
    PlyrPdata* player;
} PlyrSidekickProcPdata;

static float p_plyr_sidekick(void);

struct AnimEntryName {
    unsigned int identifier;
    char name[1];
};

typedef struct PlyrProcVtable {
    void* reserved[6];
    void (*sleep)(void);
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
    struct {
        unsigned char hidden : 1;
        unsigned char pad : 7;
    } flags_2C;
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



static float p_plyr_aux2(void);

int fetch_shujinko_special_number_for(unsigned int move_id);

/*
 * Soft ceiling: retail behavior and ABI recovered. Remaining differences are
 * branch-arm layout, one helper-result register copy, and predicate lowering.
 */
int is_special_move_available(PlyrPdata* pdata, int move_id) {
    void* profile = p1_profile;
    int special_number;
    int character = pdata->character_id;

    if (character == 0x19 || character == 0x1A) {
        if (pdata->plyr_num == 1) {
            profile = p2_profile;
        }
        special_number = fetch_shujinko_special_number_for(move_id);
        if (special_number < 0) {
            return special_number == -2;
        }
        if (is_mark_as_unlocked(profile, 4) == 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * Soft ceiling: all mapped-move comparisons match in retail order. Retail
 * retains two sentinel comparisons that share the same -2 result as default.
 */
int fetch_shujinko_special_number_for(unsigned int move_id) {
    if (move_id == 0x1203) return 4;
    if (move_id == 0x1205) return 5;
    if (move_id == 0x421E) return 2;
    if (move_id == 0x4219) return 1;
    if (move_id == 0x4241) return 3;
    if (move_id == 0x4242) return 6;
    if (move_id == 0x421B) return 0;
    if (move_id == 0x4239) return 7;
    if (move_id == 0x4243) return 8;
    if (move_id == 0x4244) return 9;
    if (move_id == 0x4245) return 10;
    if (move_id == 0x3FFEFFFE) return -2;
    return -2;
}void tag_team_activate_player(MkObj* object, int active) {
    PlyrClumpView* clump;
    PlyrClumpLink* link;
    PlyrClumpLink* sentinel;

    clump = (PlyrClumpView*)object->clump;
    if (clump == 0) {
        return;
    }

    link = clump->geometry_list.next;
    sentinel = &clump->geometry_list;
    while (link != sentinel) {
        PlyrClumpNode* node =
            (PlyrClumpNode*)((char*)link - 0x28);
        PlyrGeometryView* geometry =
            (PlyrGeometryView*)node->geometry;
        unsigned int offset = 0;
        unsigned int count = geometry->material_count;
        unsigned int i;

        for (i = 0; i < count; i++) {
            RpMaterial* material =
                *(RpMaterial**)((char*)geometry->materials + offset);
            unsigned int local =
                *(unsigned int*)((char*)material +
                                 MkmaterialLocalOffset);
            PlyrSpecularMaterialView* specular =
                (PlyrSpecularMaterialView*)
                    ((char*)material + SpecularMaterialOffset);
            int primary = ((local >> 10) & 1) ^ 1;
            int alternate;

            local %= 0x1000;
            local &= ~0x400;
            alternate = (local % 10) > 5;

            if (active != 0) {
                if (primary != 0 && alternate == 0) {
                    specular->flags_2C.hidden = 0;
                } else {
                    specular->flags_2C.hidden = 1;
                }
            } else if (primary != 0 || alternate != 0) {
                specular->flags_2C.hidden = 1;
            } else {
                specular->flags_2C.hidden = 0;
            }
            offset += sizeof(*geometry->materials);
        }
        link = link->next;
    }
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

void plyr_start_script_in_plyr_pdata_proc(
    FighterMirror* fighter, int proc_id, unsigned int function) {
    PlyrScriptProcPdata* pdata;
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

int plyr_in_spin_react(PlyrPdata* pdata) {
    MkProc* proc = pdata->anim_proc;
    AnimPdata* animation;

    if (proc != 0) {
        if (proc->instance == (int)pdata->anim_proc_instance) {
            /* Keep the live process. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc != 0) {
        animation = (AnimPdata*)pdata_of_proc(proc);
        if (animation != 0 &&
            (strcmp(animation->script_entry->name,
                    "SS_R_kabaldash_2_lo") == 0 ||
             strcmp(animation->script_entry->name,
                    "SS_R_kabaldash_2_in") == 0)) {
            return 1;
        }
    }
    return 0;
}

int plyr_pdata_is_alt_costume(PlyrInfo* player) {
    return player->flags_14_bits.alternate_costume;
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

float plyr_anim_get_frame(AnimPdata* pdata) {
    return pdata->frame;
}

AnimPdata* get_my_plyr_anim_pdata(void) {
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

int get_my_particle_player_bank_num(void) {
    return plyr_pdata->plyr_num != 0 ? 2 : 1;
}

int get_my_plyr_num(void) {
    return plyr_pdata->plyr_num;
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

MkObj* get_my_plyr_obj(void) {
    return plyr_obj;
}

MkObj* get_his_plyr_obj(void) {
    return plyr_pdata->his_obj;
}

PlyrInfo* get_plyr_info(void) {
    return plyr_pdata->plyr_info;
}

void cleanup_player_globals(void) {
}

float plyr_get_pos(unsigned int axis) {
    float position = plyr_obj->pos.value.x;

    if (axis == 1) {
        position = plyr_obj->pos.value.y;
    } else if (axis == 2) {
        position = plyr_obj->pos.value.z;
    }
    return position;
}

void set_player_state(PlyrInfo* player, int state) {
    player->player_state = state;
}

void switch_to_bgnd_moveset(PlyrPdata* pdata, int moveset_index) {
    GlobalMoveset* moveset;
    MkObj* primary;
    MkObj* secondary;

    if (moveset_index >= 2) {
        return;
    }
    moveset = &global_movesets[moveset_index + 6];
    primary = moveset->primary_weapon;
    if (primary != 0) {
        if (primary->hdr.instance == moveset->primary_weapon_instance) {
            /* Keep the live weapon. */
        } else {
            primary = 0;
        }
    } else {
        primary = 0;
    }
    secondary = moveset->secondary_weapon;
    if (secondary != 0) {
        if (secondary->hdr.instance ==
            moveset->secondary_weapon_instance) {
            /* Keep the live weapon. */
        } else {
            secondary = 0;
        }
    } else {
        secondary = 0;
    }
    if (primary != 0 || secondary != 0) {
        plyr_weapon_release(pdata);
        plyr_weapon2_release(pdata);
    }
    plyr_weapon_hide(
        pdata, 1, (PlyrMirrorSlots*)&pdata->global_moveset->primary_weapon);
    pdata->player_slot = 3;
    pdata->global_moveset = moveset;
    pdata->global_moveset_definition = pdata->global_moveset->definition;
    pdata->mirror_slots =
        (PlyrMirrorSlots*)&pdata->global_moveset->primary_weapon;
    if (pdata->baraka_moveset_callback != 0) {
        pdata->baraka_moveset_callback(
            pdata, (PlyrMirrorSlots*)&pdata->global_moveset->primary_weapon);
    }
    snd_req(0xDC7);
    if (primary != 0) {
        MkObj* player_object;

        plyr_weapon_grab(pdata, primary);
        plyr_weapon_show(
            pdata, 1, (PlyrMirrorSlots*)&moveset->primary_weapon);
        player_object = plyr_pdata->tracked_obj;
        if (player_object != 0) {
            if (player_object->hdr.instance ==
                plyr_pdata->tracked_obj_instance) {
                /* Keep the live player object. */
            } else {
                player_object = 0;
            }
        } else {
            player_object = 0;
        }
        if (player_object != 0) {
            MkObj* reflection = moveset->reflection_weapon;

            if (reflection != 0) {
                if (reflection->hdr.instance ==
                    moveset->reflection_weapon_instance) {
                    /* Keep the live reflection. */
                } else {
                    reflection = 0;
                }
            } else {
                reflection = 0;
            }
            if (reflection == 0) {
                reflection = load_bgnd_weapon_reflection(
                    (WeaponDefinition*)primary->field_5C);
                if (reflection != 0) {
                    moveset->reflection_weapon = reflection;
                    moveset->reflection_weapon_instance =
                        reflection->hdr.instance;
                    mk_insert(
                        &reflection->hdr,
                        &moveset->reflection_owner->reflections);
                }
            }
        }
        primary->light_flags = 0x100C;
    }
    if (secondary != 0) {
        plyr_weapon2_grab(pdata, secondary);
        plyr_weapon_show(
            pdata, 1, (PlyrMirrorSlots*)&moveset->primary_weapon);
        secondary->light_flags = 0x100C;
    }
    pdata->global_moveset_definition = moveset->definition;
    show_fighting_style(moveset, pdata->plyr_num);
}

void register_baraka_cb_functions(void) {
    plyr_pdata->baraka_moveset_callback = baraka_advance_active_moveset;
}

typedef struct BarakaMovesetWeapons {
    PlyrMirrorObjLatch primary;
    PlyrMirrorObjLatch reflection;
    char pad10[8];
    PlyrMirrorObjLatch secondary;
} BarakaMovesetWeapons;

static inline MkObj* baraka_live_object(PlyrMirrorObjLatch* latch) {
    MkObj* object = latch->obj;

    if (object != 0) {
        if (object->hdr.instance != latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    return object;
}

static inline BarakaBladesPdata* baraka_blades(PlyrPdata* pdata) {
    MkProc* proc = pdata->baraka_blades_monitor;

    if (proc != 0) {
        if (proc->instance !=
            (int)pdata->baraka_blades_monitor_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc == 0) {
        return 0;
    }
    return (BarakaBladesPdata*)pdata_of_proc(proc);
}

static inline void baraka_retract_blades(PlyrPdata* pdata) {
    BarakaBladesPdata* blades = baraka_blades(pdata);
    PlyrMirrorSlots* slots;
    MkObj* first;
    MkObj* second;

    if (blades == 0) {
        return;
    }
    blades->blade_a_target = 0.0f;
    blades->blade_a_step = -0.1f;
    blades->blade_b_target = 0.0f;
    blades->blade_b_step = -0.1f;

    slots = pdata->mirror_slots;
    if (slots == 0) {
        return;
    }
    first = baraka_live_object(&slots->weapon[0].secondary);
    second = baraka_live_object(&slots->weapon[1].secondary);
    if (first != 0 && second != 0) {
        first->flags_09_bits.tightrope_restricted = 1;
        first->flags_09_bits.bit6 = 0;
        first->flags_09_bits.launched = 1;
        first->flags_08_bits.moving = 0;
        second->flags_09_bits.tightrope_restricted = 1;
        second->flags_09_bits.bit6 = 0;
        second->flags_09_bits.launched = 1;
        second->flags_08_bits.moving = 0;
        blades->command = 0;
    }
}

static inline void baraka_extend_blades(PlyrPdata* pdata) {
    BarakaBladesPdata* blades = baraka_blades(pdata);

    if (blades != 0) {
        blades->blade_a_target = 1.0f;
        blades->blade_a_step = 0.1f;
        blades->blade_b_target = 1.0f;
        blades->blade_b_step = 0.1f;
        if (is_load_meter_active() == 0) {
            snd_req(0x163);
        }
    }
}

static int baraka_advance_active_moveset(
    PlyrPdata* pdata, PlyrMirrorSlots* context) {
    BarakaMovesetWeapons* weapons = (BarakaMovesetWeapons*)context;
    MkObj* object;

    object = baraka_live_object(&weapons->primary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
    }
    object = baraka_live_object(&weapons->secondary);
    if (object != 0) {
        object->hide_flag_bits.hidden = 1;
    }

    if (context == &pdata->weapon_styles[2]->mirror_slots) {
        baraka_retract_blades(pdata);
        baraka_extend_blades(pdata);
    } else {
        baraka_retract_blades(pdata);
        plyr_weapon_show(pdata, 1, context);
    }
    return 0;
}

static inline void plyr_start_script_in_slot(
    ScriptSlot* script, int proc_id, unsigned int function) {
    PlyrScriptProcPdata* pdata;
    MkProc* proc;

    proc = _create_mkproc_generic_tinystack(
        proc_id, 0x1F, p_plyr_script_in_proc, sizeof(*pdata),
        (MkHdr**)&pdata);
    if (proc != 0 && pdata != 0) {
        pdata->script = script;
        pdata->func_index = function;
        set_process_as_scriptable(proc);
    }
}

void show_baraka_one_blade_only(PlyrPdata* pdata, int first_blade) {
    MkProc* proc = pdata->baraka_blades_monitor;
    BarakaBladesPdata* blades;

    if (proc != 0) {
        if (proc->instance !=
            (int)pdata->baraka_blades_monitor_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc != 0) {
        blades = (BarakaBladesPdata*)pdata_of_proc(proc);
        if (blades != 0) {
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
    }
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

float p_baraka_blades_controller(void) {
    BarakaBladesPdata* blades = (BarakaBladesPdata*)apdata;
    MkBone* blade;
    float value;
    float step;

    blade = blades->blade_a;
    value = blade->scale.x;
    if (value != blades->blade_a_target) {
        blade->scale.x = value + blades->blade_a_step;
        step = blades->blade_a_step;
        if ((step > 0.0f &&
             blades->blade_a->scale.x > blades->blade_a_target) ||
            (step < 0.0f &&
             blades->blade_a->scale.x < blades->blade_a_target)) {
            blades->blade_a->scale.x = blades->blade_a_target;
            if (blades->blade_a_target != 0.0f) {
                blades->command = 1;
            }
        }
    }
    blade = blades->blade_b;
    value = blade->scale.x;
    if (value != blades->blade_b_target) {
        blade->scale.x = value + blades->blade_b_step;
        step = blades->blade_b_step;
        if ((step > 0.0f &&
             blades->blade_b->scale.x > blades->blade_b_target) ||
            (step < 0.0f &&
             blades->blade_b->scale.x < blades->blade_b_target)) {
            blades->blade_b->scale.x = blades->blade_b_target;
            if (blades->blade_b_target != 0.0f) {
                blades->command = 1;
            }
        }
    }
    return 1.0f;
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
        pdata.jaw->pending_movement = 0;
        pdata.jaw->active_movement = 0;
        plyr_pdata->jaw_monitor = monitor.proc;
        plyr_pdata->jaw_monitor_instance = monitor.proc->instance;
        pdata.jaw->jaw_bone = get_bone_with_tag(plyr_obj, 1);
        mk_insert(monitor.hdr, &plyr_obj->child_list);
    }
}

static float p_baraka_jaw_controller(void) {
    BarakaJawPdata* pdata = (BarakaJawPdata*)apdata;
    MKMATRIX matrix __attribute__((aligned(16)));
    Quat rotation;
    AnimPdata* animation;
    int next_ticks;

    if (pdata == 0) {
        return -1.0f;
    }
    if (pdata->active_movement == 0) {
        if (pdata->pending_movement == 0) {
            if ((unsigned short)randu0(1000) < 10) {
                pdata->active_movement = idle_jaw_movement;
                pdata->movement_index = 0;
                pdata->movement_ticks = pdata->active_movement->ticks;
                gxQuatCopy(
                    &pdata->saved_rotation, &pdata->jaw_bone->rotation);
            }
        } else {
            pdata->active_movement = pdata->pending_movement;
            pdata->movement_index = 0;
            pdata->movement_ticks = pdata->active_movement->ticks;
            pdata->pending_movement = 0;
            gxQuatCopy(&pdata->saved_rotation, &pdata->jaw_bone->rotation);
        }
    }
    if (pdata->active_movement != 0) {
        if (--pdata->movement_ticks == 0) {
            next_ticks =
                pdata->active_movement[pdata->movement_index + 1].ticks;
            if (next_ticks < 0) {
                if (next_ticks == -2) {
                    gxQuatCopy(
                        &pdata->jaw_bone->rotation, &pdata->saved_rotation);
                }
                pdata->active_movement = 0;
            } else {
                pdata->movement_index++;
                pdata->movement_ticks =
                    idle_jaw_movement[pdata->movement_index].ticks;
            }
        } else {
            quat_to_mat(&matrix, &pdata->jaw_bone->rotation);
            matrix.at.y +=
                pdata->active_movement[pdata->movement_index].delta;
            normalize_v3((Vec*)&matrix.at);
            if (RtQuatConvertFromMatrix(&rotation, &matrix) != 0) {
                gxQuatCopy(&pdata->jaw_bone->rotation, &rotation);
                gxQuatNorm(&pdata->jaw_bone->rotation);
            }
        }
    }

    animation = pdata->animation;
    if (animation != 0) {
        if (animation->hdr.instance != pdata->animation_instance) {
            animation = 0;
        }
    } else {
        animation = 0;
    }
    if (animation != 0) {
        animation->flags |= 0x4000;
        animation->old_flags |= 0x4000;
    }
    return 1.0f;
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
    int object_id = (int)object->oid;

    if (object_id == 0x1001) {
        return 0;
    }
    if (object_id == 0x1002) {
        return 1;
    }
    return -1;
}

/* Soft ceiling: 77.64% - visibility and owned-child filtering recovered. */
void show_player(PlyrPdata* player) {
    MkObj* object;
    MkObj* fighter;
    MkPtr* link;

    if (player == 0) {
        return;
    }
    object = player->tracked_obj;
    if (object != 0) {
        if (object->hdr.instance == player->tracked_obj_instance) {
            /* Keep the live object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        unhide_obj(object);
    }
    if (is_weapon_style(player->fighter_definition) != 0) {
        plyr_weapon_show(player, 1, player->mirror_slots);
    }

    fighter = player->tracked_obj;
    if (fighter != 0) {
        if (fighter->hdr.instance == player->tracked_obj_instance) {
            /* Keep the live object. */
        } else {
            fighter = 0;
        }
    } else {
        fighter = 0;
    }
    if (fighter != 0) {
        object = player->aux_weapon_latch.obj;
        if (object != 0) {
            if (object->hdr.instance ==
                player->aux_weapon_latch.instance) {
                /* Keep the live object. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            unhide_obj(object);
        }
        object = player->mirror_obj.obj;
        if (object != 0) {
            if (object->hdr.instance == player->mirror_obj.instance) {
                /* Keep the live object. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
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
                object = (MkObj*)link->hdr;
                if (object->hdr.vtbl == &vtbl_mkobj) {
                    /* Keep the object-typed link. */
                } else {
                    object = 0;
                }
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

    object = player->tracked_obj;
    if (object != 0) {
        if (object->hdr.instance == player->tracked_obj_instance) {
            /* Keep the live object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        hide_obj(object);
    }
    if (hide_weapons != 0) {
        for (i = 0; i < 3; i++) {
            plyr_weapon_hide(
                player, 1, &player->weapon_styles[i]->mirror_slots);
        }
    }

    fighter = player->tracked_obj;
    if (fighter != 0) {
        if (fighter->hdr.instance == player->tracked_obj_instance) {
            /* Keep the live object. */
        } else {
            fighter = 0;
        }
    } else {
        fighter = 0;
    }
    if (fighter != 0) {
        object = player->aux_weapon_latch.obj;
        if (object != 0) {
            if (object->hdr.instance ==
                player->aux_weapon_latch.instance) {
                /* Keep the live object. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            hide_obj(object);
        }
        object = player->mirror_obj.obj;
        if (object != 0) {
            if (object->hdr.instance == player->mirror_obj.instance) {
                /* Keep the live object. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
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
                object = (MkObj*)link->hdr;
                if (object->hdr.vtbl == &vtbl_mkobj) {
                    /* Keep the object-typed link. */
                } else {
                    object = 0;
                }
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

    v3_sub_v3(&translation, position, &object->pos.value);
    object->pos.value.x = position->x;
    object->pos.value.y = position->y;
    object->pos.value.z = position->z;
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

    v3_sub_v3(&translation, position, &object->pos.value);
    object->pos.value.x = position->x;
    object->pos.value.y = position->y;
    object->pos.value.z = position->z;
    object->ang.x = angles->x;
    object->ang.y = angles->y;
    object->ang.z = angles->z;
    update_obj_pos(object);
    obj_translate_cloth(object, &translation);
}

void xfer_player_proc(MkProc* proc, MkProcEntryFn entry) {
    CmdScript* script;

    pdata_of_proc(proc);
    script = get_cmdscript_for_proc(proc);
    script->state = 0;
    xfer_proc(proc, entry);
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
    pdata->flags.word = (unsigned int)*flags;
    return 1;
}

static float p_load_plyr_model_async(void) {
    LoadPlyrModelPdata* pdata = (LoadPlyrModelPdata*)apdata;
    MkFileEntry* model_files;
    ScriptSlot* script;
    PlyrModelDataTable* data;
    const char* art_section;
    const char* model_script;
    LoadPlyrFlags flags;
    int language;
    int alternate;
    int player;
    int char_id;

    if (pdata == 0) {
        return -1.0f;
    }
    player = pdata->player;
    language = 7;
    char_id = pdata->char_id;
    flags = pdata->flags;
    if (player == 0) {
        language = 3;
    }
    alternate = flags.bits.alternate;
    if (alternate != 0) {
        model_files =
            global_player_data[char_id].alternate_model_files;
    } else {
        model_files = global_player_data[char_id].model_files;
    }
    if (char_id < 0 || char_id >= 0x2C) {
        return -1.0f;
    }
    model_script = global_player_data[char_id].model_script;
    load_ssf(model_files);
    script = cmdscript_loadfile_by_name(language, model_script);
    if (script->table_count == 0) {
        return -1.0f;
    }
    data = (PlyrModelDataTable*)get_data_table(
        script, script->table_count);
    if (alternate != 0) {
        if (flags.bits.alternate_costume) {
            art_section = data->alternate_costume_art;
        } else {
            art_section = data->alternate_art;
        }
    } else if (flags.bits.alternate_costume) {
        art_section = data->primary_costume_art;
    } else {
        art_section = data->primary_art;
    }
    load_ssf(model_files);
    if (player == 0) {
        load_art_section_by_name_async(0x3000A, art_section);
        if (data->shared_art != 0) {
            load_art_section_by_name_async(0x3000B, data->shared_art);
        }
    } else if (player == 1) {
        load_art_section_by_name_async(0x4000A, art_section);
        if (data->shared_art != 0) {
            load_art_section_by_name_async(0x4000B, data->shared_art);
        }
    }
    return -1.0f;
}

int load_player_style_scripts(PlyrPdata* pdata) {
    int initial_language;
    int language;
    int index;

    initial_language = 8;
    if (pdata->plyr_num == 0) {
        initial_language = 4;
    }
    language = initial_language;
    for (index = 0; index < 3; index++, language++) {
        if (pdata->runtime_data->style_scripts[index] != 0) {
            pdata->weapon_styles[index]->script =
                cmdscript_loadfile_by_name(
                    language,
                    pdata->runtime_data->style_scripts[index]);
            if (pdata->weapon_styles[index]->script->table_count != 0) {
                pdata->weapon_styles[index]->definition =
                    (PlyrStyleDefinition*)get_data_table(
                        pdata->weapon_styles[index]->script,
                        pdata->weapon_styles[index]->script->table_count);
                pdata->weapon_styles[index]->animation_header =
                    pdata->weapon_styles[index]
                        ->definition->animation_header;
                if (is_weapon_style(
                        (PlyrFighterDefinition*)
                            pdata->weapon_styles[index]) != 0) {
                    PlyrWeaponStyle* style =
                        pdata->weapon_styles[index];
                    MkObj* player_object =
                        pdata->plyr_info->slot.mirror_a;

                    if (style->definition->primary_weapon != 0) {
                        MkObj* weapon = load_weapon(
                            style->definition->primary_weapon,
                            player_object);

                        if (weapon != 0) {
                            MkObj* reflection;

                            style->mirror_slots.weapon[0].primary.obj = weapon;
                            style->mirror_slots.weapon[0].primary.instance =
                                weapon->hdr.instance;
                            mk_insert(
                                &weapon->hdr,
                                &style->script->pdata_list);
                            reflection = load_weapon_reflection(
                                style->definition->primary_weapon,
                                player_object);
                            if (reflection != 0) {
                                style->mirror_slots.weapon[0].mirror.obj =
                                    reflection;
                                style->mirror_slots.weapon[0].mirror.instance =
                                    reflection->hdr.instance;
                                mk_insert(
                                    &reflection->hdr,
                                    &style->script->pdata_list);
                                obj_create_sobjs(reflection);
                                sobj_set_priority(
                                    obj_first_sobj(reflection), 6);
                            }
                        }
                    }
                    if (style->definition->secondary_weapon != 0) {
                        MkObj* weapon = load_weapon(
                            style->definition->secondary_weapon,
                            player_object);

                        if (weapon != 0) {
                            MkObj* reflection;

                            style->mirror_slots.weapon[1].primary.obj = weapon;
                            style->mirror_slots.weapon[1].primary.instance =
                                weapon->hdr.instance;
                            mk_insert(
                                &weapon->hdr,
                                &style->script->pdata_list);
                            reflection = load_weapon_reflection(
                                style->definition->secondary_weapon,
                                player_object);
                            if (reflection != 0) {
                                style->mirror_slots.weapon[1].mirror.obj =
                                    reflection;
                                style->mirror_slots.weapon[1].mirror.instance =
                                    reflection->hdr.instance;
                                mk_insert(
                                    &reflection->hdr,
                                    &style->script->pdata_list);
                                obj_create_sobjs(reflection);
                                sobj_set_priority(
                                    obj_first_sobj(reflection), 6);
                            }
                        }
                    }
                }
                generate_ai_table_moveset(
                    pdata->weapon_styles[index]);
            }
        }
    }
    return 1;
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

static inline void pw_plyr_inline(void) {
    MkObj* object;
    MkProc* anim_proc;
    MkProc* hand_proc;

    plyr_pdata = (PlyrPdata*)apdata;
    object = plyr_pdata->tracked_obj;
    if (object != 0) {
        if (object->hdr.instance == plyr_pdata->tracked_obj_instance) {
            /* Keep the live object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    plyr_obj = object;
    anim_proc = plyr_pdata->anim_proc;
    if (anim_proc != 0) {
        if (anim_proc->instance ==
            (int)plyr_pdata->anim_proc_instance) {
            /* Keep the live process. */
        } else {
            anim_proc = 0;
        }
    } else {
        anim_proc = 0;
    }
    plyr_anim_proc = anim_proc;
    if (anim_proc != 0) {
        plyr_anim_pdata = (AnimPdata*)pdata_of_proc(anim_proc);
        if (g_perform_validation == 1 &&
            plyr_pdata->saved_anim_script_word != 0) {
            get_game_state();
        }
        g_perform_validation = 0;
    }
    hand_proc = plyr_pdata->left_hand_anim_proc;
    if (hand_proc != 0) {
        if (hand_proc->instance ==
            (int)plyr_pdata->left_hand_anim_instance) {
            /* Keep the live process. */
        } else {
            hand_proc = 0;
        }
    } else {
        hand_proc = 0;
    }
    plyr_lefthand_anim_proc = hand_proc;
    hand_proc = plyr_pdata->right_hand_anim_proc;
    if (hand_proc != 0) {
        if (hand_proc->instance ==
            (int)plyr_pdata->right_hand_anim_instance) {
            /* Keep the live process. */
        } else {
            hand_proc = 0;
        }
    } else {
        hand_proc = 0;
    }
    plyr_righthand_anim_proc = hand_proc;
    if (g_game_info.plyr0.slot.pdata != 0 &&
        g_game_info.plyr1.slot.pdata != 0) {
        his_obj = plyr_pdata->his_obj;
        his_pdata = plyr_pdata->his_plyr_pdata;
    }
}

void pw_plyr(void) {
    pw_plyr_inline();
}

static float p_plyr_aux(void) {
    if (aproc->pid == 0x100A) {
        if (g_game_info.plyr1.slot.mirror_a == 0) {
            return 1.0f;
        }
    } else {
        if (g_game_info.plyr0.slot.mirror_a == 0) {
            return 1.0f;
        }
    }
    ((PlyrProcVtable*)aproc->vtbl)->jump_sleep(p_plyr_aux2, 0.0f);
    return 0.0f;
}

static float p_plyr_aux2(void) {
    if (plyr_pdata->postround_value != 0.0f &&
        (g_game_info.flags & 1) == 0 &&
        !g_game_info.pause_flag_bits.fatality_window) {
        if (plyr_pdata->plyr_num == 0) {
            damage_p1(-1.0f * plyr_pdata->postround_value);
        } else {
            damage_p2(-1.0f * plyr_pdata->postround_value);
        }
    }
    if (plyr_pdata->drone_request == 1) {
        drone_ai_watcher();
    } else if (plyr_pdata->aux_update_callback != 0) {
        plyr_pdata->aux_update_callback();
    }
    return 1.0f;
}

float player_sleep_forever(void) {
    return 1.0f;
}

void set_attack_type(int attack_type) {
    plyr_pdata->attack_type = attack_type;
}

static inline void initialize_player_shadow(
    int pid, MkObj* shadow) {
    PlyrPdata* pdata = plyr_pdata;
    MkObj* object = plyr_obj;

    if (shadow != 0) {
        obj_create_sobjs(shadow);
        sobj_set_priority(obj_first_sobj(shadow), 6);
        init_shadow((ShadowObject*)pdata, shadow);
        if (pdata->shadowbox != 0) {
            create_shadow_proc(pid, pdata, object, shadow);
        }
    }
}

float p_plyr_start(void) {
    FighterRuntimeData* runtime;
    MkObj* shadow = 0;
    MkObj* sidekick;
    MkObj* saved_object;
    MkProc* proc;
    MkProc* head_proc;
    unsigned int start_script;
    int shadow_slot;
    int player_state;

    runtime = plyr_pdata->runtime_data;
    set_root_and_obj_movement_weights(0.0f, 1.0f, plyr_anim_pdata);
    set_anim_script(
        plyr_anim_pdata,
        plyr_pdata->fighter_definition->duck_exit_animation,
        0x20);

    if (g_game_info.field_08 != 0 &&
        (g_game_info.section->flags70 & 9) != 0) {
        shadow_slot = plyr_pdata->plyr_num == 0 ? 0x3000A : 0x4000A;
        shadow = (MkObj*)load_named_model_from_slot(
            shadow_slot, "REFLECT", 0x5002, 0);
        if (shadow != 0) {
            SetupShadowPlayerPipeline(shadow->clump);
            build_bones_tbl(shadow, shadow_bones);
            pull_bone_hierarchy_mkobj(shadow);
            if ((g_game_info.section->flags70 & 1) != 0) {
                insert_fgnd_mkobj(shadow);
            }
            if ((g_game_info.section->flags70 & 8) == 0) {
                hide_obj(shadow);
            }
            shadow->light_flags = 4;
            if (plyr_pdata->plyr_info->flags_14_bits.alternate_costume) {
                plyr_pdata->mirror_bone_map =
                    runtime->alternate_mirror_bone_map;
            } else {
                plyr_pdata->mirror_bone_map =
                    runtime->primary_mirror_bone_map;
            }
            if (aproc->pid == 0x1001) {
                initialize_player_shadow(0x6002, shadow);
            } else {
                initialize_player_shadow(0x6003, shadow);
            }
        }
    }
    if (aproc->pid == 0x1001) {
        g_game_info.plyr0.slot.mirror_b = shadow;
    } else {
        g_game_info.plyr1.slot.mirror_b = shadow;
    }

    if (plyr_pdata->plyr_info->flags_14_bits.alternate_costume) {
        plyr_obj->ground_colls = runtime->alternate_ground_collision;
        start_script = runtime->alternate_start_script;
    } else {
        start_script = runtime->primary_start_script;
        plyr_obj->ground_colls = runtime->primary_ground_collision;
    }
    if (start_script != 0) {
        cmdscript_setup_execution(plyr_pdata->cmo, start_script);
        cmdscript_execute(plyr_pdata->cmo);
    }

    if (aproc->pid == 0x1001) {
        while (g_game_info.plyr1.slot.mirror_a == 0 ||
               g_game_info.plyr1.slot.pdata == 0) {
            _mkproc_sleep_ticks = 2.0f;
            ((PlyrProcVtable*)aproc->vtbl)->sleep();
        }
        plyr_pdata->opponent_proc = (MkProc*)g_game_info.plyr1.idle_proc;
        plyr_pdata->opponent_proc_instance =
            ((MkProc*)g_game_info.plyr1.idle_proc)->instance;
        plyr_pdata->his_plyr_pdata = g_game_info.plyr1.slot.pdata;
        plyr_pdata->his_obj = g_game_info.plyr1.slot.mirror_a;
    } else {
        while (g_game_info.plyr0.slot.mirror_a == 0 ||
               g_game_info.plyr0.slot.pdata == 0) {
            _mkproc_sleep_ticks = 2.0f;
            ((PlyrProcVtable*)aproc->vtbl)->sleep();
        }
        plyr_pdata->opponent_proc = (MkProc*)g_game_info.plyr0.idle_proc;
        plyr_pdata->opponent_proc_instance =
            ((MkProc*)g_game_info.plyr0.idle_proc)->instance;
        plyr_pdata->his_plyr_pdata = g_game_info.plyr0.slot.pdata;
        plyr_pdata->his_obj = g_game_info.plyr0.slot.mirror_a;
    }

    plyr_pdata = (PlyrPdata*)apdata;
    sidekick = plyr_pdata->tracked_obj;
    if (sidekick != 0) {
        if (sidekick->hdr.instance != plyr_pdata->tracked_obj_instance) {
            sidekick = 0;
        }
    } else {
        sidekick = 0;
    }
    plyr_obj = sidekick;
    proc = plyr_pdata->anim_proc;
    if (proc != 0) {
        if (proc->instance != (int)plyr_pdata->anim_proc_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    plyr_anim_proc = proc;
    if (proc != 0) {
        plyr_anim_pdata = (AnimPdata*)pdata_of_proc(proc);
        if (g_perform_validation == 1 &&
            plyr_pdata->saved_anim_script_word != 0) {
            get_game_state();
        }
        g_perform_validation = 0;
    }
    proc = plyr_pdata->left_hand_anim_proc;
    if (proc != 0) {
        if (proc->instance != (int)plyr_pdata->left_hand_anim_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    plyr_lefthand_anim_proc = proc;
    proc = plyr_pdata->right_hand_anim_proc;
    if (proc != 0) {
        if (proc->instance != (int)plyr_pdata->right_hand_anim_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    plyr_righthand_anim_proc = proc;
    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        his_obj = plyr_pdata->his_obj;
        his_pdata = plyr_pdata->his_plyr_pdata;
    }

    plyr_pdata->taunt_life_scale = 1.0f;
    plyr_pdata->fatality_advance = 0;
    plyr_pdata->facial_damage_complete = 0;
    head_proc = create_mkproc_headtracking(0x6005, plyr_obj, plyr_pdata);
    if (head_proc != 0) {
        mk_insert((MkHdr*)head_proc, &plyr_obj->child_list);
    }
    saved_object = plyr_obj;
    if (plyr_pdata->sidekick_available != 0) {
        sidekick = plyr_pdata->sidekick_obj;
        if (sidekick != 0) {
            if (sidekick->hdr.instance != plyr_pdata->sidekick_instance) {
                sidekick = 0;
            }
        } else {
            sidekick = 0;
        }
        plyr_obj = sidekick;
        runtime = plyr_pdata->runtime_data;
        if (plyr_pdata->plyr_info->flags_14_bits.alternate_costume) {
            sidekick->ground_colls = runtime->alternate_ground_collision;
            start_script = runtime->alternate_start_script;
        } else {
            start_script = runtime->primary_start_script;
            sidekick->ground_colls = runtime->primary_ground_collision;
        }
        if (start_script != 0) {
            cmdscript_setup_execution(plyr_pdata->cmo, start_script);
            cmdscript_execute(plyr_pdata->cmo);
        }
        head_proc = create_mkproc_headtracking(
            0x6005, plyr_obj, plyr_pdata);
        if (head_proc != 0) {
            mk_insert((MkHdr*)head_proc, &plyr_obj->child_list);
        }
        plyr_obj = saved_object;
    }
    player_initialize_chores();
    player_state = aproc->pid == 0x1001
        ? g_game_info.plyr0.player_state
        : g_game_info.plyr1.player_state;
    if ((int)mode_of_play == 6) {
        ((PlyrProcVtable*)aproc->vtbl)->jump_sleep(
            p_plyr_pz_fighter_start, 0.0f);
        return 0.0f;
    } else if (player_state == 0) {
        ((PlyrProcVtable*)aproc->vtbl)->jump_sleep(drone_start, 0.0f);
        return 0.0f;
    } else {
        ((PlyrProcVtable*)aproc->vtbl)->jump_sleep(p_joy_start, 0.0f);
        return 0.0f;
    }
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

void plyr_turn_off_shadowbox(PlyrInfo* player) {
    if (player->slot.mirror_b != 0) {
        hide_obj(player->slot.pdata->shadowbox);
    }
}

void plyr_turn_on_mirrorguy(PlyrInfo* player) {
    if ((g_game_info.section->flags70 & 8) != 0 &&
        player->slot.mirror_b != 0) {
        unhide_obj(player->slot.mirror_b);
    }
}

void plyr_turn_off_mirrorguy(PlyrInfo* player) {
    if (player->slot.mirror_b != 0) {
        hide_obj(player->slot.mirror_b);
    }
}

void delete_player(int player_index) {
    PlyrInfo* player;
    PlyrPdata* pdata;
    MkObj* object;
    int sidekick_player;
    int shadow_pid;

    player = &g_game_info.plyr0;
    if (player_index == 0) {
        shadow_pid = 0x6002;
        fxbanks_unload_by_owner(1);
    } else if (player_index == 1) {
        player = &g_game_info.plyr1;
        shadow_pid = 0x6003;
        fxbanks_unload_by_owner(2);
    } else {
        return;
    }
    pdata = player->slot.pdata;
    if (pdata != 0) {
        object = pdata->tracked_obj;
        if (object != 0) {
            if (object->hdr.instance != pdata->tracked_obj_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            if (object == player->slot.mirror_a) {
                if (object->hdr.instance != 0) {
                    ((void (*)(MkHdr*))object->hdr.vtbl->destroy)(
                        (MkHdr*)object);
                }
                player->slot.mirror_a = 0;
            }
            player->slot.pdata->tracked_obj = 0;
            player->slot.pdata->tracked_obj_instance = 0;
            if (player->slot.mirror_b != 0) {
                MkHdr* shadow = (MkHdr*)player->slot.mirror_b;
                if (shadow->instance != 0) {
                    ((void (*)(MkHdr*))shadow->vtbl->destroy)(shadow);
                }
                player->slot.mirror_b = 0;
                TearDownShadow((ShadowObject*)player->slot.pdata);
                destroy_mkprocs_pid(shadow_pid);
            }
        }

        if (player->slot.pdata->plyr_num == 0) {
            destroy_mkprocs_pid(0xC028);
        } else {
            destroy_mkprocs_pid(0xC029);
        }

        pdata = player->slot.pdata;
        object = pdata->sidekick_obj;
        if (object != 0) {
            if (object->hdr.instance != pdata->sidekick_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            if (object->hdr.instance != 0) {
                ((void (*)(MkHdr*))object->hdr.vtbl->destroy)((MkHdr*)object);
            }
            player->slot.pdata->sidekick_obj = 0;
            player->slot.pdata->sidekick_instance = 0;
        }
        sidekick_player = player->slot.pdata->plyr_num;
        if (sidekick_player == 0) {
            destroy_mkprocs_pid(0xC028);
        } else {
            destroy_mkprocs_pid(0xC029);
        }
        pdata = player->slot.pdata;
        if (pdata->instance != 0) {
            ((void (*)(MkHdr*))((MkHdr*)pdata)->vtbl->destroy)((MkHdr*)pdata);
        }
        if (sidekick_player == 0) {
            destroy_mkprocs_pid(0xC028);
        } else {
            destroy_mkprocs_pid(0xC029);
        }
        player->slot.pdata = 0;
        destroy_mkprocs_pid(((MkProc*)player->idle_proc)->pid);
        destroy_mkprocs_pid(((MkProc*)player->field_68)->pid);
        destroy_mkprocs_pid(0x1003);
        kill_fstyle_signs_for_plyr(player);
        term_player_collision(player);
        if (player_index == 0) {
            unload_script(3);
            unload_script(4);
            unload_script(5);
            unload_script(6);
        } else {
            unload_script(7);
            unload_script(8);
            unload_script(9);
            unload_script(10);
        }
    }
}

#define BIND_PLAYER_ANIM(make_, proc_, instance_)                       \
    do {                                                                \
        MkProc* created = make_(0x5002, p_anim_idle, &animation);       \
        if (created != 0) {                                             \
            animation->obj = object;                                    \
            animation->obj_instance = object->hdr.instance;             \
            animation->owner = pdata;                                   \
            animation->owner_instance = pdata->instance;                \
            (proc_) = created;                                          \
            (instance_) = created->instance;                            \
        }                                                               \
    } while (0)

#define BIND_GORO_ANIM(index_)                                          \
    do {                                                                \
        MkProc* created = create_mkproc_hand_anim(                       \
            0x5002, p_anim_idle, &animation);                            \
        if (created != 0) {                                             \
            animation->obj = object;                                    \
            animation->obj_instance = object->hdr.instance;             \
            animation->owner = pdata;                                   \
            animation->owner_instance = pdata->instance;                \
            pdata->goro_hand_anim[index_].proc = created;               \
            pdata->goro_hand_anim[index_].instance = created->instance; \
            animation->bone_remap = goro_hand_to_hand2_remapping;       \
        }                                                               \
    } while (0)

static void setup_plyr_anims(PlyrPdata* pdata) {
    AnimPdata* animation;
    MkObj* object;
    int moveset_base;
    int index;
    int mode;
    int load_signs;

    object = pdata->tracked_obj;
    if (object != 0) {
        if (object->hdr.instance != pdata->tracked_obj_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object == 0) {
        return;
    }

    BIND_PLAYER_ANIM(
        create_mkproc_anim, pdata->anim_proc, pdata->anim_proc_instance);
    BIND_PLAYER_ANIM(
        create_mkproc_hand_anim, pdata->left_hand_anim_proc,
        pdata->left_hand_anim_instance);
    BIND_PLAYER_ANIM(
        create_mkproc_hand_anim, pdata->field_7C.proc,
        pdata->field_7C.instance);
    BIND_PLAYER_ANIM(
        create_mkproc_hand_anim, pdata->right_hand_anim_proc,
        pdata->right_hand_anim_instance);
    BIND_PLAYER_ANIM(
        create_mkproc_hand_anim, pdata->field_84.proc,
        pdata->field_84.instance);

    if (pdata->character_id == 0x1E) {
        BIND_GORO_ANIM(0);
        BIND_GORO_ANIM(1);
        BIND_GORO_ANIM(2);
        BIND_GORO_ANIM(3);
    }

    moveset_base = pdata == g_game_info.plyr0.slot.pdata ? 0 : 3;
    for (index = 0; index < 3; index++) {
        pdata->weapon_styles[index] =
            (PlyrWeaponStyle*)&global_movesets[moveset_base + index];
    }
    load_player_style_scripts(pdata);
    advance_active_moveset(pdata);

    mode = (int)mode_of_play;
    load_signs = 1;
    if ((mode == 0 || mode == 10) &&
        g_game_info.feature_flags.bits.powerbars_locked) {
        load_signs = 0;
    } else if (mode == 6) {
        load_signs = 0;
    }
    if (load_signs != 0) {
        load_player_fstyle_signs(pdata);
    }
    load_player_anim_files(pdata);

    if (pdata->face_animations[0] != 0) {
        BIND_PLAYER_ANIM(
            create_mkproc_face_anim, pdata->face_anim_proc,
            pdata->face_anim_proc_instance);
        BIND_PLAYER_ANIM(
            create_mkproc_face_anim, pdata->field_8C.proc,
            pdata->field_8C.instance);
    }
}

#undef BIND_GORO_ANIM
#undef BIND_PLAYER_ANIM

static void load_player_anim_files(PlyrPdata* pdata) {
    int slot;
    const char* section_name;
    MkFileInfo* section;
    int index;

    slot = 0x4000C;
    if (pdata == g_game_info.plyr0.slot.pdata) {
        slot = 0x3000C;
    }
    unload_section_slot(slot);
    if (pdata->plyr_info->flags_14_bits.alternate_costume) {
        section_name =
            pdata->runtime_data->alternate_animation_section;
    } else {
        section_name = pdata->runtime_data->animation_section;
    }
    section = find_section_by_name(section_name);
    if (section != 0 && section->name != 0) {
        add_anim_section_async(
            slot, section, pdata->animation_data, 0, 0);
    }
    if ((int)mode_of_play == 6) {
        PlyrWeaponStyle* style = pdata->weapon_styles[0];

        section = find_section_by_name(
            pdata->runtime_data->chess_animation_section);
        add_anim_section_async(
            slot, section, &style->animation_data, 0, 0);
    } else {
        for (index = 0; index < 3; index++) {
            PlyrWeaponStyle* style = pdata->weapon_styles[index];

            if (style->definition != 0) {
                section = find_section_by_name(
                    style->definition->animation_section_name);
                style->animation_header =
                    style->definition->animation_header;
                if (section != 0) {
                    add_anim_section_async(
                        slot, section, &style->animation_data, 1, 0);
                }
            }
        }
    }
    wait_for_slot_load(slot);
}

typedef union PlyrProcCreateFlags {
    int word;
    struct {
        unsigned char one_shot : 1;
        unsigned char defer_run : 1;
        unsigned char pad_bits : 6;
        unsigned char pad_bytes[3];
    } bits;
} PlyrProcCreateFlags;

extern MkFlippedBoneMap flipped_human_bones;

static inline MkObj* loaded_model_as_mkobj(void* model) {
    MkObj* object;

    if (model != 0) {
        object = (MkObj*)model;
    } else {
        object = 0;
    }
    return object;
}

void create_player(int player_index, PlyrInfo* player) {
    PlyrProcCreateFlags flags;
    PlyrProcCreateFlags flags_arg;
    MkProc* player_proc;
    MkProc* aux_proc;
    PlyrPdata* pdata;
    FighterRuntimeData* runtime;
    MkObj* object;
    AniTextureControl* texture;
    const char* art_section;
    const char* face_texture;
    const char* const* effect_banks;
    unsigned int face_art_id;
    int player_pid = 0;
    int aux_pid = 0;
    int art_slot;
    int loaded;
    int index;
    LoadBgndCtx effect_context;

    player->field_04 = player_index;
    if (player->player_index == 0x2C) {
        do {
            player->player_index = (unsigned short)randu0(0x2C);
            if ((unsigned short)randu0(0x19) == 0) {
                player->flags_14_bits.alternate_costume = 1;
            } else {
                player->flags_14_bits.alternate_costume = 0;
            }
        } while (is_char_locked(
                     player->player_index,
                     player->flags_14_bits.alternate_costume) != 0);
    }
    init_shadow_system();
    if (player_index == 0) {
        player_pid = 0x1001;
        aux_pid = 0x100A;
    } else if (player_index == 1) {
        player_pid = 0x1002;
        aux_pid = 0x100B;
    }

    flags.word = 0;
    flags.bits.defer_run = 1;
    flags_arg = flags;
    player_proc = get_mkproc_bigstack(&flags_arg.word);
    if (player_proc != 0) {
        player->slot.pdata = get_mkpdata_plyr();
        player_proc = create_mkproc(
            8, player_proc, player_pid, player_sleep_forever,
            (MkHdr*)player->slot.pdata);
        set_process_as_scriptable(player_proc);
    }
    player->idle_proc = player_proc;

    player->slot.pdata->own_player_proc = (MkProc*)player->idle_proc;
    player->slot.pdata->own_player_proc_instance =
        ((MkProc*)player->idle_proc)->instance;
    player->slot.pdata->plyr_info = player;
    player->slot.pdata->mirror_slots = 0;

    flags.word = 0;
    flags.bits.defer_run = 1;
    flags_arg = flags;
    aux_proc = get_mkproc_nostack(&flags_arg.word);
    if (aux_proc != 0) {
        aux_proc = create_mkproc(
            8, aux_proc, aux_pid, player_sleep_forever,
            (MkHdr*)player->slot.pdata);
    }
    player->field_68 = aux_proc;
    player->slot.pdata->aux_player_proc = (MkProc*)player->field_68;
    player->slot.pdata->aux_player_proc_instance =
        ((MkProc*)player->field_68)->instance;
    player->slot.pdata->saved_anim_script_word = 0;
    player->slot.pdata->plyr_num = player_index;
    player->field_04 = player_index;
    plyr_pdata = 0;
    player->slot.pdata->character_id = player->player_index;

    if (player->flags_14_bits.alternate_costume) {
        load_ssf(global_player_data[player->player_index].alternate_model_files);
    } else {
        load_ssf(global_player_data[player->player_index].model_files);
    }

    pdata = player->slot.pdata;
    if (pdata->character_id < 0 || pdata->character_id >= 0x2C) {
        loaded = 0;
    } else {
        pdata->cmo = cmdscript_loadfile_by_name(
            pdata->plyr_num == 0 ? 3 : 7,
            global_player_data[pdata->character_id].model_script);
        if (pdata->cmo->table_count == 0) {
            loaded = 0;
        } else {
            pdata->runtime_data =
                (FighterRuntimeData*)get_data_table(
                    pdata->cmo, pdata->cmo->table_count);
            generate_ai_table_player((FighterMirror*)pdata);
            loaded = 1;
        }
    }
    if (loaded == 0) {
        delete_player(((MkProc*)player->idle_proc)->pid);
        return;
    }

    runtime = pdata->runtime_data;
    if (pdata->plyr_info->flags_14_bits.alternate_costume) {
        art_section = pdata->plyr_info->flags_14_bits.alternate_palette
            ? runtime->alternate_palette_art_section
            : runtime->alternate_art_section;
    } else {
        art_section = pdata->plyr_info->flags_14_bits.alternate_palette
            ? runtime->palette_art_section
            : runtime->primary_art_section;
    }

    object = 0;
    if (player_pid == 0x1001) {
        load_art_section_by_name(0x3000A, art_section);
        if (runtime->shared_art_section != 0) {
            load_art_section_by_name(0x3000B, runtime->shared_art_section);
        }
        object = (MkObj*)load_named_model_from_slot(
            0x3000A, "COSTUME", player_pid, 1);
    } else if (player_pid == 0x1002) {
        load_art_section_by_name(0x4000A, art_section);
        if (runtime->shared_art_section != 0) {
            load_art_section_by_name(0x4000B, runtime->shared_art_section);
        }
        object = (MkObj*)load_named_model_from_slot(
            0x4000A, "COSTUME", player_pid, 1);
    }
    object = loaded_model_as_mkobj(object);
    player->slot.mirror_a = object;
    if (player->slot.mirror_a == 0) {
        return;
    }
    object = player->slot.mirror_a;

    player->slot.pdata->tracked_obj = player->slot.mirror_a;
    player->slot.pdata->tracked_obj_instance =
        player->slot.mirror_a->hdr.instance;
    if (has_sidekick(pdata) == 0) {
        if (player->flags_14_bits.alternate_costume) {
            face_texture = player->flags_14_bits.alternate_palette
                ? runtime->alternate_palette_face_texture
                : runtime->alternate_face_texture;
        } else {
            face_texture = player->flags_14_bits.alternate_palette
                ? runtime->palette_face_texture
                : runtime->primary_face_texture;
        }
        if (face_texture != 0) {
            if (player->slot.mirror_a->oid == 0x1001) {
                art_slot = 0x3000A;
            } else if (player->slot.mirror_a->oid == 0x1002) {
                art_slot = 0x4000A;
            } else {
                art_slot = -1;
            }
            face_art_id = get_artid_of_named_item_in_slot(
                art_slot, "FACEDAM", 0);
            if (face_art_id != 0) {
                texture = append_wiff_to_clump_material(
                    art_slot, (char*)face_art_id,
                    (RpClump*)player->slot.mirror_a->clump,
                    (char*)face_texture);
                if (texture != 0) {
                    index = get_ani_texture_numframes(texture);
                    if (index != 4 && index != 5) {
                        if (texture->instance != 0) {
                            ((void (*)(MkHdr*))texture->vtbl->destroy)(
                                (MkHdr*)texture);
                        }
                    } else {
                        insert_ani_texture_control_item(
                            texture, &pdata->facial_texture);
                    }
                }
            }
        }
    }

    player->slot.mirror_a->flags_08_bits.gravity_enabled = 1;
    player->slot.mirror_a->flags_08_bits.rotation_enabled = 1;
    player->slot.pdata->tracked_obj = player->slot.mirror_a;
    player->slot.pdata->tracked_obj_instance =
        player->slot.mirror_a->hdr.instance;
    player->slot.mirror_a->flags_09_bits.bit4 = 1;
    player->slot.mirror_a->flags_09_bits.launched = 1;
    player->slot.mirror_a->flags_09_bits.tightrope_restricted = 1;
    player->slot.mirror_a->flags_09_bits.face_opponent = 0;
    player->slot.mirror_a->hide_flag_bits.weapon_effect = 1;
    player->slot.mirror_a->flipped_bone_map = &flipped_human_bones;
    player->field_0C = 1.0f;

    setup_plyr_anims(player->slot.pdata);
    player->slot.mirror_a->light_flags = 0x100C;
    obj_create_sobjs(player->slot.mirror_a);
    specskin_initialize_clump(player->slot.mirror_a->clump);
    if (player->flags_14_bits.alternate_costume) {
        build_bones_tbl(player->slot.mirror_a,
                        player->slot.pdata->runtime_data
                            ->alternate_bone_tags);
        plyr_obj_load_bld_data(
            (FighterMirror*)player->slot.pdata,
            player->slot.pdata->large_blood_spawn_state,
            player->slot.mirror_a, "ALT_BLOODPATH");
    } else {
        build_bones_tbl(player->slot.mirror_a,
                        player->slot.pdata->runtime_data
                            ->primary_bone_tags);
        plyr_obj_load_bld_data(
            (FighterMirror*)player->slot.pdata,
            player->slot.pdata->large_blood_spawn_state,
            player->slot.mirror_a, "BLOODPATH");
    }
    if ((int)mode_of_play == 6) {
        puzzle_fighter_scale(player->slot.mirror_a, 1.0f);
    }
    insert_fgnd_mkobj(player->slot.mirror_a);
    start_cloth_bones(player->slot.mirror_a);
    limb_sever_hide_z_meat_chunks_all(player->slot.mirror_a);
    player->slot.mirror_a->pos.value.x = -1.0f;
    player->slot.mirror_a->pos.value.y = 1.05f;
    player->slot.mirror_a->pos.value.z = 0.0f;
    player->slot.mirror_a->flags_09_bits.launched = 1;
    ground_me((MkHdr*)player->slot.mirror_a);
    player->slot.mirror_a->flags_09_bits.launched = 0;
    player->slot.mirror_a->ang.y = 1.5707964f;
    init_player_collision(player);
    update_mkobj(player->slot.mirror_a);
    if (player_index == 1) {
        player->slot.mirror_a->hide_flag_bits.bit6 = 1;
    }

    ((MkProc*)player->idle_proc)->pre_destroy = pw_plyr;
    ((MkProc*)player->idle_proc)->destroy_cb = ps_plyr;
    if (player->field_68 != 0) {
        ((MkProc*)player->field_68)->pre_destroy = pw_plyr;
        ((MkProc*)player->field_68)->destroy_cb = ps_plyr;
        xfer_proc((MkProc*)player->field_68, p_plyr_aux);
    }
    player->slot.pdata->controller_port = player->pad_index;

    effect_banks = player->flags_14_bits.alternate_costume
        ? player->slot.pdata->runtime_data->alternate_effect_banks
        : player->slot.pdata->runtime_data->effect_banks;
    if (effect_banks != 0) {
        effect_context.art_id =
            player->field_04 == 0 ? 0x3000B : 0x4000B;
        effect_context.bgnd_obj = player->slot.mirror_a;
        effect_context.field_08 = player;
        active_cmdscript->mko = player->slot.pdata->cmo;
        active_cmdscript->mko->load_ctx = &effect_context;
        for (index = 0; effect_banks[index] != 0; index++) {
            load_effect_bank((char*)effect_banks[index]);
        }
        active_cmdscript->mko->load_ctx = 0;
    }

    create_sidekick(player_index, player);
    if (!g_game_info.pause_flag_bits.shared_hand_anims_loaded &&
        (int)mode_of_play != 6) {
        g_game_info.pause_flag_bits.shared_hand_anims_loaded = 1;
        load_shared_and_hand_anims();
    }
    load_ssf(misc_anims_list_file_table);
    reactions_cmo = cmdscript_loadfile(0xE, &cmo_script_reactions);
    start_constrain_proc();
    init_debug_variables();
}

static void create_sidekick(int player_index, PlyrInfo* player) {
    PlyrPdata* pdata = player->slot.pdata;
    PlyrPdata* face_pdata;
    FighterRuntimeData* runtime;
    FighterRuntimeData* face_runtime;
    MkObj* object = 0;
    AniTextureControl* texture;
    AnimPdata* animation;
    MkProc* animation_proc;
    const char* art_section;
    const char* face_texture;
    unsigned int face_art_id;
    int player_pid;
    int art_slot;
    int frame_count;

    if (has_sidekick(pdata) == 0) {
        return;
    }
    select_fighter_voice_in_bank(player_index, 0);
    if ((int)mode_of_play == 6) {
        return;
    }

    pdata->sidekick_available = 1;
    player_pid = ((MkProc*)player->idle_proc)->pid;
    runtime = pdata->runtime_data;
    if (pdata->plyr_info->flags_14_bits.alternate_costume) {
        art_section = pdata->plyr_info->flags_14_bits.alternate_palette
            ? runtime->alternate_palette_art_section
            : runtime->alternate_art_section;
    } else {
        art_section = pdata->plyr_info->flags_14_bits.alternate_palette
            ? runtime->palette_art_section
            : runtime->primary_art_section;
    }

    if (player_pid == 0x1001) {
        load_art_section_by_name(0x3000A, art_section);
        if (runtime->shared_art_section != 0) {
            load_art_section_by_name(0x3000B, runtime->shared_art_section);
        }
        object = (MkObj*)load_named_model_from_slot(
            0x3000A, "COSTUME", player_pid, 1);
    } else if (player_pid == 0x1002) {
        load_art_section_by_name(0x4000A, art_section);
        if (runtime->shared_art_section != 0) {
            load_art_section_by_name(0x4000B, runtime->shared_art_section);
        }
        object = (MkObj*)load_named_model_from_slot(
            0x4000A, "COSTUME", player_pid, 1);
    }
    object = loaded_model_as_mkobj(object);
    if (object == 0) {
        return;
    }

    pdata->sidekick_obj = object;
    pdata->sidekick_instance = object->hdr.instance;
    face_pdata = player->slot.pdata;
    if (has_sidekick(pdata) == 0) {
        face_runtime = face_pdata->runtime_data;
        if (player->flags_14_bits.alternate_costume) {
            face_texture = player->flags_14_bits.alternate_palette
                ? face_runtime->alternate_palette_face_texture
                : face_runtime->alternate_face_texture;
        } else {
            face_texture = player->flags_14_bits.alternate_palette
                ? face_runtime->palette_face_texture
                : face_runtime->primary_face_texture;
        }
        if (face_texture != 0) {
            if (object->oid == 0x1001) {
                art_slot = 0x3000A;
            } else if (object->oid == 0x1002) {
                art_slot = 0x4000A;
            } else {
                art_slot = -1;
            }
            face_art_id = get_artid_of_named_item_in_slot(
                art_slot, "FACEDAM", 0);
            if (face_art_id != 0) {
                texture = append_wiff_to_clump_material(
                    art_slot, (char*)face_art_id,
                    (RpClump*)object->clump, (char*)face_texture);
                if (texture != 0) {
                    frame_count = get_ani_texture_numframes(texture);
                    if (frame_count == 4 || frame_count == 5) {
                        insert_ani_texture_control_item(
                            texture, &face_pdata->facial_texture);
                    } else if (texture->instance != 0) {
                        ((void (*)(MkHdr*))texture->vtbl->destroy)(
                            (MkHdr*)texture);
                    }
                }
            }
        }
    }

    object->flags_08_bits.gravity_enabled = 1;
    object->flags_08_bits.rotation_enabled = 1;
    object->flags_09_bits.bit4 = 1;
    object->flags_09_bits.launched = 1;
    object->flags_09_bits.tightrope_restricted = 1;
    object->flags_09_bits.face_opponent = 0;
    object->hide_flag_bits.weapon_effect = 1;
    object->flipped_bone_map = &flipped_human_bones;
    object->light_flags = 0x100C;
    obj_create_sobjs(object);
    specskin_initialize_clump(object->clump);
    if (player->flags_14_bits.alternate_costume) {
        build_bones_tbl(
            object, player->slot.pdata->runtime_data->primary_bone_tags);
    } else {
        build_bones_tbl(
            object, player->slot.pdata->runtime_data->primary_bone_tags);
    }
    insert_fgnd_mkobj(object);
    start_cloth_bones(object);
    limb_sever_hide_z_meat_chunks_all(object);
    object->pos.value.x = -1.0f;
    object->pos.value.y = 1.05f;
    object->pos.value.z = 0.0f;
    if (player_index == 1) {
        object->pos.value.x = 1.0f;
    }
    object->flags_09_bits.launched = 1;
    ground_me((MkHdr*)object);
    object->flags_09_bits.launched = 0;
    object->ang.y = 1.5707964f;
    update_mkobj(object);

    if (object != 0) {
        animation_proc =
            create_mkproc_anim(0x5002, p_anim_idle, &animation);
        if (animation_proc != 0) {
            animation->obj = object;
            animation->obj_instance = object->hdr.instance;
            animation->owner = 0;
            animation->owner_instance = 0;
            pdata->sidekick_anim_proc = animation_proc;
            pdata->sidekick_anim_instance = animation_proc->instance;
        }
        animation->step = 1.0f;
        transition_to_anim_script(
            animation, pdata->fighter_definition->duck_exit_animation,
            0, 0.1f);
        xfer_proc(animation_proc, (MkProcEntryFn)p_animate);
        object->flags_09_bits.bit6 = 1;
        object->flags_09_bits.launched = 1;
        update_bone_hierarchy(
            object != 0 ? as_mkhdr(&object->hdr) : 0);
        ground_me(object != 0 ? as_mkhdr(&object->hdr) : 0);
        hide_obj(object);
    }
}

typedef union PlyrFloatBits {
    float value;
    unsigned int bits;
} PlyrFloatBits;

static inline float plyr_inverse_sqrt(float squared) {
    PlyrFloatBits estimate_bits;
    float estimate;
    float product;
    float correction;

    if (squared <= 0.0f) {
        return 0.0f;
    }
    estimate_bits.value = squared;
    estimate_bits.bits = 0x5F375A00U - (estimate_bits.bits >> 1);
    estimate = estimate_bits.value;
    product = estimate * (squared * estimate);
    correction = 3.0f - product;
    return 0.0625f * estimate * correction *
           -(correction * (product * correction) - 12.0f);
}

float active_sidekick_swap_from_behind(PlyrPdata* pdata) {
    MkObj* sidekick = pdata->sidekick_obj;
    MkObj* player;
    MkObj* opponent;
    float opponent_x;
    float opponent_z;
    float dx;
    float dz;
    float inverse_distance;

    if (sidekick != 0) {
        if (sidekick->hdr.instance != pdata->sidekick_instance) {
            sidekick = 0;
        }
    } else {
        sidekick = 0;
    }
    if (pdata->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    player = plyr_obj;
    opponent = his_obj;
    opponent_z = opponent->pos.value.z;
    dz = player->pos.value.z - opponent_z;
    opponent_x = opponent->pos.value.x;
    dx = player->pos.value.x - opponent_x;
    inverse_distance = plyr_inverse_sqrt(dx * dx + dz * dz);
    dx *= inverse_distance;
    dz *= inverse_distance;
    dx *= -2.0f;
    dz *= -2.0f;
    dx += opponent_x;
    dz += opponent_z;
    active_sidekick_swap(pdata, 1);
    hide_obj(sidekick);
    plyr_obj->pos.value.x = dx;
    plyr_obj->pos.value.y = g_game_info.field_34;
    plyr_obj->pos.value.z = dz;
    bgnd_clear_danger_zone_callback(plyr_pdata);
    if (get_current_bgnd() != 2) {
        set_constrain_last_pos_pdata(&his_obj->pos.value);
    }
    clear_my_face_opponent_flag();
    return 0.0f;
}

float active_sidekick_swap_from_sky(PlyrPdata* pdata) {
    MkObj* player;
    MkObj* opponent;
    float opponent_x;
    float opponent_z;
    float dx;
    float dz;
    float inverse_distance;

    if (pdata->plyr_num == 0) {
        destroy_mkprocs_pid(0xC028);
    } else {
        destroy_mkprocs_pid(0xC029);
    }
    player = plyr_obj;
    opponent = his_obj;
    opponent_z = opponent->pos.value.z;
    dz = player->pos.value.z - opponent_z;
    opponent_x = opponent->pos.value.x;
    dx = player->pos.value.x - opponent_x;
    inverse_distance = plyr_inverse_sqrt(dx * dx + dz * dz);
    dx *= inverse_distance;
    dz *= inverse_distance;
    dx *= 3.0f;
    dz *= 3.0f;
    dx += opponent_x;
    dz += opponent_z;
    active_sidekick_swap(pdata, 1);
    plyr_obj->pos.value.x = dx;
    plyr_obj->pos.value.z = dz;
    plyr_obj->pos.value.y = g_game_info.field_34 + 4.0f;
    return 0.0f;
}

float active_sidekick_swap_change_style(PlyrPdata* pdata) {
    MkProc* process = pdata->own_player_proc;
    CmdScript* script;

    if (process != 0) {
        if (process->instance ==
            (int)pdata->own_player_proc_instance) {
            /* Keep the live process. */
        } else {
            process = 0;
        }
    } else {
        process = 0;
    }
    active_sidekick_swap(pdata, 2);
    script = get_cmdscript_for_proc(process);
    script->unk28 = 0x7C;
    ((PlyrProcVtable*)aproc->vtbl)
        ->jump_sleep(r_call_script_function, 0.0f);
    return 0.0f;
}

float active_sidekick_swap(PlyrPdata* pdata, int mode) {
    MkObj* sidekick;
    PlyrInfo* player;
    MkProc* process;
    AnimPdata* sidekick_animation;
    AnimPdata* player_animation;
    PlyrSidekickProcPdata* process_data;
    CmdScript* saved_interpreter;
    AniData* saved_animation;
    unsigned int saved_animation_flags;
    int saved_hide_bit;
    float saved_frame;
    float saved_step;
    float saved_weight;
    float pos_x;
    float pos_y;
    float pos_z;
    float ang_x;
    float ang_y;
    float ang_z;

    sidekick = pdata->sidekick_obj;
    player = pdata->plyr_info;
    if (sidekick != 0) {
        if (sidekick->hdr.instance == pdata->sidekick_instance) {
            /* Keep the live object. */
        } else {
            sidekick = 0;
        }
    } else {
        sidekick = 0;
    }
    process = pdata->sidekick_anim_proc;
    if (process != 0) {
        if (process->instance ==
            (int)pdata->sidekick_anim_instance) {
            /* Keep the live process. */
        } else {
            process = 0;
        }
    } else {
        process = 0;
    }
    sidekick_animation = (AnimPdata*)pdata_of_proc(process);
    process = pdata->anim_proc;
    if (process != 0) {
        if (process->instance == (int)pdata->anim_proc_instance) {
            /* Keep the live process. */
        } else {
            process = 0;
        }
    } else {
        process = 0;
    }
    player_animation = (AnimPdata*)pdata_of_proc(process);

    unhide_obj(sidekick);
    sidekick->pos_vel.z = 0.0f;
    sidekick->pos_vel.y = 0.0f;
    sidekick->pos_vel.x = 0.0f;
    sidekick->gravity = 0.0f;
    sidekick->pos.value.y = g_game_info.field_34;
    if (plyr_pdata->status_data->reaction_cleanup != 0) {
        saved_interpreter = active_cmdscript;
        active_cmdscript = &global_script_interpreter;
        cmdscript_setup_execution(
            plyr_pdata->cmo,
            plyr_pdata->status_data->reaction_cleanup);
        cmdscript_execute(plyr_pdata->cmo);
        active_cmdscript = saved_interpreter;
    }

    saved_animation = sidekick_animation->animation;
    saved_hide_bit = sidekick->hide_flag_bits.bit6;
    saved_step = sidekick_animation->step;
    saved_weight = sidekick_animation->weight;
    saved_frame = sidekick_animation->frame;
    saved_animation_flags = sidekick_animation->flags;
    player->slot.pdata->sidekick_active =
        pdata->sidekick_active == 0;
    tag_team_activate_player(
        player->slot.mirror_a, pdata->sidekick_active == 0);
    tag_team_activate_player(sidekick, pdata->sidekick_active);

    pos_x = sidekick->pos.value.x;
    pos_y = sidekick->pos.value.y;
    pos_z = sidekick->pos.value.z;
    ang_x = sidekick->ang.x;
    ang_y = sidekick->ang.y;
    ang_z = sidekick->ang.z;
    sidekick->pos.value.x = player->slot.mirror_a->pos.value.x;
    sidekick->pos.value.y = player->slot.mirror_a->pos.value.y;
    sidekick->pos.value.z = player->slot.mirror_a->pos.value.z;
    sidekick->ang.x = player->slot.mirror_a->ang.x;
    sidekick->ang.y = player->slot.mirror_a->ang.y;
    sidekick->ang.z = player->slot.mirror_a->ang.z;
    player->slot.mirror_a->pos.value.x = pos_x;
    player->slot.mirror_a->pos.value.y = pos_y;
    player->slot.mirror_a->pos.value.z = pos_z;
    player->slot.mirror_a->ang.x = ang_x;
    player->slot.mirror_a->ang.y = ang_y;
    player->slot.mirror_a->ang.z = ang_z;
    set_root_and_obj_movement_weights(
        0.0f, player_animation->weight, sidekick_animation);
    set_root_and_obj_movement_weights(
        0.0f, saved_weight, player_animation);
    update_mkobj(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    update_mkobj(
        player->slot.mirror_a != 0
            ? as_mkhdr(&player->slot.mirror_a->hdr) : 0);
    sidekick->ground_colls_y = player->slot.mirror_a->ground_colls_y;
    sidekick->hide_flag_bits.bit6 =
        player->slot.mirror_a->hide_flag_bits.bit6;

    if ((player_animation->flags & 8) != 0) {
        set_plyr_anim_script_frame(
            sidekick_animation, player_animation->animation,
            0xB, player_animation->frame);
    } else {
        set_plyr_anim_script_frame(
            sidekick_animation, player_animation->animation,
            3, player_animation->frame);
    }
    sidekick_animation->step = player_animation->step;
    sidekick->flags_09_bits.bit6 = 0;
    sidekick->flags_09_bits.launched = 0;
    if (pdata->plyr_num == 0) {
        process = _create_mkproc_generic_tinystack(
            0xC028, 8, p_plyr_sidekick, sizeof(*process_data),
            (MkHdr**)&process_data);
    } else {
        process = _create_mkproc_generic_tinystack(
            0xC029, 8, p_plyr_sidekick, sizeof(*process_data),
            (MkHdr**)&process_data);
    }
    if (process == 0 || process_data == 0) {
        hide_obj(sidekick);
    } else {
        process_data->player = plyr_pdata;
    }

    player->slot.mirror_a->hide_flag_bits.bit6 = saved_hide_bit;
    if ((saved_animation_flags & 8) != 0) {
        set_plyr_anim_script_frame(
            player_animation, saved_animation, 8, saved_frame);
    } else {
        set_plyr_anim_script_frame(
            player_animation, saved_animation, 0, saved_frame);
    }
    player_animation->step = saved_step;
    player->slot.mirror_a->flags_09_bits.bit6 = 1;
    player->slot.mirror_a->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        player->slot.mirror_a != 0
            ? as_mkhdr(&player->slot.mirror_a->hdr) : 0);
    ground_me(
        player->slot.mirror_a != 0
            ? as_mkhdr(&player->slot.mirror_a->hdr) : 0);
    select_fighter_voice_in_bank(
        pdata->plyr_num, pdata->sidekick_active);
    if (mode != 0) {
        snd_req(0xDC1);
        if (is_local_plyr() != 0) {
            advance_active_moveset(plyr_pdata);
        }
    }
    return 0.0f;
}

typedef struct SidekickActionView {
    char pad00[0x318];
    AniData* charge_exit_animation;
    char pad31C[0x28];
    AniData* common_exit_animation;
    char pad348[0x130];
    ScriptSlot* cmo;
} SidekickActionView;

static inline void plyr_sleep(float ticks) {
    _mkproc_sleep_ticks = ticks;
    ((PlyrProcVtable*)aproc->vtbl)->sleep();
}

float sidekick_cool_vanish(PlyrPdata* pdata) {
    MkObj* sidekick;
    MkProc* anim_proc;
    AnimPdata* animation;
    SidekickActionView* actions = (SidekickActionView*)pdata;
    int function;

    anim_proc = pdata->sidekick_anim_proc;
    if (anim_proc != 0) {
        if (anim_proc->instance != (int)pdata->sidekick_anim_instance) {
            anim_proc = 0;
        }
    } else {
        anim_proc = 0;
    }
    animation = (AnimPdata*)pdata_of_proc(anim_proc);
    sidekick = pdata->sidekick_obj;
    if (sidekick != 0) {
        if (sidekick->hdr.instance != pdata->sidekick_instance) {
            sidekick = 0;
        }
    } else {
        sidekick = 0;
    }
    if (sidekick == 0) {
        return 0.0f;
    }
    if (pdata->sidekick_active == 0) {
        transition_to_anim_script(
            animation, actions->charge_exit_animation, 3, 0.25f);
        animation->step = 1.2f;
        sidekick->flags_09_bits.bit6 = 1;
        while (animation->frame < 5.0f) {
            plyr_sleep(1.0f);
        }
        sidekick->flags_09_bits.bit6 = 0;
        sidekick->flags_09_bits.launched = 0;
        while (animation->frame < 26.0f) {
            plyr_sleep(1.0f);
        }
        snd_req(0x32C);
        obj_set_gravity(sidekick, -0.01f);
        if (sidekick->hide_flag_bits.hidden == 0) {
            function = get_script_function_by_name(
                actions->cmo, "sidekick_swap_pfx");
            plyr_start_script_in_slot(actions->cmo, 0xC025, function);
        }
        plyr_sleep(30.0f);
        obj_set_gravity(sidekick, 0.0f);
        sidekick->flags_08_bits.moving = 0;
    } else {
        transition_to_anim_script(
            animation, actions->common_exit_animation, 3, 0.1f);
        animation->step = 2.0f;
        sidekick->flags_09_bits.bit6 = 1;
        while (animation->frame < 5.0f) {
            plyr_sleep(1.0f);
        }
        sidekick->flags_09_bits.bit6 = 0;
        sidekick->flags_09_bits.launched = 0;
        snd_req_delay(0x33B, 0x10, 0);
        if (sidekick->hide_flag_bits.hidden == 0) {
            function = get_script_function_by_name(
                actions->cmo, "sidekick_swap_pfx");
            plyr_start_script_in_slot(actions->cmo, 0xC025, function);
        }
        plyr_sleep(30.0f);
    }
    hide_obj(sidekick);
    return 0.0f;
}

static float p_plyr_sidekick(void) {
    PlyrSidekickProcPdata* pdata = (PlyrSidekickProcPdata*)apdata;
    MkProc* anim_proc;
    AnimPdata* animation;
    MkObj* sidekick;

    anim_proc = pdata->player->sidekick_anim_proc;
    if (anim_proc != 0) {
        if (anim_proc->instance !=
            (int)pdata->player->sidekick_anim_instance) {
            anim_proc = 0;
        }
    } else {
        anim_proc = 0;
    }
    animation = (AnimPdata*)pdata_of_proc(anim_proc);
    sidekick = pdata->player->sidekick_obj;
    if (sidekick != 0) {
        if (sidekick->hdr.instance != pdata->player->sidekick_instance) {
            sidekick = 0;
        }
    } else {
        sidekick = 0;
    }
    if (sidekick == 0) {
        return -1.0f;
    }
    animpdata_ani_to_frame_x(animation, 12.0f);
    sidekick->flags_09_bits.bit6 = 0;
    sidekick->flags_09_bits.launched = 0;
    animpdata_ani_to_frame_x(animation, 17.0f);
    random_foot(1);
    sidekick->flags_09_bits.bit6 = 1;
    sidekick->flags_09_bits.launched = 1;
    update_bone_hierarchy(
        sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    ground_me(sidekick != 0 ? as_mkhdr(&sidekick->hdr) : 0);
    sidekick->hide_flag_bits.bit6 ^= 1;
    animation->flags ^= 8;
    transition_to_anim_script(
        animation,
        pdata->player->fighter_definition->duck_exit_animation,
        0x20, 0.1f);
    animpdata_ani_loop_more_frames(animation, 10.0f);
    sidekick_cool_vanish(pdata->player);
    return -1.0f;
}

int is_sidekick_active(PlyrInfo* player) {
    PlyrPdata* pdata;

    pdata = (PlyrPdata*)player->slot.fighter;
    if (pdata->sidekick_available != 0) {
        return pdata->sidekick_active;
    }
    return 0;
}

void start_plyrs(void) {
    create_player(0, &g_game_info.plyr0);
    setup_sound_banks(0xC);
    create_player(1, &g_game_info.plyr1);
    setup_sound_banks(0xD);
    xfer_proc((MkProc*)g_game_info.plyr0.idle_proc, p_plyr_start);
    xfer_proc((MkProc*)g_game_info.plyr1.idle_proc, p_plyr_start);
}

static inline void randomize_player(PlyrInfo* player) {
    int attempts = 0;
    int alternate = 0;
    int character = 0;

    do {
        character = (unsigned short)randu0(0x2C);
        alternate = (unsigned short)randu0(4) == 0;
        attempts++;
    } while (is_char_locked(character, alternate) != 0 && attempts < 50);
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
    resolve_alternate_palettes(&g_game_info.plyr1);
}

static const Vec minigame_sd_bid[11] = {
    {0.5243f, 0.34789f, 0.5243f},
    {0.55431f, 0.2401f, 0.55431f},
    {0.87393f, 0.57632f, 0.87393f},
    {0.67988f, 0.65284f, 0.67988f},
    {1.22129f, 1.22129f, 1.22129f},
    {0.55431f, 0.55431f, 0.55431f},
    {1.498f, 1.498f, 1.498f},
    {0.69322f, 0.69322f, 0.69322f},
    {0.78092f, 0.78092f, 0.78092f},
    {0.56518f, 0.56518f, 0.56518f},
    {0.54001f, 0.54001f, 0.54001f},
};

void puzzle_fighter_scale(MkObj* object, float scale) {
    const Vec* direct_scale;
    int index;

    direct_scale = scale == 1.0f ? minigame_sd_bid : 0;
    for (index = 0; index < 0x2F; index++) {
        MkBone* bone;
        int scale_index;

        if (index == 0) {
            scale_index = 0;
        } else if (index == 9) {
            scale_index = 3;
        } else if (index == 0xD) {
            scale_index = 5;
        } else if (index == 0x10) {
            scale_index = 6;
        } else if (index <= 6) {
            scale_index = 1;
        } else if (index <= 0xB) {
            scale_index = 2;
        } else if (index <= 0xE) {
            scale_index = 7;
        } else if (index <= 0x11) {
            scale_index = 8;
        } else if (index <= 0x15) {
            scale_index = 9;
        } else if (index <= 0x17) {
            scale_index = 10;
        } else {
            scale_index = 4;
        }
        bone = object->bones[index];
        if (bone != 0) {
            bone->flags_55_bits.scale_controlled = 1;
            if (direct_scale != 0) {
                bone->scale.x = direct_scale[scale_index].x;
                bone->scale.y = direct_scale[scale_index].y;
                bone->scale.z = direct_scale[scale_index].z;
            } else {
                bone->scale.x =
                    scale * minigame_sd_bid[scale_index].x + 1.0f;
                bone->scale.y =
                    scale * minigame_sd_bid[scale_index].y + 1.0f;
                bone->scale.z =
                    scale * minigame_sd_bid[scale_index].z + 1.0f;
            }
        }
    }
    if (object->bones[0x30] != 0 &&
        object->bones[0x30]->parent_matrix != 0) {
        MkBone* bone = object->bones[0x30];

        bone->flags_55_bits.scale_controlled = 1;
        if (direct_scale != 0) {
            bone->scale.x = direct_scale[6].x;
            bone->scale.y = direct_scale[6].y;
            bone->scale.z = direct_scale[6].z;
        } else {
            bone->scale.x = minigame_sd_bid[6].x * scale + 1.0f;
            bone->scale.y = minigame_sd_bid[6].y * scale + 1.0f;
            bone->scale.z = minigame_sd_bid[6].z * scale + 1.0f;
        }
    }
}

void vdestroy_mkpdata_plyr(PlyrPdata* pdata) {
    destroy_mkpdata_plyr(pdata);
}

#define DESTROY_PLYR_REF(pointer_, instance_)                         \
    do {                                                              \
        MkHdr* live_object = (MkHdr*)(pointer_);                       \
        if (live_object != 0) {                                       \
            if (live_object->instance != (instance_)) {               \
                live_object = 0;                                      \
            }                                                         \
        } else {                                                      \
            live_object = 0;                                          \
        }                                                             \
        if (live_object != 0) {                                       \
            MkHdr* owned_object = (MkHdr*)(pointer_);                  \
            if (owned_object->instance != 0) {                        \
                ((void (*)(MkHdr*))owned_object->vtbl->destroy)(      \
                    owned_object);                                    \
            }                                                         \
            (pointer_) = 0;                                           \
            (instance_) = 0;                                          \
        }                                                             \
    } while (0)

void destroy_mkpdata_plyr(PlyrPdata* pdata) {
    DESTROY_PLYR_REF(pdata->tracked_obj, pdata->tracked_obj_instance);
    DESTROY_PLYR_REF(
        pdata->aux_weapon_latch.obj, pdata->aux_weapon_latch.instance);
    DESTROY_PLYR_REF(pdata->mirror_obj.obj, pdata->mirror_obj.instance);
    DESTROY_PLYR_REF(pdata->anim_proc, pdata->anim_proc_instance);
    DESTROY_PLYR_REF(
        pdata->left_hand_anim_proc, pdata->left_hand_anim_instance);
    DESTROY_PLYR_REF(
        pdata->right_hand_anim_proc, pdata->right_hand_anim_instance);
    DESTROY_PLYR_REF(
        pdata->face_anim_proc, pdata->face_anim_proc_instance);
    DESTROY_PLYR_REF(pdata->field_7C.proc, pdata->field_7C.instance);
    DESTROY_PLYR_REF(pdata->field_84.proc, pdata->field_84.instance);
    DESTROY_PLYR_REF(pdata->field_8C.proc, pdata->field_8C.instance);
    DESTROY_PLYR_REF(
        pdata->goro_hand_anim[0].proc, pdata->goro_hand_anim[0].instance);
    DESTROY_PLYR_REF(
        pdata->goro_hand_anim[1].proc, pdata->goro_hand_anim[1].instance);
    DESTROY_PLYR_REF(
        pdata->goro_hand_anim[2].proc, pdata->goro_hand_anim[2].instance);
    DESTROY_PLYR_REF(
        pdata->goro_hand_anim[3].proc, pdata->goro_hand_anim[3].instance);

    destroy_list(&pdata->active_weapon_links);
    if (pdata->blood_model_data != 0) {
        free_mem(pdata->blood_model_data);
        pdata->blood_model_data = 0;
    }
    pdata->vtbl = free_mkpdata_plyrs;
    pdata->instance = 0;
    free_mkpdata_plyrs = pdata;
}

PlyrPdata* get_mkpdata_plyr(void) {
    PlyrPdata* pdata = free_mkpdata_plyrs;
    int index;

    if (pdata != 0) {
        free_mkpdata_plyrs = (PlyrPdata*)pdata->vtbl;
        pdata->vtbl = &vtbl_mkpdata_plyr;
        mk_set_instance(&pdata->instance);
        pdata->tracked_obj = 0;
        pdata->tracked_obj_instance = 0;
        pdata->held_opponent_latch.obj = 0;
        pdata->held_opponent_latch.instance = 0;
        pdata->aux_weapon_latch.obj = 0;
        pdata->aux_weapon_latch.instance = 0;
        pdata->mirror_obj.obj = 0;
        pdata->mirror_obj.instance = 0;
        pdata->hold_proc = 0;
        pdata->hold_proc_instance = 0;
        pdata->item_links = 0;
        pdata->anim_proc = 0;
        pdata->anim_proc_instance = 0;
        pdata->left_hand_anim_proc = 0;
        pdata->left_hand_anim_instance = 0;
        pdata->right_hand_anim_proc = 0;
        pdata->right_hand_anim_instance = 0;
        pdata->face_anim_proc = 0;
        pdata->face_anim_proc_instance = 0;
        pdata->field_7C.proc = 0;
        pdata->field_7C.instance = 0;
        pdata->field_84.proc = 0;
        pdata->field_84.instance = 0;
        pdata->field_8C.proc = 0;
        pdata->field_8C.instance = 0;
        pdata->goro_hand_anim[0].proc = 0;
        pdata->goro_hand_anim[0].instance = 0;
        pdata->goro_hand_anim[1].proc = 0;
        pdata->goro_hand_anim[1].instance = 0;
        pdata->goro_hand_anim[2].proc = 0;
        pdata->goro_hand_anim[2].instance = 0;
        pdata->goro_hand_anim[3].proc = 0;
        pdata->goro_hand_anim[3].instance = 0;
        pdata->transient_proc = 0;
        pdata->transient_proc_instance = 0;
        pdata->reserved_BC[0] = 0;
        pdata->reserved_BC[1] = 0;
        pdata->reserved_BC[2] = 0;
        pdata->reserved_BC[3] = 0;
        pdata->reserved_BC[4] = 0;
        pdata->reserved_BC[5] = 0;
        pdata->reserved_BC[6] = 0;
        pdata->reserved_BC[7] = 0;
        pdata->impaled_item_a.obj = 0;
        pdata->impaled_item_a.instance = 0;
        pdata->impaled_item_b.obj = 0;
        pdata->impaled_item_b.instance = 0;
        pdata->impaled_item_a_secondary.obj = 0;
        pdata->impaled_item_a_secondary.instance = 0;
        pdata->impaled_item_b_secondary.obj = 0;
        pdata->impaled_item_b_secondary.instance = 0;
        pdata->reserved_FC = 0;
        pdata->spear_proc = 0;
        pdata->spear_proc_instance = 0;
        pdata->reserved_108[0] = 0;
        pdata->own_player_proc = 0;
        pdata->own_player_proc_instance = 0;
        pdata->aux_player_proc = 0;
        pdata->aux_player_proc_instance = 0;
        pdata->reserved_108[1] = 0;
        pdata->reserved_108[2] = 0;
        pdata->reserved_11C[0] = 0;
        pdata->reserved_11C[1] = 0;
        pdata->reserved_11C[2] = 0;
        pdata->reserved_11C[3] = 0;
        pdata->reserved_11C[4] = 0;
        pdata->reserved_11C[5] = 0;
        pdata->active_weapon_links = 0;
        pdata->reserved_138[0] = 0;
        pdata->reserved_138[1] = 0;
        pdata->reserved_138[2] = 0;
        for (index = 0; index < 15; index++) {
            pdata->reserved_obj_latches[index].obj = 0;
            pdata->reserved_obj_latches[index].instance = 0;
        }
        pdata->held_by_player = 0;
        pdata->hold_state = 0;
        pdata->state_flags.bits.pad_bit7 = 0;
        pdata->aux_update_callback = 0;
        for (index = 0; index < 3; index++) {
            pdata->weapon_styles[index] = 0;
        }
        pdata->baraka_moveset_callback = 0;
        pdata->player_slot = -1;
        pdata->fighter_definition = 0;
        pdata->blood_model_data = 0;
        pdata->facial_texture.atc = 0;
        pdata->facial_texture.instance = 0;
        pdata->facial_damage = 0.0f;
        pdata->fatality_shove_active = 0;
        pdata->jaw_monitor = 0;
        pdata->jaw_monitor_instance = 0;
        pdata->baraka_blades_monitor = 0;
        pdata->baraka_blades_monitor_instance = 0;
        pdata->reserved_710[0] = 0;
        pdata->reserved_710[1] = 0;
        pdata->sidekick_obj = 0;
        pdata->sidekick_instance = 0;
        pdata->sidekick_anim_proc = 0;
        pdata->sidekick_anim_instance = 0;
        pdata->sidekick_active = 0;
        pdata->sidekick_available = 0;
    }
    return pdata;
}

void init_mkpdata_plyrs(void) {
    int index;

    memset(_mkpdata_plyrs, 0, sizeof(_mkpdata_plyrs));
    free_mkpdata_plyrs = &_mkpdata_plyrs[0];
    for (index = 0; index < PLYR_PDATA_POOL_COUNT - 1; index++) {
        _mkpdata_plyrs[index].vtbl = &_mkpdata_plyrs[index + 1];
    }
    _mkpdata_plyrs[index].vtbl = 0;
}

void load_aux_weapon(WeaponDefinition* definition) {
    MkObj* weapon = load_weapon(definition, plyr_obj);

    if (weapon != 0) {
        plyr_aux_weapon_grab(plyr_pdata, weapon);
        weapon->hide_flag_bits.hidden = 0;
    }
}

float p_animate_weapon_rest(void) {
    set_anim_script(anim_pdata, anim_pdata->hand_animation, 0x40);
    anim_pdata->rest_ticks = 10;
    ((PlyrProcVtable*)aproc->vtbl)
        ->jump_sleep(p_animate_weapon_rest_lp, 1.0f);
    /* Retail process continuation value. */
    return 1.0f;
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

static inline void release_other_player_inline(void) {
    MkObj* held;
    MkProc* hold_proc;
    MkProc* animation_proc;
    AnimPdata* animation;

    plyr_obj->flags_09_bits.bit4 = 1;
    plyr_obj->flags_09_bits.face_opponent = 1;

    held = plyr_pdata->held_opponent_latch.obj;
    if (held != 0) {
        if (held->hdr.instance ==
            plyr_pdata->held_opponent_latch.instance) {
            /* Keep the live object. */
        } else {
            held = 0;
        }
    } else {
        held = 0;
    }
    if (held != 0) {
        held->flags_09_bits.launched = 1;
        held->flags_09_bits.bit4 = 1;
        held->flags_09_bits.head_tracking = 1;
        held->flags_09_bits.tightrope_restricted = 1;
        held->flags_09_bits.face_opponent = 1;
    }
    plyr_pdata->held_opponent_latch.obj = 0;
    plyr_pdata->held_opponent_latch.instance = 0;

    plyr_pdata->his_plyr_pdata->held_by_player = 0;
    plyr_pdata->his_plyr_pdata->hold_state = 0;

    hold_proc = plyr_pdata->hold_proc;
    if (hold_proc != 0) {
        if (hold_proc->instance ==
            (int)plyr_pdata->hold_proc_instance) {
            /* Keep the live process. */
        } else {
            hold_proc = 0;
        }
    } else {
        hold_proc = 0;
    }
    if (hold_proc != 0) {
        plyr_pdata->hold_proc = 0;
        plyr_pdata->hold_proc_instance = 0;
        if (hold_proc->instance != 0) {
            ((void (*)(MkHdr*))hold_proc->vtbl->destroy)((MkHdr*)hold_proc);
        }

        animation_proc = plyr_pdata->his_plyr_pdata->anim_proc;
        if (animation_proc != 0) {
            if (animation_proc->instance ==
                (int)plyr_pdata->his_plyr_pdata->anim_proc_instance) {
                /* Keep the live process. */
            } else {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        if (animation_proc != 0) {
            animation = (AnimPdata*)pdata_of_proc(animation_proc);
            animation->transition_weight = 1.0f;
            animation->transition_step = 0.0f;
        }
    }
}

void release_other_player(void) {
    release_other_player_inline();
}

int check_release_other_player(void) {
    MkProc* hold_proc = plyr_pdata->hold_proc;

    if (hold_proc != 0 &&
        hold_proc->instance == (int)plyr_pdata->hold_proc_instance) {
        release_other_player_inline();
        return 1;
    }
    return 0;
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

static inline void set_object_flip(
    MkObj* object, AnimPdata* animation, int flip_state) {
    switch (flip_state) {
    case 1:
        if (object->hide_flag_bits.bit6 != 0) {
            object->hide_flag_bits.bit6 = 0;
            animation->flags ^= 8;
        }
        break;
    case 2:
        if (object->hide_flag_bits.bit6 == 0) {
            object->hide_flag_bits.bit6 = 1;
            animation->flags ^= 8;
        }
        break;
    }
}

MkHdr* plyr_grab_other_flip_states(
    int player_flip, int opponent_flip) {
    MkObj* opponent = plyr_pdata->his_obj;
    AnimPdata* opponent_animation = 0;
    MkObj* held;
    MkProc* hold_proc;
    MkProc* opponent_anim_proc;
    GrabBoneMatcher* matcher;

    set_object_flip(plyr_obj, plyr_anim_pdata, player_flip);

    held = plyr_pdata->held_opponent_latch.obj;
    if (held != 0) {
        if (held->hdr.instance !=
            plyr_pdata->held_opponent_latch.instance) {
            held = 0;
        }
    } else {
        held = 0;
    }
    hold_proc = plyr_pdata->hold_proc;
    if (hold_proc != 0) {
        if (hold_proc->instance !=
            (int)plyr_pdata->hold_proc_instance) {
            hold_proc = 0;
        }
    } else {
        hold_proc = 0;
    }
    if (held != 0 || hold_proc != 0) {
        return 0;
    }

    if (opponent_flip != 0) {
        opponent_anim_proc = plyr_pdata->his_plyr_pdata->anim_proc;
        if (opponent_anim_proc != 0) {
            if (opponent_anim_proc->instance !=
                (int)plyr_pdata->his_plyr_pdata->anim_proc_instance) {
                opponent_anim_proc = 0;
            }
        } else {
            opponent_anim_proc = 0;
        }
        if (opponent_anim_proc != 0) {
            opponent_animation =
                (AnimPdata*)pdata_of_proc(opponent_anim_proc);
        }
    }
    if (opponent_animation != 0) {
        set_object_flip(opponent, opponent_animation, opponent_flip);
    }
    opponent->flags_09_bits.launched = 0;
    opponent->flags_09_bits.bit4 = 0;
    opponent->flags_09_bits.head_tracking = 0;
    opponent->flags_09_bits.tightrope_restricted = 0;
    opponent->flags_09_bits.face_opponent = 0;
    plyr_obj->flags_09_bits.bit4 = 0;
    plyr_obj->flags_09_bits.face_opponent = 0;
    plyr_pdata->held_opponent_latch.obj = opponent;
    plyr_pdata->held_opponent_latch.instance =
        opponent->hdr.instance;
    matcher = (GrabBoneMatcher*)start_bone_matcher(
        plyr_obj, plyr_obj->fallback_bone_index,
        opponent, opponent->fallback_bone_index, 5.0f);
    if (matcher != 0) {
        matcher->source_offset.z = 0.0f;
        matcher->source_offset.y = 0.0f;
        matcher->source_offset.x = 0.0f;
        matcher->target_offset.z = 0.0f;
        matcher->target_offset.y = 0.0f;
        matcher->target_offset.x = 0.0f;
        matcher->flags |= 0x10000000;
        matcher->flags |= 0x04000000;
        matcher->flags |= 0x02000000;
        matcher->weight = 0.75f;
        plyr_pdata->hold_proc = (MkProc*)matcher;
        plyr_pdata->hold_proc_instance = matcher->hdr.instance;
    }
    return (MkHdr*)matcher;
}

#undef DESTROY_PLYR_REF

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

void become_plyr2_proc(void) {
    apdata_save = apdata;
    apdata = (MkHdr*)g_game_info.plyr1.slot.pdata;
    pw_plyr_inline();
}

void become_plyr1_proc(void) {
    apdata_save = apdata;
    apdata = (MkHdr*)g_game_info.plyr0.slot.pdata;
    pw_plyr_inline();
}

void swap_active_plyr_proc(void) {
    PlyrPdata* current = (PlyrPdata*)apdata;

    apdata = (MkHdr*)current->his_plyr_pdata;
    pw_plyr_inline();
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
