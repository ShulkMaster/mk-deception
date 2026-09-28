#include "game/memcard.h"

#include "game/konquest_save.h"
#include "game/nbc.h"
#include "game/plyrprofile.h"
#include "platform/gcmcard.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_vtbl.h"

#pragma use_lmw_stmw on

void pause_all_game_sounds(void);
void unpause_all_game_sounds(void);
void fire_screen_studio_event(int id, int arg);
int get_mode_of_play(void);
void mcard_msg_end(void);
void mcard_msg_read(int device);
void mcard_msg_save(int device);
void mcard_msg_auto_save(int device);
void mcard_msg_create(int device);
void mcard_msg_deleting_data(int device);
void mcard_msg_middle_sleep(int mode, int unused);
void mcard_msg_save_successful(void);
void mcard_msg_save_failed(int device);
void mcard_msg_create_successful(void);
void mcard_msg_create_failed(void);
void mcard_msg_delete_successful_generic(void);
void mcard_msg_delete_failed_generic(void);
void mcard_msg_profile_reset_confirmation(void);
void mcard_msg_cant_enter_konquest(int device, const char* profileName);
void update_storage_status_for_one_device(int device);

extern int f_writing_to_memcard;
extern char konq_region_data_buffer[0x1F54];
extern PlayerProfile p1_profile;
extern int p1_profile_device;
extern int p1_profile_slot;
extern int msg_cant_enter_konquest_answer;
extern int msg_profile_reset_confirmation_answer;

static const char stringBase0[] = " \0%d %s\0\0MKD";

#define STR_SPACE (&stringBase0[0])
#define STR_COUNT_UNIT (&stringBase0[2])
#define STR_EMPTY_NAME (&stringBase0[8])

static int states_when_device_present[7] = {0, 2, 5, 4, 6, 7, 3};
static int states_when_device_error[4] = {3, 4, 6, 7};

static char right_full_card_space_string[0x32];
static char left_full_card_space_string[0x32];
ProfileUnlockSummary gp_data;
StorageDevice storage_status[STORAGE_MAX_DEVICES];

static int format_request_flag[2];
static int states_when_device_full[1] = {STORAGE_STATUS_FULL};
static int states_when_device_unformatted[1] = {STORAGE_STATUS_UNFORMATTED};

void* p1_profile_common;
void* p2_profile_common;
void* p1_profile_konquest;
void* p2_profile_konquest;
int g_bMemCardScreensDisabled;
static int wls_device_cursor;
int mu_access_progress;
static int gap_08_80510D6C_sbss;

static const float kOne = 1.0f;
static const float kThree = 3.0f;

#define SAVE_CHUNK_SIZE 0x1F54
#define SAVE_PROFILE_STRIDE 0xFAA0
#define SAVE_EVENT_PROGRESS 0x1FBB

typedef struct KonquestRegionStateView {
    unsigned char pad00[0x5D];
    unsigned char field_0x5D;
} KonquestRegionStateView;

static inline char* storage_device_name(int device) {
    char* name = (char*)STR_SPACE;

    if (device < 0 || device >= 2) {
        return name;
    }
    name = DEVICE_AT(device)->name;
    return name;
}

void get_storage_device_name_list(char** out) {
    int i;
    char* name;

    for (i = 0; i < STORAGE_MAX_DEVICES; i++) {
        name = storage_device_name(i);
        if (strlen(name) == 0) {
            name = (char*)get_device_reference_name(i);
        }
        out[i] = name;
    }
}

void check_format_or_recreate(void) {
    int device;
    int flag;
    MkVtableMkproc* vtbl;

    for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
        flag = 0;
        if (device >= 0 && device < STORAGE_MAX_DEVICES) {
            flag = format_request_flag[device];
        }
        if (flag == 0) {
            continue;
        }
        update_storage_status(0);
        if (DEVICE_AT(device)->status != STORAGE_STATUS_UNFORMATTED &&
            DEVICE_AT(device)->status != STORAGE_STATUS_BROKEN_FILE &&
            DEVICE_AT(device)->status != STORAGE_STATUS_FORMAT_NEEDED &&
            DEVICE_AT(device)->status != STORAGE_STATUS_FORMAT_ALT &&
            DEVICE_AT(device)->status != STORAGE_STATUS_NO_FILE) {
            fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
            continue;
        }
        format_or_recreate_a_device(device);
        if (device >= 0 && device < STORAGE_MAX_DEVICES) {
            format_request_flag[device] = 0;
        }
        _mkproc_sleep_ticks = kOne;
        vtbl = aproc->vtbl;
        vtbl->sleep();
        fire_screen_studio_event(PPWLS_EVENT_REFRESH, 0);
    }
}

void reset_format_or_recreate_flags(void) {
    int i;

    for (i = 0; i < STORAGE_MAX_DEVICES; i++) {
        if (i >= 0 && i < STORAGE_MAX_DEVICES) {
            format_request_flag[i] = 0;
        }
    }
}

void format_or_recreate_right_device(void) {
    int device;

    device = wls_device_cursor + 1;
    if (device >= 0 && device < 2) {
        format_request_flag[device] = 1;
    }
}

void format_or_recreate_left_device(void) {
    if (wls_device_cursor >= 0 && wls_device_cursor < 2) {
        format_request_flag[wls_device_cursor] = 1;
    }
}

static inline void load_mc_icon_list(GVTexturePair out, int device) {
    int i;
    unsigned char icon;

    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        out.colors[i] = 0;
    }
    if (device < 0 || device >= STORAGE_MAX_DEVICES) {
        return;
    }
    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        icon = DEVICE_AT(device)->profiles[i].icon;
        out.colors[i] = load_named_tga_from_slot(PPWLS_SCREEN_SLOT, ppwls_icon[icon].color);
    }
}

void create_right_mc_icon_list(GVTexturePair out, int count) {
    load_mc_icon_list(out, wls_device_cursor + 1);
}

void create_left_mc_icon_list(GVTexturePair out, int count) {
    load_mc_icon_list(out, wls_device_cursor);
}

static inline void clear_mcard_text_matrix(char** out) {
    int i;

    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        out[i] = 0;
    }
}

void get_right_mcard_text_matrix(char** out) {
    int i;
    int device;

    device = wls_device_cursor + 1;
    clear_mcard_text_matrix(out);
    if (device < 0) {
        return;
    }
    if (device >= STORAGE_MAX_DEVICES) {
        return;
    }
    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        out[i] = DEVICE_AT(device)->profiles[i].name;
    }
}

void get_left_mcard_text_matrix(char** out) {
    int i;
    int device;

    device = wls_device_cursor;
    clear_mcard_text_matrix(out);
    if (device < 0) {
        return;
    }
    if (device >= STORAGE_MAX_DEVICES) {
        return;
    }
    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        out[i] = DEVICE_AT(device)->profiles[i].name;
    }
}

char* get_right_storage_device_space_needed(void) {
    char* result;
    int device;
    int needed;
    const char* unit;

    result = right_full_card_space_string;
    device = wls_device_cursor + 1;
    if (device < 0 || device >= STORAGE_MAX_DEVICES) {
        return result;
    }
    needed = STORAGE_BLOCKS_REQUIRED - (int)DEVICE_AT(device)->freeBlocks;
    if (needed < 0) {
        needed = 0;
    }
    if (needed == 1) {
        unit = nbc_find_text(0x1e, 1);
        sprintf(result, STR_COUNT_UNIT, needed, unit);
    } else {
        unit = nbc_find_text(0x1f, 1);
        sprintf(result, STR_COUNT_UNIT, needed, unit);
    }
    return right_full_card_space_string;
}

int get_right_storage_device_display_status(void) {
    int device;

    device = wls_device_cursor + 1;
    if (device < 0 || device >= 2) {
        return -1;
    }
    return find_device_display_status(device);
}

static inline char* storage_device_display_name(int device) {
    char* name = (char*)STR_EMPTY_NAME;

    if (device < 0 || device >= 2) {
        return name;
    }
    name = storage_device_name(device);
    if (strlen(name) == 0) {
        name = (char*)get_device_reference_name(device);
    }
    return name;
}

/* TODO: [near miss] 93.89%; empty-name pool address is scheduled before the
 * cursor load; likely needs the anonymous readonly string pool. */
char* get_right_storage_device_name(void) {
    int device = wls_device_cursor + 1;

    return storage_device_display_name(device);
}

char* get_left_storage_device_space_needed(void) {
    char* result;
    int needed;
    const char* unit;

    result = left_full_card_space_string;
    if (wls_device_cursor < 0 || wls_device_cursor >= 2) {
        return result;
    }
    needed = STORAGE_BLOCKS_REQUIRED - (int)DEVICE_AT(wls_device_cursor)->freeBlocks;
    if (needed < 0) {
        needed = 0;
    }
    if (needed == 1) {
        unit = nbc_find_text(0x1e, 1);
        sprintf(result, STR_COUNT_UNIT, needed, unit);
    } else {
        unit = nbc_find_text(0x1f, 1);
        sprintf(result, STR_COUNT_UNIT, needed, unit);
    }
    return left_full_card_space_string;
}

int get_left_storage_device_status(void) {
    if (wls_device_cursor < 0 || wls_device_cursor >= 2) {
        return -1;
    }
    return find_device_display_status(wls_device_cursor);
}

char* get_left_storage_device_name(void) {
    int device = wls_device_cursor;

    return storage_device_display_name(device);
}

void set_memcard_cursor_for(int delta) {
    if (delta < -1 || delta > 1) {
        return;
    }
    wls_device_cursor += delta;
    if (wls_device_cursor < 0) {
        wls_device_cursor = 0;
        return;
    }
    if (wls_device_cursor <= 1) {
        return;
    }
    wls_device_cursor = 1;
}

void add_to_wls_left_cursor(int delta) {
    if (delta < -1 || delta > 1) {
        return;
    }
    wls_device_cursor += delta;
    if (wls_device_cursor < 0) {
        wls_device_cursor = 0;
        return;
    }
    if (wls_device_cursor <= 0) {
        return;
    }
    wls_device_cursor = 0;
}

int get_wls_left_cursor(void) {
    return wls_device_cursor;
}

void set_sal_cursor(int device) {
    if (device < 0 || device > 2) {
        wls_device_cursor = 0;
        return;
    }
    wls_device_cursor = device;
}

void set_wls_left_cursor(int device) {
    if (device < 0 || device >= 2) {
        wls_device_cursor = 0;
        return;
    }
    wls_device_cursor = device;
}

int is_device_unformatted(int device) {
    int i;
    int status;

    status = DEVICE_AT(device)->status;
    for (i = 0; i < 1; i++) {
        if (status == states_when_device_unformatted[i]) {
            return 1;
        }
    }
    return 0;
}

/* TODO: [near miss] 79.11%; same table-scan register residue as is_device_present. */
int is_device_error(int device) {
    int i;
    int status;

    status = DEVICE_AT(device)->status;
    for (i = 0; i < 4; i++) {
        if (status == states_when_device_error[i]) {
            return 1;
        }
    }
    return 0;
}

int is_device_full(int device) {
    int i;
    int status;

    status = DEVICE_AT(device)->status;
    for (i = 0; i < 1; i++) {
        if (status == states_when_device_full[i]) {
            return 1;
        }
    }
    return 0;
}

/* TODO: [near miss] 79.11%; table scan agrees; table base/offset registers and
 * the status load schedule remain (int/unsigned/break forms measured). */
int is_device_present(int device) {
    int i;
    int status;

    status = DEVICE_AT(device)->status;
    for (i = 0; i < 7; i++) {
        if (status == states_when_device_present[i]) {
            return 1;
        }
    }
    return 0;
}

int is_storage_device_full(int device) {
    StorageDevice* dev;

    dev = DEVICE_AT(device);
    if (dev->freeBlocks < 0x3Au || (unsigned int)dev->freeBytes < 1u) {
        return 1;
    }
    return 0;
}

int is_memcard_scanner_running(void) {
    return find_mkproc_pid(MEMCARD_SCAN_PID) != 0;
}

void kill_async_memcard_scan(void) {
    destroy_mkprocs_pid(MEMCARD_SCAN_PID);
}

#pragma dont_inline on
void reset_sg_status(StorageDevice* device, int slot) {
    device->inUse[slot] = 0;
    set_profile_to_default((PlayerProfile*)&device->profiles[slot]);
}

/* TODO: [near miss] 82.51%; stores/calls agree; loop counter and zero-copy
 * registers are allocated in a different order. */
void reset_storage_device_status_structure(int device) {
    StorageDevice* base;
    int i;

    if (device >= 0 && device < STORAGE_MAX_DEVICES) {
        base = DEVICE_AT(device);
    base->status = -1;
    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        base->inUse[i] = 0;
        set_profile_to_default((PlayerProfile*)&base->profiles[i]);
    }
    DEVICE_AT(device)->profileCount = 0;
    summarize_unlocked_items();
    set_gsettings_to_default(&base->settings);
    }
}

void reset_all_storage_devices_status_structure(void) {
    int i;

    for (i = 0; i < STORAGE_MAX_DEVICES; i++) {
        reset_storage_device_status_structure(i);
    }
}
#pragma dont_inline reset

int create_new_mk5_profile_file(int device) {
    int ok;
    StorageDevice* base;

    if (device < 0 || device >= 2) {
        return 0;
    }
    base = DEVICE_AT(device);
    if (base->status != STORAGE_STATUS_NO_FILE) {
        return 0;
    }
    pause_all_game_sounds();
    ok = save_to_memcard_w_error(device, 3, nbc_find_text(0x41, 1), &base->settings, 1,
                                 &base->freeBlocks, &base->freeBytes);
    unpause_all_game_sounds();
    return ok;
}

int compare_checksums(const char* a, const char* b) {
    int i;

    for (i = 0; i < 4; i++) {
        if (*a++ != *b++) {
            return 0;
        }
    }
    return 1;
}

void move_player_pin(unsigned char* src, unsigned char* dest) {
    int i;

    for (i = 0; i < 6; i++) {
        *dest++ = *src++;
    }
}

void move_player_name(unsigned char* src, unsigned char* dest) {
    int i;

    for (i = 0; i < 0xB; i++) {
        *dest++ = *src++;
    }
}

/* TODO: [near miss] 97.10%; base/index GPR coloring remains; stop at coloring. */
void storage_status_change_calculations(int device) {
    int i;
    StorageDevice* base;
    unsigned char* profileCount;

    base = DEVICE_AT(device);
    profileCount = &base->profileCount;
    *profileCount = 0;
    for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
        if (base->profiles[i].present != 0) {
            *profileCount = *profileCount + 1;
        }
    }
}

int load_konquest_region_from_memcard_w_error(
    int device, int slot, int arg, int region, void* buffer, char* cardName,
    int nameLen, unsigned int* freeBlocks, int* freeBytes) {
    int result;
    int scratch;
    int cont;
    int attempts;
    unsigned int offset;

    scratch = 0;
    result = -100;
    offset = (region + slot * 8 - 1) * SAVE_CHUNK_SIZE + 0x28B8;
    do {
        result = -100;
        cont = 0;
        if (region - 1 < 0 || region - 1 >= 8) {
            return 0;
        }
        while (cont == 0) {
            attempts = 2;
            if (arg >= 1 && arg < 3) {
                mcard_msg_read(device);
            }
            while (attempts != 0 && result != 0) {
                attempts--;
                result = load_from_memcard2(
                    device, 0, offset, STR_EMPTY_NAME, &stringBase0[9], buffer,
                    SAVE_CHUNK_SIZE, cardName, nameLen, freeBlocks, freeBytes, &scratch);
            }
            if (result == 0 &&
                validate_region_buffer(
                    ((KonquestRegionStateView*)p1_profile_konquest)->field_0x5D) == 0) {
                region_data_corruption_message_handler();
            }
            mcard_msg_end();
            cont = check_load_region_data_result(&result, device, scratch, 0);
            mcard_msg_end();
        }
        if (result == 0) {
            return 1;
        }
    } while (bad_load_region_data_result_resolution(&result, device) == 0);
    return 0;
}

void region_data_corruption_message_handler(void) {
    mcard_msg_cant_enter_konquest(p1_profile_device, p1_profile.name);
    if (msg_cant_enter_konquest_answer == 2) {
        mcard_msg_end();
        mcard_msg_profile_reset_confirmation();
        if (msg_profile_reset_confirmation_answer == 1) {
            mcard_msg_end();
            erase_player_profile(p1_profile_device, p1_profile_slot);
        }
    }
    mcard_msg_end();
    quit_from_konquest();
}

#pragma dont_inline on
/* TODO: [near miss] 72.10%; nonvolatile homes for strs/storage/scratch and the
 * region-result call schedule remain. */
int load_from_memcard_w_error(int device, int mode, void* settings, char* cardName, int nameLen,
                              unsigned int* freeBlocks, int* freeBytes) {
    char* strs;
    int result;
    int scratch;
    int cont;
    StorageDevice* dev;

    strs = (char*)stringBase0;
    dev = DEVICE_AT(device);
    scratch = 0;
    result = -100;
    do {
        result = -100;
        cont = 0;
        while (cont == 0) {
            cont = 2;
            if (mode >= 1 && mode < 3) {
                mcard_msg_read(device);
            }
            while (cont != 0 && result != 0) {
                cont -= 1;
                result = load_from_memcard2(device, 0, 0, strs + 8, strs + 9, settings,
                                            STORAGE_LOAD_SIZE, cardName, nameLen, freeBlocks,
                                            freeBytes, &scratch);
            }
            mcard_msg_end();
            if (get_mode_of_play() == 7) {
                if (mode == 2) {
                    summarize_unlocked_items();
                    dev->status = result;
                    check_new_mu_for_in_use_profiles(device);
                    cont = 1;
                } else {
                    cont = check_load_region_data_result(&result, device, scratch, 1);
                }
            } else {
                cont = check_load_profile_result(&result, device);
            }
            mcard_msg_end();
        }
        if (get_mode_of_play() != 7 || mode == 2) {
            if (result == 0) {
                return 1;
            }
            return 0;
        }
        if (result == 0) {
            return 1;
        }
        cont = bad_load_region_data_result_resolution(&result, device);
    } while (cont == 0);
    return 0;
}
#pragma dont_inline reset

/* TODO: [near miss] 98.18%; switch emits a dead branch after the mode-6 fallthrough. */
void end_save_message(int mode, int result, int device, int flag) {
    switch (mode) {
    case 5:
        return;
    case 3:
        if (result == 0) {
            mcard_msg_create_successful();
        } else {
            mcard_msg_create_failed();
        }
        mcard_msg_end();
        return;
    case 1:
    case 2:
    case 6:
        if (result == 0) {
            mcard_msg_save_successful();
        } else {
            mcard_msg_save_failed(device);
        }
        mcard_msg_end();
        return;
    case 4:
        if (result == 0) {
            mcard_msg_delete_successful_generic();
        } else {
            mcard_msg_delete_failed_generic();
        }
        mcard_msg_end();
        return;
    case 7:
    case 8:
        if (result == 0) {
            if (flag != 1) {
                return;
            }
            mcard_msg_save_successful();
            mcard_msg_end();
            return;
        }
        mcard_msg_save_failed(device);
        mcard_msg_end();
        return;
    default:
        mcard_msg_end();
        return;
    }
}

/* TODO: [breakthrough needed] 68.64%; retail's region range check and retry/dispatch CFG differ; needs structural comparison. */
int save_konquest_region_to_memcard_w_error(int device, int slot, int mode, const char* title,
                                           unsigned int region, void* regionBuf, int flag,
                                           unsigned int* freeBlocks, int* freeBytes) {
    int result;
    int resolved;
    int tries;

    if (region < 1 || region > 8) {
        return 0;
    }

    do {
        result = 4;
        resolved = 0;
        f_writing_to_memcard = 1;
        while (resolved == 0) {
            switch (mode) {
            case 1:
            case 6:
            case 7:
                mcard_msg_save(device);
                break;
            case 2:
            case 8:
                mcard_msg_auto_save(device);
                break;
            case 3:
                mcard_msg_create(device);
                break;
            case 4:
            case 5:
                mcard_msg_deleting_data(device);
                break;
            }

            _mkproc_sleep_ticks = kThree;
            aproc->vtbl->sleep();

            tries = 2;
            while (tries-- != 0 && result != 0) {
                result = save_to_memcard2(
                    device, 0,
                    ((region - 1) + (unsigned int)slot * 8) * SAVE_CHUNK_SIZE + 0x28B8,
                    flag, STR_EMPTY_NAME, &stringBase0[9], regionBuf, SAVE_CHUNK_SIZE,
                    freeBlocks, freeBytes, 0, flag, mode, 0);
            }

            mcard_msg_middle_sleep(mode, 1);
            end_save_message(mode, result, device, 1);
            if (mode == 5) {
                resolved = check_save_profile_result(&result, device, 0);
            } else {
                resolved = check_save_region_data_result(&result, device, mode);
            }
            mcard_msg_end();
        }

        if (result == 0) {
            return 1;
        }
        if (mode == 5) {
            return 0;
        }
    } while (bad_save_region_data_result_resolution(&result, device) == 0);

    return 0;
}

/* TODO: [near miss] 79.08%; prologue homes, retry-loop, switch and vtable
 * scheduling remain. */
int save_settings_to_memcard_w_error(int device, int mode, const char* title,
                                     GameSettings* settings, int flag,
                                     unsigned int* freeBlocks, int* freeBytes) {
    MkVtableMkproc* vtbl;
    int result;
    int resolved;
    int tries;

    result = 4;
    resolved = 0;
    if (flag != 0) {
        return 0;
    }

    f_writing_to_memcard = 1;
    do {
        save_gsettings(device);
        switch (mode) {
        case 1:
        case 6:
        case 7:
            mcard_msg_save(device);
            break;
        case 2:
        case 8:
            mcard_msg_auto_save(device);
            break;
        case 3:
            mcard_msg_create(device);
            break;
        case 4:
        case 5:
            mcard_msg_deleting_data(device);
            break;
        }

        _mkproc_sleep_ticks = kThree;
        vtbl = aproc->vtbl;
        vtbl->sleep();

        tries = 2;
        resolved = 0;
        while (tries-- != 0 && result != 0) {
            result = save_to_memcard2(device, 0, 0, 0, stringBase0 + 8,
                                      stringBase0 + 9, settings, STORAGE_LOAD_SIZE,
                                      freeBlocks, freeBytes, 0, 0, mode, 0);
        }
        mcard_msg_middle_sleep(mode, 2);
        end_save_message(mode, result, device, 2);
        resolved = check_save_profile_result(&result, device, 0);
        mcard_msg_end();
        if (resolved == 0) {
            update_storage_status_for_one_device(device);
        }
    } while (resolved == 0);

    return result == 0;
}

#pragma dont_inline on
/* TODO: [near miss] 97.97%; stringBase0 pool alias (literals ""/"MKD" land in .sdata), progress add order and retry-loop coloring remain. */
int save_to_memcard_w_error(int device, int mode, const char* title, void* settings, int flag,
                            unsigned int* freeBlocks, int* freeBytes) {
    StorageDevice* dev;
    int* deviceFreeBytes;
    unsigned int* deviceFreeBlocks;
    unsigned int modeMinus1;
    unsigned int modeMinus6;
    int result;
    int cont;
    int tries;
    int profile;
    int chunk;
    int chunkOff;
    int progressBase;
    char* strs;
    MkVtableMkproc* vtbl;

    dev = DEVICE_AT(device);
    deviceFreeBytes = &dev->freeBytes;
    deviceFreeBlocks = &dev->freeBlocks;
    modeMinus1 = mode - 1;
    modeMinus6 = mode - 6;
    result = 0;

    for (;;) {
        result = 0;
        cont = 0;
        f_writing_to_memcard = 1;
        if (mode == 3) {
            mu_access_progress = 0;
            fire_screen_studio_event(SAVE_EVENT_PROGRESS, 0);
            _mkproc_sleep_ticks = kThree;
            vtbl = aproc->vtbl;
            vtbl->sleep();
        }
        while (cont == 0) {
            switch (mode) {
            case 0:
                break;
            case 1:
            case 6:
            case 7:
                mcard_msg_save(device);
                break;
            case 2:
            case 8:
                mcard_msg_auto_save(device);
                break;
            case 3:
                mcard_msg_create(device);
                break;
            case 4:
                mcard_msg_deleting_data(device);
                break;
            case 5:
                mcard_msg_deleting_data(device);
                break;
            }
            _mkproc_sleep_ticks = kOne;
            vtbl = aproc->vtbl;
            vtbl->sleep();

            if (mode == 3) {
                mu_access_progress = 9;
                fire_screen_studio_event(SAVE_EVENT_PROGRESS, 0);
                _mkproc_sleep_ticks = kThree;
                vtbl = aproc->vtbl;
                vtbl->sleep();
                strs = (char*)stringBase0;
                tries = 2;
                result = 4;
                while (tries-- != 0 && result != 0) {
                    result = save_to_memcard2(device, 0, 0, flag, strs + 8, strs + 9, settings,
                                              STORAGE_LOAD_SIZE, freeBlocks, freeBytes, 1, 0, mode,
                                              0);
                }
                mu_access_progress = 0x12;
                fire_screen_studio_event(SAVE_EVENT_PROGRESS, 0);
                _mkproc_sleep_ticks = kThree;
                vtbl = aproc->vtbl;
                vtbl->sleep();
                if (result == 0) {
                    memset(konq_region_data_buffer, 0, SAVE_CHUNK_SIZE);
                    profile = 0;
                    progressBase = 0;
                    while (profile < 7 && result == 0) {
                        chunk = 0;
                        chunkOff = 0;
                        while (chunk < 8 && result == 0) {
                            tries = 2;
                            result = 4;
                            while (tries-- != 0 && result != 0) {
                                result = save_to_memcard2(
                                    device, 0,
                                    profile * SAVE_PROFILE_STRIDE + chunkOff + STORAGE_LOAD_SIZE, 0,
                                    strs + 8, strs + 9, konq_region_data_buffer, SAVE_CHUNK_SIZE,
                                    deviceFreeBlocks, deviceFreeBytes, 0, 0, mode, 0);
                            }
                            chunk += 1;
                            chunkOff += SAVE_CHUNK_SIZE;
                        }
                        mu_access_progress = (chunk + progressBase) + 0x13;
                        fire_screen_studio_event(SAVE_EVENT_PROGRESS, 0);
                        _mkproc_sleep_ticks = kThree;
                        vtbl = aproc->vtbl;
                        vtbl->sleep();
                        profile += 1;
                        progressBase += 7;
                    }
                }
            }

            if (result == 0 || mode != 3) {
                strs = (char*)stringBase0;
                tries = 2;
                result = 4;
                while (tries-- != 0 && result != 0) {
                    result = save_to_memcard2(device, 0, 0, 0, strs + 8, strs + 9, settings,
                                              STORAGE_LOAD_SIZE, freeBlocks, freeBytes, 0, flag,
                                              mode, 0);
                }
            }

            if (mode == 3) {
                mu_access_progress = 100;
                fire_screen_studio_event(SAVE_EVENT_PROGRESS, 0);
                _mkproc_sleep_ticks = kThree;
                vtbl = aproc->vtbl;
                vtbl->sleep();
            }

            mcard_msg_middle_sleep(mode, 0);
            end_save_message(mode, result, device, 0);
            if (mode != 4) {
                if (get_mode_of_play() == 7 && (mode == 2 || mode == 8)) {
                    cont = check_save_region_data_result(&result, device, mode);
                } else if (modeMinus1 <= 1 || modeMinus6 <= 1 || mode == 8) {
                    cont = check_save_profile_result(&result, device, 1);
                } else {
                    cont = check_save_profile_result(&result, device, flag);
                }
            } else {
                cont = 1;
            }
            mcard_msg_end();
        }

        if (get_mode_of_play() == 7 && (mode == 2 || mode == 8)) {
            if (result != 0) {
                if (bad_save_region_data_result_resolution(&result, device) != 0) {
                    return 0;
                }
                continue;
            }
            return 1;
        }
        if (result == 0) {
            return 1;
        }
        return 0;
    }
}
#pragma dont_inline reset

/* TODO: [near miss] 98.24%; loop base/index coloring remains; stop at coloring. */
void insert_mu(int device, int arg1, int arg2) {
    StorageDevice* base;
    int i;

    if (g_bMemCardScreensDisabled == 1) {
        return;
    }
    if (device < 0 || device >= 2) {
        reset_storage_device_status_structure(device);
        return;
    }
    base = DEVICE_AT(device);
    load_from_memcard_w_error(device, 2, &base->settings, base->name, STORAGE_NAME_LEN,
                              &base->freeBlocks, &base->freeBytes);
    if (device >= 0 && device < 2 && base->status == 0) {
        for (i = 0; i < STORAGE_MAX_SLOTS; i++) {
            if (base->profiles[i].present == 0) {
                strcpy(base->profiles[i].name, nbc_find_text(7, 1));
            }
        }
    }
}

void remove_mu(int device, int arg1, int arg2) {
    if (device < 0 || device >= STORAGE_MAX_DEVICES) {
        return;
    }
    reset_storage_device_status_structure(device);
    DEVICE_AT(device)->status = 1;
    strcpy(DEVICE_AT(device)->name, STR_EMPTY_NAME);
    DEVICE_AT(device)->freeBlocks = 0;
    DEVICE_AT(device)->freeBytes = 0;
}

int init_memcard(void) {
    int ok;
    int i;

    ok = init_gc_memcard();
    if (ok != 0) {
        reset_all_storage_devices_status_structure();
        for (i = 0; i < STORAGE_MAX_DEVICES; i++) {
            DEVICE_AT(i)->status = 1;
        }
    }
    return ok;
}

int get_mu_access_progress(void) {
    return mu_access_progress;
}
