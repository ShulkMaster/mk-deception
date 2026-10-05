#include "game/switch.h"
#include "game/controller.h"
#include "game/plyr_globals.h"
#include "game/mk_chess.h"
#include "game/menu.h"
#include "game/konquest.h"
#include "game/minigames.h"
#include "game/moves.h"
#include "platform/joy.h"
#include "platform/io.h"
#include "game/ejb.h"
#include "game/attract.h"
#include "game/game_info.h"
#include "game/game.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_vtbl.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_obj.h"
#include "platform/display.h"
#include "platform/main.h"
#include "runtime/utils.h"

static float switch_proc_up(void);
static float switch_proc_down(void);
float switch_proc_left(void);
float switch_proc_right(void);

static void dash_back_check(float direction);

static inline int switch_input_eaten(void) {
    if (is_controller_removed()) {
        return 1;
    }
    if (g_game_info.switch_input_flags.eat_switches) {
        return 1;
    }
    return 0;
}

static float dispatch_switch(MkProcEntryFn entry) {
    aproc->vtbl->jump_sleep(entry, 0.0f);
    return 0.0f;
}

float pad_rt_stick_btn_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        return -1.0f;
    }
    return -1.0f;
}

float pad_lt_stick_btn_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        switch (get_game_state()) {
        case 7: break;
        case 0x12: return dispatch_switch(p_puzzle_switch_lt_stick);
        case 2:
            if (switch_pdata->player->pad_index == 0) {
                return dispatch_switch(p_swap_levels);
            }
            break;
        }
    }
    return -1.0f;
}

float pad_select_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        get_game_state();
    }
    return -1.0f;
}

float pad_start_proc(void) {
    int state;

    if (is_plyr_controller_enabled(switch_pdata->player) &&
        (switch_pdata->player->player_state == 2 ||
         switch_pdata->player->player_state == 1)) {
        switch (get_game_state()) {
        case 7:
        case 0x12:
        case 0x17:
            aproc->vtbl->jump_sleep(p_pause_menu_switch, 0.0f);
            return 0.0f;
        case 0x13:
        case 0x14:
            aproc->vtbl->jump_sleep(p_switch_proc_start, 0.0f);
            return 0.0f;
        case 3:
            aproc->vtbl->jump_sleep(p_atm_start_button, 0.0f);
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
            case 7:
                if (ok_to_join_in() && proc_create(do_join_in, 0x2073) != 0) {
                    ((struct JoinInPdata*)mab_generic_pdata)->player = switch_pdata->player->pad_index;
                }
                break;
            case 3:
                aproc->vtbl->jump_sleep(p_atm_start_button, 0.0f);
                return 0.0f;
            }
        } else if (get_game_state() == 7 && are_controllers_locked() &&
                   g_game_info.flag_bits.lens_flare_enabled == 0 &&
                   g_game_info.flag_bits.field_bit6 != 0) {
            aproc->vtbl->jump_sleep(p_pause_menu_switch, 0.0f);
            return 0.0f;
        }
    }
    return -1.0f;
}

float pad_r2_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        switch (get_game_state()) {
        case 7: return dispatch_switch(p_block);
        case 0x12: return dispatch_switch(p_puzzle_switch_drop);
        }
    }
    return -1.0f;
}

float pad_r1_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        switch (get_game_state()) {
        case 7: return dispatch_switch(switch_proc_attack_5);
        case 0x17: return dispatch_switch(p_board_switch_r2);
        case 0x13: return dispatch_switch(p_konquest_switch_R1);
        }
    }
    return -1.0f;
}

float pad_l2_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        switch (get_game_state()) {
        case 7: return dispatch_switch(switch_proc_pickup);
        case 2: break;
        }
    }
    return -1.0f;
}

float pad_l1_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        switch (get_game_state()) {
        case 7: return dispatch_switch(switch_proc_advance_moveset);
        case 0x17: return dispatch_switch(p_board_switch_l1);
        }
    }
    return -1.0f;
}

float pad_rrt_proc(void) {
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
            aproc->vtbl->jump_sleep(switch_proc_attack_4, 0.0f);
            return 0.0f;
        case 0x12:
            aproc->vtbl->jump_sleep(p_puzzle_switch_4, 0.0f);
            return 0.0f;
        case 0x13:
            aproc->vtbl->jump_sleep(p_konquest_switch_4, 0.0f);
            return 0.0f;
        case 0x14:
            aproc->vtbl->jump_sleep(p_konquest_switch_4, 0.0f);
            return 0.0f;
        case 0x17:
            aproc->vtbl->jump_sleep(p_board_switch_4, 0.0f);
            return 0.0f;
        }
    }
    return -1.0f;
}

float pad_rlt_proc(void) {
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
            aproc->vtbl->jump_sleep(switch_proc_attack_1, 0.0f);
            return 0.0f;
        case 0x12:
            aproc->vtbl->jump_sleep(p_puzzle_switch_1, 0.0f);
            return 0.0f;
        case 0x13:
            aproc->vtbl->jump_sleep(p_konquest_switch_1, 0.0f);
            return 0.0f;
        case 0x14:
            aproc->vtbl->jump_sleep(p_konquest_switch_1, 0.0f);
            return 0.0f;
        case 0x17:
            aproc->vtbl->jump_sleep(p_board_switch_1, 0.0f);
            return 0.0f;
        }
    }
    return -1.0f;
}

float pad_rdn_proc(void) {
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
            aproc->vtbl->jump_sleep(switch_proc_attack_3, 0.0f);
            return 0.0f;
        case 0x12:
            aproc->vtbl->jump_sleep(p_puzzle_switch_3, 0.0f);
            return 0.0f;
        case 0x13:
        case 0x16:
            aproc->vtbl->jump_sleep(p_konquest_switch_3, 0.0f);
            return 0.0f;
        case 0x14:
            aproc->vtbl->jump_sleep(p_konquest_switch_3, 0.0f);
            return 0.0f;
        case 0x17:
            aproc->vtbl->jump_sleep(p_board_switch_3, 0.0f);
            return 0.0f;
        case 0x18:
            aproc->vtbl->jump_sleep(p_board_switch_over_3, 0.0f);
            return 0.0f;
        case 3:
            aproc->vtbl->jump_sleep(p_atm_start_button, 0.0f);
            return 0.0f;
        }
    } else {
        switch (get_game_state()) {
        case 3:
            aproc->vtbl->jump_sleep(p_atm_start_button, 0.0f);
            return 0.0f;
        }
    }
    return -1.0f;
}

float pad_rup_proc(void) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        switch (get_game_state()) {
        case 7: return dispatch_switch(switch_proc_attack_2);
        case 0x12: return dispatch_switch(p_puzzle_switch_2);
        case 0x17: return dispatch_switch(p_board_switch_2);
        case 0x13:
        case 0x14: return dispatch_switch(p_konquest_inventory_switch);
        }
    }
    return -1.0f;
}

static float dispatch_direction(
    MkProcEntryFn fight_entry, MkProcEntryFn puzzle_entry) {
    if (is_plyr_controller_enabled(switch_pdata->player)) {
        if (switch_input_eaten()) {
            return -1.0f;
        }
        switch (get_game_state()) {
        case 7: return dispatch_switch(fight_entry);
        case 0x12: return dispatch_switch(puzzle_entry);
        }
    }
    return -1.0f;
}

float pad_lrt_proc(void) {
    return dispatch_direction(switch_proc_right, p_puzzle_switch_right);
}

float pad_llt_proc(void) {
    return dispatch_direction(switch_proc_left, p_puzzle_switch_left);
}

float pad_ldn_proc(void) {
    return dispatch_direction(switch_proc_down, p_puzzle_switch_down);
}

float pad_lup_proc(void) {
    return dispatch_direction(switch_proc_up, p_puzzle_switch_up);
}

/* TODO: [near miss] 87.78%; unsigned bit-result normalization boundary unresolved;
 * existing complete predicate reuse adds branches; retain direct guarded read. */
int ck_eat_online_switches(void) {
    if (is_controller_removed()) {
        return 1;
    }
    return g_game_info.switch_input_flags.eat_switches != 0;
}

float switch_proc_right(void) {
    dash_back_check(-1.0f);
    return -1.0f;
}

float switch_proc_left(void) {
    dash_back_check(1.0f);
    return -1.0f;
}

static int angle_jump_held(void) {
    PlyrInfo* player = switch_pdata->player;
    int state;

    if (player == 0) {
        return 0;
    }
    state = player->slot.pdata->state;
    if (state == 0x900) {
        return 0;
    }
    if (state == 0x901) {
        return 0;
    }
    if ((state & 0x4200) == 0 && check_switch(player->pad_index, 0xC)) {
        if (check_switch(player->pad_index, 0xD)) {
            return 1;
        }
        if (check_switch(player->pad_index, 0xF)) {
            return 1;
        }
    }
    return 0;
}

static inline void set_switch_player_globals(PlyrInfo* player) {
    plyr_pdata = player->slot.pdata;
    plyr_obj = player->slot.mirror_a;
    his_obj = player->slot.pdata->his_obj;
}

static inline float check_player_duck(PlyrInfo* player) {
    int state;

    if (player != 0) {
        state = player->slot.pdata->state;
        if (state != 0x900 && state != 0x901) {
            if ((state & 0x4200) == 0) {
                if (check_switch(player->pad_index, 0xE) &&
                    check_switch(player->pad_index, 0xD)) {
                    xfer_proc(player->idle_proc, joy_duck_loop);
                }
                if (check_switch(player->pad_index, 0xE) &&
                    check_switch(player->pad_index, 0xF)) {
                    xfer_proc(player->idle_proc, joy_duck_loop);
                }
            }
            player->slot.pdata->last_back_dash_tick = 0;
        }
    }
    return -1.0f;
}

static float switch_proc_down(void) {
    return check_player_duck(switch_pdata->player);
}

#pragma optimize_for_size on
#pragma use_lmw_stmw on
static inline float wait_for_angle_jump(PlyrInfo* player, int scans) {
    for (;;) {
        if (!angle_jump_held()) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
        if (--scans == 0) {
            if (angle_jump_held()) {
                if (check_switch(player->pad_index, 0xD)) {
                    xfer_proc(player->idle_proc, x_angle_jump_right);
                }
                if (check_switch(player->pad_index, 0xF)) {
                    xfer_proc(player->idle_proc, x_angle_jump_left);
                }
            }
            break;
        }
    }
    return -1.0f;
}

static float switch_proc_up(void) {
    return wait_for_angle_jump(switch_pdata->player, 3);
}
#pragma optimize_for_size reset
#pragma use_lmw_stmw reset

#pragma optimize_for_size on
#pragma use_lmw_stmw on
static void dash_back_check(float direction) {
    PlyrInfo* player = switch_pdata->player;
    MkProc* idle_proc;

    if (player != 0) {
        set_switch_player_globals(player);
        idle_proc = player->idle_proc;

        if ((unsigned int)(exec_tick_ctr - plyr_pdata->last_back_dash_tick) < 13 &&
            direction * which_way_is_towards() < 0.0f &&
            !is_this_move_disabled_exec(0x6208)) {
            if ((player->slot.pdata->state & 0x4200) == 0) {
                xfer_proc(idle_proc, joy_dash_back);
            }
        } else {
            switch_proc_up();
            set_switch_player_globals(player);
            switch_proc_down();
        }
        if (direction * which_way_is_towards() < 0.0f) {
            plyr_pdata->last_back_dash_tick = exec_tick_ctr;
        }
        plyr_pdata = 0;
        plyr_obj = 0;
    }
}
#pragma optimize_for_size reset
#pragma use_lmw_stmw reset

float angle_jump_scan_after_move(void) {
    if (check_switch(plyr_pdata->controller_port, 0xC) &&
        check_switch(plyr_pdata->controller_port, 0xD)) {
        return dispatch_switch(x_angle_jump_right);
    }
    if (check_switch(plyr_pdata->controller_port, 0xC) &&
        check_switch(plyr_pdata->controller_port, 0xF)) {
        return dispatch_switch(x_angle_jump_left);
    }
    return 0.0f;
}
