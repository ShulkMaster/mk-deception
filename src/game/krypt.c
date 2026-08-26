#include "game/krypt.h"

#include "game/bgnd.h"
#include "game/attract.h"
#include "game/game_info.h"
#include "game/menu.h"
#include "game/nbc.h"
#include "game/plyrprofile.h"
#include "platform/display_metrics.h"
#include "runtime/asset.h"
#include "runtime/cam.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
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
#include "runtime/sound.h"
#include "runtime/utils.h"

/* Krypt and Kontent gameplay for the GQNE5D retail object. */

/* --- callees outside this TU (prototypes only) --- */
int get_coffin_bit(const unsigned char* bits, unsigned int index);
void* pfx_get_field(void* pfx, int index, int type);
int pfx_get_struct_size(void* pfx, int type);
void pfx_texture_animate(
    PfxVm* pfx, float rate, int a, int b, int c, int d);
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
int check_switch(int player, int action);
void scan_switches(void);
int run_ending(int fighter);
int play_movie(int movie, int (*tapout)(void));
char* get_ending_thumbnail_name(int fighter);
RwTexture* load_named_tga_from_slot(int slot, const char* name);
RwTexture* load_tga(int slot, unsigned int oid);
void load_string_bank(unsigned int bank, char* path);
int pan_snd_req(int sound_id, float pan);
int advance_anim(AnimPdata* animation);
int pose_anim(AnimPdata* animation, int update_object);
float get_pan_value(const Vec* position);
unsigned int fx_by_owner(const char* name, int owner);
void fx_reset(unsigned int handle);
void fx_set(unsigned int handle, int parameter, float value);
unsigned int fx_next_emitter(unsigned int handle);
MkPfx* pfx_from_emitter(unsigned int handle);
int emitter_id_from_handle(unsigned int handle);
void fx_restart_emit(unsigned int handle);
void fx_set_param_v3(unsigned int handle, int parameter, float x, float y, float z);
MkObj* get_mkobj_frame(int type, void* frame);
void insert_particle_mkobj(MkObj* object);
AnimScript* get_animation(int animation_id);
void transition_to_anim_script(
    AnimPdata* animation, AnimScript* script, int flags, float blend);
MkProc* create_mkproc_anim(int pid, MkProcEntryFn entry, AnimPdata** out_animation);
void set_root_and_obj_movement_weights(float root_weight, float obj_weight, AnimPdata* animation);
void build_bones_tbl(MkObj* object, const int* tags);
void obj_create_sobjs(MkObj* object);
void insert_ground_me_mkobj(MkObj* object);
char* strlwr(char* string);
void shake_camera(int ticks, float strength);
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
    int (*jump_sleep)(MkProcEntryFn entry, float result);
} MkVtableMkprocLocal;

typedef struct KontentPdata {
    MkHdr hdr;                    /* +0x000 */
    int current_selection;        /* +0x008 */
    int item_count;               /* +0x00C */
    int items[0x1B7];             /* +0x010 */
    int category;                 /* +0x6EC */
    int player_port;              /* +0x6F0 */
    PlayerProfile* profile;       /* +0x6F4 */
    ProfileCommon* profile_common; /* +0x6F8 */
    StringObj* bio_text;          /* +0x6FC */
    unsigned int bio_text_instance; /* +0x700 */
} KontentPdata; /* 0x704 */

typedef struct KryptFogFadePdata {
    MkHdr hdr;
    int alpha_step;
} KryptFogFadePdata; /* 0x0C */

typedef struct KryptFogObject {
    MkHdr hdr;
    unsigned char field_0x08[0x30];
    float camera_z;
} KryptFogObject; /* partial, camera_z at +0x38 */

typedef struct KryptCameraFollowPdata {
    MkHdr hdr;
    void* field_0x08;
    MkObj* obj;
    unsigned int obj_instance;
} KryptCameraFollowPdata; /* 0x14 */

typedef struct KryptCharacterAnimProcPdata {
    MkHdr hdr;
    MkHdr* obj;
    unsigned int obj_instance;
    unsigned int script_index;
} KryptCharacterAnimProcPdata; /* 0x14 */

typedef struct KryptCharacterMonitorPdata {
    MkHdr hdr;
    int delay_ticks;
    int elapsed_ticks;
} KryptCharacterMonitorPdata; /* 0x10 */

extern int menu_player;
extern int mode_of_play;
extern PlayerProfile p1_profile;
extern PlayerProfile p2_profile;
extern ProfileCommon* p1_profile_common;
extern ProfileCommon* p2_profile_common;
extern void* p1_profile_konquest;
extern void* p2_profile_konquest;
extern AnimScript* bgnd_animations[];
extern MkFileEntry kon_unique_npcs_file_table[];
extern int konquest_human_bones[];
extern MkFlippedBoneMap flipped_konquest_human_bones;
extern char monk_ground_colls[];
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
extern int winner_for_ending;
extern MkHdr* empty_pdata;
extern int exec_tick_ctr;

/* krypt.o .data */
int koin_position_offsets[6] = {0xE9, -0x8D, -0xE9, -0x2E, 0x2F, 0x8C};
int tombstone_pattern[4][4] = {
    {0, 1, 2, 3},
    {1, 3, 0, 2},
    {2, 0, 3, 1},
    {3, 2, 1, 0},
};
static int award_sound_table[3] = {0x3C4, 0x3C5, 0x3C6};
static int background_sounds[35] = {
    0x3D1, 0x3D2, 0x3D3, 0x3D4, 0x3D5, 0x3D6, 0x3D7,
    0x3D8, 0x3D9, 0x3DA, 0x3DB, 0x3DC, 0x3DD, 0x3DE,
    0x3DF, 0x3E0, 0x3E1, 0x3E2, 0x3E3, 0x3E4, 0x3E5,
    0x3E6, 0x3E7, 0x3E8, 0x3E9, 0x3EA, 0x3EB, 0x3EC,
    0x3ED, 0x3EE, 0x3EF, 0x3F0, 0x3F1, 0x3F2, 0x3F3,
};

typedef struct KryptPointLightDef {
    int type;
    MkProcEntryFn proc;
    int flags;
    RwRGBAReal color;
    float radius;
    float field_20;
    float field_24;
    float field_28;
} KryptPointLightDef; /* 0x2C */

KryptPointLightDef camera_point_light = {
    2, 0, 3, {1.0f, 1.0f, 1.0f, 1.0f}, 8.0f, 0.0f, 3.0f, 0.0f,
};
int footstep_frames[2][2] = {{20, 60}, {15, 37}};
unsigned int filter_masks[6] = {0x6000, 0x10004, 0, 0x48, 0x220, 0x10};

static const CamVec3 s_cam_pos = {-1.126f, 6.534f, 58.529f};
static const CamVec3 s_cam_ang = {0.215f, 0.38f, 0.0f};

/* Tombstone HUD layout constants (krypt.o .sdata2 @2548..@2742). */
static const float s_tomb_col_spacing = 3.0f;
static const float s_tomb_col_origin = -28.5f;
static const float s_tomb_row_spacing = 5.0f;
static const float s_tomb_row_origin = 50.0f;
static const float s_tomb_z_near = 0.2f;
static const float s_tomb_z_far = 0.055f;
static const float s_tomb_digit_x_outer = 0.225f;
static const float s_tomb_digit_x_mid = 0.15f;
static const float s_tomb_digit_x_inner = 0.075f;
static const float s_tomb_koin_y = 2.5f;
static const float s_tomb_koin_z_near = 0.22f;
static const float s_tomb_koin_z_far = 0.085f;
static const float s_tomb_koin_open_uv_bias = 1.0f;

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
static float p_single_move_footstep_proc(void);
static float p_running_footstep_proc(void);
static float p_counting_sound(void);
static float p_krypt_animate(void);
static float p_fade_fog(void);
static int do_dialog(int dialog_type);
static int deduct_koins(int amount, unsigned int koin_type);
static void start_opening_coffin_effects(const Vec* origin);
static void update_use_key_string(void);
float p_fog_follow_camera(void);
void remove_prize_description(void);

static inline void mkproc_jump_sleep(MkProcEntryFn entry) {
    ((MkVtableMkprocLocal*)aproc->vtbl)->jump_sleep(entry, 0.0f);
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

static inline void place_visible_coffin_rows_from(Vec* origin) {
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
        origin->x = x;
        origin->y = 0.0f;
        origin->z = -(5.0f * (float)row - 50.0f);
        set_pebble_positions_for_row(row, col, 9, origin);
    }
}

static inline void place_visible_coffin_rows(void) {
    Vec origin = {0.0f, 0.0f, 0.0f};
    place_visible_coffin_rows_from(&origin);
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
    CoffinEntry* entries;

    coffin = kontent_pdata->items[kontent_pdata->current_selection];
    entries = coffin_data;
    if (gallery_data_loaded == 0) {
        return 0;
    }
    if (coffin < 0 || coffin > 0x1B7) {
        return 0;
    }
    return entries[coffin].long_description;
}

char* get_coffin_blurb(void) {
    int selection;
    int coffin;
    CoffinEntry* entries;

    selection = kontent_pdata->current_selection;
    coffin = kontent_pdata->items[selection];
    entries = coffin_data;
    if (selection < kontent_pdata->item_count) {
        if (kontent_pdata->category == 2) {
            return (char*)global_player_data[coffin].name;
        }
        if (gallery_data_loaded == 0) {
            return 0;
        }
        if (coffin < 0 || coffin > 0x1B7) {
            return 0;
        }
        return entries[coffin].blurb;
    }
    return "";
}
void create_fullscreen_gallery_image_list(int* out, int count) {
    int i;
    int coffin;
    unsigned int oid;
    MkFileInfo* section;
    StringObj* text;
    CoffinEntry* entry;
    int gallery_art;

    coffin = kontent_pdata->items[kontent_pdata->current_selection];
    oid = 0;
    if (gallery_data_loaded != 0) {
        gallery_art = coffin_data[coffin].gallery_art;
        section = get_mk_file_info_from_current_ssf(gallery_art);
        if (is_section_loading_or_loaded(0x150067, section) == 0) {
            unload_section_slot(0x150067);
        }
        load_art_section(0x150067, section);
        oid = (gallery_art + 0x3EA) * 0x10000U | 2;
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

static int kontent_gallery_movie_tapout(void);

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

    if (gallery_data_loaded == 0) {
        type = -1;
    } else if (coffin < 0 || coffin > 0x1B7) {
        type = -1;
    } else {
        type = coffin_data[coffin].kontent_type;
    }
    if (type == 5 || type == 9) {
        if (gallery_data_loaded == 0) {
            movie = -1;
        } else if (coffin < 0 || coffin > 0x1B7) {
            movie = -1;
        } else if ((unsigned int)coffin_data[coffin].movie_kind == 0x14U) {
            movie = coffin_data[coffin].unlock_or_movie;
        } else {
            movie = -1;
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

static int kontent_gallery_movie_tapout(void) {
    scan_switches();
    if (check_switch_edge(kontent_pdata->player_port, 0xB) != 0 ||
        check_switch_edge(kontent_pdata->player_port, 6) != 0) {
        return 1;
    }
    return 0;
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
            if (gallery_data_loaded == 0) {
                oid = 0;
            } else if (coffin < 0 || coffin > 0x1B7) {
                oid = 0;
            } else {
                oid = (unsigned int)coffin_data[coffin].gallery_art | 0x03E90000;
            }
            ((RwTexture**)out[0])[i] = load_tga(0x150068, oid);
        }
    }
}

int get_number_kontent_items(void) {
    int i;
    int valid;
    CoffinEntry* entry;

    kontent_pdata->item_count = 0;
    switch (kontent_pdata->category) {
    case 2:
        for (i = 0; i < 0x2C; i++) {
            if (is_mark_as_unlocked(kontent_pdata->profile, 6, i) != 0 &&
                get_ending_thumbnail_name(i) != 0) {
                kontent_pdata->items[kontent_pdata->item_count] = i;
                kontent_pdata->item_count++;
            }
        }
        break;
    case 0:
    case 1:
    case 4:
        for (i = 400; i < 439; i++) {
            entry = &coffin_data[i];
            if (entry == 0) {
                valid = 0;
            } else {
                valid = 1;
                if ((filter_masks[kontent_pdata->category] &
                     (1U << (entry->kontent_type & 0xFF))) == 0) {
                    valid = 0;
                }
            }
            if (valid != 0) {
                kontent_pdata->items[kontent_pdata->item_count] = i;
                kontent_pdata->item_count++;
            }
        }
        /* fall through */
    default:
    case 3:
        for (i = 0; i < 400; i++) {
            if (get_coffin_bit(kontent_pdata->profile_common->coffin_bits, i) != 0) {
                entry = &coffin_data[i];
                if (entry == 0) {
                    valid = 0;
                } else {
                    valid = 1;
                    if ((filter_masks[kontent_pdata->category] &
                         (1U << (entry->kontent_type & 0xFF))) == 0) {
                        valid = 0;
                    }
                }
                if (valid != 0) {
                    kontent_pdata->items[kontent_pdata->item_count] = i;
                    kontent_pdata->item_count++;
                }
            }
        }
        break;
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
        kontent_pdata->profile = &p1_profile;
        kontent_pdata->profile_common = p1_profile_common;
        winner_for_ending = 0;
    } else {
        kontent_pdata->player_port = g_game_info.plyr1.pad_index;
        set_player_state(&g_game_info.plyr1, 2);
        move_profile_p1_to_p2();
        kontent_pdata->profile = &p2_profile;
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
    if (proc != 0) {
        return -1.0f;
    }
    gamelogic_jump(0, p_atm_loop);
    return -1.0f;
}

/* ========================================================================= */
/* Cluster B - showcase getters and setters                                  */
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
    if (obj != 0) {
        if (obj->instance != pdata->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0) {
        return obj;
    }
    return 0;
}

int get_krypt_current_column(void) {
    return krypt_pdata->current_column;
}

int get_krypt_current_row(void) {
    return krypt_pdata->current_row;
}

void transition_to_krypt_character_anim_script(
    int animation_id, int flags, int step) {
    krypt_pdata->footstep_frame_index = 0;
    transition_to_anim_script(
        krypt_pdata->anim_pdata, get_animation(animation_id), flags, 0.1f);
    krypt_pdata->anim_pdata->step = (float)step;
}
void set_krypt_character_anim_script(
    int animation_id, int flags, void* script_args, float step) {
    krypt_pdata->footstep_frame_index = 0;
    set_anim_script(
        krypt_pdata->anim_pdata,
        (AniData*)get_animation(animation_id), flags);
    krypt_pdata->anim_pdata->step = step;
}
void set_krypt_character_previous_root_angle(void* script_args, float angle) {
    MkObjLatch* pdata;
    MkObj* obj;

    pdata = (MkObjLatch*)pdata_of_proc(aproc);
    if (pdata != 0) {
        obj = (MkObj*)pdata->obj;
        if (obj != 0) {
            if (obj->hdr.instance != pdata->obj_instance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }
        if (obj != 0) {
            obj->bone_angle_68 = angle;
            update_mkobj(obj);
        }
    }
}
void set_krypt_character_angle(void* script_args, float angle) {
    MkObjLatch* pdata;
    MkObj* obj;

    pdata = (MkObjLatch*)pdata_of_proc(aproc);
    if (pdata != 0) {
        obj = (MkObj*)pdata->obj;
        if (obj != 0) {
            if (obj->hdr.instance != pdata->obj_instance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }
        if (obj != 0) {
            obj->hide_flag_bits.pin_animation = 0;
            obj->ang.y = angle;
            update_mkobj(obj);
        }
    }
}
void set_krypt_character_pos(const Vec* position) {
    MkObjLatch* pdata;
    MkObj* obj;

    pdata = (MkObjLatch*)pdata_of_proc(aproc);
    if (pdata != 0) {
        obj = (MkObj*)pdata->obj;
        if (obj != 0) {
            if (obj->hdr.instance != pdata->obj_instance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }
        if (obj != 0) {
            obj->hide_flag_bits.pin_animation = 0;
            obj->pos.value.x = position->x;
            obj->pos.value.y = position->y;
            obj->pos.value.z = position->z;
            update_mkobj(obj);
        }
    }
}
static float p_run_character_animation(void) {
    KryptCharacterAnimProcPdata* pdata;
    MkHdr* obj;
    AnimPdata* animation;
    MkProc* animation_proc;

    pdata = (KryptCharacterAnimProcPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    cmdscript_setup_execution(g_game_info.cmdscript, pdata->script_index);
    cmdscript_execute(g_game_info.cmdscript);

    obj = pdata->obj;
    if (obj != 0) {
        if (obj->instance != pdata->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0) {
        animation = krypt_pdata->anim_pdata;
        animation_proc = animation->proc;
        if (animation_proc != 0) {
            if (animation_proc->instance != animation->proc_instance) {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        if (animation_proc != 0) {
            if (animation->proc->instance != 0) {
                animation->proc->hdr.typed_vtbl->destroy((MkHdr*)animation->proc);
            }
            krypt_pdata->anim_pdata->proc = 0;
            krypt_pdata->anim_pdata->proc_instance = 0;
        }
        if (krypt_pdata->anim_pdata->hdr.instance != 0) {
            krypt_pdata->anim_pdata->hdr.typed_vtbl->destroy(
                (MkHdr*)krypt_pdata->anim_pdata);
        }
        krypt_pdata->anim_pdata = 0;
        krypt_pdata->anim_proc = 0;
    }
    return -1.0f;
}
static inline MkObj* load_krypt_character_impl(char* character_name) {
    MkObj* object;
    MkSobj* sobj;
    char section_name[0x40];

    if (find_mkproc_pid(0x8240) != 0) {
        return 0;
    }
    load_ssf(kon_unique_npcs_file_table);
    sprintf(section_name, "kon_%s.sec", character_name);
    strlwr(section_name);
    load_art_section_by_name(0x140065, section_name);
    object = (MkObj*)load_named_model_from_slot(0x140065, "NPC", 0x8312, 0);
    if (object != 0) {
        Vec scale = {1.7f, 1.7f, 1.7f};

        obj_create_sobjs(object);
        sobj = (MkSobj*)obj_first_sobj(object);
        if (sobj != 0) {
            sobj->flags09_bits.bit4 = 0;
            sobj->flags09_bits.bit2 = 0;
        }
        object->flags_08_bits.airborne = 1;
        object->flags_08_bits.angular_velocity_enabled = 1;
        object->flags_08_bits.scale_active = 1;
        build_bones_tbl(object, konquest_human_bones);
        object->flipped_bone_map = &flipped_konquest_human_bones;
        object->flags_09_bits.launched = 1;
        object->flags_09_bits.bit6 = 1;
        object->ground_colls = monk_ground_colls;
        object->light_flags = 4;
        hide_obj(object);
        object->scale.x = scale.x;
        object->scale.y = scale.y;
        object->scale.z = scale.z;
        krypt_pdata->anim_proc =
            create_mkproc_anim(0x5002, p_krypt_animate, &krypt_pdata->anim_pdata);
        if (krypt_pdata->anim_proc != 0) {
            krypt_pdata->anim_pdata->obj = object;
            krypt_pdata->anim_pdata->obj_instance = object->hdr.instance;
            set_root_and_obj_movement_weights(0.0f, 1.0f, krypt_pdata->anim_pdata);
            krypt_pdata->anim_pdata->step = 1.0f;
            set_anim_script(
                krypt_pdata->anim_pdata,
                (AniData*)bgnd_animations[0], 0);
        }
        insert_ground_me_mkobj(object);
        insert_fgnd_mkobj(object);
        update_mkobj(object);
    }
    return object;
}

MkObj* load_krypt_character(char* character_name) {
    return load_krypt_character_impl(character_name);
}
static float p_krypt_animate(void) {
    AnimPdata* animation;
    MkObj* object;
    float frame_delta;
    float volume;

    if (krypt_pdata->anim_pdata->last_update_tick != (unsigned int)exec_tick_ctr) {
        advance_anim(krypt_pdata->anim_pdata);
        pose_anim(krypt_pdata->anim_pdata, 1);
        animation = krypt_pdata->anim_pdata;
        if (animation->high_frame == 76.0f) {
            frame_delta = animation->frame -
                          (float)footstep_frames[0][krypt_pdata->footstep_frame_index];
            if (frame_delta < 0.0f) {
                frame_delta = -frame_delta;
            }
            if (frame_delta < animation->step) {
                object = animation->obj;
                if (object != 0) {
                    if (object->hdr.instance != animation->obj_instance) {
                        object = 0;
                    }
                } else {
                    object = 0;
                }
                volume = get_volume_from_distance(&object->pos.value, 40.0f, 10.0f);
                if (volume != 0.0f) {
                    pan_vol_pitch_random_snd_req(
                        0x2B, get_pan_value(&object->pos.value), volume, 1.0f);
                }
            }
            krypt_pdata->footstep_frame_index++;
            if (krypt_pdata->footstep_frame_index >= 2) {
                krypt_pdata->footstep_frame_index = 0;
            }
        } else if (animation->high_frame == 43.0f) {
            frame_delta = animation->frame -
                          (float)footstep_frames[1][krypt_pdata->footstep_frame_index];
            if (frame_delta < 0.0f) {
                frame_delta = -frame_delta;
            }
            if (frame_delta < animation->step) {
                object = animation->obj;
                if (object != 0) {
                    if (object->hdr.instance != animation->obj_instance) {
                        object = 0;
                    }
                } else {
                    object = 0;
                }
                volume = get_volume_from_distance(&object->pos.value, 40.0f, 10.0f);
                if (volume != 0.0f) {
                    pan_vol_pitch_random_snd_req(
                        0x2A, get_pan_value(&object->pos.value), volume, 1.0f);
                }
            }
            krypt_pdata->footstep_frame_index++;
            if (krypt_pdata->footstep_frame_index >= 2) {
                krypt_pdata->footstep_frame_index = 0;
            }
        }
    }
    return 1.0f;
}
static float p_monitor_krypt_characters(void) {
    KryptCharacterMonitorPdata* pdata;
    KryptCharacterAnimProcPdata* animation_pdata;
    CameraPdata* camera;
    char** character_names;
    unsigned int* character_scripts;
    MkObj* object;
    MkProc* proc;
    unsigned int script_index;

    pdata = (KryptCharacterMonitorPdata*)pdata_of_proc(aproc);
    camera = get_pdata_of_camera();
    if (camera == 0) {
        return -1.0f;
    }
    if (pdata->elapsed_ticks >= pdata->delay_ticks && (camera->flags & 0x80) != 0) {
        character_names = (char**)get_data_table_by_name("krypt_character_names");
        character_scripts =
            (unsigned int*)get_data_table_by_name("krypt_character_info_table");
        object = load_krypt_character_impl(
            character_names[randu0((unsigned short)get_row_count_for_table_by_pointer(
                g_game_info.cmdscript, character_names))]);
        script_index = character_scripts[randu0(
            (unsigned short)get_row_count_for_table_by_pointer(
                g_game_info.cmdscript, character_scripts))];
        if (object != 0) {
            animation_pdata = 0;
            proc = _create_mkproc_generic_bigstack(
                0x8240, 0x1F, p_run_character_animation, 0x14,
                (MkHdr**)&animation_pdata);
            if (proc != 0 && animation_pdata != 0) {
                zero_pdata_payload(0x14, &animation_pdata->hdr);
                animation_pdata->obj = &object->hdr;
                animation_pdata->obj_instance = object->hdr.instance;
                animation_pdata->script_index = script_index;
                set_process_as_scriptable(proc);
                mk_insert(&object->hdr, &proc->pdata_list);
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        mkproc_sleep();
        while (find_mkproc_pid(0x8240) != 0) {
            _mkproc_sleep_ticks = 1.0f;
            mkproc_sleep();
        }
        pdata->delay_ticks = randu0(0x12C) + 0x12C;
        pdata->elapsed_ticks = 0;
    } else {
        pdata->elapsed_ticks++;
    }
    return 1.0f;
}
static float p_play_random_noise(void) {
    static unsigned short number_of_sounds_left = 35;
    int* end;
    unsigned short index;
    int last_sound;
    int sound;
    float pan;
    unsigned int delay;

    index = (unsigned short)randu0(number_of_sounds_left);
    end = &background_sounds[number_of_sounds_left];
    number_of_sounds_left--;
    sound = background_sounds[index];
    last_sound = end[-1];
    end[-1] = sound;
    background_sounds[index] = last_sound;
    if (number_of_sounds_left == 0) {
        number_of_sounds_left = 35;
    }

    pan = sfrand(2.0f);
    delay = (unsigned short)randu0(180) + 480;
    pan_snd_req(sound, pan);
    return (float)delay;
}

/* ========================================================================= */
/* Cluster C - tombstone HUD pfx and coffin-grid pebbles                     */
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
    } else {
    dst_f = (float*)pfx_get_field(pfx, -2, 0x301);
    src_f = (float*)pfx_get_field(pfx, -1, 0x301);
    stride_f = pfx_get_struct_size(pfx, 0x301);
    src_i = (int*)pfx_get_field(pfx, -1, 0x100);
    dst_i = (int*)pfx_get_field(pfx, -2, 0x100);
    stride_i = pfx_get_struct_size(pfx, 0x100);

    for (i = 0; i < pfx->count; i++) {
        dst_i[0] = src_i[0];
        dst_i[1] = src_i[1];
        dst_i[2] = src_i[2];
        src_i = (int*)((char*)src_i + stride_i);
        dst_i = (int*)((char*)dst_i + stride_i);
        *dst_f = *src_f;
        src_f = (float*)((char*)src_f + stride_f);
        dst_f = (float*)((char*)dst_f + stride_f);
    }
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
    pfx = (TombstonePfx*)pfx_arg;
    pfx->capacity = particle_count;
    pfx->count = particle_count;

    pfx->flags_150_bits.bit6 = 1;
    pfx->scale = scale;

    pfx->flags_150_bits.bit7 = 1;
    pfx->flags_150_bits.bit4 = 1;

    pfx->mat_a = 1.0f;
    pfx->mat_b = 0.0f;
    pfx->mat_c = 0.0f;
    pfx->mat_d = 0.0f;
    pfx->mat_e = 1.0f;
    pfx->mat_f = 0.0f;
    pfx_native_set_rgba(&pfx->rgba_1B4, 0.0f, 0.0f, 0.0f, 255.0f);

    pfx->flags_150_bits.bit3 = 1;

    pfx->uv_scale = 1.0f;
    pfx->uv_0 = 0.0f;
    pfx->uv_rate = 0.0125f;
    pfx_native_set_rgba(&pfx->rgba_160, 255.0f, 255.0f, 255.0f, 0.0f);

    /* Texture path/name are retail immediates (section-relative ids). */
    set_pfx_texture((PfxVm*)pfx, (void*)0x00140064, tex_name);
    pfx_texture_animate((PfxVm*)pfx, 1.0f, anim_a, anim_b, anim_c, anim_d);
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
    int is_near;

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
            opened = get_coffin_bit(
                krypt_pdata->profile_common->coffin_bits,
                col_off + (view_col + row * 0x14));
            col = view_col + col_off;
            coffin_idx = col + row * 0x14;
            letter_bias = opened ? 0x20 : 0;

            pos[0] = base_x - 0.14f;
            pos[1] = 1.9f;
            is_near = coffin_data[coffin_idx].coffin_type == 4;
            if (is_near != 0) pos[2] = z_near;
            else pos[2] = z_far;
            pos = (float*)((char*)pos + pos_stride);
            *uv = (float)(letter_bias + row);
            uv = (float*)((char*)uv + uv_stride);

            pos[0] = base_x + 0.14f;
            pos[1] = 1.9f;
            if ((unsigned int)coffin_data[coffin_idx].coffin_type == 4U)
                pos[2] = z_near;
            else pos[2] = z_far;
            pos = (float*)((char*)pos + pos_stride);
            *uv = (float)(letter_bias + col);
            uv = (float*)((char*)uv + uv_stride);
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
    int fill;
    float base_x;
    float base_z;
    float z_near;
    float z_far;
    int row;
    int col;
    int coffin_idx;
    unsigned int cost;
    unsigned int thousands;
    unsigned int hundreds;
    unsigned int tens;
    unsigned int ones;
    unsigned int slot;

    particle = 0;
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
            cost = coffin_data[coffin_idx].cost;
            thousands = cost / 1000;
            hundreds = (cost / 100) - thousands * 10;
            tens = (cost / 10) - thousands * 100 - hundreds * 10;
            ones = cost - thousands * 1000 - hundreds * 100 - tens * 10;
            slot = 0;

            if (thousands != 0) {
                pos[0] = base_x - 0.225f;
                if ((unsigned int)coffin_data[coffin_idx].prize_kind == 6U)
                    pos[1] = -10.0f;
                else pos[1] = 1.6f;
                if ((unsigned int)coffin_data[coffin_idx].coffin_type == 4U)
                    pos[2] = z_near;
                else pos[2] = z_far;
                pos = (float*)((char*)pos + pos_stride);
                particle += 1;
                slot = 4;
                *uv = (float)thousands;
                uv = (float*)((char*)uv + uv_stride);
            }

            if (slot == 4) {
                pos[0] = base_x - s_tomb_digit_x_inner;
            } else if (hundreds != 0) {
                slot = 3;
                pos[0] = base_x - s_tomb_digit_x_mid;
            }

            if (slot >= 3) {
                if ((unsigned int)coffin_data[coffin_idx].prize_kind == 6U)
                    pos[1] = -10.0f;
                else pos[1] = 1.6f;
                if ((unsigned int)coffin_data[coffin_idx].coffin_type == 4U)
                    pos[2] = z_near;
                else pos[2] = z_far;
                pos = (float*)((char*)pos + pos_stride);
                particle += 1;
                *uv = (float)hundreds;
                uv = (float*)((char*)uv + uv_stride);
            }

            if (slot == 4) {
                pos[0] = base_x + s_tomb_digit_x_inner;
            } else if (slot == 3) {
                pos[0] = base_x;
            } else if (tens != 0) {
                slot = 2;
                pos[0] = base_x - s_tomb_digit_x_inner;
            }

            if (slot >= 2) {
                if ((unsigned int)coffin_data[coffin_idx].prize_kind == 6U)
                    pos[1] = -10.0f;
                else pos[1] = 1.6f;
                if ((unsigned int)coffin_data[coffin_idx].coffin_type == 4U)
                    pos[2] = z_near;
                else pos[2] = z_far;
                pos = (float*)((char*)pos + pos_stride);
                particle += 1;
                *uv = (float)tens;
                uv = (float*)((char*)uv + uv_stride);
            }

            if (slot == 4) {
                pos[0] = base_x + s_tomb_digit_x_outer;
            } else if (slot == 3) {
                pos[0] = base_x + s_tomb_digit_x_mid;
            } else if (slot == 2) {
                pos[0] = base_x + s_tomb_digit_x_inner;
            } else {
                pos[0] = base_x;
                slot = 1;
            }

            if (slot >= 1) {
                if ((unsigned int)coffin_data[coffin_idx].prize_kind == 6U)
                    pos[1] = -10.0f;
                else pos[1] = 1.6f;
                if ((unsigned int)coffin_data[coffin_idx].coffin_type == 4U)
                    pos[2] = z_near;
                else pos[2] = z_far;
                pos = (float*)((char*)pos + pos_stride);
                particle += 1;
                *uv = (float)ones;
                uv = (float*)((char*)uv + uv_stride);
            }

            base_x += s_tomb_col_spacing;
        }
        base_z -= s_tomb_row_spacing;
        base_x = tombstone_col_x(view_col);
    }

    fill = particle;
    while (fill < ((TombstonePfx*)pfx)->count) {
        pos[0] = 0.0f;
        pos[1] = -1.0f;
        pos[2] = 0.0f;
        pos = (float*)((char*)pos + pos_stride);
        *uv = 0.0f;
        uv = (float*)((char*)uv + uv_stride);
        fill += 1;
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
    int is_near;
    int is_open;

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
            pos[0] = base_x;
            pos[1] = 2.5f;
            is_near = coffin_data[coffin_idx].coffin_type == 4;
            if (is_near != 0) pos[2] = z_near;
            else pos[2] = z_far;
            *uv = (float)(unsigned int)coffin_data[coffin_idx].prize_kind;
            if ((unsigned int)coffin_data[coffin_idx].prize_kind == 6U) {
                is_open = get_coffin_bit(
                    krypt_pdata->profile_common->coffin_bits, coffin_idx) != 0;
                if (is_open != 0) *uv += 1.0f;
            }
            uv = (float*)((char*)uv + uv_stride);
            pos = (float*)((char*)pos + pos_stride);
            base_x += s_tomb_col_spacing;
        }
        base_z -= s_tomb_row_spacing;
        base_x = tombstone_col_x(view_col);
    }
}

void force_wallet_to_open_position(void) {
    ScreenObj* wallet_back;
    ScreenObj* wallet_front;
    StringObj* text;
    char value[12];
    int i;

    wallet_back = krypt_pdata->wallet_back.obj;
    if (wallet_back != 0) {
        if (wallet_back->instance != krypt_pdata->wallet_back.obj_instance) {
            wallet_back = 0;
        }
    } else {
        wallet_back = 0;
    }
    wallet_front = krypt_pdata->wallet_front.obj;
    if (wallet_front != 0) {
        if (wallet_front->instance != krypt_pdata->wallet_front.obj_instance) {
            wallet_front = 0;
        }
    } else {
        wallet_front = 0;
    }
    if (wallet_back != 0 && wallet_front != 0) {
        wallet_back->y = 0;
        wallet_front->y = 0;
    }

    for (i = 0; i < 6; i++) {
        text = krypt_pdata->wallet_text[i].obj;
        if (text != 0) {
            if (text->instance != krypt_pdata->wallet_text[i].obj_instance) {
                text = 0;
            }
        } else {
            text = 0;
        }
        if (text != 0) {
            text->y = 0x22;
        }
    }

    for (i = 0; i < 6; i++) {
        format_value_to_display(value, krypt_pdata->profile_common->koin_totals[i]);
        text = krypt_pdata->wallet_text[i].obj;
        if (text != 0) {
            if (text->instance != krypt_pdata->wallet_text[i].obj_instance) {
                text = 0;
            }
        } else {
            text = 0;
        }
        if (text != 0) {
            update_string_obj(text, 0, value);
        } else {
            text = string_center_xy(0x8310, 0, value,
                                    screen_width / 2 + koin_position_offsets[i], 0x22, 0x4B);
            krypt_pdata->wallet_text[i].obj = text;
            krypt_pdata->wallet_text[i].obj_instance = text->instance;
        }
    }
    krypt_pdata->wallet_open_ticks = 0x78;
    krypt_pdata->wallet_open = 1;
}
void heads_up_display_visible(int visible) {
    ScreenObj* wallet_back;
    ScreenObj* wallet_front;
    ScreenObj* icon_a;
    ScreenObj* icon_b;
    StringObj* label_a;
    StringObj* label_b;
    StringObj* value_a;
    StringObj* value_b;
    StringObj* wallet_text;
    int i;

    wallet_back = krypt_pdata->wallet_back.obj;
    if (wallet_back != 0) {
        if (wallet_back->instance != krypt_pdata->wallet_back.obj_instance) {
            wallet_back = 0;
        }
    } else {
        wallet_back = 0;
    }
    wallet_front = krypt_pdata->wallet_front.obj;
    if (wallet_front != 0) {
        if (wallet_front->instance != krypt_pdata->wallet_front.obj_instance) {
            wallet_front = 0;
        }
    } else {
        wallet_front = 0;
    }
    icon_a = krypt_pdata->open_button.obj;
    if (icon_a != 0) {
        if (icon_a->instance != krypt_pdata->open_button.obj_instance) {
            icon_a = 0;
        }
    } else {
        icon_a = 0;
    }
    icon_b = krypt_pdata->exit_button.obj;
    if (icon_b != 0) {
        if (icon_b->instance != krypt_pdata->exit_button.obj_instance) {
            icon_b = 0;
        }
    } else {
        icon_b = 0;
    }
    label_a = krypt_pdata->hud_label_l.obj;
    if (label_a != 0) {
        if (label_a->instance != krypt_pdata->hud_label_l.obj_instance) {
            label_a = 0;
        }
    } else {
        label_a = 0;
    }
    value_a = krypt_pdata->hud_string_20011.obj;
    if (value_a != 0) {
        if (value_a->instance != krypt_pdata->hud_string_20011.obj_instance) {
            value_a = 0;
        }
    } else {
        value_a = 0;
    }
    label_b = krypt_pdata->hud_label_r.obj;
    if (label_b != 0) {
        if (label_b->instance != krypt_pdata->hud_label_r.obj_instance) {
            label_b = 0;
        }
    } else {
        label_b = 0;
    }
    value_b = krypt_pdata->use_key_string.obj;
    if (value_b != 0) {
        if (value_b->instance != krypt_pdata->use_key_string.obj_instance) {
            value_b = 0;
        }
    } else {
        value_b = 0;
    }

    if (wallet_back != 0) {
        if (visible) unhide_screen_obj(wallet_back); else hide_screen_obj(wallet_back);
    }
    if (wallet_front != 0) {
        if (visible) unhide_screen_obj(wallet_front); else hide_screen_obj(wallet_front);
    }
    if (icon_a != 0) {
        if (visible) unhide_screen_obj(icon_a); else hide_screen_obj(icon_a);
    }
    if (icon_b != 0) {
        if (visible) unhide_screen_obj(icon_b); else hide_screen_obj(icon_b);
    }
    if (label_a != 0 && value_a != 0) {
        if (visible) {
            unhide_string_obj(label_a);
            unhide_string_obj(value_a);
        } else {
            hide_string_obj(label_a);
            hide_string_obj(value_a);
        }
    }
    if (label_b != 0 && value_b != 0) {
        if (krypt_pdata->available_key_coffin && visible) {
            unhide_string_obj(label_b);
            unhide_string_obj(value_b);
        } else {
            hide_string_obj(label_b);
            hide_string_obj(value_b);
        }
    }
    for (i = 0; i < 6; i++) {
        wallet_text = krypt_pdata->wallet_text[i].obj;
        if (wallet_text != 0) {
            if (wallet_text->instance != krypt_pdata->wallet_text[i].obj_instance) {
                wallet_text = 0;
            }
        } else {
            wallet_text = 0;
        }
        if (wallet_text != 0) {
            if (visible) unhide_string_obj(wallet_text); else hide_string_obj(wallet_text);
        }
    }
}
void init_heads_up_display(void) {
    ScreenObj* screen_obj;
    StringObj* string_obj;
    StringObj* right_label;
    StringObj* right_text;
    ScreenObj* award_notice_top;
    ScreenObj* award_notice_left;
    ScreenObj* award_notice_right;
    ScreenObj* award_notice_bottom;
    ScreenObj* award_frame;
    char value[12];
    int i;

    screen_obj = load_named_2d_pfxobj(0x140066, 0x830F, "OPEN_BUTTON", 0, 0x4C);
    if (screen_obj != 0) {
        screen_obj->x = screen_width - 0x32 - screen_obj->pfx2d->tex_w;
        screen_obj->y = screen_height - 0x52;
        krypt_pdata->open_button.obj = screen_obj;
        krypt_pdata->open_button.obj_instance = screen_obj->instance;
    }
    screen_obj = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "EXIT_BUTTON", 0, 0x32, screen_height - 0x52, 0x4C);
    krypt_pdata->exit_button.obj = screen_obj;
    krypt_pdata->exit_button.obj_instance = screen_obj->instance;

    string_obj = string_left_xy(0x8310, 7, "l", 0x32, 0x32, 0x4D);
    krypt_pdata->hud_label_l.obj = string_obj;
    krypt_pdata->hud_label_l.obj_instance = string_obj->instance;
    string_obj = string_left_xy(
        0x8310, 0, get_string_by_id(0x20011), 0x56, 0x37, 0x4D);
    krypt_pdata->hud_string_20011.obj = string_obj;
    krypt_pdata->hud_string_20011.obj_instance = string_obj->instance;
    right_label = string_right_xy(
        0x8310, 7, "R", screen_width - 0x32, 0x32, 0x4D);
    krypt_pdata->hud_label_r.obj = right_label;
    krypt_pdata->hud_label_r.obj_instance = right_label->instance;
    right_text = string_right_xy(
        0x8310, 0, get_string_by_id(0x20012), screen_width - 0x5A, 0x37, 0x4D);
    krypt_pdata->use_key_string.obj = right_text;
    krypt_pdata->use_key_string.obj_instance = right_text->instance;

    screen_obj = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "WALLET_A", 0, screen_width / 2 - 0x140, 0, 0x4C);
    krypt_pdata->wallet_back.obj = screen_obj;
    krypt_pdata->wallet_back.obj_instance = screen_obj->instance;
    screen_obj = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "WALLET_B", 0, screen_width / 2 + 0xC0, 0, 0x4C);
    krypt_pdata->wallet_front.obj = screen_obj;
    krypt_pdata->wallet_front.obj_instance = screen_obj->instance;

    for (i = 0; i < 6; i++) {
        format_value_to_display(value, krypt_pdata->profile_common->koin_totals[i]);
        string_obj = krypt_pdata->wallet_text[i].obj;
        if (string_obj != 0) {
            if (string_obj->instance != krypt_pdata->wallet_text[i].obj_instance) {
                string_obj = 0;
            }
        } else {
            string_obj = 0;
        }
        if (string_obj != 0) {
            update_string_obj(string_obj, 0, value);
        } else {
            string_obj = string_center_xy(
                0x8310, 0, value, screen_width / 2 + koin_position_offsets[i], 0x22, 0x4B);
            krypt_pdata->wallet_text[i].obj = string_obj;
            krypt_pdata->wallet_text[i].obj_instance = string_obj->instance;
        }
    }

    award_notice_top = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "AWARD_NOTICE_TOP", 0,
        screen_width / 2 - 0x100, screen_height / 2 + 0x80, 0x4C);
    krypt_pdata->award_notice_top.obj = award_notice_top;
    krypt_pdata->award_notice_top.obj_instance = award_notice_top->instance;
    award_notice_left = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "AWARD_NOTICE_LEFT", 0,
        screen_width / 2 - 0xD0, screen_height / 2 - 0x80, 0x4C);
    krypt_pdata->award_notice_left.obj = award_notice_left;
    krypt_pdata->award_notice_left.obj_instance = award_notice_left->instance;
    award_notice_right = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "AWARD_NOTICE_RIGHT", 0,
        screen_width / 2 + 0xC0, screen_height / 2 - 0x80, 0x4C);
    krypt_pdata->award_notice_right.obj = award_notice_right;
    krypt_pdata->award_notice_right.obj_instance = award_notice_right->instance;
    award_notice_bottom = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "AWARD_NOTICE_BOTTOM", 0,
        screen_width / 2 - 0x100, screen_height / 2 - 0xC0, 0x4C);
    krypt_pdata->award_notice_bottom.obj = award_notice_bottom;
    krypt_pdata->award_notice_bottom.obj_instance = award_notice_bottom->instance;
    award_frame = load_named_2d_pfxobj_xy(
        0x140066, 0x830F, "AWARDFRAME", 0,
        screen_width / 2 - 0x80, screen_height / 2 - 0x80, 0x4C);
    krypt_pdata->award_frame.obj = award_frame;
    krypt_pdata->award_frame.obj_instance = award_frame->instance;

    krypt_pdata->wallet_open_ticks = 0x78;
    krypt_pdata->wallet_open = 1;
    hide_screen_obj(award_notice_top);
    hide_screen_obj(award_notice_left);
    hide_screen_obj(award_notice_right);
    hide_screen_obj(award_notice_bottom);
    hide_screen_obj(award_frame);
    if (krypt_pdata->available_key_coffin != 0) {
        update_use_key_string();
    } else {
        hide_string_obj(right_label);
        hide_string_obj(right_text);
    }
}

/* ========================================================================= */
/* Cluster D - coffin open and FX                                            */
/* ========================================================================= */

static inline void display_prize_description_impl(
    CoffinEntry* entries, int index, int last_index, int available,
    unsigned int string_bank, int priority) {
    const char* title;
    char* description;
    StringObj* description_obj;
    int kontent_type;

    if (!available) {
        kontent_type = -1;
    } else if (index < 0 || index > last_index) {
        kontent_type = -1;
    } else {
        kontent_type = entries[index].kontent_type;
    }
    title = get_string_by_id(string_bank | kontent_type);
    string_center_xy(
        0xA00F, 9, title, screen_width / 2,
        screen_height / 2 + 0x8A, priority);
    if (!available) {
        description = 0;
    } else if (index < 0 || index > last_index) {
        description = 0;
    } else {
        description = entries[index].blurb;
    }
    if ((unsigned int)entries[index].kontent_type == 0xC &&
        (get_language() == 0 || get_language() == 5)) {
        char* separator = strchr(description, ':');
        if (separator != 0) *separator = '\n';
    }
    if (description != 0) {
        description_obj = create_wrapped_string(
            0xA00F, load_font(9), description, screen_width / 2 - 0xC0,
            screen_height / 2 - 0x87, 0x180, 0, 1, 0);
        if (description_obj != 0) {
            description_obj->priority = priority;
            insert_2d_obj((ScreenObj*)description_obj);
        }
    }
}

static inline void move_picture_to_camera_impl(KryptScreenObjLatch* picture_latch) {
    ScreenObj* picture;

    picture = picture_latch->obj;
    if (picture != 0) {
        if (picture->instance != picture_latch->obj_instance) picture = 0;
    } else {
        picture = 0;
    }
    unhide_screen_obj(picture);
    picture->scale_x = 0.1f;
    picture->scale_y = 0.1f;
    picture->x = screen_width / 2;
    picture->y = screen_height / 2;
    while (picture->scale_x < 2.0f) {
        picture->scale_x += 0.1f;
        picture->scale_y += 0.1f;
        picture->x = screen_width / 2 -
            (int)(256.0f * picture->scale_x * 0.5f);
        picture->y = screen_height / 2 -
            (int)(256.0f * picture->scale_y * 0.5f);
        _mkproc_sleep_ticks = 1.0f;
        mkproc_sleep();
    }
    hide_screen_obj(picture);
    picture->scale_x = 1.0f;
    picture->scale_y = 1.0f;
}

static inline ScreenObj* live_screen_latch(KryptScreenObjLatch* latch) {
    ScreenObj* object = latch->obj;
    if (object != 0) {
        if (object->instance != latch->obj_instance) object = 0;
    } else {
        object = 0;
    }
    return object;
}

static float p_move_camera_and_open_coffin(void) {
    Vec camera_angles = {1.0f, 3.4415927f, 0.0f};
    Vec camera_offset = {0.8f, 5.0f, 4.0f};
    Vec coffin_position;
    CameraPdata* camera;
    CoffinEntry* entry;
    MkSobj* coffin;
    MkSobj* lid;
    ScreenObj* object;
    ScreenObj* notice_top;
    ScreenObj* notice_left;
    ScreenObj* notice_right;
    ScreenObj* notice_bottom;
    StringObj* wallet_text;
    MkFileInfo* section;
    unsigned int effect;
    unsigned int art_oid;
    unsigned int koin_type;
    unsigned int old_total;
    int selected;
    int gallery_art;

    selected = krypt_pdata->current_column + krypt_pdata->current_row * 20;
    camera = get_pdata_of_camera();
    if (camera == 0) {
        return -1.0f;
    }
    camera->speed = 0.5f;
    heads_up_display_visible(0);

    {
        Vec coffin_offset = {-0.582f, -0.503f, 1.4008f};
        coffin_position.x = 3.0f * krypt_pdata->current_column + -28.5f;
        coffin_position.y = 0.0f;
        coffin_position.z = -(5.0f * krypt_pdata->current_row - 50.0f);

        coffin = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x3C);
        if (coffin != 0) {
            coffin->pos.x = coffin_position.x + coffin_offset.x;
            coffin->pos.y = coffin_position.y + coffin_offset.y;
            coffin->pos.z = coffin_position.z + coffin_offset.z;
        }
    }
    {
        Vec origin = {0.0f, 0.0f, 0.0f};
        krypt_pdata->coffin_pebble_type0->count = 0;
        krypt_pdata->coffin_pebble_type1->count = 0;
        krypt_pdata->coffin_pebble_type2->count = 0;
        krypt_pdata->coffin_pebble_type3->count = 0;
        krypt_pdata->lid_closed_pebbles->count = 0;
        krypt_pdata->lid_open_pebbles->count = 0;
        place_visible_coffin_rows_from(&origin);
    }
    krypt_pdata->tombstone_hud_ticks = 3;

    {
        KryptFogFadePdata* fade_pdata = 0;
        if (_create_mkproc_generic_tinystack(
                0x823F, 0x1F, p_fade_fog, sizeof(KryptFogFadePdata),
                (MkHdr**)&fade_pdata) != 0 && fade_pdata != 0) {
            fade_pdata->alpha_step = -8;
        }
    }

    camera->target_pos.x = coffin_position.x + camera_offset.x;
    camera->target_pos.y = coffin_position.y + camera_offset.y;
    camera->target_pos.z = coffin_position.z + camera_offset.z;
    camera->target_ang.x = camera_angles.x;
    camera->target_ang.y = camera_angles.y;
    camera->target_ang.z = camera_angles.z;
    camera->flags_bits.pos_done = 0;
    _mkproc_sleep_ticks = 1.0f;
    mkproc_sleep();

    start_opening_coffin_effects(&coffin_position);
    shake_camera(10, 0.0075f);
    coffin = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x3C);
    if (coffin != 0) {
        snd_req(0x3BB);
        snd_req(0x3BC);
        coffin->pos_vel.y = 0.02f;
        while (coffin->pos.y < 0.5f) {
            _mkproc_sleep_ticks = 1.0f;
            mkproc_sleep();
        }
        coffin->pos_vel.y = 0.0f;
    }

    lid = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x3D);
    if (lid != 0) {
        snd_req(0x3BD);
        lid->ang_vel.z = 0.06f;
        while (lid->ang.z < 2.3561945f) {
            _mkproc_sleep_ticks = 1.0f;
            mkproc_sleep();
        }
        lid->ang_vel.z = 0.0f;
    }

    move_picture_to_camera_impl(&krypt_pdata->award_frame);
    notice_top = live_screen_latch(&krypt_pdata->award_notice_top);
    notice_left = live_screen_latch(&krypt_pdata->award_notice_left);
    notice_right = live_screen_latch(&krypt_pdata->award_notice_right);
    notice_bottom = live_screen_latch(&krypt_pdata->award_notice_bottom);
    unhide_screen_obj(notice_top);
    unhide_screen_obj(notice_left);
    unhide_screen_obj(notice_right);
    unhide_screen_obj(notice_bottom);

    if (krypt_pdata->award_image_left == 0) {
        load_ssf(krypt_art_file_table);
        art_oid = 0;
        entry = &coffin_data[
            krypt_pdata->current_column + krypt_pdata->current_row * 20];
        if (krypt_data_loaded != 0) {
            gallery_art = entry->gallery_art;
            section = get_mk_file_info_from_current_ssf(gallery_art);
            if (is_section_loading_or_loaded(0x150067, section) == 0) {
                unload_section_slot(0x150067);
            }
            load_art_section(0x150067, section);
            art_oid = (gallery_art + 0x3EA) << 16;
        }
        krypt_pdata->award_image_left = load_2d_pfxobj_xy(
            0x150067, 0x830F, (char*)art_oid, 0,
            screen_width / 2 - 0xC0, screen_height / 2 - 0x80, 0x4C);
        krypt_pdata->award_image_right = load_2d_pfxobj_xy(
            0x150067, 0x830F, (char*)(art_oid + 1), 0,
            screen_width / 2 + 0x40, screen_height / 2 - 0x80, 0x4C);
    }
    display_prize_description_impl(
        coffin_data, selected, 0x190, krypt_data_loaded, 0x20000, 0x4A);
    entry = &coffin_data[
        krypt_pdata->current_column + krypt_pdata->current_row * 20];
    snd_req(award_sound_table[entry->award_sound_index]);

    object = load_named_2d_pfxobj(0x140066, 0x830F, "OK_BUTTON", 0, 0x4C);
    if (object != 0) {
        object->x = screen_width - 0x32 - object->pfx2d->tex_w;
        object->y = 0x32;
    }
    for (;;) {
        if (check_switch_edge(krypt_pdata->player_port, 6) != 0) {
            _mkproc_sleep_ticks = 1.0f;
            mkproc_sleep();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        mkproc_sleep();
    }
    if (object->instance != 0) {
        object->typed_vtbl->destroy(object);
    }

    destroy_mkobjs_oid(0x8311);
    notice_top = live_screen_latch(&krypt_pdata->award_notice_top);
    notice_left = live_screen_latch(&krypt_pdata->award_notice_left);
    notice_right = live_screen_latch(&krypt_pdata->award_notice_right);
    notice_bottom = live_screen_latch(&krypt_pdata->award_notice_bottom);
    hide_screen_obj(notice_top);
    hide_screen_obj(notice_left);
    hide_screen_obj(notice_right);
    hide_screen_obj(notice_bottom);
    object = krypt_pdata->award_image_left;
    hide_screen_obj(object);
    object = krypt_pdata->award_image_left;
    if (object->instance != 0) object->typed_vtbl->destroy(object);
    krypt_pdata->award_image_left = 0;
    object = krypt_pdata->award_image_right;
    hide_screen_obj(object);
    object = krypt_pdata->award_image_right;
    if (object->instance != 0) object->typed_vtbl->destroy(object);
    krypt_pdata->award_image_right = 0;
    del_string_obj_by_id(0xA00F);

    lid = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x3D);
    if (lid != 0) {
        snd_req(0x3BE);
        lid->ang_vel.z = -0.06f;
        while (lid->ang.z - 0.08f > 0.0f) {
            _mkproc_sleep_ticks = 1.0f;
            mkproc_sleep();
        }
        lid->ang_vel.z = 0.0f;
        lid->ang.z = 0.0f;
        snd_req(0x3BF);
    }

    camera->target_pos.x = coffin_position.x;
    camera->target_pos.y = coffin_position.y;
    camera->target_pos.z = coffin_position.z;
    camera->target_pos.y = 4.4f;
    camera->target_pos.z += 5.0f;
    camera->target_ang.x = 0.42f;
    camera->target_ang.y = 3.1415927f;
    camera->speed = 1.0f;
    shake_camera(8, 0.0075f);
    {
        KryptFogFadePdata* fade_pdata = 0;
        if (_create_mkproc_generic_tinystack(
                0x823F, 0x1F, p_fade_fog, sizeof(KryptFogFadePdata),
                (MkHdr**)&fade_pdata) != 0 && fade_pdata != 0) {
            fade_pdata->alpha_step = 8;
        }
    }

    {
        Vec dust_offset = {0.0f, 0.25f, 2.0f};
        Vec effect_position = {0.0f, 0.0f, 0.0f};
        effect = fx_by_owner("dirt_fountain", 4);
        fx_reset(effect);
        effect_position.x = coffin_position.x + dust_offset.x;
        effect_position.y = coffin_position.y + dust_offset.y;
        effect_position.z = coffin_position.z + dust_offset.z;
        fx_set_param_v3(
            effect, 0x202, effect_position.x, effect_position.y,
            effect_position.z);
        fx_restart_emit(effect);
        effect = fx_by_owner("coffin_dust_pfx", 4);
        if (effect != 0) {
            fx_set(effect, 0x204, 1.0f);
        }
    }

    coffin = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x3C);
    if (coffin != 0) {
        snd_req(0x3C0);
        coffin->pos_vel.y = -0.02f;
        while (coffin->pos.y > -0.503f) {
            _mkproc_sleep_ticks = 1.0f;
            mkproc_sleep();
        }
        coffin->pos_vel.y = 0.0f;
        coffin->pos.y = -0.503f;
    }
    heads_up_display_visible(1);

    if (krypt_pdata->award_applied == 0) {
        ProfileCommon* profile;
        unsigned int* totals;
        int amount;

        entry = &coffin_data[
            krypt_pdata->current_column + krypt_pdata->current_row * 20];
        if ((unsigned int)entry->movie_kind >= 100U) {
            profile = krypt_pdata->profile_common;
            amount = entry->unlock_or_movie;
            koin_type = entry->movie_kind - 100;
            totals = profile->koin_totals;
            old_total = totals[koin_type];
            totals[koin_type] += amount;
            profile->koin_earned[koin_type] += amount;
            if (save_profile(menu_player, 2) == 0) {
                totals[koin_type] -= amount;
                profile->koin_earned[koin_type] -= amount;
                _mkproc_sleep_ticks = 1.0f;
                mkproc_sleep();
            } else {
                _create_mkproc_generic_tinystack(
                    0x8246, 0x1F, p_counting_sound, 8, &empty_pdata);
                wallet_text = krypt_pdata->wallet_text[koin_type].obj;
                if (wallet_text != 0) {
                    if (wallet_text->instance !=
                        krypt_pdata->wallet_text[koin_type].obj_instance) {
                        wallet_text = 0;
                    }
                } else {
                    wallet_text = 0;
                }
                display_numerical_change(
                    wallet_text, 0, old_total, amount, 1, 0x14);
                destroy_mkprocs_pid(0x8246);
                _mkproc_sleep_ticks = 30.0f;
                mkproc_sleep();
            }
        }
    }
    mkproc_jump_sleep(p_krypt_loop);
    return 0.0f;
}
static float p_fade_fog(void) {
    KryptFogFadePdata* pdata;
    int alpha;
    int step;

    pdata = (KryptFogFadePdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }

    step = pdata->alpha_step;
    if (obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x46) != 0) {
        if (step < 0) {
            alpha = 0xFF;
        } else {
            alpha = 0;
        }

        while ((unsigned int)alpha <= 0xFF) {
            obj_set_sobj_alpha(g_game_info.bgnd_obj, 0x46, alpha);
            _mkproc_sleep_ticks = 1.0f;
            mkproc_sleep();
            alpha += step;
        }
    }

    return -1.0f;
}
void remove_prize_description(void) {
    del_string_obj_by_id(0xA00F);
}
void display_prize_description(CoffinEntry* entries, int index, int last_index, int available,
                               unsigned int string_bank, int priority) {
    display_prize_description_impl(
        entries, index, last_index, available, string_bank, priority);
}
void move_picture_to_camera(KryptScreenObjLatch* picture_latch) {
    move_picture_to_camera_impl(picture_latch);
}
static void start_opening_coffin_effects(const Vec* origin) {
    Vec object_offsets[5] = {
        {-0.1f, 0.0f, 0.8f},
        {-0.1f, 0.0f, 2.2f},
        {0.0f, 0.0f, 2.5f},
        {0.1f, 0.0f, 2.2f},
        {0.1f, 0.0f, 0.8f},
    };
    float object_angles[5] = {-1.5707964f, -1.5707964f, 0.0f, 1.5707964f, 1.5707964f};
    Vec dust_offset = {0.0f, 0.25f, 2.0f};
    Vec dirt_position = {0.0f, 0.0f, 0.0f};
    unsigned int dust_effect;
    unsigned int dirt_effect;
    unsigned int emitter;
    MkPfx* particle;
    MkObj* object;
    int i;

    dust_effect = fx_by_owner("coffin_dust_pfx", 4);
    fx_reset(dust_effect);
    fx_set(dust_effect, 0x204, 0.0f);
    for (i = 0; i < 5; i++) {
        object = get_mkobj_frame(0x8311, 0);
        if (object != 0) {
            object->pos.value.x = origin->x + object_offsets[i].x;
            object->pos.value.y = origin->y + object_offsets[i].y;
            object->pos.value.z = origin->z + object_offsets[i].z;
            object->ang.y = object_angles[i];
            insert_particle_mkobj(object);
            update_mkobj(as_mkhdr(&object->hdr));
            emitter = fx_next_emitter(dust_effect);
            particle = pfx_from_emitter(emitter);
            if (particle != 0) {
                pfx_bind_emitter_num_to_obj(
                    particle, object, 0, emitter_id_from_handle(emitter));
                fx_restart_emit(emitter);
            }
        }
    }
    dirt_effect = fx_by_owner("dirt_fountain", 4);
    fx_reset(dirt_effect);
    dirt_position.x = origin->x + dust_offset.x;
    dirt_position.y = origin->y + dust_offset.y;
    dirt_position.z = origin->z + dust_offset.z;
    fx_set_param_v3(dirt_effect, 0x202, dirt_position.x, dirt_position.y, dirt_position.z);
    fx_restart_emit(dirt_effect);
}

/* ========================================================================= */
/* Cluster E - input, dialog, and economy                                    */
/* ========================================================================= */

static inline void move_krypt_selection(CameraPdata* camera, int direction) {
    if (camera == 0) {
        return;
    }
    if (direction == 0) {
        if (camera->target_pos.x < 28.5f) {
            camera->target_pos.x += 3.0f;
            krypt_pdata->current_column++;
            krypt_pdata->layout_dirty = 1;
            krypt_pdata->field_0x108 = 0;
        } else {
            krypt_pdata->field_0x108 = 1;
            return;
        }
    } else if (direction == 1) {
        if (camera->target_pos.x > -28.5f) {
            camera->target_pos.x -= 3.0f;
            krypt_pdata->current_column--;
            krypt_pdata->layout_dirty = 1;
            krypt_pdata->field_0x108 = 0;
        } else {
            krypt_pdata->field_0x108 = 1;
            return;
        }
    } else if (direction == 2) {
        if (camera->target_pos.z > -40.0f) {
            camera->target_pos.z -= 5.0f;
            krypt_pdata->current_row++;
            krypt_pdata->layout_dirty = 1;
            krypt_pdata->field_0x108 = 0;
        } else {
            krypt_pdata->field_0x108 = 1;
            return;
        }
    } else {
        if (camera->target_pos.z < 55.0f) {
            camera->target_pos.z += 5.0f;
            krypt_pdata->current_row--;
            krypt_pdata->layout_dirty = 1;
            krypt_pdata->field_0x108 = 0;
        } else {
            krypt_pdata->field_0x108 = 1;
            return;
        }
    }
}

static inline void start_move_footstep(
    int pid, MkProcEntryFn entry, MkHdr** pdata) {
    _create_mkproc_generic_tinystack(pid, 0x1F, entry, 8, pdata);
}

static inline void handle_held_krypt_direction(
    CameraPdata* camera, int* latch, int switch_id, int direction,
    unsigned int* counter, MkHdr** running_pdata, MkHdr** release_pdata) {
    if (check_switch(krypt_pdata->player_port, switch_id) != 0) {
        if (++*counter > 30U) {
            *counter = 25;
            move_krypt_selection(camera, direction);
            *running_pdata = 0;
            if (find_mkproc_pid(0x8243) == 0 &&
                krypt_pdata->field_0x108 == 0) {
                _create_mkproc_generic_tinystack(
                    0x8243, 0x1F, p_running_footstep_proc, 8, running_pdata);
            }
        }
    } else {
        *latch = 0;
        *counter = 0;
        destroy_mkprocs_pid(0x8243);
        *release_pdata = 0;
        if (find_mkproc_pid(0x8242) == 0 &&
            krypt_pdata->field_0x108 == 0) {
            _create_mkproc_generic_tinystack(
                0x8242, 0x1F, p_single_move_footstep_proc, 8, release_pdata);
        }
        krypt_pdata->field_0x108 = 0;
    }
}

static float handle_controller_input(void) {
    static int right_button_down;
    static int left_button_down;
    static int up_button_down;
    static int down_button_down;
    static unsigned int counter;
    CameraPdata* camera;
    CoffinEntry* entry;
    ScreenObj* open_button;
    ScreenObj* exit_button;
    StringObj* text;
    MkFileInfo* section;
    int index;
    int cost;
    int kind;
    int key_bit;
    int available;
    int row;
    int column;
    MkHdr* right_running_pdata;
    MkHdr* left_running_pdata;
    MkHdr* up_running_pdata;
    MkHdr* down_running_pdata;
    MkHdr* right_edge_pdata;
    MkHdr* left_edge_pdata;
    MkHdr* up_edge_pdata;
    MkHdr* down_edge_pdata;
    MkHdr* right_release_pdata;
    MkHdr* left_release_pdata;
    MkHdr* up_release_pdata;
    MkHdr* down_release_pdata;

    camera = get_pdata_of_camera();
    if (camera != 0) {
    if (right_button_down != 0) {
        handle_held_krypt_direction(camera, &right_button_down, 0xD, 0, &counter,
                                    &right_running_pdata, &right_release_pdata);
    } else if (left_button_down != 0) {
        handle_held_krypt_direction(camera, &left_button_down, 0xF, 1, &counter,
                                    &left_running_pdata, &left_release_pdata);
    } else if (up_button_down != 0) {
        handle_held_krypt_direction(camera, &up_button_down, 0xC, 2, &counter,
                                    &up_running_pdata, &up_release_pdata);
    } else if (down_button_down != 0) {
        handle_held_krypt_direction(camera, &down_button_down, 0xE, 3, &counter,
                                    &down_running_pdata, &down_release_pdata);
    } else if (check_switch_edge(krypt_pdata->player_port, 0xD) != 0) {
        right_button_down = 1;
        move_krypt_selection(camera, 0);
        right_edge_pdata = 0;
        start_move_footstep(
            0x8242, p_single_move_footstep_proc, &right_edge_pdata);
    } else if (check_switch_edge(krypt_pdata->player_port, 0xF) != 0) {
        left_button_down = 1;
        move_krypt_selection(camera, 1);
        left_edge_pdata = 0;
        start_move_footstep(
            0x8242, p_single_move_footstep_proc, &left_edge_pdata);
    } else if (check_switch_edge(krypt_pdata->player_port, 0xC) != 0) {
        up_button_down = 1;
        move_krypt_selection(camera, 2);
        up_edge_pdata = 0;
        start_move_footstep(
            0x8242, p_single_move_footstep_proc, &up_edge_pdata);
    } else if (check_switch_edge(krypt_pdata->player_port, 0xE) != 0) {
        down_button_down = 1;
        move_krypt_selection(camera, 3);
        down_edge_pdata = 0;
        start_move_footstep(
            0x8242, p_single_move_footstep_proc, &down_edge_pdata);
    } else if (check_switch_edge(krypt_pdata->player_port, 6) != 0) {
        index = krypt_pdata->current_column + krypt_pdata->current_row * 20;
        entry = &coffin_data[index];
        cost = entry->cost;
        kind = entry->prize_kind;
        if (get_coffin_bit(krypt_pdata->profile_common->coffin_bits, index) != 0) {
            snd_req(0x1AA8);
        } else {
            if ((unsigned int)kind == 6) {
                if (cost > krypt_pdata->konquest_key_max) {
                    key_bit = -1;
                } else {
                    key_bit = krypt_pdata->konquest_key_table[cost] -
                              krypt_pdata->konquest_key_table[0] - 1;
                }
                if (key_bit < 0) {
                    available = 0;
                } else if (get_u8_bit(krypt_pdata->profile_konquest->key_bits,
                                      krypt_pdata->konquest_key_bit_count,
                                      key_bit) != 0) {
                    available = 1;
                } else {
                    available = 0;
                }
            } else {
                if (krypt_pdata->profile_common->koin_totals[kind] >=
                    (unsigned int)cost) {
                    available = 1;
                } else {
                    available = 0;
                }
            }
            if (available != 0) {
                krypt_pdata->award_applied = 0;
                if (do_dialog(0) != 0) {
                    load_ssf(krypt_art_file_table);
                    if (krypt_data_loaded != 0) {
                        index = krypt_pdata->current_column +
                                krypt_pdata->current_row * 20;
                        entry = &coffin_data[index];
                        section = get_mk_file_info_from_current_ssf(entry->gallery_art);
                        unload_section_slot(0x150067);
                        load_art_section_async(0x150067, section);
                    }
                    index = krypt_pdata->current_column +
                            krypt_pdata->current_row * 20;
                    set_coffin_bit(krypt_pdata->profile_common->coffin_bits, index, 1);
                    entry = &coffin_data[index];
                    if ((unsigned int)entry->movie_kind != 0 &&
                        (unsigned int)entry->movie_kind < 20) {
                        mark_as_unlocked((PlayerProfile*)krypt_pdata->player_profile,
                                         entry->movie_kind, entry->unlock_or_movie);
                    }
                    if (deduct_koins(cost, kind) != 0) {
                        mkproc_jump_sleep(p_move_camera_and_open_coffin);
                        return 0.0f;
                    }
                    open_button = krypt_pdata->open_button.obj;
                    if (open_button != 0) {
                        if (open_button->instance !=
                            krypt_pdata->open_button.obj_instance) {
                            open_button = 0;
                        }
                    } else {
                        open_button = 0;
                    }
                    exit_button = krypt_pdata->exit_button.obj;
                    if (exit_button != 0) {
                        if (exit_button->instance !=
                            krypt_pdata->exit_button.obj_instance) {
                            exit_button = 0;
                        }
                    } else {
                        exit_button = 0;
                    }
                    if (krypt_pdata->award_applied == 0) {
                        index = krypt_pdata->current_column +
                                krypt_pdata->current_row * 20;
                        set_coffin_bit(krypt_pdata->profile_common->coffin_bits, index, 0);
                        entry = &coffin_data[index];
                        if ((unsigned int)entry->movie_kind != 0 &&
                            (unsigned int)entry->movie_kind < 20) {
                            mark_as_locked((PlayerProfile*)krypt_pdata->player_profile,
                                           entry->movie_kind, entry->unlock_or_movie);
                        }
                    }
                    snd_req(0x1AA4);
                    if (open_button != 0) unhide_screen_obj(open_button);
                    if (exit_button != 0) unhide_screen_obj(exit_button);
                    return 0.0f;
                }
            } else {
                if (kind == 6) {
                    do_dialog(2);
                } else {
                    do_dialog(1);
                }
            }
        }
    } else if (check_switch_edge(krypt_pdata->player_port, 7) != 0) {
        for (index = 0; index < 6; index++) {
            TrackedSound* sound = krypt_pdata->fire_pot_sounds[index];
            if (sound != 0) {
                if (sound->hdr.instance != 0) {
                    sound->hdr.typed_vtbl->destroy(&sound->hdr);
                }
                krypt_pdata->fire_pot_sounds[index] = 0;
            }
        }
        stop_sound_tracking_process(&krypt_pdata->tracked_sound_list);
        if (prize_description_block != 0) free_mem(prize_description_block);
        if (coffin_data != 0) {
            free_mem(coffin_data);
            coffin_data = 0;
        }
        fade_to_black(10, 1);
        gamelogic_jump(6, p_main_menu);
    }

    if (check_switch(krypt_pdata->player_port, 2) != 0) {
        if (krypt_pdata->wallet_open == 0) {
            krypt_pdata->wallet_open = 1;
            snd_req(0x3B8);
        }
        open_button = krypt_pdata->wallet_back.obj;
        if (open_button != 0) {
            if (open_button->instance != krypt_pdata->wallet_back.obj_instance) {
                open_button = 0;
            }
        } else {
            open_button = 0;
        }
        exit_button = krypt_pdata->wallet_front.obj;
        if (exit_button != 0) {
            if (exit_button->instance != krypt_pdata->wallet_front.obj_instance) {
                exit_button = 0;
            }
        } else {
            exit_button = 0;
        }
        if (open_button != 0 && exit_button != 0) {
            if (open_button->y + 6 < 0) {
                open_button->y += 6;
                exit_button->y += 6;
            } else {
                open_button->y = 0;
                exit_button->y = 0;
            }
        }
        for (index = 0; index < 6; index++) {
            text = krypt_pdata->wallet_text[index].obj;
            if (text != 0) {
                if (text->instance != krypt_pdata->wallet_text[index].obj_instance) {
                    text = 0;
                }
            } else {
                text = 0;
            }
            if (text != 0) {
                if (text->y < 34) {
                    text->y += 6;
                    text->render_y += 6;
                } else text->y = 34;
            }
        }
    } else if (krypt_pdata->wallet_open_ticks != 0) {
        krypt_pdata->wallet_open_ticks--;
    } else {
        if (krypt_pdata->wallet_open != 0) {
            krypt_pdata->wallet_open = 0;
            snd_req(0x3B9);
        }
        open_button = krypt_pdata->wallet_back.obj;
        if (open_button != 0) {
            if (open_button->instance != krypt_pdata->wallet_back.obj_instance) {
                open_button = 0;
            }
        } else {
            open_button = 0;
        }
        exit_button = krypt_pdata->wallet_front.obj;
        if (exit_button != 0) {
            if (exit_button->instance != krypt_pdata->wallet_front.obj_instance) {
                exit_button = 0;
            }
        } else {
            exit_button = 0;
        }
        if (open_button != 0 && exit_button != 0) {
            if (open_button->y - 6 > -100) {
                open_button->y -= 6;
                exit_button->y -= 6;
            } else {
                open_button->y = -100;
                exit_button->y = -100;
            }
        }
        for (index = 0; index < 6; index++) {
            text = krypt_pdata->wallet_text[index].obj;
            if (text != 0) {
                if (text->instance != krypt_pdata->wallet_text[index].obj_instance) {
                    text = 0;
                }
            } else {
                text = 0;
            }
            if (text != 0) {
                if (text->y > -66) {
                    text->y -= 6;
                    text->render_y -= 6;
                } else text->y = -66;
            }
        }
    }
    if (check_switch_edge(krypt_pdata->player_port, 1) != 0 &&
        krypt_pdata->available_key_coffin != 0) {
        row = krypt_pdata->available_key_coffin / 20;
        column = krypt_pdata->available_key_coffin % 20;
        if (row != krypt_pdata->current_row || column != krypt_pdata->current_column) {
            snd_req(0x3C1);
            camera->target_pos.x = 3.0f * column - 28.5f;
            camera->target_pos.y = 4.4f;
            camera->target_pos.z = 55.0f - 5.0f * row;
            krypt_pdata->current_row = row;
            krypt_pdata->current_column = column;
            krypt_pdata->layout_dirty = 1;
        }
    }
    }
    return 1.0f;
}
static int do_dialog(int dialog_type) {
    ScreenObj* dialog_left;
    ScreenObj* dialog_right;
    ScreenObj* dialog_bottom;
    ScreenObj* yes_button;
    ScreenObj* no_button;
    ScreenObj* ok_button;
    ScreenObj* hud_icon_a;
    ScreenObj* hud_icon_b;
    StringObj* dialog_text;
    const char* message;
    int result;

    dialog_bottom = 0;
    yes_button = 0;
    no_button = 0;
    ok_button = 0;
    hud_icon_a = krypt_pdata->open_button.obj;
    if (hud_icon_a != 0) {
        if (hud_icon_a->instance != krypt_pdata->open_button.obj_instance) hud_icon_a = 0;
    } else {
        hud_icon_a = 0;
    }
    hud_icon_b = krypt_pdata->exit_button.obj;
    if (hud_icon_b != 0) {
        if (hud_icon_b->instance != krypt_pdata->exit_button.obj_instance) hud_icon_b = 0;
    } else {
        hud_icon_b = 0;
    }
    message = 0;
    dialog_left = load_named_2d_pfxobj(0x140066, 0x830F, "DIALOG_LEFT", 0, 0x4C);
    dialog_left->x = screen_width / 2 - 0xC0;
    dialog_left->y = screen_height / 2 + 0x32;
    dialog_right = load_named_2d_pfxobj(0x140066, 0x830F, "DIALOG_RIGHT", 0, 0x4C);
    dialog_right->x = dialog_left->x + 0x100;
    dialog_right->y = dialog_left->y;
    eat_switch_edge(krypt_pdata->player_port, 6);
    eat_switch_edge(krypt_pdata->player_port, 0xB);
    force_wallet_to_open_position();

    switch (dialog_type) {
    case 0:
        dialog_bottom = load_named_2d_pfxobj(
            0x140066, 0x830F, "DIALOG_BOTTOM", 0, 0x4C);
        dialog_bottom->x = dialog_left->x;
        dialog_bottom->y = dialog_left->y - 7;
        yes_button = load_named_2d_pfxobj(0x140066, 0x830F, "YES_BUTTON", 0, 0x4B);
        yes_button->x = dialog_left->x + 0x163 - yes_button->pfx2d->tex_w;
        yes_button->y = dialog_left->y - 3;
        no_button = load_named_2d_pfxobj(0x140066, 0x830F, "NO_BUTTON", 0, 0x4B);
        no_button->x = dialog_left->x + 0x1C;
        no_button->y = dialog_left->y - 3;
        message = get_string_by_id(0x20018);
        snd_req(0x1AA4);
        break;
    case 1:
        ok_button = load_named_2d_pfxobj(0x140066, 0x830F, "OK_BUTTON", 0, 0x4B);
        if (ok_button != 0) {
            ok_button->x = dialog_left->x + 0x163 - ok_button->pfx2d->tex_w;
            ok_button->y = dialog_left->y - 3;
        }
        message = get_string_by_id(0x20019);
        snd_req(0x1AA8);
        break;
    case 2:
        ok_button = load_named_2d_pfxobj(0x140066, 0x830F, "OK_BUTTON", 0, 0x4B);
        if (ok_button != 0) {
            ok_button->x = dialog_left->x + 0x163 - ok_button->pfx2d->tex_w;
            ok_button->y = dialog_left->y - 3;
        }
        message = get_string_by_id(0x2001A);
        snd_req(0x1AA8);
        break;
    }
    dialog_text = create_wrapped_string(
        0x8310, load_font(9), message, screen_width / 2 - 0x96,
        dialog_left->y + 0x5A, 0x12C, 0, 1, 0);
    if (dialog_text != 0) {
        dialog_text->priority = 0x4B;
        insert_2d_obj((ScreenObj*)dialog_text);
    }
    hide_screen_obj(hud_icon_a);
    hide_screen_obj(hud_icon_b);

    for (;;) {
        if (check_switch_edge(krypt_pdata->player_port, 6) != 0) {
            snd_req(0x1AA5);
            if (dialog_type != 0) {
                unhide_screen_obj(hud_icon_a);
                unhide_screen_obj(hud_icon_b);
            }
            result = 1;
            eat_switch_edge(krypt_pdata->player_port, 6);
            eat_switch_edge(krypt_pdata->player_port, 0xB);
            break;
        }
        if (check_switch_edge(krypt_pdata->player_port, 7) != 0) {
            if (dialog_type == 0) {
                result = 0;
                unhide_screen_obj(hud_icon_a);
                unhide_screen_obj(hud_icon_b);
                break;
            }
            eat_switch_edge(krypt_pdata->player_port, 5);
            eat_switch_edge(krypt_pdata->player_port, 7);
            eat_switch_edge(krypt_pdata->player_port, 4);
            eat_switch_edge(krypt_pdata->player_port, 8);
        }
        _mkproc_sleep_ticks = 1.0f;
        mkproc_sleep();
    }

    if (dialog_left->instance != 0) dialog_left->typed_vtbl->destroy(dialog_left);
    if (dialog_right->instance != 0) dialog_right->typed_vtbl->destroy(dialog_right);
    if (dialog_bottom != 0 && dialog_bottom->instance != 0)
        dialog_bottom->typed_vtbl->destroy(dialog_bottom);
    if (dialog_text->instance != 0) dialog_text->typed_vtbl->destroy(dialog_text);
    if (yes_button != 0 && yes_button->instance != 0)
        yes_button->typed_vtbl->destroy(yes_button);
    if (no_button != 0 && no_button->instance != 0)
        no_button->typed_vtbl->destroy(no_button);
    if (ok_button != 0 && ok_button->instance != 0)
        ok_button->typed_vtbl->destroy(ok_button);
    return result;
}
static float p_running_footstep_proc(void) {
    MkHdr* footstep_pdata;

    for (;;) {
        if (krypt_pdata->field_0x108 != 0) {
            break;
        }
        random_snd_req(0x2A);
        _mkproc_sleep_ticks = 14.0f;
        mkproc_sleep();
    }

    if (find_mkproc_pid(0x8242) == 0) {
        footstep_pdata = 0;
        _create_mkproc_generic_tinystack(
            0x8242, 0x1F, p_single_move_footstep_proc, 8,
            &footstep_pdata);
    }
    return -1.0f;
}
static float p_single_move_footstep_proc(void) {
    random_snd_req(0x2B);
    _mkproc_sleep_ticks = 19.0f;
    mkproc_sleep();
    random_snd_req(0x2B);
    _mkproc_sleep_ticks = 17.0f;
    mkproc_sleep();
    snd_req(0x3CC);
    return -1.0f;
}
static int deduct_koins(int amount, unsigned int koin_type) {
    ProfileCommon* profile;
    StringObj* wallet_text;
    int old_total;
    int key_bit;

    if (koin_type == 6) {
        if (amount > krypt_pdata->konquest_key_max) {
            key_bit = -1;
        } else {
            key_bit = krypt_pdata->konquest_key_table[amount] -
                      krypt_pdata->konquest_key_table[0] - 1;
        }
        if (key_bit < 0) {
            return 0;
        }
        set_u8_bit(krypt_pdata->profile_konquest->key_bits,
                   krypt_pdata->konquest_key_bit_count, key_bit, 0);
        if (save_profile(menu_player, 2) == 0) {
            set_u8_bit(krypt_pdata->profile_konquest->key_bits,
                       krypt_pdata->konquest_key_bit_count, key_bit, 1);
            return 0;
        }
        update_use_key_string();
        snd_req(0x3C2);
        _mkproc_sleep_ticks = 60.0f;
        mkproc_sleep();
        return 1;
    }

    profile = krypt_pdata->profile_common;
    old_total = profile->koin_totals[koin_type];
    profile->koin_totals[koin_type] = old_total - amount;
    profile->koin_spent[koin_type] += amount;
    if (save_profile(menu_player, 2) == 0) {
        profile->koin_totals[koin_type] += amount;
        profile->koin_spent[koin_type] -= amount;
        return 0;
    }

    _create_mkproc_generic_tinystack(
        0x8246, 0x1F, p_counting_sound, 8, &empty_pdata);
    wallet_text = krypt_pdata->wallet_text[koin_type].obj;
    if (wallet_text != 0) {
        if (wallet_text->instance != krypt_pdata->wallet_text[koin_type].obj_instance) {
            wallet_text = 0;
        }
    } else {
        wallet_text = 0;
    }
    display_numerical_change(wallet_text, 0, old_total, -amount, 1, 0x14);
    destroy_mkprocs_pid(0x8246);
    _mkproc_sleep_ticks = 30.0f;
    mkproc_sleep();
    return 1;
}
static float p_counting_sound(void) {
    static int counter;

    if (counter <= 0) {
        counter = 5;
        snd_req(0x3BA);
    } else {
        counter--;
    }

    return 1.0f;
}

/* ========================================================================= */
/* Coffin-grid pebble layout                                                 */
/* ========================================================================= */

void set_pebble_positions_for_row(int row, int start_col, int count, const Vec* origin) {
    Vec lid_offset = {0.0f, 0.0f, 1.75f};
    int i;
    int col;
    int pattern;
    PebbleData* pebble;
    RwV3d* position;
    RwV3d* lid_position;
    int lid_count;

    col = start_col;
    pebble = 0;
    position = 0;
    i = 0;
    while (i < count) {
        pattern = tombstone_pattern[row % 4][col % 4];
        switch (pattern) {
        case 0:
            pebble = krypt_pdata->coffin_pebble_type0;
            break;
        case 1:
            pebble = krypt_pdata->coffin_pebble_type1;
            break;
        case 2:
            pebble = krypt_pdata->coffin_pebble_type2;
            break;
        case 3:
            pebble = krypt_pdata->coffin_pebble_type3;
            break;
        }

        if (pebble != 0) {
            position = &pebble->pebbles[pebble->count].matrix.pos;
            if (position == 0) {
                return;
            }
            position->x = 3.0f * (float)i + origin->x;
            position->y = 0.0f;
            position->z = origin->z;
            pebble->count = pebble->count + 1;
        }

        if (get_coffin_bit(krypt_pdata->profile_common->coffin_bits, col + row * 0x14) != 0) {
            pebble = krypt_pdata->lid_open_pebbles;
        } else {
            pebble = krypt_pdata->lid_closed_pebbles;
        }

        if (pebble != 0) {
            lid_count = pebble->count;
            lid_position = &pebble->pebbles[lid_count].matrix.pos;
            lid_position->x = position->x + lid_offset.x;
            lid_position->y = position->y + lid_offset.y;
            lid_position->z = position->z + lid_offset.z;
            /* Retail then forces Y to 0. */
            lid_position->y = 0.0f;
            pebble->count = lid_count + 1;
        }
        col++;
        i++;
    }
}

static inline void sobj_enable_pebble_bit(MkSobj* sobj) {
    sobj->flags09_bits.bit4 = 1;
}

void setup_tombstones(void) {
    MkSobj* sobj;
    MkSobj* sobj_b;
    PebbleData* pebble;
    int i;
    int col;
    int row;

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x15);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata(sobj, 5, 0);
        krypt_pdata->pebble_wall_a = pebble;
        for (i = 0; i < 5; i++) {
            krypt_pdata->pebble_wall_a->pebbles[i].matrix.pos.z =
                -(5.0f * (4.0f * (float)i) - 50.0f);
        }
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x16);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata(sobj, 5, 0);
        krypt_pdata->pebble_wall_b = pebble;
        for (i = 0; i < 5; i++) {
            krypt_pdata->pebble_wall_b->pebbles[i].matrix.pos.z =
                -(5.0f * (4.0f * (float)i) - 45.0f);
        }
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x17);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata(sobj, 5, 0);
        krypt_pdata->pebble_wall_c = pebble;
        for (i = 0; i < 5; i++) {
            krypt_pdata->pebble_wall_c->pebbles[i].matrix.pos.z =
                -(5.0f * (4.0f * (float)i) - 40.0f);
        }
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x18);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        pebble = create_pebble_userdata(sobj, 5, 0);
        krypt_pdata->pebble_wall_d = pebble;
        for (i = 0; i < 5; i++) {
            krypt_pdata->pebble_wall_d->pebbles[i].matrix.pos.z =
                -(5.0f * (4.0f * (float)i) - 35.0f);
        }
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 9);
    if (sobj != 0) {
        sobj_b = obj_find_sobj_by_id(g_game_info.bgnd_obj, 10);
        if (sobj_b != 0) {
        sobj_enable_pebble_bit(sobj_b);
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->pebble_grid_a = create_pebble_userdata(sobj, 0xc, 0);
        krypt_pdata->pebble_grid_b = create_pebble_userdata(sobj_b, 0xc, 0);
        for (i = 0; i < 0xc; i++) {
            col = i % 3;
            row = i / 3;
            krypt_pdata->pebble_grid_a->pebbles[i].matrix.pos.x =
                3.0f * (float)((col + 1) * 5 - 1) + -28.5f;
            krypt_pdata->pebble_grid_a->pebbles[i].matrix.pos.y = 0.0f;
            krypt_pdata->pebble_grid_a->pebbles[i].matrix.pos.z =
                -(5.0f * (float)((row + 1) * 4 - 1) - 50.0f);
            krypt_pdata->pebble_grid_b->pebbles[i].matrix.pos.x =
                krypt_pdata->pebble_grid_a->pebbles[i].matrix.pos.x;
            krypt_pdata->pebble_grid_b->pebbles[i].matrix.pos.y =
                krypt_pdata->pebble_grid_a->pebbles[i].matrix.pos.y;
            krypt_pdata->pebble_grid_b->pebbles[i].matrix.pos.z =
                krypt_pdata->pebble_grid_a->pebbles[i].matrix.pos.z;
        }
        }
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 1);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type0 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 3);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type1 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 5);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type2 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 7);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->coffin_pebble_type3 = create_pebble_userdata((MkSobj*)sobj, 10, 0);
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x3c);
    if (sobj != 0) {
        sobj->flags_08_bits.bit6 = 1;
        sobj->flags_08_bits.bit5 = 1;
    }
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x3d);
    if (sobj != 0) {
        sobj->flags_08_bits.angular_velocity_enabled = 1;
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x5a);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->lid_closed_pebbles = create_pebble_userdata((MkSobj*)sobj, 0x24, 0);
    }
    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x5b);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->lid_open_pebbles = create_pebble_userdata((MkSobj*)sobj, 0x24, 0);
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x32);
    if (sobj != 0) {
        sobj_enable_pebble_bit(sobj);
        krypt_pdata->fire_pot_pebbles = create_pebble_userdata((MkSobj*)sobj, 6, 0);
        position_fire_pots();
    }

    sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, 0x46);
    if (sobj != 0) {
        MkObjLatch* fog_pdata;

        sobj_set_priority(sobj, 0x14);
        sobj->flags_08_bits.bit6 = 1;
        fog_pdata = 0;
        if (_create_mkproc_generic_tinystack(
                0x823a, 0x1f, (MkProcEntryFn)p_fog_follow_camera, 0x10,
                (MkHdr**)&fog_pdata) != 0 && fog_pdata != 0) {
            fog_pdata->obj = (MkHdr*)sobj;
            fog_pdata->obj_instance = ((MkHdr*)sobj)->instance;
        }
    }

    for (i = 0; i < 5; i++) {
        sobj = obj_find_sobj_by_id(g_game_info.bgnd_obj, i + 0x1e);
        if (sobj != 0) {
            sobj->flags_08_bits.bit6 = 1;
            sobj->flags_08_bits.bit3 = 1;
            hide_sobj(sobj);
        }
    }
}

static void position_fire_pots(void) {
    PebbleData* pots;
    int i;

    pots = krypt_pdata->fire_pot_pebbles;
    pots->pebbles[0].matrix.pos.x = -9.069f;
    pots->pebbles[0].matrix.pos.y = 0.0f;
    pots->pebbles[0].matrix.pos.z = -51.581f;
    pots->pebbles[1].matrix.pos.x = 9.3f;
    pots->pebbles[1].matrix.pos.y = 0.0f;
    pots->pebbles[1].matrix.pos.z = -51.581f;
    pots->pebbles[2].matrix.pos.x = -32.944f;
    pots->pebbles[2].matrix.pos.y = 0.0f;
    pots->pebbles[2].matrix.pos.z = 7.598f;
    pots->pebbles[3].matrix.pos.x = -33.134f;
    pots->pebbles[3].matrix.pos.y = 0.0f;
    pots->pebbles[3].matrix.pos.z = 21.839f;
    pots->pebbles[4].matrix.pos.x = 32.963f;
    pots->pebbles[4].matrix.pos.y = 0.0f;
    pots->pebbles[4].matrix.pos.z = 7.355f;
    pots->pebbles[5].matrix.pos.x = 33.2f;
    pots->pebbles[5].matrix.pos.y = 0.0f;
    pots->pebbles[5].matrix.pos.z = 22.056f;

    for (i = 0; i < 6; i++) {
        krypt_pdata->fire_pot_sounds[i] = get_sound_tracker_data();
        krypt_pdata->fire_pot_sounds[i]->pos_x = pots->pebbles[i].matrix.pos.x;
        krypt_pdata->fire_pot_sounds[i]->pos_y = pots->pebbles[i].matrix.pos.y;
        krypt_pdata->fire_pot_sounds[i]->pos_z = pots->pebbles[i].matrix.pos.z;
        krypt_pdata->fire_pot_sounds[i]->pos_y = 1.0f;
        krypt_pdata->fire_pot_sounds[i]->sound_id = 0x3C3;
        krypt_pdata->fire_pot_sounds[i]->min_dist = 5.0f;
        krypt_pdata->fire_pot_sounds[i]->max_dist = 20.0f;
        krypt_pdata->fire_pot_sounds[i]->positional_pan = 1;
        make_new_tracked_sound(
            &krypt_pdata->tracked_sound_list, krypt_pdata->fire_pot_sounds[i]);
    }
}

/* ========================================================================= */
/* Cluster F - mode shell (Agent A)                                          */
/* ========================================================================= */

float p_fog_follow_camera(void) {
    CameraObj* camera;
    MkObjLatch* pdata;
    KryptFogObject* fog;

    pdata = (MkObjLatch*)pdata_of_proc(aproc);
    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }

    if (pdata != 0 && camera != 0) {
        fog = (KryptFogObject*)pdata->obj;
        if (fog != 0) {
            if (fog->hdr.instance != pdata->obj_instance) {
                fog = 0;
            }
        } else {
            fog = 0;
        }
        if (fog != 0) {
            fog->camera_z = camera->pos.z;
        }
    }
    return 1.0f;
}
static float p_follow_camera(void) {
    CameraObj* camera;
    KryptCameraFollowPdata* pdata;
    MkObj* obj;

    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }

    pdata = (KryptCameraFollowPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }

    obj = pdata->obj;
    if (obj != 0) {
        if (obj->hdr.instance != pdata->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return -1.0f;
    }

    if (pdata != 0 && obj != 0 && camera != 0) {
        obj->pos.value.x = camera->pos.x;
        obj->pos.value.y = camera->pos.y;
        obj->pos.value.z = camera->pos.z;

        if (krypt_pdata->pfx_koins != 0) {
            krypt_pdata->pfx_koins->mat_e = camera->pos.x;
            krypt_pdata->pfx_koins->mat_f = 2.0f;
            krypt_pdata->pfx_koins->mat_g = camera->pos.z - 3.0f;
        }
        if (krypt_pdata->pfx_numbers != 0) {
            krypt_pdata->pfx_numbers->mat_e = camera->pos.x;
            krypt_pdata->pfx_numbers->mat_f = 2.0f;
            krypt_pdata->pfx_numbers->mat_g = camera->pos.z - 3.0f;
        }
        if (krypt_pdata->pfx_letters != 0) {
            krypt_pdata->pfx_letters->mat_e = camera->pos.x;
            krypt_pdata->pfx_letters->mat_f = 2.0f;
            krypt_pdata->pfx_letters->mat_g = camera->pos.z - 3.0f;
        }
    }
    return 1.0f;
}
int load_pix_section(
    int slot, const CoffinEntry* entries, int index, int enabled, int flags) {
    int gallery_art;
    MkFileInfo* section;

    if (enabled == 0) {
        return 0;
    }

    gallery_art = entries[index].gallery_art;
    section = get_mk_file_info_from_current_ssf(gallery_art);
    if (is_section_loading_or_loaded(slot, section) == 0) {
        unload_section_slot(slot);
    }
    load_art_section(slot, section);
    return ((gallery_art + 0x3EA) << 16) | flags;
}
int load_binary_data(
    MkFileInfo* file_info, CoffinEntry** entries, void* unused,
    int* loaded, int entry_count) {
    unsigned char* strings;
    MkFileEntry* file;
    int strings_size;
    int entries_size;
    int i;

    if (*loaded != 0) {
        return 1;
    }

    file = mk_file_open_language(file_info, "rb", (void*)1);
    if (file == 0) {
        return 0;
    }

    entries_size = entry_count * sizeof(CoffinEntry);
    strings_size = mk_file_length(file) - entries_size;
    strings = get_mem(strings_size);
    if (strings == 0) {
        return 0;
    }

    *entries = get_mem(entries_size);
    if (*entries == 0) {
        return 0;
    }

    mk_file_read(*entries, sizeof(CoffinEntry), entry_count, file);
    mk_file_read(strings, 1, strings_size, file);
    mk_file_close(file);

    for (i = 0; i < entry_count; i++) {
        (*entries)[i].blurb += (unsigned int)strings;
        (*entries)[i].long_description += (unsigned int)strings;
    }

    *loaded = 1;
    return 1;
}
#pragma dont_inline on
static void init_konquest_keys(void) {
    CoffinEntry* entry;
    int key_bit;
    int index;

    krypt_pdata->konquest_key_table = (int*)get_data_table_by_name("konquest_keys");
    if (krypt_pdata->konquest_key_table != 0) {
        krypt_pdata->konquest_key_max = get_row_count_for_table_by_pointer(
            g_game_info.cmdscript, krypt_pdata->konquest_key_table);
        krypt_pdata->konquest_key_bit_count =
            krypt_pdata->konquest_key_table[krypt_pdata->konquest_key_max - 1] -
            krypt_pdata->konquest_key_table[0] - 1;
        krypt_pdata->konquest_key_max -= 2;

        for (index = 1; index < 400; index++) {
            entry = &coffin_data[index];
            if ((unsigned int)entry->prize_kind == 6) {
                if (entry->cost > krypt_pdata->konquest_key_max) {
                    key_bit = -1;
                } else {
                    key_bit = krypt_pdata->konquest_key_table[entry->cost] -
                              krypt_pdata->konquest_key_table[0] - 1;
                }
                if (get_u8_bit(krypt_pdata->profile_konquest->key_bits,
                               krypt_pdata->konquest_key_bit_count, key_bit) != 0 &&
                    get_coffin_bit(krypt_pdata->profile_common->coffin_bits, index) == 0) {
                    break;
                }
            }
        }
        if (index >= 400) {
            index = 0;
        }
        krypt_pdata->available_key_coffin = index;
    }
}
#pragma dont_inline reset
static void update_use_key_string(void) {
    StringObj* use_key_text;
    CoffinEntry* entry;
    const char* use_key_label;
    char key_name[4];
    char text[40];
    int key_bit;
    int index;

    use_key_text = krypt_pdata->use_key_string.obj;
    if (use_key_text != 0) {
        if (use_key_text->instance != krypt_pdata->use_key_string.obj_instance) {
            use_key_text = 0;
        }
    } else {
        use_key_text = 0;
    }

    for (index = 1; index < 400; index++) {
        entry = &coffin_data[index];
        if ((unsigned int)entry->prize_kind == 6) {
            if (entry->cost > krypt_pdata->konquest_key_max) {
                key_bit = -1;
            } else {
                key_bit = krypt_pdata->konquest_key_table[entry->cost] -
                          krypt_pdata->konquest_key_table[0] - 1;
            }
            if (get_u8_bit(krypt_pdata->profile_konquest->key_bits,
                           krypt_pdata->konquest_key_bit_count, key_bit) != 0) {
                if (get_coffin_bit(
                        krypt_pdata->profile_common->coffin_bits, index) == 0) {
                    break;
                }
            }
        }
    }
    if (index >= 400) {
        index = 0;
    }
    krypt_pdata->available_key_coffin = index;
    if (krypt_pdata->available_key_coffin != 0) {
        use_key_label = get_string_by_id(0x20012);
        sprintf(key_name, "%c%c", krypt_pdata->available_key_coffin / 20 + 'A',
                krypt_pdata->available_key_coffin % 20 + 'A');
        sprintf(text, "%s %s", use_key_label, key_name);
        update_string_obj(use_key_text, 0, text);
    }
}

float p_krypt_loop(void) {
    handle_controller_input();
    if (krypt_pdata->layout_dirty != 0) {
        krypt_pdata->tombstone_hud_ticks = 3;
        {
            Vec origin = {0.0f, 0.0f, 0.0f};
            krypt_pdata->coffin_pebble_type0->count = 0;
            krypt_pdata->coffin_pebble_type1->count = 0;
            krypt_pdata->coffin_pebble_type2->count = 0;
            krypt_pdata->coffin_pebble_type3->count = 0;
            krypt_pdata->lid_closed_pebbles->count = 0;
            krypt_pdata->lid_open_pebbles->count = 0;
            place_visible_coffin_rows_from(&origin);
        }
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
    MkObj* light;
    CamVec3 cam_pos = s_cam_pos;
    CamVec3 cam_ang = s_cam_ang;
    MkHdr* random_noise_pdata;
    KryptCharacterMonitorPdata* monitor_pdata;

    start_sound_tracking_process(&krypt_pdata->tracked_sound_list);
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

    setup_sound_banks(4);
    wait_for_sound_banks_to_load();
    start_tunes();
    _create_mkproc_generic_tinystack(
        0x823E, 0x1F, p_play_random_noise, 8, &random_noise_pdata);

    load_string_bank(0x20000, "krypt_strings_eng.mko");
    setup_tombstones();
    active_cmdscript->mko = g_game_info.cmdscript;
    load_ssf(krypt_art_file_table);
    xfer_camera(p_krypt_camera_proc, 1);
    _mkproc_sleep_ticks = 2.0f;
    mkproc_sleep();
    set_camera_position(&cam_pos);
    set_camera_angle(&cam_ang);

    {
        Vec origin = {0.0f, 0.0f, 0.0f};
        krypt_pdata->coffin_pebble_type0->count = 0;
        krypt_pdata->coffin_pebble_type1->count = 0;
        krypt_pdata->coffin_pebble_type2->count = 0;
        krypt_pdata->coffin_pebble_type3->count = 0;
        krypt_pdata->lid_closed_pebbles->count = 0;
        krypt_pdata->lid_open_pebbles->count = 0;
        place_visible_coffin_rows_from(&origin);
    }

    pfx_create_raw_userdata(0, 0, 0x24, 0x202, 2, init_tombstone_koins, 0x823B,
                            update_tombstone_koins, (void**)&krypt_pdata->pfx_koins);
    krypt_pdata->pfx_koins->flag_bits.visible = 1;
    pfx_create_raw_userdata(0, 0, 0x48, 0x202, 2, init_tombstone_letters, 0x823C,
                            update_tombstone_letters, (void**)&krypt_pdata->pfx_letters);
    krypt_pdata->pfx_letters->flag_bits.visible = 1;
    pfx_create_raw_userdata(0, 0, 0x90, 0x202, 2, init_tombstone_numbers, 0x823D,
                            update_tombstone_numbers, (void**)&krypt_pdata->pfx_numbers);
    krypt_pdata->pfx_numbers->flag_bits.visible = 1;

    krypt_pdata->footstep_frame_index = 0;
    light = load_light((LightDef*)&camera_point_light, (MkPtr**)&point_light_list, 0);
    if (light != 0) {
        light->flags_08_bits.airborne = 1;
    }

    bgnd_anim_camera_setup();
    cam_set_intro_cam_pause_ticks(60.0f);
    camera_init_animation(*(void**)((char*)bgnd_animations + 0x78), 0);
    camera_run_animation(0);
    turn_camera_on();
    fade_from_black(0x14, 0);
    camera_wait_for_animation_completion();
    bgnd_anim_camera_ended();

    turn_controllers_on();
    init_konquest_keys();
    init_heads_up_display();
    monitor_pdata = 0;
    if (_create_mkproc_generic_bigstack(
            0x8241, 0x1F, p_monitor_krypt_characters,
            sizeof(KryptCharacterMonitorPdata), (MkHdr**)&monitor_pdata) != 0 &&
        monitor_pdata != 0) {
        monitor_pdata->elapsed_ticks = 0;
        monitor_pdata->delay_ticks = 0;
    }

    mkproc_jump_sleep(p_krypt_loop);
    return 0.0f;
}

float p_init_krypt_mode(void) {
    RwResourcesSetArenaSize(0x100000);
    zero_pdata_payload(0x150, (MkHdr*)krypt_pdata);
    if (menu_player == 0) {
        krypt_pdata->player_port = g_game_info.plyr0.pad_index;
        set_player_state(&g_game_info.plyr0, 2);
        krypt_pdata->player_profile = &p1_profile;
        krypt_pdata->profile_common = p1_profile_common;
        krypt_pdata->profile_konquest = p1_profile_konquest;
    } else {
        krypt_pdata->player_port = g_game_info.plyr1.pad_index;
        set_player_state(&g_game_info.plyr1, 2);
        move_profile_p1_to_p2();
        krypt_pdata->player_profile = &p2_profile;
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
    MkProc* proc;

    set_section_memory_scheme(7);
    proc = _create_mkproc_generic_bigstack(
        0x2001, 0x23, p_init_krypt_mode, 0x150,
        (MkHdr**)&krypt_pdata);
    if (proc != 0) {
        set_process_as_scriptable(proc);
        return -1.0f;
    }
    gamelogic_jump(0, p_atm_loop);
    return -1.0f;
}
