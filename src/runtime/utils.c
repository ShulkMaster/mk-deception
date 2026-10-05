#include "game/ground_fx.h"
#include "runtime/utils.h"
#include "game/fx.h"
#include "game/bgnd_jmt.h"
#include "game/ai.h"
#include "game/bgnd.h"
#include "mw/mwScreenEngineGlue.h"
#include "msl/mslcore.h"
#include "runtime/mk_particle.h"
#include "platform/display_metrics.h"
#include "game/mcardmsg.h"
#include "runtime/sound_data.h"
#include "runtime/cmath.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"
#include "runtime/mtRand2.h"
#include "platform/gcutils.h"
#include "movie/MovieManager.h"
#include "mw/mwFile.h"
#include "game/konquest.h"
#include "game/mk_chess.h"
#include "game/minigames.h"

#include "game/game_info.h"
#include "game/game.h"
#include "game/ladder.h"
#include "game/menu.h"
#include "game/plyr.h"
#include "game/plyr_globals.h"
#include "game/specular.h"
#include "game/controller.h"
#include "game/switch.h"
#include "game/memcard.h"
#include "game/nbc.h"
#include "game/plyrprofile.h"
#include "game/settings.h"
#include "movie/movie_info.h"
#include "movie/MkMovies.h"
#include "mw/mwMemHeap.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
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
#include "runtime/sound_bank.h"
#include "rw/rpmatfx.h"
#include "rw/rpskin.h"
#include "rw/rwfreelist.h"
#include "rw/rwresources.h"

void limb_sever_reset_limbs(PlyrInfo* player);

extern int p1_profile_status;
extern int p2_profile_status;
extern int p1_profile_device;
extern int p2_profile_device;
extern int p1_profile_slot;
extern int p2_profile_slot;
extern int p1_rumble_on;
extern int p2_rumble_on;
extern int mcard_msg_active;
extern MslSoundHandle bgnd_music_ptr1;
extern MslSoundHandle bgnd_music_ptr2;
extern _mslSystem* msi;

extern MkHdr* apdata_save;
extern PlyrPdata* his_pdata;
extern MkProc* plyr_anim_proc;
extern int f_fatality_finished;
extern int f_fatality_available;
extern int f_fatality_was_done;
extern int b_game_timer_off;
extern int konquest_human_bones[17];
extern MkFlippedBoneMap flipped_konquest_human_bones;
extern unsigned char monk_ground_colls[];

void init_bgnd_info_struct(void);
void setDroneOverrideSwitch(int activated, void* info);
void reset_obstacle_internal_id(void);
void terminate_bgnd_collisions(void);
void screen_engine_cleanup(void);
void delete_light_lists(void);
void cleanup_fight(void);
void term_collision_system(void);
void bleed_init(void);
void initialize_background_danger_zones(void);
void mslUnDuckAll(_mslSystem* system);
void init_collision_system(void);
void pfxscript_initialize(void);

struct FixedHeapConfigStorage {
    FixedHeapConfig heaps;
    unsigned long field_0x34;
};
typedef char FixedHeapConfigStorageSizeCheck[
    sizeof(struct FixedHeapConfigStorage) == 0x38 ? 1 : -1];

static struct FixedHeapConfigStorage current_heap_block_counts;
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

struct LoadProfilePdata {
    MkHdr hdr;
    int player;
    int port;
    unsigned char* code;
};

struct ProfileCommonRumble {
    char pad00[0xFC];
    int rumble;
};

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

int screen_engine_movie_table[6] = {0, 1, 2, 3, 0x2A, 0x29};

MovieInfoEntry movie_info[MOVIE_INFO_COUNT] = {
    { MOVIE_NAME(0x000), 256, 256, 0, 0, 0, 1, 1 },
    { MOVIE_NAME(0x00B), 256, 256, 0, 0, 0, 1, 1 },
    { MOVIE_NAME(0x015), 256, 256, 0, 0, 0, 1, 1 },
    { MOVIE_NAME(0x020), 256, 256, 0, 0, 0, 1, 1 },
    { MOVIE_NAME(0x02D), 640, 400, MOVIE_NAME(0x03A), 736, 320, 0, 0 },
    { MOVIE_NAME(0x047), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x054), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x061), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x06E), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x07B), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x088), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x095), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0A2), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0AF), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0BC), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0C9), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0D6), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0E3), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0F0), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x0FD), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x108), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x113), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x11E), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x129), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x134), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x13F), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x14A), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x155), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x161), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x16D), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x17A), 640, 480, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x187), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x194), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1A1), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1AD), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1BA), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1C7), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1D4), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1E1), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1EE), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x1FB), 640, 400, 0, 0, 0, 0, 0 },
    { MOVIE_NAME(0x208), 640, 480, MOVIE_NAME(0x213), 704, 480, 0, 0 },
    { MOVIE_NAME(0x21E), 640, 480, MOVIE_NAME(0x228), 704, 480, 0, 0 },
    { MOVIE_NAME(0x232), 640, 480, MOVIE_NAME(0x23C), 704, 480, 0, 0 },
    { MOVIE_NAME(0x246), 640, 480, MOVIE_NAME(0x252), 704, 480, 0, 0 },
};

struct FadeScreenPdata {
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
};

struct BlinkCursorPdata {
    MkHdr hdr;
    ScreenObj* obj;
    int on_ticks;
    int off_ticks;
};

static const float kBlinkDoneTick = -1.0f;

static float p_blink_cursor(void);
static void show_or_hide_2dobj(MkHdr* hdr);

struct DebugDamagePdata {
    MkHdr hdr;
    StringObj* object;
    unsigned int object_instance;
    int direction;
    int alpha;
    int delay;
};

double __fabs(double value);

static float p_debug_damage_txt(void) {
    struct DebugDamagePdata* pdata;
    StringObj* object;
    PfxFontInstance* part;
    unsigned char alpha;

    pdata = (struct DebugDamagePdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    if (pdata->delay > 0) {
        object = MK_LIVE(pdata->object, pdata->object_instance);
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
        object = MK_LIVE(pdata->object, pdata->object_instance);
        if (object != 0) {
            alpha = pdata->alpha;
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

    object = MK_LIVE(pdata->object, pdata->object_instance);
    if (object != 0 && object->instance != 0) {
        object->typed_vtbl->destroy(object);
    }
    return -1.0f;
}

void display_debug_damage(PlyrInfo* player, float damage) {
    struct DebugDamagePdata* pdata;
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
        0x209B, 0, text, screen_position.x,
        screen_position.y, 0x1D);
    if (_create_mkproc_generic_nostack(
            0x209D, 0x1F, p_debug_damage_txt,
            sizeof(struct DebugDamagePdata), (MkHdr**)&pdata) != 0) {
        pdata->object = object;
        pdata->object_instance = object->instance;
        pdata->delay = 60;
        pdata->alpha = 0xFF;
        pdata->direction = player->controller_slot;
    }
}

int printf(const char* fmt, ...);

static inline int fade_pause_allows_tick(void) {
    if (g_game_info.feature_flags.bits.high_bit == 0 &&
        is_controller_removed() != 0) {
        return 0;
    }
    return 1;
}

static inline const char* movie_path_for_id(int movie_id)
{
    const char* path;

    if (movie_id >= 0x2D || movie_id < 0) {
        movie_id = 0;
    }
    if (is_widescreen_mode() != 0) {
        path = movie_info[movie_id].ws_path;
        if (path != 0) {
            return path;
        }
    }
    return movie_info[movie_id].path;
}

int play_movie(int movie_id, MovieTapoutFn tapout_cb) {
    int height;
    int width;
    int extra;
    char buf[0x100];
    const char* open_path;
    _mwFile* file;
    unsigned int play_type;
    const char* path;

    if (is_widescreen_mode() != 0 && movie_info[movie_id].ws_path != 0) {
        height = movie_info[movie_id].ws_height;
        width = movie_info[movie_id].ws_width;
    } else {
        height = movie_info[movie_id].height;
        width = movie_info[movie_id].width;
    }

    extra = movie_info[movie_id].tex_extra;
    path = movie_path_for_id(movie_id);

    play_type = movie_info[movie_id].play_type;
    if (play_type == 1) {
        path = movie_path_for_id(movie_id);
        mkMovieTexPlay(0, path, width, height, extra, 0);
    } else if (play_type == 0) {
        if (strncmp(STR_MOVIE_V_PREFIX, path, 2) == 0 || strncmp(STR_MOVIE_VP_PREFIX, path, 3) == 0) {
            sprintf(buf, STR_KRYPT_MOVIE_PATH, path);
            open_path = pathname_create(buf, 0);
            snprintf(buf, sizeof(buf), STR_KRYPT_MOVIE_WIN, path);
            buf[sizeof(buf) - 1] = 0;
            path = buf;
        } else {
            sprintf(buf, STR_MOVIE_PATH, path);
            open_path = pathname_create(buf, 0);
        }

        file = mwFileOpen(open_path, 0x21);
        if (file != 0) {
            mwFileClose(file);
            Simple_MoviePlayFullScreen(path, width, height, tapout_cb);
            return 1;
        }
        return 0;
    }
    return 0;
}

/* TODO: [near miss] 87.50%; zero tapout argument schedules before movie-ID load. */
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

    if (koin_type < 0 || koin_type >= 6) {
        return;
    }
    if (amount >= 0) {
        profile = player == 0 ? &p1_profile : &p2_profile;
        profile->koins[koin_type] += amount;
        profile->lifetime_koins[koin_type] += amount;
        g_game_info.pselect.field_1ec = g_game_info.pselect.field_1e8;
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

    mode = jump_target_mode;
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

    art_section = get_shared_art_section_for_player(plyr_obj);
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
    object = MK_HDR_LIVE(latch->obj, latch->instance);

    if (object != 0) {
        RpClumpForAllAtomics(
            object->clump, force_specular_texture_atomic_callback, texture);
    }

    latch = &plyr_pdata->mirror_slots->weapon[1].primary;
    object = MK_HDR_LIVE(latch->obj, latch->instance);

    if (object != 0) {
        RpClumpForAllAtomics(
            object->clump, force_specular_texture_atomic_callback, texture);
    }

    latch = &plyr_pdata->aux_weapon_latch;
    object = MK_HDR_LIVE(latch->obj, latch->instance);

    if (object != 0) {
        RpClumpForAllAtomics(
            object->clump, force_specular_texture_atomic_callback, texture);
    }
}

void pause_procs(int flag) {
    g_game_info.pause_flag_bits.controller_disable_guard = flag;
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
    g_game_info.flag_bits.level_fatality_done = state;
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

        if (object->hdr.vtbl == MK_VTABLE_ADDRESS(vtbl_mkobj)) {
            resolved = object;
        } else {
            resolved = 0;
        }
        if (resolved != 0 && resolved->oid == id) {
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

static float p_blink_cursor(void) {
    int on_ticks;
    ScreenObj* live;
    struct BlinkCursorPdata* pdata;
    unsigned int instance;
    int off_ticks;
    ScreenObj* object;

    pdata = (struct BlinkCursorPdata*)apdata;
    if (pdata != 0) {
        object = pdata->obj;
        on_ticks = pdata->on_ticks;
        off_ticks = pdata->off_ticks;
        if (object != 0) {
            instance = object->instance;
        } else {
            return kBlinkDoneTick;
        }
    } else {
        return kBlinkDoneTick;
    }

    for (;;) {
        live = MK_LIVE(object, instance);
        if (live != 0) {
            live->flag_bits.hidden = 0;
            _mkproc_sleep_ticks = on_ticks;
            aproc->vtbl->sleep();
        } else {
            return kBlinkDoneTick;
        }

        live = MK_LIVE(object, instance);
        if (live != 0) {
            live->flag_bits.hidden = 1;
            _mkproc_sleep_ticks = off_ticks;
            aproc->vtbl->sleep();
        } else {
            return kBlinkDoneTick;
        }
    }
}

void blink_cursor(ScreenObj* obj, int proc_id, int on_ticks, int off_ticks) {
    MkProc* proc;
    struct BlinkCursorPdata* pdata;

    proc = _create_mkproc_generic_bigstack(
        proc_id, 0x1F, p_blink_cursor, 0x28, (MkHdr**)&mab_generic_pdata);
    if (proc != 0) {
        pdata = mab_generic_pdata;
        pdata->obj = obj;
        pdata = mab_generic_pdata;
        pdata->on_ticks = on_ticks;
        pdata = mab_generic_pdata;
        pdata->off_ticks = off_ticks;
    } else {
        pdata = mab_generic_pdata;
        pdata->obj = 0;
    }
}

void hide_or_show_2d_obj_by_id(int oid, int hide) {
    set_2dobj_hide_state = hide;
    set_2dobj_oid = oid;
    apply_to_mklist(show_or_hide_2dobj, &screen_obj_list);
}

static void show_or_hide_2dobj(MkHdr* hdr) {
    ScreenObj* screen;
    StringObj* text;

    if (hdr->vtbl == MK_VTABLE_ADDRESS(vtbl_mkpdata_screen_obj)) {
        screen = (ScreenObj*)hdr;
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (screen->oid == set_2dobj_oid) {
            screen->flag_bits.hidden = (set_2dobj_hide_state & 1);
        }
    }

    if (hdr->vtbl == MK_VTABLE_ADDRESS(vtbl_mkpdata_string_obj)) {
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
    ((StringObjVisBits*)&text->flags)->hidden = (set_2dobj_hide_state & 1);
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

/* TODO: [near miss] 97.50%; loop state and parameter homes differ;
 * current/target math and live-instance latch agree. */
void display_numerical_change(
    StringObj* string, int font, int start, int change,
    int ticks, int acceleration_interval) {
    unsigned int instance;
    char text[40];
    int current = start;
    int target = current + change;
    int step = change < 0 ? -1 : 1;
    int tick_count = 0;
    int acceleration_count = 0;

    if (string != 0) {
        instance = string->instance;

        while (current != target) {
            StringObj* live;

            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
            tick_count++;
            if (tick_count >= ticks) {
                int distance;
                int step_magnitude;
                int next;

                tick_count = 0;
                live = MK_LIVE(string, instance);
                if (live == 0) {
                    return;
                }
                distance = target - current >= 0 ? target - current : -(target - current);
                step_magnitude = step < 0 ? -step : step;
                next = current + step;
                if (distance <= step_magnitude) {
                    next = target;
                }
                current = next;
                format_value_to_display(text, current);
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

struct MaterialColorById {
    int id;
    RwRGBA color;
};

RpAtomic* set_atomic_material_color_by_id(
    RpAtomic* atomic, struct MaterialColorById* color_by_id) {
    RpMaterial* material;
    RpGeometry* geometry;
    MkmaterialPluginData* plugin;
    unsigned int count;
    int found;
    unsigned int i;

    geometry = atomic->geometry;
    if (geometry != 0) {
        found = 0;
        count = geometry->matList.numMaterials;
        for (i = 0; i < count; i++) {
            material = geometry->matList.materials[i];
            plugin = MK_MATERIAL_PLUGIN(material);
            if ((plugin->flags & 0xFFF) ==
                (unsigned int)color_by_id->id) {
                found = 1;
                material->color = color_by_id->color;
            }
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
    struct MaterialColorById color_by_id;

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
    struct LoadProfilePdata* pdata;
    int device;
    int slot;
    int scan_state;
    int player;
    unsigned char* code;
    int port;
    StorageProfileSlot* profile;

    scan_state = slot = device = 0;
    pdata = (struct LoadProfilePdata*)apdata;
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
    case 3:
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
    struct LoadProfilePdata* pdata;
    MkProc* proc;

    pdata = 0;
    proc = _create_mkproc_generic_bigstack(
        LOAD_PROFILE_PROC_PID, 0x1F, p_load_profile, sizeof(struct LoadProfilePdata), (MkHdr**)&pdata);
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
                aproc->vtbl->sleep();
            }
            if (p1_profile_load_complete == PROFILE_LOAD_OK) {
                p1_rumble_on = ((struct ProfileCommonRumble*)p1_profile_common)->rumble;
            }
            fire_screen_studio_event(PROFILE_LOAD_EVENT, 1);
            return p1_profile_load_complete;
        }

        while (p2_profile_load_complete == PROFILE_LOAD_PENDING) {
            _mkproc_sleep_ticks = 1.0f;
            aproc->vtbl->sleep();
        }
        if (p2_profile_load_complete == PROFILE_LOAD_OK) {
            p2_rumble_on = ((struct ProfileCommonRumble*)p2_profile_common)->rumble;
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

    if (hdr->vtbl == MK_VTABLE_ADDRESS(vtbl_mkpdata_screen_obj)) {
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

    if (hdr->vtbl == MK_VTABLE_ADDRESS(vtbl_mkpdata_string_obj)) {
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
    set_2dobj_alpha = alpha;
    set_2dobj_oid = id;
    apply_to_mklist(obj_set_alpha_by_id, &screen_obj_list);
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
void pfx_2d_obj_set_alpha(ScreenObj* obj, unsigned char alpha) {
    int i;

    for (i = 0; i < 4; i++) {
        obj->pfx2d->verts[i].a = alpha;
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
    obj = load_2d_pfxobj(0, FADE_BLACK_OID, 0x10017, 0, 0xd);
    if (obj != 0) {
        obj->x = -0x32;
        obj->y = -0x32;
        obj->flag_bits.scaled = 1;
        obj->scale_x = kFadeScaleX;
        obj->scale_y = kFadeScaleY;
    }
}

static inline void publish_fade_screen_alpha(ScreenObj* obj, unsigned char alpha) {
    int i;

    for (i = 0; i < 4; i++) {
        obj->pfx2d->verts[i].a = alpha;
    }
    obj->pfx2d->mirror = 1;
}

static float p_fade_screen(void) {
    struct FadeScreenPdata* pdata;
    ScreenObj* obj;
    float volume_step;
    float volume;
    int next_alpha;
    int done;
    int branch_done;

    if (fade_pause_allows_tick() == 0) {
        return kFadeSleepTick;
    }

    pdata = (struct FadeScreenPdata*)apdata;
    if (pdata == 0) {
        return kFadeDoneTick;
    }

    obj = MK_LIVE(pdata->screen_obj, pdata->screen_instance);
    if (obj == 0) {
        return kFadeDoneTick;
    }

    if (pdata->to_fade != 0) {
        volume_step = 1.0f / (255.0f / (float)pdata->frames);
        next_alpha = (int)pdata->alpha + (pdata->frames & 0xFF);
        if (next_alpha > 0xFF) {
            pdata->alpha = 0xFF;
        } else {
            pdata->alpha = next_alpha;
        }

        obj = MK_LIVE(pdata->screen_obj, pdata->screen_instance);

        if (obj != 0) {
            publish_fade_screen_alpha(obj, pdata->alpha);

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
            pdata->alpha = next_alpha;
        }

        obj = MK_LIVE(pdata->screen_obj, pdata->screen_instance);

        if (obj != 0) {
            publish_fade_screen_alpha(obj, pdata->alpha);

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
            obj = MK_LIVE(pdata->screen_obj, pdata->screen_instance);
            if (obj != 0 && obj->instance != 0) {
                obj->typed_vtbl->destroy(obj);
            }
        }
    }

    return done != 0 ? kFadeDoneTick : kFadeSleepTick;
}

#pragma dont_inline on
/* TODO: [near miss] 98.81%; owner reloads and saved-register homes agree;
 * initial vertex publication retains volatile-register swaps. */
static void fade_screen(int frames, int color, int flag, int to_fade) {
    struct FadeScreenPdata* pdata;
    struct FadeScreenPdata* initial_fade;
    MkProc* proc;
    ScreenObj* obj;
    float volume_step;
    float volume;
    int next_alpha;
    int wait_count;
    int tick_allowed;
    int i;
    unsigned char alpha;

    frames = ((float)frames * inverse_game_speed);
    if (find_mkproc_pid(FADE_PROC_PID) == 0) {
        if (to_fade == 0) {
            destroy_mkprocs_pid(FADE_PROC_PID);
            delete_screen_obj_oid(FADE_BLACK_OID);
            delete_screen_obj_oid(FADE_WHITE_OID);
        }

        proc = _create_mkproc_generic_nostack(
            FADE_PROC_PID, 0x1F, p_fade_screen,
            sizeof(struct FadeScreenPdata), (MkHdr**)&pdata);
        if (proc != 0) {
            pdata->frames = frames;
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

                initial_fade = pdata;
                if (initial_fade->to_fade != 0) {
                    volume_step =
                        1.0f / (255.0f / (float)initial_fade->frames);
                    next_alpha =
                        (int)initial_fade->alpha + (initial_fade->frames & 0xFF);
                    if (next_alpha > 0xFF) {
                        initial_fade->alpha = 0xFF;
                    } else {
                        initial_fade->alpha = next_alpha;
                    }

                    obj = MK_LIVE(initial_fade->screen_obj, initial_fade->screen_instance);

                    if (obj != 0) {
                        alpha = initial_fade->alpha;
                        for (i = 0; i < 4; i++) {
                            obj->pfx2d->verts[i].a = alpha;
                        }
                        obj->pfx2d->mirror = 1;

                        if (initial_fade->audio_flag != 0) {
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
                        1.0f / (255.0f / (float)initial_fade->frames);
                    if ((int)initial_fade->alpha - (initial_fade->frames & 0xFF) < 0) {
                        initial_fade->alpha = 0;
                    } else {
                        initial_fade->alpha =
                            initial_fade->alpha - (unsigned char)initial_fade->frames;
                    }

                    obj = MK_LIVE(initial_fade->screen_obj, initial_fade->screen_instance);

                    if (obj != 0) {
                        alpha = initial_fade->alpha;
                        for (i = 0; i < 4; i++) {
                            obj->pfx2d->verts[i].a = alpha;
                        }
                        obj->pfx2d->mirror = 1;

                        if (initial_fade->audio_flag != 0) {
                            volume = snd_get_game_vol();
                            volume += volume_step;
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
            aproc->vtbl->sleep();

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
#pragma dont_inline reset

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
        vertex_alpha = scaled_alpha;
        obj->pfx2d->verts[0].a = vertex_alpha;
        obj->pfx2d->verts[1].a = vertex_alpha;
        obj->pfx2d->verts[2].a = vertex_alpha;
        obj->pfx2d->verts[3].a = vertex_alpha;
        obj->pfx2d->mirror = 1;
    }
}

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
        unsigned int* _flags = &(mtx).flags;                                   \
        *_flags = *_flags & ~kUvMtxDirty;                                      \
    } while (0)

static RpMaterial* material_set_uv_scroll_matrix(RpMaterial* material,
                                                 void* matrix);
static RpMaterial* material_set_uv_scroll_matrix_2(RpMaterial* material,
                                                   void* matrix);

#pragma dont_inline on

static void uv_scroll_dual_pass(UvScrollControl* ctrl) {
    float rate_v;
    rate_v = ctrl->rateV1;
    ctrl->mtx1.pos.x += ctrl->rateU1 * game_speed;
    ctrl->mtx1.pos.y += rate_v * game_speed;
    UV_WRAP(ctrl->mtx1.pos.x);
    UV_WRAP(ctrl->mtx1.pos.y);
    UV_CLEAR_DIRTY(ctrl->mtx1);
    rate_v = ctrl->rateV2;
    ctrl->mtx2.pos.x += ctrl->rateU2 * game_speed;
    ctrl->mtx2.pos.y += rate_v * game_speed;
    UV_WRAP(ctrl->mtx2.pos.x);
    UV_WRAP(ctrl->mtx2.pos.y);
    UV_CLEAR_DIRTY(ctrl->mtx2);
    RpGeometryForAllMaterials(ctrl->atomic->geometry, material_set_uv_scroll_matrix, &ctrl->mtx1);
    RpGeometryForAllMaterials(ctrl->atomic->geometry, material_set_uv_scroll_matrix_2, &ctrl->mtx2);
}

static void uv_scroll_pass_2(UvScrollControl* ctrl) {
    RpAtomic* atomic;
    RpGeometry* geom;
    float rateV = ctrl->rateV2;
    UV_ADVANCE_PAIR(ctrl->mtx2.pos.x, ctrl->mtx2.pos.y, ctrl->rateU2, rateV);
    UV_CLEAR_DIRTY(ctrl->mtx2);
    atomic = ctrl->atomic;
    geom = atomic->geometry;
    RpGeometryForAllMaterials(geom, material_set_uv_scroll_matrix_2, &ctrl->mtx2);
}

static void uv_scroll_pass_1(UvScrollControl* ctrl) {
    RpAtomic* atomic;
    RpGeometry* geom;
    float rate_v;

    rate_v = ctrl->rateV1;
    ctrl->mtx1.pos.x += ctrl->rateU1 * game_speed;
    ctrl->mtx1.pos.y += rate_v * game_speed;
    UV_WRAP(ctrl->mtx1.pos.x);
    UV_WRAP(ctrl->mtx1.pos.y);
    UV_CLEAR_DIRTY(ctrl->mtx1);
    atomic = ctrl->atomic;
    geom = atomic->geometry;
    RpGeometryForAllMaterials(geom, material_set_uv_scroll_matrix, &ctrl->mtx1);
}

#pragma dont_inline off

static RpMaterial* material_set_uv_scroll_matrix_2(RpMaterial* material,
                                                   void* matrix) {
    RwMatrix* dual;
    RwMatrix* base;
    RpMatFXMaterialGetUVTransformMatrices(material, &dual, &base);
    RpMatFXMaterialSetUVTransformMatrices(material, dual, matrix);
    return material;
}

static RpMaterial* material_set_uv_scroll_matrix(RpMaterial* material,
                                                 void* matrix) {
    RwMatrix* dual;
    RwMatrix* base;
    RpMatFXMaterialGetUVTransformMatrices(material, &dual, &base);
    RpMatFXMaterialSetUVTransformMatrices(material, matrix, base);
    return material;
}

static inline void uv_init_transform_pair(UvScrollControl* ctrl) {
    float one = 1.0f;
    float zero = 0.0f;
    ctrl->mtx1.at.z = one;
    ctrl->mtx1.up.y = one;
    ctrl->mtx1.right.x = one;
    ctrl->mtx1.up.x = zero;
    ctrl->mtx1.right.z = zero;
    ctrl->mtx1.right.y = zero;
    ctrl->mtx1.at.y = zero;
    ctrl->mtx1.at.x = zero;
    ctrl->mtx1.up.z = zero;
    ctrl->mtx1.pos.z = zero;
    ctrl->mtx1.pos.y = zero;
    ctrl->mtx1.pos.x = zero;
    ctrl->mtx1.flags = ctrl->mtx1.flags | kUvMtxFlags;
    ctrl->mtx2.at.z = one;
    ctrl->mtx2.up.y = one;
    ctrl->mtx2.right.x = one;
    ctrl->mtx2.up.x = zero;
    ctrl->mtx2.right.z = zero;
    ctrl->mtx2.right.y = zero;
    ctrl->mtx2.at.y = zero;
    ctrl->mtx2.at.x = zero;
    ctrl->mtx2.up.z = zero;
    ctrl->mtx2.pos.z = zero;
    ctrl->mtx2.pos.y = zero;
    ctrl->mtx2.pos.x = zero;
    ctrl->mtx2.flags = ctrl->mtx2.flags | kUvMtxFlags;
}

static inline void material_apply_scroll_effects(RpMaterial* material) {
    int effects;
    RwBlendFunction dst;
    RwBlendFunction src;
    effects = RpMatFXMaterialGetEffects(material);
    if (effects == kMatFxDual) {
        RwTexture* dual_texture = RpMatFXMaterialGetDualTexture(material);
        RpMatFXMaterialGetDualBlendModes(material, &src, &dst);
        RpMatFXMaterialSetEffects(material, kMatFxDualUvTransform);
        RpMatFXMaterialSetDualBlendModes(material, src, dst);
        RpMatFXMaterialSetDualTexture(material, dual_texture);
    } else {
        RpMatFXMaterialSetEffects(material, kMatFxUvTransform);
    }
}

static inline void material_advance_uv_pair(RwV3d* uv, float rateU, float rateV) {
    uv->x = rateU * game_speed + uv->x;
    uv->y = rateV * game_speed + uv->y;
    UV_WRAP(uv->x);
    UV_WRAP(uv->y);
}

static void* material_scroll_uvs_callback(void* mat, void* data) {
    UvScrollControl* ctrl;
    unsigned int flags;
    unsigned int pass1;
    ctrl = data;
    flags = ctrl->pass_flags;
    pass1 = flags & kUvPass1;
    if (pass1 != 0 && (flags & kUvPass2) != 0) {
        material_advance_uv_pair(&ctrl->mtx1.pos, ctrl->rateU1, ctrl->rateV1);
        UV_CLEAR_DIRTY(ctrl->mtx1);
        material_advance_uv_pair(&ctrl->mtx2.pos, ctrl->rateU2, ctrl->rateV2);
        UV_CLEAR_DIRTY(ctrl->mtx2);
        material_set_uv_scroll_matrix(mat, &ctrl->mtx1);
        material_set_uv_scroll_matrix_2(mat, &ctrl->mtx2);
    } else if (pass1 != 0) {
        material_advance_uv_pair(&ctrl->mtx1.pos, ctrl->rateU1, ctrl->rateV1);
        UV_CLEAR_DIRTY(ctrl->mtx1);
        material_set_uv_scroll_matrix(mat, &ctrl->mtx1);
    } else if ((flags & kUvPass2) != 0) {
        material_advance_uv_pair(&ctrl->mtx2.pos, ctrl->rateU2, ctrl->rateV2);
        UV_CLEAR_DIRTY(ctrl->mtx2);
        material_set_uv_scroll_matrix_2(mat, &ctrl->mtx2);
    }
    return mat;
}

#pragma dont_inline on
static RpAtomic* atomic_scroll_uvs_callback(RpAtomic* atomic, void* data) {
    UvScrollControl* ctrl;
    unsigned int flags;
    unsigned int bit0;
    ctrl = data;
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
#pragma dont_inline off

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
                discard_stale_mkptr(node);
                node = next;
                continue;
            }
            owner = MK_HDR_LIVE(ctrl->owner, ctrl->owner_instance);
            if (owner == object) {
                return ctrl;
            }
            node = node->next;
        }
    }
    return 0;
}

static float p_process_uvscrolling(void) {
    RpClump* clump;
    MkPtr* next;
    UvScrollControl* ctrl;
    MkObj* owner;
    MkPtr* node;
    if (MKPTR_LIST_AVAILABLE(&uv_scroll_control_list)) {
        node = uv_scroll_control_list;
        while (node != 0) {
            ctrl = (UvScrollControl*)node->hdr;
            if (node->instance != ctrl->hdr.instance) {
                next = node->next;
                discard_stale_mkptr(node);
                node = next;
                continue;
            }
            owner = MK_HDR_LIVE(ctrl->owner, ctrl->owner_instance);
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

static inline UvScrollControl* create_uv_scroll_control(void) {
    UvScrollControl* ctrl;
    ctrl = (UvScrollControl*)get_mkhdr_generic(sizeof(UvScrollControl));
    if (ctrl != 0) {
        ctrl->target = 0;
        ctrl->target_is_atomic = 1;
        ctrl->pass_flags = 0;
        ctrl->rateU1 = 0.0f;
        ctrl->rateV1 = 0.0f;
        ctrl->rateU2 = 0.0f;
        ctrl->rateV2 = 0.0f;
        MKMatrixSetIdentity(&ctrl->mtx1);
        MKMatrixSetIdentity(&ctrl->mtx2);
    }
    return ctrl;
}

/* TODO: [near miss] 99.326920%; dual-texture result staging copy remains. */
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
    ctrl = create_uv_scroll_control();
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
        if (u1 || v1) {
            ctrl->pass_flags |= kUvPass1;
        }
        if (u2 || v2) {
            ctrl->pass_flags |= kUvPass2;
        }
        ctrl->material = material;
        ctrl->target_is_atomic = 0;
        mk_insert(&ctrl->hdr, &uv_scroll_control_list);
    } else {
        return 0;
    }
    return ctrl;
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
    ctrl = (UvScrollControl*)get_mkhdr_generic(sizeof(UvScrollControl));
    if (ctrl != 0) {
        ctrl->target = 0;
        ctrl->target_is_atomic = 1;
        ctrl->pass_flags = 0;
        ctrl->rateU1 = 0.0f;
        ctrl->rateV1 = 0.0f;
        ctrl->rateU2 = 0.0f;
        ctrl->rateV2 = 0.0f;
        MKMatrixSetIdentity(&ctrl->mtx1);
        MKMatrixSetIdentity(&ctrl->mtx2);
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
        if (u1 || v1) {
            ctrl->pass_flags |= kUvPass1;
        }
        if (u2 || v2) {
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

    subobject = obj_create_sobjs_by_id(owner, sobj_id);
    if (subobject != 0) {
        result = sobj_start_uv_scroll(owner, subobject, u1, v1, u2, v2);
    } else {
        result = 0;
    }
    return result;
}

AniTextureControl* replace_sobj_texture_with_named_wiff(
    MkSobj* sobj, int handle, const char* texture, const char* wiff) {
    AniTextureControl* result;
    unsigned int art_oid;

    art_oid = get_artid_of_named_item_in_slot(handle, texture, 1);
    switch (art_oid) {
    default:
        if (sobj != 0) {
            result = attach_wiff_to_atomic_material(
                handle, art_oid, sobj->atomic, (char*)wiff);
        } else {
            return 0;
        }
        return result;
    case 0:
        return 0;
    }
}

float sfrand_ab(float a, float b) {
    float high;
    float low;
    float range;
    float fraction;
    float scaled;
    unsigned int random_low;
    unsigned int random_value;
    unsigned int random_high;

    high = a >= b ? a : b;
    low = a <= b ? a : b;
    range = (high - low) >= 0.0f ? high - low : -(high - low);
    random_low = (unsigned char)genlrand();
    random_high = (unsigned char)genlrand() << 8;
    random_value = random_high | random_low;
    fraction = random_value;
    fraction /= 65535.0f;
    scaled = range * fraction;
    return low + scaled;
}

static inline float utils_random_magnitude(float max) {
    float range;
    float fraction;
    unsigned int random_low;
    unsigned int random_value;

    range = max >= 0.0f ? max : -max;
    random_low = (unsigned char)genlrand();
    random_value = (unsigned char)genlrand() << 8;
    random_value = random_value | random_low;
    fraction = random_value;
    fraction /= 65535.0f;
    return range * fraction;
}

int random_percent(float percent) {
    return utils_random_magnitude(1.0f) <= percent;
}

float sfrand(float max) {
    return utils_random_magnitude(2.0f * max) - max;
}

float frand(float max) {
    return utils_random_magnitude(max);
}

int signrand(unsigned short range) {
    char message[80];
    unsigned int first_random;
    unsigned int second_random;
    unsigned short limit;
    unsigned int random_low;
    unsigned int random_value;
    unsigned int result;

    first_random = genlrand();
    second_random = genlrand();
    limit = range * 2 + 1;
    random_low = (unsigned char)first_random;
    random_value = ((unsigned char)second_random << 8) | random_low;
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
    return (unsigned short)result - range;
}

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
    return mode_of_play;
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
    memset(game_state_stack, 0, sizeof(game_state_stack));
}

void init_global_vars(void) {
    int state;

    if ((unsigned int)global_instance_ctr < 0x80000000U) {
        global_instance_ctr = 0xF0000000;
    }
    apdata_save = 0;

    init_bgnd_info_struct();
    init_game_info_struct();

    plyr_pdata = 0;
    plyr_obj = 0;
    his_obj = 0;
    his_pdata = 0;
    plyr_anim_proc = 0;
    plyr_anim_pdata = 0;

    p1_freeze_light_item.obj = 0;
    p1_freeze_light_item.instance = 0;
    p2_freeze_light_item.obj = 0;
    p2_freeze_light_item.instance = 0;
    p1_freeze_proc_item.object = 0;
    p1_freeze_proc_item.instance = 0;
    p2_freeze_proc_item.object = 0;
    p2_freeze_proc_item.instance = 0;
    rope_proc_item.proc = 0;
    rope_proc_item.instance = 0;
    sobj_ctrl_proc_item.proc = 0;
    sobj_ctrl_proc_item.instance = 0;

    g_game_info.flag_bits.field_bit0 = 0;
    f_fatality_finished = 0;
    f_fatality_available = 0;
    f_fatality_was_done = 0;
    b_game_timer_off = 0;

    state = get_game_state();

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
    now = debug_get_usec_timer();
    if (now < usec_timer_data[id].start) {
        usec_timer_data[id].elapsed = usec_timer_data[id].start - now;
    } else {
        usec_timer_data[id].elapsed = now - usec_timer_data[id].start;
    }
    return usec_timer_data[id].elapsed;
}

void start_usec_timer(int id) {
    usec_timer_data[id].start = debug_get_usec_timer();
    usec_timer_data[id].elapsed = 0;
    usec_timer_data[id].running = 1;
}

void get_clean_system(void) {
    MkProc* proc;
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

    proc = get_mkproc_nostack(mkproc_init_flags_none());
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
