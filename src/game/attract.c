#include "game/attract.h"
#include "game/game.h"
#include "game/controller.h"
#include "game/ladder.h"
#include "game/plyr.h"
#include "platform/display.h"
#include "platform/display_metrics.h"
#include "platform/main.h"
#include "platform/gcio.h"
#include "game/profile_unlock.h"
#include "game/minigames.h"
#include "game/menu.h"
#include "game/plyrprofile.h"
#include "runtime/cam.h"
#include "runtime/sound.h"
#include "platform/gcutils.h"

#include "game/game_info.h"
#include "platform/io.h"
#include "platform/main_jump.h"
#include "runtime/fonts.h"
#include "libmkparticle/pfxfont.h"
#include "runtime/image.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_vtbl.h"
#include "runtime/plyr_info.h"
#include "runtime/section.h"
#include "runtime/utils.h"
#include "movie/movie_info.h"

struct AtmObjLatch {
    MkHdr* obj;
    unsigned int instance;
};

struct BioFileEntry {
    MkFileInfo* primary;
    MkFileInfo* alt;
    int sound_id;
    int unlock_bit;
    int string_id_a;
    int string_id_b;
};

struct BioFlasherPdata {
    MkHdr hdr;
    StringObj* press_start_obj;
    unsigned int press_start_inst;
    StringObj* bio_text_obj;
    unsigned int bio_text_inst;
};

static const char stringBase0[] =
    "bio_strings_eng.mko\0"
    "PART_A\0"
    "PART_B\0";

static const float sleep_ticks_one = 1.0f;
static const float sleep_ticks_twenty = 20.0f;
static const float sleep_ticks_neg_one = -1.0f;
static const float sleep_ticks_one_point_five = 1.5f;
static const float sleep_ticks_five = 5.0f;
static const float sleep_ticks_legal = 300.0f;
static const float sleep_ticks_zero = 0.0f;
static const float sleep_ticks_bio_tap = 45.0f;
static const float bio_text_scale_a = 0.42f;
static const float bio_text_scale_b = 0.92f;
static const float bio_text_scale_c = 0.8f;

extern struct BioFileEntry bio_file_table[];
extern const MkFileEntry bios_file_table[];
extern const MkFileEntry bio_text_file_table[];
extern MkFileInfo sec_eu_biofont;

extern int next_bio_screen;
extern int b_game_timer_off;

int gap_08_805107B4_sbss;
int atm_current_page;
struct AtmObjLatch press_start_item;
struct AtmObjLatch press_start_proc_item;
static MkHdr* atm_flash_pdata;
int atm_logo_tapped_out;
int atm_movie_tapped_out;
int memcard_boot_screen_displayed;

extern float p_puzzle_fighter(void);
extern float p_mk_chess(void);

void xfer_puzzle_exit(int arg);
unsigned int get_next_bgnd(void);

static void atm_old_mkda_logo(void);
static void atm_bio_screen(void);
static float p_bio_press_start_flasher(void);
static void atm_midway_logo(void);
static float p_flash_atm_text(void);
static void post_atm_flash(void);
static void pre_atm_flash(void);
static void atm_demo_puzzle(void);
static void atm_demo_chess(void);
static void atm_demo_fight(void);
static void atm_quad_movie(void);
static void atm_intro_movie(void);
static void atm_mkda_logo(void);
static int atm_mkd_logo_tapout(void);
static int atm_movie_tapout(void);

static void mkproc_sleep(void);
static void mkproc_jump_sleep(MkProcEntryFn entry);

MkProcEntryFn atm_list[] = {
    (MkProcEntryFn)atm_midway_logo,
    (MkProcEntryFn)atm_intro_movie,
    (MkProcEntryFn)atm_mkda_logo,
    (MkProcEntryFn)atm_demo_fight,
    (MkProcEntryFn)atm_quad_movie,
    (MkProcEntryFn)atm_demo_chess,
    (MkProcEntryFn)atm_bio_screen,
    (MkProcEntryFn)atm_demo_puzzle,
    (MkProcEntryFn)atm_mkda_logo,
    (MkProcEntryFn)atm_demo_fight,
    (MkProcEntryFn)atm_quad_movie,
    (MkProcEntryFn)atm_demo_chess,
    (MkProcEntryFn)atm_bio_screen,
    (MkProcEntryFn)atm_demo_puzzle,
    (MkProcEntryFn)atm_mkda_logo,
    (MkProcEntryFn)atm_demo_fight,
    (MkProcEntryFn)atm_quad_movie,
    (MkProcEntryFn)atm_demo_chess,
    (MkProcEntryFn)atm_bio_screen,
    (MkProcEntryFn)atm_demo_puzzle,
    (MkProcEntryFn)atm_mkda_logo,
    (MkProcEntryFn)atm_demo_fight,
    (MkProcEntryFn)atm_quad_movie,
    (MkProcEntryFn)atm_demo_chess,
    (MkProcEntryFn)atm_bio_screen,
    (MkProcEntryFn)atm_demo_puzzle,
};

#define ATTRACT_PAGE_SETUP() \
    do { \
        int _ap_zero; \
        _ap_zero = 0; \
        set_section_memory_scheme(SECTION_MEMORY_SCHEME_ATTRACT); \
        g_game_info.plyr0.player_index = 0x2C; \
        g_game_info.plyr1.player_index = 0x2C; \
        g_game_info.plyr0.field_14 = _ap_zero; \
        g_game_info.plyr1.field_14 = _ap_zero; \
        g_game_info.feature_flags.bits.powerbars_locked = _ap_zero; \
        set_mode_of_play(0xD); \
        atm_logo_tapped_out = _ap_zero; \
        atm_movie_tapped_out = _ap_zero; \
        push_game_state(3); \
        set_player_state(&g_game_info.plyr0, _ap_zero); \
        set_player_state(&g_game_info.plyr1, _ap_zero); \
        g_game_info.field_1F8 = _ap_zero; \
        one_player_ladder_init(); \
        setup_sound_banks(1); \
        press_start_item.obj = 0; \
        press_start_item.instance = 0; \
        press_start_proc_item.obj = 0; \
        press_start_proc_item.instance = 0; \
        load_font(0); \
        turn_controllers_on(); \
    } while (0)
static int attract_widescreen_x(void);
static void destroy_pfx_link(MkHdr* obj);
static void set_game_info_flag_bit5(int value);
static int check_start_or_a(void);
static int atm_tapout_scan(void);

static void mkproc_sleep(void) {
    aproc->vtbl->sleep();
}

static void mkproc_jump_sleep(MkProcEntryFn entry) {
    aproc->vtbl->jump_sleep(entry, sleep_ticks_zero);
}

static void set_game_info_flag_bit5(int value) {
    if (value != 0) {
        g_game_info.feature_flags.bits.powerbars_locked = 1;
    } else {
        g_game_info.feature_flags.bits.powerbars_locked = 0;
    }
}

static int attract_widescreen_x(void) {
    int x;

    if (is_widescreen_mode() != 0) {
        x = (screen_width - 0x280) / 2;
        return x - 0x40;
    }
    return -0x40;
}

static void destroy_pfx_link(MkHdr* obj) {
    void (*destroy_fn)(MkHdr*);

    if (obj == 0) {
        return;
    }
    if (obj->instance == 0) {
        return;
    }
    destroy_fn = (void (*)(MkHdr*))obj->vtbl->destroy;
    destroy_fn(obj);
}

static int check_start_or_a(void) {
    if (check_switch_edge_any_pad(0xB) != 0) {
        return 1;
    }
    if (check_switch_edge_any_pad(6) != 0) {
        return 1;
    }
    return 0;
}

static int atm_tapout_scan(void) {
    scan_switches();
    if (check_switch_edge_any_pad(0xB) != 0) {
        return 1;
    }
    if (check_switch_edge_any_pad(6) != 0) {
        return 1;
    }
    return 0;
}

static StringObj* press_start_item_live(void) {
    StringObj* item;
    StringObj* live;

    item = (StringObj*)press_start_item.obj;
    live = MK_LIVE(item, press_start_item.instance);
    return live;
}

static void atm_setup_press_start_flasher(void) {
    MkProc* proc;
    MkHdr* pdata;
    StringObj* str_obj;
    const char* text;
    int x;

    proc = _create_mkproc_generic_tinystack(0x2005, 0x1F, p_flash_atm_text,
                                                                     0xC, &pdata);
    if (proc != 0) {
        proc->pre_destroy = pre_atm_flash;
        proc->destroy_cb = post_atm_flash;
        press_start_proc_item.obj = (MkHdr*)proc;
        press_start_proc_item.instance = proc->instance;
    }

    text = get_string(1);
    x = screen_width / 2;
    str_obj = string_center_xy(0x2010, 0, text, x, 0x41, 0x1D);
    if (str_obj != 0) {
        press_start_item.obj = (MkHdr*)str_obj;
        press_start_item.instance = str_obj->instance;
    }
}

static void atm_old_mkda_logo(void) {
    StringObj* start_item;
    int frame;
    int pressed;
    int x;

    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    load_ssf((MkFileEntry*)attract_file_table);
    load_art_section(0x90046, &sec_attract);
    atm_setup_press_start_flasher();

    _mkproc_sleep_ticks = sleep_ticks_one;
    mkproc_sleep();

    x = attract_widescreen_x();
    load_2d_pfxobj_xy(0x90046, 0x2018, 0x01460000, 0, x, 0, 0x1E);
    load_2d_pfxobj_xy(0x90046, 0x2018, 0x01460001, 0, x + 0x200, 0, 0x1E);
    load_font(0);
    turn_camera_on();
    fade_from_black(8, 1);

    frame = 0;
    while (frame < 0x708) {
        pressed = check_start_or_a();
        if (pressed != 0) {
            turn_controllers_off();
            start_item = press_start_item_live();
            destroy_pfx_link((MkHdr*)start_item);
            set_game_info_flag_bit5(0);
            reset_game_speed();
            snd_req(0x1B47);
            fade_to_black(4, 1);
            turn_display_off();
            gamelogic_jump(6, p_main_menu);
        }

        randu0(0x1F4);
        _mkproc_sleep_ticks = sleep_ticks_one;
        mkproc_sleep();
        frame++;
    }

    fade_to_black(8, 1);
    turn_display_off();
    gamelogic_jump(0, p_atm_loop);
}

static int gp_unlock_bit_set(unsigned int hi, unsigned int lo, int bit) {
    unsigned long long bits;
    unsigned long long mask;

    bits = ((unsigned long long)hi << 32) | (unsigned long long)lo;
    mask = 1ULL << (unsigned int)bit;
    return (bits & mask) != 0ull;
}

/* TODO: [breakthrough needed] 83.52%; page setup and load order still differ. */
static void atm_bio_screen(void) {
    int bio_index;
    int use_alt;
    int unlock_bit;
    int sound_id;
    int match;
    int i;
    int frame;
    int pressed;
    int x;
    MkProc* flasher;
    struct BioFlasherPdata* pdata;
    StringObj* str_obj;
    const char* text;
    struct BioFileEntry* entry;
    PfxFontSlot* font;
    int text_x;
    int text_y;
    int text_y_off;
    unsigned char color[4];

    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;

    if (next_bio_screen < 0) {
        next_bio_screen = (randu0(0x1AU) & 0xFFFFU);
    }

    for (;;) {
        bio_index = next_bio_screen;
        unlock_bit = bio_file_table[bio_index].unlock_bit;
        if (gp_unlock_bit_set(gp_data.cat7.words[0], gp_data.cat7.words[1], unlock_bit) != 0) {
            break;
        }
        next_bio_screen = bio_index + 1;
        if ((unsigned int)next_bio_screen < 0x1AU) {
            continue;
        }
        next_bio_screen = 0;
    }

    use_alt = 0;
    if ((randu0(2U) & 0xFFFFU) != 0) {
        unlock_bit = bio_file_table[bio_index].unlock_bit;
        if (gp_unlock_bit_set(gp_data.cat8.words[0], gp_data.cat8.words[1], unlock_bit) != 0) {
            use_alt = 1;
        }
    }

    next_bio_screen = (bio_index + 1) % 0x1A;
    load_font(0);

    flasher = _create_mkproc_generic_tinystack(
        0x2005, 0x1F, p_bio_press_start_flasher, sizeof(*pdata), (MkHdr**)&pdata);
    if (flasher != 0 && pdata != 0) {
        pdata->press_start_obj = 0;
        pdata->press_start_inst = 0;
        pdata->bio_text_obj = 0;
        pdata->bio_text_inst = 0;
        text = get_string(1);
        str_obj = string_center_xy(0x2010, 0, text, 0xA6, 0x2E, 0x1D);
        if (str_obj != 0) {
            pdata->press_start_obj = str_obj;
            pdata->press_start_inst = str_obj->instance;
        }
    }

    load_ssf((MkFileEntry*)bio_text_file_table);
    load_string_bank(0x20000U, (char*)&stringBase0[0]);
    load_ssf((MkFileEntry*)bios_file_table);
    load_art_section(0x90046, &sec_eu_biofont);
    load_font(0x10);

    entry = &bio_file_table[bio_index];
    if (use_alt != 0) {
        add_art_section(0x90046, entry->alt);
    } else {
        add_art_section(0x90046, entry->primary);
    }

    sound_id = entry->sound_id;
    unlock_bit = entry->unlock_bit;
    match = -1;
    for (i = 0; i < 0x1A; i++) {
        if (bio_file_table[i].unlock_bit == unlock_bit) {
            match = i;
            break;
        }
    }

    if (match != -1) {
        int height;

        entry = &bio_file_table[match];
        if (use_alt != 0) {
            text = get_string_by_id(entry->string_id_b | 0x20000);
        } else {
            text = get_string_by_id(entry->string_id_a | 0x20000);
        }

        height = screen_height;
        font = load_font(0x10);
        text_x = (bio_text_scale_a * (float)screen_width);
        text_y = (bio_text_scale_b * (float)height);
        text_y_off = (bio_text_scale_c * (float)height);

        str_obj = create_wrapped_string(0x9017, font, text, text_x, text_y, 0x14A, text_y_off, 0, 1);
        str_obj->priority = 0x10;
        color[0] = 0xFF;
        color[1] = 0xFF;
        color[2] = 0xFF;
        color[3] = 0xFF;
        pfxfont_set_string_color(&str_obj->pfx, (unsigned int*)color);
        insert_string_obj((ScreenObj*)str_obj);
    }

    x = (screen_width - 0x300) / 2;
    load_named_2d_pfxobj_xy(0x90046, 0x4006, &stringBase0[0x14], 0, x, 0, 0x1E);
    load_named_2d_pfxobj_xy(0x90046, 0x4007, &stringBase0[0x1B], 0, x + 0x200, 0, 0x1E);

    turn_camera_on();
    fade_from_black(0xC, 1);

    frame = 0;
    while (frame < 0x708) {
        if (frame == 0x2D) {
            snd_req(sound_id);
        }

        pressed = check_start_or_a();
        if (pressed != 0) {
            atm_current_page = 2;
            g_game_info.field_210 = g_game_info.field_210 + 1;
            snd_req(0x1B47);
            _mkproc_sleep_ticks = sleep_ticks_bio_tap;
            mkproc_sleep();
            break;
        }

        _mkproc_sleep_ticks = sleep_ticks_one;
        mkproc_sleep();
        frame++;
    }

    fade_to_black(0xC, 1);
    gamelogic_jump(0, p_atm_loop);
}

static inline int find_bio_unlock_index(int unlock_bit)
{
    int index;
    for (index = 0; index < 0x1A; index++) {
        if (bio_file_table[index].unlock_bit == unlock_bit) {
            return index;
        }
    }
    return -1;
}

StringObj* put_bio_text(int unlock_bit, int use_alt) {
    int index;
    const char* text;
    int height;
    StringObj* str_obj;
    unsigned char color[4];

    index = find_bio_unlock_index(unlock_bit);
    if (index == -1) {
        return 0;
    }

    if (use_alt != 0) {
        text = get_string_by_id(bio_file_table[index].string_id_b | 0x20000);
    } else {
        text = get_string_by_id(bio_file_table[index].string_id_a | 0x20000);
    }

    height = screen_height;
    str_obj = create_wrapped_string(
        0x9017, load_font(0x10), text, (bio_text_scale_a * (float)screen_width),
        (bio_text_scale_b * (float)height), 0x14A,
        (bio_text_scale_c * (float)height), 0, 1);
    str_obj->priority = 0x10;
    color[0] = 0xFF;
    color[1] = 0xFF;
    color[2] = 0xFF;
    color[3] = 0xFF;
    pfxfont_set_string_color(&str_obj->pfx, (unsigned int*)color);
    insert_string_obj((ScreenObj*)str_obj);
    return str_obj;
}

static float p_bio_press_start_flasher(void) {
    struct BioFlasherPdata* pdata;
    StringObj* raw;
    StringObj* obj;

    pdata = (struct BioFlasherPdata*)apdata;

    raw = pdata->press_start_obj;
    obj = MK_LIVE(raw, pdata->press_start_inst);
    if (obj != 0) {
        obj->visibility.hidden = 0;
    }

    raw = pdata->bio_text_obj;
    obj = MK_LIVE(raw, pdata->bio_text_inst);
    if (obj != 0) {
        obj->visibility.hidden = 0;
    }

    _mkproc_sleep_ticks = sleep_ticks_twenty;
    mkproc_sleep();

    raw = pdata->press_start_obj;
    obj = MK_LIVE(raw, pdata->press_start_inst);
    if (obj != 0) {
        obj->visibility.hidden = 1;
    }

    raw = pdata->bio_text_obj;
    obj = MK_LIVE(raw, pdata->bio_text_inst);
    if (obj != 0) {
        obj->visibility.hidden = 1;
    }

    return sleep_ticks_twenty;
}

void atm_reset_current_page(int page) {

    atm_current_page = page;
    g_game_info.field_210 = g_game_info.field_210 + 1;
}

static void atm_midway_logo(void) {
    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    play_movie(0x2A, atm_movie_tapout);
    turn_controllers_off();
    gamelogic_jump(0, p_atm_loop);
}

static float p_flash_atm_text(void) {
    StringObj* item;

    _mkproc_sleep_ticks = sleep_ticks_twenty;
    mkproc_sleep();

    item = press_start_item_live();
    if (item != 0) {
        item->visibility.hidden = 0;
    }

    _mkproc_sleep_ticks = sleep_ticks_twenty;
    mkproc_sleep();

    item = press_start_item_live();
    if (item != 0) {
        item->visibility.hidden = 1;
    }

    return sleep_ticks_one;
}

static void post_atm_flash(void) {
    atm_flash_pdata = 0;
}

static void pre_atm_flash(void) {
    atm_flash_pdata = apdata;
}

static void atm_demo_puzzle(void) {
    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    set_game_info_flag_bit5(1);
    gamelogic_jump(3, p_puzzle_fighter);
}

static void atm_demo_chess(void) {
    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    rnd_plyrs();
    set_game_info_flag_bit5(1);
    set_mode_of_play(9);
    gamelogic_jump(5, p_mk_chess);
}

/* TODO: [near miss] 99.53%; demo flag byte, true bit, and timer zero use different GPRs. */
static void atm_demo_fight(void) {
    int bgnd;

    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    rnd_plyrs();
    bgnd = get_next_bgnd();
    g_game_info.bgnd_id = bgnd;
    b_game_timer_off = 0;
    g_game_info.feature_flags.bits.powerbars_locked = 1;
    set_mode_of_play(0);
    gamelogic_jump(2, p_gamelogic);
}

static void atm_quad_movie(void) {
    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    play_movie(0x2B, atm_movie_tapout);
    if (atm_movie_tapped_out != 0) {
        atm_current_page = 2;
        g_game_info.field_210++;
    }
    turn_controllers_off();
    gamelogic_jump(0, p_atm_loop);
}

static void atm_intro_movie(void) {
    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    play_movie(MOVIE_ID_INTRO, atm_movie_tapout);
    turn_controllers_off();
    gamelogic_jump(0, p_atm_loop);
}

static void atm_mkda_logo(void) {
    int lang;

    ATTRACT_PAGE_SETUP();
    g_game_info.field_1F8 = 0;
    lang = get_language();
    if (lang == 3) {
        if (play_movie(0x2C, atm_mkd_logo_tapout) == 0) {
            atm_old_mkda_logo();
        }
    } else {
        if (play_movie(0x29, atm_mkd_logo_tapout) == 0) {
            atm_old_mkda_logo();
        }
    }

    turn_controllers_off();
    if (atm_logo_tapped_out != 0) {
        turn_display_off();
        gamelogic_jump(6, p_main_menu);
    } else {
        turn_display_off();
        gamelogic_jump(0, p_atm_loop);
    }
}

float p_atm_start_button(void) {
    GameInfo* game;
    MkProc* proc;

    game = &g_game_info;
    if (game->feature_flags.bits.powerbars_locked == 0) {
        return sleep_ticks_neg_one;
    }

    if (__mini_game_display_ctrl != 0) {
        xfer_puzzle_exit(0);
        return sleep_ticks_neg_one;
    }

    turn_controllers_off();
    game->feature_flags.bits.powerbars_locked = 0;
    proc = aproc;
    proc->flags_bits.skip_if_paused = 1;
    pause_procs(1);
    snd_req_vol(0x1AA5, sleep_ticks_one_point_five);
    _mkproc_sleep_ticks = sleep_ticks_five;
    mkproc_sleep();
    fade_to_black(0xC, 1);
    pause_procs(0);
    atm_current_page = 2;
    g_game_info.field_210 = g_game_info.field_210 + 1;
    gamelogic_jump(0, p_atm_loop);
    return sleep_ticks_neg_one;
}

float p_atm_loop(void) {
    MkProcEntryFn page;

    unassign_player(&g_game_info.plyr0);
    unassign_player(&g_game_info.plyr1);
    g_game_info.plyr0.player_state = 0;
    g_game_info.plyr1.player_state = 0;
    push_game_state(3);

    if (atm_current_page >= 0x1A) {
        atm_current_page = 1;
        g_game_info.field_210 = g_game_info.field_210 + 1;
    }

    page = atm_list[atm_current_page++];
    page();
    return sleep_ticks_neg_one;
}

float p_attract_mode(void) {
    GameInfo* game;
    int zero;
    int bgnd_slot;
    int x;
    int x2;
    ScreenObj* legal_b;
    ScreenObj* legal_a;
    void (*destroy_fn)(MkHdr*);

    zero = 0;
    set_section_memory_scheme(SECTION_MEMORY_SCHEME_ATTRACT);
    game = &g_game_info;
    game->plyr0.player_index = 0x2C;
    game->plyr1.player_index = 0x2C;
    game->plyr0.field_14 = zero;
    game->plyr1.field_14 = zero;
    game->feature_flags.bits.powerbars_locked = zero;
    set_mode_of_play(0xD);
    atm_logo_tapped_out = zero;
    atm_movie_tapped_out = zero;
    push_game_state(3);
    set_player_state(&game->plyr0, zero);
    set_player_state(&game->plyr1, zero);
    g_game_info.field_1F8 = zero;
    one_player_ladder_init();
    setup_sound_banks(1);
    press_start_item.obj = 0;
    press_start_item.instance = 0;
    press_start_proc_item.obj = 0;
    press_start_proc_item.instance = 0;
    load_font(0);
    turn_controllers_on();

    bgnd_slot = 0x23;
    g_game_info.field_1F8 = zero;
    g_game_info.field_210 = zero;
    game->bgnd_id = bgnd_slot;

    if (((unsigned int)__cntlzw(memcard_boot_screen_displayed) >> 5) != 0) {
        load_ssf((MkFileEntry*)attract_file_table);
        load_art_section_language(SEC_SLOT_HANDLE_ATTRACT_LEGAL, &sec_legal_screen);

        if (is_widescreen_mode() != 0) {
            x = ((screen_width - 0x280) / 2) - 0x40;
        } else {
            x = -0x40;
        }

        legal_a = load_2d_pfxobj_xy(SEC_SLOT_HANDLE_ATTRACT_LEGAL, 0x2018, 0x017E0000, 0, x,
                                    0, 0x1E);
        x2 = x + 0x200;
        legal_b = load_2d_pfxobj_xy(SEC_SLOT_HANDLE_ATTRACT_LEGAL, 0x2018, 0x017E0001, 0,
                                    x2, 0, 0x1E);

        turn_camera_on();
        fade_from_black(8, 1);
        _mkproc_sleep_ticks = sleep_ticks_legal;
        aproc->vtbl->sleep();
        fade_to_black(8, 1);
        destroy_fade_box();
        turn_camera_off();

        if (legal_a != 0) {
            if (legal_a->instance != 0U) {
                destroy_fn = (void (*)(MkHdr*))legal_a->vtbl->destroy;
                destroy_fn((MkHdr*)legal_a);
            }
        }
        if (legal_b != 0) {
            if (legal_b->instance != 0U) {
                destroy_fn = (void (*)(MkHdr*))legal_b->vtbl->destroy;
                destroy_fn((MkHdr*)legal_b);
            }
        }

        memcard_boot_screen_displayed = 1;
        gamelogic_jump(6, p_player_profile_boot_screen_entry_point);
    }

    aproc->vtbl->jump_sleep(p_atm_loop, sleep_ticks_zero);
    return sleep_ticks_zero;
}

static int atm_mkd_logo_tapout(void) {
    if (atm_tapout_scan() != 0) {
        turn_controllers_off();
        atm_logo_tapped_out = 1;
        return 1;
    }
    return 0;
}

static int atm_movie_tapout(void) {
    if (atm_tapout_scan() != 0) {
        atm_movie_tapped_out = 1;
        return 1;
    }
    return 0;
}
