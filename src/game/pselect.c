#include "game/pselect.h"
#include "game/game.h"
#include "game/controller.h"

#include "game/bgnd_types.h"
#include "game/bgnd.h"
#include "game/game_info.h"
#include "game/menu.h"
#include "game/ladder.h"
#include "game/plyr_globals.h"
#include "game/plyr.h"
#include "game/profile_unlock.h"
#include "game/profile_code.h"
#include "mw/mwScreenEngineGlue.h"
#include "platform/main.h"
#include "platform/io.h"
#include "platform/main_jump.h"
#include "runtime/asset.h"
#include "runtime/cstring.h"
#include "runtime/cam.h"
#include "runtime/light.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_info.h"
#include "runtime/section.h"
#include "runtime/utils.h"
#include "runtime/sound.h"

#pragma use_lmw_stmw on

struct PselectCharEntry {
    int char_id;
    int name_sound;
    char* head_name;
    char* head_lock;
    char* body_name;
    char* alt_sec;
    char* difficulty;
    char* style0;
    char* style1;
    char* style2;
};

struct AltBodyPdata {
    MkHdr hdr;
    int player;
};

struct NameSoundPdata {
    MkHdr hdr;
    int sound_id;
    int countdown;
};

struct RandomSelectPdata {
    MkHdr hdr;
    int player;
    int step;
    int num_chars;
    int field_14;
};

struct WagerRepeatPdata {
    MkHdr hdr;
    int ticks;
};

struct BgSaveProfilePdata {
    MkHdr hdr;
    int player;
};

struct ProfileCodeKey {
    int switch_index;
    unsigned char value;
    char pad05[3];
};

struct PselectProfileView {
    char pad00[0x40];
    int koins[7];
    char pad5C[0x2C];
    int wager_wins[6];
    int wager_losses[7];
    char padBC[0x514 - 0xBC];
    int bg_team_valid;
    int bg_team[5];
};

struct BgPselectTeamRecord {
    int entries[6];
    RwTexture** colors;
    RwTexture** alphas;
    int focus;
};

struct BgPselectTeamPdata {
    MkHdr hdr;
    struct BgPselectTeamRecord teams[2];
};

struct BgPselectTeamView {
    char pad00[8];
    int count;
    int chars[5];
    RwTexture** colors;
    RwTexture** alphas;
    int focus;
};

struct PselectBgndEntry {
    int bgnd_id;
    const char* tex_name;
    int flags;
};

struct BgPselectPdata {
    MkHdr hdr;
    int count0;
    int team0[5];
    int pad20;
    int pad24;
    int focus0;
    int count1;
    int team1[5];
    int pad44;
    int pad48;
    int focus1;
};

extern int menu_player;
extern ProfileUnlockBits64 default_char_bits;
extern ProfileUnlockBits64 default_alt_char_bits;
extern ProfileUnlockBits64 default_pz_char_bits;
extern GlobalBackgroundEntry global_background_data[];

void set_default_button_repeat_time(void);
void set_button_repeat_time(int ticks);
unsigned int get_next_bgnd(void);
void ck_do_profile_save(void);
void refresh_active_screen(void);
void refresh_screen_by_name(char* name);
void check_reset_player_selection(int player, int start_pos);
int check_for_winner(void);
void wager_cancelled(void);
int get_current_wager_koin(void);
float p_enter_profile_code(void);
void snd_req_delay(int sound_id, int delay);

static float p_load_alternate_body(void);
static float p_play_name_sound(void);
static float p_name_sound_die(void);
static float p_random_player_select(void);
static float p_wager_save_profiles(void);
static float p_save_bg_profile(void);

float p_ladder_select(void);
float p_puzzle_fighter(void);

extern int game_save_loop_count;

extern struct PselectProfileView p1_profile;
extern struct PselectProfileView p2_profile;
extern int p1_profile_status;
extern int p2_profile_status;
extern int profile_code_state[2];
extern struct ProfileCodeKey pne_kode_data_table[12];

struct ChessDefInfo {
    int bgnd_num;
    int p1_empty;
    char pad08[4];
    int team0[5];
    int p2_empty;
    char pad24[4];
    int team1[5];
    int ready;
};

extern struct ChessDefInfo g_chess_definition_info;
float p_mk_chess(void);

int pselect_mode;
int p1_selbox_start_pos;
int p2_selbox_start_pos;
int p1_selbox_pos;
int p2_selbox_pos;
RwTexture* p1_alternate;
RwTexture* p2_alternate;
RwTexture* p1_alternate_alpha;
RwTexture* p2_alternate_alpha;
int name_sound_state;
int background_selbox_pos;
int name_sound_active;
int psel_p1_handicap;
int psel_p2_handicap;
int wager_completed_ran;
int f_arena_select_active;

static const float sleep_ticks_one = 1.0f;
static const float sleep_ticks_neg_one = -1.0f;
static const float sleep_ticks_name = 120.0f;
#include "game/pselect_stringBase0.inc"

int wager_koin_order[6] = {5, 4, 3, 2, 1, 0};

struct PselectBgndEntry pselect_bgnd_tbl[22] = {
    {0, &stringBase0[0x0], 6},
    {19, &stringBase0[0xB], 7},
    {18, &stringBase0[0x19], 7},
    {21, &stringBase0[0x27], 1},
    {17, &stringBase0[0x33], 6},
    {14, &stringBase0[0x41], 2},
    {8, &stringBase0[0x51], 3},
    {6, &stringBase0[0x5D], 1},
    {12, &stringBase0[0x67], 3},
    {2, &stringBase0[0x78], 3},
    {13, &stringBase0[0x83], 3},
    {5, &stringBase0[0x8D], 2},
    {9, &stringBase0[0x98], 4},
    {1, &stringBase0[0xA5], 0},
    {16, &stringBase0[0xB3], 0},
    {20, &stringBase0[0xBE], 1},
    {3, &stringBase0[0xC9], 1},
    {7, &stringBase0[0xD6], 1},
    {10, &stringBase0[0xE3], 1},
    {11, &stringBase0[0xEE], 0},
    {4, &stringBase0[0xF9], 1},
    {15, &stringBase0[0x101], 1},
};

struct PselectBgndEntry bg_pselect_bgnd_table[5] = {
    {20, &stringBase0[0xBE], 0},
    {16, &stringBase0[0xB3], 0},
    {3, &stringBase0[0xC9], 0},
    {1, &stringBase0[0xA5], 0},
    {11, &stringBase0[0xEE], 0},
};

struct PselectBgndEntry pz_pselect_bgnd_table[6] = {
    {0, &stringBase0[0x10B], 0},
    {9, &stringBase0[0x11C], 0},
    {19, &stringBase0[0x12B], 0},
    {8, &stringBase0[0x140], 0},
    {21, &stringBase0[0x153], 0},
    {18, &stringBase0[0x161], 0},
};

#define PSELECT_STRING(offset) ((char*)&stringBase0[offset])

struct PselectCharEntry pselect_char_tbl[27] = {
    {12, 86, PSELECT_STRING(0x171), PSELECT_STRING(0x17D), PSELECT_STRING(0x190), PSELECT_STRING(0x19C), PSELECT_STRING(0x1B0), PSELECT_STRING(0x1B2), PSELECT_STRING(0x1BA), PSELECT_STRING(0x1BF)},
    {21, 84, PSELECT_STRING(0x1C6), PSELECT_STRING(0x1D0), PSELECT_STRING(0x1E1), PSELECT_STRING(0x1EB), PSELECT_STRING(0x1FD), PSELECT_STRING(0x1FF), PSELECT_STRING(0x206), PSELECT_STRING(0x20F)},
    {0, 95, PSELECT_STRING(0x217), PSELECT_STRING(0x217), PSELECT_STRING(0x225), PSELECT_STRING(0x233), PSELECT_STRING(0x249), PSELECT_STRING(0x24B), PSELECT_STRING(0x253), PSELECT_STRING(0x25B)},
    {4, 91, PSELECT_STRING(0x265), PSELECT_STRING(0x217), PSELECT_STRING(0x272), PSELECT_STRING(0x27F), PSELECT_STRING(0x249), PSELECT_STRING(0x294), PSELECT_STRING(0x29F), PSELECT_STRING(0x2AA)},
    {30, 103, PSELECT_STRING(0x2AE), PSELECT_STRING(0x2AE), PSELECT_STRING(0x2B8), PSELECT_STRING(0x2C2), PSELECT_STRING(0x2D4), PSELECT_STRING(0x2D6), PSELECT_STRING(0x2DD), PSELECT_STRING(0x2E4)},
    {1, 77, PSELECT_STRING(0x2F1), PSELECT_STRING(0x217), PSELECT_STRING(0x2FD), PSELECT_STRING(0x309), PSELECT_STRING(0x2D4), PSELECT_STRING(0x31D), PSELECT_STRING(0x323), PSELECT_STRING(0x32C)},
    {3, 99, PSELECT_STRING(0x333), PSELECT_STRING(0x217), PSELECT_STRING(0x340), PSELECT_STRING(0x34D), PSELECT_STRING(0x1B0), PSELECT_STRING(0x362), PSELECT_STRING(0x36B), PSELECT_STRING(0x372)},
    {14, 82, PSELECT_STRING(0x37D), PSELECT_STRING(0x388), PSELECT_STRING(0x399), PSELECT_STRING(0x3A4), PSELECT_STRING(0x1FD), PSELECT_STRING(0x3B7), PSELECT_STRING(0x3BD), PSELECT_STRING(0x3C9)},
    {8, 97, PSELECT_STRING(0x3D6), PSELECT_STRING(0x3E2), PSELECT_STRING(0x3F5), PSELECT_STRING(0x401), PSELECT_STRING(0x2D4), PSELECT_STRING(0x415), PSELECT_STRING(0x41F), PSELECT_STRING(0x42A)},
    {23, 94, PSELECT_STRING(0x432), PSELECT_STRING(0x43E), PSELECT_STRING(0x451), PSELECT_STRING(0x45D), PSELECT_STRING(0x1B0), PSELECT_STRING(0x471), PSELECT_STRING(0x47B), PSELECT_STRING(0x483)},
    {11, 83, PSELECT_STRING(0x489), PSELECT_STRING(0x495), PSELECT_STRING(0x4A8), PSELECT_STRING(0x4B4), PSELECT_STRING(0x4C8), PSELECT_STRING(0x4CA), PSELECT_STRING(0x4D6), PSELECT_STRING(0x4DD)},
    {19, 85, PSELECT_STRING(0x4E6), PSELECT_STRING(0x4F1), PSELECT_STRING(0x503), PSELECT_STRING(0x50E), PSELECT_STRING(0x249), PSELECT_STRING(0x521), PSELECT_STRING(0x529), PSELECT_STRING(0x532)},
    {6, 81, PSELECT_STRING(0x53E), PSELECT_STRING(0x217), PSELECT_STRING(0x549), PSELECT_STRING(0x554), PSELECT_STRING(0x1B0), PSELECT_STRING(0x567), PSELECT_STRING(0x571), PSELECT_STRING(0x57E)},
    {32, 19, PSELECT_STRING(0x582), PSELECT_STRING(0x582), PSELECT_STRING(0x58E), PSELECT_STRING(0x4B4), PSELECT_STRING(0x59A), PSELECT_STRING(0x59A), PSELECT_STRING(0x59A), PSELECT_STRING(0x59A)},
    {5, 92, PSELECT_STRING(0x59C), PSELECT_STRING(0x217), PSELECT_STRING(0x5AB), PSELECT_STRING(0x5BA), PSELECT_STRING(0x1B0), PSELECT_STRING(0x5D1), PSELECT_STRING(0x5DA), PSELECT_STRING(0x5E6)},
    {10, 78, PSELECT_STRING(0x5F0), PSELECT_STRING(0x217), PSELECT_STRING(0x5FE), PSELECT_STRING(0x60C), PSELECT_STRING(0x1B0), PSELECT_STRING(0x622), PSELECT_STRING(0x627), PSELECT_STRING(0x634)},
    {27, 102, PSELECT_STRING(0x63C), PSELECT_STRING(0x64B), PSELECT_STRING(0x661), PSELECT_STRING(0x670), PSELECT_STRING(0x1FD), PSELECT_STRING(0x687), PSELECT_STRING(0x695), PSELECT_STRING(0x6A4)},
    {15, 100, PSELECT_STRING(0x6A6), PSELECT_STRING(0x6B1), PSELECT_STRING(0x6C3), PSELECT_STRING(0x6CE), PSELECT_STRING(0x1FD), PSELECT_STRING(0x6E1), PSELECT_STRING(0x6EC), PSELECT_STRING(0x6F6)},
    {25, 96, PSELECT_STRING(0x701), PSELECT_STRING(0x70F), PSELECT_STRING(0x724), PSELECT_STRING(0x732), PSELECT_STRING(0x1FD), PSELECT_STRING(0x748), PSELECT_STRING(0x74F), PSELECT_STRING(0x75C)},
    {9, 89, PSELECT_STRING(0x769), PSELECT_STRING(0x774), PSELECT_STRING(0x786), PSELECT_STRING(0x791), PSELECT_STRING(0x249), PSELECT_STRING(0x7A4), PSELECT_STRING(0x7AC), PSELECT_STRING(0x7B9)},
    {7, 76, PSELECT_STRING(0x7C4), PSELECT_STRING(0x217), PSELECT_STRING(0x7CF), PSELECT_STRING(0x7DB), PSELECT_STRING(0x1FD), PSELECT_STRING(0x7EF), PSELECT_STRING(0x7F9), PSELECT_STRING(0x800)},
    {22, 79, PSELECT_STRING(0x806), PSELECT_STRING(0x812), PSELECT_STRING(0x825), PSELECT_STRING(0x831), PSELECT_STRING(0x1FD), PSELECT_STRING(0x845), PSELECT_STRING(0x84F), PSELECT_STRING(0x857)},
    {31, 104, PSELECT_STRING(0x862), PSELECT_STRING(0x862), PSELECT_STRING(0x871), PSELECT_STRING(0x880), PSELECT_STRING(0x249), PSELECT_STRING(0x896), PSELECT_STRING(0x89E), PSELECT_STRING(0x8A5)},
    {20, 88, PSELECT_STRING(0x8B2), PSELECT_STRING(0x8BD), PSELECT_STRING(0x8CF), PSELECT_STRING(0x8DA), PSELECT_STRING(0x4C8), PSELECT_STRING(0x8ED), PSELECT_STRING(0x8F8), PSELECT_STRING(0x903)},
    {24, 80, PSELECT_STRING(0x90F), PSELECT_STRING(0x91C), PSELECT_STRING(0x932), PSELECT_STRING(0x93F), PSELECT_STRING(0x1FD), PSELECT_STRING(0x954), PSELECT_STRING(0x95F), PSELECT_STRING(0x967)},
    {18, 87, PSELECT_STRING(0x971), PSELECT_STRING(0x97B), PSELECT_STRING(0x98C), PSELECT_STRING(0x996), PSELECT_STRING(0x1FD), PSELECT_STRING(0x9A8), PSELECT_STRING(0x9B0), PSELECT_STRING(0x9B6)},
    {16, 90, PSELECT_STRING(0x9C3), PSELECT_STRING(0x9D0), PSELECT_STRING(0x9E4), PSELECT_STRING(0x9F1), PSELECT_STRING(0x1FD), PSELECT_STRING(0xA06), PSELECT_STRING(0xA0E), PSELECT_STRING(0xA17)},
};

struct PselectCharEntry pselect_pz_char_tbl[12] = {
    {0, 95, PSELECT_STRING(0xA20), PSELECT_STRING(0xA20), PSELECT_STRING(0xA31), 0, PSELECT_STRING(0xA42), PSELECT_STRING(0xA49), 0, 0},
    {1, 77, PSELECT_STRING(0xA56), PSELECT_STRING(0xA56), PSELECT_STRING(0xA65), 0, PSELECT_STRING(0xA74), PSELECT_STRING(0xA7A), 0, 0},
    {5, 92, PSELECT_STRING(0xA86), PSELECT_STRING(0xA86), PSELECT_STRING(0xA98), 0, PSELECT_STRING(0xAAA), PSELECT_STRING(0xAB8), 0, 0},
    {6, 81, PSELECT_STRING(0xACC), PSELECT_STRING(0xACC), PSELECT_STRING(0xADA), 0, PSELECT_STRING(0xAE8), PSELECT_STRING(0xAF1), 0, 0},
    {8, 97, PSELECT_STRING(0xB00), PSELECT_STRING(0xB00), PSELECT_STRING(0xB0F), 0, PSELECT_STRING(0xB1E), PSELECT_STRING(0xB26), 0, 0},
    {3, 99, PSELECT_STRING(0xB34), PSELECT_STRING(0xB34), PSELECT_STRING(0xB44), 0, PSELECT_STRING(0xB54), PSELECT_STRING(0xB5B), 0, 0},
    {10, 78, PSELECT_STRING(0xB68), PSELECT_STRING(0xB79), PSELECT_STRING(0xB91), 0, PSELECT_STRING(0xBA2), PSELECT_STRING(0xBAB), 0, 0},
    {12, 86, PSELECT_STRING(0xBBA), PSELECT_STRING(0xBC9), PSELECT_STRING(0xBDF), 0, PSELECT_STRING(0xBEE), PSELECT_STRING(0xBF8), 0, 0},
    {4, 91, PSELECT_STRING(0xC08), PSELECT_STRING(0xC18), PSELECT_STRING(0xC2F), 0, PSELECT_STRING(0xC3F), PSELECT_STRING(0xC45), 0, 0},
    {21, 84, PSELECT_STRING(0xC51), PSELECT_STRING(0xC5E), PSELECT_STRING(0xC72), 0, PSELECT_STRING(0xC7F), PSELECT_STRING(0xC85), 0, 0},
    {19, 85, PSELECT_STRING(0xC91), PSELECT_STRING(0xC9F), PSELECT_STRING(0xCB4), 0, PSELECT_STRING(0xCC2), PSELECT_STRING(0xCCD), 0, 0},
    {23, 94, PSELECT_STRING(0xCDE), PSELECT_STRING(0xCED), PSELECT_STRING(0xD03), 0, PSELECT_STRING(0xD12), PSELECT_STRING(0xD18), 0, 0},
};

struct LightDefAmbient pselect_ambient = {
    1, 0, 3, {0.6f, 0.6f, 0.6f, 0.0f},
};

LightDef* pselect_light_list[3] = {
    (LightDef*)&pselect_ambient, 0, 0,
};

static int rnd_sleep_tbl[19] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 10, 10, 15, 20, 30, 40, -1
};

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

static inline void mkproc_sleep_one(void) {
    _mkproc_sleep_ticks = sleep_ticks_one;
    aproc->vtbl->sleep();
}

static inline struct PselectCharEntry* pselect_char_at(int slot) {
    if (pselect_mode == 2) {
        return &pselect_pz_char_tbl[slot];
    }
    return &pselect_char_tbl[slot];
}

static inline int pselect_char_id_at(int slot) {
    if (pselect_mode == 2) {
        return pselect_pz_char_tbl[slot].char_id;
    }
    return pselect_char_tbl[slot].char_id;
}

static inline int pselect_grid_cols(void) {
    if (mode_of_play == 6) {
        return 6;
    }
    return 9;
}

static inline int pselect_grid_cells(void) {
    if (mode_of_play == 6) {
        return 0xC;
    }
    return 0x1B;
}

/* TODO: [Scope warn] GC shifted team view; canonical records regress exact siblings. */
static inline struct BgPselectTeamView* bg_team_view(struct BgPselectPdata* pdata, int team) {
    return (struct BgPselectTeamView*)((char*)pdata + team * 0x24);
}

static inline int bg_team_focus_of(struct BgPselectPdata* pdata, int team, int char_id) {
    struct BgPselectTeamView* teamv;
    int i;
    int member_count;

    if (pdata == 0) {
        return -1;
    }
    teamv = bg_team_view(pdata, team);
    member_count = teamv->count - 1;
    for (i = 0; i < 5; i++) {
        if (i >= member_count) {
            break;
        }
        if (teamv->chars[i] == char_id) {
            return i;
        }
    }
    return -1;
}

int is_pselect_mode(void) {
    return is_game_state_in_stack(4) != 0;
}

void pselect_update_profile_settings(void) {
    if (is_pselect_mode() == 0) {
        return;
    }

    switch (mode_of_play) {
    case 0:
    case 1:
        refresh_screen_by_name((char*)STR_P_SELECT);
        break;
    case 4:
        refresh_screen_by_name((char*)STR_P_PRACTICE);
        break;
    case 6:
        refresh_screen_by_name((char*)STR_PZ_SELECT);
        break;
    case 9:
        refresh_screen_by_name((char*)STR_BG_SELECT);
        break;
    default:
        return;
    }

    check_reset_player_selection(0, p1_selbox_start_pos);
    check_reset_player_selection(1, p2_selbox_start_pos);
}

int pselect_background_select_available(void) {
    int players;

    if (g_game_info.feature_flags.bits.high_bit) {
        return g_game_info.feature_flags.bits.pad_6 != 0;
    }

    if (mode_of_play == 6) {
        players = 0;
        if (g_game_info.plyr0.player_state != 0) {
            players = 1;
        }
        if (g_game_info.plyr1.player_state != 0) {
            players += 1;
        }
        if (players == 2) {
            return 1;
        }
    } else if (mode_of_play != 0) {
        return 1;
    }
    return 0;
}

int pselect_is_random(int player) {
    int pos;
    int character;

    pos = p2_selbox_pos;
    if (player == 0) {
        pos = p1_selbox_pos;
    }
    if (pselect_mode == 2) {
        character = pselect_pz_char_tbl[pos].char_id;
    } else {
        character = pselect_char_tbl[pos].char_id;
    }
    if (character == 0x20) {
        return 1;
    }
    return 0;
}

void pselect_random_select(int player) {
    struct RandomSelectPdata* pdata;
    PlyrInfo* plyr;
    int pad;

    if (_create_mkproc_generic_nostack(0x902C, 0x1F,
                                       p_random_player_select, sizeof(struct RandomSelectPdata),
                                       (MkHdr**)&pdata) == 0) {
        return;
    }

    pdata->player = player;
    pdata->step = 0;
    pdata->field_14 = 0;

    if (mode_of_play == 4 &&
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
    g_game_info.pads[pad].flag_bits.disabled = 1;
}

static inline int pselect_character_id_valid(int char_id) {
    int valid = 0;

    if ((unsigned long long)char_id < 0x2CULL) {
        valid = 1;
    }
    return valid;
}

static inline int pselect_character_locked_impl(int char_id, int alt_bit) {
    unsigned long long mask;
    unsigned long long bit;

    if (!pselect_character_id_valid(char_id)) {
        return 1;
    }

    switch (mode_of_play) {
    case 6:
        mask = gp_data.pz_chars.value;
        mask |= default_pz_char_bits.value;
        break;
    default:
        if (alt_bit != 0) {
            mask = gp_data.cat2.value;
            mask |= default_alt_char_bits.value;
        } else {
            mask = gp_data.cat1.value;
            mask |= default_char_bits.value;
        }
    }

    bit = 1ULL << char_id;
    if ((mask & bit) != 0ull) {
        return 0;
    }
    return 1;
}

/* TODO: [breakthrough] 97.55%; retail table ownership, frame and retained homes recovered;
 * validity branch/join and shift-argument staging need original inline evidence. */
static float p_random_player_select(void) {
    int attempts;
    int slot;
    int char_id;
    struct RandomSelectPdata* pdata;
    int pad;

    pdata = (struct RandomSelectPdata*)apdata;
    if (rnd_sleep_tbl[pdata->step] == -1) {
        fire_screen_studio_event(0x1FB3, pdata->player + 1);
        pad = (&g_game_info.plyr0)[pdata->player].pad_index;
        g_game_info.pads[pad].flag_bits.disabled = 0;
        return sleep_ticks_neg_one;
    }

    attempts = 0;
    slot = randu0(pdata->num_chars & 0xFFFF) & 0xFFFF;
    char_id = pselect_char_id_at(slot);
    while (pselect_character_locked_impl(char_id, 0) || char_id == 0x20) {
        slot = randu0(pdata->num_chars & 0xFFFF) & 0xFFFF;
        char_id = pselect_char_id_at(slot);
        attempts += 1;
        if (attempts > 0x32) {
            slot = 0;
            break;
        }
    }

    pselect_update_selbox_pos(pdata->player, slot);
    return rnd_sleep_tbl[pdata->step++];
}

void award_bet(void) {
    struct PselectProfileView* winner;
    struct PselectProfileView* loser;
    int winner_num;
    int koin;

    koin = wager_koin_order[g_game_info.pselect.field_1d4];
    if (koin < 0 || koin > 6) {
        g_game_info.pselect.field_1d4 = 0;
        g_game_info.pselect.field_1d8 = 0;
        g_game_info.pselect.field_1dc = 0;
        g_game_info.pselect.field_1d0 = 0;
        g_game_info.pselect.field_1e0 = 1;
        g_game_info.pselect.field_1f0 = 1;
        set_default_button_repeat_time();
        wager_completed_ran = 0;
        return;
    }

    if (g_game_info.field_1F8 == 2 &&
        g_game_info.pselect.field_1d0 != 0) {
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
            winner->koins[koin] += g_game_info.pselect.field_1d8 * 2;
            winner->wager_wins[koin] += g_game_info.pselect.field_1d8;
            loser->wager_losses[koin] += g_game_info.pselect.field_1d8;
            g_game_info.pselect.field_1dc = g_game_info.pselect.field_1d8;
        }
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

/* TODO: [near miss] 97.89%; save/refund CFG agrees; refund owners and zero stores use rotated volatile GPRs. */
static float p_wager_save_profiles(void) {
    int p1_saved;
    int p2_saved;

    turn_controllers_off();
    p1_saved = (save_profile)(0, 2);
    p2_saved = (save_profile)(1, 2);
    if (p1_saved == 0 || p2_saved == 0) {
        int amount = g_game_info.pselect.field_1d8;
        if (amount > 0) {
            int koin = wager_koin_order[g_game_info.pselect.field_1d4];
            g_game_info.pselect.field_1d4 = 0;
            g_game_info.pselect.field_1d8 = 0;
            g_game_info.pselect.field_1dc = 0;
            p1_profile.koins[koin] = p1_profile.koins[koin] + amount;
            p2_profile.koins[koin] += amount;
            g_game_info.pselect.field_1d0 = 0;
            g_game_info.pselect.field_1e0 = 1;
            g_game_info.pselect.field_1f0 = 1;
            set_default_button_repeat_time();
            wager_completed_ran = 0;
            game_save_loop_count = 0;
        }
        wager_cancelled();
    }
    turn_controllers_on();
    return sleep_ticks_neg_one;
}

static inline int wager_clamp_bet_to_balances(
    int* p1_koins, int* p2_koins, unsigned int koin) {
    int p1_count;
    int p2_count;

    p1_count = p1_koins[koin];
    g_game_info.pselect.field_1d0 = 1;
    p2_count = p2_koins[koin];
    if (p1_count > 0 && p2_count > 0) {
        if (p1_count > p2_count) {
            p1_count = p2_count;
        }
        if (g_game_info.pselect.field_1d8 > p1_count) {
            g_game_info.pselect.field_1d8 = p1_count;
        }
        return 1;
    }
    return 0;
}

static inline void wager_debit_balances(
    int* p1_koins, int* p2_koins, int koin) {
    if (wager_clamp_bet_to_balances(p1_koins, p2_koins, koin) == 0) {
        wager_cancelled();
        return;
    }

    p1_koins[koin] -= g_game_info.pselect.field_1d8;
    p2_koins[koin] -= g_game_info.pselect.field_1d8;
    proc_create(p_wager_save_profiles, 0x209F);
}

void wager_completed(void) {
    int koin;

    if (wager_completed_ran != 0) {
        return;
    }
    wager_completed_ran = 1;
    set_default_button_repeat_time();

    koin = wager_koin_order[g_game_info.pselect.field_1d4];
    if (koin < 0 || koin > 6) {
        wager_cancelled();
        return;
    }
    if (g_game_info.pselect.field_1d8 == 0) {
        wager_cancelled();
        return;
    }

    wager_debit_balances(p1_profile.koins, p2_profile.koins, koin);
}

int get_current_wager_koin(void) {
    return wager_koin_order[g_game_info.pselect.field_1d4];
}

void ck_decrement_wager_koin_type(void) {
    int p1_count;
    int attempts;
    int available;
    int old_type;

    old_type = g_game_info.pselect.field_1d4;
    attempts = 0;
    do {
        int koin;
        int p2_count;

        g_game_info.pselect.field_1d4 -= 1;
        if (g_game_info.pselect.field_1d4 < 0) {
            g_game_info.pselect.field_1d4 = 5;
        }
        attempts += 1;
        koin = wager_koin_order[g_game_info.pselect.field_1d4];
        p1_count = p1_profile.koins[koin];
        p2_count = p2_profile.koins[koin];
        if (p1_count > 0 && p2_count > 0) {
            if (p1_count > p2_count) {
                p1_count = p2_count;
            }
            if (g_game_info.pselect.field_1d8 > p1_count) {
                g_game_info.pselect.field_1d8 = p1_count;
            }
            available = 1;
        } else {
            available = 0;
        }
    } while (!available && attempts <= 6);

    if (attempts > 6) {
        g_game_info.pselect.field_1d4 = old_type;
    }
}

void ck_increment_wager_koin_type(void) {
    int p1_count;
    int attempts;
    int available;
    int old_type;

    old_type = g_game_info.pselect.field_1d4;
    attempts = 0;
    do {
        int koin;
        int p2_count;

        g_game_info.pselect.field_1d4 += 1;
        if (g_game_info.pselect.field_1d4 >= 6) {
            g_game_info.pselect.field_1d4 = 0;
        }
        attempts += 1;
        koin = wager_koin_order[g_game_info.pselect.field_1d4];
        p1_count = p1_profile.koins[koin];
        p2_count = p2_profile.koins[koin];
        if (p1_count > 0 && p2_count > 0) {
            if (p1_count > p2_count) {
                p1_count = p2_count;
            }
            if (g_game_info.pselect.field_1d8 > p1_count) {
                g_game_info.pselect.field_1d8 = p1_count;
            }
            available = 1;
        } else {
            available = 0;
        }
    } while (!available && attempts <= 6);

    if (attempts > 6) {
        g_game_info.pselect.field_1d4 = old_type;
    }
}

static float p_wager_repeat_process(void) {
    struct WagerRepeatPdata* pdata;

    pdata = (struct WagerRepeatPdata*)apdata;
    pdata->ticks -= 1;
    if (pdata->ticks < 0) {
        g_game_info.pselect.field_1f0 = 1;
        return sleep_ticks_neg_one;
    }
    return sleep_ticks_one;
}

void ck_decrement_bet(void) {
    MkProc* proc;
    MkHdr* pdata;
    int koin;

    koin = get_current_wager_koin();
    if (koin < 0 || koin > 6) {
        return;
    }

    proc = find_mkproc_pid(0x20A4);
    if (proc != 0) {
        g_game_info.pselect.field_1f0 = 1;
        if (proc->instance != 0) {
            proc->vtbl->destroy(proc);
        }
    }

    if (p1_profile_status == 1 && p2_profile_status == 1) {
        if (g_game_info.pselect.field_1d8 > 0) {
            g_game_info.pselect.field_1d8 -= g_game_info.pselect.field_1f0;
            if (g_game_info.pselect.field_1d8 < 0) {
                g_game_info.pselect.field_1d8 = 0;
            }
        }

        proc = find_mkproc_pid(0x20A3);
        if (proc != 0) {
            pdata = pdata_of_proc(proc);
            if (pdata != 0) {
                ((struct WagerRepeatPdata*)pdata)->ticks = 7;
                g_game_info.pselect.field_1f0 += 3;
                if (g_game_info.pselect.field_1f0 < 0x32) {
                    g_game_info.pselect.field_1f0 = 0x32;
                }
                if (g_game_info.pselect.field_1f0 < 0x1E) {
                    g_game_info.pselect.field_1f0 = 7;
                }
            }
        } else {
            g_game_info.pselect.field_1f0 = 1;
            proc = _create_mkproc_generic_tinystack(
                0x20A3, 0x1F, p_wager_repeat_process, sizeof(struct WagerRepeatPdata), &pdata);
            if (proc != 0) {
                ((struct WagerRepeatPdata*)pdata)->ticks = 7;
            }
        }
    }
}

/* TODO: [near miss] 99.40860%; soft ceiling: nine affordability GPR rows remain; preserve lazy profile checks and output-slot ABI. */
void ck_increment_bet(void) {
    MkProc* proc;
    MkHdr* pdata;
    int koin;

    koin = get_current_wager_koin();
    if (koin < 0 || koin > 6) {
        return;
    }

    proc = find_mkproc_pid(0x20A3);
    if (proc != 0) {
        g_game_info.pselect.field_1f0 = 1;
        if (proc->instance != 0) {
            proc->vtbl->destroy(proc);
        }
    }

    if (p1_profile_status == 1 && p2_profile_status == 1) {
        int next_amount =
            g_game_info.pselect.field_1d8 + g_game_info.pselect.field_1f0;
        if ((unsigned int)p1_profile.koins[koin] >= next_amount &&
            (unsigned int)p2_profile.koins[koin] >= next_amount) {
            g_game_info.pselect.field_1d8 = next_amount;
            proc = find_mkproc_pid(0x20A4);
            if (proc != 0) {
                pdata = pdata_of_proc(proc);
                if (pdata != 0) {
                    ((struct WagerRepeatPdata*)pdata)->ticks = 7;
                    g_game_info.pselect.field_1f0 += 3;
                    if (g_game_info.pselect.field_1f0 > 0x32) {
                        g_game_info.pselect.field_1f0 = 3;
                    }
                }
            } else {
                g_game_info.pselect.field_1f0 = 1;
                proc = _create_mkproc_generic_tinystack(
                    0x20A4, 0x1F, p_wager_repeat_process, sizeof(struct WagerRepeatPdata), &pdata);
                if (proc != 0) {
                    ((struct WagerRepeatPdata*)pdata)->ticks = 7;
                }
            }
        }
    }
}

/* TODO: [breakthrough needed] 92.96%; strength reduction creates a separate
 * zero index; recover retail accumulator sharing without tying unrelated locals. */
int ok_to_bring_out_wager_screen(void) {
    int has_common_koin;
    int i;
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

    has_common_koin = 0;
    mop = mode_of_play;
    if (mop != 1 && mop != 0) {
        return 0;
    }
    if (mop == 7 || mop == 4) {
        return 0;
    }

    for (i = 0; i < 6; i++) {
        unsigned int p1_koins = p1_profile.koins[i];
        unsigned int p2_koins = p2_profile.koins[i];

        if (p1_koins != 0 && p2_koins != 0) {
            has_common_koin = 1;
        }
    }
    if (!has_common_koin) {
        return 0;
    }
    if (g_game_info.pselect.field_1d0 != 0) {
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

int pselect_bgnd_has_weapon(void) {
    struct PselectBgndEntry* tbl;
    int max;
    int pos;

    switch (pselect_mode) {
    case 0:
        tbl = pselect_bgnd_tbl;
        max = 0x16;
        break;
    case 1:
        tbl = bg_pselect_bgnd_table;
        max = 5;
        break;
    case 2:
        tbl = pz_pselect_bgnd_table;
        max = 6;
        break;
    default:
        return 0;
    }

    if ((pos = background_selbox_pos) < 0 || pos > max) {
        return 0;
    }
    return tbl[pos].flags & 4;
}

int pselect_bgnd_has_level_transition(void) {
    struct PselectBgndEntry* tbl;
    int max;
    int pos;

    switch (pselect_mode) {
    case 0:
        tbl = pselect_bgnd_tbl;
        max = 0x16;
        break;
    case 1:
        tbl = bg_pselect_bgnd_table;
        max = 5;
        break;
    case 2:
        tbl = pz_pselect_bgnd_table;
        max = 6;
        break;
    default:
        return 0;
    }

    if ((pos = background_selbox_pos) < 0 || pos > max) {
        return 0;
    }
    return tbl[pos].flags & 2;
}

int pselect_bgnd_has_deathtrap(void) {
    struct PselectBgndEntry* tbl;
    int max;
    int pos;

    switch (pselect_mode) {
    case 0:
        tbl = pselect_bgnd_tbl;
        max = 0x16;
        break;
    case 1:
        tbl = bg_pselect_bgnd_table;
        max = 5;
        break;
    case 2:
        tbl = pz_pselect_bgnd_table;
        max = 6;
        break;
    default:
        return 0;
    }

    if ((pos = background_selbox_pos) < 0 || pos > max) {
        return 0;
    }
    return tbl[pos].flags & 1;
}

/* TODO: [near miss] 96.43%; first balance producer improved; profile/zero register web remains. */
void ck_restore_kiddy(void) {
    int amount;

    amount = g_game_info.pselect.field_1d8;
    if (amount > 0) {
        int koin = wager_koin_order[g_game_info.pselect.field_1d4];
        g_game_info.pselect.field_1d4 = 0;
        g_game_info.pselect.field_1d8 = 0;
        g_game_info.pselect.field_1dc = 0;
        p1_profile.koins[koin] = p1_profile.koins[koin] + amount;
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
    proc = _create_mkproc_generic_bigstack(player + 0x9026, 0x1F,
                                           p_enter_profile_code, sizeof(ProfileCodePdata),
                                           (MkHdr**)&pdata);
    eat_switch_edge(port, 2);
    eat_switch_edge(port, 6);
    if (proc == 0) {
        return;
    }

    push_game_state(0x1B);
    zero_pdata_payload(sizeof(ProfileCodePdata), &pdata->hdr);
    plyr = g_game_info.pads[port].player;
    pdata->old_player_state = plyr->player_state;
    set_player_state(g_game_info.pads[port].player, 1);
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
            int digit;
            for (digit = 0; digit < sizeof(pdata->code); digit++) {
                pdata->code[digit] = 0;
            }
            pdata->count = 0;
            profile_code_state[player] = pdata->count;
            if (player == 0) {
                fire_screen_studio_event(0x1FC4, player + 1);
            } else {
                fire_screen_studio_event(0x1FC5, player + 1);
            }
        }
    } else {
        result = load_profile(player, port, pdata->code);
        switch (result) {
        case 0:
            break;
        case 1:
            profile_code_state[player] = 8;
            fire_screen_studio_event(0x1FAA, player + 1);
            break;
        case 2:
            profile_code_state[player] = 7;
            fire_screen_studio_event(0x1FAA, player + 1);
            break;
        case 3:
            profile_code_state[player] = 9;
            fire_screen_studio_event(0x1FAA, player + 1);
            break;
        }

        if (get_game_state() == 0x1B) {
            pop_game_state(0x1B);
        }
        set_player_state(&(&g_game_info.plyr0)[pdata->player], pdata->old_player_state);
        return sleep_ticks_neg_one;
    }
    return sleep_ticks_one;
}


/* TODO: [near miss] 97.91045%; validity join/compare registers remain; return rewrites and whole-TU nopeephole regress callers. */
int is_char_locked(int char_id, int alt_bit) {
    unsigned long long mask;
    unsigned long long bit;

    if (!pselect_character_id_valid(char_id)) {
        return 1;
    }

    switch (mode_of_play) {
    case 6:
        mask = gp_data.pz_chars.value;
        mask |= default_pz_char_bits.value;
        break;
    default:
        if (alt_bit != 0) {
            mask = gp_data.cat2.value;
            mask |= default_alt_char_bits.value;
        } else {
            mask = gp_data.cat1.value;
            mask |= default_char_bits.value;
        }
    }

    bit = 1ULL << (unsigned int)char_id;
    if ((mask & bit) != 0ull) {
        return 0;
    }
    return 1;
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

int pselect_get_arena_index(void) {
    struct PselectBgndEntry* tbl;
    int count;
    int i;

    switch (pselect_mode) {
    case 0:
        tbl = pselect_bgnd_tbl;
        count = 0x16;
        break;
    case 1:
        tbl = bg_pselect_bgnd_table;
        count = 5;
        break;
    case 2:
        tbl = pz_pselect_bgnd_table;
        count = 6;
        break;
    default:
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
    struct PselectBgndEntry* table;
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

    switch (pselect_mode) {
    case 0:
        table = pselect_bgnd_tbl;
        count = 0x16;
        break;
    case 1:
        table = bg_pselect_bgnd_table;
        count = 5;
        break;
    case 2:
        table = pz_pselect_bgnd_table;
        count = 6;
        break;
    default:
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
        if (mode_of_play == 9 || !is_bgnd_locked(force_bgnd_num)) {
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
    switch (pselect_mode) {
    case 0:
        return 0x16;
    case 1:
        return 5;
    case 2:
        return 6;
    default:
        return 0;
    }
}

void get_pz_special_move_list(PselectTexOut out, int use_difficulty) {
    char* name;
    unsigned int i;

    for (i = 0; i < 0xC; i++) {
        if (use_difficulty != 0) {
            name = pselect_pz_char_tbl[i].difficulty;
        } else {
            name = pselect_pz_char_tbl[i].style0;
        }

        if (name != 0) {
            out.colors[i] = load_named_tga_from_slot(PSELECT_SEC_SLOT, name);
            out.alphas[i] =
                load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT, name);
        } else {
            out.colors[i] = 0;
            out.alphas[i] = 0;
        }
    }
}

void get_background_select_textures(PselectTexOut out) {
    int dst;
    int n;
    int i;
    struct PselectBgndEntry* tbl;

    dst = 0;
    switch (pselect_mode) {
    case 0:
        tbl = pselect_bgnd_tbl;
        n = 0x16;
        break;
    case 1:
        tbl = bg_pselect_bgnd_table;
        n = 5;
        break;
    case 2:
        tbl = pz_pselect_bgnd_table;
        n = 6;
        break;
    default:
        return;
    }

    i = 0;
    while (i < n) {
        out.colors[dst] =
            load_named_tga_from_slot(PSELECT_SEC_SLOT, tbl[i].tex_name);
        out.alphas[dst] = load_named_alpha_texture_from_slot(
            PSELECT_SEC_SLOT, tbl[i].tex_name);
        i++;
        dst += 1;
    }
}

void get_pselect_body_textures(PselectTexOut out) {
    int n;
    int i;
    char* name;
    struct PselectCharEntry* tbl;

    if (pselect_mode == 2) {
        n = 0xC;
        tbl = pselect_pz_char_tbl;
    } else {
        n = 0x1B;
        tbl = pselect_char_tbl;
    }

    memset(out.colors, 0, n * sizeof(*out.colors));
    memset(out.alphas, 0, n * sizeof(*out.alphas));

    for (i = 0; i < n; i++) {
        name = tbl[i].body_name;
        if (name != 0) {
            out.colors[i] = load_named_tga_from_slot(PSELECT_SEC_SLOT, name);
            out.alphas[i] =
                load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT, name);
        }
    }

    if (pselect_mode == 0) {
        if (p1_alternate != 0) {
            out.colors[n] = p1_alternate;
            out.alphas[n] = p1_alternate_alpha;
        } else {
            out.colors[n] = out.colors[p1_selbox_pos];
            out.alphas[n] = out.alphas[p1_selbox_pos];
        }
        if (p2_alternate != 0) {
            out.colors[n + 1] = p2_alternate;
            out.alphas[n + 1] = p2_alternate_alpha;
        } else {
            out.colors[n + 1] = out.colors[p2_selbox_pos];
            out.alphas[n + 1] = out.alphas[p2_selbox_pos];
        }
    }
}

int get_num_pselect_body_textures(void) {
    switch (mode_of_play) {
    case 6:
        return 0xC;
    case 9:
        return 0x1B;
    default:
        return 0x1D;
    }
}

static inline RwTexture* pselect_load_team_head_color(int char_id) {
    int slot;

    for (slot = 0; slot < 0x1B; slot++) {
        if (pselect_char_tbl[slot].char_id == char_id) {
            return load_named_tga_from_slot(PSELECT_SEC_SLOT,
                                            pselect_char_tbl[slot].head_name);
        }
    }
    return 0;
}

static inline RwTexture* pselect_load_team_head_alpha(int char_id) {
    int slot;

    for (slot = 0; slot < 0x1B; slot++) {
        if (pselect_char_tbl[slot].char_id == char_id) {
            return load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT,
                                            pselect_char_tbl[slot].head_name);
        }
    }
    return 0;
}

static inline int bg_team_record_focus_of(struct BgPselectTeamPdata* pdata,
                                           int team, int char_id) {
    int i;
    int member_count;

    if (pdata == 0) {
        return -1;
    }
    member_count = pdata->teams[team].entries[0] - 1;
    for (i = 0; i < 5; i++) {
        if (i >= member_count) {
            break;
        }
        if (pdata->teams[team].entries[i + 1] == char_id) {
            return i;
        }
    }
    return -1;
}

static inline void pselect_update_team_preview(int player) {
    struct BgPselectTeamPdata* pdata;
    int sel_pos;
    int char_id;
    int count;
    RwTexture* tex;

    if (pselect_mode != 1) {
        return;
    }
    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }
    count = pdata->teams[player].entries[0];
    sel_pos = p2_selbox_pos;
    if (player == 0) {
        sel_pos = p1_selbox_pos;
    }
    if (pselect_mode == 2) {
        char_id = pselect_pz_char_tbl[sel_pos].char_id;
    } else {
        char_id = pselect_char_tbl[sel_pos].char_id;
    }
    if (count > 0 && count <= 5) {
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT,
                                       pselect_char_tbl[sel_pos].head_name);
        pdata->teams[player].colors[count - 1] = tex;
    }
    pdata->teams[player].focus =
        bg_team_record_focus_of(get_screen_pdata(), player, char_id);
}

void get_bg_pselect_team_textures(PselectTexOut out, int team) {
    struct BgPselectTeamPdata* pdata;
    int i;
    int char_id;

    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    pdata->teams[team].colors = out.colors;
    pdata->teams[team].alphas = out.alphas;

    for (i = 0; i < 5; i++) {
        if (i > pdata->teams[team].entries[0] - 2) {
            out.colors[i] = 0;
            out.alphas[i] = 0;
            continue;
        }
        char_id = pdata->teams[team].entries[i + 1];
        if (char_id != 0x2C) {
            out.colors[i] = pselect_load_team_head_color(char_id);
            out.alphas[i] = pselect_load_team_head_alpha(char_id);
        } else {
            out.colors[i] = 0;
            out.alphas[i] = 0;
        }
    }

    pselect_update_team_preview(team);
}

/* TODO: [near miss] 98.88%; shared validity join differs before allocation;
 * value numbering also merges its zero with the comparison high word. */
void get_pselect_head_textures(PselectTexOut out) {
    int n;
    int i;
    struct PselectCharEntry* tbl;

    if (pselect_mode == 2) {
        n = 0xC;
        tbl = pselect_pz_char_tbl;
    } else {
        n = 0x1B;
        tbl = pselect_char_tbl;
    }

    for (i = 0; i < n; i++) {
        if (is_char_locked(pselect_char_id_at(i), 0)) {
            out.colors[i] = load_named_tga_from_slot(PSELECT_SEC_SLOT, tbl[i].head_lock);
            out.alphas[i] =
                load_named_alpha_texture_from_slot(PSELECT_SEC_SLOT, tbl[i].head_lock);
        } else {
            if (tbl[i].head_name != 0) {
                out.colors[i] = load_named_tga_from_slot(
                    PSELECT_SEC_SLOT, tbl[i].head_name);
                out.alphas[i] = load_named_alpha_texture_from_slot(
                    PSELECT_SEC_SLOT, tbl[i].head_name);
            } else {
                out.colors[i] = 0;
                out.alphas[i] = 0;
            }
        }
    }
}

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

char* pselect_get_arena_name(void) {
    int bgnd_id;

    bgnd_id = 0x23;
    switch (pselect_mode) {
    case 0:
        bgnd_id = pselect_bgnd_tbl[background_selbox_pos].bgnd_id;
        break;
    case 1:
        bgnd_id = bg_pselect_bgnd_table[background_selbox_pos].bgnd_id;
        break;
    case 2:
        bgnd_id = pz_pselect_bgnd_table[background_selbox_pos].bgnd_id;
        break;
    }
    if (bgnd_id != 0x23) {
        return get_string_by_id(
            (unsigned int)global_background_data[bgnd_id].field8 | 0x10000u);
    }
    return 0;
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
    return global_player_data[char_id].name;
}

void pselect_player_moved(int player) {
    struct BgPselectTeamPdata* pdata;
    int sel_pos;
    int char_id;
    int count;
    RwTexture* tex;

    if (pselect_mode != 1) {
        return;
    }
    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }
    count = pdata->teams[player].entries[0];
    sel_pos = p2_selbox_pos;
    if (player == 0) {
        sel_pos = p1_selbox_pos;
    }
    if (pselect_mode == 2) {
        char_id = pselect_pz_char_tbl[sel_pos].char_id;
    } else {
        char_id = pselect_char_tbl[sel_pos].char_id;
    }
    if (count > 0 && count <= 5) {
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT,
                                       pselect_char_tbl[sel_pos].head_name);
        pdata->teams[player].colors[count - 1] = tex;
    }
    pdata->teams[player].focus =
        bg_team_record_focus_of(get_screen_pdata(), player, char_id);
}

/* TODO: [breakthrough needed] 94.48%; count-address and selector-index CSE
 * differ before allocation; recover the original owner/inline boundary. */
void bg_pselect_player_canceled(int player) {
    struct BgPselectTeamPdata* pdata;
    int* entries;
    int count;
    int sel_pos;
    int char_id;
    RwTexture* tex;

    if (pselect_mode != 1) {
        return;
    }
    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    entries = pdata->teams[player].entries;
    count = entries[0];
    sel_pos = (player == 0) ? p1_selbox_pos : p2_selbox_pos;

    if (count >= 1 && count <= 5) {
        entries[count] = 0x2C;
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT, STR_HEAD_PROXY);
        pdata->teams[player].colors[count - 1] = tex;
    }

    if (count >= 2 && count <= 6) {
        tex = load_named_tga_from_slot(PSELECT_SEC_SLOT,
                                       pselect_char_tbl[sel_pos].head_name);
        pdata->teams[player].colors[count - 2] = tex;
        pdata->teams[player].entries[count - 1] = 0x2C;
        char_id = pselect_char_id_at(sel_pos);
        pdata->teams[player].focus =
            bg_team_focus_of(get_screen_pdata(), player, char_id);
    }
}

void pselect_player_canceled(int player) {
    int other_player;
    int start;

    other_player = player == 0;
    if (g_game_info.players[other_player].player_state == 2) {
        return;
    }

    g_game_info.players[player].player_state = 1;
    name_sound_state -= 1;
    g_game_info.players[player].field_14 = 0;

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
    if (mode_of_play == 4) {
        g_game_info.players[other_player].player_state = 0;
    }
    fire_screen_studio_event(0x1FEC, player + 1);
    start = (player == 0) ? p1_selbox_start_pos : p2_selbox_start_pos;
    check_reset_player_selection(player, start);
}

void resolve_alternate_palettes(PlyrInfo* plyr) {
    PlyrInfo* other;
    unsigned int bit;

    other = &g_game_info.plyr0;
    if (plyr == other) {
        other = &g_game_info.plyr1;
    }

    if (g_game_info.plyr0.player_index != g_game_info.plyr1.player_index ||
        g_game_info.plyr0.flags_14_bits.alternate_costume !=
            g_game_info.plyr1.flags_14_bits.alternate_costume) {
        plyr->flags_14_bits.alternate_palette = 0;
        return;
    }

    if (plyr->player_state == 2 && other->player_state == 2) {
        plyr->flags_14_bits.alternate_palette = 1;
        other->flags_14_bits.alternate_palette = 0;
    } else if (plyr->player_state == 0 && other->player_state == 0) {
        bit = randu0(2);
        plyr->flags_14_bits.alternate_palette = bit;
        other->flags_14_bits.alternate_palette = 1 - plyr->flags_14_bits.alternate_palette;
    } else {
        if (plyr->player_state == 0 || plyr->player_state == 3) {
            plyr->flags_14_bits.alternate_palette = 1;
        } else {
            plyr->flags_14_bits.alternate_palette = 0;
        }
        if (other->player_state == 0 || other->player_state == 3) {
            other->flags_14_bits.alternate_palette = 1;
        } else {
            other->flags_14_bits.alternate_palette = 0;
        }
    }
}

/* TODO: [near miss] 99.09%; calls, snapshots and output slots agree;
 * codeless validity join and preview-search/address homes remain. */
void pselect_player_selected(PlyrInfo* plyr) {
    PlyrInfo* other;
    int* sel_pos;
    int studio_ev;
    int char_id;
    int pad;
    int locked;
    int focus;
    PlyrInfoFlags14* flags;
    int flag_word;

    if (plyr->player_state == 2) {
        return;
    }

    if (pselect_mode != 1) {
        if (mode_of_play == 4) {
            if (plyr->field_04 == menu_player) {
                set_player_state(plyr, 2);
                other = &g_game_info.players[0];
                if (plyr == other) {
                    other = &g_game_info.players[1];
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

    char_id = pselect_char_id_at(*sel_pos);
    plyr->player_index = char_id;
    flags = &plyr->flags_14_bits;
    flags->alternate_costume = 0;

    if (mode_of_play == 4) {
        pad = g_game_info.players[menu_player].pad_index;
    } else {
        pad = plyr->pad_index;
    }

    if (pselect_mode == 0 && check_switch(pad, 0xB) != 0) {
        locked = is_char_locked(plyr->player_index, 1);
        if (locked == 0) {
            flags->alternate_costume = 1;
            if (pselect_mode == 0) {
                MkHdr* pdata;
                int proc_id;
                int player;

                player = plyr->field_04;
                proc_id = player + 0x9035;
                destroy_mkprocs_pid(proc_id);
                if (_create_mkproc_generic_bigstack(proc_id, 0x1F,
                                                    p_load_alternate_body, sizeof(struct AltBodyPdata),
                                                    &pdata) != 0) {
                    ((struct AltBodyPdata*)pdata)->player = player;
                }
            }
        }
    }

    resolve_alternate_palettes(plyr);

    if (mode_of_play != 9) {
        flag_word = plyr->field_14;
        load_plyr_model_async(plyr->field_04, plyr->player_index, &flag_word);
    }

    if (pselect_mode == 2) {
        MkHdr* pdata;
        int sound_id = pselect_pz_char_tbl[*sel_pos].name_sound;

        if (_create_mkproc_generic_nostack(0x9029, 0x1F, p_play_name_sound, sizeof(struct NameSoundPdata),
                                           &pdata) != 0) {
            ((struct NameSoundPdata*)pdata)->sound_id = sound_id;
            ((struct NameSoundPdata*)pdata)->countdown = 0xB4;
        }
    } else if (pselect_mode == 0) {
        MkHdr* pdata;
        int sound_id;

        vdebug_print_message(STR_SELECTING_FMT,
                             pselect_char_tbl[*sel_pos].head_name,
                             exec_tick_ctr);
        sound_id = pselect_char_tbl[*sel_pos].name_sound;
        if (_create_mkproc_generic_nostack(0x9029, 0x1F, p_play_name_sound, sizeof(struct NameSoundPdata),
                                           &pdata) != 0) {
            ((struct NameSoundPdata*)pdata)->sound_id = sound_id;
            ((struct NameSoundPdata*)pdata)->countdown = 0xB4;
        }
    }

    if (pselect_mode == 1) {
        char_id = pselect_char_id_at(*sel_pos);
        focus = bg_team_focus_of(get_screen_pdata(),
                                 plyr->field_04, char_id);
        if (focus >= 0) {
            struct BgPselectPdata* preview = get_screen_pdata();

            fire_screen_studio_event(studio_ev, plyr->field_04);
            bg_team_view(preview, plyr->field_04)->focus = focus;
        }
    }
}

/* TODO: [breakthrough needed] 94.02%; initial pcode shares BODY_ALT across calls;
 * recover retail address rematerialization without changing the string pool. */
static float p_load_alternate_body(void) {
    struct AltBodyPdata* pdata;
    int slot;
    RwTexture** color_out;
    RwTexture** alpha_out;
    char* sec_name;

    pdata = (struct AltBodyPdata*)apdata;
    if (pdata->player == 0) {
        color_out = &p1_alternate;
        alpha_out = &p1_alternate_alpha;
        sec_name = pselect_char_tbl[p1_selbox_pos].alt_sec;
        slot = PSELECT_ALT_SLOT_P1;
    } else {
        color_out = &p2_alternate;
        alpha_out = &p2_alternate_alpha;
        sec_name = pselect_char_tbl[p2_selbox_pos].alt_sec;
        slot = PSELECT_ALT_SLOT_P2;
    }

    load_ssf((MkFileEntry*)pselect_file_table);
    unload_section_slot(slot);
    add_art_section_by_name_async(slot, sec_name);
    wait_for_slot_load(slot);

    *color_out = load_named_tga_from_slot(slot, STR_BODY_ALT);
    if (*color_out != 0) {
        *alpha_out = load_named_alpha_texture_from_slot(slot, STR_BODY_ALT);
        refresh_active_screen();
    }
    return sleep_ticks_neg_one;
}

static float p_play_name_sound(void) {
    struct NameSoundPdata* pdata;
    MkVtableMkproc* vtbl;
    MkProcJumpSleepFn jump_sleep;

    pdata = (struct NameSoundPdata*)apdata;
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
    vtbl = aproc->vtbl;
    jump_sleep = vtbl->jump_sleep;
    jump_sleep(p_name_sound_die, sleep_ticks_name);
    return sleep_ticks_name;
}

static float p_name_sound_die(void) {
    name_sound_active = 0;
    name_sound_state += 1;
    return sleep_ticks_neg_one;
}

int pselect_get_body_texture_index(int player) {
    int pos;

    if (pselect_mode == 0) {
        if ((&g_game_info.plyr0)[player].flags_14_bits.alternate_costume != 0) {
            return player == 0 ? 0x1b : 0x1c;
        }
    }
    pos = p2_selbox_pos;
    if (player != 0) {
        return pos;
    }
    return p1_selbox_pos;
}

int pselect_get_selbox_pos(int player) {
    if (player == 0) {
        return p1_selbox_pos;
    }
    if (player == 1) {
        return p2_selbox_pos;
    }
    return 0;
}

/* TODO: [breakthrough needed] 95.98290%; validity false-edge and shift staging differ;
 * PS2 outlines the lock call; recover the original GC predicate/inline boundary. */
void pselect_update_selbox_pos(int player, int new_pos) {
    int* pos_p;
    int cols;
    int n_cells;
    int skips;
    int delta;
    int char_id;
    int row2;
    int second_row_end;
    int abs_delta;

    if (player == 0) {
        if (mode_of_play != 4 && g_game_info.plyr0.player_state == 0) {
            return;
        }
        pos_p = &p1_selbox_pos;
    } else if (player == 1) {
        if (mode_of_play != 4 && g_game_info.plyr1.player_state == 0) {
            return;
        }
        pos_p = &p2_selbox_pos;
    } else {
        return;
    }

    switch (mode_of_play) {
    case 6:
        cols = 6;
        n_cells = 0xC;
        break;
    default:
        cols = 9;
        n_cells = 0x1B;
        break;
    }

    skips = 0;
    char_id = pselect_char_id_at(new_pos);
    row2 = cols * 2;
    delta = new_pos - *pos_p;
    second_row_end = row2 - 1;

    while (is_char_locked(char_id, 0)) {
        if (char_id == 0x20) {
            break;
        }
        if (delta == 1 && (*pos_p == cols - 1 || *pos_p == second_row_end ||
                           *pos_p == cols * 3 - 1)) {
            new_pos = *pos_p - (cols - 1);
            *pos_p = new_pos;
            delta = 1;
        } else {
            abs_delta = delta < 0 ? -delta : delta;
            if (row2 == abs_delta) {
                new_pos = *pos_p + delta;
                *pos_p = new_pos;
                if (delta > 0) {
                    delta = -cols;
                } else {
                    delta = cols;
                }
            } else if (delta == -1 &&
                       (*pos_p == 0 || *pos_p == cols || *pos_p == row2)) {
                new_pos = cols + *pos_p;
                new_pos -= 1;
                *pos_p = new_pos;
            } else {
                new_pos = *pos_p + delta;
                *pos_p = new_pos;
            }
        }

        if (new_pos >= n_cells) {
            new_pos = new_pos - n_cells;
        } else if (new_pos < 0) {
            if (delta == -cols) {
                new_pos = new_pos + n_cells;
            } else {
                new_pos = cols;
            }
        }

        skips += 1;
        if (skips > 5) {
            skips = 0;
            *pos_p = *pos_p + 1;
            if (new_pos >= n_cells) {
                new_pos = 0;
            }
        }

        char_id = pselect_char_id_at(new_pos);
    }

    *pos_p = new_pos;
    if (player == 0) {
        g_game_info.plyr0.player_index = pselect_char_id_at(new_pos);
    } else if (player == 1) {
        g_game_info.plyr1.player_index = pselect_char_id_at(new_pos);
    }
    fire_screen_studio_event(0x1FA4, player);
}

/* TODO: [near miss] 98.37%; shared inlined character-validity compare and redundant join branch remain. */
void check_reset_player_selection(int player, int start_pos) {
    int pos;
    int char_id;

    if (g_game_info.players[player].player_state != 1) {
        return;
    }

    pos = p2_selbox_pos;
    if (player == 0) {
        pos = p1_selbox_pos;
    }
    char_id = pselect_char_id_at(pos);
    if (is_char_locked(char_id, 0)) {
        pselect_update_selbox_pos(player, start_pos);
    }
}

#pragma opt_propagation off
int bg_pselect_get_stage(int team) {
    struct BgPselectPdata* pdata;

    pdata = get_screen_pdata();
    if (pdata != 0) {
        return bg_team_view(pdata, team)->count;
    }
    return 0;
}

int bg_pselect_get_offender_class(int team) {
    struct BgPselectPdata* pdata;

    pdata = get_screen_pdata();
    if (pdata != 0) {
        return bg_team_view(pdata, team)->focus;
    }
    return -1;
}

#pragma opt_propagation reset

/* TODO: [breakthrough] 91.83%; shared focus bound recovered; team root/index staging and register homes remain. */
void bg_pselect_set_character(int team) {
    struct BgPselectPdata* pdata;
    struct BgPselectTeamView* teamv;
    int sel_pos;
    int char_id;

    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    sel_pos = p2_selbox_pos;
    if (team == 0) {
        sel_pos = p1_selbox_pos;
    }
    teamv = bg_team_view(pdata, team);
    if (teamv->count >= 1 && teamv->count <= 5) {
        char_id = pselect_char_id_at(sel_pos);
        teamv->chars[teamv->count - 1] = char_id;
        bg_team_view(pdata, team)->focus = bg_team_focus_of(
            get_screen_pdata(), team, char_id);
    }
}

#pragma opt_propagation off
void bg_pselect_set_stage(int team, int stage) {
    struct BgPselectPdata* pdata;
    struct BgPselectTeamView* teamv;
    PlyrInfo* plyr;

    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    plyr = &g_game_info.plyr1;
    if (team == 0) {
        plyr = &g_game_info.plyr0;
    }
    teamv = bg_team_view(pdata, team);
    teamv->count = stage;
    if (stage == 6) {
        set_player_state(plyr, 2);
    }
}

#pragma opt_propagation reset

/* TODO: [breakthrough needed] 93.50%; copy-owner address is reused before count store;
 * recover retail rematerialization boundary without changing shared team layout. */
void bg_pselect_save_team(int team) {
    struct BgPselectPdata* pdata;
    struct PselectProfileView* profile;
    MkHdr* save_pdata;
    MkProc* proc;
    int i;

    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }

    if (team == 0) {
        if (p1_profile_status != 1) {
            goto failed;
        }
        profile = &p1_profile;
    } else {
        if (p2_profile_status != 1) {
            goto failed;
        }
        profile = &p2_profile;
    }

    for (i = 0; i < 5; i++) {
        profile->bg_team[i] = bg_team_view(pdata, team)->chars[i];
    }
    profile->bg_team_valid = 1;
    bg_team_view(pdata, team)->count = 6;

    proc = _create_mkproc_generic_bigstack(
        0x902A, 0x1F, p_save_bg_profile, sizeof(struct BgSaveProfilePdata), &save_pdata);
    if (proc == 0) {
        fire_screen_studio_event(team + 0x1FDD, team + 1);
    } else {
        ((struct BgSaveProfilePdata*)save_pdata)->player = team;
    }
    return;

failed:
    fire_screen_studio_event(team + 0x1FDD, team + 1);
}

static float p_save_bg_profile(void) {
    struct BgSaveProfilePdata* pdata;
    int player;
    int saved;

    pdata = (struct BgSaveProfilePdata*)apdata;
    player = pdata->player;
    saved = (save_profile)(player, 1);
    _mkproc_sleep_ticks = sleep_ticks_one;
    aproc->vtbl->sleep();
    if (saved != 0) {
        fire_screen_studio_event(player + 0x1FC8, player + 1);
    } else {
        fire_screen_studio_event(player + 0x1FDD, player + 1);
    }
    return sleep_ticks_neg_one;
}

void bg_pselect_load_team(int team) {
    struct BgPselectTeamPdata* pdata;
    struct PselectProfileView* profile;
    int i;

    pdata = get_screen_pdata();
    if (pdata == 0) {
        return;
    }
    if (team == 0) {
        if (p1_profile_status != 1 || p1_profile.bg_team_valid == 0) {
            goto failed;
        }
        profile = &p1_profile;
    } else {
        if (p2_profile_status != 1 || p2_profile.bg_team_valid == 0) {
            goto failed;
        }
        profile = &p2_profile;
    }
    for (i = 0; i < 5; i++) {
        pdata->teams[team].entries[i + 1] = profile->bg_team[i];
    }
    pdata->teams[team].entries[0] = 6;
    fire_screen_studio_event(team + 0x1FE1, team + 1);
    return;

failed:
    fire_screen_studio_event(team + 0x1FDF, team + 1);
}

float p_bg_pselect(void) {
    GameInfo* gi;
    struct ChessDefInfo* chess;
    int* chess_team0;
    int* chess_team1;
    struct BgPselectPdata* pdata;
    int n_selecting;
    int n_done;
    int wait;
    int i;

    turn_controllers_off();
    pselect_mode = 1;
    pselect_init();
    push_game_state(4);

    pdata = (struct BgPselectPdata*)get_mkpdata_generic(sizeof(*pdata));
    if (pdata == 0) {
        gamelogic_jump(6, p_main_menu);
    }
    mk_insert(&pdata->hdr, &aproc->pdata_list);
    zero_pdata_payload(sizeof(*pdata), &pdata->hdr);

    for (i = 0; i < 5; i++) {
        pdata->team0[i] = 0x2C;
        pdata->team1[i] = 0x2C;
    }
    pdata->focus0 = -1;
    pdata->focus1 = -1;

    unload_section_slot(PSELECT_SEC_SLOT);
    load_ssf((MkFileEntry*)pselect_file_table);
    load_art_section_by_name(PSELECT_SEC_SLOT, STR_BG_ART);
    load_screen(STR_BG_SELECT, PSELECT_SEC_SLOT, &pdata->hdr, 0);
    turn_camera_on();
    turn_controllers_on();

    if ((g_game_info.field_04 >> 7) & 1) {
        ck_for_controller_removed();
    }

    gi = &g_game_info;
    chess = &g_chess_definition_info;
    chess_team0 = chess->team0;
    chess_team1 = chess->team1;
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
            if (mode_of_play != 9) {
                mkproc_sleep_one();
                if (gi->plyr0.player_state == 2 || gi->plyr0.player_state == 3) {
                    n_done = 1;
                }
                if (gi->plyr1.player_state == 2 || gi->plyr1.player_state == 3) {
                    n_done += 1;
                }
                while (wait > 0 && name_sound_state < n_done) {
                    wait -= 1;
                    mkproc_sleep_one();
                }
            }

            fire_screen_studio_event(0x1FE7, 0);
            wait_for_screen_close();

            if (target_game_mode == 3) {
                int p1_empty;
                int p2_empty;

                pop_game_state();
                memcpy(chess_team0, pdata->team0, sizeof(pdata->team0));
                memcpy(chess_team1, pdata->team1, sizeof(pdata->team1));
                p1_empty = (gi->plyr0.player_state == 0);
                p2_empty = (gi->plyr1.player_state == 0);
                chess->ready = 1;
                chess->p1_empty = p1_empty;
                chess->p2_empty = p2_empty;
                g_chess_definition_info.bgnd_num = force_bgnd_num;
            }
            if (target_game_mode == 3) {
                gamelogic_jump(5, p_mk_chess);
            }
        }

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
float p_pz_pselect(void) {
    int bgnd;
    int n_selecting;
    int n_done;
    int wait;
    GameInfo* gi;
    PlyrInfoFlags14* f0;
    PlyrInfoFlags14* f1;

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

    if ((g_game_info.field_04 >> 7) & 1) {
        ck_for_controller_removed();
    }

    gi = &g_game_info;

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
            if (mode_of_play != 9) {
                mkproc_sleep_one();
                if (gi->plyr0.player_state == 2 || gi->plyr0.player_state == 3) {
                    n_done = 1;
                }
                if (gi->plyr1.player_state == 2 || gi->plyr1.player_state == 3) {
                    n_done += 1;
                }
                while (wait > 0 && name_sound_state < n_done) {
                    wait -= 1;
                    mkproc_sleep_one();
                }
            }

            fire_screen_studio_event(0x1FE7, 0);
            wait_for_screen_close();

            if (target_game_mode == 1) {
                f0 = &gi->plyr0.flags_14_bits;
                f1 = &gi->plyr1.flags_14_bits;
                f0->alternate_costume = 0;
                f1->alternate_costume = 0;
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

float p_pselect(void) {
    GameInfo* gi;
    PlyrInfo* plyr1;
    PlyrInfo* plyr0;
    GcPadFlags* pad2;
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

    if (mode_of_play == 4) {
        disable_all_ports_but_me((&g_game_info.plyr0)[menu_player].pad_index);
    }

    load_ssf((MkFileEntry*)pselect_file_table);
    load_art_section_by_name(PSELECT_SEC_SLOT, STR_PSELECT_ART);

    if (mode_of_play == 4) {
        load_screen(STR_P_PRACTICE, PSELECT_SEC_SLOT, 0, 0);
    } else {
        load_screen(STR_P_SELECT, PSELECT_SEC_SLOT, 0, 0);
    }
    turn_controllers_on();
    turn_camera_on();

    if ((g_game_info.field_04 >> 7) & 1) {
        ck_for_controller_removed();
    }

    gi = &g_game_info;
    plyr1 = &gi->plyr1;
    plyr0 = &gi->plyr0;
    pad2 = &gi->pads[2].flag_bits;

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
            if (mode_of_play == 0) {
                turn_controllers_off();
            }

            wait = 0xB4;
            n_done = 0;
            if (mode_of_play != 9) {
                mkproc_sleep_one();
                if (gi->plyr0.player_state == 2 || gi->plyr0.player_state == 3) {
                    n_done = 1;
                }
                if (gi->plyr1.player_state == 2 || gi->plyr1.player_state == 3) {
                    n_done += 1;
                }
                while (wait > 0 && name_sound_state < n_done) {
                    wait -= 1;
                    mkproc_sleep_one();
                }
            }

            fire_screen_studio_event(0x1FE7, 0);
            wait_for_screen_close();
            pop_game_state();

            if (target_game_mode == 0) {
                turn_controllers_off();
                gi->plyr0.field_10 = (float)psel_p1_handicap / 100.0f;
                gi->plyr1.field_10 = (float)psel_p2_handicap / 100.0f;

                switch (mode_of_play) {
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
                    unassign_player(menu_player == 0 ? plyr1 : plyr0);
                    pad2->connected = 1;
                    assign_player(2);
                    pad2->connected = 0;
                    gamelogic_jump(2, p_gamelogic);
                    break;
                }
            }
        }

        if (target_game_mode == 5) {
            wait_for_screen_close();
            amount = gi->pselect.field_1d8;
            if (amount > 0) {
                order = wager_koin_order[gi->pselect.field_1d4];
                p1_koin = &p1_profile.koins[order];
                p2_koin = &p2_profile.koins[order];
                *p1_koin += amount;
                *p2_koin += amount;
                gi->pselect.field_1d4 = 0;
                gi->pselect.field_1d8 = 0;
                gi->pselect.field_1dc = 0;
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

static void init_startup_selboxes(void);

/* TODO: [near miss] 97.37%; separate player search positions agree; shared validity join and unlock-mask GPR homes remain. */
static void init_startup_selboxes(void) {
    int max_col;
    int p1_pos;
    int p2_pos;
    int mop;

    mop = mode_of_play;
    p1_selbox_start_pos = 2;
    p2_selbox_start_pos = 6;
    if (mop == 6) {
        p1_selbox_start_pos = 0;
        p2_selbox_start_pos = 5;
    }

    switch (mop) {
    case 6:
        max_col = 6;
        break;
    default:
        max_col = 9;
        break;
    }

    p1_pos = p1_selbox_start_pos;
    while (is_char_locked(pselect_char_id_at(p1_pos), 0)) {
        p1_pos += 1;
        if (p1_pos > max_col) {
            p1_pos = 0;
        }
    }
    p1_selbox_start_pos = p1_pos;
    p1_selbox_pos = p1_pos;
    if (g_game_info.plyr0.player_state == 1) {
        fire_screen_studio_event(0x1FA4, 0);
    }

    p2_pos = p2_selbox_start_pos;
    while (is_char_locked(pselect_char_id_at(p2_pos), 0)) {
        p2_pos -= 1;
        if (p2_pos < 0) {
            p2_pos = max_col;
        }
    }
    p2_selbox_start_pos = p2_pos;
    p2_selbox_pos = p2_pos;
    if (g_game_info.plyr1.player_state == 1) {
        fire_screen_studio_event(0x1FA4, 1);
    }
}

static void pselect_init(void) {
    init_startup_selboxes();
    set_section_memory_scheme(0xA);
    set_menu_mode(-1);
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

    if (mode_of_play == 4) {
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

    camera_obj->pos.z = 7.0f;
    camera_obj->ang.y = 3.1415927f;

    g_game_info.bgnd_id = 0x23;
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
