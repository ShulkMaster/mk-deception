#ifndef GAME_KONQUEST_MISSIONS_H
#define GAME_KONQUEST_MISSIONS_H

#ifdef __cplusplus
extern "C" {
#endif

int trial_show_standard_fight_messages(void);
void trial_state_collision_check(int collision_result, int player);
int trial_block_callback(int player);
float end_of_trial_wrapup(int winner);
int trial_change_style_callback(int player);
void skip_end_of_trial_wrapup(void);
float p_konquest_register_bleeding(void);
extern int trial_end_round(void);
extern void trial_round_init(void);
extern void trial_start_new_round(void);
float trial_run_loser_animation_script(void);
extern int trial_get_round_length(void);
extern int trial_check_state(void);
extern void trial_game_init(void);
extern void trial_setup_fight(void);
extern float p_show_text_window(void);
extern int text_window_state;
void drone_start_bleeding(int player, float rate);
void konquest_run_ending(void);
void drone_set_health(int player, float health);
void trial_set_move_message(const char* message, const char* parameter);
void trial_show_spoken_text_window(int, float, float, float, int, int, int, int, int);
void trial_restart_round(void);
void drone_set_difficulty_level(int difficulty);
void trial_set_tick_function(unsigned int script_enabled);
void trial_set_next_setup_function(int setup_function);
void drone_do_special_move(int player, int script_function);
void trial_show_text_window(int, float, float, float, int, int);
void trial_set_ending_functions(int winner_function, int loser_function);
int konquest_passed_last_mission(void);
void trial_end_tunes(void);
void drone_face_monk(void);
void trial_show_monk(int show);
void drone_set_special_directions(int unused, int flags);
void drone_set_anim_step(float step);
void drone_set_position(int, float, float, float);
void trial_set_special_restrictions(unsigned int restrictions);
void trial_add_success_condition(int index, int value, int required_count);
void trial_set_num_rounds(int rounds);
void drone_set_handicap(int player, float handicap);
void trial_set_next_mission(int, int, int, int, int, int, int, int);
void give_koin_award(int amount, int type);
void drone_set_damage_multiplier(int player, float multiplier);
void trial_start_tunes(void);
void trial_do_dialog(int, int, float, float, float, unsigned int, int);
void trial_start_countdown(int, float, float);
void cleanup_mission_state(void);
void trial_set_round_health_restoration(float restoration);
void drone_set_script(int, int);
void drone_change_to_style(int player, int style);
void trial_setup_nis_scene(int);
int trial_get_background_root(void);
void drone_apply_damage(int player, float damage);
void drone_set_switch_state(int player, unsigned int switch_state);
int trial_never_passed_this_mission(void);
void show_text(int, unsigned int, unsigned int, float, float, float, unsigned int, int);
void trial_set_type(int type);
void drone_dispatch_switches(int player);
void trial_set_round_timer(int ticks);
extern char danton20_charmap[];
void trial_set_combo_requirement(int hits, float damage);
float trial_damage_callback(int player, int damage_type, float damage);
void trial_mirror_anims_if_needed(void);
void play_background_music(int sound);

#ifdef __cplusplus
}
#endif

#endif
