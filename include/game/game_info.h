#ifndef MKD_GAME_INFO_H
#define MKD_GAME_INFO_H

#include "game/bgnd_types.h"
typedef struct SwitchMapEntry SwitchMapEntry;
#include "math/gxVect.h"
#include "runtime/plyr_info.h"
#include "runtime/mk_struct.h"

typedef struct MkObj MkObj;
typedef struct MkProc MkProc;
typedef struct PlyrPdata PlyrPdata;
typedef struct ScriptSlot ScriptSlot;

typedef struct SkyMkobj {
    char pad00[0x18];
    void* clump;
} SkyMkobj;

typedef struct GcPadFlags {
    unsigned char disabled : 1;
    unsigned char connected : 1;
    unsigned char stick_dispatch : 1;
    unsigned char pad : 5;
} GcPadFlags;

typedef struct GcPadSlot {
    union {
        unsigned int flags_word;
        GcPadFlags flag_bits;
        struct {
            unsigned char flags;
            unsigned char pad_01[3];
        };
    };
    SwitchMapEntry* switch_map;
    PlyrInfo* player;
    unsigned int prev_buttons;
    unsigned int buttons;
    unsigned int edge;
    union {
        unsigned int stick_pack;
        struct {
            unsigned char stick_axis_0;
            unsigned char stick_axis_1;
            unsigned char stick_axis_2;
            unsigned char stick_axis_3;
        };
    };
} GcPadSlot;

typedef struct GamePauseFlags {
    unsigned char fatality_window : 1;
    unsigned char pad_bit6 : 1;
    unsigned char ladder_complete : 1;
    unsigned char rumble_stopped_for_pause : 1;
    unsigned char shared_hand_anims_loaded : 1;
    unsigned char controller_disable_guard : 1;
    unsigned char controllers_disabled : 1;
    unsigned char paused : 1;
} GamePauseFlags;

typedef struct GameSwitchInputFlags {
    unsigned char hide_obstacles : 1;
    unsigned char hide_konquest_collision : 1;
    unsigned char field_bit5 : 1;
    unsigned char view_danger_zones : 1;
    unsigned char eat_switches : 1;
    unsigned char pad_2_0 : 3;
} GameSwitchInputFlags;

typedef struct GameFeatureFlagBits {
    unsigned char online_game : 1;
    unsigned char field_bit6 : 1;
    unsigned char powerbars_locked : 1;
    unsigned char pad_4_0 : 5;
} GameFeatureFlagBits;

typedef union GameFeatureFlags {
    unsigned char raw;
    GameFeatureFlagBits bits;
} GameFeatureFlags;

typedef struct BgndWallHiderRuntime BgndWallHiderRuntime;

typedef struct GameInfoFlags {
    unsigned int high_res_path : 1;
    unsigned int field_bit6 : 1;
    unsigned int lens_flare_enabled : 1;
    unsigned int level_transition_active : 1;
    unsigned int level_fatality_active : 1;
    unsigned int level_fatality_done : 1;
    unsigned int load_complete : 1;
    unsigned int field_bit0 : 1;
} GameInfoFlags;

#define GC_PAD_FLAG_DISABLED 0x80
#define GC_PAD_FLAG_CONNECTED 0x40
#define GC_PAD_FLAG_STICK_DISPATCH 0x20
#define GC_PAD_SLOT_STRIDE 0x1C
#define GC_PAD_SLOTS_OFF 0x17C

typedef struct GameInfoPselectTail {
    unsigned int field_1d0;
    int field_1d4;
    int field_1d8;
    int field_1dc;
    int field_1e0;
    int field_1e4;
    int field_1e8;
    int field_1ec;
    int field_1f0;
    int field_1f4;
} GameInfoPselectTail;

typedef struct BgndWallHiderData {
    MkHdr hdr;
    MkPtr* walls;
    float hide_distance;
    union {
        unsigned int flags;
        struct {
            union {
                unsigned char flags_byte;
                struct {
                    unsigned char disabled : 1;
                    unsigned char kill_process : 1;
                    unsigned char pad_low : 6;
                } flag_bits;
            };
            char pad11[3];
        };
    };
    BgndWallHiderRuntime* runtime;
} BgndWallHiderData;

typedef struct GameInfo {
    union {
        GameInfoFlags flag_bits;
        struct {
            unsigned char flags;
            union {
                unsigned char pause_flags;
                GamePauseFlags pause_flag_bits;
            };
            GameSwitchInputFlags switch_input_flags;
            char pad03;
        }
#ifdef __cplusplus
        flag_bytes
#endif
        ;
    };
    union {
        unsigned int feature_flags_word;
        GameFeatureFlags feature_flags;
    };
    int field_08;
    BgndDataTable* section;
    BgndMisc* misc;
    void* mode_table;
    int bgnd_id;
    int active_level;
    ScriptSlot* cmdscript;
    int bgnd_cycle_index;
    char pad28[4];
    MkObj* bgnd_obj;
    MkObj* sky;
    float field_34;
    Vec impact_vector;
    PlyrInfo* active_player;
    PlyrInfo* collision_player_info;
    MkObj* player_objects[2];
    int collision_player_side;
    PlyrPdata* collision_player_pdata;
    unsigned int collision_event_id;
    MkPtr* displayed_items;
    MkPtr* field_64;
    BgndWallHiderData* wall_hider;
    MkProc* camera_proc;
    unsigned int camera_proc_instance;
    union {
        int field_74;
        struct {
            unsigned char clean_floor : 1;
            unsigned char field_74_bit6 : 1;
            unsigned char field_74_pad_bits : 6;
            unsigned char field_75_77[3];
        } floor_flags;
        struct {
            unsigned char blood_pad_high : 2;
            unsigned char blood_enabled : 1;
            unsigned char blood_pad_low : 5;
            unsigned char blood_pad_75_77[3];
        } blood_flags;
    };
    int field_78;
    unsigned int bgnd_timer_ticks[3];
    unsigned int bgnd_timer_limits[3];
    MkPtr* npc_list;
    float crack_count;
    char pad9C[8];
    union {
        struct {
            PlyrInfo plyr0;
            PlyrInfo plyr1;
        };
        PlyrInfo players[2];
    };
    union {
        GcPadSlot pads[4];
        struct {
            GcPadSlot first_three_pads[3];
            GameInfoPselectTail pselect;
        } pad_overlay;
        struct {
            GcPadSlot first_three_pads[3];
            GameInfoPselectTail pselect;
        };
    };
    int field_1F8;
    int field_1FC;
    int field_200;
    int field_204;
    int field_208;
    int field_20C;
    int field_210;
    int field_214;
    int field_218;
} GameInfo;

typedef char GameInfoSizeCheck[sizeof(GameInfo) == 0x21C ? 1 : -1];

extern GameInfo g_game_info;

#endif
