#ifndef GAME_ENDING_H
#define GAME_ENDING_H

#ifdef __cplusplus
extern "C" {
#endif

float p_credits_screen(void);
void credits_add_text(const char* center_text, const char* right_text, int monochrome);
void ending_show_image(int image);
void ending_show_text(int script_index, int duration);
extern float p_champion_screen(void);
void run_ending(int fighter);

#ifdef __cplusplus
}
#endif

#endif
