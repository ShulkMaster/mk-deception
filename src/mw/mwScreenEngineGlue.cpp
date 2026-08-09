/*
 * mwScreenEngineGlue.o - Midway C APIs for screen load / tick / render / nav.
 *
 * NonMatching: B18d 2D-menu callables lifted; rest of ~61KB Glue stays ASM.
 * Lifted set:
 *   broadcast / fire / process_events / wait / preload / load_screen
 *   screen_engine_render / p_screen_engine_tick / init_screen_engine
 *   p_handle_screen_engine_controller / screen_engine_fire_switches
 *   p_repeat_button_input / p_repeat_analog_stick_input / set_target_game_mode
 *   pause / get_screen_pdata / screen_share_pdata / vdestroy_screen_engine
 *   mkScreenEngineClient::LoadScreenSet / ReadStringData / CreatePoly /
 *   CreateElement (POLY+TEXT) / CreateInstance / ScreenPoly::Render /
 *   ScreenText Render/ProcessEngineEvent/SetComponent/Close/ChangeCase /
 *   KeyPad::ChangeCase / _ChangeCase (SEElements walk) /
 *   GetStartArray (wrap line starts)
 *   CreateMatrixStack + frame ops
 *   SCtl ProcessParams: KeyEntry / KeyPad / TextItem / WifImage / ImageList /
 *   TextList / SpreadSheet
 *   KeyPad SetKey / HandleAction / HandleEvent / RefreshOption / Init / Dispose
 *   KeyEntry HandleAction / Init / Dispose
 *   TextItem UpdateString / ScrollText / HandleAction / HandleEvent /
 *   RefreshOption / Init / Dispose
 *   WifImage HandleEvent / Close / Dispose / Init
 *   TextList Init / ClearStrings / RefreshCollection / RefreshOption /
 *   Update / ScrollInc / ScrollDec / Move / HandleEvent / HandleAction
 *   ImageList::Update / RefreshCollection / RefreshOption /
 *   Increment / Decrement / Scroll* / HandleEvent / HandleAction
 *   SpreadSheet Init / RefreshCollection / RefreshOption(Fi/Fv) / Scroll*
 *   SpreadSheet HandleEvent / HandleAction
 *   PreRender / PostRender (empty)
 *   mkGameVariables GetInt / SetInt / GetString / string collections /
 *   int arrays / texture free / destructor plus tiny stubs
 *   no-op Get/Set row/col/string/Dispose/Init/IsValidOption
 *   retail leaf batch: menu/profile getters, current screen name, free_string,
 *   popup setters/state, ScreenClient/ScreenNode weak defaults, client
 *   malloc/free/resource library/unload wrappers, ImageList Init, and
 *   mkScreenEngineClient HandleEvent / CreateAction and six online-action
 *   Update methods
 *   SpreadSheet image/text ClearContents / AllocateCollection / Dispose /
 *   FinishSetup / Update
 *   wager coin-count formatting and all remaining retail destructors
 *
 * Soft ceilings (measured 2026-07-23 B18d Glue gap fill; -sdata 0 TU):
 * wait_for_screen_close ~59.5% (aproc/sleep ha/l + stmw frame).
 * preload ~99.2%; load_screen ~95.8%; broadcast/fire ~81-83%.
 * fire_switches ~86.3% (float i2f / NV); tick/check_allow ~92%.
 * CreatePoly ~79.4% (string-pool / vert schedule).
 * ScreenPoly::Render ~78.6% (stwux/psq + FPR).
 * CreateElement ~80.3% (font-cache/filter).
 * screen_engine_render ~79.0% (pause/state ha/l).
 * p_handle ~81%; p_repeat_button ~65.4% / p_repeat_analog ~63.4%.
 * ReadStringData ~84.4% (@914 mtctr / line-copy); LoadScreenSet ~92%.
 * set_target_game_mode ~99.7%; CreateInstance ~82%.
 * GetFloat__15mkGameVariablesFi ~97.5% (SDA reloc label; bytes match).
 * GetTextureCollection__15mkGameVariables... ~97.66% (typed per-case stack pairs).
 * GetStringCollection__15mkGameVariables... ~99.46% (stringBase reloc).
 * GetStringMatrixCollection__15mkGameVariables... ~54.9% (cmp tree).
 * GetString__15mkGameVariablesFi ~53.4% (cmp tree).
 * TextList ProcessParams ~90.3% (GV malloc / sprintf schedule); stop.
 * TextList ClearStrings ~95.8%; RefreshCollection(i) ~84.3%; stop.
 * TextList Update ~61.5% (linked-poly pos / retail stfs rA=0); RefreshOption ~68.2%; stop.
 * TextList ScrollInc/Dec ~99.4% (vtbl load r5 vs r12); stop.
 * TextList Move ~59%; HandleEvent ~85%; HandleAction ~59%; stop.
 * GetStartArray ~72.5% (wrap i2f / dual-pass schedule); stop.
 * SpreadSheet ProcessParams ~98.4% (FinishSetup / init schedule); stop.
 * SpreadSheet RefreshCollection ~85.3% (reg / vtbl schedule); stop.
 * SpreadSheet RefreshOption(Fi) ~91.8%; ScrollRight/Left/Up ~91-93%; ScrollDown ~87.4%; stop.
 * SpreadSheet Init / RefreshOption() 100%.
 * SpreadSheet HandleEvent ~65.4% (event cascade vs ifs); HandleAction ~17.8%
 *   (binary cmp tree vs switch + helpers); stop -- readable NonMatching.
 * SetComponent ScreenText/Poly ~0% -- retail jump-table (@7928/@7538) vs switch; algo OK; stop.
 * _ChangeCase ~68.4% -- nested helpers vs retail 3-level unroll; KeyPad ChangeCase 100%; stop.
 * ImageList Update ~72.6% / RefreshCollection ~69.8% (tex bind schedule); stop.
 * ImageList RefreshOption ~90.6%; Inc ~90.6%; Dec ~84.4%; Scroll* ~99.3%; stop.
 * ImageList HandleEvent ~62%; HandleAction ~35% (cmp tree); stop.
 * KeyPad SetKey ~65% (lang table stack copy); HandleAction ~89%;
 *   HandleEvent ~89%; RefreshOption ~49% (copy loop schedule); stop.
 * TextItem HandleAction ~27% (signed cmp tree vs switch); UpdateString ~45%;
 *   RefreshOption ~87%; ScrollText ~87%; HandleEvent ~87%; stop.
 * WifImage HandleEvent ~93.5%; Close ~58% / Dispose ~63% (ATC release);
 *   KeyEntry HandleAction ~70%; Init/Dispose for KeyPad TextItem KeyEntry WifImage 100%.
 * Full mkScreenEngineClient::HandleAction game-action dispatcher.
 * HandleEvent__20mkScreenEngineClient: exact 0x1E0 algorithm, ~32.9%;
 *   sparse switch form reproducibly exits GC/2.7 with optimizer code 159.
 * CreateAction__20mkScreenEngineClient: all seven action subclasses lifted,
 *   ~38.6%; sparse switch form also reproducibly exits with code 159.
 * ScreenActionCheckOnline::Update ~90.4% -- bitfield `extrwi.` source form
 *   reproducibly exits with code 159; five other online Updates are 100%.
 * SpreadSheet image Update ~73.0%, text Update ~75.9%; full retail windowing,
 *   content refresh, marker placement, color/texture sync, and events lifted.
 * SpreadSheet image/text FinishSetup ~78.9%/~66.5%; ClearContents
 *   ~70.6%/~81.2%. Remaining differences are loop/register scheduling.
 * wager_load_koin_count_string_array ~78.8%; retail-unrolled O4 form exits
 *   GC/2.7 with code 159, so this function alone uses optimization level 3.
 * All retail callable symbols are present; remaining missing object symbols
 *   are compiler-emitted vtables or retail-owned data definitions.
 */

#include "mw/mwScreenEngineGlue.h"
#include "mw/mwMemHeap.h"

#include "game/game_info.h"
#include "game/memcard.h"
#include "game/plyrprofile.h"
#include "game/settings.h"
#include "libmkparticle/particle.h"
#include "libmkparticle/pfx2d.h"
#include "libmkparticle/pfxfont.h"
#include "movie/MkMovies.h"
#include "mwScreenEngine/ScreenPoly.h"
#include "mwScreenEngine/ScreenSCtl.h"
#include "mwScreenEngine/ScreenText.h"
#include "mwScreenEngine/TextureCollection.h"
#include "platform/gcutils.h"
#include "platform/gcio.h"
#include "runtime/asset.h"
#include "runtime/fonts.h"
#include "runtime/hashtable.h"
#include "runtime/image.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_info.h"
#include "runtime/section.h"
#include "runtime/utils.h"
#include "math/gxMat.h"

extern "C" {

/* MWCC 2.7 O4 workaround: removing this duplicate triggers optimizer code 159. */
#pragma use_lmw_stmw on

/* C view of ScreenAction fields used by the game-specific action handlers. */
typedef struct ScreenActionView {
    void* vtbl;                 /* +0x00 */
    int state_04;               /* +0x04 */
    int state_08;               /* +0x08 */
    unsigned char pad_0C[0x0C]; /* +0x0C */
    void* event;                /* +0x18 */
    int arg;                    /* +0x1C */
    unsigned int event_index;   /* +0x20 */
    unsigned char pad_24[4];    /* +0x24 */
    int eventUser;              /* +0x28 */
    void* owner;                /* +0x2C */
    void* params;               /* +0x30 */
} ScreenActionView;

/* MkProc.flags (+0xA8) bit 0x08 (SKIP_IF_PAUSED) -> retail rlwimi. */
typedef struct MkProcPauseFlag {
    unsigned char pad0 : 4;
    unsigned char skip_if_paused : 1;
    unsigned char pad1 : 3;
} MkProcPauseFlag;

typedef struct MkVtableMkprocLocal {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
    int (*system_stack)(void);
    int (*local_stack)(void);
    float (*jump_sleep)(MkProcEntryFn entry);
} MkVtableMkprocLocal;

char* strncpy(char* dst, const char* src, unsigned long n);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strupr(char* s);
char* strcpy(char* dst, const char* src);
int sprintf(char* buf, const char* fmt, ...);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, unsigned long n);
int stricmp(const char* a, const char* b);
unsigned long strlen(const char* s);
void* memcpy(void* dst, const void* src, unsigned long n);
void* memset(void* dst, int value, unsigned long n);
void* __nw__FUl(unsigned long size);
void free_mem(void* mem);
void __ct__17ScreenMatrixStackFv(void* self);
void __dt__17ScreenMatrixStackFv(void* self, short del);
void __dl__FPv(void* p);
RwFrame* RwFrameCreate(void);
RwBool RwFrameDestroy(RwFrame* frame);
RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* delta, int combine);
RwFrame* RwFrameScale(RwFrame* frame, const RwV3d* scale, int combine);
RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, float angle, int combine);
RwFrame* RwFrameSetIdentity(RwFrame* frame);
RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child);
void MKMatrixTranslate(void* matrix, float* delta, int combine);
int is_controller_removed(void);

int ReadHexInt__10ScreenUtilFPc(char* s);
void* Malloc__10ScreenUtilFUliPc(unsigned long size, int tag, char* name);
char* GetName__9ScreenSetFv(void* set);
int LoadSetData__15ScreenInstancerFP9ScreenSetPvUiPv(void* set, void* data,
                                                     unsigned int size, void* unused);
void* __nw__10ScreenNodeFUl(unsigned long size);
void __ct__10ScreenNodeFv(void* node);
void* __nw__12ScreenActionFUl(unsigned long size);
void __ct__12ScreenActionFv(void* action);
void StartLocal__17ScreenActionStackFv(void* stack);
void EndLocal__17ScreenActionStackFv(void* stack);
void ProcessSubActions__12ScreenObjectFPC12ScreenActioni(
    void* self, const void* action, int match);
int GetResourceID__12ScreenParamsFUi(void* params, unsigned int index);
int GetInt__12ScreenParamsFUi(void* params, unsigned int index);
int GetBoolean__12ScreenParamsFUi(void* params, unsigned int index);
float GetFloat__12ScreenParamsFUi(void* params, unsigned int index);
ScreenNode* GetScreenNode__12ScreenParamsFUi(void* params, unsigned int index);
char* GetName__12ScreenParamsFUi(void* params, unsigned int index);
unsigned int GetColor__12ScreenParamsFUi(void* params, unsigned int index);
int GetCount__12ScreenParamsCFv(void* params);
unsigned int HasSubActions__11ScreenEventCFUi(void* event,
                                              unsigned int index);
unsigned int GetNumOfSubActions__11ScreenEventCFUi(void* event,
                                                    unsigned int index);
unsigned int GetAction__11ScreenEventCFUi(void* event, unsigned int index);
void* GetParams__11ScreenEventCFUi(void* event, unsigned int index);
void* CreateAction__17ScreenActionStackFUi(unsigned int type);
void PushAction__17ScreenActionStackFP12ScreenAction(void* stack,
                                                     void* action);
void DestroyScreen__15ScreenInstancerFP6Screen(void* screen);
void __ct__21ScreenResourceLibraryFP21ScreenResourceLibrary(void* self,
                                                             void* parent);
extern void* __vt__29mkScreenEngineResourceLibrary[];
extern void* __vt__18ScreenActionRandom[];
extern void* __vt__23ScreenActionCheckOnline[];
extern void* __vt__27ScreenActionOnlineChallenge[];
extern void* __vt__32ScreenActionOnlineIsOpponentIdle[];
extern void* __vt__32ScreenActionOnlinePickChallenger[];
extern void* __vt__33ScreenActionOnlineChallengeCancel[];
extern void* __vt__37ScreenActionOnlineResetChallengeState[];
extern void* __vt__20mkScreenEngineClient[];
extern void* __vt__15mkGameVariables[];

/* Retail leaves create_mkproc's MkProc* in r3. */
MkProc* _create_mkproc_generic_bigstack(int proc_id, int priority, void* proc_fn, int pdata_size,
                                        void** pdata_out);
MkProc* _create_mkproc_generic_nostack(int proc_id, int priority, void* proc_fn, int pdata_size,
                                       void** pdata_out);

/* ScreenMgr C++ (mangled); returns nonzero on accept (async load may continue). */
int LoadScreen__9ScreenMgrFPcUi(void* mgr, char* name, unsigned int flag);
void BroadcastEvent__9ScreenMgrFiii(void* mgr, int event, int a, int b);
void FireEvent__9ScreenMgrFiiUi(void* mgr, int event, int a, unsigned int b);
void Render__9ScreenMgrFv(void* mgr);
void Idle__9ScreenMgrFi(void* mgr, int dt);
void UpdateAnimations__9ScreenMgrFi(void* mgr, int dt);
void Dispose__9ScreenMgrFUi(void* mgr, unsigned int flags);
void Init__9ScreenMgrFP12ScreenClient(void* mgr, void* client);
void* GetActiveScreen__9ScreenMgrFv(void* mgr);
int FindScreen__9ScreenMgrFPcPP6Screen(void* mgr, char* name, void** screen);
void RegisterGameVariables__13ScreenControlFUiP13GameVariables(unsigned int unused, void* vars);
void Init__13ScreenControlFv(void* self);
void RefreshAllCollections__13ScreenControlFP6Screen(void* screen);
void RefreshAllOptions__13ScreenControlFP6Screen(void* screen);
void __ct__9ScreenMgrFv(void* self);
void __dt__9ScreenMgrFv(void* self, short del);
void __ct__12ScreenClientFv(void* self);
void __ct__13GameVariablesFv(void* self);
void SetCollectionRange__13GameVariablesFUiUi(
    void* self, unsigned int first, unsigned int last);
void SetOptionRange__13GameVariablesFUiUi(
    void* self, unsigned int first, unsigned int last);
void* __dt__20mkScreenEngineClientFv(void* self, short del);
mkGameVariables* __dt__15mkGameVariablesFv(
    mkGameVariables* self, short del);
void __register_global_object(
    void* object, void* destructor, void* registration);

void fxbanks_unload_by_owner(int owner);
int get_stick_pos(int port, int which, float* out_x, float* out_y);
int check_allow_screen_engine_control__Fv(void);
void set_player_state(PlyrInfo* plyr, int state);
void destroy_mkprocs_pid(int pid);
int check_switch(int port, int switch_index);
void bg_pselect_set_stage(int player, int stage);
void bg_pselect_set_character(int character);
void pselect_player_canceled(int player);
void pselect_player_selected(PlyrInfo* player);
void bg_pselect_player_canceled(int player);
void pselect_player_moved(int player);
void pselect_set_arena(int arena);
void pselect_bgnd_select_done(void);
void pselect_handicap_update(int player, int value);
int bg_pselect_get_stage(int player);
int pselect_get_selbox_pos(int player);
int bg_pselect_get_offender_class(int player);
int pselect_get_arena_index(void);
void cconfig_set_current_cell(int player, int cell);
void add_to_wls_left_cursor(int value);
void set_volume(int channel, int value);
void set_memcard_cursor_for(int value);
void ppc_set_button_answer(int value);
void ppc_set_current_icon_selection(unsigned char value);
void ppv_update_profile_cursor(int value);
void controller_setup_save_to_profile(int player, int value);
void ppc_transition_pause(int value);
void controller_setup_p1_state(int value);
void controller_setup_p2_state(int value);
void adjust_brightness(int value);
void set_save_progress_flag(int value);
void set_language(int value);
int controller_get_texture_index_for_button(int player, int button);
int get_left_storage_device_status(void);
int controller_get_player_last_button(int player);
int get_right_storage_device_display_status(void);
int ppc_get_code_state(void);
int ok_to_bring_out_wager_screen(void);
int pselect_bgnd_has_deathtrap(void);
int pselect_bgnd_has_level_transition(void);
int pselect_bgnd_has_weapon(void);
int get_save_progress_flag(void);
int konquest_is_save_allowed(void);
int trial_never_passed_this_mission(void);
int pselect_get_body_texture_index(int player);
int get_contrast_value(void);
int get_brightness_value(void);
int get_widescreen_state(void);
int get_progressive_scan_state(void);
int get_gamma_value(void);
int get_color_red_value(void);
int get_color_green_value(void);
int get_color_blue_value(void);

extern MkFileEntry screen_engine_file_table[];
extern MkVtable5 vtbl_screen_engine;
extern ScreenEngineClient screen_engine_client;
extern char screen_manager[];
extern mkGameVariables game_variables;
extern PausedStudioEvent paused_event_queue[12];
extern int pause_screen_engine_proc;
extern int target_game_mode;
extern int menu_player;
extern int menu_mode_sub_var;
extern int gameoption_exitwithsave;
extern int multi_profile_cursor_p1;
extern int multi_profile_cursor_p2;
void fade_to_black(int ticks, int freeze);
char* GetName__6ScreenFv(void* screen);

/* Retail Glue sdata (set_default_button_repeat_time / set_button_repeat_time). */
int button_repeat_time;

/* Sleep 1.0f; full-TU retail labels this @4240 after a leading -1.0f in .sdata2. */
static const float kSleepNegOne = -1.0f;
static const float kSleepOne = 1.0f;
static const float kMsPerSec = 1000.0f;

/* ScreenMgr.m_activeCount @ +0x1A4 (C view of ScreenMgr layout). */
typedef struct ScreenMgrActive {
    char pad00[0x1A4];
    int active_count; /* +0x1A4 */
    void* screens[16]; /* +0x1A8 */
} ScreenMgrActive;

int current_render_state;
unsigned int s_nRepeatedStickBits;
char popup_message_text[0x200] = {0};
char popup_title_text[0x100] = {0};
char popup_options_text[0x100] = {0};

void __sinit_mwScreenEngineGlue_cpp(void) {
    ScreenEngineBssIsland* island;

    island = (ScreenEngineBssIsland*)paused_event_queue;

    __ct__9ScreenMgrFv(island->screen_manager);
    __register_global_object(
        island->screen_manager, __dt__9ScreenMgrFv,
        (char*)island + 0x60);

    __ct__12ScreenClientFv(&island->client);
    *(void***)&island->client = __vt__20mkScreenEngineClient;
    __register_global_object(
        &island->client, __dt__20mkScreenEngineClientFv,
        (char*)island + 0x2D0);

    __ct__13GameVariablesFv(&island->game_variables);
    island->game_variables.m_vtbl = __vt__15mkGameVariables;
    SetCollectionRange__13GameVariablesFUiUi(
        &island->game_variables, 0, 0x7FFFFFFF);
    SetOptionRange__13GameVariablesFUiUi(
        &island->game_variables, 0, 0x7FFFFFFF);
    __register_global_object(
        &island->game_variables, __dt__15mkGameVariablesFv,
        (char*)island + 0x35C);
}
int pprofile_stage_var;
int popup_type;
/* Per-player filtered stick edge bits (plyr_idx index; size 0x10). */
static unsigned int stick_bits[4];

int get_menu_mode_sub_var(void) {
    return menu_mode_sub_var;
}

char* get_current_screen_name(void) {
    ScreenMgrActive* mgr;
    void* screen;

    mgr = (ScreenMgrActive*)screen_manager;
    if (mgr->active_count < 0) {
        screen = 0;
    } else {
        screen = mgr->screens[mgr->active_count];
    }
    if (screen == 0) {
        return 0;
    }
    return GetName__6ScreenFv(screen);
}

void refresh_active_screen(void) {
    void* screen;

    screen = GetActiveScreen__9ScreenMgrFv(screen_manager);
    if (screen != 0) {
        RefreshAllCollections__13ScreenControlFP6Screen(screen);
        RefreshAllOptions__13ScreenControlFP6Screen(screen);
    }
}

void refresh_screen_by_name(char* name) {
    void* screen;

    screen = 0;
    FindScreen__9ScreenMgrFPcPP6Screen(screen_manager, name, &screen);
    if (screen != 0) {
        RefreshAllCollections__13ScreenControlFP6Screen(screen);
        RefreshAllOptions__13ScreenControlFP6Screen(screen);
    }
}

void screen_engine_cleanup(void) {
    Dispose__9ScreenMgrFUi(screen_manager, 1);
    memset(screen_engine_client.fontCache, 0,
           sizeof(screen_engine_client.fontCache));
    screen_engine_client.share_pdata = 0;
    screen_engine_client.share_instance = 0;
    pause_screen_engine_proc = 0;
    destroy_mkprocs_pid(0x9011);
    destroy_mkprocs_pid(0x901F);
    memset(paused_event_queue, 0, sizeof(paused_event_queue));
}

int get_gameoption_exitwithsave(void) {
    return gameoption_exitwithsave;
}

int get_multi_profile_cursor_p2(void) {
    return multi_profile_cursor_p2;
}

int get_multi_profile_cursor_p1(void) {
    return multi_profile_cursor_p1;
}

void free_string(void* string) {
    _mwMemFree(string, 0, 0);
}

int GetArtSlot__Fv(void) {
    return screen_engine_client.slot;
}

void PrintObjectDepth__20mkScreenEngineClientFP16ScreenRenderInfoii(
    ScreenEngineClient* self, void* info, int depth, int flags) {
    (void)self;
    (void)info;
    (void)depth;
    (void)flags;
}

void Reset__20mkScreenEngineClientFv(ScreenEngineClient* self) {
    (void)self;
}

void SetCurrent__20mkScreenEngineClientFP9ScreenSet(ScreenEngineClient* self,
                                                     void* set) {
    (void)self;
    (void)set;
}

void UnloadScreen__20mkScreenEngineClientFP6Screen(ScreenEngineClient* self,
                                                    void* screen) {
    (void)self;
    DestroyScreen__15ScreenInstancerFP6Screen(screen);
}

void UnloadScreenSet__20mkScreenEngineClientFi(ScreenEngineClient* self,
                                                int unloadId) {
    int count;
    int i;

    do {
        count = get_slot_file_count(self->slot);
        unload_section_slot_file(self->slot, count);
        if (count == unloadId) {
            break;
        }
    } while (count > 0);

    for (i = 0; i < 7; i++) {
        if (unloadId <= self->fontCache[i].unloadId) {
            self->fontCache[i].name = 0;
            self->fontCache[i].unloadId = 0;
        }
    }
}

void* CreateAction__20mkScreenEngineClientFi(ScreenEngineClient* self,
                                              int type) {
    void** action;

    (void)self;
    if (type == 0x139C) {
        action = (void**)__nw__12ScreenActionFUl(0x3C);
        if (action != 0) {
            __ct__12ScreenActionFv(action);
            action[0] = __vt__37ScreenActionOnlineResetChallengeState;
        }
    } else {
        if (type < 0x139C) {
            if (type == 0x138E) {
                action = (void**)__nw__12ScreenActionFUl(0x3C);
                if (action == 0) {
                    return 0;
                }
                __ct__12ScreenActionFv(action);
                action[0] = __vt__33ScreenActionOnlineChallengeCancel;
                return action;
            }
            if (type < 0x138E) {
                if (type == 0x138C) {
                    action = (void**)__nw__12ScreenActionFUl(0x3C);
                    if (action == 0) {
                        return 0;
                    }
                    __ct__12ScreenActionFv(action);
                    action[0] = __vt__27ScreenActionOnlineChallenge;
                    return action;
                }
            } else if (type < 0x1390) {
                action = (void**)__nw__12ScreenActionFUl(0x3C);
                if (action == 0) {
                    return 0;
                }
                __ct__12ScreenActionFv(action);
                action[0] = __vt__32ScreenActionOnlinePickChallenger;
                return action;
            }
        } else {
            if (type == 0x1F41) {
                action = (void**)__nw__12ScreenActionFUl(0x3C);
                if (action == 0) {
                    return 0;
                }
                __ct__12ScreenActionFv(action);
                action[0] = __vt__18ScreenActionRandom;
                return action;
            }
            if (type < 0x1F41) {
                if (type < 0x139E) {
                    action = (void**)__nw__12ScreenActionFUl(0x3C);
                    if (action == 0) {
                        return 0;
                    }
                    __ct__12ScreenActionFv(action);
                    action[0] = __vt__32ScreenActionOnlineIsOpponentIdle;
                    return action;
                }
            } else if (type == 0x61ED) {
                action = (void**)__nw__12ScreenActionFUl(0x3C);
                if (action == 0) {
                    return 0;
                }
                __ct__12ScreenActionFv(action);
                action[0] = __vt__23ScreenActionCheckOnline;
                return action;
            }
        }
        action = 0;
    }
    return action;
}

int Update__32ScreenActionOnlineIsOpponentIdleFP9ScreenMgrR17ScreenActionStacki(
    void* action, void* mgr, void* stack, int dt) {
    ScreenActionView* view = (ScreenActionView*)action;

    (void)mgr;
    (void)dt;
    StartLocal__17ScreenActionStackFv(stack);
    EndLocal__17ScreenActionStackFv(stack);
    view->state_04 = 0;
    view->state_08 = 0;
    return 1;
}

int Update__37ScreenActionOnlineResetChallengeStateFP9ScreenMgrR17ScreenActionStacki(
    void* action, void* mgr, void* stack, int dt) {
    ScreenActionView* view = (ScreenActionView*)action;

    (void)mgr;
    (void)dt;
    StartLocal__17ScreenActionStackFv(stack);
    EndLocal__17ScreenActionStackFv(stack);
    view->state_04 = 0;
    view->state_08 = 0;
    return 1;
}

int Update__32ScreenActionOnlinePickChallengerFP9ScreenMgrR17ScreenActionStacki(
    void* action, void* mgr, void* stack, int dt) {
    ScreenActionView* view = (ScreenActionView*)action;

    (void)mgr;
    (void)dt;
    StartLocal__17ScreenActionStackFv(stack);
    EndLocal__17ScreenActionStackFv(stack);
    view->state_04 = 0;
    view->state_08 = 0;
    return 1;
}

int Update__33ScreenActionOnlineChallengeCancelFP9ScreenMgrR17ScreenActionStacki(
    void* action, void* mgr, void* stack, int dt) {
    ScreenActionView* view = (ScreenActionView*)action;

    (void)mgr;
    (void)dt;
    StartLocal__17ScreenActionStackFv(stack);
    EndLocal__17ScreenActionStackFv(stack);
    view->state_04 = 0;
    view->state_08 = 0;
    return 1;
}

int Update__27ScreenActionOnlineChallengeFP9ScreenMgrR17ScreenActionStacki(
    void* action, void* mgr, void* stack, int dt) {
    ScreenActionView* view = (ScreenActionView*)action;
    void* params;

    (void)mgr;
    (void)dt;
    params = view->params;
    StartLocal__17ScreenActionStackFv(stack);
    if (params != 0) {
        GetInt__12ScreenParamsFUi(params, 0);
    }
    EndLocal__17ScreenActionStackFv(stack);
    view->state_04 = 0;
    view->state_08 = 0;
    return 1;
}

int Update__23ScreenActionCheckOnlineFP9ScreenMgrR17ScreenActionStacki(
    void* action, void* mgr, void* stack, int dt) {
    ScreenActionView* view = (ScreenActionView*)action;
    void* owner;

    (void)mgr;
    (void)dt;
    view->state_04 = 0;
    view->state_08 = 0;
    owner = view->owner;
    if ((g_game_info.field_04 & 0x80) != 0 && owner != 0) {
        ProcessSubActions__12ScreenObjectFPC12ScreenActioni(owner, action, 0);
    }
    EndLocal__17ScreenActionStackFv(stack);
    return 1;
}

int Update__18ScreenActionRandomFP9ScreenMgrR17ScreenActionStacki(
    void* action, void* mgr, void* stack, int dt) {
    typedef void (*ActionInit)(void*, void*, unsigned int, void*,
                               unsigned int, void*, int);
    ScreenActionView* view = (ScreenActionView*)action;
    void* event;
    unsigned int eventIndex;
    unsigned int actionType;
    void* params;
    void* created;

    (void)mgr;
    (void)dt;
    view->state_04 = 0;
    view->state_08 = 0;
    event = view->event;
    if (event != 0) {
        eventIndex = view->event_index;
        if (HasSubActions__11ScreenEventCFUi(event, eventIndex) != 0) {
            eventIndex +=
                (unsigned short)randu0((unsigned short)
                    GetNumOfSubActions__11ScreenEventCFUi(event, eventIndex));
            actionType = GetAction__11ScreenEventCFUi(event, eventIndex);
            params = GetParams__11ScreenEventCFUi(event, eventIndex);
            created = CreateAction__17ScreenActionStackFUi(actionType);
            (*(ActionInit**)created)[4](
                created, event, eventIndex,
                view->owner, actionType, params, view->eventUser);
            PushAction__17ScreenActionStackFP12ScreenAction(stack, created);
        }
    }
    EndLocal__17ScreenActionStackFv(stack);
    return 1;
}

void DestroyResourceLibrary__20mkScreenEngineClientFP21ScreenResourceLibrary(
    ScreenEngineClient* self, void* library) {
    typedef void (*DeletingDtor)(void*, int);

    (void)self;
    if (library != 0) {
        (*(DeletingDtor**)library)[2](library, 1);
    }
}

void* CreateResourceLibrary__20mkScreenEngineClientFP21ScreenResourceLibrary(
    ScreenEngineClient* self, void* parent) {
    ScreenResourceLibView* library;

    (void)self;
    library = (ScreenResourceLibView*)__nw__FUl(0x34);
    if (library != 0) {
        __ct__21ScreenResourceLibraryFP21ScreenResourceLibrary(library, parent);
        library->vtbl = __vt__29mkScreenEngineResourceLibrary;
        hashtable_dynamic_init(&library->strings, 0x101, wave_heap);
    }
    return library;
}

void Free__20mkScreenEngineClientFPv(ScreenEngineClient* self, void* mem) {
    (void)self;
    free_mem(mem);
}

void* Malloc__20mkScreenEngineClientFUliPc(ScreenEngineClient* self,
                                            unsigned long size, int tag,
                                            char* name) {
    (void)self;
    (void)tag;
    (void)name;
    return _mwMemMalloc(wave_heap, size, 4, 0, 0, 0);
}

void RefreshCollection__13ScreenControlFv(void* self) {
    (void)self;
}

void RefreshOption__13ScreenControlFv(void* self) {
    (void)self;
}

void SetVisible__12ScreenObjectFUi(void* self, unsigned int visible) {
    unsigned int* flags;

    flags = (unsigned int*)((char*)*(void**)((char*)self + 0x1C) + 0x0C);
    if (visible != 0) {
        *flags |= 1;
    } else {
        *flags &= ~1U;
    }
}

unsigned int IsVisible__12ScreenObjectFv(void* self) {
    unsigned int* flags;

    flags = (unsigned int*)((char*)*(void**)((char*)self + 0x1C) + 0x0C);
    return *flags & 1;
}

void ProcessEngineEvent__10ScreenNodeFP9ScreenMgri(void* self, void* mgr,
                                                   int event) {
    (void)self;
    (void)mgr;
    (void)event;
}

int GetNumNodes__10ScreenNodeCFv(void* self) {
    (void)self;
    return 0;
}

int NeedIdleProcessing__10ScreenNodeFv(void* self) {
    (void)self;
    return 0;
}

void Close__10ScreenNodeFv(void* self) {
    (void)self;
}

int HandleAction__10ScreenNodeFP9ScreenMgrPC12ScreenAction(
    void* self, void* mgr, const void* action) {
    (void)self;
    (void)mgr;
    (void)action;
    return 0;
}

void SetMatrixStack__10ScreenNodeFP17ScreenMatrixStack(void* self,
                                                       void* stack) {
    (void)self;
    (void)stack;
}

void ReportError__20mkScreenEngineClientFPcPci(ScreenEngineClient* self,
                                               char* message, char* file,
                                               int line) {
    (void)self;
    (void)message;
    (void)file;
    (void)line;
}

void set_default_button_repeat_time(void) {
    button_repeat_time = 15;
}

void set_button_repeat_time(int ticks) {
    button_repeat_time = ticks;
}

void ppc_set_stage_value(int value) {
    pprofile_stage_var = value;
}

void set_popup_type(int type) {
    if (type < 0 || type >= 13) {
        popup_type = 0;
    } else {
        popup_type = type;
    }
}

char* GetString__21ScreenResourceLibraryFPc(void* self, char* name) {
    (void)self;
    (void)name;
    return 0;
}

char* GetString__21ScreenResourceLibraryFi(void* self, int id) {
    (void)self;
    (void)id;
    return 0;
}

int DoneLoadingSet__12ScreenClientFP9ScreenSet(void* self, void* set) {
    (void)self;
    (void)set;
    return 1;
}

char* GetString__29mkScreenEngineResourceLibraryFPc(void* self, char* name) {
    typedef char* (*GetStringByName)(void*, char*);
    ScreenResourceLibView* library;
    void* parent;
    char* result;

    library = (ScreenResourceLibView*)self;
    result = (char*)hashtable_get(&library->strings, name);
    parent = library->parent;
    if (result == 0 && parent != 0) {
        result = (*(GetStringByName**)parent)[4](parent, name);
    }
    return result;
}

void Init__9ImageListFv(void* self) {
    Init__13ScreenControlFv(self);
}

/* Retail @stringBase0 prefix through "scr_%s.sec" at +0x1C9. */
static const char stringBase0[] =
    "English-US\0" /* +0x0 */
    "Spanish\0"
    "German\0"
    "French\0"
    "Italian\0"
    "DEL\0"
    "SUPR\0"
    "ENTF\0"
    "SUPPR\0"
    "Canc\0"
    "SPC\0"
    "ESP.\0"
    "LEER\0"
    "ESPACE\0"
    "SPZ\0"
    "END\0"
    "FIN\0"
    "ENDE\0"
    "Fine\0"
    "0\0"
    "1\0"
    "2\0"
    "3\0"
    "4\0"
    "5\0"
    "6\0"
    "7\0"
    "8\0"
    "9\0"
    "10\0"
    "11\0"
    "12\0"
    "13\0"
    "14\0"
    "15\0"
    "16\0"
    "17\0"
    "18\0"
    "19\0"
    "20\0"
    "21\0"
    "22\0"
    "23\0"
    "24\0"
    "25\0"
    "26\0"
    "27\0"
    "28\0"
    "29\0"
    "30\0"
    "31\0"
    "32\0"
    "33\0"
    "34\0"
    "35\0"
    "36\0"
    "37\0"
    "38\0"
    "39\0"
    "40\0"
    "41\0"
    "42\0"
    "43\0"
    "44\0"
    "45\0"
    "46\0"
    "47\0"
    "48\0"
    "49\0"
    "50\0"
    "51\0"
    "52\0"
    "53\0"
    "54\0"
    "55\0"
    "56\0"
    "57\0"
    "58\0"
    "59\0"
    "60\0"
    "61\0"
    "62\0"
    "63\0"
    "64\0"
    "65\0"
    "66\0"
    "67\0"
    "68\0"
    "69\0"
    "70\0"
    "71\0"
    "72\0"
    "73\0"
    "74\0"
    "75\0"
    "76\0"
    "77\0"
    "78\0"
    "79\0"
    "80\0"
    "81\0"
    "82\0"
    "83\0"
    "84\0"
    "85\0"
    "86\0"
    "87\0"
    "88\0"
    "89\0"
    "90\0"
    "91\0"
    "92\0"
    "93\0"
    "94\0"
    "OFF\0"
    "96\0"
    "97\0"
    "98\0"
    "99\0"
    "Arcade\0"
    "Puzzle Kombat\0"
    "Konquest\0"
    "MK Chess\0"
    "Versus\0"
    "Go Online\0"
    "\0"
    "scr_%s.sec\0" /* +0x1C9 */
    "SS-texture cache\0"
    "%d\0"
    "Game Options: Text Collection\0"
    "Game Options: Progressive scan\0"
    "Game Options: Wide screen adjust\0"
    "%s\0"
    "%d%%\0"
    "UNFORMED\0"
    "uninitalized\0"
    "UI sound number out of range: %d\0"
    "SS-Spreadsheet\0"
    "TextItem::RefreshOption\0"
    "SS-4TextList\0"
    "SS-3TextList\0"
    "SS-1TextList\0"
    "SS-2TextList\0"
    "SS-5TextList\0"
    "SS-6TextList\0"
    "SS-7TextList\0"
    "SS-Image List\0"
    "<COLOR=0x\0"
    "Screen - Text Obj\0"
    "CLOUDS\0" /* +0x331 */
    "screen_fx.mko\0" /* +0x338 */
    "%s_MET\0" /* +0x346 */
    "8X8\0" /* +0x34D */
    "Strings/\0" /* +0x351 */
    "real string\0" /* +0x35A */
    "empty string\0" /* +0x366 */
    "STRINGS\0" /* +0x373 */
    "SCREEN"; /* +0x37B */

void set_popup_options_text(const char* text) {
    if (strlen(text) >= sizeof(popup_options_text)) {
        strcpy(popup_options_text, stringBase0 + 0x257);
    } else {
        strcpy(popup_options_text, text);
    }
}

void set_popup_title_text(const char* text) {
    if (strlen(text) >= sizeof(popup_title_text)) {
        strcpy(popup_title_text, stringBase0 + 0x257);
    } else {
        strcpy(popup_title_text, text);
    }
}

void set_popup_message_text(const char* text) {
    if (strlen(text) >= sizeof(popup_message_text)) {
        strcpy(popup_message_text, stringBase0 + 0x257);
    } else {
        strcpy(popup_message_text, text);
    }
}


/*
 * Queue / fire a studio event through ScreenMgr.
 * When paused or a controller is removed, events are deferred into
 * paused_event_queue (except a reserved id band) and drained on the next
 * non-deferred call.
 *
 * Retail order: process_events -> broadcast -> fire -> vdestroy.
 * process_events drains then Idle(0). fire queues with raw flag pack;
 * broadcast ends in BroadcastEvent; fire ends in FireEvent.
 */

void screen_engine_process_events(void) {
    int i;
    PausedStudioEvent* base;
    PausedStudioEvent* entry;
    int* flags_ptr;
    int flags;

    /* Soft ceiling: process_events -- drain/frame leftovers; stop. */
    i = 0;
    base = paused_event_queue;
    do {
        entry = &base[i];
        if (entry->event == 0) {
            break;
        }
        flags_ptr = &entry->flags;
        flags = *flags_ptr;
        if (flags < 0) {
            /* Retail: cntlzw(-1 - flags) >> 5  (== 1 iff flags == -1). */
            BroadcastEvent__9ScreenMgrFiii(screen_manager, (int)entry->event,
                                           (int)(__cntlzw(-1 - flags) >> 5), 0);
        } else {
            FireEvent__9ScreenMgrFiiUi(screen_manager, (int)entry->event, flags & 0xFF,
                                       (unsigned int)((flags >> 8) & 0xFF));
        }
        i += 1;
        entry->event = 0;
        *flags_ptr = 0;
    } while (i < 12);

    Idle__9ScreenMgrFi(screen_manager, 0);
}

int broadcast_screen_studio_event(int event, int flag) {
    int deferred;
    int i;
    PausedStudioEvent* base;
    PausedStudioEvent* entry;
    int* flags_ptr;
    int flags;

    deferred = 0;
    if ((((g_game_info.field_04 >> 7) & 1) == 0 && is_controller_removed() != 0) ||
        pause_screen_engine_proc != 0) {
        deferred = 1;
    }

    if (deferred == 0) {
        i = 0;
        base = paused_event_queue;
        do {
            entry = &base[i];
            if (entry->event == 0) {
                break;
            }
            flags_ptr = &entry->flags;
            flags = *flags_ptr;
            if (flags < 0) {
                BroadcastEvent__9ScreenMgrFiii(
                    screen_manager, (int)entry->event,
                    (int)(__cntlzw(-1 - flags) >> 5), 0);
            } else {
                FireEvent__9ScreenMgrFiiUi(screen_manager, (int)entry->event, flags & 0xFF,
                                           (unsigned int)((flags >> 8) & 0xFF));
            }
            i += 1;
            entry->event = 0;
            *flags_ptr = 0;
        } while (i < 12);
    }

    if (deferred != 0) {
        if ((unsigned int)event < 0x3EEu || (unsigned int)event > 0x405u) {
            int left;

            base = paused_event_queue;
            i = 0;
            left = 12;
            do {
                entry = &base[i];
                if (entry->event == 0) {
                    /* Keep both arms (retail subic/subfe + add. path). */
                    entry->event = (unsigned int)event;
                    if ((int)((flag != 0) - 2) < 0) {
                        entry->flags = -1 - (flag == 0);
                    } else {
                        entry->flags =
                            (int)((0xFFFFFFFFu - (unsigned int)(flag == 0)) & 0xFFu);
                    }
                    break;
                }
                if ((unsigned int)event == entry->event) {
                    break;
                }
                i += 1;
                left -= 1;
            } while (left != 0);
        }
        return 0;
    }

    BroadcastEvent__9ScreenMgrFiii(screen_manager, event, flag, 0);
    return 1;
}

/*
 * Same pause/controller deferral as broadcast, but live path is FireEvent
 * (not BroadcastEvent). Deferred flags: if flag>=0 store flag&0xff else raw.
 * Callers: menu / mcardmsg / pselect / settings refresh ids.
 * Soft ceiling: fire ~83% -- deferred queue mtctr vs do/while + -sdata 0; stop.
 */
void fire_screen_studio_event(int event, int flag) {
    int deferred;
    int i;
    PausedStudioEvent* base;
    PausedStudioEvent* entry;
    int* flags_ptr;
    int flags;
    unsigned int uevent;
    int left;
    int off;

    uevent = (unsigned int)event;
    deferred = 0;
    if ((((g_game_info.field_04 >> 7) & 1) == 0 && is_controller_removed() != 0) ||
        pause_screen_engine_proc != 0) {
        deferred = 1;
    }

    if (deferred == 0) {
        i = 0;
        base = paused_event_queue;
        off = 0;
        do {
            entry = (PausedStudioEvent*)((char*)base + off);
            if (entry->event == 0) {
                break;
            }
            flags_ptr = &entry->flags;
            flags = *flags_ptr;
            if (flags < 0) {
                BroadcastEvent__9ScreenMgrFiii(screen_manager, (int)entry->event,
                                               (flags == -1) ? 1 : 0, 0);
            } else {
                FireEvent__9ScreenMgrFiiUi(screen_manager, (int)entry->event, flags & 0xFF,
                                           (unsigned int)((flags >> 8) & 0xFF));
            }
            i += 1;
            entry->event = 0;
            *flags_ptr = 0;
            off += 8;
        } while (i < 12);
    }

    if (deferred != 0) {
        if (uevent < 0x3EEu || uevent > 0x405u) {
            base = paused_event_queue;
            off = 0;
            left = 12;
            do {
                entry = (PausedStudioEvent*)((char*)base + off);
                if (entry->event == 0) {
                    entry->event = uevent;
                    if (flag >= 0) {
                        entry->flags = flag & 0xFF;
                    } else {
                        entry->flags = flag;
                    }
                    return;
                }
                if (uevent == entry->event) {
                    return;
                }
                off += 8;
                left -= 1;
            } while (left != 0);
        }
        return;
    }

    FireEvent__9ScreenMgrFiiUi(screen_manager, event, flag, 0);
}



/*
 * Sleep current proc until screen-engine tick pid 0x9011 is gone.
 * Soft ceiling: wait_for_screen_close ~59.5% -- -sdata 0 forces ha/l for
 * aproc/_mkproc_sleep_ticks (retail @sda21) + larger stmw frame; algo OK.
 */
void wait_for_screen_close(void) {
    float sleep;
    int pid_base;
    MkVtableMkprocLocal* vtbl;

    sleep = kSleepOne;
    /* Retail: lis r31,1; subi r3,r31,0x6fef -> 0x9011. */
    pid_base = 0x10000;
    while (find_mkproc_pid(pid_base - 0x6FEF) != 0) {
        _mkproc_sleep_ticks = sleep;
        vtbl = (MkVtableMkprocLocal*)aproc->hdr.vtbl;
        vtbl->sleep();
    }
}

/*
 * Load screen SSF, then async-load each slash-separated name as scr_<part>.sec
 * into the given section slot (language-aware).
 *
 * Retail keeps @stringBase0 in r31 and passes stringBase0+0x1C9 to sprintf.
 * Soft ceiling: preload_screen_data ~99.6% -- zero/str_base r30/r31; stop.
 * Q6 try: literal "scr_%s.sec" dropped to ~93.8% -- keep stringBase0.
 */
void preload_screen_data(const char* name, int slot) {
    char name_buf[0x100];
    char path_buf[0x80];
    const char* str_base;
    int zero;
    char* slash;
    char* cursor;
    char* strchr_r3;

    /*
     * Soft ceiling: ~99.56% -- zero/str_base r30/r31 NV unreproducible; stop.
     * Tried: decl order, char/pointer zero, assign order, strchr_r3 shape.
     * Q6 literal "scr_%s.sec" -5%; keep stringBase0.
     */
    load_ssf(screen_engine_file_table);
    strncpy(name_buf, name, 0x100);
    name_buf[0xFF] = 0;

    cursor = name_buf;
    strchr_r3 = strchr(cursor, '/');
    /* Keep: li zero, mr slash, addi str_base (order matches; regs swapped). */
    zero = 0;
    str_base = stringBase0;
    slash = strchr_r3;
    while (slash != 0) {
        *slash = (char)zero;
        sprintf(path_buf, str_base + 0x1C9, cursor);
        add_art_section_by_name_async_language(slot, path_buf);
        cursor = slash + 1;
        slash = strchr(cursor, '/');
    }
}

/*
 * Ask ScreenMgr to load `name`, then spawn screen tick + 3 controller procs.
 * share_pdata may be published on screen_engine_client for get_screen_pdata.
 *
 * B18: On LoadScreen success, allocates vtbl_screen_engine MkHdr
 * (size 0x8) and insert_2d_obj's it onto image.screen_obj_list -- that is
 * the Midway must-run path for menu chrome to reach render_2d_objs. Host
 * still fills ScreenClient::LoadScreenSet so ScreenMgr::Render has Screens.
 *
 * Soft ceiling: ~91.6% -- stmw vs split stw + instance branch shape.
 * B21 PPWLS: load_screen("common/memory_card/mc_main", 0x90046, ...) from
 * p_player_profile_whats_loaded_screen; studio events 0x1FB7/0x1FBE refresh/done.
 */
void load_screen(const char* name, int slot, MkHdr* share_pdata, int unload_slot) {
    const char* name_nv;
    int slot_nv;
    MkHdr* share_nv;
    int unload_nv;
    MkHdr* current;
    unsigned int loaded;
    ScreenObj* screen_obj;
    MkProc* tick;
    MkProc* ctrl;
    ScreenCtrlPdata* ctrl_pdata;
    MkProcPauseFlag* pflags;
    int port;

    /* Pin all 4 args into NVs first so prologue can emit stmw r28. */
    name_nv = name;
    slot_nv = slot;
    share_nv = share_pdata;
    unload_nv = unload_slot;

    screen_engine_client.slot = slot_nv;

    if (unload_nv != 0) {
        unload_section_slot(slot_nv);
    }

    current = screen_engine_client.share_pdata;
    if (current != 0) {
        if (current->instance != (unsigned int)screen_engine_client.share_instance) {
            current = 0;
        }
    } else {
        current = 0;
    }

    /* Q1 try: instance->pdata (Ghidra) dropped load_screen ~2% -- keep pdata->instance. */
    if ((share_nv == 0 || current == 0 || share_nv == current) && share_nv != 0) {
        screen_engine_client.share_pdata = share_nv;
        screen_engine_client.share_instance = (int)share_nv->instance;
    }

    loaded = (unsigned int)LoadScreen__9ScreenMgrFPcUi(screen_manager, (char*)name_nv, 1);
    if (loaded == 0) {
        return;
    }
    if (find_mkproc_pid(SCREEN_ENGINE_PID) != 0) {
        return;
    }

    if (apdata == 0) {
        apdata = share_nv;
    }

    target_game_mode = 0x18;

    screen_obj = (ScreenObj*)get_mkhdr(&vtbl_screen_engine, 0x8);
    if (screen_obj != 0) {
        insert_2d_obj(screen_obj);
    }
    if (screen_obj == 0) {
        return;
    }

    tick = _create_mkproc_generic_bigstack(SCREEN_ENGINE_PID, 0x24, p_screen_engine_tick__Fv, 0, 0);
    if (tick == 0) {
        return;
    }

    pflags = (MkProcPauseFlag*)&tick->flags;
    pflags->skip_if_paused = 1;
    mk_insert((MkHdr*)screen_obj, &tick->pdata_list_b);

    for (port = 0; port < 3; port++) {
        ctrl = _create_mkproc_generic_nostack(SCREEN_ENGINE_CTRL_PID, 0x1F,
                                             p_handle_screen_engine_controller__Fv, 0x1C,
                                             (void**)&ctrl_pdata);
        if (ctrl != 0) {
            ctrl_pdata->port = port;
            ctrl_pdata->field0C = 0;
            ctrl_pdata->field10 = 0;
            pflags = (MkProcPauseFlag*)&ctrl->flags;
            pflags->skip_if_paused = 1;
        }
        mk_insert((MkHdr*)ctrl, &tick->pdata_list_b);
    }
}

/* ---- B18d Midway 2D: destroy / render / tick / nav ---- */

static int screen_engine_is_deferred(void) {
    int deferred;

    deferred = 0;
    if ((((g_game_info.field_04 >> 7) & 1) == 0 && is_controller_removed() != 0) ||
        pause_screen_engine_proc != 0) {
        deferred = 1;
    }
    return deferred;
}

static void drain_paused_studio_events(void) {
    int i;
    PausedStudioEvent* base;
    PausedStudioEvent* entry;
    int* flags_ptr;
    int flags;

    i = 0;
    base = paused_event_queue;
    do {
        entry = &base[i];
        if (entry->event == 0) {
            break;
        }
        flags_ptr = &entry->flags;
        flags = *flags_ptr;
        if (flags < 0) {
            BroadcastEvent__9ScreenMgrFiii(screen_manager, (int)entry->event,
                                           (flags == -1) ? 1 : 0, 0);
        } else {
            FireEvent__9ScreenMgrFiiUi(screen_manager, (int)entry->event, flags & 0xFF,
                                       (unsigned int)((flags >> 8) & 0xFF));
        }
        i += 1;
        entry->event = 0;
        *flags_ptr = 0;
    } while (i < 12);
}

/* Retail leaves r3 untouched after memfree (int slot, no explicit return). */
int vdestroy_screen_engine(MkHdr* hdr) {
    hdr->instance = 0;
    mkhdr_memfree(hdr);
}

/*
 * Bridge from image.render_2d_objs (vtbl_screen_engine latch) -> ScreenMgr::Render.
 * PreRender/PostRender / CreateElement supply the actual quads.
 * Soft ceiling: screen_engine_render ~79% -- -sdata 0 pause/state ha/l; stop.
 */
void screen_engine_render(void) {
    int saved_x;
    int saved_y;
    int state;

    if (pause_screen_engine_proc != 0) {
        return;
    }

    get_pfxsystem_widescreen_offset(&saved_x, &saved_y);
    pfxsystem_widescreen_offset(0, 0);
    current_render_state = 0;
    Render__9ScreenMgrFv(screen_manager);

    state = current_render_state;
    if (state == 1) {
        pfx2d_end_render();
    } else if (state == 3) {
        pfx_end_batch();
    } else if (state == 2) {
        pfxfont_end_render();
    }

    current_render_state = 0;
    pfxsystem_widescreen_offset(saved_x, saved_y);
}

void* get_screen_pdata(void) {
    MkHdr* share;

    /* Retail: beq-to-null epilogue + beqlr on instance match. */
    share = screen_engine_client.share_pdata;
    if (share != 0) {
        if (share->instance == (unsigned int)screen_engine_client.share_instance) {
            return share;
        }
        return 0;
    }
    return 0;
}

void screen_share_pdata(MkHdr* share) {
    ScreenEngineClient* client;
    MkHdr* current;

    /*
     * Retail (asm): empty-keep diamond on latch ? bnelr if both live & distinct
     * ? beqlr if share null ? stw pdata via client ? reload client ha/l ? stw instance.
     * Soft: ~95.9% if MWCC won't emit bnelr/beqlr + dual lis; algo OK.
     */
    client = &screen_engine_client;
    current = client->share_pdata;
    if (current != 0) {
        if (current->instance == (unsigned int)client->share_instance) {
            /* keep */
        } else {
            current = 0;
        }
    } else {
        current = 0;
    }
    if (share != 0) {
        if (current != 0) {
            if (share != current) {
                return;
            }
        }
    }
    if (share == 0) {
        return;
    }
    client->share_pdata = share;
    client = &screen_engine_client;
    client->share_instance = (int)share->instance;
}

void pause_screen_engine(int paused) {
    pause_screen_engine_proc = paused;
}

/*
 * Hold-repeat pdata for p_repeat_* (size 0x1C).
 * D-Pad -> p_repeat_button_input; C-stick -> p_repeat_analog_stick_input.
 * switchIndex for C-stick is bit number (0x10..0x13 -> 1<<n in stick mask).
 */
typedef struct RepeatButtonPdata {
    MkHdr hdr;           /* +0x00 */
    int port;            /* +0x08 */
    int switchIndex;     /* +0x0C -- pad switch row or stick bit index */
    int eventId;         /* +0x10 -- studio FireEvent id */
    int delayLeft;       /* +0x14 -- initial frames before repeat */
    int repeating;       /* +0x18 -- 0 = delay phase, 1 = repeating */
} RepeatButtonPdata;

static float p_repeat_analog_stick_input__Fv(void);
static float p_repeat_button_input__Fv(void);

/* Retail: destroy pid 0x9021+plyr, spawn repeat proc, fill pdata.
 * fire_switches inlines this 8x (no shared helper call). */
#define SPAWN_REPEAT_INPUT(port_, plyr_, swIdx_, evt_, fn_)                    \
    do {                                                                       \
        MkProc* _proc;                                                         \
        RepeatButtonPdata* _pdata;                                             \
        if ((port_) != 2) {                                                    \
            destroy_mkprocs_pid(0x9021 + (plyr_));                             \
            _proc = _create_mkproc_generic_nostack(                            \
                0x9021 + (plyr_), 0x1F, (fn_), 0x1C, (void**)&_pdata);        \
            if (_proc != 0) {                                                  \
                _pdata->port = (port_);                                        \
                _pdata->switchIndex = (swIdx_);                                \
                _pdata->eventId = (evt_);                                      \
                _pdata->repeating = 0;                                         \
                _pdata->delayLeft = 0x1E;                                      \
                _proc->sleep_ticks = (float)button_repeat_time;                \
                /* Retail: lbz/rlwimi/stb on flags low byte @+0xa8. */          \
                ((MkProcPauseFlag*)&_proc->flags)->skip_if_paused = 1;         \
            }                                                                  \
        } else {                                                               \
            _proc = 0;                                                         \
        }                                                                      \
        (void)_proc;                                                           \
    } while (0)

#define SPAWN_REPEAT_ANALOG(port_, plyr_, swIdx_, evt_, bit_)                  \
    do {                                                                       \
        MkProc* _proc;                                                         \
        RepeatButtonPdata* _pdata;                                             \
        if ((port_) != 2) {                                                    \
            destroy_mkprocs_pid(0x9021 + (plyr_));                             \
            _proc = _create_mkproc_generic_nostack(                            \
                0x9021 + (plyr_), 0x1F, (void*)p_repeat_analog_stick_input__Fv,\
                0x1C, (void**)&_pdata);                                        \
            if (_proc != 0) {                                                  \
                _pdata->port = (port_);                                        \
                _pdata->switchIndex = (swIdx_);                                \
                _pdata->eventId = (evt_);                                      \
                _pdata->repeating = 0;                                         \
                _pdata->delayLeft = 0x1E;                                      \
                _proc->sleep_ticks = (float)button_repeat_time;                \
                ((MkProcPauseFlag*)&_proc->flags)->skip_if_paused = 1;         \
                s_nRepeatedStickBits |= (bit_);                                \
            }                                                                  \
        }                                                                      \
    } while (0)

/*
 * Map pad edge bits + stick dirs to ScreenMgr::FireEvent studio ids.
 * See SE_EVT_* / pad bit table in mwScreenEngineGlue.h.
 * D-Pad / C-stick edges spawn hold-repeat mkprocs (retail; port 2 skips).
 * Soft ceiling: fire_switches ~86.3% -- float i2f / NV color leftovers; stop.
 */
void screen_engine_fire_switches(int port, unsigned int switches, int plyr_idx) {
    unsigned int bits;
    unsigned int* slotBits;
    float sx;
    float sy;

    if ((switches & 0x1) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_L2, plyr_idx + 1, 0);
    }
    if ((switches & 0x2) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_R2, plyr_idx + 1, 0);
    }
    if ((switches & 0x4) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_L1, plyr_idx + 1, 0);
    }
    if ((switches & 0x8) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_R1, plyr_idx + 1, 0);
    }
    if ((switches & 0x10) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_FACE_Y, plyr_idx + 1, 0);
    }
    if ((switches & 0x20) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_FACE_X, plyr_idx + 1, 0);
    }
    if ((switches & 0x40) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_CONFIRM, plyr_idx + 1, 0);
    }
    if ((switches & 0x80) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_BACK, plyr_idx + 1, 0);
    }
    if ((switches & 0x100) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_SELECT, plyr_idx + 1, 0);
    }
    if ((switches & 0x800) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_START, plyr_idx + 1, 0);
    }
    if ((switches & 0x1000) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_DPAD_UP, plyr_idx + 1, 0);
        SPAWN_REPEAT_INPUT(port, plyr_idx, 0xC, SE_EVT_DPAD_UP,
                           (void*)p_repeat_button_input__Fv);
    }
    if ((switches & 0x2000) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_DPAD_RIGHT, plyr_idx + 1, 0);
        SPAWN_REPEAT_INPUT(port, plyr_idx, 0xD, SE_EVT_DPAD_RIGHT,
                           (void*)p_repeat_button_input__Fv);
    }
    if ((switches & 0x4000) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_DPAD_DOWN, plyr_idx + 1, 0);
        SPAWN_REPEAT_INPUT(port, plyr_idx, 0xE, SE_EVT_DPAD_DOWN,
                           (void*)p_repeat_button_input__Fv);
    }
    if ((switches & 0x8000) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_DPAD_LEFT, plyr_idx + 1, 0);
        SPAWN_REPEAT_INPUT(port, plyr_idx, 0xF, SE_EVT_DPAD_LEFT,
                           (void*)p_repeat_button_input__Fv);
    }

    bits = 0;
    if (get_stick_pos(port, 1, &sx, &sy) != 0) {
        if (sx < 0.0f) {
            bits |= 0x80000u;
        }
        if (sx > 0.0f) {
            bits |= 0x20000u;
        }
        if (sy < 0.0f) {
            bits |= 0x10000u;
        }
        if (sy > 0.0f) {
            bits |= 0x40000u;
        }
    }
    if (get_stick_pos(port, 0, &sx, &sy) != 0) {
        if (sx < 0.0f) {
            bits |= 0x800000u;
        }
        if (sx > 0.0f) {
            bits |= 0x200000u;
        }
        if (sy < 0.0f) {
            bits |= 0x100000u;
        }
        if (sy > 0.0f) {
            bits |= 0x400000u;
        }
        /* Retail: two rlwinm clears (not a single ~0xA00000 / ~0x500000). */
        if (sx == 0.0f) {
            s_nRepeatedStickBits &= ~0x800000u;
            s_nRepeatedStickBits &= ~0x200000u;
        }
        if (sy == 0.0f) {
            s_nRepeatedStickBits &= ~0x100000u;
            s_nRepeatedStickBits &= ~0x400000u;
        }
    }

    /* Retail: stick_bits + ((plyr+1)<<2) - 4, then lwzu for bit tests. */
    slotBits = &stick_bits[plyr_idx + 1];
    slotBits[-1] = bits & ~s_nRepeatedStickBits;
    slotBits -= 1;

    if ((*slotBits & 0x10000u) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_CSTICK_DOWN, plyr_idx + 1, 0);
        SPAWN_REPEAT_ANALOG(port, plyr_idx, 0x10, SE_EVT_CSTICK_DOWN, 0x10000u);
    }
    if ((*slotBits & 0x20000u) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_CSTICK_RIGHT, plyr_idx + 1, 0);
        SPAWN_REPEAT_ANALOG(port, plyr_idx, 0x11, SE_EVT_CSTICK_RIGHT, 0x20000u);
    }
    if ((*slotBits & 0x40000u) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_CSTICK_UP, plyr_idx + 1, 0);
        SPAWN_REPEAT_ANALOG(port, plyr_idx, 0x12, SE_EVT_CSTICK_UP, 0x40000u);
    }
    if ((*slotBits & 0x80000u) != 0) {
        FireEvent__9ScreenMgrFiiUi(screen_manager, SE_EVT_CSTICK_LEFT, plyr_idx + 1, 0);
        SPAWN_REPEAT_ANALOG(port, plyr_idx, 0x13, SE_EVT_CSTICK_LEFT, 0x80000u);
    }
}

/*
 * C-stick hold-repeat: re-sample sticks; FireEvent while bit still held.
 * Soft ceiling: ~63% -- -sdata 0 float/SDA + oris vs ori bit set; stop.
 */
static float p_repeat_analog_stick_input__Fv(void) {
    RepeatButtonPdata* pdata;
    GcPadSlot* slot;
    int user;
    int port;
    unsigned int bits;
    float sx;
    float sy;

    user = -1;
    pdata = (RepeatButtonPdata*)apdata;
    if (pdata == 0) {
        s_nRepeatedStickBits = 0;
        return kSleepNegOne;
    }
    if (check_allow_screen_engine_control__Fv() != 0) {
        s_nRepeatedStickBits = 0;
        return kSleepNegOne;
    }

    port = pdata->port;
    slot = &g_game_info.pads[port];
    if (slot->player != 0) {
        user = slot->player->controller_slot + 1;
    }

    bits = 0;
    if (get_stick_pos(port, 1, &sx, &sy) != 0) {
        if (sx < 0.0f) {
            bits |= 0x80000u;
        }
        if (sx > 0.0f) {
            bits |= 0x20000u;
        }
        if (sy < 0.0f) {
            bits |= 0x10000u;
        }
        if (sy > 0.0f) {
            bits |= 0x40000u;
        }
    }
    if (get_stick_pos(port, 0, &sx, &sy) != 0) {
        if (sx < 0.0f) {
            bits |= 0x800000u;
        }
        if (sx > 0.0f) {
            bits |= 0x200000u;
        }
        if (sy < 0.0f) {
            bits |= 0x100000u;
        }
        if (sy > 0.0f) {
            bits |= 0x400000u;
        }
        if (sx == 0.0f) {
            s_nRepeatedStickBits &= ~0x800000u;
            s_nRepeatedStickBits &= ~0x200000u;
        }
        if (sy == 0.0f) {
            s_nRepeatedStickBits &= ~0x100000u;
            s_nRepeatedStickBits &= ~0x400000u;
        }
    }

    if (((1u << pdata->switchIndex) & bits) == 0) {
        s_nRepeatedStickBits = 0;
        return kSleepNegOne;
    }

    FireEvent__9ScreenMgrFiiUi(screen_manager, pdata->eventId, user, 0);
    return (float)button_repeat_time;
}

/*
 * Soft ceiling: p_repeat_button_input ~65% -- -sdata 0 float/SDA leftovers; stop.
 */
static float p_repeat_button_input__Fv(void) {
    RepeatButtonPdata* pdata;
    GcPadSlot* slot;
    int user;
    float sleep;

    pdata = (RepeatButtonPdata*)apdata;
    if (pdata == 0) {
        return kSleepNegOne;
    }
    if (check_allow_screen_engine_control__Fv() != 0) {
        return kSleepNegOne;
    }

    user = -1;
    slot = &g_game_info.pads[pdata->port];
    if (slot->player != 0) {
        user = slot->player->controller_slot + 1;
    }

    if (pdata->repeating == 0) {
        if (pdata->delayLeft > 0) {
            if (check_switch(pdata->port, pdata->switchIndex) == 0) {
                return kSleepNegOne;
            }
            pdata->delayLeft -= 1;
            return kSleepOne;
        }
        pdata->repeating = 1;
    }

    if (check_switch(pdata->port, pdata->switchIndex) == 0) {
        return kSleepNegOne;
    }
    FireEvent__9ScreenMgrFiiUi(screen_manager, pdata->eventId, user, 0);
    sleep = (float)button_repeat_time;
    return sleep;
}

/*
 * Latch target_game_mode; for modes 6..21 optionally set_player_state.
 * Twin of HandleAction SE_ACT_SET_TARGET_GAME_MODE (uses global menu_player).
 * Retail: dense switch on mode -> (mode-6) jump table @8152.
 * Soft ceiling: ~99.7% -- jump-table reloc label only; stop.
 */
void set_target_game_mode(int menu_player_arg, int mode) {
    target_game_mode = mode;
    switch (mode) {
    case 6:
    case 8:
    case 9:
    case 11:
    case 21:
        if (menu_player_arg == 0) {
            set_player_state(&g_game_info.plyr0, 1);
            set_player_state(&g_game_info.plyr1, 0);
        } else if (menu_player_arg == 1) {
            set_player_state(&g_game_info.plyr1, 1);
            set_player_state(&g_game_info.plyr0, 0);
        }
        break;
    case 7:
        set_player_state(&g_game_info.plyr0, 1);
        set_player_state(&g_game_info.plyr1, 1);
        break;
    case 10:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
        break;
    }
}

/* Returns 1 when ScreenEngine input must be suppressed. */
/*
 * Per-port pad -> screen_engine_fire_switches. Spawned by load_screen (pids 0x901F+).
 * Defined before check_allow so MWCC emits `bl` (retail order) instead of inlining.
 *
 * Soft ceiling: ~73% -- -sdata 0 apdata/float ha/l vs @sda21; stop.
 */
float p_handle_screen_engine_controller__Fv(void) {
    ScreenCtrlPdata* ctrl;
    GameInfo* gi;
    char* pad_base;
    PlyrInfo* player;
    int plyr_idx;
    int port;
    int pad_off;
    unsigned char flags;
    unsigned int edge;

    plyr_idx = -1;
    ctrl = (ScreenCtrlPdata*)apdata;
    if (ctrl == 0) {
        return kSleepNegOne;
    }
    if (check_allow_screen_engine_control__Fv() != 0) {
        return kSleepOne;
    }

    gi = &g_game_info;
    /*
     * Soft ceiling: ~73-81% -- -sdata 0 apdata/float ha/l vs @sda21; stop.
     * Note: gi->pads[port].* typed form dropped fuzzy (~74% vs ~81%) -- retail
     * reloads &g_game_info for edge; keep (char*)+stride open-code (Q flip).
     */
    if (((gi->pause_flags >> 1) & 1) == 0) {
        port = ctrl->port;
        pad_off = port * GC_PAD_SLOT_STRIDE;
        pad_base = (char*)gi + pad_off;
        flags = *(unsigned char*)(pad_base + GC_PAD_SLOTS_OFF);
        if (((flags >> 7) & 1) == 0) {
            if (((flags >> 6) & 1) != 0) {
                player = *(PlyrInfo**)(pad_base + 0x184);
                if (player != 0) {
                    plyr_idx = player->controller_slot;
                }
                edge = *(unsigned int*)((char*)&g_game_info + pad_off + 0x190);
                screen_engine_fire_switches(port, edge, plyr_idx);
            }
        }
    }
    return kSleepOne;
}

/*
 * ScreenEngine frame tick (pid 0x9011). Drains deferred studio events, then
 * ScreenMgr::Idle + UpdateAnimations. Returns -1 when stack empty (exits wait).
 */
float p_screen_engine_tick__Fv(void) {
    ScreenMgrActive* mgr;
    int dt;
    int deferred;
    int rr;

    rr = refresh_rate();
    dt = (int)(kMsPerSec / (float)rr);

    mgr = (ScreenMgrActive*)screen_manager;
    if (mgr->active_count < 0) {
        movie_player_reset();
        Dispose__9ScreenMgrFUi(screen_manager, 1);
        fxbanks_unload_by_owner(8);
        return kSleepNegOne;
    }

    deferred = screen_engine_is_deferred();
    if (deferred == 0) {
        drain_paused_studio_events();
    }
    if (deferred != 0) {
        return kSleepOne;
    }

    drain_paused_studio_events();
    Idle__9ScreenMgrFi(screen_manager, dt);
    UpdateAnimations__9ScreenMgrFi(screen_manager, dt);
    mkMovieTexPlayerIdleUpdate();
    return kSleepOne;
}

int check_allow_screen_engine_control__Fv(void) {
    int deferred;
    GameInfo* gi;

    gi = &g_game_info;
    deferred = 0;
    if ((((gi->field_04 >> 7) & 1) == 0 && is_controller_removed() != 0) ||
        pause_screen_engine_proc != 0) {
        deferred = 1;
    }
    if (deferred == 0) {
        drain_paused_studio_events();
    }

    if (deferred == 0) {
        if (((gi->field_04 >> 7) & 1) != 0) {
            if (is_controller_removed() != 0 || pause_screen_engine_proc != 0) {
                return 1;
            }
        }
    }
    return deferred;
}

/*
 * Boot once: ScreenMgr::Init(client) + ScreenControl::RegisterGameVariables.
 * Retail addi from paused_event_queue via ScreenEngineBssIsland overlay
 * (+0x6C mgr / +0x2DC client / +0x368 vars; @814/@815/@816 pads).
 *
 * Soft ceiling: ~99.1% -- objdiff flags addi reloc args; bytes match size.
 */
void init_screen_engine(void) {
    ScreenEngineBssIsland* island;

    island = (ScreenEngineBssIsland*)paused_event_queue;
    Init__9ScreenMgrFP12ScreenClient(island->screen_manager, &island->client);
    RegisterGameVariables__13ScreenControlFUiP13GameVariables(0, &island->game_variables);
}

/*
 * =====================================================================
 * mkGameVariables -- game subclass of GameVariables (Glue TU).
 * Layout 0x1C; vtbl __vt__15mkGameVariables. The full game-facing virtual
 * surface is lifted here.
 * =====================================================================
 */

/* Retail @4397 -- shared 0.0f in Glue .sdata2. */
static const float kGvFloatZero = 0.0f;

extern void Free__10ScreenUtilFPv(void* p);

void FreeTextureCollection__15mkGameVariablesFiP15GMTextureInfo_t(
    mkGameVariables* /*self*/, int /*id*/, GMTextureInfo_t* info) {
    if (info->data != 0) {
        Free__10ScreenUtilFPv(info->data);
    }
    info->ready = 0;
    info->data = 0;
}

void* memset(void* dst, int value, unsigned long size);
void create_inventory_image_list(GVTexturePair out, unsigned int count);
int get_number_items_in_inventory(void);
int get_num_pselect_body_textures(void);
void get_pselect_body_textures(GVTexturePair out);
void get_pselect_head_textures(GVTexturePair out);
void get_bg_pselect_team_textures(GVTexturePair out, int team);
int get_num_selectable_bgnds(void);
void get_background_select_textures(GVTexturePair out);
int controller_get_num_adjustable_buttons(void);
void cconfig_get_button_textures(GVTexturePair out);
void get_pz_special_move_list(GVTexturePair out, int player);
void create_left_mc_icon_list(GVTexturePair out, int count);
void create_right_mc_icon_list(GVTexturePair out, int count);
int get_number_kontent_items(void);
void create_gallery_image_list(GVTexturePair out, unsigned int count);
void create_fullscreen_gallery_image_list(GVTexturePair out, int count);
int ppl_get_multi_profile_count(int player);
void ppl_get_multi_profile_icon_p1(GVTexturePair out, unsigned int count);
void ppl_get_multi_profile_icon_p2(GVTexturePair out, unsigned int count);
int get_num_modeselect_portraits(void);
void get_modeselect_portrait_list(GVTexturePair out);
void ppv_view_profile_icon_list(GVTexturePair out);

#define ALLOC_GV_TEXTURE_COLLECTION(itemCount, itemCapacity)                         \
    do {                                                                            \
        pointerBytes = (itemCapacity) * sizeof(void*);                               \
        allocSize = sizeof(GVTextureCollection) + pointerBytes;                      \
        allocSize += pointerBytes;                                                   \
        collection = (GVTextureCollection*)Malloc__10ScreenUtilFUliPc(               \
            allocSize, 0, (char*)(stringBase0 + 0x1D4));                             \
        memset(collection, 0, allocSize);                                             \
        collection->colors = (RwTexture**)(collection + 1);                          \
        collection->alphas = collection->colors + (itemCapacity);                    \
        out->data = collection;                                                      \
    } while (0)

int GetTextureCollection__15mkGameVariablesFiP15GMTextureInfo_tRUi(
    mkGameVariables* /*self*/, int id, GMTextureInfo_t* out,
    unsigned int* columnsOut) {
    GVTextureCollection* collection;
    GVTexturePair pair;
    unsigned int count = 0;
    unsigned long allocSize;
    unsigned long pointerBytes;

    /*
     * Runtime-complete retail dispatcher. The repeated allocation/setup in
     * retail is represented by a typed allocation macro; each collection is
     * one 0x30-byte header followed by parallel color/alpha pointer arrays.
     * The image-list helpers take GVTexturePair by value. MWCC materializes
     * one addressable argument copy per case, matching retail's 0xA0 frame.
     * Soft ceiling: ~97.66% -- three equivalent arithmetic emit islands
     * remain (gallery rounding, team selector, special-move selector).
     */
    switch (id) {
    case 0x48:
        count = get_number_items_in_inventory();
        if (count < 12) {
            ALLOC_GV_TEXTURE_COLLECTION(count, 12);
        } else {
            ALLOC_GV_TEXTURE_COLLECTION(count, count);
        }
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        create_inventory_image_list(pair, count);
        *columnsOut = (count + 1) >> 1;
        count = 2;
        break;
    case 0x1fe9:
        count = get_number_kontent_items();
        if (count < 12) {
            ALLOC_GV_TEXTURE_COLLECTION(count, 12);
        } else {
            ALLOC_GV_TEXTURE_COLLECTION(count, count);
        }
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        create_gallery_image_list(pair, count);
        *columnsOut = 6;
        count = (count + (*columnsOut - 1)) / *columnsOut;
        break;
    case 0x1fea:
        count = 2;
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        create_fullscreen_gallery_image_list(pair, 2);
        *columnsOut = 2;
        count = (count + (*columnsOut - 1)) / *columnsOut;
        break;
    case 0x1fda:
        count = 0x2c;
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        get_pselect_head_textures(pair);
        break;
    case 0x1fdc:
    case 0x1fdd:
        count = 10;
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        get_bg_pselect_team_textures(pair, id - 0x1fdc);
        break;
    case 0x1fd7:
    case 0x1fd8:
    case 0x1fd9:
        count = get_num_pselect_body_textures();
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        get_pselect_body_textures(pair);
        break;
    case 0x1fde:
        count = get_num_selectable_bgnds();
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        get_background_select_textures(pair);
        break;
    case 0x2454:
        count = get_num_modeselect_portraits();
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        get_modeselect_portrait_list(pair);
        break;
    case 0x1fe2:
        count = 7;
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        create_left_mc_icon_list(pair, 7);
        *columnsOut = 1;
        break;
    case 0x1fe3:
        count = 7;
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        create_right_mc_icon_list(pair, 7);
        *columnsOut = 1;
        break;
    case 0x1fe0:
    case 0x1fe1:
        count = 12;
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        get_pz_special_move_list(pair, id == 0x1fe0);
        break;
    case 0x1fdf:
        count = controller_get_num_adjustable_buttons();
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        cconfig_get_button_textures(pair);
        break;
    case 0x300e:
        count = 7;
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        ppv_view_profile_icon_list(pair);
        *columnsOut = 1;
        break;
    case 0x1fed:
        count = ppl_get_multi_profile_count(0);
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        ppl_get_multi_profile_icon_p1(pair, count);
        *columnsOut = 1;
        if (count == 0) {
            count = 1;
        }
        break;
    case 0x1fee:
        count = ppl_get_multi_profile_count(1);
        ALLOC_GV_TEXTURE_COLLECTION(count, count);
        out->data->count = count;
        pair.colors = out->data->colors;
        pair.alphas = out->data->alphas;
        ppl_get_multi_profile_icon_p2(pair, count);
        *columnsOut = 1;
        if (count == 0) {
            count = 1;
        }
        break;
    }
    return count;
}

#undef ALLOC_GV_TEXTURE_COLLECTION

int GetNumStrings__15mkGameVariablesFi(mkGameVariables* /*self*/, int /*id*/) {
    return 0;
}

void FreeStringCollection__15mkGameVariablesFiPPcUi(mkGameVariables* /*self*/, int /*id*/,
                                                    char** /*strings*/,
                                                    unsigned int /*count*/) {}

extern char* menu_string_matrix[];
extern char* pselect_string_matrix[];
extern char* wager_p1_koin_count_string_matrix[];
extern char* wager_p2_koin_count_string_matrix[];
extern char* profile_test_matrix_a[];
extern char* profile_test_matrix_b[];
extern char* view_profile_stats[];
extern char* multi_profile_names_p1[];
extern char* multi_profile_names_p2[];
#if !defined(TARGET_PC)
#pragma section sdata_type ".sdata" ".sbss" data_mode=sda_rel
__declspec(section ".sdata") extern char* create_profile_storage_location_name_list[];
#pragma section sdata_type
#else
extern char* create_profile_storage_location_name_list[];
#endif
extern char* game_settings_rounds_to_win_numbers[];
extern char* number_strings[];
extern int ui_sound_table[];

typedef struct GVStringMatrixIsland {
    unsigned char pad000[0x6B0];
    char* menu[6];             /* +0x6B0 */
    char* pselect[20];         /* +0x6C8 */
    char* wagerP1[6];          /* +0x718 */
    char* wagerP2[6];          /* +0x730 */
    char* profileTestA[7];     /* +0x748 */
    char* profileTestB[7];     /* +0x764 */
    char* viewProfileStats[9]; /* +0x780 */
    char* multiProfileP1[14];  /* +0x7A4 */
    char* multiProfileP2[14];  /* +0x7DC */
} GVStringMatrixIsland;

void get_left_mcard_text_matrix(char** out);
void get_right_mcard_text_matrix(char** out);
void* get_movelist_strings(unsigned int* out_max);
int ppl_get_multi_profile_names_p1(char** out);
int ppl_get_multi_profile_names_p2(char** out);
int wager_load_koin_count_string_array__F8PLYR_NUM(int player);
extern int p1_profile_status;
extern int p2_profile_status;
extern unsigned char p1_profile[0x5c0];
extern unsigned char p2_profile[0x5c0];
void get_soundtrack_title_list(const char*** titles_out, unsigned int* count_out,
                               int* stride_out);
void get_storage_device_name_list(char** out);
void get_profile_stats(char** outs);

int GetStringMatrixCollection__15mkGameVariablesFiPPPcRi(
    mkGameVariables* /*self*/, int id, char*** out, int* rows) {
    /*
     * Soft ceiling: ~99.59% -- instructions match; the typed contiguous matrix
     * view gives equivalent base+offset addresses with different reloc identities.
     */
    GVStringMatrixIsland* matrices = (GVStringMatrixIsland*)ui_sound_table;
    unsigned int count = 0;

    switch (id) {
    case 1:
        *out = matrices->menu;
        *rows = 1;
        count = 6;
        break;
    case 0x50:
        *out = matrices->menu;
        *rows = 1;
        count = 5;
        break;
    case 4:
        *out = matrices->menu;
        *rows = 1;
        count = 3;
        break;
    case 0x65:
        *out = (char**)get_movelist_strings(&count);
        *rows = 3;
        break;
    case 0x3c:
        *out = matrices->pselect;
        *rows = 10;
        count = 2;
        break;
    case 0x1fe4:
        get_left_mcard_text_matrix(matrices->profileTestA);
        *out = matrices->profileTestA;
        *rows = 1;
        count = 7;
        break;
    case 0x1fe5:
        get_right_mcard_text_matrix(matrices->profileTestB);
        *out = matrices->profileTestB;
        *rows = 1;
        count = 7;
        break;
    case 0x300c:
        get_storage_device_name_list(create_profile_storage_location_name_list);
        *out = create_profile_storage_location_name_list;
        *rows = 1;
        count = 2;
        break;
    case 0x300d:
        get_profile_stats(matrices->viewProfileStats);
        *out = matrices->viewProfileStats;
        *rows = 3;
        count = 3;
        break;
    case 0x1feb:
        *rows = 1;
        count = ppl_get_multi_profile_names_p1(matrices->multiProfileP1);
        *out = matrices->multiProfileP1;
        if (count == 0) {
            count = 1;
        }
        break;
    case 0x1fec:
        *rows = 1;
        count = ppl_get_multi_profile_names_p2(matrices->multiProfileP2);
        *out = matrices->multiProfileP2;
        if (count == 0) {
            count = 1;
        }
        break;
    case 0x1fef:
        if (wager_load_koin_count_string_array__F8PLYR_NUM(0) != 0) {
            *out = matrices->wagerP1;
            count = 6;
            *rows = 1;
        } else {
            *rows = 1;
            count = 1;
        }
        break;
    case 0x1ff0:
        if (wager_load_koin_count_string_array__F8PLYR_NUM(1) != 0) {
            *out = matrices->wagerP2;
            count = 6;
            *rows = 1;
        } else {
            *rows = 1;
            count = 1;
        }
        break;
    case 0x1ff2:
        get_soundtrack_title_list((const char***)out, &count, rows);
        return count;
    case 0:
        return 0;
    }
    return count;
}

#pragma optimization_level 3
int wager_load_koin_count_string_array__F8PLYR_NUM(int player) {
    char buffer[80];

    if (player == 0 && p1_profile_status == 1) {
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p1_profile + 0x54));
        strcpy(wager_p1_koin_count_string_matrix[0], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p1_profile + 0x50));
        strcpy(wager_p1_koin_count_string_matrix[1], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p1_profile + 0x4C));
        strcpy(wager_p1_koin_count_string_matrix[2], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p1_profile + 0x48));
        strcpy(wager_p1_koin_count_string_matrix[3], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p1_profile + 0x44));
        strcpy(wager_p1_koin_count_string_matrix[4], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p1_profile + 0x40));
        strcpy(wager_p1_koin_count_string_matrix[5], buffer);
        return 1;
    } else if (player == 1 && p2_profile_status == 1) {
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p2_profile + 0x54));
        strcpy(wager_p2_koin_count_string_matrix[0], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p2_profile + 0x50));
        strcpy(wager_p2_koin_count_string_matrix[1], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p2_profile + 0x4C));
        strcpy(wager_p2_koin_count_string_matrix[2], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p2_profile + 0x48));
        strcpy(wager_p2_koin_count_string_matrix[3], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p2_profile + 0x44));
        strcpy(wager_p2_koin_count_string_matrix[4], buffer);
        sprintf(buffer, stringBase0 + 0x1E5, *(int*)(p2_profile + 0x40));
        strcpy(wager_p2_koin_count_string_matrix[5], buffer);
        return 1;
    }
    return 0;
}
#pragma optimization_level 4

int GetStringCollection__15mkGameVariablesFiPPPc(
    mkGameVariables* /*self*/, int id, char*** out) {
    /*
     * Soft ceiling: ~99.46% -- instructions match; anonymous retail
     * @stringBase0 relocation identity differs in the source object.
     */
    char** strings;
    int i;

    switch (id) {
    case 0x33f4:
        strings = (char**)Malloc__10ScreenUtilFUliPc(
            5 * sizeof(char*), 0, (char*)(stringBase0 + 0x1E8));
        *out = strings;
        for (i = 0; i < 5; i++) {
            strings[i] = get_string_by_id((unsigned int)(i + 6) | 0x10000);
        }
        return 5;
    case 0x33f5:
        *out = game_settings_rounds_to_win_numbers;
        return 4;
    case 0x33f6:
        *out = number_strings;
        return 100;
    case 0x33f7:
        strings = (char**)Malloc__10ScreenUtilFUliPc(
            2 * sizeof(char*), 0, (char*)(stringBase0 + 0x1E8));
        *out = strings;
        for (i = 0; i < 2; i++) {
            strings[i] = get_string_by_id((unsigned int)(i + 0xb) | 0x10000);
        }
        return 2;
    case 0x33f8:
        strings = (char**)Malloc__10ScreenUtilFUliPc(
            4 * sizeof(char*), 0, (char*)(stringBase0 + 0x1E8));
        *out = strings;
        for (i = 0; i < 4; i++) {
            strings[i] = get_string_by_id((unsigned int)(i + 0xd) | 0x10000);
        }
        return 4;
    case 0x37dd:
        strings = (char**)Malloc__10ScreenUtilFUliPc(
            2 * sizeof(char*), 0, (char*)(stringBase0 + 0x206));
        *out = strings;
        for (i = 0; i < 2; i++) {
            strings[i] = get_string_by_id((unsigned int)(i + 0xb) | 0x10000);
        }
        return 2;
    case 0x37dc:
        strings = (char**)Malloc__10ScreenUtilFUliPc(
            2 * sizeof(char*), 0, (char*)(stringBase0 + 0x225));
        *out = strings;
        for (i = 0; i < 2; i++) {
            strings[i] = get_string_by_id((unsigned int)(i + 0xb) | 0x10000);
        }
        return 2;
    }
    return 0;
}

void SetColState__15mkGameVariablesFiii(mkGameVariables* /*self*/, int /*id*/, int /*col*/,
                                        int /*value*/) {}

int GetColState__15mkGameVariablesFii(mkGameVariables* /*self*/, int /*id*/, int /*col*/) {
    return 0;
}

void SetRowState__15mkGameVariablesFiii(mkGameVariables* /*self*/, int /*id*/, int /*row*/,
                                        int /*value*/) {}

int GetRowState__15mkGameVariablesFii(mkGameVariables* /*self*/, int /*id*/, int /*row*/) {
    return 0;
}

extern int multi_profile_cursor_p1;
extern int multi_profile_cursor_p2;
void konquest_set_current_inventory_item(int item);
void kontent_set_current_selection(int item);
void set_current_soundtrack(int index);
int get_current_soundtrack(void);
int get_number_items_in_inventory(void);

void SetIntArray__15mkGameVariablesFiPii(
    mkGameVariables* /*self*/, int id, int* values, int /*count*/) {
    switch (id) {
    case 3:
        break;
    case 0x4a: {
        unsigned int columns = ((unsigned int)get_number_items_in_inventory() + 1) >> 1;
        konquest_set_current_inventory_item(values[2] + values[3] * columns);
        break;
    }
    case 0x1fcf:
        kontent_set_current_selection(values[2] + values[3] * 6);
        break;
    case 0x1fda:
        multi_profile_cursor_p1 = values[3] + values[2];
        break;
    case 0x1fdb:
        multi_profile_cursor_p2 = values[3] + values[2];
        break;
    case 0x1fea:
        set_current_soundtrack(values[3]);
        break;
    }
}

void GetIntArray__15mkGameVariablesFiPii(
    mkGameVariables* /*self*/, int id, int* values, int /*count*/) {
    switch (id) {
    case 0x1fea:
        values[3] = get_current_soundtrack();
        values[2] = 0;
        break;
    }
}

void SetString__15mkGameVariablesFiPc(mkGameVariables* /*self*/, int /*id*/, char* /*str*/) {}

static char temp_string_buf_3481[0x100];
extern const char* mk6_version_string;
extern char popup_title_text[0x100];
extern char popup_options_text[0x100];
extern int psel_p1_handicap;
extern int psel_p2_handicap;

char* get_controller_vibration_string(int player);
char* pselect_get_player_name(int player);
int get_konq_profile_value(int type, int index);
void format_value_to_display(char* dest, int value);
char* locate_inventory_text(int page);
char* get_heros_name(int which);
char* movelist_get_character_name(void);
char* movelist_get_counter(void);
char* pselect_get_style_name(int player, int style);
char* pselect_get_difficulty_level(int player);
char* pselect_get_arena_name(void);
const char* get_p1_player_name(void);
const char* get_p2_player_name(void);
char* get_left_storage_device_name(void);
char* get_right_storage_device_name(void);
char* get_left_storage_device_space_needed(void);
char* get_right_storage_device_space_needed(void);
char* get_current_create_a_profile_name(void);
void get_gallery_page_number_string(char* out);
char* get_coffin_blurb(void);
char* get_long_coffin_description(void);
const char* get_current_soundtrack_title(void);
const char* get_current_soundtrack_composer(void);
const char* get_current_soundtrack_description(void);
int get_continue_timer(void);
const char* get_screens_online_options_newaccountname(void);
const char* get_screens_online_options_newaccountpassword(void);
char* ppv_get_current_profile_name(void);
void ppv_get_current_profile_arcade_finishes(char* dest);
void ppv_get_current_profile_koins(char* dest, int index);

char* GetString__15mkGameVariablesFi(mkGameVariables* /*self*/, int id) {
    /* Soft ceiling: ~53.4% -- complete ID/call inventory; retail binary cmp tree. */
    char* result = temp_string_buf_3481;
    result[0] = 0;

    switch (id) {
    case 0x46:
        sprintf(result, "%s", get_heros_name(0));
        return result;
    case 0x47:
        return result;
    case 0x48:
        format_value_to_display(result, get_konq_profile_value(0xc, 0));
        return result;
    case 0x49:
        sprintf(result, "%s", locate_inventory_text(0));
        return result;
    case 0x4b:
        sprintf(result, "%s", locate_inventory_text(1));
        return result;
    case 0x4c:
        format_value_to_display(result, get_konq_profile_value(7, 0));
        return result;
    case 0x4d:
        format_value_to_display(result, get_konq_profile_value(8, 0));
        return result;
    case 0x4e:
        format_value_to_display(result, get_konq_profile_value(9, 0));
        return result;
    case 0x4f:
        format_value_to_display(result, get_konq_profile_value(10, 0));
        return result;
    case 0x50:
        format_value_to_display(result, get_konq_profile_value(0xb, 0));
        return result;
    case 0x96:
        return movelist_get_character_name();
    case 0x97:
        return movelist_get_counter();
    case 200:
        result[0] = 0;
        return (char*)mk6_version_string;
    case 0x1f74:
        return pselect_get_player_name(0);
    case 0x1f75:
        return pselect_get_player_name(1);
    case 0x1f95:
        return pselect_get_arena_name();
    case 0x1f96:
        return result;
    case 0x1f97:
        return pselect_get_difficulty_level(0);
    case 0x1f98:
        return pselect_get_difficulty_level(1);
    case 0x1f99:
    case 0x1f9a:
    case 0x1f9b:
        return pselect_get_style_name(0, id - 0x1f99);
    case 0x1f9c:
    case 0x1f9d:
    case 0x1f9e:
        return pselect_get_style_name(1, id - 0x1f9c);
    case 0x1fa7:
        return (char*)get_p1_player_name();
    case 0x1fa8:
        return (char*)get_p2_player_name();
    case 0x1faa:
        result[0] = 0;
        return popup_title_text;
    case 0x1fab:
        result[0] = 0;
        return (char*)"uninitalized";
    case 0x1fad:
        result[0] = 0;
        return popup_options_text;
    case 0x1fb1:
        return get_left_storage_device_name();
    case 0x1fb2:
        return get_right_storage_device_name();
    case 0x1fbc:
        return get_controller_vibration_string(0);
    case 0x1fc3:
        return get_controller_vibration_string(1);
    case 0x1fca:
        sprintf(result, "%d%%", psel_p1_handicap);
        return result;
    case 0x1fcb:
        sprintf(result, "%d%%", psel_p2_handicap);
        return result;
    case 0x1fcc:
        sprintf(result, "%s", get_coffin_blurb());
        return result;
    case 0x1fce:
        get_gallery_page_number_string(result);
        return result;
    case 0x1fd1:
        return get_current_create_a_profile_name();
    case 0x1fd8:
        sprintf(result, "%s", get_long_coffin_description());
        return result;
    case 0x1fdc:
        sprintf(result, "%d", *(int*)((char*)&g_game_info + 0x214));
        return result;
    case 0x1fe5:
        sprintf(result, "%d", *(int*)((char*)&g_game_info + 0x1d8));
        return result;
    case 0x1feb:
        return (char*)get_current_soundtrack_title();
    case 0x1fec:
        return (char*)get_current_soundtrack_composer();
    case 0x1fed:
        return (char*)get_current_soundtrack_description();
    case 0x1fef:
        sprintf(result, "%d", get_continue_timer());
        return result;
    case 0x1ff7:
        return get_left_storage_device_space_needed();
    case 0x1ff8:
        return get_right_storage_device_space_needed();
    case 0x2719:
        return (char*)get_screens_online_options_newaccountname();
    case 0x271a:
        return (char*)get_screens_online_options_newaccountpassword();
    case 0x2f47:
        ppv_get_current_profile_arcade_finishes(result);
        return result;
    case 0x2f48:
        return ppv_get_current_profile_name();
    case 0x2f49:
        ppv_get_current_profile_koins(result, 5);
        return result;
    case 0x2f4a:
        ppv_get_current_profile_koins(result, 4);
        return result;
    case 0x2f4b:
        ppv_get_current_profile_koins(result, 3);
        return result;
    case 0x2f4c:
        ppv_get_current_profile_koins(result, 2);
        return result;
    case 0x2f4d:
        ppv_get_current_profile_koins(result, 1);
        return result;
    case 0x2f4e:
        ppv_get_current_profile_koins(result, 0);
        return result;
    }
    result[0] = 0;
    return (char*)"UNFORMED";
}

void SetFloat__15mkGameVariablesFif(mkGameVariables* /*self*/, int /*id*/, float /*value*/) {}

float GetFloat__15mkGameVariablesFi(mkGameVariables* /*self*/, int /*id*/) {
    /* Soft ceiling ~97.5%: SDA reloc label (@4397 vs local); opcode match. */
    return kGvFloatZero;
}

extern int menu_mode_var;
extern int menu_mode_sub_var;
extern int arena_focus_var;
extern int arena_sub_menu_var;
extern int psel_p1_handicap;
extern int psel_p2_handicap;
extern int pprofile_stage_var;
extern int gameoption_exitwithsave;
extern int need_to_reload_systemart;
extern int pause_player;
extern int mode_of_play;
extern int profile_code_state[2];
extern int popup_type;
extern int winner;

/*
 * Game-facing integer options. IDs are the retail Screen resource IDs; the
 * apparently sparse switch is intentional and mirrors the retail dispatcher.
 */
void SetInt__15mkGameVariablesFii(mkGameVariables* /*self*/, int id, int value) {
    if (id >= 0x332c && id <= 0x3333) {
        set_game_option(id, value);
        return;
    }

    switch (id) {
    case 0x1770:
        bg_pselect_set_stage(0, value);
        break;
    case 0x1771:
        bg_pselect_set_stage(1, value);
        break;
    case 0x1f94:
        pselect_set_arena(value);
        break;
    case 0x238d:
        menu_mode_var = value;
        break;
    case 0x238e:
        menu_mode_sub_var = value;
        break;
    case 0x1fc6:
        arena_focus_var = value;
        break;
    case 0x1ffe:
        arena_sub_menu_var = value;
        break;
    case 0x1fca:
        psel_p1_handicap = value;
        pselect_handicap_update(0, value);
        break;
    case 0x1fcb:
        psel_p2_handicap = value;
        pselect_handicap_update(1, value);
        break;
    case 0x1fc7:
        cconfig_set_current_cell(0, value);
        break;
    case 0x1fc8:
        cconfig_set_current_cell(1, value);
        break;
    case 0x1fb0:
        add_to_wls_left_cursor(value);
        break;
    case 0x2b5c:
    case 0x2b5d:
    case 0x2b5e:
    case 0x2b5f:
    case 0x2b60:
        set_volume(id - 0x2b5c, value);
        break;
    case 0x1fdd:
        set_memcard_cursor_for(value);
        break;
    case 0x1fd4:
        pprofile_stage_var = value;
        break;
    case 0x2f44:
        ppc_set_button_answer(value != 0 ? 2 : 1);
        break;
    case 0x1fd2:
        ppc_set_current_icon_selection(value);
        break;
    case 0x2f46:
        ppv_update_profile_cursor(value);
        break;
    case 0x1fde:
        controller_setup_save_to_profile(0, value);
        break;
    case 0x1fdf:
        controller_setup_save_to_profile(1, value);
        break;
    case 0x2f4f:
        ppc_transition_pause(value);
        break;
    case 0x1ff9:
        controller_setup_p1_state(value);
        break;
    case 0x1ffa:
        controller_setup_p2_state(value);
        break;
    case 0x3715:
        adjust_brightness(value);
        break;
    case 0x1ff5:
        gameoption_exitwithsave = value != 0;
        break;
    case 0x1ff2:
        set_save_progress_flag(value);
        break;
    case 0x2f50:
        set_language(value);
        need_to_reload_systemart = 1;
        break;
    }
}

int GetInt__15mkGameVariablesFi(mkGameVariables* /*self*/, int id) {
    int state;

    if (id >= 0x1f7c && id <= 0x1f93) {
        return id - 0x1f7c;
    }
    if (id >= 0x1fb6 && id <= 0x1fbc) {
        return controller_get_texture_index_for_button(0, id - 0x1fb6);
    }
    if (id >= 0x1fbd && id <= 0x1fc3) {
        return controller_get_texture_index_for_button(1, id - 0x1fbd);
    }
    if (id >= 0x332c && id <= 0x3333) {
        return get_game_option(id);
    }

    switch (id) {
    case 0x1770:
    case 0x1f7a:
        return bg_pselect_get_stage(0);
    case 0x1771:
    case 0x1f7b:
        return bg_pselect_get_stage(1);
    case 0x1f72:
    case 0x1f73:
        state = (id == 0x1f72 ? g_game_info.plyr0.player_state
                              : g_game_info.plyr1.player_state);
        if (state == 1) {
            return 2;
        }
        return state > 0 && state < 3 ? 3 : 1;
    case 0x1f76:
        return pselect_get_selbox_pos(0);
    case 0x1f77:
        return pselect_get_selbox_pos(1);
    case 0x1f78:
        return bg_pselect_get_offender_class(0);
    case 0x1f79:
        return bg_pselect_get_offender_class(1);
    case 0x1f94:
        return pselect_get_arena_index();
    case 0x1fa4:
        return mode_of_play;
    case 0x1fa5:
        return pause_player;
    case 0x1fa6:
        return menu_player;
    case 0x1fa9:
        return profile_code_state[0];
    case 0x1fac:
        return current_render_state;
    case 0x1fae:
        return popup_type;
    case 0x1faf:
        return get_left_storage_device_status();
    case 0x1fc6:
        return arena_focus_var;
    case 0x1fc7:
        return controller_get_player_last_button(0);
    case 0x1fc8:
        return controller_get_player_last_button(1);
    case 0x1fc9:
        return get_right_storage_device_display_status();
    case 0x1fca:
        return psel_p1_handicap;
    case 0x1fcb:
        return psel_p2_handicap;
    case 0x1fd4:
        return pprofile_stage_var;
    case 0x1fd7:
        return pne_is_name_already_used();
    case 0x1fe0:
        return get_mu_access_progress();
    case 0x1fe1:
        return ok_to_bring_out_wager_screen();
    case 0x1fe7:
        return pselect_bgnd_has_deathtrap();
    case 0x1fe8:
        return pselect_bgnd_has_level_transition();
    case 0x1fe9:
        return pselect_bgnd_has_weapon();
    case 0x1ff0:
        return winner - 1;
    case 0x1ff2:
        return get_save_progress_flag();
    case 0x1ff3:
        return get_num_controllers();
    case 0x1ff4:
        return konquest_is_save_allowed();
    case 0x1ff6:
        return trial_never_passed_this_mission();
    case 0x1ffb:
        return pselect_get_body_texture_index(0);
    case 0x1ffc:
        return pselect_get_body_texture_index(1);
    case 0x1ffe:
        return arena_sub_menu_var;
    case 0x238c:
        return randu0(get_num_modeselect_portraits()) & 0xffff;
    case 0x238d:
        return menu_mode_var;
    case 0x238e:
        return menu_mode_sub_var;
    case 0x2b5c:
    case 0x2b5d:
    case 0x2b5e:
    case 0x2b5f:
    case 0x2b60:
        return get_volume(id - 0x2b5c);
    case 0x2f48:
        return ppc_get_code_state();
    case 0x2f50:
        return get_language();
    case 0x3714:
    case 0x3718:
        return get_contrast_value();
    case 0x3715:
    case 0x371a:
        return get_brightness_value();
    case 0x3716:
        return get_widescreen_state();
    case 0x3717:
        return get_progressive_scan_state();
    case 0x3719:
        return get_gamma_value();
    case 0x371b:
        return get_color_red_value();
    case 0x371c:
        return get_color_green_value();
    case 0x371d:
        return get_color_blue_value();
    }
    return 0;
}

int IsValidOption__15mkGameVariablesFi(mkGameVariables* /*self*/, int /*id*/) {
    return 1;
}

void Dispose__15mkGameVariablesFv(mkGameVariables* /*self*/) {}

void Init__15mkGameVariablesFv(mkGameVariables* /*self*/) {}

extern void* __vt__15mkGameVariables[];

mkGameVariables* __dt__15mkGameVariablesFv(mkGameVariables* self, short del) {
    if (self != 0) {
        self->m_vtbl = __vt__15mkGameVariables;
        if (del > 0) {
            __dl__FPv(self);
        }
    }
    return self;
}

/*
 * =====================================================================
 * mkScreenEngineClient -- LoadScreenSet / ReadStringData / CreatePoly
 * (next-modules T0-T1). Mangled C symbols so NonMatching Glue.o compares.
 * =====================================================================
 */

enum { kMallocTagInit = 0x494E4954 /* 'INIT' */ };

/* Retail @914 language folder names for Strings/<lang>/... lines. */
static const char* const s_stringLangFolders[6] = {
    stringBase0 + 0x0,  /* English-US */
    stringBase0 + 0xB,  /* Spanish */
    stringBase0 + 0x13, /* German */
    stringBase0 + 0x1A, /* French */
    stringBase0 + 0x21, /* Italian */
    stringBase0 + 0x0,  /* English-US */
};

extern void* __vt__10ScreenPoly;

/*
 * ReadStringData -- parse STRINGS binary lines:
 *   Strings/<lang>/<key>/<hexpairs...>
 * Language must match get_language_setting(); store decoded bytes in
 * set->resourceLib->strings (Hashtable @ +0x08).
 * Soft ceiling: ReadStringData ~84.4% -- @914 mtctr/bdnz vs addic./bne;
 *   line-copy / NV coloring leftovers. Empty-first branch flip regresses; stop.
 */
void ReadStringData__20mkScreenEngineClientFP9ScreenSetPvUi(void* this_unused,
                                                              void* set,
                                                              char* data,
                                                              unsigned int size) {
    char* end;
    char* cursor;
    char line[0x800];
    char* langWanted;
    char* slash1;
    char* slash2;
    char* slash3;
    char* hex;
    char* out;
    unsigned int nbytes;
    int i;
    unsigned char* decoded;
    void* prev;
    ScreenSetView* setView;
    ScreenResourceLibView* resourceLib;
    int lang;
    char* langs[6];
    int n;
    char* dst;
    const char* strBase;
    char c;

    (void)this_unused;

    /* Spill set/data/end first so r4/r5 can be reused for langs copy (retail). */
    setView = (ScreenSetView*)set;
    cursor = data;
    end = data + size;

    /*
     * Retail @914: mtctr 3x dword-pair (lwz/lwzu + stw/stwu) into stack langs[].
     * Soft ceiling: for-loop copy (same 6 pointers; not lwzu/stwu shape).
     */
    for (i = 0; i < 6; i++) {
        langs[i] = (char*)s_stringLangFolders[i];
    }

    lang = get_language_setting();
    langWanted = langs[lang];
    resourceLib = setView->resourceLib;
    strBase = stringBase0;

    while (cursor < end) {
        n = 0;
        dst = line;
        /* Retail: check then copy; prefer while-cond for blt-continue shape. */
        while (*cursor != '\n' && cursor < end) {
            c = *cursor;
            n += 1;
            cursor += 1;
            *dst = c;
            dst += 1;
            if (n >= 0x800) {
                n -= 1;
                break;
            }
        }
        line[n] = 0;
        cursor += 1; /* retail always advances past the delimiter */

        if (strncmp(line, strBase + 0x351, 8) != 0) {
            continue;
        }

        slash1 = strchr(line, '/');
        slash2 = strchr(slash1 + 1, '/');
        *slash2 = 0;
        slash3 = strrchr(slash2 + 1, '/');
        *slash3 = 0;

        if (stricmp(slash1 + 1, langWanted) != 0) {
            continue;
        }

        /* Retail: strlen(slash3+1); empty on divwu. beq; else walk hex+2. */
        nbytes = (unsigned int)strlen(slash3 + 1) / 6;
        if (nbytes != 0) {
            decoded = (unsigned char*)Malloc__10ScreenUtilFUliPc(
                nbytes, kMallocTagInit, (char*)(strBase + 0x35A));
            hex = slash3 + 1;
            out = (char*)decoded;
            for (i = 0; i < (int)nbytes; i++) {
                *out = (char)ReadHexInt__10ScreenUtilFPc(hex + 2);
                hex += 6;
                out += 1;
            }
        } else {
            decoded = (unsigned char*)Malloc__10ScreenUtilFUliPc(
                1, kMallocTagInit, (char*)(strBase + 0x366));
            decoded[0] = 0;
        }

        prev = hashtable_get(&resourceLib->strings, slash2 + 1);
        if (prev != 0) {
            _mwMemFree(prev, 0, 0);
        }
        hashtable_store(&resourceLib->strings, slash2 + 1, decoded);
    }
}

int LoadScreenSet__20mkScreenEngineClientFP9ScreenSet(ScreenEngineClient* client,
                                                     void* set) {
    char path[0x88];
    MkFileInfo* sec;
    int fileIndex;
    int size;
    void* block;
    char* name;
    ScreenSetView* setView;
    int ok;

    size = 0;
    setView = (ScreenSetView*)set;
    load_ssf(screen_engine_file_table);
    name = GetName__9ScreenSetFv(set);
    sprintf(path, stringBase0 + 0x1C9, name);
    sec = find_section_by_name(path);
    /* Retail: beq fail epilogue (no early return). */
    ok = 0;
    if (sec != 0) {
        fileIndex = add_art_section_async(client->slot, sec);
        wait_for_slot_load(client->slot);
        setView->unloadId = fileIndex;

        block = load_named_binary_block_from_file(
            client->slot, fileIndex, (char*)(stringBase0 + 0x373), &size);
        ReadStringData__20mkScreenEngineClientFP9ScreenSetPvUi(
            client, set, (char*)block, (unsigned int)size);

        block = load_named_binary_block_from_file(
            client->slot, fileIndex, (char*)(stringBase0 + 0x37B), &size);
        LoadSetData__15ScreenInstancerFP9ScreenSetPvUiPv(set, block,
                                                         (unsigned int)size, 0);
        ok = 1;
    }
    return ok;
}

static void ApplyPolyTextureFilter(RwTexture* tex, unsigned int useLinear) {
    RwTextureFilterView* view;

    if (tex == 0) {
        return;
    }
    view = (RwTextureFilterView*)tex;
    /* Retail: store mode bits, then reload and OR filter enable bit0. */
    if (useLinear != 0) {
        view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
    } else {
        view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
    }
    view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
}

/*
 * CreatePoly -- ScreenPoly 0x7C from SEPoly_t.
 * Offsets init 0; Y = 480 - pos.y; V = 1 - uv.v; filterFlags bit6 from se->flags.
 * After TGA/alpha load: inline filter write (two stores) + live ScreenObj bind.
 * Soft ceiling: CreatePoly ~79% -- -sdata 0 / string-pool / vert schedule; stop.
 */
void* CreatePoly__20mkScreenEngineClientFP8SEPoly_t(ScreenEngineClient* client,
                                                   SEPoly_t* se) {
    ScreenPoly* poly;
    char* base;
    RwTexture* color;
    RwTexture* alpha;
    RwTextureFilterView* view;
    ScreenObj* obj;
    RwRaster* raster;
    ScreenPolyVert* dst;
    int* map;
    int n;
    int vi;

    poly = (ScreenPoly*)__nw__10ScreenNodeFUl(0x7c);
    if (poly != 0) {
        __ct__10ScreenNodeFv(poly);
        poly->vtbl = &__vt__10ScreenPoly;
        /* Retail ctor store order: obj, instance, flags, filter word, Y, X, tex. */
        poly->screenObj = 0;
        poly->screenObjInstance = 0;
        poly->flags = 0;
        *(unsigned int*)&poly->filterFlags = 0;
        poly->offsetY = 0.0f;
        poly->offsetX = 0.0f;
        poly->colorTex = 0;
    }

    /* Retail rlwimi bit6 from (se->flags << 5). */
    ((ScreenPolyFilterBits*)&poly->filterFlags)->linear =
        ((se->flags << 5) & 0x40) != 0;
    poly->offsetX = 0.0f;
    poly->offsetY = 0.0f;

    if (se->textureString == 0 || se->textureString[0] == 0) {
        /* Retail @stringBase0+0x34D == "8X8". */
        se->textureString = (char*)(stringBase0 + 0x34D);
    }
    base = strrchr(se->textureString, '\\');
    if (base != 0) {
        base += 1;
    } else {
        base = se->textureString;
    }
    strupr(base);

    color = load_named_tga_from_slot(client->slot, base);
    poly->colorTex = color;
    if (color != 0) {
        view = (RwTextureFilterView*)color;
        if ((poly->filterFlags & 0x40) != 0) {
            view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
        } else {
            view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
        }
        view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
    }
    obj = poly->screenObj;
    if (obj != 0) {
        if ((unsigned int)obj->instance !=
            (unsigned int)poly->screenObjInstance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0) {
        if (color != 0) {
            raster = ((RwTextureFilterView*)color)->raster;
        } else {
            raster = 0;
        }
        obj->texture = raster;
        obj->pfx2d->texture = color;
    }

    alpha = load_named_alpha_texture_from_slot(client->slot, base);
    poly->alphaTex = alpha;
    if (alpha != 0) {
        view = (RwTextureFilterView*)alpha;
        if ((poly->filterFlags & 0x40) != 0) {
            view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
        } else {
            view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
        }
        view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
    }
    obj = poly->screenObj;
    if (obj != 0) {
        if ((unsigned int)obj->instance !=
            (unsigned int)poly->screenObjInstance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0) {
        obj->pfx2d->alpha_texture = alpha;
    }

    map = vert_map__10ScreenPoly;
    dst = poly->verts;
    n = 4;
    do {
        vi = *map;
        dst->x = se->positions[vi].x;
        dst->y = 480.0f - se->positions[vi].y;
        dst->u = se->uvs[vi][0];
        dst->v = 1.0f - se->uvs[vi][1];
        dst->rgba[0] = se->colors[vi][0];
        dst->rgba[1] = se->colors[vi][1];
        dst->rgba[2] = se->colors[vi][2];
        dst->rgba[3] = se->colors[vi][3];
        map += 1;
        dst += 1;
        n -= 1;
    } while (n != 0);

    return poly;
}

extern RwMatrix* RwFrameGetLTM(RwFrame* frame);

enum { kScreenPolyPfxOid = 0x900B };

static unsigned char ScreenPolyModulateChannel(unsigned char src, float translation,
                                               float scale) {
    float v;

    v = scale * (255.0f * translation + (float)(unsigned int)src);
    return (unsigned char)(int)v;
}

/*
 * ScreenPoly::Render -- mode-select chrome present leaf.
 * Get LTM from info->matrixStack frame, ensure ScreenObj+Pfx2d via
 * load_2d_pfxobj_with_texture(0x900B), switch to pfx2d batch, transform
 * verts + modulate RGBA by ScreenRenderInfo, pfx2d_render.
 *
 * Soft ceiling: Render ~78.6% -- stwux/psq prologue + FPR coloring; stop.
 * Present: hide bit7 + colorTex; create/bind ScreenObj; pfx2d batch; LTM verts.
 */
void Render__10ScreenPolyFP16ScreenRenderInfo(ScreenPoly* poly,
                                              ScreenRenderInfoC* info) {
    ScreenMatrixStackC* stack;
    float* ltm;
    ScreenObj* obj;
    ScreenObj* live;
    Pfx2dObj* pfx;
    RwTexture* color;
    RwTexture* alpha;
    RwTextureFilterView* view;
    float local[4];
    float world[4];
    int i;
    ScreenPolyVert* src;
    Pfx2dVert* dst;

    stack = (ScreenMatrixStackC*)info->matrixStack;
    ltm = (float*)RwFrameGetLTM((RwFrame*)stack->frame);

    /* filterFlags bit7 hide (signed >=0) + require colorTex -- retail wrap. */
    if ((signed char)poly->filterFlags >= 0 && poly->colorTex != 0) {
        obj = poly->screenObj;
        if (obj != 0) {
            if (obj->instance != poly->screenObjInstance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }

        if (obj == 0) {
            color = poly->colorTex;
            obj = load_2d_pfxobj_with_texture(kScreenPolyPfxOid, color, 0, 0x11);
            if (obj == 0) {
                return;
            }
            pull_screen_obj(obj);
            poly->screenObj = obj;
            poly->screenObjInstance = obj->instance;

            color = poly->colorTex;
            if (color != 0) {
                view = (RwTextureFilterView*)color;
                if ((poly->filterFlags & 0x40) != 0) {
                    view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
                } else {
                    view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
                }
                view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
            }
            live = poly->screenObj;
            if (live != 0) {
                if (live->instance != poly->screenObjInstance) {
                    live = 0;
                }
            } else {
                live = 0;
            }
            if (live != 0) {
                if (color != 0) {
                    live->texture = ((RwTextureFilterView*)color)->raster;
                } else {
                    live->texture = 0;
                }
                live->pfx2d->texture = color;
            }

            alpha = poly->alphaTex;
            if (alpha != 0) {
                view = (RwTextureFilterView*)alpha;
                if ((poly->filterFlags & 0x40) != 0) {
                    view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
                } else {
                    view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
                }
                view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
            }
            live = poly->screenObj;
            if (live != 0) {
                if (live->instance != poly->screenObjInstance) {
                    live = 0;
                }
            } else {
                live = 0;
            }
            if (live != 0) {
                live->pfx2d->alpha_texture = alpha;
            }
        }

        pfx = obj->pfx2d;

        if (current_render_state == 2) {
            pfxfont_end_render();
        } else if (current_render_state == 3) {
            pfx_end_batch();
        }
        if (current_render_state != 1) {
            pfx2d_begin_render();
        }
        current_render_state = 1;

        /* verts @ Pfx2dObj +0x00 -- copy then overwrite xy/rgba from LTM. */
        memcpy(pfx->verts, poly->verts, 0x50);

        i = 0;
        src = poly->verts;
        dst = pfx->verts;
        do {
            local[0] = src->x + poly->offsetX;
            local[2] = 0.0f;
            local[1] = (480.0f - src->y) + poly->offsetY;
            local[3] = 0.0f;
            gxMatV3MatAddV3((Vec*)world, (Vec*)local, (Mat33*)ltm, (Vec*)(ltm + 12));

            if (is_widescreen_mode() != 0) {
                dst->x = world[0] + 40.0f;
            } else {
                dst->x = world[0];
            }
            i += 1;
            dst->y = 480.0f - world[1];

            dst->r = (unsigned char)(int)(info->colorScale[0] *
                (255.0f * info->colorTranslation[0] +
                 (float)(unsigned int)src->rgba[0]));
            dst->g = (unsigned char)(int)(info->colorScale[1] *
                (255.0f * info->colorTranslation[1] +
                 (float)(unsigned int)src->rgba[1]));
            dst->b = (unsigned char)(int)(info->colorScale[2] *
                (255.0f * info->colorTranslation[2] +
                 (float)(unsigned int)src->rgba[2]));
            dst->a = (unsigned char)(int)(info->colorScale[3] *
                (255.0f * info->colorTranslation[3] +
                 (float)(unsigned int)src->rgba[3]));

            src += 1;
            dst += 1;
        } while (i < 4);

        pfx2d_render(pfx);
    }
}

unsigned int IsVisible__10ScreenPolyFv(ScreenPoly* poly) {
    /* Retail: extrwi bit7 + cntlzw/srwi (== 0 -> visible). */
    return (unsigned int)__cntlzw((poly->filterFlags >> 7) & 1) >> 5;
}

void SetVisible__10ScreenPolyFUi(ScreenPoly* poly, unsigned int visible) {
    /* Retail: cntlzw(visible) + rlwimi bit7 (hide when visible==0). */
    ((ScreenPolyFilterBits*)&poly->filterFlags)->hidden = (visible == 0);
}

/*
 * ScreenPoly::SetComponent -- anim keys write verts / UV / hide / RGBA.
 * Jump table types 8..0x14: 8..B pos; C/E/10/12 UV; D/F/11/13 RGBA; 14 hide.
 * Soft ceiling: SetComponent -- exact 0x1A0 retail size; objdiff cannot align
 * the jump-table relocation. Structured algorithm and clamp stores match.
 *
 * Mode-select title banner (scr_main_menu.sec): Polys `decepCutout` /
 * `kanjiCutout` (+ `bannerTop` / `bannerShadow`) use type 0x14 hide (and
 * UV/RGBA keys) so Latin "DECEPTIO..." and kanji cutouts cross-fade. Driven by
 * ScreenAnimEffect::Process -> GetValue -> this. Tick:
 * p_screen_engine_tick -> ScreenMgr::UpdateAnimations -> UpdateSceneAnimation.
 */
typedef struct ScreenAnimControlC {
    unsigned int type; /* +0x00 */
    int flag; /* +0x04 */
} ScreenAnimControlC;

void SetComponent__10ScreenPolyFP17ScreenAnimControlPfi(ScreenPoly* poly,
                                                       ScreenAnimControlC* ctrl,
                                                       float* values,
                                                       int unused) {
    unsigned int t;
    unsigned int idx;
    int map;
    ScreenPolyVert* vert;
    int i;
    unsigned char flags;
    unsigned int hide;

    (void)unused;
    t = ctrl->type;
    switch (t) {
    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
        /* Position -> vert_map[type-8]; Y flipped vs 480. */
        idx = t - 8;
        map = vert_map__10ScreenPoly[idx];
        vert = &poly->verts[map];
        vert->x = values[0];
        vert->y = 480.0f - values[1];
        break;
    case 0xC:
    case 0xE:
    case 0x10:
    case 0x12:
        /* UV -> vert_map[(type-0xC)>>1]; V flipped vs 1. */
        idx = (t - 0xC) >> 1;
        map = vert_map__10ScreenPoly[idx];
        vert = &poly->verts[map];
        vert->u = values[0];
        vert->v = 1.0f - values[1];
        break;
    case 0xD:
    case 0xF:
    case 0x11:
    case 0x13:
        /* RGBA: clamp [0,1] then *255 into vert_map[(type-0xD)>>1]. */
        for (i = 0; i < 4; i++) {
            if (values[i] > 1.0f) {
                values[i] = 1.0f;
            }
            if (values[i] < 0.0f) {
                values[i] = 0.0f;
            }
        }
        idx = (t - 0xD) >> 1;
        map = vert_map__10ScreenPoly[idx];
        vert = &poly->verts[map];
        vert->rgba[0] = (unsigned char)(int)(255.0f * values[0]);
        vert->rgba[1] = (unsigned char)(int)(255.0f * values[1]);
        vert->rgba[2] = (unsigned char)(int)(255.0f * values[2]);
        vert->rgba[3] = (unsigned char)(int)(255.0f * values[3]);
        break;
    case 0x14:
        /* Hide when values[0]==0 -- inline bit7 (not SetVisible). */
        hide = (values[0] == 0.0f);
        flags = poly->filterFlags;
        flags = (unsigned char)((flags & ~0x80) | (hide << 7));
        poly->filterFlags = flags;
        break;
    default:
        break;
    }
    poly->flags = 1;
}

void Close__10ScreenPolyFv(ScreenPoly* poly) {
    ScreenObj* obj;
    typedef int (*DestroyFn)(ScreenObj*);
    DestroyFn* vtbl;

    /* Retail: live diamond, reload screenObj, vcall+0x10 if instance!=0, clear. */
    obj = poly->screenObj;
    if (obj != 0) {
        if (obj->instance == (unsigned int)poly->screenObjInstance) {
            /* keep */
        } else {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0) {
        obj = poly->screenObj;
        if (obj->instance != 0) {
            vtbl = *(DestroyFn**)obj;
            vtbl[0x10 / 4](obj);
        }
        poly->screenObj = 0;
        poly->screenObjInstance = 0;
    }
}

RwTexture* GetScreenPolyTexture__FPv(ScreenPoly* poly) {
    return poly->colorTex;
}

void SetScreenPolyTexture__FPvP9RwTexture(ScreenPoly* poly, RwTexture* tex) {
    ScreenObj* obj;
    RwTextureFilterView* view;
    RwRaster* raster;

    /*
     * Soft ceiling: ~97.3% -- extrwi. vs rlwinm. on bit6; stop.
     * Q3 tries: (>>6)&1 ~87%; (flags & 0x40) ~95.9%; bitfield overlay best.
     */
    poly->colorTex = tex;
    if (tex != 0) {
        view = (RwTextureFilterView*)tex;
        if (((ScreenPolyFilterBits*)&poly->filterFlags)->linear) {
            view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
        } else {
            view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
        }
        view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
    }

    obj = poly->screenObj;
    if (obj != 0) {
        if (obj->instance == (unsigned int)poly->screenObjInstance) {
            /* keep */
        } else {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return;
    }
    if (tex != 0) {
        raster = ((RwTextureFilterView*)tex)->raster;
    } else {
        raster = 0;
    }
    obj->texture = raster;
    /* Retail: unconditional pfx2d->texture store (no null check). */
    obj->pfx2d->texture = tex;
}

/* ScreenText / SEText layouts: mwScreenEngine/ScreenText.h */

typedef struct ScreenView {
    unsigned char pad00[0x54];
    ScreenSetView* set; /* +0x54 */
    SEScreenDataView* data; /* +0x58 -- Screen::m_data; CHAR names via strings */
} ScreenView;

/* SE PTCL element (CreateElement 'PTCL'). */
typedef struct SEParticle {
    unsigned int typeTag; /* +0x00 'PTCL' */
    int pad04;
    void* liveObject; /* +0x08 */
    unsigned int unk0c;
    char* fxName; /* +0x10 */
    float posX; /* +0x14 */
    float posY; /* +0x18 */
    float posZ; /* +0x1C */
} SEParticle;

/* SE CHAR element -- index selects model name from screen holder table. */
typedef struct SEChar {
    unsigned int typeTag; /* +0x00 'CHAR' */
    int nameIndex; /* +0x04 */
} SEChar;

typedef struct ScreenParticle {
    void* vtbl; /* +0x00 */
    unsigned char pad04[0x0C];
    char* fxName; /* +0x10 */
    int fxHandle; /* +0x14 */
    void* pfx; /* +0x18 */
    int hide; /* +0x1C -- nonzero = hidden */
} ScreenParticle;

typedef struct ScreenModel {
    void* vtbl; /* +0x00 */
    unsigned char pad04[0x0C];
    MkObj* model; /* +0x10 */
    unsigned int modelInstance; /* +0x14 */
    int visible; /* +0x18 */
} ScreenModel;

/* MkObj::hide_flags bit 0x20, expressed as an MSB-first byte bitfield. */
typedef struct MkObjHideBits {
    unsigned char pad0 : 2;
    unsigned char hidden : 1;
    unsigned char pad1 : 5;
} MkObjHideBits;

extern void* __vt__14ScreenParticle;
extern void* __vt__11ScreenModel;
extern void load_effect_bank_with_context(char* name, void* ctx);
extern int fx_by_id(char* name, int flags);
extern void* find_pfx_by_handle(int handle);
extern void fx_set_param_v3(int handle, int param, float x, float y, float z);
extern void* load_named_model_from_slot(int slot, const char* name, int flags, int unk);
extern void* obj_create_sobjs(void* obj);
extern void render_mkobj(void* mkobj);
extern void render_transl_atomics(void);
extern int curr_pipeline_used;
extern void* __dt__10ScreenNodeFv(void* node, short del);
extern void __dl__10ScreenNodeFPv(void* node);

/*
 * ScreenModel -- CHAR elements (mode-select 3D "CLOUDS" model etc.).
 * CreateElement('CHAR') loads mkobj; Render flushes 2D/font/pfx batch then
 * render_mkobj + transl atomics. Soft ceiling OK (NonMatching).
 */
static MkObj* ScreenModelLive(ScreenModel* self) {
    MkObj* mkobj;

    mkobj = self->model;
    if (mkobj != 0) {
        if (mkobj->hdr.instance == self->modelInstance) {
            /* keep */
        } else {
            mkobj = 0;
        }
    } else {
        mkobj = 0;
    }
    return mkobj;
}

void Render__11ScreenModelFP16ScreenRenderInfo(ScreenModel* self,
                                               void* /*info*/) {
    MkObj* mkobj;
    unsigned char flags;

    mkobj = ScreenModelLive(self);

    if (current_render_state == 2) {
        pfxfont_end_render();
    } else if (current_render_state == 1) {
        pfx2d_end_render();
    } else if (current_render_state == 3) {
        pfx_end_batch();
    }
    current_render_state = 0;

    if (mkobj != 0) {
        flags = mkobj->flag_bytes.hide_flags;
        flags = (unsigned char)((flags & ~0x20) | 0);
        mkobj->flag_bytes.hide_flags = flags;
        render_mkobj(mkobj);
        render_transl_atomics();
        flags = mkobj->flag_bytes.hide_flags;
        flags = (unsigned char)((flags & ~0x20) | 0x20);
        mkobj->flag_bytes.hide_flags = flags;
    }
}

void Dispose__11ScreenModelFv(ScreenModel* self) {
    MkObj* mkobj;
    void** vtbl;

    mkobj = ScreenModelLive(self);
    if (mkobj != 0) {
        mkobj = self->model;
        if (mkobj->hdr.instance != 0) {
            vtbl = *(void***)mkobj;
            ((void (*)(MkObj*))vtbl[4])(mkobj);
        }
        self->model = 0;
        self->modelInstance = 0;
    }
}

unsigned int IsVisible__11ScreenModelFv(ScreenModel* self) {
    return (unsigned int)self->visible;
}

void SetVisible__11ScreenModelFUi(ScreenModel* self, unsigned int visible) {
    self->visible = (visible != 0);
}

ScreenModel* __dt__11ScreenModelFv(ScreenModel* self, short del) {
    if (self != 0) {
        self->vtbl = &__vt__11ScreenModel;
        Dispose__11ScreenModelFv(self);
        __dt__10ScreenNodeFv(self, 0);
        if (del > 0) {
            __dl__10ScreenNodeFPv(self);
        }
    }
    return self;
}

/*
 * ScreenParticle -- PTCL elements (screen_fx.mko FX). Render pushes LTM
 * position into the FX handle and batches render_pfx.
 */
void Render__14ScreenParticleFP16ScreenRenderInfo(ScreenParticle* self,
                                                  ScreenRenderInfoC* info) {
    ScreenMatrixStackC* stack;
    float* ltm;

    stack = (ScreenMatrixStackC*)info->matrixStack;
    ltm = (float*)RwFrameGetLTM((RwFrame*)stack->frame);
    fx_set_param_v3(self->fxHandle, 0x202, ltm[12], ltm[13], ltm[14]);

    if (self->pfx != 0 && self->hide == 0) {
        if (current_render_state == 2) {
            pfxfont_end_render();
        } else if (current_render_state == 1) {
            pfx2d_end_render();
        }
        if (current_render_state != 3) {
            pfx_start_batch();
        }
        current_render_state = 3;
        curr_pipeline_used = 0;
        render_pfx((MkPfx*)self->pfx);
    }
}

void Dispose__14ScreenParticleFv(ScreenParticle* /*self*/) {}

unsigned int IsVisible__14ScreenParticleFv(ScreenParticle* self) {
    return (unsigned int)__cntlzw(self->hide) >> 5;
}

void SetVisible__14ScreenParticleFUi(ScreenParticle* self, unsigned int visible) {
    self->hide = (visible == 0);
    hide_pfx((MkPfx*)self->pfx, self->hide);
}

ScreenParticle* __dt__14ScreenParticleFv(ScreenParticle* self, short del) {
    if (self != 0) {
        self->vtbl = &__vt__14ScreenParticle;
        __dt__10ScreenNodeFv(self, 0);
        if (del > 0) {
            __dl__10ScreenNodeFPv(self);
        }
    }
    return self;
}

extern void* __vt__10ScreenText;
extern void* __vt__25mkScreenEngineMatrixStack;

static StringObj* ScreenTextLiveObj(ScreenText* text) {
    StringObj* obj;

    /* Retail diamond: keep on instance==, else zero (beq/bne/b shape). */
    obj = text->stringObj;
    if (obj != 0) {
        if (obj->instance == (unsigned int)text->stringObjInstance) {
            /* keep */
        } else {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    return obj;
}

unsigned int IsVisible__10ScreenTextFv(ScreenText* text) {
    StringObj* obj;

    /* Inline latch (retail has no helper bl). Soft ~94% empty-keep if still short. */
    obj = text->stringObj;
    if (obj != 0) {
        if (obj->instance == (unsigned int)text->stringObjInstance) {
            /* keep */
        } else {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    /* Retail: lbz flags@+0x0C; extrwi bit24 (MSB) -- StringObjVisBits.hidden. */
    return ((StringObjVisBits*)&obj->flags)->hidden;
}

void SetVisible__10ScreenTextFUi(ScreenText* text, unsigned int visible) {
    StringObj* obj;

    obj = text->stringObj;
    if (obj != 0) {
        if (obj->instance == (unsigned int)text->stringObjInstance) {
            /* keep */
        } else {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return;
    }
    /*
     * Soft ~93.9% -- empty-keep diamond leftover.
     * Q3 try: manual cntlzw+mask ~83.9% -- keep StringObjVisBits setter.
     */
    ((StringObjVisBits*)&obj->flags)->hidden = (visible == 0);
}

/*
 * ProcessEngineEvent -- rebuild wrapped StringObj from SEText.
 * Retail accepts 0x407 and 0x409 only (0x408 early-outs in the cmp cascade).
 * Soft ceiling: ProcessEngineEvent ~98.5% -- live-obj branch / NV leftovers; stop.
 */
void ProcessEngineEvent__10ScreenTextFP9ScreenMgri(ScreenText* text, void* /*mgr*/,
                                                   int event) {
    SEText* se;
    StringObj* live;
    StringObj* created;
    unsigned char color[4];

    /* Match retail binary-search shape around 0x408. */
    if (event == 0x408) {
        return;
    }
    if (event < 0x408) {
        if (event < 0x407) {
            return;
        }
    } else if (event >= 0x40A) {
        return;
    }

    live = text->stringObj;
    if (live != 0) {
        if ((unsigned int)live->instance != (unsigned int)text->stringObjInstance) {
            live = 0;
        }
    } else {
        live = 0;
    }
    if (live != 0) {
        return;
    }

    se = text->seData;
    /* Retail Y: (int)(480.0f - posY); stack arg = valign, r10 = halign. */
    created = create_wrapped_string(kScreenTextStringOid, text->font, text->string,
                                    (int)se->posX, (int)(480.0f - se->posY), (int)se->wrapW,
                                    (int)se->yOff, se->halign, se->valign);
    if (created == 0) {
        return;
    }
    text->stringObj = created;
    text->stringObjInstance = (int)created->instance;
    se = text->seData;
    color[0] = se->color[0];
    color[1] = se->color[1];
    color[2] = se->color[2];
    color[3] = se->color[3];
    pfxfont_set_string_color(&created->pfx, (unsigned int*)color);
}

/*
 * GetStartArray -- word-wrap line start offsets for ScreenText string.
 * Pass 1 counts wrapped lines; pass 2 mallocs int[count] and fills starts.
 * Wrap width from live StringObj.wrap_w; glyph advances from font metrics.
 * Soft ceiling: emit / i2f / NV leftovers OK for NonMatching callable.
 */
int* GetStartArray__10ScreenTextFPcRi(ScreenText* text, char* str, int* outCount) {
    int lineCount;
    int remain;
    char* cursor;
    int* starts;
    int i;
    int lastSpace;
    int breakLen;
    float width;
    float wrapW;
    unsigned char ch;
    StringObj* live;
    FontMetrics* metrics;
    GlyphMetrics* glyph;
    int wrapInt;

    lineCount = 0;
    remain = (int)strlen(str);
    if (text->font == 0 || str == 0 || remain < 1) {
        *outCount = 1;
        return 0;
    }

    cursor = str;
    while (remain > 0) {
        width = 0.0f;
        breakLen = 0;
        lastSpace = 0;
        i = 0;
        while (i < remain) {
            ch = (unsigned char)cursor[i];
            if (ch == 0 || ch == '\n') {
                break;
            }
            if (ch == ' ') {
                metrics = text->font->metrics;
                width += metrics->space_width;
                lastSpace = i;
            } else if (ch == '<' &&
                       strncmp(cursor + i, stringBase0 + 0x315, 9) == 0) {
                i += 0x12;
                continue;
            } else if (ch >= 0x20) {
                metrics = text->font->metrics;
                glyph = &metrics->glyphs[ch - 0x20];
                width += glyph->advance + metrics->letter_spacing;
            }

            live = text->stringObj;
            if (live != 0) {
                if (live->instance != (unsigned int)text->stringObjInstance) {
                    live = 0;
                }
            } else {
                live = 0;
            }
            if (live != 0) {
                wrapInt = live->wrap_w;
            } else {
                wrapInt = 0;
            }
            wrapW = (float)wrapInt;
            if (wrapW > 0.0f) {
                live = text->stringObj;
                if (live != 0) {
                    if (live->instance != (unsigned int)text->stringObjInstance) {
                        live = 0;
                    }
                } else {
                    live = 0;
                }
                if (live != 0) {
                    wrapInt = live->wrap_w;
                } else {
                    wrapInt = 0;
                }
                wrapW = (float)wrapInt;
                if (width > wrapW && lastSpace > 0) {
                    i = lastSpace;
                    break;
                }
            }
            i += 1;
        }
        breakLen = i;
        lineCount += 1;
        cursor += breakLen + 1;
        remain -= breakLen + 1;
    }

    *outCount = lineCount;
    starts = (int*)Malloc__10ScreenUtilFUliPc(
        (unsigned long)(lineCount << 2), kMallocTagInit,
        (char*)(stringBase0 + 0x31f));
    {
        int* out;
        int startOff;

        out = starts;
        startOff = 0;
        remain = (int)strlen(str);
        cursor = str;
        while (remain > 0) {
            width = 0.0f;
            lastSpace = 0;
            i = 0;
            while (i < remain) {
                ch = (unsigned char)cursor[i];
                if (ch == 0 || ch == '\n') {
                    break;
                }
                if (ch == ' ') {
                    metrics = text->font->metrics;
                    width += metrics->space_width;
                    lastSpace = i;
                } else if (ch == '<' &&
                           strncmp(cursor + i, stringBase0 + 0x315, 9) == 0) {
                    i += 0x12;
                    continue;
                } else if (ch >= 0x20) {
                    metrics = text->font->metrics;
                    glyph = &metrics->glyphs[ch - 0x20];
                    width += glyph->advance + metrics->letter_spacing;
                }

                live = text->stringObj;
                if (live != 0) {
                    if (live->instance != (unsigned int)text->stringObjInstance) {
                        live = 0;
                    }
                } else {
                    live = 0;
                }
                if (live != 0) {
                    wrapInt = live->wrap_w;
                } else {
                    wrapInt = 0;
                }
                wrapW = (float)wrapInt;
                if (wrapW > 0.0f) {
                    live = text->stringObj;
                    if (live != 0) {
                        if (live->instance != (unsigned int)text->stringObjInstance) {
                            live = 0;
                        }
                    } else {
                        live = 0;
                    }
                    if (live != 0) {
                        wrapInt = live->wrap_w;
                    } else {
                        wrapInt = 0;
                    }
                    wrapW = (float)wrapInt;
                    if (width > wrapW && lastSpace > 0) {
                        i = lastSpace;
                        break;
                    }
                }
                i += 1;
            }
            *out = startOff;
            out += 1;
            breakLen = i;
            cursor += breakLen + 1;
            startOff += breakLen + 1;
            remain -= breakLen + 1;
        }
    }
    return starts;
}

/*
 * SetComponent -- anim keys: 0 pos, 2 RGBA, 3 halign, 4 valign, 0x14 hide.
 * Soft ceiling: SetComponent -- retail jump-table scheduling; structured
 * switch is 8 bytes short. Algorithm, control type order, and clamps match.
 */
void SetComponent__10ScreenTextFP17ScreenAnimControlPfi(ScreenText* text,
                                                       ScreenAnimControlC* ctrl,
                                                       float* values, int unused) {
    StringObj* obj;
    unsigned int t;
    int i;

    (void)unused;
    t = ctrl->type;
    obj = ScreenTextLiveObj(text);
    if (obj == 0) {
        return;
    }
    if (t > 0x14) {
        return;
    }
    switch (t) {
    case 0:
        obj->render_x = (int)values[0];
        obj->render_y = 480 - (int)values[1];
        break;
    case 2:
        for (i = 0; i < 4; i++) {
            if (values[i] > 1.0f) {
                values[i] = 1.0f;
            }
            if (values[i] < 0.0f) {
                values[i] = 0.0f;
            }
        }
        obj->pfx.instance0.rgba[3] = (unsigned char)(int)(255.0f * values[3]);
        obj->pfx.instance0.rgba[1] = (unsigned char)(int)(255.0f * values[1]);
        obj->pfx.instance0.rgba[2] = (unsigned char)(int)(255.0f * values[2]);
        obj->pfx.instance0.rgba[0] = (unsigned char)(int)(255.0f * values[0]);
        break;
    case 3:
        string_obj_set_halign(obj, (int)values[0] & 0xff);
        break;
    case 4:
        string_obj_set_valign(obj, text->font, (int)values[0] & 0xff);
        break;
    case 0x14:
        ((StringObjVisBits*)&obj->flags)->hidden = (values[0] == 0.0f);
        break;
    default:
        break;
    }
}

/* Soft ceiling: GetStringLen ~96% - live-object latch branch scheduling; stop. */
int GetStringLen__10ScreenTextFv(ScreenText* text) {
    StringObj* obj;
    const char* str;
    int len;

    obj = ScreenTextLiveObj(text);
    if (obj == 0) {
        len = 0;
    } else {
        str = obj->text;
        if (str != 0) {
            len = (int)strlen(str);
        } else {
            len = 0;
        }
    }
    return len;
}

/*
 * Apply upper/lower to a live StringObj text buffer, then refresh glyphs.
 * Used by _ChangeCase for single-char TEXT leaves (KeyPad keys).
 */
static void ChangeCaseStringObj(StringObj* obj, PfxFontSlot* font, int toUpper) {
    char* p;
    char ch;

    p = (char*)obj->text;
    if (p == 0) {
        update_string_obj_pfx(obj, font, obj->text);
        return;
    }
    if (toUpper != 0) {
        while (*p != 0) {
            ch = *p;
            if (ch >= 'a' && ch <= 'z') {
                *p = (char)(ch - 0x20);
            }
            p++;
        }
    } else {
        while (*p != 0) {
            ch = *p;
            if (ch >= 'A' && ch <= 'Z') {
                *p = (char)(ch + 0x20);
            }
            p++;
        }
    }
    update_string_obj_pfx(obj, font, obj->text);
}

/*
 * Inline TEXT leaf: live StringObj, strlen==1, then ChangeCaseStringObj.
 * Retail open-codes this at nesting depth 1-2 (no ChangeCase__10ScreenText bl).
 */
static void ChangeCaseTextInline(ScreenText* text, int toUpper) {
    StringObj* obj;
    const char* str;
    int len;

    obj = ScreenTextLiveObj(text);
    if (obj == 0) {
        return;
    }
    str = obj->text;
    if (str == 0) {
        len = 0;
    } else {
        len = (int)strlen(str);
    }
    if (len != 1) {
        return;
    }
    obj = ScreenTextLiveObj(text);
    if (obj == 0) {
        return;
    }
    ChangeCaseStringObj(obj, text->font, toUpper);
}

/* SEElements packed table + SEObject/TEXT heads (C view; ScreenObject.h is C++). */
typedef struct SEElementsC {
    int count;
} SEElementsC;

typedef struct SEObjectC {
    unsigned int typeTag; /* +0x00 */
    int pad04;
    void* liveObject; /* +0x08 -- ScreenText* for TEXT */
    unsigned int flags;
    void* events;
    void* transform;
    SEElementsC* children; /* +0x18 */
} SEObjectC;

enum {
    kSeTagOBJ = 0x4F424A20, /* 'OBJ ' */
    kSeTagGROP = 0x47524F50, /* 'GROP' */
    kSeTagTEXT = 0x54455854 /* 'TEXT' */
};

#define SeEntryAt(list, i) (*(SEObjectC**)((char*)(list) + 4 + (i) * 4))

void ChangeCase__10ScreenTextFi(ScreenText* text, int toUpper);

static int SeTagIsGroup(unsigned int tag) {
    return tag == (unsigned int)kSeTagOBJ || tag == (unsigned int)kSeTagGROP;
}

/*
 * _ChangeCase -- walk SEElements tree; single-char TEXT leaves flip case.
 * Retail unrolls three OBJ/GROP levels then recurses; TEXT at depth 3 uses
 * GetStringLen + ChangeCase__10ScreenTextFi.
 * Soft ceiling: ~68.4% -- nested helpers vs retail 3-level unroll; stop.
 */
static void _ChangeCase__FP12SEElements_tUi(SEElementsC* list, unsigned int toUpper) {
    int i;
    int j;
    int k;
    SEObjectC* entry;
    SEObjectC* mid;
    SEObjectC* inner;
    SEElementsC* midList;
    SEElementsC* innerList;
    ScreenText* text;

    if (list == 0) {
        return;
    }
    for (i = 0; i < list->count; i++) {
        entry = SeEntryAt(list, i);
        if (SeTagIsGroup(entry->typeTag)) {
            midList = entry->children;
            if (midList == 0) {
                continue;
            }
            for (j = 0; j < midList->count; j++) {
                mid = SeEntryAt(midList, j);
                if (SeTagIsGroup(mid->typeTag)) {
                    innerList = mid->children;
                    if (innerList == 0) {
                        continue;
                    }
                    for (k = 0; k < innerList->count; k++) {
                        inner = SeEntryAt(innerList, k);
                        if (SeTagIsGroup(inner->typeTag)) {
                            _ChangeCase__FP12SEElements_tUi(inner->children,
                                                            toUpper);
                        } else if (inner->typeTag == (unsigned int)kSeTagTEXT) {
                            text = (ScreenText*)inner->liveObject;
                            if (GetStringLen__10ScreenTextFv(text) == 1) {
                                ChangeCase__10ScreenTextFi(text, (int)toUpper);
                            }
                        }
                    }
                } else if (mid->typeTag == (unsigned int)kSeTagTEXT) {
                    ChangeCaseTextInline((ScreenText*)mid->liveObject,
                                         (int)toUpper);
                }
            }
        } else if (entry->typeTag == (unsigned int)kSeTagTEXT) {
            ChangeCaseTextInline((ScreenText*)entry->liveObject, (int)toUpper);
        }
    }
}

/*
 * KeyPad::ChangeCase -- flip single-char labels under m_ext->children, latch
 * active @ +0xE4.
 */
void ChangeCase__6KeyPadFUi(KeyPad* self, unsigned int toUpper) {
    SEObjectC* ext;

    ext = *(SEObjectC**)((char*)self + 0x1C);
    _ChangeCase__FP12SEElements_tUi(ext->children, toUpper);
    self->active = (int)toUpper;
}

/*
 * ChangeCase -- mode!=0 upper, mode==0 lower; then refresh pfx glyphs.
 * Soft ceiling: ChangeCase ~97.4% -- exact byte walk; remaining obj/cursor/ch
 * register carousel only.
 */
void ChangeCase__10ScreenTextFi(ScreenText* text, int toUpper) {
    StringObj* obj;
    StringObj* live;
    char* str;
    char ch;

    obj = text->stringObj;
    if (obj != 0) {
        if (obj->instance == (unsigned int)text->stringObjInstance) {
            live = obj;
        } else {
            live = 0;
        }
    } else {
        live = 0;
    }
    obj = live;
    if (obj == 0) {
        return;
    }
    str = (char*)obj->text;
    if (str != 0) {
        if (toUpper != 0) {
            char* walk;

            walk = str;
            while ((ch = *walk) != 0) {
                if (ch >= 'a' && ch <= 'z') {
                    *walk -= 0x20;
                }
                walk++;
            }
        } else {
            char* walk;

            walk = str;
            while ((ch = *walk) != 0) {
                if (ch >= 'A' && ch <= 'Z') {
                    *walk += 0x20;
                }
                walk++;
            }
        }
    }
    update_string_obj_pfx(obj, text->font, str);
}

void Close__10ScreenTextFv(ScreenText* text) {
    StringObj* obj;
    typedef int (*DestroyFn)(StringObj*);
    DestroyFn* vtbl;

    /* Retail: live diamond, reload stringObj, vcall+0x10 if instance!=0, clear. */
    obj = text->stringObj;
    if (obj != 0) {
        if (obj->instance == (unsigned int)text->stringObjInstance) {
            /* keep */
        } else {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    if (obj != 0) {
        obj = text->stringObj;
        if (obj->instance != 0) {
            vtbl = *(DestroyFn**)obj;
            vtbl[0x10 / 4](obj);
        }
        text->stringObj = 0;
        text->stringObjInstance = 0;
    }
}

/*
 * ScreenText::Render -- pfxfont present leaf for mode-select labels.
 * Soft ceiling: Render ~93.3% -- extrwi/reg color leftovers; stop.
 */
void Render__10ScreenTextFP16ScreenRenderInfo(ScreenText* text, ScreenRenderInfoC* info) {
    ScreenMatrixStackC* stack;
    float* ltm;
    StringObj* obj;
    StringObj* live;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    unsigned char color[4];
    float delta[3];

    stack = (ScreenMatrixStackC*)info->matrixStack;
    ltm = (float*)RwFrameGetLTM((RwFrame*)stack->frame);

    obj = text->stringObj;
    live = obj;
    if (obj != 0) {
        if ((unsigned int)obj->instance == (unsigned int)text->stringObjInstance) {
            live = obj;
        } else {
            live = 0;
        }
    } else {
        live = 0;
    }
    obj = live;
    if (obj == 0) {
        return;
    }
    /* flags bit7 hidden -- MSB bitfield -> retail extrwi. */
    if (((StringObjVisBits*)&obj->flags)->hidden != 0) {
        return;
    }

    if (current_render_state == 1) {
        pfx2d_end_render();
    } else if (current_render_state == 3) {
        pfx_end_batch();
    }
    if (current_render_state != 2) {
        pfxfont_begin_render();
    }
    current_render_state = 2;

    r = obj->pfx.instance0.rgba[0];
    g = obj->pfx.instance0.rgba[1];
    b = obj->pfx.instance0.rgba[2];
    a = obj->pfx.instance0.rgba[3];
    color[0] = ScreenPolyModulateChannel(r, info->colorTranslation[0], info->colorScale[0]);
    color[1] = ScreenPolyModulateChannel(g, info->colorTranslation[1], info->colorScale[1]);
    color[2] = ScreenPolyModulateChannel(b, info->colorTranslation[2], info->colorScale[2]);
    color[3] = ScreenPolyModulateChannel(a, info->colorTranslation[3], info->colorScale[3]);
    pfxfont_set_string_color(&obj->pfx, (unsigned int*)color);
    pfxfont_set_transform(&obj->pfx, ltm);

    delta[0] = (float)(obj->render_x - obj->x);
    delta[2] = 0.0f;
    delta[1] = (float)(obj->y - obj->render_y);
    MKMatrixTranslate(obj->pfx.transform, delta, 1);

    /* Retail converts x/y inside each widescreen branch (no early f30/f31). */
    if (is_widescreen_mode() != 0) {
        pfxfont_string_render(&obj->pfx, (float)obj->x + 40.0f, (float)obj->y);
    } else {
        pfxfont_string_render(&obj->pfx, (float)obj->x, (float)obj->y);
    }

    obj->pfx.instance0.rgba[0] = r;
    obj->pfx.instance0.rgba[1] = g;
    obj->pfx.instance0.rgba[2] = b;
    obj->pfx.instance0.rgba[3] = a;
}

/*
 * CreateElement -- POLY/TEXT/PTCL/CHAR dispatch.
 * TEXT + font cache inlined (retail has no CreateText / ResolveFont bls).
 * Soft ceiling: CreateElement ~84.7% -- TEXT font-cache and PTCL/CHAR
 * register/SDA scheduling remain; stop.
 */
void* CreateElement__20mkScreenEngineClientFP9ScreenMgrP6ScreenP12ScreenObjectPv(
    ScreenEngineClient* client, void* /*mgr*/, void* screen, void* /*parent*/, void* data) {
    unsigned int tag;
    ScreenText* text;
    ScreenView* screenView;
    SEText* se;
    SEParticle* sePtcl;
    SEChar* seChar;
    ScreenParticle* particle;
    ScreenModel* model;
    ScreenResourceLibView* lib;
    void** vtbl;
    char* (*getString)(void* self, char* name);
    PfxFontSlot* font;
    PfxFontSlot* cacheFont;
    ScreenFontCacheRow* row;
    ScreenFontCacheRow* rows;
    int i;
    int freeSlot;
    FontFace* face;
    void* metrics;
    int metSize;
    char metName[0x40];
    ScreenSetView* set;
    char* fontName;
    char* modelName;
    MkObj* mkobj;
    SEStringTableView* nameTable;
    int fxCtx[3];

    tag = *(unsigned int*)data;
    if (tag == 0x504F4C59) { /* 'POLY' */
        return CreatePoly__20mkScreenEngineClientFP8SEPoly_t(client, (SEPoly_t*)data);
    }
    if (tag == 0x54455854) { /* 'TEXT' */
        se = (SEText*)data;
        screenView = (ScreenView*)screen;
        text = (ScreenText*)__nw__10ScreenNodeFUl(0x24);
        if (text != 0) {
            __ct__10ScreenNodeFv(text);
            text->vtbl = &__vt__10ScreenText;
            /* Retail store order: seData, font, flags, stringObj, instance. */
            text->seData = se;
            text->font = 0;
            text->flags = 0;
            text->stringObj = 0;
            text->stringObjInstance = 0;

            fontName = se->string1;
            font = load_named_font(fontName);
            text->font = font;
            if (font == 0) {
                set = screenView->set;
                strupr(fontName);
                rows = client->fontCache;
                freeSlot = -1;
                cacheFont = 0;
                for (i = 0; i < 7; i++) {
                    row = &rows[i];
                    if (row->name == 0) {
                        if (freeSlot < 0) {
                            freeSlot = i;
                        }
                    } else if (strcmp(row->name, fontName) == 0) {
                        cacheFont = &row->font;
                        break;
                    }
                }
                if (cacheFont == 0) {
                    if (freeSlot >= 0 && freeSlot < 7) {
                        if (strlen(fontName) >= 0x3C) {
                            fontName[0x3C] = 0;
                        }
                        face = (FontFace*)load_named_tga_from_slot(client->slot,
                                                                  fontName);
                        sprintf(metName, stringBase0 + 0x346, fontName);
                        metrics =
                            load_named_binary_block(client->slot, metName, &metSize);
                        (void)metSize;
                        /* Retail: enable bit, then linear filter. */
                        face->flags_50 = (face->flags_50 & 0xffffff00) | 1;
                        face->flags_50 = (face->flags_50 & 0xffff00ff) | 0x3300;
                        row = &rows[freeSlot];
                        row->font.face = face;
                        row->font.metrics = (FontMetrics*)metrics;
                        row->name = fontName;
                        row->unloadId = set->unloadId;
                        cacheFont = &row->font;
                    }
                }
                text->font = cacheFont;
            }

            /* Retail: no null guards on set / resourceLib. */
            lib = screenView->set->resourceLib;
            vtbl = *(void***)lib;
            getString = (char* (*)(void*, char*))vtbl[4];
            text->string = getString(lib, se->string0);
        }
        return text;
    }
    if (tag == 0x5054434C) { /* 'PTCL' */
        sePtcl = (SEParticle*)data;
        fxCtx[0] = 0x90046;
        fxCtx[1] = 0;
        fxCtx[2] = 0;
        set_process_as_scriptable(aproc);
        active_cmdscript = get_cmdscript_for_proc(aproc);
        load_effect_bank_with_context((char*)(stringBase0 + 0x338), fxCtx);

        particle = (ScreenParticle*)__nw__10ScreenNodeFUl(0x20);
        if (particle != 0) {
            __ct__10ScreenNodeFv(particle);
            particle->vtbl = &__vt__14ScreenParticle;
            particle->pfx = 0;
            particle->fxHandle = 0;
            particle->fxName = 0;
            particle->hide = 0;
        }
        particle->fxName = sePtcl->fxName;
        particle->fxHandle = fx_by_id(sePtcl->fxName, 8);
        if (particle->fxHandle != 0) {
            particle->pfx = find_pfx_by_handle(particle->fxHandle);
        }
        fx_set_param_v3(particle->fxHandle, 0x202, sePtcl->posX, sePtcl->posY,
                        sePtcl->posZ);
        return particle;
    }
    if (tag == 0x43484152) { /* 'CHAR' */
        seChar = (SEChar*)data;
        screenView = (ScreenView*)screen;
        nameTable = screenView->data->strings;
        modelName = SEStringAt(nameTable, (unsigned int)seChar->nameIndex);

        model = (ScreenModel*)__nw__10ScreenNodeFUl(0x1c);
        if (model != 0) {
            __ct__10ScreenNodeFv(model);
            model->vtbl = &__vt__11ScreenModel;
            model->model = 0;
            model->modelInstance = 0;
            model->visible = 1;
        }
        mkobj = (MkObj*)load_named_model_from_slot(screen_engine_client.slot, modelName,
                                                   0x9011, 1);
        model->model = mkobj;
        model->modelInstance = mkobj->hdr.instance;
        mkobj->light_flags = 1;
        ((MkObjHideBits*)&mkobj->flag_bytes.hide_flags)->hidden = 1;
        obj_create_sobjs(mkobj);
        if (strcmp(modelName, stringBase0 + 0x331) == 0) {
            sobj_set_priority(obj_find_sobj_by_id(mkobj, 1), 2);
            sobj_set_priority(obj_find_sobj_by_id(mkobj, 2), 1);
            sobj_set_priority(obj_find_sobj_by_id(mkobj, 3), 0);
        }
        insert_fgnd_mkobj(mkobj);
        return model;
    }
    return 0;
}

/*
 * mkScreenEngineMatrixStack -- RwFrame wrapper (T2 LTM path).
 * Create allocates 8 bytes: vtbl + RwFrame*. Ops pass combine mode 2.
 * LTM consumers read frame at +0x04 via RwFrameGetLTM.
 */
typedef struct mkScreenEngineMatrixStack mkScreenEngineMatrixStack;

typedef struct mkScreenEngineMatrixStackVtbl {
    void* rtti; /* +0x00 */
    void* pad04; /* +0x04 */
    void (*dtor)(mkScreenEngineMatrixStack* self, short del); /* +0x08 */
    void (*init)(mkScreenEngineMatrixStack* self); /* +0x0C */
    void (*dispose)(mkScreenEngineMatrixStack* self); /* +0x10 */
    void (*setIdentity)(mkScreenEngineMatrixStack* self); /* +0x14 */
    void (*addChild)(mkScreenEngineMatrixStack* self, mkScreenEngineMatrixStack* child); /* +0x18 */
    void (*rotate)(mkScreenEngineMatrixStack* self, void* axis, float angle); /* +0x1C */
    void (*scale)(mkScreenEngineMatrixStack* self, void* scale); /* +0x20 */
    void (*translate)(mkScreenEngineMatrixStack* self, void* delta); /* +0x24 */
} mkScreenEngineMatrixStackVtbl;

struct mkScreenEngineMatrixStack {
    mkScreenEngineMatrixStackVtbl* vtbl; /* +0x00 */
    void* frame; /* +0x04 -- RwFrame* */
};

void Translate__25mkScreenEngineMatrixStackFP14Screen3DVector(mkScreenEngineMatrixStack* self,
                                                             void* delta) {
    RwFrameTranslate((RwFrame*)self->frame, (const RwV3d*)delta, 2);
}

void Scale__25mkScreenEngineMatrixStackFP14Screen3DVector(mkScreenEngineMatrixStack* self,
                                                         void* scale) {
    RwFrameScale((RwFrame*)self->frame, (const RwV3d*)scale, 2);
}

void Rotate__25mkScreenEngineMatrixStackFP14Screen3DVectorf(mkScreenEngineMatrixStack* self,
                                                           void* axis, float angle) {
    RwFrameRotate((RwFrame*)self->frame, (const RwV3d*)axis, angle, 2);
}

void AddChild__25mkScreenEngineMatrixStackFP17ScreenMatrixStack(mkScreenEngineMatrixStack* self,
                                                               mkScreenEngineMatrixStack* child) {
    if (child != 0) {
        RwFrameAddChild((RwFrame*)self->frame, (RwFrame*)child->frame);
    }
}

void SetIdentity__25mkScreenEngineMatrixStackFv(mkScreenEngineMatrixStack* self) {
    RwFrameSetIdentity((RwFrame*)self->frame);
}

void Dispose__25mkScreenEngineMatrixStackFv(mkScreenEngineMatrixStack* self) {
    RwFrameDestroy((RwFrame*)self->frame);
}

mkScreenEngineMatrixStack* __dt__25mkScreenEngineMatrixStackFv(mkScreenEngineMatrixStack* self,
                                                              short del) {
    if (self != 0) {
        self->vtbl = (mkScreenEngineMatrixStackVtbl*)&__vt__25mkScreenEngineMatrixStack;
        __dt__17ScreenMatrixStackFv(self, 0);
        if (del > 0) {
            __dl__FPv(self);
        }
    }
    return self;
}

void SetRootTransformation__20mkScreenEngineClientFP17ScreenMatrixStack(void* /*client*/,
                                                                        void* /*stack*/) {
    /* Retail no-op. */
}

void DestroyMatrixStack__20mkScreenEngineClientFP17ScreenMatrixStack(void* /*client*/,
                                                                    mkScreenEngineMatrixStack* stack) {
    /*
     * Soft ceiling: DestroyMatrixStack ~86.3% -- MWCC preloads vtbl from r4
     * (lwz r5,0(r4) / lwz r12,8(r5)) vs retail mr;li;lwz;lwz from r3; stop.
     */
    if (stack != 0) {
        stack->vtbl->dtor(stack, 1);
    }
}

void* CreateMatrixStack__20mkScreenEngineClientFv(void* /*client*/) {
    mkScreenEngineMatrixStack* stack;

    stack = (mkScreenEngineMatrixStack*)__nw__FUl(0x8);
    if (stack != 0) {
        __ct__17ScreenMatrixStackFv(stack);
        stack->vtbl = (mkScreenEngineMatrixStackVtbl*)&__vt__25mkScreenEngineMatrixStack;
        stack->frame = RwFrameCreate();
    }
    return stack;
}

int LoadScreen__20mkScreenEngineClientFP9ScreenSetP6ScreenUi(void* /*client*/,
                                                            void* /*set*/,
                                                            void* /*screen*/,
                                                            unsigned int /*flags*/) {
    return 1;
}

/* Retail empty present hooks (vtbl slots before CreateInstance). */
void PostRender__20mkScreenEngineClientFv(void* /*client*/) {}

void PreRender__20mkScreenEngineClientFv(void* /*client*/) {}

/*
 * CreateInstance -- SCtl subclass factory by fourcc typeId (classInfo->typeId).
 * Alloc via ScreenControl::operator new, run ScreenControl ctor, then install
 * subclass vtbl + zero subclass fields. mgr/params unused in retail.
 *
 * Case body order matches retail emission (TEXT..KENT). Soft: vtbl lis vs SDA.
 */
enum {
    kSeInstIMLI = 0x494D4C49, /* 'IMLI' */
    kSeInstKENT = 0x4B454E54, /* 'KENT' */
    kSeInstKPAD = 0x4B504144, /* 'KPAD' */
    kSeInstLIST = 0x4C495354, /* 'LIST' */
    kSeInstSPSH = 0x53505348, /* 'SPSH' */
    kSeInstSPSI = 0x53505349, /* 'SPSI' */
    kSeInstTEXT = 0x54455854, /* 'TEXT' */
    kSeInstWIFI = 0x57494649  /* 'WIFI' */
};

extern void* __nw__13ScreenControlFUl(unsigned long size);
extern void __ct__13ScreenControlFv(void* self);
extern void* __vt__8TextItem;
extern void* __vt__6KeyPad;
extern void* __vt__11SpreadSheet;
extern void* __vt__16SpreadSheet_text;
extern void* __vt__17SpreadSheet_image;
extern void* __vt__9ImageList;
extern void* __vt__8WifImage;
extern void* __vt__8TextList;
extern void* __vt__8KeyEntry;

void* CreateInstance__20mkScreenEngineClientFP9ScreenMgriP12ScreenParams(
    void* /*this*/, void* /*mgr*/, int typeId, void* /*params*/) {
    TextItem* textItem;
    KeyPad* keyPad;
    SpreadSheet_text* ssText;
    SpreadSheet_image* ssImage;
    ImageList* imageList;
    WifImage* wifImage;
    TextList* textList;
    KeyEntry* keyEntry;

    /*
     * Soft ceiling: CreateInstance ~98.1% -- dispatch r3/r4 coloring and
     * default-return placement remain; stop.
     */
    switch (typeId) {
    case kSeInstTEXT:
        textItem = (TextItem*)__nw__13ScreenControlFUl(0xc4);
        if (textItem != 0) {
            __ct__13ScreenControlFv(textItem);
            textItem->vtbl = &__vt__8TextItem;
            textItem->textNode = 0;
            textItem->optionId = 0;
            textItem->editBuf = 0;
            textItem->curChar = -1;
            textItem->cursorPos = -1;
            textItem->scrollPos = 0;
            textItem->scrollLimit = -1;
            textItem->indexTable = 0;
        }
        return textItem;
    case kSeInstKPAD:
        keyPad = (KeyPad*)__nw__13ScreenControlFUl(0xe8);
        if (keyPad != 0) {
            __ct__13ScreenControlFv(keyPad);
            keyPad->vtbl = &__vt__6KeyPad;
            keyPad->editLen = 0;
            keyPad->active = 1;
        }
        return keyPad;
    case kSeInstSPSH:
        ssText = (SpreadSheet_text*)__nw__13ScreenControlFUl(0x120);
        if (ssText != 0) {
            __ct__13ScreenControlFv(ssText);
            ssText->sheet.vtbl = &__vt__11SpreadSheet;
            ssText->sheet.rows = 0;
            ssText->sheet.cols = 0;
            ssText->sheet.unkE8 = 0;
            ssText->sheet.unkEC = 0;
            ssText->sheet.flagF0 = 0;
            ssText->sheet.cellArray = 0;
            ssText->sheet.flag104 = 1;
            ssText->sheet.flag108 = 1;
            ssText->sheet.scrollX = 0;
            ssText->sheet.unkD4 = 0;
            ssText->sheet.scrollY = 0;
            ssText->sheet.unkDC = 0;
            ssText->sheet.nodeB0 = 0;
            ssText->sheet.nodeB4 = 0;
            ssText->sheet.vtbl = &__vt__16SpreadSheet_text;
            ssText->unk11C = 0;
            ssText->unk118 = 0;
        }
        return ssText;
    case kSeInstSPSI:
        ssImage = (SpreadSheet_image*)__nw__13ScreenControlFUl(0x124);
        if (ssImage != 0) {
            __ct__13ScreenControlFv(ssImage);
            ssImage->sheet.vtbl = &__vt__11SpreadSheet;
            ssImage->sheet.rows = 0;
            ssImage->sheet.cols = 0;
            ssImage->sheet.unkE8 = 0;
            ssImage->sheet.unkEC = 0;
            ssImage->sheet.flagF0 = 0;
            ssImage->sheet.cellArray = 0;
            ssImage->sheet.flag104 = 1;
            ssImage->sheet.flag108 = 1;
            ssImage->sheet.scrollX = 0;
            ssImage->sheet.unkD4 = 0;
            ssImage->sheet.scrollY = 0;
            ssImage->sheet.unkDC = 0;
            ssImage->sheet.nodeB0 = 0;
            ssImage->sheet.nodeB4 = 0;
            ssImage->sheet.vtbl = &__vt__17SpreadSheet_image;
            ssImage->unk118 = 0;
            ssImage->unk11C = 0;
            ssImage->unk120 = 0;
        }
        return ssImage;
    case kSeInstIMLI:
        imageList = (ImageList*)__nw__13ScreenControlFUl(0x114);
        if (imageList != 0) {
            __ct__13ScreenControlFv(imageList);
            imageList->vtbl = &__vt__9ImageList;
            imageList->itemNodes = 0;
            imageList->linkNode = 0;
            imageList->scrollBase = 0;
            imageList->focusIndex = 0;
            imageList->itemCount = 0;
            imageList->wrap = 1;
            imageList->textureInfo.data = 0;
            imageList->textureInfo.ready = 0;
        }
        return imageList;
    case kSeInstWIFI:
        wifImage = (WifImage*)__nw__13ScreenControlFUl(0xfc);
        if (wifImage != 0) {
            __ct__13ScreenControlFv(wifImage);
            wifImage->vtbl = &__vt__8WifImage;
            wifImage->statusNode = 0;
            wifImage->started = 0;
            wifImage->curTexture = 0;
            wifImage->curTextureInstance = 0;
        }
        return wifImage;
    case kSeInstLIST:
        textList = (TextList*)__nw__13ScreenControlFUl(0xd8);
        if (textList != 0) {
            __ct__13ScreenControlFv(textList);
            textList->vtbl = &__vt__8TextList;
            textList->itemNodes = 0;
            textList->linkedNode = 0;
            textList->strings = 0;
            textList->focusIndex = 0;
            textList->focusMax = 0;
            textList->stringCount = 0;
            textList->itemCount = 0;
            textList->wrap = 1;
            textList->unkD4 = 0;
        }
        return textList;
    case kSeInstKENT:
        keyEntry = (KeyEntry*)__nw__13ScreenControlFUl(0xa4);
        if (keyEntry != 0) {
            __ct__13ScreenControlFv(keyEntry);
            keyEntry->ctrl.vtbl = &__vt__8KeyEntry;
        }
        return keyEntry;
    default:
        return 0;
    }
}

/*
 * SCtl ProcessParams -- typed field fills from ScreenParams.
 * NonMatching: C for objdiff; linked DOL still uses retail ASM.
 */

int GetResourceID__12ScreenParamsFUi(void* params, unsigned int index);
int GetInt__12ScreenParamsFUi(void* params, unsigned int index);
int GetBoolean__12ScreenParamsFUi(void* params, unsigned int index);
float GetFloat__12ScreenParamsFUi(void* params, unsigned int index);
ScreenNode* GetScreenNode__12ScreenParamsFUi(void* params, unsigned int index);
char* GetName__12ScreenParamsFUi(void* params, unsigned int index);
unsigned int GetColor__12ScreenParamsFUi(void* params, unsigned int index);

extern void* m_pGameVariables__13ScreenControl;
extern void FreeTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_t(
    void* self, unsigned int unused, int id, GMTextureInfo_t* info);
extern int GetTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_tRUi(
    void* self, unsigned int unused, int id, GMTextureInfo_t* out,
    unsigned int* count);
extern int GetTextureCollection__22GameVariableDispatcherFiP15GMTextureInfo_tRUi(
    void* self, int id, GMTextureInfo_t* out, unsigned int* count);
extern void FreeTextureCollection__22GameVariableDispatcherFiP15GMTextureInfo_t(
    void* self, int id, GMTextureInfo_t* info);

void ProcessParams__8KeyEntryFP12ScreenParams(KeyEntry* /*self*/, void* /*params*/) {}

void ProcessParams__6KeyPadFP12ScreenParams(KeyPad* self, void* params) {
    self->pageResIds[0] = GetResourceID__12ScreenParamsFUi(params, 0);
    self->pageResIds[1] = GetResourceID__12ScreenParamsFUi(params, 1);
    self->pageResIds[2] = GetResourceID__12ScreenParamsFUi(params, 2);
    self->pageResIds[3] = GetResourceID__12ScreenParamsFUi(params, 3);
    self->maxLen = GetInt__12ScreenParamsFUi(params, 4);
    self->pageCount = GetInt__12ScreenParamsFUi(params, 5);
    self->pageIndex = 0;
    self->editLen = 0;
}

void ProcessParams__8TextItemFP12ScreenParams(TextItem* self, void* params) {
    self->textNode = GetScreenNode__12ScreenParamsFUi(params, 1);
    self->optionId = GetResourceID__12ScreenParamsFUi(params, 0);
}

void ProcessParams__8WifImageFP12ScreenParams(WifImage* self, void* params) {
    int i;
    char* name;

    if (params == 0) {
        return;
    }
    self->imageCount = GetInt__12ScreenParamsFUi(params, 0);
    if (self->imageCount > 0x10) {
        return;
    }
    self->unkF8 = GetFloat__12ScreenParamsFUi(params, 1);
    self->statusNode = (ScreenPoly*)GetScreenNode__12ScreenParamsFUi(params, 2);
    for (i = 0; i < self->imageCount; i++) {
        name = GetName__12ScreenParamsFUi(params, (unsigned int)(i + 3));
        strupr(name);
        self->images[i] = load_named_tga_from_slot(screen_engine_client.slot, name);
    }
}

/*
 * =====================================================================
 * KeyEntry / KeyPad / TextItem / WifImage -- handlers + lifecycle
 * Soft ceilings measured after lift; NonMatching Glue keeps retail ASM linked.
 * =====================================================================
 */

extern void Dispose__13ScreenControlFv(void* self);
extern void Init__13ScreenControlFv(void* self);
extern char* GetString__22GameVariableDispatcherFUiUi(void* self, unsigned int unused,
                                                      unsigned int id);
extern void SetString__22GameVariableDispatcherFUiUiPc(void* self, unsigned int unused,
                                                       unsigned int id, char* str);
extern unsigned int IsValidOption__22GameVariableDispatcherFUiUi(void* self,
                                                                  unsigned int unused,
                                                                  unsigned int id);
extern void FireEvent__12ScreenObjectFP9ScreenMgriiUi(void* self, void* mgr, int event,
                                                      int a, unsigned int b);
extern void Free__10ScreenUtilFPv(void* p);
extern void ProcessSubActions__12ScreenObjectFPC12ScreenActioni(void* self,
                                                                const void* action,
                                                                int match);
extern int HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(
    void* self, void* mgr, const void* action);
extern void FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
    void* self, unsigned int unused, unsigned int id, char** strings,
    unsigned int count);
extern int GetStringMatrixCollection__22GameVariableDispatcherFUiUiPPPcRi(
    void* self, unsigned int unused, unsigned int id, char*** strings,
    int* rows);

void Dispose__9ImageListFv(ImageList* self) {
    if (m_pGameVariables__13ScreenControl != 0) {
        FreeTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_t(
            m_pGameVariables__13ScreenControl, (unsigned int)self->gvContext,
            self->collectionId, &self->textureInfo);
    }
    if (self->itemNodes != 0) {
        Free__10ScreenUtilFPv(self->itemNodes);
    }
    Dispose__13ScreenControlFv(self);
}

void Dispose__8TextListFv(TextList* self) {
    if (self->collectionId == -1 && self->optionId != -1) {
        if (self->strings != 0) {
            Free__10ScreenUtilFPv(self->strings[0]);
            self->strings[0] = 0;
            Free__10ScreenUtilFPv(self->strings);
            self->strings = 0;
        }
    } else if (m_pGameVariables__13ScreenControl != 0) {
        FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
            m_pGameVariables__13ScreenControl, (unsigned int)self->gvContext,
            (unsigned int)self->collectionId, self->strings,
            (unsigned int)self->stringCount);
    }
    self->strings = 0;
    if (self->itemNodes != 0) {
        Free__10ScreenUtilFPv(self->itemNodes);
        self->itemNodes = 0;
    }
    Dispose__13ScreenControlFv(self);
}

void Dispose__17SpreadSheet_imageFv(SpreadSheet_image* self) {
    typedef void (*ClearContents)(void*);

    if (self->unk118 != 0) {
        Free__10ScreenUtilFPv((void*)self->unk118);
        self->unk118 = 0;
    }
    if (self->unk120 != 0) {
        (*(ClearContents**)self)[0x58 / 4](self);
        FreeTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_t(
            m_pGameVariables__13ScreenControl,
            (unsigned int)self->sheet.gvContext, self->sheet.collectionId,
            (GMTextureInfo_t*)&self->unk11C);
        self->unk120 = 0;
        self->unk11C = 0;
    }
    Dispose__13ScreenControlFv(self);
}

void Dispose__16SpreadSheet_textFv(SpreadSheet_text* self) {
    typedef void (*ClearContents)(void*);
    void** extra;

    extra = (void**)((char*)self + 0xA8);
    if (self->unk118 != 0) {
        Free__10ScreenUtilFPv((void*)self->unk118);
        self->unk118 = 0;
    }
    if (*extra != 0) {
        Free__10ScreenUtilFPv(*extra);
        *extra = 0;
    }
    if (self->unk11C != 0) {
        (*(ClearContents**)self)[0x58 / 4](self);
        FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
            m_pGameVariables__13ScreenControl,
            (unsigned int)self->sheet.gvContext,
            (unsigned int)self->sheet.collectionId,
            (char**)self->unk11C,
            (unsigned int)(self->sheet.rows * self->sheet.cols));
        self->unk11C = 0;
    }
    Dispose__13ScreenControlFv(self);
}

void AllocateCollection__17SpreadSheet_imageFv(SpreadSheet_image* self) {
    unsigned int count;

    count = 0;
    FreeTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_t(
        m_pGameVariables__13ScreenControl, (unsigned int)self->sheet.pad9C,
        self->sheet.collectionId, (GMTextureInfo_t*)&self->unk11C);
    self->unk11C = 0;
    self->unk120 = 0;
    self->sheet.unkEC =
        GetTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_tRUi(
            m_pGameVariables__13ScreenControl,
            (unsigned int)self->sheet.pad9C, self->sheet.collectionId,
            (GMTextureInfo_t*)&self->unk11C, &count);
    self->sheet.unkE8 = (int)count;
    self->unk120 = 1;
}

void AllocateCollection__16SpreadSheet_textFv(SpreadSheet_text* self) {
    FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
        m_pGameVariables__13ScreenControl, (unsigned int)self->sheet.pad9C,
        (unsigned int)self->sheet.collectionId, (char**)self->unk11C,
        (unsigned int)(self->sheet.unkE8 * self->sheet.unkEC));
    self->unk11C = 0;
    self->sheet.unkEC =
        GetStringMatrixCollection__22GameVariableDispatcherFUiUiPPPcRi(
            m_pGameVariables__13ScreenControl,
            (unsigned int)self->sheet.pad9C,
            (unsigned int)self->sheet.collectionId,
            (char***)&self->unk11C, &self->sheet.unkE8);
}

void ClearContents__17SpreadSheet_imageFv(SpreadSheet_image* self) {
    ScreenPoly* poly;
    ScreenPoly** cells;
    void* live;
    int count;
    int i;

    count = self->sheet.rows * self->sheet.cols;
    cells = (ScreenPoly**)self->unk118;
    if (cells == 0) {
        return;
    }
    i = 0;
    while (i < count) {
        poly = cells[i];
        if (poly != 0) {
            poly->colorTex = 0;
            live = poly->screenObj;
            if (live == 0) {
                live = 0;
            } else if (*(unsigned int*)((char*)live + 4) !=
                       (unsigned int)poly->screenObjInstance) {
                live = 0;
            }
            if (live != 0) {
                *(int*)((char*)live + 0x10) = 0;
                *(int*)(*(char**)((char*)live + 0x34) + 0xC8) = 0;
            }
        }
        i++;
    }
}

void ClearContents__16SpreadSheet_textFv(SpreadSheet_text* self) {
    int i;
    int off;
    int count;
    ScreenText* text;
    StringObj* live;
    unsigned char color[4];

    count = self->sheet.rows * self->sheet.cols;
    if (self->unk118 == 0) {
        return;
    }
    i = 0;
    off = 0;
    while (i < count) {
        text = *(ScreenText**)((char*)self->unk118 + off);
        if (text != 0) {
            live = text->stringObj;
            if (live != 0) {
                if (live->instance != (unsigned int)text->stringObjInstance) {
                    live = 0;
                }
            } else {
                live = 0;
            }
            if (live != 0) {
                color[0] = live->pfx.instance0.rgba[0];
                color[1] = live->pfx.instance0.rgba[1];
                color[2] = live->pfx.instance0.rgba[2];
                color[3] = live->pfx.instance0.rgba[3];
                update_string_obj_pfx(live, text->font, stringBase0 + 0x1C8);
                pfxfont_set_string_color(&live->pfx, (unsigned int*)color);
            }
        }
        i++;
        off += 4;
    }
}

void FinishSetup__17SpreadSheet_imageFP12ScreenParamsi(
    SpreadSheet_image* self, void* params, int nodeIndex) {
    int i;
    int count;
    int paramCount;
    ScreenPoly* first;
    ScreenPoly* linked;
    ScreenPolyVert* firstVert;
    ScreenPolyVert* linkedVert;

    paramCount = GetCount__12ScreenParamsCFv(params);
    count = self->sheet.rows * self->sheet.cols;
    self->unk118 = (int)Malloc__10ScreenUtilFUliPc(
        (unsigned long)((paramCount - nodeIndex) << 2), kMallocTagInit,
        (char*)(stringBase0 + 0x285));
    for (i = 0; i < count; i++, nodeIndex++) {
        ((void**)self->unk118)[i] =
            GetScreenNode__12ScreenParamsFUi(params, (unsigned int)nodeIndex);
    }
    if (self->sheet.bindNodeB0 != 0) {
        first = *(ScreenPoly**)self->unk118;
        linked = (ScreenPoly*)self->sheet.nodeB0;
        firstVert = &first->verts[vert_map__10ScreenPoly[0]];
        linkedVert = &linked->verts[vert_map__10ScreenPoly[0]];
        self->sheet.layout0[0] = firstVert->x - linkedVert->x;
        self->sheet.layout0[1] =
            (480.0f - firstVert->y) - (480.0f - linkedVert->y);
        self->sheet.layout0[2] = kGvFloatZero;
    }
    if (self->sheet.bindNodeB4 != 0) {
        first = *(ScreenPoly**)self->unk118;
        firstVert = &first->verts[vert_map__10ScreenPoly[0]];
        self->sheet.layout1[0] = firstVert->x - kGvFloatZero;
        self->sheet.layout1[1] = (480.0f - firstVert->y) - kGvFloatZero;
        self->sheet.layout1[2] = kGvFloatZero;
    }
}

void FinishSetup__16SpreadSheet_textFP12ScreenParamsi(
    SpreadSheet_text* self, void* params, int nodeIndex) {
    int i;
    int count;
    int paramCount;
    ScreenText* text;
    StringObj* live;
    float x;
    float y;

    paramCount = GetCount__12ScreenParamsCFv(params);
    count = self->sheet.rows * self->sheet.cols;
    self->unk118 = (int)Malloc__10ScreenUtilFUliPc(
        (unsigned long)((paramCount - nodeIndex) << 2), kMallocTagInit,
        (char*)(stringBase0 + 0x285));
    self->sheet.cellColors = (unsigned char*)Malloc__10ScreenUtilFUliPc(
        (unsigned long)(count << 2), kMallocTagInit,
        (char*)(stringBase0 + 0x285));
    for (i = 0; i < count; i++, nodeIndex++) {
        text = (ScreenText*)GetScreenNode__12ScreenParamsFUi(
            params, (unsigned int)nodeIndex);
        ((ScreenText**)self->unk118)[i] = text;
        live = text->stringObj;
        if (live != 0) {
            if (live->instance != (unsigned int)text->stringObjInstance) {
                live = 0;
            }
        } else {
            live = 0;
        }
        if (live == 0) {
            self->sheet.cellColors[i * 4 + 0] = text->seData->color[0];
            self->sheet.cellColors[i * 4 + 1] = text->seData->color[1];
            self->sheet.cellColors[i * 4 + 2] = text->seData->color[2];
            self->sheet.cellColors[i * 4 + 3] = text->seData->color[3];
        } else {
            self->sheet.cellColors[i * 4 + 0] = live->pfx.instance0.rgba[0];
            self->sheet.cellColors[i * 4 + 1] = live->pfx.instance0.rgba[1];
            self->sheet.cellColors[i * 4 + 2] = live->pfx.instance0.rgba[2];
            self->sheet.cellColors[i * 4 + 3] = live->pfx.instance0.rgba[3];
        }
    }
    if (self->sheet.bindNodeB0 != 0) {
        text = *(ScreenText**)self->unk118;
        live = text->stringObj;
        if (live != 0 &&
            live->instance != (unsigned int)text->stringObjInstance) {
            live = 0;
        }
        if (live == 0) {
            x = text->seData->posX;
            y = text->seData->posY;
        } else {
            x = (float)live->x;
            y = (float)(480 - live->y);
        }
        self->sheet.layout0[0] = x - kGvFloatZero;
        self->sheet.layout0[1] = y - kGvFloatZero;
        self->sheet.layout0[2] = kGvFloatZero - kGvFloatZero;
    }
    if (self->sheet.bindNodeB4 != 0) {
        text = *(ScreenText**)self->unk118;
        live = text->stringObj;
        if (live != 0 &&
            live->instance != (unsigned int)text->stringObjInstance) {
            live = 0;
        }
        if (live == 0) {
            x = text->seData->posX;
            y = text->seData->posY;
        } else {
            x = (float)live->x;
            y = (float)(480 - live->y);
        }
        self->sheet.layout1[0] = x - kGvFloatZero;
        self->sheet.layout1[1] = y - kGvFloatZero;
        self->sheet.layout1[2] = kGvFloatZero - kGvFloatZero;
    }
}

void Update__17SpreadSheet_imageFv(SpreadSheet_image* self) {
    int x;
    int y;
    int cell;
    int source;
    int sourceCount;
    int visibleRows;
    int visibleCols;
    ScreenPoly* poly;
    ScreenPoly* marker;
    ScreenPolyVert* vert;
    ScreenObj* live;
    unsigned int* texture;
    void** textures;
    void (**vtbl)(void);

    if (self->sheet.rows <= 0 || self->sheet.cols <= 0 || self->unk11C == 0) {
        return;
    }
    sourceCount = *(int*)((char*)self->unk11C + 0x14);
    if (self->sheet.unkEC < 1 || self->sheet.unkE8 < 1) {
        for (cell = 0; cell < sourceCount; cell++) {
            poly = ((ScreenPoly**)self->unk118)[cell];
            ((void (*)(void*, int))((void**)poly->vtbl)[7])(poly, 0);
        }
    }
    if (self->unk120 == 0) {
        return;
    }
    visibleRows = self->sheet.unkE8;
    if (self->sheet.rows < visibleRows) {
        visibleRows = self->sheet.rows;
    }
    visibleCols = self->sheet.unkEC;
    if (self->sheet.cols < visibleCols) {
        visibleCols = self->sheet.cols;
    }
    textures = *(void***)((char*)self->unk11C + 8);
    for (y = 0; y < self->sheet.cols; y++) {
        for (x = 0; x < self->sheet.rows; x++) {
            cell = x + y * self->sheet.rows;
            source = x + self->sheet.scrollX +
                     self->sheet.unkE8 * (y + self->sheet.unkD4);
            poly = ((ScreenPoly**)self->unk118)[cell];
            if (y < visibleCols && x < visibleRows && source < sourceCount) {
                vtbl = (void (**)(void))poly->vtbl;
                ((void (*)(void*))vtbl[4])(poly);
                texture = (unsigned int*)textures[source];
                poly->colorTex = (RwTexture*)texture;
                if (texture != 0) {
                    if (((poly->filterFlags >> 6) & 1) == 0) {
                        texture[0x14] = (texture[0x14] & 0xFFFF00FF) | 0x3300;
                    } else {
                        texture[0x14] = (texture[0x14] & 0xFFFF00FF) | 0x1100;
                    }
                    texture[0x14] = (texture[0x14] & 0xFFFFFF00) | 1;
                }
                live = poly->screenObj;
                if (live != 0 &&
                    *(unsigned int*)((char*)live + 4) !=
                        (unsigned int)poly->screenObjInstance) {
                    live = 0;
                }
                if (live != 0) {
                    *(unsigned int*)((char*)live + 0x10) =
                        texture == 0 ? 0 : texture[0];
                    *(unsigned int**)(*(char**)((char*)live + 0x34) + 0xC8) =
                        texture;
                }
                ((void (*)(void*))vtbl[3])(poly);
                ((void (*)(void*, int))vtbl[7])(poly, 1);
            } else {
                ((void (*)(void*, int))((void**)poly->vtbl)[7])(poly, 0);
            }
            if (self->sheet.bindNodeB0 != 0 && y == 0 &&
                self->sheet.scrollY == x + self->sheet.scrollX) {
                vert = &poly->verts[vert_map__10ScreenPoly[0]];
                marker = (ScreenPoly*)self->sheet.nodeB0;
                ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 1);
                marker->offsetX = vert->x - self->sheet.layout0[0];
                marker->offsetY =
                    (480.0f - vert->y) - self->sheet.layout0[1];
            }
        }
        if (self->sheet.bindNodeB4 != 0 &&
            self->sheet.unkDC == y + self->sheet.unkD4) {
            cell = self->sheet.scrollY + y * self->sheet.rows -
                   self->sheet.scrollX;
            poly = ((ScreenPoly**)self->unk118)[cell];
            vert = &poly->verts[vert_map__10ScreenPoly[0]];
            marker = (ScreenPoly*)self->sheet.nodeB4;
            ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 1);
            marker->offsetX = vert->x - self->sheet.layout1[0];
            marker->offsetY = (480.0f - vert->y) - self->sheet.layout1[1];
        }
    }
    if (self->sheet.unkE8 == 0 || self->sheet.unkEC == 0) {
        if (self->sheet.nodeB0 != 0) {
            marker = (ScreenPoly*)self->sheet.nodeB0;
            ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 0);
        }
        if (self->sheet.nodeB4 != 0) {
            marker = (ScreenPoly*)self->sheet.nodeB4;
            ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 0);
        }
    }
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, screen_manager, 0x53500000, 0, 0);
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, screen_manager, 0x53500001, 0, 0);
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, screen_manager, 0x53500002, 0, 0);
}

void Update__16SpreadSheet_textFv(SpreadSheet_text* self) {
    int x;
    int y;
    int cell;
    int source;
    int sourceCount;
    int visibleRows;
    int visibleCols;
    ScreenText* text;
    StringObj* live;
    ScreenPoly* marker;
    float pos;
    unsigned char color[4];
    void (**vtbl)(void);

    if (self->sheet.rows <= 0 || self->sheet.cols <= 0) {
        return;
    }
    sourceCount = self->sheet.unkEC * self->sheet.unkE8;
    if (self->sheet.unkEC < 1 || self->sheet.unkE8 < 1) {
        for (cell = 0; cell < sourceCount; cell++) {
            text = ((ScreenText**)self->unk118)[cell];
            ((void (*)(void*, int))((void**)text->vtbl)[7])(text, 0);
        }
    }
    if (self->unk11C == 0) {
        return;
    }
    visibleRows = self->sheet.unkE8;
    if (self->sheet.rows < visibleRows) {
        visibleRows = self->sheet.rows;
    }
    visibleCols = self->sheet.unkEC;
    if (self->sheet.cols < visibleCols) {
        visibleCols = self->sheet.cols;
    }
    for (y = 0; y < self->sheet.cols; y++) {
        for (x = 0; x < self->sheet.rows; x++) {
            cell = x + y * self->sheet.rows;
            source = x + self->sheet.scrollX +
                     self->sheet.unkE8 * (y + self->sheet.unkD4);
            text = ((ScreenText**)self->unk118)[cell];
            if (y < visibleCols && x < visibleRows && source < sourceCount) {
                vtbl = (void (**)(void))text->vtbl;
                ((void (*)(void*))vtbl[4])(text);
                live = text->stringObj;
                if (live != 0 &&
                    live->instance != (unsigned int)text->stringObjInstance) {
                    live = 0;
                }
                if (live != 0) {
                    color[0] = live->pfx.instance0.rgba[0];
                    color[1] = live->pfx.instance0.rgba[1];
                    color[2] = live->pfx.instance0.rgba[2];
                    color[3] = live->pfx.instance0.rgba[3];
                    update_string_obj_pfx(
                        live, text->font, ((char**)self->unk11C)[source]);
                    pfxfont_set_string_color(&live->pfx,
                                             (unsigned int*)color);
                }
                ((void (*)(void*))vtbl[3])(text);
                ((void (*)(void*, int))vtbl[7])(text, 1);
                if (self->sheet.useColor != 0) {
                    if ((self->sheet.bindNodeB0 == 0 ||
                         self->sheet.scrollY != x + self->sheet.scrollX) &&
                        (self->sheet.bindNodeB4 == 0 ||
                         self->sheet.unkDC != y + self->sheet.unkD4)) {
                        if (self->sheet.bindNodeB0 != 0 ||
                            self->sheet.bindNodeB4 != 0) {
                            live = text->stringObj;
                            if (live != 0 &&
                                live->instance !=
                                    (unsigned int)text->stringObjInstance) {
                                live = 0;
                            }
                            if (live != 0) {
                                color[0] = self->sheet.cellColors[cell * 4 + 0];
                                color[1] = self->sheet.cellColors[cell * 4 + 1];
                                color[2] = self->sheet.cellColors[cell * 4 + 2];
                                color[3] = self->sheet.cellColors[cell * 4 + 3];
                                pfxfont_set_string_color(&live->pfx,
                                                         (unsigned int*)color);
                                text->flags |= 1;
                            }
                        }
                    } else {
                        live = text->stringObj;
                        if (live != 0 &&
                            live->instance !=
                                (unsigned int)text->stringObjInstance) {
                            live = 0;
                        }
                        if (live != 0) {
                            color[0] = self->sheet.color[0];
                            color[1] = self->sheet.color[1];
                            color[2] = self->sheet.color[2];
                            color[3] = self->sheet.color[3];
                            pfxfont_set_string_color(&live->pfx,
                                                     (unsigned int*)color);
                            text->flags |= 1;
                        }
                    }
                }
            } else {
                ((void (*)(void*, int))((void**)text->vtbl)[7])(text, 0);
            }
            if (self->sheet.bindNodeB0 != 0 &&
                self->sheet.scrollY == x + self->sheet.scrollX) {
                live = text->stringObj;
                if (live != 0 &&
                    live->instance != (unsigned int)text->stringObjInstance) {
                    live = 0;
                }
                pos = live == 0 ? text->seData->posX : (float)live->x;
                marker = (ScreenPoly*)self->sheet.nodeB0;
                ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 1);
                marker->offsetX = pos - self->sheet.layout0[0];
                marker->offsetY = kGvFloatZero;
            }
        }
        if (self->sheet.bindNodeB4 != 0 &&
            self->sheet.unkDC == y + self->sheet.unkD4) {
            cell = self->sheet.scrollY + y * self->sheet.rows -
                   self->sheet.scrollX;
            text = ((ScreenText**)self->unk118)[cell];
            live = text->stringObj;
            if (live != 0 &&
                live->instance != (unsigned int)text->stringObjInstance) {
                live = 0;
            }
            pos = live == 0 ? text->seData->posY : (float)(480 - live->y);
            marker = (ScreenPoly*)self->sheet.nodeB4;
            ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 1);
            marker->offsetX = kGvFloatZero;
            marker->offsetY = pos - self->sheet.layout1[1];
        }
    }
    if (self->sheet.unkE8 == 0 || self->sheet.unkEC == 0) {
        if (self->sheet.nodeB0 != 0) {
            marker = (ScreenPoly*)self->sheet.nodeB0;
            ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 0);
        }
        if (self->sheet.nodeB4 != 0) {
            marker = (ScreenPoly*)self->sheet.nodeB4;
            ((void (*)(void*, int))((void**)marker->vtbl)[7])(marker, 0);
        }
    }
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, screen_manager, 0x53500000, 0, 0);
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, screen_manager, 0x53500001, 0, 0);
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, screen_manager, 0x53500002, 0, 0);
}

typedef struct ScreenGameFlags {
    unsigned char high : 1;
    unsigned char second : 1;
    unsigned char pad : 6;
} ScreenGameFlags;

extern int HandleAction__22GameVariableDispatcherFP9ScreenMgrPC12ScreenAction(
    void* self, void* mgr, const void* action);
extern int GetInt__22GameVariableDispatcherFi(void* self, int id);
extern void SetInt__22GameVariableDispatcherFUiUii(void* self, unsigned int context,
                                                   unsigned int id, int value);
extern void SetString__22GameVariableDispatcherFiPc(void* self, int id, char* value);
extern int ui_sound_table[];
extern int p1_profile_status;
extern int p2_profile_status;
extern unsigned char p1_profile[0x5c0];
extern unsigned char p2_profile[0x5c0];
extern void* GetAnimScene__6ScreenFi(void* screen, int index);
extern void PlayUntilTime__15ScreenAnimSceneFi(void* scene, int time);
extern void* GetScreenObject__12ScreenParamsFUi(void* params, unsigned int index);
extern int GetCount__12ScreenParamsCFv(void* params);
extern unsigned int ScreenIntegerCompare__Fiii(int lhs, int op, int rhs);
extern void vdebug_print_message(const char* fmt, ...);
extern void snd_req(int sound_id);
extern void snd_stop_all(void);
extern void fx_resume_emit(void);
extern void fx_reset(void);
extern void movelist_change_style(void);
extern void start_movelist(void);
extern void movelist_change_move(int move);
extern void pselect_update_selbox_pos(int player, int position);
extern void send_player_status_msg(void* player);
extern void pselect_start_code_entry(int player, int pad);
extern int pselect_background_select_available(void);
extern void format_or_recreate_left_device(void);
extern void format_or_recreate_right_device(void);
extern void pselect_handicap_show(int player, int show);
extern void start_loading_kontent_image(void);
extern void bg_pselect_save_team(int player);
extern void bg_pselect_load_team(int player);
extern void wager_completed(void);
extern void wager_cancelled(void);
extern void ck_decrement_bet(void);
extern void ck_decrement_wager_koin_type(void);
extern void ck_increment_wager_koin_type(void);
extern void ck_increment_bet(void);
extern void pselect_random_select(int player);
extern void play_current_soundtrack(void);
extern int pselect_is_random(int player);
extern void kill_kontent_bio_text(void);
extern void unhide_kontent_bio_text(void);
extern void hide_kontent_bio_text(void);
extern void reset_video_defaults(void);
extern void adjust_screen_reset(void);
extern void adjust_screen_position(int direction);
extern void set_snd_vol(int handle, int sound_id, float volume);
extern void SetColorScale__12ScreenObjectFP8SEVec4_t(void* self, void* color);
extern void HandleEvent__22GameVariableDispatcherFP12ScreenObjectii(
    void* self, void* object, int event, int arg);
extern void unload_p1_player_profile(void);
extern void unload_p2_player_profile(void);
extern void pselect_init_arena_select(void);

static void ProcessActionSubActions(const ScreenActionView* action) {
    ProcessSubActions__12ScreenObjectFPC12ScreenActioni(action->owner, action, 0);
}

/*
 * mkScreenEngineClient::HandleAction.
 *
 * Game-specific Screen action dispatcher. The ID set and side-effect ordering
 * come from retail GQNE5D; unknown IDs intentionally do nothing.
 *
 * Soft ceiling after 30 passes: full retail action/call inventory is lifted.
 * Objdiff cannot map the large sparse-switch CFG (reports 0%); opcode-sequence
 * comparison is ~54.8%. Remaining shape is MWCC allocation/frame coloring
 * (r28-r31 / 0x60 versus retail r27-r31 / 0x70) plus one shared tail.
 */
void HandleAction__20mkScreenEngineClientFP9ScreenMgrPC12ScreenActioni(
    ScreenEngineClient* /*self*/, void* mgr, const ScreenActionView* action,
    int /*handled*/) {
    void* params;
    void* node;
    void* activeScreen;
    void* activeLibrary;
    void* animScene;
    void* animScreen;
    int id;
    int player;
    int value;
    int value2;
    int resource;
    unsigned int colorValue;
    unsigned int vertexColor;
    int profileStatus;
    unsigned char* profile;
    unsigned char* colorBytes;
    float colorScale[4];
    unsigned char textColor[4];
    unsigned char savedTextColor[4];

    params = action->params;
    if (params == 0) {
        return;
    }
    if (HandleAction__22GameVariableDispatcherFP9ScreenMgrPC12ScreenAction(
            m_pGameVariables__13ScreenControl, mgr, action) != 0) {
        return;
    }

    id = action->arg;
    switch (id) {
    case 1:
        animScreen = *(void**)((char*)action->owner + 0x20);
        value = GetInt__12ScreenParamsFUi(params, 0);
        animScene = GetAnimScene__6ScreenFi(animScreen, value);
        resource = GetResourceID__12ScreenParamsFUi(params, 1);
        value = GetInt__15mkGameVariablesFi(&game_variables, resource);
        PlayUntilTime__15ScreenAnimSceneFi(
            animScene,
            (int)((float)*(int*)(*(char**)((char*)animScene + 0x14) + 4) *
                  ((float)value / 100.0f)));
        break;
    case 0x432:
        value = GetInt__12ScreenParamsFUi(params, 0);
        if (value != 0 && value == 2) {
            ProcessActionSubActions(action);
        }
        break;
    case 0x7e3:
        RefreshAllOptions__13ScreenControlFP6Screen(
            *(void**)((char*)action->owner + 0x20));
        break;
    case 0x7e4:
        RefreshAllCollections__13ScreenControlFP6Screen(
            *(void**)((char*)action->owner + 0x20));
        break;
    case SE_ACT_SET_TARGET_GAME_MODE:
        player = action->eventUser - 1;
        if (player >= 0) {
            menu_player = player;
        }
        target_game_mode = GetInt__12ScreenParamsFUi(params, 0);
        switch (target_game_mode) {
        case 6:
        case 8:
        case 9:
        case 11:
        case 21:
            if (menu_player == 0) {
                set_player_state(&g_game_info.plyr0, 1);
                set_player_state(&g_game_info.plyr1, 0);
            } else if (menu_player == 1) {
                set_player_state(&g_game_info.plyr1, 1);
                set_player_state(&g_game_info.plyr0, 0);
            }
            break;
        case 7:
            set_player_state(&g_game_info.plyr0, 1);
            set_player_state(&g_game_info.plyr1, 1);
            break;
        }
        if ((void*)aproc->hdr.vtbl != (void*)&vtbl_mkproc_nostack) {
            fade_to_black(8, 1);
        }
        Dispose__9ScreenMgrFUi(screen_manager, 0);
        break;
    case 0x138a:
        resource = GetResourceID__12ScreenParamsFUi(params, 0);
        if ((unsigned int)resource < 0x3d) {
            if (ui_sound_table[resource] == 0x1b4e) {
                MkProc* proc;
                MkHdr* pdata;

                proc = find_mkproc_pid(0x2001);
                if (proc != 0) {
                    pdata = pdata_of_proc(proc);
                    if (pdata != 0) {
                        set_snd_vol(*(int*)((char*)pdata + 8), 0x1c0a,
                                    1.0f);
                    }
                }
            }
            snd_req(ui_sound_table[resource]);
        } else {
            vdebug_print_message(stringBase0 + 0x264, resource);
        }
        break;
    case 0x1392:
        snd_stop_all();
        break;
    case 0x1393:
        {
            ScreenText* text;
            StringObj* live;

            text = (ScreenText*)GetScreenObject__12ScreenParamsFUi(params, 0);
            colorValue = GetColor__12ScreenParamsFUi(params, 1);
            live = text->stringObj;
            if (live != 0 &&
                live->instance != (unsigned int)text->stringObjInstance) {
                live = 0;
            }
            if (live != 0) {
                textColor[0] = ((unsigned char*)&colorValue)[0];
                textColor[1] = ((unsigned char*)&colorValue)[1];
                textColor[2] = ((unsigned char*)&colorValue)[2];
                textColor[3] = ((unsigned char*)&colorValue)[3];
                pfxfont_set_string_color(
                    (PfxFontString*)((char*)live + 0x3c),
                    (unsigned int*)textColor);
                text->flags |= 1;
            }
        }
        break;
    case 0x1395:
        node = GetScreenObject__12ScreenParamsFUi(params, 0);
        colorValue = GetColor__12ScreenParamsFUi(params, 1);
        colorBytes = (unsigned char*)&colorValue;
        colorScale[0] = (float)colorBytes[0] / 255.0f;
        colorScale[1] = (float)colorBytes[1] / 255.0f;
        colorScale[2] = (float)colorBytes[2] / 255.0f;
        colorScale[3] = (float)colorBytes[3] / 255.0f;
        SetColorScale__12ScreenObjectFP8SEVec4_t(node, colorScale);
        break;
    case 0x1394:
        node = GetScreenObject__12ScreenParamsFUi(params, 0);
        value = GetInt__12ScreenParamsFUi(params, 1);
        vertexColor = GetColor__12ScreenParamsFUi(params, 2);
        colorBytes = (unsigned char*)&vertexColor;
        ((ScreenPoly*)node)->verts[vert_map__10ScreenPoly[value]].rgba[0] =
            colorBytes[0];
        ((ScreenPoly*)node)->verts[vert_map__10ScreenPoly[value]].rgba[1] =
            colorBytes[1];
        ((ScreenPoly*)node)->verts[vert_map__10ScreenPoly[value]].rgba[2] =
            colorBytes[2];
        ((ScreenPoly*)node)->verts[vert_map__10ScreenPoly[value]].rgba[3] =
            colorBytes[3];
        ((ScreenPoly*)node)->flags |= 1;
        break;
    case 0x1396:
        node = GetScreenObject__12ScreenParamsFUi(params, 0);
        if (*(void**)((char*)node + 0x14) != 0) {
            fx_resume_emit();
        }
        break;
    case 0x1397:
        node = GetScreenObject__12ScreenParamsFUi(params, 0);
        if (*(void**)((char*)node + 0x14) != 0) {
            fx_reset();
        }
        break;
    case 0x13a7:
        GetInt__12ScreenParamsFUi(params, 0);
        movelist_change_style();
        break;
    case 0x13a8:
        start_movelist();
        break;
    case 0x13a9:
        node = GetScreenObject__12ScreenParamsFUi(params, 0);
        movelist_change_move(*(int*)((char*)node + 0xdc));
        break;
    case 0x1f40:
        value = GetInt__12ScreenParamsFUi(params, 0);
        value2 = GetInt__12ScreenParamsFUi(params, 1);
        node = 0;
        if (value == 1) {
            g_game_info.plyr0.controller_slot = 0;
            node = &g_game_info.plyr0;
        } else if (value == 2) {
            g_game_info.plyr1.controller_slot = 1;
            node = &g_game_info.plyr1;
        }
        if (node != 0) {
            pselect_update_selbox_pos(value - 1, value2);
            if (((PlyrInfo*)node)->pad_index != -1 &&
                ((PlyrInfo*)node)->pad_index != 2) {
                send_player_status_msg(node);
            }
        }
        break;
    case 0x1f42:
        bg_pselect_set_character(GetInt__12ScreenParamsFUi(params, 0) - 1);
        break;
    case 0x1f43:
        player = action->eventUser - 1;
        pselect_start_code_entry(
            player, ((PlyrInfo*)((char*)&g_game_info.plyr0 + player * 0x6c))->pad_index);
        break;
    case 0x1f44:
        if (pselect_background_select_available() != 0) {
            ProcessActionSubActions(action);
        }
        break;
    case 0x1f45:
        player = action->eventUser == 1 ? 0 : 1;
        ((unsigned char*)((char*)&g_game_info.plyr0 + player * 0x6c))[0x14] |= 0x80;
        break;
    case 0x1f46:
        set_ppwls_input_done();
        break;
    case 0x1f47:
        reset_ppwls_timeout();
        break;
    case 0x1f48:
        if (((ScreenGameFlags*)&g_game_info.field_04)->high != 0 &&
            ((ScreenGameFlags*)&g_game_info.field_04)->second != 0) {
            ProcessActionSubActions(action);
        }
        break;
    case 0x1f49:
        format_or_recreate_left_device();
        break;
    case 0x1f4a:
        format_or_recreate_right_device();
        break;
    case 0x1f4d:
        pselect_handicap_show(0, 0);
        break;
    case 0x1f4e:
        pselect_handicap_show(1, 0);
        break;
    case 0x1f4f:
        start_loading_kontent_image();
        break;
    case 0x1f50:
        if (((ScreenGameFlags*)&g_game_info.field_04)->high == 0 &&
            count_all_profiles() != 0) {
            ProcessActionSubActions(action);
        }
        break;
    case 0x1f51:
        if (((ScreenGameFlags*)&g_game_info.field_04)->high == 0) {
            value = GetInt__12ScreenParamsFUi(params, 0);
            if (value == 1) {
                profileStatus = p1_profile_status;
                profile = p1_profile;
            } else {
                profileStatus = p2_profile_status;
                profile = p2_profile;
            }
            if (profileStatus == 1 && *(int*)(profile + 0x514) != 0) {
                ProcessActionSubActions(action);
            }
        }
        break;
    case 0x1f52:
        bg_pselect_save_team(action->eventUser - 1);
        break;
    case 0x1f53:
        bg_pselect_load_team(action->eventUser - 1);
        break;
    case 0x1f56:
        g_game_info.plyr0.player_state = 1;
        fire_screen_studio_event(0x1fa4, 0);
        break;
    case 0x1f57:
        g_game_info.plyr1.player_state = 1;
        fire_screen_studio_event(0x1fa4, 1);
        break;
    case 0x1f58:
        wager_completed();
        break;
    case 0x1f59:
        wager_cancelled();
        break;
    case 0x1f5b:
        ck_decrement_bet();
        break;
    case 0x1f5c:
        ck_decrement_wager_koin_type();
        break;
    case 0x1f5d:
        ck_increment_wager_koin_type();
        break;
    case 0x1f5a:
        ck_increment_bet();
        break;
    case 0x1f5f:
        pselect_random_select(action->eventUser - 1);
        break;
    case 0x1f60:
        play_current_soundtrack();
        break;
    case 0x1f62:
        if (g_game_info.pad_overlay.pselect.field_1d0 != 0) {
            ProcessActionSubActions(action);
        }
        break;
    case 0x1f63:
        pselect_player_canceled(GetInt__12ScreenParamsFUi(params, 0) - 1);
        break;
    case 0x1f64:
        if (pselect_is_random(action->eventUser - 1) != 0) {
            ProcessActionSubActions(action);
        }
        break;
    case 0x1f65:
        kill_kontent_bio_text();
        break;
    case 0x1f66:
        unhide_kontent_bio_text();
        break;
    case 0x1f67:
        hide_kontent_bio_text();
        break;
    case 0x2328:
        value = GetInt__12ScreenParamsFUi(params, 0);
        node = GetScreenNode__12ScreenParamsFUi(params, 1);
        if (GetCount__12ScreenParamsCFv(params) == 4) {
            value2 = GetInt__12ScreenParamsFUi(params, 2);
            resource = GetInt__12ScreenParamsFUi(params, 3);
        } else {
            value2 = 0x100;
            resource = 0x100;
        }
        mkMovieTexInit(value, node, value2, resource);
        break;
    case 0x2329:
        screen_engine_play_movie(GetResourceID__12ScreenParamsFUi(params, 1));
        break;
    case 0x232a:
        movie_player_reset();
        break;
    case 0x232b:
        mkMovieTexStop(GetInt__12ScreenParamsFUi(params, 0));
        break;
    case 0x235a:
        fade_to_black(8, 1);
        snd_stop_all();
        break;
    case 0x2afa:
        {
            ScreenText* text;
            StringObj* live;
            void* libraryOwner;
            char* name;
            char* string;
            int activeIndex;
            char* (*lookup)(void*, char*);

            text = (ScreenText*)GetScreenObject__12ScreenParamsFUi(params, 0);
            name = GetName__12ScreenParamsFUi(params, 1);
            activeIndex = *(int*)((char*)mgr + 0x1a4);
            activeScreen =
                activeIndex < 0
                    ? 0
                    : *(void**)((char*)mgr + 0x1a8 + activeIndex * 4);
            libraryOwner = *(void**)((char*)activeScreen + 0x54);
            activeLibrary = *(void**)((char*)libraryOwner + 8);
            lookup = *(char* (**)(void*, char*))(
                *(char**)activeLibrary + 0x10);
            string = lookup(activeLibrary, name);

            live = text->stringObj;
            if (live != 0 &&
                live->instance != (unsigned int)text->stringObjInstance) {
                live = 0;
            }
            if (live != 0) {
                savedTextColor[0] = *((unsigned char*)live + 0xb4);
                savedTextColor[1] = *((unsigned char*)live + 0xb5);
                savedTextColor[2] = *((unsigned char*)live + 0xb6);
                savedTextColor[3] = *((unsigned char*)live + 0xb7);
                update_string_obj_pfx(live, text->font, string);
                pfxfont_set_string_color(
                    (PfxFontString*)((char*)live + 0x3c),
                    (unsigned int*)savedTextColor);
            }
        }
        break;
    case 0x2af8:
        resource = GetResourceID__12ScreenParamsFUi(params, 0);
        value = GetInt__12ScreenParamsFUi(params, 1);
        SetInt__22GameVariableDispatcherFUiUii(
            m_pGameVariables__13ScreenControl, 0, resource, value);
        break;
    case 0x2b01:
        resource = GetResourceID__12ScreenParamsFUi(params, 0);
        SetString__22GameVariableDispatcherFiPc(
            m_pGameVariables__13ScreenControl, resource,
            (char*)stringBase0 + 0x1c8);
        RefreshAllOptions__13ScreenControlFP6Screen(
            *(void**)((char*)action->owner + 0x20));
        break;
    case 0x2b03:
        resource = GetResourceID__12ScreenParamsFUi(params, 0);
        value = GetInt__12ScreenParamsFUi(params, 1);
        SetInt__22GameVariableDispatcherFUiUii(
            m_pGameVariables__13ScreenControl, 0, resource,
            GetInt__22GameVariableDispatcherFi(m_pGameVariables__13ScreenControl,
                                               resource) + value);
        break;
    case 0x2c88:
        {
            MkProc* proc;
            MkHdr* pdata;

            reset_default_audio_settings();
            proc = find_mkproc_pid(0x2001);
            if (proc == 0) {
                return;
            }
            pdata = pdata_of_proc(proc);
            if (pdata == 0) {
                return;
            }
            set_snd_vol(*(int*)((char*)pdata + 8), 0x1c0a, 1.0f);
        }
        break;
    case 0x2ee0:
        save_game_settings_in_action_handler();
        break;
    case 0x2ee1:
        pselect_start_code_entry(
            0, ((PlyrInfo*)((char*)&g_game_info.plyr0 +
                           (action->eventUser - 1) * 0x6c))->pad_index);
        break;
    case 0x2ee2:
        reset_video_defaults();
        break;
    case 0x32c8:
        reset_default_gameplay_settings();
        break;
    case 0x36b0:
        reset_video_defaults();
        break;
    case 0x36b2:
        adjust_screen_reset();
        break;
    case 0x36b3:
        adjust_screen_position(GetInt__12ScreenParamsFUi(params, 0));
        break;
    case 0x6590:
        resource = GetResourceID__12ScreenParamsFUi(params, 0);
        value = GetInt__12ScreenParamsFUi(params, 1);
        value2 = GetInt__12ScreenParamsFUi(params, 2);
        if (ScreenIntegerCompare__Fiii(
                GetInt__22GameVariableDispatcherFi(
                    m_pGameVariables__13ScreenControl, resource),
                value, value2) != 0) {
            ProcessActionSubActions(action);
        }
        break;
    case 0xff0007:
        pp_name_entry_proces_char_entry(
            GetName__12ScreenParamsFUi(params, 0));
        break;
    }
}

void HandleEvent__20mkScreenEngineClientFP12ScreenObjectii(
    ScreenEngineClient* self, void* object, int event, int arg) {
    PlyrInfo* player;

    (void)self;
    if (event != 0x92826) {
        if (event < 0x92826) {
            switch (event) {
            default:
                player = &g_game_info.plyr1;
                if ((unsigned int)(event + 0xFFF70000) == 0x2824) {
                    player = &g_game_info.plyr0;
                }
                pselect_player_selected(player);
                break;
            case 0x1FBA:
                pselect_set_arena(-1);
                pselect_bgnd_select_done();
                break;
            case 0x1FB9:
                pselect_bgnd_select_done();
                break;
            case 0x1FC4:
                if (p1_profile_status != 0) {
                    unload_p1_player_profile();
                }
                profile_code_state[0] = 0;
                fire_screen_studio_event(0x1FE4, 1);
                break;
            case 0x1FC5:
            if (p2_profile_status != 0) {
                unload_p2_player_profile();
            }
                profile_code_state[1] = 0;
            fire_screen_studio_event(0x1FE4, 1);
                break;
            case 0x1FEF:
                destroy_mkprocs_pid(0x20A5);
                break;
            }
        } else if (event != 0x9282B) {
            if (event < 0x9282B) {
                if (event < 0x9282A) {
                    if (event < 0x92828) {
                        bg_pselect_player_canceled(1);
                    } else {
                        pselect_init_arena_select();
                    }
                } else {
                    pselect_handicap_show(0, 1);
                }
            } else {
                switch (event) {
                case 0x92830:
                    pselect_player_moved(0);
                    break;
                case 0x92831:
                    pselect_player_moved(1);
                    break;
                }
            }
        } else {
            pselect_handicap_show(1, 1);
        }
    } else {
        bg_pselect_player_canceled(0);
    }
    HandleEvent__22GameVariableDispatcherFP12ScreenObjectii(
        m_pGameVariables__13ScreenControl, object, event, arg);
}

/* Localized KeyPad special-key labels (retail @2631/@2632/@2633). */
static const char* const s_keyPadDel[6] = {
    stringBase0 + 0x29, /* DEL */
    stringBase0 + 0x2D, /* SUPR */
    stringBase0 + 0x32, /* ENTF */
    stringBase0 + 0x37, /* SUPPR */
    stringBase0 + 0x3D, /* Canc */
    stringBase0 + 0x29, /* DEL */
};
static const char* const s_keyPadSpc[6] = {
    stringBase0 + 0x42, /* SPC */
    stringBase0 + 0x46, /* ESP. */
    stringBase0 + 0x4B, /* LEER */
    stringBase0 + 0x50, /* ESPACE */
    stringBase0 + 0x57, /* SPZ */
    stringBase0 + 0x42, /* SPC */
};
static const char* const s_keyPadEnd[6] = {
    stringBase0 + 0x5B, /* END */
    stringBase0 + 0x5F, /* FIN */
    stringBase0 + 0x63, /* ENDE */
    stringBase0 + 0x5F, /* FIN (FR shares ES) */
    stringBase0 + 0x68, /* Fine */
    stringBase0 + 0x5B, /* END */
};

void Init__8KeyEntryFv(KeyEntry* self) {
    Init__13ScreenControlFv(self);
}

void Dispose__8KeyEntryFv(KeyEntry* self) {
    Dispose__13ScreenControlFv(self);
}

void Init__6KeyPadFv(KeyPad* self) {
    Init__13ScreenControlFv(self);
}

void Dispose__6KeyPadFv(KeyPad* self) {
    Dispose__13ScreenControlFv(self);
}

void Init__8TextItemFv(TextItem* self) {
    Init__13ScreenControlFv(self);
}

void Init__8WifImageFv(WifImage* /*self*/) {}

/*
 * KeyPad::SetKey -- DEL / SPC / END / char insert against editBuf.
 * Soft ceiling: language-table stack copy vs static ptr tables; stop.
 */
void SetKey__6KeyPadFP9ScreenMgrPC12ScreenActionPc(KeyPad* self, void* mgr,
                                                     const void* actionIn, char* key) {
    const ScreenActionView* action;
    int user;
    int lang;

    action = (const ScreenActionView*)actionIn;
    user = action->eventUser;
    lang = get_language_setting();

    if (strcmp(s_keyPadDel[lang], key) == 0) {
        /* Backspace / delete last char. */
        if (self->editLen >= self->maxLen) {
            FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, 0xFB0000, user, 0);
        }
        if (self->editLen > 0) {
            self->editLen -= 1;
            self->editBuf[self->editLen] = 0;
        }
    } else if (strcmp(s_keyPadSpc[lang], key) == 0) {
        /* Insert space when under maxLen. */
        if (self->editLen < self->maxLen) {
            self->editBuf[self->editLen] = ' ';
            self->editLen += 1;
            self->editBuf[self->editLen] = 0;
        }
    } else if (strcmp(s_keyPadEnd[lang], key) == 0) {
        /* Page advance / wrap; early return skips SetString. */
        if (IsValidOption__22GameVariableDispatcherFUiUi(
                m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
                (unsigned int)self->pageResIds[self->pageIndex]) == 0) {
            FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, 0xFC0000, user, 0);
        } else {
            self->pageIndex += 1;
            if (self->pageIndex >= self->pageCount) {
                self->pageIndex = self->pageCount - 1;
                FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, 0xFD0001, user, 0);
            } else {
                (*(void (**)(KeyPad*))(*(unsigned char**)self + 0x4c))(self);
                FireEvent__12ScreenObjectFP9ScreenMgriiUi(
                    self, mgr, self->pageIndex + 0xFDFFFF, user, 0);
                FireEvent__12ScreenObjectFP9ScreenMgriiUi(
                    self, mgr, self->pageIndex + 0xFF0000, user, 0);
            }
        }
        FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, 0xFA0001, user, 0);
        return;
    } else if (self->editLen < self->maxLen) {
        /* Normal char: optional case fold from active latch, then append. */
        if (self->active != 0) {
            if (key[0] >= 'a' && key[0] <= 'z') {
                key[0] = (char)(key[0] - 0x20);
            }
        } else {
            if (key[0] >= 'A' && key[0] <= 'Z') {
                key[0] = (char)(key[0] + 0x20);
            }
        }
        self->editBuf[self->editLen] = key[0];
        self->editLen += 1;
        self->editBuf[self->editLen] = 0;
    } else {
        FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, 0xFA0000, user, 0);
    }

    SetString__22GameVariableDispatcherFUiUiPc(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->pageResIds[self->pageIndex], self->editBuf);
}

/*
 * KeyPad::HandleAction -- page set/inc/dec, compares, SetKey, ChangeCase.
 * Soft ceiling: retail jump table @6120 vs switch; stop.
 */
int HandleAction__6KeyPadFP9ScreenMgrPC12ScreenAction(KeyPad* self, void* mgr,
                                                      const void* actionIn) {
    const ScreenActionView* action;
    void* params;
    int arg;
    int result;
    int pageChanged;
    int prevPage;
    int user;
    char* name;

    action = (const ScreenActionView*)actionIn;
    arg = action->arg;
    params = action->params;
    user = action->eventUser;
    result = 1;
    pageChanged = 0;
    prevPage = self->pageIndex;

    switch (arg) {
    case 0xFF0001:
        self->pageIndex = GetInt__12ScreenParamsFUi(params, 0);
        pageChanged = 1;
        break;
    case 0xFF0002:
        pageChanged = 1;
        self->pageIndex += GetInt__12ScreenParamsFUi(params, 0);
        if (self->pageIndex >= self->pageCount) {
            self->pageIndex = self->pageCount - 1;
        } else {
            (*(void (**)(KeyPad*))(*(unsigned char**)self + 0x4c))(self);
            FireEvent__12ScreenObjectFP9ScreenMgriiUi(
                self, mgr, self->pageIndex + 0xFDFFFF, user, 0);
            FireEvent__12ScreenObjectFP9ScreenMgriiUi(
                self, mgr, self->pageIndex + 0xFF0000, user, 0);
        }
        break;
    case 0xFF0003:
        pageChanged = 1;
        self->pageIndex -= GetInt__12ScreenParamsFUi(params, 0);
        if (self->pageIndex < 0) {
            self->pageIndex = 0;
        } else {
            (*(void (**)(KeyPad*))(*(unsigned char**)self + 0x4c))(self);
            FireEvent__12ScreenObjectFP9ScreenMgriiUi(
                self, mgr, self->pageIndex + 0xFE0001, user, 0);
            FireEvent__12ScreenObjectFP9ScreenMgriiUi(
                self, mgr, self->pageIndex + 0xFF0000, user, 0);
        }
        break;
    case 0xFF0004:
        if (self->pageIndex == GetInt__12ScreenParamsFUi(params, 0)) {
            ProcessSubActions__12ScreenObjectFPC12ScreenActioni(self, actionIn, 0);
        }
        break;
    case 0xFF0005:
        if (self->pageIndex != GetInt__12ScreenParamsFUi(params, 0)) {
            ProcessSubActions__12ScreenObjectFPC12ScreenActioni(self, actionIn, 0);
        }
        break;
    case 0xFF0007:
        name = GetName__12ScreenParamsFUi(params, 0);
        SetKey__6KeyPadFP9ScreenMgrPC12ScreenActionPc(self, mgr, actionIn, name);
        break;
    case 0xFF0010:
        /* Toggle case: ChangeCase(!active) via cntlzw/extrwi. */
        ChangeCase__6KeyPadFUi(self, self->active == 0);
        break;
    default:
        result = HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(self, mgr,
                                                                           actionIn);
        break;
    }

    if (pageChanged != 0 && prevPage != self->pageIndex) {
        user = action->eventUser;
        FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, self->pageIndex + 0xFDFFFF,
                                                  user, 0);
        FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, self->pageIndex + 0xFF0000,
                                                  user, 0);
    }
    return result;
}

/*
 * KeyEntry::HandleAction -- 0xFF0007 looks up key label via Screen resourceLib,
 * then SetKey on parent KeyPad at +0x24.
 */
int HandleAction__8KeyEntryFP9ScreenMgrPC12ScreenAction(KeyEntry* self, void* mgr,
                                                        const void* actionIn) {
    const ScreenActionView* action;
    char* name;
    ScreenView* screen;
    KeyPad* keypad;
    ScreenResourceLibView* lib;
    char* (*getStr)(ScreenResourceLibView* lib, char* key);
    char* keyStr;

    action = (const ScreenActionView*)actionIn;
    if (action->arg == 0xFF0007) {
        name = GetName__12ScreenParamsFUi(action->params, 0);
        screen = *(ScreenView**)((char*)self + 0x20);
        keypad = *(KeyPad**)((char*)self + 0x24);
        lib = screen->set->resourceLib;
        getStr = *(char* (**)(ScreenResourceLibView*, char*))(*(unsigned char**)lib + 0x10);
        keyStr = getStr(lib, name);
        SetKey__6KeyPadFP9ScreenMgrPC12ScreenActionPc(keypad, mgr, actionIn, keyStr);
        return 1;
    }
    return HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(self, mgr,
                                                                     actionIn);
}

void HandleEvent__6KeyPadFP9ScreenMgrii(KeyPad* self, void* mgr, int event, int arg) {
    if (event != 0x407) {
        return;
    }
    (*(void (**)(KeyPad*))(*(unsigned char**)self + 0x4c))(self);
    (*(void (**)(KeyPad*))(*(unsigned char**)self + 0x48))(self);
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, mgr, self->pageIndex + 0xFF0000, arg,
                                              0);
}

/*
 * KeyPad::RefreshOption -- pull GV string into editBuf (cap 0x1F chars).
 */
void RefreshOption__6KeyPadFv(KeyPad* self) {
    char* src;
    int i;
    char ch;

    self->editLen = 0;
    src = GetString__22GameVariableDispatcherFUiUi(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->pageResIds[self->pageIndex]);
    if (src == 0) {
        self->editBuf[0] = 0;
        return;
    }
    for (;;) {
        i = self->editLen;
        ch = src[i];
        if (ch == 0 || i >= 0x1f) {
            break;
        }
        self->editBuf[i] = ch;
        self->editLen = i + 1;
    }
    self->editBuf[self->editLen] = 0;
}

/* Visible wrap-line page size from ScreenText live StringObj / font metrics. */
static int TextItemPageLines(TextItem* self) {
    ScreenText* text;
    StringObj* live;
    int lines;

    lines = 1;
    text = (ScreenText*)self->textNode;
    if (text == 0 || text->font == 0) {
        return lines;
    }
    live = ScreenTextLiveObj(text);
    if (live == 0) {
        return lines;
    }
    lines = live->y_off / (int)text->font->metrics->line_height;
    if (lines < 1) {
        lines = 1;
    }
    return lines;
}

static void TextItemRestoreEditChar(TextItem* self) {
    if (self->editBuf != 0 && self->cursorPos != -1 && self->curChar != -1) {
        self->editBuf[self->cursorPos] = (char)self->curChar;
        self->cursorPos = -1;
        self->curChar = (signed char)-1;
    }
}

static void TextItemFreeScrollState(TextItem* self) {
    if (self->indexTable != 0) {
        Free__10ScreenUtilFPv(self->indexTable);
        self->indexTable = 0;
    }
    if (self->editBuf != 0) {
        TextItemRestoreEditChar(self);
        Free__10ScreenUtilFPv(self->editBuf);
        self->editBuf = 0;
    }
}

static void TextItemRefreshNode(TextItem* self) {
    void* node;

    node = self->textNode;
    (*(void (**)(void*))(*(unsigned char**)node + 0x0c))(node);
}

/*
 * TextItem::UpdateString -- push gvString or scrolled editBuf into live pfx.
 * Soft ceiling: live-obj diamond / color schedule; stop.
 */
void UpdateString__8TextItemFv(TextItem* self) {
    ScreenText* text;
    StringObj* live;
    unsigned char color[4];
    int pageLines;
    int holeAt;
    char* str;

    if (self->scrollLimit <= 1) {
        text = (ScreenText*)self->textNode;
        live = ScreenTextLiveObj(text);
        if (live != 0) {
            color[0] = live->pfx.instance0.rgba[0];
            color[1] = live->pfx.instance0.rgba[1];
            color[2] = live->pfx.instance0.rgba[2];
            color[3] = live->pfx.instance0.rgba[3];
            update_string_obj_pfx(live, text->font, self->gvString);
            pfxfont_set_string_color(&live->pfx, (unsigned int*)color);
        }
        return;
    }

    if (self->editBuf == 0) {
        return;
    }

    pageLines = TextItemPageLines(self);
    holeAt = self->scrollPos + pageLines;
    TextItemRestoreEditChar(self);
    if (holeAt < self->scrollLimit) {
        self->cursorPos = self->indexTable[holeAt];
        self->curChar = (signed char)self->editBuf[self->indexTable[holeAt]];
        self->editBuf[self->indexTable[holeAt]] = 0;
    }

    text = (ScreenText*)self->textNode;
    live = ScreenTextLiveObj(text);
    if (live != 0) {
        color[0] = live->pfx.instance0.rgba[0];
        color[1] = live->pfx.instance0.rgba[1];
        color[2] = live->pfx.instance0.rgba[2];
        color[3] = live->pfx.instance0.rgba[3];
        str = self->editBuf + self->indexTable[self->scrollPos];
        update_string_obj_pfx(live, text->font, str);
        pfxfont_set_string_color(&live->pfx, (unsigned int*)color);
    }
}

/*
 * TextItem::ScrollText -- dir==0 subtract amount; else add + clamp to page.
 */
void ScrollText__8TextItemFii(TextItem* self, int dir, int amount) {
    int pageLines;

    if (dir == 0) {
        self->scrollPos -= amount;
        if (self->scrollPos < 0) {
            self->scrollPos = 0;
        }
    } else {
        pageLines = TextItemPageLines(self);
        self->scrollPos += amount;
        if (self->scrollLimit - self->scrollPos < pageLines) {
            self->scrollPos = self->scrollLimit - pageLines;
            if (self->scrollPos < 0) {
                self->scrollPos = 0;
            }
        }
    }
    UpdateString__8TextItemFv(self);
    TextItemRefreshNode(self);
}

void Dispose__8TextItemFv(TextItem* self) {
    Dispose__13ScreenControlFv(self);
    TextItemFreeScrollState(self);
}

/*
 * TextItem::RefreshOption -- GetString, GetStartArray, clamp scroll, copy buf.
 * Soft ceiling: page-lines / scroll clamp schedule; stop.
 */
void RefreshOption__8TextItemFv(TextItem* self) {
    int pageLines;
    int prevLimit;
    int wasAtEnd;
    int remain;
    ScreenText* text;

    if (self->textNode == 0) {
        return;
    }

    pageLines = TextItemPageLines(self);
    wasAtEnd = (self->scrollLimit - self->scrollPos - pageLines) == 0;
    prevLimit = self->scrollLimit;

    self->gvString = GetString__22GameVariableDispatcherFUiUi(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->optionId);
    if (self->gvString == 0) {
        return;
    }

    TextItemFreeScrollState(self);
    text = (ScreenText*)self->textNode;
    self->indexTable =
        GetStartArray__10ScreenTextFPcRi(text, self->gvString, &self->scrollLimit);

    if (prevLimit == -1) {
        prevLimit = self->scrollLimit;
        wasAtEnd = 0;
    }

    pageLines = TextItemPageLines(self);
    if (wasAtEnd != 0) {
        self->scrollPos += self->scrollLimit - prevLimit;
        remain = self->scrollLimit - pageLines;
        if (remain <= 0) {
            self->scrollPos = 0;
        } else if (self->scrollLimit - self->scrollPos >= pageLines) {
            self->scrollPos = remain;
        }
    } else if (self->scrollPos >= self->scrollLimit) {
        self->scrollPos = self->scrollLimit - pageLines;
        if (self->scrollPos < 0) {
            self->scrollPos = 0;
        }
    }

    (*(void (**)(void*))(*(unsigned char**)self->textNode + 0x10))(self->textNode);

    if (self->scrollLimit > 1) {
        self->editBuf = (char*)Malloc__10ScreenUtilFUliPc(
            (unsigned long)(strlen(self->gvString) + 1), kMallocTagInit,
            (char*)(stringBase0 + 0x294));
        strcpy(self->editBuf, self->gvString);
    }

    UpdateString__8TextItemFv(self);
    TextItemRefreshNode(self);
}

/*
 * TextItem::HandleAction -- scroll / reset / end / compare / show-hide 0x7db.
 * Soft ceiling: retail signed cmp tree vs switch; stop.
 */
int HandleAction__8TextItemFP9ScreenMgrPC12ScreenAction(TextItem* self, void* mgr,
                                                        const void* actionIn) {
    const ScreenActionView* action;
    void* params;
    int arg;
    int result;
    int pageLines;
    int p0;
    int p1;
    void* node;

    action = (const ScreenActionView*)actionIn;
    arg = action->arg;
    params = action->params;
    result = 1;

    switch (arg) {
    case 0xF1000000:
        if (params == 0 || self->textNode == 0 || self->indexTable == 0) {
            break;
        }
        ScrollText__8TextItemFii(self, GetInt__12ScreenParamsFUi(params, 0), 1);
        break;
    case 0xF1000001:
        if (params == 0 || self->textNode == 0 || self->indexTable == 0) {
            break;
        }
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        ScrollText__8TextItemFii(self, p0, p1);
        break;
    case 0xF1000002:
        self->scrollPos = 0;
        if (self->textNode == 0 || self->indexTable == 0 || self->editBuf == 0) {
            break;
        }
        UpdateString__8TextItemFv(self);
        TextItemRefreshNode(self);
        break;
    case 0xF1000003:
        if (self->scrollPos != 0) {
            ProcessSubActions__12ScreenObjectFPC12ScreenActioni(self, actionIn, 0);
        }
        break;
    case 0xF1000004:
        pageLines = TextItemPageLines(self);
        if (self->scrollPos < self->scrollLimit - pageLines) {
            ProcessSubActions__12ScreenObjectFPC12ScreenActioni(self, actionIn, 0);
        }
        break;
    case 0xF1000005:
        if (self->textNode == 0 || self->indexTable == 0 || self->editBuf == 0) {
            break;
        }
        pageLines = TextItemPageLines(self);
        self->scrollPos = self->scrollLimit - pageLines;
        if (self->scrollPos < 0) {
            self->scrollPos = 0;
        }
        UpdateString__8TextItemFv(self);
        TextItemRefreshNode(self);
        break;
    case 0x7db:
        node = GetScreenNode__12ScreenParamsFUi(params, 0);
        if (node == (void*)self) {
            p1 = GetInt__12ScreenParamsFUi(params, 1);
            if (p1 == 0) {
                (*(void (**)(TextItem*))(*(unsigned char**)self + 0x48))(self);
            } else if (p1 == 1) {
                (*(void (**)(TextItem*))(*(unsigned char**)self + 0x4c))(self);
            }
        } else if (node != 0) {
            (*(int (**)(void*, void*, const void*))(*(unsigned char**)node + 0x28))(
                node, mgr, actionIn);
        }
        break;
    default:
        result = HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(self, mgr,
                                                                           actionIn);
        break;
    }
    return result;
}

void HandleEvent__8TextItemFP9ScreenMgrii(TextItem* self, void* /*mgr*/, int event,
                                          int /*arg*/) {
    if (event == 0x407 || event == 0x1389) {
        self->scrollPos = 0;
        self->scrollLimit = -1;
        (*(void (**)(TextItem*))(*(unsigned char**)self + 0x4c))(self);
    } else if (event == 0x3eb) {
        TextItemFreeScrollState(self);
    }
}

/*
 * WifImage ATC release -- Clear curTexture when instance latch matches.
 */
static void WifImageReleaseAtc(WifImage* self) {
    AniTextureControl* atc;

    atc = (AniTextureControl*)self->curTexture;
    if (atc == 0) {
        return;
    }
    if (atc->instance != self->curTextureInstance) {
        return;
    }
    if (atc->instance != 0) {
        (*(int (**)(AniTextureControl*))(*(unsigned char**)atc + 0x10))(atc);
    }
    self->curTexture = 0;
    self->curTextureInstance = 0;
}

void Close__8WifImageFv(WifImage* self) {
    self->started = 0;
    WifImageReleaseAtc(self);
}

void Dispose__8WifImageFv(WifImage* self) {
    Dispose__13ScreenControlFv(self);
    WifImageReleaseAtc(self);
}

/*
 * WifImage::HandleEvent -- on 0x405 once: build ATC from images[] onto status poly.
 * Soft ceiling: ScreenPoly live diamond schedule; stop.
 */
void HandleEvent__8WifImageFP9ScreenMgrii(WifImage* self, void* /*mgr*/, int event,
                                          int /*arg*/) {
    ScreenPoly* poly;
    ScreenObj* live;
    AniTextureControl* atc;
    int i;

    if (event != 0x405 || self->started != 0) {
        return;
    }
    poly = (ScreenPoly*)self->statusNode;
    live = poly->screenObj;
    if (live != 0) {
        if (live->instance != poly->screenObjInstance) {
            live = 0;
        }
    } else {
        live = 0;
    }
    if (live == 0) {
        return;
    }

    atc = get_ani_texture_control();
    set_ani_texture_numframes(atc, self->imageCount);
    set_ani_texture_framerate(atc, self->unkF8);
    for (i = 0; i < self->imageCount; i++) {
        set_ani_texture_rwtexture(atc, i, (RwTexture*)self->images[i]);
    }
    set_ani_texture_screen_obj(atc, live);
    insert_ani_texture_control(atc);
    self->curTexture = atc;
    self->curTextureInstance = atc->instance;
    self->started = 1;
}

/*
 * SpreadSheet::ProcessParams -- shared SPSH/SPSI param fill, then vtbl
 * FinishSetup(params, nextIndex). Used by profile/mem-screen grids.
 * Soft ceiling: ~98.4% -- init order and vtbl temp scheduling; stop.
 */
void ProcessParams__11SpreadSheetFP12ScreenParams(SpreadSheet* self, void* params) {
    int nextIndex;
    int bits;

    self->rows = GetInt__12ScreenParamsFUi(params, 0);
    self->cols = GetInt__12ScreenParamsFUi(params, 1);
    self->unkE8 = GetInt__12ScreenParamsFUi(params, 2);
    self->unkEC = GetInt__12ScreenParamsFUi(params, 3);
    /* Retail boolize: neg/andc/srwi => (value != 0). */
    self->flag104 = (int)((unsigned int)(-self->unkE8 & ~self->unkE8) >> 31);
    self->flag108 = (int)((unsigned int)(-self->unkEC & ~self->unkEC) >> 31);
    self->collectionId = GetResourceID__12ScreenParamsFUi(params, 4);
    self->optionId = GetResourceID__12ScreenParamsFUi(params, 5);
    self->useColor = GetBoolean__12ScreenParamsFUi(params, 6);
    bits = GetInt__12ScreenParamsFUi(params, 7);
    bits &= 1;
    self->bindNodeB0 = (int)((unsigned int)(-bits & ~bits) >> 31);
    bits = GetInt__12ScreenParamsFUi(params, 7);
    bits &= 2;
    self->bindNodeB4 = (int)((unsigned int)(-bits & ~bits) >> 31);
    self->flagF0 = GetBoolean__12ScreenParamsFUi(params, 8);
    nextIndex = 0xa;
    /* hasExtraRes lives on subclasses at +0x10C -- same store for SPSH/SPSI. */
    ((SpreadSheet_text*)self)->hasExtraRes = GetBoolean__12ScreenParamsFUi(params, 9);
    if (self->useColor != 0) {
        unsigned int colorWord;
        unsigned char* colorBytes;

        colorWord =
            GetColor__12ScreenParamsFUi(params, (unsigned int)nextIndex++);
        colorBytes = (unsigned char*)&colorWord;
        self->color[0] = colorBytes[0];
        self->color[1] = colorBytes[1];
        self->color[2] = colorBytes[2];
        self->color[3] = colorBytes[3];
    }
    if (((SpreadSheet_text*)self)->hasExtraRes != 0) {
        ((SpreadSheet_text*)self)->extraResId =
            GetResourceID__12ScreenParamsFUi(params, (unsigned int)nextIndex++);
    }
    if (self->bindNodeB0 != 0) {
        self->nodeB0 =
            GetScreenNode__12ScreenParamsFUi(params, (unsigned int)nextIndex++);
    }
    if (self->bindNodeB4 != 0) {
        self->nodeB4 =
            GetScreenNode__12ScreenParamsFUi(params, (unsigned int)nextIndex++);
    }
    (*(void (**)(SpreadSheet*, void*, int))(*(unsigned char**)self + 0x54))(
        self, params, nextIndex);
    if (m_pGameVariables__13ScreenControl != 0) {
        self->cellArray = 1;
        self->unkEC = 0;
        self->unkE8 = 0;
    } else {
        self->rows = 0;
        self->cols = 0;
        self->unkE8 = 0;
        self->unkEC = 0;
    }
}

extern void FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
    void* self, unsigned int unused, unsigned int id, char** strings,
    unsigned int count);
extern unsigned int GetStringCollection__22GameVariableDispatcherFUiUiPPPc(
    void* self, unsigned int unused, unsigned int id, char*** out);
extern int GetInt__22GameVariableDispatcherFUiUi(void* self, unsigned int unused,
                                                 unsigned int id);
extern void GetIntArray__22GameVariableDispatcherFUiUiPii(
    void* self, unsigned int unused, unsigned int id, int* out, int count);
extern void SetIntArray__22GameVariableDispatcherFUiUiPii(
    void* self, unsigned int unused, unsigned int id, int* values, int count);
extern int GetRowState__22GameVariableDispatcherFUiUii(void* self, unsigned int unused,
                                                       unsigned int id, int row);
extern void SetRowState__22GameVariableDispatcherFUiUiii(void* self, unsigned int unused,
                                                         unsigned int id, int row,
                                                         int value);
extern int GetColState__22GameVariableDispatcherFUiUii(void* self, unsigned int unused,
                                                       unsigned int id, int col);
extern void SetColState__22GameVariableDispatcherFUiUiii(void* self, unsigned int unused,
                                                         unsigned int id, int col,
                                                         int value);
extern void SetInt__22GameVariableDispatcherFUiUii(void* self, unsigned int unused,
                                                   unsigned int id, int value);
extern unsigned int IsValidInt__22GameVariableDispatcherFUiUiUiUii(
    void* self, unsigned int a, unsigned int b, unsigned int c, unsigned int id,
    int value);
extern unsigned int ScreenIntegerCompare__Fiii(int lhs, int op, int rhs);
extern void vdebug_print_message(const char* fmt, ...);
extern void set_snd_vol(int handle, int sound_id, float volume);
extern void snd_req(int sound_id);
extern int ui_sound_table[];

/*
 * SpreadSheet nav click -- extraResId indexes ui_sound_table (< 0x3D).
 * Special id 0x1B4E ducks pid 0x2001 via set_snd_vol before snd_req.
 */
static void SpreadSheetPlayNavSound(SpreadSheet* self) {
    SpreadSheet_text* ss;
    unsigned int soundIndex;
    int soundId;
    MkProc* proc;
    MkHdr* pdata;

    ss = (SpreadSheet_text*)self;
    if (ss->hasExtraRes == 0) {
        return;
    }
    soundIndex = (unsigned int)ss->extraResId;
    if (soundIndex >= 0x3Du) {
        vdebug_print_message(stringBase0 + 0x264, (int)soundIndex);
        return;
    }
    soundId = ui_sound_table[soundIndex];
    if (soundId == 0x1B4E) {
        proc = find_mkproc_pid(0x2001);
        if (proc != 0) {
            pdata = pdata_of_proc(proc);
            if (pdata != 0) {
                set_snd_vol(*(int*)((char*)pdata + 8), 0x1C0A, 1.0f);
            }
        }
    }
    snd_req(soundId);
}

void Init__11SpreadSheetFv(SpreadSheet* self) {
    Init__13ScreenControlFv(self);
}

/*
 * SpreadSheet::RefreshCollection -- ClearContents + AllocateCollection, clamp
 * focus/window into counts, optional Update (vtbl+0x44).
 */
void RefreshCollection__11SpreadSheetFv(SpreadSheet* self) {
    void** vtbl;
    void (*clearContents)(SpreadSheet* self);
    void (*allocateCollection)(SpreadSheet* self);
    void (*update)(SpreadSheet* self);

    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    if (self->collectionId == -1) {
        return;
    }
    vtbl = (void**)self->vtbl;
    clearContents = (void (*)(SpreadSheet*))vtbl[0x58 / 4];
    clearContents(self);
    allocateCollection = (void (*)(SpreadSheet*))vtbl[0x50 / 4];
    allocateCollection(self);
    if (self->scrollY >= self->unkE8 && self->unkE8 > 0) {
        self->scrollY = self->unkE8 - 1;
        self->scrollX = self->scrollY - self->rows + 1;
        if (self->scrollX < 0) {
            self->scrollX = 0;
        }
    }
    if (self->unkDC >= self->unkEC && self->unkEC > 0) {
        self->unkDC = self->unkEC - 1;
        self->unkD4 = self->unkDC - self->cols + 1;
        if (self->unkD4 < 0) {
            self->unkD4 = 0;
        }
    }
    if (self->cellArray != 0) {
        update = (void (*)(SpreadSheet*))vtbl[0x44 / 4];
        update(self);
    }
}

/*
 * SpreadSheet::RefreshOption(int) -- GetIntArray into +0xD0 (4 ints), clamp
 * focus into counts and keep windows covering focus; Update if doUpdate.
 */
void RefreshOption__11SpreadSheetFi(SpreadSheet* self, int doUpdate) {
    void** vtbl;
    void (*update)(SpreadSheet* self);

    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    GetIntArray__22GameVariableDispatcherFUiUiPii(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->optionId, &self->scrollX, 4);
    if (self->scrollY < 0 || self->unkE8 == 0) {
        self->scrollY = 0;
    } else if (self->scrollY >= self->unkE8) {
        self->scrollY = self->unkE8 - 1;
    }
    if (self->scrollY < self->scrollX) {
        self->scrollX = self->scrollY;
    } else if (self->scrollY >= self->scrollX + self->rows) {
        self->scrollX = self->scrollY - self->rows + 1;
    }
    if (self->unkDC < 0 || self->unkEC == 0) {
        self->unkDC = 0;
    } else if (self->unkDC >= self->unkEC) {
        self->unkDC = self->unkEC - 1;
    }
    if (self->unkDC < self->unkD4) {
        self->unkD4 = self->unkDC;
    } else if (self->unkDC >= self->unkD4 + self->cols) {
        self->unkD4 = self->unkDC - self->cols + 1;
    }
    if (self->unkEC <= self->cols) {
        self->unkD4 = 0;
    }
    if (self->unkE8 <= self->rows) {
        self->scrollX = 0;
    }
    if (self->cellArray != 0 && doUpdate != 0) {
        vtbl = (void**)self->vtbl;
        update = (void (*)(SpreadSheet*))vtbl[0x44 / 4];
        update(self);
    }
}

void RefreshOption__11SpreadSheetFv(SpreadSheet* self) {
    RefreshOption__11SpreadSheetFi(self, 1);
}

void ScrollRight__11SpreadSheetFi(SpreadSheet* self, int delta) {
    int rem;
    void (*update)(SpreadSheet* self);

    if (self->cellArray == 0) {
        return;
    }
    if (self->unkE8 < 1) {
        return;
    }
    if (delta >= self->unkE8) {
        delta = delta % self->unkE8;
    }
    self->scrollY += delta;
    rem = (self->scrollY - self->scrollX) % self->rows;
    if (self->scrollY < self->unkE8) {
        if (rem == 0) {
            self->scrollX += delta;
        }
    } else if (self->flagF0 != 0) {
        self->scrollX = 0;
        self->scrollY = 0;
    } else {
        self->scrollY = self->unkE8 - 1;
        delta = 0;
    }
    if (delta != 0) {
        SpreadSheetPlayNavSound(self);
    }
    update = (void (*)(SpreadSheet*))((void**)self->vtbl)[0x44 / 4];
    update(self);
}

void ScrollLeft__11SpreadSheetFi(SpreadSheet* self, int delta) {
    int rem;
    void (*update)(SpreadSheet* self);

    if (self->cellArray == 0) {
        return;
    }
    if (self->unkE8 < 1) {
        return;
    }
    if (delta >= self->unkE8) {
        delta = delta % self->unkE8;
    }
    self->scrollY -= delta;
    if (self->scrollY >= 0) {
        rem = (self->scrollY - self->scrollX) % self->rows;
        if (rem == -1) {
            self->scrollX -= delta;
        }
    } else if (self->flagF0 != 0) {
        self->scrollY = self->unkE8 - 1;
        self->scrollX = self->unkE8 - self->scrollY - 1;
    } else {
        self->scrollX = 0;
        self->scrollY = 0;
        delta = 0;
    }
    if (delta != 0) {
        SpreadSheetPlayNavSound(self);
    }
    update = (void (*)(SpreadSheet*))((void**)self->vtbl)[0x44 / 4];
    update(self);
}

void ScrollUp__11SpreadSheetFi(SpreadSheet* self, int delta) {
    int rem;
    void (*update)(SpreadSheet* self);

    if (self->cellArray == 0) {
        return;
    }
    if (self->unkEC < 1) {
        return;
    }
    if (delta >= self->unkEC) {
        delta = delta % self->unkEC;
    }
    self->unkDC -= delta;
    if (self->unkDC >= 0) {
        rem = (self->unkDC - self->unkD4) % self->cols;
        if (rem < 0) {
            self->unkD4 -= delta;
            if (self->unkD4 < 0) {
                self->unkD4 = 0;
            }
        }
    } else if (self->flagF0 != 0) {
        self->unkDC = self->unkEC - 1;
        self->unkD4 = self->unkEC - self->cols;
        if (self->unkD4 < 0) {
            self->unkD4 = 0;
        }
    } else {
        if (self->unkD4 != 0) {
            self->unkD4 -= delta;
            if (self->unkD4 < 0) {
                self->unkD4 = 0;
            }
        }
        self->unkDC = 0;
        delta = 0;
    }
    if (delta != 0) {
        SpreadSheetPlayNavSound(self);
    }
    update = (void (*)(SpreadSheet*))((void**)self->vtbl)[0x44 / 4];
    update(self);
}

void ScrollDown__11SpreadSheetFi(SpreadSheet* self, int delta) {
    int winMax;
    void (*update)(SpreadSheet* self);

    if (self->cellArray == 0) {
        return;
    }
    if (self->unkEC < 1) {
        return;
    }
    winMax = self->cols - 1;
    if (delta >= self->unkEC) {
        delta = delta % self->unkEC;
    }
    self->unkDC += delta;
    if (self->unkDC < self->unkEC) {
        if (self->unkDC - self->unkD4 >= self->cols) {
            self->unkD4 = self->unkDC - winMax;
        }
    } else if (self->flagF0 != 0) {
        self->unkD4 = 0;
        self->unkDC = 0;
    } else {
        self->unkDC = self->unkEC - 1;
        self->unkD4 = self->unkEC - self->cols;
        if (self->unkD4 < 0) {
            self->unkD4 = 0;
        }
        delta = 0;
    }
    if (delta != 0) {
        SpreadSheetPlayNavSound(self);
    }
    update = (void (*)(SpreadSheet*))((void**)self->vtbl)[0x44 / 4];
    update(self);
}

/* Persist scroll/focus ints after a Scroll* when optionId is bound. */
static void SpreadSheetSyncOptionArray(SpreadSheet* self) {
    if (self->optionId < 0) {
        return;
    }
    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    SetIntArray__22GameVariableDispatcherFUiUiPii(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->optionId, &self->scrollX, 4);
}

static void SpreadSheetFireSpEvent(SpreadSheet* self, int event) {
    FireEvent__12ScreenObjectFP9ScreenMgriiUi(self, screen_manager, event, 0, 0);
}

static void SpreadSheetSetRowStateFire(SpreadSheet* self, int row, int value) {
    SetRowState__22GameVariableDispatcherFUiUiii(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->collectionId, row, value);
    SpreadSheetFireSpEvent(self, 0x53500000);
}

static int SpreadSheetGetRowState(SpreadSheet* self, int row) {
    return GetRowState__22GameVariableDispatcherFUiUii(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->collectionId, row);
}

static void SpreadSheetSetColStateFire(SpreadSheet* self, int col, int value) {
    SetColState__22GameVariableDispatcherFUiUiii(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->collectionId, col, value);
    SpreadSheetFireSpEvent(self, 0x53500001);
}

static int SpreadSheetGetColState(SpreadSheet* self, int col) {
    return GetColState__22GameVariableDispatcherFUiUii(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->collectionId, col);
}

static void SpreadSheetCompareSubActions(SpreadSheet* self, const void* action, int lhs,
                                         int op, int rhs) {
    if (ScreenIntegerCompare__Fiii(lhs, op, rhs) != 0) {
        ProcessSubActions__12ScreenObjectFPC12ScreenActioni(self, action, 0);
    }
}

/*
 * ClearContents + AllocateCollection + clamp focus/window -- no Update.
 * Used by HandleEvent 0x405 (RefreshCollection without the cellArray Update).
 */
static void SpreadSheetRebuildClampNoUpdate(SpreadSheet* self) {
    void** vtbl;
    void (*clearContents)(SpreadSheet* self);
    void (*allocateCollection)(SpreadSheet* self);

    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    if (self->collectionId == -1) {
        return;
    }
    vtbl = (void**)self->vtbl;
    clearContents = (void (*)(SpreadSheet*))vtbl[0x58 / 4];
    clearContents(self);
    allocateCollection = (void (*)(SpreadSheet*))vtbl[0x50 / 4];
    allocateCollection(self);
    if (self->scrollY >= self->unkE8 && self->unkE8 > 0) {
        self->scrollY = self->unkE8 - 1;
        self->scrollX = self->scrollY - self->rows + 1;
        if (self->scrollX < 0) {
            self->scrollX = 0;
        }
    }
    if (self->unkDC >= self->unkEC && self->unkEC > 0) {
        self->unkDC = self->unkEC - 1;
        self->unkD4 = self->unkDC - self->cols + 1;
        if (self->unkD4 < 0) {
            self->unkD4 = 0;
        }
    }
}

/*
 * SpreadSheet::HandleEvent -- rebuild on 0x405 (empty dims), 0x407 (reset scroll),
 * 0x53500003 (refresh collection).
 * Soft ceiling: ~65.4% -- event cascade schedule; stop.
 */
void HandleEvent__11SpreadSheetFP9ScreenMgrii(SpreadSheet* self, void* /*mgr*/, int event,
                                              int /*arg*/) {
    if (event == 0x405) {
        if (self->unkE8 == 0 || self->unkEC == 0) {
            SpreadSheetRebuildClampNoUpdate(self);
            RefreshOption__11SpreadSheetFi(self, 1);
        }
    } else if (event == 0x407) {
        self->scrollX = 0;
        self->unkD4 = 0;
        self->scrollY = 0;
        self->unkDC = 0;
        SetIntArray__22GameVariableDispatcherFUiUiPii(
            m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
            (unsigned int)self->optionId, &self->scrollX, 4);
        RefreshCollection__11SpreadSheetFv(self);
        RefreshOption__11SpreadSheetFi(self, 1);
    } else if (event == 0x53500003) {
        RefreshCollection__11SpreadSheetFv(self);
        RefreshOption__11SpreadSheetFi(self, 1);
    }
}

/*
 * SpreadSheet::HandleAction -- scroll, row/col state, compares, show/hide (0x7db).
 * Unknown args fall through to ScreenControl::HandleAction. result init 1.
 * Soft ceiling: ~17.8% -- retail binary cmp tree vs switch; stop.
 */
int HandleAction__11SpreadSheetFP9ScreenMgrPC12ScreenAction(SpreadSheet* self, void* mgr,
                                                            const void* actionIn) {
    const ScreenActionView* action;
    void* params;
    int arg;
    int result;
    int p0;
    int p1;
    int p2;
    int idx;
    int state;
    int lhs;
    int vis;
    void* node;
    void (*show)(SpreadSheet* self);
    void (*hide)(SpreadSheet* self);

    action = (const ScreenActionView*)actionIn;
    arg = action->arg;
    params = action->params;
    result = 1;

    switch (arg) {
    case 0x7d2:
        ScrollDown__11SpreadSheetFi(self, 1);
        SpreadSheetSyncOptionArray(self);
        break;
    case 0x7d3:
        ScrollUp__11SpreadSheetFi(self, 1);
        SpreadSheetSyncOptionArray(self);
        break;
    case 0x7d4:
        ScrollLeft__11SpreadSheetFi(self, 1);
        SpreadSheetSyncOptionArray(self);
        break;
    case 0x7d5:
        ScrollRight__11SpreadSheetFi(self, 1);
        SpreadSheetSyncOptionArray(self);
        break;
    case 0x53500060:
        ScrollUp__11SpreadSheetFi(self, GetInt__12ScreenParamsFUi(params, 0));
        SpreadSheetSyncOptionArray(self);
        break;
    case 0x53500061:
        ScrollDown__11SpreadSheetFi(self, GetInt__12ScreenParamsFUi(params, 0));
        SpreadSheetSyncOptionArray(self);
        break;
    case 0x53500062:
        ScrollLeft__11SpreadSheetFi(self, GetInt__12ScreenParamsFUi(params, 0));
        SpreadSheetSyncOptionArray(self);
        break;
    case 0x53500063:
        ScrollRight__11SpreadSheetFi(self, GetInt__12ScreenParamsFUi(params, 0));
        SpreadSheetSyncOptionArray(self);
        break;

    case 0x53500000:
        /* Set row state at unkD4 + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetSetRowStateFire(self, self->unkD4 + p0, p1);
        break;
    case 0x53500001:
        /* Add p1 to row state at unkD4 + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        idx = self->unkD4 + p0;
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        state = SpreadSheetGetRowState(self, idx);
        SpreadSheetSetRowStateFire(self, idx, state + p1);
        break;
    case 0x53500002:
        /* Subtract p1 from row state at unkD4 + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        idx = self->unkD4 + p0;
        state = SpreadSheetGetRowState(self, idx);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetSetRowStateFire(self, idx, state - p1);
        break;
    case 0x53500003:
    case 0x53500005:
        /* Subtract p0 from row state at focus Y (unkDC). */
        idx = self->unkDC;
        state = SpreadSheetGetRowState(self, idx);
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        SpreadSheetSetRowStateFire(self, idx, state - p0);
        break;
    case 0x53500004:
        /* Add p0 to row state at focus Y (unkDC). */
        idx = self->unkDC;
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        state = SpreadSheetGetRowState(self, idx);
        SpreadSheetSetRowStateFire(self, idx, state + p0);
        break;

    case 0x53500006:
        /* Compare window origin Y (unkD4). */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, self->unkD4, p0, p1);
        break;
    case 0x53500007:
        /* Compare row state at unkD4 + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        state = SpreadSheetGetRowState(self, self->unkD4 + p0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        p2 = GetInt__12ScreenParamsFUi(params, 2);
        SpreadSheetCompareSubActions(self, actionIn, state, p1, p2);
        break;
    case 0x53500008:
        /* Compare visible col span (min(unkEC - unkD4, cols)). */
        vis = self->unkEC - self->unkD4;
        if (self->cols < vis) {
            vis = self->cols;
        }
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, vis, p0, p1);
        break;
    case 0x53500009:
        /* Compare focus offset within window (unkDC - unkD4). */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, self->unkDC - self->unkD4, p0,
                                     p1);
        break;
    case 0x5350000A:
        /* Compare focus Y (unkDC). */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, self->unkDC, p0, p1);
        break;
    case 0x5350000B:
        /* Compare row state at focus Y. */
        state = SpreadSheetGetRowState(self, self->unkDC);
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, state, p0, p1);
        break;
    case 0x5350000C:
        /* Start/mid/end class of focus Y vs unkEC. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        if (self->unkEC > 0) {
            if (self->unkEC == 1) {
                /* Retail leaves GetInt(1) in r3 as compare lhs. */
                lhs = p1;
            } else if (self->unkDC == 0) {
                lhs = 0;
            } else if (self->unkDC == self->unkEC - 1) {
                lhs = 2;
            } else {
                lhs = 1;
            }
            SpreadSheetCompareSubActions(self, actionIn, lhs, p0, p1);
        }
        break;

    case 0x53500030:
        /* Set col state at scrollX + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetSetColStateFire(self, self->scrollX + p0, p1);
        break;
    case 0x53500031:
        /* Add p1 to col state at scrollX + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        idx = self->scrollX + p0;
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        state = SpreadSheetGetColState(self, idx);
        SpreadSheetSetColStateFire(self, idx, state + p1);
        break;
    case 0x53500032:
        /* Subtract p1 from col state at scrollX + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        idx = self->scrollX + p0;
        state = SpreadSheetGetColState(self, idx);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetSetColStateFire(self, idx, state - p1);
        break;
    case 0x53500033:
        /* Set col state at focus X (scrollY). */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        SpreadSheetSetColStateFire(self, self->scrollY, p0);
        break;
    case 0x53500034:
        /* Add p0 to col state at focus X. */
        idx = self->scrollY;
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        state = SpreadSheetGetColState(self, idx);
        SpreadSheetSetColStateFire(self, idx, state + p0);
        break;
    case 0x53500035:
        /* Subtract p0 from col state at focus X. */
        idx = self->scrollY;
        state = SpreadSheetGetColState(self, idx);
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        SpreadSheetSetColStateFire(self, idx, state - p0);
        break;

    case 0x53500036:
        /* Compare window origin X (scrollX). */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, self->scrollX, p0, p1);
        break;
    case 0x53500037:
        /* Compare col state at scrollX + p0. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        state = SpreadSheetGetColState(self, self->scrollX + p0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        p2 = GetInt__12ScreenParamsFUi(params, 2);
        SpreadSheetCompareSubActions(self, actionIn, state, p1, p2);
        break;
    case 0x53500038:
        /* Compare visible row span (min(unkE8 - scrollX, rows)). */
        vis = self->unkE8 - self->scrollX;
        if (self->rows < vis) {
            vis = self->rows;
        }
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, vis, p0, p1);
        break;
    case 0x53500039:
        /* Compare focus offset within window (scrollY - scrollX). */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, self->scrollY - self->scrollX, p0,
                                     p1);
        break;
    case 0x5350003A:
        /*
         * Compare focus X; also run sub-actions when unkE8==1 and rhs!=1 even if
         * the compare fails (retail CR quirk path).
         */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        if (ScreenIntegerCompare__Fiii(self->scrollY, p0, p1) != 0 ||
            (self->unkE8 == 1 && p1 != 1)) {
            ProcessSubActions__12ScreenObjectFPC12ScreenActioni(self, actionIn, 0);
        }
        break;
    case 0x5350003B:
        /* Compare col state at focus X. */
        state = SpreadSheetGetColState(self, self->scrollY);
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        SpreadSheetCompareSubActions(self, actionIn, state, p0, p1);
        break;
    case 0x5350003C:
        /* Start/mid/end class of focus X vs unkE8. */
        p0 = GetInt__12ScreenParamsFUi(params, 0);
        p1 = GetInt__12ScreenParamsFUi(params, 1);
        if (self->scrollY == 0) {
            lhs = 0;
        } else if (self->scrollY == self->unkE8 - 1) {
            lhs = 2;
        } else {
            lhs = 1;
        }
        SpreadSheetCompareSubActions(self, actionIn, lhs, p0, p1);
        break;

    case 0x7db:
        /* Show/hide only when params node 0 is this object -- no peer delegate. */
        node = GetScreenNode__12ScreenParamsFUi(params, 0);
        if (node == (void*)self) {
            p1 = GetInt__12ScreenParamsFUi(params, 1);
            if (p1 == 0) {
                show = (void (*)(SpreadSheet*))((void**)self->vtbl)[0x48 / 4];
                show(self);
            } else if (p1 == 1) {
                hide = (void (*)(SpreadSheet*))((void**)self->vtbl)[0x4c / 4];
                hide(self);
            }
        }
        break;

    default:
        result = HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(self, mgr,
                                                                           actionIn);
        break;
    }
    return result;
}

void Init__8TextListFv(TextList* self) {
    Init__13ScreenControlFv(self);
}

/*
 * ClearStrings -- reset each visible ScreenText string color via pfxfont.
 * Soft ceiling: ~95.8% -- live-obj / color schedule; stop.
 */
#pragma dont_inline on
void ClearStrings__8TextListFv(TextList* self) {
    int i;
    int off;
    ScreenText* text;
    StringObj* live;
    unsigned char color[4];

    i = 0;
    off = 0;
    while (i < self->itemCount) {
        text = (ScreenText*)(*(void**)((char*)self->itemNodes + off));
        if (text != 0) {
            live = text->stringObj;
            if (live != 0) {
                if (live->instance != (unsigned int)text->stringObjInstance) {
                    live = 0;
                }
            } else {
                live = 0;
            }
            if (live != 0) {
                color[0] = live->pfx.instance0.rgba[0];
                color[1] = live->pfx.instance0.rgba[1];
                color[2] = live->pfx.instance0.rgba[2];
                color[3] = live->pfx.instance0.rgba[3];
                update_string_obj_pfx(live, text->font, 0);
                pfxfont_set_string_color(&live->pfx, (unsigned int*)color);
            }
        }
        i += 1;
        off += 4;
    }
}
#pragma dont_inline off

/*
 * TextList::RefreshCollection(flag) -- Free+Get string collection, clamp
 * focusMax, optionally Update (vtbl+0x44) when unkD4 and flag==1.
 * Soft ceiling: ~84.3% -- Free/Get / window scan schedule; stop.
 */
void RefreshCollection__8TextListFi(TextList* self, int doUpdate) {
    char** oldStrings;
    unsigned int oldCount;
    unsigned int count;
    int i;
    int idx;
    int found;
    void** vtbl;
    void (*update)(TextList* self);

    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    if (self->collectionId == -1) {
        return;
    }
    ClearStrings__8TextListFv(self);
    oldStrings = self->strings;
    oldCount = (unsigned int)self->stringCount;
    FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
        m_pGameVariables__13ScreenControl, (unsigned int)self->gvContext,
        (unsigned int)self->collectionId, oldStrings, oldCount);
    self->strings = 0;
    count = GetStringCollection__22GameVariableDispatcherFUiUiPPPc(
        m_pGameVariables__13ScreenControl, (unsigned int)self->gvContext,
        (unsigned int)self->collectionId, &self->strings);
    self->stringCount = (int)(count & 0xffff);
    if (self->stringCount < 0) {
        /* fall through to Update check */
    } else {
        if (self->focusMax < 0) {
            self->focusMax = 0;
        }
        if (self->focusMax >= self->stringCount) {
            self->focusMax = self->stringCount - 1;
        }
        if (self->stringCount != 0) {
            found = 0;
            i = 0;
            while (i < self->itemCount) {
                idx = self->focusIndex + i;
                idx = idx - (idx / self->stringCount) * self->stringCount;
                if (idx == self->focusMax) {
                    found = 1;
                    break;
                }
                i += 1;
            }
            if (found == 0) {
                self->focusIndex = self->focusMax;
            }
        }
    }
    if (self->unkD4 != 0 && doUpdate == 1) {
        vtbl = (void**)self->vtbl;
        update = (void (*)(TextList*))vtbl[0x44 / 4];
        update(self);
    }
}

void RefreshCollection__8TextListFv(TextList* self) {
    RefreshCollection__8TextListFi(self, 1);
}

/*
 * TextList::ProcessParams -- LIST control for mem-screen / menu option rows.
 * Malloc names: SS-5/6/7TextList (@stringBase0+0x2E0/0x2ED/0x2FA).
 * Soft ceiling: ~90.3% -- GV malloc/sprintf schedule; stop.
 */
void ProcessParams__8TextListFP12ScreenParams(TextList* self, void* params) {
    int nodeIndex;
    int i;
    int off;
    void* node;
    ScreenText* text;
    StringObj* live;
    char* buf;
    char scratch[10];
    char* p;
    int n;

    nodeIndex = 6;
    self->itemCount = GetInt__12ScreenParamsFUi(params, 0);
    self->collectionId = GetResourceID__12ScreenParamsFUi(params, 1);
    self->optionId = GetResourceID__12ScreenParamsFUi(params, 2);
    self->wrap = GetBoolean__12ScreenParamsFUi(params, 3);
    self->hasLinkedNode = GetBoolean__12ScreenParamsFUi(params, 5);
    self->hasParamB0 = GetBoolean__12ScreenParamsFUi(params, 4);
    if (self->hasParamB0 != 0) {
        self->paramB0 = GetInt__12ScreenParamsFUi(params, 6);
    }
    if (self->hasLinkedNode != 0) {
        self->linkedNode = GetScreenNode__12ScreenParamsFUi(params, 6);
        nodeIndex = 7;
    }
    self->itemNodes = (ScreenNode**)Malloc__10ScreenUtilFUliPc(
        (unsigned long)(self->itemCount << 2), kMallocTagInit,
        (char*)(stringBase0 + 0x2e0));
    off = 0;
    for (i = 0; i < self->itemCount; i++) {
        node = GetScreenNode__12ScreenParamsFUi(params, (unsigned int)nodeIndex);
        nodeIndex += 1;
        *(void**)((char*)self->itemNodes + off) = node;
        off += 4;
    }
    if (self->itemCount > 0) {
        text = (ScreenText*)self->itemNodes[self->focusMax - self->focusIndex];
        live = text->stringObj;
        if (live != 0) {
            if (live->instance != (unsigned int)text->stringObjInstance) {
                live = 0;
            }
        } else {
            live = 0;
        }
        if (live == 0) {
            self->color[0] = text->seData->color[0];
            self->color[1] = text->seData->color[1];
            self->color[2] = text->seData->color[2];
            self->color[3] = text->seData->color[3];
        } else {
            /* StringObj+0xB4 == pfx.instance0.rgba (PfxFontInstance+0x18). */
            self->color[0] = live->pfx.instance0.rgba[0];
            self->color[1] = live->pfx.instance0.rgba[1];
            self->color[2] = live->pfx.instance0.rgba[2];
            self->color[3] = live->pfx.instance0.rgba[3];
        }
    }
    if (m_pGameVariables__13ScreenControl != 0) {
        self->unkD4 = 1;
        if (self->collectionId == -1 && self->optionId != -1) {
            self->strings = (char**)Malloc__10ScreenUtilFUliPc(
                4, kMallocTagInit, (char*)(stringBase0 + 0x2ed));
            buf = (char*)Malloc__10ScreenUtilFUliPc(
                0xb, kMallocTagInit, (char*)(stringBase0 + 0x2fa));
            *self->strings = buf;
            p = scratch;
            n = 0xa;
            do {
                *p = 0x20;
                p += 1;
                n -= 1;
            } while (n != 0);
            sprintf(scratch, stringBase0 + 0x1e5, self->focusMax);
            p = scratch;
            n = 0xa;
            do {
                *buf = *p;
                p += 1;
                buf += 1;
                n -= 1;
            } while (n != 0);
            /* Retail writes then falls into the common zeroing below. */
            self->stringCount = 1;
            self->focusMax = 0;
        }
    }
    self->stringCount = 0;
    self->focusMax = 0;
    self->focusIndex = 0;
}

/*
 * TextList::RefreshOption -- GetInt option cursor, advance until IsValidInt,
 * optional "%d" single-string mode, clamp window, then Update (vtbl+0x44).
 */
void RefreshOption__8TextListFv(TextList* self) {
    int atEnd;
    int found;
    int i;
    int idx;
    char* dst;
    char scratch[10];
    char* p;
    int n;

    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    self->focusMax = GetInt__22GameVariableDispatcherFUiUi(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->optionId);
    for (;;) {
        if (IsValidInt__22GameVariableDispatcherFUiUiUiUii(
                m_pGameVariables__13ScreenControl, (unsigned int)self->gvContext,
                (unsigned int)self->collectionId, (unsigned int)self->pad9C,
                (unsigned int)self->optionId, self->focusMax) != 0) {
            break;
        }
        atEnd = 1;
        self->focusMax = self->focusMax + 1;
        self->focusMax =
            self->focusMax -
            (self->focusMax / self->stringCount) * self->stringCount;
        if (self->focusMax != self->stringCount - 1) {
            atEnd = 0;
        }
        if (self->wrap == 0 && atEnd != 0) {
            break;
        }
    }
    if (self->collectionId == -1 && self->optionId != -1) {
        if (self->strings != 0) {
            dst = self->strings[0];
            p = scratch;
            n = 0xa;
            do {
                *p = 0x20;
                p += 1;
                n -= 1;
            } while (n != 0);
            sprintf(scratch, stringBase0 + 0x1e5, self->focusMax);
            p = scratch;
            n = 0xa;
            do {
                *dst = *p;
                p += 1;
                dst += 1;
                n -= 1;
            } while (n != 0);
        }
        self->focusMax = 0;
        self->stringCount = 1;
    }
    if (self->stringCount < 0) {
        /* skip clamp */
    } else {
        if (self->focusMax < 0) {
            self->focusMax = 0;
        }
        if (self->focusMax >= self->stringCount) {
            self->focusMax = self->stringCount - 1;
        }
        if (self->stringCount != 0) {
            found = 0;
            i = 0;
            while (i < self->itemCount) {
                idx = self->focusIndex + i;
                idx = idx - (idx / self->stringCount) * self->stringCount;
                if (idx == self->focusMax) {
                    found = 1;
                    break;
                }
                i += 1;
            }
            if (found == 0) {
                self->focusIndex = self->focusMax;
            }
        }
    }
    if (self->unkD4 != 0) {
        (*(void (**)(TextList*))(*(unsigned char**)self + 0x44))(self);
    }
}

/*
 * TextList::Update -- refresh option strings onto visible ScreenText rows;
 * move linked ScreenPoly highlight to the focused row when present.
 * Soft ceiling: linked-poly pos (retail stfs via rA=0 / absolute 0).
 */
void Update__8TextListFv(TextList* self) {
    int atEnd;
    int found;
    int i;
    int off;
    int idx;
    int strIdx;
    char* dst;
    char scratch[10];
    char* p;
    int n;
    ScreenText* text;
    StringObj* live;
    ScreenPoly* link;
    int v;
    int map;
    unsigned char color[4];
    void (*dispose)(void* node);
    void (*init)(void* node);
    void (*setVisible)(void* node, unsigned int visible);
    char* str;

    if (self->stringCount < 1) {
        return;
    }
    if (self->strings == 0 &&
        !(self->collectionId == -1 && self->optionId == -1)) {
        RefreshCollection__8TextListFi(self, 0);
        if (m_pGameVariables__13ScreenControl != 0) {
            self->focusMax = GetInt__22GameVariableDispatcherFUiUi(
                m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
                (unsigned int)self->optionId);
            for (;;) {
                if (IsValidInt__22GameVariableDispatcherFUiUiUiUii(
                        m_pGameVariables__13ScreenControl,
                        (unsigned int)self->gvContext,
                        (unsigned int)self->collectionId,
                        (unsigned int)self->pad9C,
                        (unsigned int)self->optionId, self->focusMax) != 0) {
                    break;
                }
                atEnd = 1;
                self->focusMax = self->focusMax + 1;
                self->focusMax =
                    self->focusMax -
                    (self->focusMax / self->stringCount) * self->stringCount;
                if (self->focusMax != self->stringCount - 1) {
                    atEnd = 0;
                }
                if (self->wrap == 0 && atEnd != 0) {
                    break;
                }
            }
            if (self->collectionId == -1 && self->optionId != -1) {
                if (self->strings != 0) {
                    dst = self->strings[0];
                    p = scratch;
                    n = 0xa;
                    do {
                        *p = 0x20;
                        p += 1;
                        n -= 1;
                    } while (n != 0);
                    sprintf(scratch, stringBase0 + 0x1e5, self->focusMax);
                    p = scratch;
                    n = 0xa;
                    do {
                        *dst = *p;
                        p += 1;
                        dst += 1;
                        n -= 1;
                    } while (n != 0);
                }
                self->focusMax = 0;
                self->stringCount = 1;
            }
            if (self->stringCount >= 0) {
                if (self->focusMax < 0) {
                    self->focusMax = 0;
                }
                if (self->focusMax >= self->stringCount) {
                    self->focusMax = self->stringCount - 1;
                }
                if (self->stringCount != 0) {
                    found = 0;
                    i = 0;
                    while (i < self->itemCount) {
                        idx = self->focusIndex + i;
                        idx = idx -
                              (idx / self->stringCount) * self->stringCount;
                        if (idx == self->focusMax) {
                            found = 1;
                            break;
                        }
                        i += 1;
                    }
                    if (found == 0) {
                        self->focusIndex = self->focusMax;
                    }
                }
            }
        }
    }

    i = 0;
    off = 0;
    while (i < self->itemCount) {
        text = (ScreenText*)(*(void**)((char*)self->itemNodes + off));
        if (i >= self->stringCount || self->strings == 0) {
            setVisible =
                *(void (**)(void*, unsigned int))(*(unsigned char**)text +
                                                  0x1c);
            setVisible(text, 0);
        } else {
            strIdx = self->focusIndex + i;
            strIdx =
                strIdx - (strIdx / self->stringCount) * self->stringCount;
            if (self->linkedNode != 0 && strIdx == self->focusMax) {
                /*
                 * Retail stores the source pos via stfs 0(r0) (rA=0 => EA=0) --
                 * MWCC leftover. Intended: stack floats, copy onto linked
                 * ScreenPoly verts via vert_map (Y flipped vs 480).
                 */
                float x;
                float y;
                float z;

                live = text->stringObj;
                if (live != 0) {
                    if (live->instance !=
                        (unsigned int)text->stringObjInstance) {
                        live = 0;
                    }
                } else {
                    live = 0;
                }
                if (live == 0) {
                    x = text->seData->posX;
                    y = text->seData->posY;
                    z = 0.0f;
                } else {
                    x = (float)live->x;
                    y = (float)(480 - live->y);
                    z = 0.0f;
                }
                (void)z;
                link = (ScreenPoly*)self->linkedNode;
                for (v = 0; v < 4; v++) {
                    map = vert_map__10ScreenPoly[v];
                    link->verts[map].x = x;
                    link->verts[map].y = 480.0f - y;
                }
            }
            dispose =
                *(void (**)(void*))(*(unsigned char**)text + 0x10);
            dispose(text);
            str = self->strings[strIdx];
            live = text->stringObj;
            if (live != 0) {
                if (live->instance !=
                    (unsigned int)text->stringObjInstance) {
                    live = 0;
                }
            } else {
                live = 0;
            }
            if (live != 0) {
                color[0] = live->pfx.instance0.rgba[0];
                color[1] = live->pfx.instance0.rgba[1];
                color[2] = live->pfx.instance0.rgba[2];
                color[3] = live->pfx.instance0.rgba[3];
                update_string_obj_pfx(live, text->font, str);
                pfxfont_set_string_color(&live->pfx, (unsigned int*)color);
            }
            init = *(void (**)(void*))(*(unsigned char**)text + 0x0c);
            init(text);
            setVisible =
                *(void (**)(void*, unsigned int))(*(unsigned char**)text +
                                                  0x1c);
            setVisible(text, 1);
        }
        i += 1;
        off += 4;
    }
}

void ScrollDec__8TextListFi(TextList* self, int delta) {
    if (self->unkD4 != 0) {
        (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x50))(
            self, self->itemCount - delta);
    }
}

void ScrollInc__8TextListFi(TextList* self, int delta) {
    if (self->unkD4 != 0) {
        (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x50))(
            self, self->itemCount + delta);
    }
}

/*
 * TextList::Move -- nudge focusMax by delta, wrap/IsValidInt scan, sync GV + Update.
 */
void Move__8TextListFi(TextList* self, int delta) {
    int oldFocus;
    int hitBoundary;
    int tmp;
    int ok;
    int atBound;
    int winStart;
    int winEnd;

    if (self->unkD4 == 0) {
        return;
    }
    oldFocus = self->focusMax;
    hitBoundary = 1;
    tmp = 0;
    if (oldFocus != self->stringCount - 1 || delta <= 0) {
        tmp = 0;
        if (oldFocus == 0 && delta < 0) {
            tmp = 1;
        }
        if (tmp == 0) {
            hitBoundary = 0;
        }
    }
    if (self->wrap == 0 && hitBoundary != 0) {
        return;
    }
    self->focusMax = oldFocus + delta;
    if (self->collectionId == -1 && self->optionId != -1) {
        /* Numeric single-string mode -- skip wrap/valid/window. */
    } else if (self->stringCount <= 0) {
        /* fall through to SetInt */
    } else {
        self->focusMax = self->focusMax -
                         (self->focusMax / self->stringCount) * self->stringCount;
        if (self->focusMax < 0) {
            self->focusMax = self->focusMax + self->stringCount;
        }
        ok = 0;
        for (;;) {
            if (IsValidInt__22GameVariableDispatcherFUiUiUiUii(
                    m_pGameVariables__13ScreenControl,
                    (unsigned int)self->gvContext,
                    (unsigned int)self->collectionId,
                    (unsigned int)self->pad9C,
                    (unsigned int)self->optionId, self->focusMax) != 0) {
                ok = 1;
                break;
            }
            if (delta < 0) {
                self->focusMax = self->focusMax - 1;
                self->focusMax =
                    self->focusMax -
                    (self->focusMax / self->stringCount) * self->stringCount;
                if (self->focusMax < 0) {
                    self->focusMax = self->focusMax + self->stringCount;
                }
            } else {
                self->focusMax = self->focusMax + 1;
                self->focusMax =
                    self->focusMax -
                    (self->focusMax / self->stringCount) * self->stringCount;
            }
            atBound = 0;
            if (self->focusMax == self->stringCount - 1 && delta > 0) {
                atBound = 1;
            } else if (self->focusMax == 0 && delta < 0) {
                atBound = 1;
            }
            if (self->wrap == 0 && atBound != 0) {
                ok = 0;
                break;
            }
        }
        if (ok == 0) {
            self->focusMax = oldFocus;
            return;
        }
        if (self->stringCount < self->itemCount) {
            self->focusIndex = 0;
        } else {
            winStart = self->focusIndex -
                       (self->focusIndex / self->stringCount) * self->stringCount;
            if (oldFocus == winStart && delta < 0) {
                self->focusIndex = self->focusMax;
            } else {
                winEnd = self->focusIndex + self->itemCount - 1;
                winEnd = winEnd - (winEnd / self->stringCount) * self->stringCount;
                if (oldFocus == winEnd && delta >= 0) {
                    self->focusIndex = self->focusIndex + delta;
                    self->focusIndex =
                        self->focusIndex -
                        (self->focusIndex / self->stringCount) * self->stringCount;
                    if (self->focusIndex < 0) {
                        self->focusIndex = self->focusIndex + self->stringCount;
                    }
                    for (;;) {
                        if (IsValidInt__22GameVariableDispatcherFUiUiUiUii(
                                m_pGameVariables__13ScreenControl,
                                (unsigned int)self->gvContext,
                                (unsigned int)self->collectionId,
                                (unsigned int)self->pad9C,
                                (unsigned int)self->optionId,
                                self->focusIndex) != 0) {
                            break;
                        }
                        if (delta < 0) {
                            self->focusIndex = self->focusIndex - 1;
                            self->focusIndex =
                                self->focusIndex -
                                (self->focusIndex / self->stringCount) *
                                    self->stringCount;
                            if (self->focusIndex < 0) {
                                self->focusIndex =
                                    self->focusIndex + self->stringCount;
                            }
                        } else {
                            self->focusIndex = self->focusIndex + 1;
                            self->focusIndex =
                                self->focusIndex -
                                (self->focusIndex / self->stringCount) *
                                    self->stringCount;
                        }
                        atBound = 0;
                        if (self->focusIndex == self->stringCount - 1 &&
                            delta > 0) {
                            atBound = 1;
                        } else if (self->focusIndex == 0 && delta < 0) {
                            atBound = 1;
                        }
                        if (self->wrap == 0 && atBound != 0) {
                            break;
                        }
                    }
                }
            }
        }
    }
    if (self->optionId >= 0 && m_pGameVariables__13ScreenControl != 0) {
        SetInt__22GameVariableDispatcherFUiUii(
            m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
            (unsigned int)self->optionId, self->focusMax);
    }
    if (self->collectionId == -1) {
        self->focusMax = GetInt__22GameVariableDispatcherFUiUi(
            m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
            (unsigned int)self->optionId);
    }
    (*(void (**)(TextList*))(*(unsigned char**)self + 0x44))(self);
}

/*
 * TextList::HandleEvent -- 0x407 rebuild strings; 0x3EB teardown.
 */
void HandleEvent__8TextListFP9ScreenMgrii(TextList* self, void* /*mgr*/, int event,
                                          int /*arg*/) {
    char** table;
    char* buf;

    if (event == 0x407) {
        if (self->collectionId == -1 && self->optionId != -1 &&
            self->strings == 0) {
            table = (char**)Malloc__10ScreenUtilFUliPc(
                4, kMallocTagInit, (char*)(stringBase0 + 0x2c6));
            self->strings = table;
            buf = (char*)Malloc__10ScreenUtilFUliPc(
                0xb, kMallocTagInit, (char*)(stringBase0 + 0x2d3));
            self->strings[0] = buf;
        }
        RefreshCollection__8TextListFi(self, 0);
        (*(void (**)(TextList*))(*(unsigned char**)self + 0x4c))(self);
    } else if (event >= 0x407) {
        /* no-op */
    } else if (event == 0x3eb) {
        if (self->collectionId == -1 && self->optionId != -1) {
            if (self->strings != 0) {
                Free__10ScreenUtilFPv(self->strings[0]);
                self->strings[0] = 0;
                Free__10ScreenUtilFPv(self->strings);
                self->strings = 0;
                ClearStrings__8TextListFv(self);
            }
        } else if (m_pGameVariables__13ScreenControl != 0 &&
                   self->strings != 0) {
            FreeStringCollection__22GameVariableDispatcherFUiUiPPcUi(
                m_pGameVariables__13ScreenControl,
                (unsigned int)self->gvContext,
                (unsigned int)self->collectionId, self->strings,
                (unsigned int)self->stringCount);
            self->strings = 0;
        }
    }
}

/*
 * TextList::HandleAction -- nav (Move/Scroll), refresh, compare, confirm SetInt.
 */
int HandleAction__8TextListFP9ScreenMgrPC12ScreenAction(TextList* self, void* mgr,
                                                        const void* actionIn) {
    const ScreenActionView* action;
    void* params;
    int arg;
    int result;
    int a;
    int b;
    int cls;
    int i;
    int idx;
    void* node;
    int (*nodeHandle)(void* node, void* mgr, const void* action);

    action = (const ScreenActionView*)actionIn;
    arg = action->arg;
    params = action->params;
    result = 1;

    if (arg == 0x7e2) {
        node = GetScreenNode__12ScreenParamsFUi(params, 0);
        if (node == (void*)self) {
            if (self->optionId >= 0 && m_pGameVariables__13ScreenControl != 0) {
                SetInt__22GameVariableDispatcherFUiUii(
                    m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
                    (unsigned int)self->optionId, self->focusMax);
            }
        } else if (node != 0) {
            nodeHandle =
                *(int (**)(void*, void*, const void*))(*(unsigned char**)node +
                                                       0x28);
            nodeHandle(node, mgr, actionIn);
        }
    } else if (arg >= 0x7e2) {
        /* High compare / Move aliases (0x100001..). */
        if (arg == 0x100003) {
            a = GetInt__12ScreenParamsFUi(params, 0);
            b = GetInt__12ScreenParamsFUi(params, 1);
            if (self->stringCount > 0) {
                if (ScreenIntegerCompare__Fiii(self->focusMax, a, b) != 0) {
                    ProcessSubActions__12ScreenObjectFPC12ScreenActioni(
                        self, actionIn, 0);
                }
            }
        } else if (arg >= 0x100003) {
            if (arg == 0x100005) {
                a = GetInt__12ScreenParamsFUi(params, 0);
                b = GetInt__12ScreenParamsFUi(params, 1);
                if (self->stringCount > 0) {
                    i = 0;
                    while (i < self->itemCount) {
                        idx = self->focusIndex + i;
                        idx = idx - (idx / self->stringCount) * self->stringCount;
                        if (idx == self->focusMax) {
                            if (ScreenIntegerCompare__Fiii(i, a, b) != 0) {
                                ProcessSubActions__12ScreenObjectFPC12ScreenActioni(
                                    self, actionIn, 0);
                            }
                        }
                        i += 1;
                    }
                }
            } else if (arg >= 0x100005) {
                result =
                    HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(
                        self, mgr, actionIn);
            } else {
                /* 0x100004 -- start/mid/end class of focusMax. */
                a = GetInt__12ScreenParamsFUi(params, 0);
                b = GetInt__12ScreenParamsFUi(params, 1);
                if (self->stringCount > 0) {
                    if (self->stringCount == 1) {
                        /* Retail leaves r3 = GetInt(1) as compare lhs. */
                        cls = b;
                    } else if (self->focusMax == 0) {
                        cls = 0;
                    } else if (self->focusMax == self->stringCount - 1) {
                        cls = 2;
                    } else {
                        cls = 1;
                    }
                    if (ScreenIntegerCompare__Fiii(cls, a, b) != 0) {
                        ProcessSubActions__12ScreenObjectFPC12ScreenActioni(
                            self, actionIn, 0);
                    }
                }
            }
        } else if (arg == 0x100001) {
            (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x50))(self, 1);
        } else if (arg >= 0x100001) {
            (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x50))(self,
                                                                          -1);
        } else {
            result = HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(
                self, mgr, actionIn);
        }
    } else if (arg == 0x7d3) {
        (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x58))(self, 1);
    } else if (arg >= 0x7d3) {
        if (arg == 0x7db) {
            node = GetScreenNode__12ScreenParamsFUi(params, 0);
            if (node == (void*)self) {
                a = GetInt__12ScreenParamsFUi(params, 1);
                if (a == 0) {
                    (*(void (**)(TextList*))(*(unsigned char**)self + 0x48))(
                        self);
                } else if (a == 1) {
                    (*(void (**)(TextList*))(*(unsigned char**)self + 0x4c))(
                        self);
                }
            } else if (node != 0) {
                nodeHandle =
                    *(int (**)(void*, void*, const void*))(*(unsigned char**)node +
                                                           0x28);
                nodeHandle(node, mgr, actionIn);
            }
        } else {
            result = HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(
                self, mgr, actionIn);
        }
    } else if (arg == 0x7d1) {
        (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x50))(self, -1);
    } else if (arg >= 0x7d1) {
        (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x54))(self, 1);
    } else if (arg >= 0x7d0) {
        (*(void (**)(TextList*, int))(*(unsigned char**)self + 0x50))(self, 1);
    } else {
        result = HandleAction__13ScreenControlFP9ScreenMgrPC12ScreenAction(
            self, mgr, actionIn);
    }
    return result;
}

/*
 * ImageList::Update -- bind GameVariables texture strip onto POLY item nodes.
 * Mem-screen icon lists (PPWLS / view profile) tick this after RefreshCollection.
 * Soft ceiling: ~72.6% -- tex/filter/live-obj schedule; stop.
 */
void Update__9ImageListFv(ImageList* self) {
    int i;
    int off;
    ImageListTexCollection* col;
    int count;
    int idx;
    int wrapped;
    ScreenPoly* poly;
    ScreenPoly* link;
    RwTexture* color;
    RwTexture* alpha;
    ScreenObj* obj;
    RwTextureFilterView* view;
    void (*setVisible)(void* node, int visible);
    ScreenPolyVert* vert;

    i = 0;
    off = 0;
    col = self->textureInfo.data;
    while (i < self->itemCount) {
        poly = (ScreenPoly*)(*(void**)((char*)self->itemNodes + off));
        setVisible = *(void (**)(void*, int))(*(unsigned char**)poly + 0x1c);
        setVisible(poly, 0);
        i += 1;
        off += 4;
    }
    if (col == 0) {
        return;
    }
    if (self->textureInfo.ready == 0) {
        return;
    }
    count = col->count;
    if (count < 1) {
        return;
    }
    if (col->colors == 0) {
        return;
    }
    i = 0;
    off = 0;
    while (i < self->itemCount) {
        idx = self->scrollBase + i;
        if (idx >= count && self->wrap == 0) {
            poly = (ScreenPoly*)(*(void**)((char*)self->itemNodes + off));
            setVisible = *(void (**)(void*, int))(*(unsigned char**)poly + 0x1c);
            setVisible(poly, 0);
            i += 1;
            off += 4;
            continue;
        }
        wrapped = idx - (idx / count) * count;
        poly = (ScreenPoly*)(*(void**)((char*)self->itemNodes + off));
        color = (RwTexture*)col->colors[wrapped];
        poly->colorTex = color;
        if (color != 0) {
            view = (RwTextureFilterView*)color;
            if (((ScreenPolyFilterBits*)&poly->filterFlags)->linear) {
                view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
            } else {
                view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
            }
            view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
        }
        obj = poly->screenObj;
        if (obj != 0) {
            if (obj->instance != (unsigned int)poly->screenObjInstance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }
        if (obj != 0) {
            if (color != 0) {
                obj->texture = ((RwTextureFilterView*)color)->raster;
            } else {
                obj->texture = 0;
            }
            obj->pfx2d->texture = color;
        }
        alpha = (RwTexture*)col->alphas[wrapped];
        poly->alphaTex = alpha;
        if (alpha != 0) {
            view = (RwTextureFilterView*)alpha;
            if (((ScreenPolyFilterBits*)&poly->filterFlags)->linear) {
                view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x1100;
            } else {
                view->filterFlags = (view->filterFlags & 0xffff00ff) | 0x3300;
            }
            view->filterFlags = (view->filterFlags & 0xffffff00) | 1;
        }
        obj = poly->screenObj;
        if (obj != 0) {
            if (obj->instance != (unsigned int)poly->screenObjInstance) {
                obj = 0;
            }
        } else {
            obj = 0;
        }
        if (obj != 0) {
            obj->pfx2d->alpha_texture = alpha;
        }
        setVisible = *(void (**)(void*, int))(*(unsigned char**)poly + 0x1c);
        if (col->colors[wrapped] != 0) {
            setVisible(poly, 1);
        } else {
            setVisible(poly, 0);
        }
        i += 1;
        off += 4;
    }
    link = (ScreenPoly*)self->linkNode;
    if (link == 0) {
        return;
    }
    idx = self->focusIndex - self->scrollBase;
    if (idx < 0) {
        idx += count;
    }
    poly = (ScreenPoly*)self->itemNodes[idx];
    vert = &poly->verts[vert_map__10ScreenPoly[0]];
    link->offsetX = vert->x - self->layoutX;
    link->offsetY = (480.0f - vert->y) - self->layoutY;
    setVisible = *(void (**)(void*, int))(*(unsigned char**)link + 0x1c);
    setVisible(link, 1);
}

/*
 * ImageList::RefreshCollection -- Free+Get texture strip, clamp focus, Update.
 * Soft ceiling: ~69.8% -- Dispatcher arg/accept schedule; stop.
 */
void RefreshCollection__9ImageListFv(ImageList* self) {
    unsigned int countScratch;
    ImageListTexCollection* col;
    int accept;
    void** vtbl;
    void (*updateFn)(ImageList* self);
    void (*refreshOptFn)(ImageList* self);

    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    if (self->collectionId == -1) {
        return;
    }
    countScratch = 1;
    FreeTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_t(
        m_pGameVariables__13ScreenControl, (unsigned int)self->gvContext,
        self->collectionId, &self->textureInfo);
    GetTextureCollection__22GameVariableDispatcherFUiiP15GMTextureInfo_tRUi(
        m_pGameVariables__13ScreenControl, (unsigned int)self->gvContext,
        self->collectionId, &self->textureInfo, &countScratch);
    col = self->textureInfo.data;
    if (col != 0) {
        /*
         * Retail: if texReady==0 and colors!=0, skip refreshFlag check;
         * else require refreshFlag==1. Then count!=0 before Update.
         */
        accept = 0;
        if (self->textureInfo.ready == 0 && col->colors != 0) {
            accept = 1;
        } else if (col->refreshFlag == 1) {
            accept = 1;
        }
        if (accept != 0 && col->count != 0) {
            self->textureInfo.ready = 1;
            col->refreshFlag = 0;
            if (self->focusIndex > col->count) {
                self->focusIndex = col->count - 1;
            } else if (self->focusIndex < 0) {
                self->focusIndex = 0;
            }
            vtbl = (void**)self->vtbl;
            updateFn = (void (*)(ImageList*))vtbl[0x4c / 4];
            updateFn(self);
        }
    }
    vtbl = (void**)self->vtbl;
    refreshOptFn = (void (*)(ImageList*))vtbl[0x44 / 4];
    refreshOptFn(self);
}

/*
 * ImageList::RefreshOption -- GetInt focus, clamp, sync scrollBase, Update.
 */
void RefreshOption__9ImageListFv(ImageList* self) {
    int count;

    if (m_pGameVariables__13ScreenControl == 0) {
        return;
    }
    if (self->textureInfo.ready == 0) {
        return;
    }
    count = self->textureInfo.data->count;
    self->focusIndex = GetInt__22GameVariableDispatcherFUiUi(
        m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
        (unsigned int)self->optionId);
    if (self->focusIndex > count) {
        self->focusIndex = count - 1;
    }
    if (self->focusIndex < 0) {
        self->focusIndex = 0;
    }
    self->scrollBase = self->focusIndex;
    (*(void (**)(ImageList*))(*(unsigned char**)self + 0x44))(self);
}

void ScrollDec__9ImageListFi(ImageList* self, int delta) {
    if (self->textureInfo.ready != 0) {
        (*(void (**)(ImageList*, int))(*(unsigned char**)self + 0x54))(self,
                                                                       delta);
    }
}

void ScrollInc__9ImageListFi(ImageList* self, int delta) {
    if (self->textureInfo.ready != 0) {
        (*(void (**)(ImageList*, int))(*(unsigned char**)self + 0x50))(self,
                                                                       delta);
    }
}

/*
 * ImageList::Decrement -- step focus/scroll backward by delta.
 */
void Decrement__9ImageListFi(ImageList* self, int delta) {
    int count;

    if (self->textureInfo.ready == 0) {
        return;
    }
    count = self->textureInfo.data->count;
    if (self->focusIndex == self->scrollBase) {
        if (self->focusIndex == 0 && self->wrap == 0) {
            delta = 0;
        } else {
            self->scrollBase = self->scrollBase - delta;
            if (self->scrollBase < 0) {
                self->scrollBase = self->scrollBase + count;
            }
        }
    }
    self->focusIndex = self->focusIndex - delta;
    if (self->focusIndex < 0) {
        self->focusIndex = self->focusIndex + count;
    }
    if (self->optionId >= 0 && m_pGameVariables__13ScreenControl != 0) {
        SetInt__22GameVariableDispatcherFUiUii(
            m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
            (unsigned int)self->optionId, self->focusIndex);
    }
    (*(void (**)(ImageList*))(*(unsigned char**)self + 0x44))(self);
}

/*
 * ImageList::Increment -- step focus/scroll forward by delta.
 */
void Increment__9ImageListFi(ImageList* self, int delta) {
    int count;
    int winEnd;

    if (self->textureInfo.ready == 0) {
        return;
    }
    count = self->textureInfo.data->count;
    if (self->wrap == 0 && self->focusIndex + delta >= count) {
        return;
    }
    winEnd = self->scrollBase + self->itemCount - 1;
    winEnd = winEnd - (winEnd / count) * count;
    if (self->focusIndex == winEnd) {
        self->scrollBase = self->scrollBase + delta;
        if (self->scrollBase >= count) {
            self->scrollBase = self->scrollBase - count;
        }
    }
    self->focusIndex = self->focusIndex + delta;
    if (self->focusIndex >= count) {
        self->focusIndex = self->focusIndex - count;
    }
    if (self->optionId >= 0 && m_pGameVariables__13ScreenControl != 0) {
        SetInt__22GameVariableDispatcherFUiUii(
            m_pGameVariables__13ScreenControl, (unsigned int)self->pad9C,
            (unsigned int)self->optionId, self->focusIndex);
    }
    (*(void (**)(ImageList*))(*(unsigned char**)self + 0x44))(self);
}

/*
 * ImageList::HandleEvent -- 0x407 load tex; 0x408 free; 0x405 accept/refresh.
 * Soft ceiling: ~62% -- event cascade / thin GetTexture schedule; stop.
 */
void HandleEvent__9ImageListFP9ScreenMgrii(ImageList* self, void* mgr, int event,
                                           int arg) {
    unsigned int countScratch;
    ImageListTexCollection* col;
    int accept;

    if (event == 0x407) {
        (*(void (**)(ImageList*))(*(unsigned char**)self + 0x4c))(self);
        if (m_pGameVariables__13ScreenControl != 0 &&
            self->collectionId != -1) {
            countScratch = 0;
            GetTextureCollection__22GameVariableDispatcherFiP15GMTextureInfo_tRUi(
                m_pGameVariables__13ScreenControl, self->collectionId,
                &self->textureInfo, &countScratch);
            (*(void (**)(ImageList*, void*, int, int))(*(unsigned char**)self +
                                                       0x3c))(self, mgr, 0x405,
                                                              arg);
        }
    } else if (event >= 0x407) {
        if (event >= 0x409) {
            /* no-op (includes 0x9282a) */
        } else {
            /* 0x408 */
            if (m_pGameVariables__13ScreenControl != 0 &&
                self->collectionId != -1) {
                FreeTextureCollection__22GameVariableDispatcherFiP15GMTextureInfo_t(
                    m_pGameVariables__13ScreenControl, self->collectionId,
                    &self->textureInfo);
            }
        }
    } else if (event == 0x405) {
        col = self->textureInfo.data;
        if (col != 0) {
            if (col->reloadFlag == 1) {
                (*(void (**)(ImageList*))(*(unsigned char**)self + 0x48))(self);
            }
            accept = 0;
            if (self->textureInfo.ready == 0 && col->colors != 0) {
                accept = 1;
            } else if (col->refreshFlag == 1) {
                accept = 1;
            }
            if (accept != 0 && col->count != 0) {
                self->textureInfo.ready = 1;
                col->refreshFlag = 0;
                if (self->focusIndex > col->count) {
                    self->focusIndex = col->count - 1;
                } else if (self->focusIndex < 0) {
                    self->focusIndex = 0;
                }
                (*(void (**)(ImageList*))(*(unsigned char**)self + 0x4c))(self);
                (*(void (**)(ImageList*))(*(unsigned char**)self + 0x44))(self);
            }
        }
    }
}

/*
 * ImageList::HandleAction -- Increment/Decrement/Scroll + refresh; else 0.
 * Soft ceiling: ~35% -- retail binary cmp tree vs sequential ifs; stop.
 */
int HandleAction__9ImageListFP9ScreenMgrPC12ScreenAction(ImageList* self,
                                                         void* mgr,
                                                         const void* actionIn) {
    const ScreenActionView* action;
    void* params;
    int arg;
    int result;
    int a;
    void* node;
    int (*nodeHandle)(void* node, void* mgr, const void* action);

    action = (const ScreenActionView*)actionIn;
    arg = action->arg;
    params = action->params;
    result = 1;

    if (arg == 0x7d3) {
        (*(void (**)(ImageList*, int))(*(unsigned char**)self + 0x5c))(self, 1);
    } else if (arg >= 0x7d3) {
        if (arg == 0x7db) {
            node = GetScreenNode__12ScreenParamsFUi(params, 0);
            if (node == (void*)self) {
                a = GetInt__12ScreenParamsFUi(params, 1);
                if (a == 0) {
                    (*(void (**)(ImageList*))(*(unsigned char**)self + 0x48))(
                        self);
                } else if (a == 1) {
                    (*(void (**)(ImageList*))(*(unsigned char**)self + 0x4c))(
                        self);
                }
            } else if (node != 0) {
                nodeHandle =
                    *(int (**)(void*, void*, const void*))(*(unsigned char**)node +
                                                           0x28);
                nodeHandle(node, mgr, actionIn);
            }
        } else {
            result = 0;
        }
    } else if (arg == 0x7d1) {
        (*(void (**)(ImageList*, int))(*(unsigned char**)self + 0x54))(self, 1);
    } else if (arg >= 0x7d1) {
        (*(void (**)(ImageList*, int))(*(unsigned char**)self + 0x58))(self, 1);
    } else if (arg >= 0x7d0) {
        (*(void (**)(ImageList*, int))(*(unsigned char**)self + 0x50))(self, 1);
    } else {
        result = 0;
    }
    return result;
}

void ProcessParams__9ImageListFP12ScreenParams(ImageList* self, void* params) {
    typedef struct ScreenPolyIndexedVertView {
        unsigned char pad00[0x18];
        float x;
        float y;
    } ScreenPolyIndexedVertView;

    int i;
    int nodeIndex;
    ScreenNode* link;
    ScreenPoly* poly;
    ScreenPolyIndexedVertView* vert;
    void (*setVisible)(void* node, int visible);

    /*
     * Soft ceiling ~94.7%: remaining FPR/vtbl scheduling and NV color.
     */

    self->itemCount = GetInt__12ScreenParamsFUi(params, 0);
    self->collectionId = GetResourceID__12ScreenParamsFUi(params, 1);
    self->optionId = GetResourceID__12ScreenParamsFUi(params, 2);
    self->wrap = GetBoolean__12ScreenParamsFUi(params, 3);
    self->hasLinkNode = GetBoolean__12ScreenParamsFUi(params, 4);
    link = 0;
    nodeIndex = 5;
    if (self->hasLinkNode != 0) {
        link = GetScreenNode__12ScreenParamsFUi(params, 5);
        nodeIndex = 6;
    }
    self->itemNodes = (ScreenNode**)Malloc__10ScreenUtilFUliPc(
        (unsigned long)(self->itemCount << 2), kMallocTagInit,
        (char*)(stringBase0 + 0x307));
    for (i = 0; i < self->itemCount; i++) {
        self->itemNodes[i] =
            GetScreenNode__12ScreenParamsFUi(params, (unsigned int)(i + nodeIndex));
        setVisible =
            *(void (**)(void*, int))(*(unsigned char**)self->itemNodes[i] + 0x1c);
        setVisible(self->itemNodes[i], 0);
    }
    if (self->hasLinkNode != 0) {
        self->linkNode = link;
        poly = (ScreenPoly*)self->itemNodes[0];
        if (poly != 0) {
            float y;
            float x;

            vert = (ScreenPolyIndexedVertView*)((unsigned char*)poly +
                                                vert_map__10ScreenPoly[0] * 0x14);
            y = vert->y;
            x = vert->x;
            self->layoutX = x - kGvFloatZero;
            self->layoutY = (480.0f - y) - kGvFloatZero;
            self->layoutZ = kGvFloatZero;
        }
        if (link != 0) {
            setVisible = *(void (**)(void*, int))(*(unsigned char**)link + 0x1c);
            setVisible(link, 0);
        }
    }
}

extern void* __vt__8KeyEntry;
extern void __dt__13ScreenControlFv(void* self, short del);
extern void __dl__13ScreenControlFPv(void* self);
extern void __dt__21ScreenResourceLibraryFv(void* self, short del);

#define DEFINE_SCREEN_CONTROL_DTOR(name, vtable)                    \
    void* name(void* self, short del) {                             \
        if (self != 0) {                                            \
            *(void**)self = &(vtable);                              \
            __dt__13ScreenControlFv(self, 0);                       \
            if (del > 0) {                                         \
                __dl__13ScreenControlFPv(self);                     \
            }                                                       \
        }                                                           \
        return self;                                                \
    }

DEFINE_SCREEN_CONTROL_DTOR(__dt__8KeyEntryFv, __vt__8KeyEntry)
DEFINE_SCREEN_CONTROL_DTOR(__dt__11SpreadSheetFv, __vt__11SpreadSheet)
DEFINE_SCREEN_CONTROL_DTOR(__dt__6KeyPadFv, __vt__6KeyPad)
DEFINE_SCREEN_CONTROL_DTOR(__dt__8TextItemFv, __vt__8TextItem)
DEFINE_SCREEN_CONTROL_DTOR(__dt__8WifImageFv, __vt__8WifImage)
DEFINE_SCREEN_CONTROL_DTOR(__dt__8TextListFv, __vt__8TextList)
DEFINE_SCREEN_CONTROL_DTOR(__dt__9ImageListFv, __vt__9ImageList)

#undef DEFINE_SCREEN_CONTROL_DTOR

extern void __dt__12ScreenActionFv(void* self, short del);
extern void __dl__12ScreenActionFPv(void* self);
extern void __dt__12ScreenClientFv(void* self, short del);

#define DEFINE_SCREEN_ACTION_DTOR(name, vtable)                     \
    void* name(void* self, short del) {                             \
        if (self != 0) {                                            \
            *(void**)self = (vtable);                               \
            __dt__12ScreenActionFv(self, 0);                        \
            if (del > 0) {                                         \
                __dl__12ScreenActionFPv(self);                      \
            }                                                       \
        }                                                           \
        return self;                                                \
    }

DEFINE_SCREEN_ACTION_DTOR(
    __dt__18ScreenActionRandomFv, __vt__18ScreenActionRandom)
DEFINE_SCREEN_ACTION_DTOR(
    __dt__23ScreenActionCheckOnlineFv, __vt__23ScreenActionCheckOnline)
DEFINE_SCREEN_ACTION_DTOR(
    __dt__27ScreenActionOnlineChallengeFv,
    __vt__27ScreenActionOnlineChallenge)
DEFINE_SCREEN_ACTION_DTOR(
    __dt__32ScreenActionOnlineIsOpponentIdleFv,
    __vt__32ScreenActionOnlineIsOpponentIdle)
DEFINE_SCREEN_ACTION_DTOR(
    __dt__32ScreenActionOnlinePickChallengerFv,
    __vt__32ScreenActionOnlinePickChallenger)
DEFINE_SCREEN_ACTION_DTOR(
    __dt__33ScreenActionOnlineChallengeCancelFv,
    __vt__33ScreenActionOnlineChallengeCancel)
DEFINE_SCREEN_ACTION_DTOR(
    __dt__37ScreenActionOnlineResetChallengeStateFv,
    __vt__37ScreenActionOnlineResetChallengeState)

#undef DEFINE_SCREEN_ACTION_DTOR

void* __dt__20mkScreenEngineClientFv(void* self, short del) {
    if (self != 0) {
        *(void**)self = __vt__20mkScreenEngineClient;
        __dt__12ScreenClientFv(self, 0);
        if (del > 0) {
            __dl__FPv(self);
        }
    }
    return self;
}

void* __dt__17SpreadSheet_imageFv(SpreadSheet_image* self, short del) {
    if (self != 0) {
        self->sheet.vtbl = &__vt__17SpreadSheet_image;
        self->sheet.vtbl = &__vt__11SpreadSheet;
        __dt__13ScreenControlFv(self, 0);
        if (del > 0) {
            __dl__13ScreenControlFPv(self);
        }
    }
    return self;
}

void* __dt__16SpreadSheet_textFv(SpreadSheet_text* self, short del) {
    if (self != 0) {
        self->sheet.vtbl = &__vt__16SpreadSheet_text;
        self->sheet.vtbl = &__vt__11SpreadSheet;
        __dt__13ScreenControlFv(self, 0);
        if (del > 0) {
            __dl__13ScreenControlFPv(self);
        }
    }
    return self;
}

void* __dt__10ScreenPolyFv(ScreenPoly* self, short del) {
    ScreenObj* live;

    if (self != 0) {
        self->vtbl = &__vt__10ScreenPoly;
        live = self->screenObj;
        if (live != 0 &&
            *(unsigned int*)((char*)live + 4) !=
                (unsigned int)self->screenObjInstance) {
            live = 0;
        }
        if (live != 0) {
            if (*(int*)((char*)self->screenObj + 4) != 0) {
                ((void (*)(void*))(*(void***)self->screenObj)[4])(
                    self->screenObj);
            }
            self->screenObj = 0;
            self->screenObjInstance = 0;
        }
        __dt__10ScreenNodeFv(self, 0);
        if (del > 0) {
            __dl__10ScreenNodeFPv(self);
        }
    }
    return self;
}

void* __dt__10ScreenTextFv(ScreenText* self, short del) {
    StringObj* live;

    if (self != 0) {
        self->vtbl = &__vt__10ScreenText;
        live = self->stringObj;
        if (live != 0 &&
            live->instance != (unsigned int)self->stringObjInstance) {
            live = 0;
        }
        if (live != 0) {
            if (*(int*)((char*)self->stringObj + 4) != 0) {
                ((void (*)(void*))(*(void***)self->stringObj)[4])(
                    self->stringObj);
            }
            self->stringObj = 0;
            self->stringObjInstance = 0;
        }
        __dt__10ScreenNodeFv(self, 0);
        if (del > 0) {
            __dl__10ScreenNodeFPv(self);
        }
    }
    return self;
}

void* __dt__29mkScreenEngineResourceLibraryFv(void* self, short del) {
    if (self != 0) {
        *(void**)self = __vt__29mkScreenEngineResourceLibrary;
        hashtable_foreach((Hashtable*)((char*)self + 8), free_string);
        hashtable_destroy((Hashtable*)((char*)self + 8));
        __dt__21ScreenResourceLibraryFv(self, 0);
        if (del > 0) {
            __dl__FPv(self);
        }
    }
    return self;
}

}
