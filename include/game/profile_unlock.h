#ifndef GAME_PROFILE_UNLOCK_H
#define GAME_PROFILE_UNLOCK_H

typedef union ProfileUnlockBits64 {
    unsigned long long value;
    unsigned int words[2];
} ProfileUnlockBits64;

typedef struct ProfileUnlockSummary {
    ProfileUnlockBits64 cat1; /* +0x00 */
    ProfileUnlockBits64 cat2; /* +0x08 */
    ProfileUnlockBits64 cat3; /* +0x10 */
    unsigned int cat5; /* +0x18 */
    unsigned int pad1C;
    ProfileUnlockBits64 cat7; /* +0x20 */
    ProfileUnlockBits64 cat8; /* +0x28 */
    ProfileUnlockBits64 pz_chars; /* +0x30 */
    ProfileUnlockBits64 pz_bgnds; /* +0x38 */
} ProfileUnlockSummary;

extern ProfileUnlockSummary gp_data;

#endif
