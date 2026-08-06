#include "game/krypt.h"

#include "game/bgnd.h"
#include "game/attract.h"
#include "game/game_info.h"
#include "game/menu.h"
#include "runtime/cam.h"
#include "runtime/fonts.h"
#include "runtime/light.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_proc.h"
#include "runtime/section.h"
#include "runtime/utils.h"

/*
 * krypt.o - Wave 3 NonMatching scaffold (display-only MVP).
 * Retail krypt entry is p_krypt_mode (title/menu/attract are separate paths).
 * Agent B: pdata layout / getters / tombstone helpers.
 * Agent A: mode shell (p_krypt_mode / init / setup / loop).
 * Retail emission order preserved. Deferred clusters are empty stubs.
 *
 * Port readiness:
 *   Structs: MISSING
 *   Matching: 19.41% (.text)
 *   Linked: NO
 *   Status: SCAFFOLD
 *   Gaps: major mode payloads, PFX userdata, and render records remain unresolved
 */

/* --- callees outside this TU (prototypes only) --- */
int get_coffin_bit(const unsigned char* bits, unsigned int index);
void* pfx_get_field(void* pfx, int index, int type);
int pfx_get_struct_size(void* pfx, int type);
void pfx_texture_animate(float rate, void* pfx, int a, int b, int c, int d);
void RwResourcesSetArenaSize(int size);
int move_profile_p1_to_p2(void);
void set_player_state(PlyrInfo* plyr, int state);
void gamelogic_jump(int action, MkProcEntryFn logic);
float p_atm_loop(void);
void zero_pdata_payload(int size, MkHdr* pdata);
int get_menu_mode_sub_var(void);
void turn_controllers_on(void);
void disable_all_ports_but_me(int port);
void fire_screen_studio_event(int event, int arg);
void pause_screen_engine(int paused);
void wait_for_screen_close(void);
void load_screen(const char* path, int slot, MkHdr* share, int unload);
void eat_switch_edge(int player, int action);
int check_switch_edge(int player, int action);
void scan_switches(void);
int run_ending(int fighter);
int play_movie(int movie, int (*tapout)(void));
char* get_ending_thumbnail_name(int fighter);
int is_mark_as_unlocked(void* profile, int category, int index);
RwTexture* load_named_tga_from_slot(int slot, const char* name);
RwTexture* load_tga(int slot, unsigned int oid);
void load_string_bank(unsigned int bank, char* path);
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
    int (*jump_sleep)(MkProcEntryFn entry);
} MkVtableMkprocLocal;

typedef struct KontentPdata {
    MkHdr hdr;                    /* +0x000 */
    int current_selection;        /* +0x008 */
    int item_count;               /* +0x00C */
    int items[0x1B7];             /* +0x010 */
    int category;                 /* +0x6EC */
    int player_port;              /* +0x6F0 */
    void* profile;                /* +0x6F4 */
    ProfileCommon* profile_common; /* +0x6F8 */
    StringObj* bio_text;          /* +0x6FC */
    unsigned int bio_text_instance; /* +0x700 */
} KontentPdata; /* 0x704 */

extern int menu_player;
extern unsigned long mode_of_play;
extern char p1_profile[];
extern char p2_profile[];
extern ProfileCommon* p1_profile_common;
extern ProfileCommon* p2_profile_common;
extern void* p1_profile_konquest;
extern void* p2_profile_konquest;
extern char bgnd_animations[];
extern void* point_light_list;
extern char camera_point_light[];
extern float _mkproc_sleep_ticks;
extern CmdScript* active_cmdscript;
extern GlobalPlayerEntry global_player_data[];
extern MkFileInfo sec_krypt_screen_art;
extern MkFileInfo sec_krypt_award_art;
extern MkFileInfo sec_kryptdata;
extern MkFileInfo sec_krypt_thumbs;
extern MkFileInfo sec_ending_thumbs;
extern MkFileInfo sec_eu_biofont;
extern MkFileEntry krypt_art_file_table[];
extern MkFileEntry galleryart_file_table[];
extern MkFileEntry bio_text_file_table[];
extern MkFileEntry bios_file_table[];
extern unsigned int filter_masks[];
extern int winner_for_ending;

/* krypt.o .data */
extern int tombstone_pattern[4][4];

/* Opened-coffin pebble offset {0, 0, 1.75} - retail @791 */
static const float s_open_lid_offset[3] = {0.0f, 0.0f, 1.75f};
static const float s_cam_pos[3] = {-1.126f, 6.534f, 58.529f};
static const float s_cam_ang[3] = {0.215f, 0.38f, 0.0f};

/* Tombstone HUD layout constants (krypt.o .sdata2 @2548..@2742). */
static const float s_tomb_col_spacing = 3.0f;
static const float s_tomb_col_origin = -28.5f;
static const float s_tomb_row_spacing = 5.0f;
static const float s_tomb_row_origin = 50.0f;
static const float s_tomb_z_near = 0.14f;
static const float s_tomb_z_far = 1.9f;
static const float s_tomb_letter_x_half = 0.2f;
static const float s_tomb_letter_y = 0.055f;
static const float s_tomb_digit_y_open = -10.0f;
static const float s_tomb_digit_y_closed = 1.6f;
static const float s_tomb_digit_x_outer = 1.6f;
static const float s_tomb_digit_x_mid = 0.15f;
static const float s_tomb_digit_x_inner = 0.075f;
static const float s_tomb_koin_y = 2.5f;
static const float s_tomb_koin_z_near = 0.22f;
static const float s_tomb_koin_z_far = 0.085f;
static const float s_tomb_koin_open_uv_bias = 1.0f;
static const float s_tomb_hidden_pos = 0.0f;
static const float s_tomb_hidden_uv_y = -1.0f;

/* Sbss */
int krypt_data_loaded;
int gallery_data_loaded;
KontentPdata* kontent_pdata;
void* prize_description_block;
CoffinEntry* coffin_data;
KryptPdata* krypt_pdata;

/* Forward decls for statics referenced before definition (retail order). */
static void set_letter_positions_and_values(void* pfx);
static void set_number_positions_and_values(void* pfx);
static void set_koin_positions_and_colors(void* pfx);
static void position_fire_pots(void);
float p_fog_follow_camera(void);

static inline void mkproc_jump_sleep(MkProcEntryFn entry) {
    ((MkVtableMkprocLocal*)aproc->vtbl)->jump_sleep(entry);
}

static inline void mkproc_sleep(void) {
    ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
}

static inline void reset_coffin_pebble_counts(void) {
    if (krypt_pdata->coffin_pebble_type0 != 0) {
        krypt_pdata->coffin_pebble_type0->count = 0;
    }
    if (krypt_pdata->coffin_pebble_type1 != 0) {
        krypt_pdata->coffin_pebble_type1->count = 0;
    }
    if (krypt_pdata->coffin_pebble_type2 != 0) {
        krypt_pdata->coffin_pebble_type2->count = 0;
    }
    if (krypt_pdata->coffin_pebble_type3 != 0) {
        krypt_pdata->coffin_pebble_type3->count = 0;
    }
    if (krypt_pdata->lid_closed_pebbles != 0) {
        krypt_pdata->lid_closed_pebbles->count = 0;
    }
    if (krypt_pdata->lid_open_pebbles != 0) {
        krypt_pdata->lid_open_pebbles->count = 0;
    }
}

static inline void place_visible_coffin_rows(void) {
    float origin[3];
    int row;
    int col;
    int row_end;
    float x;

    row = krypt_pdata->current_row - 1;
    if (row > 0x10) {
        row = 0x10;
    }
    if (row < 0) {
        row = 0;
    }
    col = krypt_pdata->current_column - 4;
    if (col < 0) {
        col = 0;
    } else if (col > 0xB) {
        col = 0xB;
    }
    x = 3.0f * (float)col + -28.5f;
    row_end = row + 4;
    for (; row < row_end; row++) {
        origin[0] = x;
        origin[1] = 0.0f;
        origin[2] = -(5.0f * (float)row - 50.0f);
        set_pebble_positions_for_row(row, col, 9, origin);
    }
}

/* ========================================================================= */
/* Cluster A - Kontent gallery                                               */
/* ========================================================================= */

static inline StringObj* kontent_bio_text_live(void) {
    StringObj* raw;
    StringObj* live;

    raw = kontent_pdata->bio_text;
    if (raw != 0) {
        if (raw->instance == kontent_pdata->bio_text_instance) {
            live = raw;
        } else {
            live = 0;
        }
    } else {
        live = 0;
    }
    return live;
}

void hide_kontent_bio_text(void) {
    StringObj* text;

    text = kontent_bio_text_live();
    if (text != 0) {
        hide_string_obj(text);
    }
}

void unhide_kontent_bio_text(void) {
    StringObj* text;

    text = kontent_bio_text_live();
    if (text != 0) {
        unhide_string_obj(text);
    }
}

void kill_kontent_bio_text(void) {
    StringObj* text;

    /* Soft ceiling: kill_kontent_bio_text ~98.18% - pure GPR coloring. */
    text = kontent_bio_text_live();
    if (text != 0) {
        if (text->instance != 0) {
            text->vtbl->destroy();
        }
        kontent_pdata->bio_text = 0;
        kontent_pdata->bio_text_instance = 0;
    }
}

void get_gallery_page_number_string(char* out) {
    sprintf(out, "%d of %d", kontent_pdata->current_selection / 12 + 1,
            (kontent_pdata->item_count - 1) / 12 + 1);
}

char* get_long_coffin_description(void) {
    int coffin;

    coffin = kontent_pdata->items[kontent_pdata->current_selection];
    if (gallery_data_loaded == 0 || coffin < 0 || coffin > 0x1B7) {
        return 0;
    }
    return coffin_data[coffin].long_description;
}

char* get_coffin_blurb(void) {
    int selection;
    int coffin;

    selection = kontent_pdata->current_selection;
    coffin = kontent_pdata->items[selection];
    if (selection >= kontent_pdata->item_count) {
        return "";
    }
    if (kontent_pdata->category == 2) {
        return (char*)global_player_data[coffin].name;
    }
    if (gallery_data_loaded == 0 || coffin < 0 || coffin > 0x1B7) {
        return 0;
    }
    return coffin_data[coffin].blurb;
}
void create_fullscreen_gallery_image_list(int* out, int count) {
    int i;
    int coffin;
    unsigned int oid;
    MkFileInfo* section;
    StringObj* text;
    CoffinEntry* entry;

    coffin = kontent_pdata->items[kontent_pdata->current_selection];
    oid = 0;
    if (gallery_data_loaded != 0) {
        entry = &coffin_data[coffin];
        section = get_mk_file_info_from_current_ssf(entry->gallery_art);
        if (is_section_loading_or_loaded(0x150067, section) == 0) {
            unload_section_slot(0x150067);
        }
        load_art_section(0x150067, section);
        oid = (entry->gallery_art + 0x3EA) * 0x10000U | 2;
    }
    for (i = 0; i < count; i++) {
        ((RwTexture**)out[0])[i] = load_tga(0x150067, oid + i);
    }

    if (kontent_pdata->category == 0) {
        load_ssf(bio_text_file_table);
        load_string_bank(0x20000, "bio_strings_eng.mko");
        load_ssf(bios_file_table);
        load_art_section(0x11005C, &sec_eu_biofont);
        load_font_in_slot(0x10, (char*)0x11005C, 0x1490001, 0x1490000);
        entry = &coffin_data[coffin];
        text = put_bio_text(entry->unlock_or_movie, entry->kontent_type != 0xD);
        if (text != 0) {
            kontent_pdata->bio_text = text;
            kontent_pdata->bio_text_instance = text->instance;
        }
    }
}

static int kontent_gallery_movie_tapout(void) {
    scan_switches();
    return check_switch_edge(kontent_pdata->player_port, 0xB) != 0 ||
           check_switch_edge(kontent_pdata->player_port, 6) != 0;
}

void start_loading_kontent_image(void) {
    int coffin;
    int type;
    int movie;
    MkFileInfo* section;

    coffin = kontent_pdata->items[kontent_pdata->current_selection];
    if (kontent_pdata->item_count == 0) {
        return;
    }
    eat_switch_edge(kontent_pdata->player_port, 6);
    if (kontent_pdata->category == 2) {
        if (kontent_pdata->current_selection < kontent_pdata->item_count) {
            pause_screen_engine(1);
            run_ending(coffin);
            eat_switch_edge(kontent_pdata->player_port, 6);
            pause_screen_engine(0);
            fade_from_black(8, 1);
        }
        return;
    }

    type = -1;
    if (gallery_data_loaded != 0 && coffin >= 0 && coffin <= 0x1B7) {
        type = coffin_data[coffin].kontent_type;
    }
    if (type == 5 || type == 9) {
        movie = -1;
        if (gallery_data_loaded != 0 && coffin >= 0 && coffin <= 0x1B7 &&
            coffin_data[coffin].movie_kind == 0x14) {
            movie = coffin_data[coffin].unlock_or_movie;
        }
        if (movie != -1) {
            play_movie(movie, kontent_gallery_movie_tapout);
        }
    } else if (kontent_pdata->current_selection < kontent_pdata->item_count) {
        load_ssf(krypt_art_file_table);
        if (gallery_data_loaded != 0) {
            section = get_mk_file_info_from_current_ssf(coffin_data[coffin].gallery_art);
            unload_section_slot(0x150067);
            load_art_section_async(0x150067, section);
        }
        fire_screen_studio_event(0x1FEA, 1);
    }
}
void kontent_set_current_selection(int selection) {
    kontent_pdata->current_selection = selection;
}
void create_gallery_image_list(int* out, int count) {
    int i;
    int coffin;
    unsigned int oid;
    char* name;

    for (i = 0; i < count; i++) {
        coffin = kontent_pdata->items[i];
        if (kontent_pdata->category == 2) {
            name = get_ending_thumbnail_name(coffin);
            ((RwTexture**)out[0])[i] = load_named_tga_from_slot(0x150068, name);
        } else {
            oid = 0;
            if (gallery_data_loaded != 0 && coffin >= 0 && coffin <= 0x1B7) {
                oid = (unsigned int)coffin_data[coffin].gallery_art | 0x03E90000;
            }
            ((RwTexture**)out[0])[i] = load_tga(0x150068, oid);
        }
    }
}

int get_number_kontent_items(void) {
    int i;
    int category;
    CoffinEntry* entry;

    kontent_pdata->item_count = 0;
    category = kontent_pdata->category;
    if (category == 2) {
        for (i = 0; i < 0x2C; i++) {
            if (is_mark_as_unlocked(kontent_pdata->profile, 6, i) != 0 &&
                get_ending_thumbnail_name(i) != 0) {
                kontent_pdata->items[kontent_pdata->item_count++] = i;
            }
        }
        return kontent_pdata->item_count;
    }
    if (category != 3 && category >= 0 && category < 5) {
        for (i = 400; i < 439; i++) {
            entry = &coffin_data[i];
            if ((filter_masks[category] & (1U << (entry->kontent_type & 0xFF))) != 0) {
                kontent_pdata->items[kontent_pdata->item_count++] = i;
            }
        }
    }
    for (i = 0; i < 400; i++) {
        if (get_coffin_bit(kontent_pdata->profile_common->coffin_bits, i) != 0) {
            entry = &coffin_data[i];
            if ((filter_masks[category] & (1U << (entry->kontent_type & 0xFF))) != 0) {
                kontent_pdata->items[kontent_pdata->item_count++] = i;
            }
        }
    }
    return kontent_pdata->item_count;
}

float p_kontent_setup(void) {
    MkFileEntry* file;
    char* strings;
    int length;
    int stringLength;
    int i;

    zero_pdata_payload(0x704, (MkHdr*)kontent_pdata);
    mode_of_play = 5;
    if (menu_player == 0) {
        kontent_pdata->player_port = g_game_info.plyr0.pad_index;
        set_player_state(&g_game_info.plyr0, 2);
        kontent_pdata->profile = p1_profile;
        kontent_pdata->profile_common = p1_profile_common;
        winner_for_ending = 0;
    } else {
        kontent_pdata->player_port = g_game_info.plyr1.pad_index;
        set_player_state(&g_game_info.plyr1, 2);
        move_profile_p1_to_p2();
        kontent_pdata->profile = p2_profile;
        kontent_pdata->profile_common = p2_profile_common;
        winner_for_ending = 1;
    }
    gallery_data_loaded = 0;
    kontent_pdata->category = get_menu_mode_sub_var() - 0x2BE;
    load_ssf(galleryart_file_table);
    if (gallery_data_loaded == 0) {
        file = mk_file_open_language(&sec_kryptdata, "rb", (void*)1);
        if (file != 0) {
            length = mk_file_length(file);
            stringLength = length - 0x4498;
            strings = (char*)get_mem(stringLength);
            if (strings != 0) {
                coffin_data = (CoffinEntry*)get_mem(0x4498);
                if (coffin_data != 0) {
                    mk_file_read(coffin_data, 0x28, 0x1B7, file);
                    mk_file_read(strings, 1, stringLength, file);
                    mk_file_close(file);
                    for (i = 0; i < 0x1B7; i++) {
                        coffin_data[i].blurb += (int)strings;
                        coffin_data[i].long_description += (int)strings;
                    }
                    gallery_data_loaded = 1;
                }
            }
        }
    }
    if (kontent_pdata->category == 2) {
        load_art_section(0x150068, &sec_ending_thumbs);
    } else {
        load_art_section(0x150068, &sec_krypt_thumbs);
    }
    load_ssf(krypt_art_file_table);
    load_screen("common/kontent/kontent_main", 0x140064, 0, 1);
    turn_camera_on();
    turn_controllers_on();
    disable_all_ports_but_me(kontent_pdata->player_port);
    wait_for_screen_close();
    if (prize_description_block != 0) {
        free_mem(prize_description_block);
    }
    if (coffin_data != 0) {
        free_mem(coffin_data);
        coffin_data = 0;
    }
    gamelogic_jump(6, p_main_menu);
    return 1.0f;
}
float p_kontent(void) {
    MkProc* proc;

    set_section_memory_scheme(7);
    proc = _create_mkproc_generic_bigstack(0x2001, 0x23, p_kontent_setup, 0x704,
                                           (MkHdr**)&kontent_pdata);
    if (proc == 0) {
        gamelogic_jump(0, p_atm_loop);
    }
    return 1.0f;
}

/* ========================================================================= */
/* Cluster B - showcase getters (real) / setters (deferred stubs)            */
/* ========================================================================= */

void* get_krypt_anim_pdata(void) {
    return krypt_pdata->anim_pdata;
}

void* get_krypt_character_obj(void) {
    MkObjLatch* pdata;
    MkHdr* obj;

    pdata = (MkObjLatch*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return 0;
    }

    obj = pdata->obj;
    if (obj == 0) {
        return 0;
    }
    if (obj->instance != pdata->obj_instance) {
        return 0;
    }
    return obj;
}

int get_krypt_current_column(void) {
    return krypt_pdata->current_column;
}

int get_krypt_current_row(void) {
    return krypt_pdata->current_row;
}

void transition_to_krypt_character_anim_script(void) {}
void set_krypt_character_anim_script(void) {}
void set_krypt_character_previous_root_angle(void) {}
void set_krypt_character_angle(void) {}
void set_krypt_character_pos(void) {}
static float p_run_character_animation(void) { return 0.0f; }
void load_krypt_character(void) {}
static float p_krypt_animate(void) { return 0.0f; }
static float p_monitor_krypt_characters(void) { return 0.0f; }
static float p_play_random_noise(void) { return 0.0f; }

/* ========================================================================= */
/* Cluster C - tombstone HUD pfx + coffin-grid pebbles (MVP)                 */
/* ========================================================================= */

/*
 * Shared update path for letter/number/koin tombstone particle systems.
 * While krypt_pdata->tombstone_hud_ticks > 0, rebuild layouts via set_*;
 * otherwise copy live UV/color fields from the -1 slot into the -2 slot.
 */
static inline float update_tombstone_common(void (*rebuild)(void* pfx)) {
    TombstonePfx* pfx;
    float* dst_f;
    float* src_f;
    int stride_f;
    int* src_i;
    int* dst_i;
    int stride_i;
    int i;
    int count;

    pfx = (TombstonePfx*)apfx->matrix;
    if (krypt_pdata->tombstone_hud_ticks != 0) {
        rebuild(pfx);
        krypt_pdata->tombstone_hud_ticks -= 1;
        return 1.0f;
    }

    dst_f = (float*)pfx_get_field(pfx, -2, 0x301);
    src_f = (float*)pfx_get_field(pfx, -1, 0x301);
    stride_f = pfx_get_struct_size(pfx, 0x301);
    src_i = (int*)pfx_get_field(pfx, -1, 0x100);
    dst_i = (int*)pfx_get_field(pfx, -2, 0x100);
    stride_i = pfx_get_struct_size(pfx, 0x100);

    count = pfx->count;
    for (i = 0; i < count; i++) {
        dst_i[0] = src_i[0];
        dst_i[1] = src_i[1];
        dst_i[2] = src_i[2];
        src_i = (int*)((char*)src_i + stride_i);
        dst_i = (int*)((char*)dst_i + stride_i);
        *dst_f = *src_f;
        src_f = (float*)((char*)src_f + stride_f);
        dst_f = (float*)((char*)dst_f + stride_f);
    }
    return 1.0f;
}

float update_tombstone_letters(void) {
    return update_tombstone_common(set_letter_positions_and_values);
}

float update_tombstone_numbers(void) {
    return update_tombstone_common(set_number_positions_and_values);
}

float update_tombstone_koins(void) {
    return update_tombstone_common(set_koin_positions_and_colors);
}

static inline void init_tombstone_common(void* pfx_arg, int particle_count, float scale,
                                  void* tex_name, int anim_a, int anim_b, int anim_c,
                                  int anim_d) {
    TombstonePfx* pfx;
    unsigned char flags;

    pfx = (TombstonePfx*)pfx_arg;
    pfx->capacity = particle_count;
    pfx->count = particle_count;

    flags = pfx->flags_150;
    flags = (unsigned char)((flags & ~0x40) | 0x40);
    pfx->flags_150 = flags;
    pfx->scale = scale;

    flags = pfx->flags_150;
    flags = (unsigned char)((flags & ~0x80) | 0x80);
    pfx->flags_150 = flags;

    flags = pfx->flags_150;
    flags = (unsigned char)((flags & ~0x10) | 0x10);
    pfx->flags_150 = flags;

    pfx->mat_a = 1.0f;
    pfx->mat_b = 0.0f;
    pfx->mat_c = 0.0f;
    pfx->mat_d = 0.0f;
    pfx->mat_e = 1.0f;
    pfx->mat_f = 0.0f;
    pfx_native_set_rgba(&pfx->rgba_1B4, 0.0f, 0.0f, 0.0f, 255.0f);

    flags = pfx->flags_150;
    flags = (unsigned char)((flags & ~0x08) | 0x08);
    pfx->flags_150 = flags;

    pfx->uv_scale = 1.0f;
    pfx->uv_0 = 0.0f;
    pfx->uv_rate = 0.0125f;
    pfx_native_set_rgba(&pfx->rgba_160, 255.0f, 255.0f, 255.0f, 0.0f);

    /* Texture path/name are retail immediates (section-relative ids). */
    set_pfx_texture((PfxVm*)pfx, (void*)0x00140064, tex_name);
    pfx_texture_animate(1.0f, pfx, anim_a, anim_b, anim_c, anim_d);
    pfx->anim_frame = 0;
}

void init_tombstone_letters(void* pfx) {
    init_tombstone_common(pfx, 0x48, 0.3f, (void*)0x012a0005, 0x100, 0x20, 0x20, 0x40);
}

void init_tombstone_numbers(void* pfx) {
    init_tombstone_common(pfx, 0x90, 0.18f, (void*)0x012a0006, 0x80, 0x20, 0x2a, 0xa);
}

void init_tombstone_koins(void* pfx) {
    init_tombstone_common(pfx, 0x24, 0.4f, (void*)0x012a0004, 0x80, 0x2a, 0x2a, 0x8);
}

static inline void tombstone_viewport_origin(int* out_row, int* out_col) {
    int row;
    int col;

    row = krypt_pdata->current_row - 1;
    if (row > 0x10) {
        row = 0x10;
    }
    if (row < 0) {
        row = 0;
    }
    col = krypt_pdata->current_column - 4;
    if (col < 0) {
        col = 0;
    } else if (col > 0xB) {
        col = 0xB;
    }
    *out_row = row;
    *out_col = col;
}

static inline float tombstone_row_z(int row) {
    return -(s_tomb_row_spacing * (float)row - s_tomb_row_origin);
}

static inline float tombstone_col_x(int col) {
    return s_tomb_col_spacing * (float)col + s_tomb_col_origin;
}

static inline float tombstone_coffin_z(int coffin_type, float z_near, float z_far) {
    if (coffin_type == 4) {
        return z_near;
    }
    return z_far;
}

static inline float tombstone_digit_y(int prize_kind) {
    if (prize_kind == 6) {
        return s_tomb_digit_y_open;
    }
    return s_tomb_digit_y_closed;
}

static inline void tombstone_write_particle(float** pos, float** uv, int pos_stride, int uv_stride,
                                     float x, float y, float z, float value) {
    (*pos)[0] = x;
    (*pos)[1] = y;
    (*pos)[2] = z;
    *pos = (float*)((char*)*pos + pos_stride);
    **uv = value;
    *uv = (float*)((char*)*uv + uv_stride);
}

static void set_letter_positions_and_values(void* pfx) {
    float* uv;
    float* pos;
    int uv_stride;
    int pos_stride;
    int view_row;
    int view_col;
    int row_off;
    int col_off;
    float base_x;
    float base_z;
    float z_near;
    float z_far;
    int row;
    int col;
    int coffin_idx;
    int opened;
    int letter_bias;
    CoffinEntry* entry;

    uv = (float*)pfx_get_field(pfx, -2, 0x301);
    uv_stride = pfx_get_struct_size(pfx, 0x301);
    pos = (float*)pfx_get_field(pfx, -2, 0x100);
    pos_stride = pfx_get_struct_size(pfx, 0x100);
    tombstone_viewport_origin(&view_row, &view_col);
    base_x = tombstone_col_x(view_col);
    base_z = tombstone_row_z(view_row);

    for (row_off = 0; row_off < 4; row_off++) {
        row = view_row + row_off;
        z_near = s_tomb_z_near + base_z;
        z_far = s_tomb_z_far + base_z;
        for (col_off = 0; col_off < 9; col_off++) {
            col = view_col + col_off;
            coffin_idx = col + row * 0x14;
            opened = get_coffin_bit(krypt_pdata->profile_common->coffin_bits, coffin_idx);
            letter_bias = opened ? 0x20 : 0;
            entry = &coffin_data[coffin_idx];

            tombstone_write_particle(&pos, &uv, pos_stride, uv_stride,
                                     base_x - s_tomb_letter_x_half, s_tomb_letter_y,
                                     tombstone_coffin_z(entry->coffin_type, z_near, z_far),
                                     (float)(letter_bias + row));
            tombstone_write_particle(&pos, &uv, pos_stride, uv_stride,
                                     base_x + s_tomb_letter_x_half, s_tomb_letter_y,
                                     tombstone_coffin_z(entry->coffin_type, z_near, z_far),
                                     (float)(letter_bias + col));
            base_x += s_tomb_col_spacing;
        }
        base_z -= s_tomb_row_spacing;
        base_x = tombstone_col_x(view_col);
    }
}

static void set_number_positions_and_values(void* pfx) {
    float* uv;
    float* pos;
    int uv_stride;
    int pos_stride;
    int view_row;
    int view_col;
    int row_off;
    int col_off;
    int particle;
    int particle_limit;
    float base_x;
    float base_z;
    float z_near;
    float z_far;
    int row;
    int col;
    int coffin_idx;
    CoffinEntry* entry;
    int cost;
    int thousands;
    int hundreds;
    int tens;
    int ones;
    int slot;
    float x;

    uv = (float*)pfx_get_field(pfx, -2, 0x301);
    uv_stride = pfx_get_struct_size(pfx, 0x301);
    pos = (float*)pfx_get_field(pfx, -2, 0x100);
    pos_stride = pfx_get_struct_size(pfx, 0x100);
    particle = 0;
    tombstone_viewport_origin(&view_row, &view_col);
    base_x = tombstone_col_x(view_col);
    base_z = tombstone_row_z(view_row);

    for (row_off = 0; row_off < 4; row_off++) {
        row = view_row + row_off;
        z_near = s_tomb_z_near + base_z;
        z_far = s_tomb_z_far + base_z;
        for (col_off = 0; col_off < 9; col_off++) {
            col = view_col + col_off;
            coffin_idx = col + row * 0x14;
            entry = &coffin_data[coffin_idx];
            cost = entry->cost;
            thousands = cost / 1000;
            hundreds = (cost / 100) - thousands * 10;
            tens = (cost / 10) - thousands * 100 - hundreds * 10;
            ones = cost - thousands * 1000 - hundreds * 100 - tens * 10;
            slot = 0;

            if (thousands != 0) {
                tombstone_write_particle(&pos, &uv, pos_stride, uv_stride,
                                         base_x - s_tomb_digit_x_outer,
                                         tombstone_digit_y(entry->prize_kind),
                                         tombstone_coffin_z(entry->coffin_type, z_near, z_far),
                                         (float)thousands);
                particle += 1;
                slot = 4;
            }

            if (slot == 4) {
                x = base_x - s_tomb_digit_x_inner;
            } else if (hundreds != 0) {
                slot = 3;
                x = base_x - s_tomb_digit_x_mid;
            } else {
                x = base_x;
            }

            if (slot >= 3) {
                tombstone_write_particle(&pos, &uv, pos_stride, uv_stride, x,
                                         tombstone_digit_y(entry->prize_kind),
                                         tombstone_coffin_z(entry->coffin_type, z_near, z_far),
                                         (float)hundreds);
                particle += 1;
            }

            if (slot == 4) {
                x = base_x + s_tomb_digit_x_inner;
            } else if (slot == 3) {
                x = base_x;
            } else if (tens != 0) {
                slot = 2;
                x = base_x - s_tomb_digit_x_inner;
            } else {
                x = base_x;
            }

            if (slot >= 2) {
                tombstone_write_particle(&pos, &uv, pos_stride, uv_stride, x,
                                         tombstone_digit_y(entry->prize_kind),
                                         tombstone_coffin_z(entry->coffin_type, z_near, z_far),
                                         (float)tens);
                particle += 1;
            }

            if (slot == 4) {
                x = base_x + s_tomb_digit_x_outer;
            } else if (slot == 3) {
                x = base_x + s_tomb_digit_x_mid;
            } else if (slot == 2) {
                x = base_x + s_tomb_digit_x_inner;
            } else {
                x = base_x;
                slot = 1;
            }

            if (slot >= 1) {
                tombstone_write_particle(&pos, &uv, pos_stride, uv_stride, x,
                                         tombstone_digit_y(entry->prize_kind),
                                         tombstone_coffin_z(entry->coffin_type, z_near, z_far),
                                         (float)ones);
                particle += 1;
            }

            base_x += s_tomb_col_spacing;
        }
        base_z -= s_tomb_row_spacing;
        base_x = tombstone_col_x(view_col);
    }

    particle_limit = ((TombstonePfx*)pfx)->count;
    while (particle < particle_limit) {
        tombstone_write_particle(&pos, &uv, pos_stride, uv_stride, s_tomb_hidden_pos,
                                 s_tomb_hidden_uv_y, s_tomb_hidden_pos, s_tomb_hidden_pos);
        particle += 1;
    }
}

static void set_koin_positions_and_colors(void* pfx) {
    float* uv;
    float* pos;
    int uv_stride;
    int pos_stride;
    int view_row;
    int view_col;
    int row_off;
    int col_off;
    float base_x;
    float base_z;
    float z_near;
    float z_far;
    int row;
    int col;
    int coffin_idx;
    CoffinEntry* entry;
    int koin_kind;
    float koin_uv;

    uv = (float*)pfx_get_field(pfx, -2, 0x301);
    uv_stride = pfx_get_struct_size(pfx, 0x301);
    pos = (float*)pfx_get_field(pfx, -2, 0x100);
    pos_stride = pfx_get_struct_size(pfx, 0x100);
    tombstone_viewport_origin(&view_row, &view_col);
    base_x = tombstone_col_x(view_col);
    base_z = tombstone_row_z(view_row);

    for (row_off = 0; row_off < 4; row_off++) {
        row = view_row + row_off;
        z_near = s_tomb_koin_z_near + base_z;
        z_far = s_tomb_koin_z_far + base_z;
        for (col_off = 0; col_off < 9; col_off++) {
            col = view_col + col_off;
            coffin_idx = col + row * 0x14;
            entry = &coffin_data[coffin_idx];
            koin_kind = entry->prize_kind;
            koin_uv = (float)koin_kind;
            if (koin_kind == 6
                && get_coffin_bit(krypt_pdata->profile_common->coffin_bits, coffin_idx) != 0) {
                koin_uv += s_tomb_koin_open_uv_bias;
            }
            tombstone_write_particle(&pos, &uv, pos_stride, uv_stride, base_x, s_tomb_koin_y,
                                     tombstone_coffin_z(entry->coffin_type, z_near, z_far), koin_uv);
            base_x += s_tomb_col_spacing;
        }
        base_z -= s_tomb_row_spacing;
        base_x = tombstone_col_x(view_col);
    }
}

void force_wallet_to_open_position(void) {}
void heads_up_display_visible(void) {}
void init_heads_up_display(void) {}

/* ========================================================================= */
/* Cluster D - coffin open / FX (deferred)                                   */
/* ========================================================================= */

static float p_move_camera_and_open_coffin(void) { return 0.0f; }
static float p_fade_fog(void) { return 0.0f; }
void remove_prize_description(void) {}
void display_prize_description(void) {}
void move_picture_to_camera(void) {}
void start_opening_coffin_effects(void) {}

/* ========================================================================= */
/* Cluster E - input / dialog / economy (deferred)                           */
/* ========================================================================= */

void handle_controller_input(void) {}
void do_dialog(void) {}
static float p_running_footstep_proc(void) { return 0.0f; }
static float p_single_move_footstep_proc(void) { return 0.0f; }
void deduct_koins(void) {}
static float p_counting_sound(void) { return 0.0f; }

/* ========================================================================= */
/* Coffin-grid pebble layout (MVP)                                           */
/* ========================================================================= */

void set_pebble_positions_for_row(int row, int start_col, int count, float* origin) {
    int col;
    int i;
    int pattern;
    PebbleData* pebble;
    RwMatrix* mat;
    PebbleData* lid;
    RwMatrix* lid_mat;
    int lid_count;

    col = start_col;
    for (i = 0; i < count; i++, col++) {
        pattern = tombstone_pattern[row & 3][col & 3];
        pebble = 0;
        if (pattern == 0) {
            pebble = krypt_pdata->coffin_pebble_type0;
        } else if (pattern == 1) {
            pebble = krypt_pdata->coffin_pebble_type1;
        } else if (pattern == 2) {
            pebble = krypt_pdata->coffin_pebble_type2;
        } else if (pattern == 3) {
            pebble = krypt_pdata->coffin_pebble_type3;
        }

        mat = 0;
        if (pebble != 0) {
            mat = &pebble->pebbles[pebble->count].matrix;
            if (mat == 0) {
                return;
            }
            mat->pos.x = 3.0f * (float)i + origin[0];
            mat->pos.y = 0.0f;
            mat->pos.z = origin[2];
            pebble->count = pebble->count + 1;
        }

        if (get_coffin_bit(krypt_pdata->profile_common->coffin_bits, col + row * 0x14) != 0) {
            lid = krypt_pdata->lid_open_pebbles;
        } else {
            lid = krypt_pdata->lid_closed_pebbles;
        }

        if (lid != 0 && mat != 0) {
            lid_count = lid->count;
            lid_mat = &lid->pebbles[lid_count].matrix;
            lid_mat->pos.x = mat->pos.x + s_open_lid_offset[0];
            lid_mat->pos.y = mat->pos.y + s_open_lid_offset[1];
            lid_mat->pos.z = mat->pos.z + s_open_lid_offset[2];
            /* Retail then forces Y to 0. */
            lid_mat->pos.y = 0.0f;
            lid->count = lid_count + 1;
        }
    }
}

static inline void* krypt_bgnd_obj(void) {
    return g_game_info.bgnd_obj;
}

static inline void sobj_enable_pebble_bit(void* sobj) {
    MkSobj* s;

    s = (MkSobj*)sobj;
    s->flags09 = (unsigned char)((s->flags09 & ~0x10) | 0x10);
}

static inline void fill_wall_pebble_z(PebbleData* pebble, float spacing, float base) {
    int i;
    RwMatrix* mat;

    for (i = 0; i < 5; i++) {
        mat = &pebble->pebbles[i].matrix;
        mat->pos.z = -(spacing * (float)i - base);
    }
}

void setup_tombstones(void) {
    void* bgnd;
    void* sobj;
    void* sobj_b;
    PebbleData* pebble;
    RwMatrix* mat_a;
    RwMatrix* mat_b;
    int i;
    int col;
    int row;

    bgnd = krypt_bgnd_obj();

    sobj = obj_find_sobj_by_id(bgnd, 0x15);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata((MkSobj*)sobj, 5, 0);
        krypt_pdata->pebble_wall_a = pebble;
        fill_wall_pebble_z(pebble, 5.0f * 4.0f, 50.0f);
    }

    sobj = obj_find_sobj_by_id(bgnd, 0x16);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata((MkSobj*)sobj, 5, 0);
        krypt_pdata->pebble_wall_b = pebble;
        fill_wall_pebble_z(pebble, 5.0f * 4.0f, 45.0f);
    }

    sobj = obj_find_sobj_by_id(bgnd, 0x17);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata((MkSobj*)sobj, 5, 0);
        krypt_pdata->pebble_wall_c = pebble;
        fill_wall_pebble_z(pebble, 5.0f * 4.0f, 40.0f);
    }

    sobj = obj_find_sobj_by_id(bgnd, 0x18);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata((MkSobj*)sobj, 5, 0);
        krypt_pdata->pebble_wall_d = pebble;
        fill_wall_pebble_z(pebble, 5.0f * 4.0f, 35.0f);
    }

    sobj = obj_find_sobj_by_id(bgnd, 9);
    sobj_b = obj_find_sobj_by_id(bgnd, 10);
    if (sobj != 0 && sobj_b != 0) {
        sobj_enable_pebble_bit(sobj_b);
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->pebble_grid_a = create_pebble_userdata((MkSobj*)sobj, 0xc, 0);
        krypt_pdata->pebble_grid_b = create_pebble_userdata((MkSobj*)sobj_b, 0xc, 0);
        for (i = 0; i < 0xc; i++) {
            col = i % 3;
            row = i / 3;
            mat_a = &krypt_pdata->pebble_grid_a->pebbles[i].matrix;
            mat_b = &krypt_pdata->pebble_grid_b->pebbles[i].matrix;
            mat_a->pos.x = 3.0f * (float)((col + 1) * 5 - 1) + (-28.5f);
            mat_a->pos.y = 0.0f;
            mat_a->pos.z = -(5.0f * (float)((row + 1) * 4 - 1) - 50.0f);
            mat_b->pos.x = mat_a->pos.x;
            mat_b->pos.y = mat_a->pos.y;
            mat_b->pos.z = mat_a->pos.z;
        }
    }

    sobj = obj_find_sobj_by_id(bgnd, 1);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type0 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }
    sobj = obj_find_sobj_by_id(bgnd, 3);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type1 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }
    sobj = obj_find_sobj_by_id(bgnd, 5);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type2 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }
    sobj = obj_find_sobj_by_id(bgnd, 7);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type3 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }

    sobj = obj_find_sobj_by_id(bgnd, 0x3c);
    if (sobj != 0) {
        ((MkObj*)sobj)->flags_08 =
            (unsigned char)((((MkObj*)sobj)->flags_08 & ~0x40) | 0x40);
        ((MkObj*)sobj)->flags_08 =
            (unsigned char)((((MkObj*)sobj)->flags_08 & ~0x20) | 0x20);
    }
    sobj = obj_find_sobj_by_id(bgnd, 0x3d);
    if (sobj != 0) {
        ((MkObj*)sobj)->flags_08 =
            (unsigned char)((((MkObj*)sobj)->flags_08 & ~0x04) | 0x04);
    }

    sobj = obj_find_sobj_by_id(bgnd, 0x5a);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->lid_closed_pebbles = create_pebble_userdata((MkSobj*)sobj, 0x24, 0);
    }
    sobj = obj_find_sobj_by_id(bgnd, 0x5b);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->lid_open_pebbles = create_pebble_userdata((MkSobj*)sobj, 0x24, 0);
    }

    sobj = obj_find_sobj_by_id(bgnd, 0x32);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->fire_pot_pebbles = create_pebble_userdata((MkSobj*)sobj, 6, 0);
        position_fire_pots();
    }

    sobj = obj_find_sobj_by_id(bgnd, 0x46);
    if (sobj != 0) {
        MkObjLatch* fog_pdata;

        sobj_set_priority(sobj, 0x14);
        ((MkObj*)sobj)->flags_08 =
            (unsigned char)((((MkObj*)sobj)->flags_08 & ~0x40) | 0x40);
        fog_pdata = 0;
        _create_mkproc_generic_tinystack(0x823a, 0x1f, (MkProcEntryFn)p_fog_follow_camera, 0x10,
                                         (MkHdr**)&fog_pdata);
        if (fog_pdata != 0) {
            fog_pdata->obj = (MkHdr*)sobj;
            fog_pdata->obj_instance = ((MkHdr*)sobj)->instance;
        }
    }

    for (i = 0; i < 5; i++) {
        sobj = obj_find_sobj_by_id(bgnd, i + 0x1e);
        if (sobj != 0) {
            ((MkObj*)sobj)->flags_08 =
                (unsigned char)((((MkObj*)sobj)->flags_08 & ~0x40) | 0x40);
            ((MkObj*)sobj)->flags_08 =
                (unsigned char)((((MkObj*)sobj)->flags_08 & ~0x08) | 0x08);
            hide_sobj(sobj);
        }
    }
}

void position_fire_pots(void) {
    PebbleData* pebble;
    int i;
    TrackedSound* sound;
    RwMatrix* mat;
    /* Retail pot (x,0,z) at +0x30 + i*0x40 - ASM @4301..@4311. */
    static const float pot_xz[6][2] = {
        {-9.069f, -51.581f}, {9.3f, -51.581f},   {-32.944f, 7.598f},
        {-33.134f, 21.839f}, {32.963f, 7.355f}, {33.2f, 22.056f},
    };

    pebble = krypt_pdata->fire_pot_pebbles;

    for (i = 0; i < 6; i++) {
        mat = &pebble->pebbles[i].matrix;
        mat->pos.x = pot_xz[i][0];
        mat->pos.y = 0.0f;
        mat->pos.z = pot_xz[i][1];
    }

    /* Tracked sounds - call sites kept; audio itself is MVP-deferred. */
    for (i = 0; i < 6; i++) {
        sound = get_sound_tracker_data();
        krypt_pdata->fire_pot_sounds[i] = sound;
        mat = &pebble->pebbles[i].matrix;
        sound->pos_x = mat->pos.x;
        sound->pos_y = mat->pos.y;
        sound->pos_z = mat->pos.z;
        sound->pos_y = 1.0f;
        sound->sound_id = 0x3c3;
        sound->min_dist = 5.0f;
        sound->max_dist = 20.0f;
        make_new_tracked_sound(&krypt_pdata->tracked_sound_list, sound);
    }
}

/* ========================================================================= */
/* Cluster F - mode shell (Agent A)                                          */
/* ========================================================================= */

float p_fog_follow_camera(void) { return 0.0f; }
static float p_follow_camera(void) { return 0.0f; }
void load_pix_section(void) {}
void load_binary_data(void) {}
void init_konquest_keys(void) {}
void update_use_key_string(void) {}

float p_krypt_loop(void) {
    /* MVP: no-op instead of handle_controller_input */
    if (krypt_pdata->layout_dirty != 0) {
        krypt_pdata->tombstone_hud_ticks = 3;
        reset_coffin_pebble_counts();
        place_visible_coffin_rows();
        krypt_pdata->layout_dirty = 0;
    }
    return 1.0f;
}

float p_setup_krypt(void) {
    MkFileEntry* file;
    void* string_pool;
    int file_len;
    int string_len;
    int i;
    void* light;
    unsigned char* flags;
    CamVec3 cam_pos;
    CamVec3 cam_ang;

    cam_pos.x = s_cam_pos[0];
    cam_pos.y = s_cam_pos[1];
    cam_pos.z = s_cam_pos[2];
    cam_ang.x = s_cam_ang[0];
    cam_ang.y = s_cam_ang[1];
    cam_ang.z = s_cam_ang[2];

    /* MVP trim: skip start_sound_tracking_process */
    load_background(0x18);
    load_art_section_language(0x140066, &sec_krypt_screen_art);
    add_art_section(0x140066, &sec_krypt_award_art);
    load_font(9);
    load_font(0);
    load_font(7);

    if (krypt_data_loaded == 0) {
        file = mk_file_open_language(&sec_kryptdata, "rb", (void*)1);
        if (file != 0) {
            file_len = mk_file_length(file);
            string_len = file_len - 0x4498;
            string_pool = get_mem(string_len);
            if (string_pool != 0) {
                coffin_data = (CoffinEntry*)get_mem(0x4498);
                if (coffin_data != 0) {
                    mk_file_read(coffin_data, 0x28, 0x1B7, file);
                    mk_file_read(string_pool, 1, (unsigned int)string_len, file);
                    mk_file_close(file);
                    for (i = 0; i < 0x1B7; i++) {
                        coffin_data[i].blurb =
                            (char*)((int)string_pool + (int)coffin_data[i].blurb);
                        coffin_data[i].long_description =
                            (char*)((int)string_pool + (int)coffin_data[i].long_description);
                    }
                    krypt_data_loaded = 1;
                }
            }
        }
    }

    /* MVP trim: skip sound banks / tunes / p_play_random_noise */

    load_string_bank(0x20000, "krypt_strings_eng.mko");
    setup_tombstones();
    if (active_cmdscript != 0) {
        active_cmdscript->mko = g_game_info.cmdscript;
    }
    load_ssf(krypt_art_file_table);
    xfer_camera(p_krypt_camera_proc, 1);
    _mkproc_sleep_ticks = 2.0f;
    mkproc_sleep();
    set_camera_position(&cam_pos);
    set_camera_angle(&cam_ang);

    reset_coffin_pebble_counts();
    place_visible_coffin_rows();

    pfx_create_raw_userdata(0, 0, 0x24, 0x202, 2, init_tombstone_koins, 0x823B,
                            update_tombstone_koins, &krypt_pdata->pfx_koins);
    if (krypt_pdata->pfx_koins != 0) {
        flags = (unsigned char*)krypt_pdata->pfx_koins + 8;
        *flags = (unsigned char)((*flags & 0xEF) | 0x10);
    }
    pfx_create_raw_userdata(0, 0, 0x48, 0x202, 2, init_tombstone_letters, 0x823C,
                            update_tombstone_letters, &krypt_pdata->pfx_letters);
    if (krypt_pdata->pfx_letters != 0) {
        flags = (unsigned char*)krypt_pdata->pfx_letters + 8;
        *flags = (unsigned char)((*flags & 0xEF) | 0x10);
    }
    pfx_create_raw_userdata(0, 0, 0x90, 0x202, 2, init_tombstone_numbers, 0x823D,
                            update_tombstone_numbers, &krypt_pdata->pfx_numbers);
    if (krypt_pdata->pfx_numbers != 0) {
        flags = (unsigned char*)krypt_pdata->pfx_numbers + 8;
        *flags = (unsigned char)((*flags & 0xEF) | 0x10);
    }

    krypt_pdata->anim_flag = 0;
    light = load_light((LightDef*)camera_point_light, (MkPtr**)&point_light_list, 0);
    if (light != 0) {
        flags = (unsigned char*)light + 8;
        *flags = (unsigned char)((*flags & 0xBF) | 0x40);
    }

    bgnd_anim_camera_setup();
    cam_set_intro_cam_pause_ticks(60.0f);
    camera_init_animation(*(void**)(bgnd_animations + 0x78), 0);
    camera_run_animation(0);
    turn_camera_on();
    fade_from_black(0x14, 0);
    camera_wait_for_animation_completion();
    bgnd_anim_camera_ended();

    /* MVP trim: skip controllers / konquest keys / HUD / character monitor */

    mkproc_jump_sleep(p_krypt_loop);
    return 0.0f;
}

float p_init_krypt_mode(void) {
    RwResourcesSetArenaSize(0x100000);
    zero_pdata_payload(0x150, (MkHdr*)krypt_pdata);
    if (menu_player == 0) {
        krypt_pdata->player_state_copy = g_game_info.plyr0.player_state;
        set_player_state(&g_game_info.plyr0, 2);
        krypt_pdata->player_profile = p1_profile;
        krypt_pdata->profile_common = p1_profile_common;
        krypt_pdata->profile_konquest = p1_profile_konquest;
    } else {
        krypt_pdata->player_state_copy = g_game_info.plyr1.player_state;
        set_player_state(&g_game_info.plyr1, 2);
        move_profile_p1_to_p2();
        krypt_pdata->player_profile = p2_profile;
        krypt_pdata->profile_common = p2_profile_common;
        krypt_pdata->profile_konquest = p2_profile_konquest;
    }
    krypt_data_loaded = 0;
    krypt_pdata->tombstone_hud_ticks = 3;
    set_mode_of_play(0xB);
    push_game_state(0xB);
    mkproc_jump_sleep(p_setup_krypt);
    return 0.0f;
}

float p_krypt_mode(void) {
    set_section_memory_scheme(7);
    krypt_pdata = 0;
    _create_mkproc_generic_bigstack(0x2001, 0x23, p_init_krypt_mode, 0x150, (MkHdr**)&krypt_pdata);
    if (krypt_pdata == 0) {
        /* Retail falls back to attract via p_atm_loop. */
        gamelogic_jump(0, p_atm_loop);
    } else {
        set_process_as_scriptable(aproc);
    }
    return -1.0f;
}
