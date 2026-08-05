#include "game/game_info.h"
#include "runtime/fonts.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_pdata.h"
#include "runtime/image.h"

extern int game_tick_ctr;

extern int mode_of_play;
extern int screen_width;
extern GlobalPlayerEntry global_player_data[];

typedef PlyrScreenLatch ScreenLatch;

typedef struct ProcLatch {
    MkProc* object;
    unsigned int instance;
} ProcLatch;

typedef struct PbarHideStringItem {
    ScreenLatch* latch;
    unsigned char alpha;
    unsigned char pad[3];
} PbarHideStringItem;

typedef struct PbarFadePdata {
    MkHdr hdr;
    int alpha;
} PbarFadePdata;

typedef struct PbarExtendPdata {
    MkHdr hdr;
    int active;
} PbarExtendPdata;

typedef struct PwrbarProcVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    MkVtblFn destroy;
    MkVtblFn dispatch;
    MkVtblFn sleep;
} PwrbarProcVtable;

typedef PlyrFightingLightState FightingLightState;

int BAR_BACK_X = 8;
int PB_CNTR_RING_X = 0x100;

float p1_disp_life;
float p2_disp_life;
int f_p1_force_adjustment;
int f_p2_force_adjustment;
ProcLatch pwr_bar_proc_item;
ScreenLatch pbar_cntr_dragon_item;
ScreenLatch pbar_cntr_item;
ScreenLatch p2_bar_icon_item;
ScreenLatch p1_bar_icon_item;
ScreenLatch p2_bolt_3_item;
ScreenLatch p2_bolt_2_item;
ScreenLatch p2_bolt_1_item;
ScreenLatch p1_bolt_3_item;
ScreenLatch p1_bolt_2_item;
ScreenLatch p1_bolt_1_item;
ScreenLatch p2_pbar_backb_item;
ScreenLatch p1_pbar_backb_item;
ScreenLatch p2_pbar_back_item;
ScreenLatch p1_pbar_back_item;
ScreenLatch p2_pbar_red_item;
ScreenLatch p1_pbar_red_item;
ScreenLatch p2_pbar_item;
ScreenLatch p1_pbar_item;
ScreenLatch p2_name_item;
ScreenLatch p1_name_item;
float p2_bar_red_start;
float p2_bar_back_start;
float p1_bar_red_start;
float p1_bar_back_start;
int p2_last_combo_break_count;
int p1_last_combo_break_count;
int f_powerbars_retracted;
int f_p2_warning_given;
int f_p1_warning_given;

extern ScreenLatch game_timer_item;

ScreenLatch* pbar_hide_screen_items[] = {
    &p1_bar_icon_item, &p2_bar_icon_item,
    &g_game_info.plyr0.fighting_lights.base,
    &g_game_info.plyr1.fighting_lights.base,
    &p1_pbar_backb_item, &p2_pbar_backb_item,
    &p1_bolt_1_item, &p1_bolt_2_item, &p1_bolt_3_item,
    &p2_bolt_1_item, &p2_bolt_2_item, &p2_bolt_3_item, 0
};
ScreenLatch* pbar_item_list[] = {
    &p1_pbar_item, &p2_pbar_item,
    &p1_pbar_back_item, &p2_pbar_back_item,
    &p1_pbar_backb_item, &p2_pbar_backb_item,
    &p1_bar_icon_item, &p2_bar_icon_item,
    &p1_pbar_red_item, &p2_pbar_red_item,
    &pbar_cntr_item, &pbar_cntr_dragon_item,
    &p1_bolt_1_item, &p1_bolt_2_item, &p1_bolt_3_item,
    &p2_bolt_1_item, &p2_bolt_2_item, &p2_bolt_3_item,
    &g_game_info.plyr0.fighting_lights.base,
    &g_game_info.plyr1.fighting_lights.base,
    &g_game_info.plyr0.fighting_lights.red,
    &g_game_info.plyr1.fighting_lights.red,
    &g_game_info.plyr0.fighting_lights.green,
    &g_game_info.plyr1.fighting_lights.green,
    &g_game_info.plyr0.fighting_lights.airborne,
    &g_game_info.plyr1.fighting_lights.airborne, 0
};
ScreenLatch* pbar_string_item_list[] = {
    &p1_name_item, &p2_name_item, &game_timer_item,
    &g_game_info.plyr0.name_latch, &g_game_info.plyr1.name_latch, 0
};
PbarHideStringItem pbar_hide_string_items[] = {
    {&p1_name_item, 0xFF, {0, 0, 0}},
    {&p2_name_item, 0xFF, {0, 0, 0}},
    {&g_game_info.plyr0.name_latch, 0xB4, {0, 0, 0}},
    {&g_game_info.plyr1.name_latch, 0xB4, {0, 0, 0}},
    {0, 0, {0, 0, 0}}
};

static ScreenObj* medal_objs[8];

int is_plyr_airborn(MkObj* object);
int is_timer_off(void);
int trial_show_standard_fight_messages(void);
float p_power_bar_proc(void);
void display_debug_damage(PlyrInfo* player);
void snd_req(int sound_id);
void shake_camera(int strength, MkHdr* pdata, float duration);
float p_unhide_pbar_items(void);
float p_update_fighting_state_lights(void);
int sprintf(char* destination, const char* format, ...);
void set_string_obj_alpha(StringObj* object, float alpha);
void pfx_2d_obj_set_alpha_by_id(int oid, int alpha);

static float bar_speed = 0.01f;
static ScreenObj* screen_latch_object(ScreenLatch* latch);

static inline FightingLightState* fighting_light_state(PlyrInfo* player) {
    return &player->fighting_lights;
}

static inline StringObj* string_latch_object(ScreenLatch* latch) {
    StringObj* object = (StringObj*)latch->object;

    if (object != 0) {
        if ((unsigned int)object->instance == latch->instance) {
            /* Keep the live object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    return object;
}

static inline ScreenObj* owned_screen_latch_object(ScreenLatch* latch) {
    ScreenObj* object = latch->object;

    if (object != 0) {
        if ((unsigned int)object->instance == latch->instance) {
            /* Keep the live object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    return object;
}

static inline void owned_set_quad_alpha(
    ScreenObj* object, unsigned char alpha) {
    int i;

    if (object == 0 || object->pfx2d == 0) {
        return;
    }
    for (i = 0; i < 4; i++) {
        object->pfx2d->verts[i].a = alpha;
    }
}

static inline void set_latched_quad_alpha(ScreenLatch* latch) {
    ScreenObj* object = latch->object;

    if (object != 0) {
        if ((unsigned int)object->instance == latch->instance) {
            /* Keep the live object. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        object->pfx2d->verts[0].a = 0xFF;
        object->pfx2d->verts[1].a = 0xFF;
        object->pfx2d->verts[2].a = 0xFF;
        object->pfx2d->verts[3].a = 0xFF;
    }
}

static void set_quad_alpha(ScreenObj* object, unsigned char alpha) {
    int i;

    if (object == 0 || object->pfx2d == 0) {
        return;
    }
    for (i = 0; i < 4; i++) {
        object->pfx2d->verts[i].a = alpha;
    }
}

void show_wins_in_a_row(void) {
    StringObj* string = 0;
    ScreenLatch* latch;
    char text[0x50];
    int wins;

    del_string_obj_by_id(0x201E);
    wins = g_game_info.plyr0.field_44;
    if (wins >= 1 || g_game_info.plyr1.field_44 >= 1) {
        if (wins >= 1) {
            sprintf(text, get_string(5), wins);
            string = string_left_xy(0x201E, 0, text, 0x14, 0x3D, 0x1D);
            if (string != 0) {
                latch = &g_game_info.plyr0.name_latch;
                latch->object = (ScreenObj*)string;
                latch->instance = string->instance;
            }
        } else {
            sprintf(text, get_string(5), g_game_info.plyr1.field_44);
            string = string_right_xy(
                0x201E, 0, text, screen_width - 0x14, 0x3D, 0x1D);
            if (string != 0) {
                latch = &g_game_info.plyr1.name_latch;
                latch->object = (ScreenObj*)string;
                latch->instance = string->instance;
            }
        }
        if (string != 0 && f_powerbars_retracted != 0) {
            set_string_obj_alpha(string, 0.0f);
        } else {
            pfx_2d_obj_set_alpha_by_id(0x201E, 0xB4);
        }
    }
}

int are_powerbars_retracted(void) {
    return f_powerbars_retracted;
}

static inline int red_light_active(PlyrInfo* player) {
    PlyrPdata* pdata = player->slot.pdata;

    if (pdata->blocking_disabled != 0) {
        return 1;
    }
    return pdata->blocking_disable_tick_1 > game_tick_ctr;
}

static inline int airborne_light_active(PlyrInfo* player) {
    PlyrPdata* pdata = player->slot.pdata;

    if (pdata->blocking_disabled_2 != 0) {
        return 1;
    }
    if (pdata->blocking_disable_tick_2 > game_tick_ctr) {
        return 1;
    }
    return is_plyr_airborn(player->slot.mirror_a) != 0;
}

int check_for_red_light(PlyrInfo* player) {
    return red_light_active(player);
}

int check_for_green_light(PlyrInfo* player) {
    return airborne_light_active(player);
}

/* Soft ceiling: 95.16% -- latch register allocation and branch placement. */
float p_unhide_pbar_items(void) {
    PbarFadePdata* pdata = (PbarFadePdata*)apdata;
    PbarHideStringItem* string_item;
    ScreenLatch* latch;
    ScreenObj* screen;
    StringObj* string;
    unsigned int alpha;
    int screen_index;
    int string_index;

    alpha = (unsigned char)pdata->alpha;
    for (screen_index = 0;
         pbar_hide_screen_items[screen_index] != 0; screen_index++) {
        latch = pbar_hide_screen_items[screen_index];
        screen = owned_screen_latch_object(latch);
        if (screen != 0 && screen->pfx2d->verts[0].a < 0xFF) {
            screen->pfx2d->verts[0].a = (unsigned char)alpha;
            screen->pfx2d->verts[1].a = (unsigned char)alpha;
            screen->pfx2d->verts[2].a = (unsigned char)alpha;
            screen->pfx2d->verts[3].a = (unsigned char)alpha;
        }
    }
    for (string_index = 0;
         pbar_hide_string_items[string_index].latch != 0; string_index++) {
        string_item = &pbar_hide_string_items[string_index];
        string = string_latch_object(string_item->latch);
        if (string != 0) {
            if (alpha <= string_item->alpha) {
                set_string_obj_alpha(string, (float)alpha);
            } else {
                set_string_obj_alpha(string, (float)string_item->alpha);
            }
        }
    }
    pdata->alpha += 8;
    if (pdata->alpha >= 0xFF) {
        return -1.0f;
    }
    if (pdata->alpha > 0xFF) {
        pdata->alpha = 0xFF;
    }
    return 1.0f;
}

/* Soft ceiling: 91.91% -- four inlined latch branch diamonds remain shorter. */
void retract_power_bars(void) {
    ScreenObj* p1_back;
    ScreenObj* p1_red;
    ScreenObj* p2_back;
    ScreenObj* p2_red;
    PbarHideStringItem* string_item;
    ScreenLatch* latch;
    ScreenObj* screen;
    StringObj* string;
    int screen_index;
    int string_index;
    int allowed;

    if (g_game_info.pselect.field_1f4 == 1) {
        switch (mode_of_play) {
        case 4:
        case 8:
            allowed = 0;
            break;
        default:
            if (g_game_info.feature_flags.bits.powerbars_locked == 1) {
                allowed = 0;
            } else {
                allowed = 1;
            }
            break;
        }
    } else {
        allowed = 0;
    }
    if (!allowed) {
        return;
    }
    p1_back = owned_screen_latch_object(&p1_pbar_back_item);
    p1_red = owned_screen_latch_object(&p1_pbar_red_item);
    p2_back = owned_screen_latch_object(&p2_pbar_back_item);
    p2_red = owned_screen_latch_object(&p2_pbar_red_item);
    if (p1_back == 0 || p2_back == 0 || p1_red == 0) {
        return;
    }
    if (p2_red == 0) {
        return;
    }

    p1_bar_back_start = p1_back->pfx2d->verts[1].x;
    p1_back->pfx2d->verts[1].x = p1_back->pfx2d->verts[2].x;
    p1_back->pfx2d->verts[0].x = p1_back->pfx2d->verts[1].x;
    p1_bar_red_start = p1_red->pfx2d->verts[1].x;
    p1_red->pfx2d->verts[1].x = p1_red->pfx2d->verts[2].x;
    p1_red->pfx2d->verts[0].x = p1_red->pfx2d->verts[1].x;
    p2_bar_back_start = p2_back->pfx2d->verts[2].x;
    p2_back->pfx2d->verts[2].x = p2_back->pfx2d->verts[1].x;
    p2_back->pfx2d->verts[3].x = p2_back->pfx2d->verts[2].x;
    p2_bar_red_start = p2_red->pfx2d->verts[2].x;
    p2_red->pfx2d->verts[2].x = p2_red->pfx2d->verts[1].x;
    p2_red->pfx2d->verts[3].x = p2_red->pfx2d->verts[2].x;

    for (screen_index = 0;
         pbar_hide_screen_items[screen_index] != 0; screen_index++) {
        latch = pbar_hide_screen_items[screen_index];
        screen = owned_screen_latch_object(latch);
        if (screen != 0) {
            screen->pfx2d->verts[0].a = 0;
            screen->pfx2d->verts[1].a = 0;
            screen->pfx2d->verts[2].a = 0;
            screen->pfx2d->verts[3].a = 0;
        }
    }
    for (string_index = 0;
         pbar_hide_string_items[string_index].latch != 0; string_index++) {
        string_item = &pbar_hide_string_items[string_index];
        string = string_latch_object(string_item->latch);
        if (string != 0) {
            set_string_obj_alpha(string, 0.0f);
        }
    }
    f_powerbars_retracted = 1;
}

void pbar_force_pb_setting_with_offset(unsigned int player, float offset) {
    if (player == 0) {
        f_p1_force_adjustment = 1;
        g_game_info.plyr0.field_0C += offset;
        if (g_game_info.plyr0.field_0C > 1.0f) {
            g_game_info.plyr0.field_0C = 1.0f;
        }
    } else {
        f_p2_force_adjustment = 1;
        g_game_info.plyr1.field_0C += offset;
        if (g_game_info.plyr1.field_0C > 1.0f) {
            g_game_info.plyr1.field_0C = 1.0f;
        }
    }
}

static inline void start_powerbar_monitor_impl(void) {
    MkHdr* pdata;
    MkProc* proc;

    if (find_mkproc_pid(0x2003) == 0) {
        pwr_bar_proc_item.object = 0;
        pwr_bar_proc_item.instance = 0;
        proc = create_mkproc_fx(0x2003, p_power_bar_proc, &pdata);
        if (proc != 0) {
            pwr_bar_proc_item.object = proc;
            pwr_bar_proc_item.instance = proc->instance;
        }
    }
}

void start_powerbar_monitor(void) {
    start_powerbar_monitor_impl();
}

/* Soft ceiling: 96.06% -- four latch branches and one pdata reload only. */
float p_extend_powerbars(void) {
    ScreenObj* p1_back = owned_screen_latch_object(&p1_pbar_back_item);
    ScreenObj* p1_red = owned_screen_latch_object(&p1_pbar_red_item);
    ScreenObj* p2_back = owned_screen_latch_object(&p2_pbar_back_item);
    ScreenObj* p2_red = owned_screen_latch_object(&p2_pbar_red_item);
    MkHdr* pdata;

    if (p1_back == 0 || p2_back == 0 || p1_red == 0 || p2_red == 0) {
        return -1.0f;
    }
    f_powerbars_retracted = 0;
    bar_speed = 0.12244897f;
    if (p1_back->pfx2d->verts[0].x > p1_bar_back_start) {
        p1_back->pfx2d->verts[0].x -= 30.0f;
        p1_back->pfx2d->verts[1].x = p1_back->pfx2d->verts[0].x;
        if (p1_back->pfx2d->verts[0].x < p1_bar_back_start) {
            p1_back->pfx2d->verts[0].x = p1_bar_back_start;
            p1_back->pfx2d->verts[1].x = p1_back->pfx2d->verts[0].x;
        }
    }
    if (p2_back->pfx2d->verts[2].x < p2_bar_back_start) {
        p2_back->pfx2d->verts[2].x += 30.0f;
        p2_back->pfx2d->verts[3].x = p2_back->pfx2d->verts[2].x;
        if (p2_back->pfx2d->verts[2].x > p2_bar_back_start) {
            p2_back->pfx2d->verts[2].x = p2_bar_back_start;
            p2_back->pfx2d->verts[3].x = p2_back->pfx2d->verts[2].x;
        }
    }
    if (p1_red->pfx2d->verts[0].x > p1_bar_red_start) {
        p1_red->pfx2d->verts[0].x -= 30.0f;
        p1_red->pfx2d->verts[1].x = p1_red->pfx2d->verts[0].x;
        if (p1_red->pfx2d->verts[0].x < p1_bar_red_start) {
            p1_red->pfx2d->verts[0].x = p1_bar_red_start;
            p1_red->pfx2d->verts[1].x = p1_red->pfx2d->verts[0].x;
        }
    }
    if (p2_red->pfx2d->verts[2].x < p2_bar_red_start) {
        p2_red->pfx2d->verts[2].x += 30.0f;
        p2_red->pfx2d->verts[3].x = p2_red->pfx2d->verts[2].x;
        if (p2_red->pfx2d->verts[2].x > p2_bar_red_start) {
            p2_red->pfx2d->verts[2].x = p2_bar_red_start;
            p2_red->pfx2d->verts[3].x = p2_red->pfx2d->verts[2].x;
        }
    }
    if (p1_back->pfx2d->verts[0].x == p1_bar_back_start &&
        p2_back->pfx2d->verts[2].x == p2_bar_back_start &&
        p1_red->pfx2d->verts[0].x == p1_bar_red_start &&
        p2_red->pfx2d->verts[2].x == p2_bar_red_start) {
        if (_create_mkproc_generic_nostack(
                0x2094, 0x1F, p_unhide_pbar_items, 0x28, &pdata) != 0) {
            ((PbarExtendPdata*)pdata)->active = 0;
            shake_camera(3, pdata, 0.01f);
            snd_req(0xD9C);
            bar_speed = 0.01f;
        }
        return -1.0f;
    }
    return 1.0f;
}

/* Soft ceiling: 97.68% -- two redundant latch-branch emissions only. */
void extend_powerbars(void) {
    MkHdr* pdata;
    int allowed;

    if (g_game_info.pselect.field_1f4 == 1) {
        switch (mode_of_play) {
        case 4:
        case 8:
            allowed = 0;
            break;
        default:
            if (g_game_info.feature_flags.bits.powerbars_locked == 1) {
                allowed = 0;
            } else {
                allowed = 1;
            }
            break;
        }
    } else {
        allowed = 0;
    }
    if (allowed && f_powerbars_retracted != 0 &&
        _create_mkproc_generic_nostack(
            0x2094, 0x1F, p_extend_powerbars, 0x28, &pdata) != 0) {
        ((PbarExtendPdata*)pdata)->active = 0;
        set_latched_quad_alpha(&p1_pbar_backb_item);
        set_latched_quad_alpha(&p2_pbar_backb_item);
        snd_req(0xD9B);
    }
}

/* Soft ceiling: 97.38% -- two inlined latch branch emissions only. */
float p_move_pbars_off_screen(void) {
    ScreenLatch* latch;
    ScreenObj* screen;
    StringObj* string;
    int frame;
    int screen_index;
    int medal_index;
    int string_index;

    for (frame = 0; frame < 20; frame++) {
        for (screen_index = 0;
             pbar_item_list[screen_index] != 0; screen_index++) {
            latch = pbar_item_list[screen_index];
            screen = latch->object;
            if (screen != 0) {
                if ((unsigned int)screen->instance == latch->instance) {
                    /* Keep the live object. */
                } else {
                    screen = 0;
                }
            } else {
                screen = 0;
            }
            if (screen != 0) {
                screen->y += 6;
            }
        }
        for (medal_index = 0; medal_index < 8; medal_index++) {
            screen = medal_objs[medal_index];
            if (screen != 0) {
                screen->y += 6;
            }
        }
        for (string_index = 0;
             pbar_string_item_list[string_index] != 0; string_index++) {
            latch = pbar_string_item_list[string_index];
            string = (StringObj*)latch->object;
            if (string != 0) {
                if ((unsigned int)string->instance == latch->instance) {
                    /* Keep the live object. */
                } else {
                    string = 0;
                }
            } else {
                string = 0;
            }
            if (string != 0) {
                string->render_y += 6;
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        ((PwrbarProcVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

static void latch_screen(ScreenLatch* latch, ScreenObj* object) {
    latch->object = object;
    latch->instance = object != 0 ? object->instance : 0;
}

static void brighten_screen(ScreenObj* object, int enabled) {
    if (enabled) {
        if (object->pfx2d->verts[0].a < 0xD7) {
            object->pfx2d->verts[0].a += 0x28;
            object->pfx2d->verts[1].a += 0x28;
            object->pfx2d->verts[2].a += 0x28;
            object->pfx2d->verts[3].a += 0x28;
        } else {
            object->pfx2d->verts[0].a = 0xFF;
            object->pfx2d->verts[1].a = 0xFF;
            object->pfx2d->verts[2].a = 0xFF;
            object->pfx2d->verts[3].a = 0xFF;
        }
    } else {
        if (object->pfx2d->verts[0].a > 0x28) {
            object->pfx2d->verts[0].a -= 0x28;
            object->pfx2d->verts[1].a -= 0x28;
            object->pfx2d->verts[2].a -= 0x28;
            object->pfx2d->verts[3].a -= 0x28;
        } else {
            object->pfx2d->verts[0].a = 0;
            object->pfx2d->verts[1].a = 0;
            object->pfx2d->verts[2].a = 0;
            object->pfx2d->verts[3].a = 0;
        }
    }
}

/* Soft ceiling: 95.22% -- local/register layout and one redundant state store. */
void init_fighting_state_lights(void) {
    FightingLightState* player_1_state =
        fighting_light_state(&g_game_info.plyr0);
    FightingLightState* player_2_state =
        fighting_light_state(&g_game_info.plyr1);
    int player_index;

    for (player_index = 0; player_index < 2; player_index++) {
        FightingLightState* state =
            player_index == 0 ? player_1_state : player_2_state;
        ScreenObj* base;
        ScreenObj* light;
        int flags = 0;

        state->flags_word = 0;
        if (player_index == 0) {
            base = load_2d_pfxobj(
                0x10005, 0x2027, (char*)0x2002B, flags, 0x28);
            if (base != 0) {
                base->x = (screen_width - 0x280) / 2 + 0xAB;
                base->y = 0x18D;
            }
            state->base.object = base;
            state->base.instance = base->instance;
        } else {
            flags = (unsigned char)flags | 0x20;
            base = load_2d_pfxobj(
                0x10005, 0x2027, (char*)0x2002B, flags, 0x28);
            if (base != 0) {
                base->x = screen_width - base->pfx2d->tex_w - 0xAC -
                          (screen_width - 0x280) / 2;
                base->y = 0x18D;
            }
            state->base.object = base;
            state->base.instance = base->instance;
        }

        light = load_2d_pfxobj(
            0x10005, 0x2028, (char*)0x20028,
            base->flags, 0x25);
        if (light != 0) {
            if (player_index == 0) {
                light->x = (screen_width - 0x280) / 2 + 0xAB;
                light->y = 0x18D;
            } else {
                light->x = screen_width - light->pfx2d->tex_w - 0xAB -
                           (screen_width - 0x280) / 2;
                light->y = 0x18D;
            }
            light->pfx2d->verts[0].a += 0x28;
            light->pfx2d->verts[1].a += 0x28;
            light->pfx2d->verts[2].a += 0x28;
            light->pfx2d->verts[3].a += 0x28;
            state->red.object = light;
            state->red.instance = light->instance;
        }

        light = load_2d_pfxobj(
            0x10005, 0x2029, (char*)0x20029,
            light->flags, 0x26);
        if (light != 0) {
            if (player_index == 0) {
                light->x = (screen_width - 0x280) / 2 + 0xAB;
                light->y = 0x18D;
            } else {
                light->x = screen_width - light->pfx2d->tex_w - 0xAB -
                           (screen_width - 0x280) / 2;
                light->y = 0x18D;
            }
            light->pfx2d->verts[0].a += 0x28;
            light->pfx2d->verts[1].a += 0x28;
            light->pfx2d->verts[2].a += 0x28;
            light->pfx2d->verts[3].a += 0x28;
            state->green.object = light;
            state->green.instance = light->instance;
        }

        light = load_2d_pfxobj(
            0x10005, 0x202A, (char*)0x2002A,
            light->flags, 0x26);
        if (light != 0) {
            if (player_index == 0) {
                light->x = (screen_width - 0x280) / 2 + 0xAB;
                light->y = 0x18D;
            } else {
                light->x = screen_width - light->pfx2d->tex_w - 0xAB -
                           (screen_width - 0x280) / 2;
                light->y = 0x18D;
            }
            light->pfx2d->verts[0].a += 0x28;
            light->pfx2d->verts[1].a += 0x28;
            light->pfx2d->verts[2].a += 0x28;
            light->pfx2d->verts[3].a += 0x28;
            state->airborne.object = light;
            state->airborne.instance = light->instance;
        }
    }
    if (find_mkproc_pid(0x2093) == 0) {
        _create_mkproc_generic_nostack(
            0x2093, 0x1F, p_update_fighting_state_lights, 0, 0);
    }
}

/* Soft ceiling: 90.92% -- register allocation and latch branch placement. */
float p_update_fighting_state_lights(void) {
    PlyrInfo* player_1 = &g_game_info.plyr0;
    PlyrInfo* player_2 = &g_game_info.plyr1;
    FightingLightState* player_1_state = fighting_light_state(player_1);
    FightingLightState* player_2_state = fighting_light_state(player_2);
    FightingLightState* state;
    ScreenObj* light;
    PlyrInfo* player;
    PlyrPdata* pdata;
    int active;
    int player_index;

    for (player_index = 0; player_index < 2; player_index++) {
        player = player_index == 0 ? player_1 : player_2;
        state = fighting_light_state(player);

        if (player->slot.mirror_a == 0) {
            break;
        }
        pdata = player->slot.pdata;
        if (pdata->blocking_disabled != 0) {
            active = 1;
        } else if (pdata->blocking_disable_tick_1 > game_tick_ctr) {
            active = 1;
        } else {
            active = 0;
        }
        if (active != 0) {
            state->red_active = 1;
        } else {
            state->red_active = 0;
        }
        if (player->controller_slot == 0) {
            if (player_1_state->green_trigger) {
                active = 1;
            } else {
                active = 0;
            }
        } else if (player->controller_slot == 1) {
            if (player_2_state->green_trigger) {
                active = 1;
            } else {
                active = 0;
            }
        } else {
            active = 0;
        }
        if (active != 0) {
            state->green_active = 1;
        } else {
            state->green_active = 0;
        }
        if (pdata->blocking_disabled_2 != 0) {
            active = 1;
        } else if (pdata->blocking_disable_tick_2 > game_tick_ctr) {
            active = 1;
        } else if (is_plyr_airborn(player->slot.mirror_a) != 0) {
            active = 1;
        } else {
            active = 0;
        }
        if (active != 0) {
            state->airborne_active = 1;
        } else {
            state->airborne_active = 0;
        }
    }

    for (player_index = 0; player_index < 2; player_index++) {
        state = player_index == 0 ? player_1_state : player_2_state;

        light = owned_screen_latch_object(&state->red);
        if (light == 0) {
            return -1.0f;
        }
        brighten_screen(light, state->red_active);

        light = owned_screen_latch_object(&state->green);
        if (light == 0) {
            return -1.0f;
        }
        brighten_screen(light, state->green_active);

        light = owned_screen_latch_object(&state->airborne);
        if (light == 0) {
            return -1.0f;
        }
        brighten_screen(light, state->airborne_active);
    }
    return 1.0f;
}

int adjust_player_life(int player_index, float amount) {
    int depleted;

    switch (player_index) {
    case 0:
        depleted = 0;
        if ((mode_of_play == 10 || mode_of_play == 0 || mode_of_play == 1) &&
            (g_game_info.flags & 0x20) == 0) {
            if (g_game_info.plyr0.field_0C <= 0.0f) {
                depleted = 1;
            }
        } else {
            if (amount == -1.0f) {
                g_game_info.plyr0.field_0C = 0.0f;
            } else if ((g_game_info.field_04 & 0x20) == 0) {
                g_game_info.plyr0.field_0C += amount;
            }
            if (amount > 0.0f) {
                if (g_game_info.plyr0.field_0C > 1.0f) {
                    g_game_info.plyr0.field_0C = 1.0f;
                }
            } else if (g_game_info.plyr0.field_0C <= 0.0f) {
                if (mode_of_play == 4) {
                    g_game_info.plyr0.field_0C = 1.0f;
                } else {
                    g_game_info.plyr0.field_0C = 0.0f;
                    depleted = 1;
                }
            }
            if (mode_of_play == 4) {
                display_debug_damage(&g_game_info.plyr0);
            }
        }
        return depleted;
    case 1:
        depleted = 0;
        if ((mode_of_play == 10 || mode_of_play == 0 || mode_of_play == 1) &&
            (g_game_info.flags & 0x20) == 0) {
            /* Retail checks player one's life in this invulnerability path. */
            if (g_game_info.plyr0.field_0C <= 0.0f) {
                depleted = 1;
            }
        } else {
            if (amount == -1.0f) {
                g_game_info.plyr1.field_0C = 0.0f;
            } else if ((g_game_info.field_04 & 0x20) == 0) {
                g_game_info.plyr1.field_0C += amount;
            }
            if (amount > 0.0f) {
                if (g_game_info.plyr1.field_0C > 1.0f) {
                    g_game_info.plyr1.field_0C = 1.0f;
                }
            } else if (g_game_info.plyr1.field_0C <= 0.0f) {
                if (mode_of_play == 4) {
                    g_game_info.plyr1.field_0C = 1.0f;
                } else {
                    g_game_info.plyr1.field_0C = 0.0f;
                    depleted = 1;
                }
            }
            if (mode_of_play == 4) {
                display_debug_damage(&g_game_info.plyr1);
            }
        }
        return depleted;
    default:
        return 0;
    }
}

int adjust_p2_life(float amount) {
    int depleted = 0;

    if ((mode_of_play == 10 || mode_of_play == 0 || mode_of_play == 1) &&
        (g_game_info.flags & 0x20) == 0) {
        return g_game_info.plyr0.field_0C <= 0.0f;
    }
    if (amount == -1.0f) {
        g_game_info.plyr1.field_0C = 0.0f;
    } else if ((g_game_info.field_04 & 0x20) == 0) {
        g_game_info.plyr1.field_0C += amount;
    }
    if (amount > 0.0f) {
        if (g_game_info.plyr1.field_0C > 1.0f) {
            g_game_info.plyr1.field_0C = 1.0f;
        }
    } else if (g_game_info.plyr1.field_0C <= 0.0f) {
        if (mode_of_play == 4) {
            g_game_info.plyr1.field_0C = 1.0f;
        } else {
            g_game_info.plyr1.field_0C = 0.0f;
            depleted = 1;
        }
    }
    if (mode_of_play == 4) {
        display_debug_damage(&g_game_info.plyr1);
    }
    return depleted;
}

int adjust_p1_life(float amount) {
    int depleted = 0;

    if ((mode_of_play == 10 || mode_of_play == 0 || mode_of_play == 1) &&
        (g_game_info.flags & 0x20) == 0) {
        return g_game_info.plyr0.field_0C <= 0.0f;
    }
    if (amount == -1.0f) {
        g_game_info.plyr0.field_0C = 0.0f;
    } else if ((g_game_info.field_04 & 0x20) == 0) {
        g_game_info.plyr0.field_0C += amount;
    }
    if (amount > 0.0f) {
        if (g_game_info.plyr0.field_0C > 1.0f) {
            g_game_info.plyr0.field_0C = 1.0f;
        }
    } else if (g_game_info.plyr0.field_0C <= 0.0f) {
        if (mode_of_play == 4) {
            g_game_info.plyr0.field_0C = 1.0f;
        } else {
            g_game_info.plyr0.field_0C = 0.0f;
            depleted = 1;
        }
    }
    if (mode_of_play == 4) {
        display_debug_damage(&g_game_info.plyr0);
    }
    return depleted;
}

static void update_power_bar_verts(void);
static void update_combo_break_counts(void);

static ScreenObj* screen_latch_object(ScreenLatch* latch) {
    ScreenObj* object = latch->object;

    if (object == 0 || (unsigned int)object->instance != latch->instance) {
        return 0;
    }
    return object;
}

/* Soft ceiling: 98.58% -- latch branch direction and pool labels only. */
static void update_power_bar_verts(void) {
    ScreenObj* player1;
    ScreenObj* player2;
    float life;
    float fill;

    player1 = p1_pbar_item.object;
    if (player1 != 0) {
        if ((unsigned int)player1->instance == p1_pbar_item.instance) {
            /* Keep the live object. */
        } else {
            player1 = 0;
        }
    } else {
        player1 = 0;
    }
    player2 = p2_pbar_item.object;
    if (player2 != 0) {
        if ((unsigned int)player2->instance == p2_pbar_item.instance) {
            /* Keep the live object. */
        } else {
            player2 = 0;
        }
    } else {
        player2 = 0;
    }
    if (player1 == 0 || player2 == 0) {
        return;
    }
    life = p1_disp_life;
    if (life < 0.5f) {
        life += 0.02f;
    }
    fill = 262.0f * life;
    player1->pfx2d->verts[0].x =
        (262.0f + (24.0f + (float)BAR_BACK_X)) - fill;
    player1->pfx2d->verts[0].y = 417.0f;
    if (p1_disp_life > 0.0f) {
        player1->pfx2d->verts[1].x =
            ((262.0f + (24.0f + (float)BAR_BACK_X)) - 7.0f) - fill;
    } else {
        player1->pfx2d->verts[1].x =
            262.0f + (24.0f + (float)BAR_BACK_X);
    }
    player1->pfx2d->verts[1].y = 431.0f;
    player1->pfx2d->verts[2].x =
        262.0f + (24.0f + (float)BAR_BACK_X);
    player1->pfx2d->verts[2].y = 431.0f;
    player1->pfx2d->verts[3].x =
        262.0f + (24.0f + (float)BAR_BACK_X);
    player1->pfx2d->verts[3].y = 417.0f;
    player1->pfx2d->mirror = 1;

    life = p2_disp_life;
    if (life < 0.5f) {
        life += 0.02f;
    }
    player2->pfx2d->verts[0].x =
        (float)screen_width -
        (262.0f + (24.0f + (float)BAR_BACK_X));
    player2->pfx2d->verts[0].y = 417.0f;
    player2->pfx2d->verts[1].x =
        (float)screen_width -
        (262.0f + (24.0f + (float)BAR_BACK_X));
    player2->pfx2d->verts[1].y = 431.0f;
    if (p2_disp_life > 0.0f) {
        player2->pfx2d->verts[2].x =
            (float)screen_width -
            (((262.0f + (24.0f + (float)BAR_BACK_X)) - 7.0f) -
             (262.0f * life));
    } else {
        player2->pfx2d->verts[2].x =
            (float)screen_width -
            (262.0f + (24.0f + (float)BAR_BACK_X));
    }
    player2->pfx2d->verts[2].y = 431.0f;
    player2->pfx2d->verts[3].x =
        1.0f +
        ((float)screen_width -
         ((262.0f + (24.0f + (float)BAR_BACK_X)) -
          (262.0f * life)));
    player2->pfx2d->verts[3].y = 417.0f;
    player2->pfx2d->mirror = 1;
}

static ScreenObj* ensure_combo_bolt(
    ScreenLatch* latch, int player_index, int x_offset,
    PlyrInfo* player, int threshold) {
    ScreenObj* object = screen_latch_object(latch);

    if (object == 0) {
        object = load_2d_pfxobj(
            0x10005, 0x2017, (char*)0x20019, 0, 0x1B);
        if (object != 0) {
            if (player_index == 0) {
                object->x = BAR_BACK_X + x_offset;
            } else {
                object->x = screen_width - (BAR_BACK_X + x_offset);
            }
            object->y = 0x18C;
            latch->object = object;
            latch->instance = object->instance;
        }
    }
    if (object != 0) {
        if (player->slot.pdata->breaker_strength > threshold) {
            object->flag_bits.hidden = 0;
        } else {
            object->flag_bits.hidden = 1;
        }
    }
    return object;
}

/* Soft ceiling: 93.41% -- six shortened inlined latch diamonds only. */
static void update_combo_break_counts(void) {
    if (g_game_info.plyr0.slot.pdata->breaker_strength !=
        p1_last_combo_break_count) {
        ensure_combo_bolt(
            &p1_bolt_1_item, 0, 0x45,
            &g_game_info.plyr0, 0);
        ensure_combo_bolt(
            &p1_bolt_2_item, 0, 0x57,
            &g_game_info.plyr0, 1);
        ensure_combo_bolt(
            &p1_bolt_3_item, 0, 0x69,
            &g_game_info.plyr0, 2);
        p1_last_combo_break_count =
            g_game_info.plyr0.slot.pdata->breaker_strength;
    }
    if (g_game_info.plyr1.slot.pdata->breaker_strength !=
        p2_last_combo_break_count) {
        ensure_combo_bolt(
            &p2_bolt_1_item, 1, 0x55,
            &g_game_info.plyr1, 0);
        ensure_combo_bolt(
            &p2_bolt_2_item, 1, 0x67,
            &g_game_info.plyr1, 1);
        ensure_combo_bolt(
            &p2_bolt_3_item, 1, 0x79,
            &g_game_info.plyr1, 2);
        p2_last_combo_break_count =
            g_game_info.plyr1.slot.pdata->breaker_strength;
    }
}

static inline void update_plyr_medals_impl(void) {
    int object_index;
    int medal_index;
    int x;
    int i;

    if (mode_of_play == 8 && trial_show_standard_fight_messages() == 0) {
        return;
    }
    for (i = 0; i < 8; i++) {
        if (medal_objs[i] != 0) {
            destroy_screen_obj(medal_objs[i]);
            medal_objs[i] = 0;
        }
    }

    x = BAR_BACK_X + 0x100;
    object_index = 0;
    for (medal_index = 0;
         medal_index < g_game_info.plyr0.field_40; medal_index++) {
        medal_objs[object_index] = load_2d_pfxobj(
            0x10005, 0x2022, (char*)0x20018, 0, 0x16);
        if (medal_objs[object_index] != 0) {
            medal_objs[object_index]->x = x;
            medal_objs[object_index]->y = 0x177;
            if (g_game_info.plyr0.controller_slot == 0) {
                x -= 0x18;
            } else {
                x += 0x18;
            }
            object_index++;
        }
    }

    x = screen_width - (BAR_BACK_X + 0x120);
    object_index = 3;
    for (medal_index = 0;
         medal_index < g_game_info.plyr1.field_40; medal_index++) {
        medal_objs[object_index] = load_2d_pfxobj(
            0x10005, 0x2022, (char*)0x20018, 0, 0x16);
        if (medal_objs[object_index] != 0) {
            medal_objs[object_index]->x = x;
            medal_objs[object_index]->y = 0x177;
            if (g_game_info.plyr1.controller_slot == 0) {
                x -= 0x18;
            } else {
                x += 0x18;
            }
            object_index++;
        }
    }
}

/* Soft ceiling: 98.16% -- loop-local register allocation only. */
void update_plyr_medals(void) {
    update_plyr_medals_impl();
}

/*
 * Soft ceiling: 89.84% -- repeated latch diamonds and player-state register
 * allocation only; object destruction order and conditions match retail.
 */
void destroy_pwr_bars(void) {
    MkProc* process;
    ScreenLatch* latch;
    ScreenObj* screen;
    StringObj* string;
    FightingLightState* state;
    FightingLightState* player_1_state;
    FightingLightState* player_2_state;
    int player_index;
    int screen_index;
    int string_index;
    int i;

    process = pwr_bar_proc_item.object;
    if (process != 0) {
        if ((unsigned int)process->instance == pwr_bar_proc_item.instance) {
            /* Keep the live process. */
        } else {
            process = 0;
        }
    } else {
        process = 0;
    }
    if (process != 0 && process->instance != 0) {
        ((PwrbarProcVtable*)process->vtbl)->destroy();
    }
    for (screen_index = 0;
         pbar_item_list[screen_index] != 0; screen_index++) {
        latch = pbar_item_list[screen_index];
        screen = owned_screen_latch_object(latch);
        if (screen != 0 && screen->instance != 0) {
            screen->vtbl->destroy();
        }
    }
    delete_screen_obj_oid(0x2015);
    for (string_index = 0;
         pbar_string_item_list[string_index] != 0; string_index++) {
        latch = pbar_string_item_list[string_index];
        string = string_latch_object(latch);
        if (string != 0 && string->instance != 0) {
            string->vtbl->destroy();
        }
    }
    for (i = 0; i < 8; i++) {
        medal_objs[i] = 0;
    }
    f_powerbars_retracted = 0;
    player_1_state = fighting_light_state(&g_game_info.plyr0);
    player_2_state = fighting_light_state(&g_game_info.plyr1);

    for (player_index = 0; player_index < 2; player_index++) {
        state = player_index == 0 ? player_1_state : player_2_state;
        state->flags_word = 0;

        screen = owned_screen_latch_object(&state->base);
        if (screen != 0 && screen->instance != 0) {
            screen->vtbl->destroy();
        }
        screen = owned_screen_latch_object(&state->red);
        if (screen != 0 && screen->instance != 0) {
            screen->vtbl->destroy();
        }
        screen = owned_screen_latch_object(&state->green);
        if (screen != 0 && screen->instance != 0) {
            screen->vtbl->destroy();
        }
        screen = owned_screen_latch_object(&state->airborne);
        if (screen != 0 && screen->instance != 0) {
            screen->vtbl->destroy();
        }
        destroy_mkprocs_pid(0x2093);
    }
}

static void clear_screen_latch(ScreenLatch* latch) {
    latch->object = 0;
    latch->instance = 0;
}

void init_pwr_bars(void) {
    ScreenObj* object;
    ScreenObj* back;
    StringObj* name;
    int p2_flags = 0;
    int i;

    clear_screen_latch(&p1_name_item);
    clear_screen_latch(&p2_name_item);
    clear_screen_latch(&p1_pbar_item);
    clear_screen_latch(&p2_pbar_item);
    clear_screen_latch(&p1_pbar_back_item);
    clear_screen_latch(&p2_pbar_back_item);
    clear_screen_latch(&p1_pbar_red_item);
    clear_screen_latch(&p2_pbar_red_item);
    clear_screen_latch(&p1_pbar_backb_item);
    clear_screen_latch(&p2_pbar_backb_item);
    clear_screen_latch(&p1_bar_icon_item);
    clear_screen_latch(&p2_bar_icon_item);
    clear_screen_latch(&pbar_cntr_item);
    clear_screen_latch(&pbar_cntr_dragon_item);
    clear_screen_latch(&p1_bolt_1_item);
    clear_screen_latch(&p1_bolt_2_item);
    clear_screen_latch(&p1_bolt_3_item);
    clear_screen_latch(&p2_bolt_1_item);
    clear_screen_latch(&p2_bolt_2_item);
    clear_screen_latch(&p2_bolt_3_item);

    p1_disp_life = 0.0f;
    p2_disp_life = 0.0f;
    f_p1_warning_given = 0;
    f_p2_warning_given = 0;
    f_powerbars_retracted = 0;
    p1_last_combo_break_count = -1;
    p2_last_combo_break_count = -1;
    if (mode_of_play != 10) {
        f_p1_force_adjustment = 0;
        f_p2_force_adjustment = 0;
    } else {
        f_p1_force_adjustment = 1;
        f_p2_force_adjustment = 1;
        g_game_info.plyr0.field_0C = g_game_info.plyr0.field_10;
        g_game_info.plyr1.field_0C = g_game_info.plyr1.field_10;
    }

    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20012, 0, 0x19);
    if (object != 0) {
        p1_pbar_item.object = object;
        p1_pbar_item.instance = object->instance;
        object->draw_flags.on = 0;
    }
    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20015, 0, 0x1A);
    if (object != 0) {
        object->pfx2d->verts[0].x =
            (262.0f + (24.0f + (float)BAR_BACK_X)) - 262.0f;
        object->pfx2d->verts[0].y = 417.0f;
        object->pfx2d->verts[1].x =
            ((262.0f + (24.0f + (float)BAR_BACK_X)) - 7.0f) - 262.0f;
        object->pfx2d->verts[1].y = 431.0f;
        object->pfx2d->verts[2].x =
            262.0f + (24.0f + (float)BAR_BACK_X);
        object->pfx2d->verts[2].y = 431.0f;
        object->pfx2d->verts[3].x =
            262.0f + (24.0f + (float)BAR_BACK_X);
        object->pfx2d->verts[3].y = 417.0f;
        object->draw_flags.on = 0;
        p1_pbar_red_item.object = object;
        p1_pbar_red_item.instance = object->instance;
    }

    p2_flags = (unsigned char)p2_flags | 0x20;
    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20012, p2_flags, 0x19);
    if (object != 0) {
        p2_pbar_item.object = object;
        p2_pbar_item.instance = object->instance;
        object->draw_flags.on = 0;
    }
    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20015, p2_flags, 0x1A);
    if (object != 0) {
        object->pfx2d->verts[0].x =
            (float)screen_width -
            (262.0f + (24.0f + (float)BAR_BACK_X));
        object->pfx2d->verts[0].y = 417.0f;
        object->pfx2d->verts[1].x =
            (float)screen_width -
            (262.0f + (24.0f + (float)BAR_BACK_X));
        object->pfx2d->verts[1].y = 431.0f;
        object->pfx2d->verts[2].x =
            (float)screen_width -
            (((262.0f + (24.0f + (float)BAR_BACK_X)) - 7.0f) - 262.0f);
        object->pfx2d->verts[2].y = 431.0f;
        object->pfx2d->verts[3].x =
            1.0f +
            ((float)screen_width -
             ((262.0f + (24.0f + (float)BAR_BACK_X)) - 262.0f));
        object->pfx2d->verts[3].y = 417.0f;
        object->draw_flags.on = 0;
        p2_pbar_red_item.object = object;
        p2_pbar_red_item.instance = object->instance;
    }

    back = load_2d_pfxobj(
        0x10005, 0x2015, (char*)0x20013, 0, 0x18);
    if (back != 0) {
        p1_pbar_back_item.object = back;
        p1_pbar_back_item.instance = back->instance;
        back->x = BAR_BACK_X;
        back->y = 0x189;
        object = load_2d_pfxobj(
            0x10005, 0x2015, (char*)0x20014, 0, 0x18);
        if (object != 0) {
            object->x = back->x + back->pfx2d->tex_w;
            object->y = 0x189;
            p1_pbar_backb_item.object = object;
            p1_pbar_backb_item.instance = object->instance;
        }
    }

    back = load_2d_pfxobj(
        0x10005, 0x2015, (char*)0x20013, p2_flags, 0x18);
    if (back != 0) {
        p2_pbar_back_item.object = back;
        p2_pbar_back_item.instance = back->instance;
        back->x = screen_width - (BAR_BACK_X + back->pfx2d->tex_w);
        back->y = 0x189;
        object = load_2d_pfxobj(
            0x10005, 0x2015, (char*)0x20014, 0, 0x18);
        if (object != 0) {
            object->x = back->x - (object->pfx2d->tex_w - 8);
            object->y = 0x189;
            p2_pbar_backb_item.object = object;
            p2_pbar_backb_item.instance = object->instance;
        }
    }

    name = string_left_xy(
        0x2016, 5,
        (const char*)global_player_data[g_game_info.plyr0.player_index].name,
        BAR_BACK_X + 0x23, 0x1A3, 0x17);
    if (name != 0) {
        name->visibility.hidden = 0;
        p1_name_item.object = (ScreenObj*)name;
        p1_name_item.instance = name->instance;
        object = load_named_2d_pfxobj_xy(
            0x3000B, 0x2050, "LILHEAD", 0,
            BAR_BACK_X + 0xC, 0x184, 0x1C);
        if (object != 0) {
            object->flag_bits.hidden = 0;
            p1_bar_icon_item.object = object;
            p1_bar_icon_item.instance = object->instance;
        }
    }

    name = string_right_xy(
        0x2016, 5,
        (const char*)global_player_data[g_game_info.plyr1.player_index].name,
        screen_width - (BAR_BACK_X + 0x23), 0x1A3, 0x17);
    if (name != 0) {
        name->visibility.hidden = 0;
        p2_name_item.object = (ScreenObj*)name;
        p2_name_item.instance = name->instance;
        object = load_named_2d_pfxobj_xy(
            0x4000B, 0x2051, "LILHEAD", p2_flags,
            screen_width - (BAR_BACK_X + 0x8C), 0x184, 0x1C);
        if (object != 0) {
            object->flag_bits.hidden = 0;
            p2_bar_icon_item.object = object;
            p2_bar_icon_item.instance = object->instance;
        }
    }

    update_power_bar_verts();
    if (is_timer_off() || (g_game_info.field_04 & 0x20) != 0 ||
        mode_of_play == 4) {
        object = load_2d_pfxobj(
            0x10005, 0x204F, (char*)0x20017, 0, 0x15);
        if (object != 0) {
            pull_screen_obj(object);
            insert_screen_obj(object);
            object->x = PB_CNTR_RING_X;
            object->y = 0x165;
            pbar_cntr_dragon_item.object = object;
            pbar_cntr_dragon_item.instance = object->instance;
        }
    } else {
        object = load_2d_pfxobj_xy(
            0x10005, 0x204F, (char*)0x20016, 0,
            PB_CNTR_RING_X, 0x165, 0x18);
        if (object != 0) {
            pull_screen_obj(object);
            insert_screen_obj(object);
            pbar_cntr_item.object = object;
            pbar_cntr_item.instance = object->instance;
        }
    }

    for (i = 0; i < 8; i++) {
        medal_objs[i] = 0;
    }
    update_plyr_medals_impl();
    start_powerbar_monitor_impl();
    init_fighting_state_lights();
    update_combo_break_counts();
    retract_power_bars();
}

/* Soft ceiling: 99.92% -- two identical 0.06f pool-label relocations differ. */
float p_power_bar_proc(void) {
    int changed;

    changed = 0;
    if (f_powerbars_retracted != 0) {
        return 1.0f;
    }
    if (f_p1_force_adjustment == 1) {
        f_p1_force_adjustment = 0;
        changed = 1;
        p1_disp_life = g_game_info.plyr0.field_0C;
    }
    if (f_p2_force_adjustment == 1) {
        f_p2_force_adjustment = 0;
        changed = 1;
        p2_disp_life = g_game_info.plyr1.field_0C;
    }

    if (p1_disp_life > g_game_info.plyr0.field_0C) {
        p1_disp_life -= bar_speed;
        if (p1_disp_life <= 0.06f && f_p1_warning_given == 0) {
            if (g_game_info.plyr0.field_0C > 0.0f &&
                !g_game_info.pause_flag_bits.fatality_window) {
                snd_req(0xDC2);
            }
            f_p1_warning_given = 1;
        }
        if (p1_disp_life < g_game_info.plyr0.field_0C) {
            p1_disp_life = g_game_info.plyr0.field_0C;
        }
        changed = 1;
    } else if (p1_disp_life < g_game_info.plyr0.field_0C) {
        p1_disp_life += bar_speed;
        if (p1_disp_life > g_game_info.plyr0.field_0C) {
            p1_disp_life = g_game_info.plyr0.field_0C;
        }
        changed = 1;
    }

    if (p2_disp_life > g_game_info.plyr1.field_0C) {
        p2_disp_life -= bar_speed;
        if (p2_disp_life <= 0.06f && f_p2_warning_given == 0) {
            if (g_game_info.plyr1.field_0C > 0.0f &&
                !g_game_info.pause_flag_bits.fatality_window) {
                snd_req(0xDC2);
            }
            f_p2_warning_given = 1;
        }
        if (p2_disp_life < g_game_info.plyr1.field_0C) {
            p2_disp_life = g_game_info.plyr1.field_0C;
        }
        changed = 1;
    } else if (p2_disp_life < g_game_info.plyr1.field_0C) {
        p2_disp_life += bar_speed;
        if (p2_disp_life > g_game_info.plyr1.field_0C) {
            p2_disp_life = g_game_info.plyr1.field_0C;
        }
        changed = 1;
    }
    if (changed != 0) {
        update_power_bar_verts();
    }
    update_combo_break_counts();
    return 1.0f;
}
