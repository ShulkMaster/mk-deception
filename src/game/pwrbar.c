#include "game/game_info.h"
#include "runtime/fonts.h"
#include "runtime/plyr_pdata.h"
#include "runtime/mk_pdata.h"
#include "runtime/image.h"

/* Retail TU-local; remaining pwrbar code owns the writes. */
extern int f_powerbars_retracted;
extern unsigned int game_tick_ctr;
extern int f_p1_force_adjustment;
extern int f_p2_force_adjustment;
extern int mode_of_play;
extern int f_p1_warning_given;
extern int f_p2_warning_given;
extern float p1_disp_life;
extern float p2_disp_life;
extern int screen_width;
extern int p1_last_combo_break_count;
extern int p2_last_combo_break_count;
extern int BAR_BACK_X;
extern int PB_CNTR_RING_X;
extern GlobalPlayerEntry global_player_data[];

typedef struct ScreenLatch {
    ScreenObj* object;
    unsigned int instance;
} ScreenLatch;

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

typedef struct PwrbarProcVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    MkVtblFn destroy;
    MkVtblFn dispatch;
    MkVtblFn sleep;
} PwrbarProcVtable;

typedef struct FightingLightState {
    unsigned int flags;
    ScreenLatch base;
    ScreenLatch red;
    ScreenLatch green;
    ScreenLatch airborne;
} FightingLightState;

extern ScreenLatch p1_pbar_item;
extern ProcLatch pwr_bar_proc_item;
extern ScreenLatch p2_pbar_item;
extern ScreenLatch p1_bolt_1_item;
extern ScreenLatch p1_bolt_2_item;
extern ScreenLatch p1_bolt_3_item;
extern ScreenLatch p2_bolt_1_item;
extern ScreenLatch p2_bolt_2_item;
extern ScreenLatch p2_bolt_3_item;
extern ScreenLatch p1_pbar_back_item;
extern ScreenLatch p1_pbar_red_item;
extern ScreenLatch p2_pbar_back_item;
extern ScreenLatch p2_pbar_red_item;
extern ScreenLatch p1_pbar_backb_item;
extern ScreenLatch p2_pbar_backb_item;
extern ScreenLatch p1_name_item;
extern ScreenLatch p2_name_item;
extern ScreenLatch p1_bar_icon_item;
extern ScreenLatch p2_bar_icon_item;
extern ScreenLatch pbar_cntr_item;
extern ScreenLatch pbar_cntr_dragon_item;
extern ScreenLatch* pbar_hide_screen_items[];
extern ScreenLatch* pbar_item_list[];
extern ScreenLatch* pbar_string_item_list[];
extern PbarHideStringItem pbar_hide_string_items[];
extern float p1_bar_back_start;
extern float p1_bar_red_start;
extern float p2_bar_back_start;
extern float p2_bar_red_start;

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
    return (FightingLightState*)player->pad1C;
}

static inline StringObj* string_latch_object(ScreenLatch* latch) {
    StringObj* object = (StringObj*)latch->object;

    if (object == 0 || object->instance != latch->instance) {
        return 0;
    }
    return object;
}

static inline ScreenObj* owned_screen_latch_object(ScreenLatch* latch) {
    ScreenObj* object = latch->object;

    if (object == 0 || (unsigned int)object->instance != latch->instance) {
        return 0;
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
                latch = (ScreenLatch*)g_game_info.plyr0.pad4C;
                latch->object = (ScreenObj*)string;
                latch->instance = string->instance;
            }
        } else {
            sprintf(text, get_string(5), g_game_info.plyr1.field_44);
            string = string_right_xy(
                0x201E, 0, text, screen_width - 0x14, 0x3D, 0x1D);
            if (string != 0) {
                latch = (ScreenLatch*)g_game_info.plyr1.pad4C;
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

int check_for_red_light(PlyrInfo* player) {
    PlyrPdata* pdata;

    pdata = player->slot.pdata;
    if (pdata->blocking_disabled != 0) {
        return 1;
    }
    return pdata->blocking_disable_tick_1 > game_tick_ctr;
}

int check_for_green_light(PlyrInfo* player) {
    PlyrPdata* pdata;

    pdata = player->slot.pdata;
    if (pdata->blocking_disabled_2 != 0) {
        return 1;
    }
    if (pdata->blocking_disable_tick_2 > game_tick_ctr) {
        return 1;
    }
    return is_plyr_airborn(player->slot.mirror_a) != 0;
}

float p_unhide_pbar_items(void) {
    PbarFadePdata* pdata = (PbarFadePdata*)apdata;
    PbarHideStringItem* string_item;
    ScreenLatch** item;
    ScreenObj* screen;
    StringObj* string;
    unsigned int alpha;

    alpha = (unsigned char)pdata->alpha;
    for (item = pbar_hide_screen_items; *item != 0; item++) {
        screen = owned_screen_latch_object(*item);
        if (screen != 0 && screen->pfx2d->verts[0].a < 0xFF) {
            owned_set_quad_alpha(screen, (unsigned char)alpha);
        }
    }
    for (string_item = pbar_hide_string_items;
         string_item->latch != 0; string_item++) {
        string = string_latch_object(string_item->latch);
        if (string != 0) {
            set_string_obj_alpha(
                string,
                (float)(alpha <= string_item->alpha
                    ? alpha : string_item->alpha));
        }
    }
    pdata->alpha += 8;
    if (pdata->alpha >= 0xFF) {
        return -1.0f;
    }
    return 1.0f;
}

void retract_power_bars(void) {
    ScreenObj* p1_back;
    ScreenObj* p1_red;
    ScreenObj* p2_back;
    ScreenObj* p2_red;
    ScreenLatch** item;
    PbarHideStringItem* string_item;
    StringObj* string;
    int allowed;

    allowed = g_game_info.pselect.field_1f4 == 1 &&
              mode_of_play != 4 && mode_of_play != 8 &&
              (g_game_info.field_04 & 0x20) == 0;
    if (!allowed) {
        return;
    }
    p1_back = owned_screen_latch_object(&p1_pbar_back_item);
    p1_red = owned_screen_latch_object(&p1_pbar_red_item);
    p2_back = owned_screen_latch_object(&p2_pbar_back_item);
    p2_red = owned_screen_latch_object(&p2_pbar_red_item);
    if (p1_back == 0 || p1_red == 0 || p2_back == 0 || p2_red == 0) {
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

    for (item = pbar_hide_screen_items; *item != 0; item++) {
        owned_set_quad_alpha(owned_screen_latch_object(*item), 0);
    }
    for (string_item = pbar_hide_string_items;
         string_item->latch != 0; string_item++) {
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

void start_powerbar_monitor(void) {
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

float p_extend_powerbars(void) {
    ScreenObj* p1_back = screen_latch_object(&p1_pbar_back_item);
    ScreenObj* p1_red = screen_latch_object(&p1_pbar_red_item);
    ScreenObj* p2_back = screen_latch_object(&p2_pbar_back_item);
    ScreenObj* p2_red = screen_latch_object(&p2_pbar_red_item);
    MkHdr* pdata;

    if (p1_back == 0 || p1_red == 0 || p2_back == 0 || p2_red == 0) {
        return 0.0f;
    }
    f_powerbars_retracted = 0;
    if (p1_back->pfx2d->verts[0].x > p1_bar_back_start) {
        p1_back->pfx2d->verts[0].x -= 30.0f;
        if (p1_back->pfx2d->verts[0].x < p1_bar_back_start) {
            p1_back->pfx2d->verts[0].x = p1_bar_back_start;
        }
        p1_back->pfx2d->verts[1].x = p1_back->pfx2d->verts[0].x;
    }
    if (p2_back->pfx2d->verts[2].x < p2_bar_back_start) {
        p2_back->pfx2d->verts[2].x += 30.0f;
        if (p2_back->pfx2d->verts[2].x > p2_bar_back_start) {
            p2_back->pfx2d->verts[2].x = p2_bar_back_start;
        }
        p2_back->pfx2d->verts[3].x = p2_back->pfx2d->verts[2].x;
    }
    if (p1_red->pfx2d->verts[0].x > p1_bar_red_start) {
        p1_red->pfx2d->verts[0].x -= 30.0f;
        if (p1_red->pfx2d->verts[0].x < p1_bar_red_start) {
            p1_red->pfx2d->verts[0].x = p1_bar_red_start;
        }
        p1_red->pfx2d->verts[1].x = p1_red->pfx2d->verts[0].x;
    }
    if (p2_red->pfx2d->verts[2].x < p2_bar_red_start) {
        p2_red->pfx2d->verts[2].x += 30.0f;
        if (p2_red->pfx2d->verts[2].x > p2_bar_red_start) {
            p2_red->pfx2d->verts[2].x = p2_bar_red_start;
        }
        p2_red->pfx2d->verts[3].x = p2_red->pfx2d->verts[2].x;
    }
    if (p1_back->pfx2d->verts[0].x == p1_bar_back_start &&
        p2_back->pfx2d->verts[2].x == p2_bar_back_start &&
        p1_red->pfx2d->verts[0].x == p1_bar_red_start &&
        p2_red->pfx2d->verts[2].x == p2_bar_red_start) {
        if (_create_mkproc_generic_nostack(
                0x2094, 0x1F, p_unhide_pbar_items, 0x28, &pdata) != 0) {
            shake_camera(3, pdata, 0.01f);
            snd_req(0xD9C);
        }
    }
    return 0.0f;
}

void extend_powerbars(void) {
    MkHdr* pdata;
    int allowed;

    allowed = g_game_info.pselect.field_1f4 == 1 &&
              mode_of_play != 4 && mode_of_play != 8 &&
              (g_game_info.field_04 & 0x20) == 0;
    if (allowed && f_powerbars_retracted != 0 &&
        _create_mkproc_generic_nostack(
            0x2094, 0x1F, p_extend_powerbars, 0x28, &pdata) != 0) {
        set_quad_alpha(
            screen_latch_object(&p1_pbar_backb_item), 0xFF);
        set_quad_alpha(
            screen_latch_object(&p2_pbar_backb_item), 0xFF);
        snd_req(0xD9B);
    }
}

float p_move_pbars_off_screen(void) {
    ScreenLatch** item;
    ScreenLatch* latch;
    ScreenObj* screen;
    ScreenObj** medal;
    StringObj* string;
    int frame;
    int i;

    for (frame = 0; frame < 20; frame++) {
        for (item = pbar_item_list; *item != 0; item++) {
            latch = *item;
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
        medal = medal_objs;
        i = 8;
        do {
            screen = *medal;
            if (screen != 0) {
                screen->y += 6;
            }
            medal++;
            i--;
        } while (i != 0);
        for (item = pbar_string_item_list; *item != 0; item++) {
            latch = *item;
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
    int alpha;

    if (object == 0 || object->pfx2d == 0) {
        return;
    }
    alpha = object->pfx2d->verts[0].a;
    if (enabled) {
        alpha += 0x28;
        if (alpha > 0xFF) {
            alpha = 0xFF;
        }
    } else {
        alpha -= 0x28;
        if (alpha < 0) {
            alpha = 0;
        }
    }
    set_quad_alpha(object, (unsigned char)alpha);
}

void init_fighting_state_lights(void) {
    int player_index;

    for (player_index = 0; player_index < 2; player_index++) {
        PlyrInfo* player =
            player_index == 0 ? &g_game_info.plyr0 : &g_game_info.plyr1;
        FightingLightState* state = fighting_light_state(player);
        ScreenObj* base;
        ScreenObj* light;
        int flags = player_index != 0 ? 0x20 : 0;
        int x;

        state->flags = 0;
        base = load_2d_pfxobj(
            0x10005, 0x2027, (char*)0x2002B, flags, 0x28);
        if (base != 0) {
            x = player_index == 0
                ? (screen_width - 0x280) / 2 + 0xAB
                : screen_width - base->pfx2d->tex_w - 0xAC -
                      (screen_width - 0x280) / 2;
            base->x = x;
            base->y = 0x18D;
        }
        latch_screen(&state->base, base);

        light = load_2d_pfxobj(
            0x10005, 0x2028, (char*)0x20028,
            base != 0 ? base->flags : flags, 0x25);
        if (light != 0) {
            light->x = player_index == 0
                ? (screen_width - 0x280) / 2 + 0xAB
                : screen_width - light->pfx2d->tex_w - 0xAB -
                      (screen_width - 0x280) / 2;
            light->y = 0x18D;
            set_quad_alpha(
                light, (unsigned char)(light->pfx2d->verts[0].a + 0x28));
        }
        latch_screen(&state->red, light);

        light = load_2d_pfxobj(
            0x10005, 0x2029, (char*)0x20029,
            light != 0 ? light->flags : flags, 0x26);
        if (light != 0) {
            light->x = player_index == 0
                ? (screen_width - 0x280) / 2 + 0xAB
                : screen_width - light->pfx2d->tex_w - 0xAB -
                      (screen_width - 0x280) / 2;
            light->y = 0x18D;
            set_quad_alpha(
                light, (unsigned char)(light->pfx2d->verts[0].a + 0x28));
        }
        latch_screen(&state->green, light);

        light = load_2d_pfxobj(
            0x10005, 0x202A, (char*)0x2002A,
            light != 0 ? light->flags : flags, 0x26);
        if (light != 0) {
            light->x = player_index == 0
                ? (screen_width - 0x280) / 2 + 0xAB
                : screen_width - light->pfx2d->tex_w - 0xAB -
                      (screen_width - 0x280) / 2;
            light->y = 0x18D;
            set_quad_alpha(
                light, (unsigned char)(light->pfx2d->verts[0].a + 0x28));
        }
        latch_screen(&state->airborne, light);
    }
    if (find_mkproc_pid(0x2093) == 0) {
        _create_mkproc_generic_nostack(
            0x2093, 0x1F, p_update_fighting_state_lights, 0, 0);
    }
}

float p_update_fighting_state_lights(void) {
    int player_index;

    for (player_index = 0; player_index < 2; player_index++) {
        PlyrInfo* player =
            player_index == 0 ? &g_game_info.plyr0 : &g_game_info.plyr1;
        FightingLightState* state = fighting_light_state(player);

        if (player->slot.mirror_a == 0) {
            continue;
        }
        if (check_for_red_light(player)) {
            state->flags |= 0x80;
        } else {
            state->flags &= ~0x80;
        }
        if (check_for_green_light(player)) {
            state->flags |= 0x20;
        } else {
            state->flags &= ~0x20;
        }
        brighten_screen(
            screen_latch_object(&state->red),
            (state->flags & 0x80) != 0);
        brighten_screen(
            screen_latch_object(&state->green),
            (state->flags & 0x40) != 0);
        brighten_screen(
            screen_latch_object(&state->airborne),
            (state->flags & 0x20) != 0);
    }
    return 0.0f;
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

#pragma dont_inline on
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
#pragma dont_inline reset

static ScreenObj* ensure_combo_bolt(
    ScreenLatch* latch, int x, int combo_count, int threshold) {
    ScreenObj* object = screen_latch_object(latch);

    if (object == 0) {
        object = load_2d_pfxobj(
            0x10005, 0x2017, (char*)0x20019, 0, 0x1B);
        if (object != 0) {
            object->x = x;
            object->y = 0x18C;
            latch->object = object;
            latch->instance = object->instance;
        }
    }
    if (object != 0) {
        if (combo_count > threshold) {
            object->flags &= ~0x10;
        } else {
            object->flags |= 0x10;
        }
    }
    return object;
}

static void update_combo_break_counts(void) {
    int count;

    count = g_game_info.plyr0.slot.pdata->breaker_strength;
    if (count != p1_last_combo_break_count) {
        ensure_combo_bolt(&p1_bolt_1_item, 0x4D, count, 0);
        ensure_combo_bolt(&p1_bolt_2_item, 0x5F, count, 1);
        ensure_combo_bolt(&p1_bolt_3_item, 0x71, count, 2);
        p1_last_combo_break_count = count;
    }
    count = g_game_info.plyr1.slot.pdata->breaker_strength;
    if (count != p2_last_combo_break_count) {
        ensure_combo_bolt(
            &p2_bolt_1_item, screen_width - 0x5D, count, 0);
        ensure_combo_bolt(
            &p2_bolt_2_item, screen_width - 0x6F, count, 1);
        ensure_combo_bolt(
            &p2_bolt_3_item, screen_width - 0x81, count, 2);
        p2_last_combo_break_count = count;
    }
}

void update_plyr_medals(void) {
    ScreenObj* medal;
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
        medal = load_2d_pfxobj(
            0x10005, 0x2022, (char*)0x20018, 0, 0x16);
        medal_objs[object_index] = medal;
        if (medal != 0) {
            medal->x = x;
            medal->y = 0x177;
            x += g_game_info.plyr0.controller_slot == 0 ? -0x18 : 0x18;
            object_index++;
        }
    }

    x = screen_width - (BAR_BACK_X + 0x120);
    object_index = 3;
    for (medal_index = 0;
         medal_index < g_game_info.plyr1.field_40; medal_index++) {
        medal = load_2d_pfxobj(
            0x10005, 0x2022, (char*)0x20018, 0, 0x16);
        medal_objs[object_index] = medal;
        if (medal != 0) {
            medal->x = x;
            medal->y = 0x177;
            x += g_game_info.plyr1.controller_slot == 0 ? -0x18 : 0x18;
            object_index++;
        }
    }
}

void destroy_pwr_bars(void) {
    MkProc* process;
    ScreenLatch** item;
    ScreenObj* screen;
    StringObj* string;
    FightingLightState* state;
    PlyrInfo* player;
    int player_index;
    int i;

    process = pwr_bar_proc_item.object;
    if (process != 0 &&
        (unsigned int)process->instance != pwr_bar_proc_item.instance) {
        process = 0;
    }
    if (process != 0 && process->instance != 0) {
        ((PwrbarProcVtable*)process->vtbl)->destroy();
    }
    for (item = pbar_item_list; *item != 0; item++) {
        screen = owned_screen_latch_object(*item);
        if (screen != 0 && screen->instance != 0) {
            screen->vtbl->destroy();
        }
    }
    delete_screen_obj_oid(0x2015);
    for (item = pbar_string_item_list; *item != 0; item++) {
        string = string_latch_object(*item);
        if (string != 0 && string->instance != 0) {
            string->vtbl->destroy();
        }
    }
    for (i = 0; i < 8; i++) {
        medal_objs[i] = 0;
    }
    f_powerbars_retracted = 0;

    for (player_index = 0; player_index < 2; player_index++) {
        player = player_index == 0
            ? &g_game_info.plyr0 : &g_game_info.plyr1;
        state = fighting_light_state(player);
        state->flags = 0;

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
    int p2_flags = 0x20000000;
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
    if (mode_of_play == 10) {
        f_p1_force_adjustment = 1;
        f_p2_force_adjustment = 1;
        g_game_info.plyr0.field_0C = g_game_info.plyr0.field_10;
        g_game_info.plyr1.field_0C = g_game_info.plyr1.field_10;
    } else {
        f_p1_force_adjustment = 0;
        f_p2_force_adjustment = 0;
    }

    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20012, 0, 0x19);
    latch_screen(&p1_pbar_item, object);
    if (object != 0) {
        object->flags &= ~0x80;
    }
    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20015, 0, 0x1A);
    if (object != 0) {
        object->pfx2d->verts[0].x = (float)(BAR_BACK_X + 0x18);
        object->pfx2d->verts[0].y = 417.0f;
        object->pfx2d->verts[1].x = (float)(BAR_BACK_X + 0x11);
        object->pfx2d->verts[1].y = 431.0f;
        object->pfx2d->verts[2].x = (float)(BAR_BACK_X + 0x11E);
        object->pfx2d->verts[2].y = 431.0f;
        object->pfx2d->verts[3].x = (float)(BAR_BACK_X + 0x11E);
        object->pfx2d->verts[3].y = 417.0f;
        object->flags &= ~0x80;
    }
    latch_screen(&p1_pbar_red_item, object);

    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20012, p2_flags, 0x19);
    latch_screen(&p2_pbar_item, object);
    if (object != 0) {
        object->flags &= ~0x80;
    }
    object = load_2d_pfxobj(
        0x10005, 0x2017, (char*)0x20015, p2_flags, 0x1A);
    if (object != 0) {
        object->pfx2d->verts[0].x =
            (float)(screen_width - BAR_BACK_X - 0x11E);
        object->pfx2d->verts[0].y = 417.0f;
        object->pfx2d->verts[1].x =
            (float)(screen_width - BAR_BACK_X - 0x11E);
        object->pfx2d->verts[1].y = 431.0f;
        object->pfx2d->verts[2].x =
            (float)(screen_width - BAR_BACK_X - 0x11);
        object->pfx2d->verts[2].y = 431.0f;
        object->pfx2d->verts[3].x =
            (float)(screen_width - BAR_BACK_X - 0x11D);
        object->pfx2d->verts[3].y = 417.0f;
        object->flags &= ~0x80;
    }
    latch_screen(&p2_pbar_red_item, object);

    back = load_2d_pfxobj(
        0x10005, 0x2015, (char*)0x20013, 0, 0x18);
    latch_screen(&p1_pbar_back_item, back);
    if (back != 0) {
        back->x = BAR_BACK_X;
        back->y = 0x189;
        object = load_2d_pfxobj(
            0x10005, 0x2015, (char*)0x20014, p2_flags, 0x18);
        if (object != 0) {
            object->x = back->x + back->pfx2d->tex_w;
            object->y = 0x189;
        }
        latch_screen(&p1_pbar_backb_item, object);
    }

    back = load_2d_pfxobj(
        0x10005, 0x2015, (char*)0x20013, p2_flags, 0x18);
    latch_screen(&p2_pbar_back_item, back);
    if (back != 0) {
        back->x = screen_width - (BAR_BACK_X + back->pfx2d->tex_w);
        back->y = 0x189;
        object = load_2d_pfxobj(
            0x10005, 0x2015, (char*)0x20014, 0, 0x18);
        if (object != 0) {
            object->x = back->x - (object->pfx2d->tex_w - 8);
            object->y = 0x189;
        }
        latch_screen(&p2_pbar_backb_item, object);
    }

    name = string_left_xy(
        0x2016, 5,
        (const char*)global_player_data[g_game_info.plyr0.player_index].name,
        BAR_BACK_X + 0x23, 0x1A3, 0x17);
    if (name != 0) {
        name->flags &= 0x7FFFFFFF;
        p1_name_item.object = (ScreenObj*)name;
        p1_name_item.instance = name->instance;
        object = load_named_2d_pfxobj_xy(
            0x3000B, 0x2050, "LILHEAD", 0,
            BAR_BACK_X + 0xC, 0x184, 0x1C);
        if (object != 0) {
            object->flags &= ~0x10;
        }
        latch_screen(&p1_bar_icon_item, object);
    }

    name = string_right_xy(
        0x2016, 5,
        (const char*)global_player_data[g_game_info.plyr1.player_index].name,
        screen_width - (BAR_BACK_X + 0x23), 0x1A3, 0x17);
    if (name != 0) {
        name->flags &= 0x7FFFFFFF;
        p2_name_item.object = (ScreenObj*)name;
        p2_name_item.instance = name->instance;
        object = load_named_2d_pfxobj_xy(
            0x4000B, 0x2051, "LILHEAD", p2_flags,
            screen_width - (BAR_BACK_X + 0x8C), 0x184, 0x1C);
        if (object != 0) {
            object->flags &= ~0x10;
        }
        latch_screen(&p2_bar_icon_item, object);
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
        }
        latch_screen(&pbar_cntr_dragon_item, object);
    } else {
        object = load_2d_pfxobj_xy(
            0x10005, 0x204F, (char*)0x20016, 0,
            PB_CNTR_RING_X, 0x165, 0x18);
        if (object != 0) {
            pull_screen_obj(object);
            insert_screen_obj(object);
        }
        latch_screen(&pbar_cntr_item, object);
    }

    for (i = 0; i < 8; i++) {
        medal_objs[i] = 0;
    }
    update_plyr_medals();
    start_powerbar_monitor();
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
