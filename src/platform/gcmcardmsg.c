#include "platform/gcmcardmsg.h"

#include "game/memcard.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_vtbl.h"

/*
 * gcmcardmsg.o - Midway GC memcard popup/message bodies (B20 + B21 PPWLS).
 * NonMatching: ASM still linked for DOL. Function order = retail emission.
 * Campaign history: docs/campaigns/index.md (B20-B22)
 */

#if !defined(TARGET_PC)
#pragma use_lmw_stmw on
#endif

void mcard_msg_remove_screen(void);
void recover_from_message(void);
void init_memcard_msg_screen(void);
void set_memcard_popup_message_title_text(const char* text);
void set_memcard_popup_message_body_text(const char* text);
void set_memcard_popup_message_options_text(const char* text);
void set_memcard_popup_message_type(int type);
void fire_up_memcard_mesage_screen(void);
void prepare_for_haulting_message(void);
void prepare_for_sleeping_message(void);
const char* nbc_find_text(int index, int table);
int sprintf(char* dest, const char* fmt, ...);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
unsigned long strlen(const char* s);
void* get_p1_pad(void);
void* get_p2_pad(void);
int check_switch_action(void* pad, int action);
void eat_switch_action(void* pad, int action);
int check_switch_edge(int pad, int action);
void eat_switch_edge(int pad, int action);
void snd_req(int id);
void pause_procs(int pause);
int update_storage_status(int flag);
int nbc_get_language(void);

extern int mcard_msg_active;
extern int mcard_hault_msg_active;
extern int f_writing_to_memcard;

/* Retail stringBase0 slices used by format popups (full pool stays in ASM). */
static const char STR_MC_FMT_SPACE[] = " ";
static const char STR_MC_FMT_SSS[] = "%s %s %s";
static const char STR_MC_FMT_SS[] = "%s %s%s";
static const char STR_MC_FMT_BODY_SPACES[] = "  ";
static const char STR_MC_FMT_SSSS[] = "%s %s %s %s";
static const char STR_MC_FMT_SDSDS[] = "%s %d %s %d %s";
static const char STR_MC_FMT_SS_COMMA[] = "%s, %s";
static const char STR_MC_FMT_SSSDSDS[] = "%s %s %s %d %s %d %s";
static const char STR_MC_FMT_NO_SPACE_OPTS[] = "%s %s %s   %s\n%s   ";

/* Retail's three contiguous 0x1E popup scratch buffers. */
static char message_buf_temp2[0x1e];
static char message_buf_temp1[0x1e];
static char message_buffer[0x1e];

/* .sdata: NBC text ids Slot A / Slot B (shared with gcmcard get_device_reference_name). */
int gc_mc_default_name[2] = {0x6a, 0x6b};

/* .sbss answers for format flow (retail). */
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

/* Once-flag: PPWLS boot no-card / space popup already shown. */
static int low_storage_slot_message_done;

/* Soft ceiling: format popup helpers -- pad/answer emit vs inlined rtn bodies. */
static int pad_action_pressed(int action) {
    if (check_switch_action(get_p1_pad(), action) != 0) {
        return 1;
    }
    if (check_switch_action(get_p2_pad(), action) != 0) {
        return 1;
    }
    return 0;
}

static void eat_pad_action(int action) {
    eat_switch_action(get_p1_pad(), action);
    eat_switch_action(get_p2_pad(), action);
}

static void format_msg_accept(int* answerOut, int answer) {
    snd_req(0x1aa5);
    mcard_hault_msg_active = 0;
    pause_procs(0);
    mcard_msg_remove_screen();
    *answerOut = answer;
}

static void sleep_aproc(float ticks) {
    _mkproc_sleep_ticks = ticks;
    aproc->vtbl->sleep();
}


/*
 * Halt-message id classifier shared by is_this_a_hault_message / mcard_msg_end.
 * Soft ceiling: range emit vs compact form; algo matches retail decision tree.
 */
static int is_hault_message_id(int id) {
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

/*
 * Soft ceiling: gc_no_space_routine -- no-space popup + answer; OSResetSystem
 * reboot path left soft (host must not hard-reset). Algo OK for menu path.
 */
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
        /* fall through to cleanup */
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
    } else if (mcard_msg_no_space_answer < 2 || mcard_msg_no_space_answer > 3) {
        ret = 0;
    } else {
        /* Soft: retail OSResetSystem(1,0,1) reboot; skip on host. */
        ret = 0;
    }

    mcard_msg_end();
    return ret;
}

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

    /* Soft ceiling: popup string-pool / halt-classifier emit; algo OK for PPWLS. */
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
        sprintf(message_buffer, "%s", bodyPart);
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

void mcard_msg_end(void) {
    int active;

    active = mcard_msg_active;
    if (active == 0) {
        return;
    }
    /* Soft ceiling: ~emit vs compact classifier; algo matches retail. */
    if (is_hault_message_id(active) == 0 && active != 0) {
        _mkproc_sleep_ticks = 0.0f;
        aproc->vtbl->sleep();
    }
    mcard_msg_remove_screen();
    recover_from_message();
    f_writing_to_memcard = 0;
    mcard_msg_active = 0;
}

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

static void mcard_msg_card_change_at_format_rtn(void) {
    /* Soft ceiling: pad poll schedule; algo OK. */
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
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; no device-range check (retail). */
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x8d, 0));
    set_memcard_popup_message_body_text(nbc_find_text(0x8c, 0));
    partA = nbc_find_text(0x8f, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x8e, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
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
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x88, 0));
    partA = nbc_find_text(0x28, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x27, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x25;
    prepare_for_sleeping_message();
    sleep_aproc(1.0f);
}

void mcard_msg_save_failed(int device) {
    /* Soft ceiling: string-pool / sleep emit; device unused (retail). */
    (void)device;
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

void mcard_msg_create_failed(int device) {
    (void)device;
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

void mcard_msg_create_successful(int device) {
    (void)device;
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
    (void)device;
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

void mcard_msg_confirm_erase(void) {
    int lang;

    /* Soft ceiling: language type + sleep emit; algo OK for delete confirm. */
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
    void* pad;

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

void mcard_msg_load_no_card_konq_region_hault(const char* profileName, int unused, int device) {
    const char* part5;
    const char* part6;
    const char* part7;
    const char* part8;

    (void)device;
    if (profileName == 0) {
        profileName = "";
    }
    if (unused < 0 || unused >= 2) {
        unused = 0;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(4, 0));
    if (unused == 1) {
        strcpy(message_buf_temp1, nbc_find_text(0x6d, 0));
    } else {
        strcpy(message_buf_temp1, nbc_find_text(0x6e, 0));
    }
    if (strlen(profileName) != 0) {
        strcpy(message_buf_temp2, profileName);
    } else {
        strcpy(message_buf_temp2, nbc_find_text(0x6f, 0));
    }
    part8 = nbc_find_text(8, 0);
    part7 = nbc_find_text(7, 0);
    part6 = nbc_find_text(6, 0);
    part5 = nbc_find_text(5, 0);
    sprintf(message_buffer, "%s %s%s %s%s %s", part5, message_buf_temp1,
            part6, message_buf_temp2, part7, part8);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x80, 0));
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x24;
    msg_load_no_card_konq_region_hault_player = unused;
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
        profileName = "";
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x75, 0));
    sprintf(message_buffer, "%s", nbc_find_text(0x76, 0));
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
    void* pad;

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

void mcard_msg_save_no_card_konq_region_hault(const char* profileName, int unused) {
    const char* part5;
    const char* part6;
    const char* part7;
    const char* part8;

    if (profileName == 0) {
        profileName = "";
    }
    if (unused < 0 || unused >= 2) {
        unused = 0;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(4, 0));
    if (unused == 1) {
        strcpy(message_buf_temp1, nbc_find_text(0x6d, 0));
    } else {
        strcpy(message_buf_temp1, nbc_find_text(0x6e, 0));
    }
    if (strlen(profileName) != 0) {
        strcpy(message_buf_temp2, profileName);
    } else {
        strcpy(message_buf_temp2, nbc_find_text(0x6f, 0));
    }
    part8 = nbc_find_text(8, 0);
    part7 = nbc_find_text(7, 0);
    part6 = nbc_find_text(6, 0);
    part5 = nbc_find_text(5, 0);
    sprintf(message_buffer, "%s %s%s %s%s %s", part5, message_buf_temp1,
            part6, message_buf_temp2, part7, part8);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x7f, 0));
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    msg_save_no_card_konq_region_hault_player = unused;
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

void mcard_msg_name_conflict(void) {
    /* Soft ceiling: string-pool / halt-classifier emit; algo OK for create. */
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x68, 0));
    sprintf(message_buffer, "%s", nbc_find_text(0x69, 0));
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

static void mcard_msg_format_failed_rtn(void) {
    /* Soft ceiling: pad poll schedule; algo OK. */
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_format_failed_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_format_failed_answer, 2);
    }
}

void mcard_msg_format_failed(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x5e, 0));
    partA = nbc_find_text(0x60, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x5f, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x10, 0));
    set_memcard_popup_message_type(5);
    fire_up_memcard_mesage_screen();
    msg_format_failed_answer = 0;
    mcard_msg_active = 0x16;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

void mcard_msg_format_successful(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x5b, 0));
    partA = nbc_find_text(0x5d, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x5c, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x15;
    prepare_for_sleeping_message();
    sleep_aproc(60.0f);
}

void mcard_msg_formating(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x58, 0));
    partA = nbc_find_text(0x5a, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x59, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0x14;
    prepare_for_sleeping_message();
    sleep_aproc(60.0f);
}

static void mcard_msg_format_confirmation_rtn(void) {
    /* Soft ceiling: pad poll schedule; algo OK. */
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
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: ~92% -- stringBase0 / sleep emit; stop. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x55, 0));
    partA = nbc_find_text(0x57, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x56, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x11, 0));
    set_memcard_popup_message_type(8);
    fire_up_memcard_mesage_screen();
    msg_format_confirmation_answer = 0;
    mcard_msg_active = 0x13;
    prepare_for_haulting_message();
    {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
}

static void mcard_msg_no_file_rtn(void) {
    /* Soft ceiling: pad poll schedule; algo OK. */
    if (pad_action_pressed(0) != 0) {
        eat_pad_action(0);
        format_msg_accept(&msg_no_file_answer, 1);
    }
    if (pad_action_pressed(1) != 0) {
        eat_pad_action(1);
        format_msg_accept(&msg_no_file_answer, 2);
    }
}

void mcard_msg_no_file(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0xc, 0));
    partA = nbc_find_text(0xe, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0xd, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
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
    /* Soft ceiling: player-split pad poll; algo OK. */
    if (msg_card_gone_player == 0) {
        if (check_switch_action(get_p1_pad(), 0) != 0) {
            eat_switch_action(get_p1_pad(), 0);
            format_msg_accept(&msg_card_gone_answer, 1);
        }
        if (check_switch_action(get_p1_pad(), 1) != 0) {
            eat_switch_action(get_p1_pad(), 1);
            format_msg_accept(&msg_card_gone_answer, 2);
        }
    } else {
        if (check_switch_action(get_p2_pad(), 0) != 0) {
            eat_switch_action(get_p2_pad(), 0);
            format_msg_accept(&msg_card_gone_answer, 1);
        }
        if (check_switch_action(get_p2_pad(), 1) != 0) {
            eat_switch_action(get_p2_pad(), 1);
            format_msg_accept(&msg_card_gone_answer, 2);
        }
    }
}

void mcard_msg_card_gone(const char* profileName, int device) {
    const char* name;

    /* Soft ceiling: string-pool / strcat emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    name = profileName;
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
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
    if (strlen(name) == 0) {
        strcat(message_buffer, nbc_find_text(0x6f, 0));
    } else {
        strcat(message_buffer, name);
    }
    strcat(message_buffer, nbc_find_text(7, 0));
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(nbc_find_text(0x10, 0));
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    msg_card_gone_answer = 0;
    mcard_msg_active = 2;
    msg_card_gone_player = device;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_crc_failure_rtn(void) {
    /* Soft ceiling: pad/edge poll schedule; algo OK. */
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
    const char* name;
    const char* partA;
    const char* slotName;
    const char* partB;
    const char* optA;
    const char* optB;
    const char* optC;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    name = nameOrNull;
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x17, 0));
    partA = nbc_find_text(0x19, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x18, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x1c, 0);
    optB = nbc_find_text(0x1b, 0);
    optC = nbc_find_text(0x1a, 0);
    sprintf(message_buffer, STR_MC_FMT_SSSS, optC, name, optB, optA);
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
    const char* name;
    const char* partA;
    const char* slotName;
    const char* partB;
    const char* optA;
    const char* optB;
    const char* optC;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    name = nameOrNull;
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x52, 0));
    partA = nbc_find_text(0x54, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x53, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x15, 0);
    optB = nbc_find_text(0x13, 0);
    optC = nbc_find_text(0x12, 0);
    sprintf(message_buffer, STR_MC_FMT_SSSS, optC, name, optB, optA);
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_incompatible_card_answer = 0;
    mcard_msg_active = 0x11;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

static void mcard_msg_no_space_rtn(void) {
    /* Soft ceiling: pad/edge poll; algo OK. */
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

void mcard_msg_wrong_device(const char* nameOrNull, int device) {
    const char* name;
    const char* partA;
    const char* slotName;
    const char* partB;
    const char* optA;
    const char* optB;
    const char* optC;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    name = nameOrNull;
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x48, 0));
    partA = nbc_find_text(0x4a, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x49, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x15, 0);
    optB = nbc_find_text(0x13, 0);
    optC = nbc_find_text(0x12, 0);
    sprintf(message_buffer, STR_MC_FMT_SSSS, optC, name, optB, optA);
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
    const char* name;
    const char* partA;
    const char* slotName;
    const char* partB;
    const char* optA;
    const char* optB;
    const char* optC;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    name = nameOrNull;
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x45, 0));
    partA = nbc_find_text(0x47, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x46, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x15, 0);
    optB = nbc_find_text(0x13, 0);
    optC = nbc_find_text(0x12, 0);
    sprintf(message_buffer, STR_MC_FMT_SSSS, optC, name, optB, optA);
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

void mcard_msg_another_market(const char* nameOrNull, int device) {
    const char* name;
    const char* partA;
    const char* slotName;
    const char* partB;
    const char* optA;
    const char* optB;
    const char* optC;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    name = nameOrNull;
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x42, 0));
    partA = nbc_find_text(0x44, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x43, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x14, 0);
    optB = nbc_find_text(0x13, 0);
    optC = nbc_find_text(0x12, 0);
    sprintf(message_buffer, STR_MC_FMT_SSSS, optC, name, optB, optA);
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
    const char* name;
    const char* partA;
    const char* slotName;
    const char* partB;
    const char* optA;
    const char* optB;
    const char* optC;

    /* Soft ceiling: string-pool / sleep emit; algo OK.
     * Retail body also zeros msg_crc_failure_answer (shared sda quirk). */
    if (device < 0 || device >= 2) {
        return;
    }
    name = nameOrNull;
    if (name == 0) {
        name = STR_MC_FMT_SPACE;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x3c, 0));
    partA = nbc_find_text(0x3e, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x3d, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x41, 0);
    optB = nbc_find_text(0x40, 0);
    optC = nbc_find_text(0x3f, 0);
    sprintf(message_buffer, STR_MC_FMT_SSSS, optC, name, optB, optA);
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xb);
    fire_up_memcard_mesage_screen();
    msg_crc_failure_answer = 0;
    msg_sys_corrupt_answer = 0;
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

void mcard_msg_no_cards_at_settings(void) {
    const char* a;
    const char* b;
    const char* c;
    const char* optA;
    const char* optB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
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
    const char* partA;
    const char* slotName;
    const char* partB;
    const char* optA;
    const char* optB;
    const char* optC;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x2f, 0));
    partA = nbc_find_text(0x31, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x30, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    optA = nbc_find_text(0x15, 0);
    optB = nbc_find_text(0x13, 0);
    optC = nbc_find_text(0x12, 0);
    sprintf(message_buffer, STR_MC_FMT_SSSS, optC, nameOrNull, optB, optA);
    set_memcard_popup_message_options_text(message_buffer);
    set_memcard_popup_message_type(0xc);
    fire_up_memcard_mesage_screen();
    mcard_msg_mu_removed_answer = 0;
    mcard_msg_active = 0xc;
    prepare_for_haulting_message();
    sleep_aproc(1.0f);
}

void mcard_msg_delete_failed_generic(int device) {
    (void)device;
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

void mcard_msg_delete_successful_generic(int device) {
    (void)device;
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
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x23, 0));
    partA = nbc_find_text(0x25, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x24, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 8;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_delete_successful(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x20, 0));
    partA = nbc_find_text(0x22, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x21, 0);
    sprintf(message_buffer, STR_MC_FMT_SSS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 7;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_deleting_file(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x1d, 0));
    partA = nbc_find_text(0x1f, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x1e, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
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
    (void)device;
}

void mcard_msg_deleting_data(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x2c, 0));
    partA = nbc_find_text(0x2e, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x2d, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 0xb;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}

void mcard_msg_create(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x29, 0));
    partA = nbc_find_text(0x2b, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x2a, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(6);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 10;
    prepare_for_sleeping_message();
    sleep_aproc(30.0f);
}

void mcard_msg_save(int device) {
    const char* partA;
    const char* slotName;
    const char* partB;

    /* Soft ceiling: string-pool / sleep emit; algo OK. */
    if (device < 0 || device >= 2) {
        return;
    }
    init_memcard_msg_screen();
    set_memcard_popup_message_title_text(nbc_find_text(0x26, 0));
    partA = nbc_find_text(0x28, 0);
    slotName = nbc_find_text(gc_mc_default_name[device], 0);
    partB = nbc_find_text(0x27, 0);
    sprintf(message_buffer, STR_MC_FMT_SS, partB, slotName, partA);
    set_memcard_popup_message_body_text(message_buffer);
    set_memcard_popup_message_options_text(STR_MC_FMT_SPACE);
    set_memcard_popup_message_type(9);
    fire_up_memcard_mesage_screen();
    mcard_msg_active = 9;
    prepare_for_sleeping_message();
    sleep_aproc(90.0f);
}
