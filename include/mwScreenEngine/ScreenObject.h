#ifndef MWSCREENENGINE_SCREENOBJECT_H
#define MWSCREENENGINE_SCREENOBJECT_H

#include "mwScreenEngine/ScreenNode.h"
#include "mwScreenEngine/ScreenAnimControl.h"
#include "mwScreenEngine/ScreenMatrixStack.h"

/*
 * ScreenObject -- interactive ScreenEngine node (retail size 0x90).
 *
 * Extends ScreenNode. LoadScreenSet fills m_ext (SEObjectExt) with
 * flags / events / transform / children before Render / FireEvent / focus
 * produce results.
 *
 * Wave C focus / event path (pad ids in mwScreenEngineGlue.h):
 *   FireEvent(event, arg, flag) -- recurse to m_focus[arg], then
 *     ScreenUtil::HandleEvent + virtual HandleEvent + ProcessEvent
 *   SetFocus -- fires 0x3ED (lose) / 0x3EC (gain) when fireEvents!=0
 *   FindNextFocusObject -- walks actions 0x3E8 / 0x3F6 for next node
 *   BroadcastEngine range 0x3E8..0x3EB and 0x407..0x408 -> ProcessEngineEvent
 *
 * Vtbl extends ScreenNode and adds HandleEvent at +0x3C:
 *   +0x10 Dispose / +0x14 Render / +0x18 SetComponent /
 *   +0x1C SetVisible / +0x20 IsVisible / +0x28 HandleAction /
 *   +0x2C SetMatrixStack / +0x3C HandleEvent
 *
 * Layout verified from ScreenObject ctor stores + accessors.
 */

class Screen;
class ScreenMgr;
class ScreenAction;
class ScreenEvent;
class ScreenMatrixStack;
class ScreenParams;
class ScreenObject;
struct ScreenChildEntry;

struct SEVec4_t {
    float x, y, z, w;
};

struct SETransform {
    /* Disc layout: +0x00 feeds Rotate angles; +0x0C feeds Scale vector. */
    float scale[3]; /* +0x00 -- euler angles about X/Y/Z in UpdateTransform */
    union {
        float rotation[3];
        Screen3DVector rotationVector; /* +0x0C -- Scale() argument */
    };
    union {
        float translation[3];
        Screen3DVector translationVector; /* +0x18 */
    };
    union {
        float pivot[3];
        Screen3DVector pivotVector; /* +0x24 */
    };
    float colorScale[4]; /* +0x30 */
    float colorTranslation[4]; /* +0x40 */
};

struct ScreenEventList {
    int count; /* +0x00 */
    ScreenEvent* events[1]; /* +0x04 -- retail ILP32 packed table */
};

/* Retail CreateElements takes SEElements_t* (mangled); same packed shape. */
struct SEElements_t {
    int count; /* +0x00 */
    ScreenChildEntry* entries[1]; /* +0x04 -- retail ILP32 packed table */
};

typedef SEElements_t ScreenChildList;

struct ScreenChildEntry {
    unsigned int typeTag; /* +0x00 -- 'OBJ ' / 'GROP' / 'TEXT' / 'POLY' / ... */
    int pad04; /* +0x04 */
    ScreenObject* object; /* +0x08 -- live instance after Create* */
};

/*
 * Retail PatchText/Poly take SEBaseElement_t* (mangled name). Same layout as
 * ScreenChildEntry / element head; keep a distinct struct tag for MWCC.
 */
struct SEBaseElement_t {
    unsigned int typeTag; /* +0x00 */
    int pad04; /* +0x04 */
    ScreenObject* liveObject; /* +0x08 */
};

/* Disc attribute / classInfo blobs (PatchAttribue*). Primary tag capital _T
 * matches retail mangling PatchAttribue__FP17SEBaseAttribute_T... */
struct SEBaseAttribute_T {
    int type; /* +0x00 */
    unsigned int value; /* +0x04 -- string index or relocated ptr */
};

typedef SEBaseAttribute_T SEBaseAttribute_t;

struct SEAttributes_t {
    unsigned int count; /* +0x00 */
    SEBaseAttribute_t* attributes[1]; /* +0x04 -- trailing pointer table */
};

struct SEObjectClassInfo {
    int typeId; /* +0x00 */
    SEAttributes_t* params; /* +0x04 -- attr head / CreateInstance params */
};

/*
 * Disc event action (stride 0x10) starting at SEEvent/ScreenEvent +0x0C.
 * Overlay of ScreenEventSlot shifted -4: actionType @ +0x04, attrs @ +0x0C
 * (== GetParams slot @ event+0x18+i*0x10 after Patch).
 */
struct SEAction_t {
    unsigned int lead; /* +0x00 -- m_unk0c for [0]; prior slot trailer after */
    unsigned int actionType; /* +0x04 */
    unsigned short startOfSubAction; /* +0x08 */
    unsigned short numOfSubActions; /* +0x0A */
    SEAttributes_t* attrs; /* +0x0C -- ScreenParams* after Patch */
};

/*
 * Same disc blob as ScreenEvent (GetEvent returns this memory).
 * PatchScreenEvent walks actionCount @ +0x04; ProcessEvent uses
 * numActions @ +0x08 (retail reads both; do not "fix" either).
 */
struct SEEvent_t {
    unsigned int id; /* +0x00 -- ScreenEvent::m_id */
    unsigned int actionCount; /* +0x04 -- PatchScreenEvent; m_unk04 */
    unsigned int numActions; /* +0x08 -- ProcessEvent; m_numActions */
    SEAction_t actions[1]; /* +0x0C -- trailing action table, stride 0x10 */
};

/*
 * Disc SeRef -- retail file-relative u32 (ILP32). After Patch* reloc it holds
 * a live pointer bit-pattern. Keep SeRef as u32 in the disc blob
 * (do not store non-ILP32 pointers in-place).
 */
typedef unsigned int SeRef;

struct SEVec3_t {
    float x, y, z;
};

/*
 * TEXT element -- PatchTextObject relocates string0/string1 via SEStringTable.
 * CreateElement('TEXT') builds ScreenText; ProcessEngineEvent 0x407..0x409
 * must run before Render so StringObj exists (create_wrapped_string 0x900C).
 * Font: load_named_font(string1) else client TGA+%s_MET cache (gc_font draw).
 */
struct SETextElement_t {
    unsigned int typeTag; /* +0x00 'TEXT' */
    int pad04; /* +0x04 */
    ScreenObject* liveObject; /* +0x08 */
    unsigned int unk0c; /* +0x0C */
    SeRef string0; /* +0x10 -- string key; char* after Patch */
    SeRef string1; /* +0x14 -- font name; char* after Patch */
    float posX; /* +0x18 */
    float posY; /* +0x1C -- ProcessEngineEvent Y = (480 - posY) */
    float unk20; /* +0x20 */
    float wrapW; /* +0x24 */
    float yOff; /* +0x28 */
    unsigned char color[4]; /* +0x2C */
    unsigned char halign; /* +0x30 */
    unsigned char valign; /* +0x31 */
};

/*
 * POLY element -- packed disc layout (size through +0x70).
 * Midway CreatePoly encodes retail vert/UV/RGBA + textureString bind.
 *
 * Layout (ILP32 packed -- do not insert native pointers inside the blob):
 *   +0x00 typeTag 'POLY'
 *   +0x0C flags (bit0 -> ScreenPoly filterFlags bit6 linear)
 *   +0x10 positions[4] SEVec3
 *   +0x40 colors[4][4]
 *   +0x50 uvs[4][2]
 *   +0x70 textureString SeRef -> char* after PatchPoly
 *
 * CreatePoly: basename after '\\', strupr, load_named_tga + alpha twin.
 * Vert Y = 480 - position.y (screen space); UV V = 1 - uv.v; offsets init 0.
 */
struct SEPolyElement_t {
    unsigned int typeTag; /* +0x00 */
    int pad04; /* +0x04 */
    ScreenObject* liveObject; /* +0x08 */
    unsigned int flags; /* +0x0C */
    SEVec3_t positions[4]; /* +0x10 -- stride 0x0C */
    unsigned char colors[4][4]; /* +0x40 */
    float uvs[4][2]; /* +0x50 -- stride 0x08 */
    SeRef textureString; /* +0x70 -- idx then char* after Patch */
};

/*
 * Disc SEObject_t / runtime m_ext. Primary tag name must be SEObject_t for
 * MWCC mangling (CreateObject / PatchScreenObject / ProcessControls).
 * CreateObject stores the live ScreenObject* at +0x08.
 *
 * Disc pointer slots are file-relative u32 until Patch* reloc (retail ILP32).
 */
struct SEObject_t {
    unsigned int typeTag; /* +0x00 */
    int pad04; /* +0x04 */
    ScreenObject* liveObject; /* +0x08 */
    unsigned int flags; /* +0x0C -- bit0 visible; bit1 suppress FireEvent */
    ScreenEventList* events; /* +0x10 */
    SETransform* transform; /* +0x14 */
    ScreenChildList* children; /* +0x18 -- SEElements_t same shape */
    SEObjectClassInfo* classInfo; /* +0x1c -- CreateInstance / attribute head */
};

typedef SEObject_t SEObjectExt;

/*
 * Retail packed pointer table at +4 after count (not C array[1], ILP32).
 * Macros -- MWCC -inline off emits bl for C++ inline helpers.
 */
#define ScreenChildEntryPtrSlot(list, i) (&(list)->entries[(i)])

#define ScreenChildEntryAt(list, i) (*ScreenChildEntryPtrSlot((list), (i)))

#define SeEntryObject(entry) ((entry)->object)
#define SeTransformOf(ext) ((ext)->transform)
#define SeChildrenOf(ext) ((ext)->children)
#define SeEventsOf(ext) ((ext)->events)
#define SeClassInfoOf(ext) ((ext)->classInfo)

#define ScreenEventPtrSlot(list, i) (&(list)->events[(i)])
#define ScreenEventAt(list, i) (*ScreenEventPtrSlot((list), (i)))

inline SEBaseAttribute_t** SEAttributePtrSlot(SEAttributes_t* list, unsigned int i) {
    return &list->attributes[i];
}

inline SEBaseAttribute_t* SEAttributeAt(SEAttributes_t* list, unsigned int i) {
    return *SEAttributePtrSlot(list, i);
}

inline SEAction_t* SEActionAt(SEEvent_t* event, unsigned int i) {
    return &event->actions[i];
}

struct ScreenAnimLastEvent {
    ScreenAnimControl* control; /* +0x00 */
    int lastEvent; /* +0x04 -- -1 empty */
};

struct ScreenRenderInfo {
    unsigned int flags; /* +0x00 */
    ScreenMatrixStack* matrixStack; /* +0x04 */
    float colorScale[4]; /* +0x08 */
    float colorTranslation[4]; /* +0x18 */
};

enum {
    kScreenTagOBJ = 0x4F424A20, /* 'OBJ ' */
    kScreenTagGROP = 0x47524F50, /* 'GROP' */
    kScreenTagBASE = 0x42415345, /* 'BASE' */
    kScreenTagTEXT = 0x54455854, /* 'TEXT' */
    kScreenTagPOLY = 0x504F4C59, /* 'POLY' */
    kScreenTagPART = 0x50415254, /* 'PART' */
    kScreenTagPTCL = 0x5054434C, /* 'PTCL' -- particle; FX soft ceiling */
    kScreenTagCHAR = 0x43484152, /* 'CHAR' */
    kScreenTagSCTL = 0x5343746C /* 'SCtl' -- control head */
};

class ScreenObject : public ScreenNode {
public:
    ScreenObject();
    virtual ~ScreenObject();

    virtual void Dispose();
    virtual void Render(ScreenRenderInfo* info);
    virtual void SetComponent(ScreenAnimControl* ctrl, float* values, int unused);
    virtual void SetVisible(unsigned int visible); /* also weak in Glue */
    virtual unsigned int IsVisible(); /* also weak in Glue */
    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    virtual void SetMatrixStack(ScreenMatrixStack* stack);
    virtual void HandleEvent(ScreenMgr* mgr, int event, int arg);

    void CreateMatrixStack();
    void SetColorScale(SEVec4_t* color);
    void SetColorTranslation(SEVec4_t* color);
    void UpdateTransform();

    int GetNumEvents() const;
    ScreenEvent* GetEvent(unsigned int index);
    ScreenObject* GetFocus(int index);
    void SetFocus(ScreenMgr* mgr, ScreenObject* obj, int index, int fireEvents);
    void ClearActiveObjects();
    void SetParent(ScreenObject* parent);

    void ProcessSubActions(ScreenMgr* mgr, const ScreenAction* action, int match);
    void ProcessSubActions(const ScreenAction* action, int match);
    void ProcessEvent(ScreenMgr* mgr, int event, int arg);
    int HasEvent(int event);
    void FireEvent(ScreenMgr* mgr, int event, int arg, unsigned int flag);
    void BroadcastEvent(ScreenMgr* mgr, int event, int arg);
    ScreenObject* FindNextFocusObject(int event);

    int GetLastEvent(ScreenAnimControl* ctrl);
    void SetLastEvent(ScreenAnimControl* ctrl, int event);

    /* ScreenNode: +0x00..+0x0C */
    union {
        float m_extraTrans[3];
        Screen3DVector m_extraTranslation; /* +0x10 */
    };
    SEObjectExt* m_ext; /* +0x1C */
    Screen* m_screen; /* +0x20 */
    ScreenObject* m_parent; /* +0x24 */
    ScreenObject* m_focus[4]; /* +0x28 */
    ScreenMatrixStack* m_matrixStack; /* +0x38 */
    unsigned int m_objTag; /* +0x3C -- 'OBJ ' */
    ScreenAnimLastEvent m_lastEvents[10]; /* +0x40 */
};

#endif
