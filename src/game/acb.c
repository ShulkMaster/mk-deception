#include "game/acb.h"
#include "game/plyr_globals.h"

#include "game/controller.h"
#include "game/game_info.h"
#include "game/movelist.h"
#include "game/plyr.h"
#include "runtime/fonts.h"
#include "libmkparticle/pfx2d.h"
#include "mw/mwScreenEngineGlue.h"
#include "platform/display_metrics.h"
#include "runtime/cam.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_vtbl.h"
#include "runtime/plyr_info.h"
#include "runtime/utils.h"

static const char movelist_charmap[] = "lrRLYBAX...../.:.";

static const char stringBase0[] =
    "%d / %d\0"
    "STYLE_SPECIAL\0"
    "pause_movelist\0"
    "%u ( %0.3f, %0.3f, %0.3f ) %c\0";

#define STR_MOVELIST_COUNTER_FMT (&stringBase0[0])
#define STR_STYLE_SPECIAL (&stringBase0[0x8])
#define STR_PAUSE_MOVELIST (&stringBase0[0x16])

static const float movelist_loop_neg_one = -1.0f;
static const float movelist_loop_pos_one = 1.0f;

struct MovelistVtable {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(MovelistPdata* owner);
};
typedef char check_MovelistVtable_size[(sizeof(struct MovelistVtable) == sizeof(MkVtable5)) ? 1 : -1];

static void vdestroy_movelist(MovelistPdata* pdata);

struct MovelistVtable vtbl_movelist = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_movelist,
};

static char space[] = " ";

extern int pause_player;
extern unsigned char p1_profile_switch_map[];

char* get_current_screen_name(void);

static void init_movelist(MovelistPdata* movelist_pdata);
static float p_loop_movelist(void);

static inline void movelist_set_pfx_byte_flags(
    ScreenObj* pfx_obj, int set_bit4, int set_bit1) {
    pfx_obj->flag_bits.hidden = set_bit4;
    if (set_bit1 != 0) {
        pfx_obj->flag_bits.bit1 = 1;
    }
}

static inline void movelist_show_valid_style(MovelistPdata* screen_pdata) {
    int zero;
    int style_index;
    int move_count;
    ScreenObj* pfx_obj;

    zero = 0;
    do {
        style_index = screen_pdata->style_idx;
        if (style_index >= screen_pdata->style_count) {
            screen_pdata->style_idx = zero;
        }
        if (screen_pdata->style_idx < zero) {
            screen_pdata->style_idx = screen_pdata->style_count - 1;
        }
        style_index = screen_pdata->style_idx;
        move_count = screen_pdata->styles[style_index].max_move;
    } while (move_count == 0);
    hide_or_show_2d_obj_by_id(0x9012, 1);
    pfx_obj = MK_LIVE(screen_pdata->styles[screen_pdata->style_idx].pfx_obj, screen_pdata->styles[screen_pdata->style_idx].pfx_inst);
    if (pfx_obj == 0) {
        return;
    }
    movelist_set_pfx_byte_flags(pfx_obj, 0, 0);
}

void* movelist_get_character_name(void) {
    int player_index;

    if (pause_player == 1) {
        player_index = g_game_info.plyr1.player_index;
        return global_player_data[player_index].name;
    }
    player_index = g_game_info.plyr0.player_index;
    return global_player_data[player_index].name;
}

void* movelist_get_counter(void) {
    MovelistPdata* screen_pdata;

    screen_pdata = get_screen_pdata();
    if (screen_pdata != 0) {
        return screen_pdata->counter_buf;
    }
    return 0;
}

#pragma opt_propagation off
void movelist_change_move(int delta) {
    MovelistPdata* screen_pdata;
    int style_index;
    int max_move;
    int display_move;

    screen_pdata = get_screen_pdata();
    if (screen_pdata == 0) {
        return;
    }
    style_index = screen_pdata->style_idx;
    max_move = screen_pdata->styles[style_index].max_move;
    display_move = delta + 1;
    if (delta >= max_move) {
        display_move = max_move;
    }
    sprintf(screen_pdata->counter_buf, STR_MOVELIST_COUNTER_FMT, display_move, max_move);
}

#pragma opt_propagation reset

void movelist_change_style(int delta) {
    MovelistPdata* screen_pdata;
    int zero;
    int style_index;
    int move_count;
    ScreenObj* pfx_obj;

    screen_pdata = get_screen_pdata();
    zero = 0;
    if (screen_pdata == 0) {
        return;
    }
    do {
        screen_pdata->style_idx += delta;
        if (screen_pdata->style_idx >= screen_pdata->style_count) {
            screen_pdata->style_idx = zero;
        }
        if (screen_pdata->style_idx < zero) {
            screen_pdata->style_idx = screen_pdata->style_count - 1;
        }
        style_index = screen_pdata->style_idx;
        move_count = screen_pdata->styles[style_index].max_move;
    } while (move_count == 0);

    hide_or_show_2d_obj_by_id(0x9012, 1);

    pfx_obj = MK_LIVE(screen_pdata->styles[screen_pdata->style_idx].pfx_obj, screen_pdata->styles[screen_pdata->style_idx].pfx_inst);

    if (pfx_obj == 0) {
        return;
    }
    movelist_set_pfx_byte_flags(pfx_obj, 0, 0);
}

#pragma opt_propagation off
void* get_movelist_strings(int* out_max) {
    MovelistPdata* screen_pdata;

    screen_pdata = get_screen_pdata();
    if (screen_pdata != 0) {
        int style_index = screen_pdata->style_idx;
        *out_max = screen_pdata->styles[style_index].max_move;
        return screen_pdata->styles[screen_pdata->style_idx].moves;
    }
    return 0;
}

#pragma opt_propagation reset

void start_movelist(void) {
    MovelistPdata* movelist_pdata;
    GameInfoPlyr* pad_ptr;
    int side_flag;
    int char_index;
    MkProc* proc;

    destroy_mkprocs_pid(0x9008);
    proc = _create_mkproc_generic_bigstack(0x9008, 0x1F, p_loop_movelist, sizeof(MovelistPdata),
                                           (MkHdr**)&movelist_pdata);
    if (proc != 0) {
        movelist_pdata->hdr.vtbl = MK_VTABLE_ADDRESS(vtbl_movelist);
        zero_pdata_payload(sizeof(MovelistPdata), &movelist_pdata->hdr);
        proc->flags_bits.skip_if_paused = 1;
        if (pause_player == 1) {
            pad_ptr = &g_game_info.plyr1;
            movelist_pdata->plyr = pad_ptr;
            side_flag =
                is_a_to_the_right_of_b(g_game_info.plyr1.slot.mirror_a,
                                       g_game_info.plyr0.slot.mirror_a) != 0;
            movelist_pdata->switch_side = side_flag;
        } else {
            pad_ptr = &g_game_info.plyr0;
            movelist_pdata->plyr = pad_ptr;
            side_flag =
                is_a_to_the_right_of_b(g_game_info.plyr0.slot.mirror_a,
                                       g_game_info.plyr1.slot.mirror_a) != 0;
            movelist_pdata->switch_side = side_flag;
            movelist_pdata->switch_map = p1_profile_switch_map;
        }
        set_game_switch_map(movelist_pdata->plyr);
        char_index = movelist_pdata->plyr->pad_index;
        movelist_pdata->switch_map = g_game_info.pads[char_index].switch_map;
        set_default_switch_map(movelist_pdata->plyr);
        screen_share_pdata(&movelist_pdata->hdr);
        init_movelist(movelist_pdata);
    }
    toggle_normal_2d_rendering(0);
}

static void vdestroy_movelist(MovelistPdata* pdata) {
    destroy_list(&pdata->obj_list);
    toggle_normal_2d_rendering(1);
    pdata->hdr.instance = 0;
    mkhdr_memfree(&pdata->hdr);
}

static void init_movelist(MovelistPdata* movelist_pdata) {
    int row_count;
    MovelistStyleSlot* style;
    int style_slot;
    GameInfoPlyr* screen_wrapper;
    FighterMirror* char_data;
    MovelistRow* move_table;
    MovelistPdata* screen_pdata;
    MovelistRow* row;
    int row_index;
    MovelistMoveEntry* move_entry;
    ScreenObj* pfx_obj;
    ScreenObj* named_pfx;
    int style_index;
    int max_move;
    int display_move;
    int half_obj_w;
    int half_screen_w;
    MoveTableContainer* table_container;
    ScriptSlot* cmo;
    char* row_rewrite_src;
    int row_field;
    const char* row_button_text;
    int row_style_index;
    int row_special_arg;

    screen_wrapper = movelist_pdata->plyr;
    char_data = screen_wrapper->slot.fighter;
    cmo = char_data->cmo;
    movelist_pdata->style_idx = char_data->style_idx;
    table_container = char_data->move_table_container;
    move_table = table_container->move_table;
    if (move_table != 0) {
        row_count = get_row_count_for_table_by_pointer(cmo, move_table);
        for (row_index = 0; row_index < row_count; row_index++) {
            row = &move_table[row_index];
            row_rewrite_src = row->rewrite_src;
            row_field = row->field4;
            row_button_text = row->button_text;
            row_style_index = row->style_index;
            row_special_arg = row->special_arg;
            screen_pdata = get_screen_pdata();
            if (screen_pdata != 0) {
                style = &screen_pdata->styles[row_style_index];
                move_entry = screen_pdata->styles[row_style_index].moves;
                move_entry += style->max_move;
                move_entry->field_00 = row_field;
                move_entry->rewrite_src = row_rewrite_src;
                if (row_style_index >= screen_pdata->style_count) {
                    screen_pdata->style_count = row_style_index + 1;
                }
                if (row_style_index == 3) {
                    GameInfoPlyr* inner;

                    inner = screen_pdata->plyr;
                    if (is_special_move_available(inner->slot.pdata, row_special_arg) == 0) {
                        continue;
                    }
                }
                if (row_button_text != 0) {
                    move_entry->button_text = row_button_text;
                } else {
                    move_entry->button_text = space;
                }
                rewrite_button_string(movelist_charmap, row_rewrite_src,
                                      screen_pdata->switch_side,
                                      screen_pdata->switch_map);
                style->max_move++;
            }
        }
    }
    for (style_slot = 0; style_slot < 3; style_slot++) {
        FighterStyleScreen* screen;
        Pfx2dObj* pfx2d;

        screen = MK_LIVE(char_data->style_objs[style_slot]->screen, char_data->style_objs[style_slot]->screen_inst);

        if (screen != 0) {
            pfx2d = screen->pfx2d;
            pfx_obj = load_2d_pfxobj_with_texture(0x9012, pfx2d->texture, 0, 5);
            if (pfx_obj != 0) {
                movelist_set_pfx_byte_flags(pfx_obj, 1, 1);
                movelist_pdata->styles[style_slot].pfx_obj = pfx_obj;
                movelist_pdata->styles[style_slot].pfx_inst = pfx_obj->instance;
                mk_insert((MkHdr*)pfx_obj, &movelist_pdata->obj_list);
                half_obj_w = char_data->style_objs[style_slot]->layout->width / 2;
                half_screen_w = screen_width / 2;
                pfx_obj->x = half_screen_w - half_obj_w;
                pfx_obj->y = screen_height - 0x188;
            }
        }
    }
    if (movelist_pdata->style_count > 3) {
        named_pfx = load_named_2d_pfxobj(
            0x10005, 0x9012, STR_STYLE_SPECIAL, 0, 5);
        if (named_pfx != 0) {
            movelist_set_pfx_byte_flags(named_pfx, 1, 1);
            movelist_pdata->styles[movelist_pdata->style_count - 1].pfx_obj =
                named_pfx;
            movelist_pdata->styles[movelist_pdata->style_count - 1].pfx_inst =
                named_pfx->instance;
            mk_insert((MkHdr*)named_pfx, &movelist_pdata->obj_list);
            half_screen_w = screen_width / 2;
            named_pfx->x = half_screen_w - 0x6F;
            named_pfx->y = screen_height - 0x188;
        }
    }
    screen_pdata = get_screen_pdata();
    if (screen_pdata != 0) {
        movelist_show_valid_style(screen_pdata);
    }
    screen_pdata = get_screen_pdata();
    if (screen_pdata != 0) {
        style_index = screen_pdata->style_idx;
        max_move = screen_pdata->styles[style_index].max_move;
        display_move = 1;
        if (max_move <= 0) {
            display_move = max_move;
        }
        sprintf(screen_pdata->counter_buf, STR_MOVELIST_COUNTER_FMT,
                display_move, max_move);
    }
}

static float p_loop_movelist(void) {
    char* screen_name;

    screen_name = get_current_screen_name();
    if (screen_name == 0 || strcmp(screen_name, STR_PAUSE_MOVELIST) != 0) {
        return movelist_loop_pos_one;
    }
    return movelist_loop_neg_one;
}
