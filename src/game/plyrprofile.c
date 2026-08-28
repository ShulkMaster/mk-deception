#include "game/plyrprofile.h"

#include "game/attract.h"
#include "game/game_info.h"
#include "game/konquest_save.h"
#include "game/memcard.h"
#include "game/menu.h"
#include "game/pselect.h"
#include "platform/gcmcardmsg.h"
#include "platform/main.h"
#include "platform/main_jump.h"
#include "runtime/cam.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/plyr_info.h"
#include "runtime/section.h"
#include "runtime/utils.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"

/*
 * plyrprofile.o - profiles + boot PPWLS (B21) + menu create/view/delete (B22).
 * See docs/campaigns/index.md (B20-B22).
 */

#pragma use_lmw_stmw on

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
void reset_sg_status(StorageDevice* device, int slot);
int save_konquest_region_to_memcard_w_error(int device, int slot, int mode, const char* title,
                                           unsigned int region, void* regionBuf, int flag,
                                           unsigned int* freeBlocks, int* freeBytes);
int format_card_and_create_mkda_file(int device);
int gc_delete_file(int device, const char* fileName);
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
static void pne_set_players_name_to_default(char* name, int* charPos);
static float p_player_profile_whats_loaded_screen(void);
int get_wls_left_cursor(void);
void set_sal_cursor(int v);
/* Ring walk remains open-coded so retail keeps this helper out of p_view_profile. */
static void pv_recalculate_profiles_and_position(int* outDevice, int* outSlot,
                                                 int* outCount, int* outPosition);
void erase_player_profile(int device, int slot);
void format_value_to_display(char* dest, unsigned int value);
int does_name_already_exist(const char* name);
RwTexture* load_named_tga_from_slot(int slot, const char* name);
unsigned long strlen(const char* s);
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

char player_name[0xB];

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

static inline void mkproc_sleep(void) {
    MkVtableMkprocLocal* vtbl;

    vtbl = (MkVtableMkprocLocal*)aproc->vtbl;
    vtbl->sleep();
}

/* Retail jump_sleep takes ticks in f1 (like pselect name-sound). */
static inline float mkproc_jump_sleep(MkProcEntryFn entry) {
    MkVtableMkprocLocal* vtbl;
    float (*js)(float, MkProcEntryFn);

    vtbl = (MkVtableMkprocLocal*)aproc->vtbl;
    js = (float (*)(float, MkProcEntryFn))vtbl->jump_sleep;
    return js(0.0f, entry);
}

/* .sdata2 */
static const float kOne = 1.0f;
static const float kNegOne = -1.0f;
static const float kSleepBeforeCreateScreen = 5.0f; /* @2555 */
static const float kInitialCancelDelay = 45.0f; /* @2556 */
static const float kSleepIntro = 10.0f; /* @2557 */
static const float kSleepLoop = 2.0f;   /* @2558 */
static const float kSleepPost = 50.0f;  /* @4196 */
static const float kZero = 0.0f;        /* @4199 / @1814 used as -1 */

/* MWCC emits tentative small-data objects in reverse declaration order. */
static unsigned char player_icon;
static unsigned char player_kode[6];
static int player_kode_current_digit;
static unsigned char player_confirm_kode[6];
static int player_confirm_kode_current_digit;
int pprofile_pad;
int pprofile_player;
int p1_profile_status;
int p1_profile_device;
int p1_profile_slot;
int p2_profile_status;
int p2_profile_device;
int p2_profile_slot;
static int button_answer;
int ppwls_input_done;
static int screen_obj;
static int position;
static int number_profiles;
int select_location_done;
int pne_char_position;
static int name_entry_done;

static int first_button_press = 1;
int profile_code_state[2] = {0, 0};
static int scan_cards_timer = 0x3c;
static int pos_device = -1;
static int pos_slot = -1;

typedef struct ProfileNameKey {
    const char* name;
    unsigned char value;
    unsigned char pad05[3];
} ProfileNameKey;

ProfileNameKey pne_alpha_data_table[39] = {
    {"NUM_00", '0'}, {"NUM_01", '1'}, {"NUM_02", '2'}, {"NUM_03", '3'},
    {"NUM_04", '4'}, {"NUM_05", '5'}, {"NUM_06", '6'}, {"NUM_07", '7'},
    {"NUM_08", '8'}, {"NUM_09", '9'},
    {"CHARACTER_A", 'A'}, {"CHARACTER_B", 'B'}, {"CHARACTER_C", 'C'},
    {"CHARACTER_D", 'D'}, {"CHARACTER_E", 'E'}, {"CHARACTER_F", 'F'},
    {"CHARACTER_G", 'G'}, {"CHARACTER_H", 'H'}, {"CHARACTER_I", 'I'},
    {"CHARACTER_J", 'J'}, {"CHARACTER_K", 'K'}, {"CHARACTER_L", 'L'},
    {"CHARACTER_M", 'M'}, {"CHARACTER_N", 'N'}, {"CHARACTER_O", 'O'},
    {"CHARACTER_P", 'P'}, {"CHARACTER_Q", 'Q'}, {"CHARACTER_R", 'R'},
    {"CHARACTER_S", 'S'}, {"CHARACTER_T", 'T'}, {"CHARACTER_U", 'U'},
    {"CHARACTER_V", 'V'}, {"CHARACTER_W", 'W'}, {"CHARACTER_X", 'X'},
    {"CHARACTER_Y", 'Y'}, {"CHARACTER_Z", 'Z'}, {"CHARACTER_SPC", ' '},
    {"CHARACTER_DEL", '_'}, {"CHARACTER_END", '0'},
};

typedef struct ProfileCodeKey {
    int switch_index;
    unsigned char value;
    unsigned char pad05[3];
} ProfileCodeKey;

ProfileCodeKey pne_kode_data_table[12] = {
    {12, 1}, {15, 2}, {14, 3}, {13, 4}, {2, 5}, {0, 6},
    {3, 7}, {1, 8}, {4, 9}, {7, 10}, {6, 11}, {5, 12},
};

extern ProfileUnlockBits64 default_char_bits;
extern ProfileUnlockBits64 default_alt_char_bits;
extern ProfileUnlockBits64 default_bgnd_bits;
extern ProfileUnlockBits64 default_pz_char_bits;
extern ProfileUnlockBits64 default_pz_bgnd_bits;
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
ProfileIconNames ppwls_icon[] = {
    {"MC_EMPTY_ICON", "MC_EMPTY_ICON_L"}, {"MC_ICON1", "MC_ICON1_L"},
    {"MC_ICON2", "MC_ICON2_L"},           {"MC_ICON3", "MC_ICON3_L"},
    {"MC_ICON4", "MC_ICON4_L"},           {"MC_ICON5", "MC_ICON5_L"},
    {"MC_ICON6", "MC_ICON6_L"},           {"MC_ICON7", "MC_ICON7_L"},
    {"MC_ICON8", "MC_ICON8_L"},           {"MC_ICON9", "MC_ICON9_L"},
    {"MC_ICON10", "MC_ICON10_L"},         {"MC_ICON11", "MC_ICON11_L"},
    {"MC_ICON12", "MC_ICON12_L"},         {"MC_ICON13", "MC_ICON13_L"},
    {"MC_ICON14", "MC_ICON14_L"},         {"MC_ICON15", "MC_ICON15_L"},
    {"MC_ICON16", "MC_ICON16_L"},         {"MC_ICON17", "MC_ICON17_L"},
    {"MC_ICON18", "MC_ICON18_L"},         {"MC_ICON19", "MC_ICON19_L"},
    {"MC_ICON20", "MC_ICON20_L"},         {"MC_ICON21", "MC_ICON21_L"},
    {"MC_ICON22", "MC_ICON22_L"},         {"MC_ICON23", "MC_ICON23_L"},
    {"MC_ICON24", "MC_ICON24_L"},
};

float p_reset_ppwls_timeout(void);
float p_atm_loop(void);
void set_profile_to_default(PlayerProfile* profile);
void unload_player_profiles(void);

static inline void spawn_ppwls_timeout_proc(void) {
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

PlayerProfile p1_profile;
PlayerProfile p2_profile;
extern void* p1_profile_common;
extern void* p2_profile_common;
extern void* p1_profile_konquest;
extern void* p2_profile_konquest;
extern int mcard_msg_active;
extern int default_switch_map[]; /* stride 0xC; copy word0 of each into profile +0x108 */
extern int p1_rumble_on;
extern int p2_rumble_on;

static inline void copy_profile_switch_defaults(PlayerProfile* profile) {
    int i;
    const char* src;

    src = (const char*)default_switch_map;
    for (i = 0; i < PROFILE_SWITCHMAP_COUNT; i++) {
        profile->switch_map[i] = *(const int*)(src + i * PROFILE_SWITCHMAP_STRIDE);
    }
}

static inline void clear_storage_in_use(int device, int slot) {
    if ((device != -1) & (slot != -1)) {
        if (device >= 0 && device < STORAGE_MAX_DEVICES &&
            slot >= 0 && slot < STORAGE_MAX_SLOTS) {
            DEVICE_AT(device)->inUse[slot] = 0;
        }
    }
}

static inline void set_profile_to_default_impl(PlayerProfile* profile) {
    unsigned char* konquest;

    memset(profile, 0, PROFILE_SIZE);
    profile->active = 1;
    strcpy(
        profile->name,
        nbc_find_text(NBC_DEFAULT_PROFILE_NAME, NBC_DEFAULT_PROFILE_SUB));
    copy_profile_switch_defaults(profile);
    konquest = profile->konquest;
    memset(konquest, 0, PROFILE_KONQUEST_SIZE);
    *(int*)(konquest + KONQUEST_FIELD_68) = PROFILE_KONQUEST_FIELD_68;
    profile->unlock_cat7.words[1] = PROFILE_DEFAULT_UNLOCK_CAT7_LO;
    profile->unlock_cat7.words[0] = 0;
    profile->unlock_cat6 = PROFILE_DEFAULT_UNLOCK_CAT5;
}

static inline int find_device_display_status_impl(int device, int compact_no_file) {
    int status;

    if (device < 0 || device >= STORAGE_MAX_DEVICES) return -1;
    status = DEVICE_AT(device)->status;
    if (status == STORAGE_STATUS_BROKEN_FILE) return 6;
    if (status == STORAGE_STATUS_8) return 7;
    if (status == STORAGE_STATUS_FORMAT_NEEDED) return 8;
    if (status == STORAGE_STATUS_10) return 9;
    if (status == STORAGE_STATUS_FORMAT_ALT) return 8;
    if (is_device_unformatted(device) != 0) return 5;
    if (is_device_present(device) != 0) {
        if (is_device_error(device) != 0) return 4;
        if (is_device_full(device) != 0) return 2;
        if (DEVICE_AT(device)->status == STORAGE_STATUS_NO_FILE) {
            if (compact_no_file != 0) {
                return -(is_storage_device_full(device) != 0) + 3;
            }
            if (is_storage_device_full(device) != 0) return 2;
            return 3;
        }
        return 0;
    }
    return 1;
}

/*
 * Soft ceiling: erase_player_profile -- retail retains a redundant upper-bound
 * branch that MWCC folds in clean structured C; the body is retail-correct.
 */
void erase_player_profile(int device, int slot) {
    StorageDevice* base;
    unsigned int* freeBlocks;
    int* freeBytes;
    const char* title;
    int region;
    int ok;

    if (device >= 0 && device < STORAGE_MAX_DEVICES && slot >= 0) {
        if (slot < STORAGE_MAX_SLOTS) {
            base = DEVICE_AT(device);
            reset_sg_status(base, slot);
            storage_status_change_calculations(device);
            mcard_msg_deleting_data(device);
            clear_region_buffer();

            freeBlocks = &base->freeBlocks;
            freeBytes = &base->freeBytes;
            region = 1;
            ok = 1;
            while (region < 9 && ok != 0) {
                title = nbc_find_text(NBC_MEMCARD_TITLE, 1);
                ok = save_konquest_region_to_memcard_w_error(
                    device, slot, 5, title, (unsigned char)region,
                    konq_region_data_buffer, 0, freeBlocks, freeBytes);
                if (ok != 0 && mcard_msg_active != MCARD_MSG_ACTIVE_PROGRESS) {
                    mcard_msg_end();
                    mcard_msg_deleting_data(device);
                }
                region++;
            }

            if (ok != 0) {
                title = nbc_find_text(NBC_MEMCARD_TITLE, 1);
                ok = save_to_memcard_w_error(
                    device, 4, title, &base->settings, 0,
                    freeBlocks, freeBytes);
            }

            if (ok != 0) {
                mcard_msg_delete_successful_generic();
            } else {
                mcard_msg_delete_failed_generic();
            }
            mcard_msg_end();
        }
    }
}

/*
 * Soft ceiling: p_delete_profile -- retail inlines the erase path; the remaining
 * differences are device/register and save-call argument scheduling.
 */
float p_delete_profile(void) {
    StorageDevice* storage;
    unsigned int* freeBlocks;
    int* freeBytes;
    const char* title;
    int statusChanged;
    int device;
    int slot;
    int region;
    int ok;

    number_profiles = 0;
    position = 0;
    screen_obj = 0;
    scan_cards_timer = 0x3C;
    pos_device = -1;
    pos_slot = -1;
    pprofile_player = menu_player;

    if (menu_player == 0 || menu_player == 1) {
        pprofile_pad = (&g_game_info.plyr0)[menu_player].pad_index;
        set_mode_of_play(3);
        push_game_state(0xD);
        set_player_state(&(&g_game_info.plyr0)[pprofile_player], 2);
        setup_sound_banks(1);
        wait_for_sound_banks_to_load();
        unload_player_profiles();
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
                            storage = DEVICE_AT(device);
                            reset_sg_status(storage, slot);
                            storage_status_change_calculations(device);
                            mcard_msg_deleting_data(device);
                            clear_region_buffer();

                            freeBlocks = &storage->freeBlocks;
                            freeBytes = &storage->freeBytes;
                            ok = 1;
                            region = 1;
                            while (region < 9 && ok != 0) {
                                title = nbc_find_text(NBC_MEMCARD_TITLE, 1);
                                ok = save_konquest_region_to_memcard_w_error(
                                    device, slot, 5, title, (unsigned char)region,
                                    konq_region_data_buffer, 0, freeBlocks,
                                    freeBytes);
                                if (ok != 0 && mcard_msg_active != MCARD_MSG_ACTIVE_PROGRESS) {
                                    mcard_msg_end();
                                    mcard_msg_deleting_data(device);
                                }
                                region++;
                            }

                            if (ok != 0) {
                                title = nbc_find_text(NBC_MEMCARD_TITLE, 1);
                                ok = save_to_memcard_w_error(
                                    device, 4, title, &storage->settings, 0,
                                    freeBlocks, freeBytes);
                            }
                            if (ok != 0) {
                                mcard_msg_delete_successful_generic();
                            } else {
                                mcard_msg_delete_failed_generic();
                            }
                            mcard_msg_end();
                        }
                    }
                }
                pv_recalculate_profiles_and_position(&pos_device, &pos_slot, &number_profiles,
                                                    &position);
                _mkproc_sleep_ticks = kOne;
                mkproc_sleep();
                fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
                button_answer = 0;
            }

            _mkproc_sleep_ticks = kOne;
            mkproc_sleep();
        }

        fade_to_black(PROFILE_MENU_FADE_FRAMES, 1);
    }

    set_player_state(&(&g_game_info.plyr0)[pprofile_player], 0);
    set_mode_of_play(0xD);
    pop_game_state();
    gamelogic_jump(6, p_main_menu);
    return kNegOne;
}

/* Screen GetString sink: dest buffer + koin index 0..5. */
void ppv_get_current_profile_koins(char* dest, int index) {
    StorageProfileSlot* slot;
    int value;

    if (pos_device >= 0 && pos_device < STORAGE_MAX_DEVICES &&
        pos_slot >= 0 && pos_slot < STORAGE_MAX_SLOTS) {
        slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    } else {
        slot = 0;
    }
    if (index < 0 || index >= 6 || slot == 0) {
        value = 0;
    } else {
        value = slot->koins[index];
    }
    format_value_to_display(dest, value);
}

/* outs[0..8] string buffers for the nine retail win/loss stat pairs. */
void get_profile_stats(char** outs) {
    StorageProfileSlot* slot;
    char result[12];
    char second_1[12], first_1[12];
    char second_2[12], first_2[12];
    char second_3[12], first_3[12];
    char second_4[12], first_4[12];
    char second_5[12], first_5[12];
    char second_6[12], first_6[12];
    char second_7[12], first_7[12];
    char second_8[12], first_8[12];
    char second_9[12], first_9[12];
    unsigned int raw_first;
    unsigned int raw_second;
    unsigned int first;
    unsigned int second;
    int out_index;

    for (out_index = 0; out_index < 9; out_index++) {
        strcpy(outs[out_index], "0 / 0");
    }

    if (pos_device >= 0 && pos_device < STORAGE_MAX_DEVICES &&
        pos_slot >= 0 && pos_slot < STORAGE_MAX_SLOTS) {
        slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    } else {
        slot = 0;
    }
    if (slot != 0) {
        raw_first = slot->view_stats_early[0][0];
        raw_second = slot->view_stats_early[0][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_1, first);
        format_value_to_display(second_1, second);
        sprintf(result, "%s / %s", first_1, second_1);
        strcpy(outs[0], result);

        raw_first = slot->view_stats_early[1][0];
        raw_second = slot->view_stats_early[1][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_2, first);
        format_value_to_display(second_2, second);
        sprintf(result, "%s / %s", first_2, second_2);
        strcpy(outs[1], result);

        raw_first = slot->view_stats_early[2][0];
        raw_second = slot->view_stats_early[2][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_3, first);
        format_value_to_display(second_3, second);
        sprintf(result, "%s / %s", first_3, second_3);
        strcpy(outs[2], result);

        raw_first = slot->view_stats_mid[0][0];
        raw_second = slot->view_stats_mid[0][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_4, first);
        format_value_to_display(second_4, second);
        sprintf(result, "%s / %s", first_4, second_4);
        strcpy(outs[3], result);

        raw_first = slot->view_stats_mid[1][0];
        raw_second = slot->view_stats_mid[1][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_5, first);
        format_value_to_display(second_5, second);
        sprintf(result, "%s / %s", first_5, second_5);
        strcpy(outs[4], result);

        raw_first = slot->view_stats_mid[2][0];
        raw_second = slot->view_stats_mid[2][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_6, first);
        format_value_to_display(second_6, second);
        sprintf(result, "%s / %s", first_6, second_6);
        strcpy(outs[5], result);

        raw_first = slot->view_stats_late[0][0];
        raw_second = slot->view_stats_late[0][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_7, first);
        format_value_to_display(second_7, second);
        sprintf(result, "%s / %s", first_7, second_7);
        strcpy(outs[6], result);

        raw_first = slot->view_stats_late[1][0];
        raw_second = slot->view_stats_late[1][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_8, first);
        format_value_to_display(second_8, second);
        sprintf(result, "%s / %s", first_8, second_8);
        strcpy(outs[7], result);

        raw_first = slot->view_stats_late[2][0];
        raw_second = slot->view_stats_late[2][1];
        first = 999999U;
        if (raw_first < 1000000U) first = raw_first;
        second = 999999U;
        if (raw_second < 1000000U) second = raw_second;
        format_value_to_display(first_9, first);
        format_value_to_display(second_9, second);
        sprintf(result, "%s / %s", first_9, second_9);
        strcpy(outs[8], result);
    }
}

void ppv_get_current_profile_arcade_finishes(char* dest) {
    StorageProfileSlot* slot;
    int value;

    if (pos_device >= 0 && pos_device < STORAGE_MAX_DEVICES &&
        pos_slot >= 0 && pos_slot < STORAGE_MAX_SLOTS) {
        slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    } else {
        slot = 0;
    }
    if (slot != 0) {
        value = slot->arcade_finishes;
    } else {
        value = 0;
    }
    format_value_to_display(dest, value);
}

/* Soft ceiling: ppv_get_current_profile_name -- storage index schedule. */
char* ppv_get_current_profile_name(void) {
    StorageProfileSlot* slot;

    if (pos_device >= 0 && pos_device < STORAGE_MAX_DEVICES &&
        pos_slot >= 0 && pos_slot < STORAGE_MAX_SLOTS) {
        slot = &DEVICE_AT(pos_device)->profiles[pos_slot];
    } else {
        slot = 0;
    }
    if (slot != 0) {
        return slot->name;
    }
    return nbc_find_text(NBC_EMPTY_PROFILE_NAME, 1);
}

static void pv_recalculate_profiles_and_position(int* outDevice, int* outSlot,
                                                 int* outCount, int* outPosition) {
    int count;
    int i;
    int device;
    int slot;
    int nextDevice;
    int nextSlot;

    count = 0;
    *outDevice = -1;
    *outSlot = -1;
    for (i = 0; i < STORAGE_MAX_DEVICES; i++) {
        count += DEVICE_AT(i)->profileCount;
    }
    *outCount = count;
    *outPosition = *outCount / 2;
    if (*outCount > 0) {
        *outDevice = 1;
        *outSlot = 6;
        for (i = 0; i < *outPosition + 1; i++) {
            device = *outDevice;
            slot = *outSlot;
            nextDevice = device;
            nextSlot = slot;
            if (device >= 0 && device < STORAGE_MAX_DEVICES && slot >= 0 &&
                slot < STORAGE_MAX_SLOTS) {
                if (slot < 6) {
                    nextSlot = slot + 1;
                } else {
                    nextSlot = 0;
                    if (device < 1) {
                        nextDevice = device + 1;
                    } else {
                        nextDevice = nextSlot;
                    }
                }
            }
            while (nextDevice != device || nextSlot != slot) {
                if (DEVICE_AT(nextDevice)->profiles[nextSlot].present == 1) {
                    *outDevice = nextDevice;
                    *outSlot = nextSlot;
                    break;
                }
                if (nextDevice >= 0 && nextDevice < STORAGE_MAX_DEVICES &&
                    nextSlot >= 0 && nextSlot < STORAGE_MAX_SLOTS) {
                    if (nextSlot < 6) {
                        nextSlot++;
                    } else {
                        nextSlot = 0;
                        if (nextDevice < 1) {
                            nextDevice++;
                        } else {
                            nextDevice = nextSlot;
                        }
                    }
                }
            }
            if (nextDevice == device && nextSlot == slot) {
                *outDevice = -1;
                *outSlot = -1;
            }
        }
    }
}

static inline void ppv_find_previous_present(int* device, int* slot) {
    int startDevice;
    int startSlot;
    int walkDevice;
    int walkSlot;

    startDevice = *device;
    startSlot = *slot;
    walkDevice = startDevice;
    walkSlot = startSlot;
    if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES &&
        walkSlot >= 0 && walkSlot < STORAGE_MAX_SLOTS) {
        if (walkSlot > 0) {
            walkSlot--;
        } else {
            walkSlot = STORAGE_MAX_SLOTS - 1;
            if (walkDevice > 0) {
                walkDevice--;
            } else {
                walkDevice = STORAGE_MAX_DEVICES - 1;
            }
        }
    }
    while (walkDevice != startDevice || walkSlot != startSlot) {
        if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1) {
            *device = walkDevice;
            *slot = walkSlot;
            return;
        }
        if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES &&
            walkSlot >= 0 && walkSlot < STORAGE_MAX_SLOTS) {
            if (walkSlot > 0) {
                walkSlot--;
            } else {
                walkSlot = STORAGE_MAX_SLOTS - 1;
                if (walkDevice > 0) {
                    walkDevice--;
                } else {
                    walkDevice = STORAGE_MAX_DEVICES - 1;
                }
            }
        }
    }
    *device = -1;
    *slot = -1;
}

static inline StorageProfileSlot* ppv_find_next_present(int* device, int* slot) {
    int startDevice;
    int startSlot;
    int walkDevice;
    int walkSlot;

    startDevice = *device;
    startSlot = *slot;
    walkDevice = startDevice;
    walkSlot = startSlot;
    if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES &&
        walkSlot >= 0 && walkSlot < STORAGE_MAX_SLOTS) {
        if (walkSlot < STORAGE_MAX_SLOTS - 1) {
            walkSlot++;
        } else {
            walkSlot = 0;
            if (walkDevice < STORAGE_MAX_DEVICES - 1) {
                walkDevice++;
            } else {
                walkDevice = 0;
            }
        }
    }
    while (walkDevice != startDevice || walkSlot != startSlot) {
        if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1) {
            *device = walkDevice;
            *slot = walkSlot;
            return &DEVICE_AT(walkDevice)->profiles[walkSlot];
        }
        if (walkDevice >= 0 && walkDevice < STORAGE_MAX_DEVICES &&
            walkSlot >= 0 && walkSlot < STORAGE_MAX_SLOTS) {
            if (walkSlot < STORAGE_MAX_SLOTS - 1) {
                walkSlot++;
            } else {
                walkSlot = 0;
                if (walkDevice < STORAGE_MAX_DEVICES - 1) {
                    walkDevice++;
                } else {
                    walkDevice = 0;
                }
            }
        }
    }
    *device = -1;
    *slot = -1;
    return 0;
}

static inline StorageProfileSlot* ppv_profile_at(int device, int slot) {
    StorageProfileSlot* result;

    if (device >= 0 && device < STORAGE_MAX_DEVICES &&
        slot >= 0 && slot < STORAGE_MAX_SLOTS) {
        result = &DEVICE_AT(device)->profiles[slot];
    } else {
        result = 0;
    }
    return result;
}

/* Soft ceiling: exact instruction stream/size; remaining differences are GPR coloring. */
void ppv_view_profile_icon_list(GVTexturePair out) {
    StorageProfileSlot* profile;
    int i;
    int before;
    int after;
    int start;
    int count;
    int device;
    int slot;
    int steps;

    device = pos_device;
    slot = pos_slot;
    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        out.colors[i] = 0;
    }
    if (number_profiles == 0) {
        return;
    }

    before = 3;
    if (position < 3) {
        before = position;
    }
    after = 3;
    if (number_profiles - position <= 3) {
        after = (number_profiles - position) - 1;
    }
    count = before + after + (number_profiles != 0);
    start = 3 - before;

    steps = before;
    while (steps > 0) {
        ppv_find_previous_present(&device, &slot);
        steps--;
    }

    for (i = start; i < start + count; i++) {
        if (i == start) {
            profile = ppv_profile_at(device, slot);
        } else {
            profile = ppv_find_next_present(&device, &slot);
        }
        if (profile != 0) {
            out.colors[i] =
                load_named_tga_from_slot(PROFILE_MENU_SCREEN_SLOT, ppwls_icon[profile->icon].alpha);
        }
    }
}

void ppv_update_profile_cursor(int delta) {
    int newPosition;

    newPosition = position + delta;
    if (newPosition >= 0 && newPosition < number_profiles) {
        if (delta > 0) {
            (void)ppv_find_next_present(&pos_device, &pos_slot);
            position++;
        } else {
            ppv_find_previous_present(&pos_device, &pos_slot);
            position--;
        }
    }
    fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
}

/* Soft ceiling: p_view_profile sleep/timer schedule; present OK. */
float p_view_profile(void) {
    int statusChanged;

    number_profiles = 0;
    position = 0;
    screen_obj = 0;
    scan_cards_timer = 0x3C;
    pos_device = -1;
    pos_slot = -1;
    pprofile_player = menu_player;

    if (menu_player == 0 || menu_player == 1) {
        pprofile_pad = (&g_game_info.plyr0)[menu_player].pad_index;
        set_mode_of_play(3);
        push_game_state(0xD);
        set_player_state(&(&g_game_info.plyr0)[pprofile_player], 2);
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
    set_player_state(&(&g_game_info.plyr0)[pprofile_player], 0);
    gamelogic_jump(6, p_main_menu);
    return kNegOne;
}

char* get_current_create_a_profile_name(void) {
    return player_name;
}

int pne_is_name_already_used(void) {
    return 0;
}

static inline void initialize_player_name_entry(void) {
    char* out;
    int i;

    if (first_button_press != 0) {
        out = player_name;
        for (i = 0; i < 10; i++) {
            *out++ = '_';
        }
        first_button_press = 0;
        player_name[10] = '\0';
        pne_char_position = 0;
    }
}

void pp_name_entry_proces_char_entry(const char* key_name) {
    int found;
    int key;
    int i;
    char* out;
    unsigned char value;

    found = 0;
    key = 0;
    if (key_name == 0) {
        return;
    }
    i = 0;
    while (i < 39 && found == 0) {
        strcmp(key_name, pne_alpha_data_table[i].name);
        if (strcmp(key_name, pne_alpha_data_table[i].name) == 0) {
            found = 1;
            key = i;
        }
        i++;
    }
    if (found == 0) {
        return;
    }

    switch (key) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
        initialize_player_name_entry();
        if (pne_char_position < 10) {
            value = pne_alpha_data_table[key].value;
            out = player_name;
            if (pne_char_position > 0) {
                for (i = 0; i < pne_char_position; i++) {
                    out++;
                }
            }
            *out = value;
            fire_screen_studio_event(0x2FA8, 0);
            if (pne_char_position < 10) {
                pne_char_position++;
            }
        }
        break;
    case 37:
        if (first_button_press != 0) {
            initialize_player_name_entry();
            pne_char_position = 1;
        }
        if (pne_char_position >= 1) {
            pne_char_position--;
            value = pne_alpha_data_table[key].value;
            out = player_name;
            if (pne_char_position > 0) {
                for (i = 0; i < pne_char_position; i++) {
                    out++;
                }
            }
            *out = value;
            fire_screen_studio_event(0x2FA8, 0);
        }
        break;
    case 38:
        if (pne_char_position > 0) {
            name_entry_done = 1;
        }
        break;
    default:
        break;
    }
}

void ppc_set_button_answer(int answer) {
    button_answer = answer;
}

void ppc_set_current_icon_selection(unsigned char icon) {
    player_icon = icon;
}

int ppc_get_code_state(void) {
    return profile_code_state[pprofile_player];
}

void ppc_transition_pause(int paused) {
    if (paused != 0) {
        turn_controllers_off();
    } else {
        turn_controllers_on();
        disable_all_ports_but_me(pprofile_pad);
    }
}

static inline void create_profile_sleep(float ticks) {
    _mkproc_sleep_ticks = ticks;
    mkproc_sleep();
}

/* Returns nonzero when the player confirms leaving profile creation. */
static inline int create_profile_cancel_prompt(float delay) {
    int answered;

    answered = 0;
    fire_screen_studio_event(PROFILE_MENU_EVENT_CANCEL_ASK, 0);
    button_answer = 0;
    if (delay != 0.0f) {
        create_profile_sleep(delay);
    }
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

static inline void clear_profile_code(unsigned char code[6], int* digit) {
    unsigned char* cursor;
    int i;

    cursor = code;
    for (i = 0; i < 6; i++) {
        *cursor++ = 0;
    }
    *digit = 0;
}

static inline void initialize_profile_code(unsigned char code[6], int* digit) {
    profile_code_state[pprofile_player] = 0;
    fire_screen_studio_event(0x1FAA, pprofile_player + 1);
    clear_profile_code(code, digit);
}

static inline int scan_profile_code(unsigned char code[6], int* digit) {
    int i;
    int position;
    int scan_player;
    int scan_pad;
    unsigned char* cursor;

    cursor = code;
    scan_player = pprofile_player;
    scan_pad = pprofile_pad;
    if (*digit < 6) {
        for (i = 0; i < 12; i++) {
            if (check_switch_edge(scan_pad,
                                  pne_kode_data_table[i].switch_index)) {
                position = 0;
                for (; position < *digit; position++) cursor++;
                *cursor = pne_kode_data_table[i].value;
                *digit += 1;
                profile_code_state[scan_player] = *digit;
                fire_screen_studio_event(0x1FAA, scan_player + 1);
                break;
            }
        }
    }
    return *digit != 6;
}

static inline void enter_profile_code(unsigned char code[6], int* digit) {
    int i;

    while (scan_profile_code(code, digit)) {
        create_profile_sleep(kOne);
        if (check_switch_edge(pprofile_pad, 0xB)) {
            for (i = 0; i < 6; i++) {
                code[i] = 0;
            }
            *digit = 0;
            if (pprofile_player == 0) {
                fire_screen_studio_event(0x1FC4, pprofile_player + 1);
            } else {
                fire_screen_studio_event(0x1FC5,
                                         pprofile_player + 1);
            }
            fire_screen_studio_event(0x1FAA, pprofile_player + 1);
        }
    }
    create_profile_sleep(kOne);
}

static inline int profile_codes_equal(
    const unsigned char entered[6], const unsigned char original[6]) {
    const unsigned char* entered_cursor;
    const unsigned char* original_cursor;
    int i;

    entered_cursor = entered;
    original_cursor = original;
    for (i = 0; i < 6; i++) {
        if (*original_cursor != *entered_cursor) return 0;
        original_cursor++;
        entered_cursor++;
    }
    return 1;
}

static inline int run_create_profile_name_phase(char* name) {
    for (;;) {
        ppc_set_stage_value(0);
        fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
        name_entry_done = 0;
        turn_controllers_on();
        disable_all_ports_but_me(pprofile_pad);
        button_answer = 0;

        while (name_entry_done == 0) {
            create_profile_sleep(kOne);
            if (button_answer == 2) {
                if (create_profile_cancel_prompt(kInitialCancelDelay)) {
                    return 0;
                }
                name_entry_done = 0;
            }
        }

        update_storage_status(0);
        if (does_name_already_exist(name)) {
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
        if (button_answer != 2) {
            return 1;
        }
    }
}

static inline int run_create_profile_icon_phase(void) {
    ppc_set_stage_value(2);
    fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
    button_answer = 0;
    while (button_answer != 1) {
        create_profile_sleep(kOne);
        if (button_answer == 2 && create_profile_cancel_prompt(0.0f)) {
            return 0;
        }
    }
    return 1;
}

float p_create_profile(void) {
    int restart_name_entry;
    int code_confirmed;
    int attempts;
    int device;
    int slot;
    int timer;
    int profileCount;
    int i;
    char* profile_name;
    char* name_cursor;

    set_mode_of_play(3);
    turn_controllers_off();
    name_entry_done = 0;
    pprofile_player = menu_player;
    if (menu_player == 0 || menu_player == 1) {
        pprofile_pad = (&g_game_info.plyr0)[menu_player].pad_index;
        push_game_state(0xD);
        set_player_state(&(&g_game_info.plyr0)[pprofile_player], 2);
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
        create_profile_sleep(kSleepBeforeCreateScreen);
        load_screen(PROFILE_CREATE_SCREEN, PROFILE_MENU_SCREEN_SLOT, 0, 1);
        profile_name = player_name;

        for (;;) {
            if (!run_create_profile_name_phase(profile_name)) {
                break;
            }
            name_cursor = player_name;
            for (i = 0; i < 10; i++) {
                if (*name_cursor == '_') {
                    *name_cursor = '\0';
                }
                name_cursor++;
            }

        if (!run_create_profile_icon_phase()) {
            break;
        }

        for (;;) {
            ppc_set_stage_value(3);
            fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
            initialize_profile_code(player_kode, &player_kode_current_digit);
            enter_profile_code(player_kode, &player_kode_current_digit);

            attempts = 3;
            code_confirmed = 0;
            ppc_set_stage_value(4);
            fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
            while (attempts != 0 && !code_confirmed) {
                attempts--;
                initialize_profile_code(player_confirm_kode,
                                        &player_confirm_kode_current_digit);
                enter_profile_code(player_confirm_kode,
                                   &player_confirm_kode_current_digit);
                code_confirmed = profile_codes_equal(
                    player_confirm_kode, player_kode);
            }
            if (code_confirmed != 0) break;
        }

        set_sal_cursor(0);
        timer = 0x1E;
        ppc_set_stage_value(5);
        fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
        create_profile_sleep(kOne);
        reset_format_or_recreate_flags();
        select_location_done = 0;
        button_answer = 0;
        restart_name_entry = 0;
        while (select_location_done == 0) {
            if (timer-- == 0) {
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
                if (device >= 0 && device < STORAGE_MAX_DEVICES) {
                    StorageDevice* device_status;

                    update_storage_status(0);
                    device_status = DEVICE_AT(device);
                    if (device_status->status == STORAGE_STATUS_OK) {
                        if (find_device_display_status_impl(device, 0) == 0) {
                            profileCount = device_status->profileCount;
                        } else {
                            profileCount = -1;
                        }
                        if (does_name_already_exist(player_name)) {
                            mcard_msg_name_conflict();
                            pne_set_players_name_to_default(
                                player_name, &pne_char_position);
                            restart_name_entry = 1;
                            break;
                        } else if (profileCount >= 0 &&
                                   profileCount < STORAGE_MAX_SLOTS) {
                            slot = -1;
                            if (device_status->status == 0) {
                                for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
                                    if (device_status->profiles[i].present == 0) {
                                        slot = i;
                                        break;
                                    }
                                }
                            }
                            if (slot != -1) {
                                reset_sg_status(device_status, slot);
                                device_status->profiles[slot].icon = player_icon;
                                move_player_name(
                                    player_name,
                                    device_status->profiles[slot].name);
                                move_player_pin(
                                    player_kode,
                                    device_status->profiles[slot].pin);
                                device_status->profiles[slot].present = 1;
                                device_status->profiles[slot].idChecksum =
                                    (int)random();
                                select_location_done = save_to_memcard_w_error(
                                    device, 6, nbc_find_text(0x30, 1),
                                    &device_status->settings, 0,
                                    &device_status->freeBlocks,
                                    &device_status->freeBytes);
                                snd_req(0x1AA5);
                            } else {
                                break;
                            }
                        } else {
                            snd_req(0x1AA8);
                        }
                    }
                } else {
                    snd_req(0x1AA8);
                }
                button_answer = 0;
            } else if (button_answer == 2) {
                if (create_profile_cancel_prompt(0.0f)) {
                    break;
                }
            }
        }
        if (restart_name_entry) {
            continue;
        }
        if (select_location_done == 0) {
            break;
        }
        create_profile_sleep(kOne);
        fire_screen_studio_event(PROFILE_MENU_EVENT_REFRESH, 0);
        create_profile_sleep(kOne);
        ppc_set_stage_value(6);
        fire_screen_studio_event(PROFILE_CREATE_EVENT_STAGE, 0);
        button_answer = 0;
        while (button_answer != 1) {
            create_profile_sleep(kOne);
        }
        break;
        }
    }

    turn_all_ports_on();
    turn_controllers_on();
    pop_game_state();
    set_mode_of_play(0xD);
    fade_to_black(PROFILE_MENU_FADE_FRAMES, 1);
    set_player_state(&(&g_game_info.plyr0)[pprofile_player], 0);
    gamelogic_jump(6, p_main_menu);
    return kNegOne;
}

void format_value_to_display(char* dest, unsigned int value) {
    char number[12] = {0};
    int length;

    strcpy(dest, "");
    sprintf(number, "%u", value);
    length = strlen(number);
    switch (length) {
    case 0:
    case 1:
    case 2:
    case 3:
        strcpy(dest, number);
        break;
    case 4:
    case 5:
    case 6:
        strncat(dest, number, length - 3);
        strcat(dest, ",");
        strncat(dest, number + length - 3, 3);
        break;
    case 7:
        strncat(dest, number, length - 6);
        strcat(dest, ".");
        strncat(dest, number + length - 6, 2);
        strcat(dest, " M");
        break;
    case 8:
        strncat(dest, number, length - 6);
        strcat(dest, ".");
        strncat(dest, number + length - 6, 1);
        strcat(dest, " M");
        break;
    case 9:
        strncat(dest, number, length - 6);
        strcat(dest, " M");
        break;
    case 10:
        strncat(dest, number, length - 9);
        strcat(dest, ".");
        strncat(dest, number + length - 9, 2);
        strcat(dest, " G");
        break;
    default:
        strcpy(dest, nbc_find_text(0x33, 1));
        break;
    }
}

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
    unsigned long long unlocked;
    unsigned int* words;

    unlocked = 0;
    if (profile != &p1_profile && profile != &p2_profile) {
        return 0;
    }

    switch (category) {
    case 1:
        if (character < 0 || character >= 44) {
            return 0;
        }
        mask = 1ULL << character;
        unlocked =
            (profile->unlock_cat1.value | default_char_bits.value) & mask;
        break;
    case 2:
        if (character < 0 || character >= 44) {
            return 0;
        }
        mask = 1ULL << character;
        unlocked = profile->unlock_cat2.value & mask;
        break;
    case 3:
        if (character < 0 || character >= 35) {
            return 0;
        }
        mask = 1ULL << character;
        unlocked = (unsigned long long)profile->unlock_cat3 & mask;
        break;
    case 4:
        if (character < 0 || character >= 44) {
            return 0;
        }
        mask = 1ULL << character;
        unlocked = profile->unlock_cat4.value & mask;
        break;
    case 5:
        if (character < 0 || character >= 11) {
            return 0;
        }
        mask = 1ULL << character;
        unlocked = (unsigned long long)profile->unlock_cat5 & mask;
        break;
    case 6:
        if (character < 0 || character >= 11) {
            return 0;
        }
        mask = 1ULL << (character + 10);
        unlocked = (unsigned long long)profile->unlock_cat6 & mask;
        break;
    case 7:
    case 8:
        if (character < 0 || character >= 44) {
            return 0;
        }
        words = category == 7 ? profile->unlock_cat7.words : profile->unlock_cat8.words;
        mask = 1ULL << character;
        unlocked = (((unsigned long long)words[0] << 32) | words[1]) & mask;
        break;
    case 9:
        if (character < 0 || character >= 44) {
            return 0;
        }
        mask = 1ULL << character;
        unlocked = profile->unlock_cat9.value & mask;
        break;
    case 10:
        if (character < 0 || character >= 35) {
            return 0;
        }
        mask = 1ULL << character;
        unlocked = (unsigned long long)profile->unlock_cat10 & mask;
        break;
    default:
        break;
    }
    return unlocked != 0;
}

static inline void mark_bitset_locked(
    unsigned int words[2], unsigned long long mask) {
    unsigned int low;
    unsigned int high;

    low = words[1];
    high = words[0];
    words[1] = low & (unsigned int)mask;
    words[0] = high & (unsigned int)(mask >> 32);
}

void mark_as_locked(PlayerProfile* profile, int category, int character) {
    unsigned long long mask;
    unsigned int* words;

    if (profile != &p1_profile && profile != &p2_profile) {
        return;
    }
    switch (category) {
    case 1:
        if (character < 0 || character >= 44) return;
        mask = ~(1ULL << character);
        mark_bitset_locked(profile->unlock_cat1.words, mask);
        return;
    case 2:
        if (character < 0 || character >= 44) return;
        mask = ~(1ULL << character);
        mark_bitset_locked(profile->unlock_cat2.words, mask);
        return;
    case 3:
        if (character < 0 || character >= 35) return;
        mask = ~(1ULL << character);
        profile->unlock_cat3 &= (unsigned int)mask;
        return;
    case 4:
        if (character < 0 || character >= 44) return;
        mask = ~(1ULL << character);
        mark_bitset_locked(profile->unlock_cat4.words, mask);
        return;
    case 5:
        if (character < 0 || character >= 11) return;
        mask = ~(1ULL << character);
        profile->unlock_cat5 &= (unsigned int)mask;
        return;
    case 6:
        if (character < 0 || character >= 11) return;
        mask = ~(1ULL << (character + 10));
        profile->unlock_cat6 &= (unsigned int)mask;
        return;
    case 7:
    case 8:
        if (character < 0 || character >= 44) return;
        words = category == 7 ? profile->unlock_cat7.words : profile->unlock_cat8.words;
        mask = ~(1ULL << character);
        mark_bitset_locked(words, mask);
        return;
    case 9:
        if (character < 0 || character >= 44) return;
        mask = ~(1ULL << character);
        mark_bitset_locked(profile->unlock_cat9.words, mask);
        return;
    case 10:
        if (character < 0 || character >= 35) return;
        mask = ~(1ULL << character);
        profile->unlock_cat10 &= (unsigned int)mask;
        return;
    default:
        return;
    }
}

static inline void mark_bitset_unlocked(
    unsigned int words[2], unsigned long long mask) {
    unsigned int low;
    unsigned int high;

    low = words[1];
    high = words[0];
    words[1] = low | (unsigned int)mask;
    words[0] = high | (unsigned int)(mask >> 32);
}

/* Consumers: nis, krypt handle_controller_input, projectile. */
void mark_as_unlocked(PlayerProfile* profile, int category, int character) {
    unsigned long long mask;
    unsigned int* words;

    if (profile != &p1_profile && profile != &p2_profile) {
        return;
    }
    switch (category) {
    case 1:
        if (character < 0 || character >= 44) return;
        mask = 1ULL << character;
        mark_bitset_unlocked(profile->unlock_cat1.words, mask);
        return;
    case 2:
        if (character < 0 || character >= 44) return;
        mask = 1ULL << character;
        mark_bitset_unlocked(profile->unlock_cat2.words, mask);
        return;
    case 3:
        if (character < 0 || character >= 35) return;
        profile->unlock_cat3 |= (unsigned int)(1ULL << character);
        return;
    case 4:
        if (character < 0 || character >= 44) return;
        mask = 1ULL << character;
        mark_bitset_unlocked(profile->unlock_cat4.words, mask);
        return;
    case 5:
        if (character < 0 || character >= 11) return;
        profile->unlock_cat5 |= (unsigned int)(1ULL << character);
        return;
    case 6:
        if (character < 0 || character >= 11) return;
        profile->unlock_cat6 |= (unsigned int)(1ULL << (character + 10));
        return;
    case 7:
    case 8:
        if (character < 0 || character >= 44) return;
        words = category == 7 ? profile->unlock_cat7.words : profile->unlock_cat8.words;
        mask = 1ULL << character;
        mark_bitset_unlocked(words, mask);
        return;
    case 9:
        if (character < 0 || character >= 44) return;
        mask = 1ULL << character;
        mark_bitset_unlocked(profile->unlock_cat9.words, mask);
        return;
    case 10:
        if (character < 0 || character >= 35) return;
        profile->unlock_cat10 |= (unsigned int)(1ULL << character);
        return;
    default:
        return;
    }
}

void summarize_unlocked_items(void) {
    int device;
    int slot;

    gp_data.cat1.value = default_char_bits.value;
    gp_data.cat5 = 0;
    gp_data.cat2.value = default_alt_char_bits.value;
    gp_data.cat3.value = default_bgnd_bits.value;
    gp_data.cat5 = PROFILE_DEFAULT_UNLOCK_CAT5;
    gp_data.cat7.value = PROFILE_DEFAULT_UNLOCK_CAT7_LO;
    gp_data.cat8.value = 0;
    gp_data.pz_chars.value = default_pz_char_bits.value;
    gp_data.pz_bgnds.value = default_pz_bgnd_bits.value;

    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        if (find_device_display_status_impl(device, 0) != 0) {
            continue;
        }
        for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
            if (DEVICE_AT(device)->profiles[slot].present == 0) {
                continue;
            }
            gp_data.cat1.value |=
                DEVICE_AT(device)->profiles[slot].unlock_cat1.value;
            gp_data.cat2.value |=
                DEVICE_AT(device)->profiles[slot].unlock_cat2.value;
            gp_data.cat3.value |=
                DEVICE_AT(device)->profiles[slot].unlock_cat3;
            gp_data.cat5 |= DEVICE_AT(device)->profiles[slot].unlock_cat6;
            gp_data.cat7.value |=
                DEVICE_AT(device)->profiles[slot].unlock_cat7.value;
            gp_data.cat8.value |=
                DEVICE_AT(device)->profiles[slot].unlock_cat8.value;
            gp_data.pz_chars.value |=
                DEVICE_AT(device)->profiles[slot].unlock_cat9.value;
            gp_data.pz_bgnds.value |=
                DEVICE_AT(device)->profiles[slot].unlock_cat10;
        }
    }
    pselect_update_profile_settings();
}

static inline int profile_pins_equal(
    PlayerProfile* live, StorageProfileSlot* slot) {
    unsigned char* live_pin;
    unsigned char* stored_pin;
    int i;

    live_pin = live->pin;
    stored_pin = slot->pin;
    for (i = 0; i < 6; i++) {
        if (*live_pin != *stored_pin) {
            return 0;
        }
        live_pin++;
        stored_pin++;
    }
    return 1;
}

static inline int profile_names_equal(
    PlayerProfile* live, StorageProfileSlot* slot) {
    char* live_name;
    char* stored_name;
    int i;

    live_name = live->name;
    stored_name = slot->name;
    for (i = 0; i < STORAGE_NAME_LEN; i++) {
        if (*live_name != *stored_name) {
            return 0;
        }
        live_name++;
        stored_name++;
    }
    return 1;
}

static inline int profile_fully_matches(
    PlayerProfile* live, StorageProfileSlot* slot) {
    if (profile_pins_equal(live, slot) == 0) return 0;
    if (profile_names_equal(live, slot) == 0) return 0;
    return live->idChecksum == slot->idChecksum;
}

static inline int profile_fully_matches_snapshot(
    PlayerProfile* live, StorageProfileSlot* slot, int id_checksum) {
    if (profile_pins_equal(live, slot) == 0) return 0;
    if (profile_names_equal(live, slot) == 0) return 0;
    return id_checksum == slot->idChecksum;
}

void check_new_mu_for_in_use_profiles(int device) {
    StorageDevice* new_device;
    StorageProfileSlot* stored;
    int slot;
    int identity_matches;
    int id_checksum;

    new_device = DEVICE_AT(device);
    if (new_device->status != 0) {
        return;
    }

    if (p1_profile_status == 1) {
        stored = &DEVICE_AT(p1_profile_device)->profiles[p1_profile_slot];
        identity_matches = profile_fully_matches(&p1_profile, stored);
        if (identity_matches == 0) {
            id_checksum = p1_profile.idChecksum;
            for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
                if (new_device->profiles[slot].present == 1) {
                    stored = &new_device->profiles[slot];
                    identity_matches = profile_fully_matches_snapshot(
                        &p1_profile, stored, id_checksum);
                    if (identity_matches != 0) {
                        new_device->inUse[slot] = 1;
                        p1_profile_device = device;
                        p1_profile_slot = slot;
                    }
                }
            }
        } else {
            DEVICE_AT(p1_profile_device)->inUse[p1_profile_slot] = 1;
        }
    }

    if (p2_profile_status == 1) {
        stored = &DEVICE_AT(p2_profile_device)->profiles[p2_profile_slot];
        identity_matches = profile_fully_matches(&p2_profile, stored);
        if (identity_matches == 0) {
            id_checksum = p2_profile.idChecksum;
            for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
                if (new_device->profiles[slot].present == 1) {
                    stored = &new_device->profiles[slot];
                    identity_matches = profile_fully_matches_snapshot(
                        &p2_profile, stored, id_checksum);
                    if (identity_matches != 0) {
                        new_device->inUse[slot] = 1;
                        p2_profile_device = device;
                        p2_profile_slot = slot;
                    }
                }
            }
        } else {
            DEVICE_AT(p2_profile_device)->inUse[p2_profile_slot] = 1;
        }
    }
}

/*
 * Consumer: save_profile (utils / CARD path).
 * Soft ceiling: the validate_* routines are exact-size; the remaining
 * differences are pointer-role coloring in their profile search loops.
 */
static inline void advance_device_slot(int* device, int* slot) {
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
        do {
            tries -= 1;
            found = ppv_find_next_present(&scanDevice, &scanSlot);
            if (found == 0) {
                matched = 0;
                break;
            }
            matched = profile_fully_matches(live, found);
        } while ((scanDevice != device || scanSlot != slotIndex) &&
                 matched == 0 && tries > 0);

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
    int* devicePtr;
    int* slotPtr;
    PlayerProfile* live;
    StorageProfileSlot* stored;
    StorageProfileSlot* found;
    int device;
    int slot;
    int scanDevice;
    int scanSlot;
    int tries;
    int waitLeft;
    int matched;

    if (player != 0 && player != 1) {
        return 0;
    }
    if (player == 0) {
        devicePtr = &p1_profile_device;
        slotPtr = &p1_profile_slot;
        live = &p1_profile;
    } else {
        devicePtr = &p2_profile_device;
        slotPtr = &p2_profile_slot;
        live = &p2_profile;
    }
    if (*devicePtr < 0 || *devicePtr >= STORAGE_MAX_DEVICES ||
        *slotPtr < 0 || *slotPtr >= STORAGE_MAX_SLOTS) {
        return 0;
    }
    if (live == 0) {
        return 0;
    }

    for (;;) {
        waitLeft = 0x1e;
        while (update_storage_status(0) != 0 && waitLeft > 0) {
            waitLeft--;
        }
        device = *devicePtr;
        slot = *slotPtr;
        stored = &DEVICE_AT(device)->profiles[slot];
        if (profile_fully_matches(live, stored)) {
            return 1;
        }

        scanDevice = device;
        scanSlot = slot;
        tries = 0x49;
        do {
            tries--;
            found = ppv_find_next_present(&scanDevice, &scanSlot);
            if (found == 0) {
                matched = 0;
                break;
            }
            matched = profile_fully_matches(live, found);
        } while ((scanDevice != device || scanSlot != slot) &&
                 matched == 0 && tries > 0);

        if (matched != 0) {
            *devicePtr = scanDevice;
            *slotPtr = scanSlot;
            return 1;
        }
        mcard_msg_save_no_card_konq_region_hault(p1_profile.name, 0);
        if (msg_save_no_card_konq_region_hault_answer == 2) {
            mcard_msg_end();
            mcard_msg_quit_confirmation();
            if (msg_quit_confirmation_answer == 1) {
                quit_from_konquest();
                return 0;
            }
        }
    }
}

int validate_konq_load_location(int player) {
    PlayerProfile* live;
    int* devicePtr;
    int* slotPtr;
    StorageProfileSlot* slot;
    StorageProfileSlot* found;
    StorageDevice* deviceStatus;
    int device;
    int slotIndex;
    int scanDevice;
    int scanSlot;
    int tries;
    int waitLeft;
    int matched;

    if (player != 0 && player != 1) {
        return 0;
    }
    if (player == 0) {
        devicePtr = &p1_profile_device;
        slotPtr = &p1_profile_slot;
        live = &p1_profile;
    } else {
        devicePtr = &p2_profile_device;
        slotPtr = &p2_profile_slot;
        live = &p2_profile;
    }
    if (*devicePtr < 0 || *devicePtr >= STORAGE_MAX_DEVICES ||
        *slotPtr < 0 || *slotPtr >= STORAGE_MAX_SLOTS) {
        return 0;
    }
    if (live == 0) {
        return 0;
    }

    do {
        waitLeft = 0x1e;
        while (update_storage_status(0) != 0 && waitLeft > 0) {
            waitLeft--;
        }
        device = *devicePtr;
        slotIndex = *slotPtr;
        deviceStatus = DEVICE_AT(device);
        slot = &deviceStatus->profiles[slotIndex];
        matched = profile_fully_matches(live, slot);
        if (matched != 0) {
            return 1;
        }

        scanDevice = device;
        scanSlot = slotIndex;
        tries = 0x49;
        do {
            tries--;
            found = ppv_find_next_present(&scanDevice, &scanSlot);
            if (found == 0) {
                matched = 0;
                break;
            }
            matched = profile_fully_matches(live, found);
        } while ((scanDevice != device || scanSlot != slotIndex) &&
                 matched == 0 && tries > 0);

        if (matched != 0) {
            *devicePtr = scanDevice;
            *slotPtr = scanSlot;
            return 1;
        }
        if (deviceStatus->status == 1) {
            mcard_msg_load_no_card_konq_region_hault(
                p1_profile.name, player, device);
            if (msg_load_no_card_konq_region_hault_answer == 2) {
                quit_from_konquest();
                return 0;
            }
        } else {
            mcard_msg_load_no_card_konq_region_hault(
                p1_profile.name, player, device);
            if (msg_load_no_card_konq_region_hault_answer == 2) {
                quit_from_konquest();
                return 0;
            }
        }
    } while (1);
}

void quit_from_konquest(void) {
    unload_player_profiles();
    gamelogic_jump(6, p_main_menu);
}

static inline int multi_code_matches_slot(
    const unsigned char* code, StorageProfileSlot* slot) {
    const unsigned char* code_cursor;
    unsigned char* pin_cursor;
    int i;

    code_cursor = code;
    pin_cursor = slot->pin;
    for (i = 0; i < 6; i++) {
        if (*code_cursor != *pin_cursor) {
            return 0;
        }
        code_cursor++;
        pin_cursor++;
    }
    return 1;
}

static inline int multi_code_matches_pin(
    const unsigned char* code, const unsigned char* pin) {
    int i;

    for (i = 0; i < 6; i++) {
        if (*code != *pin) {
            return 0;
        }
        code++;
        pin++;
    }
    return 1;
}

static inline StorageProfileSlot* find_next_matching_profile(
    const unsigned char* code, int* device, int* slot) {
    int startDevice;
    int startSlot;
    int walkDevice;
    int walkSlot;
    int first;

    startDevice = *device;
    startSlot = *slot;
    walkDevice = startDevice;
    walkSlot = startSlot;
    first = 1;
    advance_device_slot(&walkDevice, &walkSlot);
    while (walkDevice != startDevice || walkSlot != startSlot) {
        if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1 &&
            DEVICE_AT(walkDevice)->inUse[walkSlot] == 0 &&
            multi_code_matches_pin(
                code, DEVICE_AT(walkDevice)->profiles[walkSlot].pin) != 0) {
            *device = walkDevice;
            *slot = walkSlot;
            return &DEVICE_AT(walkDevice)->profiles[walkSlot];
        }
        advance_device_slot(&walkDevice, &walkSlot);
        if (first != 0) {
            advance_device_slot(&startDevice, &startSlot);
            first = 0;
        }
    }
    *device = -1;
    *slot = -1;
    return 0;
}

static inline void find_next_matching_slot(
    const unsigned char* code, int* device, int* slot) {
    int startDevice;
    int startSlot;
    int walkDevice;
    int walkSlot;
    int first;

    startDevice = *device;
    startSlot = *slot;
    walkDevice = startDevice;
    walkSlot = startSlot;
    first = 1;
    advance_device_slot(&walkDevice, &walkSlot);
    while (walkDevice != startDevice || walkSlot != startSlot) {
        if (DEVICE_AT(walkDevice)->profiles[walkSlot].present == 1 &&
            DEVICE_AT(walkDevice)->inUse[walkSlot] == 0 &&
            multi_code_matches_pin(
                code, DEVICE_AT(walkDevice)->profiles[walkSlot].pin) != 0) {
            *device = walkDevice;
            *slot = walkSlot;
            return;
        }
        advance_device_slot(&walkDevice, &walkSlot);
        if (first != 0) {
            advance_device_slot(&startDevice, &startSlot);
            first = 0;
        }
    }
    *device = -1;
    *slot = -1;
}

int move_to_profile(int count, unsigned char* code, int* devicePtr, int* slotPtr) {
    int i;
    int startDevice;
    int startSlot;
    int device;
    int slot;
    int first;

    i = 0;
    *devicePtr = STORAGE_MAX_DEVICES - 1;
    *slotPtr = STORAGE_MAX_SLOTS - 1;
    while (i < count) {
        device = *devicePtr;
        first = 1;
        slot = *slotPtr;
        startDevice = device;
        startSlot = slot;
        advance_device_slot(&device, &slot);
        while (device != startDevice || slot != startSlot) {
            if (DEVICE_AT(device)->profiles[slot].present == 1 &&
                DEVICE_AT(device)->inUse[slot] == 0 &&
                multi_code_matches_slot(
                    code, &DEVICE_AT(device)->profiles[slot]) != 0) {
                *devicePtr = device;
                *slotPtr = slot;
                break;
            }
            advance_device_slot(&device, &slot);
            if (first != 0) {
                advance_device_slot(&startDevice, &startSlot);
                first = 0;
            }
        }
        if (device == startDevice && slot == startSlot) {
            *devicePtr = -1;
            *slotPtr = -1;
        }
        if (*devicePtr == -1 || *slotPtr == -1) return 0;
        i++;
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
static inline int ppl_count_matching_profiles(const unsigned char* code) {
    int count;
    int device;
    int slotIndex;
    StorageDevice* dev;

    count = 0;
    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        dev = DEVICE_AT(device);
        for (slotIndex = 0; slotIndex < STORAGE_MAX_SLOTS; slotIndex++) {
            if (dev->profiles[slotIndex].present != 0) {
                if (multi_code_matches_slot(
                        code, &dev->profiles[slotIndex]) != 0 &&
                    dev->inUse[slotIndex] == 0) {
                    count += 1;
                }
            }
        }
    }
    return count;
}

static inline int ppl_fill_matching_names(
    const unsigned char* code, char** out) {
    int count;
    int device;
    int slotIndex;
    StorageDevice* dev;

    count = 0;
    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        dev = DEVICE_AT(device);
        for (slotIndex = 0; slotIndex < STORAGE_MAX_SLOTS; slotIndex++) {
            if (dev->profiles[slotIndex].present != 0 &&
                dev->inUse[slotIndex] == 0) {
                if (multi_code_matches_slot(
                        code, &dev->profiles[slotIndex]) != 0) {
                    out[count] = dev->profiles[slotIndex].name;
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
    const unsigned char* code;
    int count;

    count = 0;
    for (i = 0; i < PPL_NAME_SLOTS; i++) {
        out[i] = "";
    }
    proc = find_mkproc_pid(PPL_LIST_PID_P2);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            code = list->code;
            count = ppl_fill_matching_names(code, out);
        }
    }
    return count;
}

int ppl_get_multi_profile_names_p1(char** out) {
    int i;
    MkProc* proc;
    PplListPdata* list;
    const unsigned char* code;
    int count;

    count = 0;
    for (i = 0; i < PPL_NAME_SLOTS; i++) {
        out[i] = "";
    }
    proc = find_mkproc_pid(PPL_LIST_PID_P1);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            code = list->code;
            count = ppl_fill_matching_names(code, out);
        }
    }
    return count;
}

/* Soft ceiling: exact instruction stream/size; remaining differences are GPR coloring. */
static void ppl_get_multi_profile_icons(
    unsigned char* code, GVTexturePair* out, int count);

void ppl_get_multi_profile_icon_p2(GVTexturePair out, int count) {
    int i;
    MkProc* proc;
    PplListPdata* list;
    GVTexturePair copy;

    i = 0;
    for (i = 0; i < count; i++) {
        out.colors[i] = 0;
    }
    proc = find_mkproc_pid(PPL_LIST_PID_P2);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            copy = out;
            ppl_get_multi_profile_icons(list->code, &copy, count);
        }
    }
}

void ppl_get_multi_profile_icon_p1(GVTexturePair out, int count) {
    int i;
    MkProc* proc;
    PplListPdata* list;
    GVTexturePair copy;

    i = 0;
    for (i = 0; i < count; i++) {
        out.colors[i] = 0;
    }
    proc = find_mkproc_pid(PPL_LIST_PID_P1);
    if (proc != 0) {
        list = (PplListPdata*)pdata_of_proc(proc);
        if (list != 0) {
            copy = out;
            ppl_get_multi_profile_icons(list->code, &copy, count);
        }
    }
}

static void ppl_get_multi_profile_icons(
    unsigned char* code, GVTexturePair* out, int count) {
    int i;
    int device;
    int slot;
    int screenSlot;
    StorageProfileSlot* profile;

    device = STORAGE_MAX_DEVICES - 1;
    slot = STORAGE_MAX_SLOTS - 1;
    if (is_pselect_mode() != 0) {
        screenSlot = PSELECT_SEC_SLOT;
    } else {
        screenSlot = PPWLS_SCREEN_SLOT;
    }
    for (i = 0; i < count; i++) {
        profile = find_next_matching_profile(code, &device, &slot);
        if (profile != 0) {
            out->colors[i] =
                load_named_tga_from_slot(screenSlot, ppwls_icon[profile->icon].color);
        }
    }
}

int ppl_get_multi_profile_count(int player) {
    /* Soft ceiling: exact size/operations; remaining differences are GPR coloring. */
    MkProc* proc;
    PplListPdata* list;
    const unsigned char* code;
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
            code = list->code;
            count = ppl_count_matching_profiles(code);
        }
    }
    return count;
}

static inline StorageProfileSlot* storage_profile_at(
    const int* devicePtr, const int* slotPtr) {
    int device;
    int slot;

    device = *devicePtr;
    if (device >= 0 && device < STORAGE_MAX_DEVICES) {
        slot = *slotPtr;
        if (slot >= 0 && slot < STORAGE_MAX_SLOTS) {
            return &DEVICE_AT(device)->profiles[slot];
        }
    }
    return 0;
}

static inline void mark_profile_as_in_use_impl(int device, int slot) {
    if (device < 0) {
        return;
    }
    if (device >= STORAGE_MAX_DEVICES) {
        return;
    }
    if (slot < 0 || slot >= STORAGE_MAX_SLOTS) {
        return;
    }
    DEVICE_AT(device)->inUse[slot] = 1;
}

StorageProfileSlot* scan_storage_for_code(int* state, int player, int port,
                                          unsigned char* code, int* device, int* slot) {
    int matchCount;
    int previousCount;
    int cursor;
    int timer;
    int selected;
    int eventPlayer;
    StorageProfileSlot* profile;

    selected = 0;
    if (is_memcard_scanner_running() != 0) {
        kill_async_memcard_scan();
    }
    eventPlayer = player + 1;

    for (;;) {
        if (update_storage_status(0) != 0) {
            fire_screen_studio_event(0x1FE4, 1);
        }

        matchCount = ppl_count_matching_profiles(code);
        switch (matchCount) {
        case 0:
            *device = -1;
            *slot = -1;
            *state = 1;
            profile_code_state[player] = 8;
            fire_screen_studio_event(0x1FAA, eventPlayer);
            return 0;
        case 1:
            *device = STORAGE_MAX_DEVICES - 1;
            *slot = STORAGE_MAX_SLOTS - 1;
            find_next_matching_slot(code, device, slot);
            /* Retail repeats the device sentinel in both short-circuit arms. */
            if (*device == -1 || *device == -1) {
                *state = 1;
                return 0;
            }
            *state = 2;
            mark_profile_as_in_use_impl(*device, *slot);
            return &DEVICE_AT(*device)->profiles[*slot];
        default:
            *device = STORAGE_MAX_DEVICES - 1;
            *slot = STORAGE_MAX_SLOTS - 1;
            profile = find_next_matching_profile(code, device, slot);
            break;
        }

        profile_code_state[player] = 10;
        fire_screen_studio_event(0x1FAA, eventPlayer);
        previousCount = matchCount;
        timer = 0;

        while (selected == 0) {
            matchCount = ppl_count_matching_profiles(code);
            if (matchCount != previousCount) {
                break;
            }

            if (timer-- == 0) {
                if (update_storage_status(0) != 0) {
                    fire_screen_studio_event(0x1FE4, 1);
                    fire_screen_studio_event(0x1FE4, 2);
                    break;
                }
                timer = 0x1E;
            }

            if ((selected == 0) & check_switch_action(port, 2)) {
                if (player == 0) {
                    fire_screen_studio_event(0x1FC2, eventPlayer);
                    cursor = get_multi_profile_cursor_p1();
                } else {
                    fire_screen_studio_event(0x1FC3, eventPlayer);
                    cursor = get_multi_profile_cursor_p2();
                }
                if (move_to_profile(cursor + 1, code, device, slot) != 0) {
                    profile = storage_profile_at(device, slot);
                    *state = 2;
                    mark_profile_as_in_use_impl(*device, *slot);
                    selected = 1;
                }
                fire_screen_studio_event(0x1FE4, 1);
                fire_screen_studio_event(0x1FE4, 2);
            }

            if ((selected == 0) & check_switch_action(port, 1)) {
                profile = 0;
                *state = 3;
                selected = 1;
            }
            _mkproc_sleep_ticks = kOne;
            mkproc_sleep();
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
    profile_code_state[0] = profile_code_state[1];
    set_profile_to_default_impl(&p2_profile);
    p2_profile_status = 0;
    p2_profile_device = -1;
    p2_profile_slot = -1;
    profile_code_state[1] = 0;
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
    profile_code_state[1] = profile_code_state[0];
    set_profile_to_default_impl(&p1_profile);
    p1_profile_status = 0;
    p1_profile_device = -1;
    p1_profile_slot = -1;
    profile_code_state[0] = 0;
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
    if (device < 0) {
        return;
    }
    if (device >= STORAGE_MAX_DEVICES) {
        return;
    }
    if (slot < 0 || slot >= STORAGE_MAX_SLOTS) {
        return;
    }
    DEVICE_AT(device)->inUse[slot] = 1;
}

/* Display codes: 0 empty/unknown, 1 absent, 2 full, 3 ready, 4 error,
 * 5 unformatted, and 6..9 mapped from raw status 6/8/9/10/0xb. */
int find_device_display_status(int device) {
    return find_device_display_status_impl(device, 1);
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

/* Called from check_format_or_recreate during the PPWLS loop. */
void format_or_recreate_a_device(int device) {
    int status;

    status = DEVICE_AT(device)->status;
    if (status == STORAGE_STATUS_FORMAT_NEEDED || status == STORAGE_STATUS_UNFORMATTED ||
        DEVICE_AT(device)->status == STORAGE_STATUS_FORMAT_NEEDED ||
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
}

/* Soft ceiling: exact size/operations; two remaining differences are GPR coloring. */
static float p_player_profile_whats_loaded_screen(void) {
    int status_changed;

    turn_camera_on();
    load_screen(PPWLS_SCREEN_PATH, PPWLS_SCREEN_SLOT, 0, 0);
    update_storage_status(0);
    gc_boot_space_check();
    load_game_settings();
    ppwls_input_done = 0;
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

static inline int profile_name_text_equal(char* first, char* second) {
    int length;
    int i;

    length = (int)strlen(first);
    if (length != (int)strlen(second)) {
        return 0;
    }
    for (i = 0; i <= length; i++) {
        if ((signed char)*first != (signed char)*second) {
            return 0;
        }
        first++;
        second++;
    }
    return 1;
}

static void pne_set_players_name_to_default(char* name, int* charPos) {
    int suffix;
    int matches;
    int hits;
    int device;
    int slot;
    int nameLen;
    int i;
    char* p;
    StorageDevice* dev;

    suffix = 0;
    matches = 1;
    while (matches != 0 && suffix < 0x63) {
        suffix++;
        sprintf(name, "%s %d", nbc_find_text(0xb, 1), suffix);
        hits = 0;
        for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
            dev = DEVICE_AT(device);
            for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
                if (dev->profiles[slot].present != 0) {
                    if (profile_name_text_equal(
                            name, dev->profiles[slot].name) != 0 &&
                        dev->inUse[slot] == 0) {
                        hits++;
                    }
                }
            }
        }
        matches = hits;
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
    char local[STORAGE_NAME_LEN];
    int i;
    int device;
    int slot;
    int hits;
    StorageDevice* dev;
    char* p;
    const char* q;

    p = local;
    q = name;
    for (i = 0; i < 0xB; i++) {
        *p++ = *q++;
    }
    p = local;
    for (i = 0; i < 10; i++) {
        if (*p == '_') {
            *p = '\0';
        }
        p++;
    }

    hits = 0;
    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        dev = DEVICE_AT(device);
        for (slot = 0; slot < STORAGE_MAX_SLOTS; slot++) {
            if (dev->profiles[slot].present == 0) {
                continue;
            }
            if (profile_name_text_equal(
                    local, dev->profiles[slot].name) != 0 &&
                dev->inUse[slot] == 0) {
                hits++;
            }
        }
    }
    return hits != 0;
}

/* Valid coffin indices are 0..0x257; reject with >= 0x258 (cmplwi; blt). */
#define COFFIN_BIT_COUNT 0x258

void set_coffin_bit(unsigned char* bits, unsigned int index, int value) {
    unsigned int byte_index;
    unsigned int mask;

    if (index >= COFFIN_BIT_COUNT) {
        return;
    }
    byte_index = index >> 3;
    mask = 1 << (index & 7);
    if (value != 0) {
        bits[byte_index] = mask | bits[byte_index];
    } else {
        bits[byte_index] &= ~mask;
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
    set_profile_to_default_impl(profile);
}

/* Soft ceiling: exact size/operations; switch-map loop coloring remains. */

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
    profile_code_state[1] = 8;
}

void unload_p1_player_profile(void) {
    clear_storage_in_use(p1_profile_device, p1_profile_slot);
    set_profile_to_default(&p1_profile);
    p1_rumble_on = 0;
    p1_profile_status = 0;
    p1_profile_device = -1;
    p1_profile_slot = -1;
    switch_map_unload_player_profile(&g_game_info.plyr0);
    profile_code_state[0] = 8;
}

#pragma dont_inline on
void unload_player_profiles(void) {
    unload_p1_player_profile();
    unload_p2_player_profile();
}
#pragma dont_inline reset

/* Soft ceiling: exact size/operations; twin reset register coloring remains. */
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
