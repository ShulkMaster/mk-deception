#ifndef GAME_KRYPT_H
#define GAME_KRYPT_H

#include "runtime/mk_pebble.h"
#include "runtime/mk_struct.h"
#include "runtime/sound_tracker.h"
#include "libmkparticle/color.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * krypt.o - display-only MVP NonMatching scaffold (Wave 3).
 * Retail entry: call p_krypt_mode (menu / attract navigation separate).
 * Full-mode input/open-FX clusters remain partial. The Kontent gallery entry,
 * bio text lifecycle, selection, and description getters are lifted.
 *
 * Layouts below are GQNE5D-derived (ASM / Ghidra), not PS2.
 */

/* Per-coffin record in the kryptdata binary (stride 0x28, indices 0..0x1B7). */
typedef struct CoffinEntry {
    int coffin_type; /* +0x00 - 4 = near-z tombstone */
    int cost;        /* +0x04 - koin price digits */
    int prize_kind;  /* +0x08 - 6 = open / wallet prize */
    int gallery_art; /* +0x0C - full-size section/member id */
    int kontent_type; /* +0x10 - gallery filter / movie classification */
    char* blurb; /* +0x14 - get_coffin_blurb */
    int pad18;
    int movie_kind; /* +0x1C - 0x14 when +0x20 is a movie id */
    int unlock_or_movie; /* +0x20 - bio unlock bit or movie id */
    char* long_description; /* +0x24 - get_long_coffin_description */
} CoffinEntry; /* 0x28 */

/* Profile common blob: coffin open bitset @ +0xB0 (get_coffin_bit). */
typedef struct ProfileCommon {
    char pad00[0xB0];
    unsigned char coffin_bits[1]; /* opaque bitset; address passed to get_coffin_bit */
} ProfileCommon;

/*
 * Tombstone particle VM (MkPfx.matrix / +0x40 region used as pfx base).
 * Offsets relative to the pointer passed to init_tombstone_* / set_*_positions.
 */
typedef struct TombstonePfx {
    char pad00[0x50];
    int capacity; /* +0x50 */
    int count;    /* +0x54 */
    char pad58[0xF8];
    unsigned char flags_150; /* +0x150 */
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
    char pad1AC[8];
    PfxColor rgba_1B4; /* +0x1B4 */
    float scale;      /* +0x1B8 */
} TombstonePfx;

/*
 * Main krypt mode payload (size 0x150; zeroed in p_init_krypt_mode).
 * Only fields touched by setup / tombstone / getters are named.
 */
typedef struct KryptPdata {
    unsigned char hdr[0x08]; /* +0x00 */
    int player_state_copy;   /* +0x08 */
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
    unsigned char unk40[0x10];
    int layout_dirty;  /* +0x50 - p_krypt_loop refreshes pebbles */
    int current_row;   /* +0x54 */
    int current_column; /* +0x58 */
    unsigned char unk5c[0x90];
    int tombstone_hud_ticks; /* +0xEC - countdown; update_* refresh while >0 */
    void* pfx_koins;         /* +0xF0 */
    void* pfx_letters;       /* +0xF4 */
    void* pfx_numbers;       /* +0xF8 */
    unsigned char unkfc[0x04];
    void* anim_pdata; /* +0x100 - get_krypt_anim_pdata */
    unsigned char unk104[0x08];
    int anim_flag; /* +0x10C - cleared by anim script setters */
    unsigned char unk110[0x18];
    MkPtr* tracked_sound_list;        /* +0x128 - fire-pot sound list head */
    TrackedSound* fire_pot_sounds[6]; /* +0x12C..+0x140 */
    void* player_profile;             /* +0x144 */
    ProfileCommon* profile_common;    /* +0x148 */
    void* profile_konquest;           /* +0x14C */
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

/* Tombstone / coffin grid (MVP - Cluster C) */
void setup_tombstones(void);
void set_pebble_positions_for_row(int row, int start_col, int count, float* origin);
void position_fire_pots(void);
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
