#include "game/nis.h"

#include "game/game_info.h"
#include "runtime/fonts.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

typedef struct ScreenObj ScreenObj;

/*
 * NIS scene pdata (0x14) -- MkHdr + cancel/scene MKO func indices + script.
 * scene_func @ +0x0C runs in p_run_nis_scene; cancel_func @ +0x08 on skip.
 */
typedef struct NisPdata {
    MkHdr hdr;                 /* +0x00 */
    unsigned int cancel_func;  /* +0x08 */
    unsigned int scene_func;   /* +0x0C */
    ScriptSlot* cmdscript;     /* +0x10 */
} NisPdata; /* 0x14 */

typedef struct KamidoguDropPdata {
    MkHdr hdr;               /* +0x00 */
    MkObj* owner;            /* +0x08 */
    unsigned int owner_id;   /* +0x0C */
    float quat[4];           /* +0x10 */
} KamidoguDropPdata; /* 0x20 */

/* Local vtbl shape so sleep/jump_sleep take the right args (mk_vtbl uses MkVtblFn). */
typedef struct MkVtableMkprocLocal {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
    int (*system_stack)(void);
    int (*local_stack)(void);
    /* NIS loads f1 before the call (sleep ticks / zero); take float to match. */
    float (*jump_sleep)(MkProcEntryFn entry, float ticks);
} MkVtableMkprocLocal;

static const char stringBase0[] =
    "SHUJINKO_UNLOCKED_A\0"
    "SHUJINKO_UNLOCKED_B\0"
    "nx1_nis_1_anims.sec\0"
    "unlock_screen.sec\0"
    "nx1_dialog_text.mko\0";

const int gap_04_80314D64_rodata = 0;

static const float flt_340 = 0.3f;
static const float flt_341 = 180.0f;
static const double dbl_343 = 4503601774854144.0;
static const float flt_348 = 60.0f;
static const float flt_349 = 1.0f;
static const float flt_350 = -1.0f;
static const float flt_361 = 0.0f;
static const float flt_362 = 2.0f;
static const float flt_375 = 0.025f;
static const float flt_376 = 1.4f;
static const float flt_377 = 0.005f;

unsigned int nis_event_list[4];

/* MWCC emits .sbss in reverse declaration order. */
int gap_08_80510E74_sbss;
int nis_wait_override;

extern char bgnd_animations[];
extern float identity_quat[];
extern char p1_profile[];
extern int screen_width;
extern int screen_height;

/* Retail leaves MkProc* in r3; Matching mk_pdata.h types these as void. */
MkProc* _create_mkproc_generic_tinystack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                         int pdata_size, MkHdr** pdata_out);
MkProc* _create_mkproc_generic_nostack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                       int pdata_size, MkHdr** pdata_out);
MkProc* _create_mkproc_generic_bigstack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                        int pdata_size, MkHdr** pdata_out);

void load_named_2d_pfxobj_xy(int slot, int oid, const char* name, int x, int y, int arg6, int arg7);
ScreenObj* insert_2d_obj(ScreenObj* obj);
void fade_from_black(int frames, int flag);
void pfx_2d_obj_set_alpha_by_id(int oid, int alpha);
void delete_screen_obj_oid(int oid);
MkProc* get_fake_bone_matcher_proc(void* arg);
void mkscripts_destroy_fk_bonematcher(void* bonematcher);
void set_mat(void* mat, void* src);
void mat_to_quat(void* quat, void* mat);
void snd_req_vol(int sound_id, float volume);
void interp_quat(void* dst, void* src, void* scratch, float t);
void quat_to_mat(void* mat, void* quat);
void display_load_meter(int slot);
void turn_camera_on(void);
void load_background(int id);
void add_anim_section_by_name_async_pal(int slot, const char* name, void* bss, int arg4, int arg5);
void wait_for_slot_load(int slot);
void load_art_section_by_name(int slot, const char* name);
void start_plyrs(void);
void setup_sound_banks(int bank);
void wait_for_sound_banks_to_load(void);
void kill_head_tracking(void);
void fade_to_black(int frames, int flag);
void set_mode_of_play(int mode);
void start_tunes(void);
void delete_player(int player);
void unload_section_slot(int slot);
void mark_as_unlocked(void* profile, int category, int character);
void set_konq_profile_value(int profile, int field, int value);
void save_profile(int profile, int slot);
void gamelogic_jump(int action, void (*logic)(void));
void memset(void* dest, int val, int size);
void push_game_state(int state);
void turn_controllers_on(void);
int check_switch_edge(int player, int edge);
void eat_switch_edge(int player, int edge);
void pop_game_state(void);
void set_section_memory_scheme(int scheme);
void zero_pdata_payload(int size, MkHdr* dest);

extern void p_credits_screen(void);

static float p_fade_fullscreen_image(void);
static float p_drop_kamidogu(void);
static float p_init_skip_nis(void);
static float p_check_skip_nis(void);
static float p_run_nis_cancel_function(void);
static float p_run_nis_scene(void);

static void mkproc_sleep(void) {
    ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
}

void show_shujinko_unlock_screen(int string_id) {
    int x_pos;
    unsigned int string_id_full;
    const char* text;
    PfxFontSlot* font;
    StringObj* str_obj;
    int text_y;
    double conv_val;

    x_pos = (screen_width - 0x300) / 2;
    load_named_2d_pfxobj_xy(0x10005, 0x7F01, &stringBase0[0], 0, x_pos, 0, 0xE);
    load_named_2d_pfxobj_xy(0x10005, 0x7F01, &stringBase0[0x14], 0, x_pos + 0x200, 0, 0xE);
    string_id_full = (unsigned int)string_id;
    string_id_full = string_id_full | 0x20000;
    text = get_string_by_id(string_id_full);
    font = load_font(9);
    conv_val = (double)screen_height;
    text_y = (int)(flt_340 * (float)conv_val);
    str_obj = create_wrapped_string(0x7F01, font, text, screen_width / 4, text_y, 0, 1, 0, 0);
    insert_2d_obj((ScreenObj*)str_obj);
    fade_from_black(8, 1);
    _mkproc_sleep_ticks = flt_341;
    mkproc_sleep();
    _create_mkproc_generic_tinystack(0x9031, 0x1F, p_fade_fullscreen_image, 0, 0);
}

static float p_fade_fullscreen_image(void) {
    float sleep_ticks;
    unsigned int alpha;

    _mkproc_sleep_ticks = flt_348;
    mkproc_sleep();
    sleep_ticks = flt_349;
    alpha = 0xFF;
    while ((alpha & 0xFF) > 2) {
        pfx_2d_obj_set_alpha_by_id(0x7F01, (int)alpha);
        _mkproc_sleep_ticks = sleep_ticks;
        mkproc_sleep();
        alpha -= 2;
    }
    pfx_2d_obj_set_alpha_by_id(0x7F01, 0);
    _mkproc_sleep_ticks = sleep_ticks;
    mkproc_sleep();
    delete_screen_obj_oid(0x7F01);
    return flt_350;
}

void release_kamidogu(MkObj* owner, void* bonematcher) {
    MkProc* matcher_proc;
    MkPtr* list_item;
    MkHdr* owner_hdr;
    KamidoguDropPdata* pdata;
    MkHdr* pdata_storage;
    float mat_buf[0x18];
    float zero_val;

    matcher_proc = get_fake_bone_matcher_proc(bonematcher);
    if (owner != 0) {
        owner_hdr = as_mkhdr((MkHdr*)owner);
    } else {
        owner_hdr = 0;
    }
    list_item = find_in_mklist(owner_hdr, &matcher_proc->pdata_list_b);
    list_item->hdr = 0;
    destroy_mkptr(list_item);
    mkscripts_destroy_fk_bonematcher(bonematcher);
    if (owner == 0) {
        return;
    }
    owner->flags_08 = (unsigned char)((owner->flags_08 & 0xBF) | 0x40);
    pdata_storage = 0;
    if (_create_mkproc_generic_nostack(0x902E, 0x1F, p_drop_kamidogu, 0x20, &pdata_storage) == 0) {
        return;
    }
    pdata = (KamidoguDropPdata*)pdata_storage;
    set_mat(mat_buf, owner->field_24);
    zero_val = flt_361;
    owner->pos.x = mat_buf[0xC];
    owner->pos.y = mat_buf[0xD];
    owner->pos.z = mat_buf[0xE];
    mat_buf[0xC] = zero_val;
    mat_buf[0xD] = zero_val;
    mat_buf[0xE] = zero_val;
    mat_to_quat(pdata->quat, mat_buf);
    pdata->owner = owner;
    pdata->owner_id = owner->hdr.instance;
    snd_req_vol(0x1789, flt_362);
}

static float p_drop_kamidogu(void) {
    KamidoguDropPdata* pdata;
    MkObj* owner;
    MkObj* live;
    float y_pos;
    float new_y;

    pdata = (KamidoguDropPdata*)apdata;
    if (pdata == 0) {
        return flt_350;
    }
    owner = pdata->owner;
    if (owner == 0) {
        live = 0;
    } else {
        if (owner->hdr.instance == pdata->owner_id) {
            live = owner;
        } else {
            live = 0;
        }
    }
    if (live == 0) {
        return flt_350;
    }
    interp_quat(pdata->quat, identity_quat, pdata->quat, flt_375);
    quat_to_mat(live->field_24, pdata->quat);
    y_pos = live->pos.y;
    if (y_pos >= flt_376) {
        new_y = y_pos - flt_377;
        live->pos.y = new_y;
        if (live->pos.y < flt_376) {
            live->pos.y = flt_376;
        }
    }
    return flt_349;
}

void p_konquest_ending(void) {
    GameInfo* info;
    unsigned char flags;

    set_process_as_scriptable(aproc);
    set_section_memory_scheme(0);
    display_load_meter(0x50014);
    turn_camera_on();
    load_background(0x22);
    add_anim_section_by_name_async_pal(0x50014, &stringBase0[0x28], &bgnd_animations[0x28], 0, 0);
    wait_for_slot_load(0x50014);
    load_art_section_by_name(0x10005, &stringBase0[0x3C]);
    load_string_bank(2, (char*)&stringBase0[0x4E]);
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
    _mkproc_sleep_ticks = flt_349;
    mkproc_sleep();
    xfer_proc((MkProc*)info->plyr0.idle_proc, p_idle);
    xfer_proc((MkProc*)info->plyr1.idle_proc, p_idle);
    kill_head_tracking();
    destroy_mkprocs_pid(0x1003);
    fade_to_black(8, 0);
    set_mode_of_play(0);
    flags = g_game_info.flags;
    flags = (unsigned char)((flags & 0xFD) | 2);
    g_game_info.flags = flags;
    start_tunes();
    cmdscript_setup_execution(g_game_info.cmdscript, 0);
    cmdscript_execute(g_game_info.cmdscript);
    delete_player(0);
    delete_player(1);
    unload_section_slot(0x50014);
    mark_as_unlocked(p1_profile, 1, 0x19);
    set_konq_profile_value(0, 4, 1);
    save_profile(0, 2);
    gamelogic_jump(6, p_credits_screen);
}

void nis_set_wait_override(int value) {
    nis_wait_override = value;
}

void nis_clear_event_list(void) {
    memset(nis_event_list, 0, 0x10);
    nis_wait_override = 0;
}

void nis_show_cancel_message(void) {
    MkProc* parent_proc;
    NisPdata* pdata;
    MkProc* new_proc;

    parent_proc = find_mkproc_pid(0x900C);
    if (parent_proc != 0) {
        pdata = (NisPdata*)pdata_of_proc(parent_proc);
        if (pdata != 0 && pdata->cancel_func != 0) {
            new_proc = _create_mkproc_generic_nostack(0x901A, 0x1F, p_init_skip_nis, 0, 0);
            if (new_proc != 0) {
                mk_insert((MkHdr*)new_proc, &parent_proc->pdata_list_b);
            }
        }
        turn_controllers_on();
    }
}

static float p_init_skip_nis(void) {
    const char* text;
    PfxFontSlot* font;
    StringObj* str_obj;

    text = get_string_by_id(0x10001);
    font = load_font(6);
    str_obj = string_center_xy(0x900F, 6, text, screen_width / 2, 0x1A1, 0xB);
    if (str_obj != 0) {
        mk_insert((MkHdr*)str_obj, &aproc->pdata_list_b);
    }
    ((MkVtableMkprocLocal*)aproc->vtbl)->jump_sleep(p_check_skip_nis, flt_361);
    return flt_361;
}

static float p_check_skip_nis(void) {
    MkProc* parent_proc;
    NisPdata* pdata;

    parent_proc = find_mkproc_pid(0x900C);
    if (parent_proc == 0) {
        return flt_350;
    }
    if (check_switch_edge(g_game_info.plyr0.pad_index, 6) == 0 &&
        check_switch_edge(g_game_info.plyr1.pad_index, 6) == 0) {
        return flt_349;
    }
    pdata = (NisPdata*)pdata_of_proc(parent_proc);
    if (pdata != 0) {
        if (pdata->cancel_func != 0) {
            xfer_proc(parent_proc, p_run_nis_cancel_function);
        }
    }
    eat_switch_edge(0, 6);
    eat_switch_edge(1, 6);
    return flt_350;
}

static float p_run_nis_cancel_function(void) {
    NisPdata* pdata;

    pdata = (NisPdata*)apdata;
    if (pdata != 0) {
        cmdscript_setup_execution(pdata->cmdscript, pdata->cancel_func);
        cmdscript_execute(pdata->cmdscript);
    }
    return flt_350;
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

void nis_signal_event(int event) {
    unsigned int word_index = (unsigned int)event >> 5;
    unsigned int bit = (unsigned int)event & 0x1F;

    nis_event_list[word_index] |= 1U << bit;
}

void nis_wait_for_event(int event, int timeout) {
    float sleep_ticks;
    unsigned int offset;
    int bit_mask;
    int remaining;
    char* list;
    unsigned int word;

    sleep_ticks = flt_349;
    list = (char*)nis_event_list;
    offset = (unsigned int)event;
    offset = offset << 29;
    offset = offset >> 3;
    bit_mask = 1 << (event & 0x1F);
    remaining = timeout;
    for (;;) {
        if (nis_wait_override == 0) {
            word = *(unsigned int*)(list + offset);
            if ((word & bit_mask) != 0) {
                break;
            }
        }
        if (remaining == 0) {
            break;
        }
        if (remaining > 0) {
            remaining--;
        }
        _mkproc_sleep_ticks = sleep_ticks;
        mkproc_sleep();
    }
}

void nis_init(ScriptSlot* cmdscript, unsigned int scene_func, unsigned int cancel_func) {
    MkProc* proc;
    NisPdata* pdata;
    MkHdr* pdata_ptr;

    memset(nis_event_list, 0, 0x10);
    nis_wait_override = 0;
    push_game_state(0x16);
    pdata_ptr = 0;
    proc = _create_mkproc_generic_bigstack(0x900C, 0x1F, p_run_nis_scene, 0x14, &pdata_ptr);
    if (proc == 0) {
        return;
    }
    pdata = (NisPdata*)pdata_ptr;
    zero_pdata_payload(0x14, (MkHdr*)pdata);
    pdata->scene_func = scene_func;
    pdata->cancel_func = cancel_func;
    pdata->cmdscript = cmdscript;
    nis_wait_override = 0;
    set_process_as_scriptable(proc);
}

static float p_run_nis_scene(void) {
    NisPdata* pdata;

    pdata = (NisPdata*)apdata;
    if (pdata != 0) {
        cmdscript_setup_execution(pdata->cmdscript, pdata->scene_func);
        cmdscript_execute(pdata->cmdscript);
    }
    return flt_350;
}
