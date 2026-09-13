#include "game/ladder.h"
#include "game/plyr.h"
#include "game/cloth.h"
#include "platform/io.h"
#include "platform/main_jump.h"
#include "runtime/cam.h"
#include "runtime/section.h"
#include "runtime/sound.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"
void* memcpy(void* destination, const void* source, unsigned long size);

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
    AniData* other_paths[25];
    AniData* intro_camera; /* +0x78 */
    AniData* travel_camera; /* +0x7C */
} LadderBgndAnimations;

typedef struct LadderObjVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    void (*destroy)(MkObj* object);
} LadderObjVtable;

typedef struct LadderStringRef {
    StringObj* object;
    unsigned int instance;
} LadderStringRef;

/*
 * Retail lays the ladder tables in one contiguous TU-local data region.
 * Keeping that relationship typed lets MWCC use the shared data base without
 * scattering byte-offset arithmetic through the ladder screen code.
 */
typedef struct LadderHudEntry {
    int background_id;
    char* texture_name;
    int x;
    int y;
} LadderHudEntry;

typedef struct LadderCharacterTexture {
    int character_id;
    char* texture_name;
} LadderCharacterTexture;

typedef struct LadderDataRegion {
    LadderCoinType coin_offsets[6]; /* +0x00 */
    const char* ladder_koins[4];    /* +0x30 */
    int koin_awards[9];             /* +0x40 */
    int puzzle_koin_awards[7];      /* +0x64 */
    int chess_koin_awards[7];       /* +0x80 */
    LadderHudEntry ladder_hud[35];     /* +0x9C */
    LadderHudEntry puzzle_hud[6]; /* +0x2CC */
    LadderCharacterTexture puzzle_characters[12]; /* +0x32C */
    char pad38C[0x974];
    float camera_frames[8][2]; /* +0xD00 */
    LadderPlacement player_positions[8]; /* +0xD40 */
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

/* TODO: [breakthrough needed] 49.333332%; retail retains float constant
 * conversions; constant-folding control is neutral; need source-boundary evidence. */
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

/* TODO: [near miss] 97.15909%; compiler retains byte offset across background
 * query instead of retail index; strength-reduction control is neutral. */
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

        y = ladder_data->ladder_hud[background_id].y;
        if ((int)mode_of_play == 6) {
            y += 60;
        }
        arena_name = string_center_xy(
            0x209A,
            9,
            text,
            ladder_data->ladder_hud[background_id].x,
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

extern MkFileEntry gameart_file_table[];
extern MkFileInfo sec_fightingart;
extern unsigned char char_piece_ground_colls[];
extern const char* pz_ladder_koins[2];
extern unsigned short n_pz_ladder_koins;
extern PlyrPdata* his_pdata;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
float p_puzzle_fighter(void);
float p_gamelogic(void);
float p_animate(void);
float p_animated_intro_done(void);
int move_to_end_point(const Vec* endpoint, float* initial_speed,
                     float* final_speed, int mode, float rate);
void tag_team_activate_player(MkObj* object, int player);
MkProc* create_mkproc_headtracking(int pid, MkObj* object, PlyrPdata* pdata);
void ground_me(MkObj* object);

typedef struct LadderProcessVtable {
    MkVtblFn functions[4];
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
} LadderProcessVtable;

static inline void ladder_sleep(float ticks) {
    _mkproc_sleep_ticks = ticks;
    ((LadderProcessVtable*)aproc->vtbl)->sleep();
}
/* Recovered p_ladder_select (0x8008E6F4, retail size0xDD8).
 * Both ladder presentations retain their
 * authored camera, opponent-selection, award and game-state transitions. */
/* TODO: [breakthrough needed] 82.87133%; canonical script fields improve
 * matching; remaining reconstruction differences require localized recovery. */
float p_ladder_select(void) {
    PlyrInfo* opponent;
    PlyrInfo* player;
    MkProc* tracking;
    CameraPdata* camera;
    CameraPdata* saved_camera;
    LadderDataRegion* data;
    int i;
    int j;
    int character;
    int flags;
    int frames;
    int resume_frames;
    int alpha;
    int award;
    int difficulty;
    int coin_type;
    float saved_speed;
    float initial_speed;
    float final_speed;
    CamVec3 position;
    CamVec3 angle = {0.0326f, 3.1415927f, 0.0f};
    StringObj* arena_name;
    const char* coin;

    data = LADDER_DATA_REGION;
    tracking = 0;
    set_section_memory_scheme((int)mode_of_play == 6 ? 0 : 11);
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
        opponent->player_state = 0;
        opponent->field_04 = 1;
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
    load_background((int)mode_of_play == 6 ? 23 : 22);
    if ((int)mode_of_play == 6) {
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 6; j++) {
                if (current_ladder_tbl[i].background_id ==
                    data->puzzle_hud[j].background_id) break;
            }
            if (j >= 6) j = 0;
            bgnd_append_texture_to_material(8-i, 28-i,
                data->puzzle_hud[j].texture_name, 0);
            bgnd_swap_textures(8-i, 28-i, 1);
            character = current_ladder_tbl[i].character_id;
            if (is_char_locked(character, character == 21))
                character = current_ladder_tbl[i].locked_character_id;
            for (j = 0; j < 12; j++) {
                if (character == data->puzzle_characters[j].character_id) break;
            }
            if (j >= 12) j = 0;
            bgnd_append_texture_to_material(18-i, 38-i,
                data->puzzle_characters[j].texture_name, 0);
            bgnd_swap_textures(18-i, 38-i, 1);
        }
    } else {
        for (i = 0; i < 8; i++) {
            int background = current_ladder_tbl[i].background_id;
            if (is_bgnd_locked(background))
                background = current_ladder_tbl[i].locked_background_id;
            bgnd_append_texture_to_material(18-i, 8-i,
                data->ladder_hud[background].texture_name, 0);
            bgnd_swap_textures(18-i, 8-i, 1);
            if (i != curr_ladder_pos && i != 7) {
                place_plyr_on_ladder(i, 0);
                if (i == 6) place_plyr_on_ladder(i, 1);
            } else if (i == 6 && i == curr_ladder_pos) {
                place_plyr_on_ladder(i, 1);
            }
        }
    }
    character = current_ladder_tbl[curr_ladder_pos].character_id;
    if (is_char_locked(character, 0))
        character = current_ladder_tbl[curr_ladder_pos].locked_character_id;
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
        if ((int)mode_of_play == 6) snd_req(0x1AA1);
        camera = get_pdata_of_camera();
        camera->speed = 1.5f * game_speed;
        camera_init_animation(bgnd_animations.intro_camera, p_animated_intro_done);
        if ((int)mode_of_play != 6) camera->speed = 0.5f * game_speed;
        camera_run_animation(0);
    } else {
        snd_req((int)mode_of_play == 6 ? 0x1AA2 : 0x1AA0);
        if ((int)mode_of_play == 6) {
            position.x = 0.0f;
            position.y = 3.571f * (float)(curr_ladder_pos-1) + -26.283203f;
            position.z = 7.376953f;
            go_to_camera_cut_with_angle(&position, &angle);
        } else {
            camera_init_animation(bgnd_animations.travel_camera, p_animated_intro_done);
            camera_run_animation_start_end(data->camera_frames[curr_ladder_pos][0],
                data->camera_frames[curr_ladder_pos][1], 0, 1);
        }
    }
    get_pdata_of_camera()->speed = 1e-10f;
    turn_camera_on();
    fade_from_black(20, 0);
    camera = get_pdata_of_camera();
    camera->speed = 1.5f * game_speed;
    if (curr_ladder_pos == 0 && (int)mode_of_play != 6) {
        frames = (int)(72.0f * inverse_game_speed);
        ladder_sleep((float)frames);
        snd_req(0x1A9F);
        if (camera != 0) {
            frames = (int)(22.0f * inverse_game_speed);
            for (i = 0; i < frames; i++) {
                camera->speed *= 0.9f;
                ladder_sleep(1.0f);
            }
            camera->speed *= 1e-7f;
        }
        saved_camera = get_pdata_of_camera();
        saved_speed = saved_camera->speed;
        saved_camera->speed = saved_speed * 1e-9f;
        create_player(opponent->field_04, opponent);
        saved_camera->speed = saved_speed;
        resume_frames = game_speed == 1.2f ? frames + 6 : frames;
        for (i = 0; i < resume_frames; i++) {
            camera->speed += 2.0f / (float)frames;
            ladder_sleep(1.0f);
        }
    } else if ((int)mode_of_play == 6) {
        initial_speed = 0.0f;
        final_speed = 0.0f;
        position.x = 0.0f;
        position.y = 3.572f * (float)curr_ladder_pos + -26.283203f;
        position.z = 7.376953f;
        while (!move_to_end_point((const Vec*)&position, &initial_speed,
                                 &final_speed, 0, 2.0f)) ladder_sleep(1.0f);
    }
    if ((int)mode_of_play == 0) {
        MkObj* object;
        MkProc* animation_proc;
        AnimPdata* animation;
        PlyrPdata* pdata;
        unsigned int script;
        LadderPlacement* placement;
        if (curr_ladder_pos > 0) {
            saved_camera = get_pdata_of_camera();
            saved_speed = saved_camera->speed;
            saved_camera->speed = saved_speed * 1e-9f;
            create_player(opponent->field_04, opponent);
            saved_camera->speed = saved_speed;
        }
        if (opponent->player_index == 27)
            tag_team_activate_player(opponent->slot.mirror_a, randu0(1));
        object = opponent->slot.mirror_a;
        if (object != 0) {
            placement = &data->player_positions[curr_ladder_pos];
            object->pos.value = placement->position;
            object->ang.y = placement->angle_y;
            object->hide_flag_bits.bit6 = placement->mirrored;
            object->flags_09_bits.launched = 1;
            object->flags_09_bits.bit6 = 1;
            object->ground_colls = char_piece_ground_colls;
            g_game_info.field_34 = placement->position.y - 10.0f;
            object->ground_colls_y = placement->position.y;
            cloth_change_ground_plane_for(g_game_info.field_34);
            ground_me(object);
            pdata = opponent->slot.pdata;
            animation_proc = pdata->anim_proc;
            if (animation_proc == 0 || animation_proc->hdr.instance != pdata->anim_proc_instance)
                animation_proc = 0;
            animation = (AnimPdata*)pdata_of_proc(animation_proc);
            set_anim_script(animation, pdata->fighter_definition->duck_exit_animation, 0x20);
            animation->script = (AnimScript*)pdata->fighter_definition->duck_exit_animation;
            if (opponent->player_index != 12)
                tracking = create_mkproc_headtracking(0x6006, object, pdata);
            if (opponent->flags_14_bits.alternate_costume)
                script = pdata->runtime_data->alternate_script_48;
            else
                script = pdata->runtime_data->primary_script_24;
            if (script != 0) {
                CmdScript* previous_script = active_cmdscript;
                PlyrPdata* previous_player = plyr_pdata;
                PlyrPdata* previous_opponent = his_pdata;
                MkObj* previous_object = plyr_obj;
                MkObj* previous_other_object = his_obj;
                plyr_pdata = pdata;
                his_pdata = 0;
                plyr_obj = object;
                his_obj = 0;
                active_cmdscript = &global_script_interpreter;
                cmdscript_setup_execution(pdata->cmo, script);
                cmdscript_execute(pdata->cmo);
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
    arena_name = 0;
    alpha = 0;
    while (alpha < 255) {
        arena_name = bgnd_name_item.object;
        if (arena_name == 0 || arena_name->instance != bgnd_name_item.instance)
            arena_name = 0;
        /* Retail advances alpha only while its string remains valid. */
        if (arena_name != 0) {
            set_string_obj_alpha(arena_name, (float)(unsigned char)alpha);
            alpha += (signed char)(8.0f * game_speed);
            ladder_sleep(1.0f);
        }
    }
    set_string_obj_alpha(arena_name, 255.0f);
    if ((int)mode_of_play == 6) {
        if ((g_game_info.plyr0.player_state == 2 && p1_profile_status == 1) ||
            (g_game_info.plyr1.player_state == 2 && p2_profile_status == 1)) {
            award = 0;
            if (curr_ladder_pos >= 0 && curr_ladder_pos <= 6) {
                award = data->puzzle_koin_awards[curr_ladder_pos] * 5;
                difficulty = game_settings.rounds_to_win;
                if (difficulty == 0) award = (int)(0.5f * (float)award);
                else if (difficulty == 1) award = (int)(0.75f * (float)award);
                else if (difficulty == 3) award = (int)(1.1f * (float)award);
                else if (difficulty == 4) award = (int)(1.25f * (float)award);
            }
            g_game_info.pselect.field_1e8 = award;
            coin = pz_ladder_koins[randu0(n_pz_ladder_koins) & 0xFFFF];
            coin_type = 0;
            for (i = 0; i < 6; i++) {
                if (strcmp(coin_offset_tbl[i].name, coin) == 0) {
                    coin_type = coin_offset_tbl[i].type;
                    break;
                }
            }
            g_game_info.pselect.field_1e4 = coin_type;
            show_koin_award(0, award, coin_type, 0x23);
            ladder_sleep(30.0f);
        }
        ladder_sleep(90.0f);
    } else ladder_sleep(90.0f * inverse_game_speed);
    fade_to_black(10, 1);
    if (tracking != 0 && tracking->hdr.instance != 0)
        ((LadderProcessVtable*)tracking->hdr.vtbl)->destroy(tracking);
    if ((int)mode_of_play == 6) gamelogic_jump(3, p_puzzle_fighter);
    gamelogic_jump(2, p_gamelogic);
    return -1.0f;
}
