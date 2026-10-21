#include "game/ladder.h"
#include "game/game.h"
#include "game/plyr.h"
#include "game/plyr_globals.h"
#include "game/cloth.h"
#include "platform/io.h"
#include "platform/main_jump.h"
#include "runtime/cam.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
#include "runtime/section.h"
#include "runtime/sound.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"

#include "game/bgnd.h"
#include "game/game_info.h"
#include "game/pselect.h"
#include "game/settings.h"
#include "platform/main.h"
#include "runtime/anim_pdata.h"
#include "runtime/anim_api.h"
#include "runtime/asset.h"
#include "runtime/fonts.h"
#include "runtime/light.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/utils.h"

#pragma use_lmw_stmw on

struct LadderCoinType {
    int type;
    char* name;
};

struct LadderEntry {
    int background_id;
    int locked_background_id;
    int character_id;
    int locked_character_id;
};

struct LadderModelEntry {
    int character_id;
    char* model_name;
};

struct LadderPlacement {
    Vec position;
    float angle_y;
    int mirrored;
};

struct LadderBgndAnimations {
    AnimScript* default_piece;
    AnimScript* piece_six;
    AnimScript* pieces_one_three_five;
    AnimScript* defeated_piece;
    AnimScript* piece_two;
    AnimScript* other_paths[25];
    AnimScript* intro_camera;
    AnimScript* travel_camera;
};

struct LadderStringRef {
    StringObj* object;
    unsigned int instance;
};

struct LadderHudEntry {
    int background_id;
    char* texture_name;
    int x;
    int y;
};

struct LadderCharacterTexture {
    int character_id;
    char* texture_name;
};


struct LadderCoinType coin_offset_tbl[6] = {
    {0, "LAD_WALLETKOIN_PLATINUM"},
    {1, "LAD_WALLETKOIN_ONYX"},
    {2, "LAD_WALLETKOIN_SAPPHIRE"},
    {3, "LAD_WALLETKOIN_JADE"},
    {4, "LAD_WALLETKOIN_RUBY"},
    {5, "LAD_WALLETKOIN_GOLD"},
};
enum { LADDER_COIN_TYPE_COUNT = sizeof(coin_offset_tbl) / sizeof(coin_offset_tbl[0]) };
const char* ladder_koins[4] = {
    "LAD_WALLETKOIN_SAPPHIRE",
    "LAD_WALLETKOIN_JADE",
    "LAD_WALLETKOIN_RUBY",
    "LAD_WALLETKOIN_GOLD",
};
int koin_award_table[9] = {20, 30, 40, 50, 60, 70, 80, 100, 0};
int pz_koin_award_table[7] = {50, 70, 80, 100, 120, 150, 0};

const char* chess_koins[2] = {
    "LAD_WALLETKOIN_PLATINUM",
    "LAD_WALLETKOIN_GOLD",
};
unsigned short n_chess_koins = 2;
int chess_koin_award_table[7] = {30, 30, 80, 60, 60, 90, 0};

struct LadderHudEntry ladder_data_table[35] = {
    {0, "LADDER_BEETLELAIR", 320, 33},
    {1, "LADDER_COURTYARD", 320, 33},
    {2, "LADDER_DARKPRISON", 320, 33},
    {3, "LADDER_DEADPOOL", 320, 33},
    {4, "LADDER_DKA", 320, 73},
    {5, "LADDER_DRAGONMOUNTAIN", 320, 73},
    {6, "LADDER_FALLINGCLIFF", 320, 33},
    {7, "LADDER_FORTRESS", 320, 73},
    {8, "LADDER_HELLSFOUNDRY", 320, 33},
    {9, "LADDER_KANGTOMB", 320, 73},
    {10, "LADDER_KUATANPALACE", 320, 33},
    {11, "LADDER_LIVINGFOREST", 320, 33},
    {12, "LADDER_LOSTWARRIORS", 320, 33},
    {13, "LADDER_LOWER_MINES", 320, 33},
    {14, "LADDER_NETHERBELLY", 320, 73},
    {15, "LADDER_NEXUS", 320, 33},
    {16, "LADDER_PORTAL", 320, 33},
    {17, "LADDER_REDDRAGON", 320, 73},
    {18, "LADDER_SKYTEMPLE", 320, 33},
    {19, "LADDER_SLAUGHTERHOUSE", 320, 73},
    {20, "LADDER_THEPIT", 320, 33},
    {21, "LADDER_YINYANG", 320, 73},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
};

struct LadderHudEntry pz_ladder_data_table[6] = {
    {0, "BGND_BEETLELAIR_PF", 320, 33},
    {8, "BGND_HELLFOUNDRY_PF", 320, 33},
    {9, "BGND_KANGSTOMB_PF", 320, 33},
    {18, "BGND_SKYTEMPLE_PF", 320, 33},
    {19, "BGND_SLAUGHTERHOUSE_PF", 320, 33},
    {21, "BGND_YINYANG_PF", 320, 33},
};

struct LadderCharacterTexture pz_char_ladder_data[12] = {
    {0, "PZLD_LADDER_SCORPION"},
    {1, "PZLD_LADDER_BARAKA"},
    {5, "PZLD_LADDER_NIGHTWOLF"},
    {6, "PZLD_LADDER_ERMAC"},
    {8, "PZLD_LADDER_SINDEL"},
    {3, "PZLD_LADDER_SUBZERO"},
    {10, "PZLD_LADDER_BORAICHO"},
    {12, "PZLD_LADDER_KENSHI"},
    {4, "PZLD_LADDER_MILEENA"},
    {21, "PZLD_LADDER_JADE"},
    {19, "PZLD_LADDER_KABAL"},
    {23, "PZLD_LADDER_RAIDEN"},
};
struct LadderEntry ladder_data_table1[8] = {
    {18, 18, 0, 0},
    {1, 2, 15, 22},
    {14, 14, 18, 24},
    {10, 8, 25, 4},
    {9, 21, 5, 10},
    {6, 6, 9, 6},
    {0, 0, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table2[8] = {
    {10, 8, 9, 19},
    {11, 6, 20, 6},
    {21, 21, 10, 3},
    {15, 2, 16, 4},
    {14, 14, 21, 22},
    {12, 0, 1, 24},
    {19, 19, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table3[8] = {
    {6, 6, 11, 24},
    {12, 0, 1, 1},
    {15, 19, 10, 10},
    {13, 20, 21, 22},
    {14, 14, 20, 20},
    {21, 21, 15, 3},
    {18, 18, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table4[8] = {
    {0, 0, 24, 24},
    {15, 19, 11, 6},
    {18, 18, 0, 0},
    {5, 17, 5, 5},
    {10, 8, 25, 10},
    {14, 14, 20, 1},
    {6, 6, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table5[8] = {
    {11, 21, 12, 5},
    {19, 19, 7, 7},
    {1, 14, 18, 10},
    {18, 18, 20, 20},
    {8, 8, 4, 4},
    {0, 0, 25, 22},
    {2, 6, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table6[8] = {
    {16, 20, 14, 0},
    {2, 2, 11, 5},
    {14, 14, 16, 19},
    {15, 17, 23, 24},
    {21, 21, 5, 5},
    {6, 6, 6, 6},
    {0, 13, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table7[8] = {
    {12, 8, 8, 20},
    {7, 0, 23, 0},
    {13, 6, 6, 6},
    {3, 21, 10, 1},
    {2, 18, 16, 7},
    {14, 14, 9, 3},
    {19, 19, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table8[8] = {
    {11, 6, 22, 22},
    {0, 0, 1, 1},
    {19, 19, 10, 10},
    {15, 14, 0, 0},
    {13, 21, 3, 3},
    {10, 17, 21, 24},
    {18, 18, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table9[8] = {
    {10, 2, 1, 1},
    {3, 13, 15, 6},
    {16, 17, 5, 5},
    {7, 20, 14, 0},
    {9, 8, 4, 4},
    {11, 14, 16, 22},
    {12, 6, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table10[8] = {
    {21, 21, 12, 10},
    {19, 19, 7, 7},
    {8, 8, 4, 4},
    {11, 13, 6, 6},
    {5, 17, 8, 19},
    {2, 18, 22, 0},
    {0, 0, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table11[8] = {
    {1, 19, 12, 3},
    {3, 13, 7, 7},
    {7, 17, 23, 22},
    {16, 18, 4, 4},
    {9, 21, 15, 6},
    {5, 2, 8, 5},
    {8, 20, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table12[8] = {
    {1, 19, 8, 24},
    {3, 13, 9, 22},
    {7, 17, 12, 7},
    {5, 2, 16, 10},
    {16, 20, 23, 20},
    {9, 21, 18, 19},
    {8, 8, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table13[8] = {
    {1, 2, 8, 24},
    {3, 13, 9, 22},
    {7, 17, 12, 7},
    {16, 20, 23, 20},
    {9, 14, 18, 19},
    {5, 5, 16, 10},
    {8, 19, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry ladder_data_table14[8] = {
    {1, 2, 8, 24},
    {3, 18, 9, 22},
    {7, 17, 12, 7},
    {16, 20, 23, 20},
    {8, 19, 18, 19},
    {5, 14, 16, 10},
    {13, 13, 27, 27},
    {4, 4, 29, 29},
};

struct LadderEntry* ladder_data_table_list[14] = {
    ladder_data_table1,
    ladder_data_table2,
    ladder_data_table3,
    ladder_data_table4,
    ladder_data_table5,
    ladder_data_table6,
    ladder_data_table7,
    ladder_data_table8,
    ladder_data_table9,
    ladder_data_table10,
    ladder_data_table11,
    ladder_data_table12,
    ladder_data_table13,
    ladder_data_table14,
};

struct LadderEntry pz_ladder_data_table1[6] = {
    {0, 0, 0, 0},
    {8, 8, 1, 1},
    {9, 9, 5, 5},
    {18, 18, 6, 6},
    {19, 19, 8, 8},
    {21, 21, 3, 3},
};

struct LadderEntry pz_ladder_data_table2[6] = {
    {8, 8, 1, 1},
    {0, 0, 0, 0},
    {19, 19, 8, 8},
    {18, 18, 6, 6},
    {21, 21, 5, 5},
    {9, 9, 3, 3},
};

struct LadderEntry pz_ladder_data_table3[6] = {
    {9, 9, 5, 5},
    {18, 18, 6, 6},
    {19, 19, 8, 8},
    {21, 21, 3, 3},
    {0, 0, 0, 0},
    {8, 8, 1, 1},
};

struct LadderEntry pz_ladder_data_table4[6] = {
    {18, 18, 21, 6},
    {21, 21, 23, 3},
    {8, 8, 12, 1},
    {0, 0, 10, 0},
    {9, 9, 4, 5},
    {19, 19, 19, 8},
};

struct LadderEntry pz_ladder_data_table5[6] = {
    {19, 19, 19, 8},
    {9, 9, 4, 5},
    {0, 0, 10, 0},
    {21, 21, 23, 3},
    {8, 8, 12, 1},
    {18, 18, 21, 6},
};

struct LadderEntry* pz_ladder_data_table_list[5] = {
    pz_ladder_data_table1,
    pz_ladder_data_table2,
    pz_ladder_data_table3,
    pz_ladder_data_table4,
    pz_ladder_data_table5,
};

Vec pz_cam_pos_tbl[6] = {
    {0.0f, -26.933f, 7.376953f},
    {0.0f, -23.361f, 7.376953f},
    {0.0f, -19.79f, 7.376953f},
    {0.0f, -16.219f, 7.376953f},
    {0.0f, -12.647f, 7.376953f},
    {0.0f, -9.076f, 7.376953f},
};

float ladder_frame_tbl[8][2] = {
    {0.0f, 120.0f},
    {120.0f, 240.0f},
    {240.0f, 360.0f},
    {360.0f, 480.0f},
    {480.0f, 600.0f},
    {600.0f, 720.0f},
    {720.0f, 840.0f},
    {840.0f, 960.0f},
};

struct LadderPlacement ladder_pos_tbl[8] = {
    {{-1.562f, -4.368f, 34.132f}, 0.784f, 0},
    {{1.281f, -0.918f, 29.0f}, 0.38f, 0},
    {{-1.562f, 4.0f, 24.557f}, 0.784f, 0},
    {{1.971f, 7.4f, 19.929f}, 0.38f, 0},
    {{-1.562f, 12.41f, 15.65f}, 0.784f, 0},
    {{1.8f, 15.81f, 10.62f}, 0.38f, 0},
    {{-1.0f, 20.9f, 6.1f}, 0.784f, 0},
    {{0.572f, 24.702f, 2.05f}, 0.38f, 0},
};
struct LadderPlacement dead_pos_tbl[8] = {
    {{-1.192f, -4.289f, 33.814f}, 0.784f, 0},
    {{0.89f, -0.84f, 29.464f}, 0.38f, 0},
    {{-1.655f, 4.102f, 22.009f}, 0.784f, 0},
    {{0.234f, 7.57f, 20.07f}, 0.38f, 0},
    {{-1.162f, 12.052f, 15.495f}, 0.784f, 0},
    {{0.734f, 16.01f, 10.854f}, 0.38f, 0},
    {{-1.006f, 20.852f, 6.2f}, 0.784f, 0},
    {{1.572f, 24.462f, 1.75f}, 0.38f, 0},
};

struct LadderPlacement ladder_small_pos_tbl[8] = {
    {{-1.562f, -4.368f, 34.132f}, 0.784f, 0},
    {{1.281f, -0.87f, 29.705f}, 0.38f, 0},
    {{-1.562f, 4.1152f, 24.557f}, 0.58f, 0},
    {{2.041f, 7.57f, 19.729f}, 0.38f, 0},
    {{-1.562f, 12.591f, 15.65f}, -0.38f, 0},
    {{2.0f, 16.01f, 10.62f}, 0.38f, 0},
    {{-0.956f, 21.052f, 5.972f}, 0.784f, 0},
    {{1.572f, 24.462f, 1.75f}, 0.38f, 0},
};

struct LadderModelEntry ladder_models[25] = {
    {27, "NOOBSMOKE"},
    {24, "CASSIUS"},
    {23, "RAIDEN"},
    {22, "DAIROU"},
    {21, "JADE"},
    {20, "KOBRA"},
    {19, "KABAL"},
    {18, "KIRA"},
    {16, "LIUKANG"},
    {15, "TANYA"},
    {14, "SKAB"},
    {11, "HOTARU"},
    {4, "MILEENA"},
    {5, "NIGHTWOLF"},
    {10, "BORAICHO"},
    {1, "BARAKA"},
    {0, "SCORPION"},
    {3, "SUBZERO"},
    {6, "ERMAC"},
    {12, "KENSHI"},
    {8, "SINDEL"},
    {7, "ASHRAH"},
    {9, "LIMEI"},
    {25, "SHUJINKO"},
    {-1, ""},
};

struct LadderDirectLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    RwRGBAReal color;
    Vec angle;
};

struct LadderAmbientLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    RwRGBAReal color;
};

struct LadderGroundCollisionEntry {
    int bone;
    Vec offset;
    float radius;
};

struct LadderDirectLightDef ladder_skinned_obj_light_def = {
    3, p_track_cam_ang_y_light, 1,
    {1.0f, 1.0f, 1.0f, 1.0f},
    {6.01f, 3.19f, 0.0f},
};

struct LadderAmbientLightDef ladder_skinned_obj_ambient_light_def = {
    1, 0, 3,
    {0.25f, 0.25f, 0.25f, 1.0f},
};

struct LadderGroundCollisionEntry ladder_piece_ground_colls[4] = {
    {7, {0.0f, 0.0f, 0.0f}, 0.1f},
    {8, {0.0f, 0.0f, 0.0f}, 0.1f},
    {10, {0.0f, 0.0f, 0.0f}, 0.3f},
    {-1, {0.0f, 0.0f, 0.0f}, 0.0f},
};

struct LadderGroundCollisionEntry char_piece_ground_colls[3] = {
    {7, {0.0f, 0.0f, 0.0f}, 0.3f},
    {8, {0.0f, 0.0f, 0.0f}, 0.3f},
    {-1, {0.0f, 0.0f, 0.0f}, 0.0f},
};

int ladder_piece_bones[18] = {
    17, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0
};

extern int pz_loss_in_a_row;
extern GlobalBackgroundEntry global_background_data[];
extern unsigned short n_ladder_koins;
extern int p1_profile_status;
extern int p2_profile_status;
extern struct LadderBgndAnimations bgnd_animations;

void insert_ground_me_mkobj(MkObj* object);
AnimPdata* animate_obj(
    MkObj* object,
    AnimScript* script,
    const int* bones,
    MkFlippedBoneMap* flipped_bones,
    void* ground_collisions,
    float playback_rate,
    int active);
static int ladder_data_tbl_offset = -1;
static int curr_ladder_pos;
static int curr_ladder_char;
static struct LadderStringRef bgnd_name_item;
static struct LadderEntry* current_ladder_tbl;
const char* ladder_koin_type_to_string(int type) {
    int i;

    for (i = 0; i < LADDER_COIN_TYPE_COUNT; i++) {
        if (coin_offset_tbl[i].type == type) {
            return coin_offset_tbl[i].name;
        }
    }
    return 0;
}

static inline int ladder_str_to_koin_type(const char* name) {
    int i;

    for (i = 0; i < LADDER_COIN_TYPE_COUNT; i++) {
        if (strcmp(coin_offset_tbl[i].name, name) == 0) {
            return coin_offset_tbl[i].type;
        }
    }
    return 0;
}

const char* get_rnd_chess_koin_type(int difficulty) {
    const char* coin;

    coin = chess_koins[randu0(n_chess_koins) & 0xFFFF];
    g_game_info.pselect.field_1e4 = ladder_str_to_koin_type(coin);
    return coin;
}

static inline const char* get_rnd_ladder_koin_type(void) {
    const char* coin;

    coin = ladder_koins[randu0(n_ladder_koins) & 0xFFFF];
    g_game_info.pselect.field_1e4 = ladder_str_to_koin_type(coin);
    return coin;
}

int get_chess_leader_won_coin_award(void) {
    int difficulty;
    int award;

    difficulty = game_settings.arcade_difficulty;
    if (difficulty < 2) {
        return 0;
    }

    award = 200;
    if (difficulty == 2) {
        return award * 1.0f;
    }
    if (difficulty == 3) {
        return award * 1.5f;
    }
    if (difficulty == 4) {
        award = award * 2.0f;
    }
    return award;
}

int get_chess_coin_award(int coin_index) {
    int award;

    if (coin_index < 0 || coin_index > 6) {
        return 0;
    }

    award = chess_koin_award_table[coin_index] * 5;
    if (game_settings.arcade_difficulty == 0) {
        return (0.5f * (float)award);
    }
    if (game_settings.arcade_difficulty == 1) {
        return (0.75f * (float)award);
    }
    if (game_settings.arcade_difficulty == 3) {
        return (1.1f * (float)award);
    }
    if (game_settings.arcade_difficulty == 4) {
        award = (1.25f * (float)award);
    }
    return award;
}

int get_ladder_position(void) {
    return curr_ladder_pos;
}

#pragma opt_common_subs off
static inline int ladder_get_bgnd_at_pos(int position)
{
    if (is_bgnd_locked(current_ladder_tbl[position].background_id)) {
        return current_ladder_tbl[position].locked_background_id;
    }
    return current_ladder_tbl[position].background_id;
}

int advance_ladder_position(void) {
    int ladder_size;
    int mode;
    int table_index;
    int next_position;

    ladder_size = 8;
    mode = mode_of_play;
    if (mode == 6) {
        ladder_size = 6;
    }

    if (curr_ladder_pos == 0 || curr_ladder_char == -1) {
        if (g_game_info.plyr0.player_state == 2) {
            curr_ladder_char = g_game_info.plyr0.player_index;
        } else {
            curr_ladder_char = g_game_info.plyr1.player_index;
        }
    }

    next_position = ++curr_ladder_pos;
    if (next_position == ladder_size) {
        g_game_info.pause_flag_bits.ladder_complete = 1;
        curr_ladder_pos = 0;
        curr_ladder_char = -1;
        g_game_info.field_20C = 0;

        if (mode == 6) {
            pz_loss_in_a_row = 0;
            table_index = randu0(5) & 0xFFFF;
            ladder_data_tbl_offset = table_index;
            current_ladder_tbl = pz_ladder_data_table_list[table_index];
        } else {
            table_index = randu0(14) & 0xFFFF;
            ladder_data_tbl_offset = table_index;
            current_ladder_tbl = ladder_data_table_list[table_index];
        }
        return 1;
    }

    g_game_info.bgnd_id = ladder_get_bgnd_at_pos(next_position);
    return 0;
}
#pragma opt_common_subs reset

void init_current_ladder_char(void) {
    curr_ladder_char = -1;
}

int ladder_get_current_bgnd(void) {
    return ladder_get_bgnd_at_pos(curr_ladder_pos);
}
void one_player_ladder_init(void) {
    int table_index;

    curr_ladder_pos = 0;
    curr_ladder_char = -1;
    g_game_info.field_20C = 0;

    if (mode_of_play == 6) {
        pz_loss_in_a_row = 0;
        table_index = randu0(5) & 0xFFFF;
        ladder_data_tbl_offset = table_index;
        current_ladder_tbl = pz_ladder_data_table_list[table_index];
    } else {
        table_index = randu0(14) & 0xFFFF;
        ladder_data_tbl_offset = table_index;
        current_ladder_tbl = ladder_data_table_list[table_index];
    }
}

static inline int get_curr_difficulty(void) {
    float difficulty_ranges[10] = {
        5.0f, 25.0f,
        15.0f, 40.0f,
        25.0f, 60.0f,
        40.0f, 80.0f,
        80.0f, 100.0f,
    };
    int display_difficulty;
    float step;

    display_difficulty = mode_of_play == 6
        ? game_settings.rounds_to_win
        : game_settings.kombat_difficulty;
    if (display_difficulty > 4) {
        display_difficulty = 4;
    } else if (display_difficulty < 0) {
        display_difficulty = 0;
    }
    step = (difficulty_ranges[display_difficulty * 2 + 1] -
            difficulty_ranges[display_difficulty * 2]) / 7.0f;
    return 0.5f + (step * (float)curr_ladder_pos +
        difficulty_ranges[display_difficulty * 2]);
}

static inline int get_ladder_coin_award(int position) {
    int award;
    int difficulty;

    if (position < 0 || position > 8) return 0;
    if (mode_of_play == 6) {
        if (position > 6) return 0;
        award = pz_koin_award_table[position] * 5;
        difficulty = game_settings.rounds_to_win;
    } else {
        award = koin_award_table[position] * 5;
        difficulty = game_settings.kombat_difficulty;
    }
    if (difficulty == 0) return 0.5f * (float)award;
    if (difficulty == 1) return 0.75f * (float)award;
    if (difficulty == 3) return 1.1f * (float)award;
    if (difficulty == 4) award = 1.25f * (float)award;
    return award;
}

static void build_ladder_hud_data(void) {
    char text_buffer[120];
    StringObj* arena_name;
    const char* text;
    int background_id;
    int award;
    int hours;
    int minutes;
    int seconds;
    int ladder_position;

    arena_name = 0;
    ladder_position = curr_ladder_pos;
    if (is_bgnd_locked(
            current_ladder_tbl[ladder_position].background_id)) {
        background_id =
            current_ladder_tbl[ladder_position].locked_background_id;
    } else {
        background_id =
            current_ladder_tbl[ladder_position].background_id;
    }

    text = get_string_by_id(
        global_background_data[background_id].field8 |
        0x10000u);
    if (text != 0) {
        int x;
        int y;

        x = ladder_data_table[background_id].x;
        y = ladder_data_table[background_id].y;
        if (mode_of_play == 6) {
            y += 60;
        }
        arena_name = string_center_xy(
            0x209A,
            9,
            text,
            x,
            y,
            0x1D);
        if (arena_name != 0) {
            bgnd_name_item.object = arena_name;
            bgnd_name_item.instance = arena_name->instance;
        }
    }
    set_string_obj_alpha(arena_name, 0.0f);

    hours = g_game_info.field_20C / 3600;
    minutes = g_game_info.field_20C / 60 - hours * 60;
    seconds = g_game_info.field_20C - (minutes * 60 + hours * 3600);
    if (hours != 0) {
        sprintf(
            text_buffer,
            get_string_by_id(0x10029),
            hours,
            minutes,
            seconds);
        string_left_xy(
            0x209A, 0, text_buffer, 0x31, 0x18F, 0x1D);
    } else {
        sprintf(
            text_buffer,
            get_string_by_id(0x1002A),
            minutes,
            seconds);
        string_left_xy(
            0x209A, 0, text_buffer, 0x31, 0x18F, 0x1D);
    }

    award = get_curr_difficulty();
    sprintf(text_buffer, get_string(0x24), award);
    string_left_xy(
        0x209A, 0, text_buffer, 0x31, 0x17B, 0x1D);

    if (g_game_info.plyr0.player_state != 2 ||
        p1_profile_status != 1) {
        if (g_game_info.plyr1.player_state != 2) {
            return;
        }
        if (p2_profile_status != 1) {
            return;
        }
    }

    award = get_ladder_coin_award(curr_ladder_pos);
    g_game_info.pselect.field_1e8 = award;

    if (mode_of_play != 6) {
        get_rnd_ladder_koin_type();
        show_koin_award(
            0,
            g_game_info.pselect.field_1e8,
            g_game_info.pselect.field_1e4,
            0x23);
    }
}

static inline char* fetch_plyr_ladder_model_name(
    struct LadderModelEntry* models, int character_id, int alternate_model)
{
    int index;
    for (index = 0; models[index].character_id != -1; index++) {
        if (models[index].character_id == character_id) {
            if (alternate_model != 0) {
                return "SMOKE";
            }
            return models[index].model_name;
        }
    }
    return 0;
}

static inline int ladder_get_char_at_pos(int position)
{
    if (is_char_locked(current_ladder_tbl[position].character_id, 0)) {
        return current_ladder_tbl[position].locked_character_id;
    }
    return current_ladder_tbl[position].character_id;
}

static void place_plyr_on_ladder(int position, int alternate_model) {
    char* model_name;
    AnimScript* piece_animation;
    AnimPdata* animation;
    MkObj* object;
    MkSobj* first_sobj;

    model_name = fetch_plyr_ladder_model_name(
        ladder_models,
        ladder_get_char_at_pos(position),
        alternate_model);
    if (model_name == 0) {
        return;
    }

    object =
        load_named_model_from_slot(0x18006D, model_name, 0xC022, 0);
    if (object == 0) {
        return;
    }

    gxVectCopy(&object->pos.value, &ladder_small_pos_tbl[position].position);
    object->ang.y = ladder_small_pos_tbl[position].angle_y;
    if (alternate_model != 0) {
        object->pos.value.x = 1.437f;
        object->ang.y = -0.5f;
    }

    if (curr_ladder_pos > position) {
        gxVectCopy(&object->pos.value, &dead_pos_tbl[position].position);
        if (object->pos.value.x < 0.0f) {
            object->ang.y = -0.4053982f;
        } else {
            object->ang.y = 0.784f;
        }
        if (alternate_model != 0) {
            object->pos.value.x = 1.437f;
            object->ang.y = 0.6f;
        }
    }

    object->hide_flag_bits.bit6 = ladder_small_pos_tbl[position].mirrored;
    object->ground_colls_y = ladder_small_pos_tbl[position].position.y;
    obj_change_to_skinned_obj_light_list(
        object, (LightDef*)&ladder_skinned_obj_light_def);
    if (position == 0U) {
        obj_add_to_skinned_obj_light_list_with_ambient(
            object, (LightDef*)&ladder_skinned_obj_ambient_light_def);
    }

    obj_create_sobjs(object);
    first_sobj = obj_first_sobj(object);
    if (first_sobj == 0) {
        if (object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy(&object->hdr);
        }
        return;
    }

    sobj_set_priority(first_sobj, 0x12);
    object->flags_09_bits.launched = 1;
    object->flags_09_bits.bit6 = 1;
    insert_ground_me_mkobj(object);
    unhide_obj(object);
    insert_fgnd_mkobj(object);
    mk_insert(&object->hdr, &g_game_info.bgnd_obj->child_list);

    switch (position) {
    case 6:
        piece_animation = bgnd_animations.piece_six;
        break;
    case 2:
        piece_animation = bgnd_animations.piece_two;
        break;
    case 1:
    case 3:
    case 5:
        piece_animation = bgnd_animations.pieces_one_three_five;
        break;
    case 4:
    default:
        piece_animation = bgnd_animations.default_piece;
        break;
    }

    animation = animate_obj(
        object,
        piece_animation,
        ladder_piece_bones,
        0,
        ladder_piece_ground_colls,
        1.0f,
        1);
    if (curr_ladder_pos > position) {
        AnimScript* defeated_animation = bgnd_animations.defeated_piece;
        if (alternate_model != 0) {
            set_anim_script_frame(animation, defeated_animation, 0x20, 60.0f);
        } else {
            set_anim_script(
                animation, defeated_animation, 0x20);
        }
        animation->step = 1.0f;
    }
}

extern MkFileInfo sec_fightingart;
extern const char* pz_ladder_koins[2];
extern unsigned short n_pz_ladder_koins;
extern PlyrPdata* his_pdata;
float p_puzzle_fighter(void);
float p_animated_intro_done(void);
int move_to_end_point(const Vec* endpoint, float* initial_speed,
                     float* final_speed, int mode, float rate);
void tag_team_activate_player(MkObj* object, int player);

static inline void ladder_sleep(float ticks) {
    _mkproc_sleep_ticks = ticks;
    aproc->vtbl->sleep();
}

static inline void ladder_create_player_with_slow_camera(PlyrInfo* opponent) {
    CameraPdata* camera = get_pdata_of_camera();
    float speed = camera->speed;
    camera->speed *= 1e-9f;
    create_player(opponent->field_04, opponent);
    camera->speed = speed;
}

static inline const char* get_rnd_pz_ladder_koin_type(void)
{
    const char* coin;

    coin = pz_ladder_koins[randu0(n_pz_ladder_koins) & 0xFFFF];
    g_game_info.pselect.field_1e4 = ladder_str_to_koin_type(coin);
    return coin;
}

static inline void show_ladder_hud_data(void)
{
    StringObj* arena_name;
    int alpha;

    alpha = 0;
    arena_name = 0;
    while (alpha < 255) {
        arena_name = MK_LIVE(bgnd_name_item.object, bgnd_name_item.instance);
        if (arena_name != 0) {
            set_string_obj_alpha(arena_name, (unsigned char)alpha);
            alpha += (signed char)(8.0f * game_speed);
            ladder_sleep(1.0f);
        }
    }
    set_string_obj_alpha(arena_name, 255.0f);
}

float p_ladder_select(void) {
    int i;
    PlyrInfo* player;
    MkProc* tracking;
    CameraPdata* camera;
    PlyrInfo* opponent;
    int j;
    int character;
    float initial_speed;
    float final_speed;
    int flags;
    int resume_frames;
    int award;
    int frames;

    tracking = 0;
    if (mode_of_play != 6) {
        set_section_memory_scheme(11);
    } else {
        set_section_memory_scheme(0);
    }
    push_game_state(5);
    turn_controllers_off();
    if (g_game_info.plyr0.player_state == 0) {
        opponent = &g_game_info.plyr0;
        opponent->field_04 = 0;
        player = &g_game_info.plyr1;
    } else if (g_game_info.plyr1.player_state == 0) {
        opponent = &g_game_info.plyr1;
        opponent->field_04 = 1;
        player = &g_game_info.plyr0;
    } else {
        opponent = &g_game_info.plyr1;
        opponent->field_04 = 1;
        g_game_info.plyr1.player_state = 0;
        player = &g_game_info.plyr0;
    }
    load_ssf(gameart_file_table);
    load_art_section(0x10005, &sec_fightingart);
    load_font(0);
    load_font(1);
    load_font(3);
    load_font(9);
    setup_sound_banks(11);
    wait_for_sound_banks_to_load();
    set_process_as_scriptable(aproc);
    if (mode_of_play == 6) {
        load_background(23);
    } else {
        load_background(22);
    }
    if (mode_of_play == 6) {
        for (i = 0; i < 6; i++) {
            int background = current_ladder_tbl[i].background_id;

            for (j = 0; j < 6; j++) {
                if (background == pz_ladder_data_table[j].background_id) break;
            }
            if (j >= 6) j = 0;
            bgnd_append_texture_to_material(8-i, 28-i,
                pz_ladder_data_table[j].texture_name, 0);
            bgnd_swap_textures(8-i, 28-i, 1);
            character = current_ladder_tbl[i].character_id;
            if (is_char_locked(character, character == 21))
                character = current_ladder_tbl[i].locked_character_id;
            for (j = 0; j < 12; j++) {
                if (character == pz_char_ladder_data[j].character_id) break;
            }
            if (j >= 12) j = 0;
            bgnd_append_texture_to_material(18-i, 38-i,
                pz_char_ladder_data[j].texture_name, 0);
            bgnd_swap_textures(18-i, 38-i, 1);
        }
    } else {
        for (i = 0; i < 8; i++) {
            bgnd_append_texture_to_material(18-i, 8-i,
                ladder_data_table[ladder_get_bgnd_at_pos(i)].texture_name, 0);
            bgnd_swap_textures(18-i, 8-i, 1);
            if (i != curr_ladder_pos && i != 7) {
                place_plyr_on_ladder(i, 0);
                if (i == 6) place_plyr_on_ladder(i, 1);
            } else if (i == 6 && i == curr_ladder_pos) {
                place_plyr_on_ladder(i, 1);
            }
        }
    }
    character = ladder_get_char_at_pos(curr_ladder_pos);
    opponent->player_index = character;
    if (curr_ladder_pos > 0 && curr_ladder_char != -1)
        player->player_index = curr_ladder_char;
    opponent->field_14 = 0;
    resolve_alternate_palettes(player);
    flags = opponent->field_14;
    load_plyr_model_async(opponent->field_04, opponent->player_index, &flags);
    g_game_info.bgnd_id = ladder_get_current_bgnd();
    build_ladder_hud_data();
    set_intro_camera_path((void*)1);
    bgnd_anim_camera_setup();
    if (curr_ladder_pos == 0) {
        if (mode_of_play == 6) snd_req(0x1AA1);
        camera = get_pdata_of_camera();
        camera->speed = 1.5f * game_speed;
        camera_init_animation(bgnd_animations.intro_camera, p_animated_intro_done);
        if (mode_of_play != 6) camera->speed = 0.5f * game_speed;
        camera_run_animation(0);
    } else {
        if (mode_of_play == 6) {
            snd_req(0x1AA2);
        } else {
            snd_req(0x1AA0);
        }
        if (mode_of_play == 6) {
            CamVec3 angle = {0.0326f, 3.1415927f, 0.0f};
            CamVec3 position;

            position.x = 0.0f;
            position.y = 3.571f * (float)(curr_ladder_pos-1) + -26.283203f;
            position.z = 7.376953f;
            go_to_camera_cut_with_angle(&position, &angle);
        } else {
            camera_init_animation(bgnd_animations.travel_camera, p_animated_intro_done);
            camera_run_animation_start_end(ladder_frame_tbl[curr_ladder_pos][0],
                ladder_frame_tbl[curr_ladder_pos][1], 0, 1);
        }
    }
    camera = get_pdata_of_camera();
    camera->speed = 1e-10f;
    turn_camera_on();
    fade_from_black(20, 0);
    camera = get_pdata_of_camera();
    camera->speed = 1.5f * game_speed;
    if (curr_ladder_pos == 0 && mode_of_play != 6) {
        frames = (72.0f * inverse_game_speed);
        ladder_sleep(frames);
        snd_req(0x1A9F);
        if (camera != 0) {
            frames = (22.0f * inverse_game_speed);
            for (i = 0; i < frames; i++) {
                camera->speed *= 0.9f;
                ladder_sleep(1.0f);
            }
            camera->speed *= 1e-7f;
        }
        ladder_create_player_with_slow_camera(opponent);
        resume_frames = frames;
        if (game_speed == 1.2f) resume_frames = frames + 6;
        for (i = 0; i < resume_frames; i++) {
            camera->speed += 2.0f / (float)frames;
            ladder_sleep(1.0f);
        }
    } else if (mode_of_play == 6) {
        CamVec3 position;

        initial_speed = 0.0f;
        final_speed = 0.0f;
        position.x = 0.0f;
        position.y = 3.572f * (float)curr_ladder_pos + -26.283203f;
        position.z = 7.376953f;
        while (!move_to_end_point(&position, &initial_speed,
                                 &final_speed, 0, 2.0f)) ladder_sleep(1.0f);
    }
    if (mode_of_play == 0) {
        MkObj* object;
        MkProc* animation_proc;
        AnimPdata* animation;
        PlyrPdata* pdata;
        unsigned int script;
        struct LadderPlacement* placement;
        int placement_position;
        if (curr_ladder_pos > 0) {
            ladder_create_player_with_slow_camera(opponent);
        }
        if (opponent->player_index == 27)
            tag_team_activate_player(opponent->slot.mirror_a, (unsigned short)randu0(1));
        object = opponent->slot.mirror_a;
        if (object != 0) {
            placement_position = curr_ladder_pos;
            opponent->slot.mirror_a->pos.value.x =
                ladder_pos_tbl[placement_position].position.x;
            placement = &ladder_pos_tbl[placement_position];
            opponent->slot.mirror_a->pos.value.y = placement->position.y;
            opponent->slot.mirror_a->pos.value.z = placement->position.z;
            opponent->slot.mirror_a->ang.y = placement->angle_y;
            opponent->slot.mirror_a->hide_flag_bits.bit6 = placement->mirrored;
            opponent->slot.mirror_a->flags_09_bits.launched = 1;
            opponent->slot.mirror_a->flags_09_bits.bit6 = 1;
            opponent->slot.mirror_a->ground_colls = char_piece_ground_colls;
            g_game_info.field_34 = placement->position.y - 10.0f;
            opponent->slot.mirror_a->ground_colls_y = placement->position.y;
            cloth_change_ground_plane_for(g_game_info.field_34);
            ground_me(opponent->slot.mirror_a);
            pdata = opponent->slot.pdata;
            animation_proc = MK_HDR_LIVE(pdata->anim_proc, pdata->anim_proc_instance);
            animation = (AnimPdata*)pdata_of_proc(animation_proc);
            set_anim_script(animation, opponent->slot.pdata->fighter_definition->duck_exit_animation, 0x20);
            animation->script = opponent->slot.pdata->fighter_definition->duck_exit_animation;
            if (opponent->player_index != 12)
                tracking = create_mkproc_headtracking(0x6006, opponent->slot.mirror_a, opponent->slot.pdata);
            if (opponent->flags_14_bits.alternate_costume)
                script = opponent->slot.pdata->runtime_data->alternate_script_48;
            else
                script = opponent->slot.pdata->runtime_data->primary_script_24;
            if (script != 0) {
                CmdScript* previous_script;
                PlyrPdata* previous_player;
                PlyrPdata* previous_opponent;
                MkObj* previous_object;
                MkObj* previous_other_object;

                previous_player = plyr_pdata;
                previous_script = active_cmdscript;
                previous_opponent = his_pdata;
                previous_object = plyr_obj;
                previous_other_object = his_obj;
                plyr_pdata = opponent->slot.pdata;
                his_pdata = 0;
                plyr_obj = opponent->slot.mirror_a;
                his_obj = 0;
                active_cmdscript = &global_script_interpreter;
                cmdscript_setup_execution(opponent->slot.pdata->cmo, script);
                cmdscript_execute(opponent->slot.pdata->cmo);
                active_cmdscript = previous_script;
                plyr_pdata = previous_player;
                his_pdata = previous_opponent;
                plyr_obj = previous_object;
                his_obj = previous_other_object;
            }
            xfer_proc(animation_proc, p_animate);
        }
    }
    camera_wait_for_animation_completion();
    bgnd_anim_camera_ended();
    set_intro_camera_path(0);
    show_ladder_hud_data();
    if (mode_of_play == 6) {
        if ((g_game_info.plyr0.player_state == 2 && p1_profile_status == 1) ||
            (g_game_info.plyr1.player_state == 2 && p2_profile_status == 1)) {
            award = get_ladder_coin_award(curr_ladder_pos);
            g_game_info.pselect.field_1e8 = award;
            get_rnd_pz_ladder_koin_type();
            show_koin_award(0, g_game_info.pselect.field_1e8,
                g_game_info.pselect.field_1e4, 0x23);
            ladder_sleep(30.0f);
        }
        ladder_sleep(90.0f);
    } else ladder_sleep(90.0f * inverse_game_speed);
    fade_to_black(10, 1);
    if (tracking != 0 && tracking->hdr.instance != 0)
        tracking->vtbl->destroy(tracking);
    if (mode_of_play == 6) gamelogic_jump(3, p_puzzle_fighter);
    gamelogic_jump(2, p_gamelogic);
    return -1.0f;
}
