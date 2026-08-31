#include "platform/gcutils.h"

#include "dolphin/gx.h"
#include "dolphin/os.h"
#include "dolphin/vi.h"
#include "platform/display.h"
#include "platform/os_types.h"

#define OS_BUS_CLOCK (*(unsigned int*)0x800000F8)

#pragma options align=packed
typedef union GcFilter {
    struct {
        unsigned int word_0;
        unsigned short half_4;
        unsigned char byte_6;
    } packed;
    unsigned char values[7];
} GcFilter;
#pragma options align=reset

extern void pokeFilter(GcFilter* filter);
extern void setup_post_effect_buffers(void);
extern void feedback_effect(void);
extern void VMQuit(void);
extern void turn_rumble_off(int port);
extern int is_progressive_scan_mode(void);
extern char* strcpy(char* destination, const char* source);
extern char* strcat(char* destination, const char* source);

extern int _RwDlPixelFormat;
extern GXRenderModeObj* _RwDlRenderMode;
extern int f_writing_to_memcard;
extern int f_writing_konquest_profile;
extern char pathname_buffer[];

static OSAlarm reset_watch_alarm;
static OSThread reset_thread;
static OSMutex gp_mutex;
static unsigned char reset_handler_stack[0x820];
RwMemoryFunctions mem_funcs;

int feedback_blendrate = 0xE0;
int gc_screen_brightness = 10;

/* These tentative declarations make the internal names visible to early users;
 * their explicit definitions follow reset_watch in retail parse order. */
static int reset_watch_is_running;
static OSThread* game_main_thread;
int gap_08_8051032C_sbss;
static unsigned int ticks_per_msec;
static unsigned int ticks_per_usec;
static int display_offset_save_x;
static int display_offset_save_y;
int display_offset_x;
int display_offset_y;
int old_use_feedback_effect;

void set_texture_mipmap_KL_manual(RwTexture* texture, int k, float l) {
    (void)texture;
    (void)k;
    (void)l;
}

void clear_alpha_channel(void) {}

void gc_enable_alpha_writes(unsigned int enable) {
    GXSetAlphaUpdate(enable);
}

static inline void fill_gamma_filter(GcFilter* filter) {
    const GcFilter default_filter = {0};
    int low = gc_screen_brightness - 2;

    *filter = default_filter;
    filter->values[0] = low;
    filter->values[1] = low;
    filter->values[2] = gc_screen_brightness;
    filter->values[3] = gc_screen_brightness + 2;
    filter->values[4] = gc_screen_brightness;
    filter->values[5] = low;
    filter->values[6] = low;
}

void gc_setup_render_mode(unsigned int pixel_format) {
    static int count;
    GcFilter filter;
    int old_count;

    pixel_format = pixel_format != 0;
    _RwDlPixelFormat = pixel_format;
    old_count = count;
    count = ~old_count;
    GXSetPixelFmt(pixel_format, 0);
    fill_gamma_filter(&filter);
    pokeFilter(&filter);
}

void adjust_gamma(void) {
    GcFilter filter;

    fill_gamma_filter(&filter);
    pokeFilter(&filter);
}

void set_gc_display_props(int brightness) {
    gc_screen_brightness = (brightness - 50) / 6 + 10;
}

void render_post_3D_effect(void) {
    static int bPostEffectBuffersInitialized;

    if (bPostEffectBuffersInitialized == 0) {
        setup_post_effect_buffers();
        bPostEffectBuffersInitialized = 1;
    }
    if (use_feedback_effect != 0) {
        feedback_effect();
    }
    old_use_feedback_effect = use_feedback_effect;
}

int is_widescreen_mode(void) {
    return 0;
}

void render_startup(void) {}

int get_platform_language_setting(void) {
    switch (OSGetLanguage()) {
    case 3:
        return 1;
    case 1:
        return 2;
    case 2:
        return 3;
    case 4:
        return 4;
    default:
        return 0;
    }
}

void gc_stop_reset_watch(void) {
    if (reset_watch_is_running != 0) {
        OSCancelAlarm(&reset_watch_alarm);
        OSCancelThread(&reset_thread);
        reset_watch_is_running = 0;
    }
}

void* do_gamecube_reset(void* argument);
static void reset_watch(OSAlarm* alarm, OSContext* context);

void gc_start_reset_watch(void) {
    if (reset_watch_is_running == 0) {
        unsigned int bus_clock;

        OSInitMutex(&gp_mutex);
        game_main_thread = OSGetCurrentThread();
        OSCreateThread(&reset_thread, do_gamecube_reset, 0,
                       &reset_handler_stack[0x800], 0x800, 0, 1);
        OSCreateAlarm(&reset_watch_alarm);
        bus_clock = OS_BUS_CLOCK >> 2;
        OSSetPeriodicAlarm(&reset_watch_alarm, 0,
                           (bus_clock / 1000) * 16, reset_watch);
        reset_watch_is_running = 1;
    }
}

void gc_release_renderpipe(void) {
    OSUnlockMutex(&gp_mutex);
}

void gc_grab_renderpipe(void) {
    OSLockMutex(&gp_mutex);
}

static void reset_watch(OSAlarm* alarm, OSContext* context) {
    static int must_reset;
    static int f_reset_button_pressed;
    int reset_pressed;

    (void)alarm;
    (void)context;
    reset_pressed = OSGetResetButtonState();
    if (reset_pressed != 0) {
        f_reset_button_pressed = 1;
    }
    if (f_reset_button_pressed != 0 && reset_pressed == 0) {
        must_reset = 1;
    }
    if (must_reset != 0 && f_writing_to_memcard == 0 &&
        f_writing_konquest_profile == 0) {
        OSResumeThread(&reset_thread);
        OSYieldThread();
    }
}

static int gc_must_reset;
int use_feedback_effect = 0;
static int reset_watch_is_running = 0;
static OSThread* game_main_thread = 0;

#define GC_THREAD_OWNER_IS_LIVE(owner) ((owner) != 0 && *(owner) != 0)

void handle_reset_switch(void) {
    if (gc_must_reset != 0) {
        OSLockMutex(&gp_mutex);
        if (GC_THREAD_OWNER_IS_LIVE(&game_main_thread)) {
            OSCancelThread(game_main_thread);
            game_main_thread = 0;
        }
        GXDrawDone();
        GXAbortFrame();
        GXDrawDone();
        VISetBlack(1);
        VIFlush();
        VIWaitForRetrace();
        VMQuit();
        if (reset_watch_is_running != 0) {
            OSCancelAlarm(&reset_watch_alarm);
        }
        turn_rumble_off(0);
        turn_rumble_off(1);
        turn_rumble_off(2);
        turn_rumble_off(3);
        OSResetSystem(0, is_progressive_scan_mode(), 0);
    }
}

const char* get_movie_path(void) {
    return "/moviegc/";
}

void gc_movie_start(void) {
    int pal_mode;
    int format;

    display_offset_save_x = display_offset_x;
    display_offset_save_y = display_offset_y;
    format = VIGetTvFormat();
    switch (format) {
    case 1:
    case 5:
        pal_mode = 1;
        break;
    default:
        pal_mode = 0;
        break;
    }
    if (pal_mode != 0) {
        adjust_display_offset(30, 0, 1);
    } else {
        adjust_display_offset(0, 0, 1);
    }
}

void* do_gamecube_reset(void* argument) {
    int progressive;

    (void)argument;
    OSLockMutex(&gp_mutex);
    if (GC_THREAD_OWNER_IS_LIVE(&game_main_thread)) {
        OSCancelThread(game_main_thread);
        game_main_thread = 0;
    }
    GXDrawDone();
    GXAbortFrame();
    GXDrawDone();
    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    VMQuit();
    if (reset_watch_is_running != 0) {
        OSCancelAlarm(&reset_watch_alarm);
    }
    turn_rumble_off(0);
    turn_rumble_off(1);
    turn_rumble_off(2);
    turn_rumble_off(3);
    progressive = is_progressive_scan_mode();
    /* A successful console reset is operationally non-returning. */
    OSResetSystem(0, progressive, 0);
}

#undef GC_THREAD_OWNER_IS_LIVE

void adjust_display_offset(int x, int y, int reset) {
    int x_origin;
    int format;
    int pal_mode;

    if (reset == 0) {
        display_offset_x += x;
        display_offset_y += y;
        if (display_offset_x > 12) display_offset_x = 12;
        if (display_offset_x < -12) display_offset_x = -12;
        if (display_offset_y > 12) display_offset_y = 12;
        if (display_offset_y < -12) display_offset_y = -12;
    } else {
        display_offset_x = 0;
        display_offset_y = 0;
    }
    if (_RwDlRenderMode != 0) {
        format = VIGetTvFormat();
        switch (format) {
        case 1:
        case 5:
            pal_mode = 1;
            break;
        default:
            pal_mode = 0;
            break;
        }
        x_origin = 40;
        if (pal_mode != 0) {
            x_origin = 30;
        }
        _RwDlRenderMode->viXOrigin = x_origin + display_offset_x;
        if (_RwDlRenderMode != 0) {
            _RwDlRenderMode->viYOrigin = display_offset_y;
        } else {
            return;
        }
        VIConfigure(_RwDlRenderMode);
        VIFlush();
        VIWaitForRetrace();
        VIWaitForRetrace();
    } else {
        return;
    }
}

int refresh_rate(void) {
    int format = VIGetTvFormat();
    int rate;

    switch (format) {
    case 1:
        rate = 50;
        break;
    default:
        rate = 60;
        break;
    }
    return rate;
}

int is_pal_mode(void) {
    int format = VIGetTvFormat();
    int pal_mode;

    switch (format) {
    case 1:
    case 5:
        pal_mode = 1;
        break;
    default:
        pal_mode = 0;
        break;
    }
    return pal_mode;
}

char* pathname_create(const char* path, int prepend_art_path) {
    const char* art_path = "/art/";
    char* cursor;

    if (path != 0) {
        if (prepend_art_path != 0) {
            strcpy(pathname_buffer, art_path);
        }
        if (prepend_art_path != 0) {
            strcat(pathname_buffer, path);
        } else {
            strcpy(pathname_buffer, path);
        }
        cursor = pathname_buffer;
        while (*cursor != '\0') {
            if (*cursor == '\\') {
                *cursor = '/';
            }
            cursor++;
        }
        return pathname_buffer;
    }
    return 0;
}

long long debug_get_usec_timer(void) {
    return (OSGetTime() * 8) /
           ((OS_BUS_CLOCK >> 2) / 125000);
}

long long debug_get_msec_timer(void) {
    return OSGetTime() / ((OS_BUS_CLOCK >> 2) / 1000);
}

void init_debug_timers(void) {
    unsigned int bus_clock = OS_BUS_CLOCK >> 2;
    ticks_per_msec = bus_clock / 1000;
    ticks_per_usec = (bus_clock / 125000) >> 3;
}
