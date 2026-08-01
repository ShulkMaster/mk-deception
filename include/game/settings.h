#ifndef SETTINGS_H
#define SETTINGS_H

/*
 * Game settings blob (settings.o / utils getters).
 * Offsets from get_blood_level / get_puzzle_rounds_to_win / are_death_traps_on.
 */
typedef struct GameSettings {
    float volume[6];           /* +0x00 */
    int kombat_difficulty;     /* +0x18 */
    int arcade_difficulty;     /* +0x1C */
    int rounds_to_win;         /* +0x20 */
    int round_time;            /* +0x24 */
    int blood_level;           /* +0x28 - retail get_puzzle_rounds_to_win */
    union {
        int brightness;
        int round_timer_value; /* +0x2C - reset_game_timer */
    };
    int damage_level;          /* +0x30 */
    int combo_breaker;         /* +0x34 - retail get_blood_level / death-trap gate */
    int fatalities;            /* +0x38 - death traps */
    int pad_3C;                /* +0x3C */
    int konquest_latch;        /* +0x40 - cleared on Konquest menu entry */
    int pad_44[4];             /* +0x44 */
    int color_red;             /* +0x54 - video calibrate (menu getters) */
    int color_blue;            /* +0x58 */
    int color_green;           /* +0x5C */
    int display_brightness;    /* +0x60 - GC display props (!= brightness +0x2C) */
    int gamma;                 /* +0x64 */
    int contrast;              /* +0x68 */
} GameSettings;

extern GameSettings game_settings;

int get_kombat_difficulty(void);
void reset_default_gameplay_settings(void);
void reset_default_audio_settings(void);
void set_game_option(int option_id, int value);
int get_game_option(int option_id);
void set_volume(int channel, int percent);
int get_volume(int channel);
int save_game_settings(void);
void save_game_settings_in_action_handler(void);
int load_game_settings(void);
void memory_move_game_setting(GameSettings* dst, const GameSettings* src);
void set_gsettings_to_default(GameSettings* dst);
int save_gsettings(int device);
void init_gsettings(void);

#endif
