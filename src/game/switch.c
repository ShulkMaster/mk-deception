#include "game/switch.h"
#include "game/controller.h"
#include "game/game_info.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_obj.h"

typedef int s32;
typedef float f32;

typedef struct SwitchPdata {
    MkHdr hdr;
    PlyrInfo* player;
} SwitchPdata;

typedef struct SwitchProcVtable {
    int (*reserved[6])(void);
    void (*sleep)(MkProc* proc);
    int (*stack_ops[2])(void);
    f32 (*jump_sleep)(f32 ticks, MkProcEntryFn entry);
} SwitchProcVtable;

typedef struct JoinPdata {
    MkHdr hdr;
    int port;
} JoinPdata;

typedef struct SwitchGameFlags {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char pad : 5;
} SwitchGameFlags;

extern SwitchPdata* switch_pdata;
extern JoinPdata* mab_generic_pdata;
extern unsigned long display_off;
extern PlyrPdata* plyr_pdata;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern int exec_tick_ctr;
extern f32 _mkproc_sleep_ticks;
extern f32 joy_dash_back(void);
extern f32 joy_duck_loop(void);
extern f32 x_angle_jump_left(void);
extern f32 x_angle_jump_right(void);

int get_game_state(void);
int ok_to_join_in(void);
void* proc_create(void* proc_fn, int proc_id);
f32 do_join_in(void);
f32 p_pause_menu_switch(void);
f32 p_switch_proc_start(void);
f32 switch_proc_attack_4(void);
f32 switch_proc_attack_3(void);
f32 switch_proc_attack_1(void);
f32 p_puzzle_switch_4(void);
f32 p_puzzle_switch_3(void);
f32 p_puzzle_switch_1(void);
f32 p_konquest_switch_4(void);
f32 p_konquest_switch_3(void);
f32 p_konquest_switch_1(void);
f32 p_board_switch_4(void);
f32 p_board_switch_over_3(void);
f32 p_board_switch_3(void);
f32 p_board_switch_1(void);
f32 p_atm_start_button(void);
f32 p_block(void);
f32 p_puzzle_switch_drop(void);
f32 switch_proc_attack_5(void);
f32 p_konquest_switch_R1(void);
f32 p_board_switch_r2(void);
f32 switch_proc_advance_moveset(void);
f32 p_board_switch_l1(void);
f32 switch_proc_pickup(void);
f32 switch_proc_attack_2(void);
f32 p_puzzle_switch_2(void);
f32 p_konquest_inventory_switch(void);
f32 p_board_switch_2(void);
f32 p_puzzle_switch_right(void);
f32 p_puzzle_switch_left(void);
f32 p_puzzle_switch_down(void);
f32 p_puzzle_switch_up(void);
f32 p_puzzle_switch_lt_stick(void);
f32 p_swap_levels(void);
f32 switch_proc_up(void);
f32 switch_proc_down(void);
f32 switch_proc_left(void);
f32 switch_proc_right(void);
int check_switch(int port, int switch_index);
int is_this_move_disabled_exec(int move_id);
f32 which_way_is_towards(void);

s32 dash_back_check(f32 direction);

static int switch_input_blocked(void) {
    return !is_plyr_controller_enabled(switch_pdata->player) ||
           is_controller_removed() ||
           g_game_info.switch_input_flags.eat_switches;
}

static f32 dispatch_switch(MkProcEntryFn entry) {
    ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, entry);
    return 0.0f;
}

f32 pad_rt_stick_btn_proc(void) {
    return -1.0f;
}

f32 pad_lt_stick_btn_proc(void) {
    if (switch_input_blocked()) {
        return -1.0f;
    }
    if (get_game_state() == 2 && switch_pdata->player->pad_index == 0) {
        return dispatch_switch(p_swap_levels);
    }
    if (get_game_state() == 0x12) {
        return dispatch_switch(p_puzzle_switch_lt_stick);
    }
    return -1.0f;
}

f32 pad_select_proc(void) {
    if (!switch_input_blocked()) {
        get_game_state();
    }
    return -1.0f;
}

f32 pad_r2_proc(void) {
    if (switch_input_blocked()) {
        return -1.0f;
    }
    if (get_game_state() == 7) {
        return dispatch_switch(p_block);
    }
    if (get_game_state() == 0x12) {
        return dispatch_switch(p_puzzle_switch_drop);
    }
    return -1.0f;
}

f32 pad_r1_proc(void) {
    if (switch_input_blocked()) {
        return -1.0f;
    }
    switch (get_game_state()) {
    case 7: return dispatch_switch(switch_proc_attack_5);
    case 0x13: return dispatch_switch(p_konquest_switch_R1);
    case 0x17: return dispatch_switch(p_board_switch_r2);
    }
    return -1.0f;
}

f32 pad_l2_proc(void) {
    if (!switch_input_blocked() && get_game_state() == 7) {
        return dispatch_switch(switch_proc_pickup);
    }
    return -1.0f;
}

f32 pad_l1_proc(void) {
    if (switch_input_blocked()) {
        return -1.0f;
    }
    if (get_game_state() == 7) {
        return dispatch_switch(switch_proc_advance_moveset);
    }
    if (get_game_state() == 0x17) {
        return dispatch_switch(p_board_switch_l1);
    }
    return -1.0f;
}

f32 pad_rup_proc(void) {
    if (switch_input_blocked()) {
        return -1.0f;
    }
    switch (get_game_state()) {
    case 7: return dispatch_switch(switch_proc_attack_2);
    case 0x12: return dispatch_switch(p_puzzle_switch_2);
    case 0x13:
    case 0x14: return dispatch_switch(p_konquest_inventory_switch);
    case 0x17: return dispatch_switch(p_board_switch_2);
    }
    return -1.0f;
}

static f32 dispatch_direction(
    MkProcEntryFn fight_entry, MkProcEntryFn puzzle_entry) {
    if (switch_input_blocked()) {
        return -1.0f;
    }
    if (get_game_state() == 7) {
        return dispatch_switch(fight_entry);
    }
    if (get_game_state() == 0x12) {
        return dispatch_switch(puzzle_entry);
    }
    return -1.0f;
}

f32 pad_lrt_proc(void) {
    return dispatch_direction(switch_proc_right, p_puzzle_switch_right);
}

f32 pad_llt_proc(void) {
    return dispatch_direction(switch_proc_left, p_puzzle_switch_left);
}

f32 pad_ldn_proc(void) {
    return dispatch_direction(switch_proc_down, p_puzzle_switch_down);
}

f32 pad_lup_proc(void) {
    return dispatch_direction(switch_proc_up, p_puzzle_switch_up);
}

/*
 * Soft ceilings: pad_start_proc, pad_rrt_proc/pad_rlt_proc, and pad_rdn_proc
 * differ only in TU-local float-pool relocation identity.
 */
f32 pad_start_proc(void) {
    int state;

    if (is_plyr_controller_enabled(switch_pdata->player) &&
        (switch_pdata->player->player_state == 2 ||
         switch_pdata->player->player_state == 1)) {
        switch (get_game_state()) {
        case 7:
        case 0x12:
        case 0x17:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_pause_menu_switch);
            return 0.0f;
        case 0x13:
        case 0x14:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_switch_proc_start);
            return 0.0f;
        case 3:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_atm_start_button);
            return 0.0f;
        }
    } else {
        if (switch_pdata->player == 0) {
            return -1.0f;
        }

        if (switch_pdata->player->player_state == 0) {
            state = get_game_state();
            switch (state) {
            case 0x12:
                if ((int)display_off != 0) {
                    break;
                }
                /* Fall through. */
            case 7:
                if (ok_to_join_in() && proc_create(do_join_in, 0x2073) != 0) {
                    mab_generic_pdata->port = switch_pdata->player->pad_index;
                }
                break;
            case 3:
                ((SwitchProcVtable*)aproc->vtbl)
                    ->jump_sleep(0.0f, p_atm_start_button);
                return 0.0f;
            }
        } else if (get_game_state() == 7 && are_controllers_locked() &&
                   ((SwitchGameFlags*)&g_game_info.flags)->bit5 == 0 &&
                   ((SwitchGameFlags*)&g_game_info.flags)->bit6 != 0) {
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_pause_menu_switch);
            return 0.0f;
        }
    }
    return -1.0f;
}

f32 pad_rrt_proc(void) {
    int eat_switch;

    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (is_controller_removed()) {
            eat_switch = 1;
        } else if (g_game_info.switch_input_flags.eat_switches) {
            eat_switch = 1;
        } else {
            eat_switch = 0;
        }
        if (eat_switch != 0) {
            return -1.0f;
        }

        switch (get_game_state()) {
        case 7:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, switch_proc_attack_4);
            return 0.0f;
        case 0x12:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_puzzle_switch_4);
            return 0.0f;
        case 0x13:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_konquest_switch_4);
            return 0.0f;
        case 0x14:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_konquest_switch_4);
            return 0.0f;
        case 0x17:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_board_switch_4);
            return 0.0f;
        }
    }
    return -1.0f;
}

f32 pad_rlt_proc(void) {
    int eat_switch;

    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (is_controller_removed()) {
            eat_switch = 1;
        } else if (g_game_info.switch_input_flags.eat_switches) {
            eat_switch = 1;
        } else {
            eat_switch = 0;
        }
        if (eat_switch != 0) {
            return -1.0f;
        }

        switch (get_game_state()) {
        case 7:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, switch_proc_attack_1);
            return 0.0f;
        case 0x12:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_puzzle_switch_1);
            return 0.0f;
        case 0x13:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_konquest_switch_1);
            return 0.0f;
        case 0x14:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_konquest_switch_1);
            return 0.0f;
        case 0x17:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_board_switch_1);
            return 0.0f;
        }
    }
    return -1.0f;
}

f32 pad_rdn_proc(void) {
    int eat_switch;

    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (is_controller_removed()) {
            eat_switch = 1;
        } else if (g_game_info.switch_input_flags.eat_switches) {
            eat_switch = 1;
        } else {
            eat_switch = 0;
        }
        if (eat_switch != 0) {
            return -1.0f;
        }

        switch (get_game_state()) {
        case 7:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, switch_proc_attack_3);
            return 0.0f;
        case 0x12:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_puzzle_switch_3);
            return 0.0f;
        case 0x13:
        case 0x16:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_konquest_switch_3);
            return 0.0f;
        case 0x14:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_konquest_switch_3);
            return 0.0f;
        case 0x17:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_board_switch_3);
            return 0.0f;
        case 0x18:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_board_switch_over_3);
            return 0.0f;
        case 3:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_atm_start_button);
            return 0.0f;
        }
    } else {
        switch (get_game_state()) {
        case 3:
            ((SwitchProcVtable*)aproc->vtbl)->jump_sleep(0.0f, p_atm_start_button);
            return 0.0f;
        }
    }
    return -1.0f;
}

int ck_eat_online_switches(void) {
    if (is_controller_removed()) {
        return 1;
    }
    return g_game_info.switch_input_flags.eat_switches != 0;
}

f32 switch_proc_right(void) {
    dash_back_check(-1.0f);
    return -1.0f;
}

/* Soft ceiling: +1.0f relocation identity awaits the rest of the TU; code exact. */
f32 switch_proc_left(void) {
    dash_back_check(1.0f);
    return -1.0f;
}

static int angle_jump_held(PlyrInfo* player) {
    PlyrPdata* pdata;

    if (player == 0 || player->slot.pdata == 0) {
        return 0;
    }
    pdata = player->slot.pdata;
    if (pdata->state == 0x900 || pdata->state == 0x901 ||
        (pdata->state & 0x4200) != 0 ||
        !check_switch(player->pad_index, 0xC)) {
        return 0;
    }
    return check_switch(player->pad_index, 0xD) ||
           check_switch(player->pad_index, 0xF);
}

f32 switch_proc_down(void) {
    PlyrInfo* player = switch_pdata->player;
    PlyrPdata* pdata;

    if (player == 0 || player->slot.pdata == 0) {
        return 0.0f;
    }
    pdata = player->slot.pdata;
    if (pdata->state != 0x900 && pdata->state != 0x901) {
        if ((pdata->state & 0x4200) == 0 &&
            check_switch(player->pad_index, 0xE)) {
            if (check_switch(player->pad_index, 0xD) ||
                check_switch(player->pad_index, 0xF)) {
                xfer_proc((MkProc*)player->idle_proc, joy_duck_loop);
            }
        }
        pdata->last_back_dash_tick = 0;
    }
    return 0.0f;
}

f32 switch_proc_up(void) {
    PlyrInfo* player = switch_pdata->player;
    int scans = 3;

    while (angle_jump_held(player) && scans-- != 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((SwitchProcVtable*)aproc->vtbl)->sleep(aproc);
    }
    if (angle_jump_held(player)) {
        if (check_switch(player->pad_index, 0xD)) {
            xfer_proc((MkProc*)player->idle_proc, x_angle_jump_right);
        }
        if (check_switch(player->pad_index, 0xF)) {
            xfer_proc((MkProc*)player->idle_proc, x_angle_jump_left);
        }
    }
    return 0.0f;
}

s32 dash_back_check(f32 direction) {
    PlyrInfo* player = switch_pdata->player;
    PlyrPdata* pdata;

    if (player == 0 || player->slot.pdata == 0) {
        return 0;
    }
    pdata = player->slot.pdata;
    plyr_pdata = pdata;
    plyr_obj = player->slot.mirror_a;
    his_obj = pdata->his_obj;

    if ((unsigned int)(exec_tick_ctr - pdata->last_back_dash_tick) < 13 &&
        direction * which_way_is_towards() < 0.0f &&
        !is_this_move_disabled_exec(0x6208)) {
        if ((pdata->state & 0x4200) == 0) {
            xfer_proc((MkProc*)player->idle_proc, joy_dash_back);
        }
    } else {
        switch_proc_up();
        switch_proc_down();
    }
    if (direction * which_way_is_towards() < 0.0f) {
        pdata->last_back_dash_tick = exec_tick_ctr;
    }
    plyr_pdata = 0;
    plyr_obj = 0;
    return 0;
}

f32 angle_jump_scan_after_move(void) {
    int port = plyr_pdata->controller_port;

    if (check_switch(port, 0xC) && check_switch(port, 0xD)) {
        return dispatch_switch(x_angle_jump_right);
    }
    if (check_switch(port, 0xC) && check_switch(port, 0xF)) {
        return dispatch_switch(x_angle_jump_left);
    }
    return 0.0f;
}
