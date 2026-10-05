#include "game/controller.h"

#include "game/game_info.h"
#include "game/mcardmsg.h"
#include "game/plyrprofile.h"
#include "game/switch.h"
#include "game/trial.h"
#include "platform/display.h"
#include "platform/gcio.h"
#include "platform/io.h"
#include "platform/main.h"
#include "runtime/fonts.h"
#include "runtime/image.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_vtbl.h"
#include "runtime/utils.h"
#include "runtime/cstdio.h"
#include "platform/display_metrics.h"

#define RUMBLE_PROC_PID 0x2064
#define CONTROLLER_FADEBOX_OID 0x2081
#define CONTROLLER_ACCEPT_SWITCH 0xB
#define CONTROLLER_SCREEN_CENTER ((screen_width - 0x280) / 2)

struct RumblePdata {
    MkHdr hdr;
    int port;
    int strength;
    int ticks;
};

SwitchMapEntry default_switch_map[16] = {
    {0x0001, pad_l2_proc, "PAD_L2"},
    {0x0002, pad_r2_proc, "PAD_R2"},
    {0x0004, pad_l1_proc, "PAD_L1"},
    {0x0008, pad_r1_proc, "PAD_R1"},
    {0x0010, pad_rup_proc, "PAD_RUP"},
    {0x0020, pad_rrt_proc, "PAD_RRT"},
    {0x0040, pad_rdn_proc, "PAD_RDN"},
    {0x0080, pad_rlt_proc, "PAD_RLT"},
    {0x0100, pad_select_proc, "PAD_SELECT"},
    {0x0200, pad_lt_stick_btn_proc, "PAD_LAS_BTN"},
    {0x0400, pad_rt_stick_btn_proc, "PAD_RAS_BTN"},
    {0x0800, pad_start_proc, "PAD_START"},
    {0x1000, pad_lup_proc, "PAD_LUP"},
    {0x2000, pad_lrt_proc, "PAD_LRT"},
    {0x4000, pad_ldn_proc, "PAD_LDN"},
    {0x8000, pad_llt_proc, "PAD_LLT"}
};
extern int p1_profile_status;
extern int p2_profile_status;
SwitchMapEntry p2_profile_switch_map[PROFILE_SWITCHMAP_COUNT];
SwitchMapEntry p1_profile_switch_map[PROFILE_SWITCHMAP_COUNT];
SwitchMapEntry p2_temp_switch_map[PROFILE_SWITCHMAP_COUNT];
SwitchMapEntry p1_temp_switch_map[PROFILE_SWITCHMAP_COUNT];
extern int menu_player;
extern int sounds_muted;
extern void mute_all_game_sounds(void);
extern void unmute_all_game_sounds(void);

static float p_rumble_controller(void);
static float p_do_controller_removed(void);

struct ControllerRemovedPdata {
    MkHdr hdr;
    int port;
    int controllers_disabled;
};

struct ControllerScreenObjRef {
    ScreenObj* object;
    int instance;
};

int p1_use_temp_switch_map;
int p2_use_temp_switch_map;
static struct ControllerScreenObjRef cnt_rem_fadebox_item;
int p2_temp_rumble_state;
int p1_temp_rumble_state;
int p2_rumble_on;
int p1_rumble_on;

#define DRAW_CONTROLLER_REMOVED_TEXT(screen_oid, player_x, port_number, text_buffer) \
    do {                                                                            \
        string_center_xy((screen_oid), 3, get_string(0x1D),                          \
                         CONTROLLER_SCREEN_CENTER + 0x140, 0x154, 0);                \
        string_center_xy((screen_oid), 0, get_string(0x1E),                          \
                         (player_x) + 0xA0 + CONTROLLER_SCREEN_CENTER, 0x122, 0);     \
        string_center_xy((screen_oid), 0, get_string(0x1F),                          \
                         (player_x) + 0xA0 + CONTROLLER_SCREEN_CENTER, 0x10E, 0);     \
        string_center_xy((screen_oid), 0, get_string(0x20),                          \
                         (player_x) + 0xA0 + CONTROLLER_SCREEN_CENTER, 0xFA, 0);      \
        sprintf((text_buffer), get_string(0x21), (port_number) + 1);                 \
        string_center_xy((screen_oid), 0, (text_buffer),                             \
                         (player_x) + 0xA0 + CONTROLLER_SCREEN_CENTER, 0xE6, 0);      \
        string_center_xy((screen_oid), 0, get_string(0x22),                          \
                         (player_x) + 0xA0 + CONTROLLER_SCREEN_CENTER, 0xD2, 0);      \
        string_center_xy((screen_oid), 0, get_string(0x23),                          \
                         (player_x) + 0xA0 + CONTROLLER_SCREEN_CENTER, 0xBE, 0);      \
    } while (0)

int find_bit(const SwitchMapEntry* switch_map, unsigned int bit) {
    int i;

    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        if (switch_map[i].mask == bit) {
            return i;
        }
    }
    return -1;
}

const char* get_controller_vibration_string(int player) {
    if (player == 0) {
        if (p1_rumble_on != 0) {
            p1_temp_rumble_state = 1;
            return get_string(0x95);
        }
        p1_temp_rumble_state = 0;
        return get_string(0x96);
    }
    if (p2_rumble_on != 0) {
        p2_temp_rumble_state = 1;
        return get_string(0x95);
    }
    p2_temp_rumble_state = 0;
    return get_string(0x96);
}

void turn_all_rumble_motors_off(void) {
    MkProc* proc;

    proc = find_mkproc_pid(RUMBLE_PROC_PID);
    if (proc != 0 && proc->instance != 0) {
        proc->vtbl->destroy(proc);
    }
    turn_rumble_off(0);
    turn_rumble_off(1);
    turn_rumble_off(2);
    turn_rumble_off(3);
}

static float p_rumble_controller(void) {
    struct RumblePdata* pdata;

    pdata = (struct RumblePdata*)apdata;
    if (pdata != 0) {
        turn_rumble_on(pdata->port, pdata->strength);
        _mkproc_sleep_ticks = pdata->ticks;
        aproc->vtbl->sleep();
        turn_rumble_off(pdata->port);
    }
    return 0.0f;
}

void ck_rumble_controller(int player, int strength, int ticks) {
    int game_state;
    int port;
    struct RumblePdata* pdata;

    game_state = get_game_state();
    if (player == 0) {
        if (p1_rumble_on == 0) {
            return;
        }
        if (g_game_info.plyr0.player_state != 2 && game_state != 0xE) {
            return;
        }
        port = g_game_info.plyr0.pad_index;
    } else {
        if (p2_rumble_on == 0) {
            return;
        }
        if (g_game_info.plyr1.player_state != 2 && game_state != 0xE) {
            return;
        }
        port = g_game_info.plyr1.pad_index;
    }

    if (is_rumble_available(port) == 0) {
        return;
    }
    if (_create_mkproc_generic_tinystack(
            RUMBLE_PROC_PID, 0x1F, p_rumble_controller, sizeof(struct RumblePdata), (MkHdr**)&pdata) == 0) {
        return;
    }

    pdata->port = port;
    pdata->strength = strength;
    pdata->ticks = ticks;
}

/* TODO: [near miss] 98.15%; locked flag/unpause test materialize as inline-return r0 (cmpwi) in retail; pdata/pad r27/r28 swap. */
static float p_do_controller_removed(void) {
    struct ControllerRemovedPdata* pdata;
    PlyrInfo* player;
    ScreenObj* fadebox;
    int port;
    int player_side;
    int player_pad;
    int player_x;
    int screen_oid;
    int other_proc_pid;
    int controllers_locked;
    char initial_text[0x50];
    char retry_text[0x50];

    load_font(0);
    load_font(3);

    pdata = (struct ControllerRemovedPdata*)apdata;
    port = pdata->port;
    if (port < 0) {
        return -1.0f;
    }
    if (port > 3) {
        return -1.0f;
    }

    while (g_game_info.flag_bits.high_res_path || display_off != 0) {
        if (g_game_info.pads[port].flag_bits.connected) {
            unmute_all_game_sounds();
            return -1.0f;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    switch (get_game_state()) {
    case 0:
    case 2:
    case 3:
    case 9:
    case 12:
        controllers_locked = 0;
        break;
    default:
        controllers_locked = 1;
        break;
    }
    if (controllers_locked == 0) {
        unmute_all_game_sounds();
        return -1.0f;
    }

    player = g_game_info.pads[port].player;
    if (player == 0) {
        return -1.0f;
    }

    player_side = player->field_04;
    if (player_side == 0) {
        player_pad = g_game_info.plyr0.pad_index;
        player_x = 0;
        screen_oid = 0x207F;
        other_proc_pid = 0x2066;
    } else if (player_side == 1) {
        player_pad = g_game_info.plyr1.pad_index;
        player_x = 0x140;
        screen_oid = 0x2080;
        other_proc_pid = 0x2065;
    } else {
        return -1.0f;
    }

    fadebox = MK_LIVE(cnt_rem_fadebox_item.object, cnt_rem_fadebox_item.instance);
    if (fadebox == 0) {
        fadebox = load_2d_pfxobj(0, CONTROLLER_FADEBOX_OID, 0x10017, 0, 3);
        if (fadebox != 0) {
            cnt_rem_fadebox_item.object = fadebox;
            cnt_rem_fadebox_item.instance = fadebox->instance;
            fadebox->x = -0x32;
            fadebox->y = -0x32;
            fadebox->flag_bits.scaled = 1;
            fadebox->flag_bits.bit1 = 1;
            fadebox->scale_x = 50.0f;
            fadebox->scale_y = 40.0f;
            pfx_2d_obj_set_alpha(fadebox, 0xA5);
        }
    }

    DRAW_CONTROLLER_REMOVED_TEXT(screen_oid, player_x, player_pad, initial_text);
    if (!g_game_info.feature_flags.bits.high_bit) {
        pause_procs(1);
    }

    while (!check_switch_edge(player_pad, CONTROLLER_ACCEPT_SWITCH)) {
        fadebox = MK_LIVE(cnt_rem_fadebox_item.object, cnt_rem_fadebox_item.instance);
        if (fadebox == 0) {
            fadebox = load_2d_pfxobj(0, CONTROLLER_FADEBOX_OID, 0x10017, 0, 3);
            if (fadebox != 0) {
                cnt_rem_fadebox_item.object = fadebox;
                cnt_rem_fadebox_item.instance = fadebox->instance;
                fadebox->x = -0x32;
                fadebox->y = -0x32;
                fadebox->flag_bits.scaled = 1;
                fadebox->flag_bits.bit1 = 1;
                fadebox->scale_x = 50.0f;
                fadebox->scale_y = 40.0f;
                pfx_2d_obj_set_alpha(fadebox, 0xA5);
            }
            DRAW_CONTROLLER_REMOVED_TEXT(screen_oid, player_x, player_pad, retry_text);
        }

        switch (get_game_state()) {
        case 0:
        case 2:
        case 3:
        case 9:
        case 12:
            controllers_locked = 0;
            break;
        default:
            controllers_locked = 1;
            break;
        }
        if (controllers_locked == 0) {
            break;
        }

        if (!g_game_info.feature_flags.bits.high_bit) {
            if (display_off == 0) {
                pause_procs(1);
            } else {
                pause_procs(0);
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }

    init_controller();
    if (!is_mcardmsg_active() && find_mkproc_pid(0x208B) == 0) {
        pause_procs(0);
    }
    delete_screen_obj_oid(screen_oid);
    del_string_obj_by_id(screen_oid);
    flush_controller_switch_buffers();
    eat_switch_edge(player_pad, CONTROLLER_ACCEPT_SWITCH);

    if (pdata->controllers_disabled != 0) {
        g_game_info.pause_flag_bits.controllers_disabled = 1;
    }

    if (find_mkproc_pid(other_proc_pid) == 0) {
        delete_screen_obj_oid(CONTROLLER_FADEBOX_OID);
        cnt_rem_fadebox_item.object = 0;
        cnt_rem_fadebox_item.instance = 0;
        unmute_all_game_sounds();
    }
    return -1.0f;
}

void update_pause_menu_controller_state(void) {
    MkProc* proc;
    struct ControllerRemovedPdata* pdata;

    proc = find_mkproc_pid(0x208B);
    if (proc != 0) {
        pdata = (struct ControllerRemovedPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->controllers_disabled = (g_game_info.pause_flags >> 1) & 1;
        }
    }
}

int is_controller_removed(void) {
    int removed;

    if (find_mkproc_pid(0x2065) != 0 || find_mkproc_pid(0x2066) != 0) {
        removed = 1;
    } else {
        removed = 0;
    }
    return removed;
}

void update_cnt_removed_controller_state(void) {
    MkProc* proc;
    struct ControllerRemovedPdata* pdata;

    proc = find_mkproc_pid(0x2065);
    if (proc != 0) {
        pdata = (struct ControllerRemovedPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->controllers_disabled = (g_game_info.pause_flags >> 1) & 1;
        }
    }

    proc = find_mkproc_pid(0x2066);
    if (proc != 0) {
        pdata = (struct ControllerRemovedPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->controllers_disabled = (g_game_info.pause_flags >> 1) & 1;
        }
    }
}

void controller_removed(int port) {
    MkProc* proc;
    struct ControllerRemovedPdata* pdata;
    PlyrInfo* player;
    int pid;

    proc = find_mkproc_pid(RUMBLE_PROC_PID);
    if (proc != 0 && proc->instance != 0) {
        proc->vtbl->destroy(proc);
    }
    turn_rumble_off(0);
    turn_rumble_off(1);
    turn_rumble_off(2);
    turn_rumble_off(3);

    player = g_game_info.pads[port].player;
    pid = 0x2066;
    if (player->field_04 == 0) {
        pid = 0x2065;
    }
    if (find_mkproc_pid(pid) == 0) {
        proc = _create_mkproc_generic_bigstack(
            pid, 4, p_do_controller_removed, sizeof(struct ControllerRemovedPdata), (MkHdr**)&pdata);
        if (proc != 0) {
            pdata->port = port;
            pdata->controllers_disabled = g_game_info.pause_flag_bits.controllers_disabled;
            proc->flags_bits.skip_if_paused = 1;
            if (sounds_muted == 0) {
                mute_all_game_sounds();
            }
        }
    }
}

void ck_for_controller_removed(void) {
    int port;

    if (g_game_info.plyr0.player_state != 0) {
        port = g_game_info.plyr0.pad_index;
        if (port >= 0 && !g_game_info.pads[port].flag_bits.connected) {
            controller_removed(g_game_info.plyr0.pad_index);
        }
    }

    if (g_game_info.plyr1.player_state != 0) {
        port = g_game_info.plyr1.pad_index;
        if (port >= 0 && !g_game_info.pads[port].flag_bits.connected) {
            controller_removed(g_game_info.plyr1.pad_index);
        }
    }
}

void dispatch_right_sticks(int port) {
    float x;
    float y;

    if (g_game_info.pads[port].flag_bits.connected &&
        get_stick_pos(port, 1, &x, &y) != 0 && y > 0.0f) {
        g_game_info.pads[port].buttons |= g_game_info.pads[port].switch_map[0].mask;
    }
}

void dispatch_pad_sticks(int port) {
    float x;
    float y;

    if (g_game_info.pads[port].flag_bits.connected &&
        get_stick_pos(port, 0, &x, &y) != 0) {
        if (x < 0.0f) {
            g_game_info.pads[port].buttons |= g_game_info.pads[port].switch_map[15].mask;
        }
        if (x > 0.0f) {
            g_game_info.pads[port].buttons |= g_game_info.pads[port].switch_map[13].mask;
        }
        if (y < 0.0f) {
            g_game_info.pads[port].buttons |= g_game_info.pads[port].switch_map[12].mask;
        }
        if (y > 0.0f) {
            g_game_info.pads[port].buttons |= g_game_info.pads[port].switch_map[14].mask;
        }
    }
}

int are_controllers_locked(void) {
    switch (get_game_state()) {
    case 0:
    case 2:
    case 3:
    case 9:
    case 12:
        return 0;
    default:
        return 1;
    }
}

int assign_player(int port) {
    PlyrInfo* player;
    int old_port;
    int removed_proc_active;

    if (g_game_info.pads[port].flag_bits.connected == 0) {
        return 0;
    }
    if (g_game_info.pads[port].player != 0 && are_controllers_locked() != 0) {
        return 0;
    }
    if (find_mkproc_pid(0x2065) != 0 || find_mkproc_pid(0x2066) != 0) {
        removed_proc_active = 1;
    } else {
        removed_proc_active = 0;
    }
    if (removed_proc_active != 0) {
        return 0;
    }

    player = get_player_for_port(port);
    if (player == 0) {
        return 0;
    }
    if (player->pad_index >= 0 && are_controllers_locked() != 0) {
        return 0;
    }
    if (player->player_state == 3) {
        return 0;
    }

    if (player != 0 && port >= 0) {
        old_port = player->pad_index;
        if (old_port != -1 && player != 0) {
            if (old_port > -1) {
                g_game_info.pads[old_port].player = 0;
                flush_controller_switch_buffers();
            }
            if (player->pad_index == 2) {
                g_game_info.pads[player->pad_index].flag_bits.connected = 0;
            }
            player->pad_index = -1;
            if (g_game_info.field_1F8 > 0) {
                g_game_info.field_1F8--;
            }
        }

        if (g_game_info.field_1F8 < 2) {
            g_game_info.field_1F8++;
        }

        old_port = player->pad_index;
        if (old_port > 0 && old_port != port && player != 0) {
            if (old_port > -1) {
                g_game_info.pads[old_port].player = 0;
                flush_controller_switch_buffers();
            }
            if (player->pad_index == 2) {
                g_game_info.pads[player->pad_index].flag_bits.connected = 0;
            }
            player->pad_index = -1;
            if (g_game_info.field_1F8 > 0) {
                g_game_info.field_1F8--;
            }
        }

        g_game_info.pads[port].player = player;
        g_game_info.pads[port].player->pad_index = port;
        if (g_game_info.pads[port].player != 0 &&
            g_game_info.pads[port].player->slot.fighter != 0) {
            g_game_info.pads[port].player->slot.pdata->controller_port = port;
        }
        if (player == &g_game_info.plyr0) {
            g_game_info.pads[port].player->field_04 = 0;
        } else {
            g_game_info.pads[port].player->field_04 = 1;
        }
    } else {
        g_game_info.field_1F8--;
        if (player != 0) {
            player->pad_index = -1;
            player->field_04 = 3;
        }
        g_game_info.pads[port].player = 0;
        return 0;
    }
    return 1;
}

void unassign_player(PlyrInfo* player) {
    int port;

    if (player != 0) {
        port = player->pad_index;
        if (port > -1) {
            g_game_info.pads[port].player = 0;
            flush_controller_switch_buffers();
        }
        if (player->pad_index == 2) {
            g_game_info.pads[player->pad_index].flag_bits.connected = 0;
        }
        player->pad_index = -1;
        if (g_game_info.field_1F8 > 0) {
            g_game_info.field_1F8--;
        }
    }
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
void init_temp_switch_map(int player, int use_profile) {
    SwitchMapEntry* temp_map;
    PlayerProfile* profile;
    int* profile_status;
    int i;

    if (player == 0) {
        temp_map = p1_temp_switch_map;
        p1_use_temp_switch_map = 0;
        profile_status = &p1_profile_status;
        profile = &p1_profile;
    } else {
        temp_map = p2_temp_switch_map;
        p2_use_temp_switch_map = 0;
        profile_status = &p2_profile_status;
        profile = &p2_profile;
    }

    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        if (*profile_status == 1 && use_profile == 1) {
            temp_map[i].mask = profile->switch_map[i];
            temp_map[i].proc_fn = default_switch_map[i].proc_fn;
            temp_map[i].label = default_switch_map[i].label;
        } else {
            temp_map[i].mask = default_switch_map[i].mask;
            temp_map[i].proc_fn = default_switch_map[i].proc_fn;
            temp_map[i].label = default_switch_map[i].label;
        }
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

void set_default_switch_map(PlyrInfo* player) {
    int port;

    if (player == 0) {
        return;
    }
    if (player->player_state != 2) {
        return;
    }
    port = player->pad_index;
    if (port < 0) {
        return;
    }
    if (port > 3) {
        return;
    }
    g_game_info.pads[port].switch_map = default_switch_map;
}

static inline void assign_pad_switch_map(PlyrInfo* player, SwitchMapEntry* map) {
    int port;

    if (player == 0) {
        return;
    }
    if (player->player_state != 2) {
        return;
    }
    port = player->pad_index;
    if (port < 0) {
        return;
    }
    if (port > 3) {
        return;
    }
    g_game_info.pads[port].switch_map = map;
}

/* TODO: [near miss] 96.66%; selected-profile copies and loop registers remain; audit BSS first-use order. */
void set_game_switch_map(PlyrInfo* player) {
    int* use_temp_map;
    SwitchMapEntry* profile_map;
    PlayerProfile* profile;
    int* profile_status;
    SwitchMapEntry* temp_map;
    int i;

    if (player->field_04 == 0) {
        temp_map = p1_temp_switch_map;
        use_temp_map = &p1_use_temp_switch_map;
        profile_status = &p1_profile_status;
    } else {
        temp_map = p2_temp_switch_map;
        use_temp_map = &p2_use_temp_switch_map;
        profile_status = &p2_profile_status;
    }

    if (*profile_status != 0) {
        if (*use_temp_map != 0) {
            assign_pad_switch_map(player, temp_map);
            return;
        }
        if (player->field_04 == 0) {
            profile_map = p1_profile_switch_map;
            profile = &p1_profile;
        } else {
            profile_map = p2_profile_switch_map;
            profile = &p2_profile;
        }
        for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
            SwitchMapEntry* entry = &profile_map[i];
            const SwitchMapEntry* defaults = &default_switch_map[i];
            entry->mask = profile->switch_map[i];
            entry->proc_fn = defaults->proc_fn;
            entry->label = defaults->label;
        }
        assign_pad_switch_map(player, profile_map);
    } else if (*use_temp_map != 0) {
        assign_pad_switch_map(player, temp_map);
    } else {
        assign_pad_switch_map(player, default_switch_map);
    }
}

void set_game_switch_maps(void) {
    set_game_switch_map(&g_game_info.plyr0);
    set_game_switch_map(&g_game_info.plyr1);
}

void set_default_switch_maps(void) {
    set_default_switch_map(&g_game_info.plyr0);
    set_default_switch_map(&g_game_info.plyr1);
}

void switch_map_unload_player_profile(PlyrInfo* player) {
    int* rumble;
    int* use_temp_map;

    if (player->field_04 == 0) {
        rumble = &p1_rumble_on;
        use_temp_map = &p1_use_temp_switch_map;
    } else {
        rumble = &p2_rumble_on;
        use_temp_map = &p2_use_temp_switch_map;
    }
    *rumble = 0;
    *use_temp_map = 0;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
/* TODO: [near miss] 91.59%; table-copy behavior agrees; row-address GPR allocation and flag-store scheduling remain. */
void init_player_switch_maps(void) {
    SwitchMapEntry* dest;
    SwitchMapEntry* src;
    int i;

    dest = p1_temp_switch_map;
    p1_use_temp_switch_map = 0;
    src = default_switch_map;
    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        dest[i] = src[i];
    }

    dest = p2_temp_switch_map;
    p2_use_temp_switch_map = 0;
    src = default_switch_map;
    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        dest[i] = src[i];
    }
    p1_rumble_on = 0;
    p2_rumble_on = 0;
}

#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

PlyrInfo* get_player_for_port(int port) {
    PlyrInfo* player;
    int i;
    int found;

    if (mode_of_play == 8 && port == 2) {
        return trial_get_drone_info();
    }
    if (mode_of_play == 7) {
        return &g_game_info.plyr0;
    }
    if (mode_of_play == 4 && port == 2) {
        if (menu_player == 0) {
            player = &g_game_info.plyr1;
        } else {
            player = &g_game_info.plyr0;
        }
        return player;
    }

    if (g_game_info.field_1F8 == 0) {
        if (port == 0 || port == 2) {
            return &g_game_info.plyr0;
        }
        return &g_game_info.plyr1;
    }

    found = 0;
    for (i = 0; i < 3; i++) {
        if (g_game_info.pads[i].player != 0) {
            found = 1;
            break;
        }
    }
    if (found != 0) {
        player = g_game_info.pads[i].player;
        if (player == &g_game_info.plyr0) {
            player = &g_game_info.plyr1;
        } else {
            player = &g_game_info.plyr0;
        }
    } else {
        if (port == 0 || port == 2) {
            player = &g_game_info.plyr0;
        } else {
            player = &g_game_info.plyr1;
        }
    }
    return player;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
int check_for_non_game_locked_controller_state(void) {
    int game_state;

    game_state = get_game_state();
    if (game_state == 0x1B) {
        return 1;
    }
    if (game_state == 0x1A) {
        return 1;
    }
    if (game_state == 0x0D) {
        return 1;
    }
    if (game_state == 0x0E) {
        return 1;
    }
    return game_state == 0x19;
}

static inline int game_state_requires_active_player(int game_state) {
    if (game_state == 0x1B) {
        return 1;
    }
    if (game_state == 0x1A) {
        return 1;
    }
    if (game_state == 0x0D) {
        return 1;
    }
    if (game_state == 0x0E) {
        return 1;
    }
    if (game_state == 0x19) {
        return 1;
    }
    return 0;
}

static inline int controller_removed_screen_active(void) {
    if (find_mkproc_pid(0x2065) != 0 || find_mkproc_pid(0x2066) != 0) {
        return 1;
    }
    return 0;
}

int is_plyr_controller_enabled(PlyrInfo* player) {
    int active;

    if (player == 0) {
        return 0;
    }
    if (player->player_state == 3) {
        return 1;
    }
    if (player->pad_index == -1) {
        return 0;
    }

    active = 0;
    if (g_game_info.pads[player->pad_index].player->player_state == 2) {
        active = 1;
    }
    if (g_game_info.pads[player->pad_index].player->player_state == 1) {
        active = 1;
    }

    if (game_state_requires_active_player(get_game_state()) && active == 0 &&
        get_game_state() != 0x1A) {
        return 0;
    }
    if (g_game_info.switch_input_flags.eat_switches) {
        return 1;
    }

    if (controller_removed_screen_active() == 0) {
        switch (mode_of_play) {
        case 0:
        case 8:
        case 9:
        case 10:
            if (get_game_state() != 0x1A && active == 0) {
                return 0;
            }
            break;
        }
        if (g_game_info.pause_flag_bits.controllers_disabled) {
            return 0;
        }
        if (g_game_info.pads[player->pad_index].flag_bits.disabled) {
            return 0;
        }
    }
    return 1;
}

void init_port_info_struct(void) {
    unsigned int i;

    for (i = 0; i < 4; i++) {
        g_game_info.pads[i].flags_word = 0;
        g_game_info.pads[i].flag_bits.stick_dispatch = 1;
        g_game_info.pads[i].switch_map = default_switch_map;
        g_game_info.pads[i].player = 0;
        g_game_info.pads[i].prev_buttons = 0;
        g_game_info.pads[i].buttons = 0;
        g_game_info.pads[i].edge = 0;
        g_game_info.pads[i].stick_pack = 0;
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset
