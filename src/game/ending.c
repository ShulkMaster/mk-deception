#include "game/game_info.h"
#include "game/minigames.h"
#include "mw/mwScreenEngineGlue.h"
#include "runtime/fonts.h"
#include "runtime/image.h"
#include "runtime/cam.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/section.h"
#include "runtime/utils.h"
#include "platform/gcutils.h"
#include "platform/io.h"
#include "platform/main.h"
#include "platform/main_jump.h"

#pragma use_lmw_stmw on

extern MkVtable5 vtbl_mkpdata_string_obj;
extern int screen_width;
extern int text_window_state;
extern float p_show_text_window(void);
extern float p_main_menu(void);
extern float p_credits_screen(void);
extern int char_for_ending;
extern int winner_for_ending;
extern MkFileEntry bio_text_file_table;
extern MkFileEntry endings_file_table;
extern MkFileInfo sec_ending_champion;
extern int champion_bad_guys[2];
void setup_sound_banks(int bank);
void wait_for_sound_banks_to_load(void);
int snd_req(int sound_id);
void snd_stop_all(void);
void run_ending(int fighter);
float p_character_ending_sequence(void);
char* strcpy(char* dest, const char* src);

typedef struct EndingScriptPdata {
    MkHdr hdr;
    unsigned int func_index;
    ScriptSlot* script;
} EndingScriptPdata;

typedef union EndingObjectRef {
    MkHdr* hdr;
    StringObj* string;
} EndingObjectRef;

typedef struct EndingTextWindowPdata {
    MkHdr hdr;             /* +0x000 */
    int x;                 /* +0x008 */
    int y;                 /* +0x00C */
    int font;              /* +0x010 */
    int wrap_width;        /* +0x014 */
    int field_018;         /* +0x018 */
    int oid;               /* +0x01C */
    int duration;          /* +0x020 */
    int field_024;         /* +0x024 */
    int alignment;         /* +0x028 */
    int field_02C;         /* +0x02C */
    int field_030;         /* +0x030 */
    int field_034;         /* +0x034 */
    char text[0x4B0];      /* +0x038 */
    int field_4E8;         /* +0x4E8 */
    int string_id;         /* +0x4EC */
    char pad4F0[0x70];
} EndingTextWindowPdata; /* 0x560 */

typedef union EndingTextWindowPdataOut {
    MkHdr* hdr;
    EndingTextWindowPdata* pdata;
} EndingTextWindowPdataOut;

typedef struct EndingScrollPdata {
    MkHdr hdr;
    float step;        /* +0x08 */
    float accumulator; /* +0x0C */
} EndingScrollPdata;

typedef struct EndingProcSleepVtable {
    void* reserved[6];
    int (*sleep)(void);
} EndingProcSleepVtable;

typedef struct EndingScreenObjItem {
    ScreenObj* object;
    unsigned int instance;
} EndingScreenObjItem;

typedef struct EndingDataEntry {
    int fighter;
    MkFileInfo* art_section;
    const char* script_function;
    const char* thumbnail;
    const char* image_1a;
    const char* image_1b;
    const char* image_2a;
    const char* image_2b;
    const char* image_3a;
    const char* image_3b;
    int speech_id;
} EndingDataEntry; /* 0x2C */

extern EndingScreenObjItem ending_image_1a_item;
extern EndingScreenObjItem ending_image_1b_item;
extern EndingScreenObjItem ending_image_2a_item;
extern EndingScreenObjItem ending_image_2b_item;
extern EndingScreenObjItem ending_image_3a_item;
extern EndingScreenObjItem ending_image_3b_item;
extern EndingDataEntry ending_data_table[26];
extern int ending_speech;
extern int f_ending_speech_paused;

#define ENDING_SCREEN_ITEM_OBJECT(item)                                      \
    (((item)->object != 0 && (item)->object->instance == (item)->instance) ? \
         (item)->object :                                                    \
         0)

static int scrolling_text_string_count;

static void fade_ending_screen_images(int image, int ticks);
static float p_early_out_monitor(void);
static void count_scrolling_text_strings(MkHdr* object);
static float p_scrolling_text(void);

/*
 * Soft ceiling: ending_show_text ~70.42% - the typed 0x560-byte text-window
 * pdata contract is complete; remaining differences are allocation/FP shape.
 */
void ending_show_text(int string_id, int duration) {
    EndingTextWindowPdataOut pdata;
    const char* text;
    int width;

    if (refresh_rate() == 50) {
        duration = (int)(0.799 * (double)duration);
    }

    text = get_string_by_id(string_id | 0x20000);
    if (_create_mkproc_generic_bigstack(
            0x9002, aproc->priority + 1, p_show_text_window,
            sizeof(EndingTextWindowPdata), &pdata.hdr) != 0) {
        zero_pdata_payload(sizeof(EndingTextWindowPdata), pdata.hdr);
        text_window_state = 0;
        pdata.pdata->field_4E8 = 0;
        pdata.pdata->field_018 = 0;
        width = screen_width;
        pdata.pdata->x = (int)((float)width * 0.08f);
        pdata.pdata->y = 0x56;
        pdata.pdata->font = 0x1D;
        pdata.pdata->wrap_width = (int)((float)width * 0.85f);
        pdata.pdata->oid = 0x860;
        pdata.pdata->alignment = 8;
        pdata.pdata->duration = duration;
        pdata.pdata->field_024 = -1;
        pdata.pdata->field_02C = -1;
        pdata.pdata->field_030 = 0;
        pdata.pdata->field_034 = 0;
        pdata.pdata->string_id = 0x50014;
        strcpy(pdata.pdata->text, text);
    }
}

/*
 * Soft ceiling: ~77.96% - the credits lifecycle is recovered; residual
 * differences are repeated state stores and process-pdata allocation shape.
 */
float p_credits_screen(void) {
    EndingScrollPdata* scroll_pdata;
    ScriptSlot* credits_script;
    MkHdr* pdata_hdr;

    push_game_state(0xC);
    set_process_as_scriptable(aproc);
    set_section_memory_scheme(7);
    turn_controllers_on();
    turn_camera_on();

    ending_image_1a_item.object = 0;
    ending_image_1a_item.instance = 0;
    ending_image_1b_item.object = 0;
    ending_image_1b_item.instance = 0;
    ending_image_2a_item.object = 0;
    ending_image_2a_item.instance = 0;
    ending_image_2b_item.object = 0;
    ending_image_2b_item.instance = 0;
    ending_image_3a_item.object = 0;
    ending_image_3a_item.instance = 0;
    ending_image_3b_item.object = 0;
    ending_image_3b_item.instance = 0;
    ending_speech = 0;
    f_ending_speech_paused = 0;

    load_ssf(&bio_text_file_table);
    load_string_bank(0x20000, "bio_strings_eng.mko");
    load_ssf(&endings_file_table);
    load_font(8);
    if ((int)mode_of_play == 6) {
        load_puzzle_champion_screen();
    }
    snd_req(0x1C06);
    load_screen(
        "common/credits/kontent_credits", 0x140064, 0, 1);

    pdata_hdr = 0;
    _create_mkproc_generic_nostack(
        0x209E,
        0x1F,
        p_scrolling_text,
        sizeof(EndingScrollPdata),
        &pdata_hdr);
    scroll_pdata = (EndingScrollPdata*)pdata_hdr;
    scroll_pdata->accumulator = 0.0f;
    scroll_pdata->step = 0.5f * game_speed;
    _create_mkproc_generic_tinystack(
        0x902D, 0x1F, p_early_out_monitor, 0, 0);

    load_ssf(&endings_file_table);
    credits_script =
        cmdscript_loadfile_by_name(0x10, "credits.mko");
    cmdscript_setup_execution(credits_script, 1);
    cmdscript_execute(credits_script);

    do {
        scrolling_text_string_count = 0;
        apply_to_mklist(
            count_scrolling_text_strings, &screen_obj_list);
        if (scrolling_text_string_count != 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((EndingProcSleepVtable*)aproc->vtbl)->sleep();
        }
    } while (scrolling_text_string_count != 0);

    fade_to_black(8, 1);
    _mkproc_sleep_ticks = 60.0f;
    ((EndingProcSleepVtable*)aproc->vtbl)->sleep();
    destroy_mkprocs_pid(0x902D);
    fade_to_black(8, 1);
    gamelogic_jump(6, p_main_menu);
    return -1.0f;
}

/* Soft ceiling: p_early_out_monitor ~99.83% -- residual branch/NV emit. */
static float p_early_out_monitor(void) {
    if (check_switch(g_game_info.plyr0.pad_index, 6) != 0 ||
        check_switch(g_game_info.plyr1.pad_index, 6) != 0) {
        fade_to_black(8, 1);
        gamelogic_jump(6, p_main_menu);
    }
    return 1.0f;
}

void credits_add_text(const char* center_text, const char* right_text, int monochrome) {
    StringObj* text;

    if (right_text != 0) {
        text = string_right_xy(
            0x4008, 8, right_text, screen_width - 0x20, -0x1D, 0x10);
        if (text != 0 && monochrome == 1) {
            text->pfx.instance0.rgba[2] = 0;
        }
    }

    if (center_text != 0) {
        text = string_right_xy(
            0x4008, 8, center_text, screen_width / 2, -0x1D, 0x10);
        if (text != 0 && monochrome == 1) {
            text->pfx.instance0.rgba[2] = 0;
        }
    }

    _mkproc_sleep_ticks = 58.0f;
    ((EndingProcSleepVtable*)aproc->vtbl)->sleep();
}

static void count_scrolling_text_strings(MkHdr* object) {
    EndingObjectRef text;

    if (object->vtbl == &vtbl_mkpdata_string_obj) {
        text.hdr = object;
    } else {
        text.string = 0;
    }
    if (text.string == 0) {
        return;
    }
    if (text.string->oid != 0x4008) {
        return;
    }
    scrolling_text_string_count++;
}

static void scroll_text_strings(MkHdr* object);

/* Soft ceiling: p_scrolling_text ~99.58% -- residual local/NV emit. */
static float p_scrolling_text(void) {
    EndingScrollPdata* scroll;

    scroll = (EndingScrollPdata*)apdata;
    scroll->accumulator += scroll->step;
    if (scroll->accumulator >= 1.0f) {
        scroll->accumulator -= 1.0f;
        apply_to_mklist(scroll_text_strings, &screen_obj_list);
    }
    return 1.0f;
}

static void scroll_text_strings(MkHdr* object) {
    EndingObjectRef text;

    if (object->vtbl == &vtbl_mkpdata_string_obj) {
        text.hdr = object;
    } else {
        text.string = 0;
    }

    if (text.string != 0 && text.string->oid == 0x4008) {
        text.string->render_y++;
        if (text.string->render_y >= 0x1E0 && object->instance != 0) {
            ((void (*)(MkHdr*))object->vtbl->destroy)(object);
        }
    }
}

/* Soft ceiling: p_ending_script_in_proc ~99.55% -- local -1.0f pool label only. */
static float p_ending_script_in_proc(void) {
    EndingScriptPdata* pdata;

    pdata = (EndingScriptPdata*)pdata_of_proc(aproc);
    if (pdata->func_index == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(pdata->script, pdata->func_index);
    cmdscript_execute(pdata->script);
    return -1.0f;
}

/*
 * Soft ceiling: ~82.51% - champion presentation is recovered; the duplicated
 * retail good/bad selection is retained because both routes name the same two
 * champion panes in this build.
 */
float p_champion_screen(void) {
    const char* image_a;
    const char* image_b;
    int champion;
    int is_bad;
    int screen_x;
    int ticks;
    int index;
    int port;

    push_game_state(0xC);
    champion = char_for_ending;
    if (champion != 0x2C) {
        push_game_state(0xC);
        set_process_as_scriptable(aproc);
        set_section_memory_scheme(7);
        turn_controllers_on();
        turn_camera_on();

        ending_image_1a_item.object = 0;
        ending_image_1a_item.instance = 0;
        ending_image_1b_item.object = 0;
        ending_image_1b_item.instance = 0;
        ending_image_2a_item.object = 0;
        ending_image_2a_item.instance = 0;
        ending_image_2b_item.object = 0;
        ending_image_2b_item.instance = 0;
        ending_image_3a_item.object = 0;
        ending_image_3a_item.instance = 0;
        ending_image_3b_item.object = 0;
        ending_image_3b_item.instance = 0;
        ending_speech = 0;
        f_ending_speech_paused = 0;

        load_ssf(&bio_text_file_table);
        load_string_bank(0x20000, "bio_strings_eng.mko");
        load_ssf(&endings_file_table);
        load_art_section(0x140064, &sec_ending_champion);
        snd_req(0x1AAB);

        is_bad = 0;
        for (index = 0; index < 2; index++) {
            if (champion_bad_guys[index] == champion) {
                is_bad = 1;
                break;
            }
        }
        if (is_bad) {
            image_a = "CHAMPION_A";
            image_b = "CHAMPION_B";
        } else {
            image_a = "CHAMPION_A";
            image_b = "CHAMPION_B";
        }

        if (is_widescreen_mode()) {
            screen_x = (screen_width - 0x280) / 2 - 0x40;
        } else {
            screen_x = -0x40;
        }
        load_named_2d_pfxobj_xy(
            0x140064,
            0x4002,
            image_a,
            0,
            screen_x,
            0,
            0x1E);
        load_named_2d_pfxobj_xy(
            0x140064,
            0x4003,
            image_b,
            0,
            screen_x + 0x200,
            0,
            0x1E);
        fade_from_black(0x1E, 1);
        turn_controllers_on();

        ticks = 0;
        while (ticks < 0x320) {
            if (winner_for_ending == 0) {
                port = g_game_info.plyr0.pad_index;
            } else {
                port = g_game_info.plyr1.pad_index;
            }
            if (check_switch_edge(port, 6) == 1) {
                ticks = 0x320;
            }
            _mkproc_sleep_ticks = 1.0f;
            ((EndingProcSleepVtable*)aproc->vtbl)->sleep();
            ticks++;
        }
        turn_controllers_off();
        fade_to_black(0xC, 1);
    }
    gamelogic_jump(9, p_character_ending_sequence);
    return -1.0f;
}

void ending_show_image(int image) {
    if (image == 1) {
        fade_ending_screen_images(1, 10);
        return;
    }
    if (image == 2) {
        fade_ending_screen_images(2, 30);
        return;
    }
    if (image == 3) {
        fade_ending_screen_images(3, 30);
        return;
    }
    fade_to_black(8, 1);
}

static void fade_ending_screen_images(int image, int ticks) {
    ScreenObj* image_1a;
    ScreenObj* image_1b;
    ScreenObj* image_2a;
    ScreenObj* image_2b;
    ScreenObj* image_3a;
    ScreenObj* image_3b;
    unsigned char current_alpha;
    unsigned char next_alpha;
    unsigned char current_step;
    unsigned char next_step;
    unsigned char current_final;
    unsigned char next_final;

    image_1a = ENDING_SCREEN_ITEM_OBJECT(&ending_image_1a_item);
    image_1b = ENDING_SCREEN_ITEM_OBJECT(&ending_image_1b_item);
    image_2a = ENDING_SCREEN_ITEM_OBJECT(&ending_image_2a_item);
    image_2b = ENDING_SCREEN_ITEM_OBJECT(&ending_image_2b_item);
    image_3a = ENDING_SCREEN_ITEM_OBJECT(&ending_image_3a_item);
    image_3b = ENDING_SCREEN_ITEM_OBJECT(&ending_image_3b_item);

    if (image == 1) {
        image_1a->flags &= ~0x10;
        image_1b->flags &= ~0x10;
        current_alpha = 0;
        current_step = 0xFF / (ticks + 1);
        next_alpha = 0;
        next_step = 0;
        current_final = 0xFF;
        next_final = 0;
    } else if (image == 2) {
        image_2a->flags &= ~0x10;
        image_2b->flags &= ~0x10;
        current_alpha = 0xFF;
        current_step = (unsigned char)-(unsigned char)(0xFF / (ticks + 1));
        next_alpha = 0;
        next_step = (unsigned char)-current_step;
        current_final = 0;
        next_final = 0xFF;
    } else {
        image_3a->flags &= ~0x10;
        image_3b->flags &= ~0x10;
        current_alpha = 0xFF;
        current_step = (unsigned char)-(unsigned char)(0xFF / (ticks + 1));
        next_alpha = 0;
        next_step = (unsigned char)-current_step;
        current_final = 0;
        next_final = 0xFF;
    }

    while (ticks != 0) {
        current_alpha += current_step;
        next_alpha += next_step;

        if (image == 3) {
            if (image_2a != 0) {
                pfx_2d_obj_set_alpha(image_2a, current_alpha);
            }
            if (image_2b != 0) {
                pfx_2d_obj_set_alpha(image_2b, current_alpha);
            }
            if (image_3a != 0) {
                pfx_2d_obj_set_alpha(image_3a, next_alpha);
            }
            if (image_3b != 0) {
                pfx_2d_obj_set_alpha(image_3b, next_alpha);
            }
        } else {
            if (image_1a != 0) {
                pfx_2d_obj_set_alpha(image_1a, current_alpha);
            }
            if (image_1b != 0) {
                pfx_2d_obj_set_alpha(image_1b, current_alpha);
            }
            if (image_2a != 0) {
                pfx_2d_obj_set_alpha(image_2a, next_alpha);
            }
            if (image_2b != 0) {
                pfx_2d_obj_set_alpha(image_2b, next_alpha);
            }
        }

        _mkproc_sleep_ticks = 1.0f;
        ((EndingProcSleepVtable*)aproc->vtbl)->sleep();

        image_1a = ENDING_SCREEN_ITEM_OBJECT(&ending_image_1a_item);
        image_1b = ENDING_SCREEN_ITEM_OBJECT(&ending_image_1b_item);
        image_2a = ENDING_SCREEN_ITEM_OBJECT(&ending_image_2a_item);
        image_2b = ENDING_SCREEN_ITEM_OBJECT(&ending_image_2b_item);
        image_3a = ENDING_SCREEN_ITEM_OBJECT(&ending_image_3a_item);
        image_3b = ENDING_SCREEN_ITEM_OBJECT(&ending_image_3b_item);
        ticks--;
    }

    if (image == 3) {
        if (image_2a != 0) {
            pfx_2d_obj_set_alpha(image_2a, current_final);
        }
        if (image_2b != 0) {
            pfx_2d_obj_set_alpha(image_2b, current_final);
        }
        if (image_3a != 0) {
            pfx_2d_obj_set_alpha(image_3a, next_final);
        }
        if (image_3b != 0) {
            pfx_2d_obj_set_alpha(image_3b, next_final);
        }
    } else {
        if (image_1a != 0) {
            pfx_2d_obj_set_alpha(image_1a, current_final);
        }
        if (image_1b != 0) {
            pfx_2d_obj_set_alpha(image_1b, current_final);
        }
        if (image_2a != 0) {
            pfx_2d_obj_set_alpha(image_2a, next_final);
        }
        if (image_2b != 0) {
            pfx_2d_obj_set_alpha(image_2b, next_final);
        }
    }

    if (image == 2) {
        image_1a->flags |= 0x10;
        image_1b->flags |= 0x10;
    } else if (image == 3) {
        image_2a->flags |= 0x10;
        image_2b->flags |= 0x10;
    }
}

/* Soft ceiling: p_character_ending_sequence ~99.76% -- residual call/return emit. */
float p_character_ending_sequence(void) {
    set_section_memory_scheme(7);
    if (char_for_ending != 0x2C) {
        run_ending(char_for_ending);
    }
    fade_to_black(0xF, 1);
    gamelogic_jump(0xA, p_credits_screen);
    return -1.0f;
}

/*
 * Runs the selected fighter's scripted ending. All table fields are named so
 * the six ending panes and script/audio assets remain independent of pointer
 * width assumptions in the control flow.
 * Soft ceiling: ~77.51% - the algorithm is recovered; table-index and NV
 * allocation remain compiler-shape work.
 */
void run_ending(int fighter) {
    EndingScriptPdata* script_pdata;
    EndingDataEntry* ending;
    ScreenObj* image;
    ScriptSlot* script;
    MkProc* ending_proc;
    MkHdr* pdata_hdr;
    int script_function;
    int screen_x;
    int index;

    push_game_state(0xC);
    set_process_as_scriptable(aproc);
    set_section_memory_scheme(7);
    turn_controllers_on();
    turn_camera_on();

    ending_image_1a_item.object = 0;
    ending_image_1a_item.instance = 0;
    ending_image_1b_item.object = 0;
    ending_image_1b_item.instance = 0;
    ending_image_2a_item.object = 0;
    ending_image_2a_item.instance = 0;
    ending_image_2b_item.object = 0;
    ending_image_2b_item.instance = 0;
    ending_image_3a_item.object = 0;
    ending_image_3a_item.instance = 0;
    ending_image_3b_item.object = 0;
    ending_image_3b_item.instance = 0;
    ending_speech = 0;
    f_ending_speech_paused = 0;

    load_ssf(&bio_text_file_table);
    load_string_bank(0x20000, "bio_strings_eng.mko");
    load_ssf(&endings_file_table);
    load_font(8);

    ending = 0;
    for (index = 0; index < 26; index++) {
        if (ending_data_table[index].fighter == fighter) {
            ending = &ending_data_table[index];
            break;
        }
    }
    if (ending == 0) {
        return;
    }

    load_art_section(0x2001E, ending->art_section);
    setup_sound_banks(10);
    _mkproc_sleep_ticks = 30.0f;
    ((EndingProcSleepVtable*)aproc->vtbl)->sleep();
    wait_for_sound_banks_to_load();
    ending_speech = snd_req(ending->speech_id);

    if (is_widescreen_mode()) {
        screen_x = (screen_width - 0x280) / 2 - 0x40;
    } else {
        screen_x = -0x40;
    }

#define LOAD_ENDING_IMAGE(item, oid, texture, x_pos)                  \
    do {                                                              \
        image = load_named_2d_pfxobj_xy(                              \
            0x2001E, (oid), (texture), 0, (x_pos), 0x2A, 0x1E);      \
        if (image != 0) {                                             \
            image->flags |= 0x10;                                     \
            (item).object = image;                                    \
            (item).instance = image->instance;                         \
        }                                                             \
    } while (0)

    LOAD_ENDING_IMAGE(
        ending_image_1a_item, 0x4002, ending->image_1a, screen_x);
    LOAD_ENDING_IMAGE(
        ending_image_1b_item,
        0x4003,
        ending->image_1b,
        screen_x + 0x200);
    LOAD_ENDING_IMAGE(
        ending_image_2a_item, 0x4004, ending->image_2a, screen_x);
    LOAD_ENDING_IMAGE(
        ending_image_2b_item,
        0x4005,
        ending->image_2b,
        screen_x + 0x200);
    if (ending->image_3a != 0) {
        LOAD_ENDING_IMAGE(
            ending_image_3a_item,
            0x4004,
            ending->image_3a,
            screen_x);
    }
    if (ending->image_3b != 0) {
        LOAD_ENDING_IMAGE(
            ending_image_3b_item,
            0x4005,
            ending->image_3b,
            screen_x + 0x200);
    }

#undef LOAD_ENDING_IMAGE

    script = cmdscript_loadfile_by_name(0x10, "endings.mko");
    script_function =
        get_script_function_by_name(script, ending->script_function);
    pdata_hdr = 0;
    ending_proc = _create_mkproc_generic_bigstack(
        0x20A0,
        0x1F,
        p_ending_script_in_proc,
        sizeof(EndingScriptPdata),
        &pdata_hdr);
    if (ending_proc != 0 && pdata_hdr != 0) {
        script_pdata = (EndingScriptPdata*)pdata_hdr;
        script_pdata->func_index = script_function;
        script_pdata->script = script;
        set_process_as_scriptable(ending_proc);
    }

    while (find_mkproc_pid(0x20A0) != 0) {
        int port;

        if (winner_for_ending == 0) {
            port = g_game_info.plyr0.pad_index;
        } else {
            port = g_game_info.plyr1.pad_index;
        }
        if (check_switch_edge(port, 6) == 1) {
            turn_controllers_off();
            fade_to_black(8, 1);
            if ((int)mode_of_play == 5) {
                turn_controllers_on();
                destroy_mkprocs_pid(0x20A0);
                destroy_mkprocs_pid(0x9002);
                snd_stop_all();
                break;
            }
            gamelogic_jump(10, p_credits_screen);
        }
        _mkproc_sleep_ticks = 1.0f;
        ((EndingProcSleepVtable*)aproc->vtbl)->sleep();
    }

    fade_to_black(8, 1);
    ending_speech = 0;
    f_ending_speech_paused = 0;
    delete_screen_obj_oid(0x4002);
    delete_screen_obj_oid(0x4003);
    delete_screen_obj_oid(0x4004);
    delete_screen_obj_oid(0x4005);
    delete_screen_obj_oid(0x4004);
    delete_screen_obj_oid(0x4005);
}

const char* get_ending_thumbnail_name(int fighter) {
    int index;

    for (index = 0; index < 26; index++) {
        if (ending_data_table[index].fighter == fighter) {
            return ending_data_table[index].thumbnail;
        }
    }
    return 0;
}
