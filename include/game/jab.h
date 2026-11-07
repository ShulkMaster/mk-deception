#ifndef GAME_JAB_H
#define GAME_JAB_H

typedef struct Vec Vec;

typedef struct MkObj MkObj;
typedef struct LightDef LightDef;

MkObj* jab_spawn_point_light_at_world_pos(LightDef* definition, Vec* position);
void jab_face_obj(MkObj* object, Vec* direction);
void jab_setup_kiss_emitter_obj(MkObj* object);
void obj_scale_over_time(MkObj* object, Vec* target, float ticks);

extern void bulvan_function(int command);
void jab_attach_drink_obj_to_hand(MkObj* drink, float* offset, const Vec* angles);
void jab_start_jade_boomerang_throw( struct JabObjectRef* proc_ref, struct JabObjectRef* boomerang_ref, float unused_parameter);
void jab_stop_dragon_king_shake(void);
void jab_destroy_drink_obj_in_hand(void);
void jab_flash_screen(int color, float intensity, float duration);
void initialize_clone_lights(LightDef** definitions);
void jab_shake_dragon_king(float distance, float speed);
void jab_release_jade_boomerang(struct JabObjectRef* proc_ref);
MkObj* jab_attach_point_light_to_obj_bone(LightDef* definition, MkObj* object, int bone);
extern void sh_spawn_grinder_crush_pfx(void);
extern void sh_set_grinder_speed(float speed);
#endif
