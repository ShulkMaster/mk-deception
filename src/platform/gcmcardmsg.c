#include "platform/gcmcardmsg.h"

#include "game/memcard.h"
#include "game/mcardmsg.h"
#include "game/nbc.h"
#include "platform/gcmcard.h"
#include "platform/io.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
#include "runtime/sound.h"
#include "runtime/utils.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_vtbl.h"
#include "msl/mslcore.h"
#include "dolphin/gx.h"
#include "dolphin/vi.h"
#include "dolphin/os.h"

#pragma use_lmw_stmw on

extern int mcard_msg_active;
extern int mcard_hault_msg_active;
extern _mslSystem* msi;

static const char stringBase0[] =
#include "platform/gcmcardmsg_stringBase0.inc"
;

#define STR_MC_FMT_SPACE (&stringBase0[0x5EA8])
#define STR_MC_FMT_SSS (&stringBase0[0x5EE2])
#define STR_MC_FMT_SS (&stringBase0[0x5EEB])
#define STR_MC_FMT_BODY_SPACES (&stringBase0[0x5EF3])
#define STR_MC_FMT_SSSS (&stringBase0[0x5ED6])
#define STR_MC_FMT_SDSDS (&stringBase0[0x5F05])
#define STR_MC_FMT_SS_COMMA (&stringBase0[0x5F14])
#define STR_MC_FMT_SSSDSDS (&stringBase0[0x5EAA])
#define STR_MC_FMT_NO_SPACE_OPTS (&stringBase0[0x5EBF])
#define STR_MC_FMT_S (&stringBase0[0x5ED3])
#define STR_MC_FMT_KONQUEST_BODY (&stringBase0[0x5EF6])

static char message_buf_temp2[0x1e];
static char message_buf_temp1[0x1e];
static char message_buffer[0x1e];

int gc_mc_default_name[2] = {0x6a, 0x6b};

int msg_format_confirmation_answer;
int msg_format_failed_answer;
int mcard_msg_card_changed_at_format_answer;
int msg_no_file_answer;
int msg_card_gone_answer;
static int msg_card_gone_player;
int mcard_msg_wrong_device_answer;
int mcard_msg_mu_removed_answer;
int mcard_msg_no_cards_at_settings_answer;
int mcard_msg_no_cards_at_cap_answer;
int mcard_msg__no_cards_at_boot_answer;
int mcard_msg_incompatible_card_answer;
int msg_another_market_answer;
int msg_sys_corrupt_answer;
int msg_crc_failure_answer;
int mcard_msg_card_damaged_answer;
int mcard_msg_no_space_answer;
int mcard_msg_confirm_erase_answer;
int mcard_msg_name_conflict_answer;
int msg_save_cancelled_answer;
int msg_profile_reset_confirmation_answer;
int msg_load_no_card_konq_region_hault_answer;
static int msg_load_no_card_konq_region_hault_player;
int msg_cant_enter_konquest_answer;
int msg_save_no_card_konq_region_hault_answer;
static int msg_save_no_card_konq_region_hault_player;
int msg_quit_confirmation_answer;
int msg_save_error_konq_region_answer;

static int low_storage_slot_message_done;

static inline int pad_action_pressed(int action) {
    if (check_switch_action(get_p1_pad(), action) != 0) {
        return 1;
    }
    if (check_switch_action(get_p2_pad(), action) != 0) {
        return 1;
    }
    return 0;
}

static inline void eat_pad_action(int action) {
    eat_switch_action(get_p1_pad(), action);
    eat_switch_action(get_p2_pad(), action);
}

static inline void format_msg_accept(int* answerOut, int answer) {
    snd_req(0x1aa5);
    mcard_hault_msg_active = 0;
    pause_procs(0);
    mcard_msg_remove_screen();
    *answerOut = answer;
}

static inline void sleep_aproc(float ticks) {
    _mkproc_sleep_ticks = ticks;
    aproc->vtbl->sleep();
}

static inline int is_hault_message_id(int id) {
    if (id < 0x14) {
        if (id == 3) {
            return 0;
        }
        if (id > 3) {
            if (id >= 0xc) {
                return 1;
            }
            if (id >= 6) {
                return 0;
            }
            return 1;
        }
        if (id >= 2) {
            return 1;
        }
        if (id >= 0) {
            return 0;
        }
        return 0;
    }
    if (id < 0x2a) {
        if (id >= 0x25) {
            return 0;
        }
        if (id >= 0x16) {
            return 1;
        }
        return 0;
    }
    if (id >= 0x2f) {
        return 0;
    }
    if (id >= 0x2d) {
        return 0;
    }
    return 1;
}

/* TODO: [breakthrough] 66.09%; retail reboot calls restored; compare answer CFG and register lifetimes. */
int gc_no_space_routine(const char* nameOrNull, int device) {
    const char* name;
    const char* a;
    const char* b;
    const char* c;
    const char* slotName;
    const char* d;
    const char* optA;
    const char* optB;
    const char* optC;
    const char* optD;
    int ret;

    ret = 1;
    if (device < 0 || device >= 2) {

    } else {
        name = nameOrNull;
        if (name == 0) {
            name = STR_MC_FMT_SPACE;
        }
        init_memcard_msg_screen();
        set_memcard_popup_message_title_text(nbc_find_text(0x4b, 0));
        a = nbc_find_text(0x4f, 0);
        b = nbc_find_text(0x4e, 0);
        c = nbc_find_text(0x4d, 0);
        slotName = nbc_find_text(gc_mc_default_name[device], 0);
        d = nbc_find_text(0x4c, 0);
        sprintf(message_buffer, STR_MC_FMT_SSSDSDS, d, slotName, c, 1, b, 0x3a, a);
        set_memcard_popup_message_body_text(message_buffer);
        optA = nbc_find_text(0x51, 0);
        optB = nbc_find_text(0x50, 0);
        optC = nbc_find_text(0x13, 0);
        optD = nbc_find_text(0x12, 0);
        sprintf(message_buffer, STR_MC_FMT_NO_SPACE_OPTS, optD, name, optC, optB, optA);
        set_memcard_popup_message_options_text(message_buffer);
        set_memcard_popup_message_type(0xb);
        fire_up_memcard_mesage_screen();
        mcard_msg_no_space_answer = 0;
        mcard_msg_active = 0x12;
        prepare_for_haulting_message();
        sleep_aproc(1.0f);
    }

    if (mcard_msg_no_space_answer == 2) {
        ret = 1;
    } else if (mcard_msg_no_space_answer == 3) {
        mslStopAll(msi);
        GXDrawDone();
        VISetBlack(1);
        VIFlush();
        VIWaitForRetrace();
        OSResetSystem(1, 0, 1);
    } else {
        ret = 0;
    }

    mcard_msg_end();
    return ret;
}

/* TODO: [breakthrough needed] 78.44%; retail pool restored; remaining call/branch lowering needs comparison. */
void gc_boot_space_check(void) {
    int prevStatus[2];
    int device;
    int changed;
    int statusRc;
    int anyPresent;
    const char* slotName;
    const char* optA;
    const char* optB;
    const char* optC;
    const char* bodyPart;

    if (low_storage_slot_message_done != 0) {
        return;
    }

    for (;;) {
        for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
            prevStatus[device] = DEVICE_AT(device)->status;
        }
        statusRc = update_storage_status(0);
        changed = 0;
        for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
            if (DEVICE_AT(device)->status != prevStatus[device]) {
                changed = 1;
                break;
            }
        }
        if (changed != 0 || statusRc != 0) {
            continue;
        }

        anyPresent = 0;
        for (device = 0; device < STORAGE_MAX_DEVICES; device++) {
            if (DEVICE_AT(device)->status != STORAGE_STATUS_ABSENT) {
                anyPresent = 1;
            }
        }
        if (anyPresent != 0) {
            break;
        }

        slotName = nbc_find_text(0x70, 0);
        if (slotName == 0) {
            slotName = STR_MC_FMT_SPACE;
        }
        init_memcard_msg_screen();
        set_memcard_popup_message_title_text(nbc_find_text(0x32, 0));
        bodyPart = nbc_find_text(0x33, 0);
        sprintf(message_buffer, STR_MC_FMT_S, bodyPart);
        set_memcard_popup_message_body_text(message_buffer);
        optA = nbc_find_text(0x15, 0);
        optB = nbc_find_text(0x13, 0);
        optC = nbc_find_text(0x12, 0);
        sprintf(message_buffer, STR_MC_FMT_SSSS, optC, slotName, optB, optA);
        set_memcard_popup_message_options_text(message_buffer);
        set_memcard_popup_message_type(0xb);
        fire_up_memcard_mesage_screen();
        mcard_msg__no_cards_at_boot_answer = 0;
        mcard_msg_active = 0x19;
        prepare_for_haulting_message();
        sleep_aproc(1.0f);

        if (mcard_msg_active != 0) {
            if (is_hault_message_id(mcard_msg_active) == 0 && mcard_msg_active != 0) {
                sleep_aproc(120.0f);
            }
            mcard_msg_remove_screen();
            recover_from_message();
            f_writing_to_memcard = 0;
            mcard_msg_active = 0;
        }

        if (mcard_msg__no_cards_at_boot_answer == 2) {
            continue;
        }
        break;
    }

    low_storage_slot_message_done = 1;
}

int is_this_a_hault_message(void) {
    return is_hault_message_id(mcard_msg_active);
}

/* TODO: [breakthrough needed] 63.42%; shared halt classifier emits a different CFG; compare retail call order. */
void mcard_msg_end(void) {
    int active;

    active = mcard_msg_active;
    if (active == 0) {
        return;
    }
    if (is_hault_message_id(active) == 0 && active != 0) {
        _mkproc_sleep_ticks = 0.0f;
        aproc->vtbl->sleep();
    }
    mcard_msg_remove_screen();
    recover_from_message();
    f_writing_to_memcard = 0;
    mcard_msg_active = 0;
}

/* TODO: [breakthrough needed] 60.80645%; retail pool restored; remaining call/branch lowering needs comparison. */
void mcard_msg_middle_sleep(int mode, int caller) {
    if ((mode == 7 || mode == 8) && caller == 0) {
        return;
    }
    if (is_hault_message_id(mcard_msg_active) == 0 &&
        mcard_msg_active != 0 &&
        (mcard_msg_active == 0x25 || mcard_msg_active == 9)) {
        sleep_aproc(0.0f);
    }
}

/* TODO: [breakthrough needed] 73.65%; pad poll call schedule differs; compare retail call order. */
static void mcard_msg_card_change_at_format_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_card_changed_at_format_answer, 2);
    }
    if (pad_action_pressed(3) != 0) {
        eat_pad_action(3);
        format_msg_accept(&mcard_msg_card_changed_at_format_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_card_changed_at_format_answer, 2);
    }
}

void mcard_msg_card_changed_at_format(int device) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x8d, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x8c, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS, nbc_find_text(0x8e, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x8f, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x90, 0));
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x2c;
    mcard_msg_card_changed_at_format_answer = 0;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_card_inaccessable_in_konq_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        snd_req(0x1aa5);
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
    }
}

void mcard_msg_card_inaccessable_in_konq(void) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x8b, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x8c, 0));
    set_memcard_popup_message_options_text(nbc_find_text(0xf, 0));
    if (nbc_get_language() == 3) {
        set_memcard_popup_message_type(0xa);
    } else {
        set_memcard_popup_message_type(0xb);
    }
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x2b;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

void mcard_msg_auto_save(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x88, 0));
    sprintf(message_buffer, STR_MC_FMT_SS, nbc_find_text(0x27, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x28, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x25;
    prepare_for_sleeping_message();
    sleep_aproc(1.0f);
}

/* TODO: [near miss] 82.878784%; retail pool restored; compiler caches the repeated space pointer. */
void mcard_msg_save_failed(int device) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x83, 0));
    set_memcard_popup_message_body_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x27;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

/* TODO: [near miss] 82.878784%; retail pool restored; compiler caches the repeated space pointer. */
void mcard_msg_create_failed(int device) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x87, 0));
    set_memcard_popup_message_body_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(5);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x29;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

/* TODO: [near miss] 82.878784%; retail pool restored; compiler caches the repeated space pointer. */
void mcard_msg_create_successful(int device) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x86, 0));
    set_memcard_popup_message_body_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(2);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x28;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_save_successful(int device) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x82, 0));
    set_memcard_popup_message_body_text(STR_MC_FMT_BODY_SPACES);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(2);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x26;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

static void mcard_msg_confirm_erase_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_confirm_erase_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_confirm_erase_answer, 2);
    }
}

/* TODO: [breakthrough needed] 44.395603%; retail pool restored; remaining call/branch lowering needs comparison. */
void mcard_msg_confirm_erase(void) {
    int lang;

    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x89, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x8a, 0));
    set_memcard_popup_message_options_text(nbc_find_text(0x11, 0));
    lang = nbc_get_language();
    if (lang == 1) {
        set_memcard_popup_message_type(2);
    } else {
        set_memcard_popup_message_type(9);
    }
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x2a;
    mcard_msg_confirm_erase_answer = 0;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_load_no_card_konq_region_hault_rtn(void) {
    int pad;

    if (msg_load_no_card_konq_region_hault_player == 0) {
        pad = get_p1_pad();
        if (check_switch_action(pad, 0) != 0) {
            eat_switch_action(get_p1_pad(), 0);
            format_msg_accept(&msg_load_no_card_konq_region_hault_answer, 1);
        }
        pad = get_p1_pad();
        if (check_switch_action(pad, 1) != 0) {
            eat_switch_action(get_p1_pad(), 1);
            format_msg_accept(&msg_load_no_card_konq_region_hault_answer, 2);
        }
    } else if (msg_load_no_card_konq_region_hault_player == 1) {
        pad = get_p2_pad();
        if (check_switch_action(pad, 0) != 0) {
            eat_switch_action(get_p2_pad(), 0);
            format_msg_accept(&msg_load_no_card_konq_region_hault_answer, 1);
        }
        pad = get_p2_pad();
        if (check_switch_action(pad, 1) != 0) {
            eat_switch_action(get_p2_pad(), 1);
            format_msg_accept(&msg_load_no_card_konq_region_hault_answer, 2);
        }
    } else {
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
    }
}

static inline void build_konquest_no_card_body(void)
{
    sprintf(message_buffer, STR_MC_FMT_KONQUEST_BODY,
            nbc_find_text(5, 0), message_buf_temp1,
            nbc_find_text(6, 0), message_buf_temp2,
            nbc_find_text(7, 0), nbc_find_text(8, 0));
    set_memcard_popup_message_body_text(message_buffer);
}

/* TODO: [near miss] 99.95238%; body operations agree; five shared profile/body buffer offsets need BSS placement. */
void mcard_msg_load_no_card_konq_region_hault(
    const char* profileName, int player, int device) {
    if (profileName == 0) {
        profileName = STR_MC_FMT_SPACE;
    }
    if (player < 0 || player >= 2) {
        player = 0;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(4, 0));
    if (player == 1) {
        strcpy(message_buf_temp1, nbc_find_text(0x6d, 0));
    } else {
        strcpy(message_buf_temp1, nbc_find_text(0x6e, 0));
    }
    if (strlen(profileName) != 0) {
        strcpy(message_buf_temp2, profileName);
    } else {
        strcpy(message_buf_temp2, nbc_find_text(0x6f, 0));
    }
    build_konquest_no_card_body();
    set_memcard_popup_message_options_text(nbc_find_text(0x80, 0));
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x24;
    msg_load_no_card_konq_region_hault_player = player;
    msg_load_no_card_konq_region_hault_answer = 0;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_profile_damaged_in_konquest_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        snd_req(0x1aa5);
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
    }
}

void mcard_msg_profile_damaged_in_konquest(void) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x7b, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x7c, 0));
    set_memcard_popup_message_options_text(nbc_find_text(0x7d, 0));
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x23;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_profile_reset_confirmation_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_profile_reset_confirmation_answer, 2);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_profile_reset_confirmation_answer, 1);
    }
}

void mcard_msg_profile_reset_confirmation(void) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x79, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x7a, 0));
    set_memcard_popup_message_options_text(nbc_find_text(0x7e, 0));
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x22;
    msg_profile_reset_confirmation_answer = 0;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_cant_enter_konquest_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        format_msg_accept(&msg_cant_enter_konquest_answer, 2);
    }
    if (check_switch_action(get_p1_pad(), 1) != 0 ||
        check_switch_action(get_p2_pad(), 1) != 0) {
        eat_switch_action(get_p1_pad(), 1);
        eat_switch_action(get_p2_pad(), 1);
        format_msg_accept(&msg_cant_enter_konquest_answer, 1);
    }
}

void mcard_msg_cant_enter_konquest(int device, const char* profileName) {
    if (device < 0 || device >= 2) {
        return;
    }
    if (profileName == 0) {
        profileName = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x75, 0));
    sprintf(message_buffer, STR_MC_FMT_S, nbc_find_text(0x76, 0));
    if (strlen(profileName) != 0) {
        strcat(message_buffer, profileName);
    } else {
        strcat(message_buffer, nbc_find_text(0x81, 0));
    }
    strcat(message_buffer, nbc_find_text(0x77, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x78, 0));
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x21;
    msg_cant_enter_konquest_answer = 0;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_save_no_card_konq_region_hault_rtn(void) {
    int pad;

    if (msg_save_no_card_konq_region_hault_player == 0) {
        pad = get_p1_pad();
        if (check_switch_action(pad, 0) != 0) {
            eat_switch_action(get_p1_pad(), 0);
            format_msg_accept(&msg_save_no_card_konq_region_hault_answer, 1);
        }
        pad = get_p1_pad();
        if (check_switch_action(pad, 1) != 0) {
            eat_switch_action(get_p1_pad(), 1);
            format_msg_accept(&msg_save_no_card_konq_region_hault_answer, 2);
        }
    } else if (msg_save_no_card_konq_region_hault_player == 1) {
        pad = get_p2_pad();
        if (check_switch_action(pad, 0) != 0) {
            eat_switch_action(get_p2_pad(), 0);
            format_msg_accept(&msg_save_no_card_konq_region_hault_answer, 1);
        }
        pad = get_p2_pad();
        if (check_switch_action(pad, 1) != 0) {
            eat_switch_action(get_p2_pad(), 1);
            format_msg_accept(&msg_save_no_card_konq_region_hault_answer, 2);
        }
    } else {
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
    }
}

/* TODO: [near miss] 99.95238%; body operations agree; five shared profile/body buffer offsets need BSS placement. */
void mcard_msg_save_no_card_konq_region_hault(const char* profileName, int player) {

    if (profileName == 0) {
        profileName = STR_MC_FMT_SPACE;
    }
    if (player < 0 || player >= 2) {
        player = 0;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(4, 0));
    if (player == 1) {
        strcpy(message_buf_temp1, nbc_find_text(0x6d, 0));
    } else {
        strcpy(message_buf_temp1, nbc_find_text(0x6e, 0));
    }
    if (strlen(profileName) != 0) {
        strcpy(message_buf_temp2, profileName);
    } else {
        strcpy(message_buf_temp2, nbc_find_text(0x6f, 0));
    }
    build_konquest_no_card_body();
    set_memcard_popup_message_options_text(nbc_find_text(0x7f, 0));
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    msg_save_no_card_konq_region_hault_player = player;
    msg_save_no_card_konq_region_hault_answer = 0;
    mcard_msg_active = 0x20;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void msg_quit_confirmation_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        format_msg_accept(&msg_quit_confirmation_answer, 1);
    }
    if (check_switch_action(get_p1_pad(), 1) != 0 ||
        check_switch_action(get_p2_pad(), 1) != 0) {
        eat_switch_action(get_p1_pad(), 1);
        eat_switch_action(get_p2_pad(), 1);
        format_msg_accept(&msg_quit_confirmation_answer, 2);
    }
}

void mcard_msg_quit_confirmation(void) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x73, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x74, 0));
    set_memcard_popup_message_options_text(nbc_find_text(0x11, 0));
    set_memcard_popup_message_type(5);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x1f;
    msg_save_error_konq_region_answer = 0;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void msg_save_error_konq_region_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        format_msg_accept(&msg_save_error_konq_region_answer, 1);
    }
    if (check_switch_action(get_p1_pad(), 1) != 0 ||
        check_switch_action(get_p2_pad(), 1) != 0) {
        eat_switch_action(get_p1_pad(), 1);
        eat_switch_action(get_p2_pad(), 1);
        format_msg_accept(&msg_save_error_konq_region_answer, 2);
    }
}

void mcard_msg_save_error_konq_region(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x71, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x72, 0));
    set_memcard_popup_message_options_text(nbc_find_text(0x7f, 0));
    if (nbc_get_language() == 2) {
        set_memcard_popup_message_type(4);
    } else {
        set_memcard_popup_message_type(5);
    }
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x1e;
    msg_save_error_konq_region_answer = 0;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_name_conflict_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_name_conflict_answer, 1);
    }
}

/* TODO: [breakthrough needed] 77.35%; retail pool restored; remaining call/branch lowering needs comparison. */
void mcard_msg_name_conflict(void) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x68, 0));
    sprintf(message_buffer, STR_MC_FMT_S, nbc_find_text(0x69, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0xf, 0));
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    mcard_msg_name_conflict_answer = 0;
    mcard_msg_active = 0x1b;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
    if (is_hault_message_id(mcard_msg_active) == 0 && mcard_msg_active != 0) {
        sleep_aproc(0.0f);
    }
    mcard_msg_remove_screen();
    recover_from_message();
    f_writing_to_memcard = 0;
    mcard_msg_active = 0;
}

static void mcard_msg_save_cancelled_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_save_cancelled_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_save_cancelled_answer, 2);
    }
}

static void mcard_msg_no_room_for_profile_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        snd_req(0x1aa5);
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
    }
}

static void mcard_msg_debug_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        snd_req(0x1aa5);
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
    }
}

/* TODO: [breakthrough needed] 74.91%; pad poll call schedule differs; compare retail call order. */
static void mcard_msg_format_failed_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_format_failed_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_format_failed_answer, 2);
    }
}

static inline void format_failed_body(int device) {
    const char* suffix;
    const char* slot_name;
    const char* prefix;
    suffix = nbc_find_text(0x60, 0);
    slot_name = nbc_find_text(gc_mc_default_name[device], 0);
    prefix = nbc_find_text(0x5f, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, prefix, slot_name, suffix);
}

void mcard_msg_format_failed(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x5e, 0));
    format_failed_body(device);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x10, 0));
    set_memcard_popup_message_type(5);
    fire_up_memcard_mesage_screen();
    msg_format_failed_answer = 0;
    mcard_msg_active = 0x16;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static inline void set_memcard_slot_message_body(int device, int first_text,
                                                int last_text, const char* format) {
    const char* partA;
    const char* slotName;
    const char* partB;

    partA = nbc_find_text(first_text, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(last_text, 0);
    sprintf(message_buffer, format, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
}

void mcard_msg_format_successful(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x5b, 0));
    set_memcard_slot_message_body(device, 0x5d, 0x5c, STR_MC_FMT_SSS);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x15;
    prepare_for_sleeping_message();
    sleep_aproc(60.0f);
}

void mcard_msg_formating(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x58, 0));
    sprintf(message_buffer, STR_MC_FMT_SS, nbc_find_text(0x59, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x5a, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x14;
    prepare_for_sleeping_message();
    sleep_aproc(60.0f);
}

/* TODO: [breakthrough needed] 74.91%; pad poll call schedule differs; compare retail call order. */
static void mcard_msg_format_confirmation_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_format_confirmation_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_format_confirmation_answer, 2);
    }
}

void mcard_msg_format_confirmation(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x55, 0));
    sprintf(message_buffer, STR_MC_FMT_SS, nbc_find_text(0x56, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x57, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x11, 0));
    set_memcard_popup_message_type(8);
    fire_up_memcard_mesage_screen();
    msg_format_confirmation_answer = 0;
    mcard_msg_active = 0x13;
    prepare_for_haulting_message();
    _mkproc_sleep_ticks = 1.0f;
    aproc->vtbl->sleep();
}

/* TODO: [breakthrough needed] 73.34%; pad poll call schedule differs; compare retail call order. */
static void mcard_msg_no_file_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_no_file_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_no_file_answer, 2);
    }
}

static inline void no_file_body(int device) {
    const char* suffix;
    const char* slot_name;

    suffix = nbc_find_text(0xe, 0);
    slot_name = nbc_find_text(gc_mc_default_name[device], 0);
    sprintf(message_buffer, STR_MC_FMT_SS,
        nbc_find_text(0xd, 0), slot_name, suffix);
}

void mcard_msg_no_file(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0xc, 0));
    no_file_body(device);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x11, 0));
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    msg_no_file_answer = 0;
    mcard_msg_active = 4;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_card_gone_rtn(void) {
    if (msg_card_gone_player == 0) {
        if (check_switch_action(get_p1_pad(), 0) != 0) {
            eat_switch_action(get_p1_pad(), 0);
            format_msg_accept(&msg_card_gone_answer, 1);
        }
        if (check_switch_action(get_p1_pad(), 1) != 0) {
            eat_switch_action(get_p1_pad(), 1);
            format_msg_accept(&msg_card_gone_answer, 2);
        }
    } else if (msg_card_gone_player == 1) {
        if (check_switch_action(get_p2_pad(), 0) != 0) {
            eat_switch_action(get_p2_pad(), 0);
            format_msg_accept(&msg_card_gone_answer, 1);
        }
        if (check_switch_action(get_p2_pad(), 1) != 0) {
            eat_switch_action(get_p2_pad(), 1);
            format_msg_accept(&msg_card_gone_answer, 2);
        }
    } else {
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
    }
}

void mcard_msg_card_gone(const char* profileName, int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    if (profileName == 0) {
        profileName = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(4, 0));
    strcpy(message_buffer, nbc_find_text(5, 0));
    if (device == 1) {
        strcat(message_buffer, nbc_find_text(0x6d, 0));
    } else {
        strcat(message_buffer, nbc_find_text(0x6e, 0));
    }
    strcat(message_buffer, nbc_find_text(6, 0));
    if (strlen(profileName) != 0) {
        strcat(message_buffer, profileName);
    } else {
        strcat(message_buffer, nbc_find_text(0x6f, 0));
    }
    strcat(message_buffer, nbc_find_text(7, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x10, 0));
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    msg_card_gone_player = device;
    msg_card_gone_answer = 0;
    mcard_msg_active = 2;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

/* TODO: [breakthrough needed] 82.56%; pad and edge poll schedule differs; compare retail call order. */
static void mcard_msg_crc_failure_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_crc_failure_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_crc_failure_answer, 2);
    }
    if (check_switch_edge(0, 5) != 0 || check_switch_edge(1, 5) != 0) {
        eat_switch_edge(0, 5);
        eat_switch_edge(1, 5);
        format_msg_accept(&msg_crc_failure_answer, 3);
    }
}

void mcard_msg_crc_failure(const char* nameOrNull, int device) {

    if (device < 0 || device >= 2) {
        return;
    }
    if (nameOrNull == 0) {
        nameOrNull = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x17, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS, nbc_find_text(0x18, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x19, 0));
    set_memcard_popup_message_body_text(message_buffer);
    sprintf(message_buffer, STR_MC_FMT_SSSS, nbc_find_text(0x1a, 0),
        nameOrNull, nbc_find_text(0x1b, 0), nbc_find_text(0x1c, 0));
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    msg_crc_failure_answer = 0;
    mcard_msg_active = 5;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_incompatible_card_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_incompatible_card_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_incompatible_card_answer, 2);
    }
}

void mcard_msg_incompatible_card(const char* nameOrNull, int device) {

    if (device < 0 || device >= 2) {
        return;
    }
    if (nameOrNull == 0) {
        nameOrNull = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x52, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS, nbc_find_text(0x53, 0),
            nbc_find_text(gc_mc_default_name[device], 0),
            nbc_find_text(0x54, 0));
    set_memcard_popup_message_body_text(message_buffer);
    sprintf(message_buffer, STR_MC_FMT_SSSS, nbc_find_text(0x12, 0),
            nameOrNull, nbc_find_text(0x13, 0), nbc_find_text(0x15, 0));
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_incompatible_card_answer = 0;
    mcard_msg_active = 0x11;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

/* TODO: [breakthrough needed] 74.04%; pad and edge poll schedule differs; compare retail call order. */
static void mcard_msg_no_space_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_no_space_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_no_space_answer, 2);
    }
    if (check_switch_edge(0, 5) != 0 || check_switch_edge(1, 5) != 0) {
        eat_switch_edge(0, 5);
        eat_switch_edge(1, 5);
        format_msg_accept(&mcard_msg_no_space_answer, 3);
    }
}

static void mcard_msg_wrong_device_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_wrong_device_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_wrong_device_answer, 2);
    }
}

void mcard_msg_wrong_device(const char* name, int device) {

    if (device < 0 || device >= 2) {
        return;
    }
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x48, 0));
    sprintf(message_buffer, STR_MC_FMT_SS,
            nbc_find_text(0x49, 0),
            nbc_find_text(gc_mc_default_name[device], 0),
            nbc_find_text(0x4a, 0));
    set_memcard_popup_message_body_text(message_buffer);
    sprintf(message_buffer, STR_MC_FMT_SSSS,
            nbc_find_text(0x12, 0), name,
            nbc_find_text(0x13, 0), nbc_find_text(0x15, 0));
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_wrong_device_answer = 0;
    mcard_msg_active = 0x10;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_card_damaged_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_card_damaged_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_card_damaged_answer, 2);
    }
}

void mcard_msg_card_damaged(const char* nameOrNull, int device) {

    if (device < 0 || device >= 2) {
        return;
    }
    if (nameOrNull == 0) {
        nameOrNull = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x45, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS, nbc_find_text(0x46, 0),
            nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x47, 0));
    set_memcard_popup_message_body_text(message_buffer);
    sprintf(message_buffer, STR_MC_FMT_SSSS, nbc_find_text(0x12, 0), nameOrNull,
            nbc_find_text(0x13, 0), nbc_find_text(0x15, 0));
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(5);
    fire_up_memcard_mesage_screen();
    mcard_msg_card_damaged_answer = 0;
    mcard_msg_active = 0xf;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_another_market_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_another_market_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_another_market_answer, 2);
    }
    if (pad_action_pressed(3) != 0) {
        eat_pad_action(3);
        format_msg_accept(&msg_another_market_answer, 3);
    }
}

void mcard_msg_another_market(const char* name, int device) {

    if (device < 0 || device >= 2) {
        return;
    }
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x42, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS,
        nbc_find_text(0x43, 0),
        nbc_find_text(gc_mc_default_name[device], 0),
        nbc_find_text(0x44, 0));
    set_memcard_popup_message_body_text(message_buffer);
    sprintf(message_buffer, STR_MC_FMT_SSSS,
        nbc_find_text(0x12, 0), name,
        nbc_find_text(0x13, 0), nbc_find_text(0x14, 0));
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    msg_another_market_answer = 0;
    mcard_msg_active = 0xe;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_sys_corrupt_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_sys_corrupt_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_sys_corrupt_answer, 2);
    }
    if (pad_action_pressed(3) != 0) {
        eat_pad_action(3);
        format_msg_accept(&msg_sys_corrupt_answer, 3);
    }
}

void mcard_msg_sys_corrupt(const char* nameOrNull, int device) {

    if (device < 0 || device >= 2) {
        return;
    }
    if (nameOrNull == 0) {
        nameOrNull = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x3c, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS, nbc_find_text(0x3d, 0),
            nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x3e, 0));
    set_memcard_popup_message_body_text(message_buffer);
    sprintf(message_buffer, STR_MC_FMT_SSSS, nbc_find_text(0x3f, 0), nameOrNull,
            nbc_find_text(0x40, 0), nbc_find_text(0x41, 0));
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    msg_crc_failure_answer = 0;
    mcard_msg_active = 0xd;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_no_cards_at_cap_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        format_msg_accept(&mcard_msg_no_cards_at_cap_answer, 1);
    }
}

static void mcard_msg_no_cards_at_settings_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_no_cards_at_settings_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_no_cards_at_settings_answer, 2);
    }
}

/* TODO: [near miss] 95.54054%; retail pool restored; residual call/reload lowering. */
void mcard_msg_no_cards_at_settings(void) {
    const char* a;
    const char* b;
    const char* c;
    const char* optA;
    const char* optB;

    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x38, 0));
    a = nbc_find_text(0x3b, 0);
    b = nbc_find_text(0x3a, 0);
    c = nbc_find_text(0x39, 0);
    sprintf(message_buffer, STR_MC_FMT_SDSDS, c, 1, b, 0x3a, a);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x15, 0);
    optB = nbc_find_text(0x16, 0);
    sprintf(message_buffer, STR_MC_FMT_SS_COMMA, optB, optA);
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    mcard_msg_no_cards_at_settings_answer = 0;
    mcard_msg_active = 0x1d;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_no_cards_at_boot_rtn(void) {
    if (check_switch_action(get_p1_pad(), 0) != 0 ||
        check_switch_action(get_p2_pad(), 0) != 0) {
        eat_switch_action(get_p1_pad(), 0);
        eat_switch_action(get_p2_pad(), 0);
        snd_req(0x1aa5);
        mcard_hault_msg_active = 0;
        pause_procs(0);
        mcard_msg_remove_screen();
        mcard_msg__no_cards_at_boot_answer = 1;
    }
    if (check_switch_action(get_p1_pad(), 1) != 0 ||
        check_switch_action(get_p2_pad(), 1) != 0) {
        eat_switch_action(get_p1_pad(), 1);
        eat_switch_action(get_p2_pad(), 1);
        mcard_hault_msg_active = 0;
        snd_req(0x1aa5);
        pause_procs(0);
        mcard_msg_remove_screen();
        mcard_msg__no_cards_at_boot_answer = 2;
    }
}

static void mcard_msg_mu_removed_rtn(void) {
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&mcard_msg_mu_removed_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&mcard_msg_mu_removed_answer, 2);
    }
}

void mcard_msg_mu_removed(const char* nameOrNull, int device) {

    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x2f, 0));
    sprintf(
        message_buffer, STR_MC_FMT_SS, nbc_find_text(0x30, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x31, 0));
    set_memcard_popup_message_body_text(message_buffer);
    sprintf(
        message_buffer, STR_MC_FMT_SSSS, nbc_find_text(0x12, 0), nameOrNull,
        nbc_find_text(0x13, 0), nbc_find_text(0x15, 0));
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_mu_removed_answer = 0;
    mcard_msg_active = 0xc;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

/* TODO: [near miss] 82.878784%; retail pool restored; compiler caches the repeated space pointer. */
void mcard_msg_delete_failed_generic(void) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x23, 0));
    set_memcard_popup_message_body_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x2e;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

/* TODO: [near miss] 82.878784%; retail pool restored; compiler caches the repeated space pointer. */
void mcard_msg_delete_successful_generic(void) {
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x20, 0));
    set_memcard_popup_message_body_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x2d;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_delete_failed(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x23, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS,
            nbc_find_text(0x24, 0),
            nbc_find_text(gc_mc_default_name[device], 0),
            nbc_find_text(0x25, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 8;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_delete_successful(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x20, 0));
    sprintf(message_buffer, STR_MC_FMT_SSS, nbc_find_text(0x21, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x22, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 7;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

static inline void format_deleting_body(int device) {
    const char* suffix;
    const char* slot_name;
    const char* prefix;
    suffix = nbc_find_text(0x1f, 0);
    slot_name = nbc_find_text(gc_mc_default_name[device], 0);
    prefix = nbc_find_text(0x1e, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, prefix, slot_name, suffix);
}

void mcard_msg_deleting_file(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x1d, 0));
    format_deleting_body(device);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(8);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 6;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_no_storage(const char* text) {
    if (text == 0) {
        text = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(9, 0));
    sprintf(message_buffer, STR_MC_FMT_SS,
            nbc_find_text(10, 0), text, nbc_find_text(0xb, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 3;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_read(int device) {
}

void mcard_msg_deleting_data(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x2c, 0));
    sprintf(message_buffer, STR_MC_FMT_SS, nbc_find_text(0x2d, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x2e, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0xb;
    prepare_for_sleeping_message();
    sleep_aproc(30.0f);
}

void mcard_msg_create(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x29, 0));
    set_memcard_slot_message_body(device, 0x2b, 0x2a, STR_MC_FMT_SS);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(6);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 10;
    prepare_for_sleeping_message();
    sleep_aproc(30.0f);
}

void mcard_msg_save(int device) {
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x26, 0));
    sprintf(message_buffer, STR_MC_FMT_SS, nbc_find_text(0x27, 0),
        nbc_find_text(gc_mc_default_name[device], 0), nbc_find_text(0x28, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 9;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void (*msg_routine_table[])(void) = {
    mcmsg_nothing,
    mcmsg_nothing,
    mcard_msg_card_gone_rtn,
    mcmsg_nothing,
    mcard_msg_no_file_rtn,
    mcard_msg_crc_failure_rtn,
    mcmsg_nothing,
    mcmsg_nothing,
    mcmsg_nothing,
    mcmsg_nothing,
    mcmsg_nothing,
    mcmsg_nothing,
    mcard_msg_mu_removed_rtn,
    mcard_msg_sys_corrupt_rtn,
    mcard_msg_another_market_rtn,
    mcard_msg_card_damaged_rtn,
    mcard_msg_wrong_device_rtn,
    mcard_msg_incompatible_card_rtn,
    mcard_msg_no_space_rtn,
    mcard_msg_format_confirmation_rtn,
    mcmsg_nothing,
    mcmsg_nothing,
    mcard_msg_format_failed_rtn,
    mcard_msg_no_room_for_profile_rtn,
    mcard_msg_debug_rtn,
    mcard_msg_no_cards_at_boot_rtn,
    mcard_msg_save_cancelled_rtn,
    mcard_msg_name_conflict_rtn,
    mcard_msg_no_cards_at_cap_rtn,
    mcard_msg_no_cards_at_settings_rtn,
    msg_save_error_konq_region_rtn,
    msg_quit_confirmation_rtn,
    mcard_msg_save_no_card_konq_region_hault_rtn,
    mcard_msg_cant_enter_konquest_rtn,
    mcard_msg_profile_reset_confirmation_rtn,
    mcard_msg_profile_damaged_in_konquest_rtn,
    mcard_msg_load_no_card_konq_region_hault_rtn,
    mcmsg_nothing,
    mcmsg_nothing,
    mcmsg_nothing,
    mcmsg_nothing,
    mcmsg_nothing,
    mcard_msg_confirm_erase_rtn,
    mcard_msg_card_inaccessable_in_konq_rtn,
    mcard_msg_card_change_at_format_rtn,
    mcmsg_nothing,
    mcmsg_nothing,
};

const char* gc_mc_msg_text[] = {
#include "platform/gcmcardmsg_text.inc"
};
