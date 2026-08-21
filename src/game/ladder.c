#include "game/ladder.h"

#include "game/bgnd.h"
#include "game/game_info.h"
#include "game/pselect.h"
#include "game/settings.h"
#include "platform/main.h"
#include "runtime/anim_pdata.h"
#include "runtime/fonts.h"
#include "runtime/light.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/utils.h"

#pragma use_lmw_stmw on

typedef struct LadderCoinType {
    int type;
    const char* name;
} LadderCoinType;

typedef struct LadderEntry {
    int background_id;
    int locked_background_id;
    int character_id;
    int locked_character_id;
} LadderEntry; /* 0x10 */

typedef struct LadderModelEntry {
    int character_id;
    const char* model_name;
} LadderModelEntry; /* 0x08 */

typedef struct LadderPlacement {
    Vec position;
    float angle_y;
    int mirrored;
} LadderPlacement; /* 0x14 */

typedef struct LadderBgndAnimations {
    AnimScript* default_piece;
    AnimScript* piece_six;
    AnimScript* pieces_one_three_five;
    AnimScript* defeated_piece;
    AnimScript* piece_two;
} LadderBgndAnimations;

typedef struct LadderObjVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    void (*destroy)(MkObj* object);
} LadderObjVtable;

typedef struct LadderPauseFlags {
    unsigned char pad_7_6 : 2;
    unsigned char ladder_complete : 1;
    unsigned char pad_4_0 : 5;
} LadderPauseFlags;

typedef struct LadderStringRef {
    StringObj* object;
    unsigned int instance;
} LadderStringRef;

/*
 * Retail lays the ladder tables in one contiguous TU-local data region.
 * Keeping that relationship typed lets MWCC use the shared data base without
 * scattering byte-offset arithmetic through the ladder screen code.
 */
typedef struct LadderDataRegion {
    LadderCoinType coin_offsets[6]; /* +0x00 */
    const char* ladder_koins[4];    /* +0x30 */
    int koin_awards[9];             /* +0x40 */
    int puzzle_koin_awards[7];      /* +0x64 */
    int chess_koin_awards[7];       /* +0x80 */
    LadderEntry ladder_hud[35];     /* +0x9C */
    char pad2CC[0xB14];
    LadderPlacement defeated_positions[8]; /* +0xDE0 */
    LadderPlacement small_positions[8];    /* +0xE80 */
    LadderModelEntry models[25];           /* +0xF20 */
} LadderDataRegion;

#define LADDER_DATA_REGION ((LadderDataRegion*)coin_offset_tbl)

LadderCoinType coin_offset_tbl[6] = {
    {0, "LAD_WALLETKOIN_PLATINUM"},
    {1, "LAD_WALLETKOIN_ONYX"},
    {2, "LAD_WALLETKOIN_SAPPHIRE"},
    {3, "LAD_WALLETKOIN_JADE"},
    {4, "LAD_WALLETKOIN_RUBY"},
    {5, "LAD_WALLETKOIN_GOLD"},
};
const char* chess_koins[2] = {
    "LAD_WALLETKOIN_PLATINUM",
    "LAD_WALLETKOIN_GOLD",
};
unsigned short n_chess_koins = 2;
int chess_koin_award_table[7] = {30, 30, 80, 60, 60, 90, 0};
extern void* ladder_data_table_list[14];
extern void* pz_ladder_data_table_list[5];
extern int pz_loss_in_a_row;
extern GlobalBackgroundEntry global_background_data[];
extern unsigned short n_ladder_koins;
extern int p1_profile_status;
extern int p2_profile_status;
int strcmp(const char* left, const char* right);
int sprintf(char* destination, const char* format, ...);
extern LightDef ladder_skinned_obj_light_def;
extern LightDef ladder_skinned_obj_ambient_light_def;
extern unsigned char ladder_piece_ground_colls[];
extern unsigned char ladder_piece_bones[];
extern LadderBgndAnimations bgnd_animations;

MkObj* load_named_model_from_slot(
    int slot, const char* name, int flags, int unused);
void obj_create_sobjs(MkObj* object);
void insert_ground_me_mkobj(MkObj* object);
AnimPdata* animate_obj(
    MkObj* object,
    AnimScript* script,
    float speed,
    void* bones,
    int start_frame,
    void* ground_collisions,
    int active);
static int ladder_data_tbl_offset = -1;
static int curr_ladder_pos;
int curr_ladder_char;
static LadderStringRef bgnd_name_item;
static LadderEntry* current_ladder_tbl;
static const float chess_leader_award_normal = 200.0f;
static const float chess_leader_award_hard = 300.0f;
static const float chess_leader_award_max = 400.0f;
const char* ladder_koin_type_to_string(int type) {
    int i;

    for (i = 0; i < 6; i++) {
        if (coin_offset_tbl[i].type == type) {
            return coin_offset_tbl[i].name;
        }
    }
    return 0;
}

/*
 * Soft ceiling: get_rnd_chess_koin_type ~70.43% -- typed table walk is
 * coherent; remaining differences are NV allocation and pool placement.
 */
const char* get_rnd_chess_koin_type(void) {
    const char* coin;
    int coin_type;
    int index;

    coin = chess_koins[randu0(n_chess_koins) & 0xFFFF];
    coin_type = 0;
    for (index = 0; index < 6; index++) {
        if (strcmp(coin_offset_tbl[index].name, coin) == 0) {
            coin_type = coin_offset_tbl[index].type;
            break;
        }
    }
    g_game_info.pselect.field_1e4 = coin_type;
    return coin;
}

/*
 * Soft ceiling: get_chess_leader_won_coin_award ~49.33% -- retail preserves
 * float-to-int conversions of constant awards; keep the readable algorithm.
 */
int get_chess_leader_won_coin_award(void) {
    int difficulty;
    int award;

    difficulty = game_settings.arcade_difficulty;
    if (difficulty < 2) {
        return 0;
    }

    award = 200;
    if (difficulty == 2) {
        return chess_leader_award_normal;
    }
    if (difficulty == 3) {
        return chess_leader_award_hard;
    }
    if (difficulty == 4) {
        award = chess_leader_award_max;
    }
    return award;
}

/*
 * Soft ceiling: get_chess_coin_award ~99.47% -- opcodes are exact; only
 * compiler-generated float-pool relocation labels differ.
 */
int get_chess_coin_award(int coin_index) {
    int award;

    if (coin_index < 0 || coin_index > 6) {
        return 0;
    }

    award = chess_koin_award_table[coin_index] * 5;
    if (game_settings.arcade_difficulty == 0) {
        return (int)(0.5f * (float)award);
    }
    if (game_settings.arcade_difficulty == 1) {
        return (int)(0.75f * (float)award);
    }
    if (game_settings.arcade_difficulty == 3) {
        return (int)(1.1f * (float)award);
    }
    if (game_settings.arcade_difficulty == 4) {
        award = (int)(1.25f * (float)award);
    }
    return award;
}

int get_ladder_position(void) {
    return curr_ladder_pos;
}

/*
 * Soft ceiling: advance_ladder_position ~97.16% -- retail keeps the ladder
 * index in r31 and rematerializes its scaled offset after the call; MWCC keeps
 * the scaled offset in r31 instead. The algorithm and memory accesses agree.
 */
int advance_ladder_position(void) {
    int ladder_size;
    int mode;
    int table_index;
    int background_id;
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
        ((LadderPauseFlags*)&g_game_info.pause_flags)->ladder_complete = 1;
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

    if (is_bgnd_locked(
            current_ladder_tbl[next_position].background_id)) {
        background_id =
            current_ladder_tbl[next_position].locked_background_id;
    } else {
        background_id =
            current_ladder_tbl[next_position].background_id;
    }
    g_game_info.bgnd_id = background_id;
    return 0;
}

void init_current_ladder_char(void) {
    curr_ladder_char = -1;
}

int ladder_get_current_bgnd(void) {
    int ladder_position;

    ladder_position = curr_ladder_pos;
    if (is_bgnd_locked(
            current_ladder_tbl[ladder_position].background_id)) {
        return current_ladder_tbl[ladder_position].locked_background_id;
    }
    return current_ladder_tbl[ladder_position].background_id;
}

void one_player_ladder_init(void) {
    int table_index;

    curr_ladder_pos = 0;
    curr_ladder_char = -1;
    g_game_info.field_20C = 0;

    if ((int)mode_of_play == 6) {
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

/*
 * Builds the ladder screen's arena, elapsed-time, difficulty, and coin-award
 * strings. The local difficulty ranges intentionally remain an initialized
 * array: retail copies the ten-float table to the stack before interpolation.
 * Soft ceiling: ~89.09% - the recovered algorithm is complete; remaining
 * differences are nonvolatile allocation and structured award-tail branching.
 */
static void build_ladder_hud_data(void) {
    char text_buffer[120];
    LadderDataRegion* ladder_data;
    StringObj* arena_name;
    const char* text;
    const char* coin;
    int background_id;
    int difficulty;
    int award;
    int hours;
    int minutes;
    int seconds;
    int index;
    int ladder_position;
    int coin_type;

    ladder_data = (LadderDataRegion*)coin_offset_tbl;
    ladder_position = curr_ladder_pos;
    if (is_bgnd_locked(
            current_ladder_tbl[ladder_position].background_id)) {
        background_id =
            current_ladder_tbl[ladder_position].locked_background_id;
    } else {
        background_id =
            current_ladder_tbl[ladder_position].background_id;
    }

    arena_name = 0;
    text = get_string_by_id(
        (unsigned int)global_background_data[background_id].field8 |
        0x10000u);
    if (text != 0) {
        int y;

        y = ladder_data->ladder_hud[background_id].locked_character_id;
        if ((int)mode_of_play == 6) {
            y += 60;
        }
        arena_name = string_center_xy(
            0x209A,
            9,
            text,
            ladder_data->ladder_hud[background_id].character_id,
            y,
            0x1D);
        if (arena_name != 0) {
            bgnd_name_item.object = arena_name;
            bgnd_name_item.instance = arena_name->instance;
        }
    }
    ((void (*)(void*, float))set_string_obj_alpha)(arena_name, 0.0f);

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

    {
        float difficulty_ranges[10] = {
            5.0f, 25.0f,
            15.0f, 40.0f,
            25.0f, 60.0f,
            40.0f, 80.0f,
            80.0f, 100.0f,
        };
        int display_difficulty;

        display_difficulty = (int)mode_of_play == 6
            ? game_settings.rounds_to_win
            : game_settings.kombat_difficulty;
        if (display_difficulty > 4) {
            display_difficulty = 4;
        } else if (display_difficulty < 0) {
            display_difficulty = 0;
        }
        award = (int)(
            0.5f +
            ((difficulty_ranges[display_difficulty * 2 + 1] -
              difficulty_ranges[display_difficulty * 2]) /
             7.0f) *
                (float)curr_ladder_pos +
            difficulty_ranges[display_difficulty * 2]);
        sprintf(text_buffer, get_string(0x24), award);
        string_left_xy(
            0x209A, 0, text_buffer, 0x31, 0x17B, 0x1D);
    }

    if (g_game_info.plyr0.player_state != 2 ||
        p1_profile_status != 1) {
        if (g_game_info.plyr1.player_state != 2) {
            return;
        }
        if (p2_profile_status != 1) {
            return;
        }
    }

    if (curr_ladder_pos < 0 || curr_ladder_pos > 8) {
        award = 0;
    } else if ((int)mode_of_play == 6) {
        if (curr_ladder_pos > 6) {
            award = 0;
        } else {
            award =
                ladder_data->puzzle_koin_awards[curr_ladder_pos] * 5;
            difficulty = game_settings.rounds_to_win;
        }
    } else {
        award = ladder_data->koin_awards[curr_ladder_pos] * 5;
        difficulty = game_settings.kombat_difficulty;
    }

    if (award != 0) {
        if (difficulty == 0) {
            award = (int)(0.5f * (float)award);
        } else if (difficulty == 1) {
            award = (int)(0.75f * (float)award);
        } else if (difficulty == 3) {
            award = (int)(1.1f * (float)award);
        } else if (difficulty == 4) {
            award = (int)(1.25f * (float)award);
        }
    }
    g_game_info.pselect.field_1e8 = award;

    if ((int)mode_of_play != 6) {
        coin =
            ladder_data->ladder_koins[
                randu0(n_ladder_koins) & 0xFFFF];
        for (index = 0; index < 6; index++) {
            if (strcmp(
                    ladder_data->coin_offsets[index].name, coin) == 0) {
                coin_type = ladder_data->coin_offsets[index].type;
                break;
            }
        }
        if (index == 6) {
            coin_type = 0;
        }
        g_game_info.pselect.field_1e4 = coin_type;
        ((void (*)(int, int, int, int))show_koin_award)(
            0,
            g_game_info.pselect.field_1e8,
            g_game_info.pselect.field_1e4,
            0x23);
    }
}

/*
 * Builds one visible ladder fighter model. Typed placement rows capture the
 * retail 0x14-byte position/angle/mirror stride used by both live and defeated
 * pieces.
 * Soft ceiling: place_plyr_on_ladder ~75.86% - the typed shared data view
 * restores retail table-base coalescing; remaining differences are NV
 * allocation and a few address-expression shapes.
 */
static void place_plyr_on_ladder(int position, int alternate_model) {
    LadderDataRegion* ladder_data;
    LadderModelEntry* model_entry;
    LadderPlacement* defeated_placement;
    LadderPlacement* placement;
    LadderObjVtable* vtable;
    const char* model_name;
    AnimScript* piece_animation;
    AnimPdata* animation;
    MkObj* object;
    void* first_sobj;
    int character_id;

    ladder_data = LADDER_DATA_REGION;
    if (is_char_locked(
            current_ladder_tbl[position].character_id, 0)) {
        character_id =
            current_ladder_tbl[position].locked_character_id;
    } else {
        character_id = current_ladder_tbl[position].character_id;
    }

    model_name = 0;
    for (model_entry = ladder_data->models;
         model_entry->character_id != -1;
         model_entry++) {
        if (model_entry->character_id == character_id) {
            if (alternate_model != 0) {
                model_name = "SMOKE";
            } else {
                model_name = model_entry->model_name;
            }
            break;
        }
    }
    if (model_name == 0) {
        return;
    }

    object =
        load_named_model_from_slot(0x18006D, model_name, 0xC022, 0);
    if (object == 0) {
        return;
    }

    placement = &ladder_data->small_positions[position];
    object->pos.value.x = placement->position.x;
    object->pos.value.y = placement->position.y;
    object->pos.value.z = placement->position.z;
    object->ang.y = placement->angle_y;
    if (alternate_model != 0) {
        object->pos.value.x = 1.437f;
        object->ang.y = -0.5f;
    }

    if (curr_ladder_pos > position) {
        defeated_placement =
            &ladder_data->defeated_positions[position];
        object->pos.value.x = defeated_placement->position.x;
        object->pos.value.y = defeated_placement->position.y;
        object->pos.value.z = defeated_placement->position.z;
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

    object->hide_flag_bits.bit6 = placement->mirrored;
    object->ground_colls_y = placement->position.y;
    obj_change_to_skinned_obj_light_list(
        object, &ladder_skinned_obj_light_def);
    if (position == 0) {
        obj_add_to_skinned_obj_light_list_with_ambient(
            object, &ladder_skinned_obj_ambient_light_def);
    }

    obj_create_sobjs(object);
    first_sobj = obj_first_sobj(object);
    if (first_sobj == 0) {
        if (object->hdr.instance != 0) {
            vtable = (LadderObjVtable*)object->hdr.vtbl;
            vtable->destroy(object);
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
    case 1:
    case 3:
    case 5:
        piece_animation = bgnd_animations.pieces_one_three_five;
        break;
    case 2:
        piece_animation = bgnd_animations.piece_two;
        break;
    case 6:
        piece_animation = bgnd_animations.piece_six;
        break;
    default:
        piece_animation = bgnd_animations.default_piece;
        break;
    }

    animation = animate_obj(
        object,
        piece_animation,
        1.0f,
        ladder_piece_bones,
        0,
        ladder_piece_ground_colls,
        1);
    if (curr_ladder_pos > position) {
        if (alternate_model != 0) {
            set_anim_script_frame(
                60.0f,
                animation,
                (AniData*)bgnd_animations.defeated_piece,
                0x20);
        } else {
            set_anim_script(
                animation, (AniData*)bgnd_animations.defeated_piece, 0x20);
        }
        animation->step = 1.0f;
    }
}
