#ifndef GAME_KONQUEST_H
#define GAME_KONQUEST_H

typedef struct LipSyncKeyframe {
    float time;
    int frame;
} LipSyncKeyframe;

void konquest_state_init(void);
void render_konquest_shadows(void);
void set_camera_to_look_at_hero(void);

#endif
