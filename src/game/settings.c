#include "game/settings.h"

#include "game/memcard.h"
#include "platform/gcmcardmsg.h"
#include "runtime/mk_pdata.h"
#include "runtime/cstring.h"
#include "runtime/utils.h"
#include "game/nbc.h"
#include "game/menu.h"
#include "platform/gcmcard.h"
#include "mw/mwScreenEngineGlue.h"

int is_memcard_scanner_running(void);

static const float default_volume_scale = 100.0f;
static const float default_volume_offset = 0.005f;

GameSettings default_game_settings = {
    0.75f, 0.75f, 0.75f, 0.75f, 0.75f, 1.0f, 2, 2, 2, 2, 2, 60, 5, 3, 1,
    0,    0,    0,    0,    0,    0,   50, 50, 50, 50, 50, 85,
};

GameSettings game_settings;

int game_settings_status;
int game_settings_device;

static const float sleep_ticks_one = 1.0f;
static const float sleep_ticks_neg_one = -1.0f;

static float p_save_game_settings(void);

int get_kombat_difficulty(void) {
    return game_settings.kombat_difficulty;
}

void reset_default_gameplay_settings(void) {
    game_settings.kombat_difficulty = 2;
    game_settings.arcade_difficulty = 2;
    game_settings.rounds_to_win = 2;
    game_settings.round_time = 2;
    game_settings.blood_level = 2;
    game_settings.brightness = 60;
    game_settings.damage_level = 5;
    game_settings.combo_breaker = 3;
    game_settings.fatalities = 1;
}

#pragma opt_strength_reduction off
#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
void reset_default_audio_settings(void) {
    int channel;

    for (channel = 0; channel < 6; channel++) {
        game_settings.volume[channel] = default_game_settings.volume[channel];
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset
#pragma opt_strength_reduction reset

void set_game_option(int option_id, int value) {
    int index;

    index = option_id - 0x332C;
    switch (index) {
    case 0:
        if (value < 0) {
            return;
        }
        if (value >= 5) {
            return;
        }
        game_settings.kombat_difficulty = value;
        return;
    case 1:
        if (value < 0) {
            return;
        }
        if (value >= 5) {
            return;
        }
        game_settings.arcade_difficulty = value;
        return;
    case 2:
        if (value < 0) {
            return;
        }
        if (value >= 5) {
            return;
        }
        game_settings.rounds_to_win = value;
        return;
    case 3:
        if (value < 1) {
            return;
        }
        if (value > 3) {
            return;
        }
        game_settings.round_time = value;
        return;
    case 4:
        if (value < 1) {
            return;
        }
        if (value > 2) {
            return;
        }
        game_settings.blood_level = value;
        return;
    case 5:
        if (value < 20) {
            return;
        }
        if (value > 95) {
            return;
        }
        if (value < game_settings.brightness) {
            game_settings.brightness -= 5;
            if (game_settings.brightness < 20) {
                game_settings.brightness = 20;
            }
            return;
        }
        game_settings.brightness += 5;
        if (game_settings.brightness > 95) {
            game_settings.brightness = 95;
        }
        return;
    case 6:
        if (value < 0) {
            return;
        }
        if (value > 1) {
            return;
        }
        game_settings.fatalities = value;
        return;
    case 7:
        if (value < 0) {
            return;
        }
        if (value > 3) {
            return;
        }
        game_settings.combo_breaker = value;
        return;
    }
}

int get_game_option(int option_id) {
    int index;

    index = option_id - 0x332C;
    switch (index) {
    case 0:
        return game_settings.kombat_difficulty;
    case 1:
        return game_settings.arcade_difficulty;
    case 2:
        return game_settings.rounds_to_win;
    case 3:
        return game_settings.round_time;
    case 4:
        return game_settings.blood_level;
    case 5:
        return game_settings.brightness;
    case 6:
        return game_settings.fatalities;
    case 7:
        return game_settings.combo_breaker;
    default:
        return 0;
    }
}

void set_volume(int channel, int percent) {
    int clamped;

    clamped = percent;
    if (clamped < 0) {
        clamped = 0;
    }
    if (clamped > 100) {
        clamped = 100;
    }
    game_settings.volume[channel] =
        default_volume_offset + (float)clamped / default_volume_scale;
}

int get_volume(int channel) {
    return game_settings.volume[channel] * default_volume_scale;
}

#pragma dont_inline on
int save_game_settings(void) {
    const char* text;
    int result;

    for (;;) {
        text = nbc_find_text(0x3F, 1);
        result = save_settings_to_memcard_w_error(0, 2, text, &storage_status[0].settings, 0,
                                                  &storage_status[0].freeBlocks, &storage_status[0].freeBytes);
        if (result != 0) {
            game_settings_device = 0;
            return 1;
        }
        game_settings_device = -1;
        if (result == 0) {
            text = nbc_find_text(0x3F, 1);
            result = save_settings_to_memcard_w_error(1, 2, text, &storage_status[1].settings, 0,
                                                      &storage_status[1].freeBlocks, &storage_status[1].freeBytes);
            if (result != 0) {
                game_settings_device = 1;
                return 1;
            }
            game_settings_device = -1;
        }

        if (storage_status[0].status != 1 || storage_status[1].status != 1) {
            break;
        }
        mcard_msg_no_cards_at_settings();
        mcard_msg_end();
        if (mcard_msg_no_cards_at_settings_answer != 2) {
            break;
        }
    }

    if (get_language() == 2) {
        text = nbc_find_text(0x40, 1);
        mcard_msg_no_storage(text);
    } else {
        text = nbc_find_text(0x3F, 1);
        mcard_msg_no_storage(text);
    }
    mcard_msg_end();
    game_settings_device = -1;
    return 0;
}
#pragma dont_inline reset

void save_game_settings_in_action_handler(void) {
    MkProc* proc;

    proc = find_mkproc_pid(0x300D);
    if (proc == 0) {
        _create_mkproc_generic_bigstack(0x300D, 0x1F, p_save_game_settings, 0, 0);
    }
}

#pragma dont_inline on
static float p_save_game_settings(void) {
    save_game_settings();
    _mkproc_sleep_ticks = sleep_ticks_one;
    aproc->vtbl->sleep();
    fire_screen_studio_event(0x1FEB, 0);
    return sleep_ticks_neg_one;
}
#pragma dont_inline reset

static inline int find_ready_settings_device(void) {
    int count = 0;
    int device = 0;
    int found = 0;

    while (found == 0 && count < 2) {
        if (storage_status[device].status == 0) {
            found = 1;
        } else {
            device++;
            count++;
            if (device >= 2) {
                device = 0;
            }
        }
    }
    if (found != 0) {
        return device;
    }
    return -1;
}

static inline int load_game_settings_from(int device) {
    if (device < 0 || device >= 2) {
        return 0;
    }
    if (is_device_present(device) != 0) {
        memcpy(&game_settings, &storage_status[device].settings, sizeof(GameSettings));
        game_settings_status = 1;
        game_settings_device = device;
        push_video_settings();
        return 1;
    }
    return 0;
}

#pragma optimize_for_size on
#pragma use_lmw_stmw on
int load_game_settings(void) {
    int selected;
    int result = 0;

    if (is_memcard_scanner_running() == 0) {
        update_storage_status(0);
    }
    selected = find_ready_settings_device();
    if (selected != -1) {
        result = load_game_settings_from(selected);
    }
    return result;
}
#pragma optimize_for_size reset
#pragma use_lmw_stmw reset

void memory_move_game_setting(GameSettings* dst, const GameSettings* src) {
    memcpy(dst, src, sizeof(GameSettings));
}

void set_gsettings_to_default(GameSettings* dst) {
    memcpy(dst, &default_game_settings, sizeof(GameSettings));
}

int save_gsettings(int device) {
    StorageDevice* storage;
    int result;

    if (device < 0 || device >= 2) {
        result = 0;
    } else {
        storage = &storage_status[device];
        if (storage->status == 0) {
            memcpy(&storage->settings, &game_settings, sizeof(GameSettings));
            result = 1;
        } else {
            result = 0;
        }
    }
    return result;
}

void init_gsettings(void) {
    memcpy(&game_settings, &default_game_settings, sizeof(GameSettings));
    game_settings_status = 0;
    game_settings_device = -1;
}
