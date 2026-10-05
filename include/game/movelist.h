#ifndef MKD_MOVELIST_H
#define MKD_MOVELIST_H

#include "game/game_info.h"
#include "runtime/mk_struct.h"
#include "runtime/image.h"
#include "runtime/mk_vtbl.h"

#define MOVELIST_STYLE_STRIDE 0x1B0
#define MOVELIST_MOVES_PER_STYLE 35

typedef struct MovelistRow {
    int style_index;
    int field4;
    const char* button_text;
    char* rewrite_src;
    int special_arg;
} MovelistRow;

typedef struct MovelistMoveEntry {
    int field_00;
    char* rewrite_src;
    const char* button_text;
} MovelistMoveEntry;

typedef struct MovelistStyleSlot {
    MovelistMoveEntry moves[MOVELIST_MOVES_PER_STYLE];
    ScreenObj* pfx_obj;
    unsigned int pfx_inst;
    int max_move;
} MovelistStyleSlot;

typedef struct MovelistPdata {
    MkHdr hdr;

    GameInfoPlyr* plyr;
    void* switch_map;
    MovelistStyleSlot styles[5];
    char counter_buf[8];
    int switch_side;
    int style_idx;
    int style_count;
    MkPtr* obj_list;
} MovelistPdata;

#endif
