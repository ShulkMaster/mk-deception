#include "runtime/utils.h"

#include "game/game_info.h"
#include "game/ladder.h"
#include "game/menu.h"
#include "game/plyr.h"
#include "game/specular.h"
#include "game/controller.h"
#include "game/memcard.h"
#include "game/nbc.h"
#include "game/plyrprofile.h"
#include "game/settings.h"
#include "movie/movie_info.h"
#include "movie/MkMovies.h"
#include "mw/mwMemHeap.h"
#include "math/gxMath.h"
#include "platform/main.h"
#include "platform/gcmcard.h"
#include "platform/display.h"
#include "platform/fog.h"
#include "platform/io.h"
#include "runtime/fonts.h"
#include "runtime/cam.h"
#include "runtime/image.h"
#include "runtime/asset.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_vtbl.h"
#include "runtime/plyr_anim_pdata.h"
#include "runtime/plyr_pdata.h"
#include "runtime/section.h"
#include "runtime/sound.h"
#include "rw/rpmatfx.h"
#include "rw/rpskin.h"
#include "rw/rwfreelist.h"
#include "rw/rwresources.h"

extern MkVtable5 vtbl_mkpdata_string_obj;

void* memset(void* dst, int c, unsigned long n);
unsigned int genlrand(void);
int get_platform_language_setting(void);
int ck_eat_online_switches(void);
long long debug_get_usec_timer(void);
void limb_sever_reset_limbs(PlyrInfo* player);

/* mode_of_play lives in main.o sbss (also referenced from display.c). */
extern int mode_of_play;
extern int screen_width;
extern int p1_profile_status;
extern int p2_profile_status;
extern int p1_profile_device;
extern int p2_profile_device;
extern int p1_profile_slot;
extern int p2_profile_slot;
extern int f_writing_to_memcard;
extern void* p1_profile_common;
extern void* p2_profile_common;
extern PlayerProfile p1_profile;
extern PlayerProfile p2_profile;
extern int p1_rumble_on;
extern int p2_rumble_on;
extern void fire_screen_studio_event(int event, int arg);
/* setup_fixed_block_heaps indexes jump_target_mode (.sdata), not mode_of_play. */
extern int jump_target_mode;
extern int use_feedback_effect;
extern MkPtr* pfx_render_list;
extern MkPtr* pfx_clone_render_list;
extern int small_ground_fx;
extern int large_ground_fx;
extern int mcard_msg_active;
extern MslSoundHandle bgnd_music_ptr1;
extern MslSoundHandle bgnd_music_ptr2;
extern float game_volume;
extern _mslSystem* msi;

extern MkHdr* apdata_save;
extern PlyrPdata* plyr_pdata;
extern MkObj* plyr_obj;
extern MkObj* his_obj;
extern PlyrPdata* his_pdata;
extern MkProc* plyr_anim_proc;
extern AnimPdata* plyr_anim_pdata;
extern int f_fatality_finished;
extern int f_fatality_available;
extern int f_fatality_was_done;
extern int b_game_timer_off;
extern int konquest_human_bones[17];
extern MkFlippedBoneMap flipped_konquest_human_bones;
extern unsigned char monk_ground_colls[];

typedef struct GlobalObjectLatch {
    void* object;
    unsigned int instance;
} GlobalObjectLatch;

extern GlobalObjectLatch p1_freeze_light_item;
extern GlobalObjectLatch p2_freeze_light_item;
extern GlobalObjectLatch p1_freeze_proc_item;
extern GlobalObjectLatch p2_freeze_proc_item;
extern GlobalObjectLatch rope_proc_item;
extern GlobalObjectLatch sobj_ctrl_proc_item;

void init_bgnd_info_struct(void);
void init_game_info_struct(void);
void setDroneOverrideSwitch(int activated, void* info);
void reset_obstacle_internal_id(void);
void turn_switch_log_off(void);
void delete_player(int player_index);
void destroy_background_extras(void);
void terminate_bgnd_collisions(void);
void screen_engine_cleanup(void);
void delete_light_lists(void);
void cleanup_minigame_system(void);
void cleanup_konquest(void);
void mk_chess_cleanup(void);
void cleanup_drone_ai(void);
void cleanup_fight(void);
void unload_all_effect_banks(void);
void term_collision_system(void);
void bleed_init(void);
void initialize_background_danger_zones(void);
void mslStopAll(_mslSystem* system);
void mslUnDuckAll(_mslSystem* system);
void init_collision_system(void);
void init_screen_engine(void);
void pfxscript_initialize(void);

typedef struct FixedHeapConfigStorage {
    FixedHeapConfig heaps;
    unsigned long field_0x34;
} FixedHeapConfigStorage;
typedef char FixedHeapConfigStorageSizeCheck[
    sizeof(FixedHeapConfigStorage) == 0x38 ? 1 : -1];

static FixedHeapConfigStorage current_heap_block_counts;
static int game_state_stack[8];
static int game_state_stack_depth = -1;
static MkPtr* uv_scroll_control_list;
static int set_2dobj_oid;
static unsigned char set_2dobj_alpha;
static int set_2dobj_hide_state;
static int tick_count;
static int language_setting;
static int p2_profile_load_complete;
static int p1_profile_load_complete;

char pathname_buffer[0x78];
UsecTimerData usec_timer_data[9];
int depth_of_field_active;

typedef struct LoadProfilePdata {
    MkHdr hdr;
    int player;
    int port;
    unsigned char* code;
} LoadProfilePdata;

typedef struct ProfileCommonRumble {
    char pad00[0xFC];
    int rumble;
} ProfileCommonRumble;

static const float kFadeScaleX = 50.0f;
static const float kFadeScaleY = 40.0f;
static const float kFadeSleepTick = 1.0f;
static const float kFadeDoneTick = -1.0f;
static const float kMinGameVol = 0.0f;

#define FADE_PROC_PID 0x2098
#define FADE_BLACK_OID 0x2057
#define FADE_WHITE_OID 0x2056
#define LOAD_PROFILE_PROC_PID 0x3009
#define MEMCARD_SCAN_PROC_PID 0x300B
#define PROFILE_LOAD_EVENT 0x1FE4
#define PROFILE_LOAD_PENDING 0
#define PROFILE_LOAD_NOT_FOUND 1
#define PROFILE_LOAD_OK 2
#define PROFILE_LOAD_ERROR 3
#define MKPTR_LIST_AVAILABLE(list) ((list) != 0)

static const Vec kDebugDamageBoneOffset = {0.0f, 0.0f, 0.0f};

/*
 * utils.o @stringBase0 (0x802FF2A4). Offsets used by play_movie / fade_screen.
 * Movie basenames also appear here in retail; movie_info[] below uses its own
 * string literals so paths resolve without the ASM reloc pool.
 */
static const char stringBase0[] =
    "ARCADE.SFD\0CHESS.SFD\0PUZZLE.SFD\0KONQUEST.SFD\0OPENINGN.SFD\0OPENINGW.SFD\0"
    "V_MK4FLY.SFD\0V_MKDRSB.SFD\0V_MKDTPC.SFD\0V_MKDTRL.SFD\0V_MKDTRS.SFD\0"
    "V_MKDASK.SFD\0V_MKDATV.SFD\0V_MKMOUT.SFD\0V_MKMPRO.SFD\0V_MKMSZD.SFD\0"
    "V_MKMTMP.SFD\0V_QCSKLW.SFD\0V_QCVCST.SFD\0V_RAIDLT.SFD\0VP_MNK.SFD\0"
    "VP_ZHA.SFD\0VP_MOI.SFD\0VP_VAL.SFD\0VP_GOJ.SFD\0VP_HUA.SFD\0VP_MIA.SFD\0"
    "VP_SIL.SFD\0VP_CHOU.SFD\0VP_CHOY.SFD\0V_MKDTRE.SFD\0V_MKD_OA.SFD\0"
    "VTC_BARA.SFD\0VTC_GORO.SFD\0VTC_JAX.SFD\0VTC_KITA.SFD\0VTC_RAID.SFD\0"
    "VTC_SCOR.SFD\0VTC_SHUJ.SFD\0VTC_SIND.SFD\0VTC_SONY.SFD\0VTC_SUBZ.SFD\0"
    "TITLEN.SFD\0TITLEW.SFD\0LOGON.SFD\0LOGOW.SFD\0QUADN.SFD\0QUADW.SFD\0"
    "TITLEFN.SFD\0TITLEFW.SFD\0%d\0V_\0VP_\0/kryptmovies/%s\0KRYPT\\%s\0"
    "/movies/%s\0LAD_KOINBAR\0WEAPREFL\0WHITE_FADEBOX\0FADEBOX\0"
    "Randu0 Error 02: Input: %d  Output: %d\n\0"
    "Randu0 Error 04: Input: %d  Output: %d\n\0"
    "Bad material or data\0"
    "Warning: Loading Material that has no texture!\0"
    "g_game_info\0konquest_human_bones\0flipped_konquest_human_bones\0"
    "monk_ground_colls\0";

#define STR_MOVIE_V_PREFIX (&stringBase0[0x261])
#define STR_MOVIE_VP_PREFIX (&stringBase0[0x264])
#define STR_KRYPT_MOVIE_PATH (&stringBase0[0x268])
#define STR_KRYPT_MOVIE_WIN (&stringBase0[0x278])
#define STR_MOVIE_PATH (&stringBase0[0x281])
#define STR_WHITE_FADEBOX (&stringBase0[0x2A1])
#define STR_FADEBOX (&stringBase0[0x2AF])
#define STR_DAMAGE_INT (&stringBase0[0x25E])
#define STR_RANDU_ERROR_02 (&stringBase0[0x2B7])
#define STR_RANDU_ERROR_04 (&stringBase0[0x2DF])
#define STR_LAD_KOINBAR (&stringBase0[0x28C])
#define STR_WEAPREFL (&stringBase0[0x298])
#define STR_GAME_INFO_TABLE (&stringBase0[0x34B])
#define STR_KONQUEST_HUMAN_BONES (&stringBase0[0x357])
#define STR_FLIPPED_KONQUEST_HUMAN_BONES (&stringBase0[0x36C])
#define STR_MONK_GROUND_COLLS (&stringBase0[0x389])
#define MOVIE_NAME(offset) (&stringBase0[(offset)])

/*
 * B17 P0: movie_info[] lifted from utils.o .data:0x80337220 (Ghidra + ASM).
 * NonMatching: retail table still comes from split ASM for sha1; this C copy is
 * for play_movie path resolution. Entry size 0x20; 45 slots (0..0x2C).
 */
int screen_engine_movie_table[6] = {0, 1, 2, 3, 0x2A, 0x29};

MovieInfoEntry movie_info[MOVIE_INFO_COUNT] = {
    /* 0x00 */ { MOVIE_NAME(0x000), 256, 256, 0, 0, 0, 1, 1 },
    /* 0x01 */ { MOVIE_NAME(0x00B), 256, 256, 0, 0, 0, 1, 1 },
    /* 0x02 */ { MOVIE_NAME(0x015), 256, 256, 0, 0, 0, 1, 1 },
    /* 0x03 */ { MOVIE_NAME(0x020), 256, 256, 0, 0, 0, 1, 1 },
    /* 0x04 MOVIE_ID_INTRO */ { MOVIE_NAME(0x02D), 640, 400, MOVIE_NAME(0x03A), 736, 320, 0, 0 },
    /* 0x05 */ { MOVIE_NAME(0x047), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x06 */ { MOVIE_NAME(0x054), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x07 */ { MOVIE_NAME(0x061), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x08 */ { MOVIE_NAME(0x06E), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x09 */ { MOVIE_NAME(0x07B), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x0A */ { MOVIE_NAME(0x088), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x0B */ { MOVIE_NAME(0x095), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x0C */ { MOVIE_NAME(0x0A2), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x0D */ { MOVIE_NAME(0x0AF), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x0E */ { MOVIE_NAME(0x0BC), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x0F */ { MOVIE_NAME(0x0C9), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x10 */ { MOVIE_NAME(0x0D6), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x11 */ { MOVIE_NAME(0x0E3), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x12 */ { MOVIE_NAME(0x0F0), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x13 */ { MOVIE_NAME(0x0FD), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x14 */ { MOVIE_NAME(0x108), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x15 */ { MOVIE_NAME(0x113), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x16 */ { MOVIE_NAME(0x11E), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x17 */ { MOVIE_NAME(0x129), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x18 */ { MOVIE_NAME(0x134), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x19 */ { MOVIE_NAME(0x13F), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x1A */ { MOVIE_NAME(0x14A), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x1B */ { MOVIE_NAME(0x155), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x1C */ { MOVIE_NAME(0x161), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x1D */ { MOVIE_NAME(0x16D), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x1E */ { MOVIE_NAME(0x17A), 640, 480, 0, 0, 0, 0, 0 },
    /* 0x1F */ { MOVIE_NAME(0x187), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x20 */ { MOVIE_NAME(0x194), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x21 */ { MOVIE_NAME(0x1A1), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x22 */ { MOVIE_NAME(0x1AD), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x23 */ { MOVIE_NAME(0x1BA), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x24 */ { MOVIE_NAME(0x1C7), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x25 */ { MOVIE_NAME(0x1D4), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x26 */ { MOVIE_NAME(0x1E1), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x27 */ { MOVIE_NAME(0x1EE), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x28 */ { MOVIE_NAME(0x1FB), 640, 400, 0, 0, 0, 0, 0 },
    /* 0x29 MOVIE_ID_TITLE */ { MOVIE_NAME(0x208), 640, 480, MOVIE_NAME(0x213), 704, 480, 0, 0 },
    /* 0x2A MOVIE_ID_MIDWAY_LOGO */ { MOVIE_NAME(0x21E), 640, 480, MOVIE_NAME(0x228), 704, 480, 0, 0 },
    /* 0x2B MOVIE_ID_QUAD */ { MOVIE_NAME(0x232), 640, 480, MOVIE_NAME(0x23C), 704, 480, 0, 0 },
    /* 0x2C MOVIE_ID_TITLE_ALT */ { MOVIE_NAME(0x246), 640, 480, MOVIE_NAME(0x252), 704, 480, 0, 0 },
};

typedef struct FadeScreenPdata {
    MkHdr hdr;
    int to_fade;
    int frames;
    int color;
    int audio_flag;
    ScreenObj* screen_obj;
    unsigned int screen_instance;
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    unsigned char alpha;
} FadeScreenPdata;

typedef struct MkVtableMkprocLocal {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
} MkVtableMkprocLocal;

/*
 * blink_cursor pdata (get_mkhdr size 0x28). Toggles ScreenObjFlags.hidden
 * (byte +0x0C bit4 / 0x10) -- same bit as hide_screen_obj / show_or_hide
 * screen path. StringObjs use StringObjVisBits.hidden (0x80) instead.
 */
typedef struct BlinkCursorPdata {
    MkHdr hdr; /* +0x00 */
    ScreenObj* obj; /* +0x08 */
    int on_ticks; /* +0x0C */
    int off_ticks; /* +0x10 */
} BlinkCursorPdata;

typedef MkProc* (*UtilsCreateMkprocFn)(int proc_id, int priority, MkProcEntryFn proc_fn,
                                       int pdata_size, MkHdr** pdata_out);

static const float kBlinkDoneTick = -1.0f;

static float p_blink_cursor(void);
static void show_or_hide_2dobj(MkHdr* hdr);

typedef struct DebugDamagePdata {
    MkHdr hdr;
    StringObj* object;
    unsigned int object_instance;
    int direction;
    int alpha;
    int delay;
} DebugDamagePdata;

void get_bone_offset_world_pos(
    MkObj* object, int bone, const Vec* offset, Vec* position);
void camera_get_screen_pos_from_world_pos(
    const Vec* world, RwV2d* screen);
int sprintf(char* dest, const char* fmt, ...);
double __fabs(double value);

static float p_debug_damage_txt(void) {
    DebugDamagePdata* pdata;
    StringObj* object;
    PfxFontInstance* part;
    unsigned char alpha;

    pdata = (DebugDamagePdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    if (pdata->delay > 0) {
        object = pdata->object;
        if (object != 0) {
            object = object->instance == pdata->object_instance ? object : 0;
        } else {
            object = 0;
        }
        if (object != 0) {
            if (pdata->direction == 0) {
                object->render_x--;
                object->render_y++;
            } else {
                object->render_x++;
                object->render_y++;
            }
        }
        pdata->delay--;
        return 1.0f;
    }

    if (pdata->alpha > 0) {
        pdata->alpha -= 8;
        object = pdata->object;
        if (object != 0) {
            object = object->instance == pdata->object_instance ? object : 0;
        } else {
            object = 0;
        }
        if (object != 0) {
            alpha = (unsigned char)pdata->alpha;
            part = &object->pfx.instance0;
            while (part != 0) {
                part->rgba[3] = alpha;
                part = part->next;
            }
            if (pdata->direction == 0) {
                object->render_x--;
                object->render_y++;
            } else {
                object->render_x++;
                object->render_y++;
            }
        }
        if (pdata->alpha - 8 < 0) {
            pdata->alpha = 0;
        }
        return 1.0f;
    }

    object = pdata->object;
    if (object != 0) {
        object = object->instance == pdata->object_instance ? object : 0;
    } else {
        object = 0;
    }
    if (object != 0 && object->instance != 0) {
        object->typed_vtbl->destroy(object);
    }
    return -1.0f;
}

void display_debug_damage(PlyrInfo* player, float damage) {
    DebugDamagePdata* pdata;
    StringObj* object;
    char text[0x50];
    Vec bone_offset = kDebugDamageBoneOffset;
    Vec world_position;
    RwV2d screen_position;

    if ((float)__fabs(damage) < 0.01f) {
        return;
    }
    damage *= -1.0f;
    get_bone_offset_world_pos(
        player->slot.mirror_a, 9, &bone_offset, &world_position);
    camera_get_screen_pos_from_world_pos(&world_position, &screen_position);
    sprintf(text, STR_DAMAGE_INT, (int)(100.0f * damage));
    object = string_center_xy(
        0x209B, 0, text, (int)screen_position.x,
        (int)screen_position.y, 0x1D);
    if (_create_mkproc_generic_nostack(
            0x209D, 0x1F, p_debug_damage_txt,
            sizeof(DebugDamagePdata), (MkHdr**)&pdata) != 0) {
        pdata->object = object;
        pdata->object_instance = object->instance;
        pdata->delay = 60;
        pdata->alpha = 0xFF;
        pdata->direction = player->controller_slot;
    }
}

void Simple_MoviePlayFullScreen(const char* path, int width, int height, MovieTapoutFn tapout);
void mkMovieTexPlay(int index, const char* name, int a, int b, int c, int use_mfs);
int is_widescreen_mode(void);
int strncmp(const char* a, const char* b, unsigned long n);
int sprintf(char* dest, const char* fmt, ...);
int snprintf(char* dest, unsigned long n, const char* fmt, ...);
int printf(const char* fmt, ...);
const char* pathname_create(const char* path, int flag);
void* mwFileOpen(const char* path, int mode);
void mwFileClose(void);

extern GameInfo g_game_info;

int is_controller_removed(void);
float snd_get_game_vol(void);
void snd_set_game_vol(float vol);

static inline int fade_pause_allows_tick(void) {
    if (g_game_info.feature_flags.bits.high_bit == 0 &&
        is_controller_removed() != 0) {
        return 0;
    }
    return 1;
}

/*
 * Soft ceiling: play_movie ~98.42% fuzzy.
 * Leftover: r30/r31 swap (scaled movie_id index vs path), one volatile r0/r4
 * move, and @stringBase0 vs named stringBase0 relocations. Algorithm and CFG
 * match retail; stop per near-miss budget.
 */
int play_movie(int movie_id, MovieTapoutFn tapout_cb) {
    const char* path;
    int height;
    int width;
    int extra;
    int path_id;
    char buf[0x100];
    const char* open_path;
    unsigned int play_type;

    /* Dims from original movie_id (retail may index OOB before clamp). */
    if (is_widescreen_mode() != 0 && movie_info[movie_id].ws_path != 0) {
        height = movie_info[movie_id].ws_height;
        width = movie_info[movie_id].ws_width;
    } else {
        height = movie_info[movie_id].height;
        width = movie_info[movie_id].width;
    }

    extra = movie_info[movie_id].tex_extra;
    path_id = movie_id;
    /* Retail: cmpwi 0x2d / bge (not cmpwi 0x2c / bgt). */
    if (movie_id >= 0x2D || movie_id < 0) {
        path_id = 0;
    }

    if (is_widescreen_mode() != 0 && movie_info[path_id].ws_path != 0) {
        path = movie_info[path_id].ws_path;
    } else {
        path = movie_info[path_id].path;
    }

    play_type = movie_info[movie_id].play_type;
    /* play_type 1: in-scene texture movie; intro/midway logos use type 0. */
    if (play_type == 1) {
        int texture_path_id;

        texture_path_id = movie_id;
        if (movie_id >= 0x2D || movie_id < 0) {
            texture_path_id = 0;
        }
        if (is_widescreen_mode() != 0 && movie_info[texture_path_id].ws_path != 0) {
            path = movie_info[texture_path_id].ws_path;
        } else {
            path = movie_info[texture_path_id].path;
        }
        mkMovieTexPlay(0, path, width, height, extra, 0);
    } else if (play_type == 0) {
        /* Retail strncmp(prefix, path, n) - prefix in r3. */
        if (strncmp(STR_MOVIE_V_PREFIX, path, 2) == 0 || strncmp(STR_MOVIE_VP_PREFIX, path, 3) == 0) {
            sprintf(buf, STR_KRYPT_MOVIE_PATH, path);
            open_path = pathname_create(buf, 0);
            snprintf(buf, 0x100, STR_KRYPT_MOVIE_WIN, path);
            buf[0xFF] = 0;
            path = buf;
        } else {
            sprintf(buf, STR_MOVIE_PATH, path);
            open_path = pathname_create(buf, 0);
        }

        if (mwFileOpen(open_path, 0x21) != 0) {
            mwFileClose();
            Simple_MoviePlayFullScreen(path, width, height, tapout_cb);
            return 1;
        }
        return 0;
    }
    return 0;
}

void screen_engine_play_movie(int index) {
    if (index >= 0xC8) {
        index -= 0xC8;
    }
    play_movie(screen_engine_movie_table[index], 0);
}

int are_death_traps_on(void) {
    if (game_settings.combo_breaker == 0) {
        return 0;
    }
    return (game_settings.fatalities != 0);
}

int get_blood_level(void) {
    return game_settings.combo_breaker;
}

int get_puzzle_rounds_to_win(void) {
    return game_settings.blood_level;
}

void get_point_on_circle(float* center, float radius, float angle, float* out) {
    out[0] = radius * gxMathSin(angle) + center[0];
    out[1] = center[1];
    out[2] = radius * gxMathCos(angle) + center[2];
}

void award_koins_to_player(int player, int amount, int koin_type) {
    PlayerProfile* profile;

    if (koin_type >= 0) {
        if (koin_type >= 6) {
            return;
        }
        if (amount >= 0) {
            profile = player == 0 ? &p1_profile : &p2_profile;
            profile->koins[koin_type] += amount;
            profile->lifetime_koins[koin_type] += amount;
            g_game_info.pselect.field_1ec = g_game_info.pselect.field_1e8;
        }
    }
}

void show_koin_award(int player, int amount, int koin_type, int y) {
    int x;
    char text[0x6C];
    int state;

    if (koin_type < 0 || koin_type >= 6) {
        return;
    }
    x = 0x32;
    if (player != 0) {
        x = screen_width - 0x96;
    }
    load_named_2d_pfxobj_xy(
        0x10005, 0x209A, ladder_koin_type_to_string(koin_type),
        0, x, y, 0x30);
    load_named_2d_pfxobj_xy(
        0x10005, 0x209A, STR_LAD_KOINBAR,
        0, x - 1, y, 0x31);
    sprintf(text, STR_DAMAGE_INT, amount);
    if (amount > 999) {
        string_right_xy(0x209A, 1, text, x + 0x7C, y + 4, 0x1D);
    } else {
        string_right_xy(0x209A, 1, text, x + 0x73, y + 4, 0x1D);
    }
    if (game_state_stack_depth < 0) {
        state = 0;
    } else {
        state = game_state_stack[game_state_stack_depth];
    }
    if (state != 5) {
        if (mode_of_play == 6) {
            snd_req(0x1B17);
        } else {
            snd_req(0xDC6);
        }
    }
}

float sobj_set_bounding_sphere_radius(MkSobj* sobj, float radius) {
    RpAtomic* atomic;
    RwSphere* sphere;

    atomic = sobj->atomic;
    if ((atomic->interpolator.flags & 2) != 0) {
        _rpAtomicResyncInterpolatedSphere(atomic);
    }
    sphere = &sobj->atomic->boundingSphere;
    if (sphere != 0) {
        sphere->radius = radius;
    }
    return 0.0f;
}

float sobj_get_bounding_sphere_radius(MkSobj* sobj) {
    RpAtomic* atomic;
    RwSphere* sphere;

    atomic = sobj->atomic;
    if ((atomic->interpolator.flags & 2) != 0) {
        _rpAtomicResyncInterpolatedSphere(atomic);
    }
    sphere = &sobj->atomic->boundingSphere;
    if (sphere != 0) {
        return sphere->radius;
    }
    return 0.0f;
}

int get_mkptr_count(void) {
    return current_heap_block_counts.heaps.mkptrCount;
}

void setup_fixed_block_heaps(void) {
    unsigned int mode;

    current_heap_block_counts.heaps.mkobjCount = 100;
    current_heap_block_counts.heaps.mksobjCount = 600;
    current_heap_block_counts.heaps.mkprocCount = 0xA0;
    current_heap_block_counts.heaps.bigstackCount = 0x19;
    current_heap_block_counts.heaps.tinystackCount = 0x87;
    current_heap_block_counts.heaps.mkptrCount = 0xC00;
    current_heap_block_counts.heaps.fixed16Count = 0x100;
    current_heap_block_counts.heaps.fixed32Count = 0x100;
    current_heap_block_counts.heaps.fixed64Count = 0x100;
    current_heap_block_counts.heaps.fixed128Count = 0x80;
    current_heap_block_counts.heaps.fixed512Count = 0x80;
    current_heap_block_counts.heaps.fixed1024Count = 0x18;
    current_heap_block_counts.field_0x34 = 0;

    /* Retail loads jump_target_mode (.sdata), not mode_of_play. */
    /* Soft ceiling under -O4,s: 99.86% -- jump-table relocation symbol only. */
    mode = (unsigned int)jump_target_mode;
    switch (mode) {
    case 4:
        current_heap_block_counts.heaps.mkptrCount = 6000;
        current_heap_block_counts.heaps.mkobjCount = 0x15E;
        current_heap_block_counts.heaps.mksobjCount = 0x2D0;
        current_heap_block_counts.heaps.fixed16Count = 0x200;
        current_heap_block_counts.heaps.fixed32Count = 0x1EA;
        current_heap_block_counts.heaps.fixed64Count = 0x47E;
        current_heap_block_counts.heaps.fixed128Count = 200;
        current_heap_block_counts.heaps.fixed512Count = 0xD2;
        current_heap_block_counts.heaps.fixed1024Count = 5;
        break;
    case 1:
    case 6:
    case 0xB:
        current_heap_block_counts.heaps.mkobjCount = 0x10;
        current_heap_block_counts.heaps.mksobjCount = 0x10;
        current_heap_block_counts.heaps.fixed16Count = 600;
        current_heap_block_counts.heaps.fixed32Count = 800;
        current_heap_block_counts.heaps.fixed64Count = 500;
        current_heap_block_counts.heaps.fixed128Count = 0x80;
        current_heap_block_counts.heaps.fixed1024Count = 0x20;
        break;
    case 8:
        current_heap_block_counts.heaps.mkobjCount = 0x10;
        current_heap_block_counts.heaps.mksobjCount = 0x10;
        current_heap_block_counts.heaps.fixed16Count = 200;
        current_heap_block_counts.heaps.fixed32Count = 400;
        current_heap_block_counts.heaps.fixed64Count = 500;
        current_heap_block_counts.heaps.fixed128Count = 0x80;
        current_heap_block_counts.heaps.fixed1024Count = 0x20;
        break;
    case 3:
        current_heap_block_counts.heaps.fixed16Count = 200;
        current_heap_block_counts.heaps.fixed32Count = 100;
        current_heap_block_counts.heaps.fixed64Count = 0x96;
        current_heap_block_counts.heaps.fixed128Count = 0x80;
        current_heap_block_counts.heaps.fixed512Count = 0xDC;
        current_heap_block_counts.heaps.fixed1024Count = 1;
        break;
    case 2:
        current_heap_block_counts.heaps.fixed512Count = 0xAF;
        current_heap_block_counts.heaps.fixed64Count = 0x114;
        break;
    default:
        break;
    }

    mwMemDestroyFixedBlockHeaps();
    mwMemAllocateFixedBlockHeaps(&current_heap_block_counts.heaps);
}

void load_and_set_refl_on_weapon(void) {
    int art_section;
    unsigned int art_id;
    RwTexture* texture;
    PlyrMirrorObjLatch* latch;
    MkObj* object;

    art_section = get_shared_art_section_for_player((SharedArtPlayer*)plyr_obj);
    if (art_section == 0) {
        return;
    }
    art_id = get_artid_of_named_item_in_slot(art_section, STR_WEAPREFL, 0);
    if (art_id == 0) {
        return;
    }
    texture = load_tga(art_section, art_id);
    if (texture == 0) {
        return;
    }

    latch = &plyr_pdata->mirror_slots->weapon[0].primary;
    object = latch->obj;
    if (object != 0) {
        if (object->hdr.instance != latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        RpClumpForAllAtomics(
            object->clump, force_specular_texture_atomic_callback, texture);
    }

    latch = &plyr_pdata->mirror_slots->weapon[1].primary;
    object = latch->obj;
    if (object != 0) {
        if (object->hdr.instance != latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        RpClumpForAllAtomics(
            object->clump, force_specular_texture_atomic_callback, texture);
    }

    latch = &plyr_pdata->aux_weapon_latch;
    object = latch->obj;
    if (object != 0) {
        if (object->hdr.instance != latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        RpClumpForAllAtomics(
            object->clump, force_specular_texture_atomic_callback, texture);
    }
}

void pause_procs(int flag) {
    g_game_info.pause_flag_bits.controller_disable_guard = (unsigned char)flag;
    if (flag != 0 && !g_game_info.pause_flag_bits.rumble_stopped_for_pause) {
        turn_all_rumble_motors_off();
        g_game_info.pause_flag_bits.rumble_stopped_for_pause = 1;
    }
    if (flag == 0) {
        g_game_info.pause_flag_bits.rumble_stopped_for_pause = 0;
    }
}

int get_level_fatality_done_flag_state(void) {
    return g_game_info.flag_bits.level_fatality_done;
}

void set_level_fatality_done_flag_state(int state) {
    g_game_info.flag_bits.level_fatality_done = (unsigned char)state;
}

void pos_cam_for_current_level(void) {
    if (camera_obj != 0) {
        skip_camera_intro();
        force_midpoint_calculation_update = 1;
        adj_cam_pos();
    }
    xfer_camera(p_camera_proc, 0);
}

void reset_severed_limbs(int player) {
    PlyrInfo* player_info;

    player_info = player == 0 ? &g_game_info.plyr0 : &g_game_info.plyr1;
    if (player_info->slot.mirror_a != 0) {
        limb_sever_reset_limbs(player_info);
    }
}

void set_far_clip_plane(float dist) {
    if (Camera != 0) {
        RwCameraSetFarClipPlane(Camera, dist);
    }
}

MkObj* find_obj_by_id(int id) {
    MkPtr* link;

    link = first_mkptr(&fgnd_mkobj_list);
    while (link != 0) {
        MkObj* object = (MkObj*)link->hdr;
        MkObj* resolved;

        if (object->hdr.vtbl == &vtbl_mkobj) {
            resolved = object;
        } else {
            resolved = 0;
        }
        if (resolved != 0 && (int)resolved->oid == id) {
            return resolved;
        }
        link = next_mkptr(link);
    }
    return 0;
}

MkProc* proc_create(MkProcEntryFn proc_fn, int proc_id) {
    return _create_mkproc_generic_bigstack(
        proc_id, 0x1F, proc_fn, 0x28, (MkHdr**)&mab_generic_pdata);
}

int get_language(void) {
    return language_setting;
}

void set_language(int language) {
    language_setting = 0;
}

void initialize_language_settings(void) {
    get_platform_language_setting();
    language_setting = 0;
}

int get_language_setting(void) {
    int language = language_setting;

    if (language == 0) {
        language = 5;
    }
    return language;
}

/*
 * Cursor / UI blinker (pid from caller; Konquest text cursor uses 0x8255).
 * Sleep on_ticks visible, off_ticks hidden; dies when ScreenObj live-check fails.
 */
static inline ScreenObj* resolve_blink_object(
    ScreenObj* object, unsigned int instance) {
    ScreenObj* resolved;

    if (object != 0) {
        if ((unsigned int)object->instance == instance) {
            resolved = object;
        } else {
            resolved = 0;
        }
    } else {
        resolved = 0;
    }
    return resolved;
}

static float p_blink_cursor(void) {
    BlinkCursorPdata* pdata;
    ScreenObj* object;
    ScreenObj* live;
    unsigned int instance;
    int on_ticks;
    int off_ticks;

    pdata = (BlinkCursorPdata*)apdata;
    if (pdata != 0) {
        object = pdata->obj;
        on_ticks = pdata->on_ticks;
        off_ticks = pdata->off_ticks;
        if (object != 0) {
            instance = (unsigned int)object->instance;
        } else {
            return kBlinkDoneTick;
        }
    } else {
        return kBlinkDoneTick;
    }

    for (;;) {
        live = resolve_blink_object(object, instance);
        if (live != 0) {
            live->flag_bits.hidden = 0;
            _mkproc_sleep_ticks = (float)on_ticks;
            ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
        } else {
            return kBlinkDoneTick;
        }

        live = resolve_blink_object(object, instance);
        if (live != 0) {
            live->flag_bits.hidden = 1;
            _mkproc_sleep_ticks = (float)off_ticks;
            ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
        } else {
            return kBlinkDoneTick;
        }
    }
}

void blink_cursor(ScreenObj* obj, int proc_id, int on_ticks, int off_ticks) {
    MkProc* proc;
    BlinkCursorPdata* pdata;

    proc = ((UtilsCreateMkprocFn)_create_mkproc_generic_bigstack)(
        proc_id, 0x1F, (MkProcEntryFn)p_blink_cursor, 0x28, (MkHdr**)&mab_generic_pdata);
    if (proc != 0) {
        /* Retail reloads mab_generic_pdata for each store. */
        pdata = (BlinkCursorPdata*)mab_generic_pdata;
        pdata->obj = obj;
        pdata = (BlinkCursorPdata*)mab_generic_pdata;
        pdata->on_ticks = on_ticks;
        pdata = (BlinkCursorPdata*)mab_generic_pdata;
        pdata->off_ticks = off_ticks;
    } else {
        pdata = (BlinkCursorPdata*)mab_generic_pdata;
        pdata->obj = 0;
    }
}

void hide_or_show_2d_obj_by_id(int oid, int hide) {
    set_2dobj_hide_state = hide;
    set_2dobj_oid = oid;
    apply_to_mklist((MkListApplyFn)show_or_hide_2dobj, &screen_obj_list);
}

/*
 * ScreenObj: hide bit 0x10 (ScreenObjFlags). StringObj: hidden bit 0x80
 * (StringObjVisBits) -- retail uses different rlwimi inserts per type.
 */
static void show_or_hide_2dobj(MkHdr* hdr) {
    ScreenObj* screen;
    StringObj* text;

    if (hdr->vtbl == &vtbl_mkpdata_screen_obj) {
        screen = (ScreenObj*)hdr;
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (screen->oid == set_2dobj_oid) {
            screen->flag_bits.hidden = (unsigned char)(set_2dobj_hide_state & 1);
        }
    }

    if (hdr->vtbl == &vtbl_mkpdata_string_obj) {
        text = (StringObj*)hdr;
    } else {
        text = 0;
    }
    if (text == 0) {
        return;
    }
    if (text->oid != set_2dobj_oid) {
        return;
    }
    ((StringObjVisBits*)&text->flags)->hidden = (unsigned char)(set_2dobj_hide_state & 1);
}

void service_game_timers(void) {
    int next;
    int depth;
    int top;

    next = tick_count + 1;
    tick_count = next;
    if (next < 0x3C) {
        return;
    }

    depth = game_state_stack_depth;
    tick_count = 0;
    g_game_info.field_208 += 1;

    if (depth < 0) {
        top = 0;
    } else {
        top = game_state_stack[depth];
    }

    if (top != 7) {
        if (depth < 0) {
            top = 0;
        } else {
            top = game_state_stack[depth];
        }
        if (top != 0x12) {
            return;
        }
    }
    g_game_info.field_20C += 1;
}

void display_numerical_change(
    StringObj* string, int font, int start, int change,
    int ticks, int acceleration_interval) {
    char text[40];
    int target = start + change;
    int step = change < 0 ? -1 : 1;
    int tick_count = 0;
    int acceleration_count = 0;

    if (string != 0) {
        unsigned int instance = string->instance;

        while (start != target) {
            StringObj* live;

            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            tick_count++;
            if (tick_count >= ticks) {
                int distance;
                int step_magnitude;
                int next;

                tick_count = 0;
                if (string != 0) {
                    if (string->instance == instance) {
                        live = string;
                    } else {
                        live = 0;
                    }
                } else {
                    live = 0;
                }
                if (live == 0) {
                    return;
                }
                distance = -(target - start);
                if (target - start >= 0) {
                    distance = target - start;
                }
                step_magnitude = step < 0 ? -step : step;
                next = start + step;
                if (distance <= step_magnitude) {
                    next = target;
                }
                start = next;
                format_value_to_display(text, start);
                update_string_obj(live, font, text);
            }
            acceleration_count++;
            if (acceleration_count >= acceleration_interval) {
                step *= 2;
                acceleration_count = 0;
            }
        }
    }
}

void show_material(RpMaterial* material) {
    SpecularMaterialPluginData* specular;

    specular = mk_get_specular_material_plugin(material);
    if (specular != 0) {
        specular->flags.bits.hidden = 0;
    }
}

void hide_material(RpMaterial* material) {
    SpecularMaterialPluginData* specular;

    specular = mk_get_specular_material_plugin(material);
    if (specular != 0) {
        specular->flags.bits.hidden = 1;
    }
}

RpMaterial* material_set_color(
    RpMaterial* material, const RwRGBA* color) {
    material->color = *color;
    return material;
}

typedef struct MaterialColorById {
    int id;
    RwRGBA color;
} MaterialColorById;

RpAtomic* set_atomic_material_color_by_id(
    RpAtomic* atomic, MaterialColorById* color_by_id) {
    RpGeometry* geometry;
    unsigned int count;
    int found;
    int i;

    geometry = atomic->geometry;
    if (geometry != 0) {
        found = 0;
        i = 0;
        for (count = (unsigned int)geometry->matList.numMaterials;
             count > 0; count--) {
            RpMaterial* material = geometry->matList.materials[i];
            if ((MK_MATERIAL_PLUGIN(material)->flags & 0xFFF) ==
                (unsigned int)color_by_id->id) {
                found = 1;
                material->color = color_by_id->color;
            }
            i++;
        }
        if (found != 0) {
            geometry->flags |= 0x40;
        }
    }
    return atomic;
}

RpAtomic* set_atomic_material_color(
    RpAtomic* atomic, const RwRGBA* color) {
    RpGeometry* geometry;

    geometry = atomic->geometry;
    if (geometry != 0) {
        geometry->flags |= 0x40;
        RpGeometryForAllMaterials(
            geometry, (RpMaterialCallBack)material_set_color, (void*)color);
    }
    return atomic;
}

void obj_set_color_for_material_by_id(
    MkObj* obj, int id, RwRGBA* color) {
    MaterialColorById color_by_id;

    color_by_id.color = *color;
    color_by_id.id = id;
    RpClumpForAllAtomics(
        obj->clump, (RpAtomicCallBack)set_atomic_material_color_by_id,
        &color_by_id);
}

void obj_set_color_for_all_materials(MkObj* obj, const RwRGBA* color) {
    RpClumpForAllAtomics(
        obj->clump, (RpAtomicCallBack)set_atomic_material_color, (void*)color);
}

void sobj_set_color_for_all_materials(MkSobj* sobj, const RwRGBA* color) {
    RpAtomic* atomic;
    RpGeometry* geometry;

    atomic = sobj->atomic;
    if (atomic != 0) {
        geometry = atomic->geometry;
        if (geometry != 0) {
            RpGeometryForAllMaterials(
                geometry, (RpMaterialCallBack)material_set_color, (void*)color);
        }
    }
}

#pragma dont_inline on
int save_profile(int player, int mode) {
    StorageDevice* device_status;
    int profile_status;
    int device;
    int slot;

    if (mode_of_play == 8) {
        return 0;
    }

    profile_status = player == 0 ? p1_profile_status : p2_profile_status;
    if (profile_status != 1) {
        f_writing_to_memcard = 0;
        return 0;
    }

    if (mode_of_play == 7 && (mode == 2 || mode == 8)) {
        if (validate_konq_save_location(player) == 0) {
            f_writing_to_memcard = 0;
            return 0;
        }
    } else if (validate_save_location(player) == 0) {
        f_writing_to_memcard = 0;
        return 0;
    }

    if (player == 0) {
        device = p1_profile_device;
        slot = p1_profile_slot;
    } else {
        device = p2_profile_device;
        slot = p2_profile_slot;
    }

    if (device < 0 || device >= STORAGE_MAX_DEVICES) {
        f_writing_to_memcard = 0;
        return 0;
    }

    device_status = &storage_status[device];
    memory_save_profile(player, (PlayerProfile*)&device_status->profiles[slot]);
    return save_to_memcard_w_error(
        device, mode, nbc_find_text(0x30, 1), &device_status->settings, 0,
        &device_status->freeBlocks, &device_status->freeBytes);
}

void save_both_profiles(int unused) {
    save_profile(0, 2);
    save_profile(1, 2);
}
#pragma dont_inline reset

float p_load_profile(void) {
    LoadProfilePdata* pdata;
    StorageProfileSlot* profile;
    int device;
    int slot;
    int scan_state;
    unsigned char* code;
    int player;
    int port;

    scan_state = slot = device = 0;
    pdata = (LoadProfilePdata*)apdata;
    player = pdata->player;
    port = pdata->port;
    code = pdata->code;

    if (find_mkproc_pid(MEMCARD_SCAN_PROC_PID) == 0) {
        update_storage_status(0);
    }

    profile = scan_storage_for_code(
        &scan_state, player, port, code, &device, &slot);
    switch (scan_state) {
    case 2:
        if (profile != 0) {
            if (player == 0) {
                p1_profile_status = 1;
                p1_profile_device = device;
                p1_profile_slot = slot;
                p1_profile_load_complete = PROFILE_LOAD_OK;
                mark_profile_as_in_use(device, slot);
            } else {
                p2_profile_status = 1;
                p2_profile_device = device;
                p2_profile_slot = slot;
                p2_profile_load_complete = PROFILE_LOAD_OK;
                mark_profile_as_in_use(device, slot);
            }
            memory_load_profile(player, (PlayerProfile*)profile);
            fire_screen_studio_event(PROFILE_LOAD_EVENT, 1);
        } else if (player == 0) {
            p1_profile_status = 0;
            p1_profile_device = -1;
            p1_profile_slot = -1;
            p1_profile_load_complete = PROFILE_LOAD_NOT_FOUND;
        } else {
            p2_profile_status = 0;
            p2_profile_device = -1;
            p2_profile_slot = -1;
            p2_profile_load_complete = PROFILE_LOAD_NOT_FOUND;
        }
        break;
    case 1:
        if (player == 0) {
            p1_profile_status = 0;
            p1_profile_device = -1;
            p1_profile_slot = -1;
            p1_profile_load_complete = PROFILE_LOAD_NOT_FOUND;
        } else {
            p2_profile_status = 0;
            p2_profile_device = -1;
            p2_profile_slot = -1;
            p2_profile_load_complete = PROFILE_LOAD_NOT_FOUND;
        }
        break;
    default:
        if (player == 0) {
            p1_profile_status = 0;
            p1_profile_device = -1;
            p1_profile_slot = -1;
            p1_profile_load_complete = PROFILE_LOAD_ERROR;
        } else {
            p2_profile_status = 0;
            p2_profile_device = -1;
            p2_profile_slot = -1;
            p2_profile_load_complete = PROFILE_LOAD_ERROR;
        }
        break;
    }

    return -1.0f;
}

int load_profile(int player, int port, unsigned char* code) {
    LoadProfilePdata* pdata;
    MkProc* proc;

    pdata = 0;
    proc = _create_mkproc_generic_bigstack(
        LOAD_PROFILE_PROC_PID, 0x1F, p_load_profile, sizeof(LoadProfilePdata), (MkHdr**)&pdata);
    if (proc != 0) {
        pdata->player = player;
        pdata->port = port;
        pdata->code = code;
    }

    if (player == 0) {
        p1_profile_status = 0;
        p1_profile_device = -1;
        p1_profile_slot = -1;
        p1_profile_load_complete = PROFILE_LOAD_PENDING;
    } else {
        p2_profile_status = 0;
        p2_profile_device = -1;
        p2_profile_slot = -1;
        p2_profile_load_complete = PROFILE_LOAD_PENDING;
    }

    if (proc != 0) {
        if (player == 0) {
            while (p1_profile_load_complete == PROFILE_LOAD_PENDING) {
                _mkproc_sleep_ticks = 1.0f;
                ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
            }
            if (p1_profile_load_complete == PROFILE_LOAD_OK) {
                p1_rumble_on = ((ProfileCommonRumble*)p1_profile_common)->rumble;
            }
            fire_screen_studio_event(PROFILE_LOAD_EVENT, 1);
            return p1_profile_load_complete;
        }

        while (p2_profile_load_complete == PROFILE_LOAD_PENDING) {
            _mkproc_sleep_ticks = 1.0f;
            ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
        }
        if (p2_profile_load_complete == PROFILE_LOAD_OK) {
            p2_rumble_on = ((ProfileCommonRumble*)p2_profile_common)->rumble;
        }
        fire_screen_studio_event(PROFILE_LOAD_EVENT, 2);
        return p2_profile_load_complete;
    }

    return 0;
}

static void obj_set_alpha_by_id(MkHdr* hdr) {
    ScreenObj* screen;
    StringObj* text;
    PfxFontInstance* part;
    unsigned char alpha;
    int i;

    if (hdr->vtbl == &vtbl_mkpdata_screen_obj) {
        screen = (ScreenObj*)hdr;
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (screen->oid != set_2dobj_oid) {
            return;
        }
        for (i = 0; i < 4; i++) {
            screen->pfx2d->verts[i].a = set_2dobj_alpha;
        }
        screen->pfx2d->mirror = 1;
        return;
    }

    if (hdr->vtbl == &vtbl_mkpdata_string_obj) {
        text = (StringObj*)hdr;
    } else {
        text = 0;
    }
    if (text == 0) {
        return;
    }
    if (text->oid != set_2dobj_oid) {
        return;
    }
    alpha = set_2dobj_alpha;
    part = &text->pfx.instance0;
    while (part != 0) {
        part->rgba[3] = alpha;
        part = part->next;
    }
}

void pfx_2d_obj_set_alpha_by_id(int id, int alpha) {
    set_2dobj_alpha = (unsigned char)alpha;
    set_2dobj_oid = id;
    apply_to_mklist(obj_set_alpha_by_id, &screen_obj_list);
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
void pfx_2d_obj_set_alpha(ScreenObj* obj, int alpha) {
    int i;

    for (i = 0; i < 4; i++) {
        obj->pfx2d->verts[i].a = (unsigned char)alpha;
    }
    obj->pfx2d->mirror = 1;
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

void destroy_fade_box(void) {
    destroy_mkprocs_pid(FADE_PROC_PID);
    delete_screen_obj_oid(FADE_BLACK_OID);
    delete_screen_obj_oid(FADE_WHITE_OID);
}

void create_fade_box(void) {
    ScreenObj* obj;

    destroy_mkprocs_pid(FADE_PROC_PID);
    delete_screen_obj_oid(FADE_BLACK_OID);
    delete_screen_obj_oid(FADE_WHITE_OID);
    obj = load_2d_pfxobj(0, FADE_BLACK_OID, (char*)0x10017, 0, 0xd);
    if (obj != 0) {
        obj->x = -0x32;
        obj->y = -0x32;
        obj->flag_bits.scaled = 1;
        obj->scale_x = kFadeScaleX;
        obj->scale_y = kFadeScaleY;
    }
}

static inline ScreenObj* resolve_fade_screen_object(FadeScreenPdata* pdata) {
    ScreenObj* object;
    ScreenObj* resolved;

    object = pdata->screen_obj;
    if (object != 0) {
        if (object->instance == pdata->screen_instance) {
            resolved = object;
        } else {
            resolved = 0;
        }
    } else {
        resolved = 0;
    }
    return resolved;
}

static float p_fade_screen(void) {
    FadeScreenPdata* pdata;
    ScreenObj* obj;
    float volume_step;
    float volume;
    int next_alpha;
    int done;
    int branch_done;
    int i;
    unsigned char alpha;

    if (fade_pause_allows_tick() == 0) {
        return kFadeSleepTick;
    }

    pdata = (FadeScreenPdata*)apdata;
    if (pdata == 0) {
        return kFadeDoneTick;
    }

    obj = resolve_fade_screen_object(pdata);
    if (obj == 0) {
        return kFadeDoneTick;
    }

    if (pdata->to_fade != 0) {
        volume_step = 1.0f / (255.0f / (float)pdata->frames);
        next_alpha = (int)pdata->alpha + (pdata->frames & 0xFF);
        if (next_alpha > 0xFF) {
            pdata->alpha = 0xFF;
        } else {
            pdata->alpha = (unsigned char)next_alpha;
        }

        obj = resolve_fade_screen_object(pdata);

        if (obj != 0) {
            alpha = pdata->alpha;
            for (i = 0; i < 4; i++) {
                obj->pfx2d->verts[i].a = alpha;
            }
            obj->pfx2d->mirror = 1;

            if (pdata->audio_flag != 0) {
                volume = snd_get_game_vol() - volume_step;
                if (volume > 0.0f) {
                    snd_set_game_vol(volume);
                } else {
                    snd_set_game_vol(0.0f);
                }
            }
            if (pdata->alpha == 0xFF) {
                branch_done = 1;
            } else {
                branch_done = 0;
            }
        } else {
            branch_done = 1;
        }
        done = branch_done;
    } else {
        volume_step = 1.0f / (255.0f / (float)pdata->frames);
        next_alpha = (int)pdata->alpha - (pdata->frames & 0xFF);
        if (next_alpha < 0) {
            pdata->alpha = 0;
        } else {
            pdata->alpha = (unsigned char)next_alpha;
        }

        obj = resolve_fade_screen_object(pdata);

        if (obj != 0) {
            alpha = pdata->alpha;
            for (i = 0; i < 4; i++) {
                obj->pfx2d->verts[i].a = alpha;
            }
            obj->pfx2d->mirror = 1;

            if (pdata->audio_flag != 0) {
                volume = snd_get_game_vol();
                volume += volume_step;
                if (volume < game_volume) {
                    snd_set_game_vol(volume);
                } else {
                    snd_set_game_vol(game_volume);
                }
            }
            if (pdata->alpha == 0) {
                branch_done = 1;
            } else {
                branch_done = 0;
            }
        } else {
            branch_done = 1;
        }
        done = branch_done;

        if (done != 0) {
            obj = resolve_fade_screen_object(pdata);
            if (obj != 0 && obj->instance != 0) {
                obj->typed_vtbl->destroy(obj);
            }
        }
    }

    return done != 0 ? kFadeDoneTick : kFadeSleepTick;
}

/* Keep out-of-line so fade_from / fade_to wrappers match retail bl. */
#if !defined(TARGET_PC)
#pragma dont_inline on
#endif
static void fade_screen(int frames, int color, int flag, int to_fade) {
    FadeScreenPdata* pdata;
    MkProc* proc;
    ScreenObj* obj;
    float volume_step;
    float volume;
    int next_alpha;
    int scaled_frames;
    int wait_count;
    int tick_allowed;
    int i;
    unsigned char alpha;

    scaled_frames = (int)((float)frames * inverse_game_speed);
    if (find_mkproc_pid(FADE_PROC_PID) == 0) {
        if (to_fade == 0) {
            destroy_mkprocs_pid(FADE_PROC_PID);
            delete_screen_obj_oid(FADE_BLACK_OID);
            delete_screen_obj_oid(FADE_WHITE_OID);
        }

        proc = _create_mkproc_generic_nostack(
            FADE_PROC_PID, 0x1F, (MkProcEntryFn)p_fade_screen,
            sizeof(FadeScreenPdata), (MkHdr**)&pdata);
        if (proc != 0) {
            pdata->frames = scaled_frames;
            pdata->color = color;
            pdata->audio_flag = flag;
            pdata->to_fade = to_fade;
            pdata->color_r = 0xFF;
            pdata->color_g = 0xFF;
            pdata->color_b = 0xFF;
            if (to_fade != 0) {
                pdata->alpha = 0;
            } else {
                pdata->alpha = 0xFF;
            }

            proc->flags_bits.skip_if_paused = 1;

            if (pdata->color == 1) {
                obj = load_named_2d_pfxobj(
                    0, FADE_WHITE_OID, (char*)STR_WHITE_FADEBOX, 0, 0xD);
            } else {
                obj = load_named_2d_pfxobj(
                    0, FADE_BLACK_OID, (char*)STR_FADEBOX, 0, 0xD);
            }

            if (obj != 0) {
                pdata->screen_obj = obj;
                pdata->screen_instance = obj->instance;
                obj->x = -0x32;
                obj->y = -0x32;
                obj->flag_bits.scaled = 1;
                obj->scale_x = kFadeScaleX;
                obj->scale_y = kFadeScaleY;
                obj->priority = 0x13;

                if (pdata->to_fade != 0) {
                    volume_step =
                        1.0f / (255.0f / (float)pdata->frames);
                    next_alpha =
                        (int)pdata->alpha + (pdata->frames & 0xFF);
                    if (next_alpha > 0xFF) {
                        pdata->alpha = 0xFF;
                    } else {
                        pdata->alpha = (unsigned char)next_alpha;
                    }

                    obj = pdata->screen_obj;
                    if (obj != 0) {
                        obj = obj->instance == pdata->screen_instance ? obj : 0;
                    } else {
                        obj = 0;
                    }

                    if (obj != 0) {
                        alpha = pdata->alpha;
                        for (i = 0; i < 4; i++) {
                            obj->pfx2d->verts[i].a = alpha;
                        }
                        obj->pfx2d->mirror = 1;

                        if (pdata->audio_flag != 0) {
                            volume = snd_get_game_vol() - volume_step;
                            if (volume > 0.0f) {
                                snd_set_game_vol(volume);
                            } else {
                                snd_set_game_vol(0.0f);
                            }
                        }
                    }
                } else {
                    volume_step =
                        1.0f / (255.0f / (float)pdata->frames);
                    if ((int)pdata->alpha - (pdata->frames & 0xFF) < 0) {
                        pdata->alpha = 0;
                    } else {
                        pdata->alpha =
                            pdata->alpha - (unsigned char)pdata->frames;
                    }

                    obj = pdata->screen_obj;
                    if (obj != 0) {
                        obj = obj->instance == pdata->screen_instance ? obj : 0;
                    } else {
                        obj = 0;
                    }

                    if (obj != 0) {
                        alpha = pdata->alpha;
                        for (i = 0; i < 4; i++) {
                            obj->pfx2d->verts[i].a = alpha;
                        }
                        obj->pfx2d->mirror = 1;

                        if (pdata->audio_flag != 0) {
                            volume = snd_get_game_vol() + volume_step;
                            if (volume < game_volume) {
                                snd_set_game_vol(volume);
                            } else {
                                snd_set_game_vol(game_volume);
                            }
                        }
                    }
                }
            }
        }

        wait_count = 0x104;
        while (find_mkproc_pid(FADE_PROC_PID) != 0) {
            _mkproc_sleep_ticks = kFadeSleepTick;
            ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();

            if (g_game_info.feature_flags.bits.high_bit == 0 &&
                is_controller_removed() != 0) {
                tick_allowed = 0;
            } else {
                tick_allowed = 1;
            }
            if (tick_allowed != 0) {
                wait_count--;
            }
            if (wait_count < 0) {
                destroy_mkprocs_pid(FADE_PROC_PID);
                delete_screen_obj_oid(FADE_BLACK_OID);
                delete_screen_obj_oid(FADE_WHITE_OID);
                return;
            }
        }
    }
}
#if !defined(TARGET_PC)
#pragma dont_inline reset
#endif

void fade_from_black(int frames, int flag) {
    fade_screen(frames, 0, flag, 0);
}

void fade_from_white(int frames, int flag) {
    fade_screen(frames, 1, flag, 0);
}

void fade_to_black(int frames, int flag) {
    fade_screen(frames, 0, flag, 1);
}

void fade_to_white(int frames, int flag) {
    fade_screen(frames, 1, flag, 1);
}

void set_string_obj_alpha(StringObj* obj, float alpha) {
    float scaled_alpha;

    if (obj != 0) {
        scaled_alpha = 255.0f * alpha;
        if (scaled_alpha < 0.0f) {
            scaled_alpha = 0.0f;
        }
        if (scaled_alpha > 255.0f) {
            scaled_alpha = 255.0f;
        }
        obj->pfx.instance0.rgba[3] = (signed char)scaled_alpha;
    }
}

void set_screen_obj_alpha(ScreenObj* obj, float alpha) {
    float scaled_alpha;
    signed char vertex_alpha;

    if (obj != 0) {
        scaled_alpha = 255.0f * alpha;
        if (scaled_alpha < 0.0f) {
            scaled_alpha = 0.0f;
        }
        if (scaled_alpha > 255.0f) {
            scaled_alpha = 255.0f;
        }
        vertex_alpha = (signed char)scaled_alpha;
        obj->pfx2d->verts[0].a = vertex_alpha;
        obj->pfx2d->verts[1].a = vertex_alpha;
        obj->pfx2d->verts[2].a = vertex_alpha;
        obj->pfx2d->verts[3].a = vertex_alpha;
        obj->pfx2d->mirror = 1;
    }
}

extern double fmod(double x, double y);
extern void MKMatrixSetIdentity(void* m);
/*
 * UV scroll (MatFX) -- 3D material UV animation (bgnd / konquest / gcpipemanager).
 * Mode-select 2D cloud polys use ScreenAnim UV keys, not this path.
 * Soft ceiling: dual/pass/matrix emit vs open-code; p_process coloring; stop.
 */
enum {
    kUvPass1 = 1,
    kUvPass2 = 2,
    kUvMtxDirty = 0x20000,
    kUvMtxFlags = 0x20003,
    kMatFxNone = 0,
    kMatFxDual = 4,
    kMatFxUvTransform = 5,
    kMatFxDualUvTransform = 6
};

/* Open-coded like retail; keep as macros so dual/pass sizes stay near ASM. */
#define UV_WRAP(comp)                                                          \
    do {                                                                       \
        float _v = (comp);                                                     \
        if (_v >= 1.0f) {                                                      \
            (comp) = (float)fmod((double)_v, 1.0);                             \
        } else if (_v <= -1.0f) {                                              \
            (comp) = (float)fmod((double)_v, 1.0);                             \
        }                                                                      \
    } while (0)

#define UV_ADVANCE_PAIR(u, v, rateU, rateV)                                    \
    do {                                                                       \
        (u) = (rateU) * game_speed + (u);                                      \
        (v) = (rateV) * game_speed + (v);                                      \
        UV_WRAP(u);                                                            \
        UV_WRAP(v);                                                            \
    } while (0)

#define UV_CLEAR_DIRTY(mtx)                                                    \
    do {                                                                       \
        unsigned int* _flags = (unsigned int*)&(mtx)[3];                       \
        *_flags = *_flags & ~kUvMtxDirty;                                      \
    } while (0)

static RpMaterial* material_set_uv_scroll_matrix(RpMaterial* material,
                                                 void* matrix);
static RpMaterial* material_set_uv_scroll_matrix_2(RpMaterial* material,
                                                   void* matrix);

#if !defined(TARGET_PC)
#pragma dont_inline on
#endif

static void uv_scroll_dual_pass(UvScrollControl* ctrl) {
    RpAtomic* atomic;
    RpGeometry* geom;
    UV_ADVANCE_PAIR(ctrl->mtx1[12], ctrl->mtx1[13], ctrl->rateU1, ctrl->rateV1);
    UV_CLEAR_DIRTY(ctrl->mtx1);
    UV_ADVANCE_PAIR(ctrl->mtx2[12], ctrl->mtx2[13], ctrl->rateU2, ctrl->rateV2);
    UV_CLEAR_DIRTY(ctrl->mtx2);
    atomic = ctrl->atomic;
    geom = atomic->geometry;
    RpGeometryForAllMaterials(geom, material_set_uv_scroll_matrix, &ctrl->mtx1[0]);
    RpGeometryForAllMaterials(geom, material_set_uv_scroll_matrix_2, &ctrl->mtx2[0]);
}

static void uv_scroll_pass_2(UvScrollControl* ctrl) {
    RpAtomic* atomic;
    RpGeometry* geom;
    UV_ADVANCE_PAIR(ctrl->mtx2[12], ctrl->mtx2[13], ctrl->rateU2, ctrl->rateV2);
    UV_CLEAR_DIRTY(ctrl->mtx2);
    atomic = ctrl->atomic;
    geom = atomic->geometry;
    RpGeometryForAllMaterials(geom, material_set_uv_scroll_matrix_2, &ctrl->mtx2[0]);
}

static void uv_scroll_pass_1(UvScrollControl* ctrl) {
    RpAtomic* atomic;
    RpGeometry* geom;
    UV_ADVANCE_PAIR(ctrl->mtx1[12], ctrl->mtx1[13], ctrl->rateU1, ctrl->rateV1);
    UV_CLEAR_DIRTY(ctrl->mtx1);
    atomic = ctrl->atomic;
    geom = atomic->geometry;
    RpGeometryForAllMaterials(geom, material_set_uv_scroll_matrix, &ctrl->mtx1[0]);
}

#if !defined(TARGET_PC)
#pragma dont_inline off
#endif

static RpMaterial* material_set_uv_scroll_matrix_2(RpMaterial* material,
                                                   void* matrix) {
    RwMatrix* base;
    RwMatrix* dual;
    RpMatFXMaterialGetUVTransformMatrices(material, &dual, &base);
    RpMatFXMaterialSetUVTransformMatrices(material, dual, matrix);
    return material;
}

static RpMaterial* material_set_uv_scroll_matrix(RpMaterial* material,
                                                 void* matrix) {
    RwMatrix* base;
    RwMatrix* dual;
    RpMatFXMaterialGetUVTransformMatrices(material, &dual, &base);
    RpMatFXMaterialSetUVTransformMatrices(material, matrix, base);
    return material;
}

static inline void uv_init_transform_pair(UvScrollControl* ctrl) {
    float one = 1.0f;
    float zero = 0.0f;
    ctrl->mtx1[10] = one;
    ctrl->mtx1[5] = one;
    ctrl->mtx1[0] = one;
    ctrl->mtx1[4] = zero;
    ctrl->mtx1[2] = zero;
    ctrl->mtx1[1] = zero;
    ctrl->mtx1[9] = zero;
    ctrl->mtx1[8] = zero;
    ctrl->mtx1[6] = zero;
    ctrl->mtx1[14] = zero;
    ctrl->mtx1[13] = zero;
    ctrl->mtx1[12] = zero;
    *(unsigned int*)&ctrl->mtx1[3] = *(unsigned int*)&ctrl->mtx1[3] | kUvMtxFlags;
    ctrl->mtx2[10] = one;
    ctrl->mtx2[5] = one;
    ctrl->mtx2[0] = one;
    ctrl->mtx2[4] = zero;
    ctrl->mtx2[2] = zero;
    ctrl->mtx2[1] = zero;
    ctrl->mtx2[9] = zero;
    ctrl->mtx2[8] = zero;
    ctrl->mtx2[6] = zero;
    ctrl->mtx2[14] = zero;
    ctrl->mtx2[13] = zero;
    ctrl->mtx2[12] = zero;
    *(unsigned int*)&ctrl->mtx2[3] = *(unsigned int*)&ctrl->mtx2[3] | kUvMtxFlags;
}

static inline void material_apply_scroll_effects(RpMaterial* material) {
    int effects;
    RwTexture* dual_texture;
    int src;
    int dst;
    effects = RpMatFXMaterialGetEffects(material);
    if (effects == kMatFxDual) {
        dual_texture = RpMatFXMaterialGetDualTexture(material);
        RpMatFXMaterialGetDualBlendModes(material, &src, &dst);
        RpMatFXMaterialSetEffects(material, kMatFxDualUvTransform);
        RpMatFXMaterialSetDualBlendModes(material, src, dst);
        RpMatFXMaterialSetDualTexture(material, dual_texture);
    } else {
        RpMatFXMaterialSetEffects(material, kMatFxUvTransform);
    }
}

static void* material_scroll_uvs_callback(void* mat, void* data) {
    UvScrollControl* ctrl;
    unsigned int flags;
    ctrl = (UvScrollControl*)data;
    flags = ctrl->pass_flags;
    if ((flags & kUvPass1) != 0 && (flags & kUvPass2) != 0) {
        UV_ADVANCE_PAIR(ctrl->mtx1[12], ctrl->mtx1[13], ctrl->rateU1, ctrl->rateV1);
        UV_CLEAR_DIRTY(ctrl->mtx1);
        UV_ADVANCE_PAIR(ctrl->mtx2[12], ctrl->mtx2[13], ctrl->rateU2, ctrl->rateV2);
        UV_CLEAR_DIRTY(ctrl->mtx2);
        material_set_uv_scroll_matrix(mat, &ctrl->mtx1[0]);
        material_set_uv_scroll_matrix_2(mat, &ctrl->mtx2[0]);
    } else if ((flags & kUvPass1) != 0) {
        UV_ADVANCE_PAIR(ctrl->mtx1[12], ctrl->mtx1[13], ctrl->rateU1, ctrl->rateV1);
        UV_CLEAR_DIRTY(ctrl->mtx1);
        material_set_uv_scroll_matrix(mat, &ctrl->mtx1[0]);
    } else if ((flags & kUvPass2) != 0) {
        UV_ADVANCE_PAIR(ctrl->mtx2[12], ctrl->mtx2[13], ctrl->rateU2, ctrl->rateV2);
        UV_CLEAR_DIRTY(ctrl->mtx2);
        material_set_uv_scroll_matrix_2(mat, &ctrl->mtx2[0]);
    }
    return mat;
}

#if !defined(TARGET_PC)
#pragma dont_inline on
#endif
static RpAtomic* atomic_scroll_uvs_callback(RpAtomic* atomic, void* data) {
    UvScrollControl* ctrl;
    unsigned int flags;
    unsigned int bit0;
    ctrl = (UvScrollControl*)data;
    flags = ctrl->pass_flags;
    bit0 = flags & kUvPass1;
    if (bit0 != 0 && (flags & kUvPass2) != 0) {
        uv_scroll_dual_pass(ctrl);
    } else if (bit0 != 0) {
        uv_scroll_pass_1(ctrl);
    } else if ((flags & kUvPass2) != 0) {
        uv_scroll_pass_2(ctrl);
    }
    return atomic;
}
#if !defined(TARGET_PC)
#pragma dont_inline off
#endif

static inline MkObj* resolve_uv_scroll_owner(UvScrollControl* ctrl) {
    MkObj* owner;
    MkObj* resolved;

    owner = ctrl->owner;
    if (owner != 0) {
        if ((unsigned int)owner->hdr.instance == ctrl->owner_instance) {
            resolved = owner;
        } else {
            resolved = 0;
        }
    } else {
        resolved = 0;
    }
    return resolved;
}

UvScrollControl* find_uv_scroll_control_for_obj(MkObj* object) {
    MkPtr* node;
    MkPtr* next;
    UvScrollControl* ctrl;
    MkObj* owner;
    if (MKPTR_LIST_AVAILABLE(&uv_scroll_control_list)) {
        node = uv_scroll_control_list;
        while (node != 0) {
            ctrl = (UvScrollControl*)node->hdr;
            if (node->instance != ctrl->hdr.instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
                continue;
            }
            owner = resolve_uv_scroll_owner(ctrl);
            if (owner == object) {
                return ctrl;
            }
            node = node->next;
        }
    }
    return 0;
}

static float p_process_uvscrolling(void) {
    MkPtr* node;
    MkPtr* next;
    UvScrollControl* ctrl;
    MkObj* owner;
    RpClump* clump;
    if (MKPTR_LIST_AVAILABLE(&uv_scroll_control_list)) {
        node = uv_scroll_control_list;
        while (node != 0) {
            ctrl = (UvScrollControl*)node->hdr;
            if (node->instance != ctrl->hdr.instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
                continue;
            }
            owner = resolve_uv_scroll_owner(ctrl);
            if (owner != 0) {
                if (ctrl->target_is_atomic != 0) {
                    if (ctrl->target != 0) {
                        atomic_scroll_uvs_callback(ctrl->atomic, ctrl);
                    } else {
                        clump = owner->clump;
                        RpClumpForAllAtomics(clump, atomic_scroll_uvs_callback, ctrl);
                    }
                } else {
                    material_scroll_uvs_callback(ctrl->material, ctrl);
                }
            }
            node = node->next;
        }
    }
    return 1.0f;
}

UvScrollControl* material_start_uv_scroll(MkObj* owner, RpMaterial* material,
                                          float u1, float v1, float u2,
                                          float v2) {
    UvScrollControl* ctrl;
    if (material == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    ctrl = (UvScrollControl*)get_mkhdr_generic(0xb0);
    if (ctrl != 0) {
        ctrl->target = 0;
        ctrl->target_is_atomic = 1;
        ctrl->pass_flags = 0;
        ctrl->rateU1 = 0.0f;
        ctrl->rateV1 = 0.0f;
        ctrl->rateU2 = 0.0f;
        ctrl->rateV2 = 0.0f;
        MKMatrixSetIdentity(&ctrl->mtx1[0]);
        MKMatrixSetIdentity(&ctrl->mtx2[0]);
    }
    if (ctrl != 0) {
        ctrl->owner = owner;
        ctrl->owner_instance = owner->hdr.instance;
        mk_insert(&ctrl->hdr, &owner->child_list);
        uv_init_transform_pair(ctrl);
        material_apply_scroll_effects(material);
        ctrl->rateU1 = u1;
        ctrl->rateV1 = v1;
        ctrl->rateU2 = u2;
        ctrl->rateV2 = v2;
        ctrl->pass_flags = 0;
        if (u1 != 0.0f || v1 != 0.0f) {
            ctrl->pass_flags |= kUvPass1;
        }
        if (u2 != 0.0f || v2 != 0.0f) {
            ctrl->pass_flags |= kUvPass2;
        }
        ctrl->material = material;
        ctrl->target_is_atomic = 0;
        mk_insert(&ctrl->hdr, &uv_scroll_control_list);
        return ctrl;
    }
    return 0;
}

UvScrollControl* sobj_start_uv_scroll(MkObj* owner, MkSobj* subobject, float u1,
                                      float v1, float u2, float v2) {
    UvScrollControl* ctrl;
    RpGeometry* geom;
    void* skin;
    int count;
    int i;
    RpMaterial* material;
    int effects;
    RwTexture* dual_texture;
    int src;
    int dst;
    if (owner == 0) {
        return 0;
    }
    ctrl = (UvScrollControl*)get_mkhdr_generic(0xb0);
    if (ctrl != 0) {
        ctrl->target = 0;
        ctrl->target_is_atomic = 1;
        ctrl->pass_flags = 0;
        ctrl->rateU1 = 0.0f;
        ctrl->rateV1 = 0.0f;
        ctrl->rateU2 = 0.0f;
        ctrl->rateV2 = 0.0f;
        MKMatrixSetIdentity(&ctrl->mtx1[0]);
        MKMatrixSetIdentity(&ctrl->mtx2[0]);
    }
    if (ctrl != 0) {
        ctrl->owner = owner;
        ctrl->owner_instance = owner->hdr.instance;
        uv_init_transform_pair(ctrl);
        skin = RpSkinGeometryGetSkin(subobject->atomic->geometry);
        if (skin != 0) {
            RpSkinAtomicSetType(subobject->atomic, 2);
            geom = subobject->atomic->geometry;
            count = geom->matList.numMaterials;
            for (i = 0; i < count; i++) {
                RpMatFXMaterialSetEffects(geom->matList.materials[i],
                                          kMatFxUvTransform);
            }
        } else {
            RpMatFXAtomicEnableEffects(subobject->atomic);
            geom = subobject->atomic->geometry;
            count = geom->matList.numMaterials;
            for (i = 0; i < count; i++) {
                material = geom->matList.materials[i];
                effects = RpMatFXMaterialGetEffects(material);
                if (effects == kMatFxDual) {
                    dual_texture = RpMatFXMaterialGetDualTexture(material);
                    RpMatFXMaterialGetDualBlendModes(material, &src, &dst);
                    RpMatFXMaterialSetEffects(material, kMatFxDualUvTransform);
                    RpMatFXMaterialSetDualBlendModes(material, src, dst);
                    RpMatFXMaterialSetDualTexture(material, dual_texture);
                } else if (effects == kMatFxNone) {
                    RpMatFXMaterialSetEffects(material, kMatFxUvTransform);
                }
            }
        }
        ctrl->rateU1 = u1;
        ctrl->rateV1 = v1;
        ctrl->rateU2 = u2;
        ctrl->rateV2 = v2;
        ctrl->pass_flags = 0;
        if (u1 != 0.0f || v1 != 0.0f) {
            ctrl->pass_flags |= kUvPass1;
        }
        if (u2 != 0.0f || v2 != 0.0f) {
            ctrl->pass_flags |= kUvPass2;
        }
        ctrl->atomic = subobject->atomic;
        ctrl->target_is_atomic = 1;
        mk_insert(&ctrl->hdr, &uv_scroll_control_list);
    } else {
        return 0;
    }
    return ctrl;
}

UvScrollControl* start_sobj_uv_scroll(
    MkObj* owner, int sobj_id, float u1, float v1, float u2, float v2) {
    MkSobj* subobject;
    void* result;

    subobject = (MkSobj*)obj_create_sobjs_by_id(owner, sobj_id);
    if (subobject != 0) {
        result = sobj_start_uv_scroll(owner, subobject, u1, v1, u2, v2);
    } else {
        result = 0;
    }
    return result;
}

AniTextureControl* replace_sobj_texture_with_named_wiff(
    MkSobj* sobj, int handle, const char* texture, const char* wiff) {
    unsigned int art_oid;
    AniTextureControl* result;

    art_oid = get_artid_of_named_item_in_slot(handle, texture, 1);
    if (art_oid != 0) {
        if (sobj != 0) {
            result = attach_wiff_to_atomic_material(
                handle, art_oid, sobj->atomic, (char*)wiff);
        } else {
            result = 0;
        }
    } else {
        result = 0;
    }
    return result;
}

/* Soft ceiling: exact algorithm; ternary abs leaves one fmr plus FPR coloring. */
float sfrand_ab(float a, float b) {
    float high;
    float low;
    float range;
    float scaled;
    unsigned int random_low;
    unsigned int random_value;

    high = a >= b ? a : b;
    low = a <= b ? a : b;
    range = high - low;
    range = range >= 0.0f ? range : -range;
    random_low = (unsigned char)genlrand();
    random_value = ((unsigned char)genlrand() << 8) | random_low;
    scaled = range * ((float)random_value / 65535.0f);
    return low + scaled;
}

/* Soft ceiling: random_percent ~97.73% -- FPR coloring only. */

int random_percent(float percent) {
    unsigned int random_low;
    unsigned int random_value;
    float roll;

    random_low = (unsigned char)genlrand();
    random_value = ((unsigned char)genlrand() << 8) | random_low;
    roll = 100.0f * ((float)random_value / 65535.0f);
    return roll <= percent;
}

/* Soft ceiling: exact algorithm; ternary abs leaves one fmr plus FPR coloring. */

float sfrand(float max) {
    float range;
    float scaled;
    unsigned int random_low;
    unsigned int random_value;

    range = 2.0f * max;
    range = range >= 0.0f ? range : -range;
    random_low = (unsigned char)genlrand();
    random_value = ((unsigned char)genlrand() << 8) | random_low;
    scaled = range * ((float)random_value / 65535.0f);
    return scaled - max;
}

/* Soft ceiling: frand ~99.41% -- one FPR allocation difference. */

float frand(float max) {
    float range;
    unsigned int random_low;
    unsigned int random_value;

    range = max >= 0.0f ? max : -max;
    random_low = (unsigned char)genlrand();
    random_value = ((unsigned char)genlrand() << 8) | random_low;
    return range * ((float)random_value / 65535.0f);
}

/* Soft ceiling: signrand ~97.76% -- GPR coloring and scheduling only. */
int signrand(unsigned short range) {
    char message[80];
    unsigned int first_random;
    unsigned int second_random;
    unsigned int original_range;
    unsigned int limit;
    unsigned int random_value;
    unsigned int result;

    first_random = genlrand();
    second_random = genlrand();
    limit = (unsigned short)(range * 2 + 1);
    random_value = (unsigned char)first_random;
    original_range = (unsigned short)range;
    random_value |= (unsigned char)second_random << 8;
    result = limit * random_value;
    result >>= 16;
    if (limit != 0) {
        if (result >= limit) {
            sprintf(message, STR_RANDU_ERROR_02, limit, result);
            printf(message);
            result = 0;
        }
    } else if (result != limit) {
        sprintf(message, STR_RANDU_ERROR_04, limit, result);
        printf(message);
        result = 0;
    }
    return (unsigned short)result - original_range;
}

/*
 * Exact 16-bit random scaling and validation; 93.64%, retail/local 180/172.
 * Residue is pooled diagnostic-string addressing and multiply-result coloring.
 */
unsigned int randu0(unsigned int max) {
    char message[80];
    unsigned int first_random;
    unsigned int limit;
    unsigned int random_value;
    unsigned int result;

    first_random = genlrand();
    random_value = (unsigned char)first_random |
                   ((unsigned char)genlrand() << 8);
    limit = (unsigned short)max;
    result = limit * random_value;
    result >>= 16;
    if (limit != 0) {
        if (result >= limit) {
            sprintf(message, STR_RANDU_ERROR_02, limit,
                    result);
            printf(message);
            result = 0;
        }
    } else if (result != limit) {
        sprintf(message, STR_RANDU_ERROR_04, limit,
                result);
        printf(message);
        result = 0;
    }
    return (unsigned short)result;
}

unsigned int random(void) {
    return genlrand();
}

int get_mode_of_play(void) {
    return (int)mode_of_play;
}

void set_mode_of_play(int mode) {
    mode_of_play = mode;
}

int player_control_allowed(void) {
    int depth;
    int state;

    depth = game_state_stack_depth;
    if (depth < 0) {
        state = 0;
    } else {
        state = game_state_stack[depth];
    }
    if (state == 7) {
        return ck_eat_online_switches() == 0;
    }
    return 0;
}

void pop_game_state(void) {
    if (game_state_stack_depth < 0) {
        game_state_stack_depth = 0;
    }
    game_state_stack_depth -= 1;
}

void push_game_state(int state) {
    int depth;
    int top;

    depth = game_state_stack_depth;
    if (depth > -1) {
        if (depth < 0) {
            top = 0;
        } else {
            top = game_state_stack[depth];
        }
        if (state == top) {
            return;
        }
    }
    if (depth >= 7) {
        game_state_stack_depth = 7;
    }
    /* Retail: if (state < 0 && state >= 0x1d) return; - impossible; keep fallthrough. */
    if (state < 0) {
        if (state >= 0x1d) {
            return;
        }
    }
    depth = game_state_stack_depth;
    depth += 1;
    game_state_stack_depth = depth;
    game_state_stack[depth] = state;
}

int is_game_state_in_stack(int state) {
    int depth;
    int i;

    depth = game_state_stack_depth;
    for (i = 0; i <= depth; i++) {
        if (game_state_stack[i] == state) {
            return 1;
        }
    }
    return 0;
}

int get_game_state(void) {
    int depth;

    depth = game_state_stack_depth;
    if (depth < 0) {
        return 0;
    }
    return game_state_stack[depth];
}

void reset_game_state(void) {
    game_state_stack_depth = -1;
    memset(game_state_stack, 0, 0x20);
}

void init_global_vars(void) {
    int depth;
    int state;

    if ((unsigned int)global_instance_ctr < 0x80000000U) {
        global_instance_ctr = 0xF0000000;
    }
    apdata_save = 0;

    init_bgnd_info_struct();
    init_game_info_struct();

    depth = game_state_stack_depth;
    state = 0;

    plyr_pdata = 0;
    plyr_obj = 0;
    his_obj = 0;
    his_pdata = 0;
    plyr_anim_proc = 0;
    plyr_anim_pdata = 0;

    p1_freeze_light_item.object = 0;
    p1_freeze_light_item.instance = 0;
    p2_freeze_light_item.object = 0;
    p2_freeze_light_item.instance = 0;
    p1_freeze_proc_item.object = 0;
    p1_freeze_proc_item.instance = 0;
    p2_freeze_proc_item.object = 0;
    p2_freeze_proc_item.instance = 0;
    rope_proc_item.object = 0;
    rope_proc_item.instance = 0;
    sobj_ctrl_proc_item.object = 0;
    sobj_ctrl_proc_item.instance = 0;

    g_game_info.flag_bits.field_bit0 = (unsigned char)state;
    f_fatality_finished = 0;
    f_fatality_available = 0;
    f_fatality_was_done = 0;
    b_game_timer_off = 0;

    if (depth >= 0) {
        state = game_state_stack[depth];
    }

    reset_game_state();
    push_game_state(state);
    turn_controllers_on();
    setDroneOverrideSwitch(0, 0);

    register_c_table(STR_GAME_INFO_TABLE, &g_game_info);
    register_c_table(STR_KONQUEST_HUMAN_BONES, konquest_human_bones);
    register_c_table(STR_FLIPPED_KONQUEST_HUMAN_BONES,
                     &flipped_konquest_human_bones);
    register_c_table(STR_MONK_GROUND_COLLS, monk_ground_colls);
}

unsigned long long stop_usec_timer(int id) {
    unsigned long long now;

    usec_timer_data[id].running = 0;
    now = (unsigned long long)debug_get_usec_timer();
    if (now < usec_timer_data[id].start) {
        usec_timer_data[id].elapsed = usec_timer_data[id].start - now;
    } else {
        usec_timer_data[id].elapsed = now - usec_timer_data[id].start;
    }
    return usec_timer_data[id].elapsed;
}

void start_usec_timer(int id) {
    usec_timer_data[id].start = (unsigned long long)debug_get_usec_timer();
    usec_timer_data[id].elapsed = 0;
    usec_timer_data[id].running = 1;
}

void get_clean_system(void) {
    MkProc* proc = 0;
    int proc_status;
    RwBBox bounds;

    reset_game_speed();
    turn_camera_off();
    turn_fog_off();
    depth_of_field_active = 0;
    use_feedback_effect = 0;
    reset_obstacle_internal_id();
    turn_all_rumble_motors_off();
    turn_all_ports_on();
    turn_switch_log_off();
    destroy_list(&pfx_render_list);
    destroy_list(&pfx_clone_render_list);
    delete_player(0);
    delete_player(1);
    g_game_info.pause_flag_bits.shared_hand_anims_loaded = 0;
    cleanup_player_globals();
    destroy_background_extras();
    movie_player_reset();
    script_system_reset();
    terminate_bgnd_collisions();
    small_ground_fx = 0;
    large_ground_fx = 0;
    stop_ani_texture_control();
    destroy_list(&uv_scroll_control_list);
    destroy_mkprocs_pid(0x2020);
    screen_engine_cleanup();
    destroy_all_mkprocs();
    destroy_fonts();
    delete_light_lists();
    cleanup_minigame_system();
    cleanup_konquest();
    mk_chess_cleanup();
    cleanup_drone_ai();
    cleanup_fight();
    wait_for_display_to_flush();
    display_shutdown();
    purge_delayed_mem_frees();
    RwResourcesEmptyArena();
    RwFreeListPurgeAllFreeLists();
    unload_all_effect_banks();
    term_collision_system();
    mk_system_reset();
    start_ani_texture_control();

    uv_scroll_control_list = 0;
    proc_status = 0;
    proc = get_mkproc_nostack(&proc_status);
    create_mkproc(
        0x10, proc, 0x2020, p_process_uvscrolling, 0);

    menu_init();
    bleed_init();
    init_file_loading_table();
    initialize_background_danger_zones();
    if (msi != 0) {
        mslStopAll(msi);
        mslUnDuckAll(msi);
        snd_set_game_vol(game_volume);
    }
    bgnd_music_ptr1 = 0;
    bgnd_music_ptr2 = 0;
    toggle_normal_2d_rendering(1);
    mcard_msg_active = 0;
    bounds.inf.z = -100.0f;
    bounds.inf.y = -100.0f;
    bounds.inf.x = -100.0f;
    bounds.sup.z = 100.0f;
    bounds.sup.y = 100.0f;
    bounds.sup.x = 100.0f;
    World = RpWorldCreate(&bounds);
    init_collision_system();
    init_camera();
    init_screen_engine();
    pfxscript_initialize();
}

int simple_3d_projectile_collision(
    const Vec* previous_position, const Vec* current_position,
    const Vec* target_position, int mode, float collision_radius_squared,
    float maximum_distance_squared, float close_distance_squared) {
    Vec difference;
    float current_distance_squared;
    float target_distance_squared;

    v3_sub_v3(&difference, target_position, current_position);
    current_distance_squared =
        difference.x * difference.x + difference.z * difference.z;
    if (game_speed > 1.0f) {
        collision_radius_squared *= game_speed * game_speed;
    }
    if (current_distance_squared < collision_radius_squared) {
        return 0;
    }

    v3_sub_v3(&difference, target_position, previous_position);
    target_distance_squared =
        difference.x * difference.x + difference.z * difference.z;
    v3_sub_v3(&difference, previous_position, current_position);
    if (difference.x * difference.x + difference.z * difference.z >
        target_distance_squared) {
        if (mode == 1 &&
            target_distance_squared < close_distance_squared) {
            return 3;
        }
        return 4;
    }
    if (target_distance_squared > maximum_distance_squared) {
        return 2;
    }
    return 1;
}

int is_blind(PlyrPdata* fighter) {
    unsigned int* flags;

    if (fighter != 0) {
        flags = fighter->status_flags;
        if (flags != 0 && (*flags & 8) != 0) {
            return 1;
        }
    }
    return 0;
}

int is_big_boss(PlyrPdata* fighter) {
    unsigned int* flags;

    if (fighter != 0) {
        flags = fighter->status_flags;
        if (flags != 0 && (*flags & 4) != 0) {
            return 1;
        }
    }
    return 0;
}

int has_sidekick(PlyrPdata* fighter) {
    unsigned int* flags;

    if (fighter != 0) {
        flags = fighter->status_flags;
        if (flags != 0 && (*flags & 2) != 0) {
            return 1;
        }
    }
    return 0;
}

int am_i_female(PlyrPdata* fighter) {
    unsigned int* flags;

    if (fighter != 0) {
        flags = fighter->status_flags;
        if (flags != 0 && (*flags & 1) != 0) {
            return 0;
        }
    }
    return 1;
}
