#include "game/acb.h"

#include "game/game_info.h"
#include "game/movelist.h"
#include "libmkparticle/pfx2d.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_vtbl.h"
#include "runtime/plyr_info.h"

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

static int vdestroy_movelist(void* self);

MkVtable5 vtbl_movelist = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    (MkVtblFn)vdestroy_movelist,
};

static char space[] = " ";

extern GlobalPlayerEntry global_player_data[];
extern int pause_player;
extern int screen_width;
extern int screen_height;
extern unsigned char p1_profile_switch_map[];

void* get_screen_pdata(void);
void destroy_mkprocs_pid(int proc_id);
int is_a_to_the_right_of_b(void* a, void* b);
void set_game_switch_map(void* map);
void set_default_switch_map(void* map);
void screen_share_pdata(void* pdata);
void toggle_normal_2d_rendering(int enable);
int is_special_move_available(void* char_data, void* arg);
void rewrite_button_string(const char* charmap, const char* src, int player_side, void* switch_map);
void* load_2d_pfxobj_with_texture(int id, void* texture, int arg2, int arg3);
void* load_named_2d_pfxobj(int arg0, int id, const char* name, int arg3, int arg4);
void hide_or_show_2d_obj_by_id(int id, int show);
char* get_current_screen_name(void);
int strcmp(const char* a, const char* b);
int sprintf(char* buf, const char* fmt, ...);

static void init_movelist(MovelistPdata* movelist_pdata);
static float p_loop_movelist(void);

static inline void movelist_set_pfx_byte_flags(
    MovelistPfxObj* pfx_obj, int set_bit4, int set_bit1) {
    pfx_obj->flags_0C_bits.bit4 = set_bit4;
    if (set_bit1 != 0) {
        pfx_obj->flags_0C_bits.bit1 = 1;
    }
}

static inline void movelist_show_valid_style(MovelistPdata* screen_pdata) {
    int zero;
    int style_index;
    MovelistStyleSlot* style;
    int move_count;
    MovelistPfxObj* pfx_obj;

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
        style = movelist_style_slot(screen_pdata, style_index);
        move_count = style->max_move;
    } while (move_count == 0);
    hide_or_show_2d_obj_by_id(0x9012, 1);
    style_index = screen_pdata->style_idx;
    style = movelist_style_slot(screen_pdata, style_index);
    pfx_obj = style->pfx_obj;
    if (pfx_obj != 0) {
        if (pfx_obj->instance != style->pfx_inst) {
            pfx_obj = 0;
        }
    } else {
        pfx_obj = 0;
    }
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

    screen_pdata = (MovelistPdata*)get_screen_pdata();
    if (screen_pdata != 0) {
        return screen_pdata->counter_buf;
    }
    return 0;
}

void movelist_change_move(int delta) {
    MovelistPdata* screen_pdata;
    int style_index;
    MovelistStyleSlot* style;
    int max_move;
    int display_move;

    screen_pdata = (MovelistPdata*)get_screen_pdata();
    if (screen_pdata == 0) {
        return;
    }
    style_index = screen_pdata->style_idx;
    style = movelist_style_slot(screen_pdata, style_index);
    max_move = style->max_move;
    display_move = delta + 1;
    if (delta >= max_move) {
        display_move = max_move;
    }
    sprintf(screen_pdata->counter_buf, STR_MOVELIST_COUNTER_FMT, display_move, max_move);
}

static inline MovelistPfxObj* movelist_style_live_pfx_obj(MovelistStyleSlot* owner) {
    MovelistPfxObj* object = owner->pfx_obj;
    if (object != 0) {
        if (object->instance == owner->pfx_inst) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

/* TODO: [near miss] 99.051730%; register coloring; one-trial ceiling. */
void movelist_change_style(int delta) {
    MovelistPdata* screen_pdata;
    int zero;
    int style_index;
    MovelistStyleSlot* style;
    int move_count;
    MovelistPfxObj* pfx_obj;

    screen_pdata = (MovelistPdata*)get_screen_pdata();
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
        style = movelist_style_slot(screen_pdata, style_index);
        move_count = style->max_move;
    } while (move_count == 0);

    hide_or_show_2d_obj_by_id(0x9012, 1);

    style_index = screen_pdata->style_idx;
    style = movelist_style_slot(screen_pdata, style_index);
    pfx_obj = movelist_style_live_pfx_obj(style);

    if (pfx_obj == 0) {
        return;
    }
    movelist_set_pfx_byte_flags(pfx_obj, 0, 0);
}

void* get_movelist_strings(int* out_max) {
    MovelistPdata* screen_pdata;

    screen_pdata = (MovelistPdata*)get_screen_pdata();
    if (screen_pdata != 0) {
        *out_max = movelist_style_slot(screen_pdata, screen_pdata->style_idx)->max_move;
        return movelist_style_slot(screen_pdata, screen_pdata->style_idx)->moves;
    }
    return 0;
}

void start_movelist(void) {
    MovelistPdata* movelist_pdata;
    GameInfoPlyr* pad_ptr;
    int side_flag;
    int char_index;
    MkProc* proc;

    destroy_mkprocs_pid(0x9008);
    proc = _create_mkproc_generic_bigstack(0x9008, 0x1F, p_loop_movelist, 0x898,
                                           (MkHdr**)&movelist_pdata);
    if (proc != 0) {
        movelist_pdata->vtbl = &vtbl_movelist;
        zero_pdata_payload(0x898, (MkHdr*)movelist_pdata);
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
        screen_share_pdata(movelist_pdata);
        init_movelist(movelist_pdata);
    }
    toggle_normal_2d_rendering(0);
}

static int vdestroy_movelist(void* self) {
    MovelistPdata* pdata;

    pdata = (MovelistPdata*)self;
    destroy_list(&pdata->obj_list);
    toggle_normal_2d_rendering(1);
    pdata->field_04 = 0;
    mkhdr_memfree((MkHdr*)self);
}

static void init_movelist(MovelistPdata* movelist_pdata) {
    GameInfoPlyr* screen_wrapper;
    FighterMirror* char_data;
    void* move_table;
    int row_count;
    int row_index;
    MovelistPdata* screen_pdata;
    MovelistRow* row;
    MovelistStyleSlot* style;
    MovelistMoveEntry* move_entry;
    int move_slot;
    int style_slot;
    MovelistPfxObj* pfx_obj;
    MovelistPfxObj* named_pfx;
    int style_index;
    int max_move;
    int display_move;
    int half_obj_w;
    int half_screen_w;
    MoveTableContainer* table_container;
    ScriptSlot* cmo;
    int row_style_index;
    int row_field;
    const char* row_button_text;
    const char* row_rewrite_src;
    void* row_special_arg;

    screen_wrapper = movelist_pdata->plyr;
    char_data = screen_wrapper->slot.fighter;
    cmo = char_data->cmo;
    movelist_pdata->style_idx = char_data->style_idx;
    table_container = (MoveTableContainer*)char_data->move_table_container;
    move_table = table_container->move_table;
    if (move_table != 0) {
        row_count = get_row_count_for_table_by_pointer(cmo, move_table);
        for (row_index = 0; row_index < row_count; row_index++) {
            row = &((MovelistRow*)move_table)[row_index];
            row_rewrite_src = row->rewrite_src;
            row_field = row->field4;
            row_button_text = row->button_text;
            row_style_index = row->style_index;
            row_special_arg = row->special_arg;
            screen_pdata = (MovelistPdata*)get_screen_pdata();
            if (screen_pdata != 0) {
                style = movelist_style_slot(screen_pdata, row_style_index);
                move_slot = style->max_move;
                move_entry = &style->moves[move_slot];
                move_entry->field_00 = row_field;
                move_entry->rewrite_src = row_rewrite_src;
                if (row_style_index >= screen_pdata->style_count) {
                    screen_pdata->style_count = row_style_index + 1;
                }
                if (row_style_index == 3) {
                    GameInfoPlyr* inner;

                    inner = screen_pdata->plyr;
                    if (is_special_move_available(inner->slot.fighter, row_special_arg) == 0) {
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
                style->max_move = move_slot + 1;
            }
        }
    }
    for (style_slot = 0; style_slot < 3; style_slot++) {
        FighterStyleObj* style_obj;
        FighterStyleScreen* screen;
        Pfx2dObj* pfx2d;

        style_obj = char_data->style_objs[style_slot];
        screen = style_obj->screen;
        if (screen != 0) {
            if (screen->instance != style_obj->screen_inst) {
                screen = 0;
            }
        } else {
            screen = 0;
        }
        if (screen != 0) {
            pfx2d = (Pfx2dObj*)screen->pfx2d;
            pfx_obj = (MovelistPfxObj*)load_2d_pfxobj_with_texture(0x9012, pfx2d->texture, 0, 5);
            if (pfx_obj != 0) {
                style = movelist_style_slot(movelist_pdata, style_slot);
                movelist_set_pfx_byte_flags(pfx_obj, 1, 1);
                style->pfx_obj = pfx_obj;
                style->pfx_inst = pfx_obj->instance;
                mk_insert((MkHdr*)pfx_obj, &movelist_pdata->obj_list);
                half_obj_w = style_obj->layout->width / 2;
                half_screen_w = screen_width / 2;
                pfx_obj->x = half_screen_w - half_obj_w;
                pfx_obj->y = screen_height - 0x188;
            }
        }
    }
    if (movelist_pdata->style_count > 3) {
        named_pfx = (MovelistPfxObj*)load_named_2d_pfxobj(
            0x10005, 0x9012, STR_STYLE_SPECIAL, 0, 5);
        if (named_pfx != 0) {
            movelist_set_pfx_byte_flags(named_pfx, 1, 1);
            movelist_style_slot(movelist_pdata, movelist_pdata->style_count)
                ->special_pfx = named_pfx;
            movelist_style_slot(movelist_pdata, movelist_pdata->style_count)
                ->special_pfx_inst = named_pfx->instance;
            mk_insert((MkHdr*)named_pfx, &movelist_pdata->obj_list);
            half_screen_w = screen_width / 2;
            named_pfx->x = half_screen_w - 0x6F;
            named_pfx->y = screen_height - 0x188;
        }
    }
    screen_pdata = (MovelistPdata*)get_screen_pdata();
    if (screen_pdata != 0) {
        movelist_show_valid_style(screen_pdata);
    }
    screen_pdata = (MovelistPdata*)get_screen_pdata();
    if (screen_pdata != 0) {
        style_index = screen_pdata->style_idx;
        style = movelist_style_slot(screen_pdata, style_index);
        max_move = style->max_move;
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
    if (screen_name != 0) {
        if (strcmp(screen_name, STR_PAUSE_MOVELIST) == 0) {
            return movelist_loop_pos_one;
        }
    }
    return movelist_loop_neg_one;
}
