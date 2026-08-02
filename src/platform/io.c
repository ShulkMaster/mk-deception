#include "platform/io.h"

#include "game/game_info.h"
#include "platform/main.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_vtbl.h"
#include "runtime/utils.h"

typedef struct {
    unsigned char gpr;
    unsigned char fpr;
    unsigned char reserved[2];
    char* input_arg_area;
    char* reg_save_area;
} __va_list[1];

#define va_start(list, last_arg) __va_start(list, last_arg)

extern int vsprintf(char* buffer, const char* format, __va_list args);

extern int current_player_is_drone(void);
extern int get_konquest_drone_switch_state(int player);
extern int is_plyr_controller_enabled(PlyrInfo* player);
extern void update_cnt_removed_controller_state(void);
extern void update_pause_menu_controller_state(void);
extern int joypad_state_5(void);
extern int find_bit(SwitchMapEntry* map, unsigned int mask);
extern SwitchMapEntry default_switch_map[16];
extern float p_switch_log_fadeoff(void);
extern void del_string_obj_by_id(int id);
extern void string_left_xy(int id, int font, const char* text, int x, int y,
                           int flags);
extern float which_way_is_towards(void);
extern int fatality_check_distance(unsigned int action);
extern int get_fatality_available_flag(void);
extern int is_special_move_available(PlyrPdata* player, int move_id);
extern int is_this_move_disabled_exec(unsigned int action);
extern float xz_distance_between_players(void);
extern void pre_attack_chores(void);
extern void plyr_going_to_attack_with_action(unsigned int action);
extern void share_my_attack_info(float scale, float duration);
extern void trial_register_special_move(unsigned int action);
extern void cmdscript_reset_stack(void);
extern void cmdscript_setup_execution(ScriptSlot* script, unsigned int action);
extern void call_player_script_function(ScriptSlot* script);
extern ScriptSlot* reactions_cmo;
extern void do_my_fatality(void);
extern void do_my_2nd_fatality(void);

typedef struct SwitchLogEntry {
    int switch_index;
    int tick;
    const char* label;
    int joy_state;
    int mapped_index;
} SwitchLogEntry;

typedef struct SwitchProcData {
    MkHdr hdr;
    PlyrInfo* player;
} SwitchProcData;

void show_sw_log(void);
void log_switch(int player, int switch_index, int tick,
                const char* label, int mapped_index, PlyrInfo* info);
void post_switchp(void);
void pre_switchp(void);

static const char io_text[] =
    "/logs/errorlog.txt\0 \0R\0l\0r\0Y\0X\0A\0B\0(\0/\0{\0:\0a\0";

const char* practice_font_offset_tbl[16] = {
    io_text + 0x13, io_text + 0x15, io_text + 0x17, io_text + 0x19,
    io_text + 0x1B, io_text + 0x1D, io_text + 0x1F, io_text + 0x21,
    io_text + 0x13, io_text + 0x13, io_text + 0x13, io_text + 0x13,
    io_text + 0x23, io_text + 0x25, io_text + 0x27, io_text + 0x29
};

SwitchLogEntry p1_switch_log[30];
SwitchLogEntry p2_switch_log[30];
SwitchLogEntry p1_pad_switch_log[30];
SwitchLogEntry p2_pad_switch_log[30];
int practice_p1_list[5];
int practice_p2_list[5];
static char debug_message_buffer[0x100];

unsigned char stick_dead_zone = 0x20;
int p1_log_index = -1;
int p2_log_index = -1;
int p1_pad_log_index = -1;
int p2_pad_log_index = -1;
int p1_current_log_index = -1;
int p2_current_log_index = -1;
int gap_07_8050F8FC_sdata;

int debug_message_handler_set;
int sw_log_on;
int last_switch_time;
int practice_p2_index;
int practice_p1_index;
MkHdr* switch_pdata;

void scan_remote_switches(void) {
}

void scan_switch_sequences(unsigned int* sequence) {
    SwitchLogEntry* switch_log;
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
        int timing = sequence[1];
        unsigned int action = sequence[2];
        unsigned int requirements[30];
        int requirement_count = 0;
        int matched = 1;
        sequence += 3;
        if (sequence_id + 0x10000 == 0xFFFFFFFF) break;
        while (sequence[0] + 0x10000 != 0xFFFFFFFF &&
               (sequence[0] & 0x80000000) && requirement_count < 30) {
            requirements[requirement_count++] = *sequence++;
        }
        if (requirement_count > 0) {
            int history = log_index;
            int requirement = requirement_count - 1;
            while (requirement >= 0) {
                SwitchLogEntry* entry = &switch_log[history];
                unsigned int expected = requirements[requirement];
                unsigned int expected_switch;
                if (expected == 0x80000001) expected_switch = toward;
                else if (expected == 0x80000002) expected_switch = away;
                else if (expected == 0x80000003) expected_switch = 0xC;
                else if (expected == 0x80000004) expected_switch = 0xE;
                else {
                    matched = 0;
                    break;
                }
                if ((unsigned int)entry->switch_index != expected_switch ||
                    (unsigned int)(exec_tick_ctr - entry->tick < 0
                        ? entry->tick - exec_tick_ctr : exec_tick_ctr - entry->tick) >
                        (unsigned int)(timing - (timing & 0x0F000000))) {
                    matched = 0;
                    break;
                }
                if (--history < 0) history = 29;
                requirement--;
            }
        }
        if (matched) {
            unsigned int selected_action = action;
            if (sequence_id != 0) {
                if (sequence_id + 0xC0010000 == 0xFFFFFFFE) {
                    if (mode_of_play == 8 || !fatality_check_distance(action) ||
                        !get_fatality_available_flag() ||
                        (action == (unsigned int)do_my_fatality &&
                         !is_special_move_available(plyr_pdata, 0x4243)) ||
                        (action == (unsigned int)do_my_2nd_fatality &&
                         !is_special_move_available(plyr_pdata, 0x4244))) continue;
                } else if (sequence_id + 0xC0010000 == 0xFFFFFFFD) {
                    if (mode_of_play == 8 || !get_fatality_available_flag() ||
                        !is_special_move_available(plyr_pdata, 0x4245)) continue;
                } else if (is_this_move_disabled_exec(sequence_id)) continue;
            }
            if (plyr_pdata != 0 && plyr_pdata->character_id == 3 && action == 0xA &&
                g_game_info.plyr0.slot.mirror_a != 0 &&
                g_game_info.plyr1.slot.mirror_a != 0 &&
                xz_distance_between_players() < 2.25f) selected_action = 9;
            if (plyr_pdata != 0 && plyr_pdata->character_id == 0x1B &&
                is_sidekick_active(plyr_pdata->plyr_info) &&
                selected_action == 0x1D &&
                g_game_info.plyr0.slot.mirror_a != 0 &&
                g_game_info.plyr1.slot.mirror_a != 0 &&
                xz_distance_between_players() < 2.25f) selected_action = 0x1E;
            action = selected_action;
            plyr_pdata->field_234 = 1;
            if (plyr_pdata != 0 && plyr_pdata->state != 0x4203) pre_attack_chores();
            plyr_going_to_attack_with_action(action);
            share_my_attack_info(2.0f, 0.3f);
            trial_register_special_move(action);
            switch (timing & 0x0F000000) {
            case 0x02000000:
                cmdscript_reset_stack();
                cmdscript_setup_execution(plyr_pdata->fighter_definition->cmo, action);
                call_player_script_function(plyr_pdata->fighter_definition->cmo);
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
    }
}

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
            if (old_proc->instance != 0) old_proc->vtbl->destroy();
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
            if (old_proc->instance != 0) old_proc->vtbl->destroy();
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

void log_switch(int player, int switch_index, int tick,
                const char* label, int mapped_index, PlyrInfo* info) {
    SwitchLogEntry* entry;
    last_switch_time = exec_tick_ctr;
    if (player == 0) {
        if (switch_index >= 12 && switch_index < 16) {
            if (++p1_pad_log_index >= 30) p1_pad_log_index = 0;
            entry = &p1_pad_switch_log[p1_pad_log_index];
            entry->switch_index = switch_index;
            entry->tick = tick;
            entry->label = label;
            entry->mapped_index = mapped_index;
        } else {
            if (++p1_log_index >= 30) p1_log_index = 0;
            entry = &p1_switch_log[p1_log_index];
            entry->switch_index = switch_index;
            entry->tick = tick;
            entry->label = label;
            entry->mapped_index = mapped_index;
            if (g_game_info.plyr0.slot.fighter != 0)
                entry->joy_state = joypad_state_5();
            else
                entry->joy_state = 0;
        }
        if (++practice_p1_index >= 5) practice_p1_index = 0;
        practice_p1_list[practice_p1_index] = mapped_index;
    } else {
        if (switch_index >= 12 && switch_index < 16) {
            if (++p2_pad_log_index >= 30) p2_pad_log_index = 0;
            entry = &p2_pad_switch_log[p2_pad_log_index];
            entry->switch_index = switch_index;
            entry->tick = tick;
            entry->label = label;
            entry->mapped_index = mapped_index;
        } else {
            if (++p2_log_index >= 30) p2_log_index = 0;
            entry = &p2_switch_log[p2_log_index];
            entry->switch_index = switch_index;
            entry->tick = tick;
            entry->label = label;
            entry->mapped_index = mapped_index;
            if (g_game_info.plyr1.slot.fighter != 0)
                entry->joy_state = joypad_state_5();
            else
                entry->joy_state = 0;
        }
        if (++practice_p2_index >= 5) practice_p2_index = 0;
        practice_p2_list[practice_p2_index] = mapped_index;
    }
    if (sw_log_on != 0) show_sw_log();
}

void turn_switch_log_off(void) {
    sw_log_on = 0;
}

void turn_switch_log_on(void) {
    int i;
    sw_log_on = 1;
    for (i = 0; i < 30; i++) {
        p1_switch_log[i].switch_index = -1;
        p2_switch_log[i].switch_index = -1;
        p1_pad_switch_log[i].switch_index = -1;
        p2_pad_switch_log[i].switch_index = -1;
        p1_switch_log[i].tick = -1;
        p2_switch_log[i].tick = -1;
        p1_pad_switch_log[i].tick = -1;
        p2_pad_switch_log[i].tick = -1;
        p1_switch_log[i].label = io_text + 0x2D;
        p2_switch_log[i].label = io_text + 0x2D;
        p1_pad_switch_log[i].label = io_text + 0x2D;
        p2_pad_switch_log[i].label = io_text + 0x2D;
    }
    practice_p1_index = 0;
    practice_p2_index = 0;
    for (i = 0; i < 5; i++) {
        practice_p1_list[i] = -1;
        practice_p2_list[i] = -1;
    }
}

void init_switch_log(void) {
    int i;
    for (i = 0; i < 30; i++) {
        p1_switch_log[i].switch_index = -1;
        p2_switch_log[i].switch_index = -1;
        p1_pad_switch_log[i].switch_index = -1;
        p2_pad_switch_log[i].switch_index = -1;
        p1_switch_log[i].tick = -1;
        p2_switch_log[i].tick = -1;
        p1_pad_switch_log[i].tick = -1;
        p2_pad_switch_log[i].tick = -1;
        p1_switch_log[i].label = io_text + 0x2D;
        p2_switch_log[i].label = io_text + 0x2D;
        p1_pad_switch_log[i].label = io_text + 0x2D;
        p2_pad_switch_log[i].label = io_text + 0x2D;
    }
    practice_p1_index = 0;
    practice_p2_index = 0;
    for (i = 0; i < 5; i++) {
        practice_p1_list[i] = -1;
        practice_p2_list[i] = -1;
    }
}

int get_stick_pos(int port, int stick, float* horizontal, float* vertical) {
    int x = 0;
    int y = 0;
    GcPadSlot* pad;
    if (port < 0 || g_game_info.pause_flag_bits.controllers_disabled) {
        return 0;
    }
    pad = &g_game_info.pads[port];
    if (pad->stick_pack == 0) {
        return 0;
    }
    if (stick == 0) {
        x = pad->stick_axis_3 - 0x7F;
        y = pad->stick_axis_2 - 0x7F;
    } else if (stick == 1) {
        y = pad->stick_axis_0 - 0x7F;
        x = pad->stick_axis_1 - 0x7F;
    }
    if ((x < 0 ? -x : x) < (int)stick_dead_zone) x = 0;
    if ((y < 0 ? -y : y) < (int)stick_dead_zone) y = 0;
    if (x < 0) *horizontal = (float)x / 56.0f;
    else *horizontal = (float)(unsigned int)x / 56.0f;
    if (y < 0) *vertical = (float)y / 56.0f;
    else *vertical = (float)(unsigned int)y / 56.0f;
    if (*horizontal < -1.0f) *horizontal = -1.0f;
    else if (*horizontal > 1.0f) *horizontal = 1.0f;
    if (*vertical < -1.0f) *vertical = -1.0f;
    else if (*vertical > 1.0f) *vertical = 1.0f;
    return 1;
}

void eat_switch_edge(int port, int switch_index) {
    unsigned int mask;
    GcPadSlot* pad;
    if (port >= 0) {
        pad = &g_game_info.pads[port];
        mask = pad->switch_map[switch_index].mask;
        pad->buttons |= mask;
        pad->prev_buttons |= mask;
        pad->edge &= ~mask;
    }
}

int check_switch_edge_any_pad(int switch_index) {
    int port;
    for (port = 0; port < 4; port++) {
        GcPadSlot* pad = &g_game_info.pads[port];
        if (pad->flag_bits.connected &&
            (pad->edge & pad->switch_map[switch_index].mask)) return 1;
    }
    return 0;
}

int check_switch_edge(int port, int switch_index) {
    GcPadSlot* pad;
    PlyrInfo* player = g_game_info.pads[port].player;
    if (mode_of_play == 8 && current_player_is_drone()) {
        player = &g_game_info.plyr1;
        if (g_game_info.plyr0.player_state == 3) player = &g_game_info.plyr0;
        pad = &g_game_info.pads[player->controller_slot];
        return (get_konquest_drone_switch_state(player->pad_index) &
                pad->switch_map[switch_index].mask) != 0;
    }
    if (port < 0 || port >= 3) return 0;
    pad = &g_game_info.pads[port];
    if (!pad->flag_bits.connected || !is_plyr_controller_enabled(player)) return 0;
    return (pad->edge & pad->switch_map[switch_index].mask) != 0;
}

int check_switch(int port, int switch_index) {
    GcPadSlot* pad;
    PlyrInfo* player = g_game_info.pads[port].player;
    if (plyr_pdata != 0 && mode_of_play == 8 && current_player_is_drone()) {
        player = &g_game_info.plyr1;
        if (g_game_info.plyr0.player_state == 3) player = &g_game_info.plyr0;
        pad = &g_game_info.pads[player->controller_slot];
        return (get_konquest_drone_switch_state(player->controller_slot) &
                pad->switch_map[switch_index].mask) != 0;
    }
    if (port < 0 || port >= 3) return 0;
    pad = &g_game_info.pads[port];
    if (!pad->flag_bits.connected || !is_plyr_controller_enabled(player)) return 0;
    return pad->switch_map[switch_index].mask ==
           (pad->buttons & pad->switch_map[switch_index].mask);
}

void unstack_switches(void) {
    int port;
    for (port = 0; port < 3; port++) {
        GcPadSlot* pad = &g_game_info.pads[port];
        if (pad->flag_bits.connected && !pad->flag_bits.disabled &&
            pad->player != 0) {
            int switch_index;
            for (switch_index = 0; switch_index < 16; switch_index++) {
                SwitchMapEntry* mapping = &pad->switch_map[switch_index];
                if (pad->edge & mapping->mask) {
                    union {
                        MkHdr* header;
                        SwitchProcData* switch_data;
                    } pdata;
                    MkProc* proc = _create_mkproc_generic_tinystack(
                        0x3002, 6, mapping->proc_fn, sizeof(SwitchProcData),
                        &pdata.header);
                    if (proc != 0) {
                        int mapped_index;
                        proc->pre_destroy = pre_switchp;
                        proc->destroy_cb = post_switchp;
                        pdata.switch_data->player = pad->player;
                        mapped_index = find_bit(default_switch_map, mapping->mask);
                        if (mapped_index < 0) mapped_index = switch_index;
                        log_switch(pdata.switch_data->player->controller_slot,
                                   switch_index,
                                   exec_tick_ctr, mapping->label, mapped_index,
                                   pdata.switch_data->player);
                    }
                }
            }
        }
    }
}

void reapply_controller_disabled_state(unsigned int state) {
    GcPadSlot* pad = &g_game_info.pads[2];
    int count = 3;
    do {
        if (state & 1) pad->flags |= GC_PAD_FLAG_DISABLED;
        else pad->flags &= ~GC_PAD_FLAG_DISABLED;
        state >>= 1;
        pad--;
    } while (--count != 0);
    if (state & 1) g_game_info.pause_flags |= 2;
    else g_game_info.pause_flags &= ~2;
}

unsigned int get_controller_disabled_state(void) {
    unsigned int state = 0;
    int port;
    if (g_game_info.pause_flags & 2) state = 1;
    for (port = 0; port < 3; port++) {
        state <<= 1;
        if (g_game_info.pads[port].flags & GC_PAD_FLAG_DISABLED) state |= 1;
    }
    return state;
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

void turn_all_ports_on(void) {
    int port;
    for (port = 0; port < 3; port++) g_game_info.pads[port].flag_bits.disabled = 0;
}

void disable_all_ports_but_me(unsigned int my_port) {
    unsigned int port;
    for (port = 0; port < 3; port++)
        if (port != my_port) g_game_info.pads[port].flag_bits.disabled = 1;
}

void turn_port_off(int port) {
    g_game_info.pads[port].flag_bits.disabled = 1;
}

void turn_controllers_off(void) {
    int port;
    if (!g_game_info.pause_flag_bits.controller_disable_guard) {
        for (port = 0; port < 3; port++) {
            g_game_info.pads[port].prev_buttons = 0;
            g_game_info.pads[port].buttons = 0;
            g_game_info.pads[port].edge = 0;
        }
        g_game_info.pause_flag_bits.controllers_disabled = 1;
        update_cnt_removed_controller_state();
        update_pause_menu_controller_state();
    }
}

void flush_controller_switch_buffers(void) {
    int port;
    for (port = 0; port < 3; port++) {
        g_game_info.pads[port].prev_buttons = 0;
        g_game_info.pads[port].buttons = 0;
        g_game_info.pads[port].edge = 0;
    }
}

void post_switchp(void) {
    switch_pdata = 0;
}

void pre_switchp(void) {
    switch_pdata = apdata;
}

void debug_error_message() {
}

void debug_print_message() {
}

void vdebug_print_message(const char* format, ...) {
    __va_list args;
    va_start(args, format);
    vsprintf(debug_message_buffer, format, args);
}
