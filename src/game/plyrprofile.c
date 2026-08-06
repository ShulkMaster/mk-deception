#include "game/plyrprofile.h"

#include "game/attract.h"
#include "game/game_info.h"
#include "game/konquest_save.h"
#include "game/memcard.h"
#include "game/menu.h"
#include "platform/gcmcardmsg.h"
#include "platform/main.h"
#include "platform/main_jump.h"
#include "runtime/cam.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_info.h"
#include "runtime/section.h"
#include "runtime/utils.h"

/*
 * plyrprofile.o - profiles + boot PPWLS (B21) + menu create/view/delete (B22).
 * See docs/campaigns/index.md (B20-B22).
 */

#pragma use_lmw_stmw on

void* memset(void* d, int c, unsigned long n);
void* memcpy(void* d, const void* s, unsigned long n);
int memcmp(const void* a, const void* b, unsigned long n);
char* strcpy(char* d, const char* s);
char* nbc_find_text(int a, int b);
void load_screen(const char* path, int slot, int a, int b);
int update_storage_status(int flag);
void gc_boot_space_check(void);
void destroy_mkprocs_pid(int pid);
void set_wls_left_cursor(int v);
void fire_screen_studio_event(int id, int arg);
void reset_format_or_recreate_flags(void);
void check_format_or_recreate(void);
void turn_camera_on(void);
void turn_camera_off(void);
void turn_controllers_on(void);
void turn_controllers_off(void);
void turn_all_ports_on(void);
void disable_all_ports_but_me(int port);
void ck_for_controller_removed(void);
void switch_map_unload_player_profile(PlyrInfo* plyr);
void storage_status_change_calculations(int device);
void reset_sg_status(StorageDevice* device, int slot);
int save_konquest_region_to_memcard_w_error(int device, int slot, int mode, const char* title,
                                           unsigned int region, void* regionBuf, int flag,
                                           unsigned int* freeBlocks, int* freeBytes);
int format_card_and_create_mkda_file(int device);
int gc_delete_file(int device, const char* fileName);
int create_new_mk5_profile_file(int device);
int is_device_unformatted(int device);
int is_device_present(int device);
int is_device_error(int device);
int is_device_full(int device);
int is_storage_device_full(int device);
void set_mode_of_play(int mode);
void push_game_state(int state);
void set_player_state(PlyrInfo* plyr, int state);
void setup_sound_banks(int bank);
void wait_for_sound_banks_to_load(void);
void ppc_set_stage_value(int stage);
void pne_set_players_name_to_default(char* name, int* charPos);
int get_wls_left_cursor(void);
void set_sal_cursor(int v);
void pv_recalculate_profiles_and_position(int* outDevice, int* outSlot, int* outCount,
                                          int* outPosition);
void erase_player_profile(int device, int slot);
void format_value_to_display(char* dest, unsigned int value);
int does_name_already_exist(const char* name);
RwTexture* load_named_tga_from_slot(int slot, const char* name);
unsigned long strlen(const char* s);
int sprintf(char* buf, const char* fmt, ...);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, unsigned long n);
int strcmp(const char* a, const char* b);
int check_switch_edge(int port, int switch_index);
int check_switch_action(int port, int action);
int is_memcard_scanner_running(void);
void kill_async_memcard_scan(void);
int get_multi_profile_cursor_p1(void);
int get_multi_profile_cursor_p2(void);
void snd_req(int sound_id);
void move_player_name(const char* src, char* dst);
void move_player_pin(const unsigned char* src, unsigned char* dst);

extern int msg_card_gone_answer;
extern int menu_player;
extern char konq_region_data_buffer[];

int pprofile_pad;
char player_name[0xB];
int pne_char_position;
int select_location_done;

extern MkProc* aproc;
extern float _mkproc_sleep_ticks;
extern GameInfo g_game_info;

typedef struct MkVtableMkprocLocal {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc*);
    int (*dispatch)(void);
    int (*sleep)(void);
    int (*system_stack)(void);
    int (*local_stack)(void);
    float (*jump_sleep)(MkProcEntryFn entry);
} MkVtableMkprocLocal;

static void mkproc_sleep(void) {
    MkVtableMkprocLocal* vtbl;

    vtbl = (MkVtableMkprocLocal*)aproc->vtbl;
    vtbl->sleep();
}

/* Retail jump_sleep takes ticks in f1 (like pselect name-sound). */
static float mkproc_jump_sleep(MkProcEntryFn entry) {
    MkVtableMkprocLocal* vtbl;
    float (*js)(float, MkProcEntryFn);

    vtbl = (MkVtableMkprocLocal*)aproc->vtbl;
    js = (float (*)(float, MkProcEntryFn))vtbl->jump_sleep;
    return js(0.0f, entry);
}

/* .sdata2 */
static const float kOne = 1.0f;
static const float kNegOne = -1.0f;
static const float kSleepIntro = 10.0f; /* @2557 */
static const float kSleepLoop = 2.0f;   /* @2558 */
static const float kSleepPost = 50.0f;  /* @4196 */
static const float kZero = 0.0f;        /* @4199 / @1814 used as -1 */

/* .sbss / .sdata */
int ppwls_input_done;
static int pos_device;
static int pos_slot;
static int button_answer;
static unsigned char player_icon;
static unsigned char player_kode[6];
static int player_kode_current_digit;
static unsigned char player_confirm_kode[6];
static int player_confirm_kode_current_digit;
static int number_profiles;
static int position;
static int screen_obj;
static int scan_cards_timer;
extern int pprofile_player;
extern int first_button_press;
extern int name_entry_done;

typedef struct ProfileNameKey {
    const char* name;
    unsigned char value;
    unsigned char pad05[3];
} ProfileNameKey;

extern ProfileNameKey pne_alpha_data_table[39];

typedef struct ProfileCodeKey {
    int switch_index;
    unsigned char value;
    unsigned char pad05[3];
} ProfileCodeKey;

extern ProfileCodeKey pne_kode_data_table[12];

typedef struct ProfileUnlockSummary {
    unsigned int cat1[2]; /* +0x00 */
    unsigned int cat2[2]; /* +0x08 */
    unsigned int cat3[2]; /* +0x10 */
    unsigned int cat5; /* +0x18 */
    unsigned int pad1C;
    unsigned int cat7[2]; /* +0x20 */
    unsigned int cat8[2]; /* +0x28 */
    unsigned int pz_chars[2]; /* +0x30 */
    unsigned int pz_bgnds[2]; /* +0x38 */
} ProfileUnlockSummary;

extern ProfileUnlockSummary gp_data;
extern unsigned int default_char_bits[2];
extern unsigned int default_alt_char_bits[2];
extern unsigned int default_bgnd_bits[2];
extern unsigned int default_pz_char_bits[2];
extern unsigned int default_pz_bgnd_bits[2];
void pselect_update_profile_settings(void);

#define PROFILE_MENU_SCREEN_SLOT 0x90046
#define PROFILE_VIEW_SCREEN "common/player_profile/pp_view_profile"
#define PROFILE_DELETE_SCREEN "common/player_profile/pp_delete_profile"
#define PROFILE_CREATE_SCREEN "common/player_profile/pp_create_profile"
#define PROFILE_MENU_EVENT_REFRESH 0x1FB7
#define PROFILE_MENU_EVENT_CANCEL_ASK 0x1FE3
#define PROFILE_MENU_EVENT_CANCEL_NO 0x1FAB
#define PROFILE_CREATE_EVENT_STAGE 0x2FA9
#define PROFILE_MENU_FADE_FRAMES 8
#define NBC_MEMCARD_TITLE 0x30
#define MCARD_MSG_ACTIVE_PROGRESS 0xB

/*
 * PPWLS profile icon TGA names (color + _L alpha pairs). Indexed as icon*2.
 * Soft: string pool layout for Matching; names match retail stringBase0.
 */
char* ppwls_icon[] = {
    "MC_EMPTY_ICON", "MC_EMPTY_ICON_L", "MC_ICON1",  "MC_ICON1_L",  "MC_ICON2",
    "MC_ICON2_L",    "MC_ICON3",        "MC_ICON3_L", "MC_ICON4",   "MC_ICON4_L",
    "MC_ICON5",      "MC_ICON5_L",      "MC_ICON6",  "MC_ICON6_L",  "MC_ICON7",
    "MC_ICON7_L",    "MC_ICON8",        "MC_ICON8_L", "MC_ICON9",   "MC_ICON9_L",
    "MC_ICON10",     "MC_ICON10_L",     "MC_ICON11", "MC_ICON11_L", "MC_ICON12",
    "MC_ICON12_L",   "MC_ICON13",       "MC_ICON13_L", "MC_ICON14", "MC_ICON14_L",
    "MC_ICON15",     "MC_ICON15_L",     "MC_ICON16", "MC_ICON16_L", "MC_ICON17",
    "MC_ICON17_L",   "MC_ICON18",       "MC_ICON18_L", "MC_ICON19", "MC_ICON19_L",
    "MC_ICON20",     "MC_ICON20_L",     "MC_ICON21", "MC_ICON21_L", "MC_ICON22",
    "MC_ICON22_L",   "MC_ICON23",       "MC_ICON23_L", "MC_ICON24", "MC_ICON24_L",
};

float p_reset_ppwls_timeout(void);
float p_atm_loop(void);
void set_profile_to_default(PlayerProfile* profile);
void unload_player_profiles(void);

static void spawn_ppwls_timeout_proc(void) {
    MkHdr* pdata;

    destroy_mkprocs_pid(PPWLS_PROC_PID);
    ppwls_input_done = 0;
    _create_mkproc_generic_tinystack(PPWLS_PROC_PID, PPWLS_PROC_PRIO, p_reset_ppwls_timeout,
                                     PPWLS_TIMEOUT_PROC_PDATA, &pdata);
}

/* Profile blob 0x5C0 -- retail init offsets. */
#define PROFILE_COMMON_OFF 0x8
#define PROFILE_SWITCHMAP_OFF 0x108
#define PROFILE_KONQUEST_OFF 0x190
#define KONQUEST_FIELD_68 0x68
#define PROFILE_SWITCHMAP_STRIDE 0xC
#define PROFILE_DEFAULT_UNLOCK_CAT7_LO 0x15804FB
#define PROFILE_DEFAULT_UNLOCK_CAT5 0x3FF
#define PROFILE_KONQUEST_FIELD_68 10
#define PPWLS_TIMEOUT_TICKS 600
#define NBC_DEFAULT_PROFILE_NAME 7
#define NBC_DEFAULT_PROFILE_SUB 1
#define NBC_EMPTY_PROFILE_NAME 0x32
#define MEMCARD_SAVE_FILENAME "MKD"

extern PlayerProfile p1_profile;
extern PlayerProfile p2_profile;
extern int p1_profile_status;
extern int p2_profile_status;
extern int p1_profile_device;
extern int p2_profile_device;
extern int p1_profile_slot;
extern int p2_profile_slot;
extern void* p1_profile_common;
extern void* p2_profile_common;
extern void* p1_profile_konquest;
extern void* p2_profile_konquest;
extern int mcard_msg_active;
extern int default_switch_map[]; /* stride 0xC; copy word0 of each into profile +0x108 */
/* .sdata size 8 -- p1 @+0, p2 @+4 */
typedef struct ProfileCodePair {
    int p1;
    int p2;
} ProfileCodePair;
extern ProfileCodePair profile_code_state;
extern int p1_rumble_on;
extern int p2_rumble_on;

static void copy_profile_switch_defaults(PlayerProfile* profile) {
    int i;
    const char* src;

    src = (const char*)default_switch_map;
    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        profile->switch_map[i] = *(const int*)(src + i * PROFILE_SWITCHMAP_STRIDE);
    }
}

static void clear_storage_in_use(int device, int slot) {
    if (device >= 0 && device < STORAGE_MAX_DEVICES && slot >= 0 && slot < STORAGE_MAX_SLOTS) {
        DEVICE_AT(device)->inUse[slot] = 0;
    }
}

/*
 * Soft ceiling: erase_player_profile -- region clear and profile-table commit
 * are retail-correct; residual is loop/register scheduling.
 */
void erase_player_profile(int device, int slot) {
    StorageDevice* base;
    const char* title;
    int region;
    int ok;
    int saveOk;

    if (device < 0 || device >= STORAGE_MAX_DEVICES || slot < 0 || slot >= STORAGE_MAX_SLOTS) {
        return;
    }

    base = DEVICE_AT(device);
    reset_sg_status(base, slot);
    storage_status_change_calculations(device);
    mcard_msg_deleting_data(device);
    clear_region_buffer();

    ok = 1;
    region = 1;
    while (region < 9 && ok != 0) {
        title = nbc_find_text(NBC_MEMCARD_TITLE, 1);
        ok = save_konquest_region_to_memcard_w_error(device, slot, 5, title, (unsigned int)region,
                                                    konq_region_data_buffer, 0, &base->freeBlocks,
                                                    &base->freeBytes);
        if (ok != 0 && mcard_msg_active != MCARD_MSG_ACTIVE_PROGRESS) {
            mcard_msg_end();
            mcard_msg_deleting_data(device);
        }
        region++;
    }

    saveOk = 0;
    if (ok != 0) {
        title = nbc_find_text(NBC_MEMCARD_TITLE, 1);
        saveOk = save_to_memcard_w_error(device, 4, title, &base->settings, 0, &base->freeBlocks,
                                        &base->freeBytes);
    }

    if (saveOk == 0) {
        mcard_msg_delete_failed_generic(0);
    } else {
        mcard_msg_delete_successful_generic(0);
    }
    mcard_msg_end();
}

/*
 * Soft ceiling: p_delete_profile sleep/event schedule + inline erase vs
 * erase_player_profile call (retail inlines); present OK.
 */
float p_delete_profile(void) {
    PlyrInfo* plyr;
    int statusChanged;
    int device;
    int slot;

    number_profiles = 0;
    position = 0;
    screen_obj = 0;
    scan_cards_timer = 0x3C;
    pos_device = -1;
    pos_slot = -1;
    pprofile_player = menu_player;

    if (menu_player == 0 || menu_player == 1) {
        plyr = &(&g_game_info.plyr0)[pprofile_player];
        pprofile_pad = plyr->pad_index;
        set_mode_of_play(3);
        push_game_state(0xD);
        set_player_state(plyr, 2);
        setup_sound_banks(1);
        wait_for_sound_banks_to_load();
        set_mode_of_play(3);
        pv_recalculate_profiles_and_position(&pos_device, &pos_slot, &number_profiles, &position);
        load_screen(PROFILE_DELETE_SCREEN, PROFILE_MENU_SCREEN_SLOT, 0, 1);
        turn_camera_on();
        turn_controllers_on();
        button_answer = 0;

        while (button_answer != 2) {
            statusChanged = scan_cards_timer;
            scan_cards_timer = statusChanged - 1;
            if (statusChanged == 0) {
                if (update_storage_status(0) != 0) {
                    pv_recalculate_profiles_and_position(&pos_device, &pos_slot, &number_profiles,
                                                        &position);
                }
                fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                scan_cards_timer = 0x1E;
            }

            if (pos_device != -1 && pos_slot != -1 && number_profiles > 0 && button_answer == 1) {
                mcard_msg_confirm_erase();
                while (mcard_msg_confirm_erase_answer == 0) {
                    _mkproc_sleep_ticks = kOne;
                    mkproc_sleep();
                }
                mcard_msg_end();

                if (mcard_msg_confirm_erase_answer == 1) {
                    if (update_storage_status(0) == 0) {
                        device = pos_device;
                        slot = pos_slot;
                        if (device >= 0 && device < STORAGE_MAX_DEVICES && slot >= 0 &&
                            slot < STORAGE_MAX_SLOTS) {
                            erase_player_profile(device, slot);
                        }
                    }
                    pv_recalculate_profiles_and_position(&pos_device, &pos_slot, &number_profiles,
                                                        &position);
                    _mkproc_sleep_ticks = kOne;
                    mkproc_sleep();
                    fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                    button_answer = 0;
                }
            }

            _mkproc_sleep_ticks = kOne;
            mkproc_sleep();
        }

        fade_to_black(PROFILE_MENU_FADE_FRAMES, 1);
    }

    plyr = &(&g_game_info.plyr0)[pprofile_player];
    set_player_state(plyr, 0);
    set_mode_of_play(0xD);
    pop_game_state();
    gamelogic_jump(6, p_main_menu);
    return kNegOne;
}

/* Screen GetString sink: dest buffer + koin index 0..5. */
void ppv_get_current_profile_koins(char* dest, int index) {
    StorageProfileSlot* slot;
    int value;

    if (pos_device < 0 || pos_device >= STORAGE_MAX_DEVICES || pos_slot < 0 ||
        pos_slot >= STORAGE_MAX_SLOTS) {
        slot = 0;
    } else {
        slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    }
    if (index < 0 || index > 5 || slot == 0) {
        value = 0;
    } else {
        value = slot->koins[index];
    }
    format_value_to_display(dest, value);
}

/* outs[0..8] string buffers for the nine retail win/loss stat pairs. */
void get_profile_stats(char** outs) {
    StorageProfileSlot* slot;
    int (*pairs[3])[2];
    char values[2][12];
    char result[32];
    unsigned int value;
    int group;
    int pair;
    int out_index;
    int side;

    for (out_index = 0; out_index < 9; out_index++) {
        strcpy(outs[out_index], "0 / 0");
    }

    if (pos_device < 0 || pos_device >= STORAGE_MAX_DEVICES ||
        pos_slot < 0 || pos_slot >= STORAGE_MAX_SLOTS) {
        return;
    }
    slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    pairs[0] = slot->view_stats_early;
    pairs[1] = slot->view_stats_mid;
    pairs[2] = slot->view_stats_late;

    out_index = 0;
    for (group = 0; group < 3; group++) {
        for (pair = 0; pair < 3; pair++) {
            for (side = 0; side < 2; side++) {
                value = (unsigned int)pairs[group][pair][side];
                if (value >= 1000000U) {
                    value = 999999U;
                }
                format_value_to_display(values[side], value);
            }
            sprintf(result, "%s / %s", values[0], values[1]);
            strcpy(outs[out_index], result);
            out_index++;
        }
    }
}

void ppv_get_current_profile_arcade_finishes(char* dest) {
    StorageProfileSlot* slot;
    int value;

    if (pos_device < 0 || pos_device >= STORAGE_MAX_DEVICES || pos_slot < 0 ||
        pos_slot >= STORAGE_MAX_SLOTS) {
        slot = 0;
    } else {
        slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    }
    if (slot == 0) {
        value = 0;
    } else {
        value = slot->arcade_finishes;
    }
    format_value_to_display(dest, value);
}

/* Soft ceiling: ppv_get_current_profile_name -- storage index schedule. */
char* ppv_get_current_profile_name(void) {
    StorageProfileSlot* slot;

    if (pos_device < 0 || pos_device >= STORAGE_MAX_DEVICES || pos_slot < 0 ||
        pos_slot >= STORAGE_MAX_SLOTS) {
        slot = 0;
    } else {
        slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    }
    if (slot == 0) {
        return nbc_find_text(NBC_EMPTY_PROFILE_NAME, 1);
    }
    return slot->name;
}

/*
 * Soft ceiling: pv_recalculate_profiles_and_position walk emit; algo OK.
 */
void pv_recalculate_profiles_and_position(int* outDevice, int* outSlot, int* outCount,
                                          int* outPosition) {
    int count;
    int i;
    int device;
    int slot;
    int nextDevice;
    int nextSlot;
    int startDevice;
    int startSlot;
    int found;

    count = 0;
    *outDevice = -1;
    *outSlot = -1;
    for (i = 0; i < STORAGE_MAX_DEVICES; i++) {
        count += DEVICE_AT(i)->profileCount;
    }
    *outCount = count;
    *outPosition = count / 2;
    if (count < 1) {
        return;
    }

    *outDevice = 1;
    *outSlot = 6;
    for (i = 0; i < *outPosition + 1; i++) {
        device = *outDevice;
        slot = *outSlot;
        if (device >= 0 && device < STORAGE_MAX_DEVICES && slot >= 0 && slot < STORAGE_MAX_SLOTS) {
            if (slot < 6) {
                nextSlot = slot + 1;
                nextDevice = device;
            } else {
                nextSlot = 0;
                nextDevice = (device < 1) ? device + 1 : 0;
            }
        } else {
            nextDevice = device;
            nextSlot = slot;
        }

        startDevice = device;
        startSlot = slot;
        found = 0;
        while (1) {
            if (nextDevice == startDevice && nextSlot == startSlot) {
                *outDevice = -1;
                *outSlot = -1;
                break;
            }
            if (DEVICE_AT(nextDevice)->profiles[nextSlot].present == 1) {
                *outDevice = nextDevice;
                *outSlot = nextSlot;
                found = 1;
                break;
            }
            if (nextDevice >= 0 && nextDevice < STORAGE_MAX_DEVICES && nextSlot >= 0 &&
                nextSlot < STORAGE_MAX_SLOTS) {
                if (nextSlot < 6) {
                    nextSlot++;
                } else {
                    nextSlot = 0;
                    nextDevice = (nextDevice < 1) ? nextDevice + 1 : 0;
                }
            }
        }
        (void)found;
    }
}

/*
 * Soft ceiling: ppv_view_profile_icon_list -- window walk emit; loads up to 7
 * icon TGAs around current position for view/delete screens.
 */
void ppv_view_profile_icon_list(McIconListArg* arg) {
    RwTexture** out;
    int i;
    int before;
    int after;
    int start;
    int count;
    int device;
    int slot;
    int walkDevice;
    int walkSlot;
    int steps;
    StorageProfileSlot* profile;

    out = arg->textures;
    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        out[i] = 0;
    }
    if (number_profiles == 0) {
        return;
    }

    before = 3;
    if (position < 3) {
        before = position;
    }
    after = 3;
    if (number_profiles - position < 4) {
        after = (number_profiles - position) - 1;
    }
    start = 3 - before;
    count = before + after + (number_profiles != 0);

    device = pos_device;
    slot = pos_slot;
    steps = before;
    while (steps > 0) {
        walkDevice = device;
        walkSlot = slot;
        if (device >= 0 && device < STORAGE_MAX_DEVICES && slot >= 0 && slot < STORAGE_MAX_SLOTS) {
            if (slot < 1) {
                walkSlot = 6;
                walkDevice = (device < 1) ? 1 : device - 1;
            } else {
                walkSlot = slot - 1;
            }
        }
        while (walkDevice != device || walkSlot != slot) {
            if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1) {
                break;
            }
            if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES && walkSlot >= 0 &&
                walkSlot < STORAGE_MAX_SLOTS) {
                if (walkSlot < 1) {
                    walkSlot = 6;
                    walkDevice = (walkDevice < 1) ? 1 : walkDevice - 1;
                } else {
                    walkSlot--;
                }
            }
        }
        if (walkDevice == device && walkSlot == slot) {
            device = -1;
            slot = -1;
        } else {
            device = walkDevice;
            slot = walkSlot;
        }
        steps--;
    }

    for (i = 0; i < count; i++) {
        if (i == 0) {
            if (device < 0 || device >= STORAGE_MAX_DEVICES || slot < 0 ||
                slot >= STORAGE_MAX_SLOTS) {
                profile = 0;
            } else {
                profile = &DEVICE_AT(device)->profiles[slot];
            }
        } else {
            walkDevice = device;
            walkSlot = slot;
            if (device >= 0 && device < STORAGE_MAX_DEVICES && slot >= 0 && slot < STORAGE_MAX_SLOTS) {
                if (slot < 6) {
                    walkSlot = slot + 1;
                } else {
                    walkSlot = 0;
                    walkDevice = (device < 1) ? device + 1 : 0;
                }
            }
            profile = 0;
            while (1) {
                if (walkDevice == device && walkSlot == slot) {
                    device = -1;
                    slot = -1;
                    break;
                }
                if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1) {
                    profile = &DEVICE_AT(walkDevice)->profiles[walkSlot];
                    device = walkDevice;
                    slot = walkSlot;
                    break;
                }
                if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES && walkSlot >= 0 &&
                    walkSlot < STORAGE_MAX_SLOTS) {
                    if (walkSlot < 6) {
                        walkSlot++;
                    } else {
                        walkSlot = 0;
                        walkDevice = (walkDevice < 1) ? walkDevice + 1 : 0;
                    }
                }
            }
        }
        if (profile != 0) {
            out[start + i] =
                load_named_tga_from_slot(PROFILE_MENU_SCREEN_SLOT, ppwls_icon[profile->icon * 2]);
        }
    }
}

void ppv_update_profile_cursor(int delta) {
    int device;
    int slot;
    int walkDevice;
    int walkSlot;

    if (position + delta < 0 || position + delta >= number_profiles) {
        fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
        return;
    }

    if (delta < 1) {
        device = pos_device;
        slot = pos_slot;
        if (pos_device >= 0 && pos_device < STORAGE_MAX_DEVICES && pos_slot >= 0 &&
            pos_slot < STORAGE_MAX_SLOTS) {
            if (pos_slot < 1) {
                slot = 6;
                device = (pos_device < 1) ? 1 : pos_device - 1;
            } else {
                slot = pos_slot - 1;
            }
        }
        walkDevice = device;
        walkSlot = slot;
        while (walkDevice != pos_device || walkSlot != pos_slot) {
            if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1) {
                pos_device = walkDevice;
                pos_slot = walkSlot;
                position--;
                fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                return;
            }
            if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES && walkSlot >= 0 &&
                walkSlot < STORAGE_MAX_SLOTS) {
                if (walkSlot < 1) {
                    walkSlot = 6;
                    walkDevice = (walkDevice < 1) ? 1 : walkDevice - 1;
                } else {
                    walkSlot--;
                }
            }
        }
        pos_device = -1;
        pos_slot = -1;
        position--;
    } else {
        device = pos_device;
        slot = pos_slot;
        if (pos_device >= 0 && pos_device < STORAGE_MAX_DEVICES && pos_slot >= 0 &&
            pos_slot < STORAGE_MAX_SLOTS) {
            if (pos_slot < 6) {
                slot = pos_slot + 1;
            } else {
                slot = 0;
                device = (pos_device < 1) ? pos_device + 1 : 0;
            }
        }
        walkDevice = device;
        walkSlot = slot;
        while (walkDevice != pos_device || walkSlot != pos_slot) {
            if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1) {
                pos_device = walkDevice;
                pos_slot = walkSlot;
                position++;
                fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                return;
            }
            if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES && walkSlot >= 0 &&
                walkSlot < STORAGE_MAX_SLOTS) {
                if (walkSlot < 6) {
                    walkSlot++;
                } else {
                    walkSlot = 0;
                    walkDevice = (walkDevice < 1) ? walkDevice + 1 : 0;
                }
            }
        }
        pos_device = -1;
        pos_slot = -1;
        position++;
    }
    fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
}

/* Soft ceiling: p_view_profile sleep/timer schedule; present OK. */
float p_view_profile(void) {
    PlyrInfo* plyr;
    int statusChanged;

    number_profiles = 0;
    position = 0;
    screen_obj = 0;
    scan_cards_timer = 0x3C;
    pos_device = -1;
    pos_slot = -1;
    pprofile_player = menu_player;

    if (menu_player == 0 || menu_player == 1) {
        plyr = &(&g_game_info.plyr0)[pprofile_player];
        pprofile_pad = plyr->pad_index;
        set_mode_of_play(3);
        push_game_state(0xD);
        set_player_state(plyr, 2);
        setup_sound_banks(1);
        wait_for_sound_banks_to_load();
        pv_recalculate_profiles_and_position(&pos_device, &pos_slot, &number_profiles, &position);
        load_screen(PROFILE_VIEW_SCREEN, PROFILE_MENU_SCREEN_SLOT, 0, 1);
        turn_camera_on();
        turn_controllers_on();
        button_answer = 0;

        while (button_answer != 2) {
            statusChanged = scan_cards_timer;
            scan_cards_timer = statusChanged - 1;
            if (statusChanged == 0) {
                if (update_storage_status(0) != 0) {
                    pv_recalculate_profiles_and_position(&pos_device, &pos_slot, &number_profiles,
                                                        &position);
                    fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                }
                scan_cards_timer = 0x1E;
            }
            _mkproc_sleep_ticks = kOne;
            mkproc_sleep();
        }

        fade_to_black(PROFILE_MENU_FADE_FRAMES, 1);
    }

    pop_game_state();
    set_mode_of_play(0xD);
    plyr = &(&g_game_info.plyr0)[pprofile_player];
    set_player_state(plyr, 0);
    gamelogic_jump(6, p_main_menu);
    return kNegOne;
}

char* get_current_create_a_profile_name(void) {
    return player_name;
}

int pne_is_name_already_used(void) {
    return 0;
}

void pp_name_entry_proces_char_entry(const char* key_name) {
    int found;
    int key;
    int i;
    char* out;

    found = 0;
    key = 0;
    if (key_name == 0) {
        return;
    }
    for (i = 0; i < 39 && found == 0; i++) {
        strcmp(key_name, pne_alpha_data_table[i].name);
        if (strcmp(key_name, pne_alpha_data_table[i].name) == 0) {
            found = 1;
            key = i;
        }
    }
    if (found == 0) {
        return;
    }

    if (key >= 0 && key < 37) {
        if (first_button_press != 0) {
            out = player_name;
            for (i = 0; i < 10; i++) {
                *out++ = '_';
            }
            first_button_press = 0;
            player_name[10] = '\0';
            pne_char_position = 0;
        }
        if (pne_char_position < 10) {
            out = player_name + pne_char_position;
            *out = pne_alpha_data_table[key].value;
            fire_screen_studio_event(0x2FA8, 0);
            if (pne_char_position < 10) {
                pne_char_position++;
            }
        }
    } else if (key == 37) {
        if (first_button_press != 0) {
            out = player_name;
            for (i = 0; i < 10; i++) {
                *out++ = '_';
            }
            first_button_press = 0;
            player_name[10] = '\0';
            pne_char_position = 1;
        }
        if (pne_char_position > 0) {
            pne_char_position--;
            player_name[pne_char_position] = pne_alpha_data_table[key].value;
            fire_screen_studio_event(0x2FA8, 0);
        }
    } else if (key < 39 && pne_char_position > 0) {
        name_entry_done = 1;
    }
}
/* Soft ceiling: pp_name_entry_proces_char_entry ~75.54% -- retail algorithm recovered;
 * residual is loop/branch scheduling and shared table relocation emission. */

void ppc_set_button_answer(int answer) {
    button_answer = answer;
}

void ppc_set_current_icon_selection(unsigned char icon) {
    player_icon = icon;
}

int ppc_get_code_state(void) {
    return ((int*)&profile_code_state)[pprofile_player];
}

void ppc_transition_pause(int paused) {
    if (paused != 0) {
        turn_controllers_off();
    } else {
        turn_controllers_on();
        disable_all_ports_but_me(pprofile_pad);
    }
}

static void create_profile_sleep(float ticks) {
    _mkproc_sleep_ticks = ticks;
    mkproc_sleep();
}

/* Returns nonzero when the player confirms leaving profile creation. */
static int create_profile_cancel_prompt(void) {
    int answered;

    answered = 0;
    fire_screen_studio_event(PROFILE_MENU_EVENT_CANCEL_ASK, 0);
    button_answer = 0;
    create_profile_sleep(kSleepLoop);
    while (!answered) {
        create_profile_sleep(kOne);
        if (button_answer == 1) {
            return 1;
        }
        if (button_answer == 2) {
            fire_screen_studio_event(PROFILE_MENU_EVENT_CANCEL_NO, 0);
            answered = 1;
        }
    }
    button_answer = 0;
    return 0;
}

static void clear_profile_code(unsigned char code[6], int* digit) {
    int i;

    for (i = 0; i < 6; i++) {
        code[i] = 0;
    }
    *digit = 0;
    ((int*)&profile_code_state)[pprofile_player] = 0;
    fire_screen_studio_event(0x1FAA, pprofile_player + 1);
}

static void enter_profile_code(unsigned char code[6], int* digit) {
    int i;

    while (*digit < 6) {
        for (i = 0; i < 12; i++) {
            if (check_switch_edge(pprofile_pad,
                                  pne_kode_data_table[i].switch_index)) {
                code[*digit] = pne_kode_data_table[i].value;
                *digit += 1;
                ((int*)&profile_code_state)[pprofile_player] = *digit;
                fire_screen_studio_event(0x1FAA, pprofile_player + 1);
                break;
            }
        }
        if (*digit != 6) {
            create_profile_sleep(kOne);
            if (check_switch_edge(pprofile_pad, 0xB)) {
                clear_profile_code(code, digit);
                if (pprofile_player == 0) {
                    fire_screen_studio_event(0x1FC4, 1);
                } else {
                    fire_screen_studio_event(0x1FC5,
                                             pprofile_player + 1);
                }
                fire_screen_studio_event(0x1FAA, pprofile_player + 1);
            }
        }
    }
    create_profile_sleep(kOne);
}

float p_create_profile(void) {
    PlyrInfo* plyr;
    StorageDevice* device_status;
    StorageProfileSlot* profile;
    const char* title;
    int exit_requested;
    int created;
    int restart_workflow;
    int name_confirmed;
    int icon_confirmed;
    int code_confirmed;
    int attempts;
    int device;
    int slot;
    int timer;
    int i;

    set_mode_of_play(3);
    turn_controllers_off();
    name_entry_done = 0;
    pprofile_player = menu_player;
    exit_requested = menu_player != 0 && menu_player != 1;
    created = 0;

    if (!exit_requested) {
        plyr = &(&g_game_info.plyr0)[pprofile_player];
        pprofile_pad = plyr->pad_index;
        push_game_state(0xD);
        set_player_state(plyr, 2);
        setup_sound_banks(1);
        wait_for_sound_banks_to_load();
        first_button_press = 1;
        unload_player_profiles();
        pne_set_players_name_to_default(player_name, &pne_char_position);
        turn_camera_on();
        turn_controllers_on();
        disable_all_ports_but_me(pprofile_pad);
        ck_for_controller_removed();
        turn_controllers_off();
        while (mcard_msg_active != 0) {
            create_profile_sleep(kOne);
        }
        create_profile_sleep(kSleepIntro);
        load_screen(PROFILE_CREATE_SCREEN, PROFILE_MENU_SCREEN_SLOT, 0, 1);
    }

    while (!exit_requested && !created) {
        restart_workflow = 0;
        name_confirmed = 0;
        while (!exit_requested && !name_confirmed) {
            ppc_set_stage_value(0);
            fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
            name_entry_done = 0;
            turn_controllers_on();
            disable_all_ports_but_me(pprofile_pad);
            button_answer = 0;

            while (!exit_requested && name_entry_done == 0) {
                create_profile_sleep(kOne);
                if (button_answer == 2) {
                    exit_requested = create_profile_cancel_prompt();
                    name_entry_done = 0;
                }
            }
            if (exit_requested) {
                break;
            }

            update_storage_status(0);
            if (does_name_already_exist(player_name)) {
                mcard_msg_name_conflict();
                create_profile_sleep(kSleepIntro);
                continue;
            }

            turn_controllers_off();
            ppc_set_stage_value(1);
            fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
            button_answer = 0;
            while (button_answer == 0) {
                create_profile_sleep(kOne);
            }
            if (button_answer == 1) {
                name_confirmed = 1;
            }
        }
        if (exit_requested) {
            break;
        }

        for (i = 0; i < 10; i++) {
            if (player_name[i] == '_') {
                player_name[i] = '\0';
            }
        }

        icon_confirmed = 0;
        ppc_set_stage_value(2);
        fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
        button_answer = 0;
        while (!exit_requested && !icon_confirmed) {
            create_profile_sleep(kOne);
            if (button_answer == 1) {
                icon_confirmed = 1;
            } else if (button_answer == 2) {
                exit_requested = create_profile_cancel_prompt();
            }
        }
        if (exit_requested) {
            break;
        }

        code_confirmed = 0;
        while (!code_confirmed) {
            ppc_set_stage_value(3);
            fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
            clear_profile_code(player_kode, &player_kode_current_digit);
            enter_profile_code(player_kode, &player_kode_current_digit);

            ppc_set_stage_value(4);
            fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
            attempts = 3;
            while (attempts > 0 && !code_confirmed) {
                attempts--;
                clear_profile_code(player_confirm_kode,
                                   &player_confirm_kode_current_digit);
                enter_profile_code(player_confirm_kode,
                                   &player_confirm_kode_current_digit);
                code_confirmed =
                    memcmp(player_kode, player_confirm_kode, 6) == 0;
            }
        }

        set_sal_cursor(0);
        timer = 0x1E;
        ppc_set_stage_value(5);
        fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
        create_profile_sleep(kOne);
        reset_format_or_recreate_flags();
        select_location_done = 0;
        button_answer = 0;

        while (!exit_requested && !restart_workflow && !created) {
            if (select_location_done != 0) {
                create_profile_sleep(kOne);
                fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                create_profile_sleep(kOne);
                ppc_set_stage_value(6);
                fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
                button_answer = 0;
                while (button_answer != 1) {
                    create_profile_sleep(kOne);
                }
                created = 1;
                break;
            }

            timer--;
            if (timer < 0) {
                check_format_or_recreate();
                if (update_storage_status(1) != 0) {
                    create_profile_sleep(kSleepLoop);
                    fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                }
                timer = 0x1E;
            }

            create_profile_sleep(kOne);
            if (button_answer == 1) {
                device = get_wls_left_cursor();
                if (device < 0 || device >= STORAGE_MAX_DEVICES) {
                    snd_req(0x1AA8);
                } else {
                    update_storage_status(0);
                    device_status = DEVICE_AT(device);
                    if (device_status->status == 0) {
                        if (find_device_display_status(device) != 0) {
                            snd_req(0x1AA8);
                        } else if (does_name_already_exist(player_name)) {
                            mcard_msg_name_conflict();
                            pne_set_players_name_to_default(
                                player_name, &pne_char_position);
                            restart_workflow = 1;
                        } else if (device_status->profileCount >
                                   STORAGE_MAX_SLOTS - 1) {
                            snd_req(0x1AA8);
                        } else {
                            slot = -1;
                            for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
                                if (device_status->profiles[i].present == 0) {
                                    slot = i;
                                    break;
                                }
                            }
                            if (slot >= 0) {
                                profile = &device_status->profiles[slot];
                                reset_sg_status(device_status, slot);
                                profile->icon = player_icon;
                                move_player_name(player_name, profile->name);
                                move_player_pin(player_kode, profile->pin);
                                profile->present = 1;
                                profile->idChecksum = (int)random();
                                title = nbc_find_text(0x30, 1);
                                select_location_done =
                                    save_to_memcard_w_error(
                                        device, 6, title,
                                        &device_status->settings, 0,
                                        &device_status->freeBlocks,
                                        &device_status->freeBytes);
                                snd_req(0x1AA5);
                            }
                        }
                    }
                }
                button_answer = 0;
            } else if (button_answer == 2) {
                exit_requested = create_profile_cancel_prompt();
            }
        }
    }

    turn_all_ports_on();
    turn_controllers_on();
    pop_game_state();
    set_mode_of_play(0xD);
    fade_to_black(PROFILE_MENU_FADE_FRAMES, 1);
    plyr = &(&g_game_info.plyr0)[pprofile_player];
    set_player_state(plyr, 0);
    gamelogic_jump(6, p_main_menu);
    return kNegOne;
}

void format_value_to_display(char* dest, unsigned int value) {
    char number[12] = {0};
    int length;

    strcpy(dest, "");
    sprintf(number, "%u", value);
    length = strlen(number);
    if (length >= 0 && length < 4) {
        strcpy(dest, number);
    } else if (length >= 4 && length < 7) {
        strncat(dest, number, length - 3);
        strcat(dest, ",");
        strncat(dest, number + length - 3, 3);
    } else if (length == 7) {
        strncat(dest, number, length - 6);
        strcat(dest, ".");
        strncat(dest, number + length - 6, 2);
        strcat(dest, " M");
    } else if (length == 8) {
        strncat(dest, number, length - 6);
        strcat(dest, ".");
        strncat(dest, number + length - 6, 1);
        strcat(dest, " M");
    } else if (length == 9) {
        strncat(dest, number, length - 6);
        strcat(dest, " M");
    } else if (length == 10) {
        strncat(dest, number, length - 9);
        strcat(dest, ".");
        strncat(dest, number + length - 9, 2);
        strcat(dest, " G");
    } else {
        strcpy(dest, nbc_find_text(0x33, 1));
    }
}
/* Soft ceiling: format_value_to_display ~54.60% -- all retail magnitude formats recovered;
 * residual is switch-tree and shared-string-pool emission. */

char* get_heros_name(int which) {
    char* name;

    name = p2_profile.name;
    if (which != 0) {
        return name;
    }
    return p1_profile.name;
}

int is_mark_as_unlocked(PlayerProfile* profile, int category, int character) {
    unsigned long long mask;
    unsigned int* words;
    unsigned int high;
    unsigned int low;

    high = 0;
    low = 0;
    if (profile != &p1_profile && profile != &p2_profile) {
        return 0;
    }

    switch (category) {
    case 1:
        if (character < 0 || character >= 44) {
            return 0;
        }
        words = profile->unlock_cat1;
        break;
    case 2:
        if (character < 0 || character >= 44) {
            return 0;
        }
        words = profile->unlock_cat2;
        break;
    case 3:
        if (character < 0 || character >= 35) {
            return 0;
        }
        mask = 1ULL << character;
        low = profile->unlock_cat3 & (unsigned int)mask;
        return low != 0;
    case 4:
        if (character < 0 || character >= 11) {
            return 0;
        }
        mask = 1ULL << character;
        low = profile->unlock_cat4 & (unsigned int)mask;
        return low != 0;
    case 5:
        if (character < 0 || character >= 11) {
            return 0;
        }
        mask = 1ULL << (character + 10);
        low = profile->unlock_cat5 & (unsigned int)mask;
        return low != 0;
    case 6:
        if (character < 0 || character >= 44) {
            return 0;
        }
        words = profile->unlock_cat6;
        break;
    case 7:
    case 8:
        if (character < 0 || character >= 44) {
            return 0;
        }
        words = category == 7 ? profile->unlock_cat7 : profile->unlock_cat8;
        break;
    case 9:
        if (character < 0 || character >= 44) {
            return 0;
        }
        words = profile->unlock_cat9;
        break;
    case 10:
        if (character < 0 || character >= 35) {
            return 0;
        }
        mask = 1ULL << character;
        low = profile->unlock_cat10 & (unsigned int)mask;
        return low != 0;
    default:
        return 0;
    }

    mask = 1ULL << character;
    high = words[0] & (unsigned int)(mask >> 32);
    low = words[1] & (unsigned int)mask;
    return high != 0 || low != 0;
}
/* Soft ceiling: is_mark_as_unlocked ~61.79% -- typed retail bitsets recovered;
 * residual is switch order and 64-bit shift helper scheduling. */

void mark_as_locked(PlayerProfile* profile, int category, int character) {
    unsigned long long mask;
    unsigned int* words;

    if (profile != &p1_profile && profile != &p2_profile) {
        return;
    }
    switch (category) {
    case 1:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat1;
        break;
    case 2:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat2;
        break;
    case 3:
        if (character < 0 || character >= 35) return;
        profile->unlock_cat3 &= ~(unsigned int)(1ULL << character);
        return;
    case 4:
        if (character < 0 || character >= 11) return;
        profile->unlock_cat4 &= ~(unsigned int)(1ULL << character);
        return;
    case 5:
        if (character < 0 || character >= 11) return;
        profile->unlock_cat5 &= ~(unsigned int)(1ULL << (character + 10));
        return;
    case 6:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat6;
        break;
    case 7:
    case 8:
        if (character < 0 || character >= 44) return;
        words = category == 7 ? profile->unlock_cat7 : profile->unlock_cat8;
        break;
    case 9:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat9;
        break;
    case 10:
        if (character < 0 || character >= 35) return;
        profile->unlock_cat10 &= ~(unsigned int)(1ULL << character);
        return;
    default:
        return;
    }
    mask = 1ULL << character;
    words[1] &= ~(unsigned int)mask;
    words[0] &= ~(unsigned int)(mask >> 32);
}
/* Soft ceiling: mark_as_locked ~61.95% -- typed retail bitsets recovered;
 * residual is switch order and 64-bit shift helper scheduling. */

/* Consumers: nis, krypt handle_controller_input, projectile. Soft -- bitfield emit. */
void mark_as_unlocked(PlayerProfile* profile, int category, int character) {
    unsigned long long mask;
    unsigned int* words;

    if (profile != &p1_profile && profile != &p2_profile) {
        return;
    }
    switch (category) {
    case 1:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat1;
        break;
    case 2:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat2;
        break;
    case 3:
        if (character < 0 || character >= 35) return;
        profile->unlock_cat3 |= (unsigned int)(1ULL << character);
        return;
    case 4:
        if (character < 0 || character >= 11) return;
        profile->unlock_cat4 |= (unsigned int)(1ULL << character);
        return;
    case 5:
        if (character < 0 || character >= 11) return;
        profile->unlock_cat5 |= (unsigned int)(1ULL << (character + 10));
        return;
    case 6:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat6;
        break;
    case 7:
    case 8:
        if (character < 0 || character >= 44) return;
        words = category == 7 ? profile->unlock_cat7 : profile->unlock_cat8;
        break;
    case 9:
        if (character < 0 || character >= 44) return;
        words = profile->unlock_cat9;
        break;
    case 10:
        if (character < 0 || character >= 35) return;
        profile->unlock_cat10 |= (unsigned int)(1ULL << character);
        return;
    default:
        return;
    }
    mask = 1ULL << character;
    words[1] |= (unsigned int)mask;
    words[0] |= (unsigned int)(mask >> 32);
}
/* Soft ceiling: mark_as_unlocked ~69.66% -- typed retail bitsets recovered;
 * residual is switch order and 64-bit shift helper scheduling. */

void summarize_unlocked_items(void) {
    int device;
    int slot;
    StorageProfileSlot* profile;

    gp_data.cat1[0] = default_char_bits[0];
    gp_data.cat1[1] = default_char_bits[1];
    gp_data.cat2[0] = default_alt_char_bits[0];
    gp_data.cat2[1] = default_alt_char_bits[1];
    gp_data.cat3[0] = default_bgnd_bits[0];
    gp_data.cat3[1] = default_bgnd_bits[1];
    gp_data.cat5 = PROFILE_DEFAULT_UNLOCK_CAT5;
    gp_data.pad1C = 0;
    gp_data.cat7[0] = 0;
    gp_data.cat7[1] = PROFILE_DEFAULT_UNLOCK_CAT7_LO;
    gp_data.cat8[0] = 0;
    gp_data.cat8[1] = 0;
    gp_data.pz_chars[0] = default_pz_char_bits[0];
    gp_data.pz_chars[1] = default_pz_char_bits[1];
    gp_data.pz_bgnds[0] = default_pz_bgnd_bits[0];
    gp_data.pz_bgnds[1] = default_pz_bgnd_bits[1];

    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        if (find_device_display_status(device) != 0) {
            continue;
        }
        for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
            profile = &DEVICE_AT(device)->profiles[slot];
            if (profile->present == 0) {
                continue;
            }
            gp_data.cat1[0] |= profile->unlock_cat1[0];
            gp_data.cat1[1] |= profile->unlock_cat1[1];
            gp_data.cat2[0] |= profile->unlock_cat2[0];
            gp_data.cat2[1] |= profile->unlock_cat2[1];
            gp_data.cat3[1] |= profile->unlock_cat3;
            gp_data.cat5 |= profile->unlock_cat5;
            gp_data.cat7[0] |= profile->unlock_cat7[0];
            gp_data.cat7[1] |= profile->unlock_cat7[1];
            gp_data.cat8[0] |= profile->unlock_cat8[0];
            gp_data.cat8[1] |= profile->unlock_cat8[1];
            gp_data.pz_chars[0] |= profile->unlock_cat9[0];
            gp_data.pz_chars[1] |= profile->unlock_cat9[1];
            gp_data.pz_bgnds[1] |= profile->unlock_cat10;
        }
    }
    pselect_update_profile_settings();
}
/* Soft ceiling: summarize_unlocked_items ~52.51% -- retail default/loaded-profile
 * aggregation recovered; residual is inlined device-status and loop scheduling. */

static int profile_pins_equal(PlayerProfile* live, StorageProfileSlot* slot);
static int profile_names_equal(PlayerProfile* live, StorageProfileSlot* slot);

void check_new_mu_for_in_use_profiles(int device) {
    PlayerProfile* live;
    StorageProfileSlot* stored;
    int* profile_device;
    int* profile_slot;
    int profile_status;
    int player;
    int slot;
    int identity_matches;

    if (device < 0 || device >= STORAGE_MAX_DEVICES ||
        DEVICE_AT(device)->status != 0) {
        return;
    }

    for (player = 0; player < 2; player++) {
        if (player == 0) {
            live = &p1_profile;
            profile_device = &p1_profile_device;
            profile_slot = &p1_profile_slot;
            profile_status = p1_profile_status;
        } else {
            live = &p2_profile;
            profile_device = &p2_profile_device;
            profile_slot = &p2_profile_slot;
            profile_status = p2_profile_status;
        }
        if (profile_status != 1) {
            continue;
        }

        identity_matches = 0;
        if (*profile_device >= 0 &&
            *profile_device < STORAGE_MAX_DEVICES &&
            *profile_slot >= 0 && *profile_slot < STORAGE_MAX_SLOTS) {
            stored = &DEVICE_AT(*profile_device)->profiles[*profile_slot];
            identity_matches =
                profile_pins_equal(live, stored) &&
                profile_names_equal(live, stored) &&
                stored->idChecksum == live->idChecksum;
        }

        if (identity_matches) {
            DEVICE_AT(*profile_device)->inUse[*profile_slot] = 1;
            continue;
        }

        for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
            stored = &DEVICE_AT(device)->profiles[slot];
            if (stored->present == 1 &&
                profile_pins_equal(live, stored) &&
                profile_names_equal(live, stored) &&
                stored->idChecksum == live->idChecksum) {
                DEVICE_AT(device)->inUse[slot] = 1;
                *profile_slot = slot;
                *profile_device = device;
            }
        }
    }
}

/*
 * Consumer: save_profile (utils / CARD path).
 * Soft ceiling: validate_save_location ~83% -- mtctr/bdnz pin-name walks vs
 * indexed/addic emit + present-slot search NV; algo OK, stop.
 */
static int profile_pins_equal(PlayerProfile* live, StorageProfileSlot* slot) {
    int i;

    for (i = 0; i < 6; i++) {
        if (live->pin[i] != slot->pin[i]) {
            return 0;
        }
    }
    return 1;
}

static int profile_names_equal(PlayerProfile* live, StorageProfileSlot* slot) {
    int i;

    for (i = 0; i < STORAGE_NAME_LEN; i++) {
        if ((signed char)live->name[i] != (signed char)slot->name[i]) {
            return 0;
        }
    }
    return 1;
}

static int profile_fully_matches(PlayerProfile* live, StorageProfileSlot* slot) {
    if (profile_pins_equal(live, slot) == 0) {
        return 0;
    }
    if (profile_names_equal(live, slot) == 0) {
        return 0;
    }
    return (int)(__cntlzw(slot->idChecksum - live->idChecksum) >> 5);
}

static void advance_device_slot(int* device, int* slot) {
    if (*device < 0 || *device >= STORAGE_MAX_DEVICES || *slot < 0 || *slot >= STORAGE_MAX_SLOTS) {
        return;
    }
    if (*slot < STORAGE_MAX_SLOTS - 1) {
        *slot += 1;
        return;
    }
    *slot = 0;
    if (*device < 1) {
        *device += 1;
    } else {
        *device = 0;
    }
}

int validate_save_location(int player) {
    int* devicePtr;
    int* slotPtr;
    PlayerProfile* live;
    StorageProfileSlot* slot;
    StorageProfileSlot* found;
    int device;
    int slotIndex;
    int scanDevice;
    int scanSlot;
    int nextDevice;
    int nextSlot;
    int tries;
    int waitLeft;
    int matched;

    if (player != 0 && player != 1) {
        return 0;
    }
    if (player == 0) {
        devicePtr = &p1_profile_device;
        live = &p1_profile;
        slotPtr = &p1_profile_slot;
    } else {
        devicePtr = &p2_profile_device;
        live = &p2_profile;
        slotPtr = &p2_profile_slot;
    }
    if (*devicePtr < 0 || *devicePtr >= STORAGE_MAX_DEVICES || *slotPtr < 0 ||
        *slotPtr >= STORAGE_MAX_SLOTS) {
        return 0;
    }
    if (live == 0) {
        return 0;
    }

    do {
        waitLeft = 0x1E;
        while (update_storage_status(0) != 0 && waitLeft > 0) {
            waitLeft -= 1;
        }
        device = *devicePtr;
        slotIndex = *slotPtr;
        slot = &DEVICE_AT(device)->profiles[slotIndex];
        matched = profile_fully_matches(live, slot);
        if (matched != 0) {
            return 1;
        }

        scanDevice = device;
        scanSlot = slotIndex;
        tries = 0x49;
        found = 0;
        do {
            tries -= 1;
            nextDevice = scanDevice;
            nextSlot = scanSlot;
            advance_device_slot(&nextDevice, &nextSlot);
            while (nextDevice != scanDevice || nextSlot != scanSlot) {
                if (DEVICE_AT(nextDevice)->profiles[nextSlot].present == 1) {
                    found = &DEVICE_AT(nextDevice)->profiles[nextSlot];
                    scanDevice = nextDevice;
                    scanSlot = nextSlot;
                    break;
                }
                advance_device_slot(&nextDevice, &nextSlot);
            }
            if (nextDevice == scanDevice && nextSlot == scanSlot && found == 0) {
                scanDevice = -1;
                scanSlot = -1;
                found = 0;
            }
            if (found == 0) {
                matched = 0;
                break;
            }
            matched = profile_fully_matches(live, found);
            if ((scanDevice == device && scanSlot == slotIndex) || matched != 0 || tries < 1) {
                break;
            }
            found = 0;
        } while (1);

        if (matched != 0) {
            *devicePtr = scanDevice;
            *slotPtr = scanSlot;
            return 1;
        }
        mcard_msg_card_gone(live->name, player);
        mcard_msg_end();
    } while (msg_card_gone_answer != 2);
    return 0;
}

/* Consumer: konquest_save. */
int validate_konq_save_location(int player) {
    (void)player;
    /* Soft ceiling: validate_konq_save_location ~0.57% -- Konquest CARD
     * region validation remains outside the recovered ordinary-profile path. */
    return 0;
}

int validate_konq_load_location(int player) {
    /* Soft ceiling: validate_konq_load_location ~0.55% -- Konquest CARD
     * region validation remains outside the recovered ordinary-profile path. */
    (void)player;
    return 0;
}

void quit_from_konquest(void) {
    unload_player_profiles();
    gamelogic_jump(6, p_main_menu);
}

int move_to_profile(int count, unsigned char* code, int* devicePtr, int* slotPtr) {
    int i;
    int j;
    int startDevice;
    int startSlot;
    StorageProfileSlot* profile;

    *devicePtr = STORAGE_MAX_DEVICES - 1;
    *slotPtr = STORAGE_MAX_SLOTS - 1;

    for (i = 0; i < count; i++) {
        advance_device_slot(devicePtr, slotPtr);
        startDevice = *devicePtr;
        startSlot = *slotPtr;

        do {
            profile = &DEVICE_AT(*devicePtr)->profiles[*slotPtr];
            if (profile->present == 1 && DEVICE_AT(*devicePtr)->inUse[*slotPtr] == 0) {
                for (j = 0; j < 6; j++) {
                    if (code[j] != profile->pin[j]) {
                        break;
                    }
                }
                if (j == 6) {
                    break;
                }
            }
            advance_device_slot(devicePtr, slotPtr);
        } while (*devicePtr != startDevice || *slotPtr != startSlot);

        if (*devicePtr == startDevice && *slotPtr == startSlot) {
            profile = &DEVICE_AT(*devicePtr)->profiles[*slotPtr];
            for (j = 0; j < 6; j++) {
                if (code[j] != profile->pin[j]) {
                    break;
                }
            }
            if (profile->present != 1 || DEVICE_AT(*devicePtr)->inUse[*slotPtr] != 0 || j != 6) {
                *devicePtr = -1;
                *slotPtr = -1;
                return 0;
            }
        }
    }
    return 1;
}

#define PPL_LIST_PID_P1 0x9026
#define PPL_LIST_PID_P2 0x9027
#define PPL_NAME_SLOTS 14

/*
 * UI list pdata: 6-byte code/PIN lives at +0x14 (retail lbz walk).
 * Compare against StorageProfileSlot.pin (@ +0x13 of each slot).
 */
static int multi_code_matches_slot(const unsigned char* code, StorageProfileSlot* slot) {
    int i;

    for (i = 0; i < 6; i++) {
        if (code[i] != slot->pin[i]) {
            return 0;
        }
    }
    return 1;
}

static int ppl_count_matching_profiles(const unsigned char* code) {
    int count;
    int device;
    int slotIndex;
    StorageDevice* dev;
    StorageProfileSlot* slot;

    count = 0;
    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        dev = DEVICE_AT(device);
        for (slotIndex = 0; slotIndex < STORAGE_MAX_SLOTS; slotIndex++) {
            slot = &dev->profiles[slotIndex];
            if (slot->present != 0) {
                if (multi_code_matches_slot(code, slot) != 0 &&
                    dev->inUse[slotIndex] == 0) {
                    count += 1;
                }
            }
        }
    }
    return count;
}

static int ppl_fill_matching_names(const unsigned char* code, char** out) {
    int count;
    int device;
    int slotIndex;
    StorageDevice* dev;
    StorageProfileSlot* slot;

    count = 0;
    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        dev = DEVICE_AT(device);
        for (slotIndex = 0; slotIndex < STORAGE_MAX_SLOTS; slotIndex++) {
            slot = &dev->profiles[slotIndex];
            if (slot->present != 0 && dev->inUse[slotIndex] == 0) {
                if (multi_code_matches_slot(code, slot) != 0) {
                    out[count] = slot->name;
                    count += 1;
                }
            }
        }
    }
    return count;
}

/* Screen multi-profile list: fills out[] with name string pointers; returns count. */
int ppl_get_multi_profile_names_p2(char** out) {
    int i;
    MkProc* proc;
    PplListPdata* list;
    int count;

    for (i = 0; i < PPL_NAME_SLOTS; i++) {
        out[i] = "";
    }
    count = 0;
    proc = find_mkproc_pid(PPL_LIST_PID_P2);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            count = ppl_fill_matching_names(list->code, out);
        }
    }
    return count;
}

int ppl_get_multi_profile_names_p1(char** out) {
    int i;
    MkProc* proc;
    PplListPdata* list;
    int count;

    for (i = 0; i < PPL_NAME_SLOTS; i++) {
        out[i] = "";
    }
    count = 0;
    proc = find_mkproc_pid(PPL_LIST_PID_P1);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            count = ppl_fill_matching_names(list->code, out);
        }
    }
    return count;
}

/* Soft ceiling: ppl_get_multi_profile_icons -- device/slot ring + TGA load; stop. */
void ppl_get_multi_profile_icons(unsigned char* code, int* out, int count);

void ppl_get_multi_profile_icon_p2(int* out, int count) {
    int i;
    int* dest;
    MkProc* proc;
    PplListPdata* list;
    int pack[2];

    dest = (int*)out[0];
    i = 0;
    if (count > 0) {
        for (i = 0; i < count; i++) {
            dest[i] = 0;
        }
    }
    proc = find_mkproc_pid(PPL_LIST_PID_P2);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            pack[0] = out[0];
            pack[1] = out[1];
            ppl_get_multi_profile_icons(list->code, pack, count);
        }
    }
}

void ppl_get_multi_profile_icon_p1(int* out, int count) {
    int i;
    int* dest;
    MkProc* proc;
    PplListPdata* list;
    int pack[2];

    dest = (int*)out[0];
    i = 0;
    if (count > 0) {
        for (i = 0; i < count; i++) {
            dest[i] = 0;
        }
    }
    proc = find_mkproc_pid(PPL_LIST_PID_P1);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            pack[0] = out[0];
            pack[1] = out[1];
            ppl_get_multi_profile_icons(list->code, pack, count);
        }
    }
}

void ppl_get_multi_profile_icons(unsigned char* code, int* out, int count) {
    int i;
    int device;
    int slot;
    int screenSlot;
    RwTexture** textures;
    StorageProfileSlot* profile;

    screenSlot = PPWLS_SCREEN_SLOT;
    if (p_pselect() != 0) {
        screenSlot = PROFILE_MENU_SCREEN_SLOT;
    }
    textures = (RwTexture**)out[0];
    device = STORAGE_MAX_DEVICES - 1;
    slot = STORAGE_MAX_SLOTS - 1;
    for (i = 0; i < count; i++) {
        if (move_to_profile(1, code, &device, &slot) != 0) {
            profile = &DEVICE_AT(device)->profiles[slot];
            textures[i] = load_named_tga_from_slot(screenSlot, ppwls_icon[profile->icon * 2]);
        }
    }
}

int ppl_get_multi_profile_count(int player) {
    /* Soft ceiling: ~85% -- pin CTR emit / NV; algo OK. */
    MkProc* proc;
    PplListPdata* list;
    int count;

    count = 0;
    if (player == 0) {
        proc = find_mkproc_pid(PPL_LIST_PID_P1);
    } else {
        proc = find_mkproc_pid(PPL_LIST_PID_P2);
    }
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            count = ppl_count_matching_profiles(list->code);
        }
    }
    return count;
}

StorageProfileSlot* scan_storage_for_code(int* state, int player, int port,
                                          unsigned char* code, int* device, int* slot) {
    int matchCount;
    int previousCount;
    int cursor;
    int timer;
    int selected;
    StorageProfileSlot* profile;

    if (is_memcard_scanner_running() != 0) {
        kill_async_memcard_scan();
    }

    for (;;) {
        if (update_storage_status(0) != 0) {
            fire_screen_studio_event(0x1FE4, 1);
        }

        matchCount = ppl_count_matching_profiles(code);
        if (matchCount == 0) {
            *device = -1;
            *slot = -1;
            *state = 1;
            ((int*)&profile_code_state)[player] = 8;
            fire_screen_studio_event(0x1FAA, player + 1);
            return 0;
        }

        if (move_to_profile(1, code, device, slot) == 0) {
            *state = 1;
            return 0;
        }
        profile = &DEVICE_AT(*device)->profiles[*slot];

        if (matchCount == 1) {
            *state = 2;
            DEVICE_AT(*device)->inUse[*slot] = 1;
            return profile;
        }

        ((int*)&profile_code_state)[player] = 10;
        fire_screen_studio_event(0x1FAA, player + 1);
        previousCount = matchCount;
        timer = 0;
        selected = 0;

        while (selected == 0) {
            matchCount = ppl_count_matching_profiles(code);
            if (matchCount != previousCount) {
                break;
            }

            timer -= 1;
            if (timer < 0) {
                if (update_storage_status(0) != 0) {
                    fire_screen_studio_event(0x1FE4, 1);
                    fire_screen_studio_event(0x1FE4, 2);
                    break;
                }
                timer = 0x1E;
            }

            if (check_switch_action(port, 2) != 0) {
                if (player == 0) {
                    fire_screen_studio_event(0x1FC2, 1);
                    cursor = get_multi_profile_cursor_p1();
                } else {
                    fire_screen_studio_event(0x1FC3, player + 1);
                    cursor = get_multi_profile_cursor_p2();
                }
                if (move_to_profile(cursor + 1, code, device, slot) != 0) {
                    profile = &DEVICE_AT(*device)->profiles[*slot];
                    *state = 2;
                    DEVICE_AT(*device)->inUse[*slot] = 1;
                    selected = 1;
                }
                fire_screen_studio_event(0x1FE4, 1);
                fire_screen_studio_event(0x1FE4, 2);
            }

            if (selected == 0 && check_switch_action(port, 1) != 0) {
                profile = 0;
                *state = 3;
                selected = 1;
            }
            if (selected == 0) {
                mkproc_sleep();
            }
        }

        if (selected != 0) {
            return profile;
        }
    }
}

/* Copy active profile blob into dest (prep for CARD write). */
void memory_save_profile(int player, PlayerProfile* dest) {
    if (player == 0) {
        memcpy(dest, &p1_profile, PROFILE_SIZE);
    } else {
        memcpy(dest, &p2_profile, PROFILE_SIZE);
    }
}

/* Copy src blob into active profile (after CARD read). */
void memory_load_profile(int player, PlayerProfile* src) {
    if (player == 0) {
        memcpy(&p1_profile, src, PROFILE_SIZE);
    } else {
        memcpy(&p2_profile, src, PROFILE_SIZE);
    }
}

/*
 * Soft ceiling: move_profile_* ~99% -- stmw / default-reset schedule. Soft OK.
 * Returns 1 on success (dest was empty), else 0.
 */
int move_profile_p2_to_p1(void) {
    if (p1_profile_status != 0) {
        return 0;
    }
    memcpy(&p1_profile, &p2_profile, PROFILE_SIZE);
    p1_profile_status = p2_profile_status;
    p1_profile_device = p2_profile_device;
    p1_profile_slot = p2_profile_slot;
    profile_code_state.p1 = profile_code_state.p2;
    set_profile_to_default(&p2_profile);
    p2_profile_status = 0;
    p2_profile_device = -1;
    p2_profile_slot = -1;
    profile_code_state.p2 = 0;
    p1_rumble_on = p2_rumble_on;
    p2_rumble_on = 0;
    return 1;
}

int move_profile_p1_to_p2(void) {
    if (p2_profile_status != 0) {
        return 0;
    }
    memcpy(&p2_profile, &p1_profile, PROFILE_SIZE);
    p2_profile_status = p1_profile_status;
    p2_profile_device = p1_profile_device;
    p2_profile_slot = p1_profile_slot;
    profile_code_state.p2 = profile_code_state.p1;
    set_profile_to_default(&p1_profile);
    p1_profile_status = 0;
    p1_profile_device = -1;
    p1_profile_slot = -1;
    profile_code_state.p1 = 0;
    p2_rumble_on = p1_rumble_on;
    p1_rumble_on = 0;
    return 1;
}

int count_all_profiles(void) {
    int total;
    int i;

    total = 0;
    for (i = 0; i < STORAGE_MAX_DEVICES; i++) {
        total += DEVICE_AT(i)->profileCount;
    }
    return total;
}

void mark_profile_as_in_use(int device, int slot) {
    if (device < 0 || device >= STORAGE_MAX_DEVICES) {
        return;
    }
    if (slot < 0 || slot >= STORAGE_MAX_SLOTS) {
        return;
    }
    DEVICE_AT(device)->inUse[slot] = 1;
}

/*
 * Soft ceiling: find_device_display_status -- status map / branch schedule; algo OK.
 * Display codes: 0 empty/unknown, 1 absent, 2 full, 3 ready, 4 error, 5 unformatted,
 * 6..9 mapped from raw status 6/8/9/10/0xb.
 */
int find_device_display_status(int device) {
    int status;

    if (device < 0 || device > 1) {
        return -1;
    }
    status = DEVICE_AT(device)->status;
    if (status == STORAGE_STATUS_BROKEN_FILE) {
        return 6;
    }
    if (status == STORAGE_STATUS_8) {
        return 7;
    }
    if (status == STORAGE_STATUS_FORMAT_NEEDED) {
        return 8;
    }
    if (status == STORAGE_STATUS_10) {
        return 9;
    }
    if (status == STORAGE_STATUS_FORMAT_ALT) {
        return 8;
    }
    if (is_device_unformatted(device) != 0) {
        return 5;
    }
    if (is_device_present(device) == 0) {
        return 1;
    }
    if (is_device_error(device) != 0) {
        return 4;
    }
    if (is_device_full(device) != 0) {
        return 2;
    }
    if (DEVICE_AT(device)->status == STORAGE_STATUS_NO_FILE) {
        if (is_storage_device_full(device) != 0) {
            return 2;
        }
        return 3;
    }
    return 0;
}

/* Soft ceiling: p_reset_ppwls_timeout ~99.7% -- float @sda21 pool names only; stop. */
float p_reset_ppwls_timeout(void) {
    int ticks_left;

    ticks_left = PPWLS_TIMEOUT_TICKS;
    while (ticks_left != 0) {
        _mkproc_sleep_ticks = kOne;
        mkproc_sleep();
        if (mcard_msg_active == 0) {
            ticks_left--;
        }
    }
    ppwls_input_done = 1;
    return kZero;
}

void reset_ppwls_timeout(void) {
    spawn_ppwls_timeout_proc();
}

/*
 * Soft ceiling: format_or_recreate_a_device -- format/delete/create schedule; algo OK.
 * Called from check_format_or_recreate during PPWLS loop.
 */
float format_or_recreate_a_device(int device) {
    int status;

    status = DEVICE_AT(device)->status;
    if (status == STORAGE_STATUS_FORMAT_NEEDED || status == STORAGE_STATUS_UNFORMATTED ||
        status == STORAGE_STATUS_FORMAT_ALT || is_device_unformatted(device) != 0) {
        format_card_and_create_mkda_file(device);
        fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
        update_storage_status(0);
    }
    if (DEVICE_AT(device)->status == STORAGE_STATUS_BROKEN_FILE) {
        if (gc_delete_file(device, MEMCARD_SAVE_FILENAME) != 0) {
            DEVICE_AT(device)->status = STORAGE_STATUS_NO_FILE;
        }
        fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
        update_storage_status(0);
    }
    if (DEVICE_AT(device)->status == STORAGE_STATUS_NO_FILE) {
        create_new_mk5_profile_file(device);
        fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
        update_storage_status(0);
    }
    spawn_ppwls_timeout_proc();
    return kZero;
}

/* Soft ceiling: p_player_profile_whats_loaded_screen ~96.26% -- recovered
 * PPWLS orchestration; residual is sleep/event scheduling and pool labels. */
float p_player_profile_whats_loaded_screen(void) {
    int status_changed;

    turn_camera_on();
    load_screen(PPWLS_SCREEN_PATH, PPWLS_SCREEN_SLOT, 0, 0);
    update_storage_status(0);
    gc_boot_space_check();
    load_game_settings();
    spawn_ppwls_timeout_proc();
    set_wls_left_cursor(0);
    fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
    fade_from_black(PPWLS_FADE_FRAMES, 1);
    _mkproc_sleep_ticks = kSleepIntro;
    mkproc_sleep();
    fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
    reset_format_or_recreate_flags();

    while (ppwls_input_done == 0) {
        status_changed = update_storage_status(1);
        if (status_changed != 0) {
            _mkproc_sleep_ticks = kOne;
            mkproc_sleep();
            fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
            spawn_ppwls_timeout_proc();
        }
        check_format_or_recreate();
        _mkproc_sleep_ticks = kSleepLoop;
        mkproc_sleep();
    }

    fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
    load_game_settings();
    _mkproc_sleep_ticks = kSleepPost;
    mkproc_sleep();
    fade_to_black(PPWLS_FADE_FRAMES, 1);
    turn_camera_off();
    fire_screen_studio_event(PPWLS_EVENT_DONE, 0);
    _mkproc_sleep_ticks = kOne;
    mkproc_sleep();
    gamelogic_jump(0, p_atm_loop);
    return kZero;
}

void set_ppwls_input_done(void) {
    ppwls_input_done = 1;
}

/* Soft ceiling: p_player_profile_boot_screen_entry_point ~99.4% -- float @sda21 pool names; stop. */
float p_player_profile_boot_screen_entry_point(void) {
    _mkproc_sleep_ticks = kOne;
    mkproc_sleep();
    set_section_memory_scheme(SECTION_MEMORY_SCHEME_PROFILE);
    mkproc_jump_sleep(p_player_profile_whats_loaded_screen);
    return kZero;
}

void pne_set_players_name_to_default(char* name, int* charPos) {
    int suffix;
    int unique;
    int device;
    int slot;
    int nameLen;
    int i;
    char* p;
    StorageProfileSlot* profile;
    const char* base;

    /* Soft ceiling: uniqueness walk emit; algo OK for create name stage. */
    suffix = 0;
    unique = 0;
    while (unique == 0 && suffix <= 0x62) {
        suffix++;
        base = nbc_find_text(0xb, 1);
        sprintf(name, "%s %d", base, suffix);
        unique = 1;
        for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
            for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
                profile = &DEVICE_AT(device)->profiles[slot];
                if (profile->present == 0) {
                    continue;
                }
                if (DEVICE_AT(device)->inUse[slot] != 0) {
                    continue;
                }
                nameLen = (int)strlen(name);
                if (nameLen != (int)strlen(profile->name)) {
                    continue;
                }
                for (i = 0; i <= nameLen; i++) {
                    if (name[i] != profile->name[i]) {
                        break;
                    }
                }
                if (i > nameLen) {
                    unique = 0;
                }
            }
        }
    }

    nameLen = (int)strlen(name);
    p = name + nameLen;
    for (i = nameLen; i < 10; i++) {
        *p++ = '_';
    }
    name[10] = '\0';
    *charPos = nameLen;
}

int does_name_already_exist(const char* name) {
    char local[0x1C];
    int i;
    int device;
    int slot;
    int hits;
    int nameLen;
    StorageProfileSlot* profile;
    char* p;
    const char* q;

    for (i = 0; i < 0xB; i++) {
        local[i] = name[i];
    }
    for (i = 0; i < 10; i++) {
        if (local[i] == '_') {
            local[i] = '\0';
        }
    }

    hits = 0;
    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
            profile = &DEVICE_AT(device)->profiles[slot];
            if (profile->present == 0) {
                continue;
            }
            if (DEVICE_AT(device)->inUse[slot] != 0) {
                continue;
            }
            nameLen = (int)strlen(local);
            if (nameLen != (int)strlen(profile->name)) {
                continue;
            }
            p = local;
            q = profile->name;
            for (i = 0; i <= nameLen; i++) {
                if (*p != *q) {
                    break;
                }
                p++;
                q++;
            }
            if (i > nameLen) {
                hits++;
            }
        }
    }
    return hits != 0;
}

/* Valid coffin indices are 0..0x257; reject with >= 0x258 (cmplwi; blt). */
#define COFFIN_BIT_COUNT 0x258

void set_coffin_bit(unsigned char* bits, unsigned int index, int value) {
    unsigned char mask;

    if (index >= COFFIN_BIT_COUNT) {
        return;
    }
    mask = (unsigned char)(1 << (index & 7));
    if (value != 0) {
        bits[index >> 3] = (unsigned char)(bits[index >> 3] | mask);
    } else {
        bits[index >> 3] = (unsigned char)(bits[index >> 3] & ~mask);
    }
}

int get_coffin_bit(const unsigned char* bits, unsigned int index) {
    if (index >= COFFIN_BIT_COUNT) {
        return 0;
    }
    /* Soft ceiling: get_coffin_bit ~98% -- slw operand reg color; stop. */
    return (bits[index >> 3] & (1 << (index & 7))) != 0;
}

/* Retail global; same body used by init/unload/move_profile. */
void set_profile_to_default(PlayerProfile* profile) {
    const char* default_name;

    memset(profile, 0, PROFILE_SIZE);
    profile->active = 1;
    default_name = nbc_find_text(NBC_DEFAULT_PROFILE_NAME, NBC_DEFAULT_PROFILE_SUB);
    strcpy(profile->name, default_name);
    copy_profile_switch_defaults(profile);
    memset(profile->konquest, 0, PROFILE_KONQUEST_SIZE);
    profile->unlock_cat7[1] = PROFILE_DEFAULT_UNLOCK_CAT7_LO;
    *(int*)(profile->konquest + KONQUEST_FIELD_68) = PROFILE_KONQUEST_FIELD_68;
    profile->unlock_cat7[0] = 0;
    profile->unlock_cat5 = PROFILE_DEFAULT_UNLOCK_CAT5;
}

/*
 * Soft ceiling: set_profile_to_default ~74% -- typed members vs byte/offset emit; stop.
 */

/*
 * Soft ceiling: unload_p* -- retail subfic/subfe for device/slot != -1;
 * switch_map arg is g_game_info.plyr0 / plyr1. Soft OK.
 */
void unload_p2_player_profile(void) {
    clear_storage_in_use(p2_profile_device, p2_profile_slot);
    set_profile_to_default(&p2_profile);
    p2_rumble_on = 0;
    p2_profile_status = 0;
    p2_profile_device = -1;
    p2_profile_slot = -1;
    switch_map_unload_player_profile(&g_game_info.plyr1);
    profile_code_state.p2 = 8;
}

void unload_p1_player_profile(void) {
    clear_storage_in_use(p1_profile_device, p1_profile_slot);
    set_profile_to_default(&p1_profile);
    p1_rumble_on = 0;
    p1_profile_status = 0;
    p1_profile_device = -1;
    p1_profile_slot = -1;
    switch_map_unload_player_profile(&g_game_info.plyr0);
    profile_code_state.p1 = 8;
}

/* Soft ceiling: unload_player_profiles -- wrapper inlines both leaf resets. */
void unload_player_profiles(void) {
    unload_p1_player_profile();
    unload_p2_player_profile();
}

/* Soft ceiling: init_player_profiles ~96.82% -- twin reset / NV schedule. Soft OK. */
void init_player_profiles(void) {
    set_profile_to_default(&p1_profile);
    p1_profile_status = 0;
    p1_profile_device = -1;
    p1_profile_slot = -1;
    set_profile_to_default(&p2_profile);
    p2_profile_status = 0;
    p2_profile_device = -1;
    p2_profile_slot = -1;
    p1_profile_common = p1_profile.name;
    p2_profile_common = p2_profile.name;
    p1_profile_konquest = p1_profile.konquest;
    p2_profile_konquest = p2_profile.konquest;
}
