#ifndef MKD_GAME_MOVESET_H
#define MKD_GAME_MOVESET_H

typedef struct ScreenObj ScreenObj;
typedef struct MkObj MkObj;
typedef struct MkPtr MkPtr;

typedef struct MovesetReflectionOwner {
    char pad00[0x94];
    MkPtr* reflections; /* +0x94 */
} MovesetReflectionOwner;

typedef struct MovesetDefinition {
    unsigned int animation_header; /* +0x00 - copied into runtime style */
    char pad04[8];
    const char* style_sign_name;    /* +0x0C */
    int style_sign_width;           /* +0x10 */
    const char* style_section_name; /* +0x14 */
    const char* animation_section_name; /* +0x18 */
    char pad1C[0x60];
    float camera_distance_offset; /* +0x7C - tightrope camera framing */
} MovesetDefinition;

typedef struct GlobalMoveset {
    unsigned int animation_header;
    MovesetDefinition* definition; /* +0x04 */
    MovesetReflectionOwner* reflection_owner; /* +0x08 */
    MkObj* primary_weapon; /* +0x0C */
    unsigned int primary_weapon_instance; /* +0x10 */
    MkObj* reflection_weapon; /* +0x14 */
    unsigned int reflection_weapon_instance; /* +0x18 */
    char pad1C[8];
    MkObj* secondary_weapon; /* +0x24 */
    unsigned int secondary_weapon_instance; /* +0x28 */
    char pad2C[0x40];
    ScreenObj* style_sign;          /* +0x6C */
    unsigned int style_sign_instance; /* +0x70 */
    unsigned int standing_animation_script; /* +0x74 */
    char pad78[0x134];
} GlobalMoveset; /* 0x1AC */

#define GLOBAL_MOVESET_COUNT 8

extern GlobalMoveset global_movesets[GLOBAL_MOVESET_COUNT];

#endif
