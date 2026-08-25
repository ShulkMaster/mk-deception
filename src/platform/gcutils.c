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

int use_feedback_effect;
static int reset_watch_is_running;
static OSThread* game_main_thread;
static int gc_must_reset;
static int f_reset_button_pressed;
static int must_reset;
static int post_effect_buffers_initialized;
static int pixel_format_count;
int old_use_feedback_effect;
int display_offset_y;
int display_offset_x;
static int display_offset_save_y;
static int display_offset_save_x;
static unsigned int ticks_per_usec;
static unsigned int ticks_per_msec;
int gap_08_8051032C_sbss;

static const GcFilter default_filter;

void set_texture_mipmap_KL_manual(void) {}

void clear_alpha_channel(void) {}

void gc_enable_alpha_writes(unsigned char enable) {
    GXSetAlphaUpdate(enable);
}

static inline void fill_gamma_filter(GcFilter* filter) {
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
    GcFilter filter;
    int count = pixel_format_count;

    pixel_format = pixel_format != 0;
    _RwDlPixelFormat = pixel_format;
    pixel_format_count = ~count;
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
    if (post_effect_buffers_initialized == 0) {
        setup_post_effect_buffers();
        post_effect_buffers_initialized = 1;
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
    case 2:
        return 3;
    case 4:
        return 4;
    default:
        return 0;
    case 3:
        return 1;
    case 1:
        return 2;
    }
}

void gc_stop_reset_watch(void) {
    if (reset_watch_is_running != 0) {
        OSCancelAlarm(&reset_watch_alarm);
        OSCancelThread(&reset_thread);
        reset_watch_is_running = 0;
    }
}

static void* do_gamecube_reset(void* argument);
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

void handle_reset_switch(void) {
    if (gc_must_reset != 0) {
        OSLockMutex(&gp_mutex);
        if (game_main_thread != 0) {
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
    pal_mode = format == 1 || format == 5;
    if (pal_mode != 0) {
        adjust_display_offset(30, 0, 1);
    } else {
        adjust_display_offset(0, 0, 1);
    }
}

static void* do_gamecube_reset(void* argument) {
    (void)argument;
    OSLockMutex(&gp_mutex);
    if (game_main_thread != 0) {
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
    return 0;
}

void adjust_display_offset(int x, int y, int reset) {
    int x_origin;
    int format;

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
        x_origin = 40;
        format = VIGetTvFormat();
        if (format == 1 || format == 5) {
            x_origin = 30;
        }
        _RwDlRenderMode->viXOrigin = x_origin + display_offset_x;
        if (_RwDlRenderMode != 0) {
            _RwDlRenderMode->viYOrigin = display_offset_y;
            VIConfigure(_RwDlRenderMode);
            VIFlush();
            VIWaitForRetrace();
            VIWaitForRetrace();
        }
    }
}

int refresh_rate(void) {
    return VIGetTvFormat() == 1 ? 50 : 60;
}

int is_pal_mode(void) {
    int format = VIGetTvFormat();
    return format == 1 || format == 5;
}

char* pathname_create(const char* path, int prepend_art_path) {
    char* cursor;

    if (path == 0) {
        return 0;
    }
    if (prepend_art_path != 0) {
        strcpy(pathname_buffer, "/art/");
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

long long debug_get_usec_timer(void) {
    return (OSGetTime() * 8) /
           ((OS_BUS_CLOCK >> 2) / 125000);
}

long long debug_get_msec_timer(void) {
    return OSGetTime() / ((OS_BUS_CLOCK >> 2) / 1000);
}

void init_debug_timers(void) {
    unsigned int bus_clock = OS_BUS_CLOCK >> 2;
    ticks_per_usec = (bus_clock / 125000) >> 3;
    ticks_per_msec = bus_clock / 1000;
}
