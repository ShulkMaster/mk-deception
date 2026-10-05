#include "game/nis.h"
#include "game/game.h"
#include "game/ending.h"
#include "game/plyrprofile.h"
#include "game/konquest_items.h"
#include "game/bgnd.h"
#include "game/plyr.h"
#include "runtime/cstring.h"
#include "platform/main_jump.h"
#include "runtime/utils.h"
#include "runtime/image.h"
#include "runtime/cam.h"
#include "platform/io.h"
#include "platform/display_metrics.h"
#include "runtime/sound.h"

#include "game/game_info.h"
#include "math/mk_math.h"
#include "runtime/fonts.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

struct NisPdata {
    MkHdr hdr;
    unsigned int cancel_func;
    unsigned int scene_func;
    ScriptSlot* cmdscript;
};

struct KamidoguDropPdata {
    MkHdr hdr;
    MkObj* owner;
    unsigned int owner_id;
    Quat quat;
};

static const char stringBase0[] =
    "SHUJINKO_UNLOCKED_A\0"
    "SHUJINKO_UNLOCKED_B\0"
    "nx1_nis_1_anims.sec\0"
    "unlock_screen.sec\0"
    "nx1_dialog_text.mko\0";

const int gap_04_80314D64_rodata = 0;

unsigned int nis_event_list[4];

int gap_08_80510E74_sbss;
int nis_wait_override;

extern char bgnd_animations[];

struct FatalityFakeBoneMatcher;
MkProc* get_fake_bone_matcher_proc(struct FatalityFakeBoneMatcher* matcher);
void mkscripts_destroy_fk_bonematcher(void* bonematcher);
void add_anim_section_by_name_async_pal(int slot, const char* name, void* bss, int arg4, int arg5);
void wait_for_slot_load(int slot);
void load_art_section_by_name(int slot, const char* name);
void kill_head_tracking(void);
void unload_section_slot(int slot);
void set_section_memory_scheme(int scheme);

static float p_fade_fullscreen_image(void);
static float p_drop_kamidogu(void);
static float p_init_skip_nis(void);
static float p_check_skip_nis(void);
static float p_run_nis_cancel_function(void);
static float p_run_nis_scene(void);

static void mkproc_sleep(void) {
    aproc->vtbl->sleep();
}

#pragma optimize_for_size on
#pragma use_lmw_stmw on
void show_shujinko_unlock_screen(int string_id) {
    int x_pos;
    StringObj* str_obj;

    x_pos = (screen_width - 0x300) / 2;
    load_named_2d_pfxobj_xy(0x10005, 0x7F01, &stringBase0[0], 0, x_pos, 0, 0xE);
    load_named_2d_pfxobj_xy(0x10005, 0x7F01, &stringBase0[0x14], 0, x_pos + 0x200, 0, 0xE);
    str_obj = create_wrapped_string(0x7F01, load_font(9), get_string_by_id(string_id | 0x20000), screen_width / 4, 0.3f * screen_height, screen_width / 2, 0, 1, 0);
    insert_2d_obj((ScreenObj*)str_obj);
    fade_from_black(8, 1);
    _mkproc_sleep_ticks = 180.0f;
    mkproc_sleep();
    _create_mkproc_generic_tinystack(0x9031, 0x1F, p_fade_fullscreen_image, 0, 0);
}

#pragma optimize_for_size reset
#pragma use_lmw_stmw reset

static float p_fade_fullscreen_image(void) {
    float sleep_ticks;
    unsigned int alpha;

    _mkproc_sleep_ticks = 60.0f;
    mkproc_sleep();
    sleep_ticks = 1.0f;
    alpha = 0xFF;
    while ((alpha & 0xFF) > 2) {
        pfx_2d_obj_set_alpha_by_id(0x7F01, alpha);
        _mkproc_sleep_ticks = sleep_ticks;
        mkproc_sleep();
        alpha -= 2;
    }
    pfx_2d_obj_set_alpha_by_id(0x7F01, 0);
    _mkproc_sleep_ticks = sleep_ticks;
    mkproc_sleep();
    delete_screen_obj_oid(0x7F01);
    return -1.0f;
}

#pragma optimize_for_size on
#pragma use_lmw_stmw on

void release_kamidogu(MkObj* owner, void* bonematcher) {
    MkProc* matcher_proc;
    MkPtr* list_item;
    struct KamidoguDropPdata* pdata;
    MKMATRIX matrix;

    matcher_proc = get_fake_bone_matcher_proc(bonematcher);
    list_item = find_in_mklist(owner != 0 ? as_mkhdr(&owner->hdr) : 0, &matcher_proc->pdata_list_b);
    discard_stale_mkptr(list_item);
    mkscripts_destroy_fk_bonematcher(bonematcher);
    if (owner == 0) {
        return;
    }
    owner->flags_08_bits.airborne = 1;
    if (_create_mkproc_generic_nostack(0x902E, 0x1F, p_drop_kamidogu, sizeof(struct KamidoguDropPdata), (MkHdr**)&pdata) == 0) {
        return;
    }
    set_mat(&matrix, owner->field_24);
    owner->pos.value.x = matrix.pos.x;
    owner->pos.value.y = matrix.pos.y;
    owner->pos.value.z = matrix.pos.z;
    matrix.pos.x = matrix.pos.y = matrix.pos.z = 0.0f;
    mat_to_quat(&pdata->quat, &matrix);
    pdata->owner = owner;
    pdata->owner_id = owner->hdr.instance;
    snd_req_vol(0x1789, 2.0f);
}

static float p_drop_kamidogu(void) {
    struct KamidoguDropPdata* pdata;
    MkObj* owner;

    pdata = (struct KamidoguDropPdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }
    owner = MK_HDR_LIVE(pdata->owner, pdata->owner_id);
    if (owner == 0) {
        return -1.0f;
    }
    interp_quat(&pdata->quat, &identity_quat, &pdata->quat, 0.025f);
    quat_to_mat(owner->field_24, &pdata->quat);
    if (owner->pos.value.y >= 1.4f) {
        owner->pos.value.y -= 0.005f;
        if (owner->pos.value.y <= 1.4f) {
            owner->pos.value.y = 1.4f;
        }
    }
    return 1.0f;
}

#pragma optimize_for_size reset
#pragma use_lmw_stmw reset

float p_konquest_ending(void) {
    GameInfo* info;

    set_process_as_scriptable(aproc);
    set_section_memory_scheme(0);
    display_load_meter(0x50014);
    turn_camera_on();
    load_background(0x22);
    add_anim_section_by_name_async_pal(0x50014, &stringBase0[0x28], &bgnd_animations[0x28], 0, 0);
    wait_for_slot_load(0x50014);
    load_art_section_by_name(0x10005, &stringBase0[0x3C]);
    load_string_bank(0x20000, (char*)&stringBase0[0x4E]);
    info = &g_game_info;
    info->plyr0.player_state = 2;
    info->plyr0.player_index = 0x19;
    info->plyr0.field_14 = 0;
    info->plyr1.player_state = 2;
    info->plyr1.player_index = 0x1D;
    info->plyr1.field_14 = 0;
    start_plyrs();
    setup_sound_banks(8);
    wait_for_sound_banks_to_load();
    _mkproc_sleep_ticks = 1.0f;
    mkproc_sleep();
    xfer_proc(info->plyr0.idle_proc, p_idle);
    xfer_proc(info->plyr1.idle_proc, p_idle);
    kill_head_tracking();
    destroy_mkprocs_pid(0x1003);
    fade_to_black(8, 0);
    set_mode_of_play(0);
    g_game_info.flag_bits.load_complete = 1;
    start_tunes();
    cmdscript_setup_execution(g_game_info.cmdscript, 1);
    cmdscript_execute(g_game_info.cmdscript);
    delete_player(0);
    delete_player(1);
    unload_section_slot(0x50014);
    mark_as_unlocked(&p1_profile, 1, 0x19);
    set_konq_profile_value(0, 4, 1);
    save_profile(0, 2);
    gamelogic_jump(6, p_credits_screen);
    return -1.0f;
}

void nis_set_wait_override(int value) {
    nis_wait_override = value;
}

void nis_clear_event_list(void) {
    memset(nis_event_list, 0, sizeof(nis_event_list));
    nis_wait_override = 0;
}

void nis_show_cancel_message(void) {
    MkProc* parent_proc;
    struct NisPdata* pdata;
    MkProc* new_proc;

    parent_proc = find_mkproc_pid(0x900C);
    if (parent_proc != 0) {
        pdata = (struct NisPdata*)pdata_of_proc(parent_proc);
        if (pdata != 0 && pdata->cancel_func != 0) {
            new_proc = _create_mkproc_generic_nostack(0x901A, 0x1F, p_init_skip_nis, 0, 0);
            if (new_proc != 0) {
                mk_insert((MkHdr*)new_proc, &parent_proc->pdata_list_b);
            }
        }
        turn_controllers_on();
    }
}

#pragma optimize_for_size on
static float p_init_skip_nis(void) {
    const char* text;
    StringObj* str_obj;

    text = get_string_by_id(0x10001);
    load_font(6);
    str_obj = string_center_xy(0x900F, 6, text, screen_width / 2, 0x1A1, 0xB);
    if (str_obj != 0) {
        mk_insert((MkHdr*)str_obj, &aproc->pdata_list_b);
    }
    aproc->vtbl->jump_sleep(p_check_skip_nis, 0.0f);
    return 0.0f;
}

#pragma optimize_for_size reset

static float p_check_skip_nis(void) {
    MkProc* parent_proc;
    struct NisPdata* pdata;

    parent_proc = find_mkproc_pid(0x900C);
    if (parent_proc == 0) {
        return -1.0f;
    }
    if (check_switch_edge(g_game_info.plyr0.pad_index, 6) != 0 ||
        check_switch_edge(g_game_info.plyr1.pad_index, 6) != 0) {
        pdata = (struct NisPdata*)pdata_of_proc(parent_proc);
        if (pdata != 0) {
            if (pdata->cancel_func != 0) {
                xfer_proc(parent_proc, p_run_nis_cancel_function);
            }
        }
        eat_switch_edge(0, 6);
        eat_switch_edge(1, 6);
        return -1.0f;
    }
    return 1.0f;
}

static float p_run_nis_cancel_function(void) {
    struct NisPdata* pdata;

    pdata = (struct NisPdata*)apdata;
    if (pdata != 0) {
        cmdscript_setup_execution(pdata->cmdscript, pdata->cancel_func);
        cmdscript_execute(pdata->cmdscript);
    }
    return -1.0f;
}

int nis_scene_done(void) {
    MkProc* proc;

    proc = find_mkproc_pid(0x900C);
    if (proc == 0) {
        return 1;
    }
    return 0;
}

void nis_end(void) {
    pop_game_state();
    destroy_mkprocs_pid(0x900C);
}

/* TODO: [near miss] 94%; identical bitset operations; eight volatile GPR assignments remain. */
void nis_signal_event(int event) {
    nis_event_list[(unsigned int)event >> 5] |= 1U << (event & 0x1F);
}

#pragma optimize_for_size on
#pragma use_lmw_stmw on

void nis_wait_for_event(int event, int timeout) {
    unsigned int* event_word;
    int bit_index;
    float sleep_ticks;

    sleep_ticks = 1.0f;
    event_word = &nis_event_list[(unsigned int)event >> 5];
    bit_index = event & 0x1F;
    while (nis_wait_override != 0 ||
           (*event_word & (1U << bit_index)) == 0) {
        if (timeout == 0) {
            break;
        }
        if (timeout > 0) {
            timeout--;
        }
        _mkproc_sleep_ticks = sleep_ticks;
        mkproc_sleep();
    }
}

void nis_init(ScriptSlot* cmdscript, unsigned int scene_func, unsigned int cancel_func) {
    MkProc* proc;
    struct NisPdata* pdata;

    memset(nis_event_list, 0, sizeof(nis_event_list));
    nis_wait_override = 0;
    push_game_state(0x16);
    proc = _create_mkproc_generic_bigstack(0x900C, 0x1F, p_run_nis_scene, sizeof(struct NisPdata), (MkHdr**)&pdata);
    if (proc == 0) {
        return;
    }
    zero_pdata_payload(sizeof(struct NisPdata), &pdata->hdr);
    pdata->scene_func = scene_func;
    pdata->cancel_func = cancel_func;
    pdata->cmdscript = cmdscript;
    nis_wait_override = 0;
    set_process_as_scriptable(proc);
}

#pragma optimize_for_size reset
#pragma use_lmw_stmw reset

static float p_run_nis_scene(void) {
    struct NisPdata* pdata;

    pdata = (struct NisPdata*)apdata;
    if (pdata != 0) {
        cmdscript_setup_execution(pdata->cmdscript, pdata->scene_func);
        cmdscript_execute(pdata->cmdscript);
    }
    return -1.0f;
}
