#include "runtime/mk_struct.h"
#include "runtime/asset.h"
#include "runtime/image.h"
#include "runtime/fonts.h"
#include "runtime/utils.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_particle.h"
#include "runtime/cam.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_mem.h"
#include "runtime/section.h"
#include "runtime/shadow.h"
#include "runtime/sound_tracker.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "game/game_info.h"
#include "game/konquest.h"
#include "game/konquest_items.h"
#include "game/settings.h"
#include "game/konquest_save.h"
#include "platform/io.h"
#include "platform/gcutils.h"
#include "msl/msl_types.h"

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
        char pad00[0x5C];
        unsigned char hero_age; /* +0x5C */
        char pad5D[7];
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

typedef struct KonquestEnumerationEntry {
    char pad00[8];
    int enumeration; /* +0x08 */
    char pad0C[0x10];
} KonquestEnumerationEntry; /* 0x1C */

typedef struct KonquestRegionTable {
    void* region_asset;
    const char* interior_art_name; /* +0x04 */
    char pad08[0x14];
    const char* effect_bank_name; /* +0x1C */
    char pad20[8];
    unsigned int setup_function; /* +0x28 */
    char pad2C[8];
    KonquestEnumerationEntry* enumerations; /* +0x34 */
    char pad38[0x14];
    void* ambient_light_table; /* +0x4C */
    void* sky_ambient_light_table; /* +0x50 */
    char pad54[0x0C];
    int ambient_sound_a; /* +0x60 */
    int ambient_sound_b; /* +0x64 */
    int ambient_sound; /* +0x68 */
} KonquestRegionTable;

typedef struct KonquestTransitionPdata {
    MkHdr hdr;
    void* object; /* +0x08 */
    int state;    /* +0x0C */
    int flags;    /* +0x10 */
} KonquestTransitionPdata;

typedef union KonquestTransitionPdataRef {
    MkHdr* hdr;
    KonquestTransitionPdata* transition;
} KonquestTransitionPdataRef;

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

typedef struct KonquestCameraScriptPdata {
    MkHdr hdr;
    void* owner;  /* +0x08 */
    void* script; /* +0x0C */
} KonquestCameraScriptPdata;

typedef union KonquestCameraScriptPdataRef {
    MkHdr* hdr;
    KonquestCameraScriptPdata* camera;
} KonquestCameraScriptPdataRef;

typedef struct KonquestDialogPdata {
    MkHdr hdr;
    char pad08[0x20];
    int active; /* +0x28 */
    char text[0x518]; /* +0x2C */
    unsigned int print_speed; /* +0x544 */
    char pad548[4];
    unsigned int print_ticks; /* +0x54C */
} KonquestDialogPdata;

typedef union KonquestDialogPdataRef {
    MkHdr* hdr;
    KonquestDialogPdata* dialog;
} KonquestDialogPdataRef;

typedef struct KonquestNpcSpatial {
    char pad00[0x4C];
    Vec position; /* +0x4C */
} KonquestNpcSpatial;

typedef union KonquestNpc {
    unsigned char raw[0x20];
    struct {
        char pad00[0x0C];
        KonquestNpcSpatial* spatial; /* +0x0C */
        char pad10[4];
        MkSobj* dialog_owner; /* +0x14 */
        char pad18[4];
        unsigned char flags; /* +0x1C */
        unsigned char state_flags; /* +0x1D */
        char pad1E[2];
    } fields;
} KonquestNpc;

typedef struct KonquestNpcSceneState {
    char pad00[0x0C];
    MkObj* object;
    char pad10[0x10];
    MkProc* process;
} KonquestNpcSceneState;

typedef struct KonquestNpcRuntime {
    char pad00[0x0C];
    int id;
    char pad10[4];
    KonquestNpcSceneState* scene_state;
    char pad18[5];
    unsigned char state_flags;
    char pad1E[0x3A];
    int type;
    char pad5C[0x1C0];
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
    char pad10[0x28];
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
    void* fight_files;
    MkFileEntry* art_files; /* +0x04 */
    char pad08[0x0C];
} KonquestRegionAsset; /* 0x14 */

typedef struct KonquestNpcCameraTarget {
    char pad00[0x0C];
    MkObj* focus_object; /* +0x0C */
    char pad10[0x10];
    int ready; /* +0x20 */
} KonquestNpcCameraTarget;

typedef struct KonquestGrounding {
    MkHdr hdr;
    char pad08[0x0C];
    KonquestNpcCameraTarget* camera_target; /* +0x14 */
    char pad18[5];
    unsigned char flags; /* +0x1D */
} KonquestGrounding;

typedef struct KonquestObjectLatch {
    MkHdr* object;
    unsigned int instance;
} KonquestObjectLatch;

typedef struct KonquestDialogOwner {
    char pad00[0x1C];
    unsigned char flags; /* +0x1C */
} KonquestDialogOwner;

typedef struct KonquestDialogArt {
    char pad00[0x24];
    KonquestDialogOwner* owner; /* +0x24 */
} KonquestDialogArt;

typedef struct KonquestDialogRuntimePdata {
    MkHdr hdr;
    KonquestObjectLatch art[3]; /* +0x08 */
    KonquestDialogArt* dialog_art; /* +0x20 */
} KonquestDialogRuntimePdata;

typedef struct KonquestAwardArtPdata {
    char pad00[0x0C];
    KonquestObjectLatch art[7]; /* +0x0C */
} KonquestAwardArtPdata;

typedef struct KonquestPdata {
    char pad00[8];
    MkProc* region_load_proc;     /* +0x08 */
    unsigned char flags;
    char pad0d[3];
    int game_modes[4];
    int game_mode_index;
    ScriptSlot* script_owner;    /* +0x24 */
    KonquestRegionTable* region_table; /* +0x28 */
    int interior_active;         /* +0x2C */
    char pad30[0x18];
    int hud_visible;             /* +0x48 */
    KonquestObjectLatch hud_objects[7]; /* +0x4C .. +0x83 */
    char pad84[8];
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
    char pad114[8];
    int hero_state;              /* +0x11c */
    char pad120[8];
    int current_portal_uid;      /* +0x128 */
    char pad12c[0x0c];
    unsigned char current_time[0x18]; /* +0x138 */
    int time_passing;            /* +0x150 */
    int time_rate_changed;        /* +0x154 */
    int region_index;              /* +0x158 */
    KonquestTileRecord* tile_structs; /* +0x15C */
    char pad160[4];
    MkHdr* tile_objects;           /* +0x164 */
    unsigned int tile_objects_instance; /* +0x168 */
    KonquestObjectLatch tile_model; /* +0x16C */
    char pad174[8];
    int tile_width;              /* +0x17c */
    int tile_height;             /* +0x180 */
    float tile_origin_x;         /* +0x184 */
    float tile_origin_z;         /* +0x188 */
    char pad18c[0x6C];
    KonquestObjectLatch movement_npc; /* +0x1F8 */
    char pad200[0xB8];
    KonquestObjectLatch objective_beam; /* +0x2B8 */
    int objective_visible;       /* +0x2C0 */
    char pad2C4[0x28];
    void* objective_table;       /* +0x2EC */
    char pad2F0[4];
    MkPtr* pui_list;             /* +0x2F4 */
    char pad2F8[4];
    void* reference_pui;         /* +0x2fc */
    struct KonquestTriggerStruct* active_trigger; /* +0x300 */
    unsigned int active_trigger_instance; /* +0x304 */
    char pad308[0xDC];
    unsigned int pui_begin;      /* +0x3e4 - first table index */
    unsigned int pui_end;        /* +0x3e8 - one past final table index */
    unsigned int dynamic_pui_begin; /* +0x3EC */
    unsigned int dynamic_pui_end;   /* +0x3F0 */
    char pad3F4[0x2C];
    int weather_type;            /* +0x420 */
    int weather_param_b;         /* +0x424 */
    int weather_param_a;         /* +0x428 */
    char pad42C[4];
    MkPtr* attached_sounds;      /* +0x430 */
    MslSoundHandle ambient_sound_a; /* +0x434 */
    MslSoundHandle ambient_sound_b; /* +0x438 */
    MslSoundHandle ambient_sound_main; /* +0x43C */
} KonquestPdata;

typedef struct KonquestSwitchState {
    int input_port;
} KonquestSwitchState;

typedef struct KonquestSwitchPdata {
    MkHdr hdr;
    KonquestSwitchState* state;
} KonquestSwitchPdata;

typedef struct MonkStateData {
    char pad00[0x24];
    int transition_order; /* +0x24 */
    AniData* animation; /* +0x28 */
    char pad2C[4];
    int transition; /* +0x30 */
    MkProcEntryFn control_proc; /* +0x34 */
} MonkStateData; /* 0x38 */

typedef struct KonquestProcSleepVtable {
    char pad00[0x18];
    int (*sleep)(void); /* +0x18 */
} KonquestProcSleepVtable;

typedef struct KonquestProcJumpVtable {
    char pad00[0x24];
    int (*jump_sleep)(MkProcEntryFn entry, MkProc* proc, float ticks);
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
    char pad08[0x10];
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
    char pad00[0x18];
    MkPtr* objects; /* +0x18 */
    char pad1C[0x134];
}; /* 0x150 */

typedef struct KonquestUidObject {
    MkHdr hdr;
    int uid; /* +0x08 */
} KonquestUidObject;

typedef union KonquestUidObjectRef {
    MkHdr* hdr;
    KonquestUidObject* object;
} KonquestUidObjectRef;

typedef union KonquestArtIdRef {
    unsigned int id;
    char* name;
} KonquestArtIdRef;

typedef struct KonquestChildDefinition {
    char pad00[0x0C];
    int enumeration_index; /* +0x0C */
} KonquestChildDefinition;

typedef struct KonquestPuiDefinition {
    int type; /* +0x00 */
    char pad04[0x0C];
    int value; /* +0x10 */
    char pad14[0x1C];
    int spawn_script_index; /* +0x30 */
    int pickup_script_index; /* +0x34 */
} KonquestPuiDefinition;

typedef struct KonquestChildObject {
    MkHdr hdr;
    char pad08[8];
    KonquestChildDefinition* definition; /* +0x10 */
} KonquestChildObject;

typedef struct KonquestCollisionVolume {
    char pad00[0x14];
    int active; /* +0x14 */
    MkPtr* objects; /* +0x18 */
} KonquestCollisionVolume;

typedef struct KonquestCollisionOwner {
    MkHdr hdr;
    char pad08[8];
    KonquestCollisionVolume* collision_volume; /* +0x10 */
} KonquestCollisionOwner;

typedef struct KonquestRemoveCollisionPdata {
    MkHdr hdr;
    KonquestCollisionOwner* owner; /* +0x08 */
    unsigned int owner_instance; /* +0x0C */
} KonquestRemoveCollisionPdata;

typedef struct KonquestTriggerDefinition {
    int type;
    char pad04[0x18];
    int state; /* +0x1C */
} KonquestTriggerDefinition;

typedef struct KonquestTriggerStruct {
    MkHdr hdr;
    KonquestTriggerDefinition* owned_data; /* +0x08 */
    char pad0C[0x10];
    MkProc* script_proc; /* +0x1C */
    unsigned int script_proc_instance; /* +0x20 */
    unsigned char flags; /* +0x24 */
} KonquestTriggerStruct;

typedef struct KonquestTriggerScriptPdata {
    MkHdr hdr;
    KonquestTriggerStruct* trigger;
    unsigned int trigger_instance;
} KonquestTriggerScriptPdata;

typedef struct KonquestAmbientFadePdata {
    MkHdr hdr;
    float volume_a;
    float volume_b;
    float target_a;
    float target_b;
    float delay;
} KonquestAmbientFadePdata;

typedef struct KonquestNisLoadPdata {
    MkHdr hdr;
    int slot;
    char* animation_name;
    char* art_name;
} KonquestNisLoadPdata;

typedef struct KonquestLightAdjustPdata {
    MkHdr hdr;
    int light_index;
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

typedef struct KonquestChestInteraction {
    char pad00[0x1C];
    int closed;
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
    int initiated;
    int separate_characters;
    float separation;
    int interaction_type;
    char pad1C[0x10];
} KonquestInteractionPdata;

typedef struct KonquestKoinAwardPdata {
    MkHdr hdr;
    int amount;
    unsigned int icon_offset;
} KonquestKoinAwardPdata;

typedef struct KonquestPuiDelayView {
    MkHdr hdr;
    KonquestChestOwner* owner;
    unsigned int owner_instance;
    char pad10[0x0C];
    int behavior;
    MkObj* object;
    unsigned int object_instance;
    char pad28[0x0C];
    float lifetime;
} KonquestPuiRuntime;

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

typedef struct KonquestTriggerRequirement {
    int type;
    int field_04;
    Vec position;
    char pad14[0x10];
    KonquestTriggerOrientation* orientation;
} KonquestTriggerRequirement;

typedef struct KonquestTriggerRequirementOwner {
    MkHdr hdr;
    KonquestTriggerRequirement* requirement;
} KonquestTriggerRequirementOwner;

typedef struct KonquestDialogKillPdata {
    MkHdr hdr;
    int attempts;
} KonquestDialogKillPdata;

typedef struct KonquestDynamicPuiPdata {
    MkHdr hdr;
    KonquestPuiDefinition* item;
    int source_type;
    Vec position;
    unsigned int source_instance;
    int critical;
} KonquestDynamicPuiPdata;

typedef struct KonquestDoorPdata {
    MkHdr hdr;
    int open_ticks;
    int remain_open;
    void* door;
    int transition_ticks;
} KonquestDoorPdata;

typedef union KonquestRemoveCollisionPdataRef {
    void* raw;
    KonquestRemoveCollisionPdata* remove_collision;
} KonquestRemoveCollisionPdataRef;

typedef struct KonquestAttachedSound {
    MkHdr hdr;
    char pad08[0x24];
    int enabled; /* +0x2C */
    int uid; /* +0x30 */
} KonquestAttachedSound;

typedef union KonquestAttachedSoundRef {
    MkHdr* hdr;
    KonquestAttachedSound* sound;
} KonquestAttachedSoundRef;

typedef struct KonquestTile {
    char pad00[4];
    int visible; /* +0x04 */
    char pad08[0x0C];
    void* scene; /* +0x14 */
    MkPtr* objects; /* +0x18 */
} KonquestTile;

static RpAtomic* konquest_atomic_from_frame_link(RwLLLink* link) {
    return (RpAtomic*)((char*)link -
                       KONQUEST_OFFSETOF(RpAtomic, frameLink));
}

static MkSobj* konquest_sobj_from_atomic(RpAtomic* atomic) {
    MksobjPluginData* plugin;

    plugin = (MksobjPluginData*)((char*)atomic + MksobjLocalOffset);
    return plugin->sobj;
}

extern KonquestPdata* konquest_pdata;
extern KonquestProfile* p1_profile_konquest;
extern unsigned char konquest_save_data[];
extern int konquest_force_region_reload;
extern KonquestNpc* g_active_npc;
extern int konquest_editor_mode_on;
extern int g_fade_hud_in;
extern int mode_of_play;
extern int screen_width;
extern int screen_height;
extern int konquest_save_on_exit;
extern unsigned char monk_ground_colls[];
extern unsigned char monk_laying_on_ground_colls[];
extern KonquestRegionAsset konquest_region_data[9];
extern const MkFileEntry konquest_common_file_table[];
extern KonquestSwitchPdata* switch_pdata;
extern MonkStateData monk_state_data[];
extern CmdScript* active_cmdscript;
extern KonquestCommonProfile* p1_profile_common;
extern KonquestCharacterBits default_char_bits;
extern KonquestCharacterBits default_alt_char_bits;
extern int konq_nis_anims[0x14];
extern MkPtr* nis_participants;
extern int konquest_human_bones[17];
extern MkFlippedBoneMap flipped_konquest_human_bones;
extern void* g_pui_events;
extern int f_writing_konquest_profile;
extern int in_exit_meditation;
extern MslSoundHandle meditate_stream;
extern int b_game_timer_off;
static unsigned char pdata_monk[0x74C];
extern float original_game_speed;
extern float game_speed;
extern float ticks_per_game_day;
extern float ticks_per_hour;
extern float game_hours_per_tick;
extern float sun_angle_change_per_tick;
extern int use_feedback_effect;
extern int feedback_blendrate;
static Vec old_hero_position = {0.0f, 500.0f, 0.0f};

void* memcpy(void* dst, const void* src, unsigned long size);
void* memset(void* dst, int value, unsigned long size);
int sprintf(char* destination, const char* format, ...);
void* get_screen_pdata(void);
static float p_konquest_fade_screen(void);
float snd_get_game_vol(void);
void snd_set_game_vol(float volume);
extern float game_volume;
void run_interaction_camera_script(void* owner, void* script);
void set_konq_profile_value(int category, int index, int value);
int get_konq_profile_value(int category, int index);
void snd_req_delay(int sound, int delay);
void set_game_speed(float speed);
void update_visible_tiles(void);
void cleanup_npc_manager(void);
void npc_shadow_teardown(void);
void destroy_konquest_shadow_collision_lists(void);
void cleanup_mission_state(void);
int is_this_the_monk_npc(KonquestNpc* npc);
void npc_set_wait_ticks(float ticks);
void npc_suspend_cmdscript(void);
float p_konquest_interaction(void);
float p_transition_to_fight(void);
float p_load_hero_art_section(void);
int refresh_rate(void);
void obj_create_sobjs(MkObj* object);
void insert_ground_me_mkobj(MkObj* object);
void build_bones_tbl(MkObj* object, void* bones);
void obj_apply_to_sobj_with_id(
    MkObj* object, int id, void (*callback)(void*));
int load_effect_bank(const char* name);
void initialize_tile_patch_sobj(char* object);
void generate_door_paths(void);
void profile_region_change(void);
int mslSoundIsValid(MslSoundHandle handle);
void set_snd_vol(MslSoundHandle handle, int sound, float volume);
void* get_mkobj_frame(int type, void* frame);
MkPfx* find_pfx_by_handle(unsigned int handle);
void* get_script_function_by_name(ScriptSlot* owner, const char* name);
void del_string_obj_by_id(int id);
void sobj_set_priority(void* object, int priority);
void hide_atomic(void* atomic);
void* _create_mkproc_generic_tinystack(int pid, int priority, void* entry,
                                       int pdata_size, void** pdata);
void* _create_mkproc_generic_bigstack(int pid, int priority, void* entry,
                                      int pdata_size, void** pdata);
void* _create_mkproc_generic_nostack(int pid, int priority, void* entry,
                                     int pdata_size, void** pdata);
void zero_pdata_payload(int size, void* pdata);
void p_konquest_kill_dialog_procs(void);
void p_show_fight_message(void);
void run_camera_script(void* owner, void* script, int flags);
void destroy_mkprocs_pid(int pid);
void kill_lip_sync_procs(void);
float duration_of_lip_sync(const LipSyncKeyframe* keyframes);
int get_mode_of_play(void);
double ceil(double value);
int create_dialog_proc(MkSobj* owner, const char* text, int print_speed);
int spawn_dynamic_pui_at_pos(
    KonquestPuiDefinition* item, int source_type, const Vec* position,
    void* source, int critical);
int is_it_safe_to_spawn_pui(KonquestPuiDefinition* item);
void set_process_as_scriptable(MkProc* proc);

static void konquest_fade_screen(
    int ticks, int white, int fade_sound, int to_black) {
    KonquestFadePdata* pdata;
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

    pdata = 0;
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
    pdata->alpha = to_black != 0 ? 0 : 0xFF;
    proc->flags |= 8;

    if (white == 1) {
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
    object->flags |= 8;
    object->scale_x = 50.0f;
    object->scale_y = 40.0f;

    volume_step = (float)ticks / 255.0f;
    if (to_black != 0) {
        alpha = pdata->alpha + (unsigned char)ticks;
        pdata->alpha = alpha > 0xFF ? 0xFF : alpha;
        if (pdata->object != 0 &&
            pdata->object->instance == pdata->object_instance) {
            pfx_2d_obj_set_alpha(pdata->object, pdata->alpha);
            if (fade_sound != 0) {
                volume = snd_get_game_vol() - volume_step;
                snd_set_game_vol(volume > 0.0f ? volume : 0.0f);
            }
        }
    } else {
        alpha = pdata->alpha - (unsigned char)ticks;
        pdata->alpha = alpha < 0 ? 0 : alpha;
        if (pdata->object != 0 &&
            pdata->object->instance == pdata->object_instance) {
            pfx_2d_obj_set_alpha(pdata->object, pdata->alpha);
            if (fade_sound != 0) {
                volume = snd_get_game_vol() + volume_step;
                snd_set_game_vol(
                    volume < game_volume ? volume : game_volume);
            }
        }
    }
}

typedef struct KonquestFadeDestroyVtable {
    void* reserved[4];
    void (*destroy)(ScreenObj* object);
} KonquestFadeDestroyVtable;

static ScreenObj* resolve_konquest_fade_object(
    KonquestFadePdata* pdata) {
    ScreenObj* object;

    object = pdata->object;
    if (object != 0 &&
        object->instance == pdata->object_instance) {
        return object;
    }
    return 0;
}

static float p_konquest_fade_screen(void) {
    KonquestFadePdata* pdata;
    ScreenObj* object;
    float volume_step;
    float volume;
    int alpha;
    int complete;

    if ((g_game_info.field_04 & 0x80) == 0 &&
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

    volume_step = (float)pdata->ticks / 255.0f;
    if (pdata->to_black != 0) {
        alpha = pdata->alpha + (unsigned char)pdata->ticks;
        pdata->alpha = alpha > 0xFF ? 0xFF : alpha;

        object = resolve_konquest_fade_object(pdata);
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, pdata->alpha);
            if (pdata->fade_sound != 0) {
                volume = snd_get_game_vol() - volume_step;
                snd_set_game_vol(volume > 0.0f ? volume : 0.0f);
            }
            complete = pdata->alpha == 0xFF;
        } else {
            complete = 1;
        }
    } else {
        alpha = pdata->alpha - (unsigned char)pdata->ticks;
        pdata->alpha = alpha < 0 ? 0 : alpha;

        object = resolve_konquest_fade_object(pdata);
        if (object != 0) {
            pfx_2d_obj_set_alpha(object, pdata->alpha);
            if (pdata->fade_sound != 0) {
                volume = snd_get_game_vol() + volume_step;
                snd_set_game_vol(
                    volume < game_volume ? volume : game_volume);
            }
            complete = pdata->alpha == 0;
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

typedef struct KonquestInventoryImageList {
    RwTexture** images;
    RwTexture** alpha_images;
} KonquestInventoryImageList;

typedef struct KonquestInventoryScreenPdata {
    char pad00[0x2F8];
    int selected_item;
} KonquestInventoryScreenPdata;

typedef struct KonquestPuiRuntime {
    MkHdr hdr;
    char pad08[8];
    unsigned int id; /* +0x10 */
    char pad14[0x0C];
    MkObj* render_object; /* +0x20 */
    unsigned int render_object_instance; /* +0x24 */
    unsigned char flags; /* +0x28 */
    char pad29[0x0F];
    float spawn_delay; /* +0x38 */
} KonquestPuiDelayView;

typedef struct KonquestPuiListNode {
    KonquestPuiDelayView* object;
    struct KonquestPuiListNode* next;
    char pad08[8];
    unsigned int object_instance; /* +0x10 */
} KonquestPuiListNode;

extern const char* inv_item_default_string;
extern const char* inv_item_empty_string;

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
            if (mode == 0) {
                text =
                    get_konq_profile_value_item_description(item);
            } else if (mode == 1) {
                text = get_konq_profile_value_item_name(item);
            }
            break;
        }
        index++;
    }
    return text;
}

void pui_delay_spawn(unsigned int id, float delay) {
    KonquestPuiListNode* node;
    KonquestPuiDelayView* object;

    node = (KonquestPuiListNode*)konquest_pdata->pui_list;
    object = 0;
    while (node != 0) {
        object = node->object;
        if (node->object_instance != object->hdr.instance) {
            KonquestPuiListNode* next = node->next;

            node->object = 0;
            destroy_mkptr((MkPtr*)node);
            node = next;
            continue;
        }

        if (object != 0 && object->id == id) {
            break;
        }
        node = node->next;
        object = 0;
    }

    if (object != 0 && delay > 0.0f) {
        object->flags |= 0x20;
        object->spawn_delay = delay;
    }
}

void pui_set_color(unsigned int id, int red, int green, int blue, int alpha) {
    KonquestPuiListNode* node;
    KonquestPuiDelayView* object;
    MkObj* render_object;
    RpMaterialColor color;

    node = (KonquestPuiListNode*)konquest_pdata->pui_list;
    object = 0;
    while (node != 0) {
        object = node->object;
        if (node->object_instance != object->hdr.instance) {
            KonquestPuiListNode* next = node->next;

            node->object = 0;
            destroy_mkptr((MkPtr*)node);
            node = next;
            continue;
        }

        if (object != 0 && object->id == id) {
            break;
        }
        node = node->next;
        object = 0;
    }

    if (object != 0) {
        render_object = object->render_object;
        if (render_object != 0 &&
            render_object->hdr.instance == object->render_object_instance) {
            color.red = red;
            color.green = green;
            color.blue = blue;
            color.alpha = alpha;
            obj_set_color_for_material_by_id(render_object, 5, &color);
        }
    }
}

void update_tile_grid(void);
float p_konquest_camera_proc(void);
float p_collide_monk(void);
MkHdr* konquest_display_award_tga(int award, int arg, int mode);
int get_game_state(void);
void set_interior_cam_pos_and_ang(void);
void camera_exit_script(void);
MslSoundHandle snd_req(int sound);
void snd_stop(MslSoundHandle sound);
float get_pan_value(const Vec* position);
MslSoundHandle pan_vol_snd_req(int sound, float pan, float volume);
void* get_data_table(void* owner, int index);
float p_display_konquest_title(void);
float p_fade_konquest_hud(void);
void fade_to_black(int ticks, int freeze);
void gamelogic_jump(int mode, void* entry);
void p_konquest_mode(void);
void p_gamelogic(void);
void RwResourcesSetArenaSize(int size);
char* get_string_by_id(int id);
void camera_set_lookat_focus(void* object);
void camera_set_movement_focus_obj(void* object);
void hide_sobj_and_children(void* object);
void set_true_clip_flag_on_sobj_and_children(void* object, int value);
void hide_tile_objects(void* tile);
void remove_collisions_from_tile_and_tile_objects(void* tile);
void show_konquest_object(MkHdr* object);
void object_transition_to_state(void* object, int state, int ticks);
unsigned int fx(const char* name);
unsigned int fx_by_owner(void* owner, int type);
unsigned int fx_next_emitter(unsigned int effect);
void fx_set_param_v3(
    unsigned int effect, int param, float x, float y, float z);
void fx_reset(unsigned int effect);
void fx_reset_emit(unsigned int effect);
void fx_restart_emit(unsigned int effect);
void fx_resume_emit(unsigned int effect);
unsigned int get_table_index_by_pointer(void* owner, void* item);
unsigned int get_row_count_for_table_by_pointer(
    ScriptSlot* slot, void* table);
void cmdscript_execute(ScriptSlot* slot);
void cmdscript_setup_execution(ScriptSlot* slot, unsigned int func_index);
void cmdscript_set_parameters(CmdScript* script, unsigned int count, ...);
void set_u8_bit(unsigned char* bits, int count, int index, int value);
int get_u8_bit(unsigned char* bits, int count, int index);
unsigned long strlen(const char* text);
int is_time_a_greater_than_time_b(void* a, void* b);
void add_days_to_time(void* time, int days);
void exit_meditation(void);
void enable_trigger(void* trigger, int enabled);
void pulsate_object(
    MkHdr* object, int pulse_type, int rise_ticks, int fall_ticks,
    float minimum, float maximum);
AniData* get_animation(void);
void transition_to_anim_script(
    AnimPdata* pdata, AniData* animation, int transition, float blend);
void free_mem(void* memory);
float p_cross_fade_ambient_sounds(void);
float p_konquest_load_nis_anims(void);
float p_run_trigger_script(void);
void adjust_light_for_new_time_of_day(
    int light_index, void* light_table, int last_row);
float p_control_konquest_monk(void);
float p_animate(void);
float p_anim_idle(void);
float p_npc_idle(void);
void npc_xfer(void* npc, MkProcEntryFn entry, int transition);
void npc_restart_his_normal_behavior(int npc_id);
void hero_stop_moving(void);
void set_root_and_obj_movement_weights(
    float root_weight, float object_weight, AnimPdata* animation);

void transition_hero_to_anim_script(
    int transition, float blend, float step) {
    transition_to_anim_script(
        konquest_pdata->hero_anim, get_animation(), transition, blend);
    konquest_pdata->hero_anim->step = step;
}

void konquest_load_interior_art(void) {
    if (konquest_pdata->region_table->interior_art_name != 0) {
        unload_section_slot(0xA002F);
        load_ssf(konquest_region_data[konquest_pdata->region_index].fight_files);
        add_art_section_by_name_async(
            0xA002F, konquest_pdata->region_table->interior_art_name);
    }
}

void vdestroy_trigger_struct(KonquestTriggerStruct* trigger) {
    if (((trigger->flags >> 4) & 1) && trigger->owned_data != 0) {
        free_mem(trigger->owned_data);
    }
    trigger->hdr.instance = 0;
    mkhdr_memfree(&trigger->hdr);
}

void start_konquest_ambient_sounds(void) {
    KonquestAmbientFadePdata* fade;

    konquest_pdata->ambient_sound_main =
        snd_req(konquest_pdata->region_table->ambient_sound);
    _create_mkproc_generic_tinystack(
        0x8226, 0x1F, p_cross_fade_ambient_sounds, sizeof(*fade),
        (void**)&fade);
    if (fade != 0) {
        fade->volume_a = 0.0f;
        fade->volume_b = 0.0f;
        fade->target_a = 0.0f;
        fade->target_b = 0.0f;
        fade->delay = (float)(randu0(0x384) + 0x12C);
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

void wait_for_region_load(void) {
    while (((konquest_pdata->flags >> 7) & 1) == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
}

void nis_wait_for_region_load(void) {
    while (((konquest_pdata->flags >> 7) & 1) == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    xfer_proc(konquest_pdata->region_load_proc, p_idle);
}

float p_adjust_ambient_light(void) {
    void* light_table;
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
            pdata->light_index, light_table, last_row);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

float p_adjust_sky_ambient_light(void) {
    void* light_table;
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
            pdata->light_index, light_table, last_row);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

void p_konquest_kill_dialog_procs(void) {
    KonquestDialogKillPdata* pdata;

    pdata = (KonquestDialogKillPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return;
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

void execute_trigger(KonquestTriggerStruct* trigger) {
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
            trigger->flags |= 8;
            if (trigger->owned_data->type == 1) {
                trigger->owned_data->state = 0;
            }
        }
    }
}

void p_konquest_open_door(void) {
    KonquestDoorPdata* pdata;

    pdata = (KonquestDoorPdata*)pdata_of_proc(aproc);
    object_transition_to_state(
        pdata->door, 1, pdata->transition_ticks);
    if (pdata->remain_open != 0) {
        return;
    }
    _mkproc_sleep_ticks = (float)pdata->open_ticks;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    object_transition_to_state(pdata->door, 0, 1);
}

float p_konquest_nis_housekeeping(void) {
    update_tile_grid();
    if (konquest_pdata->region_index == 0 &&
        check_switch_edge(konquest_pdata->input_port, 6) != 0) {
        turn_controllers_off();
        fade_to_black(8, 1);
        xfer_camera(p_idle, 1);
    }
    return 1.0f;
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
                npc_restart_his_normal_behavior(npc->id);
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

int konquest_set_dialog_text(
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
            g_active_npc->fields.dialog_owner, text, print_speed);
    case 0:
        load_font(6);
        return create_dialog_proc(0, text, 3);
    default:
        return 0;
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

void set_look_at_npc(int target_type) {
    KonquestGrounding* npc;
    KonquestNpcCameraTarget* target;
    int valid;

    npc = 0;
    switch (target_type) {
    case 1:
        break;
    case 2:
        npc = konquest_pdata->hero_grounding;
        if (npc != 0) {
            if (npc->hdr.instance ==
                konquest_pdata->grounding_instance) {
                /* Valid NPC latch. */
            } else {
                npc = 0;
            }
        } else {
            npc = 0;
        }
        break;
    case 0:
        npc = (KonquestGrounding*)konquest_pdata->movement_npc.object;
        if (npc != 0) {
            if (npc->hdr.instance ==
                konquest_pdata->movement_npc.instance) {
                /* Valid NPC latch. */
            } else {
                npc = 0;
            }
        } else {
            npc = 0;
        }
        break;
    }
    if (npc != 0) {
        target = npc->camera_target;
        if (target == 0) {
            valid = 0;
        } else if (target->focus_object == 0) {
            valid = 0;
        } else {
            valid = target->ready != 0;
        }
        if (valid != 0) {
            camera_set_lookat_focus(target->focus_object);
        }
    }
}

void set_movement_npc(int target_type) {
    KonquestGrounding* npc;
    KonquestNpcCameraTarget* target;
    int valid;

    npc = 0;
    switch (target_type) {
    case 1:
        break;
    case 2:
        npc = konquest_pdata->hero_grounding;
        if (npc != 0) {
            if (npc->hdr.instance ==
                konquest_pdata->grounding_instance) {
                /* Valid NPC latch. */
            } else {
                npc = 0;
            }
        } else {
            npc = 0;
        }
        break;
    case 0:
        npc = (KonquestGrounding*)konquest_pdata->movement_npc.object;
        if (npc != 0) {
            if (npc->hdr.instance ==
                konquest_pdata->movement_npc.instance) {
                /* Valid NPC latch. */
            } else {
                npc = 0;
            }
        } else {
            npc = 0;
        }
        break;
    }
    if (npc != 0) {
        target = npc->camera_target;
        if (target == 0) {
            valid = 0;
        } else if (target->focus_object == 0) {
            valid = 0;
        } else {
            valid = target->ready != 0;
        }
        if (valid != 0) {
            camera_set_movement_focus_obj(target->focus_object);
        }
    }
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

void p_spawn_dynamic_pui(void) {
    KonquestDynamicPuiPdata* pdata;

    pdata = (KonquestDynamicPuiPdata*)pdata_of_proc(aproc);
    cmdscript_set_parameters(
        active_cmdscript, 2, pdata->item, pdata->source_type);
    cmdscript_setup_execution(
        konquest_pdata->script_owner,
        pdata->item->spawn_script_index);
    cmdscript_execute(konquest_pdata->script_owner);
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
    if (valid == 0 || is_it_safe_to_spawn_pui(item) == 0) {
        return 0;
    }

    proc = _create_mkproc_generic_nostack(
        0xA021, 0x1F, p_spawn_dynamic_pui, sizeof(*pdata),
        (void**)&pdata);
    if (proc == 0) {
        return 0;
    }
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
        pdata->position = *position;
    } else {
        if (source == 0) {
            if (proc->instance != 0) {
                ((KonquestDestroyable*)proc)->vtbl->destroy(
                    (KonquestDestroyable*)proc);
            }
            return 0;
        }
        pdata->source_instance = (unsigned int)source;
    }
    return 1;
}

int is_character_unlocked_in_profile(
    int character, int alternate) {
    unsigned int high;
    unsigned int low;
    unsigned long long bits;
    unsigned long long mask;

    if (alternate != 0) {
        high = p1_profile_common->alternate_character_bits[0] |
               default_alt_char_bits.high;
        low = p1_profile_common->alternate_character_bits[1] |
              default_alt_char_bits.low;
    } else {
        high = p1_profile_common->character_bits[0] |
               default_char_bits.high;
        low = p1_profile_common->character_bits[1] |
              default_char_bits.low;
    }
    bits = ((unsigned long long)high << 32) | low;
    mask = (unsigned long long)1 << character;
    if ((bits & mask) != 0) {
        return 1;
    }
    return 0;
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

void give_krypt_key_to_player(int award, int arg) {
    MkProc* proc;
    MkHdr* notice;
    unsigned int notice_instance;
    KonquestGrounding* grounding;

    if (konquest_pdata->hero_anim != 0) {
        proc = konquest_pdata->hero_anim->proc;
        if (proc != 0 &&
            proc->instance != konquest_pdata->hero_anim->proc_instance) {
            proc = 0;
        }
        xfer_proc(proc, p_animate);
    }

    hero_stop_moving();
    _mkproc_sleep_ticks = 10.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    notice = konquest_display_award_tga(award, arg, 0);
    notice_instance = notice->instance;
    while (notice != 0 && notice->instance == notice_instance) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_pdata->hero_anim != 0) {
        proc = konquest_pdata->hero_anim->proc;
        if (proc != 0 &&
            proc->instance != konquest_pdata->hero_anim->proc_instance) {
            proc = 0;
        }
        xfer_proc(proc, p_control_konquest_monk);
    }

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0 &&
        grounding->hdr.instance != konquest_pdata->grounding_instance) {
        grounding = 0;
    }
    if (grounding != 0) {
        npc_xfer(grounding, p_npc_idle, 0);
    }
}

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
        hero->hide_flags &= ~2;
        hero->pos.x = x;
        hero->pos.y = y;
        hero->pos.z = z;
        hero->ang.y = angle;
        update_mkobj(as_mkhdr(&hero->hdr));
        old_hero_position = hero->pos;
    }
    if (camera != 0) {
        camera->target_ang_y = angle;
    }
    update_tile_grid();
}

void correct_camera_angle(MkObj* hero, int snap) {
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

    camera->target_ang_x = 0.17f;
    if (snap != 0) {
        margin = 0.0f;
    }

    hero_angle = hero->ang.y;
    difference = hero_angle - camera->target_ang_y;
    magnitude = difference >= 0.0f ? difference : -difference;
    if (magnitude <= 0.01f) {
        camera->target_ang_y = hero_angle;
    } else if (difference < 0.0f) {
        if (difference <= -3.1415927f - margin) {
            camera->target_ang_y +=
                (0.075f * (6.2831855f + difference)) / 3.1415927f;
        } else if (difference > -3.1415927f + margin) {
            camera->target_ang_y -=
                (-0.075f * difference) / 3.1415927f;
        }
    } else {
        if (difference >= 3.1415927f + margin) {
            camera->target_ang_y -=
                (0.075f * (6.2831855f - difference)) / 3.1415927f;
        } else if (difference < 3.1415927f - margin) {
            camera->target_ang_y +=
                (0.075f * difference) / 3.1415927f;
        }
    }
    camera->target_ang_y = norm_angle(camera->target_ang_y);
}

float ramp_game_speed(void) {
    KonquestGameSpeedPdata* pdata;
    float multiplier;
    float target;
    float difference;

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

    for (;;) {
        difference = target - game_speed;
        if (difference < 0.0f) {
            difference = -difference;
        }
        if (difference <= 0.3f) {
            break;
        }

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

void konquest_setup_pui_particle(void* owner, int shared_render_object) {
    unsigned int handle;
    MkPfx* effect;
    MkObj* object;
    int emitter;

    handle = fx_by_owner(owner, 4);
    effect = find_pfx_by_handle(handle);
    fx_resume_emit(handle);

    for (emitter = 0; emitter < effect->slot_count; emitter++) {
        if (shared_render_object != 0) {
            object = (MkObj*)get_mkobj_frame(0xA00E, 0);
            pfx_bind_render_to_obj(effect, object, 1);
        } else {
            object = pfx_bind_emitter_num_to_new_obj(
                effect, (void*)0xA00E, emitter);
        }
        object->flags_08 |= 0x40;
        object->pos.z = 0.0f;
        object->pos.x = 0.0f;
        object->pos.y = -1000.0f;
        update_mkobj(object);
    }
}

float p_fade_ambient_sounds(void) {
    KonquestSoundFadePdata* pdata;
    int sound_a;
    int sound_b;
    int sound_main;
    float volume;
    float difference;
    float step;

    pdata = (KonquestSoundFadePdata*)pdata_of_proc(aproc);
    sound_a = konquest_pdata->region_table->ambient_sound_a;
    sound_b = konquest_pdata->region_table->ambient_sound_b;
    sound_main = konquest_pdata->region_table->ambient_sound;
    if (pdata == 0) {
        return -1.0f;
    }

    volume = pdata->current_volume;
    for (;;) {
        difference = volume - pdata->target_volume;
        if (difference < 0.0f) {
            difference = -difference;
        }
        step = pdata->step;
        if (step < 0.0f) {
            step = -step;
        }
        if (difference <= step) {
            break;
        }

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

float p_control_konquest_monk(void) {
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
        ->jump_sleep(state->control_proc, aproc, 0.0f);
    return 0.0f;
}

float p_monk_getup(void) {
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

float p_close_konquest_chest(void) {
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

void start_character_separation_process(float separation) {
    KonquestInteractionPdata* pdata;
    MkSobj* dialog_owner;

    if (is_this_the_monk_npc(g_active_npc) == 0 &&
        (g_active_npc->fields.flags & 0x40) != 0) {
        if (aproc->pid == 0xA014) {
            npc_set_wait_ticks(1.0f);
            npc_suspend_cmdscript();
            return;
        }

        for (;;) {
            dialog_owner = g_active_npc->fields.dialog_owner;
            if (dialog_owner != 0 && dialog_owner->id_flags != 0 &&
                dialog_owner->bound_hdr != 0) {
                break;
            }
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
        }

        _create_mkproc_generic_tinystack(
            0xA01F, 0x19, p_konquest_interaction, sizeof(*pdata),
            (void**)&pdata);
        zero_pdata_payload(sizeof(*pdata), pdata);
        pdata->npc = g_active_npc;
        pdata->initiated = 1;
        pdata->separate_characters = 1;
        pdata->interaction_type = 0x2D;
        pdata->separation = separation;
    }
}

void exit_meditation(void) {
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

void konquest_transition_to_fight(int save_progress) {
    KonquestSavedState* save;
    int other_state;

    save = (KonquestSavedState*)konquest_save_data;
    other_state = 0;
    g_game_info.field_1F8 = 1;
    if (save->fight_mode == 8) {
        other_state = 3;
    }

    if (g_game_info.plyr0.player_index == konquest_pdata->input_port) {
        set_player_state(&g_game_info.plyr0, 2);
        set_player_state(&g_game_info.plyr1, other_state);
        g_game_info.plyr0.player_index = save->player_a;
        g_game_info.plyr1.player_index = save->player_b;
    } else {
        set_player_state(&g_game_info.plyr1, 2);
        set_player_state(&g_game_info.plyr0, other_state);
        g_game_info.plyr0.player_index = save->player_b;
        g_game_info.plyr1.player_index = save->player_a;
    }
    turn_controllers_off();
    b_game_timer_off = 1;
    if (save_progress != 0) {
        save->valid = 1;
        save->region = konquest_pdata->region_index;
        save_konq_common_data_to_buffer();
        save_konq_memory_to_krd_buffer(
            p1_profile_konquest->raw[0x5D]);
    }
    _create_mkproc_generic_bigstack(
        0x9013, 0x1F, p_transition_to_fight, 0, 0);
}

float p_display_konquest_title(void) {
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
    } else {
        _mkproc_sleep_ticks = 530.0f;
    }
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    for (alpha = 0; alpha < 253; alpha += 2) {
        pfx_2d_obj_set_alpha(title, alpha);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (refresh_rate() == 60) {
        _mkproc_sleep_ticks = 100.0f;
    } else {
        _mkproc_sleep_ticks = 20.0f;
    }
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

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

void konquest_hide_hud(void) {
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

float p_fade_konquest_hud(void) {
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
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[0], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[1], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[3], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[4], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[3], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[4], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[5], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_objects[6], object);
        if (object != 0) set_screen_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[0], object);
        if (object != 0) set_string_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[1], object);
        if (object != 0) set_string_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[2], object);
        if (object != 0) set_string_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_extra_label, object);
        if (object != 0) set_string_obj_alpha(object, alpha);
        KONQUEST_RESOLVE_LATCH(konquest_pdata->hud_labels[5], object);
        if (object != 0) set_string_obj_alpha(object, alpha);

        if (g_fade_hud_in != 0) {
            if (alpha >= 1.0f) {
                konquest_show_hud();
                break;
            }
        } else if (alpha <= 0.0f) {
            konquest_hide_hud();
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return -1.0f;
}

float p_show_koin_award_text(void) {
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

int load_hero_model(int animation_script) {
    unsigned char hero_age;
    int slot;
    MkObj* hero;
    AnimPdata* animation;
    MkProc* created;
    char model_name[16];

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
    hero->pos.z = 0.0f;
    hero->pos.y = 0.0f;
    hero->pos.x = 0.0f;
    hero->ang.y = 3.1415927f;
    hero->flags_08 |= 0x20;
    hero->flags_08 |= 4;
    hero->flags_08 |= 0x40;
    hero->flags_08 |= 8;
    update_mkobj(as_mkhdr(&hero->hdr));
    hero->light_flags = 1;
    build_bones_tbl(hero, konquest_human_bones);
    hero->flipped_bone_map = &flipped_konquest_human_bones;
    hero->flags_09 |= 0x80;
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
    memset(pdata_monk, 0, sizeof(pdata_monk));
    init_shadow((ShadowObject*)pdata_monk, hero);
    ShadowStrength = 0.7f;
    return (int)created;
}

void setup_konquest_pui(KonquestPuiRuntime* pui) {
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
        if (object != 0 && object->hdr.instance != pui->object_instance) {
            object = 0;
        }
        if (object != 0) {
            object->flags_08 |= 0x40;
            object->flags_08 |= 8;
        }
        return;
    case 1:
        object = pui->object;
        if (object != 0 && object->hdr.instance != pui->object_instance) {
            object = 0;
        }
        if (object != 0) {
            object->flags_08 |= 0x40;
            object->flags_08 |= 8;
            object->flags_08 &= ~1;
            object->pos_vel.x = 0.0f;
            object->pos_vel.y = 0.005f;
            object->pos_vel.z = 0.0f;
        }
        return;
    case 2:
        object = pui->object;
        if (object != 0 && object->hdr.instance != pui->object_instance) {
            object = 0;
        }
        if (object != 0) {
            object->flags_08 |= 8;
            object->flags_08 |= 4;
            object->flags_08 |= 0x40;
            object->flags_08 &= ~1;
            object->pos_vel.x = 0.0f;
            object->pos_vel.y = 0.005f;
            object->pos_vel.z = 0.0f;
            object->ang_vel.y = 0.05f;
        }
        return;
    case 3:
        owner = pui->owner;
        if (owner != 0 && owner->hdr.instance != pui->owner_instance) {
            owner = 0;
        }
        object = pui->object;
        if (object != 0 && object->hdr.instance != pui->object_instance) {
            object = 0;
        }
        hero = konquest_pdata->hero_object;
        if (hero != 0 && hero->hdr.instance != konquest_pdata->hero_instance) {
            hero = 0;
        }
        if (hero != 0 && object != 0 && owner != 0) {
            x = hero->field_24->at.x;
            z = hero->field_24->at.z;
            length_squared = x * x + z * z;
            inverse_length = 0.0f;
            if (length_squared > 0.0f) {
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
            object->flags_08 |= 0x40;
            object->flags_08 |= 8;
            owner->interaction->closed = 0;
            pui->lifetime = 60.0f;
        }
        return;
    case 4:
        object = pui->object;
        if (object != 0 && object->hdr.instance != pui->object_instance) {
            object = 0;
        }
        if (object != 0) {
            object->flags_08 |= 8;
            object->flags_08 |= 4;
            object->flags_08 |= 0x40;
            object->flags_08 &= ~1;
            object->ang_vel.y = 0.05f;
        }
        return;
    }
}

void load_konquest_tiles(void) {
    MkObj* model;
    MkHdr* tile_objects;
    unsigned int setup_function;
    KonquestArtIdRef function_ref;
    KonquestSectionContext context;
    KonquestScriptSlotView* script;

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
        function_ref.name = get_script_function_by_name(
            konquest_pdata->script_owner, "konquest_001");
        setup_function = function_ref.id;
    } else {
        setup_function = konquest_pdata->region_table->setup_function;
    }
    konquest_pdata->flags |= 0x40;
    cmdscript_setup_execution(
        konquest_pdata->script_owner, setup_function);
    cmdscript_execute(konquest_pdata->script_owner);
    konquest_pdata->flags &= ~0x40;
    update_visible_tiles();
    generate_door_paths();

    script = (KonquestScriptSlotView*)konquest_pdata->script_owner;
    tile_objects = konquest_pdata->tile_objects;
    if (tile_objects != 0 &&
        tile_objects->instance != konquest_pdata->tile_objects_instance) {
        tile_objects = 0;
    }

    if (konquest_pdata->region_table->effect_bank_name != 0) {
        context.slot = 0x60029;
        context.owner = tile_objects;
        context.flags = 0;
        active_cmdscript->mko = konquest_pdata->script_owner;
        script->section_context = &context.slot;
        load_effect_bank(
            konquest_pdata->region_table->effect_bank_name);
        script->section_context = 0;
    }

    context.slot = 0x60030;
    context.owner = tile_objects;
    context.flags = 0;
    save_current_ssf();
    load_ssf((MkFileEntry*)konquest_common_file_table);
    active_cmdscript->mko = konquest_pdata->script_owner;
    script->section_context = &context.slot;
    load_effect_bank("common_pfx.mko");
    script->section_context = 0;
    restore_previous_ssf();
}

void konquest_start_npc_nis(void) {
    MkObj* hero;
    MkProc* fade_proc;
    KonquestSoundFadePdata* fade;
    MkSobj* dialog_owner;
    MonkStateData* state;

    if (g_active_npc != 0) {
        g_active_npc->fields.flags |= 0x40;
        g_active_npc->fields.flags |= 0x80;
        g_active_npc->fields.flags |= 8;
    }
    if (konquest_pdata != 0) {
        konquest_pdata->flags &= ~0x10;
    }

    hero = konquest_pdata->hero_object;
    if (hero != 0 && hero->hdr.instance != konquest_pdata->hero_instance) {
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
    konquest_pdata->time_passing = 0;

    if (aproc->pid == 0xA014) {
        npc_set_wait_ticks(1.0f);
        npc_suspend_cmdscript();
        return;
    }
    _mkproc_sleep_ticks = 1.0f;
    ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();

    if ((g_active_npc->fields.state_flags & 0x40) == 0) {
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

    for (;;) {
        dialog_owner = g_active_npc->fields.dialog_owner;
        if (dialog_owner != 0 && dialog_owner->id_flags != 0 &&
            dialog_owner->bound_hdr != 0) {
            break;
        }
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }
    konquest_pdata->movement_npc.object = (MkHdr*)g_active_npc;
    konquest_pdata->movement_npc.instance =
        ((MkHdr*)g_active_npc)->instance;
}

float p_konquest_load_nis_anims(void) {
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

MkHdr* get_konquest_tile_objects_obj(void) {
    MkHdr* object;

    if (konquest_pdata == 0) {
        return 0;
    }
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
    return object;
}

void get_konquest_pui_object_pos(Vec* position, MkSobj* sobj) {
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
        *position = object->pos;
    }
}

void konquest_set_current_portal_uid(int uid) {
    konquest_pdata->current_portal_uid = uid;
}

int get_hero_state(void) {
    return konquest_pdata->hero_state;
}

int get_previous_konquest_region_number(void) {
    return p1_profile_konquest->raw[0x5e];
}

void set_reference_pui(void* pui) {
    konquest_pdata->reference_pui = pui;
}

void set_age_progression(int progression) {
    *(int*)(konquest_save_data + 0x94) = progression;
}

int get_save_progress_flag(void) {
    return konquest_save_on_exit;
}

void set_save_progress_flag(int enabled) {
    konquest_save_on_exit = enabled != 0;
}

void set_monk_age(int age) {
    p1_profile_konquest->fields.hero_age = (unsigned char)age;
}

int get_monk_age(void) {
    return p1_profile_konquest->fields.hero_age;
}

void start_time_passing(void) {
    konquest_pdata->time_passing = 1;
}

void stop_time_passing(void) {
    konquest_pdata->time_passing = 0;
}

void set_konquest_weather(int type, int param_a, int param_b) {
    konquest_pdata->weather_type = type;
    konquest_pdata->weather_param_a = param_a;
    konquest_pdata->weather_param_b = param_b;
}

int get_num_puis(void) {
    return (int)(konquest_pdata->pui_end - konquest_pdata->pui_begin) - 1;
}

int get_konquest_pui_inventory_bit_index(const int* pui) {
    if (pui != 0) {
        return pui[6];
    }
    return -1;
}

int check_skip_conversation_flag(void) {
    if (konquest_pdata != 0) {
        return (konquest_pdata->flags >> 4) & 1;
    }
    return 0;
}

void get_current_time(void* time) {
    memcpy(time, konquest_pdata->current_time, sizeof(konquest_pdata->current_time));
}

int get_konquest_game_mode(void) {
    if (konquest_pdata->game_mode_index < 0) {
        return 0;
    }
    return konquest_pdata->game_modes[konquest_pdata->game_mode_index];
}

int is_game_mode_in_stack(int game_mode) {
    int index;

    for (index = 0; index <= konquest_pdata->game_mode_index; index++) {
        if (konquest_pdata->game_modes[index] == game_mode) {
            return 1;
        }
    }
    return 0;
}

float p_monitor_meditation_time(void) {
    unsigned char target_time[0x18];

    memcpy(
        target_time, konquest_pdata->current_time,
        sizeof(target_time));
    add_days_to_time(target_time, 7);

    while (is_time_a_greater_than_time_b(
               target_time, konquest_pdata->current_time)) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestProcSleepVtable*)aproc->vtbl)->sleep();
    }

    if (konquest_pdata->interior_active == 0) {
        exit_meditation();
    }
    return -1.0f;
}

float p_konquest_switch_1(void) {
    KonquestSwitchState* input;
    int game_mode;

    input = switch_pdata->state;
    if (input == 0 || konquest_editor_mode_on != 0) {
        return -1.0f;
    }
    if (input->input_port != konquest_pdata->input_port) {
        return -1.0f;
    }

    game_mode = get_konquest_game_mode();
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
    KonquestTriggerRequirementOwner* owner, MkObj* hero) {
    KonquestTriggerRequirement* requirement;
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
        delta_z = requirement->position.z - hero->pos.z;
        inverse_length = 0.0f;
        delta_x = requirement->position.x - hero->pos.x;
        length_squared = delta_x * delta_x + delta_z * delta_z;
        if (length_squared > 0.0f) {
            estimate.value = length_squared;
            estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
            product = estimate.value * (length_squared * estimate.value);
            correction = 3.0f - product;
            inverse_length =
                0.0625f * estimate.value * correction *
                -(correction * (product * correction) - 12.0f);
        }

        inverse_forward_length = 0.0f;
        forward_x = hero->field_24->at.x;
        forward_z = hero->field_24->at.z;
        forward_length_squared =
            forward_x * forward_x + forward_z * forward_z;
        if (forward_length_squared > 0.0f) {
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
                delta_x * inverse_length * forward_x +
                delta_z * inverse_length * forward_z) < 0.7853982f) {
            result = 1;
        }
        break;
    case 2:
        if (requirement->orientation != 0) {
            delta_z = requirement->position.z - hero->pos.z;
            inverse_length = 0.0f;
            delta_x = requirement->position.x - hero->pos.x;
            length_squared = delta_x * delta_x + delta_z * delta_z;
            if (length_squared > 0.0f) {
                estimate.value = length_squared;
                estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
                product =
                    estimate.value * (length_squared * estimate.value);
                correction = 3.0f - product;
                inverse_length =
                    0.0625f * estimate.value * correction *
                    -(correction * (product * correction) - 12.0f);
            }

            inverse_forward_length = 0.0f;
            forward_x = hero->field_24->at.x;
            forward_z = hero->field_24->at.z;
            forward_length_squared =
                forward_x * forward_x + forward_z * forward_z;
            if (forward_length_squared > 0.0f) {
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
                delta_x * inverse_length * forward_x +
                delta_z * inverse_length * forward_z);
            if (facing_angle < 0.7853982f) {
                angle = requirement->orientation->angle;
                sine = gxMathSin(angle->angle);
                if ((requirement->position.z - hero->pos.z) *
                            gxMathCos(angle->angle) +
                        (requirement->position.x - hero->pos.x) * sine +
                        (requirement->position.y - hero->pos.y) * 0.0f <
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

void konquest_state_init(void) {
    memset(konquest_save_data, 0, 0x98);
}

void konquest_restore_saved_state(void) {
    KonquestSavedState* save;
    char* script_name;
    int preserve_word;

    save = (KonquestSavedState*)konquest_save_data;
    script_name = save->script_name;
    if (script_name != 0 && script_name[0] != 0 &&
        strcmp(script_name, "0") != 0) {
        active_cmdscript->unk28 = get_script_function_by_name(
            konquest_pdata->script_owner, script_name);
    }
    preserve_word = save->preserve_word;
    zero_pdata_payload(sizeof(*save), (MkHdr*)save);
    save->preserve_word = preserve_word;
    update_visible_tiles();
}

void cleanup_konquest(void) {
    if (konquest_pdata != 0) {
        stop_sound_tracking_process(&konquest_pdata->attached_sounds);
        konquest_pdata = 0;
    }
    cleanup_npc_manager();
    TearDownShadow((ShadowObject*)pdata_monk);
    destroy_shadow_system();
    npc_shadow_teardown();
    destroy_konquest_shadow_collision_lists();
    if (g_pui_events != 0) {
        free_mem(g_pui_events);
        g_pui_events = 0;
    }
    if (mode_of_play != 7 && mode_of_play != 8) {
        memset(konquest_save_data, 0, sizeof(KonquestSavedState));
    }
    cleanup_mission_state();
    f_writing_konquest_profile = 0;
}

void set_konquest_region_number(unsigned int region) {
    if (p1_profile_konquest->raw[0x5d] == region) {
        return;
    }
    p1_profile_konquest->raw[0x5e] = p1_profile_konquest->raw[0x5d];
    p1_profile_konquest->raw[0x5d] = (unsigned char)region;
    *(int*)&p1_profile_konquest->raw[0x64] = 0;
    konquest_force_region_reload = 0;
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

void konquest_set_current_inventory_item(int item) {
    char* pdata;

    pdata = get_screen_pdata();
    if (pdata != 0) {
        *(int*)(pdata + 0x2f8) = item;
    }
}

void konquest_fade_from_black(int ticks, int event) {
    konquest_fade_screen(ticks, 0, event, 0);
}

void konquest_fade_to_black(int ticks, int event) {
    konquest_fade_screen(ticks, 0, event, 1);
}

void vdestroy_konquest_sobj_info(MkHdr* object) {
    object->instance = 0;
    mkhdr_memfree(object);
}

void vdestroy_konquest_sobj(KonquestSobj* object) {
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

void vdestroy_konquest_obj(KonquestObject* object) {
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

static void vdestroy_dialog_pdata(KonquestDialogRuntimePdata* pdata) {
    KonquestDestroyable* object;

    if (pdata->dialog_art != 0 && pdata->dialog_art->owner != 0) {
        pdata->dialog_art->owner->flags &= ~4;
    }

    object = (KonquestDestroyable*)pdata->art[0].object;
    if (object != 0) {
        if (object->instance == pdata->art[0].instance) {
            /* Valid ownership latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[0].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[0].object = 0;
        pdata->art[0].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[1].object;
    if (object != 0) {
        if (object->instance == pdata->art[1].instance) {
            /* Valid ownership latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[1].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[1].object = 0;
        pdata->art[1].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[2].object;
    if (object != 0) {
        if (object->instance == pdata->art[2].instance) {
            /* Valid ownership latch. */
        } else {
            object = 0;
        }
    } else {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[2].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[2].object = 0;
        pdata->art[2].instance = 0;
    }

    pdata->hdr.instance = 0;
    mkhdr_memfree(&pdata->hdr);
}

static void destroy_award_art(KonquestAwardArtPdata* pdata) {
    KonquestDestroyable* object;

    object = (KonquestDestroyable*)pdata->art[0].object;
    if (object != 0 && object->instance != pdata->art[0].instance) {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[0].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[0].object = 0;
        pdata->art[0].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[1].object;
    if (object != 0 && object->instance != pdata->art[1].instance) {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[1].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[1].object = 0;
        pdata->art[1].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[3].object;
    if (object != 0 && object->instance != pdata->art[3].instance) {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[3].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[3].object = 0;
        pdata->art[3].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[2].object;
    if (object != 0 && object->instance != pdata->art[2].instance) {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[2].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[2].object = 0;
        pdata->art[2].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[4].object;
    if (object != 0 && object->instance != pdata->art[4].instance) {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[4].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[4].object = 0;
        pdata->art[4].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[5].object;
    if (object != 0 && object->instance != pdata->art[5].instance) {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[5].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[5].object = 0;
        pdata->art[5].instance = 0;
    }

    object = (KonquestDestroyable*)pdata->art[6].object;
    if (object != 0 && object->instance != pdata->art[6].instance) {
        object = 0;
    }
    if (object != 0) {
        if (pdata->art[6].object->instance != 0) {
            object->vtbl->destroy(object);
        }
        pdata->art[6].object = 0;
        pdata->art[6].instance = 0;
    }
}

void remove_collision_volume_on_object(void) {
    KonquestRemoveCollisionPdataRef pdata;
    KonquestCollisionOwner* owner;
    KonquestCollisionVolume* volume;

    pdata.raw = pdata_of_proc(aproc);
    owner = pdata.remove_collision->owner;
    if (owner != 0 &&
        owner->hdr.instance != pdata.remove_collision->owner_instance) {
        owner = 0;
    }

    if (owner != 0) {
        volume = owner->collision_volume;
        if (volume != 0) {
            destroy_list(&volume->objects);
            volume->active = 0;
        }
    }
}

void enable_attached_sound_by_uid(int uid, int enabled) {
    KonquestAttachedSoundRef sound;
    MkPtr* link;

    link = konquest_pdata->attached_sounds;
    while (link != 0) {
        sound.hdr = link->hdr;
        if (sound.hdr->instance != link->instance) {
            MkPtr* next;

            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (sound.sound->uid == uid) {
                sound.sound->enabled = enabled;
            }
            link = link->next;
        }
    }
}

/* Soft ceiling: set_interaction_camera_script ~96% -- global load coloring; stop. */
void set_interaction_camera_script(void* script) {
    if ((g_active_npc->raw[0x1d] & 0x40) == 0) {
        run_interaction_camera_script(konquest_pdata->script_owner, script);
    }
}

void mini_mission_inactive(int mission) {
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

int is_pui_an_interior_item(const char* pui) {
    return *(const float*)(pui + 0x1c) >= 1000.0f;
}

/* Soft ceiling: should_this_pui_be_saved ~93.33% -- comparison scheduling; stop. */
int should_this_pui_be_saved(const char* pui) {
    if (pui != 0 && (pui[0x28] & 2) != 0) {
        return 1;
    }
    return 0;
}

void play_beam_advance_sound(int delay) {
    snd_req_delay(0x158e, delay);
}

void npc_hide_skip_message(void) {
    del_string_obj_by_id(0x900f);
}

void* get_nth_tile_struct(int index) {
    if (index < konquest_pdata->tile_width * konquest_pdata->tile_height + 1) {
        return &konquest_pdata->tile_structs[index];
    }
    return 0;
}

void initialize_tile_patch_sobj(char* object) {
    object[9] |= 0x80;
    sobj_set_priority(object, 9);
}

void* hide_an_atomic(void* atomic) {
    hide_atomic(atomic);
    return atomic;
}

int setup_sobj_for_tile_object(MkSobj* object, void* tile_object);
int setup_pebble_system_for_tile_object(MkSobj* object, void* tile_object);

void setup_children_sobjs_of_tile_object(
    MkSobj* object, void* tile_object) {
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
            atomic = konquest_atomic_from_frame_link(link);
            if (atomic->object.type == 1) {
                MkSobj* child;

                child = konquest_sobj_from_atomic(atomic);
                if (child != 0) {
                    setup_sobj_for_tile_object(child, tile_object);
                }
            }
            link = next;
        }
        frame = next_frame;
    }
}

void setup_children_pebbles_of_tile_object(
    MkSobj* object, void* tile_object) {
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
            atomic = konquest_atomic_from_frame_link(link);
            if (atomic->object.type == 1) {
                MkSobj* child;

                child = konquest_sobj_from_atomic(atomic);
                if (child != 0) {
                    setup_pebble_system_for_tile_object(child, tile_object);
                }
            }
            link = next;
        }
        frame = next_frame;
    }
}

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

void show_fight_message(int message) {
    void* pdata;
    void* proc;

    pdata = 0;
    proc = _create_mkproc_generic_tinystack(
        0x9030, 0x1f, p_show_fight_message, 0xc, &pdata);
    if (proc != 0 && pdata != 0) {
        ((int*)pdata)[2] = message;
    }
}

void save_hero_position_and_angle_prior_to_fight(float angle_offset) {
    char* hero;

    hero = (char*)konquest_pdata->hero_object;
    if (hero != 0 && *(unsigned int*)(hero + 4) != konquest_pdata->hero_instance) {
        hero = 0;
    }
    if (hero != 0) {
        *(float*)(konquest_save_data + 0x10) = *(float*)(hero + 0xa0);
        *(float*)(konquest_save_data + 0x14) = *(float*)(hero + 0xa4);
        *(float*)(konquest_save_data + 0x18) = *(float*)(hero + 0xa8);
        *(float*)(konquest_save_data + 0x1c) =
            *(float*)(hero + 0xd4) + angle_offset;
    }
}

void konquest_run_camera_script(void* script, int flags) {
    if (konquest_pdata->script_owner != 0) {
        run_camera_script(konquest_pdata->script_owner, script, flags);
    }
}

void restore_hero_grounding(void) {
    KonquestGrounding* grounding;

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0 &&
        grounding->hdr.instance != konquest_pdata->grounding_instance) {
        grounding = 0;
    }
    if (grounding != 0) {
        grounding->flags &= ~2;
    }
}

void suspend_hero_grounding(void) {
    KonquestGrounding* grounding;

    grounding = konquest_pdata->hero_grounding;
    if (grounding != 0 &&
        grounding->hdr.instance != konquest_pdata->grounding_instance) {
        grounding = 0;
    }
    if (grounding != 0) {
        grounding->flags |= 2;
    }
}

void konquest_camera_return_to_normal(void) {
    destroy_mkprocs_pid(0x9006);
    xfer_camera(p_konquest_camera_proc, 1);
}

int konquest_nis_anims_loaded(void) {
    return find_mkproc_pid(0x901b) == 0;
}

void give_reward_to_player(int award) {
    int pid;

    pid = aproc->pid;
    if (pid == 0xa014 || pid == 0xa002) {
        konquest_display_award_tga(award, 0, 2);
    } else {
        konquest_display_award_tga(award, 0, 1);
    }
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

void mini_mission_completed(int mission) {
    snd_req(0x15a1);
    set_konq_profile_value(4, mission, 1);
    set_konq_profile_value(3, mission, 0);
}

void* get_pui_item_at_inv_bit_index(int index) {
    if (konquest_pdata->pui_begin + index + 1 <
        konquest_pdata->pui_end) {
        return get_data_table(konquest_pdata->script_owner, index);
    }
    return 0;
}

void display_konquest_title(void) {
    void* pdata;
    char* proc;

    pdata = 0;
    if (konquest_editor_mode_on == 0) {
        proc = _create_mkproc_generic_tinystack(
            0x8228, 0x1f, p_display_konquest_title, 8, &pdata);
        if (proc != 0) {
            proc[0xa8] |= 0x10;
        }
    }
}

void konquest_fade_hud(int fade_in) {
    destroy_mkprocs_pid(0xa023);
    _create_mkproc_generic_tinystack(
        0xa023, 0x2e, p_fade_konquest_hud, 0, 0);
    g_fade_hud_in = fade_in;
}

void konquest_transition_from_fight(void) {
    fade_to_black(8, 1);
    gamelogic_jump(4, p_konquest_mode);
}

float p_transition_to_fight(void) {
    fade_to_black(8, 1);
    RwResourcesSetArenaSize(0x48000);
    mode_of_play = 8;
    gamelogic_jump(2, p_gamelogic);
    return -1.0f;
}

void npc_show_skip_message(void) {
    char* text;

    text = get_string_by_id(0x10001);
    load_font(6);
    string_center_xy(0x900f, 6, text, screen_width / 2, 0x1a1, 0xb);
}

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

void set_hero_punched_ground_collisions(int punched) {
    MkObj* hero;

    hero = konquest_pdata->hero_object;
    if (hero != 0 && hero->hdr.instance != konquest_pdata->hero_instance) {
        hero = 0;
    }
    if (punched == 0) {
        hero->ground_colls = monk_ground_colls;
    } else {
        hero->ground_colls = monk_laying_on_ground_colls;
    }
}

void unhide_tile(KonquestTile* tile) {
    MkPtr* link;

    tile->visible = 1;
    if (tile->scene != 0) {
        unhide_sobj_and_children(tile->scene);
        set_true_clip_flag_on_sobj_and_children(tile->scene, 0);
    }

    link = tile->objects;
    while (link != 0) {
        if (link->instance != link->hdr->instance) {
            MkPtr* next;

            next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            show_konquest_object(link->hdr);
            link = link->next;
        }
    }
}

void hide_tile(KonquestTile* tile) {
    tile->visible = 0;
    if (tile->scene != 0) {
        hide_sobj_and_children(tile->scene);
        set_true_clip_flag_on_sobj_and_children(tile->scene, 0);
    }
    hide_tile_objects(tile);
    remove_collisions_from_tile_and_tile_objects(tile);
}

AniTextureControl* konquest_create_monk_face_ani_texture(MkObj* object) {
    char texture_name[0x40];
    KonquestArtIdRef art;
    int slot;
    AniTextureControl* texture;

    texture = 0;
    slot = 0xB002A;
    if (mode_of_play == 8) {
        slot = 0x2001E;
    }

    if (konquest_editor_mode_on == 0) {
        sprintf(
            texture_name, "KON_HERO_0%d_MOUTH",
            p1_profile_konquest->fields.hero_age + 1);
        art.id = get_artid_of_named_item_in_slot(slot, texture_name, 0);
        if (art.id != 0) {
            texture = append_wiff_to_clump_material_id(
                slot, art.name, object->clump, 1);
        }
    }
    return texture;
}

KonquestChildObject* find_child_subobject_by_enumeration(
    KonquestObject* object, int enumeration) {
    MkPtr* link;

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
                konquest_pdata->region_table->enumerations[index].enumeration ==
                    enumeration) {
                return child;
            }
            link = link->next;
        }
    }
    return 0;
}

void npc_play_teleported_sound(void) {
    Vec position = {0.0f, 0.0f, 0.0f};
    float volume;

    volume = 0.0f;
    if (g_active_npc != 0) {
        position = g_active_npc->fields.spatial->position;
        volume = get_volume_from_distance(&position, 25.0f, 10.0f);
    }
    if (volume != 0.0f) {
        pan_vol_snd_req(0x147E, get_pan_value(&position), volume);
    }
}

float p_load_hero_art_section(void) {
    char section_name[0x20];
    int slot;

    sprintf(
        section_name, "konquest_monk_%d.sec",
        p1_profile_konquest->fields.hero_age + 1);
    if (mode_of_play == 8) {
        KonquestSaveHeader* save;

        save = (KonquestSaveHeader*)konquest_save_data;
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

void* get_visible_tile_set(int index) {
    char* tile;

    tile = get_nth_tile_struct(index);
    if (tile == 0) {
        return 0;
    }
    return *(void**)(tile + 0x1c);
}

float p_konquest_transition_to_state(void) {
    KonquestTransitionPdataRef pdata;

    pdata.hdr = pdata_of_proc(aproc);
    if (pdata.hdr != 0) {
        object_transition_to_state(
            pdata.transition->object,
            pdata.transition->state,
            pdata.transition->flags);
    }
    return -1.0f;
}

int is_button_pressed(char* button) {
    if (button == 0) {
        return 0;
    }
    if ((button[0x24] & 0x40) != 0) {
        button[0x24] &= ~0x40;
        return 1;
    }
    return 0;
}

int konquest_is_save_allowed(void) {
    void* intro_script;
    MkProc* proc;
    KonquestCameraScriptPdataRef pdata;
    int count;
    int index;

    intro_script = get_script_function_by_name(
        konquest_pdata->script_owner, "fight_intro_cam_1");
    proc = find_mkproc_pid(0x9006);
    if (proc != 0) {
        pdata.hdr = pdata_of_proc(proc);
        if (pdata.hdr != 0 && pdata.camera->script == intro_script) {
            return 0;
        }
    }

    index = 0;
    count = konquest_pdata->game_mode_index + 1;
    if (konquest_pdata->game_mode_index >= 0) {
        do {
            if (konquest_pdata->game_modes[index] == 4) {
                return 0;
            }
            index++;
            count -= 1;
        } while (count != 0);
    }
    if (get_game_state() == 0x14) {
        return 0;
    }
    return 1;
}

void hero_start_fx_at_position(void* owner, const float* offset) {
    unsigned int effect;
    MkObj* hero;
    float position[3];

    hero = konquest_pdata->hero_object;
    if (hero != 0 && hero->hdr.instance != konquest_pdata->hero_instance) {
        hero = 0;
    }
    effect = fx_by_owner(owner, 4);
    position[0] = hero->pos.x + offset[0];
    position[1] = hero->pos.y + offset[1];
    position[2] = hero->pos.z + offset[2];
    fx_set_param_v3(
        effect, 0x202, position[0], position[1], position[2]);
    fx_restart_emit(effect);
}

void attach_pfx_to_object(
    MkObj* object, void* effect_owner, const Vec* offset) {
    unsigned int effect;
    unsigned int emitter;
    float x;
    float y;
    float z;

    effect = fx_by_owner(effect_owner, 4);
    emitter = fx_next_emitter(effect);
    if (object != 0) {
        x = object->pos.x + offset->x;
        y = object->pos.y + offset->y;
        z = object->pos.z + offset->z;
        if (emitter != 0) {
            fx_set_param_v3(emitter, 0x202, x, y, z);
            fx_restart_emit(emitter);
            fx_resume_emit(emitter);
        }
    }
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

void player_add_item_to_inventory(KonquestPuiDefinition* item) {
    int index;
    unsigned int table_index;
    int current_value;

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
        &p1_profile_konquest->raw[0x291], get_num_puis(), index, 1);
    if (item == 0) {
        return;
    }

    switch (item->type) {
    case 3:
        current_value = get_konq_profile_value(7, 0);
        set_konq_profile_value(7, 0, item->value + current_value);
        return;
    case 4:
        current_value = get_konq_profile_value(8, 0);
        set_konq_profile_value(8, 0, item->value + current_value);
        return;
    case 5:
        current_value = get_konq_profile_value(9, 0);
        set_konq_profile_value(9, 0, item->value + current_value);
        return;
    case 6:
        current_value = get_konq_profile_value(10, 0);
        set_konq_profile_value(10, 0, item->value + current_value);
        return;
    case 7:
        current_value = get_konq_profile_value(11, 0);
        set_konq_profile_value(11, 0, item->value + current_value);
        return;
    case 8:
        current_value = get_konq_profile_value(12, 0);
        set_konq_profile_value(12, 0, item->value + current_value);
        return;
    default:
        return;
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
        if (table_index >= konquest_pdata->pui_end ||
            table_index <= konquest_pdata->pui_begin) {
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

void player_remove_item_from_inventory(void* item) {
    int index;
    unsigned int table_index;

    if (item == 0) {
        index = -1;
    } else {
        table_index = get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (konquest_pdata->pui_begin < table_index &&
            table_index < konquest_pdata->pui_end) {
            index = table_index - (konquest_pdata->pui_begin + 1);
        } else {
            index = -1;
        }
    }
    if (index >= 0) {
        set_u8_bit(
            &p1_profile_konquest->raw[0x291], get_num_puis(), index, 0);
    }
}

int player_has_item(void* item) {
    int index;
    unsigned int table_index;

    if (item == 0) {
        index = -1;
    } else {
        table_index = get_table_index_by_pointer(konquest_pdata->script_owner, item);
        if (konquest_pdata->pui_begin < table_index &&
            table_index < konquest_pdata->pui_end) {
            index = table_index - (konquest_pdata->pui_begin + 1);
        } else {
            index = -1;
        }
    }
    if (index < 0) {
        return 0;
    }
    return get_u8_bit(
        &p1_profile_konquest->raw[0x291], get_num_puis(), index);
}

void calc_print_speed_for_nis_dialog(void* unused, unsigned int ticks) {
    KonquestDialogPdataRef pdata;
    unsigned int speed;
    unsigned int length;

    pdata.hdr = pdata_of_proc(aproc);
    speed = 3;
    length = strlen(pdata.dialog->text);
    if ((int)length > 0) {
        speed = ticks / length;
    }
    if ((int)speed >= 10) {
        speed = 10;
    }
    if ((int)speed < 2) {
        speed = 2;
    } else if ((int)speed >= 10) {
        speed = 10;
    }
    pdata.dialog->print_speed = speed;
    pdata.dialog->print_ticks = ticks;
    pdata.dialog->active = 1;
}

int get_tile_from_position(const float* position) {
    int row;
    int column;

    if (position[0] >= 1000.0f) {
        return konquest_pdata->tile_width * konquest_pdata->tile_height;
    }
    row = (int)((position[2] + konquest_pdata->tile_origin_z) / 192.0f);
    if (row < 0 || row >= konquest_pdata->tile_height) {
        return -1;
    }
    column = (int)((position[0] + konquest_pdata->tile_origin_x) / 192.0f);
    if (column < 0 || column >= konquest_pdata->tile_width) {
        return -1;
    }
    return column + row * konquest_pdata->tile_width;
}

void* find_konquest_object_struct_by_uid(int uid) {
    int tile_index;

    tile_index = 0;
    do {
        KonquestTileRecord* tile;
        MkPtr* link;

        if (tile_index >= konquest_pdata->tile_width * konquest_pdata->tile_height) {
            return 0;
        }
        tile = &konquest_pdata->tile_structs[tile_index];
        link = tile->objects;
        while (link != 0) {
            KonquestUidObjectRef object;

            object.hdr = link->hdr;
            if (link->instance == object.hdr->instance) {
                if (object.object->uid == uid) {
                    return object.object;
                }
                link = link->next;
            } else {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            }
        }
        tile_index += 1;
    } while (1);
}

void check_and_act_on_trigger_timed_action(char* action) {
    if (is_time_a_greater_than_time_b(
            konquest_pdata->current_time, action + 0x28) != 0) {
        if (*(int*)(action + 0x40) == 2) {
            enable_trigger(*(void**)(action + 8), 0);
        } else if (*(int*)(action + 0x40) > 0 &&
                   *(int*)(action + 0x40) < 2) {
            enable_trigger(*(void**)(action + 8), 1);
        }
        *(int*)(action + 0x40) = 0;
    }
}
