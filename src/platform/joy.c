#include "platform/joy.h"

#include "game/game_info.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_anim_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"

typedef struct JoyProcVtable {
    void* slots_00[6];
    void (*sleep)(void);
    void* slots_1C[2];
    float (*jump_sleep)(MkProcEntryFn entry, float delay);
} JoyProcVtable;

/* ABI boundary: MkProc exposes its generic runtime vtable type. */
#define JOY_PROC_VTABLE(proc) ((JoyProcVtable*)(proc)->vtbl)

extern float which_way_is_towards(void);
extern void jump_away_opponent(void);
extern void jump_towards_opponent(void);
extern float j_exit_blend_stance(void);
extern float j_exit(void);
extern void blend_to_ani(AniData* animation, int flags, float blend_time);
extern void set_my_state(int state);
extern void trial_clear_provision(void);
extern PlyrPdata* his_pdata;
extern int is_this_move_disabled_exec(int move);
extern float xz_distance_between_players(void);
extern void trial_increment_state_value(int player, int state, int value);
extern void snd_req(int sound);
extern MkProc* plyr_anim_proc;
extern float p_animate(void);
extern int round_winner;
extern int f_fatality_available;
extern int my_next_duck_state;
extern unsigned int game_tick_ctr;
extern void init_ground_move(PlyrPdata* pdata, PlyrAnimPdata* anim_pdata);
extern void back_to_normal(void);
extern void rotate_towards_him(float blend_time);
extern void head_tracking_on(void);
extern int check_for_dead_movement(void);
extern int check_switch(SwitchData* switches, int button);
extern int exec_tick_ctr;
extern int g_drone_blocking_in_reaction;
extern void drone_ai_finished_request(void);
extern int am_i_blocking(void);
extern int should_i_weapon_block();
extern float j_duck_block_loop(void);
extern float x_block(void);
extern float drone_start(void);
extern void disable_this_move_exec(int move, int duration);
extern void enable_this_move_exec(int move);
extern void end_of_round_check(void);
extern void angle_jump_scan_after_move(void);
extern float get_my_angle_y_error(void);
extern int am_i_flipped_or_turned(void);
extern void blend_to_stance(float blend_time);
extern float step_backward(void);
extern float step_forward(void);
extern float step_left(void);
extern float step_right(void);
extern MkObj* plyr_obj;

typedef struct JoySharedAnimations {
    unsigned char pad00[0x2E4];
    AniData* duck_block_animation;
} JoySharedAnimations;

extern JoySharedAnimations shared_ani;

static void jump_to(MkProcEntryFn entry) {
    JOY_PROC_VTABLE(aproc)->jump_sleep(entry, 0.0f);
}

void dodge_3d_scan(void) {
    if (his_pdata->throw_restriction != 3 &&
        (his_pdata->state & 0x1000) != 0 &&
        is_this_move_disabled_exec(0x6004) == 0 &&
        xz_distance_between_players() < 9.0f) {
        trial_increment_state_value(plyr_pdata->plyr_num, 0x15, 0);
        plyr_anim_pdata->playback_rate = 1.75f;
        plyr_anim_pdata->field_64 = 1.2f;
        if (plyr_pdata->dodge_sound_played == 0) {
            snd_req(0xDC3);
            plyr_pdata->dodge_sound_played = 1;
        }
    }
}

float joy_duck_loop(void) {
    back_to_normal();
    if (my_next_duck_state != 0) {
        plyr_pdata->state = my_next_duck_state;
        my_next_duck_state = 0;
    }
    if (plyr_pdata->drone_request != 0) {
        drone_ai_finished_request();
    }
    if (round_winner != 0 && f_fatality_available == 0) {
        if (plyr_pdata->state & 0x100) {
            blend_to_ani(plyr_pdata->fighter_definition->duck_exit_animation,
                         0x20, 0.1f);
            _mkproc_sleep_ticks = 5.0f;
            JOY_PROC_VTABLE(aproc)->sleep();
            set_my_state(0);
        }
        _mkproc_sleep_ticks = 1.0f;
        JOY_PROC_VTABLE(aproc)->sleep();
        jump_to(j_exit);
        return 0.0f;
    }

    xfer_proc(plyr_anim_proc, p_animate);
    plyr_pdata->duck_loop_counter = 10;
    if (plyr_pdata->state != 0x101) {
        plyr_pdata->duck_loop_counter = 0;
        set_my_state(0x100);
        if (plyr_anim_pdata->current_animation !=
            plyr_pdata->fighter_definition->duck_animation) {
            plyr_anim_pdata->flags |= 0x40;
            blend_to_ani(plyr_pdata->fighter_definition->duck_animation, 0,
                         0.1f);
        }
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 0x11, 0);

    while (check_switch(plyr_pdata->switch_data, 0xE) == 0 &&
           (plyr_pdata->drone_request == 0 || plyr_pdata->field_728 == 1)) {
        if (check_for_dead_movement() != 0) {
            break;
        }
        if (plyr_pdata->drone_request == 0) {
            if (am_i_blocking() != 0) {
                if (should_i_weapon_block() != 0) {
                    blend_to_ani(
                        plyr_pdata->fighter_definition->duck_block_animation,
                        0, 0.1f);
                } else {
                    blend_to_ani(shared_ani.duck_block_animation, 0, 0.1f);
                }
                plyr_anim_pdata->playback_rate = 1.0f;
                jump_to(j_duck_block_loop);
                return 0.0f;
            }
        } else if (plyr_pdata->field_728 == 2) {
            plyr_pdata->state = 0x900;
            plyr_pdata->block_start_tick = exec_tick_ctr;
            g_drone_blocking_in_reaction = 0;
            if (should_i_weapon_block(plyr_pdata, exec_tick_ctr) != 0) {
                blend_to_ani(
                    plyr_pdata->fighter_definition->duck_block_animation, 0,
                    0.1f);
            } else {
                blend_to_ani(shared_ani.duck_block_animation, 0, 0.1f);
            }
            plyr_anim_pdata->playback_rate = 1.0f;
            jump_to(j_duck_block_loop);
            return 0.0f;
        } else if (plyr_pdata->field_728 == 3) {
            plyr_pdata->state = 0xA00;
            blend_to_ani(plyr_pdata->fighter_definition->duck_exit_animation,
                         0x20, 0.1f);
            _mkproc_sleep_ticks = 5.0f;
            JOY_PROC_VTABLE(aproc)->sleep();
            set_my_state(0);
            jump_to(x_block);
            return 0.0f;
        }

        plyr_pdata->duck_loop_counter++;
        if (his_pdata->state & 0x1000) {
            plyr_pdata->duck_loop_counter += 3;
        }
        if (plyr_pdata->duck_loop_counter > 5) {
            set_my_state(0x101);
        }
        _mkproc_sleep_ticks = 1.0f;
        JOY_PROC_VTABLE(aproc)->sleep();
    }

    blend_to_ani(plyr_pdata->fighter_definition->duck_exit_animation, 0x20,
                 0.1f);
    _mkproc_sleep_ticks = 5.0f;
    JOY_PROC_VTABLE(aproc)->sleep();
    set_my_state(0);
    trial_clear_provision();
    jump_to(j_exit);
    return 0.0f;
}

float joy_duck_remote_end(void) {
    blend_to_ani(plyr_pdata->fighter_definition->duck_exit_animation, 0x20,
                 0.1f);
    _mkproc_sleep_ticks = 5.0f;
    JOY_PROC_VTABLE(aproc)->sleep();
    set_my_state(0);
    trial_clear_provision();
    jump_to(j_exit);
    return 0.0f;
}

float joy_duck_remote_start(void) {
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_pdata->duck_loop_counter = 10;
    if (plyr_pdata->state != 0x101) {
        plyr_pdata->duck_loop_counter = 0;
        set_my_state(0x100);
        if (plyr_anim_pdata->current_animation !=
            plyr_pdata->fighter_definition->duck_animation) {
            plyr_anim_pdata->flags |= 0x40;
            blend_to_ani(plyr_pdata->fighter_definition->duck_animation, 0,
                         0.1f);
        }
    }
    trial_increment_state_value(plyr_pdata->plyr_num, 0x11, 0);
    while (round_winner == 0 || f_fatality_available != 0) {
        plyr_pdata->duck_loop_counter++;
        if (his_pdata->state & 0x1000) {
            plyr_pdata->duck_loop_counter += 3;
        }
        if (plyr_pdata->duck_loop_counter > 5) {
            set_my_state(0x101);
        }
        _mkproc_sleep_ticks = 1.0f;
        JOY_PROC_VTABLE(aproc)->sleep();
    }
    jump_to(j_exit);
    return 0.0f;
}

float p_joy_loop(void) {
    int angle_outside_limit;

    if (plyr_pdata->drone_handoff_pending != 0) {
        plyr_pdata->drone_handoff_pending = 0;
        jump_to(drone_start);
        return 0.0f;
    }

    if (plyr_pdata->special_move_disabled == 1) {
        if (check_switch(plyr_pdata->switch_data, 0xC) != 0 ||
            check_switch(plyr_pdata->switch_data, 0xE) != 0) {
            disable_this_move_exec(0x6004, 30);
        } else {
            plyr_pdata->special_move_disabled = 0;
            enable_this_move_exec(0x6004);
        }
    }

    end_of_round_check();
    if (g_game_info.pause_flag_bits.controllers_disabled) {
        return 1.0f;
    }
    if (!player_control_allowed()) {
        return 1.0f;
    }
    if (am_i_blocking()) {
        jump_to(x_block);
        return 0.0f;
    }

    if (plyr_pdata->angle_jump_pending != 0) {
        angle_jump_scan_after_move();
    }
    plyr_pdata->angle_jump_pending = 0;

    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        check_switch(plyr_pdata->switch_data, 0xF) != 0) {
        jump_to(joy_duck_loop);
        return 0.0f;
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        check_switch(plyr_pdata->switch_data, 0xD) != 0) {
        jump_to(joy_duck_loop);
        return 0.0f;
    }

    if (check_switch(plyr_pdata->switch_data, 0xD) != 0) {
        angle_outside_limit = get_my_angle_y_error() > 2.7f;
        if ((float)angle_outside_limit >= 0.0f) {
            angle_outside_limit = get_my_angle_y_error() > 2.7f;
        } else {
            angle_outside_limit = -(get_my_angle_y_error() > 2.7f);
        }
        if (angle_outside_limit != 0) {
            rotate_towards_him(0.2f);
            jump_to(j_exit);
            return 0.0f;
        }
        if (which_way_is_towards() > 0.0f) {
            jump_to(step_backward);
        } else {
            jump_to(step_forward);
        }
        return 0.0f;
    }

    if (check_switch(plyr_pdata->switch_data, 0xF) != 0) {
        angle_outside_limit = get_my_angle_y_error() > 2.7f;
        if ((float)angle_outside_limit >= 0.0f) {
            angle_outside_limit = get_my_angle_y_error() > 2.7f;
        } else {
            angle_outside_limit = -(get_my_angle_y_error() > 2.7f);
        }
        if (angle_outside_limit != 0) {
            rotate_towards_him(0.2f);
            jump_to(j_exit);
            return 0.0f;
        }
        if (which_way_is_towards() < 0.0f) {
            jump_to(step_backward);
        } else {
            jump_to(step_forward);
        }
        return 0.0f;
    }

    if (check_switch(plyr_pdata->switch_data, 0xE) != 0) {
        if (!am_i_flipped_or_turned()) {
            jump_to(step_right);
        } else {
            jump_to(step_left);
        }
        return 0.0f;
    }

    if (!g_game_info.feature_flags.bits.high_bit &&
        plyr_anim_pdata->current_animation ==
            plyr_pdata->fighter_definition->duck_animation) {
        set_my_state(0);
        blend_to_stance(0.1f);
        jump_to(j_exit);
        return 0.0f;
    }

    if (check_switch(plyr_pdata->switch_data, 0xC) != 0) {
        if (which_way_is_towards() > 0.0f) {
            if (!plyr_obj->hide_flag_bits.bit6) {
                jump_to(step_right);
            } else {
                jump_to(step_left);
            }
        } else if (plyr_obj->hide_flag_bits.bit6) {
            jump_to(step_right);
        } else {
            jump_to(step_left);
        }
        return 0.0f;
    }
    return 1.0f;
}

float p_joy_entry(void) {
    float playback_rate = plyr_anim_pdata->playback_rate;
    int saved_state = plyr_pdata->state;
    float field_80 = plyr_anim_pdata->field_80;
    float field_AC = plyr_anim_pdata->field_AC;
    int waited = 0;

    init_ground_move(plyr_pdata, plyr_anim_pdata);
    back_to_normal();
    xfer_proc(plyr_anim_proc, p_animate);
    plyr_anim_pdata->playback_rate = playback_rate;
    plyr_anim_pdata->field_80 = field_80;
    plyr_anim_pdata->field_AC = field_AC;
    rotate_towards_him(0.2f);
    while (plyr_pdata->action_lock_a > game_tick_ctr) {
        waited++;
        if (waited > 60) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        JOY_PROC_VTABLE(aproc)->sleep();
    }
    head_tracking_on();
    if (check_for_dead_movement() == 1) {
        jump_to(p_joy_loop);
        return 0.0f;
    }
    if ((saved_state & 0x100) &&
        check_switch(plyr_pdata->switch_data, 0xE) != 0) {
        my_next_duck_state = 0x101;
        jump_to(joy_duck_loop);
        return 0.0f;
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        check_switch(plyr_pdata->switch_data, 0xF) != 0) {
        jump_to(joy_duck_loop);
        return 0.0f;
    }
    if (check_switch(plyr_pdata->switch_data, 0xE) != 0 &&
        check_switch(plyr_pdata->switch_data, 0xD) != 0) {
        jump_to(joy_duck_loop);
        return 0.0f;
    }
    if (check_switch(plyr_pdata->switch_data, 0xC) != 0 ||
        check_switch(plyr_pdata->switch_data, 0xE) != 0) {
        if (aproc->pid == 0x1001) {
            plyr_pdata->special_move_disabled = 1;
        }
        plyr_pdata->special_move_disabled = 1;
    }
    plyr_pdata->angle_jump_pending = 1;
    jump_to(p_joy_loop);
    return 0.0f;
}

float p_joy_start(void) {
    jump_to(p_joy_loop);
    return 0.0f;
}

float x_angle_jump_left(void) {
    if (which_way_is_towards() < 0.0f) {
        jump_away_opponent();
    } else {
        jump_towards_opponent();
    }
    jump_to(j_exit_blend_stance);
    return 0.0f;
}

float x_angle_jump_right(void) {
    if (which_way_is_towards() > 0.0f) {
        jump_away_opponent();
    } else {
        jump_towards_opponent();
    }
    jump_to(j_exit_blend_stance);
    return 0.0f;
}
