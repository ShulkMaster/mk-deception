#include "math/gxVect.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"

typedef struct KonquestInteriorProcVtable {
    void* reserved[9];
    void (*jump_sleep)(MkProcEntryFn entry, MkProc* proc);
} KonquestInteriorProcVtable;

typedef struct KonquestCameraPdata {
    MkHdr hdr;
    char pad08[4];
    Vec position;
    Vec angle;
} KonquestCameraPdata;

typedef struct KonquestInteriorRoom {
    char pad00[8];
    const void** items; /* +0x08 */
    char pad0C[0x24];
    Vec camera_position; /* +0x30 */
    Vec camera_angle;    /* +0x3C */
    char pad48[4];
    int* npc_data;       /* +0x4C */
} KonquestInteriorRoom;

typedef struct KonquestInteriorSaveData {
    MkObj* interior_object;                 /* +0x00 */
    unsigned int interior_object_instance; /* +0x04 */
    Vec hero_position;                     /* +0x08 */
    char pad14[4];
    float hero_angle;                      /* +0x18 */
    char pad1C[4];
    Vec exterior_camera_position;          /* +0x20 */
    Vec exterior_camera_angle;             /* +0x2C */
    void* background_lights;               /* +0x38 */
    void* special_lights;                  /* +0x3C */
    KonquestInteriorRoom* current_interior; /* +0x40 */
    int building_id;                        /* +0x44 */
    int exterior_door_bits;                 /* +0x48 */
    char pad4C[8];
    unsigned int exit_script_index;         /* +0x54 */
    unsigned int entry_script_index;        /* +0x58 */
    char pad5C[0x0C];
} KonquestInteriorSaveData;

typedef struct KonquestInteriorPdata {
    char pad00[0x24];
    ScriptSlot* script_owner; /* +0x24 */
    struct KonquestRegionTable* region_table; /* +0x28 */
    char pad2C[0xCC];
    MkObj* hero_object;                     /* +0xF8 */
    unsigned int hero_instance;             /* +0xFC */
    char pad100[0x7C];
    int tile_width;                         /* +0x17C */
    int tile_height;                        /* +0x180 */
    char pad184[8];
    float camera_offset_x; /* +0x18C */
    char pad190[4];
    float camera_offset_z; /* +0x194 */
} KonquestInteriorPdata;

typedef struct KonquestEnumerationEntry {
    int partner_index;
    int partner_uid;
    int enumeration;
    char pad0C[0x10];
} KonquestEnumerationEntry;

typedef struct KonquestRegionTable {
    char pad00[0x34];
    KonquestEnumerationEntry* enumerations;
} KonquestRegionTable;

typedef struct KonquestChildDefinition {
    char pad00[0x0C];
    int enumeration_index;
} KonquestChildDefinition;

typedef struct KonquestChildObject {
    MkHdr hdr;
    char pad08[8];
    KonquestChildDefinition* definition;
} KonquestChildObject;

typedef struct KonquestNpcRecord {
    char pad00[0x1C];
    unsigned char flags;
} KonquestNpcRecord;

KonquestInteriorSaveData konq_interior_save_data;
extern KonquestInteriorPdata* konquest_pdata;

int is_pui_an_interior_item(const void* pui);
int get_game_state(void);
int get_konquest_game_mode(void);
float p_konq_interior_exit_point(void);
void handle_controller_input(void);
void trigger_update(int force);
void npc_update(int force);
void pui_update(void);
void konquest_transition_object_to_state(
    int object_uid, int enumeration, int state);
void* find_konquest_object_struct_by_uid(int uid);
KonquestChildObject* find_child_subobject_by_enumeration(
    void* object, int enumeration);
KonquestChildObject* find_door_partner_sobj(KonquestChildObject* door);
KonquestCameraPdata* get_pdata_of_camera(void);
void turn_controllers_off(void);
void turn_controllers_on(void);
void suspend_hero_grounding(void);
void restore_hero_grounding(void);
void hero_stop_moving(void);
void suspend_hero_state_process(void);
void resume_hero_state_process(void);
void stop_hero_collisions(void);
void start_hero_collisions(void);
void fade_to_black(int ticks, int flags);
void fade_from_black(int ticks, int flags);
void set_monk_position(
    KonquestInteriorSaveData* save, float x, float y, float z, float angle);
void destroy_list(MkPtr** list);
void remove_fgnd_mkobj(void* object);
void xfer_camera(MkProcEntryFn entry, int immediate);
void set_camera_position(const Vec* position);
void set_camera_angle(const Vec* angle);
void resume_weather_effects(KonquestInteriorSaveData* save);
unsigned int get_row_count_for_table_by_pointer(
    ScriptSlot* script, void* table);
KonquestNpcRecord* find_npc_by_data(int npc_data);
void remove_npc(int npc_data);
void remove_interior_room_objects(void);
void delete_triggers_from_tile(int tile_count, KonquestInteriorPdata* pdata);
void update_tile_grid(void);
void konquest_set_object_to_state(
    int object_uid, int enumeration, int state);
void start_time_passing(void);
void konquest_show_hud(void);
void pop_game_state(int state);
void trigger_update(int force);
void npc_update(int force);
float p_konquest_loop(void);
float p_idle(void);
float konquest_camera_loop(void);

extern MkPtr* bgnd_light_list;
extern MkPtr* special_light_list;

int get_door_enum_from_exterior_door_bits(int door_bits) {
    int door = 1;

    if (door_bits & 1) {
        door = 1;
    } else if (door_bits & 2) {
        door = 2;
    } else if (door_bits & 4) {
        door = 3;
    } else if (door_bits & 8) {
        door = 4;
    } else if (door_bits & 0x10) {
        door = 5;
    } else if (door_bits & 0x20) {
        door = 6;
    } else if (door_bits & 0x40) {
        door = 7;
    } else if (door_bits & 0x80) {
        door = 8;
    } else if (door_bits & 0x100) {
        door = 9;
    } else if (door_bits & 0x200) {
        door = 10;
    } else if (door_bits & 0x400) {
        door = 11;
    } else if (door_bits & 0x800) {
        door = 12;
    }
    return door;
}

float get_ir_cam_ang_x(void) {
    float angle = 0.0f;

    if (get_game_state() == 0x14) {
        angle = konq_interior_save_data.current_interior->camera_angle.x;
    }
    return angle;
}

float get_ir_cam_ang_y(void) {
    float angle = 0.0f;

    if (get_game_state() == 0x14) {
        angle = konq_interior_save_data.current_interior->camera_angle.y;
    }
    return angle;
}

float get_ir_cam_ang_z(void) {
    float angle = 0.0f;

    if (get_game_state() == 0x14) {
        angle = konq_interior_save_data.current_interior->camera_angle.z;
    }
    return angle;
}

float get_ir_cam_pos_x(int include_offset) {
    float position = 0.0f;

    if (get_game_state() == 0x14) {
        position =
            konq_interior_save_data.current_interior->camera_position.x;
        if (include_offset != 0) {
            position += konquest_pdata->camera_offset_x;
        }
    }
    return position;
}

float get_ir_cam_pos_y(void) {
    float position = 0.0f;

    if (get_game_state() == 0x14) {
        position =
            konq_interior_save_data.current_interior->camera_position.y;
    }
    return position;
}

float get_ir_cam_pos_z(int include_offset) {
    float position = 0.0f;

    if (get_game_state() == 0x14) {
        position =
            konq_interior_save_data.current_interior->camera_position.z;
        if (include_offset != 0) {
            position += konquest_pdata->camera_offset_z;
        }
    }
    return position;
}

void interior_exit_button_script(void) {
    MkProc* proc;

    if (get_konquest_game_mode() != 0) {
        return;
    }
    proc = find_mkproc_pid(0x2001);
    if (proc != 0) {
        xfer_proc(proc, p_konq_interior_exit_point);
    }
}

int get_building_id_for_exterior(void) {
    return konq_interior_save_data.building_id;
}

int get_doors_for_exterior(void) {
    return konq_interior_save_data.exterior_door_bits;
}

void get_primary_door_enum_for_exterior(void) {
    get_door_enum_from_exterior_door_bits(
        konq_interior_save_data.exterior_door_bits);
}

void initialize_konquest_interior(void) {
    konq_interior_save_data.interior_object = 0;
    konq_interior_save_data.interior_object_instance = 0;
}

void p_konq_interior_loop(void) {
    if (get_konquest_game_mode() == 0) {
        handle_controller_input();
    }
    trigger_update(0);
    npc_update(0);
    pui_update();
}

void close_exterior_doors(int building_id, int door_bits) {
    int door_enum;
    void* building;
    KonquestChildObject* door;
    KonquestChildObject* partner;
    KonquestChildDefinition* definition;

    door_enum = get_door_enum_from_exterior_door_bits(door_bits);
    konquest_transition_object_to_state(building_id, door_enum, 0);

    building = find_konquest_object_struct_by_uid(building_id);
    if (building == 0) {
        return;
    }
    door = find_child_subobject_by_enumeration(building, door_enum);
    if (door == 0) {
        return;
    }
    definition = door->definition;
    if (definition == 0) {
        return;
    }
    if (konquest_pdata->region_table
            ->enumerations[definition->enumeration_index]
            .partner_uid == -1) {
        return;
    }

    partner = find_door_partner_sobj(door);
    if (partner != 0 && partner->definition != 0) {
        konquest_transition_object_to_state(
            building_id,
            konquest_pdata->region_table
                ->enumerations[partner->definition->enumeration_index]
                .enumeration,
            0);
    }
}

float p_konq_interior_exit_point(void) {
    MkObj* hero = konquest_pdata->hero_object;
    MkObj* interior_object =
        konq_interior_save_data.interior_object;
    KonquestCameraPdata* camera = get_pdata_of_camera();
    int* npc_data;
    unsigned int npc_count;
    unsigned int index;
    int door_enum;
    void* building;
    KonquestChildObject* door;
    KonquestChildObject* partner;

    if (hero != 0 && hero->hdr.instance != konquest_pdata->hero_instance) {
        hero = 0;
    }
    if (interior_object != 0 &&
        interior_object->hdr.instance !=
            konq_interior_save_data.interior_object_instance) {
        interior_object = 0;
    }

    turn_controllers_off();
    suspend_hero_grounding();
    hero_stop_moving();
    suspend_hero_state_process();
    stop_hero_collisions();

    if (konq_interior_save_data.exit_script_index != 0) {
        cmdscript_setup_execution(
            konquest_pdata->script_owner,
            konq_interior_save_data.exit_script_index);
        cmdscript_execute(konquest_pdata->script_owner);
    }
    fade_to_black(4, 0);

    if (hero != 0) {
        set_monk_position(
            &konq_interior_save_data,
            konq_interior_save_data.hero_position.x,
            konq_interior_save_data.hero_position.y,
            konq_interior_save_data.hero_position.z,
            konq_interior_save_data.hero_angle);
        update_mkobj(hero);
    }

    destroy_list(&bgnd_light_list);
    destroy_list(&special_light_list);
    bgnd_light_list =
        (MkPtr*)konq_interior_save_data.background_lights;
    special_light_list =
        (MkPtr*)konq_interior_save_data.special_lights;

    if (interior_object != 0) {
        remove_fgnd_mkobj(interior_object);
    }

    xfer_camera(p_idle, 1);
    set_camera_position(
        &konq_interior_save_data.exterior_camera_position);
    set_camera_angle(&konq_interior_save_data.exterior_camera_angle);
    camera->position = konq_interior_save_data.exterior_camera_position;
    camera->angle = konq_interior_save_data.exterior_camera_angle;
    resume_weather_effects(&konq_interior_save_data);

    npc_data = konq_interior_save_data.current_interior->npc_data;
    if (npc_data != 0) {
        npc_count = get_row_count_for_table_by_pointer(
            konquest_pdata->script_owner, npc_data);
        for (index = 0; index < npc_count; ++index) {
            KonquestNpcRecord* npc = find_npc_by_data(npc_data[index]);

            if (npc != 0) {
                npc->flags &= (unsigned char)~0x40;
                remove_npc(npc_data[index]);
            }
        }
    }

    remove_interior_room_objects();
    delete_triggers_from_tile(
        konquest_pdata->tile_width * konquest_pdata->tile_height,
        konquest_pdata);
    update_tile_grid();

    door_enum = get_door_enum_from_exterior_door_bits(
        konq_interior_save_data.exterior_door_bits);
    konquest_set_object_to_state(
        konq_interior_save_data.building_id, door_enum, 1);

    building = find_konquest_object_struct_by_uid(
        konq_interior_save_data.building_id);
    if (building != 0) {
        door = find_child_subobject_by_enumeration(building, door_enum);
        if (door != 0 && door->definition != 0 &&
            konquest_pdata->region_table
                    ->enumerations[door->definition->enumeration_index]
                    .partner_uid != -1) {
            partner = find_door_partner_sobj(door);
            if (partner != 0 && partner->definition != 0) {
                konquest_set_object_to_state(
                    konq_interior_save_data.building_id,
                    konquest_pdata->region_table
                        ->enumerations[
                            partner->definition->enumeration_index]
                        .enumeration,
                    1);
            }
        }
    }

    start_time_passing();
    konquest_show_hud();
    restore_hero_grounding();
    fade_from_black(4, 0);
    if (konq_interior_save_data.entry_script_index != 0) {
        cmdscript_setup_execution(
            konquest_pdata->script_owner,
            konq_interior_save_data.entry_script_index);
        cmdscript_execute(konquest_pdata->script_owner);
    }
    start_hero_collisions();
    resume_hero_state_process();
    pop_game_state(0x13);
    xfer_camera(konquest_camera_loop, 1);
    turn_controllers_on();
    trigger_update(1);
    npc_update(1);
    ((KonquestInteriorProcVtable*)aproc->vtbl)
        ->jump_sleep(p_konquest_loop, aproc);
    return 0.0f;
}

int is_pui_in_current_interior(const void* pui) {
    const void** items;
    unsigned int count;
    unsigned int offset;

    if (!is_pui_an_interior_item(pui)) {
        return 0;
    }

    items = konq_interior_save_data.current_interior->items;
    if (items != 0 && items != 0) {
        count = get_row_count_for_table_by_pointer(
            konquest_pdata->script_owner, (void*)items);
        offset = 0;
        while (count != 0) {
            if (*(const void**)((char*)items + offset) == pui) {
                return 1;
            }
            offset += sizeof(void*);
            count--;
        }
    }
    return 0;
}
