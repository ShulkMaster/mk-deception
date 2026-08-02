#ifndef PLATFORM_MAIN_JUMP_H
#define PLATFORM_MAIN_JUMP_H

/*
 * Retail `gamelogic_jump` lives in `platform/main.c` (main.o):
 * reset_game_state -> push_game_state -> system_stack -> create mkproc ->
 * longjmp(exec_loop_jump_buffer, mode) into main's exec loop.
 *
 * GQNE5D Matching keeps the symbol in main.o (do not split for retail link).
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef float (*MainProcEntryFn)(void);

void gamelogic_jump(int mode, MainProcEntryFn entry);

/* Retail default 0xC; set by every gamelogic_jump. */
extern int jump_target_mode;

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MAIN_JUMP_H */
