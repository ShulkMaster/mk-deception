#include "game/settings.h"

#include "game/memcard.h"
#include "platform/gcmcardmsg.h"
#include "runtime/mk_pdata.h"

extern int mcard_msg_no_cards_at_settings_answer;

void* memcpy(void* dst, const void* src, int size);
const char* nbc_find_text(int index, int table);
int get_language(void);
int update_storage_status(int arg);
int is_memcard_scanner_running(void);
int is_device_present(int device);
void push_video_settings(void);
void fire_screen_studio_event(int event, int arg);

static const float default_volume_scale = 100.0f;
static const float default_volume_offset = 0.005f;
static const double int_to_float_bias = 4503601774854144.0;

GameSettings default_game_settings = {
    0.75f, 0.75f, 0.75f, 0.75f, 0.75f, 1.0f, 2, 2, 2, 2, 2, 60, 5, 3, 1,
    0,    0,    0,    0,    0,    0,   50, 50, 50, 50, 50, 85,
};

GameSettings game_settings;
const int gap_05_8033E3C4_data = 0;
int gap_06_803B34AC_bss;

/* MWCC emits .sbss in reverse declaration order. */
int game_settings_status;
int game_settings_device;

static const float sleep_ticks_one = 1.0f;
static const float sleep_ticks_neg_one = -1.0f;

typedef struct MkVtableMkprocLocal {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
    int (*system_stack)(void);
    int (*local_stack)(void);
    float (*jump_sleep)(MkProcEntryFn entry);
} MkVtableMkprocLocal;

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

/* Soft ceiling: reset_default_audio_settings ~61.58% -- typed copy is exact;
 * residual is byte-index lfsx/stfsx scheduling versus MWCC clrrwi emission. */
#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
void reset_default_audio_settings(void) {
    int offset;

    for (offset = 0; offset < 24; offset += 4) {
        game_settings.volume[offset >> 2] =
            default_game_settings.volume[offset >> 2];
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

/* Soft ceiling: set_game_option ~98.30% -- brightness base/value coloring; stop. */
void set_game_option(int option_id, int value) {
    int index;
    int current;
    int next;

    index = option_id - 0x332C;
    if ((unsigned)index > 7U) {
        return;
    }
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
    case 5: {
        GameSettings* settings;

        if (value < 20) {
            return;
        }
        if (value > 95) {
            return;
        }
        settings = &game_settings;
        current = settings->brightness;
        if (value < current) {
            next = current - 5;
            settings->brightness = next;
            if (next >= 20) {
                return;
            }
            settings->brightness = 20;
            return;
        }
        next = current + 5;
        settings->brightness = next;
        if (next <= 95) {
            return;
        }
        settings->brightness = 95;
        return;
    }
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
/* Soft ceiling: set_volume ~99.38% -- instructions match; only generated
 * float/double constant relocation labels differ. */

int get_volume(int channel) {
    return (int)(game_settings.volume[channel] * default_volume_scale);
}
/* Soft ceiling: get_volume ~99.58% -- instructions match; only the generated
 * 100.0f constant relocation label differs. */

#pragma dont_inline on
int save_game_settings(void) {
    StorageDevice* storage;
    int* device0_free_bytes;
    unsigned int* device0_free_blocks;
    GameSettings* device0_settings;
    int* device1_free_bytes;
    unsigned int* device1_free_blocks;
    GameSettings* device1_settings;
    const char* text;
    int result;

    storage = storage_status;
    device0_free_bytes = &storage[0].freeBytes;
    device0_free_blocks = &storage[0].freeBlocks;
    device0_settings = &storage[0].settings;
    device1_free_bytes = &storage[1].freeBytes;
    device1_free_blocks = &storage[1].freeBlocks;
    device1_settings = &storage[1].settings;
    for (;;) {
        text = nbc_find_text(0x3F, 1);
        result = save_settings_to_memcard_w_error(0, 2, text, device0_settings, 0,
                                                  device0_free_blocks, device0_free_bytes);
        if (result != 0) {
            game_settings_device = 0;
            return 1;
        }
        game_settings_device = -1;
        if (result == 0) {
            text = nbc_find_text(0x3F, 1);
            result = save_settings_to_memcard_w_error(1, 2, text, device1_settings, 0,
                                                      device1_free_blocks, device1_free_bytes);
            if (result != 0) {
                game_settings_device = 1;
                return 1;
            }
        }
        game_settings_device = -1;

        if (storage[0].status != 1 || storage[1].status != 1) {
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
/* Soft ceiling: save_game_settings ~97.41% -- typed field pointers and retry
 * CFG match retail; residual is status-load versus SDA-store scheduling. */
#pragma dont_inline reset

void save_game_settings_in_action_handler(void) {
    MkProc* proc;

    proc = find_mkproc_pid(0x300D);
    if (proc == 0) {
        _create_mkproc_generic_bigstack(0x300D, 0x1F, (MkProcEntryFn)p_save_game_settings, 0, 0);
    }
}

/* mkproc 0x300D: write settings, sleep one tick, notify screen studio; return -1. */
#pragma dont_inline on
static float p_save_game_settings(void) {
    MkVtableMkprocLocal* vtbl;

    save_game_settings();
    _mkproc_sleep_ticks = sleep_ticks_one;
    vtbl = (MkVtableMkprocLocal*)aproc->vtbl;
    vtbl->sleep();
    fire_screen_studio_event(0x1FEB, 0);
    return sleep_ticks_neg_one;
}
/* Soft ceiling: p_save_game_settings ~99.47% -- instructions match; only the
 * generated sleep/return float relocation labels differ. */
#pragma dont_inline reset

int load_game_settings(void) {
    int device;
    int selected;
    int found;
    int count;
    int result;

    result = 0;
    if (is_memcard_scanner_running() == 0) {
        update_storage_status(0);
    }
    device = 0;
    found = 0;
    count = 0;
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
    selected = -1;
    if (found != 0) {
        selected = device;
    }
    if (selected != -1) {
        if (selected < 0 || selected >= 2) {
            result = 0;
        } else if (is_device_present(selected) != 0) {
            memcpy(&game_settings, &storage_status[selected].settings, sizeof(GameSettings));
            game_settings_status = 1;
            game_settings_device = selected;
            push_video_settings();
            result = 1;
        } else {
            result = 0;
        }
    }
    return result;
}
/* Soft ceiling: load_game_settings ~81.41% -- typed device scan/load algorithm
 * is retail-correct; residual is loop/result register and branch scheduling. */

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
    memcpy(&game_settings, &default_game_settings, 0x6C);
    game_settings_status = 0;
    game_settings_device = -1;
}
