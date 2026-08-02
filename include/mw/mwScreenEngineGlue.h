#ifndef MW_SCREEN_ENGINE_GLUE_H
#define MW_SCREEN_ENGINE_GLUE_H

#include "runtime/hashtable.h"
#include "runtime/mk_struct.h"

typedef struct RwRaster RwRaster;

/*
 * mwScreenEngineGlue.o -- Midway C APIs around ScreenMgr / ScreenClient.
 *
 * B15 / B18 / B18d: p_main_menu + pselect call these unmangled C entry points
 * after art SSF load. Retail TU is ~61KB (.text); the 2D-menu callables are
 * lifted here (NonMatching -- ASM still linked for the rest).
 *
 * Soft ceiling: load_screen ~91.6%, preload ~99.2%, broadcast/fire ~81-85%;
 * wait_for_screen_close ~99.8%; fire_switches ~91%; CreateElement ~81%;
 * ReadStringData ~86%; CreateInstance ~82%; CreatePoly ~83%;
 * set_target_game_mode ~99.7%; LoadScreenSet ~92%; p_handle ~80%.
 *
 * Retail: Typical title path (retail screen name, not the literal "MAIN_MENU"):
 *   preload_screen_data("common/main_menu/m_mode_select", 0x90046);
 *   load_screen("common/main_menu/m_mode_select", 0x90046, 0, 0);
 *   ...
 *   wait_for_screen_close();
 * Character select (B19) reuses the same Glue with slot 0x17006A.
 *
 * =====================================================================
 * CALL PATH -- load_screen -> image screen_obj_list -> render_2d_objs
 * =====================================================================
 *
 *   load_screen(name, slot, share_pdata, unload_slot)
 *     1. screen_engine_client.slot = slot; optional unload_section_slot
 *     2. ScreenMgr::LoadScreen(name, 1)
 *     3. If LoadScreen != 0 and no pid 0x9011 yet:
 *          get_mkhdr(&vtbl_screen_engine, 0x8)
 *          insert_2d_obj(latch)                  -- onto image.screen_obj_list
 *          spawn p_screen_engine_tick (0x9011) + 3 controller procs
 *     4. Per frame (display -> render_2d_objs):
 *          vtbl_screen_engine entry -> screen_engine_render()
 *            -> ScreenMgr::Render() -> Screen::RenderAll (ScreenObject tree)
 *     5. Tick: p_screen_engine_tick -> Idle / UpdateAnimations
 *        Ctrl: p_handle_screen_engine_controller -> fire_switches -> FireEvent
 *
 * =====================================================================
 * Retail MUST-RUN (visible 2D menu -- link; do not stub away)
 * =====================================================================
 *
 *   init_screen_engine (boot once)
 *   preload_screen_data / load_screen / wait_for_screen_close
 *   broadcast_screen_studio_event
 *   screen_engine_render / p_screen_engine_tick / p_handle_screen_engine_controller
 *   screen_engine_fire_switches (pad edges -> FireEvent; soft-ceiling OK)
 *   ScreenMgr::LoadScreen / Idle / Render / FireEvent / BroadcastEvent
 *   insert_2d_obj path in load_screen (engine latch on screen_obj_list)
 *   render_2d_objs -> screen_engine_render bridge
 *
 * =====================================================================
 * CreateElement POLY / TEXT
 * =====================================================================
 *
 * POLY -- retail CreatePoly builds ScreenPoly 0x7C from SEPolyElement_t.
 *   Disc layout is ILP32 SeRef @+0x70. See ScreenPoly.h.
 *
 * TEXT -- CreateText builds ScreenText 0x24; labels need:
 *   1. CreateElement('TEXT') (Midway CreateText preferred)
 *   2. ProcessEngineEvent 0x407/0x409 (0x408 no-op) before Render
 *   3. Link pfxfont + gc_font (nativefont_*)
 *   Soft: ProcessEngineEvent ~98.5%; ScreenText::Render ~93.3%
 *   See ScreenText.h / ScreenUtil.h.
 */

/* Screen engine tick pid (also wait_for_screen_close). */
#define SCREEN_ENGINE_PID 0x9011
/* Per-port controller handler pids start at this id (ports 0..2). */
#define SCREEN_ENGINE_CTRL_PID 0x901F

/*
 * =====================================================================
 * WAVE C NAV -- pad edges -> ScreenMgr::FireEvent studio ids
 * =====================================================================
 *
 * gcio maps GC PAD bits through default_switch_map into logical switch
 * bits on GcPadSlot.edge (+0x14). p_handle_screen_engine_controller
 * reads that edge and calls screen_engine_fire_switches.
 *
 * Logical bit -> FireEvent(id, user=plyr_idx+1, 0):
 *   0x0001 L2     -> 0x3F9     0x0002 R2     -> 0x3FB
 *   0x0004 L1     -> 0x3F8     0x0008 R1/Z   -> 0x3FA
 *   0x0010 Y      -> 0x3F3     0x0020 X      -> 0x3F6
 *   0x0040 A      -> 0x3F2     0x0080 B      -> 0x3F7
 *   0x0100 Select -> 0x3F4     0x0800 Start  -> 0x3F5
 *   0x1000 D-Up   -> 0x3EE     0x2000 D-Right-> 0x3F1
 *   0x4000 D-Down -> 0x3EF     0x8000 D-Left -> 0x3F0
 * C-stick dirs (one-shot via s_nRepeatedStickBits): 0x401..0x404
 *
 * Mode-select MVP cares about:
 *   D-Pad / stick -> focus walk (ScreenSetFocusAction on those events)
 *   A (0x3F2) confirm / B (0x3F7) back -- scripted actions on focused obj
 *
 * Focus change (ScreenObject::SetFocus, fireEvents!=0):
 *   lose focus 0x3ED / gain focus 0x3EC
 *
 * Confirm -> leave menu idle: scr_*.sec actions with m_arg == 0x1389
 * (mkScreenEngineClient::HandleAction) write target_game_mode from
 * GetInt(0). Modes 6/7/11 -> pselect path in p_main_menu; 0x18 = idle.
 * Full HandleAction stays ASM; set_target_game_mode is the C twin.
 * D-Pad edges spawn hold-repeat mkprocs (p_repeat_button_input).
 */
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

/* Menu idle sentinel while mode select is open (p_main_menu / load_screen). */
#define SE_MENU_IDLE_MODE 0x18

/* Studio events deferred while paused / controller-removed (12 slots). */
typedef struct PausedStudioEvent {
    unsigned int event; /* +0x00 -- 0 = empty */
    int flags;          /* +0x04 -- queued: -1/-2; live FireEvent pack */
} PausedStudioEvent;

/*
 * =====================================================================
 * Retail BSS -- exact sizes (Glue island)
 * =====================================================================
 *
 * Contiguous Glue BSS island (symbols.txt). init_screen_engine casts
 * paused_event_queue to ScreenEngineBssIsland* (mgr/client/vars members):
 *
 *   paused_event_queue     .bss size 0x60  (12 x PausedStudioEvent)
 *   @814                   .bss size 0x0C  (+0x60) -- MWCC pad
 *   screen_manager         .bss size 0x264 (+0x6C from queue)  -- ScreenMgr
 *   @815                   .bss size 0x0C  (+0x2D0)
 *   screen_engine_client   .bss size 0x80  (+0x2DC)           -- below
 *   @816                   .bss size 0x0C  (+0x35C)
 *   game_variables         .bss size 0x1C  (+0x368)           -- mkGameVariables
 *   pause_screen_engine_proc .sbss size 0x4                   -- int flag
 *
 * Wrong overlay / undersized buffers -> silent crash on Init/Register.
 * Provide zeroed storage of these sizes before init_screen_engine().
 *
 * mkGameVariables is the game subclass of mwScreenEngine GameVariables
 * (same 0x1C layout; distinct __vt__15mkGameVariables). Tiny Get/Set stubs
 * live in this Glue TU (SetFloat / GetNumStrings / ...).
 */

/* Same layout as GameVariables (0x1C); subclass only swaps vtbl. */
typedef struct mkGameVariables {
    void* m_vtbl; /* +0x00 -- __vt__15mkGameVariables */
    int m_optMin; /* +0x04 */
    int m_optMax; /* +0x08 */
    int m_colMin; /* +0x0c */
    int m_colMax; /* +0x10 */
    int m_pad14; /* +0x14 */
    struct mkGameVariables* m_next; /* +0x18 */
} mkGameVariables; /* sizeof == 0x1C */

#include "libmkparticle/pfxfont.h"

/* Font name cache row (stride 0x10) at ScreenEngineClient+0x08. */
typedef struct ScreenFontCacheRow {
    char* name; /* +0x00 */
    int unloadId; /* +0x04 */
    PfxFontSlot font; /* +0x08 -- face + metrics */
} ScreenFontCacheRow; /* 0x10 */

/*
 * Global screen-engine client latch (bss size 0x80). share_pdata is
 * validated by comparing MkHdr.instance to share_instance before reuse.
 */
typedef struct ScreenEngineClient {
    char pad00[4];
    int slot; /* +0x04 -- section slot from load_screen */
    /*
     * Font name cache: 7 slots of stride 0x10 at +0x08.
     * CreateElement TEXT fills these when load_named_font fails (TGA + %s_MET).
     */
    ScreenFontCacheRow fontCache[7]; /* +0x08 .. +0x77 */
    MkHdr* share_pdata; /* +0x78 */
    int share_instance; /* +0x7C */
} ScreenEngineClient; /* sizeof == 0x80 */

/*
 * Overlay for init_screen_engine: retail addi from paused_event_queue base.
 * Linker still emits separate symbols (queue / mgr / client / vars + @814..@816).
 */
typedef struct ScreenEngineBssIsland {
    PausedStudioEvent queue[12]; /* +0x00 size 0x60 */
    unsigned char pad_814[0xC]; /* +0x60 -- symbols.txt @814 */
    char screen_manager[0x264]; /* +0x6C -- ScreenMgr storage */
    unsigned char pad_815[0xC]; /* +0x2D0 -- @815 */
    ScreenEngineClient client; /* +0x2DC */
    unsigned char pad_816[0xC]; /* +0x35C -- @816 */
    mkGameVariables game_variables; /* +0x368 */
} ScreenEngineBssIsland;

/* SE string table (Screen.h SEStringTable_t) -- C view for Glue. */
typedef struct SEStringTableView {
    unsigned int count; /* +0x00 */
    /* char* strings[count] packed at +0x04 */
} SEStringTableView;

/* Screen::m_data (SEScreen_t) -- C view; CHAR names via strings. */
typedef struct SEScreenDataView {
    unsigned char pad00[0x14];
    SEStringTableView* strings; /* +0x14 */
} SEScreenDataView;

/* Retail packed string at +4 after count (ILP32). No null/bounds guard. */
static inline char* SEStringAt(SEStringTableView* table, unsigned int index) {
    return ((char**)((char*)table + 4))[index];
}

/*
 * C views of C++ ScreenSet / resource lib (Glue LoadScreenSet / ReadStringData).
 * Matches retail offsets used by mkScreenEngineClient -- not full class layout.
 */
typedef struct ScreenResourceLibView {
    void* vtbl; /* +0x00 */
    void* parent; /* +0x04 */
    Hashtable strings; /* +0x08 -- ReadStringData store */
} ScreenResourceLibView;

typedef struct ScreenSetView {
    void* vtbl; /* +0x00 */
    int unloadId; /* +0x04 -- art file_index after LoadScreenSet */
    ScreenResourceLibView* resourceLib; /* +0x08 */
} ScreenSetView;

/* RwTexture filter/addressing word CreatePoly writes (retail +0x50). */
typedef struct RwTextureFilterView {
    RwRaster* raster; /* +0x00 */
    unsigned char pad04[0x4C];
    unsigned int filterFlags; /* +0x50 */
} RwTextureFilterView;

/* Controller-handler pdata (0x1C) for p_handle_screen_engine_controller. */
typedef struct ScreenCtrlPdata {
    MkHdr hdr;   /* +0x00 */
    int port;    /* +0x08 */
    int field0C; /* +0x0C */
    int field10; /* +0x10 */
} ScreenCtrlPdata;

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
float p_screen_engine_tick__Fv(void);
float p_handle_screen_engine_controller__Fv(void);
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

#endif

