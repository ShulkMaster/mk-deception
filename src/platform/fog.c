#include "platform/fog.h"

#include "platform/display.h"

float fog_color_real[4] = {1.0f, 1.0f, 1.0f, 1.0f};

float fog_density = 1.0f;
float fog_distance = 0.01f;
int fog_type = 1;
static unsigned int fog_color = 0xFFFFFFFF;

/* MWCC emits .sbss in reverse declaration order; pad first so fog_on is at +0. */
int gap_08_8051066C_sbss;
int fog_on;

void update_fog_render_states(void) {
    if (fog_on) {
        set_render_state(0xe, 1);
        set_render_state(0x10, fog_type);
        fog_color = 0xFF000000 | (((int)(255.0f * fog_color_real[0]) & 0xFF) << 16) |
                    (((int)(255.0f * fog_color_real[1]) & 0xFF) << 8) |
                    ((int)(255.0f * fog_color_real[2]) & 0xFF);
        set_render_state(0xf, fog_color);
        Camera->fogPlane = fog_distance;
    } else {
        set_render_state(0xe, 0);
    }
}

void turn_fog_off(void) {
    fog_color_real[0] = 0.0f;
    fog_on = 0;
    fog_color_real[1] = 0.0f;
    fog_color_real[2] = 0.0f;
    fog_color_real[3] = 0.0f;
}

void turn_fog_on(void) {
    fog_on = 1;
}
