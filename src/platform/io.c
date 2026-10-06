#include "game/switch.h"
#include "platform/io.h"

#include "game/controller.h"
#include "game/game_info.h"
#include "platform/main.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_vtbl.h"
#include "runtime/utils.h"
#include "runtime/cstdio.h"
#include "game/trial.h"
#include "game/ejb.h"
#include "game/plyr.h"
#include "game/moves.h"
#include "runtime/fonts.h"
#include "runtime/mk_cmdscript.h"

#include "game/fatality.h"

static float p_switch_log_fadeoff(void);

static const char io_text[] =
    "/logs/errorlog.txt\0 \0R\0l\0r\0Y\0X\0A\0B\0(\0/\0{\0:\0a\0";

const char* practice_font_offset_tbl[16] = {
    io_text + 0x13, io_text + 0x15, io_text + 0x17, io_text + 0x19,
    io_text + 0x1B, io_text + 0x1D, io_text + 0x1F, io_text + 0x21,
    io_text + 0x13, io_text + 0x13, io_text + 0x13, io_text + 0x13,
    io_text + 0x23, io_text + 0x25, io_text + 0x27, io_text + 0x29
};

struct SwitchLogEntry p1_switch_log[30];
struct SwitchLogEntry p2_switch_log[30];
struct SwitchLogEntry p1_pad_switch_log[30];
struct SwitchLogEntry p2_pad_switch_log[30];
int practice_p1_list[5];
int practice_p2_list[5];

unsigned char stick_dead_zone = 0x20;
int p1_log_index = -1;
int p2_log_index = -1;
int p1_pad_log_index = -1;
int p2_pad_log_index = -1;
int p1_current_log_index = -1;
int p2_current_log_index = -1;

SwitchPdata* switch_pdata;
int practice_p1_index;
int practice_p2_index;
int last_switch_time;
int sw_log_on;
int debug_message_handler_set;

static inline int check_decoy_distance_too_close(void) {
    if (plyr_pdata == 0) return 0;
    if (g_game_info.plyr0.slot.mirror_a == 0 ||
        g_game_info.plyr1.slot.mirror_a == 0) return 0;
    if (xz_distance_between_players() < 2.25f) return 1;
    return 0;
}

static inline void clear_controller_buttons(GameInfo* game) {
    int port;
    for (port = 0; port < 3; port++) {
        game->pads[port].edge =
            game->pads[port].buttons =
                game->pads[port].prev_buttons = 0;
    }
    game->pause_flag_bits.controllers_disabled = 1;
}

void vdebug_print_message(const char* format, ...) {
    static char buf[0x100];
    __va_list args;
    __builtin_va_info(&args);
    vsprintf(buf, format, args);
}

void debug_print_message() {
}

void debug_error_message() {
}

void pre_switchp(void) {
    switch_pdata = (SwitchPdata*)apdata;
}

void post_switchp(void) {
    switch_pdata = 0;
}

void flush_controller_switch_buffers(void) {
    int port;
    for (port = 0; port < 3; port++) {
        g_game_info.pads[port].prev_buttons = 0;
        g_game_info.pads[port].buttons = 0;
        g_game_info.pads[port].edge = 0;
    }
}

void turn_controllers_off(void) {
    int disabled = g_game_info.pause_flag_bits.controller_disable_guard;
    if (!disabled) {
        clear_controller_buttons(&g_game_info);
        update_cnt_removed_controller_state();
        update_pause_menu_controller_state();
    }
}

void turn_port_off(int port) {
    g_game_info.pads[port].flag_bits.disabled = 1;
}

void disable_all_ports_but_me(unsigned int my_port) {
    unsigned int port;
    for (port = 0; port < 3; port++)
        if (port != my_port) g_game_info.pads[port].flag_bits.disabled = 1;
}

void turn_all_ports_on(void) {
    int port;
    for (port = 0; port < 3; port++) g_game_info.pads[port].flag_bits.disabled = 0;
}

void turn_controllers_on(void) {
    int port;
    g_game_info.pause_flag_bits.controllers_disabled = 0;
    for (port = 0; port < 3; port++) {
        if (g_game_info.pads[port].player != 0)
            g_game_info.pads[port].flag_bits.disabled = 0;
    }
    update_cnt_removed_controller_state();
    update_pause_menu_controller_state();
}

unsigned int get_controller_disabled_state(void) {
    unsigned int state = 0;
    int port;
    if (g_game_info.pause_flag_bits.controllers_disabled) state = 1;
    for (port = 0; port < 3; port++) {
        state <<= 1;
        if (g_game_info.pads[port].flag_bits.disabled) state |= 1;
    }
    return state;
}

void reapply_controller_disabled_state(unsigned int state) {
    int port;

    for (port = 2; port >= 0; port--) {
        if (state & 1) {
            GcPadSlot* pad = &g_game_info.pads[port];
            pad->flag_bits.disabled = 1;
        } else {
            GcPadSlot* pad = &g_game_info.pads[port];
            pad->flag_bits.disabled = 0;
        }
        state >>= 1;
    }
    if (state & 1) g_game_info.pause_flag_bits.controllers_disabled = 1;
    else g_game_info.pause_flag_bits.controllers_disabled = 0;
}

void unstack_switches(void) {
    int port;
    int switch_index;
    for (port = 0; port < 3; port++) {
        if (g_game_info.pads[port].flag_bits.connected &&
            !g_game_info.pads[port].flag_bits.disabled) {
            if (g_game_info.pads[port].player == 0) {
                continue;
            }
            for (switch_index = 0; switch_index < 16; switch_index++) {
                if (g_game_info.pads[port].edge &
                    g_game_info.pads[port].switch_map[switch_index].mask) {
                    SwitchPdata* pdata;
                    MkProc* proc = _create_mkproc_generic_tinystack(
                        0x3002, 6,
                        g_game_info.pads[port].switch_map[switch_index].proc_fn,
                        sizeof(SwitchPdata), (MkHdr**)&pdata);
                    if (proc != 0) {
                        int mapped_index;
                        proc->pre_destroy = pre_switchp;
                        proc->destroy_cb = post_switchp;
                        pdata->player = g_game_info.pads[port].player;
                        if (pdata->player != 0) {
                            mapped_index = find_bit(
                                default_switch_map,
                                g_game_info.pads[port].switch_map[switch_index].mask);
                            if (mapped_index < 0) mapped_index = switch_index;
                            log_switch(
                                pdata->player->controller_slot,
                                switch_index, exec_tick_ctr,
                                g_game_info.pads[port].switch_map[switch_index].label,
                                mapped_index);
                        }
                    }
                }
            }
        }
    }
}

int check_switch(int port, int switch_index) {
    unsigned int switches;
    PlyrInfo* player = g_game_info.pads[port].player;
    if (plyr_pdata != 0 && mode_of_play == 8 && current_player_is_drone()) {
        player = &g_game_info.plyr1;
        if (g_game_info.plyr0.player_state == 3) player = &g_game_info.plyr0;
        switches = get_konquest_drone_switch_state(player->controller_slot);
        if (switches &
            g_game_info.pads[player->controller_slot]
                .switch_map[switch_index].mask) {
            return 1;
        }
        return 0;
    }
    if (port < 0 || port >= 3) return 0;
    if (!g_game_info.pads[port].flag_bits.connected) return 0;
    return !is_plyr_controller_enabled(player)
               ? 0
               : g_game_info.pads[port].switch_map[switch_index].mask ==
                     (g_game_info.pads[port].buttons &
                      g_game_info.pads[port].switch_map[switch_index].mask);
}

/* TODO: [near miss] 97.33%; final switch-index multiply precedes map load; operations and gates agree. */
int check_switch_edge(int port, int switch_index) {
    PlyrInfo* player = g_game_info.pads[port].player;
    if (mode_of_play == 8 && current_player_is_drone()) {
        unsigned int state;
        player = &g_game_info.plyr1;
        if (g_game_info.plyr0.player_state == 3) player = &g_game_info.plyr0;
        state = get_konquest_drone_switch_state(player->pad_index);
        if (state &
            g_game_info.pads[player->controller_slot]
                .switch_map[switch_index].mask) {
            return 1;
        }
        return 0;
    }
    if (port < 0 || port >= 3) return 0;
    if (!g_game_info.pads[port].flag_bits.connected) return 0;
    return !is_plyr_controller_enabled(player)
        ? 0
        : (g_game_info.pads[port].switch_map[switch_index].mask &
           g_game_info.pads[port].edge) != 0;
}

int check_switch_edge_any_pad(int switch_index) {
    int port;
    for (port = 0; port < 4; port++) {
        if (g_game_info.pads[port].flag_bits.connected &&
            (g_game_info.pads[port].edge &
             g_game_info.pads[port].switch_map[switch_index].mask)) return 1;
    }
    return 0;
}

void eat_switch_edge(int port, int switch_index) {
    unsigned int mask;
    if (port >= 0) {
        mask = g_game_info.pads[port].switch_map[switch_index].mask;
        g_game_info.pads[port].buttons |= mask;
        g_game_info.pads[port].prev_buttons |= mask;
        g_game_info.pads[port].edge &= ~mask;
    }
}

/* TODO: [near miss] 98.44538%; axis sampling agrees; repeated threshold
 * load and magnitude registers remain; whole-unit CSE-off regresses. */
int get_stick_pos(int port, int stick, float* horizontal, float* vertical) {
    int x = 0;
    int y = 0;
    if (port < 0) return 0;
    if (g_game_info.pause_flag_bits.controllers_disabled) return 0;
    if (g_game_info.pads[port].stick_pack == 0) {
        return 0;
    }
    if (stick == 0) {
        x = g_game_info.pads[port].stick_axis_3 - 0x7F;
        y = g_game_info.pads[port].stick_axis_2 - 0x7F;
    } else if (stick == 1) {
        x = g_game_info.pads[port].stick_axis_1 - 0x7F;
        y = g_game_info.pads[port].stick_axis_0 - 0x7F;
    }
    if ((x < 0 ? -x : x) < stick_dead_zone) x = 0;
    if ((y < 0 ? -y : y) < stick_dead_zone) y = 0;
    if (x < 0) *horizontal = (float)x / 56.0f;
    else *horizontal = (float)x / 56.0f;
    if (y < 0) *vertical = (float)y / 56.0f;
    else *vertical = (float)y / 56.0f;
    if (*horizontal < -1.0f) *horizontal = -1.0f;
    else if (*horizontal > 1.0f) *horizontal = 1.0f;
    if (*vertical < -1.0f) *vertical = -1.0f;
    else if (*vertical > 1.0f) *vertical = 1.0f;
    return 1;
}

/* TODO: [near miss] 97.05%; layout and store order agree; literal/BSS base register rows remain; stop at coloring. */
void init_switch_log(void) {
    int i;
    for (i = 0; i < 30; i++) {
        p1_switch_log[i].switch_index = -1;
        p1_pad_switch_log[i].switch_index = -1;
        p2_switch_log[i].switch_index = -1;
        p2_pad_switch_log[i].switch_index = -1;
        p1_switch_log[i].tick = -1;
        p1_pad_switch_log[i].tick = -1;
        p2_switch_log[i].tick = -1;
        p2_pad_switch_log[i].tick = -1;
        p1_switch_log[i].label = io_text + 0x2D;
        p1_pad_switch_log[i].label = io_text + 0x2D;
        p2_switch_log[i].label = io_text + 0x2D;
        p2_pad_switch_log[i].label = io_text + 0x2D;
    }
    practice_p1_index = 0;
    practice_p2_index = 0;
    for (i = 0; i < 5; i++) {
        practice_p1_list[i] = -1;
        practice_p2_list[i] = -1;
    }
}

/* TODO: [near miss] 94.22%; reset label/index GPR homes and setup scheduling remain. */
void turn_switch_log_on(void) {
    const char* label;
    int i;
    sw_log_on = 1;
    label = io_text + 0x2D;
    for (i = 0; i < 30; i++) {
        p1_switch_log[i].switch_index = -1;
        p1_pad_switch_log[i].switch_index = -1;
        p2_switch_log[i].switch_index = -1;
        p2_pad_switch_log[i].switch_index = -1;
        p1_switch_log[i].tick = -1;
        p1_pad_switch_log[i].tick = -1;
        p2_switch_log[i].tick = -1;
        p2_pad_switch_log[i].tick = -1;
        p1_switch_log[i].label = label;
        p1_pad_switch_log[i].label = label;
        p2_switch_log[i].label = label;
        p2_pad_switch_log[i].label = label;
    }
    practice_p1_index = 0;
    practice_p2_index = 0;
    for (i = 0; i < 5; i++) {
        practice_p1_list[i] = -1;
        practice_p2_list[i] = -1;
    }
}

void turn_switch_log_off(void) {
    sw_log_on = 0;
}

void log_switch(int player, int switch_index, int tick,
                const char* label, int mapped_index) {
    last_switch_time = exec_tick_ctr;
    if (player == 0) {
        switch (switch_index) {
        case 12:
        case 13:
        case 14:
        case 15:
            if (++p1_pad_log_index >= 30) p1_pad_log_index = 0;
            p1_pad_switch_log[p1_pad_log_index].switch_index = switch_index;
            p1_pad_switch_log[p1_pad_log_index].tick = tick;
            p1_pad_switch_log[p1_pad_log_index].label = label;
            p1_pad_switch_log[p1_pad_log_index].mapped_index = mapped_index;
            break;
        default:
            if (++p1_log_index >= 30) p1_log_index = 0;
            p1_switch_log[p1_log_index].switch_index = switch_index;
            p1_switch_log[p1_log_index].tick = tick;
            p1_switch_log[p1_log_index].label = label;
            p1_switch_log[p1_log_index].mapped_index = mapped_index;
            if (g_game_info.plyr0.slot.fighter != 0)
                p1_switch_log[p1_log_index].joy_state = joypad_state_5(g_game_info.plyr0.slot.pdata);
            else
                p1_switch_log[p1_log_index].joy_state = 0;
            break;
        }
        if (++practice_p1_index >= 5) practice_p1_index = 0;
        practice_p1_list[practice_p1_index] = mapped_index;
    } else {
        switch (switch_index) {
        case 12:
        case 13:
        case 14:
        case 15:
            if (++p2_pad_log_index >= 30) p2_pad_log_index = 0;
            p2_pad_switch_log[p2_pad_log_index].switch_index = switch_index;
            p2_pad_switch_log[p2_pad_log_index].tick = tick;
            p2_pad_switch_log[p2_pad_log_index].label = label;
            p2_pad_switch_log[p2_pad_log_index].mapped_index = mapped_index;
            break;
        default:
            if (++p2_log_index >= 30) p2_log_index = 0;
            p2_switch_log[p2_log_index].switch_index = switch_index;
            p2_switch_log[p2_log_index].tick = tick;
            p2_switch_log[p2_log_index].label = label;
            p2_switch_log[p2_log_index].mapped_index = mapped_index;
            if (g_game_info.plyr1.slot.fighter != 0)
                p2_switch_log[p2_log_index].joy_state = joypad_state_5(g_game_info.plyr1.slot.pdata);
            else
                p2_switch_log[p2_log_index].joy_state = 0;
            break;
        }
        if (++practice_p2_index >= 5) practice_p2_index = 0;
        practice_p2_list[practice_p2_index] = mapped_index;
    }
    if (sw_log_on != 0) show_sw_log();
}

/* TODO: [breakthrough] 86.73953%; retail table layout recovered;
 * remaining function-local code generation needs inspection. */
void show_sw_log(void) {
    int index;
    int y;
    int count;
    MkProc* old_proc;
    if (mode_of_play == 4) {
        del_string_obj_by_id(0x2083);
        index = practice_p1_index;
        y = 0x4B;
        old_proc = find_mkproc_pid(0x206C);
        if (old_proc != 0) {
            pfx_2d_obj_set_alpha_by_id(0x2083, 0xFF);
            if (old_proc->instance != 0) old_proc->vtbl->destroy(old_proc);
        }
        proc_create(p_switch_log_fadeoff, 0x206C);
        for (count = 0; count < 5 && practice_p1_list[index] != -1; count++) {
            const char* text = practice_font_offset_tbl[practice_p1_list[index]];
            if (*text != ' ') string_left_xy(0x2083, 7, text, 0x32, y, 0x1F);
            if (--index < 0) index = 4;
            y += 0x1E;
        }
        index = practice_p2_index;
        y = 0x4B;
        old_proc = find_mkproc_pid(0x206C);
        if (old_proc != 0) {
            pfx_2d_obj_set_alpha_by_id(0x2083, 0xFF);
            if (old_proc->instance != 0) old_proc->vtbl->destroy(old_proc);
        }
        proc_create(p_switch_log_fadeoff, 0x206C);
        for (count = 0; count < 5 && practice_p2_list[index] != -1; count++) {
            const char* text = practice_font_offset_tbl[practice_p2_list[index]];
            if (*text != ' ') string_left_xy(0x2083, 7, text, 0x235, y, 0x1F);
            if (--index < 0) index = 4;
            y += 0x1E;
        }
    } else {
        del_string_obj_by_id(0x2012);
        index = p1_pad_log_index < 0 ? 0 : p1_pad_log_index;
        y = 0x1E;
        for (count = 0; count < 4; count++) {
            if (p1_pad_switch_log[index].label != 0) {
                string_left_xy(0x2012, 0, p1_pad_switch_log[index].label,
                               0x14, y, 0);
                y += 0x19;
            }
            if (--index < 0) index = 29;
        }
        index = p1_log_index < 0 ? 0 : p1_log_index;
        y = 0x1E;
        for (count = 0; count < 4; count++) {
            if (p1_switch_log[index].label != 0) {
                string_left_xy(0x2012, 0, p1_switch_log[index].label,
                               0xAA, y, 0);
                y += 0x19;
            }
            if (--index < 0) index = 29;
        }
        index = p2_pad_log_index < 0 ? 0 : p2_pad_log_index;
        y = 0x1E;
        for (count = 0; count < 4; count++) {
            if (p2_pad_switch_log[index].label != 0) {
                string_left_xy(0x2012, 0, p2_pad_switch_log[index].label,
                               0x159, y, 0);
                y += 0x19;
            }
            if (--index < 0) index = 29;
        }
        index = p2_log_index < 0 ? 0 : p2_log_index;
        y = 0x1E;
        for (count = 0; count < 4; count++) {
            if (p2_switch_log[index].label != 0) {
                string_left_xy(0x2012, 0, p2_switch_log[index].label,
                               0x1EF, y, 0);
                y += 0x19;
            }
            if (--index < 0) index = 29;
        }
    }
}

static inline void reset_switch_log_state(void) {
    int i;

    for (i = 0; i < 30; i++) {
        p1_switch_log[i].switch_index = -1;
        p1_pad_switch_log[i].switch_index = -1;
        p2_switch_log[i].switch_index = -1;
        p2_pad_switch_log[i].switch_index = -1;
        p1_switch_log[i].tick = -1;
        p1_pad_switch_log[i].tick = -1;
        p2_switch_log[i].tick = -1;
        p2_pad_switch_log[i].tick = -1;
        p1_switch_log[i].label = io_text + 0x2D;
        p1_pad_switch_log[i].label = io_text + 0x2D;
        p2_switch_log[i].label = io_text + 0x2D;
        p2_pad_switch_log[i].label = io_text + 0x2D;
    }
    practice_p1_index = 0;
    practice_p2_index = 0;
    for (i = 0; i < 5; i++) {
        practice_p1_list[i] = -1;
        practice_p2_list[i] = -1;
    }
}

/* TODO: [near miss] 98.75%; log-reset phase agrees;
 * byte alpha decrement has one extra register copy. */
static float p_switch_log_fadeoff(void) {
    int alpha;

    _mkproc_sleep_ticks = 180.0f;
    aproc->vtbl->sleep();
    alpha = 0xF7;
    while ((unsigned char)alpha > 8) {
        pfx_2d_obj_set_alpha_by_id(0x2083, alpha);
        _mkproc_sleep_ticks = 1.0f;
        alpha = (unsigned char)(alpha - 8);
        aproc->vtbl->sleep();
    }
    pfx_2d_obj_set_alpha_by_id(0x2083, 0);

    reset_switch_log_state();
    return -1.0f;
}

static inline unsigned int check_subzero_decoy(unsigned int action) {
    if (plyr_pdata == 0) return action;
    if (plyr_pdata->character_id != 3) return action;
    if (action == 0xA && check_decoy_distance_too_close()) action = 9;
    return action;
}

static inline unsigned int check_smoke_decoy(unsigned int action) {
    if (plyr_pdata == 0) return action;
    if (plyr_pdata->character_id != 0x1B) return action;
    if (!is_sidekick_active(plyr_pdata->plyr_info)) return action;
    if (action == 0x1D && check_decoy_distance_too_close()) action = 0x1E;
    return action;
}

static inline void execute_scan_sequence_cmd(unsigned int action,
                                             int execution_mode) {
    plyr_pdata->field_234 = 1;
    if (plyr_pdata != 0 && plyr_pdata->state != 0x4203)
        pre_attack_chores();
    plyr_going_to_attack_with_action(action);
    share_my_attack_info(2.0f, 0.3f);
    trial_register_special_move(action);
    switch (execution_mode) {
    case 0x01000000:
        aproc->vtbl
            ->jump_sleep((MkProcEntryFn)action, 0.0f);
        break;
    case 0x02000000:
        cmdscript_reset_stack();
        cmdscript_setup_execution(
            plyr_pdata->fighter_definition->cmo, action);
        call_player_script_function(
            plyr_pdata->fighter_definition->cmo);
        break;
    case 0x03000000:
        cmdscript_reset_stack();
        cmdscript_setup_execution(plyr_pdata->cmo, action);
        call_player_script_function(plyr_pdata->cmo);
        break;
    case 0x04000000:
        cmdscript_reset_stack();
        cmdscript_setup_execution(reactions_cmo, action);
        call_player_script_function(reactions_cmo);
        break;
    }

}

static inline void scan_switch_sequence(
    const int* requirements, int remaining, struct SwitchLogEntry* switch_log,
    int history, unsigned int toward, unsigned int away, int max_tick_gap,
    unsigned int sequence_id, unsigned int action, int execution_mode) {
    for (; remaining-- > 0;) {
        struct SwitchLogEntry* entry = &switch_log[history];
        int expected;
        int tick_delta;

        if (--history < 0) history = 29;
        expected = requirements[remaining];
        switch (expected) {
        case (int)0x80000001:
            if ((unsigned int)entry->switch_index != toward) return;
            break;
        case (int)0x80000002:
            if ((unsigned int)entry->switch_index != away) return;
            break;
        case (int)0x80000003:
            if ((unsigned int)entry->switch_index != 0xC) return;
            break;
        case (int)0x80000004:
            if ((unsigned int)entry->switch_index != 0xE) return;
            break;
        default:
            return;
        }
        tick_delta = exec_tick_ctr - entry->tick;
        if (tick_delta < 0) tick_delta = -tick_delta;
        if ((unsigned int)tick_delta > (unsigned int)max_tick_gap) return;
    }
    if (sequence_id != 0) {
        if (sequence_id + 0xC0010000 == 0x0000FFFE) {
            if (mode_of_play == 8 ||
                !fatality_check_distance(action) ||
                !get_fatality_available_flag() ||
                (action == (unsigned int)do_my_fatality &&
                 !is_special_move_available(plyr_pdata, 0x4243)) ||
                (action == (unsigned int)do_my_2nd_fatality &&
                 !is_special_move_available(plyr_pdata, 0x4244)))
                return;
        }
        if (sequence_id + 0xC0010000 == 0x0000FFFD) {
            if (mode_of_play == 8 ||
                !get_fatality_available_flag() ||
                !is_special_move_available(plyr_pdata, 0x4245))
                return;
        } else if (is_this_move_disabled_exec(sequence_id)) {
            return;
        }
        action = check_smoke_decoy(check_subzero_decoy(action));
    }
    execute_scan_sequence_cmd(action, execution_mode);
}

/* TODO: [near miss] 97.68%; equivalent rejection branches and GPR coloring remain; stop at compiler model. */
void scan_switch_sequences(unsigned int* sequence) {
    struct SwitchLogEntry* switch_log;
    int log_index;
    int player_state;
    unsigned int toward;
    unsigned int away;
    int sequence_index;

    if (plyr_pdata == g_game_info.plyr0.slot.pdata) {
        log_index = p1_pad_log_index;
        player_state = g_game_info.plyr0.player_state;
        switch_log = p1_pad_switch_log;
    } else {
        log_index = p2_pad_log_index;
        player_state = g_game_info.plyr1.player_state;
        switch_log = p2_pad_switch_log;
    }
    if (log_index == -1 && player_state != 3) return;
    if (which_way_is_towards() < 0.0f) {
        toward = 0xD;
        away = 0xF;
    } else {
        away = 0xD;
        toward = 0xF;
    }
    for (sequence_index = 0; sequence_index < 12; sequence_index++) {
        unsigned int sequence_id = sequence[0];
        int timing;
        unsigned int action;
        int execution_mode;
        int max_tick_gap;
        int requirements[30];
        int requirement_count;

        if (sequence_id + 0x10000 == 0x0000FFFF) break;
        timing = sequence[1];
        requirement_count = 0;
        action = sequence[2];
        execution_mode = timing & 0x0F000000;
        max_tick_gap = timing - execution_mode;
        sequence += 3;
        while (1) {
            unsigned int requirement = *sequence;
            if (requirement + 0x10000 == 0x0000FFFF ||
                !(requirement & 0x80000000)) break;
            if (requirement_count >= 30) return;
            requirements[requirement_count] = requirement;
            sequence++;
            requirement_count++;
        }
        scan_switch_sequence(requirements, requirement_count, switch_log,
                             log_index, toward, away, max_tick_gap,
                             sequence_id, action, execution_mode);
    }
}

void scan_remote_switches(void) {
}
