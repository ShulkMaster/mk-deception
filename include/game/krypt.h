#ifndef GAME_KRYPT_H
#define GAME_KRYPT_H

#include "runtime/mk_pebble.h"
#include "runtime/anim_pdata.h"
#include "runtime/image.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_struct.h"
#include "runtime/sound_tracker.h"
#include "libmkparticle/color.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GQNE5D-derived Krypt/Kontent layouts and gameplay interfaces. */

/* Per-coffin record in the kryptdata binary (stride 0x28, indices 0..0x1B7). */
typedef struct CoffinEntry {
    int coffin_type; /* +0x00 - 4 = near-z tombstone */
    int cost;        /* +0x04 - koin price digits */
    int prize_kind;  /* +0x08 - 6 = open / wallet prize */
    int gallery_art; /* +0x0C - full-size section/member id */
    int kontent_type; /* +0x10 - gallery filter / movie classification */
    char* blurb; /* +0x14 - get_coffin_blurb */
    int award_sound_index; /* +0x18 */
    int movie_kind; /* +0x1C - also koin type + 100 for currency awards */
    int unlock_or_movie; /* +0x20 - unlock/movie id or currency award amount */
    char* long_description; /* +0x24 - get_long_coffin_description */
} CoffinEntry; /* 0x28 */

typedef struct KryptScreenObjLatch {
    ScreenObj* obj;
    unsigned int obj_instance;
} KryptScreenObjLatch; /* 0x08 */

typedef struct KryptStringObjLatch {
    StringObj* obj;
    unsigned int obj_instance;
} KryptStringObjLatch; /* 0x08 */

/* Profile common blob: coffin open bitset @ +0xB0 (get_coffin_bit). */
typedef struct ProfileCommon {
    char pad00[0x38];
    unsigned int koin_totals[6]; /* +0x38 */
    unsigned int koin_earned[6]; /* +0x50 */
    unsigned int koin_spent[6]; /* +0x68 */
    char pad80[0x30];
    unsigned char coffin_bits[1]; /* opaque bitset; address passed to get_coffin_bit */
} ProfileCommon;

typedef struct KryptProfileKonquest {
    unsigned char pad00[0x291];
    unsigned char key_bits[1]; /* +0x291 */
} KryptProfileKonquest;

typedef struct TombstonePfxFlags150 {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char pad : 3;
} TombstonePfxFlags150;

typedef struct KryptPfxFlags {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char visible : 1;
    unsigned char pad : 4;
} KryptPfxFlags;

/*
 * Tombstone particle VM (MkPfx.matrix / +0x40 region used as pfx base).
 * Offsets relative to the pointer passed to init_tombstone_* / set_*_positions.
 */
typedef struct TombstonePfx {
    MkHdr hdr;
    union {
        unsigned char flags;
        KryptPfxFlags flag_bits;
    };
    char pad09[3];
    char pad0C[0x44];
    int capacity; /* +0x50 */
    int count;    /* +0x54 */
    char pad58[0xF8];
    union {
        unsigned char flags_150; /* +0x150 */
        TombstonePfxFlags150 flags_150_bits;
    };
    char pad151[3];
    float uv_scale; /* +0x154 */
    float uv_0;     /* +0x158 */
    float uv_rate;  /* +0x15C */
    PfxColor rgba_160; /* +0x160 */
    char pad164[0x1E];
    short anim_frame; /* +0x182 */
    char pad184[0x10];
    float mat_a; /* +0x194 */
    float mat_b; /* +0x198 */
    float mat_c; /* +0x19C */
    float mat_d; /* +0x1A0 */
    float mat_e; /* +0x1A4 */
    float mat_f; /* +0x1A8 */
    float mat_g; /* +0x1AC */
    char pad1B0[4];
    PfxColor rgba_1B4; /* +0x1B4 */
    float scale;      /* +0x1B8 */
} TombstonePfx;

/*
 * Main krypt mode payload (size 0x150; zeroed in p_init_krypt_mode).
 * Only fields touched by setup / tombstone / getters are named.
 */
typedef struct KryptPdata {
    unsigned char hdr[0x08]; /* +0x00 */
    int player_port;         /* +0x08 */
    PebbleData* pebble_wall_a;   /* +0x0C - sobj id 0x15, 5 pebbles */
    PebbleData* pebble_wall_b;   /* +0x10 - sobj id 0x16 */
    PebbleData* pebble_wall_c;   /* +0x14 - sobj id 0x17 */
    PebbleData* pebble_wall_d;   /* +0x18 - sobj id 0x18 */
    PebbleData* coffin_pebble_type0; /* +0x1C - sobj 1 - pattern type 0 */
    PebbleData* coffin_pebble_type1; /* +0x20 - sobj 3 - pattern type 1 */
    PebbleData* coffin_pebble_type2; /* +0x24 - sobj 5 - pattern type 2 */
    PebbleData* coffin_pebble_type3; /* +0x28 - sobj 7 - pattern type 3 */
    PebbleData* pebble_grid_a;       /* +0x2C - sobj 9, 12 pebbles */
    PebbleData* pebble_grid_b;       /* +0x30 - sobj 10 */
    PebbleData* lid_closed_pebbles;  /* +0x34 - sobj 0x5A, 0x24 */
    PebbleData* lid_open_pebbles;    /* +0x38 - sobj 0x5B, 0x24 */
    PebbleData* fire_pot_pebbles;    /* +0x3C - sobj 0x32, 6 pots */
    KryptScreenObjLatch wallet_back;  /* +0x40 */
    KryptScreenObjLatch wallet_front; /* +0x48 */
    int layout_dirty;  /* +0x50 - p_krypt_loop refreshes pebbles */
    int current_row;   /* +0x54 */
    int current_column; /* +0x58 */
    ScreenObj* award_image_left;  /* +0x5C */
    ScreenObj* award_image_right; /* +0x60 */
    KryptScreenObjLatch award_notice_top;    /* +0x64 */
    KryptScreenObjLatch award_notice_left;   /* +0x6C */
    KryptScreenObjLatch award_notice_right;  /* +0x74 */
    KryptScreenObjLatch award_notice_bottom; /* +0x7C */
    KryptScreenObjLatch award_frame;         /* +0x84 */
    KryptScreenObjLatch open_button; /* +0x8C */
    KryptScreenObjLatch exit_button; /* +0x94 */
    KryptStringObjLatch wallet_text[6]; /* +0x9C */
    KryptStringObjLatch hud_label_l; /* +0xCC */
    KryptStringObjLatch hud_label_r; /* +0xD4 */
    KryptStringObjLatch hud_string_20011; /* +0xDC */
    KryptStringObjLatch use_key_string; /* +0xE4 */
    int tombstone_hud_ticks; /* +0xEC - countdown; update_* refresh while >0 */
    TombstonePfx* pfx_koins;   /* +0xF0 */
    TombstonePfx* pfx_letters; /* +0xF4 */
    TombstonePfx* pfx_numbers; /* +0xF8 */
    int award_applied; /* +0xFC */
    AnimPdata* anim_pdata; /* +0x100 - get_krypt_anim_pdata */
    MkProc* anim_proc; /* +0x104 */
    int field_0x108;
    int footstep_frame_index; /* +0x10C - reset by animation script setters */
    int wallet_open_ticks; /* +0x110 */
    int wallet_open;       /* +0x114 */
    int* konquest_key_table; /* +0x118 */
    int konquest_key_max;    /* +0x11C */
    int available_key_coffin; /* +0x120 */
    int konquest_key_bit_count; /* +0x124 */
    MkPtr* tracked_sound_list;        /* +0x128 - fire-pot sound list head */
    TrackedSound* fire_pot_sounds[6]; /* +0x12C..+0x140 */
    void* player_profile;             /* +0x144 */
    ProfileCommon* profile_common;    /* +0x148 */
    KryptProfileKonquest* profile_konquest; /* +0x14C */
} KryptPdata;

/* Mode shell */
float p_krypt_mode(void);
float p_init_krypt_mode(void);
float p_setup_krypt(void);
float p_krypt_loop(void);

/* Kontent gallery */
void hide_kontent_bio_text(void);
void unhide_kontent_bio_text(void);
void kill_kontent_bio_text(void);
void get_gallery_page_number_string(char* out);
char* get_long_coffin_description(void);
char* get_coffin_blurb(void);
void create_fullscreen_gallery_image_list(int* out, int count);
void start_loading_kontent_image(void);
void kontent_set_current_selection(int selection);
void create_gallery_image_list(int* out, int count);
int get_number_kontent_items(void);
float p_kontent_setup(void);
float p_kontent(void);

/* Tombstone / coffin grid */
void setup_tombstones(void);
int load_pix_section(
    int slot, const CoffinEntry* entries, int index, int enabled, int flags);
int load_binary_data(
    MkFileInfo* file_info, CoffinEntry** entries, void* unused,
    int* loaded, int entry_count);
void display_prize_description(CoffinEntry* entries, int index, int last_index, int available,
                               unsigned int string_bank, int priority);
void move_picture_to_camera(KryptScreenObjLatch* picture_latch);
void remove_prize_description(void);
MkObj* load_krypt_character(char* character_name);
void set_pebble_positions_for_row(int row, int start_col, int count, const Vec* origin);
void init_tombstone_letters(void* pfx);
void init_tombstone_numbers(void* pfx);
void init_tombstone_koins(void* pfx);
float update_tombstone_letters(void);
float update_tombstone_numbers(void);
float update_tombstone_koins(void);

/* Thin showcase getters (Cluster B; Agent B may deepen) */
void* get_krypt_anim_pdata(void);
void* get_krypt_character_obj(void);
int get_krypt_current_column(void);
int get_krypt_current_row(void);
void transition_to_krypt_character_anim_script(int animation_id, int flags, int step);
void set_krypt_character_anim_script(
    int animation_id, int flags, void* script_args, float step);
void set_krypt_character_previous_root_angle(void* script_args, float angle);
void set_krypt_character_angle(void* script_args, float angle);
void set_krypt_character_pos(const Vec* position);

/* Sbss globals */
extern int krypt_data_loaded;
extern int gallery_data_loaded;
typedef struct KontentPdata KontentPdata;
extern KontentPdata* kontent_pdata;
extern void* prize_description_block;
extern CoffinEntry* coffin_data;
extern KryptPdata* krypt_pdata;

#ifdef __cplusplus
}
#endif

#endif /* GAME_KRYPT_H */
