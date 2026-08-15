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
typedef struct MkFileEntry MkFileEntry;

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
    char pad00[4];
    const char* primary_art_section;
    const char* primary_face_texture;
    const char* animation_section; /* +0x0C */
    const int* primary_bone_tags;
    void* primary_mirror_bone_map; /* +0x14 */
    void* primary_ground_collision; /* +0x18 */
    char pad1C[4];
    unsigned int primary_start_script; /* +0x20 */
    char pad24[4];
    const char* alternate_art_section;
    const char* alternate_face_texture;
    const char* alternate_animation_section; /* +0x30 */
    const int* alternate_bone_tags;
    void* alternate_mirror_bone_map; /* +0x38 */
    void* alternate_ground_collision; /* +0x3C */
    char pad40[4];
    unsigned int alternate_start_script; /* +0x44 */
    char pad48[4];
    const char* palette_art_section;
    const char* palette_face_texture;
    const char* alternate_palette_art_section;
    const char* alternate_palette_face_texture;
    const char* shared_art_section;
    char pad60[0x0C];
    int win_sound_id; /* +0x6C */
    char pad70[0x10];
    char* const* style_scripts; /* +0x80 */
    char pad84[0x14];
    const char* chess_animation_section; /* +0x98 */
    char pad9C[4];
    const char* const* effect_banks;
    const char* const* alternate_effect_banks;
    char padA8[4];
    Vec* half_sever_velocities; /* +0xAC */
};

/* global_player_data[] stride 0x10 (movelist_get_character_name). */
typedef struct GlobalPlayerEntry {
    char* name; /* +0x00 */
    MkFileEntry* model_files; /* +0x04 */
    MkFileEntry* alternate_model_files; /* +0x08 */
    const char* model_script; /* +0x0C */
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
    unsigned char alternate_palette : 1;
    unsigned char pad : 6;
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

/* Historical name in GameInfo / movelist. */
typedef PlyrInfo GameInfoPlyr;

#endif
