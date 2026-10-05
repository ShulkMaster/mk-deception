#ifndef GAME_CONTROLLER_TYPES_H
#define GAME_CONTROLLER_TYPES_H

typedef float (*SwitchMapProcFn)(void);
typedef struct SwitchMapEntry {
    unsigned int mask;
    SwitchMapProcFn proc_fn;
    const char* label;
} SwitchMapEntry;

#endif
