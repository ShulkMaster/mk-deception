#include "game/menu.h"

#include "game/attract.h"
#include "game/game_info.h"
#include "game/ending.h"
#include "game/controller.h"
#include "game/switch.h"
#include "game/plyr.h"
#include "game/konquest_save.h"
#include "game/plyrprofile.h"
#include "game/pselect.h"
#include "game/settings.h"
#include "mw/mwScreenEngineGlue.h"
#include "platform/io.h"
#include "platform/gcio.h"
#include "platform/display_metrics.h"
#include "game/game.h"
#include "game/ladder.h"
#include "platform/main.h"
#include "platform/gcutils.h"
#include "runtime/asset.h"
#include "runtime/cam.h"
#include "runtime/fonts.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/section.h"
#include "runtime/sound.h"
#include "runtime/utils.h"

struct SoundtrackTitleEntry {
    const char* meta;
    const char* title;
};

struct SoundtrackScreenPdata {
    MkHdr hdr;
    int sound_ids[21];
    char pad_05C[0xC];
    struct SoundtrackTitleEntry titles[21];
    char pad_110[0x18];
    const char* composers[21];
    char pad_17C[0xC];
    const char* details[21];
    char pad_1DC[0xC];
    const char* descriptions[21];
    char pad_23C[0xC];
    int title_count;
    int current;
};

struct ControllerWatcherPdata {
    MkHdr hdr;
    int last_num_controllers;
};

struct ControllerConfigPdata {
    MkHdr hdr;
    int p1_last_button;
    int p1_cell;
    int p1_save;
    int p2_last_button;
    int p2_cell;
    int p2_save;
};

struct MenuPlayerWalkView {
    char pad_00[0xA4];
    PlyrInfo player;
};

struct VersionCodePdata {
    MkHdr hdr;
    unsigned int ticks;
};

struct OptionsSoundPdata {
    MkHdr hdr;
    MslSoundHandle sound_handle;
};

struct PauseMenuPdata {
    MkHdr hdr;
    int player;
    int was_paused;
};

extern int disc_error_occurred;
extern int p1_profile_status;
extern int p2_profile_status;
extern int last_switch_time;
extern SwitchMapEntry p1_temp_switch_map[];
extern SwitchMapEntry p2_temp_switch_map[];
extern int controller_image_offset_tbl[];
extern int p1_rumble_on;
extern int p2_rumble_on;
extern int p1_temp_rumble_state;
extern int p2_temp_rumble_state;
extern int p1_use_temp_switch_map;
extern int p2_use_temp_switch_map;
extern const char* mk6_version_string;
extern const char* number_strings[];
extern unsigned long display_off;
extern int pause_player;
extern int game_save_loop_count;
extern int __mini_game_display_ctrl;

void snd_stop(MslSoundHandle handle);
int get_menu_mode_sub_var(void);
int get_gameoption_exitwithsave(void);
void set_button_repeat_time(int ticks);
void set_default_button_repeat_time(void);
void ck_do_profile_save(void);
void xfer_puzzle_exit(int mode);

extern float p_konquest_mode(void);
extern float p_kontent(void);
extern float p_krypt_mode(void);

static float p_controller_watcher(void);

int menu_mode_net = -1;
int online_locked_port = -1;
int selected_refresh_rate = 0x3C;

static int do_main_menu_timeout;
static int main_menu_timeout_ticks;
static int widescreen_mode_on;
static int progressive_scan_on;
int need_to_reload_systemart;
int lan_networking_selected;
int target_game_mode;
int menu_player;

static const float sleep_ticks_neg_one = -1.0f;
static const float sleep_ticks_one = 1.0f;

#define MENU_TARGET_OPTIONS 15
#define MENU_TARGET_CREATE_PROFILE 18
#define MENU_TARGET_VIEW_PROFILE 19
#define MENU_TARGET_DELETE_PROFILE 20
#define MENU_TARGET_IDLE 24

static const char stringBase0[] =
     "msel_goro.sec\0"
     "msel_shao_kahn.sec\0"
     "msel_ashrah.sec\0"
     "msel_baraka.sec\0"
     "msel_boraicho.sec\0"
     "msel_dairou.sec\0"
     "msel_darrius.sec\0"
     "msel_ermac.sec\0"
     "msel_havik.sec\0"
     "msel_hotaru.sec\0"
     "msel_jade.sec\0"
     "msel_kabal.sec\0"
     "msel_kenshi.sec\0"
     "msel_kira.sec\0"
     "msel_kobra.sec\0"
     "msel_liu_kang.sec\0"
     "msel_li_mei.sec\0"
     "msel_mileena.sec\0"
     "msel_nightwolf.sec\0"
     "msel_raiden.sec\0"
     "msel_scorpion.sec\0"
     "msel_shujinko.sec\0"
     "msel_sindel.sec\0"
     "msel_smokenoob.sec\0"
     "msel_subzero.sec\0"
     "msel_tanya.sec\0"
     "msel_goro_alt.sec\0"
     "msel_shao_kahn_alt.sec\0"
     "msel_ashrah_alt.sec\0"
     "msel_baraka_alt.sec\0"
     "msel_boraicho_alt.sec\0"
     "msel_dairou_alt.sec\0"
     "msel_darrius_alt.sec\0"
     "msel_ermac_alt.sec\0"
     "msel_havik_alt.sec\0"
     "msel_hotaru_alt.sec\0"
     "msel_jade_alt.sec\0"
     "msel_kabal_alt.sec\0"
     "msel_kenshi_alt.sec\0"
     "msel_kira_alt.sec\0"
     "msel_kobra_alt.sec\0"
     "msel_liu_kang_alt.sec\0"
     "msel_li_mei_alt.sec\0"
     "msel_mileena_alt.sec\0"
     "msel_nightwolf_alt.sec\0"
     "msel_raiden_alt.sec\0"
     "msel_scorpion_alt.sec\0"
     "msel_shujinko_alt.sec\0"
     "msel_sindel_alt.sec\0"
     "msel_smokenoob_alt.sec\0"
     "msel_subzero_alt.sec\0"
     "msel_tanya_alt.sec\0"
     "common/music_select/music_select\0"
     "NEWACCOUNTPASSWORD\0"
     "NEWACCOUNTNAME\0"
     "MSEL_PORTRAIT\0"
     "GC_A_BTN\0"
     "GC_X_BTN\0"
     "GC_B_BTN\0"
     "GC_Y_BTN\0"
     "GC_L_BTN\0"
     "GC_R_BTN\0"
     "GC_Z_BTN\0"
     "common/options/op_controllerconfig\0"
     "common/options/op_options\0"
     "Current scheme is: %d\n\0"
     "konquest/popups/k_pause_menu\0"
     "pause_menu/pause_menu\0"
     "common/main_menu/m_mode_select";

#define STR_SOUNDTRACK_SCREEN (&stringBase0[0x3BA])
#define STR_NEWACCOUNTPASSWORD (&stringBase0[0x3DB])
#define STR_NEWACCOUNTNAME (&stringBase0[0x3EE])
#define STR_MAIN_MENU_SCREEN (&stringBase0[0x4D1])

static ModeSelectPortrait portrait_list[] = {
    { &stringBase0[0x0], 0x1E },
    { &stringBase0[0xE], 0x1F },
    { &stringBase0[0x21], 0x7 },
    { &stringBase0[0x31], 0x1 },
    { &stringBase0[0x41], 0xA },
    { &stringBase0[0x53], 0x16 },
    { &stringBase0[0x63], 0x18 },
    { &stringBase0[0x74], 0x6 },
    { &stringBase0[0x83], 0xE },
    { &stringBase0[0x92], 0xB },
    { &stringBase0[0xA2], 0x15 },
    { &stringBase0[0xB0], 0x13 },
    { &stringBase0[0xBF], 0xC },
    { &stringBase0[0xCF], 0x12 },
    { &stringBase0[0xDD], 0x14 },
    { &stringBase0[0xEC], 0x10 },
    { &stringBase0[0xFE], 0x9 },
    { &stringBase0[0x10E], 0x4 },
    { &stringBase0[0x11F], 0x5 },
    { &stringBase0[0x132], 0x17 },
    { &stringBase0[0x142], 0x0 },
    { &stringBase0[0x154], 0x19 },
    { &stringBase0[0x166], 0x8 },
    { &stringBase0[0x176], 0x1B },
    { &stringBase0[0x189], 0x3 },
    { &stringBase0[0x19A], 0xF },
    { &stringBase0[0x1A9], 0x1001E },
    { &stringBase0[0x1BB], 0x1001F },
    { &stringBase0[0x1D2], 0x10007 },
    { &stringBase0[0x1E6], 0x10001 },
    { &stringBase0[0x1FA], 0x1000A },
    { &stringBase0[0x210], 0x10016 },
    { &stringBase0[0x224], 0x10018 },
    { &stringBase0[0x239], 0x10006 },
    { &stringBase0[0x24C], 0x1000E },
    { &stringBase0[0x25F], 0x1000B },
    { &stringBase0[0x273], 0x10015 },
    { &stringBase0[0x285], 0x10013 },
    { &stringBase0[0x298], 0x1000C },
    { &stringBase0[0x2AC], 0x10012 },
    { &stringBase0[0x2BE], 0x10014 },
    { &stringBase0[0x2D1], 0x10010 },
    { &stringBase0[0x2E7], 0x10009 },
    { &stringBase0[0x2FB], 0x10004 },
    { &stringBase0[0x310], 0x10005 },
    { &stringBase0[0x327], 0x10017 },
    { &stringBase0[0x33B], 0x10000 },
    { &stringBase0[0x351], 0x10019 },
    { &stringBase0[0x367], 0x10008 },
    { &stringBase0[0x37B], 0x1001B },
    { &stringBase0[0x392], 0x10003 },
    { &stringBase0[0x3A7], 0x1000F },
};

static int button_checklist[] = {7, 4, 6, 5, 2, 1, 3};

void adjust_screen_reset(void) {
    adjust_display_offset(0, 0, 1);
}

void adjust_screen_position(int direction) {
    switch (direction) {
    case 0:
        adjust_display_offset(-1, 0, 0);
        break;
    case 3:
        adjust_display_offset(0, 1, 0);
        break;
    case 2:
        adjust_display_offset(0, -1, 0);
        break;
    case 1:
        adjust_display_offset(1, 0, 0);
        break;
    }
}

int get_color_red_value(void) {
    return game_settings.color_red;
}

int get_color_green_value(void) {
    return game_settings.color_green;
}

int get_color_blue_value(void) {
    return game_settings.color_blue;
}

int get_gamma_value(void) {
    return game_settings.gamma;
}

int get_widescreen_state(void) {
    return widescreen_mode_on;
}

int get_progressive_scan_state(void) {
    return progressive_scan_on;
}

int get_contrast_value(void) {
    return game_settings.contrast;
}

int get_brightness_value(void) {
    return game_settings.display_brightness;
}

void push_video_settings(void) {
    set_gc_display_props(game_settings.display_brightness);
}

void reset_video_defaults(void) {
    game_settings.display_brightness = 0x32;
    game_settings.display_brightness = 0x32;
    set_gc_display_props(0x32);
}

void adjust_brightness(int delta) {
    int* brightness;
    int value;
    int next;

    brightness = &game_settings.display_brightness;
    value = *brightness;
    next = delta - value;
    if (value >= 0 && value <= 0x64) {
        *brightness = value + next * 5;
    }
    if (*brightness > 0x64) {
        *brightness = 0x64;
    }
    if (*brightness < 0) {
        *brightness = 0;
    }
    set_gc_display_props(*brightness);
}

void play_current_soundtrack(void) {
    struct SoundtrackScreenPdata* pdata;

        pdata = get_screen_pdata();
    if (pdata != 0) {
        snd_req(pdata->sound_ids[pdata->current]);
    }
}

const char* get_current_soundtrack_composer(void) {
    struct SoundtrackScreenPdata* pdata;

        pdata = get_screen_pdata();
    if (pdata != 0) {
        return pdata->composers[pdata->current];
    }
    return 0;
}

const char* get_current_soundtrack_description(void) {
    struct SoundtrackScreenPdata* pdata;

        pdata = get_screen_pdata();
    if (pdata != 0) {
        return pdata->descriptions[pdata->current];
    }
    return 0;
}

const char* get_current_soundtrack_title(void) {
    struct SoundtrackScreenPdata* pdata;

        pdata = get_screen_pdata();
    if (pdata != 0) {
        return pdata->titles[pdata->current].title;
    }
    return 0;
}

int get_current_soundtrack(void) {
    struct SoundtrackScreenPdata* pdata;

        pdata = get_screen_pdata();
    if (pdata != 0) {
        return pdata->current;
    }
    return 0;
}

void set_current_soundtrack(int index) {
    struct SoundtrackScreenPdata* pdata;

        pdata = get_screen_pdata();
    if (pdata != 0) {
        pdata->current = index;
    }
}

void get_soundtrack_title_list(const char*** titles_out, int* count_out, int* stride_out) {
    struct SoundtrackScreenPdata* pdata;

    pdata = get_screen_pdata();
    if (pdata != 0) {
        *count_out = pdata->title_count;
        *stride_out = 2;
        *titles_out = (const char**)pdata->titles;
    }
}

static inline void soundtrack_build_titles(struct SoundtrackScreenPdata* pdata) {
    static int track_list[] = {
        0x1B4F, 0x1B50, 0x1B51, 0x1B52, 0x1B53, 0x1B54, 0x1B55,
        0x1B56, 0x1B57, 0x1B58, 0x1B59, 0x1B5A, 0x1B5B, 0x1B5C,
        0x1B5D, 0x1B5E, 0x1B5F, 0x1B60, 0x1B61, 0x1B62, 0x1B63,
    };
    int track;

    pdata->title_count = track = 0;
    while (track < 21) {
        if ((gp_data.cat5 & (1 << track)) != 0) {
            pdata->sound_ids[pdata->title_count] = track_list[track];
            pdata->titles[pdata->title_count].meta =
                number_strings[pdata->title_count + 1];
            pdata->titles[pdata->title_count].title =
                get_string_by_id(0x10000 | (track + 0x2B));
            pdata->composers[pdata->title_count] =
                get_string_by_id(0x10000 | (track + 0x55));
            pdata->details[pdata->title_count] =
                get_string_by_id(0x10000 | (track + 0x40));
            pdata->descriptions[pdata->title_count] =
                get_string_by_id(0x10000 | (track + 0x6A));
            pdata->title_count++;
        }
        track++;
    }
}

float p_soundtrack(void) {
    struct SoundtrackScreenPdata* pdata;

    pdata = (struct SoundtrackScreenPdata*)get_mkpdata_generic(sizeof(struct SoundtrackScreenPdata));
    if (pdata == 0) {
        gamelogic_jump(6, p_main_menu);
    }
    mk_insert(&pdata->hdr, &aproc->pdata_list);
    zero_pdata_payload(sizeof(struct SoundtrackScreenPdata), &pdata->hdr);
    setup_sound_banks(9);
    wait_for_sound_banks_to_load();

    soundtrack_build_titles(pdata);

    push_game_state(9);
    load_screen(STR_SOUNDTRACK_SCREEN, 0x90046, &pdata->hdr, 1);
    turn_camera_on();
    turn_controllers_on();
    wait_for_screen_close();
    gamelogic_jump(6, p_main_menu);
    return sleep_ticks_neg_one;
}

const char* get_screens_online_options_newaccountpassword(void) {
    return STR_NEWACCOUNTPASSWORD;
}

const char* get_screens_online_options_newaccountname(void) {
    return STR_NEWACCOUNTNAME;
}

const char* get_p2_player_name(void) {
    if (p2_profile_status == 1) {
        return p2_profile.name;
    }
    return get_string_by_id(0x10003);
}

const char* get_p1_player_name(void) {
    if (p1_profile_status == 1) {
        return p1_profile.name;
    }
    return get_string_by_id(0x10002);
}

#pragma opt_common_subs off
void get_modeselect_portrait_list(GVTexturePair out) {
    *out.colors = load_named_tga_from_slot(0x90046, &stringBase0[0x3FD]);
    *out.alphas = load_named_alpha_texture_from_slot(0x90046, &stringBase0[0x3FD]);
}
#pragma opt_common_subs reset

int get_num_modeselect_portraits(void) {
    return 1;
}

void set_menu_mode(int mode) {
    if (mode > 3) {
        return;
    }
    menu_mode_net = mode;
}

#pragma optimize_for_size on
void controller_setup_save_to_profile(int player, int save) {
    MkProc* proc;
    int* rumble_on;
    int* temp_rumble;
    int* use_temp_map;
    SwitchMapEntry* temp_map;
    PlayerProfile* profile;
    int i;
    struct ControllerConfigPdata* pdata;

    if (player == 0) {
        profile = &p1_profile;
        rumble_on = &p1_rumble_on;
        temp_map = p1_temp_switch_map;
        use_temp_map = &p1_use_temp_switch_map;
        temp_rumble = &p1_temp_rumble_state;
    } else {
        profile = &p2_profile;
        rumble_on = &p2_rumble_on;
        temp_map = p2_temp_switch_map;
        use_temp_map = &p2_use_temp_switch_map;
        temp_rumble = &p2_temp_rumble_state;
    }

    proc = find_mkproc_pid(0x2001);
    if (proc != 0) {
        pdata = (struct ControllerConfigPdata*)pdata_of_proc(proc);
        if (player == 0) {
            pdata->p1_save = save;
        } else {
            pdata->p2_save = save;
        }
    }

    if (save != 0) {
        profile->rumble = *temp_rumble;
        *rumble_on = *temp_rumble;
        *use_temp_map = 0;
        for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
            profile->switch_map[i] = temp_map[i].mask;
        }
    } else {
        *use_temp_map = 1;
    }
}
#pragma optimize_for_size reset

void cconfig_get_button_textures(GVTexturePair out) {
    out.colors[0] = load_named_tga_from_slot(0x90046, &stringBase0[0x40B]);
    out.colors[1] = load_named_tga_from_slot(0x90046, &stringBase0[0x414]);
    out.colors[2] = load_named_tga_from_slot(0x90046, &stringBase0[0x41D]);
    out.colors[3] = load_named_tga_from_slot(0x90046, &stringBase0[0x426]);
    out.colors[6] = load_named_tga_from_slot(0x90046, &stringBase0[0x42F]);
    out.colors[4] = load_named_tga_from_slot(0x90046, &stringBase0[0x438]);
    out.colors[5] = load_named_tga_from_slot(0x90046, &stringBase0[0x441]);
}

int controller_get_texture_index_for_button(int player, int button) {
    int logical_button;
    int mapped_bit;

    logical_button = button_checklist[button];
    if (player == 0) {
        mapped_bit = p1_temp_switch_map[logical_button].mask;
    } else {
        mapped_bit = p2_temp_switch_map[logical_button].mask;
    }
    return controller_image_offset_tbl[find_bit(default_switch_map, mapped_bit)];
}

int controller_get_player_last_button(int player) {
    MkProc* proc;
    struct ControllerConfigPdata* pdata;

    proc = find_mkproc_pid(0x2001);
    if (proc != 0) {
        pdata = (struct ControllerConfigPdata*)pdata_of_proc(proc);
        if (pdata == 0) {
            return 0;
        }
        if (player == 0) {
            return pdata->p1_last_button;
        }
        return pdata->p2_last_button;
    }
    return 0;
}

#pragma optimize_for_size on
void cconfig_assign_button(int player, int button) {
    struct ControllerConfigPdata* pdata;
    int cell;
    SwitchMapEntry* temp_map;
    int* rumble_on;
    PlyrInfo* plyr;
    unsigned int new_bit;
    unsigned int old_bit;
    int old_index;

    pdata = (struct ControllerConfigPdata*)pdata_of_proc(aproc);
    if (pdata != 0) {
        if (player == 0) {
            plyr = &g_game_info.plyr0;
            cell = pdata->p1_cell;
            temp_map = p1_temp_switch_map;
            rumble_on = &p1_rumble_on;
        } else {
            plyr = &g_game_info.plyr1;
            cell = pdata->p2_cell;
            temp_map = p2_temp_switch_map;
            rumble_on = &p2_rumble_on;
        }

        if (cell == 7) {
            init_temp_switch_map(player, 0);
            p1_rumble_on = 0;
            p2_rumble_on = 0;
        }

        if (cell == 6) {
            if (default_switch_map[6].mask ==
                default_switch_map[button].mask) {
                if (is_rumble_available(plyr->pad_index) == 0) {
                    *rumble_on = 0;
                } else {
                    if (*rumble_on != 0) {
                        *rumble_on = 0;
                    } else {
                        *rumble_on = 1;
                        snd_req(0x1B3D);
                    }
                }
            }
            ck_rumble_controller(player, 10, 30);
        }

        if (cell < 6) {
            old_bit = temp_map[button_checklist[cell]].mask;
            new_bit = default_switch_map[button].mask;
            if (old_bit != new_bit) {
                old_index = find_bit(temp_map, new_bit);
                if (old_index >= 0) {
                    temp_map[old_index].mask = old_bit;
                }
                temp_map[button_checklist[cell]].mask = new_bit;
            }
        }
        fire_screen_studio_event(player + 0x1FAC, player);
    }
}
#pragma optimize_for_size reset

#pragma optimize_for_size on
void cconfig_set_current_cell(int player, int cell) {
    MkProc* proc;
    struct ControllerConfigPdata* pdata;

    proc = find_mkproc_pid(0x2001);
    if (proc != 0) {
        pdata = (struct ControllerConfigPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            if (player == 0) {
                pdata->p1_cell = cell - 1;
            } else {
                pdata->p2_cell = cell - 1;
            }
        }
    }
}
#pragma optimize_for_size reset

int controller_get_num_adjustable_buttons(void) {
    return 7;
}

void controller_setup_p2_state(int enabled) {
    if (enabled != 0) {
        init_temp_switch_map(1, 1);
        set_player_state(&g_game_info.plyr1, 1);
    } else {
        set_player_state(&g_game_info.plyr1, 0);
    }
}

void controller_setup_p1_state(int enabled) {
    if (enabled != 0) {
        init_temp_switch_map(0, 1);
        set_player_state(&g_game_info.plyr0, 1);
    } else {
        set_player_state(&g_game_info.plyr0, 0);
    }
}

/* TODO: [near miss] 98.554214%; retail base-plus-player walk agrees; NV register coloring remains. */
float p_controller_config(void) {
    struct ControllerConfigPdata* pdata;
    struct ControllerConfigPdata* live_pdata;
    MkProc* proc;
    PlyrInfo* p1;
    PlyrInfo* p2;
    struct MenuPlayerWalkView* players;
    struct MenuPlayerWalkView* player_view;
    int* button_ptr;
    int player;
    int player_offset;
    int button_index;

    pdata = (struct ControllerConfigPdata*)get_mkpdata_generic(sizeof(struct ControllerConfigPdata));
    if (pdata == 0) {
        gamelogic_jump(6, p_main_menu);
    }
    mk_insert(&pdata->hdr, &aproc->pdata_list);
    zero_pdata_payload(sizeof(struct ControllerConfigPdata), &pdata->hdr);

    p1 = &g_game_info.plyr0;
    unassign_player(p1);
    p2 = &g_game_info.plyr1;
    unassign_player(p2);
    if (menu_player == 1 && p1_profile_status == 1) {
        move_profile_p1_to_p2();
    }

    load_screen(&stringBase0[0x44A], 0x90046, 0, 1);

    proc = find_mkproc_pid(0x2001);
    if (proc != 0) {
        live_pdata = (struct ControllerConfigPdata*)pdata_of_proc(proc);
        if (live_pdata != 0) {
            init_temp_switch_map(0, 1);
            live_pdata->p1_last_button = 1;
            live_pdata->p2_last_button = 1;
            live_pdata->p1_cell = 1;
            live_pdata->p2_cell = 1;
        }
    }

    proc = find_mkproc_pid(0x2001);
    if (proc != 0) {
        live_pdata = (struct ControllerConfigPdata*)pdata_of_proc(proc);
        if (live_pdata != 0) {
            init_temp_switch_map(1, 1);
            live_pdata->p1_last_button = 1;
            live_pdata->p2_last_button = 1;
            live_pdata->p1_cell = 1;
            live_pdata->p2_cell = 1;
        }
    }

    turn_camera_on();
    turn_controllers_on();
    push_game_state(0xE);
    target_game_mode = MENU_TARGET_IDLE;
    players = (struct MenuPlayerWalkView*)&g_game_info;

    while (target_game_mode == MENU_TARGET_IDLE) {
        player = 0;
        player_offset = 0;
        do {
            player_view = (struct MenuPlayerWalkView*)((char*)players + player_offset);
            if (player_view->player.player_state == 1) {
                int pad;

                pad = player_view->player.pad_index;
                button_index = 0;
                button_ptr = button_checklist;
                do {
                    if (check_switch_edge(pad, *button_ptr) != 0) {
                        if (player == 0) {
                            pdata->p1_last_button = button_index + 1;
                        } else {
                            pdata->p2_last_button = button_index + 1;
                        }
                        cconfig_assign_button(player, *button_ptr);
                        break;
                    }
                    button_index++;
                    button_ptr++;
                } while (button_index < 7);
            }
            player++;
            player_offset += sizeof(PlyrInfo);
        } while (player < 2);
        _mkproc_sleep_ticks = sleep_ticks_one;
        aproc->vtbl->sleep();
    }

    wait_for_screen_close();
    if (pdata->p1_save != 0) {
        save_profile(0, 1);
    }
    if (pdata->p2_save != 0) {
        save_profile(1, 1);
    }
    set_default_switch_map(p1);
    set_default_switch_map(p2);
    gamelogic_jump(6, p_main_menu);
    set_player_state(p1, 0);
    set_player_state(p2, 0);
    pop_game_state();
    return sleep_ticks_neg_one;
}

/* TODO: [near miss] 86.98%; algorithm exact; branch scheduling remains. */
float p_version_code(void) {
    struct VersionCodePdata* pdata;
    StringObj* text;

    pdata = (struct VersionCodePdata*)apdata;
    if (check_switch(0, 7) != 0 && check_switch(0, 2) != 0) {
        pdata->ticks++;
        if (pdata->ticks < 0x168) {
            return sleep_ticks_one;
        }
        load_font(0);
        if (is_widescreen_mode() != 0) {
            text = string_left_xy(0, 0, mk6_version_string,
                                  ((screen_width - 0x280) / 2) + 0x208, 0x1E, 1);
        } else {
            text = string_left_xy(0, 0, mk6_version_string, 0x208, 0x1E, 1);
        }
        if (text != 0) {
            text->visibility.keep_when_suppress = 1;
        }
        return sleep_ticks_neg_one;
    }
    pdata->ticks = 0;
    return sleep_ticks_one;
}

float p_game_options(void) {
    GameSettings saved_settings;
    struct VersionCodePdata* version_pdata;
    struct OptionsSoundPdata* sound_pdata;
    MslSoundHandle sound_handle;

    memory_move_game_setting(&saved_settings, &game_settings);
    get_menu_mode_sub_var();
    load_screen(&stringBase0[0x46D], 0x90046, 0, 1);
    turn_camera_on();
    turn_controllers_on();
    push_game_state(9);
    set_button_repeat_time(6);

    if (_create_mkproc_generic_nostack(0x20A1, 0x1F, p_version_code,
                                       sizeof(struct VersionCodePdata),
                                       (MkHdr**)&version_pdata) != 0) {
        version_pdata->ticks = 0;
    }

    sound_handle = snd_req(0x1C0A);
    sound_pdata = (struct OptionsSoundPdata*)get_mkpdata_generic(sizeof(struct OptionsSoundPdata));
    if (sound_pdata != 0) {
        mk_insert(&sound_pdata->hdr, &aproc->pdata_list);
        sound_pdata->sound_handle = sound_handle;
    }

    wait_for_screen_close();
    pop_game_state();
    selected_refresh_rate = refresh_rate();
    if (get_gameoption_exitwithsave() != 0) {
        save_game_settings();
    } else {
        memory_move_game_setting(&game_settings, &saved_settings);
        set_gc_display_props(game_settings.display_brightness);
    }
    turn_controllers_off();
    set_default_button_repeat_time();
    snd_stop(sound_handle);
    gamelogic_jump(6, p_main_menu);
    return sleep_ticks_neg_one;
}

float p_pause_menu(void) {
    MkProcEntryFn next_proc;
    struct PauseMenuPdata* pdata;
    MkVtableMkproc* vtbl;
    unsigned int scheme;
    int screen_slot;
    const char* screen_name;
    int jump_mode;

    pdata = (struct PauseMenuPdata*)apdata;
    next_proc = 0;
    jump_mode = 6;

    eat_switch_edge(pdata->player, 0xB);
    g_game_info.switch_input_flags.eat_switches = 1;
    pause_player = g_game_info.pads[pdata->player].player->field_04;
    set_default_switch_maps();

    scheme = get_current_section_memory_scheme();
    switch (scheme) {
    case 1:
        screen_slot = 0x60026;
        break;
    case 2:
        screen_slot = 0x70039;
        break;
    case 7:
        screen_slot = 0x150068;
        break;
    case 3:
        screen_slot = 0x11005C;
        break;
    case 4:
    case 9:
        screen_slot = 0x90046;
        break;
    case 10:
        screen_slot = 0x17006A;
        break;
    case 0:
    case 5:
    case 8:
        screen_slot = 0x50014;
        break;
    case 6:
    default:
        vdebug_print_message(&stringBase0[0x487], scheme);
        screen_slot = 0;
        break;
    }

    switch (mode_of_play) {
    case 7:
        screen_name = &stringBase0[0x49E];
        break;
    default:
        screen_name = &stringBase0[0x4BB];
        break;
    }
    load_screen(screen_name, screen_slot, 0, 0);
    pause_all_game_sounds();

    if (g_game_info.feature_flags.bits.high_bit != 0) {
        g_game_info.field_214 = 0x1E;
    }

    while (find_mkproc_pid(0x9011) != 0) {
        if (g_game_info.feature_flags.bits.high_bit == 0) {
            pause_procs(1);
        } else {
            g_game_info.pause_flag_bits.controllers_disabled = 0;
        }
        _mkproc_sleep_ticks = sleep_ticks_one;
        vtbl = aproc->vtbl;
        vtbl->sleep();
    }

    g_game_info.switch_input_flags.eat_switches = 0;
    switch (target_game_mode) {
    case 5:
        pop_game_state();
        push_game_state(9);
        next_proc = p_main_menu;
        break;
    case 6:
    case 7:
    case 11:
        next_proc = p_pselect;
        break;
    case 8:
        next_proc = p_bg_pselect;
        break;
    case 9:
        next_proc = p_pz_pselect;
        break;
    case 2:
        jump_mode = 4;
        next_proc = p_konquest_mode;
        break;
    default:
        break;
    }

    flush_controller_switch_buffers();
    eat_switch_edge(pdata->player, 0xB);
    if (g_game_info.feature_flags.bits.high_bit != 0) {
        eat_switch_edge(pdata->player, 6);
        eat_switch_edge(pdata->player, 4);
        eat_switch_edge(pdata->player, 5);
    } else {
        _mkproc_sleep_ticks = sleep_ticks_one;
        vtbl = aproc->vtbl;
        vtbl->sleep();
    }

    pause_procs(0);
    unpause_all_game_sounds();
    if (next_proc != 0) {
        if (target_game_mode != 2 && game_save_loop_count != 0) {
            ck_do_profile_save();
        }
        if (__mini_game_display_ctrl != 0) {
            xfer_puzzle_exit(1);
        }
        gamelogic_jump(jump_mode, next_proc);
    }

    if (pdata->was_paused != 0) {
        g_game_info.pause_flag_bits.controllers_disabled = 1;
    }
    set_game_switch_maps();
    return sleep_ticks_neg_one;
}

const char* get_pause_menu_name(void) {
    switch (mode_of_play) {
    case 7:
        return &stringBase0[0x49E];
    default:
        return &stringBase0[0x4BB];
    }
}

int get_pause_menu_ssh(void) {
    int scheme;
    int slot;

    scheme = get_current_section_memory_scheme();
    switch (scheme) {
    case 1:
        slot = 0x60026;
        break;
    case 2:
        slot = 0x70039;
        break;
    case 7:
        slot = 0x150068;
        break;
    case 3:
        slot = 0x11005C;
        break;
    case 4:
    case 9:
        slot = 0x90046;
        break;
    case 10:
        slot = 0x17006A;
        break;
    case 0:
    case 5:
    case 8:
        slot = 0x50014;
        break;
    case 6:
    default:
        vdebug_print_message(&stringBase0[0x487], scheme);
        slot = 0;
        break;
    }
    return slot;
}

static inline int pause_menu_allowed(void) {
    if (find_mkproc_pid(0x9011) != 0) {
        return 0;
    }
    if (find_mkproc_pid(0x208B) != 0) {
        return 0;
    }
    if (mode_of_play == 6) {
        if ((int)display_off != 0) {
            return 0;
        }
        if (g_game_info.flag_bits.high_res_path != 0) {
            return 0;
        }
    }
    return 1;
}

float p_pause_menu_switch(void) {
    struct PauseMenuPdata* pdata;
    MkProc* proc;
    int can_pause;
    int player;
    int was_paused;

    if (switch_pdata->player->player_state != 2) {
        return sleep_ticks_neg_one;
    }

    can_pause = pause_menu_allowed();

    if (can_pause != 0) {
        player = switch_pdata->player->pad_index;
        proc = _create_mkproc_generic_bigstack(0x208B, 0x1F, p_pause_menu,
                                               sizeof(struct PauseMenuPdata), (MkHdr**)&pdata);
        if (proc != 0) {
            proc->flags_bits.skip_if_paused = 1;
            pdata->player = player;
            was_paused = g_game_info.pause_flag_bits.controllers_disabled;
            turn_controllers_on();
            pdata->was_paused = was_paused;
            if (g_game_info.feature_flags.bits.high_bit == 0) {
                pause_procs(1);
            }
        }
    }
    return sleep_ticks_neg_one;
}

/* TODO: [near miss] 92.97%; we hoist g_game_info+0x110 where retail rematerializes it;
 * retail names portrait_list$0000 (function-local static?), check before coloring. */
float p_main_menu(void) {
    PlyrInfo* p1;
    PlyrInfo* p2;
    unsigned int mode;
    int tries;
    int portrait;
    unsigned int flags;
    struct ControllerWatcherPdata* watcherPdata;
    MkVtableMkproc* vtbl;
    unsigned int switchTime;
    PlyrInfo* konquestPlyr;
    int konquestPort;

    disc_error_occurred = 0;
    online_locked_port = -1;
    lan_networking_selected = 0;
    switchTime = 0;

    set_section_memory_scheme(4);

    mode = get_mode_of_play();
    if (mode <= 0xC) {
        switch (mode) {
        case 0:
        case 1:
        case 6:
        case 9:
        case 10:
        case 12:
            if (p1_profile_status == 1 && p2_profile_status == 1) {
                unload_player_profiles();
            }
            break;
        default:
            break;
        }
    }

    if (p1_profile_status == 1) {
        if (p2_profile_status == 1) {
            unload_p2_player_profile();
        }
    } else if (p2_profile_status == 1) {
        move_profile_p2_to_p1();
    }

    set_mode_of_play(0xD);
    push_game_state(9);
    one_player_ladder_init();

    p1 = &g_game_info.plyr0;
    p2 = &g_game_info.plyr1;
    unassign_player(p1);
    unassign_player(p2);
    set_player_state(p1, 0);
    set_player_state(p2, 0);
    g_game_info.feature_flags.bits.high_bit = 0;
    init_plyr_info_struct(p1);
    init_plyr_info_struct(p2);
    init_bet_info_struct();

    setup_sound_banks(1);
    wait_for_sound_banks_to_load();
    unload_section_slot(0x90046);

    tries = 0x78;
    portrait = 0;
    do {
        int lockedFlags;
        int alternate;

        portrait = randu0(0x34);
        flags = portrait_list[portrait & 0xFFFF].flags;
        lockedFlags = flags & 0xFFFEFFFF;
        alternate = (flags >> 16) & 1;
        if (is_char_locked(lockedFlags, alternate) == 0) {
            break;
        }
        tries--;
    } while (tries > 0);
    if (tries == 0) {
        portrait = 0;
    }

    load_ssf((MkFileEntry*)msel_art_file_table);
    add_art_section_by_name_async(0x90046, portrait_list[portrait & 0xFFFF].sec_name);

    watcherPdata = 0;
    _create_mkproc_generic_nostack(0x902F, 0x1F, p_controller_watcher,
                                   sizeof(struct ControllerWatcherPdata),
                                   (MkHdr**)&watcherPdata);
    if (watcherPdata != 0) {
        watcherPdata->last_num_controllers = get_num_controllers();
    }

    preload_screen_data(STR_MAIN_MENU_SCREEN, 0x90046);
    load_screen(STR_MAIN_MENU_SCREEN, 0x90046, 0, 0);
    turn_camera_on();
    turn_controllers_on();

    main_menu_timeout_ticks = 0xE10;
    do_main_menu_timeout = 1;
    target_game_mode = MENU_TARGET_IDLE;

    for (;;) {
        mode = target_game_mode;
        if (mode != MENU_TARGET_IDLE) {
            pop_game_state();
            push_game_state(0x1C);
            wait_for_screen_close();

            if (mode > 0x17) {
                gamelogic_jump(0, p_attract_mode);
            } else {
                switch (mode) {
                case 2:
                    game_settings.konquest_loading_image = 0;
                    set_mode_of_play(7);
                    clear_region_buffer();
                    konquestPlyr = (menu_player == 0) ? &g_game_info.plyr0 : &g_game_info.plyr1;
                    konquestPort = konquestPlyr->pad_index;
                    unassign_player(p1);
                    unassign_player(p2);
                    assign_player(konquestPort);
                    load_krd_buffer_from_memcard(0, 1);
                    gamelogic_jump(4, p_konquest_mode);
                    break;
                case 4:
                    gamelogic_jump(6, p_main_menu);
                    break;
                case 6:
                    set_mode_of_play(0);
                    if (menu_player == 1 && p1_profile_status == 1) {
                        move_profile_p1_to_p2();
                    }
                    gamelogic_jump(1, p_pselect);
                    break;
                case 7:
                    set_mode_of_play(1);
                    if (menu_player == 1 && p1_profile_status == 1) {
                        move_profile_p1_to_p2();
                    }
                    gamelogic_jump(1, p_pselect);
                    break;
                case 11:
                    set_mode_of_play(4);
                    if (menu_player == 1 && p1_profile_status == 1) {
                        move_profile_p1_to_p2();
                    }
                    gamelogic_jump(1, p_pselect);
                    break;
                case 9:
                    set_mode_of_play(6);
                    if (menu_player == 1 && p1_profile_status == 1) {
                        move_profile_p1_to_p2();
                    }
                    one_player_ladder_init();
                    gamelogic_jump(6, p_pz_pselect);
                    break;
                case 8:
                    set_mode_of_play(9);
                    if (menu_player == 1 && p1_profile_status == 1) {
                        move_profile_p1_to_p2();
                    }
                    gamelogic_jump(6, p_bg_pselect);
                    break;
                case MENU_TARGET_OPTIONS:
                    gamelogic_jump(6, p_game_options);
                    break;
                case 14:
                    gamelogic_jump(6, p_controller_config);
                    break;
                case 16:
                    gamelogic_jump(6, p_kontent);
                    break;
                case 17:
                    gamelogic_jump(7, p_krypt_mode);
                    break;
                case MENU_TARGET_CREATE_PROFILE:
                    gamelogic_jump(6, p_create_profile);
                    break;
                case MENU_TARGET_VIEW_PROFILE:
                    gamelogic_jump(6, p_view_profile);
                    break;
                case MENU_TARGET_DELETE_PROFILE:
                    gamelogic_jump(6, p_delete_profile);
                    break;
                case 21:
                case 22:
                    gamelogic_jump(6, p_soundtrack);
                    break;
                case 23:
                    gamelogic_jump(6, p_credits_screen);
                    break;
                case 5:
                    gamelogic_jump(6, p_main_menu);
                    break;
                default:
                    gamelogic_jump(0, p_attract_mode);
                    break;
                }
            }
        }

        if (last_switch_time != (int)switchTime) {
            switchTime = last_switch_time;
            main_menu_timeout_ticks = 0xE10;
        }

        if (do_main_menu_timeout != 0) {
            main_menu_timeout_ticks--;
            if (main_menu_timeout_ticks <= 0) {
                atm_reset_current_page(1);
                gamelogic_jump(0, p_attract_mode);
            }
        }

        if (disc_error_occurred != 0) {
            broadcast_screen_studio_event(0x23F2, 1);
            disc_error_occurred = 0;
        }

        _mkproc_sleep_ticks = sleep_ticks_one;
        vtbl = aproc->vtbl;
        vtbl->sleep();
    }
}

static float p_controller_watcher(void) {
    struct ControllerWatcherPdata* pdata;
    int count;

    pdata = (struct ControllerWatcherPdata*)apdata;
    count = get_num_controllers();
    if (count != pdata->last_num_controllers) {
        pdata->last_num_controllers = count;
        fire_screen_studio_event(0x1FED, 0);
    }
    return sleep_ticks_one;
}

void menu_init(void) {
}
