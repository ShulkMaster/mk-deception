#include "runtime/anim_pdata.h"
#include "runtime/asset.h"
#include "game/konquest.h"
#include "game/collision.h"
#include "game/nis.h"
#include "game/player_actions.h"
#include "math/gxMath.h"
#include "math/mk_math.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/cam.h"
#include "runtime/image.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/section.h"
#include "runtime/mk_struct.h"
#include "runtime/cstring.h"
#include "runtime/utils.h"
#include "platform/gcutils.h"
#include "platform/io.h"
#include "platform/main.h"
#include "rw/rwcamera_internal.h"
#include "rw/rplight.h"
#include "libmkparticle/rw_engine.h"

typedef struct KonquestWaypoint {
    Vec position; /* +0x00 */
    float angle; /* +0x0C */
    unsigned int flags; /* +0x10 */
    int script_function; /* +0x14 */
} KonquestWaypoint;

typedef struct KonquestPathData {
    MkHdr hdr;
    KonquestWaypoint* waypoints; /* +0x08 */
    int table_index; /* +0x0C */
    int waypoint_count; /* +0x10 */
    int travel_mode; /* +0x14 */
    int destination_type; /* +0x18 */
    Vec destination; /* +0x1C */
    int use_animation_override; /* +0x28 */
    int animation_override; /* +0x2C */
    union {
        int current_waypoint;
        int reaction_state;
    }; /* +0x30 */
    int target_waypoint; /* +0x34 */
    int previous_waypoint; /* +0x38 */
    int step_direction; /* +0x3C */
    float speed; /* +0x40 */
} KonquestPathData;

typedef struct KonquestNpcEventDefinition {
    int script_function;
    float distance;
} KonquestNpcEventDefinition;

typedef struct KonquestTimedEvent KonquestTimedEvent;

typedef struct KonquestNpcData {
    char* model_name; /* +0x00 */
    char* texture_name; /* +0x04 */
    char* name; /* +0x08 */
    int visible_material_ids[15]; /* +0x0C, -1 terminated */
    KonquestTimedEvent* timed_events; /* +0x48 */
    Vec position; /* +0x4C */
    float angle_y; /* +0x58 */
    KonquestNpcEventDefinition events[8]; /* +0x5C */
    char pad9C[8];
    int idle_animation; /* +0xA4 */
    int walk_animation; /* +0xA8 */
    int run_animation; /* +0xAC */
    char padB0[4];
} KonquestNpcData;

typedef struct KonquestTileOrigin {
    char pad00[4];
    int loaded; /* +0x04 */
    Vec origin; /* +0x08 */
} KonquestTileOrigin;

typedef struct KonquestDialogAnimation KonquestDialogAnimation;

typedef struct KonquestNpcAnimState {
    MkHdr hdr;
    int state_08; /* +0x08 */
    MkObj* object; /* +0x0C */
    CollisionObj* editor_object; /* +0x10 */
    AniTextureControl* lip_texture; /* +0x14 */
    unsigned int lip_texture_instance; /* +0x18 */
    union {
        int dialog_anim;
        KonquestDialogAnimation* dialog_sequence;
    }; /* +0x1C */
    MkProc* proc; /* +0x20 */
    struct KonquestNpc* owner_npc; /* +0x24 */
    char pad28[4];
    unsigned int alpha; /* +0x2C */
} KonquestNpcAnimState;

typedef struct KonquestNpcEvent {
    unsigned int script_function;
    float distance;
    int enabled;
} KonquestNpcEvent;

typedef struct KonquestDoor {
    MkHdr hdr;
    char pad08[0x48];
    KonquestWaypoint* path_waypoints; /* +0x50 */
} KonquestDoor;

typedef union KonquestTime {
    struct {
        int year;
        int month;
        int day_of_month;
        int day_of_week;
        int hour;
        int minute;
    };
    int words[6];
} KonquestTime;

struct KonquestTimedEvent {
    KonquestTime time; /* +0x00 */
    unsigned int script_function; /* +0x18 */
    unsigned int event_slot_3_script; /* +0x1C */
    void* path; /* +0x20 */
    int path_id; /* +0x24 */
};

typedef struct KonquestRandomDialogSequence {
    int dialog;
    int animation;
} KonquestRandomDialogSequence;

struct KonquestDialogAnimation {
    int animation;
    unsigned int duration;
    float speed;
};

typedef struct KonquestTextureSearch {
    const char* root;
    unsigned int root_length;
    char texture_name[32];
} KonquestTextureSearch;

typedef struct AniData {
    char pad00[0x18];
    float last_frame; /* +0x18 */
} AniData;
typedef RpMaterial* (*KonquestMaterialCallback)(
    RpMaterial* material, void* data);

typedef struct KonquestNpc {
    MkHdr hdr;
    char* name; /* +0x08 */
    KonquestNpcData* data; /* +0x0C */
    KonquestPathData* path; /* +0x10 */
    KonquestNpcAnimState* animation; /* +0x14 */
    MkProc* proc; /* +0x18 */
    union {
        int flags; /* +0x1C */
        struct {
            union {
                unsigned char flags_1C;
                struct {
                    unsigned char flags_1C_pad_high : 1;
                    unsigned char reaction_active : 1;
                    unsigned char flags_1C_bit5 : 1;
                    unsigned char model_visible : 1;
                    unsigned char ignore_events : 1;
                    unsigned char flags_1C_pad_low : 3;
                };
            };
            union {
                unsigned char flags_1D;
                struct {
                    unsigned char flags_1D_pad_high : 3;
                    unsigned char skip_visibility : 1;
                    unsigned char flags_1D_pad_mid : 1;
                    unsigned char reaction_mode : 1;
                    unsigned char flags_1D_pad_low : 2;
                };
            };
            union {
                unsigned char timed_event_flags; /* +0x1E */
                struct {
                    unsigned char animation_override : 1;
                    unsigned char wait_for_animation : 1;
                    unsigned char state_change_pending : 1;
                    unsigned char reset_timed_events : 1;
                    unsigned char timed_event_pad_low : 4;
                };
            };
            unsigned char flags_1F;
        };
    };
    union {
        char runtime_state[0x20]; /* +0x20 */
        struct {
            unsigned int saved_object_flags; /* +0x20 */
            Vec saved_position; /* +0x24 */
            char pad30[8];
            float saved_gravity; /* +0x38 */
            float saved_animation_step; /* +0x3C */
        };
    };
    int tile_index; /* +0x40 */
    int data_table_index; /* +0x44 */
    Vec initial_position; /* +0x48 */
    float current_waypoint_angle; /* +0x54 */
    int state_58; /* +0x58 */
    AniData* queued_animation; /* +0x5C */
    float queued_animation_frame; /* +0x60 */
    unsigned int animation_flags; /* +0x64 */
    int conversation_count; /* +0x68 */
    int punch_count; /* +0x6C */
    float wait_ticks; /* +0x70 */
    float animation_speed; /* +0x74 */
    unsigned int saved_script_position; /* +0x78 */
    char saved_script_state[0x140]; /* +0x7C */
    unsigned int saved_script_stack_depth; /* +0x1BC */
    unsigned int saved_event_script; /* +0x1C0 */
    float camera_distance_squared; /* +0x1C4 */
    KonquestNpcEvent events[8]; /* +0x1C8 */
    KonquestTimedEvent* next_timed_event; /* +0x228 */
    KonquestTime next_event_time; /* +0x22C */
    KonquestTime wake_time; /* +0x244 */
    MkProc* turn_proc; /* +0x25C */
    unsigned int turn_proc_instance; /* +0x260 */
    int state_264; /* +0x264 */
    int state_268; /* +0x268 */
    int state_26C; /* +0x26C */
    float distance_270; /* +0x270 */
    float value_274; /* +0x274 */
    float distance_278; /* +0x278 */
    unsigned int art_id; /* +0x27C */
} KonquestNpc;

typedef int (*KonquestNpcEventCheckFn)(float distance);
typedef void (*KonquestNpcEventStateFn)(KonquestNpc* npc);

typedef struct KonquestNpcStateDef {
    KonquestNpcEventCheckFn check;
    KonquestNpcEventStateFn setup;
    KonquestNpcEventStateFn cleanup;
} KonquestNpcStateDef;

typedef struct KonquestNpcLoadConfig {
    char pad00[0x0C];
    char* art_section_name; /* +0x0C */
    char* animation_section_name; /* +0x10 */
    char pad14[0x18];
    char* string_bank_name; /* +0x2C */
    struct KonquestDialogDefinition* dialog_definitions; /* +0x30 */
} KonquestNpcLoadConfig;

typedef struct KonquestDialogDefinition {
    int string_id;
    int sound_id;
    LipSyncKeyframe* keyframes;
} KonquestDialogDefinition;

typedef struct KonquestNpcPdata {
    char pad00[0x24];
    ScriptSlot* waypoint_script; /* +0x24 */
    KonquestNpcLoadConfig* load_config; /* +0x28 */
    char pad2C[0x0C];
    int npc_count; /* +0x38 */
    MkPtr* npc_list; /* +0x3C */
    MkPtr* visible_npc_list; /* +0x40 */
    int dialog_ready; /* +0x44 */
    char pad48[0xB0];
    MkObj* monk; /* +0xF8 */
    unsigned int monk_instance; /* +0xFC */
    KonquestNpc* monk_npc; /* +0x100 */
    unsigned int monk_npc_instance; /* +0x104 */
    AnimPdata* monk_animation; /* +0x108 */
    char pad10C[4];
    int visible_tile_set; /* +0x110 */
    char pad114[8];
    int collision_mode; /* +0x11C */
    char pad120[0x0C];
    int attack_arg_a; /* +0x12C */
    int attack_arg_b; /* +0x130 */
    char pad134[4];
    KonquestTime current_time; /* +0x138 */
    char pad150[0xB0];
    int conversation_state_a; /* +0x200 */
    int conversation_state_b; /* +0x204 */
    int conversation_mode_a; /* +0x208 */
    int conversation_mode_b; /* +0x20C */
    int conversation_event; /* +0x210 */
    KonquestRandomDialogSequence random_dialog_sequences[20]; /* +0x214 */
    int random_dialog_sequence_count; /* +0x2B4 */
    char pad2B8[0x150];
    MkPtr* door_list; /* +0x408 */
    char pad40C[0x34];
    MkObj* camera_target; /* +0x440 */
    unsigned int camera_target_instance; /* +0x444 */
    KonquestNpc* hero_npc; /* +0x448 */
    unsigned int hero_npc_instance; /* +0x44C */
} KonquestNpcPdata;

typedef struct KonquestAnimations {
    char pad00[0x10];
    AniData* npc_attack; /* +0x10 */
} KonquestAnimations;

typedef struct NpcManagerPdata NpcManagerPdata;
typedef struct NpcManagerObject NpcManagerObject;
typedef struct NpcManagerModelSlot NpcManagerModelSlot;

typedef void (*NpcManagerDestroyFn)(
    NpcManagerPdata* manager, NpcManagerObject* object);

struct NpcManagerObject {
    char pad00[0x10];
    NpcManagerDestroyFn destroy; /* +0x10 */
};

struct NpcManagerModelSlot {
    unsigned int art_id; /* +0x00 */
    MkObj* object; /* +0x04 */
    AniTextureControl* lip_texture; /* +0x08 */
    unsigned int lip_texture_instance; /* +0x0C */
    KonquestNpcData* data; /* +0x10 */
    float age; /* +0x14 */
    int available; /* +0x18 */
    char name[0x40]; /* +0x1C */
};

struct NpcManagerPdata {
    NpcManagerObject* object; /* +0x00 */
    unsigned int object_instance; /* +0x04 */
    int visible_tile_set; /* +0x08 */
    int visible_count; /* +0x0C */
    int special_count; /* +0x10 */
    float nearest_npc_distance; /* +0x14 */
    KonquestNpc* visible_npcs[15]; /* +0x18 */
    NpcManagerModelSlot special_models[2]; /* +0x54 */
    NpcManagerModelSlot models[22]; /* +0x10C */
};

typedef struct KonquestTriggerData {
    char pad00[0x20];
    int source_type; /* +0x20 */
    KonquestNpc* source_npc; /* +0x24 */
} KonquestTriggerData;

typedef struct KonquestTrigger {
    char pad00[8];
    KonquestTriggerData* data; /* +0x08 */
} KonquestTrigger;

typedef struct AnimState AnimState;
typedef struct GroundCollTable {
    int bone;
    Vec offset;
    float radius;
} GroundCollTable;

typedef struct KonquestFlippedBoneMap {
    int count;
    int* bones;
} KonquestFlippedBoneMap;

typedef struct KonquestReactionPdata {
    MkHdr hdr;
    char pad08[4];
    MkObj* object; /* +0x0C */
    char pad10[0x10];
    MkProc* animation_proc; /* +0x20 */
    KonquestNpc* npc; /* +0x24 */
} KonquestReactionPdata;

typedef struct KonquestModelLoadPdata {
    MkHdr hdr;
    char pad08[0x1C];
    KonquestNpc* npc; /* +0x24 */
} KonquestModelLoadPdata;

typedef struct BloodFallObjectRef {
    MkObj* object;
    unsigned int instance;
} BloodFallObjectRef;

typedef struct BloodFallPdata {
    MkHdr hdr;
    float ground_y;
    BloodFallObjectRef objects[3];
    unsigned int emitters[3];
} BloodFallPdata;

typedef union BloodFallFloatBits {
    float value;
    unsigned int bits;
} BloodFallFloatBits;

typedef union KonquestSqrtBits {
    float value;
    unsigned int bits;
} KonquestSqrtBits;

typedef struct KonquestCameraPositionView {
    char pad00[0x40];
    float x;
    char pad44[4];
    float z;
} KonquestCameraPositionView;

typedef struct KonquestCameraView {
    char pad00[4];
    KonquestCameraPositionView* position;
} KonquestCameraView;

typedef struct KonquestNpcProcessPdata {
    MkHdr hdr;
    char pad08[4];
    int update_enabled; /* +0x0C */
    char pad10[0x14];
    KonquestNpc* npc; /* +0x24 */
} KonquestNpcProcessPdata;

typedef struct KonquestAnimPdata {
    MkHdr hdr;
    char pad08[0x24];
    AniData* animation; /* +0x2C */
    unsigned int flags; /* +0x30 */
    char pad34[4];
    float frame; /* +0x38 */
    float low_frame; /* +0x3C */
    float high_frame; /* +0x40 */
    float step; /* +0x44 */
    char pad48[8];
    Vec root_offset; /* +0x50 */
} KonquestAnimPdata;

typedef struct KonquestNpcProcSleepVtable {
    char pad00[0x18];
    void (*sleep)(void); /* +0x18 */
} KonquestNpcProcSleepVtable;

typedef struct KonquestProcDestroyVtable KonquestProcDestroyVtable;

struct KonquestProcDestroyVtable {
    char pad00[0x10];
    void (*destroy)(
        MkProc* proc, KonquestProcDestroyVtable* vtable); /* +0x10 */
};

typedef struct KonquestLipSyncPdata {
    MkHdr hdr;
    int mode; /* +0x08 */
    KonquestNpc* npc; /* +0x0C */
    union {
        AniTextureControl* texture;
        KonquestAnimPdata* animation;
    }; /* +0x10 */
    union {
        unsigned int texture_instance;
        char* animation_table;
    }; /* +0x14 */
    LipSyncKeyframe* keyframes; /* +0x18 */
    unsigned int sound_handle; /* +0x1C */
    float elapsed; /* +0x20 */
    int stop_requested; /* +0x24 */
} KonquestLipSyncPdata;

typedef struct KonquestWaypointScriptPdata {
    MkHdr hdr;
    unsigned int function_index; /* +0x08 */
    KonquestNpc* npc; /* +0x0C */
} KonquestWaypointScriptPdata;

typedef struct KonquestObjectScriptPdata {
    MkHdr hdr;
    int function_index;
    MkObj* object;
} KonquestObjectScriptPdata;

typedef struct KonquestBoneMatcher {
    MkHdr hdr;
    unsigned char flags_08;
    char pad09[3];
    float child_weight;
    MkObj* parent;
    unsigned int parent_instance;
    int parent_bone;
    Vec parent_offset;
} KonquestBoneMatcher;

typedef struct TurnAndFacePdata {
    MkHdr hdr;
    float angle; /* +0x08 */
    Vec position; /* +0x0C */
    int use_angle; /* +0x18 */
    int saved_pin_animation; /* +0x1C */
    int target_kind; /* +0x20 */
    union {
        KonquestNpc* npc;
        MkObj* object;
    } target; /* +0x24 */
} TurnAndFacePdata;

typedef union ObliqueMatrixCell {
    float value;
    unsigned int flags;
} ObliqueMatrixCell;

typedef union KonquestFloatBits {
    float value;
    unsigned int bits;
} KonquestFloatBits;

typedef struct KonquestNpcShadows {
    char pad00[0x10];
    Vec light_direction; /* +0x10 */
    char pad1C[4];
    MKMATRIX projection; /* +0x20 */
    MkObj* objects[15]; /* +0x60 */
    union {
        unsigned char flags; /* +0x9C */
        struct {
            signed char initialized : 1;
            signed char update_disabled : 1;
            signed char render_disabled : 1;
            signed char pad_flags : 5;
        };
    };
    char pad9D[3];
    int alpha; /* +0xA0 */
    float scales[15]; /* +0xA4 */
    RpMaterial* materials[15]; /* +0xE0 */
    RpAtomic* (*render)(
        RpAtomic* atomic, struct KonquestNpcShadows* shadows); /* +0x11C */
    int clear_alpha_pending; /* +0x120 */
    char pad124[0x0C];
} KonquestNpcShadows;

typedef ObliqueMatrixCell ObliqueMatrix[16];

typedef struct KonquestCmdScriptView {
    char pad00[0x14];
    unsigned int position;
    char pad18[8];
    int state;
    char pad24[0x40];
    char execution_state[0x140];
} KonquestCmdScriptView;

KonquestNpc* g_active_npc;
static NpcManagerPdata* npc_manager_pdata;
static int current_npc_count;
static int wait_ticks;
extern KonquestNpcPdata* konquest_pdata;
int konquest_human_bones[17] = {
    16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0
};
int _flipped_konquest_human_bones[16] = {
    0, 2, 1, 3, 5, 4, 6, 8, 7, 11, 10, 9, 13, 12, 15, 14
};
KonquestFlippedBoneMap flipped_konquest_human_bones = {
    16, _flipped_konquest_human_bones
};
GroundCollTable npc_ground_colls[3] = {
    {7, {0.0f, 0.0f, 0.0f}, 0.1f},
    {8, {0.0f, 0.0f, 0.0f}, 0.1f},
    {-1, {0.0f, 0.0f, 0.0f}, 0.0f},
};
extern GroundCollTable npc_punched_ground_colls[5];
int npc_fast_anims[175];
extern int konquest_editor_mode_on;
extern CameraObj* camera_obj;
extern void* Camera;
static KonquestNpcShadows npc_shadows;
extern KonquestAnimations konquest_animations;
extern KonquestNpcStateDef g_event_tbl[8];
extern MkFileEntry konquest_common_file_table[];
extern MkFileEntry kon_unique_npcs_file_table[];
extern MkFileInfo sec_konquest_common_art;
extern int konquest_npc_bones[];
extern float __float_max[];
extern float game_speed;
extern float inverse_game_speed;
static char* dialog_text;
extern float p_anim_idle(void);
void vdestroy_path_data_struct(KonquestPathData* path);
void vdestroy_konquest_npc_struct(KonquestNpc* npc);

MkVtable5 vtbl_konquest_npc_struct = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    (MkVtblFn)vdestroy_konquest_npc_struct,
};

MkVtable5 vtbl_path_data_struct = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    (MkVtblFn)vdestroy_path_data_struct,
};

KonquestTrigger* find_trigger_by_id(void);
void execute_trigger(KonquestTrigger* trigger);
void destroy_mkproc_nostack(MkProc* proc);
void set_root_and_obj_movement_weights(
    float root_weight, float object_weight, AnimState* animation);
void npc_play_dialog_and_anim_sequence(int dialog, int animation);
void npc_wait_for_dialog(void);
int konquest_set_dialog_text(
    const char* text, const LipSyncKeyframe* keyframes);
void npc_turn_and_face_angle(KonquestNpc* npc, float angle);
void nav_get_unit_vector_to_area(int area_index, Vec* out, Vec* position);
void nav_get_unit_vector_to_closest_area(Vec* out, Vec* position);
void nav_get_unit_vector_to_nav_portal(
    Vec* out, Vec* position, int area_index, int portal_id);
int nav_what_area_is_point_in(Vec* position, int hint_area);
int nav_which_area_is_next(int from_area, int to_area);
void resume_hero_state_process(void);
AniData* get_animation(int animation_id);
AniTextureControl* konquest_create_monk_face_ani_texture(MkObj* object);
void* get_data_table_by_name(const char* name);
void transition_to_anim_script(
    KonquestAnimPdata* animation, AniData* script, int flags, float blend);
void set_anim_script(
    KonquestAnimPdata* animation, AniData* script, int flags);
float anim_script_lastframe(AniData* script);
void npc_travel_path(int path_id, int path_arg, int travel_mode);
KonquestTileOrigin* get_nth_tile_struct(int index);
int get_tile_from_position(const Vec* position);
void npc_set_his_flags(KonquestNpcData* data, int flags, int enabled);
void* get_door_path(int door_id);
RpGeometry* RpGeometryForAllMaterials(
    RpGeometry* geometry, KonquestMaterialCallback callback, void* data);
int get_konquest_game_mode(void);
int is_game_mode_in_stack(int mode);
MkProc* create_mkproc_anim(
    int pid, MkProcEntryFn entry, AnimPdata** animation);
void npc_ani_1_frame(void);
void add_npc(KonquestNpcData* data);
void* memcpy(void* dst, const void* src, unsigned long size);
int check_skip_conversation_flag(void);
void snd_stop(unsigned int sound_handle);
unsigned int snd_req(int sound_id);
int mslSoundIsValid(unsigned int sound_handle);
float p_wait_for_dialog(void);
float p_do_lip_synch(void);
float p_npc_proc(void);
float p_npc_idle(void);
int is_time_a_greater_than_time_b(
    const KonquestTime* time_a, const KonquestTime* time_b);
int is_time_a_equal_to_time_b(
    const KonquestTime* time_a, const KonquestTime* time_b);
int is_valid_event_time(const KonquestTime* time);
int calc_next_occurrence_of_event(
    KonquestTime* result, const KonquestTime* event,
    const KonquestTime* current);
int does_event_a_trump_event_b(
    const KonquestTimedEvent* event_a,
    const KonquestTimedEvent* event_b);
KonquestTimedEvent* npc_which_event_is_more_recent(
    const KonquestTime* current, KonquestTimedEvent* event_a,
    KonquestTimedEvent* event_b);
void add_minutes_to_time(KonquestTime* time, int minutes);
void add_hours_to_time(KonquestTime* time, int hours);
void add_days_to_time(KonquestTime* time, int days);
void add_months_to_time(KonquestTime* time, int months);
void add_years_to_time(KonquestTime* time, int years);
RpMaterial* obj_find_material_by_id(MkObj* object, int material_id);
unsigned int fx_by_owner(const char* name, unsigned int owner);
void fx_set_param_v3(
    unsigned int handle, int parameter, float x, float y, float z);
void fx_restart_emit(unsigned int handle);
int is_blood_disabled(void);
unsigned int fx_next_emitter(unsigned int effect);
void fx_resume_emit(unsigned int effect);
void fx_reset_emit(unsigned int effect);
MkPfx* pfx_from_emitter(unsigned int effect);
int emitter_id_from_handle(unsigned int effect);
MkObj* get_mkobj_frame(int type, void* frame);
void insert_particle_mkobj(MkObj* object);
void get_bone_world_pos(MkObj* object, int bone, Vec* position);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
void obj_set_all_sobjs_priority(MkObj* object, int priority);
void build_bones_tbl(MkObj* object, int* bones, int flags);
MkObj* load_model_from_slot(int slot, unsigned int model_id, int heap_id);
void obj_create_sobjs(MkObj* object);
void insert_ground_me_mkobj(MkObj* object);
CollisionObj* add_shape_to_global_collision_list(
    const CollisionShape* shape, unsigned int flags);
void set_anim_script_frame(
    AnimPdata* animation, AniData* script, int flags, float frame);
void sobj_swap_material_texture(
    MkSobj* sobj, unsigned int material_id, RwTexture* texture);
void sobj_use_material_color(void* sobj);
void remove_fgnd_mkobj(void* object);
int advance_anim(AnimState* animation);
int pose_anim(AnimState* animation, int update_object);
void konquest_open_door_sobj(KonquestDoor* door, int remain_open);
void remove_npc(KonquestNpcData* data);
void get_visible_tile_set(int tile_set);
void npc_make_visible(KonquestNpc* npc);
void npc_make_invisible(KonquestNpc* npc);
void random_snd_req_delay(int group, int delay);
KonquestBoneMatcher* start_bone_matcher(
    float blend_ticks, MkObj* parent, int parent_bone,
    MkObj* child, int child_bone);

void npc_force_state_for_npc(KonquestNpc* npc, int event_index);
void npc_signal_event(KonquestNpc* npc, int event_index);
void npc_ani_to_frame_x(float frame);
void npc_ani_to_blend_frame(float blend_frames);
void npc_suspend_cmdscript(void);
void npc_switch_camera_focus(int focus_mode);
static void npc_set_state_for_npc(KonquestNpc* npc, int event_index);
/* Near match: 89.490425% - retail path ownership, special waypoint selectors,
 * nearest-waypoint search, position synchronization, and waypoint-script
 * process creation are recovered. Remaining differences are typed pointer
 * truth lowering, nonvolatile save form, and local scheduling. */
static void npc_set_path(
    KonquestNpc* npc, void* path, int table_index, int row_count, int flags,
    int travel_mode);
/*
 * Soft ceiling: 84.0% - the body is exact; MWCC splits retail's r30-r31
 * stmw/lmw save and restore sequences.
 */
static RpMaterial* MaterialFindTextureWithRootString(
    RpMaterial* material, void* root_string);
static RpMaterial* hide_npc_materials(
    RpMaterial* material, void* material_ids);
void npc_shadow_update(void);
static void set_shadow_bones(
    MkObj* shadow, MkObj* source, float scale);
static void append_oblique_projection(
    ObliqueMatrix result, ObliqueMatrix left, ObliqueMatrix right);
static float p_update_npc_shadows(void);
static float p_npc_waypoint_script(void);
static float p_npc_load_model(void);
static void npc_resolve_events(KonquestNpc* npc);
float p_npc_manager(void);
void npc_update(int update_all);
static RpAtomic* shadow_render_callback(RpAtomic* atomic);
static void npc_invisible_update(KonquestNpc* npc);
static void npc_pre_wake(void);
static void npc_post_sleep(void);
static void npc_dispatch_timed_events_for_all_npcs(void);
static int npc_check_visibility_and_calc_dist(KonquestNpc* npc);
static void npc_manager_release_npc_model(KonquestNpc* npc);
static void npc_manager_load_new_npc_model(KonquestNpc* npc);
static void npc_manager_find_model_for_npc(KonquestNpc* npc);
static int is_it_safe_to_make_this_npc_visible(KonquestNpc* npc);
static RpAtomic* AtomicFindTextureWithRootString(
    RpAtomic* atomic, void* root_string);
static void material_restore_texture_pointer(RpMaterial* material);
static void material_store_texture_pointer(RpMaterial* material);
static void setup_current_and_next_events(KonquestNpc* npc, int initialize);
static void npc_check_next_event(KonquestNpc* npc);
static void npc_setup_path_for_event(
    KonquestNpc* npc, KonquestTimedEvent* event, int preserve_path);
static inline int npc_event_has_active_animation(KonquestNpc* npc) {
    KonquestNpcAnimState* state = npc->animation;
    int active;

    if (state == 0) {
        active = 0;
    } else if (state->object == 0) {
        active = 0;
    } else {
        active = state->proc != 0;
    }
    return active;
}

static inline void npc_set_event_script(
    KonquestNpc* npc, int event_index, unsigned int script_function) {
    if (npc == 0) {
        char message[256];

        sprintf(
            message,
            "NP: %s -- npc_set_event_script -- Couldn't find npc",
            "(null)");
    } else {
        KonquestNpcEvent* event = &npc->events[event_index];

        event->script_function = script_function;
        event->enabled = 1;
        event->distance = 0.0f;
        if (npc_event_has_active_animation(npc) != 0) {
            MkObj* object = npc->animation->object;

            object->pos_vel.z = 0.0f;
            npc->animation->object->pos_vel.y = 0.0f;
            npc->animation->object->pos_vel.x = 0.0f;
            npc->animation->object->ang_vel.z = 0.0f;
            npc->animation->object->ang_vel.y = 0.0f;
            npc->animation->object->ang_vel.x = 0.0f;
        }
    }
}
static void load_model_for_npc(KonquestNpc* npc);
static void npc_notify_nearby_npcs_that_player_hit_someone(KonquestNpc* npc);
static float p_hero_turn_and_face(void);
static float p_blood_fall_control(void);

static inline int npc_is_visible_model_active(KonquestNpc* npc) {
    unsigned char flags = npc->flags_1C;

    if ((flags & 0x40) != 0) {
        return 0;
    }
    if ((flags & 0x80) != 0) {
        return 0;
    }
    if ((flags & 4) != 0) {
        return 0;
    }
    if ((flags & 0x10) == 0) {
        return 0;
    }
    return 1;
}

static inline float npc_fast_sqrt(float value) {
    KonquestSqrtBits estimate;
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
    /* The retail table is packed and the masked value is a byte offset. */
    estimate.bits =
        *(unsigned short*)((unsigned char*)GXMathSqrtTable + table_index) << 8;
    estimate.bits |= exponent;
    return 0.5f *
        (estimate.value *
         (3.0f - (estimate.value * estimate.value) / value));
}

static inline KonquestNpc* npc_find_by_data_inline(
    KonquestNpcData* data) {
    KonquestNpc* candidate;
    KonquestNpc* result;
    MkPtr* link;

    candidate = konquest_pdata->monk_npc;
    if (candidate != 0) {
        if (candidate->hdr.instance != konquest_pdata->monk_npc_instance) {
            candidate = 0;
        }
    } else {
        candidate = 0;
    }
    if (candidate != 0 && candidate->data == data) {
        return candidate;
    }

    candidate = konquest_pdata->hero_npc;
    if (candidate != 0) {
        if (candidate->hdr.instance != konquest_pdata->hero_npc_instance) {
            candidate = 0;
        }
    } else {
        candidate = 0;
    }
    if (candidate != 0 && candidate->data == data) {
        return candidate;
    }

    result = 0;
    if (&konquest_pdata->npc_list != 0) {
        link = konquest_pdata->npc_list;
        while (link != 0) {
            KonquestNpc* linked_npc = (KonquestNpc*)link->hdr;

            if (link->instance != linked_npc->hdr.instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else if (linked_npc->data == data) {
                result = linked_npc;
                break;
            } else {
                link = link->next;
            }
        }
    }
    return result;
}

static inline KonquestNpc* npc_find_by_table_index_inline(int table_index) {
    KonquestNpc* npc = konquest_pdata->monk_npc;
    MkPtr* link;

    if (npc != 0) {
        if (npc->hdr.instance != konquest_pdata->monk_npc_instance) {
            npc = 0;
        }
    } else {
        npc = 0;
    }
    if (npc != 0 && npc->data_table_index == table_index) {
        return npc;
    }

    npc = 0;
    if (&konquest_pdata->npc_list != 0) {
        link = konquest_pdata->npc_list;
        while (link != 0) {
            KonquestNpc* linked_npc = (KonquestNpc*)link->hdr;

            if (link->instance != linked_npc->hdr.instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else if (linked_npc->data_table_index == table_index) {
                npc = linked_npc;
                break;
            } else {
                link = link->next;
            }
        }
    }
    return npc;
}

/* Shared inline camera-focus sequence used by the standalone script entry and
 * conversation camera transitions. */
static inline void npc_switch_camera_focus_inline(int focus_mode) {
    MkObj* focus;

    camera_set_movement_rate(0.0f);
    camera_set_rotation_rate(0.0f);
    switch (focus_mode) {
    case 2:
        focus = konquest_pdata->monk;
        if (focus != 0) {
            if (focus->hdr.instance != konquest_pdata->monk_instance) {
                focus = 0;
            }
        } else {
            focus = 0;
        }
        camera_set_lookat_focus(focus);
        camera_set_movement_focus_obj(g_active_npc->animation->object);
        break;
    case 0:
        camera_set_lookat_focus(g_active_npc->animation->object);
        focus = konquest_pdata->monk;
        if (focus != 0) {
            if (focus->hdr.instance != konquest_pdata->monk_instance) {
                focus = 0;
            }
        } else {
            focus = 0;
        }
        camera_set_movement_focus_obj(focus);
        break;
    case 1:
        focus = konquest_pdata->camera_target;
        if (focus != 0) {
            if (focus->hdr.instance !=
                konquest_pdata->camera_target_instance) {
                focus = 0;
            }
        } else {
            focus = 0;
        }
        camera_set_lookat_focus(focus);
        camera_set_movement_focus_obj(g_active_npc->animation->object);
        break;
    }
    camera_set_glitch_flag();
}

static inline int npc_is_same(
    KonquestNpc* left, KonquestNpc* right) {
    if (left == right) {
        return 1;
    }
    return 0;
}

static inline int npc_next_waypoint(KonquestNpc* npc) {
    KonquestPathData* path = npc->path;
    int waypoint_count;
    int next;

    if (path->waypoints == 0) {
        next = 0;
    } else {
        waypoint_count = path->waypoint_count;
        if (waypoint_count == 1) {
            next = 0;
        } else {
            next = path->current_waypoint;
            if (path->step_direction == 1) {
                next++;
                if (next >= waypoint_count) {
                    if (path->destination_type == 6) {
                        next = 0;
                    } else {
                        path->step_direction = -1;
                        next = npc->path->current_waypoint - 1;
                    }
                }
            } else {
                next--;
                if (next < 0) {
                    if (path->destination_type == 6) {
                        next = waypoint_count - 1;
                    } else {
                        next = 1;
                        path->step_direction = 1;
                    }
                }
            }
        }
    }
    return next;
}

static inline int npc_nav_area_hint(int area) {
    unsigned int value = (unsigned int)area;

    return (int)(value & ((value >> 31) - 1));
}

static inline void npc_get_path_target(KonquestNpc* npc, Vec* target) {
    if (npc != 0) {
        if (npc->path != 0) {
            if ((unsigned int)(
                    npc->path->destination_type + (int)0x80000000) == 7) {
                target->x = npc->path->destination.x;
                target->y = npc->path->destination.y;
                target->z = npc->path->destination.z;
            } else if (npc->path->waypoints == 0) {
                target->x = npc->initial_position.x;
                target->y = npc->initial_position.y;
                target->z = npc->initial_position.z;
            } else {
                target->x = npc->path
                                ->waypoints[npc->path->target_waypoint]
                                .position.x;
                target->y = npc->path
                                ->waypoints[npc->path->target_waypoint]
                                .position.y;
                target->z = npc->path
                                ->waypoints[npc->path->target_waypoint]
                                .position.z;
            }
        }
        target->y = 0.0f;
    }
}

static inline void npc_start_current_waypoint_script(KonquestNpc* npc) {
    if (npc->path->waypoints != 0) {
        if (npc->path->waypoints[npc->path->current_waypoint]
                .script_function != 0) {
            KonquestWaypointScriptPdata* pdata;
            MkProc* proc = _create_mkproc_generic_tinystack(
                0xA017, 0x1F, p_npc_waypoint_script,
                sizeof(*pdata), (MkHdr**)&pdata);

            if (proc != 0) {
                set_process_as_scriptable(proc);
                pdata->function_index =
                    npc->path->waypoints[npc->path->current_waypoint]
                        .script_function;
                pdata->npc = npc;
            }
        }
    }
}

static inline Vec* npc_get_navigation_direction(
    KonquestNpc* npc, Vec* direction) {
    int current_area;
    int destination_area = npc->state_26C;

    if (npc->path->current_waypoint != -1) {
        return 0;
    }
    current_area = nav_what_area_is_point_in(
        &npc->data->position, npc_nav_area_hint(npc->state_268));
    if (current_area >= 0) {
        if (npc->state_264 < 0) {
            npc->state_264 = current_area;
        } else if (npc->state_264 != npc->state_268) {
            npc->state_264 = npc->state_268;
        }
        npc->state_268 = current_area;
    }
    if (destination_area < 0) {
        return 0;
    }
    if (current_area < 0) {
        if (npc->state_264 >= 0) {
            nav_get_unit_vector_to_area(
                npc->state_264, direction, &npc->data->position);
        } else {
            nav_get_unit_vector_to_closest_area(
                direction, &npc->data->position);
        }
        return direction;
    }
    {
        int next_area = nav_which_area_is_next(
            current_area, destination_area);

        if (current_area != next_area) {
            nav_get_unit_vector_to_nav_portal(
                direction, &npc->data->position, current_area, next_area);
            return direction;
        }
    }
    return 0;
}

static inline void npc_suspend_animation_wait(void) {
    KonquestCmdScriptView* script;
    CmdScript* saved_script;
    KonquestNpc* npc = g_active_npc;

    if (npc == 0) {
        return;
    }
    npc->wait_ticks -= 1.0f;
    if (npc->wait_ticks <= 0.0f) {
        npc->wait_ticks = 0.0f;
        return;
    }
    script = (KonquestCmdScriptView*)active_cmdscript;
    script->state = 2;
    saved_script = active_cmdscript;
    npc = g_active_npc;
    cmdscript_step_backward();
    memcpy(
        npc->saved_script_state,
        ((KonquestCmdScriptView*)active_cmdscript)->execution_state,
        sizeof(npc->saved_script_state));
    npc->saved_script_position =
        ((KonquestCmdScriptView*)active_cmdscript)->position;
    npc->saved_script_stack_depth = get_script_stack_depth();
    active_cmdscript = saved_script;
}

/* Near match: 78.13265% and four bytes from retail. Both special and regular
 * model-slot ownership paths, destruction, alpha teardown, and slot resets
 * match; residue is branch scheduling and GPR save form. */
static inline void npc_manager_remove_visible_entry(KonquestNpc* npc) {
    int index;

    for (index = 0; index < 15; index++) {
        if (npc_manager_pdata->visible_npcs[index] == npc) {
            npc_manager_pdata->visible_npcs[index] = 0;
            npc_manager_pdata->visible_count--;
            if (npc->data->events[7].script_function == 1) {
                npc_manager_pdata->special_count--;
            }
            npc->model_visible = 0;
            return;
        }
    }
}static inline void npc_manager_setup_model(MkObj* object) {
    MkSobj* sobj;

    obj_create_sobjs(object);
    sobj = (MkSobj*)obj_first_sobj(object);
    if (sobj != 0) {
        sobj->flags09_bits.bit4 = 0;
        sobj->flags09_bits.bit2 = 0;
    }
    obj_set_all_sobjs_priority(object, 0x13);
    obj_for_all_atomics_set_material_alpha(object, 0);
    if (sobj->atomic->geometry != 0) {
        RpGeometryForAllMaterials(
            sobj->atomic->geometry,
            (KonquestMaterialCallback)material_store_texture_pointer, 0);
    }
    object->flags_08_bits.airborne = 1;
    object->flags_08_bits.angular_velocity_enabled = 1;
    build_bones_tbl(object, konquest_human_bones, 1);
    object->flipped_bone_map = &flipped_konquest_human_bones;
    object->flags_09_bits.launched = 1;
    object->flags_09_bits.bit6 = 1;
    object->ground_colls = npc_ground_colls;
    object->light_flags = 1;
}

static inline unsigned int npc_manager_find_mouth_art_id(MkObj* object) {
    KonquestTextureSearch search;

    search.texture_name[0] = 0;
    search.root_length = 10;
    search.root = "kon_mouth0";
    RpClumpForAllAtomics(
        object->clump, AtomicFindTextureWithRootString, &search);
    if (search.texture_name[0] != 0) {
        strupr(search.texture_name);
        return get_artid_of_named_item_in_slot(
            0x6002B, search.texture_name, 0);
    }
    return 0;
}

static inline void npc_copy_time(
    KonquestTime* destination, const KonquestTime* source) {
    *destination = *source;
}

static inline void npc_find_next_timed_event(KonquestNpc* npc) {
    KonquestTimedEvent* event = npc->data->timed_events;
    int count = get_row_count_for_table_by_pointer(
        konquest_pdata->waypoint_script, event);
    const KonquestTime* current = &konquest_pdata->current_time;
    KonquestTime next_time;
    KonquestTime previous_next_time;

    if (npc->next_timed_event != 0) {
        npc->next_timed_event = 0;
        npc_copy_time(&previous_next_time, &npc->next_event_time);
        current = &previous_next_time;
    }
    while (count-- != 0) {
        if (is_valid_event_time(&event->time) != 0 &&
            calc_next_occurrence_of_event(
                &next_time, &event->time, current) != 0) {
            if (npc->next_timed_event == 0) {
                npc->next_timed_event = event;
                npc_copy_time(&npc->next_event_time, &next_time);
            } else if (is_time_a_greater_than_time_b(
                           &next_time, &npc->next_event_time) == 0 &&
                       (is_time_a_equal_to_time_b(
                            &next_time, &npc->next_event_time) == 0 ||
                        does_event_a_trump_event_b(
                            event, npc->next_timed_event) != 0)) {
                npc->next_timed_event = event;
                npc_copy_time(&npc->next_event_time, &next_time);
            }
        }
        event++;
    }
}

void npc_sleep_until_model_loaded(void) {
    KonquestCmdScriptView* script;
    CmdScript* saved_script;
    KonquestNpc* npc;

    npc = g_active_npc;
    if (npc == 0 || aproc->pid != 0xA014) {
        return;
    }

    npc->wait_ticks = 2.0f;
    npc = g_active_npc;
    if (npc == 0) {
        return;
    }

    npc->wait_ticks -= 1.0f;
    if (npc->wait_ticks <= 0.0f) {
        npc->wait_ticks = 0.0f;
        return;
    }

    script = (KonquestCmdScriptView*)active_cmdscript;
    script->state = 2;
    saved_script = active_cmdscript;
    npc = g_active_npc;
    cmdscript_step_backward();
    memcpy(
        npc->saved_script_state,
        ((KonquestCmdScriptView*)active_cmdscript)->execution_state,
        sizeof(npc->saved_script_state));
    npc->saved_script_position =
        ((KonquestCmdScriptView*)active_cmdscript)->position;
    npc->saved_script_stack_depth = get_script_stack_depth();
    active_cmdscript = saved_script;
}

void cleanup_npc_manager(void) {
    if (npc_manager_pdata != 0) {
        if (npc_manager_pdata->object_instance != 0) {
            NpcManagerObject* object = npc_manager_pdata->object;

            object->destroy(npc_manager_pdata, object);
        }
        npc_manager_pdata = 0;
    }
    g_active_npc = 0;
}

/* Near match: 87.696075%, 16 bytes over retail. Validated hero-NPC reuse,
 * construction, Damashi data/position state, visibility flags, scriptable
 * process, and animation-process ownership are exact; residue is latch/GPR
 * lowering and nonvolatile save form. */
void make_damashi_npc(MkObj* object) {
    KonquestNpc* npc = konquest_pdata->hero_npc;

    if (npc != 0) {
        if (npc->hdr.instance != konquest_pdata->hero_npc_instance) {
            npc = 0;
        }
    } else {
        npc = 0;
    }
    if (npc == 0) {
        npc = (KonquestNpc*)get_mkhdr(
            &vtbl_konquest_npc_struct, sizeof(*npc));
        if (npc != 0) {
            zero_pdata_payload(sizeof(*npc), &npc->hdr);
            npc->state_268 = -3;
            npc->state_264 = -3;
            npc->state_26C = -3;
            npc->distance_270 = __float_max[0];
            npc->value_274 = 0.0f;
            npc->distance_278 = __float_max[0];
        }
        npc->data = 0;
        npc->path = 0;
        npc->animation = 0;
        npc->proc = 0;
        npc->state_58 = 0;
        npc->queued_animation = 0;
        npc->queued_animation_frame = 0.0f;
        npc->animation_flags = 0;
        npc->camera_distance_squared = 0.0f;
        npc->conversation_count = 0;
        npc->punch_count = 0;
        npc->saved_script_stack_depth = 0;
        npc->wait_ticks = 0.0f;
        npc->animation_speed = 1.0f;
        npc->turn_proc = 0;
        npc->turn_proc_instance = 0;
        npc->flags = 0;
        memset(npc->runtime_state, 0, sizeof(npc->runtime_state));
        npc->saved_animation_step = 1.0f;
        npc->data =
            (KonquestNpcData*)get_data_table_by_name("damashi_npc");
        if (npc->data == 0) {
            return;
        }
        npc->name = "damashi_npc";
        konquest_pdata->hero_npc = npc;
        konquest_pdata->hero_npc_instance = npc->hdr.instance;
        npc->data->position.x = object->pos.x;
        npc->data->position.y = object->pos.y;
        npc->data->position.z = object->pos.z;
        npc->tile_index = get_tile_from_position(&object->pos);
        if (npc_event_has_active_animation(npc) != 0) {
            npc->animation->object->flags_09_bits.bit6 = 0;
            npc->animation->object->pos.x = object->pos.x;
            npc->animation->object->pos.y = object->pos.y;
            npc->animation->object->pos.z = object->pos.z;
        }
        npc->data->angle_y = 0.0f;
        npc->flags_1C |= 0x10;
        npc->flags_1D |= 2;
        npc->flags_1C |= 0x40;
        npc->timed_event_flags |= 0x40;
        npc->saved_object_flags = 0;
        npc->saved_gravity = 0.0f;
    }

    if (npc->animation == 0) {
        KonquestNpcAnimState* animation = 0;
        MkProc* proc = _create_mkproc_generic_bigstack(
            0xA002, 8, p_npc_idle, sizeof(*animation),
            (MkHdr**)&animation);

        if (proc != 0) {
            AnimPdata* animation_pdata = 0;

            set_process_as_scriptable(proc);
            proc->pre_destroy = (MkProcCallbackFn)npc_pre_wake;
            proc->destroy_cb = (MkProcCallbackFn)npc_post_sleep;
            npc->animation = animation;
            npc->proc = proc;
            animation->alpha = 0xFF;
            animation->lip_texture = 0;
            animation->lip_texture_instance = 0;
            animation->owner_npc = npc;
            animation->proc = 0;
            animation->object = object;
            animation->editor_object = 0;
            animation->dialog_anim = 0;
            animation->proc = create_mkproc_anim(
                0x5002, p_anim_idle, &animation_pdata);
            animation_pdata->obj = object;
            animation_pdata->obj_instance = object->hdr.instance;
        }
    }
}

/*
 * Soft ceiling: 92.552635% - both cmdscript suspension and process-sleep paths
 * are exact; fixed-copy scheduling, save form, and float relocations remain.
 */
void npc_wait_for_wake_up(void) {
    if (aproc->pid == 0xA014) {
        if (is_time_a_greater_than_time_b(
                &konquest_pdata->current_time,
                &g_active_npc->wake_time) == 0) {
            KonquestCmdScriptView* script;
            CmdScript* saved_script;
            KonquestNpc* npc;

            g_active_npc->wait_ticks = 2.0f;
            npc = g_active_npc;
            if (npc == 0) {
                return;
            }
            npc->wait_ticks -= 1.0f;
            if (npc->wait_ticks <= 0.0f) {
                npc->wait_ticks = 0.0f;
                return;
            }
            script = (KonquestCmdScriptView*)active_cmdscript;
            script->state = 2;
            saved_script = active_cmdscript;
            npc = g_active_npc;
            cmdscript_step_backward();
            memcpy(
                npc->saved_script_state,
                ((KonquestCmdScriptView*)active_cmdscript)->execution_state,
                sizeof(npc->saved_script_state));
            npc->saved_script_position =
                ((KonquestCmdScriptView*)active_cmdscript)->position;
            npc->saved_script_stack_depth = get_script_stack_depth();
            active_cmdscript = saved_script;
        }
    } else {
        while (is_time_a_greater_than_time_b(
                   &konquest_pdata->current_time,
                   &g_active_npc->wake_time) == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/*
 * Soft ceiling: 82.921875% - the typed body is exact; MWCC uses split
 * saves/restores and different fixed-size-copy/GPR scheduling from retail.
 */
void npc_set_wake_up_time(int unit, int amount) {
    if (is_time_a_greater_than_time_b(
            &konquest_pdata->current_time,
            &g_active_npc->wake_time) != 0) {
        memcpy(
            &g_active_npc->wake_time, &konquest_pdata->current_time,
            sizeof(KonquestTime));
        switch (unit) {
        case 0:
            add_minutes_to_time(&g_active_npc->wake_time, amount);
            return;
        case 1:
            add_hours_to_time(&g_active_npc->wake_time, amount);
            return;
        case 2:
            add_days_to_time(&g_active_npc->wake_time, amount);
            return;
        case 3:
            add_months_to_time(&g_active_npc->wake_time, amount);
            return;
        case 4:
            add_years_to_time(&g_active_npc->wake_time, amount);
            break;
        }
    }
}

void npc_set_my_conversation_counter(int count) {
    if (g_active_npc != 0) {
        g_active_npc->conversation_count = count;
    }
}

void npc_set_his_conversation_counter(
    KonquestNpcData* data, int count) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (npc == 0) {
        char message[256];

        sprintf(
            message,
            "NPC_ID: %s -- npc_set_his_conversation_counter -- Couldn't find npc",
            data->name);
        return;
    }
    npc->conversation_count = count;
}

void npc_set_my_punch_counter(int count) {
    if (g_active_npc != 0) {
        g_active_npc->punch_count = count;
    }
}

void npc_set_his_punch_counter(KonquestNpcData* data, int count) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (npc == 0) {
        char message[256];

        sprintf(
            message,
            "NPC_ID: %s -- npc_set_his_punch_counter -- Couldn't find that npc",
            data->name);
        return;
    }
    npc->punch_count = count;
}

/*
 * Soft ceiling: 82.560974% - the body is exact; the zero-vector relocation
 * label and split r29-r31 save/restore form differ.
 */
void npc_start_fx_at_his_position(
    KonquestNpcData* data, const char* effect_name, const Vec* offset) {
    Vec position = {0.0f, 0.0f, 0.0f};

    if (data != 0) {
        unsigned int effect = fx_by_owner(effect_name, 4);

        if (effect != 0) {
            position.x = data->position.x + offset->x;
            position.y = data->position.y + offset->y;
            position.z = data->position.z + offset->z;
            fx_set_param_v3(
                effect, 0x202, position.x, position.y, position.z);
            fx_restart_emit(effect);
        }
    }
}

/*
 * Soft ceiling: 86.333336% - the shared lookup and active-animation checks
 * are exact; only latch branches and nonvolatile-register coloring differ.
 */
MkObj* npc_get_obj(KonquestNpcData* data) {
    KonquestNpc* npc = npc_find_by_data_inline(data);
    KonquestNpcAnimState* state;
    int has_active_animation;

    if (npc == 0) {
        return 0;
    }
    state = npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        return state->object;
    }
    return 0;
}

/* Near match: 90.41441% - effect/process setup, three latched objects, bone
 * placement, normalized randomized velocities, and emitter ownership match.
 * Remaining differences are loop cursor/register allocation and save form. */
void npc_start_blood_fall(void) {
    BloodFallPdata* pdata;
    MkProc* proc;
    MkPfx* particle;
    unsigned int effect;
    int index;

    if (is_blood_disabled() != 0) {
        return;
    }
    effect = fx_by_owner("bleed_trails", 4);
    if (effect == 0) {
        return;
    }
    particle = pfx_from_emitter(effect);
    if (particle == 0) {
        return;
    }
    proc = _create_mkproc_generic_nostack(
        0x601B, 0x1F, p_blood_fall_control, sizeof(*pdata),
        (MkHdr**)&pdata);
    if (proc == 0) {
        return;
    }
    zero_pdata_payload(sizeof(*pdata), &pdata->hdr);
    if (effect == 0) {
        if (proc->instance != 0) {
            proc->vtbl->destroy(proc);
        }
        return;
    }
    pdata->ground_y = g_active_npc->animation->object->ground_colls_y;

    for (index = 0; index < 3; index++) {
        MkObj* monk = konquest_pdata->monk;
        MkObj* object;
        float length_squared;
        float inverse_length;

        if (monk != 0) {
            if (monk->hdr.instance != konquest_pdata->monk_instance) {
                monk = 0;
            }
        } else {
            monk = 0;
        }
        if (monk == 0) {
            return;
        }
        object = get_mkobj_frame(0x6015, 0);
        if (object == 0) {
            return;
        }
        effect = fx_next_emitter(effect);
        if (effect == 0) {
            if (object->hdr.instance != 0) {
                object->hdr.typed_vtbl->destroy(&object->hdr);
            }
            return;
        }
        pdata->objects[index].object = object;
        pdata->objects[index].instance = object->hdr.instance;
        pdata->emitters[index] = effect;
        fx_resume_emit(effect);
        pfx_bind_emitter_num_to_obj(
            particle, object, 0, emitter_id_from_handle(effect));
        insert_particle_mkobj(object);
        object->flags_08_bits.airborne = 1;
        object->flags_08_bits.gravity_enabled = 1;
        get_bone_world_pos(
            g_active_npc->animation->object, 10, &object->pos);
        object->pos_vel.x =
            g_active_npc->animation->object->pos.x - monk->pos.x;
        object->pos_vel.z =
            g_active_npc->animation->object->pos.z - monk->pos.z;
        rotate_xz(
            &object->pos_vel, &object->pos_vel,
            sfrand(0.7853982f));
        object->pos_vel.y = 1.0f;
        length_squared =
            object->pos_vel.z * object->pos_vel.z +
            (object->pos_vel.x * object->pos_vel.x +
             object->pos_vel.y * object->pos_vel.y);
        inverse_length = 0.0f;
        if (length_squared > 0.0f) {
            BloodFallFloatBits estimate;
            float product;
            float correction;

            estimate.value = length_squared;
            estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
            product = estimate.value * (length_squared * estimate.value);
            correction = 3.0f - product;
            inverse_length =
                0.0625f * estimate.value * correction *
                -((correction * (product * correction)) - 12.0f);
        }
        object->pos_vel.x *= inverse_length;
        object->pos_vel.y *= inverse_length;
        object->pos_vel.z *= inverse_length;
        object->pos_vel.x *= 0.02f + frand(0.05f);
        object->pos_vel.z *= 0.02f + frand(0.05f);
        object->pos_vel.y *= 0.03f + frand(0.03f);
    }
}

/* Near match: 92.27358% - fall integration, splat creation, emitter reset,
 * destruction, and completion return policy match retail. Remaining residue
 * is register allocation, constant lifetime, and save-set selection. */
static float p_blood_fall_control(void) {
    BloodFallPdata* pdata = (BloodFallPdata*)pdata_of_proc(aproc);
    int no_objects = 1;
    int index;

    for (index = 0; index < 3; index++) {
        MkObj* object = pdata->objects[index].object;

        if (object != 0) {
            if (object->hdr.instance != pdata->objects[index].instance) {
                object = 0;
            }
        } else {
            object = 0;
        }
        if (object != 0) {
            no_objects = 0;
            if (object->pos.y > pdata->ground_y) {
                object->pos_vel.y -= 0.003f;
            } else {
                unsigned int splat_effect;

                object->pos.y = pdata->ground_y;
                splat_effect = fx_by_owner("blood_splat", 4);
                if (splat_effect != 0) {
                    MkPfx* particle = pfx_from_emitter(splat_effect);
                    MkObj* splat =
                        (MkObj*)pfx_get_emitter_obj(particle, 0);

                    if (splat == 0) {
                        splat = (MkObj*)pfx_bind_emitter_num_to_new_obj(
                            particle, (void*)0x6015, 0);
                        splat->flags_08_bits.airborne = 1;
                    }
                    y_angle_to_MKMATRIX(
                        &splat->frame->modelling,
                        frand(6.2831855f));
                    splat->pos.x = object->pos.x;
                    splat->pos.y = object->pos.y;
                    splat->pos.z = object->pos.z;
                    update_mkobj(
                        splat != 0 ? as_mkhdr(&splat->hdr) : 0);
                    fx_restart_emit(splat_effect);
                }
                fx_reset_emit(pdata->emitters[index]);
                if (object->hdr.instance != 0) {
                    object->hdr.typed_vtbl->destroy(&object->hdr);
                }
                return 1.0f;
            }
        }
    }
    return no_objects != 0 ? -1.0f : 1.0f;
}

/*
 * Soft ceiling: 88.45238% - the body is exact; the zero-vector relocation
 * label and split r30-r31 save/restore form differ.
 */
void npc_start_fx_at_position(
    const char* effect_name, const Vec* offset) {
    Vec position = {0.0f, 0.0f, 0.0f};

    if (g_active_npc != 0) {
        unsigned int effect = fx_by_owner(effect_name, 4);
        KonquestNpcData* data = g_active_npc->data;

        position.x = data->position.x + offset->x;
        position.y = data->position.y + offset->y;
        position.z = data->position.z + offset->z;
        fx_set_param_v3(
            effect, 0x202, position.x, position.y, position.z);
        fx_restart_emit(effect);
    }
}

void npc_reset_my_timed_events(void) {
    if (g_active_npc != 0) {
        g_active_npc->reset_timed_events = 1;
    }
}

/* Near match: 80.588234%, 16 bytes over retail. All operations and branches
 * are exact; MWCC emits split r29-r31 saves/restores instead of stmw/lmw. */
void npc_reset_all_timed_events(void) {
    MkPtr** list = &konquest_pdata->npc_list;
    MkPtr* link;

    if (list != 0) {
        link = *list;
        while (link != 0) {
            KonquestNpc* npc;

            npc = (KonquestNpc*)link->hdr;
            if (link->instance != npc->hdr.instance) {
                MkPtr* next;

                next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (npc != 0) {
                    npc->reset_timed_events = 1;
                }
                link = link->next;
            }
        }
    }
}

/* Soft ceiling: 95.6579% - exact body and size; zero-float relocation and
 * equivalent latch branch scheduling remain. */
void npc_switch_camera_focus(int focus_mode) {
    npc_switch_camera_focus_inline(focus_mode);
}

/* Matching: retail uses the compact save/restore form selected by the local
 * size mode; the typed queued/live animation paths compile exactly. */
#pragma optimize_for_size on
void npc_glitch_to_ani(int animation_id, int flags) {
    AniData* animation;
    KonquestNpc* npc;

    npc = g_active_npc;
    animation = get_animation(animation_id);

    if (aproc->pid == 0xA014) {
        npc->queued_animation = animation;
        npc->animation_flags = flags;
        npc->queued_animation_frame = 0.0f;
    } else if (npc->animation != 0) {
        KonquestAnimPdata* animation_pdata =
            (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);

        set_anim_script(animation_pdata, animation, flags);
    }
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: 97.58242% - lookup, animation selection, queued-state path,
 * and live-animation script update are exact; only latch/GPR emission differs.
 */
void npc_glitch_him_to_ani(
    KonquestNpcData* data, int animation_id, int flags) {
    KonquestNpc* npc = npc_find_by_data_inline(data);
    AniData* animation = get_animation(animation_id);

    if (aproc->pid == 0xA014) {
        npc->queued_animation = animation;
        npc->animation_flags = flags;
        npc->queued_animation_frame = 0.0f;
    } else if (npc->animation != 0) {
        KonquestAnimPdata* animation_pdata =
            (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);

        set_anim_script(animation_pdata, animation, flags);
    }
}

/* Soft ceiling: 94.117645% - retail retains a redundant repeated null branch. */
void npc_set_anim_proc(MkProcEntryFn entry) {
    if (g_active_npc != 0 && g_active_npc->animation != 0) {
        xfer_proc(g_active_npc->animation->proc, entry);
    }
}

int npc_get_punch_count(void) {
    if (g_active_npc != 0) {
        return g_active_npc->punch_count;
    }
    return 0;
}

int npc_get_conversation_count(void) {
    if (g_active_npc != 0) {
        return g_active_npc->conversation_count;
    }
    return 0;
}

void npc_fire_trigger(void) {
    KonquestTrigger* trigger = find_trigger_by_id();

    if (g_active_npc != 0) {
        trigger->data->source_npc = g_active_npc;
        trigger->data->source_type = 3;
        execute_trigger(trigger);
    }
}

/* Soft ceiling: 99.82758% - only the -1.0f relocation differs. */
static float p_npc_waypoint_script(void) {
    KonquestWaypointScriptPdata* pdata =
        (KonquestWaypointScriptPdata*)pdata_of_proc(aproc);

    if (pdata->function_index != 0) {
        cmdscript_set_parameters(
            active_cmdscript, 1, pdata->npc->data);
        cmdscript_setup_execution(
            konquest_pdata->waypoint_script, pdata->function_index);
        cmdscript_execute(konquest_pdata->waypoint_script);
    }
    return -1.0f;
}

KonquestNpcData* get_active_npc_data(void) {
    KonquestNpcData* data = 0;

    if (g_active_npc != 0) {
        data = g_active_npc->data;
    }
    return data;
}

/*
 * Soft ceiling: 93.088234% - the retail-sized algorithm is exact; five-GPR
 * coloring, the texture-instance latch branch, and animation-load scheduling
 * differ.
 */
void npc_play_random_dialog_sequence(void) {
    KonquestRandomDialogSequence* sequences =
        konquest_pdata->random_dialog_sequences;
    int count = konquest_pdata->random_dialog_sequence_count;

    if (konquest_pdata->load_config->dialog_definitions != 0 &&
        npc_dialog_wait_for_widescreen_bars() != 0) {
        unsigned short choice = randu0((unsigned short)count);
        KonquestDialogDefinition* dialog =
            &konquest_pdata->load_config->dialog_definitions[
                sequences[choice].dialog];
        int sound_id;
        LipSyncKeyframe* keyframes;

        dialog_text = get_string_by_id(dialog->string_id | 0x20000);
        keyframes = dialog->keyframes;
        sound_id = dialog->sound_id;
        if (g_active_npc->animation != 0 && sound_id != -1) {
            KonquestLipSyncPdata* lip;

            if (_create_mkproc_generic_nostack(
                    0x8232, 0x1F, p_do_lip_synch, sizeof(*lip),
                    (MkHdr**)&lip) != 0) {
                AniTextureControl* texture;
                KonquestNpcAnimState* animation;
                KonquestLipSyncPdata* target;
                unsigned int texture_instance;

                zero_pdata_payload(sizeof(*lip), &lip->hdr);
                lip->mode = 1;
                lip->npc = g_active_npc;
                target = lip;
                animation = g_active_npc->animation;
                texture = animation->lip_texture;
                texture_instance = animation->lip_texture_instance;
                target->texture = texture;
                target->texture_instance = texture_instance;
                lip->sound_handle = sound_id;
                texture = lip->texture;
                if (texture != 0) {
                    if ((unsigned int)texture->instance !=
                        lip->texture_instance) {
                        texture = 0;
                    }
                } else {
                    texture = 0;
                }
                if (texture != 0) {
                    lip->keyframes = keyframes;
                }
                lip->stop_requested = 0;
            }
        }
        if (g_active_npc->animation != 0) {
            g_active_npc->animation->dialog_anim =
                sequences[choice].animation;
        }
        if (dialog_text != 0) {
            konquest_set_dialog_text(dialog_text, dialog->keyframes);
        }
        npc_wait_for_dialog();
        konquest_pdata->random_dialog_sequence_count = 0;
    }
}

void npc_set_random_dialog_and_anim_sequence(int dialog, int animation) {
    int index = konquest_pdata->random_dialog_sequence_count;

    if (index < 20) {
        konquest_pdata->random_dialog_sequences[index].animation = animation;
        konquest_pdata->random_dialog_sequences[
            konquest_pdata->random_dialog_sequence_count].dialog = dialog;
        konquest_pdata->random_dialog_sequence_count++;
    }
}

/* Near match: 90.31868% at exact retail size. Dialog lookup, lip-sync process
 * ownership, validated animated texture, text selection, and wait ordering
 * match; residue is compact-mode register allocation and latch scheduling. */
#pragma optimize_for_size on
void npc_play_dialog_and_anim_sequence(int dialog_id, int animation_id) {
    int sound_id;
    LipSyncKeyframe* keyframes;
    KonquestDialogDefinition* dialog;
    KonquestDialogDefinition* definitions =
        konquest_pdata->load_config->dialog_definitions;

    if (definitions == 0) {
        return;
    }
    dialog = &definitions[dialog_id];
    if (check_skip_conversation_flag() == 0 &&
        npc_dialog_wait_for_widescreen_bars() != 0) {
        keyframes = dialog->keyframes;
        sound_id = dialog->sound_id;

        if (g_active_npc->animation != 0 && sound_id != -1) {
            KonquestLipSyncPdata* lip;

            if (_create_mkproc_generic_nostack(
                    0x8232, 0x1F, p_do_lip_synch, sizeof(*lip),
                    (MkHdr**)&lip) != 0) {
                AniTextureControl* texture;
                KonquestNpcAnimState* animation;

                zero_pdata_payload(sizeof(*lip), &lip->hdr);
                lip->mode = 1;
                lip->npc = g_active_npc;
                animation = g_active_npc->animation;
                lip->texture = animation->lip_texture;
                lip->texture_instance = animation->lip_texture_instance;
                lip->sound_handle = sound_id;
                texture = lip->texture;
                if (texture != 0) {
                    if ((unsigned int)texture->instance !=
                        lip->texture_instance) {
                        texture = 0;
                    }
                } else {
                    texture = 0;
                }
                if (texture != 0) {
                    lip->keyframes = keyframes;
                }
                lip->stop_requested = 0;
            }
        }
        if (g_active_npc->animation != 0) {
            g_active_npc->animation->dialog_anim = animation_id;
        }
        dialog_text = get_string_by_id(dialog->string_id | 0x20000);
        if (dialog_text != 0) {
            konquest_set_dialog_text(dialog_text, dialog->keyframes);
        }
        npc_wait_for_dialog();
    }
}
#pragma optimize_for_size reset



/*
 * Matching: 99.86842% - instructions are exact; only the 1.0f relocation
 * differs.
 */
static int npc_dialog_wait_for_widescreen_bars(void) {
    if (konquest_pdata->dialog_ready != 0) {
        return 1;
    }
    if (find_mkproc_pid(0x8229) == 0) {
        return 0;
    }
    while (konquest_pdata->dialog_ready == 0) {
        npc_ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
    }
    return 1;
}/*
 * Soft ceiling: 85.7619% - controller gating, attack transition/speed/sound,
 * frame waits, collision arguments/state, idle restoration, and flag clear are
 * recovered. The size residue is repeated typed active-animation lowering and
 * nonvolatile/FPR allocation.
 */
void npc_attack(int attack_arg_a, int attack_arg_b) {
    if (aproc->pid != 0xA014) {
        KonquestAnimPdata* animation;
        int idle_animation;

        turn_controllers_off();
        animation = (KonquestAnimPdata*)pdata_of_proc(
            g_active_npc->animation->proc);
        transition_to_anim_script(
            animation, konquest_animations.npc_attack, 3, 0.5f);
        animation->step = 0.5f;
        random_snd_req_delay(0x7A, 0x19);
        npc_ani_to_frame_x(13.0f);
        if (attack_arg_a != 0) {
            konquest_pdata->attack_arg_a = attack_arg_a;
            konquest_pdata->attack_arg_b = attack_arg_b;
        }
        turn_controllers_on();
        konquest_pdata->collision_mode = 0xB;
        npc_ani_to_blend_frame(10.0f);

        idle_animation = g_active_npc->data->idle_animation;
        g_active_npc->queued_animation = get_animation(idle_animation);
        g_active_npc->animation_flags = 0;
        g_active_npc->queued_animation_frame = 0.0f;
        if (aproc->pid != 0xA014) {
            KonquestNpcAnimState* state = g_active_npc->animation;
            int has_active_animation;

            if (state == 0) {
                has_active_animation = 0;
            } else if (state->object == 0) {
                has_active_animation = 0;
            } else {
                has_active_animation = state->proc != 0;
            }
            if (has_active_animation != 0) {
                AniData* idle = get_animation(idle_animation);
                KonquestAnimPdata* current =
                    (KonquestAnimPdata*)pdata_of_proc(state->proc);

                current->step = 1.0f;
                if (current->animation != idle || current->flags != 0 ||
                    current->step != 1.0f) {
                    transition_to_anim_script(
                        current, idle, 0, 0.05f);
                }
            }
        }
        g_active_npc->flags &= 0xF7FFFFFF;
    }
}

/* Near match: 83.37344%, 12 bytes short of retail. NPC construction, monk and
 * animation-process latches, hero data ownership, position/runtime snapshot,
 * face texture, and process setup are complete; residue is latch lowering,
 * GPR allocation, and split save/restore emission. */
KonquestNpc* konquest_make_monk_an_npc(void) {
    KonquestNpc* npc = konquest_pdata->monk_npc;
    MkObj* monk;

    if (npc != 0) {
        if (npc->hdr.instance != konquest_pdata->monk_npc_instance) {
            npc = 0;
        }
    } else {
        npc = 0;
    }
    if (npc != 0) {
        return npc;
    }

    npc = (KonquestNpc*)get_mkhdr(
        &vtbl_konquest_npc_struct, sizeof(*npc));
    if (npc != 0) {
        zero_pdata_payload(sizeof(*npc), &npc->hdr);
        npc->state_268 = -3;
        npc->state_264 = -3;
        npc->state_26C = -3;
        npc->distance_270 = __float_max[0];
        npc->value_274 = 0.0f;
        npc->distance_278 = __float_max[0];
    }
    npc->data = 0;
    npc->path = 0;
    npc->animation = 0;
    npc->proc = 0;
    npc->state_58 = 0;
    npc->queued_animation = 0;
    npc->queued_animation_frame = 0.0f;
    npc->animation_flags = 0;
    npc->camera_distance_squared = 0.0f;
    npc->conversation_count = 0;
    npc->punch_count = 0;
    npc->saved_script_stack_depth = 0;
    npc->wait_ticks = 0.0f;
    npc->animation_speed = 1.0f;
    npc->turn_proc = 0;
    npc->turn_proc_instance = 0;
    npc->flags = 0;
    memset(npc->runtime_state, 0, sizeof(npc->runtime_state));
    npc->saved_animation_step = 1.0f;
    npc->data = (KonquestNpcData*)get_data_table_by_name("hero_npc");
    if (npc->data == 0) {
        return 0;
    }
    npc->name = "hero_npc";

    monk = konquest_pdata->monk;
    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (monk == 0) {
        return 0;
    }

    konquest_pdata->monk_npc = npc;
    konquest_pdata->monk_npc_instance = npc->hdr.instance;
    npc->data->position.x = monk->pos.x;
    npc->data->position.y = monk->pos.y;
    npc->data->position.z = monk->pos.z;
    npc->tile_index = get_tile_from_position(&npc->data->position);
    if (npc_event_has_active_animation(npc) != 0) {
        npc->animation->object->flags_09_bits.bit6 = 0;
        npc->animation->object->pos.x = npc->data->position.x;
        npc->animation->object->pos.y = npc->data->position.y;
        npc->animation->object->pos.z = npc->data->position.z;
    }
    npc->data->angle_y = monk->ang.y;
    npc->flags_1C |= 0x10;
    npc->flags_1D &= (unsigned char)~2;
    npc->flags_1C |= 0x40;
    npc->timed_event_flags |= 0x40;
    npc->saved_object_flags = monk->flags_word_08;
    npc->runtime_state[2] &= (char)~2;
    npc->saved_gravity = monk->gravity;

    if (npc->animation == 0) {
        KonquestNpcAnimState* animation = 0;
        MkProc* proc = _create_mkproc_generic_bigstack(
            0xA002, 8, p_npc_idle, sizeof(*animation),
            (MkHdr**)&animation);

        if (proc != 0) {
            AniTextureControl* texture;
            AnimPdata* monk_animation;
            MkProc* animation_proc;

            set_process_as_scriptable(proc);
            proc->pre_destroy = (MkProcCallbackFn)npc_pre_wake;
            proc->destroy_cb = (MkProcCallbackFn)npc_post_sleep;
            npc->animation = animation;
            npc->proc = proc;
            animation->alpha = 0xFF;
            texture = konquest_create_monk_face_ani_texture(monk);
            if (texture != 0) {
                animation->lip_texture = texture;
                animation->lip_texture_instance = texture->instance;
            }
            animation->owner_npc = npc;
            animation->proc = 0;
            animation->object = monk;
            animation->editor_object = 0;
            animation->dialog_anim = 0;
            monk_animation = konquest_pdata->monk_animation;
            animation_proc = monk_animation->proc;
            if (animation_proc != 0) {
                if (animation_proc->instance !=
                    monk_animation->proc_instance) {
                    animation_proc = 0;
                }
            } else {
                animation_proc = 0;
            }
            animation->proc = animation_proc;
        }
    }
    return npc;
}

/* Soft ceiling: 88.42105% - typed active-animation boolean lowers differently. */
void npc_set_gravity(float gravity) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->gravity = gravity;
    }
}

void npc_set_my_ground_level(float ground_level) {
    g_active_npc->animation->object->ground_colls_y = ground_level;
}

/*
 * Soft ceiling: 83.08989% - record stride, removal loop, stale-list cleanup,
 * tile visibility, animation latch, and flag fallback are all recovered.
 * Residue is nonvolatile allocation and equivalent branch scheduling.
 */
void remove_npc_list(KonquestNpcData* list) {
    unsigned int count = get_row_count_for_table_by_pointer(
        konquest_pdata->waypoint_script, list);
    unsigned int index;

    for (index = 0; index < count; index++) {
        remove_npc(&list[index]);
    }

    get_visible_tile_set(konquest_pdata->visible_tile_set);
    discard_list(&konquest_pdata->visible_npc_list);
    if (&konquest_pdata->npc_list != 0) {
        MkPtr* link = konquest_pdata->npc_list;

        while (link != 0) {
            KonquestNpc* npc = (KonquestNpc*)link->hdr;

            if (link->instance != npc->hdr.instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                KonquestTileOrigin* tile =
                    get_nth_tile_struct(npc->tile_index);

                if (tile != 0) {
                    if (tile->loaded != 0) {
                        mk_insert(
                            &npc->hdr,
                            &konquest_pdata->visible_npc_list);
                    } else {
                        KonquestNpcAnimState* state = npc->animation;
                        int has_active_animation;

                        if (state == 0) {
                            has_active_animation = 0;
                        } else if (state->object == 0) {
                            has_active_animation = 0;
                        } else {
                            has_active_animation = state->proc != 0;
                        }
                        if (has_active_animation != 0 ||
                            (npc->flags_1C & 4) != 0) {
                            mk_insert(
                                &npc->hdr,
                                &konquest_pdata->visible_npc_list);
                        }
                    }
                }
                link = link->next;
            }
        }
    }
}

/*
 * Soft ceiling: 97.28395% - the shared lookup and flag update are exact;
 * only equivalent latch branches and register allocation differ.
 */
void npc_set_his_flags(
    KonquestNpcData* data, int flags, int enabled) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (npc != 0) {
        if (enabled != 0) {
            npc->flags |= flags;
            return;
        }
        npc->flags &= ~flags;
    }
}

/*
 * Soft ceiling: 85.333336% - the shared typed lookup algorithm is exact;
 * stale-latch branch polarity and nonvolatile-register allocation remain.
 */
int npc_get_his_flag_state(KonquestNpcData* data, int flags) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (npc != 0) {
        return npc->flags & flags;
    }
    return 0;
}

int npc_get_flag_state(int flags) {
    return g_active_npc->flags & flags;
}

/*
 * Soft ceiling: 84.65116% - the typed door-list traversal and waypoint-pointer
 * comparison are exact; save form and register allocation remain.
 */
void npc_open_door_at_waypoint(void) {
    KonquestWaypointScriptPdata* pdata =
        (KonquestWaypointScriptPdata*)pdata_of_proc(aproc);
    KonquestWaypoint* waypoints = 0;

    if (pdata != 0) {
        waypoints = pdata->npc->path->waypoints;
    }
    if (&konquest_pdata->door_list != 0) {
        MkPtr* link = konquest_pdata->door_list;

        while (link != 0) {
            KonquestDoor* door = (KonquestDoor*)link->hdr;

            if (link->instance != door->hdr.instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else if (door->path_waypoints == waypoints) {
                konquest_open_door_sobj(door, 0);
                break;
            } else {
                link = link->next;
            }
        }
    }
}

#pragma dont_inline on
#pragma optimize_for_size on
/* Matching: compact save/restore lowering plus the retail non-inlined call
 * boundary reproduces the function exactly. */
void npc_at_waypoint_set_flags(int flags, int enabled) {
    KonquestWaypointScriptPdata* pdata =
        (KonquestWaypointScriptPdata*)pdata_of_proc(aproc);

    if (pdata != 0) {
        npc_set_his_flags(pdata->npc->data, flags, enabled);
    }
}
#pragma optimize_for_size reset
#pragma dont_inline reset

/*
 * Soft ceiling: 86.585365% - lookup and byte-width bit update are exact;
 * only stale-latch branch lowering and register allocation differ.
 */
void npc_ignore_his_events(KonquestNpcData* data, int enabled) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (npc != 0) {
        if (enabled != 0) {
            npc->ignore_events = 1;
            return;
        }
        npc->ignore_events = 0;
    }
}

void npc_ignore_events(int enabled) {
    if (enabled != 0) {
        g_active_npc->ignore_events = 1;
        return;
    }
    g_active_npc->ignore_events = 0;
}

void npc_set_flags(int flags, int enabled) {
    if (enabled != 0) {
        g_active_npc->flags |= flags;
        return;
    }
    g_active_npc->flags &= ~flags;
}

/* Near match: 95.03509% at exact retail size. Repeated path ownership loads,
 * waypoint selection, navigation hint lowering, steering, inverse length,
 * arrival scripts, and final position/facing stores match. Residue is GPR/FPR
 * allocation, call scheduling, and one equivalent branch placement. */
static int npc_update_pos_on_path(
    KonquestNpc* npc, int destination_type, int destination) {
    KonquestWaypoint* waypoints;
    Vec target;
    Vec direction;
    Vec* movement;
    float speed;
    int target_waypoint;

    if (npc->path == 0) {
        return 1;
    }
    waypoints = npc->path->waypoints;
    if (is_npc_at_destination(npc, destination_type, destination) != 0) {
        if (waypoints == 0) {
            npc->data->angle_y = npc->current_waypoint_angle;
        } else {
            npc->data->angle_y =
                waypoints[npc->path->current_waypoint].angle;
        }
        npc->path->previous_waypoint = npc->path->current_waypoint;
        return 1;
    }

    if (npc->path->target_waypoint == npc->path->current_waypoint) {
        npc->path->target_waypoint = npc_next_waypoint(npc);
    }
    target_waypoint = npc->path->target_waypoint;
    if (npc->path != 0) {
        if ((unsigned int)(
                npc->path->destination_type + (int)0x80000000) == 7) {
            target.x = npc->path->destination.x;
            target.y = npc->path->destination.y;
            target.z = npc->path->destination.z;
        } else if (waypoints == 0) {
            target.x = npc->initial_position.x;
            target.y = npc->initial_position.y;
            target.z = npc->initial_position.z;
        } else {
            target.x = waypoints[target_waypoint].position.x;
            target.y = waypoints[target_waypoint].position.y;
            target.z = waypoints[target_waypoint].position.z;
        }
    }
    target.y = 0.0f;

    if (npc->path->travel_mode == 0) {
        speed = 0.03f * npc->path->speed * game_speed;
    } else {
        speed = 0.0525f * npc->path->speed * game_speed;
    }

    if (dist_xz_to_xz(&npc->data->position, &target) > 0.1f + speed) {
        movement = 0;
        if (npc->path->current_waypoint == -1) {
            int current_area;
            int destination_area;
            int has_navigation_direction = 0;

            if (target.x != npc->distance_270 ||
                target.z != npc->distance_278) {
                npc->state_26C = nav_what_area_is_point_in(
                    &target, npc_nav_area_hint(npc->state_26C));
                npc->distance_270 = target.x;
                npc->distance_278 = target.z;
            }
            destination_area = npc->state_26C;
            current_area = nav_what_area_is_point_in(
                &npc->data->position, npc_nav_area_hint(npc->state_268));
            if (current_area >= 0) {
                if (npc->state_264 < 0) {
                    npc->state_264 = current_area;
                } else if (npc->state_264 != npc->state_268) {
                    npc->state_264 = npc->state_268;
                }
                npc->state_268 = current_area;
            }
            if (destination_area >= 0) {
                if (current_area < 0) {
                    if (npc->state_264 >= 0) {
                        nav_get_unit_vector_to_area(
                            npc->state_264, &direction,
                            &npc->data->position);
                    } else {
                        nav_get_unit_vector_to_closest_area(
                            &direction, &npc->data->position);
                    }
                    has_navigation_direction = 1;
                } else {
                    int next_area = nav_which_area_is_next(
                        current_area, destination_area);

                    if (current_area != next_area) {
                        nav_get_unit_vector_to_nav_portal(
                            &direction, &npc->data->position,
                            current_area, next_area);
                        has_navigation_direction = 1;
                    }
                }
            }
            if (has_navigation_direction != 0) {
                movement = &direction;
            }
        }

        if (movement == 0) {
            float length_squared;
            float inverse_length = 0.0f;

            movement = &direction;
            direction.x = target.x - npc->data->position.x;
            direction.y = target.y - npc->data->position.y;
            direction.z = target.z - npc->data->position.z;
            length_squared =
                direction.z * direction.z +
                (direction.x * direction.x + direction.y * direction.y);
            if (length_squared <= 0.0f) {
                inverse_length = 0.0f;
            } else {
                KonquestFloatBits estimate;
                float product;
                float correction;

                estimate.value = length_squared;
                estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
                product = estimate.value * (length_squared * estimate.value);
                correction = 3.0f - product;
                inverse_length =
                    0.0625f * estimate.value * correction *
                    -((correction * (product * correction)) - 12.0f);
            }
            direction.x *= inverse_length;
            direction.y *= inverse_length;
            direction.z *= inverse_length;
        }

        movement->x *= speed;
        movement->y *= speed;
        movement->z *= speed;
        movement->y = 0.0f;
        npc->data->position.x += movement->x;
        npc->data->position.y += movement->y;
        npc->data->position.z += movement->z;
        npc->data->angle_y = gxMathArcTanYX(movement->x, movement->z);
        npc->data->angle_y =
            0.000005992112f *
            (float)(((int)(166886.1f * npc->data->angle_y)) & 0xFFFFF);
    } else {
        npc->data->position.x = target.x;
        npc->data->position.y = target.y;
        npc->data->position.z = target.z;
        npc->path->current_waypoint = target_waypoint;
        if (npc->path->waypoints != 0) {
            if (npc->path->waypoints[npc->path->current_waypoint]
                    .script_function != 0) {
                KonquestWaypointScriptPdata* pdata = 0;
                MkProc* proc = _create_mkproc_generic_tinystack(
                    0xA017, 0x1F, p_npc_waypoint_script,
                    sizeof(*pdata), (MkHdr**)&pdata);

                if (proc != 0) {
                    set_process_as_scriptable(proc);
                    pdata->function_index =
                        npc->path->waypoints[npc->path->current_waypoint]
                            .script_function;
                    pdata->npc = npc;
                }
            }
        }
        if (is_npc_at_destination(npc, destination_type, destination) != 0) {
            if (waypoints == 0) {
                npc->data->angle_y = npc->current_waypoint_angle;
            } else {
                npc->data->angle_y =
                    waypoints[npc->path->current_waypoint].angle;
            }
            npc->path->previous_waypoint = npc->path->current_waypoint;
            return 1;
        }
    }

    npc->tile_index = get_tile_from_position(&npc->data->position);
    return 0;
}

void npc_set_dialog_anim(int animation) {
    if (g_active_npc->animation != 0) {
        g_active_npc->animation->dialog_anim = animation;
    }
}

void hero_handle_conversation(void) {
    for (;;) {
        KonquestNpcPdata* pdata = konquest_pdata;

        nis_wait_for_event(pdata->conversation_event, -1);
        if (pdata->conversation_mode_a != 2) {
            nis_wait_for_event(konquest_pdata->conversation_event + 1, -1);
            continue;
        }

        xfer_proc(g_active_npc->animation->proc, p_anim_idle);
        npc_play_dialog_and_anim_sequence(
            pdata->conversation_state_a, pdata->conversation_state_b);
        nis_signal_event(konquest_pdata->conversation_event + 1);
        konquest_pdata->conversation_event += 2;
    }
}

/* Soft ceiling: 90.40084% - exact retail size, camera/control-flow sequence,
 * animation transfers, and event ordering; save and GPR scheduling remain. */
void npc_play_conversation_part(
    int dialog_id, int animation_id, int conversation_mode) {
    KonquestNpcPdata* pdata = konquest_pdata;

    pdata->conversation_state_a = dialog_id;
    pdata->conversation_state_b = animation_id;
    pdata->conversation_mode_a = conversation_mode;
    if (check_skip_conversation_flag() == 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        nis_signal_event(konquest_pdata->conversation_event);

        if (conversation_mode != 2) {
            if (konquest_pdata->conversation_mode_b != conversation_mode) {
                KonquestNpc* monk;
                KonquestAnimPdata* animation;

                npc_switch_camera_focus_inline(conversation_mode);
                konquest_pdata->conversation_mode_b = conversation_mode;
                monk = konquest_pdata->monk_npc;
                if (monk != 0) {
                    if (monk->hdr.instance !=
                        konquest_pdata->monk_npc_instance) {
                        monk = 0;
                    }
                } else {
                    monk = 0;
                }
                xfer_proc(
                    monk->animation->proc, (MkProcEntryFn)p_animate);
                animation = (KonquestAnimPdata*)pdata_of_proc(
                    monk->animation->proc);
                animation->animation = 0;
                animation->step = 1.0f;
                set_anim_script(
                    animation,
                    get_animation(monk->data->idle_animation), 0);
            }
            xfer_proc(g_active_npc->animation->proc, p_anim_idle);
            npc_play_dialog_and_anim_sequence(dialog_id, animation_id);
            nis_signal_event(konquest_pdata->conversation_event + 1);
            konquest_pdata->conversation_event += 2;
            return;
        }

        if (konquest_pdata->conversation_mode_b == 0) {
            KonquestAnimPdata* animation;

            npc_switch_camera_focus_inline(2);
            konquest_pdata->conversation_mode_b = 2;
            xfer_proc(
                g_active_npc->animation->proc, (MkProcEntryFn)p_animate);
            animation = (KonquestAnimPdata*)pdata_of_proc(
                g_active_npc->animation->proc);
            animation->animation = 0;
            animation->step = 1.0f;
            set_anim_script(
                animation,
                get_animation(g_active_npc->data->idle_animation), 0);
        } else if (konquest_pdata->conversation_mode_b == 1) {
            npc_switch_camera_focus_inline(2);
            konquest_pdata->conversation_mode_b = 2;
        }
        nis_wait_for_event(konquest_pdata->conversation_event + 1, -1);
    }
}

void conversation_term(void) {
    konquest_pdata->conversation_state_a = 0;
    konquest_pdata->conversation_state_b = 0;
    konquest_pdata->conversation_mode_a = 0;
    konquest_pdata->conversation_mode_b = 0;
    konquest_pdata->conversation_event = 0;
    resume_hero_state_process();
}

void conversation_init(int mode) {
    konquest_pdata->conversation_state_a = 0;
    konquest_pdata->conversation_state_b = 0;
    konquest_pdata->conversation_mode_a = mode;
    konquest_pdata->conversation_mode_b = mode;
    konquest_pdata->conversation_event = 0;
}

/* Near match: 94.16309%, four bytes short of retail. Both animation-process
 * and monk-NPC handles are validated in retail order; the remaining difference
 * is one compiler-emission instruction plus register allocation. */
void npc_play_two_player_one_shot_anims(int npc_animation_id,
                                        int monk_animation_id) {
    int npc_finished = 0;
    int monk_finished = 0;

    if (g_active_npc != 0) {
        int blocked;

        if (npc_event_has_active_animation(g_active_npc) == 1) {
            blocked = (g_active_npc->timed_event_flags & 0x40) == 0;
        } else {
            blocked = 1;
        }
        if (blocked == 0) {
            KonquestAnimPdata* npc_animation =
                (KonquestAnimPdata*)pdata_of_proc(
                    g_active_npc->animation->proc);
            AnimPdata* monk_animation;
            MkProc* monk_animation_proc;
            KonquestNpc* monk_npc;

            g_active_npc->queued_animation = get_animation(npc_animation_id);
            g_active_npc->animation_flags = 3;
            g_active_npc->queued_animation_frame = 0.0f;
            if (aproc->pid != 0xA014 &&
                npc_event_has_active_animation(g_active_npc) != 0) {
                AniData* script = get_animation(npc_animation_id);
                KonquestAnimPdata* current = (KonquestAnimPdata*)pdata_of_proc(
                    g_active_npc->animation->proc);

                current->step = 1.0f;
                if (current->animation != script || current->flags != 3 ||
                    current->step != 1.0f) {
                    transition_to_anim_script(current, script, 3, 0.1f);
                }
            }

            monk_animation = konquest_pdata->monk_animation;
            monk_animation_proc = monk_animation->proc;
            if (monk_animation_proc != 0) {
                if (monk_animation_proc->instance !=
                    monk_animation->proc_instance) {
                    monk_animation_proc = 0;
                }
            } else {
                monk_animation_proc = 0;
            }
            monk_npc = konquest_pdata->monk_npc;
            if (monk_npc != 0) {
                if (monk_npc->hdr.instance !=
                    konquest_pdata->monk_npc_instance) {
                    monk_npc = 0;
                }
            } else {
                monk_npc = 0;
            }
            xfer_proc(monk_animation_proc, (MkProcEntryFn)p_animate);
            monk_animation->step = 1.0f;
            transition_to_anim_script((KonquestAnimPdata*)monk_animation,
                                      get_animation(monk_animation_id), 0,
                                      0.1f);

            while (npc_finished == 0 || monk_finished == 0) {
                if (npc_finished == 0 &&
                    npc_animation->frame >= npc_animation->high_frame - 10.0f) {
                    int idle_animation = g_active_npc->data->idle_animation;

                    g_active_npc->queued_animation =
                        get_animation(idle_animation);
                    g_active_npc->animation_flags = 0;
                    g_active_npc->queued_animation_frame = 0.0f;
                    if (aproc->pid != 0xA014 &&
                        npc_event_has_active_animation(g_active_npc) != 0) {
                        AniData* idle = get_animation(idle_animation);
                        KonquestAnimPdata* current =
                            (KonquestAnimPdata*)pdata_of_proc(
                                g_active_npc->animation->proc);

                        current->step = 1.0f;
                        if (current->animation != idle || current->flags != 0 ||
                            current->step != 1.0f) {
                            transition_to_anim_script(current, idle, 0, 0.1f);
                        }
                    }
                    npc_finished = 1;
                }
                if (monk_finished == 0 &&
                    monk_animation->frame >=
                        monk_animation->high_frame - 10.0f) {
                    monk_animation->step = 1.0f;
                    transition_to_anim_script(
                        (KonquestAnimPdata*)monk_animation,
                        get_animation(monk_npc->data->idle_animation), 0, 0.1f);
                    monk_finished = 1;
                }
                npc_ani_1_frame();
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            }
        }
    }
}

/*
 * Soft ceiling: 93.888885% - the typed pointer comparison and return CFG are
 * exact; retail retains one redundant branch in the stale-instance latch.
 */
int is_this_the_monk_npc(KonquestNpc* npc) {
    KonquestNpc* monk = konquest_pdata->monk_npc;

    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_npc_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }

    if (npc_is_same(monk, npc)) {
        return 1;
    }
    return 0;
}

/*
 * Soft ceiling: 90.75572% - the typed body is exact; the proc-pointer boolean
 * lowering, split nonvolatile saves/restores, and 1.0f relocation differ.
 */
void npc_wait_for_dialog(void) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;
    int skip_wait;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation == 1) {
        skip_wait = !g_active_npc->wait_for_animation;
    } else {
        skip_wait = 1;
    }

    if (skip_wait == 0) {
        MkProc* wait_proc = _create_mkproc_generic_bigstack(
            0x9025, 0x1F, p_wait_for_dialog, 0, 0);

        if (wait_proc != 0) {
            mk_insert_no_own(
                &g_active_npc->hdr, &wait_proc->pdata_list);
            while (find_mkproc_pid(0x9025) != 0) {
                if (check_skip_conversation_flag() != 0) {
                    if (&active_proc_list != 0) {
                        MkPtr* link = active_proc_list;

                        while (link != 0) {
                            MkProc* proc = (MkProc*)link->hdr;

                            if (link->instance !=
                                (unsigned int)proc->instance) {
                                MkPtr* next = link->next;

                                link->hdr = 0;
                                destroy_mkptr(link);
                                link = next;
                            } else {
                                if (proc->pid == 0x8232) {
                                    KonquestLipSyncPdata* lip =
                                        (KonquestLipSyncPdata*)
                                            pdata_of_proc(proc);
                                    AniTextureControl* texture = lip->texture;
                                    unsigned int sound_handle =
                                        lip->sound_handle;

                                    if (texture != 0) {
                                        if ((unsigned int)texture->instance ==
                                            lip->texture_instance) {
                                            /* The latch is still live. */
                                        } else {
                                            texture = 0;
                                        }
                                    } else {
                                        texture = 0;
                                    }
                                    if (sound_handle != 0) {
                                        snd_stop(sound_handle);
                                    }
                                    if (lip->mode == 1 && texture != 0) {
                                        set_ani_texture_frame(texture, 0);
                                    }
                                    if ((unsigned int)proc->instance != 0) {
                                        KonquestProcDestroyVtable* vtable =
                                            (KonquestProcDestroyVtable*)
                                                proc->vtbl;

                                        vtable->destroy(proc, vtable);
                                    }
                                }
                                link = link->next;
                            }
                        }
                    }
                    destroy_mkprocs_pid(0x9025);
                    destroy_mkprocs_pid(0xA012);
                    return;
                }
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            }
        }
    }
}

/*
 * Soft ceiling: 99.85294% - instructions are exact; only the signed-int to
 * float conversion constant relocation differs.
 */
/* Near match: 89.58064%, four bytes from retail. Sequence timing, animation
 * playback/restoration, dialog cancellation, and lip-sync shutdown are exact;
 * the remaining difference is save/scheduling emission. */
float p_wait_for_dialog(void) {
    KonquestNpc* npc = (KonquestNpc*)apdata;
    KonquestAnimPdata* animation;

    if (npc == 0) {
        return -1.0f;
    }
    animation = (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);
    if (npc->animation->dialog_sequence != 0) {
        KonquestDialogAnimation* sequence =
            npc->animation->dialog_sequence;
        unsigned int count = get_row_count_for_table_by_pointer(
            konquest_pdata->waypoint_script, sequence);
        AniData* saved_animation = animation->animation;
        unsigned int saved_flags = animation->flags;
        unsigned int index = 0;

        while (index < count) {
            float remaining =
                (float)sequence->duration / get_game_speed();

            transition_to_anim_script(
                animation, get_animation(sequence->animation),
                0x20003, 0.05f);
            animation->step = sequence->speed;
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            if (remaining > 0.0f) {
                remaining -= 1.0f;
            }
            while ((npc->flags_1C & 4) != 0 &&
                   (sequence->duration == 0 || remaining > 0.0f) &&
                   animation->frame < animation->high_frame - 10.0f) {
                advance_anim((AnimState*)animation);
                pose_anim((AnimState*)animation, 1);
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                if (remaining > 0.0f) {
                    remaining -= 1.0f;
                }
            }
            if ((npc->flags_1C & 4) != 0) {
                while (sequence->duration != 0 && remaining > 0.0f) {
                    _mkproc_sleep_ticks = 1.0f;
                    remaining -= 1.0f;
                    ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                }
                index++;
                sequence++;
                continue;
            }
            break;
        }
        if (animation->animation != saved_animation ||
            animation->flags != saved_flags) {
            animation->step = 1.0f;
            transition_to_anim_script(
                animation, saved_animation, saved_flags, 0.05f);
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
    npc->animation->dialog_sequence = 0;
    while ((npc->flags_1C & 4) != 0) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        if (animation->animation != 0) {
            advance_anim((AnimState*)animation);
            pose_anim((AnimState*)animation, 1);
        }
    }
    {
        MkProc* lip_proc = find_mkproc_pid(0x8232);

        if (lip_proc != 0) {
            KonquestLipSyncPdata* lip =
                (KonquestLipSyncPdata*)pdata_of_proc(lip_proc);

            if (lip != 0) {
                lip->stop_requested = 1;
            }
        }
    }
    return -1.0f;
}

void npc_stop_goro_bone_match(void) {
    MkProc* proc = find_mkproc_pid(0x500F);

    if (proc != 0) {
        destroy_mkproc_nostack(proc);
    }
}

/*
 * Soft ceiling: 81.88172% - script object flag, shared NPC lookup, matcher
 * arguments, three offset components, and byte flag update match at retail
 * size. Residue is latch/save/register emission and float relocations.
 */
void npc_start_goro_bone_match(KonquestNpcData* data) {
    KonquestObjectScriptPdata* pdata =
        (KonquestObjectScriptPdata*)pdata_of_proc(aproc);
    KonquestNpc* npc;
    KonquestBoneMatcher* matcher;

    pdata->object->flags_09 &= ~0x40;
    npc = npc_find_by_data_inline(data);
    matcher = start_bone_matcher(
        0.0f, npc->animation->object, 10, pdata->object, 0);
    matcher->parent_offset.x = -0.04f;
    matcher->parent_offset.y = 0.1f;
    matcher->parent_offset.z = -0.15f;
    matcher->flags_08 |= 8;
}

void vdestroy_path_data_struct(KonquestPathData* path) {
    path->hdr.instance = 0;
    mkhdr_memfree(&path->hdr);
}

/* Matching: 99.8% - code is exact; only the 1.0f relocation differs. */
KonquestPathData* get_new_path_data_struct(void) {
    KonquestPathData* path = (KonquestPathData*)get_mkhdr(
        &vtbl_path_data_struct, sizeof(KonquestPathData));

    if (path != 0) {
        zero_pdata_payload(sizeof(KonquestPathData), &path->hdr);
        path->travel_mode = 0;
        path->speed = 1.0f;
        path->table_index = -2;
    }
    return path;
}

void vdestroy_konquest_npc_struct(KonquestNpc* npc) {
    npc->hdr.instance = 0;
    mkhdr_memfree(&npc->hdr);
}

/*
 * Soft ceiling: 83.098595% - ID validation, table-index decode, stale-list
 * cleanup, state check, and event dispatch match at retail size. Remaining
 * differences are latch branch polarity, save form, and register allocation.
 */
int npc_collision_callback(unsigned int* collision_id) {
    unsigned int id = *collision_id;

    if (id != 0 && (id & 0x80000000) == 0 &&
        konquest_pdata->collision_mode == 2) {
        int table_index = id - 0x10001;
        KonquestNpc* npc = npc_find_by_table_index_inline(table_index);

        if (npc->state_58 != 5) {
            npc_signal_event(npc, 5);
        }
        return 1;
    }
    return 0;
}

void npc_signal_event(KonquestNpc* npc, int event_index) {
    if (!npc->ignore_events) {
        npc_set_state_for_npc(npc, event_index);
    }
}

/* Near match: 85.76744%. Shared NPC lookup, slot-7 script publication,
 * animation reset, and state transition are exact; residue is inline/latch
 * scheduling and the portable failure diagnostic. */
void npc_take_control_of_him(
    KonquestNpcData* data, unsigned int script_function) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (npc == 0) {
        char message[256];

        sprintf(
            message,
            "NP: %s -- npc_take_control_of_him -- Couldn't find npc",
            data->name);
        return;
    }
    npc_set_event_script(npc, 7, script_function);
    npc_set_state_for_npc(npc, 7);
}

/*
 * Soft ceiling: 86.77631% - shared lookup, state read, and 0/7 restart rule
 * are exact; stale-latch lowering and nonvolatile allocation remain.
 */
void npc_restart_his_normal_behavior(KonquestNpcData* data) {
    KonquestNpc* npc = npc_find_by_data_inline(data);
    int state = npc->state_58;

    if (state == 0 || state == 7) {
        npc_force_state_for_npc(npc, 0);
    }
}

/*
 * Soft ceiling: 86.447914% - lookup, persistent angle, active-animation
 * predicate, byte flag access, and object angle store are exact; latch and
 * register allocation differ.
 */
void npc_set_his_ang_y(KonquestNpcData* data, float angle) {
    KonquestNpc* npc = npc_find_by_data_inline(data);
    KonquestNpcAnimState* state;
    int has_active_animation;

    npc->data->angle_y = angle;
    state = npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->hide_flags &= ~2;
        npc->animation->object->ang.y = angle;
    }
}

/* Soft ceiling: 92.413795% - typed active-animation boolean lowers differently. */
void npc_set_my_ang_y(float angle) {
    KonquestNpcAnimState* state;
    int has_active_animation;

    g_active_npc->data->angle_y = angle;
    state = g_active_npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkObj* object = state->object;

        object->hide_flag_bits.pin_animation = 0;
        g_active_npc->animation->object->ang.y = angle;
    }
}

/*
 * Soft ceiling: 89.45385% - lookup, position stores, tile update, animation
 * predicate, access width, and object position transfer are exact; remaining
 * differences are latch branches and FPR/GPR allocation.
 */
void npc_set_his_world_pos(
    KonquestNpcData* data, float x, float y, float z) {
    KonquestNpc* npc = npc_find_by_data_inline(data);
    KonquestNpcAnimState* state;
    Vec position;
    int has_active_animation;

    position.x = x;
    position.y = y;
    position.z = z;
    npc->data->position.x = x;
    npc->data->position.y = position.y;
    npc->data->position.z = position.z;
    npc->tile_index = get_tile_from_position(&position);
    state = npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->hide_flags &= ~2;
        npc->animation->object->pos.x = position.x;
        npc->animation->object->pos.y = position.y;
        npc->animation->object->pos.z = position.z;
    }
}

/*
 * Soft ceiling: 96.14035% - the typed body is exact; proc-pointer boolean
 * lowering and the nonvolatile save form differ.
 */
void npc_set_my_world_pos(float x, float y, float z) {
    Vec position;
    KonquestNpc* npc = g_active_npc;
    KonquestNpcAnimState* state;
    int has_active_animation;

    position.x = x;
    position.y = y;
    position.z = z;
    npc->data->position.x = position.x;
    npc->data->position.y = position.y;
    npc->data->position.z = position.z;
    npc->tile_index = get_tile_from_position(&position);

    state = npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->hide_flag_bits.pin_animation = 0;
        npc->animation->object->pos.x = position.x;
        npc->animation->object->pos.y = position.y;
        npc->animation->object->pos.z = position.z;
    }
}

/* Soft ceiling: 97.25% - typed active-animation boolean lowers differently. */
void npc_set_my_pos(float x, float y, float z) {
    KonquestTileOrigin* tile =
        get_nth_tile_struct(g_active_npc->tile_index);
    KonquestNpc* npc = g_active_npc;
    Vec position;
    KonquestNpcAnimState* state;
    int has_active_animation;

    position.x = x + tile->origin.x;
    position.y = y + tile->origin.y;
    position.z = z + tile->origin.z;
    npc->data->position.x = position.x;
    npc->data->position.y = position.y;
    npc->data->position.z = position.z;
    npc->tile_index = get_tile_from_position(&position);

    state = npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkObj* object = state->object;

        object->hide_flag_bits.pin_animation = 0;
        npc->animation->object->pos.x = position.x;
        npc->animation->object->pos.y = position.y;
        npc->animation->object->pos.z = position.z;
    }
}

float duration_of_lip_sync(const LipSyncKeyframe* keyframes) {
    float duration;

    if (keyframes == 0) {
        return 0.0f;
    }
    while (keyframes->frame != -1) {
        keyframes++;
    }
    duration = keyframes->time;
    if (duration < 0.0f) {
        duration = 0.0f;
    }
    return duration * (float)refresh_rate();
}

/*
 * Soft ceiling: 83.731346% - the typed body is exact; nonvolatile GPR
 * coloring, split saves/restores, and an equivalent latch branch differ.
 */
void kill_lip_sync_procs(void) {
    MkPtr* link;

    if (&active_proc_list != 0) {
        link = active_proc_list;
        while (link != 0) {
            MkHdr* hdr = link->hdr;

            if (link->instance != hdr->instance) {
                MkPtr* next = link->next;

                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                MkProc* proc = (MkProc*)hdr;

                if (proc->pid == 0x8232) {
                    KonquestLipSyncPdata* lip =
                        (KonquestLipSyncPdata*)pdata_of_proc(proc);
                    AniTextureControl* texture = lip->texture;
                    unsigned int sound_handle = lip->sound_handle;

                    if (texture != 0) {
                        if ((unsigned int)texture->instance ==
                            lip->texture_instance) {
                            /* The latched texture is still live. */
                        } else {
                            texture = 0;
                        }
                    } else {
                        texture = 0;
                    }
                    if (sound_handle != 0) {
                        snd_stop(sound_handle);
                    }
                    if (lip->mode == 1 && texture != 0) {
                        set_ani_texture_frame(texture, 0);
                    }
                    if ((unsigned int)proc->instance != 0) {
                        KonquestProcDestroyVtable* vtable =
                            (KonquestProcDestroyVtable*)proc->vtbl;

                        vtable->destroy(proc, vtable);
                    }
                }
                link = link->next;
            }
        }
    }
}

/*
 * Soft ceiling: 89.84127% - the typed body is exact; local GPR coloring,
 * split saves/restores, and an equivalent latch branch differ.
 */
void npc_lip_synch(int sound_id, LipSyncKeyframe* keyframes) {
    KonquestLipSyncPdata* lip;

    if (g_active_npc->animation != 0 && sound_id != -1 &&
        _create_mkproc_generic_nostack(
            0x8232, 0x1F, p_do_lip_synch, sizeof(*lip),
            (MkHdr**)&lip) != 0) {
        AniTextureControl* texture;
        KonquestNpcAnimState* animation;
        KonquestLipSyncPdata* target;
        unsigned int texture_instance;

        zero_pdata_payload(sizeof(*lip), &lip->hdr);
        lip->mode = 1;
        lip->npc = g_active_npc;
        target = lip;
        animation = g_active_npc->animation;
        texture = animation->lip_texture;
        texture_instance = animation->lip_texture_instance;
        target->texture = texture;
        target->texture_instance = texture_instance;
        lip->sound_handle = sound_id;

        texture = lip->texture;
        if (texture != 0) {
            if ((unsigned int)texture->instance ==
                lip->texture_instance) {
                /* The latched texture is still live. */
            } else {
                texture = 0;
            }
        } else {
            texture = 0;
        }
        if (texture != 0) {
            lip->keyframes = keyframes;
        }
        lip->stop_requested = 0;
    }
}

/*
 * Soft ceiling: 95.992645% - sound lifecycle, 60 Hz keyframe dispatch,
 * animation/texture modes, stale texture latches, NPC cancellation flags, and
 * elapsed-time update match at retail size. Residue is latch branch polarity
 * and local constant relocation labeling.
 */
float p_do_lip_synch(void) {
    KonquestLipSyncPdata* lip =
        (KonquestLipSyncPdata*)pdata_of_proc(aproc);

    if (lip == 0) {
        return -1.0f;
    }
    if (lip->mode == 0) {
        return -1.0f;
    }
    if (lip->elapsed == 0.0f) {
        lip->sound_handle = snd_req(lip->sound_handle);
    }
    if (lip->keyframes != 0) {
        while (lip->elapsed >= 60.0f * lip->keyframes->time) {
            int frame = lip->keyframes->frame;

            if (frame < 0) {
                return -1.0f;
            }
            if (lip->mode == 3) {
                AniData* animation =
                    ((AniData**)lip->animation_table)[frame + 6];

                if (animation != 0) {
                    transition_to_anim_script(
                        lip->animation, animation, 3, 0.15f);
                }
            } else {
                AniTextureControl* texture = lip->texture;

                if (texture != 0) {
                    if ((unsigned int)texture->instance !=
                        lip->texture_instance) {
                        texture = 0;
                    }
                } else {
                    texture = 0;
                }
                if (texture != 0) {
                    set_ani_texture_frame(texture, frame);
                }
            }
            lip->keyframes++;
        }
    }
    if (lip->mode == 1) {
        if (mslSoundIsValid(lip->sound_handle) == 0) {
            AniTextureControl* texture = lip->texture;

            if (texture != 0) {
                if ((unsigned int)texture->instance !=
                    lip->texture_instance) {
                    texture = 0;
                }
            } else {
                texture = 0;
            }
            if (texture != 0) {
                set_ani_texture_frame(texture, 0);
            }
            return -1.0f;
        }
        if ((((lip->npc->flags_1C & 4) == 0) &&
             ((lip->npc->flags_1C & 0x80) != 0)) ||
            lip->stop_requested != 0) {
            AniTextureControl* texture;

            snd_stop(lip->sound_handle);
            texture = lip->texture;
            if (texture != 0) {
                if ((unsigned int)texture->instance !=
                    lip->texture_instance) {
                    texture = 0;
                }
            } else {
                texture = 0;
            }
            if (texture != 0) {
                set_ani_texture_frame(texture, 0);
            }
            return -1.0f;
        }
    }
    lip->elapsed += game_speed;
    return 1.0f;
}

/* Near match: 97.48572%, four bytes over retail. Target selection, square-root
 * estimate, distance bands, turn clamping, angular wrapping, and stores are
 * exact; residue is FPR/GPR coloring, float relocations, and one staged union
 * store that crashes this MWCC build when expressed separately. */
static void npc_update_current_direction(
    KonquestNpc* npc, const Vec* navigation_direction) {
    KonquestPathData* path = npc->path;
    float turn_amount;
    float target_angle;

    if (path->travel_mode == 0) {
        turn_amount = 0.108f * path->speed;
    } else {
        turn_amount = 0.144f * path->speed;
    }

    if (navigation_direction == 0) {
        float target_x;
        float target_z;
        float delta_x;
        float delta_z;
        float distance;
        float inverse_distance;

        if (path != 0) {
            if ((unsigned int)(
                    path->destination_type + (int)0x80000000) == 7) {
                target_x = path->destination.x;
                target_z = path->destination.z;
            } else if (path->waypoints == 0) {
                target_x = npc->initial_position.x;
                target_z = npc->initial_position.z;
            } else {
                target_x =
                    path->waypoints[path->target_waypoint].position.x;
                target_z =
                    path->waypoints[path->target_waypoint].position.z;
            }
        }
        delta_x = target_x - npc->animation->object->pos.x;
        delta_z = target_z - npc->animation->object->pos.z;
        distance = npc_fast_sqrt(delta_x * delta_x + delta_z * delta_z);
        inverse_distance = distance > 0.0f ? 1.0f / distance : distance;
        target_angle = gxMathArcTanYX(
            delta_x * inverse_distance, delta_z * inverse_distance);
        if (distance <= 1.0f) {
            turn_amount *= 4.0f;
        } else if (distance <= 2.0f) {
            turn_amount *= 2.0f;
        }
    } else {
        target_angle = gxMathArcTanYX(
            navigation_direction->x, navigation_direction->z);
    }

    if (get_konquest_game_mode() == 4) {
        turn_amount = 1.0f;
    }
    {
        float turn_scale = 1.0f;
        float delta_angle;

        if (turn_amount >= 1.0f) {
            turn_scale = 1.0f;
        } else {
            turn_scale = turn_amount;
        }
        delta_angle =
            0.000005992112f *
            (float)(((int)(166886.1f * target_angle)) & 0xFFFFF);
        delta_angle -= npc->animation->object->ang.y;

        if (delta_angle > 3.1415927f) {
            delta_angle -= 6.2831855f;
        } else if (delta_angle < -3.1415927f) {
            delta_angle += 6.2831855f;
        }
        delta_angle *= turn_scale;
        target_angle = npc->animation->object->ang.y + delta_angle;
    }
    npc->animation->object->ang.y =
        0.000005992112f *
        (float)(((int)(166886.1f * target_angle)) & 0xFFFFF);
}

/* Near match: 83.915565% at exact retail size. Live global-NPC ownership,
 * inlined command-script suspension, target refresh after sleeps, navigation,
 * animation selection, waypoint scripts, facing, and final object state match.
 * Residue is register allocation, scheduling islands, and equivalent branch
 * polarity around the range and navigation latches. */
void npc_travel_path(
    int destination_type, int destination, int travel_mode) {
    if (g_active_npc == 0) {
        return;
    }
    if (g_active_npc->path != 0) {
        if (travel_mode >= 2) {
            travel_mode = 1;
        }
        g_active_npc->path->travel_mode = travel_mode;
        g_active_npc->path->destination_type = destination_type;
    }

    if (aproc->pid == 0xA014) {
        if (npc_update_pos_on_path(
                g_active_npc, destination_type, destination) == 0) {
            g_active_npc->wait_ticks = 2.0f;
            npc_suspend_animation_wait();
        } else {
            g_active_npc->wait_ticks = 0.0f;
        }
        return;
    }

    if (npc_event_has_active_animation(g_active_npc) == 0) {
        return;
    }
    g_active_npc->animation->object->flags_09_bits.bit6 = 0;
    while (is_npc_at_destination(
               g_active_npc, destination_type, destination) == 0) {
        Vec target;
        float speed;

        if (g_active_npc->path->target_waypoint ==
            g_active_npc->path->current_waypoint) {
            g_active_npc->path->target_waypoint =
                npc_next_waypoint(g_active_npc);
        }
        npc_get_path_target(g_active_npc, &target);
        if (g_active_npc->path->current_waypoint == -1 &&
            (target.x != g_active_npc->distance_270 ||
             target.z != g_active_npc->distance_278)) {
            g_active_npc->state_26C = nav_what_area_is_point_in(
                &target, npc_nav_area_hint(g_active_npc->state_26C));
            g_active_npc->distance_270 = target.x;
            g_active_npc->distance_278 = target.z;
        }

        speed =
            (g_active_npc->path->travel_mode == 0 ? 0.03f : 0.0525f) *
            g_active_npc->path->speed * game_speed;
        while (dist_xz_to_xz(&g_active_npc->data->position, &target) >
               0.2f + speed) {
            Vec direction;
            Vec* navigation_direction = 0;

            if ((g_active_npc->flags_1C & 0x40) == 0 &&
                (is_game_mode_in_stack(1) != 0 ||
                 is_game_mode_in_stack(3) != 0)) {
                MkObj* monk = konquest_pdata->monk;

                if (monk != 0 &&
                    monk->hdr.instance != konquest_pdata->monk_instance) {
                    monk = 0;
                }
                if (monk != 0 && g_active_npc->animation != 0 &&
                    g_active_npc->animation->object != 0) {
                    float delta_x =
                        monk->pos.x -
                        g_active_npc->animation->object->pos.x;
                    float delta_z =
                        monk->pos.z -
                        g_active_npc->animation->object->pos.z;
                    float distance_squared =
                        delta_x * delta_x + delta_z * delta_z;

                    if (distance_squared > 4.0f &&
                        distance_squared <= 16.0f) {
                        int idle_animation =
                            g_active_npc->data->idle_animation;

                        g_active_npc->queued_animation =
                            get_animation(idle_animation);
                        g_active_npc->animation_flags = 0;
                        g_active_npc->queued_animation_frame = 0.0f;
                        if (aproc->pid != 0xA014 &&
                            npc_event_has_active_animation(g_active_npc) != 0) {
                            AniData* idle = get_animation(idle_animation);
                            KonquestAnimPdata* animation =
                                (KonquestAnimPdata*)pdata_of_proc(
                                    g_active_npc->animation->proc);

                            animation->step = 1.0f;
                            if (animation->animation != idle ||
                                animation->flags != 0 ||
                                animation->step != 1.0f) {
                                transition_to_anim_script(
                                    animation, idle, 0, 0.1f);
                            }
                        }
                        do {
                            _mkproc_sleep_ticks = 1.0f;
                            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                            npc_ani_1_frame();
                        } while (is_game_mode_in_stack(1) != 0);
                    }
                }
            }

            if (g_active_npc->animation->proc != 0) {
                KonquestAnimPdata* animation =
                    (KonquestAnimPdata*)pdata_of_proc(
                        g_active_npc->animation->proc);
                AniData* target_animation;

                animation->step = g_active_npc->path->speed;
                if (g_active_npc->path->use_animation_override == 0) {
                    target_animation = get_animation(
                        travel_mode == 0
                            ? g_active_npc->data->walk_animation
                            : g_active_npc->data->run_animation);
                } else {
                    target_animation =
                        get_animation(
                            g_active_npc->path->animation_override);
                }
                if (animation->animation != target_animation) {
                    transition_to_anim_script(
                        animation, target_animation, 0, 0.1f);
                    _mkproc_sleep_ticks = 1.0f;
                    ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                }
            }
            navigation_direction =
                npc_get_navigation_direction(g_active_npc, &direction);
            npc_update_current_direction(
                g_active_npc, navigation_direction);
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();

            npc_get_path_target(g_active_npc, &target);
            speed =
                (g_active_npc->path->travel_mode == 0 ? 0.03f : 0.0525f) *
                g_active_npc->path->speed * game_speed;
        }

        g_active_npc->path->current_waypoint =
            g_active_npc->path->target_waypoint;
        npc_start_current_waypoint_script(g_active_npc);
    }

    g_active_npc->path->previous_waypoint =
        g_active_npc->path->current_waypoint;
    if ((destination_type & (int)0x80000000) == 0) {
        if (g_active_npc->path->waypoints == 0) {
            npc_turn_and_face_angle(
                g_active_npc, g_active_npc->current_waypoint_angle);
        } else {
            npc_turn_and_face_angle(
                g_active_npc,
                g_active_npc
                    ->path->waypoints[g_active_npc->path->current_waypoint]
                    .angle);
        }
    }
    g_active_npc->path->use_animation_override = 0;
    g_active_npc->animation->object->pos_vel.x = 0.0f;
    g_active_npc->animation->object->pos_vel.y = 0.0f;
    g_active_npc->animation->object->pos_vel.z = 0.0f;
    g_active_npc->animation->object->flags_09_bits.bit6 = 1;
}

/*
 * Soft ceiling: 94.791664% - indexed waypoint angle loading uses addi/lfsx
 * instead of retail's equivalent add/lfs addressing.
 */
void npc_face_current_waypoint_angle(void) {
    if (g_active_npc != 0 && g_active_npc->path != 0) {
        KonquestPathData* path = g_active_npc->path;

        if (path->waypoints != 0) {
            KonquestWaypoint* waypoint;

            waypoint = &path->waypoints[path->current_waypoint];
            npc_turn_and_face_angle(g_active_npc, waypoint->angle);
        } else {
            npc_turn_and_face_angle(
                g_active_npc, g_active_npc->current_waypoint_angle);
        }
    }
}

/*
 * Soft ceiling: 98.98305% - the 2.0f relocation and equivalent FPR coloring
 * differ.
 */
void npc_travel_to_world_position(Vec* position, int travel_mode) {
    if (g_active_npc != 0 && g_active_npc->path != 0) {
        KonquestPathData* path = g_active_npc->path;
        float delta_z;
        float delta_x;

        path->destination.x = position->x;
        g_active_npc->path->destination.y = position->y;
        g_active_npc->path->destination.z = position->z;
        delta_z = g_active_npc->data->position.z - position->z;
        delta_x = g_active_npc->data->position.x - position->x;
        if (delta_x * delta_x + delta_z * delta_z > 2.0f) {
            g_active_npc->path->current_waypoint = -1;
            g_active_npc->path->target_waypoint = 0;
            g_active_npc->path->previous_waypoint = -1;
            g_active_npc->path->waypoint_count = 1;
            npc_travel_path(0x80000007, 0, travel_mode);
        }
        g_active_npc->path->table_index = -1;
    }
}

void npc_travel_path_anim_override(
    int path_id, int path_arg, int animation, int travel_mode) {
    if (g_active_npc != 0) {
        g_active_npc->path->use_animation_override = 1;
        g_active_npc->path->animation_override = animation;
        npc_travel_path(path_id, path_arg, travel_mode);
    }
}

/*
 * Soft ceiling: 97.86232% - the inline-helper algorithm differs only in GPR
 * allocation and equivalent branches to the shared false return.
 */
static int is_npc_at_destination(
    KonquestNpc* npc, int destination_type, int destination) {
    KonquestPathData* path;
    KonquestPathData* active_path;
    int current;
    int active_current;
    int next;

    next = npc_next_waypoint(npc);
    path = npc->path;
    current = path->current_waypoint;
    if (current < 0) {
        return 0;
    }
    active_path = g_active_npc->path;
    active_current = active_path->current_waypoint;
    if (next == active_current) {
        return 1;
    }
    if (destination_type == (int)0x80000007) {
        return 1;
    }

    switch (destination_type & 0x7FFFFFFF) {
    case 0:
        if (destination == 0x20000000) {
            if (active_current == active_path->waypoint_count - 1) {
                return 1;
            }
        } else if (destination == 0x10000000) {
            if (active_current == 0) {
                return 1;
            }
        } else if (current == destination) {
            return 1;
        }
        break;
    case 1:
        if ((path->waypoints[current].flags & 1) != 0) {
            int count = 0;
            int index;

            for (index = 0; index < current; index++) {
                if ((path->waypoints[index].flags & 1) != 0) {
                    count++;
                }
            }
            if (count == destination) {
                return 1;
            }
        }
        break;
    case 3:
        if (path->waypoint_count == 0) {
            return 1;
        }
        if (path->previous_waypoint != current) {
            return 1;
        }
        break;
    case 4:
        if ((path->waypoints[current].flags & 1) != 0 &&
            path->previous_waypoint != current) {
            return 1;
        }
        break;
    case 5:
    case 6:
        return 0;
    default:
        return 0;
    }
    return 0;
}

void npc_set_my_movement_weight(float root_weight, float object_weight) {
    if (g_active_npc->animation != 0) {
        AnimState* animation = (AnimState*)pdata_of_proc(
            g_active_npc->animation->proc);

        set_root_and_obj_movement_weights(
            root_weight, object_weight, animation);
    }
}

/* Soft ceiling: 89.52381% - typed active-animation boolean lowers differently. */
void npc_set_snap_to_ground(int enabled) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        state->object->flags_09_bits.bit6 = enabled;
    }
}

/*
 * Soft ceiling: 91.52252% - queued inverse-speed timing, suspension, and live
 * game-speed sleep loop match at retail size. Remaining differences are FPR
 * allocation, inline scheduling, save form, and relocation labeling.
 */
void npc_sleep(float ticks) {
    float remaining = ticks;

    if (aproc->pid == 0xA014) {
        if (g_active_npc->wait_ticks > 0.0f) {
            npc_suspend_animation_wait();
        } else {
            g_active_npc->wait_ticks =
                1.0f + remaining * inverse_game_speed;
            npc_suspend_animation_wait();
        }
    } else {
        while (remaining > 0.0f) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            remaining -= game_speed;
        }
    }
}

/*
 * Soft ceiling: 84.478264% - queued and live animation paths, duration math,
 * and the shared cmdscript suspension sequence match at retail size. Remaining
 * differences are inline scheduling, save form, and register allocation.
 */
void npc_ani_to_end(void) {
    if (aproc->pid == 0xA014) {
        if (g_active_npc->wait_ticks > 0.0f) {
            npc_suspend_animation_wait();
        } else {
            g_active_npc->wait_ticks =
                1.0f +
                ((g_active_npc->queued_animation->last_frame -
                  g_active_npc->queued_animation_frame) /
                 g_active_npc->animation_speed);
            npc_suspend_animation_wait();
        }
    } else {
        KonquestNpcAnimState* state = g_active_npc->animation;
        int has_active_animation;

        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation != 0) {
            KonquestAnimPdata* animation =
                (KonquestAnimPdata*)pdata_of_proc(state->proc);

            npc_ani_to_frame_x(animation->high_frame);
        }
    }
}

/*
 * Soft ceiling: 89.74026% - command-script suspension and direct animation
 * advance/pose paths are exact; predicate lowering, copy scheduling, save form,
 * and float relocations remain.
 */
void npc_ani_1_frame(void) {
    if (aproc->pid == 0xA014) {
        KonquestCmdScriptView* script;
        CmdScript* saved_script;
        KonquestNpc* npc;

        g_active_npc->wait_ticks = 2.0f;
        npc = g_active_npc;
        if (npc == 0) {
            return;
        }
        npc->wait_ticks -= 1.0f;
        if (npc->wait_ticks <= 0.0f) {
            npc->wait_ticks = 0.0f;
            return;
        }
        script = (KonquestCmdScriptView*)active_cmdscript;
        script->state = 2;
        saved_script = active_cmdscript;
        npc = g_active_npc;
        cmdscript_step_backward();
        memcpy(
            npc->saved_script_state,
            ((KonquestCmdScriptView*)active_cmdscript)->execution_state,
            sizeof(npc->saved_script_state));
        npc->saved_script_position =
            ((KonquestCmdScriptView*)active_cmdscript)->position;
        npc->saved_script_stack_depth = get_script_stack_depth();
        active_cmdscript = saved_script;
    } else {
        KonquestNpcAnimState* state = g_active_npc->animation;
        int has_active_animation;

        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation != 0 && state->object->bone_count != 0) {
            AnimState* animation = (AnimState*)pdata_of_proc(state->proc);

            advance_anim(animation);
            pose_anim(animation, 1);
        }
    }
}

/*
 * Soft ceiling: 86.638885% - queued blend-duration math, shared suspension,
 * and live high-frame delegation match at retail size. Remaining differences
 * are inline scheduling, save form, registers, and float relocations.
 */
void npc_ani_to_blend_frame(float blend_frames) {
    if (aproc->pid == 0xA014) {
        if (g_active_npc->wait_ticks > 0.0f) {
            npc_suspend_animation_wait();
        } else {
            g_active_npc->wait_ticks =
                1.0f +
                ((g_active_npc->queued_animation->last_frame -
                  blend_frames) /
                 g_active_npc->animation_speed);
            npc_suspend_animation_wait();
        }
    } else if (g_active_npc->animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(
                g_active_npc->animation->proc);

        npc_ani_to_frame_x(animation->high_frame - blend_frames);
    }
}

/* Matching: 99.0% - code is exact; only the TU-local float relocation differs. */
void npc_set_wait_ticks(float ticks) {
    g_active_npc->wait_ticks = ticks + 1.0f;
}

/*
 * Soft ceiling: 88.78049% - the body is exact; MWCC splits retail's r30-r31
 * stmw/lmw save and restore sequences.
 */
void npc_suspend_cmdscript(void) {
    KonquestCmdScriptView* script;
    KonquestNpc* npc;
    CmdScript* saved_script;

    if (g_active_npc == 0) {
        return;
    }
    g_active_npc->wait_ticks -= 1.0f;
    if (g_active_npc->wait_ticks <= 0.0f) {
        g_active_npc->wait_ticks = 0.0f;
        return;
    }

    script = (KonquestCmdScriptView*)active_cmdscript;
    script->state = 2;
    saved_script = active_cmdscript;
    npc = g_active_npc;
    cmdscript_step_backward();
    memcpy(
        npc->saved_script_state,
        ((KonquestCmdScriptView*)active_cmdscript)->execution_state,
        sizeof(npc->saved_script_state));
    npc->saved_script_position =
        ((KonquestCmdScriptView*)active_cmdscript)->position;
    npc->saved_script_stack_depth = get_script_stack_depth();
    active_cmdscript = saved_script;
}

/*
 * Soft ceiling: 94.540985% - queued duration math, cmdscript suspension, live
 * frame loop, game-speed lookahead, and sleep dispatch match at retail size.
 * Remaining differences are register allocation and float relocations.
 */
void npc_ani_to_frame_x(float frame) {
    if (aproc->pid == 0xA014) {
        if (g_active_npc->wait_ticks > 0.0f) {
            npc_suspend_animation_wait();
        } else {
            g_active_npc->wait_ticks =
                1.0f +
                ((g_active_npc->queued_animation_frame - frame) /
                 g_active_npc->animation_speed);
            npc_suspend_animation_wait();
        }
    } else if (g_active_npc->animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(
                g_active_npc->animation->proc);

        while (animation->frame <= frame) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            if (animation->step * game_speed + animation->frame > frame) {
                break;
            }
        }
    }
}

/*
 * Soft ceiling: 94.42277% - queued tick setup, suspension, live animation
 * loop, game-speed decrement, and sleep dispatch match at retail size. The
 * residue is FPR allocation, save form, and float relocation labeling.
 */
void npc_ani_for_x_ticks(int ticks) {
    if (aproc->pid == 0xA014) {
        if (g_active_npc->wait_ticks > 0.0f) {
            npc_suspend_animation_wait();
        } else {
            g_active_npc->wait_ticks = 1.0f + (float)ticks;
            npc_suspend_animation_wait();
        }
    } else {
        float remaining = (float)ticks;

        while (remaining > 0.0f) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            remaining -= game_speed;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/*
 * Soft ceiling: 91.3932% - table count, queued last-animation timing, live
 * per-entry transitions, ten-frame overlap, animation advancement, and sleep
 * behavior are exact. Remaining code-size delta comes from repeated typed
 * active-animation predicate and inline/register scheduling.
 */
void npc_blend_to_ani_string(int* animation_ids) {
    unsigned int count = get_row_count_for_table_by_pointer(
        konquest_pdata->waypoint_script, animation_ids);

    if (aproc->pid == 0xA014) {
        if (g_active_npc->wait_ticks > 0.0f) {
            npc_suspend_animation_wait();
        } else {
            g_active_npc->queued_animation =
                get_animation(animation_ids[count - 1]);
            g_active_npc->animation_flags = 3;
            g_active_npc->queued_animation_frame = 0.0f;
            g_active_npc->wait_ticks =
                1.0f +
                g_active_npc->queued_animation->last_frame /
                    g_active_npc->animation_speed;
            npc_suspend_animation_wait();
        }
    } else if (g_active_npc->animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(
                g_active_npc->animation->proc);
        unsigned int index;

        for (index = 0; index < count; index++, animation_ids++) {
            int animation_id = *animation_ids;

            g_active_npc->queued_animation = get_animation(animation_id);
            g_active_npc->animation_flags = 3;
            g_active_npc->queued_animation_frame = 0.0f;
            if (aproc->pid != 0xA014) {
                KonquestNpcAnimState* state = g_active_npc->animation;
                int has_active_animation;

                if (state == 0) {
                    has_active_animation = 0;
                } else if (state->object == 0) {
                    has_active_animation = 0;
                } else {
                    has_active_animation = state->proc != 0;
                }
                if (has_active_animation != 0) {
                    AniData* target = get_animation(animation_id);
                    KonquestAnimPdata* current =
                        (KonquestAnimPdata*)pdata_of_proc(state->proc);

                    current->step = 1.0f;
                    if (current->animation != target ||
                        current->flags != 3 || current->step != 1.0f) {
                        transition_to_anim_script(
                            current, target, 3, 0.1f);
                    }
                }
            }
            while (animation->frame < animation->high_frame - 10.0f) {
                npc_ani_1_frame();
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            }
        }
    }
}

/* Soft ceiling: 92.97298% - typed validity lowering and GPR coloring differ. */
void npc_set_ani_frame(float frame) {
    KonquestNpc* npc = g_active_npc;
    KonquestNpcAnimState* state = npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }

    if (has_active_animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(state->proc);

        animation->frame = frame;
        if (frame > animation->high_frame) {
            animation->frame = animation->high_frame;
        }
    } else {
        npc->queued_animation_frame = frame;
    }
}

void npc_set_ani_flags(unsigned int flags) {
    if (g_active_npc->animation != 0 &&
        g_active_npc->animation->proc != 0) {
        AnimPdata* animation =
            (AnimPdata*)pdata_of_proc(g_active_npc->animation->proc);

        animation->flags |= flags;
        return;
    }
    g_active_npc->animation_flags |= flags;
}

/*
 * Soft ceiling: 91.74775% - typed validity lowering, GPR save form, and the
 * zero-float relocation differ.
 */
void npc_blend_to_ani_with_offset(
    int animation_id, int flags, float blend, float step) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }

    if (has_active_animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(state->proc);
        MkObj* object;
        int skip_sleep;

        animation->step = step;
        transition_to_anim_script(
            animation, get_animation(animation_id), flags, blend);
        object = g_active_npc->animation->object;
        object->pos.x += animation->root_offset.x;
        object = g_active_npc->animation->object;
        object->pos.y += animation->root_offset.y;
        object = g_active_npc->animation->object;
        object->pos.z += animation->root_offset.z;

        state = g_active_npc->animation;
        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation == 1) {
            skip_sleep = !g_active_npc->wait_for_animation;
        } else {
            skip_sleep = 1;
        }
        if (skip_sleep == 0) {
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    } else {
        g_active_npc->queued_animation = get_animation(animation_id);
        g_active_npc->animation_flags = flags;
        g_active_npc->queued_animation_frame = 0.0f;
    }
}

/*
 * Soft ceiling: 82.57746% - queued state, live-animation predicate, target
 * lookup, speed update, change detection, and transition call match at retail
 * size. Residue is predicate lowering, FPR/GPR allocation, and save form.
 */
void npc_blend_to_ani(
    int animation_id, unsigned int flags, float blend, float speed) {
    g_active_npc->queued_animation = get_animation(animation_id);
    g_active_npc->animation_flags = flags;
    g_active_npc->queued_animation_frame = 0.0f;

    if (aproc->pid != 0xA014) {
        KonquestNpcAnimState* state = g_active_npc->animation;
        int has_active_animation;

        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation != 0) {
            AniData* target = get_animation(animation_id);
            KonquestAnimPdata* animation =
                (KonquestAnimPdata*)pdata_of_proc(state->proc);

            animation->step = speed;
            if (animation->animation != target ||
                animation->flags != flags || animation->step != speed) {
                transition_to_anim_script(
                    animation, target, flags, blend);
            }
        }
    }
}

/*
 * Soft ceiling: 90.46032% - queued long-wait suspension and the live infinite
 * sleep/advance loop match at retail size. Residue is inline scheduling, save
 * form, and float relocation labeling.
 */
void npc_wait_for_state_change(void) {
    if (aproc->pid == 0xA014) {
        g_active_npc->wait_ticks = 1001.0f;
        npc_suspend_animation_wait();
        return;
    }
    for (;;) {
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        npc_ani_1_frame();
    }
}

/*
 * Soft ceiling: 88.5812% - nonvolatile GPR coloring/save form, typed
 * active-animation boolean lowering, and float relocations differ.
 */
static float p_turn_and_face(void) {
    MkObj* target_object;
    int use_npc_position;
    TurnAndFacePdata* pdata;
    float target_angle;

    pdata = (TurnAndFacePdata*)pdata_of_proc(aproc);
    target_object = 0;
    use_npc_position = 0;
    if (pdata->target_kind == 0x9003) {
        target_object = pdata->target.object;
    } else {
        KonquestNpc* target_npc = pdata->target.npc;
        KonquestNpcAnimState* state = target_npc->animation;
        int has_active_animation;
        int use_data;

        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation == 1) {
            use_data = !target_npc->wait_for_animation;
        } else {
            use_data = 1;
        }
        if (use_data != 0) {
            use_npc_position = 1;
        } else {
            target_object = state->object;
        }
    }

    if (pdata->use_angle != 0) {
        target_angle = pdata->angle;
    } else {
        float destination_x = pdata->position.x;
        float destination_z = pdata->position.z;
        float target_x;
        float target_z;

        if (use_npc_position != 0) {
            target_x = pdata->target.npc->data->position.x;
            target_z = pdata->target.npc->data->position.z;
        } else {
            target_x = target_object->pos.x;
            target_z = target_object->pos.z;
        }
        target_angle = gxMathArcTanYX(
            destination_x - target_x, destination_z - target_z);
    }

    if (use_npc_position != 0) {
        pdata->target.npc->data->angle_y = target_angle;
        return -1.0f;
    }
    if (get_konquest_game_mode() == 4) {
        target_object->ang.y = target_angle;
        return -1.0f;
    }

    {
        float difference = ang_sub_ang(target_angle, target_object->ang.y);
        float magnitude;

        target_object->hide_flag_bits.pin_animation = 0;
        if (difference >= 0.0f) {
            magnitude = difference;
        } else {
            magnitude = -difference;
        }
        if (magnitude > 0.1f) {
            if (difference < 0.0f) {
                target_object->ang.y -= 0.1f;
            } else {
                target_object->ang.y += 0.1f;
            }
            return 1.0f;
        }
    }
    target_object->ang.y = target_angle;
    return -1.0f;
}

/* Near match: 71.927925%, 16 bytes short of retail. Path/turn-process latches,
 * active-animation gate, duplicated next-waypoint selection, angle calculation,
 * process pdata initialization, and wait loop match. Residue is inlined-helper
 * register allocation, pointer truth lowering, and save scheduling. */
void npc_turn_and_face_next_waypoint(void) {
    KonquestPathData* path = g_active_npc->path;
    MkProc* turn_proc = g_active_npc->turn_proc;

    if (path == 0) {
        return;
    }
    if (turn_proc != 0) {
        if (turn_proc->instance != g_active_npc->turn_proc_instance) {
            turn_proc = 0;
        }
    } else {
        turn_proc = 0;
    }
    if (turn_proc == 0) {
        int active = npc_event_has_active_animation(g_active_npc);
        int turn_immediately;

        if (active == 1) {
            turn_immediately = g_active_npc->wait_for_animation == 0;
        } else {
            turn_immediately = 1;
        }

        if (turn_immediately != 0) {
            float angle;

            if (path->waypoints == 0) {
                angle = 0.0f;
            } else {
                KonquestWaypoint* waypoint =
                    &path->waypoints[npc_next_waypoint(g_active_npc)];
                KonquestNpcData* data = g_active_npc->data;

                angle = gxMathArcTanYX(
                    waypoint->position.x - data->position.x,
                    waypoint->position.z - data->position.z);
            }
            g_active_npc->data->angle_y = angle;
            return;
        }

        {
            TurnAndFacePdata* pdata = 0;
            MkProc* proc = _create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_turn_and_face, sizeof(*pdata),
                (MkHdr**)&pdata);

            if (proc != 0) {
                float angle;

                if (path->waypoints == 0) {
                    angle = 0.0f;
                } else {
                    KonquestWaypoint* waypoint =
                        &path->waypoints[npc_next_waypoint(g_active_npc)];
                    KonquestNpcData* data = g_active_npc->data;

                    angle = gxMathArcTanYX(
                        waypoint->position.x - data->position.x,
                        waypoint->position.z - data->position.z);
                }
                pdata->angle = angle;
                pdata->target_kind = 0xA002;
                pdata->target.npc = g_active_npc;
                pdata->use_angle = 1;
                g_active_npc->turn_proc = proc;
                g_active_npc->turn_proc_instance = proc->instance;
            }
        }

        while (1) {
            turn_proc = g_active_npc->turn_proc;
            if (turn_proc != 0) {
                if (turn_proc->instance !=
                    g_active_npc->turn_proc_instance) {
                    turn_proc = 0;
                }
            } else {
                turn_proc = 0;
            }
            if (turn_proc == 0) {
                break;
            }
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/* Soft ceiling: 89.52381% - typed active-animation boolean lowers differently. */
void npc_set_pinanim_flag(int enabled) {
    KonquestNpcAnimState* state = g_active_npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkObj* object = state->object;

        object->hide_flag_bits.pin_animation = enabled;
    }
}

/*
 * Soft ceiling: 86.55738% - hero latch, world delta, atan argument order,
 * process creation, pdata size, angle, and object target are exact. Residue is
 * latch branch shape, zero-vector relocation, saves, and register allocation.
 */
void hero_turn_to_face_position(const Vec* position) {
    MkObj* hero = konquest_pdata->monk;
    Vec direction = {0.0f, 0.0f, 0.0f};
    TurnAndFacePdata* pdata = 0;

    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->monk_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0) {
        float angle;

        direction.x = position->x - hero->pos.x;
        direction.z = position->z - hero->pos.z;
        angle = gxMathArcTanYX(direction.x, direction.z);
        if (_create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_hero_turn_and_face,
                sizeof(*pdata), (MkHdr**)&pdata) != 0) {
            pdata->angle = angle;
            pdata->target.object = hero;
        }
    }
}

/*
 * Matching: retail order resolves the final TU-local float relocation.
 */
static float p_hero_turn_and_face(void) {
    TurnAndFacePdata* pdata = (TurnAndFacePdata*)pdata_of_proc(aproc);
    float target_angle = pdata->angle;
    MkObj* object = pdata->target.object;
    float difference = ang_sub_ang(target_angle, object->ang.y);
    float magnitude;

    object->hide_flag_bits.pin_animation = 0;
    if (difference >= 0.0f) {
        magnitude = difference;
    } else {
        magnitude = -difference;
    }
    if (magnitude > 0.1f) {
        if (difference < 0.0f) {
            object->ang.y -= 0.1f;
        } else {
            object->ang.y += 0.1f;
        }
        return 1.0f;
    }
    object->ang.y = target_angle;
    return -1.0f;
}

/*
 * Soft ceiling: 87.396225% - typed latch joins, active-animation boolean
 * lowering, pdata reload scheduling, and the 1.0f relocation differ.
 */
void npc_turn_and_face_angle(KonquestNpc* npc, float angle) {
    MkProc* turn_proc = npc->turn_proc;

    if (turn_proc != 0) {
        if ((unsigned int)turn_proc->instance ==
            npc->turn_proc_instance) {
            /* The latched process is still live. */
        } else {
            turn_proc = 0;
        }
    } else {
        turn_proc = 0;
    }
    if (turn_proc == 0) {
        KonquestNpcAnimState* state = npc->animation;
        int has_active_animation;
        int use_data;

        if (state == 0) {
            has_active_animation = 0;
        } else if (state->object == 0) {
            has_active_animation = 0;
        } else {
            has_active_animation = state->proc != 0;
        }
        if (has_active_animation == 1) {
            use_data = !npc->wait_for_animation;
        } else {
            use_data = 1;
        }
        if (use_data != 0) {
            npc->data->angle_y = angle;
            return;
        }

        {
            TurnAndFacePdata* pdata;

            turn_proc = _create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_turn_and_face,
                sizeof(TurnAndFacePdata), (MkHdr**)&pdata);
            if (turn_proc != 0) {
                pdata->angle = angle;
                pdata->saved_pin_animation =
                    state->object->hide_flag_bits.pin_animation;
                pdata->target_kind = 0xA002;
                pdata->target.npc = npc;
                pdata->use_angle = 1;
                npc->turn_proc = turn_proc;
                npc->turn_proc_instance = turn_proc->instance;
            }
        }

        while ((turn_proc = npc->turn_proc) != 0 &&
               (unsigned int)turn_proc->instance ==
                   npc->turn_proc_instance) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/*
 * Near match: 91.01755%, 20 bytes short of retail. Monk/process latches,
 * ownership-sensitive animation reloads, both turn-process setups, and the
 * wait loop match. Pointer truth lowering and nonvolatile allocation remain.
 */
void npc_turn_and_face_player(int turn_player) {
    MkObj* monk = konquest_pdata->monk;
    KonquestNpcAnimState* state;
    int has_active_animation;

    if (monk != 0) {
        if ((unsigned int)monk->hdr.instance ==
            konquest_pdata->monk_instance) {
            /* The latched object is still live. */
        } else {
            monk = 0;
        }
    } else {
        monk = 0;
    }

    state = g_active_npc->animation;
    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        MkProc* turn_proc = g_active_npc->turn_proc;
        TurnAndFacePdata* pdata;

        if (turn_proc != 0) {
            if ((unsigned int)turn_proc->instance ==
                g_active_npc->turn_proc_instance) {
                /* The latched process is still live. */
            } else {
                turn_proc = 0;
            }
        } else {
            turn_proc = 0;
        }
        if (turn_proc == 0) {
            turn_proc = _create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_turn_and_face,
                sizeof(TurnAndFacePdata), (MkHdr**)&pdata);
            if (turn_proc != 0) {
                pdata->target_kind = 0xA002;
                pdata->target.npc = g_active_npc;
                pdata->saved_pin_animation =
                    g_active_npc->animation->object->hide_flag_bits
                        .pin_animation;
                g_active_npc->animation->object->hide_flag_bits
                    .pin_animation = 0;
                pdata->use_angle = 0;
                pdata->position.x = monk->pos.x;
                pdata->position.y = monk->pos.y;
                pdata->position.z = monk->pos.z;
                g_active_npc->turn_proc = turn_proc;
                g_active_npc->turn_proc_instance = turn_proc->instance;
            }
        }

        if (turn_player != 0) {
            turn_proc = _create_mkproc_generic_nostack(
                0xA01E, 0x1F, p_turn_and_face,
                sizeof(TurnAndFacePdata), (MkHdr**)&pdata);
            if (turn_proc != 0) {
                pdata->target.object = monk;
                pdata->saved_pin_animation =
                    monk->hide_flag_bits.pin_animation;
                monk->hide_flag_bits.pin_animation = 0;
                pdata->target_kind = 0x9003;
                pdata->use_angle = 0;
                pdata->position.x =
                    g_active_npc->animation->object->pos.x;
                pdata->position.y =
                    g_active_npc->animation->object->pos.y;
                pdata->position.z =
                    g_active_npc->animation->object->pos.z;
            }
        }

        while ((turn_proc = g_active_npc->turn_proc) != 0 &&
               (unsigned int)turn_proc->instance ==
                   g_active_npc->turn_proc_instance) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/*
 * Soft ceiling: 89.70886% - process object, hero latch, world delta, angle
 * conversion, normalized quadrant math, signed conversion, and return match at
 * retail size. Residue is latch branches, zero-vector relocations, and GPRs.
 */
int npc_get_collision_direction_in_script(void) {
    KonquestObjectScriptPdata* pdata =
        (KonquestObjectScriptPdata*)pdata_of_proc(aproc);
    MkObj* hero = konquest_pdata->monk;
    int direction = 0;

    if (hero != 0) {
        if (hero->hdr.instance != konquest_pdata->monk_instance) {
            hero = 0;
        }
    } else {
        hero = 0;
    }
    if (hero != 0 && pdata != 0) {
        MkObj* object = pdata->object;

        if (object != 0) {
            Vec delta = {0.0f, 0.0f, 0.0f};
            Vec angles = {0.0f, 0.0f, 0.0f};

            if (hero != 0 && object != 0) {
                delta.x = hero->pos.x - object->pos.x;
                delta.y = hero->pos.y - object->pos.y;
                delta.z = hero->pos.z - object->pos.z;
                v3_to_xy_ang(&angles, &delta);
                direction = (int)(norm_angle(
                    0.7853982f +
                    ang_sub_ang(angles.y, object->ang.y)) /
                    1.5707964f);
            }
        }
    }
    return direction;
}

void npc_shove_reaction_standard_shutdown(void) {
    g_active_npc->reaction_active = 0;
    g_active_npc->ignore_events = 0;
    g_active_npc->reaction_mode = 0;
    if (g_active_npc->path != 0) {
        g_active_npc->path->reaction_state = -1;
    }
}

/*
 * Soft ceiling: 91.163635% - the typed body is exact; MWCC uses split
 * nonvolatile saves/restores, and the float relocations differ.
 */
void npc_run_shove_animation(int animation_id) {
    KonquestAnimPdata* animation_pdata =
        (KonquestAnimPdata*)pdata_of_proc(g_active_npc->animation->proc);
    AniData* animation = get_animation(animation_id);

    if (animation != 0) {
        float final_frame = anim_script_lastframe(animation) - 10.0f;

        transition_to_anim_script(animation_pdata, animation, 3, 0.1f);
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        while (animation_pdata->frame < final_frame) {
            npc_ani_1_frame();
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
}

/*
 * Matching: 99.88372% - code is exact; only the zero-float relocation differs.
 */
void npc_shove_reaction_standard_setup(void) {
    g_active_npc->reaction_active = 1;
    g_active_npc->ignore_events = 1;
    g_active_npc->reaction_mode = 1;
    if (g_active_npc->animation != 0 &&
        g_active_npc->animation->object != 0) {
        g_active_npc->animation->object->pos_vel.z = 0.0f;
        g_active_npc->animation->object->pos_vel.y = 0.0f;
        g_active_npc->animation->object->pos_vel.x = 0.0f;
        g_active_npc->animation->object->ang_vel.z = 0.0f;
        g_active_npc->animation->object->ang_vel.y = 0.0f;
        g_active_npc->animation->object->ang_vel.x = 0.0f;
    }
}

/*
 * Matching: 96.94444% - the validated monk latch uses an equivalent branch
 * schedule.
 */
int npc_punch_reaction_check_data(void) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);
    MkObj* monk = konquest_pdata->monk;

    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (reaction == 0 || monk == 0) {
        return 0;
    }
    if (reaction->npc == 0 || reaction->object == 0) {
        return 0;
    }
    return 1;
}

/*
 * Matching: 99.71591% - instructions are exact; only float relocations differ.
 */
void npc_run_punch_animation(
    int animation_id, int flags, int unused, int use_blend,
    void* script_args, float gravity) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);

    if (reaction != 0) {
        KonquestAnimPdata* animation_pdata =
            (KonquestAnimPdata*)pdata_of_proc(reaction->animation_proc);

        if (animation_pdata != 0) {
            AniData* animation = get_animation(animation_id);

            if (animation != 0) {
                float final_frame =
                    anim_script_lastframe(animation) - 10.0f;

                if (use_blend != 0) {
                    transition_to_anim_script(
                        animation_pdata, animation, flags, 0.05f);
                } else {
                    set_anim_script(animation_pdata, animation, 3);
                }
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                reaction->object->pos.y = animation_pdata->root_offset.y;
                reaction->object->gravity = gravity;

                while (animation_pdata->frame < final_frame) {
                    if (gravity != 0.0f) {
                        reaction->object->flags_08_bits.moving = 1;
                    }
                    npc_ani_1_frame();
                    _mkproc_sleep_ticks = 1.0f;
                    ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
                }
            }
        }
    }
}

/*
 * Matching: 97.25% - the validated monk latch uses an equivalent branch
 * schedule.
 */
void npc_snap_to_face_monk(void) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);
    MkObj* monk = konquest_pdata->monk;

    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (monk != 0 && reaction != 0 && reaction->object != 0) {
        reaction->object->ang.y = gxMathArcTanYX(
            monk->pos.x - reaction->object->pos.x,
            monk->pos.z - reaction->object->pos.z);
    }
}

/*
 * Matching: 94.5% - the animation-presence boolean uses an equivalent
 * three-instruction normalization sequence.
 */
void npc_punch_reaction_standard_shutdown(void) {
    KonquestNpcAnimState* animation;
    int has_animation;

    g_active_npc->reaction_active = 0;
    g_active_npc->ignore_events = 0;
    g_active_npc->reaction_mode = 0;
    if (g_active_npc->path != 0) {
        g_active_npc->path->reaction_state = -1;
    }

    animation = g_active_npc->animation;
    if (animation == 0) {
        has_animation = 0;
    } else if (animation->object == 0) {
        has_animation = 0;
    } else {
        has_animation = animation->proc != 0;
    }
    if (has_animation != 0) {
        animation->object->ground_colls = &npc_ground_colls;
    }
}

void npc_prepare_for_unconscious_state(void) {
    g_active_npc->reaction_active = 0;
    g_active_npc->ignore_events = 1;
    g_active_npc->reaction_mode = 0;
    if (g_active_npc->path != 0) {
        g_active_npc->path->reaction_state = -1;
    }
}

/*
 * Matching: 99.88372% - code is exact; only the zero-float relocation differs.
 */
void npc_punch_reaction_standard_setup(void) {
    KonquestReactionPdata* reaction =
        (KonquestReactionPdata*)pdata_of_proc(aproc);

    if (reaction->object != 0) {
        reaction->object->pos_vel.z = 0.0f;
        reaction->object->pos_vel.y = 0.0f;
        reaction->object->pos_vel.x = 0.0f;
        reaction->object->ang_vel.z = 0.0f;
        reaction->object->ang_vel.y = 0.0f;
        reaction->object->ang_vel.x = 0.0f;
        reaction->object->ground_colls = &npc_punched_ground_colls;
    }
    g_active_npc->reaction_active = 1;
    g_active_npc->ignore_events = 1;
    g_active_npc->reaction_mode = 1;
    npc_notify_nearby_npcs_that_player_hit_someone(g_active_npc);
}

/* Near match: 88.63492% at exact retail size. Visible-list cleanup,
 * loaded-tile gate, squared XZ radius, and violent-event dispatch match;
 * residue is branch scheduling and register allocation. */
#pragma optimize_for_size on
static void npc_notify_nearby_npcs_that_player_hit_someone(
    KonquestNpc* source) {
    MkPtr* link = konquest_pdata->visible_npc_list;

    while (link != 0) {
        KonquestNpc* npc = (KonquestNpc*)link->hdr;

        if (link->instance != npc->hdr.instance) {
            MkPtr* next = link->next;

            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (npc != 0 && npc->data != source->data) {
                KonquestTileOrigin* tile =
                    get_nth_tile_struct(npc->tile_index);

                if (tile != 0 && tile->loaded != 0) {
                    float dz = source->data->position.z -
                               npc->data->position.z;
                    float dx = source->data->position.x -
                               npc->data->position.x;

                    if (dx * dx + dz * dz < 100.0f &&
                        npc->state_58 != 6) {
                        npc_signal_event(npc, 6);
                    }
                }
            }
            link = link->next;
        }
    }
}
#pragma optimize_for_size reset



/*
 * Soft ceiling: 88.24272% - queued idle setup, long suspension, live script
 * transition, step reset, and perpetual advance/sleep loop match at retail
 * size. Residue is inline scheduling, save form, registers, and relocations.
 */
void npc_stand_still(void) {
    if (aproc->pid == 0xA014) {
        g_active_npc->queued_animation =
            get_animation(g_active_npc->data->idle_animation);
        g_active_npc->animation_flags = 0;
        g_active_npc->queued_animation_frame = 0.0f;
        g_active_npc->wait_ticks = 1001.0f;
        npc_suspend_animation_wait();
        return;
    } else {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(
                g_active_npc->animation->proc);
        AniData* idle = get_animation(g_active_npc->data->idle_animation);

        if (animation->animation != idle) {
            transition_to_anim_script(animation, idle, 0, 0.05f);
            animation->step = 1.0f;
            _mkproc_sleep_ticks = 1.0f;
            ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
        }
    }
    for (;;) {
        npc_ani_1_frame();
        _mkproc_sleep_ticks = 1.0f;
        ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
    }
}void npc_set_ani_speed(float speed) {
    g_active_npc->animation_speed = speed;
    if (g_active_npc->animation != 0) {
        AnimPdata* animation =
            (AnimPdata*)pdata_of_proc(g_active_npc->animation->proc);

        animation->step = speed;
    }
}

/*
 * Matching: retail order resolves the integer-conversion constant relocation.
 */
void npc_change_path_speed(float speed) {
    if (g_active_npc->path != 0) {
        g_active_npc->path->speed = (float)(int)speed;
    }
}

/*
 * Soft ceiling: 83.34286% - the retail-complete typed algorithm remains separated
 * from retail by stale-latch branch polarity, GPR coloring, and split saves.
 */
KonquestNpc* find_npc_by_data(KonquestNpcData* data) {
    return npc_find_by_data_inline(data);
}

/* Near match: 95.80357% at exact retail size. Monk/game-mode gating, active
 * animation validation, distance fade, priority, alpha, and event dispatch
 * match; only latch branches and register scheduling remain. */
/* Near match: 92.6134% at exact retail size. Animation snapshot, repeated
 * object ownership loads, editor collision update, repulsion, monk gating,
 * ground trace, typed render flags, and position handoff match. Residue is
 * register allocation, scheduling, and one equivalent branch polarity. */
static void npc_post_sleep(void) {
    KonquestNpcAnimState* animation = g_active_npc->animation;
    KonquestNpcData* data = g_active_npc->data;

    if (animation->proc != 0) {
        KonquestAnimPdata* animation_pdata =
            (KonquestAnimPdata*)pdata_of_proc(animation->proc);

        if (animation_pdata != 0) {
            g_active_npc->queued_animation = animation_pdata->animation;
            g_active_npc->animation_flags = animation_pdata->flags;
            g_active_npc->queued_animation_frame = animation_pdata->frame;
        }
    }

    if (animation->object != 0) {
        if (konquest_editor_mode_on == 0) {
            Vec collision_position;

            collision_position.x = data->position.x;
            collision_position.y = data->position.y;
            collision_position.z = data->position.z;
            collision_position.y += -1.0f;
            if (animation->editor_object != 0) {
                update_collision_obj_pos(
                    animation->editor_object, &collision_position);
            }
            if ((g_active_npc->flags_1C & 0x80) != 0) {
                animation->object->hide_flag_bits.pin_animation = 0;
            }
            if ((g_active_npc->flags_1D & 4) != 0) {
                Vec movement;

                movement.x = animation->object->pos.x - data->position.x;
                movement.y = animation->object->pos.y - data->position.y;
                movement.z = animation->object->pos.z - data->position.z;
                if (npc_repel_against_global_collision_list(
                        &data->position, &movement, &collision_position,
                        0x10000) != 0) {
                    animation->object->pos.x = collision_position.x;
                    animation->object->pos.z = collision_position.z;
                    animation->object->hide_flag_bits.pin_animation = 0;
                }
            }

            {
                Vec segment_start;
                Vec segment_end;
                Vec hit_point;
                KonquestNpc* monk;

                segment_end.x = animation->object->pos.x;
                segment_end.y = animation->object->pos.y;
                segment_end.z = animation->object->pos.z;
                segment_start.x = animation->object->pos.x;
                segment_start.y = animation->object->pos.y;
                segment_start.z = animation->object->pos.z;
                segment_end.y = -50.0f;
                segment_start.y = 50.0f;
                if (animation->object->oid != 0xFFFF9010) {
                    monk = konquest_pdata->monk_npc;
                    if (monk != 0) {
                        if (monk->hdr.instance !=
                            konquest_pdata->monk_npc_instance) {
                            monk = 0;
                        }
                    } else {
                        monk = 0;
                    }
                    if (monk != g_active_npc ||
                        get_konquest_game_mode() != 8) {
                        if (collide_segment_against_global_collision_list_quads(
                                &segment_start, &segment_end,
                                &hit_point) != 0) {
                            animation->object->flags_08_bits.moving = 1;
                            animation->object->gravity = -0.00325f;
                            animation->object->hide_flag_bits.pin_animation = 0;
                            animation->object->ground_colls_y = hit_point.y;
                        } else {
                            if (animation->object->ground_colls_y != 0.0f) {
                                animation->object->pos.y = 0.0f;
                            }
                            animation->object->flags_08_bits.moving = 0;
                            animation->object->ground_colls_y = 0.0f;
                        }
                    }
                }
            }
        }
        if ((animation->owner_npc->flags_1D & 2) != 0) {
            animation->object->flags_09_bits.launched = 0;
        } else {
            animation->object->flags_09_bits.launched = 1;
        }
        data->position.x = animation->object->pos.x;
        data->position.y = animation->object->pos.y;
        data->position.z = animation->object->pos.z;
        data->angle_y = animation->object->ang.y;
    }
    g_active_npc = 0;
}

/* Near match: 78.44934%, eight bytes short of retail. Special asynchronous
 * model loading, pdata ownership, animation/runtime snapshots, event setup,
 * saved script restoration/execution, and state reset are complete; residue
 * is pointer-truth lowering, GPR allocation, and save scheduling. */
float p_npc_proc(void) {
    KonquestNpc* npc = g_active_npc;

    if (npc_event_has_active_animation(npc) == 0) {
        if (npc->data->events[7].script_function == 1) {
            int flags = 0x80000000;
            MkProc* loader = get_mkproc_bigstack(&flags);

            if (create_mkproc(
                    0x1F, loader, 0xA01B, p_npc_load_model,
                    &npc->animation->hdr) != 0) {
                MkPtr* animation_link = first_mkptr(&loader->pdata_list);

                animation_link->f.no_own = 1;
            }
            while (npc_event_has_active_animation(npc) == 0) {
                _mkproc_sleep_ticks = 1.0f;
                ((KonquestNpcProcSleepVtable*)aproc->vtbl)->sleep();
            }
        } else {
            load_model_for_npc(npc);
        }
    }

    if (konquest_editor_mode_on == 0) {
        int previous_state;

        npc = g_active_npc;
        previous_state = npc->state_58;
        if ((npc->flags_1D & 0x10) != 0) {
            return 1.0f;
        }
        if (npc_event_has_active_animation(npc) != 0) {
            KonquestAnimPdata* animation =
                (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);

            npc->saved_animation_step = animation->step;
            npc->animation->object->flags_09_bits.bit6 = 0;
            npc->saved_object_flags =
                npc->animation->object->flags_word_08;
            npc->saved_gravity = npc->animation->object->gravity;
        }
        npc->saved_position = npc->data->position;
        if (g_event_tbl[npc->state_58].setup != 0) {
            g_event_tbl[npc->state_58].setup(npc);
        }
        if (npc->events[npc->state_58].script_function != 0) {
            unsigned int script_function =
                npc->events[npc->state_58].script_function;
            ScriptSlot* slot = konquest_pdata->waypoint_script;

            cmdscript_set_parameters(active_cmdscript, 1, npc->data);
            if (g_active_npc != 0) {
                if (g_active_npc->saved_event_script != script_function) {
                    g_active_npc->wait_ticks = 0.0f;
                }
                if (g_active_npc->wait_ticks > 0.0f) {
                    memcpy(
                        active_cmdscript->stack_mem,
                        g_active_npc->saved_script_state,
                        sizeof(g_active_npc->saved_script_state));
                    active_cmdscript->func_name =
                        (char*)(&slot->bytecode[
                            slot->func_defs[script_function - 1]
                                .code_offset] - 1);
                    active_cmdscript->pc =
                        (unsigned int*)g_active_npc->saved_script_position;
                    active_cmdscript->stack_sp =
                        &active_cmdscript->stack_mem[
                            g_active_npc->saved_script_stack_depth];
                    active_cmdscript->stack_end =
                        active_cmdscript->stack_sp + 1;
                } else {
                    g_active_npc->saved_event_script = script_function;
                    cmdscript_setup_execution(slot, script_function);
                }
            }
            cmdscript_execute(konquest_pdata->waypoint_script);
            g_active_npc->wait_ticks = 0.0f;
        }
        if (previous_state == g_active_npc->state_58) {
            npc_force_state_for_npc(g_active_npc, 0);
        }
    }
    return 1.0f;
}

/* Matching: 97.5% - code is exact; only the return float relocation differs. */
float p_npc_idle(void) {
    return 1.0f;
}

static void npc_pre_wake(void) {
    KonquestNpcProcessPdata* process =
        (KonquestNpcProcessPdata*)pdata_of_proc(aproc);
    KonquestNpc* npc = process->npc;

    g_active_npc = npc;
    if (process->update_enabled != 0) {
        KonquestNpc* monk = konquest_pdata->monk_npc;

        if (monk != 0) {
            if (monk->hdr.instance != konquest_pdata->monk_npc_instance) {
                monk = 0;
            }
        } else {
            monk = 0;
        }
        if (monk != npc ||
            (get_konquest_game_mode() != 8 &&
             get_konquest_game_mode() != 9)) {
            KonquestNpcAnimState* state = npc->animation;
            int active;

            if (state == 0) {
                active = 0;
            } else if (state->object == 0) {
                active = 0;
            } else {
                active = state->proc != 0;
            }
            if (active != 0) {
                if (npc->camera_distance_squared > 1600.0f &&
                    (npc->flags_1D & 0x20) == 0) {
                    float alpha;

                    obj_set_all_sobjs_priority(state->object, 0x13);
                    alpha = 1.0f -
                        ((npc->camera_distance_squared - 1600.0f) / 900.0f);
                    if (alpha < 0.0f) {
                        alpha = 0.0f;
                    }
                    if (alpha > 1.0f) {
                        alpha = 1.0f;
                    }
                    npc->animation->alpha =
                        (unsigned int)(255.0f * alpha);
                    obj_for_all_atomics_set_material_alpha(
                        npc->animation->object, npc->animation->alpha);
                } else if (state->alpha < 255) {
                    obj_set_all_sobjs_priority(state->object, 0x13);
                    npc->animation->alpha += 20;
                    if (npc->animation->alpha > 255) {
                        npc->animation->alpha = 255;
                    }
                    obj_for_all_atomics_set_material_alpha(
                        npc->animation->object, npc->animation->alpha);
                } else {
                    obj_set_all_sobjs_priority(state->object, 0x10);
                }
            }
        }
    }
    npc_resolve_events(g_active_npc);
}

/*
 * Matching: 99.545456% - code is exact; only the -1.0f relocation differs.
 */
static float p_npc_load_model(void) {
    KonquestModelLoadPdata* pdata = (KonquestModelLoadPdata*)apdata;

    load_model_for_npc(pdata->npc);
    return -1.0f;
}

/* Near match: 89.20513% - exact mode gates, transition latch, event scan, and
 * typed callback dispatch; pointer-latch branches and GPR coloring remain. */
static void npc_resolve_events(KonquestNpc* npc) {
    KonquestNpc* monk;
    int mode;
    int event_index;
    int found;

    found = 0;
    mode = get_konquest_game_mode();
    monk = konquest_pdata->monk_npc;
    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_npc_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (mode != 4) {
        if (mode == 3) {
            return;
        }
        if ((monk == 0 || monk->state_58 != 7) &&
            (npc->flags_1C & 8) == 0 &&
            (npc->flags_1D & 0x10) == 0) {
            if (npc->state_change_pending != 0) {
                npc->state_change_pending = 0;
                return;
            }
            event_index = 0;
            while (event_index < 8) {
                KonquestNpcEvent* event = &npc->events[event_index];

                if (event->enabled != 0 && npc->state_58 != event_index &&
                    g_event_tbl[event_index].check != 0 &&
                    g_event_tbl[event_index].check(event->distance) != 0) {
                    found = 1;
                    break;
                }
                event_index++;
            }
            if (found != 0) {
                npc_set_state_for_npc(npc, event_index);
            }
        }
    }
}

static void npc_set_state_for_npc(KonquestNpc* npc, int event_index) {
    if (npc->events[event_index].enabled != 0) {
        npc_force_state_for_npc(npc, event_index);
    }
}

/*
 * Soft ceiling: 88.65169% - shared lookup, visibility teardown, list removal,
 * and nonnegative count clamp are exact; latch branches and GPRs remain.
 */
void remove_npc(KonquestNpcData* data) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (konquest_editor_mode_on == 0 && npc != 0) {
        npc_make_invisible(npc);
        konquest_pdata->npc_count--;
        mk_pull_destroy(&npc->hdr, &konquest_pdata->npc_list);
        if (konquest_pdata->npc_count < 0) {
            konquest_pdata->npc_count = 0;
        }
    }
}

/*
 * Soft ceiling: 88.96342% - allocation, initialization, ownership, table
 * metadata, position, event copies, art lookup, and list insertion are exact.
 * The typed event-pointer loop uses different induction registers/scheduling,
 * and MWCC splits retail's nonvolatile save/restore sequence.
 */
void add_npc(KonquestNpcData* data) {
    KonquestNpc* npc = (KonquestNpc*)get_mkhdr(
        &vtbl_konquest_npc_struct, sizeof(*npc));

    if (npc != 0) {
        zero_pdata_payload(sizeof(*npc), &npc->hdr);
        npc->state_268 = -3;
        npc->state_264 = -3;
        npc->state_26C = -3;
        npc->distance_270 = __float_max[0];
        npc->value_274 = 0.0f;
        npc->distance_278 = __float_max[0];
    }

    npc->data = 0;
    npc->path = 0;
    npc->animation = 0;
    npc->flags = 0;
    npc->state_58 = 0;
    npc->queued_animation = 0;
    npc->queued_animation_frame = 0.0f;
    npc->animation_flags = 0;
    npc->camera_distance_squared = 0.0f;
    npc->conversation_count = 0;
    npc->punch_count = 0;
    npc->saved_script_stack_depth = 0;
    npc->wait_ticks = 0.0f;
    npc->animation_speed = 1.0f;
    npc->turn_proc = 0;
    npc->turn_proc_instance = 0;
    npc->flags = 0;
    memset(npc->runtime_state, 0, sizeof(npc->runtime_state));
    npc->saved_animation_step = 1.0f;
    npc->data = data;

    if (npc->path == 0) {
        KonquestPathData* path = (KonquestPathData*)get_mkhdr(
            &vtbl_path_data_struct, sizeof(*path));

        if (path != 0) {
            zero_pdata_payload(sizeof(*path), &path->hdr);
            path->travel_mode = 0;
            path->speed = 1.0f;
            path->table_index = -2;
        }
        npc->path = path;
    }
    if (data->timed_events == 0) {
        npc->path->table_index = 0;
    }

    npc->data_table_index = get_table_index_by_pointer(
        konquest_pdata->waypoint_script, data);
    npc->name = get_name_of_table_by_pointer(
        konquest_pdata->waypoint_script, data);
    npc->initial_position.x = data->position.x;
    npc->initial_position.y = data->position.y;
    npc->initial_position.z = data->position.z;
    npc->current_waypoint_angle = data->angle_y;

    if ((unsigned int)npc->data_table_index >= 0x10000) {
        if (npc->hdr.instance != 0) {
            npc->hdr.typed_vtbl->destroy(&npc->hdr);
        }
        return;
    }

    npc->tile_index = get_tile_from_position(&npc->data->position);
    {
        KonquestNpcEvent* event = npc->events;
        KonquestNpcEventDefinition* definition = data->events;
        int count = 8;

        do {
            event->script_function = definition->script_function;
            event->distance = definition->distance;
            if (definition->script_function != 0) {
                event->enabled = 1;
            } else {
                event->enabled = 0;
            }
            event++;
            definition++;
            count--;
        } while (count != 0);
    }
    if (strcmp(data->name, "damashi_npc") != 0) {
        unsigned int art_id = get_artid_of_named_item_in_slot(
            0x6002B, data->name, 1);

        if (art_id != 0) {
            npc->art_id = art_id;
        } else {
            npc->art_id = -1;
        }
    }
    konquest_pdata->npc_count++;
    mk_append(&npc->hdr, &konquest_pdata->npc_list);
}

/* Near match: 93.1341%, 12 bytes short of retail. The three guarded list
 * traversals, saved command-script restoration, and distinct visible-list
 * insertion branches are exact; residue is string relocation, boolean
 * normalization, and register scheduling. */
void start_running_npcs(void) {
    NpcManagerPdata* manager;
    MkProc* manager_proc;
    MkPtr* link;
    int index;

    _create_mkproc_generic_nostack(0xA01C, 0x1F, p_update_npc_shadows, 0, 0);
    manager_proc = find_mkproc_pid(0xA014);
    if (manager_proc == 0) {
        manager_proc = _create_mkproc_generic_nostack(
            0xA014, 0x1F, p_npc_manager, sizeof(*manager), (MkHdr**)&manager);
        if (manager_proc != 0) {
            zero_pdata_payload(sizeof(*manager), (MkHdr*)manager);
            manager->visible_tile_set = konquest_pdata->visible_tile_set;
            manager->nearest_npc_distance = -1.0f;
            for (index = 0; index < 22; index++) {
                NpcManagerModelSlot* slot = &manager->models[index];
                memset(slot, 0, sizeof(*slot));
                slot->available = 1;
            }
            for (index = 0; index < 2; index++) {
                NpcManagerModelSlot* slot = &manager->special_models[index];
                memset(slot, 0, sizeof(*slot));
                slot->available = 1;
            }
            npc_manager_pdata = manager;
        }
    }
    if (manager_proc != 0) {
        set_process_as_scriptable(manager_proc);
    }

    {
        MkPtr** npc_list = &konquest_pdata->npc_list;

        if (npc_list != 0) {
            link = *npc_list;
            while (link != 0) {
                KonquestNpc* npc = (KonquestNpc*)link->hdr;
                if (link->instance != npc->hdr.instance) {
                    MkPtr* next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else {
                    if (npc->name != 0 &&
                        strcmp(npc->name, "npc_sektor_seller") == 0) {
                        npc->events[4].script_function =
                            get_script_function_by_name(
                                konquest_pdata->waypoint_script,
                                "npc_block_only");
                    }
                    link = link->next;
                }
            }
        }
    }

    {
        MkPtr** npc_list = &konquest_pdata->npc_list;

        if (npc_list != 0) {
            link = *npc_list;
            while (link != 0) {
                KonquestNpc* npc = (KonquestNpc*)link->hdr;
                if (link->instance != npc->hdr.instance) {
                    MkPtr* next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else {
                    unsigned int script_function =
                        npc->data->events[7].script_function;
                    if (script_function != 0) {
                        ScriptSlot* slot = konquest_pdata->waypoint_script;
                        g_active_npc = npc;
                        if (npc != 0) {
                            if (npc->saved_event_script != script_function) {
                                npc->wait_ticks = 0.0f;
                            }
                            if (g_active_npc->wait_ticks > 0.0f) {
                                memcpy(
                                    active_cmdscript->stack_mem,
                                    g_active_npc->saved_script_state,
                                    sizeof(g_active_npc->saved_script_state));
                                active_cmdscript->func_name =
                                    (char*)(
                                        slot->func_defs[script_function - 1]
                                            .name_offset +
                                        slot->string_reloc - 1);
                                active_cmdscript->pc = (unsigned int*)
                                    g_active_npc->saved_script_position;
                                active_cmdscript->stack_sp =
                                    &active_cmdscript->stack_mem[
                                        g_active_npc
                                            ->saved_script_stack_depth];
                                active_cmdscript->stack_end =
                                    active_cmdscript->stack_sp + 1;
                            } else {
                                g_active_npc->saved_event_script =
                                    script_function;
                                cmdscript_setup_execution(
                                    slot, script_function);
                            }
                        }
                        cmdscript_execute(
                            konquest_pdata->waypoint_script);
                    }
                    link = link->next;
                }
            }
        }
    }

    g_active_npc = 0;
    npc_update(1);
    get_visible_tile_set(konquest_pdata->visible_tile_set);
    discard_list(&konquest_pdata->visible_npc_list);
    {
        MkPtr** npc_list = &konquest_pdata->npc_list;

        if (npc_list != 0) {
            link = *npc_list;
            while (link != 0) {
                KonquestNpc* npc = (KonquestNpc*)link->hdr;
                if (link->instance != npc->hdr.instance) {
                    MkPtr* next = link->next;
                    link->hdr = 0;
                    destroy_mkptr(link);
                    link = next;
                } else {
                    KonquestTileOrigin* tile =
                        get_nth_tile_struct(npc->tile_index);
                    if (tile != 0) {
                        if (tile->loaded != 0) {
                            mk_insert(
                                &npc->hdr,
                                &konquest_pdata->visible_npc_list);
                        } else if (npc_event_has_active_animation(npc) != 0) {
                            mk_insert(
                                &npc->hdr,
                                &konquest_pdata->visible_npc_list);
                        } else if ((npc->flags_1C & 0x20) != 0) {
                            mk_insert(
                                &npc->hdr,
                                &konquest_pdata->visible_npc_list);
                        }
                    }
                    link = link->next;
                }
            }
        }
    }
}

/* Matching: retail's compact count-controlled loop and save form are selected
 * by the localized size mode. */
#pragma optimize_for_size on
void add_npc_list_to_world(int* npc_ids) {
    unsigned int count = get_row_count_for_table_by_pointer(
        konquest_pdata->waypoint_script, npc_ids);
    unsigned int index = 0;
    int npc_id = *npc_ids;

    while (index < count) {
        add_npc((KonquestNpcData*)npc_id);
        npc_id = npc_ids[1];
        npc_ids++;
        index++;
    }
}
#pragma optimize_for_size reset



/* Near match: 90.81739%, exact retail size. Validated monk and animation
 * ownership, collision-mode and distance gates, vector initialization order,
 * vector-to-angle conversion, normalized angular subtraction, and absolute
 * threshold. Residue is pointer-latch lowering and register allocation. */
int npc_hit_by_punch(
    KonquestNpc* npc, float maximum_distance, float maximum_angle) {
    MkObj* monk = konquest_pdata->monk;
    KonquestNpcAnimState* state;
    MkObj* object;
    int active;
    Vec delta;
    Vec angles;
    float difference;

    if (monk != 0) {
        if (monk->hdr.instance != konquest_pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (monk == 0) {
        return 0;
    }
    state = npc->animation;
    if (state == 0) {
        active = 0;
    } else if (state->object == 0) {
        active = 0;
    } else {
        active = state->proc != 0;
    }
    if (active == 0) {
        return 0;
    }
    if (konquest_pdata->collision_mode != 3) {
        return 0;
    }
    object = state->object;
    if (dist_xz_to_xz(&object->pos, &monk->pos) >
        maximum_distance) {
        return 0;
    }
    delta = (Vec){0.0f, 0.0f, 0.0f};
    angles = (Vec){0.0f, 0.0f, 0.0f};
    delta.x = object->pos.x - monk->pos.x;
    delta.y = object->pos.y - monk->pos.y;
    delta.z = object->pos.z - monk->pos.z;
    v3_to_xy_ang(&angles, &delta);
    difference = ang_sub_ang(
        norm_angle(monk->ang.y), norm_angle(angles.y));
    if (difference < 0.0f) {
        difference = -difference;
    }
    if (difference <= maximum_angle) {
        return 1;
    }
    return 0;
}

/* Matching: 99.7619% - code is exact; only the 1.0f relocation differs. */
static void npc_shoved_setup(KonquestNpc* npc) {
    if (npc->animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);

        npc->ignore_events = 1;
        animation->step = 1.0f;
    }
}

static void npc_shoved_cleanup(KonquestNpc* npc) {
    npc->ignore_events = 0;
    npc->reaction_active = 0;
}

static void npc_override_cleanup(KonquestNpc* npc) {
    npc->animation_override = 0;
    npc->ignore_events = 0;
    npc->reaction_active = 0;
}

static void npc_override_setup(KonquestNpc* npc) {
    npc->animation_override = 1;
    npc->ignore_events = 1;
    npc->reaction_active = 1;
}

/*
 * Soft ceiling: 94.23077% - typed active-animation boolean lowering and the
 * 1.0f relocation differ.
 */
static void npc_punched_setup(KonquestNpc* npc) {
    KonquestNpcAnimState* state = npc->animation;
    int has_active_animation;

    if (state == 0) {
        has_active_animation = 0;
    } else if (state->object == 0) {
        has_active_animation = 0;
    } else {
        has_active_animation = state->proc != 0;
    }
    if (has_active_animation != 0) {
        KonquestAnimPdata* animation =
            (KonquestAnimPdata*)pdata_of_proc(state->proc);

        npc->animation->object->flags_09_bits.bit6 = 0;
        npc->ignore_events = 1;
        animation->step = 1.0f;
    }
}

static void npc_plyr_violent_setup(KonquestNpc* npc) {
}

/*
 * Soft ceiling: 96.447365% - equivalent pointer-validation latch branches
 * and GPR coloring differ.
 */
static int plyr_leave_area_check(float distance) {
    KonquestNpcPdata* pdata = konquest_pdata;
    KonquestNpc* npc = g_active_npc;
    MkObj* monk = pdata->monk;
    int is_near;

    if (monk != 0) {
        if (monk->hdr.instance != pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (monk == 0) {
        is_near = 0;
    } else if (dist_xz_to_xz(
                   &npc->data->position, &monk->pos) < distance) {
        is_near = 1;
    } else {
        is_near = 0;
    }
    return is_near == 0;
}

/*
 * Soft ceiling: 86.97369% - the body is exact; pointer-validation latch/GPR
 * coloring and equivalent nonzero booleanization differ.
 */
static int plyr_near_check(float distance) {
    KonquestNpcPdata* pdata = konquest_pdata;
    KonquestNpc* npc = g_active_npc;
    MkObj* monk = pdata->monk;
    int is_near;

    if (monk != 0) {
        if (monk->hdr.instance != pdata->monk_instance) {
            monk = 0;
        }
    } else {
        monk = 0;
    }
    if (monk == 0) {
        is_near = 0;
    } else if (dist_xz_to_xz(
                   &npc->data->position, &monk->pos) < distance) {
        is_near = 1;
    } else {
        is_near = 0;
    }
    return is_near != 0;
}

void npc_enable_event(int event_index, int enabled) {
    if (g_active_npc != 0 && event_index >= 0 && event_index < 7) {
        g_active_npc->events[event_index].enabled = enabled;
    }
}

/*
 * Soft ceiling: 95.19481% - shared lookup, bounds check, stride, and store are
 * exact; remaining differences are latch branches and GPR allocation.
 */
void npc_enable_his_event(
    KonquestNpcData* data, int event_index, int enabled) {
    KonquestNpc* npc = npc_find_by_data_inline(data);

    if (event_index >= 0 && event_index < 7) {
        npc->events[event_index].enabled = enabled;
    }
}

/* Near match: 80.61818% - exact retail size and list/update control flow;
 * active-animation boolean lowering, saves, and float relocation remain. */
float p_npc_manager(void) {
    MkPtr* link;

    link = konquest_pdata->visible_npc_list;
    while (link != 0) {
        KonquestNpc* npc = (KonquestNpc*)link->hdr;

        if (link->instance != npc->hdr.instance) {
            MkPtr* next = link->next;

            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            int active;
            int should_update;

            if (npc->animation == 0) {
                active = 0;
            } else if (npc->animation->object == 0) {
                active = 0;
            } else {
                active = npc->animation->proc != 0;
            }
            if (active == 1) {
                should_update = npc->wait_for_animation == 0;
            } else {
                should_update = 1;
            }
            if (should_update != 0) {
                npc_invisible_update(npc);
            }
            link = link->next;
        }
    }
    return 1.0f;
}

/* Near match: 83.8% - exact event setup, animation snapshot, script resume,
 * execution, and state completion algorithm; scheduling and GPRs remain. */
static void npc_invisible_update(KonquestNpc* npc) {
    if (konquest_editor_mode_on == 0) {
        int previous_state;

        g_active_npc = npc;
        if ((npc->flags_1D & 0x10) == 0) {
            npc_resolve_events(npc);
            previous_state = npc->state_58;
            if (npc->events[previous_state].script_function != 0) {
                unsigned int script_function;
                ScriptSlot* slot;

                if (npc->wait_ticks == 0.0f) {
                    int active;

                    if (npc->animation == 0) {
                        active = 0;
                    } else if (npc->animation->object == 0) {
                        active = 0;
                    } else {
                        active = npc->animation->proc != 0;
                    }
                    if (active != 0) {
                        KonquestAnimPdata* animation =
                            (KonquestAnimPdata*)pdata_of_proc(
                                npc->animation->proc);

                        npc->saved_animation_step = animation->step;
                        npc->animation->object->flags_09_bits.bit6 = 0;
                        npc->saved_object_flags =
                            npc->animation->object->flags_word_08;
                        npc->saved_gravity = npc->animation->object->gravity;
                    }
                    npc->saved_position = npc->data->position;
                    if (g_event_tbl[npc->state_58].setup != 0) {
                        g_event_tbl[npc->state_58].setup(npc);
                    }
                }
                cmdscript_set_parameters(active_cmdscript, 1, npc->data);
                slot = konquest_pdata->waypoint_script;
                script_function =
                    npc->events[npc->state_58].script_function;
                if (g_active_npc != 0) {
                    if (g_active_npc->saved_event_script != script_function) {
                        g_active_npc->wait_ticks = 0.0f;
                    }
                    if (g_active_npc->wait_ticks > 0.0f) {
                        memcpy(
                            active_cmdscript->stack_mem,
                            g_active_npc->saved_script_state,
                            sizeof(g_active_npc->saved_script_state));
                        active_cmdscript->func_name =
                            (char*)(&slot->bytecode[
                                slot->func_defs[script_function - 1]
                                    .code_offset] - 1);
                        active_cmdscript->pc =
                            (unsigned int*)g_active_npc->saved_script_position;
                        active_cmdscript->stack_sp =
                            &active_cmdscript->stack_mem[
                                g_active_npc->saved_script_stack_depth];
                        active_cmdscript->stack_end =
                            active_cmdscript->stack_sp + 1;
                    } else {
                        g_active_npc->saved_event_script = script_function;
                        cmdscript_setup_execution(slot, script_function);
                    }
                }
                cmdscript_execute(konquest_pdata->waypoint_script);
                if (*active_cmdscript->pc == 0 &&
                    previous_state == npc->state_58) {
                    npc_force_state_for_npc(npc, 0);
                }
            }
        }
        g_active_npc = 0;
    }
}

/* Near match: 74.718124% at exact retail size. Timed-event dispatch, model-age
 * update, tile/list rebuilding, two-model throttle, visibility transitions,
 * and stale-link cleanup are complete; residue is GPR allocation, list-loop
 * branch shaping, and nonvolatile save scheduling. */
void npc_update(int update_all) {
    MkPtr* link;
    NpcManagerModelSlot* slot;
    int models_made_visible = 0;
    int index;

    npc_dispatch_timed_events_for_all_npcs();
    slot = npc_manager_pdata->models;
    for (index = 0; index < 22; index++) {
        if (slot->available != 0) {
            slot->age += 1.0f;
        }
        slot++;
    }
    if (update_all == 0 && --wait_ticks > 0) {
        return;
    }

    wait_ticks = 7;
    npc_manager_pdata->visible_tile_set = konquest_pdata->visible_tile_set;
    get_visible_tile_set(konquest_pdata->visible_tile_set);
    discard_list(&konquest_pdata->visible_npc_list);

    link = konquest_pdata->npc_list;
    while (link != 0) {
        KonquestNpc* npc = (KonquestNpc*)link->hdr;

        if (link->instance != npc->hdr.instance) {
            MkPtr* next = link->next;

            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            KonquestTileOrigin* tile =
                get_nth_tile_struct(npc->tile_index);

            if (tile != 0) {
                if (tile->loaded != 0 ||
                    npc_event_has_active_animation(npc) != 0 ||
                    (npc->flags_1C & 0x20) != 0) {
                    mk_insert(
                        &npc->hdr, &konquest_pdata->visible_npc_list);
                }
            }
            link = link->next;
        }
    }

    npc_manager_pdata->nearest_npc_distance = -1.0f;
    link = konquest_pdata->visible_npc_list;
    while (link != 0) {
        KonquestNpc* npc = (KonquestNpc*)link->hdr;

        if (link->instance != npc->hdr.instance) {
            MkPtr* next = link->next;

            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (npc_check_visibility_and_calc_dist(npc) != 0) {
                if ((npc->flags_1C & 0x10) == 0) {
                    if (models_made_visible < 2 || update_all != 0) {
                        npc_make_visible(npc);
                        models_made_visible++;
                    } else {
                        wait_ticks = 0;
                    }
                }
            } else if ((npc->flags_1C & 0x10) != 0) {
                npc_make_invisible(npc);
            }
            link = link->next;
        }
    }
}

/*
 * Matching: 99.44444% - code is exact; only the return-value float relocation
 * differs in this partial TU.
 */
static float p_update_npc_shadows(void) {
    npc_shadow_update();
    return 1.0f;
}

KonquestNpcStateDef g_event_tbl[8] = {
    {0, 0, 0},
    {plyr_near_check, 0, 0},
    {plyr_leave_area_check, 0, 0},
    {0, 0, 0},
    {0, npc_punched_setup, 0},
    {0, npc_shoved_setup, npc_shoved_cleanup},
    {0, npc_plyr_violent_setup, 0},
    {0, npc_override_setup, npc_override_cleanup},
};

static KonquestNpcShadows npc_shadows = {0};

GroundCollTable npc_punched_ground_colls[5] = {
    {7, {0.0f, 0.0f, 0.0f}, 0.04f},
    {8, {0.0f, 0.0f, 0.0f}, 0.04f},
    {0, {0.0f, 0.0f, 0.0f}, 0.075f},
    {10, {0.0f, 0.0f, 0.0f}, 0.075f},
    {-1, {0.0f, 0.0f, 0.0f}, 0.0f},
};

/* Near match: 77.0814%, four bytes short of retail. Stale-link cleanup,
 * schedule activation/expiration, state reset, and every byte-width flag
 * update match; residue is list-loop branch shaping and GPR allocation. */
static void npc_dispatch_timed_events_for_all_npcs(void) {
    MkPtr** list = &konquest_pdata->npc_list;
    MkPtr* link;

    if (list == 0) {
        return;
    }
    link = *list;

    while (link != 0) {
        KonquestNpc* npc = (KonquestNpc*)link->hdr;

        if (link->instance != npc->hdr.instance) {
            MkPtr* next = link->next;

            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
        } else {
            if (npc->data->timed_events != 0 && npc->state_58 == 0) {
                if (npc->next_timed_event == 0) {
                    npc->timed_event_flags &= (unsigned char)~0x10;
                    setup_current_and_next_events(npc, 1);
                } else if ((npc->timed_event_flags & 0x10) != 0 &&
                           (npc->timed_event_flags & 0x80) == 0) {
                    npc->next_timed_event = 0;
                    setup_current_and_next_events(npc, 0);
                    npc_force_state_for_npc(npc, 0);
                    npc->timed_event_flags &= (unsigned char)~0x10;
                    npc->flags_1D &= (unsigned char)~0x10;
                    if ((npc->flags_1C & 2) != 0 ||
                        (npc->flags_1D & 1) != 0) {
                        npc->flags_1C &= (unsigned char)~8;
                    }
                    npc->flags_1D &= (unsigned char)~1;
                    npc->flags_1C &= (unsigned char)~2;
                } else {
                    npc_check_next_event(npc);
                }
            }
            link = link->next;
        }
    }
}

#pragma optimize_for_size on

/* Near match: 96.42308% under the authentic TU-wide -O4,p flags with
 * retail-evidenced localized size lowering. Current-day selection, both event
 * script slots, path setup, and future-event selection are exact. */
static void setup_current_and_next_events(
    KonquestNpc* npc, int initialize) {
    KonquestTimedEvent* event;
    KonquestTimedEvent* current_event = 0;
    const KonquestTime* current_time = &konquest_pdata->current_time;

    event = npc->data->timed_events;
    if (event != 0) {
        int count = get_row_count_for_table_by_pointer(
            konquest_pdata->waypoint_script, event);

        while (count-- != 0) {
            if (event->path_id != 0 &&
                is_valid_event_time(&event->time) != 0 &&
                is_this_a_current_event_for_today(&event->time) != 0) {
                current_event = npc_which_event_is_more_recent(
                    current_time, current_event, event);
            }
            event++;
        }
    }
    if (current_event != 0) {
        if (npc->animation_override == 0) {
            npc_set_event_script(
                npc, 0, current_event->script_function);
            if (current_event->event_slot_3_script != 0) {
                npc_set_event_script(
                    npc, 3, current_event->event_slot_3_script);
            }
            npc_setup_path_for_event(npc, current_event, initialize);
        }
    } else {
        npc->events[0].script_function = 0;
    }
    npc_find_next_timed_event(npc);
}

/* Near match: 93.12712%. Deadline, pending-path handoff, script/path
 * activation, state reset, and next-event reselection are exact. Remaining
 * residue is helper inlining/scheduling and the portable null diagnostic. */
static void npc_check_next_event(KonquestNpc* npc) {
    if (npc->animation_override == 0 &&
        is_time_a_greater_than_time_b(
            &npc->next_event_time,
            &konquest_pdata->current_time) == 0) {
        if ((npc->flags_1D & 0x10) != 0) {
            if ((npc->flags_1C & 2) != 0) {
                npc->flags_1D |= 1;
                npc->flags_1C &= (unsigned char)~2;
            }
            npc->flags_1D &= (unsigned char)~0x10;
            npc_force_state_for_npc(npc, 0);
            npc->punch_count = 0;
        }
        if ((npc->flags_1D & 1) == 0 &&
            (npc->flags_1C & 2) == 0) {
            KonquestTimedEvent* event = npc->next_timed_event;

            if (npc->animation_override == 0) {
                npc_set_event_script(
                    npc, 0, event->script_function);
                if (event->event_slot_3_script != 0) {
                    npc_set_event_script(
                        npc, 3, event->event_slot_3_script);
                }
                npc_setup_path_for_event(npc, event, 0);
            }
            npc_force_state_for_npc(npc, 0);
            npc_find_next_timed_event(npc);
        }
    }
}

#pragma optimize_for_size reset



/* Near match: 88.28704% - exact state-transition algorithm and retail table;
 * pointer truth normalization and split GPR saves/restores add four emitted
 * instructions relative to retail. */
void npc_force_state_for_npc(KonquestNpc* npc, int next_state) {
    CmdScript* script;
    KonquestAnimPdata* animation;
    int active;
    int old_state;

    npc->wait_ticks = 0.0f;
    if (npc->proc != 0) {
        script = get_cmdscript_for_proc(npc->proc);
        script->stack_sp = script->stack_mem;
        script->stack_end = script->stack_sp + 1;
    }
    old_state = npc->state_58;
    npc->state_58 = next_state;
    if (g_event_tbl[old_state].cleanup != 0) {
        g_event_tbl[old_state].cleanup(npc);
    }
    if (npc->path != 0) {
        npc->path->use_animation_override = 0;
    }
    if (npc->animation == 0) {
        active = 0;
    } else if (npc->animation->object == 0) {
        active = 0;
    } else {
        active = npc->animation->proc != 0;
    }
    if (active != 0) {
        animation = (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);
        animation->step = npc->saved_animation_step;
        npc->animation->object->flags_word_08 = npc->saved_object_flags;
        npc->animation->object->gravity = npc->saved_gravity;
    }
    if (((npc->flags_1C & 0x80) != 0 ||
         (npc->flags_1C & 4) != 0) && npc->state_58 != 7) {
        npc->flags_1C &= (unsigned char)~0x80;
        npc->flags_1C &= (unsigned char)~4;
    }
    if (next_state != 0) {
        npc->state_change_pending = 1;
    }
    if (npc->proc != 0) {
        if (npc->animation == 0) {
            active = 0;
        } else if (npc->animation->object == 0) {
            active = 0;
        } else {
            active = npc->animation->proc != 0;
        }
        if (active != 0) {
            xfer_proc(npc->proc, p_npc_proc);
        }
    }
}

/* Near match: 86.43519% - exact state-transition algorithm and callback ABI;
 * pointer truth normalization and split GPR saves/restores account for the
 * remaining compiler-emission-only size difference. */
void npc_xfer(
    KonquestNpc* npc, MkProcEntryFn entry, int next_state) {
    CmdScript* script;
    KonquestAnimPdata* animation;
    int active;
    int old_state;

    npc->wait_ticks = 0.0f;
    if (npc->proc != 0) {
        script = get_cmdscript_for_proc(npc->proc);
        script->stack_sp = script->stack_mem;
        script->stack_end = script->stack_sp + 1;
    }
    old_state = npc->state_58;
    npc->state_58 = next_state;
    if (g_event_tbl[old_state].cleanup != 0) {
        g_event_tbl[old_state].cleanup(npc);
    }
    if (npc->path != 0) {
        npc->path->use_animation_override = 0;
    }
    if (npc->animation == 0) {
        active = 0;
    } else if (npc->animation->object == 0) {
        active = 0;
    } else {
        active = npc->animation->proc != 0;
    }
    if (active != 0) {
        animation = (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);
        animation->step = npc->saved_animation_step;
        npc->animation->object->flags_word_08 = npc->saved_object_flags;
        npc->animation->object->gravity = npc->saved_gravity;
    }
    if (((npc->flags_1C & 0x80) != 0 ||
         (npc->flags_1C & 4) != 0) && npc->state_58 != 7) {
        npc->flags_1C &= (unsigned char)~0x80;
        npc->flags_1C &= (unsigned char)~4;
    }
    if (next_state != 0) {
        npc->state_change_pending = 1;
    }
    if (npc->proc != 0) {
        if (npc->animation == 0) {
            active = 0;
        } else if (npc->animation->object == 0) {
            active = 0;
        } else {
            active = npc->animation->proc != 0;
        }
        if (active != 0) {
            xfer_proc(npc->proc, entry);
        }
    }
}/* Near match: 89.27273%, four bytes from retail. Door/table path resolution,
 * preserved-path handoff, event flags, and restart policy are exact; only
 * branch/register scheduling remains. */
static void npc_setup_path_for_event(
    KonquestNpc* npc, KonquestTimedEvent* event, int preserve_path) {
    void* path;
    int table_index = 0;
    int row_count = 0;

    if (event->path_id > 1) {
        path = get_door_path(event->path_id);
        if (path != 0) {
            table_index = event->path_id;
            row_count = 4;
        } else {
            return;
        }
    } else {
        path = event->path;
        if (path != 0) {
            table_index = get_table_index_by_pointer(
                konquest_pdata->waypoint_script, path);
            row_count = get_row_count_for_table_by_pointer(
                konquest_pdata->waypoint_script, path);
        }
    }

    if (preserve_path != 0 &&
        ((npc->flags_1C & 2) != 0 || (npc->flags_1D & 1) != 0)) {
        if (npc->path->table_index >= 0x10000000) {
            if (npc->path->waypoints != path &&
                (npc->flags_1D & 0x10) == 0) {
                npc_set_path(
                    npc, npc->path->waypoints, npc->path->table_index,
                    npc->path->waypoint_count, 3, 0);
                npc_set_path(npc, path, table_index, row_count, 0, 1);
                npc->flags_1C &= (unsigned char)~2;
                npc->flags_1D &= (unsigned char)~1;
            }
            return;
        }
        npc->flags_1C &= (unsigned char)~2;
        npc->flags_1D &= (unsigned char)~1;
    }
    if (npc->path->table_index == -2 ||
        ((npc->timed_event_flags & 0x10) != 0 &&
         ((npc->flags_1C & 2) != 0 || (npc->flags_1D & 1) != 0))) {
        npc_set_path(npc, path, table_index, row_count, 0, 0);
        return;
    }
    npc_set_path(npc, path, table_index, row_count, 0, 1);
}/*
 * Soft ceiling: 89.5% - the typed comparisons are exact; equivalent shared
 * current-time addressing and branch scheduling differ.
 */
static int is_this_a_current_event_for_today(
    const KonquestTime* event) {
    const KonquestTime* current = &konquest_pdata->current_time;

    if (event->year != -1 && event->year != current->year) {
        return 0;
    }
    if (event->month != -1 && event->month != current->month) {
        return 0;
    }
    if (event->day_of_month == -1 && event->day_of_week == -1) {
        return 1;
    }
    if (event->day_of_month == -1 &&
        event->day_of_week == current->day_of_week) {
        if (current->hour > event->hour) {
            return 1;
        }
        if (current->hour == event->hour &&
            current->minute >= event->minute) {
            return 1;
        }
        return 0;
    }
    if (event->day_of_month == current->day_of_month &&
        event->day_of_week == -1) {
        if (current->hour > event->hour) {
            return 1;
        }
        if (current->hour == event->hour &&
            current->minute >= event->minute) {
            return 1;
        }
        return 0;
    }
    if (event->day_of_month == current->day_of_month &&
        event->day_of_week == current->day_of_week) {
        if (current->hour > event->hour) {
            return 1;
        }
        if (current->hour == event->hour &&
            current->minute >= event->minute) {
            return 1;
        }
    }
    return 0;
}

/* Near match: 99.68421% at exact retail size. The signed render latch,
 * post-guard color initialization, render-state/light/alpha call order, typed
 * callback ABI, and cleanup match; only one render-state argument register
 * differs. */
#pragma optimize_for_size on
static RpAtomic* shadow_render_callback(RpAtomic* atomic) {
    RpLight* light;
    RpWorld* world;
    RpAtomic* result;

    if (npc_shadows.render_disabled != 0) {
        return 0;
    }
    light = RpLightCreate(2);
    world = (RpWorld*)RwEngineInstance->curWorld;
    {
        RwRGBAReal color = {1.0f, 1.0f, 1.0f, 1.0f};

        gc_enable_alpha_writes(1);
        if (npc_shadows.clear_alpha_pending != 0) {
            clear_alpha_channel();
            npc_shadows.clear_alpha_pending = 0;
        }
        RpLightSetColor(light, &color);
    }
    if (world != 0) {
        RpWorldAddLight(world, light);
    }
    RwEngineInstance->fpRenderStateSet(10, 9);
    RwEngineInstance->fpRenderStateSet(11, 8);
    result = npc_shadows.render(atomic, &npc_shadows);
    RwEngineInstance->fpRenderStateSet(10, 5);
    RwEngineInstance->fpRenderStateSet(11, 6);
    gc_enable_alpha_writes(0);
    if (light != 0) {
        if (world != 0) {
            RpWorldRemoveLight(world, light);
        }
        RpLightDestroy(light);
    }
    return result;
}
#pragma optimize_for_size reset

/* Near match: 98.333336% at exact retail size. Compact lowering reproduces the
 * 15-object traversal, material lookup, packed color construction, and cached
 * alpha store; only two loop-register assignments differ. */
#pragma optimize_for_size on
void npc_shadow_set_alpha(int alpha) {
    int shadow_alpha = 255 - alpha;
    int index;

    for (index = 0; index < 15; index++) {
        RpMaterial* material =
            obj_find_material_by_id(npc_shadows.objects[index], 0);
        RpMaterialColor color = {0xFFFFFFFF};

        color.red = (unsigned char)shadow_alpha;
        color.blue = (unsigned char)shadow_alpha;
        color.green = (unsigned char)shadow_alpha;
        color.alpha = (unsigned char)shadow_alpha;
        material->color = color;
    }
    npc_shadows.alpha = shadow_alpha;
}
#pragma optimize_for_size reset

/* Near match: 90.695656%, 12 bytes over retail. All 15-slot visibility,
 * signed flag semantics, grounded-height packed color, three atomic reloads,
 * projection staging, corrected packed sqrt lookup, float position copies,
 * distance scaling, reveal, and bone projection match. Residue is GPR/FPR
 * allocation and instruction scheduling. */
#pragma optimize_for_size on
void npc_shadow_update(void) {
    KonquestCameraPositionView* camera =
        ((KonquestCameraView*)Camera)->position;
    RpMaterialColor color = {0xFFFFFFFF};
    float camera_x;
    float camera_z;
    int index;

    if (npc_shadows.update_disabled != 0) {
        return;
    }
    camera_x = camera->x;
    camera_z = camera->z;
    current_npc_count = 0;
    for (index = 0; index < 15; index++) {
        MkObj* shadow = npc_shadows.objects[index];
        KonquestNpc* npc = npc_manager_pdata->visible_npcs[index];

        shadow->hide_flag_bits.hidden = 1;
        if (npc != 0) {
            KonquestNpc* monk = konquest_pdata->monk_npc;

            if (monk != 0) {
                if (monk->hdr.instance !=
                    konquest_pdata->monk_npc_instance) {
                    monk = 0;
                }
            } else {
                monk = 0;
            }
            if (npc->data != monk->data) {
                int active = npc_event_has_active_animation(npc);
                int skip_shadow;

                if (active == 1) {
                    skip_shadow = npc->wait_for_animation == 0;
                } else {
                    skip_shadow = 1;
                }
                if (skip_shadow == 0) {
                    MkObj* object = npc->animation->object;

                    if (object != 0) {
                        float scale;

                        if (object->ground_colls_y < 0.0f) {
                            scale =
                                (-0.3f - object->ground_colls_y) / -0.15f;
                        } else {
                            scale =
                                (0.5f - object->ground_colls_y) / 0.4f;
                        }
                        scale *= npc_shadows.scales[index];
                        if (!(scale < 0.1f)) {
                            float projected_x;
                            float projected_y;
                            float projected_z;
                            float radius;
                            float distance_squared;
                            float shadow_scale;
                            int alpha;

                            if (scale > 1.0f) {
                                scale = 1.0f;
                            }
                            alpha = (int)(scale * npc_shadows.alpha);
                            color.red = (unsigned char)alpha;
                            color.blue = (unsigned char)alpha;
                            color.green = (unsigned char)alpha;
                            color.alpha = (unsigned char)alpha;
                            npc_shadows.materials[index]->color = color;
                            if ((((MkSobj*)obj_first_sobj(shadow))
                                     ->atomic->interpolator.flags & 2) != 0) {
                                _rpAtomicResyncInterpolatedSphere(
                                    ((MkSobj*)obj_first_sobj(shadow))->atomic);
                            }
                            projected_x = object->pos.x *
                                npc_shadows.projection.right.x +
                                1.4f * npc_shadows.projection.up.x;
                            projected_x = object->pos.z *
                                npc_shadows.projection.at.x + projected_x;
                            projected_x = object->pos.x - projected_x;
                            projected_y = object->pos.x *
                                npc_shadows.projection.right.y +
                                1.4f * npc_shadows.projection.up.y;
                            projected_y = object->pos.z *
                                npc_shadows.projection.at.y + projected_y;
                            projected_y = 0.0f - projected_y;
                            projected_z = object->pos.x *
                                npc_shadows.projection.right.z +
                                1.4f * npc_shadows.projection.up.z;
                            projected_z = object->pos.z *
                                npc_shadows.projection.at.z + projected_z;
                            projected_z = object->pos.z - projected_z;
                            radius = npc_fast_sqrt(
                                projected_z * projected_z +
                                (projected_x * projected_x +
                                 projected_y * projected_y));
                            ((MkSobj*)obj_first_sobj(shadow))
                                ->atomic->boundingSphere.radius = radius;
                            {
                                float delta_x = camera_x - object->pos.x;
                                float delta_y = 0.0f;
                                float delta_z = camera_z - object->pos.z;

                                distance_squared = delta_z * delta_z +
                                    (delta_x * delta_x + delta_y * delta_y);
                            }
                            if (distance_squared < 169.0f) {
                                shadow_scale = 1.0f;
                            } else if (distance_squared < 1225.0f) {
                                shadow_scale =
                                    1.0f +
                                    ((npc_fast_sqrt(distance_squared) -
                                      13.0f) /
                                     3.0f);
                            } else {
                                continue;
                            }
                            current_npc_count++;
                            shadow->hide_flag_bits.hidden = 0;
                            shadow->pos.x = object->pos.x;
                            shadow->pos.y = object->pos.y;
                            shadow->pos.z = object->pos.z;
                            shadow->flags_08_bits.bit7 = 1;
                            if (shadow_scale > 3.0f) {
                                shadow_scale = 3.0f;
                            }
                            if (shadow_scale < 1.2f) {
                                shadow_scale = 1.2f;
                            }
                            set_shadow_bones(shadow, object, shadow_scale);
                        }
                    }
                }
            }
        }
    }
    npc_shadows.clear_alpha_pending = 1;
}
#pragma optimize_for_size reset

/*
 * Soft ceiling: retail gives the local matrix 16-byte stack alignment. The
 * portable matrix declaration preserves the algorithm and exact object
 * accesses, leaving only frame offsets and FPR allocation different.
 */
void npc_shadow_set_light_angle(const Vec* angles) {
    MKMATRIX rotation;
    float projection_x;
    float projection_z;

    if (angles == 0) {
        return;
    }
    if (angles->x < 0.52359873f) {
        return;
    }

    YXZ_angles_to_MKMATRIX(angles, &rotation);
    projection_x = rotation.at.x / rotation.at.y;
    projection_z = rotation.at.z / rotation.at.y;
    npc_shadows.light_direction.x = rotation.at.x;
    npc_shadows.light_direction.y = rotation.at.y;
    npc_shadows.light_direction.z = rotation.at.z;
    MKMatrixSetIdentity(&npc_shadows.projection);
    npc_shadows.projection.up.x = -projection_x;
    npc_shadows.projection.up.y = 0.0f;
    npc_shadows.projection.up.z = -projection_z;
}

/*
 * Recovered bone projection using the shared RenderWare/Midway matrix layout.
 * The source and shadow skeleton counts must agree, as in retail.
 */
static void set_shadow_bones(MkObj* shadow, MkObj* source, float scale) {
    RwMatrix inverse;
    MKMATRIX bone_projection;
    float light_x;
    float light_y;
    float light_z;
    unsigned int index;

    if (shadow == 0 || source == 0 || source->bone_count != shadow->bone_count) {
        return;
    }

    memcpy(shadow->field_24, source->field_24, sizeof(RwMatrix));
    RwMatrixInvert(&inverse, source->field_24);
    light_x =
        npc_shadows.light_direction.x * inverse.right.x +
        npc_shadows.light_direction.y * inverse.up.x +
        npc_shadows.light_direction.z * inverse.at.x;
    light_y =
        npc_shadows.light_direction.x * inverse.right.y +
        npc_shadows.light_direction.y * inverse.up.y +
        npc_shadows.light_direction.z * inverse.at.y;
    light_z =
        npc_shadows.light_direction.x * inverse.right.z +
        npc_shadows.light_direction.y * inverse.up.z +
        npc_shadows.light_direction.z * inverse.at.z;
    append_oblique_projection(
        (ObliqueMatrixCell*)shadow->field_24,
        (ObliqueMatrixCell*)source->field_24,
        (ObliqueMatrixCell*)&npc_shadows.projection);

    MKMatrixSetIdentity(&bone_projection);
    bone_projection.pos.x = -(light_x / light_y);
    bone_projection.pos.y = 0.0f;
    bone_projection.pos.z = -(light_z / light_y);
    for (index = 0; index < source->bone_count; index++) {
        RwMatrix* shadow_matrix = &shadow->bones[index]->matrix;

        append_oblique_projection(
            (ObliqueMatrixCell*)shadow_matrix,
            (ObliqueMatrixCell*)&source->bones[index]->matrix,
            (ObliqueMatrixCell*)&bone_projection);
        if (index != 7 && index != 8) {
            shadow_matrix->right.x *= scale;
            shadow_matrix->at.z *= scale;
        } else {
            shadow_matrix->right.x *= 0.6f;
            shadow_matrix->at.z *= 0.6f;
        }
    }
    RwFrameUpdateObjects(shadow->frame);
}

/* Matching: 99.96183% - the 0.01f literal relocation differs. */
static void append_oblique_projection(
    ObliqueMatrix result, ObliqueMatrix left, ObliqueMatrix right) {
    result[0].value =
        left[2].value * right[8].value +
        (left[0].value * right[0].value +
         left[1].value * right[4].value);
    result[1].value =
        left[2].value * right[9].value +
        (left[0].value * right[1].value +
         left[1].value * right[5].value);
    result[2].value =
        left[2].value * right[10].value +
        (left[0].value * right[2].value +
         left[1].value * right[6].value);

    result[4].value =
        left[6].value * right[8].value +
        (left[4].value * right[0].value +
         left[5].value * right[4].value);
    result[5].value =
        left[6].value * right[9].value +
        (left[4].value * right[1].value +
         left[5].value * right[5].value);
    result[6].value =
        left[6].value * right[10].value +
        (left[4].value * right[2].value +
         left[5].value * right[6].value);

    result[8].value =
        left[10].value * right[8].value +
        (left[8].value * right[0].value +
         left[9].value * right[4].value);
    result[9].value =
        left[10].value * right[9].value +
        (left[8].value * right[1].value +
         left[9].value * right[5].value);
    result[10].value =
        left[10].value * right[10].value +
        (left[8].value * right[2].value +
         left[9].value * right[6].value);

    result[3].flags = left[3].flags & right[3].flags;
    result[12].value =
        left[14].value * right[8].value +
        (left[12].value * right[0].value +
         left[13].value * right[4].value);
    result[13].value =
        left[14].value * right[9].value +
        (left[12].value * right[1].value +
         left[13].value * right[5].value);
    result[14].value =
        left[14].value * right[10].value +
        (left[12].value * right[2].value +
         left[13].value * right[6].value);
    result[13].value += 0.01f;
    result[3].flags = 3;
}

/*
 * Soft ceiling: 82.67857% - the typed body is exact; nonvolatile GPR coloring
 * and split saves differ from retail.
 */
/* Matching: compact loop/save lowering reproduces typed destruction, the
 * 15-entry clear, callback reset, and signed initialized latch exactly. */
#pragma optimize_for_size on
void npc_shadow_teardown(void) {
    int index;

    for (index = 0; index < 15; index++) {
        MkObj* object = npc_shadows.objects[index];

        if (object != 0 && object->hdr.instance != 0) {
            object->hdr.typed_vtbl->destroy(&object->hdr);
        }
        npc_shadows.objects[index] = 0;
    }
    npc_shadows.render = 0;
    npc_shadows.initialized = 0;
}
#pragma optimize_for_size reset

/* Near match: 93.333336% at exact retail size. Retail function order,
 * signed initialization guard, model-slot reloads, typed SObj flags/callback,
 * bone/material setup, foreground insertion, and packed color byte order
 * match; residue is loop-address and nonvolatile-register allocation. */
#pragma optimize_for_size on
void npc_shadow_init(void) {
    int index;

    if (npc_shadows.initialized != 0) {
        return;
    }

    load_ssf(konquest_common_file_table);
    load_art_section_language(0x60030, &sec_konquest_common_art);
    for (index = 0; index < 15; index++) {
        MkSobj* sobj;

        npc_shadows.objects[index] = (MkObj*)load_named_model_from_slot(
            0x60030, "NPC_SHADOW", 0x7F00, 0);
        if (npc_shadows.objects[index] == 0) {
            return;
        }

        obj_create_sobjs(npc_shadows.objects[index]);
        npc_shadows.objects[index]->hide_flag_bits.hidden = 1;
        sobj = (MkSobj*)obj_first_sobj(npc_shadows.objects[index]);
        sobj->flags_08_bits.bit0 = 0;
        sobj->flags09_bits.bit4 = 1;
        npc_shadows.objects[index]->light_flags = 0;
        build_bones_tbl(
            npc_shadows.objects[index], konquest_npc_bones, 0);
        npc_shadows.scales[index] = 1.0f;
        npc_shadows.materials[index] =
            obj_find_material_by_id(npc_shadows.objects[index], 0);
        sobj_set_priority(sobj, 0xD);
        sobj_use_material_color(sobj);
        if (npc_shadows.render == 0) {
            npc_shadows.render =
                (RpAtomic* (*)(RpAtomic*, KonquestNpcShadows*))
                    sobj->atomic->renderCallBack;
        }
        sobj->atomic->renderCallBack = shadow_render_callback;
        if (sobj->atomic->renderCallBack == 0) {
            sobj->atomic->renderCallBack = AtomicDefaultRenderCallBack;
        }
        sobj->flags09_bits.has_pebbles = 1;
        sobj->flags09_bits.bit7 = 1;
        insert_fgnd_mkobj(npc_shadows.objects[index]);
    }

    for (index = 0; index < 15; index++) {
        RpMaterial* material =
            obj_find_material_by_id(npc_shadows.objects[index], 0);
        RpMaterialColor color = {0xFFFFFFFF};

        color.red = 0x7F;
        color.blue = 0x7F;
        color.green = 0x7F;
        color.alpha = 0x7F;
        material->color = color;
    }
    npc_shadows.alpha = 0x7F;
    npc_shadows.initialized = 1;
}
#pragma optimize_for_size reset

void npc_assign_door_path(int door_id, int travel_mode) {
    void* path = get_door_path(door_id);

    if (path == 0) {
        npc_set_path(g_active_npc, 0, 0, 0, 0, travel_mode);
        return;
    }
    npc_set_path(
        g_active_npc, path, door_id, 4, 0x40000000, travel_mode);
}

/*
 * Soft ceiling: 97.65958% - lookup, table metadata recovery, and six-argument
 * path setup are exact; only latch branches and register coloring remain.
 */
void npc_assign_path_to_him(
    KonquestNpcData* data, void* path, int flags, int travel_mode) {
    if (data != 0) {
        KonquestNpc* npc = npc_find_by_data_inline(data);

        if (npc != 0) {
            int row_count = 0;
            int table_index = 0;

            if (path != 0) {
                row_count = get_row_count_for_table_by_pointer(
                    konquest_pdata->waypoint_script, path);
                table_index = get_table_index_by_pointer(
                    konquest_pdata->waypoint_script, path);
            }
            npc_set_path(
                npc, path, table_index, row_count, flags, travel_mode);
        }
    }
}

/* Matching: compact save/restore lowering reproduces the typed path lookup and
 * assignment body exactly. */
#pragma optimize_for_size on
void npc_assign_path(void* path, int flags, int travel_mode) {
    if (g_active_npc != 0) {
        int row_count = 0;
        int table_index = 0;

        if (path != 0) {
            row_count = get_row_count_for_table_by_pointer(
                konquest_pdata->waypoint_script, path);
            table_index = get_table_index_by_pointer(
                konquest_pdata->waypoint_script, path);
        }
        npc_set_path(
            g_active_npc, path, table_index, row_count, flags, travel_mode);
    }
}
#pragma optimize_for_size reset

static void npc_set_path(
    KonquestNpc* npc, void* path_pointer, int table_index, int row_count,
    int waypoint, int travel_mode) {
    KonquestWaypoint* waypoints = (KonquestWaypoint*)path_pointer;

    if (npc == 0) {
        return;
    }
    if (npc->path == 0) {
        KonquestPathData* path = (KonquestPathData*)get_mkhdr(
            &vtbl_path_data_struct, sizeof(*path));

        if (path != 0) {
            zero_pdata_payload(sizeof(*path), &path->hdr);
            path->travel_mode = 0;
            path->speed = 1.0f;
            path->table_index = -2;
        }
        npc->path = path;
    }

    if (waypoints != 0 && npc->path->waypoints == waypoints) {
        if (npc->path->current_waypoint != waypoint &&
            waypoint != 0x30000000) {
            if (waypoint == 0x40000000) {
                return;
            }
        } else {
            return;
        }
    }

    npc->path->waypoints = waypoints;
    npc->path->table_index = table_index;
    npc->path->waypoint_count = row_count;
    npc->path->step_direction = 1;
    npc->path->speed = 1.0f;

    if (waypoint >= row_count) {
        if (waypoint == 0x10000000 || waypoint == 0x40000000) {
            waypoint = 0;
        } else if (waypoint == 0x30000000) {
            float nearest_distance = 250000.0f;
            int nearest = 0;
            int index;

            if (npc != 0 && waypoints != 0) {
                int count = get_row_count_for_table_by_pointer(
                    konquest_pdata->waypoint_script, waypoints);

                for (index = 0; index < count; index++) {
                    float delta_z =
                        npc->data->position.z - waypoints[index].position.z;
                    float delta_x =
                        npc->data->position.x - waypoints[index].position.x;
                    float distance =
                        delta_x * delta_x + delta_z * delta_z;

                    if (distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = index;
                    }
                }
                npc->path->step_direction = 1;
            }
            waypoint = nearest;
        } else {
            waypoint = row_count - 1;
        }
    }
    if (waypoint < 0) {
        waypoint = 0;
    }

    if (travel_mode != 0) {
        npc->path->current_waypoint = -1;
        npc->path->target_waypoint = waypoint;
        npc->path->previous_waypoint = -1;
        return;
    }

    npc->path->current_waypoint = waypoint;
    npc->path->target_waypoint = waypoint;
    npc->path->previous_waypoint = waypoint;
    if (waypoints != 0) {
        KonquestWaypoint* current = &waypoints[waypoint];

        npc->data->position.x = current->position.x;
        npc->data->position.y = current->position.y;
        npc->data->position.z = current->position.z;
        npc->tile_index = get_tile_from_position(&current->position);
        if (npc_event_has_active_animation(npc) != 0) {
            npc->animation->object->flags_09_bits.bit6 = 0;
            npc->animation->object->pos.x = current->position.x;
            npc->animation->object->pos.y = current->position.y;
            npc->animation->object->pos.z = current->position.z;
        }
        if (current->script_function != 0) {
            KonquestWaypointScriptPdata* pdata = 0;
            MkProc* proc;

            proc = _create_mkproc_generic_tinystack(
                0xA017, 0x1F, p_npc_waypoint_script,
                sizeof(*pdata), (MkHdr**)&pdata);
            if (proc != 0) {
                set_process_as_scriptable(proc);
                pdata->function_index = current->script_function;
                pdata->npc = npc;
            }
        }
    } else {
        npc->data->position.x = npc->initial_position.x;
        npc->data->position.y = npc->initial_position.y;
        npc->data->position.z = npc->initial_position.z;
        npc->tile_index = get_tile_from_position(&npc->initial_position);
        if (npc_event_has_active_animation(npc) != 0) {
            npc->animation->object->flags_09_bits.bit6 = 0;
            npc->animation->object->pos.x = npc->initial_position.x;
            npc->animation->object->pos.y = npc->initial_position.y;
            npc->animation->object->pos.z = npc->initial_position.z;
        }
    }
}

/* Near match: 75.205574%, 12 bytes short of retail with retail-evidenced
 * compact loop lowering. Model-cache attachment,
 * texture/material setup, repeated object ownership loads, animation process
 * restoration, aligned collision construction, and both rollback paths match.
 * Residue is visible-array loop induction, pointer-boolean lowering, split
 * saves, and the pooled "0" string relocation. */
#pragma optimize_for_size on
static void load_model_for_npc(KonquestNpc* npc) {
    if (npc->animation != 0) {
        MkSobj* sobj;
        AnimPdata* animation;
        MkProc* animation_proc;
        MkHdr* object_header;

        npc_manager_find_model_for_npc(npc);
        if (npc->animation != 0) {
            sobj = (MkSobj*)obj_first_sobj(npc->animation->object);
            if (sobj != 0) {
                if (sobj->atomic->geometry != 0) {
                    RpGeometryForAllMaterials(
                        sobj->atomic->geometry,
                        (KonquestMaterialCallback)
                            material_restore_texture_pointer,
                        0);
                }
                if (strcmp(npc->data->texture_name, "0") != 0) {
                    RwTexture* texture = load_named_tga_from_slot(
                        0x6002B, npc->data->texture_name);

                    if (texture != 0) {
                        sobj_swap_material_texture(sobj, 5, texture);
                    }
                }
                if (sobj->atomic->geometry != 0) {
                    RpGeometryForAllMaterials(
                        sobj->atomic->geometry, hide_npc_materials,
                        npc->data->visible_material_ids);
                }
            }
        }
        if (npc->animation->object == 0) {
            npc_manager_remove_visible_entry(npc);
            return;
        }

        npc->animation->object->hide_flag_bits.pin_animation = 0;
        npc->animation->object->flags_0C = 0;
        npc->saved_object_flags = npc->animation->object->flags_word_08;
        npc->animation->object->ang.y = npc->data->angle_y;
        npc->animation->object->pos.x = npc->data->position.x;
        npc->animation->object->pos.y = npc->data->position.y;
        npc->animation->object->pos.z = npc->data->position.z;
        if (npc->animation->object != 0) {
            object_header = as_mkhdr(&npc->animation->object->hdr);
        } else {
            object_header = 0;
        }
        update_mkobj(object_header);
        if (konquest_editor_mode_on != 0) {
            animation_proc = create_mkproc_anim(
                0x5002, (MkProcEntryFn)p_animate, &animation);
        } else {
            animation_proc = create_mkproc_anim(
                0x5002, p_anim_idle, &animation);
        }
        if (animation_proc == 0) {
            npc_manager_remove_visible_entry(npc);
            return;
        }

        animation->obj = npc->animation->object;
        animation->obj_instance = npc->animation->object->hdr.instance;
        set_root_and_obj_movement_weights(
            0.0f, 1.0f, (AnimState*)animation);
        if (npc->queued_animation == 0) {
            set_anim_script(
                (KonquestAnimPdata*)animation,
                get_animation(npc->data->idle_animation), 0);
        } else {
            set_anim_script_frame(
                animation, npc->queued_animation,
                npc->animation_flags, npc->queued_animation_frame);
            if (npc->queued_animation_frame > animation->high_frame) {
                npc->queued_animation_frame = animation->high_frame;
            }
        }
        animation->step = 1.0f;
        npc->animation->proc = animation_proc;
        pose_anim((AnimState*)animation, 1);

        if (konquest_editor_mode_on == 0 &&
            npc_event_has_active_animation(npc) != 0) {
            CollisionShape shape __attribute__((aligned(16)));
            Vec center;

            center.x = npc->animation->object->pos.x;
            center.y = npc->animation->object->pos.y;
            center.z = npc->animation->object->pos.z;
            center.y += -1.0f;
            build_col_shape_vertical_cylinder(&shape, &center, 0.3f, 3.0f);
            npc->animation->editor_object =
                add_shape_to_global_collision_list(
                    &shape, npc->data_table_index + 0x10001);
        }
        if ((npc->flags_1D & 0x20) != 0) {
            sobj = (MkSobj*)obj_first_sobj(npc->animation->object);
            if (sobj != 0) {
                sobj->flags09_bits.bit4 = 1;
            }
        }
        insert_fgnd_mkobj(npc->animation->object);
        npc->wait_for_animation = 1;
    }
}
#pragma optimize_for_size reset



void npc_make_invisible(KonquestNpc* npc) {
    int index;

    if (npc_is_visible_model_active(npc) == 0) {
        return;
    }

    for (index = 0; index < 15; index++) {
        if (npc_manager_pdata->visible_npcs[index] == npc) {
            npc_manager_pdata->visible_npcs[index] = 0;
            npc_manager_pdata->visible_count--;
            if (npc->data->events[7].script_function == 1) {
                npc_manager_pdata->special_count--;
            }
            npc->flags_1C &= (unsigned char)~0x10;
            break;
        }
    }

    npc->timed_event_flags &= (unsigned char)~0x40;
    if ((npc->flags_1D & 0x40) == 0) {
        MkObj* object = npc->animation->object;

        if (object != 0) {
            npc->data->position.x = object->pos.x;
            npc->data->position.y = object->pos.y;
            npc->data->position.z = object->pos.z;
            npc->tile_index = get_tile_from_position(&object->pos);
            if (npc_event_has_active_animation(npc) != 0) {
                object->flags_09_bits.bit6 = 0;
                npc->animation->object->pos.x = object->pos.x;
                npc->animation->object->pos.y = object->pos.y;
                npc->animation->object->pos.z = object->pos.z;
            }
            npc->data->angle_y = npc->animation->object->ang.y;
        }
        if (npc->animation->proc != 0) {
            KonquestAnimPdata* animation =
                (KonquestAnimPdata*)pdata_of_proc(npc->animation->proc);

            npc->queued_animation_frame = animation->frame;
            npc->queued_animation = animation->animation;
            npc->animation_flags = animation->flags;
        }
        {
            CmdScript* script = get_cmdscript_for_proc(npc->proc);

            if (script->pc != 0) {
                CmdScript* saved_script = active_cmdscript;

                active_cmdscript = script;
                cmdscript_step_backward();
                memcpy(
                    npc->saved_script_state, active_cmdscript->stack_mem,
                    sizeof(npc->saved_script_state));
                npc->saved_script_position =
                    (unsigned int)active_cmdscript->pc;
                npc->saved_script_stack_depth = get_script_stack_depth();
                active_cmdscript = saved_script;
                npc->wait_ticks = npc->proc->sleep_ticks;
            }
        }
    }

    npc_manager_release_npc_model(npc);
    if (konquest_editor_mode_on == 0 &&
        npc->animation->editor_object != 0) {
        if ((npc->animation->editor_object != 0
                 ? as_mkhdr(&npc->animation->editor_object->hdr)
                 : 0)->instance != 0) {
            (npc->animation->editor_object != 0
                 ? as_mkhdr(&npc->animation->editor_object->hdr)
                 : 0)->typed_vtbl->destroy(
                npc->animation->editor_object != 0
                    ? as_mkhdr(&npc->animation->editor_object->hdr)
                    : 0);
        }
        npc->animation->editor_object = 0;
    }
    if (npc->animation != 0) {
        if (npc->animation->proc != 0 &&
            npc->animation->proc->instance != 0) {
            npc->animation->proc->hdr.typed_vtbl->destroy(
                &npc->animation->proc->hdr);
        }
        if (npc->proc->instance != 0) {
            npc->proc->hdr.typed_vtbl->destroy(&npc->proc->hdr);
        }
        npc->animation = 0;
        npc->proc = 0;
    }
}

static void npc_manager_release_npc_model(KonquestNpc* npc) {
    int index;

    if (npc->data->events[7].script_function == 1) {
        for (index = 0; index < 2; index++) {
            NpcManagerModelSlot* slot =
                &npc_manager_pdata->special_models[index];

            if (slot->data == npc->data) {
                MkObj* object = npc->animation->object;
                AniTextureControl* texture = npc->animation->lip_texture;

                if (object != 0) {
                    if (object->hdr.instance != 0) {
                        object->hdr.typed_vtbl->destroy(&object->hdr);
                    }
                    npc->animation->object = 0;
                }
                if (texture != 0) {
                    if ((unsigned int)texture->instance !=
                        npc->animation->lip_texture_instance) {
                        texture = 0;
                    }
                } else {
                    texture = 0;
                }
                if (texture != 0) {
                    MkHdr* texture_hdr = (MkHdr*)npc->animation->lip_texture;

                    if (texture_hdr->instance != 0) {
                        texture_hdr->typed_vtbl->destroy(texture_hdr);
                    }
                    npc->animation->lip_texture = 0;
                    npc->animation->lip_texture_instance = 0;
                }
                slot->data = 0;
                slot->available = 1;
                slot->art_id = 0;
                slot->object = 0;
                slot->age = 0.0f;
                return;
            }
        }
        return;
    }

    for (index = 0; index < 22; index++) {
        NpcManagerModelSlot* slot = &npc_manager_pdata->models[index];

        if (slot->data == npc->data) {
            if (slot->object != 0) {
                obj_for_all_atomics_set_material_alpha(slot->object, 0);
                remove_fgnd_mkobj(slot->object);
            }
            slot->data = 0;
            slot->available = 1;
            return;
        }
    }
}

/* Near match: 90.49% at exact retail size. Retail uses the compact loop
 * lowering selected here for all four fixed-capacity manager scans. Model
 * cache keys use the +0 model name, and slot selection, eviction, ownership,
 * and process initialization match; residue is register/branch scheduling. */
#pragma optimize_for_size on
void npc_make_visible(KonquestNpc* npc) {
    NpcManagerModelSlot* slot = 0;
    KonquestNpcAnimState* animation = 0;
    int index;

    if ((npc->flags_1C & 0x10) != 0 ||
        is_it_safe_to_make_this_npc_visible(npc) == 0) {
        return;
    }

    for (index = 0; index < 15; index++) {
        if (npc_manager_pdata->visible_npcs[index] == 0) {
            npc_manager_pdata->visible_npcs[index] = npc;
            npc_manager_pdata->visible_count++;
            if (npc->data->events[7].script_function == 1) {
                npc_manager_pdata->special_count++;
                if (npc_manager_pdata->special_count > 2) {
                    npc_manager_pdata->visible_count--;
                    npc_manager_pdata->special_count--;
                    return;
                }
            }
            npc->flags_1C |= 0x10;
            break;
        }
    }

    if (npc->data->events[7].script_function == 1) {
        for (index = 0; index < 2; index++) {
            NpcManagerModelSlot* candidate =
                &npc_manager_pdata->special_models[index];

            if (candidate->available != 0 &&
                strcmp(candidate->name, npc->data->model_name) == 0) {
                slot = candidate;
                break;
            }
        }
        if (slot == 0) {
            for (index = 0; index < 2; index++) {
                NpcManagerModelSlot* candidate =
                    &npc_manager_pdata->special_models[index];

                if (candidate->available != 0) {
                    slot = candidate;
                    break;
                }
            }
        }
    } else {
        unsigned int art_id = get_artid_of_named_item_in_slot(
            0x6002B, npc->data->model_name, 1);

        for (index = 0; index < 22; index++) {
            NpcManagerModelSlot* candidate =
                &npc_manager_pdata->models[index];

            if (candidate->art_id == art_id &&
                candidate->available != 0) {
                slot = candidate;
                break;
            }
        }
        if (slot == 0) {
            float oldest_age = -1.0f;

            for (index = 0; index < 22; index++) {
                NpcManagerModelSlot* candidate =
                    &npc_manager_pdata->models[index];

                if (candidate->age > oldest_age &&
                    candidate->available != 0) {
                    oldest_age = candidate->age;
                    slot = candidate;
                }
            }
            if (slot != 0 && slot->object != 0) {
                if (slot->object->hdr.instance != 0) {
                    slot->object->hdr.typed_vtbl->destroy(
                        &slot->object->hdr);
                }
                slot->object = 0;
                slot->art_id = 0;
            }
        }
    }

    if (slot != 0) {
        slot->available = 0;
        slot->data = npc->data;
        if (npc->animation == 0) {
            MkProc* proc = _create_mkproc_generic_bigstack(
                0xA002, 8, p_npc_proc, sizeof(*animation),
                (MkHdr**)&animation);

            if (proc != 0) {
                set_process_as_scriptable(proc);
                proc->pre_destroy = (MkProcCallbackFn)npc_pre_wake;
                proc->destroy_cb = (MkProcCallbackFn)npc_post_sleep;
                npc->animation = animation;
                npc->proc = proc;
                animation->lip_texture = 0;
                animation->lip_texture_instance = 0;
                animation->owner_npc = npc;
                animation->proc = 0;
                animation->object = 0;
                animation->editor_object = 0;
                animation->dialog_anim = 0;
                animation->alpha = 0;
            }
        }
    }
}
#pragma optimize_for_size reset

static int is_it_safe_to_make_this_npc_visible(KonquestNpc* npc) {
    KonquestNpc* farthest;
    int index;

    if (get_konquest_game_mode() == 3 &&
        (npc->flags_1D & 0x20) == 0) {
        return 0;
    }
    if (npc->data->events[7].script_function == 1 &&
        npc_manager_pdata->special_count >= 2) {
        farthest = 0;
        for (index = 0; index < 15; index++) {
            KonquestNpc* visible =
                npc_manager_pdata->visible_npcs[index];

            if (visible != 0 &&
                visible->data->events[7].script_function == 1 &&
                npc_is_visible_model_active(visible) != 0) {
                if (farthest == 0 ||
                    visible->camera_distance_squared >
                        farthest->camera_distance_squared) {
                    farthest = visible;
                }
            }
        }
        if (farthest != 0) {
            if ((npc->flags_1C & 0x40) != 0 ||
                farthest->camera_distance_squared >
                    npc->camera_distance_squared) {
                npc_make_invisible(farthest);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
    if (npc_manager_pdata->visible_count >= 15) {
        farthest = 0;
        for (index = 0; index < 15; index++) {
            KonquestNpc* visible =
                npc_manager_pdata->visible_npcs[index];

            if (visible != 0 &&
                npc_is_visible_model_active(visible) != 0) {
                if (farthest == 0 ||
                    visible->camera_distance_squared >
                        farthest->camera_distance_squared) {
                    farthest = visible;
                }
            }
        }
        if (farthest != 0) {
            if ((npc->flags_1C & 0x40) != 0 ||
                farthest->camera_distance_squared >
                    npc->camera_distance_squared) {
                npc_make_invisible(farthest);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
    return 1;
}

/* Near match: 82.12931%, 28 bytes over retail. Model loading, section
 * ownership, object setup, mouth WIFF discovery, and cache writes match.
 * Residue is GPR coloring, loop lowering, and save scheduling. */
static void npc_manager_load_new_npc_model(KonquestNpc* npc) {
    AniTextureControl* lip_texture = 0;
    NpcManagerModelSlot* slot = 0;
    int slot_handle = 0;
    unsigned int art_id = 0;
    NpcManagerPdata* manager = npc_manager_pdata;
    int is_special = npc->data->events[7].script_function;
    int index;
    MkObj* object;

    if (is_special == 1) {
        for (index = 0; index < 2; index++) {
            if (manager->special_models[index].available == 0 &&
                manager->special_models[index].data == npc->data) {
                slot_handle = 0x6002C + index;
                slot = &manager->special_models[index];
                break;
            }
        }
    } else {
        for (index = 0; index < 22; index++) {
            if (manager->models[index].available == 0 &&
                manager->models[index].data == npc->data) {
                slot = &manager->models[index];
                break;
            }
        }
    }
    if (slot == 0) {
        return;
    }

    {
        char* model_name = npc->data->model_name;

        if (is_special == 1) {
            char section_name[0x40];

            sprintf(section_name, "%s.sec", model_name);
            strlwr(section_name);
            load_ssf(kon_unique_npcs_file_table);
            load_art_section_by_name(slot_handle, section_name);
            strncpy(slot->name, model_name, 0x3F);
            slot->name[0x3F] = 0;
            object = (MkObj*)load_named_model_from_slot(
                slot_handle, "NPC", 0xA002, 0);
        } else {
            art_id = get_artid_of_named_item_in_slot(
                0x6002B, model_name, 1);
            object = load_model_from_slot(0x6002B, art_id, 0xA002);
        }
        if (object == 0) {
            return;
        }
    }

    npc_manager_setup_model(object);
    insert_ground_me_mkobj(object);
    if (konquest_editor_mode_on == 0) {
        if (npc->art_id == 0) {
            unsigned int mouth_art_id =
                npc_manager_find_mouth_art_id(object);

            if (mouth_art_id != 0) {
                npc->art_id = mouth_art_id;
            } else {
                npc->art_id = (unsigned int)-1;
            }
        }
        if (npc->art_id != 0 && npc->art_id != (unsigned int)-1) {
            lip_texture = append_wiff_to_clump_material_id(
                0x6002B, (char*)npc->art_id, object->clump, 1);
        }
    }

    slot->art_id = art_id;
    slot->object = object;
    if (lip_texture != 0) {
        slot->lip_texture = lip_texture;
        slot->lip_texture_instance = lip_texture->instance;
    }
    slot->age = 0.0f;
    slot->available = 0;
    slot->data = npc->data;
    if (npc->animation != 0) {
        npc->animation->object = object;
        if (lip_texture != 0) {
            npc->animation->lip_texture = lip_texture;
            npc->animation->lip_texture_instance = lip_texture->instance;
        }
    }
}

/* Near match: 83.02809%, four bytes over retail. Cache selection,
 * texture-instance validation, recreation, and object setup match; residue is
 * branch polarity, loop lowering, and GPR coloring. */
static void npc_manager_find_model_for_npc(KonquestNpc* npc) {
    NpcManagerPdata* manager = npc_manager_pdata;

    if (npc->data->events[7].script_function != 1) {
        unsigned int art_id = get_artid_of_named_item_in_slot(
            0x6002B, npc->data->model_name, 1);
        int found = 0;
        int index;
        NpcManagerModelSlot* slot = manager->models;

        for (index = 0; index < 22; index++, slot++) {
            if (slot->art_id == art_id && slot->available == 0 &&
                slot->data == npc->data) {
                found = 1;
            }
            if (found != 0) {
                AniTextureControl* lip_texture;

                slot->age = 0.0f;
                if (npc->animation != 0) {
                    npc->animation->object = slot->object;
                    lip_texture = slot->lip_texture;
                    if (lip_texture != 0) {
                        if ((unsigned int)lip_texture->instance !=
                            slot->lip_texture_instance) {
                            lip_texture = 0;
                        }
                    } else {
                        lip_texture = 0;
                    }
                    if (lip_texture == 0) {
                        if (npc->art_id == 0) {
                            unsigned int mouth_art_id =
                                npc_manager_find_mouth_art_id(
                                    npc->animation->object);

                            if (mouth_art_id != 0) {
                                npc->art_id = mouth_art_id;
                            } else {
                                npc->art_id = (unsigned int)-1;
                            }
                        }
                        if (npc->art_id != 0 &&
                            npc->art_id != (unsigned int)-1) {
                            lip_texture = append_wiff_to_clump_material_id(
                                0x6002B, (char*)npc->art_id,
                                npc->animation->object->clump, 1);
                            if (lip_texture != 0) {
                                slot->lip_texture = lip_texture;
                                slot->lip_texture_instance =
                                    lip_texture->instance;
                            }
                        }
                    }
                    npc->animation->lip_texture = slot->lip_texture;
                    npc->animation->lip_texture_instance =
                        slot->lip_texture_instance;
                }
                npc_manager_setup_model(slot->object);
                return;
            }
        }
    }
    npc_manager_load_new_npc_model(npc);
}

static RpAtomic* AtomicFindTextureWithRootString(
    RpAtomic* atomic, void* root_string) {
    if (atomic->geometry != 0) {
        RpGeometryForAllMaterials(
            atomic->geometry, MaterialFindTextureWithRootString, root_string);
    }
    return atomic;
}

static RpMaterial* MaterialFindTextureWithRootString(
    RpMaterial* material, void* data) {
    KonquestTextureSearch* search = (KonquestTextureSearch*)data;

    if (material->texture != 0 && search->root != 0 &&
        strnicmp(
            material->texture->name, search->root,
            search->root_length) == 0) {
        strncpy(
            search->texture_name, material->texture->name,
            sizeof(search->texture_name));
        return 0;
    }
    return material;
}

static void material_restore_texture_pointer(RpMaterial* material) {
    SpecularMaterialPluginData* spec =
        mk_get_specular_material_plugin(material);

    if (spec != 0) {
        material->texture = spec->saved_texture;
    }
}

static void material_store_texture_pointer(RpMaterial* material) {
    SpecularMaterialPluginData* spec =
        mk_get_specular_material_plugin(material);

    if (spec != 0) {
        spec->saved_texture = material->texture;
    }
}

static RpMaterial* hide_npc_materials(
    RpMaterial* material, void* data) {
    int* material_ids = (int*)data;
    unsigned int material_id =
        MK_MATERIAL_PLUGIN(material)->flags & 0xFFF;

    if (material_id >= 20) {
        if (material_ids != 0) {
            while (*material_ids != -1) {
                if (*material_ids == (int)material_id) {
                    show_material(material);
                    return material;
                }
                material_ids++;
            }
        }
        hide_material(material);
    }
    return material;
}

/*
 * Soft ceiling: 86.07692% - the typed visibility and distance algorithm is
 * exact; FPR/stack scheduling, save form, and float relocations differ.
 */
static int npc_check_visibility_and_calc_dist(KonquestNpc* npc) {
    if (npc->reaction_active) {
        KonquestNpcData* data = npc->data;
        CameraObj* camera = camera_obj;
        float delta_x;
        float delta_y;
        float delta_z;

        delta_y = data->position.y - camera->pos_y;
        delta_x = data->position.x - camera->pos_x;
        delta_z = data->position.z - camera->pos_z;
        npc->camera_distance_squared =
            delta_z * delta_z +
            (delta_x * delta_x + delta_y * delta_y);
        return 1;
    }
    if (npc->skip_visibility) {
        return 0;
    }

    {
        KonquestTileOrigin* tile = get_nth_tile_struct(npc->tile_index);
        KonquestNpcData* data;
        RwSphere sphere;
        RwMatrix* matrix;
        float direction_x;
        float direction_y;
        float direction_z;
        float inverse_length = 0.0f;
        float length_squared;

        if (tile == 0 || tile->loaded == 0) {
            return 0;
        }

        data = npc->data;
        sphere.center.x = data->position.x;
        sphere.center.y = data->position.y;
        sphere.center.z = data->position.z;
        matrix = camera_obj->field_24;
        direction_y = matrix->at.y;
        direction_x = matrix->at.x;
        direction_z = matrix->at.z;
        length_squared =
            direction_z * direction_z +
            (direction_x * direction_x + direction_y * direction_y);
        if (length_squared > 0.0f) {
            KonquestFloatBits estimate;
            float product;
            float correction;

            estimate.value = length_squared;
            estimate.bits = 0x5F375A00 - (estimate.bits >> 1);
            product = estimate.value *
                      (length_squared * estimate.value);
            correction = 3.0f - product;
            inverse_length =
                0.0625f * estimate.value * correction *
                -((correction * (product * correction)) - 12.0f);
        }

        direction_x *= inverse_length;
        direction_y *= inverse_length;
        direction_z *= inverse_length;
        sphere.center.x += 3.0f * direction_x;
        sphere.center.y += 3.0f * direction_y;
        sphere.center.z += 3.0f * direction_z;
        sphere.radius = 1.0f;
        if (RwCameraFrustumTestSphere(Camera, &sphere) == 0) {
            return 0;
        }
    }

    {
        KonquestNpcData* data = npc->data;
        CameraObj* camera = camera_obj;
        float delta_x = data->position.x - camera->pos_x;
        float delta_y = data->position.y - camera->pos_y;
        float delta_z = data->position.z - camera->pos_z;
        float distance_squared =
            delta_z * delta_z +
            (delta_x * delta_x + delta_y * delta_y);

        npc->camera_distance_squared = distance_squared;
        if (distance_squared > 2500.0f) {
            return 0;
        }
        return 1;
    }
}

void initialize_npc_data(void) {
    g_active_npc = 0;
    if (konquest_pdata->load_config->art_section_name != 0) {
        load_art_section_by_name(
            0x6002B, konquest_pdata->load_config->art_section_name);
        unload_section_slot(0x6002E);
        add_anim_section_by_name_async(
            0x6002E,
            konquest_pdata->load_config->animation_section_name,
            npc_fast_anims, 0, 1);
        wait_for_slot_load(0x6002E);
        if (konquest_editor_mode_on == 0 &&
            konquest_pdata->load_config->string_bank_name != 0) {
            load_string_bank(
                0x20000, konquest_pdata->load_config->string_bank_name);
        }
    }
}
