#ifndef MW_SCREEN_ENGINE_GLUE_H
#define MW_SCREEN_ENGINE_GLUE_H

#include "runtime/mk_struct.h"

/* Screen engine tick pid (also wait_for_screen_close). */
#define SCREEN_ENGINE_PID 0x9011
/* Per-port controller handler pids start at this id (ports 0..2). */
#define SCREEN_ENGINE_CTRL_PID 0x901F

#define SE_EVT_FOCUS_GAIN 0x3EC
#define SE_EVT_FOCUS_LOSE 0x3ED
#define SE_EVT_DPAD_UP 0x3EE
#define SE_EVT_DPAD_DOWN 0x3EF
#define SE_EVT_DPAD_LEFT 0x3F0
#define SE_EVT_DPAD_RIGHT 0x3F1
#define SE_EVT_CONFIRM 0x3F2 /* A */
#define SE_EVT_FACE_Y 0x3F3
#define SE_EVT_SELECT 0x3F4
#define SE_EVT_START 0x3F5
#define SE_EVT_FACE_X 0x3F6
#define SE_EVT_BACK 0x3F7 /* B */
#define SE_EVT_L1 0x3F8
#define SE_EVT_L2 0x3F9
#define SE_EVT_R1 0x3FA
#define SE_EVT_R2 0x3FB
#define SE_EVT_CSTICK_DOWN 0x401
#define SE_EVT_CSTICK_RIGHT 0x402
#define SE_EVT_CSTICK_UP 0x403
#define SE_EVT_CSTICK_LEFT 0x404

/* HandleAction m_arg: params GetInt(0) -> target_game_mode (+ player latch). */
#define SE_ACT_SET_TARGET_GAME_MODE 0x1389

/* Studio events deferred while paused / controller-removed (12 slots). */
typedef struct PausedStudioEvent {
    unsigned int event; /* +0x00 -- 0 = empty */
    int flags;          /* +0x04 -- queued: -1/-2; live FireEvent pack */
} PausedStudioEvent;

#include "libmkparticle/pfxfont.h"

/* Font name cache row (stride 0x10) at ScreenEngineClient+0x08. */
typedef struct ScreenFontCacheRow {
    char* name; /* +0x00 */
    int unloadId; /* +0x04 */
    PfxFontSlot font; /* +0x08 -- face + metrics */
} ScreenFontCacheRow; /* 0x10 */

/* Controller-handler pdata (0x1C) for p_handle_screen_engine_controller. */
typedef struct ScreenCtrlPdata {
    MkHdr hdr;   /* +0x00 */
    int port;    /* +0x08 */
    int field0C; /* +0x0C */
    int field10; /* +0x10 */
} ScreenCtrlPdata;

#ifdef __cplusplus
extern "C" {
#endif

void init_screen_engine(void);
void pause_screen_engine(int paused);
void wait_for_screen_close(void);
void preload_screen_data(const char* name, int slot);
/* share_pdata: optional MkHdr* shared across screens; unload_slot!=0 unloads slot. */
void load_screen(const char* name, int slot, MkHdr* share_pdata, int unload_slot);
/* Returns 1 if event was broadcast now, 0 if queued / skipped while deferred. */
int broadcast_screen_studio_event(int event, int flag);
/* Drain paused queue then ScreenMgr::Idle(0). */
void screen_engine_process_events(void);
/* Like broadcast but live path is FireEvent; simpler deferred flag pack. */
void fire_screen_studio_event(int event, int flag);

void screen_engine_render(void);
void vdestroy_screen_engine(MkHdr* hdr);


void screen_engine_fire_switches(int port, unsigned int switches, int plyr_idx);

/*
 * Latch target_game_mode and (for modes 6..21) set_player_state side effects.
 * Same body as HandleAction SE_ACT_SET_TARGET_GAME_MODE without menu_player
 * rewrite from action flags. menu_player_arg: 0 = P1, 1 = P2.
 */
void set_target_game_mode(int menu_player_arg, int mode);

/* Returns share_pdata if instance still valid; else NULL. Callers cast. */
void* get_screen_pdata(void);
void screen_share_pdata(MkHdr* share);
extern void screen_engine_cleanup(void);
int get_menu_mode_sub_var(void);
void set_button_repeat_time(int ticks);
void set_default_button_repeat_time(void);
void set_popup_title_text(const char* text);
char* get_current_screen_name(void);
void set_popup_message_text(const char* text);
void set_popup_options_text(const char* text);
void refresh_screen_by_name(char* name);
int get_multi_profile_cursor_p2(void);
void set_popup_type(int type);
int get_gameoption_exitwithsave(void);
void set_gameoption_exitwithsave(int save);
void refresh_active_screen(void);
int get_multi_profile_cursor_p1(void);
void ppc_set_stage_value(int stage);
int ppc_get_stage_value(void);

#ifdef __cplusplus
}
#endif

#endif
