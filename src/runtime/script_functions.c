/*
 * MKO command-script native wrappers.
 *
 * Retail wrappers read arguments from current_args and publish return values
 * through active_cmdscript. Keep these leaves in retail function order.
 */

#include "game/pfxscript.h"
#include "game/pz_fatality.h"
#include "game/projectile.h"
#include "game/konquest.h"
#include "game/ejb.h"
#include "game/constrain.h"
#include "game/jdn.h"
#include "game/jmt.h"
#include "game/plyr.h"
#include "math/gxVect.h"
#include "runtime/limb.h"
#include "runtime/cam.h"
#include "runtime/mk_vtbl.h"
extern unsigned char* current_args;
extern unsigned char* active_cmdscript;
extern unsigned char exit_table_340[];
extern unsigned char* plyr_obj;
extern unsigned char* plyr_anim_pdata;
extern unsigned int pz_fighter_state;

typedef struct FakeBoneMatcher FakeBoneMatcher;
typedef struct MkFlippedBoneMap MkFlippedBoneMap;
typedef struct MovesAttackInfo MovesAttackInfo;
typedef struct ScriptAnimPdataView {
    char pad00[0x44];
    float step;
    char pad48[0x1C];
    float weight;
} ScriptAnimPdataView;
typedef struct FenceSection FenceSection;
typedef struct LightDef LightDef;
typedef struct MkProc MkProc;
typedef struct MkSobj MkSobj;
typedef struct CameraAnimEvent CameraAnimEvent;
typedef struct PlyrInfo PlyrInfo;
typedef struct SObj SObj;
typedef struct BgndAppendTextureEntry BgndAppendTextureEntry;
typedef struct BgndSwapTextureEntry BgndSwapTextureEntry;

typedef struct ScriptFlagWordView {
    MkHdr hdr;
    unsigned int flags;
} ScriptFlagWordView;

typedef struct ScriptFlagArgs {
    unsigned int header;
    ScriptFlagWordView* object;
    int bit;
    int enabled;
} ScriptFlagArgs;

typedef struct GetLimbObjArgs {
    unsigned int header;
    LimbRuntime* runtime;
    unsigned int bone_index;
} GetLimbObjArgs;

typedef struct ScriptPointerResult {
    char pad00[0x2C];
    MkHdr* value;
} ScriptPointerResult;

typedef struct ScriptStringResult {
    char pad00[0x2C];
    char* value;
} ScriptStringResult;

typedef struct ScriptVecResult {
    char pad00[0x2C];
    Vec* value;
} ScriptVecResult;

typedef struct ScriptProcResult {
    char pad00[0x2C];
    MkProc* value;
} ScriptProcResult;

typedef struct ScriptSobjResult {
    char pad00[0x2C];
    SObj* value;
} ScriptSobjResult;

typedef struct ScriptMkObjResult {
    char pad00[0x2C];
    MkObj* value;
} ScriptMkObjResult;

typedef void (*ScriptEntryFn)(void);
typedef float (*ScriptProcEntryFn)(void);
extern ScriptProcEntryFn script_callable_function_table[];

typedef struct ExitFloatIntArgs {
    unsigned int header;
    unsigned int exit_index;
    float float_value;
    int int_value;
} ExitFloatIntArgs;

typedef struct TrialRequiredAttackArgs {
    unsigned int header;
    int attack;
    int count;
} TrialRequiredAttackArgs;

typedef struct BackgroundColorArgs {
    unsigned int header;
    int red;
    int green;
    int blue;
    int alpha;
} BackgroundColorArgs;

typedef struct ScriptSingleIntArgs {
    unsigned int header;
    int value;
} ScriptSingleIntArgs;

typedef struct ScriptSingleFloatArgs {
    unsigned int header;
    float value;
} ScriptSingleFloatArgs;

typedef struct ScriptPointerArgs {
    unsigned int header;
    void* value;
} ScriptPointerArgs;

typedef struct ScriptFloatPointerArgs {
    unsigned int header;
    float* value;
} ScriptFloatPointerArgs;

typedef struct ScriptSobjArgs {
    unsigned int header;
    SObj* value;
} ScriptSobjArgs;

typedef struct ScriptTwoPointerArgs {
    unsigned int header;
    void* first;
    void* second;
} ScriptTwoPointerArgs;

typedef struct ScriptTwoIntArgs {
    unsigned int header;
    int first;
    int second;
} ScriptTwoIntArgs;

typedef struct ScriptThreeIntArgs {
    unsigned int header;
    int first;
    int second;
    int third;
} ScriptThreeIntArgs;

typedef struct ScriptMorphArgs {
    unsigned int header;
    int object_id;
    int start_shape;
    int end_shape;
    int ticks;
} ScriptMorphArgs;

typedef struct ScriptObjectArgs {
    unsigned int header;
    MkHdr* object;
} ScriptObjectArgs;

typedef struct ScriptMkObjArgs {
    unsigned int header;
    MkObj* object;
} ScriptMkObjArgs;

typedef struct ScriptAirborneArgs {
    unsigned int header;
    MkHdr* object;
    PlyrPdata* player;
} ScriptAirborneArgs;

typedef struct ScriptFenceArgs {
    unsigned int header;
    const Vec* point;
    const FenceSection* sections;
    int start_index;
    int mirrored;
} ScriptFenceArgs;

typedef struct ScriptFishAttackArgs {
    unsigned int header;
    MkHdr* player;
    int direction;
    int flags;
} ScriptFishAttackArgs;

typedef struct ScriptAxisArgs {
    unsigned int header;
    MkHdr* player;
    int axis;
} ScriptAxisArgs;

typedef struct ScriptLightningArgs {
    unsigned int header;
    PlyrInfo* owner;
    const Vec* position;
} ScriptLightningArgs;

typedef struct ScriptSobjAlphaArgs {
    unsigned int header;
    MkObj* object;
    int sobj_index;
    int alpha;
} ScriptSobjAlphaArgs;

typedef struct ScriptObjectIntArgs {
    unsigned int header;
    MkObj* object;
    int value;
} ScriptObjectIntArgs;

typedef struct ScriptVolumeArgs {
    unsigned int header;
    const Vec* position;
    float far_distance;
    float near_distance;
} ScriptVolumeArgs;

typedef struct ScriptBodyExplodeArgs {
    unsigned int header;
    MkHdr* player;
    const Vec* position;
    float velocity;
} ScriptBodyExplodeArgs;

typedef struct ScriptFishFlagArgs {
    unsigned int header;
    void* entries;
    int hidden;
    int count;
} ScriptFishFlagArgs;

typedef struct ScriptLightListArgs {
    unsigned int header;
    MkObj* object;
    LightDef* light;
} ScriptLightListArgs;

typedef struct ScriptPebbleVecArgs {
    unsigned int header;
    int player;
    int index;
    const Vec* value;
} ScriptPebbleVecArgs;

typedef struct ScriptPebbleBounceArgs {
    unsigned int header;
    int player;
    int index;
    const Vec* velocity;
    int flags;
} ScriptPebbleBounceArgs;

typedef struct ScriptMkObjFloatArgs {
    unsigned int header;
    MkObj* object;
    float value;
} ScriptMkObjFloatArgs;

typedef struct ScriptMkObjVecArgs {
    unsigned int header;
    MkObj* object;
    float x;
    float y;
    float z;
} ScriptMkObjVecArgs;

typedef struct ScriptSkytempleExplodeArgs {
    unsigned int header;
    int player;
    float x;
    float y;
    float z;
} ScriptSkytempleExplodeArgs;

typedef struct ScriptPebbleArrangeArgs {
    unsigned int header;
    int player;
    int count;
    const Vec* position;
} ScriptPebbleArrangeArgs;

typedef struct ScriptPebbleVelocityArgs {
    unsigned int header;
    int player;
    int count;
    float x;
    float y;
    float z;
} ScriptPebbleVelocityArgs;

typedef struct ScriptNamedSobjFxArgs {
    unsigned int header;
    unsigned int string_id;
    int sobj_id;
    float y_offset;
} ScriptNamedSobjFxArgs;

typedef struct ScriptSwapTextureTableArgs {
    unsigned int header;
    const BgndSwapTextureEntry* entries;
    int frame;
} ScriptSwapTextureTableArgs;

typedef struct ScriptSwapTextureArgs {
    unsigned int header;
    int sobj_id;
    int material_id;
    int frame;
} ScriptSwapTextureArgs;

typedef struct ScriptAppendTextureTableArgs {
    unsigned int header;
    const BgndAppendTextureEntry* entries;
} ScriptAppendTextureTableArgs;

typedef struct ScriptAppendTextureArgs {
    unsigned int header;
    int sobj_id;
    int material_id;
    unsigned int string_id;
    int texture_slot;
} ScriptAppendTextureArgs;

typedef struct ScriptLightColorArgs {
    unsigned int header;
    int light_id;
    float red;
    float green;
    float blue;
} ScriptLightColorArgs;

typedef struct ScriptIntFloatArgs {
    unsigned int header;
    int integer;
    float floating;
} ScriptIntFloatArgs;

typedef struct ScriptVecValueArgs {
    unsigned int header;
    float x;
    float y;
    float z;
} ScriptVecValueArgs;

typedef struct ScriptVecPointerArgs {
    unsigned int header;
    const Vec* vector;
} ScriptVecPointerArgs;

typedef struct ScriptEntryArgs {
    unsigned int header;
    ScriptEntryFn entry;
} ScriptEntryArgs;

typedef struct ScriptReactionArgs {
    unsigned int header;
    int reaction;
    float rate;
    int strength;
} ScriptReactionArgs;

typedef struct ScriptStartProjectileArgs {
    unsigned int header;
    int bone_id;
    MkObj* existing_object;
    unsigned int string_id;
    float speed;
    float tolerance;
    Vec* bone_offset;
} ScriptStartProjectileArgs;

typedef struct ScriptPlyrPdataArgs {
    unsigned int header;
    PlyrPdata* player;
} ScriptPlyrPdataArgs;

typedef struct ScriptPlyrEntryArgs {
    unsigned int header;
    PlyrPdata* player;
    ScriptEntryFn entry;
} ScriptPlyrEntryArgs;

typedef struct ScriptEntryPlayerArgs {
    unsigned int header;
    ScriptEntryFn entry;
    int player;
} ScriptEntryPlayerArgs;

typedef struct ScriptResumeEffectArgs {
    unsigned int header;
    int player;
    int bone_id;
    int effect_handle;
    int bind_mode;
    int blood_required;
} ScriptResumeEffectArgs;

typedef struct ScriptLoadPlayerModelArgs {
    unsigned int header;
    unsigned int string_id;
    int player;
    int object_type;
    int flags;
} ScriptLoadPlayerModelArgs;

typedef struct ScriptLoadSlotModelArgs {
    unsigned int header;
    int slot;
    unsigned int string_id;
    int flags;
    int user_data;
} ScriptLoadSlotModelArgs;

typedef struct PzConstrainArgs {
    unsigned int header;
    int mode;
    float value;
} PzConstrainArgs;

typedef struct ScriptAttackArgs {
    unsigned int header;
    int animation_id;
    union {
        PuzzleAttackParameters* puzzle;
        MovesAttackInfo* standard;
    } parameters;
    int arg3;
} ScriptAttackArgs;

typedef struct ScriptAnimationArgs {
    unsigned int header;
    int animation_id;
    int flags;
    float frame;
    float blend;
} ScriptAnimationArgs;

typedef struct ScriptThreeVecArgs {
    unsigned int header;
    Vec* out;
    const Vec* first;
    const Vec* second;
} ScriptThreeVecArgs;

typedef struct ScriptCircleArgs {
    unsigned int header;
    float* center;
    float radius;
    float angle;
    float* out;
} ScriptCircleArgs;

typedef struct ScriptCameraRectangleArgs {
    unsigned int header;
    MkSobj* object;
    const Vec* center;
    float min_x;
    float min_z;
    float max_x;
    float max_z;
} ScriptCameraRectangleArgs;

typedef struct ScriptCameraCylinderArgs {
    unsigned int header;
    MkSobj* object;
    const Vec* center;
    float radius;
    float height;
} ScriptCameraCylinderArgs;

typedef struct FakeBoneMatcherResetArgs {
    unsigned int header;
    FakeBoneMatcher* matcher;
    const Vec* parent_offset;
    const Vec* child_offset;
    const Vec* rotation;
    int bone_index;
    MkHdr* object;
    float blend;
} FakeBoneMatcherResetArgs;

typedef struct FakeBoneMatcherArgs {
    unsigned int header;
    MkHdr* parent;
    MkHdr* child;
    int bone_index;
    const Vec* parent_offset;
    const Vec* child_offset;
    const Vec* rotation;
    int mode;
    float blend;
} FakeBoneMatcherArgs;

typedef struct PopHeadArgs {
    unsigned int header;
    PlyrInfo* player;
    float x_velocity;
    float y_velocity;
    float z_velocity;
    float angular_velocity;
} PopHeadArgs;

typedef struct ScriptEightIntArgs {
    unsigned int header;
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;
    int arg6;
    int arg7;
    int arg8;
} ScriptEightIntArgs;

typedef struct LimbBoneAttachArgs {
    unsigned int header;
    PlyrInfo* target_player;
    int owner_bone;
    const Vec* offset;
    const Vec* rotation;
    PlyrInfo* owner_player;
    int limb;
    int target_bone;
    int include_children;
} LimbBoneAttachArgs;

typedef struct Gore2PebbleArgs {
    unsigned int header;
    unsigned int object_id;
    int bone;
    MkObj* source;
    FighterMirror* decal_owner;
    const Vec* velocity;
    const Vec* rotation;
    const Vec* scale;
    const Vec* position_offset;
    float vertical_acceleration;
    int bounce_count;
    float bounce_scale;
} Gore2PebbleArgs;

typedef struct ScriptIntResult {
    char pad00[0x2C];
    int value;
} ScriptIntResult;

typedef struct ScriptFloatResult {
    char pad00[0x2C];
    float value;
} ScriptFloatResult;

typedef struct ScriptCommandView {
    char pad00[0x20];
    int state;
    int move_attributes;
    int branch_target;
    int result;
    void* animation;
    char pad34[0x178];
    ScriptEntryFn exit;
} ScriptCommandView;

typedef struct ScriptDistanceFuncDef {
    int name_offset;
    unsigned int code_offset;
    unsigned int attributes_id;
} ScriptDistanceFuncDef;

typedef struct ScriptDistanceSlot {
    char pad00[0x74];
    ScriptDistanceFuncDef* functions;
    int string_relocation;
    char pad7C[0x0C];
    unsigned int* bytecode;
} ScriptDistanceSlot;

typedef struct ScriptDistanceCommand {
    MkHdr hdr;
    ScriptDistanceSlot* slot;
    char* function_name;
    int argument_count;
    unsigned int* program_counter;
    void* previous_program_counter;
    unsigned int* argument_header;
    int state;
    void* attributes;
} ScriptDistanceCommand;

#define ACTIVE_DISTANCE_SCRIPT \
    ((ScriptDistanceCommand*)active_cmdscript)

typedef struct ScriptExitView {
    char pad00[0x20];
    int state;
    char pad24[0x188];
    ScriptProcEntryFn exit;
} ScriptExitView;

typedef struct ScriptExitArgs {
    unsigned int header;
    int exit_value;
    int exit_arg0;
    int input_unlock_tick;
    int blocking_tick;
    int exit_arg1;
    int exit_arg2;
} ScriptExitArgs;

#define CURRENT_EXIT_ARGS ((ScriptExitArgs*)current_args)
#define CURRENT_PLAYER_PDATA ((PlyrPdata*)plyr_pdata)
#define ACTIVE_SCRIPT_EXIT ((ScriptExitView*)active_cmdscript)

typedef struct ScriptObjectRef {
    MkHdr* object;
    unsigned int instance;
} ScriptObjectRef;

typedef struct ScriptGroundObjView {
    MkHdr header;
    unsigned char flags08;
    unsigned char flags09;
} ScriptGroundObjView;

typedef void (*ScriptDestroyFn)(MkVtable5* vtable);

typedef struct ScriptDestroyVtable {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    ScriptDestroyFn destroy;
} ScriptDestroyVtable;

typedef struct ScriptActiveState {
    char pad00[8];
    int state;
} ScriptActiveState;

typedef struct ScriptOpponentProcLatch {
    char pad00[0x5C];
    MkProc* proc;
    unsigned int proc_instance;
} ScriptOpponentProcLatch;

/* Partial external MkObj view: retail accesses its position at +0xA0 here. */
typedef struct ScriptNpcCameraObjectView {
    char pad00[0xA0];
    Vec pos;
} ScriptNpcCameraObjectView;

typedef struct ScriptNpcBody {
    char pad00[0x0C];
    ScriptNpcCameraObjectView* camera_object;
} ScriptNpcBody;

typedef struct ScriptNpcHandle {
    char pad00[0x14];
    ScriptNpcBody* body;
} ScriptNpcHandle;

typedef struct ScriptNpcCameraArgs {
    unsigned int header;
    int npc_id;
    union {
        float movement_x;
        float orbit_speed;
    };
    union {
        float movement_y;
        int orbit_direction;
    };
    float movement_z;
    float lookat_y;
    union {
        float travel_time;
        int position_mode;
    };
    float initial_speed;
    float final_speed;
    int rotation_direction;
    int movement_mode;
} ScriptNpcCameraArgs;

typedef struct ScriptPlaceSlaveArgs {
    unsigned int header;
    int slave;
    int owner;
    int placement_mode;
    float values[13];
} ScriptPlaceSlaveArgs;


typedef union ScriptRawArg {
    int i;
    unsigned int u;
    float f;
    void* pointer;
} ScriptRawArg;

typedef struct ScriptRawArgs {
    unsigned int header;
    ScriptRawArg slots[16];
} ScriptRawArgs;

typedef struct ScriptRawResult {
    char pad00[0x2C];
    ScriptRawArg value;
} ScriptRawResult;

typedef union ScriptArgsRef {
    ScriptRawArgs* raw;
    unsigned char* bytes;
    GetLimbObjArgs* get_limb_obj;
    ExitFloatIntArgs* exit_float_int;
    TrialRequiredAttackArgs* trial_attack;
    BackgroundColorArgs* background_color;
    ScriptSingleIntArgs* single_int;
    ScriptSingleFloatArgs* single_float;
    ScriptPointerArgs* pointer;
    ScriptFloatPointerArgs* float_pointer;
    ScriptSobjArgs* sobj;
    ScriptTwoPointerArgs* two_pointer;
    ScriptTwoIntArgs* two_int;
    ScriptThreeIntArgs* three_int;
    ScriptMorphArgs* morph;
    ScriptObjectArgs* object;
    ScriptMkObjArgs* mkobj;
    ScriptAirborneArgs* airborne;
    ScriptFenceArgs* fence;
    ScriptFishAttackArgs* fish_attack;
    ScriptAxisArgs* axis;
    ScriptLightningArgs* lightning;
    ScriptSobjAlphaArgs* sobj_alpha;
    ScriptObjectIntArgs* object_int;
    ScriptVolumeArgs* volume;
    ScriptBodyExplodeArgs* body_explode;
    ScriptFishFlagArgs* fish_flag;
    ScriptLightListArgs* light_list;
    ScriptPebbleVecArgs* pebble_vec;
    ScriptPebbleBounceArgs* pebble_bounce;
    ScriptMkObjFloatArgs* mkobj_float;
    ScriptMkObjVecArgs* mkobj_vec;
    ScriptSkytempleExplodeArgs* skytemple_explode;
    ScriptPebbleArrangeArgs* pebble_arrange;
    ScriptPebbleVelocityArgs* pebble_velocity;
    ScriptNamedSobjFxArgs* named_sobj_fx;
    ScriptSwapTextureTableArgs* swap_texture_table;
    ScriptSwapTextureArgs* swap_texture;
    ScriptAppendTextureTableArgs* append_texture_table;
    ScriptAppendTextureArgs* append_texture;
    ScriptLightColorArgs* light_color;
    ScriptIntFloatArgs* int_float;
    ScriptVecValueArgs* vec_value;
    ScriptVecPointerArgs* vec_pointer;
    ScriptEntryArgs* entry;
    ScriptReactionArgs* reaction;
    ScriptStartProjectileArgs* start_projectile;
    ScriptPlyrPdataArgs* plyr_pdata;
    ScriptPlyrEntryArgs* plyr_entry;
    ScriptEntryPlayerArgs* entry_player;
    ScriptResumeEffectArgs* resume_effect;
    ScriptLoadPlayerModelArgs* load_player_model;
    ScriptLoadSlotModelArgs* load_slot_model;
    PzConstrainArgs* pz_constrain;
    ScriptAttackArgs* attack;
    ScriptAnimationArgs* animation;
    ScriptThreeVecArgs* three_vec;
    ScriptCircleArgs* circle;
    ScriptCameraRectangleArgs* camera_rectangle;
    ScriptCameraCylinderArgs* camera_cylinder;
    FakeBoneMatcherResetArgs* fake_bone_reset;
    FakeBoneMatcherArgs* fake_bone_match;
    PopHeadArgs* pop_head;
    LimbBoneAttachArgs* limb_bone_attach;
    Gore2PebbleArgs* gore2_pebble;
    ScriptEightIntArgs* eight_int;
    ScriptFlagArgs* flag;
    ScriptExitArgs* exit_args;
} ScriptArgsRef;

typedef union ScriptResultRef {
    ScriptRawResult* raw;
    unsigned char* bytes;
    ScriptPointerResult* pointer;
    ScriptStringResult* string;
    ScriptVecResult* vector;
    ScriptProcResult* proc;
    ScriptSobjResult* sobj;
    ScriptMkObjResult* mkobj;
    ScriptIntResult* integer;
    ScriptFloatResult* floating;
    ScriptCommandView* command;
    ScriptExitView* exit;
} ScriptResultRef;

extern float _mkproc_sleep_ticks;
extern float inverse_game_speed;
void update_bone_hierarchy(void* object);
void ground_me(void* object);
void nis_init(int state, int arg0, int arg1);
ScriptNpcHandle* find_npc_by_data(int npc_id, void* args, float value);
void nb_place_slave_in_bgnd(int slave, int owner, const char* name,
                            int placement_mode, float value0, float value1,
                            float value2, float value3, float value4,
                            float value5, float value6, float value7,
                            float value8, float value9, float value10,
                            float value11, float value12);

void stop_usec_timer(int timer);
void start_usec_timer(int timer);
int printf(const char* format, ...);
void obj_setup_for_animation(
    MkObj* object, const int* tags, MkFlippedBoneMap* flipped_bone_map,
    void* ground_colls);
void xfer_proc(MkProc* proc, MkProcEntryFn entry);
void update_mkobj(void* object);
void* get_mkobj_frame(int id, int frame);
void setup_screen_for_fatality(void);
void trial_add_required_attack(unsigned char attack, unsigned char count,
                               int flags);
void set_background_color(unsigned char red, unsigned char green,
                          unsigned char blue, unsigned char alpha);
unsigned short randu0(unsigned short limit);
void* get_animation(int animation_id);
void pz_fighter_set_y_constrain(unsigned char* player_obj, int mode,
                                PzConstrainArgs* args, float value);
void pz_fighter_attack(
    void* animation, PuzzleAttackParameters* attack, int reaction);
void attack_opponent_with(
    void* animation, MovesAttackInfo* attack, int reaction);
void advance_my_moveset(void);
void j_call_player_script_function(void);
int was_i_hit_x_times(int hit_count);
int was_button_and_direction(int button, int direction);
void glitch_to_ani_frame(void* animation, int flags,
                         ScriptAnimationArgs* args, float frame);
void blend_to_ani_frame(void* animation, int flags,
                        ScriptAnimationArgs* args, float frame, float blend);
void glitch_to_ani(void* animation, int flags);
void reaction_xfer_him_nohit(void* entry);
void set_anim_hiframe(void* script_args, float frame);
int was_button_pressed(int button);
int am_i_airborn(void);
int is_his_chest_to_screen(void);
int is_my_chest_to_screen(void);
int am_i_on_the_left2(void* me, void* him);
int am_i_on_the_left(void);
int is_he_flipped(void);
int am_i_flipped(void);
int is_fast_getup(void);
int disable_impale_check(void);
void tag_team_activate_player(int player, int active);
void load_and_set_refl_on_weapon(void);
void advance_active_moveset(int amount);
int get_active_moveset_from_pdata(void* pdata);
void fx_transfer(int effect, int owner);
void bgnd_force_specularity_off_for_material(void* object, int material);
void bgnd_sobj_set_texture_kl_values(void* object, int material, int texture,
                                     float value, int flags);
char* get_script_string_arg(int argument);
void bgnd_replace_tex_with_wiff_and_ani(
    int object, const char* texture, float frame_rate, int first_frame,
    int last_frame);
void jab_attach_wiff_to_sobj(
    int object, int sobj, const char* wiff, const char* animation,
    int flags, int mode, float scale);
MkObj* start_projectile_from_sidekick_bone(
    int bone, MkObj* existing_object, const char* projectile,
    float speed, float tolerance, const Vec* bone_offset);
void bgnd_chunk_explosion_match_velocity_with_params(
    int arg4, int arg5, int arg6, int arg7, float x, float y, float z);
int ncs_bgnd_preload_named_model(
    const char* model, int slot, int flags, int heap, int light, int mode);
void attach_pfx_to_object_by_uid(
    int object_uid, const char* effect_name, int bone, int flags);
void bgnd_create_named_npc_in_slot(
    int slot, const char* name, int object_id, int flags);
void bgnd_launch_fx_at_active_sobj_pos_with_offset(
    const char* effect_name, float x, float y, float z);
void* bgnd_launch_fx_at_bid_of_mkobj(
    const char* effect_name, MkObj* object, int bone);
void* bgnd_launch_fx_at_position(
    const char* effect_name, float x, float y, float z);
void* bgnd_pfxhandle_spawn_at_bid(
    const char* effect_name, MkObj* object, int bone);
void general_flash_fx(
    int player, MkObj* object, const char* effect_name, int bone,
    int use_bone, float y_offset);
void mk_chess_launch_fx_at_active_piece_with_offset(
    const char* effect_name, float x, float y, float z);
void* pfxhandle_bgnd_spawn_at_position(
    const char* effect_name, float x, float y, float z);
void pui_play_pfx(int object_id, int effect_id, const char* effect_name);
void trial_setup_onscreen_display_items(
    int first_item, int item_count, const char* effect_name);
MkObj* load_cloth_boned_model(
    const char* name, int object_id, int slot, int flags, int cloth_flags,
    int collision_flags, int render_flags);
void konquest_map_setup_fight(
    int first, int second, int third, int fourth, int fifth, int sixth,
    int seventh, int eighth, const char* arena_name);
void debug_print_message(void);
void fxsys_set(int parameter, float value);
void fxsys_set_v3(int parameter, float x, float y, float z);
void fx_bind_render_to_sobj(int effect, void* object);
void fx_bind_emitter_to_obj_bone(int effect, void* object, int bone);
void fx_bind_render_to_obj_bone(int effect, void* object, int bone);
void fx_disable_ztest(int effect, int disable);
void fx_set_render_priority(int effect, int priority);
void fx_hide(int effect, int hide);
void fx_get_v3(int effect, int parameter, void* value);
void fx_set(int effect, int parameter, float value);
void fx_set_param_v3(int effect, int parameter, float x, float y, float z);
void enable_profiling(int enable);
void kill_on_y_less_than_field(int object, int field);
void change_on_y_less_than_field(int object, int field);
void change_on_y_less(int object, float value);
void change_on_less(int object, float value);
void change_on_greater(int object, float value);
void kill_roundrobin(int value);
void kill_percent(float value);
void kill_on_greater(int object, float value);
void udpate_roundrobin(int value);
void update_assign(int object, int value);
void update_attract(int object, int target, float value);
void update_bounce(int object, int target, int axis, float value);
void update_texanim_hold(int object, int texture, float value, int first, int last);
void update_texanim(int object, int texture, float value, int first, int last);
void update_lerp_color(int object, int color, float value, int first, int last,
                       int flags);
void update_fade_alpha2(int object, int alpha, float start, float end,
                        int first, int last);
void update_fade_alpha(int object, int alpha, float start, float end);
void update_mul_scalar(int object, float x, float y, float z);
void update_wrapbox(int object, float x, float y, float z, float w);
void update_add_constant(int object, float value);
void update_add_constant_v3(int object, float x, float y, float z);
void update_copy(int object);
void update_add(int object, int value);
void initial_multiply_float(int object, float x, float y);
void initial_set_float(int object, float x, float y);
void initial_add_v3(int object, int value);
void initial_divert(int object, float x, float y);
void initial_reflect(int object);
void set_cycle_emission(int value);
void set_cycle_length(float start, float end);
void fx_reset_emit(int effect);
void fx_pause_emit(int effect);
void fx_resume_emit(int effect);
int fx_next_emitter(int effect);
void fx_restart_emit(int effect);
void reset_effect(void);
void spawn_color(int a, int b, int c, int d, int e);
void spawn_random_size(int value);
void set_growth_coefficient(float value);
void set_drag_coefficient(float value);
void set_rotation(float start, float end);
void kill_at_plane(float value);
void emit_constant_rate(void);
void emit_roundrobin_mechanism(int a, int b);
void emit_value_i(int a, int b);
void emit_value(int a, float b);
void emit_from_pos_clamp_y(int a, int b, float c, float d, float e, float f,
                           float g, float h);
void emit_from_pos(int a, int b, float c, float d, float e, float f, float g);
void emit_spherical_section(int a, float b, float c, float d, float e,
                            float f, float g, float h);
void emit_spherical_from_boundary(int a, float b);
void emit_spherical(int a, float b);
void texture_animation_with_vsize(float a, float b, int c, float d);
void texture_animation(float a, int b, float c);
void emission_duration(float value);
void emit_cylindrical(int a, float b, float c, float d, float e, float f,
                      float g, float h);
void emit_cartesian(int a, float b, float c, float d, float e, float f,
                    float g);
void emit_disc2(int a, float b, float c, float d, float e, float f);
void emit_disc(int a, float b, float c, float d, float e);
void emit_cuboid(int a, float b, float c, float d);
void emit_from_point(float a, float b, float c);
void emit_uv(int a, float b, float c);
void emit_color(int a, int b, int c, int d, int e);
void emit_in_range(int a, float b, float c);
int fx_by_owner(char* name, int owner);
int fx2(int effect, char* name);
void set_vertex_color(int color);
void set_light(int light);
void set_aspect_ratio(float x, float y);
void set_bounding_radius(float radius);
void create_y_mirror_effect(int effect);
void z_bias(float value);
void particle_size(float value);
void face_y(void);
void set_decal_plane(int plane);
void create_multiemit_parametric_fx(int a, char* b, int c);
void create_parametric_fx(int a, char* b);
void create_multiemit_step_fx(int a, char* b, int c);
void create_step_fx(int a, char* b);
void bind_to_bone(int bone);
void create_step_effect(int effect);
void parametric_update(int value);
float dist_xz_to_xz(void* a, void* b);
void v3_to_xz_ang(void* out, void* value);
void v3_to_xy_ang(void* out, void* value);
float length_v3(void* value);
void rotate_xz(void* out, void* value, float angle);
float gxMathArcCos(float value);
float gxMathSin(float value);
float gxMathCos(float value);
float uv_v3_to_v3_dist(void* out, void* a, void* b);
float xz_dot_xz(void* a, void* b);
void YXZ_angles_to_quat(void* out, void* angles);
void* obj_get_bone_rot_quat(void* object, int bone);
void uv_from_angle_y(void* out, float angle);
float xz_to_y_ang(void* value);
void xz_unit_vector(void* out, void* a, void* b);
float v3_dot_v3(void* a, void* b);
void zero_v3(void* value);
void normalize_v3(void* value);
void scale_v3(void* out, void* value, float scale);
void v3_add_v3_scaled(void* out, void* a, void* b, float scale);
void v3_add_v3(void* out, void* a, void* b);
void v3_sub_v3(void* out, void* a, void* b);
void v3_x_mat(void* out, void* value, void* matrix);
void mkobj_get_matrix_pos(void* out, void* object);
void mkobj_get_matrix_right(void* out, void* object);
void mkobj_get_matrix_at(void* out, void* object);
void* mkobj_get_matrix(void* object);
int plyr_in_spin_react(void* pdata);
void* force_calc_bone_world_mat(void* object, int bone);
void obj_set_sobj_pos(void* object, int sobj, void* value);
void get_bone_relative_pos(void* object, int bone, void* out);
void get_bone_offset_world_pos(void* object, int bone, void* offset, void* out);
void get_bone_world_pos(void* object, int bone, void* out);
void bone_matcher_child_set_offset(void* matcher, void* offset);
void bone_matcher_parent_set_offset(void* matcher, void* offset);
void* start_bone_matcher(void* a, void* b, int c, int d, float e);
void obj_get_ang_vel(void* out, void* object);
void obj_set_ang_vel(MkObj* object, void* value);
void obj_set_pos_vel(MkObj* object, void* value);
void obj_get_pos_vel(void* out, void* object);
MkSobj* obj_find_sobj_by_id(MkObj* object, unsigned int id);
void obj_set_light_flag(void* object, int flag);
void sobj_get_ang(void* out, void* sobj);
void sobj_get_ang_vel(void* out, void* sobj);
void sobj_set_ang_vel(MkSobj* sobj, void* value);
void sobj_set_ang(MkSobj* sobj, void* value);
void sobj_get_pos_vel(void* out, void* sobj);
void sobj_set_pos_vel(MkSobj* sobj, void* value);
void sobj_get_pos(void* out, void* sobj);
void sobj_set_pos(MkSobj* sobj, void* value);
void obj_get_ang(void* out, void* object);
void obj_set_ang(void* object, void* value);
void obj_set_ground_colls_y(void* object, float value);
void obj_set_ground_colls(void* object, void* value);
void konquest_load_interior_art(void);
void konquest_start_nis_anims_load(char* first, char* second);
int konquest_nis_anims_loaded(void);
void get_current_time(void* value);
void konquest_nis_end(void);
void konquest_nis_init(int value);
void nis_wait_for_region_load(void);
void wait_for_region_load(void);
void nis_register_participant(int a, int b);
void nis_set_wait_override(int value);
void nis_show_cancel_message(void);
int nis_scene_done(void);
void nis_end(void);
void nis_wait_for_event(int a, int b);
void nis_signal_event(int value);
void* get_fatality_state_ptr(void);
void animpdata_ani_to_frame_x_with_flag_check(void* anim, float frame,
                                               int flag, int value);
void mkscripts_set_anim_check_flag(void* anim, int flag);
void mkscripts_mkobj_insert_mkobj_cleanuplist(void* object, void* list);
void mkscripts_destroy_gusher(void* value);
void mkscripts_destroy_fk_bonematcher(void* value);
void mkscripts_destroy_bonematcher(void* value);
int dkp_check_plyr_state_for_grab(int state);
void* start_prison_grab_proc(int a, int b, float c, float d);
void done_prison_grab_proc(int value);
void kill_spear(void);
void xfer_spearproc_to_retract(void* value);
void destroy_spearproc_bonematcher(void* value);
void insert_mkobj_spearproc_parentobjitem(void* a, void* b);
void* get_spearobj_from_spearproc(void* value);
void* fire_sc_spear(int a, int b, int c, int d, int e, int f);
void subzero_start_ice_chunks(int value);
void* subzero_start_iceman(void);
void* subzero_start_iceblock(void);
void sindel_scream_react_sound_start(void);
void sindel_sonic_sounds(int a, int b);
int sindel_sonic_waves(float value);
void start_raiden_lightning_scroll(int a, float b, float c, int d, int e);
void* ft_raiden_summon_lightning_bolt(int a, int b, char* name);
void kill_raiden_summon_lightning_bolt(void* value);
void fix_axe_angle(void* value);
void ft_mileena_start_veil_ripoff(void);
void fat_goro_fold_arms(int a, int b, float c, int d);
void* fatality_boraicho_get_jug(int a, int b);
void* fatality_boraicho_light_fart_torch(int a);
void* fatality_boraicho_get_torch(int a, int b);
void show_baraka_one_blade_only(int a, int b);
void* fatality_ashrah_get_doll(int a, int b, int c);
void fire_multi_emitter_pfx_via_tbl(
    const char* name, const void* table, MkObj* object, int* handles);
unsigned int pfxhandle_spawn_at_bid_next_bind_render(
    unsigned int effect, MkObj* object, int bone_id);
unsigned int pfxhandle_bgnd_spawn_at_sobj_id(
    const char* name, unsigned int sobj_id);
unsigned int pfxhandle_spawn_at_bid(
    const char* name, MkObj* object, int bone_id);
unsigned int pfxhandle_spawn_at_bid_next(
    unsigned int effect, MkObj* object, int bone_id);
void pfx_spawn_at_bid(char* name, int a, int b);
void* limb_sever_throw_away(int a, int b, int c);
void auto_calc_limbobj_bone_world_pos(void* a, void* b);
void limb_sever_show_z_meat_chunks(MkObj* obj, int limb, int show_all);
void limb_sever_show_z_meat_chunks_all(MkObj* obj);
void limb_sever_show_z_meat_chunks_all_plyr_num(int a);
void limb_sever_explode_apart_plyr_num(int a, float b, float c, float d, int e);
void reset_blood_decals(void);
void destroy_gore2_obj(void* a, void* b);
void* attach_gore2_obj(int a, int b, int c, int d, int e);
void start_bodyslam_bodysplat(float a, float b, float c, float d, float e);
void fatality_explode_victim(int a, float b, float c);
void kill_gusher(int a);
void start_sweat_particles_scripts(int a, int b);
void* start_blood_particles_scripts(int a, int b);
void start_sweat_particles(int a, int b, int c, int d);
void* start_blood_particles(int a, int b, int c, int d);
void mks_spawn_blood_pool_at_bid(int a, int b, int c, int d);
void spawn_blood_pool_at_bid(int a, int b, int c);
void spawn_bld_splat(char* name, int a, int b);
void* plyr_weapon2_release(int a);
void* plyr_weapon_release(int a);
void bone_matcher_reset_dest_mat_rot(int a, int b);
void bone_matcher_set_ang_pos(int a, int b, int c, int d, int e, int f);
void* weapon_bm_ignore(int a, int b);
void* regrab_weapon(int a, int b, int c, int d, int e, int f, int g);
void weapon_reflection_show_hide(int a, int b, int c);
void* show_single_weapon(int a, int b);
void* clone_my_weapon(int a, int b);
void clone_weapon_to_secondary(int a, int b);
void advance_to_weapon_style(int a);
int is_weapon_style(int a);
int am_i_female(int a);
float sobj_set_bounding_sphere_radius(void* sobj, float value);
float sobj_get_bounding_sphere_radius(void* sobj);
unsigned long play_his_snd_req(int a);
unsigned long play_his_random_voice(int a);
void obj_unhide_material_by_id(void* object, int id);
void obj_hide_material_by_id(void* object, int id);
void bm_force_fake_child_bid(int a, int b);
int fat_bgnd_char_setup_radius_check(const FatalityRadiusCheck* check);
void set_victim_v3_units_away(float a, float b);
void reset_fake_bone_matcher(FakeBoneMatcher* matcher,
                             const Vec* parent_offset,
                             const Vec* child_offset, const Vec* rotation,
                             int bone_index, MkHdr* object, float blend);
MkHdr* ft_fake_bone_matcher(MkHdr* parent, MkHdr* child, int bone_index,
                            const Vec* parent_offset, const Vec* child_offset,
                            const Vec* rotation, int mode, float blend);
int get_game_state(void);
void fatality_release_other_player(void);
int get_level_fatality_done_flag_state(void);
void set_level_fatality_done_flag_state(int value);
void limb_sever_destroy_existing_attach_proc(int a, int b);
void limb_sever_bone_attach(
    PlyrInfo* target_player, int owner_bone,
    const Vec* offset, const Vec* rotation,
    PlyrInfo* owner_player, int limb, int target_bone,
    int include_children);
MkHdr* limb_sever_pop_head_up(PlyrInfo* player, float x_velocity, float y_velocity,
                              float z_velocity, float angular_velocity);
void* mks_limb_sever(int a, int b, int c);
void* limb_sever_find_existing_update_proc(int a, int b, int c);
void limb_sever_update_slide_end_coeff(int a, float b);
void* proc_of_anim_pdata(void* anim);
void set_pdata_anim_step(void* anim, float step);
void plyr_turn_on_shadowbox(int a);
void plyr_turn_off_shadowbox(PlyrInfo* player);
void plyr_turn_on_mirrorguy(int a);
void plyr_turn_off_mirrorguy(PlyrInfo* player);
void animpdata_ani_to_blend_frame(void* anim, float frame);
void animpdata_ani_to_end_at1(void* anim);
void animpdata_ani_to_end(void* anim);
void animpdata_ani_x_more_frames(void* anim, float frames);
void animpdata_ani_loop_more_frames(void* anim, float frames);
void animpdata_ani_to_frame_x(void* anim, float frame);
void mks_animpdata_set_cur_frame(void* anim, float frame);
void animpdata_ani_1_frame(void* anim);
void check_to_register_miss(void);
void auto_ani_off(void);
void ncs_dkp_camera_konqchar_show_hide_alpha(int a, int b);
void ncs_camera_wall_show_hide_alpha(void* regions);
void* ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void);
void ncs_bgnd_nuke_collision_to_script_interface(void);
void* retrieve_bgnd_obj(void);
void fkbm_obj_face_obj(int a, int b, int c, int d, int e);
void start_obj_scalar_proc(int a, int b, int c, int d);
void* insert_particle_mkobj(int a);
float mkobj_pos_pos_dot_normal_xz(int a, int b, int c);
int obj_get_bid_for_tid(int a, int b);
MkSobj* obj_create_sobjs_by_id(MkObj* object, int id);
void* unhide_sobj_by_sobj_id(void* obj, unsigned int id);
void* hide_sobj_by_sobj_id(void* obj, unsigned int id);
void sobj_set_priority(int a, int b);
void unhide_sobj(int a);
void hide_sobj(int a);
void unhide_obj(int a);
void hide_obj(int a);
void* pfx_plyr_bankowner(int a);
void ncs_script_debug_quickie(int a, float b, int c);
float get_inverse_game_speed(void);
void ck_rumble_controller(int a, int b, int c);
int check_for_green_light(int a);
int check_for_red_light(int a);
void ck_put_weapon_away(int a);
void* find_obj_by_id(int a);
void yinyang_reset_music_index(void);
void yinyang_play_evil_tune(void);
void yinyang_play_good_tune(void);
float dist_v3_to_v3(void* a, void* b);
int is_point_in_fortress_exclusion_zone(void* point);
void fortress_setup_exclusion_zone(int a, float b, float c, float d, float e);
void set_evil_swap_status(int a);
int ok_to_do_evil_swap(void);
void set_evil_condition(int a);
int yy_is_evil_time_active(void);
void bgnd_make_object_transl(int a);
int are_death_traps_on(void);
char* get_string_by_id(unsigned int id);
void ending_show_text(int script_index, int duration);
void ending_show_image(int image);
void midpoint_v3(Vec* out, const Vec* first, const Vec* second);
Vec* sobj_get_world_pos(SObj* object);
void get_point_on_circle(float* center, float radius, float angle, float* out);
MkHdr* cut_player_in_half(MkHdr* player);
int is_plyr_airborn(MkHdr* object, PlyrPdata* player);
int get_offset_of_closest_fence_section(const Vec* point,
                                        const FenceSection* sections,
                                        int start_index, int mirrored);
MkProc* find_mkproc_pid(int pid);
void start_fish_attack(MkHdr* player, int direction, int flags);
int is_timer_off(void);
void debug_create_axis_indicator(MkHdr* player, int axis);
void bgnd_level_fatality_end(void);
void bgnd_level_fatality_start(int player);
void bgnd_level_transition_end(void);
void bgnd_level_transition_start(void);
void do_lightning_strike(PlyrInfo* owner, const Vec* position);
void obj_set_sobj_alpha(MkObj* object, int sobj_index, int alpha);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
float get_volume_from_distance(const Vec* position, float far_distance,
                               float near_distance);
float get_pan_value(const Vec* position);
void mab_test(void);
void player_body_explode(MkHdr* player, const Vec* position, float velocity);
void bgnd_clear_danger_zone_callback(int zone);
void yinyang_set_bad_fish_hide_flag(void* entries, int hidden, int count);
void yinyang_set_good_fish_hide_flag(void* entries, int hidden, int count);
void yinyang_make_fish_jump(int fish, int velocity);
void destroy_mkobjs_oid(int oid);
void do_yinyang_statue_explosion(MkHdr* statue);
void obj_add_to_skinned_obj_light_list_with_ambient(MkObj* object,
                                                    LightDef* light);
void obj_change_to_bgnd_obj_light_list(MkObj* object, LightDef* light);
void obj_change_to_skinned_obj_light_list(MkObj* object, LightDef* light);
void yinyang_stop_lensflare(void);
void yinyang_start_lensflare(void);
void misc_data_set_test_float(float value);
void misc_data_set_test_u32(unsigned int value);
void obj_set_sobj_priority(MkObj* object, int sobj_index, int priority);
void sobj_disable_blending(SObj* object);
SObj* obj_first_sobj(MkObj* object);
void pebble_turn_culling_off(int player);
void pebble_turn_culling_on(int player);
void pebble_unhide_me(int player, int index);
void pebble_hide_me(int player, int index);
void pebble_setup_bounce_props(int player, int index, const Vec* velocity,
                               int flags);
void pebble_set_ang_vel(int player, int index, const Vec* velocity);
void pebble_set_ang(int player, int index, const Vec* angles);
void pebble_set_scale(int player, int index, const Vec* scale);
void pebble_set_vel(int player, int index, const Vec* velocity);
void pebble_set_pos(int player, int index, const Vec* position);
void destroy_mkobj(void* object);
void obj_turn_gravity_off(void* object);
void obj_set_gravity(void* object, float gravity);
void insert_fgnd_mkobj(void* object);
int get_player_number(void* object);
MkObj* load_named_model_for_player(const char* name, int player, int object_type,
                                   int flags);
MkObj* load_named_model_from_slot(int slot, const char* name, int flags,
                                  int user_data);
float script_fabs(float value);
void set_obj_light_flags(MkObj* object, int flags);
void set_obj_ang(MkObj* object, float x, float y, float z);
void set_obj_pos(MkObj* object, float x, float y, float z);
void bgnd_unhide_sobj_list(int list_id);
void bgnd_hide_sobj_list(int list_id);
int random_percent(float percent);
void bgnd_sobj_start_morph(int object_id, int start_shape, int end_shape,
                           int ticks);
void delete_screen_obj_oid(int oid);
void reset_collision_system(void);
void pos_cam_for_current_level(void);
void reset_severed_limbs(int player);
void move_plyrs_to_round_start(void);
void set_far_clip_plane(float distance);
void skytemple_player_explode(
    unsigned int player, float x, float y, float z);
void skytemple_make_scream_sound(int player);
void turn_controllers_on(void);
void turn_controllers_off(void);
void bgnd_clear_face_opponent_flags(void);
void mab_script_trace_func(char* message);
void skytemple_arrange_fence_pebbles_around_pos(int player, int count,
                                                const Vec* position);
void skytemple_set_fence_pebble_vel(int player, int count, float x, float y,
                                    float z);
void scripted_camera_script_exit(void);
void bgnd_launch_fx_at_sobj_pos(char* name, int sobj_id, float y_offset);
int bgnd_get_first_shape_center_for_obstacle_id(int obstacle_id,
                                                int scratch_index);
void misc_data_set_obj_ptr1(MkObj* object);
MkObj* misc_data_get_obj_ptr1(void);
void misc_data_set_col_obj_id2(int object_id);
int misc_data_get_col_obj_id2(void);
void misc_data_set_col_obj_id(int object_id);
int misc_data_get_col_obj_id(void);
MkObj* bgnd_fetch_obj(int model_id);
void bgnd_swap_textures_tbl(const BgndSwapTextureEntry* entries, int frame);
void bgnd_swap_textures(int sobj_id, int material_id, int frame);
void bgnd_append_texture_to_material_tbl(
    const BgndAppendTextureEntry* entries);
void bgnd_append_texture_to_material(int sobj_id, int material_id,
                                     char* texture_name, int texture_slot);
void bgnd_light_set_color(int light_id, float red, float green, float blue);
float bgnd_get_float(int value_id);
unsigned int bgnd_get_u32(int value_id);
int bgnd_get_int(int value_id);
void mks_start_fatality_iceball(int player);
void mks_debug_display_cloth_ontop(int enabled);
void start_bow(int player, float strength);
int get_bid_with_flip(MkObj* object, unsigned int bone_id);
void start_plyr_attack(float radius);
void set_active_projectile_upward_attack(const Vec* velocity);
void set_active_add_ang_y(float angle);
void set_active_projectile_random_rot(float x, float y, float z);
void set_active_projectile_dn_sound(int sound_id);
void set_active_projectile_sound(int start_sound, int loop_sound,
                                 int end_sound);
void set_active_projectile_random_pos(float x, float y, float z);
void set_active_projectile_velocity_damp(const Vec* damping);
void set_active_projectile_target_pos(const Vec* position);
void set_active_projectile_max_ticks(int ticks);
void set_active_projectile_p_handler(ScriptEntryFn handler);
void set_active_projectile_target_ground(float speed, float target_y,
                                         float ground_y);
void set_active_projectile_velocity(const Vec* velocity);
void active_projectile_setup_done(void);
void set_active_projectile_hit_gnd_script(ScriptEntryFn entry);
void set_active_projectile_end_script(ScriptEntryFn entry);
void set_active_projectile_hit_script(ScriptEntryFn entry);
void set_active_projectile_not_duckable(void);
void set_active_projectile_rx_info(int reaction, float rate, int strength);
MkObj* start_projectile_from_plyr_bone(int bone_id, MkObj* existing_object,
                                       const char* model_name, float speed,
                                       float tolerance,
                                       const Vec* bone_offset);
void run_reaction_cleanup_function(PlyrPdata* player);
int reaction_xfer_him(int reaction, float rate, int strength);
void mks_set_cb1_wind_normal(float x, float y, float z);
void mks_bgnd_start_wind(float x, float y, float z);
void mks_npc_build_bones_tbl(int model_id, const int* bone_tags);
void mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
    PlyrPdata* player);
void mks_xfer_collision_info_plyr_to_bgnd_script(PlyrPdata* player,
                                                  ScriptEntryFn entry);
void mks_xfer_collision_info_plyr_to_script(ScriptEntryFn entry, int player);
void resume_effect_at_plyr_num_bid(
    int player, int bone_id, unsigned int effect_handle,
    int bind_mode, int blood_required);

/* Typed declarations used by imported script wrappers. */
int add_facial_damage(void *, float);
int add_npc_list_to_world(int);
int add_trigger_list_to_world(void);
int ani_1_frame(void);
int ani_no_pos(void);
int ani_through_end(void);
int ani_to_blend_frame(void *, float);
int ani_to_end(void);
int ani_to_frame_x(void *, float);
int ani_with_pos(void);
int ani_x_more_frames(void *, float);
int auto_ani_on(void);
int back_rollup_check(void);
int back_rollup_check_reverse(void);
int bgnd_active_sobj_no_ztest(void);
int bgnd_active_sobj_no_zwrite(void);
int bgnd_add_wall_to_hide(int);
int bgnd_add_wall_to_unhide(int);
int bgnd_allow_dirty_floor(void);
int bgnd_always_face_y(int);
int bgnd_clean_beetlelair(void);
int bgnd_clean_slaughterhouse(void);
int bgnd_clean_up_floor(void);
int bgnd_collision_if_rx_override(int);
int bgnd_collison_if_set_info(void);
int bgnd_collison_if_to_scripts_activate(void);
int bgnd_create_sobjs(void);
int bgnd_delete_danger_zone(int);
int bgnd_delete_proc_by_id(int);
int bgnd_detach_rope(int);
int bgnd_enable_wall_hider(int);
int bgnd_end_the_game_and_restart(void);
int bgnd_get_active_sobj_pos(int);
int bgnd_hide_active_sobj(void);
int bgnd_hide_mirror_guys(void);
int bgnd_hide_pebbles(int);
int bgnd_hide_preload_obj(int);
int bgnd_hide_sobj(int);
int bgnd_hide_sobj_and_children(int);
int bgnd_init_all_uv_scroll_w_control(void);
int bgnd_init_cracks(void);
int bgnd_init_timers(int);
int bgnd_kill_all_launched_sobjs(void);
int bgnd_no_z_test(int);
int bgnd_no_z_write(int);
int bgnd_npc_like_plyr(int);
int bgnd_preload_obj_attach_rope(int);
int bgnd_reg_col_cb_for_beetle_lair(void);
int bgnd_remove_cracks(void);
int bgnd_remove_wall_from_hider(int);
int bgnd_reset_players_animation_height(void);
int bgnd_reset_sobj(int);
int bgnd_restore_player(void);
int bgnd_set_active_danger_zone(int);
int bgnd_set_active_sobj(int);
int bgnd_set_active_sobj_rop(int);
int bgnd_set_fx_ang_dir_to_i_vector(void);
int bgnd_set_fx_ang_y(void *, float);
int bgnd_set_viewing_of_danger_zones(int);
int bgnd_setup_rx_handler(int);
int bgnd_sh_level_1(void);
int bgnd_sh_level_2(void);
int bgnd_shadow_control(int);
int bgnd_start_cracks(void);
int bgnd_start_sh_fx(void);
int bgnd_start_sobj_uv_scroll_tbl(int);
int bgnd_start_wall_hider(int);
int bgnd_swap_level(int);
int bgnd_takeover_plyr(int);
int bgnd_turn_off_backface_culling(int);
int bgnd_turn_on_backface_culling(int);
int bgnd_unhide_active_sobj(void);
int bgnd_unhide_mirror_guys(void);
int bgnd_unhide_pebbles(int);
int bgnd_unhide_preload_obj(int);
int bgnd_unhide_sobj(int);
int bgnd_unhide_sobj_and_children(int);
int bgnd_update_active_mksobj(void);
int bgnd_xfer_attacker(int);
int blast_effect_at_plyr(void);
int bulvan_function(int);
int change_monk_age(int);
int check_for_combo_message(void);
int clear_both_face_opponent_flags(void);
int clear_cliff_data(void);
int clear_his_f_constrained(void);
int clear_my_face_opponent_flag(void);
int configure_iceball(int);
int conversation_init(int);
int conversation_term(void);
int courtyard_start_lensflare(void);
int damage_him(void *, float);
int damage_me(void *, float);
int danger_zone_eligible_on(void);
void destroy_kabal_smoke(void);
int destroy_sobj_ctrl_proc(void);
int disable_attack5(int);
int disable_blocking(void);
int disable_both_repel_flags(void);
int disable_joy_temp(int);
int disable_mileena_collisions(int);
int disable_my_attacks(int);
int display_konquest_title(void);
int display_time_progression_images(int);
int dk_taunt_at_screen(void);
int dont_fence_plyr_in(int);
int drone_ai_increase_big_boss_stage(int);
int drone_dispatch_switches(int);
int drone_face_monk(void);
int drone_set_difficulty_level(int);
int drone_super_combo_refresh(void);
int ejb_call(int);
int ejb_release_other_player(int);
int ejb_too_close_repell(void);
int enable_all_my_blocking(void);
int enable_his_blocking(void);
int end_air_move(void);
int face_bleed_me(int);
int face_opponent_180(void);
int face_opponent_now(void);
int fade_fatality_screen(void);
int fire_trigger(int);
int flash_hit_at_bid(int);
int force_ai_style(int);
int forced_step_forward(void);
int freeze_player(void);
int front_rollup_check(void);
int get_projectile_script_last_pos(int);
int get_projectile_script_velocity(int);
int give_reward_to_player(int);
int gusher_destroy_list(void);
int head_tracking_off(void);
int hero_handle_conversation(void);
int hero_stop_moving(void);
int hero_turn_to_face_position(int);
int hide_konquest_object_by_uid(int);
int hide_objective_arrow_and_beam(void);
int high_flash_check(void);
int idle_hero_anim_proc(void);
int idle_his_anim_proc(void);
int if_collision_autoface_him(void);
int if_collision_autoface_me(void);
int init_3d_move(void);
int init_3d_move_no_aniproc(void);
int init_3d_move_no_face(void);
int init_air_move(void);
int init_air_move_no_aniproc(void);
int init_ground_move_no_aniproc(void);
int init_move(void);
int init_scripted_camera(void);
int init_still_move(void);
int initialize_background_danger_zones(void);
int initialize_clone_lights(int);
int interior_exit_button_script(void);
int jab_destroy_drink_obj_in_hand(void);
int jab_release_jade_boomerang(int);
int jab_setup_kiss_emitter_obj(int);
int jab_stop_dragon_king_shake(void);
void kabal_collision_control_victim(int);
int kenshi_teleport_position(void);
void kick_the_camera(void);
int kill_dynamic_pui(int);
int kill_konquest_dialog_procs(void);
int kill_lip_sync_procs(void);
int kill_pui(int);
int kobra_teleport_position(void);
int konquest_camera_return_to_normal(void);
int konquest_end_npc_interaction(void);
int konquest_end_npc_nis(void);
int konquest_fade_hud(int);
int konquest_hero_portal_in(void);
int konquest_hide_damashi(void);
int konquest_hide_hud(int);
int konquest_run_ending(void);
int konquest_set_current_portal_uid(int);
int konquest_show_hud(void);
int konquest_start_npc_interaction(void);
int konquest_start_npc_nis(void);
int konquest_transition_to_fight(int);
int load_tile_objects(int);
int low_flash_check(void);
int match_my_ypos_with_his(void);
int medium_flash_check(void);
int mileena_sky_set_position(void);
int mini_mission_completed(int);
int mini_mission_inactive(int);
int mk_chess_activate_my_properties(void);
int mk_chess_air_move(void);
int mk_chess_ani_1_frame(void);
int mk_chess_ani_idle(void);
int mk_chess_ani_to_end(void);
int mk_chess_blend_into_cell_orgin_in_x_frames_by_caller(void);
int mk_chess_blend_to_normal_stance(void);
int mk_chess_deactivate_my_properties(void);
int mk_chess_dont_constrain_piece(void);
void mk_chess_load_chess_table(void*);
int mk_chess_make_spellcaster(int);
int mk_chess_piece_die(void);
int mk_chess_piece_event_from_script(int);
int mk_chess_piece_is_idle(void);
int mk_chess_piece_match_y_ang_to_anim(void);
int mk_chess_piece_set_state(int);
int mk_chess_piece_temporarily_gone(void);
int mk_chess_set_glitch_stance_flag(void);
int mk_chess_set_piece_state(int);
int mk_chess_snap_to_my_cell_now(void);
int mk_chess_snap_to_stance(void);
int mk_chess_snd_request(int);
int mk_chess_spell_force_fight(void);
int mk_chess_spell_has_completed(void);
int mk_chess_spell_has_completed_but_wait_for_fight(void);
int mk_chess_spell_kill_target(int);
int mk_chess_spell_rescue_current_target(void);
int mk_chess_stop_me(void);
int mk_chess_wait_until_attack_cam_closes_in(void);
int mks_cb1_eq_cloth_bone(int);
int mks_cb2_eq_cloth_bone(int);
int mks_cc1_insert_cb1(void);
int mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs(void);
int mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside(void);
int mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl(void);
int mks_ccp1_insert_cb1(void);
int mks_npc_start_cloth_bones(int);
int mks_set_cb1_target_bone_cb2(void);
void mks_set_flipped_bones(MkFlippedBoneMap* bone_map);
void mks_start_goro_arms_fixup(void);
int nis_clear_event_list(void);
int nis_remove_non_participants(void);
int noob_victory_entrance(void);
int noobsmoke_sidekick_double_charge(void);
int noobsmoke_sidekick_projectile(void);
int npc_ani_1_frame(void);
int npc_ani_for_x_ticks(int);
int npc_ani_to_end(void);
int npc_ani_to_frame_x(void *, float);
int npc_blend_to_ani_string(int);
void npc_face_current_waypoint_angle(void);
int npc_fire_trigger(int);
int npc_hide_skip_message(void);
int npc_ignore_events(int);
int npc_open_door_at_waypoint(void);
int npc_play_random_dialog_sequence(void);
int npc_play_teleported_sound(void);
int npc_prepare_for_unconscious_state(void);
int npc_punch_reaction_standard_setup(void);
int npc_punch_reaction_standard_shutdown(void);
int npc_reset_my_timed_events(void);
int npc_restart_his_normal_behavior(int);
int npc_run_shove_animation(int);
int npc_set_ani_flags(int);
int npc_set_ani_frame(void *, float);
int npc_set_ani_speed(void *, float);
int npc_set_dialog_anim(int);
int npc_set_gravity(void *, float);
int npc_set_my_ang_y(void *, float);
int npc_set_my_conversation_counter(int);
int npc_set_my_punch_counter(int);
int npc_set_pinanim_flag(int);
int npc_set_snap_to_ground(int);
int npc_shove_reaction_standard_setup(void);
int npc_shove_reaction_standard_shutdown(void);
int npc_show_skip_message(void);
int npc_sleep(void *, float);
int npc_sleep_until_model_loaded(void);
int npc_snap_to_face_monk(void);
int npc_stand_still(void);
void npc_start_blood_fall(void);
int npc_start_goro_bone_match(int);
int npc_stop_goro_bone_match(void);
int npc_switch_camera_focus(int);
int npc_turn_and_face_next_waypoint(void);
int npc_turn_and_face_player(int);
int npc_wait_for_state_change(void);
int npc_wait_for_wake_up(void);
int obj_enable_grounding(int);
int pickup_dynamic_pui(int);
int pickup_pui(int);
int play_background_music(int);
int play_beam_advance_sound(int);
int player_add_item_to_inventory(int);
int player_feet_land_chores(void);
int plyr_rotate_obj_y180(void);
int plyr_set_gravity(void *, float);
int popup_reaction_max_hit_rules(void);
int pz_fighter_allow_continuation(void);
void pz_fighter_allow_easy_continuation(void);
void pz_fighter_check_breakout(void);
void pz_fighter_create_space_between_fighters(void);
void pz_fighter_create_space_between_fighters_for_special_moves(void);
int pz_fighter_distance_check_wo_super_check(void);
void pz_fighter_disallow_continuation(void);
void pz_fighter_dont_fudge_desired_distance(void);
float pz_fighter_exit(void);
int pz_fighter_force_repel_during_attack(void);
void pz_fighter_function(unsigned int);
float pz_fighter_long_exit(void);
void pz_fighter_move_into_fighting_position(void);
void pz_fighter_reaction_xfer_him(int);
void pz_fighter_release_other_player(int);
void pz_fighter_reset_continuation(void);
void pz_fighter_shaking(void);
void pz_fighter_step_throw_into_check(void);
void pz_fighter_wipe_blood_off_hands(void);
int random_dk_foot(void);
int random_voice_him(int);
int register_baraka_cb_functions(void);
void release_both_players(void);
int remove_collision_volume_on_object(void);
int remove_npc_list(int);
int remove_widescreen_bars(void);
int restore_collision_volume_on_object(void);
int restore_hero_grounding(void);
int resume_hero_state_process(void);
int retract_spear_from_camera(int);
int scorpion_teleport_position(void);
int set_active_projectile_2d_track(void);
int set_active_projectile_3d_track(void);
int set_active_projectile_continue_thru_hit(void);
int set_age_progression(int);
int set_ani_speed(void *, float);
int set_ani_weight(void *, float);
int set_attackers_attack_region(int);
int set_block_requirement(int);
int set_both_face_opponent_flags(void);
int set_cliff_watcher_round(int);
int set_current_time(int);
int set_hero_position_relative_to_chest(void);
int set_interaction_camera_script(int);
int set_konquest_region_number(int);
int set_krypt_character_pos(int);
int set_last_character_trained_with(int);
int set_look_at_npc(int);
int set_monk_age(int);
int set_movement_npc(int);
int set_my_float_1(void *, float);
int set_my_secondary_state(int);
int set_reference_pui(int);
int setup_for_flip_ani(void);
int setup_interior_fighting_arena(void);
int setup_vomit_slip_sound(void);
int sh_lower_level_pebble_hide(void);
int sh_lower_level_pebble_unhide(void);
int show_fight_message(int);
int show_objective_arrow_and_beam(void);
int show_player(int);
int show_shujinko_unlock_screen(int);
int slamdown_reaction_max_hit_rules(void);
int slow_ani_end(void *, float);
int smoke_victory_entrance(void);
int snd_major_hit_voice(void);
int snd_stop(int);
void sobj_no_zwrite(void* sobj);
int spad_norm_vector(int);
void special_move_cam_end(void);
int start_baraka_blades_monitor(void);
int start_baraka_jaw_monitor(void);
int start_bl_beetles_live_top_floor(void);
int start_chunk_launch_monitor(void);
int start_constrain_proc(void);
int start_hero_collisions(void);
void start_kabal_smoke(void *, float);
int start_konquest_ambient_sounds(void);
int start_mini_mission(int);
int start_rope_proc(void);
int start_shadow_watcher(void);
int start_sobj_ctrl_proc(void);
int start_sobj_launch_monitor(void);
int start_special_weapon_monitor(int);
int start_subobject_pulsing_effect(int);
int start_time_passing(void);
int step_throw_into_check(void);
int step_throw_outof_retract(void);
int stop_chest_camera_script(void);
int stop_hero_collisions(void);
int stop_konquest_ambient_sounds(void);
int stop_me(void);
int stop_time_passing(void);
int stop_vomit_slip_sound(void);
int super_charge_me(void);
int suspend_hero_grounding(void);
int suspend_hero_state_process(void);
int suspend_in_midair(void *, float);
int switch_plyr_positions(void);
int tightrope_restrictions_off(void);
int tightrope_restrictions_on(void);
int transition_to_region(int);
int trial_debug_mission_list(int);
int trial_mirror_anims_if_needed(void);
int trial_register_special_move(int);
void trial_restart_round(void);
int trial_set_next_setup_function(int);
int trial_set_num_rounds(int);
int trial_set_round_timer(int);
int trial_set_special_restrictions(int);
int trial_set_tick_function(int);
int trial_set_type(int);
void trial_setup_nis_scene(int);
int trial_show_monk(int);
int turn_into_energy_player(void);
int turn_me_pi(void);
int turn_to_face_exterior_door(void);
int turn_to_face_interior_door(void);
int unfreeze_player(void);
int unhide_konquest_object_by_uid(int);
int update_my_last_switch(void);
int wait_for_slot_load(int);
int wait_to_land(void);
int wall_eligible_off(void);
int wall_eligible_on(void);
int weapon_trail_off(void);
int weapon_trail_on(void);
int whoosh_fx(int);

/* Typed declarations used by imported script wrappers. */
int add_days_to_time(int, int);
int add_hours_to_time(int, int);
int add_months_to_time(int, int);
int add_object_to_tile(int, int, int, void *, float, float, float, float);
int add_to_konq_profile_value(int, int);
void add_widescreen_bars(float);
int add_years_to_time(int, int);
int adjust_his_damage_multiplier(void *, float);
int adjust_my_damage_multiplier(void *, float);
int air_collision_pause(int, void *, float, float);
int ani_loop_more_frames(void *, float);
int ani_to_frame_x_aniproc(void *, float);
int assign_obj_to_trigger(int, int);
int attach_pfx_to_object(int, char*, int);
int bgnd_act_at_time(int, int, void *, float, float, float);
int bgnd_add_new_normal_check_for_hider(void *, float, float, float, float);
int bgnd_add_scripted_brains_to_npc(int, int);
int bgnd_apply_active_sobj_pos_vel_drag(void *, float, float, float);
int bgnd_apply_zoffset(int, void *, float);
int bgnd_attach_rope_to_bgnd_obj(int, int, int);
int bgnd_collision_if_disable_col(int, int);
int bgnd_collision_if_enable_col(int, int);
int bgnd_collision_if_monitor_col_as(int, int, int, int);
int bgnd_collison_if_monitor_col(int, int, int, int, int, int, int);
int bgnd_collison_if_set_return_result(int);
int bgnd_create_pebbles(int, int, int, int, int);
int bgnd_current_rx_set_info(int, void *, float);
int bgnd_destroy_sobj_uv_scroll_w_control(int);
int bgnd_enable_danger_zone(int, int);
int bgnd_enable_obj_pos_and_ang_setting(int, int, int);
int bgnd_fade_object(int, void *, float);
int bgnd_force_ground_to(void *, float);
int bgnd_force_plyr_ground_plane(int, void *, float);
int bgnd_init_pebbles(int, int, int);
int bgnd_insert_obj_ctrl_section(int, int);
int bgnd_jtb_debug_info(void *, float, float, float);
int bgnd_launch_plyr_blood_fx(int, int);
int bgnd_make_displayed_item_pickupable_at_active_sobj_pos(int);
int bgnd_move_player(int, int, int);
int bgnd_npc_adjust_y_ang(int, void *, float);
int bgnd_npc_get_pos(int, int);
int bgnd_npc_set_ani_speed(int, void *, float);
int bgnd_npc_set_aux_int_data(int, int, int);
int bgnd_npc_set_pos(int, void *, float, float, float);
int bgnd_npc_set_pos_vel(int, void *, float, float, float);
int bgnd_npc_set_pos_vel_heading(int, void *, float);
int bgnd_npc_set_pos_y(int, void *, float);
int bgnd_npc_set_scale(int, void *, float, float, float);
int bgnd_npc_set_y_ang(int, void *, float);
int bgnd_npc_start_ani(int, int, int, void *, float, float);
int bgnd_obj_insert_obj_ctrl_section(int, int);
int bgnd_pebble_burst_at_chunk_pos(int, int, int);
int bgnd_pebble_burst_at_pebble_pos(int, int, int);
int bgnd_pebble_burst_at_pos(int, int, int, void *, float, float, float);
int bgnd_pebble_burst_set_end_state(int, int, int, int);
int bgnd_pebble_burst_set_value(int, int, int, int, void *, float, float, float, float, float, float);
int bgnd_pebble_burst_set_value_min_max(int, int, int, int, void *, float, float);
int bgnd_pebble_change_current_end_behavior(int);
int bgnd_pebble_gravity(int, void *, float);
int bgnd_pebble_rand_scale(int, void *, float, float);
int bgnd_pebble_set_current_info(int, void *, float);
int bgnd_pebble_set_current_pebble(int, int);
int bgnd_pebble_simple_launch_at_time(int, int, int, int);
int bgnd_place_crack_when_plyr_hits_ground(int);
int bgnd_place_object_at_position(int, int, int, int, int);
int bgnd_register_danger_zone_callback(int, int);
int bgnd_rotate_sobj(int, void *, float, float, float);
int bgnd_rotate_xz_about_orgin_active_sobj(void *, float);
int bgnd_run_camera_script(int, int);
int bgnd_set_active_sobj_ang(void *, float, float, float);
int bgnd_set_active_sobj_in_obj(int, int);
int bgnd_set_active_sobj_pos(void *, float, float, float);
int bgnd_set_active_sobj_pos_vel(void *, float, float, float);
int bgnd_set_active_sobj_scale(void *, float, float, float);
int bgnd_set_active_sobj_zoffset(void *, float);
int bgnd_set_collision_plane_for_launched_sobj(int, int, int);
int bgnd_set_danger_zone_center_position(void *, float, float, float);
int bgnd_set_danger_zone_depth(void *, float);
int bgnd_set_danger_zone_radius(void *, float);
int bgnd_set_danger_zone_width(void *, float);
int bgnd_set_danger_zone_y_angle(void *, float);
int bgnd_set_kill_plane_for_launched_sobj(int);
int bgnd_set_launch_velocity_based_on_sobj_pos(int, int, int, void *, float, float);
int bgnd_set_material_color(int, int, int, int, int, int);
int bgnd_set_new_ground_plane(void *, float);
int bgnd_set_player_shadow_ground_plane(int, void *, float);
int bgnd_set_plyr_gravity(void *, float);
int bgnd_set_sobj_launch_params(int, int, int, void *, float, float, float, float, float, float, float);
int bgnd_set_sobj_launch_params_exact(int, int, void *, float, float, float, float, float, float);
int bgnd_set_wall_hide_distance(void *, float);
int bgnd_sobj_cam_frustum_test_into_transparent(int, void *, float, float);
int bgnd_sobj_cam_volume_test_steer_over(int, void *, float, float);
int bgnd_sobj_get_ang(int, int);
int bgnd_sobj_set_alpha(int, int);
int bgnd_sobj_set_ang(int, void *, float, float, float);
int bgnd_sobj_set_ani_frame(int, int, int);
int bgnd_sobj_set_ani_framerate(int, int, void *, float);
int bgnd_sobj_set_pos(int, void *, float, float, float);
int bgnd_sobj_set_pos_vel(int, void *, float, float, float);
int bgnd_sobj_set_priority(int, int);
int bgnd_sobj_set_rel_pos(int, void *, float, float, float);
int bgnd_start_preload_sobj_morph(int, int, int, int);
int bgnd_start_preload_sobj_uv_scroll(int, int, void *, float, float, float, float);
int bgnd_start_script_in_proc(int, int);
int bgnd_start_script_in_proc_bigstack(int, int);
int bgnd_start_timer(int, int, int);
int close_exterior_doors(int, int);
int cloth_change_ground_plane_for(void *, float);
int damage_player(int, void *, float);
int delete_obstacle_from_background_by_id(int);
int disable_konquest_object_zwrite_by_uid(int);
int dk_voice_call(int, int);
int drone_apply_damage(int, void *, float);
int drone_change_to_style(int, int);
int drone_do_special_move(int, int);
void drone_lip_synch(int, LipSyncKeyframe*);
int drone_set_anim_step(void *, float);
int drone_set_damage_multiplier(int, void *, float);
int drone_set_handicap(int, void *, float);
int drone_set_health(int, void *, float);
void drone_set_position(int, float, float, float);
void drone_set_script(int, int);
int drone_set_special_directions(int, int);
int drone_set_switch_state(int, int);
int drone_start_bleeding(int, void *, float);
int enable_attached_sound_by_uid(int, int);
int enable_trigger(int, int);
int face_ang_from_pos_to_him(int, int, int);
int face_point(void *, float, float, float);
int fade_from_black(int, int);
int fade_from_white(int, int);
int fade_to_black(int, int);
int fade_to_white(int, int);
int fight_fx_im_hit_flash(int, int, int, int, void *, float);
int flash_hit_at_bid_with_y(int, void *, float);
int give_koin_award(int, int);
int give_krypt_key_to_player(int, int);
int hf_bgnd_set_in_setup_zone(int, int);
int hf_bgnd_set_smasher_mode(int, int);
int hide_player(int, int);
int hit_START_chores(int, int, void *, float, float);
int if_collision_slow_ani_x(void *, float, float);
int jab_attach_drink_obj_to_hand(int, int, int);
int jab_face_obj(int, int);
int jab_flash_screen(int, void *, float, float);
int jab_shake_dragon_king(void *, float, float);
int jab_start_jade_boomerang_throw(int, int, void *, float);
int konquest_fade_from_black(int, int);
int konquest_fade_to_black(int, int);
int konquest_open_door(int, int);
int konquest_run_camera_script(int, int);
int konquest_teleport_hero_to_location(int);
int konquest_transition_object_to_state(int, int, int);
int land_chores(int, int, void *, float, float);
int launch_me_up(void *, float, float);
int load_bgnd_style(int, char*, void*);
int load_script_as_reaction(int, int);
int mk_chess_add_movement_skill(int, int, int, int);
int mk_chess_ani_loop_more_frames(void *, float);
int mk_chess_ani_to_blend_frame(void *, float);
int mk_chess_ani_to_frame_x(void *, float);
int mk_chess_blend_into_cell_orgin_in_x_frames(void *, float);
int mk_chess_blend_to_ani(int, int, void *, float, float);
int mk_chess_blend_to_ani_frame(int, int, void *, float, float, float);
int mk_chess_blend_to_desired_cell_position_setting(void *, float);
int mk_chess_blend_to_my_cell_pos(void *, float);
int mk_chess_define_class_initial_power(void *, float);
int mk_chess_glitch_to_ani_frame(int, int, void *, float, float);
int mk_chess_init_piece(int, int);
int mk_chess_launch_special_fx(int, int, int);
int mk_chess_launch_up(void *, float, float);
int mk_chess_queue_up_piece_event(int, int);
int mk_chess_set_ani_speed(void *, float);
int mk_chess_set_cell_offset(void *, float, float, float);
int mk_chess_set_normal_stance_script(int);
int mk_chess_set_obj_move_weight(void *, float);
int mk_chess_set_piece_event_script(int, int);
int mk_chess_set_piece_info(int, void *, float);
int mk_chess_set_piece_type_as(int, int);
int mk_chess_shifter_switch(int, int, void *, float, float);
int mk_chess_snap_into_cell_orgin_over_x_frames(void *, float);
int mk_chess_spell_move_target_from_temp_area_to(int);
int mk_chess_spell_move_target_to_target(int, int);
int mk_chess_spell_move_target_to_temp_area(int);
int mk_chess_spell_set_target_health(int, void *, float);
int mk_chess_spell_show_target_portrait(int);
int mk_chess_spell_target_add_access_restrictions(int, int, int, int);
int mks_away_vel_update_by_group(int, int, void *, float, float, float);
int mks_bgnd_obj_enable_cloth_update(int, int);
int mks_blend_start_update_by_group(int, int);
int mks_cb1_add_coll_pt(void *, float, float, float);
int mks_cb1_set_coll_offset(void *, float, float, float);
int mks_cb1_set_coll_offset_xz(void *, float, float);
int mks_cb1_set_ground_y(void *, float);
int mks_cb1_set_scale(int, void *, float, float, float);
int mks_cc1_eq_insert_cloth_coll(int, void *, float);
int mks_cc1_expand_cyl(void *, float, float);
int mks_ccp1_eq_insert_cloth_coll_plane(int, void *, float, float, float, float);
int mks_cloth_bones_init_by_tbl(int, int);
int mks_debug_display_cloth_coll_cyl(int, int, int);
int mks_debug_display_cloth_coll_plane(void *, float);
int mks_gravity_update_by_group(int, int, void *, float, float, float, float);
int mks_insert_cloth_force_bones(void *, float, float);
int mks_mat_id_set_zbias(int, void *, float);
int mks_npc_cb1_eq_cloth_bone(int, int);
int mks_npc_cc1_eq_insert_cloth_coll(int, int, void *, float);
int mks_npc_cloth_bones_init_by_tbl(int, int, int);
int mks_npc_disable_ground_y_all_cloth_bones(int);
int mks_npc_set_ground_y_all_cloth_bones(int, void *, float);
int mks_npc_set_target(int, int, int);
int mks_obj_enable_update_cloth(int, int);
int mks_removehide_by_group(int, int);
int mks_set_ground_y_all_cloth_bones(void *, float);
int mks_set_update_delay(int, int);
int mks_shadow_scale(int, int, void *, float, float);
int mks_start_axis_indicator_p_axis_track_bone_world_mat(int, void *, float);
void mks_victim_bleed(int, int);
int move_player(int, int, int);
int move_player_no_constrain_update(int, int, int);
int myvel_his_angle_y(void *, float, float, float);
int myvel_his_angle_y_inout(void *, float, float, float);
int myvel_my_angle_y(void *, float, float, float);
int nb_npc_slave_plyr_process_collision(int);
int nbc_script_debug_point(int, int, void *, float);
int ncs_set_pebble_pos(int, int, int);
int npc_ani_to_blend_frame(void *, float);
int npc_assign_door_path(int, int);
int npc_assign_path(int, int, int);
int npc_assign_path_to_him(int, int, int, int);
int npc_at_waypoint_set_flags(int, int);
int npc_attack(int, int);
int npc_blend_to_ani(int, int, void *, float, float);
int npc_blend_to_ani_with_offset(int, int, void *, float, float);
int npc_change_path_speed(void *, float);
int npc_enable_event(int, int);
int npc_enable_his_event(int, int, int);
int npc_glitch_him_to_ani(int, int, int);
int npc_glitch_to_ani(int, int);
int npc_ignore_his_events(int, int);
int npc_lip_synch(int, int);
int npc_play_conversation_part(int, int, int);
int npc_play_dialog_and_anim_sequence(int, int);
int npc_play_two_player_one_shot_anims(int, int);
int npc_run_punch_animation(int, int, int, int, void *, float);
int npc_set_flags(int, int);
int npc_set_his_ang_y(int, void *, float);
int npc_set_his_conversation_counter(int, int);
int npc_set_his_flags(int, int, int);
int npc_set_his_punch_counter(int, int);
int npc_set_his_world_pos(int, void *, float, float, float);
int npc_set_my_ground_level(void *, float);
int npc_set_my_movement_weight(void *, float, float);
int npc_set_my_pos(void *, float, float, float);
int npc_set_my_world_pos(void *, float, float, float);
void npc_set_random_dialog_and_anim_sequence(int, int);
int npc_set_wake_up_time(int, int);
void npc_start_fx_at_his_position(void*, const char*, const Vec*);
void npc_start_fx_at_position(const char*, const Vec*);
int npc_take_control_of_him(int, int);
int npc_travel_path(int, int, int);
int npc_travel_path_anim_override(int, int, int, int);
int npc_travel_to_world_position(int, int);
int obj_get_scale(int, int);
int obj_scale_over_time(int, int, void *, float);
void obj_set_flipped_bones(MkObj* object, MkFlippedBoneMap* bone_map);
void obj_set_scale(MkObj*, void*);
int obj_set_z_offsets(int, void *, float);
int obj_sobj_cam_frustum_test_into_transparent(int, int, void *, float, float);
int open_chest_and_give_item_to_player(int, int);
int open_chest_and_unlock_kontent(int, int);
int pan_vol_pitch_snd_req(int, void *, float, float, float);
int pebble_get_pos(int, int, int);
int play_sound_2(int, int);
int player_impale(int, int);
int player_remove_item_from_inventory(int);
int plyr_scale_pos_vel(void *, float, float, float);
int plyr_set_vel_xz_y(void *, float, float);
int plyr_start_script_in_plyr_pdata_proc(int, int, int);
int plyr_start_script_in_proc(int, int);
int plyr_weapon_grab(int, int);
int pui_delay_spawn(int, void *, float);
int pui_play_pfx_sequence(int, int, int);
int pui_set_color(int, unsigned char, unsigned char, unsigned char, unsigned char);
int pui_set_kill_time(int, int, int);
int pz_fighter_check_to_toggle_obj_and_ani_flips(int);
int pz_fighter_force_reaction_in_ticks(int, int);
int pz_fighter_register_move(int, int, int, int, int);
int random_hit_n_voice(int, int);
int random_snd_req_delay(int, int);
int rd_set_impact_vector(void *, float);
int release_kamidogu(int, int);
int remove_collision_volume_on_object_with_uid(int);
int restore_collision_volume_on_object_with_uid(int);
void resume_effect_at_obj_bid(MkObj*, int, unsigned int, int, int);
int run_camera_script(int, int, int);
int save_hero_position_and_angle_prior_to_fight(void *, float);
int set_active_projectile_block_script(int);
int set_active_projectile_velocity_to_hit_gnd(void *, float);
int set_ani_speed_miss_hit(void *, float, float);
int set_background_obstacle_disable_flag(int, int);
int set_background_obstacle_repel_flag(int, int);
int set_hero_punched_ground_collisions(int);
int set_his_damage_multiplier(void *, float);
int set_konq_profile_value(int, int, int);
int set_konquest_object_face_y_by_uid(int);
int set_konquest_object_render_order_priority_by_uid(int, int);
int set_konquest_weather(int, int, int);
int set_krypt_character_angle(void *, float);
int set_krypt_character_anim_script(int, int, void *, float);
int set_krypt_character_previous_root_angle(void *, float);
int set_monk_position(void *, float, float, float, float);
int set_my_damage_multiplier(void *, float);
int set_pui_status(int, int);
int set_snd_vol(int, int, void *, float);
int set_tile_grid_size(int, int);
int set_tile_visibility(int, int);
void shake_camera(int, void *, float);
int share_my_attack_info(void *, float, float);
int slow_ani_x(void *, float, float);
int slow_ani_x_if_miss(void *, float, float, float);
int snd_req_delay(int, int);
int sobj_set_alpha(int, int);
int spad_add_vector(int, void *, float, float, float);
int spad_rotate_xz_vector(int, void *, float);
int spad_scale_vector(int, int, void *, float);
int spad_set_heading_vector_to(int, void *, float, float);
int spad_set_vector(int, int);
int spad_set_vector_setting(int, void *, float, float, float);
int spad_set_vector_y(int, void *, float);
int spad_set_y_angle_plus_offset_from_xz_vector(int, void *, float, float, float);
int spad_sub_vectors(int, int, int);
int spawn_pui(int, int, int);
int start_character_separation_process(void *, float);
int start_cliff_watcher(void *, float);
int start_konquest_interior(int, int, int, int, int, int, int);
int start_scorpion_teleport_scale(void *, float, float);
void start_subzero_decoy(void*, float);
int transition_hero_to_anim_script(int, int, void *, float, float);
int transition_to_krypt_character_anim_script(int, int, int);
int trial_add_success_condition(int, int, int);
int trial_set_combo_requirement(int, void *, float);
int trial_set_ending_functions(int, int);
int trial_set_next_mission(int, int, int, int, int, int, int, int);
int trial_set_round_health_restoration(void *, float);
void trial_start_countdown(int, float, float);
void trial_state_collision_check(int, int);
int trigger_set_time_for_enable(int, int, int, int);
void uv_my_angle_y(void* direction, float angle_offset);
int xfer_player_proc_to_script(int, int);

/* Typed declarations used by imported script wrappers. */
int plyr_invulnerable_to_projectiles(int, int);

/* Typed declarations used by imported script wrappers. */
int advance_my_sidekick_from_behind_with_moveset(void);
int am_i_airborn_check_in_reaction(void);
int bgnd_add_fx_to_hide(void);
float bgnd_current_rx_get_info(int);
int bgnd_fx_get_binded_obj(int);
float bgnd_get_camera_y_angle(void);
float bgnd_get_camera_z_pos(void);
int bgnd_get_exec_tick_ctr(void);
int bgnd_get_obj_pointer(int);
int bgnd_get_preload_obj(int);
float bgnd_get_sobj_ang_y(int);
int bgnd_is_active_sobj_hidden(void);
int bgnd_kill_fx(void);
int bgnd_launch_plyr_up_and_forward_running(void);
float bgnd_npc_get_ang_y(int);
float bgnd_pebble_fetch_current_info(int);
int bgnd_pfx_reset_effect(void);
int bgnd_pfx_resume_effect(void);
float bgnd_sobj_get_x_pos(int);
float bgnd_sobj_get_y_pos(int);
float bgnd_sobj_get_z_pos(int);
int bgnd_timer_get_tick_count(int);
int can_fallingcliff_fall(void);
int current_player_is_drone(void);
float degrees_to_rad(void *, float);
int do_i_have_life_left(void);
int drone_ai_should_ermac_fly_kick(void);
int drone_ai_should_ermac_ground_slam(void);
int float_to_int(void *, float);
float frand(void *, float);
int get_active_npc_data(void);
int get_building_id_for_exterior(void);
int get_cliff_data(void);
int get_cliff_watcher_round(void);
int get_collision_result(void);
int get_current_bgnd(void);
int get_doors_for_exterior(void);
int get_exec_tick_ctr(void);
float get_game_speed(void);
int get_general_pebble_data(int);
int get_hero_state(void);
int get_his_previous_state(void);
int get_his_secondary_state(void);
float get_ir_cam_ang_x(int);
float get_ir_cam_ang_y(int);
float get_ir_cam_ang_z(int);
float get_ir_cam_pos_x(int);
float get_ir_cam_pos_y(int);
float get_ir_cam_pos_z(int);
int get_kombat_difficulty(void);
int get_konquest_tile_objects_obj(void);
int get_krypt_anim_pdata(void);
int get_krypt_character_obj(void);
int get_krypt_current_column(void);
int get_krypt_current_row(void);
int get_last_character_trained_with(void);
int get_mode_of_play(void);
int get_monk_age(void);
int get_my_particle_player_bank_num(void);
int get_my_plyr_num(void);
int get_pickup_object(void);
int get_previous_konquest_region_number(void);
int get_projectile_his_plyr_num(void);
int get_projectile_script_plyr_num(void);
int get_projectile_script_plyr_pdata(void);
int get_pui_status(int);
int get_sobj_pebble_obj(int);
int get_taunts_performed(void);
int get_tile_sobj_by_id(int);
int get_victory_flip_flags(void);
float int_to_float(int);
int is_big_boss(int);
int is_blood_disabled(void);
int is_drone(void);
int is_he_airborn(void);
int is_he_blocking(void);
int is_load_meter_active(void);
int is_local_plyr(void);
int is_mini_mission_active(int);
int is_mini_mission_completed(int);
int is_mini_mission_started(int);
int is_reaction_xfer_him_allowed(void);
float jump_towards_opponent_bgnd_transition(void);
int konquest_passed_last_mission(void);
int load_krypt_character(void);
int local_collision_allowed_plyr_pdata(void);
int mk_chess_active_piece_near_edge(void);
int mk_chess_check_glitch_into_stance(void);
int mk_chess_check_snap_into_stance(void);
int mk_chess_fetch_active_defined_team(void);
float mk_chess_get_piece_event_data(int);
float mk_chess_get_piece_info(int);
float mk_chess_spell_get_target_health(int);
float mk_chess_spell_get_target_max_health(int);
int mk_chess_spell_is_this_a_forced_fight(void);
float mks_get_victim_to_tr_dot(int);
int noobsmoke_fire_projectile_request(void);
int npc_get_collision_direction_in_script(void);
int npc_get_conversation_count(void);
int npc_get_flag_state(int);
int npc_get_obj(int);
int npc_get_punch_count(void);
int npc_punch_reaction_check_data(void);
int player_has_item(int);
float plyr_get_anim_frame(void);
float plyr_get_anim_hiframe(void);
int plyr_get_f_constrained(int);
float plyr_get_pos(int);
int plyr_snd_req(int);
float pz_fighter_fetch_distance_to_center_pos(void);
int pz_finish_him_request(void);
float rad_to_degrees(void *, float);
int random_foot(int);
int random_hit(int);
int random_snd_req(int);
int random_voice(int);
int refresh_rate(void);
int restart_effect(void);
int set_active_projectile_tracking_light(int);
float sfrand(void *, float);
int snd_req(int);
float spad_xz_length_vector(int);
int spawn_dynamic_pui(int);
int spawn_dynamic_pui_critical(int);
float throw_spear(void);
int trial_get_background_root(void);
int trial_invisible_callback(int);

/* Typed declarations used by imported script wrappers. */
float bgnd_blood_control(int, int, void *, float);
int bgnd_create_pebbles_with_sobj(int, int, int, int);
float bgnd_get_anim_info(int, int, void *, float);
int bgnd_npc_get_aux_int_data(int, int);
float bgnd_process_active_sobj_info(int, void *, float, float);
float bgnd_process_collision_info(int, void *, float, float, float, float, float, float, float, float);
int build_bones_tbl(int, int);
int fire_spear_at_camera(int, int);
int get_konq_profile_value(int, int);
int is_character_unlocked_in_profile(int, int);
int jab_attach_point_light_to_obj_bone(int, int, int);
int jab_spawn_point_light_at_world_pos(int, int);
int konquest_start_damashi(void *, float, float, float);
int launch_fx_at_pos_with_obj(int, void *, float, float, float);
int mk_chess_fetch_active_defined_teams_class(int);
int mk_chess_fetch_bp_num_based_on_pchr_num(int);
int mk_chess_piece_test_and_set_timer(int, int);
int mk_chess_request_piece_script_for_action(int);
int mk_chess_xfer_piece_from_scripts(int, int, int);
int ncs_create_pebble_monitor_proc(int, int, int, int);
int ncs_create_pebbles_with_sobj(int, int);
int npc_get_his_flag_state(int, int);
int pan_vol_pitch_random_hit(int, void *, float, float, float);
int pan_vol_pitch_random_snd_req(int, void *, float, float, float);
int pan_vol_snd_req(int, void *, float, float);
int plyr_snd_req_no_plyr_proc(int, int);
int snd_req_vol(int, void *, float);
float spad_get_pos(int, int);
float spad_xz_cos_two_vectors(int, int);
float spad_xz_dot_xz(int, int);

/* Typed declarations used by imported script wrappers. */
int bgnd_launch_fx_at_plyr_pos_and_y(void *, float);
int bgnd_set_fx_z_offset(void *, float);

/* Typed declarations used by imported script wrappers. */
int ani_col_abort(int, int, int, float, float, float, float);
int ani_to_fall_to_frame(int, void *, float, float);
int ani_to_frame_sound(int, float, float);
int ani_to_frame_x_col(int, int, int, float, float, float, float);
int animate_obj(int, void*, int, int, int, int, float);
extern int heart_beat;
int attach_sound_to_object_by_uid(int, int, int, int, float, float);
int attach_wiff_to_konquest_object_by_uid(int, char*, void*, float);
int bgnd_create_danger_zone(int, int, int, int, float);
int bgnd_launch_fx_at_plyr_bid(int);
int bgnd_launch_fx_to_sobj(int);
int bgnd_launch_plyr_up_and_forward(int, int, float, float, float, float, float);
int bgnd_launch_sobj(int, int, int, int, int, int, int, float);
int bgnd_pebble_change_current_behavior(int, int, float, float, float, float, float, float);
int bgnd_pebble_change_current_behavior_to_bounce(int, int, float, float, float, float, float, float);
int bgnd_pebble_launch_at_time(int, int, int, int, float, float, float, float, float, float, float, float);
int bgnd_place_point_light_for_ticks(int, int, int, float);
int bgnd_place_weapon_at_position(int, int, int, int, int, int, int, float, float, float, float, float, float, float, float);
int bgnd_preload_named_model(int);
int bgnd_pulsate_object(int, int, int, void *, float, float);
int bgnd_pulsate_object_with_caps(int, int, int, int, int, float, float);
int bgnd_pulsate_object_with_caps_and_scale(int, int, int, int, int, void *, float, float, float, float, float, float, float, float);
int bgnd_set_sobj_uv_scroll_abs_values(int, float, float, float, float);
int bgnd_set_sobj_uv_scroll_rate_values(int, float, float, float, float);
int bgnd_start_sobj_uv_scroll(int, int, float, float, float, float);
int bgnd_start_sobj_uv_scroll_w_control(int, int, int, float, float, float, float);
int display_konquest_text(int, int, float, float, float);
typedef struct AnimScript AnimScript;
void drone_blend_to_ani(AnimScript*, int, float);
int force_away(int, int, float, float);
int force_forward(int, int, float, float);
int got_hit_fx(int, int, int, int, int, int, float);
int hero_start_fx_at_position(int);
void interaction_cam_set_target_info(int, float, float, float, float, float, float);
int konquest_setup_pui_particle(int);
int konquest_use_portal(int, int, int, float, float, float);
int limb_sever_set_motion(int, int, int, int, int, int, int, int, float, float, float);
int mk_chess_ani_until_reached_destination(int, float, float, float, float, float);
int mk_chess_force_away(int, int, float, float);
int mk_chess_launch_n_land_ani_with_xz(int, int, int, float, float, float, float, float, float, float, float);
int mk_chess_place_special_cell_at(int, int, int, int, float, float, float, float);
int mk_chess_put_active_piece_at_cell(int, float, float);
int mk_chess_rotate_towards_cell(int, void *, float, float, float, float);
int mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(int, int, int, int, void *, float, float, float, float);
int mks_set_rotate_update_by_group(int, int, int, float, float, float);
int mks_set_sin_update_by_group(int, int, int, int, float, float, float, float, float, float);
int obj_grnd_bounce(int, int, int, void *, float, float, float);
int obj_match_obj_pos(int, int, int, float);
int parse_args(void*, ...);
int npc_set_anim_proc(ScriptProcEntryFn);
int plyr_spawn_his_anim_limb(
    int, int, int, void*, int, ScriptProcEntryFn, unsigned char*, float);
int player_area_collision_check(int, int, float, float, float);
float pz_fighter_inline_force_away_with_ani(int, int, float, float);
int set_active_projectile_collision_info(int, void *, float, float, float);
int shake_hit_voice(int, int, int, float);
void show_text(int, unsigned int, unsigned int, unsigned int, int, float, float, float);
int sidekick_switch_style_swap(int, float);
int single_frame_collision_check(int, int, int, void *, float, float, float);
int special_move_cam_him(int, int, int, float, float, float, float, float);
void special_move_cam_setup(int, int, int, float, float, float, float, float);
void special_move_cam_setup2(int, int, int, MkObj*, MkObj*, float, float,
                             float, float, float);
void start_gore2_pebbles(
    unsigned int object_id, int bone, MkObj* source,
    FighterMirror* decal_owner, const Vec* velocity,
    const Vec* rotation, const Vec* scale,
    const Vec* position_offset, float vertical_acceleration,
    float bounce_scale, int bounce_count);
int start_gusher(int *, int, int, int, int, int);
int transition_to_anim_script_frame(int, void*, int, void*, float, float);
void trial_do_dialog(int, int, float, float, float, unsigned int, int);
void trial_show_spoken_text_window(int, float, float, float, int, int, int, int, int);
void trial_show_text_window(int, int, int, float, float, float);
float two_player_animation_blend(int, int, float, float);

/* Data used by imported script wrappers. */
float p_animated_intro_done(void);

/* Typed declarations used by imported script wrappers. */
int credits_add_text(char*, int);
int trial_add_required_sequence(char*);
int trial_set_move_message(char*);

/* Typed declarations used by imported script wrappers. */
int attack_to_frame_x(void*, int, int, int, float, float, float, float);
int launch_n_land_ani(void*, int, void*, float, float, float, float, float, float);
int lower_mines_ani_to_point(void*, int, int, int, float, float, float, float, float, float);
int newani_to_frame_x(void*, int, float, float, float, float);
void pz_fighter_startup_attack(
    void*, int, int, int, unsigned int,
    float, float, float, float, float);
int two_player_animation(void*, void*, float);
int two_player_animation_flip(void*, void*, float);
int two_player_animation_match_attacker(void*, void*, float);
void* get_function_attributes_table(ScriptDistanceSlot*, int);
void trial_register_script_function(unsigned int);

void _script_hang(void) {
}

void _stop_usec_timer(void) {
    stop_usec_timer(0);
    printf("Elapsed time: %d\n");
}

void _start_usec_timer(void) {
    start_usec_timer(0);
}

void _obj_setup_for_animation(void) {
    obj_setup_for_animation(((ScriptRawArgs*)current_args)->slots[0].pointer,
                            ((ScriptRawArgs*)current_args)->slots[1].pointer,
                            ((ScriptRawArgs*)current_args)->slots[2].pointer,
                            ((ScriptRawArgs*)current_args)->slots[3].pointer);
}

/* Soft ceiling: _npc_set_anim_proc ~96.25% -- pooled-string relocation labels only. */
void _npc_set_anim_proc(void) {
    int function_index;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x12,
               &function_index);
    npc_set_anim_proc(
        (ScriptProcEntryFn)script_callable_function_table[function_index - 1]);
}

void _animate_obj(void) {
    ScriptArgsRef args;
    int temp_r31_91;

    args.bytes = current_args;
    temp_r31_91 = args.raw->slots[0].i;
    ((ScriptRawResult*)active_cmdscript)->value.i = animate_obj(temp_r31_91, get_animation(args.raw->slots[1].i), args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[6].i, args.raw->slots[5].f);
}

void _start_gusher(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = start_gusher(&heart_beat, args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i);
}

/* Soft ceiling: _plyr_spawn_his_anim_limb ~82.12% -- typed call ABI scheduling. */
void _plyr_spawn_his_anim_limb(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        plyr_spawn_his_anim_limb(
            ((ScriptRawArgs*)current_args)->slots[0].i,
            ((ScriptRawArgs*)current_args)->slots[1].i,
            ((ScriptRawArgs*)current_args)->slots[2].i,
            (void*)get_animation(((ScriptRawArgs*)current_args)->slots[3].i),
            ((ScriptRawArgs*)current_args)->slots[4].i,
            script_callable_function_table[
                ((ScriptRawArgs*)current_args)->slots[5].i - 1],
            current_args, ((ScriptRawArgs*)current_args)->slots[6].f);
}
void _xfer_proc(void) {
    xfer_proc(((ScriptRawArgs*)current_args)->slots[0].pointer,
              *(void**)(exit_table_340 +
                        ((ScriptRawArgs*)current_args)->slots[1].i * 4 + 0x54));
}

void _transition_to_anim_script_frame(void) {
    ScriptArgsRef args;
    int temp_r31_191;

    args.bytes = current_args;
    temp_r31_191 = args.raw->slots[0].i;
    ((ScriptRawResult*)active_cmdscript)->value.i = transition_to_anim_script_frame(temp_r31_191, get_animation(args.raw->slots[1].i), args.raw->slots[2].i, current_args, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _two_player_animation_blend(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_animation(args.raw->slots[0].i);
    ((ScriptRawResult*)active_cmdscript)->value.f = two_player_animation_blend(args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _set_anim_script(void) {
    ScriptArgsRef args;
    AnimPdata* animation;

    args.bytes = current_args;
    animation = (AnimPdata*)args.raw->slots[0].pointer;
    set_anim_script(
        animation,
        (AniData*)get_animation(args.raw->slots[1].i),
        args.raw->slots[2].i);
}

void _anim_pdata_for_proc(void) {
    void* pdata;

    pdata = 0;
    if (((ScriptRawArgs*)current_args)->slots[0].pointer != 0) {
        pdata = pdata_of_proc(((ScriptRawArgs*)current_args)->slots[0].pointer);
    }
    ((ScriptRawResult*)active_cmdscript)->value.pointer = pdata;
}

void _set_bonematcher_flag(void) {
    ScriptFlagWordView* object;
    int shift;

    object = ((ScriptFlagArgs*)current_args)->object;
    shift = 31 - ((ScriptFlagArgs*)current_args)->bit;
    if (((ScriptFlagArgs*)current_args)->enabled != 0) {
        object->flags = object->flags | (1U << shift);
    } else {
        object->flags = object->flags & ~(1U << shift);
    }
}

void _get_bonematcher_flag(void) {
    unsigned int flags;
    int bit;

    flags = *(unsigned int*)(*(char**)(current_args + 4) + 8);
    bit = ((ScriptRawArgs*)current_args)->slots[1].i;
    ((ScriptRawResult*)active_cmdscript)->value.i =
        (flags & (1U << (31 - bit))) != 0;
}

void _update_mkobj(void) {
    update_mkobj(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _get_plyr_pdata_flag(void) {
    unsigned int flags;
    int bit;

    flags = *(unsigned int*)(*(char**)(current_args + 4) + 0x1c);
    bit = ((ScriptRawArgs*)current_args)->slots[1].i;
    ((ScriptRawResult*)active_cmdscript)->value.i =
        (flags & (1U << (31 - bit))) != 0;
}

void _set_obj_flag(void) {
    ScriptFlagWordView* object;
    int shift;

    object = ((ScriptFlagArgs*)current_args)->object;
    shift = 31 - ((ScriptFlagArgs*)current_args)->bit;
    if (((ScriptFlagArgs*)current_args)->enabled != 0) {
        object->flags = object->flags | (1U << shift);
    } else {
        object->flags = object->flags & ~(1U << shift);
    }
}

void _get_obj_flag(void) {
    unsigned int flags;
    int bit;

    flags = *(unsigned int*)(*(char**)(current_args + 4) + 8);
    bit = ((ScriptRawArgs*)current_args)->slots[1].i;
    ((ScriptRawResult*)active_cmdscript)->value.i =
        (flags & (1U << (31 - bit))) != 0;
}

void _get_limb_obj(void) {
    GetLimbObjArgs* args;
    LimbRuntime* runtime;
    LimbProcLatch* latch;
    MkHdr* object;
    unsigned int bone_index;

    args = (GetLimbObjArgs*)current_args;
    bone_index = args->bone_index;
    runtime = args->runtime;
    latch = &runtime->bone_procs[bone_index];
    object = latch->hdr;
    if (object != 0) {
        if (object->instance != latch->instance) {
            object = 0;
        }
    } else {
        object = 0;
    }
    ((ScriptPointerResult*)active_cmdscript)->value = object;
}

void _destroy_item_obj(void) {
    ScriptArgsRef args;
    ScriptObjectRef* ref;
    MkHdr* object;
    ScriptDestroyVtable* vtable;

    args.bytes = current_args;
    ref = args.raw->slots[0].pointer;
    object = ref->object;
    if (object != 0 && object->instance != ref->instance) {
        object = 0;
    }
    if (object != 0) {
        object = ref->object;
        if (object->instance != 0) {
            vtable = (ScriptDestroyVtable*)object->vtbl;
            vtable->destroy(object->vtbl);
        }
        ref->object = 0;
        ref->instance = 0;
    }
}

void _init_item_obj(void) {
    int* item;

    item = *(int**)(current_args + 4);
    item[0] = 0;
    item[1] = 0;
}

void _ck_item_obj(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    ScriptObjectRef* ref;
    MkHdr* object;

    args.bytes = current_args;
    result.bytes = active_cmdscript;
    ref = args.raw->slots[0].pointer;
    object = ref->object;
    if (object != 0 && object->instance != ref->instance) {
        object = 0;
    }
    result.pointer->value = object;
}

void _insert_item_obj(void) {
    int* item;
    int* object;

    object = *(int**)(current_args + 4);
    item = *(int**)(current_args + 8);
    item[0] = (int)object;
    item[1] = object[1];
}

void _get_new_mkobj(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        get_mkobj_frame(((ScriptRawArgs*)current_args)->slots[0].i, 0);
}

void _nis_init(void) {
    ScriptActiveState* script;
    int arg0;
    int arg1;

    script = (ScriptActiveState*)active_cmdscript;
    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x14,
               &arg0, &arg1);
    nis_init(script->state, arg0, arg1);
}

void _clear_fight_hud(void) {
    setup_screen_for_fatality();
}

void _drone_blend_to_ani(void) {
    int sp10;
    int spC;
    float sp8;
    AnimScript* animation;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x17, &sp10, &spC, &sp8);
    animation = (AnimScript*)get_animation(sp10);
    drone_blend_to_ani(animation, spC, sp8);
}

void _camera_set_target(void) {
    Vec angle;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &angle.x, &angle.y, &angle.z);
    set_camera_target_angle(&angle);
}

void _camera_set_destination(void) {
    Vec position;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &position.x, &position.y, &position.z);
    set_camera_destination(&position);
}

void _trial_add_required_attack_nohit(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_add_required_attack(args.trial_attack->attack,
                              args.trial_attack->count, 0);
}

void _trial_add_required_attack(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_add_required_attack(args.trial_attack->attack,
                              args.trial_attack->count, 2);
}

void _pz_fighter_should_continue_move(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = (pz_fighter_state >> 2) & 1;
}

void _set_background_color(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_background_color(args.background_color->red,
                         args.background_color->green,
                         args.background_color->blue,
                         args.background_color->alpha);
}

void _camera_init_animation(void) {
    int animation_id;
    AniData* animation;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1F,
               &animation_id);
    animation = (AniData*)get_animation(animation_id);
    camera_init_animation(animation, p_animated_intro_done);
}

void _camera_set_angle(void) {
    Vec angle;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &angle.x, &angle.y, &angle.z);
    set_camera_angle(&angle);
}

void _camera_set_position(void) {
    Vec position;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &position.x, &position.y, &position.z);
    set_camera_position(&position);
}

void _set_move_pz_attributes_to(void) {
    int sp8;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x21, &sp8);
    ((ScriptCommandView*)active_cmdscript)->move_attributes = sp8;
}

void _get_current_player_number(void) {
    if (((PlyrPdata*)plyr_pdata)->plyr_num == 1) {
        ((ScriptRawResult*)active_cmdscript)->value.i = 1;
    } else {
        ((ScriptRawResult*)active_cmdscript)->value.i = 0;
    }
}

void _randu0(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    unsigned short value;

    args.bytes = current_args;
    value = randu0(args.single_int->value);
    script.bytes = active_cmdscript;
    script.integer->value = value;
}

void _my_attack_hit(void) {
    ScriptResultRef script;
    PlyrPdata* player;

    player = (PlyrPdata*)plyr_pdata;
    if (player->collision_result == 1) {
        script.bytes = active_cmdscript;
        script.integer->value = 1;
        return;
    }
    script.bytes = active_cmdscript;
    script.integer->value = 0;
}

void _pz_fighter_startup_attack(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    pz_fighter_startup_attack(
        ((ScriptCommandView*)active_cmdscript)->animation,
        args.raw->slots[5].i, args.raw->slots[6].i,
        args.raw->slots[7].i, args.raw->slots[8].i,
        args.raw->slots[1].f, args.raw->slots[2].f,
        args.raw->slots[3].f, args.raw->slots[4].f,
        args.raw->slots[9].f);
}

void _pz_fighter_distance_check_wo_super_check(void) {
    unsigned int function_index;
    int result;

    result = pz_fighter_distance_check_wo_super_check();
    if (result == 1) {
        function_index = ((ScriptRawArgs*)current_args)->slots[0].u;
        ACTIVE_DISTANCE_SCRIPT->program_counter =
            ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
            ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1]
                .code_offset;
        ACTIVE_DISTANCE_SCRIPT->attributes =
            get_function_attributes_table(
                ACTIVE_DISTANCE_SCRIPT->slot, function_index);
        trial_register_script_function(function_index);
        function_index--;
        ACTIVE_DISTANCE_SCRIPT->function_name =
            (char*)(ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index]
                        .name_offset +
                    ACTIVE_DISTANCE_SCRIPT->slot->string_relocation) -
            1;
    } else if (result == 2) {
        function_index = ((ScriptRawArgs*)current_args)->slots[1].u;
        ACTIVE_DISTANCE_SCRIPT->program_counter =
            ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
            ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1]
                .code_offset;
        ACTIVE_DISTANCE_SCRIPT->attributes =
            get_function_attributes_table(
                ACTIVE_DISTANCE_SCRIPT->slot, function_index);
        trial_register_script_function(function_index);
        function_index--;
        ACTIVE_DISTANCE_SCRIPT->function_name =
            (char*)(ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index]
                        .name_offset +
                    ACTIVE_DISTANCE_SCRIPT->slot->string_relocation) -
            1;
    }
}

void _pz_fighter_distance_check(void) {
    unsigned int function_index;
    int result;

    result = pz_fighter_distance_check();
    if (result == 1) {
        function_index = ((ScriptRawArgs*)current_args)->slots[0].u;
        ACTIVE_DISTANCE_SCRIPT->program_counter =
            ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
            ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1]
                .code_offset;
        ACTIVE_DISTANCE_SCRIPT->attributes =
            get_function_attributes_table(
                ACTIVE_DISTANCE_SCRIPT->slot, function_index);
        trial_register_script_function(function_index);
        function_index--;
        ACTIVE_DISTANCE_SCRIPT->function_name =
            (char*)(ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index]
                        .name_offset +
                    ACTIVE_DISTANCE_SCRIPT->slot->string_relocation) -
            1;
    } else if (result == 2) {
        function_index = ((ScriptRawArgs*)current_args)->slots[1].u;
        ACTIVE_DISTANCE_SCRIPT->program_counter =
            ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
            ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1]
                .code_offset;
        ACTIVE_DISTANCE_SCRIPT->attributes =
            get_function_attributes_table(
                ACTIVE_DISTANCE_SCRIPT->slot, function_index);
        trial_register_script_function(function_index);
        function_index--;
        ACTIVE_DISTANCE_SCRIPT->function_name =
            (char*)(ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index]
                        .name_offset +
                    ACTIVE_DISTANCE_SCRIPT->slot->string_relocation) -
            1;
    }
}

void _pz_fighter_set_y_constrain(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pz_fighter_set_y_constrain(plyr_obj, args.pz_constrain->mode,
                               args.pz_constrain,
                               args.pz_constrain->value);
}

void _pz_fighter_attack(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    void* animation;

    args.bytes = current_args;
    animation = get_animation(args.attack->animation_id);
    script.bytes = active_cmdscript;
    script.command->animation = animation;
    args.bytes = current_args;
    script.bytes = active_cmdscript;
    pz_fighter_attack(script.command->animation,
                      args.attack->parameters.puzzle,
                      args.attack->arg3);
}

void _exit_attack_with(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    PlyrPdata* player;

    args.bytes = current_args;
    result.bytes = active_cmdscript;
    player = (PlyrPdata*)plyr_pdata;
    player->script_exit_value_int = args.exit_args->exit_value;
    player->script_exit_args[0] = args.exit_args->exit_arg0;
    player->input_unlock_tick = args.exit_args->input_unlock_tick;
    player->blocking_disable_tick_1 = args.exit_args->blocking_tick;
    player->script_exit_args[1] = args.exit_args->exit_arg1;
    player->script_exit_args[2] = args.exit_args->exit_arg2;
    result.exit->exit = j_exit_6;
    result.exit->state = 2;
}

void _attack_opponent_with(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    void* animation;

    args.bytes = current_args;
    animation = get_animation(args.attack->animation_id);
    script.bytes = active_cmdscript;
    script.command->animation = animation;
    args.bytes = current_args;
    script.bytes = active_cmdscript;
    attack_opponent_with(script.command->animation,
                         args.attack->parameters.standard,
                         args.attack->arg3);
}

void _drone_combo(void) {
}

void _drone_xfer_him(void) {
    reaction_xfer_him_nohit(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _drone_super_combo(void) {
}

/* Soft ceiling: _xfer_camera ~96.59% -- pooled-string relocation labels only. */
void _xfer_camera(void) {
    int function_index;
    int reset_projection;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x23,
               &function_index, &reset_projection);
    xfer_camera(
        (ScriptProcEntryFn)script_callable_function_table[function_index - 1],
        reset_projection);
}

void _camera_setup_for_custom_orbit_to_relative_point(void) {
    ScriptNpcCameraArgs* args;
    ScriptNpcHandle* npc;
    ScriptNpcBody* body;
    Vec movement;
    Vec lookat;

    args = (ScriptNpcCameraArgs*)current_args;
    movement.x = args->movement_x;
    movement.y = args->movement_y;
    movement.z = args->movement_z;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    lookat.z = 0.0f;
    npc = find_npc_by_data(args->npc_id, current_args, args->movement_z);
    body = npc->body;
    if (body != 0) {
        camera_set_movement_focus_obj((MkObj*)body->camera_object);
        camera_set_lookat_focus((MkObj*)body->camera_object);
        camera_set_look_mode(8);
        camera_set_movement_mode(args->movement_mode);
        camera_set_movement_offset(&movement, current_args);
        camera_set_lookat_offset(&lookat, current_args);
        camera_set_initial_speed(args->initial_speed);
        camera_set_final_speed(args->final_speed);
        camera_set_rotation_direction(args->rotation_direction);
        camera_set_travel_time(args->travel_time);
        camera_set_center_of_rotation(&body->camera_object->pos);
        camera_set_radial_movement(1);
        camera_set_custom_camera_movement_flag(1);
    }
}

void _camera_set_position_relative_to_npc(void) {
    ScriptNpcCameraArgs* args;
    ScriptNpcHandle* npc;
    ScriptNpcBody* body;
    Vec movement;
    Vec lookat;

    args = (ScriptNpcCameraArgs*)current_args;
    movement.x = args->movement_x;
    movement.y = args->movement_y;
    movement.z = args->movement_z;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    lookat.z = 0.0f;
    npc = find_npc_by_data(args->npc_id, current_args, args->movement_z);
    body = npc->body;
    if (body != 0) {
        camera_set_movement_focus_obj((MkObj*)body->camera_object);
        camera_set_lookat_focus((MkObj*)body->camera_object);
        camera_set_look_mode(9);
        camera_set_movement_mode(args->position_mode);
        camera_set_movement_offset(&movement, current_args);
        camera_set_lookat_offset(&lookat, current_args);
        camera_set_glitch_flag();
    }
}

void _camera_setup_for_tracking_npc(void) {
    ScriptNpcCameraArgs* args;
    ScriptNpcHandle* npc;
    ScriptNpcBody* body;
    Vec movement;
    Vec lookat;

    args = (ScriptNpcCameraArgs*)current_args;
    movement.x = args->movement_x;
    movement.y = args->movement_y;
    movement.z = args->movement_z;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    lookat.z = 0.0f;
    npc = find_npc_by_data(args->npc_id, current_args, args->movement_z);
    body = npc->body;
    if (body != 0) {
        camera_set_movement_focus_obj((MkObj*)body->camera_object);
        camera_set_lookat_focus((MkObj*)body->camera_object);
        camera_set_look_mode(0);
        camera_set_movement_mode(2);
        camera_set_movement_offset(&movement, current_args);
        camera_set_lookat_offset(&lookat, current_args);
        camera_set_movement_rate(0.1f);
    }
}

void _camera_setup_for_orbiting_npc(void) {
    ScriptNpcCameraArgs* args;
    ScriptNpcHandle* npc;
    ScriptNpcBody* body;
    Vec movement;
    Vec lookat;

    args = (ScriptNpcCameraArgs*)current_args;
    movement.x = 0.0f;
    movement.y = args->movement_z;
    movement.z = 0.0f;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    lookat.z = 0.0f;
    npc = find_npc_by_data(args->npc_id, current_args, 0.0f);
    body = npc->body;
    if (body != 0) {
        camera_set_movement_focus_obj((MkObj*)body->camera_object);
        camera_set_lookat_focus((MkObj*)body->camera_object);
        camera_setup_simple_rotation(args->orbit_direction,
                                     args->orbit_speed);
        camera_set_look_mode(8);
        camera_set_movement_mode(7);
        camera_set_movement_offset(&movement, current_args);
        camera_set_lookat_offset(&lookat, current_args);
        camera_set_movement_rate(0.1f);
    }
}

void _camera_set_lookat_focus_obj(void) {
    camera_set_lookat_focus(
        (MkObj*)((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _camera_set_lookat_offset_obj_rel(void) {
    ScriptArgsRef args;
    Vec offset;

    args.bytes = current_args;
    offset.x = args.raw->slots[0].f;
    offset.y = args.raw->slots[1].f;
    offset.z = args.raw->slots[2].f;
    camera_set_lookat_offset_obj_rel(&offset, current_args);
}

void _camera_set_lookat_offset(void) {
    ScriptArgsRef args;
    Vec offset;

    args.bytes = current_args;
    offset.x = args.raw->slots[0].f;
    offset.y = args.raw->slots[1].f;
    offset.z = args.raw->slots[2].f;
    camera_set_lookat_offset(&offset, current_args);
}

void _camera_set_movement_offset_obj_rel(void) {
    ScriptArgsRef args;
    Vec offset;

    args.bytes = current_args;
    offset.x = args.raw->slots[0].f;
    offset.y = args.raw->slots[1].f;
    offset.z = args.raw->slots[2].f;
    camera_set_movement_offset_obj_rel(&offset, current_args);
}

void _camera_set_movement_offset(void) {
    ScriptArgsRef args;
    Vec offset;

    args.bytes = current_args;
    offset.x = args.raw->slots[0].f;
    offset.y = args.raw->slots[1].f;
    offset.z = args.raw->slots[2].f;
    camera_set_movement_offset(&offset, current_args);
}

void _branch_next_style(void) {
    ScriptArgsRef args;
    ScriptResultRef script;

    advance_my_moveset();
    args.bytes = current_args;
    script.bytes = active_cmdscript;
    script.command->branch_target = args.single_int->value;
    script.bytes = active_cmdscript;
    script.command->exit = j_call_player_script_function;
    script.bytes = active_cmdscript;
    script.command->state = 2;
}

void _true_branch_next_sidekick_style(void) {
    ScriptArgsRef args;
    ScriptResultRef script;

    script.bytes = active_cmdscript;
    if (script.command->result == 0) {
        return;
    }
    args.bytes = current_args;
    script.command->branch_target = args.single_int->value;
    script.bytes = active_cmdscript;
    script.command->exit = j_call_player_script_function;
    script.bytes = active_cmdscript;
    script.command->state = 2;
}

void _true_branch_next_style(void) {
    ScriptArgsRef args;
    ScriptResultRef script;

    script.bytes = active_cmdscript;
    if (script.command->result != 0) {
        advance_my_moveset();
        args.bytes = current_args;
        script.bytes = active_cmdscript;
        script.command->branch_target = args.single_int->value;
        script.bytes = active_cmdscript;
        script.command->exit = j_call_player_script_function;
        script.bytes = active_cmdscript;
        script.command->state = 2;
    }
}

void _was_i_hit_x_times(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    int result;

    args.bytes = current_args;
    result = was_i_hit_x_times(args.single_int->value);
    script.bytes = active_cmdscript;
    script.command->result = result;
}

void _ani_col_abort(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = ani_col_abort(args.raw->slots[1].i, args.raw->slots[4].i, args.raw->slots[6].i, args.raw->slots[0].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[5].f);
}

void _was_button_and_direction(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    int result;

    args.bytes = current_args;
    result = was_button_and_direction(args.two_int->first,
                                      args.two_int->second);
    script.bytes = active_cmdscript;
    script.command->result = result;
}

void _disable_grounding(void) {
    plyr_obj[9] &= 0x7f;
}

void _set_player_hiframe(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_anim_hiframe(current_args, args.raw->slots[0].f);
}

void _set_player_movement_weight(void) {
    ((ScriptAnimPdataView*)plyr_anim_pdata)->weight =
        ((ScriptRawArgs*)current_args)->slots[0].f;
}

void _set_player_step(void) {
    ((ScriptAnimPdataView*)plyr_anim_pdata)->step =
        ((ScriptRawArgs*)current_args)->slots[0].f;
}

void _newani_to_frame_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    newani_to_frame_x(((ScriptCommandView*)active_cmdscript)->animation, args.raw->slots[5].i, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _glitch_to_ani_frame(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    void* animation;

    args.bytes = current_args;
    animation = get_animation(args.animation->animation_id);
    script.bytes = active_cmdscript;
    script.command->animation = animation;
    args.bytes = current_args;
    script.bytes = active_cmdscript;
    glitch_to_ani_frame(script.command->animation, args.animation->flags,
                        args.animation, args.animation->frame);
}

void _blend_to_ani_frame(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    void* animation;

    args.bytes = current_args;
    animation = get_animation(args.animation->animation_id);
    script.bytes = active_cmdscript;
    script.command->animation = animation;
    args.bytes = current_args;
    script.bytes = active_cmdscript;
    blend_to_ani_frame(script.command->animation, args.animation->flags,
                       args.animation, args.animation->frame,
                       args.animation->blend);
}

void _glitch_him_to_ani(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    PlyrPdata* player;
    ScriptOpponentProcLatch* opponent;
    MkProc* proc;
    AnimPdata* pdata;

    args.bytes = current_args;
    script.bytes = active_cmdscript;
    script.command->animation = get_animation(args.raw->slots[0].i);
    player = (PlyrPdata*)plyr_pdata;
    opponent = (ScriptOpponentProcLatch*)player->his_plyr_pdata;
    proc = opponent->proc;
    if (proc != 0 && proc->instance != opponent->proc_instance) {
        proc = 0;
    }
    if (proc != 0) {
        pdata = (AnimPdata*)pdata_of_proc(proc);
        set_anim_script(pdata, (AniData*)script.command->animation,
                        args.raw->slots[1].i);
    }
}

void _glitch_to_ani(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    void* animation;

    args.bytes = current_args;
    animation = get_animation(args.animation->animation_id);
    script.bytes = active_cmdscript;
    script.command->animation = animation;
    script.bytes = active_cmdscript;
    args.bytes = current_args;
    glitch_to_ani(script.command->animation, args.animation->flags);
}

void _enable_grounding(void) {
    ScriptGroundObjView* player;
    MkHdr* object;

    player = (ScriptGroundObjView*)plyr_obj;
    player->flags09 |= 0x80;
    object = player != 0 ? as_mkhdr((MkHdr*)player) : 0;
    update_bone_hierarchy(object);
    player = (ScriptGroundObjView*)plyr_obj;
    object = player != 0 ? as_mkhdr((MkHdr*)player) : 0;
    ground_me(object);
}

void _was_button_pressed(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        was_button_pressed(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _am_i_airborn(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = am_i_airborn();
}

void _am_i_blocking(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = am_i_blocking();
}

void _is_his_chest_to_screen(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_his_chest_to_screen();
}

void _is_my_chest_to_screen(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_my_chest_to_screen();
}

void _am_i_on_the_left2(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        am_i_on_the_left2(((ScriptRawArgs*)current_args)->slots[0].pointer,
                          ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _am_i_on_the_left(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = am_i_on_the_left();
}

void _is_he_flipped(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_he_flipped();
}

void _am_i_flipped_or_turned(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = am_i_flipped_or_turned();
}

void _am_i_flipped(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = am_i_flipped();
}

void _lower_mines_ani_to_point(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    lower_mines_ani_to_point(((ScriptCommandView*)active_cmdscript)->animation, args.raw->slots[4].i, args.raw->slots[8].i, args.raw->slots[9].i, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f);
}

void _launch_n_land_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    launch_n_land_ani(((ScriptCommandView*)active_cmdscript)->animation, args.raw->slots[4].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f);
}

void _attack_to_frame_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    attack_to_frame_x(((ScriptCommandView*)active_cmdscript)->animation, args.raw->slots[5].i, args.raw->slots[6].i, args.raw->slots[7].i, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _two_player_animation_match_attacker(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    two_player_animation_match_attacker(((ScriptCommandView*)active_cmdscript)->animation, current_args, args.raw->slots[1].f);
}

void _two_player_animation_flip(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    two_player_animation_flip(((ScriptCommandView*)active_cmdscript)->animation, current_args, args.raw->slots[1].f);
}

void _two_player_animation(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptCommandView*)active_cmdscript)->animation = get_animation(args.raw->slots[0].i);
    two_player_animation(((ScriptCommandView*)active_cmdscript)->animation, current_args, args.raw->slots[1].f);
}

void _is_fast_getup(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_fast_getup();
}

void _disable_impale_check(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = disable_impale_check();
}

void _blend_to_ani(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    void* animation;

    args.bytes = current_args;
    animation = get_animation(args.animation->animation_id);
    script.bytes = active_cmdscript;
    script.command->animation = animation;
    args.bytes = current_args;
    script.bytes = active_cmdscript;
    blend_to_ani(
        (AniData*)script.command->animation,
        args.animation->flags,
        args.animation->frame);
}

void _exit_react(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    PlyrPdata* player;

    args.bytes = current_args;
    result.bytes = active_cmdscript;
    player = (PlyrPdata*)plyr_pdata;
    player->script_exit_value_int = args.exit_args->exit_value;
    player->script_exit_args[0] = args.exit_args->exit_arg0;
    player->input_unlock_tick = args.exit_args->input_unlock_tick;
    player->blocking_disable_tick_2 = args.exit_args->blocking_tick;
    player->script_exit_args[1] = args.exit_args->exit_arg1;
    player->script_exit_args[2] = args.exit_args->exit_arg2;
    result.exit->exit = j_exit_react;
    result.exit->state = 2;
}

void _exit_6(void) {
    CURRENT_PLAYER_PDATA->script_exit_value_int = CURRENT_EXIT_ARGS->exit_value;
    CURRENT_PLAYER_PDATA->script_exit_args[0] = CURRENT_EXIT_ARGS->exit_arg0;
    CURRENT_PLAYER_PDATA->input_unlock_tick = CURRENT_EXIT_ARGS->input_unlock_tick;
    CURRENT_PLAYER_PDATA->blocking_disable_tick_1 = CURRENT_EXIT_ARGS->blocking_tick;
    CURRENT_PLAYER_PDATA->script_exit_args[1] = CURRENT_EXIT_ARGS->exit_arg1;
    CURRENT_PLAYER_PDATA->script_exit_arg_2 = CURRENT_EXIT_ARGS->exit_arg2;
    ACTIVE_SCRIPT_EXIT->exit = j_exit_6;
    ACTIVE_SCRIPT_EXIT->state = 2;
}

void _exit_float_int(void) {
    ScriptArgsRef args;
    ScriptResultRef script;
    PlyrPdata* player;
    ScriptProcEntryFn* exits;

    args.bytes = current_args;
    script.bytes = active_cmdscript;
    player = (PlyrPdata*)plyr_pdata;
    exits = (ScriptProcEntryFn*)exit_table_340;

    script.exit->exit = exits[args.exit_float_int->exit_index];
    player->summon_position_x = args.exit_float_int->float_value;
    player->script_exit_value_int = args.exit_float_int->int_value;
    script.exit->state = 2;
}

void _script_exit(void) {
    ((ScriptCommandView*)active_cmdscript)->exit =
        ((ScriptEntryFn*)exit_table_340)
            [((ScriptRawArgs*)current_args)->slots[0].i];
    ((ScriptCommandView*)active_cmdscript)->state = 2;
}

void _script_return(void) {
    ((ScriptCommandView*)active_cmdscript)->exit = 0;
    ((ScriptCommandView*)active_cmdscript)->state = 2;
}

void _print_v(void) {
}

void _print_f(void) {
}

void _print_i(void) {
}

void _print_s(void) {
}

void _true_xfer_him(void) {
    if (((ScriptRawResult*)active_cmdscript)->value.i != 0) {
        reaction_xfer_him_nohit(((ScriptRawArgs*)current_args)->slots[0].pointer);
    }
}

/* Soft ceiling: _script_sleep ~99.31% -- pool-label-only objdiff noise; stop. */
void _script_sleep(void) {
    int ticks;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1F,
               &ticks);
    _mkproc_sleep_ticks = (float)ticks * inverse_game_speed;
    aproc->vtbl->sleep();
}

void j_sleep_forever(void) {
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep();
    }
}

void _tag_team_activate_player(void) {
    tag_team_activate_player(((ScriptRawArgs*)current_args)->slots[0].i,
                             ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _load_and_set_refl_on_weapon(void) {
    load_and_set_refl_on_weapon();
}

void _advance_active_moveset(void) {
    advance_active_moveset(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _get_active_moveset_from_pdata(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        get_active_moveset_from_pdata(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _fx_transfer(void) {
    fx_transfer(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _bgnd_force_specularity_off_for_material(void) {
    bgnd_force_specularity_off_for_material(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                            ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _bgnd_sobj_set_texture_kl_values(void) {
    bgnd_sobj_set_texture_kl_values(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                    ((ScriptRawArgs*)current_args)->slots[1].i,
                                    ((ScriptRawArgs*)current_args)->slots[2].i,
                                    ((ScriptRawArgs*)current_args)->slots[3].f,
                                    ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _debug_print_message(void) {
    get_script_string_arg(1);
    debug_print_message();
}

void _fxsys_set(void) {
    fxsys_set(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _fxsys_set_v3(void) {
    fxsys_set_v3(((ScriptRawArgs*)current_args)->slots[0].i,
                 ((ScriptRawArgs*)current_args)->slots[1].f,
                 ((ScriptRawArgs*)current_args)->slots[2].f,
                 ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _fx_bind_render_to_sobj(void) {
    fx_bind_render_to_sobj(((ScriptRawArgs*)current_args)->slots[0].i,
                           ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _fx_bind_emitter_to_obj_bone(void) {
    fx_bind_emitter_to_obj_bone(((ScriptRawArgs*)current_args)->slots[0].i,
                                ((ScriptRawArgs*)current_args)->slots[1].pointer,
                                ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _fx_bind_render_to_obj_bone(void) {
    fx_bind_render_to_obj_bone(((ScriptRawArgs*)current_args)->slots[0].i,
                               ((ScriptRawArgs*)current_args)->slots[1].pointer,
                               ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _fx_disable_ztest(void) {
    fx_disable_ztest(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx_set_render_priority(void) {
    fx_set_render_priority(((ScriptRawArgs*)current_args)->slots[0].i,
                           ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx_hide(void) {
    fx_hide(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx_get_v3(void) {
    fx_get_v3(((ScriptRawArgs*)current_args)->slots[0].i,
              ((ScriptRawArgs*)current_args)->slots[1].i,
              ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _fx_set(void) {
    fx_set(((ScriptRawArgs*)current_args)->slots[0].i,
           ((ScriptRawArgs*)current_args)->slots[1].i,
           ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _fx_set_param_v3(void) {
    fx_set_param_v3(((ScriptRawArgs*)current_args)->slots[0].i,
                    ((ScriptRawArgs*)current_args)->slots[1].i,
                    ((ScriptRawArgs*)current_args)->slots[2].f,
                    ((ScriptRawArgs*)current_args)->slots[3].f,
                    ((ScriptRawArgs*)current_args)->slots[4].f);
}

void _enable_profiling(void) {
    enable_profiling(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _kill_on_y_less_than_field(void) {
    kill_on_y_less_than_field(((ScriptRawArgs*)current_args)->slots[0].i,
                              ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _change_on_y_less_than_field(void) {
    change_on_y_less_than_field(((ScriptRawArgs*)current_args)->slots[0].i,
                                ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _change_on_y_less(void) {
    change_on_y_less(((ScriptRawArgs*)current_args)->slots[0].i,
                     ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _change_on_less(void) {
    change_on_less(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _change_on_greater(void) {
    change_on_greater(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _kill_roundrobin(void) {
    kill_roundrobin(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _kill_percent(void) {
    kill_percent(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _kill_on_greater(void) {
    kill_on_greater(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _udpate_roundrobin(void) {
    udpate_roundrobin(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _update_assign(void) {
    update_assign(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _update_attract(void) {
    update_attract(((ScriptRawArgs*)current_args)->slots[0].i,
                   ((ScriptRawArgs*)current_args)->slots[1].i,
                   ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _update_bounce(void) {
    update_bounce(((ScriptRawArgs*)current_args)->slots[0].i,
                  ((ScriptRawArgs*)current_args)->slots[1].i,
                  ((ScriptRawArgs*)current_args)->slots[2].i,
                  ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_texanim_hold(void) {
    update_texanim_hold(((ScriptRawArgs*)current_args)->slots[0].i,
                        ((ScriptRawArgs*)current_args)->slots[1].i,
                        ((ScriptRawArgs*)current_args)->slots[2].f,
                        ((ScriptRawArgs*)current_args)->slots[3].i,
                        ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _update_texanim(void) {
    update_texanim(((ScriptRawArgs*)current_args)->slots[0].i,
                   ((ScriptRawArgs*)current_args)->slots[1].i,
                   ((ScriptRawArgs*)current_args)->slots[2].f,
                   ((ScriptRawArgs*)current_args)->slots[3].i,
                   ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _update_lerp_color(void) {
    update_lerp_color(((ScriptRawArgs*)current_args)->slots[0].i,
                      ((ScriptRawArgs*)current_args)->slots[1].i,
                      ((ScriptRawArgs*)current_args)->slots[2].f,
                      ((ScriptRawArgs*)current_args)->slots[3].i,
                      ((ScriptRawArgs*)current_args)->slots[4].i,
                      ((ScriptRawArgs*)current_args)->slots[5].i);
}

void _update_fade_alpha2(void) {
    update_fade_alpha2(((ScriptRawArgs*)current_args)->slots[0].i,
                       ((ScriptRawArgs*)current_args)->slots[1].i,
                       ((ScriptRawArgs*)current_args)->slots[2].f,
                       ((ScriptRawArgs*)current_args)->slots[3].f,
                       ((ScriptRawArgs*)current_args)->slots[4].i,
                       ((ScriptRawArgs*)current_args)->slots[5].i);
}

void _update_fade_alpha(void) {
    update_fade_alpha(((ScriptRawArgs*)current_args)->slots[0].i,
                      ((ScriptRawArgs*)current_args)->slots[1].i,
                      ((ScriptRawArgs*)current_args)->slots[2].f,
                      ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_mul_scalar(void) {
    update_mul_scalar(((ScriptRawArgs*)current_args)->slots[0].i,
                      ((ScriptRawArgs*)current_args)->slots[1].f,
                      ((ScriptRawArgs*)current_args)->slots[2].f,
                      ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_wrapbox(void) {
    update_wrapbox(((ScriptRawArgs*)current_args)->slots[0].i,
                   ((ScriptRawArgs*)current_args)->slots[1].f,
                   ((ScriptRawArgs*)current_args)->slots[2].f,
                   ((ScriptRawArgs*)current_args)->slots[3].f,
                   ((ScriptRawArgs*)current_args)->slots[4].f);
}

void _update_add_constant(void) {
    update_add_constant(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _update_add_constant_v3(void) {
    update_add_constant_v3(((ScriptRawArgs*)current_args)->slots[0].i,
                           ((ScriptRawArgs*)current_args)->slots[1].f,
                           ((ScriptRawArgs*)current_args)->slots[2].f,
                           ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_copy(void) {
    update_copy(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _update_add(void) {
    update_add(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _initial_multiply_float(void) {
    initial_multiply_float(((ScriptRawArgs*)current_args)->slots[0].i,
                           ((ScriptRawArgs*)current_args)->slots[1].f,
                           ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _initial_set_float(void) {
    initial_set_float(((ScriptRawArgs*)current_args)->slots[0].i,
                      ((ScriptRawArgs*)current_args)->slots[1].f,
                      ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _initial_add_v3(void) {
    initial_add_v3(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _initial_divert(void) {
    initial_divert(((ScriptRawArgs*)current_args)->slots[0].i,
                   ((ScriptRawArgs*)current_args)->slots[1].f,
                   ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _initial_reflect(void) {
    initial_reflect(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_cycle_emission(void) {
    set_cycle_emission(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_cycle_length(void) {
    set_cycle_length(((ScriptRawArgs*)current_args)->slots[0].f,
                     ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _fx_reset_emit(void) {
    fx_reset_emit(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_pause_emit(void) {
    fx_pause_emit(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_resume_emit(void) {
    fx_resume_emit(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_next_emitter(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        fx_next_emitter(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_reset(void) {
    fx_reset(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_restart_emit(void) {
    fx_restart_emit(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _reset_effect(void) {
    get_script_string_arg(1);
    reset_effect();
}

void _resume_effect(void) {
    resume_effect(get_script_string_arg(1));
}

void _spawn_color(void) {
    spawn_color(((ScriptRawArgs*)current_args)->slots[0].i,
                ((ScriptRawArgs*)current_args)->slots[1].i,
                ((ScriptRawArgs*)current_args)->slots[2].i,
                ((ScriptRawArgs*)current_args)->slots[3].i,
                ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _spawn_random_size(void) {
    spawn_random_size(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_growth_coefficient(void) {
    set_growth_coefficient(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _set_drag_coefficient(void) {
    set_drag_coefficient(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _set_rotation(void) {
    set_rotation(((ScriptRawArgs*)current_args)->slots[0].f, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _kill_at_plane(void) {
    kill_at_plane(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _emit_constant_rate(void) {
    emit_constant_rate();
}

void _emit_roundrobin_mechanism(void) {
    emit_roundrobin_mechanism(((ScriptRawArgs*)current_args)->slots[0].i,
                              ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _emit_value_i(void) {
    emit_value_i(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _emit_value(void) {
    emit_value(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _emit_from_pos_clamp_y(void) {
    emit_from_pos_clamp_y(((ScriptRawArgs*)current_args)->slots[0].i,
                          ((ScriptRawArgs*)current_args)->slots[1].i,
                          ((ScriptRawArgs*)current_args)->slots[2].f,
                          ((ScriptRawArgs*)current_args)->slots[3].f,
                          ((ScriptRawArgs*)current_args)->slots[4].f,
                          ((ScriptRawArgs*)current_args)->slots[5].f,
                          ((ScriptRawArgs*)current_args)->slots[6].f,
                          ((ScriptRawArgs*)current_args)->slots[7].f);
}

void _emit_from_pos(void) {
    emit_from_pos(((ScriptRawArgs*)current_args)->slots[0].i,
                  ((ScriptRawArgs*)current_args)->slots[1].i,
                  ((ScriptRawArgs*)current_args)->slots[2].f,
                  ((ScriptRawArgs*)current_args)->slots[3].f,
                  ((ScriptRawArgs*)current_args)->slots[4].f,
                  ((ScriptRawArgs*)current_args)->slots[5].f,
                  ((ScriptRawArgs*)current_args)->slots[6].f);
}

void _emit_spherical_section(void) {
    emit_spherical_section(((ScriptRawArgs*)current_args)->slots[0].i,
                           ((ScriptRawArgs*)current_args)->slots[1].f,
                           ((ScriptRawArgs*)current_args)->slots[2].f,
                           ((ScriptRawArgs*)current_args)->slots[3].f,
                           ((ScriptRawArgs*)current_args)->slots[4].f,
                           ((ScriptRawArgs*)current_args)->slots[5].f,
                           ((ScriptRawArgs*)current_args)->slots[6].f,
                           ((ScriptRawArgs*)current_args)->slots[7].f);
}

void _emit_spherical_from_boundary(void) {
    emit_spherical_from_boundary(((ScriptRawArgs*)current_args)->slots[0].i,
                                 ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _emit_spherical(void) {
    emit_spherical(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _texture_animation_with_vsize(void) {
    texture_animation_with_vsize(((ScriptRawArgs*)current_args)->slots[0].f,
                                 ((ScriptRawArgs*)current_args)->slots[1].f,
                                 ((ScriptRawArgs*)current_args)->slots[2].i,
                                 ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _texture_animation(void) {
    texture_animation(((ScriptRawArgs*)current_args)->slots[0].f,
                      ((ScriptRawArgs*)current_args)->slots[1].i,
                      ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _emission_duration(void) {
    emission_duration(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _emit_cylindrical(void) {
    emit_cylindrical(((ScriptRawArgs*)current_args)->slots[0].i,
                     ((ScriptRawArgs*)current_args)->slots[1].f,
                     ((ScriptRawArgs*)current_args)->slots[2].f,
                     ((ScriptRawArgs*)current_args)->slots[3].f,
                     ((ScriptRawArgs*)current_args)->slots[4].f,
                     ((ScriptRawArgs*)current_args)->slots[5].f,
                     ((ScriptRawArgs*)current_args)->slots[6].f,
                     ((ScriptRawArgs*)current_args)->slots[7].f);
}

void _emit_cartesian(void) {
    emit_cartesian(((ScriptRawArgs*)current_args)->slots[0].i,
                   ((ScriptRawArgs*)current_args)->slots[1].f,
                   ((ScriptRawArgs*)current_args)->slots[2].f,
                   ((ScriptRawArgs*)current_args)->slots[3].f,
                   ((ScriptRawArgs*)current_args)->slots[4].f,
                   ((ScriptRawArgs*)current_args)->slots[5].f,
                   ((ScriptRawArgs*)current_args)->slots[6].f);
}

void _emit_disc2(void) {
    emit_disc2(((ScriptRawArgs*)current_args)->slots[0].i,
               ((ScriptRawArgs*)current_args)->slots[1].f,
               ((ScriptRawArgs*)current_args)->slots[2].f,
               ((ScriptRawArgs*)current_args)->slots[3].f,
               ((ScriptRawArgs*)current_args)->slots[4].f,
               ((ScriptRawArgs*)current_args)->slots[5].f);
}

void _emit_disc(void) {
    emit_disc(((ScriptRawArgs*)current_args)->slots[0].i,
              ((ScriptRawArgs*)current_args)->slots[1].f,
              ((ScriptRawArgs*)current_args)->slots[2].f,
              ((ScriptRawArgs*)current_args)->slots[3].f,
              ((ScriptRawArgs*)current_args)->slots[4].f);
}

void _emit_cuboid(void) {
    emit_cuboid(((ScriptRawArgs*)current_args)->slots[0].i,
                ((ScriptRawArgs*)current_args)->slots[1].f,
                ((ScriptRawArgs*)current_args)->slots[2].f,
                ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _emit_from_point(void) {
    emit_from_point(((ScriptRawArgs*)current_args)->slots[0].f,
                    ((ScriptRawArgs*)current_args)->slots[1].f,
                    ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _emit_uv(void) {
    emit_uv(((ScriptRawArgs*)current_args)->slots[0].i,
            ((ScriptRawArgs*)current_args)->slots[1].f,
            ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _emit_color(void) {
    emit_color(((ScriptRawArgs*)current_args)->slots[0].i,
               ((ScriptRawArgs*)current_args)->slots[1].i,
               ((ScriptRawArgs*)current_args)->slots[2].i,
               ((ScriptRawArgs*)current_args)->slots[3].i,
               ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _emit_in_range(void) {
    emit_in_range(((ScriptRawArgs*)current_args)->slots[0].i,
                  ((ScriptRawArgs*)current_args)->slots[1].f,
                  ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _fx_by_owner(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        fx_by_owner(get_script_string_arg(1), ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx2(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        fx2(((ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2));
}

void _fx(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = fx(get_script_string_arg(1));
}

void _set_vertex_color(void) {
    set_vertex_color(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_light(void) {
    set_light(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_aspect_ratio(void) {
    set_aspect_ratio(((ScriptRawArgs*)current_args)->slots[0].f, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _set_bounding_radius(void) {
    set_bounding_radius(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _create_y_mirror_effect(void) {
    create_y_mirror_effect(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _z_bias(void) {
    z_bias(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _particle_size(void) {
    particle_size(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _face_y(void) {
    face_y();
}

void _set_decal_plane(void) {
    set_decal_plane(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _create_multiemit_parametric_fx(void) {
    create_multiemit_parametric_fx(((ScriptRawArgs*)current_args)->slots[0].i,
                                   get_script_string_arg(2),
                                   ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _create_parametric_fx(void) {
    create_parametric_fx(((ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2));
}

void _create_multiemit_step_fx(void) {
    create_multiemit_step_fx(((ScriptRawArgs*)current_args)->slots[0].i,
                             get_script_string_arg(2),
                             ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _create_step_fx(void) {
    create_step_fx(((ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2));
}

void _bind_to_bone(void) {
    bind_to_bone(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _create_step_effect(void) {
    create_step_effect(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _parametric_update(void) {
    parametric_update(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _dist_xz_to_xz(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        dist_xz_to_xz(((ScriptRawArgs*)current_args)->slots[0].pointer,
                      ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _v3_to_xz_ang(void) {
    v3_to_xz_ang(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _v3_to_xy_ang(void) {
    v3_to_xy_ang(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _length_v3(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        length_v3(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _rotate_xz(void) {
    rotate_xz(((ScriptRawArgs*)current_args)->slots[0].pointer,
              ((ScriptRawArgs*)current_args)->slots[1].pointer,
              ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _gxMathArcCos(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        gxMathArcCos(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _gxMathSin(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        gxMathSin(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _gxMathCos(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        gxMathCos(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _uv_v3_to_v3_dist(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        uv_v3_to_v3_dist(((ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((ScriptRawArgs*)current_args)->slots[1].pointer,
                         ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _xz_dot_xz(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        xz_dot_xz(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _YXZ_angles_to_quat(void) {
    YXZ_angles_to_quat(((ScriptRawArgs*)current_args)->slots[0].pointer,
                       ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_get_bone_rot_quat(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_get_bone_rot_quat(((ScriptRawArgs*)current_args)->slots[0].pointer,
                              ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _uv_from_angle_y(void) {
    uv_from_angle_y(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _xz_to_y_ang(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        xz_to_y_ang(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _xz_unit_vector(void) {
    xz_unit_vector(((ScriptRawArgs*)current_args)->slots[0].pointer,
                   ((ScriptRawArgs*)current_args)->slots[1].pointer,
                   ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _v3_dot_v3(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        v3_dot_v3(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _zero_v3(void) {
    zero_v3(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _normalize_v3(void) {
    normalize_v3(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _scale_v3(void) {
    scale_v3(((ScriptRawArgs*)current_args)->slots[0].pointer,
             ((ScriptRawArgs*)current_args)->slots[1].pointer,
             ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _v3_add_v3_scaled(void) {
    v3_add_v3_scaled(((ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((ScriptRawArgs*)current_args)->slots[1].pointer,
                     ((ScriptRawArgs*)current_args)->slots[2].pointer,
                     ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _v3_add_v3(void) {
    v3_add_v3(((ScriptRawArgs*)current_args)->slots[0].pointer,
              ((ScriptRawArgs*)current_args)->slots[1].pointer,
              ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _v3_sub_v3(void) {
    v3_sub_v3(((ScriptRawArgs*)current_args)->slots[0].pointer,
              ((ScriptRawArgs*)current_args)->slots[1].pointer,
              ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _v3_x_mat(void) {
    v3_x_mat(((ScriptRawArgs*)current_args)->slots[0].pointer,
             ((ScriptRawArgs*)current_args)->slots[1].pointer,
             ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _mkobj_get_matrix_pos(void) {
    mkobj_get_matrix_pos(((ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _mkobj_get_matrix_right(void) {
    mkobj_get_matrix_right(((ScriptRawArgs*)current_args)->slots[0].pointer,
                           ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _mkobj_get_matrix_at(void) {
    mkobj_get_matrix_at(((ScriptRawArgs*)current_args)->slots[0].pointer,
                        ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _mkobj_get_matrix(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        mkobj_get_matrix(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_in_spin_react(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        plyr_in_spin_react(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _force_calc_bone_world_mat(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        force_calc_bone_world_mat(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_set_sobj_pos(void) {
    obj_set_sobj_pos(((ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((ScriptRawArgs*)current_args)->slots[1].i,
                     ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _get_bone_relative_pos(void) {
    get_bone_relative_pos(((ScriptRawArgs*)current_args)->slots[0].pointer,
                          ((ScriptRawArgs*)current_args)->slots[1].i,
                          ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _get_bone_offset_world_pos(void) {
    get_bone_offset_world_pos(((ScriptRawArgs*)current_args)->slots[0].pointer,
                              ((ScriptRawArgs*)current_args)->slots[1].i,
                              ((ScriptRawArgs*)current_args)->slots[2].pointer,
                              ((ScriptRawArgs*)current_args)->slots[3].pointer);
}

void _get_bone_world_pos(void) {
    get_bone_world_pos(((ScriptRawArgs*)current_args)->slots[0].pointer,
                       ((ScriptRawArgs*)current_args)->slots[1].i,
                       ((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _bone_matcher_child_set_offset(void) {
    bone_matcher_child_set_offset(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _bone_matcher_parent_set_offset(void) {
    bone_matcher_parent_set_offset(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                   ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _start_bone_matcher(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        start_bone_matcher(((ScriptRawArgs*)current_args)->slots[0].pointer,
                           ((ScriptRawArgs*)current_args)->slots[1].pointer,
                           ((ScriptRawArgs*)current_args)->slots[2].i,
                           ((ScriptRawArgs*)current_args)->slots[3].i,
                           ((ScriptRawArgs*)current_args)->slots[4].f);
}

void _obj_get_ang_vel(void) {
    obj_get_ang_vel(((ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_set_ang_vel(void) {
    obj_set_ang_vel(((ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_set_pos_vel(void) {
    obj_set_pos_vel(((ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_get_pos_vel(void) {
    obj_get_pos_vel(((ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_find_sobj_by_id(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_find_sobj_by_id(((ScriptRawArgs*)current_args)->slots[0].pointer,
                            ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_set_light_flag(void) {
    obj_set_light_flag(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_set_ground_colls_y(void) {
    obj_set_ground_colls_y(((ScriptRawArgs*)current_args)->slots[0].pointer,
                           ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _konquest_load_interior_art(void) {
    konquest_load_interior_art();
}

void _konquest_start_nis_anims_load(void) {
    char* second;
    char* first;

    second = get_script_string_arg(2);
    first = get_script_string_arg(1);
    konquest_start_nis_anims_load(first, second);
}

void _konquest_nis_end(void) {
    konquest_nis_end();
}

void _konquest_nis_init(void) {
    konquest_nis_init(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _nis_wait_for_region_load(void) {
    nis_wait_for_region_load();
}

void _wait_for_region_load(void) {
    wait_for_region_load();
}

void _nis_register_participant(void) {
    nis_register_participant(((ScriptRawArgs*)current_args)->slots[0].i,
                             ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _nis_set_wait_override(void) {
    nis_set_wait_override(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _nis_show_cancel_message(void) {
    nis_show_cancel_message();
}

void _nis_end(void) {
    nis_end();
}

void _nis_wait_for_event(void) {
    nis_wait_for_event(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _nis_signal_event(void) {
    nis_signal_event(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _animpdata_ani_to_frame_x_with_flag_check(void) {
    animpdata_ani_to_frame_x_with_flag_check(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                             ((ScriptRawArgs*)current_args)->slots[1].f,
                                             ((ScriptRawArgs*)current_args)->slots[2].i,
                                             ((ScriptRawArgs*)current_args)->slots[3].i);
}

void _mkscripts_set_anim_check_flag(void) {
    mkscripts_set_anim_check_flag(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _mkscripts_destroy_gusher(void) {
    mkscripts_destroy_gusher(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _mkscripts_destroy_fk_bonematcher(void) {
    mkscripts_destroy_fk_bonematcher(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _mkscripts_destroy_bonematcher(void) {
    mkscripts_destroy_bonematcher(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _dkp_check_plyr_state_for_grab(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        dkp_check_plyr_state_for_grab(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _start_prison_grab_proc(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        start_prison_grab_proc(((ScriptRawArgs*)current_args)->slots[0].i,
                               ((ScriptRawArgs*)current_args)->slots[1].i,
                               ((ScriptRawArgs*)current_args)->slots[2].f,
                               ((ScriptRawArgs*)current_args)->slots[3].f);
}

void _done_prison_grab_proc(void) { done_prison_grab_proc(((ScriptRawArgs*)current_args)->slots[0].i); }

void _kill_spear(void) { kill_spear(); }

void _xfer_spearproc_to_retract(void) { xfer_spearproc_to_retract(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _destroy_spearproc_bonematcher(void) { destroy_spearproc_bonematcher(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _fire_sc_spear(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        fire_sc_spear(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                      ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i,
                      ((ScriptRawArgs*)current_args)->slots[4].i, ((ScriptRawArgs*)current_args)->slots[5].i);
}

void _subzero_start_ice_chunks(void) { subzero_start_ice_chunks(((ScriptRawArgs*)current_args)->slots[0].i); }

void _sindel_scream_react_sound_start(void) { sindel_scream_react_sound_start(); }

void _sindel_sonic_sounds(void) { sindel_sonic_sounds(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i); }

void _sindel_sonic_waves(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = sindel_sonic_waves(((ScriptRawArgs*)current_args)->slots[0].f);
}

void _start_raiden_lightning_scroll(void) {
    start_raiden_lightning_scroll(((ScriptRawArgs*)current_args)->slots[0].i,
                                  ((ScriptRawArgs*)current_args)->slots[1].f,
                                  ((ScriptRawArgs*)current_args)->slots[2].f,
                                  ((ScriptRawArgs*)current_args)->slots[3].i,
                                  ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _ft_raiden_summon_lightning_bolt(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        ft_raiden_summon_lightning_bolt(((ScriptRawArgs*)current_args)->slots[0].i,
                                        ((ScriptRawArgs*)current_args)->slots[1].i,
                                        get_script_string_arg(3));
}

void _kill_raiden_summon_lightning_bolt(void) { kill_raiden_summon_lightning_bolt(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _fix_axe_angle(void) { fix_axe_angle(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _ft_mileena_start_veil_ripoff(void) { ft_mileena_start_veil_ripoff(); }

void _fat_goro_fold_arms(void) {
    fat_goro_fold_arms(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                       ((ScriptRawArgs*)current_args)->slots[2].f, ((ScriptRawArgs*)current_args)->slots[3].i);
}

void _fatality_boraicho_get_jug(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_get_jug(((ScriptRawArgs*)current_args)->slots[0].i,
                                  ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _fatality_boraicho_light_fart_torch(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_light_fart_torch(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _fatality_boraicho_get_torch(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_get_torch(((ScriptRawArgs*)current_args)->slots[0].i,
                                    ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _show_baraka_one_blade_only(void) {
    show_baraka_one_blade_only(((ScriptRawArgs*)current_args)->slots[0].i,
                               ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _fatality_ashrah_get_doll(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_ashrah_get_doll(((ScriptRawArgs*)current_args)->slots[0].i,
                                 ((ScriptRawArgs*)current_args)->slots[1].i,
                                 ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _fire_multi_emitter_pfx_via_tbl(void) {
    char* name = get_script_string_arg(1);
    fire_multi_emitter_pfx_via_tbl(
        name, ((ScriptRawArgs*)current_args)->slots[1].pointer,
        ((ScriptRawArgs*)current_args)->slots[2].pointer,
        ((ScriptRawArgs*)current_args)->slots[3].pointer);
}

void _pfxhandle_spawn_at_bid_next_bind_render(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_spawn_at_bid_next_bind_render(((ScriptRawArgs*)current_args)->slots[0].i,
                                                ((ScriptRawArgs*)current_args)->slots[1].pointer,
                                                ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _pfxhandle_bgnd_spawn_at_sobj_id(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_bgnd_spawn_at_sobj_id(get_script_string_arg(1),
                                        ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _pfxhandle_spawn_at_bid(void) {
    char* name = get_script_string_arg(1);
    ((ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_spawn_at_bid(name, ((ScriptRawArgs*)current_args)->slots[1].pointer,
                               ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _pfxhandle_spawn_at_bid_next(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_spawn_at_bid_next(((ScriptRawArgs*)current_args)->slots[0].i,
                                    ((ScriptRawArgs*)current_args)->slots[1].pointer,
                                    ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _pfx_spawn_at_bid(void) {
    char* name = get_script_string_arg(1);
    pfx_spawn_at_bid(name, ((ScriptRawArgs*)current_args)->slots[1].i, ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_throw_away(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        limb_sever_throw_away(((ScriptRawArgs*)current_args)->slots[0].i,
                              ((ScriptRawArgs*)current_args)->slots[1].i,
                              ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_show_z_meat_chunks(void) {
    limb_sever_show_z_meat_chunks(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((ScriptRawArgs*)current_args)->slots[1].i,
                                  ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_show_z_meat_chunks_all(void) { limb_sever_show_z_meat_chunks_all(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _limb_sever_show_z_meat_chunks_all_plyr_num(void) { limb_sever_show_z_meat_chunks_all_plyr_num(((ScriptRawArgs*)current_args)->slots[0].i); }

void _limb_sever_explode_apart_plyr_num(void) {
    limb_sever_explode_apart_plyr_num(((ScriptRawArgs*)current_args)->slots[0].i,
                                      ((ScriptRawArgs*)current_args)->slots[1].f,
                                      ((ScriptRawArgs*)current_args)->slots[2].f,
                                      ((ScriptRawArgs*)current_args)->slots[3].f,
                                      ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _reset_blood_decals(void) { reset_blood_decals(); }

void _attach_gore2_obj(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        attach_gore2_obj(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                         ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i,
                         ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _start_gore2_pebbles(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_gore2_pebbles(
        args.gore2_pebble->object_id,
        args.gore2_pebble->bone,
        args.gore2_pebble->source,
        args.gore2_pebble->decal_owner,
        args.gore2_pebble->velocity,
        args.gore2_pebble->rotation,
        args.gore2_pebble->scale,
        args.gore2_pebble->position_offset,
        args.gore2_pebble->vertical_acceleration,
        args.gore2_pebble->bounce_scale,
        args.gore2_pebble->bounce_count);
}

void _start_bodyslam_bodysplat(void) {
    start_bodyslam_bodysplat(((ScriptRawArgs*)current_args)->slots[0].f,
                             ((ScriptRawArgs*)current_args)->slots[1].f,
                             ((ScriptRawArgs*)current_args)->slots[2].f,
                             ((ScriptRawArgs*)current_args)->slots[3].f,
                             ((ScriptRawArgs*)current_args)->slots[4].f);
}

void _load_cloth_boned_model(void) {
    ScriptArgsRef args;
    const char* name;

    args.bytes = current_args;
    name = get_script_string_arg(1);
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        load_cloth_boned_model(
            name, args.raw->slots[1].i, args.raw->slots[2].i,
            args.raw->slots[3].i, args.raw->slots[4].i,
            args.raw->slots[5].i, args.raw->slots[6].i);
}

void _fatality_explode_victim(void) {
    fatality_explode_victim(((ScriptRawArgs*)current_args)->slots[0].i,
                            ((ScriptRawArgs*)current_args)->slots[1].f,
                            ((ScriptRawArgs*)current_args)->slots[2].f);
}

void _kill_gusher(void) { kill_gusher(((ScriptRawArgs*)current_args)->slots[0].i); }

void _start_sweat_particles_scripts(void) { start_sweat_particles_scripts(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i); }

void _start_blood_particles_scripts(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        start_blood_particles_scripts(((ScriptRawArgs*)current_args)->slots[0].i,
                                      ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _start_sweat_particles(void) {
    start_sweat_particles(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                          ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i);
}

void _start_blood_particles(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        start_blood_particles(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                              ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i);
}

void _mks_spawn_blood_pool_at_bid(void) {
    mks_spawn_blood_pool_at_bid(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                                ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i);
}

void _spawn_blood_pool_at_bid(void) {
    spawn_blood_pool_at_bid(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                            ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _spawn_bld_splat(void) {
    char* name = get_script_string_arg(1);
    spawn_bld_splat(name, ((ScriptRawArgs*)current_args)->slots[1].i, ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _plyr_weapon2_release(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_weapon2_release(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _plyr_weapon_release(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_weapon_release(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _bone_matcher_reset_dest_mat_rot(void) { bone_matcher_reset_dest_mat_rot(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i); }

void _bone_matcher_set_ang_pos(void) {
    bone_matcher_set_ang_pos(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                             ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i,
                             ((ScriptRawArgs*)current_args)->slots[4].i, ((ScriptRawArgs*)current_args)->slots[5].i);
}

void _weapon_bm_ignore(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        weapon_bm_ignore(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _regrab_weapon(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        regrab_weapon(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                      ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i,
                      ((ScriptRawArgs*)current_args)->slots[4].i, ((ScriptRawArgs*)current_args)->slots[5].i,
                      ((ScriptRawArgs*)current_args)->slots[6].i);
}

void _weapon_reflection_show_hide(void) {
    weapon_reflection_show_hide(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                                ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _show_single_weapon(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        show_single_weapon(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _clone_my_weapon(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        clone_my_weapon(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _clone_weapon_to_secondary(void) { clone_weapon_to_secondary(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i); }

void _advance_to_weapon_style(void) { advance_to_weapon_style(((ScriptRawArgs*)current_args)->slots[0].i); }

void _is_weapon_style(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_weapon_style(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _am_i_female(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = am_i_female(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _sobj_set_bounding_sphere_radius(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        sobj_set_bounding_sphere_radius(((ScriptRawArgs*)current_args)->slots[0].pointer,
                                        ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _sobj_get_bounding_sphere_radius(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        sobj_get_bounding_sphere_radius(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _play_his_snd_req(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = play_his_snd_req(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _play_his_random_voice(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = play_his_random_voice(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _obj_unhide_material_by_id(void) {
    obj_unhide_material_by_id(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_hide_material_by_id(void) {
    obj_hide_material_by_id(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _bm_force_fake_child_bid(void) {
    bm_force_fake_child_bid(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _fat_bgnd_char_setup_radius_check(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        fat_bgnd_char_setup_radius_check(
            ((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _set_victim_v3_units_away(void) {
    set_victim_v3_units_away(((ScriptRawArgs*)current_args)->slots[0].f,
                             ((ScriptRawArgs*)current_args)->slots[1].f);
}

void _reset_fake_bone_matcher(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    reset_fake_bone_matcher(
        args.fake_bone_reset->matcher,
        args.fake_bone_reset->parent_offset,
        args.fake_bone_reset->child_offset,
        args.fake_bone_reset->rotation,
        args.fake_bone_reset->bone_index,
        args.fake_bone_reset->object,
        args.fake_bone_reset->blend);
}

void _ft_fake_bone_matcher(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    MkHdr* matcher;

    args.bytes = current_args;
    matcher = ft_fake_bone_matcher(
        args.fake_bone_match->parent,
        args.fake_bone_match->child,
        args.fake_bone_match->bone_index,
        args.fake_bone_match->parent_offset,
        args.fake_bone_match->child_offset,
        args.fake_bone_match->rotation,
        args.fake_bone_match->mode,
        args.fake_bone_match->blend);
    result.bytes = active_cmdscript;
    result.pointer->value = matcher;
}

void _fatality_release_other_player(void) { fatality_release_other_player(); }

void _set_level_fatality_done_flag_state(void) { set_level_fatality_done_flag_state(((ScriptRawArgs*)current_args)->slots[0].i); }

void _limb_sever_destroy_existing_attach_proc(void) {
    limb_sever_destroy_existing_attach_proc(((ScriptRawArgs*)current_args)->slots[0].i,
                                            ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _limb_sever_bone_attach(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    limb_sever_bone_attach(
        args.limb_bone_attach->target_player,
        args.limb_bone_attach->owner_bone,
        args.limb_bone_attach->offset,
        args.limb_bone_attach->rotation,
        args.limb_bone_attach->owner_player,
        args.limb_bone_attach->limb,
        args.limb_bone_attach->target_bone,
        args.limb_bone_attach->include_children);
}

void _limb_sever_pop_head_up(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    MkHdr* head;

    args.bytes = current_args;
    head = limb_sever_pop_head_up(
        args.pop_head->player,
        args.pop_head->x_velocity,
        args.pop_head->y_velocity,
        args.pop_head->z_velocity,
        args.pop_head->angular_velocity);
    result.bytes = active_cmdscript;
    result.pointer->value = head;
}

void _mks_limb_sever(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        mks_limb_sever(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                       ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_find_existing_update_proc(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        limb_sever_find_existing_update_proc(((ScriptRawArgs*)current_args)->slots[0].i,
                                              ((ScriptRawArgs*)current_args)->slots[1].i,
                                              ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_set_motion(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = limb_sever_set_motion(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[4].i, args.raw->slots[5].i, args.raw->slots[7].i, args.raw->slots[9].i, args.raw->slots[10].i, args.raw->slots[3].f, args.raw->slots[6].f, args.raw->slots[8].f);
}

void _limb_sever_update_slide_end_coeff(void) { limb_sever_update_slide_end_coeff(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _set_pdata_anim_step(void) { set_pdata_anim_step(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _puzzle_fighter_scale(void) { puzzle_fighter_scale(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _plyr_turn_on_shadowbox(void) { plyr_turn_on_shadowbox(((ScriptRawArgs*)current_args)->slots[0].i); }

void _plyr_turn_off_shadowbox(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_turn_off_shadowbox(args.raw->slots[0].pointer);
}

void _plyr_turn_on_mirrorguy(void) { plyr_turn_on_mirrorguy(((ScriptRawArgs*)current_args)->slots[0].i); }

void _plyr_turn_off_mirrorguy(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_turn_off_mirrorguy(args.raw->slots[0].pointer);
}

void _animpdata_ani_to_blend_frame(void) { animpdata_ani_to_blend_frame(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_to_end_at1(void) { animpdata_ani_to_end_at1(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _animpdata_ani_to_end(void) { animpdata_ani_to_end(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _animpdata_ani_x_more_frames(void) { animpdata_ani_x_more_frames(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_loop_more_frames(void) { animpdata_ani_loop_more_frames(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_to_frame_x(void) { animpdata_ani_to_frame_x(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _mks_animpdata_set_cur_frame(void) { mks_animpdata_set_cur_frame(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_1_frame(void) { animpdata_ani_1_frame(((ScriptRawArgs*)current_args)->slots[0].pointer); }

void _check_to_register_miss(void) { check_to_register_miss(); }

void _auto_ani_off(void) { auto_ani_off(); }

void _ncs_bgnd_preload_named_model(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    const char* model;

    args.bytes = current_args;
    result.bytes = active_cmdscript;
    model = get_script_string_arg(2);
    get_script_string_arg(1);
    result.integer->value = ncs_bgnd_preload_named_model(
        model, args.raw->slots[2].i, args.raw->slots[3].i,
        args.raw->slots[4].i, args.raw->slots[5].i,
        args.raw->slots[6].i);
}

void _ncs_dkp_camera_konqchar_show_hide_alpha(void) {
    ncs_dkp_camera_konqchar_show_hide_alpha(((ScriptRawArgs*)current_args)->slots[0].i,
                                            ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _ncs_camera_wall_show_hide_alpha(void) {
    ncs_camera_wall_show_hide_alpha(
        ((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _ncs_bgnd_nuke_collision_to_script_interface(void) { ncs_bgnd_nuke_collision_to_script_interface(); }

void _fkbm_obj_face_obj(void) {
    fkbm_obj_face_obj(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                      ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i,
                      ((ScriptRawArgs*)current_args)->slots[4].i);
}

void _obj_grnd_bounce(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_grnd_bounce(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[4].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[5].f);
}

void _start_obj_scalar_proc(void) {
    start_obj_scalar_proc(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                          ((ScriptRawArgs*)current_args)->slots[2].i, ((ScriptRawArgs*)current_args)->slots[3].i);
}

void _obj_match_obj_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_match_obj_pos(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[2].f);
}

void _insert_particle_mkobj(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        insert_particle_mkobj(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _mkobj_pos_pos_dot_normal_xz(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        mkobj_pos_pos_dot_normal_xz(((ScriptRawArgs*)current_args)->slots[0].i,
                                    ((ScriptRawArgs*)current_args)->slots[1].i,
                                    ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _obj_get_bid_for_tid(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        obj_get_bid_for_tid(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_create_sobjs_by_id(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_create_sobjs_by_id(
            ((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _unhide_sobj_by_sobj_id(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        unhide_sobj_by_sobj_id(
            ((ScriptRawArgs*)current_args)->slots[0].pointer,
            ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _hide_sobj_by_sobj_id(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        hide_sobj_by_sobj_id(
            ((ScriptRawArgs*)current_args)->slots[0].pointer,
            ((ScriptRawArgs*)current_args)->slots[1].i);
}

void _sobj_set_priority(void) { sobj_set_priority(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i); }

void _unhide_sobj(void) { unhide_sobj(((ScriptRawArgs*)current_args)->slots[0].i); }

void _hide_sobj(void) { hide_sobj(((ScriptRawArgs*)current_args)->slots[0].i); }

void _unhide_obj(void) { unhide_obj(((ScriptRawArgs*)current_args)->slots[0].i); }

void _hide_obj(void) { hide_obj(((ScriptRawArgs*)current_args)->slots[0].i); }

void _pfx_plyr_bankowner(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        pfx_plyr_bankowner(((ScriptRawArgs*)current_args)->slots[0].i);
}

void _ncs_script_debug_quickie(void) {
    ncs_script_debug_quickie(((ScriptRawArgs*)current_args)->slots[0].i,
                             ((ScriptRawArgs*)current_args)->slots[1].f,
                             ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _ck_rumble_controller(void) {
    ck_rumble_controller(((ScriptRawArgs*)current_args)->slots[0].i, ((ScriptRawArgs*)current_args)->slots[1].i,
                         ((ScriptRawArgs*)current_args)->slots[2].i);
}

void _check_for_green_light(void) { ((ScriptRawResult*)active_cmdscript)->value.i = check_for_green_light(((ScriptRawArgs*)current_args)->slots[0].i); }

void _check_for_red_light(void) { ((ScriptRawResult*)active_cmdscript)->value.i = check_for_red_light(((ScriptRawArgs*)current_args)->slots[0].i); }

void _ck_put_weapon_away(void) { ck_put_weapon_away(((ScriptRawArgs*)current_args)->slots[0].i); }

void _find_obj_by_id(void) { ((ScriptRawResult*)active_cmdscript)->value.pointer = find_obj_by_id(((ScriptRawArgs*)current_args)->slots[0].i); }

void _yinyang_reset_music_index(void) { yinyang_reset_music_index(); }

void _yinyang_play_evil_tune(void) { yinyang_play_evil_tune(); }

void _yinyang_play_good_tune(void) { yinyang_play_good_tune(); }

void _dist_v3_to_v3(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f =
        dist_v3_to_v3(((ScriptRawArgs*)current_args)->slots[0].pointer, ((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _is_point_in_fortress_exclusion_zone(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i =
        is_point_in_fortress_exclusion_zone(((ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _fortress_setup_exclusion_zone(void) {
    fortress_setup_exclusion_zone(((ScriptRawArgs*)current_args)->slots[0].i,
                                  ((ScriptRawArgs*)current_args)->slots[1].f,
                                  ((ScriptRawArgs*)current_args)->slots[2].f,
                                  ((ScriptRawArgs*)current_args)->slots[3].f,
                                  ((ScriptRawArgs*)current_args)->slots[4].f);
}

void _set_evil_swap_status(void) { set_evil_swap_status(((ScriptRawArgs*)current_args)->slots[0].i); }

void _set_evil_condition(void) { set_evil_condition(((ScriptRawArgs*)current_args)->slots[0].i); }

void _bgnd_make_object_transl(void) { bgnd_make_object_transl(((ScriptRawArgs*)current_args)->slots[0].i); }

void _get_string_by_id(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    char* string;

    args.bytes = current_args;
    string = get_string_by_id(args.single_int->value);
    result.bytes = active_cmdscript;
    result.string->value = string;
}

void _ending_show_text(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ending_show_text(args.two_int->first, args.two_int->second);
}

void _ending_show_image(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ending_show_image(args.single_int->value);
}

void _cam_set_intro_cam_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    cam_set_intro_cam_speed(args.single_float->value);
}

void _midpoint_v3(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    midpoint_v3(args.three_vec->out, args.three_vec->first,
                args.three_vec->second);
}

void _sobj_get_world_pos(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    Vec* position;

    args.bytes = current_args;
    position = sobj_get_world_pos(args.sobj->value);
    result.bytes = active_cmdscript;
    result.vector->value = position;
}

void _get_point_on_circle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_point_on_circle(args.circle->center, args.circle->radius,
                        args.circle->angle, args.circle->out);
}

void _cut_player_in_half(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    MkHdr* upper_body;

    args.bytes = current_args;
    upper_body = cut_player_in_half(args.object->object);
    result.bytes = active_cmdscript;
    result.pointer->value = upper_body;
}

void _is_plyr_airborn(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int airborne;

    args.bytes = current_args;
    airborne = is_plyr_airborn(args.airborne->object, args.airborne->player);
    result.bytes = active_cmdscript;
    result.integer->value = airborne;
}

void _get_offset_of_closest_fence_section(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int offset;

    args.bytes = current_args;
    offset = get_offset_of_closest_fence_section(
        args.fence->point, args.fence->sections, args.fence->start_index,
        args.fence->mirrored);
    result.bytes = active_cmdscript;
    result.integer->value = offset;
}

void _find_mkproc_pid(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    MkProc* proc;

    args.bytes = current_args;
    proc = find_mkproc_pid(args.single_int->value);
    result.bytes = active_cmdscript;
    result.proc->value = proc;
}

void _start_fish_attack(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_fish_attack(args.fish_attack->player, args.fish_attack->direction,
                      args.fish_attack->flags);
}

void _hide_sobj_if_camera_is_in_rectangle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hide_sobj_if_camera_is_in_rectangle(
        args.camera_rectangle->object, args.camera_rectangle->center,
        args.camera_rectangle->min_x, args.camera_rectangle->min_z,
        args.camera_rectangle->max_x, args.camera_rectangle->max_z);
}

void _hide_sobj_if_camera_is_in_cylinder(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hide_sobj_if_camera_is_in_cylinder(
        args.camera_cylinder->object, args.camera_cylinder->center,
        args.camera_cylinder->radius, args.camera_cylinder->height);
}

void _turn_off_sobj_if_camera_is_in_rectangle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    turn_off_sobj_if_camera_is_in_rectangle(
        args.camera_rectangle->object, args.camera_rectangle->center,
        args.camera_rectangle->min_x, args.camera_rectangle->min_z,
        args.camera_rectangle->max_x, args.camera_rectangle->max_z);
}

void _turn_off_sobj_if_camera_is_in_cylinder(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    turn_off_sobj_if_camera_is_in_cylinder(
        args.camera_cylinder->object, args.camera_cylinder->center,
        args.camera_cylinder->radius, args.camera_cylinder->height);
}

void _debug_create_axis_indicator(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    debug_create_axis_indicator(args.axis->player, args.axis->axis);
}

void _bgnd_level_fatality_end(void) {
    bgnd_level_fatality_end();
}

void _bgnd_level_fatality_start(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_level_fatality_start(args.single_int->value);
}

void _bgnd_level_transition_end(void) {
    bgnd_level_transition_end();
}

void _bgnd_level_transition_start(void) {
    bgnd_level_transition_start();
}

void _do_lightning_strike(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    do_lightning_strike(args.lightning->owner, args.lightning->position);
}

void _obj_set_sobj_alpha(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_set_sobj_alpha(args.sobj_alpha->object, args.sobj_alpha->sobj_index,
                       args.sobj_alpha->alpha);
}

void _obj_for_all_atomics_set_material_alpha(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_for_all_atomics_set_material_alpha(args.object_int->object,
                                           args.object_int->value);
}

void _get_volume_from_distance(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    float volume;

    args.bytes = current_args;
    volume = get_volume_from_distance(
        args.volume->position, args.volume->far_distance,
        args.volume->near_distance);
    result.bytes = active_cmdscript;
    result.floating->value = volume;
}

void _get_pan_value(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    float pan;

    args.bytes = current_args;
    pan = get_pan_value((const Vec*)args.float_pointer->value);
    result.bytes = active_cmdscript;
    result.floating->value = pan;
}

void _mab_test(void) {
    mab_test();
}

void _player_body_explode(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    player_body_explode(args.body_explode->player,
                        args.body_explode->position,
                        args.body_explode->velocity);
}

void _bgnd_clear_danger_zone_callback(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_clear_danger_zone_callback(args.single_int->value);
}

void _yinyang_set_bad_fish_hide_flag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    yinyang_set_bad_fish_hide_flag(
        args.fish_flag->entries, args.fish_flag->hidden,
        args.fish_flag->count);
}

void _yinyang_set_good_fish_hide_flag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    yinyang_set_good_fish_hide_flag(
        args.fish_flag->entries, args.fish_flag->hidden,
        args.fish_flag->count);
}

void _yinyang_make_fish_jump(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    yinyang_make_fish_jump(args.two_int->first, args.two_int->second);
}

void _destroy_mkobjs_oid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    destroy_mkobjs_oid(args.single_int->value);
}

void _do_yinyang_statue_explosion(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    do_yinyang_statue_explosion(args.object->object);
}

void _obj_add_to_skinned_obj_light_list_with_ambient(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_add_to_skinned_obj_light_list_with_ambient(
        args.light_list->object, args.light_list->light);
}

void _obj_change_to_bgnd_obj_light_list(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_change_to_bgnd_obj_light_list(args.light_list->object,
                                      args.light_list->light);
}

void _obj_change_to_skinned_obj_light_list(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_change_to_skinned_obj_light_list(args.light_list->object,
                                         args.light_list->light);
}

void _yinyang_stop_lensflare(void) {
    yinyang_stop_lensflare();
}

void _yinyang_start_lensflare(void) {
    yinyang_start_lensflare();
}

void _misc_data_set_test_float(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    misc_data_set_test_float(args.single_float->value);
}

void _misc_data_set_test_u32(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    misc_data_set_test_u32(args.single_int->value);
}

void _obj_set_sobj_priority(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_set_sobj_priority(args.sobj_alpha->object,
                          args.sobj_alpha->sobj_index,
                          args.sobj_alpha->alpha);
}

void _sobj_disable_blending(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    sobj_disable_blending(args.sobj->value);
}

void _obj_first_sobj(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    SObj* object;

    args.bytes = current_args;
    object = obj_first_sobj(args.mkobj->object);
    result.bytes = active_cmdscript;
    result.sobj->value = object;
}

void _pebble_turn_culling_off(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_turn_culling_off(args.single_int->value);
}

void _pebble_turn_culling_on(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_turn_culling_on(args.single_int->value);
}

void _pebble_unhide_me(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_unhide_me(args.two_int->first, args.two_int->second);
}

void _pebble_hide_me(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_hide_me(args.two_int->first, args.two_int->second);
}

void _pebble_setup_bounce_props(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_setup_bounce_props(
        args.pebble_bounce->player, args.pebble_bounce->index,
        args.pebble_bounce->velocity, args.pebble_bounce->flags);
}

void _pebble_set_ang_vel(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_set_ang_vel(args.pebble_vec->player, args.pebble_vec->index,
                       args.pebble_vec->value);
}

void _pebble_set_ang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_set_ang(args.pebble_vec->player, args.pebble_vec->index,
                   args.pebble_vec->value);
}

void _pebble_set_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_set_scale(args.pebble_vec->player, args.pebble_vec->index,
                     args.pebble_vec->value);
}

void _pebble_set_vel(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_set_vel(args.pebble_vec->player, args.pebble_vec->index,
                   args.pebble_vec->value);
}

void _pebble_set_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_set_pos(args.pebble_vec->player, args.pebble_vec->index,
                   args.pebble_vec->value);
}

void _destroy_mkobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    destroy_mkobj(args.mkobj->object);
}

void _obj_turn_gravity_off(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_turn_gravity_off(args.mkobj->object);
}

void _obj_set_gravity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_set_gravity(args.mkobj_float->object, args.mkobj_float->value);
}

void _insert_fgnd_mkobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    insert_fgnd_mkobj(args.mkobj->object);
}

void _get_player_number(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int player;

    args.bytes = current_args;
    player = get_player_number(args.mkobj->object);
    result.bytes = active_cmdscript;
    result.integer->value = player;
}

void _load_named_model_for_player(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    char* name;
    MkObj* object;

    args.bytes = current_args;
    name = get_script_string_arg(1);
    object = load_named_model_for_player(
        name, args.load_player_model->player,
        args.load_player_model->object_type,
        args.load_player_model->flags);
    result.bytes = active_cmdscript;
    result.mkobj->value = object;
}

void _load_named_model_from_slot(void) {
    ScriptArgsRef saved;
    ScriptArgsRef current;
    ScriptResultRef result;
    char* name;
    MkObj* object;

    saved.bytes = current_args;
    name = get_script_string_arg(2);
    current.bytes = current_args;
    object = load_named_model_from_slot(
        current.load_slot_model->slot, name, saved.load_slot_model->flags,
        saved.load_slot_model->user_data);
    result.bytes = active_cmdscript;
    result.mkobj->value = object;
}

void _script_fabs(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    float value;

    args.bytes = current_args;
    value = script_fabs(args.single_float->value);
    result.bytes = active_cmdscript;
    result.floating->value = value;
}

void _set_obj_light_flags(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_obj_light_flags(args.object_int->object, args.object_int->value);
}

void _set_obj_ang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_obj_ang(args.mkobj_vec->object, args.mkobj_vec->x,
                args.mkobj_vec->y, args.mkobj_vec->z);
}

void _set_obj_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_obj_pos(args.mkobj_vec->object, args.mkobj_vec->x,
                args.mkobj_vec->y, args.mkobj_vec->z);
}

void _bgnd_unhide_sobj_list(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_unhide_sobj_list(args.single_int->value);
}

void _bgnd_hide_sobj_list(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_hide_sobj_list(args.single_int->value);
}

void _random_percent(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int value;

    args.bytes = current_args;
    value = random_percent(args.single_float->value);
    result.bytes = active_cmdscript;
    result.integer->value = value;
}

void _bgnd_sobj_start_morph(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_start_morph(args.morph->object_id, args.morph->start_shape,
                          args.morph->end_shape, args.morph->ticks);
}

void _delete_screen_obj_oid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    delete_screen_obj_oid(args.single_int->value);
}

void _reset_collision_system(void) {
    reset_collision_system();
}

void _pos_cam_for_current_level(void) {
    pos_cam_for_current_level();
}

void _reset_severed_limbs(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    reset_severed_limbs(args.single_int->value);
}

void _move_plyrs_to_round_start(void) {
    move_plyrs_to_round_start();
}

void _set_far_clip_plane(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_far_clip_plane(args.single_float->value);
}

void _skytemple_player_explode(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    skytemple_player_explode(
        args.skytemple_explode->player, args.skytemple_explode->x,
        args.skytemple_explode->y, args.skytemple_explode->z);
}

void _skytemple_make_scream_sound(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    skytemple_make_scream_sound(args.single_int->value);
}

void _turn_controllers_on(void) {
    turn_controllers_on();
}

void _turn_controllers_off(void) {
    turn_controllers_off();
}

void _bgnd_clear_face_opponent_flags(void) {
    bgnd_clear_face_opponent_flags();
}

void _mab_script_trace_func(void) {
    mab_script_trace_func(get_script_string_arg(1));
}

void _skytemple_arrange_fence_pebbles_around_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    skytemple_arrange_fence_pebbles_around_pos(
        args.pebble_arrange->player, args.pebble_arrange->count,
        args.pebble_arrange->position);
}

void _skytemple_set_fence_pebble_vel(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    skytemple_set_fence_pebble_vel(
        args.pebble_velocity->player, args.pebble_velocity->count,
        args.pebble_velocity->x, args.pebble_velocity->y,
        args.pebble_velocity->z);
}

void _scripted_camera_script_exit(void) {
    scripted_camera_script_exit();
}

void _bgnd_launch_fx_at_sobj_pos(void) {
    ScriptArgsRef args;
    char* name;

    args.bytes = current_args;
    name = get_script_string_arg(1);
    bgnd_launch_fx_at_sobj_pos(name, args.named_sobj_fx->sobj_id,
                               args.named_sobj_fx->y_offset);
}

void _bgnd_get_first_shape_center_for_obstacle_id(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int found;

    args.bytes = current_args;
    found = bgnd_get_first_shape_center_for_obstacle_id(
        args.two_int->first, args.two_int->second);
    result.bytes = active_cmdscript;
    result.integer->value = found;
}

void _misc_data_set_obj_ptr1(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    misc_data_set_obj_ptr1(args.mkobj->object);
}

void _misc_data_get_obj_ptr1(void) {
    ScriptResultRef result;
    MkObj* object;

    object = misc_data_get_obj_ptr1();
    result.bytes = active_cmdscript;
    result.mkobj->value = object;
}

void _misc_data_set_col_obj_id2(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    misc_data_set_col_obj_id2(args.single_int->value);
}

void _misc_data_get_col_obj_id2(void) {
    ScriptResultRef result;
    int object_id;

    object_id = misc_data_get_col_obj_id2();
    result.bytes = active_cmdscript;
    result.integer->value = object_id;
}

void _misc_data_set_col_obj_id(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    misc_data_set_col_obj_id(args.single_int->value);
}

void _misc_data_get_col_obj_id(void) {
    ScriptResultRef result;
    int object_id;

    object_id = misc_data_get_col_obj_id();
    result.bytes = active_cmdscript;
    result.integer->value = object_id;
}

void _bgnd_fetch_obj(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    MkObj* object;

    args.bytes = current_args;
    object = bgnd_fetch_obj(args.single_int->value);
    result.bytes = active_cmdscript;
    result.mkobj->value = object;
}

void _bgnd_swap_textures_tbl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_swap_textures_tbl(args.swap_texture_table->entries,
                           args.swap_texture_table->frame);
}

void _bgnd_swap_textures(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_swap_textures(args.swap_texture->sobj_id,
                       args.swap_texture->material_id,
                       args.swap_texture->frame);
}

void _bgnd_append_texture_to_material_tbl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_append_texture_to_material_tbl(
        args.append_texture_table->entries);
}

void _bgnd_append_texture_to_material(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_append_texture_to_material(
        args.append_texture->sobj_id, args.append_texture->material_id,
        get_script_string_arg(3), args.append_texture->texture_slot);
}

void _bgnd_light_set_color(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_light_set_color(args.light_color->light_id, args.light_color->red,
                         args.light_color->green, args.light_color->blue);
}

void _bgnd_get_float(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    float value;

    args.bytes = current_args;
    value = bgnd_get_float(args.single_int->value);
    result.bytes = active_cmdscript;
    result.floating->value = value;
}

void _bgnd_get_u32(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    unsigned int value;

    args.bytes = current_args;
    value = bgnd_get_u32(args.single_int->value);
    result.bytes = active_cmdscript;
    result.integer->value = value;
}

void _bgnd_get_int(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int value;

    args.bytes = current_args;
    value = bgnd_get_int(args.single_int->value);
    result.bytes = active_cmdscript;
    result.integer->value = value;
}

void _mks_start_fatality_iceball(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_start_fatality_iceball(args.single_int->value);
}

void _send_speedup_msg(void) {
}

void _mks_debug_display_cloth_ontop(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_debug_display_cloth_ontop(args.single_int->value);
}

void _start_bow(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_bow(args.int_float->integer, args.int_float->floating);
}

void _get_bid_with_flip(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int bone_id;

    args.bytes = current_args;
    bone_id = get_bid_with_flip(args.object_int->object,
                                args.object_int->value);
    result.bytes = active_cmdscript;
    result.integer->value = bone_id;
}

void _start_plyr_attack(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_plyr_attack(args.single_float->value);
}

void _set_active_projectile_upward_attack(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_upward_attack(args.vec_pointer->vector);
}

void _set_active_add_ang_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_add_ang_y(args.single_float->value);
}

void _set_active_projectile_random_rot(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_random_rot(
        args.vec_value->x, args.vec_value->y, args.vec_value->z);
}

void _set_active_projectile_dn_sound(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_dn_sound(args.single_int->value);
}

void _set_active_projectile_sound(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_sound(args.three_int->first, args.three_int->second,
                                args.three_int->third);
}

void _set_active_projectile_random_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_random_pos(
        args.vec_value->x, args.vec_value->y, args.vec_value->z);
}

void _set_active_projectile_velocity_damp(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_velocity_damp(args.vec_pointer->vector);
}

void _set_active_projectile_target_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_target_pos(args.vec_pointer->vector);
}

void _set_active_projectile_max_ticks(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_max_ticks(args.single_int->value);
}

void _set_active_projectile_p_handler(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_p_handler(args.entry->entry);
}

void _set_active_projectile_target_ground(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_target_ground(
        args.vec_value->x, args.vec_value->y, args.vec_value->z);
}

void _set_active_projectile_velocity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_velocity(args.vec_pointer->vector);
}

void _active_projectile_setup_done(void) {
    active_projectile_setup_done();
}

void _set_active_projectile_hit_gnd_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_hit_gnd_script(args.entry->entry);
}

void _set_active_projectile_end_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_end_script(args.entry->entry);
}

void _set_active_projectile_hit_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_hit_script(args.entry->entry);
}

void _set_active_projectile_impale_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_impale_info(
        (ProjectileImpaleInfo*)args.two_pointer->first,
        (const int*)args.two_pointer->second);
}

void _set_active_projectile_not_duckable(void) {
    set_active_projectile_not_duckable();
}

void _set_active_projectile_rx_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_rx_info(
        args.reaction->reaction, args.reaction->rate,
        args.reaction->strength);
}

void _start_projectile_from_plyr_bone(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    MkObj* projectile;

    args.bytes = current_args;
    projectile = start_projectile_from_plyr_bone(
        args.start_projectile->bone_id,
        args.start_projectile->existing_object,
        get_script_string_arg(3), args.start_projectile->speed,
        args.start_projectile->tolerance,
        args.start_projectile->bone_offset);
    result.bytes = active_cmdscript;
    result.mkobj->value = projectile;
}

void _run_reaction_cleanup_function(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    run_reaction_cleanup_function(args.plyr_pdata->player);
}

void _reaction_xfer_him(void) {
    ScriptArgsRef args;
    ScriptResultRef result;
    int transferred;

    args.bytes = current_args;
    transferred = reaction_xfer_him(
        args.reaction->reaction, args.reaction->rate,
        args.reaction->strength);
    result.bytes = active_cmdscript;
    result.integer->value = transferred;
}

void _mks_set_cb1_wind_normal(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_set_cb1_wind_normal(
        args.vec_value->x, args.vec_value->y, args.vec_value->z);
}

void _mks_bgnd_start_wind(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_bgnd_start_wind(args.vec_value->x, args.vec_value->y,
                        args.vec_value->z);
}

void _mks_npc_build_bones_tbl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_build_bones_tbl(
        args.raw->slots[0].i, args.raw->slots[1].pointer);
}

void _mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
        args.plyr_pdata->player);
}

void _mks_xfer_collision_info_plyr_to_bgnd_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_xfer_collision_info_plyr_to_bgnd_script(
        args.plyr_entry->player, args.plyr_entry->entry);
}

void _mks_xfer_collision_info_plyr_to_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_xfer_collision_info_plyr_to_script(
        args.entry_player->entry, args.entry_player->player);
}

void _resume_effect_at_plyr_num_bid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    resume_effect_at_plyr_num_bid(
        args.resume_effect->player, args.resume_effect->bone_id,
        args.resume_effect->effect_handle, args.resume_effect->bind_mode,
        args.resume_effect->blood_required);
}

void _resume_effect_at_obj_bid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    resume_effect_at_obj_bid(
        args.raw->slots[0].pointer, args.raw->slots[1].i,
        args.raw->slots[2].u, args.raw->slots[3].i,
        args.raw->slots[4].i);
}

void _mks_start_gusher(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        mks_start_gusher(
            args.raw->slots[0].i, args.raw->slots[1].i, current_args,
            args.raw->slots[2].f, args.raw->slots[3].f,
            args.raw->slots[4].f, args.raw->slots[5].f,
            args.raw->slots[6].f, args.raw->slots[7].f);
}

void _mks_victim_bleed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_victim_bleed(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_plyr_stop(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_plyr_stop(args.raw->slots[0].i);
}

void _mks_set_plyr_to_center_ang_offset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_set_plyr_to_center_ang_offset(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mks_bgnd_cam_offset_away(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_bgnd_cam_offset_away(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _mks_bgnd_pfx_bind_to_sobj(void) {
    mks_bgnd_pfx_bind_to_sobj(
        get_script_string_arg(1),
        ((ScriptRawArgs*)current_args)->slots[1].u);
}

void _mks_set_update_delay(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_set_update_delay(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_removehide_by_group(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_removehide_by_group(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_blend_start_update_by_group(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_blend_start_update_by_group(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_shadow_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_shadow_scale(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _mks_gravity_update_by_group(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_gravity_update_by_group(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _mks_away_vel_update_by_group(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_away_vel_update_by_group(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _mks_set_rotate_update_by_group(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_set_rotate_update_by_group(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[5].i, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _mks_set_sin_update_by_group(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_set_sin_update_by_group(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[9].i, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f);
}

void _bgnd_init_timers(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_init_timers(args.raw->slots[0].i);
}

void _bgnd_obj_insert_obj_ctrl_section(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_obj_insert_obj_ctrl_section(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_insert_obj_ctrl_section(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_insert_obj_ctrl_section(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _destroy_sobj_ctrl_proc(void) {
    destroy_sobj_ctrl_proc();
}

void _start_sobj_ctrl_proc(void) {
    start_sobj_ctrl_proc();
}

void _set_active_projectile_collision_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_collision_info(args.raw->slots[1].i, current_args, args.raw->slots[0].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _set_active_projectile_tracking_light(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = set_active_projectile_tracking_light(args.raw->slots[0].i);
}

void _set_active_projectile_continue_thru_hit(void) {
    set_active_projectile_continue_thru_hit();
}

void _set_active_projectile_3d_track(void) {
    set_active_projectile_3d_track();
}

void _set_active_projectile_2d_track(void) {
    set_active_projectile_2d_track();
}

void _set_active_projectile_velocity_to_hit_gnd(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_velocity_to_hit_gnd(current_args, args.raw->slots[0].f);
}

void _get_projectile_script_velocity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_projectile_script_velocity(args.raw->slots[0].i);
}

void _get_projectile_script_last_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_projectile_script_last_pos(args.raw->slots[0].i);
}

void _get_projectile_script_plyr_pdata(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_projectile_script_plyr_pdata();
}

void _get_projectile_his_plyr_num(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_projectile_his_plyr_num();
}

void _get_projectile_script_plyr_num(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_projectile_script_plyr_num();
}

void _mks_get_victim_to_tr_dot(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = mks_get_victim_to_tr_dot(args.raw->slots[0].i);
}

void _ani_x_more_frames(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_x_more_frames(current_args, args.raw->slots[0].f);
}

void _bgnd_npc_set_ani_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_ani_speed(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mks_npc_disable_ground_y_all_cloth_bones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_disable_ground_y_all_cloth_bones(args.raw->slots[0].i);
}

void _mks_npc_set_ground_y_all_cloth_bones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_set_ground_y_all_cloth_bones(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mks_npc_cb1_eq_cloth_bone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_cb1_eq_cloth_bone(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_npc_cc1_eq_insert_cloth_coll(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_cc1_eq_insert_cloth_coll(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _mks_npc_set_target(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_set_target(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _mks_npc_cloth_bones_init_by_tbl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_cloth_bones_init_by_tbl(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _mks_npc_start_cloth_bones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_npc_start_cloth_bones(args.raw->slots[0].i);
}

void _bgnd_attach_rope_to_bgnd_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_attach_rope_to_bgnd_obj(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_detach_rope(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_detach_rope(args.raw->slots[0].i);
}

void _start_rope_proc(void) {
    start_rope_proc();
}

void _bgnd_preload_obj_attach_rope(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_preload_obj_attach_rope(args.raw->slots[0].i);
}

void _bgnd_hide_preload_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_hide_preload_obj(args.raw->slots[0].i);
}

void _bgnd_unhide_preload_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_unhide_preload_obj(args.raw->slots[0].i);
}

void _load_bgnd_style(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    load_bgnd_style(args.raw->slots[0].i, get_script_string_arg(2), current_args);
}

void _mks_set_cb1_target_bone_cb2(void) {
    mks_set_cb1_target_bone_cb2();
}

void _mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(args.raw->slots[0].i, args.raw->slots[2].i, args.raw->slots[4].i, args.raw->slots[6].i, current_args, args.raw->slots[1].f, args.raw->slots[3].f, args.raw->slots[5].f, args.raw->slots[7].f);
}

void _mks_debug_display_cloth_coll_plane(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_debug_display_cloth_coll_plane(current_args, args.raw->slots[0].f);
}

void _mks_debug_display_cloth_coll_cyl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_debug_display_cloth_coll_cyl(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl(void) {
    mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl();
}

void _collision_result_dont_care(void) {
    collision_result_dont_care();
}

void _clear_collision_result(void) {
    clear_collision_result();
}

void _wait_for_collision_result(void) {
}

void _reaction_sync_advance(void) {
}

void _plyr_set_gravity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_set_gravity(current_args, args.raw->slots[0].f);
}

void _plyr_scale_pos_vel(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_scale_pos_vel(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _animpdata_get_anim_hiframe(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f =
        animpdata_get_anim_hiframe(args.raw->slots[0].pointer);
}

void _plyr_get_anim_hiframe(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = plyr_get_anim_hiframe();
}

void _plyr_get_anim_frame(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = plyr_get_anim_frame();
}

void _plyr_set_vel_xz_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_set_vel_xz_y(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _player_area_collision_check(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = player_area_collision_check(args.raw->slots[2].i, args.raw->slots[4].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[3].f);
}

void _single_frame_collision_check(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = single_frame_collision_check(args.raw->slots[0].i, args.raw->slots[3].i, args.raw->slots[4].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[5].f);
}

void _is_he_airborn(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_he_airborn();
}

void _auto_ani_on(void) {
    auto_ani_on();
}

void _mks_obj_enable_update_cloth(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_obj_enable_update_cloth(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_bgnd_obj_enable_cloth_update(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_bgnd_obj_enable_cloth_update(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _start_special_weapon_monitor(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_special_weapon_monitor(args.raw->slots[0].i);
}

void _xz_distance_between_players(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = xz_distance_between_players();
}

void _start_scorpion_teleport_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_scorpion_teleport_scale(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _blast_effect_at_plyr(void) {
    blast_effect_at_plyr();
}

void _configure_iceball(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    configure_iceball(args.raw->slots[0].i);
}

void _scorpion_teleport_position(void) {
    scorpion_teleport_position();
}

void _kenshi_teleport_position(void) {
    kenshi_teleport_position();
}

void _start_shadow_watcher(void) {
    start_shadow_watcher();
}

void _update_my_last_switch(void) {
    update_my_last_switch();
}

void _local_collision_allowed_plyr_pdata(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = local_collision_allowed_plyr_pdata();
}

void _kill_plyr_life(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    kill_plyr_life(args.raw->slots[0].i);
}

void _is_local_plyr(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_local_plyr();
}

void _is_reaction_xfer_him_allowed(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_reaction_xfer_him_allowed();
}

void _throw_spear(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = throw_spear();
}

void _pan_vol_pitch_snd_req(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pan_vol_pitch_snd_req(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _force_ai_style(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    force_ai_style(args.raw->slots[0].i);
}

void _xfer_player_proc_to_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    xfer_player_proc_to_script(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _start_subzero_decoy(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_subzero_decoy(current_args, args.raw->slots[0].f);
}

void _jmt_debug_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jmt_debug_script(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _can_fallingcliff_fall(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = can_fallingcliff_fall();
}

void _get_online_evil_state(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = 0;
}

void _send_yinyang_evil_msg(void) {
}

void _set_remote_plyr_online_pos(void) {
}

void _enable_no_sync_anim_f(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    enable_no_sync_anim_f(args.raw->slots[0].i);
}

void _enable_no_adjustment_f(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    enable_no_adjustment_f(args.raw->slots[0].i);
}

void _get_exec_tick_ctr(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_exec_tick_ctr();
}

void _set_cliff_watcher_round(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_cliff_watcher_round(args.raw->slots[0].i);
}

void _get_cliff_watcher_round(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_cliff_watcher_round();
}

void _clear_cliff_data(void) {
    clear_cliff_data();
}

void _start_cliff_watcher(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_cliff_watcher(current_args, args.raw->slots[0].f);
}

void _get_cliff_data(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_cliff_data();
}

void _destroy_kabal_smoke(void) {
    destroy_kabal_smoke();
}

void _start_kabal_smoke(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_kabal_smoke(current_args, args.raw->slots[0].f);
}

void _kobra_teleport_position(void) {
    kobra_teleport_position();
}

void _mileena_sky_set_position(void) {
    mileena_sky_set_position();
}

void _disable_mileena_collisions(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    disable_mileena_collisions(args.raw->slots[0].i);
}

void _switch_plyr_positions(void) {
    switch_plyr_positions();
}

void _kabal_collision_control_victim(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    kabal_collision_control_victim(args.raw->slots[0].i);
}

void _gusher_destroy_list(void) {
    gusher_destroy_list();
}

void _cloth_change_ground_plane_for(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    cloth_change_ground_plane_for(current_args, args.raw->slots[0].f);
}

void _set_constrain_last_pos_pdata(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_constrain_last_pos_pdata(args.raw->slots[0].pointer);
}

void _get_current_bgnd(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_current_bgnd();
}

void _damage_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    damage_player(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _flying_collision(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    flying_collision(args.raw->slots[0].i, args.raw->slots[3].i, args.raw->slots[4].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f, args.raw->slots[9].f);
}

void _player_area_collision_ticks(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    player_area_collision_ticks(args.raw->slots[2].i, args.raw->slots[4].i, current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[3].f, args.raw->slots[5].f);
}

void _get_adjusted_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f =
        get_adjusted_speed(args.raw->slots[0].f, args.raw->slots[1].f);
}

void _adjust_kabal_position(void) {
    adjust_kabal_position();
}

void _is_drone(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_drone();
}

void _release_both_players(void) {
    release_both_players();
}

void _remove_impaled_projectiles(void) {
    remove_impaled_projectiles();
}

void _online_sync_reset(void) {
    online_sync_reset();
}

void _advance_my_sidekick_from_behind_with_moveset(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = advance_my_sidekick_from_behind_with_moveset();
}

void _kill_ermac_eyes(void) {
    kill_ermac_eyes();
}

void _check_for_online_condition(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i =
        check_for_online_condition(args.raw->slots[0].pointer);
}

void _increment_taunts_performed(void) {
    increment_taunts_performed();
}

void _get_taunts_performed(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_taunts_performed();
}

void _mk_chess_fetch_bp_num_based_on_pchr_num(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_bp_num_based_on_pchr_num(args.raw->slots[0].i);
}

void _bgnd_fx_get_binded_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_fx_get_binded_obj(args.raw->slots[0].i);
}

void _bgnd_enable_obj_pos_and_ang_setting(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_enable_obj_pos_and_ang_setting(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _mk_chess_fetch_active_defined_team(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_active_defined_team();
}

void _mk_chess_fetch_active_defined_teams_class(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_active_defined_teams_class(args.raw->slots[0].i);
}

void _mk_chess_air_move(void) {
    mk_chess_air_move();
}

void _mk_chess_active_piece_near_edge(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_active_piece_near_edge();
}

void _courtyard_start_lensflare(void) {
    courtyard_start_lensflare();
}

void _mk_chess_queue_up_piece_event(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_queue_up_piece_event(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mk_chess_piece_set_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_piece_set_state(args.raw->slots[0].i);
}

void _mk_chess_shifter_switch(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_shifter_switch(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _mk_chess_blend_to_my_cell_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_blend_to_my_cell_pos(current_args, args.raw->slots[0].f);
}

void _mk_chess_snap_to_my_cell_now(void) {
    mk_chess_snap_to_my_cell_now();
}

void _mk_chess_xfer_piece_from_scripts(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_xfer_piece_from_scripts(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _mk_chess_piece_die(void) {
    mk_chess_piece_die();
}

void _mk_chess_launch_special_fx(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_launch_special_fx(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _mk_chess_set_piece_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_piece_info(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mk_chess_launch_fx_at_active_piece_with_offset(void) {
    ScriptArgsRef args;
    const char* effect_name;

    args.bytes = current_args;
    effect_name = get_script_string_arg(1);
    mk_chess_launch_fx_at_active_piece_with_offset(
        effect_name, args.raw->slots[1].f, args.raw->slots[2].f,
        args.raw->slots[3].f);
}

void _mk_chess_spell_rescue_current_target(void) {
    mk_chess_spell_rescue_current_target();
}

void _mk_chess_get_piece_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = mk_chess_get_piece_info(args.raw->slots[0].i);
}

void _mk_chess_piece_temporarily_gone(void) {
    mk_chess_piece_temporarily_gone();
}

void _mk_chess_launch_up(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_launch_up(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _mk_chess_spell_get_target_health(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = mk_chess_spell_get_target_health(args.raw->slots[0].i);
}

void _mk_chess_spell_move_target_from_temp_area_to(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_spell_move_target_from_temp_area_to(args.raw->slots[0].i);
}

void _mk_chess_spell_move_target_to_temp_area(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_spell_move_target_to_temp_area(args.raw->slots[0].i);
}

void _mk_chess_spell_target_add_access_restrictions(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_spell_target_add_access_restrictions(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _mk_chess_spell_has_completed_but_wait_for_fight(void) {
    mk_chess_spell_has_completed_but_wait_for_fight();
}

void _mk_chess_spell_force_fight(void) {
    mk_chess_spell_force_fight();
}

void _mk_chess_spell_is_this_a_forced_fight(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_spell_is_this_a_forced_fight();
}

void _mk_chess_spell_kill_target(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_spell_kill_target(args.raw->slots[0].i);
}

void _mk_chess_spell_move_target_to_target(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_spell_move_target_to_target(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mk_chess_spell_set_target_health(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_spell_set_target_health(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mk_chess_spell_get_target_max_health(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = mk_chess_spell_get_target_max_health(args.raw->slots[0].i);
}

void _mk_chess_spell_show_target_portrait(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_spell_show_target_portrait(args.raw->slots[0].i);
}

void _mk_chess_spell_has_completed(void) {
    mk_chess_spell_has_completed();
}

void _mk_chess_make_spellcaster(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_make_spellcaster(args.raw->slots[0].i);
}

void _mk_chess_set_piece_event_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_piece_event_script(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mk_chess_piece_test_and_set_timer(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_piece_test_and_set_timer(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mk_chess_deactivate_my_properties(void) {
    mk_chess_deactivate_my_properties();
}

void _mk_chess_ani_until_reached_destination(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_ani_until_reached_destination(args.raw->slots[5].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _mk_chess_rotate_towards_cell(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_rotate_towards_cell(args.raw->slots[3].i, current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[4].f);
}

void _mk_chess_request_piece_script_for_action(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_request_piece_script_for_action(args.raw->slots[0].i);
}

void _mk_chess_piece_is_idle(void) {
    mk_chess_piece_is_idle();
}

void _mk_chess_put_active_piece_at_cell(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_put_active_piece_at_cell(args.raw->slots[2].i, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _mk_chess_launch_n_land_ani_with_xz(void) {
    ScriptArgsRef args;
    float spC;
    float sp8;

    args.bytes = current_args;
    sp8 = args.raw->slots[9].f;
    spC = args.raw->slots[10].f;
    mk_chess_launch_n_land_ani_with_xz(args.raw->slots[0].i, args.raw->slots[11].i, args.raw->slots[12].i, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f);
}

void _mk_chess_stop_me(void) {
    mk_chess_stop_me();
}

void _mk_chess_get_piece_event_data(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = mk_chess_get_piece_event_data(args.raw->slots[0].i);
}

void _bgnd_active_sobj_no_ztest(void) {
    bgnd_active_sobj_no_ztest();
}

void _mk_chess_set_cell_offset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_cell_offset(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _mk_chess_force_away(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_force_away(args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[0].f, args.raw->slots[2].f);
}

void _mk_chess_add_movement_skill(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_add_movement_skill(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _mk_chess_define_class_initial_power(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_define_class_initial_power(current_args, args.raw->slots[0].f);
}

void _mk_chess_activate_my_properties(void) {
    mk_chess_activate_my_properties();
}

void _mk_chess_wait_until_attack_cam_closes_in(void) {
    mk_chess_wait_until_attack_cam_closes_in();
}

void _mk_chess_snd_request(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_snd_request(args.raw->slots[0].i);
}

void _mk_chess_dont_constrain_piece(void) {
    mk_chess_dont_constrain_piece();
}

void _mk_chess_piece_match_y_ang_to_anim(void) {
    mk_chess_piece_match_y_ang_to_anim();
}

void _mk_chess_check_glitch_into_stance(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_check_glitch_into_stance();
}

void _mk_chess_set_glitch_stance_flag(void) {
    mk_chess_set_glitch_stance_flag();
}

void _mk_chess_glitch_to_ani_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_glitch_to_ani_frame(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _mk_chess_set_ani_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_ani_speed(current_args, args.raw->slots[0].f);
}

void _bgnd_add_fx_to_hide(void) {
    get_script_string_arg(1);
    bgnd_add_fx_to_hide();
}

void _mk_chess_place_special_cell_at(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_place_special_cell_at(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[7].i, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f);
}

void _mk_chess_piece_event_from_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_piece_event_from_script(args.raw->slots[0].i);
}

void _mk_chess_blend_to_desired_cell_position_setting(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_blend_to_desired_cell_position_setting(current_args, args.raw->slots[0].f);
}

void _mk_chess_blend_into_cell_orgin_in_x_frames_by_caller(void) {
    mk_chess_blend_into_cell_orgin_in_x_frames_by_caller();
}

void _mk_chess_set_obj_move_weight(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_obj_move_weight(current_args, args.raw->slots[0].f);
}

void _mk_chess_snap_into_cell_orgin_over_x_frames(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_snap_into_cell_orgin_over_x_frames(current_args, args.raw->slots[0].f);
}

void _mk_chess_check_snap_into_stance(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = mk_chess_check_snap_into_stance();
}

void _mk_chess_snap_to_stance(void) {
    mk_chess_snap_to_stance();
}

void _mk_chess_blend_to_ani_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_blend_to_ani_frame(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _mk_chess_blend_to_normal_stance(void) {
    mk_chess_blend_to_normal_stance();
}

void _mk_chess_set_normal_stance_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_normal_stance_script(args.raw->slots[0].i);
}

void _bgnd_kill_fx(void) {
    get_script_string_arg(1);
    bgnd_kill_fx();
}

void _bgnd_no_z_test(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_no_z_test(args.raw->slots[0].i);
}

void _bgnd_get_active_sobj_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_get_active_sobj_pos(args.raw->slots[0].i);
}

void _mk_chess_set_piece_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_piece_state(args.raw->slots[0].i);
}

void _mk_chess_set_piece_type_as(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_set_piece_type_as(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mk_chess_blend_into_cell_orgin_in_x_frames(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_blend_into_cell_orgin_in_x_frames(current_args, args.raw->slots[0].f);
}

void _mk_chess_ani_loop_more_frames(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_ani_loop_more_frames(current_args, args.raw->slots[0].f);
}

void _mk_chess_ani_idle(void) {
    mk_chess_ani_idle();
}

void _mk_chess_ani_to_blend_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_ani_to_blend_frame(current_args, args.raw->slots[0].f);
}

void _mk_chess_ani_to_end(void) {
    mk_chess_ani_to_end();
}

void _mk_chess_ani_1_frame(void) {
    mk_chess_ani_1_frame();
}

void _mk_chess_ani_to_frame_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_ani_to_frame_x(current_args, args.raw->slots[0].f);
}

void _mk_chess_blend_to_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_blend_to_ani(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _mk_chess_init_piece(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_init_piece(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mk_chess_load_chess_table(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mk_chess_load_chess_table(args.raw->slots[0].pointer);
}

void _bgnd_enable_wall_hider(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_enable_wall_hider(args.raw->slots[0].i);
}

void _bgnd_npc_set_pos_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_pos_y(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_npc_set_pos_vel_heading(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_pos_vel_heading(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_npc_set_pos_vel(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_pos_vel(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_npc_get_ang_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_npc_get_ang_y(args.raw->slots[0].i);
}

void _bgnd_npc_adjust_y_ang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_adjust_y_ang(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_npc_get_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_get_pos(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_npc_get_aux_int_data(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_npc_get_aux_int_data(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_npc_start_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_start_ani(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, current_args, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _bgnd_npc_set_aux_int_data(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_aux_int_data(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_add_scripted_brains_to_npc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_add_scripted_brains_to_npc(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_npc_set_y_ang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_y_ang(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_npc_like_plyr(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_like_plyr(args.raw->slots[0].i);
}

void _bgnd_npc_set_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_scale(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_create_named_npc_in_slot(void) {
    ScriptArgsRef current;
    ScriptArgsRef saved;
    const char* name;

    saved.bytes = current_args;
    name = get_script_string_arg(2);
    current.bytes = current_args;
    bgnd_create_named_npc_in_slot(
        current.raw->slots[0].i, name, saved.raw->slots[2].i,
        saved.raw->slots[3].i);
}

void _bgnd_npc_set_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_npc_set_pos(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _danger_zone_eligible_on(void) {
    danger_zone_eligible_on();
}

void _bgnd_get_exec_tick_ctr(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_get_exec_tick_ctr();
}

void _start_sobj_launch_monitor(void) {
    start_sobj_launch_monitor();
}

void _start_chunk_launch_monitor(void) {
    start_chunk_launch_monitor();
}

void _snd_req_delay(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    snd_req_delay(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_set_material_color(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_material_color(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[5].i);
}

void _bgnd_set_fx_ang_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_fx_ang_y(current_args, args.raw->slots[0].f);
}

void _bgnd_launch_plyr_blood_fx(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_launch_plyr_blood_fx(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_sobj_set_priority(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_priority(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_pfxhandle_spawn_at_bid(void) {
    ScriptArgsRef args;
    const char* effect_name;

    args.bytes = current_args;
    effect_name = get_script_string_arg(1);
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        bgnd_pfxhandle_spawn_at_bid(
            effect_name, (MkObj*)args.raw->slots[1].pointer,
            args.raw->slots[2].i);
}

void _bgnd_launch_fx_at_bid_of_mkobj(void) {
    ScriptArgsRef args;
    const char* effect_name;

    args.bytes = current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_bid_of_mkobj(
        effect_name, (MkObj*)args.raw->slots[1].pointer,
        args.raw->slots[2].i);
}

void _bgnd_launch_fx_at_plyr_bid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(1);
    bgnd_launch_fx_at_plyr_bid(args.raw->slots[1].i);
}

void _cam_set_intro_cam_pause_ticks(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    cam_set_intro_cam_pause_ticks(args.raw->slots[0].f);
}

void _bgnd_collision_if_monitor_col_as(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_collision_if_monitor_col_as(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _bgnd_pebble_burst_set_end_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_burst_set_end_state(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _bgnd_pebble_change_current_end_behavior(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_change_current_end_behavior(args.raw->slots[0].i);
}

void _bgnd_pebble_burst_set_value(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_burst_set_value(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, current_args, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f, args.raw->slots[9].f);
}

void _bgnd_pebble_burst_set_value_min_max(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_burst_set_value_min_max(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, current_args, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _bgnd_pebble_change_current_behavior_to_bounce(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_change_current_behavior_to_bounce(args.raw->slots[6].i, args.raw->slots[7].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _bgnd_start_preload_sobj_morph(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_preload_sobj_morph(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _bgnd_start_preload_sobj_uv_scroll(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_preload_sobj_uv_scroll(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _bgnd_launch_fx_at_active_sobj_pos_with_offset(void) {
    ScriptArgsRef args;
    const char* effect_name;

    args.bytes = current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_active_sobj_pos_with_offset(
        effect_name, args.raw->slots[1].f, args.raw->slots[2].f,
        args.raw->slots[3].f);
}

void _bgnd_active_sobj_no_zwrite(void) {
    bgnd_active_sobj_no_zwrite();
}

void _bgnd_set_active_sobj_zoffset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj_zoffset(current_args, args.raw->slots[0].f);
}

void _face_point(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    face_point(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _share_my_attack_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    share_my_attack_info(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _ani_with_pos(void) {
    ani_with_pos();
}

void _ani_no_pos(void) {
    ani_no_pos();
}

void _bgnd_pebble_burst_at_pebble_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_burst_at_pebble_pos(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_pebble_burst_at_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_burst_at_pos(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, current_args, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _nb_place_slave_in_bgnd(void) {
    ScriptPlaceSlaveArgs* args;
    const char* name;

    args = (ScriptPlaceSlaveArgs*)current_args;
    name = get_script_string_arg(3);
    nb_place_slave_in_bgnd(
        args->slave, args->owner, name, args->placement_mode,
        args->values[0], args->values[1], args->values[2], args->values[3],
        args->values[4], args->values[5], args->values[6], args->values[7],
        args->values[8], args->values[9], args->values[10], args->values[11],
        args->values[12]);
}

void _nb_npc_slave_plyr_process_collision(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    nb_npc_slave_plyr_process_collision(args.raw->slots[0].i);
}

void _bgnd_collision_if_enable_col(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_collision_if_enable_col(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_collision_if_disable_col(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_collision_if_disable_col(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_pebble_simple_launch_at_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_simple_launch_at_time(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _bgnd_pebble_set_current_pebble(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_set_current_pebble(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_remove_cracks(void) {
    bgnd_remove_cracks();
}

void _bgnd_pebble_rand_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_rand_scale(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_pebble_burst_at_chunk_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_burst_at_chunk_pos(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_unhide_pebbles(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_unhide_pebbles(args.raw->slots[0].i);
}

void _bgnd_hide_pebbles(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_hide_pebbles(args.raw->slots[0].i);
}

void _spad_set_heading_vector_to(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_set_heading_vector_to(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _degrees_to_rad(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = degrees_to_rad(current_args, args.raw->slots[0].f);
}

void _rad_to_degrees(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = rad_to_degrees(current_args, args.raw->slots[0].f);
}

void _bgnd_pebble_set_current_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_set_current_info(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_pebble_fetch_current_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_pebble_fetch_current_info(args.raw->slots[0].i);
}

void _bgnd_pebble_change_current_behavior(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_change_current_behavior(args.raw->slots[6].i, args.raw->slots[7].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _bgnd_init_pebbles(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_init_pebbles(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_pebble_gravity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pebble_gravity(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _get_sobj_pebble_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = get_sobj_pebble_obj(args.raw->slots[0].i);
}

void _get_general_pebble_data(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = get_general_pebble_data(args.raw->slots[0].i);
}

void _ncs_create_pebble_monitor_proc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = ncs_create_pebble_monitor_proc(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _ncs_set_pebble_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ncs_set_pebble_pos(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _ncs_create_pebbles_with_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = ncs_create_pebbles_with_sobj(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_create_pebbles_with_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_create_pebbles_with_sobj(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _bgnd_create_pebbles(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_create_pebbles(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i);
}

void _bgnd_pebble_launch_at_time(void) {
    ScriptArgsRef args;
    float sp8;

    args.bytes = current_args;
    sp8 = args.raw->slots[10].f;
    bgnd_pebble_launch_at_time(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[11].i, args.raw->slots[12].i, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f, args.raw->slots[9].f);
}

void _bgnd_jtb_debug_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_jtb_debug_info(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _get_game_speed(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = get_game_speed();
}

void _get_soul_sine(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = get_soul_sine(args.raw->slots[0].i);
}

void _bgnd_set_active_sobj_ang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj_ang(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _build_sine_table_for_scripts(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = build_sine_table_for_scripts();
}

void _bgnd_is_active_sobj_hidden(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_is_active_sobj_hidden();
}

void _bgnd_set_fx_ang_dir_to_i_vector(void) {
    bgnd_set_fx_ang_dir_to_i_vector();
}

void _bgnd_set_active_sobj_in_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj_in_obj(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _pfxhandle_bgnd_spawn_at_position(void) {
    ScriptArgsRef args;
    const char* effect_name;

    args.bytes = current_args;
    effect_name = get_script_string_arg(1);
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        pfxhandle_bgnd_spawn_at_position(
            effect_name, args.raw->slots[1].f, args.raw->slots[2].f,
            args.raw->slots[3].f);
}

void _bgnd_launch_fx_to_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(1);
    bgnd_launch_fx_to_sobj(args.raw->slots[1].i);
}

void _bgnd_launch_fx_at_position(void) {
    ScriptArgsRef args;
    const char* effect_name;

    args.bytes = current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_position(
        effect_name, args.raw->slots[1].f, args.raw->slots[2].f,
        args.raw->slots[3].f);
}

void _bgnd_set_sobj_launch_params_exact(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_sobj_launch_params_exact(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f);
}

void _load_script_as_reaction(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    load_script_as_reaction(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_collision_if_rx_override(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_collision_if_rx_override(args.raw->slots[0].i);
}

void _set_background_obstacle_disable_flag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_background_obstacle_disable_flag(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _set_background_obstacle_repel_flag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_background_obstacle_repel_flag(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_collison_if_monitor_col(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_collison_if_monitor_col(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[5].i, args.raw->slots[6].i);
}

void _bgnd_collison_if_set_info(void) {
    bgnd_collison_if_set_info();
}

void _bgnd_collison_if_set_return_result(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_collison_if_set_return_result(args.raw->slots[0].i);
}

void _bgnd_collison_if_to_scripts_activate(void) {
    bgnd_collison_if_to_scripts_activate();
}

void _initialize_background_danger_zones(void) {
    initialize_background_danger_zones();
}

void _spad_xz_cos_two_vectors(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = spad_xz_cos_two_vectors(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_timer_get_tick_count(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_timer_get_tick_count(args.raw->slots[0].i);
}

void _spad_xz_length_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = spad_xz_length_vector(args.raw->slots[0].i);
}

void _bgnd_current_rx_set_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_current_rx_set_info(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_current_rx_get_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_current_rx_get_info(args.raw->slots[0].i);
}

void _bgnd_setup_rx_handler(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_setup_rx_handler(args.raw->slots[0].i);
}

void _bgnd_start_timer(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_timer(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_kill_all_launched_sobjs(void) {
    bgnd_kill_all_launched_sobjs();
}

void _jump_towards_opponent_bgnd_transition(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = jump_towards_opponent_bgnd_transition();
}

void _allow_shard_pfx_now(void) {
    allow_shard_pfx_now();
}

void _kill_shard_pfx_now(void) {
    kill_shard_pfx_now();
}

void _bgnd_set_fx_z_offset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(1);
    bgnd_set_fx_z_offset(current_args, args.raw->slots[1].f);
}

void _bgnd_force_plyr_ground_plane(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_force_plyr_ground_plane(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_set_launch_velocity_based_on_sobj_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_launch_velocity_based_on_sobj_pos(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, current_args, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _bgnd_update_active_mksobj(void) {
    bgnd_update_active_mksobj();
}

void _bgnd_shadow_control(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_shadow_control(args.raw->slots[0].i);
}

void _bgnd_set_plyr_gravity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_plyr_gravity(current_args, args.raw->slots[0].f);
}

void _bgnd_move_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_move_player(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _spad_sub_vectors(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_sub_vectors(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_act_at_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_act_at_time(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _bgnd_xfer_attacker(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_xfer_attacker(args.raw->slots[0].i);
}

void _spad_get_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = spad_get_pos(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _sfrand(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = sfrand(current_args, args.raw->slots[0].f);
}

void _spad_add_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_add_vector(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_launch_fx_at_plyr_pos_and_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(1);
    bgnd_launch_fx_at_plyr_pos_and_y(current_args, args.raw->slots[1].f);
}

void _bgnd_start_cracks(void) {
    bgnd_start_cracks();
}

void _bgnd_init_cracks(void) {
    bgnd_init_cracks();
}

void _bgnd_place_crack_when_plyr_hits_ground(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_place_crack_when_plyr_hits_ground(args.raw->slots[0].i);
}

void _plyr_get_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = plyr_get_pos(args.raw->slots[0].i);
}

void _bgnd_clean_up_floor(void) {
    bgnd_clean_up_floor();
}

void _bgnd_allow_dirty_floor(void) {
    bgnd_allow_dirty_floor();
}

void _cam_recalc_midpoint(void) {
    cam_recalc_midpoint();
}

void _camera_get_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = camera_get_pos(args.raw->slots[0].i);
}

void _camera_set_movement_offset_explicit(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_movement_offset_explicit(args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_force_ground_to(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_force_ground_to(current_args, args.raw->slots[0].f);
}

void _camera_set_lookat_offset_explicit(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_lookat_offset_explicit(args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_run_camera_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_run_camera_script(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _spad_set_y_angle_plus_offset_from_xz_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_set_y_angle_plus_offset_from_xz_vector(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _spad_set_vector_setting(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_set_vector_setting(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_process_active_sobj_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_process_active_sobj_info(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_blood_control(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_blood_control(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _delete_obstacle_from_background_by_id(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    delete_obstacle_from_background_by_id(args.raw->slots[0].i);
}

void _bgnd_make_displayed_item_pickupable_at_active_sobj_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_make_displayed_item_pickupable_at_active_sobj_pos(args.raw->slots[0].i);
}

void _bgnd_reset_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_reset_sobj(args.raw->slots[0].i);
}

void _restart_effect(void) {
    get_script_string_arg(1);
    restart_effect();
}

void _bgnd_set_active_sobj_rop(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj_rop(args.raw->slots[0].i);
}

void _bgnd_set_active_sobj_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj_scale(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_set_active_sobj_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj_pos(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_set_active_sobj_pos_vel(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj_pos_vel(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_apply_active_sobj_pos_vel_drag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_apply_active_sobj_pos_vel_drag(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _int_to_float(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = int_to_float(args.raw->slots[0].i);
}

void _float_to_int(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = float_to_int(current_args, args.raw->slots[0].f);
}

void _bgnd_set_sobj_launch_params(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_sobj_launch_params(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, current_args, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f, args.raw->slots[9].f);
}

void _spad_scale_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_scale_vector(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _spad_set_vector_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_set_vector_y(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_process_collision_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_process_collision_info(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f);
}

void _bgnd_takeover_plyr(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_takeover_plyr(args.raw->slots[0].i);
}

void _spad_norm_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_norm_vector(args.raw->slots[0].i);
}

void _spad_rotate_xz_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_rotate_xz_vector(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _spad_xz_dot_xz(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = spad_xz_dot_xz(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _spad_set_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spad_set_vector(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_launch_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_launch_sobj(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[5].i, args.raw->slots[7].i, args.raw->slots[6].f);
}

void _bgnd_set_collision_plane_for_launched_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_collision_plane_for_launched_sobj(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_set_kill_plane_for_launched_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_kill_plane_for_launched_sobj(args.raw->slots[0].i);
}

void _bgnd_fade_object(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_fade_object(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_get_sobj_ang_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_sobj_ang_y(args.raw->slots[0].i);
}

void _bgnd_rotate_xz_about_orgin_active_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_rotate_xz_about_orgin_active_sobj(current_args, args.raw->slots[0].f);
}

void _bgnd_hide_active_sobj(void) {
    bgnd_hide_active_sobj();
}

void _bgnd_unhide_active_sobj(void) {
    bgnd_unhide_active_sobj();
}

void _bgnd_set_active_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_sobj(args.raw->slots[0].i);
}

void _bgnd_chunk_explosion_match_velocity_with_params(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(4);
    bgnd_chunk_explosion_match_velocity_with_params(
        args.raw->slots[4].i, args.raw->slots[5].i,
        args.raw->slots[6].i, args.raw->slots[7].i,
        args.raw->slots[0].f, args.raw->slots[1].f,
        args.raw->slots[2].f);
}

void _bgnd_restore_player(void) {
    bgnd_restore_player();
}

void _dont_fence_plyr_in(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    dont_fence_plyr_in(args.raw->slots[0].i);
}

void _bgnd_launch_plyr_up_and_forward_running(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_launch_plyr_up_and_forward_running();
}

void _bgnd_launch_plyr_up_and_forward(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_launch_plyr_up_and_forward(args.raw->slots[4].i, args.raw->slots[6].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[5].f);
}

void _bgnd_register_danger_zone_callback(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_register_danger_zone_callback(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_swap_level(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_swap_level(args.raw->slots[0].i);
}

void _bgnd_clean_beetlelair(void) {
    bgnd_clean_beetlelair();
}

void _start_bl_beetles_live_top_floor(void) {
    start_bl_beetles_live_top_floor();
}

void _pz_fighter_allow_easy_continuation(void) {
    pz_fighter_allow_easy_continuation();
}

void _pz_fighter_reset_continuation(void) {
    pz_fighter_reset_continuation();
}

void _pz_fighter_disallow_continuation(void) {
    pz_fighter_disallow_continuation();
}

void _pz_fighter_allow_continuation(void) {
    pz_fighter_allow_continuation();
}

void _bgnd_add_new_normal_check_for_hider(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_add_new_normal_check_for_hider(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_remove_wall_from_hider(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_remove_wall_from_hider(args.raw->slots[0].i);
}

void _bgnd_add_wall_to_hide(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_add_wall_to_hide(args.raw->slots[0].i);
}

void _bgnd_add_wall_to_unhide(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_add_wall_to_unhide(args.raw->slots[0].i);
}

void _bgnd_start_wall_hider(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_wall_hider(args.raw->slots[0].i);
}

void _bgnd_turn_off_backface_culling(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_turn_off_backface_culling(args.raw->slots[0].i);
}

void _bgnd_sobj_cam_frustum_test_into_transparent(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_cam_frustum_test_into_transparent(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _obj_sobj_cam_frustum_test_into_transparent(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_sobj_cam_frustum_test_into_transparent(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_sobj_cam_volume_test_steer_over(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_cam_volume_test_steer_over(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_rotate_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_rotate_sobj(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_set_wall_hide_distance(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_wall_hide_distance(current_args, args.raw->slots[0].f);
}

void _bgnd_set_new_ground_plane(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_new_ground_plane(current_args, args.raw->slots[0].f);
}

void _bgnd_place_weapon_at_position(void) {
    ScriptArgsRef args;
    float sp28;
    float sp24;
    float sp20;
    float sp1C;
    float sp18;
    float sp14;
    float sp10;
    float spC;
    float sp8;

    args.bytes = current_args;
    sp8 = args.raw->slots[13].f;
    spC = args.raw->slots[14].f;
    sp10 = args.raw->slots[15].f;
    sp14 = args.raw->slots[16].f;
    sp18 = args.raw->slots[18].f;
    sp1C = args.raw->slots[19].f;
    sp20 = args.raw->slots[20].f;
    sp24 = args.raw->slots[21].f;
    sp28 = args.raw->slots[22].f;
    bgnd_place_weapon_at_position(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[17].i, args.raw->slots[23].i, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, args.raw->slots[8].f, args.raw->slots[9].f, args.raw->slots[10].f, args.raw->slots[11].f, args.raw->slots[12].f);
}

void _bgnd_clean_slaughterhouse(void) {
    bgnd_clean_slaughterhouse();
}

void _bgnd_sh_level_2(void) {
    bgnd_sh_level_2();
}

void _bgnd_sh_level_1(void) {
    bgnd_sh_level_1();
}

void _bgnd_start_sh_fx(void) {
    bgnd_start_sh_fx();
}

void _pz_fighter_reaction_xfer_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pz_fighter_reaction_xfer_him(args.raw->slots[0].i);
}

void _pz_fighter_function(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pz_fighter_function(args.raw->slots[0].i);
}

void _pz_fighter_fetch_distance_to_center_pos(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = pz_fighter_fetch_distance_to_center_pos();
}

void _set_anim_hiframe(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_anim_hiframe(current_args, args.raw->slots[0].f);
}

void _bgnd_reg_col_cb_for_beetle_lair(void) {
    bgnd_reg_col_cb_for_beetle_lair();
}

void _pz_fighter_wipe_blood_off_hands(void) {
    pz_fighter_wipe_blood_off_hands();
}

void _bgnd_get_preload_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_get_preload_obj(args.raw->slots[0].i);
}

void _bgnd_preload_named_model(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(1);
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_preload_named_model(args.raw->slots[1].i);
}

void _sobj_set_alpha(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    sobj_set_alpha(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _obj_get_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_get_scale(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _obj_set_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_set_scale(args.raw->slots[0].pointer, args.raw->slots[1].pointer);
}

void _bgnd_sobj_set_ani_framerate(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_ani_framerate(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _bgnd_sobj_set_ani_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_ani_frame(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _bgnd_sobj_set_alpha(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_alpha(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_sobj_set_rel_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_rel_pos(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_sobj_set_ang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_ang(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_sobj_get_ang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_get_ang(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_sobj_set_pos_vel(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_pos_vel(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_sobj_set_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_sobj_set_pos(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_sobj_get_z_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_z_pos(args.raw->slots[0].i);
}

void _bgnd_sobj_get_y_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_y_pos(args.raw->slots[0].i);
}

void _bgnd_sobj_get_x_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_x_pos(args.raw->slots[0].i);
}

void _bgnd_unhide_sobj_and_children(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_unhide_sobj_and_children(args.raw->slots[0].i);
}

void _bgnd_hide_sobj_and_children(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_hide_sobj_and_children(args.raw->slots[0].i);
}

void _bgnd_unhide_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_unhide_sobj(args.raw->slots[0].i);
}

void _bgnd_hide_sobj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_hide_sobj(args.raw->slots[0].i);
}

void _head_tracking_on(void) {
    head_tracking_on();
}

void _head_tracking_off(void) {
    head_tracking_off();
}

void _load_aux_weapon(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    load_aux_weapon(args.raw->slots[0].pointer);
}

void _ani_loop_more_frames(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_loop_more_frames(current_args, args.raw->slots[0].f);
}

void _ani_1_frame(void) {
    ani_1_frame();
}

void _pz_fighter_force_reaction_in_ticks(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pz_fighter_force_reaction_in_ticks(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_replace_tex_with_wiff_and_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_replace_tex_with_wiff_and_ani(
        args.raw->slots[0].i, get_script_string_arg(2),
        args.raw->slots[2].f, args.raw->slots[3].i,
        args.raw->slots[4].i);
}

void _bgnd_pulsate_object_with_caps_and_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pulsate_object_with_caps_and_scale(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[5].i, args.raw->slots[6].i, current_args, args.raw->slots[2].f, args.raw->slots[4].f, args.raw->slots[7].f, args.raw->slots[8].f, args.raw->slots[9].f, args.raw->slots[10].f, args.raw->slots[11].f, args.raw->slots[12].f);
}

void _bgnd_pulsate_object_with_caps(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pulsate_object_with_caps(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[5].i, args.raw->slots[6].i, args.raw->slots[2].f, args.raw->slots[4].f);
}

void _bgnd_pulsate_object(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_pulsate_object(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[3].i, current_args, args.raw->slots[2].f, args.raw->slots[4].f);
}

void _bgnd_turn_on_backface_culling(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_turn_on_backface_culling(args.raw->slots[0].i);
}

void _bgnd_no_z_write(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_no_z_write(args.raw->slots[0].i);
}

void _frand(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = frand(current_args, args.raw->slots[0].f);
}

void _plyr_start_script_in_plyr_pdata_proc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_start_script_in_plyr_pdata_proc(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _plyr_start_script_in_proc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_start_script_in_proc(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_always_face_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_always_face_y(args.raw->slots[0].i);
}

void _bgnd_apply_zoffset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_apply_zoffset(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_start_sobj_uv_scroll(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_sobj_uv_scroll(args.raw->slots[0].i, args.raw->slots[5].i, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _bgnd_create_sobjs(void) {
    bgnd_create_sobjs();
}

void _sobj_no_zwrite(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    sobj_no_zwrite(args.raw->slots[0].pointer);
}

void _register_baraka_cb_functions(void) {
    register_baraka_cb_functions();
}

void _start_baraka_blades_monitor(void) {
    start_baraka_blades_monitor();
}

void _start_baraka_jaw_monitor(void) {
    start_baraka_jaw_monitor();
}

void _pz_fighter_inline_force_away_with_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = pz_fighter_inline_force_away_with_ani(args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[0].f, args.raw->slots[2].f);
}

void _suspend_in_midair(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    suspend_in_midair(current_args, args.raw->slots[0].f);
}

void _snd_major_hit_voice(void) {
    snd_major_hit_voice();
}

void _pz_fighter_shaking(void) {
    pz_fighter_shaking();
}

void _pz_fighter_create_space_between_fighters(void) {
    pz_fighter_create_space_between_fighters();
}

void _pz_fighter_force_repel_during_attack(void) {
    pz_fighter_force_repel_during_attack();
}

void _pz_fighter_dont_fudge_desired_distance(void) {
    pz_fighter_dont_fudge_desired_distance();
}

void _pz_fighter_check_to_toggle_obj_and_ani_flips(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pz_fighter_check_to_toggle_obj_and_ani_flips(args.raw->slots[0].i);
}

void _pz_fighter_move_into_fighting_position(void) {
    pz_fighter_move_into_fighting_position();
}

void _pz_fighter_release_other_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pz_fighter_release_other_player(args.raw->slots[0].i);
}

void _pz_fighter_step_throw_into_check(void) {
    pz_fighter_step_throw_into_check();
}

void _pz_fighter_check_breakout(void) {
    pz_fighter_check_breakout();
}

void _pz_fighter_long_exit(void) {
    pz_fighter_long_exit();
}

void _pz_fighter_exit(void) {
    pz_fighter_exit();
}

void _pz_fighter_register_move(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pz_fighter_register_move(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i);
}

void _get_my_particle_player_bank_num(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_my_particle_player_bank_num();
}

void _flash_hit_at_bid_with_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    flash_hit_at_bid_with_y(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _flash_hit_at_bid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    flash_hit_at_bid(args.raw->slots[0].i);
}

void _fight_fx_im_hit_flash(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    fight_fx_im_hit_flash(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, current_args, args.raw->slots[4].f);
}

void _launch_fx_at_pos_with_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = launch_fx_at_pos_with_obj(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _low_flash_check(void) {
    low_flash_check();
}

void _medium_flash_check(void) {
    medium_flash_check();
}

void _high_flash_check(void) {
    high_flash_check();
}

void _bgnd_place_point_light_for_ticks(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_place_point_light_for_ticks(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[2].f);
}

void _bgnd_delete_danger_zone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_delete_danger_zone(args.raw->slots[0].i);
}

void _bgnd_set_active_danger_zone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_active_danger_zone(args.raw->slots[0].i);
}

void _bgnd_set_danger_zone_radius(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_danger_zone_radius(current_args, args.raw->slots[0].f);
}

void _bgnd_set_viewing_of_danger_zones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_viewing_of_danger_zones(args.raw->slots[0].i);
}

void _bgnd_set_danger_zone_y_angle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_danger_zone_y_angle(current_args, args.raw->slots[0].f);
}

void _bgnd_set_danger_zone_depth(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_danger_zone_depth(current_args, args.raw->slots[0].f);
}

void _bgnd_set_danger_zone_width(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_danger_zone_width(current_args, args.raw->slots[0].f);
}

void _bgnd_set_danger_zone_center_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_danger_zone_center_position(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _bgnd_enable_danger_zone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_enable_danger_zone(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_create_danger_zone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_create_danger_zone(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[4].i, args.raw->slots[3].f);
}

void _bgnd_place_object_at_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_place_object_at_position(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i);
}

void _do_i_have_life_left(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = do_i_have_life_left();
}

void _hide_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hide_player(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _setup_for_flip_ani(void) {
    setup_for_flip_ani();
}

void _get_victory_flip_flags(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_victory_flip_flags();
}

void _obj_set_z_offsets(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_set_z_offsets(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _get_my_plyr_num(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_my_plyr_num();
}

void _do_victory_camera(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    do_victory_camera((VictoryCameraConfig*)args.raw->slots[0].pointer);
}

void _unfreeze_player(void) {
    unfreeze_player();
}

void _turn_into_energy_player(void) {
    turn_into_energy_player();
}

void _is_blood_disabled(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_blood_disabled();
}

void _plyr_get_f_constrained(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = plyr_get_f_constrained(args.raw->slots[0].i);
}

void _move_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    move_player(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _face_ang_from_pos_to_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    face_ang_from_pos_to_him(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _move_player_no_constrain_update(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    move_player_no_constrain_update(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _show_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    show_player(args.raw->slots[0].i);
}

void _plyr_invulnerable_to_projectiles(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_invulnerable_to_projectiles(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _active_sidekick_swap_from_behind(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    active_sidekick_swap_from_behind(args.plyr_pdata->player);
}

void _active_sidekick_swap_from_sky(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    active_sidekick_swap_from_sky(args.plyr_pdata->player);
}

void _active_sidekick_swap_change_style(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    active_sidekick_swap_change_style(args.plyr_pdata->player);
}

void _is_sidekick_active(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i =
        is_sidekick_active((PlyrInfo*)args.raw->slots[0].pointer);
}

void _taunt_increase_life(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i =
        taunt_increase_life(
            args.raw->slots[0].f, args.raw->slots[1].f);
}

void _get_collision_result(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_collision_result();
}

void _general_flash_fx(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    general_flash_fx(
        args.raw->slots[0].i, (MkObj*)args.raw->slots[1].pointer,
        get_script_string_arg(3), args.raw->slots[3].i,
        args.raw->slots[4].i, args.raw->slots[5].f);
}

void _get_kombat_difficulty(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_kombat_difficulty();
}

void _uv_my_angle_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    uv_my_angle_y(
        args.raw->slots[0].pointer, args.raw->slots[1].f);
}

void _obj_set_flipped_bones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_set_flipped_bones(
        args.raw->slots[0].pointer, args.raw->slots[1].pointer);
}

void _run_camera_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    run_camera_script(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _am_i_airborn_check_in_reaction(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = am_i_airborn_check_in_reaction();
}

void _obj_enable_grounding(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_enable_grounding(args.raw->slots[0].i);
}

void _camera_idle(void) {
    camera_idle();
}

void _drone_ai_increase_big_boss_stage(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_ai_increase_big_boss_stage(args.raw->slots[0].i);
}

void _pz_finish_him_request(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = pz_finish_him_request();
}

void _pz_fighter_create_space_between_fighters_for_special_moves(void) {
    pz_fighter_create_space_between_fighters_for_special_moves();
}

void _freeze_player(void) {
    freeze_player();
}

void _get_his_previous_state(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_his_previous_state();
}

void _get_his_secondary_state(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_his_secondary_state();
}

void _pebble_get_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pebble_get_pos(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _is_big_boss(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = is_big_boss(args.raw->slots[0].i);
}

void _is_load_meter_active(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_load_meter_active();
}

void _plyr_snd_req_no_plyr_proc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = plyr_snd_req_no_plyr_proc(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _noobsmoke_sidekick_double_charge(void) {
    noobsmoke_sidekick_double_charge();
}

void _noobsmoke_sidekick_projectile(void) {
    noobsmoke_sidekick_projectile();
}

void _forced_step_forward(void) {
    forced_step_forward();
}

void _start_projectile_from_sidekick_bone(void) {
    ScriptArgsRef args;
    ScriptResultRef result;

    args.bytes = current_args;
    result.bytes = active_cmdscript;
    result.mkobj->value = start_projectile_from_sidekick_bone(
        args.raw->slots[0].i,
        (MkObj*)args.raw->slots[1].pointer,
        get_script_string_arg(3), args.raw->slots[3].f,
        args.raw->slots[4].f,
        (const Vec*)args.raw->slots[5].pointer);
}

void _noobsmoke_fire_projectile_request(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = noobsmoke_fire_projectile_request();
}

void _smoke_victory_entrance(void) {
    smoke_victory_entrance();
}

void _noob_victory_entrance(void) {
    noob_victory_entrance();
}

void _sidekick_switch_style_swap(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    sidekick_switch_style_swap(args.raw->slots[1].i, args.raw->slots[0].f);
}

void _dk_voice_call(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    dk_voice_call(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _dk_taunt_at_screen(void) {
    dk_taunt_at_screen();
}

void _special_move_cam_setup2(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    special_move_cam_setup2(args.raw->slots[5].i, args.raw->slots[6].i,
                            args.raw->slots[7].i,
                            (MkObj*)args.raw->slots[8].pointer,
                            (MkObj*)args.raw->slots[9].pointer,
                            args.raw->slots[0].f,
                            args.raw->slots[1].f, args.raw->slots[2].f,
                            args.raw->slots[3].f, args.raw->slots[4].f);
}

void _rd_set_impact_vector(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    rd_set_impact_vector(current_args, args.raw->slots[0].f);
}

void _get_plyr_pdata_plyr_num(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.pointer =
        get_plyr_pdata_plyr_num(args.raw->slots[0].i);
}

void _is_special_move_available(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i =
        is_special_move_available(
            (PlyrPdata*)args.raw->slots[0].pointer,
            args.raw->slots[1].i);
}

void _retract_spear_from_camera(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    retract_spear_from_camera(args.raw->slots[0].i);
}

void _fire_spear_at_camera(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = fire_spear_at_camera(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _kick_the_camera(void) {
    kick_the_camera();
}

void _random_dk_foot(void) {
    random_dk_foot();
}

void _stop_vomit_slip_sound(void) {
    stop_vomit_slip_sound();
}

void _drone_ai_should_ermac_fly_kick(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = drone_ai_should_ermac_fly_kick();
}

void _drone_ai_should_ermac_ground_slam(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = drone_ai_should_ermac_ground_slam();
}

void _clear_his_f_constrained(void) {
    clear_his_f_constrained();
}

void _idle_his_anim_proc(void) {
    idle_his_anim_proc();
}

void _set_active_projectile_block_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_active_projectile_block_script(args.raw->slots[0].i);
}

void _bgnd_get_obj_pointer(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_get_obj_pointer(args.raw->slots[0].i);
}

void _bgnd_get_anim_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_anim_info(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _is_character_unlocked_in_profile(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = is_character_unlocked_in_profile(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mini_mission_inactive(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mini_mission_inactive(args.raw->slots[0].i);
}

void _mini_mission_completed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mini_mission_completed(args.raw->slots[0].i);
}

void _start_mini_mission(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_mini_mission(args.raw->slots[0].i);
}

void _is_mini_mission_started(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_started(args.raw->slots[0].i);
}

void _is_mini_mission_active(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_active(args.raw->slots[0].i);
}

void _is_mini_mission_completed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_completed(args.raw->slots[0].i);
}

void _player_add_item_to_inventory(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    player_add_item_to_inventory(args.raw->slots[0].i);
}

void _player_remove_item_from_inventory(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    player_remove_item_from_inventory(args.raw->slots[0].i);
}

void _player_has_item(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = player_has_item(args.raw->slots[0].i);
}

void _add_to_konq_profile_value(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_to_konq_profile_value(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _get_last_character_trained_with(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_last_character_trained_with();
}

void _set_last_character_trained_with(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_last_character_trained_with(args.raw->slots[0].i);
}

void _get_konq_profile_value(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = get_konq_profile_value(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _set_konq_profile_value(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_konq_profile_value(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _set_pui_status(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_pui_status(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _get_pui_status(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = get_pui_status(args.raw->slots[0].i);
}

void _hf_bgnd_set_in_setup_zone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hf_bgnd_set_in_setup_zone(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _hf_bgnd_set_smasher_mode(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hf_bgnd_set_smasher_mode(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_set_player_shadow_ground_plane(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_player_shadow_ground_plane(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _bgnd_end_the_game_and_restart(void) {
    bgnd_end_the_game_and_restart();
}

void _bgnd_pfx_resume_effect(void) {
    get_script_string_arg(1);
    bgnd_pfx_resume_effect();
}

void _bgnd_pfx_reset_effect(void) {
    get_script_string_arg(1);
    bgnd_pfx_reset_effect();
}

void _bgnd_reset_players_animation_height(void) {
    bgnd_reset_players_animation_height();
}

void _nbc_script_debug_point(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    nbc_script_debug_point(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _bgnd_unhide_mirror_guys(void) {
    bgnd_unhide_mirror_guys();
}

void _bgnd_hide_mirror_guys(void) {
    bgnd_hide_mirror_guys();
}

void _bgnd_set_sobj_uv_scroll_abs_values(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_sobj_uv_scroll_abs_values(args.raw->slots[4].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_set_sobj_uv_scroll_rate_values(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_set_sobj_uv_scroll_rate_values(args.raw->slots[4].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _bgnd_init_all_uv_scroll_w_control(void) {
    bgnd_init_all_uv_scroll_w_control();
}

void _bgnd_destroy_sobj_uv_scroll_w_control(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_destroy_sobj_uv_scroll_w_control(args.raw->slots[0].i);
}

void _bgnd_start_sobj_uv_scroll_w_control(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = bgnd_start_sobj_uv_scroll_w_control(args.raw->slots[0].i, args.raw->slots[5].i, args.raw->slots[6].i, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _sh_lower_level_pebble_unhide(void) {
    sh_lower_level_pebble_unhide();
}

void _sh_lower_level_pebble_hide(void) {
    sh_lower_level_pebble_hide();
}

void _bgnd_get_camera_z_pos(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_camera_z_pos();
}

void _bgnd_get_camera_y_angle(void) {
    ((ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_camera_y_angle();
}

void _bgnd_delete_proc_by_id(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_delete_proc_by_id(args.raw->slots[0].i);
}

void _bgnd_start_script_in_proc_bigstack(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_script_in_proc_bigstack(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _bgnd_start_script_in_proc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_script_in_proc(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_run_shove_animation(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_run_shove_animation(args.raw->slots[0].i);
}

void _npc_shove_reaction_standard_shutdown(void) {
    npc_shove_reaction_standard_shutdown();
}

void _npc_shove_reaction_standard_setup(void) {
    npc_shove_reaction_standard_setup();
}

void _npc_punch_reaction_standard_shutdown(void) {
    npc_punch_reaction_standard_shutdown();
}

void _npc_prepare_for_unconscious_state(void) {
    npc_prepare_for_unconscious_state();
}

void _npc_punch_reaction_standard_setup(void) {
    npc_punch_reaction_standard_setup();
}

void _npc_get_collision_direction_in_script(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = npc_get_collision_direction_in_script();
}

void _npc_punch_reaction_check_data(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = npc_punch_reaction_check_data();
}

void _npc_run_punch_animation(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_run_punch_animation(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, current_args, args.raw->slots[4].f);
}

void _npc_snap_to_face_monk(void) {
    npc_snap_to_face_monk();
}

void _interior_exit_button_script(void) {
    interior_exit_button_script();
}

void _get_ir_cam_ang_z(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_z(args.raw->slots[0].i);
}

void _get_ir_cam_ang_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_y(args.raw->slots[0].i);
}

void _get_ir_cam_ang_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_x(args.raw->slots[0].i);
}

void _get_ir_cam_pos_z(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_z(args.raw->slots[0].i);
}

void _get_ir_cam_pos_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_y(args.raw->slots[0].i);
}

void _get_ir_cam_pos_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_x(args.raw->slots[0].i);
}

void _start_konquest_interior(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_konquest_interior(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[5].i, args.raw->slots[6].i);
}

void _refresh_rate(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = refresh_rate();
}

void _show_shujinko_unlock_screen(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    show_shujinko_unlock_screen(args.raw->slots[0].i);
}

void _trial_register_special_move(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_register_special_move(args.raw->slots[0].i);
}

void _trial_invisible_callback(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = trial_invisible_callback(args.raw->slots[0].i);
}

void _trial_set_special_restrictions(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_special_restrictions(args.raw->slots[0].i);
}

void _drone_set_handicap(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_handicap(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _drone_start_bleeding(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_start_bleeding(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _drone_set_damage_multiplier(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_damage_multiplier(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _release_kamidogu(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    release_kamidogu(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _start_constrain_proc(void) {
    start_constrain_proc();
}

void _credits_add_text(void) {
    ScriptArgsRef args;
    char* temp_r31_19847;

    args.bytes = current_args;
    temp_r31_19847 = get_script_string_arg(2);
    get_script_string_arg(1);
    credits_add_text(temp_r31_19847, args.raw->slots[2].i);
}

void _trial_state_collision_check(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_state_collision_check(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _trial_debug_mission_list(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_debug_mission_list(args.raw->slots[0].i);
}

void _trial_get_background_root(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = trial_get_background_root();
}

void _build_bones_tbl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = build_bones_tbl(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _play_background_music(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    play_background_music(args.raw->slots[0].i);
}

void _give_koin_award(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    give_koin_award(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _konquest_run_ending(void) {
    konquest_run_ending();
}

void _trial_set_round_health_restoration(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_round_health_restoration(current_args, args.raw->slots[0].f);
}

void _camera_set_anim_aux_data(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_anim_aux_data(
        (CameraAnimEvent*)args.raw->slots[0].pointer);
}

void _trial_mirror_anims_if_needed(void) {
    trial_mirror_anims_if_needed();
}

void _current_player_is_drone(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = current_player_is_drone();
}

void _drone_apply_damage(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_apply_damage(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _drone_set_special_directions(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_special_directions(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _drone_set_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_position(args.raw->slots[0].i, args.raw->slots[1].f,
                       args.raw->slots[2].f, args.raw->slots[3].f);
}

void _drone_set_health(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_health(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _drone_do_special_move(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_do_special_move(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _drone_change_to_style(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_change_to_style(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _drone_lip_synch(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_lip_synch(args.raw->slots[0].i,
                    (LipSyncKeyframe*)args.raw->slots[1].pointer);
}

void _trial_show_monk(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_show_monk(args.raw->slots[0].i);
}

void _show_text(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    show_text(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[6].i, args.raw->slots[7].i, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _trial_set_next_mission(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_next_mission(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[5].i, args.raw->slots[6].i, args.raw->slots[7].i);
}

void _trial_setup_nis_scene(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_setup_nis_scene(args.raw->slots[0].i);
}

void _drone_set_anim_step(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_anim_step(current_args, args.raw->slots[0].f);
}

void _drone_face_monk(void) {
    drone_face_monk();
}

void _drone_set_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_script(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _trial_do_dialog(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_do_dialog(
        args.raw->slots[0].i, args.raw->slots[1].i,
        args.raw->slots[2].f, args.raw->slots[3].f,
        args.raw->slots[4].f, args.raw->slots[5].i,
        args.raw->slots[6].i);
}

void _drone_set_difficulty_level(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_difficulty_level(args.raw->slots[0].i);
}

void _drone_dispatch_switches(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_dispatch_switches(args.raw->slots[0].i);
}

void _drone_set_switch_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    drone_set_switch_state(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _trial_restart_round(void) {
    trial_restart_round();
}

void _trial_start_countdown(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_start_countdown(args.raw->slots[0].i,
                          args.raw->slots[1].f,
                          args.raw->slots[2].f);
}

void _trial_set_move_message(void) {
    char* temp_r31_20333;

    temp_r31_20333 = get_script_string_arg(2);
    get_script_string_arg(1);
    trial_set_move_message(temp_r31_20333);
}

void _trial_set_next_setup_function(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_next_setup_function(args.raw->slots[0].i);
}

void _trial_set_combo_requirement(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_combo_requirement(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _trial_add_required_sequence(void) {
    char* temp_r31_20382;

    temp_r31_20382 = get_script_string_arg(2);
    get_script_string_arg(1);
    trial_add_required_sequence(temp_r31_20382);
}

void _trial_setup_onscreen_display_items(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_setup_onscreen_display_items(
        args.raw->slots[0].i, args.raw->slots[1].i,
        get_script_string_arg(3));
}

void _trial_add_success_condition(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_add_success_condition(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _trial_set_type(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_type(args.raw->slots[0].i);
}

void _trial_show_spoken_text_window(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_show_spoken_text_window(args.raw->slots[0].i,
                                  args.raw->slots[1].f,
                                  args.raw->slots[2].f,
                                  args.raw->slots[3].f,
                                  args.raw->slots[4].i,
                                  args.raw->slots[5].i,
                                  args.raw->slots[6].i,
                                  args.raw->slots[7].i,
                                  args.raw->slots[8].i);
}

void _trial_show_text_window(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_show_text_window(args.raw->slots[0].i, args.raw->slots[4].i, args.raw->slots[5].i, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _trial_set_ending_functions(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_ending_functions(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _trial_set_tick_function(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_tick_function(args.raw->slots[0].i);
}

void _trial_set_round_timer(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_round_timer(args.raw->slots[0].i);
}

void _trial_set_num_rounds(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trial_set_num_rounds(args.raw->slots[0].i);
}

void _konquest_map_setup_fight(void) {
    ScriptArgsRef args;
    const char* arena_name;

    args.bytes = current_args;
    arena_name = get_script_string_arg(9);
    konquest_map_setup_fight(
        args.raw->slots[0].i, args.raw->slots[1].i,
        args.raw->slots[2].i, args.raw->slots[3].i,
        args.raw->slots[4].i, args.raw->slots[5].i,
        args.raw->slots[6].i, args.raw->slots[7].i, arena_name);
}

void _initialize_clone_lights(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    initialize_clone_lights(args.raw->slots[0].i);
}

void _jab_spawn_point_light_at_world_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = jab_spawn_point_light_at_world_pos(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _jab_attach_point_light_to_obj_bone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = jab_attach_point_light_to_obj_bone(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _jab_flash_screen(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jab_flash_screen(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _jab_stop_dragon_king_shake(void) {
    jab_stop_dragon_king_shake();
}

void _jab_shake_dragon_king(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jab_shake_dragon_king(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _fade_fatality_screen(void) {
    fade_fatality_screen();
}

void _jab_attach_wiff_to_sobj(void) {
    ScriptArgsRef args;
    const char* animation;

    args.bytes = current_args;
    animation = get_script_string_arg(4);
    jab_attach_wiff_to_sobj(
        args.raw->slots[0].i, args.raw->slots[1].i,
        get_script_string_arg(3), animation, args.raw->slots[4].i,
        args.raw->slots[6].i, args.raw->slots[5].f);
}

void _jab_destroy_drink_obj_in_hand(void) {
    jab_destroy_drink_obj_in_hand();
}

void _jab_attach_drink_obj_to_hand(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jab_attach_drink_obj_to_hand(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _jab_face_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jab_face_obj(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _obj_scale_over_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    obj_scale_over_time(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _jab_release_jade_boomerang(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jab_release_jade_boomerang(args.raw->slots[0].i);
}

void _jab_start_jade_boomerang_throw(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jab_start_jade_boomerang_throw(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _jab_setup_kiss_emitter_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    jab_setup_kiss_emitter_obj(args.raw->slots[0].i);
}

void _kill_konquest_dialog_procs(void) {
    kill_konquest_dialog_procs();
}

void _hero_turn_to_face_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hero_turn_to_face_position(args.raw->slots[0].i);
}

void _attach_pfx_to_object(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    attach_pfx_to_object(args.raw->slots[0].i, get_script_string_arg(2), args.raw->slots[2].i);
}

void _npc_hide_skip_message(void) {
    npc_hide_skip_message();
}

void _npc_show_skip_message(void) {
    npc_show_skip_message();
}

void _konquest_hero_portal_in(void) {
    konquest_hero_portal_in();
}

void _konquest_set_current_portal_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_set_current_portal_uid(args.raw->slots[0].i);
}

void _npc_sleep_until_model_loaded(void) {
    npc_sleep_until_model_loaded();
}

void _set_konquest_weather(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_konquest_weather(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _set_age_progression(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_age_progression(args.raw->slots[0].i);
}

void _display_time_progression_images(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    display_time_progression_images(args.raw->slots[0].i);
}

void _konquest_use_portal(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_use_portal(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[5].i, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _konquest_teleport_hero_to_location(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_teleport_hero_to_location(args.raw->slots[0].i);
}

void _show_fight_message(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    show_fight_message(args.raw->slots[0].i);
}

void _npc_start_blood_fall(void) {
    npc_start_blood_fall();
}

void _npc_get_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = npc_get_obj(args.raw->slots[0].i);
}

void _get_hero_state(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_hero_state();
}

void _interaction_cam_set_target_info(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    interaction_cam_set_target_info(args.raw->slots[6].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f);
}

void _set_movement_npc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_movement_npc(args.raw->slots[0].i);
}

void _set_look_at_npc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_look_at_npc(args.raw->slots[0].i);
}

void _get_krypt_anim_pdata(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_krypt_anim_pdata();
}

void _get_krypt_character_obj(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_krypt_character_obj();
}

void _get_krypt_current_column(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_krypt_current_column();
}

void _get_krypt_current_row(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_krypt_current_row();
}

void _transition_to_krypt_character_anim_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    transition_to_krypt_character_anim_script(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _set_krypt_character_anim_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_krypt_character_anim_script(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _set_krypt_character_previous_root_angle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_krypt_character_previous_root_angle(current_args, args.raw->slots[0].f);
}

void _set_krypt_character_angle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_krypt_character_angle(current_args, args.raw->slots[0].f);
}

void _set_krypt_character_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_krypt_character_pos(args.raw->slots[0].i);
}

void _load_krypt_character(void) {
    get_script_string_arg(1);
    ((ScriptRawResult*)active_cmdscript)->value.i = load_krypt_character();
}

void _npc_start_fx_at_his_position(void) {
    npc_start_fx_at_his_position(
        ((ScriptRawArgs*)current_args)->slots[0].pointer,
        get_script_string_arg(2),
        (const Vec*)((ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _npc_start_fx_at_position(void) {
    npc_start_fx_at_position(
        get_script_string_arg(1),
        (const Vec*)((ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _hero_start_fx_at_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(1);
    hero_start_fx_at_position(args.raw->slots[1].i);
}

void _set_hero_position_relative_to_chest(void) {
    set_hero_position_relative_to_chest();
}

void _get_pickup_object(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_pickup_object();
}

void _set_reference_pui(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_reference_pui(args.raw->slots[0].i);
}

void _open_chest_and_give_item_to_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    open_chest_and_give_item_to_player(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _open_chest_and_unlock_kontent(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    open_chest_and_unlock_kontent(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _give_krypt_key_to_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    give_krypt_key_to_player(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _give_reward_to_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    give_reward_to_player(args.raw->slots[0].i);
}

void _npc_play_two_player_one_shot_anims(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_play_two_player_one_shot_anims(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_assign_door_path(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_assign_door_path(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_assign_path_to_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_assign_path_to_him(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _npc_assign_path(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_assign_path(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _npc_open_door_at_waypoint(void) {
    npc_open_door_at_waypoint();
}

void _setup_vomit_slip_sound(void) {
    setup_vomit_slip_sound();
}

void _nis_remove_non_participants(void) {
    nis_remove_non_participants();
}

void _set_camera_to_look_at_hero(void) {
    set_camera_to_look_at_hero();
}

void _npc_reset_my_timed_events(void) {
    npc_reset_my_timed_events();
}

void _konquest_setup_pui_particle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_script_string_arg(1);
    konquest_setup_pui_particle(args.raw->slots[1].i);
}

void _npc_wait_for_wake_up(void) {
    npc_wait_for_wake_up();
}

void _npc_set_wake_up_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_wake_up_time(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _save_hero_position_and_angle_prior_to_fight(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    save_hero_position_and_angle_prior_to_fight(current_args, args.raw->slots[0].f);
}

void _npc_set_my_conversation_counter(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_my_conversation_counter(args.raw->slots[0].i);
}

void _npc_set_his_conversation_counter(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_his_conversation_counter(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_set_my_punch_counter(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_my_punch_counter(args.raw->slots[0].i);
}

void _npc_set_his_punch_counter(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_his_punch_counter(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _stop_chest_camera_script(void) {
    stop_chest_camera_script();
}

void _konquest_run_camera_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_run_camera_script(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _set_hero_punched_ground_collisions(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_hero_punched_ground_collisions(args.raw->slots[0].i);
}

void _restore_hero_grounding(void) {
    restore_hero_grounding();
}

void _suspend_hero_grounding(void) {
    suspend_hero_grounding();
}

void _start_hero_collisions(void) {
    start_hero_collisions();
}

void _stop_hero_collisions(void) {
    stop_hero_collisions();
}

void _add_hours_to_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_hours_to_time(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _add_days_to_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_days_to_time(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _add_months_to_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_months_to_time(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _add_years_to_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_years_to_time(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _pui_play_pfx_sequence(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pui_play_pfx_sequence(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _pui_play_pfx(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pui_play_pfx(
        args.raw->slots[0].i, args.raw->slots[1].i,
        get_script_string_arg(3));
}

void _pui_set_kill_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pui_set_kill_time(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _pui_set_color(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pui_set_color(args.raw->slots[0].i, (unsigned char)args.raw->slots[1].i, (unsigned char)args.raw->slots[2].i, (unsigned char)args.raw->slots[3].i, (unsigned char)args.raw->slots[4].i);
}

void _pui_delay_spawn(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pui_delay_spawn(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _transition_to_region(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    transition_to_region(args.raw->slots[0].i);
}

void _get_previous_konquest_region_number(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_previous_konquest_region_number();
}

void _set_konquest_region_number(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_konquest_region_number(args.raw->slots[0].i);
}

void _npc_play_teleported_sound(void) {
    npc_play_teleported_sound();
}

void _konquest_hide_damashi(void) {
    konquest_hide_damashi();
}

void _konquest_start_damashi(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = konquest_start_damashi(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _konquest_camera_return_to_normal(void) {
    konquest_camera_return_to_normal();
}

void _display_konquest_text(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = display_konquest_text(args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _hero_stop_moving(void) {
    hero_stop_moving();
}

void _set_monk_age(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_monk_age(args.raw->slots[0].i);
}

void _get_monk_age(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_monk_age();
}

void _change_monk_age(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    change_monk_age(args.raw->slots[0].i);
}

void _hero_handle_conversation(void) {
    hero_handle_conversation();
}

void _nis_clear_event_list(void) {
    nis_clear_event_list();
}

void _npc_switch_camera_focus(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_switch_camera_focus(args.raw->slots[0].i);
}

void _npc_at_waypoint_set_flags(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_at_waypoint_set_flags(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _kill_dynamic_pui(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    kill_dynamic_pui(args.raw->slots[0].i);
}

void _spawn_dynamic_pui_critical(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = spawn_dynamic_pui_critical(args.raw->slots[0].i);
}

void _spawn_dynamic_pui(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = spawn_dynamic_pui(args.raw->slots[0].i);
}

void _kill_pui(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    kill_pui(args.raw->slots[0].i);
}

void _pickup_dynamic_pui(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pickup_dynamic_pui(args.raw->slots[0].i);
}

void _pickup_pui(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    pickup_pui(args.raw->slots[0].i);
}

void _spawn_pui(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    spawn_pui(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _attach_pfx_to_object_by_uid(void) {
    ScriptArgsRef current;
    ScriptArgsRef saved;
    const char* effect_name;

    saved.bytes = current_args;
    effect_name = get_script_string_arg(2);
    current.bytes = current_args;
    attach_pfx_to_object_by_uid(
        current.raw->slots[0].i, effect_name, saved.raw->slots[2].i,
        saved.raw->slots[3].i);
}

void _add_trigger_list_to_world(void) {
    add_trigger_list_to_world();
}

void _npc_sleep(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_sleep(current_args, args.raw->slots[0].f);
}

void _npc_get_punch_count(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = npc_get_punch_count();
}

void _npc_get_conversation_count(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = npc_get_conversation_count();
}

void _konquest_passed_last_mission(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = konquest_passed_last_mission();
}

void _konquest_transition_object_to_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_transition_object_to_state(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _npc_fire_trigger(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_fire_trigger(args.raw->slots[0].i);
}

void _konquest_open_door(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_open_door(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _get_active_npc_data(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_active_npc_data();
}

void _npc_glitch_him_to_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_glitch_him_to_ani(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _npc_glitch_to_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_glitch_to_ani(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_ani_1_frame(void) {
    npc_ani_1_frame();
}

void _npc_set_random_dialog_and_anim_sequence(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_random_dialog_and_anim_sequence(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_play_random_dialog_sequence(void) {
    npc_play_random_dialog_sequence();
}

void _npc_play_dialog_and_anim_sequence(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_play_dialog_and_anim_sequence(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _play_beam_advance_sound(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    play_beam_advance_sound(args.raw->slots[0].i);
}

void _show_objective_arrow_and_beam(void) {
    show_objective_arrow_and_beam();
}

void _hide_objective_arrow_and_beam(void) {
    hide_objective_arrow_and_beam();
}

void _npc_attack(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_attack(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_change_path_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_change_path_speed(current_args, args.raw->slots[0].f);
}

void _npc_set_pinanim_flag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_pinanim_flag(args.raw->slots[0].i);
}

void _npc_set_ani_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_ani_frame(current_args, args.raw->slots[0].f);
}

void _npc_set_my_ground_level(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_my_ground_level(current_args, args.raw->slots[0].f);
}

void _turn_to_face_interior_door(void) {
    turn_to_face_interior_door();
}

void _turn_to_face_exterior_door(void) {
    turn_to_face_exterior_door();
}

void _start_subobject_pulsing_effect(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_subobject_pulsing_effect(args.raw->slots[0].i);
}

void _resume_hero_state_process(void) {
    resume_hero_state_process();
}

void _suspend_hero_state_process(void) {
    suspend_hero_state_process();
}

void _idle_hero_anim_proc(void) {
    idle_hero_anim_proc();
}

void _transition_hero_to_anim_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    transition_hero_to_anim_script(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _close_exterior_doors(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    close_exterior_doors(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _get_doors_for_exterior(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_doors_for_exterior();
}

void _get_building_id_for_exterior(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_building_id_for_exterior();
}

void _npc_set_ani_flags(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_ani_flags(args.raw->slots[0].i);
}

void _npc_blend_to_ani_string(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_blend_to_ani_string(args.raw->slots[0].i);
}

void _npc_set_gravity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_gravity(current_args, args.raw->slots[0].f);
}

void _remove_npc_list(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    remove_npc_list(args.raw->slots[0].i);
}

void _setup_interior_fighting_arena(void) {
    setup_interior_fighting_arena();
}

void _npc_get_his_flag_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = npc_get_his_flag_state(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_get_flag_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = npc_get_flag_state(args.raw->slots[0].i);
}

void _npc_set_his_flags(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_his_flags(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _npc_ignore_his_events(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_ignore_his_events(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_ignore_events(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_ignore_events(args.raw->slots[0].i);
}

void _npc_set_flags(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_flags(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_enable_event(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_enable_event(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_enable_his_event(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_enable_his_event(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _npc_set_dialog_anim(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_dialog_anim(args.raw->slots[0].i);
}

void _npc_turn_and_face_next_waypoint(void) {
    npc_turn_and_face_next_waypoint();
}

void _npc_turn_and_face_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_turn_and_face_player(args.raw->slots[0].i);
}

void _set_interaction_camera_script(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_interaction_camera_script(args.raw->slots[0].i);
}

void _npc_play_conversation_part(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_play_conversation_part(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _conversation_term(void) {
    conversation_term();
}

void _conversation_init(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    conversation_init(args.raw->slots[0].i);
}

void _konquest_end_npc_interaction(void) {
    konquest_end_npc_interaction();
}

void _konquest_end_npc_nis(void) {
    konquest_end_npc_nis();
}

void _konquest_start_npc_nis(void) {
    konquest_start_npc_nis();
}

void _start_character_separation_process(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    start_character_separation_process(current_args, args.raw->slots[0].f);
}

void _konquest_start_npc_interaction(void) {
    konquest_start_npc_interaction();
}

void _remove_widescreen_bars(void) {
    remove_widescreen_bars();
}

void _add_widescreen_bars(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_widescreen_bars(args.raw->slots[0].f);
}

void _npc_take_control_of_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_take_control_of_him(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _konquest_fade_hud(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_fade_hud(args.raw->slots[0].i);
}

void _konquest_hide_hud(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_hide_hud(args.raw->slots[0].i);
}

void _konquest_show_hud(void) {
    konquest_show_hud();
}

void _display_konquest_title(void) {
    display_konquest_title();
}

void _enable_attached_sound_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    enable_attached_sound_by_uid(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _attach_sound_to_object_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    attach_sound_to_object_by_uid(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[4].i, args.raw->slots[5].i, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _start_konquest_ambient_sounds(void) {
    start_konquest_ambient_sounds();
}

void _stop_konquest_ambient_sounds(void) {
    stop_konquest_ambient_sounds();
}

void _npc_set_my_movement_weight(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_my_movement_weight(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _npc_set_ani_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_ani_speed(current_args, args.raw->slots[0].f);
}

void _bgnd_start_sobj_uv_scroll_tbl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bgnd_start_sobj_uv_scroll_tbl(args.raw->slots[0].i);
}

void _npc_set_snap_to_ground(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_snap_to_ground(args.raw->slots[0].i);
}

void _npc_stop_goro_bone_match(void) {
    npc_stop_goro_bone_match();
}

void _npc_start_goro_bone_match(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_start_goro_bone_match(args.raw->slots[0].i);
}

void _npc_ani_for_x_ticks(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_ani_for_x_ticks(args.raw->slots[0].i);
}

void _npc_ani_to_blend_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_ani_to_blend_frame(current_args, args.raw->slots[0].f);
}

void _start_time_passing(void) {
    start_time_passing();
}

void _stop_time_passing(void) {
    stop_time_passing();
}

void _get_tile_sobj_by_id(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = get_tile_sobj_by_id(args.raw->slots[0].i);
}

void _get_konquest_tile_objects_obj(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_konquest_tile_objects_obj();
}

void _get_current_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_current_time(args.raw->slots[0].pointer);
}

void _set_current_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_current_time(args.raw->slots[0].i);
}

void _npc_restart_his_normal_behavior(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_restart_his_normal_behavior(args.raw->slots[0].i);
}

void _npc_set_his_ang_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_his_ang_y(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _npc_set_my_ang_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_my_ang_y(current_args, args.raw->slots[0].f);
}

void _npc_set_his_world_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_his_world_pos(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _npc_set_my_world_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_my_world_pos(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _npc_set_my_pos(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_set_my_pos(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _restore_collision_volume_on_object_with_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    restore_collision_volume_on_object_with_uid(args.raw->slots[0].i);
}

void _remove_collision_volume_on_object_with_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    remove_collision_volume_on_object_with_uid(args.raw->slots[0].i);
}

void _restore_collision_volume_on_object(void) {
    restore_collision_volume_on_object();
}

void _remove_collision_volume_on_object(void) {
    remove_collision_volume_on_object();
}

void _npc_ani_to_frame_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_ani_to_frame_x(current_args, args.raw->slots[0].f);
}

void _konquest_transition_to_fight(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_transition_to_fight(args.raw->slots[0].i);
}

void _attach_wiff_to_konquest_object_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    attach_wiff_to_konquest_object_by_uid(args.raw->slots[0].i, get_script_string_arg(2), current_args, args.raw->slots[2].f);
}

void _set_konquest_object_render_order_priority_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_konquest_object_render_order_priority_by_uid(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _disable_konquest_object_zwrite_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    disable_konquest_object_zwrite_by_uid(args.raw->slots[0].i);
}

void _set_konquest_object_face_y_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_konquest_object_face_y_by_uid(args.raw->slots[0].i);
}

void _unhide_konquest_object_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    unhide_konquest_object_by_uid(args.raw->slots[0].i);
}

void _hide_konquest_object_by_uid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hide_konquest_object_by_uid(args.raw->slots[0].i);
}

void _konquest_fade_from_black(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_fade_from_black(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _konquest_fade_to_black(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    konquest_fade_to_black(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_lip_synch(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_lip_synch(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_ani_to_end(void) {
    npc_ani_to_end();
}

void _npc_wait_for_state_change(void) {
    npc_wait_for_state_change();
}

void _npc_blend_to_ani_with_offset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_blend_to_ani_with_offset(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _npc_blend_to_ani(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_blend_to_ani(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _npc_face_current_waypoint_angle(void) {
    npc_face_current_waypoint_angle();
}

void _npc_travel_to_world_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_travel_to_world_position(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _npc_travel_path_anim_override(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_travel_path_anim_override(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _npc_travel_path(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    npc_travel_path(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i);
}

void _npc_stand_still(void) {
    npc_stand_still();
}

void _add_npc_list_to_world(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_npc_list_to_world(args.raw->slots[0].i);
}

void _fire_trigger(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    fire_trigger(args.raw->slots[0].i);
}

void _set_monk_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_monk_position(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _load_tile_objects(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    load_tile_objects(args.raw->slots[0].i);
}

void _trigger_set_time_for_enable(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    trigger_set_time_for_enable(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i);
}

void _enable_trigger(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    enable_trigger(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _assign_obj_to_trigger(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    assign_obj_to_trigger(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _add_object_to_tile(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_object_to_tile(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, current_args, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f);
}

void _set_tile_visibility(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_tile_visibility(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _set_tile_grid_size(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_tile_grid_size(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_start_axis_indicator_p_axis_track_bone_world_mat(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_start_axis_indicator_p_axis_track_bone_world_mat(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs(void) {
    mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs();
}

void _mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside(void) {
    mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside();
}

void _mks_ccp1_insert_cb1(void) {
    mks_ccp1_insert_cb1();
}

void _mks_cc1_insert_cb1(void) {
    mks_cc1_insert_cb1();
}

void _mks_ccp1_eq_insert_cloth_coll_plane(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_ccp1_eq_insert_cloth_coll_plane(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _mks_cc1_expand_cyl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cc1_expand_cyl(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _mks_cc1_eq_insert_cloth_coll(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cc1_eq_insert_cloth_coll(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mks_cb1_set_scale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cb1_set_scale(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _mks_cb1_set_coll_offset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cb1_set_coll_offset(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _mks_cb1_set_coll_offset_xz(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cb1_set_coll_offset_xz(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _mks_cb1_add_coll_pt(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cb1_add_coll_pt(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _mks_cb1_set_ground_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cb1_set_ground_y(current_args, args.raw->slots[0].f);
}

void _mks_set_ground_y_all_cloth_bones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_set_ground_y_all_cloth_bones(current_args, args.raw->slots[0].f);
}

void _mks_insert_cloth_force_bones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_insert_cloth_force_bones(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _mks_cb2_eq_cloth_bone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cb2_eq_cloth_bone(args.raw->slots[0].i);
}

void _mks_cb1_eq_cloth_bone(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cb1_eq_cloth_bone(args.raw->slots[0].i);
}

void _mks_mat_id_set_zbias(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_mat_id_set_zbias(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _mks_cloth_bones_init_by_tbl(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_cloth_bones_init_by_tbl(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _mks_start_goro_arms_fixup(void) {
    mks_start_goro_arms_fixup();
}

void _mks_set_flipped_bones(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    mks_set_flipped_bones(args.raw->slots[0].pointer);
}

void _is_he_blocking(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = is_he_blocking();
}

void _set_ani_speed_miss_hit(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_ani_speed_miss_hit(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _set_block_requirement(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_block_requirement(args.raw->slots[0].i);
}

void _special_move_cam_end(void) {
    special_move_cam_end();
}

void _special_move_cam_setup(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    special_move_cam_setup(args.raw->slots[5].i, args.raw->slots[6].i,
                           args.raw->slots[7].i, args.raw->slots[0].f,
                           args.raw->slots[1].f, args.raw->slots[2].f,
                           args.raw->slots[3].f, args.raw->slots[4].f);
}

void _whoosh_fx(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    whoosh_fx(args.raw->slots[0].i);
}

void _drone_super_combo_refresh(void) {
    drone_super_combo_refresh();
}

void _set_attackers_attack_region(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_attackers_attack_region(args.raw->slots[0].i);
}

void _set_attack_type(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_attack_type(args.raw->slots[0].i);
}

void _got_hit_fx(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    got_hit_fx(args.raw->slots[0].i, args.raw->slots[1].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[4].i, args.raw->slots[6].i, args.raw->slots[5].f);
}

void _camera_set_animation_parent_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_animation_parent_position(
        (Vec*)args.raw->slots[0].pointer);
}

void _camera_set_animation_parent_angle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_animation_parent_angle(
        (const Vec*)args.raw->slots[0].pointer, args.raw->slots[1].i);
}

void _cam_set_ground_plane(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    cam_set_ground_plane(args.raw->slots[0].f);
}

void _set_camera_velocity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_camera_velocity((const Vec*)args.raw->slots[0].pointer);
}

void _get_camera_velocity(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_camera_velocity((Vec*)args.raw->slots[0].pointer);
}

void _set_camera_angle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_camera_angle((Vec*)args.raw->slots[0].pointer);
}

void _get_camera_angle(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_camera_angle((Vec*)args.raw->slots[0].pointer);
}

void _set_camera_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_camera_position((Vec*)args.raw->slots[0].pointer);
}

void _get_camera_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    get_camera_position((Vec*)args.raw->slots[0].pointer);
}

void _fade_from_white(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    fade_from_white(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _fade_to_white(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    fade_to_white(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _fade_from_black(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    fade_from_black(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _fade_to_black(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    fade_to_black(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _turn_camera_off(void) {
    turn_camera_off();
}

void _turn_camera_on(void) {
    turn_camera_on();
}

void _camera_run_animation(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_run_animation(args.raw->slots[0].i);
}

void _camera_set_speed_scalar(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_speed_scalar(args.raw->slots[0].f);
}

void _camera_get_victim(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer = camera_get_victim();
}

void _camera_set_victim(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_victim((MkObj*)args.raw->slots[0].pointer);
}

void _camera_get_attacker(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer = camera_get_attacker();
}

void _camera_set_attacker(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_attacker((MkObj*)args.raw->slots[0].pointer);
}

void _camera_special_function(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_special_function(args.raw->slots[0].i);
}

void _camera_setup_tightrope_angle_offset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_setup_tightrope_angle_offset(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _camera_setup_radial_position(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_setup_radial_position(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _camera_unpause_player(void) {
    camera_unpause_player();
}

void _camera_pause_player(void) {
    camera_pause_player();
}

void _camera_setup_radial_sweep(void) {
    ScriptArgsRef args;
    float sp8;

    args.bytes = current_args;
    sp8 = args.raw->slots[8].f;
    camera_setup_radial_sweep(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f, args.raw->slots[5].f, args.raw->slots[6].f, args.raw->slots[7].f, sp8);
}

void _camera_set_movement_focus_obj(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_movement_focus_obj((MkObj*)args.raw->slots[0].i);
}

void _camera_set_custom_camera_movement_flag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_custom_camera_movement_flag(args.raw->slots[0].i);
}

void _camera_set_radial_movement(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_radial_movement(args.raw->slots[0].i);
}

void _camera_set_center_of_rotation(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_center_of_rotation(
        (const CamVec3*)args.raw->slots[0].pointer);
}

void _camera_set_travel_time(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_travel_time(args.raw->slots[0].f);
}

void _camera_set_rotation_direction(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_rotation_direction(args.raw->slots[0].i);
}

void _camera_set_final_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_final_speed(args.raw->slots[0].f);
}

void _camera_set_initial_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_initial_speed(args.raw->slots[0].f);
}

void _camera_set_rotation_rate(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_rotation_rate(args.raw->slots[0].f);
}

void _camera_set_movement_rate(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_movement_rate(args.raw->slots[0].f);
}

void _camera_get_mirror_flag(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = camera_get_mirror_flag();
}

void _camera_set_check_konquest_collisions_flag(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_check_konquest_collisions_flag(args.raw->slots[0].i);
}

void _camera_set_glitch_flag(void) {
    camera_set_glitch_flag();
}

void _camera_set_look_mode(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_look_mode(args.raw->slots[0].i);
}

void _camera_set_movement_mode(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_set_movement_mode(args.raw->slots[0].i);
}

void _camera_wait_for_pos_move_done(void) {
    camera_wait_for_pos_move_done();
}

void _camera_wait_for_ang_move_done(void) {
    camera_wait_for_ang_move_done();
}

void _camera_wait_for_pos_and_ang_move_done(void) {
    camera_wait_for_pos_and_ang_move_done();
}

void _find_best_conversation_camera_position(void) {
    find_best_conversation_camera_position();
}

void _camera_check_reverse_move_offset(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    camera_check_reverse_move_offset(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _camera_is_pos_move_done(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = camera_is_pos_move_done();
}

void _camera_is_ang_move_done(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = camera_is_ang_move_done();
}

void _camera_reset_ang_done_flag(void) {
    camera_reset_ang_done_flag();
}

void _camera_reset_pos_done_flag(void) {
    camera_reset_pos_done_flag();
}

void _init_scripted_camera(void) {
    init_scripted_camera();
}

void _get_intro_camera_path(void) {
    ((ScriptRawResult*)active_cmdscript)->value.pointer = get_intro_camera_path();
}

void _ani_to_frame_x_col(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_to_frame_x_col(args.raw->slots[1].i, args.raw->slots[4].i, args.raw->slots[6].i, args.raw->slots[0].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[5].f);
}

void _if_collision_autoface_him(void) {
    if_collision_autoface_him();
}

void _if_collision_autoface_me(void) {
    if_collision_autoface_me();
}

void _bulvan_function(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    bulvan_function(args.raw->slots[0].i);
}

void _set_his_damage_multiplier(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_his_damage_multiplier(current_args, args.raw->slots[0].f);
}

void _adjust_his_damage_multiplier(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    adjust_his_damage_multiplier(current_args, args.raw->slots[0].f);
}

void _adjust_my_damage_multiplier(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    adjust_my_damage_multiplier(current_args, args.raw->slots[0].f);
}

void _advance_my_moveset(void) {
    advance_my_moveset();
}

void _disable_my_attacks(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    disable_my_attacks(args.raw->slots[0].i);
}

void _wall_eligible_off(void) {
    wall_eligible_off();
}

void _wall_eligible_on(void) {
    wall_eligible_on();
}

void _face_bleed_me(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    face_bleed_me(args.raw->slots[0].i);
}

void _air_collision_pause(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    air_collision_pause(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _player_impale(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    player_impale(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _plyr_weapon_grab(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    plyr_weapon_grab(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _weapon_trail_off(void) {
    weapon_trail_off();
}

void _weapon_trail_on(void) {
    weapon_trail_on();
}

void _set_ani_weight(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_ani_weight(current_args, args.raw->slots[0].f);
}

void _front_rollup_check(void) {
    front_rollup_check();
}

void _disable_joy_temp(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    disable_joy_temp(args.raw->slots[0].i);
}

void _disable_blocking(void) {
    disable_blocking();
}

void _enable_his_blocking(void) {
    enable_his_blocking();
}

void _match_my_ypos_with_his(void) {
    match_my_ypos_with_his();
}

void _slamdown_reaction_max_hit_rules(void) {
    slamdown_reaction_max_hit_rules();
}

void _popup_reaction_max_hit_rules(void) {
    popup_reaction_max_hit_rules();
}

void _back_rollup_check_reverse(void) {
    back_rollup_check_reverse();
}

void _back_rollup_check(void) {
    back_rollup_check();
}

void _force_forward(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    force_forward(args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[0].f, args.raw->slots[2].f);
}

void _ejb_call(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ejb_call(args.raw->slots[0].i);
}

void _myvel_my_angle_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    myvel_my_angle_y(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _rotate_towards_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    rotate_towards_him(args.raw->slots[0].f);
}

void _disable_both_repel_flags(void) {
    disable_both_repel_flags();
}

void _wait_to_land(void) {
    wait_to_land();
}

void _player_feet_land_chores(void) {
    player_feet_land_chores();
}

void _set_my_float_1(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_my_float_1(current_args, args.raw->slots[0].f);
}

void _release_other_player(void) {
    release_other_player();
}

void _ejb_too_close_repell(void) {
    ejb_too_close_repell();
}

void _ejb_release_other_player(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ejb_release_other_player(args.raw->slots[0].i);
}

void _damage_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    damage_him(current_args, args.raw->slots[0].f);
}

void _damage_me(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    damage_me(current_args, args.raw->slots[0].f);
}

void _enable_all_my_blocking(void) {
    enable_all_my_blocking();
}

void _turn_me_pi(void) {
    turn_me_pi();
}

void _disable_this_move_exec(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    disable_this_move_exec(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _set_both_face_opponent_flags(void) {
    set_both_face_opponent_flags();
}

void _clear_both_face_opponent_flags(void) {
    clear_both_face_opponent_flags();
}

void _clear_my_face_opponent_flag(void) {
    clear_my_face_opponent_flag();
}

void _super_charge_me(void) {
    super_charge_me();
}

void _step_throw_outof_retract(void) {
    step_throw_outof_retract();
}

void _step_throw_into_check(void) {
    step_throw_into_check();
}

void _add_facial_damage(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    add_facial_damage(current_args, args.raw->slots[0].f);
}

void _random_voice_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    random_voice_him(args.raw->slots[0].i);
}

void _set_my_damage_multiplier(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_my_damage_multiplier(current_args, args.raw->slots[0].f);
}

void _disable_attack5(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    disable_attack5(args.raw->slots[0].i);
}

void _special_move_cam_him(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    special_move_cam_him(args.raw->slots[5].i, args.raw->slots[6].i, args.raw->slots[7].i, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f, args.raw->slots[4].f);
}

void _plyr_rotate_obj_y180(void) {
    plyr_rotate_obj_y180();
}

void _check_for_combo_message(void) {
    check_for_combo_message();
}

void _myvel_his_angle_y(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    myvel_his_angle_y(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _stop_me(void) {
    stop_me();
}

void _myvel_his_angle_y_inout(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    myvel_his_angle_y_inout(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _wait_for_slot_load(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    wait_for_slot_load(args.raw->slots[0].i);
}

void _get_mode_of_play(void) {
    ((ScriptRawResult*)active_cmdscript)->value.i = get_mode_of_play();
}

void _destroy_mkprocs_pid(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    destroy_mkprocs_pid(args.raw->slots[0].i);
}

void _kill_lip_sync_procs(void) {
    kill_lip_sync_procs();
}

void _plyr_snd_req(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = plyr_snd_req(args.raw->slots[0].i);
}

void _random_hit(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = random_hit(args.raw->slots[0].i);
}

void _random_hit_n_voice(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    random_hit_n_voice(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _shake_hit_voice(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    shake_hit_voice(args.raw->slots[0].i, args.raw->slots[2].i, args.raw->slots[3].i, args.raw->slots[1].f);
}

void _pan_vol_pitch_random_snd_req(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = pan_vol_pitch_random_snd_req(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _snd_stop(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    snd_stop(args.raw->slots[0].i);
}

void _random_foot(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = random_foot(args.raw->slots[0].i);
}

void _random_voice(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = random_voice(args.raw->slots[0].i);
}

void _pan_vol_snd_req(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = pan_vol_snd_req(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _pan_vol_pitch_random_hit(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = pan_vol_pitch_random_hit(args.raw->slots[0].i, current_args, args.raw->slots[1].f, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _random_snd_req_delay(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    random_snd_req_delay(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _random_snd_req(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = random_snd_req(args.raw->slots[0].i);
}

void _set_snd_vol(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_snd_vol(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f);
}

void _snd_req_vol(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = snd_req_vol(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _snd_req(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ((ScriptRawResult*)active_cmdscript)->value.i = snd_req(args.raw->slots[0].i);
}

void _ani_to_fall_to_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_to_fall_to_frame(args.raw->slots[1].i, current_args, args.raw->slots[0].f, args.raw->slots[2].f);
}

void _shake_camera(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    shake_camera(args.raw->slots[0].i, current_args, args.raw->slots[1].f);
}

void _force_away(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    force_away(args.raw->slots[1].i, args.raw->slots[3].i, args.raw->slots[0].f, args.raw->slots[2].f);
}

void _play_sound_2(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    play_sound_2(args.raw->slots[0].i, args.raw->slots[1].i);
}

void _ani_through_end(void) {
    ani_through_end();
}

void _ani_to_end(void) {
    ani_to_end();
}

void _face_opponent_180(void) {
    face_opponent_180();
}

void _face_opponent_now(void) {
    face_opponent_now();
}

void _hit_START_chores(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    hit_START_chores(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _land_chores(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    land_chores(args.raw->slots[0].i, args.raw->slots[1].i, current_args, args.raw->slots[2].f, args.raw->slots[3].f);
}

void _launch_me_up(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    launch_me_up(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _tightrope_restrictions_off(void) {
    tightrope_restrictions_off();
}

void _tightrope_restrictions_on(void) {
    tightrope_restrictions_on();
}

void _init_3d_move_no_aniproc(void) {
    init_3d_move_no_aniproc();
}

void _init_3d_move_no_face(void) {
    init_3d_move_no_face();
}

void _init_3d_move(void) {
    init_3d_move();
}

void _end_air_move(void) {
    end_air_move();
}

void _init_air_move_no_aniproc(void) {
    init_air_move_no_aniproc();
}

void _init_air_move(void) {
    init_air_move();
}

void _init_still_move(void) {
    init_still_move();
}

void _init_ground_move_no_aniproc(void) {
    init_ground_move_no_aniproc();
}

void _init_ground_move(void) {
    init_ground_move();
}

void _init_move(void) {
    init_move();
}

void _set_ani_speed(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_ani_speed(current_args, args.raw->slots[0].f);
}

void _set_my_secondary_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_my_secondary_state(args.raw->slots[0].i);
}

void _set_my_state(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    set_my_state(args.raw->slots[0].i);
}

void _back_to_normal(void) {
    back_to_normal();
}

void _if_collision_slow_ani_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    if_collision_slow_ani_x(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _slow_ani_end(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    slow_ani_end(current_args, args.raw->slots[0].f);
}

void _slow_ani_x_if_miss(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    slow_ani_x_if_miss(current_args, args.raw->slots[0].f, args.raw->slots[1].f, args.raw->slots[2].f);
}

void _slow_ani_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    slow_ani_x(current_args, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _ani_to_blend_frame(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_to_blend_frame(current_args, args.raw->slots[0].f);
}

void _ani_to_frame_x(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_to_frame_x(current_args, args.raw->slots[0].f);
}

void _ani_to_frame_x_aniproc(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_to_frame_x_aniproc(current_args, args.raw->slots[0].f);
}

void _ani_to_frame_sound(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    ani_to_frame_sound(args.raw->slots[2].i, args.raw->slots[0].f, args.raw->slots[1].f);
}

void _blend_to_stance(void) {
    ScriptArgsRef args;

    args.bytes = current_args;
    blend_to_stance(args.raw->slots[0].f);
}
