#include "runtime/script_functions.h"
#include "platform/main.h"
#include "game/game.h"
#include "game/fatality.h"
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
#include "game/plyr_sound.h"
#include "game/cloth.h"
#include "game/mk_chess.h"
#include "game/moves.h"
#include "runtime/sound.h"
#include "game/krypt.h"
#include "math/mk_math.h"
#include "math/gxMath.h"
#include "game/konquest_items.h"
#include "runtime/light.h"
#include "platform/io.h"
#include "game/trial.h"
#include "runtime/anim_pdata.h"
#include "runtime/section.h"
#include "runtime/plyr_anim_pdata.h"
#include "runtime/anim_api.h"
#include "platform/gcutils.h"
#include "game/settings.h"
#include "game/plyr_globals.h"
#include "game/controller.h"
#include "runtime/mk_obj.h"
#include "game/nis.h"
#include "game/konquest_time.h"
#include "game/pwrbar.h"
#include "game/fx.h"
#include "runtime/mk_plugins.h"
#include "game/collision.h"
#include "game/konquest_missions.h"
#include "game/bgnd_jmt.h"
#include "game/ending.h"
extern unsigned char* current_args;
extern unsigned char* active_cmdscript;
extern PuzzleFightersEngine g_pz_fighters_engine;

struct FakeBoneMatcher;
struct MkFlippedBoneMap;
struct MovesAttackInfo;
struct PebbleData;
struct PuiItem;
struct FenceSection;

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
    MkSobj* value;
};

struct ScriptMkObjResult {
    char pad00[0x2C];
    MkObj* value;
};

static float j_sleep_forever(void);
static float j_call_player_script_function(void);

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
    MkSobj* value;
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
    Vec* position;
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
    Vec* value;
};

struct ScriptPebbleBounceArgs {
    unsigned int header;
    int player;
    int index;
    Vec* velocity;
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
    Vec* vector;
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
    Vec* first;
    Vec* second;
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
    Vec* offset;
    Vec* rotation;
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
    Vec* velocity;
    const Vec* rotation;
    Vec* scale;
    Vec* position_offset;
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



struct ScriptActiveState {
    char pad00[8];
    ScriptSlot* state;
};

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
    void (*function)(void);
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

KonquestNpc* find_npc_by_data(KonquestNpcData* data);
void nb_place_slave_in_bgnd(
    int npc_id, int rope_model_index, const char* model_name, int model_id,
    float anchor_x, float anchor_y, float anchor_z, float rope_length,
    float local_angle_x, float local_angle_y, float local_angle_z,
    float object_angle_x, float object_angle_y, float object_angle_z,
    float acceleration_divisor, float acceleration_scale, float field_8C);

unsigned long long stop_usec_timer(int id);
void start_usec_timer(int id);
int printf(const char* format, ...);
void obj_setup_for_animation(MkObj* object, const int* tags, MkFlippedBoneMap* flipped_bone_map, void* ground_colls);
void trial_add_required_attack(unsigned char attack, unsigned char count,
                               int flags);
unsigned short randu0(unsigned short limit);
extern void** bgnd_animation_table;
extern void* pz_shared_ani[];
extern void* npc_fast_anims[];
extern void* konq_nis_anims[];
extern void* konquest_animations[];
extern void* shared_ani[];
void pz_fighter_attack(AniData* animation, PuzzleAttackParameters* attack, int reaction);
void attack_opponent_with(
    AniData* animation, struct MovesAttackInfo* attack, int reaction);
static float j_call_player_script_function(void);
void glitch_to_ani_frame(void* animation, int flags,
                         struct ScriptAnimationArgs* args, float frame);
void glitch_to_ani(AniData* animation, int transition);
void reaction_xfer_him_nohit(void* entry);
int am_i_airborn(void);
int is_my_chest_to_screen(void);
void load_and_set_refl_on_weapon(void);
void advance_active_moveset(PlyrPdata* player);
int get_active_moveset_from_pdata(void* pdata);
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
void bgnd_create_named_npc_in_slot(unsigned int npc_id, const char* model_name, unsigned int animation_id, unsigned int bone_table);
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
MkObj* load_cloth_boned_model(const char* name, int object_id, int slot, int flags, const int* bone_tags, ClothInitEntry* cloth_table, int cloth_count);
void konquest_map_setup_fight(
    int first, int second, int third, int fourth, int fifth, int sixth,
    int seventh, int eighth, const char* arena_name);
void fxsys_set_v3(int parameter, float x, float y, float z);
void fx_bind_render_to_sobj(unsigned int handle, MkSobj* object);
void fx_get_v3(unsigned int handle, int field, Vec* value);
struct PfxScriptColorRow;
void update_lerp_color(int color_field, int age_field, float duration,
                       int color_count, int first_color,
                       struct PfxScriptColorRow* table);
void update_fade_alpha2(int color_field, int age_field, float start_time,
                        float duration, int start_alpha, int end_alpha);
void fx_resume_emit(unsigned int handle);
void reset_effect(void);
void emit_from_pos_clamp_y(int a, int b, float c, float d, float e, float f,
                           float g, float h);
void emit_spherical_section(int a, float b, float c, float d, float e,
                            float f, float g, float h);
void texture_animation_with_vsize(float a, float b, int c, float d);
void emit_cylindrical(int a, float b, float c, float d, float e, float f,
                      float g, float h);
void emit_cartesian(int a, float b, float c, float d, float e, float f,
                    float g);
void parametric_update(const struct PfxParametricEffectDescription* value);
void mkobj_get_matrix_right(void* out, void* object);
void mkobj_get_matrix_at(void* out, void* object);
MkBone* force_calc_bone_world_mat(MkObj* obj, int bone);
typedef struct BoneMatcherState BoneMatcherState;
void bone_matcher_child_set_offset(BoneMatcherState* matcher, Vec* offset);
void bone_matcher_parent_set_offset(BoneMatcherState* matcher, Vec* offset);
void* get_fatality_state_ptr(void);
void animpdata_ani_to_frame_x_with_flag_check(void* anim, float frame,
                                               int flag, int value);
void mkscripts_mkobj_insert_mkobj_cleanuplist(MkHdr* object, MkObj* owner);
void mkscripts_destroy_gusher(MkHdr* gusher);
void mkscripts_destroy_fk_bonematcher(MkHdr* bonematcher);
void mkscripts_destroy_bonematcher(MkHdr* bonematcher);
int dkp_check_plyr_state_for_grab(PlyrPdata* player);
struct PrisonGrabPdata* start_prison_grab_proc(MkObj* object, Vec* direction, float angle, float strength);
void xfer_spearproc_to_retract(MkProc* proc);
void destroy_spearproc_bonematcher(MkProc* proc);
void insert_mkobj_spearproc_parentobjitem(MkObj* parent, MkProc* proc);
MkObj* get_spearobj_from_spearproc(MkProc* proc);
MkProc* fire_sc_spear(PlyrPdata* player, Vec* velocity, int field_34, int flag_40, MkHdr* bound_object, int flag_20);
void subzero_start_ice_chunks(PlyrPdata* player);
MkObj* subzero_start_iceman(void);
MkObj* subzero_start_iceblock(void);
struct FatalityObjectLatch;
int sindel_sonic_waves(float value);
void start_raiden_lightning_scroll(int a, float b, float c, int d, int e);
void* ft_raiden_summon_lightning_bolt(int a, int b, char* name);
void kill_raiden_summon_lightning_bolt(void* value);
void fat_goro_fold_arms(int a, int b, float c, int d);
void* fatality_boraicho_get_jug(int a, int b);
MkObj* fatality_boraicho_get_torch(const Vec* parent_offset, const Vec* rotation);
MkObj* fatality_ashrah_get_doll(const Vec* parent_offset, const Vec* child_offset, const Vec* rotation);
void pfx_spawn_at_bid(const char* name, MkObj* object, int bone_id);
void* limb_sever_throw_away(int a, int b, int c);
void auto_calc_limbobj_bone_world_pos(void* a, void* b);
int attach_gore2_obj(MkObj* owner, int bone, unsigned int object_id,
                     const Vec* offset, const Vec* rotation);
void fatality_explode_victim(PlyrInfo* player_info, float arg1, float arg2);
void kill_gusher(MkProc* proc);

void start_sweat_particles(int particle_mask, int bone, PlyrPdata* player, MkObj* object);
void* start_blood_particles(int a, int b, int c, int d);
void mks_spawn_blood_pool_at_bid(PlyrInfo* source, MkObj* object, int bone, int large);
void spawn_blood_pool_at_bid(PlyrInfo* source, int bone, int large);
MkObj* plyr_weapon2_release(PlyrPdata* player);
MkObj* plyr_weapon_release(PlyrPdata* player);
void bone_matcher_reset_dest_mat_rot(MkObj* object, int bone_index);
void bone_matcher_set_ang_pos(int a, int b, int c, int d, int e, int f);
MkObj* weapon_bm_ignore(int weapon, int ignored);
void weapon_reflection_show_hide(PlyrPdata* player, int secondary, int hidden);
MkObj* show_single_weapon(PlyrPdata* player, int secondary);
void advance_to_weapon_style(PlyrPdata* player);
int is_weapon_style(PlyrFighterDefinition* style);
int am_i_female(PlyrPdata* fighter);
float sobj_set_bounding_sphere_radius(MkSobj* sobj, float radius);
float sobj_get_bounding_sphere_radius(MkSobj* sobj);
MslSoundHandle play_his_snd_req(int sound);
MslSoundHandle play_his_random_voice(int sound);
void obj_unhide_material_by_id(MkObj* object, int id);
void obj_hide_material_by_id(MkObj* object, int id);
void bm_force_fake_child_bid(BoneMatcherState* matcher, int bone_id);
void reset_fake_bone_matcher(struct FakeBoneMatcher* matcher,
                             const Vec* parent_offset,
                             const Vec* child_offset, const Vec* rotation,
                             int bone_index, MkHdr* object, float blend);
MkHdr* ft_fake_bone_matcher(MkHdr* parent, MkHdr* child, int bone_index,
                            const Vec* parent_offset, const Vec* child_offset,
                            const Vec* rotation, int mode, float blend);
int get_game_state(void);
int get_level_fatality_done_flag_state(void);
void set_level_fatality_done_flag_state(int state);
void limb_sever_destroy_existing_attach_proc(PlyrInfo* player, int limb);
MkHdr* limb_sever_pop_head_up(PlyrInfo* player, float x_velocity, float y_velocity,
                              float z_velocity, float angular_velocity);
MkObj* mks_limb_sever(MkObj* object, int limb, int include_children);
struct NcsLimbUpdatePdata* limb_sever_find_existing_update_proc(PlyrInfo* player, int limb, int proc_id);
MkProc* proc_of_anim_pdata(AnimPdata* anim);
void set_pdata_anim_step(AnimPdata* pdata, float step);
void animpdata_ani_to_blend_frame(AnimPdata* anim, float blend_frames);
void animpdata_ani_to_end_at1(AnimPdata* anim);
void animpdata_ani_to_end(AnimPdata* anim);
void animpdata_ani_x_more_frames(AnimPdata* anim, float frames);
void animpdata_ani_loop_more_frames(AnimPdata* anim, float frames);
void animpdata_ani_to_frame_x(AnimPdata* anim, float frame);
void mks_animpdata_set_cur_frame(AnimPdata* anim, float frame);
void animpdata_ani_1_frame(AnimPdata* anim);
void ncs_dkp_camera_konqchar_show_hide_alpha(int character_index, MkObj* character);
void fkbm_obj_face_obj(int a, int b, int c, int d, int e);
int obj_get_bid_for_tid(MkObj* obj, int tag);
void* pfx_plyr_bankowner(int a);
void ncs_script_debug_quickie(int a, float b, int c);
void ck_put_weapon_away(PlyrInfo* player);
MkObj* find_obj_by_id(int id);
void yinyang_reset_music_index(void);
void yinyang_play_evil_tune(void);
void yinyang_play_good_tune(void);
int is_point_in_fortress_exclusion_zone(const Vec* point);
void fortress_setup_exclusion_zone(const Vec* center, float width, float height, float depth, float angle);
void set_evil_swap_status(int a);
int ok_to_do_evil_swap(void);
void set_evil_condition(int a);
int yy_is_evil_time_active(void);
int are_death_traps_on(void);
char* get_string_by_id(unsigned int id);
void get_point_on_circle(float* center, float radius, float angle, float* out);
MkHdr* cut_player_in_half(MkHdr* player);
int get_offset_of_closest_fence_section(const Vec* point,
                                        const struct FenceSection* sections,
                                        int start_index, int mirrored);
void start_fish_attack(MkHdr* player, int direction, int flags);
void debug_create_axis_indicator(MkHdr* player, int axis);
void obj_for_all_atomics_set_material_alpha(MkObj* object, int alpha);
void mab_test(void);
void player_body_explode(MkHdr* player, const Vec* position, float velocity);
void yinyang_set_bad_fish_hide_flag(struct YinyangFishPair* fish, int hide, int count);
void yinyang_set_good_fish_hide_flag(struct YinyangFishPair* fish, int hide, int count);
void yinyang_make_fish_jump(int fish, int velocity);
void destroy_mkobjs_oid(int oid);
void do_yinyang_statue_explosion(MkHdr* statue);
void misc_data_set_test_float(float value);
void misc_data_set_test_u32(unsigned int value);
int random_percent(float percent);
void reset_collision_system(void);
void pos_cam_for_current_level(void);
void reset_severed_limbs(int player);
void set_far_clip_plane(float dist);
void skytemple_player_explode(
    unsigned int player, float x, float y, float z);
void skytemple_make_scream_sound(int player);
void mab_script_trace_func(char* message);
void bgnd_launch_fx_at_sobj_pos(
    const char* name, unsigned int sobj_id, float y_offset);
int bgnd_get_first_shape_center_for_obstacle_id(int obstacle_id,
                                                int scratch_index);
void misc_data_set_obj_ptr1(void* object);
void* misc_data_get_obj_ptr1(void);
void misc_data_set_col_obj_id2(int object_id);
int misc_data_get_col_obj_id2(void);
void misc_data_set_col_obj_id(int object_id);
int misc_data_get_col_obj_id(void);
void run_reaction_cleanup_function(PlyrPdata* player);
int reaction_xfer_him(int reaction, float rate, int strength);
void mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
    PlyrPdata* player);
void mks_xfer_collision_info_plyr_to_bgnd_script(PlyrPdata* player,
                                                  int script_function);
void resume_effect_at_plyr_num_bid(
    int player, int bone_id, unsigned int effect_handle,
    int bind_mode, int blood_required);

void add_npc_list_to_world(int* npc_ids);
void ani_1_frame(void);
void ani_to_blend_frame(float);
void ani_to_end(void);
void ani_to_frame_x(float);
void bgnd_end_the_game_and_restart(void);
void bgnd_hide_mirror_guys(void);
void bgnd_reset_players_animation_height(void);
void bgnd_restore_player(void);
int bgnd_turn_off_backface_culling(int);
int bgnd_turn_on_backface_culling(int);
void bgnd_unhide_mirror_guys(void);
void configure_iceball(MkObj* iceball);
void conversation_init(int mode);
void conversation_term(void);
void damage_him(float amount);
void damage_me(float amount);
void face_bleed_me(int size);
void face_opponent_now(void);
typedef struct KonquestTriggerDefinition KonquestTriggerDefinition;
void flash_hit_at_bid(int bone);
void freeze_player(void);
void head_tracking_off(void);
void hero_handle_conversation(void);
void hero_turn_to_face_position(const Vec* position);
void high_flash_check(void);
void init_air_move(void);
void init_ground_move_no_aniproc(void);
void interior_exit_button_script(void);
void kill_lip_sync_procs(void);
void low_flash_check(void);
void medium_flash_check(void);
void mk_chess_load_chess_table(void*);
struct ChessSpellDefinition;
void npc_ani_1_frame(void);
void npc_ani_for_x_ticks(int ticks);
void npc_ani_to_end(void);
void npc_ani_to_frame_x(float frame);
void npc_blend_to_ani_string(int* animation_ids);
void npc_face_current_waypoint_angle(void);
void npc_fire_trigger(unsigned int);
void npc_ignore_events(int enabled);
void npc_open_door_at_waypoint(void);
void npc_play_random_dialog_sequence(void);
void npc_prepare_for_unconscious_state(void);
void npc_punch_reaction_standard_setup(void);
void npc_punch_reaction_standard_shutdown(void);
void npc_reset_my_timed_events(void);
void npc_restart_his_normal_behavior(KonquestNpcData* data);
void npc_run_shove_animation(int animation_id);
void npc_set_ani_flags(unsigned int flags);
void npc_set_ani_frame(float);
void npc_set_ani_speed(float);
void npc_set_dialog_anim(int animation);
void npc_set_gravity(float gravity);
void npc_set_my_ang_y(float angle);
void npc_set_my_conversation_counter(int count);
void npc_set_my_punch_counter(int count);
void npc_set_pinanim_flag(int enabled);
void npc_set_snap_to_ground(int enabled);
void npc_shove_reaction_standard_setup(void);
void npc_shove_reaction_standard_shutdown(void);
void npc_sleep(float ticks);
void npc_sleep_until_model_loaded(void);
void npc_snap_to_face_monk(void);
void npc_stand_still(void);
void npc_start_blood_fall(void);
void npc_start_goro_bone_match(KonquestNpcData* data);
void npc_stop_goro_bone_match(void);
void npc_switch_camera_focus(int focus_mode);
void npc_turn_and_face_next_waypoint(void);
void npc_turn_and_face_player(int turn_player);
void npc_wait_for_state_change(void);
void npc_wait_for_wake_up(void);
void pz_fighter_allow_continuation(void);
void pz_fighter_allow_easy_continuation(void);
void pz_fighter_create_space_between_fighters(void);
void pz_fighter_create_space_between_fighters_for_special_moves(void);
int pz_fighter_distance_check_wo_super_check(void);
void pz_fighter_disallow_continuation(void);
float pz_fighter_exit(void);
void pz_fighter_function(unsigned int);
float pz_fighter_long_exit(void);
void pz_fighter_reaction_xfer_him(int);
void pz_fighter_release_other_player(int);
void pz_fighter_reset_continuation(void);
void pz_fighter_shaking(void);
void pz_fighter_step_throw_into_check(void);
void pz_fighter_wipe_blood_off_hands(void);
void remove_npc_list(KonquestNpcData* list);
void retract_spear_from_camera(MkProc* proc);
void set_ani_speed(float);
void setup_interior_fighting_arena(void);
void snd_major_hit_voice(void);
void snd_stop(MslSoundHandle handle);
void spad_norm_vector(int);
void start_constrain_proc(void);
int start_special_weapon_monitor(int);
void stop_me(void);
void suspend_in_midair(float ticks);
int trial_debug_mission_list(int);
void turn_to_face_interior_door(void);
void unfreeze_player(void);

void ani_loop_more_frames(float);
void bgnd_collison_if_monitor_col(
    int, unsigned int, int, int, int, unsigned int, int);
void bgnd_create_pebbles(
    int model_index, unsigned int object_id, unsigned int player, int mode,
    unsigned int count);
void bgnd_enable_obj_pos_and_ang_setting(
    MkObj* object, Vec* position, Vec* angles);
void bgnd_jtb_debug_info(float arg0, float arg1, float arg2);
void bgnd_npc_start_ani(
    unsigned int, unsigned int, unsigned int, float, float);
void bgnd_pebble_burst_set_value(int, unsigned int, unsigned int, int, void *,
                                 float, float, float, float, float, float);
void bgnd_pebble_burst_set_value_min_max(int, unsigned int, unsigned int, int,
                                         void *, float, float);
void bgnd_set_active_sobj_in_obj(int, unsigned int);
void bgnd_set_active_sobj_pos(void *, float, float, float);
void bgnd_set_collision_plane_for_launched_sobj(int, unsigned int,
                                                 unsigned int);
void bgnd_set_launch_velocity_based_on_sobj_pos(
    int velocity_index, unsigned int source_id, unsigned int target_id,
    void* script, float horizontal_velocity, float vertical_velocity);
void bgnd_set_sobj_launch_params(int, int, unsigned int, void *, float, float,
                                 float, float, float, float, float);
void bgnd_set_sobj_launch_params_exact(
    int velocity_index, int angle_index, void* script, float impact_scale,
    float angle_x, float angle_y, float angle_z, float vertical_velocity,
    float heading);
void bgnd_sobj_cam_frustum_test_into_transparent(
    unsigned int, float, float);
void bgnd_start_preload_sobj_uv_scroll(
    int, int, float, float, float, float);
void bgnd_start_script_in_proc_bigstack(
    int process_id, unsigned int script_index);
void close_exterior_doors(int building_id, int door_bits);
void damage_player(PlyrPdata* source, float amount);
void dk_voice_call(int voice_group, unsigned int cooldown_ticks);
void drone_lip_synch(int, LipSyncKeyframe*);
void fade_from_black(int frames, int flag);
void fade_from_white(int frames, int flag);
void fade_to_black(int frames, int flag);
void fade_to_white(int frames, int flag);
void fight_fx_im_hit_flash(int player, MkObj* object, int bone, int use_bone, void* script_args, float y_offset);
void flash_hit_at_bid_with_y(int bone, float y_offset);
void hf_bgnd_set_in_setup_zone(int* ptr, int value);
void hf_bgnd_set_smasher_mode(int* ptr, int value);
void load_script_as_reaction(unsigned int slot, int script);
typedef struct ClothInitEntry ClothInitEntry;
void nb_npc_slave_plyr_process_collision(unsigned int);
void nbc_script_debug_point(int arg0, int arg1, float arg2);
void ncs_set_pebble_pos(PebbleData* data, int index, Vec* position);
void npc_ani_to_blend_frame(float blend_frames);
void npc_assign_door_path(long, int);
void npc_assign_path(void* path, int flags, int travel_mode);
void npc_assign_path_to_him(KonquestNpcData* data, void* path, int flags, int travel_mode);
void npc_at_waypoint_set_flags(int flags, int enabled);
void npc_attack(int attack_arg_a, int attack_arg_b);
void npc_blend_to_ani( int animation_id, unsigned int flags, float blend, float speed);
void npc_blend_to_ani_with_offset( int animation_id, int flags, float blend, float step);
void npc_change_path_speed(float);
void npc_enable_event(int event_index, int enabled);
void npc_enable_his_event(KonquestNpcData* data, int event_index, int enabled);
void npc_glitch_him_to_ani(KonquestNpcData* data, int animation_id, int flags);
void npc_glitch_to_ani(int animation_id, int flags);
void npc_ignore_his_events(KonquestNpcData* data, int enabled);
void npc_lip_synch(int sound_id, LipSyncKeyframe* keyframes);
void npc_play_conversation_part(int dialog_id, int animation_id, int conversation_mode);
void npc_play_dialog_and_anim_sequence(int dialog_id, int animation_id);
void npc_play_two_player_one_shot_anims(int npc_animation_id, int monk_animation_id);
void npc_run_punch_animation(int animation_id, int flags, int unused, int use_blend, void* script_args, float gravity);
void npc_set_flags(int flags, int enabled);
void npc_set_his_ang_y(KonquestNpcData* data, float angle);
void npc_set_his_conversation_counter(KonquestNpcData* data, int count);
void npc_set_his_flags(KonquestNpcData* data, int flags, int enabled);
void npc_set_his_punch_counter(KonquestNpcData* data, int count);
void npc_set_his_world_pos( KonquestNpcData* data, float x, float y, float z);
void npc_set_my_ground_level(float ground_level);
void npc_set_my_movement_weight(float root_weight, float object_weight);
void npc_set_my_pos(float x, float y, float z);
void npc_set_my_world_pos(float x, float y, float z);
void npc_set_random_dialog_and_anim_sequence(int, int);
void npc_set_wake_up_time(int unit, int amount);
void npc_start_fx_at_his_position(KonquestNpcData* data, const char* effect_name, const Vec* offset);
void npc_start_fx_at_position(const char*, const Vec*);
void npc_take_control_of_him(KonquestNpcData* data, unsigned int script_function);
void npc_travel_path(int, int, int);
void npc_travel_path_anim_override(int path_id, int path_arg, int animation, int travel_mode);
void npc_travel_to_world_position(Vec* position, int travel_mode);
int obj_set_z_offsets(int, void *, float);
void obj_sobj_cam_frustum_test_into_transparent(
    MkObj*, unsigned int, float, float);
void open_chest_and_give_item_to_player(
    KonquestPuiDefinition*, KonquestPuiDefinition*);
MslSoundHandle pan_vol_pitch_snd_req( int sound_id, float pan, float volume, float pitch);
void plyr_start_script_in_plyr_pdata_proc(FighterMirror* fighter, int proc_id, unsigned int function);
void plyr_weapon_grab(PlyrPdata* player, MkObj* item);
void pui_set_color(
    unsigned int, unsigned char, unsigned char, unsigned char, unsigned char);
void pz_fighter_check_to_toggle_obj_and_ani_flips(unsigned int expected_player);
void pz_fighter_force_reaction_in_ticks(int reaction, int ticks);
void pz_fighter_register_move(int move, int table, int parameter1, int parameter2, int character);
void random_snd_req_delay(int group, int delay);
void rd_set_impact_vector(float scale);
void run_camera_script(int script, int argument, int flags);
void slow_ani_x(float, float);
void snd_req_delay(int sound_id, int delay);
void spad_scale_vector(int, unsigned int, void *, float);
void spad_set_vector(int index, unsigned int source);
void spad_sub_vectors(int destination, int first, int second);


float bgnd_get_camera_y_angle(void);
float bgnd_get_camera_z_pos(void);
float frand(float max);
KonquestNpcData* get_active_npc_data(void);
int get_building_id_for_exterior(void);
int get_doors_for_exterior(void);
void* get_general_pebble_data(PebbleData* pebble_data);
float get_ir_cam_ang_x(int);
float get_ir_cam_ang_y(int);
float get_ir_cam_ang_z(int);
float get_ir_cam_pos_x(int);
float get_ir_cam_pos_y(int);
float get_ir_cam_pos_z(int);
int get_mode_of_play(void);
MkObj* get_pickup_object(void);
MkSobj* get_tile_sobj_by_id(int);
int is_big_boss(PlyrPdata* fighter);
int npc_get_collision_direction_in_script(void);
int npc_get_conversation_count(void);
int npc_get_flag_state(int);
MkObj* npc_get_obj(KonquestNpcData* data);
int npc_get_punch_count(void);
int npc_punch_reaction_check_data(void);
MslSoundHandle plyr_snd_req(int sound_offset);
int pz_finish_him_request(void);
MslSoundHandle random_foot(int group);
MslSoundHandle random_voice(int group);
int restart_effect(void);
float sfrand(float max);
int trial_invisible_callback(PlyrPdata* player);

int bgnd_create_pebbles_with_sobj(int, int, int, int);
float bgnd_get_anim_info(int arg, int arg1, float arg2);
MkProc* fire_spear_at_camera(PlyrPdata* player, unsigned int ticks);
MkObj* konquest_start_damashi(void*, float, float, float);
int ncs_create_pebble_monitor_proc(int, int, int, int);
PebbleData* ncs_create_pebbles_with_sobj(MkSobj* object, unsigned int count);
int npc_get_his_flag_state(KonquestNpcData* data, int flags);
MslSoundHandle pan_vol_pitch_random_hit(int, float, float, float);
MslSoundHandle pan_vol_snd_req(int sound_id, float pan, float volume);
float spad_get_pos(int, unsigned int);


void ani_to_fall_to_frame(
    float landing_frame, int sound_id, float target_frame);
AnimPdata* animate_obj(MkObj* object, AnimScript* script, const int* bone_tags, MkFlippedBoneMap* flipped_bones, void* ground_collisions, float playback_rate, int active);
void bgnd_create_danger_zone(int, unsigned int, unsigned int,
                             float, unsigned int);
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
void force_forward(float, int, float, int);
void mk_chess_launch_n_land_ani_with_xz(
    int animation_id, float launch_frame, float initial_speed,
    float landing_frame, float vertical_speed, float gravity, float blend,
    float start_x, float start_y, float target_x, float target_y, int turn,
    unsigned int sound);
void obj_grnd_bounce(MkObj* object, const Vec* velocity, float gravity,
                     float ground_offset, int bounces, float restitution);
void obj_match_obj_pos(
    MkObj* source, MkObj* destination, float blend, int snap);
void parse_args(const char*, ...);
typedef struct NcsLimbOwner NcsLimbOwner;
MkProc* plyr_spawn_his_anim_limb(PlyrInfo* player, int limb, int include_children, AniData* animation_script, int transition, MkProcEntryFn entry, float animation_step);
float pz_fighter_inline_force_away_with_ani(
    float, unsigned int, float, unsigned int);
int single_frame_collision_check(
    int region, float radius, float height, int reaction, int strength,
    float reaction_rate);
void start_gore2_pebbles(
    unsigned int object_id, int bone, MkObj* source,
    FighterMirror* decal_owner, Vec* velocity,
    const Vec* rotation, Vec* scale,
    Vec* position_offset, float vertical_acceleration,
    float bounce_scale, int bounce_count);



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
void pz_fighter_startup_attack(AniData* animation, float frame1, float frame2, float frame3, float frame4, unsigned int field0C, unsigned int field10, int field14, unsigned int reaction_mode, float desired_distance);
float two_player_animation_match_attacker(
    AniData* animation, float attacker_step);
void* get_function_attributes_table(struct ScriptDistanceSlot*, int);

static void _script_hang(void) {
}

static void _stop_usec_timer(void) {
    stop_usec_timer(0);
    printf("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui");
}

static void _start_usec_timer(void) {
    start_usec_timer(0);
}

static void _obj_setup_for_animation(void) {
    obj_setup_for_animation(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[3].pointer);
}

static void _npc_set_anim_proc(void) {
    int function_index;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x12,
               &function_index);
    npc_set_anim_proc(
        (MkProcEntryFn)script_callable_function_table[function_index - 1]);
}

static void _animate_obj(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    MkObj* object = args->slots[0].pointer;
    AnimScript* script = get_animation(args->slots[1].i);

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = animate_obj(
        object, script, args->slots[2].pointer,
        args->slots[3].pointer, args->slots[4].pointer,
        args->slots[5].f, args->slots[6].i);
}

static void _start_gusher(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = (int)start_gusher(
        heart_beat, (FighterMirror*)args->slots[0].i,
        (MkObj*)args->slots[1].i, args->slots[2].i,
        (Vec*)args->slots[3].i,
        (Vec*)args->slots[4].i);
}

static void _plyr_spawn_his_anim_limb(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    AniData* animation = get_animation(args->slots[3].i);

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_spawn_his_anim_limb(
            args->slots[0].pointer, args->slots[1].i, args->slots[2].i,
            animation, args->slots[4].i,
            (MkProcEntryFn)(script_callable_function_table + args->slots[5].i)[-1],
            args->slots[6].f);
}

static void _xfer_proc(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;

    xfer_proc(args->slots[0].pointer,
              (MkProcEntryFn)(script_callable_function_table + args->slots[1].i)[-1]);
}

static void _transition_to_anim_script_frame(void) {
    struct ScriptTransitionFrameArgs* args = (struct ScriptTransitionFrameArgs*)current_args;
    AnimPdata* animation = args->animation;
    AnimScript* script = get_animation(args->animation_id);

    args = (struct ScriptTransitionFrameArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        transition_to_anim_script_frame(
            animation, script, args->flags, args->transition_frames,
            args->frame);
}

static void _two_player_animation_blend(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    AniData* animation = get_animation(args->slots[0].i);

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = two_player_animation_blend(
        animation, args->slots[1].f, args->slots[2].f,
        args->slots[3].i, args->slots[4].i);
}

static void _set_anim_script(void) {
    struct ScriptRawArgs* args;
    AnimPdata* animation;
    AnimScript* script;

    args = (struct ScriptRawArgs*)current_args;
    animation = args->slots[0].pointer;
    script = get_animation(args->slots[1].i);
    set_anim_script(animation, script,
        ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _anim_pdata_for_proc(void) {
    void* pdata;

    pdata = 0;
    if (((struct ScriptRawArgs*)current_args)->slots[0].pointer != 0) {
        pdata = pdata_of_proc(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
    }
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = pdata;
}

static void _set_bonematcher_flag(void) {
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

static void _get_bonematcher_flag(void) {
    struct ScriptFlagArgs* args;
    int shift;

    args = (struct ScriptFlagArgs*)current_args;
    shift = 31;
    shift -= args->bit;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (args->object->flags & (1U << shift)) != 0;
}

static void _update_mkobj(void) {
    update_mkobj(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _get_plyr_pdata_flag(void) {
    struct ScriptRawArgs* args;
    PlyrPdata* pdata;
    int shift;

    args = (struct ScriptRawArgs*)current_args;
    shift = 31;
    shift -= args->slots[1].i;
    pdata = args->slots[0].pointer;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (pdata->state_flags.raw_word & (1U << shift)) != 0;
}

static void _set_obj_flag(void) {
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

static void _get_obj_flag(void) {
    struct ScriptFlagArgs* args;
    int shift;

    args = (struct ScriptFlagArgs*)current_args;
    shift = 31;
    shift -= args->bit;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (args->object->flags & (1U << shift)) != 0;
}

static void _get_limb_obj(void) {
    struct GetLimbObjArgs* args = (struct GetLimbObjArgs*)current_args;
    unsigned int bone_index = args->bone_index;
    LimbRuntime* runtime = args->runtime;
    MkHdr* object = MK_LIVE(runtime->bone_procs[bone_index].hdr,
                            runtime->bone_procs[bone_index].instance);

    ((struct ScriptPointerResult*)active_cmdscript)->value = object;
}

static inline MkHdr* script_live_item_object(const MkHdrLatch* ref) {
    MkHdr* object = ref->hdr;

    if (object != 0) {
        return object->instance == ref->instance ? object : 0;
    }
    return 0;
}

static void _destroy_item_obj(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    MkHdrLatch* ref = args->slots[0].pointer;

    if (script_live_item_object(ref) != 0) {
        MkHdr* object = ref->hdr;
        if (object->instance != 0) {
            MkHdrVtable* vtable = object->typed_vtbl;
            vtable->destroy(object);
        }
        ref->hdr = 0;
        ref->instance = 0;
    }
}

static void _init_item_obj(void) {
    int* item;

    item = *(int**)(current_args + 4);
    item[0] = 0;
    item[1] = 0;
}

static void _ck_item_obj(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    MkHdrLatch* ref = args->slots[0].pointer;
    MkHdr* object = MK_LIVE(ref->hdr, ref->instance);

    ((struct ScriptPointerResult*)active_cmdscript)->value = object;
}

static void _insert_item_obj(void) {
    int* item;
    int* object;

    object = *(int**)(current_args + 4);
    item = *(int**)(current_args + 8);
    item[0] = (int)object;
    item[1] = object[1];
}

static void _get_new_mkobj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_mkobj_frame(((struct ScriptRawArgs*)current_args)->slots[0].i, 0);
}

static void _nis_init(void) {
    struct ScriptActiveState* script;
    int arg0;
    int arg1;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x14,
               &arg0, &arg1);
    script = (struct ScriptActiveState*)active_cmdscript;
    nis_init(script->state, arg0, arg1);
}

static void _clear_fight_hud(void) {
    setup_screen_for_fatality();
}

static void _drone_blend_to_ani(void) {
    int sp10;
    int spC;
    float sp8;
    AnimScript* animation;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x17, &sp10, &spC, &sp8);
    animation = get_animation(sp10);
    drone_blend_to_ani(animation, spC, sp8);
}

static void _camera_set_target(void) {
    Vec angle;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &angle.x, &angle.y, &angle.z);
    set_camera_target_angle(&angle);
}

static void _camera_set_destination(void) {
    Vec position;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &position.x, &position.y, &position.z);
    set_camera_destination(&position);
}

static void _trial_add_required_attack_nohit(void) {
    struct TrialRequiredAttackArgs* args;

    args = (struct TrialRequiredAttackArgs*)current_args;
    trial_add_required_attack(args->attack,
                              args->count, 0);
}

static void _trial_add_required_attack(void) {
    struct TrialRequiredAttackArgs* args;

    args = (struct TrialRequiredAttackArgs*)current_args;
    trial_add_required_attack(args->attack,
                              args->count, 2);
}

static void _pz_fighter_should_continue_move(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = (g_pz_fighters_engine.fighter_move.policy_flags >> 2) & 1;
}

static void _set_background_color(void) {
    struct BackgroundColorArgs* args;

    args = (struct BackgroundColorArgs*)current_args;
    set_background_color((unsigned char)args->red,
                         (unsigned char)args->green,
                         (unsigned char)args->blue,
                         (unsigned char)args->alpha);
}

static void _camera_init_animation(void) {
    int animation_id;
    AniData* animation;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1F,
               &animation_id);
    animation = get_animation(animation_id);
    camera_init_animation(animation, p_animated_intro_done);
}

static void _camera_set_angle(void) {
    Vec angle;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &angle.x, &angle.y, &angle.z);
    set_camera_angle(&angle);
}

static void _camera_set_position(void) {
    Vec position;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1B,
               &position.x, &position.y, &position.z);
    set_camera_position(&position);
}

static void _set_move_pz_attributes_to(void) {
    int sp8;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x21, &sp8);
    ((struct ScriptCommandView*)active_cmdscript)->move_attributes = sp8;
}

static void _get_current_player_number(void) {
    if ((plyr_pdata)->plyr_num == 1) {
        ((struct ScriptRawResult*)active_cmdscript)->value.i = 1;
    } else {
        ((struct ScriptRawResult*)active_cmdscript)->value.i = 0;
    }
}

static void _randu0(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptIntResult* script;
    unsigned short value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = randu0(args->value);
    script = (struct ScriptIntResult*)active_cmdscript;
    script->value = value;
}

static void _my_attack_hit(void) {
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

static void _pz_fighter_startup_attack(void) {
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
    ACTIVE_DISTANCE_SCRIPT->function_name =
        &((char*)ACTIVE_DISTANCE_SCRIPT->slot->string_relocation)
            [ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index].name_offset - 1];
}

static void _pz_fighter_distance_check_wo_super_check(void) {
    int result = pz_fighter_distance_check_wo_super_check();

    if (result == 1) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    } else if (result == 2) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[1].u);
    }
}

static void _pz_fighter_distance_check(void) {
    int result = pz_fighter_distance_check();

    if (result == 1) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    } else if (result == 2) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[1].u);
    }
}

static void _pz_fighter_set_y_constrain(void) {
    struct PzConstrainArgs* args;

    args = (struct PzConstrainArgs*)current_args;
    pz_fighter_set_y_constrain((MkObj*)plyr_obj, args->mode,
                               args->value);
}

static void _pz_fighter_attack(void) {
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

static void _exit_attack_with(void) {
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

static void _attack_opponent_with(void) {
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

static void _drone_combo(void) {
}

static void _check_his_state(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    unsigned int function_index;
    function_index = args->slots[1].u;

    if (his_pdata->state == args->slots[0].i) {
        return;
    }
    script_distance_branch(function_index);
}

static void _drone_xfer_him(void) {
    reaction_xfer_him_nohit(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _drone_super_combo(void) {
}

static void _xfer_camera(void) {
    int function_index;
    int reset_projection;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x23,
               &function_index, &reset_projection);
    xfer_camera(
        (MkProcEntryFn)script_callable_function_table[function_index - 1],
        reset_projection);
}

static void _camera_set_movement_focus(void) {
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

static void _camera_setup_for_custom_orbit_to_relative_point(void) {
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

static void _camera_set_position_relative_to_npc(void) {
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

static void _camera_setup_for_tracking_npc(void) {
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

static void _camera_setup_for_orbiting_npc(void) {
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

static void _camera_set_lookat_focus_obj(void) {
    camera_set_lookat_focus(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _camera_set_lookat_focus(void) {
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

static void _camera_set_lookat_offset_obj_rel(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_lookat_offset_obj_rel(&offset, current_args);
}

static void _camera_set_lookat_offset(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_lookat_offset(&offset);
}

static void _camera_set_movement_offset_obj_rel(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_movement_offset_obj_rel(&offset, current_args);
}

static void _camera_set_movement_offset(void) {
    struct ScriptRawArgs* args;
    Vec offset;

    args = (struct ScriptRawArgs*)current_args;
    offset.x = args->slots[0].f;
    offset.y = args->slots[1].f;
    offset.z = args->slots[2].f;
    camera_set_movement_offset(&offset);
}

static void _if_switching_to(void) {
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

static void _branch_next_style(void) {
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

static void _true_branch_next_sidekick_style(void) {
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

static void _true_branch_next_style(void) {
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

static void _was_i_hit_x_times(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptCommandView* script;
    int result;

    args = (struct ScriptSingleIntArgs*)current_args;
    result = was_i_hit_x_times(args->value);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->result = result;
}

static void _ani_col_abort(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = ani_col_abort(
        args->slots[0].f, args->slots[1].i, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].f,
        args->slots[6].i);
}

static void _was_button_and_direction(void) {
    struct ScriptTwoIntArgs* args;
    struct ScriptCommandView* script;
    int result;

    args = (struct ScriptTwoIntArgs*)current_args;
    result = was_button_and_direction(args->first,
                                      args->second);
    script = (struct ScriptCommandView*)active_cmdscript;
    script->result = result;
}

static void _hit_branch(void) {
    if (plyr_pdata->collision_result == 1) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

static void _block_branch(void) {
    if (plyr_pdata->collision_result == 2) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

static void _miss_branch(void) {
    if (plyr_pdata->collision_result == 0) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

static void _disable_grounding(void) {
    plyr_obj->flags.launched = 0;
}

static void _set_player_hiframe(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_anim_hiframe(args->slots[0].f);
}

static void _set_player_movement_weight(void) {
    ((AnimPdata*)plyr_anim_pdata)->weight =
        ((struct ScriptRawArgs*)current_args)->slots[0].f;
}

static void _set_player_step(void) {
    ((AnimPdata*)plyr_anim_pdata)->step =
        ((struct ScriptRawArgs*)current_args)->slots[0].f;
}

static void _newani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    args = (struct ScriptRawArgs*)current_args;
    newani_to_frame_x(
        ((struct ScriptCommandView*)active_cmdscript)->animation,
        args->slots[1].f, args->slots[2].f, args->slots[3].f,
        args->slots[4].f, args->slots[5].i);
}

static void _blend_to_ani_inout(void) {
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

static void _glitch_to_ani_frame(void) {
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

static void _blend_to_ani_frame(void) {
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

static void _glitch_him_to_ani(void) {
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

static void _glitch_to_ani(void) {
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

static void _enable_grounding(void) {
    MkObj* player;
    MkHdr* object;

    plyr_obj->flags.launched = 1;
    player = plyr_obj;
    object = player != 0 ? as_mkhdr(&player->hdr) : 0;
    update_bone_hierarchy(object);
    player = plyr_obj;
    object = player != 0 ? as_mkhdr(&player->hdr) : 0;
    ground_me(object);
}

static void _was_button_pressed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        was_button_pressed(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _am_i_airborn(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_airborn();
}

static void _am_i_blocking(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_blocking();
}

static void _is_his_chest_to_screen(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_his_chest_to_screen();
}

static void _is_my_chest_to_screen(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_my_chest_to_screen();
}

static void _am_i_on_the_left2(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        am_i_on_the_left2(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                          ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _am_i_on_the_left(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_on_the_left();
}

static void _is_he_flipped(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_he_flipped();
}

static void _am_i_flipped_or_turned(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_flipped_or_turned();
}

static void _am_i_flipped(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_flipped();
}

static void _lower_mines_ani_to_point(void) {
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

static void _launch_n_land_ani(void) {
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

static void _attack_to_frame_x(void) {
    struct ScriptRawArgs* args;
    ((struct ScriptCommandView*)active_cmdscript)->animation =
        get_animation(((struct ScriptRawArgs*)current_args)->slots[0].i);
    args = (struct ScriptRawArgs*)current_args;
    attack_to_frame_x(((struct ScriptCommandView*)active_cmdscript)->animation,
        args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f,
        args->slots[5].u, args->slots[6].u, args->slots[7].i);
}

static void _two_player_animation_match_attacker(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    two_player_animation_match_attacker(((struct ScriptCommandView*)active_cmdscript)->animation, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _two_player_animation_flip(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    two_player_animation_flip(((struct ScriptCommandView*)active_cmdscript)->animation, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _two_player_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptCommandView*)active_cmdscript)->animation = get_animation(args->slots[0].i);
    two_player_animation(((struct ScriptCommandView*)active_cmdscript)->animation, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _is_fast_getup(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_fast_getup();
}

static void _disable_impale_check(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = disable_impale_check();
}

static void _blend_to_ani(void) {
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

static void _exit_react(void) {
    CURRENT_PLAYER_PDATA->script_exit_value_int = CURRENT_EXIT_ARGS->exit_value;
    CURRENT_PLAYER_PDATA->script_exit_args[0] = CURRENT_EXIT_ARGS->exit_arg0;
    CURRENT_PLAYER_PDATA->input_unlock_tick = CURRENT_EXIT_ARGS->input_unlock_tick;
    CURRENT_PLAYER_PDATA->blocking_disable_tick_2 = CURRENT_EXIT_ARGS->blocking_tick;
    CURRENT_PLAYER_PDATA->script_exit_args[1] = CURRENT_EXIT_ARGS->exit_arg1;
    CURRENT_PLAYER_PDATA->script_exit_arg_2 = CURRENT_EXIT_ARGS->exit_arg2;
    ACTIVE_SCRIPT_EXIT->exit = j_exit_react;
    ACTIVE_SCRIPT_EXIT->state = 2;
}

static void _exit_6(void) {
    CURRENT_PLAYER_PDATA->script_exit_value_int = CURRENT_EXIT_ARGS->exit_value;
    CURRENT_PLAYER_PDATA->script_exit_args[0] = CURRENT_EXIT_ARGS->exit_arg0;
    CURRENT_PLAYER_PDATA->input_unlock_tick = CURRENT_EXIT_ARGS->input_unlock_tick;
    CURRENT_PLAYER_PDATA->blocking_disable_tick_1 = CURRENT_EXIT_ARGS->blocking_tick;
    CURRENT_PLAYER_PDATA->script_exit_args[1] = CURRENT_EXIT_ARGS->exit_arg1;
    CURRENT_PLAYER_PDATA->script_exit_arg_2 = CURRENT_EXIT_ARGS->exit_arg2;
    ACTIVE_SCRIPT_EXIT->exit = j_exit_6;
    ACTIVE_SCRIPT_EXIT->state = 2;
}

static void _exit_float_int(void) {
    ACTIVE_SCRIPT_EXIT->exit =
        exit_table[((struct ExitFloatIntArgs*)current_args)->exit_index];
    CURRENT_PLAYER_PDATA->summon_position_x =
        ((struct ExitFloatIntArgs*)current_args)->float_value;
    CURRENT_PLAYER_PDATA->script_exit_value_int =
        ((struct ExitFloatIntArgs*)current_args)->int_value;
    ACTIVE_SCRIPT_EXIT->state = 2;
}

static void _script_exit(void) {
    ACTIVE_SCRIPT_EXIT->exit =
        exit_table[((struct ScriptRawArgs*)current_args)->slots[0].i];
    ACTIVE_SCRIPT_EXIT->state = 2;
}

static void _script_return(void) {
    ((struct ScriptCommandView*)active_cmdscript)->exit = 0;
    ((struct ScriptCommandView*)active_cmdscript)->state = 2;
}

static void _print_v(void) {
}

static void _print_f(void) {
}

static void _print_i(void) {
}

static void _print_s(void) {
}

static void _gosub(void) {
    unsigned int function_index;
    const struct ScriptDistanceFuncDef* function;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x12,
               &function_index);
    push_script_stack_frame(1);
    ACTIVE_DISTANCE_SCRIPT->program_counter =
        ACTIVE_DISTANCE_SCRIPT->slot->bytecode +
        ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1].code_offset;
    function = &ACTIVE_DISTANCE_SCRIPT->slot->functions[function_index - 1];
    ACTIVE_DISTANCE_SCRIPT->function_name =
        &((char*)ACTIVE_DISTANCE_SCRIPT->slot->string_relocation)[function->name_offset - 1];
}

static void _branch(void) {
    script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
}

static void _true_branch(void) {
    if (((struct ScriptCommandView*)active_cmdscript)->result != 0) {
        script_distance_branch(((struct ScriptRawArgs*)current_args)->slots[0].u);
    }
}

static void _true_xfer_him(void) {
    int condition = ((struct ScriptRawResult*)active_cmdscript)->value.i;
    void* reaction = ((struct ScriptRawArgs*)current_args)->slots[0].pointer;

    if (condition != 0) {
        reaction_xfer_him_nohit(reaction);
    }
}

static void _script_sleep(void) {
    int ticks;

    parse_args("Elapsed time: %d\n\0u\0uu\0iuf\0fff\0i\0v\0ui" + 0x1F,
               &ticks);
    _mkproc_sleep_ticks = (float)ticks * inverse_game_speed;
    aproc->vtbl->sleep();
}

static float j_sleep_forever(void) {
    for (;;) {
        _mkproc_sleep_ticks = 60.0f;
        aproc->vtbl->sleep();
    }
}

static float j_call_player_script_function(void) {
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

static void _tag_team_activate_player(void) {
    tag_team_activate_player(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                             ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _load_and_set_refl_on_weapon(void) {
    load_and_set_refl_on_weapon();
}

static void _advance_active_moveset(void) {
    advance_active_moveset(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _get_active_moveset_from_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        get_active_moveset_from_pdata(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _fx_transfer(void) {
    fx_transfer(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _bgnd_force_specularity_off_for_material(void) {
    bgnd_force_specularity_off_for_material(
        ((struct ScriptRawArgs*)current_args)->slots[0].u,
        ((struct ScriptRawArgs*)current_args)->slots[1].u);
}

static void _bgnd_sobj_set_texture_kl_values(void) {
    bgnd_sobj_set_texture_kl_values(((struct ScriptRawArgs*)current_args)->slots[0].u,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[2].i,
                                    ((struct ScriptRawArgs*)current_args)->slots[3].f,
                                    ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _debug_print_message(void) {
    get_script_string_arg(1);
    debug_print_message();
}

static void _fxsys_set(void) {
    fxsys_set(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _fxsys_set_v3(void) {
    fxsys_set_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
                 ((struct ScriptRawArgs*)current_args)->slots[1].f,
                 ((struct ScriptRawArgs*)current_args)->slots[2].f,
                 ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _fx_bind_render_to_sobj(void) {
    fx_bind_render_to_sobj(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _fx_bind_emitter_to_obj_bone(void) {
    fx_bind_emitter_to_obj_bone(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _fx_bind_render_to_obj_bone(void) {
    fx_bind_render_to_obj_bone(((struct ScriptRawArgs*)current_args)->slots[0].i,
                               ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                               ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _fx_disable_ztest(void) {
    fx_disable_ztest(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _fx_set_render_priority(void) {
    fx_set_render_priority(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _fx_hide(void) {
    fx_hide(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _fx_get_v3(void) {
    fx_get_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
              ((struct ScriptRawArgs*)current_args)->slots[1].i,
              ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _fx_set(void) {
    fx_set(((struct ScriptRawArgs*)current_args)->slots[0].i,
           ((struct ScriptRawArgs*)current_args)->slots[1].i,
           ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _fx_set_param_v3(void) {
    fx_set_param_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
                    ((struct ScriptRawArgs*)current_args)->slots[1].i,
                    ((struct ScriptRawArgs*)current_args)->slots[2].f,
                    ((struct ScriptRawArgs*)current_args)->slots[3].f,
                    ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

static void _enable_profiling(void) {
    enable_profiling(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _kill_on_y_less_than_field(void) {
    kill_on_y_less_than_field(((struct ScriptRawArgs*)current_args)->slots[0].i,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _change_on_y_less_than_field(void) {
    change_on_y_less_than_field(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _change_on_y_less(void) {
    change_on_y_less(((struct ScriptRawArgs*)current_args)->slots[0].i,
                     ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _change_on_less(void) {
    change_on_less(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _change_on_greater(void) {
    change_on_greater(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _kill_roundrobin(void) {
    kill_roundrobin(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _kill_percent(void) {
    kill_percent(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _kill_on_greater(void) {
    kill_on_greater(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _udpate_roundrobin(void) {
    udpate_roundrobin(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _update_assign(void) {
    update_assign(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _update_attract(void) {
    update_attract(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].i,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _update_bounce(void) {
    update_bounce(((struct ScriptRawArgs*)current_args)->slots[0].i,
                  ((struct ScriptRawArgs*)current_args)->slots[1].i,
                  ((struct ScriptRawArgs*)current_args)->slots[2].i,
                  ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _update_texanim_hold(void) {
    update_texanim_hold(((struct ScriptRawArgs*)current_args)->slots[0].i,
                        ((struct ScriptRawArgs*)current_args)->slots[1].i,
                        ((struct ScriptRawArgs*)current_args)->slots[2].f,
                        ((struct ScriptRawArgs*)current_args)->slots[3].i,
                        ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _update_texanim(void) {
    update_texanim(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].i,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f,
                   ((struct ScriptRawArgs*)current_args)->slots[3].i,
                   ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _update_lerp_color(void) {
    update_lerp_color(((struct ScriptRawArgs*)current_args)->slots[0].i,
        ((struct ScriptRawArgs*)current_args)->slots[1].i,
        ((struct ScriptRawArgs*)current_args)->slots[2].f,
        ((struct ScriptRawArgs*)current_args)->slots[3].i,
        ((struct ScriptRawArgs*)current_args)->slots[4].i,
        ((struct ScriptRawArgs*)current_args)->slots[5].pointer);
}

static void _update_fade_alpha2(void) {
    update_fade_alpha2(((struct ScriptRawArgs*)current_args)->slots[0].i,
        ((struct ScriptRawArgs*)current_args)->slots[1].i,
        ((struct ScriptRawArgs*)current_args)->slots[2].f,
        ((struct ScriptRawArgs*)current_args)->slots[3].f,
        ((struct ScriptRawArgs*)current_args)->slots[4].i,
        ((struct ScriptRawArgs*)current_args)->slots[5].i);
}

static void _update_fade_alpha(void) {
    update_fade_alpha(((struct ScriptRawArgs*)current_args)->slots[0].i,
                      ((struct ScriptRawArgs*)current_args)->slots[1].i,
                      ((struct ScriptRawArgs*)current_args)->slots[2].f,
                      ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _update_mul_scalar(void) {
    update_mul_scalar(((struct ScriptRawArgs*)current_args)->slots[0].i,
                      ((struct ScriptRawArgs*)current_args)->slots[1].f,
                      ((struct ScriptRawArgs*)current_args)->slots[2].f,
                      ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _update_wrapbox(void) {
    update_wrapbox(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].f,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f,
                   ((struct ScriptRawArgs*)current_args)->slots[3].f,
                   ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

static void _update_add_constant(void) {
    update_add_constant(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _update_add_constant_v3(void) {
    update_add_constant_v3(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f,
                           ((struct ScriptRawArgs*)current_args)->slots[2].f,
                           ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _update_copy(void) {
    update_copy(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _update_add(void) {
    update_add(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _initial_multiply_float(void) {
    initial_multiply_float(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f,
                           ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _initial_set_float(void) {
    initial_set_float(((struct ScriptRawArgs*)current_args)->slots[0].i,
                      ((struct ScriptRawArgs*)current_args)->slots[1].f,
                      ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _initial_add_v3(void) {
    initial_add_v3(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _initial_divert(void) {
    initial_divert(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].f,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _initial_reflect(void) {
    initial_reflect(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _set_cycle_emission(void) {
    set_cycle_emission(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _set_cycle_length(void) {
    set_cycle_length(((struct ScriptRawArgs*)current_args)->slots[0].f,
                     ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _fx_reset_emit(void) {
    fx_reset_emit(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _fx_pause_emit(void) {
    fx_pause_emit(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _fx_resume_emit(void) {
    fx_resume_emit(((struct ScriptRawArgs*)current_args)->slots[0].u);
}

static void _fx_next_emitter(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fx_next_emitter(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _fx_reset(void) {
    fx_reset(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _fx_restart_emit(void) {
    fx_restart_emit(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _reset_effect(void) {
    get_script_string_arg(1);
    reset_effect();
}

static void _resume_effect(void) {
    resume_effect(get_script_string_arg(1));
}

static void _spawn_color(void) {
    spawn_color(((struct ScriptRawArgs*)current_args)->slots[0].i,
                ((struct ScriptRawArgs*)current_args)->slots[1].i,
                ((struct ScriptRawArgs*)current_args)->slots[2].i,
                ((struct ScriptRawArgs*)current_args)->slots[3].i,
                ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _spawn_random_size(void) {
    spawn_random_size(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _set_growth_coefficient(void) {
    set_growth_coefficient(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _set_drag_coefficient(void) {
    set_drag_coefficient(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _set_rotation(void) {
    set_rotation(((struct ScriptRawArgs*)current_args)->slots[0].f, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _kill_at_plane(void) {
    kill_at_plane(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _emit_constant_rate(void) {
    emit_constant_rate();
}

static void _emit_roundrobin_mechanism(void) {
    emit_roundrobin_mechanism(((struct ScriptRawArgs*)current_args)->slots[0].i,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _emit_value_i(void) {
    emit_value_i(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _emit_value(void) {
    emit_value(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _emit_from_pos_clamp_y(void) {
    emit_from_pos_clamp_y(((struct ScriptRawArgs*)current_args)->slots[0].i,
                          ((struct ScriptRawArgs*)current_args)->slots[1].i,
                          ((struct ScriptRawArgs*)current_args)->slots[2].f,
                          ((struct ScriptRawArgs*)current_args)->slots[3].f,
                          ((struct ScriptRawArgs*)current_args)->slots[4].f,
                          ((struct ScriptRawArgs*)current_args)->slots[5].f,
                          ((struct ScriptRawArgs*)current_args)->slots[6].f,
                          ((struct ScriptRawArgs*)current_args)->slots[7].f);
}

static void _emit_from_pos(void) {
    emit_from_pos(((struct ScriptRawArgs*)current_args)->slots[0].i,
                  ((struct ScriptRawArgs*)current_args)->slots[1].i,
                  ((struct ScriptRawArgs*)current_args)->slots[2].f,
                  ((struct ScriptRawArgs*)current_args)->slots[3].f,
                  ((struct ScriptRawArgs*)current_args)->slots[4].f,
                  ((struct ScriptRawArgs*)current_args)->slots[5].f,
                  ((struct ScriptRawArgs*)current_args)->slots[6].f);
}

static void _emit_spherical_section(void) {
    emit_spherical_section(((struct ScriptRawArgs*)current_args)->slots[0].i,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f,
                           ((struct ScriptRawArgs*)current_args)->slots[2].f,
                           ((struct ScriptRawArgs*)current_args)->slots[3].f,
                           ((struct ScriptRawArgs*)current_args)->slots[4].f,
                           ((struct ScriptRawArgs*)current_args)->slots[5].f,
                           ((struct ScriptRawArgs*)current_args)->slots[6].f,
                           ((struct ScriptRawArgs*)current_args)->slots[7].f);
}

static void _emit_spherical_from_boundary(void) {
    emit_spherical_from_boundary(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                 ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _emit_spherical(void) {
    emit_spherical(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _texture_animation_with_vsize(void) {
    texture_animation_with_vsize(((struct ScriptRawArgs*)current_args)->slots[0].f,
                                 ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                 ((struct ScriptRawArgs*)current_args)->slots[2].i,
                                 ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _texture_animation(void) {
    texture_animation(((struct ScriptRawArgs*)current_args)->slots[0].f,
        ((struct ScriptRawArgs*)current_args)->slots[1].i,
        ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _emission_duration(void) {
    emission_duration(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _emit_cylindrical(void) {
    emit_cylindrical(((struct ScriptRawArgs*)current_args)->slots[0].i,
                     ((struct ScriptRawArgs*)current_args)->slots[1].f,
                     ((struct ScriptRawArgs*)current_args)->slots[2].f,
                     ((struct ScriptRawArgs*)current_args)->slots[3].f,
                     ((struct ScriptRawArgs*)current_args)->slots[4].f,
                     ((struct ScriptRawArgs*)current_args)->slots[5].f,
                     ((struct ScriptRawArgs*)current_args)->slots[6].f,
                     ((struct ScriptRawArgs*)current_args)->slots[7].f);
}

static void _emit_cartesian(void) {
    emit_cartesian(((struct ScriptRawArgs*)current_args)->slots[0].i,
                   ((struct ScriptRawArgs*)current_args)->slots[1].f,
                   ((struct ScriptRawArgs*)current_args)->slots[2].f,
                   ((struct ScriptRawArgs*)current_args)->slots[3].f,
                   ((struct ScriptRawArgs*)current_args)->slots[4].f,
                   ((struct ScriptRawArgs*)current_args)->slots[5].f,
                   ((struct ScriptRawArgs*)current_args)->slots[6].f);
}

static void _emit_disc2(void) {
    emit_disc2(((struct ScriptRawArgs*)current_args)->slots[0].i,
               ((struct ScriptRawArgs*)current_args)->slots[1].f,
               ((struct ScriptRawArgs*)current_args)->slots[2].f,
               ((struct ScriptRawArgs*)current_args)->slots[3].f,
               ((struct ScriptRawArgs*)current_args)->slots[4].f,
               ((struct ScriptRawArgs*)current_args)->slots[5].f);
}

static void _emit_disc(void) {
    emit_disc(((struct ScriptRawArgs*)current_args)->slots[0].i,
              ((struct ScriptRawArgs*)current_args)->slots[1].f,
              ((struct ScriptRawArgs*)current_args)->slots[2].f,
              ((struct ScriptRawArgs*)current_args)->slots[3].f,
              ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

static void _emit_cuboid(void) {
    emit_cuboid(((struct ScriptRawArgs*)current_args)->slots[0].i,
                ((struct ScriptRawArgs*)current_args)->slots[1].f,
                ((struct ScriptRawArgs*)current_args)->slots[2].f,
                ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _emit_from_point(void) {
    emit_from_point(((struct ScriptRawArgs*)current_args)->slots[0].f,
                    ((struct ScriptRawArgs*)current_args)->slots[1].f,
                    ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _emit_uv(void) {
    emit_uv(((struct ScriptRawArgs*)current_args)->slots[0].i,
            ((struct ScriptRawArgs*)current_args)->slots[1].f,
            ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _emit_color(void) {
    emit_color(((struct ScriptRawArgs*)current_args)->slots[0].i,
               ((struct ScriptRawArgs*)current_args)->slots[1].i,
               ((struct ScriptRawArgs*)current_args)->slots[2].i,
               ((struct ScriptRawArgs*)current_args)->slots[3].i,
               ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _emit_in_range(void) {
    emit_in_range(((struct ScriptRawArgs*)current_args)->slots[0].i,
                  ((struct ScriptRawArgs*)current_args)->slots[1].f,
                  ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _fx_by_owner(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fx_by_owner(get_script_string_arg(1), ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _fx2(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fx2(((struct ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2));
}

static void _fx(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = fx(get_script_string_arg(1));
}

static void _set_vertex_color(void) {
    set_vertex_color(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _set_light(void) {
    set_light(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _set_aspect_ratio(void) {
    set_aspect_ratio(((struct ScriptRawArgs*)current_args)->slots[0].f, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _set_bounding_radius(void) {
    set_bounding_radius(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _create_y_mirror_effect(void) {
    create_y_mirror_effect(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _z_bias(void) {
    z_bias(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _particle_size(void) {
    particle_size(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _face_y(void) {
    face_y();
}

static void _set_decal_plane(void) {
    set_decal_plane(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _create_multiemit_parametric_fx(void) {
    create_multiemit_parametric_fx(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                   get_script_string_arg(2),
                                   ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _create_parametric_fx(void) {
    create_parametric_fx(((struct ScriptRawArgs*)current_args)->slots[0].pointer, get_script_string_arg(2));
}

static void _create_multiemit_step_fx(void) {
    create_multiemit_step_fx((struct PfxStepEffectDescription*)((struct ScriptRawArgs*)current_args)->slots[0].i,
                             get_script_string_arg(2),
                             ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _create_step_fx(void) {
    create_step_fx((struct PfxStepEffectDescription*)((struct ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2));
}

static void _bind_to_bone(void) {
    bind_to_bone(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _create_step_effect(void) {
    create_step_effect(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _parametric_update(void) {
    parametric_update(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _dist_xz_to_xz(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        dist_xz_to_xz(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                      ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _v3_to_xz_ang(void) {
    v3_to_xz_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _v3_to_xy_ang(void) {
    v3_to_xy_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _length_v3(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        length_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _rotate_xz(void) {
    rotate_xz(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _gxMathArcCos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        gxMathArcCos(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _gxMathSin(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        gxMathSin(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _gxMathCos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        gxMathCos(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _uv_v3_to_v3_dist(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        uv_v3_to_v3_dist(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _xz_dot_xz(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        xz_dot_xz(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _YXZ_angles_to_quat(void) {
    YXZ_angles_to_quat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                       ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_get_bone_rot_quat(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_get_bone_rot_quat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _uv_from_angle_y(void) {
    uv_from_angle_y(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _xz_to_y_ang(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        xz_to_y_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _xz_unit_vector(void) {
    xz_unit_vector(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                   ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                   ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _v3_dot_v3(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        v3_dot_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _zero_v3(void) {
    zero_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _normalize_v3(void) {
    normalize_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _scale_v3(void) {
    scale_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _v3_add_v3_scaled(void) {
    v3_add_v3_scaled(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _v3_add_v3(void) {
    v3_add_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _v3_sub_v3(void) {
    v3_sub_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
              ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _v3_x_mat(void) {
    v3_x_mat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
             ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _mkobj_get_matrix_pos(void) {
    mkobj_get_matrix_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _mkobj_get_matrix_right(void) {
    mkobj_get_matrix_right(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                           ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _mkobj_get_matrix_at(void) {
    mkobj_get_matrix_at(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _mkobj_get_matrix(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        mkobj_get_matrix(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_in_spin_react(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        plyr_in_spin_react(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _force_calc_bone_world_mat(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        force_calc_bone_world_mat(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _obj_set_sobj_pos(void) {
    obj_set_sobj_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].i,
                     ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _get_bone_relative_pos(void) {
    get_bone_relative_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                          ((struct ScriptRawArgs*)current_args)->slots[1].i,
                          ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _get_bone_offset_world_pos(void) {
    get_bone_offset_world_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i,
                              ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
                              ((struct ScriptRawArgs*)current_args)->slots[3].pointer);
}

static void _get_bone_world_pos(void) {
    get_bone_world_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                       ((struct ScriptRawArgs*)current_args)->slots[1].i,
                       ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _bone_matcher_child_set_offset(void) {
    bone_matcher_child_set_offset(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _bone_matcher_parent_set_offset(void) {
    bone_matcher_parent_set_offset(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                   ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _start_bone_matcher(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        start_bone_matcher(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i,
            ((struct ScriptRawArgs*)current_args)->slots[2].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[3].i,
            ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

static void _obj_get_ang_vel(void) {
    obj_get_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_set_ang_vel(void) {
    obj_set_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_set_pos_vel(void) {
    obj_set_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_get_pos_vel(void) {
    obj_get_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _plyr_pdata_is_alt_costume(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_is_alt_costume(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_previous_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_get_previous_state(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_get_state(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_pchr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_pchr(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_sidekick_active(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_sidekick_active(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_pdata_get_plyr_num(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_cmo(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_cmo(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_sidekick_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_pdata_get_sidekick_obj(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_his_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_his_obj(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_plyr_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = plyr_pdata_get_plyr_obj(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_his_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_pdata_get_his_plyr_pdata(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_pdata_get_plyr_info(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_pdata_get_plyr_info(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_anim_get_frame(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_anim_get_frame(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _get_my_plyr_anim_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_my_plyr_anim_pdata();
}

static void _get_my_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_my_plyr_pdata();
}

static void _get_his_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_his_plyr_pdata();
}

static void _get_plyr_obj_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_plyr_obj_plyr_num(
        ((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _get_my_sidekick_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_my_sidekick_obj();
}

static void _get_my_plyr_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_my_plyr_obj();
}

static void _get_his_plyr_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_his_plyr_obj();
}

static void _get_plyr_info(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_plyr_info();
}

static void _obj_find_sobj_by_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_find_sobj_by_id(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _obj_set_light_flag(void) {
    obj_set_light_flag(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _sobj_get_ang(void) {
    sobj_get_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _sobj_get_ang_vel(void) {
    sobj_get_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _sobj_set_ang_vel(void) {
    sobj_set_ang_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _sobj_set_ang(void) {
    sobj_set_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _sobj_get_pos_vel(void) {
    sobj_get_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _sobj_set_pos_vel(void) {
    sobj_set_pos_vel(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                     ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _sobj_get_pos(void) {
    sobj_get_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _sobj_set_pos(void) {
    sobj_set_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_get_ang(void) {
    obj_get_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_set_ang(void) {
    obj_set_ang(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_get_pos(void) {
    obj_get_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_set_pos(void) {
    obj_set_pos(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _obj_set_ground_colls_y(void) {
    obj_set_ground_colls_y(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                           ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _obj_set_ground_colls(void) {
    obj_set_ground_colls(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                         ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _konquest_load_interior_art(void) {
    konquest_load_interior_art();
}

static void _konquest_start_nis_anims_load(void) {
    konquest_start_nis_anims_load(
        get_script_string_arg(1), get_script_string_arg(2));
}

static void _konquest_nis_anims_loaded(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = konquest_nis_anims_loaded();
}

static void _konquest_nis_end(void) {
    konquest_nis_end();
}

static void _konquest_nis_init(void) {
    konquest_nis_init(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _nis_wait_for_region_load(void) {
    nis_wait_for_region_load();
}

static void _wait_for_region_load(void) {
    wait_for_region_load();
}

static void _nis_register_participant(void) {
    nis_register_participant(((struct ScriptRawArgs*)current_args)->slots[0].i,
                             ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _nis_set_wait_override(void) {
    nis_set_wait_override(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _nis_show_cancel_message(void) {
    nis_show_cancel_message();
}

static void _nis_scene_done(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = nis_scene_done();
}

static void _nis_end(void) {
    nis_end();
}

static void _nis_wait_for_event(void) {
    nis_wait_for_event(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _nis_signal_event(void) {
    nis_signal_event(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _get_fatality_state_ptr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_fatality_state_ptr();
}

static void _animpdata_ani_to_frame_x_with_flag_check(void) {
    animpdata_ani_to_frame_x_with_flag_check(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                             ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                             ((struct ScriptRawArgs*)current_args)->slots[2].i,
                                             ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

static void _mkscripts_set_anim_check_flag(void) {
    mkscripts_set_anim_check_flag(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _mkscripts_mkobj_insert_mkobj_cleanuplist(void) {
    mkscripts_mkobj_insert_mkobj_cleanuplist(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _mkscripts_destroy_gusher(void) {
    mkscripts_destroy_gusher(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _mkscripts_destroy_fk_bonematcher(void) {
    mkscripts_destroy_fk_bonematcher(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _mkscripts_destroy_bonematcher(void) {
    mkscripts_destroy_bonematcher(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _dkp_check_plyr_state_for_grab(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        dkp_check_plyr_state_for_grab(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _start_prison_grab_proc(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        start_prison_grab_proc(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                               ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                               ((struct ScriptRawArgs*)current_args)->slots[2].f,
                               ((struct ScriptRawArgs*)current_args)->slots[3].f);
}

static void _done_prison_grab_proc(void) { done_prison_grab_proc(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _kill_spear(void) { kill_spear(); }

static void _xfer_spearproc_to_retract(void) { xfer_spearproc_to_retract(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _destroy_spearproc_bonematcher(void) { destroy_spearproc_bonematcher(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _insert_mkobj_spearproc_parentobjitem(void) {
    insert_mkobj_spearproc_parentobjitem(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _get_spearobj_from_spearproc(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_spearobj_from_spearproc(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _fire_sc_spear(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fire_sc_spear(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                      ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                      ((struct ScriptRawArgs*)current_args)->slots[4].pointer, ((struct ScriptRawArgs*)current_args)->slots[5].i);
}

static void _subzero_start_ice_chunks(void) { subzero_start_ice_chunks(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _subzero_start_iceman(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = subzero_start_iceman();
}

static void _subzero_start_iceblock(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = subzero_start_iceblock();
}

static void _sindel_scream_react_sound_start(void) { sindel_scream_react_sound_start(); }

static void _sindel_sonic_sounds(void) { sindel_sonic_sounds(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

static void _sindel_sonic_waves(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = sindel_sonic_waves(((struct ScriptRawArgs*)current_args)->slots[0].f);
}

static void _start_raiden_lightning_scroll(void) {
    start_raiden_lightning_scroll(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[2].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[3].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _ft_raiden_summon_lightning_bolt(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        ft_raiden_summon_lightning_bolt(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                        ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                        get_script_string_arg(3));
}

static void _kill_raiden_summon_lightning_bolt(void) { kill_raiden_summon_lightning_bolt(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _fix_axe_angle(void) { fix_axe_angle(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _ft_mileena_start_veil_ripoff(void) { ft_mileena_start_veil_ripoff(); }

static void _fat_goro_fold_arms(void) {
    fat_goro_fold_arms(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                       ((struct ScriptRawArgs*)current_args)->slots[2].f, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

static void _fatality_boraicho_get_jug(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_get_jug(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _fatality_boraicho_light_fart_torch(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.u =
        fatality_boraicho_light_fart_torch(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _fatality_boraicho_get_torch(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_boraicho_get_torch(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _show_baraka_one_blade_only(void) {
    show_baraka_one_blade_only(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                               ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _fatality_ashrah_get_doll(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        fatality_ashrah_get_doll(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                 ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                 ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _fire_multi_emitter_pfx_via_tbl(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);
    fire_multi_emitter_pfx_via_tbl(
        name, args->slots[1].pointer,
        args->slots[2].pointer, args->slots[3].pointer);
}

static void _pfxhandle_spawn_at_bid_next_bind_render(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.u =
        pfxhandle_spawn_at_bid_next_bind_render(((struct ScriptRawArgs*)current_args)->slots[0].u,
                                                ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                                ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _pfxhandle_bgnd_spawn_at_sobj_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.u =
        pfxhandle_bgnd_spawn_at_sobj_id(get_script_string_arg(1),
                                        ((struct ScriptRawArgs*)current_args)->slots[1].u);
}

static void _pfxhandle_spawn_at_bid(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.u =
        pfxhandle_spawn_at_bid(name, args->slots[1].pointer, args->slots[2].i);
}

static void _pfxhandle_spawn_at_bid_next(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.u =
        pfxhandle_spawn_at_bid_next(((struct ScriptRawArgs*)current_args)->slots[0].u,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                    ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _pfx_spawn_at_bid(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);

    pfx_spawn_at_bid(name, args->slots[1].pointer, args->slots[2].i);
}

static void _limb_sever_throw_away(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        limb_sever_throw_away(((struct ScriptRawArgs*)current_args)->slots[0].i,
                              ((struct ScriptRawArgs*)current_args)->slots[1].i,
                              ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _auto_calc_limbobj_bone_world_pos(void) {
    auto_calc_limbobj_bone_world_pos(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _limb_sever_show_z_meat_chunks(void) {
    limb_sever_show_z_meat_chunks(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                  ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _limb_sever_show_z_meat_chunks_all(void) { limb_sever_show_z_meat_chunks_all(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _limb_sever_show_z_meat_chunks_all_plyr_num(void) { limb_sever_show_z_meat_chunks_all_plyr_num(((struct ScriptRawArgs*)current_args)->slots[0].i); }

static void _limb_sever_explode_apart_plyr_num(void) {
    limb_sever_explode_apart_plyr_num(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                      ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                      ((struct ScriptRawArgs*)current_args)->slots[2].f,
                                      ((struct ScriptRawArgs*)current_args)->slots[3].f,
                                      ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _reset_blood_decals(void) { reset_blood_decals(); }

static void _destroy_gore2_obj(void) {
    destroy_gore2_obj(((struct ScriptRawArgs*)current_args)->slots[0].i,
                     ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _attach_gore2_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        attach_gore2_obj(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                        ((struct ScriptRawArgs*)current_args)->slots[1].i,
                        ((struct ScriptRawArgs*)current_args)->slots[2].i,
                        ((struct ScriptRawArgs*)current_args)->slots[3].pointer,
                        ((struct ScriptRawArgs*)current_args)->slots[4].pointer);
}

static void _start_gore2_pebbles(void) {
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

static void _start_bodyslam_bodysplat(void) {
    start_bodyslam_bodysplat(((struct ScriptRawArgs*)current_args)->slots[0].f,
                             ((struct ScriptRawArgs*)current_args)->slots[1].f,
                             ((struct ScriptRawArgs*)current_args)->slots[2].f,
                             ((struct ScriptRawArgs*)current_args)->slots[3].f,
                             ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

static void _load_cloth_boned_model(void) {
    struct ScriptRawArgs* args;
    const char* name;

    args = (struct ScriptRawArgs*)current_args;
    name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        load_cloth_boned_model(
            name, args->slots[1].i, args->slots[2].i,
            args->slots[3].i, args->slots[4].pointer,
            args->slots[5].pointer, args->slots[6].i);
}

static void _fatality_explode_victim(void) {
    fatality_explode_victim(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                            ((struct ScriptRawArgs*)current_args)->slots[1].f,
                            ((struct ScriptRawArgs*)current_args)->slots[2].f);
}

static void _kill_gusher(void) { kill_gusher(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _start_sweat_particles_scripts(void) { start_sweat_particles_scripts(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

static void _start_blood_particles_scripts(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.u =
        start_blood_particles_scripts(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                      ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _start_sweat_particles(void) {
    start_sweat_particles(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                          ((struct ScriptRawArgs*)current_args)->slots[2].pointer, ((struct ScriptRawArgs*)current_args)->slots[3].pointer);
}

static void _start_blood_particles(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        start_blood_particles(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                              ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

static void _mks_spawn_blood_pool_at_bid(void) {
    mks_spawn_blood_pool_at_bid(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i);
}

static void _spawn_blood_pool_at_bid(void) {
    spawn_blood_pool_at_bid(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                            ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _spawn_bld_splat(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    char* name = get_script_string_arg(1);

    spawn_bld_splat(name, args->slots[1].pointer, args->slots[2].pointer);
}

static void _plyr_weapon2_release(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_weapon2_release(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _plyr_weapon_release(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        plyr_weapon_release(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _bone_matcher_reset_dest_mat_rot(void) { bone_matcher_reset_dest_mat_rot(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

static void _bone_matcher_set_ang_pos(void) {
    bone_matcher_set_ang_pos(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                             ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                             ((struct ScriptRawArgs*)current_args)->slots[4].i, ((struct ScriptRawArgs*)current_args)->slots[5].i);
}

static void _weapon_bm_ignore(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        weapon_bm_ignore(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _regrab_weapon(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        regrab_weapon(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                      ((struct ScriptRawArgs*)current_args)->slots[2].pointer, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                      ((struct ScriptRawArgs*)current_args)->slots[4].pointer, ((struct ScriptRawArgs*)current_args)->slots[5].pointer,
                      ((struct ScriptRawArgs*)current_args)->slots[6].pointer);
}

static void _weapon_reflection_show_hide(void) {
    weapon_reflection_show_hide(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _show_single_weapon(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        show_single_weapon(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _clone_my_weapon(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        clone_my_weapon(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _clone_weapon_to_secondary(void) {
    clone_weapon_to_secondary(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _advance_to_weapon_style(void) { advance_to_weapon_style(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _is_weapon_style(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_weapon_style(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _am_i_female(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_female(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _sobj_set_bounding_sphere_radius(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        sobj_set_bounding_sphere_radius(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                        ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _sobj_get_bounding_sphere_radius(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        sobj_get_bounding_sphere_radius(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _play_his_snd_req(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = play_his_snd_req(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _play_his_random_voice(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = play_his_random_voice(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _obj_unhide_material_by_id(void) {
    obj_unhide_material_by_id(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _obj_hide_material_by_id(void) {
    obj_hide_material_by_id(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _bm_force_fake_child_bid(void) {
    bm_force_fake_child_bid(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _fat_bgnd_char_setup_radius_check(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        fat_bgnd_char_setup_radius_check(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _set_victim_v3_units_away(void) {
    set_victim_v3_units_away(((struct ScriptRawArgs*)current_args)->slots[0].f,
                             ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _reset_fake_bone_matcher(void) {
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

static void _ft_fake_bone_matcher(void) {
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

static void _get_game_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_game_state();
}

static void _fatality_release_other_player(void) { fatality_release_other_player(); }

static void _get_level_fatality_done_flag_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        get_level_fatality_done_flag_state();
}

static void _set_level_fatality_done_flag_state(void) { set_level_fatality_done_flag_state(((struct ScriptRawArgs*)current_args)->slots[0].i); }

static void _limb_sever_destroy_existing_attach_proc(void) {
    limb_sever_destroy_existing_attach_proc(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _limb_sever_bone_attach(void) {
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

static void _limb_sever_pop_head_up(void) {
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

static void _mks_limb_sever(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        mks_limb_sever(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                       ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _limb_sever_find_existing_update_proc(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        limb_sever_find_existing_update_proc(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                              ((struct ScriptRawArgs*)current_args)->slots[1].i,
                                              ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _limb_sever_set_motion(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = limb_sever_set_motion(
        args->slots[0].pointer, args->slots[1].i, args->slots[2].pointer,
        args->slots[3].f, args->slots[4].pointer, args->slots[5].i,
        args->slots[6].f, args->slots[7].i, args->slots[8].f,
        args->slots[9].i, args->slots[10].i);
}

static void _limb_sever_update_slide_end_coeff(void) { limb_sever_update_slide_end_coeff(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _proc_of_anim_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = proc_of_anim_pdata(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _set_pdata_anim_step(void) { set_pdata_anim_step(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _puzzle_fighter_scale(void) { puzzle_fighter_scale(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _plyr_turn_on_shadowbox(void) { plyr_turn_on_shadowbox(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _plyr_turn_off_shadowbox(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_turn_off_shadowbox(args->slots[0].pointer);
}

static void _plyr_turn_on_mirrorguy(void) { plyr_turn_on_mirrorguy(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _plyr_turn_off_mirrorguy(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_turn_off_mirrorguy(args->slots[0].pointer);
}

static void _animpdata_ani_to_blend_frame(void) { animpdata_ani_to_blend_frame(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _animpdata_ani_to_end_at1(void) { animpdata_ani_to_end_at1(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _animpdata_ani_to_end(void) { animpdata_ani_to_end(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _animpdata_ani_x_more_frames(void) { animpdata_ani_x_more_frames(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _animpdata_ani_loop_more_frames(void) { animpdata_ani_loop_more_frames(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _animpdata_ani_to_frame_x(void) { animpdata_ani_to_frame_x(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _mks_animpdata_set_cur_frame(void) { mks_animpdata_set_cur_frame(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].f); }

static void _animpdata_ani_1_frame(void) { animpdata_ani_1_frame(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _check_to_register_miss(void) { check_to_register_miss(); }

static void _auto_ani_off(void) { auto_ani_off(); }

static void _ncs_bgnd_preload_named_model(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        ncs_bgnd_preload_named_model(
            get_script_string_arg(1), get_script_string_arg(2),
            args->slots[2].i, args->slots[3].i,
            args->slots[4].pointer, args->slots[5].pointer,
            args->slots[6].pointer);
}

static void _ncs_dkp_camera_konqchar_show_hide_alpha(void) {
    ncs_dkp_camera_konqchar_show_hide_alpha(((struct ScriptRawArgs*)current_args)->slots[0].i,
                                            ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _ncs_camera_wall_show_hide_alpha(void) {
    ncs_camera_wall_show_hide_alpha(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata();
}

static void _ncs_bgnd_nuke_collision_to_script_interface(void) { ncs_bgnd_nuke_collision_to_script_interface(); }

static void _retrieve_bgnd_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = retrieve_bgnd_obj();
}

static void _fkbm_obj_face_obj(void) {
    fkbm_obj_face_obj(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                      ((struct ScriptRawArgs*)current_args)->slots[2].i, ((struct ScriptRawArgs*)current_args)->slots[3].i,
                      ((struct ScriptRawArgs*)current_args)->slots[4].i);
}

static void _obj_grnd_bounce(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_grnd_bounce(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].f, args->slots[3].f, args->slots[4].i, args->slots[5].f);
}

static void _start_obj_scalar_proc(void) {
    start_obj_scalar_proc(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                          ((struct ScriptRawArgs*)current_args)->slots[2].pointer, ((struct ScriptRawArgs*)current_args)->slots[3].pointer);
}

static void _obj_match_obj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_match_obj_pos(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].f, args->slots[3].i);
}

static void _insert_particle_mkobj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        insert_particle_mkobj(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _mkobj_pos_pos_dot_normal_xz(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        mkobj_pos_pos_dot_normal_xz(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                    ((struct ScriptRawArgs*)current_args)->slots[1].pointer,
                                    ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _obj_get_bid_for_tid(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        obj_get_bid_for_tid(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _obj_create_sobjs_by_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        obj_create_sobjs_by_id(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _unhide_sobj_by_sobj_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        unhide_sobj_by_sobj_id(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _hide_sobj_by_sobj_id(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        hide_sobj_by_sobj_id(
            ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
            ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _sobj_set_priority(void) { sobj_set_priority(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].i); }

static void _unhide_sobj(void) { unhide_sobj(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _hide_sobj(void) { hide_sobj(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _unhide_obj(void) { unhide_obj(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _hide_obj(void) { hide_obj(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _pfx_plyr_bankowner(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        pfx_plyr_bankowner(((struct ScriptRawArgs*)current_args)->slots[0].i);
}

static void _ncs_script_debug_quickie(void) {
    ncs_script_debug_quickie(((struct ScriptRawArgs*)current_args)->slots[0].i,
                             ((struct ScriptRawArgs*)current_args)->slots[1].f,
                             ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _get_inverse_game_speed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_inverse_game_speed();
}

static void _ck_rumble_controller(void) {
    ck_rumble_controller(((struct ScriptRawArgs*)current_args)->slots[0].i, ((struct ScriptRawArgs*)current_args)->slots[1].i,
                         ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _check_for_green_light(void) { ((struct ScriptRawResult*)active_cmdscript)->value.i = check_for_green_light(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _check_for_red_light(void) { ((struct ScriptRawResult*)active_cmdscript)->value.i = check_for_red_light(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _ck_put_weapon_away(void) { ck_put_weapon_away(((struct ScriptRawArgs*)current_args)->slots[0].pointer); }

static void _find_obj_by_id(void) { ((struct ScriptRawResult*)active_cmdscript)->value.pointer = find_obj_by_id(((struct ScriptRawArgs*)current_args)->slots[0].i); }

static void _yinyang_reset_music_index(void) { yinyang_reset_music_index(); }

static void _yinyang_play_evil_tune(void) { yinyang_play_evil_tune(); }

static void _yinyang_play_good_tune(void) { yinyang_play_good_tune(); }

static void _dist_v3_to_v3(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        dist_v3_to_v3(((struct ScriptRawArgs*)current_args)->slots[0].pointer, ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _is_point_in_fortress_exclusion_zone(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        is_point_in_fortress_exclusion_zone(((struct ScriptRawArgs*)current_args)->slots[0].pointer);
}

static void _fortress_setup_exclusion_zone(void) {
    fortress_setup_exclusion_zone(((struct ScriptRawArgs*)current_args)->slots[0].pointer,
                                  ((struct ScriptRawArgs*)current_args)->slots[1].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[2].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[3].f,
                                  ((struct ScriptRawArgs*)current_args)->slots[4].f);
}

static void _set_evil_swap_status(void) { set_evil_swap_status(((struct ScriptRawArgs*)current_args)->slots[0].i); }

static void _ok_to_do_evil_swap(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = ok_to_do_evil_swap();
}

static void _set_evil_condition(void) { set_evil_condition(((struct ScriptRawArgs*)current_args)->slots[0].i); }

static void _yy_is_evil_time_active(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = yy_is_evil_time_active();
}

static void _bgnd_make_object_transl(void) { bgnd_make_object_transl(((struct ScriptRawArgs*)current_args)->slots[0].i); }

static void _are_death_traps_on(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = are_death_traps_on();
}

static void _get_string_by_id(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptStringResult* result;
    char* string;

    args = (struct ScriptSingleIntArgs*)current_args;
    string = get_string_by_id(args->value);
    result = (struct ScriptStringResult*)active_cmdscript;
    result->value = string;
}

static void _ending_show_text(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    ending_show_text(args->first, args->second);
}

static void _ending_show_image(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    ending_show_image(args->value);
}

static void _cam_set_intro_cam_speed(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    cam_set_intro_cam_speed(args->value);
}

static void _midpoint_v3(void) {
    struct ScriptThreeVecArgs* args;

    args = (struct ScriptThreeVecArgs*)current_args;
    midpoint_v3(args->out, args->first,
                args->second);
}

static void _sobj_get_world_pos(void) {
    struct ScriptSobjArgs* args;
    struct ScriptVecResult* result;
    Vec* position;

    args = (struct ScriptSobjArgs*)current_args;
    position = sobj_get_world_pos(args->value);
    result = (struct ScriptVecResult*)active_cmdscript;
    result->value = position;
}

static void _get_point_on_circle(void) {
    struct ScriptCircleArgs* args;

    args = (struct ScriptCircleArgs*)current_args;
    get_point_on_circle(args->center, args->radius,
                        args->angle, args->out);
}

static void _cut_player_in_half(void) {
    struct ScriptObjectArgs* args;
    struct ScriptPointerResult* result;
    MkHdr* upper_body;

    args = (struct ScriptObjectArgs*)current_args;
    upper_body = cut_player_in_half(args->object);
    result = (struct ScriptPointerResult*)active_cmdscript;
    result->value = upper_body;
}

static void _is_plyr_airborn(void) {
    struct ScriptAirborneArgs* args;
    struct ScriptIntResult* result;
    int airborne;

    args = (struct ScriptAirborneArgs*)current_args;
    airborne = is_plyr_airborn(args->object, args->player);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = airborne;
}

static void _get_offset_of_closest_fence_section(void) {
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

static void _find_mkproc_pid(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptProcResult* result;
    MkProc* proc;

    args = (struct ScriptSingleIntArgs*)current_args;
    proc = find_mkproc_pid(args->value);
    result = (struct ScriptProcResult*)active_cmdscript;
    result->value = proc;
}

static void _start_fish_attack(void) {
    struct ScriptFishAttackArgs* args;

    args = (struct ScriptFishAttackArgs*)current_args;
    start_fish_attack(args->player, args->direction,
                      args->flags);
}

static void _is_timer_off(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_timer_off();
}

static void _hide_sobj_if_camera_is_in_rectangle(void) {
    struct ScriptCameraRectangleArgs* args;

    args = (struct ScriptCameraRectangleArgs*)current_args;
    hide_sobj_if_camera_is_in_rectangle(
        args->object, args->center,
        args->min_x, args->min_z,
        args->max_x, args->max_z);
}

static void _hide_sobj_if_camera_is_in_cylinder(void) {
    struct ScriptCameraCylinderArgs* args;

    args = (struct ScriptCameraCylinderArgs*)current_args;
    hide_sobj_if_camera_is_in_cylinder(
        args->object, args->center,
        args->radius, args->height);
}

static void _turn_off_sobj_if_camera_is_in_rectangle(void) {
    struct ScriptCameraRectangleArgs* args;

    args = (struct ScriptCameraRectangleArgs*)current_args;
    turn_off_sobj_if_camera_is_in_rectangle(
        args->object, args->center,
        args->min_x, args->min_z,
        args->max_x, args->max_z);
}

static void _turn_off_sobj_if_camera_is_in_cylinder(void) {
    struct ScriptCameraCylinderArgs* args;

    args = (struct ScriptCameraCylinderArgs*)current_args;
    turn_off_sobj_if_camera_is_in_cylinder(
        args->object, args->center,
        args->radius, args->height);
}

static void _debug_create_axis_indicator(void) {
    struct ScriptAxisArgs* args;

    args = (struct ScriptAxisArgs*)current_args;
    debug_create_axis_indicator(args->player, args->axis);
}

static void _bgnd_level_fatality_end(void) {
    bgnd_level_fatality_end();
}

static void _bgnd_level_fatality_start(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_level_fatality_start(args->value);
}

static void _bgnd_level_transition_end(void) {
    bgnd_level_transition_end();
}

static void _bgnd_level_transition_start(void) {
    bgnd_level_transition_start();
}

static void _do_lightning_strike(void) {
    struct ScriptLightningArgs* args;

    args = (struct ScriptLightningArgs*)current_args;
    do_lightning_strike(args->owner, args->position);
}

static void _obj_set_sobj_alpha(void) {
    struct ScriptSobjAlphaArgs* args;

    args = (struct ScriptSobjAlphaArgs*)current_args;
    obj_set_sobj_alpha(args->object, args->sobj_index,
                       args->alpha);
}

static void _obj_for_all_atomics_set_material_alpha(void) {
    struct ScriptObjectIntArgs* args;

    args = (struct ScriptObjectIntArgs*)current_args;
    obj_for_all_atomics_set_material_alpha(args->object,
                                           args->value);
}

static void _get_volume_from_distance(void) {
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

static void _get_pan_value(void) {
    struct ScriptFloatPointerArgs* args;
    struct ScriptFloatResult* result;
    float pan;

    args = (struct ScriptFloatPointerArgs*)current_args;
    pan = get_pan_value((Vec*)args->value);
    result = (struct ScriptFloatResult*)active_cmdscript;
    result->value = pan;
}

static void _mab_test(void) {
    mab_test();
}

static void _player_body_explode(void) {
    struct ScriptBodyExplodeArgs* args;

    args = (struct ScriptBodyExplodeArgs*)current_args;
    player_body_explode(args->player,
                        args->position,
                        args->velocity);
}

static void _bgnd_clear_danger_zone_callback(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_clear_danger_zone_callback((PlyrPdata*)args->value);
}

static void _yinyang_set_bad_fish_hide_flag(void) {
    struct ScriptFishFlagArgs* args;

    args = (struct ScriptFishFlagArgs*)current_args;
    yinyang_set_bad_fish_hide_flag(
        args->entries, args->hidden,
        args->count);
}

static void _yinyang_set_good_fish_hide_flag(void) {
    struct ScriptFishFlagArgs* args;

    args = (struct ScriptFishFlagArgs*)current_args;
    yinyang_set_good_fish_hide_flag(
        args->entries, args->hidden,
        args->count);
}

static void _yinyang_make_fish_jump(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    yinyang_make_fish_jump(args->first, args->second);
}

static void _destroy_mkobjs_oid(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    destroy_mkobjs_oid(args->value);
}

static void _do_yinyang_statue_explosion(void) {
    struct ScriptObjectArgs* args;

    args = (struct ScriptObjectArgs*)current_args;
    do_yinyang_statue_explosion(args->object);
}

static void _obj_add_to_skinned_obj_light_list_with_ambient(void) {
    struct ScriptLightListArgs* args;

    args = (struct ScriptLightListArgs*)current_args;
    obj_add_to_skinned_obj_light_list_with_ambient(
        args->object, args->light);
}

static void _obj_change_to_bgnd_obj_light_list(void) {
    struct ScriptLightListArgs* args;

    args = (struct ScriptLightListArgs*)current_args;
    obj_change_to_bgnd_obj_light_list(args->object,
                                      args->light);
}

static void _obj_change_to_skinned_obj_light_list(void) {
    struct ScriptLightListArgs* args;

    args = (struct ScriptLightListArgs*)current_args;
    obj_change_to_skinned_obj_light_list(args->object,
                                         args->light);
}

static void _yinyang_stop_lensflare(void) {
    yinyang_stop_lensflare();
}

static void _yinyang_start_lensflare(void) {
    yinyang_start_lensflare();
}

static void _misc_data_set_test_float(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    misc_data_set_test_float(args->value);
}

static void _misc_data_set_test_u32(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    misc_data_set_test_u32(args->value);
}

static void _obj_set_sobj_priority(void) {
    struct ScriptSobjAlphaArgs* args;

    args = (struct ScriptSobjAlphaArgs*)current_args;
    obj_set_sobj_priority(args->object,
                          args->sobj_index,
                          args->alpha);
}

static void _sobj_disable_blending(void) {
    struct ScriptSobjArgs* args;

    args = (struct ScriptSobjArgs*)current_args;
    sobj_disable_blending(args->value);
}

static void _obj_first_sobj(void) {
    struct ScriptMkObjArgs* args;
    struct ScriptSobjResult* result;
    MkSobj* object;

    args = (struct ScriptMkObjArgs*)current_args;
    object = obj_first_sobj(args->object);
    result = (struct ScriptSobjResult*)active_cmdscript;
    result->value = object;
}

static void _pebble_turn_culling_off(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    pebble_turn_culling_off(args->value);
}

static void _pebble_turn_culling_on(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    pebble_turn_culling_on(args->value);
}

static void _pebble_unhide_me(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    pebble_unhide_me(args->first, args->second);
}

static void _pebble_hide_me(void) {
    struct ScriptTwoIntArgs* args;

    args = (struct ScriptTwoIntArgs*)current_args;
    pebble_hide_me(args->first, args->second);
}

static void _pebble_setup_bounce_props(void) {
    struct ScriptPebbleBounceArgs* args;

    args = (struct ScriptPebbleBounceArgs*)current_args;
    pebble_setup_bounce_props(
        args->player, args->index,
        args->velocity, args->flags);
}

static void _pebble_set_ang_vel(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_ang_vel(args->player, args->index,
                       args->value);
}

static void _pebble_set_ang(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_ang(args->player, args->index,
                   args->value);
}

static void _pebble_set_scale(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_scale(args->player, args->index,
                     args->value);
}

static void _pebble_set_vel(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_vel(args->player, args->index,
                   args->value);
}

static void _pebble_set_pos(void) {
    struct ScriptPebbleVecArgs* args;

    args = (struct ScriptPebbleVecArgs*)current_args;
    pebble_set_pos(args->player, args->index,
                   args->value);
}

static void _destroy_mkobj(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    destroy_mkobj(args->object);
}

static void _obj_turn_gravity_off(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    obj_turn_gravity_off(args->object);
}

static void _obj_set_gravity(void) {
    struct ScriptMkObjFloatArgs* args;

    args = (struct ScriptMkObjFloatArgs*)current_args;
    obj_set_gravity(args->object, args->value);
}

static void _insert_fgnd_mkobj(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    insert_fgnd_mkobj(args->object);
}

static void _get_player_number(void) {
    struct ScriptMkObjArgs* args;
    struct ScriptIntResult* result;
    int player;

    args = (struct ScriptMkObjArgs*)current_args;
    player = get_player_number(args->object);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = player;
}

static void _load_named_model_for_player(void) {
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

static void _load_named_model_from_slot(void) {
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

static void _script_fabs(void) {
    struct ScriptSingleFloatArgs* args;
    struct ScriptFloatResult* result;
    float value;

    args = (struct ScriptSingleFloatArgs*)current_args;
    value = script_fabs(args->value);
    result = (struct ScriptFloatResult*)active_cmdscript;
    result->value = value;
}

static void _set_obj_light_flags(void) {
    struct ScriptObjectIntArgs* args;

    args = (struct ScriptObjectIntArgs*)current_args;
    set_obj_light_flags(args->object, args->value);
}

static void _set_obj_ang(void) {
    struct ScriptMkObjVecArgs* args;

    args = (struct ScriptMkObjVecArgs*)current_args;
    set_obj_ang(args->object, args->x,
                args->y, args->z);
}

static void _set_obj_pos(void) {
    struct ScriptMkObjVecArgs* args;

    args = (struct ScriptMkObjVecArgs*)current_args;
    set_obj_pos(args->object, args->x,
                args->y, args->z);
}

static void _bgnd_unhide_sobj_list(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_unhide_sobj_list((unsigned int*)args->value);
}

static void _bgnd_hide_sobj_list(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    bgnd_hide_sobj_list((unsigned int*)args->value);
}

static void _random_percent(void) {
    struct ScriptSingleFloatArgs* args;
    struct ScriptIntResult* result;
    int value;

    args = (struct ScriptSingleFloatArgs*)current_args;
    value = random_percent(args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = value;
}

static void _bgnd_sobj_start_morph(void) {
    struct ScriptMorphArgs* args;

    args = (struct ScriptMorphArgs*)current_args;
    bgnd_sobj_start_morph(args->object, args->sobj_id,
                          args->script_id, args->flags);
}

static void _delete_screen_obj_oid(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    delete_screen_obj_oid(args->value);
}

static void _reset_collision_system(void) {
    reset_collision_system();
}

static void _pos_cam_for_current_level(void) {
    pos_cam_for_current_level();
}

static void _reset_severed_limbs(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    reset_severed_limbs(args->value);
}

static void _move_plyrs_to_round_start(void) {
    move_plyrs_to_round_start();
}

static void _set_far_clip_plane(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    set_far_clip_plane(args->value);
}

static void _skytemple_player_explode(void) {
    struct ScriptSkytempleExplodeArgs* args;

    args = (struct ScriptSkytempleExplodeArgs*)current_args;
    skytemple_player_explode(
        args->player, args->x,
        args->y, args->z);
}

static void _skytemple_make_scream_sound(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    skytemple_make_scream_sound(args->value);
}

static void _turn_controllers_on(void) {
    turn_controllers_on();
}

static void _turn_controllers_off(void) {
    turn_controllers_off();
}

static void _bgnd_clear_face_opponent_flags(void) {
    bgnd_clear_face_opponent_flags();
}

static void _mab_script_trace_func(void) {
    mab_script_trace_func(get_script_string_arg(1));
}

static void _skytemple_arrange_fence_pebbles_around_pos(void) {
    struct ScriptPebbleArrangeArgs* args;

    args = (struct ScriptPebbleArrangeArgs*)current_args;
    skytemple_arrange_fence_pebbles_around_pos(
        args->player, args->count,
        args->position);
}

static void _skytemple_set_fence_pebble_vel(void) {
    struct ScriptPebbleVelocityArgs* args;

    args = (struct ScriptPebbleVelocityArgs*)current_args;
    skytemple_set_fence_pebble_vel(
        args->player, args->count,
        args->x, args->y,
        args->z);
}

static void _scripted_camera_script_exit(void) {
    scripted_camera_script_exit();
}

static void _bgnd_launch_fx_at_sobj_pos(void) {
    struct ScriptNamedSobjFxArgs* args;
    char* name;

    args = (struct ScriptNamedSobjFxArgs*)current_args;
    name = get_script_string_arg(1);
    bgnd_launch_fx_at_sobj_pos(name, args->sobj_id,
                               args->y_offset);
}

static void _bgnd_get_first_shape_center_for_obstacle_id(void) {
    struct ScriptTwoIntArgs* args;
    struct ScriptIntResult* result;
    int found;

    args = (struct ScriptTwoIntArgs*)current_args;
    found = bgnd_get_first_shape_center_for_obstacle_id(
        args->first, args->second);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = found;
}

static void _misc_data_set_obj_ptr1(void) {
    struct ScriptMkObjArgs* args;

    args = (struct ScriptMkObjArgs*)current_args;
    misc_data_set_obj_ptr1(args->object);
}

static void _misc_data_get_obj_ptr1(void) {
    struct ScriptMkObjResult* result;
    MkObj* object;

    object = misc_data_get_obj_ptr1();
    result = (struct ScriptMkObjResult*)active_cmdscript;
    result->value = object;
}

static void _misc_data_set_col_obj_id2(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    misc_data_set_col_obj_id2(args->value);
}

static void _misc_data_get_col_obj_id2(void) {
    struct ScriptIntResult* result;
    int object_id;

    object_id = misc_data_get_col_obj_id2();
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = object_id;
}

static void _misc_data_set_col_obj_id(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    misc_data_set_col_obj_id(args->value);
}

static void _misc_data_get_col_obj_id(void) {
    struct ScriptIntResult* result;
    int object_id;

    object_id = misc_data_get_col_obj_id();
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = object_id;
}

static void _bgnd_fetch_obj(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptMkObjResult* result;
    MkObj* object;

    args = (struct ScriptSingleIntArgs*)current_args;
    object = bgnd_fetch_obj(args->value);
    result = (struct ScriptMkObjResult*)active_cmdscript;
    result->value = object;
}

static void _bgnd_swap_textures_tbl(void) {
    struct ScriptSwapTextureTableArgs* args;

    args = (struct ScriptSwapTextureTableArgs*)current_args;
    bgnd_swap_textures_tbl(args->entries,
                           args->frame);
}

static void _bgnd_swap_textures(void) {
    struct ScriptSwapTextureArgs* args;

    args = (struct ScriptSwapTextureArgs*)current_args;
    bgnd_swap_textures(args->sobj_id,
                       args->material_id,
                       args->frame);
}

static void _bgnd_append_texture_to_material_tbl(void) {
    struct ScriptAppendTextureTableArgs* args;

    args = (struct ScriptAppendTextureTableArgs*)current_args;
    bgnd_append_texture_to_material_tbl(
        args->entries);
}

static void _bgnd_append_texture_to_material(void) {
    struct ScriptAppendTextureArgs* args;

    args = (struct ScriptAppendTextureArgs*)current_args;
    bgnd_append_texture_to_material(
        args->sobj_id, args->material_id,
        get_script_string_arg(3), args->texture_slot);
}

static void _bgnd_light_set_color(void) {
    struct ScriptLightColorArgs* args;

    args = (struct ScriptLightColorArgs*)current_args;
    bgnd_light_set_color(args->light_id, args->red,
                         args->green, args->blue);
}

static void _bgnd_get_float(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptFloatResult* result;
    float value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = bgnd_get_float(args->value);
    result = (struct ScriptFloatResult*)active_cmdscript;
    result->value = value;
}

static void _bgnd_get_u32(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptIntResult* result;
    unsigned int value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = bgnd_get_u32(args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = value;
}

static void _bgnd_get_int(void) {
    struct ScriptSingleIntArgs* args;
    struct ScriptIntResult* result;
    int value;

    args = (struct ScriptSingleIntArgs*)current_args;
    value = bgnd_get_int(args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = value;
}

static void _mks_start_fatality_iceball(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    mks_start_fatality_iceball(args->value);
}

static void _send_speedup_msg(void) {
}

static void _mks_debug_display_cloth_ontop(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    mks_debug_display_cloth_ontop(args->value);
}

static void _start_bow(void) {
    struct ScriptIntFloatArgs* args;

    args = (struct ScriptIntFloatArgs*)current_args;
    start_bow(args->integer, args->floating);
}

static void _get_bid_with_flip(void) {
    struct ScriptObjectIntArgs* args;
    struct ScriptIntResult* result;
    int bone_id;

    args = (struct ScriptObjectIntArgs*)current_args;
    bone_id = get_bid_with_flip(args->object,
                                args->value);
    result = (struct ScriptIntResult*)active_cmdscript;
    result->value = bone_id;
}

static void _start_plyr_attack(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    start_plyr_attack(args->value);
}

static void _set_active_projectile_upward_attack(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_upward_attack(args->vector);
}

static void _set_active_add_ang_y(void) {
    struct ScriptSingleFloatArgs* args;

    args = (struct ScriptSingleFloatArgs*)current_args;
    set_active_add_ang_y(args->value);
}

static void _set_active_projectile_random_rot(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    set_active_projectile_random_rot(
        args->x, args->y, args->z);
}

static void _set_active_projectile_dn_sound(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    set_active_projectile_dn_sound(args->value);
}

static void _set_active_projectile_sound(void) {
    struct ScriptThreeIntArgs* args;

    args = (struct ScriptThreeIntArgs*)current_args;
    set_active_projectile_sound(args->first, args->second,
                                args->third);
}

static void _set_active_projectile_random_pos(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    set_active_projectile_random_pos(
        args->x, args->y, args->z);
}

static void _set_active_projectile_velocity_damp(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_velocity_damp(args->vector);
}

static void _set_active_projectile_target_pos(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_target_pos(args->vector);
}

static void _set_active_projectile_max_ticks(void) {
    struct ScriptSingleIntArgs* args;

    args = (struct ScriptSingleIntArgs*)current_args;
    set_active_projectile_max_ticks(args->value);
}

static void _set_active_projectile_p_handler(void) {
    struct ScriptEntryArgs* args;

    args = (struct ScriptEntryArgs*)current_args;
    set_active_projectile_p_handler((MkProcEntryFn)args->entry);
}

static void _set_active_projectile_target_ground(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    set_active_projectile_target_ground(
        args->x, args->y, args->z);
}

static void _set_active_projectile_velocity(void) {
    struct ScriptVecPointerArgs* args;

    args = (struct ScriptVecPointerArgs*)current_args;
    set_active_projectile_velocity(args->vector);
}

static void _active_projectile_setup_done(void) {
    active_projectile_setup_done();
}

static void _set_active_projectile_hit_gnd_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_hit_gnd_script(args->slots[0].u);
}

static void _set_active_projectile_end_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_end_script(args->slots[0].u);
}

static void _set_active_projectile_hit_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_hit_script(args->slots[0].u);
}

static void _set_active_projectile_impale_info(void) {
    struct ScriptTwoPointerArgs* args;

    args = (struct ScriptTwoPointerArgs*)current_args;
    set_active_projectile_impale_info(
        args->first,
        args->second);
}

static void _set_active_projectile_not_duckable(void) {
    set_active_projectile_not_duckable();
}

static void _set_active_projectile_rx_info(void) {
    struct ScriptReactionArgs* args;

    args = (struct ScriptReactionArgs*)current_args;
    set_active_projectile_rx_info(
        args->reaction, args->rate,
        args->strength);
}

static void _start_projectile_from_plyr_bone(void) {
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

static void _run_reaction_cleanup_function(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    run_reaction_cleanup_function(args->player);
}

static void _reaction_xfer_him(void) {
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

static void _mks_set_cb1_wind_normal(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    mks_set_cb1_wind_normal(
        args->x, args->y, args->z);
}

static void _mks_bgnd_start_wind(void) {
    struct ScriptVecValueArgs* args;

    args = (struct ScriptVecValueArgs*)current_args;
    mks_bgnd_start_wind(args->x, args->y,
                        args->z);
}

static void _mks_npc_build_bones_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_build_bones_tbl(
        args->slots[0].i, args->slots[1].pointer);
}

static void _mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance(
        args->player);
}

static void _mks_xfer_collision_info_plyr_to_bgnd_script(void) {
    struct ScriptPlyrEntryArgs* args;

    args = (struct ScriptPlyrEntryArgs*)current_args;
    mks_xfer_collision_info_plyr_to_bgnd_script(
        args->player, args->entry);
}

static void _mks_xfer_collision_info_plyr_to_script(void) {
    struct ScriptEntryPlayerArgs* args;

    args = (struct ScriptEntryPlayerArgs*)current_args;
    mks_xfer_collision_info_plyr_to_script(
        args->entry, args->player);
}

static void _resume_effect_at_plyr_num_bid(void) {
    struct ScriptResumeEffectArgs* args;

    args = (struct ScriptResumeEffectArgs*)current_args;
    resume_effect_at_plyr_num_bid(
        args->player, args->bone_id,
        args->effect_handle, args->bind_mode,
        args->blood_required);
}

static void _resume_effect_at_obj_bid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    resume_effect_at_obj_bid(
        args->slots[0].pointer, args->slots[1].i,
        args->slots[2].u, args->slots[3].i,
        args->slots[4].i);
}

static void _mks_start_gusher(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        mks_start_gusher(
            args->slots[0].i, args->slots[1].i, current_args,
            args->slots[2].f, args->slots[3].f,
            args->slots[4].f, args->slots[5].f,
            args->slots[6].f, args->slots[7].f);
}

static void _mks_victim_bleed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_victim_bleed(args->slots[0].i, args->slots[1].i);
}

static void _mks_plyr_stop(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_plyr_stop(args->slots[0].i);
}

static void _mks_set_plyr_to_center_ang_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_plyr_to_center_ang_offset(args->slots[0].i, args->slots[1].f);
}

static void _mks_bgnd_cam_offset_away(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_bgnd_cam_offset_away(args->slots[0].f, args->slots[1].f);
}

static void _mks_bgnd_pfx_bind_to_sobj(void) {
    mks_bgnd_pfx_bind_to_sobj(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].u);
}

static void _mks_set_update_delay(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_update_delay(args->slots[0].i, args->slots[1].i);
}

static void _mks_removehide_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_removehide_by_group(args->slots[0].i, args->slots[1].i);
}

static void _mks_blend_start_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_blend_start_update_by_group(args->slots[0].i, args->slots[1].i);
}

static void _mks_shadow_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_shadow_scale(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _mks_gravity_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_gravity_update_by_group(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f);
}

static void _mks_away_vel_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_away_vel_update_by_group(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

static void _mks_set_rotate_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_rotate_update_by_group(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].f, args->slots[5].i);
}

static void _mks_set_sin_update_by_group(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_sin_update_by_group(
        args->slots[0].i, args->slots[1].i, args->slots[2].i,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].f, args->slots[7].f, args->slots[8].f,
        args->slots[9].i);
}

static void _bgnd_init_timers(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_init_timers(args->slots[0].i);
}

static void _bgnd_obj_insert_obj_ctrl_section(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_obj_insert_obj_ctrl_section(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_insert_obj_ctrl_section(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_insert_obj_ctrl_section(args->slots[0].i, args->slots[1].i);
}

static void _destroy_sobj_ctrl_proc(void) {
    destroy_sobj_ctrl_proc();
}

static void _start_sobj_ctrl_proc(void) {
    start_sobj_ctrl_proc();
}

static void _set_active_projectile_collision_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_collision_info(args->slots[0].f, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _set_active_projectile_tracking_light(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptMkObjResult*)active_cmdscript)->value =
        set_active_projectile_tracking_light(args->slots[0].pointer);
}

static void _set_active_projectile_continue_thru_hit(void) {
    set_active_projectile_continue_thru_hit();
}

static void _set_active_projectile_3d_track(void) {
    set_active_projectile_3d_track();
}

static void _set_active_projectile_2d_track(void) {
    set_active_projectile_2d_track();
}

static void _set_active_projectile_velocity_to_hit_gnd(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_velocity_to_hit_gnd(args->slots[0].f);
}

static void _get_projectile_script_velocity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_projectile_script_velocity(args->slots[0].pointer);
}

static void _get_projectile_script_last_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_projectile_script_last_pos(args->slots[0].pointer);
}

static void _get_projectile_script_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_projectile_script_plyr_pdata();
}

static void _get_projectile_his_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_projectile_his_plyr_num();
}

static void _get_projectile_script_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_projectile_script_plyr_num();
}

static void _mks_get_victim_to_tr_dot(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mks_get_victim_to_tr_dot(args->slots[0].i);
}

static void _ani_x_more_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_x_more_frames(args->slots[0].f);
}

static void _bgnd_npc_set_ani_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_ani_speed(args->slots[0].i, args->slots[1].f);
}

static void _mks_npc_disable_ground_y_all_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_disable_ground_y_all_cloth_bones(args->slots[0].i);
}

static void _mks_npc_set_ground_y_all_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_set_ground_y_all_cloth_bones(args->slots[0].i, args->slots[1].f);
}

static void _mks_npc_cb1_eq_cloth_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_cb1_eq_cloth_bone(args->slots[0].i, args->slots[1].i);
}

static void _mks_npc_cc1_eq_insert_cloth_coll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_cc1_eq_insert_cloth_coll(args->slots[0].i, args->slots[1].i, args->slots[2].f);
}

static void _mks_npc_set_target(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_set_target(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _mks_npc_cloth_bones_init_by_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_cloth_bones_init_by_tbl(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _mks_npc_start_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_npc_start_cloth_bones(args->slots[0].i);
}

static void _bgnd_attach_rope_to_bgnd_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_attach_rope_to_bgnd_obj(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_detach_rope(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_detach_rope(args->slots[0].i);
}

static void _start_rope_proc(void) {
    start_rope_proc();
}

static void _bgnd_preload_obj_attach_rope(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_preload_obj_attach_rope(args->slots[0].i);
}

static void _bgnd_hide_preload_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_preload_obj(args->slots[0].i);
}

static void _bgnd_unhide_preload_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_preload_obj(args->slots[0].i);
}

static void _load_bgnd_style(void) {
    const char* name = get_script_string_arg(2);

    load_bgnd_style(((struct ScriptRawArgs*)current_args)->slots[0].i,
                    name, current_args);
}

static void _mks_set_cb1_target_bone_cb2(void) {
    mks_set_cb1_target_bone_cb2();
}

static void _mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave(args->slots[0].i, args->slots[1].f, args->slots[2].i, args->slots[3].f, args->slots[4].i, args->slots[5].f, args->slots[6].i, args->slots[7].f);
}

static void _mks_debug_display_cloth_coll_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_debug_display_cloth_coll_plane(args->slots[0].f);
}

static void _mks_debug_display_cloth_coll_cyl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_debug_display_cloth_coll_cyl(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl(void) {
    mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl();
}

static void _collision_result_dont_care(void) {
    collision_result_dont_care();
}

static void _clear_collision_result(void) {
    clear_collision_result();
}

static void _wait_for_collision_result(void) {
}

static void _reaction_sync_advance(void) {
}

static void _plyr_set_gravity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_set_gravity(args->slots[0].f);
}

static void _plyr_scale_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_scale_pos_vel(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _animpdata_get_anim_hiframe(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        animpdata_get_anim_hiframe(args->slots[0].pointer);
}

static void _plyr_get_anim_hiframe(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_get_anim_hiframe();
}

static void _plyr_get_anim_frame(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_get_anim_frame();
}

static void _plyr_set_vel_xz_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_set_vel_xz_y(args->slots[0].f, args->slots[1].f);
}

static void _player_area_collision_check(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = player_area_collision_check(
        args->slots[0].f, args->slots[1].f, args->slots[2].i,
        args->slots[3].f, args->slots[4].i);
}

static void _single_frame_collision_check(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = single_frame_collision_check(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].i, args->slots[4].i, args->slots[5].f);
}

static void _is_he_airborn(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_he_airborn();
}

static void _auto_ani_on(void) {
    auto_ani_on();
}

static void _mks_obj_enable_update_cloth(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_obj_enable_update_cloth(args->slots[0].pointer, args->slots[1].i);
}

static void _mks_bgnd_obj_enable_cloth_update(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_bgnd_obj_enable_cloth_update(args->slots[0].i, args->slots[1].i);
}

static void _start_special_weapon_monitor(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_special_weapon_monitor(args->slots[0].i);
}

static void _xz_distance_between_players(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = xz_distance_between_players();
}

static void _start_scorpion_teleport_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_scorpion_teleport_scale(current_args, args->slots[0].f, args->slots[1].f);
}

static void _blast_effect_at_plyr(void) {
    blast_effect_at_plyr();
}

static void _configure_iceball(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    configure_iceball(args->slots[0].pointer);
}

static void _scorpion_teleport_position(void) {
    scorpion_teleport_position();
}

static void _kenshi_teleport_position(void) {
    kenshi_teleport_position();
}

static void _start_shadow_watcher(void) {
    start_shadow_watcher();
}

static void _update_my_last_switch(void) {
    update_my_last_switch();
}

static void _local_collision_allowed_plyr_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = local_collision_allowed_plyr_pdata();
}

static void _kill_plyr_life(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kill_plyr_life(args->slots[0].i);
}

static void _is_local_plyr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_local_plyr();
}

static void _is_reaction_xfer_him_allowed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_reaction_xfer_him_allowed();
}

static void _throw_spear(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = throw_spear();
}

static void _pan_vol_pitch_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pan_vol_pitch_snd_req(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _force_ai_style(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    force_ai_style(args->slots[0].i);
}

static void _xfer_player_proc_to_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    xfer_player_proc_to_script(
        (MkObj*)args->slots[0].i, args->slots[1].i);
}

static void _start_subzero_decoy(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_subzero_decoy(current_args, args->slots[0].f);
}

static void _jmt_debug_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jmt_debug_script(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

static void _can_fallingcliff_fall(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = can_fallingcliff_fall();
}

static void _get_online_evil_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = 0;
}

static void _send_yinyang_evil_msg(void) {
}

static void _set_remote_plyr_online_pos(void) {
}

static void _enable_no_sync_anim_f(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_no_sync_anim_f(args->slots[0].i);
}

static void _enable_no_adjustment_f(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_no_adjustment_f(args->slots[0].i);
}

static void _get_exec_tick_ctr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_exec_tick_ctr();
}

static void _set_cliff_watcher_round(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_cliff_watcher_round(args->slots[0].i);
}

static void _get_cliff_watcher_round(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_cliff_watcher_round();
}

static void _clear_cliff_data(void) {
    clear_cliff_data();
}

static void _start_cliff_watcher(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_cliff_watcher(args->slots[0].f);
}

static void _get_cliff_data(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_cliff_data();
}

static void _destroy_kabal_smoke(void) {
    destroy_kabal_smoke();
}

static void _start_kabal_smoke(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_kabal_smoke(current_args, args->slots[0].f);
}

static void _kobra_teleport_position(void) {
    kobra_teleport_position();
}

static void _mileena_sky_set_position(void) {
    mileena_sky_set_position();
}

static void _disable_mileena_collisions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_mileena_collisions(args->slots[0].i);
}

static void _switch_plyr_positions(void) {
    switch_plyr_positions();
}

static void _kabal_collision_control_victim(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kabal_collision_control_victim(args->slots[0].i);
}

static void _gusher_destroy_list(void) {
    gusher_destroy_list();
}

static void _cloth_change_ground_plane_for(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    cloth_change_ground_plane_for(args->slots[0].f);
}

static void _set_constrain_last_pos_pdata(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_constrain_last_pos_pdata(args->slots[0].pointer);
}

static void _get_current_bgnd(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_current_bgnd();
}

static void _damage_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    damage_player(args->slots[0].pointer, args->slots[1].f);
}

static void _flying_collision(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    flying_collision(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].i, args->slots[4].i, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f, args->slots[9].f);
}

static void _player_area_collision_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_area_collision_ticks(args->slots[0].f, args->slots[1].f, args->slots[2].i, args->slots[3].f, args->slots[4].i, args->slots[5].f);
}

static void _get_adjusted_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f =
        get_adjusted_speed(args->slots[0].f, args->slots[1].f);
}

static void _adjust_kabal_position(void) {
    adjust_kabal_position();
}

static void _is_drone(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_drone();
}

static void _release_both_players(void) {
    release_both_players();
}

static void _remove_impaled_projectiles(void) {
    remove_impaled_projectiles();
}

static void _online_sync_reset(void) {
    online_sync_reset();
}

static void _advance_my_sidekick_from_behind_with_moveset(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = advance_my_sidekick_from_behind_with_moveset();
}

static void _kill_ermac_eyes(void) {
    kill_ermac_eyes();
}

static void _check_for_online_condition(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        check_for_online_condition(args->slots[0].pointer);
}

static void _increment_taunts_performed(void) {
    increment_taunts_performed();
}

static void _get_taunts_performed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_taunts_performed();
}

static void _mk_chess_fetch_bp_num_based_on_pchr_num(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_bp_num_based_on_pchr_num(args->slots[0].i);
}

static void _bgnd_fx_get_binded_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        bgnd_fx_get_binded_obj(args->slots[0].i);
}

static void _bgnd_enable_obj_pos_and_ang_setting(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_enable_obj_pos_and_ang_setting(
        args->slots[0].pointer, args->slots[1].pointer,
        args->slots[2].pointer);
}

static void _mk_chess_fetch_active_defined_team(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_active_defined_team();
}

static void _mk_chess_fetch_active_defined_teams_class(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_fetch_active_defined_teams_class(args->slots[0].i);
}

static void _mk_chess_air_move(void) {
    mk_chess_air_move();
}

static void _mk_chess_active_piece_near_edge(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_active_piece_near_edge();
}

static void _courtyard_start_lensflare(void) {
    courtyard_start_lensflare();
}

static void _mk_chess_queue_up_piece_event(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_queue_up_piece_event(args->slots[0].i, args->slots[1].i);
}

static void _mk_chess_piece_set_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_piece_set_state(args->slots[0].i);
}

static void _mk_chess_shifter_switch(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_shifter_switch(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _mk_chess_blend_to_my_cell_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_my_cell_pos(args->slots[0].f);
}

static void _mk_chess_snap_to_my_cell_now(void) {
    mk_chess_snap_to_my_cell_now();
}

static void _mk_chess_xfer_piece_from_scripts(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_xfer_piece_from_scripts(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _mk_chess_piece_die(void) {
    mk_chess_piece_die();
}

static void _mk_chess_launch_special_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_launch_special_fx(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _mk_chess_set_piece_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_info(args->slots[0].i, args->slots[1].f);
}

static void _mk_chess_launch_fx_at_active_piece_with_offset(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    mk_chess_launch_fx_at_active_piece_with_offset(
        effect_name, args->slots[1].f, args->slots[2].f,
        args->slots[3].f);
}

static void _mk_chess_spell_rescue_current_target(void) {
    mk_chess_spell_rescue_current_target();
}

static void _mk_chess_get_piece_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_get_piece_info(args->slots[0].i);
}

static void _mk_chess_piece_temporarily_gone(void) {
    mk_chess_piece_temporarily_gone();
}

static void _mk_chess_launch_up(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_launch_up(args->slots[0].f, args->slots[1].f);
}

static void _mk_chess_spell_get_target_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_spell_get_target_health(args->slots[0].i);
}

static void _mk_chess_spell_move_target_from_temp_area_to(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_move_target_from_temp_area_to(args->slots[0].i);
}

static void _mk_chess_spell_move_target_to_temp_area(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_move_target_to_temp_area(args->slots[0].i);
}

static void _mk_chess_spell_target_add_access_restrictions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_target_add_access_restrictions(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _mk_chess_spell_has_completed_but_wait_for_fight(void) {
    mk_chess_spell_has_completed_but_wait_for_fight();
}

static void _mk_chess_spell_force_fight(void) {
    mk_chess_spell_force_fight();
}

static void _mk_chess_spell_is_this_a_forced_fight(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_spell_is_this_a_forced_fight();
}

static void _mk_chess_spell_kill_target(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_kill_target(args->slots[0].i);
}

static void _mk_chess_spell_move_target_to_target(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_move_target_to_target(args->slots[0].i, args->slots[1].i);
}

static void _mk_chess_spell_set_target_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_set_target_health(args->slots[0].i, args->slots[1].f);
}

static void _mk_chess_spell_get_target_max_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_spell_get_target_max_health(args->slots[0].i);
}

static void _mk_chess_spell_show_target_portrait(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_spell_show_target_portrait(args->slots[0].i);
}

static void _mk_chess_spell_has_completed(void) {
    mk_chess_spell_has_completed();
}

static void _mk_chess_make_spellcaster(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_make_spellcaster(args->slots[0].pointer);
}

static void _mk_chess_set_piece_event_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_event_script(args->slots[0].i, args->slots[1].function);
}

static void _mk_chess_piece_test_and_set_timer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_piece_test_and_set_timer(args->slots[0].i, args->slots[1].i);
}

static void _mk_chess_deactivate_my_properties(void) {
    mk_chess_deactivate_my_properties();
}

static void _mk_chess_ani_until_reached_destination(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_until_reached_destination(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i);
}

static void _mk_chess_rotate_towards_cell(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_rotate_towards_cell(args->slots[0].f, args->slots[1].f, args->slots[2].f, args->slots[3].i, args->slots[4].f);
}

static void _mk_chess_request_piece_script_for_action(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_request_piece_script_for_action(args->slots[0].i);
}

static void _mk_chess_piece_is_idle(void) {
    mk_chess_piece_is_idle();
}

static void _mk_chess_put_active_piece_at_cell(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_put_active_piece_at_cell(args->slots[0].f, args->slots[1].f, args->slots[2].i);
}

static void _mk_chess_launch_n_land_ani_with_xz(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_launch_n_land_ani_with_xz(
        args->slots[0].i, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].f, args->slots[7].f, args->slots[8].f,
        args->slots[9].f, args->slots[10].f, args->slots[11].i,
        args->slots[12].u);
}

static void _mk_chess_stop_me(void) {
    mk_chess_stop_me();
}

static void _mk_chess_get_piece_event_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = mk_chess_get_piece_event_data(args->slots[0].i);
}

static void _bgnd_active_sobj_no_ztest(void) {
    bgnd_active_sobj_no_ztest();
}

static void _mk_chess_set_cell_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_cell_offset(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _mk_chess_force_away(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_force_away(args->slots[0].f, args->slots[1].i, args->slots[2].f, args->slots[3].i);
}

static void _mk_chess_add_movement_skill(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_add_movement_skill(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _mk_chess_define_class_initial_power(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_define_class_initial_power(args->slots[0].f);
}

static void _mk_chess_activate_my_properties(void) {
    mk_chess_activate_my_properties();
}

static void _mk_chess_wait_until_attack_cam_closes_in(void) {
    mk_chess_wait_until_attack_cam_closes_in();
}

static void _mk_chess_snd_request(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_snd_request(args->slots[0].i);
}

static void _mk_chess_dont_constrain_piece(void) {
    mk_chess_dont_constrain_piece();
}

static void _mk_chess_piece_match_y_ang_to_anim(void) {
    mk_chess_piece_match_y_ang_to_anim();
}

static void _mk_chess_check_glitch_into_stance(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_check_glitch_into_stance();
}

static void _mk_chess_set_glitch_stance_flag(void) {
    mk_chess_set_glitch_stance_flag();
}

static void _mk_chess_glitch_to_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_glitch_to_ani_frame(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _mk_chess_set_ani_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_ani_speed(args->slots[0].f);
}

static void _bgnd_add_fx_to_hide(void) {
    bgnd_add_fx_to_hide(get_script_string_arg(1));
}

static void _mk_chess_place_special_cell_at(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_place_special_cell_at(
        args->slots[0].i, args->slots[1].i, args->slots[2].i,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].f, args->slots[7].i);
}

static void _mk_chess_piece_event_from_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_piece_event_from_script(args->slots[0].i);
}

static void _mk_chess_blend_to_desired_cell_position_setting(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_desired_cell_position_setting(args->slots[0].f);
}

static void _mk_chess_blend_into_cell_orgin_in_x_frames_by_caller(void) {
    mk_chess_blend_into_cell_orgin_in_x_frames_by_caller();
}

static void _mk_chess_set_obj_move_weight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_obj_move_weight(args->slots[0].f);
}

static void _mk_chess_snap_into_cell_orgin_over_x_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_snap_into_cell_orgin_over_x_frames(args->slots[0].f);
}

static void _mk_chess_check_snap_into_stance(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = mk_chess_check_snap_into_stance();
}

static void _mk_chess_snap_to_stance(void) {
    mk_chess_snap_to_stance();
}

static void _mk_chess_blend_to_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_ani_frame(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

static void _mk_chess_blend_to_normal_stance(void) {
    mk_chess_blend_to_normal_stance();
}

static void _mk_chess_set_normal_stance_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_normal_stance_script(args->slots[0].pointer);
}

static void _bgnd_kill_fx(void) {
    bgnd_kill_fx(get_script_string_arg(1));
}

static void _bgnd_no_z_test(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_no_z_test(args->slots[0].i);
}

static void _bgnd_get_active_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_get_active_sobj_pos((Vec*)args->slots[0].i);
}

static void _mk_chess_set_piece_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_state(args->slots[0].i);
}

static void _mk_chess_set_piece_type_as(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_set_piece_type_as(args->slots[0].i, args->slots[1].i);
}

static void _mk_chess_blend_into_cell_orgin_in_x_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_into_cell_orgin_in_x_frames(args->slots[0].f);
}

static void _mk_chess_ani_loop_more_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_loop_more_frames(args->slots[0].f);
}

static void _mk_chess_ani_idle(void) {
    mk_chess_ani_idle();
}

static void _mk_chess_ani_to_blend_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_to_blend_frame(args->slots[0].f);
}

static void _mk_chess_ani_to_end(void) {
    mk_chess_ani_to_end();
}

static void _mk_chess_ani_1_frame(void) {
    mk_chess_ani_1_frame();
}

static void _mk_chess_ani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_ani_to_frame_x(args->slots[0].f);
}

static void _mk_chess_blend_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_blend_to_ani(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _mk_chess_init_piece(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_init_piece(args->slots[0].i, args->slots[1].i);
}

static void _mk_chess_load_chess_table(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mk_chess_load_chess_table(args->slots[0].pointer);
}

static void _bgnd_enable_wall_hider(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_enable_wall_hider(args->slots[0].i);
}

static void _bgnd_npc_set_pos_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos_y(args->slots[0].i, args->slots[1].f);
}

static void _bgnd_npc_set_pos_vel_heading(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos_vel_heading(args->slots[0].i, args->slots[1].f);
}

static void _bgnd_npc_set_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos_vel(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_npc_get_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_npc_get_ang_y(args->slots[0].i);
}

static void _bgnd_npc_adjust_y_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_adjust_y_ang(args->slots[0].i, args->slots[1].f);
}

static void _bgnd_npc_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_get_pos(args->slots[0].i, args->slots[1].pointer);
}

static void _bgnd_npc_get_aux_int_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_npc_get_aux_int_data(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_npc_start_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_start_ani(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].f, args->slots[4].f);
}

static void _bgnd_npc_set_aux_int_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_aux_int_data(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_add_scripted_brains_to_npc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_scripted_brains_to_npc(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_npc_set_y_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_y_ang(args->slots[0].i, args->slots[1].f);
}

static void _bgnd_npc_like_plyr(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_like_plyr(args->slots[0].i);
}

static void _bgnd_npc_set_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_scale(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_create_named_npc_in_slot(void) {
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

static void _bgnd_npc_set_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_npc_set_pos(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _danger_zone_eligible_on(void) {
    danger_zone_eligible_on();
}

static void _bgnd_get_exec_tick_ctr(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_get_exec_tick_ctr();
}

static void _start_sobj_launch_monitor(void) {
    start_sobj_launch_monitor();
}

static void _start_chunk_launch_monitor(void) {
    start_chunk_launch_monitor();
}

static void _snd_req_delay(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    snd_req_delay(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_set_material_color(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_material_color(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i);
}

static void _bgnd_set_fx_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_fx_ang_y(args->slots[0].f);
}

static void _bgnd_launch_plyr_blood_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_launch_plyr_blood_fx(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_sobj_set_priority(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_priority(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_pfxhandle_spawn_at_bid(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        bgnd_pfxhandle_spawn_at_bid(
            effect_name, args->slots[1].pointer,
            args->slots[2].i);
}

static void _bgnd_launch_fx_at_bid_of_mkobj(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_bid_of_mkobj(
        effect_name, args->slots[1].pointer,
        args->slots[2].i);
}

static void _bgnd_launch_fx_at_plyr_bid(void) {
    bgnd_launch_fx_at_plyr_bid(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _cam_set_intro_cam_pause_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    cam_set_intro_cam_pause_ticks(args->slots[0].f);
}

static void _bgnd_collision_if_monitor_col_as(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_monitor_col_as(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _bgnd_pebble_burst_set_end_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_set_end_state(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _bgnd_pebble_change_current_end_behavior(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_change_current_end_behavior(args->slots[0].i);
}

static void _bgnd_pebble_burst_set_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_set_value(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f, args->slots[9].f);
}

static void _bgnd_pebble_burst_set_value_min_max(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_set_value_min_max(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f, args->slots[5].f);
}

static void _bgnd_pebble_change_current_behavior_to_bounce(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_change_current_behavior_to_bounce(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i, args->slots[7].i);
}

static void _bgnd_start_preload_sobj_morph(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_preload_sobj_morph(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _bgnd_start_preload_sobj_uv_scroll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_preload_sobj_uv_scroll(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f);
}

static void _bgnd_launch_fx_at_active_sobj_pos_with_offset(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_active_sobj_pos_with_offset(
        effect_name, args->slots[1].f, args->slots[2].f,
        args->slots[3].f);
}

static void _bgnd_active_sobj_no_zwrite(void) {
    bgnd_active_sobj_no_zwrite();
}

static void _bgnd_set_active_sobj_zoffset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_zoffset(current_args, args->slots[0].f);
}

static void _face_point(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    face_point(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _share_my_attack_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    share_my_attack_info(args->slots[0].f, args->slots[1].f);
}

static void _ani_with_pos(void) {
    ani_with_pos();
}

static void _ani_no_pos(void) {
    ani_no_pos();
}

static void _bgnd_pebble_burst_at_pebble_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_at_pebble_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_pebble_burst_at_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_at_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i, current_args, args->slots[3].f, args->slots[4].f, args->slots[5].f);
}

static void _nb_place_slave_in_bgnd(void) {
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

static void _nb_npc_slave_plyr_process_collision(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    nb_npc_slave_plyr_process_collision(args->slots[0].u);
}

static void _bgnd_collision_if_enable_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_enable_col(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_collision_if_disable_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_disable_col(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_pebble_simple_launch_at_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_simple_launch_at_time(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _bgnd_pebble_set_current_pebble(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_set_current_pebble(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_remove_cracks(void) {
    bgnd_remove_cracks();
}

static void _bgnd_pebble_rand_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_rand_scale(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_pebble_burst_at_chunk_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_burst_at_chunk_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_unhide_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_pebbles(args->slots[0].i);
}

static void _bgnd_hide_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_pebbles(args->slots[0].i);
}

static void _spad_set_heading_vector_to(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_heading_vector_to(args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

static void _degrees_to_rad(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = degrees_to_rad(current_args, args->slots[0].f);
}

static void _rad_to_degrees(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = rad_to_degrees(current_args, args->slots[0].f);
}

static void _bgnd_pebble_set_current_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_set_current_info(args->slots[0].i, args->slots[1].f);
}

static void _bgnd_pebble_fetch_current_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_pebble_fetch_current_info(args->slots[0].i);
}

static void _bgnd_pebble_change_current_behavior(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_change_current_behavior(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i, args->slots[7].i);
}

static void _bgnd_init_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_init_pebbles(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_pebble_gravity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pebble_gravity(args->slots[0].i, current_args, args->slots[1].f);
}

static void _get_sobj_pebble_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_sobj_pebble_obj((MkSobj*)args->slots[0].i);
}

static void _get_general_pebble_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_general_pebble_data(args->slots[0].pointer);
}

static void _ncs_create_pebble_monitor_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = ncs_create_pebble_monitor_proc(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _ncs_set_pebble_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ncs_set_pebble_pos(
        (struct PebbleData*)args->slots[0].i, args->slots[1].i,
        (Vec*)args->slots[2].i);
}

static void _ncs_create_pebbles_with_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = ncs_create_pebbles_with_sobj(args->slots[0].pointer, args->slots[1].i);
}

static void _bgnd_create_pebbles_with_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_create_pebbles_with_sobj(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _bgnd_create_pebbles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_create_pebbles(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i);
}

static void _bgnd_pebble_launch_at_time(void) {
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

static void _bgnd_jtb_debug_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_jtb_debug_info(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _get_game_speed(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_game_speed();
}

static void _get_soul_sine(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_soul_sine(args->slots[0].i);
}

static void _bgnd_set_active_sobj_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_ang(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _build_sine_table_for_scripts(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = build_sine_table_for_scripts();
}

static void _bgnd_is_active_sobj_hidden(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_is_active_sobj_hidden();
}

static void _bgnd_set_fx_ang_dir_to_i_vector(void) {
    bgnd_set_fx_ang_dir_to_i_vector();
}

static void _bgnd_set_active_sobj_in_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_in_obj(args->slots[0].i, args->slots[1].i);
}

static void _pfxhandle_bgnd_spawn_at_position(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        pfxhandle_bgnd_spawn_at_position(
            effect_name, args->slots[1].f, args->slots[2].f,
            args->slots[3].f);
}

static void _bgnd_launch_fx_to_sobj(void) {
    bgnd_launch_fx_to_sobj(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _bgnd_launch_fx_at_position(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    args = (struct ScriptRawArgs*)current_args;
    effect_name = get_script_string_arg(1);
    bgnd_launch_fx_at_position(
        effect_name, args->slots[1].f, args->slots[2].f,
        args->slots[3].f);
}

static void _bgnd_set_sobj_launch_params_exact(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_launch_params_exact(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f);
}

static void _load_script_as_reaction(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    load_script_as_reaction(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_collision_if_rx_override(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collision_if_rx_override(args->slots[0].i);
}

static void _set_background_obstacle_disable_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_background_obstacle_disable_flag(args->slots[0].i, args->slots[1].i);
}

static void _set_background_obstacle_repel_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_background_obstacle_repel_flag(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_collison_if_monitor_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collison_if_monitor_col(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i, args->slots[6].i);
}

static void _bgnd_collison_if_set_info(void) {
    bgnd_collison_if_set_info();
}

static void _bgnd_collison_if_set_return_result(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_collison_if_set_return_result(args->slots[0].i);
}

static void _bgnd_collison_if_to_scripts_activate(void) {
    bgnd_collison_if_to_scripts_activate();
}

static void _initialize_background_danger_zones(void) {
    initialize_background_danger_zones();
}

static void _spad_xz_cos_two_vectors(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_xz_cos_two_vectors(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_timer_get_tick_count(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_timer_get_tick_count(args->slots[0].i);
}

static void _spad_xz_length_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_xz_length_vector(args->slots[0].i);
}

static void _bgnd_current_rx_set_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_current_rx_set_info(args->slots[0].i, current_args, args->slots[1].f);
}

static void _bgnd_current_rx_get_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_current_rx_get_info(args->slots[0].i);
}

static void _bgnd_setup_rx_handler(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_setup_rx_handler(args->slots[0].i);
}

static void _bgnd_start_timer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_timer(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_kill_all_launched_sobjs(void) {
    bgnd_kill_all_launched_sobjs();
}

static void _jump_towards_opponent_bgnd_transition(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = jump_towards_opponent_bgnd_transition();
}

static void _allow_shard_pfx_now(void) {
    allow_shard_pfx_now();
}

static void _kill_shard_pfx_now(void) {
    kill_shard_pfx_now();
}

static void _bgnd_set_fx_z_offset(void) {
    const char* name = get_script_string_arg(1);

    bgnd_set_fx_z_offset(name,
        ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _bgnd_force_plyr_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_force_plyr_ground_plane(args->slots[0].i, current_args, args->slots[1].f);
}

static void _bgnd_set_launch_velocity_based_on_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_launch_velocity_based_on_sobj_pos(args->slots[0].i, args->slots[1].i, args->slots[2].i, current_args, args->slots[3].f, args->slots[4].f);
}

static void _bgnd_update_active_mksobj(void) {
    bgnd_update_active_mksobj();
}

static void _bgnd_shadow_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_shadow_control(args->slots[0].i);
}

static void _bgnd_set_plyr_gravity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_plyr_gravity(current_args, args->slots[0].f);
}

static void _bgnd_move_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_move_player(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _spad_sub_vectors(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_sub_vectors(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_act_at_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_act_at_time(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

static void _bgnd_xfer_attacker(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_xfer_attacker(args->slots[0].i);
}

static void _spad_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_get_pos(args->slots[0].i, args->slots[1].i);
}

static void _sfrand(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = sfrand(args->slots[0].f);
}

static void _spad_add_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_add_vector(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_launch_fx_at_plyr_pos_and_y(void) {
    bgnd_launch_fx_at_plyr_pos_and_y(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].f);
}

static void _bgnd_start_cracks(void) {
    bgnd_start_cracks();
}

static void _bgnd_init_cracks(void) {
    bgnd_init_cracks();
}

static void _bgnd_place_crack_when_plyr_hits_ground(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_place_crack_when_plyr_hits_ground(args->slots[0].i);
}

static void _plyr_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = plyr_get_pos(args->slots[0].i);
}

static void _bgnd_clean_up_floor(void) {
    bgnd_clean_up_floor();
}

static void _bgnd_allow_dirty_floor(void) {
    bgnd_allow_dirty_floor();
}

static void _cam_recalc_midpoint(void) {
    cam_recalc_midpoint();
}

static void _camera_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = camera_get_pos(args->slots[0].i);
}

static void _camera_set_movement_offset_explicit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_offset_explicit(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_force_ground_to(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_force_ground_to(current_args, args->slots[0].f);
}

static void _camera_set_lookat_offset_explicit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_lookat_offset_explicit(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_run_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_run_camera_script(args->slots[0].i, args->slots[1].i);
}

static void _spad_set_y_angle_plus_offset_from_xz_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_y_angle_plus_offset_from_xz_vector(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _spad_set_vector_setting(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_vector_setting(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_process_active_sobj_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_process_active_sobj_info(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_blood_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_blood_control(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

static void _delete_obstacle_from_background_by_id(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    delete_obstacle_from_background_by_id(args->slots[0].i);
}

static void _bgnd_make_displayed_item_pickupable_at_active_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_make_displayed_item_pickupable_at_active_sobj_pos(args->slots[0].i);
}

static void _bgnd_reset_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_reset_sobj(args->slots[0].i);
}

static void _restart_effect(void) {
    get_script_string_arg(1);
    restart_effect();
}

static void _bgnd_set_active_sobj_rop(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_rop(args->slots[0].i);
}

static void _bgnd_set_active_sobj_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_scale(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_set_active_sobj_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_pos(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_set_active_sobj_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj_pos_vel(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_apply_active_sobj_pos_vel_drag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_apply_active_sobj_pos_vel_drag(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _int_to_float(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = int_to_float(args->slots[0].i);
}

static void _float_to_int(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = float_to_int(current_args, args->slots[0].f);
}

static void _bgnd_set_sobj_launch_params(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_launch_params(args->slots[0].i, args->slots[1].i, args->slots[2].i, current_args, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f, args->slots[9].f);
}

static void _spad_scale_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_scale_vector(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

static void _spad_set_vector_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_vector_y(args->slots[0].i, args->slots[1].f);
}

static void _bgnd_process_collision_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_process_collision_info(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, args->slots[8].f);
}

static void _bgnd_takeover_plyr(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_takeover_plyr(args->slots[0].pointer);
}

static void _spad_norm_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_norm_vector(args->slots[0].i);
}

static void _spad_rotate_xz_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_rotate_xz_vector(args->slots[0].i, current_args, args->slots[1].f);
}

static void _spad_xz_dot_xz(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = spad_xz_dot_xz(args->slots[0].i, args->slots[1].i);
}

static void _spad_set_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spad_set_vector(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_launch_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_launch_sobj(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i, args->slots[6].f, args->slots[7].i);
}

static void _bgnd_set_collision_plane_for_launched_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_collision_plane_for_launched_sobj(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_set_kill_plane_for_launched_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_kill_plane_for_launched_sobj(args->slots[0].i);
}

static void _bgnd_fade_object(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_fade_object(args->slots[0].i, current_args, args->slots[1].f);
}

static void _bgnd_get_sobj_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_sobj_ang_y(args->slots[0].i);
}

static void _bgnd_rotate_xz_about_orgin_active_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_rotate_xz_about_orgin_active_sobj(args->slots[0].f);
}

static void _bgnd_hide_active_sobj(void) {
    bgnd_hide_active_sobj();
}

static void _bgnd_unhide_active_sobj(void) {
    bgnd_unhide_active_sobj();
}

static void _bgnd_set_active_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_sobj(args->slots[0].i);
}

static void _bgnd_chunk_explosion_match_velocity_with_params(void) {
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

static void _bgnd_restore_player(void) {
    bgnd_restore_player();
}

static void _dont_fence_plyr_in(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    dont_fence_plyr_in(args->slots[0].i);
}

static void _bgnd_launch_plyr_up_and_forward_running(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_launch_plyr_up_and_forward_running();
}

static void _bgnd_launch_plyr_up_and_forward(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_launch_plyr_up_and_forward(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].f,
        args->slots[6].i);
}

static void _bgnd_register_danger_zone_callback(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_register_danger_zone_callback(
        (PlyrPdata*)args->slots[0].i, args->slots[1].i);
}

static void _bgnd_swap_level(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_swap_level(args->slots[0].i);
}

static void _bgnd_clean_beetlelair(void) {
    bgnd_clean_beetlelair();
}

static void _start_bl_beetles_live_top_floor(void) {
    start_bl_beetles_live_top_floor();
}

static void _pz_fighter_allow_easy_continuation(void) {
    pz_fighter_allow_easy_continuation();
}

static void _pz_fighter_reset_continuation(void) {
    pz_fighter_reset_continuation();
}

static void _pz_fighter_disallow_continuation(void) {
    pz_fighter_disallow_continuation();
}

static void _pz_fighter_allow_continuation(void) {
    pz_fighter_allow_continuation();
}

static void _bgnd_add_new_normal_check_for_hider(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_new_normal_check_for_hider(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_remove_wall_from_hider(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_remove_wall_from_hider(args->slots[0].i);
}

static void _bgnd_add_wall_to_hide(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_wall_to_hide(args->slots[0].i);
}

static void _bgnd_add_wall_to_unhide(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_add_wall_to_unhide(args->slots[0].i);
}

static void _bgnd_start_wall_hider(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_wall_hider(args->slots[0].i);
}

static void _bgnd_turn_off_backface_culling(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_turn_off_backface_culling(args->slots[0].i);
}

static void _bgnd_sobj_cam_frustum_test_into_transparent(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_cam_frustum_test_into_transparent(
        args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

static void _obj_sobj_cam_frustum_test_into_transparent(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_sobj_cam_frustum_test_into_transparent(
        (MkObj*)args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f);
}

static void _bgnd_sobj_cam_volume_test_steer_over(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_cam_volume_test_steer_over(
        args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_rotate_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_rotate_sobj(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_set_wall_hide_distance(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_wall_hide_distance(current_args, args->slots[0].f);
}

static void _bgnd_set_new_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_new_ground_plane(current_args, args->slots[0].f);
}

static void _bgnd_place_weapon_at_position(void) {
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

static void _bgnd_clean_slaughterhouse(void) {
    bgnd_clean_slaughterhouse();
}

static void _bgnd_sh_level_2(void) {
    bgnd_sh_level_2();
}

static void _bgnd_sh_level_1(void) {
    bgnd_sh_level_1();
}

static void _bgnd_start_sh_fx(void) {
    bgnd_start_sh_fx();
}

static void _pz_fighter_reaction_xfer_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_reaction_xfer_him(args->slots[0].i);
}

static void _pz_fighter_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_function(args->slots[0].i);
}

static void _pz_fighter_fetch_distance_to_center_pos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = pz_fighter_fetch_distance_to_center_pos();
}

static void _set_anim_hiframe(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_anim_hiframe(args->slots[0].f);
}

static void _bgnd_reg_col_cb_for_beetle_lair(void) {
    bgnd_reg_col_cb_for_beetle_lair();
}

static void _pz_fighter_wipe_blood_off_hands(void) {
    pz_fighter_wipe_blood_off_hands();
}

static void _bgnd_get_preload_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        bgnd_get_preload_obj(args->slots[0].i);
}

static void _bgnd_preload_named_model(void) {
    const char* model_name;

    model_name = get_script_string_arg(1);
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        bgnd_preload_named_model(
            model_name, ((struct ScriptRawArgs*)current_args)->slots[1].i);
}

static void _sobj_set_alpha(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    sobj_set_alpha(args->slots[0].pointer, args->slots[1].i);
}

static void _obj_get_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_get_scale(args->slots[0].pointer, args->slots[1].pointer);
}

static void _obj_set_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_set_scale(args->slots[0].pointer, args->slots[1].pointer);
}

static void _bgnd_sobj_set_ani_framerate(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_ani_framerate(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

static void _bgnd_sobj_set_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_ani_frame(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _bgnd_sobj_set_alpha(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_alpha(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_sobj_set_rel_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_rel_pos(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_sobj_set_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_ang(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_sobj_get_ang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_get_ang(args->slots[0].i, (Vec*)args->slots[1].i);
}

static void _bgnd_sobj_set_pos_vel(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_pos_vel(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_sobj_set_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_sobj_set_pos(args->slots[0].i, current_args, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _bgnd_sobj_get_z_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_z_pos(args->slots[0].i);
}

static void _bgnd_sobj_get_y_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_y_pos(args->slots[0].i);
}

static void _bgnd_sobj_get_x_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_sobj_get_x_pos(args->slots[0].i);
}

static void _bgnd_unhide_sobj_and_children(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_sobj_and_children(args->slots[0].i);
}

static void _bgnd_hide_sobj_and_children(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_sobj_and_children(args->slots[0].i);
}

static void _bgnd_unhide_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_unhide_sobj(args->slots[0].i);
}

static void _bgnd_hide_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_hide_sobj(args->slots[0].i);
}

static void _head_tracking_on(void) {
    head_tracking_on();
}

static void _head_tracking_off(void) {
    head_tracking_off();
}

static void _load_aux_weapon(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    load_aux_weapon(args->slots[0].pointer);
}

static void _ani_loop_more_frames(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_loop_more_frames(args->slots[0].f);
}

static void _ani_1_frame(void) {
    ani_1_frame();
}

static void _pz_fighter_force_reaction_in_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_force_reaction_in_ticks(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_replace_tex_with_wiff_and_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_replace_tex_with_wiff_and_ani(
        ((struct ScriptRawArgs*)current_args)->slots[0].i, get_script_string_arg(2),
        args->slots[2].f, args->slots[3].i,
        args->slots[4].i);
}

static void _bgnd_pulsate_object_with_caps_and_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pulsate_object_with_caps_and_scale(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].i, args->slots[4].f, args->slots[5].i, args->slots[6].i, args->slots[7].f, args->slots[8].f, args->slots[9].f, args->slots[10].f, args->slots[11].f, args->slots[12].f);
}

static void _bgnd_pulsate_object_with_caps(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pulsate_object_with_caps(
        args->slots[0].i, args->slots[1].i, args->slots[2].f,
        args->slots[3].i, args->slots[4].f, args->slots[5].i,
        args->slots[6].i);
}

static void _bgnd_pulsate_object(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_pulsate_object(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].i, args->slots[4].f);
}

static void _bgnd_turn_on_backface_culling(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_turn_on_backface_culling(args->slots[0].i);
}

static void _bgnd_no_z_write(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_no_z_write(args->slots[0].i);
}

static void _frand(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = frand(args->slots[0].f);
}

static void _plyr_start_script_in_plyr_pdata_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_start_script_in_plyr_pdata_proc(args->slots[0].pointer, args->slots[1].i, args->slots[2].i);
}

static void _plyr_start_script_in_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_start_script_in_proc(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_always_face_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_always_face_y(args->slots[0].i);
}

static void _bgnd_apply_zoffset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_apply_zoffset(args->slots[0].i, current_args, args->slots[1].f);
}

static void _bgnd_start_sobj_uv_scroll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_sobj_uv_scroll(
        args->slots[0].i,
        args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f,
        args->slots[5].i);
}

static void _bgnd_create_sobjs(void) {
    bgnd_create_sobjs();
}

static void _sobj_no_zwrite(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    sobj_no_zwrite(args->slots[0].pointer);
}

static void _register_baraka_cb_functions(void) {
    register_baraka_cb_functions();
}

static void _start_baraka_blades_monitor(void) {
    start_baraka_blades_monitor();
}

static void _start_baraka_jaw_monitor(void) {
    start_baraka_jaw_monitor();
}

static void _pz_fighter_inline_force_away_with_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = pz_fighter_inline_force_away_with_ani(
        args->slots[0].f, args->slots[1].u,
        args->slots[2].f, args->slots[3].u);
}

static void _suspend_in_midair(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    suspend_in_midair(args->slots[0].f);
}

static void _snd_major_hit_voice(void) {
    snd_major_hit_voice();
}

static void _pz_fighter_shaking(void) {
    pz_fighter_shaking();
}

static void _pz_fighter_create_space_between_fighters(void) {
    pz_fighter_create_space_between_fighters();
}

static void _pz_fighter_force_repel_during_attack(void) {
    pz_fighter_force_repel_during_attack();
}

static void _pz_fighter_dont_fudge_desired_distance(void) {
    pz_fighter_dont_fudge_desired_distance();
}

static void _pz_fighter_check_to_toggle_obj_and_ani_flips(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_check_to_toggle_obj_and_ani_flips(args->slots[0].i);
}

static void _pz_fighter_move_into_fighting_position(void) {
    pz_fighter_move_into_fighting_position();
}

static void _pz_fighter_release_other_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_release_other_player(args->slots[0].i);
}

static void _pz_fighter_step_throw_into_check(void) {
    pz_fighter_step_throw_into_check();
}

static void _pz_fighter_check_breakout(void) {
    pz_fighter_check_breakout();
}

static void _pz_fighter_long_exit(void) {
    pz_fighter_long_exit();
}

static void _pz_fighter_exit(void) {
    pz_fighter_exit();
}

static void _pz_fighter_register_move(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pz_fighter_register_move(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i);
}

static void _get_my_particle_player_bank_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_my_particle_player_bank_num();
}

static void _flash_hit_at_bid_with_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    flash_hit_at_bid_with_y(args->slots[0].i, args->slots[1].f);
}

static void _flash_hit_at_bid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    flash_hit_at_bid(args->slots[0].i);
}

static void _fight_fx_im_hit_flash(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fight_fx_im_hit_flash(args->slots[0].i, args->slots[1].pointer, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f);
}

static void _launch_fx_at_pos_with_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = launch_fx_at_pos_with_obj(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _low_flash_check(void) {
    low_flash_check();
}

static void _medium_flash_check(void) {
    medium_flash_check();
}

static void _high_flash_check(void) {
    high_flash_check();
}

static void _bgnd_place_point_light_for_ticks(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;

    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        (int)bgnd_place_point_light_for_ticks(
            args->slots[0].pointer, args->slots[1].i,
            args->slots[2].f, args->slots[3].i);
}

static void _bgnd_delete_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_delete_danger_zone(args->slots[0].i);
}

static void _bgnd_set_active_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_active_danger_zone(args->slots[0].i);
}

static void _bgnd_set_danger_zone_radius(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_radius(args->slots[0].f);
}

static void _bgnd_set_viewing_of_danger_zones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_viewing_of_danger_zones(args->slots[0].i);
}

static void _bgnd_set_danger_zone_y_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_y_angle(args->slots[0].f);
}

static void _bgnd_set_danger_zone_depth(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_depth(args->slots[0].f);
}

static void _bgnd_set_danger_zone_width(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_width(args->slots[0].f);
}

static void _bgnd_set_danger_zone_center_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_danger_zone_center_position(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _bgnd_enable_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_enable_danger_zone(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_create_danger_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_create_danger_zone(args->slots[0].i, args->slots[1].i,
                            args->slots[2].i, args->slots[3].f,
                            args->slots[4].i);
}

static void _bgnd_place_object_at_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_place_object_at_position(args->slots[0].i, args->slots[1].i, args->slots[2].pointer, args->slots[3].pointer, args->slots[4].i);
}

static void _do_i_have_life_left(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = do_i_have_life_left();
}

static void _hide_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hide_player(args->slots[0].pointer, args->slots[1].i);
}

static void _setup_for_flip_ani(void) {
    setup_for_flip_ani();
}

static void _get_victory_flip_flags(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_victory_flip_flags();
}

static void _obj_set_z_offsets(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_set_z_offsets(args->slots[0].i, current_args, args->slots[1].f);
}

static void _get_my_plyr_num(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_my_plyr_num();
}

static void _do_victory_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    do_victory_camera(args->slots[0].pointer);
}

static void _unfreeze_player(void) {
    unfreeze_player();
}

static void _turn_into_energy_player(void) {
    turn_into_energy_player();
}

static void _is_blood_disabled(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_blood_disabled();
}

static void _plyr_get_f_constrained(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_get_f_constrained(args->slots[0].pointer);
}

static void _move_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    move_player(args->slots[0].pointer, args->slots[1].pointer,
                args->slots[2].pointer);
}

static void _face_ang_from_pos_to_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    face_ang_from_pos_to_him(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].pointer);
}

static void _move_player_no_constrain_update(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    move_player_no_constrain_update(args->slots[0].pointer,
                                   args->slots[1].pointer,
                                   args->slots[2].pointer);
}

static void _show_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_player(args->slots[0].pointer);
}

static void _plyr_invulnerable_to_projectiles(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_invulnerable_to_projectiles(args->slots[0].pointer, args->slots[1].i);
}

static void _active_sidekick_swap_from_behind(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    active_sidekick_swap_from_behind(args->player);
}

static void _active_sidekick_swap_from_sky(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    active_sidekick_swap_from_sky(args->player);
}

static void _active_sidekick_swap_change_style(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    active_sidekick_swap_change_style(args->player);
}

static void _is_sidekick_active(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        is_sidekick_active(args->slots[0].pointer);
}

static void _taunt_increase_life(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        taunt_increase_life(
            args->slots[0].f, args->slots[1].f);
}

static void _get_collision_result(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_collision_result();
}

static void _general_flash_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    general_flash_fx(
        args->slots[0].i, args->slots[1].pointer,
        get_script_string_arg(3), args->slots[3].i,
        args->slots[4].i, args->slots[5].f);
}

static void _get_kombat_difficulty(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_kombat_difficulty();
}

static void _uv_my_angle_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    uv_my_angle_y(
        args->slots[0].pointer, args->slots[1].f);
}

static void _obj_set_flipped_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_set_flipped_bones(
        args->slots[0].pointer, args->slots[1].pointer);
}

static void _run_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    run_camera_script(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _am_i_airborn_check_in_reaction(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = am_i_airborn_check_in_reaction();
}

static void _obj_enable_grounding(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_enable_grounding(args->slots[0].pointer);
}

static void _camera_idle(void) {
    camera_idle();
}

static void _drone_ai_increase_big_boss_stage(void) {
    struct ScriptPlyrPdataArgs* args;

    args = (struct ScriptPlyrPdataArgs*)current_args;
    drone_ai_increase_big_boss_stage(args->player);
}

static void _pz_finish_him_request(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pz_finish_him_request();
}

static void _pz_fighter_create_space_between_fighters_for_special_moves(void) {
    pz_fighter_create_space_between_fighters_for_special_moves();
}

static void _freeze_player(void) {
    freeze_player();
}

static void _get_his_previous_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_his_previous_state();
}

static void _get_his_secondary_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_his_secondary_state();
}

static void _pebble_get_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pebble_get_pos(args->slots[0].i, args->slots[1].i,
                   (Vec*)args->slots[2].i);
}

static void _is_big_boss(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_big_boss(args->slots[0].pointer);
}

static void _is_load_meter_active(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_load_meter_active();
}

static void _plyr_snd_req_no_plyr_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.u = plyr_snd_req_no_plyr_proc(args->slots[0].pointer, args->slots[1].i);
}

static void _noobsmoke_sidekick_double_charge(void) {
    noobsmoke_sidekick_double_charge();
}

static void _noobsmoke_sidekick_projectile(void) {
    noobsmoke_sidekick_projectile();
}

static void _forced_step_forward(void) {
    forced_step_forward();
}

static void _start_projectile_from_sidekick_bone(void) {
    struct ScriptRawArgs* args = (struct ScriptRawArgs*)current_args;
    MkObj* projectile = start_projectile_from_sidekick_bone(
        args->slots[0].i, args->slots[1].pointer, get_script_string_arg(3),
        args->slots[3].f, args->slots[4].f, args->slots[5].pointer);

    ((struct ScriptMkObjResult*)active_cmdscript)->value = projectile;
}

static void _noobsmoke_fire_projectile_request(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = noobsmoke_fire_projectile_request();
}

static void _smoke_victory_entrance(void) {
    smoke_victory_entrance();
}

static void _noob_victory_entrance(void) {
    noob_victory_entrance();
}

static void _sidekick_switch_style_swap(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    sidekick_switch_style_swap(args->slots[0].f, args->slots[1].u);
}

static void _dk_voice_call(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    dk_voice_call(args->slots[0].i, args->slots[1].i);
}

static void _dk_taunt_at_screen(void) {
    dk_taunt_at_screen();
}

static void _special_move_cam_setup2(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    special_move_cam_setup2(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i,
        args->slots[6].i, args->slots[7].i, args->slots[8].pointer,
        args->slots[9].pointer);
}

static void _rd_set_impact_vector(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    rd_set_impact_vector(args->slots[0].f);
}

static void _get_plyr_pdata_plyr_num(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_plyr_pdata_plyr_num(args->slots[0].i);
}

static void _is_special_move_available(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        is_special_move_available(
            args->slots[0].pointer,
            args->slots[1].i);
}

static void _retract_spear_from_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    retract_spear_from_camera(args->slots[0].pointer);
}

static void _fire_spear_at_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = fire_spear_at_camera(args->slots[0].pointer, args->slots[1].i);
}

static void _kick_the_camera(void) {
    kick_the_camera();
}

static void _random_dk_foot(void) {
    random_dk_foot();
}

static void _stop_vomit_slip_sound(void) {
    stop_vomit_slip_sound();
}

static void _drone_ai_should_ermac_fly_kick(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = drone_ai_should_ermac_fly_kick();
}

static void _drone_ai_should_ermac_ground_slam(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = drone_ai_should_ermac_ground_slam();
}

static void _clear_his_f_constrained(void) {
    clear_his_f_constrained();
}

static void _idle_his_anim_proc(void) {
    idle_his_anim_proc();
}

static void _set_active_projectile_block_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_active_projectile_block_script(args->slots[0].i);
}

static void _bgnd_get_obj_pointer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = bgnd_get_obj_pointer(args->slots[0].i);
}

static void _bgnd_get_anim_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_anim_info(args->slots[0].i, args->slots[1].i, args->slots[2].f);
}

static void _is_character_unlocked_in_profile(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_character_unlocked_in_profile(args->slots[0].i, args->slots[1].i);
}

static void _mini_mission_inactive(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mini_mission_inactive(args->slots[0].i);
}

static void _mini_mission_completed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mini_mission_completed(args->slots[0].i);
}

static void _start_mini_mission(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_mini_mission(args->slots[0].i);
}

static void _is_mini_mission_started(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_started(args->slots[0].i);
}

static void _is_mini_mission_active(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_active(args->slots[0].i);
}

static void _is_mini_mission_completed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_mini_mission_completed(args->slots[0].i);
}

static void _player_add_item_to_inventory(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_add_item_to_inventory(args->slots[0].pointer);
}

static void _player_remove_item_from_inventory(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_remove_item_from_inventory(args->slots[0].pointer);
}

static void _player_has_item(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = player_has_item(args->slots[0].pointer);
}

static void _add_to_konq_profile_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_to_konq_profile_value(args->slots[0].i, args->slots[1].i);
}

static void _get_last_character_trained_with(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_last_character_trained_with();
}

static void _set_last_character_trained_with(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_last_character_trained_with(args->slots[0].i);
}

static void _get_konq_profile_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_konq_profile_value(args->slots[0].i, args->slots[1].i);
}

static void _set_konq_profile_value(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konq_profile_value(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _set_pui_status(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_pui_status(args->slots[0].pointer, args->slots[1].i);
}

static void _get_pui_status(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        get_pui_status(args->slots[0].pointer);
}

static void _hf_bgnd_set_in_setup_zone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hf_bgnd_set_in_setup_zone(args->slots[0].pointer, args->slots[1].i);
}

static void _hf_bgnd_set_smasher_mode(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hf_bgnd_set_smasher_mode(args->slots[0].pointer, args->slots[1].i);
}

static void _bgnd_set_player_shadow_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_player_shadow_ground_plane(
        args->slots[0].i, args->slots[1].f);
}

static void _bgnd_end_the_game_and_restart(void) {
    bgnd_end_the_game_and_restart();
}

static void _bgnd_pfx_resume_effect(void) {
    bgnd_pfx_resume_effect(get_script_string_arg(1));
}

static void _bgnd_pfx_reset_effect(void) {
    bgnd_pfx_reset_effect(get_script_string_arg(1));
}

static void _bgnd_reset_players_animation_height(void) {
    bgnd_reset_players_animation_height();
}

static void _nbc_script_debug_point(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    nbc_script_debug_point(args->slots[0].i, args->slots[1].i, args->slots[2].f);
}

static void _bgnd_unhide_mirror_guys(void) {
    bgnd_unhide_mirror_guys();
}

static void _bgnd_hide_mirror_guys(void) {
    bgnd_hide_mirror_guys();
}

static void _bgnd_set_sobj_uv_scroll_abs_values(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_uv_scroll_abs_values(
        args->slots[0].f, args->slots[1].f,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].i);
}

static void _bgnd_set_sobj_uv_scroll_rate_values(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_set_sobj_uv_scroll_rate_values(
        args->slots[0].f, args->slots[1].f,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].i);
}

static void _bgnd_init_all_uv_scroll_w_control(void) {
    bgnd_init_all_uv_scroll_w_control();
}

static void _bgnd_destroy_sobj_uv_scroll_w_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_destroy_sobj_uv_scroll_w_control(args->slots[0].i);
}

static void _bgnd_start_sobj_uv_scroll_w_control(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i =
        bgnd_start_sobj_uv_scroll_w_control(
            args->slots[0].i, args->slots[1].f,
            args->slots[2].f, args->slots[3].f,
            args->slots[4].f, args->slots[5].i,
            args->slots[6].i);
}

static void _sh_lower_level_pebble_unhide(void) {
    sh_lower_level_pebble_unhide();
}

static void _sh_lower_level_pebble_hide(void) {
    sh_lower_level_pebble_hide();
}

static void _bgnd_get_camera_z_pos(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_camera_z_pos();
}

static void _bgnd_get_camera_y_angle(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.f = bgnd_get_camera_y_angle();
}

static void _bgnd_delete_proc_by_id(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_delete_proc_by_id(args->slots[0].i);
}

static void _bgnd_start_script_in_proc_bigstack(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_script_in_proc_bigstack(args->slots[0].i, args->slots[1].i);
}

static void _bgnd_start_script_in_proc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_script_in_proc(args->slots[0].i, args->slots[1].i);
}

static void _npc_run_shove_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_run_shove_animation(args->slots[0].i);
}

static void _npc_shove_reaction_standard_shutdown(void) {
    npc_shove_reaction_standard_shutdown();
}

static void _npc_shove_reaction_standard_setup(void) {
    npc_shove_reaction_standard_setup();
}

static void _npc_punch_reaction_standard_shutdown(void) {
    npc_punch_reaction_standard_shutdown();
}

static void _npc_prepare_for_unconscious_state(void) {
    npc_prepare_for_unconscious_state();
}

static void _npc_punch_reaction_standard_setup(void) {
    npc_punch_reaction_standard_setup();
}

static void _npc_get_collision_direction_in_script(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_collision_direction_in_script();
}

static void _npc_punch_reaction_check_data(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_punch_reaction_check_data();
}

static void _npc_run_punch_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_run_punch_animation(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, current_args, args->slots[4].f);
}

static void _npc_snap_to_face_monk(void) {
    npc_snap_to_face_monk();
}

static void _interior_exit_button_script(void) {
    interior_exit_button_script();
}

static void _get_ir_cam_ang_z(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_z(args->slots[0].i);
}

static void _get_ir_cam_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_y(args->slots[0].i);
}

static void _get_ir_cam_ang_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_ang_x(args->slots[0].i);
}

static void _get_ir_cam_pos_z(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_z(args->slots[0].i);
}

static void _get_ir_cam_pos_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_y(args->slots[0].i);
}

static void _get_ir_cam_pos_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.f = get_ir_cam_pos_x(args->slots[0].i);
}

static void _start_konquest_interior(void) {
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

static void _refresh_rate(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = refresh_rate();
}

static void _show_shujinko_unlock_screen(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_shujinko_unlock_screen(args->slots[0].i);
}

static void _trial_register_special_move(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_register_special_move(args->slots[0].i);
}

static void _trial_invisible_callback(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = trial_invisible_callback(args->slots[0].pointer);
}

static void _trial_set_special_restrictions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_special_restrictions(args->slots[0].i);
}

static void _drone_set_handicap(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_handicap(args->slots[0].i, args->slots[1].f);
}

static void _drone_start_bleeding(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_start_bleeding(args->slots[0].i, args->slots[1].f);
}

static void _drone_set_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_damage_multiplier(args->slots[0].i, args->slots[1].f);
}

static void _release_kamidogu(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    release_kamidogu(args->slots[0].pointer, args->slots[1].pointer);
}

static void _start_constrain_proc(void) {
    start_constrain_proc();
}

static void _credits_add_text(void) {
    credits_add_text(
        get_script_string_arg(1), get_script_string_arg(2),
        ((struct ScriptRawArgs*)current_args)->slots[2].i);
}

static void _trial_state_collision_check(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_state_collision_check(args->slots[0].i, args->slots[1].i);
}

static void _trial_debug_mission_list(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_debug_mission_list(args->slots[0].i);
}

static void _trial_get_background_root(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = trial_get_background_root();
}

static void _build_bones_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = build_bones_tbl(args->slots[0].pointer, args->slots[1].pointer);
}

static void _play_background_music(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    play_background_music(args->slots[0].i);
}

static void _give_koin_award(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    give_koin_award(args->slots[0].i, args->slots[1].i);
}

static void _konquest_run_ending(void) {
    konquest_run_ending();
}

static void _trial_set_round_health_restoration(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_round_health_restoration(args->slots[0].f);
}

static void _camera_set_anim_aux_data(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_anim_aux_data(
        args->slots[0].pointer);
}

static void _trial_mirror_anims_if_needed(void) {
    trial_mirror_anims_if_needed();
}

static void _current_player_is_drone(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = current_player_is_drone();
}

static void _drone_apply_damage(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_apply_damage(args->slots[0].i, args->slots[1].f);
}

static void _drone_set_special_directions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_special_directions(args->slots[0].i, args->slots[1].i);
}

static void _drone_set_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_position(args->slots[0].i, args->slots[1].f,
                       args->slots[2].f, args->slots[3].f);
}

static void _drone_set_health(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_health(args->slots[0].i, args->slots[1].f);
}

static void _drone_do_special_move(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_do_special_move(args->slots[0].i, args->slots[1].i);
}

static void _drone_change_to_style(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_change_to_style(args->slots[0].i, args->slots[1].i);
}

static void _drone_lip_synch(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_lip_synch(args->slots[0].i,
                    args->slots[1].pointer);
}

static void _trial_show_monk(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_show_monk(args->slots[0].i);
}

static void _show_text(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_text(
        args->slots[0].i, args->slots[1].i, args->slots[2].i,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i, args->slots[7].i);
}

static void _trial_set_next_mission(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_next_mission(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].i, args->slots[6].i, args->slots[7].i);
}

static void _trial_setup_nis_scene(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_setup_nis_scene(args->slots[0].i);
}

static void _drone_set_anim_step(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_anim_step(args->slots[0].f);
}

static void _drone_face_monk(void) {
    drone_face_monk();
}

static void _drone_set_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_script(args->slots[0].i, args->slots[1].i);
}

static void _trial_do_dialog(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_do_dialog(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f,
        args->slots[4].f, args->slots[5].i,
        args->slots[6].i);
}

static void _drone_set_difficulty_level(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_difficulty_level(args->slots[0].i);
}

static void _drone_dispatch_switches(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_dispatch_switches(args->slots[0].i);
}

static void _drone_set_switch_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    drone_set_switch_state(args->slots[0].i, args->slots[1].i);
}

static void _trial_restart_round(void) {
    trial_restart_round();
}

static void _trial_start_countdown(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_start_countdown(args->slots[0].i,
                          args->slots[1].f,
                          args->slots[2].f);
}

static void _trial_set_move_message(void) {
    trial_set_move_message(
        get_script_string_arg(1), get_script_string_arg(2));
}

static void _trial_set_next_setup_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_next_setup_function(args->slots[0].i);
}

static void _trial_set_combo_requirement(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_combo_requirement(args->slots[0].i, args->slots[1].f);
}

static void _trial_add_required_sequence(void) {
    trial_add_required_sequence(
        get_script_string_arg(1), get_script_string_arg(2));
}

static void _trial_setup_onscreen_display_items(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_setup_onscreen_display_items(
        args->slots[0].i, args->slots[1].i,
        get_script_string_arg(3));
}

static void _trial_add_success_condition(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_add_success_condition(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _trial_set_type(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_type(args->slots[0].i);
}

static void _trial_show_spoken_text_window(void) {
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

static void _trial_show_text_window(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_show_text_window(
        args->slots[0].i, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].i);
}

static void _trial_set_ending_functions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_ending_functions(args->slots[0].i, args->slots[1].i);
}

static void _trial_set_tick_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_tick_function(args->slots[0].i);
}

static void _trial_set_round_timer(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_round_timer(args->slots[0].i);
}

static void _trial_set_num_rounds(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trial_set_num_rounds(args->slots[0].i);
}

static void _konquest_map_setup_fight(void) {
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

static void _initialize_clone_lights(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    initialize_clone_lights(args->slots[0].pointer);
}

static void _jab_spawn_point_light_at_world_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        jab_spawn_point_light_at_world_pos(
            args->slots[0].pointer, args->slots[1].pointer);
}

static void _jab_attach_point_light_to_obj_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = jab_attach_point_light_to_obj_bone(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].i);
}

static void _jab_flash_screen(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_flash_screen(args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

static void _jab_stop_dragon_king_shake(void) {
    jab_stop_dragon_king_shake();
}

static void _jab_shake_dragon_king(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_shake_dragon_king(args->slots[0].f, args->slots[1].f);
}

static void _fade_fatality_screen(void) {
    fade_fatality_screen();
}

static void _jab_attach_wiff_to_sobj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_attach_wiff_to_sobj(
        args->slots[0].pointer, args->slots[1].i,
        get_script_string_arg(3), get_script_string_arg(4),
        args->slots[4].i, args->slots[5].f, args->slots[6].i);
}

static void _jab_destroy_drink_obj_in_hand(void) {
    jab_destroy_drink_obj_in_hand();
}

static void _jab_attach_drink_obj_to_hand(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_attach_drink_obj_to_hand(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].pointer);
}

static void _jab_face_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_face_obj(args->slots[0].pointer, args->slots[1].pointer);
}

static void _obj_scale_over_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    obj_scale_over_time(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].f);
}

static void _jab_release_jade_boomerang(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_release_jade_boomerang(args->slots[0].pointer);
}

static void _jab_start_jade_boomerang_throw(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_start_jade_boomerang_throw(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].f);
}

static void _jab_setup_kiss_emitter_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    jab_setup_kiss_emitter_obj(args->slots[0].pointer);
}

static void _kill_konquest_dialog_procs(void) {
    kill_konquest_dialog_procs();
}

static void _hero_turn_to_face_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hero_turn_to_face_position(args->slots[0].pointer);
}

static void _attach_pfx_to_object(void) {
    struct ScriptRawArgs* args;
    const char* effect_name;

    effect_name = get_script_string_arg(2);
    args = (struct ScriptRawArgs*)current_args;
    attach_pfx_to_object(
        args->slots[0].pointer, effect_name,
        args->slots[2].pointer);
}

static void _npc_hide_skip_message(void) {
    npc_hide_skip_message();
}

static void _npc_show_skip_message(void) {
    npc_show_skip_message();
}

static void _konquest_hero_portal_in(void) {
    konquest_hero_portal_in();
}

static void _konquest_set_current_portal_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_set_current_portal_uid(args->slots[0].i);
}

static void _npc_sleep_until_model_loaded(void) {
    npc_sleep_until_model_loaded();
}

static void _set_konquest_weather(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_weather(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _set_age_progression(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_age_progression(args->slots[0].i);
}

static void _display_time_progression_images(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    display_time_progression_images(args->slots[0].i);
}

static void _konquest_use_portal(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_use_portal(
        args->slots[0].i, args->slots[1].pointer, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i);
}

static void _konquest_teleport_hero_to_location(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_teleport_hero_to_location(args->slots[0].pointer);
}

static void _show_fight_message(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    show_fight_message(args->slots[0].i);
}

static void _npc_start_blood_fall(void) {
    npc_start_blood_fall();
}

static void _npc_get_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = npc_get_obj(args->slots[0].pointer);
}

static void _get_hero_state(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_hero_state();
}

static void _interaction_cam_set_target_info(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    interaction_cam_set_target_info(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].f,
        args->slots[6].i);
}

static void _set_movement_npc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_movement_npc(args->slots[0].i);
}

static void _set_look_at_npc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_look_at_npc(args->slots[0].i);
}

static void _get_krypt_anim_pdata(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_krypt_anim_pdata();
}

static void _get_krypt_character_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_krypt_character_obj();
}

static void _get_krypt_current_column(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_krypt_current_column();
}

static void _get_krypt_current_row(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_krypt_current_row();
}

static void _transition_to_krypt_character_anim_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    transition_to_krypt_character_anim_script(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _set_krypt_character_anim_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_krypt_character_anim_script(args->slots[0].i, args->slots[1].i, current_args, args->slots[2].f);
}

static void _set_krypt_character_previous_root_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_krypt_character_previous_root_angle(current_args, args->slots[0].f);
}

static void _set_krypt_character_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_krypt_character_angle(current_args, args->slots[0].f);
}

static void _set_krypt_character_pos(void) {
    struct ScriptPointerArgs* args;

    args = (struct ScriptPointerArgs*)current_args;
    set_krypt_character_pos(args->value);
}

static void _load_krypt_character(void) {
    ((struct ScriptMkObjResult*)active_cmdscript)->value =
        load_krypt_character(get_script_string_arg(1));
}

static void _npc_start_fx_at_his_position(void) {
    npc_start_fx_at_his_position(
        ((struct ScriptRawArgs*)current_args)->slots[0].pointer,
        get_script_string_arg(2),
        ((struct ScriptRawArgs*)current_args)->slots[2].pointer);
}

static void _npc_start_fx_at_position(void) {
    npc_start_fx_at_position(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _hero_start_fx_at_position(void) {
    hero_start_fx_at_position(
        get_script_string_arg(1),
        ((struct ScriptRawArgs*)current_args)->slots[1].pointer);
}

static void _set_hero_position_relative_to_chest(void) {
    set_hero_position_relative_to_chest();
}

static void _get_pickup_object(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_pickup_object();
}

static void _set_reference_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_reference_pui(args->slots[0].pointer);
}

static void _open_chest_and_give_item_to_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    open_chest_and_give_item_to_player(
        args->slots[0].pointer, args->slots[1].pointer);
}

static void _open_chest_and_unlock_kontent(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    open_chest_and_unlock_kontent(
        args->slots[0].pointer, args->slots[1].i);
}

static void _give_krypt_key_to_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    give_krypt_key_to_player(
        args->slots[0].pointer, args->slots[1].i);
}

static void _give_reward_to_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    give_reward_to_player(args->slots[0].pointer);
}

static void _npc_play_two_player_one_shot_anims(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_play_two_player_one_shot_anims(args->slots[0].i, args->slots[1].i);
}

static void _npc_assign_door_path(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_assign_door_path(args->slots[0].i, args->slots[1].i);
}

static void _npc_assign_path_to_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_assign_path_to_him(args->slots[0].pointer, args->slots[1].pointer, args->slots[2].i, args->slots[3].i);
}

static void _npc_assign_path(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_assign_path(args->slots[0].pointer, args->slots[1].i, args->slots[2].i);
}

static void _npc_open_door_at_waypoint(void) {
    npc_open_door_at_waypoint();
}

static void _setup_vomit_slip_sound(void) {
    setup_vomit_slip_sound();
}

static void _nis_remove_non_participants(void) {
    nis_remove_non_participants();
}

static void _set_camera_to_look_at_hero(void) {
    set_camera_to_look_at_hero();
}

static void _npc_reset_my_timed_events(void) {
    npc_reset_my_timed_events();
}

static void _konquest_setup_pui_particle(void) {
    struct ScriptRawArgs* args;
    char* owner;

    owner = get_script_string_arg(1);
    args = (struct ScriptRawArgs*)current_args;
    konquest_setup_pui_particle(owner, args->slots[1].i);
}

static void _npc_wait_for_wake_up(void) {
    npc_wait_for_wake_up();
}

static void _npc_set_wake_up_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_wake_up_time(args->slots[0].i, args->slots[1].i);
}

static void _save_hero_position_and_angle_prior_to_fight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    save_hero_position_and_angle_prior_to_fight(args->slots[0].f);
}

static void _npc_set_my_conversation_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_conversation_counter(args->slots[0].i);
}

static void _npc_set_his_conversation_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_conversation_counter(args->slots[0].pointer, args->slots[1].i);
}

static void _npc_set_my_punch_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_punch_counter(args->slots[0].i);
}

static void _npc_set_his_punch_counter(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_punch_counter(args->slots[0].pointer, args->slots[1].i);
}

static void _stop_chest_camera_script(void) {
    stop_chest_camera_script();
}

static void _konquest_run_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_run_camera_script(args->slots[0].pointer, args->slots[1].i);
}

static void _set_hero_punched_ground_collisions(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_hero_punched_ground_collisions(args->slots[0].i);
}

static void _restore_hero_grounding(void) {
    restore_hero_grounding();
}

static void _suspend_hero_grounding(void) {
    suspend_hero_grounding();
}

static void _start_hero_collisions(void) {
    start_hero_collisions();
}

static void _stop_hero_collisions(void) {
    stop_hero_collisions();
}

static void _add_hours_to_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_hours_to_time(args->slots[0].pointer, args->slots[1].i);
}

static void _add_days_to_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_days_to_time(args->slots[0].pointer, args->slots[1].i);
}

static void _add_months_to_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_months_to_time(args->slots[0].pointer, args->slots[1].i);
}

static void _add_years_to_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_years_to_time(args->slots[0].pointer, args->slots[1].i);
}

static void _pui_play_pfx_sequence(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_play_pfx_sequence(
        (KonquestPuiDefinition*)args->slots[0].i,
        args->slots[1].i,
        (KonquestPuiPfxSequenceRow*)args->slots[2].i);
}

static void _pui_play_pfx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_play_pfx(
        (KonquestPuiDefinition*)args->slots[0].i,
        args->slots[1].i,
        get_script_string_arg(3));
}

static void _pui_set_kill_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_set_kill_time(
        args->slots[0].pointer, args->slots[1].i,
        args->slots[2].i);
}

static void _pui_set_color(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_set_color(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i);
}

static void _pui_delay_spawn(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pui_delay_spawn(args->slots[0].pointer, args->slots[1].f);
}

static void _transition_to_region(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    transition_to_region(args->slots[0].i);
}

static void _get_previous_konquest_region_number(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_previous_konquest_region_number();
}

static void _set_konquest_region_number(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_region_number(args->slots[0].i);
}

static void _npc_play_teleported_sound(void) {
    npc_play_teleported_sound();
}

static void _konquest_hide_damashi(void) {
    konquest_hide_damashi();
}

static void _konquest_start_damashi(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        konquest_start_damashi(
            current_args, args->slots[0].f, args->slots[1].f,
            args->slots[2].f);
}

static void _konquest_camera_return_to_normal(void) {
    konquest_camera_return_to_normal();
}

static void _display_konquest_text(void) {
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

static void _hero_stop_moving(void) {
    hero_stop_moving();
}

static void _set_monk_age(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_monk_age(args->slots[0].i);
}

static void _get_monk_age(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_monk_age();
}

static void _change_monk_age(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    change_monk_age(args->slots[0].i);
}

static void _hero_handle_conversation(void) {
    hero_handle_conversation();
}

static void _nis_clear_event_list(void) {
    nis_clear_event_list();
}

static void _npc_switch_camera_focus(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_switch_camera_focus(args->slots[0].i);
}

static void _npc_at_waypoint_set_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_at_waypoint_set_flags(args->slots[0].i, args->slots[1].i);
}

static void _kill_dynamic_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kill_dynamic_pui((void*)args->slots[0].i);
}

static void _spawn_dynamic_pui_critical(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = spawn_dynamic_pui_critical(args->slots[0].pointer);
}

static void _spawn_dynamic_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = spawn_dynamic_pui(args->slots[0].pointer);
}

static void _kill_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    kill_pui((KonquestPuiDefinition*)args->slots[0].i);
}

static void _pickup_dynamic_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pickup_dynamic_pui((KonquestPuiDefinition*)args->slots[0].i);
}

static void _pickup_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    pickup_pui((KonquestPuiDefinition*)args->slots[0].i);
}

static void _spawn_pui(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    spawn_pui(
        (KonquestPuiDefinition*)args->slots[0].i,
        args->slots[1].i, args->slots[2].i);
}

static void _attach_pfx_to_object_by_uid(void) {
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

static void _add_trigger_list_to_world(void) {
    add_trigger_list_to_world();
}

static void _npc_sleep(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_sleep(args->slots[0].f);
}

static void _npc_get_punch_count(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_punch_count();
}

static void _npc_get_conversation_count(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_conversation_count();
}

static void _konquest_passed_last_mission(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = konquest_passed_last_mission();
}

static void _konquest_transition_object_to_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_transition_object_to_state(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _npc_fire_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_fire_trigger(args->slots[0].i);
}

static void _konquest_open_door(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_open_door(args->slots[0].i, args->slots[1].i);
}

static void _get_active_npc_data(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_active_npc_data();
}

static void _npc_glitch_him_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_glitch_him_to_ani(args->slots[0].pointer, args->slots[1].i, args->slots[2].i);
}

static void _npc_glitch_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_glitch_to_ani(args->slots[0].i, args->slots[1].i);
}

static void _npc_ani_1_frame(void) {
    npc_ani_1_frame();
}

static void _npc_set_random_dialog_and_anim_sequence(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_random_dialog_and_anim_sequence(args->slots[0].i, args->slots[1].i);
}

static void _npc_play_random_dialog_sequence(void) {
    npc_play_random_dialog_sequence();
}

static void _npc_play_dialog_and_anim_sequence(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_play_dialog_and_anim_sequence(args->slots[0].i, args->slots[1].i);
}

static void _play_beam_advance_sound(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    play_beam_advance_sound(args->slots[0].i);
}

static void _show_objective_arrow_and_beam(void) {
    show_objective_arrow_and_beam();
}

static void _hide_objective_arrow_and_beam(void) {
    hide_objective_arrow_and_beam();
}

static void _npc_attack(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_attack(args->slots[0].i, args->slots[1].i);
}

static void _npc_change_path_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_change_path_speed(args->slots[0].f);
}

static void _npc_set_pinanim_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_pinanim_flag(args->slots[0].i);
}

static void _npc_set_ani_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_ani_frame(args->slots[0].f);
}

static void _npc_set_my_ground_level(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_ground_level(args->slots[0].f);
}

static void _turn_to_face_interior_door(void) {
    turn_to_face_interior_door();
}

static void _turn_to_face_exterior_door(void) {
    turn_to_face_exterior_door();
}

static void _start_subobject_pulsing_effect(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_subobject_pulsing_effect(args->slots[0].i);
}

static void _resume_hero_state_process(void) {
    resume_hero_state_process();
}

static void _suspend_hero_state_process(void) {
    suspend_hero_state_process();
}

static void _idle_hero_anim_proc(void) {
    idle_hero_anim_proc();
}

static void _transition_hero_to_anim_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    transition_hero_to_anim_script(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].f, args->slots[3].f);
}

static void _close_exterior_doors(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    close_exterior_doors(args->slots[0].i, args->slots[1].i);
}

static void _get_doors_for_exterior(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_doors_for_exterior();
}

static void _get_building_id_for_exterior(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_building_id_for_exterior();
}

static void _npc_set_ani_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_ani_flags(args->slots[0].i);
}

static void _npc_blend_to_ani_string(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_blend_to_ani_string(args->slots[0].pointer);
}

static void _npc_set_gravity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_gravity(args->slots[0].f);
}

static void _remove_npc_list(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    remove_npc_list(args->slots[0].pointer);
}

static void _setup_interior_fighting_arena(void) {
    setup_interior_fighting_arena();
}

static void _npc_get_his_flag_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_his_flag_state(args->slots[0].pointer, args->slots[1].i);
}

static void _npc_get_flag_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = npc_get_flag_state(args->slots[0].i);
}

static void _npc_set_his_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_flags(args->slots[0].pointer, args->slots[1].i, args->slots[2].i);
}

static void _npc_ignore_his_events(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ignore_his_events(args->slots[0].pointer, args->slots[1].i);
}

static void _npc_ignore_events(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ignore_events(args->slots[0].i);
}

static void _npc_set_flags(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_flags(args->slots[0].i, args->slots[1].i);
}

static void _npc_enable_event(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_enable_event(args->slots[0].i, args->slots[1].i);
}

static void _npc_enable_his_event(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_enable_his_event(args->slots[0].pointer, args->slots[1].i, args->slots[2].i);
}

static void _npc_set_dialog_anim(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_dialog_anim(args->slots[0].i);
}

static void _npc_turn_and_face_next_waypoint(void) {
    npc_turn_and_face_next_waypoint();
}

static void _npc_turn_and_face_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_turn_and_face_player(args->slots[0].i);
}

static void _set_interaction_camera_script(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_interaction_camera_script(args->slots[0].pointer);
}

static void _npc_play_conversation_part(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_play_conversation_part(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _conversation_term(void) {
    conversation_term();
}

static void _conversation_init(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    conversation_init(args->slots[0].i);
}

static void _konquest_end_npc_interaction(void) {
    konquest_end_npc_interaction();
}

static void _konquest_end_npc_nis(void) {
    konquest_end_npc_nis();
}

static void _konquest_start_npc_nis(void) {
    konquest_start_npc_nis();
}

static void _start_character_separation_process(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    start_character_separation_process(args->slots[0].f);
}

static void _konquest_start_npc_interaction(void) {
    konquest_start_npc_interaction();
}

static void _remove_widescreen_bars(void) {
    remove_widescreen_bars();
}

static void _add_widescreen_bars(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_widescreen_bars(args->slots[0].f);
}

static void _npc_take_control_of_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_take_control_of_him(args->slots[0].pointer, args->slots[1].i);
}

static void _konquest_fade_hud(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_fade_hud(args->slots[0].i);
}

static void _konquest_hide_hud(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_hide_hud(args->slots[0].i);
}

static void _konquest_show_hud(void) {
    konquest_show_hud();
}

static void _display_konquest_title(void) {
    display_konquest_title();
}

static void _enable_attached_sound_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_attached_sound_by_uid(args->slots[0].i, args->slots[1].i);
}

static void _attach_sound_to_object_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    attach_sound_to_object_by_uid(
        args->slots[0].i, args->slots[1].i, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].i);
}

static void _start_konquest_ambient_sounds(void) {
    start_konquest_ambient_sounds();
}

static void _stop_konquest_ambient_sounds(void) {
    stop_konquest_ambient_sounds();
}

static void _npc_set_my_movement_weight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_movement_weight(args->slots[0].f, args->slots[1].f);
}

static void _npc_set_ani_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_ani_speed(args->slots[0].f);
}

static void _bgnd_start_sobj_uv_scroll_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bgnd_start_sobj_uv_scroll_tbl(
        (BgndUvScrollEntry*)args->slots[0].i);
}

static void _npc_set_snap_to_ground(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_snap_to_ground(args->slots[0].i);
}

static void _npc_stop_goro_bone_match(void) {
    npc_stop_goro_bone_match();
}

static void _npc_start_goro_bone_match(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_start_goro_bone_match(args->slots[0].pointer);
}

static void _npc_ani_for_x_ticks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ani_for_x_ticks(args->slots[0].i);
}

static void _npc_ani_to_blend_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ani_to_blend_frame(args->slots[0].f);
}

static void _start_time_passing(void) {
    start_time_passing();
}

static void _stop_time_passing(void) {
    stop_time_passing();
}

static void _get_tile_sobj_by_id(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_tile_sobj_by_id(args->slots[0].i);
}

static void _get_konquest_tile_objects_obj(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer =
        get_konquest_tile_objects_obj();
}

static void _get_current_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_current_time(args->slots[0].pointer);
}

static void _set_current_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_current_time(args->slots[0].pointer);
}

static void _npc_restart_his_normal_behavior(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_restart_his_normal_behavior(args->slots[0].pointer);
}

static void _npc_set_his_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_ang_y(args->slots[0].pointer, args->slots[1].f);
}

static void _npc_set_my_ang_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_ang_y(args->slots[0].f);
}

static void _npc_set_his_world_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_his_world_pos(args->slots[0].pointer, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _npc_set_my_world_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_world_pos(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _npc_set_my_pos(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_set_my_pos(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _restore_collision_volume_on_object_with_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    restore_collision_volume_on_object_with_uid(args->slots[0].i);
}

static void _remove_collision_volume_on_object_with_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    remove_collision_volume_on_object_with_uid(args->slots[0].i);
}

static void _restore_collision_volume_on_object(void) {
    restore_collision_volume_on_object();
}

static void _remove_collision_volume_on_object(void) {
    remove_collision_volume_on_object();
}

static void _npc_ani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_ani_to_frame_x(args->slots[0].f);
}

static void _konquest_transition_to_fight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_transition_to_fight(args->slots[0].i);
}

static void _attach_wiff_to_konquest_object_by_uid(void) {
    struct ScriptRawArgs* args;
    char* name;

    name = get_script_string_arg(2);
    args = (struct ScriptRawArgs*)current_args;
    attach_wiff_to_konquest_object_by_uid(
        args->slots[0].i, name, args->slots[2].f);
}

static void _set_konquest_object_render_order_priority_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_object_render_order_priority_by_uid(args->slots[0].i, args->slots[1].i);
}

static void _disable_konquest_object_zwrite_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_konquest_object_zwrite_by_uid(args->slots[0].i);
}

static void _set_konquest_object_face_y_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_konquest_object_face_y_by_uid(args->slots[0].i);
}

static void _unhide_konquest_object_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    unhide_konquest_object_by_uid(args->slots[0].i);
}

static void _hide_konquest_object_by_uid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hide_konquest_object_by_uid(args->slots[0].i);
}

static void _konquest_fade_from_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_fade_from_black(args->slots[0].i, args->slots[1].i);
}

static void _konquest_fade_to_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    konquest_fade_to_black(args->slots[0].i, args->slots[1].i);
}

static void _npc_lip_synch(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_lip_synch(args->slots[0].i, args->slots[1].pointer);
}

static void _npc_ani_to_end(void) {
    npc_ani_to_end();
}

static void _npc_wait_for_state_change(void) {
    npc_wait_for_state_change();
}

static void _npc_blend_to_ani_with_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_blend_to_ani_with_offset(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _npc_blend_to_ani(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_blend_to_ani(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _npc_face_current_waypoint_angle(void) {
    npc_face_current_waypoint_angle();
}

static void _npc_travel_to_world_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_travel_to_world_position(args->slots[0].pointer, args->slots[1].i);
}

static void _npc_travel_path_anim_override(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_travel_path_anim_override(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i);
}

static void _npc_travel_path(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    npc_travel_path(args->slots[0].i, args->slots[1].i, args->slots[2].i);
}

static void _npc_stand_still(void) {
    npc_stand_still();
}

static void _add_npc_list_to_world(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_npc_list_to_world(args->slots[0].pointer);
}

static void _fire_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fire_trigger(args->slots[0].pointer);
}

static void _set_monk_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_monk_position(args->slots[0].f, args->slots[1].f,
                      args->slots[2].f, args->slots[3].f);
}

static void _load_tile_objects(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    load_tile_objects(args->slots[0].pointer);
}

static void _trigger_set_time_for_enable(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    trigger_set_time_for_enable(
        args->slots[0].pointer,
        args->slots[1].i, args->slots[2].i,
        args->slots[3].i);
}

static void _enable_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    enable_trigger(args->slots[0].pointer, args->slots[1].i);
}

static void _assign_obj_to_trigger(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    assign_obj_to_trigger(
        args->slots[0].i, args->slots[1].pointer);
}

static void _add_object_to_tile(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_object_to_tile(
        args->slots[0].i, args->slots[1].i,
        args->slots[2].i, args->slots[3].f,
        args->slots[4].f, args->slots[5].f,
        args->slots[6].f);
}

static void _set_tile_visibility(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_tile_visibility(args->slots[0].i, args->slots[1].i);
}

static void _set_tile_grid_size(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_tile_grid_size(args->slots[0].i, args->slots[1].i);
}

static void _mks_start_axis_indicator_p_axis_track_bone_world_mat(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_start_axis_indicator_p_axis_track_bone_world_mat(args->slots[0].i, args->slots[1].f);
}

static void _mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs(void) {
    mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs();
}

static void _mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside(void) {
    mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside();
}

static void _mks_ccp1_insert_cb1(void) {
    mks_ccp1_insert_cb1();
}

static void _mks_cc1_insert_cb1(void) {
    mks_cc1_insert_cb1();
}

static void _mks_ccp1_eq_insert_cloth_coll_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_ccp1_eq_insert_cloth_coll_plane(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f, args->slots[4].f);
}

static void _mks_cc1_expand_cyl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cc1_expand_cyl(args->slots[0].f, args->slots[1].f);
}

static void _mks_cc1_eq_insert_cloth_coll(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cc1_eq_insert_cloth_coll(args->slots[0].i, args->slots[1].f);
}

static void _mks_cb1_set_scale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_scale(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _mks_cb1_set_coll_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_coll_offset(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _mks_cb1_set_coll_offset_xz(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_coll_offset_xz(args->slots[0].f, args->slots[1].f);
}

static void _mks_cb1_add_coll_pt(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_add_coll_pt(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _mks_cb1_set_ground_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_set_ground_y(args->slots[0].f);
}

static void _mks_set_ground_y_all_cloth_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_ground_y_all_cloth_bones(args->slots[0].f);
}

static void _mks_insert_cloth_force_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_insert_cloth_force_bones(args->slots[0].f, args->slots[1].f);
}

static void _mks_cb2_eq_cloth_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb2_eq_cloth_bone(args->slots[0].i);
}

static void _mks_cb1_eq_cloth_bone(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cb1_eq_cloth_bone(args->slots[0].i);
}

static void _mks_mat_id_set_zbias(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_mat_id_set_zbias(args->slots[0].i, args->slots[1].f);
}

static void _mks_cloth_bones_init_by_tbl(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_cloth_bones_init_by_tbl(args->slots[0].pointer, args->slots[1].i);
}

static void _mks_start_goro_arms_fixup(void) {
    mks_start_goro_arms_fixup();
}

static void _mks_set_flipped_bones(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    mks_set_flipped_bones(args->slots[0].pointer);
}

static void _is_he_blocking(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = is_he_blocking();
}

static void _set_ani_speed_miss_hit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_ani_speed_miss_hit(args->slots[0].f, args->slots[1].f);
}

static void _set_block_requirement(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_block_requirement(args->slots[0].i);
}

static void _special_move_cam_end(void) {
    special_move_cam_end();
}

static void _special_move_cam_setup(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    special_move_cam_setup(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i,
        args->slots[6].i, args->slots[7].i);
}

static void _whoosh_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    whoosh_fx(args->slots[0].i);
}

static void _drone_super_combo_refresh(void) {
    drone_super_combo_refresh();
}

static void _set_attackers_attack_region(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_attackers_attack_region(args->slots[0].i);
}

static void _set_attack_type(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_attack_type(args->slots[0].i);
}

static void _got_hit_fx(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    got_hit_fx(args->slots[0].i, args->slots[1].i, args->slots[2].i, args->slots[3].i, args->slots[4].i, args->slots[5].f, args->slots[6].i);
}

static void _camera_set_animation_parent_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_animation_parent_position(
        args->slots[0].pointer);
}

static void _camera_set_animation_parent_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_animation_parent_angle(
        args->slots[0].pointer, args->slots[1].i);
}

static void _cam_set_ground_plane(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    cam_set_ground_plane(args->slots[0].f);
}

static void _set_camera_velocity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_camera_velocity(args->slots[0].pointer);
}

static void _get_camera_velocity(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_camera_velocity(args->slots[0].pointer);
}

static void _set_camera_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_camera_angle(args->slots[0].pointer);
}

static void _get_camera_angle(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_camera_angle(args->slots[0].pointer);
}

static void _set_camera_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_camera_position(args->slots[0].pointer);
}

static void _get_camera_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    get_camera_position(args->slots[0].pointer);
}

static void _fade_from_white(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_from_white(args->slots[0].i, args->slots[1].i);
}

static void _fade_to_white(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_to_white(args->slots[0].i, args->slots[1].i);
}

static void _fade_from_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_from_black(args->slots[0].i, args->slots[1].i);
}

static void _fade_to_black(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    fade_to_black(args->slots[0].i, args->slots[1].i);
}

static void _turn_camera_off(void) {
    turn_camera_off();
}

static void _turn_camera_on(void) {
    turn_camera_on();
}

static void _camera_run_animation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_run_animation(args->slots[0].i);
}

static void _camera_set_speed_scalar(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_speed_scalar(args->slots[0].f);
}

static void _camera_get_victim(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = camera_get_victim();
}

static void _camera_set_victim(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_victim(args->slots[0].pointer);
}

static void _camera_get_attacker(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = camera_get_attacker();
}

static void _camera_set_attacker(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_attacker(args->slots[0].pointer);
}

static void _camera_special_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_special_function(args->slots[0].i);
}

static void _camera_setup_tightrope_angle_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_setup_tightrope_angle_offset(current_args, args->slots[0].f, args->slots[1].f);
}

static void _camera_setup_radial_position(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_setup_radial_position(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _camera_unpause_player(void) {
    camera_unpause_player();
}

static void _camera_pause_player(void) {
    camera_pause_player();
}

static void _camera_setup_radial_sweep(void) {
    struct ScriptRawArgs* args;
    float sp8;

    args = (struct ScriptRawArgs*)current_args;
    sp8 = args->slots[8].f;
    camera_setup_radial_sweep(current_args, args->slots[0].f, args->slots[1].f, args->slots[2].f, args->slots[3].f, args->slots[4].f, args->slots[5].f, args->slots[6].f, args->slots[7].f, sp8);
}

static void _camera_set_movement_focus_obj(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_focus_obj((MkObj*)args->slots[0].i);
}

static void _camera_set_custom_camera_movement_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_custom_camera_movement_flag(args->slots[0].i);
}

static void _camera_set_radial_movement(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_radial_movement(args->slots[0].i);
}

static void _camera_set_center_of_rotation(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_center_of_rotation(
        args->slots[0].pointer);
}

static void _camera_set_travel_time(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_travel_time(args->slots[0].f);
}

static void _camera_set_rotation_direction(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_rotation_direction(args->slots[0].i);
}

static void _camera_set_final_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_final_speed(args->slots[0].f);
}

static void _camera_set_initial_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_initial_speed(args->slots[0].f);
}

static void _camera_set_rotation_rate(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_rotation_rate(args->slots[0].f);
}

static void _camera_set_movement_rate(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_rate(args->slots[0].f);
}

static void _camera_get_mirror_flag(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = camera_get_mirror_flag();
}

static void _camera_set_check_konquest_collisions_flag(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_check_konquest_collisions_flag(args->slots[0].i);
}

static void _camera_set_glitch_flag(void) {
    camera_set_glitch_flag();
}

static void _camera_set_look_mode(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_look_mode(args->slots[0].i);
}

static void _camera_set_movement_mode(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_set_movement_mode(args->slots[0].i);
}

static void _camera_wait_for_pos_move_done(void) {
    camera_wait_for_pos_move_done();
}

static void _camera_wait_for_ang_move_done(void) {
    camera_wait_for_ang_move_done();
}

static void _camera_wait_for_pos_and_ang_move_done(void) {
    camera_wait_for_pos_and_ang_move_done();
}

static void _find_best_conversation_camera_position(void) {
    find_best_conversation_camera_position();
}

static void _camera_check_reverse_move_offset(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    camera_check_reverse_move_offset(args->slots[0].i, args->slots[1].i);
}

static void _camera_is_pos_move_done(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = camera_is_pos_move_done();
}

static void _camera_is_ang_move_done(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = camera_is_ang_move_done();
}

static void _camera_reset_ang_done_flag(void) {
    camera_reset_ang_done_flag();
}

static void _camera_reset_pos_done_flag(void) {
    camera_reset_pos_done_flag();
}

static void _init_scripted_camera(void) {
    init_scripted_camera();
}

static void _get_intro_camera_path(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.pointer = get_intro_camera_path();
}

static void _ani_to_frame_x_col(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_x_col(
        args->slots[0].f, args->slots[1].i, args->slots[2].f,
        args->slots[3].f, args->slots[4].i, args->slots[5].f,
        args->slots[6].i);
}

static void _if_collision_autoface_him(void) {
    if_collision_autoface_him();
}

static void _if_collision_autoface_me(void) {
    if_collision_autoface_me();
}

static void _bulvan_function(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    bulvan_function(args->slots[0].i);
}

static void _set_his_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_his_damage_multiplier(args->slots[0].f);
}

static void _adjust_his_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    adjust_his_damage_multiplier(args->slots[0].f);
}

static void _adjust_my_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    adjust_my_damage_multiplier(args->slots[0].f);
}

static void _advance_my_moveset(void) {
    advance_my_moveset();
}

static void _disable_my_attacks(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_my_attacks(args->slots[0].i);
}

static void _wall_eligible_off(void) {
    wall_eligible_off();
}

static void _wall_eligible_on(void) {
    wall_eligible_on();
}

static void _face_bleed_me(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    face_bleed_me(args->slots[0].i);
}

static void _air_collision_pause(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    air_collision_pause(args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

static void _player_impale(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    player_impale(args->slots[0].pointer, args->slots[1].pointer);
}

static void _plyr_weapon_grab(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    plyr_weapon_grab(args->slots[0].pointer, args->slots[1].pointer);
}

static void _weapon_trail_off(void) {
    weapon_trail_off();
}

static void _weapon_trail_on(void) {
    weapon_trail_on();
}

static void _set_ani_weight(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_ani_weight(args->slots[0].f);
}

static void _front_rollup_check(void) {
    front_rollup_check();
}

static void _disable_joy_temp(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_joy_temp(args->slots[0].i);
}

static void _disable_blocking(void) {
    disable_blocking();
}

static void _enable_his_blocking(void) {
    enable_his_blocking();
}

static void _match_my_ypos_with_his(void) {
    match_my_ypos_with_his();
}

static void _slamdown_reaction_max_hit_rules(void) {
    slamdown_reaction_max_hit_rules();
}

static void _popup_reaction_max_hit_rules(void) {
    popup_reaction_max_hit_rules();
}

static void _back_rollup_check_reverse(void) {
    back_rollup_check_reverse();
}

static void _back_rollup_check(void) {
    back_rollup_check();
}

static void _force_forward(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    force_forward(args->slots[0].f, args->slots[1].i,
                  args->slots[2].f, args->slots[3].i);
}

static void _ejb_call(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ejb_call(args->slots[0].i);
}

static void _myvel_my_angle_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    myvel_my_angle_y(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _rotate_towards_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    rotate_towards_him(args->slots[0].f);
}

static void _disable_both_repel_flags(void) {
    disable_both_repel_flags();
}

static void _wait_to_land(void) {
    wait_to_land();
}

static void _player_feet_land_chores(void) {
    player_feet_land_chores();
}

static void _set_my_float_1(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_float_1(args->slots[0].f);
}

static void _release_other_player(void) {
    release_other_player();
}

static void _ejb_too_close_repell(void) {
    ejb_too_close_repell();
}

static void _ejb_release_other_player(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ejb_release_other_player(args->slots[0].i);
}

static void _damage_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    damage_him(args->slots[0].f);
}

static void _damage_me(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    damage_me(args->slots[0].f);
}

static void _enable_all_my_blocking(void) {
    enable_all_my_blocking();
}

static void _turn_me_pi(void) {
    turn_me_pi();
}

static void _disable_this_move_exec(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_this_move_exec(args->slots[0].i, args->slots[1].i);
}

static void _set_both_face_opponent_flags(void) {
    set_both_face_opponent_flags();
}

static void _clear_both_face_opponent_flags(void) {
    clear_both_face_opponent_flags();
}

static void _clear_my_face_opponent_flag(void) {
    clear_my_face_opponent_flag();
}

static void _super_charge_me(void) {
    super_charge_me();
}

static void _step_throw_outof_retract(void) {
    step_throw_outof_retract();
}

static void _step_throw_into_check(void) {
    step_throw_into_check();
}

static void _add_facial_damage(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    add_facial_damage(args->slots[0].f);
}

static void _random_voice_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    random_voice_him(args->slots[0].i);
}

static void _set_my_damage_multiplier(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_damage_multiplier(args->slots[0].f);
}

static void _disable_attack5(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    disable_attack5(args->slots[0].i);
}

static void _special_move_cam_him(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    special_move_cam_him(
        args->slots[0].f, args->slots[1].f, args->slots[2].f,
        args->slots[3].f, args->slots[4].f, args->slots[5].i,
        args->slots[6].i, args->slots[7].i);
}

static void _plyr_rotate_obj_y180(void) {
    plyr_rotate_obj_y180();
}

static void _check_for_combo_message(void) {
    check_for_combo_message();
}

static void _myvel_his_angle_y(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    myvel_his_angle_y(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _stop_me(void) {
    stop_me();
}

static void _myvel_his_angle_y_inout(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    myvel_his_angle_y_inout(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _wait_for_slot_load(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    wait_for_slot_load(args->slots[0].i);
}

static void _get_mode_of_play(void) {
    ((struct ScriptRawResult*)active_cmdscript)->value.i = get_mode_of_play();
}

static void _destroy_mkprocs_pid(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    destroy_mkprocs_pid(args->slots[0].i);
}

static void _kill_lip_sync_procs(void) {
    kill_lip_sync_procs();
}

static void _plyr_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = plyr_snd_req(args->slots[0].i);
}

static void _random_hit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_hit(args->slots[0].i);
}

static void _random_hit_n_voice(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    random_hit_n_voice(args->slots[0].i, args->slots[1].i);
}

static void _shake_hit_voice(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    shake_hit_voice(args->slots[0].i, args->slots[1].f, args->slots[2].i, args->slots[3].i);
}

static void _pan_vol_pitch_random_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pan_vol_pitch_random_snd_req(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _snd_stop(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    snd_stop(args->slots[0].i);
}

static void _random_foot(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_foot(args->slots[0].i);
}

static void _random_voice(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_voice(args->slots[0].i);
}

static void _pan_vol_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pan_vol_snd_req(args->slots[0].i, args->slots[1].f, args->slots[2].f);
}

static void _pan_vol_pitch_random_hit(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = pan_vol_pitch_random_hit(args->slots[0].i, args->slots[1].f, args->slots[2].f, args->slots[3].f);
}

static void _random_snd_req_delay(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    random_snd_req_delay(args->slots[0].i, args->slots[1].i);
}

static void _random_snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = random_snd_req(args->slots[0].i);
}

static void _set_snd_vol(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_snd_vol(args->slots[0].i, args->slots[1].i, args->slots[2].f);
}

static void _snd_req_vol(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = snd_req_vol(args->slots[0].i, args->slots[1].f);
}

static void _snd_req(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ((struct ScriptRawResult*)active_cmdscript)->value.i = snd_req(args->slots[0].i);
}

static void _ani_to_fall_to_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_fall_to_frame(args->slots[0].f, args->slots[1].i, args->slots[2].f);
}

static void _shake_camera(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    shake_camera(args->slots[0].i, args->slots[1].f);
}

static void _force_away(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    force_away(args->slots[0].f, args->slots[1].i, args->slots[2].f, args->slots[3].i);
}

static void _play_sound_2(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    play_sound_2(args->slots[0].i, args->slots[1].i);
}

static void _ani_through_end(void) {
    ani_through_end();
}

static void _ani_to_end(void) {
    ani_to_end();
}

static void _face_opponent_180(void) {
    face_opponent_180();
}

static void _face_opponent_now(void) {
    face_opponent_now();
}

static void _hit_START_chores(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    hit_START_chores(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _land_chores(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    land_chores(args->slots[0].i, args->slots[1].i, args->slots[2].f, args->slots[3].f);
}

static void _launch_me_up(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    launch_me_up(args->slots[0].f, args->slots[1].f);
}

static void _tightrope_restrictions_off(void) {
    tightrope_restrictions_off();
}

static void _tightrope_restrictions_on(void) {
    tightrope_restrictions_on();
}

static void _init_3d_move_no_aniproc(void) {
    init_3d_move_no_aniproc();
}

static void _init_3d_move_no_face(void) {
    init_3d_move_no_face();
}

static void _init_3d_move(void) {
    init_3d_move();
}

static void _end_air_move(void) {
    end_air_move();
}

static void _init_air_move_no_aniproc(void) {
    init_air_move_no_aniproc();
}

static void _init_air_move(void) {
    init_air_move();
}

static void _init_still_move(void) {
    init_still_move();
}

static void _init_ground_move_no_aniproc(void) {
    init_ground_move_no_aniproc();
}

static void _init_ground_move(void) {
    init_ground_move();
}

static void _init_move(void) {
    init_move();
}

static void _set_ani_speed(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_ani_speed(args->slots[0].f);
}

static void _set_my_secondary_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_secondary_state(args->slots[0].i);
}

static void _set_my_state(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    set_my_state(args->slots[0].i);
}

static void _back_to_normal(void) {
    back_to_normal();
}

static void _if_collision_slow_ani_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    if_collision_slow_ani_x(args->slots[0].f, args->slots[1].f);
}

static void _slow_ani_end(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    slow_ani_end(args->slots[0].f);
}

static void _slow_ani_x_if_miss(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    slow_ani_x_if_miss(args->slots[0].f, args->slots[1].f, args->slots[2].f);
}

static void _slow_ani_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    slow_ani_x(args->slots[0].f, args->slots[1].f);
}

static void _ani_to_blend_frame(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_blend_frame(args->slots[0].f);
}

static void _ani_to_frame_x(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_x(args->slots[0].f);
}

static void _ani_to_frame_x_aniproc(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_x_aniproc(args->slots[0].f);
}

static void _ani_to_frame_sound(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    ani_to_frame_sound(args->slots[0].f, args->slots[1].f, args->slots[2].i);
}

static void _blend_to_stance(void) {
    struct ScriptRawArgs* args;

    args = (struct ScriptRawArgs*)current_args;
    blend_to_stance(args->slots[0].f);
}

ScriptEntryFn script_callable_function_table[1713] = {
    _script_sleep,
    _true_branch,
    _branch,
    _gosub,
    _print_s,
    _print_i,
    _print_f,
    _print_v,
    _script_return,
    _script_exit,
    _exit_float_int,
    _exit_6,
    _exit_react,
    _blend_to_ani,
    _disable_impale_check,
    _am_i_flipped,
    _am_i_flipped_or_turned,
    _is_he_flipped,
    _am_i_on_the_left,
    _am_i_on_the_left2,
    _is_my_chest_to_screen,
    _is_his_chest_to_screen,
    _am_i_blocking,
    _am_i_airborn,
    _was_button_pressed,
    _is_fast_getup,
    _two_player_animation,
    _two_player_animation_match_attacker,
    _two_player_animation_flip,
    _attack_to_frame_x,
    _lower_mines_ani_to_point,
    _launch_n_land_ani,
    _enable_grounding,
    _glitch_to_ani,
    _glitch_him_to_ani,
    _blend_to_ani_frame,
    _glitch_to_ani_frame,
    _blend_to_ani_inout,
    _set_player_movement_weight,
    _newani_to_frame_x,
    _set_player_step,
    _set_player_hiframe,
    _miss_branch,
    _block_branch,
    _hit_branch,
    _was_button_and_direction,
    _ani_col_abort,
    _was_i_hit_x_times,
    _true_branch_next_style,
    _true_branch_next_sidekick_style,
    _if_switching_to,
    _branch_next_style,
    _disable_grounding,
    _true_xfer_him,
    _camera_set_movement_offset,
    _camera_set_movement_offset_obj_rel,
    _camera_set_lookat_offset,
    _camera_set_lookat_offset_obj_rel,
    _camera_set_lookat_focus,
    _camera_set_movement_focus,
    _drone_combo,
    _drone_super_combo,
    _drone_xfer_him,
    _check_his_state,
    _set_background_color,
    _pz_fighter_attack,
    _pz_fighter_set_y_constrain,
    _pz_fighter_distance_check,
    _pz_fighter_distance_check_wo_super_check,
    _pz_fighter_startup_attack,
    _attack_opponent_with,
    _my_attack_hit,
    _pz_fighter_should_continue_move,
    _randu0,
    _exit_attack_with,
    _get_current_player_number,
    _set_move_pz_attributes_to,
    _get_plyr_pdata_flag,
    _camera_set_position,
    _camera_set_angle,
    _camera_set_destination,
    _camera_set_target,
    _camera_init_animation,
    _xfer_camera,
    _trial_add_required_attack,
    _trial_add_required_attack_nohit,
    _drone_blend_to_ani,
    _clear_fight_hud,
    _nis_init,
    _npc_set_anim_proc,
    _camera_setup_for_orbiting_npc,
    _camera_setup_for_tracking_npc,
    _camera_set_position_relative_to_npc,
    _camera_setup_for_custom_orbit_to_relative_point,
    _camera_set_lookat_focus_obj,
    _get_new_mkobj,
    _insert_item_obj,
    _init_item_obj,
    _ck_item_obj,
    _destroy_item_obj,
    _get_limb_obj,
    _get_obj_flag,
    _set_obj_flag,
    _update_mkobj,
    _get_bonematcher_flag,
    _set_bonematcher_flag,
    _anim_pdata_for_proc,
    _set_anim_script,
    _two_player_animation_blend,
    _transition_to_anim_script_frame,
    _xfer_proc,
    _plyr_spawn_his_anim_limb,
    _start_gusher,
    _obj_setup_for_animation,
    _animate_obj,
    _start_usec_timer,
    _stop_usec_timer,
    _script_hang,
    (ScriptEntryFn)p_track_cam_ang_y_light,
    (ScriptEntryFn)p_animate,
    (ScriptEntryFn)p_anim_idle,
    trial_start_tunes,
    trial_end_tunes,
    (ScriptEntryFn)p_idle,
    (ScriptEntryFn)subzero_his_tinkle_snd,
    _blend_to_stance,
    _ani_to_frame_sound,
    _ani_to_frame_x_aniproc,
    _ani_to_frame_x,
    _ani_to_blend_frame,
    _slow_ani_x,
    _slow_ani_x_if_miss,
    _slow_ani_end,
    _if_collision_slow_ani_x,
    _back_to_normal,
    _set_my_state,
    _set_my_secondary_state,
    _set_ani_speed,
    _init_move,
    _init_ground_move,
    _init_ground_move_no_aniproc,
    _init_still_move,
    _init_air_move,
    _init_air_move_no_aniproc,
    _end_air_move,
    _init_3d_move,
    _init_3d_move_no_face,
    _init_3d_move_no_aniproc,
    _tightrope_restrictions_on,
    _tightrope_restrictions_off,
    _launch_me_up,
    _land_chores,
    _hit_START_chores,
    _face_opponent_now,
    _face_opponent_180,
    _ani_to_end,
    _ani_through_end,
    _play_sound_2,
    _force_away,
    _shake_camera,
    _ani_to_fall_to_frame,
    _snd_req,
    _snd_req_vol,
    _set_snd_vol,
    _random_snd_req,
    _random_snd_req_delay,
    _pan_vol_pitch_random_hit,
    _pan_vol_snd_req,
    _random_voice,
    _random_foot,
    _snd_stop,
    _pan_vol_pitch_random_snd_req,
    _shake_hit_voice,
    _random_hit_n_voice,
    _random_hit,
    _plyr_snd_req,
    _kill_lip_sync_procs,
    _destroy_mkprocs_pid,
    _get_mode_of_play,
    _wait_for_slot_load,
    _myvel_his_angle_y_inout,
    _stop_me,
    _myvel_his_angle_y,
    _check_for_combo_message,
    _plyr_rotate_obj_y180,
    _special_move_cam_him,
    _disable_attack5,
    _set_my_damage_multiplier,
    _random_voice_him,
    _add_facial_damage,
    _step_throw_into_check,
    _step_throw_outof_retract,
    _super_charge_me,
    _clear_my_face_opponent_flag,
    _clear_both_face_opponent_flags,
    _set_both_face_opponent_flags,
    _disable_this_move_exec,
    _turn_me_pi,
    _enable_all_my_blocking,
    _damage_me,
    _damage_him,
    _ejb_release_other_player,
    _ejb_too_close_repell,
    _release_other_player,
    _set_my_float_1,
    _player_feet_land_chores,
    _wait_to_land,
    _disable_both_repel_flags,
    _rotate_towards_him,
    _myvel_my_angle_y,
    _ejb_call,
    _force_forward,
    _back_rollup_check,
    _back_rollup_check_reverse,
    _popup_reaction_max_hit_rules,
    _slamdown_reaction_max_hit_rules,
    _match_my_ypos_with_his,
    _enable_his_blocking,
    _disable_blocking,
    _disable_joy_temp,
    _front_rollup_check,
    _set_ani_weight,
    _weapon_trail_on,
    _weapon_trail_off,
    _plyr_weapon_grab,
    _player_impale,
    _air_collision_pause,
    _face_bleed_me,
    _wall_eligible_on,
    _wall_eligible_off,
    _disable_my_attacks,
    _advance_my_moveset,
    _adjust_my_damage_multiplier,
    _adjust_his_damage_multiplier,
    _set_his_damage_multiplier,
    _bulvan_function,
    _if_collision_autoface_me,
    _if_collision_autoface_him,
    _ani_to_frame_x_col,
    _get_intro_camera_path,
    _init_scripted_camera,
    _camera_reset_pos_done_flag,
    _camera_reset_ang_done_flag,
    _camera_is_ang_move_done,
    _camera_is_pos_move_done,
    _camera_check_reverse_move_offset,
    _find_best_conversation_camera_position,
    _camera_wait_for_pos_and_ang_move_done,
    _camera_wait_for_ang_move_done,
    _camera_wait_for_pos_move_done,
    _camera_set_movement_mode,
    _camera_set_look_mode,
    _camera_set_glitch_flag,
    _camera_set_check_konquest_collisions_flag,
    _camera_get_mirror_flag,
    _camera_set_movement_rate,
    _camera_set_rotation_rate,
    _camera_set_initial_speed,
    _camera_set_final_speed,
    _camera_set_rotation_direction,
    _camera_set_travel_time,
    _camera_set_center_of_rotation,
    _camera_set_radial_movement,
    _camera_set_custom_camera_movement_flag,
    _camera_set_movement_focus_obj,
    _camera_setup_radial_sweep,
    _camera_pause_player,
    _camera_unpause_player,
    _camera_setup_radial_position,
    _camera_setup_tightrope_angle_offset,
    _camera_special_function,
    _camera_set_attacker,
    _camera_get_attacker,
    _camera_set_victim,
    _camera_get_victim,
    _camera_set_speed_scalar,
    _camera_run_animation,
    _turn_camera_on,
    _turn_camera_off,
    _fade_to_black,
    _fade_from_black,
    _fade_to_white,
    _fade_from_white,
    _get_camera_position,
    _set_camera_position,
    _get_camera_angle,
    _set_camera_angle,
    _get_camera_velocity,
    _set_camera_velocity,
    _cam_set_ground_plane,
    _camera_set_animation_parent_angle,
    _camera_set_animation_parent_position,
    _got_hit_fx,
    _set_attack_type,
    _set_attackers_attack_region,
    _drone_super_combo_refresh,
    _whoosh_fx,
    _special_move_cam_setup,
    _special_move_cam_end,
    _set_block_requirement,
    _set_ani_speed_miss_hit,
    _is_he_blocking,
    _mks_set_flipped_bones,
    _mks_start_goro_arms_fixup,
    _mks_cloth_bones_init_by_tbl,
    _mks_mat_id_set_zbias,
    _mks_cb1_eq_cloth_bone,
    _mks_cb2_eq_cloth_bone,
    _mks_insert_cloth_force_bones,
    _mks_set_ground_y_all_cloth_bones,
    _mks_cb1_set_ground_y,
    _mks_cb1_add_coll_pt,
    _mks_cb1_set_coll_offset_xz,
    _mks_cb1_set_coll_offset,
    _mks_cb1_set_scale,
    _mks_cc1_eq_insert_cloth_coll,
    _mks_cc1_expand_cyl,
    _mks_ccp1_eq_insert_cloth_coll_plane,
    _mks_cc1_insert_cb1,
    _mks_ccp1_insert_cb1,
    _mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_inside,
    _mks_cc1_set_coll_fnc_eq_cloth_coll_point_cyl_abs,
    _mks_start_axis_indicator_p_axis_track_bone_world_mat,
    _set_tile_grid_size,
    _set_tile_visibility,
    _add_object_to_tile,
    _assign_obj_to_trigger,
    _enable_trigger,
    _trigger_set_time_for_enable,
    _load_tile_objects,
    _set_monk_position,
    _fire_trigger,
    _add_npc_list_to_world,
    _npc_stand_still,
    _npc_travel_path,
    _npc_travel_path_anim_override,
    _npc_travel_to_world_position,
    _npc_face_current_waypoint_angle,
    _npc_blend_to_ani,
    _npc_blend_to_ani_with_offset,
    _npc_wait_for_state_change,
    _npc_ani_to_end,
    _npc_lip_synch,
    _konquest_fade_to_black,
    _konquest_fade_from_black,
    _hide_konquest_object_by_uid,
    _unhide_konquest_object_by_uid,
    _set_konquest_object_face_y_by_uid,
    _disable_konquest_object_zwrite_by_uid,
    _set_konquest_object_render_order_priority_by_uid,
    _attach_wiff_to_konquest_object_by_uid,
    _konquest_transition_to_fight,
    _npc_ani_to_frame_x,
    _remove_collision_volume_on_object,
    _restore_collision_volume_on_object,
    _remove_collision_volume_on_object_with_uid,
    _restore_collision_volume_on_object_with_uid,
    _npc_set_my_pos,
    _npc_set_my_world_pos,
    _npc_set_his_world_pos,
    _npc_set_my_ang_y,
    _npc_set_his_ang_y,
    _npc_restart_his_normal_behavior,
    _set_current_time,
    _get_current_time,
    _get_konquest_tile_objects_obj,
    _get_tile_sobj_by_id,
    _stop_time_passing,
    _start_time_passing,
    _npc_ani_to_blend_frame,
    _npc_ani_for_x_ticks,
    _npc_start_goro_bone_match,
    _npc_stop_goro_bone_match,
    _npc_set_snap_to_ground,
    _bgnd_start_sobj_uv_scroll_tbl,
    _npc_set_ani_speed,
    _npc_set_my_movement_weight,
    _stop_konquest_ambient_sounds,
    _start_konquest_ambient_sounds,
    _attach_sound_to_object_by_uid,
    _enable_attached_sound_by_uid,
    _display_konquest_title,
    _konquest_show_hud,
    _konquest_hide_hud,
    _konquest_fade_hud,
    _npc_take_control_of_him,
    _add_widescreen_bars,
    _remove_widescreen_bars,
    _konquest_start_npc_interaction,
    _start_character_separation_process,
    _konquest_start_npc_nis,
    _konquest_end_npc_nis,
    _konquest_end_npc_interaction,
    _conversation_init,
    _conversation_term,
    _npc_play_conversation_part,
    _set_interaction_camera_script,
    _npc_turn_and_face_player,
    _npc_turn_and_face_next_waypoint,
    _npc_set_dialog_anim,
    _npc_enable_his_event,
    _npc_enable_event,
    _npc_set_flags,
    _npc_ignore_events,
    _npc_ignore_his_events,
    _npc_set_his_flags,
    _npc_get_flag_state,
    _npc_get_his_flag_state,
    _setup_interior_fighting_arena,
    _remove_npc_list,
    _npc_set_gravity,
    _npc_blend_to_ani_string,
    _npc_set_ani_flags,
    _get_building_id_for_exterior,
    _get_doors_for_exterior,
    _close_exterior_doors,
    _transition_hero_to_anim_script,
    _idle_hero_anim_proc,
    _suspend_hero_state_process,
    _resume_hero_state_process,
    _start_subobject_pulsing_effect,
    _turn_to_face_exterior_door,
    _turn_to_face_interior_door,
    _npc_set_my_ground_level,
    _npc_set_ani_frame,
    _npc_set_pinanim_flag,
    _npc_change_path_speed,
    _npc_attack,
    _hide_objective_arrow_and_beam,
    _show_objective_arrow_and_beam,
    _play_beam_advance_sound,
    _npc_play_dialog_and_anim_sequence,
    _npc_play_random_dialog_sequence,
    _npc_set_random_dialog_and_anim_sequence,
    _npc_ani_1_frame,
    _npc_glitch_to_ani,
    _npc_glitch_him_to_ani,
    _get_active_npc_data,
    _konquest_open_door,
    _npc_fire_trigger,
    _konquest_transition_object_to_state,
    _konquest_passed_last_mission,
    _npc_get_conversation_count,
    _npc_get_punch_count,
    _npc_sleep,
    _add_trigger_list_to_world,
    _attach_pfx_to_object_by_uid,
    _spawn_pui,
    _pickup_pui,
    _pickup_dynamic_pui,
    _kill_pui,
    _spawn_dynamic_pui,
    _spawn_dynamic_pui_critical,
    _kill_dynamic_pui,
    _npc_at_waypoint_set_flags,
    _npc_switch_camera_focus,
    _nis_clear_event_list,
    _hero_handle_conversation,
    _change_monk_age,
    _get_monk_age,
    _set_monk_age,
    _hero_stop_moving,
    _display_konquest_text,
    _konquest_camera_return_to_normal,
    _konquest_start_damashi,
    _konquest_hide_damashi,
    _npc_play_teleported_sound,
    _set_konquest_region_number,
    _get_previous_konquest_region_number,
    _transition_to_region,
    _pui_delay_spawn,
    _pui_set_color,
    _pui_set_kill_time,
    _pui_play_pfx,
    _pui_play_pfx_sequence,
    _add_years_to_time,
    _add_months_to_time,
    _add_days_to_time,
    _add_hours_to_time,
    _stop_hero_collisions,
    _start_hero_collisions,
    _suspend_hero_grounding,
    _restore_hero_grounding,
    _set_hero_punched_ground_collisions,
    _konquest_run_camera_script,
    _stop_chest_camera_script,
    _npc_set_his_punch_counter,
    _npc_set_my_punch_counter,
    _npc_set_his_conversation_counter,
    _npc_set_my_conversation_counter,
    _save_hero_position_and_angle_prior_to_fight,
    _npc_set_wake_up_time,
    _npc_wait_for_wake_up,
    _konquest_setup_pui_particle,
    _npc_reset_my_timed_events,
    _set_camera_to_look_at_hero,
    _nis_remove_non_participants,
    _setup_vomit_slip_sound,
    _npc_open_door_at_waypoint,
    _npc_assign_path,
    _npc_assign_path_to_him,
    _npc_assign_door_path,
    _npc_play_two_player_one_shot_anims,
    _give_reward_to_player,
    _give_krypt_key_to_player,
    _open_chest_and_unlock_kontent,
    _open_chest_and_give_item_to_player,
    _set_reference_pui,
    _get_pickup_object,
    _set_hero_position_relative_to_chest,
    _hero_start_fx_at_position,
    _npc_start_fx_at_position,
    _npc_start_fx_at_his_position,
    _load_krypt_character,
    _set_krypt_character_pos,
    _set_krypt_character_angle,
    _set_krypt_character_previous_root_angle,
    _set_krypt_character_anim_script,
    _transition_to_krypt_character_anim_script,
    _get_krypt_current_row,
    _get_krypt_current_column,
    _get_krypt_character_obj,
    _get_krypt_anim_pdata,
    _set_look_at_npc,
    _set_movement_npc,
    _interaction_cam_set_target_info,
    _get_hero_state,
    _npc_get_obj,
    _npc_start_blood_fall,
    _show_fight_message,
    _konquest_teleport_hero_to_location,
    _konquest_use_portal,
    _display_time_progression_images,
    _set_age_progression,
    _set_konquest_weather,
    _npc_sleep_until_model_loaded,
    _konquest_set_current_portal_uid,
    _konquest_hero_portal_in,
    _npc_show_skip_message,
    _npc_hide_skip_message,
    _attach_pfx_to_object,
    _hero_turn_to_face_position,
    _kill_konquest_dialog_procs,
    _jab_setup_kiss_emitter_obj,
    _jab_start_jade_boomerang_throw,
    _jab_release_jade_boomerang,
    _obj_scale_over_time,
    _jab_face_obj,
    _jab_attach_drink_obj_to_hand,
    _jab_destroy_drink_obj_in_hand,
    _jab_attach_wiff_to_sobj,
    _fade_fatality_screen,
    _jab_shake_dragon_king,
    _jab_stop_dragon_king_shake,
    _jab_flash_screen,
    _jab_attach_point_light_to_obj_bone,
    _jab_spawn_point_light_at_world_pos,
    _initialize_clone_lights,
    _konquest_map_setup_fight,
    _trial_set_num_rounds,
    _trial_set_round_timer,
    _trial_set_tick_function,
    _trial_set_ending_functions,
    _trial_show_text_window,
    _trial_show_spoken_text_window,
    _trial_set_type,
    _trial_add_success_condition,
    _trial_setup_onscreen_display_items,
    _trial_add_required_sequence,
    _trial_set_combo_requirement,
    _trial_set_next_setup_function,
    _trial_set_move_message,
    _trial_start_countdown,
    _trial_restart_round,
    _drone_set_switch_state,
    _drone_dispatch_switches,
    _drone_set_difficulty_level,
    _trial_do_dialog,
    _drone_set_script,
    _drone_face_monk,
    _drone_set_anim_step,
    _trial_setup_nis_scene,
    _trial_set_next_mission,
    _show_text,
    _trial_show_monk,
    _drone_lip_synch,
    _drone_change_to_style,
    _drone_do_special_move,
    _drone_set_health,
    _drone_set_position,
    _drone_set_special_directions,
    _drone_apply_damage,
    _current_player_is_drone,
    _trial_mirror_anims_if_needed,
    _camera_set_anim_aux_data,
    _trial_set_round_health_restoration,
    _konquest_run_ending,
    _give_koin_award,
    _play_background_music,
    _build_bones_tbl,
    _trial_get_background_root,
    _trial_debug_mission_list,
    _trial_state_collision_check,
    _credits_add_text,
    _start_constrain_proc,
    _release_kamidogu,
    _drone_set_damage_multiplier,
    _drone_start_bleeding,
    _drone_set_handicap,
    _trial_set_special_restrictions,
    _trial_invisible_callback,
    _trial_register_special_move,
    _show_shujinko_unlock_screen,
    _refresh_rate,
    _start_konquest_interior,
    _get_ir_cam_pos_x,
    _get_ir_cam_pos_y,
    _get_ir_cam_pos_z,
    _get_ir_cam_ang_x,
    _get_ir_cam_ang_y,
    _get_ir_cam_ang_z,
    _interior_exit_button_script,
    _npc_snap_to_face_monk,
    _npc_run_punch_animation,
    _npc_punch_reaction_check_data,
    _npc_get_collision_direction_in_script,
    _npc_punch_reaction_standard_setup,
    _npc_prepare_for_unconscious_state,
    _npc_punch_reaction_standard_shutdown,
    _npc_shove_reaction_standard_setup,
    _npc_shove_reaction_standard_shutdown,
    _npc_run_shove_animation,
    _bgnd_start_script_in_proc,
    _bgnd_start_script_in_proc_bigstack,
    _bgnd_delete_proc_by_id,
    _bgnd_get_camera_y_angle,
    _bgnd_get_camera_z_pos,
    _sh_lower_level_pebble_hide,
    _sh_lower_level_pebble_unhide,
    _bgnd_start_sobj_uv_scroll_w_control,
    _bgnd_destroy_sobj_uv_scroll_w_control,
    _bgnd_init_all_uv_scroll_w_control,
    _bgnd_set_sobj_uv_scroll_rate_values,
    _bgnd_set_sobj_uv_scroll_abs_values,
    _bgnd_hide_mirror_guys,
    _bgnd_unhide_mirror_guys,
    _nbc_script_debug_point,
    _bgnd_reset_players_animation_height,
    _bgnd_pfx_reset_effect,
    _bgnd_pfx_resume_effect,
    _bgnd_end_the_game_and_restart,
    _bgnd_set_player_shadow_ground_plane,
    _hf_bgnd_set_smasher_mode,
    _hf_bgnd_set_in_setup_zone,
    _get_pui_status,
    _set_pui_status,
    _set_konq_profile_value,
    _get_konq_profile_value,
    _set_last_character_trained_with,
    _get_last_character_trained_with,
    _add_to_konq_profile_value,
    _player_has_item,
    _player_remove_item_from_inventory,
    _player_add_item_to_inventory,
    _is_mini_mission_completed,
    _is_mini_mission_active,
    _is_mini_mission_started,
    _start_mini_mission,
    _mini_mission_completed,
    _mini_mission_inactive,
    _is_character_unlocked_in_profile,
    _bgnd_get_anim_info,
    _bgnd_get_obj_pointer,
    _set_active_projectile_block_script,
    _idle_his_anim_proc,
    _clear_his_f_constrained,
    _drone_ai_should_ermac_ground_slam,
    _drone_ai_should_ermac_fly_kick,
    _stop_vomit_slip_sound,
    _random_dk_foot,
    _kick_the_camera,
    _fire_spear_at_camera,
    _retract_spear_from_camera,
    _is_special_move_available,
    _get_plyr_pdata_plyr_num,
    _rd_set_impact_vector,
    _special_move_cam_setup2,
    _dk_taunt_at_screen,
    _dk_voice_call,
    _sidekick_switch_style_swap,
    _noob_victory_entrance,
    _smoke_victory_entrance,
    _noobsmoke_fire_projectile_request,
    _start_projectile_from_sidekick_bone,
    _forced_step_forward,
    _noobsmoke_sidekick_projectile,
    _noobsmoke_sidekick_double_charge,
    _plyr_snd_req_no_plyr_proc,
    _is_load_meter_active,
    _is_big_boss,
    _pebble_get_pos,
    _get_his_secondary_state,
    _get_his_previous_state,
    _freeze_player,
    _pz_fighter_create_space_between_fighters_for_special_moves,
    _pz_finish_him_request,
    _drone_ai_increase_big_boss_stage,
    _camera_idle,
    _obj_enable_grounding,
    _am_i_airborn_check_in_reaction,
    _run_camera_script,
    _obj_set_flipped_bones,
    _uv_my_angle_y,
    _get_kombat_difficulty,
    _general_flash_fx,
    _get_collision_result,
    _taunt_increase_life,
    _is_sidekick_active,
    _active_sidekick_swap_change_style,
    _active_sidekick_swap_from_sky,
    _active_sidekick_swap_from_behind,
    _plyr_invulnerable_to_projectiles,
    _show_player,
    _move_player_no_constrain_update,
    _face_ang_from_pos_to_him,
    _move_player,
    _plyr_get_f_constrained,
    _is_blood_disabled,
    _turn_into_energy_player,
    _unfreeze_player,
    _do_victory_camera,
    _get_my_plyr_num,
    _obj_set_z_offsets,
    _get_victory_flip_flags,
    _setup_for_flip_ani,
    _hide_player,
    _do_i_have_life_left,
    _bgnd_place_object_at_position,
    _bgnd_create_danger_zone,
    _bgnd_enable_danger_zone,
    _bgnd_set_danger_zone_center_position,
    _bgnd_set_danger_zone_width,
    _bgnd_set_danger_zone_depth,
    _bgnd_set_danger_zone_y_angle,
    _bgnd_set_viewing_of_danger_zones,
    _bgnd_set_danger_zone_radius,
    _bgnd_set_active_danger_zone,
    _bgnd_delete_danger_zone,
    _bgnd_place_point_light_for_ticks,
    _high_flash_check,
    _medium_flash_check,
    _low_flash_check,
    _launch_fx_at_pos_with_obj,
    _fight_fx_im_hit_flash,
    _flash_hit_at_bid,
    _flash_hit_at_bid_with_y,
    _get_my_particle_player_bank_num,
    _pz_fighter_register_move,
    _pz_fighter_exit,
    _pz_fighter_long_exit,
    _pz_fighter_check_breakout,
    _pz_fighter_step_throw_into_check,
    _pz_fighter_release_other_player,
    _pz_fighter_move_into_fighting_position,
    _pz_fighter_check_to_toggle_obj_and_ani_flips,
    _pz_fighter_dont_fudge_desired_distance,
    _pz_fighter_force_repel_during_attack,
    _pz_fighter_create_space_between_fighters,
    _pz_fighter_shaking,
    _snd_major_hit_voice,
    _suspend_in_midair,
    _pz_fighter_inline_force_away_with_ani,
    _start_baraka_jaw_monitor,
    _start_baraka_blades_monitor,
    _register_baraka_cb_functions,
    _sobj_no_zwrite,
    _bgnd_create_sobjs,
    _bgnd_start_sobj_uv_scroll,
    _bgnd_apply_zoffset,
    _bgnd_always_face_y,
    _plyr_start_script_in_proc,
    _plyr_start_script_in_plyr_pdata_proc,
    _frand,
    _bgnd_no_z_write,
    _bgnd_turn_on_backface_culling,
    _bgnd_pulsate_object,
    _bgnd_pulsate_object_with_caps,
    _bgnd_pulsate_object_with_caps_and_scale,
    _bgnd_replace_tex_with_wiff_and_ani,
    _pz_fighter_force_reaction_in_ticks,
    _ani_1_frame,
    _ani_loop_more_frames,
    _load_aux_weapon,
    _head_tracking_off,
    _head_tracking_on,
    _bgnd_hide_sobj,
    _bgnd_unhide_sobj,
    _bgnd_hide_sobj_and_children,
    _bgnd_unhide_sobj_and_children,
    _bgnd_sobj_get_x_pos,
    _bgnd_sobj_get_y_pos,
    _bgnd_sobj_get_z_pos,
    _bgnd_sobj_set_pos,
    _bgnd_sobj_set_pos_vel,
    _bgnd_sobj_get_ang,
    _bgnd_sobj_set_ang,
    _bgnd_sobj_set_rel_pos,
    _bgnd_sobj_set_alpha,
    _bgnd_sobj_set_ani_frame,
    _bgnd_sobj_set_ani_framerate,
    _obj_set_scale,
    _obj_get_scale,
    _sobj_set_alpha,
    _bgnd_preload_named_model,
    _bgnd_get_preload_obj,
    _pz_fighter_wipe_blood_off_hands,
    _bgnd_reg_col_cb_for_beetle_lair,
    _set_anim_hiframe,
    _pz_fighter_fetch_distance_to_center_pos,
    _pz_fighter_function,
    _pz_fighter_reaction_xfer_him,
    _bgnd_start_sh_fx,
    _bgnd_sh_level_1,
    _bgnd_sh_level_2,
    _bgnd_clean_slaughterhouse,
    _bgnd_place_weapon_at_position,
    _bgnd_set_new_ground_plane,
    _bgnd_set_wall_hide_distance,
    _bgnd_rotate_sobj,
    _bgnd_sobj_cam_volume_test_steer_over,
    _obj_sobj_cam_frustum_test_into_transparent,
    _bgnd_sobj_cam_frustum_test_into_transparent,
    _bgnd_turn_off_backface_culling,
    _bgnd_start_wall_hider,
    _bgnd_add_wall_to_unhide,
    _bgnd_add_wall_to_hide,
    _bgnd_remove_wall_from_hider,
    _bgnd_add_new_normal_check_for_hider,
    _pz_fighter_allow_continuation,
    _pz_fighter_disallow_continuation,
    _pz_fighter_reset_continuation,
    _pz_fighter_allow_easy_continuation,
    _start_bl_beetles_live_top_floor,
    _bgnd_clean_beetlelair,
    _bgnd_swap_level,
    _bgnd_register_danger_zone_callback,
    _bgnd_launch_plyr_up_and_forward,
    _bgnd_launch_plyr_up_and_forward_running,
    _dont_fence_plyr_in,
    _bgnd_restore_player,
    _bgnd_chunk_explosion_match_velocity_with_params,
    _bgnd_set_active_sobj,
    _bgnd_unhide_active_sobj,
    _bgnd_hide_active_sobj,
    _bgnd_rotate_xz_about_orgin_active_sobj,
    _bgnd_get_sobj_ang_y,
    _bgnd_fade_object,
    _bgnd_set_kill_plane_for_launched_sobj,
    _bgnd_set_collision_plane_for_launched_sobj,
    _bgnd_launch_sobj,
    _spad_set_vector,
    _spad_xz_dot_xz,
    _spad_rotate_xz_vector,
    _spad_norm_vector,
    _bgnd_takeover_plyr,
    _bgnd_process_collision_info,
    _spad_set_vector_y,
    _spad_scale_vector,
    _bgnd_set_sobj_launch_params,
    _float_to_int,
    _int_to_float,
    _bgnd_apply_active_sobj_pos_vel_drag,
    _bgnd_set_active_sobj_pos_vel,
    _bgnd_set_active_sobj_pos,
    _bgnd_set_active_sobj_scale,
    _bgnd_set_active_sobj_rop,
    _restart_effect,
    _bgnd_reset_sobj,
    _bgnd_make_displayed_item_pickupable_at_active_sobj_pos,
    _delete_obstacle_from_background_by_id,
    _bgnd_blood_control,
    _bgnd_process_active_sobj_info,
    _spad_set_vector_setting,
    _spad_set_y_angle_plus_offset_from_xz_vector,
    _bgnd_run_camera_script,
    _camera_set_lookat_offset_explicit,
    _bgnd_force_ground_to,
    _camera_set_movement_offset_explicit,
    _camera_get_pos,
    _cam_recalc_midpoint,
    _bgnd_allow_dirty_floor,
    _bgnd_clean_up_floor,
    _plyr_get_pos,
    _bgnd_place_crack_when_plyr_hits_ground,
    _bgnd_init_cracks,
    _bgnd_start_cracks,
    _bgnd_launch_fx_at_plyr_pos_and_y,
    _spad_add_vector,
    _sfrand,
    _spad_get_pos,
    _bgnd_xfer_attacker,
    _bgnd_act_at_time,
    _spad_sub_vectors,
    _bgnd_move_player,
    _bgnd_set_plyr_gravity,
    _bgnd_shadow_control,
    _bgnd_update_active_mksobj,
    _bgnd_set_launch_velocity_based_on_sobj_pos,
    _bgnd_force_plyr_ground_plane,
    _bgnd_set_fx_z_offset,
    _kill_shard_pfx_now,
    _allow_shard_pfx_now,
    _jump_towards_opponent_bgnd_transition,
    _bgnd_kill_all_launched_sobjs,
    _bgnd_start_timer,
    _bgnd_setup_rx_handler,
    _bgnd_current_rx_get_info,
    _bgnd_current_rx_set_info,
    _spad_xz_length_vector,
    _bgnd_timer_get_tick_count,
    _spad_xz_cos_two_vectors,
    _initialize_background_danger_zones,
    _bgnd_collison_if_to_scripts_activate,
    _bgnd_collison_if_set_return_result,
    _bgnd_collison_if_set_info,
    _bgnd_collison_if_monitor_col,
    _set_background_obstacle_repel_flag,
    _set_background_obstacle_disable_flag,
    _bgnd_collision_if_rx_override,
    _load_script_as_reaction,
    _bgnd_set_sobj_launch_params_exact,
    _bgnd_launch_fx_at_position,
    _bgnd_launch_fx_to_sobj,
    _pfxhandle_bgnd_spawn_at_position,
    _bgnd_set_active_sobj_in_obj,
    _bgnd_set_fx_ang_dir_to_i_vector,
    _bgnd_is_active_sobj_hidden,
    _build_sine_table_for_scripts,
    _bgnd_set_active_sobj_ang,
    _get_soul_sine,
    _get_game_speed,
    _bgnd_jtb_debug_info,
    _bgnd_pebble_launch_at_time,
    _bgnd_create_pebbles,
    _bgnd_create_pebbles_with_sobj,
    _ncs_create_pebbles_with_sobj,
    _ncs_set_pebble_pos,
    _ncs_create_pebble_monitor_proc,
    _get_general_pebble_data,
    _get_sobj_pebble_obj,
    _bgnd_pebble_gravity,
    _bgnd_init_pebbles,
    _bgnd_pebble_change_current_behavior,
    _bgnd_pebble_fetch_current_info,
    _bgnd_pebble_set_current_info,
    _rad_to_degrees,
    _degrees_to_rad,
    _spad_set_heading_vector_to,
    _bgnd_hide_pebbles,
    _bgnd_unhide_pebbles,
    _bgnd_pebble_burst_at_chunk_pos,
    _bgnd_pebble_rand_scale,
    _bgnd_remove_cracks,
    _bgnd_pebble_set_current_pebble,
    _bgnd_pebble_simple_launch_at_time,
    _bgnd_collision_if_disable_col,
    _bgnd_collision_if_enable_col,
    _nb_npc_slave_plyr_process_collision,
    _nb_place_slave_in_bgnd,
    _bgnd_pebble_burst_at_pos,
    _bgnd_pebble_burst_at_pebble_pos,
    _ani_no_pos,
    _ani_with_pos,
    _share_my_attack_info,
    _face_point,
    _bgnd_set_active_sobj_zoffset,
    _bgnd_active_sobj_no_zwrite,
    _bgnd_launch_fx_at_active_sobj_pos_with_offset,
    _bgnd_start_preload_sobj_uv_scroll,
    _bgnd_start_preload_sobj_morph,
    _bgnd_pebble_change_current_behavior_to_bounce,
    _bgnd_pebble_burst_set_value_min_max,
    _bgnd_pebble_burst_set_value,
    _bgnd_pebble_change_current_end_behavior,
    _bgnd_pebble_burst_set_end_state,
    _bgnd_collision_if_monitor_col_as,
    _cam_set_intro_cam_pause_ticks,
    _bgnd_launch_fx_at_plyr_bid,
    _bgnd_launch_fx_at_bid_of_mkobj,
    _bgnd_pfxhandle_spawn_at_bid,
    _bgnd_sobj_set_priority,
    _bgnd_launch_plyr_blood_fx,
    _bgnd_set_fx_ang_y,
    _bgnd_set_material_color,
    _snd_req_delay,
    _start_chunk_launch_monitor,
    _start_sobj_launch_monitor,
    _bgnd_get_exec_tick_ctr,
    _danger_zone_eligible_on,
    _bgnd_npc_set_pos,
    _bgnd_create_named_npc_in_slot,
    _bgnd_npc_set_scale,
    _bgnd_npc_like_plyr,
    _bgnd_npc_set_y_ang,
    _bgnd_add_scripted_brains_to_npc,
    _bgnd_npc_set_aux_int_data,
    _bgnd_npc_start_ani,
    _bgnd_npc_get_aux_int_data,
    _bgnd_npc_get_pos,
    _bgnd_npc_adjust_y_ang,
    _bgnd_npc_get_ang_y,
    _bgnd_npc_set_pos_vel,
    _bgnd_npc_set_pos_vel_heading,
    _bgnd_npc_set_pos_y,
    _bgnd_enable_wall_hider,
    _mk_chess_load_chess_table,
    _mk_chess_init_piece,
    _mk_chess_blend_to_ani,
    _mk_chess_ani_to_frame_x,
    _mk_chess_ani_1_frame,
    _mk_chess_ani_to_end,
    _mk_chess_ani_to_blend_frame,
    _mk_chess_ani_idle,
    _mk_chess_ani_loop_more_frames,
    _mk_chess_blend_into_cell_orgin_in_x_frames,
    _mk_chess_set_piece_type_as,
    _mk_chess_set_piece_state,
    _bgnd_get_active_sobj_pos,
    _bgnd_no_z_test,
    _bgnd_kill_fx,
    _mk_chess_set_normal_stance_script,
    _mk_chess_blend_to_normal_stance,
    _mk_chess_blend_to_ani_frame,
    _mk_chess_snap_to_stance,
    _mk_chess_check_snap_into_stance,
    _mk_chess_snap_into_cell_orgin_over_x_frames,
    _mk_chess_set_obj_move_weight,
    _mk_chess_blend_into_cell_orgin_in_x_frames_by_caller,
    _mk_chess_blend_to_desired_cell_position_setting,
    _mk_chess_piece_event_from_script,
    _mk_chess_place_special_cell_at,
    _bgnd_add_fx_to_hide,
    _mk_chess_set_ani_speed,
    _mk_chess_glitch_to_ani_frame,
    _mk_chess_set_glitch_stance_flag,
    _mk_chess_check_glitch_into_stance,
    _mk_chess_piece_match_y_ang_to_anim,
    _mk_chess_dont_constrain_piece,
    _mk_chess_snd_request,
    _mk_chess_wait_until_attack_cam_closes_in,
    _mk_chess_activate_my_properties,
    _mk_chess_define_class_initial_power,
    _mk_chess_add_movement_skill,
    _mk_chess_force_away,
    _mk_chess_set_cell_offset,
    _bgnd_active_sobj_no_ztest,
    _mk_chess_get_piece_event_data,
    _mk_chess_stop_me,
    _mk_chess_launch_n_land_ani_with_xz,
    _mk_chess_put_active_piece_at_cell,
    _mk_chess_piece_is_idle,
    _mk_chess_request_piece_script_for_action,
    _mk_chess_rotate_towards_cell,
    _mk_chess_ani_until_reached_destination,
    _mk_chess_deactivate_my_properties,
    _mk_chess_piece_test_and_set_timer,
    _mk_chess_set_piece_event_script,
    _mk_chess_make_spellcaster,
    _mk_chess_spell_has_completed,
    _mk_chess_spell_show_target_portrait,
    _mk_chess_spell_get_target_max_health,
    _mk_chess_spell_set_target_health,
    _mk_chess_spell_move_target_to_target,
    _mk_chess_spell_kill_target,
    _mk_chess_spell_is_this_a_forced_fight,
    _mk_chess_spell_force_fight,
    _mk_chess_spell_has_completed_but_wait_for_fight,
    _mk_chess_spell_target_add_access_restrictions,
    _mk_chess_spell_move_target_to_temp_area,
    _mk_chess_spell_move_target_from_temp_area_to,
    _mk_chess_spell_get_target_health,
    _mk_chess_launch_up,
    _mk_chess_piece_temporarily_gone,
    _mk_chess_get_piece_info,
    _mk_chess_spell_rescue_current_target,
    _mk_chess_launch_fx_at_active_piece_with_offset,
    _mk_chess_set_piece_info,
    _mk_chess_launch_special_fx,
    _mk_chess_piece_die,
    _mk_chess_xfer_piece_from_scripts,
    _mk_chess_snap_to_my_cell_now,
    _mk_chess_blend_to_my_cell_pos,
    _mk_chess_shifter_switch,
    _mk_chess_piece_set_state,
    _mk_chess_queue_up_piece_event,
    _courtyard_start_lensflare,
    _mk_chess_active_piece_near_edge,
    _mk_chess_air_move,
    _mk_chess_fetch_active_defined_teams_class,
    _mk_chess_fetch_active_defined_team,
    _bgnd_enable_obj_pos_and_ang_setting,
    _bgnd_fx_get_binded_obj,
    _mk_chess_fetch_bp_num_based_on_pchr_num,
    _get_taunts_performed,
    _increment_taunts_performed,
    _check_for_online_condition,
    _kill_ermac_eyes,
    _advance_my_sidekick_from_behind_with_moveset,
    _online_sync_reset,
    _remove_impaled_projectiles,
    _release_both_players,
    _is_drone,
    _adjust_kabal_position,
    _get_adjusted_speed,
    _player_area_collision_ticks,
    _flying_collision,
    _damage_player,
    _get_current_bgnd,
    _set_constrain_last_pos_pdata,
    _cloth_change_ground_plane_for,
    _gusher_destroy_list,
    _kabal_collision_control_victim,
    _switch_plyr_positions,
    _disable_mileena_collisions,
    _mileena_sky_set_position,
    _kobra_teleport_position,
    _start_kabal_smoke,
    _destroy_kabal_smoke,
    _get_cliff_data,
    _start_cliff_watcher,
    _clear_cliff_data,
    _get_cliff_watcher_round,
    _set_cliff_watcher_round,
    _get_exec_tick_ctr,
    _enable_no_adjustment_f,
    _enable_no_sync_anim_f,
    _set_remote_plyr_online_pos,
    _send_yinyang_evil_msg,
    _get_online_evil_state,
    _can_fallingcliff_fall,
    _jmt_debug_script,
    _start_subzero_decoy,
    _xfer_player_proc_to_script,
    _force_ai_style,
    _pan_vol_pitch_snd_req,
    _throw_spear,
    _is_reaction_xfer_him_allowed,
    _is_local_plyr,
    _kill_plyr_life,
    _local_collision_allowed_plyr_pdata,
    _update_my_last_switch,
    _start_shadow_watcher,
    _kenshi_teleport_position,
    _scorpion_teleport_position,
    _configure_iceball,
    _blast_effect_at_plyr,
    _start_scorpion_teleport_scale,
    _xz_distance_between_players,
    _start_special_weapon_monitor,
    _mks_bgnd_obj_enable_cloth_update,
    _mks_obj_enable_update_cloth,
    _auto_ani_on,
    _is_he_airborn,
    _single_frame_collision_check,
    _player_area_collision_check,
    _plyr_set_vel_xz_y,
    _plyr_get_anim_frame,
    _plyr_get_anim_hiframe,
    _animpdata_get_anim_hiframe,
    _plyr_scale_pos_vel,
    _plyr_set_gravity,
    _reaction_sync_advance,
    _wait_for_collision_result,
    _clear_collision_result,
    _collision_result_dont_care,
    _mks_cc1_set_coll_fnc_eq_cloth_coll_vector_cyl,
    _mks_debug_display_cloth_coll_cyl,
    _mks_debug_display_cloth_coll_plane,
    _mks_ccp1_eq_insert_cloth_coll_plane_4_pts_ave,
    _mks_set_cb1_target_bone_cb2,
    _load_bgnd_style,
    _bgnd_unhide_preload_obj,
    _bgnd_hide_preload_obj,
    _bgnd_preload_obj_attach_rope,
    _start_rope_proc,
    _bgnd_detach_rope,
    _bgnd_attach_rope_to_bgnd_obj,
    _mks_npc_start_cloth_bones,
    _mks_npc_cloth_bones_init_by_tbl,
    _mks_npc_set_target,
    _mks_npc_cc1_eq_insert_cloth_coll,
    _mks_npc_cb1_eq_cloth_bone,
    _mks_npc_set_ground_y_all_cloth_bones,
    _mks_npc_disable_ground_y_all_cloth_bones,
    _bgnd_npc_set_ani_speed,
    _ani_x_more_frames,
    _mks_get_victim_to_tr_dot,
    _get_projectile_script_plyr_num,
    _get_projectile_his_plyr_num,
    _get_projectile_script_plyr_pdata,
    _get_projectile_script_last_pos,
    _get_projectile_script_velocity,
    _set_active_projectile_velocity_to_hit_gnd,
    _set_active_projectile_2d_track,
    _set_active_projectile_3d_track,
    _set_active_projectile_continue_thru_hit,
    _set_active_projectile_tracking_light,
    _set_active_projectile_collision_info,
    _start_sobj_ctrl_proc,
    _destroy_sobj_ctrl_proc,
    _bgnd_insert_obj_ctrl_section,
    _bgnd_obj_insert_obj_ctrl_section,
    _bgnd_init_timers,
    _mks_set_sin_update_by_group,
    _mks_set_rotate_update_by_group,
    _mks_away_vel_update_by_group,
    _mks_gravity_update_by_group,
    _mks_shadow_scale,
    _mks_blend_start_update_by_group,
    _mks_removehide_by_group,
    _mks_set_update_delay,
    _mks_bgnd_pfx_bind_to_sobj,
    _mks_bgnd_cam_offset_away,
    _mks_set_plyr_to_center_ang_offset,
    _mks_plyr_stop,
    _mks_victim_bleed,
    _mks_start_gusher,
    _resume_effect_at_obj_bid,
    _resume_effect_at_plyr_num_bid,
    _mks_xfer_collision_info_plyr_to_script,
    _mks_xfer_collision_info_plyr_to_bgnd_script,
    _mks_xfer_plyr_to_STYLE_r_make_attacker_prone_in_stance,
    _mks_npc_build_bones_tbl,
    _mks_bgnd_start_wind,
    _mks_set_cb1_wind_normal,
    _reaction_xfer_him,
    _run_reaction_cleanup_function,
    _start_projectile_from_plyr_bone,
    _set_active_projectile_rx_info,
    _set_active_projectile_not_duckable,
    _set_active_projectile_impale_info,
    _set_active_projectile_hit_script,
    _set_active_projectile_end_script,
    _set_active_projectile_hit_gnd_script,
    _active_projectile_setup_done,
    _set_active_projectile_velocity,
    _set_active_projectile_target_ground,
    _set_active_projectile_p_handler,
    _set_active_projectile_max_ticks,
    _set_active_projectile_target_pos,
    _set_active_projectile_velocity_damp,
    _set_active_projectile_random_pos,
    _set_active_projectile_sound,
    _set_active_projectile_dn_sound,
    _set_active_projectile_random_rot,
    _set_active_add_ang_y,
    _set_active_projectile_upward_attack,
    _start_plyr_attack,
    _get_bid_with_flip,
    _start_bow,
    _mks_debug_display_cloth_ontop,
    _send_speedup_msg,
    _mks_start_fatality_iceball,
    _bgnd_get_int,
    _bgnd_get_u32,
    _bgnd_get_float,
    _bgnd_light_set_color,
    _bgnd_append_texture_to_material,
    _bgnd_append_texture_to_material_tbl,
    _bgnd_swap_textures,
    _bgnd_swap_textures_tbl,
    _bgnd_fetch_obj,
    _misc_data_get_col_obj_id,
    _misc_data_set_col_obj_id,
    _misc_data_get_col_obj_id2,
    _misc_data_set_col_obj_id2,
    _misc_data_get_obj_ptr1,
    _misc_data_set_obj_ptr1,
    _bgnd_get_first_shape_center_for_obstacle_id,
    _bgnd_launch_fx_at_sobj_pos,
    _scripted_camera_script_exit,
    _skytemple_set_fence_pebble_vel,
    _skytemple_arrange_fence_pebbles_around_pos,
    _mab_script_trace_func,
    _bgnd_clear_face_opponent_flags,
    _turn_controllers_off,
    _turn_controllers_on,
    _skytemple_make_scream_sound,
    _skytemple_player_explode,
    _set_far_clip_plane,
    _move_plyrs_to_round_start,
    _reset_severed_limbs,
    _pos_cam_for_current_level,
    _reset_collision_system,
    _delete_screen_obj_oid,
    _bgnd_sobj_start_morph,
    _random_percent,
    _bgnd_hide_sobj_list,
    _bgnd_unhide_sobj_list,
    _set_obj_pos,
    _set_obj_ang,
    _set_obj_light_flags,
    _script_fabs,
    _load_named_model_from_slot,
    _load_named_model_for_player,
    _get_player_number,
    _insert_fgnd_mkobj,
    _obj_set_gravity,
    _obj_turn_gravity_off,
    _destroy_mkobj,
    _pebble_set_pos,
    _pebble_set_vel,
    _pebble_set_scale,
    _pebble_set_ang,
    _pebble_set_ang_vel,
    _pebble_setup_bounce_props,
    _pebble_hide_me,
    _pebble_unhide_me,
    _pebble_turn_culling_on,
    _pebble_turn_culling_off,
    _obj_first_sobj,
    _sobj_disable_blending,
    _obj_set_sobj_priority,
    _misc_data_set_test_u32,
    _misc_data_set_test_float,
    _yinyang_start_lensflare,
    _yinyang_stop_lensflare,
    _obj_change_to_skinned_obj_light_list,
    _obj_change_to_bgnd_obj_light_list,
    _obj_add_to_skinned_obj_light_list_with_ambient,
    _do_yinyang_statue_explosion,
    _destroy_mkobjs_oid,
    _yinyang_make_fish_jump,
    _yinyang_set_good_fish_hide_flag,
    _yinyang_set_bad_fish_hide_flag,
    _bgnd_clear_danger_zone_callback,
    _player_body_explode,
    _mab_test,
    _get_pan_value,
    _get_volume_from_distance,
    _obj_for_all_atomics_set_material_alpha,
    _obj_set_sobj_alpha,
    _do_lightning_strike,
    _bgnd_level_transition_start,
    _bgnd_level_transition_end,
    _bgnd_level_fatality_start,
    _bgnd_level_fatality_end,
    _debug_create_axis_indicator,
    _turn_off_sobj_if_camera_is_in_cylinder,
    _turn_off_sobj_if_camera_is_in_rectangle,
    _hide_sobj_if_camera_is_in_cylinder,
    _hide_sobj_if_camera_is_in_rectangle,
    _is_timer_off,
    _start_fish_attack,
    _find_mkproc_pid,
    _get_offset_of_closest_fence_section,
    _is_plyr_airborn,
    _cut_player_in_half,
    _get_point_on_circle,
    _sobj_get_world_pos,
    _midpoint_v3,
    _cam_set_intro_cam_speed,
    _ending_show_image,
    _ending_show_text,
    _get_string_by_id,
    _are_death_traps_on,
    _bgnd_make_object_transl,
    _yy_is_evil_time_active,
    _set_evil_condition,
    _ok_to_do_evil_swap,
    _set_evil_swap_status,
    _fortress_setup_exclusion_zone,
    _is_point_in_fortress_exclusion_zone,
    _dist_v3_to_v3,
    _yinyang_play_good_tune,
    _yinyang_play_evil_tune,
    _yinyang_reset_music_index,
    _find_obj_by_id,
    _ck_put_weapon_away,
    _check_for_red_light,
    _check_for_green_light,
    _ck_rumble_controller,
    _get_inverse_game_speed,
    _ncs_script_debug_quickie,
    _pfx_plyr_bankowner,
    _hide_obj,
    _unhide_obj,
    _hide_sobj,
    _unhide_sobj,
    _sobj_set_priority,
    _hide_sobj_by_sobj_id,
    _unhide_sobj_by_sobj_id,
    _obj_create_sobjs_by_id,
    _obj_get_bid_for_tid,
    _mkobj_pos_pos_dot_normal_xz,
    _insert_particle_mkobj,
    _obj_match_obj_pos,
    _start_obj_scalar_proc,
    _obj_grnd_bounce,
    _fkbm_obj_face_obj,
    _retrieve_bgnd_obj,
    _ncs_bgnd_nuke_collision_to_script_interface,
    _ncs_bgnd_OBSTACLE_EVENT_get_plyr_pdata,
    _ncs_camera_wall_show_hide_alpha,
    _ncs_dkp_camera_konqchar_show_hide_alpha,
    _ncs_bgnd_preload_named_model,
    _auto_ani_off,
    _check_to_register_miss,
    _animpdata_ani_1_frame,
    _mks_animpdata_set_cur_frame,
    _animpdata_ani_to_frame_x,
    _animpdata_ani_loop_more_frames,
    _animpdata_ani_x_more_frames,
    _animpdata_ani_to_end,
    _animpdata_ani_to_end_at1,
    _animpdata_ani_to_blend_frame,
    _plyr_turn_off_mirrorguy,
    _plyr_turn_on_mirrorguy,
    _plyr_turn_off_shadowbox,
    _plyr_turn_on_shadowbox,
    _puzzle_fighter_scale,
    _set_pdata_anim_step,
    _proc_of_anim_pdata,
    _limb_sever_update_slide_end_coeff,
    _limb_sever_set_motion,
    _limb_sever_find_existing_update_proc,
    _mks_limb_sever,
    _limb_sever_pop_head_up,
    _limb_sever_bone_attach,
    _limb_sever_destroy_existing_attach_proc,
    _set_level_fatality_done_flag_state,
    _get_level_fatality_done_flag_state,
    _fatality_release_other_player,
    _get_game_state,
    _ft_fake_bone_matcher,
    _reset_fake_bone_matcher,
    _set_victim_v3_units_away,
    _fat_bgnd_char_setup_radius_check,
    _bm_force_fake_child_bid,
    _obj_hide_material_by_id,
    _obj_unhide_material_by_id,
    _play_his_random_voice,
    _play_his_snd_req,
    _sobj_get_bounding_sphere_radius,
    _sobj_set_bounding_sphere_radius,
    _am_i_female,
    _is_weapon_style,
    _advance_to_weapon_style,
    _clone_weapon_to_secondary,
    _clone_my_weapon,
    _show_single_weapon,
    _weapon_reflection_show_hide,
    _regrab_weapon,
    _weapon_bm_ignore,
    _bone_matcher_set_ang_pos,
    _bone_matcher_reset_dest_mat_rot,
    _plyr_weapon_release,
    _plyr_weapon2_release,
    _spawn_bld_splat,
    _spawn_blood_pool_at_bid,
    _mks_spawn_blood_pool_at_bid,
    _start_blood_particles,
    _start_sweat_particles,
    _start_blood_particles_scripts,
    _start_sweat_particles_scripts,
    _kill_gusher,
    _fatality_explode_victim,
    _load_cloth_boned_model,
    _start_bodyslam_bodysplat,
    _start_gore2_pebbles,
    _attach_gore2_obj,
    _destroy_gore2_obj,
    _reset_blood_decals,
    _limb_sever_explode_apart_plyr_num,
    _limb_sever_show_z_meat_chunks_all_plyr_num,
    _limb_sever_show_z_meat_chunks_all,
    _limb_sever_show_z_meat_chunks,
    _auto_calc_limbobj_bone_world_pos,
    _limb_sever_throw_away,
    _pfx_spawn_at_bid,
    _pfxhandle_spawn_at_bid_next,
    _pfxhandle_spawn_at_bid,
    _pfxhandle_bgnd_spawn_at_sobj_id,
    _pfxhandle_spawn_at_bid_next_bind_render,
    _fire_multi_emitter_pfx_via_tbl,
    _fatality_ashrah_get_doll,
    _show_baraka_one_blade_only,
    _fatality_boraicho_get_torch,
    _fatality_boraicho_light_fart_torch,
    _fatality_boraicho_get_jug,
    _fat_goro_fold_arms,
    _ft_mileena_start_veil_ripoff,
    _fix_axe_angle,
    _kill_raiden_summon_lightning_bolt,
    _ft_raiden_summon_lightning_bolt,
    _start_raiden_lightning_scroll,
    _sindel_sonic_waves,
    _sindel_sonic_sounds,
    _sindel_scream_react_sound_start,
    _subzero_start_iceblock,
    _subzero_start_iceman,
    _subzero_start_ice_chunks,
    _fire_sc_spear,
    _get_spearobj_from_spearproc,
    _insert_mkobj_spearproc_parentobjitem,
    _destroy_spearproc_bonematcher,
    _xfer_spearproc_to_retract,
    _kill_spear,
    _done_prison_grab_proc,
    _start_prison_grab_proc,
    _dkp_check_plyr_state_for_grab,
    _mkscripts_destroy_bonematcher,
    _mkscripts_destroy_fk_bonematcher,
    _mkscripts_destroy_gusher,
    _mkscripts_mkobj_insert_mkobj_cleanuplist,
    _mkscripts_set_anim_check_flag,
    _animpdata_ani_to_frame_x_with_flag_check,
    _get_fatality_state_ptr,
    _nis_signal_event,
    _nis_wait_for_event,
    _nis_end,
    _nis_scene_done,
    _nis_show_cancel_message,
    _nis_set_wait_override,
    _nis_register_participant,
    _wait_for_region_load,
    _nis_wait_for_region_load,
    _konquest_nis_init,
    _konquest_nis_end,
    _konquest_nis_anims_loaded,
    _konquest_start_nis_anims_load,
    _konquest_load_interior_art,
    _obj_set_ground_colls,
    _obj_set_ground_colls_y,
    _obj_set_pos,
    _obj_get_pos,
    _obj_set_ang,
    _obj_get_ang,
    _sobj_set_pos,
    _sobj_get_pos,
    _sobj_set_pos_vel,
    _sobj_get_pos_vel,
    _sobj_set_ang,
    _sobj_set_ang_vel,
    _sobj_get_ang_vel,
    _sobj_get_ang,
    _obj_set_light_flag,
    _obj_find_sobj_by_id,
    _get_plyr_info,
    _get_his_plyr_obj,
    _get_my_plyr_obj,
    _get_my_sidekick_obj,
    _get_plyr_obj_plyr_num,
    _get_his_plyr_pdata,
    _get_my_plyr_pdata,
    _get_my_plyr_anim_pdata,
    _plyr_anim_get_frame,
    _plyr_pdata_get_plyr_info,
    _plyr_pdata_get_his_plyr_pdata,
    _plyr_pdata_get_plyr_obj,
    _plyr_pdata_get_his_obj,
    _plyr_pdata_get_sidekick_obj,
    _plyr_pdata_get_cmo,
    _plyr_pdata_get_plyr_num,
    _plyr_pdata_sidekick_active,
    _plyr_pdata_get_pchr,
    _plyr_pdata_get_state,
    _plyr_pdata_get_previous_state,
    _plyr_pdata_is_alt_costume,
    _obj_get_pos_vel,
    _obj_set_pos_vel,
    _obj_set_ang_vel,
    _obj_get_ang_vel,
    _start_bone_matcher,
    _bone_matcher_parent_set_offset,
    _bone_matcher_child_set_offset,
    _get_bone_world_pos,
    _get_bone_offset_world_pos,
    _get_bone_relative_pos,
    _obj_set_sobj_pos,
    _force_calc_bone_world_mat,
    _plyr_in_spin_react,
    _mkobj_get_matrix,
    _mkobj_get_matrix_at,
    _mkobj_get_matrix_right,
    _mkobj_get_matrix_pos,
    _v3_x_mat,
    _v3_sub_v3,
    _v3_add_v3,
    _v3_add_v3_scaled,
    _scale_v3,
    _normalize_v3,
    _zero_v3,
    _v3_dot_v3,
    _xz_unit_vector,
    _xz_to_y_ang,
    _uv_from_angle_y,
    _obj_get_bone_rot_quat,
    _YXZ_angles_to_quat,
    _xz_dot_xz,
    _uv_v3_to_v3_dist,
    _gxMathCos,
    _gxMathSin,
    _gxMathArcCos,
    _rotate_xz,
    _length_v3,
    _v3_to_xy_ang,
    _v3_to_xz_ang,
    _dist_xz_to_xz,
    _parametric_update,
    _create_step_effect,
    _bind_to_bone,
    _create_step_fx,
    _create_multiemit_step_fx,
    _create_parametric_fx,
    _create_multiemit_parametric_fx,
    _set_decal_plane,
    _face_y,
    _particle_size,
    _z_bias,
    _create_y_mirror_effect,
    _set_bounding_radius,
    _set_aspect_ratio,
    _set_light,
    _set_vertex_color,
    _fx,
    _fx2,
    _fx_by_owner,
    _emit_in_range,
    _emit_color,
    _emit_uv,
    _emit_from_point,
    _emit_cuboid,
    _emit_disc,
    _emit_disc2,
    _emit_cartesian,
    _emit_cylindrical,
    _emission_duration,
    _texture_animation,
    _texture_animation_with_vsize,
    _emit_spherical,
    _emit_spherical_from_boundary,
    _emit_spherical_section,
    _emit_from_pos,
    _emit_from_pos_clamp_y,
    _emit_value,
    _emit_value_i,
    _emit_roundrobin_mechanism,
    _emit_constant_rate,
    _kill_at_plane,
    _set_rotation,
    _set_drag_coefficient,
    _set_growth_coefficient,
    _spawn_random_size,
    _spawn_color,
    _resume_effect,
    _reset_effect,
    _fx_restart_emit,
    _fx_reset,
    _fx_next_emitter,
    _fx_resume_emit,
    _fx_pause_emit,
    _fx_reset_emit,
    _set_cycle_length,
    _set_cycle_emission,
    _initial_reflect,
    _initial_divert,
    _initial_add_v3,
    _initial_set_float,
    _initial_multiply_float,
    _update_add,
    _update_copy,
    _update_add_constant_v3,
    _update_add_constant,
    _update_wrapbox,
    _update_mul_scalar,
    _update_fade_alpha,
    _update_fade_alpha2,
    _update_lerp_color,
    _update_texanim,
    _update_texanim_hold,
    _update_bounce,
    _update_attract,
    _update_assign,
    _udpate_roundrobin,
    _kill_on_greater,
    _kill_percent,
    _kill_roundrobin,
    _change_on_greater,
    _change_on_less,
    _change_on_y_less,
    _change_on_y_less_than_field,
    _kill_on_y_less_than_field,
    _enable_profiling,
    _fx_set_param_v3,
    _fx_set,
    _fx_get_v3,
    _fx_hide,
    _fx_set_render_priority,
    _fx_disable_ztest,
    _fx_bind_render_to_obj_bone,
    _fx_bind_emitter_to_obj_bone,
    _fx_bind_render_to_sobj,
    _fxsys_set_v3,
    _fxsys_set,
    _debug_print_message,
    _bgnd_sobj_set_texture_kl_values,
    _bgnd_force_specularity_off_for_material,
    _fx_transfer,
    _get_active_moveset_from_pdata,
    _advance_active_moveset,
    _load_and_set_refl_on_weapon,
    _tag_team_activate_player
};

int number_of_script_functions = 1713;
