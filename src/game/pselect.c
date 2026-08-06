#include "game/pselect.h"

#include "game/bgnd_types.h"
#include "game/game_info.h"
#include "game/menu.h"
#include "mw/mwScreenEngineGlue.h"
#include "platform/main.h"
#include "platform/main_jump.h"
#include "runtime/asset.h"
#include "runtime/cam.h"
#include "runtime/light.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_info.h"
#include "runtime/section.h"
#include "runtime/utils.h"

/*
 * pselect.o - B19 Wave B/C/D NonMatching chrome + confirm/back + Get* leaves.
 * Rest of TU stays ASM on GC (NonMatching = not linked). Soft ceilings OK.
 *
 * Retail emission order among lifted symbols:
 *   is_pselect_mode -> is_char_locked
 *   -> pselect_bgnd_has_* -> pselect_get_arena_index
 *   -> get_background_select_textures -> get_pselect_body_textures
 *   -> get_num_pselect_body_textures -> get_bg_pselect_team_textures
 *   -> get_pselect_head_textures
 *   -> pselect_get_style_name -> pselect_get_difficulty_level
 *   -> pselect_get_arena_name -> pselect_get_player_name
 *   -> pselect_player_moved -> bg_pselect_player_canceled
 *   -> pselect_player_canceled -> resolve_alternate_palettes
 *   -> pselect_player_selected
 *   -> p_load_alternate_body -> p_play_name_sound -> p_name_sound_die
 *   -> pselect_get_body_texture_index
 *   -> pselect_get_selbox_pos -> pselect_update_selbox_pos
 *   -> bg_pselect_get_stage -> bg_pselect_get_offender_class
 *   -> p_bg_pselect -> p_pz_pselect -> p_pselect
 *   -> pselect_init / init_startup_selboxes (order debt: init before
 *      startup walk so MWCC emits bl; retail is walk then init)
 */

#pragma use_lmw_stmw on

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

/* Unlock masks in gp_data (retail offsets). */
typedef struct GpCharBits {
    unsigned int char_hi; /* +0x00 */
    unsigned int char_lo; /* +0x04 */
    unsigned int alt_hi;  /* +0x08 */
    unsigned int alt_lo;  /* +0x0C */
    char pad10[0x20];
    unsigned int pz_hi; /* +0x30 */
    unsigned int pz_lo; /* +0x34 */
} GpCharBits;

/* Roster cell -- stride 0x28 in pselect_char_tbl / pselect_pz_char_tbl. */
typedef struct PselectCharEntry {
    int char_id;      /* +0x00 */
    int name_sound;   /* +0x04 - p_play_name_sound pdata */
    char* head_name;  /* +0x08 unlocked HEAD_* */
    char* head_lock;  /* +0x0C HEAD_*_LOCKED */
    char* body_name;  /* +0x10 BODY_* */
    char* alt_sec;    /* +0x14 body_*_alt.sec for p_load_alternate_body */
    char* difficulty; /* +0x18 */
    char* style0;     /* +0x1C */
    char* style1;     /* +0x20 */
    char* style2;     /* +0x24 */
} PselectCharEntry; /* 0x28 */

/* mkproc pdata for alt-body load (+0x08 = player from create). */
typedef struct AltBodyPdata {
    MkHdr hdr;  /* +0x00 */
    int player; /* +0x08 */
} AltBodyPdata;

/* mkproc pdata for name-sound (+0x08 id, +0x0C countdown). */
typedef struct NameSoundPdata {
    MkHdr hdr;    /* +0x00 */
    int sound_id; /* +0x08 */
    int countdown; /* +0x0C */
} NameSoundPdata;

typedef struct RandomSelectPdata {
    MkHdr hdr;     /* +0x00 */
    int player;    /* +0x08 */
    int step;      /* +0x0C */
    int num_chars; /* +0x10 */
    int field_14;  /* +0x14 */
} RandomSelectPdata; /* 0x18 */

typedef struct WagerRepeatPdata {
    MkHdr hdr; /* +0x00 */
    int ticks; /* +0x08 */
} WagerRepeatPdata; /* 0x0C */

typedef struct ProfileCodePdata {
    MkHdr hdr;             /* +0x00 */
    int player;            /* +0x08 */
    int port;              /* +0x0C */
    int count;             /* +0x10 */
    unsigned char code[6]; /* +0x14 */
    char pad1A[2];
    int old_player_state; /* +0x1C */
} ProfileCodePdata; /* 0x20 */

typedef struct ProfileCodeKey {
    int switch_index;    /* +0x00 */
    unsigned char value; /* +0x04 */
    char pad05[3];
} ProfileCodeKey; /* 0x08 */

typedef struct PselectProfileView {
    char pad00[0x40];
    int koins[7]; /* +0x40 */
    char pad5C[0x2C];
    int wager_wins[6];   /* +0x88 */
    int wager_losses[7]; /* +0xA0 */
    char padBC[0x514 - 0xBC];
    int bg_team_valid; /* +0x514 */
    int bg_team[5];    /* +0x518 */
} PselectProfileView;

/* field_14 byte: MSB-first alt (bit7) + palette (bit6). */
typedef struct PselectPlyrFlags {
    unsigned char alt : 1;
    unsigned char palette : 1;
    unsigned char lo : 6;
} PselectPlyrFlags;

/* Team block overlay at pdata + team*0x24 (count @ +0x08). */
typedef struct BgPselectTeamView {
    char pad00[8];
    int count;            /* +0x08 */
    int chars[5];         /* +0x0C */
    RwTexture** colors;   /* +0x20 */
    RwTexture** alphas;   /* +0x24 */
    int focus;            /* +0x28 */
} BgPselectTeamView; /* stride 0x24 */

/* Background thumbnail row -- stride 0x0C. flags: bit0 deathtrap, bit1
 * level transition, bit2 weapon (GetInt 0x1FE7..0x1FE9). */
typedef struct PselectBgndEntry {
    int bgnd_id;    /* +0x00 */
    char* tex_name; /* +0x04 */
    int flags;      /* +0x08 */
} PselectBgndEntry; /* 0x0C */

/*
 * Board-game share_pdata (0x50). Two team blocks stride 0x24; get_bg_*
 * overlays color/alpha pointers at +0x20/+0x24 onto the next block.
 */
typedef struct BgPselectPdata {
    MkHdr hdr;    /* +0x00 */
    int count0;   /* +0x08 */
    int team0[5]; /* +0x0C */
    int pad20;    /* +0x20 - color0** at runtime */
    int pad24;    /* +0x24 - alpha0** / team1 start */
    int focus0;   /* +0x28 */
    int count1;   /* +0x2C */
    int team1[5]; /* +0x30 */
    int pad44;    /* +0x44 - color1** */
    int pad48;    /* +0x48 - alpha1** */
    int focus1;   /* +0x4C */
} BgPselectPdata;

extern MkProc* aproc;
extern float _mkproc_sleep_ticks;
extern int menu_player;
extern int target_game_mode;
extern GpCharBits gp_data;
extern unsigned int default_char_bits[2];
extern unsigned int default_alt_char_bits[2];
extern unsigned int default_pz_char_bits[2];
extern CameraObj* camera_obj;
extern LightDef* pselect_light_list[3];
extern MkPtr* bgnd_light_list;
extern PselectCharEntry pselect_char_tbl[];
extern PselectCharEntry pselect_pz_char_tbl[];
extern PselectBgndEntry pselect_bgnd_tbl[];
extern PselectBgndEntry bg_pselect_bgnd_table[];
extern PselectBgndEntry pz_pselect_bgnd_table[];
extern GlobalPlayerEntry global_player_data[];
extern GlobalBackgroundEntry global_background_data[];

char* get_string_by_id(unsigned int id);

void turn_controllers_off(void);
void turn_controllers_on(void);
void setup_sound_banks(int which);
void wait_for_sound_banks_to_load(void);
void set_player_state(PlyrInfo* plyr, int state);
void disable_all_ports_but_me(int port);
void ck_for_controller_removed(void);
void fire_screen_studio_event(int event, int flag);
void set_default_button_repeat_time(void);
void set_button_repeat_time(int ticks);
void init_current_ladder_char(void);
int get_next_bgnd(void);
void unassign_player(PlyrInfo* plyr);
void assign_player(int port);
void ck_do_profile_save(void);
void destroy_mkprocs_pid(int pid);
void refresh_active_screen(void);
void refresh_screen_by_name(char* name);
void check_reset_player_selection(int player, int start_pos);
int check_for_winner(void);
int is_bgnd_locked(int bgnd_id);
void wager_cancelled(void);
int get_current_wager_koin(void);
void eat_switch_edge(int port, int switch_index);
float p_enter_profile_code(void);
int check_switch(int port, int switch_index);
int check_switch_edge(int port, int switch_index);
int load_plyr_model_async(int player, int char_id, int* flags);
void vdebug_print_message(const char* fmt, ...);
void snd_req_delay(int sound_id, int delay);
void* memset(void* dst, int c, unsigned long n);

static float p_load_alternate_body(void);
static float p_play_name_sound(void);
static float p_name_sound_die(void);
static float p_random_player_select(void);
static float p_wager_save_profiles(void);
static float p_save_bg_profile(void);

float p_main_menu(void);
float p_gamelogic(void);
float p_ladder_select(void);
float p_puzzle_fighter(void);

extern int game_save_loop_count;
extern int wager_koin_order[6];
extern PselectProfileView p1_profile;
extern PselectProfileView p2_profile;
extern int p1_profile_status;
extern int p2_profile_status;
extern int profile_code_state[2];
extern ProfileCodeKey pne_kode_data_table[12];

void* memcpy(void* dst, const void* src, int size);

/* GameInfo.pselect @ +0x1D0 -- see game_info.h. */
typedef GameInfoPselectTail PselectGameTail;

/* Partial g_chess_definition_info (0x40) for bg confirm handoff. */
typedef struct ChessDefInfo {
    int bgnd_num; /* +0x00 */
    int p1_empty; /* +0x04 */
    char pad08[4];
    int team0[5]; /* +0x0C */
    int p2_empty; /* +0x20 */
    char pad24[4];
    int team1[5]; /* +0x28 */
    int ready;    /* +0x3C */
} ChessDefInfo;

extern ChessDefInfo g_chess_definition_info;
float p_mk_chess(void);

/* pselect.o .sbss / .sdata (touched by init). */
int pselect_mode;
int p1_selbox_start_pos;
int p2_selbox_start_pos;
int p1_selbox_pos;
int p2_selbox_pos;
int p1_alternate;
int p2_alternate;
int p1_alternate_alpha;
int p2_alternate_alpha;
int name_sound_state;
int force_bgnd_num;
int background_selbox_pos;
int name_sound_active;
int psel_p1_handicap;
int psel_p2_handicap;
int wager_completed_ran;
int f_arena_select_active;

static const float sleep_ticks_one = 1.0f;
static const float sleep_ticks_neg_one = -1.0f;
static const float sleep_ticks_name = 120.0f;
static int rnd_sleep_tbl[19] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 10, 10, 15, 20, 30, 40, -1
};

/* Retail @stringBase0 -- entry-proc offsets match ASM addi immediates. */
#include "game/pselect_stringBase0.inc"

#define STR_P_SELECT (&stringBase0[0xD24])
#define STR_P_PRACTICE (&stringBase0[0xD3D])
#define STR_PZ_SELECT (&stringBase0[0xD58])
#define STR_BG_SELECT (&stringBase0[0xD7F])
#define STR_HEAD_PROXY (&stringBase0[0xDA2])
#define STR_SELECTING_FMT (&stringBase0[0xDAD])
#define STR_BODY_ALT (&stringBase0[0xDC0])
#define STR_BG_ART (&stringBase0[0xDC9])
#define STR_PZ_ART (&stringBase0[0xDDC])
#define STR_PSELECT_ART (&stringBase0[0xDEF])

#define PSELECT_ALT_SLOT_P1 0x17006B
#define PSELECT_ALT_SLOT_P2 0x17006C

static void pselect_init(void);
void resolve_alternate_palettes(PlyrInfo* plyr);

static inline void mkproc_sleep_one(void) {
    MkVtableMkprocLocal* vtbl;

    _mkproc_sleep_ticks = sleep_ticks_one;
    vtbl = (MkVtableMkprocLocal*)aproc->vtbl;
    vtbl->sleep();
}

static inline PselectCharEntry* pselect_char_at(int slot) {
    if (pselect_mode == 2) {
        return &pselect_pz_char_tbl[slot];
    }
    return &pselect_char_tbl[slot];
}

static inline int pselect_grid_cols(void) {
    if ((int)mode_of_play == 6) {
        return 6;
    }
    return 9;
}

static inline int pselect_grid_cells(void) {
    if ((int)mode_of_play == 6) {
        return 0xC;
    }
    return 0x1B;
}

static inline BgPselectTeamView* bg_team_view(BgPselectPdata* pdata, int team) {
    return (BgPselectTeamView*)((char*)pdata + team * 0x24);
}

/* Focus index of char_id in team chars[0..count-2]; -1 if absent. */
static inline int bg_team_focus_of(BgPselectPdata* pdata, int team, int char_id) {
    BgPselectTeamView* teamv;
    int i;

    if (pdata == 0) {
        return -1;
    }
    teamv = bg_team_view(pdata, team);
    for (i = 0; i < 5; i++) {
        if (i >= teamv->count - 1) {
            break;
        }
        if (teamv->chars[i] == char_id) {
            return i;
        }
    }
    return -1;
}

/* Soft ceiling: is_pselect_mode ~80% -- retail subic/subfe bool;
 * MWCC emits neg/or/srwi (same as ScreenEvent). Stop. */
int is_pselect_mode(void) {
    return is_game_state_in_stack(4) != 0;
}

void pselect_update_profile_settings(void) {
    char* screen_name;

    if (!is_game_state_in_stack(4)) {
        return;
    }

    switch ((int)mode_of_play) {
    case 0:
    case 1:
        screen_name = (char*)STR_P_SELECT;
        break;
    case 4:
        screen_name = (char*)STR_P_PRACTICE;
        break;
    case 6:
        screen_name = (char*)STR_PZ_SELECT;
        break;
    case 9:
        screen_name = (char*)STR_BG_SELECT;
        break;
    default:
        return;
    }

    refresh_screen_by_name(screen_name);
    check_reset_player_selection(0, p1_selbox_start_pos);
    check_reset_player_selection(1, p2_selbox_start_pos);
}

int pselect_background_select_available(void) {
    int players;

    if ((g_game_info.field_04 & 0x80) != 0) {
        return (g_game_info.field_04 & 0x40) != 0;
    }

    if ((int)mode_of_play == 6) {
        players = g_game_info.plyr0.player_state != 0;
        if (g_game_info.plyr1.player_state != 0) {
            players += 1;
        }
        return players == 2;
    }

    return (int)mode_of_play != 0;
}

int pselect_is_random(int player) {
    int pos;

    pos = p2_selbox_pos;
    if (player == 0) {
        pos = p1_selbox_pos;
    }
    return pselect_char_at(pos)->char_id == 0x20;
}

void pselect_random_select(int player) {
    RandomSelectPdata* pdata;
    PlyrInfo* plyr;
    int pad;

    pdata = 0;
    if (_create_mkproc_generic_nostack(0x902C, 0x1F,
                                       p_random_player_select, 0x18,
                                       (MkHdr**)&pdata) == 0) {
        return;
    }

    pdata->player = player;
    pdata->step = 0;
    pdata->field_14 = 0;

    if ((int)mode_of_play == 4 &&
        (&g_game_info.plyr0)[menu_player].player_state != 1) {
        pdata->player = menu_player == 0;
    }

    if (pselect_mode == 2) {
        pdata->num_chars = 0xC;
    } else {
        pdata->num_chars = 0x1B;
    }

    plyr = &(&g_game_info.plyr0)[player];
    pad = plyr->pad_index;
    g_game_info.pads[pad].flags |= 0x80;
}

static float p_random_player_select(void) {
    RandomSelectPdata* pdata;
    int slot;
    int char_id;
    int attempts;
    int pad;

    pdata = (RandomSelectPdata*)apdata;
    if (rnd_sleep_tbl[pdata->step] == -1) {
        fire_screen_studio_event(0x1FB3, pdata->player + 1);
        pad = (&g_game_info.plyr0)[pdata->player].pad_index;
        g_game_info.pads[pad].flags &= 0x7F;
        return sleep_ticks_neg_one;
    }

    attempts = 0;
    slot = randu0((unsigned int)pdata->num_chars & 0xFFFF) & 0xFFFF;
    char_id = pselect_char_at(slot)->char_id;
    while (is_char_locked(char_id, 0) || char_id == 0x20) {
        slot = randu0((unsigned int)pdata->num_chars & 0xFFFF) & 0xFFFF;
        char_id = pselect_char_at(slot)->char_id;
        attempts += 1;
        if (attempts > 0x32) {
            slot = 0;
            break;
        }
    }

    pselect_update_selbox_pos(pdata->player, slot);
    return (float)rnd_sleep_tbl[pdata->step++];
}

void award_bet(void) {
    PselectProfileView* winner;
    PselectProfileView* loser;
    int winner_num;
    int koin;
    int amount;

    koin = wager_koin_order[g_game_info.pselect.field_1d4];
    if (koin < 0 || koin > 6) {
        wager_cancelled();
        return;
    }

    if (g_game_info.field_1F8 != 2 ||
        g_game_info.pselect.field_1d0 == 0) {
        return;
    }

    winner_num = check_for_winner();
    if (winner_num == 1) {
        winner = &p1_profile;
        loser = &p2_profile;
    } else if (winner_num == 2) {
        winner = &p2_profile;
        loser = &p1_profile;
    } else {
        return;
    }

    if (g_game_info.pselect.field_1dc == 0) {
        amount = g_game_info.pselect.field_1d8;
        winner->koins[koin] += amount * 2;
        winner->wager_wins[koin] += amount;
        loser->wager_losses[koin] += amount;
        g_game_info.pselect.field_1dc = amount;
    }
}

void wager_cancelled(void) {
    g_game_info.pselect.field_1d4 = 0;
    g_game_info.pselect.field_1d8 = 0;
    g_game_info.pselect.field_1dc = 0;
    g_game_info.pselect.field_1d0 = 0;
    g_game_info.pselect.field_1e0 = 1;
    g_game_info.pselect.field_1f0 = 1;
    set_default_button_repeat_time();
    wager_completed_ran = 0;
}

static float p_wager_save_profiles(void) {
    int p1_saved;
    int p2_saved;
    int amount;
    int koin;

    turn_controllers_off();
    p1_saved = ((int (*)(int, int))save_profile)(0, 2);
    p2_saved = ((int (*)(int, int))save_profile)(1, 2);
    if (p1_saved == 0 || p2_saved == 0) {
        amount = g_game_info.pselect.field_1d8;
        if (amount > 0) {
            koin = wager_koin_order[g_game_info.pselect.field_1d4];
            g_game_info.pselect.field_1d4 = 0;
            g_game_info.pselect.field_1d8 = 0;
            g_game_info.pselect.field_1dc = 0;
            p1_profile.koins[koin] += amount;
            p2_profile.koins[koin] += amount;
            g_game_info.pselect.field_1d0 = 0;
            g_game_info.pselect.field_1e0 = 1;
            g_game_info.pselect.field_1f0 = 1;
            set_default_button_repeat_time();
            wager_completed_ran = 0;
            game_save_loop_count = 0;
        }
        g_game_info.pselect.field_1d4 = 0;
        g_game_info.pselect.field_1d8 = 0;
        g_game_info.pselect.field_1dc = 0;
        g_game_info.pselect.field_1d0 = 0;
        g_game_info.pselect.field_1e0 = 1;
        g_game_info.pselect.field_1f0 = 1;
        set_default_button_repeat_time();
        wager_completed_ran = 0;
    }
    turn_controllers_on();
    return sleep_ticks_neg_one;
}

void wager_completed(void) {
    int koin;
    int p1_count;
    int p2_count;
    int max_bet;

    if (wager_completed_ran != 0) {
        return;
    }
    wager_completed_ran = 1;
    set_default_button_repeat_time();

    koin = get_current_wager_koin();
    if (koin < 0 || koin > 6 ||
        g_game_info.pselect.field_1d8 == 0) {
        wager_cancelled();
        return;
    }

    g_game_info.pselect.field_1d0 = 1;
    p1_count = p1_profile.koins[koin];
    p2_count = p2_profile.koins[koin];
    if (p1_count <= 0 || p2_count <= 0) {
        wager_cancelled();
        return;
    }

    max_bet = p1_count;
    if (p2_count < max_bet) {
        max_bet = p2_count;
    }
    if (max_bet < g_game_info.pselect.field_1d8) {
        g_game_info.pselect.field_1d8 = max_bet;
    }

    p1_profile.koins[koin] -= g_game_info.pselect.field_1d8;
    p2_profile.koins[koin] -= g_game_info.pselect.field_1d8;
    ((void* (*)(void*, int))proc_create)(p_wager_save_profiles, 0x209F);
}

int get_current_wager_koin(void) {
    return wager_koin_order[g_game_info.pselect.field_1d4];
}

void ck_decrement_wager_koin_type(void) {
    int old_type;
    int attempts;
    int koin;
    int p1_count;
    int p2_count;
    int available;

    old_type = g_game_info.pselect.field_1d4;
    attempts = 0;
    do {
        g_game_info.pselect.field_1d4 -= 1;
        if (g_game_info.pselect.field_1d4 < 0) {
            g_game_info.pselect.field_1d4 = 5;
        }
        attempts += 1;
        koin = wager_koin_order[g_game_info.pselect.field_1d4];
        p1_count = p1_profile.koins[koin];
        p2_count = p2_profile.koins[koin];
        available = 0;
        if (p1_count > 0 && p2_count > 0) {
            if (p2_count < p1_count) {
                p1_count = p2_count;
            }
            if (p1_count < g_game_info.pselect.field_1d8) {
                g_game_info.pselect.field_1d8 = p1_count;
            }
            available = 1;
        }
    } while (!available && attempts < 7);

    if (attempts >= 7) {
        g_game_info.pselect.field_1d4 = old_type;
    }
}

void ck_increment_wager_koin_type(void) {
    int old_type;
    int attempts;
    int koin;
    int p1_count;
    int p2_count;
    int available;

    old_type = g_game_info.pselect.field_1d4;
    attempts = 0;
    do {
        g_game_info.pselect.field_1d4 += 1;
        if (g_game_info.pselect.field_1d4 > 5) {
            g_game_info.pselect.field_1d4 = 0;
        }
        attempts += 1;
        koin = wager_koin_order[g_game_info.pselect.field_1d4];
        p1_count = p1_profile.koins[koin];
        p2_count = p2_profile.koins[koin];
        available = 0;
        if (p1_count > 0 && p2_count > 0) {
            if (p2_count < p1_count) {
                p1_count = p2_count;
            }
            if (p1_count < g_game_info.pselect.field_1d8) {
                g_game_info.pselect.field_1d8 = p1_count;
            }
            available = 1;
        }
    } while (!available && attempts < 7);

    if (attempts >= 7) {
        g_game_info.pselect.field_1d4 = old_type;
    }
}

static float p_wager_repeat_process(void) {
    WagerRepeatPdata* pdata;

    pdata = (WagerRepeatPdata*)apdata;
    pdata->ticks -= 1;
    if (pdata->ticks < 0) {
        g_game_info.pselect.field_1f0 = 1;
        return sleep_ticks_neg_one;
    }
    return sleep_ticks_one;
}

void ck_decrement_bet(void) {
    MkProc* proc;
    WagerRepeatPdata* pdata;
    int koin;

    koin = get_current_wager_koin();
    if (koin < 0 || koin > 6) {
        return;
    }

    proc = find_mkproc_pid(0x20A4);
    if (proc != 0) {
        g_game_info.pselect.field_1f0 = 1;
        if (proc->instance != 0) {
            ((MkVtableMkprocLocal*)proc->vtbl)->destroy(proc);
        }
    }

    if (p1_profile_status != 1 || p2_profile_status != 1) {
        return;
    }

    if (g_game_info.pselect.field_1d8 > 0) {
        g_game_info.pselect.field_1d8 -= g_game_info.pselect.field_1f0;
        if (g_game_info.pselect.field_1d8 < 0) {
            g_game_info.pselect.field_1d8 = 0;
        }
    }

    proc = find_mkproc_pid(0x20A3);
    if (proc == 0) {
        pdata = 0;
        g_game_info.pselect.field_1f0 = 1;
        proc = _create_mkproc_generic_tinystack(
            0x20A3, 0x1F, p_wager_repeat_process, 0xC, (MkHdr**)&pdata);
        if (proc != 0) {
            pdata->ticks = 7;
        }
    } else {
        pdata = (WagerRepeatPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->ticks = 7;
            g_game_info.pselect.field_1f0 += 3;
            if (g_game_info.pselect.field_1f0 < 0x32) {
                g_game_info.pselect.field_1f0 = 0x32;
            }
            if (g_game_info.pselect.field_1f0 < 0x1E) {
                g_game_info.pselect.field_1f0 = 7;
            }
        }
    }
}

void ck_increment_bet(void) {
    MkProc* proc;
    WagerRepeatPdata* pdata;
    unsigned int next_amount;
    int koin;
    int p1_count;
    int p2_count;

    koin = get_current_wager_koin();
    if (koin < 0 || koin > 6) {
        return;
    }

    proc = find_mkproc_pid(0x20A3);
    if (proc != 0) {
        g_game_info.pselect.field_1f0 = 1;
        if (proc->instance != 0) {
            ((MkVtableMkprocLocal*)proc->vtbl)->destroy(proc);
        }
    }

    if (p1_profile_status != 1 || p2_profile_status != 1) {
        return;
    }

    next_amount = (unsigned int)(g_game_info.pselect.field_1d8 +
                                 g_game_info.pselect.field_1f0);
    p1_count = p1_profile.koins[koin];
    p2_count = p2_profile.koins[koin];
    if (next_amount > (unsigned int)p1_count ||
        next_amount > (unsigned int)p2_count) {
        return;
    }
    g_game_info.pselect.field_1d8 = (int)next_amount;

    proc = find_mkproc_pid(0x20A4);
    if (proc == 0) {
        pdata = 0;
        g_game_info.pselect.field_1f0 = 1;
        proc = _create_mkproc_generic_tinystack(
            0x20A4, 0x1F, p_wager_repeat_process, 0xC, (MkHdr**)&pdata);
        if (proc != 0) {
            pdata->ticks = 7;
        }
    } else {
        pdata = (WagerRepeatPdata*)pdata_of_proc(proc);
        if (pdata != 0) {
            pdata->ticks = 7;
            g_game_info.pselect.field_1f0 += 3;
            if (g_game_info.pselect.field_1f0 > 0x32) {
                g_game_info.pselect.field_1f0 = 3;
            }
        }
    }
}

int ok_to_bring_out_wager_screen(void) {
    int i;
    int has_common_koin;
    int mop;

    if (p1_profile_status != 1 || p2_profile_status != 1) {
        return 0;
    }
    if (g_game_info.field_1F8 < 2) {
        return 0;
    }
    if (g_game_info.plyr0.player_state != 1 ||
        g_game_info.plyr1.player_state != 1) {
        return 0;
    }

    mop = (int)mode_of_play;
    if (mop != 1 && mop != 0) {
        return 0;
    }
    /* These retail exclusions are redundant after the 0/1 gate. */
    if (mop == 7 || mop == 4) {
        return 0;
    }

    has_common_koin = 0;
    for (i = 0; i < 6; i++) {
        if (p1_profile.koins[i] != 0 && p2_profile.koins[i] != 0) {
            has_common_koin = 1;
        }
    }
    if (!has_common_koin || g_game_info.pselect.field_1d0 != 0) {
        return 0;
    }

    set_button_repeat_time(6);
    return 1;
}

void init_wagering(void) {
    g_game_info.pselect.field_1d4 = 0;
    g_game_info.pselect.field_1d8 = 0;
    g_game_info.pselect.field_1dc = 0;
    g_game_info.pselect.field_1d0 = 0;
    g_game_info.pselect.field_1e0 = 1;
    g_game_info.pselect.field_1f0 = 1;
    set_default_button_repeat_time();
    wager_completed_ran = 0;
}

void ck_restore_kiddy(void) {
    int amount;
    int koin;

    amount = g_game_info.pselect.field_1d8;
    if (amount > 0) {
        koin = wager_koin_order[g_game_info.pselect.field_1d4];
        g_game_info.pselect.field_1d4 = 0;
        g_game_info.pselect.field_1d8 = 0;
        g_game_info.pselect.field_1dc = 0;
        p1_profile.koins[koin] += amount;
        p2_profile.koins[koin] += amount;
        g_game_info.pselect.field_1d0 = 0;
        g_game_info.pselect.field_1e0 = 1;
        g_game_info.pselect.field_1f0 = 1;
        set_default_button_repeat_time();
        wager_completed_ran = 0;
        game_save_loop_count = 0;
    }
}

void pselect_start_code_entry(int player, int port) {
    ProfileCodePdata* pdata;
    MkProc* proc;
    PlyrInfo* plyr;

    destroy_mkprocs_pid(player + 0x9026);
    pdata = 0;
    proc = _create_mkproc_generic_bigstack(player + 0x9026, 0x1F,
                                           p_enter_profile_code, 0x20,
                                           (MkHdr**)&pdata);
    eat_switch_edge(port, 2);
    eat_switch_edge(port, 6);
    if (proc == 0) {
        return;
    }

    push_game_state(0x1B);
    zero_pdata_payload(0x20, (MkHdr*)pdata);
    plyr = g_game_info.pads[port].player;
    pdata->old_player_state = plyr->player_state;
    set_player_state(plyr, 1);
    pdata->player = player;
    pdata->port = port;
    profile_code_state[player] = 0;
}

float p_enter_profile_code(void) {
    ProfileCodePdata* pdata;
    int player;
    int port;
    int i;
    int result;

    pdata = (ProfileCodePdata*)apdata;
    if (pdata == 0) {
        return sleep_ticks_neg_one;
    }

    player = pdata->player;
    port = pdata->port;
    if (pdata->count < 6) {
        for (i = 0; i < 12; i++) {
            if (check_switch_edge(port,
                                  pne_kode_data_table[i].switch_index)) {
                pdata->code[pdata->count] = pne_kode_data_table[i].value;
                pdata->count += 1;
                profile_code_state[player] = pdata->count;
                fire_screen_studio_event(0x1FAA, player + 1);
                break;
            }
        }

        if (check_switch_edge(port, 0xB)) {
            for (i = 0; i < 6; i++) {
                pdata->code[i] = 0;
            }
            pdata->count = 0;
            profile_code_state[player] = 0;
            if (player == 0) {
                fire_screen_studio_event(0x1FC4, player + 1);
            } else {
                fire_screen_studio_event(0x1FC5, player + 1);
            }
        }
        return sleep_ticks_one;
    }

    result = load_profile(player, port, pdata->code);
    if (result == 2) {
        profile_code_state[player] = 7;
        fire_screen_studio_event(0x1FAA, player + 1);
    } else if (result == 1) {
        profile_code_state[player] = 8;
        fire_screen_studio_event(0x1FAA, player + 1);
    } else if (result == 3) {
        profile_code_state[player] = 9;
        fire_screen_studio_event(0x1FAA, player + 1);
    }

    if (get_game_state() == 0x1B) {
        pop_game_state();
    }
    set_player_state(&(&g_game_info.plyr0)[player], pdata->old_player_state);
    return sleep_ticks_neg_one;
}

/*
 * Soft ceiling: is_char_locked ~61% -- retail unsigned-range
 * subfc/subfe + __shl2i/cntlzw; MWCC emits cmplwi / different shift. Stop.
 * Locked when character bit is clear in (gp_data | defaults).
 */
int is_char_locked(int char_id, int alt_bit) {
    unsigned long long mask;
    unsigned long long bit;
    unsigned int mask_hi;
    unsigned int mask_lo;

    if ((unsigned int)char_id >= 0x2Cu) {
        return 1;
    }

    if ((int)mode_of_play == 6) {
        mask_hi = gp_data.pz_hi | default_pz_char_bits[0];
        mask_lo = gp_data.pz_lo | default_pz_char_bits[1];
    } else if (alt_bit != 0) {
        mask_hi = gp_data.alt_hi | default_alt_char_bits[0];
        mask_lo = gp_data.alt_lo | default_alt_char_bits[1];
    } else {
        mask_hi = gp_data.char_hi | default_char_bits[0];
        mask_lo = gp_data.char_lo | default_char_bits[1];
    }

    mask = ((unsigned long long)mask_hi << 32) | (unsigned long long)mask_lo;
    bit = 1ULL << (unsigned int)char_id;
    return (mask & bit) == 0ull;
}

void send_player_status_msg(void) {
}

void pselect_handicap_update(void) {
}

void pselect_handicap_show(void) {
}

void pselect_bgnd_select_done(void) {
    f_arena_select_active = 0;
}

/*
 * Wave D GetInt leaves (mkGameVariables):
 *   0x1FE9 weapon / 0x1FE8 level-transition / 0x1FE7 deathtrap
 *   0x1F94 arena index
 * Retail seeds table base from wager_koin_order (+0x18 / +0x120 / +0x15c).
 * Soft ceiling ~68-71% -- mode-switch / rlwinm vs & / ctr loop. Soft OK.
 */
int pselect_bgnd_has_weapon(void) {
    PselectBgndEntry* tbl;
    int max;
    int pos;

    tbl = (PselectBgndEntry*)wager_koin_order;
    if (pselect_mode == 1) {
        tbl += 24;
        max = 5;
    } else if (pselect_mode < 1) {
        if (pselect_mode < 0) {
            return 0;
        }
        tbl += 2;
        max = 0x16;
    } else if (pselect_mode < 3) {
        tbl += 29;
        max = 6;
    } else {
        return 0;
    }

    pos = background_selbox_pos;
    if (pos < 0 || pos > max) {
        return 0;
    }
    return tbl[pos].flags & 4;
}

int pselect_bgnd_has_level_transition(void) {
    PselectBgndEntry* tbl;
    int max;
    int pos;

    tbl = (PselectBgndEntry*)wager_koin_order;
    if (pselect_mode == 1) {
        tbl += 24;
        max = 5;
    } else if (pselect_mode < 1) {
        if (pselect_mode < 0) {
            return 0;
        }
        tbl += 2;
        max = 0x16;
    } else if (pselect_mode < 3) {
        tbl += 29;
        max = 6;
    } else {
        return 0;
    }

    pos = background_selbox_pos;
    if (pos < 0 || pos > max) {
        return 0;
    }
    return tbl[pos].flags & 2;
}

int pselect_bgnd_has_deathtrap(void) {
    PselectBgndEntry* tbl;
    int max;
    int pos;

    tbl = (PselectBgndEntry*)wager_koin_order;
    if (pselect_mode == 1) {
        tbl += 24;
        max = 5;
    } else if (pselect_mode < 1) {
        if (pselect_mode < 0) {
            return 0;
        }
        tbl += 2;
        max = 0x16;
    } else if (pselect_mode < 3) {
        tbl += 29;
        max = 6;
    } else {
        return 0;
    }

    pos = background_selbox_pos;
    if (pos < 0 || pos > max) {
        return 0;
    }
    return tbl[pos].flags & 1;
}

int pselect_get_arena_index(void) {
    PselectBgndEntry* tbl;
    int count;
    int i;

    tbl = (PselectBgndEntry*)wager_koin_order;
    if (pselect_mode == 1) {
        tbl += 24;
        count = 5;
    } else if (pselect_mode < 1) {
        if (pselect_mode < 0) {
            return 0;
        }
        tbl += 2;
        count = 0x16;
    } else if (pselect_mode < 3) {
        tbl += 29;
        count = 6;
    } else {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (force_bgnd_num == tbl[i].bgnd_id) {
            return i;
        }
    }
    return 0;
}

void pselect_set_arena(int new_pos) {
    PselectBgndEntry* table;
    int count;
    int direction;
    int requested_pos;

    if (f_arena_select_active == 0) {
        return;
    }
    if (new_pos == -1) {
        force_bgnd_num = -1;
        return;
    }

    if (pselect_mode == 0) {
        table = pselect_bgnd_tbl;
        count = 0x16;
    } else if (pselect_mode == 1) {
        table = bg_pselect_bgnd_table;
        count = 5;
    } else if (pselect_mode == 2) {
        table = pz_pselect_bgnd_table;
        count = 6;
    } else {
        return;
    }

    if (new_pos >= count) {
        new_pos = 0;
    }
    requested_pos = new_pos;
    if (new_pos < background_selbox_pos) {
        direction = -1;
    } else if (new_pos == count - 1 && background_selbox_pos == 0) {
        direction = -1;
    } else {
        direction = 1;
    }

    if (new_pos < 0) {
        return;
    }

    background_selbox_pos = new_pos;
    for (;;) {
        force_bgnd_num = table[background_selbox_pos].bgnd_id;
        if ((int)mode_of_play == 9 || !is_bgnd_locked(force_bgnd_num)) {
            break;
        }
        background_selbox_pos += direction;
        if (background_selbox_pos >= count) {
            background_selbox_pos = 0;
        } else if (background_selbox_pos < 0) {
            background_selbox_pos = count - 1;
        }
    }

    if (background_selbox_pos != requested_pos) {
        fire_screen_studio_event(0x1FB8, 0);
    }
}

void pselect_init_arena_select(void) {
    f_arena_select_active = 1;
    if (force_bgnd_num < 0) {
        pselect_set_arena(0);
    }
    fire_screen_studio_event(0x1FB8, 0);
}

int get_num_selectable_bgnds(void) {
    if (pselect_mode == 0) {
        return 0x16;
    }
    if (pselect_mode == 1) {
        return 5;
    }
    if (pselect_mode == 2) {
        return 6;
    }
    return 0;
}

void get_pz_special_move_list(PselectTexOut* out, int use_difficulty) {
    char* name;
    int i;

    for (i = 0; i < 0xC; i++) {
        if (use_difficulty != 0) {
            name = pselect_pz_char_tbl[i].difficulty;
        } else {
            name = pselect_pz_char_tbl[i].style0;
        }

        if (name != 0) {
            out->color[i] = load_named_tga_from_slot(PSELECT_SEC_SLOT, name);
            out->alpha[i] =
                load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT, name);
        } else {
            out->color[i] = 0;
            out->alpha[i] = 0;
        }
    }
}

/* Soft ceiling: get_background_select_textures -- stringBase0 / table base. */
void get_background_select_textures(PselectTexOut* out) {
    PselectBgndEntry* tbl;
    int n;
    int i;
    int dst;

    if (pselect_mode == 1) {
        tbl = bg_pselect_bgnd_table;
        n = 5;
    } else if (pselect_mode == 0) {
        tbl = pselect_bgnd_tbl;
        n = 0x16;
    } else if (pselect_mode == 2) {
        tbl = pz_pselect_bgnd_table;
        n = 6;
    } else {
        return;
    }

    dst = 0;
    for (i = 0; i < n; i++) {
        out->color[dst] =
            load_named_tga_from_slot(PSELECT_SEC_SLOT, tbl[i].tex_name);
        out->alpha[dst] = load_named_alpha_texture_from_slot(
            PSELECT_SEC_SLOT, tbl[i].tex_name);
        dst += 1;
    }
}

/* Soft ceiling: get_pselect_body_textures -- alternate latch / stmw. */
void get_pselect_body_textures(PselectTexOut* out) {
    PselectCharEntry* tbl;
    int n;
    int i;
    char* name;

    if (pselect_mode == 2) {
        n = 0xC;
        tbl = pselect_pz_char_tbl;
    } else {
        n = 0x1B;
        tbl = pselect_char_tbl;
    }

    memset(out->color, 0, (unsigned long)(n * 4));
    memset(out->alpha, 0, (unsigned long)(n * 4));

    for (i = 0; i < n; i++) {
        name = tbl[i].body_name;
        if (name != 0) {
            out->color[i] = load_named_tga_from_slot(PSELECT_SEC_SLOT, name);
            out->alpha[i] =
                load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT, name);
        }
    }

    if (pselect_mode == 0) {
        if (p1_alternate == 0) {
            out->color[n] = out->color[p1_selbox_pos];
            out->alpha[n] = out->alpha[p1_selbox_pos];
        } else {
            out->color[n] = (RwTexture*)p1_alternate;
            out->alpha[n] = (RwTexture*)p1_alternate_alpha;
        }
        if (p2_alternate == 0) {
            out->color[n + 1] = out->color[p2_selbox_pos];
            out->alpha[n + 1] = out->alpha[p2_selbox_pos];
        } else {
            out->color[n + 1] = (RwTexture*)p2_alternate;
            out->alpha[n + 1] = (RwTexture*)p2_alternate_alpha;
        }
    }
}

int get_num_pselect_body_textures(void) {
    int mop;

    mop = (int)mode_of_play;
    if (mop == 9) {
        return 0x1B;
    }
    if (mop >= 9) {
        return 0x1D;
    }
    if (mop == 6) {
        return 0xC;
    }
    return 0x1D;
}

/*
 * Soft ceiling: get_bg_pselect_team_textures -- dual table scans /
 * share_pdata overlays. Soft OK.
 */
void get_bg_pselect_team_textures(PselectTexOut* out, int team) {
    BgPselectPdata* pdata;
    BgPselectTeamView* teamv;
    int i;
    int char_id;
    int slot;
    int sel_pos;
    int focus_char;
    RwTexture* tex;
    PselectCharEntry* ent;

    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    teamv = bg_team_view(pdata, team);
    teamv->colors = out->color;
    teamv->alphas = out->alpha;

    for (i = 0; i < 5; i++) {
        if (i > teamv->count - 2) {
            out->color[i] = 0;
            out->alpha[i] = 0;
            continue;
        }
        char_id = teamv->chars[i];
        if (char_id == 0x2C) {
            out->color[i] = 0;
            out->alpha[i] = 0;
            continue;
        }

        tex = 0;
        for (slot = 0; slot < 0x1B; slot++) {
            ent = &pselect_char_tbl[slot];
            if (char_id == ent->char_id) {
                tex = load_named_tga_from_slot(PSELECT_SEC_SLOT, ent->head_name);
                break;
            }
        }
        out->color[i] = tex;

        tex = 0;
        for (slot = 0; slot < 0x1B; slot++) {
            ent = &pselect_char_tbl[slot];
            if (char_id == ent->char_id) {
                tex = load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT,
                                                        ent->head_name);
                break;
            }
        }
        out->alpha[i] = tex;
    }

    if (pselect_mode != 1) {
        return;
    }
    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    teamv = bg_team_view(pdata, team);
    sel_pos = (team == 0) ? p1_selbox_pos : p2_selbox_pos;
    focus_char = pselect_char_at(sel_pos)->char_id;

    if (teamv->count > 0 && teamv->count < 6) {
        ent = &pselect_char_tbl[sel_pos];
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT, ent->head_name);
        teamv->colors[teamv->count - 1] = tex;
    }

    teamv->focus = bg_team_focus_of(pdata, team, focus_char);
}

/* Soft ceiling: get_pselect_head_textures -- locked vs unlocked name pick. */
void get_pselect_head_textures(PselectTexOut* out) {
    PselectCharEntry* tbl;
    int n;
    int i;
    char* name;

    if (pselect_mode == 2) {
        n = 0xC;
        tbl = pselect_pz_char_tbl;
    } else {
        n = 0x1B;
        tbl = pselect_char_tbl;
    }

    for (i = 0; i < n; i++) {
        if (is_char_locked(tbl[i].char_id, 0)) {
            name = tbl[i].head_lock;
            out->color[i] = load_named_tga_from_slot(PSELECT_SEC_SLOT, name);
            out->alpha[i] =
                load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT, name);
        } else if (tbl[i].head_name == 0) {
            out->color[i] = 0;
            out->alpha[i] = 0;
        } else {
            name = tbl[i].head_name;
            out->color[i] = load_named_tga_from_slot(PSELECT_SEC_SLOT, name);
            out->alpha[i] =
                load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT, name);
        }
    }
}

/* Wave D GetString leaves (ScreenEngine name/style/arena chrome). Soft OK. */
char* pselect_get_style_name(int player, int style_idx) {
    int pos;

    pos = -1;
    if (player == 0) {
        pos = p1_selbox_pos;
    } else if (player == 1) {
        pos = p2_selbox_pos;
    }
    if (pos < 0 || pos >= 0x2C) {
        return 0;
    }
    return (&pselect_char_tbl[pos].style0)[style_idx];
}

char* pselect_get_difficulty_level(int player) {
    int pos;

    pos = -1;
    if (player == 0) {
        pos = p1_selbox_pos;
    } else if (player == 1) {
        pos = p2_selbox_pos;
    }
    if (pos < 0 || pos >= 0x2C) {
        return 0;
    }
    return pselect_char_tbl[pos].difficulty;
}

/* Soft ceiling: pselect_get_arena_name -- mode-switch / gbd index schedule. */
char* pselect_get_arena_name(void) {
    char* base;
    int bgnd_id;
    int pos;

    base = (char*)wager_koin_order;
    bgnd_id = 0x23;
    if (pselect_mode == 1) {
        pos = background_selbox_pos;
        bgnd_id = *(int*)(base + 0x120 + pos * 0xC);
    } else if (pselect_mode < 1) {
        if (pselect_mode >= 0) {
            pos = background_selbox_pos;
            bgnd_id = *(int*)(base + 0x18 + pos * 0xC);
        }
    } else if (pselect_mode < 3) {
        pos = background_selbox_pos;
        bgnd_id = *(int*)(base + 0x15c + pos * 0xC);
    }

    if (bgnd_id == 0x23) {
        return 0;
    }
    return get_string_by_id(
        (unsigned int)global_background_data[bgnd_id].field8 | 0x10000u);
}

char* pselect_get_player_name(int player) {
    int pos;
    int char_id;

    pos = -1;
    if (player == 0) {
        pos = p1_selbox_pos;
    } else if (player == 1) {
        pos = p2_selbox_pos;
    }
    if (pos < 0 || pos >= 0x2C) {
        return 0;
    }

    if (pselect_mode == 2) {
        char_id = pselect_pz_char_tbl[pos].char_id;
    } else {
        char_id = pselect_char_tbl[pos].char_id;
    }
    return (char*)global_player_data[char_id].name;
}

/* Soft ceiling: pselect_player_moved -- bg team focus refresh. Soft OK. */
void pselect_player_moved(int player) {
    BgPselectPdata* pdata;
    BgPselectTeamView* teamv;
    int sel_pos;
    int char_id;
    int count;
    RwTexture* tex;

    if (pselect_mode != 1) {
        return;
    }
    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    teamv = bg_team_view(pdata, player);
    count = teamv->count;
    sel_pos = p2_selbox_pos;
    if (player == 0) {
        sel_pos = p1_selbox_pos;
    }

    /* Retail re-reads pselect_mode for pz vs normal char_id. */
    if (pselect_mode == 2) {
        char_id = pselect_pz_char_tbl[sel_pos].char_id;
    } else {
        char_id = pselect_char_tbl[sel_pos].char_id;
    }

    if (count > 0 && count <= 5) {
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT,
                                       pselect_char_tbl[sel_pos].head_name);
        teamv->colors[count - 1] = tex;
    }

    teamv->focus = bg_team_focus_of((BgPselectPdata*)get_screen_pdata(), player,
                                    char_id);
}

/* Soft ceiling: bg_pselect_player_canceled -- team slot pop + HEAD_PROXY. */
void bg_pselect_player_canceled(int player) {
    BgPselectPdata* pdata;
    BgPselectTeamView* teamv;
    int count;
    int sel_pos;
    int char_id;
    RwTexture* tex;

    if (pselect_mode != 1) {
        return;
    }
    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    teamv = bg_team_view(pdata, player);
    count = teamv->count;
    sel_pos = (player == 0) ? p1_selbox_pos : p2_selbox_pos;

    if (count > 0 && count < 6) {
        teamv->chars[count - 1] = 0x2C;
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT, (char*)STR_HEAD_PROXY);
        teamv->colors[count - 1] = tex;
    }

    if (count > 1 && count < 7) {
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT,
                                       pselect_char_tbl[sel_pos].head_name);
        teamv->colors[count - 2] = tex;
        teamv->chars[count - 2] = 0x2C;
        char_id = pselect_char_at(sel_pos)->char_id;
        teamv->focus =
            bg_team_focus_of((BgPselectPdata*)get_screen_pdata(), player,
                             char_id);
    }
}

/*
 * Soft ceiling: pselect_player_canceled -- other-player state gate /
 * cntlzw other index. Soft OK.
 */
void pselect_player_canceled(int player) {
    PlyrInfo* plyrs;
    PlyrInfo* other;
    PlyrInfo* self;
    int start;

    plyrs = &g_game_info.plyr0;
    other = &plyrs[player == 0];
    self = &plyrs[player];

    if (other->player_state == 2) {
        return;
    }

    name_sound_state -= 1;
    self->player_state = 1;
    self->field_14 = 0;

    if (player == 0) {
        destroy_mkprocs_pid(0x9035);
        p1_alternate = 0;
        p1_alternate_alpha = 0;
    } else {
        destroy_mkprocs_pid(0x9036);
        p2_alternate = 0;
        p2_alternate_alpha = 0;
    }

    refresh_active_screen();
    if ((int)mode_of_play == 4) {
        other->player_state = 0;
    }
    fire_screen_studio_event(0x1FEC, player + 1);
    start = (player == 0) ? p1_selbox_start_pos : p2_selbox_start_pos;
    check_reset_player_selection(player, start);
}

/*
 * Soft ceiling: resolve_alternate_palettes -- field_14 bit6 rlwimi.
 * Soft OK.
 */
void resolve_alternate_palettes(PlyrInfo* plyr) {
    PlyrInfo* other;
    PselectPlyrFlags* flags;
    PselectPlyrFlags* oflags;
    unsigned int bit;

    other = &g_game_info.plyr0;
    if (plyr == &g_game_info.plyr0) {
        other = &g_game_info.plyr1;
    }

    flags = (PselectPlyrFlags*)&plyr->field_14;
    oflags = (PselectPlyrFlags*)&other->field_14;

    if (g_game_info.plyr0.player_index != g_game_info.plyr1.player_index ||
        ((*(unsigned char*)&g_game_info.plyr0.field_14 >> 7) & 1) !=
            ((*(unsigned char*)&g_game_info.plyr1.field_14 >> 7) & 1)) {
        flags->palette = 0;
        return;
    }

    if (plyr->player_state == 2 && other->player_state == 2) {
        flags->palette = 1;
        oflags->palette = 0;
    } else if (plyr->player_state == 0 && other->player_state == 0) {
        bit = randu0(2) & 1;
        flags->palette = (unsigned char)bit;
        oflags->palette = (unsigned char)(1 - bit);
    } else {
        if (plyr->player_state == 0 || plyr->player_state == 3) {
            flags->palette = 1;
        } else {
            flags->palette = 0;
        }
        if (other->player_state == 0 || other->player_state == 3) {
            oflags->palette = 1;
        } else {
            oflags->palette = 0;
        }
    }
}

/*
 * Soft ceiling: pselect_player_selected -- sets PlyrInfo.player_index;
 * alt costume / name-sound / model-async kept for retail confirm (no fight
 * jump). Soft OK.
 */
void pselect_player_selected(PlyrInfo* plyr) {
    PlyrInfo* other;
    int* sel_pos;
    int studio_ev;
    int char_id;
    int pad;
    int locked;
    int player;
    int focus;
    MkHdr* pdata;
    PselectPlyrFlags* flags;
    int flag_word;

    if (plyr->player_state == 2) {
        return;
    }

    if (pselect_mode != 1) {
        if ((int)mode_of_play == 4) {
            if (plyr->field_04 == menu_player) {
                set_player_state(plyr, 2);
                other = &g_game_info.plyr0;
                if (plyr == &g_game_info.plyr0) {
                    other = &g_game_info.plyr1;
                }
                set_player_state(other, 1);
            } else {
                set_player_state(plyr, 3);
            }
        } else {
            set_player_state(plyr, 2);
        }
    }

    if (plyr->field_04 == 0) {
        sel_pos = &p1_selbox_pos;
        studio_ev = 0x1FA5;
    } else if (plyr->field_04 == 1) {
        sel_pos = &p2_selbox_pos;
        studio_ev = 0x1FA6;
    } else {
        return;
    }

    char_id = pselect_char_at(*sel_pos)->char_id;
    plyr->player_index = char_id;
    flags = (PselectPlyrFlags*)&plyr->field_14;
    flags->alt = 0;

    if ((int)mode_of_play == 4) {
        pad = (&g_game_info.plyr0)[menu_player].pad_index;
    } else {
        pad = plyr->pad_index;
    }

    if (pselect_mode == 0 && check_switch(pad, 0xB) != 0) {
        locked = is_char_locked(plyr->player_index, 1);
            if (locked == 0) {
            flags->alt = 1;
            if (pselect_mode == 0) {
                player = plyr->field_04;
                destroy_mkprocs_pid(player + 0x9035);
                pdata = 0;
                _create_mkproc_generic_bigstack(player + 0x9035, 0x1F,
                                                p_load_alternate_body, 0xC,
                                                &pdata);
                if (pdata != 0) {
                    ((AltBodyPdata*)pdata)->player = player;
                }
            }
        }
    }

    resolve_alternate_palettes(plyr);

    if ((int)mode_of_play != 9) {
        flag_word = plyr->field_14;
        load_plyr_model_async(plyr->field_04, plyr->player_index, &flag_word);
    }

    if (pselect_mode == 2) {
        pdata = 0;
        _create_mkproc_generic_nostack(0x9029, 0x1F, p_play_name_sound, 0x10,
                                       &pdata);
        if (pdata != 0) {
            ((NameSoundPdata*)pdata)->sound_id =
                pselect_char_at(*sel_pos)->name_sound;
            ((NameSoundPdata*)pdata)->countdown = 0xB4;
        }
    } else if (pselect_mode == 0) {
        vdebug_print_message(STR_SELECTING_FMT,
                             pselect_char_tbl[*sel_pos].head_name,
                             exec_tick_ctr);
        pdata = 0;
        _create_mkproc_generic_nostack(0x9029, 0x1F, p_play_name_sound, 0x10,
                                       &pdata);
        if (pdata != 0) {
            ((NameSoundPdata*)pdata)->sound_id =
                pselect_char_tbl[*sel_pos].name_sound;
            ((NameSoundPdata*)pdata)->countdown = 0xB4;
        }
    }

    if (pselect_mode == 1) {
        char_id = pselect_char_tbl[*sel_pos].char_id;
        focus = bg_team_focus_of((BgPselectPdata*)get_screen_pdata(),
                                 plyr->field_04, char_id);
        if (focus >= 0) {
            fire_screen_studio_event(studio_ev, plyr->field_04);
            bg_team_view((BgPselectPdata*)get_screen_pdata(), plyr->field_04)
                ->focus = focus;
        }
    }
}

/*
 * Wave D: load alternate body SEC into per-player slot, stash BODY_ALT
 * TGA into p1/p2_alternate (+ alpha). Spawned from pselect_player_selected
 * when confirm holds switch 0xB and alt unlocked.
 *
 * Soft ceiling: p_load_alternate_body ~93% -- lis/mulli NV color on
 * char_tbl vs slot (0x17xxxx); stringBase0 reloc label. Soft OK.
 */
static float p_load_alternate_body(void) {
    AltBodyPdata* pdata;
    int slot;
    RwTexture** color_out;
    RwTexture** alpha_out;
    int sel_pos;
    char* sec_name;
    char* tex_name;
    RwTexture* tex;

    pdata = (AltBodyPdata*)apdata;
    if (pdata->player == 0) {
        sel_pos = p1_selbox_pos;
        color_out = (RwTexture**)&p1_alternate;
        slot = PSELECT_ALT_SLOT_P1;
        alpha_out = (RwTexture**)&p1_alternate_alpha;
        sec_name = pselect_char_tbl[sel_pos].alt_sec;
    } else {
        sel_pos = p2_selbox_pos;
        color_out = (RwTexture**)&p2_alternate;
        slot = PSELECT_ALT_SLOT_P2;
        alpha_out = (RwTexture**)&p2_alternate_alpha;
        sec_name = pselect_char_tbl[sel_pos].alt_sec;
    }

    load_ssf((MkFileEntry*)pselect_file_table);
    unload_section_slot(slot);
    add_art_section_by_name_async(slot, sec_name);
    wait_for_slot_load(slot);

    tex_name = (char*)STR_BODY_ALT;
    tex = load_named_tga_from_slot(slot, tex_name);
    *color_out = tex;
    if (*color_out != 0) {
        tex_name = (char*)STR_BODY_ALT;
        *alpha_out = load_named_alpha_texture_from_slot(slot, tex_name);
        refresh_active_screen();
    }
    return sleep_ticks_neg_one;
}

/* Soft ceiling: p_play_name_sound ~99.5% -- sdata2 float pool labels. Soft OK. */
static float p_play_name_sound(void) {
    NameSoundPdata* pdata;
    MkVtableMkprocLocal* vtbl;
    float (*jump_sleep)(float ticks, MkProcEntryFn entry);

    pdata = (NameSoundPdata*)apdata;
    if (pdata == 0) {
        return sleep_ticks_neg_one;
    }

    if (name_sound_active != 0) {
        pdata->countdown -= 1;
        if (pdata->countdown < 0) {
            name_sound_active = 0;
        }
        return sleep_ticks_one;
    }

    name_sound_active = 1;
    snd_req_delay(pdata->sound_id, 0x14);
    vtbl = (MkVtableMkprocLocal*)aproc->vtbl;
    jump_sleep = (float (*)(float, MkProcEntryFn))vtbl->jump_sleep;
    jump_sleep(sleep_ticks_name, p_name_sound_die);
    return sleep_ticks_name;
}

/* Soft ceiling: p_name_sound_die ~99.3% -- sdata2 float pool label. Soft OK. */
static float p_name_sound_die(void) {
    name_sound_active = 0;
    name_sound_state += 1;
    return sleep_ticks_neg_one;
}

/*
 * Wave D: ScreenEngine body-panel index via mkGameVariables::GetInt
 * (ids 0x1FFB / 0x1FFC). Alternate costume (field_14 bit7) maps to the
 * extra slots get_pselect_body_textures fills at n / n+1 (0x1B / 0x1C).
 *
 * Soft ceiling: pselect_get_body_texture_index ~90% -- alt path uses
 * cntlzw/subfic for (player==0) vs retail subic/subfe (same leftover
 * family as is_pselect_mode). Soft OK. Tail matched via p2 load then
 * player!=0 early return.
 */
int pselect_get_body_texture_index(int player) {
    PselectPlyrFlags* flags;
    int pos;

    if (pselect_mode == 0) {
        flags = (PselectPlyrFlags*)&(&g_game_info.plyr0)[player].field_14;
        if (flags->alt != 0) {
            return 0x1c - (player == 0);
        }
    }
    pos = p2_selbox_pos;
    if (player != 0) {
        return pos;
    }
    return p1_selbox_pos;
}

/* Soft ceiling: pselect_get_selbox_pos -- retail subic/subfe for player==1. */
int pselect_get_selbox_pos(int player) {
    if (player == 0) {
        return p1_selbox_pos;
    }
    if (player == 1) {
        return p2_selbox_pos;
    }
    return 0;
}

/*
 * Soft ceiling: pselect_update_selbox_pos -- grid wrap / skip locked;
 * retail inlines is_char_locked (__shl2i). Soft OK.
 */
void pselect_update_selbox_pos(int player, int new_pos) {
    int* pos_p;
    int cols;
    int n_cells;
    int skips;
    int delta;
    int char_id;
    int pos;
    int row2;
    int abs_delta;
    int mop;

    if (player == 0) {
        if ((int)mode_of_play != 4 && g_game_info.plyr0.player_state == 0) {
            return;
        }
        pos_p = &p1_selbox_pos;
    } else if (player == 1) {
        if ((int)mode_of_play != 4 && g_game_info.plyr1.player_state == 0) {
            return;
        }
        pos_p = &p2_selbox_pos;
    } else {
        return;
    }

    mop = (int)mode_of_play;
    if (mop == 6) {
        cols = 6;
        n_cells = 0xC;
    } else {
        cols = 9;
        n_cells = 0x1B;
    }

    skips = 0;
    char_id = pselect_char_at(new_pos)->char_id;
    row2 = cols * 2;
    delta = new_pos - *pos_p;
    pos = new_pos;

    while (is_char_locked(char_id, 0)) {
        if (char_id == 0x20) {
            break;
        }
        if (delta == 1 && (*pos_p == cols - 1 || *pos_p == row2 - 1 ||
                           *pos_p == cols * 3 - 1)) {
            pos = *pos_p - (cols - 1);
            *pos_p = pos;
            delta = 1;
        } else {
            abs_delta = delta < 0 ? -delta : delta;
            if (row2 == abs_delta) {
                pos = *pos_p + delta;
                *pos_p = pos;
                if (delta > 0) {
                    delta = -cols;
                } else {
                    delta = cols;
                }
            } else if (delta == -1 &&
                       (*pos_p == 0 || *pos_p == cols || *pos_p == row2)) {
                pos = cols + *pos_p - 1;
                *pos_p = pos;
            } else {
                pos = *pos_p + delta;
                *pos_p = pos;
            }
        }

        if (pos >= n_cells) {
            pos = pos - n_cells;
        } else if (pos < 0) {
            if (delta == -cols) {
                pos = pos + n_cells;
            } else {
                pos = cols;
            }
        }

        skips += 1;
        if (skips > 5) {
            skips = 0;
            *pos_p = *pos_p + 1;
            if (pos >= n_cells) {
                pos = 0;
            }
        }

        char_id = pselect_char_at(pos)->char_id;
    }

    *pos_p = pos;
    if (player == 0) {
        g_game_info.plyr0.player_index = pselect_char_at(pos)->char_id;
    } else if (player == 1) {
        g_game_info.plyr1.player_index = pselect_char_at(pos)->char_id;
    }
    fire_screen_studio_event(0x1FA4, player);
}

void check_reset_player_selection(int player, int start_pos) {
    PlyrInfo* plyr;
    int pos;
    int char_id;

    plyr = &(&g_game_info.plyr0)[player];
    if (plyr->player_state != 1) {
        return;
    }

    pos = p2_selbox_pos;
    if (player == 0) {
        pos = p1_selbox_pos;
    }
    char_id = pselect_char_at(pos)->char_id;
    if (is_char_locked(char_id, 0)) {
        pselect_update_selbox_pos(player, start_pos);
    }
}

/*
 * Wave D GetInt: 0x1F7A / 0x1771 stage; 0x1F78 / 0x1F79 offender class.
 * Soft ceiling ~62% -- null cmplwi / team*0x24 schedule. Soft OK.
 */
int bg_pselect_get_stage(int team) {
    unsigned char* pdata;

    pdata = (unsigned char*)get_screen_pdata();
    if (pdata == 0) {
        return 0;
    }
    return *(int*)(pdata + team * 0x24 + 8);
}

int bg_pselect_get_offender_class(int team) {
    unsigned char* pdata;

    pdata = (unsigned char*)get_screen_pdata();
    if (pdata == 0) {
        return -1;
    }
    return *(int*)(pdata + team * 0x24 + 0x28);
}

void bg_pselect_set_character(int team) {
    BgPselectPdata* pdata;
    BgPselectTeamView* teamv;
    int sel_pos;
    int char_id;

    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    sel_pos = p2_selbox_pos;
    if (team == 0) {
        sel_pos = p1_selbox_pos;
    }
    teamv = bg_team_view(pdata, team);
    if (teamv->count > 0 && teamv->count < 6) {
        char_id = pselect_char_at(sel_pos)->char_id;
        teamv->chars[teamv->count - 1] = char_id;
        teamv->focus = bg_team_focus_of(
            (BgPselectPdata*)get_screen_pdata(), team, char_id);
    }
}

void bg_pselect_set_stage(int team, int stage) {
    BgPselectPdata* pdata;
    BgPselectTeamView* teamv;
    PlyrInfo* plyr;

    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    teamv = bg_team_view(pdata, team);
    teamv->count = stage;
    plyr = &(&g_game_info.plyr0)[team];
    if (stage == 6) {
        set_player_state(plyr, 2);
    }
}

void bg_pselect_save_team(int team) {
    BgPselectPdata* pdata;
    BgPselectTeamView* teamv;
    PselectProfileView* profile;
    WagerRepeatPdata* save_pdata;
    MkProc* proc;
    int i;

    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    if (team == 0) {
        if (p1_profile_status != 1) {
            fire_screen_studio_event(team + 0x1FDD, team + 1);
            return;
        }
        profile = &p1_profile;
    } else {
        if (p2_profile_status != 1) {
            fire_screen_studio_event(team + 0x1FDD, team + 1);
            return;
        }
        profile = &p2_profile;
    }

    teamv = bg_team_view(pdata, team);
    for (i = 0; i < 5; i++) {
        profile->bg_team[i] = teamv->chars[i];
    }
    profile->bg_team_valid = 1;
    teamv->count = 6;

    save_pdata = 0;
    proc = _create_mkproc_generic_bigstack(
        0x902A, 0x1F, p_save_bg_profile, 0xC, (MkHdr**)&save_pdata);
    if (proc == 0) {
        fire_screen_studio_event(team + 0x1FDD, team + 1);
    } else {
        save_pdata->ticks = team;
    }
}

static float p_save_bg_profile(void) {
    WagerRepeatPdata* pdata;
    int player;
    int saved;

    pdata = (WagerRepeatPdata*)apdata;
    player = pdata->ticks;
    saved = ((int (*)(int, int))save_profile)(player, 1);
    _mkproc_sleep_ticks = sleep_ticks_one;
    ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
    if (saved == 0) {
        fire_screen_studio_event(player + 0x1FDD, player + 1);
    } else {
        fire_screen_studio_event(player + 0x1FC8, player + 1);
    }
    return sleep_ticks_neg_one;
}

void bg_pselect_load_team(int team) {
    BgPselectPdata* pdata;
    BgPselectTeamView* teamv;
    PselectProfileView* profile;
    int i;

    pdata = (BgPselectPdata*)get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    if (team == 0) {
        if (p1_profile_status != 1 || p1_profile.bg_team_valid == 0) {
            fire_screen_studio_event(team + 0x1FDF, team + 1);
            return;
        }
        profile = &p1_profile;
    } else {
        if (p2_profile_status != 1 || p2_profile.bg_team_valid == 0) {
            fire_screen_studio_event(team + 0x1FDF, team + 1);
            return;
        }
        profile = &p2_profile;
    }

    teamv = bg_team_view(pdata, team);
    for (i = 0; i < 5; i++) {
        teamv->chars[i] = profile->bg_team[i];
    }
    teamv->count = 6;
    fire_screen_studio_event(team + 0x1FE1, team + 1);
}

/*
 * Soft ceiling: p_bg_pselect ~86% -- team-fill ctr/bdnz vs for-unroll;
 * NV frame coloring. Soft OK.
 */
float p_bg_pselect(void) {
    BgPselectPdata* pdata;
    GameInfo* gi;
    ChessDefInfo* chess;
    int n_selecting;
    int n_done;
    int wait;
    int i;

    turn_controllers_off();
    pselect_mode = 1;
    pselect_init();
    push_game_state(4);

    pdata = (BgPselectPdata*)get_mkpdata_generic(0x50);
    if (pdata == 0) {
        gamelogic_jump(6, p_main_menu);
    }
    mk_insert(&pdata->hdr, &aproc->pdata_list);
    zero_pdata_payload(0x50, &pdata->hdr);

    /* Retail: mtctr 5 / store team0[i]+team1[i] via byte offset. */
    i = 0;
    do {
        pdata->team0[i] = 0x2C;
        pdata->team1[i] = 0x2C;
        i++;
    } while (i < 5);
    pdata->focus0 = -1;
    pdata->focus1 = -1;

    unload_section_slot(PSELECT_SEC_SLOT);
    load_ssf((MkFileEntry*)pselect_file_table);
    load_art_section_by_name(PSELECT_SEC_SLOT, STR_BG_ART);
    load_screen(STR_BG_SELECT, PSELECT_SEC_SLOT, &pdata->hdr, 0);
    turn_camera_on();
    turn_controllers_on();

    gi = &g_game_info;
    if ((gi->field_04 >> 7) & 1) {
        ck_for_controller_removed();
    }

    chess = &g_chess_definition_info;
    for (;;) {
        n_selecting = 0;
        if (gi->plyr0.player_state == 1) {
            n_selecting = 1;
        }
        if (gi->plyr1.player_state == 1) {
            n_selecting += 1;
        }

        if (n_selecting == 0) {
            turn_controllers_off();

            wait = 0xB4;
            n_done = 0;
            if ((int)mode_of_play != 9) {
                mkproc_sleep_one();
                if (gi->plyr0.player_state == 2 || gi->plyr0.player_state == 3) {
                    n_done = 1;
                }
                if (gi->plyr1.player_state == 2 || gi->plyr1.player_state == 3) {
                    n_done += 1;
                }
                while (wait > 0 && name_sound_state < n_done) {
                    mkproc_sleep_one();
                    wait -= 1;
                }
            }

            fire_screen_studio_event(0x1FE7, 0);
            wait_for_screen_close();

            if (target_game_mode == 3) {
                pop_game_state();
                memcpy(chess->team0, pdata->team0, 0x14);
                memcpy(chess->team1, pdata->team1, 0x14);
                chess->ready = 1;
                chess->p1_empty = (gi->plyr0.player_state == 0);
                chess->p2_empty = (gi->plyr1.player_state == 0);
                chess->bgnd_num = force_bgnd_num;
            }
            if (target_game_mode == 3) {
                gamelogic_jump(5, p_mk_chess);
            }
        }

        /* Back: target_game_mode 5 -> menu. */
        if (target_game_mode == 5) {
            wait_for_screen_close();
            turn_controllers_off();
            gamelogic_jump(6, p_main_menu);
        }

        mkproc_sleep_one();
    }

    return sleep_ticks_one;
}

#pragma opt_common_subs off
/* Soft ceiling: p_pz_pselect ~97.47% -- global/FPR coloring and pool labels. */
float p_pz_pselect(void) {
    int bgnd;
    int n_selecting;
    int n_done;
    int wait;
    GameInfo* gi;
    PselectPlyrFlags* f0;
    PselectPlyrFlags* f1;

    pselect_mode = 2;
    turn_controllers_off();
    pselect_init();
    push_game_state(4);

    unload_section_slot(PSELECT_SEC_SLOT);
    load_ssf((MkFileEntry*)pselect_file_table);
    add_art_section_by_name_async_language(PSELECT_SEC_SLOT, STR_PZ_ART);
    preload_screen_data(STR_PZ_SELECT, PSELECT_SEC_SLOT);
    load_screen(STR_PZ_SELECT, PSELECT_SEC_SLOT, 0, 0);
    turn_controllers_on();
    turn_camera_on();

    gi = &g_game_info;
    if ((gi->field_04 >> 7) & 1) {
        ck_for_controller_removed();
    }

    for (;;) {
        n_selecting = 0;
        if (gi->plyr0.player_state == 1) {
            n_selecting = 1;
        }
        if (gi->plyr1.player_state == 1) {
            n_selecting += 1;
        }

        if (n_selecting == 0) {
            bgnd = get_next_bgnd();
            gi->bgnd_id = bgnd;
            if (force_bgnd_num != -1) {
                gi->bgnd_id = force_bgnd_num;
            }

            turn_controllers_off();

            wait = 0xB4;
            n_done = 0;
            if ((int)mode_of_play != 9) {
                mkproc_sleep_one();
                if (gi->plyr0.player_state == 2 || gi->plyr0.player_state == 3) {
                    n_done = 1;
                }
                if (gi->plyr1.player_state == 2 || gi->plyr1.player_state == 3) {
                    n_done += 1;
                }
                while (wait > 0 && name_sound_state < n_done) {
                    mkproc_sleep_one();
                    wait -= 1;
                }
            }

            fire_screen_studio_event(0x1FE7, 0);
            wait_for_screen_close();

            if (target_game_mode == 1) {
                f0 = (PselectPlyrFlags*)&gi->plyr0.field_14;
                f1 = (PselectPlyrFlags*)&gi->plyr1.field_14;
                f0->alt = 0;
                f1->alt = 0;
                pop_game_state();
                if (gi->plyr0.player_state == 0) {
                    gi->field_1FC = 1;
                    gamelogic_jump(6, p_ladder_select);
                } else if (gi->plyr1.player_state == 0) {
                    gi->field_1FC = 0;
                    gamelogic_jump(6, p_ladder_select);
                }
                gamelogic_jump(3, p_puzzle_fighter);
            }
        }

        /* Back: target_game_mode 5 -> menu. */
        if (target_game_mode == 5) {
            wait_for_screen_close();
            turn_controllers_off();
            if (game_save_loop_count != 0) {
                save_both_profiles(2);
                game_save_loop_count = 0;
            }
            gamelogic_jump(6, p_main_menu);
        }

        mkproc_sleep_one();
    }

    return sleep_ticks_one;
}
#pragma opt_common_subs reset

/* pads[2].flags bit6 -- retail rlwimi (MSB-first overlay). */
typedef struct PselectPadConnFlags {
    unsigned char bit7 : 1;
    unsigned char connected : 1;
    unsigned char lo : 6;
} PselectPadConnFlags;

/*
 * Soft ceiling: p_pselect ~97.9% -- indirect sleep-call scheduling and
 * wager-refund scratch-register coloring; stop.
 */
float p_pselect(void) {
    GameInfo* gi;
    PlyrInfo* plyr1;
    PlyrInfo* plyr0;
    PselectPadConnFlags* pad2;
    int n_selecting;
    int n_done;
    int wait;
    int amount;
    int order;
    int* p1_koin;
    int* p2_koin;

    turn_controllers_off();
    pselect_mode = 0;
    pselect_init();
    push_game_state(4);

    if ((int)mode_of_play == 4) {
        disable_all_ports_but_me((&g_game_info.plyr0)[menu_player].pad_index);
    }

    load_ssf((MkFileEntry*)pselect_file_table);
    load_art_section_by_name(PSELECT_SEC_SLOT, STR_PSELECT_ART);

    if ((int)mode_of_play == 4) {
        load_screen(STR_P_PRACTICE, PSELECT_SEC_SLOT, 0, 0);
    } else {
        load_screen(STR_P_SELECT, PSELECT_SEC_SLOT, 0, 0);
    }
    turn_controllers_on();
    turn_camera_on();

    if ((g_game_info.field_04 >> 7) & 1) {
        ck_for_controller_removed();
    }

    /* Retail loop caches plyr1 @ r30 / plyr0 @ r29. */
    gi = &g_game_info;
    plyr1 = &gi->plyr1;
    plyr0 = &gi->plyr0;
    pad2 = (PselectPadConnFlags*)&gi->pads[2].flags;

    for (;;) {
        n_selecting = 0;
        if (gi->plyr0.player_state == 1) {
            n_selecting = 1;
        }
        if (gi->plyr1.player_state == 1) {
            n_selecting += 1;
        }

        if (n_selecting == 0) {
            gi->bgnd_id = get_next_bgnd();
            if ((int)mode_of_play == 0) {
                turn_controllers_off();
            }

            wait = 0xB4;
            n_done = 0;
            if ((int)mode_of_play != 9) {
                mkproc_sleep_one();
                if (gi->plyr0.player_state == 2 || gi->plyr0.player_state == 3) {
                    n_done = 1;
                }
                if (gi->plyr1.player_state == 2 || gi->plyr1.player_state == 3) {
                    n_done += 1;
                }
                while (wait > 0 && name_sound_state < n_done) {
                    mkproc_sleep_one();
                    wait -= 1;
                }
            }

            fire_screen_studio_event(0x1FE7, 0);
            wait_for_screen_close();
            pop_game_state();

            if (target_game_mode == 0) {
                turn_controllers_off();
                gi->plyr0.field_10 = (float)psel_p1_handicap / 100.0f;
                gi->plyr1.field_10 = (float)psel_p2_handicap / 100.0f;

                switch ((int)mode_of_play) {
                case 0:
                case 1:
                    if (gi->plyr0.player_state == 0) {
                        gi->field_1FC = 1;
                        gamelogic_jump(6, p_ladder_select);
                    } else if (gi->plyr1.player_state == 0) {
                        gi->field_1FC = 0;
                        gamelogic_jump(6, p_ladder_select);
                    } else {
                        gamelogic_jump(2, p_gamelogic);
                    }
                    break;
                case 4:
                    if (menu_player == 0) {
                        unassign_player(plyr1);
                    } else {
                        unassign_player(plyr0);
                    }
                    pad2->connected = 1;
                    assign_player(2);
                    pad2->connected = 0;
                    gamelogic_jump(2, p_gamelogic);
                    break;
                }
            }
        }

        /* Back: target_game_mode 5 -> wager refund -> menu. */
        if (target_game_mode == 5) {
            wait_for_screen_close();
            amount = gi->pselect.field_1d8;
            if (amount > 0) {
                order = wager_koin_order[gi->pselect.field_1d4];
                p1_koin = &p1_profile.koins[order];
                p2_koin = &p2_profile.koins[order];
                gi->pselect.field_1d4 = 0;
                gi->pselect.field_1d8 = 0;
                *p1_koin += amount;
                gi->pselect.field_1dc = 0;
                *p2_koin += amount;
                gi->pselect.field_1d0 = 0;
                gi->pselect.field_1e0 = 1;
                gi->pselect.field_1f0 = 1;
                set_default_button_repeat_time();
                wager_completed_ran = 0;
                game_save_loop_count = 0;
            }
            ck_do_profile_save();
            turn_controllers_off();
            gamelogic_jump(6, p_main_menu);
        }

        mkproc_sleep_one();
    }

    return sleep_ticks_one;
}

/*
 * MUST-run init before SSF/screen load. Sound banks deferred.
 * Soft ceiling: pselect_init -- float pool labels / NV frame leftovers.
 *
 * Order debt (Matching): retail emits init_startup_selboxes then pselect_init
 * after p_pselect. Here pselect_init is defined first with a forward decl of
 * init_startup_selboxes so MWCC emits bl (and does not inline the walk).
 */
static void init_startup_selboxes(void);

static void pselect_init(void) {
    init_startup_selboxes();
    set_section_memory_scheme(0xA);
    set_menu_mode(-1);
    /* Deferred: audio. */
    setup_sound_banks(1);
    wait_for_sound_banks_to_load();

    if (g_game_info.plyr0.player_state == 2) {
        set_player_state(&g_game_info.plyr0, 1);
    }
    if (g_game_info.plyr0.player_state == 3) {
        set_player_state(&g_game_info.plyr0, 0);
    }
    if (g_game_info.plyr1.player_state == 2) {
        set_player_state(&g_game_info.plyr1, 1);
    }
    if (g_game_info.plyr1.player_state == 3) {
        set_player_state(&g_game_info.plyr1, 0);
    }

    p1_selbox_pos = p1_selbox_start_pos;
    p2_selbox_pos = p2_selbox_start_pos;
    p1_alternate = 0;
    p2_alternate = 0;
    p1_alternate_alpha = 0;
    p2_alternate_alpha = 0;

    if ((int)mode_of_play == 4) {
        g_game_info.plyr0.field_04 = 0;
        g_game_info.plyr1.field_04 = 1;
    }

    force_bgnd_num = -1;
    background_selbox_pos = 0;
    name_sound_state = 0;
    name_sound_active = 0;
    g_game_info.plyr0.field_14 = 0;
    g_game_info.plyr1.field_14 = 0;
    g_game_info.plyr0.field_10 = sleep_ticks_one;
    g_game_info.plyr1.field_10 = sleep_ticks_one;
    psel_p1_handicap = 100;
    psel_p2_handicap = 100;
    load_lights(pselect_light_list, &bgnd_light_list);

    camera_obj->pos_z = 7.0f;
    camera_obj->ang_y = 3.1415927f;

    g_game_info.bgnd_id = 0x23;
    /* Retail zero order: 1d4, 1d8, 1dc, then 1d0. */
    g_game_info.pselect.field_1d4 = 0;
    g_game_info.pselect.field_1d8 = 0;
    g_game_info.pselect.field_1dc = 0;
    g_game_info.pselect.field_1d0 = 0;
    g_game_info.pselect.field_1e0 = 1;
    g_game_info.pselect.field_1f0 = 1;

    set_default_button_repeat_time();
    wager_completed_ran = 0;
    init_current_ladder_char();
}

/*
 * Walk from default start slots to first unlocked cell.
 * Soft ceiling: init_startup_selboxes -- retail inlines is_char_locked
 * (__shl2i); Soft OK.
 */
static void init_startup_selboxes(void) {
    int max_col;
    int pos;
    int mop;

    mop = (int)mode_of_play;
    p1_selbox_start_pos = 2;
    p2_selbox_start_pos = 6;
    if (mop == 6) {
        p1_selbox_start_pos = 0;
        p2_selbox_start_pos = 5;
    }

    if (mop == 6) {
        max_col = 6;
    } else {
        max_col = 9;
    }

    pos = p1_selbox_start_pos;
    while (is_char_locked(pselect_char_at(pos)->char_id, 0)) {
        pos += 1;
        if (pos > max_col) {
            pos = 0;
        }
    }
    p1_selbox_start_pos = pos;
    p1_selbox_pos = pos;
    if (g_game_info.plyr0.player_state == 1) {
        fire_screen_studio_event(0x1FA4, 0);
    }

    pos = p2_selbox_start_pos;
    while (is_char_locked(pselect_char_at(pos)->char_id, 0)) {
        pos -= 1;
        if (pos < 0) {
            pos = max_col;
        }
    }
    p2_selbox_start_pos = pos;
    p2_selbox_pos = pos;
    if (g_game_info.plyr1.player_state == 1) {
        fire_screen_studio_event(0x1FA4, 1);
    }
}
