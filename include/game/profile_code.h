#ifndef GAME_PROFILE_CODE_H
#define GAME_PROFILE_CODE_H

#include "runtime/mk_struct.h"

/* Shared by pselect PIN entry and plyrprofile multi-match name/icon getters.
 * Retail PID 0x9026/0x9027 pdata: MkHdr, player, controller port, digit count,
 * six PIN bytes and the player state restored after entry. */
typedef struct ProfileCodePdata {
    MkHdr hdr;             /* +0x00 */
    int player;            /* +0x08 */
    int port;              /* +0x0C */
    int count;             /* +0x10 */
    unsigned char code[6]; /* +0x14 */
    char pad1A[2];
    int old_player_state;  /* +0x1C */
} ProfileCodePdata; /* retail 0x20 */

#endif
