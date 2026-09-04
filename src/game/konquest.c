#include "runtime/mk_struct.h"
#include "runtime/asset.h"
#include "runtime/image.h"
#include "runtime/fonts.h"
#include "runtime/utils.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pebble.h"
#include "runtime/cam.h"
#include "runtime/cstring.h"
#include "runtime/cmath.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_mem.h"
#include "runtime/section.h"
#include "runtime/shadow.h"
#include "runtime/sound_tracker.h"
#include "runtime/light.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "game/game_info.h"
#include "game/konquest.h"
#include "game/konquest_items.h"
#include "game/settings.h"
#include "game/konquest_save.h"
#include "game/collision.h"
#include "game/krypt.h"
#include "platform/io.h"
#include "platform/fog.h"
#include "platform/gcutils.h"
#include "msl/msl_types.h"
#include "mw/mwMemHeap.h"
#include "rw/rplight.h"

extern RwCamera* Camera;

#define KONQUEST_OFFSETOF(type, member) \
    ((unsigned int)&((type*)0)->member)

#define KONQUEST_RESOLVE_LATCH(latch, resolved)            \
    do {                                                    \
        (resolved) = (latch).object;                        \
        if ((resolved) != 0) {                              \
            if ((resolved)->instance != (latch).instance) { \
                (resolved) = 0;                             \
            }                                               \
        } else {                                            \
            (resolved) = 0;                                 \
        }                                                   \
    } while (0)



typedef union KonquestProfile {
    unsigned char raw[0x2A0];
    struct {
        char pad00[8];
        int portal_entry_pending; /* +0x08 */
        char pad0C[0x50];
        unsigned char hero_age; /* +0x5C */
        unsigned char region_arg; /* +0x5D */
        char pad5E[6];
        int objective_index; /* +0x64 */
    } fields;
} KonquestProfile;

typedef struct KonquestCommonProfile {
    char pad00[0x140];
    unsigned int character_bits[2]; /* +0x140 */
    unsigned int alternate_character_bits[2]; /* +0x148 */
} KonquestCommonProfile;

typedef struct KonquestCharacterBits {
    unsigned int high;
    unsigned int low;
} KonquestCharacterBits;

typedef struct KonquestTileRecord KonquestTileRecord;
typedef struct ScriptSlot ScriptSlot;
typedef struct KonquestLightRow KonquestLightRow;

typedef struct CmdScript {
    char pad00[8];
    ScriptSlot* mko;
    char pad0C[0x1C];
    void* unk28;
} CmdScript;

typedef struct KonquestScriptSlotView {
    char pad00[0x90];
    int* section_context;
} KonquestScriptSlotView;

typedef struct KonquestScriptSlotHeaderView {
    char pad00[0x58];
    unsigned int table_count;
} KonquestScriptSlotHeaderView;

typedef struct KonquestObjectState {
    int sound;
    Vec position_speed; /* +0x04 */
    Vec position; /* +0x10 */
    Vec angle_speed; /* +0x1C */
    Vec angles; /* +0x28 */
} KonquestObjectState; /* 0x34 */

typedef char KonquestObjectStateSizeCheck[
    sizeof(KonquestObjectState) == 0x34 ? 1 : -1];

typedef struct KonquestEnumerationEntry {
    union {
        int partner_index;
        int locator_uid;
        int field_00;
    }; /* +0x00 */
    union {
        int partner_uid;
        int field_04;
    }; /* +0x04 */
    int enumeration; /* +0x08 */
    KonquestObjectState* states; /* +0x0C */
    float angle; /* +0x10 */
    float trigger_offset; /* +0x14 */
    float trigger_radius; /* +0x18 */
} KonquestEnumerationEntry; /* 0x1C */

typedef struct KonquestRegionTable {
    void* region_asset;
    const char* interior_art_name; /* +0x04 */
    void* map_table; /* +0x08 */
    char pad0C[0x0C];
    const char* supplemental_art_name; /* +0x18 */
    const char* effect_bank_name; /* +0x1C */
    void* weather_table; /* +0x20 */
    char pad24[4];
    unsigned int setup_function; /* +0x28 */
    char pad2C[8];
    KonquestEnumerationEntry* enumerations; /* +0x34 */
    char pad38[4];
    union {
        float far_clip_distance;
        float objective_beam_threshold;
    }; /* +0x3C */
    float fog_distance; /* +0x40 */
    float fog_density; /* +0x44 */
    KonquestLightRow* directional_light_table; /* +0x48 */
    KonquestLightRow* ambient_light_table; /* +0x4C */
    KonquestLightRow* sky_ambient_light_table; /* +0x50 */
    void* sky_table; /* +0x54 */
    char pad58[8];
    int ambient_sound_a; /* +0x60 */
    int ambient_sound_b; /* +0x64 */
    int ambient_sound; /* +0x68 */
    int ambient_effect_a; /* +0x6C */
    int ambient_effect_b; /* +0x70 */
    int time_chime_sound; /* +0x74 */
    Vec map_portal_position; /* +0x78 */
    void* pui_header; /* +0x84 */
    void* pui_footer; /* +0x88 */
    void* pui_event_table; /* +0x8C */
} KonquestRegionTable;

typedef struct KonquestSkyRow {
    int hour;
    float alpha;
    float red;
    float green;
    float blue;
    float background_alpha;
} KonquestSkyRow; /* 0x18 */

struct KonquestLightRow {
    int hour; /* +0x00 */
    int field_04;
    RwRGBAReal color; /* +0x08 */
}; /* 0x18 */

typedef struct KonquestDirectionalLightRow {
    int hour;
    float strength;
    char pad08[0x10];
} KonquestDirectionalLightRow; /* 0x18 */

typedef struct KonquestChildObject KonquestChildObject;

typedef struct KonquestTransitionPdata {
    MkHdr hdr;
    KonquestChildObject* object; /* +0x08 */
    int state;    /* +0x0C */
    int play_sound; /* +0x10 */
} KonquestTransitionPdata;

typedef struct KonquestFadePdata {
    MkHdr hdr;
    int to_black; /* +0x08 */
    int ticks; /* +0x0C */
    int white; /* +0x10 */
    int fade_sound; /* +0x14 */
    ScreenObj* object; /* +0x18 */
    unsigned int object_instance; /* +0x1C */
    unsigned char red; /* +0x20 */
    unsigned char green; /* +0x21 */
    unsigned char blue; /* +0x22 */
    unsigned char alpha; /* +0x23 */
} KonquestFadePdata;

typedef struct KonquestPortalRow {
    int uid;
    Vec target_offset;
    float camera_y_offset;
    float hero_distance;
    float camera_distance;
    int field_1C;
} KonquestPortalRow; /* 0x20 */

typedef struct KonquestPortalPdata {
    MkHdr hdr;
    int uid;
    Vec target_offset;
    float camera_y_offset;
    float hero_distance;
    float camera_distance;
    int field_24;
} KonquestPortalPdata; /* 0x28 */

typedef struct KonquestTeleportPdata {
    MkHdr hdr;
    Vec target;
} KonquestTeleportPdata; /* 0x14 */

typedef struct KonquestFightMessagePdata {
    MkHdr hdr;
    int message;
} KonquestFightMessagePdata; /* 0x0C */

typedef struct KonquestCameraScriptPdata {
    MkHdr hdr;
    void* owner;  /* +0x08 */
    void* script; /* +0x0C */
} KonquestCameraScriptPdata;

typedef struct KonquestDialogArt KonquestDialogArt;

typedef struct KonquestStringLatch {
    StringObj* object;
    unsigned int instance;
} KonquestStringLatch;

typedef struct KonquestDialogPdata {
    MkHdr hdr;
    KonquestStringLatch lines[3]; /* +0x08 */
    KonquestDialogArt* dialog_art; /* +0x20 */
    int skip_print_delay; /* +0x24 */
    int active; /* +0x28 */
    char text[1000]; /* +0x2C */
    char line_text[3][100]; /* +0x414 */
    int font; /* +0x540 */
    int print_speed; /* +0x544 */
    int line_index; /* +0x548 */
    int print_ticks; /* +0x54C */
} KonquestDialogPdata; /* 0x550 */

typedef struct KonquestNpcData {
    char pad00[0x4C];
    Vec position; /* +0x4C */
    float angle_y; /* +0x58 */
    char pad5C[0x48];
    int idle_animation; /* +0xA4 */
} KonquestNpcData;

typedef struct KonquestNpcAnimationState {
    MkHdr hdr;
    int state; /* +0x08 */
    MkObj* object; /* +0x0C */
    char pad10[0x10];
    MkProc* proc; /* +0x20 */
} KonquestNpcAnimationState;

typedef struct KonquestNpcStateFlags {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char nis_participant : 1;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char bit2 : 1;
    unsigned char bit1 : 1;
    unsigned char bit0 : 1;
} KonquestNpcStateFlags;

typedef union KonquestNpc {
    unsigned char raw[0x264];
    struct {
        MkHdr hdr;
        char pad08[4];
        KonquestNpcData* data; /* +0x0C */
        char pad10[4];
        union {
            MkSobj* dialog_owner;
            KonquestDialogArt* dialog_art;
            KonquestNpcAnimationState* animation_state;
        }; /* +0x14 */
        MkHdr* animation; /* +0x18 */
        union {
            unsigned char flags;
            struct {
                unsigned char flags_bit7 : 1;
                unsigned char flags_bit6 : 1;
                unsigned char flags_bit5 : 1;
                unsigned char flags_bit4 : 1;
                unsigned char flags_bit3 : 1;
                unsigned char flags_bit2 : 1;
                unsigned char flags_bit1 : 1;
                unsigned char flags_bit0 : 1;
            } flag_bits;
        }; /* +0x1C */
        union {
            unsigned char state_flags;
            KonquestNpcStateFlags state_flag_bits;
        }; /* +0x1D */
        char pad1E[2];
        char pad20[0x38];
        int state_58; /* +0x58 */
        char pad5C[0x10];
        int punch_count; /* +0x6C */
        char pad70[0x18C];
        float punch_distance; /* +0x1FC */
        int punch_enabled; /* +0x200 */
        char pad204[0x58];
        MkProc* turn_proc; /* +0x25C */
        unsigned int turn_proc_instance; /* +0x260 */
    } fields;
} KonquestNpc;

typedef struct KonquestNpcSceneState {
    char pad00[0x0C];
    MkObj* object;
    char pad10[0x10];
    MkProc* process;
} KonquestNpcSceneState;

typedef struct KonquestNpcRuntime {
    MkHdr hdr;
    char pad08[4];
    void* data;
    char pad10[4];
    KonquestNpcSceneState* scene_state;
    char pad18[4];
    union {
        unsigned char flags;
        struct {
            unsigned char flags_bit7 : 1;
            unsigned char nis_controlled : 1;
            unsigned char flags_rest : 6;
        };
    };
    union {
        unsigned char state_flags;
        KonquestNpcStateFlags state_flag_bits;
    };
    char pad1E[0x3A];
    int type;
    char pad5C[0x0C];
    int conversation_count; /* +0x68 */
    char pad6C[0x1B0];
    void* script_function;
} KonquestNpcRuntime;

typedef struct KonquestNisParticipant {
    MkHdr hdr;
    int type;
    KonquestNpcRuntime* npc;
    char pad10[4];
    MkProcEntryFn resume_entry;
} KonquestNisParticipant;

typedef struct KonquestSaveHeader {
    char pad00[0x0C];
    int region; /* +0x0C */
} KonquestSaveHeader;

typedef struct KonquestSavedState {
    char pad00[8];
    int valid;              /* +0x08 */
    int region;             /* +0x0C */
    Vec hero_position;      /* +0x10 */
    float hero_angle;       /* +0x1C */
    char pad20[0x18];
    char script_name[0x40]; /* +0x38 */
    int player_a;           /* +0x78 */
    int player_b;           /* +0x7C */
    char pad80[4];
    int fight_mode;         /* +0x84 */
    char pad88[8];
    int preserve_word;      /* +0x90 */
    int progression;        /* +0x94 */
} KonquestSavedState;

typedef struct KonquestRegionAsset {
    MkFileEntry* fight_files;
    MkFileEntry* art_files; /* +0x04 */
    union {
        const char* script_name;
        const char* map_art_name;
    }; /* +0x08 */
    const char* string_bank; /* +0x0C */
    const char* hud_art_name; /* +0x10 */
} KonquestRegionAsset; /* 0x14 */

typedef struct KonquestNpcCameraTarget {
    char pad00[0x0C];
    MkObj* focus_object; /* +0x0C */
    char pad10[4];
    AniTextureControl* face_texture; /* +0x14 */
    unsigned int face_texture_instance; /* +0x18 */
    char pad1C[4];
    union {
        int ready;
        MkProc* animation_proc;
    }; /* +0x20 */
    char pad24[8];
    unsigned int alpha; /* +0x2C */
} KonquestNpcCameraTarget;

typedef struct KonquestGrounding {
    MkHdr hdr;
    char pad08[0x0C];
    KonquestNpcCameraTarget* camera_target; /* +0x14 */
    char pad18[5];
    union {
        unsigned char flags;
        struct {
            unsigned char bit7 : 1;
            unsigned char bit6 : 1;
            unsigned char bit5 : 1;
            unsigned char bit4 : 1;
            unsigned char bit3 : 1;
            unsigned char bit2 : 1;
            unsigned char suspended : 1;
            unsigned char bit0 : 1;
        } flag_bits;
    }; /* +0x1D */
} KonquestGrounding;

typedef struct KonquestObjectLatch {
    MkHdr* object;
    unsigned int instance;
} KonquestObjectLatch;

typedef struct KonquestDialogOwner {
    char pad00[0x1C];
    union {
        unsigned char flags;
        struct {
            unsigned char bit7 : 1;
            unsigned char bit6 : 1;
            unsigned char bit5 : 1;
            unsigned char bit4 : 1;
            unsigned char bit3 : 1;
            unsigned char dialog_active : 1;
            unsigned char bit1 : 1;
            unsigned char bit0 : 1;
        } flag_bits;
    }; /* +0x1C */
} KonquestDialogOwner;

struct KonquestDialogArt {
    char pad00[0x0C];
    unsigned int id_flags; /* +0x0C */
    char pad10[0x10];
    MkHdr* bound_hdr; /* +0x20 */
    KonquestDialogOwner* owner; /* +0x24 */
};

typedef struct KonquestAwardArtPdata {
    MkHdr hdr;
    int mode; /* +0x08 */
    KryptScreenObjLatch art[6]; /* +0x0C */
    KonquestStringLatch description; /* +0x3C */
    int complete; /* +0x44 */
    union {
        int value;
        KonquestPuiDefinition* item;
    }; /* +0x48 */
} KonquestAwardArtPdata; /* 0x4C */

typedef struct KonquestPdataFlags {
    unsigned char region_loaded : 1;
    unsigned char triggers_active : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char bit2 : 1;
    unsigned char bit1 : 1;
    unsigned char bit0 : 1;
} KonquestPdataFlags;

typedef struct KonquestTimePair {
    unsigned int first;
    unsigned int second;
} KonquestTimePair;

typedef struct KonquestTime {
    union {
        struct {
            int year;         /* +0x00 */
            int month;        /* +0x04, zero based */
            int day_of_month; /* +0x08, zero based */
            int day_of_week;  /* +0x0C */
            int hour;         /* +0x10 */
            int minute;       /* +0x14 */
        };
        KonquestTimePair pairs[3];
    };
} KonquestTime; /* 0x18 */

typedef struct KonquestObjectiveState {
    int visible;
    Vec position;
    int target_type;
    unsigned int trigger_id;
    KonquestNpcData* npc_data;
    Vec target_offset;
    float beam_scale; /* +0x28 */
    struct KonquestObjectiveRow* table; /* +0x2C */
} KonquestObjectiveState;

typedef struct KonquestDirectionalLightDef {
    int type;
    MkProcEntryFn process;
    int flags;
    RwRGBAReal color;
    Vec angles;
} KonquestDirectionalLightDef; /* 0x28 */

typedef struct KonquestAmbientLightDef {
    int type;
    MkProcEntryFn process;
    int flags;
    RwRGBAReal color;
} KonquestAmbientLightDef; /* 0x1C */

typedef struct KonquestPdata {
    char pad00[8];
    MkProc* region_load_proc;     /* +0x08 */
    union {
        unsigned char flags;
        KonquestPdataFlags flag_bits;
    };
    char pad0d[3];
    int game_modes[4];
    int game_mode_index;
    ScriptSlot* script_owner;    /* +0x24 */
    KonquestRegionTable* region_table; /* +0x28 */
    int interior_active;         /* +0x2C */
    MkPtr* triggers;             /* +0x30 */
    MkPtr* temporary_triggers;   /* +0x34 */
    char pad38[8];
    MkPtr* npcs;                /* +0x40 */
    char pad44[4];
    int hud_visible;             /* +0x48 */
    KonquestObjectLatch hud_objects[7]; /* +0x4C .. +0x83 */
    KryptScreenObjLatch award_picture; /* +0x84 */
    KonquestObjectLatch hud_labels[6]; /* +0x8C .. +0xBB */
    char padBC[0x20];
    KonquestObjectLatch hud_extra_label; /* +0xDC */
    KonquestObjectLatch award_text; /* +0xE4 */
    KonquestObjectLatch award_art;  /* +0xEC */
    MkProc* collision_proc;      /* +0xf4 */
    MkObj* hero_object;          /* +0xf8 */
    unsigned int hero_instance;  /* +0xfc */
    KonquestGrounding* hero_grounding; /* +0x100 */
    unsigned int grounding_instance; /* +0x104 */
    AnimPdata* hero_anim;          /* +0x108 */
    int input_port;              /* +0x10C */
    int tile_load_state;          /* +0x110 */
    int tile_column;             /* +0x114 */
    int tile_row;                /* +0x118 */
    int hero_state;              /* +0x11c */
    int npc_interaction_state;   /* +0x120 */
    int animation_event_index;  /* +0x124 */
    int current_portal_uid;      /* +0x128 */
    int hero_unconscious;        /* +0x12C */
    void* unconscious_camera_script; /* +0x130 */
    float time_of_day;             /* +0x134 */
    KonquestTime current_time;       /* +0x138 */
    int time_passing;            /* +0x150 */
    int time_rate_changed;        /* +0x154 */
    int region_index;              /* +0x158 */
    KonquestTileRecord* tile_structs; /* +0x15C */
    MkPtr* sobj_infos;            /* +0x160 */
    MkHdr* tile_objects;           /* +0x164 */
    unsigned int tile_objects_instance; /* +0x168 */
    KonquestObjectLatch tile_model; /* +0x16C */
    float field_174;
    unsigned int visible_tile_bits; /* +0x178 */
    int tile_width;              /* +0x17c */
    int tile_height;             /* +0x180 */
    float tile_origin_x;         /* +0x184 */
    float tile_origin_z;         /* +0x188 */
    Vec fallback_tile_position; /* +0x18C */
    KonquestDirectionalLightDef directional_light; /* +0x198 */
    KonquestAmbientLightDef sky_ambient_light; /* +0x1C0 */
    KonquestAmbientLightDef ambient_light; /* +0x1DC */
    KonquestObjectLatch movement_npc; /* +0x1F8 */
    char pad200[0xB8];
    KonquestObjectLatch objective_beam; /* +0x2B8 */
    union {
        KonquestObjectiveState objective; /* +0x2C0 */
        struct {
            int objective_visible; /* +0x2C0 */
            Vec objective_position; /* +0x2C4 */
            char pad2D0[0x18];
            float objective_beam_scale; /* +0x2E8 */
            void* objective_table; /* +0x2EC */
        };
    };
    int objective_field_2F0;
    MkPtr* pui_list;             /* +0x2F4 */
    char pad2F8[4];
    KonquestPuiDefinition* reference_pui; /* +0x2FC */
    struct KonquestTriggerStruct* active_trigger; /* +0x300 */
    unsigned int active_trigger_instance; /* +0x304 */
    unsigned char pui_inventory_bits[0xDB]; /* +0x308 */
    unsigned char pad3E3;
    unsigned int pui_begin;      /* +0x3e4 - first table index */
    unsigned int pui_end;        /* +0x3e8 - one past final table index */
    unsigned int dynamic_pui_begin; /* +0x3EC */
    unsigned int dynamic_pui_end;   /* +0x3F0 */
    char pad3F4[4];
    int current_nav_area; /* +0x3F8 */
    char pad3FC[8];
    int door_overflow_count; /* +0x404 */
    MkPtr* door_objects;         /* +0x408 */
    int generated_door_count; /* +0x40C */
    KonquestWaypoint* door_waypoints; /* +0x410 */
    float base_shadow_strength; /* +0x414 */
    float directional_light_strength; /* +0x418 */
    char pad41C[4];
    int weather_type;            /* +0x420 */
    int weather_param_b;         /* +0x424 */
    int weather_param_a;         /* +0x428 */
    float sky_color_multiplier; /* +0x42C */
    MkPtr* attached_sounds;      /* +0x430 */
    MslSoundHandle ambient_sound_a; /* +0x434 */
    MslSoundHandle ambient_sound_b; /* +0x438 */
    MslSoundHandle ambient_sound_main; /* +0x43C */
    MkObj* damashi_object;          /* +0x440 */
    unsigned int damashi_instance; /* +0x444 */
    KonquestNpc* hero_npc;          /* +0x448 */
    unsigned int hero_npc_instance; /* +0x44C */
    MslSoundHandle damashi_sound;   /* +0x450 */
} KonquestPdata;

typedef struct KonquestSwitchState {
    int input_port;
} KonquestSwitchState;

typedef struct KonquestSwitchPdata {
    MkHdr hdr;
    KonquestSwitchState* state;
} KonquestSwitchPdata;

typedef struct KonquestR1Pdata {
    MkHdr hdr;
    PlyrInfo* player; /* +0x08 */
} KonquestR1Pdata;

typedef struct KonquestHeadTrackingPdata {
    MkHdr hdr;
    float angle_y; /* +0x08 */
} KonquestHeadTrackingPdata; /* 0x0C */

typedef char KonquestHeadTrackingPdataSizeCheck[
    sizeof(KonquestHeadTrackingPdata) == 0x0C ? 1 : -1];

typedef struct MonkStateData {
    int animation_event_count; /* +0x00 */
    int animation_event_frames[4]; /* +0x04 */
    int animation_event_bones[4]; /* +0x14 */
    int transition_order; /* +0x24 */
    AniData* animation; /* +0x28 */
    float animation_step; /* +0x2C */
    int transition; /* +0x30 */
    MkProcEntryFn control_proc; /* +0x34 */
} MonkStateData; /* 0x38 */

typedef struct KonquestProcSleepVtable {
    char pad00[0x18];
    int (*sleep)(void); /* +0x18 */
} KonquestProcSleepVtable;

typedef struct KonquestProcJumpVtable {
    char pad00[0x24];
    int (*jump_sleep)(MkProcEntryFn entry, void* context, float ticks);
} KonquestProcJumpVtable;

typedef struct KonquestDestroyable KonquestDestroyable;
typedef int (*KonquestDestroyFn)(KonquestDestroyable* object);

typedef struct KonquestDestroyVtable {
    void* reserved[4];
    KonquestDestroyFn destroy; /* +0x10 */
} KonquestDestroyVtable;

struct KonquestDestroyable {
    KonquestDestroyVtable* vtbl;
    unsigned int instance;
};

typedef struct KonquestObject {
    MkHdr hdr;
    int uid; /* +0x08 */
    char pad0C[0x0C];
    MkPtr* list_18;
    char pad1C[0x30];
    MkPtr* list_4C;
    KonquestDestroyable* owned_object; /* +0x50 */
} KonquestObject;

typedef struct KonquestSobj {
    MkHdr hdr;
    char pad08[0x40];
    KonquestDestroyable* owned_object; /* +0x48 */
    unsigned int owned_object_instance; /* +0x4C */
} KonquestSobj;

struct KonquestTileRecord {
    int index; /* +0x00 */
    int visible; /* +0x04 */
    Vec position; /* +0x08 */
    MkSobj* scene; /* +0x14 */
    MkPtr* objects; /* +0x18 */
    union {
        int state;
        void* visible_set;
    }; /* +0x1C */
    unsigned int collision_art_id; /* +0x20 */
    MkPtr* collisions; /* +0x24 */
    CollisionObjList* collision_object_list; /* +0x28 */
    int collisions_active; /* +0x2C */
    int shadow_collisions_active; /* +0x30 */
    char pad34[0x11C];
}; /* 0x150 */

typedef char KonquestTileRecordSizeCheck[
    sizeof(KonquestTileRecord) == 0x150 ? 1 : -1];

typedef struct KonquestUidObject {
    MkHdr hdr;
    int uid; /* +0x08 */
    int tile_index; /* +0x0C */
    int hidden; /* +0x10 */
    unsigned int collision_art_id; /* +0x14 */
    MkPtr* collisions; /* +0x18 */
    union {
        char pad1C[0x30];
        struct {
            char attached_pfx_name[0x20]; /* +0x1C */
            int attached_pfx_state;       /* +0x3C */
            Vec attached_pfx_position;    /* +0x40 */
        };
    };
    MkPtr* render_records; /* +0x4C */
    TrackedSound* tracked_sound; /* +0x50 */
} KonquestUidObject;

typedef struct KonquestRenderRecord KonquestRenderRecord;
typedef struct KonquestSobjInfo KonquestSobjInfo;

typedef struct KonquestMatrixPalette {
    char pad00[8];
    MKMATRIX* matrices;              /* +0x08 */
    int count;                       /* +0x0C */
    int capacity;                    /* +0x10 */
    int field_14;                    /* +0x14 */
    KonquestRenderRecord** records;  /* +0x18 */
} KonquestMatrixPalette;

typedef struct KonquestSobjBinding {
    char pad00[0x0C];
    int enumeration_index; /* +0x0C */
    KonquestMatrixPalette* palette; /* +0x10 */
    MkSobj* object; /* +0x14 */
    unsigned int object_instance; /* +0x18 */
    int hidden; /* +0x1C */
} KonquestSobjBinding;

struct KonquestRenderRecord {
    MkHdr hdr;
    KonquestUidObject* owner; /* +0x08 */
    int matrix_index; /* +0x0C */
    union {
        KonquestSobjBinding* binding;
        KonquestSobjInfo* info;
    }; /* +0x10 */
    union {
        Vec field_14;
        Vec position;
    }; /* +0x14 */
    Vec angles; /* +0x20 */
    Vec base_position; /* +0x2C */
    Vec base_angles; /* +0x38 */
    int state; /* +0x44 */
    MkHdr* state_object; /* +0x48 */
    unsigned int state_object_instance; /* +0x4C */
    union {
        MkHdr* field_50;
        KonquestWaypoint* path_waypoints;
    };
};

typedef struct KonquestTileObjectDefinition {
    int uid; /* +0x00 */
    int pebble_count; /* +0x04 */
    char* name; /* +0x08 */
} KonquestTileObjectDefinition;

typedef struct KonquestSobjInfo {
    MkHdr hdr;
    int uid; /* +0x08 */
    int enumeration_index; /* +0x0C */
    PebbleData* pebbles; /* +0x10 */
    MkSobj* object; /* +0x14 */
    unsigned int object_instance; /* +0x18 */
    int type; /* +0x1C */
    Vec position; /* +0x20 */
    unsigned int art_id; /* +0x2C */
} KonquestSobjInfo; /* 0x30 */

typedef union KonquestArtIdRef {
    unsigned int id;
    char* name;
} KonquestArtIdRef;

typedef struct KonquestDayOfWeek {
    KonquestArtIdRef art;
    int flags;
} KonquestDayOfWeek;

typedef struct KonquestChildDefinition {
    char pad00[0x0C];
    int enumeration_index; /* +0x0C */
} KonquestChildDefinition;

struct KonquestPuiDefinition {
    int type; /* +0x00 */
    int clear_inventory_for_unique_event; /* +0x04 */
    const char* model_name; /* +0x08 */
    const char* award_art_name; /* +0x0C */
    int value; /* +0x10 */
    char pad14[8];
    Vec position; /* +0x1C */
    float angle_y; /* +0x28 */
    float radius; /* +0x2C */
    int spawn_script_index; /* +0x30 */
    int pickup_script_index; /* +0x34 */
    int kill_script_index; /* +0x38 */
};

typedef char KonquestPuiDefinitionSizeCheck[
    sizeof(KonquestPuiDefinition) == 0x3C ? 1 : -1];

struct KonquestChildObject {
    MkHdr hdr;
    KonquestUidObject* owner; /* +0x08 */
    int matrix_index; /* +0x0C */
    union {
        KonquestChildDefinition* definition;
        KonquestSobjBinding* binding;
        KonquestSobjInfo* info;
    }; /* +0x10 */
    Vec position; /* +0x14 */
    Vec angles; /* +0x20 */
    Vec base_position; /* +0x2C */
    Vec base_angles; /* +0x38 */
    int state; /* +0x44 */
    MkHdr* state_object; /* +0x48 */
    unsigned int state_object_instance; /* +0x4C */
    KonquestWaypoint* path_waypoints; /* +0x50 */
};

typedef struct KonquestCollisionDefinition {
    char pad00[0x2C];
    unsigned int art_id; /* +0x2C */
} KonquestCollisionDefinition;

typedef struct KonquestCollisionPlacement {
    MkHdr hdr;
    char pad08[8];
    KonquestCollisionDefinition* definition; /* +0x10 */
    char pad14[0x18];
    Vec position; /* +0x2C */
    Vec angles; /* +0x38 */
} KonquestCollisionPlacement;

typedef struct KonquestCollisionVolume {
    MkHdr hdr;
    int uid; /* +0x08 */
    char pad0C[8];
    unsigned int art_id; /* +0x14 */
    MkPtr* objects; /* +0x18 */
    char pad1C[0x30];
    MkPtr* placements; /* +0x4C */
} KonquestCollisionVolume;

typedef struct KonquestCollisionOwner {
    MkHdr hdr;
    char pad08[8];
    KonquestCollisionVolume* collision_volume; /* +0x10 */
} KonquestCollisionOwner;

typedef KonquestChildObject KonquestDoorObject;

typedef struct KonquestRemoveCollisionPdata {
    MkHdr hdr;
    KonquestCollisionOwner* owner; /* +0x08 */
    unsigned int owner_instance; /* +0x0C */
} KonquestRemoveCollisionPdata;

typedef struct KonquestTriggerScriptArgument {
    char pad00[0x10];
    int field_10;
} KonquestTriggerScriptArgument;

typedef struct KonquestPuiRuntime KonquestPuiRuntime;

typedef struct KonquestTriggerDefinition {
    int type;
    unsigned int flags; /* +0x04 */
    Vec position; /* +0x08 */
    float radius; /* +0x14 */
    unsigned int script_index; /* +0x18 */
    int state; /* +0x1C */
    int field_20; /* +0x20 */
    union {
        KonquestTriggerScriptArgument* field_24;
        KonquestPuiRuntime* pui;
    }; /* +0x24 */
} KonquestTriggerDefinition;

typedef struct KonquestObjectiveRow {
    int target_type;
    KonquestNpcData* npc_data;
    KonquestTriggerDefinition* trigger;
    Vec target_offset;
    int requirement_type;
    int requirement_index;
} KonquestObjectiveRow;

typedef char KonquestTriggerDefinitionSizeCheck[
    sizeof(KonquestTriggerDefinition) == 0x28 ? 1 : -1];

typedef struct KonquestTriggerFlags {
    unsigned char bit7 : 1;
    unsigned char pressed : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char bit2 : 1;
    unsigned char bit1 : 1;
    unsigned char bit0 : 1;
} KonquestTriggerFlags;

typedef struct KonquestTriggerObject {
    char pad00[0x4C];
    MkPtr* doors; /* +0x4C */
} KonquestTriggerObject;

typedef struct KonquestTriggerRequirement KonquestTriggerRequirement;

typedef struct KonquestTriggerStruct {
    MkHdr hdr;
    union {
        KonquestTriggerDefinition* owned_data;
        KonquestTriggerRequirement* requirement;
    }; /* +0x08 */
    char* table_name; /* +0x0C */
    union {
        KonquestTriggerObject* object;
        KonquestUidObject* uid_object;
        KonquestUidObject* door_owner;
    }; /* +0x10 */
    unsigned int id; /* +0x14 */
    int tile_index; /* +0x18 */
    MkProc* script_proc; /* +0x1C */
    unsigned int script_proc_instance; /* +0x20 */
    union {
        unsigned char flags;
        KonquestTriggerFlags flag_bits;
    }; /* +0x24 */
    char pad25[3];
    KonquestTime action_time; /* +0x28 */
    int timed_action; /* +0x40 */
} KonquestTriggerStruct;

typedef struct KonquestTriggerScriptPdata {
    MkHdr hdr;
    KonquestTriggerStruct* trigger;
    unsigned int trigger_instance;
} KonquestTriggerScriptPdata;

typedef struct KonquestSwitchModePdata {
    MkHdr hdr;
    char pad08[0x1C];
    int switch_pressed; /* +0x24 */
} KonquestSwitchModePdata;

typedef struct KonquestAmbientFadePdata {
    MkHdr hdr;
    float step_a;
    float step_b;
    float volume_a;
    float volume_b;
    float delay;
} KonquestAmbientFadePdata;

typedef char KonquestAmbientFadePdataSizeCheck[
    sizeof(KonquestAmbientFadePdata) == 0x1C ? 1 : -1];

typedef struct KonquestNisLoadPdata {
    MkHdr hdr;
    int slot;
    char* animation_name;
    char* art_name;
} KonquestNisLoadPdata;

typedef struct KonquestLightAdjustPdata {
    MkHdr hdr;
    RpLight* light;
    MkSobj* object;
    unsigned int object_instance;
} KonquestLightAdjustPdata;

typedef struct KonquestGameSpeedPdata {
    MkHdr hdr;
    float multiplier;
    float target_scale;
} KonquestGameSpeedPdata;

typedef struct KonquestSoundFadePdata {
    MkHdr hdr;
    float step;
    float current_volume;
    float target_volume;
} KonquestSoundFadePdata;

typedef struct KonquestWeatherPdata {
    MkHdr hdr;
    char pad08[0x18];
    KonquestTime end_time; /* +0x20 */
    int initialized; /* +0x38 */
    MslSoundHandle sound; /* +0x3C */
} KonquestWeatherPdata;

typedef char KonquestWeatherPdataSizeCheck[
    sizeof(KonquestWeatherPdata) == 0x40 ? 1 : -1];

typedef struct KonquestTextWindowPdata {
    MkHdr hdr;
    int left;                           /* +0x008 */
    int bottom;                         /* +0x00C */
    int priority;                       /* +0x010 */
    int width;                          /* +0x014 */
    int top;                            /* +0x018 */
    unsigned int prompt_flags;          /* +0x01C */
    int timeout;                        /* +0x020 */
    unsigned int color;                 /* +0x024 */
    int font;                           /* +0x028 */
    int controller_port;                /* +0x02C */
    const char* button_charmap;         /* +0x030 */
    int swap_buttons;                   /* +0x034 */
    char text[0x4B0];                   /* +0x038 */
    int visible_item_count;             /* +0x4E8 */
    int art_slot;                       /* +0x4EC */
    KonquestStringLatch objects[14];    /* +0x4F0 */
} KonquestTextWindowPdata; /* 0x560 */

typedef struct AgeProgressionEntry {
    MkFileEntry* file_table;
    const char* art_section;
    unsigned int first_string_id;
    int first_duration;
    unsigned int second_string_id;
    int second_duration;
    float left_fraction;
    float bottom_fraction;
    float width_fraction;
} AgeProgressionEntry; /* 0x24 */

extern AgeProgressionEntry age_progression_table[4];

typedef struct KonquestChestInteraction {
    char pad00[8];
    Vec position; /* +0x08 */
    float radius; /* +0x14 */
    char pad18[4];
    int closed; /* +0x1C */
} KonquestChestInteraction;

typedef struct KonquestChestOwner {
    MkHdr hdr;
    KonquestChestInteraction* interaction;
} KonquestChestOwner;

typedef struct KonquestChestAnimation {
    char pad00[8];
    unsigned char flags;
    char pad09[0x47];
    float angle;
    char pad54[0x0C];
    float angular_velocity;
} KonquestChestAnimation;

typedef struct KonquestChestPdata {
    MkHdr hdr;
    KonquestChestOwner* owner;
    unsigned int owner_instance;
    KonquestChestAnimation* animation;
    int direction;
} KonquestChestPdata;

typedef struct KonquestInteractionPdata {
    MkHdr hdr;
    KonquestNpc* npc;
    int separation_active;
    int facing_active;
    float separation;
    int separation_ticks;
    int position_initialized;
    Vec last_hero_position;
} KonquestInteractionPdata;

typedef struct KonquestKoinAwardPdata {
    MkHdr hdr;
    int amount;
    unsigned int icon_offset;
} KonquestKoinAwardPdata;

typedef struct KonquestPuiRuntime KonquestPuiDelayView;

typedef struct KonquestSectionContext {
    int slot;
    MkHdr* owner;
    int flags;
} KonquestSectionContext;

typedef union KonquestFloatBits {
    float value;
    unsigned int bits;
} KonquestFloatBits;



typedef struct KonquestTriggerAngle {
    char pad00[0x28];
    float angle;
} KonquestTriggerAngle;

typedef struct KonquestTriggerOrientation {
    char pad00[0x10];
    KonquestTriggerAngle* angle;
} KonquestTriggerOrientation;

struct KonquestTriggerRequirement {
    int type;
    int field_04;
    Vec position;
    char pad14[0x10];
    KonquestTriggerOrientation* orientation;
};

typedef struct KonquestDialogKillPdata {
    MkHdr hdr;
    int attempts;
} KonquestDialogKillPdata;

typedef struct KonquestDynamicPuiPdata {
    MkHdr hdr;
    KonquestPuiDefinition* item;
    int source_type;
    Vec position;
    union {
        void* source;
        KonquestTriggerStruct* trigger_source;
        KonquestNpc* npc_source;
    }; /* +0x1C */
    int critical;
} KonquestDynamicPuiPdata;

typedef char KonquestDynamicPuiPdataSizeCheck[
    sizeof(KonquestDynamicPuiPdata) == 0x24 ? 1 : -1];

typedef struct KonquestDoorPdata {
    MkHdr hdr;
    int open_ticks;
    int remain_open;
    KonquestChildObject* door;
    int play_sound;
} KonquestDoorPdata;

typedef struct KonquestAttachedSound {
    MkHdr hdr;
    char pad08[0x24];
    int enabled; /* +0x2C */
    int uid; /* +0x30 */
} KonquestAttachedSound;

typedef struct KonquestTile {
    char pad00[4];
    int visible; /* +0x04 */
    char pad08[0x0C];
    MkSobj* scene; /* +0x14 */
    MkPtr* objects; /* +0x18 */
    char pad1C[4];
    unsigned int collision_art_id; /* +0x20 */
    MkPtr* collisions; /* +0x24 */
    CollisionObjList* collision_object_list; /* +0x28 */
    int collisions_active; /* +0x2C */
} KonquestTile;

extern AnimPdata* anim_pdata;
extern KonquestProfile* p1_profile_konquest;
extern KonquestNpc* g_active_npc;
extern int mode_of_play;
extern int screen_width;
extern int screen_height;
extern int pause_player;
extern int target_game_mode;
extern unsigned int monk_ground_colls[];
extern unsigned int monk_laying_on_ground_colls[];
extern KonquestRegionAsset konquest_region_data[9];
extern MkFileEntry konquest_common_file_table[];
extern MkFileEntry kon_earthrealm_0_file_table[];
extern MkFileEntry kon_earthrealm_1_file_table[];
extern MkFileEntry kon_earthrealm_2_file_table[];
extern MkFileEntry kon_netherrealm_1_file_table[];
extern MkFileEntry kon_chaosrealm_1_file_table[];
extern MkFileEntry kon_outworld_1_file_table[];
extern MkFileEntry kon_orderrealm_1_file_table[];
extern MkFileEntry kon_edenia_1_file_table[];
extern MkFileEntry kon_nexus_1_file_table[];
extern MkFileEntry kq_er1_fight_file_table[];
extern MkFileEntry kq_er2_fight_file_table[];
extern MkFileEntry kq_nr1_fight_file_table[];
extern MkFileEntry kq_cr1_fight_file_table[];
extern MkFileEntry kq_ow1_fight_file_table[];
extern MkFileEntry kq_or1_fight_file_table[];
extern MkFileEntry kq_ed1_fight_file_table[];
extern MkFileEntry kq_nx1_fight_file_table[];
extern MkFileEntry krypt_art_file_table[];
extern MkFileEntry konquest_pui_award_tgas_file_table[];
extern MkFileInfo sec_konquest_common_art;
extern MkFileInfo sec_konquest_monk_anims;
extern MkFileInfo sec_konquestdata;
extern MkFileInfo sec_konquest_map_common;
extern char danton20_charmap[];
extern int text_window_state;
extern KonquestSwitchPdata* switch_pdata;
static MonkStateData monk_state_data[29];
extern CmdScript* active_cmdscript;
extern KonquestCommonProfile* p1_profile_common;
extern unsigned long long default_char_bits;
extern unsigned long long default_alt_char_bits;
extern int konquest_human_bones[17];
extern MkFlippedBoneMap flipped_konquest_human_bones;
extern int f_writing_konquest_profile;
extern int b_game_timer_off;
extern MkVtable5 vtbl_trigger_struct;
extern MkVtable5 vtbl_konquest_pui;
static MkVtable5 vtbl_konquest_sobj_struct;
static MkVtable5 vtbl_konquest_obj;
extern KonquestDayOfWeek days_of_week[7];
typedef struct KonquestShadowData {
    char pad000[0x470];
    MkObj* ground_object; /* +0x470 */
    char pad474[0x2D8];
} KonquestShadowData;
static KonquestShadowData pdata_monk;
int konq_nis_anims[0x14];
KonquestSavedState konquest_save_data;
int konquest_animations[0x24];

static int konquest_save_on_exit = 1;
static const char* inv_item_default_string = "";
static const char* inv_item_empty_string = "";

KonquestTime* g_pui_events;
int konquest_editor_mode_on;
int konquest_data_loaded;
static int in_exit_meditation;
static int fix_camera_flip;
static int trigger_update_countdown;
static int g_fade_hud_in;
static float sun_angle_change_per_tick;
static float game_hours_per_tick;
static float ticks_per_hour;
static float ticks_per_game_day;
static float original_game_speed;
static MslSoundHandle meditate_stream;
static MkPtr* nis_participants;
void* konquest_prize_description_block;
CoffinEntry* chest_data;
KonquestPdata* konquest_pdata;

extern float game_speed;
extern int menu_player;
extern MkPtr* special_light_list;
extern float fog_color_real[4];
extern float fog_density;
extern float fog_distance;
extern int fog_type;
extern unsigned long display_off;
extern int use_feedback_effect;
extern int feedback_blendrate;
const char* get_pause_menu_name(void);
int get_pause_menu_ssh(void);
float p_main_menu(void);
void unload_player_profiles(void);
void wait_for_screen_close(void);
void set_background_color(int red, int green, int blue, int alpha);
void* get_data_table_by_name(const char* name);
char* get_name_of_table(ScriptSlot* owner, unsigned int index);
static Vec old_hero_position;

void* memcpy(void* dst, const void* src, unsigned long size);
void* memset(void* dst, int value, unsigned long size);
int sprintf(char* destination, const char* format, ...);
void* get_screen_pdata(void);
static float p_konquest_fade_screen(void);
static float p_hero_portal_in(void);
static float p_hero_use_portal(void);
static float p_hero_teleport(void);
static float p_show_fight_message(void);
float snd_get_game_vol(void);
void snd_set_game_vol(float volume);
extern float game_volume;
void run_interaction_camera_script(void* owner, void* script);
void set_konq_profile_value(int category, int index, int value);
int get_konq_profile_value(int category, int index);
void snd_req_delay(int sound, int delay);
int random_snd_req(int group);
void random_snd_req_delay(int group, int delay);
double pow(double base, double exponent);
void set_game_speed(float speed);
static void update_visible_tiles(void);
static void hide_currently_visible_tiles(void);
void cleanup_npc_manager(void);
void npc_shadow_teardown(void);
void destroy_konquest_shadow_collision_lists(void);
void cleanup_mission_state(void);
int is_this_the_monk_npc(KonquestNpc* npc);
KonquestNpc* find_npc_by_data(KonquestNpcData* data);
void npc_make_visible(KonquestNpcRuntime* npc);
void npc_force_state_for_npc(KonquestNpcRuntime* npc, int state);
void npc_take_control_of_him(void* data, void* script_function);
void npc_set_wait_ticks(float ticks);
void npc_suspend_cmdscript(void);
void npc_blend_to_ani(
    int animation_id, unsigned int flags, float blend, float speed);
static float p_konquest_interaction(void);
static float p_konquest_map_screen(void);
static float p_transition_to_fight(void);
static float p_load_hero_art_section(void);
int refresh_rate(void);
void insert_ground_me_mkobj(MkObj* object);
void build_bones_tbl(MkObj* object, void* bones);
void obj_apply_to_sobj_with_id(
    MkObj* object, int id, void (*callback)(void*));
int load_effect_bank(const char* name);
static void initialize_tile_patch_sobj(MkSobj* object);
static void generate_door_paths(void);
static void generate_door_trigger(
    KonquestDoorObject* door, const Vec* position, float radius);
void profile_region_change(void);
int mslSoundIsValid(MslSoundHandle handle);
void set_snd_vol(MslSoundHandle handle, int sound, float volume);
MslSoundHandle snd_req_vol(int sound, float volume);
void* get_script_function_by_name(ScriptSlot* owner, const char* name);
void del_string_obj_by_id(int id);
void sobj_set_priority(void* object, int priority);
void sobj_use_material_color(MkSobj* object);
void update_mksobj(MkSobj* object);
void npc_shadow_set_alpha(int alpha);
void npc_shadow_set_light_angle(const Vec* angles);
void hide_atomic(void* atomic);
MkProc* _create_mkproc_generic_tinystack(
    int pid, int priority, void* entry, int pdata_size, void** pdata);
MkProc* _create_mkproc_generic_bigstack(
    int pid, int priority, void* entry, int pdata_size, void** pdata);
MkProc* _create_mkproc_generic_nostack(
    int pid, int priority, void* entry, int pdata_size, void** pdata);
void zero_pdata_payload(int size, void* pdata);
static float p_konquest_kill_dialog_procs(void);
void run_camera_script(void* owner, void* script, int flags);
void destroy_mkprocs_pid(int pid);
void kill_lip_sync_procs(void);
float duration_of_lip_sync(const LipSyncKeyframe* keyframes);
int get_mode_of_play(void);
double ceil(double value);
static float p_konquest_dialog(void);
static float p_konquest_pause_menu(void);
static void vdestroy_dialog_pdata(KonquestDialogPdata* pdata);
static MkVtable5 vtbl_dialog_pdata;
float konquest_camera_loop(void);
void npc_ani_1_frame(void);
static float p_fade_ambient_sounds(void);
static MkProc* create_dialog_proc(
    KonquestDialogArt* dialog_art, const char* text, int print_speed);
int spawn_dynamic_pui_at_pos(
    KonquestPuiDefinition* item, int source_type, const Vec* position,
    void* source, int critical);
static int is_it_safe_to_spawn_pui(KonquestPuiDefinition* item);
void set_process_as_scriptable(MkProc* proc);
int transition_to_anim_script_frame(
    float transition_frames, float frame, AnimPdata* animation,
    AnimScript* script, unsigned int flags);
void insert_on_collision_obj_list(
    MkHdr* object, CollisionObjList* list);


typedef struct KonquestFadeDestroyVtable {
    void* reserved[4];
    void (*destroy)(ScreenObj* object);
} KonquestFadeDestroyVtable;


typedef struct KonquestInventoryImageList {
    RwTexture** images;
    RwTexture** alpha_images;
} KonquestInventoryImageList;

typedef struct KonquestInventoryScreenPdata {
    char pad00[0x2F8];
    int selected_item;
} KonquestInventoryScreenPdata;

typedef struct KonquestPuiFlags {
    unsigned char bit7 : 1;
    unsigned char bit6 : 1;
    unsigned char bit5 : 1;
    unsigned char bit4 : 1;
    unsigned char bit3 : 1;
    unsigned char bit2 : 1;
    unsigned char saved : 1;
    unsigned char bit0 : 1;
} KonquestPuiFlags;

typedef struct KonquestPuiEffectView {
    MkHdr hdr;
    char pad08[0x0C];
    MkObj* object;
    unsigned int object_instance;
} KonquestPuiEffectView;

typedef struct KonquestPuiPfxSequenceRow KonquestPuiPfxSequenceRow;

struct KonquestPuiRuntime {
    MkHdr hdr;
    union {
        KonquestChestOwner* owner;
        KonquestTriggerStruct* trigger_owner;
    }; /* +0x08 */
    unsigned int owner_instance; /* +0x0C */
    union {
        unsigned int id;
        KonquestPuiDefinition* item;
    }; /* +0x10 */
    char* table_name; /* +0x14 */
    int inventory_index; /* +0x18 */
    union {
        int state;
        int behavior;
    }; /* +0x1C */
    union {
        MkObj* render_object;
        MkObj* object;
    }; /* +0x20 */
    union {
        unsigned int render_object_instance;
        unsigned int object_instance;
    }; /* +0x24 */
    union {
        unsigned int flags_word;
        struct {
            union {
                unsigned char flags;
                KonquestPuiFlags flag_bits;
            };
            char flags_pad[3];
        };
    }; /* +0x28 */
    int alpha; /* +0x2C */
    int bounce_count; /* +0x30 */
    float drop_timer; /* +0x34 */
    union {
        float spawn_delay;
        float lifetime;
    }; /* +0x38 */
    float fade_delay; /* +0x3C */
    float base_y; /* +0x40 */
    KonquestTime kill_time; /* +0x44 */
    union {
        KonquestPuiPfxSequenceRow* effect_sequence;
        unsigned int attached_effect;
    }; /* +0x5C */
    unsigned int effect_60; /* +0x60 */
    unsigned int effect_64; /* +0x64 */
    PfxClone* effect_clone; /* +0x68 */
    CollisionObj* collision_object; /* +0x6C */
};

typedef char KonquestPuiRuntimeSizeCheck[
    sizeof(KonquestPuiRuntime) == 0x70 ? 1 : -1];

struct KonquestPuiPfxSequenceRow {
    float delay;
    void* effect_owner;
};

typedef struct KonquestPuiPfxSequencePdata {
    MkHdr hdr;
    Vec position;
    KonquestPuiPfxSequenceRow* sequence;
    float delay;
    unsigned int index;
} KonquestPuiPfxSequencePdata;

/* Retail order requires these callees to be declared before earlier callers. */
static void konquest_fade_screen(
    int ticks, int white, int fade_sound, int to_black);
static float p_show_koin_award_text(void);
static float p_pui_pfx_sequence(void);
static KonquestPuiRuntime* create_new_konquest_pui(
    KonquestPuiDefinition* item, int behavior, int position_mode);
static int konquest_pui_check_for_and_replace_old_chest(
    KonquestPuiDelayView* new_pui);
static void p_spawn_dynamic_pui(void);
void kill_dynamic_pui(KonquestPuiDefinition* item);
static float p_konquest_nis_housekeeping(void);
void nis_end_scene(void);
float p_adjust_directional_light(void);
float p_adjust_sky_ambient_light(void);
float p_adjust_ambient_light(void);
void execute_trigger(KonquestTriggerStruct* trigger);
KonquestTriggerStruct* find_trigger_by_id(unsigned int id);
void* get_nth_tile_struct(int index);
KonquestChildObject* find_door_partner_sobj(KonquestChildObject* door);
static RpAtomic* hide_an_atomic(RpAtomic* atomic, void* data);
static void set_objective_beam_scale(float distance);
void pui_update(void);

typedef struct KonquestPuiListNode {
    KonquestPuiDelayView* object;
    struct KonquestPuiListNode* next;
    char pad08[8];
    unsigned int object_instance; /* +0x10 */
} KonquestPuiListNode;


void add_days_to_time(void* time, int days);
void add_hours_to_time(void* time, int hours);
void add_minutes_to_time(void* time, int minutes);
void add_months_to_time(void* time, int months);
void add_years_to_time(void* time, int years);





void update_tile_grid(void);
static float p_collide_monk(void);
int collide_segment_against_global_collision_list_quads(
    const Vec* start, const Vec* end, Vec* hit_point);
int repel_against_global_collision_list(
    const Vec* position, Vec* movement, Vec* collision_position);
static MkProc* konquest_display_award_tga(
    KonquestPuiDefinition* item, int value, int mode);
static float p_display_award_image(void);
static void destroy_award_art(KonquestAwardArtPdata* pdata);
static void load_art_for_inventory_award(KonquestAwardArtPdata* pdata);
int get_game_state(void);
int camera_is_pos_move_done(void);
int get_konquest_game_mode(void);
int interaction_cam_glitched(void);
static int adjust_sky_for_new_time_of_day(
    KonquestSkyRow* sky_table, int last_row);
static void correct_camera_angle(MkObj* hero, int snap);
/*
 * Soft ceiling: konquest_check_possible_interact_with_npc ~92.55% -- the
 * validated hero/NPC latches, stale-list cleanup, distance and facing math,
 * and best-candidate stores match retail. Residue is two folded join branches
 * plus floating-point register scheduling; the source is eight bytes shorter.
 */
static KonquestNpc* konquest_check_possible_interact_with_npc(
    float* distance, float* facing_angle);
/*
 * Near match at exact retail size: both trigger modes, inverse-length steps,
 * NaN-sensitive angle tests, and orientation-side test agree. Residue is
 * stmw/lmw selection, floating-register allocation, and equivalent multiply/
 * add scheduling inside the duplicated normalization blocks.
 */
static int check_additional_trigger_fire_requirements(
    KonquestTriggerStruct* owner, MkObj* hero);
void npc_signal_event(KonquestNpc* npc, int event_index);
int npc_hit_by_punch(
    KonquestNpc* npc, float maximum_distance, float maximum_angle);
void set_interior_cam_pos_and_ang(void);
MslSoundHandle snd_req(int sound);
void snd_stop(MslSoundHandle sound);
MslSoundHandle pan_vol_snd_req(int sound, float pan, float volume);
void* get_data_table(ScriptSlot* owner, unsigned int index);
ScriptSlot* cmdscript_loadfile_by_name(int language, const char* name);
static float p_display_konquest_title(void);
float p_show_text_window(void);
static float p_fade_konquest_hud(void);
static float p_close_konquest_chest(void);
static MkProc* pui_set_chest_state(
    KonquestPuiDefinition* item, int direction);
static void pui_restore_open_chests(KonquestPuiDefinition* item);
static void handle_trigger_preprocess(KonquestTriggerStruct* trigger);
static void check_and_act_on_trigger_timed_action(
    KonquestTriggerStruct* trigger);
int is_pui_in_current_interior(const void* pui);
void konquest_hide_hud(int unused);
void konquest_show_hud(void);
void display_load_meter(int section_slot);
void npc_shadow_init(void);
void start_running_npcs(void);
static void konquest_restore_saved_state(void);
void fade_to_black(int ticks, int freeze);
void gamelogic_jump(int mode, void* entry);
float p_konquest_mode(void);
static float p_init_konquest_mode(void);
float p_setup_konquest_map(void);
float p_atm_loop(void);
void p_gamelogic(void);
void RwResourcesSetArenaSize(int size);
void set_global_collision_callback(int (*callback)(unsigned int*));
int npc_collision_callback(unsigned int* collision_id);
int get_building_id_for_exterior(void);
int get_primary_door_enum_for_exterior(void);
CollisionObjList* get_collision_obj_list(void);
void generate_shadow_collision_objects(int handle, unsigned int art_id);
void initialize_npc_data(void);
void initialize_konquest_interior(void);
void konquest_nav_init(void);
KonquestNpc* konquest_make_monk_an_npc(void);
char* get_string_by_id(unsigned int id);
static void hide_tile_objects(KonquestTileRecord* tile);
static void unhide_tile(KonquestTileRecord* tile);
static void hide_tile(KonquestTileRecord* tile);
static void konquest_update_true_clipped_tiles(void);
MkObj* get_pickup_object(void);
void set_monk_position(float x, float y, float z, float angle);
MkProc* load_hero_model(int animation_script);
void konquest_open_door_sobj(KonquestChildObject* door, int remain_open);
static void p_konquest_open_door(void);
static void remove_collisions_from_tile_and_tile_objects(
    KonquestTileRecord* tile);
static void generate_collisions_for_tile_and_tile_objects(
    KonquestTileRecord* tile);
void generate_collision_objects(
    int handle, unsigned int art_oid, const Vec* position, const Vec* angles,
    MkPtr** secondary_list);
void set_flag_for_all_collisions(MkPtr** list, unsigned int flags);
void exclusive_or_flags_for_all_collisions(
    MkPtr** list, unsigned int flags);
void purge_global_collision_list(void);
void remove_collision_list_from_konquest_shadow_lists(MkHdr* collision_list);
void insert_collision_list_on_konquest_shadow_lists(MkHdr* collision_list);
static void show_konquest_object(KonquestUidObject* object);
/*
 * Near match: object_transition_to_state 95.08% (1992 versus 2008 bytes).
 * State-table reloads, position/angle normalization, duration calculations,
 * sound gate, animated publication loop, and both final matrix paths agree.
 * Residue is inline-sqrt zero-path moves, four equivalent latch joins, and
 * FPR/stack-slot coloring.
 */
static void object_transition_to_state(
    KonquestChildObject* object, int state, int play_sound);
static void object_set_state(KonquestChildObject* record, int state);
unsigned int fx(const char* name);
unsigned int fx_by_owner(const char* name, unsigned int owner);
unsigned int fx_next_emitter(unsigned int effect);
int emitter_id_from_handle(unsigned int emitter);
float get_game_speed(void);
void fx_set_param_v3(
    unsigned int effect, int param, float x, float y, float z);
void fx_reset(unsigned int effect);
void fx_reset_emit(unsigned int effect);
void remove_fgnd_mkobj(MkObj* object);
void fx_pause_emit(unsigned int effect);
void fx_restart_emit(unsigned int effect);
void fx_resume_emit(unsigned int effect);
void get_bone_offset_world_pos(
    MkObj* object, int bone, const Vec* offset, Vec* position);
unsigned int pfxhandle_spawn_at_bid_next(
    unsigned int effect, MkObj* object, int bone);
void obj_set_all_sobjs_priority(MkObj* object, int priority);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
void shake_camera(int ticks, float magnitude);
unsigned int get_table_index_by_pointer(void* owner, void* item);
char* get_name_of_table_by_pointer(ScriptSlot* owner, void* table);
unsigned int get_row_count_for_table_by_pointer(
    ScriptSlot* slot, void* table);
void cmdscript_execute(ScriptSlot* slot);
void cmdscript_setup_execution(ScriptSlot* slot, unsigned int func_index);
void cmdscript_set_parameters(CmdScript* script, unsigned int count, ...);
void set_u8_bit(unsigned char* bits, int count, int index, int value);
int get_u8_bit(unsigned char* bits, int count, int index);
unsigned long strlen(const char* text);
int is_time_a_greater_than_time_b(void* a, void* b);
static void exit_meditation(void);
void enable_trigger(KonquestTriggerDefinition* trigger, int enabled);
void pulsate_object(
    MkHdr* object, int pulse_type, int rise_ticks, int fall_ticks,
    float minimum, float maximum);
AniData* get_animation(void);
void transition_to_anim_script(
    AnimPdata* pdata, AniData* animation, int transition, float blend);
void free_mem(void* memory);
static float p_cross_fade_ambient_sounds(void);
static float p_head_tracking(void);
static float p_konquest_load_nis_anims(void);
static float p_konquest_transition_to_state(void);
static void load_and_init_konquest_map_specific_data(int region);
static void load_konquest_tiles(void);
static float p_run_trigger_script(void);
float p_konquest_loop(void);
static void update_konquest_pui(KonquestPuiRuntime* pui);
static void setup_konquest_pui(KonquestPuiRuntime* pui);
static void update_time_screen_objs(int update_all);
void npc_reset_all_timed_events(void);
void increment_day(KonquestTime* time);
void npc_update(int arg);
void trigger_update(int arg);
int nav_what_area_is_point_in(const Vec* position, int previous_area);
int is_point_inside_shadow_exclusion_zone(
    const Vec* position, float radius);
/*
 * Soft ceiling: adjust_light_for_new_time_of_day ~85.89% -- row selection,
 * midnight wrapping, all four interpolants, clamping, multiplier placement,
 * and RpLight publication agree. The remaining delta is typed row-pointer
 * induction, repeated-address scheduling, and FPR coloring; stop.
 */
/*
 * Near match: row selection, midnight wrap, four-channel interpolation,
 * clamping, and the RpLight update match retail. The 16-byte residue is
 * typed-pointer induction and equivalent floating-point scheduling; an
 * inline-expression form was measured and rejected because it regressed.
 */
static int adjust_light_for_new_time_of_day(
    RpLight* light, KonquestLightRow* light_table, int last_row);
static float p_control_konquest_monk(void);
static float p_monk_unconscious(void);
static float p_monk_getup(void);
static float p_monk_punch_react(void);
static float p_monk_punch(void);
static float p_monk_move(void);
static float ramp_game_speed(void);
static float p_monitor_meditation_time(void);
static float p_monk_meditate(void);
static void handle_monk_input(void);
float p_animate(void);
float p_anim_idle(void);
float p_npc_idle(void);
void npc_xfer(void* npc, MkProcEntryFn entry, int transition);
void npc_restart_his_normal_behavior(void* npc_data);
void hero_stop_moving(void);
MkObj* trial_get_monk(void);
void set_root_and_obj_movement_weights(
    float root_weight, float object_weight, AnimPdata* animation);
void npc_make_invisible(KonquestNpc* npc);
void make_damashi_npc(MkObj* object);
void vdestroy_konquest_pui(KonquestPuiRuntime* pui);
static void vdestroy_konquest_sobj_info(MkHdr* object);
static void vdestroy_konquest_sobj(KonquestSobj* object);
static void vdestroy_konquest_obj(KonquestObject* object);
static int is_in_range(KonquestTriggerStruct* trigger);
static int is_button_pressed(KonquestTriggerStruct* button);
static int is_leaving_area(KonquestTriggerStruct* trigger);

MkVtable5 vtbl_konquest_pui = {
    not_mkproc, is_mkpdata, not_mksobj, not_mkmaterial,
    (MkVtblFn)vdestroy_konquest_pui,
};

static MonkStateData monk_state_data[29] = {
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 0, 0, 1.0f, 0,
     p_monk_move},
    {2, {20, 60, -1, -1}, {8, 7, -1, -1}, 2, 0, 1.0f, 0,
     p_monk_move},
    {2, {15, 37, -1, -1}, {8, 7, -1, -1}, 2, 0, 1.0f, 0,
     p_monk_move},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 4, 0, 0.5f, 3,
     p_monk_punch},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 1, 0, 1.0f, 3,
     p_monk_move},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 1, 0, 0.5f, 0,
     p_monk_move},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 1, 0, 1.0f, 11,
     p_monk_move},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 1, 0, 0.5f, 8,
     p_monk_move},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 1, 0, 2.5f, 3,
     p_monk_meditate},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 1, 0, 1.0f, 0,
     p_monk_meditate},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 1, 0, 2.5f, 3,
     p_monk_meditate},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 4, 0, 0.75f, 3,
     p_monk_punch_react},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 4, 0, 0.75f, 3,
     p_monk_getup},
    {0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, 4, 0, 0.75f, 3,
     p_monk_unconscious},
};

KonquestDayOfWeek days_of_week[7] = {
    {{0x0A8C000C}, 0x80}, {{0x0A8C000D}, 0x80},
    {{0x0A8C000E}, 0x80}, {{0x0A8C000F}, 0x100},
    {{0x0A8C0010}, 0x100}, {{0x0A8C0011}, 0x80},
    {{0x0A8C0012}, 0x100},
};

KonquestRegionAsset konquest_region_data[9] = {
    {kon_earthrealm_0_file_table, kq_er1_fight_file_table, {"er0.mko"},
     "er1_mission_dialog_eng.mko", "EARTHREALM"},
    {kon_earthrealm_1_file_table, kq_er1_fight_file_table, {"er1.mko"},
     "er1_mission_dialog_eng.mko", "EARTHREALM"},
    {kon_earthrealm_2_file_table, kq_er2_fight_file_table, {"er2.mko"},
     "er2_mission_dialog_eng.mko", "EARTHREALM"},
    {kon_netherrealm_1_file_table, kq_nr1_fight_file_table, {"nr1.mko"},
     "nr1_mission_dialog_eng.mko", "NETHERREALM"},
    {kon_chaosrealm_1_file_table, kq_cr1_fight_file_table, {"cr1.mko"},
     "cr1_mission_dialog_eng.mko", "CHAOSREALM"},
    {kon_outworld_1_file_table, kq_ow1_fight_file_table, {"ow1.mko"},
     "ow1_mission_dialog_eng.mko", "OUTWORLD"},
    {kon_orderrealm_1_file_table, kq_or1_fight_file_table, {"or1.mko"},
     "or1_mission_dialog_eng.mko", "ORDERREALM"},
    {kon_edenia_1_file_table, kq_ed1_fight_file_table, {"ed1.mko"},
     "ed1_mission_dialog_eng.mko", "EDENIA"},
    {kon_nexus_1_file_table, kq_nx1_fight_file_table, {"nx1.mko"},
     "nx1_mission_dialog_eng.mko", "NEXUS"},
};

AgeProgressionEntry age_progression_table[4] = {
    {kon_earthrealm_2_file_table, "er2_time_progression_1.sec", 0x70,
     0x1C2, 0x71, 0x1C2, 0.05f, 0.84f, 0.9f},
    {kon_chaosrealm_1_file_table, "cr1_time_progression.sec", 0x72,
     0x1C2, 0x73, 0x1C2, 0.05f, 0.84f, 0.9f},
    {kon_earthrealm_2_file_table, "er2_time_progression_2.sec", 0x74,
     0x1F4, 0x75, 0x12C, 0.05f, 0.84f, 0.9f},
    {kon_orderrealm_1_file_table, "or1_time_progression.sec", 0x76,
     0x1C2, 0x77, 0x1C2, 0.05f, 0.84f, 0.9f},
};

static MkVtable5 vtbl_konquest_sobj_info = {
    not_mkproc, is_mkpdata, not_mksobj, not_mkmaterial,
    (MkVtblFn)vdestroy_konquest_sobj_info,
};

typedef int (*KonquestTriggerPredicate)(KonquestTriggerStruct* trigger);
static KonquestTriggerPredicate trigger_function_table[4] = {
    0, is_in_range, is_button_pressed, is_leaving_area,
};

static MkVtable5 vtbl_konquest_obj = {
    not_mkproc, is_mkpdata, not_mksobj, not_mkmaterial,
    (MkVtblFn)vdestroy_konquest_obj,
};

static MkVtable5 vtbl_konquest_sobj_struct = {
    not_mkproc, is_mkpdata, not_mksobj, not_mkmaterial,
    (MkVtblFn)vdestroy_konquest_sobj,
};

static Vec old_hero_position = {0.0f, 500.0f, 0.0f};

unsigned int monk_ground_colls[15] = {
    7, 0, 0, 0, 0x3DCCCCCD, 8, 0, 0, 0, 0x3DCCCCCD,
    0xFFFFFFFF, 0, 0, 0, 0,
};

unsigned int monk_laying_on_ground_colls[20] = {
    7, 0, 0, 0, 0x3D23D70A, 8, 0, 0, 0, 0x3D23D70A,
    0, 0, 0, 0, 0x3D99999A, 0xFFFFFFFF, 0, 0, 0, 0,
};

static MkVtable5 vtbl_dialog_pdata = {
    not_mkproc, is_mkpdata, not_mksobj, not_mkmaterial,
    (MkVtblFn)vdestroy_dialog_pdata,
};

int koin_strings[6] = {0xA3, 0xA7, 0xA5, 0xA4, 0xA2, 0xA6};

























/*
 * Soft ceiling: p_konquest_dialog ~96.81% -- tokenization, two-line wrapping,
 * overflow scrolling/retry, character printing, signed timing, process wait,
 * and final delay match retail. Residue is the compiler folding repeated
 * generation-latch joins plus local constant/string relocation labels.
 */










































typedef unsigned char KonquestPuiActionBuffer[0xDB];

typedef struct KonquestPuiEventRow {
    KonquestTime time; /* +0x00 */
    int action; /* +0x18 */
    KonquestPuiDefinition* pui; /* +0x1C */
} KonquestPuiEventRow; /* 0x20 */

static int pui_get_next_action(
    KonquestPuiDefinition* pui, KonquestPuiEventRow* event_table,
    unsigned int* index);
int is_valid_event_time(const KonquestTime* time);
int calc_next_occurrence_of_event(
    KonquestTime* occurrence, const KonquestTime* event,
    const KonquestTime* current_time);












static const KonquestDirectionalLightDef konquest_directional_light_default = {
    3, p_adjust_directional_light, 3,
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.6f, 0.0f, 0.0f}
};
static const KonquestAmbientLightDef konquest_sky_ambient_light_default = {
    1, p_adjust_sky_ambient_light, 8,
    {1.0f, 1.0f, 1.0f, 1.0f}
};
static const KonquestAmbientLightDef konquest_ambient_light_default = {
    1, p_adjust_ambient_light, 2,
    {0.06f, 0.06f, 0.075f, 1.0f}
};











/*
 * Soft ceiling: restore_collision_volume_on_object_with_uid ~78.6% -- code
 * size differs by one instruction; the search and restore operations match,
 * with a rotated r28-r31 allocation and loop-test block placement.
 */
















static KonquestRenderRecord* create_konquest_sobj_for_konquest_obj(
    KonquestUidObject* owner, int uid, const Vec* position, float angle);






/*
 * Soft ceiling: setup_sobj_for_tile_object ~97.3% -- allocation, metadata,
 * object latch, transform transfer, flags, and calls match. Residue is the
 * equivalent inequality booleanization, enumeration-scan GPR coloring, and
 * local string/zero-float relocation labels.
 */
static int setup_sobj_for_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition);
static int setup_pebble_system_for_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition);
static int setup_locator_for_tile_object(
    KonquestTileObjectDefinition* definition);
/*
 * Soft ceiling: the nested frame/object traversal and inlined container/plugin
 * conversions match retail. The 24-byte delta is individual r28-r31
 * saves/restores in place of stmw/lmw.
 */
/*
 * Soft ceiling: setup_children_sobjs_of_tile_object ~74.57% -- frame and
 * atomic-list traversal, type/plugin validation, next-link ordering, and setup
 * call match retail. The low fuzzy score is the 24-byte individual-save versus
 * stmw/lmw frame delta and its register-allocation cascade.
 */
static void setup_children_sobjs_of_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition);
/*
 * Soft ceiling: identical traversal and inlined conversions to the SOBJ
 * variant above; only individual r28-r31 saves/restores versus stmw/lmw remain.
 */
/*
 * Soft ceiling: setup_children_pebbles_of_tile_object ~74.57% -- the function
 * is the same proven traversal as its sobj twin with the pebble setup callee.
 * Residue is the same 24-byte save/restore and register-allocation difference.
 */
static void setup_children_pebbles_of_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition);






















void kill_konquest_dialog_procs(void) {
    void* pdata;
    void* proc;

    pdata = 0;
    proc = _create_mkproc_generic_tinystack(
        0x8248, 0x1f, p_konquest_kill_dialog_procs, 8, &pdata);
    if (proc != 0 && pdata != 0) {
        ((int*)pdata)[2] = 0;
    }
}

/*
 * Near match: the three-process retry loop, lip-sync teardown, attempt limit,
 * sleep, and float return agree. Residue is paired saves versus stmw/lmw and
 * local constant scheduling.
 */
static float p_konquest_kill_dialog_procs(void) {
    KonquestDialogKillPdata* pdata;

    pdata = (KonquestDialogKillPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    while ((find_mkproc_pid(0x9002) != 0 ||
            find_mkproc_pid(0xA012) != 0 ||
            find_mkproc_pid(0x9025) != 0) &&
           pdata->attempts < 2) {
        destroy_mkprocs_pid(0x9002);
        destroy_mkprocs_pid(0xA012);
        destroy_mkprocs_pid(0x9025);
        kill_lip_sync_procs();
        pdata->attempts++;
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

void konquest_fade_from_black(int ticks, int event) {
    konquest_fade_screen(ticks, 0, event, 0);
}

void konquest_fade_to_black(int ticks, int event) {
    konquest_fade_screen(ticks, 0, event, 1);
}
static inline ScreenObj* resolve_konquest_fade_object(
    KonquestFadePdata* pdata) {
    ScreenObj* object;

    object = pdata->object;
    if (object != 0) {
        if (object->instance == pdata->object_instance) {
            /* Valid screen-object latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    return object;
}


/*
 * Near matches: fade setup and both per-tick paths reproduce the retail
 * object latch, alpha clamp, and sound-volume behavior. Remaining differences
 * are non-algorithmic save/restore selection, equivalent latch branch layout,
 * minor instruction scheduling, and pooled constant/string relocation labels.
 */
static void konquest_fade_screen(
    int ticks, int white, int fade_sound, int to_black) {
    KonquestFadePdata* pdata;
    KonquestFadePdata* fade;
    ScreenObj* object;
    MkProc* proc;
    float volume_step;
    float volume;
    int alpha;

    if (find_mkproc_pid(0x2098) != 0) {
        return;
    }

    if (to_black == 0) {
        destroy_fade_box();
    }

    proc = (MkProc*)_create_mkproc_generic_nostack(
        0x2098, 0x1F, p_konquest_fade_screen,
        sizeof(KonquestFadePdata), (void**)&pdata);
    if (proc == 0) {
        return;
    }

    pdata->ticks = ticks;
    pdata->white = white;
    pdata->fade_sound = fade_sound;
    pdata->to_black = to_black;
    pdata->red = 0xFF;
    pdata->green = 0xFF;
    pdata->blue = 0xFF;
    if (to_black != 0) {
        pdata->alpha = 0;
    } else {
        pdata->alpha = 0xFF;
    }
    proc->flags_bits.skip_if_paused = 1;

    if (pdata->white == 1) {
        object = load_named_2d_pfxobj(
            0, 0x2056, "WHITE_FADEBOX", 0, 0xD);
    } else {
        object = load_named_2d_pfxobj(
            0, 0x2057, "FADEBOX", 0, 0xD);
    }
    if (object == 0) {
        return;
    }

    pdata->object = object;
    pdata->object_instance = object->instance;
    object->x = -50;
    object->y = -50;
    object->priority = 19;
    object->flag_bits.scaled = 1;
    object->scale_x = 50.0f;
    object->scale_y = 40.0f;

    fade = pdata;
    if (fade->to_black != 0) {
        volume_step = 1.0f / (255.0f / (float)fade->ticks);
        alpha = fade->alpha + (unsigned char)fade->ticks;
        if (alpha > 0xFF) {
            fade->alpha = 0xFF;
        } else {
            fade->alpha = alpha;
        }
        object = resolve_konquest_fade_object(fade);
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, fade->alpha);
            if (fade->fade_sound != 0) {
                volume = snd_get_game_vol() - volume_step;
                if (volume > 0.0f) {
                    snd_set_game_vol(volume);
                } else {
                    snd_set_game_vol(0.0f);
                }
            }
        }
    } else {
        volume_step = 1.0f / (255.0f / (float)fade->ticks);
        alpha = fade->alpha - (unsigned char)fade->ticks;
        if (alpha < 0) {
            fade->alpha = 0;
        } else {
            fade->alpha = alpha;
        }
        object = resolve_konquest_fade_object(fade);
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, fade->alpha);
            if (fade->fade_sound != 0) {
                volume = snd_get_game_vol() + volume_step;
                if (volume < game_volume) {
                    snd_set_game_vol(volume);
                } else {
                    snd_set_game_vol(game_volume);
                }
            }
        }
    }
}

static float p_konquest_fade_screen(void) {
    KonquestFadePdata* pdata;
    ScreenObj* object;
    float volume_step;
    float volume;
    int alpha;
    int complete;

    if (!g_game_info.feature_flags.bits.high_bit &&
        is_controller_removed() != 0) {
        return 1.0f;
    }

    pdata = (KonquestFadePdata*)apdata;
    if (pdata == 0) {
        return -1.0f;
    }

    object = resolve_konquest_fade_object(pdata);
    if (object == 0) {
        return -1.0f;
    }

    if (pdata->to_black != 0) {
        volume_step = 1.0f / (255.0f / (float)pdata->ticks);
        alpha = pdata->alpha + (unsigned char)pdata->ticks;
        if (alpha > 0xFF) {
            pdata->alpha = 0xFF;
        } else {
            pdata->alpha = alpha;
        }

        object = resolve_konquest_fade_object(pdata);
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, pdata->alpha);
            if (pdata->fade_sound != 0) {
                volume = snd_get_game_vol() - volume_step;
                if (volume > 0.0f) {
                    snd_set_game_vol(volume);
                } else {
                    snd_set_game_vol(0.0f);
                }
            }
            if (pdata->alpha == 0xFF) {
                complete = 1;
            } else {
                complete = 0;
            }
        } else {
            complete = 1;
        }
    } else {
        volume_step = 1.0f / (255.0f / (float)pdata->ticks);
        alpha = pdata->alpha - (unsigned char)pdata->ticks;
        if (alpha < 0) {
            pdata->alpha = 0;
        } else {
            pdata->alpha = alpha;
        }

        object = resolve_konquest_fade_object(pdata);
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, pdata->alpha);
            if (pdata->fade_sound != 0) {
                volume = snd_get_game_vol() + volume_step;
                if (volume < game_volume) {
                    snd_set_game_vol(volume);
                } else {
                    snd_set_game_vol(game_volume);
                }
            }
            if (pdata->alpha == 0) {
                complete = 1;
            } else {
                complete = 0;
            }
        } else {
            complete = 1;
        }

        if (complete != 0) {
            object = resolve_konquest_fade_object(pdata);
            if (object != 0 && object->instance != 0) {
                ((KonquestFadeDestroyVtable*)object->vtbl)->
                    destroy(object);
            }
        }
    }

    return complete != 0 ? -1.0f : 1.0f;
}
static inline int konquest_game_mode_in_stack(int game_mode) {
    int index;

    for (index = 0; index <= konquest_pdata->game_mode_index; index++) {
        if (konquest_pdata->game_modes[index] == game_mode) {
            return 1;
        }
    }
    return 0;
}


/*
 * Near match: the active intro-script check, inlined mode-stack scan, game
 * state gate, and result are complete. Residue is one equivalent loop-exit
 * branch and pooled string/constant relocation labeling.
 */
int konquest_is_save_allowed(void) {
    void* intro_script;
    MkProc* proc;
    KonquestCameraScriptPdata* pdata;

    intro_script = get_script_function_by_name(
        konquest_pdata->script_owner, "fight_intro_cam_1");
    proc = find_mkproc_pid(0x9006);
    if (proc != 0) {
        pdata = (KonquestCameraScriptPdata*)pdata_of_proc(proc);
        if (pdata != 0 && pdata->script == intro_script) {
            return 0;
        }
    }

    if (konquest_game_mode_in_stack(4) != 0 ||
        get_game_state() == 0x14) {
        return 0;
    }
    return 1;
}

void konquest_set_current_portal_uid(int uid) {
    konquest_pdata->current_portal_uid = uid;
}
static inline int konquest_current_game_mode(void) {
    if (konquest_pdata->game_mode_index < 0) {
        return 0;
    }
    return konquest_pdata->game_modes[konquest_pdata->game_mode_index];
}


/*
 * Near match: the mode-stack guards, HUD/hero/camera transitions, portal-row
 * process lifecycle, latch wait, and complete restoration path agree with
 * retail. Residue is equivalent latch/loop branch layout, nonvolatile-register
 * allocation, save/restore selection, and pooled relocation labels.
 */
void konquest_hero_portal_in(void) {
    ScreenObj* arrow;
    MkObj* beam;
    MkObj* hero;
    AnimPdata* animation;
    MkProc* animation_proc;
    KonquestGrounding* grounding;
    MkProc* proc;
    MkProc* portal_proc;
    unsigned int portal_instance;
    KonquestPortalPdata* portal_pdata;
    KonquestPortalRow* rows;
    KonquestPortalRow* row;
    unsigned int row_count;
    unsigned int index;
    int mode;

    if (konquest_current_game_mode() != 0 ||
        konquest_game_mode_in_stack(9) != 0 ||
        konquest_pdata->current_portal_uid == 0) {
        return;
    }

    p1_profile_konquest->fields.portal_entry_pending = 0;
    if (konquest_pdata->game_mode_index < 0) {
        mode = 0;
    } else {
        mode = konquest_pdata->game_modes[konquest_pdata->game_mode_index];
    }
    if (mode != 9 && konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 9;
    }

    konquest_hide_hud(0);
    arrow = (ScreenObj*)konquest_pdata->hud_objects[2].object;
    if (arrow != 0) {
        if (arrow->instance == konquest_pdata->hud_objects[2].instance) {
            /* Valid HUD-arrow latch. */
        } else {
            arrow = 0;
        }
    } else {
        arrow = 0;
    }
    beam = (MkObj*)konquest_pdata->objective_beam.object;
    if (beam != 0) {
        if (beam->hdr.instance == konquest_pdata->objective_beam.instance) {
            /* Valid objective-beam latch. */
        } else {
            beam = 0;
        }
    } else {
        beam = 0;
    }
    if (arrow != 0 && beam != 0) {
        MkSobj* sky_object;

        hide_screen_obj(arrow);
        hide_obj(beam);
        if (g_game_info.sky != 0) {
            sky_object = obj_find_sobj_by_id(g_game_info.sky, 1);
            if (sky_object != 0) {
                hide_sobj(sky_object);
            }
        }
        konquest_pdata->objective_visible = 0;
    }

    turn_controllers_off();
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        hero->pos_vel.z = 0.0f;
        hero->pos_vel.y = 0.0f;
        hero->pos_vel.x = 0.0f;
        hero->flags_09_bits.bit6 = 1;
        konquest_pdata->hero_state = 0;
        hero->hide_flag_bits.still_move = 1;
        konquest_pdata->hero_anim->weight = 1.0f;
    }
    transition_to_anim_script(
        konquest_pdata->hero_anim,
        monk_state_data[konquest_pdata->hero_state].animation,
        monk_state_data[konquest_pdata->hero_state].transition, 0.1f);
    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        animation_proc = animation->proc;
        if (animation_proc != 0) {
            if (animation_proc->instance == animation->proc_instance) {
                /* Valid animation-process latch. */
            } else {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        xfer_proc(animation_proc, p_animate);
    }
    xfer_proc(get_camera_proc(), p_hold_camera_in_place);
    turn_camera_on();

    portal_proc = 0;
    portal_instance = 0;
    proc = (MkProc*)_create_mkproc_generic_bigstack(
        0xA028, 0x1F, p_hero_portal_in, sizeof(*portal_pdata),
        (void**)&portal_pdata);
    if (proc != 0 && portal_pdata != 0) {
        rows = (KonquestPortalRow*)get_data_table_by_name("region_portals");
        row_count = get_row_count_for_table_by_pointer(
            konquest_pdata->script_owner, rows);
        row = 0;
        for (index = 0; index < row_count; index++) {
            KonquestPortalRow* candidate;

            candidate = &rows[index];
            if (candidate->uid == konquest_pdata->current_portal_uid) {
                row = candidate;
                break;
            }
        }
        if (row != 0) {
            portal_instance = proc->instance;
            portal_proc = proc;
            zero_pdata_payload(sizeof(*portal_pdata), portal_pdata);
            portal_pdata->uid = row->uid;
            portal_pdata->target_offset.x = row->target_offset.x;
            portal_pdata->target_offset.y = row->target_offset.y;
            portal_pdata->target_offset.z = row->target_offset.z;
            portal_pdata->camera_y_offset = row->camera_y_offset;
            portal_pdata->hero_distance = row->hero_distance;
            portal_pdata->camera_distance = row->camera_distance;
            portal_pdata->field_24 = row->field_1C;
        } else if (proc->instance != 0) {
            proc->hdr.typed_vtbl->destroy(&proc->hdr);
        }
    }

    for (;;) {
        proc = portal_proc;
        if (proc != 0) {
            if (proc->instance == portal_instance) {
                /* Valid portal-process latch. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        if (proc == 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    arrow = (ScreenObj*)konquest_pdata->hud_objects[2].object;
    if (arrow != 0) {
        if (arrow->instance == konquest_pdata->hud_objects[2].instance) {
            /* Valid HUD-arrow latch. */
        } else {
            arrow = 0;
        }
    } else {
        arrow = 0;
    }
    beam = (MkObj*)konquest_pdata->objective_beam.object;
    if (beam != 0) {
        if (beam->hdr.instance == konquest_pdata->objective_beam.instance) {
            /* Valid objective-beam latch. */
        } else {
            beam = 0;
        }
    } else {
        beam = 0;
    }
    if (arrow != 0 && beam != 0 &&
        p1_profile_konquest->fields.objective_index <
            (int)get_row_count_for_table_by_pointer(
                konquest_pdata->script_owner,
                konquest_pdata->objective_table)) {
        if (konquest_pdata->hud_visible != 0) {
            unhide_screen_obj(arrow);
        }
        unhide_obj(beam);
        konquest_pdata->objective_visible = 1;
    }

    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        animation_proc = animation->proc;
        if (animation_proc != 0) {
            if (animation_proc->instance == animation->proc_instance) {
                /* Valid animation-process latch. */
            } else {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        xfer_proc(animation_proc, p_control_konquest_monk);
    }
    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
    }
    xfer_proc(get_camera_proc(), p_konquest_camera_proc);
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
}
static inline float konquest_inverse_length(float length_squared) {
    KonquestFloatBits estimate;
    float product;
    float correction;
    float inverse;

    inverse = 0.0f;
    if (length_squared <= 0.0f) {
        /* Degenerate direction. */
    } else {
        estimate.value = length_squared;
        estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
        product = estimate.value * (length_squared * estimate.value);
        correction = 3.0f - product;
        inverse = 0.0625f * estimate.value * correction *
                  -(correction * (product * correction) - 12.0f);
    }
    return inverse;
}


/*
 * Near match: the tile/list search, portal geometry, all three inverse-length
 * expansions, camera/effect sequence, alpha/priority changes, animation frame
 * conversion, sleeps, and return are complete. The remaining narrow residue
 * is register allocation, equivalent latch/float-branch layout, minor
 * scheduling, and pooled constant/string relocation labels.
 */
static float p_hero_portal_in(void) {
    KonquestPortalPdata* pdata;
    KonquestGrounding* grounding;
    KonquestObject* portal;
    KonquestRenderRecord* record;
    MkObj* hero;
    MkPtr* link;
    Vec position;
    float delta_z;
    float delta_x;
    float inverse_length;
    float flash_x;
    float flash_y;
    float flash_z;
    unsigned int effect;
    int tile_index;
    int tile_offset;
    int portal_uid;

    pdata = (KonquestPortalPdata*)pdata_of_proc(aproc);
    portal_uid = pdata->uid;

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    portal = 0;
    tile_index = 0;
    tile_offset = 0;
    while (tile_index <
           konquest_pdata->tile_width * konquest_pdata->tile_height) {
        KonquestTileRecord* tile;

        tile = (KonquestTileRecord*)((char*)konquest_pdata->tile_structs +
                                     tile_offset);
        if (tile != 0 && tile->objects != 0) {
            link = tile->objects;
            while (link != 0) {
                KonquestObject* candidate;

                candidate = (KonquestObject*)link->hdr;
                if (link->instance != candidate->hdr.instance) {
                    MkPtr* next;

                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else if (candidate->uid == portal_uid) {
                    portal = candidate;
                    break;
                } else {
                    link = link->next;
                }
            }
        }
        if (portal != 0) {
            break;
        }
        tile_index++;
        tile_offset += sizeof(*tile);
    }

    /* The region portal table guarantees a live render record here. */
    record = (KonquestRenderRecord*)first_mkhdr(&portal->list_4C);
    position.z = 0.0f;
    position.y = 0.0f;
    position.x = 0.0f;

    delta_x = hero->pos.value.x - record->field_14.x;
    delta_z = hero->pos.value.z - record->field_14.z;
    inverse_length =
        konquest_inverse_length(delta_x * delta_x + delta_z * delta_z);
    position.x = delta_x * inverse_length * pdata->hero_distance;
    position.z = delta_z * inverse_length * pdata->hero_distance;
    hero->pos.value.x = record->field_14.x + position.x;
    hero->pos.value.z = record->field_14.z + position.z;

    position.x = hero->pos.value.x - record->field_14.x;
    position.z = hero->pos.value.z - record->field_14.z;
    hero->ang.y = gxMathArcTanYX(position.x, position.z);
    update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
    obj_set_all_sobjs_priority(hero, 0x13);

    grounding->camera_target->alpha = 0;
    obj_for_all_atomics_set_material_alpha(
        hero, grounding->camera_target->alpha);

    delta_x = hero->pos.value.x - record->field_14.x;
    delta_z = hero->pos.value.z - record->field_14.z;
    inverse_length =
        konquest_inverse_length(delta_x * delta_x + delta_z * delta_z);
    position.x = delta_x * inverse_length * pdata->camera_distance;
    position.z = delta_z * inverse_length * pdata->camera_distance;
    position.y = pdata->camera_y_offset;
    position.x += record->field_14.x;
    position.y += record->field_14.y;
    position.z += record->field_14.z;
    position.y += pdata->target_offset.y;
    set_camera_position(&position);

    position.x = record->field_14.x + pdata->target_offset.x;
    position.y = record->field_14.y + pdata->target_offset.y;
    position.z = record->field_14.z + pdata->target_offset.z;
    look_at_target(&position);

    position.x = record->field_14.x + pdata->target_offset.x;
    position.y = record->field_14.y + pdata->target_offset.y;
    position.z = record->field_14.z + pdata->target_offset.z;
    effect = fx_by_owner("portal_dissolve", 4);
    fx_set_param_v3(
        effect, 0x201, position.x, position.y, position.z);
    fx_reset_emit(effect);
    pfxhandle_spawn_at_bid_next(effect, hero, 0);
    snd_req(0x1594);
    _mkproc_sleep_ticks = 60.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("portal_form_hero", 4);
    fx_reset_emit(effect);
    pfxhandle_spawn_at_bid_next(effect, hero, 0);
    _mkproc_sleep_ticks = 15.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    snd_req(0x1595);
    _mkproc_sleep_ticks = 15.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("portal_flash", 4);
    delta_x = hero->pos.value.x - position.x;
    delta_z = hero->pos.value.z - position.z;
    inverse_length =
        konquest_inverse_length(delta_x * delta_x + delta_z * delta_z);
    delta_x *= inverse_length;
    delta_z *= inverse_length;
    flash_x = 0.6f * delta_x;
    flash_y = record->field_14.y + pdata->target_offset.y;
    flash_z = 0.6f * delta_z;
    flash_x += record->field_14.x;
    flash_z += record->field_14.z;
    fx_set_param_v3(effect, 0x202, flash_x, flash_y, flash_z);
    fx_reset_emit(effect);
    fx_resume_emit(effect);
    shake_camera(5, 0.025f);
    _mkproc_sleep_ticks = 5.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    fade_to_white(30, 0);

    fx_reset_emit(fx_by_owner("portal_dissolve", 4));
    fx_reset_emit(fx_by_owner("portal_form_hero", 4));
    grounding->camera_target->alpha = 0xFF;
    obj_for_all_atomics_set_material_alpha(
        hero, grounding->camera_target->alpha);
    obj_set_all_sobjs_priority(hero, 0x10);

    {
        AnimScript* script;

        script = (AnimScript*)konquest_animations[31];
        set_anim_script_frame(
            (float)script->frame_count, konquest_pdata->hero_anim,
            (AniData*)script, 3);
    }
    konquest_pdata->hero_anim->step = -0.25f;
    _mkproc_sleep_ticks = 10.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("portal_flash2", 4);
    fx_set_param_v3(effect, 0x202, flash_x, flash_y, flash_z);
    fx_reset_emit(effect);
    fx_resume_emit(effect);
    fade_from_white(17, 0);
    while (konquest_pdata->hero_anim->frame > 0.0f) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    transition_to_anim_script(
        konquest_pdata->hero_anim,
        (AniData*)konquest_animations[1], 0, 0.1f);
    konquest_pdata->hero_anim->step = 0.75f;
    _mkproc_sleep_ticks = 60.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    return -1.0f;
}

/*
 * Near match: the mode guards, HUD fade and visibility changes, hero/camera
 * handoff, typed portal payload, validated process wait, profile update, and
 * mode pop all agree with retail. The remaining 24-byte delta is confined to
 * equivalent inline mode/latch branch layout, GPR allocation, and scheduling.
 */
void konquest_use_portal(
    int uid, const Vec* target_offset, int direction_mode,
    float camera_y_offset, float hero_distance, float camera_distance) {
    ScreenObj* arrow;
    MkObj* beam;
    MkObj* hero;
    AnimPdata* animation;
    MkProc* animation_proc;
    MkProc* proc;
    MkProc* portal_proc;
    KonquestPortalPdata* portal_pdata;
    unsigned int portal_instance;
    int mode;

    if (konquest_current_game_mode() != 0 ||
        konquest_game_mode_in_stack(9) != 0) {
        return;
    }

    if (konquest_pdata->game_mode_index < 0) {
        mode = 0;
    } else {
        mode = konquest_pdata->game_modes[konquest_pdata->game_mode_index];
    }
    if (mode != 9 && konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 9;
    }

    destroy_mkprocs_pid(0xA023);
    _create_mkproc_generic_tinystack(
        0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
    g_fade_hud_in = 0;

    arrow = (ScreenObj*)konquest_pdata->hud_objects[2].object;
    if (arrow != 0) {
        if (arrow->instance == konquest_pdata->hud_objects[2].instance) {
            /* Valid HUD-arrow latch. */
        } else {
            arrow = 0;
        }
    } else {
        arrow = 0;
    }
    beam = (MkObj*)konquest_pdata->objective_beam.object;
    if (beam != 0) {
        if (beam->hdr.instance == konquest_pdata->objective_beam.instance) {
            /* Valid objective-beam latch. */
        } else {
            beam = 0;
        }
    } else {
        beam = 0;
    }
    if (arrow != 0 && beam != 0) {
        MkSobj* sky_object;

        hide_screen_obj(arrow);
        hide_obj(beam);
        if (g_game_info.sky != 0) {
            sky_object = obj_find_sobj_by_id(g_game_info.sky, 1);
            if (sky_object != 0) {
                hide_sobj(sky_object);
            }
        }
        konquest_pdata->objective_visible = 0;
    }

    turn_controllers_off();
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        hero->pos_vel.z = 0.0f;
        hero->pos_vel.y = 0.0f;
        hero->pos_vel.x = 0.0f;
        hero->flags_09_bits.bit6 = 1;
        konquest_pdata->hero_state = 0;
        hero->hide_flag_bits.still_move = 1;
        konquest_pdata->hero_anim->weight = 1.0f;
    }
    transition_to_anim_script(
        konquest_pdata->hero_anim,
        monk_state_data[konquest_pdata->hero_state].animation,
        monk_state_data[konquest_pdata->hero_state].transition, 0.1f);
    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        animation_proc = animation->proc;
        if (animation_proc != 0) {
            if (animation_proc->instance == animation->proc_instance) {
                /* Valid animation-process latch. */
            } else {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        xfer_proc(animation_proc, p_animate);
    }
    xfer_proc(get_camera_proc(), p_hold_camera_in_place);

    portal_proc = 0;
    portal_instance = 0;
    proc = (MkProc*)_create_mkproc_generic_bigstack(
        0xA028, 0x1F, p_hero_use_portal, sizeof(*portal_pdata),
        (void**)&portal_pdata);
    if (proc != 0) {
        portal_instance = proc->instance;
        portal_proc = proc;
        zero_pdata_payload(sizeof(*portal_pdata), portal_pdata);
        portal_pdata->uid = uid;
        portal_pdata->target_offset.x = target_offset->x;
        portal_pdata->target_offset.y = target_offset->y;
        portal_pdata->target_offset.z = target_offset->z;
        portal_pdata->camera_y_offset = camera_y_offset;
        portal_pdata->hero_distance = hero_distance;
        portal_pdata->camera_distance = camera_distance;
        portal_pdata->field_24 = direction_mode;
    }

    for (;;) {
        proc = portal_proc;
        if (proc != 0) {
            if (proc->instance == portal_instance) {
                /* Valid portal-process latch. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        if (proc == 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    p1_profile_konquest->fields.portal_entry_pending = 1;
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
}
/*
 * Near match: the stale-safe portal search, direction-mode geometry, camera
 * placement, animation/alpha sequence, effects, sounds, fades, and cleanup
 * agree with retail. The remaining 16-byte delta is stack/register allocation,
 * equivalent branch layout, scheduling, and pooled relocation labels.
 */
static float p_hero_use_portal(void) {
    KonquestPortalPdata* pdata;
    KonquestGrounding* grounding;
    KonquestObject* portal;
    KonquestRenderRecord* record;
    MkObj* hero;
    MkPtr* link;
    Vec direction;
    Vec position;
    float inverse_length;
    unsigned int effect;
    int tile_index;
    int tile_offset;
    int portal_uid;

    pdata = (KonquestPortalPdata*)pdata_of_proc(aproc);

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    portal = 0;
    portal_uid = pdata->uid;
    tile_index = 0;
    tile_offset = 0;
    while (tile_index <
           konquest_pdata->tile_width * konquest_pdata->tile_height) {
        KonquestTileRecord* tile;

        tile = (KonquestTileRecord*)((char*)konquest_pdata->tile_structs +
                                     tile_offset);
        if (tile != 0 && tile->objects != 0) {
            link = tile->objects;
            while (link != 0) {
                KonquestObject* candidate;

                candidate = (KonquestObject*)link->hdr;
                if (link->instance != candidate->hdr.instance) {
                    MkPtr* next;

                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else if (candidate->uid == portal_uid) {
                    portal = candidate;
                    break;
                } else {
                    link = link->next;
                }
            }
        }
        if (portal != 0) {
            break;
        }
        tile_index++;
        tile_offset += sizeof(*tile);
    }
    /* The portal command guarantees a live portal and render record. */
    record = (KonquestRenderRecord*)first_mkhdr(&portal->list_4C);
    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = 0.0f;
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;

    if (pdata->field_24 != 0) {
        direction.z = pdata->hero_distance;
        rotate_xz(&position, &direction, record->angles.y);
    } else {
        direction.x = hero->pos.value.x - record->field_14.x;
        direction.z = hero->pos.value.z - record->field_14.z;
        inverse_length =
            konquest_inverse_length(direction.x * direction.x +
                                    direction.z * direction.z);
        direction.x *= inverse_length;
        direction.z *= inverse_length;
        position.x = direction.x * pdata->hero_distance;
        position.z = direction.z * pdata->hero_distance;
    }
    hero->pos.value.x = record->field_14.x + position.x;
    hero->pos.value.z = record->field_14.z + position.z;

    position.x = record->field_14.x - hero->pos.value.x;
    position.z = record->field_14.z - hero->pos.value.z;
    hero->ang.y = gxMathArcTanYX(position.x, position.z);
    update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);

    if (pdata->field_24 != 0) {
        position.x = 0.0f;
        position.y = 0.0f;
        position.z = 0.0f;
        position.z = pdata->camera_distance;
        rotate_xz(&direction, &position, record->angles.y);
        position.x = record->field_14.x + direction.x;
        position.y = record->field_14.y + pdata->camera_y_offset;
        position.z = record->field_14.z + direction.z;
    } else {
        direction.x = hero->pos.value.x - record->field_14.x;
        direction.z = hero->pos.value.z - record->field_14.z;
        inverse_length =
            konquest_inverse_length(direction.x * direction.x +
                                    direction.z * direction.z);
        direction.x *= inverse_length;
        direction.z *= inverse_length;
        position.x = direction.x * pdata->camera_distance;
        position.z = direction.z * pdata->camera_distance;
        position.y = record->field_14.y + pdata->camera_y_offset;
        position.x += record->field_14.x;
        position.z += record->field_14.z;
    }
    position.y += pdata->target_offset.y;
    set_camera_position(&position);

    position.x = record->field_14.x + pdata->target_offset.x;
    position.y = record->field_14.y + pdata->target_offset.y;
    position.z = record->field_14.z + pdata->target_offset.z;
    look_at_target(&position);

    set_anim_script(
        konquest_pdata->hero_anim, (AniData*)konquest_animations[1], 0);
    konquest_pdata->hero_anim->step = 0.75f;

    position.x = record->field_14.x + pdata->target_offset.x;
    position.y = record->field_14.y + pdata->target_offset.y;
    position.z = record->field_14.z + pdata->target_offset.z;
    effect = fx_by_owner("portal_dissolve", 4);
    fx_set_param_v3(effect, 0x201, position.x, position.y, position.z);
    pfxhandle_spawn_at_bid_next(effect, hero, 0);
    snd_req(0x1594);
    _mkproc_sleep_ticks = 60.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("portal_dissolve_hero", 4);
    fx_set_param_v3(effect, 0x201, position.x, position.y, position.z);
    pfxhandle_spawn_at_bid_next(effect, hero, 0);
    snd_req(0x1591);
    transition_to_anim_script(
        konquest_pdata->hero_anim, (AniData*)konquest_animations[31], 3,
        0.1f);
    konquest_pdata->hero_anim->step = 0.5f;
    obj_set_all_sobjs_priority(hero, 0x13);

    while (konquest_pdata->hero_anim->frame <
           konquest_pdata->hero_anim->high_frame) {
        grounding->camera_target->alpha = (unsigned int)(
            255.0f * (1.0f - konquest_pdata->hero_anim->frame /
                                 konquest_pdata->hero_anim->high_frame));
        obj_for_all_atomics_set_material_alpha(
            hero, grounding->camera_target->alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    grounding->camera_target->alpha = 0;
    obj_for_all_atomics_set_material_alpha(
        hero, grounding->camera_target->alpha);
    snd_req(0x1595);
    _mkproc_sleep_ticks = 20.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("portal_flash", 4);
    direction.x = hero->pos.value.x - position.x;
    direction.z = hero->pos.value.z - position.z;
    inverse_length =
        konquest_inverse_length(direction.x * direction.x +
                                direction.z * direction.z);
    direction.x *= inverse_length;
    direction.z *= inverse_length;
    direction.x = 0.6f * direction.x;
    direction.y = pdata->target_offset.y;
    direction.z = 0.6f * direction.z;
    direction.x += record->field_14.x;
    direction.y += record->field_14.y;
    direction.z += record->field_14.z;
    fx_set_param_v3(
        effect, 0x202, direction.x, direction.y, direction.z);
    fx_reset_emit(effect);
    fx_resume_emit(effect);
    shake_camera(5, 0.025f);
    _mkproc_sleep_ticks = 5.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    fade_to_white(30, 0);
    _mkproc_sleep_ticks = 10.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("portal_flash2", 4);
    fx_set_param_v3(
        effect, 0x202, direction.x, direction.y, direction.z);
    fx_reset_emit(effect);
    fx_resume_emit(effect);
    fade_from_white(17, 0);
    _mkproc_sleep_ticks = 90.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    fade_to_black(15, 1);
    remove_camera_offsets();
    return -1.0f;
}

/*
 * Near match at retail size: position copies, attenuation, pan, sound id, and
 * call ABI are exact; only pooled local-constant relocation labels remain.
 */
void npc_play_teleported_sound(void) {
    Vec position = {0.0f, 0.0f, 0.0f};
    KonquestNpcData* spatial;
    float volume;

    volume = 0.0f;
    if (g_active_npc != 0) {
        spatial = g_active_npc->fields.data;
        position.x = spatial->position.x;
        position.y = spatial->position.y;
        position.z = spatial->position.z;
        volume = get_volume_from_distance(&position, 25.0f, 10.0f);
    }
    if (volume == 0.0f) {
        return;
    }
    pan_vol_snd_req(0x147E, get_pan_value(&position), volume);
}

/*
 * Near match: the mode lifecycle, HUD fade, hero/animation latches, typed
 * teleport payload, scriptable process wait, controller restoration, and mode
 * pop agree with retail. The remaining eight-byte delta is GPR allocation and
 * equivalent inline latch/mode-stack branch scheduling.
 */
void konquest_teleport_hero_to_location(const Vec* target) {
    MkObj* hero;
    AnimPdata* animation;
    MkProc* animation_proc;
    MkProc* proc;
    MkProc* teleport_proc;
    KonquestGrounding* grounding;
    KonquestTeleportPdata* teleport_pdata;
    unsigned int teleport_instance;
    int mode;

    if (konquest_pdata->region_index != 4 ||
        konquest_current_game_mode() != 0 ||
        konquest_game_mode_in_stack(8) != 0) {
        return;
    }

    mode = konquest_current_game_mode();
    if (mode != 8 && konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 8;
    }

    destroy_mkprocs_pid(0xA023);
    _create_mkproc_generic_tinystack(
        0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
    g_fade_hud_in = 0;
    turn_controllers_off();

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        hero->pos_vel.z = 0.0f;
        hero->pos_vel.y = 0.0f;
        hero->pos_vel.x = 0.0f;
        hero->flags_09_bits.bit6 = 1;
        konquest_pdata->hero_state = 0;
        hero->hide_flag_bits.still_move = 1;
        konquest_pdata->hero_anim->weight = 1.0f;
    }
    transition_to_anim_script(
        konquest_pdata->hero_anim,
        monk_state_data[konquest_pdata->hero_state].animation,
        monk_state_data[konquest_pdata->hero_state].transition, 0.1f);
    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        animation_proc = animation->proc;
        if (animation_proc != 0) {
            if (animation_proc->instance == animation->proc_instance) {
                /* Valid animation-process latch. */
            } else {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        xfer_proc(animation_proc, p_animate);
    }

    teleport_proc = 0;
    teleport_instance = 0;
    proc = (MkProc*)_create_mkproc_generic_tinystack(
        0xA028, 0x1F, p_hero_teleport, sizeof(*teleport_pdata),
        (void**)&teleport_pdata);
    if (proc != 0) {
        teleport_instance = proc->instance;
        teleport_proc = proc;
        set_process_as_scriptable(proc);
        zero_pdata_payload(sizeof(*teleport_pdata), teleport_pdata);
        teleport_pdata->target.x = target->x;
        teleport_pdata->target.y = target->y;
        teleport_pdata->target.z = target->z;
    }

    for (;;) {
        proc = teleport_proc;
        if (proc != 0) {
            if (proc->instance == teleport_instance) {
                /* Valid teleport-process latch. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        if (proc == 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        animation_proc = animation->proc;
        if (animation_proc != 0) {
            if (animation_proc->instance == animation->proc_instance) {
                /* Valid animation-process latch. */
            } else {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        xfer_proc(animation_proc, p_control_konquest_monk);
    }
    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
    }
    turn_controllers_on();
    destroy_mkprocs_pid(0xA023);
    _create_mkproc_generic_tinystack(
        0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
    g_fade_hud_in = 1;
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
}
static inline void run_konquest_teleport_script(const char* name) {
    cmdscript_setup_execution(
        konquest_pdata->script_owner,
        (unsigned int)get_script_function_by_name(
            konquest_pdata->script_owner, name));
    cmdscript_execute(konquest_pdata->script_owner);
}


/*
 * Near match: all three scripted effects, dissolve/reform alpha loops, camera
 * turn, cosine travel interpolation, grounding state, final animation reverse,
 * and return agree with retail. Residue is floating-register allocation and
 * save-frame size, equivalent latch/loop scheduling, and pooled relocations.
 */
static float p_hero_teleport(void) {
    KonquestTeleportPdata* pdata;
    KonquestGrounding* grounding;
    MkObj* hero;
    AnimScript* script;

    pdata = (KonquestTeleportPdata*)pdata_of_proc(aproc);
    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    run_konquest_teleport_script("hero_do_teleport_start_effect");
    script = (AnimScript*)konquest_animations[31];
    transition_to_anim_script(
        konquest_pdata->hero_anim, (AniData*)script, 3, 0.1f);
    konquest_pdata->hero_anim->step = 0.75f;
    obj_set_all_sobjs_priority(hero, 0x13);
    while (konquest_pdata->hero_anim->frame <
           konquest_pdata->hero_anim->high_frame) {
        grounding->camera_target->alpha = (unsigned int)(
            255.0f * (1.0f - konquest_pdata->hero_anim->frame /
                                 konquest_pdata->hero_anim->high_frame));
        hero->hide_flag_bits.pin_animation = 0;
        obj_for_all_atomics_set_material_alpha(
            hero, grounding->camera_target->alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    grounding->camera_target->alpha = 0;
    obj_for_all_atomics_set_material_alpha(
        hero, grounding->camera_target->alpha);
    grounding->flag_bits.suspended = 1;

    {
        Vec camera_angle;
        float start_angle;
        float angle_delta;
        float delta_x;
        float delta_z;
        float phase;
        float blend;
        int ticks;

        delta_x = pdata->target.x - hero->pos.value.x;
        delta_z = pdata->target.z - hero->pos.value.z;
        get_camera_angle(&camera_angle);
        start_angle = camera_angle.y;
        camera_angle.y = gxMathArcTanYX(delta_x, delta_z);
        angle_delta = camera_angle.y - start_angle;
        if (angle_delta > 3.1415927f) {
            angle_delta -= 6.2831855f;
        } else if (angle_delta < -3.1415927f) {
            angle_delta += 6.2831855f;
        }

        phase = 0.0f;
        ticks = 30;
        do {
            phase += 0.104719765f;
            blend = 0.5f * (1.0f - gxMathCos(phase));
            get_camera_angle(&camera_angle);
            camera_angle.y = angle_delta * blend + start_angle;
            set_camera_target_angle(&camera_angle);
            ticks--;
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        } while (ticks != 0);
        hero->ang.y = camera_angle.y;
    }

    run_konquest_teleport_script("hero_do_teleport_travel_effect");
    {
        Vec start_position;
        float delta_x;
        float delta_y;
        float delta_z;
        float old_ground_y;
        float ground_delta;
        float phase;
        float blend;
        int ticks;

        old_ground_y = hero->ground_colls_y;
        ground_delta = pdata->target.y - old_ground_y;
        pdata->target.y += hero->pos.value.y - old_ground_y;
        start_position.x = hero->pos.value.x;
        start_position.y = hero->pos.value.y;
        start_position.z = hero->pos.value.z;
        delta_x = pdata->target.x - start_position.x;
        delta_y = pdata->target.y - start_position.y;
        delta_z = pdata->target.z - start_position.z;

        phase = 0.0f;
        ticks = 180;
        do {
            phase += 0.017453294f;
            blend = 0.5f * (1.0f - gxMathCos(phase));
            hero->pos.value.x = delta_x * blend + start_position.x;
            hero->pos.value.y = delta_y * blend + start_position.y;
            hero->pos.value.z = delta_z * blend + start_position.z;
            hero->ground_colls_y = ground_delta * blend + old_ground_y;
            update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
            ticks--;
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        } while (ticks != 0);
    }

    hero->pos.value.x = pdata->target.x;
    hero->pos.value.y = pdata->target.y;
    hero->pos.value.z = pdata->target.z;
    run_konquest_teleport_script("hero_do_teleport_end_effect");
    transition_to_anim_script_frame(
        1.0f, (float)script->frame_count, konquest_pdata->hero_anim, script,
        3);
    konquest_pdata->hero_anim->step = -0.25f;
    while (konquest_pdata->hero_anim->frame > 0.0f) {
        grounding->camera_target->alpha = (unsigned int)(
            255.0f * (1.0f - konquest_pdata->hero_anim->frame /
                                 konquest_pdata->hero_anim->high_frame));
        obj_for_all_atomics_set_material_alpha(
            hero, grounding->camera_target->alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    grounding->camera_target->alpha = 0xFF;
    obj_set_all_sobjs_priority(hero, 0x10);
    obj_for_all_atomics_set_material_alpha(
        hero, grounding->camera_target->alpha);
    grounding->flag_bits.suspended = 0;
    return -1.0f;
}

void show_fight_message(int message) {
    KonquestFightMessagePdata* pdata;
    MkProc* proc;

    pdata = 0;
    proc = (MkProc*)_create_mkproc_generic_tinystack(
        0x9030, 0x1F, p_show_fight_message,
        sizeof(KonquestFightMessagePdata), (void**)&pdata);
    if (proc != 0 && pdata != 0) {
        pdata->message = message;
    }
}
static inline void position_fight_message(ScreenObj* object, int y_offset) {
    object->x = (int)((float)(screen_width / 2) -
                      (float)(object->pfx2d->tex_w / 2) * object->scale_x);
    object->y = (int)((float)(screen_height / 2) -
                      (float)(object->pfx2d->tex_h / 2) * object->scale_y) +
                y_offset;
}


/*
 * Near match: code size and all instructions agree with retail. Objdiff's
 * remaining differences are only pooled floating-constant relocation labels.
 * Retail's compact signed-halving and counted alpha loop require size mode.
 */
#pragma optimize_for_size on
static float p_show_fight_message(void) {
    KonquestFightMessagePdata* pdata;
    ScreenObj* message;
    ScreenObj* logo;
    int tick;
    int vertex;

    pdata = (KonquestFightMessagePdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }

    if (pdata->message == 0) {
        message = load_named_2d_pfxobj(
            0x60030, 0x8301, "TRAINING_MESSAGE", 0, 0x2C);
    } else {
        message = load_named_2d_pfxobj(
            0x60030, 0x8301, "FIGHT_MESSAGE", 0, 0x2C);
    }
    logo = load_named_2d_pfxobj(
        0x60030, 0x8301, "DRAGON_LOGO", 0, 0x2D);
    if (message == 0 || logo == 0) {
        return -1.0f;
    }

    logo->scale_x = 0.0f;
    logo->scale_y = 0.0f;
    position_fight_message(logo, 0);
    logo->flag_bits.scaled = 1;

    message->scale_x = 0.0f;
    message->scale_y = 0.0f;
    position_fight_message(message, -10);
    message->flag_bits.scaled = 1;

    for (tick = 0; tick < 30; tick++) {
        message->scale_x += 0.041666668f;
        message->scale_y += 0.041666668f;
        position_fight_message(message, -10);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    _mkproc_sleep_ticks = 60.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    for (tick = 0; tick < 50; tick++) {
        if (tick < 25) {
            for (vertex = 0; vertex < 4; vertex++) {
                message->pfx2d->verts[vertex].a -= 10;
            }
        }
        message->pfx2d->mirror = 1;
        message->scale_x += 0.03f;
        message->scale_y += 0.03f;
        position_fight_message(message, -10);
        logo->scale_x += 0.12f;
        logo->scale_y += 0.12f;
        position_fight_message(logo, 0);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    _mkproc_sleep_ticks = 30.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    if (message->instance != 0) {
        message->typed_vtbl->destroy(message);
    }
    if (logo->instance != 0) {
        logo->typed_vtbl->destroy(logo);
    }
    return -1.0f;
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: hero_start_fx_at_position ~83.8% -- typed owner/offset ABI,
 * hero latch, position calculation, and effect calls match retail. Residue is
 * individual r29-r31 saves/restores versus stmw/lmw and one folded latch join.
 */
void hero_start_fx_at_position(
    const char* effect_name, const Vec* offset) {
    MkObj* hero;
    unsigned int effect;
    Vec position = {0.0f, 0.0f, 0.0f};

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    effect = fx_by_owner(effect_name, 4);
    position.x = hero->pos.value.x + offset->x;
    position.y = hero->pos.value.y + offset->y;
    position.z = hero->pos.value.z + offset->z;
    fx_set_param_v3(
        effect, 0x202, position.x, position.y, position.z);
    fx_restart_emit(effect);
}

static void vdestroy_konquest_sobj_info(MkHdr* object) {
    object->instance = 0;
    mkhdr_memfree(object);
}

static void vdestroy_konquest_sobj(KonquestSobj* object) {
    KonquestDestroyable* owned_object;

    owned_object = object->owned_object;
    if (owned_object != 0) {
        if (owned_object->instance ==
            object->owned_object_instance) {
            /* Valid ownership latch. */
        } else {
            owned_object = 0;
        }
    } else {
        owned_object = 0;
    }
    if (owned_object != 0) {
        if (object->owned_object->instance != 0) {
            object->owned_object->vtbl->destroy(object->owned_object);
        }
        object->owned_object = 0;
        object->owned_object_instance = 0;
    }
    object->hdr.instance = 0;
    mkhdr_memfree(&object->hdr);
}

static void vdestroy_konquest_obj(KonquestObject* object) {
    KonquestDestroyable* owned_object;

    destroy_list(&object->list_18);
    destroy_list(&object->list_4C);

    owned_object = object->owned_object;
    if (owned_object != 0) {
        if (owned_object->instance != 0) {
            owned_object->vtbl->destroy(owned_object);
        }
        object->owned_object = 0;
    }

    object->hdr.instance = 0;
    mkhdr_memfree(&object->hdr);
}

void create_inventory_image_list(
    KonquestInventoryImageList* list, int count) {
    int item;
    int index;

    item = -1;
    for (index = 0; index < count; index++) {
        item = find_next_item_in_inventory(item);
        if (item != -1) {
            list->images[index] =
                get_konq_profile_value_item_tga(item);
            list->alpha_images[index] =
                get_konq_profile_value_item_tga_alpha(item);
        } else {
            list->images[index] = 0;
            list->alpha_images[index] = 0;
        }
    }
}

const char* locate_inventory_text(int mode) {
    KonquestInventoryScreenPdata* screen;
    const char* text;
    int selected;
    int count;
    int item;
    int index;

    text = inv_item_default_string;
    screen = (KonquestInventoryScreenPdata*)get_screen_pdata();
    selected = screen != 0 ? screen->selected_item : 0;
    count = get_number_items_in_inventory();

    if (count == 0) {
        return inv_item_empty_string;
    }
    if (count < selected) {
        return text;
    }

    item = -1;
    index = 0;
    while (index < count) {
        item = find_next_item_in_inventory(item);
        if (item != -1 && index == selected) {
            switch (mode) {
            case 0:
                text =
                    get_konq_profile_value_item_description(item);
                break;
            case 1:
                text = get_konq_profile_value_item_name(item);
                break;
            }
            break;
        }
        index++;
    }
    return text;
}

void konquest_set_current_inventory_item(int item) {
    char* pdata;

    pdata = get_screen_pdata();
    if (pdata != 0) {
        *(int*)(pdata + 0x2f8) = item;
    }
}

int get_hero_state(void) {
    return konquest_pdata->hero_state;
}

void resume_weather_effects(void) {
    unsigned int effect;

    if (konquest_pdata->weather_type == 1) {
        effect = fx("rain");
        if (effect != 0) {
            fx_reset(effect);
            fx_restart_emit(effect);
        }
        effect = fx("rain_splash");
        if (effect != 0) {
            fx_reset(effect);
            fx_restart_emit(effect);
        }
    } else if (konquest_pdata->weather_type == 2) {
        effect = fx("snow");
        if (effect != 0) {
            fx_reset(effect);
            fx_restart_emit(effect);
        }
    }
}

void pause_weather_effects(void) {
    unsigned int effect;

    if (konquest_pdata->weather_type == 1) {
        effect = fx("rain");
        if (effect != 0) {
            fx_reset(effect);
            fx_reset_emit(effect);
        }
        effect = fx("rain_splash");
        if (effect != 0) {
            fx_reset(effect);
            fx_reset_emit(effect);
        }
    } else if (konquest_pdata->weather_type == 2) {
        effect = fx("snow");
        if (effect != 0) {
            fx_reset(effect);
            fx_reset_emit(effect);
        }
    }
}

void set_konquest_weather(int type, int param_a, int param_b) {
    konquest_pdata->weather_type = type;
    konquest_pdata->weather_param_a = param_a;
    konquest_pdata->weather_param_b = param_b;
}

/*
 * Near match: p_weather 94.46% (1556 versus 1536 bytes). All three weather
 * states, timed expiry, duplicated overlay fades, sound/script lifecycle, and
 * all 44 direct calls agree. Residue is nonvolatile save/restore choice,
 * register allocation, and the compiler's fixed-size time-copy lowering.
 */
static float p_weather(void) {
    KonquestWeatherPdata* pdata;
    MkSobj* weather_object;
    MkSobj* alpha_object;
    RpMaterial* material;
    float ticks;
    float progress;
    float alpha;

    pdata = (KonquestWeatherPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    if (konquest_pdata->weather_type == 0) {
        return 1.0f;
    }

    if (konquest_pdata->weather_type > 0) {
        if (pdata->initialized != 0) {
            if (is_time_a_greater_than_time_b(
                    &konquest_pdata->current_time, &pdata->end_time) != 0) {
                konquest_pdata->weather_type = -1;
                return 1.0f;
            }
            return 1.0f;
        }

        {
            int duration_hours;
            int pair;
            KonquestTimePair* source;
            KonquestTimePair* destination;

            duration_hours = (unsigned short)randu0(6) + 3;
            source = konquest_pdata->current_time.pairs;
            destination = pdata->end_time.pairs;
            pair = 3;
            do {
                *destination++ = *source++;
            } while (--pair != 0);
            add_hours_to_time(&pdata->end_time, duration_hours);
        }
        weather_object = obj_find_sobj_by_id(g_game_info.sky, 0x3C);
        if (weather_object != 0) {
            ticks = 0.0f;
            while (ticks < 300.0f) {
                unhide_sobj(weather_object);
                progress = ticks / 300.0f;
                alpha = progress <= 1.0f ? progress : 1.0f;
                alpha_object = obj_find_sobj_by_id(g_game_info.sky, 0x3C);
                if (alpha_object != 0) {
                    if (alpha == 0.0f) {
                        hide_sobj(alpha_object);
                    } else {
                        unhide_sobj(alpha_object);
                    }
                    material = sobj_find_material_by_id(alpha_object, 4);
                    if (material != 0) {
                        material->color.alpha =
                            (unsigned char)(255.0f * alpha);
                    }
                }
                update_mksobj(weather_object);
                alpha = 1.0f - progress;
                alpha = alpha >= 0.75f ? alpha : 0.75f;
                konquest_pdata->sky_color_multiplier = alpha;
                alpha = konquest_pdata->sky_color_multiplier;
                alpha = alpha <= 1.0f ? alpha : 1.0f;
                konquest_pdata->sky_color_multiplier = alpha;
                _mkproc_sleep_ticks = 1.0f;
                ticks += game_speed;
                ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
            }
            alpha_object = obj_find_sobj_by_id(g_game_info.sky, 0x3C);
            if (alpha_object != 0) {
                unhide_sobj(alpha_object);
                material = sobj_find_material_by_id(alpha_object, 4);
                if (material != 0) {
                    material->color.alpha = 0xFF;
                }
            }
            update_mksobj(weather_object);
            konquest_pdata->sky_color_multiplier = 0.75f;
        }

        if (konquest_pdata->weather_type < 1) {
            if (weather_object != 0) {
                ticks = 0.0f;
                while (ticks < 300.0f) {
                    unhide_sobj(weather_object);
                    progress = ticks / 300.0f;
                    alpha = 1.0f - progress;
                    alpha = alpha <= 1.0f ? alpha : 1.0f;
                    alpha_object =
                        obj_find_sobj_by_id(g_game_info.sky, 0x3C);
                    if (alpha_object != 0) {
                        if (alpha == 0.0f) {
                            hide_sobj(alpha_object);
                        } else {
                            unhide_sobj(alpha_object);
                        }
                        material = sobj_find_material_by_id(alpha_object, 4);
                        if (material != 0) {
                            material->color.alpha =
                                (unsigned char)(255.0f * alpha);
                        }
                    }
                    update_mksobj(weather_object);
                    alpha = progress;
                    alpha = alpha >= 0.75f ? alpha : 0.75f;
                    konquest_pdata->sky_color_multiplier = alpha;
                    alpha = konquest_pdata->sky_color_multiplier;
                    alpha = alpha <= 1.0f ? alpha : 1.0f;
                    konquest_pdata->sky_color_multiplier = alpha;
                    _mkproc_sleep_ticks = 1.0f;
                    ticks += game_speed;
                    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
                }
                alpha_object = obj_find_sobj_by_id(g_game_info.sky, 0x3C);
                if (alpha_object != 0) {
                    hide_sobj(alpha_object);
                    material = sobj_find_material_by_id(alpha_object, 4);
                    if (material != 0) {
                        material->color.alpha = 0;
                    }
                }
                update_mksobj(weather_object);
                konquest_pdata->sky_color_multiplier = 1.0f;
            }
            return 1.0f;
        }

        pdata->initialized = 1;
        if (konquest_pdata->weather_type == 1) {
            pdata->sound = snd_req(0x158C);
        }
        if (get_game_state() != 0x14) {
            cmdscript_setup_execution(
                konquest_pdata->script_owner,
                konquest_pdata->weather_param_a);
            cmdscript_execute(konquest_pdata->script_owner);
        }
        return 1.0f;
    }

    if (pdata->initialized != 0) {
        pdata->initialized = 0;
        konquest_pdata->weather_type = 0;
        cmdscript_setup_execution(
            konquest_pdata->script_owner, konquest_pdata->weather_param_b);
        cmdscript_execute(konquest_pdata->script_owner);
        if (mslSoundIsValid(pdata->sound) != 0) {
            snd_stop(pdata->sound);
            pdata->sound = 0;
        }

        weather_object = obj_find_sobj_by_id(g_game_info.sky, 0x3C);
        if (weather_object != 0) {
            ticks = 0.0f;
            while (ticks < 300.0f) {
                unhide_sobj(weather_object);
                progress = ticks / 300.0f;
                alpha = 1.0f - progress;
                alpha = alpha <= 1.0f ? alpha : 1.0f;
                alpha_object = obj_find_sobj_by_id(g_game_info.sky, 0x3C);
                if (alpha_object != 0) {
                    if (alpha == 0.0f) {
                        hide_sobj(alpha_object);
                    } else {
                        unhide_sobj(alpha_object);
                    }
                    material = sobj_find_material_by_id(alpha_object, 4);
                    if (material != 0) {
                        material->color.alpha =
                            (unsigned char)(255.0f * alpha);
                    }
                }
                update_mksobj(weather_object);
                alpha = progress;
                alpha = alpha >= 0.75f ? alpha : 0.75f;
                konquest_pdata->sky_color_multiplier = alpha;
                alpha = konquest_pdata->sky_color_multiplier;
                alpha = alpha <= 1.0f ? alpha : 1.0f;
                konquest_pdata->sky_color_multiplier = alpha;
                _mkproc_sleep_ticks = 1.0f;
                ticks += game_speed;
                ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
            }
            alpha_object = obj_find_sobj_by_id(g_game_info.sky, 0x3C);
            if (alpha_object != 0) {
                hide_sobj(alpha_object);
                material = sobj_find_material_by_id(alpha_object, 4);
                if (material != 0) {
                    material->color.alpha = 0;
                }
            }
            update_mksobj(weather_object);
            konquest_pdata->sky_color_multiplier = 1.0f;
        }
    }
    return 1.0f;
}

/*
 * Near match: the validated hero latch and typed position/angle snapshot agree
 * with retail. The sole four-byte delta is an equivalent folded latch branch.
 */
void save_hero_position_and_angle_prior_to_fight(float angle_offset) {
    KonquestSavedState* save;
    MkObj* hero;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        save = &konquest_save_data;
        save->hero_position.x = hero->pos.value.x;
        save->hero_position.y = hero->pos.value.y;
        save->hero_position.z = hero->pos.value.z;
        save->hero_angle = hero->ang.y + angle_offset;
    }
}

void konquest_run_camera_script(void* script, int flags) {
    if (konquest_pdata->script_owner != 0) {
        run_camera_script(konquest_pdata->script_owner, script, flags);
    }
}

/*
 * Near match: the grounding latch and typed suspension-bit update agree with
 * retail. The sole four-byte delta is an equivalent folded latch branch.
 */
void restore_hero_grounding(void) {
    KonquestGrounding* grounding;

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        grounding->flag_bits.suspended = 0;
    }
}

/*
 * Near match: the grounding latch and typed suspension-bit update agree with
 * retail. The sole four-byte delta is an equivalent folded latch branch.
 */
void suspend_hero_grounding(void) {
    KonquestGrounding* grounding;

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        grounding->flag_bits.suspended = 1;
    }
}

void transition_to_region(int region) {
    unsigned char current_region;

    delete_screen_obj_oid(0x2098);
    current_region = p1_profile_konquest->raw[0x5D];
    if (current_region != region) {
        p1_profile_konquest->raw[0x5E] = current_region;
        p1_profile_konquest->raw[0x5D] = (unsigned char)region;
        p1_profile_konquest->fields.objective_index = 0;
        game_settings.konquest_latch = 0;
    }
    profile_region_change();
    gamelogic_jump(4, p_konquest_mode);
}

int get_previous_konquest_region_number(void) {
    return p1_profile_konquest->raw[0x5e];
}

void set_konquest_region_number(int region) {
    if (p1_profile_konquest->raw[0x5d] == region) {
        return;
    }
    p1_profile_konquest->raw[0x5e] = p1_profile_konquest->raw[0x5d];
    p1_profile_konquest->raw[0x5d] = (unsigned char)region;
    *(int*)&p1_profile_konquest->raw[0x64] = 0;
    game_settings.konquest_latch = 0;
}

/*
 * Soft ceiling: konquest_hide_damashi ~91.2%. All effect, sound, sleep,
 * instance-latch, destruction, and global-clear operations agree with retail.
 * Residue is r29/r30 coloring, folded latch joins, expanded saves/restores,
 * one handle compare form, and local string/float relocation labels; source
 * is 660 bytes versus retail's 656.
 */
void konquest_hide_damashi(void) {
    KonquestNpc* npc;
    MkObj* object;
    unsigned int effect;

    npc = konquest_pdata->hero_npc;
    if (npc != 0) {
        if (npc->fields.hdr.instance != konquest_pdata->hero_npc_instance) {
            npc = 0;
        }
    } else {
        npc = 0;
    }

    effect = fx_by_owner("damashi_spawn", 4);
    if (effect != 0) {
        fx_reset(effect);
        fx_pause_emit(effect);
    }
    effect = fx_by_owner("damashi_sphere", 4);
    if (effect != 0) {
        fx_reset(effect);
        fx_pause_emit(effect);
    }
    effect = fx_by_owner("damashi_sphere_shimmer", 4);
    if (effect != 0) {
        fx_reset(effect);
        fx_pause_emit(effect);
    }
    effect = fx_by_owner("damashi_sphere_particles", 4);
    if (effect != 0) {
        fx_reset(effect);
        fx_pause_emit(effect);
    }

    if (npc == 0) {
        return;
    }
    object = konquest_pdata->damashi_object;
    if (object != 0) {
        if (object->hdr.instance != konquest_pdata->damashi_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object == 0) {
        return;
    }

    effect = fx_by_owner("damashi_die", 4);
    pfx_bind_emitter_to_obj(find_pfx_by_handle(effect), object, 1);
    fx_reset(effect);
    fx_resume_emit(effect);

    _mkproc_sleep_ticks = 16.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("damashi_die", 4);
    if (effect != 0) {
        fx_pause_emit(effect);
    }
    if (mslSoundIsValid(konquest_pdata->damashi_sound)) {
        snd_stop(konquest_pdata->damashi_sound);
    }

    if (npc != 0) {
        if (npc->fields.animation->instance != 0) {
            npc->fields.animation->typed_vtbl->destroy(
                npc->fields.animation);
        }
        if (npc->fields.hdr.instance != 0) {
            npc->fields.hdr.typed_vtbl->destroy(&npc->fields.hdr);
        }
        konquest_pdata->hero_npc = 0;
        konquest_pdata->hero_npc_instance = 0;
    }

    object = konquest_pdata->damashi_object;
    if (object != 0) {
        if (object->hdr.instance != konquest_pdata->damashi_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        if (konquest_pdata->damashi_object->hdr.instance != 0) {
            konquest_pdata->damashi_object->hdr.typed_vtbl->destroy(
                &konquest_pdata->damashi_object->hdr);
        }
        konquest_pdata->damashi_object = 0;
        konquest_pdata->damashi_instance = 0;
    }
}

/*
 * Soft ceiling: konquest_start_damashi ~95.5%. Object setup, position and
 * ground stores, global latch, sounds, all four particle bindings, sleep, NPC
 * creation, and return agree. Residue is individual r30/r31 saves/restores
 * plus local string/float relocation labels; source is 456 versus 448 bytes.
 */
MkObj* konquest_start_damashi(
    void* unused, float x, float y, float z) {
    MkObj* object;
    unsigned int effect;
    MkPfx* particle;

    object = get_mkobj_frame(0x9010, 0);
    if (object == 0) {
        return 0;
    }
    insert_fgnd_mkobj(object);
    object->pos.value.x = x;
    object->pos.value.y = y;
    object->pos.value.z = z;
    update_mkobj(object);
    object->ground_colls_y = y - 1.5f;
    konquest_pdata->damashi_object = object;
    konquest_pdata->damashi_instance = object->hdr.instance;

    snd_req(0x1589);
    konquest_pdata->damashi_sound = snd_req(0x158A);

    effect = fx_by_owner("damashi_spawn", 4);
    particle = find_pfx_by_handle(effect);
    pfx_bind_render_to_obj(particle, object, 1);
    fx_reset(effect);
    fx_restart_emit(effect);

    _mkproc_sleep_ticks = 11.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    effect = fx_by_owner("damashi_sphere", 4);
    particle = find_pfx_by_handle(effect);
    pfx_bind_render_to_obj(particle, object, 1);
    fx_resume_emit(effect);

    effect = fx_by_owner("damashi_sphere_shimmer", 4);
    particle = find_pfx_by_handle(effect);
    pfx_bind_render_to_obj(particle, object, 1);
    fx_resume_emit(effect);

    effect = fx_by_owner("damashi_sphere_particles", 4);
    particle = find_pfx_by_handle(effect);
    pfx_bind_emitter_to_obj(particle, object, 1);
    fx_resume_emit(effect);

    make_damashi_npc(object);
    return object;
}

/*
 * Soft ceiling: display_konquest_text ~94.57%. Mode gating, hero process
 * suspension/restoration, exact text-window initialization, input wait,
 * grounding restore, and return match retail. The 12-byte size residue is
 * individual r28-r31 saves/restores offset by three folded latch joins.
 */
int display_konquest_text(
    unsigned int string_id, unsigned int prompt_flags,
    float left_fraction, float bottom_fraction, float width_fraction) {
    KonquestTextWindowPdata* window;
    AnimPdata* animation;
    MkProc* animation_proc;
    KonquestGrounding* grounding;
    int wait_for_input;
    int timeout;
    int mode_index;
    int game_mode;

    wait_for_input = prompt_flags & 1;
    timeout = -1;
    if (wait_for_input != 0 || (prompt_flags & 2) != 0) {
        mode_index = konquest_pdata->game_mode_index;
        game_mode = mode_index < 0
            ? 0 : konquest_pdata->game_modes[mode_index];
        if (game_mode != 1) {
            hero_stop_moving();
            animation = konquest_pdata->hero_anim;
            if (animation != 0) {
                animation_proc = animation->proc;
                if (animation_proc != 0) {
                    if (animation_proc->instance ==
                        animation->proc_instance) {
                        /* Valid animation-process latch. */
                    } else {
                        animation_proc = 0;
                    }
                } else {
                    animation_proc = 0;
                }
                xfer_proc(animation_proc, p_animate);
            }
        }
    } else {
        timeout = 100;
    }

    if (_create_mkproc_generic_bigstack(
            0x9002, aproc->priority + 1, p_show_text_window,
            sizeof(*window), (void**)&window) == 0) {
        return 0;
    }
    zero_pdata_payload(sizeof(*window), window);
    window->left = (int)((float)screen_width * left_fraction);
    window->bottom =
        (int)((float)screen_height -
              (float)screen_height * bottom_fraction);
    window->priority = 0x24;
    window->width = (int)((float)screen_width * width_fraction);
    window->prompt_flags = prompt_flags;
    window->font = 6;
    window->timeout = timeout;
    window->color = 0xC8C8C8FF;
    window->button_charmap = danton20_charmap;
    window->swap_buttons = 0;
    window->art_slot = 0x60030;
    window->controller_port = konquest_pdata->input_port;
    strcpy(window->text, get_string_by_id(string_id));

    text_window_state = 0;
    while (text_window_state == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (wait_for_input != 0 || (prompt_flags & 2) != 0) {
        mode_index = konquest_pdata->game_mode_index;
        game_mode = mode_index < 0
            ? 0 : konquest_pdata->game_modes[mode_index];
        if (game_mode != 1) {
            animation = konquest_pdata->hero_anim;
            if (animation != 0) {
                animation_proc = animation->proc;
                if (animation_proc != 0) {
                    if (animation_proc->instance ==
                        animation->proc_instance) {
                        /* Valid animation-process latch. */
                    } else {
                        animation_proc = 0;
                    }
                } else {
                    animation_proc = 0;
                }
                xfer_proc(animation_proc, p_control_konquest_monk);
            }

            grounding = konquest_pdata->hero_grounding;
            if (grounding != 0) {
                if (grounding->hdr.instance ==
                    konquest_pdata->grounding_instance) {
                    /* Valid grounding latch. */
                } else {
                    grounding = 0;
                }
            } else {
                grounding = 0;
            }
            if (grounding != 0) {
                npc_xfer(grounding, p_npc_idle, 0);
            }
        }
    }
    return text_window_state;
}

void konquest_camera_return_to_normal(void) {
    destroy_mkprocs_pid(0x9006);
    xfer_camera(p_konquest_camera_proc, 1);
}

void konquest_load_interior_art(void) {
    if (konquest_pdata->region_table->interior_art_name != 0) {
        unload_section_slot(0xA002F);
        load_ssf(konquest_region_data[konquest_pdata->region_index].fight_files);
        add_art_section_by_name_async(
            0xA002F, konquest_pdata->region_table->interior_art_name);
    }
}

int konquest_nis_anims_loaded(void) {
    return find_mkproc_pid(0x901b) == 0;
}

/*
 * Near match: the process creation and complete typed payload agree with
 * retail. Residue is only individual r30/r31 saves versus stmw/lmw.
 */
void konquest_start_nis_anims_load(
    char* animation_name, char* art_name) {
    KonquestNisLoadPdata* pdata;

    if (_create_mkproc_generic_bigstack(
            0x901B, 0x1F, p_konquest_load_nis_anims, sizeof(*pdata),
            (void**)&pdata) != 0) {
        pdata->slot = 0xA002F;
        pdata->animation_name = animation_name;
        pdata->art_name = art_name;
    }
}

static float p_konquest_load_nis_anims(void) {
    KonquestNisLoadPdata* pdata;

    pdata = (KonquestNisLoadPdata*)apdata;
    load_ssf(konquest_region_data[konquest_pdata->region_index].fight_files);
    unload_section_slot(pdata->slot);
    if (pdata->animation_name != 0) {
        add_anim_section_by_name_async_pal(
            pdata->slot, pdata->animation_name, konq_nis_anims, 0, 1);
    }
    if (pdata->art_name != 0) {
        add_art_section_by_name_async(pdata->slot, pdata->art_name);
    }
    wait_for_slot_load(pdata->slot);
    return -1.0f;
}

void* get_konquest_region_table(void) {
    return konquest_pdata->region_table;
}
static inline MkProc* konquest_resolve_proc(
    MkProc* proc, unsigned int instance) {
    if (proc != 0) {
        if (proc->instance != instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    return proc;
}


/*
 * Near match: hero suspension/restoration, award-process lifetime, and the
 * grounding latch agree with retail. The eight-byte residue is equivalent
 * latch-join layout, nonvolatile-register coloring, and save/restore choice.
 */
void give_krypt_key_to_player(KonquestPuiDefinition* award, int arg) {
    MkProc* proc;
    MkProc* notice;
    unsigned int notice_instance;
    KonquestGrounding* grounding;

    if (konquest_pdata->hero_anim != 0) {
        proc = konquest_pdata->hero_anim->proc;
        if (proc != 0) {
            if (proc->instance !=
                konquest_pdata->hero_anim->proc_instance) {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_animate);
    }

    hero_stop_moving();
    _mkproc_sleep_ticks = 10.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    notice = konquest_display_award_tga(award, arg, 0);
    notice_instance = notice->instance;
    while (konquest_resolve_proc(notice, notice_instance) != 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_pdata->hero_anim != 0) {
        proc = konquest_pdata->hero_anim->proc;
        if (proc != 0) {
            if (proc->instance !=
                konquest_pdata->hero_anim->proc_instance) {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_control_konquest_monk);
    }

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance !=
            konquest_pdata->grounding_instance) {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
    }
}

void give_reward_to_player(KonquestPuiDefinition* award) {
    int pid;

    pid = aproc->pid;
    if (pid == 0xA014 || pid == 0xA002) {
        konquest_display_award_tga(award, 0, 2);
    } else {
        konquest_display_award_tga(award, 0, 1);
    }
}

/*
 * Soft ceiling: set_hero_position_relative_to_chest ~90.4% -- both object
 * latches, rotation, position construction, and the final call agree. The
 * residue is paired-save emission, one folded latch join, and local-constant
 * relocation labels.
 */
void set_hero_position_relative_to_chest(void) {
    Vec offset = {0.0f, 0.0f, 1.0f};
    Vec position = {0.0f, 0.0f, 0.0f};
    MkObj* hero;
    MkObj* chest;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    chest = get_pickup_object();
    if (chest != 0 && hero != 0) {
        float hero_height;

        hero_height = hero->pos.value.y;
        rotate_xz(&offset, &offset, chest->ang.y);
        position.x = chest->pos.value.x + offset.x;
        position.z = chest->pos.value.z + offset.z;
        position.y = hero_height;
        set_monk_position(
            position.x, position.y, position.z,
            3.1415927f + chest->ang.y);
    }
}
static inline KonquestPuiDelayView* find_pui_runtime_by_id(
    KonquestPuiDefinition* item) {
    KonquestPuiListNode* node;

    if (konquest_pdata->pui_list != 0) {
        node = (KonquestPuiListNode*)konquest_pdata->pui_list;
        while (node != 0) {
            KonquestPuiDelayView* pui;

            pui = node->object;
            if (node->object_instance != pui->hdr.instance) {
                KonquestPuiListNode* next;

                next = node->next;
                node->object = 0;
                destroy_mkptr((MkPtr*)node);
                node = next;
                continue;
            }
            if (pui != 0 && pui->item == item) {
                return pui;
            }
            node = node->next;
        }
    }
    return 0;
}


/*
 * Near match: the shared typed PUI lookup and the final render-object
 * generation latch reproduce retail behavior, including clearing the result
 * when list traversal exhausts. The residue is stmw/lmw versus individual
 * saves, one equivalent latch branch, and register allocation.
 */
MkObj* get_pickup_object(void) {
    KonquestPuiDelayView* pui;
    MkObj* object;

    pui = find_pui_runtime_by_id(konquest_pdata->reference_pui);
    if (pui == 0) {
        return 0;
    }

    object = pui->render_object;
    if (object != 0) {
        if (object->hdr.instance != pui->render_object_instance) {
            object = 0;
        }
        return object;
    }
    return 0;
}

void set_reference_pui(KonquestPuiDefinition* reference) {
    konquest_pdata->reference_pui = reference;
}

/*
 * Near match at retail size: mode/HUD transitions, hero suspension, chest and
 * camera wait, inventory/profile update, award wait, and restoration all
 * agree. Residue is equivalent latch/loop block placement and GPR coloring.
 */
void open_chest_and_unlock_kontent(
    KonquestPuiDefinition* item, int award_arg) {
    MkProc* chest_proc;
    unsigned int chest_proc_instance;
    MkProc* notice;
    unsigned int notice_instance;
    KonquestAwardArtPdata* notice_pdata;
    AnimPdata* animation;
    MkProc* proc;
    KonquestGrounding* grounding;
    unsigned int table_index;
    unsigned int first;
    int inventory_index;
    int current_value;
    int interior;
    int mode_index;
    int mode;

    interior = get_game_state() == 0x14;
    mode_index = konquest_pdata->game_mode_index;
    if (mode_index < 0) {
        mode = 0;
    } else {
        mode = konquest_pdata->game_modes[mode_index];
    }
    if (mode != 11 && mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 11;
    }

    if (interior == 0) {
        konquest_pdata->time_passing = 0;
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 0;
    }

    hero_stop_moving();
    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        proc = animation->proc;
        if (proc != 0) {
            if (proc->instance != animation->proc_instance) {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_animate);
    }

    chest_proc = pui_set_chest_state(item, 0);
    chest_proc_instance = chest_proc->instance;
    while (konquest_resolve_proc(chest_proc, chest_proc_instance) != 0 ||
           (find_mkproc_pid(0x9006) != 0 &&
            camera_is_pos_move_done() == 0)) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index >= 0) {
        set_u8_bit(
            &p1_profile_konquest->raw[0x291],
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index, 1);
        if (item != 0) {
            switch (item->type) {
            case 3:
                current_value = get_konq_profile_value(7, 0);
                set_konq_profile_value(7, 0, item->value + current_value);
                break;
            case 4:
                current_value = get_konq_profile_value(8, 0);
                set_konq_profile_value(8, 0, item->value + current_value);
                break;
            case 5:
                current_value = get_konq_profile_value(9, 0);
                set_konq_profile_value(9, 0, item->value + current_value);
                break;
            case 6:
                current_value = get_konq_profile_value(10, 0);
                set_konq_profile_value(10, 0, item->value + current_value);
                break;
            case 7:
                current_value = get_konq_profile_value(11, 0);
                set_konq_profile_value(11, 0, item->value + current_value);
                break;
            case 8:
                current_value = get_konq_profile_value(12, 0);
                set_konq_profile_value(12, 0, item->value + current_value);
                break;
            }
        }
    }

    notice = konquest_display_award_tga(item, award_arg, 0);
    notice_instance = notice->instance;
    notice_pdata = (KonquestAwardArtPdata*)pdata_of_proc(notice);
    while (konquest_resolve_proc(notice, notice_instance) != 0 &&
           notice_pdata->complete == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (interior == 0) {
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 1;
        konquest_pdata->time_passing = 1;
    }

    for (;;) {
        mode_index = konquest_pdata->game_mode_index;
        if (mode_index < 0) {
            mode = 0;
        } else {
            mode = konquest_pdata->game_modes[mode_index];
        }
        if (mode == 11) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    if (mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }

    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        proc = animation->proc;
        if (proc != 0) {
            if (proc->instance != animation->proc_instance) {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_control_konquest_monk);
    }
    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance != konquest_pdata->grounding_instance) {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
    }
}

/*
 * Near match at retail size: this retail twin of the unlock path has matching
 * mode, process/camera waits, typed inventory award, and restoration. Residue
 * is the same equivalent latch/loop block placement and GPR coloring.
 */
void open_chest_and_give_item_to_player(
    KonquestPuiDefinition* chest_item,
    KonquestPuiDefinition* reward_item) {
    MkProc* chest_proc;
    unsigned int chest_proc_instance;
    MkProc* notice;
    unsigned int notice_instance;
    KonquestAwardArtPdata* notice_pdata;
    AnimPdata* animation;
    MkProc* proc;
    KonquestGrounding* grounding;
    unsigned int table_index;
    unsigned int first;
    int inventory_index;
    int current_value;
    int interior;
    int mode_index;
    int mode;

    interior = get_game_state() == 0x14;
    mode_index = konquest_pdata->game_mode_index;
    if (mode_index < 0) {
        mode = 0;
    } else {
        mode = konquest_pdata->game_modes[mode_index];
    }
    if (mode != 11 && mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 11;
    }

    if (interior == 0) {
        konquest_pdata->time_passing = 0;
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 0;
    }

    hero_stop_moving();
    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        proc = animation->proc;
        if (proc != 0) {
            if (proc->instance != animation->proc_instance) {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_animate);
    }

    chest_proc = pui_set_chest_state(chest_item, 0);
    chest_proc_instance = chest_proc->instance;
    while (konquest_resolve_proc(chest_proc, chest_proc_instance) != 0 ||
           (find_mkproc_pid(0x9006) != 0 &&
            camera_is_pos_move_done() == 0)) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (chest_item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, chest_item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index >= 0) {
        set_u8_bit(
            &p1_profile_konquest->raw[0x291],
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index, 1);
        if (chest_item != 0) {
            switch (chest_item->type) {
            case 3:
                current_value = get_konq_profile_value(7, 0);
                set_konq_profile_value(
                    7, 0, chest_item->value + current_value);
                break;
            case 4:
                current_value = get_konq_profile_value(8, 0);
                set_konq_profile_value(
                    8, 0, chest_item->value + current_value);
                break;
            case 5:
                current_value = get_konq_profile_value(9, 0);
                set_konq_profile_value(
                    9, 0, chest_item->value + current_value);
                break;
            case 6:
                current_value = get_konq_profile_value(10, 0);
                set_konq_profile_value(
                    10, 0, chest_item->value + current_value);
                break;
            case 7:
                current_value = get_konq_profile_value(11, 0);
                set_konq_profile_value(
                    11, 0, chest_item->value + current_value);
                break;
            case 8:
                current_value = get_konq_profile_value(12, 0);
                set_konq_profile_value(
                    12, 0, chest_item->value + current_value);
                break;
            }
        }
    }

    notice = konquest_display_award_tga(reward_item, 0, 1);
    notice_instance = notice->instance;
    notice_pdata = (KonquestAwardArtPdata*)pdata_of_proc(notice);
    while (konquest_resolve_proc(notice, notice_instance) != 0 &&
           notice_pdata->complete == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (interior == 0) {
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 1;
        konquest_pdata->time_passing = 1;
    }

    for (;;) {
        mode_index = konquest_pdata->game_mode_index;
        if (mode_index < 0) {
            mode = 0;
        } else {
            mode = konquest_pdata->game_modes[mode_index];
        }
        if (mode == 11) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    if (mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }

    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        proc = animation->proc;
        if (proc != 0) {
            if (proc->instance != animation->proc_instance) {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_control_konquest_monk);
    }
    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance != konquest_pdata->grounding_instance) {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
    }
}

/*
 * Near match: inventory/profile updates, mode push, process allocation, typed
 * payload selection, and all seven latch clears agree with retail. The
 * remaining 36 bytes are save/restore selection and equivalent mode/index
 * branch layout with different GPR coloring.
 */
static MkProc* konquest_display_award_tga(
    KonquestPuiDefinition* item, int value, int display_mode) {
    KonquestAwardArtPdata* pdata;
    MkProc* proc;
    unsigned int table_index;
    unsigned int first;
    int inventory_index;
    int current_value;
    int mode_index;
    int mode;

    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index >= 0) {
        set_u8_bit(
            &p1_profile_konquest->raw[0x291],
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index, 1);
        if (item != 0) {
            switch (item->type) {
            case 3:
                current_value = get_konq_profile_value(7, 0);
                set_konq_profile_value(7, 0, item->value + current_value);
                break;
            case 4:
                current_value = get_konq_profile_value(8, 0);
                set_konq_profile_value(8, 0, item->value + current_value);
                break;
            case 5:
                current_value = get_konq_profile_value(9, 0);
                set_konq_profile_value(9, 0, item->value + current_value);
                break;
            case 6:
                current_value = get_konq_profile_value(10, 0);
                set_konq_profile_value(10, 0, item->value + current_value);
                break;
            case 7:
                current_value = get_konq_profile_value(11, 0);
                set_konq_profile_value(11, 0, item->value + current_value);
                break;
            case 8:
                current_value = get_konq_profile_value(12, 0);
                set_konq_profile_value(12, 0, item->value + current_value);
                break;
            }
        }
    }

    mode_index = konquest_pdata->game_mode_index;
    if (mode_index < 0) {
        mode = 0;
    } else {
        mode = konquest_pdata->game_modes[mode_index];
    }
    if (mode != 6 && mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 6;
    }

    if (display_mode >= 3 || display_mode < 0) {
        return 0;
    }
    proc = (MkProc*)_create_mkproc_generic_bigstack(
        0x8239, 0x1F, p_display_award_image, sizeof(*pdata),
        (void**)&pdata);
    if (proc != 0 && pdata != 0) {
        pdata->mode = display_mode;
        if (display_mode == 0) {
            pdata->value = value;
        } else {
            pdata->item = item;
        }
        pdata->complete = 0;
        {
            KonquestAwardArtPdata* clear_pdata;

            clear_pdata = pdata;
            clear_pdata->art[0].obj = 0;
            clear_pdata->art[0].obj_instance = 0;
            clear_pdata->art[1].obj = 0;
            clear_pdata->art[1].obj_instance = 0;
            clear_pdata->art[3].obj = 0;
            clear_pdata->art[3].obj_instance = 0;
            clear_pdata->art[2].obj = 0;
            clear_pdata->art[2].obj_instance = 0;
            clear_pdata->art[4].obj = 0;
            clear_pdata->art[4].obj_instance = 0;
            clear_pdata->art[5].obj = 0;
            clear_pdata->art[5].obj_instance = 0;
            clear_pdata->description.object = 0;
            clear_pdata->description.instance = 0;
        }
    }
    return proc;
}
static inline void set_award_display_alpha(
    KonquestAwardArtPdata* pdata, float alpha) {
    ScreenObj* screen;
    StringObj* description;

    screen = pdata->art[0].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[0].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        set_screen_obj_alpha(screen, alpha);
    }
    screen = pdata->art[1].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[1].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        set_screen_obj_alpha(screen, alpha);
    }
    screen = pdata->art[2].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[2].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        set_screen_obj_alpha(screen, alpha);
    }
    screen = pdata->art[3].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[3].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        set_screen_obj_alpha(screen, alpha);
    }
    screen = pdata->art[4].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[4].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        set_screen_obj_alpha(screen, alpha);
    }
    screen = pdata->art[5].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[5].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        set_screen_obj_alpha(screen, alpha);
    }
    description = pdata->description.object;
    if (description != 0) {
        if (description->instance != pdata->description.instance) {
            description = 0;
        }
    } else {
        description = 0;
    }
    if (description != 0) {
        set_string_obj_alpha(description, alpha);
    }
}


/*
 * Near match: all three display modes, asset and string construction, seven
 * typed alpha latches, mode waits, completion signal, fade-out, and cleanup
 * calls agree. The 24-byte residue is equivalent latch/loop block placement,
 * FPR/GPR allocation, scheduling, and pooled relocation labels.
 */
static float p_display_award_image(void) {
    KonquestAwardArtPdata* pdata;
    ScreenObj* object;
    int art_id;
    int mode_index;
    float alpha;

    pdata = (KonquestAwardArtPdata*)pdata_of_proc(aproc);
    alpha = 0.0f;
    if (pdata == 0) {
        return -1.0f;
    }
    while (g_game_info.flag_bits.pad_bit1 == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    switch (pdata->mode) {
    case 0:
        load_ssf(krypt_art_file_table);
        art_id = load_pix_section(
            0x60027, chest_data, pdata->value, konquest_data_loaded, 0);

        object = load_2d_pfxobj_xy(
            0x60027, 0x8301, (char*)art_id, 0,
            screen_width / 2 - 0xC0, screen_height / 2 - 0x80, 0x42);
        set_screen_obj_alpha(object, 0.0f);
        pdata->art[0].obj = object;
        pdata->art[0].obj_instance = object->instance;

        object = load_2d_pfxobj_xy(
            0x60027, 0x8301, (char*)(art_id + 1), 0,
            screen_width / 2 + 0x40, screen_height / 2 - 0x80, 0x42);
        set_screen_obj_alpha(object, 0.0f);
        pdata->art[1].obj = object;
        pdata->art[1].obj_instance = object->instance;

        object = load_named_2d_pfxobj_xy(
            0x60030, 0x830F, "AWARD_NOTICE_TOP", 0,
            screen_width / 2 - 0x100, screen_height / 2 + 0x80, 0x43);
        set_screen_obj_alpha(object, 0.0f);
        pdata->art[4].obj = object;
        pdata->art[4].obj_instance = object->instance;

        object = load_named_2d_pfxobj_xy(
            0x60030, 0x830F, "AWARD_NOTICE_LEFT", 0,
            screen_width / 2 - 0xD0, screen_height / 2 - 0x80, 0x43);
        set_screen_obj_alpha(object, 0.0f);
        pdata->art[3].obj = object;
        pdata->art[3].obj_instance = object->instance;

        object = load_named_2d_pfxobj_xy(
            0x60030, 0x830F, "AWARD_NOTICE_RIGHT", 0,
            screen_width / 2 + 0xC0, screen_height / 2 - 0x80, 0x43);
        set_screen_obj_alpha(object, 0.0f);
        pdata->art[2].obj = object;
        pdata->art[2].obj_instance = object->instance;

        object = load_named_2d_pfxobj_xy(
            0x60030, 0x830F, "AWARD_NOTICE_BOTTOM", 0,
            screen_width / 2 - 0x100, screen_height / 2 - 0xC0, 0x43);
        set_screen_obj_alpha(object, 0.0f);
        pdata->art[5].obj = object;
        pdata->art[5].obj_instance = object->instance;

        snd_req(0x159B);
        move_picture_to_camera(&konquest_pdata->award_picture);
        break;
    case 1:
        load_art_for_inventory_award(pdata);
        snd_req(0x159B);
        do {
            alpha += 0.05f;
            set_award_display_alpha(pdata, alpha);
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        } while (alpha < 1.0f);
        break;
    case 2:
        load_art_for_inventory_award(pdata);
        snd_req(0x159B);
        break;
    default:
        return -1.0f;
    }

    set_award_display_alpha(pdata, 1.0f);
    if (pdata->mode == 0) {
        display_prize_description(
            chest_data, pdata->value, 0x49, konquest_data_loaded,
            0x50000, 0x42);
    }

    mode_index = konquest_pdata->game_mode_index;
    if (konquest_current_game_mode() == 12 && mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
    _mkproc_sleep_ticks = 30.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    while (konquest_current_game_mode() != 12) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (pdata->mode == 0) {
        remove_prize_description();
    }
    pdata->complete = 1;
    alpha = 1.0f;
    while (alpha > 0.0f) {
        alpha -= 0.05f;
        set_award_display_alpha(pdata, alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    destroy_award_art(pdata);
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
    return -1.0f;
}

/* Soft ceiling: stop_chest_camera_script ~93.85% -- call/register scheduling; stop. */
void stop_chest_camera_script(void) {
    CameraPdata* camera_pdata;

    camera_pdata = get_pdata_of_camera();
    if (camera_pdata != 0 && find_mkproc_pid(0x9006) != 0) {
        camera_pdata->flags |= 0x40;
        if (get_game_state() == 0x14) {
            set_interior_cam_pos_and_ang();
        }
        camera_exit_script();
    }
}

/*
 * Near match: all six ScreenObj latches and the StringObj latch are destroyed
 * in retail's nonascending order with concrete vtable types. The 28-byte
 * residue is repeated pointer-reload elimination and GPR/block scheduling.
 */
static void destroy_award_art(KonquestAwardArtPdata* pdata) {
    ScreenObj* screen;
    StringObj* string;

    screen = pdata->art[0].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[0].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (pdata->art[0].obj->instance != 0) {
            pdata->art[0].obj->typed_vtbl->destroy(pdata->art[0].obj);
        }
        pdata->art[0].obj = 0;
        pdata->art[0].obj_instance = 0;
    }

    screen = pdata->art[1].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[1].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (pdata->art[1].obj->instance != 0) {
            pdata->art[1].obj->typed_vtbl->destroy(pdata->art[1].obj);
        }
        pdata->art[1].obj = 0;
        pdata->art[1].obj_instance = 0;
    }

    screen = pdata->art[3].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[3].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (pdata->art[3].obj->instance != 0) {
            pdata->art[3].obj->typed_vtbl->destroy(pdata->art[3].obj);
        }
        pdata->art[3].obj = 0;
        pdata->art[3].obj_instance = 0;
    }

    screen = pdata->art[2].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[2].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (pdata->art[2].obj->instance != 0) {
            pdata->art[2].obj->typed_vtbl->destroy(pdata->art[2].obj);
        }
        pdata->art[2].obj = 0;
        pdata->art[2].obj_instance = 0;
    }

    screen = pdata->art[4].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[4].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (pdata->art[4].obj->instance != 0) {
            pdata->art[4].obj->typed_vtbl->destroy(pdata->art[4].obj);
        }
        pdata->art[4].obj = 0;
        pdata->art[4].obj_instance = 0;
    }

    screen = pdata->art[5].obj;
    if (screen != 0) {
        if (screen->instance != pdata->art[5].obj_instance) {
            screen = 0;
        }
    } else {
        screen = 0;
    }
    if (screen != 0) {
        if (pdata->art[5].obj->instance != 0) {
            pdata->art[5].obj->typed_vtbl->destroy(pdata->art[5].obj);
        }
        pdata->art[5].obj = 0;
        pdata->art[5].obj_instance = 0;
    }

    string = pdata->description.object;
    if (string != 0) {
        if (string->instance != pdata->description.instance) {
            string = 0;
        }
    } else {
        string = 0;
    }
    if (string != 0) {
        if (pdata->description.object->instance != 0) {
            pdata->description.object->typed_vtbl->destroy(
                pdata->description.object);
        }
        pdata->description.object = 0;
        pdata->description.instance = 0;
    }
}

/*
 * Near match: table bounds, section-name construction, four image loads,
 * currency/name text selection, uppercase conversion, and wrapped-string
 * insertion agree. The 28-byte residue is GPR allocation, scheduling, and
 * equivalent branch placement in the bounds and character loops.
 */
static void load_art_for_inventory_award(KonquestAwardArtPdata* pdata) {
    KonquestPuiDefinition* item;
    ScreenObj* object;
    StringObj* description;
    PfxFontSlot* font;
    const char* art_name;
    unsigned char* cursor;
    unsigned int table_index;
    unsigned int first;
    int inventory_index;
    int type;
    char text[0x50];

    if (pdata == 0) {
        return;
    }
    item = pdata->item;
    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index < 0) {
        return;
    }

    load_ssf(konquest_pui_award_tgas_file_table);
    if (strcmp(item->award_art_name, "") == 0) {
        art_name = 0;
    } else if (strstr(item->award_art_name, "ICON_") == 0) {
        art_name = 0;
    } else {
        art_name = item->award_art_name + 5;
    }
    sprintf(text, "%s.sec", art_name);
    strlwr(text);
    load_art_section_by_name(0x60027, text);

    object = load_named_2d_pfxobj_xy(
        0x60027, 0x8301, "AWARD_TGA", 0,
        screen_width / 2 - 0x80, screen_height / 2 - 0x80, 0x42);
    set_screen_obj_alpha(object, 0.0f);
    pdata->art[0].obj = object;
    pdata->art[0].obj_instance = object->instance;

    object = load_named_2d_pfxobj_xy(
        0x60030, 0x8301, "RECEIVENOTICEB", 0,
        screen_width / 2 - 0x89, screen_height / 2 - 0x80, 0x43);
    set_screen_obj_alpha(object, 0.0f);
    pdata->art[3].obj = object;
    pdata->art[3].obj_instance = object->instance;

    object = load_named_2d_pfxobj_xy(
        0x60030, 0x8301, "RECEIVENOTICEC", 0,
        screen_width / 2 + 0x80, screen_height / 2 - 0x80, 0x43);
    set_screen_obj_alpha(object, 0.0f);
    pdata->art[2].obj = object;
    pdata->art[2].obj_instance = object->instance;

    object = load_named_2d_pfxobj_xy(
        0x60030, 0x8301, "RECEIVENOTICEA", 0,
        (screen_width - 0x280) / 2, screen_height / 2 + 0x6D, 0x43);
    set_screen_obj_alpha(object, 0.0f);
    pdata->art[4].obj = object;
    pdata->art[4].obj_instance = object->instance;

    type = item->type;
    if (type >= 3 && type <= 8) {
        sprintf(
            text, "%d %s", item->value,
            get_string_by_id(koin_strings[type - 3] | 0x30000));
    } else {
        strcpy(text, get_konq_profile_value_item_name(inventory_index));
    }
    cursor = (unsigned char*)text;
    while (*cursor != 0) {
        if ((*cursor >= 'a' && *cursor <= 'z') || *cursor >= 0xE0) {
            *cursor -= 0x20;
        }
        cursor++;
    }

    font = load_font(9);
    description = create_wrapped_string(
        0x8300, font, text, screen_width / 2 - 0x80,
        screen_height / 2 - 0x5D, 0x100, 0, 1, 0);
    if (description != 0) {
        description->priority = 0x40;
        insert_2d_obj((ScreenObj*)description);
    }
    set_string_obj_alpha(description, 0.0f);
    pdata->description.object = description;
    pdata->description.instance = description->instance;
}

/*
 * Soft ceiling: both 64-bit profile/default masks, shift helper, and bit test
 * match retail. The remaining eight bytes are individual r30/r31
 * saves/restores instead of stmw/lmw, with harmless GPR coloring in the loads.
 */
int is_character_unlocked_in_profile(
    int character, int alternate) {
    unsigned long long bits;
    unsigned long long mask;

    if (alternate != 0) {
        bits = (((unsigned long long)
                     p1_profile_common->alternate_character_bits[0]
                 << 32) |
                p1_profile_common->alternate_character_bits[1]) |
               default_alt_char_bits;
    } else {
        bits = (((unsigned long long)p1_profile_common->character_bits[0]
                 << 32) |
                p1_profile_common->character_bits[1]) |
               default_char_bits;
    }
    mask = (unsigned long long)1 << character;
    if ((bits & mask) != 0) {
        return 1;
    }
    return 0;
}

void mini_mission_inactive(int mission) {
    set_konq_profile_value(3, mission, 0);
}

void mini_mission_completed(int mission) {
    snd_req(0x15a1);
    set_konq_profile_value(4, mission, 1);
    set_konq_profile_value(3, mission, 0);
}

void start_mini_mission(int mission) {
    snd_req(0x159C);
    if (get_konq_profile_value(4, mission) == 0) {
        set_konq_profile_value(2, mission, 1);
        set_konq_profile_value(3, mission, 1);
    }
}

int is_mini_mission_started(int mission) {
    return get_konq_profile_value(2, mission);
}

int is_mini_mission_active(int mission) {
    return get_konq_profile_value(3, mission);
}

int is_mini_mission_completed(int mission) {
    return get_konq_profile_value(4, mission);
}

PuiItem* get_pui_item_at_inv_bit_index(int index) {
    unsigned int table_index;

    table_index = konquest_pdata->pui_begin + 1;
    table_index = index + table_index;
    if (table_index >= konquest_pdata->pui_end) {
        return 0;
    }
    return (PuiItem*)get_data_table(
        konquest_pdata->script_owner, table_index);
}

static inline int konquest_num_puis(void) {
    return konquest_pdata->pui_end - konquest_pdata->pui_begin - 1;
}

void player_add_item_to_inventory(KonquestPuiDefinition* item) {
    int index;
    unsigned int table_index;
    int value;

    if (item == 0) {
        index = -1;
    } else {
        table_index =
            get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (table_index <= konquest_pdata->pui_begin ||
            table_index >= konquest_pdata->pui_end) {
            index = -1;
        } else {
            index = table_index - (konquest_pdata->pui_begin + 1);
        }
    }
    if (index < 0) {
        return;
    }

    set_u8_bit(
        &p1_profile_konquest->raw[0x291], konquest_num_puis(), index, 1);
    if (item == 0) {
        return;
    }

    switch (item->type) {
    case 3:
        value = item->value + get_konq_profile_value(7, 0);
        set_konq_profile_value(7, 0, value);
        return;
    case 4:
        value = item->value + get_konq_profile_value(8, 0);
        set_konq_profile_value(8, 0, value);
        return;
    case 5:
        value = item->value + get_konq_profile_value(9, 0);
        set_konq_profile_value(9, 0, value);
        return;
    case 6:
        value = item->value + get_konq_profile_value(10, 0);
        set_konq_profile_value(10, 0, value);
        return;
    case 7:
        value = item->value + get_konq_profile_value(11, 0);
        set_konq_profile_value(11, 0, value);
        return;
    case 8:
        value = item->value + get_konq_profile_value(12, 0);
        set_konq_profile_value(12, 0, value);
        return;
    default:
        return;
    }
}

void player_remove_item_from_inventory(void* item) {
    int index;
    unsigned int table_index;

    if (item == 0) {
        index = -1;
    } else {
        table_index = get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (table_index <= konquest_pdata->pui_begin ||
            table_index >= konquest_pdata->pui_end) {
            index = -1;
        } else {
            index = table_index - (konquest_pdata->pui_begin + 1);
        }
    }
    if (index >= 0) {
        set_u8_bit(
            &p1_profile_konquest->raw[0x291], konquest_num_puis(), index, 0);
    }
}

int player_has_item(void* item) {
    int index;
    unsigned int table_index;

    if (item == 0) {
        index = -1;
    } else {
        table_index = get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (table_index <= konquest_pdata->pui_begin ||
            table_index >= konquest_pdata->pui_end) {
            index = -1;
        } else {
            index = table_index - (konquest_pdata->pui_begin + 1);
        }
    }
    if (index >= 0) {
        return get_u8_bit(
            &p1_profile_konquest->raw[0x291], konquest_num_puis(), index);
    }
    return 0;
}

/*
 * Soft ceiling: update_dropped_pui 80.51% (772 versus 780 bytes). Both object
 * latches, drop timer, collision integration, bounce damping, settle flags,
 * and owner-position update agree with m2c and retail's sole caller. Residue
 * is arithmetic scheduling and register allocation.
 */
static void update_dropped_pui(KonquestPuiDelayView* pui) {
    KonquestChestOwner* owner;
    MkObj* object;
    MkObj* settled_object;
    Vec target;
    Vec hit;

    owner = pui->owner;
    if (owner != 0) {
        if (owner->hdr.instance != pui->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }
    if (owner == 0) {
        if (pui->hdr.instance != 0) {
            pui->hdr.typed_vtbl->destroy(&pui->hdr);
        }
        return;
    }

    object = pui->render_object;
    if (object != 0) {
        if (object->hdr.instance != pui->render_object_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object == 0) {
        if (pui->hdr.instance != 0) {
            pui->hdr.typed_vtbl->destroy(&pui->hdr);
        }
        return;
    }

    pui->drop_timer -= game_speed;
    if (pui->drop_timer < 0.0f) {
        owner->interaction->closed = 1;
        pui->drop_timer = 0.0f;
    }
    object->pos_vel.y += object->gravity * game_speed;
    target.x = object->pos.value.x + object->pos_vel.x * game_speed;
    target.y = object->pos.value.y + object->pos_vel.y * game_speed;
    target.z = object->pos.value.z + object->pos_vel.z * game_speed;
    if (collide_segment_against_global_collision_list(
            &object->pos.value, &target, &hit, 0x10000) != 0) {
        object->pos.value.x += 0.6f * (hit.x - object->pos.value.x);
        object->pos.value.y += 0.6f * (hit.y - object->pos.value.y);
        object->pos.value.z += 0.6f * (hit.z - object->pos.value.z);
        object->pos_vel.x *= -1.05f;
        object->pos_vel.z *= -1.05f;
    } else {
        object->pos.value = target;
    }
    if (object->pos_vel.y < 0.0f && object->pos.value.y < 0.0f) {
        object->pos.value.y = 0.0f;
        object->pos_vel.x *= 0.5f;
        object->pos_vel.y *= 0.5f;
        object->pos_vel.z *= 0.5f;
        object->pos_vel.y *= -1.0f;
        pui->bounce_count++;
        if (pui->bounce_count >= 2) {
            settled_object = pui->render_object;
            if (settled_object != 0) {
                if (settled_object->hdr.instance !=
                    pui->render_object_instance) {
                    settled_object = 0;
                }
            } else {
                settled_object = 0;
            }
            if (settled_object != 0) {
                settled_object->flags_08_bits.airborne = 1;
                settled_object->flags_08_bits.angular_velocity_enabled = 1;
                settled_object->flags_08_bits.moving = 0;
                settled_object->pos_vel.z = 0.0f;
                settled_object->pos_vel.y = 0.0f;
                settled_object->pos_vel.x = 0.0f;
                settled_object->pos_vel.y = 0.005f;
            }
            pui->state = 1;
            owner->interaction->closed = 1;
            pui->drop_timer = 0.0f;
        }
    }
    owner->interaction->position = object->pos.value;
}

/*
 * Soft ceiling: update_konquest_pui 92.75% (976 versus 964 bytes). The timed
 * kill, five-state switch with separate state-1/state-2 bodies, child/effect
 * positioning, delayed reveal, alpha ramp, and priorities have the exact
 * retail call sequence. Residue is scheduling and register allocation.
 */
static void update_konquest_pui(KonquestPuiRuntime* runtime) {
    KonquestPuiRuntime* pui = runtime;
    MkObj* object;
    MkObj* effect_object;
    MkSobj* child;

    object = pui->render_object;
    if (object != 0) {
        if (object->hdr.instance != pui->render_object_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object == 0) {
        return;
    }
    if (pui->flag_bits.bit7 &&
        is_time_a_greater_than_time_b(
            &konquest_pdata->current_time, &pui->kill_time) != 0) {
        kill_dynamic_pui(pui->item);
        return;
    }

    switch (pui->state) {
    case 0:
    case 4:
        break;
    case 3:
        update_dropped_pui(pui);
        break;
    case 1:
        object = pui->render_object;
        if (object != 0) {
            if (object->hdr.instance != pui->render_object_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        object->pos.value.x += object->pos_vel.x * game_speed;
        object->pos.value.y += object->pos_vel.y * game_speed;
        object->pos.value.z += object->pos_vel.z * game_speed;
        if (object->pos_vel.y > 0.0f) {
            if (object->pos.value.y > pui->base_y + 0.25f) {
                object->pos.value.y = pui->base_y + 0.25f;
                object->pos_vel.y *= -1.0f;
            }
        } else if (object->pos.value.y < pui->base_y) {
            object->pos.value.y = pui->base_y;
            object->pos_vel.y *= -1.0f;
        }
        object->pos_vel.z = 0.0f;
        object->pos_vel.x = 0.0f;
        break;
    case 2:
        object = pui->render_object;
        if (object != 0) {
            if (object->hdr.instance != pui->render_object_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        object->pos.value.x += object->pos_vel.x * game_speed;
        object->pos.value.y += object->pos_vel.y * game_speed;
        object->pos.value.z += object->pos_vel.z * game_speed;
        if (object->pos_vel.y > 0.0f) {
            if (object->pos.value.y > pui->base_y + 0.25f) {
                object->pos.value.y = pui->base_y + 0.25f;
                object->pos_vel.y *= -1.0f;
            }
        } else if (object->pos.value.y < pui->base_y) {
            object->pos.value.y = pui->base_y;
            object->pos_vel.y *= -1.0f;
        }
        object->pos_vel.z = 0.0f;
        object->pos_vel.x = 0.0f;
        break;
    }

    child = obj_find_sobj_by_id(object, 2);
    if (child != 0) {
        child->pos.z = 0.0f;
        child->pos.y = 0.0f;
        child->pos.x = 0.0f;
        child->pos.y = -object->pos.value.y;
    }
    if (pui->effect_clone != 0) {
        effect_object = (MkObj*)pui->effect_clone->bind_hdr;
        if (effect_object != 0) {
            if (effect_object->hdr.instance !=
                pui->effect_clone->bind_inst) {
                effect_object = 0;
            }
        } else {
            effect_object = 0;
        }
        child = obj_find_sobj_by_id(object, 1);
        effect_object->pos.value.x = child->pos.x + object->pos.value.x;
        effect_object->pos.value.y = child->pos.y + object->pos.value.y;
        effect_object->pos.value.z = child->pos.z + object->pos.value.z;
    }
    if (pui->flag_bits.bit6) {
        if (pui->fade_delay > 0.0f) {
            pui->fade_delay -= get_game_speed();
            if (pui->fade_delay <= 0.0f) {
                pui->fade_delay = 0.0f;
                unhide_obj(object);
            }
        } else if (pui->alpha < 0xFF) {
            pui->alpha += 0xF;
            obj_for_all_atomics_set_material_alpha(object, pui->alpha);
        } else {
            pui->alpha = 0xFF;
            obj_for_all_atomics_set_material_alpha(object, pui->alpha);
            obj_set_all_sobjs_priority(object, 0x10);
            if (child != 0) {
                sobj_set_priority(child, 0x13);
            }
        }
    }
}

/*
 * Near match: all five behavior modes, seven instance latches, typed object
 * flags, velocity initialization, and inverse-length math agree. MWCC folds
 * the valid-path join branch from each structured latch, accounting for the
 * complete 28-byte residue; other annotations are register/relocation labels.
 */
static void setup_konquest_pui(KonquestPuiRuntime* pui) {
    KonquestChestOwner* owner;
    MkObj* object;
    MkObj* hero;
    float x;
    float z;
    float length_squared;
    float inverse_length;
    float product;
    float correction;
    KonquestFloatBits estimate;

    switch (pui->behavior) {
    case 0:
        object = pui->object;
        if (object != 0) {
            if (object->hdr.instance == pui->object_instance) {
                /* Valid object latch. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            object->flags_08_bits.airborne = 1;
            object->flags_08_bits.angular_velocity_enabled = 1;
        }
        return;
    case 1:
        object = pui->object;
        if (object != 0) {
            if (object->hdr.instance == pui->object_instance) {
                /* Valid object latch. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            object->flags_08_bits.airborne = 1;
            object->flags_08_bits.angular_velocity_enabled = 1;
            object->flags_08_bits.moving = 0;
            object->pos_vel.z = 0.0f;
            object->pos_vel.y = 0.0f;
            object->pos_vel.x = 0.0f;
            object->pos_vel.y = 0.005f;
        }
        return;
    case 2:
        object = pui->object;
        if (object != 0) {
            if (object->hdr.instance == pui->object_instance) {
                /* Valid object latch. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            object->flags_08_bits.angular_velocity_enabled = 1;
            object->flags_08_bits.rotation_enabled = 1;
            object->flags_08_bits.airborne = 1;
            object->flags_08_bits.moving = 0;
            object->pos_vel.z = 0.0f;
            object->pos_vel.y = 0.0f;
            object->pos_vel.x = 0.0f;
            object->pos_vel.y = 0.005f;
            object->ang_vel.y = 0.05f;
        }
        return;
    case 3:
        owner = pui->owner;
        if (owner != 0) {
            if (owner->hdr.instance == pui->owner_instance) {
                /* Valid owner latch. */
            } else {
                owner = 0;
            }
        } else {
            owner = 0;
        }
        object = pui->object;
        if (object != 0) {
            if (object->hdr.instance == pui->object_instance) {
                /* Valid object latch. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        hero = konquest_pdata->hero_object;
        if (hero != 0) {
            if (hero->hdr.instance == konquest_pdata->hero_instance) {
                /* Valid hero latch. */
            } else {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero != 0 && object != 0 && owner != 0) {
            x = hero->field_24->at.x;
            z = hero->field_24->at.z;
            length_squared = x * x + z * z;
            if (length_squared <= 0.0f) {
                inverse_length = 0.0f;
            } else {
                estimate.value = length_squared;
                estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
                product = estimate.value *
                          (length_squared * estimate.value);
                correction = 3.0f - product;
                inverse_length =
                    0.0625f * estimate.value * correction *
                    -(correction * (product * correction) - 12.0f);
            }
            object->pos_vel.x = 0.06f * (x * inverse_length);
            object->pos_vel.y = 0.15f;
            object->pos_vel.z = 0.06f * (z * inverse_length);
            object->gravity = -0.007f;
            object->flags_08_bits.airborne = 1;
            object->flags_08_bits.angular_velocity_enabled = 1;
            owner->interaction->closed = 0;
            pui->lifetime = 60.0f;
        }
        return;
    case 4:
        object = pui->object;
        if (object != 0) {
            if (object->hdr.instance == pui->object_instance) {
                /* Valid object latch. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            object->flags_08_bits.angular_velocity_enabled = 1;
            object->flags_08_bits.rotation_enabled = 1;
            object->flags_08_bits.airborne = 1;
            object->flags_08_bits.moving = 0;
            object->ang_vel.y = 0.05f;
        }
        return;
    }
}

/*
 * Soft ceiling: pickup_pui 88.42% (1116 versus 1124 bytes). All twenty-one
 * calls, three inventory lookups, award dispatch, profile updates, and object
 * destruction agree; residue is repeated induction-register allocation and
 * equivalent branch/instruction scheduling.
 */
void pickup_pui(KonquestPuiDefinition* item) {
    KonquestPuiRuntime* pui;
    KonquestKoinAwardPdata* award;
    MkHdr* object;
    unsigned int table_index;
    unsigned int first;
    int inventory_index;
    int value;

    pui = find_pui_runtime_by_id(item);
    if (pui == 0) {
        return;
    }

    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index < 0) {
        return;
    }

    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index >= 0) {
        set_u8_bit(
            konquest_pdata->pui_inventory_bits,
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index, 0);
    }

    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index >= 0) {
        set_u8_bit(
            &p1_profile_konquest->raw[0x291],
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index, 1);
        if (item != 0) {
            switch (item->type) {
            case 3:
                value = item->value + get_konq_profile_value(7, 0);
                set_konq_profile_value(7, 0, value);
                break;
            case 4:
                value = item->value + get_konq_profile_value(8, 0);
                set_konq_profile_value(8, 0, value);
                break;
            case 5:
                value = item->value + get_konq_profile_value(9, 0);
                set_konq_profile_value(9, 0, value);
                break;
            case 6:
                value = item->value + get_konq_profile_value(10, 0);
                set_konq_profile_value(10, 0, value);
                break;
            case 7:
                value = item->value + get_konq_profile_value(11, 0);
                set_konq_profile_value(11, 0, value);
                break;
            case 8:
                value = item->value + get_konq_profile_value(12, 0);
                set_konq_profile_value(12, 0, value);
                break;
            }
        }
    }

    if (item->type >= 3 && item->type <= 8) {
        award = 0;
        if (find_mkproc_pid(0x8247) != 0) {
            object = konquest_pdata->award_text.object;
            if (object != 0 &&
                object->instance != konquest_pdata->award_text.instance) {
                object = 0;
            }
            if (object != 0) {
                if (object->instance != 0) {
                    object->typed_vtbl->destroy(object);
                }
                konquest_pdata->award_text.object = 0;
                konquest_pdata->award_text.instance = 0;
            }
            object = konquest_pdata->award_art.object;
            if (object != 0 &&
                object->instance != konquest_pdata->award_art.instance) {
                object = 0;
            }
            if (object != 0) {
                if (object->instance != 0) {
                    object->typed_vtbl->destroy(object);
                }
                konquest_pdata->award_art.object = 0;
                konquest_pdata->award_art.instance = 0;
            }
            destroy_mkprocs_pid(0x8247);
        }
        if (_create_mkproc_generic_tinystack(
                0x8247, 0x1F, p_show_koin_award_text,
                sizeof(*award), (void**)&award) != 0 && award != 0) {
            award->icon_offset = item->type - 3;
            award->amount = item->value;
        }
    }
    if (pui->hdr.instance != 0) {
        pui->hdr.typed_vtbl->destroy(&pui->hdr);
    }
}

void pickup_dynamic_pui(KonquestPuiDefinition* item) {
    int valid;
    unsigned int table_index;
    int pid;

    if (item == 0) {
        valid = 0;
    } else {
        table_index =
            get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (table_index >= konquest_pdata->dynamic_pui_end ||
            table_index <= konquest_pdata->dynamic_pui_begin) {
            valid = 0;
        } else {
            valid = 1;
        }
    }
    if (valid == 0) {
        return;
    }

    pid = aproc->pid;
    if (pid == 0x9019) {
        cmdscript_set_parameters(active_cmdscript, 2, item, 1);
    } else if (pid == 0xA002 || pid == 0xA014 || pid == 0xA017) {
        cmdscript_set_parameters(active_cmdscript, 2, item, 2);
    }
    cmdscript_setup_execution(
        konquest_pdata->script_owner, item->pickup_script_index);
    cmdscript_execute(konquest_pdata->script_owner);
}
static inline KonquestPuiDelayView* find_pui_runtime_by_numeric_id(
    unsigned int id) {
    MkPtr* link;

    if (konquest_pdata->pui_list != 0) {
        link = konquest_pdata->pui_list;
        while (link != 0) {
            KonquestPuiDelayView* pui;

            pui = (KonquestPuiDelayView*)link->hdr;
            if (link->instance != pui->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (pui != 0 && pui->id == id) {
                    return pui;
                }
                link = link->next;
            }
        }
    }
    return 0;
}


/*
 * Near match: stale-safe numeric lookup, render-object latch, byte color
 * stores, and material update agree. Retail retains one valid-latch join
 * branch that MWCC folds here; the other differences are register coloring.
 */
void pui_set_color(
    unsigned int id, unsigned char red, unsigned char green,
    unsigned char blue, unsigned char alpha) {
    KonquestPuiDelayView* object;
    MkObj* render_object;
    RwRGBA color;

    object = find_pui_runtime_by_numeric_id(id);

    if (object != 0) {
        render_object = object->render_object;
        if (render_object != 0) {
            if (render_object->hdr.instance !=
                object->render_object_instance) {
                render_object = 0;
            }
        } else {
            render_object = 0;
        }
        if (render_object != 0) {
            color.red = red;
            color.green = green;
            color.blue = blue;
            color.alpha = alpha;
            obj_set_color_for_material_by_id(render_object, 5, &color);
        }
    }
}

static float p_show_koin_award_text(void) {
    KonquestKoinAwardPdata* pdata;
    StringObj* text_object;
    ScreenObj* icon_object;
    MkHdr* text;
    MkHdr* icon;
    KonquestArtIdRef art;
    char amount[12];

    pdata = (KonquestKoinAwardPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }

    sprintf(amount, "%d", pdata->amount);
    text_object = string_left_xy(0x8300, 0xE, amount, 0x64, 0x32, 0x44);
    art.id = pdata->icon_offset + 0x0A8C0024;
    icon_object = load_2d_pfxobj_xy(
        0x60030, 0x8301, art.name, 0, 0x28, 0x1E, 0x44);
    konquest_pdata->award_text.object = (MkHdr*)text_object;
    konquest_pdata->award_text.instance = text_object->instance;
    konquest_pdata->award_art.object = (MkHdr*)icon_object;
    konquest_pdata->award_art.instance = icon_object->instance;

    _mkproc_sleep_ticks = 120.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    text = konquest_pdata->award_text.object;
    if (text != 0 &&
        text->instance != konquest_pdata->award_text.instance) {
        text = 0;
    }
    icon = konquest_pdata->award_art.object;
    if (icon != 0 &&
        icon->instance != konquest_pdata->award_art.instance) {
        icon = 0;
    }
    if (text != 0) {
        if (text->instance != 0) {
            ((KonquestDestroyable*)text)->vtbl->destroy(
                (KonquestDestroyable*)text);
        }
        konquest_pdata->award_text.object = 0;
        konquest_pdata->award_text.instance = 0;
    }
    if (icon != 0) {
        if (icon->instance != 0) {
            ((KonquestDestroyable*)icon)->vtbl->destroy(
                (KonquestDestroyable*)icon);
        }
        konquest_pdata->award_art.object = 0;
        konquest_pdata->award_art.instance = 0;
    }
    return -1.0f;
}

/*
 * Soft ceiling: pui_play_pfx_sequence 89.60% (876 versus 908 bytes). All
 * nine calls, mode behavior, stale-safe latches, frustum test, process data,
 * and child-position calculation agree; residue is GPR allocation, branch
 * layout, and save/restore scheduling.
 */
void pui_play_pfx_sequence(
    KonquestPuiDefinition* item, int mode,
    KonquestPuiPfxSequenceRow* sequence) {
    KonquestPuiRuntime* pui;
    KonquestChestOwner* owner;
    MkObj* object;
    MkSobj* child;
    KonquestPuiPfxSequencePdata* pdata;
    RwSphere sphere;

    pui = find_pui_runtime_by_id(item);
    if (pui == 0 || sequence == 0) {
        return;
    }
    switch (mode) {
    case 0:
        pui->flag_bits.bit3 = 1;
        pui->flag_bits.bit4 = 0;
        pui->effect_sequence = sequence;
        return;
    case 4:
        object = pui->render_object;
        if (object != 0) {
            if (object->hdr.instance != pui->render_object_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        owner = pui->owner;
        if (owner != 0) {
            if (owner->hdr.instance != pui->owner_instance) {
                owner = 0;
            }
        } else {
            owner = 0;
        }
        if (object != 0 && owner != 0) {
            sphere.center.x = object->pos.value.x;
            sphere.center.y = object->pos.value.y;
            sphere.center.z = object->pos.value.z;
            sphere.radius = owner->interaction->radius;
            if (RwCameraFrustumTestSphere(Camera, &sphere) != 0 &&
                _create_mkproc_generic_nostack(
                    0xA01D, 0x1F, p_pui_pfx_sequence,
                    sizeof(*pdata), (void**)&pdata) != 0) {
                zero_pdata_payload(sizeof(*pdata), &pdata->hdr);
                pdata->sequence = sequence;
                pdata->index = 0;
                pdata->delay = pdata->sequence->delay;
                child = obj_find_sobj_by_id(object, 1);
                pdata->position.x = object->pos.value.x + child->pos.x;
                pdata->position.y = object->pos.value.y + child->pos.y;
                pdata->position.z = object->pos.value.z + child->pos.z;
                return;
            }
        }
        break;
    case 5:
        object = pui->render_object;
        if (object != 0) {
            if (object->hdr.instance != pui->render_object_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        owner = pui->owner;
        if (owner != 0) {
            if (owner->hdr.instance != pui->owner_instance) {
                owner = 0;
            }
        } else {
            owner = 0;
        }
        if (object != 0 && owner != 0) {
            sphere.center.x = object->pos.value.x;
            sphere.center.y = object->pos.value.y;
            sphere.center.z = object->pos.value.z;
            sphere.radius = owner->interaction->radius;
            if (RwCameraFrustumTestSphere(Camera, &sphere) != 0 &&
                _create_mkproc_generic_nostack(
                    0xA01D, 0x1F, p_pui_pfx_sequence,
                    sizeof(*pdata), (void**)&pdata) != 0) {
                zero_pdata_payload(sizeof(*pdata), &pdata->hdr);
                pdata->sequence = sequence;
                pdata->index = 0;
                pdata->delay = pdata->sequence->delay;
                child = obj_find_sobj_by_id(object, 1);
                pdata->position.x = object->pos.value.x + child->pos.x;
                pdata->position.y = object->pos.value.y + child->pos.y;
                pdata->position.z = object->pos.value.z + child->pos.z;
            }
        }
        break;
    }
}

/*
 * Soft ceiling: pui_play_pfx 90.13% (904 versus 920 bytes). All twenty calls,
 * six modes, stale-safe latches, frustum gating, emitter placement, and PUI
 * effect state agree; residue is register allocation and instruction/branch
 * scheduling.
 */
void pui_play_pfx(
    KonquestPuiDefinition* item, int mode, const char* effect_name) {
    KonquestPuiRuntime* pui;
    KonquestChestOwner* owner;
    MkObj* object;
    MkObj* emitter_object;
    MkSobj* child;
    MkPfx* effect;
    RwSphere sphere;
    unsigned int handle;
    unsigned int emitter;

    pui = find_pui_runtime_by_id(item);
    if (pui == 0) {
        return;
    }
    handle = fx_by_owner(effect_name, 4);
    if (handle == 0) {
        return;
    }
    switch (mode) {
    case 0:
        pui->flag_bits.bit3 = 1;
        pui->flag_bits.bit4 = 1;
        pui->attached_effect = handle;
        return;
    case 4:
        object = pui->render_object;
        if (object != 0) {
            if (object->hdr.instance != pui->render_object_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        owner = pui->owner;
        if (owner != 0) {
            if (owner->hdr.instance != pui->owner_instance) {
                owner = 0;
            }
        } else {
            owner = 0;
        }
        if (object != 0 && owner != 0) {
            sphere.center.x = object->pos.value.x;
            sphere.center.y = object->pos.value.y;
            sphere.center.z = object->pos.value.z;
            sphere.radius = owner->interaction->radius;
            if (RwCameraFrustumTestSphere(Camera, &sphere) != 0) {
                effect = pfx_from_handle(handle);
                emitter = fx_next_emitter(handle);
                if (emitter != 0) {
                    emitter_object = (MkObj*)pfx_get_emitter_obj(
                        effect, emitter_id_from_handle(emitter));
                    child = obj_find_sobj_by_id(object, 1);
                    emitter_object->pos.value.x =
                        object->pos.value.x + child->pos.x;
                    emitter_object->pos.value.y =
                        object->pos.value.y + child->pos.y;
                    emitter_object->pos.value.z =
                        object->pos.value.z + child->pos.z;
                    update_mkobj(emitter_object);
                    fx_restart_emit(emitter);
                    fx_resume_emit(emitter);
                    return;
                }
            }
        }
        break;
    case 5:
        object = pui->render_object;
        if (object != 0) {
            if (object->hdr.instance != pui->render_object_instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        owner = pui->owner;
        if (owner != 0) {
            if (owner->hdr.instance != pui->owner_instance) {
                owner = 0;
            }
        } else {
            owner = 0;
        }
        if (object != 0 && owner != 0) {
            sphere.center.x = object->pos.value.x;
            sphere.center.y = object->pos.value.y;
            sphere.center.z = object->pos.value.z;
            sphere.radius = owner->interaction->radius;
            if (RwCameraFrustumTestSphere(Camera, &sphere) != 0) {
                effect = pfx_from_handle(handle);
                emitter = fx_next_emitter(handle);
                if (emitter != 0) {
                    emitter_object = (MkObj*)pfx_get_emitter_obj(
                        effect, emitter_id_from_handle(emitter));
                    child = obj_find_sobj_by_id(object, 1);
                    emitter_object->pos.value.x =
                        object->pos.value.x + child->pos.x;
                    emitter_object->pos.value.y =
                        object->pos.value.y + child->pos.y;
                    emitter_object->pos.value.z =
                        object->pos.value.z + child->pos.z;
                    update_mkobj(emitter_object);
                    fx_restart_emit(emitter);
                    fx_resume_emit(emitter);
                    return;
                }
            }
        }
        break;
    case 1:
        pui->flag_bits.bit0 = 0;
        pui->effect_60 = handle;
        return;
    case 2:
        pui->flag_bits.bit0 = 1;
        pui->effect_60 = handle;
        return;
    case 3:
        pui->effect_64 = handle;
        break;
    }
}

/*
 * Soft ceiling: pui_set_kill_time ~91.1% -- PUI lookup, stale-link cleanup,
 * flag update, timestamp contents, switch CFG, and time-adjustment calls
 * match. Residue is paired-copy loop lowering and caller-saved GPR coloring.
 */
void pui_set_kill_time(
    KonquestPuiDefinition* item, int unit, int amount) {
    KonquestPuiDelayView* object;

    object = find_pui_runtime_by_id(item);
    if (object != 0) {
        KonquestTimePair* source;
        KonquestTimePair* destination;
        int pairs_remaining;

        object->flag_bits.bit7 = 1;
        source = konquest_pdata->current_time.pairs;
        destination = object->kill_time.pairs;
        pairs_remaining = 3;
        do {
            *destination++ = *source++;
        } while (--pairs_remaining != 0);
        switch (unit) {
        case 0:
            add_minutes_to_time(&object->kill_time, amount);
            break;
        case 1:
            add_hours_to_time(&object->kill_time, amount);
            break;
        case 2:
            add_days_to_time(&object->kill_time, amount);
            break;
        case 3:
            add_months_to_time(&object->kill_time, amount);
            break;
        case 4:
            add_years_to_time(&object->kill_time, amount);
            break;
        }
    }
}

/*
 * Soft ceiling: pui_delay_spawn ~87.1% -- inlined stale-safe lookup, ID test,
 * unordered-aware delay guard, flag update, and delay store match retail.
 * Residue is individual r29-r31 saves/restores versus stmw/lmw.
 */
void pui_delay_spawn(KonquestPuiDefinition* item, float delay) {
    KonquestPuiDelayView* object;

    object = find_pui_runtime_by_id(item);
    if (object != 0 && !(delay <= 0.0f)) {
        object->flag_bits.bit5 = 1;
        object->spawn_delay = delay;
    }
}

/*
 * Near match: spawn_pui 94.51% (776 versus 768 bytes). Both item-type arms,
 * inventory/profile gates, trigger transfer, stale-safe object latch, inline
 * tile lookup, and all eight calls agree. Residue is two folded latch joins,
 * individual saves/restores instead of stmw/lmw, and GPR scheduling.
 */
void spawn_pui(
    KonquestPuiDefinition* item, int behavior, int position_mode) {
    KonquestPuiRuntime* pui;
    unsigned int table_index;
    unsigned int first;
    int inventory_index;
    int active;

    switch (item->type) {
    case 0:
        if (item == 0) {
            inventory_index = -1;
        } else {
            table_index = get_table_index_by_pointer(
                konquest_pdata->script_owner, item);
            first = konquest_pdata->pui_begin;
            if (table_index <= first ||
                table_index >= konquest_pdata->pui_end) {
                inventory_index = -1;
            } else {
                inventory_index = table_index - (first + 1);
            }
        }
        if (inventory_index >= 0) {
            active = get_u8_bit(
                konquest_pdata->pui_inventory_bits,
                (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
                inventory_index);
        } else {
            active = 0;
        }
        if (active == 0) {
            pui = create_new_konquest_pui(item, behavior, position_mode);
            if (pui != 0) {
                KonquestTriggerStruct* trigger;
                int saved;

                if (item == 0) {
                    inventory_index = -1;
                } else {
                    table_index = get_table_index_by_pointer(
                        konquest_pdata->script_owner, item);
                    first = konquest_pdata->pui_begin;
                    if (table_index <= first ||
                        table_index >= konquest_pdata->pui_end) {
                        inventory_index = -1;
                    } else {
                        inventory_index = table_index - (first + 1);
                    }
                }
                if (inventory_index >= 0) {
                    saved = get_u8_bit(
                        &p1_profile_konquest->raw[0x291],
                        (konquest_pdata->pui_end -
                         konquest_pdata->pui_begin) - 1,
                        inventory_index);
                } else {
                    saved = 0;
                }
                if (saved != 0) {
                    pui_restore_open_chests(item);
                }

                trigger = pui->trigger_owner;
                if (trigger != 0) {
                    if (trigger->hdr.instance != pui->owner_instance) {
                        trigger = 0;
                    }
                } else {
                    trigger = 0;
                }
                if (trigger != 0) {
                    trigger->owned_data->flags |= 2;
                }
                if (!(item->position.x >= 1000.0f)) {
                    konquest_pui_check_for_and_replace_old_chest(pui);
                }
            }
        }
        break;
    default: {
        MkObj* object;

        pui = create_new_konquest_pui(item, behavior, position_mode);
        if (pui != 0) {
            object = pui->render_object;
            if (object != 0) {
                if (object->hdr.instance != pui->render_object_instance) {
                    object = 0;
                }
            } else {
                object = 0;
            }
            if (object != 0) {
                KonquestTileRecord* tile;
                int tile_index;

                if (object->pos.value.x >= 1000.0f) {
                    tile_index =
                        konquest_pdata->tile_width *
                        konquest_pdata->tile_height;
                } else {
                    int row;
                    int column;

                    row = (int)((object->pos.value.z +
                                 konquest_pdata->tile_origin_z) /
                                60.0f);
                    if (row < 0) {
                        tile_index = -1;
                    } else if (row >= konquest_pdata->tile_height) {
                        tile_index = -1;
                    } else {
                        column = (int)((object->pos.value.x +
                                       konquest_pdata->tile_origin_x) /
                                      60.0f);
                        if (column < 0) {
                            tile_index = -1;
                        } else if (column >= konquest_pdata->tile_width) {
                            tile_index = -1;
                        } else {
                            tile_index =
                                column + row * konquest_pdata->tile_width;
                        }
                    }
                }
                if (tile_index <
                    konquest_pdata->tile_width *
                        konquest_pdata->tile_height + 1) {
                    tile = &konquest_pdata->tile_structs[tile_index];
                } else {
                    tile = 0;
                }
                /* Retail assumes the inline tile lookup succeeds here. */
                if (tile->visible != 0) {
                    trigger_update_countdown = 0;
                }
            }
        }
        break;
    }
    }
}

/*
 * Near match: create_new_konquest_pui 97.53% (1744 versus 1752 bytes). The
 * dynamic-source selection, three allocations, trigger/model construction,
 * typed ownership, all 22 direct calls, inventory updates, and retail's late
 * trigger null check agree. The eight-byte residue is allocation-temporary
 * scheduling; remaining annotations are register allocation.
 */
static KonquestPuiRuntime* create_new_konquest_pui(
    KonquestPuiDefinition* item, int behavior, int position_mode) {
    KonquestDynamicPuiPdata* dynamic;
    KonquestPuiRuntime* pui;
    KonquestTriggerStruct* trigger;
    MkObj* object;
    unsigned int table_index;
    unsigned int trigger_id;
    unsigned int first;
    int pickup_script;
    int inventory_index;
    int valid;
    int tile_index;
    float x;
    float y;
    float z;
    float radius;

    dynamic = 0;
    if (item == 0) {
        valid = 0;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        if (table_index >= konquest_pdata->dynamic_pui_end ||
            table_index <= konquest_pdata->dynamic_pui_begin) {
            valid = 0;
        } else {
            valid = 1;
        }
    }
    if (valid == 0) {
        return 0;
    }
    if (item == 0) {
        return 0;
    }
    if (is_it_safe_to_spawn_pui(item) == 0) {
        return 0;
    }

    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index < 0) {
        return 0;
    }

    if (aproc->pid == 0xA021) {
        dynamic = (KonquestDynamicPuiPdata*)pdata_of_proc(aproc);
    }
    x = item->position.x;
    y = item->position.y;
    z = item->position.z;
    if (((dynamic != 0 && dynamic->source_type == 3) ||
         position_mode == 0) &&
        aproc->pid == 0xA021) {
        KonquestDynamicPuiPdata* position_data;

        position_data = (KonquestDynamicPuiPdata*)pdata_of_proc(aproc);
        switch (position_data->source_type) {
        case 1:
            if (position_data->trigger_source != 0) {
                x = position_data->trigger_source->owned_data->position.x;
                y = position_data->trigger_source->owned_data->position.y;
                z = position_data->trigger_source->owned_data->position.z;
            }
            break;
        case 2:
            if (position_data->npc_source != 0) {
                x = position_data->npc_source->fields.data->position.x;
                y = position_data->npc_source->fields.data->position.y;
                z = position_data->npc_source->fields.data->position.z;
            }
            break;
        case 3:
            x = position_data->position.x;
            y = position_data->position.y;
            z = position_data->position.z;
            break;
        }
    }

    pui = (KonquestPuiRuntime*)get_mkhdr(
        &vtbl_konquest_pui, sizeof(*pui));
    if (pui != 0) {
        zero_pdata_payload(sizeof(*pui), pui);
    }
    if (pui == 0) {
        return 0;
    }

    pickup_script = item->pickup_script_index;
    radius = item->radius;
    trigger_id = get_table_index_by_pointer(
        konquest_pdata->script_owner, item);
    trigger = (KonquestTriggerStruct*)get_mkhdr(
        &vtbl_trigger_struct, sizeof(*trigger));
    if (trigger != 0) {
        zero_pdata_payload(sizeof(*trigger), trigger);
    }
    trigger->owned_data =
        (KonquestTriggerDefinition*)get_mem(sizeof(*trigger->owned_data));
    trigger->id = trigger_id;
    trigger->owned_data->type = 2;
    trigger->owned_data->position.x = x;
    trigger->owned_data->position.y = y;
    trigger->owned_data->position.z = z;
    if (trigger->owned_data->position.x >= 1000.0f) {
        tile_index =
            konquest_pdata->tile_width * konquest_pdata->tile_height;
    } else {
        int row;
        int column;

        row = (int)((trigger->owned_data->position.z +
                     konquest_pdata->tile_origin_z) /
                    60.0f);
        if (row < 0) {
            tile_index = -1;
        } else if (row >= konquest_pdata->tile_height) {
            tile_index = -1;
        } else {
            column = (int)((trigger->owned_data->position.x +
                            konquest_pdata->tile_origin_x) /
                           60.0f);
            if (column < 0) {
                tile_index = -1;
            } else if (column >= konquest_pdata->tile_width) {
                tile_index = -1;
            } else {
                tile_index = column + row * konquest_pdata->tile_width;
            }
        }
    }
    trigger->tile_index = tile_index;
    trigger->owned_data->radius = radius;
    trigger->owned_data->state = 1;
    trigger->owned_data->flags = 1;
    trigger->owned_data->script_index = pickup_script;
    trigger->owned_data->pui = 0;
    trigger->object = 0;
    trigger->script_proc = 0;
    trigger->script_proc_instance = 0;
    trigger->flag_bits.bit7 = 0;
    trigger->flag_bits.pressed = 0;
    trigger->flag_bits.bit5 = 0;
    trigger->flag_bits.bit1 = 0;
    trigger->flag_bits.bit4 = 1;
    trigger->flag_bits.bit3 = 0;
    trigger->flag_bits.bit2 = 0;
    mk_insert(&trigger->hdr, &konquest_pdata->triggers);

    if (trigger == 0) {
        if (pui->hdr.instance != 0) {
            pui->hdr.typed_vtbl->destroy(&pui->hdr);
        }
        return 0;
    }

    object = load_named_model_from_slot(
        0x60025, item->model_name, 0xA00D, 0);
    if (object == 0) {
        if (pui->hdr.instance != 0) {
            pui->hdr.typed_vtbl->destroy(&pui->hdr);
        }
        if (trigger->hdr.instance != 0) {
            trigger->hdr.typed_vtbl->destroy(&trigger->hdr);
        }
        return 0;
    }

    obj_set_all_sobjs_priority(object, 0x13);
    obj_create_sobjs(object);
    trigger->owned_data->field_20 = 4;
    trigger->owned_data->pui = pui;
    object->pos.value.x = trigger->owned_data->position.x;
    object->pos.value.y = trigger->owned_data->position.y;
    object->pos.value.z = trigger->owned_data->position.z;
    object->ang.y = item->angle_y;

    pui->trigger_owner = trigger;
    pui->owner_instance = trigger->hdr.instance;
    pui->render_object = object;
    pui->render_object_instance = object->hdr.instance;
    pui->table_name = get_name_of_table_by_pointer(
        konquest_pdata->script_owner, item);
    pui->item = item;
    pui->inventory_index = inventory_index;
    pui->behavior = behavior;
    pui->flags_word = 0;
    valid = 0;
    if (item->position.x >= 1000.0f) {
        valid = 1;
    }
    pui->flag_bits.bit2 = valid;
    pui->base_y = item->position.y;
    pui->alpha = 0;
    pui->drop_timer = 0.0f;
    pui->spawn_delay = 0.0f;
    pui->fade_delay = 0.0f;
    pui->bounce_count = 0;
    pui->effect_60 = 0;
    pui->effect_64 = 0;
    pui->effect_clone = 0;
    pui->attached_effect = 0;
    if (dynamic != 0) {
        pui->flag_bits.saved = dynamic->critical;
    }

    setup_konquest_pui(pui);
    mk_insert(&pui->hdr, &konquest_pdata->pui_list);

    if (item == 0) {
        inventory_index = -1;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        first = konquest_pdata->pui_begin;
        if (table_index <= first || table_index >= konquest_pdata->pui_end) {
            inventory_index = -1;
        } else {
            inventory_index = table_index - (first + 1);
        }
    }
    if (inventory_index >= 0) {
        set_u8_bit(
            konquest_pdata->pui_inventory_bits,
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index, 1);
    }
    if (item->type != 0) {
        if (item == 0) {
            inventory_index = -1;
        } else {
            table_index = get_table_index_by_pointer(
                konquest_pdata->script_owner, item);
            first = konquest_pdata->pui_begin;
            if (table_index <= first || table_index >= konquest_pdata->pui_end) {
                inventory_index = -1;
            } else {
                inventory_index = table_index - (first + 1);
            }
        }
        if (inventory_index >= 0) {
            set_u8_bit(
                &p1_profile_konquest->raw[0x291],
                (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
                inventory_index, 0);
        }
    }
    return pui;
}

/*
 * Soft ceiling: konquest_pui_check_for_and_replace_old_chest 94.71% (672
 * versus 696 bytes). The stale-list cleanup, radius test, trigger transfer,
 * inventory/chest restoration, update, and destruction call sequence agree;
 * residue is latch/register scheduling and save/restore emission.
 */
static int konquest_pui_check_for_and_replace_old_chest(
    KonquestPuiDelayView* new_pui) {
    MkPtr* link;
    int replaced;

    replaced = 0;
    if (konquest_pdata->pui_list != 0) {
        link = konquest_pdata->pui_list;
        while (link != 0) {
            KonquestPuiDelayView* old_pui;

            old_pui = (KonquestPuiDelayView*)link->hdr;
            if (link->instance != old_pui->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (old_pui != 0 && old_pui->item->type == 0 &&
                old_pui != new_pui) {
                float delta_y;
                float delta_x;
                float delta_z;
                float distance_squared;

                delta_y = old_pui->item->position.y -
                          new_pui->item->position.y;
                delta_x = old_pui->item->position.x -
                          new_pui->item->position.x;
                delta_z = old_pui->item->position.z -
                          new_pui->item->position.z;
                distance_squared = delta_z * delta_z +
                    (delta_x * delta_x + delta_y * delta_y);
                if (distance_squared <= 0.25f) {
                    if (replaced == 0) {
                        KonquestTriggerStruct* old_trigger;
                        KonquestTriggerStruct* new_trigger;
                        MkObj* render_object;
                        int inventory_index;

                        old_trigger = old_pui->trigger_owner;
                        if (old_trigger != 0) {
                            if (old_trigger->hdr.instance !=
                                old_pui->owner_instance) {
                                old_trigger = 0;
                            }
                        } else {
                            old_trigger = 0;
                        }
                        new_trigger = new_pui->trigger_owner;
                        if (new_trigger != 0) {
                            if (new_trigger->hdr.instance !=
                                new_pui->owner_instance) {
                                new_trigger = 0;
                            }
                        } else {
                            new_trigger = 0;
                        }
                        render_object = new_pui->render_object;
                        if (render_object != 0) {
                            if (render_object->hdr.instance !=
                                new_pui->render_object_instance) {
                                render_object = 0;
                            }
                        } else {
                            render_object = 0;
                        }
                        if (old_trigger->flag_bits.bit2 != 0) {
                            new_pui->alpha = 0xFF;
                            if (new_trigger != 0 &&
                                new_trigger->flag_bits.bit2 == 0) {
                                handle_trigger_preprocess(new_trigger);
                                mk_insert(
                                    &new_trigger->hdr,
                                    &konquest_pdata->temporary_triggers);
                                new_trigger->flag_bits.bit2 = 1;
                            }
                        }
                        if (old_pui->item == 0) {
                            inventory_index = -1;
                        } else {
                            unsigned int table_index;
                            unsigned int first;

                            table_index = get_table_index_by_pointer(
                                konquest_pdata->script_owner,
                                old_pui->item);
                            first = konquest_pdata->pui_begin;
                            if (table_index <= first ||
                                table_index >= konquest_pdata->pui_end) {
                                inventory_index = -1;
                            } else {
                                inventory_index = table_index - (first + 1);
                            }
                        }
                        if (inventory_index >= 0 &&
                            get_u8_bit(
                                &p1_profile_konquest->raw[0x291],
                                (konquest_pdata->pui_end -
                                 konquest_pdata->pui_begin) - 1,
                                inventory_index) != 0) {
                            MkSobj* chest;

                            chest = obj_find_sobj_by_id(render_object, 10);
                            chest->flags_08_bits.bit3 = 1;
                            chest->ang.x = 3.926991f;
                            pui_set_chest_state(new_pui->item, 1);
                        }
                        update_mkobj(
                            render_object != 0
                                ? as_mkhdr(&render_object->hdr)
                                : 0);
                        replaced = 1;
                    }
                    if (old_pui->hdr.instance != 0) {
                        old_pui->hdr.typed_vtbl->destroy(&old_pui->hdr);
                    }
                }
            }
            link = link->next;
        }
    }
    return replaced;
}

/*
 * Soft ceiling: pui_set_chest_state ~87.3%. Lookup, stale-link cleanup, both
 * instance latches, calls, and process-data stores match. A three-GPR
 * carousel, two equivalent shorter latch joins, and expanded saves/restores
 * produce a net 16 bytes over retail.
 */
static MkProc* pui_set_chest_state(
    KonquestPuiDefinition* item, int direction) {
    KonquestPuiDelayView* pui;
    MkObj* render_object;
    MkSobj* chest;
    KonquestChestPdata* pdata;
    MkProc* proc;

    pui = find_pui_runtime_by_id(item);
    if (pui == 0) {
        return 0;
    }

    render_object = pui->render_object;
    if (render_object != 0) {
        if (render_object->hdr.instance != pui->render_object_instance) {
            render_object = 0;
        }
    } else {
        render_object = 0;
    }
    if (render_object == 0) {
        return 0;
    }

    chest = obj_find_sobj_by_id(render_object, 10);
    if (chest == 0) {
        return 0;
    }

    proc = (MkProc*)_create_mkproc_generic_tinystack(
        0xA022, 0x1F, p_close_konquest_chest, 0x18, (void**)&pdata);
    if (proc != 0) {
        KonquestChestOwner* owner;

        owner = pui->owner;
        if (owner != 0) {
            if (owner->hdr.instance != pui->owner_instance) {
                owner = 0;
            }
        } else {
            owner = 0;
        }
        owner->interaction->closed = 0;

        pdata->owner = owner;
        pdata->owner_instance = owner->hdr.instance;
        pdata->animation = (KonquestChestAnimation*)chest;
        pdata->direction = direction;
    }
    return proc;
}

/*
 * Soft ceiling: pui_restore_open_chests ~87.2% -- the PUI lookup, stale-link
 * cleanup, object-instance latch, subobject update, and access widths agree.
 * Residue is paired-save emission, one folded latch join, and the local float
 * constant's relocation label.
 */
static void pui_restore_open_chests(KonquestPuiDefinition* item) {
    KonquestPuiDelayView* pui;
    MkObj* render_object;
    MkSobj* chest;

    pui = find_pui_runtime_by_id(item);
    if (pui != 0) {
        render_object = pui->render_object;
        if (render_object != 0) {
            if (render_object->hdr.instance != pui->render_object_instance) {
                render_object = 0;
            }
        } else {
            render_object = 0;
        }
        if (render_object != 0) {
            chest = obj_find_sobj_by_id(render_object, 10);
            if (chest != 0) {
                chest->flags_08_bits.bit3 = 1;
                chest->ang.x = 3.926991f;
            }
        }
    }
}

static float p_close_konquest_chest(void) {
    KonquestChestPdata* pdata;
    KonquestChestOwner* owner;
    KonquestChestAnimation* animation;

    pdata = (KonquestChestPdata*)pdata_of_proc(aproc);
    owner = pdata->owner;
    animation = pdata->animation;
    if (owner != 0) {
        if (owner->hdr.instance != pdata->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }

    if (animation != 0) {
        animation->flags |= 8;
        switch (pdata->direction) {
        case 1:
            snd_req(0x159A);
            animation->angle = 3.926991f;
            while (animation->angle <= 6.2231855f) {
                animation->angle += 0.03f;
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
            }
            animation->angle = 0.0f;
            animation->angular_velocity = 0.0f;
            owner->interaction->closed = 1;
            break;
        case 0:
            snd_req(0x159A);
            animation->angle = 6.2831855f;
            while (animation->angle > 3.926991f) {
                animation->angle -= 0.03f;
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
            }
            animation->angle = 3.926991f;
            animation->angular_velocity = 0.0f;
            break;
        }
    }
    return -1.0f;
}

int spawn_dynamic_pui_critical(KonquestPuiDefinition* item) {
    MkObjLatch* pdata;
    MkHdr* source;
    int pid;

    pid = aproc->pid;
    if (pid == 0x9019) {
        pdata = (MkObjLatch*)pdata_of_proc(aproc);
        source = pdata->obj;
        if (source != 0) {
            if (source->instance == pdata->obj_instance) {
                /* Valid process source latch. */
            } else {
                source = 0;
            }
        } else {
            source = 0;
        }
        if (source == 0) {
            return 0;
        }
        if (spawn_dynamic_pui_at_pos(
                item, 1, 0, source, 1) != 0) {
            return 1;
        }
    } else if (pid == 0xA002 || pid == 0xA014) {
        if (g_active_npc == 0) {
            return 0;
        }
        if (spawn_dynamic_pui_at_pos(
                item, 2, 0, g_active_npc, 1) != 0) {
            return 1;
        }
    }
    return 0;
}

int spawn_dynamic_pui(KonquestPuiDefinition* item) {
    MkObjLatch* pdata;
    MkHdr* source;
    int pid;

    pid = aproc->pid;
    if (pid == 0x9019) {
        pdata = (MkObjLatch*)pdata_of_proc(aproc);
        source = pdata->obj;
        if (source != 0) {
            if (source->instance == pdata->obj_instance) {
                /* Valid process source latch. */
            } else {
                source = 0;
            }
        } else {
            source = 0;
        }
        if (source == 0) {
            return 0;
        }
        if (spawn_dynamic_pui_at_pos(
                item, 1, 0, source, 0) != 0) {
            return 1;
        }
    } else if (pid == 0xA002 || pid == 0xA014) {
        if (g_active_npc == 0) {
            return 0;
        }
        if (spawn_dynamic_pui_at_pos(
                item, 2, 0, g_active_npc, 0) != 0) {
            return 1;
        }
    }
    return 0;
}

int spawn_dynamic_pui_at_pos(
    KonquestPuiDefinition* item, int source_type, const Vec* position,
    void* source, int critical) {
    unsigned int table_index;
    int valid;
    MkProc* proc;
    KonquestDynamicPuiPdata* pdata;

    if (item == 0) {
        valid = 0;
    } else {
        table_index =
            get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (table_index >= konquest_pdata->dynamic_pui_end ||
            table_index <= konquest_pdata->dynamic_pui_begin) {
            valid = 0;
        } else {
            valid = 1;
        }
    }
    if (valid == 0) {
        return 0;
    }
    if (is_it_safe_to_spawn_pui(item) == 0) {
        return 0;
    }

    proc = _create_mkproc_generic_nostack(
        0xA021, 0x1F, p_spawn_dynamic_pui, sizeof(*pdata),
        (void**)&pdata);
    if (proc != 0) {
        set_process_as_scriptable(proc);
        pdata->item = item;
        pdata->critical = critical;
        pdata->source_type = source_type;
        if (source_type == 3) {
            if (position == 0) {
                if (proc->instance != 0) {
                    ((KonquestDestroyable*)proc)->vtbl->destroy(
                        (KonquestDestroyable*)proc);
                }
                return 0;
            }
            pdata->position.x = position->x;
            pdata->position.y = position->y;
            pdata->position.z = position->z;
        } else {
            if (source == 0) {
                if (proc->instance != 0) {
                    ((KonquestDestroyable*)proc)->vtbl->destroy(
                        (KonquestDestroyable*)proc);
                }
                return 0;
            }
            pdata->source = source;
        }
        return 1;
    }
    return 0;
}

int should_this_pui_be_saved(const struct KonquestPuiRuntime* pui) {
    if (pui != 0 && pui->flag_bits.saved) {
        return 1;
    }
    return 0;
}

static void p_spawn_dynamic_pui(void) {
    KonquestDynamicPuiPdata* pdata;

    pdata = (KonquestDynamicPuiPdata*)pdata_of_proc(aproc);
    cmdscript_set_parameters(
        active_cmdscript, 2, pdata->item, pdata->source_type);
    cmdscript_setup_execution(
        konquest_pdata->script_owner,
        pdata->item->spawn_script_index);
    cmdscript_execute(konquest_pdata->script_owner);
}

void kill_dynamic_pui(KonquestPuiDefinition* item) {
    unsigned int table_index;
    unsigned int first;
    int inventory_index;
    int valid;
    int owned;

    if (item == 0) {
        valid = 0;
    } else {
        table_index =
            get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (table_index >= konquest_pdata->dynamic_pui_end ||
            table_index <= konquest_pdata->dynamic_pui_begin) {
            valid = 0;
        } else {
            valid = 1;
        }
    }
    if (valid != 0) {
        if (item == 0) {
            inventory_index = -1;
        } else {
            table_index =
                get_table_index_by_pointer(konquest_pdata->script_owner, item);
            first = konquest_pdata->pui_begin;
            if (table_index <= first || table_index >= konquest_pdata->pui_end) {
                inventory_index = -1;
            } else {
                inventory_index = table_index - (first + 1);
            }
        }
        if (inventory_index >= 0) {
            if (item == 0) {
                inventory_index = -1;
            } else {
                table_index = get_table_index_by_pointer(
                    konquest_pdata->script_owner, item);
                first = konquest_pdata->pui_begin;
                if (table_index <= first ||
                    table_index >= konquest_pdata->pui_end) {
                    inventory_index = -1;
                } else {
                    inventory_index = table_index - (first + 1);
                }
            }
            if (inventory_index >= 0) {
                owned = get_u8_bit(
                    konquest_pdata->pui_inventory_bits,
                    (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
                    inventory_index);
            } else {
                owned = 0;
            }
            if (owned != 0) {
                cmdscript_set_parameters(active_cmdscript, 1, item);
                cmdscript_setup_execution(
                    konquest_pdata->script_owner, item->kill_script_index);
                cmdscript_execute(konquest_pdata->script_owner);
            }
        }
    }
}
static inline int find_pui_inventory_index(void* pui) {
    unsigned int table_index;

    if (pui == 0) {
        return -1;
    }
    table_index =
        get_table_index_by_pointer(konquest_pdata->script_owner, pui);
    if (table_index <= konquest_pdata->pui_begin ||
        table_index >= konquest_pdata->pui_end) {
        return -1;
    }
    return table_index - (konquest_pdata->pui_begin + 1);
}


/*
 * Soft ceiling: kill_pui 96.33% (736 versus 720 bytes). Its seven remaining
 * differences are exclusively stmw/lmw versus individual nonvolatile saves
 * and restores; the dynamic guard, bit operations, and destruction are exact.
 */
void kill_pui(KonquestPuiDefinition* item) {
    KonquestPuiDelayView* pui;
    unsigned int table_index;
    int inventory_index;
    int valid;
    int active;

    if (item == 0) {
        valid = 0;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        if (table_index >= konquest_pdata->dynamic_pui_end ||
            table_index <= konquest_pdata->dynamic_pui_begin) {
            valid = 0;
        } else {
            valid = 1;
        }
    }
    if (valid != 0) {
        inventory_index = find_pui_inventory_index(item);
        if (inventory_index >= 0) {
            inventory_index = find_pui_inventory_index(item);
            if (inventory_index >= 0) {
                active = get_u8_bit(
                    konquest_pdata->pui_inventory_bits,
                    (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
                    inventory_index);
            } else {
                active = 0;
            }
            if (active != 0) {
                pui = find_pui_runtime_by_id(item);
                if (pui != 0 && pui->hdr.instance != 0) {
                    pui->hdr.typed_vtbl->destroy(&pui->hdr);
                }
                inventory_index = find_pui_inventory_index(item);
                if (inventory_index >= 0) {
                    set_u8_bit(
                        konquest_pdata->pui_inventory_bits,
                        (konquest_pdata->pui_end -
                         konquest_pdata->pui_begin) - 1,
                        inventory_index, 0);
                }
                if (item->clear_inventory_for_unique_event != 0) {
                    inventory_index = find_pui_inventory_index(item);
                    if (inventory_index >= 0) {
                        set_u8_bit(
                            &p1_profile_konquest->raw[0x291],
                            (konquest_pdata->pui_end -
                             konquest_pdata->pui_begin) - 1,
                            inventory_index, 0);
                    }
                }
            }
        }
    }
}

int is_pui_an_interior_item(const char* pui) {
    return *(const float*)(pui + 0x1c) >= 1000.0f;
}

static inline KonquestTriggerStruct* find_trigger_by_id_inline(
    unsigned int id) {
    MkPtr* link;
    MkPtr* next;

    if (konquest_pdata->triggers != 0) {
        link = konquest_pdata->triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (trigger->id == id) {
                    return trigger;
                }
                link = link->next;
            }
        }
    }
    if (konquest_pdata->temporary_triggers != 0) {
        link = konquest_pdata->temporary_triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (trigger->id == id) {
                    return trigger;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

/*
 * Near-exact at retail size: dynamic-table bounds, action-bit semantics,
 * stale-safe two-list trigger lookup, profile clearing, and script dispatch
 * are instruction-identical. Objdiff's remaining annotations are exclusively
 * nonvolatile-register allocation.
 */
static void update_pui_actions(
    KonquestPuiActionBuffer current, KonquestPuiActionBuffer previous) {
    int dynamic_end;
    int table_index;

    dynamic_end = konquest_pdata->dynamic_pui_end;
    table_index = konquest_pdata->dynamic_pui_begin + 1;
    while (table_index < dynamic_end) {
        KonquestPuiDefinition* item;
        unsigned int script_index;
        int inventory_index;

        script_index = 0;
        inventory_index = table_index - konquest_pdata->pui_begin - 1;
        if (get_u8_bit(
                previous,
                (konquest_pdata->pui_end -
                 konquest_pdata->pui_begin) - 1,
                inventory_index) != 0) {
            item = (KonquestPuiDefinition*)get_data_table(
                konquest_pdata->script_owner, table_index);
            if (get_u8_bit(
                    current,
                    (konquest_pdata->pui_end -
                     konquest_pdata->pui_begin) - 1,
                    inventory_index) != 0) {
                if (is_it_safe_to_spawn_pui(item) != 0) {
                    script_index = item->spawn_script_index;
                }
            } else {
                int active;

                inventory_index = find_pui_inventory_index(item);
                if (inventory_index >= 0) {
                    active = get_u8_bit(
                        konquest_pdata->pui_inventory_bits,
                        (konquest_pdata->pui_end -
                         konquest_pdata->pui_begin) - 1,
                        inventory_index);
                } else {
                    active = 0;
                }
                if (active != 0) {
                    KonquestTriggerStruct* trigger;
                    unsigned int trigger_id;

                    trigger_id = get_table_index_by_pointer(
                        konquest_pdata->script_owner, item);
                    trigger = find_trigger_by_id_inline(trigger_id);
                    if (trigger != 0 && trigger->flag_bits.bit3 == 0) {
                        script_index = item->kill_script_index;
                    }
                } else if (
                    item->clear_inventory_for_unique_event != 0) {
                    inventory_index = find_pui_inventory_index(item);
                    if (inventory_index >= 0) {
                        set_u8_bit(
                            &p1_profile_konquest->raw[0x291],
                            (konquest_pdata->pui_end -
                             konquest_pdata->pui_begin) - 1,
                            inventory_index, 0);
                    }
                }
            }
            if (script_index != 0) {
                cmdscript_set_parameters(active_cmdscript, 2, item, 0);
                cmdscript_setup_execution(
                    konquest_pdata->script_owner, script_index);
                cmdscript_execute(konquest_pdata->script_owner);
            }
        }
        table_index++;
    }
}

/*
 * Near match at retail size: the dynamic-range, inventory, active-spawn, and
 * unique-event gates agree. MWCC only lays out the equivalent final false/true
 * return blocks differently, retaining two extra branches in retail.
 */
static int is_it_safe_to_spawn_pui(KonquestPuiDefinition* item) {
    unsigned int table_index;
    int inventory_index;
    int valid;
    int active;

    if (item == 0) {
        valid = 0;
    } else {
        table_index = get_table_index_by_pointer(
            konquest_pdata->script_owner, item);
        if (table_index >= konquest_pdata->dynamic_pui_end ||
            table_index <= konquest_pdata->dynamic_pui_begin) {
            valid = 0;
        } else {
            valid = 1;
        }
    }
    if (valid == 0) {
        return 0;
    }

    inventory_index = find_pui_inventory_index(item);
    if (inventory_index < 0) {
        return 0;
    }

    inventory_index = find_pui_inventory_index(item);
    if (inventory_index >= 0) {
        active = get_u8_bit(
            konquest_pdata->pui_inventory_bits,
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index);
    } else {
        active = 0;
    }
    if (active != 0) {
        return 0;
    }
    if (item->type == 0) {
        return 1;
    }

    inventory_index = find_pui_inventory_index(item);
    if (inventory_index >= 0) {
        active = get_u8_bit(
            &p1_profile_konquest->raw[0x291],
            (konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1,
            inventory_index);
    } else {
        active = 0;
    }
    if (active != 0 && item->clear_inventory_for_unique_event == 0) {
        return 0;
    }
    return 1;
}

static void scan_pui_events_and_determine_current_actions(
    KonquestPuiEventRow* event_table, KonquestPuiActionBuffer current,
    KonquestPuiActionBuffer previous) {
    unsigned int row_count;
    unsigned int index;

    row_count = get_row_count_for_table_by_pointer(
        konquest_pdata->script_owner, event_table);
    index = 0;
    while (index < row_count) {
        int action;
        int inventory_index;

        action = pui_get_next_action(
            event_table[index].pui, event_table, &index);
        if (action != -1) {
            inventory_index =
                find_pui_inventory_index(event_table[index].pui);
            if (inventory_index >= 0) {
                int bit_count;

                bit_count =
                    konquest_pdata->pui_end - konquest_pdata->pui_begin - 1;
                set_u8_bit(previous, bit_count, inventory_index, 1);
                if (action == 1) {
                    bit_count = konquest_pdata->pui_end -
                                konquest_pdata->pui_begin - 1;
                    set_u8_bit(current, bit_count, inventory_index, 1);
                }
            }
        }
        index++;
    }
}

static int pui_get_next_action(
    KonquestPuiDefinition* pui, KonquestPuiEventRow* event_table,
    unsigned int* index) {
    KonquestPuiEventRow* selected;
    KonquestPuiEventRow* event;
    KonquestTime* current_time;
    unsigned int row_count;
    int matching_count;
    int found;

    selected = 0;
    event = &event_table[*index];
    row_count = get_row_count_for_table_by_pointer(
        konquest_pdata->script_owner, event_table);
    matching_count = 0;
    found = 0;
    current_time = &konquest_pdata->current_time;
    while (*index < row_count) {
        if (event->pui != pui) {
            break;
        }
        matching_count++;
        if (is_valid_event_time(&event->time) != 0 &&
            is_time_a_greater_than_time_b(
                current_time,
                &g_pui_events[*index]) != 0) {
            calc_next_occurrence_of_event(
                &g_pui_events[*index], &event->time,
                current_time);
            found = 1;
            selected = event;
        }
        (*index)++;
        event++;
    }
    (*index)--;

    if (found != 0 && matching_count == 1 &&
        pui->clear_inventory_for_unique_event != 0 &&
        find_pui_inventory_index(pui) >= 0) {
        int inventory_index;
        int bit_count;

        inventory_index = find_pui_inventory_index(pui);
        if (inventory_index >= 0) {
            bit_count =
                konquest_pdata->pui_end - konquest_pdata->pui_begin - 1;
            set_u8_bit(
                &p1_profile_konquest->raw[0x291], bit_count,
                inventory_index, 0);
        }
    }
    if (selected != 0) {
        return selected->action;
    }
    return -1;
}

int get_pui_inventory_bit_index(void* item) {
    unsigned int table_index;
    unsigned int first;

    if (item == 0) {
        return -1;
    }
    table_index =
        get_table_index_by_pointer(konquest_pdata->script_owner, item);
    first = konquest_pdata->pui_begin;
    if (table_index <= first || table_index >= konquest_pdata->pui_end) {
        return -1;
    }
    return table_index - (first + 1);
}

/*
 * Soft ceiling: p_pui_timed_event_manager ~97.7% -- code and data contents
 * are exact; objdiff differences are local labels for the two zeroed
 * aggregates and the -1.0f/30.0f constants.
 */
static float p_pui_timed_event_manager(void) {
    KonquestPuiActionBuffer current = {0};
    KonquestPuiActionBuffer previous = {0};
    void* event_table;

    event_table = konquest_pdata->region_table->pui_event_table;
    if (event_table == 0) {
        return -1.0f;
    }
    scan_pui_events_and_determine_current_actions(
        event_table, current, previous);
    update_pui_actions(current, previous);
    return 30.0f;
}

int get_num_puis(void) {
    return (int)(konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1;
}

void start_subobject_pulsing_effect(int pulse_type) {
    MkHdr* object;

    object = konquest_pdata->tile_objects;
    if (object != 0) {
        if (object->instance ==
            konquest_pdata->tile_objects_instance) {
            /* Valid object latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        pulsate_object(object, pulse_type, 0x1E, 0xA, 6.0f, 6.0f);
    }
}

void resume_hero_state_process(void) {
    MkProc* proc;
    KonquestGrounding* grounding;

    if (konquest_pdata->hero_anim != 0) {
        proc = konquest_pdata->hero_anim->proc;
        if (proc != 0) {
            if (proc->instance ==
                konquest_pdata->hero_anim->proc_instance) {
                /* Valid process latch. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_control_konquest_monk);
    }
    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance ==
            konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
    }
}

void suspend_hero_state_process(void) {
    MkProc* proc;

    if (konquest_pdata->hero_anim != 0) {
        proc = konquest_pdata->hero_anim->proc;
        if (proc != 0) {
            if (proc->instance ==
                konquest_pdata->hero_anim->proc_instance) {
                /* Valid process latch. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        xfer_proc(proc, p_animate);
    }
}

void idle_hero_anim_proc(void) {
    MkProc* proc;
    AnimPdata* pdata;

    if (konquest_pdata->hero_anim != 0) {
        proc = konquest_pdata->hero_anim->proc;
        if (proc != 0) {
            if (proc->instance ==
                konquest_pdata->hero_anim->proc_instance) {
                /* Valid animation process latch. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        if (proc != 0) {
            xfer_proc(proc, p_anim_idle);
            pdata = (AnimPdata*)pdata_of_proc(proc);
            if (pdata != 0) {
                pdata->step = 1.0f;
                set_root_and_obj_movement_weights(0.0f, 1.0f, pdata);
            }
        }
    }
}

void transition_hero_to_anim_script(
    int script_id, int transition, float blend, float step) {
    transition_to_anim_script(
        konquest_pdata->hero_anim, get_animation(), transition, blend);
    konquest_pdata->hero_anim->step = step;
}

void start_hero_collisions(void) {
    old_hero_position.y = 0.0f;
    old_hero_position.z = 0.0f;
    old_hero_position.x = 0.0f;
    old_hero_position.y = 500.0f;
    xfer_proc(konquest_pdata->collision_proc, p_collide_monk);
}

void stop_hero_collisions(void) {
    xfer_proc(konquest_pdata->collision_proc, p_idle);
}

void cleanup_konquest(void) {
    if (konquest_pdata != 0) {
        stop_sound_tracking_process(&konquest_pdata->attached_sounds);
        konquest_pdata = 0;
    }
    cleanup_npc_manager();
    TearDownShadow((ShadowObject*)&pdata_monk);
    destroy_shadow_system();
    npc_shadow_teardown();
    destroy_konquest_shadow_collision_lists();
    if (g_pui_events != 0) {
        free_mem(g_pui_events);
        g_pui_events = 0;
    }
    if (mode_of_play != 7 && mode_of_play != 8) {
        memset(&konquest_save_data, 0, sizeof(konquest_save_data));
    }
    cleanup_mission_state();
    f_writing_konquest_profile = 0;
}

/*
 * Near match: p_head_tracking 93.98% (724 versus 712 bytes). Hero/camera
 * validation, state turns, angle wrapping and clamping, aligned head-matrix
 * transform, and all three calls agree. Residue is two equivalent latch joins,
 * individual versus combined GPR saves, and pooled-float relocation labels.
 */
static float p_head_tracking(void) {
    KonquestHeadTrackingPdata* pdata;
    CameraObj* camera;
    MkObj* hero;
    RwMatrix* head_matrix;
    MKMATRIX rotation __attribute__((aligned(16)));
    Vec position;
    float angle;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }

    pdata = (KonquestHeadTrackingPdata*)pdata_of_proc(aproc);
    if (hero == 0) {
        return 1.0f;
    }
    if (camera == 0 || pdata == 0) {
        return -1.0f;
    }

    if (konquest_pdata->npc_interaction_state == 0) {
        if (pdata->angle_y > 0.08f) {
            angle = pdata->angle_y - 0.08f;
        } else if (pdata->angle_y < -0.08f) {
            angle = 0.08f + pdata->angle_y;
        } else {
            return 1.0f;
        }
    } else {
        int state;

        angle = camera->ang.y - hero->ang.y;
        if (angle < -3.1415927f) {
            angle += 6.2831855f;
        } else if (angle > 3.1415927f) {
            angle -= 6.2831855f;
        }

        if (angle > 1.0471976f) {
            state = konquest_pdata->hero_state;
            if (state == 0 || state == 7) {
                if (state != 4 &&
                    monk_state_data[4].transition_order >=
                        monk_state_data[state].transition_order) {
                    konquest_pdata->hero_state = 4;
                }
            }
            angle -= 1.5707964f;
        } else if (angle < -1.0471976f) {
            state = konquest_pdata->hero_state;
            if (state == 0 || state == 5) {
                if (state != 6 &&
                    monk_state_data[6].transition_order >=
                        monk_state_data[state].transition_order) {
                    konquest_pdata->hero_state = 6;
                }
            }
            angle += 1.5707964f;
        } else {
            konquest_pdata->hero_state = 0;
        }

        if (angle - pdata->angle_y > 0.08f) {
            angle = 0.08f + pdata->angle_y;
        } else if (angle - pdata->angle_y < -0.08f) {
            angle = pdata->angle_y - 0.08f;
        }
    }

    head_matrix = hero->bones[10]->parent_matrix;
    position.x = head_matrix->pos.x;
    position.y = head_matrix->pos.y;
    position.z = head_matrix->pos.z;
    head_matrix->pos.x = head_matrix->pos.y = head_matrix->pos.z = 0.0f;
    y_angle_to_MKMATRIX(&rotation, angle);
    RwMatrixTransform(head_matrix, &rotation, 2);
    head_matrix->pos.x = position.x;
    head_matrix->pos.y = position.y;
    head_matrix->pos.z = position.z;
    pdata->angle_y = angle;

    return 1.0f;
}

/*
 * Soft ceiling: body and stack buffer match retail exactly. The remaining
 * 16-byte delta is three individual nonvolatile-GPR saves/restores instead of
 * retail's stmw/lmw pair.
 */
AniTextureControl* konquest_create_monk_face_ani_texture(MkObj* object) {
    char texture_name[0x40];
    char* art_name;
    int slot;
    AniTextureControl* texture;

    slot = 0xB002A;
    texture = 0;
    if (mode_of_play == 8) {
        slot = 0x2001E;
    }

    if (konquest_editor_mode_on == 0) {
        sprintf(
            texture_name, "KON_HERO_0%d_MOUTH",
            p1_profile_konquest->fields.hero_age + 1);
        art_name = (char*)get_artid_of_named_item_in_slot(
            slot, texture_name, 0);
        if (art_name != 0) {
            texture = append_wiff_to_clump_material_id(
                slot, art_name, object->clump, 1);
        }
    }
    return texture;
}

/*
 * Near match: the NIS stack wait, process transfers, hero-process latch, and
 * cleanup agree. Residue is one inlined array-address strength reduction, an
 * equivalent latch branch, and the local 1.0f relocation label.
 */
void konquest_nis_end(void) {
    KonquestPdata* pdata;
    AnimPdata* animation;
    MkProc* proc;
    int mode_index;

    while (konquest_current_game_mode() != 3) {
        if (konquest_game_mode_in_stack(3) == 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    pdata = konquest_pdata;
    mode_index = pdata->game_mode_index;
    if (mode_index != 0) {
        pdata->game_mode_index--;
    }
    konquest_pdata->hero_state = 0;
    xfer_proc(konquest_pdata->region_load_proc, p_konquest_loop);
    xfer_proc(konquest_pdata->collision_proc, p_collide_monk);

    animation = konquest_pdata->hero_anim;
    proc = animation->proc;
    if (proc != 0) {
        if (proc->instance == animation->proc_instance) {
            /* Valid animation-process latch. */
        } else {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    xfer_proc(proc, p_control_konquest_monk);
    destroy_mkprocs_pid(0x901C);
    nis_end_scene();
}

/*
 * Near match: all NIS setup, inlined helper behavior, and latch checks agree.
 * Retail retains one extra branch in each of the mode and two latch diamonds;
 * MWCC folds those joins here, producing the 12-byte size difference.
 */
void konquest_nis_init(int value) {
    KonquestGrounding* grounding;

    if (konquest_current_game_mode() != 3 &&
        konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 3;
    }

    camera_obj->flags.pad3 = 0;
    camera_obj->flags.bit04 = 0;
    camera_obj->flags.pad6 = 0;
    camera_obj->flags.bit20 = 0;
    _create_mkproc_generic_tinystack(
        0x901C, 0x1F, p_konquest_nis_housekeeping, 0, 0);

    if (konquest_pdata->collision_proc != 0) {
        stop_hero_collisions();
    }
    xfer_camera(p_idle, 1);
    idle_hero_anim_proc();

    konquest_pdata->npc_interaction_state = 0;
    nis_participants = 0;

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
        grounding->flag_bits.suspended = 0;
    }
}

void nis_wait_for_region_load(void) {
    while (((konquest_pdata->flags >> 7) & 1) == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    xfer_proc(konquest_pdata->region_load_proc, p_idle);
}

void wait_for_region_load(void) {
    while (((konquest_pdata->flags >> 7) & 1) == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
}

static float p_konquest_nis_housekeeping(void) {
    update_tile_grid();
    if (konquest_pdata->region_index == 0 &&
        check_switch_edge(konquest_pdata->input_port, 6) != 0) {
        turn_controllers_off();
        fade_to_black(8, 1);
        xfer_camera(p_idle, 1);
    }
    return 1.0f;
}

/*
 * Soft ceiling: nis_remove_non_participants ~86.6% -- the body and GPRs are
 * exact; residue is paired saves versus stmw/lmw and equivalent pointer
 * truth normalization (neg/or/srwi versus subic/subfe).
 */
void nis_remove_non_participants(void) {
    MkPtr* next;
    MkPtr* link;

    if (konquest_pdata->npcs != 0) {
        link = konquest_pdata->npcs;
        while (link != 0) {
            MkHdr* hdr;
            KonquestNpc* npc;

            hdr = link->hdr;
            if (link->instance != hdr->instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                npc = (KonquestNpc*)hdr;
                if (npc != 0 && !npc->fields.state_flag_bits.nis_participant) {
                    MkSobj* owner;
                    int active;

                    owner = npc->fields.dialog_owner;
                    if (owner == 0) {
                        active = 0;
                    } else if (owner->id_flags == 0) {
                        active = 0;
                    } else {
                        active = owner->bound_hdr != 0;
                    }
                    if (active != 0) {
                        npc_make_invisible(npc);
                    }
                }
                link = link->next;
            }
        }
    }
}
static inline int is_npc_scene_active(KonquestNpcRuntime* npc) {
    KonquestNpcSceneState* scene;
    int active;

    scene = npc->scene_state;
    if (scene == 0) {
        return 0;
    }
    if (scene->object == 0) {
        return 0;
    }
    if (scene->process != 0) {
        active = 1;
    } else {
        active = 0;
    }
    return active;
}


/*
 * Near match: allocation, both participant paths, scene readiness wait, NPC
 * takeover, and resume-entry capture agree. The 12-byte delta is two folded
 * ownership joins plus equivalent pointer-truth normalization; saves differ.
 */
void nis_register_participant(int type, void* npc_data) {
    KonquestNisParticipant* participant;

    participant = (KonquestNisParticipant*)get_mkhdr_generic(
        sizeof(KonquestNisParticipant));
    if (participant == 0) {
        return;
    }

    participant->type = type;
    switch (type) {
    case 0: {
        KonquestNpcRuntime* npc;

        if (konquest_pdata == 0) {
            break;
        }
        npc = (KonquestNpcRuntime*)find_npc_by_data(
            (KonquestNpcData*)npc_data);
        if (npc == 0) {
            break;
        }

        is_this_the_monk_npc((KonquestNpc*)npc);
        npc->state_flag_bits.nis_participant = 1;
        npc->nis_controlled = 1;
        npc_make_visible(npc);
        if (npc->state_flag_bits.bit4) {
            npc->state_flag_bits.bit4 = 0;
        }

        while (is_npc_scene_active(npc) == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        }

        if (npc->type == 7) {
            void* script_function;

            script_function = get_script_function_by_name(
                konquest_pdata->script_owner,
                "stand_around_still_no_travel");
            npc_take_control_of_him(
                npc->data, script_function);
        }
        participant->npc = npc;
        participant->resume_entry = npc->scene_state->process->entry;
        mk_insert(&participant->hdr, &nis_participants);
        break;
    }
    case 1: {
        MkObj* hero;
        KonquestNpcRuntime* npc;

        if (konquest_pdata != 0) {
            hero = konquest_pdata->hero_object;
            if (hero != 0) {
                if (hero->hdr.instance == konquest_pdata->hero_instance) {
                    /* Valid hero-object latch. */
                } else {
                    hero = 0;
                }
            } else {
                hero = 0;
            }
            if (hero != 0) {
                npc = (KonquestNpcRuntime*)konquest_pdata->hero_grounding;
                if (npc != 0) {
                    if (npc->hdr.instance ==
                        konquest_pdata->grounding_instance) {
                        /* Valid hero-NPC latch. */
                    } else {
                        npc = 0;
                    }
                } else {
                    npc = 0;
                }
                if (npc != 0) {
                    npc->state_flag_bits.nis_participant = 1;
                    participant->npc = npc;
                    mk_insert(&participant->hdr, &nis_participants);
                    npc_force_state_for_npc(npc, 0);
                }
            }
        }
        break;
    }
    }
}

void nis_end_scene(void) {
    MkPtr* link;

    link = nis_participants;
    while (link != 0) {
        KonquestNisParticipant* participant;

        participant = (KonquestNisParticipant*)link->hdr;
        if (link->instance != participant->hdr.instance) {
            MkPtr* next;

            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
            continue;
        }

        switch (participant->type) {
        case 0: {
            KonquestNpcRuntime* npc;
            KonquestNpcSceneState* scene;
            MkSobj* first_sobj;

            npc = participant->npc;
            scene = npc->scene_state;
            if (npc->type == 7 &&
                npc->script_function ==
                    get_script_function_by_name(
                        konquest_pdata->script_owner,
                        "stand_around_still_no_travel")) {
                npc_restart_his_normal_behavior(npc->data);
            }
            npc->state_flags &= ~0x20;
            first_sobj = 0;
            if (scene != 0) {
                if (scene->object != 0) {
                    first_sobj = obj_first_sobj(scene->object);
                }
                if (scene->process != 0) {
                    xfer_proc(scene->process, participant->resume_entry);
                }
                if (first_sobj != 0) {
                    first_sobj->flags09 &= ~0x10;
                }
            }
            break;
        }
        case 1:
            npc_xfer(participant->npc, p_npc_idle, 0);
            break;
        }
        link = link->next;
    }
    discard_list(&nis_participants);
}

static KonquestNpc* konquest_check_possible_interact_with_npc(
    float* distance, float* facing_angle) {
    MkObj* hero;
    KonquestNpc* selected;
    MkPtr* link;

    selected = 0;
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero-object latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    *distance = 1000.0f;
    *facing_angle = 6.2831855f;
    if (konquest_pdata->npcs != 0) {
        link = konquest_pdata->npcs;
        while (link != 0) {
            KonquestNpc* npc;

            npc = (KonquestNpc*)link->hdr;
            if (link->instance != npc->fields.hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                KonquestNpcAnimationState* animation;

                animation = npc->fields.animation_state;
                if (npc != 0 && animation != 0 && animation->object != 0) {
                    MkObj* object;
                    float object_distance;
                    float delta_x;
                    float delta_z;
                    float length_squared;
                    KonquestFloatBits estimate;
                    float product;
                    float correction;
                    float inverse_length;
                    float forward_x;
                    float forward_z;
                    float forward_length_squared;
                    KonquestFloatBits forward_estimate;
                    float forward_product;
                    float forward_correction;
                    float inverse_forward_length;
                    float angle;

                    object = animation->object;
                    object_distance = dist_xz_to_xz(
                        &hero->pos.value, &object->pos.value);
                    object = npc->fields.animation_state->object;
                    delta_z = object->pos.value.z - hero->pos.value.z;
                    delta_x = object->pos.value.x - hero->pos.value.x;
                    length_squared =
                        delta_x * delta_x + delta_z * delta_z;
                    if (length_squared <= 0.0f) {
                        inverse_length = 0.0f;
                    } else {
                        estimate.value = length_squared;
                        estimate.bits =
                            0x5F375A00 - (estimate.bits >> 1);
                        product = estimate.value *
                                  (length_squared * estimate.value);
                        correction = 3.0f - product;
                        inverse_length =
                            0.0625f * estimate.value * correction *
                            -(correction * (product * correction) - 12.0f);
                    }
                    delta_x *= inverse_length;
                    delta_z *= inverse_length;

                    forward_x = hero->field_24->at.x;
                    forward_z = hero->field_24->at.z;
                    forward_length_squared =
                        forward_x * forward_x + forward_z * forward_z;
                    if (forward_length_squared <= 0.0f) {
                        inverse_forward_length = 0.0f;
                    } else {
                        forward_estimate.value = forward_length_squared;
                        forward_estimate.bits =
                            0x5F375A00 - (forward_estimate.bits >> 1);
                        forward_product =
                            forward_estimate.value *
                            (forward_length_squared *
                             forward_estimate.value);
                        forward_correction = 3.0f - forward_product;
                        inverse_forward_length =
                            0.0625f * forward_estimate.value *
                            forward_correction *
                            -(forward_correction *
                                  (forward_product * forward_correction) -
                              12.0f);
                    }
                    forward_z *= inverse_forward_length;
                    forward_x *= inverse_forward_length;
                    angle = gxMathArcCos(
                        delta_z * forward_z + delta_x * forward_x);
                    if (object_distance + angle <
                        *distance + *facing_angle) {
                        *distance = object_distance;
                        selected = npc;
                        *facing_angle = angle;
                    }
                }
                link = link->next;
            }
        }
    }
    return selected;
}

static inline StringObj* resolve_dialog_string(
    const KonquestStringLatch* latch) {
    StringObj* object;

    object = latch->object;
    if (object != 0) {
        if (object->instance == latch->instance) {
            /* Valid string-object latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    return object;
}

static float p_konquest_dialog(void) {
    KonquestDialogPdata* pdata;
    char* token;
    char* line_text;
    StringObj* current_line;
    StringObj* line;
    int character_index;
    int length;

    pdata = (KonquestDialogPdata*)pdata_of_proc(aproc);
    {
        char delimiters[2] = " ";

        line_text = pdata->line_text[0];
        token = strtok(pdata->text, delimiters);
        while (token != 0) {
            current_line = 0;
            if (pdata->line_index == 0) {
                line = resolve_dialog_string(&pdata->lines[0]);
                current_line = line;
                if ((float)(line->text_w + get_string_width_by_font_num(
                                               pdata->font, token)) > 500.0f) {
                    line = resolve_dialog_string(&pdata->lines[1]);
                    current_line = line;
                    line_text = pdata->line_text[1];
                    pdata->line_index++;
                    pdata->skip_print_delay = 0;
                }
            }

            if (pdata->line_index == 1) {
                line = resolve_dialog_string(&pdata->lines[1]);
                current_line = line;
                if ((float)(line->text_w + get_string_width_by_font_num(
                                               pdata->font, token)) > 500.0f) {
                    StringObj* second_line;
                    StringObj* first_line;

                    line = resolve_dialog_string(&pdata->lines[1]);
                    second_line = line;
                    first_line = resolve_dialog_string(&pdata->lines[0]);
                    strcpy(pdata->line_text[0], second_line->text);
                    update_string_obj(first_line, pdata->font,
                                      pdata->line_text[0]);
                    strcpy(pdata->line_text[1], "");
                    update_string_obj(second_line, pdata->font, "");
                    line_text = pdata->line_text[0];
                    pdata->line_index = 0;
                    pdata->skip_print_delay = 0;
                    continue;
                }
            }

            length = strlen(token);
            for (character_index = 0; character_index < length;
                 character_index++) {
                if (pdata->print_ticks > 0) {
                    pdata->print_ticks -= pdata->print_speed;
                }
                strncat(line_text, token + character_index, 1);
                update_string_obj(current_line, pdata->font, line_text);
                if (pdata->print_speed != 0 && pdata->skip_print_delay == 0) {
                    _mkproc_sleep_ticks = (float)pdata->print_speed;
                    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
                }
            }
            token = strtok(0, delimiters);
            strncat(line_text, " ", 1);
        }
    }

    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    while (find_mkproc_pid(0x8232) != 0) {
        if (pdata->print_ticks > 0) {
            pdata->print_ticks--;
        }
        if (pdata->active != 0 && pdata->print_ticks <= 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    _mkproc_sleep_ticks = 5.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    return -1.0f;
}

#pragma optimize_for_size on
void calc_print_speed_for_nis_dialog(MkProc* dialog, unsigned int ticks) {
    KonquestDialogPdata* pdata;
    int length;
    int speed;
    int clamped_speed;

    speed = 3;
    pdata = (KonquestDialogPdata*)pdata_of_proc(dialog);
    length = strlen(pdata->text);
    if (length > 0) {
        speed = ticks / (unsigned int)length;
    }
    clamped_speed = 10;
    if (speed < 10) {
        clamped_speed = speed;
    }
    if (clamped_speed >= 2) {
        clamped_speed = 10;
        if (speed < 10) {
            clamped_speed = speed;
        }
    } else {
        clamped_speed = 2;
    }
    pdata->print_speed = clamped_speed;
    pdata->print_ticks = ticks;
    pdata->active = 1;
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: mode dispatch, NPC gate, lip-sync duration, double ceil,
 * print-speed clamp, font load, and dialog creation ABI match retail. The
 * 20-byte residue is scalar nonvolatile saves/restores versus stmw/lmw and
 * one equivalent print-speed move scheduled at the join.
 */
MkProc* konquest_set_dialog_text(
    const char* text, const LipSyncKeyframe* lip_sync_keyframes) {
    int print_speed;
    int text_length;
    float duration_per_character;

    print_speed = 3;
    text_length = strlen(text);
    switch (get_mode_of_play()) {
    case 7:
        if (((g_active_npc->fields.flags >> 4) & 1) == 0) {
            return 0;
        }
        if (lip_sync_keyframes != 0) {
            duration_per_character =
                duration_of_lip_sync(lip_sync_keyframes) /
                (float)text_length;
            if ((int)(float)ceil(duration_per_character) >= 10) {
                print_speed = 10;
            } else {
                print_speed = (int)(float)ceil(duration_per_character);
            }
        }
        return create_dialog_proc(
            g_active_npc->fields.dialog_art, text, print_speed);
    case 0:
        load_font(6);
        return create_dialog_proc(0, text, 3);
    default:
        return 0;
}
}

/*
 * Soft ceiling: create_dialog_proc ~99.74% -- allocation, process creation,
 * pdata layout, dialog-owner flag, three string latches, and copied buffers
 * are instruction-exact. The only residue is pooled-string relocation labels.
 */
static MkProc* create_dialog_proc(KonquestDialogArt* dialog_art,
                                  const char* text, int print_speed) {
    MkProc* proc;
    KonquestDialogPdata* pdata;
    StringObj* line;
    int flags;
    int flags_arg;

    pdata = (KonquestDialogPdata*)get_mkhdr(&vtbl_dialog_pdata, sizeof(*pdata));
    if (pdata != 0) {
        flags = 0;
        ((MkProcCreateFlagBits*)&flags)->has_pdata = 1;
        flags_arg = flags;
        proc = create_mkproc(0x1F, get_mkproc_tinystack(&flags_arg), 0xA012,
                             p_konquest_dialog, &pdata->hdr);
    } else {
        return 0;
    }
    if (proc != 0) {
        pdata->line_index = 0;
        pdata->font = 6;
        pdata->dialog_art = dialog_art;
        pdata->print_speed = print_speed;
        pdata->skip_print_delay = 0;
        pdata->print_ticks = 0;
        pdata->active = 0;
        if (dialog_art != 0) {
            dialog_art->owner->flag_bits.dialog_active = 1;
        }

        pdata->lines[0].object = 0;
        pdata->lines[0].instance = 0;
        pdata->lines[1].object = 0;
        pdata->lines[1].instance = 0;
        pdata->lines[2].object = 0;
        pdata->lines[2].instance = 0;

        line = string_left_xy(0x8300, pdata->font, " ", 0x40, 0x41, 0);
        pdata->lines[0].object = line;
        pdata->lines[0].instance = line->instance;
        mk_insert((MkHdr*)line, &proc->pdata_list_b);

        line = string_left_xy(0x8300, pdata->font, " ", 0x40,
                              0x41 - line->text_h, 0);
        pdata->lines[1].object = line;
        pdata->lines[1].instance = line->instance;
        mk_insert((MkHdr*)line, &proc->pdata_list_b);

        line = string_left_xy(0x8300, pdata->font, " ", 0x40,
                              0x41 - line->text_h * 2, 0);
        pdata->lines[2].object = line;
        pdata->lines[2].instance = line->instance;
        mk_insert((MkHdr*)line, &proc->pdata_list_b);

        strcpy(pdata->text, text);
        strcpy(pdata->line_text[0], "");
        strcpy(pdata->line_text[1], "");
        strcpy(pdata->line_text[2], "");
        return proc;
    }
    return 0;
}

/*
 * Soft ceiling: vdestroy_dialog_pdata ~88.95% -- dialog flag clearing, all
 * three generation-checked string destructions, latch clearing, and pdata
 * release agree with retail. Residue is equivalent latch-join layout and
 * temporary-register selection around the repeated ownership checks.
 */
static void vdestroy_dialog_pdata(KonquestDialogPdata* pdata) {
    MkHdr* object;

    if (pdata->dialog_art != 0 && pdata->dialog_art->owner != 0) {
        pdata->dialog_art->owner->flag_bits.dialog_active = 0;
    }

    object = (MkHdr*)pdata->lines[0].object;
    if (object != 0) {
        if (object->instance == pdata->lines[0].instance) {
            /* Valid ownership latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        if (pdata->lines[0].object->instance != 0) {
            ((KonquestDestroyable*)pdata->lines[0].object)->vtbl->destroy(
                (KonquestDestroyable*)pdata->lines[0].object);
        }
        pdata->lines[0].object = 0;
        pdata->lines[0].instance = 0;
    }

    object = (MkHdr*)pdata->lines[1].object;
    if (object != 0) {
        if (object->instance == pdata->lines[1].instance) {
            /* Valid ownership latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        if (pdata->lines[1].object->instance != 0) {
            ((KonquestDestroyable*)pdata->lines[1].object)->vtbl->destroy(
                (KonquestDestroyable*)pdata->lines[1].object);
        }
        pdata->lines[1].object = 0;
        pdata->lines[1].instance = 0;
    }

    object = (MkHdr*)pdata->lines[2].object;
    if (object != 0) {
        if (object->instance == pdata->lines[2].instance) {
            /* Valid ownership latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        if (pdata->lines[2].object->instance != 0) {
            ((KonquestDestroyable*)pdata->lines[2].object)->vtbl->destroy(
                (KonquestDestroyable*)pdata->lines[2].object);
        }
        pdata->lines[2].object = 0;
        pdata->lines[2].instance = 0;
    }

    pdata->hdr.instance = 0;
    mkhdr_memfree(&pdata->hdr);
}

/*
 * Soft ceiling: konquest_end_npc_nis ~97.06% -- process teardown, camera/HUD
 * recovery, collision restart, ambient fade, mode-stack wait, and all final
 * flag/latch clears match retail. Residue is typed-array induction selection,
 * temporary-register coloring, and local floating-constant labels.
 */
void konquest_end_npc_nis(void) {
    KonquestSoundFadePdata* fade;
    MkProc* fade_proc;
    int mode_index;
    int index;
    int contains_nis_mode;

    destroy_mkprocs_pid(0xA01F);
    destroy_mkprocs_pid(0x9006);
    if (!g_active_npc->fields.state_flag_bits.bit6) {
        konquest_pdata->time_passing = 1;
        xfer_camera(konquest_camera_loop, 1);
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 1;
    }

    hero_stop_moving();
    old_hero_position.y = 0.0f;
    old_hero_position.z = 0.0f;
    old_hero_position.x = 0.0f;
    old_hero_position.y = 500.0f;
    xfer_proc(konquest_pdata->collision_proc, p_collide_monk);

    fade = 0;
    fade_proc = find_mkproc_pid(0x822B);
    if (fade_proc != 0 && fade_proc->instance != 0) {
        ((KonquestDestroyable*)fade_proc)->vtbl->destroy(
            (KonquestDestroyable*)fade_proc);
    }
    if (_create_mkproc_generic_tinystack(
            0x822B, 0x1F, p_fade_ambient_sounds, sizeof(*fade),
            (void**)&fade) != 0) {
        fade->step = 0.013333333f;
        fade->current_volume = 0.6f;
        fade->target_volume = 1.0f;
    }

    while ((konquest_pdata->game_mode_index < 0
                ? 0
                : konquest_pdata
                      ->game_modes[konquest_pdata->game_mode_index]) != 3) {
        mode_index = konquest_pdata->game_mode_index;
        contains_nis_mode = 0;
        for (index = 0; index <= mode_index; index++) {
            if (konquest_pdata->game_modes[index] == 3) {
                contains_nis_mode = 1;
                break;
            }
        }
        if (!contains_nis_mode) {
            break;
        }
        npc_ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_pdata != 0) {
        konquest_pdata->flag_bits.bit4 = 0;
    }
    g_active_npc->fields.flag_bits.flags_bit7 = 0;
    g_active_npc->fields.flag_bits.flags_bit2 = 0;
    if (((KonquestNpcRuntime*)g_active_npc)->type != 7) {
        g_active_npc->fields.flag_bits.flags_bit3 = 0;
        g_active_npc->fields.flag_bits.flags_bit6 = 0;
    }
    konquest_pdata->movement_npc.object = 0;
    konquest_pdata->movement_npc.instance = 0;
}
static inline int is_dialog_art_active(KonquestDialogArt* dialog_art) {
    if (dialog_art == 0) {
        return 0;
    }
    if (dialog_art->id_flags == 0) {
        return 0;
    }
    return dialog_art->bound_hdr != 0;
}


/*
 * Soft ceiling: konquest_start_npc_nis ~98.02% at the exact 696-byte retail
 * size. NPC/hero state setup, animation transition, HUD and ambient fades,
 * dialog-art wait, and movement latch all align. Residue is one equivalent
 * ownership-latch branch shape, boolean truth normalization, and local
 * floating-constant relocation labels.
 */
void konquest_start_npc_nis(void) {
    MkObj* hero;
    MkProc* fade_proc;
    KonquestSoundFadePdata* fade;
    MonkStateData* state;

    if (g_active_npc != 0) {
        g_active_npc->fields.flag_bits.flags_bit6 = 1;
        g_active_npc->fields.flag_bits.flags_bit7 = 1;
        g_active_npc->fields.flag_bits.flags_bit3 = 1;
    }
    if (konquest_pdata != 0) {
        konquest_pdata->flag_bits.bit4 = 0;
    }

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        hero->pos_vel.z = 0.0f;
        hero->pos_vel.y = 0.0f;
        hero->pos_vel.x = 0.0f;
        hero->flags_09_bits.bit6 = 1;
        konquest_pdata->hero_state = 0;
        hero->hide_flag_bits.still_move = 1;
        konquest_pdata->hero_anim->weight = 1.0f;
    }
    state = &monk_state_data[konquest_pdata->hero_state];
    transition_to_anim_script(
        konquest_pdata->hero_anim, state->animation, state->transition, 0.1f);
    konquest_pdata->time_passing = 0;

    if (aproc->pid == 0xA014) {
        npc_set_wait_ticks(1.0f);
        npc_suspend_cmdscript();
        return;
    }
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    if (!g_active_npc->fields.state_flag_bits.bit6) {
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 0;
    }

    fade = 0;
    fade_proc = find_mkproc_pid(0x822B);
    if (fade_proc != 0 && fade_proc->instance != 0) {
        ((KonquestDestroyable*)fade_proc)->vtbl->destroy(
            (KonquestDestroyable*)fade_proc);
    }
    if (_create_mkproc_generic_tinystack(
            0x822B, 0x1F, p_fade_ambient_sounds, sizeof(*fade),
            (void**)&fade) != 0) {
        fade->step = -0.013333333f;
        fade->current_volume = 1.0f;
        fade->target_volume = 0.6f;
    }

    while (!is_dialog_art_active(g_active_npc->fields.dialog_art)) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    konquest_pdata->movement_npc.object = (MkHdr*)g_active_npc;
    konquest_pdata->movement_npc.instance =
        ((MkHdr*)g_active_npc)->instance;
}

/*
 * Soft ceiling: konquest_end_npc_interaction ~97.75%. Teardown, camera/HUD
 * recovery, collision restart, ambient fade, mode-stack wait/pop, and final
 * flag/latch cleanup align with retail. The four-byte size residue is typed
 * array pointer induction in place of retail's byte-offset lwzx loop; the
 * other differences are local floating-constant relocation labels.
 */
void konquest_end_npc_interaction(void) {
    KonquestSoundFadePdata* fade;
    MkProc* fade_proc;
    int mode_index;
    int index;
    int contains_interaction_mode;

    del_string_obj_by_id(0x900F);
    destroy_mkprocs_pid(0xA01F);
    destroy_mkprocs_pid(0xA01F);
    destroy_mkprocs_pid(0x9006);
    if (!g_active_npc->fields.state_flag_bits.bit6) {
        konquest_pdata->time_passing = 1;
        xfer_camera(konquest_camera_loop, 1);
        do {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        } while (find_mkproc_pid(0x8229) != 0);
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 1;
    }

    hero_stop_moving();
    old_hero_position.y = 0.0f;
    old_hero_position.z = 0.0f;
    old_hero_position.x = 0.0f;
    old_hero_position.y = 500.0f;
    xfer_proc(konquest_pdata->collision_proc, p_collide_monk);

    fade = 0;
    fade_proc = find_mkproc_pid(0x822B);
    if (fade_proc != 0 && fade_proc->instance != 0) {
        ((KonquestDestroyable*)fade_proc)->vtbl->destroy(
            (KonquestDestroyable*)fade_proc);
    }
    if (_create_mkproc_generic_tinystack(
            0x822B, 0x1F, p_fade_ambient_sounds, sizeof(*fade),
            (void**)&fade) != 0) {
        fade->step = 0.013333333f;
        fade->current_volume = 0.6f;
        fade->target_volume = 1.0f;
    }

    while (((mode_index = konquest_pdata->game_mode_index) < 0
                ? 0
                : konquest_pdata->game_modes[mode_index]) != 1) {
        contains_interaction_mode = 0;
        for (index = 0; index <= mode_index; index++) {
            if (konquest_pdata->game_modes[index] == 1) {
                contains_interaction_mode = 1;
                break;
            }
        }
        if (!contains_interaction_mode) {
            break;
        }
        npc_ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    if (mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }

    g_active_npc->fields.flag_bits.flags_bit7 = 0;
    if (konquest_pdata != 0) {
        konquest_pdata->flag_bits.bit4 = 0;
    }
    if (((KonquestNpcRuntime*)g_active_npc)->type != 7) {
        g_active_npc->fields.flag_bits.flags_bit3 = 0;
        g_active_npc->fields.flag_bits.flags_bit6 = 0;
    }
    konquest_pdata->movement_npc.object = 0;
    konquest_pdata->movement_npc.instance = 0;
}

void set_age_progression(int progression) {
    konquest_save_data.progression = progression;
}

/*
 * Soft ceiling: display_time_progression_images ~96.40%. The table lookup,
 * asset/audio lifecycle, both typed text windows, waits, and object teardown
 * match retail. The eight-byte size residue is MWCC's alternate signed / 2
 * lowering for the two image positions; other differences are register
 * coloring and local relocation labels.
 */
void display_time_progression_images(int progression) {
    AgeProgressionEntry* entry;
    KonquestTextWindowPdata* first_window;
    KonquestTextWindowPdata* second_window;
    ScreenObj* left_image;
    ScreenObj* right_image;
    MslSoundHandle sound;
    unsigned int string_id;
    int duration;
    float left_fraction;
    float bottom_fraction;
    float width_fraction;

    entry = &age_progression_table[progression - 1];
    pause_all_game_sounds();
    sound = snd_req(0x1AAA);
    load_ssf(entry->file_table);
    load_art_section_by_name(0x60027, entry->art_section);
    load_ssf((MkFileEntry*)konquest_common_file_table);
    load_art_section_language(0x60030, &sec_konquest_common_art);
    load_string_bank(
        0x40000, "konquest_info_box_descriptions_eng.mko");
    left_image = load_named_2d_pfxobj_xy(
        0x60027, 0x830F, "TIME_PROGRESSION_A", 0,
        screen_width / 2 - 0x180, 0, 0x43);
    right_image = load_named_2d_pfxobj_xy(
        0x60027, 0x830F, "TIME_PROGRESSION_B", 0,
        screen_width / 2 + 0x80, 0, 0x43);
    turn_camera_on();
    fade_from_black(8, 0);

    duration = entry->first_duration;
    string_id = entry->first_string_id;
    width_fraction = entry->width_fraction;
    bottom_fraction = entry->bottom_fraction;
    left_fraction = entry->left_fraction;
    if (_create_mkproc_generic_bigstack(
            0x9002, aproc->priority + 1, p_show_text_window,
            sizeof(*first_window), (void**)&first_window) != 0) {
        zero_pdata_payload(sizeof(*first_window), first_window);
        first_window->left =
            (int)((float)screen_width * left_fraction);
        first_window->bottom =
            (int)((float)screen_height -
                  (float)screen_height * bottom_fraction);
        first_window->priority = 0x24;
        first_window->width =
            (int)((float)screen_width * width_fraction);
        first_window->prompt_flags = 0x800;
        first_window->font = 9;
        first_window->timeout = duration;
        first_window->color = 0xC8C8C8FF;
        first_window->button_charmap = danton20_charmap;
        first_window->swap_buttons = 0;
        first_window->art_slot = 0x60030;
        first_window->controller_port = konquest_pdata->input_port;
        strcpy(
            first_window->text, get_string_by_id(string_id | 0x40000));
        text_window_state = 0;
        while (text_window_state == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }

    string_id = entry->second_string_id;
    if (string_id != 0) {
        duration = entry->second_duration;
        width_fraction = entry->width_fraction;
        bottom_fraction = entry->bottom_fraction;
        left_fraction = entry->left_fraction;
        if (_create_mkproc_generic_bigstack(
                0x9002, aproc->priority + 1, p_show_text_window,
                sizeof(*second_window), (void**)&second_window) != 0) {
            zero_pdata_payload(sizeof(*second_window), second_window);
            second_window->left =
                (int)((float)screen_width * left_fraction);
            second_window->bottom =
                (int)((float)screen_height -
                      (float)screen_height * bottom_fraction);
            second_window->priority = 0x24;
            second_window->width =
                (int)((float)screen_width * width_fraction);
            second_window->prompt_flags = 0x800;
            second_window->font = 9;
            second_window->timeout = duration;
            second_window->color = 0xC8C8C8FF;
            second_window->button_charmap = danton20_charmap;
            second_window->swap_buttons = 0;
            second_window->art_slot = 0x60030;
            second_window->controller_port = konquest_pdata->input_port;
            strcpy(
                second_window->text,
                get_string_by_id(string_id | 0x40000));
            text_window_state = 0;
            while (text_window_state == 0) {
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
            }
        }
    }

    fade_to_black(8, 0);
    if (mslSoundIsValid(sound)) {
        snd_stop(sound);
    }
    unpause_all_game_sounds();
    if (left_image->instance != 0) {
        left_image->typed_vtbl->destroy(left_image);
    }
    if (right_image->instance != 0) {
        right_image->typed_vtbl->destroy(right_image);
    }
}

/*
 * Soft ceiling: start_character_separation_process ~97.28%. The eligibility,
 * script-yield path, dialog-art wait, and typed interaction pdata setup match
 * retail. The four-byte residue is pointer truth normalization in the shared
 * dialog-art predicate; other differences are branch labeling only.
 */
void start_character_separation_process(float separation) {
    KonquestInteractionPdata* pdata;

    if (is_this_the_monk_npc(g_active_npc) == 0 &&
        g_active_npc->fields.flag_bits.flags_bit6) {
        if (aproc->pid == 0xA014) {
            npc_set_wait_ticks(1.0f);
            npc_suspend_cmdscript();
            return;
        }

        while (!is_dialog_art_active(g_active_npc->fields.dialog_art)) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        }

        _create_mkproc_generic_tinystack(
            0xA01F, 0x19, p_konquest_interaction, sizeof(*pdata),
            (void**)&pdata);
        zero_pdata_payload(sizeof(*pdata), pdata);
        pdata->npc = g_active_npc;
        pdata->separation_active = 1;
        pdata->facing_active = 1;
        pdata->separation_ticks = 0x2D;
        pdata->separation = separation;
    }
}

/*
 * Soft ceiling: konquest_start_npc_interaction ~94.96%. NPC/hero setup,
 * mode-stack handling, animation drain, HUD/ambient fades, dialog wait,
 * interaction pdata, movement latch, and conversation count match retail.
 * The eight-byte size residue is typed-array induction and equivalent
 * negative-index/predicate lowering; other differences are register/save
 * coloring and local floating-constant relocation labels.
 */
void konquest_start_npc_interaction(void) {
    KonquestSoundFadePdata* fade;
    KonquestInteractionPdata* interaction;
    MkProc* fade_proc;
    MkObj* hero;

    if (g_active_npc != 0) {
        g_active_npc->fields.flag_bits.flags_bit6 = 1;
        g_active_npc->fields.flag_bits.flags_bit7 = 1;
        g_active_npc->fields.flag_bits.flags_bit3 = 1;
    }

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        hero->pos_vel.z = 0.0f;
        hero->pos_vel.y = 0.0f;
        hero->pos_vel.x = 0.0f;
        hero->flags_09_bits.bit6 = 1;
        konquest_pdata->hero_state = 0;
        hero->hide_flag_bits.still_move = 1;
        konquest_pdata->hero_anim->weight = 1.0f;
    }
    transition_to_anim_script(
        konquest_pdata->hero_anim,
        monk_state_data[konquest_pdata->hero_state].animation,
        monk_state_data[konquest_pdata->hero_state].transition, 0.1f);
    konquest_pdata->time_passing = 0;

    if (aproc->pid == 0xA014) {
        npc_set_wait_ticks(1.0f);
        npc_suspend_cmdscript();
        return;
    }

    if (konquest_game_mode_in_stack(1) != 0) {
        npc_blend_to_ani(
            g_active_npc->fields.data->idle_animation,
            0, 0.1f, 1.0f);
        do {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
            npc_ani_1_frame();
        } while (konquest_game_mode_in_stack(1) != 0);

        hero = konquest_pdata->hero_object;
        if (hero != 0) {
            if (hero->hdr.instance == konquest_pdata->hero_instance) {
                /* Valid hero latch. */
            } else {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero != 0) {
            hero->pos_vel.z = 0.0f;
            hero->pos_vel.y = 0.0f;
            hero->pos_vel.x = 0.0f;
            hero->flags_09_bits.bit6 = 1;
            konquest_pdata->hero_state = 0;
            hero->hide_flag_bits.still_move = 1;
            konquest_pdata->hero_anim->weight = 1.0f;
        }
        transition_to_anim_script(
            konquest_pdata->hero_anim,
            monk_state_data[konquest_pdata->hero_state].animation,
            monk_state_data[konquest_pdata->hero_state].transition, 0.1f);
        konquest_pdata->time_passing = 0;
    }

    if (konquest_current_game_mode() != 1 &&
        konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 1;
    }

    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    if (!g_active_npc->fields.state_flag_bits.bit6) {
        destroy_mkprocs_pid(0xA023);
        _create_mkproc_generic_tinystack(
            0xA023, 0x2E, p_fade_konquest_hud, 0, 0);
        g_fade_hud_in = 0;
    }

    fade = 0;
    fade_proc = find_mkproc_pid(0x822B);
    if (fade_proc != 0 && fade_proc->instance != 0) {
        ((KonquestDestroyable*)fade_proc)->vtbl->destroy(
            (KonquestDestroyable*)fade_proc);
    }
    if (_create_mkproc_generic_tinystack(
            0x822B, 0x1F, p_fade_ambient_sounds, sizeof(*fade),
            (void**)&fade) != 0) {
        fade->step = -0.013333333f;
        fade->current_volume = 1.0f;
        fade->target_volume = 0.6f;
    }

    while (!is_dialog_art_active(g_active_npc->fields.dialog_art)) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (is_this_the_monk_npc(g_active_npc) == 0) {
        _create_mkproc_generic_tinystack(
            0xA01F, 0x19, p_konquest_interaction,
            sizeof(*interaction), (void**)&interaction);
        zero_pdata_payload(sizeof(*interaction), interaction);
        interaction->npc = g_active_npc;
        interaction->separation_active = 1;
        interaction->facing_active = 1;
        interaction->separation_ticks = 0x2D;
        interaction->separation = 1.5f;
    }

    konquest_pdata->movement_npc.object = (MkHdr*)g_active_npc;
    konquest_pdata->movement_npc.instance =
        ((MkHdr*)g_active_npc)->instance;
    ((KonquestNpcRuntime*)g_active_npc)->conversation_count++;
}
static inline float konquest_fast_sqrt(float value) {
    KonquestFloatBits estimate;
    unsigned int exponent;
    unsigned int table_index;

    estimate.value = value;
    if (value <= 0.0f) {
        return 0.0f;
    }
    table_index = (estimate.bits >> 10) & 0x3FFE;
    exponent =
        (((estimate.bits & 0x7F800000) + 0x3F800000) >> 1) &
        0x7F800000;
    /* Retail's packed table index is a byte offset, not an element index. */
    estimate.bits =
        *(unsigned short*)((unsigned char*)GXMathSqrtTable + table_index) << 8;
    estimate.bits |= exponent;
    return 0.5f *
        (estimate.value *
         (3.0f - (estimate.value * estimate.value) / value));
}


/*
 * Soft ceiling: p_konquest_interaction ~94.84%. Separation/collision
 * response, hero and NPC facing, stale turn-process cleanup, object updates,
 * and completion return agree with retail. The 20-byte size residue is
 * individual nonvolatile restores instead of lmw; remaining differences are
 * GPR/FPR coloring, folded latch joins, and local constant relocations.
 */
static float p_konquest_interaction(void) {
    MkObj* hero;
    KonquestInteractionPdata* pdata;
    MkProc* turn_proc;
    MkProc* validated_turn_proc;
    MkProc* current_turn_proc;
    Vec movement;
    Vec target_position;
    float length_squared;
    float inverse_length;
    float distance;
    float target_angle;
    float difference;
    int hero_facing;
    int npc_facing;

    pdata = (KonquestInteractionPdata*)apdata;
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    hero_facing = 0;
    npc_facing = 0;
    movement.z = 0.0f;
    movement.y = 0.0f;
    movement.x = 0.0f;

    turn_proc = pdata->npc->fields.turn_proc;
    if (turn_proc != 0) {
        if (turn_proc->instance == pdata->npc->fields.turn_proc_instance) {
            validated_turn_proc = turn_proc;
        } else {
            validated_turn_proc = 0;
        }
    } else {
        validated_turn_proc = 0;
    }
    if (validated_turn_proc != 0) {
        /* Revalidate the owner latch before destroying and clearing it. */
        current_turn_proc = pdata->npc->fields.turn_proc;
        if (current_turn_proc != 0) {
            if (current_turn_proc->instance !=
                pdata->npc->fields.turn_proc_instance) {
                current_turn_proc = 0;
            }
        } else {
            current_turn_proc = 0;
        }
        if (current_turn_proc != 0) {
            if (turn_proc->instance != 0) {
                ((KonquestDestroyable*)turn_proc)->vtbl->destroy(
                    (KonquestDestroyable*)turn_proc);
            }
            pdata->npc->fields.turn_proc = 0;
            pdata->npc->fields.turn_proc_instance = 0;
        }
    }

    if (hero == 0) {
        return -1.0f;
    }
    if (pdata->position_initialized == 0) {
        pdata->last_hero_position.x = hero->pos.value.x;
        pdata->last_hero_position.y = hero->pos.value.y;
        pdata->last_hero_position.z = hero->pos.value.z;
        pdata->position_initialized = 1;
    }

    if (pdata->separation_active != 0) {
        pdata->separation_ticks--;
        movement.x =
            hero->pos.value.x - pdata->npc->fields.data->position.x;
        movement.z =
            hero->pos.value.z - pdata->npc->fields.data->position.z;
        length_squared =
            movement.x * movement.x + movement.z * movement.z;
        inverse_length = konquest_inverse_length(length_squared);
        movement.x *= inverse_length;
        movement.z *= inverse_length;

        movement.x *= pdata->separation;
        movement.z *= pdata->separation;
        target_position.x =
            movement.x + pdata->npc->fields.data->position.x;
        target_position.z =
            movement.z + pdata->npc->fields.data->position.z;
        movement.x = hero->pos.value.x - target_position.x;
        movement.z = hero->pos.value.z - target_position.z;
        length_squared =
            movement.x * movement.x + movement.z * movement.z;
        distance = konquest_fast_sqrt(length_squared);
        if (distance > 0.0f) {
            inverse_length = 1.0f / distance;
        } else {
            inverse_length = distance;
        }
        movement.x *= inverse_length;
        movement.z *= inverse_length;
        hero->hide_flag_bits.pin_animation = 0;

        if (interaction_cam_glitched() == 0 && distance > 0.05f) {
            movement.x *= -0.05f;
            movement.z *= -0.05f;
            if (repel_against_global_collision_list(
                    &hero->pos.value, &movement, &target_position) == 0) {
                hero->pos.value.x += movement.x;
                hero->pos.value.z += movement.z;
                pdata->last_hero_position.x = hero->pos.value.x;
                pdata->last_hero_position.z = hero->pos.value.z;
            } else {
                hero->pos.value.x = pdata->last_hero_position.x;
                hero->pos.value.z = pdata->last_hero_position.z;
            }
        } else {
            movement.x *= -distance;
            movement.z *= -distance;
            if (repel_against_global_collision_list(
                    &hero->pos.value, &movement, &target_position) == 0) {
                hero->pos.value.x += movement.x;
                hero->pos.value.z += movement.z;
                pdata->last_hero_position.x = hero->pos.value.x;
                pdata->last_hero_position.z = hero->pos.value.z;
            } else {
                hero->pos.value.x = pdata->last_hero_position.x;
                hero->pos.value.z = pdata->last_hero_position.z;
            }
        }

        if (pdata->separation_ticks <= 0) {
            xfer_proc(konquest_pdata->collision_proc, p_idle);
            pdata->separation_active = 0;
        }
    }

    if (pdata->facing_active != 0) {
        movement.x =
            pdata->npc->fields.data->position.x - hero->pos.value.x;
        movement.z =
            pdata->npc->fields.data->position.z - hero->pos.value.z;
        target_angle = gxMathArcTanYX(movement.x, movement.z);
        difference = ang_sub_ang(target_angle, hero->ang.y);
        if (interaction_cam_glitched() == 0 &&
            (difference >= 0.0f ? difference : -difference) > 0.07f) {
            hero->hide_flag_bits.pin_animation = 0;
            if (difference < 0.0f) {
                hero->ang.y -= 0.07f;
            } else {
                hero->ang.y += 0.07f;
            }
        } else {
            hero->ang.y = target_angle;
            hero_facing = 1;
        }

        movement.x *= -1.0f;
        movement.z *= -1.0f;
        target_angle = gxMathArcTanYX(movement.x, movement.z);
        difference =
            ang_sub_ang(target_angle, pdata->npc->fields.data->angle_y);
        if (interaction_cam_glitched() == 0 &&
            (difference >= 0.0f ? difference : -difference) > 0.07f) {
            pdata->npc->fields.animation_state->object
                ->hide_flag_bits.pin_animation = 0;
            if (difference < 0.0f) {
                pdata->npc->fields.animation_state->object->ang.y -= 0.07f;
            } else {
                pdata->npc->fields.animation_state->object->ang.y += 0.07f;
            }
        } else {
            npc_facing = 1;
            pdata->npc->fields.animation_state->object->ang.y = target_angle;
        }
    }

    update_mkobj(
        pdata->npc->fields.animation_state->object != 0
            ? as_mkhdr(&pdata->npc->fields.animation_state->object->hdr)
            : 0);
    update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
    if (pdata->separation_active == 0 &&
        hero_facing != 0 && npc_facing != 0) {
        return -1.0f;
    }
    return 1.0f;
}

/*
 * Soft ceiling: set_movement_npc ~92.90%. Target selection, both generation
 * latches, camera-target readiness, and focus object agree with retail. The
 * four-byte size residue is two folded empty latch joins offset by alternate
 * pointer-truth normalization.
 */
void set_movement_npc(int target_type) {
    KonquestNpc* npc;
    KonquestNpc* candidate;
    KonquestNpcAnimationState* animation;
    int valid;

    npc = 0;
    switch (target_type) {
    case 1:
        break;
    case 2:
        candidate = (KonquestNpc*)konquest_pdata->hero_grounding;
        if (candidate != 0) {
            if (candidate->fields.hdr.instance ==
                konquest_pdata->grounding_instance) {
                /* Valid NPC latch. */
            } else {
                candidate = 0;
            }
        } else {
            candidate = 0;
        }
        npc = candidate;
        break;
    case 0:
        candidate =
            (KonquestNpc*)konquest_pdata->movement_npc.object;
        if (candidate != 0) {
            if (candidate->fields.hdr.instance ==
                konquest_pdata->movement_npc.instance) {
                /* Valid NPC latch. */
            } else {
                candidate = 0;
            }
        } else {
            candidate = 0;
        }
        npc = candidate;
        break;
    }
    if (npc != 0) {
        animation = npc->fields.animation_state;
        if (animation == 0) {
            valid = 0;
        } else if (animation->object == 0) {
            valid = 0;
        } else {
            valid = animation->proc != 0;
        }
        if (valid != 0) {
            camera_set_movement_focus_obj(animation->object);
        }
    }
}

/*
 * Soft ceiling: set_look_at_npc ~92.90%. Target selection, both generation
 * latches, camera-target readiness, and focus object agree with retail. The
 * four-byte size residue is two folded empty latch joins offset by alternate
 * pointer-truth normalization.
 */
void set_look_at_npc(int target_type) {
    KonquestNpc* npc;
    KonquestNpc* candidate;
    KonquestNpcAnimationState* animation;
    int valid;

    npc = 0;
    switch (target_type) {
    case 1:
        break;
    case 2:
        candidate = (KonquestNpc*)konquest_pdata->hero_grounding;
        if (candidate != 0) {
            if (candidate->fields.hdr.instance ==
                konquest_pdata->grounding_instance) {
                /* Valid NPC latch. */
            } else {
                candidate = 0;
            }
        } else {
            candidate = 0;
        }
        npc = candidate;
        break;
    case 0:
        candidate =
            (KonquestNpc*)konquest_pdata->movement_npc.object;
        if (candidate != 0) {
            if (candidate->fields.hdr.instance ==
                konquest_pdata->movement_npc.instance) {
                /* Valid NPC latch. */
            } else {
                candidate = 0;
            }
        } else {
            candidate = 0;
        }
        npc = candidate;
        break;
    }
    if (npc != 0) {
        animation = npc->fields.animation_state;
        if (animation == 0) {
            valid = 0;
        } else if (animation->object == 0) {
            valid = 0;
        } else {
            valid = animation->proc != 0;
        }
        if (valid != 0) {
            camera_set_lookat_focus(animation->object);
        }
    }
}

/* Soft ceiling: set_interaction_camera_script ~96% -- global load coloring. */
void set_interaction_camera_script(void* script) {
    if ((g_active_npc->raw[0x1D] & 0x40) == 0) {
        run_interaction_camera_script(konquest_pdata->script_owner, script);
    }
}

void display_konquest_title(void) {
    MkHdr* pdata;
    MkProc* proc;

    if (konquest_editor_mode_on == 0) {
        proc = _create_mkproc_generic_tinystack(
            0x8228, 0x1F, p_display_konquest_title,
            sizeof(*pdata), (void**)&pdata);
        if (proc != 0) {
            proc->flags_bits.use_game_speed = 1;
        }
    }
}

/*
 * Soft ceiling: p_display_konquest_title ~94.68%. Asset placement, refresh-
 * dependent waits, both alpha ramps, destruction, and return match retail at
 * the same control-flow shape. The eight-byte size residue is individual
 * r30/r31 saves and restores instead of stmw/lmw.
 */
static float p_display_konquest_title(void) {
    ScreenObj* title;
    unsigned char alpha;

    title = load_named_2d_pfxobj(
        0x60029, 0x830D, "ER0_MK_KONQUEST", 0, 0x40);
    if (is_widescreen_mode()) {
        title->x = 0;
    } else {
        title->x = -30;
    }
    title->y = screen_height / 4;
    pfx_2d_obj_set_alpha(title, 0);

    if (refresh_rate() == 60) {
        _mkproc_sleep_ticks = 600.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    } else {
        _mkproc_sleep_ticks = 530.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    for (alpha = 0; alpha < 253; alpha += 2) {
        pfx_2d_obj_set_alpha(title, alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (refresh_rate() == 60) {
        _mkproc_sleep_ticks = 100.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    } else {
        _mkproc_sleep_ticks = 20.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    for (alpha = 255; alpha > 2; alpha -= 2) {
        pfx_2d_obj_set_alpha(title, alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (title->instance != 0) {
        ((KonquestDestroyable*)title)->vtbl->destroy(
            (KonquestDestroyable*)title);
    }
    return -1.0f;
}

void konquest_hide_hud(int unused) {
    MkHdr* object;

    konquest_pdata->hud_visible = 0;

    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[2], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[0], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[1], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[3], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[4], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[3], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[4], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[5], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[6], object);
    if (object != 0) hide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[0], object);
    if (object != 0) hide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[1], object);
    if (object != 0) hide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[2], object);
    if (object != 0) hide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_extra_label, object);
    if (object != 0) hide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[5], object);
    if (object != 0) hide_string_obj((StringObj*)object);
}

void konquest_fade_hud(int fade_in) {
    destroy_mkprocs_pid(0xa023);
    _create_mkproc_generic_tinystack(
        0xa023, 0x2e, p_fade_konquest_hud, 0, 0);
    g_fade_hud_in = fade_in;
}

static float p_fade_konquest_hud(void) {
    MkHdr* object;
    float alpha;
    float step;

    if (g_fade_hud_in != 0) {
        alpha = 0.0f;
        step = 0.03f;
        konquest_show_hud();
    } else {
        alpha = 1.0f;
        step = -0.03f;
    }

    for (;;) {
        alpha += step;
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[2], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[0], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[1], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[3], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[4], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[3], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[4], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[5], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[6], object);
        if (object != 0) set_screen_obj_alpha((ScreenObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[0], object);
        if (object != 0) set_string_obj_alpha((StringObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[1], object);
        if (object != 0) set_string_obj_alpha((StringObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[2], object);
        if (object != 0) set_string_obj_alpha((StringObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_extra_label, object);
        if (object != 0) set_string_obj_alpha((StringObj*)object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[5], object);
        if (object != 0) set_string_obj_alpha((StringObj*)object, alpha);

        if (g_fade_hud_in != 0) {
            if (alpha >= 1.0f) {
                konquest_show_hud();
                break;
            }
        } else if (alpha <= 0.0f) {
            konquest_hide_hud(0);
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

void konquest_show_hud(void) {
    MkHdr* object;

    konquest_pdata->hud_visible = 1;
    if (konquest_pdata->objective_visible != 0) {
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[2], object);
        if (object != 0) unhide_screen_obj((ScreenObj*)object);
    }
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[0], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[1], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[3], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[4], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[5], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[6], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[0], object);
    if (object != 0) unhide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[1], object);
    if (object != 0) unhide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[2], object);
    if (object != 0) unhide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[4], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[5], object);
    if (object != 0) unhide_string_obj((StringObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[3], object);
    if (object != 0) unhide_screen_obj((ScreenObj*)object);
    KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_extra_label, object);
    if (object != 0) unhide_string_obj((StringObj*)object);
}

/*
 * Near match: code size and every instruction agree with retail; the only
 * remaining difference is the pooled "0" string's translation-unit offset.
 */
#pragma optimize_for_size on
#pragma dont_inline on
static void konquest_restore_saved_state(void) {
    char* script_name;
    int preserve_word;

    script_name = konquest_save_data.script_name;
    if (script_name != 0 && script_name[0] != 0 &&
        strcmp(script_name, "0") != 0) {
        active_cmdscript->unk28 = get_script_function_by_name(
            konquest_pdata->script_owner, script_name);
    }
    preserve_word = konquest_save_data.preserve_word;
    zero_pdata_payload(sizeof(konquest_save_data), (MkHdr*)&konquest_save_data);
    konquest_save_data.preserve_word = preserve_word;
    update_visible_tiles();
}
#pragma dont_inline reset
#pragma optimize_for_size reset

void konquest_transition_from_fight(void) {
    fade_to_black(8, 1);
    gamelogic_jump(4, p_konquest_mode);
}

/*
 * Near match: fighter assignment, saved-state fields, controller/timer state,
 * persistence calls, and transition process creation agree with retail.
 * Residue is nonvolatile-register coloring plus individual saves/restores.
 */
void konquest_transition_to_fight(int save_progress) {
    int other_state;
    PlyrInfo* player0;

    other_state = 0;
    g_game_info.field_1F8 = 1;
    if (konquest_save_data.fight_mode == 8) {
        other_state = 3;
    }

    player0 = &g_game_info.plyr0;
    if (player0->player_index == konquest_pdata->input_port) {
        set_player_state(player0, 2);
        set_player_state(&g_game_info.plyr1, other_state);
        player0->player_index = konquest_save_data.player_a;
        g_game_info.plyr1.player_index = konquest_save_data.player_b;
    } else {
        set_player_state(&g_game_info.plyr1, 2);
        set_player_state(player0, other_state);
        player0->player_index = konquest_save_data.player_b;
        g_game_info.plyr1.player_index = konquest_save_data.player_a;
    }
    turn_controllers_off();
    b_game_timer_off = 1;
    if (save_progress != 0) {
        konquest_save_data.valid = 1;
        konquest_save_data.region = konquest_pdata->region_index;
        save_konq_common_data_to_buffer();
        save_konq_memory_to_krd_buffer(
            p1_profile_konquest->raw[0x5D]);
    }
    _create_mkproc_generic_bigstack(
        0x9013, 0x1F, p_transition_to_fight, 0, 0);
}

static float p_transition_to_fight(void) {
    fade_to_black(8, 1);
    RwResourcesSetArenaSize(0x48000);
    mode_of_play = 8;
    gamelogic_jump(2, p_gamelogic);
    return -1.0f;
}

/*
 * Near match: mode/audio lifecycle, map transforms, rotated hero cursor,
 * objective/portal markers, six independent kamidogu branches, labels, input
 * wait, and cleanup agree. The 16-byte excess is equivalent mode/latch branch
 * emission; remaining annotations are register and pooled-string scheduling.
 */
static float p_konquest_map_screen(void) {
    KonquestPdata* pdata;
    MkObj* hero;
    ScreenObj* right_panel;
    ScreenObj* fade;
    ScreenObj* cursor;
    float map_scale_x;
    float map_scale_z;
    int map_left;
    int map_center_x;
    int map_center_y;
    int cursor_x;
    int index;

    pdata = konquest_pdata;
    hero = pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    map_scale_x = 1.0f;
    map_scale_z = 1.0f;

    if (konquest_current_game_mode() != 0) {
        pause_procs(0);
        return -1.0f;
    }
    if (find_mkproc_pid(0x8239) != 0) {
        pause_procs(0);
        return -1.0f;
    }
    if (konquest_current_game_mode() != 7 &&
        konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 7;
    }

    eat_switch_edge(konquest_pdata->input_port, 3);
    pause_all_game_sounds();
    snd_req(0x15A3);
    load_ssf(konquest_region_data[konquest_pdata->region_index].fight_files);
    load_art_section_by_name(
        0x60027,
        konquest_region_data[konquest_pdata->region_index].map_art_name);
    load_ssf(konquest_common_file_table);
    add_art_section(0x60027, &sec_konquest_map_common);
    load_font(0xF);
    konquest_hide_hud(0);

    map_left = screen_width / 2 - 0x140;
    load_named_2d_pfxobj_xy(
        0x60027, 0x8313, "MAP", 0, map_left, 0, 0x21);
    right_panel = load_named_2d_pfxobj_xy(
        0x60027, 0x8313, "MAP_LEGEND", 0,
        map_left + 0x200, 0, 0x21);
    fade = load_2d_pfxobj(0, 0x2098, (char*)0x10017, 0, 0x40);
    if (fade != 0) {
        fade->x = -50;
        fade->y = -50;
        fade->flag_bits.scaled = 1;
        fade->scale_x = 50.0f;
        fade->scale_y = 40.0f;
        pfx_2d_obj_set_alpha(fade, 0xFF);
    }

    map_center_x = map_left + 0x100;
    map_center_y = screen_height / 2;
    if (konquest_pdata->region_index == 1) {
        map_scale_x = 1.25f;
        map_center_x = map_left + 0x160;
        map_scale_z = 1.333f;
    } else if (konquest_pdata->region_index == 8) {
        map_scale_x = 7.0f;
        map_scale_z = 7.0f;
    }

    cursor_x = (int)(1.067f * (map_scale_x * hero->pos.value.x));
    cursor = load_named_2d_pfxobj_xy(
        0x60027, 0x8313, "HERO", 0,
        map_center_x + cursor_x - 0x10,
        map_center_y - (int)(map_scale_z * hero->pos.value.z) - 0x10,
        0x20);
    for (index = 0; index < 4; index++) {
        float angle;

        angle = -(1.5707964f * (float)index - 7.0685835f);
        cursor->pfx2d->verts[index].x =
            22.63f * gxMathCos(angle + hero->ang.y) + 16.0f;
        cursor->pfx2d->verts[index].y =
            22.63f * gxMathSin(angle + hero->ang.y) + 16.0f;
    }
    blink_cursor(cursor, 0x8245, 0x1E, 0xA);

    if (konquest_pdata->objective.visible != 0) {
        cursor_x = (int)(1.067f *
            (map_scale_x * konquest_pdata->objective.position.x));
        cursor = load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "OBJECTIVE", 0,
            map_center_x + cursor_x - 0x10,
            map_center_y -
                    (int)(map_scale_z *
                          konquest_pdata->objective.position.z) -
                0x10,
            0x20);
        blink_cursor(cursor, 0x8245, 0x1E, 0xA);
    }
    if (konquest_pdata->region_index != 1 &&
        konquest_pdata->region_index != 8) {
        cursor = load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "PORTAL", 0,
            map_center_x +
                    (int)(1.067f *
                          (map_scale_x *
                           konquest_pdata->region_table
                               ->map_portal_position.x)) -
                0x10,
            map_center_y -
                    (int)(map_scale_z *
                          konquest_pdata->region_table
                              ->map_portal_position.z) -
                0x10,
            0x20);
        blink_cursor(cursor, 0x8245, 0x1E, 0xA);
    }

    if (active_proc_list != 0) {
        MkPtr* link;

        link = active_proc_list;
        while (link != 0) {
            MkProc* proc;

            proc = (MkProc*)link->hdr;
            if (link->instance != proc->instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (proc->pid == 0x8245) {
                proc->flags_bits.skip_if_paused = 1;
            }
            link = link->next;
        }
    }

    if (get_konq_profile_value(0, 1) != 0) {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_EARTH", 0,
            right_panel->x + 0x10, screen_height - 0x66, 0x20);
    } else {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_EARTH_OFF", 0,
            right_panel->x + 0x10, screen_height - 0x66, 0x20);
    }
    if (get_konq_profile_value(0, 2) != 0) {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_NETHER", 0,
            right_panel->x + 0x39, screen_height - 0x66, 0x20);
    } else {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_NETHER_OFF", 0,
            right_panel->x + 0x39, screen_height - 0x66, 0x20);
    }
    if (get_konq_profile_value(0, 5) != 0) {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_CHAOS", 0,
            right_panel->x + 0x10, screen_height - 0x8E, 0x20);
    } else {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_CHAOS_OFF", 0,
            right_panel->x + 0x10, screen_height - 0x8E, 0x20);
    }
    if (get_konq_profile_value(0, 6) != 0) {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_ORDER", 0,
            right_panel->x + 0x39, screen_height - 0x8E, 0x20);
    } else {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_ORDER_OFF", 0,
            right_panel->x + 0x39, screen_height - 0x8E, 0x20);
    }
    if (get_konq_profile_value(0, 3) != 0) {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_OUTWORLD", 0,
            right_panel->x + 0x10, screen_height - 0xB7, 0x20);
    } else {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_OUTWORLD_OFF", 0,
            right_panel->x + 0x10, screen_height - 0xB7, 0x20);
    }
    if (get_konq_profile_value(0, 4) != 0) {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_EDENIA", 0,
            right_panel->x + 0x39, screen_height - 0xB7, 0x20);
    } else {
        load_named_2d_pfxobj_xy(
            0x60027, 0x8313, "KAMI_EDENIA_OFF", 0,
            right_panel->x + 0x39, screen_height - 0xB7, 0x20);
    }

    load_ssf(konquest_common_file_table);
    load_string_bank(0x50000, "konquest_map_strings_eng.mko");
    string_left_xy(
        0x8314, 0xF, get_string_by_id(0x50000), right_panel->x + 0x19,
        0xD8, 0x20);
    string_left_xy(
        0x8314, 0xF, get_string_by_id(0x50001), right_panel->x + 0x19,
        0xB8, 0x20);
    string_left_xy(
        0x8314, 0xF, get_string_by_id(0x50002), right_panel->x + 0x19,
        0x95, 0x20);

    while (check_switch_edge(konquest_pdata->input_port, 3) == 0 &&
           check_switch_edge(konquest_pdata->input_port, 6) == 0 &&
           is_controller_removed() == 0 &&
           check_switch_edge(konquest_pdata->input_port, 7) == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    snd_req(0x15A3);
    eat_switch_edge(konquest_pdata->input_port, 3);
    eat_switch_edge(konquest_pdata->input_port, 6);
    eat_switch_edge(konquest_pdata->input_port, 5);
    eat_switch_edge(konquest_pdata->input_port, 4);
    eat_switch_edge(konquest_pdata->input_port, 7);
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
    delete_screen_obj_oid(0x8313);
    del_string_obj_by_id(0x8314);
    load_string_bank(0x50000, "krypt_strings_eng.mko");
    unload_font(0xF);
    konquest_show_hud();
    pause_procs(0);
    unpause_all_game_sounds();
    if (fade != 0 && fade->instance != 0) {
        fade->typed_vtbl->destroy(fade);
    }
    return -1.0f;
}

/*
 * Soft ceiling: p_konquest_inventory ~98.6%. The mode stack, screen/pause
 * lifecycle, wait loop, cleanup, and ABI match. One equivalent compact
 * negative-index branch makes this four bytes smaller than retail; the other
 * residue is local string/float relocation labels.
 */
static float p_konquest_inventory(void) {
    if (konquest_current_game_mode() != 10 &&
        konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 10;
    }

    eat_switch_edge(konquest_pdata->input_port, 2);
    disable_all_ports_but_me(konquest_pdata->input_port);
    konquest_hide_hud(0);
    load_screen(
        "konquest/in_game/inventory", 0x60026,
        (MkHdr*)konquest_pdata, 0);
    pause_all_game_sounds();
    pause_procs(1);
    while (find_mkproc_pid(0x9011) != 0) {
        pause_procs(1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    if (get_game_state() != 0x14) {
        konquest_show_hud();
    }
    pause_procs(0);
    eat_switch_edge(konquest_pdata->input_port, 5);
    unpause_all_game_sounds();
    turn_all_ports_on();
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
    return -1.0f;
}

/*
 * Soft ceiling: p_konquest_inventory_switch 83.48% (728 versus 712 bytes).
 * Retail and local code perform the same player/process gates, six separately
 * inlined mode-stack scans, inventory-process creation, flag set, and pause.
 * Residue is loop induction/register allocation and constant relocation.
 */
float p_konquest_inventory_switch(void) {
    KonquestR1Pdata* input;
    MkProc* proc;
    int can_start;

    eat_switch_edge(konquest_pdata->input_port, 4);
    input = (KonquestR1Pdata*)apdata;
    if (input != 0 &&
        input->player !=
            g_game_info.pads[konquest_pdata->input_port].player) {
        return -1.0f;
    }
    if (find_mkproc_pid(0x9011) != 0) {
        can_start = 0;
    } else if (find_mkproc_pid(0x208C) != 0) {
        can_start = 0;
    } else if (find_mkproc_pid(0x8244) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(1) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(3) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(6) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(11) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(8) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(9) != 0) {
        can_start = 0;
    } else if (konquest_pdata->hero_state == 3) {
        can_start = 0;
    } else if (find_mkproc_pid(0x2098) != 0) {
        can_start = 0;
    } else {
        can_start = 1;
    }
    if (can_start == 0) {
        return -1.0f;
    }
    proc = (MkProc*)_create_mkproc_generic_bigstack(
        0x208C, 0x1F, p_konquest_inventory, 0, 0);
    if (proc != 0) {
        proc->flags_bits.skip_if_paused = 1;
        pause_procs(1);
    }
    return -1.0f;
}

/*
 * Soft ceiling: p_switch_proc_start 83.20% (716 versus 700 bytes). It is the
 * pause-menu twin of the inventory switch above, with the same proven gate
 * and the correct 0x902B process entry; the remaining differences are the
 * same repeated-loop emission and relocation labeling.
 */
float p_switch_proc_start(void) {
    KonquestR1Pdata* input;
    MkProc* proc;
    int can_start;

    input = (KonquestR1Pdata*)apdata;
    if (input != 0 &&
        input->player !=
            g_game_info.pads[konquest_pdata->input_port].player) {
        return -1.0f;
    }
    if (find_mkproc_pid(0x9011) != 0) {
        can_start = 0;
    } else if (find_mkproc_pid(0x208C) != 0) {
        can_start = 0;
    } else if (find_mkproc_pid(0x8244) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(1) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(3) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(6) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(11) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(8) != 0) {
        can_start = 0;
    } else if (konquest_game_mode_in_stack(9) != 0) {
        can_start = 0;
    } else if (konquest_pdata->hero_state == 3) {
        can_start = 0;
    } else if (find_mkproc_pid(0x2098) != 0) {
        can_start = 0;
    } else {
        can_start = 1;
    }
    if (can_start == 0) {
        return -1.0f;
    }
    proc = (MkProc*)_create_mkproc_generic_bigstack(
        0x902B, 0x1F, p_konquest_pause_menu, 0, 0);
    if (proc != 0) {
        proc->flags_bits.skip_if_paused = 1;
        pause_procs(1);
    }
    return -1.0f;
}

/*
 * Soft ceiling: p_konquest_pause_menu ~99.0% -- mode stack, screen/audio
 * lifecycle, save/exit branch, and cleanup match retail. One equivalent
 * negative-index branch is collapsed; the other residue is float relocation
 * labeling.
 */
static float p_konquest_pause_menu(void) {
    int pause_slot;
    int save_succeeded;

    pause_slot = get_pause_menu_ssh();
    if (konquest_current_game_mode() != 10 &&
        konquest_pdata->game_mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 10;
    }
    eat_switch_edge(konquest_pdata->input_port, 11);
    konquest_hide_hud(0);
    pause_player = 0;
    load_screen(get_pause_menu_name(), pause_slot, 0, 0);
    pause_all_game_sounds();
    pause_procs(1);
    while (find_mkproc_pid(0x9011) != 0) {
        pause_procs(1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    wait_for_screen_close();
    if (target_game_mode == 5) {
        save_succeeded = 1;
        if (konquest_save_on_exit != 0) {
            save_succeeded = full_konquest_save_to_memcard(
                p1_profile_konquest->fields.region_arg, 1, 7);
        } else {
            unload_player_profiles();
        }
        if (save_succeeded != 0) {
            game_speed = original_game_speed;
            gamelogic_jump(6, p_main_menu);
        } else {
            target_game_mode = 0x18;
            fade_from_black(8, 1);
        }
    }
    if (get_game_state() != 0x14) {
        konquest_show_hud();
    }
    pause_procs(0);
    eat_switch_edge(konquest_pdata->input_port, 5);
    unpause_all_game_sounds();
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
    return -1.0f;
}

int get_save_progress_flag(void) {
    return konquest_save_on_exit;
}

#pragma optimize_for_size on
void set_save_progress_flag(int enabled) {
    konquest_save_on_exit = enabled != 0;
}
#pragma optimize_for_size reset

void npc_hide_skip_message(void) {
    del_string_obj_by_id(0x900f);
}

#pragma optimize_for_size on
void npc_show_skip_message(void) {
    char* text;

    text = get_string_by_id(0x10001);
    load_font(6);
    string_center_xy(0x900f, 6, text, screen_width / 2, 0x1a1, 0xb);
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: p_konquest_switch_4 ~99.6% -- the complete 0x198-byte
 * instruction stream matches; objdiff only assigns different local labels to
 * the eight loads of the shared -1.0f value.
 */
float p_konquest_switch_4(void) {
    KonquestSwitchState* input;
    int hero_state;

    input = switch_pdata->state;
    if (input == 0) {
        return -1.0f;
    }
    if (konquest_editor_mode_on != 0) {
        return -1.0f;
    }
    if (input->input_port != konquest_pdata->input_port) {
        return -1.0f;
    }

    if (konquest_current_game_mode() != 0 &&
        konquest_current_game_mode() != 4) {
        return -1.0f;
    }
    if (get_game_state() == 0x14) {
        return -1.0f;
    }
    if (konquest_pdata->hero_state == 10 ||
        find_mkproc_pid(0x8233) != 0) {
        return -1.0f;
    }

    hero_state = konquest_pdata->hero_state;
    if (hero_state == 8 || hero_state == 9) {
        if (hero_state == 9) {
            if (konquest_pdata->interior_active == 0) {
                exit_meditation();
            }
        } else if (hero_state != 10 &&
                   monk_state_data[10].transition_order >=
                       monk_state_data[hero_state].transition_order) {
            konquest_pdata->hero_state = 10;
        }
        return -1.0f;
    }
    if (hero_state != 8 &&
        monk_state_data[8].transition_order >=
            monk_state_data[hero_state].transition_order) {
        konquest_pdata->hero_state = 8;
    }
    return -1.0f;
}

/*
 * Soft ceiling: p_konquest_switch_3 ~98.4% at the exact retail size. The
 * mode logic, NPC interaction, trigger scan, latch cleanup, state updates,
 * calls, and access widths match. Residue is one equivalent hero-latch branch,
 * one exhausted-list-cursor branch/register form, local floating-register
 * scheduling, and constant relocation labels.
 */
float p_konquest_switch_3(void) {
    KonquestSwitchState* input;
    KonquestPdata* pdata;
    KonquestNpc* npc;
    int can_interact;
    MkPtr* link;
    KonquestTriggerStruct* trigger;
    MkObj* hero;
    MkPtr** trigger_list;
    int game_mode_index;
    float distance;
    float facing_angle;
    float checked_facing_angle;

    input = switch_pdata->state;
    if (input == 0) {
        return -1.0f;
    }
    pdata = konquest_pdata;
    if (pdata == 0) {
        return -1.0f;
    }
    if (konquest_editor_mode_on != 0) {
        return -1.0f;
    }
    if (input->input_port != pdata->input_port) {
        return -1.0f;
    }

    game_mode_index = pdata->game_mode_index;
    if (konquest_current_game_mode() == 4 ||
        konquest_current_game_mode() == 5 || pdata->hero_state == 8 ||
        pdata->hero_state == 9 || pdata->hero_state == 10) {
        return -1.0f;
    }

    switch (konquest_current_game_mode()) {
    case 0:
        can_interact = 0;
        npc = konquest_check_possible_interact_with_npc(
            &distance, &facing_angle);
        if (npc != 0) {
            int in_range;

            checked_facing_angle = facing_angle;
            if (distance < 2.0f && checked_facing_angle < 0.8f) {
                in_range = 1;
            } else {
                in_range = 0;
            }
            if (in_range != 0) {
                can_interact = 1;
            }
        }

        pdata = konquest_pdata;
        hero = pdata->hero_object;
        if (hero != 0) {
            if (hero->hdr.instance != pdata->hero_instance) {
                hero = 0;
            }
        } else {
            hero = 0;
        }

        if (hero == 0) {
            link = 0;
        } else {
            trigger_list = &pdata->temporary_triggers;
            if (trigger_list != 0) {
                link = *trigger_list;
                while (link != 0) {
                    trigger = (KonquestTriggerStruct*)link->hdr;
                    if (link->instance != trigger->hdr.instance) {
                        MkPtr* next;

                        next = link->next;
                        link->hdr = 0;
                        destroy_mkptr(link);
                        link = next;
                        continue;
                    }
                    if (trigger->owned_data->state != 0 &&
                        (trigger->owned_data->flags & 2) != 0) {
                        float delta_x;
                        float delta_z;

                        delta_z = trigger->owned_data->position.z -
                                  hero->pos.value.z;
                        delta_x = trigger->owned_data->position.x -
                                  hero->pos.value.x;
                        if (delta_x * delta_x + delta_z * delta_z <
                                trigger->owned_data->radius *
                                    trigger->owned_data->radius &&
                            check_additional_trigger_fire_requirements(
                                trigger, hero)) {
                            trigger->flag_bits.pressed = 1;
                        }
                    }
                    link = link->next;
                }
            } else {
                link = 0;
            }
        }
        if (link == 0 && can_interact == 1) {
            konquest_pdata->npc_interaction_state = 0;
            npc_signal_event(npc, 3);
        }
        break;
    case 1: {
        MkProc* proc;

        proc = find_mkproc_pid(0xA012);
        if (proc != 0) {
            KonquestSwitchModePdata* proc_pdata;

            proc_pdata = (KonquestSwitchModePdata*)pdata_of_proc(proc);
            proc_pdata->switch_pressed = 1;
            if (konquest_pdata != 0) {
                konquest_pdata->flag_bits.bit4 = 1;
            }
        }
        break;
    }
    case 2:
    case 7:
        if (game_mode_index != 0) {
            pdata->game_mode_index--;
        }
        break;
    case 6:
        if (game_mode_index < 0 || konquest_current_game_mode() != 12) {
            if (game_mode_index < 3) {
                pdata->game_mode_index++;
                konquest_pdata
                    ->game_modes[konquest_pdata->game_mode_index] = 12;
            }
        }
        break;
    }
    return -1.0f;
}

/*
 * Soft ceiling: p_konquest_switch_R1 ~99.6% -- every executable instruction
 * matches; objdiff only assigns a different local label to the -1.0f value.
 */
float p_konquest_switch_R1(void) {
    KonquestR1Pdata* pdata;
    MkProc* proc;

    if (konquest_current_game_mode() != 0) {
        return -1.0f;
    }
    if (find_mkproc_pid(0x2098) != 0) {
        return -1.0f;
    }
    if (find_mkproc_pid(0x8239) != 0) {
        return -1.0f;
    }
    if (find_mkproc_pid(0x208C) != 0) {
        return -1.0f;
    }
    pdata = (KonquestR1Pdata*)apdata;
    if (pdata != 0 &&
        pdata->player !=
            g_game_info.pads[konquest_pdata->input_port].player) {
        return -1.0f;
    }
    if (konquest_pdata->region_table->map_table == 0) {
        return -1.0f;
    }
    proc = (MkProc*)_create_mkproc_generic_bigstack(
        0x8244, 0x1F, p_konquest_map_screen, 0, 0);
    if (proc != 0) {
        proc->flags_bits.skip_if_paused = 1;
        pause_procs(1);
    }
    return -1.0f;
}

/*
 * Soft ceiling: p_konquest_switch_1 99.44%, with exact size and control flow.
 * All five remaining differences are relocation labels for the shared -1.0f
 * return constant.
 */
float p_konquest_switch_1(void) {
    KonquestSwitchState* input;
    int game_mode;

    input = switch_pdata->state;
    if (input == 0) {
        return -1.0f;
    }
    if (konquest_editor_mode_on != 0) {
        return -1.0f;
    }
    if (input->input_port != konquest_pdata->input_port) {
        return -1.0f;
    }

    game_mode = konquest_current_game_mode();
    if (game_mode != 0) {
        return -1.0f;
    }

    if (konquest_pdata->hero_state != 3 &&
        monk_state_data[3].transition_order >=
            monk_state_data[konquest_pdata->hero_state].transition_order) {
        konquest_pdata->hero_state = 3;
    }
    return -1.0f;
}

static int check_additional_trigger_fire_requirements(
    KonquestTriggerStruct* owner, MkObj* hero) {
    KonquestTriggerRequirement* requirement;
    KonquestTriggerOrientation* orientation;
    KonquestTriggerAngle* angle;
    float delta_x;
    float delta_z;
    float length_squared;
    float inverse_length;
    float product;
    float correction;
    float forward_x;
    float forward_z;
    float forward_length_squared;
    float inverse_forward_length;
    float forward_product;
    float forward_correction;
    float facing_angle;
    float sine;
    KonquestFloatBits estimate;
    KonquestFloatBits forward_estimate;
    int result;

    requirement = owner->requirement;
    result = 0;
    switch (requirement->type) {
    case 3:
        delta_z = requirement->position.z - hero->pos.value.z;
        delta_x = requirement->position.x - hero->pos.value.x;
        length_squared = delta_x * delta_x + delta_z * delta_z;
        if (length_squared <= 0.0f) {
            inverse_length = 0.0f;
        } else {
            estimate.value = length_squared;
            estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
            product = estimate.value * (length_squared * estimate.value);
            correction = 3.0f - product;
            inverse_length =
                0.0625f * estimate.value * correction *
                -(correction * (product * correction) - 12.0f);
        }
        delta_x *= inverse_length;
        delta_z *= inverse_length;

        forward_x = hero->field_24->at.x;
        forward_z = hero->field_24->at.z;
        forward_length_squared =
            forward_x * forward_x + forward_z * forward_z;
        if (forward_length_squared <= 0.0f) {
            inverse_forward_length = 0.0f;
        } else {
            forward_estimate.value = forward_length_squared;
            forward_estimate.bits =
                0x5F375A00 - (forward_estimate.bits >> 1);
            forward_product =
                forward_estimate.value *
                (forward_length_squared * forward_estimate.value);
            forward_correction = 3.0f - forward_product;
            inverse_forward_length =
                0.0625f * forward_estimate.value * forward_correction *
                -(forward_correction *
                      (forward_product * forward_correction) -
                  12.0f);
        }

        forward_z *= inverse_forward_length;
        forward_x *= inverse_forward_length;
        if (gxMathArcCos(
                delta_x * forward_x + delta_z * forward_z) < 0.7853982f) {
            result = 1;
        }
        break;
    case 2:
        orientation = requirement->orientation;
        if (orientation != 0) {
            delta_z = requirement->position.z - hero->pos.value.z;
            delta_x = requirement->position.x - hero->pos.value.x;
            length_squared = delta_x * delta_x + delta_z * delta_z;
            if (length_squared <= 0.0f) {
                inverse_length = 0.0f;
            } else {
                estimate.value = length_squared;
                estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
                product =
                    estimate.value * (length_squared * estimate.value);
                correction = 3.0f - product;
                inverse_length =
                    0.0625f * estimate.value * correction *
                    -(correction * (product * correction) - 12.0f);
            }
            delta_x *= inverse_length;
            delta_z *= inverse_length;

            forward_x = hero->field_24->at.x;
            forward_z = hero->field_24->at.z;
            forward_length_squared =
                forward_x * forward_x + forward_z * forward_z;
            if (forward_length_squared <= 0.0f) {
                inverse_forward_length = 0.0f;
            } else {
                forward_estimate.value = forward_length_squared;
                forward_estimate.bits =
                    0x5F375A00 - (forward_estimate.bits >> 1);
                forward_product =
                    forward_estimate.value *
                    (forward_length_squared * forward_estimate.value);
                forward_correction = 3.0f - forward_product;
                inverse_forward_length =
                    0.0625f * forward_estimate.value *
                    forward_correction *
                    -(forward_correction *
                          (forward_product * forward_correction) -
                      12.0f);
            }

            forward_z *= inverse_forward_length;
            forward_x *= inverse_forward_length;
            facing_angle = gxMathArcCos(
                delta_x * forward_x + delta_z * forward_z);
            if (!(facing_angle >= 0.7853982f)) {
                angle = orientation->angle;
                sine = gxMathSin(angle->angle);
                if ((owner->requirement->position.z - hero->pos.value.z) *
                            gxMathCos(angle->angle) +
                        (owner->requirement->position.x - hero->pos.value.x) *
                            sine +
                        (owner->requirement->position.y - hero->pos.value.y) *
                            0.0f <
                    0.0f) {
                    result = 1;
                }
            }
        }
        break;
    default:
        result = 1;
        break;
    }
    return result;
}

/*
 * Near match: handle_monk_input 96.69% (1188 versus 1184 bytes). The mode
 * gate, dead-zone remap, normalized camera-relative movement, turn step,
 * walk/run state transitions, animation rates, and both stop paths agree.
 * Residue is two folded latch joins, individual versus grouped GPR saves,
 * local FPR scheduling, and pooled floating-constant labels.
 */
static void handle_monk_input(void) {
    MkObj* hero;
    CameraObj* camera;
    Vec direction;
    float horizontal;
    float vertical;
    float dead_zone;
    float active_range;
    float stick_length;
    float magnitude;
    float target_angle;
    float angle_difference;
    float angle_magnitude;
    int mode_index;
    int run_pressed;
    int state;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    mode_index = konquest_pdata->game_mode_index;
    if ((mode_index < 0 ? 0 : konquest_pdata->game_modes[mode_index]) != 0) {
        return;
    }
    if (get_stick_pos(
            konquest_pdata->input_port, 0, &horizontal, &vertical) != 0) {
        dead_zone = (float)stick_dead_zone * 0.0078125f;
        direction = (Vec){0.0f, 0.0f, 0.0f};
        active_range = 1.0f - dead_zone;
        if (horizontal > 0.0f) {
            direction.x = (horizontal - dead_zone) / active_range;
        } else if (horizontal < 0.0f) {
            direction.x = (horizontal + dead_zone) / active_range;
        }
        if (vertical > 0.0f) {
            direction.y = (-vertical + dead_zone) / active_range;
        } else if (vertical < 0.0f) {
            direction.y = (-vertical - dead_zone) / active_range;
        }

        stick_length = normalize_v3_length(&direction);
        magnitude = 1.0f;
        if (stick_length <= 1.0f) {
            magnitude = stick_length;
        }
        camera = camera_item.node;
        if (camera != 0) {
            if (camera->hdr.instance == camera_item.instance) {
                /* Valid camera latch. */
            } else {
                camera = 0;
            }
        } else {
            camera = 0;
        }

        if (magnitude > 0.01f) {
            konquest_pdata->npc_interaction_state = 0;
            target_angle = norm_angle(
                camera->ang.y +
                ((float)atan2(direction.y, direction.x) - 1.5707964f));
            angle_difference = hero->ang.y - target_angle;
            angle_magnitude = angle_difference >= 0.0f
                                  ? angle_difference
                                  : -angle_difference;
            if (angle_magnitude <= 0.2f) {
                hero->ang.y = target_angle;
            } else if (angle_difference < 0.0f) {
                if (angle_difference < -3.1415927f) {
                    hero->ang.y -= 0.2f;
                } else {
                    hero->ang.y += 0.2f;
                }
            } else if (angle_difference > 3.1415927f) {
                hero->ang.y += 0.2f;
            } else {
                hero->ang.y -= 0.2f;
            }
            update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);

            run_pressed = check_switch(konquest_pdata->input_port, 1);
            if (magnitude < 0.625f) {
                state = konquest_pdata->hero_state;
                if (state != 1 &&
                    monk_state_data[1].transition_order >=
                        monk_state_data[state].transition_order) {
                    konquest_pdata->hero_state = 1;
                }
                monk_state_data[1].animation_step = 0.5f + magnitude;
                konquest_pdata->hero_anim->weight = 1.0f;
            } else {
                state = konquest_pdata->hero_state;
                if (state != 2 &&
                    monk_state_data[2].transition_order >=
                        monk_state_data[state].transition_order) {
                    konquest_pdata->hero_state = 2;
                }
                if (run_pressed != 0) {
                    float animation_speed = 1.8f * magnitude;

                    konquest_pdata->hero_anim->weight = animation_speed;
                    monk_state_data[2].animation_step = animation_speed;
                } else {
                    monk_state_data[2].animation_step = 1.5f * magnitude;
                    konquest_pdata->hero_anim->weight = 1.2f * magnitude;
                }
                hero->flags_09_bits.bit6 = 0;
            }
            hero->hide_flag_bits.still_move = 0;
            correct_camera_angle(hero, 0);
            return;
        }
        if (konquest_pdata->npc_interaction_state == 0 && hero != 0) {
            hero->pos_vel.z = 0.0f;
            hero->pos_vel.y = 0.0f;
            hero->pos_vel.x = 0.0f;
            hero->flags_09_bits.bit6 = 1;
            konquest_pdata->hero_state = 0;
            hero->hide_flag_bits.still_move = 1;
            konquest_pdata->hero_anim->weight = 1.0f;
        }
    } else {
        if (hero != 0) {
            hero->ang_vel.z = 0.0f;
            hero->ang_vel.y = 0.0f;
            hero->ang_vel.x = 0.0f;
        }
        if (hero != 0) {
            hero->pos_vel.z = 0.0f;
            hero->pos_vel.y = 0.0f;
            hero->pos_vel.x = 0.0f;
            hero->flags_09_bits.bit6 = 1;
            konquest_pdata->hero_state = 0;
            hero->hide_flag_bits.still_move = 1;
            konquest_pdata->hero_anim->weight = 1.0f;
        }
    }
}

/*
 * Near match: the complete controller algorithm, repeated mode/state and
 * camera queries, stick out-parameter ABI, camera math, and state filter agree
 * with retail. The eight-byte residue is two folded join branches plus local
 * floating-constant relocation labels.
 */
void handle_controller_input(void) {
    MkObj* hero;
    CameraPdata* camera;
    float horizontal;
    float vertical;
    float horizontal_input;
    float vertical_input;
    float magnitude;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    if (konquest_current_game_mode() != 0 &&
        konquest_current_game_mode() != 4) {
        return;
    }
    if (get_game_state() == 0x14) {
        return;
    }
    if (get_game_state() == 0x15) {
        return;
    }

    if (check_switch(konquest_pdata->input_port, 2) != 0) {
        correct_camera_angle(hero, 1);
    }
    if (get_stick_pos(
            konquest_pdata->input_port, 1, &horizontal, &vertical) == 0) {
        return;
    }

    horizontal_input = horizontal;
    magnitude = horizontal_input >= 0.0f
                    ? horizontal_input
                    : -horizontal_input;
    if (magnitude > 0.01f) {
        camera = get_pdata_of_camera();
        if (camera != 0) {
            camera->target_ang.y = norm_angle(
                camera->target_ang.y - 0.035f * horizontal_input);
            if (konquest_pdata->hero_state == 0 &&
                konquest_pdata->npc_interaction_state == 0) {
                konquest_pdata->npc_interaction_state = 1;
            }
        }
    }

    vertical_input = vertical;
    camera = get_pdata_of_camera();
    if (camera == 0) {
        return;
    }
    if (konquest_pdata->hero_state != 0) {
        if (konquest_pdata->hero_state != 4) {
            if (konquest_pdata->hero_state != 5) {
                if (konquest_pdata->hero_state != 6) {
                    if (konquest_pdata->hero_state != 7 &&
                        konquest_pdata->hero_state != 9) {
                        return;
                    }
                }
            }
        }
    }
    camera->target_ang.x = norm_angle(0.3f * vertical_input + 0.17f);
}

/* Retail calls this symbol from turn_to_face_exterior_door. */
#pragma dont_inline on
void hero_stop_moving(void) {
    MkObj* hero;
    MonkStateData* state;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    if (hero != 0) {
        hero->pos_vel.z = 0.0f;
        hero->pos_vel.y = 0.0f;
        hero->pos_vel.x = 0.0f;
        hero->flags_09 |= 0x40;
        konquest_pdata->hero_state = 0;
        hero->hide_flags |= 0x80;
        konquest_pdata->hero_anim->weight = 1.0f;
    }

    state = &monk_state_data[konquest_pdata->hero_state];
    transition_to_anim_script(
        konquest_pdata->hero_anim, state->animation, state->transition, 0.1f);

    if (aproc != 0 && aproc->pid != 0xFFFFA014 &&
        aproc->pid != 0xFFFFA002) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
}
#pragma dont_inline reset

#pragma dont_inline on
static void correct_camera_angle(MkObj* hero, int snap) {
    CameraPdata* camera;
    float margin;
    float hero_angle;
    float difference;
    float magnitude;

    margin = 0.75f;
    camera = get_pdata_of_camera();
    if (camera == 0) {
        return;
    }

    camera->target_ang.x = 0.17f;
    if (snap != 0) {
        margin = 0.0f;
    }

    hero_angle = hero->ang.y;
    difference = hero_angle - camera->target_ang.y;
    magnitude = difference >= 0.0f ? difference : -difference;
    if (magnitude <= 0.01f) {
        camera->target_ang.y = hero_angle;
    } else if (difference < 0.0f) {
        if (difference <= -3.1415927f - margin) {
            camera->target_ang.y +=
                (0.075f * (6.2831855f + difference)) / 3.1415927f;
        } else if (difference > -3.1415927f + margin) {
            camera->target_ang.y -=
                (-0.075f * difference) / 3.1415927f;
        }
    } else {
        if (difference >= 3.1415927f + margin) {
            camera->target_ang.y -=
                (0.075f * (6.2831855f - difference)) / 3.1415927f;
        } else if (difference < 3.1415927f - margin) {
            camera->target_ang.y +=
                (0.075f * difference) / 3.1415927f;
        }
    }
    camera->target_ang.y = norm_angle(camera->target_ang.y);
}
#pragma dont_inline reset

static int adjust_light_for_new_time_of_day(
    RpLight* light, KonquestLightRow* light_table, int last_row) {
    RwRGBAReal* current_color;
    KonquestLightRow* previous_row;
    int current_index;
    int previous_index;
    int hour_span;
    float current_hour;
    float hour_ticks;
    float elapsed_ticks;
    float red_step;
    float green_step;
    float blue_step;
    float alpha_step;
    RwRGBAReal color;

    current_index = 0;
    current_hour = konquest_pdata->time_of_day;
    while ((float)light_table[current_index].hour < current_hour) {
        current_index++;
        if (current_index > last_row) {
            current_index = 0;
            break;
        }
    }

    previous_index = current_index - 1;
    if (previous_index < 0) {
        previous_index = last_row;
        hour_span = (24 - light_table[previous_index].hour) +
                    light_table[current_index].hour;
    } else {
        hour_span = light_table[current_index].hour -
                    light_table[previous_index].hour;
    }

    current_color = &light_table[current_index].color;
    previous_row = &light_table[previous_index];
    hour_ticks = (float)hour_span * ticks_per_hour;
    red_step =
        (current_color->red - previous_row->color.red) / hour_ticks;
    green_step =
        (current_color->green - previous_row->color.green) / hour_ticks;
    blue_step =
        (current_color->blue - previous_row->color.blue) / hour_ticks;
    alpha_step =
        (current_color->alpha - previous_row->color.alpha) / hour_ticks;

    if (current_hour < (float)previous_row->hour) {
        elapsed_ticks =
            (current_hour + (float)(24 - previous_row->hour)) *
            ticks_per_hour;
    } else {
        elapsed_ticks =
            (current_hour - (float)previous_row->hour) * ticks_per_hour;
    }

    color.red = konquest_pdata->sky_color_multiplier *
                (red_step * elapsed_ticks + previous_row->color.red);
    if (color.red > 1.0f) {
        color.red = 1.0f;
    } else if (color.red < 0.0f) {
        color.red = 0.0f;
    }
    color.green = konquest_pdata->sky_color_multiplier *
                  (green_step * elapsed_ticks + previous_row->color.green);
    if (color.green > 1.0f) {
        color.green = 1.0f;
    } else if (color.green < 0.0f) {
        color.green = 0.0f;
    }
    color.blue = konquest_pdata->sky_color_multiplier *
                 (blue_step * elapsed_ticks + previous_row->color.blue);
    if (color.blue > 1.0f) {
        color.blue = 1.0f;
    } else if (color.blue < 0.0f) {
        color.blue = 0.0f;
    }
    color.alpha =
        alpha_step * elapsed_ticks + previous_row->color.alpha;
    if (color.alpha > 1.0f) {
        color.alpha = 1.0f;
    } else if (color.alpha < 0.0f) {
        color.alpha = 0.0f;
    }

    RpLightSetColor(light, &color);
    return current_index;
}

/*
 * Near match: row selection, midnight wrapping, four-channel interpolation,
 * clamping, and background-color publication agree with retail. The remaining
 * 20-byte emission delta is typed-pointer induction and local float scheduling.
 */
static int adjust_sky_for_new_time_of_day(
    KonquestSkyRow* sky_table, int last_row) {
    KonquestSkyRow* current_row;
    KonquestSkyRow* previous_row;
    int current_index;
    int previous_index;
    int hour_span;
    float current_hour;
    float hour_ticks;
    float elapsed_ticks;
    float red_step;
    float green_step;
    float blue_step;
    float alpha_step;

    current_index = 0;
    current_hour = konquest_pdata->time_of_day;
    while ((float)sky_table[current_index].hour < current_hour) {
        current_index++;
        if (current_index > last_row) {
            current_index = 0;
            break;
        }
    }

    previous_index = current_index - 1;
    if (previous_index < 0) {
        previous_index = last_row;
        hour_span = (24 - sky_table[previous_index].hour) +
                    sky_table[current_index].hour;
    } else {
        hour_span = sky_table[current_index].hour -
                    sky_table[previous_index].hour;
    }

    current_row = &sky_table[current_index];
    previous_row = &sky_table[previous_index];
    hour_ticks = (float)hour_span * ticks_per_hour;
    red_step = (current_row->red - previous_row->red) / hour_ticks;
    green_step =
        (current_row->green - previous_row->green) / hour_ticks;
    blue_step = (current_row->blue - previous_row->blue) / hour_ticks;
    alpha_step =
        (current_row->background_alpha - previous_row->background_alpha) /
        hour_ticks;

    if (current_hour < (float)previous_row->hour) {
        elapsed_ticks =
            (current_hour + (float)(24 - previous_row->hour)) *
            ticks_per_hour;
    } else {
        elapsed_ticks =
            (current_hour - (float)previous_row->hour) * ticks_per_hour;
    }

    fog_color_real[0] = konquest_pdata->sky_color_multiplier *
                        (red_step * elapsed_ticks + previous_row->red);
    if (fog_color_real[0] > 1.0f) {
        fog_color_real[0] = 1.0f;
    } else if (fog_color_real[0] < 0.0f) {
        fog_color_real[0] = 0.0f;
    }

    fog_color_real[1] = konquest_pdata->sky_color_multiplier *
                        (green_step * elapsed_ticks + previous_row->green);
    if (fog_color_real[1] > 1.0f) {
        fog_color_real[1] = 1.0f;
    } else if (fog_color_real[1] < 0.0f) {
        fog_color_real[1] = 0.0f;
    }

    fog_color_real[2] = konquest_pdata->sky_color_multiplier *
                        (blue_step * elapsed_ticks + previous_row->blue);
    if (fog_color_real[2] > 1.0f) {
        fog_color_real[2] = 1.0f;
    } else if (fog_color_real[2] < 0.0f) {
        fog_color_real[2] = 0.0f;
    }

    fog_color_real[3] =
        alpha_step * elapsed_ticks + previous_row->background_alpha;
    if (fog_color_real[3] > 1.0f) {
        fog_color_real[3] = 1.0f;
    } else if (fog_color_real[3] < 0.0f) {
        fog_color_real[3] = 0.0f;
    }

    set_background_color(
        (int)(255.0f * fog_color_real[0]),
        (int)(255.0f * fog_color_real[1]),
        (int)(255.0f * fog_color_real[2]),
        (int)(255.0f * fog_color_real[3]));
    return current_index;
}

/*
 * Near match at retail size: the complete sky-alpha loop and calls agree.
 * Residue is loop-invariant last-row offset hoisting, register coloring, and
 * local constant relocation labels.
 */
static float p_adjust_sky(void) {
    KonquestSkyRow* sky_table;
    MkSobj* sky_object;
    RpMaterial* material;
    int last_row;
    int current_index;
    int previous_index;
    int hour_span;
    float current_hour;
    float elapsed_hours;
    float alpha_step;
    float alpha;

    sky_table = (KonquestSkyRow*)konquest_pdata->region_table->sky_table;
    last_row = get_row_count_for_table_by_pointer(
                   konquest_pdata->script_owner, sky_table) -
               1;
    while (aproc != 0) {
        current_index = adjust_sky_for_new_time_of_day(sky_table, last_row);
        previous_index = current_index - 1;
        if (previous_index < 0) {
            previous_index = last_row;
            hour_span = (24 - sky_table[previous_index].hour) +
                        sky_table[current_index].hour;
        } else {
            hour_span = sky_table[current_index].hour -
                        sky_table[previous_index].hour;
        }

        alpha_step =
            (sky_table[current_index].alpha -
             sky_table[previous_index].alpha) /
            ((float)hour_span * ticks_per_hour);
        current_hour = konquest_pdata->time_of_day;
        if (current_hour < (float)sky_table[previous_index].hour) {
            elapsed_hours =
                current_hour +
                (float)(24 - sky_table[previous_index].hour);
        } else {
            elapsed_hours =
                current_hour - (float)sky_table[previous_index].hour;
        }
        alpha = alpha_step * (elapsed_hours * ticks_per_hour) +
                sky_table[previous_index].alpha;

        sky_object = obj_find_sobj_by_id(g_game_info.sky, 10);
        if (sky_object != 0) {
            if (alpha == 0.0f) {
                hide_sobj(sky_object);
            } else {
                unhide_sobj(sky_object);
            }
            material = sobj_find_material_by_id(sky_object, 1);
            if (material != 0) {
                material->color.alpha = (unsigned char)(255.0f * alpha);
            }
        }

        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

/*
 * Soft ceiling for both ambient-light workers: table selection, row-count
 * bounds, typed process pdata, update loop, sleep, and return match retail.
 * Their 16-byte residue is solely individual nonvolatile saves/restores in
 * place of CodeWarrior's stmw/lmw pair, plus harmless register coloring.
 */
float p_adjust_sky_ambient_light(void) {
    KonquestLightRow* light_table;
    int last_row;
    KonquestLightAdjustPdata* pdata;

    light_table = konquest_pdata->region_table->sky_ambient_light_table;
    last_row = get_row_count_for_table_by_pointer(
                   konquest_pdata->script_owner, light_table) -
               1;
    pdata = (KonquestLightAdjustPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    while (pdata != 0) {
        adjust_light_for_new_time_of_day(
            pdata->light, light_table, last_row);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

float p_adjust_ambient_light(void) {
    KonquestLightRow* light_table;
    int last_row;
    KonquestLightAdjustPdata* pdata;

    light_table = konquest_pdata->region_table->ambient_light_table;
    last_row = get_row_count_for_table_by_pointer(
                   konquest_pdata->script_owner, light_table) -
               1;
    pdata = (KonquestLightAdjustPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    while (pdata != 0) {
        adjust_light_for_new_time_of_day(
            pdata->light, light_table, last_row);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

/*
 * Soft ceiling: update_sun_moon_position 96.51% (1052 versus 1036 bytes).
 * Both day/night paths, four independent fade-object lookups, material alpha
 * updates, transforms, and visibility calls agree. Residue is save/restore,
 * register allocation, and local constant scheduling.
 */
static void update_sun_moon_position(float angle) {
    Vec direction = {0.0f, 0.0f, 1.0f};
    Vec rotated;
    MkSobj* sun;
    MkSobj* moon;
    MkSobj* fade_object;
    RpMaterial* material;
    float normalized;
    float alpha;

    sun = obj_find_sobj_by_id(g_game_info.sky, 0x28);
    moon = obj_find_sobj_by_id(g_game_info.sky, 0x1E);
    if (sun == 0 || moon == 0) {
        return;
    }

    if (konquest_pdata->time_of_day >= 6.0f &&
        konquest_pdata->time_of_day < 18.0f) {
        normalized = norm_angle(angle);
        hide_sobj(moon);
        sun->ang.x = 0.2f;
        sun->ang.y = normalized;
        rotate_xz(&rotated, &direction, normalized);
        sun->pos.x = -400.0f * rotated.x;
        sun->pos.y = -400.0f * rotated.y;
        sun->pos.z = -400.0f * rotated.z;
        sun->pos.y = -125.0f * gxMathCos(normalized);
        update_mksobj(sun);
        unhide_sobj(sun);

        if (konquest_pdata->time_of_day >= 6.0f &&
            konquest_pdata->time_of_day < 7.0f) {
            alpha = (4.712385f - normalized) / 0.2618f;
            fade_object = obj_find_sobj_by_id(g_game_info.sky, 0x28);
            if (fade_object != 0) {
                if (alpha == 0.0f) {
                    hide_sobj(fade_object);
                } else {
                    unhide_sobj(fade_object);
                }
                material = sobj_find_material_by_id(fade_object, 3);
                if (material != 0) {
                    material->color.alpha = (unsigned char)(255.0f * alpha);
                }
            }
        }
        if (konquest_pdata->time_of_day >= 17.0f &&
            konquest_pdata->time_of_day < 18.0f) {
            alpha = 1.0f - ((1.8326f - normalized) / 0.2618f);
            fade_object = obj_find_sobj_by_id(g_game_info.sky, 0x28);
            if (fade_object != 0) {
                if (alpha == 0.0f) {
                    hide_sobj(fade_object);
                } else {
                    unhide_sobj(fade_object);
                }
                material = sobj_find_material_by_id(fade_object, 3);
                if (material != 0) {
                    material->color.alpha = (unsigned char)(255.0f * alpha);
                }
            }
        }
    } else {
        normalized = norm_angle(3.1415927f + angle);
        hide_sobj(sun);
        moon->ang.x = 0.2f;
        moon->ang.y = normalized;
        rotate_xz(&rotated, &direction, normalized);
        moon->pos.x = -400.0f * rotated.x;
        moon->pos.y = -400.0f * rotated.y;
        moon->pos.z = -400.0f * rotated.z;
        moon->pos.y = -115.0f * gxMathCos(normalized);
        update_mksobj(moon);
        unhide_sobj(moon);

        if (konquest_pdata->time_of_day >= 18.0f &&
            konquest_pdata->time_of_day < 19.0f) {
            alpha = (4.712385f - normalized) / 0.2618f;
            fade_object = obj_find_sobj_by_id(g_game_info.sky, 0x1E);
            if (fade_object != 0) {
                if (alpha == 0.0f) {
                    hide_sobj(fade_object);
                } else {
                    unhide_sobj(fade_object);
                }
                material = sobj_find_material_by_id(fade_object, 2);
                if (material != 0) {
                    material->color.alpha = (unsigned char)(255.0f * alpha);
                }
            }
        }
        if (konquest_pdata->time_of_day >= 5.0f &&
            konquest_pdata->time_of_day < 6.0f) {
            alpha = 1.0f - ((1.8326f - normalized) / 0.2618f);
            fade_object = obj_find_sobj_by_id(g_game_info.sky, 0x1E);
            if (fade_object != 0) {
                if (alpha == 0.0f) {
                    hide_sobj(fade_object);
                } else {
                    unhide_sobj(fade_object);
                }
                material = sobj_find_material_by_id(fade_object, 2);
                if (material != 0) {
                    material->color.alpha = (unsigned char)(255.0f * alpha);
                }
            }
        }
    }
}

/*
 * Soft ceiling: p_adjust_directional_light 81.65% (840 versus 856 bytes).
 * The table interpolation, shadow strength/alpha, sun/moon update, angle
 * correction, camera-light calls, and sleep loop have the exact retail call
 * sequence. Residue is FPR/GPR allocation and arithmetic scheduling.
 */
float p_adjust_directional_light(void) {
    KonquestDirectionalLightRow* light_table;
    KonquestLightAdjustPdata* pdata;
    MkSobj* object;
    Vec angles = {0.0f, 0.0f, 0.0f};
    int last_row;
    int current_index;
    int previous_index;
    int hour_span;
    float elapsed_hours;
    float strength_step;

    light_table = (KonquestDirectionalLightRow*)
        konquest_pdata->region_table->directional_light_table;
    last_row = get_row_count_for_table_by_pointer(
                   konquest_pdata->script_owner, light_table) - 1;
    pdata = (KonquestLightAdjustPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    object = pdata->object;
    if (object != 0) {
        if (object->hdr.instance != pdata->object_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object == 0) {
        return -1.0f;
    }

    while (pdata != 0 && object != 0) {
        current_index = adjust_light_for_new_time_of_day(
            pdata->light, (KonquestLightRow*)light_table, last_row);
        previous_index = current_index - 1;
        if (previous_index < 0) {
            previous_index = get_row_count_for_table_by_pointer(
                                 konquest_pdata->script_owner, light_table) - 1;
            hour_span = (24 - light_table[previous_index].hour) +
                        light_table[current_index].hour;
        } else {
            hour_span = light_table[current_index].hour -
                        light_table[previous_index].hour;
        }
        strength_step =
            (light_table[current_index].strength -
             light_table[previous_index].strength) /
            ((float)hour_span * ticks_per_hour);
        if (konquest_pdata->time_of_day <
            (float)light_table[previous_index].hour) {
            elapsed_hours = konquest_pdata->time_of_day +
                (float)(24 - light_table[previous_index].hour);
        } else {
            elapsed_hours = konquest_pdata->time_of_day -
                (float)light_table[previous_index].hour;
        }
        konquest_pdata->directional_light_strength =
            konquest_pdata->sky_color_multiplier *
            (strength_step * (elapsed_hours * ticks_per_hour) +
             light_table[previous_index].strength);
        ShadowStrength = konquest_pdata->base_shadow_strength *
                         konquest_pdata->directional_light_strength;
        npc_shadow_set_alpha(
            (int)(255.0f * konquest_pdata->directional_light_strength));

        object->ang.y =
            3.1415927f * (2.0f * (-konquest_pdata->time_of_day / 24.0f));
        update_sun_moon_position(object->ang.y);
        if (konquest_pdata->time_of_day >= 19.0f ||
            konquest_pdata->time_of_day < 5.0f) {
            object->ang.y -= 3.1415927f;
        }
        update_mkobj(object != 0 ? as_mkhdr(&object->hdr) : 0);
        angles = object->ang;
        UpdateShadowCameraLightSource((const float*)&angles);
        npc_shadow_set_light_angle(&angles);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

/*
 * Near match: mode handling, hero latch, repel response, camera flag, cached
 * position, vertical ground probe, gravity, and all access widths agree with
 * retail. Residue is individual GPR saves/restores, one folded latch join,
 * and local floating-constant relocation labels.
 */
static float p_collide_monk(void) {
    CameraPdata* camera;
    MkObj* hero;
    Vec movement;
    Vec collision_position;
    Vec segment_end;
    Vec segment_start;
    Vec hit_point;

    if (konquest_current_game_mode() == 8) {
        return 1.0f;
    }
    camera = get_pdata_of_camera();
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    if (hero != 0) {
        if (old_hero_position.y == 500.0f) {
            movement.x = movement.y = movement.z = 0.0f;
        } else {
            movement.x = hero->pos.value.x - old_hero_position.x;
            movement.y = hero->pos.value.y - old_hero_position.y;
            movement.z = hero->pos.value.z - old_hero_position.z;
        }
        if (repel_against_global_collision_list(
                &old_hero_position, &movement, &collision_position) != 0) {
            hero->hide_flag_bits.pin_animation = 0;
            hero->pos.value.x = collision_position.x;
            hero->pos.value.y = collision_position.y;
            hero->pos.value.z = collision_position.z;
            if (camera != 0) {
                camera->flags_bits.pos_done = 0;
            }
        }

        old_hero_position.x = hero->pos.value.x;
        old_hero_position.y = hero->pos.value.y;
        old_hero_position.z = hero->pos.value.z;
        segment_end.x = hero->pos.value.x;
        segment_end.y = hero->pos.value.y;
        segment_end.z = hero->pos.value.z;
        segment_start.x = hero->pos.value.x;
        segment_start.y = hero->pos.value.y;
        segment_start.z = hero->pos.value.z;
        segment_end.y = -50.0f;
        segment_start.y = 50.0f;
        if (collide_segment_against_global_collision_list_quads(
                &segment_start, &segment_end, &hit_point) != 0) {
            hero->flags_08_bits.moving = 1;
            hero->gravity = -0.00125f;
            hero->hide_flag_bits.pin_animation = 0;
            hero->ground_colls_y = hit_point.y;
        } else {
            hero->ground_colls_y = 0.0f;
        }
    }
    return 1.0f;
}

/*
 * Near match at the exact retail size: unconscious timing, collision and
 * camera ownership, both game-speed ramps, animation loops, sound lifecycle,
 * flag transitions, and final control transfer agree. Residue is equivalent
 * latch/creation branch layout, individual versus grouped GPR saves, vtable
 * temporary coloring, and local floating-constant labels.
 */
static float p_monk_unconscious(void) {
    KonquestGameSpeedPdata* speed_pdata = 0;
    MkObj* hero;
    MkObj* camera_focus;
    float duration;
    float elapsed;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    elapsed = 0.0f;
    duration = (float)konquest_pdata->hero_unconscious;

    transition_to_anim_script(
        konquest_pdata->hero_anim,
        monk_state_data[konquest_pdata->hero_state].animation, 0, 0.05f);
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    if (konquest_pdata->collision_proc != 0) {
        xfer_proc(konquest_pdata->collision_proc, p_idle);
    }
    run_camera_script(
        konquest_pdata->script_owner,
        konquest_pdata->unconscious_camera_script, 1);

    camera_focus = konquest_pdata->hero_object;
    if (camera_focus != 0) {
        if (camera_focus->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            camera_focus = 0;
        }
    } else {
        camera_focus = 0;
    }
    if (camera_focus != 0) {
        camera_set_lookat_focus(camera_focus);
        camera_set_movement_focus_obj(camera_focus);
    }
    camera_setup_simple_rotation(500, 6.2831855f);
    snd_req(0x15BC);
    hero->flags_0B_bits.force_anim_speed = 1;

    if (_create_mkproc_generic_tinystack(
            0x8233, 0x1F, ramp_game_speed, sizeof(*speed_pdata),
            (void**)&speed_pdata) != 0 &&
        speed_pdata != 0) {
        if (refresh_rate() == 60) {
            speed_pdata->multiplier = 2.0f;
            speed_pdata->target_scale = (float)pow(2.0, 5.0);
        } else {
            speed_pdata->multiplier = 1.928f;
            speed_pdata->target_scale = (float)pow(1.928f, 5.0);
        }
    }

    meditate_stream = snd_req(0x15BB);
    while (elapsed < duration) {
        elapsed += game_hours_per_tick;
        advance_anim(konquest_pdata->hero_anim);
        pose_anim(konquest_pdata->hero_anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    snd_stop(meditate_stream);
    snd_req(0x15BD);
    if (_create_mkproc_generic_tinystack(
            0x8233, 0x1F, ramp_game_speed, sizeof(*speed_pdata),
            (void**)&speed_pdata) != 0 &&
        speed_pdata != 0) {
        if (refresh_rate() == 60) {
            speed_pdata->multiplier = 0.5f;
            speed_pdata->target_scale = 1.0f;
        } else {
            speed_pdata->multiplier = 0.5186722f;
            speed_pdata->target_scale = 1.2f;
        }
    }

    while (find_mkproc_pid(0x8233) != 0) {
        advance_anim(konquest_pdata->hero_anim);
        pose_anim(konquest_pdata->hero_anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    hero->flags_0B_bits.force_anim_speed = 0;
    transition_to_anim_script(
        konquest_pdata->hero_anim, (AniData*)konquest_animations[13], 3,
        0.05f);
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    while (konquest_pdata->hero_anim->frame <
           konquest_pdata->hero_anim->high_frame) {
        advance_anim(konquest_pdata->hero_anim);
        pose_anim(konquest_pdata->hero_anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_pdata->collision_proc != 0) {
        xfer_proc(konquest_pdata->collision_proc, p_collide_monk);
    }
    hero->ground_colls = monk_ground_colls;
    konquest_pdata->hero_state = 0;
    camera_exit_script();
    camera_set_glitch_flag();
    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_control_konquest_monk, aproc, 0.0f);
    return 0.0f;
}

/*
 * Near match: hero generation validation and both camera-focus calls match.
 * MWCC folds the retail latch's two empty join branches and duplicate null
 * assignment, making this readable form 12 bytes shorter.
 */
void set_camera_to_look_at_hero(void) {
    MkObj* hero;

    hero = konquest_pdata->hero_object;
    if (hero != 0 && hero->hdr.instance != konquest_pdata->hero_instance) {
        hero = 0;
    }
    if (hero != 0) {
        camera_set_lookat_focus(hero);
        camera_set_movement_focus_obj(hero);
    }
}

static float p_monk_getup(void) {
    MkObj* hero;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    transition_to_anim_script(
        konquest_pdata->hero_anim,
        monk_state_data[konquest_pdata->hero_state].animation, 3, 0.05f);
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    konquest_pdata->hero_anim->step = 0.5f;

    while (konquest_pdata->hero_anim->frame <
           konquest_pdata->hero_anim->high_frame) {
        hero->flags_08 |= 1;
        advance_anim(konquest_pdata->hero_anim);
        pose_anim(konquest_pdata->hero_anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    hero->ground_colls = monk_ground_colls;
    hero->flags_09 |= 0x40;
    konquest_pdata->hero_state = 0;
    if (konquest_pdata->game_mode_index != 0) {
        konquest_pdata->game_mode_index--;
    }
    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_control_konquest_monk, aproc, 0.0f);
    return 0.0f;
}

/*
 * Near match at the exact retail size: hero latch, mode-stack update,
 * animation and sound setup, ground-collision transition, both frame loops,
 * and terminal process transfer agree. Residue is one equivalent latch branch,
 * process-vtable temporary coloring, and local floating-constant labels.
 */
static float p_monk_punch_react(void) {
    MkObj* hero;
    int mode_index;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    mode_index = konquest_pdata->game_mode_index;
    if ((mode_index < 0 || konquest_current_game_mode() != 5) &&
        mode_index < 3) {
        konquest_pdata->game_mode_index++;
        konquest_pdata->game_modes[konquest_pdata->game_mode_index] = 5;
    }

    set_anim_script(
        konquest_pdata->hero_anim,
        monk_state_data[konquest_pdata->hero_state].animation, 3);
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    hero->flags_09_bits.bit6 = 0;
    hero->pos.value.y = konquest_pdata->hero_anim->anim_offset.y;
    konquest_pdata->hero_anim->step = 0.75f;
    random_snd_req(0x78);
    random_snd_req_delay(0x7D, 0x23);
    hero->flags_09_bits.bit6 = 0;
    hero->ground_colls = monk_laying_on_ground_colls;

    while (konquest_pdata->hero_anim->frame < 20.0f) {
        advance_anim(konquest_pdata->hero_anim);
        pose_anim(konquest_pdata->hero_anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    hero->flags_09_bits.bit6 = 1;
    while (konquest_pdata->hero_anim->frame < 50.0f) {
        advance_anim(konquest_pdata->hero_anim);
        pose_anim(konquest_pdata->hero_anim, 1);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_pdata->hero_unconscious != 0) {
        konquest_pdata->hero_state = 13;
        ((KonquestProcJumpVtable*)aproc->vtbl)
            ->jump_sleep(p_monk_unconscious, aproc, 0.0f);
        return 0.0f;
    }
    konquest_pdata->hero_state = 12;
    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_monk_getup, aproc, 0.0f);
    return 0.0f;
}

/*
 * Near match: retail operations, branches, access widths, and call ABI agree.
 * Remaining differences are the equivalent hero-latch branch, GPR save/restore
 * aggregation, a local float-load schedule, and pooled constant relocations.
 */
static float p_monk_punch(void) {
    MkObj* hero;
    KonquestNpc* npc;
    int checked_for_npc;
    int can_hit_npc;
    Vec direction;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    direction = (Vec){0.0f, 0.0f, 0.0f};
    npc = 0;
    can_hit_npc = 0;
    if (hero == 0) {
        return -1.0f;
    }

    konquest_pdata->npc_interaction_state = 0;
    hero->hide_flag_bits.still_move = 0;
    hero->pos_vel.z = 0.0f;
    hero->pos_vel.y = 0.0f;
    hero->pos_vel.x = 0.0f;
    {
        MonkStateData* state;

        state = &monk_state_data[konquest_pdata->hero_state];
        transition_to_anim_script(
            anim_pdata, state->animation, state->transition | 0x20, 0.2f);
    }
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    checked_for_npc = 0;
    anim_pdata->step =
        monk_state_data[konquest_pdata->hero_state].animation_step;
    random_snd_req_delay(0x7A, 10);
    if (p1_profile_konquest->fields.hero_age == 0) {
        random_snd_req_delay(0x80, 5);
    } else {
        random_snd_req_delay(0x81, 5);
    }

    while (konquest_pdata->hero_state == 3 && anim_pdata->frame <= 14.0f) {
        advance_anim(anim_pdata);
        pose_anim(anim_pdata, 1);
        if (checked_for_npc == 0) {
            float distance;
            float facing_angle;
            int in_range;

            can_hit_npc = 0;
            npc = konquest_check_possible_interact_with_npc(
                &distance, &facing_angle);
            if (npc != 0) {
                if (distance < 3.0f && facing_angle < 1.2f) {
                    in_range = 1;
                } else {
                    in_range = 0;
                }
                if (in_range != 0) {
                    can_hit_npc = 1;
                }
            }
            checked_for_npc = 1;
        } else if (can_hit_npc == 1 && npc != 0) {
            if (npc->fields.state_58 != 4 && anim_pdata->frame >= 6.0f &&
                anim_pdata->frame <= 8.0f &&
                npc->fields.punch_enabled != 0 &&
                npc_hit_by_punch(
                    npc, npc->fields.punch_distance, 1.3962634f) != 0) {
                npc->fields.punch_count++;
                npc_signal_event(npc, 4);
            }

            if (npc->fields.flag_bits.flags_bit4) {
                MkObj* npc_object;

                npc_object = npc->fields.animation_state->object;
                if (npc_object != 0) {
                    float target_angle;
                    float difference;
                    float magnitude;

                    direction.x = npc_object->pos.value.x - hero->pos.value.x;
                    direction.y = npc_object->pos.value.y - hero->pos.value.y;
                    direction.z = npc_object->pos.value.z - hero->pos.value.z;
                    target_angle = norm_angle(xz_to_y_ang(&direction));
                    difference = hero->ang.y - target_angle;
                    magnitude = difference >= 0.0f ? difference : -difference;
                    if (magnitude <= 0.2f) {
                        hero->ang.y = target_angle;
                    } else if (difference < 0.0f) {
                        if (difference < -3.1415927f) {
                            hero->ang.y -= 0.2f;
                        } else {
                            hero->ang.y += 0.2f;
                        }
                    } else if (difference > 3.1415927f) {
                        hero->ang.y += 0.2f;
                    } else {
                        hero->ang.y -= 0.2f;
                    }
                    update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
                }
            }
        }

        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    hero->pos_vel.z = 0.0f;
    hero->pos_vel.y = 0.0f;
    hero->pos_vel.x = 0.0f;
    konquest_pdata->hero_state = 0;
    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_control_konquest_monk, aproc, 0.0f);
    return 0.0f;
}

static void exit_meditation(void) {
    MkObj* hero;
    KonquestGameSpeedPdata* pdata;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    pdata = 0;
    if (in_exit_meditation == 0 && hero != 0) {
        in_exit_meditation = 1;
        if (aproc->pid != 0x8238) {
            destroy_mkprocs_pid(0x8238);
        }
        snd_stop(meditate_stream);
        if (konquest_pdata->hero_state != 10 &&
            monk_state_data[10].transition_order >=
                monk_state_data[konquest_pdata->hero_state].transition_order) {
            konquest_pdata->hero_state = 10;
        }
        snd_req(0x15BD);
        if (_create_mkproc_generic_tinystack(
                0x8233, 0x1F, ramp_game_speed, sizeof(*pdata),
                (void**)&pdata) != 0 &&
            pdata != 0) {
            if (refresh_rate() == 60) {
                pdata->multiplier = 0.5f;
            } else {
                pdata->multiplier = 0.5186722f;
            }
            pdata->target_scale = 1.0f;
        }
        while (find_mkproc_pid(0x8233) != 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        }
        hero->flags_0B &= ~0x20;
        if (konquest_pdata->game_mode_index != 0) {
            konquest_pdata->game_mode_index--;
        }
        in_exit_meditation = 0;
    }
}

/*
 * Near match: code bytes and size agree with retail; objdiff's only residue is
 * local floating-constant relocation labeling within the shared SDA pool.
 */
static float ramp_game_speed(void) {
    KonquestGameSpeedPdata* pdata;
    float multiplier;
    float target;

    pdata = (KonquestGameSpeedPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }

    konquest_pdata->interior_active = 1;
    multiplier = pdata->multiplier;
    target = original_game_speed * pdata->target_scale;
    if (multiplier > 1.0f) {
        use_feedback_effect = 1;
        feedback_blendrate = 50;
    }

    while ((target - game_speed >= 0.0f ? target - game_speed
                                        : -(target - game_speed)) > 0.3f) {
        _mkproc_sleep_ticks = 20.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        game_speed *= multiplier;
        ticks_per_game_day = 60.0f * (600.0f / game_speed);
        ticks_per_hour = ticks_per_game_day / 24.0f;
        game_hours_per_tick = 1.0f / ticks_per_hour;
        sun_angle_change_per_tick = 6.2831855f / ticks_per_game_day;
        konquest_pdata->time_rate_changed = 1;
        if (multiplier > 1.0f) {
            feedback_blendrate += 30;
        } else {
            feedback_blendrate -= 30;
        }
    }

    set_game_speed(target);
    if (multiplier < 1.0f) {
        use_feedback_effect = 0;
    }
    konquest_pdata->interior_active = 0;
    return -1.0f;
}

static float p_monitor_meditation_time(void) {
    unsigned char target_time[0x18];

    memcpy(
        target_time, &konquest_pdata->current_time,
        sizeof(target_time));
    add_days_to_time(target_time, 7);

    while (is_time_a_greater_than_time_b(
               target_time, &konquest_pdata->current_time)) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_pdata->interior_active == 0) {
        exit_meditation();
    }
    return -1.0f;
}

/*
 * Near match: p_monk_meditate 94.63% (944 versus 932 bytes). Animation
 * completion, mode-stack entry, the speed and time-monitor processes, sound
 * lifecycle, state-10 teardown, and control transfer agree. Residue is two
 * folded latch joins, one equivalent mode-test branch, individual versus
 * grouped GPR saves, vtable coloring, and pooled floating-constant labels.
 */
static float p_monk_meditate(void) {
    MkHdr* monitor_pdata;
    KonquestGameSpeedPdata* speed_pdata;
    MkObj* hero;
    MkObj* meditation_hero;
    MonkStateData* state;
    int initial_state;
    int mode_index;
    int animation_complete;

    hero = konquest_pdata->hero_object;
    initial_state = konquest_pdata->hero_state;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    konquest_pdata->npc_interaction_state = 0;
    hero->hide_flag_bits.still_move = 0;
    state = &monk_state_data[konquest_pdata->hero_state];
    transition_to_anim_script(
        anim_pdata, state->animation, state->transition | 0x20, 0.05f);
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    while (konquest_pdata->hero_state == initial_state) {
        state = &monk_state_data[konquest_pdata->hero_state];
        anim_pdata->step = state->animation_step;
        advance_anim(anim_pdata);
        pose_anim(anim_pdata, 1);

        if ((anim_pdata->flags & 3) != 0 &&
            anim_pdata->frame >= anim_pdata->high_frame) {
            animation_complete = 1;
        } else {
            animation_complete = 0;
        }
        if (animation_complete != 0) {
            switch (initial_state) {
            case 8:
                mode_index = konquest_pdata->game_mode_index;
                if (konquest_current_game_mode() != 4 && mode_index < 3) {
                    konquest_pdata->game_mode_index++;
                    konquest_pdata
                        ->game_modes[konquest_pdata->game_mode_index] = 4;
                }

                meditation_hero = konquest_pdata->hero_object;
                if (meditation_hero != 0) {
                    if (meditation_hero->hdr.instance ==
                        konquest_pdata->hero_instance) {
                        /* Valid hero latch. */
                    } else {
                        meditation_hero = 0;
                    }
                } else {
                    meditation_hero = 0;
                }
                speed_pdata = 0;
                monitor_pdata = 0;
                if (meditation_hero != 0) {
                    snd_req(0x15BC);
                    meditation_hero->flags_0B_bits.force_anim_speed = 1;
                    if (konquest_pdata->hero_state != 9 &&
                        monk_state_data[9].transition_order >=
                            monk_state_data[konquest_pdata->hero_state]
                                .transition_order) {
                        konquest_pdata->hero_state = 9;
                    }
                    if (_create_mkproc_generic_tinystack(
                            0x8233, 0x1F, ramp_game_speed,
                            sizeof(*speed_pdata),
                            (void**)&speed_pdata) != 0 &&
                        speed_pdata != 0) {
                        if (refresh_rate() == 60) {
                            speed_pdata->multiplier = 2.0f;
                            speed_pdata->target_scale =
                                (float)pow(2.0, 5.0);
                        } else {
                            speed_pdata->multiplier = 1.928f;
                            speed_pdata->target_scale =
                                (float)pow(1.928f, 5.0);
                        }
                    }
                    _create_mkproc_generic_tinystack(
                        0x8238, 0x1F, p_monitor_meditation_time,
                        sizeof(*monitor_pdata), (void**)&monitor_pdata);
                    meditate_stream = snd_req(0x15BB);
                }
                break;
            case 10:
                if (hero != 0) {
                    hero->pos_vel.z = 0.0f;
                    hero->pos_vel.y = 0.0f;
                    hero->pos_vel.x = 0.0f;
                    hero->flags_09_bits.bit6 = 1;
                    konquest_pdata->hero_state = 0;
                    hero->hide_flag_bits.still_move = 1;
                    konquest_pdata->hero_anim->weight = 1.0f;
                }
                break;
            }
        }

        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_control_konquest_monk, aproc, 0.0f);
    return 0.0f;
}

/*
 * Near match: p_monk_move 90.11% (1028 versus 1020 bytes). The typed event
 * frame/bone tables, footstep sounds and dust effect, animation completion,
 * state transitions, and control transfer agree. Residue is two folded latch
 * joins, equivalent loop-entry scheduling of the state-table base, individual
 * versus grouped GPR saves, register coloring, and pooled constants.
 */
static float p_monk_move(void) {
    MkObj* hero;
    MkObj* effect_hero;
    MonkStateData* state;
    Vec bone_offset;
    Vec effect_position;
    unsigned int effect;
    float frame_delta;
    int initial_state;
    int current_state;
    int bone;
    int animation_complete;

    hero = konquest_pdata->hero_object;
    initial_state = konquest_pdata->hero_state;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero == 0) {
        return -1.0f;
    }

    hero->hide_flag_bits.pin_animation = 0;
    konquest_pdata->animation_event_index = 0;
    state = &monk_state_data[konquest_pdata->hero_state];
    if (anim_pdata->animation != state->animation) {
        transition_to_anim_script(
            anim_pdata, state->animation, state->transition, 0.1f);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    while (konquest_pdata->hero_state == initial_state) {
        current_state = konquest_pdata->hero_state;
        state = &monk_state_data[current_state];
        anim_pdata->step = state->animation_step;
        handle_monk_input();
        advance_anim(anim_pdata);
        pose_anim(anim_pdata, 1);

        current_state = konquest_pdata->hero_state;
        state = &monk_state_data[current_state];
        if (state->animation_event_count != 0) {
            frame_delta =
                anim_pdata->frame -
                (float)state->animation_event_frames
                    [konquest_pdata->animation_event_index];
            if ((frame_delta >= 0.0f ? frame_delta : -frame_delta) <
                anim_pdata->step) {
                if (current_state == 2) {
                    random_snd_req(0x85);
                    if (get_game_state() == 0x13 &&
                        konquest_pdata->region_table->effect_bank_name != 0) {
                        effect_hero = konquest_pdata->hero_object;
                        if (effect_hero != 0) {
                            if (effect_hero->hdr.instance ==
                                konquest_pdata->hero_instance) {
                                /* Valid hero latch. */
                            } else {
                                effect_hero = 0;
                            }
                        } else {
                            effect_hero = 0;
                        }
                        {
                            bone_offset = (Vec){0.0f, 0.0f, 0.0f};

                            bone = monk_state_data
                                [konquest_pdata->hero_state]
                                    .animation_event_bones
                                        [konquest_pdata
                                             ->animation_event_index];
                            effect = fx_by_owner("dust_puff", 4);
                            get_bone_offset_world_pos(
                                effect_hero, bone, &bone_offset,
                                &effect_position);
                            fx_set_param_v3(
                                effect, 0x202, effect_position.x,
                                effect_position.y, effect_position.z);
                            fx_restart_emit(effect);
                        }
                    }
                } else if (current_state == 1) {
                    random_snd_req(0x84);
                }
                konquest_pdata->animation_event_index++;
                if (konquest_pdata->animation_event_index >=
                    monk_state_data[konquest_pdata->hero_state]
                        .animation_event_count) {
                    konquest_pdata->animation_event_index = 0;
                }
            }
        }

        if ((anim_pdata->flags & 3) != 0 &&
            anim_pdata->frame >= anim_pdata->high_frame) {
            animation_complete = 1;
        } else {
            animation_complete = 0;
        }
        if (animation_complete != 0) {
            switch (initial_state) {
            case 4:
                current_state = konquest_pdata->hero_state;
                if (current_state != 5 &&
                    monk_state_data[5].transition_order >=
                        monk_state_data[current_state].transition_order) {
                    konquest_pdata->hero_state = 5;
                }
                break;
            case 6:
                current_state = konquest_pdata->hero_state;
                if (current_state != 7 &&
                    monk_state_data[7].transition_order >=
                        monk_state_data[current_state].transition_order) {
                    konquest_pdata->hero_state = 7;
                }
                break;
            }
        }

        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_control_konquest_monk, aproc, 0.0f);
    return 0.0f;
}

/*
 * Near match: pdata/hero validation, state selection, jump-sleep entry,
 * state-context argument, and float returns now match retail. The four-byte
 * delta is one folded hero-latch join; remaining differences are scheduling
 * of the vtable and control-proc loads plus register coloring.
 */
static float p_control_konquest_monk(void) {
    MkObj* hero;
    MonkStateData* state;

    if (pdata_of_proc(aproc) == 0) {
        return -1.0f;
    }

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero == 0) {
        return -1.0f;
    }

    state = &monk_state_data[konquest_pdata->hero_state];
    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(state->control_proc, state, 0.0f);
    return 0.0f;
}

/* Retail change_monk_age calls this leaf out of line under the TU's -inline
 * auto mode; keep that observed call boundary scoped to this definition. */
#pragma dont_inline on
void set_monk_position(float x, float y, float z, float angle) {
    CameraPdata* camera;
    MkObj* hero;

    camera = get_pdata_of_camera();
    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    if (hero != 0) {
        (hero->hide_flag_bits).pin_animation = 0;
        hero->pos.value.x = x;
        hero->pos.value.y = y;
        hero->pos.value.z = z;
        hero->ang.y = angle;
        update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
        old_hero_position.x = hero->pos.value.x;
        old_hero_position.y = hero->pos.value.y;
        old_hero_position.z = hero->pos.value.z;
    }
    if (camera != 0) {
        camera->target_ang.y = angle;
    }
    update_tile_grid();
}
#pragma dont_inline reset

void set_monk_age(int age) {
    p1_profile_konquest->fields.hero_age = (unsigned char)age;
}

int get_monk_age(void) {
    return p1_profile_konquest->fields.hero_age;
}

/*
 * Near match: the complete hero/process teardown, reload, face-texture latch,
 * position restore, and camera handoff agree. Residue is equivalent latch
 * block layout, nonvolatile-register coloring, and individual saves/restores.
 */
void change_monk_age(int age) {
    MkObj* hero;
    AnimPdata* animation;
    MkProc* animation_proc;
    KonquestGrounding* grounding;
    int slot;
    AniTextureControl* face_texture;
    char* face_art;
    CameraPdata* camera;
    MkProc* hero_proc;
    float x;
    float y;
    float z;
    float radius;
    float angle;
    char texture_name[0x40];

    if (age >= 0 && age <= 4) {

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    x = hero->pos.value.x;
    y = hero->pos.value.y;
    z = hero->pos.value.z;
    angle = hero->ang.y;
    p1_profile_konquest->fields.hero_age = (unsigned char)age;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        if (konquest_pdata->hero_object->hdr.instance != 0) {
            konquest_pdata->hero_object->hdr.typed_vtbl->destroy(
                &konquest_pdata->hero_object->hdr);
        }
        konquest_pdata->hero_object = 0;
        konquest_pdata->hero_instance = 0;
    }

    animation = konquest_pdata->hero_anim;
    if (animation != 0) {
        animation_proc = animation->proc;
        if (animation_proc != 0) {
            if (animation_proc->instance == animation->proc_instance) {
                /* Valid animation-process latch. */
            } else {
                animation_proc = 0;
            }
        } else {
            animation_proc = 0;
        }
        if (animation_proc != 0) {
            if (animation->proc->instance != 0) {
                animation->proc->hdr.typed_vtbl->destroy(&animation->proc->hdr);
            }
            konquest_pdata->hero_anim->proc = 0;
            konquest_pdata->hero_anim->proc_instance = 0;
        }
        if (konquest_pdata->hero_anim->hdr.instance != 0) {
            konquest_pdata->hero_anim->hdr.typed_vtbl->destroy(
                &konquest_pdata->hero_anim->hdr);
        }
        konquest_pdata->hero_anim = 0;
    }

    TearDownShadow((ShadowObject*)&pdata_monk);
    xfer_camera(p_idle, 1);

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero == 0 && konquest_pdata->hero_anim == 0 &&
        (hero_proc = load_hero_model(konquest_animations[0])) != 0) {
        animation = (AnimPdata*)pdata_of_proc(hero_proc);
        hero = animation->obj;
        if (hero != 0) {
            if (hero->hdr.instance == animation->obj_instance) {
                /* Valid animation-object latch. */
            } else {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero != 0) {
            konquest_pdata->hero_object = hero;
            konquest_pdata->hero_instance = hero->hdr.instance;
        }
        konquest_pdata->hero_anim = animation;
        konquest_pdata->hero_state = 0;
        (hero->hide_flag_bits).still_move = 1;
    }

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0) {
        if (grounding->hdr.instance == konquest_pdata->grounding_instance) {
            /* Valid grounding latch. */
        } else {
            grounding = 0;
        }
    } else {
        grounding = 0;
    }
    if (grounding == 0 || grounding->camera_target == 0) {
        return;
    }

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    grounding->camera_target->focus_object = hero;

    animation = konquest_pdata->hero_anim;
    animation_proc = animation->proc;
    if (animation_proc != 0) {
        if (animation_proc->instance == animation->proc_instance) {
            /* Valid animation-process latch. */
        } else {
            animation_proc = 0;
        }
    } else {
        animation_proc = 0;
    }
    grounding->camera_target->animation_proc = animation_proc;

    face_texture = grounding->camera_target->face_texture;
    if (face_texture != 0) {
        if ((unsigned int)face_texture->instance ==
            grounding->camera_target->face_texture_instance) {
            /* Valid face-texture latch. */
        } else {
            face_texture = 0;
        }
    } else {
        face_texture = 0;
    }
    if (face_texture != 0) {
        if ((unsigned int)grounding->camera_target->face_texture->instance !=
            0) {
            ((KonquestDestroyable*)grounding->camera_target->face_texture)
                ->vtbl->destroy(
                    (KonquestDestroyable*)grounding->camera_target
                        ->face_texture);
        }
        grounding->camera_target->face_texture = 0;
        grounding->camera_target->face_texture_instance = 0;
    }

    slot = 0xB002A;
    face_texture = 0;
    if (mode_of_play == 8) {
        slot = 0x2001E;
    }
    if (konquest_editor_mode_on == 0) {
        sprintf(texture_name, "KON_HERO_0%d_MOUTH",
                p1_profile_konquest->fields.hero_age + 1);
        face_art = (char*)get_artid_of_named_item_in_slot(
            slot, texture_name, 0);
        if (face_art != 0) {
            face_texture = append_wiff_to_clump_material_id(
                slot, face_art, hero->clump, 1);
        }
    }
    if (face_texture != 0) {
        grounding->camera_target->face_texture = face_texture;
        grounding->camera_target->face_texture_instance =
            face_texture->instance;
    }

    set_monk_position(x, y, z, angle);
    camera = get_pdata_of_camera();
    if (camera != 0) {
        hero = konquest_pdata->hero_object;
        if (hero != 0) {
            if (hero->hdr.instance == konquest_pdata->hero_instance) {
                /* Valid hero latch. */
            } else {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero != 0) {
            camera->movement_focus = hero;
        }
    }
    destroy_mkprocs_pid(0x9006);
    xfer_camera(p_konquest_camera_proc, 1);
    }
}

/*
 * Near match: all calls, flags, object setup, animation setup, and shadow
 * initialization agree. Residue is individual r29-r31 saves/restores versus
 * stmw/lmw and local constant/string relocation labels.
 */
MkProc* load_hero_model(int animation_script) {
    int slot;
    unsigned char hero_age;
    MkObj* hero;
    AnimPdata* animation;
    MkProc* created;
    char model_name[0x20];

    hero_age = p1_profile_konquest->fields.hero_age;
    _create_mkproc_generic_bigstack(
        0x4003, 0x1F, p_load_hero_art_section, 0, 0);
    do {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    } while (find_mkproc_pid(0x4003) != 0);

    slot = 0xB002A;
    if (mode_of_play == 8) {
        slot = 0x2001E;
    }
    sprintf(model_name, "KON_HERO_%02d", hero_age + 1);
    hero = load_named_model_from_slot(slot, model_name, 0x9003, 0);
    if (hero == 0) {
        return 0;
    }

    obj_create_sobjs(hero);
    hero->pos.value.z = 0.0f;
    hero->pos.value.y = 0.0f;
    hero->pos.value.x = 0.0f;
    hero->ang.y = 3.1415927f;
    (hero->flags_08_bits).gravity_enabled = 1;
    (hero->flags_08_bits).rotation_enabled = 1;
    (hero->flags_08_bits).airborne = 1;
    (hero->flags_08_bits).angular_velocity_enabled = 1;
    update_mkobj(hero != 0 ? as_mkhdr(&hero->hdr) : 0);
    hero->light_flags = 1;
    build_bones_tbl(hero, konquest_human_bones);
    hero->flipped_bone_map = &flipped_konquest_human_bones;
    (hero->flags_09_bits).launched = 1;
    hero->ground_colls = monk_ground_colls;

    created = create_mkproc_anim(
        0x5002, p_control_konquest_monk, &animation);
    if (created != 0) {
        animation->obj = hero;
        animation->obj_instance = hero->hdr.instance;
        set_root_and_obj_movement_weights(0.0f, 1.0f, animation);
        animation->step = 1.0f;
        set_anim_script(animation, (AniData*)animation_script, 0);
    }
    insert_fgnd_mkobj(hero);
    insert_ground_me_mkobj(hero);
    memset(&pdata_monk, 0, sizeof(pdata_monk));
    init_shadow((ShadowObject*)&pdata_monk, hero);
    ShadowStrength = 0.7f;
    return created;
}

static float p_load_hero_art_section(void) {
    char section_name[0x20];
    int slot;

    sprintf(
        section_name, "konquest_monk_%d.sec",
        p1_profile_konquest->fields.hero_age + 1);
    if (mode_of_play == 8) {
        KonquestSaveHeader* save;

        save = (KonquestSaveHeader*)&konquest_save_data;
        load_ssf(konquest_region_data[save->region].art_files);
        slot = 0x2001E;
        add_art_section_by_name_async(slot, section_name);
        wait_for_slot_load(slot);
    } else {
        load_ssf((MkFileEntry*)konquest_common_file_table);
        load_art_section_by_name(0xB002A, section_name);
    }
    return -1.0f;
}

/*
 * Near match: camera/hero latches, mode handling, shadow flags, call ABI, and
 * camera-flip correction match. Only two folded latch joins and local float
 * relocation labels remain; local text is eight bytes smaller.
 */
void render_konquest_shadows(void) {
    KonquestPdata* pdata;
    MkObj* camera;
    MkObj* candidate;
    MkObj* hero;
    int game_mode;

    camera = (MkObj*)camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance == camera_item.instance) {
            /* Valid camera object latch. */
        } else {
            camera = 0;
        }
    } else {
        camera = 0;
    }
    update_mkobj((MkHdr*)camera);

    pdata = konquest_pdata;
    if (pdata != 0) {
        candidate = pdata->hero_object;
        if (candidate != 0) {
            if (candidate->hdr.instance == pdata->hero_instance) {
                /* Valid hero latch. */
            } else {
                candidate = 0;
            }
        } else {
            candidate = 0;
        }
        hero = candidate;
        if (pdata->game_mode_index < 0) {
            game_mode = 0;
        } else {
            game_mode = pdata->game_modes[pdata->game_mode_index];
        }
        if (game_mode == 8) {
            ShadowStrength = 0.0f;
        }
    } else {
        hero = trial_get_monk();
    }

    if (hero != 0) {
        if (hero->hide_flag_bits.hidden || ShadowStrength == 0.0f) {
            pdata_monk.ground_object->hide_flag_bits.hidden = 1;
            return;
        }
        pdata_monk.ground_object->hide_flag_bits.hidden = 0;
        UpdateShadow(hero, (ShadowObject*)&pdata_monk, hero);
        if (fix_camera_flip == 1) {
            pdata_monk.ground_object->ang.y -= 3.1415927f;
            fix_camera_flip = 0;
        }
    }
}

/* Near match: only the valid-hero latch's equivalent folded branch remains. */
void set_hero_punched_ground_collisions(int punched) {
    MkObj* hero;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (punched != 0) {
        hero->ground_colls = monk_laying_on_ground_colls;
    } else {
        hero->ground_colls = monk_ground_colls;
    }
}

/*
 * Near match at retail size: only the initial x/limit load scheduling and a
 * local 60.0f relocation label differ.
 */
int get_tile_from_position(const Vec* position) {
    int row;
    int column;

    if (position->x >= 1000.0f) {
        return konquest_pdata->tile_width * konquest_pdata->tile_height;
    }
    row = (int)((position->z + konquest_pdata->tile_origin_z) / 60.0f);
    if (row < 0) {
        return -1;
    }
    if (row >= konquest_pdata->tile_height) {
        return -1;
    }
    column = (int)((position->x + konquest_pdata->tile_origin_x) / 60.0f);
    if (column < 0) {
        return -1;
    }
    if (column >= konquest_pdata->tile_width) {
        return -1;
    }
    return column + row * konquest_pdata->tile_width;
}

/*
 * Near match: range checks, inlined tile lookup, grid arithmetic, label UI,
 * and visibility calls agree. Two latch joins are folded, with minor GPR
 * coloring and local constant/string relocation-label differences.
 */
void update_tile_grid(void) {
    MkObj* hero;
    KonquestTileRecord* tile;
    StringObj* label;
    int tile_index;
    char text[0x18];

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    tile_index = konquest_pdata->tile_load_state;
    if (tile_index <
        konquest_pdata->tile_width * konquest_pdata->tile_height + 1) {
        tile = &konquest_pdata->tile_structs[tile_index];
    } else {
        tile = 0;
    }
    if (tile != 0) {
        if (hero != 0 &&
            (hero->pos.value.x < tile->position.x - 30.0f ||
             hero->pos.value.x > tile->position.x + 30.0f ||
             hero->pos.value.z < tile->position.z - 30.0f ||
             hero->pos.value.z > tile->position.z + 30.0f)) {
            hide_currently_visible_tiles();
            konquest_pdata->tile_load_state =
                get_tile_from_position(&hero->pos.value);
            konquest_pdata->tile_column =
                konquest_pdata->tile_load_state % konquest_pdata->tile_width;
            konquest_pdata->tile_row =
                konquest_pdata->tile_load_state / konquest_pdata->tile_width +
                1;

            label = (StringObj*)konquest_pdata->hud_labels[5].object;
            if (label != 0) {
                if (label->instance ==
                    konquest_pdata->hud_labels[5].instance) {
                    /* Valid grid-label latch. */
                } else {
                    label = 0;
                }
            } else {
                label = 0;
            }
            sprintf(text, "%c - %d", konquest_pdata->tile_column + 'A',
                    konquest_pdata->tile_row);
            if (label != 0) {
                update_string_obj(label, 0xE, text);
            } else {
                label = string_center_xy(
                    0x8300, 0xE, text, 0x52, screen_height - 0x9B, 0x44);
                konquest_pdata->hud_labels[5].object = (MkHdr*)label;
                konquest_pdata->hud_labels[5].instance = label->instance;
            }
            update_visible_tiles();
        }
        konquest_update_true_clipped_tiles();
    }
}

/*
 * Soft ceiling: hide_currently_visible_tiles ~87.6% at the exact retail size.
 * The 5x5 bounds, tile lookup, visibility test, and hide call match; the only
 * residue is equivalent row/column arithmetic scheduling and GPR coloring.
 * Size optimization is required for retail's divw lowering.
 */
#pragma optimize_for_size on
static void hide_currently_visible_tiles(void) {
    int index;

    for (index = 0; index < 25; index++) {
        KonquestTileRecord* tile;
        int row;
        int column;
        int tile_index;
        int width;
        int height;
        int window_row;
        int tile_row;
        int window_column;
        int tile_column;

        width = konquest_pdata->tile_width;
        window_row = index / 5;
        tile_row = konquest_pdata->tile_load_state / width;
        row = tile_row + window_row - 2;
        window_column = index % 5;
        tile_column = konquest_pdata->tile_load_state % width;
        column = tile_column + window_column - 2;
        height = konquest_pdata->tile_height;
        if (row < 0 || row > height || column < 0 || column > width) {
            tile = 0;
        } else {
            tile_index = column + row * width;
            if (tile_index >= 0 && tile_index <= width * height) {
                tile = &konquest_pdata->tile_structs[tile_index];
            } else {
                tile = 0;
            }
        }
        if (tile != 0 && tile->visible != 0) {
            hide_tile(tile);
        }
    }
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: update_visible_tiles ~92.3% at the exact retail size. Bounds,
 * calls, access widths, and visibility updates match; residue is row/column
 * arithmetic scheduling and GPR coloring. Retail's divw sequence requires the
 * scoped size optimization instead of the TU's magic-division lowering.
 */
#pragma optimize_for_size on
static inline KonquestTileRecord* get_tile_record_by_index(int index) {
    if (index <
        konquest_pdata->tile_width * konquest_pdata->tile_height + 1) {
        return &konquest_pdata->tile_structs[index];
    }
    return 0;
}

static void update_visible_tiles(void) {
    KonquestTileRecord* tile;
    unsigned int visible_tile_bits;
    int index;

        tile = get_tile_record_by_index(
        konquest_pdata->tile_load_state);
    if (tile != 0) {
        visible_tile_bits = tile->state;
    } else {
        visible_tile_bits = 0;
    }
    konquest_pdata->visible_tile_bits = visible_tile_bits;

    for (index = 0; index < 25; index++) {
        int row;
        int column;
        int tile_index;
        int width;
        int height;
        int visible;
        int window_row;
        int tile_row;
        int window_column;
        int tile_column;

        width = konquest_pdata->tile_width;
        window_row = index / 5;
        tile_row = konquest_pdata->tile_load_state / width;
        row = tile_row + window_row - 2;
        window_column = index % 5;
        tile_column = konquest_pdata->tile_load_state % width;
        column = tile_column + window_column - 2;
        height = konquest_pdata->tile_height;
        if (row < 0 || row > height || column < 0 || column > width) {
            tile = 0;
        } else {
            tile_index = column + row * width;
            if (tile_index >= 0 && tile_index <= width * height) {
                tile = &konquest_pdata->tile_structs[tile_index];
            } else {
                tile = 0;
            }
        }

        visible = 0;
        if (tile != 0) {
            if (((konquest_pdata->visible_tile_bits >> index) & 1) != 0) {
                visible = 1;
            }
            if (visible != 0 && tile->visible == 0) {
                unhide_tile(tile);
            } else if (visible == 0) {
                hide_tile(tile);
            }
        }
    }
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: unhide_tile ~83.9% at the exact retail size. The complete body
 * matches; only stmw/lmw r29-r31 versus individual save/restore emission
 * differs.
 */
static void unhide_tile(KonquestTileRecord* tile) {
    MkPtr* link;

    tile->visible = 1;
    if (tile->scene != 0) {
        unhide_sobj_and_children(tile->scene);
        set_true_clip_flag_on_sobj_and_children(tile->scene, 0);
    }

    if (tile != 0 && tile->objects != 0) {
        link = tile->objects;
        while (link != 0) {
            if (link->instance != link->hdr->instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                show_konquest_object((KonquestUidObject*)link->hdr);
                link = link->next;
            }
        }
    }
}

static void hide_tile(KonquestTileRecord* tile) {
    tile->visible = 0;
    if (tile->scene != 0) {
        hide_sobj_and_children(tile->scene);
        set_true_clip_flag_on_sobj_and_children(tile->scene, 0);
    }
    hide_tile_objects(tile);
    remove_collisions_from_tile_and_tile_objects(tile);
}

/*
 * Soft ceiling: konquest_update_true_clipped_tiles ~93.6% at the exact retail
 * size. Bounds, calls, thresholds, flag transitions, and the distance kernel
 * match; residue is 5x5 coordinate scheduling/GPR coloring and local float
 * relocation labels.
 */
#pragma optimize_for_size on
static void konquest_update_true_clipped_tiles(void) {
    int index;

    purge_global_collision_list();
    for (index = 0; index < 25; index++) {
        KonquestTileRecord* tile;
        int row;
        int column;
        int tile_index;
        int width;
        int height;

        width = konquest_pdata->tile_width;
        row = konquest_pdata->tile_load_state / width + index / 5 - 2;
        column = konquest_pdata->tile_load_state % width + index % 5 - 2;
        height = konquest_pdata->tile_height;
        if (row < 0 || row > height || column < 0 || column > width) {
            tile = 0;
        } else {
            tile_index = column + row * width;
            if (tile_index >= 0 && tile_index <= width * height) {
                tile = &konquest_pdata->tile_structs[tile_index];
            } else {
                tile = 0;
            }
        }

        if (tile != 0 && tile->visible != 0) {
            float dx;
            float dz;
            float distance_squared;

            dx = camera_obj->pos.x - tile->position.x;
            dz = camera_obj->pos.z - tile->position.z;
            distance_squared = dx * dx + dz * dz;
            if (tile->scene != 0) {
                int true_clipped;

                if (distance_squared < 3600.0f) {
                    true_clipped = 1;
                } else {
                    true_clipped = 0;
                }
                set_true_clip_flag_on_sobj_and_children(
                    tile->scene, true_clipped);
            }
            if (distance_squared < 7199.9976f) {
                generate_collisions_for_tile_and_tile_objects(tile);
            } else {
                remove_collisions_from_tile_and_tile_objects(tile);
            }
            if (distance_squared < 3600.0f) {
                if (tile->shadow_collisions_active == 0 &&
                    tile->collision_object_list != 0) {
                    insert_collision_list_on_konquest_shadow_lists(
                        &tile->collision_object_list->hdr);
                    tile->shadow_collisions_active = 1;
                }
            } else if (tile->shadow_collisions_active != 0 &&
                       tile->collision_object_list != 0) {
                remove_collision_list_from_konquest_shadow_lists(
                    &tile->collision_object_list->hdr);
                tile->shadow_collisions_active = 0;
            }
        }
    }
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: remove_collisions_from_tile_and_tile_objects ~79.0%. The
 * executable body matches; individual r28-r31 saves/restores make this 24
 * bytes larger than retail's stmw/lmw form.
 */
static void remove_collisions_from_tile_and_tile_objects(
    KonquestTileRecord* tile) {
    MkPtr* link;

    if (tile->collisions_active == 0) {
        return;
    }

    destroy_list(&tile->collisions);
    tile->collisions = 0;

    if (tile != 0 && tile->objects != 0) {
        link = tile->objects;
        while (link != 0) {
            if (link->instance != link->hdr->instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                KonquestUidObject* object = (KonquestUidObject*)link->hdr;

                destroy_list(&object->collisions);
                link = link->next;
            }
        }
    }

    tile->collisions_active = 0;
}

/*
 * Soft ceiling: generate_collisions_for_tile_and_tile_objects ~91.2% -- all
 * executable body instructions match. Residue is the zero-vector relocation
 * label and individual r29-r31 saves/restores versus retail stmw/lmw.
 */
static void generate_collisions_for_tile_and_tile_objects(
    KonquestTileRecord* tile) {
    Vec zero = {0.0f, 0.0f, 0.0f};

    if (tile->collisions_active == 0) {
        MkPtr* link;

        if (tile->collision_art_id != 0) {
            generate_collision_objects(
                0x60029, tile->collision_art_id, &zero, &zero,
                &tile->collisions);
            if (tile->collisions != 0) {
                exclusive_or_flags_for_all_collisions(&tile->collisions, 2);
                set_flag_for_all_collisions(
                    &tile->collisions, 0x80000000);
            }
        }
        if (tile != 0 && tile->objects != 0) {
            link = tile->objects;
            while (link != 0) {
                KonquestUidObject* object;

                object = (KonquestUidObject*)link->hdr;
                if (link->instance != object->hdr.instance) {
                    MkPtr* next;

                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else {
                    KonquestRenderRecord* record;

                    record = (KonquestRenderRecord*)first_mkhdr(
                        &object->render_records);
                    if (record != 0 && object->collision_art_id != 0) {
                        generate_collision_objects(
                            0x60029, object->collision_art_id,
                            &record->position, &record->angles,
                            &object->collisions);
                        set_flag_for_all_collisions(
                            &object->collisions, 0x80000000);
                    }
                    link = link->next;
                }
            }
        }
        tile->collisions_active = 1;
    }
}

void* get_visible_tile_set(int index) {
    KonquestTileRecord* tile;

    tile = get_tile_record_by_index(index);
    if (tile == 0) {
        return 0;
    }
    return tile->visible_set;
}

void play_beam_advance_sound(int delay) {
    snd_req_delay(0x158e, delay);
}

/*
 * Near match: both generation latches, objective-table bound, HUD gate,
 * visibility calls, and state publication match retail. The four-byte size
 * delta is scalar saves versus stmw/lmw and folded latch-join branches.
 */
void show_objective_arrow_and_beam(void) {
    MkHdr* arrow;
    MkHdr* beam;

    arrow = konquest_pdata->hud_objects[2].object;
    if (arrow != 0) {
        if (arrow->instance ==
            konquest_pdata->hud_objects[2].instance) {
            /* Valid screen-object latch. */
        } else {
            arrow = 0;
        }
    } else {
        arrow = 0;
    }
    beam = konquest_pdata->objective_beam.object;
    if (beam != 0) {
        if (beam->instance ==
            konquest_pdata->objective_beam.instance) {
            /* Valid beam latch. */
        } else {
            beam = 0;
        }
    } else {
        beam = 0;
    }
    if (arrow != 0) {
        if (beam == 0) {
            return;
        }
        if (p1_profile_konquest->fields.objective_index <
            (int)get_row_count_for_table_by_pointer(
                konquest_pdata->script_owner,
                konquest_pdata->objective_table)) {
            if (konquest_pdata->hud_visible != 0) {
                unhide_screen_obj((ScreenObj*)arrow);
            }
            unhide_obj(beam);
            konquest_pdata->objective_visible = 1;
        }
    }
}

void hide_objective_arrow_and_beam(void) {
    MkHdr* arrow;
    MkHdr* beam;
    MkSobj* sky_object;

    arrow = konquest_pdata->hud_objects[2].object;
    if (arrow != 0) {
        if (arrow->instance ==
            konquest_pdata->hud_objects[2].instance) {
            /* Valid screen-object latch. */
        } else {
            arrow = 0;
        }
    } else {
        arrow = 0;
    }
    beam = konquest_pdata->objective_beam.object;
    if (beam != 0) {
        if (beam->instance ==
            konquest_pdata->objective_beam.instance) {
            /* Valid beam latch. */
        } else {
            beam = 0;
        }
    } else {
        beam = 0;
    }
    if (arrow != 0) {
        if (beam == 0) {
            return;
        }
        hide_screen_obj((ScreenObj*)arrow);
        hide_obj(beam);
        if (g_game_info.sky != 0) {
            sky_object =
                (MkSobj*)obj_find_sobj_by_id(g_game_info.sky, 1);
            if (sky_object != 0) {
                hide_sobj(sky_object);
            }
        }
        konquest_pdata->objective_visible = 0;
    }
}
static inline KonquestTriggerStruct* find_trigger_by_definition(
    KonquestTriggerDefinition* definition) {
    MkPtr* link;
    MkPtr* next;

    if (konquest_pdata->triggers != 0) {
        link = konquest_pdata->triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (trigger->owned_data == definition) {
                    return trigger;
                }
                link = link->next;
            }
        }
    }
    if (konquest_pdata->temporary_triggers != 0) {
        link = konquest_pdata->temporary_triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (trigger->owned_data == definition) {
                    return trigger;
                }
                link = link->next;
            }
        }
    }
    return 0;
}


/*
 * Soft ceiling: trigger_set_time_for_enable ~89.6% -- lookup, timestamp copy,
 * timed action, and unit dispatch match retail. Residue is paired-copy loop
 * lowering (GPR decrement versus bdnz) and the resulting GPR allocation.
 */
void trigger_set_time_for_enable(KonquestTriggerDefinition* definition,
                                 int unit, int amount, int action) {
    KonquestTimePair* destination;
    int pairs_remaining;
    KonquestTimePair* source;
    KonquestTriggerStruct* trigger;

    enable_trigger(definition, action == 0);
    trigger = find_trigger_by_definition(definition);
    source = konquest_pdata->current_time.pairs;
    destination = trigger->action_time.pairs;
    pairs_remaining = 3;
    do {
        *destination++ = *source++;
    } while (--pairs_remaining != 0);
    trigger->timed_action = action != 0 ? 1 : 2;

    switch (unit) {
    case 0:
        add_minutes_to_time(&trigger->action_time, amount);
        break;
    case 1:
        add_hours_to_time(&trigger->action_time, amount);
        break;
    case 2:
        add_days_to_time(&trigger->action_time, amount);
        break;
    case 3:
        add_months_to_time(&trigger->action_time, amount);
        break;
    case 4:
        add_years_to_time(&trigger->action_time, amount);
        break;
    }
}

/*
 * Soft ceiling: enable_trigger ~87.9% -- both stale-safe list searches and
 * the complete body match retail. Residue is four individual nonvolatile-GPR
 * saves/restores in place of the retail stmw/lmw pair.
 */
void enable_trigger(KonquestTriggerDefinition* definition, int state) {
    KonquestTriggerStruct* trigger;

    if (konquest_pdata->flag_bits.triggers_active) {
        if (konquest_pdata->triggers != 0) {
            trigger = find_trigger_by_definition(definition);
            if (trigger != 0) {
                trigger->flag_bits.bit0 = 1;
            }
        } else {
            state |= 2;
        }
    }
    definition->state = state;
}
static inline KonquestCollisionVolume* find_collision_volume_by_uid(int uid) {
    int tile_index;

    tile_index = 0;
    while (tile_index <
           konquest_pdata->tile_width * konquest_pdata->tile_height) {
        KonquestTileRecord* tile;
        MkPtr* link;

        tile = &konquest_pdata->tile_structs[tile_index];
        if (tile->objects != 0) {
            link = tile->objects;
            while (link != 0) {
                KonquestCollisionVolume* volume;

                volume = (KonquestCollisionVolume*)link->hdr;
                if (link->instance != volume->hdr.instance) {
                    MkPtr* next;

                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                    continue;
                }
                if (volume->uid == uid) {
                    return volume;
                }
                link = link->next;
            }
        }
        tile_index++;
    }
    return 0;
}


/*
 * Soft ceiling: restore_collision_volume_on_object_with_uid ~94.4%. The
 * stale-safe UID search, placement lookup, art publication, collision rebuild,
 * flag update, and four-call sequence agree with retail with no opcode-class
 * mismatch. The local body is one instruction shorter from GPR allocation and
 * equivalent load/branch scheduling (272 versus 268 bytes).
 */
void restore_collision_volume_on_object_with_uid(int uid) {
    KonquestCollisionVolume* volume;
    KonquestCollisionPlacement* placement;

    volume = find_collision_volume_by_uid(uid);
    if (volume != 0) {
        placement =
            (KonquestCollisionPlacement*)first_mkhdr(&volume->placements);
        if (placement->definition->art_id != 0) {
            volume->art_id = placement->definition->art_id;
            generate_collision_objects(
                0x60029, placement->definition->art_id,
                &placement->position, &placement->angles, &volume->objects);
            set_flag_for_all_collisions(&volume->objects, 0x80000000);
        }
    }
}

/*
 * Soft ceiling: restore_collision_volume_on_object ~97.6% -- retail retains
 * one unconditional ownership-latch join branch that MWCC folds from this
 * structured form; all calls, accesses, and remaining control flow match.
 */
void restore_collision_volume_on_object(void) {
    KonquestRemoveCollisionPdata* pdata;
    KonquestCollisionOwner* owner;
    KonquestCollisionVolume* volume;
    KonquestCollisionPlacement* placement;

    pdata = (KonquestRemoveCollisionPdata*)pdata_of_proc(aproc);
    owner = pdata->owner;
    if (owner != 0) {
        if (owner->hdr.instance != pdata->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }

    if (owner != 0) {
        volume = owner->collision_volume;
        if (volume != 0) {
            placement =
                (KonquestCollisionPlacement*)first_mkhdr(&volume->placements);
            if (placement->definition->art_id != 0) {
                volume->art_id = placement->definition->art_id;
                generate_collision_objects(
                    0x60029, placement->definition->art_id,
                    &placement->position, &placement->angles,
                    &volume->objects);
                set_flag_for_all_collisions(&volume->objects, 0x80000000);
            }
        }
    }
}

/*
 * Soft ceiling: remove_collision_volume_on_object_with_uid ~93.0% -- code
 * size differs by one instruction; the search and removal operations match,
 * with the same nonvolatile-GPR rotation and loop-test placement.
 */
void remove_collision_volume_on_object_with_uid(int uid) {
    KonquestCollisionVolume* volume;

    volume = find_collision_volume_by_uid(uid);
    if (volume != 0) {
        destroy_list(&volume->objects);
        volume->art_id = 0;
    }
}

/*
 * Soft ceiling: remove_collision_volume_on_object ~97.6% -- the remaining
 * ownership-latch join is the same folded branch as the restore variant.
 */
void remove_collision_volume_on_object(void) {
    KonquestRemoveCollisionPdata* pdata;
    KonquestCollisionOwner* owner;
    KonquestCollisionVolume* volume;

    pdata = (KonquestRemoveCollisionPdata*)pdata_of_proc(aproc);
    owner = pdata->owner;
    if (owner != 0) {
        if (owner->hdr.instance != pdata->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }

    if (owner != 0) {
        volume = owner->collision_volume;
        if (volume != 0) {
            destroy_list(&volume->objects);
            volume->art_id = 0;
        }
    }
}
static inline KonquestObject* find_object_by_uid_inline(int uid) {
    int tile_index;
    int tile_offset;

    tile_index = 0;
    tile_offset = 0;
    while (tile_index <
           konquest_pdata->tile_width * konquest_pdata->tile_height) {
        KonquestTileRecord* tile;
        MkPtr* link;

        tile = (KonquestTileRecord*)((char*)konquest_pdata->tile_structs +
                                     tile_offset);
        if (tile != 0 && tile->objects != 0) {
            link = tile->objects;
            while (link != 0) {
                KonquestObject* object;

                object = (KonquestObject*)link->hdr;
                if (link->instance != object->hdr.instance) {
                    MkPtr* next;

                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else if (object->uid == uid) {
                    return object;
                } else {
                    link = link->next;
                }
            }
        }
        tile_index++;
        tile_offset += sizeof(*tile);
    }
    return 0;
}

static inline KonquestChildObject* find_child_by_enumeration_inline(
    KonquestObject* object, int enumeration) {
    KonquestChildObject* child;

    child = 0;
    if (object->list_4C != 0) {
        MkPtr* link;

        link = object->list_4C;
        while (link != 0) {
            child = (KonquestChildObject*)link->hdr;
            if (link->instance != child->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                int index;

                index = child->definition->enumeration_index;
                if (index >= 0 &&
                    konquest_pdata->region_table->enumerations[index]
                            .enumeration == enumeration) {
                    return child;
                }
                link = link->next;
            }
        }
    }
    return 0;
}


/*
 * Near match: both inlined stale-list searches and the state call are exact;
 * only the tile offset, zero, and next-link GPR colors differ.
 */
void konquest_set_object_to_state(int uid, int enumeration, int state) {
    KonquestObject* object;
    KonquestChildObject* child;

    object = find_object_by_uid_inline(uid);
    child = find_child_by_enumeration_inline(object, enumeration);
    object_set_state(child, state);
}

/*
 * Near match: state lookup, latch destruction, rotated position, composed
 * angles, and matrix publication agree. The remaining eight-byte size delta
 * is two folded latch branches; other differences are register coloring.
 */
static void object_set_state(KonquestChildObject* record, int state) {
    KonquestObjectState* state_data;
    MkHdr* state_object;
    MkSobj* object;
    MKMATRIX* matrix;
    Vec* angles;
    Vec position;
    int enumeration_index;

    if (konquest_pdata->region_table->enumerations == 0) {
        return;
    }

    state_object = record->state_object;
    if (state_object != 0) {
        if (state_object->instance == record->state_object_instance) {
            /* Valid state-object latch. */
        } else {
            state_object = 0;
        }
    } else {
        state_object = 0;
    }
    if (state_object != 0) {
        if (record->state_object->instance != 0) {
            record->state_object->typed_vtbl->destroy(record->state_object);
        }
        record->state_object = 0;
        record->state_object_instance = 0;
    }

    enumeration_index = record->binding->enumeration_index;
    angles = &record->angles;
    if (enumeration_index < 0) {
        return;
    }
    rotate_xz(
        &position,
        &konquest_pdata->region_table->enumerations[enumeration_index]
             .states[state]
             .position,
        record->base_angles.y);

    state_data = &konquest_pdata->region_table
                      ->enumerations[enumeration_index]
                      .states[state];
    position.y = state_data->position.y;
    position.x += record->base_position.x;
    position.y += record->base_position.y;
    position.z += record->base_position.z;
    record->position.x = position.x;
    record->position.y = position.y;
    record->position.z = position.z;
    angles->x =
        record->base_angles.x + state_data->angles.x;
    angles->y =
        record->base_angles.y + state_data->angles.y;
    angles->z =
        record->base_angles.z + state_data->angles.z;

    matrix = 0;
    if (record->matrix_index >= 0) {
        if (record->binding->palette == 0) {
            object = record->binding->object;
            if (object != 0) {
                if (object->hdr.instance ==
                    record->binding->object_instance) {
                    /* Valid bound-object latch. */
                } else {
                    object = 0;
                }
            } else {
                object = 0;
            }
            if (object != 0) {
                matrix = &object->frame->modelling;
            }
        } else if (record->matrix_index >= 0) {
            matrix =
                &record->binding->palette->matrices[record->matrix_index];
        }
    }

    YXZ_angles_to_MKMATRIX(angles, matrix);
    matrix->pos.x = record->position.x;
    matrix->pos.y = record->position.y;
    matrix->pos.z = record->position.z;
    record->state = state;
}
static inline KonquestUidObject* find_tile_object_by_uid(int uid) {
    int tile_index;

    for (tile_index = 0;
         tile_index <
             konquest_pdata->tile_width * konquest_pdata->tile_height;
         tile_index++) {
        KonquestTileRecord* tile;
        MkPtr* link;

        tile = &konquest_pdata->tile_structs[tile_index];
        if (tile != 0 && tile->objects != 0) {
            link = tile->objects;
            while (link != 0) {
                KonquestUidObject* object;

                object = (KonquestUidObject*)link->hdr;
                if (link->instance != object->hdr.instance) {
                    MkPtr* next;

                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                    continue;
                }
                if (object->uid == uid) {
                    return object;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

static inline KonquestChildObject* find_object_child_by_enumeration(
    KonquestUidObject* object, int enumeration) {
    MkPtr* link;

    if (object->render_records != 0) {
        link = object->render_records;
        while (link != 0) {
            KonquestChildObject* child;

            child = (KonquestChildObject*)link->hdr;
            if (link->instance != child->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (child->definition->enumeration_index >= 0 &&
                enumeration ==
                    konquest_pdata->region_table
                        ->enumerations[child->definition->enumeration_index]
                        .enumeration) {
                return child;
            }
            link = link->next;
        }
    }
    return 0;
}


/*
 * Soft ceiling: konquest_transition_object_to_state ~99.4%. Both stale-safe
 * searches, enumeration lookup, process creation, scheduling flag, and typed
 * pdata stores match at the exact retail size. Residue is only a consistent
 * rotation of the argument and tile-loop GPRs.
 */
void konquest_transition_object_to_state(
    int object_uid, int enumeration, int state) {
    KonquestTransitionPdata* pdata = 0;
    KonquestUidObject* object;

    object = find_tile_object_by_uid(object_uid);
    if (object != 0) {
        KonquestChildObject* child;

        child = find_object_child_by_enumeration(object, enumeration);

        {
            MkProc* proc;

            proc = (MkProc*)_create_mkproc_generic_tinystack(
                0x8231, 0x1F, p_konquest_transition_to_state,
                sizeof(*pdata), (void**)&pdata);
            if (proc != 0) {
                proc->flags_bits.use_game_speed = 1;
                pdata->object = child;
                pdata->state = state;
                pdata->play_sound = 1;
            }
        }
    }
}

/*
 * Soft ceiling: p_konquest_transition_to_state ~99.4%. Its null guard, call
 * ABI, two returns, and 72-byte size match; only the -1.0f relocation label
 * differs.
 */
static float p_konquest_transition_to_state(void) {
    KonquestTransitionPdata* pdata;

    pdata = (KonquestTransitionPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    object_transition_to_state(
        pdata->object, pdata->state, pdata->play_sound);
    return -1.0f;
}
static inline float door_path_distance_squared(
    MkObj* hero, KonquestWaypoint* path, Vec* delta) {
    float edge_length_squared;
    float projection_numerator;
    float edge_length;
    float projection;

    {
        float edge_x;
        float edge_y;
        float edge_z;
        float hero_x;
        float hero_z;

        edge_y = path[1].position.y - path[0].position.y;
        edge_x = path[1].position.x - path[0].position.x;
        hero_x = hero->pos.value.x - path[0].position.x;
        edge_z = path[1].position.z - path[0].position.z;
        edge_length_squared =
            edge_z * edge_z + (edge_x * edge_x + edge_y * edge_y);
        hero_z = hero->pos.value.z - path[0].position.z;
        projection_numerator = hero_x * edge_x + hero_z * edge_z;
    }
    edge_length = konquest_fast_sqrt(edge_length_squared);
    projection = projection_numerator / (edge_length * edge_length);
    delta->x =
        hero->pos.value.x -
        (projection *
             (path[1].position.x - path[0].position.x) +
         path[0].position.x);
    delta->y = hero->pos.value.y - 0.0f;
    delta->z =
        hero->pos.value.z -
        (projection *
             (path[1].position.z - path[0].position.z) +
         path[0].position.z);
    return delta->z * delta->z +
           (delta->x * delta->x + delta->y * delta->y);
}


/*
 * Near match: both stale-safe object searches, door-side selection, the two
 * projected point-to-segment distance evaluations, animation choice, retained
 * delta.y direction normalization, angular stepping, stop condition, and all
 * nine calls agree with retail. The 68-byte residue is repeated projection
 * loads/sqrt-path emission, two inverted equivalent latch branches, register
 * allocation, and scheduling (1580 versus 1512 bytes).
 */
void turn_to_face_exterior_door(void) {
    KonquestObject* building;
    KonquestChildObject* door;
    KonquestWaypoint* path;
    MkObj* hero;
    Vec delta;
    Vec angles;
    float distance_squared;
    float previous_distance_squared;
    float inverse_length;
    float angle_difference;
    int turn_direction;
    int done;

    turn_direction = 1;
    done = 0;
    building = find_object_by_uid_inline(get_building_id_for_exterior());
    if (building == 0) {
        return;
    }
    door = find_child_by_enumeration_inline(
        building, get_primary_door_enum_for_exterior());
    if (door == 0) {
        return;
    }
    path = door->path_waypoints;
    if (path == 0) {
        return;
    }

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }

    delta.x = hero->pos.value.x - path[0].position.x;
    delta.y = hero->pos.value.y - path[0].position.y;
    delta.z = hero->pos.value.z - path[0].position.z;
    {
        float side_edge_x;
        float side_edge_z;
        float side_x;
        float side_z;

        side_edge_z = path[1].position.z - path[0].position.z;
        side_edge_x = path[1].position.x - path[0].position.x;
        side_x = delta.x;
        side_z = delta.z;
        if (side_edge_z * side_x - side_edge_x * side_z < 0.0f) {
            turn_direction = 0;
        }
    }

    distance_squared = door_path_distance_squared(hero, path, &delta);
    if (distance_squared > 0.8f) {
        if (turn_direction != 0) {
            transition_to_anim_script_frame(
                0.05f, 0.0f, konquest_pdata->hero_anim,
                (AnimScript*)konquest_animations[30], 0);
        } else {
            transition_to_anim_script_frame(
                0.05f, 0.0f, konquest_pdata->hero_anim,
                (AnimScript*)konquest_animations[29], 0);
        }
    }

    while (done == 0) {
        int turning;

        turning = 0;
        previous_distance_squared = distance_squared;
        distance_squared = door_path_distance_squared(hero, path, &delta);
        delta.x = path[2].position.x - hero->pos.value.x;
        delta.z = path[2].position.z - hero->pos.value.z;
        inverse_length = konquest_inverse_length(
            delta.z * delta.z +
            (delta.x * delta.x + delta.y * delta.y));
        delta.x *= inverse_length;
        delta.y *= inverse_length;
        delta.z *= inverse_length;
        v3_to_xy_ang(&angles, &delta);
        angle_difference = ang_sub_ang(angles.y, hero->ang.y);
        if ((angle_difference >= 0.0f
                 ? angle_difference
                 : -angle_difference) > 0.04f) {
            if (angle_difference < 0.0f) {
                hero->ang.y -= 0.04f;
            } else {
                hero->ang.y += 0.04f;
            }
            turning = 1;
        } else {
            hero->ang.y = angles.y;
        }
        if (distance_squared > previous_distance_squared) {
            hero_stop_moving();
            if (turning == 0) {
                done = 1;
            }
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
}
static inline KonquestChildObject* find_trigger_door(
    KonquestTriggerObject* object, int enumeration) {
    MkPtr* link;

    if (object->doors != 0) {
        link = object->doors;
        while (link != 0) {
            KonquestChildObject* door;

            door = (KonquestChildObject*)link->hdr;
            if (link->instance != door->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (door->definition->enumeration_index >= 0 &&
                enumeration ==
                    konquest_pdata->region_table
                        ->enumerations[door->definition->enumeration_index]
                        .enumeration) {
                return door;
            }
            link = link->next;
        }
    }
    return 0;
}


/*
 * Soft ceiling: konquest_open_door ~88.0% -- the trigger latch, stale-safe
 * door lookup, enumeration test, and call ABI agree. Residue is paired-save
 * emission, one folded latch join, and scheduling of the two argument moves.
 */
void konquest_open_door(int enumeration, int remain_open) {
    KonquestTriggerScriptPdata* pdata;
    KonquestTriggerStruct* trigger;
    KonquestChildObject* door;

    pdata = (KonquestTriggerScriptPdata*)pdata_of_proc(aproc);
    trigger = pdata->trigger;
    if (trigger != 0) {
        if (trigger->hdr.instance == pdata->trigger_instance) {
            /* Valid trigger latch. */
        } else {
            trigger = 0;
        }
    } else {
        trigger = 0;
    }
    if (trigger != 0 && trigger->object != 0) {
        door = find_trigger_door(trigger->object, enumeration);
        if (door != 0) {
            konquest_open_door_sobj(door, remain_open);
        }
    }
}

static inline KonquestChildObject* find_door_partner_inline(
    KonquestChildObject* door) {
    MkPtr* link;
    int uid;
    int partner_uid;
    MkPtr* next;

    if (door == 0) {
        return 0;
    }
    uid = door->owner->uid;
    partner_uid = konquest_pdata->region_table
                      ->enumerations[door->definition->enumeration_index]
                      .partner_uid;
    if (konquest_pdata->door_objects != 0) {
        link = konquest_pdata->door_objects;
        while (link != 0) {
            KonquestChildObject* candidate;

            candidate = (KonquestChildObject*)link->hdr;
            if (link->instance != candidate->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                int candidate_uid;
                int candidate_enumeration_index;

                candidate_uid = candidate->owner->uid;
                candidate_enumeration_index =
                    candidate->definition->enumeration_index;
                if (candidate_uid == uid &&
                    partner_uid ==
                        konquest_pdata->region_table
                            ->enumerations[candidate_enumeration_index]
                            .partner_index) {
                    return candidate;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

/*
 * Near match: konquest_open_door_sobj 87.98% (680 versus 664 bytes). Both
 * door-process lifecycles, partner enumeration/search, stale-link cleanup,
 * process flags, pdata initialization, and all seven calls agree. Residue is
 * two equivalent process-latch joins, GPR save aggregation, and scheduling.
 */
void konquest_open_door_sobj(
    KonquestChildObject* door, int remain_open) {
    KonquestDoorPdata* pdata;
    KonquestChildObject* partner;
    MkProc* proc;
    int partner_uid;

    proc = (MkProc*)door->state_object;
    if (proc != 0) {
        if (proc->instance != door->state_object_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc != 0) {
        pdata = (KonquestDoorPdata*)pdata_of_proc(proc);
        pdata->open_ticks = 0x1E0;
        pdata->play_sound = 0;
        xfer_proc(proc, (MkProcEntryFn)p_konquest_open_door);
    } else {
        proc = _create_mkproc_generic_tinystack(
            0xA018, 0x1F, (MkProcEntryFn)p_konquest_open_door,
            sizeof(*pdata), (void**)&pdata);
        if (proc != 0) {
            door->state_object = &proc->hdr;
            door->state_object_instance = proc->instance;
            proc->flags_bits.use_game_speed = 1;
            pdata->door = door;
            pdata->open_ticks = 0x1E0;
            pdata->remain_open = remain_open;
            pdata->play_sound = 1;
        }
    }

    partner_uid = konquest_pdata->region_table
                      ->enumerations[door->definition->enumeration_index]
                      .partner_uid;
    if (partner_uid == -1) {
        return;
    }
    partner = find_door_partner_inline(door);
    if (partner == 0) {
        return;
    }

    proc = (MkProc*)partner->state_object;
    if (proc != 0) {
        if (proc->instance != partner->state_object_instance) {
            proc = 0;
        }
    } else {
        proc = 0;
    }
    if (proc != 0) {
        pdata = (KonquestDoorPdata*)pdata_of_proc(proc);
        pdata->open_ticks = 0x1E0;
        pdata->play_sound = 0;
        xfer_proc(proc, (MkProcEntryFn)p_konquest_open_door);
    } else {
        proc = _create_mkproc_generic_tinystack(
            0xA018, 0x1F, (MkProcEntryFn)p_konquest_open_door,
            sizeof(*pdata), (void**)&pdata);
        if (proc != 0) {
            partner->state_object = &proc->hdr;
            partner->state_object_instance = proc->instance;
            proc->flags_bits.use_game_speed = 1;
            pdata->door = partner;
            pdata->open_ticks = 0x1E0;
            pdata->remain_open = remain_open;
            pdata->play_sound = 1;
        }
    }
}

static void p_konquest_open_door(void) {
    KonquestDoorPdata* pdata;

    pdata = (KonquestDoorPdata*)pdata_of_proc(aproc);
    object_transition_to_state(
        pdata->door, 1, pdata->play_sound);
    if (pdata->remain_open != 0) {
        return;
    }
    _mkproc_sleep_ticks = (float)pdata->open_ticks;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    object_transition_to_state(pdata->door, 0, 1);
}

static void object_transition_to_state(
    KonquestChildObject* record, int state, int play_sound) {
    KonquestObjectState* state_data;
    Vec target_angles;
    Vec target_position;
    Vec* angles;
    Vec* base_position;
    float position_x;
    float position_y;
    float position_z;
    float angle_x;
    float angle_y;
    float angle_z;
    float position_length_squared;
    float angle_length_squared;
    float position_ticks;
    float angle_ticks;
    float speed;
    float inverse;
    int enumeration_index;

    if (konquest_pdata->region_table->enumerations == 0) {
        return;
    }

    angles = &record->angles;
    base_position = &record->base_position;
    enumeration_index = record->binding->enumeration_index;
    if (enumeration_index < 0) {
        return;
    }

    rotate_xz(
        &target_position,
        &konquest_pdata->region_table->enumerations[enumeration_index]
             .states[state]
             .position,
        record->base_angles.y);
    state_data = &konquest_pdata->region_table
                      ->enumerations[enumeration_index]
                      .states[state];
    target_position.y = state_data->position.y;
    target_position.x += base_position->x;
    target_position.y += base_position->y;
    target_position.z += base_position->z;
    target_angles.x = record->base_angles.x + state_data->angles.x;
    target_angles.y = record->base_angles.y + state_data->angles.y;
    target_angles.z = record->base_angles.z + state_data->angles.z;

    position_x = target_position.x - record->position.x;
    position_y = target_position.y - record->position.y;
    position_z = target_position.z - record->position.z;
    angle_x = target_angles.x - angles->x;
    angle_y = target_angles.y - angles->y;
    angle_z = target_angles.z - angles->z;

    position_length_squared =
        position_x * position_x + position_y * position_y +
        position_z * position_z;
    position_ticks = konquest_fast_sqrt(position_length_squared);
    state_data = &konquest_pdata->region_table
                      ->enumerations[enumeration_index]
                      .states[state];
    speed = konquest_fast_sqrt(
        state_data->position_speed.x * state_data->position_speed.x +
        state_data->position_speed.y * state_data->position_speed.y +
        state_data->position_speed.z * state_data->position_speed.z);
    if (speed > 0.0f) {
        position_ticks = (float)floor(position_ticks / speed);
    }
    inverse = konquest_inverse_length(position_length_squared);
    position_x *= inverse;
    position_y *= inverse;
    position_z *= inverse;
    position_x *= speed;
    position_y *= speed;
    position_z *= speed;

    angle_length_squared =
        angle_x * angle_x + angle_y * angle_y + angle_z * angle_z;
    angle_ticks = konquest_fast_sqrt(angle_length_squared);
    state_data = &konquest_pdata->region_table
                      ->enumerations[enumeration_index]
                      .states[state];
    speed = konquest_fast_sqrt(
        state_data->angle_speed.x * state_data->angle_speed.x +
        state_data->angle_speed.y * state_data->angle_speed.y +
        state_data->angle_speed.z * state_data->angle_speed.z);
    if (speed > 0.0f) {
        angle_ticks = (float)floor(angle_ticks / speed);
    }
    inverse = konquest_inverse_length(angle_length_squared);
    angle_x *= inverse;
    angle_y *= inverse;
    angle_z *= inverse;
    angle_x *= speed;
    angle_y *= speed;
    angle_z *= speed;

    if (konquest_current_game_mode() != 4 &&
        konquest_pdata->region_table->enumerations[enumeration_index]
                .states[state]
                .sound != -1 &&
        play_sound != 0) {
        float volume;

        volume = get_volume_from_distance(base_position, 25.0f, 10.0f);
        if (volume != 0.0f) {
            pan_vol_snd_req(
                konquest_pdata->region_table
                    ->enumerations[enumeration_index]
                    .states[state]
                    .sound,
                get_pan_value(base_position), volume);
        }
    }

    while (position_ticks > 0.0f || angle_ticks > 0.0f) {
        MKMATRIX* matrix;
        MkSobj* object;

        if (position_ticks > 0.0f) {
            record->position.x += position_x * game_speed;
            record->position.y += position_y * game_speed;
            record->position.z += position_z * game_speed;
            position_ticks -= game_speed;
        }
        if (angle_ticks > 0.0f) {
            angles->x += angle_x * game_speed;
            angles->y += angle_y * game_speed;
            angles->z += angle_z * game_speed;
            angle_ticks -= game_speed;
        }

        matrix = 0;
        if (record->matrix_index >= 0) {
            if (record->binding->palette == 0) {
                object = record->binding->object;
                if (object != 0) {
                    if (object->hdr.instance !=
                        record->binding->object_instance) {
                        object = 0;
                    }
                } else {
                    object = 0;
                }
                if (object != 0) {
                    matrix = &object->frame->modelling;
                }
            } else if (record->matrix_index >= 0) {
                matrix = &record->binding->palette
                              ->matrices[record->matrix_index];
            }
        }
        if (matrix != 0) {
            YXZ_angles_to_MKMATRIX(angles, matrix);
            matrix->pos.x = record->position.x;
            matrix->pos.y = record->position.y;
            matrix->pos.z = record->position.z;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    record->position.x = target_position.x;
    record->position.y = target_position.y;
    record->position.z = target_position.z;
    angles->x = target_angles.x;
    angles->y = target_angles.y;
    angles->z = target_angles.z;

    {
        MKMATRIX* matrix;
        MkSobj* object;

        matrix = 0;
        if (record->matrix_index >= 0) {
            if (record->binding->palette == 0) {
                object = record->binding->object;
                if (object != 0) {
                    if (object->hdr.instance !=
                        record->binding->object_instance) {
                        object = 0;
                    }
                } else {
                    object = 0;
                }
                if (object != 0) {
                    matrix = &object->frame->modelling;
                }
            } else if (record->matrix_index >= 0) {
                matrix = &record->binding->palette
                              ->matrices[record->matrix_index];
            }
        }
        if (matrix != 0) {
            YXZ_angles_to_MKMATRIX(&target_angles, matrix);
            matrix->pos.x = record->position.x;
            matrix->pos.y = record->position.y;
            matrix->pos.z = record->position.z;
        }
    }
    record->state = state;
}

/*
 * Soft ceiling: the guarded list traversal, stale-link cleanup, enumeration
 * lookup, and return are instruction-exact. The remaining eight bytes are
 * scalar r30/r31 saves/restores instead of retail's stmw/lmw pair.
 */
KonquestChildObject* find_child_subobject_by_enumeration(
    KonquestObject* object, int enumeration) {
    MkPtr* link;

    if (object->list_4C != 0) {
        link = object->list_4C;
        while (link != 0) {
            KonquestChildObject* child;

            child = (KonquestChildObject*)link->hdr;
            if (link->instance != child->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                int index;

                index = child->definition->enumeration_index;
                if (index >= 0 &&
                    konquest_pdata->region_table->enumerations[index]
                            .enumeration == enumeration) {
                    return child;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

/*
 * Soft ceiling: is_leaving_area ~98.1% -- all range math, state transitions,
 * access widths, and returns match. Residue is one folded hero-latch branch
 * and the local zero-vector relocation label.
 */
static int is_leaving_area(KonquestTriggerStruct* trigger) {
    MkObj* hero;
    int in_range;

    if (trigger == 0) {
        return 0;
    }
    hero = konquest_pdata->hero_object;
    {
        Vec distance = {0.0f, 0.0f, 0.0f};

        if (hero != 0) {
            if (hero->hdr.instance == konquest_pdata->hero_instance) {
                /* Valid hero latch. */
            } else {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero == 0) {
            in_range = 0;
        } else if (trigger == 0) {
            in_range = 0;
        } else {
            distance.x =
                hero->pos.value.x - trigger->owned_data->position.x;
            distance.z =
                hero->pos.value.z - trigger->owned_data->position.z;
            if (distance.x * distance.x + distance.y * distance.y +
                    distance.z * distance.z <
                trigger->owned_data->radius * trigger->owned_data->radius) {
                in_range = 1;
            } else {
                in_range = 0;
            }
        }
    }
    if (in_range == 0) {
        if (trigger->flag_bits.bit1) {
            trigger->flag_bits.bit1 = 0;
            return 1;
        }
    } else {
        trigger->flag_bits.bit1 = 1;
    }
    return 0;
}

static int is_button_pressed(KonquestTriggerStruct* button) {
    if (button == 0) {
        return 0;
    }
    if (button->flag_bits.pressed) {
        button->flag_bits.pressed = 0;
        return 1;
    }
    return 0;
}

/*
 * Soft ceiling: is_in_range ~97.4% -- the math and guards are exact; residue
 * is one folded ownership-latch join plus the local constant's relocation
 * label, whose three loaded words are identical.
 */
static int is_in_range(KonquestTriggerStruct* trigger) {
    Vec distance = {0.0f, 0.0f, 0.0f};
    MkObj* hero;

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero == 0) {
        return 0;
    }
    if (trigger == 0) {
        return 0;
    }

    distance.x = hero->pos.value.x - trigger->owned_data->position.x;
    distance.z = hero->pos.value.z - trigger->owned_data->position.z;
    return distance.x * distance.x + distance.y * distance.y +
               distance.z * distance.z <
           trigger->owned_data->radius * trigger->owned_data->radius;
}

/*
 * Soft ceiling: p_run_trigger_script ~89.4%. The trigger-instance latches,
 * type-specific script parameters, command execution, post-script recheck,
 * flag clear, and active-trigger cleanup agree with retail. Residue is the
 * pdata/clear-flag nonvolatile-register swap, two folded latch joins,
 * individual saves/restores instead of stmw/lmw, and the -1.0f relocation
 * label; those forms produce 368 bytes versus retail's 360.
 */
static float p_run_trigger_script(void) {
    KonquestTriggerScriptPdata* pdata;
    int clear_active_trigger;
    KonquestTriggerStruct* trigger;
    KonquestTriggerDefinition* definition;

    pdata = (KonquestTriggerScriptPdata*)pdata_of_proc(aproc);
    trigger = pdata->trigger;
    if (trigger != 0) {
        if (trigger->hdr.instance != pdata->trigger_instance) {
            trigger = 0;
        }
    } else {
        trigger = 0;
    }
    clear_active_trigger = 0;
    if (trigger == 0) {
        return -1.0f;
    }

    definition = trigger->owned_data;
    if (definition->type == 2) {
        clear_active_trigger = 1;
    }
    if (definition->script_index != 0) {
        if (definition->type == 2) {
            cmdscript_set_parameters(
                active_cmdscript, 1, definition->field_24->field_10);
            cmdscript_setup_execution(
                konquest_pdata->script_owner,
                trigger->owned_data->script_index);
            cmdscript_execute(konquest_pdata->script_owner);
        } else {
            cmdscript_set_parameters(active_cmdscript, 1);
            cmdscript_setup_execution(
                konquest_pdata->script_owner,
                trigger->owned_data->script_index);
            cmdscript_execute(konquest_pdata->script_owner);
        }
    }

    trigger = pdata->trigger;
    if (trigger != 0) {
        if (trigger->hdr.instance != pdata->trigger_instance) {
            trigger = 0;
        }
    } else {
        trigger = 0;
    }
    if (trigger != 0) {
        trigger->flag_bits.bit3 = 0;
    }
    if (clear_active_trigger != 0) {
        konquest_pdata->active_trigger = 0;
        konquest_pdata->active_trigger_instance = 0;
    }
    return -1.0f;
}

/*
 * Soft ceiling: fire_trigger ~93.8%. The compiler inlines the typed
 * definition-pointer search and execute_trigger, matching retail's stale-link
 * cleanup, process lifecycle, pdata, active-trigger, flag, and state updates.
 * Residue is one folded process-latch join plus individual saves/restores;
 * the source is 512 bytes versus retail's 500.
 */
/*
 * Soft ceiling: execute_trigger ~89.3%. Process ownership validation,
 * transfer/creation, typed pdata initialization, active-trigger tracking,
 * bitfield update, and type-1 state clear agree with retail. Residue is
 * nonvolatile-register coloring, one folded latch join, and individual
 * saves/restores; the source is 308 bytes versus retail's 304.
 */
static inline void execute_trigger_inline(KonquestTriggerStruct* trigger) {
    MkProc* proc;
    KonquestTriggerScriptPdata* pdata;

    if (trigger->owned_data->type != 2 ||
        konquest_pdata->active_trigger == 0) {
        proc = trigger->script_proc;
        if (proc != 0) {
            if (proc->instance == trigger->script_proc_instance) {
                /* Valid script process latch. */
            } else {
                proc = 0;
            }
        } else {
            proc = 0;
        }
        if (proc != 0) {
            xfer_proc(proc, p_run_trigger_script);
        } else {
            proc = _create_mkproc_generic_bigstack(
                0x9019, 0x18, p_run_trigger_script, sizeof(*pdata),
                (void**)&pdata);
            if (proc != 0) {
                set_process_as_scriptable(proc);
                trigger->script_proc = proc;
                trigger->script_proc_instance = proc->instance;
                pdata->trigger = trigger;
                pdata->trigger_instance = trigger->hdr.instance;
            }
        }
        if (proc != 0) {
            if (trigger->owned_data->type == 2) {
                konquest_pdata->active_trigger = trigger;
                konquest_pdata->active_trigger_instance =
                    trigger->hdr.instance;
            }
            trigger->flag_bits.bit3 = 1;
            if (trigger->owned_data->type == 1) {
                trigger->owned_data->state = 0;
            }
        }
    }
}

void fire_trigger(KonquestTriggerDefinition* definition) {
    KonquestTriggerStruct* trigger;

    trigger = find_trigger_by_definition(definition);
    if (trigger != 0) {
        execute_trigger_inline(trigger);
    }
}

void execute_trigger(KonquestTriggerStruct* trigger) {
    execute_trigger_inline(trigger);
}
static inline void play_pui_effect_now(
    KonquestPuiRuntime* pui, unsigned int handle) {
    KonquestChestOwner* owner;
    MkObj* object;
    MkObj* emitter_object;
    MkSobj* child;
    MkPfx* effect;
    RwSphere sphere;
    unsigned int emitter;

    object = pui->render_object;
    if (object != 0) {
        if (object->hdr.instance != pui->render_object_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    owner = pui->owner;
    if (owner != 0) {
        if (owner->hdr.instance != pui->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }
    if (object != 0 && owner != 0) {
        sphere.center.x = object->pos.value.x;
        sphere.center.y = object->pos.value.y;
        sphere.center.z = object->pos.value.z;
        sphere.radius = owner->interaction->radius;
        if (RwCameraFrustumTestSphere(Camera, &sphere) != 0) {
            effect = pfx_from_handle(handle);
            emitter = fx_next_emitter(handle);
            if (emitter != 0) {
                emitter_object = (MkObj*)pfx_get_emitter_obj(
                    effect, emitter_id_from_handle(emitter));
                child = obj_find_sobj_by_id(object, 1);
                emitter_object->pos.value.x =
                    object->pos.value.x + child->pos.x;
                emitter_object->pos.value.y =
                    object->pos.value.y + child->pos.y;
                emitter_object->pos.value.z =
                    object->pos.value.z + child->pos.z;
                update_mkobj(emitter_object);
                fx_restart_emit(emitter);
                fx_resume_emit(emitter);
            }
        }
    }
}

static inline void play_pui_effect_sequence_now(
    KonquestPuiRuntime* pui, KonquestPuiPfxSequenceRow* sequence) {
    KonquestChestOwner* owner;
    MkObj* object;
    MkSobj* child;
    KonquestPuiPfxSequencePdata* pdata;
    RwSphere sphere;

    object = pui->render_object;
    if (object != 0) {
        if (object->hdr.instance != pui->render_object_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    owner = pui->owner;
    if (owner != 0) {
        if (owner->hdr.instance != pui->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }
    if (object != 0 && owner != 0) {
        sphere.center.x = object->pos.value.x;
        sphere.center.y = object->pos.value.y;
        sphere.center.z = object->pos.value.z;
        sphere.radius = owner->interaction->radius;
        if (RwCameraFrustumTestSphere(Camera, &sphere) != 0 &&
            _create_mkproc_generic_nostack(
                0xA01D, 0x1F, p_pui_pfx_sequence,
                sizeof(*pdata), (void**)&pdata) != 0) {
            zero_pdata_payload(sizeof(*pdata), &pdata->hdr);
            pdata->sequence = sequence;
            pdata->index = 0;
            pdata->delay = pdata->sequence->delay;
            child = obj_find_sobj_by_id(object, 1);
            pdata->position.x = object->pos.value.x + child->pos.x;
            pdata->position.y = object->pos.value.y + child->pos.y;
            pdata->position.z = object->pos.value.z + child->pos.z;
        }
    }
}


/*
 * Near match: both effect paths, three independent frustum spheres, clone and
 * emitter ownership, delayed visibility, material alpha, aligned collision
 * cylinder, and foreground insertion agree. The 36-byte deficit is limited
 * to equivalent latch/early-return branch folding and register scheduling.
 */
static void handle_trigger_preprocess(KonquestTriggerStruct* trigger) {
    KonquestPuiRuntime* pui;
    MkObj* object;
    MkObj* clone_object;
    MkSobj* attachment;
    MkSobj* child;
    RwSphere sphere;
    Vec center;
    CollisionShape shape __attribute__((aligned(16)));

    if (trigger->owned_data->type != 2) {
        return;
    }
    pui = trigger->owned_data->pui;
    if (pui->flag_bits.bit6) {
        return;
    }

    object = pui->render_object;
    if (object != 0) {
        if (object->hdr.instance != pui->render_object_instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    attachment = obj_find_sobj_by_id(object, 1);
    child = obj_find_sobj_by_id(object, 2);
    if (child != 0) {
        child->flags_08_bits.bit6 = 1;
        child->flags_08_bits.bit5 = 0;
        sobj_set_priority(child, 0x13);
    }
    child = obj_find_sobj_by_id(object, 3);
    if (child != 0) {
        child->flags09_bits.bit5 = 1;
    }

    if ((int)pui->effect_60 != 0) {
        if (pui->flag_bits.bit0) {
            pfx_bind_render_to_sobj(
                pfx_from_handle(pui->effect_60), attachment, 0);
            fx_resume_emit(pui->effect_60);
        } else if (pui->effect_clone == 0) {
            pui->effect_clone =
                pfx_create_clone(pfx_from_handle(pui->effect_60));
            clone_object = pfx_clone_bind_render_to_new_obj(
                pui->effect_clone, 0xA00E);
            clone_object->flags_08_bits.airborne = 1;
            clone_object->pos.value.x =
                object->pos.value.x + attachment->pos.x;
            clone_object->pos.value.y =
                object->pos.value.y + attachment->pos.y;
            clone_object->pos.value.z =
                object->pos.value.z + attachment->pos.z;
        }
    }
    if ((int)pui->effect_64 != 0) {
        pfx_bind_render_to_sobj(
            pfx_from_handle(pui->effect_64), attachment, 0);
        fx_resume_emit(pui->effect_64);
    }

    if (pui->flag_bits.bit3 && !pui->flag_bits.bit2) {
        sphere.center.x = object->pos.value.x;
        sphere.center.y = object->pos.value.y;
        sphere.center.z = object->pos.value.z;
        sphere.radius = trigger->owned_data->radius;
        if (RwCameraFrustumTestSphere(Camera, &sphere) != 0) {
            if (pui->flag_bits.bit4) {
                play_pui_effect_now(pui, pui->attached_effect);
            } else {
                play_pui_effect_sequence_now(pui, pui->effect_sequence);
            }
        }
    }
    if (pui->flag_bits.bit5 && !pui->flag_bits.bit2) {
        hide_obj(object);
        pui->fade_delay = pui->spawn_delay;
    }
    pui->flag_bits.bit6 = 1;
    if (pui->flag_bits.bit2) {
        pui->alpha = 0xFF;
    }
    obj_for_all_atomics_set_material_alpha(object, pui->alpha);

    if (pui->item->type == 0 && pui->collision_object == 0) {
        center.x = object->pos.value.x;
        center.y = object->pos.value.y;
        center.z = object->pos.value.z;
        center.y = 0.0f;
        build_col_shape_vertical_cylinder(&shape, &center, 1.0f, 3.0f);
        pui->collision_object = add_shape_to_global_collision_list(
            &shape, 0x80010000);
    }
    insert_fgnd_mkobj(object);
}

/*
 * Soft ceiling: p_pui_pfx_sequence ~87.4% -- the complete body and GPR/FPR
 * allocation match. Residue is individual GPR saves/restores in place of
 * stmw/lmw plus local float-constant relocation labels.
 */
static float p_pui_pfx_sequence(void) {
    unsigned int row_count;
    KonquestPuiPfxSequencePdata* pdata;

    pdata = (KonquestPuiPfxSequencePdata*)apdata;
    row_count = get_row_count_for_table_by_pointer(
        konquest_pdata->script_owner, pdata->sequence);
    pdata->delay -= get_game_speed();
    if (pdata->delay <= 0.0f) {
        unsigned int effect;
        unsigned int emitter;
        MkPfx* particle;
        MkObj* object;

        effect = fx_by_owner(pdata->sequence[pdata->index].effect_owner, 4);
        emitter = fx_next_emitter(effect);
        if (emitter == 0) {
            return 1.0f;
        }
        particle = pfx_from_handle(effect);
        object = (MkObj*)pfx_get_emitter_obj(
            particle, emitter_id_from_handle(emitter));
        object->pos.value.x = pdata->position.x;
        object->pos.value.y = pdata->position.y;
        object->pos.value.z = pdata->position.z;
        update_mkobj(object);
        fx_reset_emit(emitter);
        fx_resume_emit(emitter);
        {
            unsigned int next_index;

            next_index = pdata->index + 1;
            pdata->index = next_index;
            if (next_index == row_count) {
                return -1.0f;
            }
        }
        pdata->delay = pdata->sequence[pdata->index].delay;
    }
    return 1.0f;
}
static inline void remove_trigger_from_world(
    KonquestTriggerStruct* trigger) {
    if (trigger != 0) {
        if (trigger->owned_data->type == 2) {
            struct KonquestPuiRuntime* pui;
            MkObj* render_object;

            pui = (struct KonquestPuiRuntime*)trigger->owned_data->field_24;
            render_object = pui->render_object;
            if (render_object != 0) {
                if (render_object->hdr.instance !=
                    pui->render_object_instance) {
                    render_object = 0;
                }
            } else {
                render_object = 0;
            }

            if ((int)pui->effect_60 != 0) {
                fx_reset_emit(pui->effect_60);
            }
            if ((int)pui->effect_64 != 0) {
                fx_reset_emit(pui->effect_64);
            }
            if (pui->effect_clone != 0) {
                if (pui->effect_clone->hdr.instance != 0) {
                    pui->effect_clone->hdr.typed_vtbl->destroy(
                        &pui->effect_clone->hdr);
                }
                pui->effect_clone = 0;
            }

            pui->flag_bits.bit6 = 0;
            pui->alpha = 0;
            if (pui->item->type == 0) {
                MkHdr* header;

                header = pui->collision_object != 0
                             ? as_mkhdr(&pui->collision_object->hdr)
                             : 0;
                if (header->instance != 0) {
                    MkHdr* destroy_arg;

                    destroy_arg = pui->collision_object != 0
                                      ? as_mkhdr(&pui->collision_object->hdr)
                                      : 0;
                    header = pui->collision_object != 0
                                 ? as_mkhdr(&pui->collision_object->hdr)
                                 : 0;
                    header->typed_vtbl->destroy(destroy_arg);
                }
                pui->collision_object = 0;
            }
            remove_fgnd_mkobj(render_object);
        }

        mk_pull_discard(
            &trigger->hdr, &konquest_pdata->temporary_triggers);
        trigger->flag_bits.bit2 = 0;
    }
}


/*
 * Near match: stale-list cleanup and the full shared trigger-removal helper
 * agree with retail. The 12-byte delta is three folded branch diamonds; the
 * remaining instruction differences are nonvolatile-register coloring.
 */
void delete_triggers_from_tile(int tile_index) {
    MkPtr* link;

    if (konquest_pdata->triggers != 0) {
        link = konquest_pdata->triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (trigger->tile_index == tile_index &&
                trigger->flag_bits.bit4 &&
                trigger->owned_data->type != 2) {
                remove_trigger_from_world(trigger);
                if (trigger->hdr.instance != 0) {
                    trigger->hdr.typed_vtbl->destroy(&trigger->hdr);
                }
            }
            link = link->next;
        }
    }
}

/*
 * Near-exact match at retail size: allocation, initialization, the inlined
 * tile lookup, flags, and insertion agree. Only local float relocation labels
 * and scratch-register selection inside the tile lookup differ.
 */
KonquestTriggerStruct* add_temporary_trigger(
    int id, int type, unsigned int flags, int state,
    unsigned int script_index, float x, float y, float z, float radius) {
    KonquestTriggerStruct* trigger;
    int row;
    int column;
    int tile_index;

    trigger = (KonquestTriggerStruct*)get_mkhdr(
        &vtbl_trigger_struct, sizeof(*trigger));
    if (trigger != 0) {
        zero_pdata_payload(sizeof(*trigger), trigger);
    }
    /* Retail initializes both allocations before its late trigger check. */
    trigger->owned_data =
        (KonquestTriggerDefinition*)get_mem(sizeof(*trigger->owned_data));
    trigger->id = id;
    trigger->owned_data->type = type;
    trigger->owned_data->position.x = x;
    trigger->owned_data->position.y = y;
    trigger->owned_data->position.z = z;
    if (trigger->owned_data->position.x >= 1000.0f) {
        tile_index =
            konquest_pdata->tile_width * konquest_pdata->tile_height;
    } else {
        row = (int)((trigger->owned_data->position.z +
                     konquest_pdata->tile_origin_z) /
                    60.0f);
        if (row < 0) {
            tile_index = -1;
        } else if (row >= konquest_pdata->tile_height) {
            tile_index = -1;
        } else {
            column = (int)((trigger->owned_data->position.x +
                            konquest_pdata->tile_origin_x) /
                           60.0f);
            if (column < 0) {
                tile_index = -1;
            } else if (column >= konquest_pdata->tile_width) {
                tile_index = -1;
            } else {
                tile_index = column + row * konquest_pdata->tile_width;
            }
        }
    }
    trigger->tile_index = tile_index;
    trigger->owned_data->radius = radius;
    trigger->owned_data->state = state;
    trigger->owned_data->flags = flags;
    trigger->owned_data->script_index = script_index;
    trigger->owned_data->field_24 = 0;
    trigger->object = 0;
    trigger->script_proc = 0;
    trigger->script_proc_instance = 0;
    trigger->flag_bits.bit7 = 0;
    trigger->flag_bits.pressed = 0;
    trigger->flag_bits.bit5 = 0;
    trigger->flag_bits.bit1 = 0;
    trigger->flag_bits.bit4 = 1;
    trigger->flag_bits.bit3 = 0;
    trigger->flag_bits.bit2 = 0;
    mk_insert(&trigger->hdr, &konquest_pdata->triggers);
    return trigger;
}

/*
 * Retail open-codes the tile lookup here rather than calling the public
 * get_tile_from_position helper defined later in this translation unit.
 */
void add_trigger_list_to_world(void) {
    void* header;
    void* footer;
    int first_index;
    int last_index;
    int table_index;

    header = get_data_table_by_name("trigger_table_header");
    footer = get_data_table_by_name("trigger_table_footer");
    first_index = (int)get_table_index_by_pointer(
        konquest_pdata->script_owner, header);
    last_index = (int)get_table_index_by_pointer(
        konquest_pdata->script_owner, footer);

    if (last_index - first_index - 1 >= 0) {
        for (table_index = first_index + 1;
             table_index < last_index;
             table_index++) {
            KonquestTriggerDefinition* definition;
            KonquestTriggerStruct* trigger;
            int row;
            int column;
            int tile_index;

            definition = (KonquestTriggerDefinition*)get_data_table(
                konquest_pdata->script_owner, table_index);
            trigger = (KonquestTriggerStruct*)get_mkhdr(
                &vtbl_trigger_struct, sizeof(*trigger));
            if (trigger != 0) {
                zero_pdata_payload(sizeof(*trigger), trigger);
            }

            trigger->id = table_index;
            if (definition->position.x >= 1000.0f) {
                tile_index =
                    konquest_pdata->tile_width * konquest_pdata->tile_height;
            } else {
                row = (int)((definition->position.z +
                             konquest_pdata->tile_origin_z) /
                            60.0f);
                if (row < 0) {
                    tile_index = -1;
                } else if (row >= konquest_pdata->tile_height) {
                    tile_index = -1;
                } else {
                    column = (int)((definition->position.x +
                                    konquest_pdata->tile_origin_x) /
                                   60.0f);
                    if (column < 0) {
                        tile_index = -1;
                    } else if (column >= konquest_pdata->tile_width) {
                        tile_index = -1;
                    } else {
                        tile_index = column + row * konquest_pdata->tile_width;
                    }
                }
            }
            trigger->tile_index = tile_index;
            trigger->owned_data = definition;
            trigger->table_name = get_name_of_table(
                konquest_pdata->script_owner, table_index);

            if ((definition->state & 2) != 0) {
                trigger->flag_bits.bit0 = 1;
                definition->state &= 1;
            }

            mk_insert(&trigger->hdr, &konquest_pdata->triggers);
        }
    }
}

/*
 * Soft ceiling: find_trigger_by_id ~88.8% -- both stale-safe list searches
 * match retail exactly; only individual GPR saves/restores differ from the
 * retail stmw/lmw pair.
 */
KonquestTriggerStruct* find_trigger_by_id(unsigned int id) {
    MkPtr* link;
    MkPtr* next;

    if (konquest_pdata->triggers != 0) {
        link = konquest_pdata->triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (trigger->id == id) {
                    return trigger;
                }
                link = link->next;
            }
        }
    }
    if (konquest_pdata->temporary_triggers != 0) {
        link = konquest_pdata->temporary_triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (trigger->id == id) {
                    return trigger;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

/*
 * Soft ceiling: assign_obj_to_trigger ~99.3% -- both stale-safe searches and
 * the final typed assignment match. Residue is only local GPR coloring in the
 * second trigger list and tile-object scan.
 */
void assign_obj_to_trigger(int object_uid, unsigned int trigger_id) {
    KonquestTriggerStruct* trigger;
    KonquestUidObject* object;

    trigger = find_trigger_by_id(trigger_id);
    if (trigger != 0) {
        object = find_tile_object_by_uid(object_uid);
        if (object != 0) {
            trigger->uid_object = object;
        }
    }
}

/*
 * Soft ceiling: the retail algorithm and ABI are reproduced. The remaining
 * difference is CodeWarrior's stmw/lmw choice versus individual nonvolatile
 * saves, with the corresponding harmless register allocation.
 */
void konquest_setup_pui_particle(
    const char* effect_name, int shared_render_object) {
    unsigned int handle;
    MkPfx* effect;
    MkObj* object;
    int emitter;

    handle = fx_by_owner(effect_name, 4);
    effect = find_pfx_by_handle(handle);
    fx_resume_emit(handle);

    for (emitter = 0; emitter < effect->slot_count; emitter++) {
        if (shared_render_object != 0) {
            object = get_mkobj_frame(0xA00E, 0);
            pfx_bind_render_to_obj(effect, object, 1);
        } else {
            object = pfx_bind_emitter_num_to_new_obj(
                effect, 0xA00E, emitter);
        }
        object->flags_08_bits.airborne = 1;
        object->pos.value.z = 0.0f;
        object->pos.value.x = 0.0f;
        object->pos.value.y = -1000.0f;
        update_mkobj(object);
    }
}
static inline unsigned int find_sobj_art_id_by_uid(int uid) {
    MkPtr* link;

    if (konquest_pdata->sobj_infos != 0) {
        link = konquest_pdata->sobj_infos;
        while (link != 0) {
            KonquestSobjInfo* info;

            info = (KonquestSobjInfo*)link->hdr;
            if (link->instance != info->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (info->uid == uid) {
                return info->art_id;
            }
            link = link->next;
        }
    }
    return 0;
}


/*
 * Near match at retail size: tile ownership, allocation, stale metadata
 * cleanup, collision-art lookup, render-record creation, scene latch, and the
 * child-frame/atomic walk agree. Residue is GPR allocation and equivalent
 * latch/list scheduling.
 */
void add_object_to_tile(
    int tile_index, int render_uid, int object_uid, float x, float y, float z,
    float angle) {
    KonquestTileRecord* tile;
    KonquestUidObject* object;
    KonquestRenderRecord* record;
    Vec position;

    if (tile_index < konquest_pdata->tile_width *
                         konquest_pdata->tile_height + 1) {
        tile = &konquest_pdata->tile_structs[tile_index];
    } else {
        tile = 0;
    }

    object = (KonquestUidObject*)_mwMemMalloc(
        konquest_objects_heap, sizeof(*object), 0x80, 0, 0, 0);
    if (object != 0) {
        object->hdr.vtbl = &vtbl_konquest_obj;
        mk_set_instance(&object->hdr.instance);
        zero_pdata_payload(sizeof(*object), object);
    }
    object->uid = object_uid;
    object->collision_art_id = find_sobj_art_id_by_uid(render_uid);
    object->attached_pfx_name[0] = '\0';
    object->tracked_sound = 0;
    object->tile_index = tile_index;
    mk_insert(&object->hdr, &tile->objects);

    position.x = x;
    position.y = y;
    position.z = z;
    record = create_konquest_sobj_for_konquest_obj(
        object, render_uid, &position, angle);
    if (render_uid < 1000) {
        KonquestSobjBinding* binding;
        KonquestUidObject* owner;
        MkSobj* model;
        RwFrame* frame;

        binding = record->binding;
        owner = record->owner;
        model = binding->object;
        if (model != 0) {
            if (model->hdr.instance == binding->object_instance) {
                /* Valid scene-object latch. */
            } else {
                model = 0;
            }
        } else {
            model = 0;
        }
        frame = model->frame->child;
        while (frame != 0) {
            RwLLLink* link;
            RwLLLink* sentinel;
            RwFrame* next_frame;

            next_frame = frame->next;
            sentinel = &frame->objectList.link;
            link = frame->objectList.link.next;
            while (link != sentinel) {
                RpAtomic* atomic;
                RwLLLink* next;

                next = link->next;
                atomic = (RpAtomic*)((char*)link -
                                     KONQUEST_OFFSETOF(RpAtomic, frameLink));
                if (atomic->object.type == 1) {
                    MkSobj* child;

                    child = ((MksobjPluginData*)((char*)atomic +
                                                 MksobjLocalOffset))
                                ->sobj;
                    if (child != 0) {
                        create_konquest_sobj_for_konquest_obj(
                            owner, child->id_flags & 0xFFF,
                            &record->base_position,
                            record->base_angles.y);
                    }
                }
                link = next;
            }
            frame = next_frame;
        }
    }
}
static inline KonquestSobjInfo* find_sobj_info_by_uid(int uid) {
    MkPtr* link;

    if (konquest_pdata->sobj_infos != 0) {
        link = konquest_pdata->sobj_infos;
        while (link != 0) {
            KonquestSobjInfo* info;

            info = (KonquestSobjInfo*)link->hdr;
            if (link->instance != info->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (info->uid == uid) {
                return info;
            }
            link = link->next;
        }
    }
    return 0;
}


/*
 * Near match at retail size: allocation, stale binding lookup, aligned matrix
 * transform, enumeration accounting, state initialization, and list ownership
 * agree. Residue is local enum/pdata GPR coloring, Vec-copy scheduling, and a
 * zero-float relocation label.
 */
static KonquestRenderRecord* create_konquest_sobj_for_konquest_obj(
    KonquestUidObject* owner, int uid, const Vec* position, float angle) {
    KonquestRenderRecord* record;

    record = (KonquestRenderRecord*)_mwMemMalloc(
        konquest_subobjects_heap, sizeof(*record), 0x80, 0, 0, 0);
    if (record != 0) {
        record->hdr.vtbl = &vtbl_konquest_sobj_struct;
        mk_set_instance(&record->hdr.instance);
        zero_pdata_payload(sizeof(*record), record);
    }

    record->info = find_sobj_info_by_uid(uid);
    record->owner = owner;
    record->matrix_index = -1;
    if (owner->render_records != 0) {
        MKMATRIX rotation __attribute__((aligned(16)));
        int enumeration_index;

        y_angle_to_MKMATRIX(&rotation, angle);
        v3_x_mat_add_v3(
            &record->base_position, &record->info->position, &rotation,
            position);
        enumeration_index = record->info->enumeration_index;
        if (enumeration_index != -1) {
            KonquestEnumerationEntry* enumeration;

            enumeration = &konquest_pdata->region_table
                               ->enumerations[enumeration_index];
            if (enumeration->field_04 < enumeration->field_00) {
                konquest_pdata->door_overflow_count++;
            }
            mk_insert_no_own(
                &record->hdr, &konquest_pdata->door_objects);
        }
    } else {
        record->base_position.x = position->x;
        record->base_position.y = position->y;
        record->base_position.z = position->z;
    }

    record->base_angles.x = 0.0f;
    record->base_angles.y = angle;
    record->base_angles.z = 0.0f;
    record->position.x = record->base_position.x;
    record->position.y = record->base_position.y;
    record->position.z = record->base_position.z;
    record->angles.x = record->base_angles.x;
    record->angles.y = record->base_angles.y;
    record->angles.z = record->base_angles.z;
    record->state = 0;
    record->state_object = 0;
    record->state_object_instance = 0;
    record->field_50 = 0;

    if (owner->render_records != 0) {
        mk_append_after_mkptr(&record->hdr, owner->render_records);
    } else {
        mk_insert(&record->hdr, &owner->render_records);
    }
    return record;
}

void* get_nth_tile_struct(int index) {
    if (index < konquest_pdata->tile_width * konquest_pdata->tile_height + 1) {
        return &konquest_pdata->tile_structs[index];
    }
    return 0;
}

/*
 * Soft ceiling: set_tile_visibility ~88.8% -- the latch, calls, fields,
 * arithmetic, and 0x174-byte CFG match. Residue is stmw/lmw versus individual
 * saves, row/width GPR coloring, and local constant/string relocation labels.
 */
void set_tile_visibility(int tile_index, int state) {
    KonquestTileRecord* tile;
    MkObj* tile_model;
    char tile_name[64];
    int row;
    int column;
    float origin;

    tile = &konquest_pdata->tile_structs[tile_index];
    tile_model = (MkObj*)konquest_pdata->tile_model.object;
    if (tile_model != 0) {
        if (tile_model->hdr.instance == konquest_pdata->tile_model.instance) {
            /* Valid tile-model latch. */
        } else {
            tile_model = 0;
        }
    } else {
        tile_model = 0;
    }
    if (tile_model == 0) {
        return;
    }

    tile->scene = obj_find_sobj_by_id(tile_model, (tile_index + 1) | 0x800);
    if (tile->scene == 0) {
        return;
    }

    set_true_clip_flag_on_sobj_and_children(tile->scene, 0);
    tile->scene->priority = 8;
    tile->index = tile_index;
    tile->visible = 0;
    row = tile_index / konquest_pdata->tile_width;
    column = tile_index % konquest_pdata->tile_width;
    origin = -konquest_pdata->tile_origin_x;
    tile->position.x =
        60.0f * (float)column + origin + 30.0f;
    tile->position.y = 0.0f;
    origin = -konquest_pdata->tile_origin_z;
    tile->position.z =
        60.0f * (float)row + origin + 30.0f;
    tile->objects = 0;
    tile->state = state;
    tile->collisions = 0;
    sprintf(tile_name, "TILE%i", tile_index + 1);
    tile->collision_art_id =
        get_artid_of_named_item_in_slot(0x60029, tile_name, 0);
    hide_sobj_and_children(tile->scene);
}

void vdestroy_trigger_struct(KonquestTriggerStruct* trigger) {
    if (((trigger->flags >> 4) & 1) && trigger->owned_data != 0) {
        free_mem(trigger->owned_data);
    }
    trigger->hdr.instance = 0;
    mkhdr_memfree(&trigger->hdr);
}

/*
 * Soft ceiling: get_door_path ~86.3% -- the complete body is exact; MWCC
 * emits three paired GPR saves/restores instead of retail's stmw/lmw pair.
 */
KonquestWaypoint* get_door_path(int door_id) {
    MkPtr* link;
    MkPtr* next;
    int region;
    int uid;

    region = ((int)(door_id & 0xF0000000u)) >> 28;
    uid = door_id & 0x0FFFFFFF;
    if (konquest_pdata->door_objects != 0) {
        link = konquest_pdata->door_objects;
        while (link != 0) {
            KonquestDoorObject* door;

            door = (KonquestDoorObject*)link->hdr;
            if (link->instance != door->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                int enumeration_index;

                if (door->owner->uid == uid) {
                    enumeration_index = door->definition->enumeration_index;
                    if (konquest_pdata->region_table
                            ->enumerations[enumeration_index]
                            .enumeration == region) {
                        return door->path_waypoints;
                    }
                }
                link = link->next;
            }
        }
    }
    return 0;
}

/*
 * Near match: allocation and stale-list traversal, both atomic-center paths,
 * partner averaging, fixed-angle normalization, all four waypoint records and
 * scripts, shared partner publication, trigger placement, and final counters
 * agree with retail. The complete 15-call sequence is identical. The 48-byte
 * residue is a separate byte-offset induction variable, three inverted
 * equivalent latch branches, and register scheduling (1376 versus 1328 bytes).
 */
static void generate_door_paths(void) {
    KonquestWaypoint* waypoints;
    MkPtr* link;
    int waypoint_index;
    int waypoint_offset;
    int generated_count;

    waypoint_index = 0;
    waypoint_offset = 0;
    generated_count = 0;
    if (konquest_pdata->door_overflow_count <= 0) {
        return;
    }

    waypoints = (KonquestWaypoint*)get_mem(
        konquest_pdata->door_overflow_count * 0x60);
    if (konquest_pdata->door_objects != 0) {
        link = konquest_pdata->door_objects;
        while (link != 0) {
            KonquestDoorObject* door;

            door = (KonquestDoorObject*)link->hdr;
            if (link->instance != door->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }

            {
                KonquestDoorObject* partner;
                KonquestSobjInfo* info;
                KonquestWaypoint* path;
                MkSobj* model;
                Vec forward = {0.0f, 0.0f, 1.0f};
                Vec center;
                Vec rotated_center;
                Vec partner_center;
                Vec direction = {0.0f, 0.0f, 0.0f};
                float angle;
                int enumeration_index;

                partner = 0;
                info = door->info;
                enumeration_index = info->enumeration_index;
                if (enumeration_index < 0 || door->path_waypoints != 0) {
                    link = link->next;
                    continue;
                }
                model = info->object;
                if (model != 0 && model->hdr.instance != info->object_instance) {
                    model = 0;
                }
                if (model != 0) {
                    rotate_xz(
                        &rotated_center,
                        (const Vec*)&RpAtomicGetWorldBoundingSphere(
                            model->atomic)->center,
                        door->base_angles.y);
                    center.x = door->base_position.x + rotated_center.x;
                    center.y = 0.0f;
                    center.z = door->base_position.z + rotated_center.z;
                } else {
                    center.x = 0.0f;
                    center.y = 0.0f;
                    center.z = 0.0f;
                }

                if (konquest_pdata->region_table
                        ->enumerations[enumeration_index]
                        .partner_uid != -1) {
                    partner = (KonquestDoorObject*)find_door_partner_inline(door);
                    if (partner != 0) {
                        MkSobj* partner_model;
                        float partner_x;
                        float partner_z;

                        partner_model = partner->info->object;
                        if (partner_model != 0 &&
                            partner_model->hdr.instance !=
                                partner->info->object_instance) {
                            partner_model = 0;
                        }
                        if (partner_model != 0) {
                            rotate_xz(
                                &partner_center,
                                (const Vec*)&RpAtomicGetWorldBoundingSphere(
                                    partner_model->atomic)->center,
                                partner->base_angles.y);
                            partner_x =
                                partner->base_position.x + partner_center.x;
                            partner_z =
                                partner->base_position.z + partner_center.z;
                        } else {
                            partner_x = 0.0f;
                            partner_z = 0.0f;
                        }
                        center.x = 0.5f * (center.x + partner_x);
                        center.z = 0.5f * (center.z + partner_z);
                    }
                }

                if (door != 0) {
                    angle = 0.000005992112f *
                        ((int)(166886.1f *
                               (door->base_angles.y +
                                konquest_pdata->region_table
                                    ->enumerations[enumeration_index]
                                    .angle)) &
                         0xFFFFF);
                    rotate_xz(&direction, &forward, angle);
                    forward.x = direction.x;
                    forward.y = direction.y;
                    forward.z = direction.z;
                } else {
                    angle = 0.0f;
                }

                path = (KonquestWaypoint*)((char*)waypoints + waypoint_offset);
                v3_add_v3_scaled(&path[0].position, &center, &forward, 3.0f);
                path[0].angle = 3.1415927f + angle;
                path[0].angle = 0.000005992112f *
                    ((int)(166886.1f * path[0].angle) & 0xFFFFF);
                path[0].flags = 0;
                path[0].script_function = (int)get_script_function_by_name(
                    konquest_pdata->script_owner, "open_door_and_enter");

                v3_add_v3_scaled(
                    &waypoints[waypoint_index + 1].position,
                    &center, &forward, -3.0f);
                path[1].angle = angle;
                path[1].flags = 0;
                path[1].script_function = 0;
                path[2].position.x = path[1].position.x;
                path[2].position.y = path[1].position.y;
                path[2].position.z = path[1].position.z;
                path[2].angle = angle;
                path[2].flags = 0;
                path[2].script_function = (int)get_script_function_by_name(
                    konquest_pdata->script_owner, "open_door_and_exit");
                path[3].position.x = path[0].position.x;
                path[3].position.y = path[0].position.y;
                path[3].position.z = path[0].position.z;
                path[3].angle = angle;
                path[3].flags = 0;
                path[3].script_function = (int)get_script_function_by_name(
                    konquest_pdata->script_owner, "npc_go_out_of_house");

                door->path_waypoints = path;
                if (partner != 0) {
                    partner->path_waypoints = path;
                }

                v3_add_v3_scaled(
                    &center, &center, &forward,
                    -0.25f +
                        konquest_pdata->region_table
                            ->enumerations[enumeration_index]
                            .trigger_offset);
                generate_door_trigger(
                    door, &center,
                    konquest_pdata->region_table
                        ->enumerations[enumeration_index]
                        .trigger_radius);
                generated_count++;
                waypoint_index += 4;
                waypoint_offset += 4 * sizeof(*path);
            }
            link = link->next;
        }
    }

    konquest_pdata->generated_door_count = generated_count;
    konquest_pdata->door_waypoints = waypoints;
}

/*
 * Soft ceiling: generate_door_trigger 88.80% (836 versus 832 bytes). The
 * composite door id, stale-list cleanup, script gate, trigger allocation,
 * inline tile lookup, flags, ownership update, and all eight calls agree.
 * Residue is one folded list-latch join and GPR/FPR instruction scheduling.
 */
static void generate_door_trigger(
    KonquestDoorObject* door, const Vec* position, float radius) {
    KonquestTriggerStruct* trigger;
    MkPtr* link;
    unsigned int door_uid;
    unsigned int enumeration;
    unsigned int trigger_id;
    char script_name[100];

    door_uid = door->owner->uid;
    enumeration = konquest_pdata->region_table
                      ->enumerations[door->definition->enumeration_index]
                      .enumeration;
    trigger_id = door_uid | (enumeration << 28);
    trigger = 0;
    if (konquest_pdata->triggers != 0) {
        link = konquest_pdata->triggers;
        while (link != 0) {
            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (trigger->owned_data->field_20 == (int)trigger_id) {
                break;
            }
            link = link->next;
        }
        if (link == 0) {
            trigger = 0;
        }
    }

    if (trigger == 0) {
        sprintf(script_name, "trigger_%i_door_%i", door_uid, enumeration);
        if (check_script_function_exists(
                konquest_pdata->script_owner, script_name) != 0) {
            unsigned int shifted_enumeration;
            unsigned int script_index;
            int tile_index;
            float x;
            float y;
            float z;

            shifted_enumeration = enumeration << 28;
            z = position->z;
            script_index = (unsigned int)get_script_function_by_name(
                konquest_pdata->script_owner, script_name);
            y = position->y;
            x = position->x;
            trigger = (KonquestTriggerStruct*)get_mkhdr(
                &vtbl_trigger_struct, sizeof(*trigger));
            if (trigger != 0) {
                zero_pdata_payload(sizeof(*trigger), trigger);
            }
            /* Retail dereferences both allocations without an early guard. */
            trigger->owned_data = (KonquestTriggerDefinition*)get_mem(
                sizeof(*trigger->owned_data));
            trigger->id = shifted_enumeration | door_uid;
            trigger->owned_data->type = 3;
            trigger->owned_data->position.x = x;
            trigger->owned_data->position.y = y;
            trigger->owned_data->position.z = z;
            if (trigger->owned_data->position.x >= 1000.0f) {
                tile_index =
                    konquest_pdata->tile_width * konquest_pdata->tile_height;
            } else {
                int row;
                int column;

                row = (int)((trigger->owned_data->position.z +
                             konquest_pdata->tile_origin_z) /
                            60.0f);
                if (row < 0) {
                    tile_index = -1;
                } else if (row >= konquest_pdata->tile_height) {
                    tile_index = -1;
                } else {
                    column = (int)((trigger->owned_data->position.x +
                                    konquest_pdata->tile_origin_x) /
                                   60.0f);
                    if (column < 0) {
                        tile_index = -1;
                    } else if (column >= konquest_pdata->tile_width) {
                        tile_index = -1;
                    } else {
                        tile_index =
                            column + row * konquest_pdata->tile_width;
                    }
                }
            }
            trigger->tile_index = tile_index;
            trigger->owned_data->radius = radius;
            trigger->owned_data->state = 1;
            trigger->owned_data->flags = 3;
            trigger->owned_data->script_index = script_index;
            trigger->owned_data->field_24 = 0;
            trigger->door_owner = 0;
            trigger->script_proc = 0;
            trigger->script_proc_instance = 0;
            trigger->flag_bits.bit7 = 0;
            trigger->flag_bits.pressed = 0;
            trigger->flag_bits.bit5 = 0;
            trigger->flag_bits.bit1 = 0;
            trigger->flag_bits.bit4 = 1;
            trigger->flag_bits.bit3 = 0;
            trigger->flag_bits.bit2 = 0;
            mk_insert(&trigger->hdr, &konquest_pdata->triggers);
            if (trigger != 0) {
                trigger->door_owner = door->owner;
            }
        }
    } else {
        trigger->owned_data->type = 3;
        trigger->door_owner = door->owner;
    }
}

/*
 * Soft ceiling: find_door_partner_sobj ~87.9% -- the lookup, stale-link
 * removal, comparisons, and returns are instruction-for-instruction aligned.
 * The residue is temporary GPR coloring plus individual GPR saves and
 * restores in place of retail's stmw/lmw pair.
 */
KonquestChildObject* find_door_partner_sobj(KonquestChildObject* door) {
    MkPtr* link;
    int uid;
    int partner_uid;
    MkPtr* next;

    if (door == 0) {
        return 0;
    }
    uid = door->owner->uid;
    partner_uid = konquest_pdata->region_table
                      ->enumerations[door->definition->enumeration_index]
                      .partner_uid;
    if (konquest_pdata->door_objects != 0) {
        link = konquest_pdata->door_objects;
        while (link != 0) {
            KonquestChildObject* candidate;

            candidate = (KonquestChildObject*)link->hdr;
            if (link->instance != candidate->hdr.instance) {
                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                int candidate_uid;
                int candidate_enumeration_index;

                candidate_uid = candidate->owner->uid;
                candidate_enumeration_index =
                    candidate->definition->enumeration_index;
                if (candidate_uid == uid &&
                    partner_uid ==
                        konquest_pdata->region_table
                            ->enumerations[candidate_enumeration_index]
                            .partner_index) {
                    return candidate;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

/*
 * Soft ceiling: load_tile_objects ~99.3% -- row traversal, object setup,
 * classification, and every call/branch match. Residue is a harmless r27/r29
 * rotation between the loop index and current SOBJ plus the pooled-string
 * relocation label.
 */
void load_tile_objects(KonquestTileObjectDefinition* definitions) {
    unsigned int row_count;
    MkObj* model;
    unsigned int index;
    KonquestTileObjectDefinition* definition;

    row_count = get_row_count_for_table_by_pointer(
        konquest_pdata->script_owner, definitions);
    model = load_named_model_from_slot(0x60029, "OBJECTS", 0x1004, 0);
    if (model == 0) {
        return;
    }

    konquest_pdata->tile_objects = &model->hdr;
    konquest_pdata->tile_objects_instance = model->hdr.instance;
    model->light_flags = 1;
    insert_fgnd_mkobj(model);
    obj_create_sobjs(model);
    RpClumpForAllAtomics(model->clump, hide_an_atomic, 0);

    definition = definitions;
    index = 0;
    while (index < row_count) {
        if (definition->uid >= 1000) {
            setup_locator_for_tile_object(definition);
        } else {
            MkSobj* object;

            object = obj_find_child_sobj_by_id(
                model, definition->uid, 2);
            if (object != 0) {
                set_true_clip_flag_on_sobj_and_children(object, 1);
                if (sobj_does_atomic_have_children(object) == 0 &&
                    definition->pebble_count == 1) {
                    if (setup_sobj_for_tile_object(object, definition) != 0) {
                        setup_children_sobjs_of_tile_object(object, definition);
                    }
                } else if (setup_pebble_system_for_tile_object(
                               object, definition) != 0) {
                    setup_children_pebbles_of_tile_object(object, definition);
                }
            }
        }
        index++;
        definition++;
    }
}

static void setup_children_sobjs_of_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition) {
    RwFrame* frame;

    frame = object->frame->child;
    while (frame != 0) {
        RwLLLink* link;
        RwLLLink* sentinel;
        RwFrame* next_frame;

        next_frame = frame->next;
        sentinel = &frame->objectList.link;
        link = frame->objectList.link.next;
        while (link != sentinel) {
            RpAtomic* atomic;
            RwLLLink* next;

            next = link->next;
            atomic = (RpAtomic*)((char*)link -
                                 KONQUEST_OFFSETOF(RpAtomic, frameLink));
            if (atomic->object.type == 1) {
                MkSobj* child;

                child = ((MksobjPluginData*)((char*)atomic +
                                             MksobjLocalOffset))
                            ->sobj;
                if (child != 0) {
                    setup_sobj_for_tile_object(child, definition);
                }
            }
            link = next;
        }
        frame = next_frame;
    }
}

static inline int find_enumeration_index(int uid) {
    unsigned int row_count;
    unsigned int index;

    if (konquest_pdata->region_table->enumerations == 0) {
        return -1;
    }
    row_count = get_row_count_for_table_by_pointer(
        konquest_pdata->script_owner,
        konquest_pdata->region_table->enumerations);
    for (index = 0; index < row_count; index++) {
        if (konquest_pdata->region_table->enumerations[index].locator_uid ==
            uid) {
            return (int)index;
        }
    }
    return -1;
}

static int setup_sobj_for_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition) {
    KonquestSobjInfo* info;
    unsigned int uid;
    char* name;
    int use_object_position;

    uid = object->id_flags & 0xFFF;
    use_object_position = (unsigned int)definition->uid != uid;
    if (use_object_position != 0) {
        name = "cr1_time_progression.sec";
    } else {
        name = definition->name;
    }
    info = (KonquestSobjInfo*)get_mkhdr(
        &vtbl_konquest_sobj_info, sizeof(*info));
    if (info != 0) {
        zero_pdata_payload(sizeof(*info), info);
        info->type = 1;
        info->uid = uid;
        info->enumeration_index = find_enumeration_index(uid);
        if (object != 0) {
            info->object = object;
            info->object_instance = object->hdr.instance;
        }
        if (strcmp(name, "cr1_time_progression.sec") != 0) {
            info->art_id = get_artid_of_named_item_in_slot(
                0x60029, name, 1);
        } else {
            info->art_id = 0;
        }
        mk_insert(&info->hdr, &konquest_pdata->sobj_infos);
    }
    if (info != 0) {
        object->flags_08_bits.bit6 = 1;
        object->flags_08_bits.bit3 = 1;
        if (use_object_position != 0) {
            info->position.x = object->pos.x;
            info->position.y = object->pos.y;
            info->position.z = object->pos.z;
            object->pos.z = 0.0f;
            object->pos.y = 0.0f;
            object->pos.x = 0.0f;
            object->flags_08_bits.bit7 = 1;
            update_mksobj(object);
        } else {
            info->position.z = 0.0f;
            info->position.y = 0.0f;
            info->position.x = 0.0f;
        }
        return 1;
    }
    return 0;
}

/*
 * Soft ceiling: setup_locator_for_tile_object ~90.4% -- the complete CFG,
 * calls, stores, and enumeration stride match. Residue is scan-local GPR
 * coloring, pooled-string offset labeling, and stmw/lmw save/restore form.
 */
static int setup_locator_for_tile_object(
    KonquestTileObjectDefinition* definition) {
    const char* name;
    int uid;
    KonquestSobjInfo* info;

    name = definition->name;
    uid = definition->uid;
    info = (KonquestSobjInfo*)get_mkhdr(
        &vtbl_konquest_sobj_info, sizeof(*info));
    if (info != 0) {
        zero_pdata_payload(sizeof(*info), info);
        info->type = 1;
        info->uid = uid;
        info->enumeration_index = find_enumeration_index(uid);
        if (strcmp(name, "cr1_time_progression.sec") != 0) {
            info->art_id = get_artid_of_named_item_in_slot(
                0x60029, name, 1);
        } else {
            info->art_id = 0;
        }
        mk_insert(&info->hdr, &konquest_pdata->sobj_infos);
    }
    if (info != 0) {
        return 1;
    }
    return 0;
}

static void setup_children_pebbles_of_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition) {
    RwFrame* frame;

    frame = object->frame->child;
    while (frame != 0) {
        RwLLLink* link;
        RwLLLink* sentinel;
        RwFrame* next_frame;

        next_frame = frame->next;
        sentinel = &frame->objectList.link;
        link = frame->objectList.link.next;
        while (link != sentinel) {
            RpAtomic* atomic;
            RwLLLink* next;

            next = link->next;
            atomic = (RpAtomic*)((char*)link -
                                 KONQUEST_OFFSETOF(RpAtomic, frameLink));
            if (atomic->object.type == 1) {
                MkSobj* child;

                child = ((MksobjPluginData*)((char*)atomic +
                                             MksobjLocalOffset))
                            ->sobj;
                if (child != 0) {
                    setup_pebble_system_for_tile_object(child, definition);
                }
            }
            link = next;
        }
        frame = next_frame;
    }
}

/*
 * Soft ceiling: setup_pebble_system_for_tile_object ~90.2% -- pebble setup,
 * transform transfer, metadata insertion, user-data initialization, and calls
 * match. Residue is equivalent nonzero booleanization, enumeration-scan GPR
 * coloring, early-return layout, and local string/zero-float relocations.
 */
static int setup_pebble_system_for_tile_object(
    MkSobj* object, KonquestTileObjectDefinition* definition) {
    PebbleData* pebbles;
    KonquestSobjInfo* info;
    int uid;
    char* name;
    int uid_difference;
    int use_object_position;
    float pos_z;
    float pos_y;
    float pos_x;
    int index;
    int* user_data;

    object->flags09_bits.bit4 = 1;
    uid = object->id_flags & 0xFFF;
    uid_difference = definition->uid - uid;
    use_object_position = uid_difference != 0;
    pebbles = create_pebble_userdata(object, definition->pebble_count, 4);
    if (pebbles == 0) {
        return 0;
    }

    if (use_object_position == 0) {
        pos_z = 0.0f;
        name = definition->name;
        pos_y = pos_z;
        pos_x = pos_z;
    } else {
        pos_x = object->pos.x;
        pos_y = object->pos.y;
        pos_z = object->pos.z;
        object->pos.z = 0.0f;
        object->pos.y = 0.0f;
        object->pos.x = 0.0f;
        object->flags_08_bits.bit7 = 1;
        update_mksobj(object);
        name = "cr1_time_progression.sec";
    }

    uid = object->id_flags & 0xFFF;
    info = (KonquestSobjInfo*)get_mkhdr(
        &vtbl_konquest_sobj_info, sizeof(*info));
    if (info != 0) {
        zero_pdata_payload(sizeof(*info), info);
        info->type = 1;
        info->uid = uid;
        info->enumeration_index = find_enumeration_index(uid);
        if (object != 0) {
            info->object = object;
            info->object_instance = object->hdr.instance;
        }
        if (strcmp(name, "cr1_time_progression.sec") != 0) {
            info->art_id = get_artid_of_named_item_in_slot(
                0x60029, name, 1);
        } else {
            info->art_id = 0;
        }
        mk_insert(&info->hdr, &konquest_pdata->sobj_infos);
    }

    info->position.x = pos_x;
    info->position.y = pos_y;
    info->position.z = pos_z;
    info->pebbles = pebbles;
    index = 0;
    user_data = (int*)pebbles->user_data;
    while (index < definition->pebble_count) {
        *user_data = 0;
        index++;
        user_data++;
    }
    pebbles->count = 0;
    unhide_sobj(object);
    return 1;
}

/*
 * Near match: model loading, script setup, both independently validated
 * tile-object contexts, and both effect-bank phases agree. Retail retains two
 * valid-latch join branches and an unreachable branch pair after model setup
 * that this compiler folds; the remaining 16-byte delta is control-flow
 * emission only.
 */
void load_konquest_tiles(void) {
    MkObj* model;
    unsigned int setup_function;

    if (konquest_pdata->region_table->region_asset == 0) {
        return;
    }

    load_art_section_by_name(
        0x60029,
        (const char*)konquest_pdata->region_table->region_asset);
    model = load_named_model_from_slot(0x60029, "TILES", 0x1004, 0);
    if (model == 0) {
        return;
    }

    model->light_flags = 1;
    insert_fgnd_mkobj(model);
    obj_create_sobjs(model);
    obj_apply_to_sobj_with_id(
        model, 0xA, (void (*)(void*))initialize_tile_patch_sobj);
    konquest_pdata->tile_model.object = &model->hdr;
    konquest_pdata->tile_model.instance = model->hdr.instance;
    konquest_pdata->tile_load_state = 0;

    if (konquest_editor_mode_on != 0) {
        setup_function = (unsigned int)get_script_function_by_name(
            konquest_pdata->script_owner, "konquest_001");
    } else {
        setup_function = konquest_pdata->region_table->setup_function;
    }
    konquest_pdata->flag_bits.triggers_active = 1;
    cmdscript_setup_execution(
        konquest_pdata->script_owner, setup_function);
    cmdscript_execute(konquest_pdata->script_owner);
    konquest_pdata->flag_bits.triggers_active = 0;
    update_visible_tiles();
    generate_door_paths();

    if (konquest_pdata->region_table->effect_bank_name != 0) {
        MkHdr* tile_objects;
        KonquestSectionContext context;

        tile_objects = konquest_pdata->tile_objects;
        if (tile_objects != 0) {
            if (tile_objects->instance !=
                konquest_pdata->tile_objects_instance) {
                tile_objects = 0;
            }
        } else {
            tile_objects = 0;
        }
        context.slot = 0x60029;
        context.owner = tile_objects;
        context.flags = 0;
        active_cmdscript->mko = konquest_pdata->script_owner;
        ((KonquestScriptSlotView*)konquest_pdata->script_owner)
            ->section_context = &context.slot;
        load_effect_bank(
            konquest_pdata->region_table->effect_bank_name);
        ((KonquestScriptSlotView*)konquest_pdata->script_owner)
            ->section_context = 0;
    }

    {
        MkHdr* tile_objects;
        KonquestSectionContext context;

        tile_objects = konquest_pdata->tile_objects;
        if (tile_objects != 0) {
            if (tile_objects->instance !=
                konquest_pdata->tile_objects_instance) {
                tile_objects = 0;
            }
        } else {
            tile_objects = 0;
        }
        context.slot = 0x60030;
        context.owner = tile_objects;
        context.flags = 0;
        save_current_ssf();
        load_ssf((MkFileEntry*)konquest_common_file_table);
        active_cmdscript->mko = konquest_pdata->script_owner;
        ((KonquestScriptSlotView*)konquest_pdata->script_owner)
            ->section_context = &context.slot;
        load_effect_bank("common_pfx.mko");
        ((KonquestScriptSlotView*)konquest_pdata->script_owner)
            ->section_context = 0;
        restore_previous_ssf();
    }
}

static void initialize_tile_patch_sobj(MkSobj* object) {
    object->flags09_bits.bit7 = 1;
    sobj_set_priority(object, 9);
}

/*
 * Near match at the exact retail size: allocation arithmetic, signed-int float
 * conversions, origins, field stores, allocation, and zeroing all agree.
 * Residue is instruction scheduling across the two conversions and GPR/FPR
 * allocation; no control-flow or layout difference remains.
 */
void set_tile_grid_size(int width, int height) {
    int allocation_size;
    float origin_x;

    allocation_size = ((width * height) + 1) * sizeof(KonquestTileRecord);
    konquest_pdata->tile_width = width;
    origin_x = 0.5f * (60.0f * (float)width);
    konquest_pdata->tile_height = height;
    konquest_pdata->tile_origin_x = origin_x;
    konquest_pdata->tile_origin_z = 0.5f * (60.0f * (float)height);
    konquest_pdata->tile_structs = get_mem(allocation_size);
    memset(konquest_pdata->tile_structs, 0, allocation_size);
}

static RpAtomic* hide_an_atomic(RpAtomic* atomic, void* data) {
    hide_atomic(atomic);
    return atomic;
}

/*
 * Soft ceiling: hide_tile_objects ~92.2%. Stale-list cleanup, attached-effect
 * reset, matrix-palette compaction, object hiding, and final state stores
 * match. Residue is nonvolatile-GPR coloring, palette-address scheduling, and
 * one folded object latch; source is 472 bytes versus retail's 488.
 */
static void hide_tile_objects(KonquestTileRecord* tile) {
    MkPtr* object_link;

    if (tile != 0 && tile->objects != 0) {
        object_link = tile->objects;
        while (object_link != 0) {
            KonquestUidObject* object;

            object = (KonquestUidObject*)object_link->hdr;
            if (object_link->instance != object->hdr.instance) {
                MkPtr* next;

                next = object_link->next;
                object_link->hdr = 0;
                destroy_mkptr(object_link);
                object_link = next;
                continue;
            }

            if ((signed char)object->attached_pfx_name[0] != 0 &&
                object->attached_pfx_state != 0) {
                fx_reset_emit(object->attached_pfx_state);
            }
            if (object->render_records != 0) {
                MkPtr* record_link;

                record_link = object->render_records;
                while (record_link != 0) {
                    KonquestRenderRecord* record;

                    record = (KonquestRenderRecord*)record_link->hdr;
                    if (record_link->instance != record->hdr.instance) {
                        MkPtr* next;

                        next = record_link->next;
                        record_link->hdr = 0;
                        destroy_mkptr(record_link);
                        record_link = next;
                        continue;
                    }

                    if (record->matrix_index >= 0) {
                        KonquestSobjBinding* binding;
                        KonquestMatrixPalette* palette;

                        binding = record->binding;
                        palette = binding->palette;
                        if (palette != 0) {
                            KonquestRenderRecord** records;
                            int index;
                            int last_index;

                            index = record->matrix_index;
                            if (index >= 0 && palette != 0) {
                                records = palette->records;
                                last_index = palette->count - 1;
                                if (index != last_index) {
                                    KonquestRenderRecord* last;

                                    last = records[last_index];
                                    if (record != 0 && last != 0) {
                                        last->matrix_index = index;
                                        set_mat(
                                            &palette->matrices[index],
                                            &palette->matrices[last_index]);
                                        records[index] = last;
                                    }
                                }
                                records[palette->count - 1] = 0;
                                palette->count--;
                                if (palette->count < 0) {
                                    palette->count = 0;
                                }
                            }
                        } else {
                            MkSobj* render_object;

                            render_object = binding->object;
                            if (render_object != 0) {
                                if (render_object->hdr.instance !=
                                    binding->object_instance) {
                                    render_object = 0;
                                }
                            } else {
                                render_object = 0;
                            }
                            if (render_object != 0) {
                                hide_sobj_and_children(render_object);
                            }
                        }
                    }
                    record->binding->hidden = 1;
                    record->matrix_index = -1;
                    record_link = record_link->next;
                }
            }
            object_link = object_link->next;
        }
    }
}

void start_time_passing(void) {
    konquest_pdata->time_passing = 1;
}

void stop_time_passing(void) {
    konquest_pdata->time_passing = 0;
}

void get_current_time(void* time) {
    memcpy(time, &konquest_pdata->current_time,
           sizeof(konquest_pdata->current_time));
}

#pragma dont_inline on
/*
 * Soft ceiling: set_current_time ~95.6% -- fields, validation CFG, recursive
 * call, float math, stores, and exact size match. Residue is one scheduled
 * load/compare pair and local floating-constant relocation labels.
 */
void set_current_time(const KonquestTime* time) {
    KonquestTime occurrence;
    float hour;

    if (time->year == -1 || time->month == -1 ||
        time->day_of_month == -1 || time->day_of_week == -1) {
        calc_next_occurrence_of_event(
            &occurrence, time, &konquest_pdata->current_time);
        set_current_time(&occurrence);
    } else {
        if (time->year != -1) {
            konquest_pdata->current_time.year = time->year;
        }
        if (time->month != -1 && time->month < 12) {
            konquest_pdata->current_time.month = time->month;
        }
        if (time->day_of_month != -1 && time->day_of_month < 30) {
            konquest_pdata->current_time.day_of_month = time->day_of_month;
        }
        if (time->day_of_week != -1 && time->day_of_week < 7) {
            konquest_pdata->current_time.day_of_week = time->day_of_week;
        }
        if (time->hour != -1 && time->hour < 24) {
            konquest_pdata->current_time.hour = time->hour;
        }
        if (time->minute != -1 && time->minute < 60) {
            konquest_pdata->current_time.minute = time->minute;
        }
    }
    hour = (float)time->hour;
    konquest_pdata->time_of_day = hour + (float)time->minute / 60.0f;
    konquest_pdata->time_rate_changed = 1;
    update_time_screen_objs(1);
    npc_reset_all_timed_events();
}
#pragma dont_inline reset

/*
 * Soft ceiling: update_time_screen_objs 89.67% (808 versus 788 bytes). The
 * five HUD latches, day-art replacement, exact 12-call sequence, time/date
 * formatting, feature flag, and access widths agree. Residue is five folded
 * valid-latch joins, individual saves/restores, and GPR scheduling.
 */
static void update_time_screen_objs(int update_all) {
    StringObj* hour_object;
    StringObj* minute_object;
    ScreenObj* day_object;
    StringObj* date_object;
    StringObj* period_object;
    char period[4];
    char period_text[4];
    char minute[4];
    char hour[4];
    char date[16];

    hour_object = (StringObj*)konquest_pdata->hud_labels[0].object;
    if (hour_object != 0) {
        if (hour_object->instance !=
            konquest_pdata->hud_labels[0].instance) {
            hour_object = 0;
        }
    } else {
        hour_object = 0;
    }
    minute_object = (StringObj*)konquest_pdata->hud_labels[1].object;
    if (minute_object != 0) {
        if (minute_object->instance !=
            konquest_pdata->hud_labels[1].instance) {
            minute_object = 0;
        }
    } else {
        minute_object = 0;
    }
    day_object = (ScreenObj*)konquest_pdata->hud_labels[3].object;
    if (day_object != 0) {
        if (day_object->instance !=
            konquest_pdata->hud_labels[3].instance) {
            day_object = 0;
        }
    } else {
        day_object = 0;
    }
    date_object = (StringObj*)konquest_pdata->hud_extra_label.object;
    if (date_object != 0) {
        if (date_object->instance !=
            konquest_pdata->hud_extra_label.instance) {
            date_object = 0;
        }
    } else {
        date_object = 0;
    }
    period_object = (StringObj*)konquest_pdata->hud_labels[2].object;
    if (period_object != 0) {
        if (period_object->instance !=
            konquest_pdata->hud_labels[2].instance) {
            period_object = 0;
        }
    } else {
        period_object = 0;
    }

    if (update_all != 0 && day_object != 0) {
        if (day_object->instance != 0) {
            day_object->typed_vtbl->destroy(day_object);
        }
        day_object = load_2d_pfxobj(
            0x60030, 0x8301,
            days_of_week[konquest_pdata->current_time.day_of_week].art.name,
            0, 0x44);
        if (day_object != 0) {
            day_object->x =
                (screen_width - 0x32) - day_object->pfx2d->tex_w;
            day_object->y = screen_height - 0x4A;
            konquest_pdata->hud_labels[3].object = (MkHdr*)day_object;
            konquest_pdata->hud_labels[3].instance = day_object->instance;
        }
        if (g_game_info.flag_bits.pad_bit1 == 0) {
            hide_screen_obj(day_object);
        }
    }

    if (hour_object != 0) {
        int display_hour;

        display_hour = (int)konquest_pdata->time_of_day % 12;
        if (display_hour == 0) {
            display_hour = 12;
        }
        sprintf(hour, "%d:", display_hour);
        update_string_obj(hour_object, 0xE, hour);
    }
    if (minute_object != 0) {
        sprintf(minute, "%02d", konquest_pdata->current_time.minute);
        update_string_obj(minute_object, 0xE, minute);
    }
    if (period_object != 0) {
        if (konquest_pdata->current_time.hour < 12) {
            sprintf(period, "AM");
        } else {
            sprintf(period, "PM");
        }
        sprintf(period_text, "%s", period);
        update_string_obj(period_object, 0xE, period_text);
    }
    if (date_object != 0) {
        sprintf(
            date, "%02d.%02d.%02d",
            konquest_pdata->current_time.month + 1,
            konquest_pdata->current_time.day_of_month + 1,
            konquest_pdata->current_time.year + 1);
        update_string_obj(date_object, 0xE, date);
    }
}

/*
 * Near match: objective-row advancement, all target modes, stale-safe trigger
 * and NPC resolution, duplicated beam publication paths, distance scaling,
 * and arrow geometry agree. The 12-byte size excess is limited to the two
 * deliberate vector-component initializations required for defined portable
 * C and equivalent branch emission; other residue is coloring and scheduling.
 */
static float p_adjust_objective_arrow_and_beam(void) {
    KonquestPdata* pdata;
    KonquestObjectiveRow* table;
    KonquestObjectiveRow* row;
    MkObj* hero;
    CameraObj* camera;
    ScreenObj* arrow;
    Vec forward = {0.0f, 0.0f, 1.0f};
    Vec target_position = {0.0f, 0.0f, 0.0f};
    Vec display_position;
    Vec tracked_position;
    Vec direction;
    Vec angles;
    Vec rotated;
    float delta_x;
    float delta_z;
    float distance_squared;
    float distance;
    float inverse_distance;
    float relative_angle;
    int index;

    pdata = konquest_pdata;
    hero = pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance != pdata->hero_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }
    arrow = (ScreenObj*)pdata->hud_objects[2].object;
    if (arrow != 0) {
        if (arrow->instance != pdata->hud_objects[2].instance) {
            arrow = 0;
        }
    } else {
        arrow = 0;
    }
    if (hero == 0) {
        return 1.0f;
    }
    if (camera == 0 || arrow == 0) {
        return -1.0f;
    }

    table = pdata->objective.table;
    if (table != 0) {
        row = &table[p1_profile_konquest->fields.objective_index];
        if (p1_profile_konquest->fields.objective_index <
                (int)get_row_count_for_table_by_pointer(
                    pdata->script_owner, table) &&
            (row->requirement_type == -1 ||
             get_konq_profile_value(
                 row->requirement_type, row->requirement_index) != 0)) {
            switch (row->target_type) {
            case 0: {
                ScreenObj* hidden_arrow;
                MkObj* hidden_beam;

                hidden_arrow =
                    (ScreenObj*)konquest_pdata->hud_objects[2].object;
                if (hidden_arrow != 0) {
                    if (hidden_arrow->instance !=
                        konquest_pdata->hud_objects[2].instance) {
                        hidden_arrow = 0;
                    }
                } else {
                    hidden_arrow = 0;
                }
                hidden_beam =
                    (MkObj*)konquest_pdata->objective_beam.object;
                if (hidden_beam != 0) {
                    if (hidden_beam->hdr.instance !=
                        konquest_pdata->objective_beam.instance) {
                        hidden_beam = 0;
                    }
                } else {
                    hidden_beam = 0;
                }
                if (hidden_arrow != 0 && hidden_beam != 0) {
                    MkSobj* sky_object;

                    hide_screen_obj(hidden_arrow);
                    hide_obj(hidden_beam);
                    if (g_game_info.sky != 0) {
                        sky_object = obj_find_sobj_by_id(g_game_info.sky, 1);
                        if (sky_object != 0) {
                            hide_sobj(sky_object);
                        }
                    }
                    konquest_pdata->objective.visible = 0;
                }
                break;
            }
            case 1:
                konquest_pdata->objective.target_type = 1;
                konquest_pdata->objective.trigger_id =
                    get_table_index_by_pointer(
                        konquest_pdata->script_owner, row->trigger);
                konquest_pdata->objective.target_offset.x =
                    row->target_offset.x;
                konquest_pdata->objective.target_offset.y =
                    row->target_offset.y;
                konquest_pdata->objective.target_offset.z =
                    row->target_offset.z;
                konquest_pdata->objective.visible = 1;
                konquest_pdata->objective_field_2F0 = 1;
                break;
            case 2:
                konquest_pdata->objective.npc_data = row->npc_data;
                konquest_pdata->objective.target_type = 2;
                konquest_pdata->objective.visible = 1;
                konquest_pdata->objective_field_2F0 = 1;
                break;
            case 3:
                konquest_pdata->objective.npc_data = row->npc_data;
                konquest_pdata->objective.target_type = 3;
                konquest_pdata->objective.visible = 1;
                konquest_pdata->objective_field_2F0 = 1;
                break;
            }
            p1_profile_konquest->fields.objective_index++;
        }
    }

    if (pdata->objective.visible == 0) {
        return 1.0f;
    }

    if (konquest_pdata->objective_field_2F0 != 0) {
        switch (pdata->objective.target_type) {
        case 1: {
            KonquestTriggerStruct* trigger;

            trigger = find_trigger_by_id(pdata->objective.trigger_id);
            if (trigger == 0) {
                return -1.0f;
            }
            target_position.x = pdata->objective.target_offset.x +
                                trigger->owned_data->position.x;
            target_position.y = pdata->objective.target_offset.y +
                                trigger->owned_data->position.y;
            target_position.z = pdata->objective.target_offset.z +
                                trigger->owned_data->position.z;
            break;
        }
        case 2:
        case 3: {
            KonquestNpc* npc;

            npc = find_npc_by_data(pdata->objective.npc_data);
            if (npc == 0) {
                return -1.0f;
            }
            target_position.x = npc->fields.data->position.x;
            target_position.y = npc->fields.data->position.y;
            target_position.z = npc->fields.data->position.z;
            break;
        }
        }

        display_position = target_position;
        {
            ScreenObj* current_arrow;
            MkObj* beam;

            current_arrow =
                (ScreenObj*)konquest_pdata->hud_objects[2].object;
            if (current_arrow != 0) {
                if (current_arrow->instance !=
                    konquest_pdata->hud_objects[2].instance) {
                    current_arrow = 0;
                }
            } else {
                current_arrow = 0;
            }
            beam = (MkObj*)konquest_pdata->objective_beam.object;
            if (beam != 0) {
                if (beam->hdr.instance !=
                    konquest_pdata->objective_beam.instance) {
                    beam = 0;
                }
            } else {
                beam = 0;
            }
            if (current_arrow != 0 && beam != 0 &&
                get_game_state() != 0x14) {
                if (konquest_pdata->hud_visible != 0) {
                    unhide_screen_obj(current_arrow);
                }
                unhide_obj(beam);
                display_position.y = 0.0f;
                konquest_pdata->objective.position.x = display_position.x;
                konquest_pdata->objective.position.y = display_position.y;
                konquest_pdata->objective.position.z = display_position.z;
                beam->pos.value.x = display_position.x;
                beam->pos.value.y = display_position.y;
                beam->pos.value.z = display_position.z;
                update_mkobj(beam != 0 ? as_mkhdr(&beam->hdr) : 0);
                konquest_pdata->objective.visible = 1;
                konquest_pdata->objective_field_2F0 = 1;
            }
        }
        konquest_pdata->objective_field_2F0 = 0;
    } else if (pdata->objective.target_type == 3) {
        KonquestNpc* npc;
        ScreenObj* current_arrow;
        MkObj* beam;

        npc = find_npc_by_data(pdata->objective.npc_data);
        if (npc == 0) {
            return -1.0f;
        }
        tracked_position = npc->fields.data->position;
        current_arrow =
            (ScreenObj*)konquest_pdata->hud_objects[2].object;
        if (current_arrow != 0) {
            if (current_arrow->instance !=
                konquest_pdata->hud_objects[2].instance) {
                current_arrow = 0;
            }
        } else {
            current_arrow = 0;
        }
        beam = (MkObj*)konquest_pdata->objective_beam.object;
        if (beam != 0) {
            if (beam->hdr.instance !=
                konquest_pdata->objective_beam.instance) {
                beam = 0;
            }
        } else {
            beam = 0;
        }
        if (current_arrow != 0 && beam != 0 &&
            get_game_state() != 0x14) {
            if (konquest_pdata->hud_visible != 0) {
                unhide_screen_obj(current_arrow);
            }
            unhide_obj(beam);
            tracked_position.y = 0.0f;
            konquest_pdata->objective.position.x = tracked_position.x;
            konquest_pdata->objective.position.y = tracked_position.y;
            konquest_pdata->objective.position.z = tracked_position.z;
            beam->pos.value.x = tracked_position.x;
            beam->pos.value.y = tracked_position.y;
            beam->pos.value.z = tracked_position.z;
            update_mkobj(beam != 0 ? as_mkhdr(&beam->hdr) : 0);
            konquest_pdata->objective.visible = 1;
            konquest_pdata->objective_field_2F0 = 1;
        }
    }

    delta_z = pdata->objective.position.z - hero->pos.value.z;
    delta_x = pdata->objective.position.x - hero->pos.value.x;
    distance_squared = delta_x * delta_x + delta_z * delta_z;
    distance = konquest_fast_sqrt(distance_squared);
    if (distance > 0.0f) {
        inverse_distance = 1.0f / distance;
    } else {
        inverse_distance = distance;
    }
    direction.x = delta_x * inverse_distance;
    direction.y = 0.0f;
    direction.z = delta_z * inverse_distance;
    set_objective_beam_scale(distance);
    v3_to_xy_ang(&angles, &direction);
    relative_angle = camera->ang.y - angles.y;
    rotated.y = 0.0f;
    rotate_xz(&rotated, &forward, relative_angle);
    rotated.x *= 50.0f;
    rotated.y *= 50.0f;
    rotated.z *= 50.0f;
    for (index = 0; index < 4; index++) {
        float vertex_angle;

        vertex_angle =
            -(1.5707964f * (float)index - 3.926991f) - relative_angle;
        arrow->pfx2d->verts[index].x =
            22.63f * gxMathCos(vertex_angle) + rotated.x;
        arrow->pfx2d->verts[index].y =
            22.63f * gxMathSin(vertex_angle) + rotated.z;
    }
    return 1.0f;
}

/*
 * Soft ceiling: set_objective_beam_scale ~91.8% -- beam/sky selection,
 * position transfer, scale calculation, and update calls match retail.
 * Residue is individual r29-r31 saves/restores instead of stmw/lmw, one
 * collapsed valid-state edge, and local constant relocation labels.
 */
static void set_objective_beam_scale(float distance) {
    KonquestObjectiveState* objective;
    MkObj* beam;
    MkSobj* scaled_object;
    float ratio;

    beam = (MkObj*)konquest_pdata->objective_beam.object;
    if (beam != 0) {
        if (beam->hdr.instance == konquest_pdata->objective_beam.instance) {
            /* Valid objective-beam latch. */
        } else {
            beam = 0;
        }
    } else {
        beam = 0;
    }
    {
        Vec base_scale = {1.0f, 1.0f, 1.0f};

        objective = &konquest_pdata->objective;
        scaled_object = 0;
        if (beam != 0) {
            if (objective != 0) {
                /* Valid objective-state view. */
            } else {
                return;
            }
            if (distance >=
                konquest_pdata->region_table->objective_beam_threshold -
                    10.0f) {
                MkSobj* beam_object;

                beam_object = obj_find_sobj_by_id(beam, 1);
                if (beam_object != 0) {
                    hide_sobj(beam_object);
                }
                if (g_game_info.sky != 0) {
                    MkSobj* sky_object;

                    sky_object = obj_find_sobj_by_id(g_game_info.sky, 1);
                    if (sky_object != 0) {
                        unhide_sobj(sky_object);
                        scaled_object = sky_object;
                        sky_object->pos.x = objective->position.x;
                        sky_object->pos.y = objective->position.y;
                        sky_object->pos.z = objective->position.z;
                    }
                }
            } else {
                if (g_game_info.sky != 0) {
                    MkSobj* sky_object;

                    sky_object = obj_find_sobj_by_id(g_game_info.sky, 1);
                    if (sky_object != 0) {
                        hide_sobj(sky_object);
                    }
                }
                scaled_object = obj_find_sobj_by_id(beam, 1);
                if (scaled_object != 0) {
                    unhide_sobj(scaled_object);
                }
            }
            if (scaled_object != 0) {
                ratio = distance / 500.0f;
                objective->beam_scale = 4.0f * ratio + 1.0f;
                scaled_object->scale.x =
                    base_scale.x * objective->beam_scale;
                scaled_object->scale.y =
                    base_scale.y * objective->beam_scale;
                scaled_object->scale.z =
                    base_scale.z * objective->beam_scale;
                update_mksobj(scaled_object);
            }
        }
    }
}

/*
 * Soft ceiling: p_adjust_compass ~95.5%. Both validated handles, all four
 * vertex updates, trig calls, constants, and access widths match. Equivalent
 * compact latch joins make this four bytes smaller than retail; the remaining
 * loop residue is GPR/FPR coloring and local constant relocation labels.
 */
static float p_adjust_compass(void) {
    CameraObj* camera;
    ScreenObj* compass;
    int index;

    camera = camera_item.node;
    if (camera != 0) {
        if (camera->hdr.instance != camera_item.instance) {
            camera = 0;
        }
    } else {
        camera = 0;
    }

    compass = (ScreenObj*)konquest_pdata->hud_objects[0].object;
    if (compass != 0) {
        if (compass->instance != konquest_pdata->hud_objects[0].instance) {
            compass = 0;
        }
    } else {
        compass = 0;
    }

    for (index = 0; index < 4; index++) {
        float angle;

        angle = -(1.5707964f * (float)index - 7.0685835f);
        compass->pfx2d->verts[index].x =
            90.5f * gxMathCos(angle - camera->ang.y) + 64.0f;
        compass->pfx2d->verts[index].y =
            90.5f * gxMathSin(angle - camera->ang.y) + 64.0f;
    }
    return 1.0f;
}

/*
 * Near match: all HUD latches, identifiers, labels, coordinates, process
 * creation, and optional region art agree with retail. The remaining 94.88%
 * difference is limited to signed division-by-two lowering, register
 * scheduling, and pooled constant/string relocation labels.
 */
static void init_heads_up_display(void) {
    MkHdr* compass_pdata;
    ScreenObj* object;
    StringObj* label;

    object = load_named_2d_pfxobj_xy(
        0x60030, 0x8301, "MINIFRAME", 0, screen_width / 2,
        screen_height / 2, 0x48);
    konquest_pdata->award_picture.obj = object;
    konquest_pdata->award_picture.obj_instance = object->instance;
    hide_screen_obj(object);

    object = load_2d_pfxobj_xy(
        0x60030, 0x8301, (char*)0x0A8C0014, 0,
        screen_width - 0xE2, screen_height - 0xA8, 0x48);
    konquest_pdata->hud_objects[3].object = (MkHdr*)object;
    konquest_pdata->hud_objects[3].instance = object->instance;
    object = load_2d_pfxobj_xy(
        0x60030, 0x8301, (char*)0x0A8C0015, 0,
        screen_width - 0xDA, screen_height - 0xA8, 0x48);
    konquest_pdata->hud_objects[4].object = (MkHdr*)object;
    konquest_pdata->hud_objects[4].instance = object->instance;
    if (object != 0) {
        object->scale_x = 20.6f;
    }
    object = load_2d_pfxobj_xy(
        0x60030, 0x8301, (char*)0x0A8C0016, 0,
        screen_width - 0x35, screen_height - 0xA8, 0x48);
    konquest_pdata->hud_objects[5].object = (MkHdr*)object;
    konquest_pdata->hud_objects[5].instance = object->instance;
    object = load_2d_pfxobj_xy(
        0x60030, 0x8301, (char*)0x0A8C0013, 0,
        screen_width - 0x78, 0x26, 0x49);
    konquest_pdata->hud_objects[6].object = (MkHdr*)object;
    konquest_pdata->hud_objects[6].instance = object->instance;

    object = load_2d_pfxobj(
        0x60030, 0x8301, days_of_week[0].art.name, 0, 0x44);
    if (object != 0) {
        object->x = (screen_width - 0x32) - object->pfx2d->tex_w;
        object->y = screen_height - 0x4A;
        konquest_pdata->hud_labels[3].object = (MkHdr*)object;
        konquest_pdata->hud_labels[3].instance = object->instance;
    }

    label = string_right_xy(
        0x8300, 0xE, "12:", screen_width - 0x78,
        screen_height - 0x75, 0x44);
    konquest_pdata->hud_labels[0].object = (MkHdr*)label;
    konquest_pdata->hud_labels[0].instance = label->instance;
    label = string_left_xy(
        0x8300, 0xE, "00", screen_width - 0x78,
        screen_height - 0x75, 0x44);
    konquest_pdata->hud_labels[1].object = (MkHdr*)label;
    konquest_pdata->hud_labels[1].instance = label->instance;
    label = string_right_xy(
        0x8300, 0xE, "AM", screen_width - 0x32,
        screen_height - 0x75, 0x44);
    konquest_pdata->hud_labels[2].object = (MkHdr*)label;
    konquest_pdata->hud_labels[2].instance = label->instance;
    label = string_right_xy(
        0x8300, 0xE, "01.01.01", screen_width - 0x32,
        screen_height - 0x5E, 0x44);
    konquest_pdata->hud_extra_label.object = (MkHdr*)label;
    konquest_pdata->hud_extra_label.instance = label->instance;

    object = load_2d_pfxobj_xy(
        0x60030, 0x8301, (char*)0x0A8C0009, 0, 0x14,
        screen_height - 0x9B, 0x45);
    konquest_pdata->hud_objects[1].object = (MkHdr*)object;
    konquest_pdata->hud_objects[1].instance = object->instance;
    object = load_2d_pfxobj_xy(
        0x60030, 0x8301, (char*)0x0A8C000A, 0, 0x14,
        screen_height - 0x9B, 0x47);
    konquest_pdata->hud_objects[0].object = (MkHdr*)object;
    konquest_pdata->hud_objects[0].instance = object->instance;
    _create_mkproc_generic_tinystack(
        0x9018, 0x1F, p_adjust_compass, sizeof(*compass_pdata),
        (void**)&compass_pdata);

    if (konquest_region_data[konquest_pdata->region_index].hud_art_name != 0) {
        object = load_named_2d_pfxobj(
            0x60029, 0x8301,
            konquest_region_data[konquest_pdata->region_index].hud_art_name,
            0, 0x45);
        if (object != 0) {
            object->x = (screen_width - 0x32) - object->pfx2d->tex_w;
            object->y = 0x1E;
            konquest_pdata->hud_labels[4].object = (MkHdr*)object;
            konquest_pdata->hud_labels[4].instance = object->instance;
        }
    }
    konquest_hide_hud(0);
}

/*
 * Near match: all three handles and sound IDs, absolute step/target test,
 * volume ramp, sleep timing, and final target stores agree with retail.
 * Residue is individual nonvolatile saves/restores, local load scheduling,
 * and floating-constant relocation labels.
 */
static float p_fade_ambient_sounds(void) {
    KonquestSoundFadePdata* pdata;
    int sound_a;
    int sound_b;
    int sound_main;
    float volume;

    pdata = (KonquestSoundFadePdata*)pdata_of_proc(aproc);
    sound_a = konquest_pdata->region_table->ambient_sound_a;
    sound_b = konquest_pdata->region_table->ambient_sound_b;
    sound_main = konquest_pdata->region_table->ambient_sound;
    if (pdata == 0) {
        return -1.0f;
    }

    volume = pdata->current_volume;
    while ((volume - pdata->target_volume >= 0.0f
                ? volume - pdata->target_volume
                : -(volume - pdata->target_volume)) >
           (pdata->step >= 0.0f ? pdata->step : -pdata->step)) {
        if (mslSoundIsValid(konquest_pdata->ambient_sound_a)) {
            set_snd_vol(konquest_pdata->ambient_sound_a, sound_a, volume);
        }
        if (mslSoundIsValid(konquest_pdata->ambient_sound_b)) {
            set_snd_vol(konquest_pdata->ambient_sound_b, sound_b, volume);
        }
        if (mslSoundIsValid(konquest_pdata->ambient_sound_main)) {
            set_snd_vol(
                konquest_pdata->ambient_sound_main, sound_main, volume);
        }
        _mkproc_sleep_ticks = 1.0f;
        volume += pdata->step;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (mslSoundIsValid(konquest_pdata->ambient_sound_a)) {
        set_snd_vol(
            konquest_pdata->ambient_sound_a, sound_a, pdata->target_volume);
    }
    if (mslSoundIsValid(konquest_pdata->ambient_sound_b)) {
        set_snd_vol(
            konquest_pdata->ambient_sound_b, sound_b, pdata->target_volume);
    }
    if (mslSoundIsValid(konquest_pdata->ambient_sound_main)) {
        set_snd_vol(
            konquest_pdata->ambient_sound_main, sound_main,
            pdata->target_volume);
    }
    return -1.0f;
}

/*
 * Soft ceiling: effect/emitter lookup, position arithmetic, null guards, and
 * restart/resume order match retail. The 12-byte residue is individual GPR
 * saves/restores versus stmw/lmw and the resulting register allocation.
 */
/*
 * Near match: p_cross_fade_ambient_sounds 99.47% at exact retail size. The
 * time-change reset, dawn/dusk fades, sound lifecycle, random ambience gate,
 * and all 22 calls agree. Remaining differences are pooled-float relocation
 * labels plus one equivalent final 1.0f argument load.
 */
static float p_cross_fade_ambient_sounds(void) {
    KonquestAmbientFadePdata* pdata;
    int hour;
    int mode;

    pdata = (KonquestAmbientFadePdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }
    if (konquest_pdata->time_passing == 0) {
        return 1.0f;
    }

    if (konquest_pdata->time_rate_changed != 0) {
        konquest_pdata->time_rate_changed--;
        if (konquest_pdata->time_of_day >= 7.0f &&
            konquest_pdata->time_of_day < 18.0f) {
            pdata->volume_a = 1.0f;
            pdata->volume_b = 0.0f;
            if (mslSoundIsValid(konquest_pdata->ambient_sound_a) != 0) {
                set_snd_vol(
                    konquest_pdata->ambient_sound_a,
                    konquest_pdata->region_table->ambient_sound_a,
                    pdata->volume_a);
            } else {
                konquest_pdata->ambient_sound_a = snd_req_vol(
                    konquest_pdata->region_table->ambient_sound_a,
                    pdata->volume_a);
            }
            if (mslSoundIsValid(konquest_pdata->ambient_sound_b) != 0) {
                snd_stop(konquest_pdata->ambient_sound_b);
                konquest_pdata->ambient_sound_b = 0;
            }
        } else {
            pdata->volume_a = 0.0f;
            pdata->volume_b = 1.0f;
            if (mslSoundIsValid(konquest_pdata->ambient_sound_b) != 0) {
                set_snd_vol(
                    konquest_pdata->ambient_sound_b,
                    konquest_pdata->region_table->ambient_sound_b,
                    pdata->volume_b);
            } else {
                konquest_pdata->ambient_sound_b = snd_req_vol(
                    konquest_pdata->region_table->ambient_sound_b,
                    pdata->volume_b);
            }
            if (mslSoundIsValid(konquest_pdata->ambient_sound_a) != 0) {
                snd_stop(konquest_pdata->ambient_sound_a);
                konquest_pdata->ambient_sound_a = 0;
            }
        }
        pdata->step_a = 0.0f;
        pdata->step_b = 0.0f;
        return 1.0f;
    }

    hour = (int)konquest_pdata->time_of_day;
    if ((float)hour == 5.0f &&
        konquest_pdata->ambient_sound_a == 0) {
        konquest_pdata->ambient_sound_a = snd_req_vol(
            konquest_pdata->region_table->ambient_sound_a,
            pdata->volume_a);
        pdata->step_a = 1.0f / (2.0f * ticks_per_hour);
        pdata->step_b = -1.0f / (2.0f * ticks_per_hour);
        return 1.0f;
    }
    if (hour == 17 &&
        konquest_pdata->ambient_sound_b == 0) {
        konquest_pdata->ambient_sound_b = snd_req_vol(
            konquest_pdata->region_table->ambient_sound_b,
            pdata->volume_b);
        pdata->step_a = -1.0f / (2.0f * ticks_per_hour);
        pdata->step_b = 1.0f / (2.0f * ticks_per_hour);
        return 1.0f;
    }

    if (konquest_pdata->ambient_sound_a != 0) {
        pdata->volume_a += pdata->step_a;
        if (pdata->volume_a <= 0.0f) {
            snd_stop(konquest_pdata->ambient_sound_a);
            konquest_pdata->ambient_sound_a = 0;
        } else if (pdata->volume_a >= 1.0f) {
            pdata->volume_a = 1.0f;
        } else {
            set_snd_vol(
                konquest_pdata->ambient_sound_a,
                konquest_pdata->region_table->ambient_sound_a,
                pdata->volume_a);
        }
    }
    if (konquest_pdata->ambient_sound_b != 0) {
        pdata->volume_b += pdata->step_b;
        if (pdata->volume_b <= 0.0f) {
            snd_stop(konquest_pdata->ambient_sound_b);
            konquest_pdata->ambient_sound_b = 0;
        } else if (pdata->volume_b >= 1.0f) {
            pdata->volume_b = 1.0f;
        } else {
            set_snd_vol(
                konquest_pdata->ambient_sound_b,
                konquest_pdata->region_table->ambient_sound_b,
                pdata->volume_b);
        }
    }

    mode = konquest_pdata->game_mode_index < 0
               ? 0
               : konquest_pdata
                     ->game_modes[konquest_pdata->game_mode_index];
    if (mode != 4) {
        if (pdata->delay <= 0.0f) {
            float pan;
            float volume;
            int sound;

            pan = sfrand(2.0f);
            volume = 0.5f + frand(0.4f);
            pdata->delay =
                (float)((unsigned short)randu0(0x384) + 0x12C);
            sound = konquest_pdata->ambient_sound_a != 0
                        ? konquest_pdata->region_table->ambient_effect_a
                        : konquest_pdata->region_table->ambient_effect_b;
            if (sound != -1) {
                pan_vol_pitch_random_snd_req(sound, pan, volume, 1.0f);
            }
        } else {
            pdata->delay -= 1.0f;
        }
    }
    return 1.0f;
}

void attach_pfx_to_object(
    MkObj* object, const char* effect_name, const Vec* offset) {
    int emitter;
    float x;
    float y;
    float z;

    emitter = fx_next_emitter(fx_by_owner(effect_name, 4));
    if (object != 0) {
        x = object->pos.value.x + offset->x;
        y = object->pos.value.y + offset->y;
        z = object->pos.value.z + offset->z;
        if (emitter != 0) {
            fx_set_param_v3(emitter, 0x202, x, y, z);
            fx_restart_emit(emitter);
            fx_resume_emit(emitter);
        }
    }
}

/*
 * Soft ceiling: attach_pfx_to_object_by_uid ~99.1%. The stale-safe UID
 * search, render-position offset, persistent attachment record, effect calls,
 * access widths, and exact 428-byte size match retail. Residue is only a
 * consistent rotation of the four argument and tile-loop nonvolatile GPRs.
 */
void attach_pfx_to_object_by_uid(
    int uid, const char* effect_name, const Vec* offset, int keep_attached) {
    KonquestUidObject* object;

    object = find_tile_object_by_uid(uid);
    if (object != 0) {
        KonquestRenderRecord* record;

        record = (KonquestRenderRecord*)first_mkhdr(&object->render_records);
        if (record != 0) {
            float x;
            float y;
            float z;

            x = record->position.x + offset->x;
            y = record->position.y + offset->y;
            z = record->position.z + offset->z;
            if (keep_attached != 0) {
                if (strlen(effect_name) <= 31) {
                    strcpy(object->attached_pfx_name, effect_name);
                    object->attached_pfx_state = 0;
                    object->attached_pfx_position.x = x;
                    object->attached_pfx_position.y = y;
                    object->attached_pfx_position.z = z;
                }
            } else {
                unsigned int effect;
                unsigned int emitter;

                effect = fx_by_owner((void*)effect_name, 4);
                emitter = fx_next_emitter(effect);
                if (emitter != 0) {
                    fx_set_param_v3(emitter, 0x202, x, y, z);
                    fx_restart_emit(emitter);
                    fx_resume_emit(emitter);
                }
            }
        }
    }
}

/*
 * Soft ceiling: enable_attached_sound_by_uid ~74.1% -- nullable-list guard,
 * stale-link cleanup, UID comparison, and enabled store match retail. The low
 * fuzzy score is individual r28-r31 saves/restores versus stmw/lmw, plus move
 * scheduling around those saves.
 */
void enable_attached_sound_by_uid(int uid, int enabled) {
    KonquestAttachedSound* sound;
    MkPtr* link;

    if (konquest_pdata->attached_sounds != 0) {
        link = konquest_pdata->attached_sounds;
        while (link != 0) {
            sound = (KonquestAttachedSound*)link->hdr;
            if (link->instance != sound->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (sound->uid == uid) {
                    sound->enabled = enabled;
                }
                link = link->next;
            }
        }
    }
}

/*
 * Soft ceiling: attach_sound_to_object_by_uid ~99.0% -- the complete lookup,
 * stale-link cleanup, pose copy, tracker fields, and list insertion match.
 * Residue is only the tile-offset/parameter nonvolatile-register carousel.
 */
void attach_sound_to_object_by_uid(
    int uid, int sound_id, int positional_pan, int tracking_enabled,
    float min_dist, float max_dist) {
    KonquestUidObject* object;
    KonquestRenderRecord* record;

    object = find_tile_object_by_uid(uid);
    if (object != 0) {
        record = (KonquestRenderRecord*)first_mkhdr(&object->render_records);
        if (record != 0) {
            object->tracked_sound = get_sound_tracker_data();
            object->tracked_sound->pos.x = record->field_14.x;
            object->tracked_sound->pos.y = record->field_14.y;
            object->tracked_sound->pos.z = record->field_14.z;
            object->tracked_sound->sound_id = sound_id;
            object->tracked_sound->min_dist = min_dist;
            object->tracked_sound->max_dist = max_dist;
            object->tracked_sound->positional_pan = positional_pan;
            object->tracked_sound->tracking_enabled = tracking_enabled;
            object->tracked_sound->owner_uid = uid;
            make_new_tracked_sound(
                &konquest_pdata->attached_sounds, object->tracked_sound);
        }
    }
}

/*
 * Soft ceiling: attach_wiff_to_konquest_object_by_uid ~97.8% -- the complete
 * scan, stale-link cleanup, render latch, and texture calls match. Residue is
 * loop-local GPR coloring and one folded latch-success branch.
 */
void attach_wiff_to_konquest_object_by_uid(
    int uid, char* name, float frame_rate) {
    KonquestUidObject* object;
    KonquestRenderRecord* record;
    KonquestSobjBinding* binding;
    MkSobj* render_object;
    AniTextureControl* control;

    object = find_tile_object_by_uid(uid);
    if (object != 0) {
        record = (KonquestRenderRecord*)first_mkhdr(&object->render_records);
        if (record != 0) {
            binding = record->binding;
            render_object = binding->object;
            if (render_object != 0) {
                if (render_object->hdr.instance == binding->object_instance) {
                    /* Valid render-object latch. */
                } else {
                    render_object = 0;
                }
            } else {
                render_object = 0;
            }
            if (render_object != 0) {
                control = attach_named_wiff_to_first_material(
                    0x60029, name, (ImageMkSobj*)render_object);
                set_ani_texture_framerate(control, frame_rate);
            }
        }
    }
}

/*
 * Soft ceiling: set_konquest_object_render_order_priority_by_uid ~97.6% --
 * the complete scan, stale-link cleanup, render latch, and call match. The
 * residue is GPR coloring and one folded latch-success branch.
 */
void set_konquest_object_render_order_priority_by_uid(
    int uid, int priority) {
    KonquestUidObject* object;
    KonquestRenderRecord* record;
    KonquestSobjBinding* binding;
    MkSobj* render_object;

    object = find_tile_object_by_uid(uid);
    if (object != 0) {
        record = (KonquestRenderRecord*)first_mkhdr(&object->render_records);
        if (record != 0) {
            binding = record->binding;
            render_object = binding->object;
            if (render_object != 0) {
                if (render_object->hdr.instance == binding->object_instance) {
                    /* Valid render-object latch. */
                } else {
                    render_object = 0;
                }
            } else {
                render_object = 0;
            }
            if (render_object != 0) {
                sobj_set_priority(render_object, priority);
            }
        }
    }
}

/*
 * Soft ceiling: disable_konquest_object_zwrite_by_uid ~97.6% -- the complete
 * lookup, latch, and bitfield store match; only GPR coloring and one folded
 * latch-success branch remain.
 */
void disable_konquest_object_zwrite_by_uid(int uid) {
    KonquestUidObject* object;
    KonquestRenderRecord* record;
    KonquestSobjBinding* binding;
    MkSobj* render_object;

    object = find_tile_object_by_uid(uid);
    if (object != 0) {
        record = (KonquestRenderRecord*)first_mkhdr(&object->render_records);
        if (record != 0) {
            binding = record->binding;
            render_object = binding->object;
            if (render_object != 0) {
                if (render_object->hdr.instance == binding->object_instance) {
                    /* Valid render-object latch. */
                } else {
                    render_object = 0;
                }
            } else {
                render_object = 0;
            }
            if (render_object != 0) {
                render_object->flags09_bits.bit7 = 1;
            }
        }
    }
}

/*
 * Soft ceiling: set_konquest_object_face_y_by_uid ~97.6% -- the complete
 * lookup, latch, and bitfield store match; only GPR coloring and one folded
 * latch-success branch remain.
 */
void set_konquest_object_face_y_by_uid(int uid) {
    KonquestUidObject* object;
    KonquestRenderRecord* record;
    KonquestSobjBinding* binding;
    MkSobj* render_object;

    object = find_tile_object_by_uid(uid);
    if (object != 0) {
        record = (KonquestRenderRecord*)first_mkhdr(&object->render_records);
        if (record != 0) {
            binding = record->binding;
            render_object = binding->object;
            if (render_object != 0) {
                if (render_object->hdr.instance == binding->object_instance) {
                    /* Valid render-object latch. */
                } else {
                    render_object = 0;
                }
            } else {
                render_object = 0;
            }
            if (render_object != 0) {
                render_object->flags09_bits.bit5 = 1;
            }
        }
    }
}

/*
 * Soft ceiling: unhide_konquest_object_by_uid ~99.2% -- all operations,
 * branches, calls, and access widths match; only the scan's GPR coloring
 * differs.
 */
void unhide_konquest_object_by_uid(int uid) {
    KonquestUidObject* object;

    object = find_tile_object_by_uid(uid);
    if (object != 0 && object->hidden != 0) {
        KonquestTileRecord* tile;

        object->hidden = 0;
        tile = (KonquestTileRecord*)get_nth_tile_struct(object->tile_index);
        if (tile->visible != 0) {
            show_konquest_object(object);
        }
    }
}

/*
 * Near match at the exact retail size: the stale-safe UID search, visibility
 * gates, effect reset, palette compaction, object latch, and final hidden-state
 * stores all agree. Residue is local scan GPR coloring, equivalent latch CFG,
 * and matrix-address scheduling.
 */
void hide_konquest_object_by_uid(int uid) {
    KonquestUidObject* object;

    object = find_tile_object_by_uid(uid);
    if (object != 0 && object->hidden == 0) {
        KonquestTileRecord* tile;

        object->hidden = 1;
        tile = (KonquestTileRecord*)get_nth_tile_struct(object->tile_index);
        if (tile->visible != 0) {
            if ((signed char)object->attached_pfx_name[0] != 0 &&
                object->attached_pfx_state != 0) {
                fx_reset_emit(object->attached_pfx_state);
            }
            if (object->render_records != 0) {
                MkPtr* record_link;

                record_link = object->render_records;
                while (record_link != 0) {
                    KonquestRenderRecord* record;

                    record = (KonquestRenderRecord*)record_link->hdr;
                    if (record_link->instance != record->hdr.instance) {
                        MkPtr* next;

                        next = record_link->next;
                        record_link->hdr = 0;
                        destroy_mkptr(record_link);
                        record_link = next;
                        continue;
                    }

                    if (record->matrix_index >= 0) {
                        KonquestSobjBinding* binding;
                        KonquestMatrixPalette* palette;

                        binding = record->binding;
                        palette = binding->palette;
                        if (palette != 0) {
                            KonquestRenderRecord** records;
                            int index;
                            int last_index;

                            index = record->matrix_index;
                            if (index >= 0 && palette != 0) {
                                records = palette->records;
                                last_index = palette->count - 1;
                                if (index != last_index) {
                                    KonquestRenderRecord* last;

                                    last = records[last_index];
                                    if (record != 0 && last != 0) {
                                        last->matrix_index = index;
                                        set_mat(
                                            &palette->matrices[index],
                                            &palette->matrices[last_index]);
                                        records[index] = last;
                                    }
                                }
                                records[palette->count - 1] = 0;
                                palette->count--;
                                if (palette->count < 0) {
                                    palette->count = 0;
                                }
                            }
                        } else {
                            MkSobj* render_object;

                            render_object = binding->object;
                            if (render_object != 0) {
                                if (render_object->hdr.instance ==
                                    binding->object_instance) {
                                    /* Valid render-object latch. */
                                } else {
                                    render_object = 0;
                                }
                            } else {
                                render_object = 0;
                            }
                            if (render_object != 0) {
                                hide_sobj_and_children(render_object);
                            }
                        }
                    }
                    record->binding->hidden = 1;
                    record->matrix_index = -1;
                    record_link = record_link->next;
                }
            }
        }
    }
}

/*
 * Near match at the exact retail size: stale-link cleanup, attached-effect
 * restart, matrix-palette insertion and pose construction, render-object
 * latching, pose stores, and visibility updates agree instruction-for-
 * instruction. The sole residue is an equivalent valid-instance branch shape.
 */
static void show_konquest_object(KonquestUidObject* object) {
    int record_number;
    KonquestRenderRecord* record;
    MkPtr* link;

    record_number = 0;
    if (object->hidden == 0 && object->render_records != 0) {
        link = object->render_records;
        while (link != 0) {
            record = (KonquestRenderRecord*)link->hdr;
            if (link->instance != record->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }

            if (record_number == 0 &&
                (signed char)object->attached_pfx_name[0] != 0) {
                unsigned int effect;

                effect = fx_by_owner(object->attached_pfx_name, 4);
                if (object->attached_pfx_state != 0) {
                    fx_reset_emit(object->attached_pfx_state);
                }
                object->attached_pfx_state = fx_next_emitter(effect);
                if (object->attached_pfx_state != 0) {
                    fx_set_param_v3(
                        object->attached_pfx_state, 0x202,
                        object->attached_pfx_position.x,
                        object->attached_pfx_position.y,
                        object->attached_pfx_position.z);
                    fx_resume_emit(object->attached_pfx_state);
                }
            }

            {
                KonquestSobjBinding* binding;
                KonquestMatrixPalette* palette;

                binding = record->binding;
                palette = binding->palette;
                if (palette != 0) {
                    if (palette->count != palette->capacity) {
                        MKMATRIX matrix __attribute__((aligned(16)));
                        float x;
                        float y;
                        float z;

                        record->matrix_index = palette->count;
                        palette->records[palette->count] = record;
                        MKMatrixSetIdentity(&matrix);
                        x = record->position.x;
                        y = record->position.y;
                        z = record->position.z;
                        YXZ_angles_to_MKMATRIX(&record->angles, &matrix);
                        set_mat(&palette->matrices[palette->count], &matrix);
                        palette->matrices[palette->count].pos.x = x;
                        palette->matrices[palette->count].pos.y = y;
                        palette->matrices[palette->count].pos.z = z;
                        palette->count++;
                    }
                } else {
                    MkSobj* render_object;

                    render_object = binding->object;
                    if (render_object != 0) {
                        if (render_object->hdr.instance ==
                            binding->object_instance) {
                            /* Valid render-object latch. */
                        } else {
                            render_object = 0;
                        }
                    } else {
                        render_object = 0;
                    }
                    if (render_object != 0 && binding->hidden != 0) {
                        render_object->pos.x = record->position.x;
                        render_object->pos.y = record->position.y;
                        render_object->pos.z = record->position.z;
                        render_object->ang.x = record->angles.x;
                        render_object->ang.y = record->angles.y;
                        render_object->ang.z = record->angles.z;
                        update_mksobj(render_object);
                        unhide_sobj_and_children(render_object);
                        record->binding->hidden = 0;
                        record->matrix_index = 0;
                    }
                }
            }

            link = link->next;
            record_number++;
        }
    }
}

/*
 * Soft ceiling: find_konquest_object_struct_by_uid ~86.2% -- the shared typed
 * tile/list scan inlines at exact retail size. Residue is offset/zero/next GPR
 * rotation and a joined r5 return instead of retail's per-edge r3 values.
 */
void* find_konquest_object_struct_by_uid(int uid) {
    return find_tile_object_by_uid(uid);
}

/*
 * Soft ceiling: start_konquest_ambient_sounds 99.78%, with exact size and
 * operations. The only remaining differences are two constant-pool relocation
 * labels for the zero and integer-to-float conversion constants.
 */
void start_konquest_ambient_sounds(void) {
    KonquestAmbientFadePdata* fade;

    konquest_pdata->ambient_sound_main =
        snd_req(konquest_pdata->region_table->ambient_sound);
    _create_mkproc_generic_tinystack(
        0x8226, 0x1F, p_cross_fade_ambient_sounds, sizeof(*fade),
        (void**)&fade);
    if (fade != 0) {
        fade->step_a = 0.0f;
        fade->step_b = 0.0f;
        fade->volume_a = 0.0f;
        fade->volume_b = 0.0f;
        fade->delay = (float)((unsigned short)randu0(0x384) + 0x12C);
    }
}

void stop_konquest_ambient_sounds(void) {
    snd_stop(konquest_pdata->ambient_sound_main);
    snd_stop(konquest_pdata->ambient_sound_a);
    snd_stop(konquest_pdata->ambient_sound_b);
    konquest_pdata->ambient_sound_main = 0;
    konquest_pdata->ambient_sound_a = 0;
    konquest_pdata->ambient_sound_b = 0;
    destroy_mkprocs_pid(0x8226);
}

/*
 * Soft ceiling: vdestroy_konquest_pui ~89.4% at the exact 436-byte size.
 * Effect cleanup, three owned-object destructions, both instance latches,
 * pointer clears, and final header free match retail. Residue is two folded
 * latch joins, their temporary GPR coloring, and expanded saves/restores.
 */
void vdestroy_konquest_pui(struct KonquestPuiRuntime* pui) {
    KonquestChestOwner* owner;
    MkObj* render_object;

    if ((int)pui->effect_60 != 0) {
        fx_reset_emit(pui->effect_60);
    }
    if ((int)pui->effect_64 != 0) {
        fx_reset_emit(pui->effect_64);
    }
    if (pui->effect_clone != 0 &&
        pui->effect_clone->hdr.instance != 0) {
        pui->effect_clone->hdr.typed_vtbl->destroy(
            &pui->effect_clone->hdr);
    }
    if (pui->collision_object != 0) {
        MkHdr* header;

        header = pui->collision_object != 0
                     ? as_mkhdr(&pui->collision_object->hdr)
                     : 0;
        if (header->instance != 0) {
            MkHdr* destroy_arg;

            destroy_arg = pui->collision_object != 0
                              ? as_mkhdr(&pui->collision_object->hdr)
                              : 0;
            header = pui->collision_object != 0
                         ? as_mkhdr(&pui->collision_object->hdr)
                         : 0;
            header->typed_vtbl->destroy(destroy_arg);
        }
    }

    owner = pui->owner;
    if (owner != 0) {
        if (owner->hdr.instance != pui->owner_instance) {
            owner = 0;
        }
    } else {
        owner = 0;
    }
    if (owner != 0) {
        if (pui->owner->hdr.instance != 0) {
            pui->owner->hdr.typed_vtbl->destroy(&pui->owner->hdr);
        }
        pui->owner = 0;
        pui->owner_instance = 0;
    }

    render_object = pui->render_object;
    if (render_object != 0) {
        if (render_object->hdr.instance != pui->render_object_instance) {
            render_object = 0;
        }
    } else {
        render_object = 0;
    }
    if (render_object != 0) {
        if (pui->render_object->hdr.instance != 0) {
            pui->render_object->hdr.typed_vtbl->destroy(
                &pui->render_object->hdr);
        }
        pui->render_object = 0;
        pui->render_object_instance = 0;
    }

    pui->hdr.instance = 0;
    mkhdr_memfree(&pui->hdr);
}

/*
 * Near match: p_konquest_loop 97.54% (1016 versus 1028 bytes). Time rollover,
 * the complete controller path, world updates, navigation, and shadow-strength
 * clamps agree. Residue is one hero-latch join, the equivalent state-filter
 * branch tree, and pooled-float relocation labels.
 */
float p_konquest_loop(void) {
    CameraPdata* camera;
    MkObj* hero;
    float horizontal;
    float vertical;
    float horizontal_input;
    float vertical_input;
    float magnitude;
    int mode_index;
    int mode;
    int update_all;

    update_all = 0;
    if (konquest_pdata->time_passing != 0) {
        konquest_pdata->time_of_day += game_hours_per_tick;
        if (konquest_pdata->time_of_day >= 24.0f) {
            konquest_pdata->time_of_day = 0.0f;
            snd_req(konquest_pdata->region_table->time_chime_sound);
            increment_day(&konquest_pdata->current_time);
            update_all = 1;
        }
        konquest_pdata->current_time.hour =
            (int)konquest_pdata->time_of_day;
        konquest_pdata->current_time.minute =
            (int)(60.0f *
                  (konquest_pdata->time_of_day -
                   (float)konquest_pdata->current_time.hour));
        if (konquest_pdata->time_of_day >= 12.0f &&
            konquest_pdata->time_of_day - 12.0f <= game_hours_per_tick) {
            snd_req(konquest_pdata->region_table->time_chime_sound);
        }
    }
    update_time_screen_objs(update_all);

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    mode_index = konquest_pdata->game_mode_index;
    mode = mode_index < 0 ? 0 : konquest_pdata->game_modes[mode_index];
    if (mode == 0 ||
        (mode_index = konquest_pdata->game_mode_index,
         (mode_index < 0 ? 0 : konquest_pdata->game_modes[mode_index]) == 4)) {
        if (get_game_state() != 0x14 && get_game_state() != 0x15) {
            if (check_switch(konquest_pdata->input_port, 2) != 0) {
                correct_camera_angle(hero, 1);
            }
            if (get_stick_pos(
                    konquest_pdata->input_port, 1, &horizontal,
                    &vertical) != 0) {
                horizontal_input = horizontal;
                magnitude = horizontal_input >= 0.0f
                                ? horizontal_input
                                : -horizontal_input;
                if (magnitude > 0.01f) {
                    camera = get_pdata_of_camera();
                    if (camera != 0) {
                        camera->target_ang.y = norm_angle(
                            camera->target_ang.y -
                            0.035f * horizontal_input);
                        if (konquest_pdata->hero_state == 0 &&
                            konquest_pdata->npc_interaction_state == 0) {
                            konquest_pdata->npc_interaction_state = 1;
                        }
                    }
                }
                vertical_input = vertical;
                camera = get_pdata_of_camera();
                if (camera != 0) {
                    switch (konquest_pdata->hero_state) {
                    case 0:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 9:
                        camera->target_ang.x =
                            norm_angle(0.3f * vertical_input + 0.17f);
                        break;
                    }
                }
            }
        }
    }
    update_tile_grid();
    npc_update(0);
    trigger_update(0);
    pui_update();

    hero = konquest_pdata->hero_object;
    if (hero != 0) {
        if (hero->hdr.instance == konquest_pdata->hero_instance) {
            /* Valid hero latch. */
        } else {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        if (konquest_pdata->current_nav_area >= 0) {
            konquest_pdata->current_nav_area = nav_what_area_is_point_in(
                &hero->pos.value, konquest_pdata->current_nav_area);
        } else {
            konquest_pdata->current_nav_area =
                nav_what_area_is_point_in(&hero->pos.value, 0);
        }
        if (is_point_inside_shadow_exclusion_zone(
                &hero->pos.value, 2.5f) != 0) {
            if (konquest_pdata->base_shadow_strength > 0.0f) {
                konquest_pdata->base_shadow_strength -= 0.1f;
                konquest_pdata->base_shadow_strength =
                    konquest_pdata->base_shadow_strength >= 0.0f
                        ? konquest_pdata->base_shadow_strength
                        : 0.0f;
            }
        } else if (konquest_pdata->base_shadow_strength < 1.0f) {
            konquest_pdata->base_shadow_strength += 0.1f;
            konquest_pdata->base_shadow_strength =
                konquest_pdata->base_shadow_strength <= 1.0f
                    ? konquest_pdata->base_shadow_strength
                    : 1.0f;
        }
    } else {
        konquest_pdata->current_nav_area = -1;
    }
    return 1.0f;
}
static inline int trigger_requirements_pass(
    KonquestTriggerStruct* trigger) {
    unsigned int requirements;
    int index;

    if (trigger == 0) {
        return 0;
    }
    if (trigger->flag_bits.bit7) {
        trigger->flag_bits.bit7 = 0;
        return 1;
    }
    requirements = trigger->owned_data->flags;
    if (requirements == 0) {
        return 0;
    }
    for (index = 1; index < 4; index++) {
        if ((requirements & 1) != 0 &&
            trigger_function_table[index](trigger) == 0) {
            return 0;
        }
        requirements >>= 1;
    }
    return 1;
}


/*
 * Near match: timed actions, predicate dispatch, process ownership, trigger
 * entry/exit, interior-PUI gating, effect/collision teardown, and both stale
 * list walks agree. The 56-byte deficit is equivalent inline-helper and latch
 * branch folding plus nonvolatile-register scheduling; the timed-action callee
 * remains an exact 100% match.
 */
void trigger_update(int force) {
    MkPtr* link;

    trigger_update_countdown--;
    if (force == 0 && konquest_current_game_mode() == 0 &&
        konquest_pdata->hero_state != 8 &&
        konquest_pdata->hero_state != 9 &&
        konquest_pdata->hero_state != 10 &&
        konquest_pdata->temporary_triggers != 0) {
        link = konquest_pdata->temporary_triggers;
        while (link != 0) {
            KonquestTriggerStruct* trigger;

            trigger = (KonquestTriggerStruct*)link->hdr;
            if (link->instance != trigger->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
                continue;
            }
            if (trigger->timed_action != 0) {
                check_and_act_on_trigger_timed_action(trigger);
            }
            if (trigger->owned_data->state != 0 &&
                !trigger->flag_bits.bit3 &&
                trigger_requirements_pass(trigger) != 0 &&
                (trigger->owned_data->type != 2 ||
                 konquest_pdata->active_trigger == 0)) {
                MkProc* proc;
                KonquestTriggerScriptPdata* pdata;

                proc = trigger->script_proc;
                if (proc != 0) {
                    if (proc->instance != trigger->script_proc_instance) {
                        proc = 0;
                    }
                } else {
                    proc = 0;
                }
                if (proc != 0) {
                    xfer_proc(proc, p_run_trigger_script);
                } else {
                    proc = _create_mkproc_generic_bigstack(
                        0x9019, 0x18, p_run_trigger_script,
                        sizeof(*pdata), (void**)&pdata);
                    if (proc != 0) {
                        set_process_as_scriptable(proc);
                        trigger->script_proc = proc;
                        trigger->script_proc_instance = proc->instance;
                        pdata->trigger = trigger;
                        pdata->trigger_instance = trigger->hdr.instance;
                    }
                }
                if (proc != 0) {
                    if (trigger->owned_data->type == 2) {
                        konquest_pdata->active_trigger = trigger;
                        konquest_pdata->active_trigger_instance =
                            trigger->hdr.instance;
                    }
                    trigger->flag_bits.bit3 = 1;
                    if (trigger->owned_data->type == 1) {
                        trigger->owned_data->state = 0;
                    }
                }
            }
            link = link->next;
        }
    }

    if (force != 0 || trigger_update_countdown <= 0) {
        MkObj* hero;

        trigger_update_countdown = 120;
        hero = konquest_pdata->hero_object;
        if (hero != 0) {
            if (hero->hdr.instance != konquest_pdata->hero_instance) {
                hero = 0;
            }
        } else {
            hero = 0;
        }
        if (hero != 0 && konquest_pdata->triggers != 0) {
            link = konquest_pdata->triggers;
            while (link != 0) {
                KonquestTriggerStruct* trigger;

                trigger = (KonquestTriggerStruct*)link->hdr;
                if (link->instance != trigger->hdr.instance) {
                    MkPtr* next;

                    next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                    continue;
                }
                if (trigger != 0) {
                    KonquestTriggerDefinition* definition;
                    float delta_x;
                    float delta_z;
                    float range;

                    definition = trigger->owned_data;
                    delta_z = definition->position.z - hero->pos.value.z;
                    delta_x = definition->position.x - hero->pos.value.x;
                    range = 43.2f + definition->radius;
                    if (delta_x * delta_x + delta_z * delta_z <
                        range * range) {
                        if (!trigger->flag_bits.bit2) {
                            if (definition->type == 2) {
                                KonquestPuiRuntime* pui;

                                pui = definition->pui;
                                if (pui != 0) {
                                    if (pui->item->position.x >= 1000.0f) {
                                        if (is_pui_in_current_interior(
                                                pui->item) != 0 &&
                                            !trigger->flag_bits.bit2) {
                                            handle_trigger_preprocess(trigger);
                                            mk_insert(
                                                &trigger->hdr,
                                                &konquest_pdata
                                                     ->temporary_triggers);
                                            trigger->flag_bits.bit2 = 1;
                                        }
                                    } else if (!trigger->flag_bits.bit2) {
                                        handle_trigger_preprocess(trigger);
                                        mk_insert(
                                            &trigger->hdr,
                                            &konquest_pdata
                                                 ->temporary_triggers);
                                        trigger->flag_bits.bit2 = 1;
                                    }
                                }
                            } else if (!trigger->flag_bits.bit2) {
                                handle_trigger_preprocess(trigger);
                                mk_insert(
                                    &trigger->hdr,
                                    &konquest_pdata->temporary_triggers);
                                trigger->flag_bits.bit2 = 1;
                            }
                        }
                    } else if (trigger->flag_bits.bit2 && trigger != 0) {
                        remove_trigger_from_world(trigger);
                    }
                }
                link = link->next;
            }
        }
    }
}

#pragma dont_inline on
static void check_and_act_on_trigger_timed_action(
    KonquestTriggerStruct* trigger) {
    if (is_time_a_greater_than_time_b(
            &konquest_pdata->current_time, &trigger->action_time) != 0) {
        switch (trigger->timed_action) {
        case 1:
            enable_trigger(trigger->owned_data, 1);
            break;
        case 2:
            enable_trigger(trigger->owned_data, 0);
            break;
        }
        trigger->timed_action = 0;
    }
}
#pragma dont_inline reset

/*
 * Near match: stale-link removal, object latch, inventory clearing, virtual
 * destruction, and live update match. Residue is paired save/restore emission,
 * one folded latch join, and raw-versus-resolved PUI register coloring.
 */
void pui_update(void) {
    MkPtr* link;

    if (konquest_pdata->pui_list != 0) {
        link = konquest_pdata->pui_list;
        while (link != 0) {
            KonquestPuiRuntime* pui;

            pui = (KonquestPuiRuntime*)link->hdr;
            if (link->instance != pui->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (pui != 0) {
                    KonquestChestOwner* owner;

                    owner = pui->owner;
                    if (owner != 0) {
                        if (owner->hdr.instance == pui->owner_instance) {
                            /* Valid PUI object latch. */
                        } else {
                            owner = 0;
                        }
                    } else {
                        owner = 0;
                    }
                    if (owner == 0) {
                        int inventory_index;

                        if (pui->item == 0) {
                            inventory_index = -1;
                        } else {
                            unsigned int table_index;
                            unsigned int first;

                            table_index = get_table_index_by_pointer(
                                konquest_pdata->script_owner, pui->item);
                            first = konquest_pdata->pui_begin;
                            if (table_index <= first ||
                                table_index >= konquest_pdata->pui_end) {
                                inventory_index = -1;
                            } else {
                                inventory_index = table_index - (first + 1);
                            }
                        }
                        if (inventory_index >= 0) {
                            set_u8_bit(
                                konquest_pdata->pui_inventory_bits,
                                (konquest_pdata->pui_end -
                                 konquest_pdata->pui_begin) -
                                    1,
                                inventory_index, 0);
                        }
                        if (pui->hdr.instance != 0) {
                            pui->hdr.typed_vtbl->destroy(&pui->hdr);
                        }
                    } else {
                        update_konquest_pui(pui);
                    }
                }
                link = link->next;
            }
        }
    }
}

int get_konquest_pui_inventory_bit_index(const int* pui) {
    if (pui != 0) {
        return pui[6];
    }
    return -1;
}

/*
 * Soft ceiling: get_konquest_pui_object_pos ~94.5% -- only the common
 * ownership-latch join branch is folded; all three float accesses are exact.
 */
void get_konquest_pui_object_pos(Vec* position, const MkSobj* sobj) {
    MkObj* object;

    object = (MkObj*)sobj->bound_hdr;
    if (object != 0) {
        if (object->hdr.instance == sobj->bound_instance) {
            /* Valid object latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (position != 0) {
        position->x = object->pos.value.x;
        position->y = object->pos.value.y;
        position->z = object->pos.value.z;
    }
}

int check_skip_conversation_flag(void) {
    if (konquest_pdata != 0) {
        return (konquest_pdata->flags >> 4) & 1;
    }
    return 0;
}

/*
 * Near match: model setup and all seven sky-subobject configurations are
 * instruction-for-instruction equivalent. Only pooled relocation labels and
 * individual r30/r31 saves versus retail's stmw/lmw remain (eight bytes).
 */
static void load_sky(void) {
    MkSobj* object;

    g_game_info.sky = load_named_model_from_slot(
        0x60029, "SKY", 0x1004, 0);
    if (g_game_info.sky == 0) {
        return;
    }

    g_game_info.sky->light_flags = 8;
    g_game_info.sky->flags_08_bits.airborne = 1;
    g_game_info.sky->flags_08_bits.angular_velocity_enabled = 1;
    konquest_pdata->field_174 = 5000.0f;
    obj_create_sobjs(g_game_info.sky);

    object = obj_find_sobj_by_id(g_game_info.sky, 40);
    if (object != 0) {
        hide_sobj(object);
        sobj_set_priority(object, 1);
        object->flags09_bits.bit7 = 1;
        object->flags_08_bits.bit6 = 1;
        object->flags_08_bits.bit3 = 1;
        sobj_use_material_color(object);
    }

    object = obj_find_sobj_by_id(g_game_info.sky, 30);
    if (object != 0) {
        hide_sobj(object);
        sobj_set_priority(object, 1);
        object->flags09_bits.bit7 = 1;
        object->flags_08_bits.bit6 = 1;
        object->flags_08_bits.bit3 = 1;
        sobj_use_material_color(object);
    }

    object = obj_find_sobj_by_id(g_game_info.sky, 20);
    if (object != 0) {
        sobj_set_priority(object, 2);
        object->flags09_bits.bit7 = 1;
        start_sobj_uv_scroll(
            g_game_info.sky, 20, 0.0005f, -0.001f, 0.0f, 0.0f);
    }

    object = obj_find_sobj_by_id(g_game_info.sky, 10);
    if (object != 0) {
        sobj_set_priority(object, 0);
        start_sobj_uv_scroll(
            g_game_info.sky, 10, 0.001f, 0.0f, 0.0f, 0.0f);
        sobj_use_material_color(object);
    }

    object = obj_find_sobj_by_id(g_game_info.sky, 50);
    if (object != 0) {
        hide_sobj(object);
    }

    object = obj_find_sobj_by_id(g_game_info.sky, 60);
    if (object != 0) {
        sobj_set_priority(object, 3);
        sobj_use_material_color(object);
        hide_sobj(object);
        update_mksobj(object);
    }

    object = obj_find_sobj_by_id(g_game_info.sky, 1);
    if (object != 0) {
        object->flags_08_bits.bit6 = 1;
        object->flags_08_bits.scale_dirty = 1;
        object->flags09_bits.bit7 = 1;
        hide_sobj(object);
    }
}

/*
 * Near match at the exact retail size: region/script loading, the fourteen
 * animation publications, hero/camera latches, common resources, objective
 * beam and weather setup, lighting/fog, saved-state restoration, objective
 * scan, controller gate, HUD label, triggers, ambience, and final transfer all
 * agree with retail. The 96.9% residue is five inverted-but-equivalent latch
 * branches, GPR/FPR allocation and scheduling, and pooled relocation labels.
 */
float p_setup_konquest_map(void) {
    KonquestAmbientFadePdata* ambient_fade;
    KonquestHeadTrackingPdata* head_tracking;
    KonquestWeatherPdata* weather;
    MkHdr* objective_process_data;
    MkHdr* process_data = 0;
    char grid_text[0x18];
    MkObj* hero;
    MkObj* beam;
    MkObj* light_object;
    MkProc* hero_proc;
    MkProc* weather_proc;
    AnimPdata* hero_anim;
    CameraPdata* camera;
    MkSobj* beam_part;
    MkSobj* beam_glow;
    ScreenObj* arrow;
    StringObj* grid_label;
    KonquestObjectiveRow* objective_table;
    KonquestObjectiveRow* objective_row;
    int region;
    int pause_slot;
    int objective_index;

    if (konquest_save_data.progression != 0) {
        display_time_progression_images(konquest_save_data.progression);
        konquest_save_data.progression = 0;
        fade_from_black(8, 1);
    }

    konquest_pdata->region_index = p1_profile_konquest->fields.region_arg;
    display_load_meter(0x60027);
    turn_camera_on();
    set_process_as_scriptable(aproc);
    activate_cmdscript();

    region = konquest_pdata->region_index;
    load_ssf(konquest_region_data[region].fight_files);
    konquest_pdata->script_owner = cmdscript_loadfile_by_name(
        0xB, konquest_region_data[region].script_name);
    konquest_pdata->region_table = (KonquestRegionTable*)get_data_table(
        konquest_pdata->script_owner,
        ((KonquestScriptSlotHeaderView*)konquest_pdata->script_owner)
            ->table_count);

    if (konquest_editor_mode_on == 0) {
        init_shadow_system();
    }
    if (konquest_editor_mode_on == 0) {
        setup_sound_banks(3);
        wait_for_sound_banks_to_load();
        start_sound_tracking_process(&konquest_pdata->attached_sounds);
    }
    npc_shadow_init();

    if (konquest_editor_mode_on == 0) {
        unload_section_slot(0x60028);
        add_anim_section_async(
            0x60028, &sec_konquest_monk_anims, konquest_animations, 0, 1);
        wait_for_slot_load(0x60028);

        monk_state_data[0].animation = (AniData*)konquest_animations[0];
        monk_state_data[1].animation = (AniData*)konquest_animations[1];
        monk_state_data[2].animation = (AniData*)konquest_animations[2];
        monk_state_data[3].animation = (AniData*)konquest_animations[4];
        monk_state_data[4].animation = (AniData*)konquest_animations[5];
        monk_state_data[5].animation = (AniData*)konquest_animations[6];
        monk_state_data[6].animation = (AniData*)konquest_animations[5];
        monk_state_data[7].animation = (AniData*)konquest_animations[6];
        monk_state_data[8].animation = (AniData*)konquest_animations[7];
        monk_state_data[9].animation = (AniData*)konquest_animations[8];
        monk_state_data[10].animation = (AniData*)konquest_animations[9];
        monk_state_data[11].animation = (AniData*)konquest_animations[10];
        monk_state_data[12].animation = (AniData*)konquest_animations[11];
        monk_state_data[13].animation = (AniData*)konquest_animations[12];

        hero = konquest_pdata->hero_object;
        if (hero != 0) {
            if (hero->hdr.instance != konquest_pdata->hero_instance) {
                hero = 0;
            }
        } else {
            hero = 0;
        }

        if (hero == 0 && konquest_pdata->hero_anim == 0 &&
            (hero_proc = load_hero_model(konquest_animations[0])) != 0) {
            hero_anim = (AnimPdata*)pdata_of_proc(hero_proc);
            hero = hero_anim->obj;
            if (hero != 0) {
                if (hero->hdr.instance != hero_anim->obj_instance) {
                    hero = 0;
                }
            } else {
                hero = 0;
            }
            if (hero != 0) {
                konquest_pdata->hero_object = hero;
                konquest_pdata->hero_instance = hero->hdr.instance;
            }
            konquest_pdata->hero_anim = hero_anim;
            konquest_pdata->hero_state = 0;
            hero->hide_flag_bits.still_move = 1;
        }

        camera = get_pdata_of_camera();
        if (camera != 0) {
            hero = konquest_pdata->hero_object;
            if (hero != 0) {
                if (hero->hdr.instance != konquest_pdata->hero_instance) {
                    hero = 0;
                }
            } else {
                hero = 0;
            }
            if (hero != 0) {
                camera->movement_focus = hero;
            }
        }

        load_ssf(konquest_common_file_table);
        load_art_section_language(0x60030, &sec_konquest_common_art);
        load_font(0);
        load_font(6);
        load_font(0xE);
        load_font(9);
        load_string_bank(0x30000, "konquest_item_descriptions_eng.mko");
        load_string_bank(
            0x40000, "konquest_info_box_descriptions_eng.mko");
        load_string_bank(0x50000, "krypt_strings_eng.mko");

        ticks_per_game_day = 60.0f * (600.0f / game_speed);
        ticks_per_hour = ticks_per_game_day / 24.0f;
        game_hours_per_tick = 1.0f / ticks_per_hour;
        sun_angle_change_per_tick = 6.2831855f / ticks_per_game_day;
        konquest_pdata->time_rate_changed = 1;

        pause_slot = get_pause_menu_ssh();
        unload_section_slot(pause_slot);
        preload_screen_data("konquest/in_game/inventory", pause_slot);
        preload_screen_data("konquest/popups/k_pause_menu", pause_slot);

        beam = load_named_model_from_slot(0x60030, "BEAM", 0x8039, 1);
        if (beam != 0) {
            beam->flags_08_bits.airborne = 1;
            obj_create_sobjs(beam);

            beam_part = obj_find_sobj_by_id(beam, 1);
            if (beam_part != 0) {
                sobj_set_priority(beam_part, 0x14);
                sobj_start_uv_scroll(
                    beam, beam_part, 0.0f, 0.01f, 0.0f, 0.0f);
                beam_part->flags09_bits.bit7 = 1;
                beam_part->flags09_bits.bit0 = 1;
                beam_part->flags_08_bits.scale_dirty = 1;
            }

            beam_glow = obj_find_sobj_by_id(beam, 2);
            if (beam_glow != 0) {
                sobj_set_priority(beam_glow, 0xB);
                sobj_start_uv_scroll(
                    beam, beam_glow, 0.0f, -0.01f, 0.0f, 0.0f);
                beam_glow->flags09_bits.bit7 = 1;
                beam_glow->flags09_bits.bit0 = 1;
            }

            konquest_pdata->objective_beam.object = &beam->hdr;
            konquest_pdata->objective_beam.instance = beam->hdr.instance;
            insert_fgnd_mkobj(beam);
            hide_obj(beam);

            arrow = load_2d_pfxobj_xy(
                0x60030, 0x8301, (char*)0x0A8C000B, 0, 0x52,
                screen_height - 0x5A, 0x46);
            konquest_pdata->hud_objects[2].object = (MkHdr*)arrow;
            konquest_pdata->hud_objects[2].instance = arrow->instance;

            _create_mkproc_generic_tinystack(
                0x9016, 0x1F, p_adjust_objective_arrow_and_beam,
                sizeof(*objective_process_data),
                (void**)&objective_process_data);
        }
        konquest_hide_hud(0);
    } else {
        load_font(0);
        load_font(0xE);
    }

    load_and_init_konquest_map_specific_data(konquest_pdata->region_index);
    load_sky();

    if (konquest_editor_mode_on == 0) {
        init_heads_up_display();

        if (konquest_pdata->region_table->weather_table != 0) {
            weather = 0;
            weather_proc = _create_mkproc_generic_tinystack(
                0x8237, 0x1F, p_weather, sizeof(*weather),
                (void**)&weather);
            if (weather_proc != 0 && weather != 0) {
                set_process_as_scriptable(weather_proc);
                activate_cmdscript();
                weather->initialized = 0;
            }
        }

        RwCameraSetFarClipPlane(
            Camera, konquest_pdata->region_table->far_clip_distance);
    }

    fog_density = konquest_pdata->region_table->fog_density;
    fog_distance = konquest_pdata->region_table->fog_distance;
    fog_type = 1;
    turn_fog_on();

    _create_mkproc_generic_tinystack(
        0x5009, 0x28, p_adjust_sky, sizeof(*process_data),
        (void**)&process_data);

    if (konquest_editor_mode_on == 0) {
        konquest_pdata->collision_proc = _create_mkproc_generic_tinystack(
            0x822A, 0x1A, p_collide_monk, sizeof(*process_data),
            (void**)&process_data);
    }

    light_object = load_light(
        (LightDef*)&konquest_pdata->directional_light, &bgnd_light_list, 0);
    if (light_object != 0) {
        light_object->flags_08_bits.angular_velocity_enabled = 1;
    }
    load_light(
        (LightDef*)&konquest_pdata->ambient_light, &bgnd_light_list, 0);
    load_light(
        (LightDef*)&konquest_pdata->sky_ambient_light, &special_light_list, 0);

    if (konquest_editor_mode_on == 0) {
        full_konquest_load_from_memcard();
        start_running_npcs();
    }

    if (konquest_save_data.valid != 0) {
        konquest_restore_saved_state();
        konquest_save_data.valid = 0;
        _mkproc_sleep_ticks = 2.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_editor_mode_on == 0) {
        hero = konquest_pdata->hero_object;
        if (hero != 0) {
            if (hero->hdr.instance != konquest_pdata->hero_instance) {
                hero = 0;
            }
        } else {
            hero = 0;
        }

        if (hero != 0 && camera_obj != 0) {
            camera_obj->pos.x = hero->pos.value.x;
            camera_obj->pos.y = hero->pos.value.y;
            camera_obj->pos.z = hero->pos.value.z;
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
            update_tile_grid();
        }

        xfer_camera(p_konquest_camera_proc, 1);
        _mkproc_sleep_ticks = 2.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    objective_table = konquest_pdata->objective.table;
    if (objective_table != 0) {
        objective_index = (int)get_row_count_for_table_by_pointer(
                              konquest_pdata->script_owner, objective_table) -
                          1;

        while (objective_index >= 0) {
            objective_row = &objective_table[objective_index];
            if (objective_row->requirement_type == -1 ||
                get_konq_profile_value(
                    objective_row->requirement_type,
                    objective_row->requirement_index) != 0) {
                break;
            }
            objective_index--;
        }

        if (objective_index < 0) {
            objective_index = 0;
        }
        p1_profile_konquest->fields.objective_index = objective_index;
    }

    mode_of_play = 7;

    head_tracking = 0;
    if (_create_mkproc_generic_tinystack(
            0x6005, 0x1F, p_head_tracking, sizeof(*head_tracking),
            (void**)&head_tracking) != 0 &&
        head_tracking != 0) {
        head_tracking->angle_y = 0.0f;
        konquest_pdata->npc_interaction_state = 0;
    }

    _mkproc_sleep_ticks = 6.0f;
    g_game_info.flag_bits.pad_bit1 = 1;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    while (is_controller_removed() != 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    turn_camera_off();

    if (active_cmdscript->unk28 != 0) {
        cmdscript_setup_execution(
            konquest_pdata->script_owner,
            (unsigned int)active_cmdscript->unk28);
        cmdscript_execute(konquest_pdata->script_owner);
    }

    if (p1_profile_konquest->fields.portal_entry_pending != 0) {
        konquest_hero_portal_in();
    }

    if (konquest_pdata->region_index == 0) {
        update_tile_grid();
        turn_controllers_on();
        konquest_pdata->flag_bits.region_loaded = 1;
        cmdscript_setup_execution(konquest_pdata->script_owner, 0x162);
        cmdscript_execute(konquest_pdata->script_owner);
        ((KonquestProcJumpVtable*)aproc->vtbl)
            ->jump_sleep(p_idle, aproc, 0.0f);
        return 0.0f;
    }

    konquest_show_hud();

    if (konquest_editor_mode_on == 0) {
        konquest_pdata->tile_column =
            konquest_pdata->tile_load_state % konquest_pdata->tile_width;
        konquest_pdata->tile_row =
            konquest_pdata->tile_load_state / konquest_pdata->tile_width + 1;

        grid_label = (StringObj*)konquest_pdata->hud_labels[5].object;
        if (grid_label != 0) {
            if (grid_label->instance !=
                konquest_pdata->hud_labels[5].instance) {
                grid_label = 0;
            }
        } else {
            grid_label = 0;
        }

        sprintf(
            grid_text, "%c - %d", konquest_pdata->tile_column + 'A',
            konquest_pdata->tile_row);
        if (grid_label != 0) {
            update_string_obj(grid_label, 0xE, grid_text);
        } else {
            grid_label = string_center_xy(
                0x8300, 0xE, grid_text, 0x52, screen_height - 0x9B, 0x44);
            konquest_pdata->hud_labels[5].object = (MkHdr*)grid_label;
            konquest_pdata->hud_labels[5].instance = grid_label->instance;
        }
    }

    trigger_update(1);

    if (konquest_editor_mode_on == 0) {
        konquest_pdata->ambient_sound_main =
            snd_req(konquest_pdata->region_table->ambient_sound);

        _create_mkproc_generic_tinystack(
            0x8226, 0x1F, p_cross_fade_ambient_sounds,
            sizeof(*ambient_fade), (void**)&ambient_fade);
        if (ambient_fade != 0) {
            ambient_fade->step_a = 0.0f;
            ambient_fade->step_b = 0.0f;
            ambient_fade->volume_a = 0.0f;
            ambient_fade->volume_b = 0.0f;
            ambient_fade->delay =
                (float)((unsigned short)randu0(0x384) + 0x12C);
        }
    }

    konquest_pdata->time_rate_changed = 1;
    if (display_off != 0) {
        turn_camera_on();
        fade_from_black(5, 0);
    }

    turn_controllers_on();
    konquest_pdata->flag_bits.region_loaded = 1;
    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_konquest_loop, aproc, 0.0f);
    return 0.0f;
}

/*
 * Near match: map assets, collision lists, sentinel tile, interior data,
 * table bounds, timed-event allocation, and event iteration agree. The
 * remaining 98.15% residue is GPR coloring, one loop-zero materialization,
 * and pooled string/float relocation labels (888 versus 884 bytes).
 */
#pragma optimize_for_size on
static void load_and_init_konquest_map_specific_data(int region) {
    MkHdr* process_data;
    unsigned int shadow_art;
    unsigned int event_count;
    unsigned int index;

    load_ssf(konquest_region_data[region].fight_files);
    if (konquest_editor_mode_on == 0) {
        initialize_npc_data();
    }
    load_binary_data(
        &sec_konquestdata, &chest_data,
        (void*)konquest_prize_description_block,
        &konquest_data_loaded, 0x49);
    load_konquest_tiles();
    konquest_nav_init();

    shadow_art = get_artid_of_named_item_in_slot(
        0x60029, "SHADOW_COL", 0);
    if (shadow_art != 0) {
        int tile_index;
        int tile_offset;

        tile_index = 0;
        tile_offset = 0;
        while (tile_index <
               konquest_pdata->tile_width * konquest_pdata->tile_height) {
            KonquestTileRecord* tile;
            CollisionObjList* collision_list;

            tile = (KonquestTileRecord*)((char*)konquest_pdata->tile_structs +
                                         tile_offset);
            collision_list = get_collision_obj_list();
            if (collision_list != 0) {
                tile->collision_object_list = collision_list;
            }
            tile_index++;
            tile_offset += sizeof(*tile);
        }
        generate_shadow_collision_objects(0x60029, shadow_art);
        konquest_pdata->base_shadow_strength = 1.0f;
    }

    if (konquest_editor_mode_on == 0) {
        int tile_count;
        KonquestTileRecord* sentinel;

        tile_count =
            konquest_pdata->tile_width * konquest_pdata->tile_height;
        sentinel = &konquest_pdata->tile_structs[tile_count];
        sentinel->scene = 0;
        sentinel->index = tile_count;
        sentinel->position.x = 1030.0f;
        sentinel->position.y = 0.0f;
        sentinel->position.z = 1030.0f;
        sentinel->objects = 0;
        sentinel->state = 0x1000;
        konquest_pdata->fallback_tile_position.x = sentinel->position.x;
        konquest_pdata->fallback_tile_position.y = sentinel->position.y;
        konquest_pdata->fallback_tile_position.z = sentinel->position.z;
    }

    if (konquest_editor_mode_on == 0) {
        initialize_konquest_interior();
        if (konquest_pdata->region_table->interior_art_name != 0) {
            unload_section_slot(0xA002F);
            load_ssf(
                konquest_region_data[konquest_pdata->region_index]
                    .fight_files);
            add_art_section_by_name_async(
                0xA002F,
                konquest_pdata->region_table->interior_art_name);
        }
        konquest_make_monk_an_npc();
    }
    if (konquest_pdata->region_table->supplemental_art_name != 0) {
        load_art_section_by_name(
            0x60025,
            konquest_pdata->region_table->supplemental_art_name);
    }

    konquest_pdata->objective_table =
        get_data_table_by_name("objective_table");
    if (konquest_editor_mode_on != 0) {
        return;
    }
    konquest_pdata->pui_begin = get_table_index_by_pointer(
        konquest_pdata->script_owner,
        get_data_table_by_name("pui_header"));
    konquest_pdata->pui_end = get_table_index_by_pointer(
        konquest_pdata->script_owner,
        get_data_table_by_name("pui_footer"));
    if (konquest_pdata->region_table->pui_header == 0) {
        return;
    }
    konquest_pdata->dynamic_pui_begin = get_table_index_by_pointer(
        konquest_pdata->script_owner,
        konquest_pdata->region_table->pui_header);
    konquest_pdata->dynamic_pui_end = get_table_index_by_pointer(
        konquest_pdata->script_owner,
        konquest_pdata->region_table->pui_footer);

    {
        KonquestTime first_event_time = {0};

        event_count = get_row_count_for_table_by_pointer(
            konquest_pdata->script_owner,
            konquest_pdata->region_table->pui_event_table);
        if (event_count != 0) {
            MkProc* proc;
            KonquestPuiEventRow* event;
            int allocation_size;

            proc = (MkProc*)_create_mkproc_generic_nostack(
                0xA019, 0x1F, p_pui_timed_event_manager,
                sizeof(*process_data), (void**)&process_data);
            if (proc != 0) {
                set_process_as_scriptable(proc);
            }
            allocation_size = event_count * sizeof(*g_pui_events);
            event = (KonquestPuiEventRow*)
                konquest_pdata->region_table->pui_event_table;
            g_pui_events = (KonquestTime*)get_mem(allocation_size);
            memset(g_pui_events, 0, allocation_size);
            for (index = 0; index < event_count; index++) {
                calc_next_occurrence_of_event(
                    &g_pui_events[index], &event[index].time,
                    &first_event_time);
            }
        }
    }
}
#pragma optimize_for_size reset
/*
 * Soft ceiling: insert_collision_on_proper_tile_list ~98.5% -- center-to-tile
 * conversion, bounds CFG, lookup, and insertion match. Residue is an r4/r5
 * rotation in tile-index calculation and local float-constant relocations.
 */
void insert_collision_on_proper_tile_list(CollisionObj* object) {
    KonquestTileRecord* tile;
    Vec center;
    int tile_index;

    get_shape_center_for_collision_obstacle(object, &center);
    if (center.x >= 1000.0f) {
        tile_index =
            konquest_pdata->tile_width * konquest_pdata->tile_height;
    } else {
        int row;
        int column;

        row = (int)((center.z + konquest_pdata->tile_origin_z) / 60.0f);
        if (row < 0) {
            tile_index = -1;
        } else if (row >= konquest_pdata->tile_height) {
            tile_index = -1;
        } else {
            column =
                (int)((center.x + konquest_pdata->tile_origin_x) / 60.0f);
            if (column < 0) {
                tile_index = -1;
            } else if (column >= konquest_pdata->tile_width) {
                tile_index = -1;
            } else {
                tile_index = column + row * konquest_pdata->tile_width;
            }
        }
    }
    if (tile_index >= 0) {
        tile = get_tile_record_by_index(tile_index);
        if (tile != 0 && tile->collision_object_list != 0) {
            insert_on_collision_obj_list(
                &object->hdr, tile->collision_object_list);
        }
    }
}

/*
 * Soft ceiling: get_tile_sobj_by_id ~79.3% -- the lookup algorithms and
 * frame are complete; residue is latch-temporary GPR allocation, one join,
 * and placement of the final special-case return move.
 */
MkSobj* get_tile_sobj_by_id(int id) {
    MkObj* model;
    MkHdr* candidate;

    model = 0;
    if (konquest_pdata != 0) {
        KONQUEST_RESOLVE_LATCH(konquest_pdata->tile_model, candidate);
        model = (MkObj*)candidate;
    }
    if (model == 0) {
        return 0;
    }

    if (id == 0x4F) {
        MkPtr* link;
        MkSobj* sobj;
        unsigned int count;

        count = 0;
        link = first_mkptr(&model->sobj_list);
        while (link != 0) {
            sobj = (MkSobj*)link->hdr;
            if ((sobj->id_flags & 0xFFF) == (unsigned int)id) {
                count++;
                if (count == 3) {
                    return sobj;
                }
            }
            link = next_mkptr(link);
        }
        return 0;
    }
    return obj_find_sobj_by_id(model, id);
}

/*
 * Near match: the typed ownership latch and return value agree with retail.
 * MWCC folds the two null returns and keeps the object in r3; retail keeps it
 * in r5 and joins the two inner failure blocks before moving it to r3.
 */
void* get_konquest_tile_objects_obj(void) {
    KonquestPdata* pdata = konquest_pdata;
    MkHdr* object = 0;

    if (pdata != 0) {
        object = pdata->tile_objects;
        if (object != 0) {
            if (object->instance == pdata->tile_objects_instance) {
                /* Valid tile-object latch. */
            } else {
                object = 0;
            }
        } else {
            object = 0;
        }
        return object;
    }
    return 0;
}

void konquest_state_init(void) {
    memset(&konquest_save_data, 0, sizeof(konquest_save_data));
}

/*
 * Near match at the exact retail size: mode-stack initialization, player
 * setup, all three typed light definitions, inventory clear, collision
 * callback, and jump-sleep agree. Residue is aggregate-copy scheduling,
 * register selection, and pooled constant relocation labels.
 */
#pragma optimize_for_size on
static float p_init_konquest_mode(void) {
    PlyrInfo* player;
    int index;

    RwResourcesSetArenaSize(0x48000);
    gc_setup_feedback_buffer_for_konquest();
    zero_pdata_payload(sizeof(*konquest_pdata), (MkHdr*)konquest_pdata);
    nis_participants = 0;
    konquest_pdata->tile_width = 5;
    konquest_pdata->tile_height = 5;
    konquest_pdata->time_of_day = 5.0f;
    konquest_pdata->time_passing = 1;
    konquest_pdata->hero_state = 0;
    konquest_pdata->game_mode_index = -1;
    index = konquest_pdata->game_mode_index;
    if (index < 0 ||
        (index < 0 ? 0 : konquest_pdata->game_modes[index]) != 0) {
        if (index < 3) {
            konquest_pdata->game_mode_index++;
            konquest_pdata
                ->game_modes[konquest_pdata->game_mode_index] = 0;
        }
    }
    konquest_pdata->region_load_proc = aproc;
    konquest_pdata->sky_color_multiplier = 1.0f;
    konquest_data_loaded = 0;
    in_exit_meditation = 0;
    set_mode_of_play(7);
    push_game_state(0x13);
    turn_controllers_off();
    player = &g_game_info.plyr0;
    konquest_pdata->input_port = player->pad_index;
    menu_player = 0;
    set_player_state(player, 2);
    set_player_state(&g_game_info.plyr1, 0);
    {
        KonquestDirectionalLightDef directional_light;
        KonquestAmbientLightDef ambient_light;
        KonquestAmbientLightDef sky_ambient_light;

        directional_light = konquest_directional_light_default;
        sky_ambient_light = konquest_sky_ambient_light_default;
        ambient_light = konquest_ambient_light_default;

        if (directional_light.type == 3) {
            memcpy(
                &konquest_pdata->directional_light, &directional_light,
                sizeof(directional_light));
        }
        if (ambient_light.type == 1) {
            memcpy(
                &konquest_pdata->ambient_light, &ambient_light,
                sizeof(ambient_light));
        }
        if (sky_ambient_light.type == 1) {
            memcpy(
                &konquest_pdata->sky_ambient_light, &sky_ambient_light,
                sizeof(sky_ambient_light));
        }
    }
    konquest_pdata->animation_event_index = 0;
    g_game_info.field_34 = 0.0f;
    original_game_speed = game_speed;
    for (index = 0; index < sizeof(konquest_pdata->pui_inventory_bits);
         index++) {
        konquest_pdata->pui_inventory_bits[index] = 0;
    }
    set_global_collision_callback(npc_collision_callback);
    ((KonquestProcJumpVtable*)aproc->vtbl)
        ->jump_sleep(p_setup_konquest_map, aproc, 0.0f);
    return 0.0f;
}
#pragma optimize_for_size reset

int get_konquest_game_mode(void) {
    return konquest_current_game_mode();
}

/*
 * Near match: the inclusive stack scan, bounds, and early return agree with
 * retail. Typed array iteration strength-reduces to pointer induction; retail
 * retains a byte-offset induction variable and lwzx, accounting for 8 bytes.
 */
int is_game_mode_in_stack(int game_mode) {
    return konquest_game_mode_in_stack(game_mode);
}

/*
 * Soft ceiling: p_konquest_mode ~93.5% -- stack layout, heap parameters,
 * calls, branches, and return paths match. Residue is stmw/lmw versus paired
 * saves/restores plus pooled-string offsets and the local -1.0f relocation.
 */
float p_konquest_mode(void) {
    MwMemHeapParams defaults;
    MwMemHeapCreateParams create;
    MwMemFixedParams fixed;

    set_section_memory_scheme(1);
    mwMemHeapGetParams(wave_heap, &defaults);

    fixed.field_0x00 = 2;
    fixed.sizeThreshold = 8;
    create.parentHeap = wave_heap;
    create.arenaSize = 1;
    create.strategyType = MW_MEM_STRATEGY_FIXED;
    create.extraSizeShift = 0;
    create.fixedInitParams = &fixed;
    create.field_0x08 = 4;
    fixed.flags = 4;
    create.name = "Konquest Obj fixed block heap";
    fixed.blockSize = 0x54;
    fixed.blockCount = 0x578;
    konquest_objects_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "Konquest Sub Obj fixed block heap";
    fixed.blockSize = 0x54;
    fixed.blockCount = 0x578;
    konquest_subobjects_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    if (_create_mkproc_generic_bigstack(
            0x2001, 0x23, p_init_konquest_mode, 0x454,
            (void**)&konquest_pdata) != 0) {
        return -1.0f;
    }
    gamelogic_jump(0, p_atm_loop);
    return -1.0f;
}
