#ifndef MWSCREENENGINE_SCREEN_H
#define MWSCREENENGINE_SCREEN_H

#include "mwScreenEngine/ScreenNode.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenAnimScene.h"
#include "mwScreenEngine/ScreenAnimEffect.h"
#include "mwScreenEngine/ScreenAnimControl.h"

/*
 * One loaded screen instance (size 0x6C). Owned in ScreenSet::m_screens[].
 *
 * ILP32 Matching size -- do NOT use sizeof(Screen) as LoadSetData stride
 * (host LP64 is larger; see menu_decomp_asks.md Playbook 2).
 *
 * ScreenEngine C++ Screen -- NOT the image.o ScreenObj list latch.
 * Glue load_screen inserts a separate MkHdr (vtbl_screen_engine) onto
 * image.screen_obj_list; that latch's draw path calls
 * screen_engine_render -> ScreenMgr::Render -> Screen::RenderAll here.
 *
 * LoadScreenSet fills m_data (objects->root) before RenderAll draws.
 *
 * =====================================================================
 * scr_*.sec / SCREEN blob
 * =====================================================================
 *
 * Retail mkScreenEngineClient::LoadScreenSet(set):
 *   1. load_ssf(screen_engine_file_table)
 *   2. find/load art section "scr_<setName>.sec"
 *   3. load_named_binary_block_from_file(..., "STRINGS", &size)
 *      -> ReadStringData (string table)
 *   4. load_named_binary_block_from_file(..., "SCREEN", &size)
 *      -> ScreenInstancer::LoadSetData(set, blob, size, 0)
 *
 * SEScreenSet_t (SCREEN binary, file-relative u32 offsets until Patch):
 *   +0x00 magic 'SSET' (0x53534554); LoadSetData writes 'DONE' (0x444F4E45)
 */
enum { kScreenBytes = 0x6C };

/* Packed Screen* array: base + i * kScreenBytes (not C++ Screen[] on host). */
#define ScreenAt(base, i) ((Screen*)((char*)(base) + (i) * kScreenBytes))

/*
 * SEScreenSet_t / SEScreen_t disc layout (file-relative u32 until Patch):
 *   SSET +0x08 screen table, +0x10 numScreens, +0x14 name offs
 *   SNGC +0x0C objects, +0x10 animScenes, +0x14 strings
 * Element typeTags: 'OBJ '/'GROP' -> CreateObject; else CreateElement.
 */

class ScreenMgr;
class ScreenMatrixStack;
class ScreenSet;
struct SEStringTable_t;

/* Packed u32-pointer table: count @ +0, refs @ +4 (retail ILP32). */
struct SERefTable {
    unsigned int count; /* +0x00 */
    unsigned int refs[1]; /* +0x04 -- relocated pointers, retail ILP32 */
};

/* Macro -- MWCC -inline off emits bl for C++ inline helpers (ILP32 +4 table). */
#define SERefSlot(table, i) (&(table)->refs[(i)])

/*
 * Anim effect nesting (PatchAnimEffects):
 *   SEAnimEffects_t -> ScreenAnimEffect*[] -> tracks(+0x0C) SERefTable
 *     -> ScreenAnimControl*[] -> keys(+0x18) SERefTable -> ScreenAnimKey*
 *
 * Disc tags SEAnimEffect_t / SEAnimEffectItem_t share these layouts.
 *
 * === PatchAnims reloc order (SNGC / file-relative to absolute) ===
 * `base` = start of SCREEN blob (same as object patching). Offsets on disc
 * are file-relative; add `base` in place. Safe order:
 *   1. For each packed ScreenAnimScene (stride 0x18 @ block+4):
 *      reloc m_elements (+0x10), m_data (+0x14)
 *   2. Walk m_elements SERefTable (+4 slots) to reloc each ptr
 *   3. scene->m_flags = 0x20 (forward / idle default)
 *   4. If (m_data->flags & 1) == 0:
 *      for each track (stride 0x0C @ data+0x0C):
 *        reloc track->effects (+0x08); PatchAnimEffects(effects, base)
 *      m_data->flags |= 1
 *      CalculateMaxTime(scene)  -- only after tracks/effects/keys relocated
 * Soft reclaim on CalculateMaxTime / Process is optional; do not reorder
 * reloc vs CalculateMaxTime for Matching.
 */
typedef ScreenAnimControl SEAnimEffectItem_t;
typedef ScreenAnimEffect SEAnimEffect_t;

struct SEAnimEffects_t {
    int count; /* +0x00 */
    SEAnimEffect_t* effects[1]; /* +0x04 -- trailing pointer table */
};

/* Macros -- MWCC -inline off emits bl for C++ inline helpers (ILP32 +4). */
#define SEAnimEffectPtrSlot(list, i) \
    (&((SEAnimEffect_t**)((char*)(list) + 4))[(i)])

#define SEAnimEffectAtOffset(list, byteOffset) \
    (*(SEAnimEffect_t**)((char*)(list) + 4 + (byteOffset)))

/* Per-track slot inside SEAnimSceneData (stride 0x0C @ +0x0C). */
struct SEAnimTrack_t {
    unsigned int unk00; /* +0x00 -- unread by CalculateMaxTime/Process/PatchAnims */
    int timeOffset; /* +0x04 */
    SEAnimEffects_t* effects; /* +0x08 */
};

struct SEAnimSceneData_t {
    unsigned int flags; /* +0x00 -- bit0 set after PatchAnims */
    int maxTime; /* +0x04 -- CalculateMaxTime / ResetTime */
    int trackCount; /* +0x08 */
    SEAnimTrack_t tracks[1]; /* +0x0C -- trailing track table */
};

/* Macro -- open-coded stride walk; -inline off would emit bl. */
#define SEAnimTrackAt(data, i) \
    (&(data)->tracks[(i)])

#define SEAnimTrackAtOffset(data, byteOffset) \
    ((SEAnimTrack_t*)((char*)(data) + 0x0C + (byteOffset)))

/* Primary SEAnimBlock_t for PatchAnims mangling; ScreenAnimSceneList is alias. */
struct SEAnimBlock_t {
    int count; /* +0x00 */
    ScreenAnimScene scenes[1]; /* +0x04 -- trailing packed scene table */
};

typedef SEAnimBlock_t ScreenAnimSceneList;

/*
 * Retail: scenes packed at +4 after count, stride 0x18.
 * Macro -- Screen.o builds with -inline off; a C++ inline emits bl and tanks
 * GetAnimScene / UpdateSceneAnimation / ShutoffAnimScenes.
 */
#define ScreenAnimSceneAt(list, i) \
    (&(list)->scenes[(i)])

#define ScreenAnimSceneAtOffset(list, byteOffset) \
    ((ScreenAnimScene*)((char*)(list) + 4 + (byteOffset)))

/* Per-screen blob inside SCREEN binary (also Screen::m_data). */
struct ScreenObjectRoot {
    unsigned int typeTag; /* +0x00 */
    int pad04;
    ScreenObject* root; /* +0x08 -- GetRoot / CreateScreen fill */
};

/* Primary tag SEScreen_t for CreateScreen mangling; ScreenData is alias. */
struct SEScreen_t {
    unsigned int magic; /* +0x00 -- 'SNGC' when from SCREEN blob */
    unsigned int unk04;
    unsigned int unk08;
    ScreenObjectRoot* objects; /* +0x0C -- root SEObject_t* */
    ScreenAnimSceneList* animScenes; /* +0x10 */
    SEStringTable_t* strings; /* +0x14 */
};

typedef SEScreen_t ScreenData;

#define SeObjectsOf(screenData) ((screenData)->objects)
#define SeAnimScenesOf(screenData) ((screenData)->animScenes)
#define SeStringsOf(screenData) ((screenData)->strings)

/* Top-level SCREEN set blob (LoadSetData param). */
struct SEScreenSet_t {
    unsigned int magic; /* +0x00 'SSET' / 'DONE' */
    unsigned int unk04;
    unsigned int* screenOffs; /* +0x08 -- table of SEScreen_t* (after reloc) */
    unsigned int unk0c;
    int numScreens; /* +0x10 */
    union {
        SeRef nameOffs[1]; /* +0x14 -- file offsets before relocation */
        char* names[1]; /* +0x14 -- trailing pointer table after relocation */
    };
};

struct SEStringTable_t {
    unsigned int count; /* +0x00 */
    union {
        SeRef stringRefs[1]; /* +0x04 -- file offsets before relocation */
        char* strings[1]; /* +0x04 -- trailing pointer table after relocation */
    };
};

/* Name table packed at +0x14 after SEScreenSet_t header (not a C member). */
inline char** SEScreenNameSlots(SEScreenSet_t* blob) {
    return blob->names;
}

#define SEScreenNameRefSlots(blob) ((blob)->nameOffs)

inline SeRef* SEStringSlot(SEStringTable_t* table, unsigned int i) {
    return &table->stringRefs[i];
}

inline char* ScreenStringAt(SEStringTable_t* table, unsigned int index) {
    if (table == 0) {
        return 0;
    }
    if (index < table->count) {
        return table->strings[index];
    }
    return 0;
}

class Screen {
public:
    Screen();
    ~Screen();

    void Dispose();
    ScreenAnimScene* GetAnimScene(int index);
    void ShutoffAnimScenes();
    void BroadcastEvent(ScreenMgr* mgr, int event, int arg);
    void UpdateSceneAnimation(int dt);
    ScreenObject* GetRoot();
    void FireEvent(ScreenMgr* mgr, int event, int arg, unsigned int flag);
    void InitMatrixStack();
    void RenderAll();
    char* GetName();
    void SetName(char* name);
    void SetHeadIdle(ScreenNode* node);
    void ProcessIdleEvent(ScreenMgr* mgr);
    void SetHeadControl(ScreenNode* node);

    int m_state; /* +0x00 -- -1 reset; 0 open; 2 closing */
    int m_opened; /* +0x04 */
    char m_name[0x44]; /* +0x08 */
    int field_0x4C; /* +0x4c -- ctor 1; purpose not confirmed */
    int m_loaded; /* +0x50 -- ctor 0; CreateScreen sets 1 */
    ScreenSet* m_set; /* +0x54 -- set by LoadSetData / parent; ctor leaves unset */
    ScreenData* m_data; /* +0x58 */
    int m_visible; /* +0x5c */
    ScreenNode* m_headIdle; /* +0x60 */
    ScreenNode* m_headControl; /* +0x64 */
    ScreenMatrixStack* m_matrixStack; /* +0x68 */
};

/*
 * ScreenInstancer -- build / tear down Screen + object trees from SE* blobs.
 *
 * Retail order: CreateScreen, Destroy*, Close*, CreateObject/Elements,
 * Patch*, ProcessScreenData, LoadSetData.
 *
 * SOFT CEILING: Patch* / ProcessControls near-miss OK; prefer callable
 * LoadSetData + Create* over Matching grind. NonMatching = ASM still linked.
 */
class ScreenInstancer {
public:
    static int CreateScreen(Screen* screen, SEScreen_t* seScreen);
    static void DestroyObject(ScreenObject* object);
    static void DestroyScreen(Screen* screen);
    /* Walk child OBJ/GROP tree and vtbl Close (+0x38). */
    static void CloseObject(ScreenObject* object);
    static void CloseScreen(Screen* screen);
    static ScreenObject* CreateObject(ScreenMgr* mgr, Screen* screen,
                                      ScreenObject* parent, SEObject_t* seObj);
    static int CreateElements(ScreenMgr* mgr, Screen* screen, ScreenObject* parent,
                              SEElements_t* elements);
    /* Relocate file offsets in SCREEN blob, then CreateScreen each entry. */
    static int LoadSetData(ScreenSet* set, void* data, unsigned int size, void* unused);
};

#endif
