#include "game/game_info.h"
#include "game/attract.h"
#include "game/plyrprofile.h"
#include "game/menu.h"
#include "game/moveset.h"
#include "game/settings.h"

#include "mw/mwScreenEngineGlue.h"
#include "platform/io.h"
#include "platform/main.h"
#include "platform/main_jump.h"
#include "runtime/mk_proc.h"
#include "runtime/fonts.h"
#include "runtime/image.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/section.h"
#include "runtime/utils.h"

extern void fade_to_black(int ticks, int sleep);
extern void save_both_profiles(int mode);
extern int trial_check_state(void);
typedef struct GameObjectLatch {
    MkHdr* obj;
    unsigned int obj_instance;
} GameObjectLatch;

const char* mk6_version_string = "0.142_gc";
int b_game_timer_off = 1;

static int continue_timer;
GameObjectLatch df_press_start_item;
GameObjectLatch df_press_start_proc_item;
int game_loop_count;
int game_save_loop_count;
int winner;
int force_bgnd_num;
int f_fatality_finished;
int f_fatality_available;
int f_fatality_was_done;
GameObjectLatch game_timer_item;
static unsigned int start_time;
static int game_time_tick;
static int last_timer_sec;
int round_winner;
unsigned long display_off;
int g_GameLossesInARow;

extern void destroy_pwr_bars(void);
extern int trial_get_round_length(void);
extern int screen_width;
extern int screen_height;
extern int snd_req(int sound_id);
extern int force_midpoint_calculation_update;
extern int target_game_mode;
extern int pause_player;
extern float p_pselect(void);
extern float p_pz_pselect(void);
extern float p_main_menu(void);
extern void move_player(MkObj* object, const Vec* position, const Vec* angles);
extern void show_player(PlyrPdata* player);
extern void bgnd_swap_level(int level);
int sprintf(char* dst, const char* format, ...);
static char timer_string[10];
GlobalMoveset global_movesets[GLOBAL_MOVESET_COUNT];
GameInfo g_game_info;
int update_game_timer(void);
void* memset(void* dest, int value, unsigned long size);

typedef union GameInfoInitPrefix {
    int word;
    struct {
        unsigned char pad_7 : 1;
        unsigned char flag_6 : 1;
        unsigned char pad_5_0 : 6;
    } bits;
} GameInfoInitPrefix;

typedef struct GameProcVtable {
    int (*reserved[6])(void);
    int (*sleep)(void*);
} GameProcVtable;

typedef struct JoinProcVtable {
    int (*reserved[6])(void);
    int (*sleep)(void* vtbl);
} JoinProcVtable;

typedef struct GameLoopProcVtable {
    int (*reserved[6])(void);
    int (*sleep)(void* vtbl);
    int (*reserved_1c[2])(void);
    int (*jump_sleep)(MkProcEntryFn entry, void* vtbl, float ticks);
} GameLoopProcVtable;

typedef struct BgndAnimationsView {
    char pad00[0x78];
    void* intro_path;
} BgndAnimationsView;

typedef struct JoinInPdata {
    MkHdr hdr;
    int player;
} JoinInPdata;

typedef struct DamageTextPdata {
    MkHdr hdr;
    int player;
    float x;
    float y;
    int font_size;
    char text[0x1C];
} DamageTextPdata;

typedef struct DamageStringVtable {
    int (*reserved[4])(void);
    int (*destroy)(StringObj* string, void* vtbl);
} DamageStringVtable;

typedef struct RoundStartPositions {
    char pad00[0xC];
    Vec player1_position;
    Vec player1_angles;
    Vec player2_position;
    Vec player2_angles;
} RoundStartPositions;

typedef struct JoinProcFlagsView {
    char pad00[0xA8];
    union {
        unsigned char flags_A8;
        struct {
            unsigned char pad_7_4 : 4;
            unsigned char bit3 : 1;
            unsigned char pad_2_0 : 3;
        } flag_bits;
    };
} JoinProcFlagsView;

typedef union JoinByteFlags {
    unsigned char raw;
    struct {
        unsigned char bit7 : 1;
        unsigned char bit6 : 1;
        unsigned char bit5 : 1;
        unsigned char bit4 : 1;
        unsigned char bit3 : 1;
        unsigned char pad_2_0 : 3;
    } bits;
} JoinByteFlags;

typedef struct GameFightView {
    char pad00[0x1F4];
    int round_number;
} GameFightView;

typedef struct FatalityAvailability {
    unsigned int field_00;
    unsigned int field_04;
    char pad08[0x14];
    unsigned int alternate_field_1C;
} FatalityAvailability;

typedef struct FighterStatusTable {
    char pad00[0x84];
    FatalityAvailability* fatality;
} FighterStatusTable;

typedef struct FinishHimPdata {
    MkHdr hdr;
    int alternate_voice;
    char pad_0C[0x1C];
} FinishHimPdata;

typedef struct EndingTiming {
    int character_id;
    float standard;
    float alternate;
    float defeated;
    float fatality_intro;
    float fatality_hold;
    float normal_intro;
    float normal_hold;
    float normal_tail;
} EndingTiming; /* 0x24 */

typedef struct DeathtrapEndingTiming {
    int arena_id;
    float intro;
    float hold;
    float tail;
} DeathtrapEndingTiming;

typedef struct KonquestLoadingPdataView {
    char pad00[0x158];
    unsigned int region;
} KonquestLoadingPdataView;

typedef struct KonquestLoadingSaveView {
    char pad00[0xC];
    unsigned int region;
} KonquestLoadingSaveView;

typedef struct LoadingScreenEntry {
    MkFileInfo* section;
    char* left_image;
    char* right_image;
} LoadingScreenEntry;

typedef struct LoadScreenPdata {
    MkHdr hdr;
    int image_index;
    float file_count;
    int section_slot;
    LoadingScreenEntry* table;
    ScreenObj* meter;
    unsigned int meter_instance;
} LoadScreenPdata;

typedef struct GameProfileRoundStats {
    char pad00[0x1C];
    int arcade_wins;
    int arcade_losses;
    int versus_wins;
    int versus_losses;
    int online_wins;
    int online_losses;
    int ladder_completions;
} GameProfileRoundStats;

extern int is_big_boss(PlyrPdata* pdata);
extern int get_blood_level(void);
extern int snd_req_delay(int sound_id, int ticks);
extern void init_pwr_bars(void);
extern void turn_switch_log_on(void);
extern void trial_game_init(void);
extern void show_fighting_style(int style, int player);
extern void bleed_startup(void);
extern void create_wall_monitor(void);
extern void reset_camera_paths(void);
extern void screen_engine_cleanup(void);
extern void wait_for_slot_load();
extern float game_speed;
extern int go_into_major_pain_please;
extern int go_into_twitch_death_please;
extern GameProfileRoundStats p1_profile;
extern GameProfileRoundStats p2_profile;
extern int p1_profile_status;
extern int p2_profile_status;
EndingTiming plyr_ending_timings[] = {
    {7, 825.0f, 400.0f, 350.0f, 120.0f, 60.0f, 120.0f, 5.0f, 80.0f},
    {1, 590.0f, 370.0f, 400.0f, 120.0f, 60.0f, 120.0f, 5.0f, 120.0f},
    {10, 1100.0f, 1030.0f, 350.0f, 120.0f, 60.0f, 120.0f, 5.0f, 120.0f},
    {24, 550.0f, 500.0f, 400.0f, 120.0f, 60.0f, 120.0f, 5.0f, 70.0f},
    {22, 630.0f, 600.0f, 250.0f, 120.0f, 60.0f, 120.0f, 5.0f, 70.0f},
    {6, 700.0f, 650.0f, 580.0f, 120.0f, 60.0f, 120.0f, 5.0f, 90.0f},
    {30, 592.0f, 600.0f, 495.0f, 120.0f, 60.0f, 120.0f, 5.0f, 140.0f},
    {11, 465.0f, 560.0f, 430.0f, 120.0f, 60.0f, 120.0f, 5.0f, 80.0f},
    {21, 500.0f, 550.0f, 330.0f, 120.0f, 60.0f, 120.0f, 5.0f, 70.0f},
    {19, 500.0f, 500.0f, 330.0f, 150.0f, 60.0f, 120.0f, 5.0f, 140.0f},
    {12, 780.0f, 650.0f, 600.0f, 120.0f, 60.0f, 120.0f, 5.0f, 80.0f},
    {18, 700.0f, 400.0f, 360.0f, 120.0f, 60.0f, 120.0f, 5.0f, 100.0f},
    {20, 770.0f, 430.0f, 500.0f, 120.0f, 60.0f, 120.0f, 5.0f, 135.0f},
    {9, 740.0f, 430.0f, 350.0f, 120.0f, 60.0f, 120.0f, 5.0f, 50.0f},
    {16, 490.0f, 760.0f, 400.0f, 120.0f, 60.0f, 120.0f, 5.0f, 120.0f},
    {4, 830.0f, 570.0f, 400.0f, 120.0f, 60.0f, 120.0f, 5.0f, 120.0f},
    {5, 550.0f, 500.0f, 450.0f, 120.0f, 60.0f, 120.0f, 5.0f, 60.0f},
    {27, 550.0f, 930.0f, 380.0f, 160.0f, 60.0f, 120.0f, 5.0f, 120.0f},
    {23, 370.0f, 390.0f, 330.0f, 120.0f, 60.0f, 120.0f, 5.0f, 120.0f},
    {0, 370.0f, 850.0f, 350.0f, 150.0f, 60.0f, 120.0f, 5.0f, 140.0f},
    {31, 454.0f, 702.0f, 518.0f, 120.0f, 60.0f, 120.0f, 5.0f, 140.0f},
    {25, 760.0f, 580.0f, 550.0f, 160.0f, 60.0f, 120.0f, 5.0f, 110.0f},
    {8, 420.0f, 600.0f, 250.0f, 120.0f, 60.0f, 120.0f, 5.0f, 120.0f},
    {14, 650.0f, 690.0f, 390.0f, 120.0f, 60.0f, 120.0f, 5.0f, 70.0f},
    {3, 700.0f, 860.0f, 330.0f, 150.0f, 60.0f, 120.0f, 5.0f, 40.0f},
    {15, 530.0f, 560.0f, 550.0f, 120.0f, 60.0f, 120.0f, 5.0f, 70.0f},
    {-1, 1500.0f, 1500.0f, 1500.0f, 120.0f, 60.0f, 120.0f, 5.0f, 140.0f},
};

DeathtrapEndingTiming deathtrap_ending_timings[] = {
    {6, 120.0f, 10.0f, 60.0f}, {7, 120.0f, 10.0f, 150.0f},
    {8, 120.0f, 90.0f, 60.0f}, {9, 120.0f, 90.0f, 60.0f},
    {10, 120.0f, 90.0f, 60.0f}, {11, 120.0f, 90.0f, 60.0f},
    {12, 120.0f, 90.0f, 60.0f}, {13, 120.0f, 10.0f, 150.0f},
    {14, 120.0f, 90.0f, 60.0f}, {15, 120.0f, 10.0f, 115.0f},
    {16, 120.0f, 10.0f, 250.0f}, {17, 120.0f, 10.0f, 185.0f},
    {18, 120.0f, 90.0f, 60.0f}, {19, 120.0f, 90.0f, 60.0f},
    {20, 120.0f, 90.0f, 60.0f}, {21, 120.0f, 90.0f, 60.0f},
    {22, 120.0f, 90.0f, 60.0f}, {23, 120.0f, 10.0f, 70.0f},
    {24, 120.0f, 10.0f, 170.0f}, {25, 120.0f, 90.0f, 60.0f},
    {26, 120.0f, 90.0f, 60.0f}, {-1, 120.0f, 90.0f, 60.0f},
};

extern MkFileInfo sec_loading_ashrah;
extern MkFileInfo sec_loading_ba_growl;
extern MkFileInfo sec_loading_ba_logo;
extern MkFileInfo sec_loading_ba_speced_2;
extern MkFileInfo sec_loading_ba_speced;
extern MkFileInfo sec_loading_baraka;
extern MkFileInfo sec_loading_boraicho;
extern MkFileInfo sec_loading_closeup;
extern MkFileInfo sec_loading_dk_closeup;
extern MkFileInfo sec_loading_dk_kobra;
extern MkFileInfo sec_loading_dk_throne;
extern MkFileInfo sec_loading_dk_tunnel;
extern MkFileInfo sec_loading_ermac;
extern MkFileInfo sec_loading_hotaru;
extern MkFileInfo sec_loading_kenshi;
extern MkFileInfo sec_loading_kenshi_throw;
extern MkFileInfo sec_loading_kick;
extern MkFileInfo sec_loading_mil_speced;
extern MkFileInfo sec_loading_mileena;
extern MkFileInfo sec_loading_nightwolf;
extern MkFileInfo sec_loading_qc_closeup2;
extern MkFileInfo sec_loading_ra_determined;
extern MkFileInfo sec_loading_ra_lightningstream;
extern MkFileInfo sec_loading_rd_speced_01;
extern MkFileInfo sec_loading_rd_speced_02;
extern MkFileInfo sec_loading_sai;
extern MkFileInfo sec_loading_sc_flamesword;
extern MkFileInfo sec_loading_sc_speced_01;
extern MkFileInfo sec_loading_sc_speced_02;
extern MkFileInfo sec_loading_scorpion;
extern MkFileInfo sec_loading_sindel;
extern MkFileInfo sec_loading_subzero;
extern MkFileInfo sec_loading_sz_icewall;
extern MkFileInfo sec_loading_trio_united;
extern MkFileInfo sec_loading_undeadarmy;
extern MkFileInfo sec_loading_limei;
extern MkFileInfo sec_ba_armed;
extern MkFileInfo sec_ba_closeup;
extern MkFileInfo sec_mil_kick;
extern MkFileInfo sec_mil_closeup_flip;
extern MkFileInfo sec_ba_blades;
extern MkFileInfo sec_mil_peering;
extern MkFileInfo sec_mil_sai;
extern MkFileInfo sec_loading_earthrealm1, sec_loading_earthrealm2;
extern MkFileInfo sec_loading_earthrealm3, sec_loading_earthrealm4;
extern MkFileInfo sec_load_er_shangtsung_02, sec_load_er_portal_01;
extern MkFileInfo sec_load_nx_portals_00, sec_load_nx_monster_00;
extern MkFileInfo sec_load_nx_kamidogus_00;
extern MkFileInfo sec_loading_or_panoramic_00, sec_loading_or_aerial_00;
extern MkFileInfo sec_loading_or_jail_02, sec_loading_or_riot_01;
extern MkFileInfo sec_load_cr_noob_02, sec_load_cr_redcore_00;
extern MkFileInfo sec_load_cr_teleport_03, sec_load_cr_havik_01;
extern MkFileInfo sec_loading_ow_capture_03, sec_loading_ow_tarkatas_00;
extern MkFileInfo sec_loading_ow_shao_01, sec_load_ow_faceoff_00;
extern MkFileInfo sec_loading_ow_kano_02;
extern MkFileInfo sec_load_nr_ermac_01, sec_load_nr_skulls_00;
extern MkFileInfo sec_load_nr_town_00, sec_load_nr_ashrah_02;
extern MkFileInfo sec_load_ed_palace_00, sec_load_ed_entrance_00;
extern MkFileInfo sec_load_ed_tanya_01, sec_load_ed_knighted_02;
extern MkFileInfo sec_chess_load, sec_loading_chess_ash_bo;
extern MkFileInfo sec_loading_chess_ashrah, sec_loading_chess_barmil;
extern MkFileInfo sec_loading_chess_bomb, sec_loading_chess_dr_vs_kb;
extern MkFileInfo sec_loading_chess_er_cs, sec_loading_chess_sindel;
extern MkFileInfo sec_loading_chess_sc_sz, sec_loading_chess_nightwolf;
extern MkFileInfo sec_loading_chess_sz_scorp, sec_loading_chess_nw_br;
extern MkFileInfo sec_loading_puzzle_br_vs_mil, sec_loading_puzzle_br_nw;
extern MkFileInfo sec_loading_puzzle_er_vs_as, sec_loading_puzzle_er_vs_sz;
extern MkFileInfo sec_loading_puzzle_ho_vs_sc, sec_loading_puzzle_snake;

LoadingScreenEntry loading_fight_pic_tbl[] = {
    {&sec_loading_ashrah, (char*)0x004C0000, (char*)0x004C0001},
    {&sec_loading_ba_growl, (char*)0x004D0000, (char*)0x004D0001},
    {&sec_loading_ba_logo, (char*)0x004E0000, (char*)0x004E0001},
    {&sec_loading_ba_speced_2, (char*)0x004F0000, (char*)0x004F0001},
    {&sec_loading_ba_speced, (char*)0x00500000, (char*)0x00500001},
    {&sec_loading_baraka, (char*)0x00510000, (char*)0x00510001},
    {&sec_loading_boraicho, (char*)0x00520000, (char*)0x00520001},
    {&sec_loading_closeup, (char*)0x00530000, (char*)0x00530001},
    {&sec_loading_dk_closeup, (char*)0x00540000, (char*)0x00540001},
    {&sec_loading_dk_kobra, (char*)0x00550000, (char*)0x00550001},
    {&sec_loading_dk_throne, (char*)0x00560000, (char*)0x00560001},
    {&sec_loading_dk_tunnel, (char*)0x00570000, (char*)0x00570001},
    {&sec_loading_ermac, (char*)0x00580000, (char*)0x00580001},
    {&sec_loading_hotaru, (char*)0x00590000, (char*)0x00590001},
    {&sec_loading_kenshi, (char*)0x005A0000, (char*)0x005A0001},
    {&sec_loading_kenshi_throw, (char*)0x005B0000, (char*)0x005B0001},
    {&sec_loading_kick, (char*)0x005C0000, (char*)0x005C0001},
    {&sec_loading_mil_speced, (char*)0x005D0000, (char*)0x005D0001},
    {&sec_loading_mileena, (char*)0x005E0000, (char*)0x005E0001},
    {&sec_loading_nightwolf, (char*)0x005F0000, (char*)0x005F0001},
    {&sec_loading_qc_closeup2, (char*)0x00600000, (char*)0x00600001},
    {&sec_loading_ra_determined, (char*)0x00610000, (char*)0x00610001},
    {&sec_loading_ra_lightningstream, (char*)0x00620000, (char*)0x00620001},
    {&sec_loading_rd_speced_01, (char*)0x00630000, (char*)0x00630001},
    {&sec_loading_rd_speced_02, (char*)0x00640000, (char*)0x00640001},
    {&sec_loading_sai, (char*)0x00650000, (char*)0x00650001},
    {&sec_loading_sc_flamesword, (char*)0x00660000, (char*)0x00660001},
    {&sec_loading_sc_speced_01, (char*)0x00670000, (char*)0x00670001},
    {&sec_loading_sc_speced_02, (char*)0x00680000, (char*)0x00680001},
    {&sec_loading_scorpion, (char*)0x00690000, (char*)0x00690001},
    {&sec_loading_sindel, (char*)0x006A0000, (char*)0x006A0001},
    {&sec_loading_subzero, (char*)0x006B0000, (char*)0x006B0001},
    {&sec_loading_sz_icewall, (char*)0x006C0000, (char*)0x006C0001},
    {&sec_loading_trio_united, (char*)0x006D0000, (char*)0x006D0001},
    {&sec_loading_undeadarmy, (char*)0x006E0000, (char*)0x006E0001},
    {&sec_loading_limei, (char*)0x006F0000, (char*)0x006F0001},
    {&sec_ba_armed, (char*)0x00700000, (char*)0x00700001},
    {&sec_ba_closeup, (char*)0x00710000, (char*)0x00710001},
    {&sec_mil_kick, (char*)0x00720000, (char*)0x00720001},
    {&sec_mil_closeup_flip, (char*)0x00730000, (char*)0x00730001},
    {&sec_ba_blades, (char*)0x00740000, (char*)0x00740001},
    {&sec_mil_peering, (char*)0x00750001, (char*)0x00750000},
    {&sec_mil_sai, (char*)0x00760000, (char*)0x00760001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_er1_pic_tbl[] = {
    {&sec_loading_earthrealm1, (char*)0x00770000, (char*)0x00770001},
    {&sec_loading_earthrealm2, (char*)0x00780000, (char*)0x00780001},
    {&sec_loading_earthrealm3, (char*)0x00790000, (char*)0x00790001},
    {&sec_loading_earthrealm4, (char*)0x007A0000, (char*)0x007A0001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_er2_pic_tbl[] = {
    {&sec_load_er_shangtsung_02, (char*)0x007B0000, (char*)0x007B0001},
    {&sec_load_er_portal_01, (char*)0x007C0000, (char*)0x007C0001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_nx1_pic_tbl[] = {
    {&sec_load_nx_portals_00, (char*)0x007D0000, (char*)0x007D0001},
    {&sec_load_nx_monster_00, (char*)0x007E0000, (char*)0x007E0001},
    {&sec_load_nx_kamidogus_00, (char*)0x007F0000, (char*)0x007F0001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_or1_pic_tbl[] = {
    {&sec_loading_or_panoramic_00, (char*)0x00800000, (char*)0x00800001},
    {&sec_loading_or_aerial_00, (char*)0x00810000, (char*)0x00810001},
    {&sec_loading_or_jail_02, (char*)0x00820000, (char*)0x00820001},
    {&sec_loading_or_riot_01, (char*)0x00830000, (char*)0x00830001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_cr1_pic_tbl[] = {
    {&sec_load_cr_noob_02, (char*)0x00840000, (char*)0x00840001},
    {&sec_load_cr_redcore_00, (char*)0x00850000, (char*)0x00850001},
    {&sec_load_cr_teleport_03, (char*)0x00860000, (char*)0x00860001},
    {&sec_load_cr_havik_01, (char*)0x00870000, (char*)0x00870001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_ow1_pic_tbl[] = {
    {&sec_loading_ow_capture_03, (char*)0x00900000, (char*)0x00900001},
    {&sec_loading_ow_tarkatas_00, (char*)0x00910000, (char*)0x00910001},
    {&sec_loading_ow_shao_01, (char*)0x00920000, (char*)0x00920001},
    {&sec_load_ow_faceoff_00, (char*)0x00930000, (char*)0x00930001},
    {&sec_loading_ow_kano_02, (char*)0x00940000, (char*)0x00940001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_nr1_pic_tbl[] = {
    {&sec_load_nr_ermac_01, (char*)0x00880000, (char*)0x00880001},
    {&sec_load_nr_skulls_00, (char*)0x00890000, (char*)0x00890001},
    {&sec_load_nr_town_00, (char*)0x008A0000, (char*)0x008A0001},
    {&sec_load_nr_ashrah_02, (char*)0x008B0000, (char*)0x008B0001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_konquest_ed1_pic_tbl[] = {
    {&sec_load_ed_palace_00, (char*)0x008C0000, (char*)0x008C0001},
    {&sec_load_ed_entrance_00, (char*)0x008D0000, (char*)0x008D0001},
    {&sec_load_ed_tanya_01, (char*)0x008E0000, (char*)0x008E0001},
    {&sec_load_ed_knighted_02, (char*)0x008F0000, (char*)0x008F0001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_chess_pic_tbl[] = {
    {&sec_chess_load, (char*)0x00950000, (char*)0x00950001},
    {&sec_loading_chess_ash_bo, (char*)0x00960000, (char*)0x00960001},
    {&sec_loading_chess_ashrah, (char*)0x00970000, (char*)0x00970001},
    {&sec_loading_chess_barmil, (char*)0x00980000, (char*)0x00980001},
    {&sec_loading_chess_bomb, (char*)0x00990000, (char*)0x00990001},
    {&sec_loading_chess_dr_vs_kb, (char*)0x009A0000, (char*)0x009A0001},
    {&sec_loading_chess_er_cs, (char*)0x009B0000, (char*)0x009B0001},
    {&sec_loading_chess_sindel, (char*)0x009C0000, (char*)0x009C0001},
    {&sec_loading_chess_sc_sz, (char*)0x009D0000, (char*)0x009D0001},
    {&sec_loading_chess_nightwolf, (char*)0x009E0000, (char*)0x009E0001},
    {&sec_loading_chess_sz_scorp, (char*)0x009F0000, (char*)0x009F0001},
    {&sec_loading_chess_nw_br, (char*)0x00A00000, (char*)0x00A00001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

LoadingScreenEntry loading_puzzle_pic_tbl[] = {
    {&sec_loading_puzzle_br_vs_mil, (char*)0x00A10000, (char*)0x00A10001},
    {&sec_loading_puzzle_br_nw, (char*)0x00A20000, (char*)0x00A20001},
    {&sec_loading_puzzle_er_vs_as, (char*)0x00A30000, (char*)0x00A30001},
    {&sec_loading_puzzle_er_vs_sz, (char*)0x00A40000, (char*)0x00A40001},
    {&sec_loading_puzzle_ho_vs_sc, (char*)0x00A50000, (char*)0x00A50001},
    {&sec_loading_puzzle_snake, (char*)0x00A60000, (char*)0x00A60001},
    {(MkFileInfo*)-1, (char*)-1, (char*)-1},
};

char* round_numbers_tbl[] = {
    (char*)0x00020020, (char*)0x00020021, (char*)0x00020022,
    (char*)0x00020023, (char*)0x00020024,
};
extern GlobalPlayerEntry global_player_data[];
extern const MkFileEntry fatalityanims_file_table[];
extern int mk_chess_check_for_fatality(void);
extern void mk_chess_game_just_ended(void);
extern int trial_end_round(void);
extern void update_plyr_medals(void);
extern int advance_ladder_position(void);
extern void award_bet(void);
extern void ck_restore_kiddy(void);
extern void one_player_ladder_init(void);
extern void finish_music(void);
extern void end_music(void);
extern void play_final_fatality_music(void);
extern void big_boss_death(void);
extern void do_win_effect(void);
extern void reset_fight(int death_trap);
extern char* round_numbers_tbl[];
extern void init_wagering(void);
extern int get_current_wager_koin(void);
extern int char_for_ending;
extern int winner_for_ending;
extern KonquestLoadingPdataView* konquest_pdata;
extern unsigned char konquest_save_data[];
extern char bgnd_animations[];
extern int g_big_boss_intro_tap_out_f;
extern float p_animate(void);
extern float p_animated_intro_done(void);
extern float p_attract_camera(void);
extern float p_camera_proc(void);
extern float p_ladder_select(void);
extern float big_boss_taunt_cam_cut(void);
extern void show_wins_in_a_row(void);
extern void extend_powerbars(void);
extern void sidekick_intro_check(void);
extern void bgnd_anim_camera_setup(void);
extern void bgnd_anim_camera_ended(void);
extern void skip_camera_intro(void);
extern int intro_done(void);
extern void trial_start_new_round(void);
extern void trial_round_init(void);
extern int trial_show_standard_fight_messages(void);
extern void mk_chess_advantage_hud(void);
extern MkProc* get_player_proc(void* player);
extern void big_boss_wait_for_intro(void);
extern void konquest_transition_from_fight(void);
extern void mk_chess_transition_from_fight(void);
extern void player_postround_chores(void);
extern const MkFileEntry gameart_file_table[];
extern MkFileInfo sec_fightingart;
extern unsigned long long debug_get_msec_timer(void);
extern void set_section_memory_scheme(int scheme);
extern void ck_for_controller_removed(void);
extern void trial_setup_fight(void);
extern void mk_chess_in_fight_setup(void);
extern int ladder_get_current_bgnd(void);
extern int load_background(int bgnd_id);
extern void setup_sound_banks(int mode);
extern void start_plyrs(void);
extern void wait_for_sound_banks_to_load(void);
extern void start_first_pass_render(void);
extern void end_first_pass_render(void);
extern const char* get_pause_menu_name(void);
extern const MkFileEntry loading_images_file_table[];
extern int num_files_loaded;
extern int f_p1_warning_given;
extern int f_p2_warning_given;
extern void unimpale_victim(PlyrPdata* player);
extern void reload_fan(PlyrPdata* player);
extern void reset_severed_limbs(int player);
extern void bleed_restart(void);
extern void advance_active_moveset(PlyrPdata* player);
extern void start_mkpfx_FadeSnapShot(void);
extern void start_constrain_proc(void);
extern void xfer_player_proc(MkProc* proc, MkProcEntryFn entry);
extern float glitch_to_stance_j_exit(void);
extern float blend_to_stance_j_exit(void);
extern float getup_from_ground(void);
extern float give_some_distance(void);
extern void start_tunes(void);
extern float p_champion_screen(void);

static void ck_do_fatality(void);

static inline StringObj* validated_game_string(GameObjectLatch* latch) {
    StringObj* string = (StringObj*)latch->obj;

    if (string != 0) {
        if ((unsigned int)string->instance == latch->obj_instance) {
            return string;
        }
        return 0;
    }
    return 0;
}

static inline ScreenObj* validated_game_screen(GameObjectLatch* latch) {
    ScreenObj* object = (ScreenObj*)latch->obj;

    if (object != 0) {
        if ((unsigned int)object->instance == latch->obj_instance) {
            return object;
        }
        return 0;
    }
    return 0;
}

static inline StringObj* validated_string_instance(
    StringObj* object, unsigned int instance) {
    if (object != 0) {
        if ((unsigned int)object->instance == instance) {
            return object;
        }
        return 0;
    }
    return 0;
}

static inline ScreenObj* validated_screen_instance(
    ScreenObj* object, unsigned int instance) {
    if (object != 0) {
        if ((unsigned int)object->instance == instance) {
            return object;
        }
        return 0;
    }
    return 0;
}

static inline MkHdr* validated_plyr_screen(PlyrScreenLatch* latch) {
    MkHdr* object = (MkHdr*)latch->object;

    if (object != 0) {
        if ((unsigned int)object->instance == latch->instance) {
            return object;
        }
        return 0;
    }
    return 0;
}

static inline MkProc* validated_anim_proc(PlyrPdata* player) {
    MkProc* proc = player->anim_proc;

    if (proc != 0) {
        if ((unsigned int)proc->instance == player->anim_proc_instance) {
            return proc;
        }
        return 0;
    }
    return 0;
}

static inline void center_fight_effect(ScreenObj* object) {
    object->x = (int)((float)(screen_width / 2) -
                      (float)(object->pfx2d->tex_w / 2) * object->scale_x);
    object->y = (int)((float)(screen_height / 2) -
                      (float)(object->pfx2d->tex_h / 2) * object->scale_y) +
                20;
}


void init_bet_info_struct(void) {
    g_game_info.pselect.field_1d0 = 0;
    g_game_info.pselect.field_1d4 = 0;
    g_game_info.pselect.field_1d8 = 0;
    g_game_info.pselect.field_1dc = 0;
    g_game_info.pselect.field_1e0 = 1;
    g_game_info.pselect.field_1e4 = 6;
    g_game_info.pselect.field_1ec = 0;
    g_game_info.pselect.field_1e8 = 0;
}

int is_timer_off(void) {
    return b_game_timer_off;
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
static void do_fight_effect(void) {
    ScreenObj* round;
    ScreenObj* round_number;
    ScreenObj* fight;
    int i;
    int material;

    round =
        load_named_2d_pfxobj(0x10005, 0x201D, "ROUND", 0, 0x2C);
    if (round == 0) {
        return;
    }
    round_number = load_2d_pfxobj(
        0x10005, 0x201D,
        round_numbers_tbl[g_game_info.pselect.field_1f4 - 1], 0, 0x2C);
    if (round_number == 0) {
        if (round != 0) {
            if (round->instance != 0) {
                round->typed_vtbl->destroy(round);
            }
            return;
        }
    }
    fight =
        load_named_2d_pfxobj(0x10005, 0x201D, "FIGHT29", 0, 0x2C);
    if (fight == 0) {
        if (round != 0) {
            if (round->instance != 0) {
                round->typed_vtbl->destroy(round);
            }
        } else if (round_number != 0) {
            if (round_number->instance != 0) {
                round_number->typed_vtbl->destroy(round_number);
            }
        }
        return;
    }

        fight->scale_x = 0.0f;
        fight->scale_y = 0.0f;
        center_fight_effect(fight);
        fight->flag_bits.scaled = 1;

        round->x = screen_width / 2 - round->pfx2d->tex_w / 2;
        round->y = screen_height / 2 - round->pfx2d->tex_h / 2;
        round_number->x = round->x + 0xC6;
        round_number->y = round->y;

    switch (g_game_info.pselect.field_1f4) {
    case 1:
        snd_req(0x46);
        snd_req_delay(0, 0x6E);
        break;
    case 2:
        snd_req(0x47);
        snd_req_delay(0, 0x6E);
        break;
    case 3:
        snd_req(0x48);
        snd_req_delay(0, 0x6E);
        break;
    case 4:
        snd_req(0x49);
        snd_req_delay(0, 0x6E);
        break;
    case 5:
        snd_req(0x4A);
        snd_req_delay(0, 0x6E);
        break;
    default:
        snd_req_delay(0, 0x1E);
        break;
    }

    _mkproc_sleep_ticks = 100.0f;
    ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);

    for (i = 0; i < 30; i++) {
        for (material = 0; material < 4; material++) {
            round->pfx2d->verts[material].a -= 8;
            round_number->pfx2d->verts[material].a -= 8;
        }
        round->pfx2d->mirror = 1;
        round_number->pfx2d->mirror = 1;

        fight->scale_x += 0.033333335f;
        fight->scale_y += 0.033333335f;
        center_fight_effect(fight);

        _mkproc_sleep_ticks = 1.0f;
        ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }

    fight->pfx2d->scale_x = 1.0f;
    fight->pfx2d->scale_y = 1.0f;
    _mkproc_sleep_ticks = 60.0f;
    ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);

    if (fight->instance != 0) {
        fight->typed_vtbl->destroy(fight);
    }
    if (round->instance != 0) {
        round->typed_vtbl->destroy(round);
    }
    if (round_number->instance != 0) {
        round_number->typed_vtbl->destroy(round_number);
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

void cleanup_fight(void) {
    memset(global_movesets, 0, sizeof(global_movesets));
}

int check_for_winner(void) {
    int winner;

    winner = 0;
    if ((int)mode_of_play == 8) {
        winner = trial_check_state();
        if (winner == 1) {
            if (g_game_info.plyr1.field_0C != 0.0f) {
                g_game_info.plyr1.field_0C = 0.0f;
                destroy_mkprocs_pid(0x2003);
            }
        } else if (winner == 2) {
            if (g_game_info.plyr0.field_0C != 0.0f) {
                g_game_info.plyr0.field_0C = 0.0f;
                destroy_mkprocs_pid(0x2003);
            }
        }
        return winner;
    }

    if (b_game_timer_off == 0 && g_game_info.flag_bits.lens_flare_enabled &&
        g_game_info.field_204 <= 0) {
        if (g_game_info.plyr0.field_0C > g_game_info.plyr1.field_0C) {
            winner = 1;
            g_game_info.plyr1.field_0C = 0.0f;
        } else if (g_game_info.plyr1.field_0C > g_game_info.plyr0.field_0C) {
            winner = 2;
            g_game_info.plyr0.field_0C = 0.0f;
        } else if (g_game_info.plyr0.field_0C == g_game_info.plyr1.field_0C) {
            if (!g_game_info.feature_flags.bits.high_bit &&
                g_game_info.plyr0.field_0C == g_game_info.plyr0.field_10 &&
                g_game_info.plyr1.field_0C == g_game_info.plyr1.field_10) {
                winner = 3;
            } else if ((randu0(2) & 0xFFFF) == 0) {
                winner = 1;
                g_game_info.plyr1.field_0C = 0.0f;
            } else {
                winner = 2;
                g_game_info.plyr0.field_0C = 0.0f;
            }
        }
    } else {
        if (!g_game_info.flag_bits.level_fatality_active) {
            if (g_game_info.plyr0.field_0C == 0.0f &&
                g_game_info.plyr1.field_0C == 0.0f) {
                if ((randu0(2) & 0xFFFF) == 0) {
                    winner = 1;
                    g_game_info.plyr1.field_0C = 0.0f;
                } else {
                    winner = 2;
                    g_game_info.plyr0.field_0C = 0.0f;
                }
            } else if (g_game_info.plyr1.field_0C == 0.0f) {
                winner = 1;
            } else if (g_game_info.plyr0.field_0C == 0.0f) {
                winner = 2;
            }
        }

        if ((int)mode_of_play == 4 && get_level_fatality_done_flag_state() != 0) {
            if (g_game_info.plyr0.player_state != 0) {
                winner = 1;
            } else {
                winner = 2;
            }
        }
    }

    if ((int)mode_of_play == 10 && winner == 3) {
        if ((randu0(2) & 0xFFFF) == 0) {
            winner = 1;
            g_game_info.plyr1.field_0C = 0.0f;
        } else {
            winner = 2;
            g_game_info.plyr0.field_0C = 0.0f;
        }
    }
    return winner;
}

float do_join_in(void) {
    int game_state = get_game_state();
    JoinInPdata* join = (JoinInPdata*)apdata;
    char message[80];
    ScreenObj* overlay;
    int player_number;

    load_font(3);
    ((JoinByteFlags*)&g_game_info.flags)->bits.bit5 = 0;
    ((JoinByteFlags*)&g_game_info.flags)->bits.bit7 = 0;
    turn_controllers_off();
    b_game_timer_off = 1;
    snd_req(0x1AA5);

    if (game_state == 3) {
        snd_req(0x1B47);
        _mkproc_sleep_ticks = 30.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        fade_to_black(8, 1);
        gamelogic_jump(0, p_attract_mode);
    }
    if (game_state != 7 && game_state != 0x12) {
        return -1.0f;
    }

    g_game_info.plyr0.player_state = 1;
    g_game_info.plyr1.player_state = 1;
    overlay = load_2d_pfxobj(0, 0x2020, (char*)0x10017, 0, 0x1E);
    if (overlay != 0) {
        overlay->x = 0x8F;
        overlay->y = 0xBE;
        ((JoinByteFlags*)&overlay->flags)->bits.bit3 = 1;
        overlay->scale_x = 22.0f;
        overlay->scale_y = 8.0f;
        pfx_2d_obj_set_alpha_by_id(0x2020, 0xB4);
    }

    player_number = join->player ? 2 : 1;
    sprintf(message, get_string(10), player_number);
    string_center_xy(
        0x2021,
        3,
        message,
        screen_width / 2,
        screen_height / 2 + 0x20,
        0x1D);
    string_center_xy(
        0x2021,
        3,
        get_string(11),
        screen_width / 2,
        screen_height / 2,
        0x1D);
    string_center_xy(
        0x2021,
        3,
        get_string(12),
        screen_width / 2,
        screen_height / 2 - 0x20,
        0x1D);

    pause_procs(1);
    g_game_info.field_1F8 = 2;
    ((JoinProcFlagsView*)aproc)->flag_bits.bit3 = 1;
    _mkproc_sleep_ticks = 60.0f;
    ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    fade_to_black(8, 1);
    pause_procs(0);
    if (game_state == 0x12) {
        gamelogic_jump(1, p_pz_pselect);
    }
    gamelogic_jump(1, p_pselect);
    return -1.0f;
}

int ok_to_join_in(void) {
    int state;

    if ((int)mode_of_play == 4) {
        return 0;
    }
    if ((unsigned int)(mode_of_play - 7) <= 1 ||
        (int)mode_of_play == 10) {
        return 0;
    }
    state = get_game_state();
    switch (state) {
    case 7:
    case 0x12:
        return 1;
    default:
        return 0;
    }
}

float p_flash_demo_fight_text(void) {
    ScreenObj* text;

    text = validated_game_screen(&df_press_start_item);
    if (text != 0) {
        text->draw_flags.on = 0;
    }

    _mkproc_sleep_ticks = 20.0f;
    ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);

    text = validated_game_screen(&df_press_start_item);
    if (text != 0) {
        text->draw_flags.on = 1;
    }
    return 20.0f;
}

float p_do_damage_text(void) {
    DamageTextPdata* pdata;
    StringObj* string;
    int velocity;
    int ticks;

    pdata = (DamageTextPdata*)apdata;
    string = 0;
    velocity = 7;

    if (pdata->player == 0) {
        string = string_right_xy(
            0x201C, 0, pdata->text, 0, (int)pdata->y, 0x1D);
    } else if (pdata->player == 1) {
        string = string_left_xy(
            0x201C, 0, pdata->text, screen_width, (int)pdata->y, 0x1D);
        velocity *= -1;
    }

    ticks = 25;
    do {
        if (string != 0) {
            string->render_x += velocity;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        ticks--;
    } while (ticks > 0);

    ticks = 0;
    do {
        if (velocity > 0) {
            velocity--;
        }
        if (velocity < 0) {
            velocity++;
        }
        string->render_x += velocity;
        set_string_obj_alpha(
            string, (float)string->pfx.instance0.rgba[3] - 4.0f);
        _mkproc_sleep_ticks = 1.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        ticks++;
    } while (ticks < 62);

    if (string->instance != 0) {
        DamageStringVtable* vtbl;

        vtbl = (DamageStringVtable*)string->vtbl;
        vtbl->destroy(string, vtbl);
    }

    return -1.0f;
}

void show_damage_text(int player, int damage, int total) {
    DamageTextPdata* pdata;

    if (_create_mkproc_generic_tinystack(
            0x2028, 0x1F, p_do_damage_text, sizeof(DamageTextPdata),
            (MkHdr**)&pdata) != 0) {
        pdata->font_size = ((damage - 1) * 2) + 13;
        if (pdata->font_size > 21) {
            pdata->font_size = 21;
        }
        pdata->player = player;
        pdata->x = 135.0f;
        pdata->y = 345.0f;
        sprintf(pdata->text, get_string(8), damage);
    }

    if (_create_mkproc_generic_tinystack(
            0x2028, 0x1F, p_do_damage_text, sizeof(DamageTextPdata),
            (MkHdr**)&pdata) != 0) {
        pdata->font_size = ((damage - 1) * 2) + 10;
        pdata->player = player;
        pdata->x = 90.0f;
        pdata->y = 345.0f - (float)(pdata->font_size + 5);
        sprintf(pdata->text, get_string(9), total);
    }
}

void move_plyrs_to_round_start(void) {
    Vec player1_angles;
    Vec player2_angles;

    if (g_game_info.plyr0.slot.mirror_a != 0) {
        RoundStartPositions* starts = (RoundStartPositions*)g_game_info.misc;

        player1_angles.x = starts->player1_angles.x;
        player1_angles.y = starts->player1_angles.y;
        player1_angles.z = starts->player1_angles.z;
        starts->player1_position.y = g_game_info.plyr0.slot.mirror_a->pos.value.y;
        move_player(
            g_game_info.plyr0.slot.mirror_a,
            &starts->player1_position,
            &player1_angles);
    }

    if (g_game_info.plyr1.slot.mirror_a != 0) {
        RoundStartPositions* starts = (RoundStartPositions*)g_game_info.misc;

        player2_angles.x = starts->player2_angles.x;
        player2_angles.y = starts->player2_angles.y;
        player2_angles.z = starts->player2_angles.z;
        starts->player2_position.y = g_game_info.plyr1.slot.mirror_a->pos.value.y;
        move_player(
            g_game_info.plyr1.slot.mirror_a,
            &starts->player2_position,
            &player2_angles);
    }

    if ((int)mode_of_play == 8) {
        if (g_game_info.pselect.field_1f4 != 1) {
            show_player(g_game_info.plyr0.slot.pdata);
            show_player(g_game_info.plyr1.slot.pdata);
        }
    } else {
        show_player(g_game_info.plyr0.slot.pdata);
        show_player(g_game_info.plyr1.slot.pdata);
    }

    bgnd_swap_level(0);
    force_midpoint_calculation_update = 1;
}

void destroy_onscreen_fight_2d_objects(void) {
    StringObj* timer;

    delete_screen_obj_oid(0x2022);
    delete_screen_obj_oid(0x2013);
    destroy_pwr_bars();

    timer = validated_game_string(&game_timer_item);
    if (timer != 0) {
        destroy_string_obj(timer);
    }
}

void do_win_effect(void) {
    PlyrInfo* victor;
    PlyrInfo* defeated;
    int timing_index;
    int deathtrap_index;
    StringObj* winner_text;
    StringObj* winner_text_handle;
    StringObj* flawless_text;
    StringObj* flawless_text_handle;
    ScreenObj* fatality_left;
    ScreenObj* fatality_left_handle;
    ScreenObj* fatality_right;
    ScreenObj* fatality_right_handle;
    unsigned int winner_text_instance;
    unsigned int flawless_text_instance;
    unsigned int fatality_left_instance;
    unsigned int fatality_right_instance;
    float intro_ticks;
    float hold_ticks;
    float tail_ticks;
    unsigned char alpha;
    int sound_id;
    const char* player_name;
    int active_profiles;
    int icon_x;
    char message[0x50];

    fatality_left_handle = 0;
    fatality_left_instance = 0;
    fatality_left = 0;
    fatality_right_handle = 0;
    fatality_right_instance = 0;
    fatality_right = 0;
    winner_text_handle = 0;
    winner_text_instance = 0;
    flawless_text_handle = 0;
    flawless_text_instance = 0;
    flawless_text = 0;
    winner_text = 0;
    victor = 0;
    defeated = 0;

    if (round_winner == 1) {
        victor = &g_game_info.plyr0;
        defeated = &g_game_info.plyr1;
    } else if (round_winner == 2) {
        victor = &g_game_info.plyr1;
        defeated = &g_game_info.plyr0;
    }

    timing_index = 0;
    while (plyr_ending_timings[timing_index].character_id >= 0) {
        if (plyr_ending_timings[timing_index].character_id ==
            victor->player_index) {
            break;
        }
        timing_index++;
    }

    if (f_fatality_was_done != 0) {
        intro_ticks = inverse_game_speed *
                      plyr_ending_timings[timing_index].fatality_intro;
        hold_ticks = inverse_game_speed *
                     plyr_ending_timings[timing_index].fatality_hold;
        tail_ticks = 0.0f;
    } else if (get_level_fatality_done_flag_state() != 0) {
        deathtrap_index = 0;
        while (deathtrap_ending_timings[deathtrap_index].arena_id >= 0) {
            if (deathtrap_ending_timings[deathtrap_index].arena_id ==
                g_game_info.field_200) {
                break;
            }
            deathtrap_index++;
        }
        intro_ticks = inverse_game_speed *
                      deathtrap_ending_timings[deathtrap_index].intro;
        hold_ticks = inverse_game_speed *
                     deathtrap_ending_timings[deathtrap_index].hold;
        tail_ticks = inverse_game_speed *
                     deathtrap_ending_timings[deathtrap_index].tail;
    } else {
        intro_ticks = inverse_game_speed *
                      plyr_ending_timings[timing_index].normal_intro;
        hold_ticks = inverse_game_speed *
                     plyr_ending_timings[timing_index].normal_hold;
        tail_ticks = inverse_game_speed *
                     plyr_ending_timings[timing_index].normal_tail;
    }

    if (round_winner == 3) {
        sprintf(message, get_string_by_id(0x10005));
    } else {
        if ((int)mode_of_play == 8 && round_winner == 0) {
            sound_id = 0x7D;
            player_name = global_player_data[25].name;
        } else {
            player_name = global_player_data[victor->player_index].name;
            sound_id = victor->slot.fighter->runtime_data->win_sound_id;
        }
        sprintf(message, get_string_by_id(0x10004), player_name);
        if (sound_id != 0) {
            snd_req(sound_id);
        }
    }

    winner_text = string_center_xy(
        0x2023, 0x11, message, screen_width / 2, 0x154, 0x1D);
    if (winner_text != 0) {
        winner_text_handle = winner_text;
        winner_text_instance = winner_text->instance;
    }

    _mkproc_sleep_ticks = intro_ticks;
    ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);

    if (f_fatality_was_done != 0 &&
        is_big_boss(defeated->slot.pdata) == 0) {
        if (get_current_ssf_file() != fatalityanims_file_table) {
            load_ssf((MkFileEntry*)fatalityanims_file_table);
        }
        add_art_section(0x50015, find_section_by_name("fatality_art.sec"));

        if (g_game_info.field_200 == 3) {
            fatality_left = load_2d_pfxobj_xy(
                0x50015, 0x6004, (char*)0x570002, 0,
                ((screen_width - 0x280) / 2) + 0x80, 0xB0, 0x2E);
            fatality_right = load_2d_pfxobj_xy(
                0x50015, 0x6004, (char*)0x570003, 0,
                ((screen_width - 0x280) / 2) + 0x180, 0xB0, 0x2E);
            snd_req(0x19);
        } else {
            fatality_left = load_2d_pfxobj_xy(
                0x50015, 0x6004, (char*)0x570000, 0,
                ((screen_width - 0x280) / 2) + 0x80, 0xB0, 0x2E);
            fatality_right = load_2d_pfxobj_xy(
                0x50015, 0x6004, (char*)0x570001, 0,
                ((screen_width - 0x280) / 2) + 0x180, 0xB0, 0x2E);
            snd_req(0x17);
        }

        alpha = 0;
        if (fatality_left != 0 && fatality_right != 0) {
            fatality_left_handle = fatality_left;
            fatality_left_instance = fatality_left->instance;
            fatality_right_handle = fatality_right;
            fatality_right_instance = fatality_right->instance;

            while (alpha < 0xFF) {
                fatality_left = validated_screen_instance(
                    fatality_left_handle, fatality_left_instance);
                fatality_right = validated_screen_instance(
                    fatality_right_handle, fatality_right_instance);
                if (fatality_left != 0) {
                    pfx_2d_obj_set_alpha(fatality_left, alpha);
                }
                if (fatality_right != 0) {
                    pfx_2d_obj_set_alpha(fatality_right, alpha);
                }
                alpha = 0xFF - alpha > 4 ? alpha + 4 : 0xFF;
                _mkproc_sleep_ticks = 1.0f;
                ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
            }

            fatality_left = validated_screen_instance(
                fatality_left_handle, fatality_left_instance);
            fatality_right = validated_screen_instance(
                fatality_right_handle, fatality_right_instance);
            if (fatality_left != 0) {
                pfx_2d_obj_set_alpha(fatality_left, alpha);
            }
            if (fatality_right != 0) {
                pfx_2d_obj_set_alpha(fatality_right, alpha);
            }
            _mkproc_sleep_ticks = hold_ticks;
            ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        }
    } else if (g_game_info.plyr0.field_0C == g_game_info.plyr0.field_10 ||
               g_game_info.plyr1.field_0C == g_game_info.plyr1.field_10) {
        flawless_text = string_center_xy(
            0x2023, 0x11, get_string(6), screen_width / 2, 0x12C, 0x1D);
        flawless_text_instance = flawless_text->instance;
        flawless_text_handle = flawless_text;
        snd_req(0x16);
        _mkproc_sleep_ticks = hold_ticks;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }

    if (g_game_info.pause_flag_bits.fatality_window != 0 &&
        f_fatality_was_done == 0) {
        _mkproc_sleep_ticks = tail_ticks;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    } else if (g_game_info.plyr0.field_0C != g_game_info.plyr0.field_10 ||
               g_game_info.plyr1.field_0C == g_game_info.plyr1.field_10) {
        _mkproc_sleep_ticks = 45.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }

    alpha = 0xFF;
    while (alpha != 0) {
        winner_text = validated_string_instance(
            winner_text_handle, winner_text_instance);
        fatality_left = validated_screen_instance(
            fatality_left_handle, fatality_left_instance);
        fatality_right = validated_screen_instance(
            fatality_right_handle, fatality_right_instance);
        flawless_text = validated_string_instance(
            flawless_text_handle, flawless_text_instance);

        if (winner_text != 0) {
            set_string_obj_alpha(winner_text, (float)alpha / 255.0f);
        }
        if (flawless_text != 0) {
            set_string_obj_alpha(flawless_text, (float)alpha / 255.0f);
        }
        if (fatality_left != 0) {
            pfx_2d_obj_set_alpha(fatality_left, alpha);
        }
        if (fatality_right != 0) {
            pfx_2d_obj_set_alpha(fatality_right, alpha);
        }
        alpha = alpha < 4 ? 0 : alpha - 4;
        _mkproc_sleep_ticks = 1.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }

    if (flawless_text != 0 && flawless_text->instance != 0) {
        flawless_text->typed_vtbl->destroy(flawless_text);
    }
    if (winner_text != 0 && winner_text->instance != 0) {
        winner_text->typed_vtbl->destroy(winner_text);
    }
    if (fatality_left != 0 && fatality_left->instance != 0) {
        fatality_left->typed_vtbl->destroy(fatality_left);
    }
    if (fatality_right != 0 && fatality_right->instance != 0) {
        fatality_right->typed_vtbl->destroy(fatality_right);
    }

    if (g_game_info.pause_flag_bits.fatality_window != 0) {
        active_profiles = 0;
        if (g_game_info.plyr0.player_state == 2) {
            active_profiles = 1;
        }
        if (g_game_info.plyr1.player_state == 2) {
            active_profiles++;
        }
        if (active_profiles == 2) {
            if (victor != 0 && g_game_info.pselect.field_1dc > 0) {
                icon_x = validated_plyr_screen(&victor->name_latch) != 0
                             ? 0x58
                             : 0x3F;
                show_koin_award(
                    victor->field_04, g_game_info.pselect.field_1dc,
                    get_current_wager_koin(), icon_x);
                game_save_loop_count = 5;
                init_wagering();
                _mkproc_sleep_ticks = 60.0f;
                ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
            }
            return;
        }
    }
    if (g_game_info.pause_flag_bits.fatality_window != 0 &&
        (int)mode_of_play == 0) {
        active_profiles = 0;
        if (g_game_info.plyr0.player_state == 2) {
            active_profiles = 1;
        }
        if (g_game_info.plyr1.player_state == 2) {
            active_profiles++;
        }
        if (active_profiles == 1 &&
            ((round_winner == 1 && g_game_info.plyr0.player_state == 2) ||
             (round_winner == 2 && g_game_info.plyr1.player_state == 2)) &&
            ((round_winner == 1 && p1_profile_status == 1) ||
             (round_winner == 2 && p2_profile_status == 1))) {
            icon_x = validated_plyr_screen(&victor->name_latch) != 0
                         ? 0x58
                         : 0x3F;
            show_koin_award(
                victor->field_04, g_game_info.pselect.field_1ec,
                g_game_info.pselect.field_1e4, icon_x);
            _mkproc_sleep_ticks = 60.0f;
            ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        }
    }
}

void reset_fight(int death_trap) {
    Vec player1_angles;
    Vec player2_angles;

    destroy_mkprocs_pid(0x501B);
    unimpale_victim(g_game_info.plyr0.slot.pdata);
    unimpale_victim(g_game_info.plyr1.slot.pdata);
    reload_fan(g_game_info.plyr0.slot.pdata);
    reload_fan(g_game_info.plyr1.slot.pdata);
    destroy_mkprocs_pid(0x2026);
    g_game_info.plyr0.slot.pdata->impaled_projectile_state = 0;
    g_game_info.plyr1.slot.pdata->impaled_projectile_state = 0;

    if (g_game_info.pause_flag_bits.fatality_window != 0) {
        return;
    }

    if (death_trap != 0) {
        if ((int)mode_of_play == 8) {
            start_mkpfx_FadeSnapShot();
        } else {
            fade_to_black(20, 0);
        }
        bgnd_swap_level(0);

        g_game_info.plyr0.slot.mirror_a->pos.value.y = g_game_info.field_34;
        g_game_info.plyr1.slot.mirror_a->pos.value.y = g_game_info.field_34;
        if (g_game_info.plyr0.slot.mirror_a != 0) {
            RoundStartPositions* starts = (RoundStartPositions*)g_game_info.misc;

            player1_angles.x = starts->player1_angles.x;
            player1_angles.y = starts->player1_angles.y;
            player1_angles.z = starts->player1_angles.z;
            starts->player1_position.y = g_game_info.plyr0.slot.mirror_a->pos.value.y;
            move_player(
                g_game_info.plyr0.slot.mirror_a,
                &starts->player1_position,
                &player1_angles);
        }
        if (g_game_info.plyr1.slot.mirror_a != 0) {
            RoundStartPositions* starts = (RoundStartPositions*)g_game_info.misc;

            player2_angles.x = starts->player2_angles.x;
            player2_angles.y = starts->player2_angles.y;
            player2_angles.z = starts->player2_angles.z;
            starts->player2_position.y = g_game_info.plyr1.slot.mirror_a->pos.value.y;
            move_player(
                g_game_info.plyr1.slot.mirror_a,
                &starts->player2_position,
                &player2_angles);
        }

        if ((int)mode_of_play == 8) {
            if (g_game_info.pselect.field_1f4 != 1) {
                show_player(g_game_info.plyr0.slot.pdata);
                show_player(g_game_info.plyr1.slot.pdata);
            }
        } else {
            show_player(g_game_info.plyr0.slot.pdata);
            show_player(g_game_info.plyr1.slot.pdata);
        }
        bgnd_swap_level(0);
        force_midpoint_calculation_update = 1;
        _mkproc_sleep_ticks = 1.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);

        reset_severed_limbs(0);
        reset_severed_limbs(1);
        bleed_restart();

        if (g_game_info.plyr0.slot.pdata->character_id == 0x1B &&
            g_game_info.plyr0.slot.pdata->sidekick_active != 0) {
            while (g_game_info.plyr0.slot.pdata->player_slot != 1) {
                advance_active_moveset(g_game_info.plyr0.slot.pdata);
            }
        } else {
            while (g_game_info.plyr0.slot.pdata->player_slot != 0) {
                advance_active_moveset(g_game_info.plyr0.slot.pdata);
            }
        }
        if (g_game_info.plyr1.slot.pdata->character_id == 0x1B &&
            g_game_info.plyr1.slot.pdata->sidekick_active != 0) {
            while (g_game_info.plyr1.slot.pdata->player_slot != 1) {
                advance_active_moveset(g_game_info.plyr1.slot.pdata);
            }
        } else {
            while (g_game_info.plyr1.slot.pdata->player_slot != 0) {
                advance_active_moveset(g_game_info.plyr1.slot.pdata);
            }
        }

        g_game_info.plyr0.slot.mirror_a->hide_flag_bits.bit6 = 0;
        g_game_info.plyr1.slot.mirror_a->hide_flag_bits.bit6 = 1;
        if ((int)mode_of_play == 8) {
            xfer_proc(
                (MkProc*)g_game_info.plyr0.idle_proc,
                glitch_to_stance_j_exit);
            xfer_proc(
                (MkProc*)g_game_info.plyr1.idle_proc,
                glitch_to_stance_j_exit);
        } else {
            xfer_proc(
                (MkProc*)g_game_info.plyr0.idle_proc,
                blend_to_stance_j_exit);
            xfer_proc(
                (MkProc*)g_game_info.plyr1.idle_proc,
                blend_to_stance_j_exit);
        }

        if (find_mkproc_pid(0x1003) == 0) {
            start_constrain_proc();
        }
        pos_cam_for_current_level();
        force_midpoint_calculation_update = 1;
        _mkproc_sleep_ticks = 1.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);

        g_game_info.plyr0.slot.pdata->state_flags.bits.bit3 = 0;
        g_game_info.plyr1.slot.pdata->state_flags.bits.bit3 = 0;
        g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit6 = 0;
        g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit6 = 0;
        return;
    }

    if (round_winner == 1) {
        if (is_big_boss(g_game_info.plyr1.slot.pdata) == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr1.idle_proc, getup_from_ground);
        }
        if (is_big_boss(g_game_info.plyr0.slot.pdata) == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr0.idle_proc, give_some_distance);
        }
    } else if (round_winner == 2) {
        if (is_big_boss(g_game_info.plyr0.slot.pdata) == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr0.idle_proc, getup_from_ground);
        }
        if (is_big_boss(g_game_info.plyr1.slot.pdata) == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr1.idle_proc, give_some_distance);
        }
    } else {
        if (is_big_boss(g_game_info.plyr0.slot.pdata) == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr0.idle_proc, getup_from_ground);
        }
        if (is_big_boss(g_game_info.plyr1.slot.pdata) == 0) {
            xfer_player_proc(
                (MkProc*)g_game_info.plyr1.idle_proc, getup_from_ground);
        }
    }
}

int round_over(void) {
    PlyrInfo* round_winner_info;
    int active_players;
    int rounds_to_win;
    int ladder_advanced;

    round_winner_info = 0;
    if (g_game_info.feature_flags.bits.powerbars_locked) {
        destroy_mkprocs_pid(0x1003);
        turn_controllers_off();
        return 1;
    }
    if ((int)mode_of_play == 8) {
        return trial_end_round() == 0;
    }

    g_game_info.flag_bits.field_bit6 = 0;
    g_game_info.flag_bits.field_bit0 = 1;
    g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;
    g_game_info.plyr1.slot.mirror_a->flags_09_bits.tightrope_restricted = 0;

    if ((int)mode_of_play != 4 && (int)mode_of_play != 10 &&
        !g_game_info.feature_flags.bits.high_bit &&
        g_game_info.plyr0.field_0C + g_game_info.plyr1.field_0C ==
            g_game_info.plyr0.field_10 + g_game_info.plyr1.field_10) {
        ck_restore_kiddy();
        fade_to_black(12, 1);
        save_both_profiles(2);
        game_save_loop_count = 0;
        turn_controllers_off();
        snd_req(0x10);
        _mkproc_sleep_ticks = 45.0f;
        ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        atm_reset_current_page(1);
        fade_to_black(10, 1);
        set_player_state(&g_game_info.plyr0, 0);
        set_player_state(&g_game_info.plyr1, 0);
        gamelogic_jump(0, p_attract_mode);
    }

    if (round_winner == 1) {
        round_winner_info = &g_game_info.plyr0;
    } else if (round_winner == 2) {
        round_winner_info = &g_game_info.plyr1;
    }

    if (round_winner_info != 0) {
        switch ((int)mode_of_play) {
        case 4:
        case 8:
            round_winner_info->field_44 = 0;
            break;
        default:
            round_winner_info->field_40++;
            if (g_game_info.feature_flags.bits.high_bit) {
                rounds_to_win = 2;
            } else {
                rounds_to_win = game_settings.round_time;
            }
            if (round_winner_info->field_40 >= rounds_to_win ||
                (int)mode_of_play == 10) {
                winner = round_winner;
                g_game_info.pause_flag_bits.fatality_window = 1;
                round_winner_info->field_44++;
                round_winner_info->slot.pdata->his_plyr_pdata
                    ->plyr_info->field_44 = 0;
            }
            break;
        }
    }

    update_plyr_medals();
    if (g_game_info.pause_flag_bits.fatality_window) {
        active_players = 0;
        if (g_game_info.plyr0.player_state == 2) {
            active_players++;
        }
        if (g_game_info.plyr1.player_state == 2) {
            active_players++;
        }

        if (active_players == 1 && (int)mode_of_play == 0) {
            if (round_winner == 1 &&
                g_game_info.plyr0.player_state == 2) {
                award_koins_to_player(
                    0, g_game_info.pselect.field_1e8,
                    g_game_info.pselect.field_1e4);
            }
            if (round_winner == 2 &&
                g_game_info.plyr1.player_state == 2) {
                award_koins_to_player(
                    1, g_game_info.pselect.field_1e8,
                    g_game_info.pselect.field_1e4);
            }
        } else if (g_game_info.pselect.field_1d0 != 0) {
            award_bet();
        }
        ck_do_fatality();
    } else {
        if (!g_game_info.flag_bits.level_fatality_done &&
            (randu0(100) & 0xFFFF) < 75) {
            go_into_major_pain_please = 1;
        }
        end_music();
    }

    turn_controllers_off();
    switch ((int)mode_of_play) {
    case 4:
    case 8:
        break;
    case 10:
        mk_chess_game_just_ended();
        break;
    default:
        do_win_effect();
        while (g_game_info.flag_bits.level_fatality_active) {
            _mkproc_sleep_ticks = 1.0f;
            ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        }
        break;
    }

    active_players = 0;
    if (g_game_info.plyr0.player_state == 2) {
        active_players++;
    }
    if (g_game_info.plyr1.player_state == 2) {
        active_players++;
    }

    if (active_players == 1 &&
        g_game_info.pause_flag_bits.fatality_window &&
        (int)mode_of_play == 0) {
        ladder_advanced = 0;
        if ((g_game_info.plyr0.player_state == 2 && round_winner == 1) ||
            (g_game_info.plyr1.player_state == 2 && round_winner == 2)) {
            ladder_advanced = advance_ladder_position();
            g_game_info.pause_flag_bits.ladder_complete = ladder_advanced;
        }

        if ((int)mode_of_play == 0) {
            if (winner == 1) {
                if (p1_profile_status == 1) {
                    p1_profile.arcade_wins++;
                    if (g_game_info.pause_flag_bits.ladder_complete) {
                        p1_profile.ladder_completions++;
                    }
                }
                if (p2_profile_status == 1) {
                    p2_profile.arcade_losses++;
                }
            } else if (winner == 2) {
                if (p2_profile_status == 1) {
                    p2_profile.arcade_wins++;
                    if (g_game_info.pause_flag_bits.ladder_complete) {
                        p2_profile.ladder_completions++;
                    }
                }
                if (p1_profile_status == 1) {
                    p1_profile.arcade_losses++;
                }
            }
        }
    }

    if (g_game_info.pause_flag_bits.fatality_window) {
        active_players = 0;
        if (g_game_info.plyr0.player_state == 2) {
            active_players++;
        }
        if (g_game_info.plyr1.player_state == 2) {
            active_players++;
        }

        if (active_players == 2 &&
            ((int)mode_of_play == 1 || (int)mode_of_play == 0)) {
            if (!g_game_info.feature_flags.bits.high_bit) {
                if (winner == 1) {
                    if (p1_profile_status == 1) {
                        p1_profile.versus_wins++;
                    }
                    if (p2_profile_status == 1) {
                        p2_profile.versus_losses++;
                    }
                } else if (winner == 2) {
                    if (p2_profile_status == 1) {
                        p2_profile.versus_wins++;
                    }
                    if (p1_profile_status == 1) {
                        p1_profile.versus_losses++;
                    }
                }
            } else if (winner == 1) {
                if (p1_profile_status == 1) {
                    p1_profile.online_wins++;
                }
                if (p2_profile_status == 1) {
                    p2_profile.online_losses++;
                }
            } else if (winner == 2) {
                if (p2_profile_status == 1) {
                    p2_profile.online_wins++;
                }
                if (p1_profile_status == 1) {
                    p1_profile.online_losses++;
                }
            }
        }

        if ((int)mode_of_play == 0) {
            active_players = 0;
            if (g_game_info.plyr0.player_state == 2) {
                active_players++;
            }
            if (g_game_info.plyr1.player_state == 2) {
                active_players++;
            }
            if (active_players == 2) {
                if (winner == 1) {
                    if (g_game_info.field_1FC == 1) {
                        g_game_info.field_1FC = 0;
                        one_player_ladder_init();
                    }
                    set_player_state(&g_game_info.plyr1, 0);
                    g_game_info.plyr1.field_10 = 1.0f;
                    unload_p2_player_profile();
                } else if (winner == 2) {
                    if (g_game_info.field_1FC == 0) {
                        g_game_info.field_1FC = 1;
                        one_player_ladder_init();
                    }
                    set_player_state(&g_game_info.plyr0, 0);
                    g_game_info.plyr0.field_10 = 1.0f;
                    unload_p1_player_profile();
                } else if (g_game_info.field_1FC == 0) {
                    set_player_state(&g_game_info.plyr1, 0);
                    g_game_info.plyr1.field_10 = 1.0f;
                } else {
                    set_player_state(&g_game_info.plyr0, 0);
                    g_game_info.plyr0.field_10 = 1.0f;
                }

                g_game_info.field_1F8 = 0;
                if (g_game_info.plyr0.player_state == 2) {
                    g_game_info.field_1F8++;
                }
                if (g_game_info.plyr1.player_state == 2) {
                    g_game_info.field_1F8++;
                }
            }
        }

        g_game_info.pselect.field_1d0 = 0;
        g_game_info.pselect.field_1d4 = 0;
        g_game_info.pselect.field_1d8 = 0;
        g_game_info.pselect.field_1dc = 0;
        g_game_info.pselect.field_1e0 = 1;
        g_game_info.pselect.field_1e4 = 6;
        g_game_info.pselect.field_1ec = 0;
        g_game_info.pselect.field_1e8 = 0;
        return 1;
    }

    if ((int)mode_of_play == 4) {
        end_music();
        reset_fight(g_game_info.flag_bits.level_fatality_done);
        _mkproc_sleep_ticks = 100.0f;
        ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    } else {
        reset_fight(g_game_info.flag_bits.level_fatality_done);
    }
    if ((int)mode_of_play != 4) {
        g_game_info.pselect.field_1f4++;
    }
    return 0;
}

void reset_game_timer(void) {
    StringObj* timer;

    timer = validated_game_string(&game_timer_item);
    if (timer != 0 && timer->instance != 0) {
        timer->typed_vtbl->destroy(timer);
    }

    switch ((int)mode_of_play) {
    case 8:
        g_game_info.field_204 = trial_get_round_length();
        break;
    case 4:
        b_game_timer_off = 1;
        break;
    default:
        if (g_game_info.feature_flags.bits.powerbars_locked) {
            g_game_info.field_204 = 0x5F;
        } else {
            if (g_game_info.feature_flags.bits.high_bit) {
                g_game_info.field_204 = 0x3C;
            } else {
                g_game_info.field_204 = game_settings.round_timer_value;
            }
            if (g_game_info.field_204 == 0x5F) {
                b_game_timer_off = 1;
            }
        }
        break;
    }

    game_time_tick = 0x3C;
    g_game_info.field_218 = exec_tick_ctr;
    last_timer_sec = -1;
    update_game_timer();
}

#pragma dont_inline on

void round_init(void) {
    if (g_game_info.plyr0.field_10 == 0.0f) {
        g_game_info.plyr0.field_10 = 1.0f;
    }
    if (g_game_info.plyr1.field_10 == 0.0f) {
        g_game_info.plyr1.field_10 = 1.0f;
    }
    if ((int)mode_of_play != 10 && (int)mode_of_play != 8) {
        g_game_info.plyr0.field_0C = g_game_info.plyr0.field_10;
        g_game_info.plyr1.field_0C = g_game_info.plyr1.field_10;
    }

    g_game_info.flag_bits.field_bit0 = 0;
    g_game_info.flag_bits.field_bit6 = 1;
    round_winner = 0;
    g_game_info.flag_bits.lens_flare_enabled = 0;
    f_fatality_finished = 0;
    g_game_info.field_200 = -1;
    f_p1_warning_given = 0;
    f_p2_warning_given = 0;
    go_into_major_pain_please = 0;
    go_into_twitch_death_please = 0;

    if (g_game_info.feature_flags.bits.powerbars_locked) {
        MkProc* process;
        StringObj* string;

        process = _create_mkproc_generic_tinystack(
            0x2005, 0x1F, p_flash_demo_fight_text, 8,
            (MkHdr**)&empty_pdata);
        if (process != 0) {
            df_press_start_proc_item.obj = (MkHdr*)process;
            df_press_start_proc_item.obj_instance = process->instance;
        }
        string = string_center_xy(
            0x2010, 0, get_string(1), screen_width / 2, 0x50, 0x1D);
        if (string != 0) {
            df_press_start_item.obj = (MkHdr*)string;
            df_press_start_item.obj_instance = string->instance;
        }
    }

    if (g_game_info.pselect.field_1f4 == 1) {
        Vec player1_angles;
        Vec player2_angles;

        if (g_game_info.plyr0.slot.mirror_a != 0) {
            RoundStartPositions* starts = (RoundStartPositions*)g_game_info.misc;

            player1_angles.x = starts->player1_angles.x;
            player1_angles.y = starts->player1_angles.y;
            player1_angles.z = starts->player1_angles.z;
            starts->player1_position.y =
                g_game_info.plyr0.slot.mirror_a->pos.value.y;
            move_player(
                g_game_info.plyr0.slot.mirror_a,
                &starts->player1_position,
                &player1_angles);
        }
        if (g_game_info.plyr1.slot.mirror_a != 0) {
            RoundStartPositions* starts = (RoundStartPositions*)g_game_info.misc;

            player2_angles.x = starts->player2_angles.x;
            player2_angles.y = starts->player2_angles.y;
            player2_angles.z = starts->player2_angles.z;
            starts->player2_position.y =
                g_game_info.plyr1.slot.mirror_a->pos.value.y;
            move_player(
                g_game_info.plyr1.slot.mirror_a,
                &starts->player2_position,
                &player2_angles);
        }
        if ((int)mode_of_play == 8) {
            if (g_game_info.pselect.field_1f4 != 1) {
                show_player(g_game_info.plyr0.slot.pdata);
                show_player(g_game_info.plyr1.slot.pdata);
            }
        } else {
            show_player(g_game_info.plyr0.slot.pdata);
            show_player(g_game_info.plyr1.slot.pdata);
        }
        bgnd_swap_level(0);
        force_midpoint_calculation_update = 1;
    }

    if (g_game_info.plyr0.slot.mirror_a != 0 &&
        g_game_info.plyr1.slot.mirror_a != 0) {
        g_game_info.plyr0.slot.mirror_a->flags_09_bits.launched = 1;
        g_game_info.plyr0.slot.mirror_a->flags_09_bits.bit6 = 1;
        g_game_info.plyr0.slot.mirror_a->flags_09_bits.tightrope_restricted = 1;
        g_game_info.plyr0.slot.mirror_a->flags_09_bits.bit4 = 1;
        g_game_info.plyr0.slot.mirror_a->flags_0B_bits.bit6 = 0;
        g_game_info.plyr1.slot.mirror_a->flags_09_bits.launched = 1;
        g_game_info.plyr1.slot.mirror_a->flags_09_bits.bit6 = 1;
        g_game_info.plyr1.slot.mirror_a->flags_09_bits.tightrope_restricted = 1;
        g_game_info.plyr1.slot.mirror_a->flags_09_bits.bit4 = 1;
        g_game_info.plyr1.slot.mirror_a->flags_0B_bits.bit6 = 0;
    }

    if (find_mkproc_pid(0x1003) == 0) {
        start_constrain_proc();
    }
    if ((int)mode_of_play != 8) {
        reset_game_timer();
    }
    if ((int)mode_of_play != 6) {
        start_tunes();
    }
}

#pragma dont_inline reset

#pragma dont_inline on

void game_init(void) {
    MkHdr* timer;

    g_game_info.plyr0.field_40 = 0;
    g_game_info.plyr1.field_40 = 0;
    winner = -1;
    g_game_info.pselect.field_1f4 = 1;
    g_game_info.flag_bits.field_bit6 = 0;
    f_fatality_available = 0;
    set_level_fatality_done_flag_state(0);

    switch ((int)mode_of_play) {
    case 8:
        trial_game_init();
        break;
    case 4:
        init_pwr_bars();
        reset_game_timer();
        turn_switch_log_on();
        load_font(7);
        break;
    default:
        if (((int)mode_of_play == 0 || (int)mode_of_play == 10) &&
            g_game_info.feature_flags.bits.powerbars_locked) {
            reset_game_timer();
        } else {
            reset_game_timer();
            init_pwr_bars();
        }
        break;
    }

    show_fighting_style((int)g_game_info.plyr0.slot.pdata->weapon_styles[0], 0);
    show_fighting_style((int)g_game_info.plyr1.slot.pdata->weapon_styles[0], 1);
    bleed_startup();
    create_wall_monitor();

    timer = game_timer_item.obj;
    timer = timer != 0
                ? (timer->instance == game_timer_item.obj_instance ? timer : 0)
                : 0;
    if (timer != 0 && timer->instance != 0) {
        timer->typed_vtbl->destroy(timer);
    }

    reset_camera_paths();
    game_timer_item.obj = 0;
    game_timer_item.obj_instance = 0;
    df_press_start_proc_item.obj = 0;
    df_press_start_proc_item.obj_instance = 0;
    df_press_start_item.obj = 0;
    df_press_start_item.obj_instance = 0;
}

#pragma dont_inline reset

float p_demo_fight_timer(void) {
    _mkproc_sleep_ticks = 2100.0f;
    ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    return -1.0f;
}

int update_game_timer(void) {
    StringObj* timer;
    int timer_active;

    round_winner = check_for_winner();
    if (round_winner != 0) {
        return 0;
    }
    if (b_game_timer_off != 0) {
        timer_active = 0;
    } else if (g_game_info.flag_bits.level_fatality_done) {
        timer_active = 0;
    } else if (g_game_info.flag_bits.level_fatality_active) {
        timer_active = 0;
    } else if (g_game_info.flag_bits.level_transition_active) {
        timer_active = 0;
    } else {
        timer_active = 1;
    }
    if (timer_active == 0) {
        return 1;
    }

    if (game_time_tick <= 0) {
        game_time_tick = 0x3C;
        if (g_game_info.feature_flags.bits.powerbars_locked &&
            find_mkproc_pid(0x2070) == 0) {
            return 0;
        }
        g_game_info.field_204--;
        if (g_game_info.field_204 <= 10) {
            snd_req(0x1AA7);
        }
    } else {
        game_time_tick--;
    }

    if (last_timer_sec != g_game_info.field_204 &&
        !g_game_info.feature_flags.bits.powerbars_locked) {
        last_timer_sec = g_game_info.field_204;
        sprintf(timer_string, "%02d", g_game_info.field_204);

        timer = validated_game_string(&game_timer_item);
        if (timer != 0 && timer->instance != 0) {
            timer->typed_vtbl->destroy(timer);
        }

        timer = string_center_xy(
            0x2013, 4, timer_string, screen_width / 2, 0x196, 0x14);
        if (timer != 0) {
            game_timer_item.obj = (MkHdr*)timer;
            game_timer_item.obj_instance = timer->instance;
        }
    }
    return 1;
}

#pragma dont_inline on

#pragma dont_inline reset

static float get_num_sections_to_load(void) {
    switch (mode_of_play) {
    case 0:
    case 8:
    case 10:
        return 30.0f;
    case 6:
        return 30.0f;
    case 9:
        return 10.0f;
    case 7:
        return 11.0f;
    default:
        return 30.0f;
    }
}

#pragma dont_inline on

static LoadingScreenEntry* get_loading_table(void) {
    unsigned int mode;
    unsigned int region;
    char* tables;

    mode = mode_of_play;
    tables = (char*)plyr_ending_timings;
    switch (mode) {
    case 0:
    case 1:
    case 4:
    case 10:
        return (LoadingScreenEntry*)(tables + 0x52C);
    case 6:
        return (LoadingScreenEntry*)(tables + 0x9A0);
    case 9:
        return (LoadingScreenEntry*)(tables + 0x904);
    case 7:
    case 8:
        if ((int)mode == 7) {
            region = konquest_pdata->region;
        } else {
            region = ((KonquestLoadingSaveView*)konquest_save_data)->region;
        }
        break;
    default:
        return (LoadingScreenEntry*)(tables + 0x52C);
    }

    switch (region) {
    case 2:
        return (LoadingScreenEntry*)(tables + 0x778);
    case 3:
        return (LoadingScreenEntry*)(tables + 0x88C);
    case 4:
        return (LoadingScreenEntry*)(tables + 0x808);
    case 5:
        return (LoadingScreenEntry*)(tables + 0x844);
    case 6:
        return (LoadingScreenEntry*)(tables + 0x7CC);
    case 7:
        return (LoadingScreenEntry*)(tables + 0x8C8);
    case 8:
        return (LoadingScreenEntry*)(tables + 0x79C);
    default:
        return (LoadingScreenEntry*)(tables + 0x73C);
    }
}

static float p_load_screen(void) {
    LoadScreenPdata* pdata;
    ScreenObj* meter;

    pdata = (LoadScreenPdata*)apdata;
    meter = pdata->meter;
    meter = meter != 0
                ? (meter->instance == pdata->meter_instance ? meter : 0)
                : 0;

    if (meter == 0) {
        ScreenObj* image;
        ScreenObj* last_image;
        ScreenObj* line;
        ScreenObj* loading_icon;
        int x;

        unload_section_slot(pdata->section_slot);
        load_ssf((MkFileEntry*)loading_images_file_table);
        add_art_section_by_name_async_language(
            pdata->section_slot, "loading_common.sec");
        add_art_section(
            pdata->section_slot,
            pdata->table[pdata->image_index].section);
        wait_for_slot_load(pdata->section_slot);

        image = load_named_2d_pfxobj(
            0, 0x2058, "FADEBOX", 0, 0x35);
        if (image != 0) {
            image->x = -50;
            image->y = -50;
            image->flag_bits.scaled = 1;
            image->scale_x = 50.0f;
            image->scale_y = 40.0f;
        }

        meter = load_named_2d_pfxobj(
            pdata->section_slot, 0x2058, "LOADING_METER", 0, 0x33);
        if (meter == 0) {
            return 1.0f;
        }
        pdata->meter = meter;
        pdata->meter_instance = meter->instance;
        mk_insert((MkHdr*)meter, &aproc->pdata_list_b);

        if (is_widescreen_mode() != 0) {
            x = -0x20;
        } else {
            x = -0x20 - ((screen_width - 0x280) / 2);
        }

        last_image = load_2d_pfxobj_xy(
            pdata->section_slot, 0x2058,
            pdata->table[pdata->image_index].left_image, 0,
            x, 0x70, 0x34);
        if (last_image != 0) {
            mk_insert((MkHdr*)last_image, &aproc->pdata_list_b);
            if ((int)pdata->table[pdata->image_index].right_image != -1) {
                last_image = load_2d_pfxobj_xy(
                    pdata->section_slot, 0x2058,
                    pdata->table[pdata->image_index].right_image, 0,
                    x + 0x200, 0x70, 0x34);
                if (last_image != 0) {
                    mk_insert((MkHdr*)last_image, &aproc->pdata_list_b);
                }
            }

            line = load_named_2d_pfxobj(
                pdata->section_slot, 0x2058, "LOADING_LINE", 0, 0x34);
            if (line != 0) {
                mk_insert((MkHdr*)line, &aproc->pdata_list_b);
                line->flag_bits.scaled = 1;
                line->scale_x = 640.0f;
                line->scale_y = 1.0f;
                line->x = 0;
                line->y = last_image->y - 3;
            }

            line = load_named_2d_pfxobj(
                pdata->section_slot, 0x2058, "LOADING_LINE", 0, 0x34);
            if (line != 0) {
                mk_insert((MkHdr*)line, &aproc->pdata_list_b);
                line->flag_bits.scaled = 1;
                line->scale_x = 640.0f;
                line->scale_y = 1.0f;
                line->x = 0;
                line->y = last_image->y + 0x101;
            }
        }

        loading_icon = load_named_2d_pfxobj(
            pdata->section_slot, 0x2058, "LOADING_IMAGE", 0, 0x32);
        if (loading_icon != 0) {
            mk_insert((MkHdr*)loading_icon, &aproc->pdata_list_b);
            if (is_widescreen_mode() != 0) {
                loading_icon->x = ((screen_width - 0x280) / 2) + 0x17C;
            } else {
                loading_icon->x = 0x168;
            }
            loading_icon->y = 0x73;
            meter->flag_bits.scaled = 1;
            meter->x = loading_icon->x + 0x39;
            meter->y = loading_icon->y + 0x18;
        }
        return 1.0f;
    }

    if (g_game_info.flag_bits.pad_bit1) {
        meter->scale_x = 91.0f;
        _mkproc_sleep_ticks = 6.0f;
        ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        delete_screen_obj_oid(0x2058);
        unload_section_slot(pdata->section_slot);
        g_game_info.flag_bits.high_res_path = 0;
        return -1.0f;
    }

    {
        float progress = (float)num_files_loaded / pdata->file_count;

        if (progress > 1.0f) {
            progress = 1.0f;
        }
        meter->scale_x = (float)(int)(183.0f * progress * 0.5f);
    }
    return 1.0f;
}

int is_load_meter_active(void) {
    return (g_game_info.flags >> 7) & 1;
}

#pragma dont_inline reset

void display_load_meter(int section_slot) {
    LoadScreenPdata* pdata;
    LoadingScreenEntry* table;
    char* tables;
    int* image_index;
    int current_index;

    tables = (char*)plyr_ending_timings;
    g_game_info.flag_bits.pad_bit1 = 0;
    g_game_info.flag_bits.high_res_path = 1;
    init_file_loading_table();

    if (_create_mkproc_generic_bigstack(
            0x203C, 0x1F, p_load_screen, sizeof(LoadScreenPdata),
            (MkHdr**)&pdata) == 0) {
        return;
    }

    pdata->meter = 0;
    pdata->meter_instance = 0;
    switch ((int)mode_of_play) {
    case 0:
    case 10:
        table = (LoadingScreenEntry*)(tables + 0x52C);
        image_index = &game_settings.pad_3C;
        break;
    case 6:
        table = (LoadingScreenEntry*)(tables + 0x9A0);
        image_index = &game_settings.pad_44[1];
        break;
    case 9:
        table = (LoadingScreenEntry*)(tables + 0x904);
        image_index = &game_settings.pad_44[0];
        break;
    case 7:
    case 8:
        image_index = &game_settings.konquest_latch;
        table = get_loading_table();
        break;
    default:
        table = (LoadingScreenEntry*)(tables + 0x52C);
        image_index = &game_settings.pad_3C;
        break;
    }

    if (table[*image_index].left_image == (char*)-1) {
        *image_index = 0;
    }
    current_index = *image_index;
    *image_index = current_index + 1;
    pdata->image_index = current_index;
    pdata->section_slot = section_slot;
    pdata->table = get_loading_table();
    pdata->file_count = get_num_sections_to_load();

    _mkproc_sleep_ticks = 1.0f;
    ((JoinProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
}

float do_continue(void) {
    int screen_slot;

    push_game_state(0xA);
    pause_player = winner == 1;
    screen_slot = get_pause_menu_ssh();
    wait_for_slot_load();
    continue_timer = 10;
    load_screen("pause_menu/pause_continue", screen_slot, 0, 0);
    turn_controllers_on();
    target_game_mode = 0x18;

    while (continue_timer > 0 && target_game_mode == 0x18) {
        _mkproc_sleep_ticks = 70.0f;
        ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        continue_timer--;
    }

    turn_controllers_off();
    fade_to_black(8, 1);
    screen_engine_cleanup();
    if (target_game_mode == 6) {
        g_GameLossesInARow++;
        if ((int)mode_of_play == 6) {
            gamelogic_jump(1, p_pz_pselect);
        } else {
            gamelogic_jump(1, p_pselect);
        }
    }

    g_GameLossesInARow = 0;
    fade_to_black(12, 1);
    save_both_profiles(2);
    game_save_loop_count = 0;
    gamelogic_jump(0, p_main_menu);
    return -1.0f;
}

int get_continue_timer(void) {
    return continue_timer;
}

int ck_fatality_available(void) {
    PlyrInfo* victor;
    PlyrInfo* defeated;
    FatalityDefinition* fatality;

    if (winner == 1) {
        victor = &g_game_info.plyr0;
        defeated = &g_game_info.plyr1;
    } else if (winner == 2) {
        victor = &g_game_info.plyr1;
        defeated = &g_game_info.plyr0;
    } else {
        return 0;
    }

    if (is_big_boss(victor->slot.pdata) != 0 ||
        is_big_boss(defeated->slot.pdata) != 0) {
        return 0;
    }

    fatality = victor->slot.pdata->status_data->fatality_definition;
    if (fatality != 0) {
        if (victor->flags_14_bits.alternate_costume &&
            fatality->secondary_script == 0) {
            return 0;
        }
        if (fatality->primary_script == 0) {
            return 0;
        }
        if ((int)mode_of_play == 8) {
            return 0;
        }
        if (g_game_info.flag_bits.level_fatality_done) {
            return 0;
        }
        return get_blood_level() != 0;
    }
    return 0;
}

static float p_say_finish_him(void) {
    FinishHimPdata* pdata;
    ScreenObj* first;
    ScreenObj* second;
    int combined_width;
    int tick;
    float scale;

    pdata = (FinishHimPdata*)apdata;
    if (pdata->alternate_voice != 0) {
        snd_req_delay(0x15, 0x14);
        second = load_2d_pfxobj(0x10005, 0x201D, (char*)0x20027, 0, 0x2D);
    } else {
        snd_req_delay(0x14, 0x14);
        second = load_2d_pfxobj(0x10005, 0x201D, (char*)0x20026, 0, 0x2D);
    }
    first = load_2d_pfxobj(0x10005, 0x201D, (char*)0x20025, 0, 0x2D);
    if (first != 0 && second != 0) {
        combined_width = (first->pfx2d->tex_w + second->pfx2d->tex_w) / 2;
        first->flag_bits.scaled = 1;
        second->flag_bits.scaled = 1;
        for (tick = 0; tick <= 30; tick += 2) {
            scale = (float)tick / 30.0f;
            first->scale_x = scale;
            first->scale_y = scale;
            first->x = (int)(-((float)combined_width * first->scale_x -
                               (float)(screen_width / 2)));
            first->y = (int)(-((float)(first->pfx2d->tex_h / 2) *
                                   first->scale_x -
                               (float)(screen_width / 2)));
            second->scale_x = scale;
            second->scale_y = scale;
            second->x = (int)((float)first->pfx2d->tex_w * first->scale_x +
                              (float)first->x);
            second->y = first->y;
            _mkproc_sleep_ticks = 1.0f;
            ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        }

        for (tick = 0; tick < 90; tick++) {
            if (f_fatality_available == 0) {
                tick = 90;
            }
            _mkproc_sleep_ticks = 1.0f;
            ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        }

        for (tick = 0; tick < 30; tick++) {
            first->pfx2d->verts[0].a -= 8;
            first->pfx2d->verts[1].a -= 8;
            first->pfx2d->verts[2].a -= 8;
            first->pfx2d->verts[3].a -= 8;
            second->pfx2d->verts[0].a -= 8;
            second->pfx2d->verts[1].a -= 8;
            second->pfx2d->verts[2].a -= 8;
            second->pfx2d->verts[3].a -= 8;
            _mkproc_sleep_ticks = 1.0f;
            ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        }

        if (first->instance != 0) {
            first->typed_vtbl->destroy(first);
        }
        if (second->instance != 0) {
            second->typed_vtbl->destroy(second);
        }
    }
    return -1.0f;
}

static void ck_do_fatality(void) {
    PlyrInfo* victim;
    PlyrInfo* victor;
    MkHdr* spawned_pdata;
    int timeout;
    int fatality_occurred = 0;
    int timing_index;
    float ending_ticks;

    if ((int)mode_of_play == 10 && mk_chess_check_for_fatality() == 0) {
        return;
    }

    f_fatality_finished = 0;
    if (winner == 1) {
        victim = &g_game_info.plyr1;
        victor = &g_game_info.plyr0;
    } else if (winner == 2) {
        victim = &g_game_info.plyr0;
        victor = &g_game_info.plyr1;
    } else {
        return;
    }

    f_fatality_available = ck_fatality_available();

    if (f_fatality_available != 0 &&
        !g_game_info.flag_bits.level_fatality_done &&
        is_big_boss(victim->slot.pdata) == 0) {
        if (_create_mkproc_generic_bigstack(
                0x900A, 0x1F, p_say_finish_him,
                sizeof(FinishHimPdata), &spawned_pdata) != 0) {
            ((FinishHimPdata*)spawned_pdata)->alternate_voice =
                am_i_female(victim->slot.pdata);
        }
        finish_music();

        timeout = 420;
        while (victim->slot.pdata->state != 0x4203 && timeout != 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
            timeout--;
        }

        timeout = 300;
        while (timeout != 0 && victim->slot.pdata->state == 0x4203) {
            if (f_fatality_was_done != 0 && get_game_state() == 0xF) {
                break;
            }
            _mkproc_sleep_ticks = 1.0f;
            ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
            timeout--;
        }
    }

    turn_controllers_off();
    f_fatality_available = 0;
    if (f_fatality_was_done != 0) {
        fatality_occurred = 1;
    }

    if (is_big_boss(victim->slot.pdata) != 0) {
        f_fatality_finished = 0;
        f_fatality_was_done = 1;
        xfer_proc((MkProc*)victim->idle_proc,
                  (MkProcEntryFn)big_boss_death);
    } else if (!fatality_occurred) {
        end_music();
        if ((randu0(100) & 0xFFFF) < 75) {
            go_into_twitch_death_please = 1;
        }
        f_fatality_finished = 1;
        return;
    }

    timing_index = 0;
    while (plyr_ending_timings[timing_index].character_id !=
           victor->player_index) {
        if (plyr_ending_timings[timing_index].character_id < 0) {
            break;
        }
        timing_index++;
    }

    switch (g_game_info.field_200) {
    case 1:
        ending_ticks = plyr_ending_timings[timing_index].standard;
        break;
    case 2:
        ending_ticks = plyr_ending_timings[timing_index].alternate;
        break;
    case 3:
        timing_index = 0;
        while (plyr_ending_timings[timing_index].character_id !=
               victim->player_index) {
            if (plyr_ending_timings[timing_index].character_id < 0) {
                break;
            }
            timing_index++;
        }
        ending_ticks = plyr_ending_timings[timing_index].defeated;
        break;
    default:
        ending_ticks = 0.0f;
        break;
    }

    if (victim->player_index == 0x1D) {
        ending_ticks = 600.0f;
    }
    while (f_fatality_finished == 0 && ending_ticks > 0.0f) {
        _mkproc_sleep_ticks = 1.0f;
        ending_ticks -= game_speed;
        ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }

    play_final_fatality_music();
    _mkproc_sleep_ticks = 20.0f;
    ((GameProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
}

void ck_do_profile_save(void) {
    fade_to_black(12, 1);
    save_both_profiles(2);
    game_save_loop_count = 0;
}

static float p_do_ending(void) {
    g_game_info.pause_flag_bits.ladder_complete = 0;
    if (winner == 1) {
        winner_for_ending = 0;
        char_for_ending = g_game_info.plyr0.player_index;
        set_player_state(&g_game_info.plyr1, 0);
        mark_as_unlocked(
            (PlayerProfile*)&p1_profile, 6, char_for_ending);
    } else {
        winner_for_ending = 1;
        char_for_ending = g_game_info.plyr1.player_index;
        set_player_state(&g_game_info.plyr0, 0);
        mark_as_unlocked(
            (PlayerProfile*)&p2_profile, 6, char_for_ending);
    }

    fade_to_black(8, 1);
    fade_to_black(12, 1);
    save_both_profiles(2);
    game_save_loop_count = 0;
    gamelogic_jump(9, p_champion_screen);
    return -1.0f;
}

float p_game_loop(void) {
    BgndAnimationsView* animations;
    MkProc* proc;
    int active_players;
    int timeout;

    set_game_switch_maps();
    if (get_game_state() == 7 && (int)mode_of_play != 8) {
        show_wins_in_a_row();
    }
    round_init();

    proc = validated_anim_proc(g_game_info.plyr0.slot.pdata);
    if (proc != 0) {
        xfer_proc(proc, p_animate);
    }
    proc = validated_anim_proc(g_game_info.plyr1.slot.pdata);
    if (proc != 0) {
        xfer_proc(proc, p_animate);
    }

    if (((int)mode_of_play == 0 || (int)mode_of_play == 10) &&
        g_game_info.feature_flags.bits.powerbars_locked) {
        load_ssf((MkFileEntry*)attract_file_table);
        load_art_section(0x50014, &sec_demo_logo);
        if (get_language() == 3) {
            load_named_2d_pfxobj_xy(
                0x50014, 0x20A1, "ATTRACT_LOGO_FRE", 0,
                screen_width - 0xA0, 0x23, 0x1E);
        } else {
            load_named_2d_pfxobj_xy(
                0x50014, 0x20A1, "ATTRACT_LOGO", 0,
                screen_width - 0xA0, 0x23, 0x1E);
        }
    }

    _mkproc_sleep_ticks = 4.0f;
    ((GameLoopProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);

    if ((int)mode_of_play == 8) {
        trial_start_new_round();
        trial_round_init();
    } else if (g_game_info.pselect.field_1f4 == 1) {
        sidekick_intro_check();
        animations = (BgndAnimationsView*)bgnd_animations;
        if (animations->intro_path != 0 && (int)mode_of_play != 4) {
            set_intro_camera_path((void*)1);
            bgnd_anim_camera_setup();
            camera_init_animation(
                animations->intro_path, p_animated_intro_done);
            camera_run_animation(0);
            turn_camera_on();
            fade_from_black(20, 1);
            camera_wait_for_animation_completion();
            bgnd_anim_camera_ended();
            set_intro_camera_path(0);
        } else {
            turn_camera_on();
            skip_camera_intro();
            fade_from_black(20, 1);
        }
        if ((int)mode_of_play == 0 || (int)mode_of_play == 10) {
            if (!g_game_info.feature_flags.bits.powerbars_locked) {
                extend_powerbars();
            }
        } else {
            extend_powerbars();
        }
    } else {
        turn_camera_on();
        if (g_game_info.plyr0.slot.mirror_a != 0) {
            unhide_obj(g_game_info.plyr0.slot.mirror_a);
        }
        if (g_game_info.plyr1.slot.mirror_a != 0) {
            unhide_obj(g_game_info.plyr1.slot.mirror_a);
        }
        if (!g_game_info.flag_bits.level_fatality_done) {
            _mkproc_sleep_ticks = 90.0f;
            ((GameLoopProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
        } else {
            fade_from_black(20, 0);
        }
    }

    if ((int)mode_of_play == 10) {
        mk_chess_advantage_hud();
    }
    if (g_game_info.feature_flags.bits.powerbars_locked) {
        xfer_camera(p_attract_camera, 1);
    } else {
        if (intro_done() == 0) {
            skip_camera_intro();
        }
        xfer_camera(p_camera_proc, 1);
    }

    if (g_game_info.pselect.field_1f4 == 1 &&
        (is_big_boss(g_game_info.plyr0.slot.pdata) != 0 ||
         is_big_boss(g_game_info.plyr1.slot.pdata) != 0)) {
        if (is_big_boss(g_game_info.plyr0.slot.pdata) != 0) {
            g_big_boss_intro_tap_out_f = 0;
            xfer_proc(
                get_player_proc(g_game_info.plyr0.slot.mirror_a),
                big_boss_taunt_cam_cut);
        } else if (is_big_boss(g_game_info.plyr1.slot.pdata) != 0) {
            g_big_boss_intro_tap_out_f = 0;
            xfer_proc(
                get_player_proc(g_game_info.plyr1.slot.mirror_a),
                big_boss_taunt_cam_cut);
        }
        big_boss_wait_for_intro();
    }

    switch ((int)mode_of_play) {
    case 4:
    case 10:
        break;
    case 8:
        if (trial_show_standard_fight_messages() != 0) {
            do_fight_effect();
        }
        break;
    default:
        do_fight_effect();
        break;
    }

    turn_controllers_on();
    g_game_info.flag_bits.lens_flare_enabled = 1;
    set_level_fatality_done_flag_state(0);
    while (update_game_timer() != 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((GameLoopProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }

    timeout = 420;
    while (g_game_info.flag_bits.level_fatality_active ||
           g_game_info.flag_bits.level_transition_active) {
        timeout--;
        if (timeout < 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((GameLoopProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }
    g_game_info.flag_bits.lens_flare_enabled = 0;

    if (g_game_info.plyr0.slot.pdata != 0) {
        if (g_game_info.plyr0.slot.pdata->status_data != 0 &&
            g_game_info.plyr0.slot.pdata->status_data->reaction_cleanup != 0) {
            run_reaction_cleanup_function(g_game_info.plyr0.slot.pdata);
        }
    }
    if (g_game_info.plyr1.slot.pdata != 0) {
        if (g_game_info.plyr1.slot.pdata->status_data != 0 &&
            g_game_info.plyr1.slot.pdata->status_data->reaction_cleanup != 0) {
            run_reaction_cleanup_function(g_game_info.plyr1.slot.pdata);
        }
    }

    if (round_over() != 0) {
        game_loop_count++;
        game_save_loop_count++;
        force_bgnd_num = -1;

        active_players = 0;
        if (g_game_info.plyr0.player_state == 2) {
            active_players++;
        }
        if (g_game_info.plyr1.player_state == 2) {
            active_players++;
        }
        if (active_players == 2) {
            g_game_info.plyr1.player_index = 0x2C;
            g_game_info.plyr0.player_index = 0x2C;
        }

        turn_controllers_off();
        set_default_switch_maps();
        if (g_game_info.feature_flags.bits.powerbars_locked) {
            df_press_start_proc_item.obj = 0;
            df_press_start_proc_item.obj_instance = 0;
            df_press_start_item.obj = 0;
            df_press_start_item.obj_instance = 0;
            fade_to_black(12, 1);
            gamelogic_jump(0, p_atm_loop);
        }

        switch ((int)mode_of_play) {
        case 8:
            konquest_transition_from_fight();
            break;
        case 10:
            mk_chess_transition_from_fight();
            break;
        default:
            break;
        }

        active_players = 0;
        if (g_game_info.plyr0.player_state == 2) {
            active_players++;
        }
        if (g_game_info.plyr1.player_state == 2) {
            active_players++;
        }
        if (active_players == 1) {
            if ((g_game_info.plyr0.player_state == 2 && winner == 1) ||
                (g_game_info.plyr1.player_state == 2 && winner == 2)) {
                g_GameLossesInARow = 0;
                if (game_save_loop_count >= 5) {
                    fade_to_black(12, 1);
                    save_both_profiles(2);
                    game_save_loop_count = 0;
                } else {
                    fade_to_black(12, 1);
                }
                if (g_game_info.pause_flag_bits.ladder_complete) {
                    ((GameLoopProcVtable*)aproc->vtbl)->jump_sleep(
                        p_do_ending, aproc->vtbl, 0.0f);
                    return 0.0f;
                }
                gamelogic_jump(1, p_ladder_select);
            }
            ((GameLoopProcVtable*)aproc->vtbl)->jump_sleep(
                (MkProcEntryFn)do_continue, aproc->vtbl, 0.0f);
            return 0.0f;
        }

        if (g_game_info.feature_flags.bits.high_bit) {
            return -1.0f;
        }
        if (g_game_info.plyr0.field_0C + g_game_info.plyr1.field_0C ==
            2.0f) {
            if (game_save_loop_count != 0) {
                fade_to_black(12, 1);
                save_both_profiles(2);
                game_save_loop_count = 0;
            } else {
                fade_to_black(8, 1);
            }
            gamelogic_jump(0, p_attract_mode);
        }
        if (game_save_loop_count >= 5) {
            fade_to_black(12, 1);
            save_both_profiles(2);
            game_save_loop_count = 0;
        } else {
            fade_to_black(12, 1);
        }
        gamelogic_jump(1, p_pselect);
    }

    player_postround_chores();
    return 1.0f;
}

float p_gamelogic(void) {
    LoadScreenPdata* pdata;
    LoadingScreenEntry* table;
    int* image_index;
    int current_index;
    int active_players;
    int pause_slot;
    const char* pause_name;
    char* tables;

    tables = (char*)plyr_ending_timings;
    turn_controllers_off();
    start_time = (unsigned int)debug_get_msec_timer();
    g_game_info.flag_bits.field_bit6 = 0;
    g_game_info.pause_flag_bits.fatality_window = 0;
    f_fatality_was_done = 0;
    round_winner = 0;
    if ((int)mode_of_play == 10) {
        set_section_memory_scheme(5);
    } else {
        set_section_memory_scheme(0);
    }

    if (g_game_info.feature_flags.bits.powerbars_locked) {
        proc_create(p_demo_fight_timer, 0x2070);
        push_game_state(3);
    } else {
        push_game_state(7);
    }

    g_game_info.flag_bits.pad_bit1 = 0;
    g_game_info.flag_bits.high_res_path = 1;
    init_file_loading_table();
    if (_create_mkproc_generic_bigstack(
            0x203C, 0x1F, p_load_screen, sizeof(LoadScreenPdata),
            (MkHdr**)&pdata) != 0) {
        pdata->meter = 0;
        pdata->meter_instance = 0;
        switch ((int)mode_of_play) {
        case 0:
        case 10:
            table = (LoadingScreenEntry*)(tables + 0x52C);
            image_index = &game_settings.pad_3C;
            break;
        case 6:
            table = (LoadingScreenEntry*)(tables + 0x9A0);
            image_index = &game_settings.pad_44[1];
            break;
        case 9:
            table = (LoadingScreenEntry*)(tables + 0x904);
            image_index = &game_settings.pad_44[0];
            break;
        case 7:
        case 8:
            image_index = &game_settings.konquest_latch;
            table = get_loading_table();
            break;
        default:
            table = (LoadingScreenEntry*)(tables + 0x52C);
            image_index = &game_settings.pad_3C;
            break;
        }
        if (table[*image_index].left_image == (char*)-1) {
            *image_index = 0;
        }
        current_index = *image_index;
        *image_index = current_index + 1;
        pdata->image_index = current_index;
        pdata->section_slot = 0x50014;
        pdata->table = get_loading_table();
        pdata->file_count = get_num_sections_to_load();
        _mkproc_sleep_ticks = 1.0f;
        ((GameLoopProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    }

    turn_camera_on();
    if (g_game_info.feature_flags.bits.high_bit) {
        ck_for_controller_removed();
    }
    load_ssf((MkFileEntry*)gameart_file_table);
    load_art_section_language(0x10005, &sec_fightingart);
    load_font(5);
    load_font(4);
    load_font(0);
    load_font(1);
    load_font(3);
    load_font(0x11);
    set_process_as_scriptable(aproc);

    switch ((int)mode_of_play) {
    case 8:
        trial_setup_fight();
        break;
    case 10:
        mk_chess_in_fight_setup();
        break;
    default:
        break;
    }

    g_game_info.pselect.field_1f4 = 1;
    if (force_bgnd_num >= 0 && force_bgnd_num < 0x23) {
        g_game_info.bgnd_id = force_bgnd_num;
    }
    force_bgnd_num = -1;
    if (g_game_info.bgnd_id < 0 || g_game_info.bgnd_id >= 0x23) {
        active_players = 0;
        if (g_game_info.plyr0.player_state == 2) {
            active_players++;
        }
        if (g_game_info.plyr1.player_state == 2) {
            active_players++;
        }
        if (active_players == 1 && (int)mode_of_play == 0) {
            g_game_info.bgnd_id = ladder_get_current_bgnd();
        }
    }
    if (g_game_info.bgnd_id < 0 || g_game_info.bgnd_id >= 0x23) {
        gamelogic_jump(6, p_main_menu);
    }
    if (load_background(g_game_info.bgnd_id) == 0) {
        turn_camera_on();
    }

    setup_sound_banks(0xE);
    start_plyrs();
    setup_sound_banks(2);
    wait_for_sound_banks_to_load();
    _mkproc_sleep_ticks = 6.0f;
    g_game_info.flag_bits.pad_bit1 = 1;
    ((GameLoopProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    start_first_pass_render();
    _mkproc_sleep_ticks = 3.0f;
    ((GameLoopProcVtable*)aproc->vtbl)->sleep(aproc->vtbl);
    end_first_pass_render();
    turn_camera_off();
    game_init();

    if (!g_game_info.feature_flags.bits.powerbars_locked) {
        pause_slot = get_pause_menu_ssh();
        pause_name = get_pause_menu_name();
        unload_section_slot(pause_slot);
        preload_screen_data(pause_name, pause_slot);
    }
    ((GameLoopProcVtable*)aproc->vtbl)->jump_sleep(
        p_game_loop, aproc->vtbl, 0.0f);
    return 0.0f;
}

void init_game_info_struct(void) {
    GameInfoInitPrefix* prefix;

    prefix = (GameInfoInitPrefix*)&g_game_info;
    prefix->word = 0;
    prefix->bits.flag_6 = 0;
}
