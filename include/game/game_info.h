#ifndef MKD_GAME_INFO_H
#define MKD_GAME_INFO_H

/*
 * g_game_info - retail .bss size 0x21C @ 0x803AE64C (symbols.txt).
 * Partial layout shared across display / gcio / attract / nis / acb / bgnd /
 * pselect / menu / game.
 *
 * Layout check (plyr1 ends 0x17C -> pads @ 0x17C -> +0x54 -> 0x1D0 -> +0x28 ->
 * field_1F8 @ 0x1F8 -> ... -> 0x21C). Do NOT insert padding between plyr1 and pads.
 *
 * Anchor evidence (ASM immediates / Ghidra VA = base+off):
 *   plyr0 +0xA4 / plyr1 +0x110 -- mk_system_init / pselect_init / attract
 *   pads[0] +0x17C stride 0x1C -- gcio scan_switches; pads[2] +0x1B4 -- pselect
 *   pselect +0x1D0...+0x1F0 -- pselect_init / init_bet_info_struct @ 0x80028884
 *   field_1F8 / field_210 -- attract.s; field_1FC -- pselect.s
 *   field_208 / field_20C -- mk_struct.s mk_system_init
 *   field_200 / field_204 / field_214 / field_218 -- game.s / menu.s / glue
 */

#include "game/bgnd_types.h"
#include "math/gxVect.h"
#include "runtime/plyr_info.h"

typedef struct MkObj MkObj;
typedef struct MkProc MkProc;
typedef struct ScriptSlot ScriptSlot;

typedef float (*SwitchMapProcFn)(void);

/* Logical controller row used by the input dispatcher (retail stride 0x0C). */
typedef struct SwitchMapEntry {
    unsigned int mask;       /* +0x00 */
    SwitchMapProcFn proc_fn; /* +0x04 */
    const char* label;       /* +0x08 */
} SwitchMapEntry;

typedef struct GcPadFlags {
    unsigned char disabled : 1;      /* bit7 */
    unsigned char connected : 1;     /* bit6 */
    unsigned char stick_dispatch : 1; /* bit5 */
    unsigned char pad : 5;
} GcPadFlags;

/* Per-port pad state @ +0x17C, stride 0x1C (gcio scan_switches). */
typedef struct GcPadSlot {
    union {
        unsigned char flags;
        GcPadFlags flag_bits;
    }; /* +0x00 */
    unsigned char pad_01[3];
    SwitchMapEntry* switch_map; /* +0x04 - logical switch rows */
    PlyrInfo* player;          /* +0x08 - abs +0x184 for port 0 */
    unsigned int prev_buttons; /* +0x0C - abs +0x188 */
    unsigned int buttons;      /* +0x10 - abs +0x18C */
    unsigned int edge;         /* +0x14 - abs +0x190 */
    union {
        unsigned int stick_pack; /* +0x18 - abs +0x194 */
        struct {
            unsigned char stick_axis_0;
            unsigned char stick_axis_1;
            unsigned char stick_axis_2;
            unsigned char stick_axis_3;
        };
    };
} GcPadSlot; /* 0x1C */

typedef struct GamePauseFlags {
    unsigned char fatality_window : 1; /* bit7 */
    unsigned char pad_6_3 : 4;
    unsigned char controller_disable_guard : 1; /* bit2 */
    unsigned char controllers_disabled : 1;     /* bit1 */
    unsigned char paused : 1;                   /* bit0 */
} GamePauseFlags;

typedef struct GameSwitchInputFlags {
    unsigned char pad_7_4 : 4;
    unsigned char eat_switches : 1; /* bit3 - pause/online input suppression */
    unsigned char pad_2_0 : 3;
} GameSwitchInputFlags;

typedef struct GameFeatureFlagBits {
    unsigned char high_bit : 1; /* bit7 */
    unsigned char pad : 7;
} GameFeatureFlagBits;

typedef union GameFeatureFlags {
    unsigned char raw;
    GameFeatureFlagBits bits;
} GameFeatureFlags;

typedef struct GameInfoFlags {
    unsigned char pad_high : 5;
    unsigned char level_fatality_done : 1; /* bit2 */
    unsigned char pad_low : 2;
} GameInfoFlags;

#define GC_PAD_FLAG_DISABLED 0x80
#define GC_PAD_FLAG_CONNECTED 0x40
#define GC_PAD_FLAG_STICK_DISPATCH 0x20
#define GC_PAD_SLOT_STRIDE 0x1C
#define GC_PAD_SLOTS_OFF 0x17C
/* player @ slot+8 -> abs +0x184; edge @ slot+0x14 -> abs +0x190 */

/*
 * Bet / pselect / round tail @ +0x1D0 (size 0x28 -> field_1F8).
 * init_bet_info_struct @ 0x80028884 and pselect_init @ 0x8008A688.
 */
typedef struct GameInfoPselectTail {
    int field_1d0; /* +0x1D0 - zeroed by pselect_init / init_bet_info_struct */
    int field_1d4; /* +0x1D4 */
    int field_1d8; /* +0x1D8 - wager amount (>0 on cancel refund path) */
    int field_1dc; /* +0x1DC */
    int field_1e0; /* +0x1E0 - set 1 in pselect_init / init_bet_info_struct */
    int field_1e4; /* +0x1E4 - init_bet_info_struct stores 6 */
    int field_1e8; /* +0x1E8 - init_bet_info_struct / ladder / minigames */
    int field_1ec; /* +0x1EC - init_bet_info_struct / do_win_effect */
    int field_1f0; /* +0x1F0 - set 1 in pselect_init */
    int field_1f4; /* +0x1F4 - round index (do_fight_effect lwz) */
} GameInfoPselectTail; /* 0x28 */

typedef struct GameInfo {
    union {
        unsigned char flags;
        GameInfoFlags flag_bits;
    };                         /* +0x00 - bit7 hi-res path; bit2 level fatality */
    union {
        unsigned char pause_flags;
        GamePauseFlags pause_flag_bits;
    }; /* +0x01 */
    GameSwitchInputFlags switch_input_flags; /* +0x02 */
    char pad03;
    union {
        unsigned char field_04; /* +0x04 - raw attract/glue flags */
        GameFeatureFlags feature_flags;
    };
    char pad05[3];
    int field_08;           /* +0x08 */
    BgndDataTable* section; /* +0x0C - get_data_table / art section */
    BgndMisc* misc;         /* +0x10 */
    void* mode_table; /* +0x14 - mode-specific table (chess table in MK Chess) */
    int bgnd_id; /* +0x18 */
    int active_level; /* +0x1C - current multi-level arena index */
    ScriptSlot* cmdscript; /* +0x20 - loaded MKO body from cmdscript_loadfile_* */
    char pad24[8];
    MkObj* bgnd_obj; /* +0x2C */
    MkObj* sky;      /* +0x30 */
    float field_34;  /* +0x34 - fade / particle / mab */
    Vec impact_vector; /* +0x38 - normalized/scaled death-trap impact */
    PlyrInfo* active_player; /* +0x44 - current fight player */
    char pad48[4];
    MkObj* player_objects[2]; /* +0x4C */
    char pad54[0x0C];
    int field_60; /* +0x60 */
    int field_64; /* +0x64 */
    int field_68; /* +0x68 */
    MkProc* camera_proc; /* +0x6C */
    unsigned int camera_proc_instance; /* +0x70 */
    int field_74; /* +0x74 */
    char pad78[0x1C];
    int field_94; /* +0x94 */
    char pad98[0xC];
    PlyrInfo plyr0; /* +0xA4 */
    PlyrInfo plyr1; /* +0x110 -- ends 0x17C */
    union {
        GcPadSlot pads[4]; /* +0x17C -- four physical GameCube ports */
        struct {
            GcPadSlot first_three_pads[3];
            GameInfoPselectTail pselect;
        } pad_overlay;
        struct {
            GcPadSlot first_three_pads[3];
            GameInfoPselectTail pselect; /* +0x1D0 -- gameplay overlay */
        };
    }; /* +0x17C..+0x1F8 */
    int field_1F8; /* +0x1F8 - attract page/state latch (attract.s stw) */
    int field_1FC; /* +0x1FC - ladder side latch (pselect.s stw) */
    int field_200; /* +0x200 - fatality / round_init latch (game.s / Ghidra VA 803AE84C) */
    int field_204; /* +0x204 - game timer word (reset_game_timer / update_game_timer) */
    int field_208; /* +0x208 - cleared in mk_system_init (stw 0x208) */
    int field_20C; /* +0x20C - cleared in mk_system_init (stw 0x20c) */
    int field_210; /* +0x210 - attract loop counter (attract.s lwz/stw 0x210) */
    int field_214; /* +0x214 - menu sets 0x1E; mwScreenEngineGlue sprintf (stw/lwz 0x214) */
    int field_218; /* +0x218 - reset_game_timer write (game.s stw 0x218) */
} GameInfo; /* 0x21C */

extern GameInfo g_game_info;

#endif
