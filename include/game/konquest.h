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
void vdestroy_trigger_struct(struct KonquestTriggerStruct* trigger);
struct KonquestTriggerDefinition;
struct MkObj;
struct KonquestTime;
void set_current_time(struct KonquestTime* time);

struct KonquestChildDefinition {
    char pad00[0x0C];
    int enumeration_index;
};

struct KonquestStringLatch {
    struct StringObj* object;
    unsigned int instance;
};

struct KonquestRegionAsset {
    const struct MkFileEntry* fight_files;
    const struct MkFileEntry* art_files;
    union {
        const char* script_name;
        const char* map_art_name;
    };
    const char* string_bank;
    const char* hud_art_name;
};

typedef struct KonquestWaypoint {
    Vec position;                /* +0x00 */
    float angle;                 /* +0x0C */
    unsigned int flags;          /* +0x10 */
    unsigned int script_function; /* +0x14 */
} KonquestWaypoint;              /* 0x18 */

typedef struct LipSyncKeyframe {
    float time;
    int frame;
} LipSyncKeyframe;

struct KonquestPuiRuntime;
int should_this_pui_be_saved(const struct KonquestPuiRuntime* pui);
int get_konquest_pui_inventory_bit_index(const struct KonquestPuiRuntime* pui);
void get_konquest_pui_object_pos(
    Vec* position, const struct KonquestPuiRuntime* pui);

int get_num_puis(void);
int get_pui_inventory_bit_index(void* item);

void konquest_state_init(void);
void cleanup_konquest(void);
float p_konquest_switch_4(void);
float p_konquest_switch_3(void);
float p_konquest_switch_1(void);
float p_konquest_switch_R1(void);
float p_konquest_inventory_switch(void);
float p_switch_proc_start(void);
void render_konquest_shadows(void);
void set_camera_to_look_at_hero(void);
void show_objective_arrow_and_beam(void);
void hide_objective_arrow_and_beam(void);
void enable_trigger(struct KonquestTriggerDefinition* definition, int state);
void npc_set_anim_proc(MkProcEntryFn entry);
void show_fight_message(int message);
MkHdr* get_konquest_tile_objects_obj(void);
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

void resume_hero_state_process(void);
void set_monk_position(float x, float y, float z, float angle);
int get_konquest_game_mode(void);
void update_tile_grid(void);
void konquest_show_hud(void);
KonquestWaypoint* get_door_path(int door_id);
void stop_time_passing(void);
void stop_hero_collisions(void);
void hero_stop_moving(void);
void suspend_hero_grounding(void);
void konquest_hide_hud(int mode);
void* get_nth_tile_struct(int index);
void start_time_passing(void);
void start_hero_collisions(void);
void suspend_hero_state_process(void);
void restore_hero_grounding(void);
void nis_wait_for_region_load(void);
int is_character_unlocked_in_profile(int, int);
void set_konquest_weather(int type, int param_a, int param_b);
void fire_trigger(struct KonquestTriggerDefinition* definition);
void player_add_item_to_inventory(KonquestPuiDefinition* item);
void change_monk_age(int);
void display_konquest_title(void);
void set_age_progression(int);
void konquest_start_nis_anims_load(char* first, char* second);
void delete_triggers_from_tile(int tile_index);
void display_time_progression_images(int);
void set_monk_age(int age);
void attach_sound_to_object_by_uid(int, int, float, float, int, int);
void resume_weather_effects(void);
void set_movement_npc(int);
void* find_konquest_object_struct_by_uid(int uid);
void konquest_fade_hud(int fade_in);
void set_konquest_object_render_order_priority_by_uid(int, int);
int is_mini_mission_started(int);
void npc_hide_skip_message(void);
void konquest_teleport_hero_to_location(Vec*);
void set_tile_visibility(int, int);
void konquest_fade_from_black(int ticks, int event);
void set_reference_pui(KonquestPuiDefinition*);
void open_chest_and_unlock_kontent(KonquestPuiDefinition*, int);
void assign_obj_to_trigger(int object_uid, struct KonquestTriggerDefinition* definition);
void player_remove_item_from_inventory(void* item);
void play_beam_advance_sound(int delay);
void konquest_end_npc_nis(void);
int is_mini_mission_active(int);
void konquest_load_interior_art(void);
void remove_collision_volume_on_object(void);
void remove_collision_volume_on_object_with_uid(int);
void start_konquest_ambient_sounds(void);
void disable_konquest_object_zwrite_by_uid(int);
void mini_mission_inactive(int mission);
void konquest_end_npc_interaction(void);
void konquest_hero_portal_in(void);
void attach_wiff_to_konquest_object_by_uid(int, char*, float);
void calc_print_speed_for_nis_dialog(MkProc* dialog, unsigned int ticks);
int player_has_item(void* item);
void set_tile_grid_size(int width, int height);
void konquest_nis_end(void);
void stop_chest_camera_script(void);
void konquest_transition_to_fight(int save_progress);
extern int konquest_editor_mode_on;
void konquest_start_npc_nis(void);
int spawn_dynamic_pui(KonquestPuiDefinition* item);
void nis_register_participant(int type, void* npc_data);
void pui_update(void);
void kill_dynamic_pui(KonquestPuiDefinition* item);
int is_mini_mission_completed(int);
void restore_collision_volume_on_object(void);
void pui_set_kill_time(KonquestPuiDefinition*, int, int);
void start_mini_mission(int mission);
void set_interaction_camera_script(void*);
void handle_controller_input(void);
void konquest_open_door(int, int);
void* get_visible_tile_set(int index);
void konquest_camera_return_to_normal(void);
extern struct KonquestRegionAsset konquest_region_data[9];
void kill_konquest_dialog_procs(void);
void add_trigger_list_to_world(void);
void pui_delay_spawn(KonquestPuiDefinition*, float);
void konquest_start_npc_interaction(void);
void give_reward_to_player(KonquestPuiDefinition*);
void load_tile_objects(struct KonquestTileObjectDefinition* definitions);
void wait_for_region_load(void);
int is_game_mode_in_stack(int mode);
void konquest_fade_to_black(int ticks, int event);
int spawn_dynamic_pui_critical(KonquestPuiDefinition* item);
int konquest_nis_anims_loaded(void);
void konquest_run_camera_script(void* script, int flags);
void set_hero_punched_ground_collisions(int punched);
int get_monk_age(void);
void mini_mission_completed(int mission);
void give_krypt_key_to_player(KonquestPuiDefinition*, int);
void npc_play_teleported_sound(void);
void enable_attached_sound_by_uid(int, int);
void konquest_nis_init(int value);
int is_pui_an_interior_item(const char* pui);
void transition_to_region(int region);
void stop_konquest_ambient_sounds(void);
void trigger_set_time_for_enable(struct KonquestTriggerDefinition* definition, int unit, int amount, int action);
void turn_to_face_exterior_door(void);
float p_konquest_loop(void);
void konquest_use_portal(int, Vec*, float, float, float, int);
int get_previous_konquest_region_number(void);
void set_konquest_region_number(int region);
void idle_hero_anim_proc(void);
void nis_remove_non_participants(void);
int check_skip_conversation_flag(void);
void npc_show_skip_message(void);
struct KonquestChildObject* find_door_partner_sobj(struct KonquestChildObject* door);
void save_hero_position_and_angle_prior_to_fight(float angle_offset);
void set_look_at_npc(int);
void restore_collision_volume_on_object_with_uid(int);
int get_hero_state(void);
void trigger_update(int force);
void start_character_separation_process(float);
void konquest_hide_damashi(void);
void set_konquest_object_face_y_by_uid(int);
void start_subobject_pulsing_effect(int pulse_type);
void konquest_set_current_portal_uid(int);
void konquest_transition_object_to_state(int, int, int);
void set_hero_position_relative_to_chest(void);
void pause_weather_effects(void);
void get_current_time(void* value);
extern float p_konquest_mode(void);
void* get_konquest_region_table(void);
extern void konquest_transition_from_fight(void);
#endif
