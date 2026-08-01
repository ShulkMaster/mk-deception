#ifndef MOVIE_INFO_H
#define MOVIE_INFO_H

/*
 * movie_info[] (.data:0x80337220, size 0x5A0) - 45 entries (0..0x2C).
 * Indexed by play_movie(movie_id, ...). path/ws_path are Sofdec basenames;
 * play_movie prefixes with /movies/%s (or /kryptmovies/%s for V_/VP_).
 * Disc files live under /moviegc/; pathname_create remaps.
 *
 * Attract / boot (see atm_* in attract.c):
 *   4    MOVIE_ID_INTRO       OPENINGN.SFD / OPENINGW.SFD
 *   0x29 MOVIE_ID_TITLE       TITLEN.SFD   / TITLEW.SFD
 *   0x2A MOVIE_ID_MIDWAY_LOGO LOGON.SFD    / LOGOW.SFD
 *   0x2B MOVIE_ID_QUAD        QUADN.SFD    / QUADW.SFD
 *   0x2C MOVIE_ID_TITLE_ALT   TITLEFN.SFD  / TITLEFW.SFD
 */
#define MOVIE_ID_INTRO 4
#define MOVIE_ID_TITLE 0x29
#define MOVIE_ID_MIDWAY_LOGO 0x2A
#define MOVIE_ID_QUAD 0x2B
#define MOVIE_ID_TITLE_ALT 0x2C
#define MOVIE_ID_MAX 0x2C
#define MOVIE_INFO_COUNT 45

typedef struct MovieInfoEntry {
    const char* path;
    int width;
    int height;
    const char* ws_path;
    int ws_width;
    int ws_height;
    /* 0 = Sofdec fullscreen (Simple_MoviePlayFullScreen); 1 = texture (mkMovieTexPlay) */
    unsigned int play_type;
    /* Passed to mkMovieTexPlay as extra (texture movies); 0 for Sofdec rows. */
    int tex_extra;
} MovieInfoEntry; /* 0x20 */

extern MovieInfoEntry movie_info[MOVIE_INFO_COUNT];

#endif
