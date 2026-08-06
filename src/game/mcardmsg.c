#include "game/mcardmsg.h"

#include "game/game_info.h"
#include "runtime/mk_proc.h"

/* Contiguous retail string pool - keep layout; access via named offsets. */
static const char stringBase0[] =
    "uninitalized\0"
    "konquest/popups/k_generic_popup\0"
    "common/c_generic_popup\0"
    "Current scheme is: %d\n\0"
    "pause_menu/pause_generic_popup";

#define STR_UNINITIALIZED (&stringBase0[0])
#define STR_KONQUEST_POPUP (&stringBase0[0xD])
#define STR_COMMON_POPUP (&stringBase0[0x2D])
#define STR_SCHEME_DEBUG_FMT (&stringBase0[0x44])
#define STR_PAUSE_POPUP (&stringBase0[0x5B])

#define GAME_STATE_MCARD_HAULT 0x1A
#define MCARD_MSG_ROUTINE_MAX 0x2E
#define POPUP_TYPE_DEFAULT 2
#define POPUP_TYPE_COUNT 0xD

typedef struct MkProcMessageFlags {
    unsigned char pad0 : 4;
    unsigned char sleeping : 1;
    unsigned char pad1 : 3;
} MkProcMessageFlags;

#if !defined(TARGET_PC)
__declspec(section ".rodata") const int gap_04_8031382C_rodata = 0;
#else
const int gap_04_8031382C_rodata = 0;
#endif

int memcard_online_save_port = -1;
#if !defined(TARGET_PC)
__declspec(section ".sdata") unsigned char gap_07_8050FC3C_sdata[4] = {0, 0, 0, 0};
#else
unsigned char gap_07_8050FC3C_sdata[4] = {0, 0, 0, 0};
#endif

/* MWCC emits .sbss in reverse declaration order. */
int gap_08_80510DFC_sbss;
int f_writing_to_memcard;
int mcard_hault_msg_active;
int mcard_msg_active;

extern void (*msg_routine_table[])(void);

void set_popup_type(int type);
void set_popup_options_text(const char* text);
void set_popup_message_text(const char* text);
void set_popup_title_text(const char* text);
void* get_pause_menu_ssh(void);
int get_current_section_memory_scheme(void);
void load_screen(const char* screen, void* ssh, int arg2, int arg3);
void vdebug_print_message(const char* fmt, ...);
void fire_screen_studio_event(int event, int arg);
void screen_engine_process_events(void);
void turn_all_rumble_motors_off(void);
void pause_procs(int flag);
int get_controller_disabled_state(void);
void turn_controllers_on(void);
void reapply_controller_disabled_state(int state);
int is_this_a_hault_message(void);
int get_game_state(void);
void pop_game_state(int state);
void push_game_state(int state);
unsigned long strlen(const char* str);

int get_p2_pad(void) {
    if (memcard_online_save_port == -1) {
        return g_game_info.plyr1.pad_index;
    }
    return -1;
}

int get_p1_pad(void) {
    if (memcard_online_save_port != -1) {
        return memcard_online_save_port;
    }
    return g_game_info.plyr0.pad_index;
}

void set_memcard_popup_message_type(int type) {
    if (type < 0 || type >= POPUP_TYPE_COUNT) {
        type = POPUP_TYPE_DEFAULT;
    }
    set_popup_type(type);
}

void set_memcard_popup_message_options_text(const char* text) {
    if ((int)strlen(text) >= 0x80) {
        set_popup_options_text(STR_UNINITIALIZED);
    } else {
        set_popup_options_text(text);
    }
}

void set_memcard_popup_message_body_text(const char* text) {
    if (strlen(text) >= 0x200) {
        set_popup_message_text(STR_UNINITIALIZED);
    } else {
        set_popup_message_text(text);
    }
}

void set_memcard_popup_message_title_text(const char* text) {
    if (strlen(text) >= 0x100) {
        set_popup_title_text(STR_UNINITIALIZED);
    } else {
        set_popup_title_text(text);
    }
}

void fire_up_memcard_mesage_screen(void) {
    void* ssh;
    int scheme;

    ssh = get_pause_menu_ssh();
    scheme = get_current_section_memory_scheme();
    switch (scheme) {
    case 1:
        load_screen(STR_KONQUEST_POPUP, ssh, 0, 0);
        break;
    case 4:
    case 7:
    case 9:
    case 10:
        load_screen(STR_COMMON_POPUP, ssh, 0, 0);
        break;
    default:
        vdebug_print_message(STR_SCHEME_DEBUG_FMT, scheme);
    case 0:
    case 2:
    case 3:
    case 5:
    case 8:
        load_screen(STR_PAUSE_POPUP, ssh, 0, 0);
        break;
    }
}

#if !defined(TARGET_PC)
__declspec(section ".data") const int gap_05_8034F0EC_data = 0;
#else
const int gap_05_8034F0EC_data = 0;
#endif

void mcmsg_nothing(void) {}

void mcard_msg_remove_screen(void) {
    fire_screen_studio_event(0x1FAB, 0);
}

void init_memcard_msg_screen(void) {
    fire_screen_studio_event(0x1FAB, 0);
    screen_engine_process_events();
    turn_all_rumble_motors_off();
}

void mcard_msg_handler(void) {
    int saved_disabled;
    int msg_id;

    msg_id = mcard_msg_active;
    if ((msg_id < 0) | (msg_id >= MCARD_MSG_ROUTINE_MAX + 1)) {
        mcard_hault_msg_active = 0;
        pause_procs(0);
        fire_screen_studio_event(0x1FAB, 0);
    }
    saved_disabled = get_controller_disabled_state();
    turn_controllers_on();
    msg_routine_table[mcard_msg_active]();
    reapply_controller_disabled_state(saved_disabled);
}

int is_mcardmsg_active(void) {
    int active;

    active = 0;
    if (is_this_a_hault_message() != 0) {
        if (mcard_hault_msg_active == 1) {
            active = 1;
        }
    } else if (mcard_msg_active != 0) {
        active = 1;
    }
    return active;
}

int ck_mcard_msg(void) {
    return mcard_hault_msg_active;
}

void recover_from_message(void) {
    MkProc* proc;
    int clear;

    clear = 0;
    proc = aproc;
    f_writing_to_memcard = clear;
    if (proc != 0) {
        ((MkProcMessageFlags*)&proc->flags)->sleeping = clear;
    }
    pause_procs(clear);
    if (is_this_a_hault_message() != 0) {
        clear = get_game_state();
        if (clear == GAME_STATE_MCARD_HAULT) {
            pop_game_state(GAME_STATE_MCARD_HAULT);
        }
    }
    mcard_hault_msg_active = 0;
}

void prepare_for_haulting_message(void) {
    MkProc* proc;
    int enable;
    int state;

    enable = 1;
    proc = aproc;
    mcard_hault_msg_active = enable;
    if (proc != 0) {
        state = 0;
        ((MkProcMessageFlags*)&proc->flags)->sleeping = state;
    }
    pause_procs(1);
    state = get_game_state();
    if (state != GAME_STATE_MCARD_HAULT) {
        push_game_state(GAME_STATE_MCARD_HAULT);
    }
}

void prepare_for_sleeping_message(void) {
    MkProc* proc;
    int pause;
    int clear_hault;

    pause = 1;
    clear_hault = 0;
    proc = aproc;
    mcard_hault_msg_active = clear_hault;
    ((MkProcMessageFlags*)&proc->flags)->sleeping = pause;
    pause_procs(pause);
}
