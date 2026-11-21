#ifndef FOG_H
#define FOG_H
extern int fog_on;
void update_fog_render_states(void);
void turn_fog_off(void);
void turn_fog_on(void);
extern float fog_color_real[4];
extern int fog_type;
extern float fog_distance;
extern float fog_density;
#endif
