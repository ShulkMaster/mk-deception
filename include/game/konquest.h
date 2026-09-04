#ifndef GAME_KONQUEST_H
#define GAME_KONQUEST_H

#include "math/gxVect.h"
#include "runtime/mk_proc.h"

typedef struct KonquestPuiDefinition KonquestPuiDefinition;
typedef struct KonquestPuiPfxSequenceRow KonquestPuiPfxSequenceRow;
typedef struct KonquestInteriorRoom KonquestInteriorRoom;
typedef struct KonquestRoomObject KonquestRoomObject;
typedef struct KonquestRoomObjectTexture KonquestRoomObjectTexture;
struct KonquestTriggerStruct;
struct KonquestTriggerDefinition;
struct MkObj;

typedef struct KonquestWaypoint {
    Vec position;                /* +0x00 */
    float angle;                 /* +0x0C */
    unsigned int flags;          /* +0x10 */
    int script_function;         /* +0x14 */
} KonquestWaypoint;              /* 0x18 */

typedef struct LipSyncKeyframe {
    float time;
    int frame;
} LipSyncKeyframe;

void konquest_state_init(void);
void cleanup_konquest(void);
void render_konquest_shadows(void);
void set_camera_to_look_at_hero(void);
void show_objective_arrow_and_beam(void);
void hide_objective_arrow_and_beam(void);
void enable_trigger(struct KonquestTriggerDefinition* definition, int state);
void npc_set_anim_proc(MkProcEntryFn entry);
void show_fight_message(int message);
void* get_konquest_tile_objects_obj(void);
void unhide_konquest_object_by_uid(int uid);
void hide_konquest_object_by_uid(int uid);
void pickup_dynamic_pui(KonquestPuiDefinition* item);
void pickup_pui(KonquestPuiDefinition* item);
void spawn_pui(KonquestPuiDefinition* item, int behavior, int position_mode);
void pui_play_pfx_sequence(
    KonquestPuiDefinition* item, int mode,
    KonquestPuiPfxSequenceRow* sequence);
void pui_play_pfx(
    KonquestPuiDefinition* item, int mode, const char* effect_name);
void kill_pui(KonquestPuiDefinition* item);
void transition_hero_to_anim_script(
    int script_id, int transition, float blend, float step);
void konquest_setup_pui_particle(
    const char* effect_name, int shared_render_object);
void hero_start_fx_at_position(
    const char* effect_name, const struct Vec* offset);
void attach_pfx_to_object(
    struct MkObj* object, const char* effect_name, const struct Vec* offset);
struct KonquestTriggerStruct* add_temporary_trigger(
    int id, int type, unsigned int flags, int state,
    unsigned int script_index, float x, float y, float z, float radius);
void trial_add_required_sequence(
    const char* message, const char* message_parameter);
void add_object_to_tile(
    int tile_index, int render_uid, int object_uid, float x, float y, float z,
    float angle);
void start_konquest_interior(
    KonquestInteriorRoom* interior, KonquestRoomObject* script_objects,
    const void** items, int* npc_data,
    KonquestRoomObjectTexture* wall_textures,
    KonquestRoomObjectTexture* floor_textures, int door_bits);

#endif
