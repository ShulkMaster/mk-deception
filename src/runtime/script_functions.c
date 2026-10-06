/* BUILD: -O4,s -use_lmw_stmw on object-wide: replaces per-wrapper optimize_for_size/use_lmw_stmw
 * pragmas; every exact wrapper stays exact and three flag getters improve. */
#include "game/game.h"
#include "runtime/anim_api_ext.h"
#include "game/ncs.h"
#include "platform/display.h"
#include "game/pz_fighters.h"
#include "runtime/bone_matcher.h"
#include "runtime/mk_obj_bone.h"
#include "runtime/anim_types.h"
#include "runtime/mk_obj_lists.h"
#include "game/pfxscript.h"
#include "game/ai.h"
#include "game/blood.h"
#include "game/cloth_wind.h"
#include "game/weapon.h"
#include "game/pz_fatality.h"
#include "game/projectile.h"
#include "game/konquest.h"
#include "game/ejb.h"
#include "game/constrain.h"
#include "game/bgnd.h"
#include "game/jdn.h"
#include "game/jab.h"
#include "game/jmt.h"
#include "game/mab.h"
#include "game/plyr.h"
#include "game/game_info.h"
#include "math/gxVect.h"
#include "runtime/limb.h"
#include "runtime/cam.h"
#include "runtime/cam_shake.h"
#include "runtime/mk_vtbl.h"
#include "runtime/asset.h"
extern unsigned char* current_args;
extern unsigned char* active_cmdscript;
extern unsigned char* plyr_obj;
extern unsigned char* plyr_anim_pdata;
extern PlyrPdata* his_pdata;
extern PuzzleFightersEngine g_pz_fighters_engine;

struct FakeBoneMatcher;
struct MkFlippedBoneMap;
struct MovesAttackInfo;
struct PebbleData;
struct PuiItem;
struct FenceSection;
struct SObj;

struct ScriptFlagWordView {
    MkHdr hdr;
    unsigned int flags;
};

struct ScriptFlagArgs {
    unsigned int header;
    struct ScriptFlagWordView* object;
    int bit;
    int enabled;
};

struct GetLimbObjArgs {
    unsigned int header;
    LimbRuntime* runtime;
    unsigned int bone_index;
};

struct ScriptPointerResult {
    char pad00[0x2C];
    MkHdr* value;
};

struct ScriptStringResult {
    char pad00[0x2C];
    char* value;
};

struct ScriptVecResult {
    char pad00[0x2C];
    Vec* value;
};

struct ScriptProcResult {
    char pad00[0x2C];
    MkProc* value;
};

struct ScriptSobjResult {
    char pad00[0x2C];
    struct SObj* value;
};

struct ScriptMkObjResult {
    char pad00[0x2C];
    MkObj* value;
};

typedef void (*ScriptEntryFn)(void);

float p_blend_to_stance_in_10(void);
float p_reverse_to_stance_in_10(void);
float p_blend_to_fstance_in_10(void);
float p_chamber_to_stance(void);
float p_chamber_to_stance_2(void);
float j_getup_back_3(void);
float j_getup_back_6(void);
float j_getup_back_9(void);
float j_getup_back_12(void);
float j_getup_front_12(void);
float j_getup_front_4(void);
float j_getup_sit_12(void);
float j_getup_sit_6(void);
float j_blend_to_stance_in_x(void);
float j_blend_to_fstance_in_x(void);
float j_sleep_forever(void);
float j_call_player_script_function(void);
float j_getup_front_6(void);
float j_getup_front_10(void);
float joy_dash_back(void);

static MkProcEntryFn exit_table[22] = {
    p_blend_to_stance_in_10,
    p_reverse_to_stance_in_10,
    p_blend_to_fstance_in_10,
    p_chamber_to_stance,
    p_chamber_to_stance_2,
    p_comboexit_to_stance,
    j_exit,
    j_getup_back_3,
    j_getup_back_6,
    j_getup_back_9,
    j_getup_back_12,
    j_getup_front_12,
    j_getup_front_4,
    j_getup_sit_12,
    j_getup_sit_6,
    j_blend_to_stance_in_x,
    j_blend_to_fstance_in_x,
    j_sleep_forever,
    j_call_player_script_function,
    j_getup_front_6,
    j_getup_front_10,
    joy_dash_back,
};
extern MkProcEntryFn script_callable_function_table[];

struct ExitFloatIntArgs {
    unsigned int header;
    unsigned int exit_index;
    float float_value;
    int int_value;
};

struct TrialRequiredAttackArgs {
    unsigned int header;
    int attack;
    int count;
};

struct BackgroundColorArgs {
    unsigned int header;
    int red;
    int green;
    int blue;
    int alpha;
};

struct ScriptSingleIntArgs {
    unsigned int header;
    int value;
};

struct ScriptSingleFloatArgs {
    unsigned int header;
    float value;
};

struct ScriptPointerArgs {
    unsigned int header;
    void* value;
};

struct ScriptFloatPointerArgs {
    unsigned int header;
    float* value;
};

struct ScriptSobjArgs {
    unsigned int header;
    struct SObj* value;
};

struct ScriptTwoPointerArgs {
    unsigned int header;
    void* first;
    void* second;
};

struct ScriptTwoIntArgs {
    unsigned int header;
    int first;
    int second;
};

struct ScriptThreeIntArgs {
    unsigned int header;
    int first;
    int second;
    int third;
};

struct ScriptMorphArgs {
    unsigned int header;
    MkObj* object;
    int sobj_id;
    int script_id;
    unsigned int flags;
};

struct ScriptObjectArgs {
    unsigned int header;
    MkHdr* object;
};

struct ScriptMkObjArgs {
    unsigned int header;
    MkObj* object;
};

struct ScriptAirborneArgs {
    unsigned int header;
    MkObj* object;
    PlyrPdata* player;
};

struct ScriptFenceArgs {
    unsigned int header;
    const Vec* point;
    const struct FenceSection* sections;
    int start_index;
    int mirrored;
};

struct ScriptFishAttackArgs {
    unsigned int header;
    MkHdr* player;
    int direction;
    int flags;
};

struct ScriptAxisArgs {
    unsigned int header;
    MkHdr* player;
    int axis;
};

struct ScriptLightningArgs {
    unsigned int header;
    PlyrInfo* owner;
    Vec* position;
};

struct ScriptSobjAlphaArgs {
    unsigned int header;
    MkObj* object;
    int sobj_index;
    int alpha;
};

struct ScriptObjectIntArgs {
    unsigned int header;
    MkObj* object;
    int value;
};

struct ScriptVolumeArgs {
    unsigned int header;
    const Vec* position;
    float far_distance;
    float near_distance;
};

struct ScriptBodyExplodeArgs {
    unsigned int header;
    MkHdr* player;
    const Vec* position;
    float velocity;
};

struct ScriptFishFlagArgs {
    unsigned int header;
    void* entries;
    int hidden;
    int count;
};

struct ScriptLightListArgs {
    unsigned int header;
    MkObj* object;
    LightDef* light;
};

struct ScriptPebbleVecArgs {
    unsigned int header;
    int player;
    int index;
    const Vec* value;
};

struct ScriptPebbleBounceArgs {
    unsigned int header;
    int player;
    int index;
    const Vec* velocity;
    int flags;
};

struct ScriptMkObjFloatArgs {
    unsigned int header;
    MkObj* object;
    float value;
};

struct ScriptMkObjVecArgs {
    unsigned int header;
    MkObj* object;
    float x;
    float y;
    float z;
};

struct ScriptSkytempleExplodeArgs {
    unsigned int header;
    int player;
    float x;
    float y;
    float z;
};

struct ScriptPebbleArrangeArgs {
    unsigned int header;
    int player;
    unsigned int count;
    Vec* position;
};

struct ScriptPebbleVelocityArgs {
    unsigned int header;
    int player;
    unsigned int count;
    float x;
    float y;
    float z;
};

struct ScriptNamedSobjFxArgs {
    unsigned int header;
    unsigned int string_id;
    int sobj_id;
    float y_offset;
};

struct ScriptSwapTextureTableArgs {
    unsigned int header;
    const BgndSwapTextureEntry* entries;
    int frame;
};

struct ScriptSwapTextureArgs {
    unsigned int header;
    int sobj_id;
    int material_id;
    int frame;
};

struct ScriptAppendTextureTableArgs {
    unsigned int header;
    const BgndAppendTextureEntry* entries;
};

struct ScriptAppendTextureArgs {
    unsigned int header;
    int sobj_id;
    int material_id;
    unsigned int string_id;
    int texture_slot;
};

struct ScriptLightColorArgs {
    unsigned int header;
    int light_id;
    float red;
    float green;
    float blue;
};

struct ScriptIntFloatArgs {
    unsigned int header;
    int integer;
    float floating;
};

struct ScriptVecValueArgs {
    unsigned int header;
    float x;
    float y;
    float z;
};

struct ScriptVecPointerArgs {
    unsigned int header;
    const Vec* vector;
};

struct ScriptEntryArgs {
    unsigned int header;
    ScriptEntryFn entry;
};

struct ScriptReactionArgs {
    unsigned int header;
    int reaction;
    float rate;
    int strength;
};

struct ScriptStartProjectileArgs {
    unsigned int header;
    int bone_id;
    MkObj* existing_object;
    unsigned int string_id;
    float speed;
    float tolerance;
    Vec* bone_offset;
};

struct ScriptPlyrPdataArgs {
    unsigned int header;
    PlyrPdata* player;
};

struct ScriptPlyrEntryArgs {
    unsigned int header;
    PlyrPdata* player;
    int entry;
};

struct ScriptEntryPlayerArgs {
    unsigned int header;
    int entry;
    int player;
};

struct ScriptResumeEffectArgs {
    unsigned int header;
    int player;
    int bone_id;
    int effect_handle;
    int bind_mode;
    int blood_required;
};

struct ScriptLoadPlayerModelArgs {
    unsigned int header;
    unsigned int string_id;
    int player;
    int object_type;
    int flags;
};

struct ScriptLoadSlotModelArgs {
    unsigned int header;
    int slot;
    unsigned int string_id;
    int flags;
    int user_data;
};

struct PzConstrainArgs {
    unsigned int header;
    int mode;
    float value;
};

struct ScriptAttackArgs {
    unsigned int header;
    int animation_id;
    union {
        PuzzleAttackParameters* puzzle;
        struct MovesAttackInfo* standard;
    } parameters;
    int arg3;
};

struct ScriptAnimationArgs {
    unsigned int header;
    int animation_id;
    int flags;
    float frame;
    float blend;
};

struct ScriptThreeVecArgs {
    unsigned int header;
    Vec* out;
    const Vec* first;
    const Vec* second;
};

struct ScriptCircleArgs {
    unsigned int header;
    float* center;
    float radius;
    float angle;
    float* out;
};

struct ScriptCameraRectangleArgs {
    unsigned int header;
    MkSobj* object;
    const Vec* center;
    float min_x;
    float min_z;
    float max_x;
    float max_z;
};

struct ScriptCameraCylinderArgs {
    unsigned int header;
    MkSobj* object;
    const Vec* center;
    float radius;
    float height;
};

struct FakeBoneMatcherResetArgs {
    unsigned int header;
    struct FakeBoneMatcher* matcher;
    const Vec* parent_offset;
    const Vec* child_offset;
    const Vec* rotation;
    int bone_index;
    MkHdr* object;
    float blend;
};

struct FakeBoneMatcherArgs {
    unsigned int header;
    MkHdr* parent;
    MkHdr* child;
    int bone_index;
    const Vec* parent_offset;
    const Vec* child_offset;
    const Vec* rotation;
    int mode;
    float blend;
};

struct PopHeadArgs {
    unsigned int header;
    PlyrInfo* player;
    float x_velocity;
    float y_velocity;
    float z_velocity;
    float angular_velocity;
};

struct LimbBoneAttachArgs {
    unsigned int header;
    PlyrInfo* target_player;
    int owner_bone;
    const Vec* offset;
    const Vec* rotation;
    PlyrInfo* owner_player;
    int limb;
    int target_bone;
    int include_children;
};

struct Gore2PebbleArgs {
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
};

struct ScriptIntResult {
    char pad00[0x2C];
    int value;
};

struct ScriptFloatResult {
    char pad00[0x2C];
    float value;
};

struct ScriptCommandView {
    char pad00[0x20];
    int state;
    int move_attributes;
    int branch_target;
    int result;
    void* animation;
    void* secondary_animation;
    char pad38[0x174];
    ScriptEntryFn exit;
};

struct ScriptDistanceFuncDef {
    int name_offset;
    unsigned int code_offset;
    unsigned int attributes_id;
};

struct ScriptDistanceSlot {
    char pad00[0x74];
    struct ScriptDistanceFuncDef* functions;
    int string_relocation;
    char pad7C[0x0C];
    unsigned int* bytecode;
};

struct ScriptDistanceCommand {
    MkHdr hdr;
    struct ScriptDistanceSlot* slot;
    char* function_name;
    int argument_count;
    unsigned int* program_counter;
    void* previous_program_counter;
    unsigned int* argument_header;
    int state;
    void* attributes;
};

#define ACTIVE_DISTANCE_SCRIPT \
    ((struct ScriptDistanceCommand*)active_cmdscript)

struct ScriptExitView {
    char pad00[0x20];
    int state;
    char pad24[0x188];
    MkProcEntryFn exit;
};

struct ScriptExitArgs {
    unsigned int header;
    int exit_value;
    int exit_arg0;
    int input_unlock_tick;
    int blocking_tick;
    int exit_arg1;
    int exit_arg2;
};

#define CURRENT_EXIT_ARGS ((struct ScriptExitArgs*)current_args)
#define CURRENT_PLAYER_PDATA (plyr_pdata)
#define ACTIVE_SCRIPT_EXIT ((struct ScriptExitView*)active_cmdscript)

struct ScriptObjectRef {
    MkHdr* object;
    unsigned int instance;
};

struct ScriptGroundObjView {
    MkHdr header;
    unsigned char flags08;
    unsigned char launched : 1;
    unsigned char : 7;
};

struct ScriptActiveState {
    char pad00[8];
    ScriptSlot* state;
};

/* Partial external MkObj view: retail accesses its position at +0xA0 here. */
struct ScriptNpcCameraObjectView {
    char pad00[0xA0];
    Vec pos;
};

struct ScriptNpcBody {
    char pad00[0x0C];
    struct ScriptNpcCameraObjectView* camera_object;
};

struct ScriptNpcHandle {
    char pad00[0x14];
    struct ScriptNpcBody* body;
};

typedef struct KonquestNpc KonquestNpc;
typedef struct KonquestNpcData KonquestNpcData;

struct ScriptNpcCameraArgs {
    unsigned int header;
    KonquestNpcData* npc_data;
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
};

struct ScriptPlaceSlaveArgs {
    unsigned int header;
    int npc_id;
    int rope_model_index;
    unsigned int model_name_offset;
    int model_id;
    Vec anchor;
    float rope_length;
    Vec local_angles;
    Vec object_angles;
    float acceleration_divisor;
    float acceleration_scale;
    float field_8C;
};

struct ScriptPlaceWeaponArgs {
    unsigned int header;
    int primary_object_id;
    int secondary_object_id;
    int primary_sobj_id;
    int secondary_sobj_id;
    int paired;
    Vec primary_position;
    Vec primary_angles;
    Vec secondary_position;
    Vec secondary_angles;
    int pickup_sobj_id;
    float radius;
    float height;
    Vec collision_center;
    int permanent;
};

union ScriptRawArg {
    int i;
    unsigned int u;
    float f;
    void* pointer;
};

struct ScriptTransitionFrameArgs {
    unsigned int header;
    AnimPdata* animation;
    int animation_id;
    unsigned int flags;
    float transition_frames;
    float frame;
};

struct ScriptRawArgs {
    unsigned int header;
    union ScriptRawArg slots[16];
};

struct ScriptRawResult {
    char pad00[0x2C];
    union ScriptRawArg value;
};

union ScriptResultRef {
    unsigned char* bytes;
    struct ScriptCommandView* command;
};

extern float inverse_game_speed;
void nis_init(ScriptSlot* cmdscript, unsigned int scene_func, unsigned int cancel_func);
KonquestNpc* find_npc_by_data(KonquestNpcData* data);
void nb_place_slave_in_bgnd(
    int npc_id, int rope_model_index, const char* model_name, int model_id,
    float anchor_x, float anchor_y, float anchor_z, float rope_length,
    float local_angle_x, float local_angle_y, float local_angle_z,
    float object_angle_x, float object_angle_y, float object_angle_z,
    float acceleration_divisor, float acceleration_scale, float field_8C);

void stop_usec_timer(int timer);
void start_usec_timer(int timer);
int printf(const char* format, ...);
void obj_setup_for_animation(
    MkObj* object, const int* tags, struct MkFlippedBoneMap* flipped_bone_map,
    void* ground_colls);
void update_mkobj(void* object);
void* get_mkobj_frame(int id, int frame);
void trial_add_required_attack(unsigned char attack, unsigned char count,
                               int flags);
unsigned short randu0(unsigned short limit);
extern void** bgnd_animation_table;
extern void* pz_shared_ani[];
extern void* npc_fast_anims[];
extern void* konq_nis_anims[];
extern void* konquest_animations[];
extern void* shared_ani[];
void pz_fighter_set_y_constrain(MkObj* player_obj, int mode, float value);
void pz_fighter_attack(
    void* animation, PuzzleAttackParameters* attack, int reaction);
void attack_opponent_with(
    AniData* animation, struct MovesAttackInfo* attack, int reaction);
void advance_my_moveset(void);
float j_call_player_script_function(void);
int was_i_hit_x_times(int hit_count);
int was_button_and_direction(int button, int direction);
void glitch_to_ani_frame(void* animation, int flags,
                         struct ScriptAnimationArgs* args, float frame);
void blend_to_ani_frame(AniData* animation, int flags, float blend, float frame);
void glitch_to_ani(void* animation, int flags);
void reaction_xfer_him_nohit(void* entry);
void set_anim_hiframe(float frame);
int was_button_pressed(int button);
int am_i_airborn(void);
int is_his_chest_to_screen(void);
int is_my_chest_to_screen(void);
int am_i_on_the_left2(void* me, void* him);
int am_i_on_the_left(void);
int is_he_flipped(void);
int is_fast_getup(void);
int disable_impale_check(void);
void tag_team_activate_player(MkObj* object, int active);
void load_and_set_refl_on_weapon(void);
void advance_active_moveset(int amount);
int get_active_moveset_from_pdata(void* pdata);
void fx_transfer(int effect, int owner);
void bgnd_force_specularity_off_for_material(
    unsigned int object_id, unsigned int material_id);
void bgnd_sobj_set_texture_kl_values(
    unsigned int object_id, unsigned int material_id, int dual_texture,
    float l, int k);
char* get_script_string_arg(int argument);
void jab_attach_wiff_to_sobj(
    MkObj* object, int sobj_id, const char* wiff_name,
    const char* texture_name, int section, float rate, int frame);
void bgnd_chunk_explosion_match_velocity_with_params(
    float velocity_scale, float vertical_velocity,
    float random_vertical_velocity, char* shard_name, int bounce_limit,
    unsigned int spawn_count, unsigned int scale_mode, int motion_mode);
MkObj* ncs_bgnd_preload_named_model(
    const char* section_name, const char* model_name, int object_type,
    int transl, Vec* position, Vec* angles, Vec* scale);
void attach_pfx_to_object_by_uid(
    int object_uid, const char* effect_name, const Vec* offset, int flags);
void bgnd_create_named_npc_in_slot(
    int slot, const char* name, int object_id, int flags);
void bgnd_launch_fx_at_active_sobj_pos_with_offset(
    const char* effect_name, float x, float y, float z);
void bgnd_launch_fx_at_bid_of_mkobj(
    const char* effect_name, MkObj* object, int bone);
void bgnd_launch_fx_at_position(
    const char* effect_name, float x, float y, float z);
unsigned int bgnd_pfxhandle_spawn_at_bid(
    const char* effect_name, MkObj* object, int bone);
void general_flash_fx(
    int player, MkObj* object, const char* effect_name, int bone,
    int use_bone, float y_offset);
void mk_chess_launch_fx_at_active_piece_with_offset(
    const char* effect_name, float x, float y, float z);
unsigned int pfxhandle_bgnd_spawn_at_position(
    const char* effect_name, float x, float y, float z);
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
void fx_disable_ztest(int effect, int disable);
void fx_set_render_priority(int effect, int priority);
void fx_hide(int effect, int hide);
void fx_get_v3(int effect, int parameter, void* value);
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
struct PfxScriptColorRow;
void update_lerp_color(int color_field, int age_field, float duration,
                       int color_count, int first_color,
                       struct PfxScriptColorRow* table);
void update_fade_alpha2(int color_field, int age_field, float start_time,
                        float duration, int start_alpha, int end_alpha);
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
void fx_pause_emit(int effect);
void fx_resume_emit(unsigned int handle);
void reset_effect(void);
void spawn_color(int a, int b, int c, int d, int e);
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
void texture_animation(float horizontal_scale, int vertical_frames, float speed);
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
int fx2(int effect, char* name);
void set_vertex_color(int color);
void set_light(int light);
void set_aspect_ratio(float x, float y);
void set_bounding_radius(float radius);
void create_y_mirror_effect(int effect);
void z_bias(float value);
void particle_size(float value);
void face_y(void);
void set_decal_plane(float* plane);
void bind_to_bone(int bone);
void create_step_effect(int effect);
void parametric_update(const struct PfxParametricEffectDescription* value);
float dist_xz_to_xz(void* a, void* b);
void v3_to_xz_ang(Vec* out, Vec* value);
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
typedef struct BoneMatcherState BoneMatcherState;
void bone_matcher_child_set_offset(BoneMatcherState* matcher, Vec* offset);
void bone_matcher_parent_set_offset(BoneMatcherState* matcher, Vec* offset);
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
void nis_register_participant(int type, void* npc_data);
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
void xfer_spearproc_to_retract(MkProc* proc);
void destroy_spearproc_bonematcher(void* value);
void insert_mkobj_spearproc_parentobjitem(void* a, void* b);
MkObj* get_spearobj_from_spearproc(MkProc* proc);
void* fire_sc_spear(int a, int b, int c, int d, int e, int f);
void subzero_start_ice_chunks(int value);
void* subzero_start_iceman(void);
void* subzero_start_iceblock(void);
void sindel_scream_react_sound_start(void);
struct FatalityObjectLatch;
void sindel_sonic_sounds(struct FatalityObjectLatch* sound, int finished);
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
typedef struct FatalityEmitterBind FatalityEmitterBind;
void fire_multi_emitter_pfx_via_tbl(
    const char* name, FatalityEmitterBind* table, MkObj* object, int* handles);
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
void destroy_gore2_obj(unsigned int object_id, int particle_index);
int attach_gore2_obj(MkObj* owner, int bone, unsigned int object_id,
                     const Vec* offset, const Vec* rotation);
void start_bodyslam_bodysplat(float a, float b, float c, float d, float e);
void fatality_explode_victim(int a, float b, float c);
void kill_gusher(int a);
void start_sweat_particles_scripts(int a, int b);

void start_sweat_particles(int a, int b, int c, int d);
void* start_blood_particles(int a, int b, int c, int d);
void mks_spawn_blood_pool_at_bid(int a, int b, int c, int d);
void spawn_blood_pool_at_bid(int a, int b, int c);
void* plyr_weapon2_release(int a);
void* plyr_weapon_release(int a);
void bone_matcher_reset_dest_mat_rot(int a, int b);
void bone_matcher_set_ang_pos(int a, int b, int c, int d, int e, int f);
MkObj* weapon_bm_ignore(int weapon, int ignored);
void* regrab_weapon(int a, int b, int c, int d, int e, int f, int g);
void weapon_reflection_show_hide(PlyrPdata* player, int secondary, int hidden);
void* show_single_weapon(int a, int b);
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
void reset_fake_bone_matcher(struct FakeBoneMatcher* matcher,
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
MkProc* proc_of_anim_pdata(AnimPdata* anim);
void set_pdata_anim_step(void* anim, float step);
void plyr_turn_on_shadowbox(int a);
void plyr_turn_off_shadowbox(PlyrInfo* player);
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
void ncs_dkp_camera_konqchar_show_hide_alpha(int character_index, MkObj* character);
void* ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void);
void ncs_bgnd_nuke_collision_to_script_interface(void);
void* retrieve_bgnd_obj(void);
void fkbm_obj_face_obj(int a, int b, int c, int d, int e);
void start_obj_scalar_proc(int a, int b, int c, int d);
float mkobj_pos_pos_dot_normal_xz(int a, int b, int c);
int obj_get_bid_for_tid(MkObj* obj, int tag);
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
MkObj* find_obj_by_id(int a);
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
void bgnd_make_object_transl(unsigned int object_id);
int are_death_traps_on(void);
char* get_string_by_id(unsigned int id);
void ending_show_text(int script_index, int duration);
void ending_show_image(int image);
void midpoint_v3(Vec* out, const Vec* first, const Vec* second);
Vec* sobj_get_world_pos(struct SObj* object);
void get_point_on_circle(float* center, float radius, float angle, float* out);
MkHdr* cut_player_in_half(MkHdr* player);
int get_offset_of_closest_fence_section(const Vec* point,
                                        const struct FenceSection* sections,
                                        int start_index, int mirrored);
void start_fish_attack(MkHdr* player, int direction, int flags);
void debug_create_axis_indicator(MkHdr* player, int axis);
void bgnd_level_fatality_end(void);
void bgnd_level_fatality_start(int player);
void bgnd_level_transition_end(void);
void bgnd_level_transition_start(void);
void obj_set_sobj_alpha(MkObj* object, int sobj_index, int alpha);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
void mab_test(void);
void player_body_explode(MkHdr* player, const Vec* position, float velocity);
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
void sobj_disable_blending(struct SObj* object);
struct SObj* obj_first_sobj(MkObj* object);
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
void destroy_mkobj(void* object);
void obj_turn_gravity_off(void* object);
void obj_set_gravity(void* object, float gravity);
void insert_fgnd_mkobj(void* object);
int get_player_number(void* object);
float script_fabs(float value);
void set_obj_light_flags(MkObj* object, int flags);
void set_obj_ang(MkObj* object, float x, float y, float z);
void set_obj_pos(MkObj* object, float x, float y, float z);
int random_percent(float percent);
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
void scripted_camera_script_exit(void);
void bgnd_launch_fx_at_sobj_pos(
    const char* name, unsigned int sobj_id, float y_offset);
int bgnd_get_first_shape_center_for_obstacle_id(int obstacle_id,
                                                int scratch_index);
void misc_data_set_obj_ptr1(MkObj* object);
MkObj* misc_data_get_obj_ptr1(void);
void misc_data_set_col_obj_id2(int object_id);
int misc_data_get_col_obj_id2(void);
void misc_data_set_col_obj_id(int object_id);
int misc_data_get_col_obj_id(void);
MkObj* bgnd_fetch_obj(int model_id);
float bgnd_get_float(int value_id);
unsigned int bgnd_get_u32(int value_id);
int bgnd_get_int(int value_id);
void mks_start_fatality_iceball(int player);
void mks_debug_display_cloth_ontop(int enabled);
void start_bow(int player, float strength);
void start_plyr_attack(float radius);
void set_active_projectile_rx_info(int reaction, float rate, int strength);
void run_reaction_cleanup_function(PlyrPdata* player);
int reaction_xfer_him(int reaction, float rate, int strength);
void mks_set_cb1_wind_normal(float x, float y, float z);
void mks_npc_build_bones_tbl(int model_id, const int* bone_tags);
void mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
    PlyrPdata* player);
void mks_xfer_collision_info_plyr_to_bgnd_script(PlyrPdata* player,
                                                  int script_function);
void mks_xfer_collision_info_plyr_to_script(int script_function, int player);
void resume_effect_at_plyr_num_bid(
    int player, int bone_id, unsigned int effect_handle,
    int bind_mode, int blood_required);

/* Typed declarations used by imported script wrappers. */
int add_facial_damage(void *, float);
int add_npc_list_to_world(int);
void add_trigger_list_to_world(void);
int ani_1_frame(void);
int ani_no_pos(void);
int ani_through_end(void);
void ani_to_blend_frame(float);
int ani_to_end(void);
void ani_to_frame_x(float);
int ani_with_pos(void);
int ani_x_more_frames(void *, float);
int auto_ani_on(void);
int back_rollup_check(void);
int back_rollup_check_reverse(void);
void bgnd_active_sobj_no_ztest(void);
void bgnd_active_sobj_no_zwrite(void);
int bgnd_allow_dirty_floor(void);
void bgnd_always_face_y(unsigned int);
int bgnd_clean_beetlelair(void);
int bgnd_clean_slaughterhouse(void);
int bgnd_clean_up_floor(void);
void bgnd_collision_if_rx_override(unsigned int);
int bgnd_collison_if_set_info(void);
int bgnd_collison_if_to_scripts_activate(void);
int bgnd_create_sobjs(void);
int bgnd_delete_danger_zone(int);
void bgnd_delete_proc_by_id(int);
int bgnd_detach_rope(int);
void bgnd_enable_wall_hider(unsigned int);
int bgnd_end_the_game_and_restart(void);
void bgnd_get_active_sobj_pos(Vec*);
void bgnd_hide_active_sobj(void);
int bgnd_hide_mirror_guys(void);
void bgnd_hide_pebbles(int);
void bgnd_hide_preload_obj(int);
void bgnd_hide_sobj(unsigned int);
void bgnd_hide_sobj_and_children(unsigned int);
void bgnd_init_all_uv_scroll_w_control(void);
int bgnd_init_cracks(void);
void bgnd_init_timers(int);
int bgnd_kill_all_launched_sobjs(void);
void bgnd_no_z_test(unsigned int);
void bgnd_no_z_write(unsigned int);
void bgnd_npc_like_plyr(unsigned int);
int bgnd_preload_obj_attach_rope(int);
int bgnd_reg_col_cb_for_beetle_lair(void);
int bgnd_remove_cracks(void);
int bgnd_reset_players_animation_height(void);
void bgnd_reset_sobj(int);
void bgnd_restore_player(void);
void bgnd_set_active_danger_zone(unsigned int);
void bgnd_set_active_sobj(int);
int bgnd_set_active_sobj_rop(int);
void bgnd_set_fx_ang_dir_to_i_vector(void);
void bgnd_set_fx_ang_y(void *, float);
int bgnd_set_viewing_of_danger_zones(int);
void bgnd_setup_rx_handler(int);
int bgnd_sh_level_1(void);
int bgnd_sh_level_2(void);
void bgnd_shadow_control(unsigned int);
int bgnd_start_cracks(void);
void bgnd_swap_level(int);
int bgnd_turn_off_backface_culling(int);
int bgnd_turn_on_backface_culling(int);
void bgnd_unhide_active_sobj(void);
int bgnd_unhide_mirror_guys(void);
void bgnd_unhide_pebbles(int);
void bgnd_unhide_preload_obj(int);
void bgnd_unhide_sobj(unsigned int);
void bgnd_unhide_sobj_and_children(unsigned int);
void bgnd_update_active_mksobj(void);
int blast_effect_at_plyr(void);
int bulvan_function(int);
void change_monk_age(int);
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
void disable_mileena_collisions(int);
int disable_my_attacks(int);
void display_konquest_title(void);
void display_time_progression_images(int);
int dk_taunt_at_screen(void);
void dont_fence_plyr_in(int);
void drone_ai_increase_big_boss_stage(PlyrPdata* victim);
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
typedef struct KonquestTriggerDefinition KonquestTriggerDefinition;
void fire_trigger(KonquestTriggerDefinition*);
int flash_hit_at_bid(int);
int force_ai_style(int);
void forced_step_forward(void);
int freeze_player(void);
int front_rollup_check(void);
void give_reward_to_player(KonquestPuiDefinition*);
int gusher_destroy_list(void);
int head_tracking_off(void);
int hero_handle_conversation(void);
int hero_stop_moving(void);
int hero_turn_to_face_position(int);
int high_flash_check(void);
int idle_hero_anim_proc(void);
void idle_his_anim_proc(void);
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
int jab_stop_dragon_king_shake(void);
void kabal_collision_control_victim(int);
void kenshi_teleport_position(void);
void kick_the_camera(void);
void kill_dynamic_pui(void*);
int kill_konquest_dialog_procs(void);
int kill_lip_sync_procs(void);
void kobra_teleport_position(void);
int konquest_camera_return_to_normal(void);
void konquest_end_npc_interaction(void);
void konquest_end_npc_nis(void);
int konquest_fade_hud(int);
void konquest_hero_portal_in(void);
void konquest_hide_damashi(void);
int konquest_hide_hud(int);
int konquest_run_ending(void);
void konquest_set_current_portal_uid(int);
int konquest_show_hud(void);
void konquest_start_npc_interaction(void);
void konquest_start_npc_nis(void);
int konquest_transition_to_fight(int);
int load_tile_objects(int);
int low_flash_check(void);
int match_my_ypos_with_his(void);
int medium_flash_check(void);
void mileena_sky_set_position(void);
int mini_mission_completed(int);
int mini_mission_inactive(int);
int mk_chess_activate_my_properties(void);
int mk_chess_air_move(void);
int mk_chess_ani_1_frame(void);
int mk_chess_ani_idle(void);
int mk_chess_ani_to_end(void);
void mk_chess_blend_into_cell_orgin_in_x_frames_by_caller(void);
int mk_chess_blend_to_normal_stance(void);
int mk_chess_deactivate_my_properties(void);
int mk_chess_dont_constrain_piece(void);
void mk_chess_load_chess_table(void*);
struct ChessSpellDefinition;
void mk_chess_make_spellcaster(const struct ChessSpellDefinition*);
int mk_chess_piece_die(void);
void mk_chess_piece_event_from_script(int);
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
void mks_set_flipped_bones(struct MkFlippedBoneMap* bone_map);
void mks_start_goro_arms_fixup(void);
int nis_clear_event_list(void);
void nis_remove_non_participants(void);
int noob_victory_entrance(void);
int noobsmoke_sidekick_double_charge(void);
int noobsmoke_sidekick_projectile(void);
int npc_ani_1_frame(void);
int npc_ani_for_x_ticks(int);
int npc_ani_to_end(void);
int npc_ani_to_frame_x(void *, float);
int npc_blend_to_ani_string(int);
void npc_face_current_waypoint_angle(void);
void npc_fire_trigger(unsigned int);
int npc_hide_skip_message(void);
int npc_ignore_events(int);
int npc_open_door_at_waypoint(void);
int npc_play_random_dialog_sequence(void);
void npc_play_teleported_sound(void);
int npc_prepare_for_unconscious_state(void);
int npc_punch_reaction_standard_setup(void);
int npc_punch_reaction_standard_shutdown(void);
int npc_reset_my_timed_events(void);
int npc_restart_his_normal_behavior(int);
int npc_run_shove_animation(int);
int npc_set_ani_flags(int);
void npc_set_ani_frame(float);
void npc_set_ani_speed(float);
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
int play_background_music(int);
int play_beam_advance_sound(int);
int player_add_item_to_inventory(int);
int player_feet_land_chores(void);
int plyr_rotate_obj_y180(void);
int plyr_set_gravity(void *, float);
float popup_reaction_max_hit_rules(void);
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
void remove_collision_volume_on_object(void);
int remove_npc_list(int);
int remove_widescreen_bars(void);
void restore_collision_volume_on_object(void);
void restore_hero_grounding(void);
void resume_hero_state_process(void);
void retract_spear_from_camera(MkProc* proc);
void scorpion_teleport_position(void);
void set_age_progression(int);
void set_ani_speed(float);
void set_ani_weight(float);
int set_attackers_attack_region(int);
int set_block_requirement(int);
int set_both_face_opponent_flags(void);
int set_cliff_watcher_round(int);
void set_current_time(void*);
void set_hero_position_relative_to_chest(void);
void set_interaction_camera_script(void*);
int set_konquest_region_number(int);
void set_krypt_character_pos(Vec*);
void set_last_character_trained_with(int);
void set_look_at_npc(int);
int set_monk_age(int);
void set_movement_npc(int);
int set_my_float_1(void *, float);
int set_my_secondary_state(int);
void set_reference_pui(KonquestPuiDefinition*);
int setup_for_flip_ani(void);
int setup_interior_fighting_arena(void);
int setup_vomit_slip_sound(void);
int show_player(int);
int show_shujinko_unlock_screen(int);
float slamdown_reaction_max_hit_rules(void);
int slow_ani_end(void *, float);
int smoke_victory_entrance(void);
int snd_major_hit_voice(void);
int snd_stop(int);
void sobj_no_zwrite(void* sobj);
void spad_norm_vector(int);
void special_move_cam_end(void);
int start_baraka_blades_monitor(void);
int start_baraka_jaw_monitor(void);
void start_chunk_launch_monitor(void);
int start_constrain_proc(void);
int start_hero_collisions(void);
void start_kabal_smoke(void *, float);
int start_konquest_ambient_sounds(void);
int start_mini_mission(int);
int start_rope_proc(void);
int start_shadow_watcher(void);
int start_sobj_ctrl_proc(void);
void start_sobj_launch_monitor(void);
int start_special_weapon_monitor(int);
int start_subobject_pulsing_effect(int);
int start_time_passing(void);
int step_throw_into_check(void);
int step_throw_outof_retract(void);
void stop_chest_camera_script(void);
int stop_hero_collisions(void);
int stop_konquest_ambient_sounds(void);
int stop_me(void);
int stop_time_passing(void);
int stop_vomit_slip_sound(void);
int super_charge_me(void);
void suspend_hero_grounding(void);
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
void turn_to_face_exterior_door(void);
int turn_to_face_interior_door(void);
int unfreeze_player(void);
int update_my_last_switch(void);
int wait_for_slot_load(int);
int wait_to_land(void);
int wall_eligible_off(void);
int wall_eligible_on(void);
int weapon_trail_off(void);
int weapon_trail_on(void);
void whoosh_fx(int);

/* Typed declarations used by imported script wrappers. */
int add_days_to_time(int, int);
int add_hours_to_time(int, int);
int add_months_to_time(int, int);
int add_to_konq_profile_value(int, int);
int add_years_to_time(int, int);
int adjust_his_damage_multiplier(void *, float);
int adjust_my_damage_multiplier(void *, float);
int air_collision_pause(int, void *, float, float);
void ani_loop_more_frames(float);
int ani_to_frame_x_aniproc(void *, float);
void assign_obj_to_trigger(int, KonquestTriggerDefinition*);
void bgnd_add_scripted_brains_to_npc(unsigned int, unsigned int);
void bgnd_apply_active_sobj_pos_vel_drag(void *, float, float, float);
void bgnd_apply_zoffset(unsigned int, void *, float);
int bgnd_attach_rope_to_bgnd_obj(int, int, int);
void bgnd_collision_if_disable_col(int, unsigned int);
void bgnd_collision_if_enable_col(int, unsigned int);
void bgnd_collision_if_monitor_col_as(int, unsigned int, unsigned int, int);
void bgnd_collison_if_monitor_col(
    int, unsigned int, int, int, int, unsigned int, int);
void bgnd_collison_if_set_return_result(int);
void bgnd_create_pebbles(
    int model_index, unsigned int object_id, unsigned int player, int mode,
    unsigned int count);
void bgnd_destroy_sobj_uv_scroll_w_control(unsigned int);
void bgnd_enable_danger_zone(unsigned int, int);
void bgnd_enable_obj_pos_and_ang_setting(
    MkObj* object, Vec* position, Vec* angles);
void bgnd_force_ground_to(void *, float);
void bgnd_force_plyr_ground_plane(unsigned int, void *, float);
int bgnd_insert_obj_ctrl_section(int, int);
int bgnd_jtb_debug_info(void *, float, float, float);
void bgnd_launch_plyr_blood_fx(int, int);
int bgnd_make_displayed_item_pickupable_at_active_sobj_pos(int);
void bgnd_move_player(unsigned int, int, int);
void bgnd_npc_adjust_y_ang(unsigned int, float);
void bgnd_npc_get_pos(unsigned int, Vec*);
void bgnd_npc_set_ani_speed(unsigned int, float);
void bgnd_npc_set_aux_int_data(unsigned int, unsigned int, int);
void bgnd_npc_set_pos(unsigned int, float, float, float);
void bgnd_npc_set_pos_vel(unsigned int, float, float, float);
void bgnd_npc_set_pos_vel_heading(unsigned int, float);
void bgnd_npc_set_pos_y(unsigned int, float);
void bgnd_npc_set_scale(unsigned int, float, float, float);
void bgnd_npc_set_y_ang(unsigned int, float);
void bgnd_npc_start_ani(
    unsigned int, unsigned int, unsigned int, float, float);
int bgnd_obj_insert_obj_ctrl_section(int, int);
void bgnd_pebble_burst_at_chunk_pos(int, int, int);
void bgnd_pebble_burst_at_pebble_pos(int, int, int);
void bgnd_pebble_burst_at_pos(int, int, int, void *, float, float, float);
void bgnd_pebble_burst_set_end_state(int, unsigned int, unsigned int, int);
void bgnd_pebble_burst_set_value(int, unsigned int, unsigned int, int, void *,
                                 float, float, float, float, float, float);
void bgnd_pebble_burst_set_value_min_max(int, unsigned int, unsigned int, int,
                                         void *, float, float);
void bgnd_pebble_change_current_end_behavior(int);
void bgnd_pebble_rand_scale(int, void *, float, float);
void bgnd_pebble_set_current_info(unsigned int, void *, float);
void bgnd_pebble_set_current_pebble(int, int);
void bgnd_pebble_simple_launch_at_time(int, int, unsigned int, int);
int bgnd_place_crack_when_plyr_hits_ground(int);
void bgnd_place_object_at_position(int, int, int, int, int);
void bgnd_register_danger_zone_callback(PlyrPdata*, int);
void bgnd_rotate_sobj(unsigned int, void*, float, float, float);
void bgnd_rotate_xz_about_orgin_active_sobj(float);
void bgnd_run_camera_script(int, int);
void bgnd_set_active_sobj_ang(void *, float, float, float);
void bgnd_set_active_sobj_in_obj(int, unsigned int);
void bgnd_set_active_sobj_pos(void *, float, float, float);
void bgnd_set_active_sobj_pos_vel(void *, float, float, float);
void bgnd_set_active_sobj_scale(void *, float, float, float);
void bgnd_set_active_sobj_zoffset(void *, float);
void bgnd_set_collision_plane_for_launched_sobj(int, unsigned int,
                                                 unsigned int);
void bgnd_set_danger_zone_center_position(float, float, float);
void bgnd_set_danger_zone_depth(float);
void bgnd_set_danger_zone_radius(float);
void bgnd_set_danger_zone_width(float);
void bgnd_set_danger_zone_y_angle(float);
void bgnd_set_kill_plane_for_launched_sobj(int);
void bgnd_set_launch_velocity_based_on_sobj_pos(
    int velocity_index, unsigned int source_id, unsigned int target_id,
    void* script, float horizontal_velocity, float vertical_velocity);
void bgnd_set_player_shadow_ground_plane(int, float);
void bgnd_set_plyr_gravity(void *, float);
void bgnd_set_sobj_launch_params(int, int, unsigned int, void *, float, float,
                                 float, float, float, float, float);
void bgnd_set_sobj_launch_params_exact(
    int velocity_index, int angle_index, void* script, float impact_scale,
    float angle_x, float angle_y, float angle_z, float vertical_velocity,
    float heading);
void bgnd_set_wall_hide_distance(void *, float);
void bgnd_sobj_cam_frustum_test_into_transparent(
    unsigned int, float, float);
void bgnd_sobj_cam_volume_test_steer_over(unsigned int, float, float);
void bgnd_sobj_get_ang(unsigned int, Vec *);
void bgnd_sobj_set_alpha(unsigned int, unsigned int);
void bgnd_sobj_set_ang(unsigned int, void *, float, float, float);
void bgnd_sobj_set_ani_frame(unsigned int, unsigned int, int);
void bgnd_sobj_set_ani_framerate(unsigned int, unsigned int, void *, float);
void bgnd_sobj_set_pos(unsigned int, void *, float, float, float);
void bgnd_sobj_set_pos_vel(unsigned int, void *, float, float, float);
void bgnd_sobj_set_priority(unsigned int, int);
void bgnd_sobj_set_rel_pos(unsigned int, void *, float, float, float);
void bgnd_start_preload_sobj_morph(int, int, unsigned int, unsigned int);
void bgnd_start_preload_sobj_uv_scroll(
    int, int, float, float, float, float);
void bgnd_start_script_in_proc(int process_id, unsigned int script_index);
void bgnd_start_script_in_proc_bigstack(
    int process_id, unsigned int script_index);
void bgnd_start_timer(unsigned int, int, int);
int close_exterior_doors(int, int);
int cloth_change_ground_plane_for(void *, float);
int damage_player(int, void *, float);
int delete_obstacle_from_background_by_id(int);
void disable_konquest_object_zwrite_by_uid(int);
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
void enable_attached_sound_by_uid(int, int);
int face_ang_from_pos_to_him(int, int, int);
int face_point(void *, float, float, float);
int fade_from_black(int, int);
int fade_from_white(int, int);
int fade_to_black(int, int);
int fade_to_white(int, int);
int fight_fx_im_hit_flash(int, int, int, int, void *, float);
int flash_hit_at_bid_with_y(int, void *, float);
int give_koin_award(int, int);
void give_krypt_key_to_player(KonquestPuiDefinition*, int);
int hf_bgnd_set_in_setup_zone(int, int);
int hf_bgnd_set_smasher_mode(int, int);
int hide_player(int, int);
int hit_START_chores(int, int, void *, float, float);
int if_collision_slow_ani_x(void *, float, float);
int jab_attach_drink_obj_to_hand(int, int, int);
int jab_flash_screen(int, void *, float, float);
int jab_shake_dragon_king(void *, float, float);
int jab_start_jade_boomerang_throw(int, int, void *, float);
int konquest_fade_from_black(int, int);
int konquest_fade_to_black(int, int);
void konquest_open_door(int, int);
int konquest_run_camera_script(int, int);
void konquest_teleport_hero_to_location(Vec*);
void konquest_transition_object_to_state(int, int, int);
int land_chores(int, int, void *, float, float);
void launch_me_up(float, float);
int load_script_as_reaction(int, int);
int mk_chess_add_movement_skill(int, int, int, int);
int mk_chess_ani_loop_more_frames(void *, float);
int mk_chess_ani_to_blend_frame(void *, float);
int mk_chess_ani_to_frame_x(void *, float);
void mk_chess_blend_into_cell_orgin_in_x_frames(float);
int mk_chess_blend_to_ani(int, int, void *, float, float);
int mk_chess_blend_to_ani_frame(int, int, void *, float, float, float);
int mk_chess_blend_to_desired_cell_position_setting(void *, float);
void mk_chess_blend_to_my_cell_pos(float);
int mk_chess_define_class_initial_power(void *, float);
int mk_chess_glitch_to_ani_frame(int, int, void *, float, float);
int mk_chess_init_piece(int, int);
void mk_chess_launch_special_fx(unsigned int, unsigned int, unsigned int);
int mk_chess_launch_up(void *, float, float);
int mk_chess_queue_up_piece_event(int, int);
int mk_chess_set_ani_speed(void *, float);
int mk_chess_set_cell_offset(void *, float, float, float);
int mk_chess_set_normal_stance_script(int);
int mk_chess_set_obj_move_weight(void *, float);
int mk_chess_set_piece_event_script(int, int);
int mk_chess_set_piece_info(int, void *, float);
void mk_chess_set_piece_type_as(int, int);
void mk_chess_shifter_switch(int, int, float, float);
void mk_chess_snap_into_cell_orgin_over_x_frames(float);
int mk_chess_spell_move_target_from_temp_area_to(int);
int mk_chess_spell_move_target_to_target(int, int);
int mk_chess_spell_move_target_to_temp_area(int);
void mk_chess_spell_set_target_health(unsigned int target, float health);
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
void mks_ccp1_eq_insert_cloth_coll_plane(int, float, float, float, float);
typedef struct ClothInitEntry ClothInitEntry;
void mks_cloth_bones_init_by_tbl(ClothInitEntry*, int);
int mks_debug_display_cloth_coll_cyl(int, int, int);
int mks_debug_display_cloth_coll_plane(void *, float);
int mks_gravity_update_by_group(int, int, void *, float, float, float, float);
void mks_insert_cloth_force_bones(float, float);
void mks_mat_id_set_zbias(int, float);
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
int myvel_his_angle_y(void *, float, float, float);
int myvel_his_angle_y_inout(void *, float, float, float);
int myvel_my_angle_y(void *, float, float, float);
void nb_npc_slave_plyr_process_collision(unsigned int);
int nbc_script_debug_point(int, int, void *, float);
void ncs_set_pebble_pos(struct PebbleData*, int, Vec*);
int npc_ani_to_blend_frame(void *, float);
int npc_assign_door_path(int, int);
int npc_assign_path(int, int, int);
int npc_assign_path_to_him(int, int, int, int);
int npc_at_waypoint_set_flags(int, int);
int npc_attack(int, int);
int npc_blend_to_ani(int, int, void *, float, float);
int npc_blend_to_ani_with_offset(int, int, void *, float, float);
void npc_change_path_speed(float);
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
void obj_set_flipped_bones(MkObj* object, struct MkFlippedBoneMap* bone_map);
void obj_set_scale(MkObj*, void*);
int obj_set_z_offsets(int, void *, float);
void obj_sobj_cam_frustum_test_into_transparent(
    MkObj*, unsigned int, float, float);
void open_chest_and_give_item_to_player(
    KonquestPuiDefinition*, KonquestPuiDefinition*);
void open_chest_and_unlock_kontent(KonquestPuiDefinition*, int);
int pan_vol_pitch_snd_req(int, void *, float, float, float);
int play_sound_2(int, int);
void player_impale(MkObj* weapon, MkObj* second_weapon);
int player_remove_item_from_inventory(int);
int plyr_scale_pos_vel(void *, float, float, float);
int plyr_set_vel_xz_y(void *, float, float);
int plyr_start_script_in_plyr_pdata_proc(int, int, int);
int plyr_start_script_in_proc(int, int);
int plyr_weapon_grab(int, int);
void pui_delay_spawn(KonquestPuiDefinition*, float);
void pui_set_color(
    unsigned int, unsigned char, unsigned char, unsigned char, unsigned char);
void pui_set_kill_time(KonquestPuiDefinition*, int, int);
int pz_fighter_check_to_toggle_obj_and_ani_flips(int);
int pz_fighter_force_reaction_in_ticks(int, int);
int pz_fighter_register_move(int, int, int, int, int);
int random_hit_n_voice(int, int);
int random_snd_req_delay(int, int);
int rd_set_impact_vector(void *, float);
int release_kamidogu(int, int);
void remove_collision_volume_on_object_with_uid(int);
void restore_collision_volume_on_object_with_uid(int);
void resume_effect_at_obj_bid(MkObj*, int, unsigned int, int, int);
int run_camera_script(int, int, int);
int save_hero_position_and_angle_prior_to_fight(void *, float);
int set_active_projectile_velocity_to_hit_gnd(void *, float);
int set_ani_speed_miss_hit(void *, float, float);
int set_hero_punched_ground_collisions(int);
int set_his_damage_multiplier(void *, float);
int set_konq_profile_value(int, int, int);
void set_konquest_object_face_y_by_uid(int);
void set_konquest_object_render_order_priority_by_uid(int, int);
int set_konquest_weather(int, int, int);
int set_krypt_character_angle(void *, float);
int set_krypt_character_anim_script(int, int, void *, float);
int set_krypt_character_previous_root_angle(void *, float);
void set_monk_position(float, float, float, float);
int set_my_damage_multiplier(void *, float);
void set_pui_status(struct PuiItem*, int);
int set_snd_vol(int, int, void *, float);
int set_tile_grid_size(int, int);
void set_tile_visibility(int, int);
int share_my_attack_info(void *, float, float);
void slow_ani_x(float, float);
int slow_ani_x_if_miss(void *, float, float, float);
int snd_req_delay(int, int);
int sobj_set_alpha(int, int);
void spad_add_vector(int, void *, float, float, float);
void spad_rotate_xz_vector(int, void *, float);
void spad_scale_vector(int, unsigned int, void *, float);
void spad_set_heading_vector_to(int, void *, float, float);
int spad_set_vector(int, int);
void spad_set_vector_setting(int, void *, float, float, float);
void spad_set_vector_y(int, void *, float);
void spad_set_y_angle_plus_offset_from_xz_vector(int, void *, float, float,
                                                 float);
int spad_sub_vectors(int, int, int);
void start_character_separation_process(float);
int start_cliff_watcher(void *, float);
int start_scorpion_teleport_scale(void *, float, float);
void start_subzero_decoy(void*, float);
int transition_to_krypt_character_anim_script(int, int, int);
int trial_add_success_condition(int, int, int);
int trial_set_combo_requirement(int, void *, float);
int trial_set_ending_functions(int, int);
void trial_set_next_mission(int, int, int, int, int, int, int, int);
int trial_set_round_health_restoration(void *, float);
void trial_start_countdown(int, float, float);
void trial_state_collision_check(int, int);
void trigger_set_time_for_enable(KonquestTriggerDefinition*, int, int, int);
void uv_my_angle_y(void* direction, float angle_offset);
void xfer_player_proc_to_script(MkObj*, int);

/* Typed declarations used by imported script wrappers. */
int plyr_invulnerable_to_projectiles(int, int);

/* Typed declarations used by imported script wrappers. */
int advance_my_sidekick_from_behind_with_moveset(void);
int am_i_airborn_check_in_reaction(void);
MkObj* bgnd_fx_get_binded_obj(unsigned int);
float bgnd_get_camera_y_angle(void);
float bgnd_get_camera_z_pos(void);
int bgnd_get_exec_tick_ctr(void);
int bgnd_get_obj_pointer(int);
MkObj* bgnd_get_preload_obj(int);
float bgnd_get_sobj_ang_y(int);
int bgnd_is_active_sobj_hidden(void);
void bgnd_kill_fx(const char*);
int bgnd_launch_plyr_up_and_forward_running(void);
float bgnd_npc_get_ang_y(int);
float bgnd_pebble_fetch_current_info(unsigned int);
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
void* get_general_pebble_data(struct PebbleData* pebble_data);
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
int get_krypt_anim_pdata(void);
int get_krypt_character_obj(void);
int get_krypt_current_column(void);
int get_krypt_current_row(void);
int get_last_character_trained_with(void);
int get_mode_of_play(void);
int get_monk_age(void);
MkObj* get_pickup_object(void);
int get_previous_konquest_region_number(void);
int get_pui_status(struct PuiItem*);
int get_taunts_performed(void);
MkSobj* get_tile_sobj_by_id(int);
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
MkObj* load_krypt_character(char* character_name);
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
int bgnd_npc_get_aux_int_data(unsigned int, unsigned int);
float bgnd_process_collision_info(int, void *, float, float, float, float, float, float, float, float);
int fire_spear_at_camera(int, int);
int get_konq_profile_value(int, int);
int is_character_unlocked_in_profile(int, int);
int jab_attach_point_light_to_obj_bone(int, int, int);
MkObj* konquest_start_damashi(void*, float, float, float);
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
float spad_get_pos(int, unsigned int);
float spad_xz_cos_two_vectors(int, int);
float spad_xz_dot_xz(int, int);

/* Typed declarations used by imported script wrappers. */
void bgnd_launch_fx_at_plyr_pos_and_y(const char*, float);
void bgnd_set_fx_z_offset(const char *, float);

/* Typed declarations used by imported script wrappers. */
int ani_col_abort(float, int, float, float, int, float, int);
void ani_to_fall_to_frame(
    float landing_frame, int sound_id, float target_frame);
void ani_to_frame_sound(float target_frame, float sound_frame, int sound_id);
int ani_to_frame_x_col(float, int, float, float, int, float, int);
AnimPdata* animate_obj(
    MkObj* object, AnimScript* script, const int* bone_tags,
    struct MkFlippedBoneMap* flipped_bones, void* ground_collisions,
    float playback_rate, int active);
void attach_sound_to_object_by_uid(int, int, float, float, int, int);
void attach_wiff_to_konquest_object_by_uid(int, char*, float);
void bgnd_create_danger_zone(int, unsigned int, unsigned int,
                             float, unsigned int);
void bgnd_launch_fx_at_plyr_bid(const char*, int);
void bgnd_launch_fx_to_sobj(const char*, int);
void bgnd_launch_plyr_up_and_forward(
    float, float, float, float, int, float, int);
void bgnd_launch_sobj(int, unsigned int, unsigned int, unsigned int,
                      unsigned int, unsigned int, float, unsigned int);
void bgnd_pebble_change_current_behavior(
    float, float, float, float, float, float, unsigned int, int);
void bgnd_pebble_change_current_behavior_to_bounce(
    float, float, float, float, float, float, unsigned int, int);
void bgnd_pebble_launch_at_time(
    int, int, float, float, float, float, float, float, float, float, float,
    unsigned int, int);
void bgnd_place_weapon_at_position(
    int, int, int, int, int,
    float, float, float, float, float, float, float, float,
    float, float, float, float, int, float, float, float, float, float, int);
MkObj* bgnd_preload_named_model(const char*, unsigned int);
void bgnd_set_sobj_uv_scroll_abs_values(
    float, float, float, float, unsigned int);
void bgnd_set_sobj_uv_scroll_rate_values(
    float, float, float, float, unsigned int);
int bgnd_start_sobj_uv_scroll_w_control(
    int, float, float, float, float, unsigned int, unsigned int);
int display_konquest_text(
    float, float, float, unsigned int, unsigned int);
typedef struct AnimScript AnimScript;
void drone_blend_to_ani(AnimScript*, int, float);
void force_away(float, int, float, int);
void force_forward(float, int, float, int);
void got_hit_fx(int, int, int, int, int, float, int);
void konquest_use_portal(int, Vec*, float, float, float, int);
int mk_chess_ani_until_reached_destination(float, float, float, float, float, int);
void mk_chess_force_away(float speed, int delay, float damping, int frames);
void mk_chess_launch_n_land_ani_with_xz(
    int animation_id, float launch_frame, float initial_speed,
    float landing_frame, float vertical_speed, float gravity, float blend,
    float start_x, float start_y, float target_x, float target_y, int turn,
    unsigned int sound);
int mk_chess_place_special_cell_at(int, int, int, float, float, float, float, int);
void mk_chess_put_active_piece_at_cell(float x, float y, int snap);
void mk_chess_rotate_towards_cell(float x, float y, float step, int track_other, float offset);
void mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(int, float, int, float, int, float, int, float);
void mks_set_rotate_update_by_group(int, int, float, float, float, int);
int mks_set_sin_update_by_group(int, int, int, float, float, float, float, float, float, int);
void obj_grnd_bounce(MkObj* object, const Vec* velocity, float gravity,
                     float ground_offset, int bounces, float restitution);
void obj_match_obj_pos(
    MkObj* source, MkObj* destination, float blend, int snap);
void parse_args(const char*, ...);
typedef struct NcsLimbOwner NcsLimbOwner;
MkProc* plyr_spawn_his_anim_limb(
    NcsLimbOwner*, int, int, AniData*, int, MkProcEntryFn, float);
int player_area_collision_check(float, float, int, float, int);
float pz_fighter_inline_force_away_with_ani(
    float, unsigned int, float, unsigned int);
void shake_hit_voice(int, float, int, int);
void show_text(int, unsigned int, unsigned int, float, float, float, unsigned int, int);
void sidekick_switch_style_swap(float unread_arg, unsigned int count);
int single_frame_collision_check(
    int region, float radius, float height, int reaction, int strength,
    float reaction_rate);
int special_move_cam_him(float, float, float, float, float, int, int, int);
void start_gore2_pebbles(
    unsigned int object_id, int bone, MkObj* source,
    FighterMirror* decal_owner, const Vec* velocity,
    const Vec* rotation, const Vec* scale,
    const Vec* position_offset, float vertical_acceleration,
    float bounce_scale, int bounce_count);
int transition_to_anim_script_frame(
    float, float, AnimPdata*, AnimScript*, unsigned int);
void trial_do_dialog(int, int, float, float, float, unsigned int, int);
void trial_show_spoken_text_window(int, float, float, float, int, int, int, int, int);
void trial_show_text_window(int, float, float, float, int, int);
float two_player_animation_blend(AniData*, float, float, int, int);

/* Data used by imported script wrappers. */
float p_animated_intro_done(void);

/* Typed declarations used by imported script wrappers. */
void credits_add_text(const char* center_text, const char* right_text, int monochrome);
void trial_set_move_message(const char* message, const char* parameter);

/* Typed declarations used by imported script wrappers. */
void attack_to_frame_x(AniData*, float, float, float, float,
                       unsigned int, unsigned int, int);
void launch_n_land_ani(
    AniData* animation, float launch_frame, float launch_step,
    float landing_frame, int landing_animation, float velocity_y,
    float gravity, float blend);
void lower_mines_ani_to_point(
    void* script, float start_frame, float animation_step, float end_frame,
    int landing_sound, float vertical_velocity, float gravity,
    float transition, Vec* target, unsigned int frame_offset);
void newani_to_frame_x(void*, float, float, float, float, int);
void pz_fighter_startup_attack(
    void*, float, float, float, float, unsigned int, unsigned int,
    int, unsigned int, float);
void two_player_animation(AniData* animation, float attacker_blend);
float two_player_animation_flip(AniData* animation, float attacker_step);
float two_player_animation_match_attacker(
    AniData* animation, float attacker_step);
void* get_function_attributes_table(struct ScriptDistanceSlot*, int);
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
    obj_setup_for_animation(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[3].pointer);
}

/* TODO: [near miss] 100% instructions; not link-exact: pooled format-string relocation targets differ (TU string pool layout). */
void _npc_set_anim_proc(void) {
    int function_index;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x12,
               &function_index);
    npc_set_anim_proc(
        script_callable_function_table[function_index - 1]);
}

void _animate_obj(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    MkObj* object = args->slots[0].pointer;
    AnimScript* script = get_animation(args->slots[1].i);

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = animate_obj(
        object, script, args->slots[2].pointer,
        args->slots[3].pointer, args->slots[4].pointer,
        args->slots[5].f, args->slots[6].i);
}

void _start_gusher(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = (int)start_gusher(
        heart_beat, (FighterMirror*)args->slots[0].i,
        (MkObj*)args->slots[1].i, args->slots[2].i,
        (Vec*)args->slots[3].i,
        (Vec*)args->slots[4].i);
}

void _plyr_spawn_his_anim_limb(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    AniData* animation = get_animation(args->slots[3].i);

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_spawn_his_anim_limb(
            args->slots[0].pointer, args->slots[1].i, args->slots[2].i,
            animation, args->slots[4].i,
            (script_callable_function_table + args->slots[5].i)[-1],
            args->slots[6].f);
}

void _xfer_proc(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;

    xfer_proc(args->slots[0].pointer,
              (script_callable_function_table + args->slots[1].i)[-1]);
}

/* TODO: [near miss] 90.22727%; post-resolver flag/FP argument staging differs. */
void _transition_to_anim_script_frame(void) {
    struct ScriptTransitionFrameArgs* args = (struct ScriptTransitionFrameArgs*)current_args;
    AnimPdata* animation = args->animation;
    AnimScript* script = get_animation(args->animation_id);

    args = (struct ScriptTransitionFrameArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        transition_to_anim_script_frame(
            args->transition_frames, args->frame,
            animation, script, args->flags);
}

void _two_player_animation_blend(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    AniData* animation = get_animation(args->slots[0].i);

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = two_player_animation_blend(
        animation, args->slots[1].f, args->slots[2].f,
        args->slots[3].i, args->slots[4].i);
}

void _set_anim_script(void) {
    struct ScriptRawArgs* args;
    AnimPdata* animation;
    AnimScript* script;

    args = (struct ScriptRawArgs*)current_args;
    animation = args->slots[0].pointer;
    script = get_animation(args->slots[1].i);
    set_anim_script(animation, script,
        ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _anim_pdata_for_proc(void) {
    void* pdata;

    pdata = 0;
    if (((struct ScriptRawArgs*)current_args)->slots[0].pointer != 0) {
        pdata = pdata_of_proc(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
    }
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = pdata;
}

void _set_bonematcher_flag(void) {
    struct ScriptFlagWordView* object;
    int shift;

    object = ((struct ScriptFlagArgs*)current_args)->object;
    shift = 31 - ((struct ScriptFlagArgs*)current_args)->bit;
    if (((struct ScriptFlagArgs*)current_args)->enabled != 0) {
        object->flags = object->flags | (1U << shift);
    } else {
        object->flags = object->flags & ~(1U << shift);
    }
}

/* TODO: [near miss] 80.62%; equivalent flag test retains argument-load staging differences. */
void _get_bonematcher_flag(void) {
    struct ScriptFlagArgs* args;
    int shift;

    args = (struct ScriptFlagArgs*)current_args;
    shift = 31 - args->bit;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (args->object->flags & (1U << shift)) != 0;
}

void _update_mkobj(void) {
    update_mkobj(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

/* TODO: [near miss] 97.69%; typed flag read and operation order agree; constant/mask/word GPR homes remain. */
void _get_plyr_pdata_flag(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    unsigned int mask = 1U << (31 - args->slots[1].i);
    PlyrPdata* pdata = args->slots[0].pointer;

    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (mask & pdata->state_flags.raw_word) != 0;
}

void _set_obj_flag(void) {
    struct ScriptFlagWordView* object;
    int shift;

    object = ((struct ScriptFlagArgs*)current_args)->object;
    shift = 31 - ((struct ScriptFlagArgs*)current_args)->bit;
    if (((struct ScriptFlagArgs*)current_args)->enabled != 0) {
        object->flags = object->flags | (1U << shift);
    } else {
        object->flags = object->flags & ~(1U << shift);
    }
}

/* TODO: [near miss] 80.62%; equivalent flag test retains argument-load scheduling and register differences. */
void _get_obj_flag(void) {
    struct ScriptFlagArgs* args;
    int shift;

    args = (struct ScriptFlagArgs*)current_args;
    shift = 31 - args->bit;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (args->object->flags & (1U << shift)) != 0;
}

void _get_limb_obj(void) {
    struct GetLimbObjArgs* args = (struct GetLimbObjArgs*)current_args;
    unsigned int bone_index = args->bone_index;
    LimbRuntime* runtime = args->runtime;
    MkHdr* object = MK_LIVE(runtime->bone_procs[bone_index].hdr,
                            runtime->bone_procs[bone_index].instance);

    ((struct ScriptPointerResult*)active_cmdscript)->value = object;
}

static inline MkHdr* script_live_item_object(const struct ScriptObjectRef* ref) {
    MkHdr* object = ref->object;

    if (object != 0) {
        return object->instance == ref->instance ? object : 0;
    }
    return 0;
}

void _destroy_item_obj(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    struct ScriptObjectRef* ref = args->slots[0].pointer;

    if (script_live_item_object(ref) != 0) {
        MkHdr* object = ref->object;
        if (object->instance != 0) {
            MkHdrVtable* vtable = object->typed_vtbl;
            vtable->destroy(object);
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
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    struct ScriptObjectRef* ref = args->slots[0].pointer;
    MkHdr* object = MK_LIVE(ref->object, ref->instance);

    ((struct ScriptPointerResult*)active_cmdscript)->value = object;
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
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_mkobj_frame(((struct ScriptRawArgs*)current_args)->slots[0].i, 0);
}

void _nis_init(void) {
    struct ScriptActiveState* script;
    int arg0;
    int arg1;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x14,
               &arg0, &arg1);
    script = (struct ScriptActiveState*)active_cmdscript;
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
    animation = get_animation(sp10);
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
    struct TrialRequiredAttackArgs* args;

    args = (struct TrialRequiredAttackArgs*)current_args;
    trial_add_required_attack(args->attack,
                              args->count, 0);
}

void _trial_add_required_attack(void) {
    struct TrialRequiredAttackArgs* args;

    args = (struct TrialRequiredAttackArgs*)current_args;
    trial_add_required_attack(args->attack,
                              args->count, 2);
}

void _pz_fighter_should_continue_move(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = (g_pz_fighters_engine.fighter_move.policy_flags >> 2) & 1;
}

void _set_background_color(void) {
    struct BackgroundColorArgs* args;

    args = (struct BackgroundColorArgs*)current_args;
    set_background_color((unsigned char)args->red,
                         (unsigned char)args->green,
                         (unsigned char)args->blue,
                         (unsigned char)args->alpha);
}

void _camera_init_animation(void) {
    int animation_id;
    AniData* animation;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1F,
               &animation_id);
    animation = get_animation(animation_id);
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
    ((struct ScriptCommandView*)active_cmdscript)->move_attributes = sp8;
}

void _get_current_player_number(void) {
    if ((plyr_pdata)->plyr_num == 1) {
        ((struct ScriptRawResult*)active_cmdscript)->value.i = 1;
    } else {
        ((struct ScriptRawResult*)active_cmdscript)->value.i = 0;
    }
}

void _randu0(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptIntResult* script;
    unsigned short value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = randu0(args->value);
    script = (struct ScriptIntResult*)active_cmdscript;
    script->value = value;
}

void _my_attack_hit(void) {
    struct ScriptIntResult* script;
    PlyrPdata* player;

    player = plyr_pdata;
    if (player->collision_result == 1) {
        script = (struct ScriptIntResult*)active_cmdscript;
        script->value = 1;
        return;
    }
    script = (struct ScriptIntResult*)active_cmdscript;
    script->value = 0;
}

void _pz_fighter_startup_attack(void) {
    struct ScriptRawArgs* args;
    ((struct ScriptCommandView*)active_cmdscript)->animation =
        get_animation(((struct ScriptRawArgs*)current_args)->slots[0].i);
    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_startup_attack(
        ((struct ScriptCommandView*)active_cmdscript)->animation,
        args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f,
        args->slots[5].u, args->slots[6].u,
        args->slots[7].i, args->slots[8].u,
        args->slots[9].f);
}

/* MKO name references are one byte ahead of the string start. */
static inline char* script_function_name_reference(
    const struct ScriptDistanceFuncDef* functions, unsigned int function_index,
    int string_relocation)
{
    return (char*)(string_relocation + functions[function_index].name_offset);
}

static inline void script_distance_branch(unsigned int function_index) {
    ACTIVE_DISTANCE_SCRIPT->program_counter =
        ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
        ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1]
            .code_offset;
    ACTIVE_DISTANCE_SCRIPT->attributes =
        get_function_attributes_table(
            ACTIVE_DISTANCE_SCRIPT->slot, function_index);
    trial_register_script_function(function_index);
    function_index--;
    ACTIVE_DISTANCE_SCRIPT->function_name = script_function_name_reference(
        ACTIVE_DISTANCE_SCRIPT->slot->functions, function_index,
        ACTIVE_DISTANCE_SCRIPT->slot->string_relocation) - 1;
}

/* TODO: [near miss] 99.73%; name-reference boundary recovered; two integer ADD operand rows remain. */
void _pz_fighter_distance_check_wo_super_check(void) {
    int result = pz_fighter_distance_check_wo_super_check();

    if (result == 1) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    } else if (result == 2) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[1].u);
    }
}

/* TODO: [near miss] 99.73%; name-reference boundary recovered; two integer ADD operand rows remain. */
void _pz_fighter_distance_check(void) {
    int result = pz_fighter_distance_check();

    if (result == 1) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    } else if (result == 2) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[1].u);
    }
}

void _pz_fighter_set_y_constrain(void) {
    struct PzConstrainArgs* args;

    args = (struct PzConstrainArgs*)current_args;
    pz_fighter_set_y_constrain((MkObj*)plyr_obj, args->mode,
                               args->value);
}

void _pz_fighter_attack(void) {
    struct ScriptAttackArgs* args;
    struct ScriptCommandView* script;
    void* animation;

    args = (struct ScriptAttackArgs*)current_args;
    animation = get_animation(args->animation_id);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->animation = animation;
    args = (struct ScriptAttackArgs*)current_args;
    script = (struct ScriptCommandView*)active_cmdscript;
    pz_fighter_attack(script->animation,
                      args->parameters.puzzle,
                      args->arg3);
}

void _exit_attack_with(void) {
    plyr_pdata->script_exit_value_int =
        CURRENT_EXIT_ARGS->exit_value;
    plyr_pdata->script_exit_args[0] =
        CURRENT_EXIT_ARGS->exit_arg0;
    plyr_pdata->input_unlock_tick =
        CURRENT_EXIT_ARGS->input_unlock_tick;
    plyr_pdata->blocking_disable_tick_1 =
        CURRENT_EXIT_ARGS->blocking_tick;
    plyr_pdata->script_exit_args[1] =
        CURRENT_EXIT_ARGS->exit_arg1;
    plyr_pdata->script_exit_arg_2 =
        CURRENT_EXIT_ARGS->exit_arg2;
    ACTIVE_SCRIPT_EXIT->exit = j_exit_6;
    ACTIVE_SCRIPT_EXIT->state = 2;
}

void _attack_opponent_with(void) {
    struct ScriptAttackArgs* args;
    struct ScriptCommandView* script;
    void* animation;

    args = (struct ScriptAttackArgs*)current_args;
    animation = get_animation(args->animation_id);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->animation = animation;
    args = (struct ScriptAttackArgs*)current_args;
    script = (struct ScriptCommandView*)active_cmdscript;
    attack_opponent_with(script->animation,
                         args->parameters.standard,
                         args->arg3);
}

void _drone_combo(void) {
}

/* TODO: [near miss] 95.00%; four final name-address ADD/SUB rows remain;
 * full branch helpers are neutral; separate address locals regress. */
void _check_his_state(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    unsigned int function_index;
    function_index = args->slots[1].u;

    if (his_pdata->state == args->slots[0].i) {
        return;
    }
    ACTIVE_DISTANCE_SCRIPT->program_counter =
        ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
        ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1].code_offset;
    ACTIVE_DISTANCE_SCRIPT->attributes = get_function_attributes_table(
        ACTIVE_DISTANCE_SCRIPT->slot, function_index);
    trial_register_script_function(function_index);
    function_index--;
    ACTIVE_DISTANCE_SCRIPT->function_name =
        (char*)(ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index]
                    .name_offset +
                ACTIVE_DISTANCE_SCRIPT->slot->string_relocation - 1);
}

void _drone_xfer_him(void) {
    reaction_xfer_him_nohit(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _drone_super_combo(void) {
}

/* TODO: [near miss] 100% instructions; not link-exact: pooled format-string relocation targets differ (TU string pool layout). */
void _xfer_camera(void) {
    int function_index;
    int reset_projection;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x23,
               &function_index, &reset_projection);
    xfer_camera(
        script_callable_function_table[function_index - 1],
        reset_projection);
}

void _camera_set_movement_focus(void) {
    switch (((struct ScriptRawArgs*)current_args)->slots[0].i) {
    case 0:
        camera_set_movement_focus_obj(g_game_info.plyr0.slot.mirror_a);
        break;
    case 1:
        camera_set_movement_focus_obj(g_game_info.plyr1.slot.mirror_a);
        break;
    case 2:
        camera_set_movement_focus_obj(camera_get_victim());
        break;
    case 3:
        camera_set_movement_focus_obj(camera_get_attacker());
        break;
    }
}

void _camera_setup_for_custom_orbit_to_relative_point(void) {
    struct ScriptNpcCameraArgs* args = (struct ScriptNpcCameraArgs*)current_args;
    KonquestNpcData* data = args->npc_data;
    struct ScriptNpcHandle* npc;
    Vec movement;
    Vec lookat;
    float travel_time;
    float initial_speed;
    float final_speed;
    int direction;
    int mode;

    movement.x = args->movement_x;
    movement.y = args->movement_y;
    movement.z = args->movement_z;
    lookat.z = 0.0f;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    travel_time = args->travel_time;
    initial_speed = args->initial_speed;
    final_speed = args->final_speed;
    direction = args->rotation_direction;
    mode = args->movement_mode;
    npc = (struct ScriptNpcHandle*)find_npc_by_data(data);
    if (npc->body != 0) {
        camera_set_movement_focus_obj((MkObj*)npc->body->camera_object);
        camera_set_lookat_focus((MkObj*)npc->body->camera_object);
        camera_set_look_mode(8);
        camera_set_movement_mode(mode);
        camera_set_movement_offset(&movement);
        camera_set_lookat_offset(&lookat);
        camera_set_initial_speed(initial_speed);
        camera_set_final_speed(final_speed);
        camera_set_rotation_direction(direction);
        camera_set_travel_time(travel_time);
        camera_set_center_of_rotation(&npc->body->camera_object->pos);
        camera_set_radial_movement(1);
        camera_set_custom_camera_movement_flag(1);
    }
}

void _camera_set_position_relative_to_npc(void) {
    struct ScriptNpcCameraArgs* args = (struct ScriptNpcCameraArgs*)current_args;
    KonquestNpcData* data = args->npc_data;
    struct ScriptNpcHandle* npc;
    Vec movement;
    Vec lookat;
    int mode;

    movement.x = args->movement_x;
    movement.y = args->movement_y;
    movement.z = args->movement_z;
    lookat.z = 0.0f;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    mode = args->position_mode;
    npc = (struct ScriptNpcHandle*)find_npc_by_data(data);
    if (npc->body != 0) {
        camera_set_movement_focus_obj((MkObj*)npc->body->camera_object);
        camera_set_lookat_focus((MkObj*)npc->body->camera_object);
        camera_set_look_mode(9);
        camera_set_movement_mode(mode);
        camera_set_movement_offset(&movement);
        camera_set_lookat_offset(&lookat);
        camera_set_glitch_flag();
    }
}

void _camera_setup_for_tracking_npc(void) {
    struct ScriptNpcCameraArgs* args = (struct ScriptNpcCameraArgs*)current_args;
    KonquestNpcData* data = args->npc_data;
    struct ScriptNpcHandle* npc;
    Vec movement;
    Vec lookat;

    movement.x = args->movement_x;
    movement.y = args->movement_y;
    movement.z = args->movement_z;
    lookat.z = 0.0f;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    npc = (struct ScriptNpcHandle*)find_npc_by_data(data);
    if (npc->body != 0) {
        camera_set_movement_focus_obj((MkObj*)npc->body->camera_object);
        camera_set_lookat_focus((MkObj*)npc->body->camera_object);
        camera_set_look_mode(0);
        camera_set_movement_mode(2);
        camera_set_movement_offset(&movement);
        camera_set_lookat_offset(&lookat);
        camera_set_movement_rate(0.1f);
    }
}

void _camera_setup_for_orbiting_npc(void) {
    struct ScriptNpcCameraArgs* args = (struct ScriptNpcCameraArgs*)current_args;
    KonquestNpcData* data = args->npc_data;
    float speed = args->orbit_speed;
    int direction = args->orbit_direction;
    struct ScriptNpcHandle* npc;
    Vec movement;
    Vec lookat;

    movement.z = 0.0f;
    movement.x = 0.0f;
    movement.y = args->movement_z;
    lookat.z = 0.0f;
    lookat.x = 0.0f;
    lookat.y = args->lookat_y;
    npc = (struct ScriptNpcHandle*)find_npc_by_data(data);
    if (npc->body != 0) {
        camera_set_movement_focus_obj((MkObj*)npc->body->camera_object);
        camera_set_lookat_focus((MkObj*)npc->body->camera_object);
        camera_setup_simple_rotation(direction, speed);
        camera_set_look_mode(8);
        camera_set_movement_mode(7);
        camera_set_movement_offset(&movement);
        camera_set_lookat_offset(&lookat);
        camera_set_movement_rate(0.1f);
    }
}

void _camera_set_lookat_focus_obj(void) {
    camera_set_lookat_focus(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _camera_set_lookat_focus(void) {
    switch (((struct ScriptRawArgs*)current_args)->slots[0].i) {
    case 0:
        camera_set_lookat_focus(g_game_info.plyr0.slot.mirror_a);
        break;
    case 1:
        camera_set_lookat_focus(g_game_info.plyr1.slot.mirror_a);
        break;
    case 2:
        camera_set_lookat_focus(camera_get_victim());
        break;
    case 3:
        camera_set_lookat_focus(camera_get_attacker());
        break;
    }
}

void _camera_set_lookat_offset_obj_rel(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_lookat_offset_obj_rel(&offset, current_args);
}

void _camera_set_lookat_offset(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_lookat_offset(&offset);
}

void _camera_set_movement_offset_obj_rel(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_movement_offset_obj_rel(&offset, current_args);
}

void _camera_set_movement_offset(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_movement_offset(&offset);
}

void _if_switching_to(void) {
    PlyrPdata* player = plyr_pdata;
    int command = ((struct ScriptRawArgs*)current_args)->slots[0].i;
    int style = player->player_slot + 1;

    if (style >= 3 || (player->sidekick_available != 0 && style >= 2)) {
        style = 0;
    }
    if (command == (int)player->weapon_styles[style]->animation_header) {
        if (player->drone_request != 0) {
            if (drone_ai_check_switching_to(command) != 0) {
                ((struct ScriptCommandView*)active_cmdscript)->result = 1;
                ((struct ScriptCommandView*)active_cmdscript)->result = 1;
                return;
            }
        } else if (was_button_pressed(2) != 0) {
            plyr_pdata->round_attack_stage++;
            ((struct ScriptCommandView*)active_cmdscript)->result = 1;
            ((struct ScriptCommandView*)active_cmdscript)->result = 1;
            return;
        }
    }
    ((struct ScriptCommandView*)active_cmdscript)->result = 0;
    ((struct ScriptCommandView*)active_cmdscript)->result = 0;
}

void _branch_next_style(void) {
    struct ScriptSingleIntArgs* args;
    union ScriptResultRef script;

    advance_my_moveset();
    args = (struct ScriptSingleIntArgs*)current_args;
    script.bytes = active_cmdscript;
    script.command->branch_target = args->value;
    script.bytes = active_cmdscript;
    script.command->exit = (ScriptEntryFn)j_call_player_script_function;
    script.bytes = active_cmdscript;
    script.command->state = 2;
}

void _true_branch_next_sidekick_style(void) {
    struct ScriptSingleIntArgs* args;
    union ScriptResultRef script;

    script.bytes = active_cmdscript;
    if (script.command->result == 0) {
        return;
    }
    args = (struct ScriptSingleIntArgs*)current_args;
    script.command->branch_target = args->value;
    script.bytes = active_cmdscript;
    script.command->exit = (ScriptEntryFn)j_call_player_script_function;
    script.bytes = active_cmdscript;
    script.command->state = 2;
}

void _true_branch_next_style(void) {
    struct ScriptSingleIntArgs* args;
    union ScriptResultRef script;

    script.bytes = active_cmdscript;
    if (script.command->result != 0) {
        advance_my_moveset();
        args = (struct ScriptSingleIntArgs*)current_args;
        script.bytes = active_cmdscript;
        script.command->branch_target = args->value;
        script.bytes = active_cmdscript;
        script.command->exit = (ScriptEntryFn)j_call_player_script_function;
        script.bytes = active_cmdscript;
        script.command->state = 2;
    }
}

void _was_i_hit_x_times(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptCommandView* script;
    int result;

    args = (struct ScriptSingleIntArgs*)current_args;
    result = was_i_hit_x_times(args->value);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->result = result;
}

void _ani_col_abort(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = ani_col_abort(
        args->slots[0].f, args->slots[1].i, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].f,
        args->slots[6].i);
}

void _was_button_and_direction(void) {
    struct ScriptTwoIntArgs* args;
    struct ScriptCommandView* script;
    int result;

    args = (struct ScriptTwoIntArgs*)current_args;
    result = was_button_and_direction(args->first,
                                      args->second);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->result = result;
}

/* TODO: [near miss] 99.77%; name-reference boundary recovered; one integer ADD operand row remains. */
void _hit_branch(void) {
    if (plyr_pdata->collision_result == 1) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

/* TODO: [near miss] 99.77%; name-reference boundary recovered; one integer ADD operand order remains. */
void _block_branch(void) {
    if (plyr_pdata->collision_result == 2) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

/* TODO: [near miss] 99.77%; name-reference boundary recovered; one integer ADD operand row remains. */
void _miss_branch(void) {
    if (plyr_pdata->collision_result == 0) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

void _disable_grounding(void) {
    ((struct ScriptGroundObjView*)plyr_obj)->launched = 0;
}

void _set_player_hiframe(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_anim_hiframe(args->slots[0].f);
}

void _set_player_movement_weight(void) {
    ((AnimPdata*)plyr_anim_pdata)->weight =
        ((struct ScriptRawArgs*)current_args)->slots[0].f;
}

void _set_player_step(void) {
    ((AnimPdata*)plyr_anim_pdata)->step =
        ((struct ScriptRawArgs*)current_args)->slots[0].f;
}

void _newani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    args = (struct ScriptRawArgs*)current_args;
    newani_to_frame_x(
        ((struct ScriptCommandView*)active_cmdscript)->animation,
        args->slots[1].f, args->slots[2].f, args->slots[3].f,
        args->slots[4].f, args->slots[5].i);
}

void _blend_to_ani_inout(void) {
    struct ScriptRawArgs* args;
    struct ScriptCommandView* script;

    ((struct ScriptCommandView*)active_cmdscript)->animation =
        get_animation(((struct ScriptRawArgs*)current_args)->slots[0].i);
    ((struct ScriptCommandView*)active_cmdscript)->secondary_animation =
        get_animation(((struct ScriptRawArgs*)current_args)->slots[1].i);
    script = (struct ScriptCommandView*)active_cmdscript;
    args = (struct ScriptRawArgs*)current_args;
    blend_to_ani_INOUT(
        script->animation, script->animation, args->slots[2].f,
        args->slots[3].f, args->slots[4].f);
}

void _glitch_to_ani_frame(void) {
    struct ScriptAnimationArgs* args;
    struct ScriptCommandView* script;
    void* animation;

    args = (struct ScriptAnimationArgs*)current_args;
    animation = get_animation(args->animation_id);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->animation = animation;
    args = (struct ScriptAnimationArgs*)current_args;
    script = (struct ScriptCommandView*)active_cmdscript;
    glitch_to_ani_frame(script->animation, args->flags,
                        args, args->frame);
}

void _blend_to_ani_frame(void) {
    struct ScriptAnimationArgs* args;
    struct ScriptCommandView* script;
    void* animation;

    args = (struct ScriptAnimationArgs*)current_args;
    animation = get_animation(args->animation_id);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->animation = animation;
    args = (struct ScriptAnimationArgs*)current_args;
    script = (struct ScriptCommandView*)active_cmdscript;
    blend_to_ani_frame(script->animation, args->flags,
                       args->frame, args->blend);
}

void _glitch_him_to_ani(void) {
    PlyrPdata* opponent;
    MkProc* proc;
    AnimPdata* pdata;

    ((struct ScriptCommandView*)active_cmdscript)->animation =
        get_animation(((struct ScriptRawArgs*)current_args)->slots[0].i);
    opponent = plyr_pdata->his_plyr_pdata;
    proc = MK_LIVE(opponent->anim_proc, opponent->anim_proc_instance);
    if (proc != 0) {
        pdata = (AnimPdata*)pdata_of_proc(proc);
        set_anim_script(pdata,
                        ((struct ScriptCommandView*)active_cmdscript)->animation,
                        ((struct ScriptRawArgs*)current_args)->slots[1].i);
    }
}

void _glitch_to_ani(void) {
    struct ScriptAnimationArgs* args;
    struct ScriptCommandView* script;
    void* animation;

    args = (struct ScriptAnimationArgs*)current_args;
    animation = get_animation(args->animation_id);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->animation = animation;
    script = (struct ScriptCommandView*)active_cmdscript;
    args = (struct ScriptAnimationArgs*)current_args;
    glitch_to_ani(script->animation, args->flags);
}

void _enable_grounding(void) {
    struct ScriptGroundObjView* player;
    MkHdr* object;

    ((struct ScriptGroundObjView*)plyr_obj)->launched = 1;
    player = (struct ScriptGroundObjView*)plyr_obj;
    object = player != 0 ? as_mkhdr((MkHdr*)player) : 0;
    update_bone_hierarchy(object);
    player = (struct ScriptGroundObjView*)plyr_obj;
    object = player != 0 ? as_mkhdr((MkHdr*)player) : 0;
    ground_me(object);
}

void _was_button_pressed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        was_button_pressed(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _am_i_airborn(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_airborn();
}

void _am_i_blocking(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_blocking();
}

void _is_his_chest_to_screen(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_his_chest_to_screen();
}

void _is_my_chest_to_screen(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_my_chest_to_screen();
}

void _am_i_on_the_left2(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        am_i_on_the_left2(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                          ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _am_i_on_the_left(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_on_the_left();
}

void _is_he_flipped(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_he_flipped();
}

void _am_i_flipped_or_turned(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_flipped_or_turned();
}

void _am_i_flipped(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_flipped();
}

void _lower_mines_ani_to_point(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    args = (struct ScriptRawArgs*)current_args;
    lower_mines_ani_to_point(
        ((struct ScriptCommandView*)active_cmdscript)->animation,
        args->slots[1].f, args->slots[2].f, args->slots[3].f,
        args->slots[4].i, args->slots[5].f, args->slots[6].f,
        args->slots[7].f, args->slots[8].pointer, args->slots[9].u);
}

void _launch_n_land_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    args = (struct ScriptRawArgs*)current_args;
    launch_n_land_ani(
        ((struct ScriptCommandView*)active_cmdscript)->animation,
        args->slots[1].f, args->slots[2].f, args->slots[3].f,
        args->slots[4].i, args->slots[5].f, args->slots[6].f,
        args->slots[7].f);
}

void _attack_to_frame_x(void) {
    struct ScriptRawArgs* args;
    ((struct ScriptCommandView*)active_cmdscript)->animation =
        get_animation(((struct ScriptRawArgs*)current_args)->slots[0].i);
    args = (struct ScriptRawArgs*)current_args;
    attack_to_frame_x(((struct ScriptCommandView*)active_cmdscript)->animation,
        args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f,
        args->slots[5].u, args->slots[6].u, args->slots[7].i);
}

void _two_player_animation_match_attacker(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    two_player_animation_match_attacker(((struct ScriptCommandView*)active_cmdscript)->animation, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _two_player_animation_flip(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    two_player_animation_flip(((struct ScriptCommandView*)active_cmdscript)->animation, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _two_player_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    two_player_animation(((struct ScriptCommandView*)active_cmdscript)->animation, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _is_fast_getup(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_fast_getup();
}

void _disable_impale_check(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = disable_impale_check();
}

void _blend_to_ani(void) {
    struct ScriptAnimationArgs* args;
    struct ScriptCommandView* script;
    void* animation;

    args = (struct ScriptAnimationArgs*)current_args;
    animation = get_animation(args->animation_id);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->animation = animation;
    args = (struct ScriptAnimationArgs*)current_args;
    script = (struct ScriptCommandView*)active_cmdscript;
    blend_to_ani(
        script->animation,
        args->flags,
        args->frame);
}

void _exit_react(void) {
    CURRENT_PLAYER_PDATA->script_exit_value_int = CURRENT_EXIT_ARGS->exit_value;
    CURRENT_PLAYER_PDATA->script_exit_args[0] = CURRENT_EXIT_ARGS->exit_arg0;
    CURRENT_PLAYER_PDATA->input_unlock_tick = CURRENT_EXIT_ARGS->input_unlock_tick;
    CURRENT_PLAYER_PDATA->blocking_disable_tick_2 = CURRENT_EXIT_ARGS->blocking_tick;
    CURRENT_PLAYER_PDATA->script_exit_args[1] = CURRENT_EXIT_ARGS->exit_arg1;
    CURRENT_PLAYER_PDATA->script_exit_arg_2 = CURRENT_EXIT_ARGS->exit_arg2;
    ACTIVE_SCRIPT_EXIT->exit = j_exit_react;
    ACTIVE_SCRIPT_EXIT->state = 2;
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
    ACTIVE_SCRIPT_EXIT->exit =
        exit_table[((struct ExitFloatIntArgs*)current_args)->exit_index];
    CURRENT_PLAYER_PDATA->summon_position_x =
        ((struct ExitFloatIntArgs*)current_args)->float_value;
    CURRENT_PLAYER_PDATA->script_exit_value_int =
        ((struct ExitFloatIntArgs*)current_args)->int_value;
    ACTIVE_SCRIPT_EXIT->state = 2;
}

void _script_exit(void) {
    ACTIVE_SCRIPT_EXIT->exit =
        exit_table[((struct ScriptRawArgs*)current_args)->slots[0].i];
    ACTIVE_SCRIPT_EXIT->state = 2;
}

void _script_return(void) {
    ((struct ScriptCommandView*)active_cmdscript)->exit = 0;
    ((struct ScriptCommandView*)active_cmdscript)->state = 2;
}

void _print_v(void) {
}

void _print_f(void) {
}

void _print_i(void) {
}

void _print_s(void) {
}

/* TODO: [near miss] 94.05%; parsed index and name-reference boundary agree; final address arithmetic scheduling remains. */
void _gosub(void) {
    unsigned int function_index;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x12,
               &function_index);
    push_script_stack_frame(1);
    ACTIVE_DISTANCE_SCRIPT->program_counter =
        ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
        ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1].code_offset;
    ACTIVE_DISTANCE_SCRIPT->function_name = script_function_name_reference(
        ACTIVE_DISTANCE_SCRIPT->slot->functions, function_index - 1,
        ACTIVE_DISTANCE_SCRIPT->slot->string_relocation) - 1;
}

/* TODO: [near miss] 99.74%; name-reference boundary recovered; one integer ADD operand row remains. */
void _branch(void) {
    script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
}

/* TODO: [near miss] 99.76%; name-reference boundary recovered; one integer ADD operand row remains. */
void _true_branch(void) {
    if (((struct ScriptCommandView*)active_cmdscript)->result != 0) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

void _true_xfer_him(void) {
    int condition = ((struct ScriptRawResult*)active_cmdscript)->value.i;
    void* reaction = ((struct ScriptRawArgs*)current_args)->slots[0].pointer;

    if (condition != 0) {
        reaction_xfer_him_nohit(reaction);
    }
}

/* TODO: [near miss] 100% instructions; not link-exact: pooled format-string relocation targets differ (TU string pool layout). */
void _script_sleep(void) {
    int ticks;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1F,
               &ticks);
    _mkproc_sleep_ticks = (float)ticks * inverse_game_speed;
    aproc->vtbl->sleep();
}

float j_sleep_forever(void) {
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep();
    }
}

float j_call_player_script_function(void) {
    ScriptSlot* slot;

    pw_plyr();
    cmdscript_reset_stack();
    slot = (plyr_pdata)->fighter_definition->cmo;
    cmdscript_setup_execution(
        slot, ((struct ScriptCommandView*)active_cmdscript)->branch_target);
    call_player_script_function(
        (plyr_pdata)->fighter_definition->cmo);
    return 1.0f;
}

void* get_animation(int animation_id) {
    void** animations;

    if ((unsigned int)animation_id + 0x60000U == 0x4BFA) {
        return 0;
    }
    if (animation_id < 0) {
        return 0;
    }
    if (animation_id >= 3000) {
        animations = bgnd_animation_table;
        animation_id -= 3000;
    } else if (animation_id >= 2000) {
        animations = pz_shared_ani;
        animation_id -= 2000;
    } else if (animation_id >= 1000) {
        animations = npc_fast_anims;
        animation_id -= 1000;
    } else if (animation_id >= 900) {
        animations = konq_nis_anims;
        animation_id -= 900;
    } else if (animation_id >= 700) {
        animations = konquest_animations;
        animation_id -= 700;
    } else if (animation_id >= 600) {
        animations = (void**)&his_pdata->reaction_animation;
        animation_id -= 600;
    } else if (animation_id >= 500) {
        animations = (void**)his_pdata->fighter_definition->alternate_animations;
        animation_id -= 500;
    } else if (animation_id >= 400) {
        animations = (void**)plyr_pdata->animation_data;
        animation_id -= 400;
    } else if (animation_id >= 100) {
        animations = shared_ani;
        animation_id -= 100;
    } else {
        animations = (void**)plyr_pdata->fighter_definition->primary_animations;
    }

    return animations[animation_id];
}

void _tag_team_activate_player(void) {
    tag_team_activate_player(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                             ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _load_and_set_refl_on_weapon(void) {
    load_and_set_refl_on_weapon();
}

void _advance_active_moveset(void) {
    advance_active_moveset(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _get_active_moveset_from_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        get_active_moveset_from_pdata(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _fx_transfer(void) {
    fx_transfer(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _bgnd_force_specularity_off_for_material(void) {
    bgnd_force_specularity_off_for_material(
        ((struct ScriptRawArgs*)current_args)->slots[0].u,
        ((struct ScriptRawArgs*)current_args)->slots[1].u);
}

void _bgnd_sobj_set_texture_kl_values(void) {
    bgnd_sobj_set_texture_kl_values(((struct ScriptRawArgs*)current_args)->slots[0].u,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[2].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[3].f,
                                    ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _debug_print_message(void) {
    get_script_string_arg(1);
    debug_print_message();
}

void _fxsys_set(void) {
    fxsys_set(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _fxsys_set_v3(void) {
    fxsys_set_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
                 ((struct ScriptRawArgs*)current_args)->slots[1].f,
                 ((struct ScriptRawArgs*)current_args)->slots[2].f,
                 ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _fx_bind_render_to_sobj(void) {
    fx_bind_render_to_sobj(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _fx_bind_emitter_to_obj_bone(void) {
    fx_bind_emitter_to_obj_bone(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _fx_bind_render_to_obj_bone(void) {
    fx_bind_render_to_obj_bone(((struct ScriptRawArgs*)current_args)->slots[0].i,
                               ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                               ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _fx_disable_ztest(void) {
    fx_disable_ztest(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx_set_render_priority(void) {
    fx_set_render_priority(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx_hide(void) {
    fx_hide(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx_get_v3(void) {
    fx_get_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
              ((struct ScriptRawArgs*)current_args)->slots[1].i,
              ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _fx_set(void) {
    fx_set(((struct ScriptRawArgs*)current_args)->slots[0].i,
           ((struct ScriptRawArgs*)current_args)->slots[1].i,
           ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _fx_set_param_v3(void) {
    fx_set_param_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
                    ((struct ScriptRawArgs*)current_args)->slots[1].i,
                    ((struct ScriptRawArgs*)current_args)->slots[2].f,
                    ((struct ScriptRawArgs*)current_args)->slots[3].f,
                    ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

void _enable_profiling(void) {
    enable_profiling(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _kill_on_y_less_than_field(void) {
    kill_on_y_less_than_field(((struct ScriptRawArgs*)current_args)->slots[0].i,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _change_on_y_less_than_field(void) {
    change_on_y_less_than_field(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _change_on_y_less(void) {
    change_on_y_less(((struct ScriptRawArgs*)current_args)->slots[0].i,
                     ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _change_on_less(void) {
    change_on_less(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _change_on_greater(void) {
    change_on_greater(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _kill_roundrobin(void) {
    kill_roundrobin(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _kill_percent(void) {
    kill_percent(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _kill_on_greater(void) {
    kill_on_greater(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _udpate_roundrobin(void) {
    udpate_roundrobin(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _update_assign(void) {
    update_assign(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _update_attract(void) {
    update_attract(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].i,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _update_bounce(void) {
    update_bounce(((struct ScriptRawArgs*)current_args)->slots[0].i,
                  ((struct ScriptRawArgs*)current_args)->slots[1].i,
                  ((struct ScriptRawArgs*)current_args)->slots[2].i,
                  ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_texanim_hold(void) {
    update_texanim_hold(((struct ScriptRawArgs*)current_args)->slots[0].i,
                        ((struct ScriptRawArgs*)current_args)->slots[1].i,
                        ((struct ScriptRawArgs*)current_args)->slots[2].f,
                        ((struct ScriptRawArgs*)current_args)->slots[3].i,
                        ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _update_texanim(void) {
    update_texanim(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].i,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f,
                   ((struct ScriptRawArgs*)current_args)->slots[3].i,
                   ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _update_lerp_color(void) {
    update_lerp_color(((struct ScriptRawArgs*)current_args)->slots[0].i,
        ((struct ScriptRawArgs*)current_args)->slots[1].i,
        ((struct ScriptRawArgs*)current_args)->slots[2].f,
        ((struct ScriptRawArgs*)current_args)->slots[3].i,
        ((struct ScriptRawArgs*)current_args)->slots[4].i,
        ((struct ScriptRawArgs*)current_args)->slots[5].pointer);
}

void _update_fade_alpha2(void) {
    update_fade_alpha2(((struct ScriptRawArgs*)current_args)->slots[0].i,
        ((struct ScriptRawArgs*)current_args)->slots[1].i,
        ((struct ScriptRawArgs*)current_args)->slots[2].f,
        ((struct ScriptRawArgs*)current_args)->slots[3].f,
        ((struct ScriptRawArgs*)current_args)->slots[4].i,
        ((struct ScriptRawArgs*)current_args)->slots[5].i);
}

void _update_fade_alpha(void) {
    update_fade_alpha(((struct ScriptRawArgs*)current_args)->slots[0].i,
                      ((struct ScriptRawArgs*)current_args)->slots[1].i,
                      ((struct ScriptRawArgs*)current_args)->slots[2].f,
                      ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_mul_scalar(void) {
    update_mul_scalar(((struct ScriptRawArgs*)current_args)->slots[0].i,
                      ((struct ScriptRawArgs*)current_args)->slots[1].f,
                      ((struct ScriptRawArgs*)current_args)->slots[2].f,
                      ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_wrapbox(void) {
    update_wrapbox(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].f,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f,
                   ((struct ScriptRawArgs*)current_args)->slots[3].f,
                   ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

void _update_add_constant(void) {
    update_add_constant(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _update_add_constant_v3(void) {
    update_add_constant_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f,
                           ((struct ScriptRawArgs*)current_args)->slots[2].f,
                           ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _update_copy(void) {
    update_copy(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _update_add(void) {
    update_add(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _initial_multiply_float(void) {
    initial_multiply_float(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f,
                           ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _initial_set_float(void) {
    initial_set_float(((struct ScriptRawArgs*)current_args)->slots[0].i,
                      ((struct ScriptRawArgs*)current_args)->slots[1].f,
                      ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _initial_add_v3(void) {
    initial_add_v3(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _initial_divert(void) {
    initial_divert(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].f,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _initial_reflect(void) {
    initial_reflect(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_cycle_emission(void) {
    set_cycle_emission(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_cycle_length(void) {
    set_cycle_length(((struct ScriptRawArgs*)current_args)->slots[0].f,
                     ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _fx_reset_emit(void) {
    fx_reset_emit(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_pause_emit(void) {
    fx_pause_emit(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_resume_emit(void) {
    fx_resume_emit(((struct ScriptRawArgs*)current_args)->slots[0].u);
}

void _fx_next_emitter(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fx_next_emitter(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_reset(void) {
    fx_reset(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _fx_restart_emit(void) {
    fx_restart_emit(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _reset_effect(void) {
    get_script_string_arg(1);
    reset_effect();
}

void _resume_effect(void) {
    resume_effect(get_script_string_arg(1));
}

void _spawn_color(void) {
    spawn_color(((struct ScriptRawArgs*)current_args)->slots[0].i,
                ((struct ScriptRawArgs*)current_args)->slots[1].i,
                ((struct ScriptRawArgs*)current_args)->slots[2].i,
                ((struct ScriptRawArgs*)current_args)->slots[3].i,
                ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _spawn_random_size(void) {
    spawn_random_size(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _set_growth_coefficient(void) {
    set_growth_coefficient(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _set_drag_coefficient(void) {
    set_drag_coefficient(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _set_rotation(void) {
    set_rotation(((struct ScriptRawArgs*)current_args)->slots[0].f, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _kill_at_plane(void) {
    kill_at_plane(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _emit_constant_rate(void) {
    emit_constant_rate();
}

void _emit_roundrobin_mechanism(void) {
    emit_roundrobin_mechanism(((struct ScriptRawArgs*)current_args)->slots[0].i,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _emit_value_i(void) {
    emit_value_i(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _emit_value(void) {
    emit_value(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _emit_from_pos_clamp_y(void) {
    emit_from_pos_clamp_y(((struct ScriptRawArgs*)current_args)->slots[0].i,
                          ((struct ScriptRawArgs*)current_args)->slots[1].i,
                          ((struct ScriptRawArgs*)current_args)->slots[2].f,
                          ((struct ScriptRawArgs*)current_args)->slots[3].f,
                          ((struct ScriptRawArgs*)current_args)->slots[4].f,
                          ((struct ScriptRawArgs*)current_args)->slots[5].f,
                          ((struct ScriptRawArgs*)current_args)->slots[6].f,
                          ((struct ScriptRawArgs*)current_args)->slots[7].f);
}

void _emit_from_pos(void) {
    emit_from_pos(((struct ScriptRawArgs*)current_args)->slots[0].i,
                  ((struct ScriptRawArgs*)current_args)->slots[1].i,
                  ((struct ScriptRawArgs*)current_args)->slots[2].f,
                  ((struct ScriptRawArgs*)current_args)->slots[3].f,
                  ((struct ScriptRawArgs*)current_args)->slots[4].f,
                  ((struct ScriptRawArgs*)current_args)->slots[5].f,
                  ((struct ScriptRawArgs*)current_args)->slots[6].f);
}

void _emit_spherical_section(void) {
    emit_spherical_section(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f,
                           ((struct ScriptRawArgs*)current_args)->slots[2].f,
                           ((struct ScriptRawArgs*)current_args)->slots[3].f,
                           ((struct ScriptRawArgs*)current_args)->slots[4].f,
                           ((struct ScriptRawArgs*)current_args)->slots[5].f,
                           ((struct ScriptRawArgs*)current_args)->slots[6].f,
                           ((struct ScriptRawArgs*)current_args)->slots[7].f);
}

void _emit_spherical_from_boundary(void) {
    emit_spherical_from_boundary(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                 ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _emit_spherical(void) {
    emit_spherical(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _texture_animation_with_vsize(void) {
    texture_animation_with_vsize(((struct ScriptRawArgs*)current_args)->slots[0].f,
                                 ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                 ((struct ScriptRawArgs*)current_args)->slots[2].i,
                                 ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _texture_animation(void) {
    texture_animation(((struct ScriptRawArgs*)current_args)->slots[0].f,
        ((struct ScriptRawArgs*)current_args)->slots[1].i,
        ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _emission_duration(void) {
    emission_duration(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _emit_cylindrical(void) {
    emit_cylindrical(((struct ScriptRawArgs*)current_args)->slots[0].i,
                     ((struct ScriptRawArgs*)current_args)->slots[1].f,
                     ((struct ScriptRawArgs*)current_args)->slots[2].f,
                     ((struct ScriptRawArgs*)current_args)->slots[3].f,
                     ((struct ScriptRawArgs*)current_args)->slots[4].f,
                     ((struct ScriptRawArgs*)current_args)->slots[5].f,
                     ((struct ScriptRawArgs*)current_args)->slots[6].f,
                     ((struct ScriptRawArgs*)current_args)->slots[7].f);
}

void _emit_cartesian(void) {
    emit_cartesian(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].f,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f,
                   ((struct ScriptRawArgs*)current_args)->slots[3].f,
                   ((struct ScriptRawArgs*)current_args)->slots[4].f,
                   ((struct ScriptRawArgs*)current_args)->slots[5].f,
                   ((struct ScriptRawArgs*)current_args)->slots[6].f);
}

void _emit_disc2(void) {
    emit_disc2(((struct ScriptRawArgs*)current_args)->slots[0].i,
               ((struct ScriptRawArgs*)current_args)->slots[1].f,
               ((struct ScriptRawArgs*)current_args)->slots[2].f,
               ((struct ScriptRawArgs*)current_args)->slots[3].f,
               ((struct ScriptRawArgs*)current_args)->slots[4].f,
               ((struct ScriptRawArgs*)current_args)->slots[5].f);
}

void _emit_disc(void) {
    emit_disc(((struct ScriptRawArgs*)current_args)->slots[0].i,
              ((struct ScriptRawArgs*)current_args)->slots[1].f,
              ((struct ScriptRawArgs*)current_args)->slots[2].f,
              ((struct ScriptRawArgs*)current_args)->slots[3].f,
              ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

void _emit_cuboid(void) {
    emit_cuboid(((struct ScriptRawArgs*)current_args)->slots[0].i,
                ((struct ScriptRawArgs*)current_args)->slots[1].f,
                ((struct ScriptRawArgs*)current_args)->slots[2].f,
                ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _emit_from_point(void) {
    emit_from_point(((struct ScriptRawArgs*)current_args)->slots[0].f,
                    ((struct ScriptRawArgs*)current_args)->slots[1].f,
                    ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _emit_uv(void) {
    emit_uv(((struct ScriptRawArgs*)current_args)->slots[0].i,
            ((struct ScriptRawArgs*)current_args)->slots[1].f,
            ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _emit_color(void) {
    emit_color(((struct ScriptRawArgs*)current_args)->slots[0].i,
               ((struct ScriptRawArgs*)current_args)->slots[1].i,
               ((struct ScriptRawArgs*)current_args)->slots[2].i,
               ((struct ScriptRawArgs*)current_args)->slots[3].i,
               ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _emit_in_range(void) {
    emit_in_range(((struct ScriptRawArgs*)current_args)->slots[0].i,
                  ((struct ScriptRawArgs*)current_args)->slots[1].f,
                  ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _fx_by_owner(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fx_by_owner(get_script_string_arg(1), ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _fx2(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fx2(((struct ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2));
}

void _fx(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = fx(get_script_string_arg(1));
}

void _set_vertex_color(void) {
    set_vertex_color(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_light(void) {
    set_light(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _set_aspect_ratio(void) {
    set_aspect_ratio(((struct ScriptRawArgs*)current_args)->slots[0].f, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _set_bounding_radius(void) {
    set_bounding_radius(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _create_y_mirror_effect(void) {
    create_y_mirror_effect(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _z_bias(void) {
    z_bias(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _particle_size(void) {
    particle_size(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _face_y(void) {
    face_y();
}

void _set_decal_plane(void) {
    set_decal_plane(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _create_multiemit_parametric_fx(void) {
    create_multiemit_parametric_fx(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                   get_script_string_arg(2),
                                   ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _create_parametric_fx(void) {
    create_parametric_fx(((struct ScriptRawArgs*)current_args)->slots[0].pointer, get_script_string_arg(2));
}

void _create_multiemit_step_fx(void) {
    create_multiemit_step_fx((struct PfxStepEffectDescription*)((struct ScriptRawArgs*)current_args)->slots[0].i,
                             get_script_string_arg(2),
                             ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _create_step_fx(void) {
    create_step_fx((struct PfxStepEffectDescription*)((struct ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2));
}

void _bind_to_bone(void) {
    bind_to_bone(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _create_step_effect(void) {
    create_step_effect(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _parametric_update(void) {
    parametric_update(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _dist_xz_to_xz(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        dist_xz_to_xz(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                      ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _v3_to_xz_ang(void) {
    v3_to_xz_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _v3_to_xy_ang(void) {
    v3_to_xy_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _length_v3(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        length_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _rotate_xz(void) {
    rotate_xz(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _gxMathArcCos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        gxMathArcCos(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _gxMathSin(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        gxMathSin(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _gxMathCos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        gxMathCos(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _uv_v3_to_v3_dist(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        uv_v3_to_v3_dist(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _xz_dot_xz(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        xz_dot_xz(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _YXZ_angles_to_quat(void) {
    YXZ_angles_to_quat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                       ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_get_bone_rot_quat(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_get_bone_rot_quat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _uv_from_angle_y(void) {
    uv_from_angle_y(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _xz_to_y_ang(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        xz_to_y_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _xz_unit_vector(void) {
    xz_unit_vector(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                   ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                   ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _v3_dot_v3(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        v3_dot_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _zero_v3(void) {
    zero_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _normalize_v3(void) {
    normalize_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _scale_v3(void) {
    scale_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _v3_add_v3_scaled(void) {
    v3_add_v3_scaled(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _v3_add_v3(void) {
    v3_add_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _v3_sub_v3(void) {
    v3_sub_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _v3_x_mat(void) {
    v3_x_mat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _mkobj_get_matrix_pos(void) {
    mkobj_get_matrix_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _mkobj_get_matrix_right(void) {
    mkobj_get_matrix_right(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                           ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _mkobj_get_matrix_at(void) {
    mkobj_get_matrix_at(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _mkobj_get_matrix(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        mkobj_get_matrix(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_in_spin_react(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        plyr_in_spin_react(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _force_calc_bone_world_mat(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        force_calc_bone_world_mat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_set_sobj_pos(void) {
    obj_set_sobj_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].i,
                     ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _get_bone_relative_pos(void) {
    get_bone_relative_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                          ((struct ScriptRawArgs*)current_args)->slots[1].i,
                          ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _get_bone_offset_world_pos(void) {
    get_bone_offset_world_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i,
                              ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
                              ((struct ScriptRawArgs*)current_args)->slots[3].pointer);
}

void _get_bone_world_pos(void) {
    get_bone_world_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                       ((struct ScriptRawArgs*)current_args)->slots[1].i,
                       ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _bone_matcher_child_set_offset(void) {
    bone_matcher_child_set_offset(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _bone_matcher_parent_set_offset(void) {
    bone_matcher_parent_set_offset(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                   ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _start_bone_matcher(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        start_bone_matcher(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i,
            ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[3].i,
            ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

void _obj_get_ang_vel(void) {
    obj_get_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_set_ang_vel(void) {
    obj_set_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_set_pos_vel(void) {
    obj_set_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_get_pos_vel(void) {
    obj_get_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _plyr_pdata_is_alt_costume(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_is_alt_costume(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_previous_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_get_previous_state(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_get_state(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_pchr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_pchr(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_sidekick_active(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_sidekick_active(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_get_plyr_num(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_cmo(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_cmo(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_sidekick_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_pdata_get_sidekick_obj(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_his_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_his_obj(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_plyr_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_plyr_obj(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_his_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_pdata_get_his_plyr_pdata(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_pdata_get_plyr_info(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_pdata_get_plyr_info(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _plyr_anim_get_frame(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_anim_get_frame(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _get_my_plyr_anim_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_my_plyr_anim_pdata();
}

void _get_my_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_my_plyr_pdata();
}

void _get_his_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_his_plyr_pdata();
}

void _get_plyr_obj_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_plyr_obj_plyr_num(
        ((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _get_my_sidekick_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_my_sidekick_obj();
}

void _get_my_plyr_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_my_plyr_obj();
}

void _get_his_plyr_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_his_plyr_obj();
}

void _get_plyr_info(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_plyr_info();
}

void _obj_find_sobj_by_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_find_sobj_by_id(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_set_light_flag(void) {
    obj_set_light_flag(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _sobj_get_ang(void) {
    sobj_get_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _sobj_get_ang_vel(void) {
    sobj_get_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _sobj_set_ang_vel(void) {
    sobj_set_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _sobj_set_ang(void) {
    sobj_set_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _sobj_get_pos_vel(void) {
    sobj_get_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _sobj_set_pos_vel(void) {
    sobj_set_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _sobj_get_pos(void) {
    sobj_get_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _sobj_set_pos(void) {
    sobj_set_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_get_ang(void) {
    obj_get_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_set_ang(void) {
    obj_set_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_get_pos(void) {
    obj_get_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_set_pos(void) {
    obj_set_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _obj_set_ground_colls_y(void) {
    obj_set_ground_colls_y(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _obj_set_ground_colls(void) {
    obj_set_ground_colls(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _konquest_load_interior_art(void) {
    konquest_load_interior_art();
}

void _konquest_start_nis_anims_load(void) {
    konquest_start_nis_anims_load(
        get_script_string_arg(1), get_script_string_arg(2));
}

void _konquest_nis_anims_loaded(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = konquest_nis_anims_loaded();
}

void _konquest_nis_end(void) {
    konquest_nis_end();
}

void _konquest_nis_init(void) {
    konquest_nis_init(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _nis_wait_for_region_load(void) {
    nis_wait_for_region_load();
}

void _wait_for_region_load(void) {
    wait_for_region_load();
}

void _nis_register_participant(void) {
    nis_register_participant(((struct ScriptRawArgs*)current_args)->slots[0].i,
                             ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _nis_set_wait_override(void) {
    nis_set_wait_override(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _nis_show_cancel_message(void) {
    nis_show_cancel_message();
}

void _nis_scene_done(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = nis_scene_done();
}

void _nis_end(void) {
    nis_end();
}

void _nis_wait_for_event(void) {
    nis_wait_for_event(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _nis_signal_event(void) {
    nis_signal_event(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _get_fatality_state_ptr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_fatality_state_ptr();
}

void _animpdata_ani_to_frame_x_with_flag_check(void) {
    animpdata_ani_to_frame_x_with_flag_check(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                             ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                             ((struct ScriptRawArgs*)current_args)->slots[2].i,
                                             ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

void _mkscripts_set_anim_check_flag(void) {
    mkscripts_set_anim_check_flag(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _mkscripts_mkobj_insert_mkobj_cleanuplist(void) {
    mkscripts_mkobj_insert_mkobj_cleanuplist(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _mkscripts_destroy_gusher(void) {
    mkscripts_destroy_gusher(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _mkscripts_destroy_fk_bonematcher(void) {
    mkscripts_destroy_fk_bonematcher(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _mkscripts_destroy_bonematcher(void) {
    mkscripts_destroy_bonematcher(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _dkp_check_plyr_state_for_grab(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        dkp_check_plyr_state_for_grab(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _start_prison_grab_proc(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        start_prison_grab_proc(((struct ScriptRawArgs*)current_args)->slots[0].i,
                               ((struct ScriptRawArgs*)current_args)->slots[1].i,
                               ((struct ScriptRawArgs*)current_args)->slots[2].f,
                               ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

void _done_prison_grab_proc(void) { done_prison_grab_proc(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _kill_spear(void) { kill_spear(); }

void _xfer_spearproc_to_retract(void) { xfer_spearproc_to_retract(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _destroy_spearproc_bonematcher(void) { destroy_spearproc_bonematcher(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _insert_mkobj_spearproc_parentobjitem(void) {
    insert_mkobj_spearproc_parentobjitem(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _get_spearobj_from_spearproc(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_spearobj_from_spearproc(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _fire_sc_spear(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fire_sc_spear(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                      ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                      ((struct ScriptRawArgs*)current_args)->slots[4].i, ((struct ScriptRawArgs*)current_args)->slots[5].i);
}

void _subzero_start_ice_chunks(void) { subzero_start_ice_chunks(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _subzero_start_iceman(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = subzero_start_iceman();
}

void _subzero_start_iceblock(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = subzero_start_iceblock();
}

void _sindel_scream_react_sound_start(void) { sindel_scream_react_sound_start(); }

void _sindel_sonic_sounds(void) { sindel_sonic_sounds(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

void _sindel_sonic_waves(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = sindel_sonic_waves(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

void _start_raiden_lightning_scroll(void) {
    start_raiden_lightning_scroll(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[2].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[3].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _ft_raiden_summon_lightning_bolt(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        ft_raiden_summon_lightning_bolt(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                        ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                        get_script_string_arg(3));
}

void _kill_raiden_summon_lightning_bolt(void) { kill_raiden_summon_lightning_bolt(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _fix_axe_angle(void) { fix_axe_angle(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _ft_mileena_start_veil_ripoff(void) { ft_mileena_start_veil_ripoff(); }

void _fat_goro_fold_arms(void) {
    fat_goro_fold_arms(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                       ((struct ScriptRawArgs*)current_args)->slots[2].f, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

void _fatality_boraicho_get_jug(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_get_jug(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _fatality_boraicho_light_fart_torch(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_light_fart_torch(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _fatality_boraicho_get_torch(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_get_torch(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _show_baraka_one_blade_only(void) {
    show_baraka_one_blade_only(((struct ScriptRawArgs*)current_args)->slots[0].i,
                               ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _fatality_ashrah_get_doll(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_ashrah_get_doll(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                 ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                 ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _fire_multi_emitter_pfx_via_tbl(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);
    fire_multi_emitter_pfx_via_tbl(
        name, args->slots[1].pointer,
        args->slots[2].pointer, args->slots[3].pointer);
}

void _pfxhandle_spawn_at_bid_next_bind_render(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_spawn_at_bid_next_bind_render(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                                ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                                ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _pfxhandle_bgnd_spawn_at_sobj_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_bgnd_spawn_at_sobj_id(get_script_string_arg(1),
                                        ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _pfxhandle_spawn_at_bid(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_spawn_at_bid(name, args->slots[1].pointer, args->slots[2].i);
}

void _pfxhandle_spawn_at_bid_next(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_spawn_at_bid_next(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                    ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _pfx_spawn_at_bid(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);

    pfx_spawn_at_bid(name, args->slots[1].i, args->slots[2].i);
}

void _limb_sever_throw_away(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        limb_sever_throw_away(((struct ScriptRawArgs*)current_args)->slots[0].i,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i,
                              ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _auto_calc_limbobj_bone_world_pos(void) {
    auto_calc_limbobj_bone_world_pos(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _limb_sever_show_z_meat_chunks(void) {
    limb_sever_show_z_meat_chunks(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_show_z_meat_chunks_all(void) { limb_sever_show_z_meat_chunks_all(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _limb_sever_show_z_meat_chunks_all_plyr_num(void) { limb_sever_show_z_meat_chunks_all_plyr_num(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _limb_sever_explode_apart_plyr_num(void) {
    limb_sever_explode_apart_plyr_num(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                      ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                      ((struct ScriptRawArgs*)current_args)->slots[2].f,
                                      ((struct ScriptRawArgs*)current_args)->slots[3].f,
                                      ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _reset_blood_decals(void) { reset_blood_decals(); }

void _destroy_gore2_obj(void) {
    destroy_gore2_obj(((struct ScriptRawArgs*)current_args)->slots[0].i,
                     ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _attach_gore2_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        attach_gore2_obj(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                        ((struct ScriptRawArgs*)current_args)->slots[1].i,
                        ((struct ScriptRawArgs*)current_args)->slots[2].i,
                        ((struct ScriptRawArgs*)current_args)->slots[3].pointer,
                        ((struct ScriptRawArgs*)current_args)->slots[4].pointer);
}

void _start_gore2_pebbles(void) {
    struct Gore2PebbleArgs* args;

    args = (struct Gore2PebbleArgs*)current_args;
    start_gore2_pebbles(
        args->object_id,
        args->bone,
        args->source,
        args->decal_owner,
        args->velocity,
        args->rotation,
        args->scale,
        args->position_offset,
        args->vertical_acceleration,
        args->bounce_scale,
        args->bounce_count);
}

void _start_bodyslam_bodysplat(void) {
    start_bodyslam_bodysplat(((struct ScriptRawArgs*)current_args)->slots[0].f,
                             ((struct ScriptRawArgs*)current_args)->slots[1].f,
                             ((struct ScriptRawArgs*)current_args)->slots[2].f,
                             ((struct ScriptRawArgs*)current_args)->slots[3].f,
                             ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

void _load_cloth_boned_model(void) {
    struct ScriptRawArgs* args;
    const char* name;

    args = (struct ScriptRawArgs*)current_args;
    name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        load_cloth_boned_model(
            name, args->slots[1].i, args->slots[2].i,
            args->slots[3].i, args->slots[4].i,
            args->slots[5].i, args->slots[6].i);
}

void _fatality_explode_victim(void) {
    fatality_explode_victim(((struct ScriptRawArgs*)current_args)->slots[0].i,
                            ((struct ScriptRawArgs*)current_args)->slots[1].f,
                            ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

void _kill_gusher(void) { kill_gusher(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _start_sweat_particles_scripts(void) { start_sweat_particles_scripts(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

void _start_blood_particles_scripts(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.u =
        start_blood_particles_scripts(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                      ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _start_sweat_particles(void) {
    start_sweat_particles(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                          ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

void _start_blood_particles(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        start_blood_particles(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                              ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

void _mks_spawn_blood_pool_at_bid(void) {
    mks_spawn_blood_pool_at_bid(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

void _spawn_blood_pool_at_bid(void) {
    spawn_blood_pool_at_bid(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                            ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _spawn_bld_splat(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);

    spawn_bld_splat(name, args->slots[1].pointer, args->slots[2].pointer);
}

void _plyr_weapon2_release(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_weapon2_release(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _plyr_weapon_release(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_weapon_release(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _bone_matcher_reset_dest_mat_rot(void) { bone_matcher_reset_dest_mat_rot(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

void _bone_matcher_set_ang_pos(void) {
    bone_matcher_set_ang_pos(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                             ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                             ((struct ScriptRawArgs*)current_args)->slots[4].i, ((struct ScriptRawArgs*)current_args)->slots[5].i);
}

void _weapon_bm_ignore(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        weapon_bm_ignore(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _regrab_weapon(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        regrab_weapon(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                      ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                      ((struct ScriptRawArgs*)current_args)->slots[4].i, ((struct ScriptRawArgs*)current_args)->slots[5].i,
                      ((struct ScriptRawArgs*)current_args)->slots[6].i);
}

void _weapon_reflection_show_hide(void) {
    weapon_reflection_show_hide(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _show_single_weapon(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        show_single_weapon(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _clone_my_weapon(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        clone_my_weapon(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _clone_weapon_to_secondary(void) {
    clone_weapon_to_secondary(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _advance_to_weapon_style(void) { advance_to_weapon_style(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _is_weapon_style(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_weapon_style(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _am_i_female(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_female(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _sobj_set_bounding_sphere_radius(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        sobj_set_bounding_sphere_radius(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                        ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _sobj_get_bounding_sphere_radius(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        sobj_get_bounding_sphere_radius(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _play_his_snd_req(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = play_his_snd_req(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _play_his_random_voice(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = play_his_random_voice(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _obj_unhide_material_by_id(void) {
    obj_unhide_material_by_id(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_hide_material_by_id(void) {
    obj_hide_material_by_id(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _bm_force_fake_child_bid(void) {
    bm_force_fake_child_bid(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _fat_bgnd_char_setup_radius_check(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fat_bgnd_char_setup_radius_check(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _set_victim_v3_units_away(void) {
    set_victim_v3_units_away(((struct ScriptRawArgs*)current_args)->slots[0].f,
                             ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _reset_fake_bone_matcher(void) {
    struct FakeBoneMatcherResetArgs* args;

    args = (struct FakeBoneMatcherResetArgs*)current_args;
    reset_fake_bone_matcher(
        args->matcher,
        args->parent_offset,
        args->child_offset,
        args->rotation,
        args->bone_index,
        args->object,
        args->blend);
}

void _ft_fake_bone_matcher(void) {
    struct FakeBoneMatcherArgs* args;
    struct ScriptPointerResult* result;
    MkHdr* matcher;

    args = (struct FakeBoneMatcherArgs*)current_args;
    matcher = ft_fake_bone_matcher(
        args->parent,
        args->child,
        args->bone_index,
        args->parent_offset,
        args->child_offset,
        args->rotation,
        args->mode,
        args->blend);
    result = (struct ScriptPointerResult*)active_cmdscript;
    result->value = matcher;
}

void _get_game_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_game_state();
}

void _fatality_release_other_player(void) { fatality_release_other_player(); }

void _get_level_fatality_done_flag_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        get_level_fatality_done_flag_state();
}

void _set_level_fatality_done_flag_state(void) { set_level_fatality_done_flag_state(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _limb_sever_destroy_existing_attach_proc(void) {
    limb_sever_destroy_existing_attach_proc(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _limb_sever_bone_attach(void) {
    struct LimbBoneAttachArgs* args;

    args = (struct LimbBoneAttachArgs*)current_args;
    limb_sever_bone_attach(
        args->target_player,
        args->owner_bone,
        args->offset,
        args->rotation,
        args->owner_player,
        args->limb,
        args->target_bone,
        args->include_children);
}

void _limb_sever_pop_head_up(void) {
    struct PopHeadArgs* args;
    struct ScriptPointerResult* result;
    MkHdr* head;

    args = (struct PopHeadArgs*)current_args;
    head = limb_sever_pop_head_up(
        args->player,
        args->x_velocity,
        args->y_velocity,
        args->z_velocity,
        args->angular_velocity);
    result = (struct ScriptPointerResult*)active_cmdscript;
    result->value = head;
}

void _mks_limb_sever(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        mks_limb_sever(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                       ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_find_existing_update_proc(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        limb_sever_find_existing_update_proc(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                              ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                              ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _limb_sever_set_motion(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = limb_sever_set_motion(
        args->slots[0].pointer, args->slots[1].i, args->slots[2].pointer,
        args->slots[3].f, args->slots[4].pointer, args->slots[5].i,
        args->slots[6].f, args->slots[7].i, args->slots[8].f,
        args->slots[9].i, args->slots[10].i);
}

void _limb_sever_update_slide_end_coeff(void) { limb_sever_update_slide_end_coeff(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _proc_of_anim_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = proc_of_anim_pdata(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _set_pdata_anim_step(void) { set_pdata_anim_step(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _puzzle_fighter_scale(void) { puzzle_fighter_scale(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _plyr_turn_on_shadowbox(void) { plyr_turn_on_shadowbox(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _plyr_turn_off_shadowbox(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_turn_off_shadowbox(args->slots[0].pointer);
}

void _plyr_turn_on_mirrorguy(void) { plyr_turn_on_mirrorguy(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _plyr_turn_off_mirrorguy(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_turn_off_mirrorguy(args->slots[0].pointer);
}

void _animpdata_ani_to_blend_frame(void) { animpdata_ani_to_blend_frame(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_to_end_at1(void) { animpdata_ani_to_end_at1(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _animpdata_ani_to_end(void) { animpdata_ani_to_end(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _animpdata_ani_x_more_frames(void) { animpdata_ani_x_more_frames(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_loop_more_frames(void) { animpdata_ani_loop_more_frames(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_to_frame_x(void) { animpdata_ani_to_frame_x(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _mks_animpdata_set_cur_frame(void) { mks_animpdata_set_cur_frame(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

void _animpdata_ani_1_frame(void) { animpdata_ani_1_frame(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

void _check_to_register_miss(void) { check_to_register_miss(); }

void _auto_ani_off(void) { auto_ani_off(); }

void _ncs_bgnd_preload_named_model(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        ncs_bgnd_preload_named_model(
            get_script_string_arg(1), get_script_string_arg(2),
            args->slots[2].i, args->slots[3].i,
            args->slots[4].pointer, args->slots[5].pointer,
            args->slots[6].pointer);
}

void _ncs_dkp_camera_konqchar_show_hide_alpha(void) {
    ncs_dkp_camera_konqchar_show_hide_alpha(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                            ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _ncs_camera_wall_show_hide_alpha(void) {
    ncs_camera_wall_show_hide_alpha(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata();
}

void _ncs_bgnd_nuke_collision_to_script_interface(void) { ncs_bgnd_nuke_collision_to_script_interface(); }

void _retrieve_bgnd_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = retrieve_bgnd_obj();
}

void _fkbm_obj_face_obj(void) {
    fkbm_obj_face_obj(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                      ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                      ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

void _obj_grnd_bounce(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_grnd_bounce(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].f, args->slots[3].f, args->slots[4].i, args->slots[5].f);
}

void _start_obj_scalar_proc(void) {
    start_obj_scalar_proc(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                          ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

void _obj_match_obj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_match_obj_pos(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].f, args->slots[3].i);
}

void _insert_particle_mkobj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        insert_particle_mkobj(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _mkobj_pos_pos_dot_normal_xz(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        mkobj_pos_pos_dot_normal_xz(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _obj_get_bid_for_tid(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        obj_get_bid_for_tid(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _obj_create_sobjs_by_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_create_sobjs_by_id(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _unhide_sobj_by_sobj_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        unhide_sobj_by_sobj_id(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _hide_sobj_by_sobj_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        hide_sobj_by_sobj_id(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _sobj_set_priority(void) { sobj_set_priority(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

void _unhide_sobj(void) { unhide_sobj(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _hide_sobj(void) { hide_sobj(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _unhide_obj(void) { unhide_obj(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _hide_obj(void) { hide_obj(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _pfx_plyr_bankowner(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        pfx_plyr_bankowner(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

void _ncs_script_debug_quickie(void) {
    ncs_script_debug_quickie(((struct ScriptRawArgs*)current_args)->slots[0].i,
                             ((struct ScriptRawArgs*)current_args)->slots[1].f,
                             ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _get_inverse_game_speed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_inverse_game_speed();
}

void _ck_rumble_controller(void) {
    ck_rumble_controller(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                         ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _check_for_green_light(void) { ((struct ScriptRawResult*)active_cmdscript)->value.i = check_for_green_light(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _check_for_red_light(void) { ((struct ScriptRawResult*)active_cmdscript)->value.i = check_for_red_light(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _ck_put_weapon_away(void) { ck_put_weapon_away(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _find_obj_by_id(void) { ((struct ScriptRawResult*)active_cmdscript)->value.pointer = find_obj_by_id(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _yinyang_reset_music_index(void) { yinyang_reset_music_index(); }

void _yinyang_play_evil_tune(void) { yinyang_play_evil_tune(); }

void _yinyang_play_good_tune(void) { yinyang_play_good_tune(); }

void _dist_v3_to_v3(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        dist_v3_to_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _is_point_in_fortress_exclusion_zone(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        is_point_in_fortress_exclusion_zone(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

void _fortress_setup_exclusion_zone(void) {
    fortress_setup_exclusion_zone(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[2].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[3].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

void _set_evil_swap_status(void) { set_evil_swap_status(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _ok_to_do_evil_swap(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = ok_to_do_evil_swap();
}

void _set_evil_condition(void) { set_evil_condition(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _yy_is_evil_time_active(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = yy_is_evil_time_active();
}

void _bgnd_make_object_transl(void) { bgnd_make_object_transl(((struct ScriptRawArgs*)current_args)->slots[0].i); }

void _are_death_traps_on(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = are_death_traps_on();
}

void _get_string_by_id(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptStringResult* result;
    char* string;

    args = (struct ScriptSingleIntArgs*)current_args;
    string = get_string_by_id(args->value);
    result = (struct ScriptStringResult*)active_cmdscript;
    result->value = string;
}

void _ending_show_text(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    ending_show_text(args->first, args->second);
}

void _ending_show_image(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    ending_show_image(args->value);
}

void _cam_set_intro_cam_speed(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    cam_set_intro_cam_speed(args->value);
}

void _midpoint_v3(void) {
    struct ScriptThreeVecArgs* args;

    args = (struct ScriptThreeVecArgs*)current_args;
    midpoint_v3(args->out, args->first,
                args->second);
}

void _sobj_get_world_pos(void) {
    struct ScriptSobjArgs* args;
    struct ScriptVecResult* result;
    Vec* position;

    args = (struct ScriptSobjArgs*)current_args;
    position = sobj_get_world_pos(args->value);
    result = (struct ScriptVecResult*)active_cmdscript;
    result->value = position;
}

void _get_point_on_circle(void) {
    struct ScriptCircleArgs* args;

    args = (struct ScriptCircleArgs*)current_args;
    get_point_on_circle(args->center, args->radius,
                        args->angle, args->out);
}

void _cut_player_in_half(void) {
    struct ScriptObjectArgs* args;
    struct ScriptPointerResult* result;
    MkHdr* upper_body;

    args = (struct ScriptObjectArgs*)current_args;
    upper_body = cut_player_in_half(args->object);
    result = (struct ScriptPointerResult*)active_cmdscript;
    result->value = upper_body;
}

void _is_plyr_airborn(void) {
    struct ScriptAirborneArgs* args;
    struct ScriptIntResult* result;
    int airborne;

    args = (struct ScriptAirborneArgs*)current_args;
    airborne = is_plyr_airborn(args->object, args->player);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = airborne;
}

void _get_offset_of_closest_fence_section(void) {
    struct ScriptFenceArgs* args;
    struct ScriptIntResult* result;
    int offset;

    args = (struct ScriptFenceArgs*)current_args;
    offset = get_offset_of_closest_fence_section(
        args->point, args->sections, args->start_index,
        args->mirrored);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = offset;
}

void _find_mkproc_pid(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptProcResult* result;
    MkProc* proc;

    args = (struct ScriptSingleIntArgs*)current_args;
    proc = find_mkproc_pid(args->value);
    result = (struct ScriptProcResult*)active_cmdscript;
    result->value = proc;
}

void _start_fish_attack(void) {
    struct ScriptFishAttackArgs* args;

    args = (struct ScriptFishAttackArgs*)current_args;
    start_fish_attack(args->player, args->direction,
                      args->flags);
}

void _is_timer_off(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_timer_off();
}

void _hide_sobj_if_camera_is_in_rectangle(void) {
    struct ScriptCameraRectangleArgs* args;

    args = (struct ScriptCameraRectangleArgs*)current_args;
    hide_sobj_if_camera_is_in_rectangle(
        args->object, args->center,
        args->min_x, args->min_z,
        args->max_x, args->max_z);
}

void _hide_sobj_if_camera_is_in_cylinder(void) {
    struct ScriptCameraCylinderArgs* args;

    args = (struct ScriptCameraCylinderArgs*)current_args;
    hide_sobj_if_camera_is_in_cylinder(
        args->object, args->center,
        args->radius, args->height);
}

void _turn_off_sobj_if_camera_is_in_rectangle(void) {
    struct ScriptCameraRectangleArgs* args;

    args = (struct ScriptCameraRectangleArgs*)current_args;
    turn_off_sobj_if_camera_is_in_rectangle(
        args->object, args->center,
        args->min_x, args->min_z,
        args->max_x, args->max_z);
}

void _turn_off_sobj_if_camera_is_in_cylinder(void) {
    struct ScriptCameraCylinderArgs* args;

    args = (struct ScriptCameraCylinderArgs*)current_args;
    turn_off_sobj_if_camera_is_in_cylinder(
        args->object, args->center,
        args->radius, args->height);
}

void _debug_create_axis_indicator(void) {
    struct ScriptAxisArgs* args;

    args = (struct ScriptAxisArgs*)current_args;
    debug_create_axis_indicator(args->player, args->axis);
}

void _bgnd_level_fatality_end(void) {
    bgnd_level_fatality_end();
}

void _bgnd_level_fatality_start(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_level_fatality_start(args->value);
}

void _bgnd_level_transition_end(void) {
    bgnd_level_transition_end();
}

void _bgnd_level_transition_start(void) {
    bgnd_level_transition_start();
}

void _do_lightning_strike(void) {
    struct ScriptLightningArgs* args;

    args = (struct ScriptLightningArgs*)current_args;
    do_lightning_strike(args->owner, args->position);
}

void _obj_set_sobj_alpha(void) {
    struct ScriptSobjAlphaArgs* args;

    args = (struct ScriptSobjAlphaArgs*)current_args;
    obj_set_sobj_alpha(args->object, args->sobj_index,
                       args->alpha);
}

void _obj_for_all_atomics_set_material_alpha(void) {
    struct ScriptObjectIntArgs* args;

    args = (struct ScriptObjectIntArgs*)current_args;
    obj_for_all_atomics_set_material_alpha(args->object,
                                           args->value);
}

void _get_volume_from_distance(void) {
    struct ScriptVolumeArgs* args;
    struct ScriptFloatResult* result;
    float volume;

    args = (struct ScriptVolumeArgs*)current_args;
    volume = get_volume_from_distance(
        args->position, args->far_distance,
        args->near_distance);
    result = (struct ScriptFloatResult*)active_cmdscript;
    result->value = volume;
}

void _get_pan_value(void) {
    struct ScriptFloatPointerArgs* args;
    struct ScriptFloatResult* result;
    float pan;

    args = (struct ScriptFloatPointerArgs*)current_args;
    pan = get_pan_value((const Vec*)args->value);
    result = (struct ScriptFloatResult*)active_cmdscript;
    result->value = pan;
}

void _mab_test(void) {
    mab_test();
}

void _player_body_explode(void) {
    struct ScriptBodyExplodeArgs* args;

    args = (struct ScriptBodyExplodeArgs*)current_args;
    player_body_explode(args->player,
                        args->position,
                        args->velocity);
}

void _bgnd_clear_danger_zone_callback(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_clear_danger_zone_callback((PlyrPdata*)args->value);
}

void _yinyang_set_bad_fish_hide_flag(void) {
    struct ScriptFishFlagArgs* args;

    args = (struct ScriptFishFlagArgs*)current_args;
    yinyang_set_bad_fish_hide_flag(
        args->entries, args->hidden,
        args->count);
}

void _yinyang_set_good_fish_hide_flag(void) {
    struct ScriptFishFlagArgs* args;

    args = (struct ScriptFishFlagArgs*)current_args;
    yinyang_set_good_fish_hide_flag(
        args->entries, args->hidden,
        args->count);
}

void _yinyang_make_fish_jump(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    yinyang_make_fish_jump(args->first, args->second);
}

void _destroy_mkobjs_oid(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    destroy_mkobjs_oid(args->value);
}

void _do_yinyang_statue_explosion(void) {
    struct ScriptObjectArgs* args;

    args = (struct ScriptObjectArgs*)current_args;
    do_yinyang_statue_explosion(args->object);
}

void _obj_add_to_skinned_obj_light_list_with_ambient(void) {
    struct ScriptLightListArgs* args;

    args = (struct ScriptLightListArgs*)current_args;
    obj_add_to_skinned_obj_light_list_with_ambient(
        args->object, args->light);
}

void _obj_change_to_bgnd_obj_light_list(void) {
    struct ScriptLightListArgs* args;

    args = (struct ScriptLightListArgs*)current_args;
    obj_change_to_bgnd_obj_light_list(args->object,
                                      args->light);
}

void _obj_change_to_skinned_obj_light_list(void) {
    struct ScriptLightListArgs* args;

    args = (struct ScriptLightListArgs*)current_args;
    obj_change_to_skinned_obj_light_list(args->object,
                                         args->light);
}

void _yinyang_stop_lensflare(void) {
    yinyang_stop_lensflare();
}

void _yinyang_start_lensflare(void) {
    yinyang_start_lensflare();
}

void _misc_data_set_test_float(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    misc_data_set_test_float(args->value);
}

void _misc_data_set_test_u32(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    misc_data_set_test_u32(args->value);
}

void _obj_set_sobj_priority(void) {
    struct ScriptSobjAlphaArgs* args;

    args = (struct ScriptSobjAlphaArgs*)current_args;
    obj_set_sobj_priority(args->object,
                          args->sobj_index,
                          args->alpha);
}

void _sobj_disable_blending(void) {
    struct ScriptSobjArgs* args;

    args = (struct ScriptSobjArgs*)current_args;
    sobj_disable_blending(args->value);
}

void _obj_first_sobj(void) {
    struct ScriptMkObjArgs* args;
    struct ScriptSobjResult* result;
    struct SObj* object;

    args = (struct ScriptMkObjArgs*)current_args;
    object = obj_first_sobj(args->object);
    result = (struct ScriptSobjResult*)active_cmdscript;
    result->value = object;
}

void _pebble_turn_culling_off(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    pebble_turn_culling_off(args->value);
}

void _pebble_turn_culling_on(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    pebble_turn_culling_on(args->value);
}

void _pebble_unhide_me(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    pebble_unhide_me(args->first, args->second);
}

void _pebble_hide_me(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    pebble_hide_me(args->first, args->second);
}

void _pebble_setup_bounce_props(void) {
    struct ScriptPebbleBounceArgs* args;

    args = (struct ScriptPebbleBounceArgs*)current_args;
    pebble_setup_bounce_props(
        args->player, args->index,
        args->velocity, args->flags);
}

void _pebble_set_ang_vel(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_ang_vel(args->player, args->index,
                       args->value);
}

void _pebble_set_ang(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_ang(args->player, args->index,
                   args->value);
}

void _pebble_set_scale(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_scale(args->player, args->index,
                     args->value);
}

void _pebble_set_vel(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_vel(args->player, args->index,
                   args->value);
}

void _pebble_set_pos(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_pos(args->player, args->index,
                   (Vec*)args->value);
}

void _destroy_mkobj(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    destroy_mkobj(args->object);
}

void _obj_turn_gravity_off(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    obj_turn_gravity_off(args->object);
}

void _obj_set_gravity(void) {
    struct ScriptMkObjFloatArgs* args;

    args = (struct ScriptMkObjFloatArgs*)current_args;
    obj_set_gravity(args->object, args->value);
}

void _insert_fgnd_mkobj(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    insert_fgnd_mkobj(args->object);
}

void _get_player_number(void) {
    struct ScriptMkObjArgs* args;
    struct ScriptIntResult* result;
    int player;

    args = (struct ScriptMkObjArgs*)current_args;
    player = get_player_number(args->object);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = player;
}

void _load_named_model_for_player(void) {
    struct ScriptLoadPlayerModelArgs* args;
    struct ScriptMkObjResult* result;
    char* name;
    MkObj* object;

    args = (struct ScriptLoadPlayerModelArgs*)current_args;
    name = get_script_string_arg(1);
    object = load_named_model_for_player(
        name, args->player,
        args->object_type,
        args->flags);
    result = (struct ScriptMkObjResult*)active_cmdscript;
    result->value = object;
}

void _load_named_model_from_slot(void) {
    struct ScriptLoadSlotModelArgs* saved;
    struct ScriptLoadSlotModelArgs* current;
    struct ScriptMkObjResult* result;
    char* name;
    MkObj* object;

    saved = (struct ScriptLoadSlotModelArgs*)current_args;
    name = get_script_string_arg(2);
    current = (struct ScriptLoadSlotModelArgs*)current_args;
    object = load_named_model_from_slot(
        current->slot, name, saved->flags,
        saved->user_data);
    result = (struct ScriptMkObjResult*)active_cmdscript;
    result->value = object;
}

void _script_fabs(void) {
    struct ScriptSingleFloatArgs* args;
    struct ScriptFloatResult* result;
    float value;

    args = (struct ScriptSingleFloatArgs*)current_args;
    value = script_fabs(args->value);
    result = (struct ScriptFloatResult*)active_cmdscript;
    result->value = value;
}

void _set_obj_light_flags(void) {
    struct ScriptObjectIntArgs* args;

    args = (struct ScriptObjectIntArgs*)current_args;
    set_obj_light_flags(args->object, args->value);
}

void _set_obj_ang(void) {
    struct ScriptMkObjVecArgs* args;

    args = (struct ScriptMkObjVecArgs*)current_args;
    set_obj_ang(args->object, args->x,
                args->y, args->z);
}

void _set_obj_pos(void) {
    struct ScriptMkObjVecArgs* args;

    args = (struct ScriptMkObjVecArgs*)current_args;
    set_obj_pos(args->object, args->x,
                args->y, args->z);
}

void _bgnd_unhide_sobj_list(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_unhide_sobj_list((unsigned int*)args->value);
}

void _bgnd_hide_sobj_list(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_hide_sobj_list((unsigned int*)args->value);
}

void _random_percent(void) {
    struct ScriptSingleFloatArgs* args;
    struct ScriptIntResult* result;
    int value;

    args = (struct ScriptSingleFloatArgs*)current_args;
    value = random_percent(args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = value;
}

void _bgnd_sobj_start_morph(void) {
    struct ScriptMorphArgs* args;

    args = (struct ScriptMorphArgs*)current_args;
    bgnd_sobj_start_morph(args->object, args->sobj_id,
                          args->script_id, args->flags);
}

void _delete_screen_obj_oid(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    delete_screen_obj_oid(args->value);
}

void _reset_collision_system(void) {
    reset_collision_system();
}

void _pos_cam_for_current_level(void) {
    pos_cam_for_current_level();
}

void _reset_severed_limbs(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    reset_severed_limbs(args->value);
}

void _move_plyrs_to_round_start(void) {
    move_plyrs_to_round_start();
}

void _set_far_clip_plane(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    set_far_clip_plane(args->value);
}

void _skytemple_player_explode(void) {
    struct ScriptSkytempleExplodeArgs* args;

    args = (struct ScriptSkytempleExplodeArgs*)current_args;
    skytemple_player_explode(
        args->player, args->x,
        args->y, args->z);
}

void _skytemple_make_scream_sound(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    skytemple_make_scream_sound(args->value);
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
    struct ScriptPebbleArrangeArgs* args;

    args = (struct ScriptPebbleArrangeArgs*)current_args;
    skytemple_arrange_fence_pebbles_around_pos(
        args->player, args->count,
        args->position);
}

void _skytemple_set_fence_pebble_vel(void) {
    struct ScriptPebbleVelocityArgs* args;

    args = (struct ScriptPebbleVelocityArgs*)current_args;
    skytemple_set_fence_pebble_vel(
        args->player, args->count,
        args->x, args->y,
        args->z);
}

void _scripted_camera_script_exit(void) {
    scripted_camera_script_exit();
}

void _bgnd_launch_fx_at_sobj_pos(void) {
    struct ScriptNamedSobjFxArgs* args;
    char* name;

    args = (struct ScriptNamedSobjFxArgs*)current_args;
    name = get_script_string_arg(1);
    bgnd_launch_fx_at_sobj_pos(name, args->sobj_id,
                               args->y_offset);
}

void _bgnd_get_first_shape_center_for_obstacle_id(void) {
    struct ScriptTwoIntArgs* args;
    struct ScriptIntResult* result;
    int found;

    args = (struct ScriptTwoIntArgs*)current_args;
    found = bgnd_get_first_shape_center_for_obstacle_id(
        args->first, args->second);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = found;
}

void _misc_data_set_obj_ptr1(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    misc_data_set_obj_ptr1(args->object);
}

void _misc_data_get_obj_ptr1(void) {
    struct ScriptMkObjResult* result;
    MkObj* object;

    object = misc_data_get_obj_ptr1();
    result = (struct ScriptMkObjResult*)active_cmdscript;
    result->value = object;
}

void _misc_data_set_col_obj_id2(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    misc_data_set_col_obj_id2(args->value);
}

void _misc_data_get_col_obj_id2(void) {
    struct ScriptIntResult* result;
    int object_id;

    object_id = misc_data_get_col_obj_id2();
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = object_id;
}

void _misc_data_set_col_obj_id(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    misc_data_set_col_obj_id(args->value);
}

void _misc_data_get_col_obj_id(void) {
    struct ScriptIntResult* result;
    int object_id;

    object_id = misc_data_get_col_obj_id();
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = object_id;
}

void _bgnd_fetch_obj(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptMkObjResult* result;
    MkObj* object;

    args = (struct ScriptSingleIntArgs*)current_args;
    object = bgnd_fetch_obj(args->value);
    result = (struct ScriptMkObjResult*)active_cmdscript;
    result->value = object;
}

void _bgnd_swap_textures_tbl(void) {
    struct ScriptSwapTextureTableArgs* args;

    args = (struct ScriptSwapTextureTableArgs*)current_args;
    bgnd_swap_textures_tbl(args->entries,
                           args->frame);
}

void _bgnd_swap_textures(void) {
    struct ScriptSwapTextureArgs* args;

    args = (struct ScriptSwapTextureArgs*)current_args;
    bgnd_swap_textures(args->sobj_id,
                       args->material_id,
                       args->frame);
}

void _bgnd_append_texture_to_material_tbl(void) {
    struct ScriptAppendTextureTableArgs* args;

    args = (struct ScriptAppendTextureTableArgs*)current_args;
    bgnd_append_texture_to_material_tbl(
        args->entries);
}

void _bgnd_append_texture_to_material(void) {
    struct ScriptAppendTextureArgs* args;

    args = (struct ScriptAppendTextureArgs*)current_args;
    bgnd_append_texture_to_material(
        args->sobj_id, args->material_id,
        get_script_string_arg(3), args->texture_slot);
}

void _bgnd_light_set_color(void) {
    struct ScriptLightColorArgs* args;

    args = (struct ScriptLightColorArgs*)current_args;
    bgnd_light_set_color(args->light_id, args->red,
                         args->green, args->blue);
}

void _bgnd_get_float(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptFloatResult* result;
    float value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = bgnd_get_float(args->value);
    result = (struct ScriptFloatResult*)active_cmdscript;
    result->value = value;
}

void _bgnd_get_u32(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptIntResult* result;
    unsigned int value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = bgnd_get_u32(args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = value;
}

void _bgnd_get_int(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptIntResult* result;
    int value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = bgnd_get_int(args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = value;
}

void _mks_start_fatality_iceball(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    mks_start_fatality_iceball(args->value);
}

void _send_speedup_msg(void) {
}

void _mks_debug_display_cloth_ontop(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    mks_debug_display_cloth_ontop(args->value);
}

void _start_bow(void) {
    struct ScriptIntFloatArgs* args;

    args = (struct ScriptIntFloatArgs*)current_args;
    start_bow(args->integer, args->floating);
}

void _get_bid_with_flip(void) {
    struct ScriptObjectIntArgs* args;
    struct ScriptIntResult* result;
    int bone_id;

    args = (struct ScriptObjectIntArgs*)current_args;
    bone_id = get_bid_with_flip(args->object,
                                args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = bone_id;
}

void _start_plyr_attack(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    start_plyr_attack(args->value);
}

void _set_active_projectile_upward_attack(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_upward_attack(args->vector);
}

void _set_active_add_ang_y(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    set_active_add_ang_y(args->value);
}

void _set_active_projectile_random_rot(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    set_active_projectile_random_rot(
        args->x, args->y, args->z);
}

void _set_active_projectile_dn_sound(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    set_active_projectile_dn_sound(args->value);
}

void _set_active_projectile_sound(void) {
    struct ScriptThreeIntArgs* args;

    args = (struct ScriptThreeIntArgs*)current_args;
    set_active_projectile_sound(args->first, args->second,
                                args->third);
}

void _set_active_projectile_random_pos(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    set_active_projectile_random_pos(
        args->x, args->y, args->z);
}

void _set_active_projectile_velocity_damp(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_velocity_damp(args->vector);
}

void _set_active_projectile_target_pos(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_target_pos(args->vector);
}

void _set_active_projectile_max_ticks(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    set_active_projectile_max_ticks(args->value);
}

void _set_active_projectile_p_handler(void) {
    struct ScriptEntryArgs* args;

    args = (struct ScriptEntryArgs*)current_args;
    set_active_projectile_p_handler((MkProcEntryFn)args->entry);
}

void _set_active_projectile_target_ground(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    set_active_projectile_target_ground(
        args->x, args->y, args->z);
}

void _set_active_projectile_velocity(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_velocity(args->vector);
}

void _active_projectile_setup_done(void) {
    active_projectile_setup_done();
}

void _set_active_projectile_hit_gnd_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_hit_gnd_script(args->slots[0].u);
}

void _set_active_projectile_end_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_end_script(args->slots[0].u);
}

void _set_active_projectile_hit_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_hit_script(args->slots[0].u);
}

void _set_active_projectile_impale_info(void) {
    struct ScriptTwoPointerArgs* args;

    args = (struct ScriptTwoPointerArgs*)current_args;
    set_active_projectile_impale_info(
        args->first,
        args->second);
}

void _set_active_projectile_not_duckable(void) {
    set_active_projectile_not_duckable();
}

void _set_active_projectile_rx_info(void) {
    struct ScriptReactionArgs* args;

    args = (struct ScriptReactionArgs*)current_args;
    set_active_projectile_rx_info(
        args->reaction, args->rate,
        args->strength);
}

void _start_projectile_from_plyr_bone(void) {
    struct ScriptStartProjectileArgs* args;
    struct ScriptMkObjResult* result;
    MkObj* projectile;

    args = (struct ScriptStartProjectileArgs*)current_args;
    projectile = start_projectile_from_plyr_bone(
        args->bone_id,
        args->existing_object,
        get_script_string_arg(3), args->speed,
        args->tolerance,
        args->bone_offset);
    result = (struct ScriptMkObjResult*)active_cmdscript;
    result->value = projectile;
}

void _run_reaction_cleanup_function(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    run_reaction_cleanup_function(args->player);
}

void _reaction_xfer_him(void) {
    struct ScriptReactionArgs* args;
    struct ScriptIntResult* result;
    int transferred;

    args = (struct ScriptReactionArgs*)current_args;
    transferred = reaction_xfer_him(
        args->reaction, args->rate,
        args->strength);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = transferred;
}

void _mks_set_cb1_wind_normal(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    mks_set_cb1_wind_normal(
        args->x, args->y, args->z);
}

void _mks_bgnd_start_wind(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    mks_bgnd_start_wind(args->x, args->y,
                        args->z);
}

void _mks_npc_build_bones_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_build_bones_tbl(
        args->slots[0].i, args->slots[1].pointer);
}

void _mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
        args->player);
}

void _mks_xfer_collision_info_plyr_to_bgnd_script(void) {
    struct ScriptPlyrEntryArgs* args;

    args = (struct ScriptPlyrEntryArgs*)current_args;
    mks_xfer_collision_info_plyr_to_bgnd_script(
        args->player, args->entry);
}

void _mks_xfer_collision_info_plyr_to_script(void) {
    struct ScriptEntryPlayerArgs* args;

    args = (struct ScriptEntryPlayerArgs*)current_args;
    mks_xfer_collision_info_plyr_to_script(
        args->entry, args->player);
}

void _resume_effect_at_plyr_num_bid(void) {
    struct ScriptResumeEffectArgs* args;

    args = (struct ScriptResumeEffectArgs*)current_args;
    resume_effect_at_plyr_num_bid(
        args->player, args->bone_id,
        args->effect_handle, args->bind_mode,
        args->blood_required);
}

void _resume_effect_at_obj_bid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    resume_effect_at_obj_bid(
        args->slots[0].pointer, args->slots[1].i,
        args->slots[2].u, args->slots[3].i,
        args->slots[4].i);
}

void _mks_start_gusher(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        mks_start_gusher(
            args->slots[0].i, args->slots[1].i, current_args,
            args->slots[2].f, args->slots[3].f,
            args->slots[4].f, args->slots[5].f,
            args->slots[6].f, args->slots[7].f);
}

void _mks_victim_bleed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_victim_bleed(args->slots[0].i, args->slots[1].i);
}

void _mks_plyr_stop(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_plyr_stop(args->slots[0].i);
}

void _mks_set_plyr_to_center_ang_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_plyr_to_center_ang_offset(args->slots[0].i, args->slots[1].f);
}

void _mks_bgnd_cam_offset_away(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_bgnd_cam_offset_away(args->slots[0].f, args->slots[1].f);
}

void _mks_bgnd_pfx_bind_to_sobj(void) {
    mks_bgnd_pfx_bind_to_sobj(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].u);
}

void _mks_set_update_delay(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_update_delay(args->slots[0].i, args->slots[1].i);
}

void _mks_removehide_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_removehide_by_group(args->slots[0].i, args->slots[1].i);
}

void _mks_blend_start_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_blend_start_update_by_group(args->slots[0].i, args->slots[1].i);
}

void _mks_shadow_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_shadow_scale(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f);
}

void _mks_gravity_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_gravity_update_by_group(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f);
}

void _mks_away_vel_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_away_vel_update_by_group(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

void _mks_set_rotate_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_rotate_update_by_group(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].f, args->slots[5].i);
}

void _mks_set_sin_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_sin_update_by_group(
        args->slots[0].i, args->slots[1].i, args->slots[2].i,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].f, args->slots[7].f, args->slots[8].f,
        args->slots[9].i);
}

void _bgnd_init_timers(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_init_timers(args->slots[0].i);
}

void _bgnd_obj_insert_obj_ctrl_section(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_obj_insert_obj_ctrl_section(args->slots[0].i, args->slots[1].i);
}

void _bgnd_insert_obj_ctrl_section(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_insert_obj_ctrl_section(args->slots[0].i, args->slots[1].i);
}

void _destroy_sobj_ctrl_proc(void) {
    destroy_sobj_ctrl_proc();
}

void _start_sobj_ctrl_proc(void) {
    start_sobj_ctrl_proc();
}

void _set_active_projectile_collision_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_collision_info(args->slots[0].f, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

void _set_active_projectile_tracking_light(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = set_active_projectile_tracking_light(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_velocity_to_hit_gnd(current_args, args->slots[0].f);
}

void _get_projectile_script_velocity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_projectile_script_velocity(args->slots[0].pointer);
}

void _get_projectile_script_last_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_projectile_script_last_pos(args->slots[0].pointer);
}

void _get_projectile_script_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_projectile_script_plyr_pdata();
}

void _get_projectile_his_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_projectile_his_plyr_num();
}

void _get_projectile_script_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_projectile_script_plyr_num();
}

void _mks_get_victim_to_tr_dot(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mks_get_victim_to_tr_dot(args->slots[0].i);
}

void _ani_x_more_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_x_more_frames(current_args, args->slots[0].f);
}

void _bgnd_npc_set_ani_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_ani_speed(args->slots[0].i, args->slots[1].f);
}

void _mks_npc_disable_ground_y_all_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_disable_ground_y_all_cloth_bones(args->slots[0].i);
}

void _mks_npc_set_ground_y_all_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_set_ground_y_all_cloth_bones(args->slots[0].i, current_args, args->slots[1].f);
}

void _mks_npc_cb1_eq_cloth_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_cb1_eq_cloth_bone(args->slots[0].i, args->slots[1].i);
}

void _mks_npc_cc1_eq_insert_cloth_coll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_cc1_eq_insert_cloth_coll(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _mks_npc_set_target(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_set_target(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _mks_npc_cloth_bones_init_by_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_cloth_bones_init_by_tbl(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _mks_npc_start_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_start_cloth_bones(args->slots[0].i);
}

void _bgnd_attach_rope_to_bgnd_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_attach_rope_to_bgnd_obj(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_detach_rope(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_detach_rope(args->slots[0].i);
}

void _start_rope_proc(void) {
    start_rope_proc();
}

void _bgnd_preload_obj_attach_rope(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_preload_obj_attach_rope(args->slots[0].i);
}

void _bgnd_hide_preload_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_preload_obj(args->slots[0].i);
}

void _bgnd_unhide_preload_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_preload_obj(args->slots[0].i);
}

void _load_bgnd_style(void) {
    const char* name = get_script_string_arg(2);

    load_bgnd_style(((struct ScriptRawArgs*)current_args)->slots[0].i,
                    name, current_args);
}

void _mks_set_cb1_target_bone_cb2(void) {
    mks_set_cb1_target_bone_cb2();
}

void _mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(args->slots[0].i, args->slots[1].f, args->slots[2].i, args->slots[3].f, args->slots[4].i, args->slots[5].f, args->slots[6].i, args->slots[7].f);
}

void _mks_debug_display_cloth_coll_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_debug_display_cloth_coll_plane(current_args, args->slots[0].f);
}

void _mks_debug_display_cloth_coll_cyl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_debug_display_cloth_coll_cyl(args->slots[0].i, args->slots[1].i, args->slots[2].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_set_gravity(current_args, args->slots[0].f);
}

void _plyr_scale_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_scale_pos_vel(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _animpdata_get_anim_hiframe(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        animpdata_get_anim_hiframe(args->slots[0].pointer);
}

void _plyr_get_anim_hiframe(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_get_anim_hiframe();
}

void _plyr_get_anim_frame(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_get_anim_frame();
}

void _plyr_set_vel_xz_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_set_vel_xz_y(current_args, args->slots[0].f, args->slots[1].f);
}

void _player_area_collision_check(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = player_area_collision_check(
        args->slots[0].f, args->slots[1].f, args->slots[2].i,
        args->slots[3].f, args->slots[4].i);
}

void _single_frame_collision_check(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = single_frame_collision_check(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].i, args->slots[4].i, args->slots[5].f);
}

void _is_he_airborn(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_he_airborn();
}

void _auto_ani_on(void) {
    auto_ani_on();
}

void _mks_obj_enable_update_cloth(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_obj_enable_update_cloth(args->slots[0].i, args->slots[1].i);
}

void _mks_bgnd_obj_enable_cloth_update(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_bgnd_obj_enable_cloth_update(args->slots[0].i, args->slots[1].i);
}

void _start_special_weapon_monitor(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_special_weapon_monitor(args->slots[0].i);
}

void _xz_distance_between_players(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = xz_distance_between_players();
}

void _start_scorpion_teleport_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_scorpion_teleport_scale(current_args, args->slots[0].f, args->slots[1].f);
}

void _blast_effect_at_plyr(void) {
    blast_effect_at_plyr();
}

void _configure_iceball(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    configure_iceball(args->slots[0].i);
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
    ((struct ScriptRawResult*)active_cmdscript)->value.i = local_collision_allowed_plyr_pdata();
}

void _kill_plyr_life(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kill_plyr_life(args->slots[0].i);
}

void _is_local_plyr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_local_plyr();
}

void _is_reaction_xfer_him_allowed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_reaction_xfer_him_allowed();
}

void _throw_spear(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = throw_spear();
}

void _pan_vol_pitch_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pan_vol_pitch_snd_req(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _force_ai_style(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    force_ai_style(args->slots[0].i);
}

void _xfer_player_proc_to_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    xfer_player_proc_to_script(
        (MkObj*)args->slots[0].i, args->slots[1].i);
}

void _start_subzero_decoy(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_subzero_decoy(current_args, args->slots[0].f);
}

void _jmt_debug_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jmt_debug_script(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _can_fallingcliff_fall(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = can_fallingcliff_fall();
}

void _get_online_evil_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = 0;
}

void _send_yinyang_evil_msg(void) {
}

void _set_remote_plyr_online_pos(void) {
}

void _enable_no_sync_anim_f(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_no_sync_anim_f(args->slots[0].i);
}

void _enable_no_adjustment_f(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_no_adjustment_f(args->slots[0].i);
}

void _get_exec_tick_ctr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_exec_tick_ctr();
}

void _set_cliff_watcher_round(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_cliff_watcher_round(args->slots[0].i);
}

void _get_cliff_watcher_round(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_cliff_watcher_round();
}

void _clear_cliff_data(void) {
    clear_cliff_data();
}

void _start_cliff_watcher(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_cliff_watcher(current_args, args->slots[0].f);
}

void _get_cliff_data(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_cliff_data();
}

void _destroy_kabal_smoke(void) {
    destroy_kabal_smoke();
}

void _start_kabal_smoke(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_kabal_smoke(current_args, args->slots[0].f);
}

void _kobra_teleport_position(void) {
    kobra_teleport_position();
}

void _mileena_sky_set_position(void) {
    mileena_sky_set_position();
}

void _disable_mileena_collisions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_mileena_collisions(args->slots[0].i);
}

void _switch_plyr_positions(void) {
    switch_plyr_positions();
}

void _kabal_collision_control_victim(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kabal_collision_control_victim(args->slots[0].i);
}

void _gusher_destroy_list(void) {
    gusher_destroy_list();
}

void _cloth_change_ground_plane_for(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    cloth_change_ground_plane_for(current_args, args->slots[0].f);
}

void _set_constrain_last_pos_pdata(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_constrain_last_pos_pdata(args->slots[0].pointer);
}

void _get_current_bgnd(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_current_bgnd();
}

void _damage_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    damage_player(args->slots[0].i, current_args, args->slots[1].f);
}

void _flying_collision(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    flying_collision(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].i, args->slots[4].i, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f, args->slots[9].f);
}

void _player_area_collision_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_area_collision_ticks(args->slots[0].f, args->slots[1].f, args->slots[2].i, args->slots[3].f, args->slots[4].i, args->slots[5].f);
}

void _get_adjusted_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        get_adjusted_speed(args->slots[0].f, args->slots[1].f);
}

void _adjust_kabal_position(void) {
    adjust_kabal_position();
}

void _is_drone(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_drone();
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
    ((struct ScriptRawResult*)active_cmdscript)->value.i = advance_my_sidekick_from_behind_with_moveset();
}

void _kill_ermac_eyes(void) {
    kill_ermac_eyes();
}

void _check_for_online_condition(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        check_for_online_condition(args->slots[0].pointer);
}

void _increment_taunts_performed(void) {
    increment_taunts_performed();
}

void _get_taunts_performed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_taunts_performed();
}

void _mk_chess_fetch_bp_num_based_on_pchr_num(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_bp_num_based_on_pchr_num(args->slots[0].i);
}

void _bgnd_fx_get_binded_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        bgnd_fx_get_binded_obj(args->slots[0].i);
}

void _bgnd_enable_obj_pos_and_ang_setting(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_enable_obj_pos_and_ang_setting(
        args->slots[0].pointer, args->slots[1].pointer,
        args->slots[2].pointer);
}

void _mk_chess_fetch_active_defined_team(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_active_defined_team();
}

void _mk_chess_fetch_active_defined_teams_class(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_active_defined_teams_class(args->slots[0].i);
}

void _mk_chess_air_move(void) {
    mk_chess_air_move();
}

void _mk_chess_active_piece_near_edge(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_active_piece_near_edge();
}

void _courtyard_start_lensflare(void) {
    courtyard_start_lensflare();
}

void _mk_chess_queue_up_piece_event(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_queue_up_piece_event(args->slots[0].i, args->slots[1].i);
}

void _mk_chess_piece_set_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_piece_set_state(args->slots[0].i);
}

void _mk_chess_shifter_switch(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_shifter_switch(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

void _mk_chess_blend_to_my_cell_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_my_cell_pos(args->slots[0].f);
}

void _mk_chess_snap_to_my_cell_now(void) {
    mk_chess_snap_to_my_cell_now();
}

void _mk_chess_xfer_piece_from_scripts(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_xfer_piece_from_scripts(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _mk_chess_piece_die(void) {
    mk_chess_piece_die();
}

void _mk_chess_launch_special_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_launch_special_fx(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _mk_chess_set_piece_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_info(args->slots[0].i, current_args, args->slots[1].f);
}

void _mk_chess_launch_fx_at_active_piece_with_offset(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    mk_chess_launch_fx_at_active_piece_with_offset(
        effect_name, args->slots[1].f, args->slots[2].f,
        args->slots[3].f);
}

void _mk_chess_spell_rescue_current_target(void) {
    mk_chess_spell_rescue_current_target();
}

void _mk_chess_get_piece_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_get_piece_info(args->slots[0].i);
}

void _mk_chess_piece_temporarily_gone(void) {
    mk_chess_piece_temporarily_gone();
}

void _mk_chess_launch_up(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_launch_up(current_args, args->slots[0].f, args->slots[1].f);
}

void _mk_chess_spell_get_target_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_spell_get_target_health(args->slots[0].i);
}

void _mk_chess_spell_move_target_from_temp_area_to(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_move_target_from_temp_area_to(args->slots[0].i);
}

void _mk_chess_spell_move_target_to_temp_area(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_move_target_to_temp_area(args->slots[0].i);
}

void _mk_chess_spell_target_add_access_restrictions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_target_add_access_restrictions(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _mk_chess_spell_has_completed_but_wait_for_fight(void) {
    mk_chess_spell_has_completed_but_wait_for_fight();
}

void _mk_chess_spell_force_fight(void) {
    mk_chess_spell_force_fight();
}

void _mk_chess_spell_is_this_a_forced_fight(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_spell_is_this_a_forced_fight();
}

void _mk_chess_spell_kill_target(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_kill_target(args->slots[0].i);
}

void _mk_chess_spell_move_target_to_target(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_move_target_to_target(args->slots[0].i, args->slots[1].i);
}

void _mk_chess_spell_set_target_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_set_target_health(args->slots[0].i, args->slots[1].f);
}

void _mk_chess_spell_get_target_max_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_spell_get_target_max_health(args->slots[0].i);
}

void _mk_chess_spell_show_target_portrait(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_show_target_portrait(args->slots[0].i);
}

void _mk_chess_spell_has_completed(void) {
    mk_chess_spell_has_completed();
}

void _mk_chess_make_spellcaster(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_make_spellcaster(args->slots[0].pointer);
}

void _mk_chess_set_piece_event_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_event_script(args->slots[0].i, args->slots[1].i);
}

void _mk_chess_piece_test_and_set_timer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_piece_test_and_set_timer(args->slots[0].i, args->slots[1].i);
}

void _mk_chess_deactivate_my_properties(void) {
    mk_chess_deactivate_my_properties();
}

void _mk_chess_ani_until_reached_destination(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_until_reached_destination(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i);
}

void _mk_chess_rotate_towards_cell(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_rotate_towards_cell(args->slots[0].f, args->slots[1].f, args->slots[2].f, args->slots[3].i, args->slots[4].f);
}

void _mk_chess_request_piece_script_for_action(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_request_piece_script_for_action(args->slots[0].i);
}

void _mk_chess_piece_is_idle(void) {
    mk_chess_piece_is_idle();
}

void _mk_chess_put_active_piece_at_cell(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_put_active_piece_at_cell(args->slots[0].f, args->slots[1].f, args->slots[2].i);
}

void _mk_chess_launch_n_land_ani_with_xz(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_launch_n_land_ani_with_xz(
        args->slots[0].i, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].f, args->slots[7].f, args->slots[8].f,
        args->slots[9].f, args->slots[10].f, args->slots[11].i,
        args->slots[12].u);
}

void _mk_chess_stop_me(void) {
    mk_chess_stop_me();
}

void _mk_chess_get_piece_event_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_get_piece_event_data(args->slots[0].i);
}

void _bgnd_active_sobj_no_ztest(void) {
    bgnd_active_sobj_no_ztest();
}

void _mk_chess_set_cell_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_cell_offset(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _mk_chess_force_away(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_force_away(args->slots[0].f, args->slots[1].i, args->slots[2].f, args->slots[3].i);
}

void _mk_chess_add_movement_skill(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_add_movement_skill(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _mk_chess_define_class_initial_power(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_define_class_initial_power(current_args, args->slots[0].f);
}

void _mk_chess_activate_my_properties(void) {
    mk_chess_activate_my_properties();
}

void _mk_chess_wait_until_attack_cam_closes_in(void) {
    mk_chess_wait_until_attack_cam_closes_in();
}

void _mk_chess_snd_request(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_snd_request(args->slots[0].i);
}

void _mk_chess_dont_constrain_piece(void) {
    mk_chess_dont_constrain_piece();
}

void _mk_chess_piece_match_y_ang_to_anim(void) {
    mk_chess_piece_match_y_ang_to_anim();
}

void _mk_chess_check_glitch_into_stance(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_check_glitch_into_stance();
}

void _mk_chess_set_glitch_stance_flag(void) {
    mk_chess_set_glitch_stance_flag();
}

void _mk_chess_glitch_to_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_glitch_to_ani_frame(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f);
}

void _mk_chess_set_ani_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_ani_speed(current_args, args->slots[0].f);
}

void _bgnd_add_fx_to_hide(void) {
    bgnd_add_fx_to_hide(get_script_string_arg(1));
}

void _mk_chess_place_special_cell_at(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_place_special_cell_at(
        args->slots[0].i, args->slots[1].i, args->slots[2].i,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].f, args->slots[7].i);
}

void _mk_chess_piece_event_from_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_piece_event_from_script(args->slots[0].i);
}

void _mk_chess_blend_to_desired_cell_position_setting(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_desired_cell_position_setting(current_args, args->slots[0].f);
}

void _mk_chess_blend_into_cell_orgin_in_x_frames_by_caller(void) {
    mk_chess_blend_into_cell_orgin_in_x_frames_by_caller();
}

void _mk_chess_set_obj_move_weight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_obj_move_weight(current_args, args->slots[0].f);
}

void _mk_chess_snap_into_cell_orgin_over_x_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_snap_into_cell_orgin_over_x_frames(args->slots[0].f);
}

void _mk_chess_check_snap_into_stance(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_check_snap_into_stance();
}

void _mk_chess_snap_to_stance(void) {
    mk_chess_snap_to_stance();
}

void _mk_chess_blend_to_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_ani_frame(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

void _mk_chess_blend_to_normal_stance(void) {
    mk_chess_blend_to_normal_stance();
}

void _mk_chess_set_normal_stance_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_normal_stance_script(args->slots[0].i);
}

void _bgnd_kill_fx(void) {
    bgnd_kill_fx(get_script_string_arg(1));
}

void _bgnd_no_z_test(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_no_z_test(args->slots[0].i);
}

void _bgnd_get_active_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_get_active_sobj_pos((Vec*)args->slots[0].i);
}

void _mk_chess_set_piece_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_state(args->slots[0].i);
}

void _mk_chess_set_piece_type_as(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_type_as(args->slots[0].i, args->slots[1].i);
}

void _mk_chess_blend_into_cell_orgin_in_x_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_into_cell_orgin_in_x_frames(args->slots[0].f);
}

void _mk_chess_ani_loop_more_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_loop_more_frames(current_args, args->slots[0].f);
}

void _mk_chess_ani_idle(void) {
    mk_chess_ani_idle();
}

void _mk_chess_ani_to_blend_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_to_blend_frame(current_args, args->slots[0].f);
}

void _mk_chess_ani_to_end(void) {
    mk_chess_ani_to_end();
}

void _mk_chess_ani_1_frame(void) {
    mk_chess_ani_1_frame();
}

void _mk_chess_ani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_to_frame_x(current_args, args->slots[0].f);
}

void _mk_chess_blend_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_ani(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f);
}

void _mk_chess_init_piece(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_init_piece(args->slots[0].i, args->slots[1].i);
}

void _mk_chess_load_chess_table(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_load_chess_table(args->slots[0].pointer);
}

void _bgnd_enable_wall_hider(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_enable_wall_hider(args->slots[0].i);
}

void _bgnd_npc_set_pos_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos_y(args->slots[0].i, args->slots[1].f);
}

void _bgnd_npc_set_pos_vel_heading(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos_vel_heading(args->slots[0].i, args->slots[1].f);
}

void _bgnd_npc_set_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos_vel(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_npc_get_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_npc_get_ang_y(args->slots[0].i);
}

void _bgnd_npc_adjust_y_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_adjust_y_ang(args->slots[0].i, args->slots[1].f);
}

void _bgnd_npc_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_get_pos(args->slots[0].i, args->slots[1].pointer);
}

void _bgnd_npc_get_aux_int_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_npc_get_aux_int_data(args->slots[0].i, args->slots[1].i);
}

void _bgnd_npc_start_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_start_ani(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].f, args->slots[4].f);
}

void _bgnd_npc_set_aux_int_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_aux_int_data(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_add_scripted_brains_to_npc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_scripted_brains_to_npc(args->slots[0].i, args->slots[1].i);
}

void _bgnd_npc_set_y_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_y_ang(args->slots[0].i, args->slots[1].f);
}

void _bgnd_npc_like_plyr(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_like_plyr(args->slots[0].i);
}

void _bgnd_npc_set_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_scale(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_create_named_npc_in_slot(void) {
    struct ScriptRawArgs* current;
    struct ScriptRawArgs* saved;
    const char* name;

    saved = (struct ScriptRawArgs*)current_args;
    name = get_script_string_arg(2);
    current = (struct ScriptRawArgs*)current_args;
    bgnd_create_named_npc_in_slot(
        current->slots[0].i, name, saved->slots[2].i,
        saved->slots[3].i);
}

void _bgnd_npc_set_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _danger_zone_eligible_on(void) {
    danger_zone_eligible_on();
}

void _bgnd_get_exec_tick_ctr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_get_exec_tick_ctr();
}

void _start_sobj_launch_monitor(void) {
    start_sobj_launch_monitor();
}

void _start_chunk_launch_monitor(void) {
    start_chunk_launch_monitor();
}

void _snd_req_delay(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    snd_req_delay(args->slots[0].i, args->slots[1].i);
}

void _bgnd_set_material_color(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_material_color(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i);
}

void _bgnd_set_fx_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_fx_ang_y(current_args, args->slots[0].f);
}

void _bgnd_launch_plyr_blood_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_launch_plyr_blood_fx(args->slots[0].i, args->slots[1].i);
}

void _bgnd_sobj_set_priority(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_priority(args->slots[0].i, args->slots[1].i);
}

void _bgnd_pfxhandle_spawn_at_bid(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        bgnd_pfxhandle_spawn_at_bid(
            effect_name, args->slots[1].pointer,
            args->slots[2].i);
}

void _bgnd_launch_fx_at_bid_of_mkobj(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_bid_of_mkobj(
        effect_name, args->slots[1].pointer,
        args->slots[2].i);
}

void _bgnd_launch_fx_at_plyr_bid(void) {
    bgnd_launch_fx_at_plyr_bid(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _cam_set_intro_cam_pause_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    cam_set_intro_cam_pause_ticks(args->slots[0].f);
}

void _bgnd_collision_if_monitor_col_as(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_monitor_col_as(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _bgnd_pebble_burst_set_end_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_set_end_state(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _bgnd_pebble_change_current_end_behavior(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_change_current_end_behavior(args->slots[0].i);
}

void _bgnd_pebble_burst_set_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_set_value(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f, args->slots[9].f);
}

void _bgnd_pebble_burst_set_value_min_max(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_set_value_min_max(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f, args->slots[5].f);
}

void _bgnd_pebble_change_current_behavior_to_bounce(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_change_current_behavior_to_bounce(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i, args->slots[7].i);
}

void _bgnd_start_preload_sobj_morph(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_preload_sobj_morph(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _bgnd_start_preload_sobj_uv_scroll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_preload_sobj_uv_scroll(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f);
}

void _bgnd_launch_fx_at_active_sobj_pos_with_offset(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_active_sobj_pos_with_offset(
        effect_name, args->slots[1].f, args->slots[2].f,
        args->slots[3].f);
}

void _bgnd_active_sobj_no_zwrite(void) {
    bgnd_active_sobj_no_zwrite();
}

void _bgnd_set_active_sobj_zoffset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_zoffset(current_args, args->slots[0].f);
}

void _face_point(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    face_point(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _share_my_attack_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    share_my_attack_info(current_args, args->slots[0].f, args->slots[1].f);
}

void _ani_with_pos(void) {
    ani_with_pos();
}

void _ani_no_pos(void) {
    ani_no_pos();
}

void _bgnd_pebble_burst_at_pebble_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_at_pebble_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_pebble_burst_at_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_at_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i, current_args, args->slots[3].f, args->slots[4].f, args->slots[5].f);
}

void _nb_place_slave_in_bgnd(void) {
    struct ScriptPlaceSlaveArgs* args;
    const char* name;

    args = (struct ScriptPlaceSlaveArgs*)current_args;
    name = get_script_string_arg(3);
    nb_place_slave_in_bgnd(
        args->npc_id, args->rope_model_index, name, args->model_id,
        args->anchor.x, args->anchor.y, args->anchor.z, args->rope_length,
        args->local_angles.x, args->local_angles.y, args->local_angles.z,
        args->object_angles.x, args->object_angles.y, args->object_angles.z,
        args->acceleration_divisor, args->acceleration_scale, args->field_8C);
}

void _nb_npc_slave_plyr_process_collision(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    nb_npc_slave_plyr_process_collision(args->slots[0].u);
}

void _bgnd_collision_if_enable_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_enable_col(args->slots[0].i, args->slots[1].i);
}

void _bgnd_collision_if_disable_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_disable_col(args->slots[0].i, args->slots[1].i);
}

void _bgnd_pebble_simple_launch_at_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_simple_launch_at_time(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _bgnd_pebble_set_current_pebble(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_set_current_pebble(args->slots[0].i, args->slots[1].i);
}

void _bgnd_remove_cracks(void) {
    bgnd_remove_cracks();
}

void _bgnd_pebble_rand_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_rand_scale(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

void _bgnd_pebble_burst_at_chunk_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_at_chunk_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_unhide_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_pebbles(args->slots[0].i);
}

void _bgnd_hide_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_pebbles(args->slots[0].i);
}

void _spad_set_heading_vector_to(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_heading_vector_to(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

void _degrees_to_rad(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = degrees_to_rad(current_args, args->slots[0].f);
}

void _rad_to_degrees(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = rad_to_degrees(current_args, args->slots[0].f);
}

void _bgnd_pebble_set_current_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_set_current_info(args->slots[0].i, current_args, args->slots[1].f);
}

void _bgnd_pebble_fetch_current_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_pebble_fetch_current_info(args->slots[0].i);
}

void _bgnd_pebble_change_current_behavior(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_change_current_behavior(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i, args->slots[7].i);
}

void _bgnd_init_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_init_pebbles(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_pebble_gravity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_gravity(args->slots[0].i, current_args, args->slots[1].f);
}

void _get_sobj_pebble_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_sobj_pebble_obj((MkSobj*)args->slots[0].i);
}

void _get_general_pebble_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_general_pebble_data(args->slots[0].pointer);
}

void _ncs_create_pebble_monitor_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = ncs_create_pebble_monitor_proc(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _ncs_set_pebble_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ncs_set_pebble_pos(
        (struct PebbleData*)args->slots[0].i, args->slots[1].i,
        (Vec*)args->slots[2].i);
}

void _ncs_create_pebbles_with_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = ncs_create_pebbles_with_sobj(args->slots[0].i, args->slots[1].i);
}

void _bgnd_create_pebbles_with_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_create_pebbles_with_sobj(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _bgnd_create_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_create_pebbles(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i);
}

void _bgnd_pebble_launch_at_time(void) {
    struct ScriptRawArgs* args;
    float sp8;

    args = (struct ScriptRawArgs*)current_args;
    sp8 = args->slots[10].f;
    bgnd_pebble_launch_at_time(
        args->slots[0].i, args->slots[1].i, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].f, args->slots[7].f, args->slots[8].f,
        args->slots[9].f, args->slots[10].f, args->slots[11].i,
        args->slots[12].i);
}

void _bgnd_jtb_debug_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_jtb_debug_info(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _get_game_speed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_game_speed();
}

void _get_soul_sine(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_soul_sine(args->slots[0].i);
}

void _bgnd_set_active_sobj_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_ang(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _build_sine_table_for_scripts(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = build_sine_table_for_scripts();
}

void _bgnd_is_active_sobj_hidden(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_is_active_sobj_hidden();
}

void _bgnd_set_fx_ang_dir_to_i_vector(void) {
    bgnd_set_fx_ang_dir_to_i_vector();
}

void _bgnd_set_active_sobj_in_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_in_obj(args->slots[0].i, args->slots[1].i);
}

void _pfxhandle_bgnd_spawn_at_position(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_bgnd_spawn_at_position(
            effect_name, args->slots[1].f, args->slots[2].f,
            args->slots[3].f);
}

void _bgnd_launch_fx_to_sobj(void) {
    bgnd_launch_fx_to_sobj(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _bgnd_launch_fx_at_position(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_position(
        effect_name, args->slots[1].f, args->slots[2].f,
        args->slots[3].f);
}

void _bgnd_set_sobj_launch_params_exact(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_launch_params_exact(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f);
}

void _load_script_as_reaction(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    load_script_as_reaction(args->slots[0].i, args->slots[1].i);
}

void _bgnd_collision_if_rx_override(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_rx_override(args->slots[0].i);
}

void _set_background_obstacle_disable_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_background_obstacle_disable_flag(args->slots[0].i, args->slots[1].i);
}

void _set_background_obstacle_repel_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_background_obstacle_repel_flag(args->slots[0].i, args->slots[1].i);
}

void _bgnd_collison_if_monitor_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collison_if_monitor_col(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i, args->slots[6].i);
}

void _bgnd_collison_if_set_info(void) {
    bgnd_collison_if_set_info();
}

void _bgnd_collison_if_set_return_result(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collison_if_set_return_result(args->slots[0].i);
}

void _bgnd_collison_if_to_scripts_activate(void) {
    bgnd_collison_if_to_scripts_activate();
}

void _initialize_background_danger_zones(void) {
    initialize_background_danger_zones();
}

void _spad_xz_cos_two_vectors(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_xz_cos_two_vectors(args->slots[0].i, args->slots[1].i);
}

void _bgnd_timer_get_tick_count(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_timer_get_tick_count(args->slots[0].i);
}

void _spad_xz_length_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_xz_length_vector(args->slots[0].i);
}

void _bgnd_current_rx_set_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_current_rx_set_info(args->slots[0].i, current_args, args->slots[1].f);
}

void _bgnd_current_rx_get_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_current_rx_get_info(args->slots[0].i);
}

void _bgnd_setup_rx_handler(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_setup_rx_handler(args->slots[0].i);
}

void _bgnd_start_timer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_timer(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_kill_all_launched_sobjs(void) {
    bgnd_kill_all_launched_sobjs();
}

void _jump_towards_opponent_bgnd_transition(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = jump_towards_opponent_bgnd_transition();
}

void _allow_shard_pfx_now(void) {
    allow_shard_pfx_now();
}

void _kill_shard_pfx_now(void) {
    kill_shard_pfx_now();
}

void _bgnd_set_fx_z_offset(void) {
    const char* name = get_script_string_arg(1);

    bgnd_set_fx_z_offset(name,
        ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _bgnd_force_plyr_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_force_plyr_ground_plane(args->slots[0].i, current_args, args->slots[1].f);
}

void _bgnd_set_launch_velocity_based_on_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_launch_velocity_based_on_sobj_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i, current_args, args->slots[3].f, args->slots[4].f);
}

void _bgnd_update_active_mksobj(void) {
    bgnd_update_active_mksobj();
}

void _bgnd_shadow_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_shadow_control(args->slots[0].i);
}

void _bgnd_set_plyr_gravity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_plyr_gravity(current_args, args->slots[0].f);
}

void _bgnd_move_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_move_player(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _spad_sub_vectors(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_sub_vectors(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_act_at_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_act_at_time(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

void _bgnd_xfer_attacker(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_xfer_attacker(args->slots[0].i);
}

void _spad_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_get_pos(args->slots[0].i, args->slots[1].i);
}

void _sfrand(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = sfrand(current_args, args->slots[0].f);
}

void _spad_add_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_add_vector(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_launch_fx_at_plyr_pos_and_y(void) {
    bgnd_launch_fx_at_plyr_pos_and_y(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

void _bgnd_start_cracks(void) {
    bgnd_start_cracks();
}

void _bgnd_init_cracks(void) {
    bgnd_init_cracks();
}

void _bgnd_place_crack_when_plyr_hits_ground(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_place_crack_when_plyr_hits_ground(args->slots[0].i);
}

void _plyr_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_get_pos(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = camera_get_pos(args->slots[0].i);
}

void _camera_set_movement_offset_explicit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_offset_explicit(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _bgnd_force_ground_to(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_force_ground_to(current_args, args->slots[0].f);
}

void _camera_set_lookat_offset_explicit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_lookat_offset_explicit(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _bgnd_run_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_run_camera_script(args->slots[0].i, args->slots[1].i);
}

void _spad_set_y_angle_plus_offset_from_xz_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_y_angle_plus_offset_from_xz_vector(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _spad_set_vector_setting(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_vector_setting(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_process_active_sobj_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_process_active_sobj_info(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

void _bgnd_blood_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_blood_control(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _delete_obstacle_from_background_by_id(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    delete_obstacle_from_background_by_id(args->slots[0].i);
}

void _bgnd_make_displayed_item_pickupable_at_active_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_make_displayed_item_pickupable_at_active_sobj_pos(args->slots[0].i);
}

void _bgnd_reset_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_reset_sobj(args->slots[0].i);
}

void _restart_effect(void) {
    get_script_string_arg(1);
    restart_effect();
}

void _bgnd_set_active_sobj_rop(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_rop(args->slots[0].i);
}

void _bgnd_set_active_sobj_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_scale(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _bgnd_set_active_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_pos(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _bgnd_set_active_sobj_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_pos_vel(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _bgnd_apply_active_sobj_pos_vel_drag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_apply_active_sobj_pos_vel_drag(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _int_to_float(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = int_to_float(args->slots[0].i);
}

void _float_to_int(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = float_to_int(current_args, args->slots[0].f);
}

void _bgnd_set_sobj_launch_params(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_launch_params(args->slots[0].i, args->slots[1].i, args->slots[2].i, current_args, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f, args->slots[9].f);
}

void _spad_scale_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_scale_vector(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _spad_set_vector_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_vector_y(args->slots[0].i, current_args, args->slots[1].f);
}

void _bgnd_process_collision_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_process_collision_info(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f);
}

void _bgnd_takeover_plyr(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_takeover_plyr(args->slots[0].pointer);
}

void _spad_norm_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_norm_vector(args->slots[0].i);
}

void _spad_rotate_xz_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_rotate_xz_vector(args->slots[0].i, current_args, args->slots[1].f);
}

void _spad_xz_dot_xz(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_xz_dot_xz(args->slots[0].i, args->slots[1].i);
}

void _spad_set_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_vector(args->slots[0].i, args->slots[1].i);
}

void _bgnd_launch_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_launch_sobj(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i, args->slots[6].f, args->slots[7].i);
}

void _bgnd_set_collision_plane_for_launched_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_collision_plane_for_launched_sobj(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_set_kill_plane_for_launched_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_kill_plane_for_launched_sobj(args->slots[0].i);
}

void _bgnd_fade_object(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_fade_object(args->slots[0].i, current_args, args->slots[1].f);
}

void _bgnd_get_sobj_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_sobj_ang_y(args->slots[0].i);
}

void _bgnd_rotate_xz_about_orgin_active_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_rotate_xz_about_orgin_active_sobj(args->slots[0].f);
}

void _bgnd_hide_active_sobj(void) {
    bgnd_hide_active_sobj();
}

void _bgnd_unhide_active_sobj(void) {
    bgnd_unhide_active_sobj();
}

void _bgnd_set_active_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj(args->slots[0].i);
}

void _bgnd_chunk_explosion_match_velocity_with_params(void) {
    struct ScriptRawArgs* args;
    char* shard_name;
    float velocity_scale;
    float vertical_velocity;
    float random_vertical_velocity;

    args = (struct ScriptRawArgs*)current_args;
    shard_name = get_script_string_arg(4);
    velocity_scale = args->slots[0].f;
    vertical_velocity = args->slots[1].f;
    random_vertical_velocity = args->slots[2].f;
    bgnd_chunk_explosion_match_velocity_with_params(
        velocity_scale, vertical_velocity, random_vertical_velocity,
        shard_name, args->slots[4].i, args->slots[5].i,
        args->slots[6].i, args->slots[7].i);
}

void _bgnd_restore_player(void) {
    bgnd_restore_player();
}

void _dont_fence_plyr_in(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    dont_fence_plyr_in(args->slots[0].i);
}

void _bgnd_launch_plyr_up_and_forward_running(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_launch_plyr_up_and_forward_running();
}

void _bgnd_launch_plyr_up_and_forward(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_launch_plyr_up_and_forward(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].f,
        args->slots[6].i);
}

void _bgnd_register_danger_zone_callback(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_register_danger_zone_callback(
        (PlyrPdata*)args->slots[0].i, args->slots[1].i);
}

void _bgnd_swap_level(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_swap_level(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_new_normal_check_for_hider(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_remove_wall_from_hider(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_remove_wall_from_hider(args->slots[0].i);
}

void _bgnd_add_wall_to_hide(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_wall_to_hide(args->slots[0].i);
}

void _bgnd_add_wall_to_unhide(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_wall_to_unhide(args->slots[0].i);
}

void _bgnd_start_wall_hider(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_wall_hider(args->slots[0].i);
}

void _bgnd_turn_off_backface_culling(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_turn_off_backface_culling(args->slots[0].i);
}

void _bgnd_sobj_cam_frustum_test_into_transparent(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_cam_frustum_test_into_transparent(
        args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

void _obj_sobj_cam_frustum_test_into_transparent(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_sobj_cam_frustum_test_into_transparent(
        (MkObj*)args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f);
}

void _bgnd_sobj_cam_volume_test_steer_over(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_cam_volume_test_steer_over(
        args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

void _bgnd_rotate_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_rotate_sobj(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_set_wall_hide_distance(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_wall_hide_distance(current_args, args->slots[0].f);
}

void _bgnd_set_new_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_new_ground_plane(current_args, args->slots[0].f);
}

void _bgnd_place_weapon_at_position(void) {
    struct ScriptPlaceWeaponArgs* args = (struct ScriptPlaceWeaponArgs*)current_args;

    bgnd_place_weapon_at_position(
        args->primary_object_id, args->secondary_object_id,
        args->primary_sobj_id, args->secondary_sobj_id, args->paired,
        args->primary_position.x, args->primary_position.y,
        args->primary_position.z, args->primary_angles.x,
        args->primary_angles.y, args->primary_angles.z,
        args->secondary_position.x, args->secondary_position.y,
        args->secondary_position.z, args->secondary_angles.x,
        args->secondary_angles.y, args->secondary_angles.z,
        args->pickup_sobj_id, args->radius, args->height,
        args->collision_center.x, args->collision_center.y,
        args->collision_center.z, args->permanent);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_reaction_xfer_him(args->slots[0].i);
}

void _pz_fighter_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_function(args->slots[0].i);
}

void _pz_fighter_fetch_distance_to_center_pos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = pz_fighter_fetch_distance_to_center_pos();
}

void _set_anim_hiframe(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_anim_hiframe(args->slots[0].f);
}

void _bgnd_reg_col_cb_for_beetle_lair(void) {
    bgnd_reg_col_cb_for_beetle_lair();
}

void _pz_fighter_wipe_blood_off_hands(void) {
    pz_fighter_wipe_blood_off_hands();
}

void _bgnd_get_preload_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        bgnd_get_preload_obj(args->slots[0].i);
}

void _bgnd_preload_named_model(void) {
    const char* model_name;

    model_name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        bgnd_preload_named_model(
            model_name, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

void _sobj_set_alpha(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    sobj_set_alpha(args->slots[0].i, args->slots[1].i);
}

void _obj_get_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_get_scale(args->slots[0].i, args->slots[1].i);
}

void _obj_set_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_set_scale(args->slots[0].pointer, args->slots[1].pointer);
}

void _bgnd_sobj_set_ani_framerate(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_ani_framerate(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _bgnd_sobj_set_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_ani_frame(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _bgnd_sobj_set_alpha(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_alpha(args->slots[0].i, args->slots[1].i);
}

void _bgnd_sobj_set_rel_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_rel_pos(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_sobj_set_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_ang(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_sobj_get_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_get_ang(args->slots[0].i, (Vec*)args->slots[1].i);
}

void _bgnd_sobj_set_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_pos_vel(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_sobj_set_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_pos(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _bgnd_sobj_get_z_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_z_pos(args->slots[0].i);
}

void _bgnd_sobj_get_y_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_y_pos(args->slots[0].i);
}

void _bgnd_sobj_get_x_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_x_pos(args->slots[0].i);
}

void _bgnd_unhide_sobj_and_children(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_sobj_and_children(args->slots[0].i);
}

void _bgnd_hide_sobj_and_children(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_sobj_and_children(args->slots[0].i);
}

void _bgnd_unhide_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_sobj(args->slots[0].i);
}

void _bgnd_hide_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_sobj(args->slots[0].i);
}

void _head_tracking_on(void) {
    head_tracking_on();
}

void _head_tracking_off(void) {
    head_tracking_off();
}

void _load_aux_weapon(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    load_aux_weapon(args->slots[0].pointer);
}

void _ani_loop_more_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_loop_more_frames(args->slots[0].f);
}

void _ani_1_frame(void) {
    ani_1_frame();
}

void _pz_fighter_force_reaction_in_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_force_reaction_in_ticks(args->slots[0].i, args->slots[1].i);
}

void _bgnd_replace_tex_with_wiff_and_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_replace_tex_with_wiff_and_ani(
        ((struct ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2),
        args->slots[2].f, args->slots[3].i,
        args->slots[4].i);
}

void _bgnd_pulsate_object_with_caps_and_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pulsate_object_with_caps_and_scale(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].i, args->slots[4].f, args->slots[5].i, args->slots[6].i, args->slots[7].f, args->slots[8].f, args->slots[9].f, args->slots[10].f, args->slots[11].f, args->slots[12].f);
}

void _bgnd_pulsate_object_with_caps(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pulsate_object_with_caps(
        args->slots[0].i, args->slots[1].i, args->slots[2].f,
        args->slots[3].i, args->slots[4].f, args->slots[5].i,
        args->slots[6].i);
}

void _bgnd_pulsate_object(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pulsate_object(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].i, args->slots[4].f);
}

void _bgnd_turn_on_backface_culling(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_turn_on_backface_culling(args->slots[0].i);
}

void _bgnd_no_z_write(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_no_z_write(args->slots[0].i);
}

void _frand(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = frand(current_args, args->slots[0].f);
}

void _plyr_start_script_in_plyr_pdata_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_start_script_in_plyr_pdata_proc(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _plyr_start_script_in_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_start_script_in_proc(args->slots[0].i, args->slots[1].i);
}

void _bgnd_always_face_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_always_face_y(args->slots[0].i);
}

void _bgnd_apply_zoffset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_apply_zoffset(args->slots[0].i, current_args, args->slots[1].f);
}

void _bgnd_start_sobj_uv_scroll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_sobj_uv_scroll(
        args->slots[0].i,
        args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f,
        args->slots[5].i);
}

void _bgnd_create_sobjs(void) {
    bgnd_create_sobjs();
}

void _sobj_no_zwrite(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    sobj_no_zwrite(args->slots[0].pointer);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = pz_fighter_inline_force_away_with_ani(
        args->slots[0].f, args->slots[1].u,
        args->slots[2].f, args->slots[3].u);
}

void _suspend_in_midair(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    suspend_in_midair(current_args, args->slots[0].f);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_check_to_toggle_obj_and_ani_flips(args->slots[0].i);
}

void _pz_fighter_move_into_fighting_position(void) {
    pz_fighter_move_into_fighting_position();
}

void _pz_fighter_release_other_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_release_other_player(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_register_move(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i);
}

void _get_my_particle_player_bank_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_my_particle_player_bank_num();
}

void _flash_hit_at_bid_with_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    flash_hit_at_bid_with_y(args->slots[0].i, current_args, args->slots[1].f);
}

void _flash_hit_at_bid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    flash_hit_at_bid(args->slots[0].i);
}

void _fight_fx_im_hit_flash(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fight_fx_im_hit_flash(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f);
}

void _launch_fx_at_pos_with_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = launch_fx_at_pos_with_obj(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
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
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;

    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (int)bgnd_place_point_light_for_ticks(
            args->slots[0].pointer, args->slots[1].i,
            args->slots[2].f, args->slots[3].i);
}

void _bgnd_delete_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_delete_danger_zone(args->slots[0].i);
}

void _bgnd_set_active_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_danger_zone(args->slots[0].i);
}

void _bgnd_set_danger_zone_radius(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_radius(args->slots[0].f);
}

void _bgnd_set_viewing_of_danger_zones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_viewing_of_danger_zones(args->slots[0].i);
}

void _bgnd_set_danger_zone_y_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_y_angle(args->slots[0].f);
}

void _bgnd_set_danger_zone_depth(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_depth(args->slots[0].f);
}

void _bgnd_set_danger_zone_width(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_width(args->slots[0].f);
}

void _bgnd_set_danger_zone_center_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_center_position(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _bgnd_enable_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_enable_danger_zone(args->slots[0].i, args->slots[1].i);
}

void _bgnd_create_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_create_danger_zone(args->slots[0].i, args->slots[1].i,
                            args->slots[2].i, args->slots[3].f,
                            args->slots[4].i);
}

void _bgnd_place_object_at_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_place_object_at_position(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i);
}

void _do_i_have_life_left(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = do_i_have_life_left();
}

void _hide_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hide_player(args->slots[0].i, args->slots[1].i);
}

void _setup_for_flip_ani(void) {
    setup_for_flip_ani();
}

void _get_victory_flip_flags(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_victory_flip_flags();
}

void _obj_set_z_offsets(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_set_z_offsets(args->slots[0].i, current_args, args->slots[1].f);
}

void _get_my_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_my_plyr_num();
}

void _do_victory_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    do_victory_camera(args->slots[0].pointer);
}

void _unfreeze_player(void) {
    unfreeze_player();
}

void _turn_into_energy_player(void) {
    turn_into_energy_player();
}

void _is_blood_disabled(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_blood_disabled();
}

void _plyr_get_f_constrained(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_get_f_constrained(args->slots[0].i);
}

void _move_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    move_player(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _face_ang_from_pos_to_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    face_ang_from_pos_to_him(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _move_player_no_constrain_update(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    move_player_no_constrain_update(args->slots[0].pointer,
                                   args->slots[1].pointer,
                                   args->slots[2].pointer);
}

void _show_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_player(args->slots[0].i);
}

void _plyr_invulnerable_to_projectiles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_invulnerable_to_projectiles(args->slots[0].i, args->slots[1].i);
}

void _active_sidekick_swap_from_behind(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    active_sidekick_swap_from_behind(args->player);
}

void _active_sidekick_swap_from_sky(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    active_sidekick_swap_from_sky(args->player);
}

void _active_sidekick_swap_change_style(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    active_sidekick_swap_change_style(args->player);
}

void _is_sidekick_active(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        is_sidekick_active(args->slots[0].pointer);
}

void _taunt_increase_life(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        taunt_increase_life(
            args->slots[0].f, args->slots[1].f);
}

void _get_collision_result(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_collision_result();
}

void _general_flash_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    general_flash_fx(
        args->slots[0].i, args->slots[1].pointer,
        get_script_string_arg(3), args->slots[3].i,
        args->slots[4].i, args->slots[5].f);
}

void _get_kombat_difficulty(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_kombat_difficulty();
}

void _uv_my_angle_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    uv_my_angle_y(
        args->slots[0].pointer, args->slots[1].f);
}

void _obj_set_flipped_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_set_flipped_bones(
        args->slots[0].pointer, args->slots[1].pointer);
}

void _run_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    run_camera_script(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _am_i_airborn_check_in_reaction(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_airborn_check_in_reaction();
}

void _obj_enable_grounding(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_enable_grounding(args->slots[0].i);
}

void _camera_idle(void) {
    camera_idle();
}

void _drone_ai_increase_big_boss_stage(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    drone_ai_increase_big_boss_stage(args->player);
}

void _pz_finish_him_request(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pz_finish_him_request();
}

void _pz_fighter_create_space_between_fighters_for_special_moves(void) {
    pz_fighter_create_space_between_fighters_for_special_moves();
}

void _freeze_player(void) {
    freeze_player();
}

void _get_his_previous_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_his_previous_state();
}

void _get_his_secondary_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_his_secondary_state();
}

void _pebble_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pebble_get_pos(args->slots[0].i, args->slots[1].i,
                   (Vec*)args->slots[2].i);
}

void _is_big_boss(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_big_boss(args->slots[0].i);
}

void _is_load_meter_active(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_load_meter_active();
}

void _plyr_snd_req_no_plyr_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_snd_req_no_plyr_proc(args->slots[0].i, args->slots[1].i);
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
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    MkObj* projectile = start_projectile_from_sidekick_bone(
        args->slots[0].i, args->slots[1].pointer, get_script_string_arg(3),
        args->slots[3].f, args->slots[4].f, args->slots[5].pointer);

    ((struct ScriptMkObjResult*)active_cmdscript)->value = projectile;
}

void _noobsmoke_fire_projectile_request(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = noobsmoke_fire_projectile_request();
}

void _smoke_victory_entrance(void) {
    smoke_victory_entrance();
}

void _noob_victory_entrance(void) {
    noob_victory_entrance();
}

void _sidekick_switch_style_swap(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    sidekick_switch_style_swap(args->slots[0].f, args->slots[1].u);
}

void _dk_voice_call(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    dk_voice_call(args->slots[0].i, args->slots[1].i);
}

void _dk_taunt_at_screen(void) {
    dk_taunt_at_screen();
}

void _special_move_cam_setup2(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    special_move_cam_setup2(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i,
        args->slots[6].i, args->slots[7].i, args->slots[8].pointer,
        args->slots[9].pointer);
}

void _rd_set_impact_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    rd_set_impact_vector(current_args, args->slots[0].f);
}

void _get_plyr_pdata_plyr_num(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_plyr_pdata_plyr_num(args->slots[0].i);
}

void _is_special_move_available(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        is_special_move_available(
            args->slots[0].pointer,
            args->slots[1].i);
}

void _retract_spear_from_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    retract_spear_from_camera(args->slots[0].pointer);
}

void _fire_spear_at_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = fire_spear_at_camera(args->slots[0].i, args->slots[1].i);
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
    ((struct ScriptRawResult*)active_cmdscript)->value.i = drone_ai_should_ermac_fly_kick();
}

void _drone_ai_should_ermac_ground_slam(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = drone_ai_should_ermac_ground_slam();
}

void _clear_his_f_constrained(void) {
    clear_his_f_constrained();
}

void _idle_his_anim_proc(void) {
    idle_his_anim_proc();
}

void _set_active_projectile_block_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_block_script(args->slots[0].i);
}

void _bgnd_get_obj_pointer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_get_obj_pointer(args->slots[0].i);
}

void _bgnd_get_anim_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_anim_info(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _is_character_unlocked_in_profile(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_character_unlocked_in_profile(args->slots[0].i, args->slots[1].i);
}

void _mini_mission_inactive(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mini_mission_inactive(args->slots[0].i);
}

void _mini_mission_completed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mini_mission_completed(args->slots[0].i);
}

void _start_mini_mission(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_mini_mission(args->slots[0].i);
}

void _is_mini_mission_started(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_started(args->slots[0].i);
}

void _is_mini_mission_active(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_active(args->slots[0].i);
}

void _is_mini_mission_completed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_completed(args->slots[0].i);
}

void _player_add_item_to_inventory(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_add_item_to_inventory(args->slots[0].i);
}

void _player_remove_item_from_inventory(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_remove_item_from_inventory(args->slots[0].i);
}

void _player_has_item(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = player_has_item(args->slots[0].i);
}

void _add_to_konq_profile_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_to_konq_profile_value(args->slots[0].i, args->slots[1].i);
}

void _get_last_character_trained_with(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_last_character_trained_with();
}

void _set_last_character_trained_with(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_last_character_trained_with(args->slots[0].i);
}

void _get_konq_profile_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_konq_profile_value(args->slots[0].i, args->slots[1].i);
}

void _set_konq_profile_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konq_profile_value(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _set_pui_status(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_pui_status(args->slots[0].pointer, args->slots[1].i);
}

void _get_pui_status(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        get_pui_status(args->slots[0].pointer);
}

void _hf_bgnd_set_in_setup_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hf_bgnd_set_in_setup_zone(args->slots[0].i, args->slots[1].i);
}

void _hf_bgnd_set_smasher_mode(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hf_bgnd_set_smasher_mode(args->slots[0].i, args->slots[1].i);
}

void _bgnd_set_player_shadow_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_player_shadow_ground_plane(
        args->slots[0].i, args->slots[1].f);
}

void _bgnd_end_the_game_and_restart(void) {
    bgnd_end_the_game_and_restart();
}

void _bgnd_pfx_resume_effect(void) {
    bgnd_pfx_resume_effect(get_script_string_arg(1));
}

void _bgnd_pfx_reset_effect(void) {
    bgnd_pfx_reset_effect(get_script_string_arg(1));
}

void _bgnd_reset_players_animation_height(void) {
    bgnd_reset_players_animation_height();
}

void _nbc_script_debug_point(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    nbc_script_debug_point(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _bgnd_unhide_mirror_guys(void) {
    bgnd_unhide_mirror_guys();
}

void _bgnd_hide_mirror_guys(void) {
    bgnd_hide_mirror_guys();
}

void _bgnd_set_sobj_uv_scroll_abs_values(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_uv_scroll_abs_values(
        args->slots[0].f, args->slots[1].f,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].i);
}

void _bgnd_set_sobj_uv_scroll_rate_values(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_uv_scroll_rate_values(
        args->slots[0].f, args->slots[1].f,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].i);
}

void _bgnd_init_all_uv_scroll_w_control(void) {
    bgnd_init_all_uv_scroll_w_control();
}

void _bgnd_destroy_sobj_uv_scroll_w_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_destroy_sobj_uv_scroll_w_control(args->slots[0].i);
}

void _bgnd_start_sobj_uv_scroll_w_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        bgnd_start_sobj_uv_scroll_w_control(
            args->slots[0].i, args->slots[1].f,
            args->slots[2].f, args->slots[3].f,
            args->slots[4].f, args->slots[5].i,
            args->slots[6].i);
}

void _sh_lower_level_pebble_unhide(void) {
    sh_lower_level_pebble_unhide();
}

void _sh_lower_level_pebble_hide(void) {
    sh_lower_level_pebble_hide();
}

void _bgnd_get_camera_z_pos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_camera_z_pos();
}

void _bgnd_get_camera_y_angle(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_camera_y_angle();
}

void _bgnd_delete_proc_by_id(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_delete_proc_by_id(args->slots[0].i);
}

void _bgnd_start_script_in_proc_bigstack(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_script_in_proc_bigstack(args->slots[0].i, args->slots[1].i);
}

void _bgnd_start_script_in_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_script_in_proc(args->slots[0].i, args->slots[1].i);
}

void _npc_run_shove_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_run_shove_animation(args->slots[0].i);
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
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_collision_direction_in_script();
}

void _npc_punch_reaction_check_data(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_punch_reaction_check_data();
}

void _npc_run_punch_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_run_punch_animation(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f);
}

void _npc_snap_to_face_monk(void) {
    npc_snap_to_face_monk();
}

void _interior_exit_button_script(void) {
    interior_exit_button_script();
}

void _get_ir_cam_ang_z(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_z(args->slots[0].i);
}

void _get_ir_cam_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_y(args->slots[0].i);
}

void _get_ir_cam_ang_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_x(args->slots[0].i);
}

void _get_ir_cam_pos_z(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_z(args->slots[0].i);
}

void _get_ir_cam_pos_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_y(args->slots[0].i);
}

void _get_ir_cam_pos_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_x(args->slots[0].i);
}

void _start_konquest_interior(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_konquest_interior(
        args->slots[0].pointer,
        args->slots[1].pointer,
        args->slots[2].pointer,
        args->slots[3].pointer,
        args->slots[4].pointer,
        args->slots[5].pointer,
        args->slots[6].i);
}

void _refresh_rate(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = refresh_rate();
}

void _show_shujinko_unlock_screen(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_shujinko_unlock_screen(args->slots[0].i);
}

void _trial_register_special_move(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_register_special_move(args->slots[0].i);
}

void _trial_invisible_callback(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = trial_invisible_callback(args->slots[0].i);
}

void _trial_set_special_restrictions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_special_restrictions(args->slots[0].i);
}

void _drone_set_handicap(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_handicap(args->slots[0].i, current_args, args->slots[1].f);
}

void _drone_start_bleeding(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_start_bleeding(args->slots[0].i, current_args, args->slots[1].f);
}

void _drone_set_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_damage_multiplier(args->slots[0].i, current_args, args->slots[1].f);
}

void _release_kamidogu(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    release_kamidogu(args->slots[0].i, args->slots[1].i);
}

void _start_constrain_proc(void) {
    start_constrain_proc();
}

void _credits_add_text(void) {
    credits_add_text(
        get_script_string_arg(1), get_script_string_arg(2),
        ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

void _trial_state_collision_check(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_state_collision_check(args->slots[0].i, args->slots[1].i);
}

void _trial_debug_mission_list(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_debug_mission_list(args->slots[0].i);
}

void _trial_get_background_root(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = trial_get_background_root();
}

void _build_bones_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = build_bones_tbl(args->slots[0].pointer, args->slots[1].pointer);
}

void _play_background_music(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    play_background_music(args->slots[0].i);
}

void _give_koin_award(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    give_koin_award(args->slots[0].i, args->slots[1].i);
}

void _konquest_run_ending(void) {
    konquest_run_ending();
}

void _trial_set_round_health_restoration(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_round_health_restoration(current_args, args->slots[0].f);
}

void _camera_set_anim_aux_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_anim_aux_data(
        args->slots[0].pointer);
}

void _trial_mirror_anims_if_needed(void) {
    trial_mirror_anims_if_needed();
}

void _current_player_is_drone(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = current_player_is_drone();
}

void _drone_apply_damage(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_apply_damage(args->slots[0].i, current_args, args->slots[1].f);
}

void _drone_set_special_directions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_special_directions(args->slots[0].i, args->slots[1].i);
}

void _drone_set_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_position(args->slots[0].i, args->slots[1].f,
                       args->slots[2].f, args->slots[3].f);
}

void _drone_set_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_health(args->slots[0].i, current_args, args->slots[1].f);
}

void _drone_do_special_move(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_do_special_move(args->slots[0].i, args->slots[1].i);
}

void _drone_change_to_style(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_change_to_style(args->slots[0].i, args->slots[1].i);
}

void _drone_lip_synch(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_lip_synch(args->slots[0].i,
                    args->slots[1].pointer);
}

void _trial_show_monk(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_show_monk(args->slots[0].i);
}

void _show_text(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_text(
        args->slots[0].i, args->slots[1].i, args->slots[2].i,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i, args->slots[7].i);
}

void _trial_set_next_mission(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_next_mission(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i, args->slots[6].i, args->slots[7].i);
}

void _trial_setup_nis_scene(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_setup_nis_scene(args->slots[0].i);
}

void _drone_set_anim_step(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_anim_step(current_args, args->slots[0].f);
}

void _drone_face_monk(void) {
    drone_face_monk();
}

void _drone_set_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_script(args->slots[0].i, args->slots[1].i);
}

void _trial_do_dialog(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_do_dialog(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].f, args->slots[5].i,
        args->slots[6].i);
}

void _drone_set_difficulty_level(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_difficulty_level(args->slots[0].i);
}

void _drone_dispatch_switches(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_dispatch_switches(args->slots[0].i);
}

void _drone_set_switch_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_switch_state(args->slots[0].i, args->slots[1].i);
}

void _trial_restart_round(void) {
    trial_restart_round();
}

void _trial_start_countdown(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_start_countdown(args->slots[0].i,
                          args->slots[1].f,
                          args->slots[2].f);
}

void _trial_set_move_message(void) {
    trial_set_move_message(
        get_script_string_arg(1), get_script_string_arg(2));
}

void _trial_set_next_setup_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_next_setup_function(args->slots[0].i);
}

void _trial_set_combo_requirement(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_combo_requirement(args->slots[0].i, current_args, args->slots[1].f);
}

void _trial_add_required_sequence(void) {
    trial_add_required_sequence(
        get_script_string_arg(1), get_script_string_arg(2));
}

void _trial_setup_onscreen_display_items(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_setup_onscreen_display_items(
        args->slots[0].i, args->slots[1].i,
        get_script_string_arg(3));
}

void _trial_add_success_condition(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_add_success_condition(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _trial_set_type(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_type(args->slots[0].i);
}

void _trial_show_spoken_text_window(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_show_spoken_text_window(args->slots[0].i,
                                  args->slots[1].f,
                                  args->slots[2].f,
                                  args->slots[3].f,
                                  args->slots[4].i,
                                  args->slots[5].i,
                                  args->slots[6].i,
                                  args->slots[7].i,
                                  args->slots[8].i);
}

void _trial_show_text_window(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_show_text_window(
        args->slots[0].i, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].i);
}

void _trial_set_ending_functions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_ending_functions(args->slots[0].i, args->slots[1].i);
}

void _trial_set_tick_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_tick_function(args->slots[0].i);
}

void _trial_set_round_timer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_round_timer(args->slots[0].i);
}

void _trial_set_num_rounds(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_num_rounds(args->slots[0].i);
}

void _konquest_map_setup_fight(void) {
    struct ScriptRawArgs* args;
    const char* arena_name;

    args = (struct ScriptRawArgs*)current_args;
    arena_name = get_script_string_arg(9);
    konquest_map_setup_fight(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].i, args->slots[3].i,
        args->slots[4].i, args->slots[5].i,
        args->slots[6].i, args->slots[7].i, arena_name);
}

void _initialize_clone_lights(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    initialize_clone_lights(args->slots[0].i);
}

void _jab_spawn_point_light_at_world_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        jab_spawn_point_light_at_world_pos(
            args->slots[0].pointer, args->slots[1].pointer);
}

void _jab_attach_point_light_to_obj_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = jab_attach_point_light_to_obj_bone(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _jab_flash_screen(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_flash_screen(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

void _jab_stop_dragon_king_shake(void) {
    jab_stop_dragon_king_shake();
}

void _jab_shake_dragon_king(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_shake_dragon_king(current_args, args->slots[0].f, args->slots[1].f);
}

void _fade_fatality_screen(void) {
    fade_fatality_screen();
}

void _jab_attach_wiff_to_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_attach_wiff_to_sobj(
        args->slots[0].pointer, args->slots[1].i,
        get_script_string_arg(3), get_script_string_arg(4),
        args->slots[4].i, args->slots[5].f, args->slots[6].i);
}

void _jab_destroy_drink_obj_in_hand(void) {
    jab_destroy_drink_obj_in_hand();
}

void _jab_attach_drink_obj_to_hand(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_attach_drink_obj_to_hand(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _jab_face_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_face_obj(args->slots[0].pointer, args->slots[1].pointer);
}

void _obj_scale_over_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_scale_over_time(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _jab_release_jade_boomerang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_release_jade_boomerang(args->slots[0].i);
}

void _jab_start_jade_boomerang_throw(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_start_jade_boomerang_throw(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _jab_setup_kiss_emitter_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_setup_kiss_emitter_obj(args->slots[0].pointer);
}

void _kill_konquest_dialog_procs(void) {
    kill_konquest_dialog_procs();
}

void _hero_turn_to_face_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hero_turn_to_face_position(args->slots[0].i);
}

void _attach_pfx_to_object(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    effect_name = get_script_string_arg(2);
    args = (struct ScriptRawArgs*)current_args;
    attach_pfx_to_object(
        args->slots[0].pointer, effect_name,
        args->slots[2].pointer);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_set_current_portal_uid(args->slots[0].i);
}

void _npc_sleep_until_model_loaded(void) {
    npc_sleep_until_model_loaded();
}

void _set_konquest_weather(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_weather(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _set_age_progression(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_age_progression(args->slots[0].i);
}

void _display_time_progression_images(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    display_time_progression_images(args->slots[0].i);
}

void _konquest_use_portal(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_use_portal(
        args->slots[0].i, args->slots[1].pointer, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i);
}

void _konquest_teleport_hero_to_location(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_teleport_hero_to_location(args->slots[0].pointer);
}

void _show_fight_message(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_fight_message(args->slots[0].i);
}

void _npc_start_blood_fall(void) {
    npc_start_blood_fall();
}

void _npc_get_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_obj(args->slots[0].i);
}

void _get_hero_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_hero_state();
}

void _interaction_cam_set_target_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    interaction_cam_set_target_info(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i);
}

void _set_movement_npc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_movement_npc(args->slots[0].i);
}

void _set_look_at_npc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_look_at_npc(args->slots[0].i);
}

void _get_krypt_anim_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_krypt_anim_pdata();
}

void _get_krypt_character_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_krypt_character_obj();
}

void _get_krypt_current_column(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_krypt_current_column();
}

void _get_krypt_current_row(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_krypt_current_row();
}

void _transition_to_krypt_character_anim_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    transition_to_krypt_character_anim_script(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _set_krypt_character_anim_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_krypt_character_anim_script(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _set_krypt_character_previous_root_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_krypt_character_previous_root_angle(current_args, args->slots[0].f);
}

void _set_krypt_character_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_krypt_character_angle(current_args, args->slots[0].f);
}

void _set_krypt_character_pos(void) {
    struct ScriptPointerArgs* args;

    args = (struct ScriptPointerArgs*)current_args;
    set_krypt_character_pos(args->value);
}

void _load_krypt_character(void) {
    ((struct ScriptMkObjResult*)active_cmdscript)->value =
        load_krypt_character(get_script_string_arg(1));
}

void _npc_start_fx_at_his_position(void) {
    npc_start_fx_at_his_position(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        get_script_string_arg(2),
        ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

void _npc_start_fx_at_position(void) {
    npc_start_fx_at_position(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _hero_start_fx_at_position(void) {
    hero_start_fx_at_position(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

void _set_hero_position_relative_to_chest(void) {
    set_hero_position_relative_to_chest();
}

void _get_pickup_object(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_pickup_object();
}

void _set_reference_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_reference_pui(args->slots[0].pointer);
}

void _open_chest_and_give_item_to_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    open_chest_and_give_item_to_player(
        args->slots[0].pointer, args->slots[1].pointer);
}

void _open_chest_and_unlock_kontent(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    open_chest_and_unlock_kontent(
        args->slots[0].pointer, args->slots[1].i);
}

void _give_krypt_key_to_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    give_krypt_key_to_player(
        args->slots[0].pointer, args->slots[1].i);
}

void _give_reward_to_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    give_reward_to_player(args->slots[0].pointer);
}

void _npc_play_two_player_one_shot_anims(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_play_two_player_one_shot_anims(args->slots[0].i, args->slots[1].i);
}

void _npc_assign_door_path(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_assign_door_path(args->slots[0].i, args->slots[1].i);
}

void _npc_assign_path_to_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_assign_path_to_him(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _npc_assign_path(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_assign_path(args->slots[0].i, args->slots[1].i, args->slots[2].i);
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
    struct ScriptRawArgs* args;
    char* owner;

    owner = get_script_string_arg(1);
    args = (struct ScriptRawArgs*)current_args;
    konquest_setup_pui_particle(owner, args->slots[1].i);
}

void _npc_wait_for_wake_up(void) {
    npc_wait_for_wake_up();
}

void _npc_set_wake_up_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_wake_up_time(args->slots[0].i, args->slots[1].i);
}

void _save_hero_position_and_angle_prior_to_fight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    save_hero_position_and_angle_prior_to_fight(current_args, args->slots[0].f);
}

void _npc_set_my_conversation_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_conversation_counter(args->slots[0].i);
}

void _npc_set_his_conversation_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_conversation_counter(args->slots[0].i, args->slots[1].i);
}

void _npc_set_my_punch_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_punch_counter(args->slots[0].i);
}

void _npc_set_his_punch_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_punch_counter(args->slots[0].i, args->slots[1].i);
}

void _stop_chest_camera_script(void) {
    stop_chest_camera_script();
}

void _konquest_run_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_run_camera_script(args->slots[0].i, args->slots[1].i);
}

void _set_hero_punched_ground_collisions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_hero_punched_ground_collisions(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_hours_to_time(args->slots[0].i, args->slots[1].i);
}

void _add_days_to_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_days_to_time(args->slots[0].i, args->slots[1].i);
}

void _add_months_to_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_months_to_time(args->slots[0].i, args->slots[1].i);
}

void _add_years_to_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_years_to_time(args->slots[0].i, args->slots[1].i);
}

void _pui_play_pfx_sequence(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_play_pfx_sequence(
        (KonquestPuiDefinition*)args->slots[0].i,
        args->slots[1].i,
        (KonquestPuiPfxSequenceRow*)args->slots[2].i);
}

void _pui_play_pfx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_play_pfx(
        (KonquestPuiDefinition*)args->slots[0].i,
        args->slots[1].i,
        get_script_string_arg(3));
}

void _pui_set_kill_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_set_kill_time(
        args->slots[0].pointer, args->slots[1].i,
        args->slots[2].i);
}

void _pui_set_color(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_set_color(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i);
}

void _pui_delay_spawn(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_delay_spawn(args->slots[0].pointer, args->slots[1].f);
}

void _transition_to_region(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    transition_to_region(args->slots[0].i);
}

void _get_previous_konquest_region_number(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_previous_konquest_region_number();
}

void _set_konquest_region_number(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_region_number(args->slots[0].i);
}

void _npc_play_teleported_sound(void) {
    npc_play_teleported_sound();
}

void _konquest_hide_damashi(void) {
    konquest_hide_damashi();
}

void _konquest_start_damashi(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        konquest_start_damashi(
            current_args, args->slots[0].f, args->slots[1].f,
            args->slots[2].f);
}

void _konquest_camera_return_to_normal(void) {
    konquest_camera_return_to_normal();
}

void _display_konquest_text(void) {
    struct ScriptRawArgs* args;
    float left_fraction;
    float bottom_fraction;
    float width_fraction;
    unsigned int string_id;
    unsigned int prompt_flags;

    args = (struct ScriptRawArgs*)current_args;
    left_fraction = args->slots[0].f;
    bottom_fraction = args->slots[1].f;
    width_fraction = args->slots[2].f;
    string_id = args->slots[3].i;
    prompt_flags = args->slots[4].i;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        display_konquest_text(
            left_fraction, bottom_fraction, width_fraction,
            string_id, prompt_flags);
}

void _hero_stop_moving(void) {
    hero_stop_moving();
}

void _set_monk_age(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_monk_age(args->slots[0].i);
}

void _get_monk_age(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_monk_age();
}

void _change_monk_age(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    change_monk_age(args->slots[0].i);
}

void _hero_handle_conversation(void) {
    hero_handle_conversation();
}

void _nis_clear_event_list(void) {
    nis_clear_event_list();
}

void _npc_switch_camera_focus(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_switch_camera_focus(args->slots[0].i);
}

void _npc_at_waypoint_set_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_at_waypoint_set_flags(args->slots[0].i, args->slots[1].i);
}

void _kill_dynamic_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kill_dynamic_pui((void*)args->slots[0].i);
}

void _spawn_dynamic_pui_critical(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = spawn_dynamic_pui_critical(args->slots[0].i);
}

void _spawn_dynamic_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = spawn_dynamic_pui(args->slots[0].i);
}

void _kill_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kill_pui((KonquestPuiDefinition*)args->slots[0].i);
}

void _pickup_dynamic_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pickup_dynamic_pui((KonquestPuiDefinition*)args->slots[0].i);
}

void _pickup_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pickup_pui((KonquestPuiDefinition*)args->slots[0].i);
}

void _spawn_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spawn_pui(
        (KonquestPuiDefinition*)args->slots[0].i,
        args->slots[1].i, args->slots[2].i);
}

void _attach_pfx_to_object_by_uid(void) {
    struct ScriptRawArgs* current;
    struct ScriptRawArgs* saved;
    const char* effect_name;

    saved = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(2);
    current = (struct ScriptRawArgs*)current_args;
    attach_pfx_to_object_by_uid(
        current->slots[0].i, effect_name,
        saved->slots[2].pointer,
        saved->slots[3].i);
}

void _add_trigger_list_to_world(void) {
    add_trigger_list_to_world();
}

void _npc_sleep(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_sleep(current_args, args->slots[0].f);
}

void _npc_get_punch_count(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_punch_count();
}

void _npc_get_conversation_count(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_conversation_count();
}

void _konquest_passed_last_mission(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = konquest_passed_last_mission();
}

void _konquest_transition_object_to_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_transition_object_to_state(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _npc_fire_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_fire_trigger(args->slots[0].i);
}

void _konquest_open_door(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_open_door(args->slots[0].i, args->slots[1].i);
}

void _get_active_npc_data(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_active_npc_data();
}

void _npc_glitch_him_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_glitch_him_to_ani(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _npc_glitch_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_glitch_to_ani(args->slots[0].i, args->slots[1].i);
}

void _npc_ani_1_frame(void) {
    npc_ani_1_frame();
}

void _npc_set_random_dialog_and_anim_sequence(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_random_dialog_and_anim_sequence(args->slots[0].i, args->slots[1].i);
}

void _npc_play_random_dialog_sequence(void) {
    npc_play_random_dialog_sequence();
}

void _npc_play_dialog_and_anim_sequence(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_play_dialog_and_anim_sequence(args->slots[0].i, args->slots[1].i);
}

void _play_beam_advance_sound(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    play_beam_advance_sound(args->slots[0].i);
}

void _show_objective_arrow_and_beam(void) {
    show_objective_arrow_and_beam();
}

void _hide_objective_arrow_and_beam(void) {
    hide_objective_arrow_and_beam();
}

void _npc_attack(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_attack(args->slots[0].i, args->slots[1].i);
}

void _npc_change_path_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_change_path_speed(args->slots[0].f);
}

void _npc_set_pinanim_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_pinanim_flag(args->slots[0].i);
}

void _npc_set_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_ani_frame(args->slots[0].f);
}

void _npc_set_my_ground_level(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_ground_level(current_args, args->slots[0].f);
}

void _turn_to_face_interior_door(void) {
    turn_to_face_interior_door();
}

void _turn_to_face_exterior_door(void) {
    turn_to_face_exterior_door();
}

void _start_subobject_pulsing_effect(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_subobject_pulsing_effect(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    transition_hero_to_anim_script(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f);
}

void _close_exterior_doors(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    close_exterior_doors(args->slots[0].i, args->slots[1].i);
}

void _get_doors_for_exterior(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_doors_for_exterior();
}

void _get_building_id_for_exterior(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_building_id_for_exterior();
}

void _npc_set_ani_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_ani_flags(args->slots[0].i);
}

void _npc_blend_to_ani_string(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_blend_to_ani_string(args->slots[0].i);
}

void _npc_set_gravity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_gravity(current_args, args->slots[0].f);
}

void _remove_npc_list(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    remove_npc_list(args->slots[0].i);
}

void _setup_interior_fighting_arena(void) {
    setup_interior_fighting_arena();
}

void _npc_get_his_flag_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_his_flag_state(args->slots[0].i, args->slots[1].i);
}

void _npc_get_flag_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_flag_state(args->slots[0].i);
}

void _npc_set_his_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_flags(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _npc_ignore_his_events(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ignore_his_events(args->slots[0].i, args->slots[1].i);
}

void _npc_ignore_events(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ignore_events(args->slots[0].i);
}

void _npc_set_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_flags(args->slots[0].i, args->slots[1].i);
}

void _npc_enable_event(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_enable_event(args->slots[0].i, args->slots[1].i);
}

void _npc_enable_his_event(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_enable_his_event(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _npc_set_dialog_anim(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_dialog_anim(args->slots[0].i);
}

void _npc_turn_and_face_next_waypoint(void) {
    npc_turn_and_face_next_waypoint();
}

void _npc_turn_and_face_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_turn_and_face_player(args->slots[0].i);
}

void _set_interaction_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_interaction_camera_script(args->slots[0].pointer);
}

void _npc_play_conversation_part(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_play_conversation_part(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _conversation_term(void) {
    conversation_term();
}

void _conversation_init(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    conversation_init(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_character_separation_process(args->slots[0].f);
}

void _konquest_start_npc_interaction(void) {
    konquest_start_npc_interaction();
}

void _remove_widescreen_bars(void) {
    remove_widescreen_bars();
}

void _add_widescreen_bars(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_widescreen_bars(args->slots[0].f);
}

void _npc_take_control_of_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_take_control_of_him(args->slots[0].i, args->slots[1].i);
}

void _konquest_fade_hud(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_fade_hud(args->slots[0].i);
}

void _konquest_hide_hud(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_hide_hud(args->slots[0].i);
}

void _konquest_show_hud(void) {
    konquest_show_hud();
}

void _display_konquest_title(void) {
    display_konquest_title();
}

void _enable_attached_sound_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_attached_sound_by_uid(args->slots[0].i, args->slots[1].i);
}

void _attach_sound_to_object_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    attach_sound_to_object_by_uid(
        args->slots[0].i, args->slots[1].i, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].i);
}

void _start_konquest_ambient_sounds(void) {
    start_konquest_ambient_sounds();
}

void _stop_konquest_ambient_sounds(void) {
    stop_konquest_ambient_sounds();
}

void _npc_set_my_movement_weight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_movement_weight(current_args, args->slots[0].f, args->slots[1].f);
}

void _npc_set_ani_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_ani_speed(args->slots[0].f);
}

void _bgnd_start_sobj_uv_scroll_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_sobj_uv_scroll_tbl(
        (BgndUvScrollEntry*)args->slots[0].i);
}

void _npc_set_snap_to_ground(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_snap_to_ground(args->slots[0].i);
}

void _npc_stop_goro_bone_match(void) {
    npc_stop_goro_bone_match();
}

void _npc_start_goro_bone_match(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_start_goro_bone_match(args->slots[0].i);
}

void _npc_ani_for_x_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ani_for_x_ticks(args->slots[0].i);
}

void _npc_ani_to_blend_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ani_to_blend_frame(current_args, args->slots[0].f);
}

void _start_time_passing(void) {
    start_time_passing();
}

void _stop_time_passing(void) {
    stop_time_passing();
}

void _get_tile_sobj_by_id(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_tile_sobj_by_id(args->slots[0].i);
}

void _get_konquest_tile_objects_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_konquest_tile_objects_obj();
}

void _get_current_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_current_time(args->slots[0].pointer);
}

void _set_current_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_current_time((void*)args->slots[0].i);
}

void _npc_restart_his_normal_behavior(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_restart_his_normal_behavior(args->slots[0].i);
}

void _npc_set_his_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_ang_y(args->slots[0].i, current_args, args->slots[1].f);
}

void _npc_set_my_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_ang_y(current_args, args->slots[0].f);
}

void _npc_set_his_world_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_world_pos(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _npc_set_my_world_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_world_pos(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _npc_set_my_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_pos(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _restore_collision_volume_on_object_with_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    restore_collision_volume_on_object_with_uid(args->slots[0].i);
}

void _remove_collision_volume_on_object_with_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    remove_collision_volume_on_object_with_uid(args->slots[0].i);
}

void _restore_collision_volume_on_object(void) {
    restore_collision_volume_on_object();
}

void _remove_collision_volume_on_object(void) {
    remove_collision_volume_on_object();
}

void _npc_ani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ani_to_frame_x(current_args, args->slots[0].f);
}

void _konquest_transition_to_fight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_transition_to_fight(args->slots[0].i);
}

void _attach_wiff_to_konquest_object_by_uid(void) {
    struct ScriptRawArgs* args;
    char* name;

    name = get_script_string_arg(2);
    args = (struct ScriptRawArgs*)current_args;
    attach_wiff_to_konquest_object_by_uid(
        args->slots[0].i, name, args->slots[2].f);
}

void _set_konquest_object_render_order_priority_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_object_render_order_priority_by_uid(args->slots[0].i, args->slots[1].i);
}

void _disable_konquest_object_zwrite_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_konquest_object_zwrite_by_uid(args->slots[0].i);
}

void _set_konquest_object_face_y_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_object_face_y_by_uid(args->slots[0].i);
}

void _unhide_konquest_object_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    unhide_konquest_object_by_uid(args->slots[0].i);
}

void _hide_konquest_object_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hide_konquest_object_by_uid(args->slots[0].i);
}

void _konquest_fade_from_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_fade_from_black(args->slots[0].i, args->slots[1].i);
}

void _konquest_fade_to_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_fade_to_black(args->slots[0].i, args->slots[1].i);
}

void _npc_lip_synch(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_lip_synch(args->slots[0].i, args->slots[1].i);
}

void _npc_ani_to_end(void) {
    npc_ani_to_end();
}

void _npc_wait_for_state_change(void) {
    npc_wait_for_state_change();
}

void _npc_blend_to_ani_with_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_blend_to_ani_with_offset(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f);
}

void _npc_blend_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_blend_to_ani(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f);
}

void _npc_face_current_waypoint_angle(void) {
    npc_face_current_waypoint_angle();
}

void _npc_travel_to_world_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_travel_to_world_position(args->slots[0].i, args->slots[1].i);
}

void _npc_travel_path_anim_override(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_travel_path_anim_override(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

void _npc_travel_path(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_travel_path(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

void _npc_stand_still(void) {
    npc_stand_still();
}

void _add_npc_list_to_world(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_npc_list_to_world(args->slots[0].i);
}

void _fire_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fire_trigger(args->slots[0].pointer);
}

void _set_monk_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_monk_position(args->slots[0].f, args->slots[1].f,
                      args->slots[2].f, args->slots[3].f);
}

void _load_tile_objects(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    load_tile_objects(args->slots[0].i);
}

void _trigger_set_time_for_enable(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trigger_set_time_for_enable(
        args->slots[0].pointer,
        args->slots[1].i, args->slots[2].i,
        args->slots[3].i);
}

void _enable_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_trigger(args->slots[0].pointer, args->slots[1].i);
}

void _assign_obj_to_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    assign_obj_to_trigger(
        args->slots[0].i, args->slots[1].pointer);
}

void _add_object_to_tile(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_object_to_tile(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].i, args->slots[3].f,
        args->slots[4].f, args->slots[5].f,
        args->slots[6].f);
}

void _set_tile_visibility(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_tile_visibility(args->slots[0].i, args->slots[1].i);
}

void _set_tile_grid_size(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_tile_grid_size(args->slots[0].i, args->slots[1].i);
}

void _mks_start_axis_indicator_p_axis_track_bone_world_mat(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_start_axis_indicator_p_axis_track_bone_world_mat(args->slots[0].i, current_args, args->slots[1].f);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_ccp1_eq_insert_cloth_coll_plane(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

void _mks_cc1_expand_cyl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cc1_expand_cyl(current_args, args->slots[0].f, args->slots[1].f);
}

void _mks_cc1_eq_insert_cloth_coll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cc1_eq_insert_cloth_coll(args->slots[0].i, current_args, args->slots[1].f);
}

void _mks_cb1_set_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_scale(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _mks_cb1_set_coll_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_coll_offset(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _mks_cb1_set_coll_offset_xz(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_coll_offset_xz(current_args, args->slots[0].f, args->slots[1].f);
}

void _mks_cb1_add_coll_pt(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_add_coll_pt(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _mks_cb1_set_ground_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_ground_y(current_args, args->slots[0].f);
}

void _mks_set_ground_y_all_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_ground_y_all_cloth_bones(current_args, args->slots[0].f);
}

void _mks_insert_cloth_force_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_insert_cloth_force_bones(args->slots[0].f, args->slots[1].f);
}

void _mks_cb2_eq_cloth_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb2_eq_cloth_bone(args->slots[0].i);
}

void _mks_cb1_eq_cloth_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_eq_cloth_bone(args->slots[0].i);
}

void _mks_mat_id_set_zbias(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_mat_id_set_zbias(args->slots[0].i, args->slots[1].f);
}

void _mks_cloth_bones_init_by_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cloth_bones_init_by_tbl(args->slots[0].pointer, args->slots[1].i);
}

void _mks_start_goro_arms_fixup(void) {
    mks_start_goro_arms_fixup();
}

void _mks_set_flipped_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_flipped_bones(args->slots[0].pointer);
}

void _is_he_blocking(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_he_blocking();
}

void _set_ani_speed_miss_hit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_ani_speed_miss_hit(current_args, args->slots[0].f, args->slots[1].f);
}

void _set_block_requirement(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_block_requirement(args->slots[0].i);
}

void _special_move_cam_end(void) {
    special_move_cam_end();
}

void _special_move_cam_setup(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    special_move_cam_setup(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i,
        args->slots[6].i, args->slots[7].i);
}

void _whoosh_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    whoosh_fx(args->slots[0].i);
}

void _drone_super_combo_refresh(void) {
    drone_super_combo_refresh();
}

void _set_attackers_attack_region(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_attackers_attack_region(args->slots[0].i);
}

void _set_attack_type(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_attack_type(args->slots[0].i);
}

void _got_hit_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    got_hit_fx(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].f, args->slots[6].i);
}

void _camera_set_animation_parent_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_animation_parent_position(
        args->slots[0].pointer);
}

void _camera_set_animation_parent_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_animation_parent_angle(
        args->slots[0].pointer, args->slots[1].i);
}

void _cam_set_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    cam_set_ground_plane(args->slots[0].f);
}

void _set_camera_velocity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_camera_velocity(args->slots[0].pointer);
}

void _get_camera_velocity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_camera_velocity(args->slots[0].pointer);
}

void _set_camera_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_camera_angle(args->slots[0].pointer);
}

void _get_camera_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_camera_angle(args->slots[0].pointer);
}

void _set_camera_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_camera_position(args->slots[0].pointer);
}

void _get_camera_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_camera_position(args->slots[0].pointer);
}

void _fade_from_white(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_from_white(args->slots[0].i, args->slots[1].i);
}

void _fade_to_white(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_to_white(args->slots[0].i, args->slots[1].i);
}

void _fade_from_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_from_black(args->slots[0].i, args->slots[1].i);
}

void _fade_to_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_to_black(args->slots[0].i, args->slots[1].i);
}

void _turn_camera_off(void) {
    turn_camera_off();
}

void _turn_camera_on(void) {
    turn_camera_on();
}

void _camera_run_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_run_animation(args->slots[0].i);
}

void _camera_set_speed_scalar(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_speed_scalar(args->slots[0].f);
}

void _camera_get_victim(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = camera_get_victim();
}

void _camera_set_victim(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_victim(args->slots[0].pointer);
}

void _camera_get_attacker(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = camera_get_attacker();
}

void _camera_set_attacker(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_attacker(args->slots[0].pointer);
}

void _camera_special_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_special_function(args->slots[0].i);
}

void _camera_setup_tightrope_angle_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_setup_tightrope_angle_offset(current_args, args->slots[0].f, args->slots[1].f);
}

void _camera_setup_radial_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_setup_radial_position(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _camera_unpause_player(void) {
    camera_unpause_player();
}

void _camera_pause_player(void) {
    camera_pause_player();
}

void _camera_setup_radial_sweep(void) {
    struct ScriptRawArgs* args;
    float sp8;

    args = (struct ScriptRawArgs*)current_args;
    sp8 = args->slots[8].f;
    camera_setup_radial_sweep(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, sp8);
}

void _camera_set_movement_focus_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_focus_obj((MkObj*)args->slots[0].i);
}

void _camera_set_custom_camera_movement_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_custom_camera_movement_flag(args->slots[0].i);
}

void _camera_set_radial_movement(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_radial_movement(args->slots[0].i);
}

void _camera_set_center_of_rotation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_center_of_rotation(
        args->slots[0].pointer);
}

void _camera_set_travel_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_travel_time(args->slots[0].f);
}

void _camera_set_rotation_direction(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_rotation_direction(args->slots[0].i);
}

void _camera_set_final_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_final_speed(args->slots[0].f);
}

void _camera_set_initial_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_initial_speed(args->slots[0].f);
}

void _camera_set_rotation_rate(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_rotation_rate(args->slots[0].f);
}

void _camera_set_movement_rate(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_rate(args->slots[0].f);
}

void _camera_get_mirror_flag(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = camera_get_mirror_flag();
}

void _camera_set_check_konquest_collisions_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_check_konquest_collisions_flag(args->slots[0].i);
}

void _camera_set_glitch_flag(void) {
    camera_set_glitch_flag();
}

void _camera_set_look_mode(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_look_mode(args->slots[0].i);
}

void _camera_set_movement_mode(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_mode(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_check_reverse_move_offset(args->slots[0].i, args->slots[1].i);
}

void _camera_is_pos_move_done(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = camera_is_pos_move_done();
}

void _camera_is_ang_move_done(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = camera_is_ang_move_done();
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
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_intro_camera_path();
}

void _ani_to_frame_x_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_x_col(
        args->slots[0].f, args->slots[1].i, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].f,
        args->slots[6].i);
}

void _if_collision_autoface_him(void) {
    if_collision_autoface_him();
}

void _if_collision_autoface_me(void) {
    if_collision_autoface_me();
}

void _bulvan_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bulvan_function(args->slots[0].i);
}

void _set_his_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_his_damage_multiplier(current_args, args->slots[0].f);
}

void _adjust_his_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    adjust_his_damage_multiplier(current_args, args->slots[0].f);
}

void _adjust_my_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    adjust_my_damage_multiplier(current_args, args->slots[0].f);
}

void _advance_my_moveset(void) {
    advance_my_moveset();
}

void _disable_my_attacks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_my_attacks(args->slots[0].i);
}

void _wall_eligible_off(void) {
    wall_eligible_off();
}

void _wall_eligible_on(void) {
    wall_eligible_on();
}

void _face_bleed_me(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    face_bleed_me(args->slots[0].i);
}

void _air_collision_pause(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    air_collision_pause(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

void _player_impale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_impale(args->slots[0].pointer, args->slots[1].pointer);
}

void _plyr_weapon_grab(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_weapon_grab(args->slots[0].i, args->slots[1].i);
}

void _weapon_trail_off(void) {
    weapon_trail_off();
}

void _weapon_trail_on(void) {
    weapon_trail_on();
}

void _set_ani_weight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_ani_weight(args->slots[0].f);
}

void _front_rollup_check(void) {
    front_rollup_check();
}

void _disable_joy_temp(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_joy_temp(args->slots[0].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    force_forward(args->slots[0].f, args->slots[1].i,
                  args->slots[2].f, args->slots[3].i);
}

void _ejb_call(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ejb_call(args->slots[0].i);
}

void _myvel_my_angle_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    myvel_my_angle_y(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _rotate_towards_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    rotate_towards_him(args->slots[0].f);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_float_1(current_args, args->slots[0].f);
}

void _release_other_player(void) {
    release_other_player();
}

void _ejb_too_close_repell(void) {
    ejb_too_close_repell();
}

void _ejb_release_other_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ejb_release_other_player(args->slots[0].i);
}

void _damage_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    damage_him(current_args, args->slots[0].f);
}

void _damage_me(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    damage_me(current_args, args->slots[0].f);
}

void _enable_all_my_blocking(void) {
    enable_all_my_blocking();
}

void _turn_me_pi(void) {
    turn_me_pi();
}

void _disable_this_move_exec(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_this_move_exec(args->slots[0].i, args->slots[1].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_facial_damage(current_args, args->slots[0].f);
}

void _random_voice_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    random_voice_him(args->slots[0].i);
}

void _set_my_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_damage_multiplier(current_args, args->slots[0].f);
}

void _disable_attack5(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_attack5(args->slots[0].i);
}

void _special_move_cam_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    special_move_cam_him(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i,
        args->slots[6].i, args->slots[7].i);
}

void _plyr_rotate_obj_y180(void) {
    plyr_rotate_obj_y180();
}

void _check_for_combo_message(void) {
    check_for_combo_message();
}

void _myvel_his_angle_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    myvel_his_angle_y(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _stop_me(void) {
    stop_me();
}

void _myvel_his_angle_y_inout(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    myvel_his_angle_y_inout(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _wait_for_slot_load(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    wait_for_slot_load(args->slots[0].i);
}

void _get_mode_of_play(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_mode_of_play();
}

void _destroy_mkprocs_pid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    destroy_mkprocs_pid(args->slots[0].i);
}

void _kill_lip_sync_procs(void) {
    kill_lip_sync_procs();
}

void _plyr_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_snd_req(args->slots[0].i);
}

void _random_hit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_hit(args->slots[0].i);
}

void _random_hit_n_voice(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    random_hit_n_voice(args->slots[0].i, args->slots[1].i);
}

void _shake_hit_voice(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    shake_hit_voice(args->slots[0].i, args->slots[1].f, args->slots[2].i, args->slots[3].i);
}

void _pan_vol_pitch_random_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pan_vol_pitch_random_snd_req(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _snd_stop(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    snd_stop(args->slots[0].i);
}

void _random_foot(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_foot(args->slots[0].i);
}

void _random_voice(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_voice(args->slots[0].i);
}

void _pan_vol_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pan_vol_snd_req(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

void _pan_vol_pitch_random_hit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pan_vol_pitch_random_hit(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

void _random_snd_req_delay(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    random_snd_req_delay(args->slots[0].i, args->slots[1].i);
}

void _random_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_snd_req(args->slots[0].i);
}

void _set_snd_vol(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_snd_vol(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

void _snd_req_vol(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = snd_req_vol(args->slots[0].i, current_args, args->slots[1].f);
}

void _snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = snd_req(args->slots[0].i);
}

void _ani_to_fall_to_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_fall_to_frame(args->slots[0].f, args->slots[1].i, args->slots[2].f);
}

void _shake_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    shake_camera(args->slots[0].i, current_args, args->slots[1].f);
}

void _force_away(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    force_away(args->slots[0].f, args->slots[1].i, args->slots[2].f, args->slots[3].i);
}

void _play_sound_2(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    play_sound_2(args->slots[0].i, args->slots[1].i);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hit_START_chores(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f);
}

void _land_chores(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    land_chores(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f);
}

void _launch_me_up(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    launch_me_up(args->slots[0].f, args->slots[1].f);
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
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_ani_speed(args->slots[0].f);
}

void _set_my_secondary_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_secondary_state(args->slots[0].i);
}

void _set_my_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_state(args->slots[0].i);
}

void _back_to_normal(void) {
    back_to_normal();
}

void _if_collision_slow_ani_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    if_collision_slow_ani_x(current_args, args->slots[0].f, args->slots[1].f);
}

void _slow_ani_end(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    slow_ani_end(current_args, args->slots[0].f);
}

void _slow_ani_x_if_miss(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    slow_ani_x_if_miss(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

void _slow_ani_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    slow_ani_x(args->slots[0].f, args->slots[1].f);
}

void _ani_to_blend_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_blend_frame(args->slots[0].f);
}

void _ani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_x(args->slots[0].f);
}

void _ani_to_frame_x_aniproc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_x_aniproc(current_args, args->slots[0].f);
}

void _ani_to_frame_sound(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_sound(args->slots[0].f, args->slots[1].f, args->slots[2].i);
}

void _blend_to_stance(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    blend_to_stance(args->slots[0].f);
}
