#include "game/game_info.h"
#include "game/konquest.h"
#include "math/gxVect.h"
#include "math/mk_math.h"
#include "runtime/asset.h"
#include "runtime/cam.h"
#include "runtime/light.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/section.h"

typedef struct KonquestInteriorProcVtable {
    void* reserved[6];
    void (*sleep)(void);                                  /* +0x18 */
    void* reserved2[2];
    void (*jump_sleep)(MkProcEntryFn entry, float ticks); /* +0x24 */
} KonquestInteriorProcVtable;

struct KonquestRoomObjectTexture {
    unsigned int material_id; /* +0x00 */
    const char* name;         /* +0x04 */
}; /* 0x08 */

struct KonquestRoomObject {
    unsigned int id;                     /* +0x00 */
    int priority;                        /* +0x04 */
    Vec position;                        /* +0x08 */
    float angle;                         /* +0x14 */
    MkPtr* collision_list;               /* +0x18 */
    KonquestRoomObjectTexture* textures; /* +0x1C */
}; /* 0x20 */

struct KonquestInteriorRoom {
    KonquestRoomObject* room_objects;   /* +0x00 */
    KonquestRoomObject* script_objects; /* +0x04 */
    const void** items;  /* +0x08 */
    Vec trigger_position; /* +0x0C */
    Vec monk_position;    /* +0x18 */
    Vec monk_angle;       /* +0x24 */
    Vec camera_position; /* +0x30 */
    Vec camera_angle;    /* +0x3C */
    LightDef** light_defs; /* +0x48 */
    int* npc_data;       /* +0x4C */
    int field_50;        /* +0x50 */
    int field_54;        /* +0x54 */
    unsigned int exit_script_index;  /* +0x58 */
    unsigned int entry_script_index; /* +0x5C */
}; /* 0x60 */

typedef struct KonquestInteriorSaveData {
    MkObj* interior_object;                 /* +0x00 */
    unsigned int interior_object_instance; /* +0x04 */
    Vec hero_position;                     /* +0x08 */
    Vec hero_angle;                        /* +0x14 */
    Vec exterior_camera_position;          /* +0x20 */
    Vec exterior_camera_angle;             /* +0x2C */
    void* background_lights;               /* +0x38 */
    void* special_lights;                  /* +0x3C */
    KonquestInteriorRoom* current_interior; /* +0x40 */
    int building_id;                        /* +0x44 */
    int exterior_door_bits;                 /* +0x48 */
    unsigned int enter_script_index;        /* +0x4C */
    unsigned int enter_done_script_index;   /* +0x50 */
    unsigned int exit_script_index;         /* +0x54 */
    unsigned int entry_script_index;        /* +0x58 */
    Vec trigger_position;                   /* +0x5C */
} KonquestInteriorSaveData; /* 0x68 */

typedef struct KonquestInteriorPdata {
    char pad00[0x24];
    ScriptSlot* script_owner; /* +0x24 */
    struct KonquestRegionTable* region_table; /* +0x28 */
    char pad2C[0xCC];
    MkObj* hero_object;                     /* +0xF8 */
    unsigned int hero_instance;             /* +0xFC */
    char pad100[0x20];
    int field_120;                          /* +0x120 */
    char pad124[0x58];
    int tile_width;                         /* +0x17C */
    int tile_height;                        /* +0x180 */
    char pad184[8];
    float camera_offset_x; /* +0x18C */
    float camera_offset_y; /* +0x190 */
    float camera_offset_z; /* +0x194 */
} KonquestInteriorPdata;

typedef struct KonquestEnumerationEntry {
    int partner_index;
    int partner_uid;
    int enumeration;
    char pad0C[0x10];
} KonquestEnumerationEntry;

typedef struct KonquestRegionTable {
    char pad00[4];
    const char* interior_art_name; /* +0x04 */
    char pad08[0x2C];
    KonquestEnumerationEntry* enumerations; /* +0x34 */
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

typedef struct KonquestNpcFlags {
    unsigned char bit7 : 1;
    unsigned char in_interior : 1; /* bit6 */
    unsigned char pad : 6;
} KonquestNpcFlags;

typedef struct KonquestNpcRecord {
    char pad00[0x1C];
    union {
        unsigned char flags; /* +0x1C */
        KonquestNpcFlags flags_bits;
    };
    union {
        unsigned char flags_1D; /* +0x1D */
        KonquestNpcFlags flags_1D_bits;
    };
} KonquestNpcRecord;

typedef struct KonquestTile {
    char pad00[8];
    Vec position; /* +0x08 */
} KonquestTile;

typedef struct KonquestTriggerData {
    char pad00[8];
    Vec position; /* +0x08 */
} KonquestTriggerData;

typedef struct KonquestTriggerBuilding {
    char pad00[8];
    int uid; /* +0x08 */
} KonquestTriggerBuilding;

typedef struct KonquestTrigger {
    MkHdr hdr;                          /* +0x00 */
    KonquestTriggerData* data;          /* +0x08 */
    char pad0C[4];
    KonquestTriggerBuilding* building;  /* +0x10 */
} KonquestTrigger;

typedef struct KonquestRoomSobj {
    unsigned int id;
    const char* name;
} KonquestRoomSobj;

/*
 * Retail pools the ten zero-Vec literal templates in reverse function order
 * and shares one triple between two functions (10 objects vs our 13), and
 * orders two @stringBase0 strings ("standard_ir_exit" before "BACKGROUND")
 * against first-use order; neither is reproduced by this compiler invocation
 * from any source ordering tried, so .rodata is layout-off while every other
 * data section is byte-exact.
 */
static KonquestRoomSobj room_sobj_list[] = {
    {0x01, 0},
    {0x05, 0},
    {0x06, 0},
    {0x07, 0},
    {0x08, "INVISI_WALL_1"},
    {0x09, "INVISI_WALL_2"},
    {0x0A, "INVISI_WALL_3"},
    {0x1C, "ROUNDWALL"},
    {0x1D, "LONG_WALL_W_DOOR"},
    {0x1E, "LONG_WALL_W_DOOR"},
    {0x1F, "LONG_WALL_1"},
    {0x20, "LONG_WALL_2"},
    {0x21, "LONG_WALL_3"},
    {0x22, "ROUNDWALL"},
    {0x23, "ROUNDWALL_L"},
    {0x24, "ROUNDWALL_L"},
    {0x25, "COLUMN"},
    {0x32, 0},
    {0x46, "LM_BUILDING"},
    {0x47, "LM_ROUNDWALL"},
    {0x48, "SK_GUARDHOUSE"},
    {0x49, "TK_ROUNDWALL"},
    {0x4A, "LONG_WALL_W_DOOR"},
    {0x4B, 0},
    {0x4D, 0},
    {0x19, "COLUMN"},
    {0x4C, "COLUMN"},
    {0x1B, "COLUMN"},
    {0x3E, "ROUNDWALL_S"},
    {0x3D, "ROUNDWALL_L"},
    {0x3F, "ROUNDWALL_L"},
    {0x40, "KR_TABLE"},
    {0x41, "KR_CHAIR"},
    {0x42, "KR_CHAIR"},
    {0x3C, "LONG_WALL_W_DOOR"},
    {0x2C, "TABLE"},
    {0x16, "CHAIR"},
    {0x17, "CHAIR"},
    {0x18, "CHAIR"},
    {0x1A, 0},
    {0x2E, "COLUMN"},
    {0x51, "TABLE"},
    {0x52, 0},
    {0x53, "BUSH_1"},
    {0x54, "BUSH_1"},
    {0x55, "OTTOMAN_1"},
    {0x56, 0},
    {0x57, "OTTOMAN_1"},
    {0x58, 0},
    {0x59, "COUCH_1"},
    {0x5A, 0},
    {0x5B, "COUCH_1"},
    {0x5C, 0},
    {0x5D, "PEDESTAL_1"},
    {0x5E, "PEDESTAL_1"},
    {0x5F, "TORCH_1"},
    {0x60, 0},
    {0x61, 0},
    {0x62, "TORCH_1"},
    {0x63, 0},
    {0x64, 0},
    {0x65, "HUTINTERIOR"},
    {0x66, "OFFICEINTERIOR"},
    {0x4E, "OW_CHAIR"},
    {0x4F, "OW_CHAIR"},
    {0xAA, "OW_TABLE"},
    {0, 0},
};

unsigned int wall_id_list[] = {
    0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x4A, 0x3C, 0
};

KonquestInteriorSaveData konq_interior_save_data = {0};

unsigned int floor_id_list[] = {1, 0};

static char global_fight_data_table_name[0x80];

extern KonquestInteriorPdata* konquest_pdata;

int is_pui_an_interior_item(const void* pui);
int get_game_state(void);
int get_konquest_game_mode(void);
static int get_door_enum_from_exterior_door_bits(int door_bits);
static float p_konq_interior_exit_point(void);
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
KonquestTrigger* find_trigger_by_id(unsigned int id);
void sobj_swap_material_texture(
    MkSobj* sobj, unsigned int material_id, RwTexture* texture);
void generate_collision_objects(
    int handle, unsigned int art_oid, const Vec* position, const Vec* angles,
    MkPtr** secondary_list);
void set_flag_for_all_collisions(MkPtr** list, unsigned int flags);
void push_game_state(int state);
void stop_time_passing(void);
void pause_weather_effects(void);
void konquest_hide_hud(int mode);
KonquestTile* get_nth_tile_struct(int index);
void add_npc(int npc_data);
float p_konquest_interior_camera_proc(void);
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
void set_monk_position(float x, float y, float z, float angle);
void destroy_list(MkPtr** list);
void remove_fgnd_mkobj(void* object);
void xfer_camera(MkProcEntryFn entry, int immediate);
void resume_weather_effects(KonquestInteriorSaveData* save);
unsigned int get_row_count_for_table_by_pointer(
    ScriptSlot* script, void* table);
KonquestNpcRecord* find_npc_by_data(int npc_data);
void remove_npc(int npc_data);
static void remove_interior_room_objects(void);
void delete_triggers_from_tile(int tile_index);
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

extern MkPtr* special_light_list;

int is_pui_in_current_interior(const void* pui) {
    const void** items;
    unsigned int count;
    unsigned int index;

    if (!is_pui_an_interior_item(pui)) {
        return 0;
    }

    items = konq_interior_save_data.current_interior->items;
    if (items != 0 && items != 0) {
        count = get_row_count_for_table_by_pointer(
            konquest_pdata->script_owner, (void*)items);
        for (index = 0; index < count; ++index) {
            if (items[index] == pui) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 * Soft ceiling: turn_to_face_interior_door ~97.6% -- FPR scratch rotation in
 * the delta/inv-sqrt block and one uncoalesced mr in the hero latch; ops,
 * branches, and size match retail; stop.
 */
void turn_to_face_interior_door(void) {
    Vec delta = {0.0f, 0.0f, 0.0f};
    Vec direction;
    Vec angles;
    union {
        float f;
        unsigned int u;
    } value_bits, guess_bits;
    MkObj* hero;
    KonquestTrigger* trigger;
    float len_sq;
    float xx;
    float yy;
    float zz;
    float guess;
    float product;
    float correction;
    float inv_len;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        hero = (hero->hdr.instance == konquest_pdata->hero_instance)
                   ? hero
                   : 0;
    } else {
        hero = 0;
    }

    trigger = find_trigger_by_id(0x1FE);
    if (trigger == 0) {
        return;
    }

    delta.x = trigger->data->position.x - hero->pos.value.x;
    delta.z = trigger->data->position.z - hero->pos.value.z;
    xx = delta.x * delta.x;
    yy = delta.y * delta.y;
    zz = delta.z * delta.z;
    len_sq = xx + yy;
    len_sq = zz + len_sq;

    if (len_sq <= 0.0f) {
        inv_len = 0.0f;
    } else {
        value_bits.f = len_sq;
        guess_bits.u = 0x5F375A00U - (value_bits.u >> 1);
        guess = guess_bits.f;
        product = guess * (len_sq * guess);
        correction = 3.0f - product;
        inv_len = 0.0625f * guess * correction *
                  -(correction * (product * correction) - 12.0f);
    }

    direction.x = delta.x * inv_len;
    direction.y = delta.y * inv_len;
    direction.z = delta.z * inv_len;
    v3_to_xy_ang(&angles, &direction);
    hero->ang.y = angles.y;
    update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
}

/*
 * Soft ceiling: close_exterior_doors ~98.0% -- retail keeps an unfused
 * "bne +8; b exit" on the definition null test; every honest shape tried
 * fuses it to the inverted beq. 53/55 rows match; stop.
 */
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

int get_primary_door_enum_for_exterior(void) {
    return get_door_enum_from_exterior_door_bits(
        konq_interior_save_data.exterior_door_bits);
}

static int get_door_enum_from_exterior_door_bits(int door_bits) {
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

int get_doors_for_exterior(void) {
    return konq_interior_save_data.exterior_door_bits;
}

int get_building_id_for_exterior(void) {
    return konq_interior_save_data.building_id;
}

/*
 * Soft ceiling: setup_interior_fighting_arena ~96.7% -- nonvolatile register
 * permutation (rec/object/entry homes) with identical operations and
 * structure; declaration-order changes only rotate it; stop.
 */
void setup_interior_fighting_arena(void) {
    KonquestRoomObject* rec;
    KonquestRoomSobj* entry;
    KonquestRoomObjectTexture* tex;
    MkObj* object;

    if (g_game_info.bgnd_obj == 0) {
        return;
    }
    rec = get_data_table_by_name(global_fight_data_table_name);
    if (rec == 0) {
        return;
    }
    obj_create_sobjs(g_game_info.bgnd_obj);

    object = g_game_info.bgnd_obj;
    entry = room_sobj_list;
    if (object != 0) {
        for (; entry->id != 0; entry++) {
            MkSobj* sobj = obj_find_sobj_by_id(object, entry->id);

            if (sobj != 0) {
                hide_sobj(sobj);
            }
        }
    }

    object = g_game_info.bgnd_obj;
    if (object != 0) {
        for (; rec->id != 0; rec++) {
            MkSobj* sobj = obj_find_sobj_by_id(object, rec->id);

            if (sobj == 0) {
                continue;
            }
            unhide_sobj(sobj);
            sobj->pos.x = rec->position.x;
            sobj->pos.y = rec->position.y;
            sobj->pos.z = rec->position.z;
            sobj->ang.z = 0.0f;
            sobj->ang.y = 0.0f;
            sobj->ang.x = 0.0f;
            sobj->ang.y = sobj->ang.y + rec->angle;
            sobj->flags_08_bits.bit7 = 1;
            sobj->flags_08_bits.bit4 = 1;
            tex = rec->textures;
            if (tex != 0) {
                for (; tex->material_id != 0; tex++) {
                    RwTexture* texture =
                        load_named_tga_from_slot(0xA002F, tex->name);

                    if (texture != 0) {
                        sobj_swap_material_texture(
                            sobj, tex->material_id, texture);
                    }
                }
            }
            sobj_set_priority(sobj, rec->priority);
        }
    }
}

float get_ir_cam_ang_z(void) {
    float angle = 0.0f;

    if (get_game_state() == 0x14) {
        angle = konq_interior_save_data.current_interior->camera_angle.z;
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

float get_ir_cam_ang_x(void) {
    float angle = 0.0f;

    if (get_game_state() == 0x14) {
        angle = konq_interior_save_data.current_interior->camera_angle.x;
    }
    return angle;
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

float get_ir_cam_pos_y(void) {
    float position = 0.0f;

    if (get_game_state() == 0x14) {
        position =
            konq_interior_save_data.current_interior->camera_position.y;
    }
    return position;
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

/*
 * Soft ceiling: place_interior_room_objects ~94.3% -- register-class residue:
 * the room_sobj_list cursor initializer lands in r0 with an extra mr, the
 * sunk name=0 stays at source position, and NV homes permute; the structure,
 * calls, and branch shapes match retail. The unguarded tail flag store on the
 * latched (possibly null) object is retail behavior; stop.
 */
static void place_interior_room_objects(KonquestRoomObject* rec) {
    MkObj* interior_object = konq_interior_save_data.interior_object;

    if (interior_object != 0) {
        interior_object =
            (interior_object->hdr.instance ==
             konq_interior_save_data.interior_object_instance)
                ? interior_object
                : 0;
    } else {
        interior_object = 0;
    }

    {
        Vec base_angle = {0.0f, 0.0f, 0.0f};
        Vec position = {0.0f, 0.0f, 0.0f};
        Vec angle = {0.0f, 0.0f, 0.0f};
        KonquestRoomObjectTexture* tex;
        KonquestRoomSobj* entry;
        const char* name;

        if (interior_object != 0) {
            if (rec != 0) {
                for (; rec->id != 0; rec++) {
                    MkSobj* sobj =
                        obj_find_sobj_by_id(interior_object, rec->id);
                    unsigned int artid;

                    if (sobj == 0) {
                        continue;
                    }
                    sobj_set_priority(sobj, rec->priority);
                    tex = rec->textures;
                    if (tex != 0) {
                        for (; tex->material_id != 0; tex++) {
                            RwTexture* texture =
                                load_named_tga_from_slot(0xA002F, tex->name);

                            if (texture != 0) {
                                sobj_swap_material_texture(
                                    sobj, tex->material_id, texture);
                            }
                        }
                    }
                    unhide_sobj(sobj);
                    sobj->pos.x = rec->position.x;
                    sobj->pos.y = rec->position.y;
                    sobj->pos.z = rec->position.z;
                    sobj->ang.z = 0.0f;
                    sobj->ang.y = 0.0f;
                    sobj->ang.x = 0.0f;
                    sobj->ang.y = sobj->ang.y + rec->angle;
                    sobj->flags_08_bits.bit7 = 1;
                    sobj->flags_08_bits.bit4 = 1;
                    position.x =
                        konquest_pdata->camera_offset_x + rec->position.x;
                    position.y =
                        konquest_pdata->camera_offset_y + rec->position.y;
                    position.z =
                        konquest_pdata->camera_offset_z + rec->position.z;
                    angle.x = base_angle.x;
                    angle.y = base_angle.y;
                    angle.z = base_angle.z;
                    angle.y = base_angle.y + rec->angle;
                    rec->collision_list = 0;
                    name = 0;
                    for (entry = room_sobj_list; entry->id != 0; entry++) {
                        if (entry->id == rec->id) {
                            name = entry->name;
                            break;
                        }
                    }
                    if (name != 0) {
                        artid = get_artid_of_named_item_in_slot(
                            0xA002F, (char*)name, 1);
                        generate_collision_objects(
                            0xA002F, artid, &position, &angle,
                            &rec->collision_list);
                        set_flag_for_all_collisions(
                            &rec->collision_list, 0x80000000);
                    }
                }
            }
        }
    }
    interior_object->flags_08_bits.bit7 = 1;
}

/*
 * Soft ceiling: remove_interior_room_objects ~96.8% -- nonvolatile register
 * permutation plus one uncoalesced entry-pointer mr; operations and CFG are
 * retail-exact; stop.
 */
static void remove_interior_room_objects(void) {
    KonquestRoomObject* rec;
    MkObj* interior_object = konq_interior_save_data.interior_object;

    if (interior_object != 0) {
        interior_object =
            (interior_object->hdr.instance ==
             konq_interior_save_data.interior_object_instance)
                ? interior_object
                : 0;
    } else {
        interior_object = 0;
    }

    rec = konq_interior_save_data.current_interior->room_objects;
    if (rec != 0) {
        while (rec->id != 0) {
            if (rec->collision_list != 0) {
                destroy_list(&rec->collision_list);
                rec->collision_list = 0;
            }
            rec++;
        }
    }
    rec = konq_interior_save_data.current_interior->script_objects;
    if (rec != 0) {
        while (rec->id != 0) {
            if (rec->collision_list != 0) {
                destroy_list(&rec->collision_list);
                rec->collision_list = 0;
            }
            rec++;
        }
    }

    if (interior_object != 0) {
        KonquestRoomSobj* entry = room_sobj_list;

        if (interior_object != 0) {
            for (; entry->id != 0; entry++) {
                MkSobj* sobj = obj_find_sobj_by_id(interior_object, entry->id);

                if (sobj != 0) {
                    hide_sobj(sobj);
                }
            }
        }
    }
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

static inline MkObj* interior_live_hero_object(KonquestInteriorPdata* owner) {
    MkObj* object = owner->hero_object;
    if (object != 0) {
        if (object->hdr.instance == owner->hero_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

static inline MkObj* interior_saved_live_interior_object(
    KonquestInteriorSaveData* owner) {
    MkObj* object = owner->interior_object;
    if (object != 0) {
        if (object->hdr.instance == owner->interior_object_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

/* TODO: [near miss] 98.403435%; register coloring, relocation offsets; one-trial ceiling. */
static float p_konq_interior_exit_point(void) {
    MkObj* hero;
    MkObj* interior_object;
    CameraPdata* camera;
    int* npc_data;
    unsigned int npc_count;
    unsigned int index;
    int door_enum;
    void* building;
    KonquestChildObject* door;
    KonquestChildObject* partner;

    hero = interior_live_hero_object(konquest_pdata);

    camera = get_pdata_of_camera();
    interior_object = interior_saved_live_interior_object(&konq_interior_save_data);


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
            konq_interior_save_data.hero_position.x,
            konq_interior_save_data.hero_position.y,
            konq_interior_save_data.hero_position.z,
            konq_interior_save_data.hero_angle.y);
        update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
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
    camera->target_pos.x = konq_interior_save_data.exterior_camera_position.x;
    camera->target_pos.y = konq_interior_save_data.exterior_camera_position.y;
    camera->target_pos.z = konq_interior_save_data.exterior_camera_position.z;
    camera->target_ang.x = konq_interior_save_data.exterior_camera_angle.x;
    camera->target_ang.y = konq_interior_save_data.exterior_camera_angle.y;
    camera->target_ang.z = konq_interior_save_data.exterior_camera_angle.z;
    resume_weather_effects(&konq_interior_save_data);

    npc_data = konq_interior_save_data.current_interior->npc_data;
    if (npc_data != 0) {
        npc_count = get_row_count_for_table_by_pointer(
            konquest_pdata->script_owner, npc_data);
        for (index = 0; index < npc_count; ++index) {
            KonquestNpcRecord* npc = find_npc_by_data(npc_data[index]);

            if (npc != 0) {
                npc->flags_bits.in_interior = 0;
            }
            remove_npc(npc_data[index]);
        }
    }

    remove_interior_room_objects();
    delete_triggers_from_tile(
        konquest_pdata->tile_width * konquest_pdata->tile_height);
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
        ->jump_sleep(p_konquest_loop, 0.0f);
    return 0.0f;
}

static float p_konq_interior_loop(void) {
    if (get_konquest_game_mode() == 0) {
        handle_controller_input();
    }
    trigger_update(0);
    npc_update(0);
    pui_update();
    return 1.0f;
}

/* TODO: [near miss] 99.831320%; original latch retained; relocation offsets, branch lowering; one-trial ceiling. */
void set_interior_cam_pos_and_ang(void) {
    Vec position = {0.0f, 0.0f, 0.0f};
    Vec angle = {0.0f, 0.0f, 0.0f};
    Vec angle_offset = {0.0f, 0.0f, 0.0f};
    CameraObj* camera;

    camera = camera_item.node;
    if (camera != 0) {
        camera = (camera->hdr.instance == camera_item.instance) ? camera : 0;
    } else {
        camera = 0;
    }

    if (camera != 0) {
        position.x = konquest_pdata->camera_offset_x +
                     konq_interior_save_data.current_interior
                         ->camera_position.x;
        position.y = konquest_pdata->camera_offset_y +
                     konq_interior_save_data.current_interior
                         ->camera_position.y;
        position.z = konquest_pdata->camera_offset_z +
                     konq_interior_save_data.current_interior
                         ->camera_position.z;
        angle.x = angle_offset.x +
                  konq_interior_save_data.current_interior->camera_angle.x;
        angle.y = angle_offset.y +
                  konq_interior_save_data.current_interior->camera_angle.y;
        angle.z = angle_offset.z +
                  konq_interior_save_data.current_interior->camera_angle.z;
        set_camera_position(&position);
        set_camera_angle(&angle);
    }
    if (camera == 0) {
        return;
    }
        update_mkobj(camera != 0 ? as_mkhdr(&camera->hdr) : 0);
}

static inline CameraObj* camera_live_node(CameraItem* owner) {
    CameraObj* object = owner->node;
    if (object != 0) {
        if (object->hdr.instance == owner->instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

/* The camera block expands set_interior_cam_pos_and_ang inline; retail has no call. */
/* TODO: [near miss] 99.441500%; relocation offsets, register coloring; one-trial ceiling. */
static float p_konq_interior_entry_point(void) {
    MkObj* hero = interior_live_hero_object(konquest_pdata);



    {
    Vec monk_position = {0.0f, 0.0f, 0.0f};
    Vec monk_angles = {0.0f, 0.0f, 0.0f};
    Vec monk_angle_base = {0.0f, 0.0f, 0.0f};
    MkObj* model;
    MkObj* interior_object;
    KonquestRoomSobj* entry;
    int* npc_data;
    unsigned int npc_count;
    unsigned int index;

    push_game_state(0x14);
    turn_controllers_off();
    konquest_pdata->field_120 = 0;
    hero_stop_moving();
    _mkproc_sleep_ticks = 10.0f;
    ((KonquestInteriorProcVtable*)aproc->vtbl)->sleep();
    stop_time_passing();
    stop_hero_collisions();
    suspend_hero_state_process();
    xfer_camera(p_idle, 1);
    _mkproc_sleep_ticks = 60.0f;
    ((KonquestInteriorProcVtable*)aproc->vtbl)->sleep();

    if (konq_interior_save_data.enter_script_index != 0) {
        cmdscript_setup_execution(
            konquest_pdata->script_owner,
            konq_interior_save_data.enter_script_index);
        cmdscript_execute(konquest_pdata->script_owner);
    }
    fade_to_black(4, 0);

    if (konquest_pdata->region_table->interior_art_name != 0) {
        wait_for_slot_load(0xA002F);
        interior_object = interior_saved_live_interior_object(&konq_interior_save_data);

        if (interior_object == 0) {
            model = load_named_model_from_slot(
                0xA002F, "BACKGROUND", 0x301E, 0);
            if (model != 0) {
                konq_interior_save_data.interior_object = model;
                konq_interior_save_data.interior_object_instance =
                    model->hdr.instance;
                obj_create_sobjs(model);
                model->pos.value.x = konquest_pdata->camera_offset_x;
                model->pos.value.y = konquest_pdata->camera_offset_y;
                model->pos.value.z = konquest_pdata->camera_offset_z;
                entry = room_sobj_list;
                if (model != 0) {
                    for (; entry->id != 0; entry++) {
                        MkSobj* sobj = obj_find_sobj_by_id(model, entry->id);

                        if (sobj != 0) {
                            hide_sobj(sobj);
                        }
                    }
                }
                model->light_flags = 1;
            }
        }
    }
    pause_weather_effects();

    if (hero != 0) {
        konq_interior_save_data.hero_position.x =
            konq_interior_save_data.trigger_position.x;
        konq_interior_save_data.hero_position.y =
            konq_interior_save_data.trigger_position.y;
        konq_interior_save_data.hero_position.z =
            konq_interior_save_data.trigger_position.z;
        konq_interior_save_data.hero_position.y = hero->pos.value.y;
        konq_interior_save_data.hero_angle.x = hero->ang.x;
        konq_interior_save_data.hero_angle.y = hero->ang.y;
        konq_interior_save_data.hero_angle.z = hero->ang.z;
        konq_interior_save_data.hero_angle.y =
            norm_angle(3.1415927f + hero->ang.y);
        monk_position.x =
            konquest_pdata->camera_offset_x +
            konq_interior_save_data.current_interior->monk_position.x;
        monk_position.y =
            konquest_pdata->camera_offset_y +
            konq_interior_save_data.current_interior->monk_position.y;
        monk_position.z =
            konquest_pdata->camera_offset_z +
            konq_interior_save_data.current_interior->monk_position.z;
        monk_angles.x =
            monk_angle_base.x +
            konq_interior_save_data.current_interior->monk_angle.x;
        monk_angles.y =
            monk_angle_base.y +
            konq_interior_save_data.current_interior->monk_angle.y;
        monk_angles.z =
            monk_angle_base.z +
            konq_interior_save_data.current_interior->monk_angle.z;
        set_monk_position(
            monk_position.x, monk_position.y, monk_position.z, monk_angles.y);
    }

    get_camera_position(&konq_interior_save_data.exterior_camera_position);
    get_camera_angle(&konq_interior_save_data.exterior_camera_angle);

    {
        Vec camera_angle_base = {0.0f, 0.0f, 0.0f};
        Vec camera_angle = {0.0f, 0.0f, 0.0f};
        Vec camera_position = {0.0f, 0.0f, 0.0f};
        CameraObj* camera = camera_live_node(&camera_item);



        if (camera != 0) {
            camera_position.x =
                konquest_pdata->camera_offset_x +
                konq_interior_save_data.current_interior->camera_position.x;
            camera_position.y =
                konquest_pdata->camera_offset_y +
                konq_interior_save_data.current_interior->camera_position.y;
            camera_position.z =
                konquest_pdata->camera_offset_z +
                konq_interior_save_data.current_interior->camera_position.z;
            camera_angle.x =
                camera_angle_base.x +
                konq_interior_save_data.current_interior->camera_angle.x;
            camera_angle.y =
                camera_angle_base.y +
                konq_interior_save_data.current_interior->camera_angle.y;
            camera_angle.z =
                camera_angle_base.z +
                konq_interior_save_data.current_interior->camera_angle.z;
            set_camera_position(&camera_position);
            set_camera_angle(&camera_angle);
        }
        if (camera != 0) {
            update_mkobj(camera != 0 ? as_mkhdr(&camera->hdr) : 0);
        }
    }

    xfer_camera(p_konquest_interior_camera_proc, 1);
    konq_interior_save_data.background_lights = bgnd_light_list;
    konq_interior_save_data.special_lights = special_light_list;
    bgnd_light_list = 0;
    special_light_list = 0;
    load_lights(
        konq_interior_save_data.current_interior->light_defs,
        &bgnd_light_list);

    interior_object = interior_saved_live_interior_object(&konq_interior_save_data);

    if (interior_object != 0) {
        insert_fgnd_mkobj(interior_object);
        update_mkobj(
            interior_object != 0 ? as_mkhdr(&interior_object->hdr) : 0);
    }
    konquest_hide_hud(1);
    entry = room_sobj_list;
    if (interior_object != 0) {
        for (; entry->id != 0; entry++) {
            MkSobj* sobj = obj_find_sobj_by_id(interior_object, entry->id);

            if (sobj != 0) {
                hide_sobj(sobj);
            }
        }
    }
    place_interior_room_objects(
        konq_interior_save_data.current_interior->room_objects);
    place_interior_room_objects(
        konq_interior_save_data.current_interior->script_objects);

    {
        KonquestTile* tile = get_nth_tile_struct(
            konquest_pdata->tile_width * konquest_pdata->tile_height);
        float trigger_x =
            konq_interior_save_data.current_interior->trigger_position.x +
            tile->position.x;
        float trigger_y =
            konq_interior_save_data.current_interior->trigger_position.y +
            tile->position.y;
        float trigger_z =
            konq_interior_save_data.current_interior->trigger_position.z +
            tile->position.z;
        int function = get_script_function_by_name(
            konquest_pdata->script_owner, "standard_ir_exit");

        add_temporary_trigger(
            0x1FE, 1, 1, 1, function, trigger_x, trigger_y, trigger_z,
            1.0f);
    }
    update_tile_grid();

    npc_data = konq_interior_save_data.current_interior->npc_data;
    if (npc_data != 0) {
        npc_count = get_row_count_for_table_by_pointer(
            konquest_pdata->script_owner, npc_data);
        for (index = 0; index < npc_count; ++index) {
            KonquestNpcRecord* npc;

            add_npc(*npc_data);
            npc = find_npc_by_data(*npc_data);
            if (npc != 0) {
                npc->flags_1D_bits.in_interior = 1;
            }
            npc_data++;
        }
    }

    _mkproc_sleep_ticks = 1.0f;
    ((KonquestInteriorProcVtable*)aproc->vtbl)->sleep();
    npc_update(1);
    trigger_update(1);
    transition_hero_to_anim_script(0x2BD, 0, 0.1f, 1.0f);
    fade_from_black(4, 0);
    if (konq_interior_save_data.enter_done_script_index != 0) {
        cmdscript_setup_execution(
            konquest_pdata->script_owner,
            konq_interior_save_data.enter_done_script_index);
        cmdscript_execute(konquest_pdata->script_owner);
    }
    start_hero_collisions();
    resume_hero_state_process();
    restore_hero_grounding();
    turn_controllers_on();
    ((KonquestInteriorProcVtable*)aproc->vtbl)
        ->jump_sleep(p_konq_interior_loop, 0.0f);
    }
    return 0.0f;
}

/*
 * Soft ceiling: start_konquest_interior ~96.1% -- the found-flag init is not
 * sunk to the search-loop exit (retail shares the idp register for both) and
 * NV homes shift; structure, calls, and both id-list walks match retail;
 * stop.
 */
void start_konquest_interior(
    KonquestInteriorRoom* interior, KonquestRoomObject* script_objects,
    const void** items, int* npc_data,
    KonquestRoomObjectTexture* wall_textures,
    KonquestRoomObjectTexture* floor_textures, int door_bits) {
    MkObjLatch* pdata;
    KonquestTrigger* trigger;
    KonquestRoomObjectTexture* table;
    KonquestRoomObject* robj;
    MkProc* proc;

    pdata = (MkObjLatch*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return;
    }
    trigger = (KonquestTrigger*)pdata->obj;
    if (trigger != 0) {
        trigger =
            (trigger->hdr.instance == pdata->obj_instance) ? trigger : 0;
    } else {
        trigger = 0;
    }
    if (trigger == 0) {
        return;
    }

    if (trigger->building != 0) {
        konq_interior_save_data.building_id = trigger->building->uid;
        konq_interior_save_data.exterior_door_bits = door_bits;
    } else {
        konq_interior_save_data.building_id = 0;
        konq_interior_save_data.exterior_door_bits = 0;
    }

    konq_interior_save_data.enter_script_index = interior->field_50;
    konq_interior_save_data.enter_done_script_index = interior->field_54;
    konq_interior_save_data.exit_script_index = interior->exit_script_index;
    konq_interior_save_data.entry_script_index =
        interior->entry_script_index;
    konq_interior_save_data.trigger_position.x =
        trigger->data->position.x;
    konq_interior_save_data.trigger_position.y =
        trigger->data->position.y;
    konq_interior_save_data.trigger_position.z =
        trigger->data->position.z;
    interior->script_objects = script_objects;
    interior->items = items;
    interior->npc_data = npc_data;
    konq_interior_save_data.current_interior = interior;

    table = wall_textures != 0
                ? wall_textures
                : get_data_table_by_name("default_walls");
    if (table == 0) {
        return;
    }
    robj = interior->room_objects;
    if (table != 0 && robj != 0) {
        for (; robj->id != 0; robj++) {
            unsigned int* idp = wall_id_list;
            int found = 0;

            if (idp != 0) {
                for (; *idp != 0; idp++) {
                    if (*idp == robj->id) {
                        found = 1;
                        break;
                    }
                }
            }
            if (found != 0) {
                robj->textures = table;
            }
        }
    }

    table = floor_textures != 0
                ? floor_textures
                : get_data_table_by_name("default_floor");
    if (table == 0) {
        return;
    }
    robj = interior->room_objects;
    if (table != 0 && robj != 0) {
        for (; robj->id != 0; robj++) {
            unsigned int* idp = floor_id_list;
            int found = 0;

            if (idp != 0) {
                for (; *idp != 0; idp++) {
                    if (*idp == robj->id) {
                        found = 1;
                        break;
                    }
                }
            }
            if (found != 0) {
                robj->textures = table;
            }
        }
    }

    proc = find_mkproc_pid(0x2001);
    if (proc != 0) {
        xfer_proc(proc, p_konq_interior_entry_point);
    }
}

void initialize_konquest_interior(void) {
    konq_interior_save_data.interior_object = 0;
    konq_interior_save_data.interior_object_instance = 0;
}
