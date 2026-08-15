#ifndef MKD_PLYR_INFO_H
#define MKD_PLYR_INFO_H

/*
 * Per-port player info in g_game_info (stride 0x6C @ +0xA4 / +0x110).
 * init_plyr_info_struct / set_player_state / plyr_pdata_get_plyr_info.
 */

typedef struct MkObj MkObj;
typedef struct Vec Vec;
typedef struct PlayerCollisionData PlayerCollisionData;
typedef struct ScriptSlot ScriptSlot;
typedef struct FighterAiTableContainer FighterAiTableContainer;
typedef struct FighterRuntimeData FighterRuntimeData;
typedef struct MkProc MkProc;
typedef struct MkPtr MkPtr;

typedef struct LinkedNode {
    void* data;
    void* link; /* +0x04 - owner / next sentinel */
} LinkedNode;

typedef struct MirrorObj {
    void* field00;
    union {
        void* field04;
        int owner_index; /* +0x04 - blood decal effect owner */
    };
} MirrorObj;

/* Width source for movelist style icon centering (style_obj+4). */
typedef struct FighterStyleLayout {
    char pad00[0x10];
    int width; /* +0x10 */
} FighterStyleLayout;

/*
 * ScreenObj-shaped latch on FighterStyleObj (+0x6C / +0x70).
 * pfx2d @ +0x34 matches ScreenObj.pfx2d (Pfx2dObj*).
 */
typedef struct FighterStyleScreen {
    void* vtbl;            /* +0x00 */
    unsigned int instance; /* +0x04 */
    char pad08[0x2C];
    void* pfx2d; /* +0x34 - Pfx2dObj* */
} FighterStyleScreen;

/*
 * Per-style UI parent on FighterMirror.style_objs[].
 * screen is a validated ScreenObj latch (+0x6C / +0x70).
 */
typedef struct FighterStyleObj {
    char pad00[4];
    FighterStyleLayout* layout; /* +0x04 */
    char pad08[0x64];
    FighterStyleScreen* screen;      /* +0x6C */
    unsigned int screen_inst;        /* +0x70 */
} FighterStyleObj;

typedef struct FighterObjectRef {
    MkObj* object;
    unsigned int instance;
} FighterObjectRef;

typedef struct PlyrScreenLatch {
    void* object;
    unsigned int instance;
} PlyrScreenLatch;

typedef struct PlyrFightingLightState {
    union {
        unsigned int flags_word;
        struct {
            unsigned char red_active : 1;
            unsigned char green_active : 1;
            unsigned char airborne_active : 1;
            unsigned char green_trigger : 1;
            unsigned char pad_flags : 4;
            unsigned char flags_pad[3];
        };
    };
    PlyrScreenLatch base;
    PlyrScreenLatch red;
    PlyrScreenLatch green;
    PlyrScreenLatch airborne;
} PlyrFightingLightState; /* 0x24 */

typedef struct FighterMirror {
    char pad00[0x18];
    MirrorObj* blood_owner;            /* +0x18 - owner index source */
    char pad1C[0x14];
    MkObj* shadow_obj;              /* +0x30 - validated shadow owner */
    unsigned int shadow_obj_instance; /* +0x34 */
    char pad38[0x24];
    MkProc* anim_proc;                  /* +0x5C */
    unsigned int anim_proc_instance;    /* +0x60 */
    char pad64[0xB8];
    MkProc* foot_print_proc;             /* +0x11C */
    unsigned int foot_print_proc_instance; /* +0x120 */
    char pad124[0x14];
    MkProc* limb_update_proc;             /* +0x138 */
    unsigned int limb_update_proc_instance; /* +0x13C */
    MkPtr* attach_proc_list;              /* +0x140 - limb attachment processes */
    union {
        FighterObjectRef severed_limbs[15]; /* +0x144 */
        struct {
            char pad144[0x70];
            MkObj* severed_half_obj;            /* +0x1B4 */
            unsigned int severed_half_instance; /* +0x1B8 */
        };
    };
    char pad1BC[0x10C];
    int field_2C8; /* +0x2C8 - set when a trial changes drone player state */
    char pad2CC[0x30];
    FighterStyleObj* style_objs[3]; /* +0x2FC - movelist style pfx parents */
    int style_idx;                  /* +0x308 - movelist starting style */
    int active_moveset;             /* +0x30C - current style/moveset */
    char pad310[0x160];
    MkObj* flag_obj; /* +0x470 - hide_flags @ +0x0A */
    char pad474[4];
    ScriptSlot* cmo; /* +0x478 */
    char pad47C[0x248];
    float facial_damage; /* +0x6C4 */
    char pad6C8[0x30];
    union {
        FighterAiTableContainer* ai_tables;
        FighterRuntimeData* runtime_data;
        void* move_table_container;
    }; /* +0x6F8 */
    char pad6FC[0x40];
    int limb_material_bank; /* +0x73C - nonzero selects material ids +0x400 */
} FighterMirror;

struct FighterRuntimeData {
    char pad00[0x6C];
    int win_sound_id; /* +0x6C */
    char pad70[0x3C];
    Vec* half_sever_velocities; /* +0xAC */
};

/* global_player_data[] stride 0x10 (movelist_get_character_name). */
typedef struct GlobalPlayerEntry {
    char* name; /* +0x00 */
    char pad04[0xC];
} GlobalPlayerEntry; /* 0x10 */

typedef struct MoveTableContainer {
    char pad00[0xB8];
    void* move_table; /* +0xB8 - rows stride 0x14 */
} MoveTableContainer;

typedef struct FighterAiMoveRow {
    int move_id;
    char pad04[0x3C];
} FighterAiMoveRow; /* 0x40 */

typedef struct FighterAiTable {
    int usable_row_count;
    FighterAiMoveRow* rows;
} FighterAiTable; /* 0x08 */

typedef struct FighterAiTableContainer {
    char pad00[0xBC];
    FighterAiTable tables[14];
} FighterAiTableContainer;

typedef struct FighterSlot {
    union {
        FighterMirror* fighter;
        struct PlyrPdata* pdata;
    };                      /* +0x00 */
    MkObj* mirror_a;        /* +0x04 */
    MirrorObj* mirror_b;    /* +0x08 */
} FighterSlot; /* 0x0C */

typedef struct PlyrInfoFlags14 {
    unsigned char alternate_costume : 1;
    unsigned char pad : 7;
    unsigned char pad15[3];
} PlyrInfoFlags14;

/*
 * set_player_state(plyr, state) stores at +0x08.
 * pad_index @ +0x00 feeds check_switch_edge / mcardmsg / gcio.
 * init_plyr_info_struct: pad_index=-1, field_04=3, player_index=0x2C.
 */
typedef struct PlyrInfo {
    int pad_index; /* +0x00 - controller port; -1 unassigned */
    union {
        int field_04;     /* +0x04 - init = 3 */
        int controller_slot; /* +0x04 - physical pad slot; init = 3 (unassigned) */
    };
    int player_state; /* +0x08 - set_player_state; gcio disconnect tests 1/2 */
    float field_0C;   /* +0x0C - init = 1.0 */
    float field_10;   /* +0x10 - sleep/handicap scale (pselect stfs @ +0xB4) */
    union {
        int field_14;
        PlyrInfoFlags14 flags_14_bits;
    }; /* +0x14 */
    PlayerCollisionData* collision_data; /* +0x18 */
    PlyrFightingLightState fighting_lights; /* +0x1C */
    int field_40; /* +0x40 */
    int field_44; /* +0x44 */
    int field_48; /* +0x48 */
    PlyrScreenLatch name_latch; /* +0x4C */
    int player_index; /* +0x54 - character / roster id latch */
    FighterSlot slot; /* +0x58 */
    void* idle_proc;  /* +0x64 */
    void* field_68;   /* +0x68 */
} PlyrInfo; /* 0x6C */

void set_player_state(PlyrInfo* player, int state);
void init_plyr_info_struct(PlyrInfo* player);
int load_plyr_model_async(int player, int char_id, int* flags);

/* Historical name in GameInfo / movelist. */
typedef PlyrInfo GameInfoPlyr;

#endif
