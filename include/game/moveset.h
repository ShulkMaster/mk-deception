#ifndef MKD_GAME_MOVESET_H
#define MKD_GAME_MOVESET_H

typedef struct ScreenObj ScreenObj;

typedef struct MovesetDefinition {
    char pad00[0x0C];
    const char* style_sign_name;    /* +0x0C */
    int style_sign_width;           /* +0x10 */
    const char* style_section_name; /* +0x14 */
} MovesetDefinition;

typedef struct GlobalMoveset {
    char pad00[4];
    MovesetDefinition* definition; /* +0x04 */
    char pad08[0x64];
    ScreenObj* style_sign;          /* +0x6C */
    unsigned int style_sign_instance; /* +0x70 */
    char pad74[0x138];
} GlobalMoveset; /* 0x1AC */

#define GLOBAL_MOVESET_COUNT 8

extern GlobalMoveset global_movesets[GLOBAL_MOVESET_COUNT];

#endif
