#include "game/controller.h"

#include "game/game_info.h"
#include "game/mcardmsg.h"
#include "game/plyrprofile.h"
#include "game/trial.h"
#include "platform/display.h"
#include "platform/gcio.h"
#include "platform/io.h"
#include "platform/main.h"
#include "runtime/fonts.h"
#include "runtime/image.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"

#define RUMBLE_PROC_PID 0x2064
#define CONTROLLER_FADEBOX_OID 0x2081
#define CONTROLLER_ACCEPT_SWITCH 0xB
#define CONTROLLER_FADEBOX_HIDDEN_FLAG 0x08
#define CONTROLLER_FADEBOX_KEEP_FLAG 0x02
#define CONTROLLER_SCREEN_CENTER ((screen_width - 0x280) / 2)

typedef struct RumblePdata {
    MkHdr hdr;
    int port;
    int strength;
    int ticks;
} RumblePdata;

typedef struct ControllerProcVtable {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
} ControllerProcVtable;

typedef struct ControllerProcPrefix {
    ControllerProcVtable* vtbl;
    unsigned int instance;
} ControllerProcPrefix;

extern int p1_rumble_on;
extern int p2_rumble_on;
extern int p1_temp_rumble_state;
extern int p2_temp_rumble_state;
extern SwitchMapEntry default_switch_map[];
extern PlayerProfile p1_profile;
extern PlayerProfile p2_profile;
extern int p1_profile_status;
extern int p2_profile_status;
extern int p1_use_temp_switch_map;
extern int p2_use_temp_switch_map;
extern SwitchMapEntry p1_temp_switch_map[];
extern SwitchMapEntry p2_temp_switch_map[];
extern SwitchMapEntry p1_profile_switch_map[];
extern SwitchMapEntry p2_profile_switch_map[];
extern int menu_player;
extern void flush_controller_switch_buffers(void);
extern int get_stick_pos(int port, int stick, float* out_x, float* out_y);
extern int sounds_muted;
extern void mute_all_game_sounds(void);
extern void unmute_all_game_sounds(void);
extern int screen_width;
extern int sprintf(char* buffer, const char* format, ...);
extern int init_controller(void);

static float p_rumble_controller(void);
static float p_do_controller_removed(void);
PlyrInfo* get_player_for_port(int port);
void set_game_switch_map(PlyrInfo* player);

typedef struct ControllerRemovedPdata {
    MkHdr hdr;
    int port;
    int controllers_disabled;
} ControllerRemovedPdata;

typedef struct ControllerScreenObjRef {
    ScreenObj* object;
    int instance;
} ControllerScreenObjRef;

static ControllerScreenObjRef cnt_rem_fadebox_item;

#define DRAW_CONTROLLER_REMOVED_TEXT(screen_oid, player_x, port_number, text_buffer) \
    do {                                                                            \
        string_center_xy((screen_oid), 3, get_string(0x1D),                          \
                         CONTROLLER_SCREEN_CENTER + 0x140, 0x154, 0);                \
        string_center_xy((screen_oid), 0, get_string(0x1E),                          \
                         (player_x) + CONTROLLER_SCREEN_CENTER + 0xA0, 0x122, 0);     \
        string_center_xy((screen_oid), 0, get_string(0x1F),                          \
                         (player_x) + CONTROLLER_SCREEN_CENTER + 0xA0, 0x10E, 0);     \
        string_center_xy((screen_oid), 0, get_string(0x20),                          \
                         (player_x) + CONTROLLER_SCREEN_CENTER + 0xA0, 0xFA, 0);      \
        sprintf((text_buffer), get_string(0x21), (port_number) + 1);                 \
        string_center_xy((screen_oid), 0, (text_buffer),                             \
                         (player_x) + CONTROLLER_SCREEN_CENTER + 0xA0, 0xE6, 0);      \
        string_center_xy((screen_oid), 0, get_string(0x22),                          \
                         (player_x) + CONTROLLER_SCREEN_CENTER + 0xA0, 0xD2, 0);      \
        string_center_xy((screen_oid), 0, get_string(0x23),                          \
                         (player_x) + CONTROLLER_SCREEN_CENTER + 0xA0, 0xBE, 0);      \
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

void set_game_switch_map(PlyrInfo* player) {
    PlayerProfile* profile;
    SwitchMapEntry* profile_map;
    SwitchMapEntry* temp_map;
    int* profile_status;
    int* use_temp_map;
    int i;

    if (player->field_04 == 0) {
        profile = &p1_profile;
        profile_map = p1_profile_switch_map;
        temp_map = p1_temp_switch_map;
        profile_status = &p1_profile_status;
        use_temp_map = &p1_use_temp_switch_map;
    } else {
        profile = &p2_profile;
        profile_map = p2_profile_switch_map;
        temp_map = p2_temp_switch_map;
        profile_status = &p2_profile_status;
        use_temp_map = &p2_use_temp_switch_map;
    }

    if (*profile_status != 0) {
        if (*use_temp_map != 0) {
            set_default_switch_map(player);
            if (player != 0 && player->player_state == 2 &&
                player->pad_index >= 0 && player->pad_index <= 3) {
                g_game_info.pads[player->pad_index].switch_map = temp_map;
            }
        } else {
            for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
                profile_map[i].mask = profile->switch_map[i];
                profile_map[i].proc_fn = default_switch_map[i].proc_fn;
                profile_map[i].label = default_switch_map[i].label;
            }
            if (player != 0 && player->player_state == 2 &&
                player->pad_index >= 0 && player->pad_index <= 3) {
                g_game_info.pads[player->pad_index].switch_map = profile_map;
            }
        }
    } else if (*use_temp_map != 0) {
        if (player != 0 && player->player_state == 2 &&
            player->pad_index >= 0 && player->pad_index <= 3) {
            g_game_info.pads[player->pad_index].switch_map = temp_map;
        }
    } else {
        set_default_switch_map(player);
    }
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

void update_pause_menu_controller_state(void) {
    MkProc* proc;
    ControllerRemovedPdata* pdata;

    proc = find_mkproc_pid(0x208B);
    if (proc != 0) {
        pdata = (ControllerRemovedPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->controllers_disabled = (g_game_info.pause_flags >> 1) & 1;
        }
    }
}

int is_controller_removed(void) {
    if (find_mkproc_pid(0x2065) != 0) {
        return 1;
    }
    if (find_mkproc_pid(0x2066) != 0) {
        return 1;
    }
    return 0;
}

void update_cnt_removed_controller_state(void) {
    MkProc* proc;
    ControllerRemovedPdata* pdata;

    proc = find_mkproc_pid(0x2065);
    if (proc != 0) {
        pdata = (ControllerRemovedPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->controllers_disabled = (g_game_info.pause_flags >> 1) & 1;
        }
    }

    proc = find_mkproc_pid(0x2066);
    if (proc != 0) {
        pdata = (ControllerRemovedPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->controllers_disabled = (g_game_info.pause_flags >> 1) & 1;
        }
    }
}

void turn_all_rumble_motors_off(void) {
    MkProc* proc;
    ControllerProcPrefix* prefix;

    proc = find_mkproc_pid(RUMBLE_PROC_PID);
    prefix = (ControllerProcPrefix*)proc;
    if (prefix != 0 && prefix->instance != 0) {
        prefix->vtbl->destroy(proc);
    }
    turn_rumble_off(0);
    turn_rumble_off(1);
    turn_rumble_off(2);
    turn_rumble_off(3);
}

void controller_removed(int port) {
    MkProc* proc;
    MkHdr* pdata_out;
    ControllerRemovedPdata* pdata;
    PlyrInfo* player;
    int pid;

    proc = find_mkproc_pid(RUMBLE_PROC_PID);
    if (proc != 0 && proc->instance != 0) {
        ((ControllerProcVtable*)proc->vtbl)->destroy(proc);
    }
    turn_rumble_off(0);
    turn_rumble_off(1);
    turn_rumble_off(2);
    turn_rumble_off(3);

    player = g_game_info.pads[port].player;
    pid = player->field_04 == 0 ? 0x2065 : 0x2066;
    if (find_mkproc_pid(pid) == 0) {
        proc = _create_mkproc_generic_bigstack(
            pid, 4, p_do_controller_removed, sizeof(ControllerRemovedPdata), &pdata_out);
        if (proc != 0) {
            pdata = (ControllerRemovedPdata*)pdata_out;
            pdata->port = port;
            pdata->controllers_disabled = (g_game_info.pause_flags >> 1) & 1;
            proc->flags |= MKPROC_FLAG_SKIP_IF_PAUSED;
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
        if (port >= 0 && (g_game_info.pads[port].flags & GC_PAD_FLAG_CONNECTED) == 0) {
            controller_removed(port);
        }
    }

    if (g_game_info.plyr1.player_state != 0) {
        port = g_game_info.plyr1.pad_index;
        if (port >= 0 && (g_game_info.pads[port].flags & GC_PAD_FLAG_CONNECTED) == 0) {
            controller_removed(port);
        }
    }
}

void dispatch_right_sticks(int port) {
    GcPadSlot* pad;
    float x;
    float y;

    pad = &g_game_info.pads[port];
    if (pad->flag_bits.connected &&
        get_stick_pos(port, 1, &x, &y) != 0 && y > 0.0f) {
        pad->buttons |= pad->switch_map[0].mask;
    }
}

void dispatch_pad_sticks(int port) {
    GcPadSlot* pad;
    float x;
    float y;

    pad = &g_game_info.pads[port];
    if (pad->flag_bits.connected == 0 ||
        get_stick_pos(port, 0, &x, &y) == 0) {
        return;
    }
    if (x < 0.0f) {
        pad->buttons |= pad->switch_map[15].mask;
    }
    if (x > 0.0f) {
        pad->buttons |= pad->switch_map[13].mask;
    }
    if (y < 0.0f) {
        pad->buttons |= pad->switch_map[12].mask;
    }
    if (y > 0.0f) {
        pad->buttons |= pad->switch_map[14].mask;
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

    if ((g_game_info.pads[port].flags & GC_PAD_FLAG_CONNECTED) == 0) {
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

    if (player == 0 || port < 0) {
        g_game_info.field_1F8--;
        if (player != 0) {
            player->pad_index = -1;
            player->field_04 = 3;
        }
        g_game_info.pads[port].player = 0;
        return 0;
    }

    old_port = player->pad_index;
    if (old_port != -1 && player != 0) {
        if (old_port >= 0) {
            g_game_info.pads[old_port].player = 0;
            flush_controller_switch_buffers();
        }
        if (player->pad_index == 2) {
            ((GcPadFlags*)&g_game_info.pads[player->pad_index].flags)->connected = 0;
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
        if (old_port >= 0) {
            g_game_info.pads[old_port].player = 0;
            flush_controller_switch_buffers();
        }
        if (player->pad_index == 2) {
            ((GcPadFlags*)&g_game_info.pads[player->pad_index].flags)->connected = 0;
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
    return 1;
}

static float p_rumble_controller(void) {
    RumblePdata* pdata;

    pdata = (RumblePdata*)apdata;
    if (pdata != 0) {
        turn_rumble_on(pdata->port, pdata->strength);
        _mkproc_sleep_ticks = (float)pdata->ticks;
        ((ControllerProcVtable*)aproc->vtbl)->sleep();
        turn_rumble_off(pdata->port);
    }
    return 0.0f;
}

void ck_rumble_controller(int player, int strength, int ticks) {
    int game_state;
    int port;
    MkHdr* pdata_out;
    RumblePdata* pdata;

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
            RUMBLE_PROC_PID, 0x1F, p_rumble_controller, sizeof(RumblePdata), &pdata_out) == 0) {
        return;
    }

    pdata = (RumblePdata*)pdata_out;
    pdata->port = port;
    pdata->strength = strength;
    pdata->ticks = ticks;
}

/*
 * Controller-removed screen process. The fade-box handle is shared by the two
 * player-specific processes; its cached instance prevents a recycled screen
 * object from being mistaken for the original one.
 * Soft ceiling: p_do_controller_removed ~83.44% - nonvolatile allocation and
 * repeated screen-item latch/UI emission remain.
 */
static float p_do_controller_removed(void) {
    ControllerRemovedPdata* pdata;
    GcPadSlot* pad;
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

    pdata = (ControllerRemovedPdata*)apdata;
    port = pdata->port;
    if (port < 0 || port > 3) {
        return -1.0f;
    }

    pad = &g_game_info.pads[port];
    while ((g_game_info.flags & 0x80) != 0 || display_off != 0) {
        if (pad->flag_bits.connected) {
            unmute_all_game_sounds();
            return -1.0f;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((ControllerProcVtable*)aproc->vtbl)->sleep();
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
    if (!controllers_locked) {
        unmute_all_game_sounds();
        return -1.0f;
    }

    player = pad->player;
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

    fadebox = cnt_rem_fadebox_item.object;
    if (fadebox != 0 && fadebox->instance != cnt_rem_fadebox_item.instance) {
        fadebox = 0;
    }
    if (fadebox == 0) {
        fadebox = load_2d_pfxobj(0, CONTROLLER_FADEBOX_OID, (char*)0x10017, 0, 3);
        if (fadebox != 0) {
            cnt_rem_fadebox_item.object = fadebox;
            cnt_rem_fadebox_item.instance = fadebox->instance;
            fadebox->x = -0x32;
            fadebox->y = -0x32;
            fadebox->flags |= CONTROLLER_FADEBOX_HIDDEN_FLAG;
            fadebox->flags |= CONTROLLER_FADEBOX_KEEP_FLAG;
            fadebox->scale_x = 50.0f;
            fadebox->scale_y = 40.0f;
            pfx_2d_obj_set_alpha(fadebox, 0xA5);
        }
    }

    DRAW_CONTROLLER_REMOVED_TEXT(screen_oid, player_x, player_pad, initial_text);
    if (!g_game_info.feature_flags.bits.high_bit) {
        pause_procs(1);
    }

    for (;;) {
        if (check_switch_edge(player_pad, CONTROLLER_ACCEPT_SWITCH)) {
            break;
        }

        fadebox = cnt_rem_fadebox_item.object;
        if (fadebox != 0 && fadebox->instance != cnt_rem_fadebox_item.instance) {
            fadebox = 0;
        }
        if (fadebox == 0) {
            fadebox = load_2d_pfxobj(0, CONTROLLER_FADEBOX_OID, (char*)0x10017, 0, 3);
            if (fadebox != 0) {
                cnt_rem_fadebox_item.object = fadebox;
                cnt_rem_fadebox_item.instance = fadebox->instance;
                fadebox->x = -0x32;
                fadebox->y = -0x32;
                fadebox->flags |= CONTROLLER_FADEBOX_HIDDEN_FLAG;
                fadebox->flags |= CONTROLLER_FADEBOX_KEEP_FLAG;
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
        if (!controllers_locked) {
            break;
        }

        if (!g_game_info.feature_flags.bits.high_bit) {
            pause_procs(display_off == 0);
        }
        _mkproc_sleep_ticks = 1.0f;
        ((ControllerProcVtable*)aproc->vtbl)->sleep();
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

void unassign_player(PlyrInfo* player) {
    int port;

    if (player != 0) {
        port = player->pad_index;
        if (port > -1) {
            g_game_info.pads[port].player = 0;
            flush_controller_switch_buffers();
        }
        if (player->pad_index == 2) {
            ((GcPadFlags*)&g_game_info.pads[player->pad_index].flags)->connected = 0;
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
    PlayerProfile* profile;
    int* profile_status;
    SwitchMapEntry* temp_map;
    int* use_temp_map;
    SwitchMapEntry* defaults;
    int i;

    if (player == 0) {
        profile = &p1_profile;
        profile_status = &p1_profile_status;
        temp_map = p1_temp_switch_map;
        use_temp_map = &p1_use_temp_switch_map;
    } else {
        profile = &p2_profile;
        profile_status = &p2_profile_status;
        temp_map = p2_temp_switch_map;
        use_temp_map = &p2_use_temp_switch_map;
    }
    *use_temp_map = 0;

    defaults = default_switch_map;
    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        if (*profile_status == 1 && use_profile == 1) {
            temp_map->mask = profile->switch_map[i];
        } else {
            temp_map->mask = defaults->mask;
        }
        temp_map->proc_fn = defaults->proc_fn;
        temp_map->label = defaults->label;
        temp_map++;
        defaults++;
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
void init_player_switch_maps(void) {
    SwitchMapEntry* src;
    SwitchMapEntry* dest;
    int i;

    p1_use_temp_switch_map = 0;
    src = default_switch_map;
    dest = p1_temp_switch_map;
    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        dest->mask = src->mask;
        dest->proc_fn = src->proc_fn;
        dest->label = src->label;
        src++;
        dest++;
    }

    p2_use_temp_switch_map = 0;
    src = default_switch_map;
    dest = p2_temp_switch_map;
    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        dest->mask = src->mask;
        dest->proc_fn = src->proc_fn;
        dest->label = src->label;
        src++;
        dest++;
    }
    p1_rumble_on = 0;
    p2_rumble_on = 0;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

/* Soft ceiling: get_player_for_port ~98.82% - one unreachable retail branch. */
PlyrInfo* get_player_for_port(int port) {
    PlyrInfo* player;
    int i;
    int found;

    if ((int)mode_of_play == 8 && port == 2) {
        return trial_get_drone_info();
    }
    if ((int)mode_of_play == 7) {
        return &g_game_info.plyr0;
    }
    if ((int)mode_of_play == 4 && port == 2) {
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
            return &g_game_info.plyr1;
        }
        return &g_game_info.plyr0;
    }

    if (port == 0 || port == 2) {
        return &g_game_info.plyr0;
    }
    return &g_game_info.plyr1;
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

int is_plyr_controller_enabled(PlyrInfo* player) {
    int active;
    int game_state;
    int controller_removed;

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

    game_state = get_game_state();
    if ((game_state == 0x1B || game_state == 0x1A || game_state == 0x0D ||
         game_state == 0x0E || game_state == 0x19) &&
        active == 0 && get_game_state() != 0x1A) {
        return 0;
    }
    if (g_game_info.switch_input_flags.eat_switches) {
        return 1;
    }

    controller_removed =
        find_mkproc_pid(0x2065) != 0 || find_mkproc_pid(0x2066) != 0;
    if (controller_removed == 0) {
        if ((mode_of_play == 0 || (mode_of_play >= 8 && mode_of_play < 0xB)) &&
            get_game_state() != 0x1A && active == 0) {
            return 0;
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
    GcPadSlot* slots;
    unsigned int i;

    slots = g_game_info.pads;
    for (i = 0; i < 4; i++) {
        slots[i].flags = 0;
        ((GcPadFlags*)&slots[i].flags)->stick_dispatch = 1;
        slots[i].switch_map = default_switch_map;
        slots[i].player = 0;
        slots[i].prev_buttons = 0;
        slots[i].buttons = 0;
        slots[i].edge = 0;
        slots[i].stick_pack = 0;
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset
