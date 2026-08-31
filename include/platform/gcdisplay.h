#ifndef PLATFORM_GCDISPLAY_H
#define PLATFORM_GCDISPLAY_H

typedef struct DragonTextPrompt {
    char* message;
    char* yes_str;
    char* no_str;
    unsigned char yes_hi;
} DragonTextPrompt;

void tile_image(unsigned char* dest);
void gc_native_display_init(void);
void gc_native_display_render_image(void);
void gc_native_display_render_text(char* text);
void gc_native_display_render_movie(void* ctx);
void gc_native_display_pass_to_RW(void);
int is_progressive_scan_mode(void);
void feedback_effect(void);
void gc_setup_feedback_buffer_for_konquest(void);
void setup_post_effect_buffers(void);
void pokeFilter(void* vfilter);
int romfont_puts(int x, int y, char* text);

extern unsigned short loading_palette[0x100];
extern unsigned char loading_image[0x10000];

#endif
