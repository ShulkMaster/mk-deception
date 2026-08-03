#ifndef MKD_MOVELIST_H
#define MKD_MOVELIST_H

#include "game/game_info.h"
#include "runtime/mk_vtbl.h"

/*
 * Movelist screen pdata (alloc size 0x898).
 * Style slots stride 0x1B0 from base; trailer @ +0x880.
 * Slot trailer (+0x1B4..+0x1BF) overlaps next slot's +0x00..+0x0F by design.
 */

#define MOVELIST_STYLE_STRIDE 0x1B0
#define MOVELIST_MOVES_PER_STYLE 35

/* Move-table row (stride 0x14) from MoveTableContainer.move_table. */
typedef struct MovelistRow {
    int style_index;         /* +0x00 */
    int field4;              /* +0x04 */
    const char* button_text; /* +0x08 */
    const char* rewrite_src; /* +0x0C */
    void* special_arg;       /* +0x10 */
} MovelistRow; /* 0x14 */

typedef struct MovelistMoveEntry {
    int field_00;            /* +0x00 - row->field4 */
    int rewrite_src;         /* +0x04 - (int)row->rewrite_src */
    const char* button_text; /* +0x08 */
} MovelistMoveEntry; /* 0xC */

/* ScreenObj-shaped 2D pfx used by movelist (load_2d_pfxobj_*). */
typedef struct MovelistPfxObj {
    void* vtbl;   /* +0x00 */
    int instance; /* +0x04 */
    char pad08[4];
    unsigned char flags_0C; /* +0x0C - bit4 / bit1 via movelist_set_pfx_byte_flags */
    char pad0D[7];
    int x; /* +0x14 */
    int y; /* +0x18 */
} MovelistPfxObj;

typedef struct MovelistStyleSlot {
    char pad00[4];
    MovelistPfxObj* special_pfx; /* +0x04 - STYLE_SPECIAL when style_count > 3 */
    int special_pfx_inst;        /* +0x08 */
    char pad0C[4];
    MovelistMoveEntry moves[MOVELIST_MOVES_PER_STYLE]; /* +0x10 */
    MovelistPfxObj* pfx_obj;                           /* +0x1B4 */
    int pfx_inst;                                      /* +0x1B8 */
    int max_move; /* +0x1BC - fill cursor during init_movelist */
} MovelistStyleSlot; /* logical end 0x1C0; stride 0x1B0 */

typedef struct MovelistPdata {
    MkVtable5* vtbl;       /* +0x00 */
    int field_04;          /* +0x04 */
    GameInfoPlyr* plyr;    /* +0x08 */
    void* switch_map;      /* +0x0C */
    char pad10[0x98];
    unsigned char flags_A8; /* +0xA8 - bit3 sleep latch */
    char padA9[0x7D7];
    char counter_buf[8]; /* +0x880 */
    int switch_side;     /* +0x888 */
    int style_idx;       /* +0x88C */
    int style_count;     /* +0x890 */
    void* obj_list;      /* +0x894 - mk_insert list head */
} MovelistPdata; /* 0x898 */

#define movelist_style_slot(pdata_, idx_) \
    ((MovelistStyleSlot*)((char*)(pdata_) + (idx_) * MOVELIST_STYLE_STRIDE))

#endif
