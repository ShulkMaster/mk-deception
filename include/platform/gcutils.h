#ifndef PLATFORM_GCUTILS_H
#define PLATFORM_GCUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

void set_texture_mipmap_KL_manual(void);
void clear_alpha_channel(void);
void gc_enable_alpha_writes(unsigned char enable);
void gc_setup_render_mode(unsigned int pixel_format);
void adjust_gamma(void);
void set_gc_display_props(int brightness);
void render_post_3D_effect(void);
int is_widescreen_mode(void);
void render_startup(void);
int get_platform_language_setting(void);
void gc_stop_reset_watch(void);
void gc_start_reset_watch(void);
void gc_release_renderpipe(void);
void gc_grab_renderpipe(void);
void handle_reset_switch(void);
const char* get_movie_path(void);
void gc_movie_start(void);
void adjust_display_offset(int x, int y, int reset);
int refresh_rate(void);
int is_pal_mode(void);
char* pathname_create(const char* path, int prepend_art_path);
long long debug_get_usec_timer(void);
long long debug_get_msec_timer(void);
void init_debug_timers(void);

extern int feedback_blendrate;
extern int gc_screen_brightness;
extern int use_feedback_effect;
extern int old_use_feedback_effect;
extern int display_offset_x;
extern int display_offset_y;

#ifdef __cplusplus
}
#endif

#endif
