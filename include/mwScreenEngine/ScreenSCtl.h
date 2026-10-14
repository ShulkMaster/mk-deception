#ifndef MWSCREENENGINE_SCREEN_SCTL_H
#define MWSCREENENGINE_SCREEN_SCTL_H

#include "mwScreenEngine/TextureCollection.h"
#include "mwScreenEngine/ScreenControl.h"

typedef struct AniTextureControl AniTextureControl;
typedef struct ScreenNode ScreenNode;
typedef struct ScreenPoly ScreenPoly;
typedef struct ScreenText ScreenText;

/*
 * C views of ScreenControl ('SCtl') subclasses created by
 * mkScreenEngineClient::CreateInstance. Retail offsets from CreateInstance
 * stores + ProcessParams / Update* field use. ScreenControl ends at 0xA4;
 * subclass sizes match __nw__ sizes in CreateInstance.
 *
 * SpreadSheet types use the recovered C++ hierarchy.
 */

/* ScreenObject base fields after the vtable (+0x04 .. +0x8F). */
typedef struct ScreenObjectHead {
    unsigned char node[0x18];  /* +0x04 .. +0x1B -- ScreenNode, m_extraTranslation */
    struct SEObject_t* ext;    /* +0x1C -- m_ext */
    struct ScreenView* screen; /* +0x20 -- m_screen */
    void* parent;              /* +0x24 -- m_parent */
    unsigned char pad28[0x68]; /* +0x28 .. +0x8F */
} ScreenObjectHead;

/* Shared ScreenControl head (ScreenObject 0x90 + control ids). */
typedef struct ScreenControlView {
    void* vtbl; /* +0x00 */
    ScreenObjectHead head; /* +0x04 .. +0x8F */
    int collectionId; /* +0x90 -- m_collectionId */
    int gvContext; /* +0x94 -- Dispatcher unused arg */
    int optionId; /* +0x98 -- m_optionId */
    int pad9C; /* +0x9C */
    int field_0xA0; /* +0xA0 -- purpose not confirmed */
} ScreenControlView; /* 0xA4 -- KeyEntry alloc size */

/* 'KENT' -- bare ScreenControl with KeyEntry vtbl. */
typedef struct KeyEntry {
    ScreenControlView ctrl;
} KeyEntry; /* 0xA4 */

class TextItem : public ScreenControl {
public:
    virtual ~TextItem();
    virtual void Init();
    virtual void Dispose();
    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    virtual void HandleEvent(ScreenMgr* mgr, int event, int arg);
    virtual void ProcessParams(ScreenParams* params);
    virtual void RefreshOption();

    void UpdateString();
    void ScrollText(int dir, int amount);

    ScreenNode* textNode;
    char* editBuf;
    char* gvString;
    signed char curChar;
    int cursorPos;
    int scrollPos;
    int scrollLimit;
    int* indexTable;
};

class KeyPad : public ScreenControl {
public:
    virtual ~KeyPad();
    virtual void Init();
    virtual void Dispose();
    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    virtual void HandleEvent(ScreenMgr* mgr, int event, int arg);
    virtual void ProcessParams(ScreenParams* params);
    virtual void RefreshOption();

    void ChangeCase(unsigned int toUpper);
    void SetKey(ScreenMgr* mgr, const ScreenAction* action, char* key);

    char editBuf[0x20];
    int editLen;
    int pageResIds[4];
    int maxLen;
    int pageIndex;
    int pageCount;
    int active;
};

/*
 * 'SPSH' / 'SPSI' SpreadSheet base fields (CreateInstance shared block).
 * ProcessParams: dims at +0xE0..+0xEC, collection/option ids, bind flags
 * +0xF4/+0xF8/+0xFC, optional color @ +0xA4, nodes at +0xB0/+0xB4.
 */
class SpreadSheet : public ScreenControl {
public:
    virtual ~SpreadSheet();
    virtual void Init();
    virtual void Dispose() = 0;
    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    virtual void HandleEvent(ScreenMgr* mgr, int event, int arg);
    virtual void ProcessParams(ScreenParams* params);
    virtual void Update() = 0;
    virtual void RefreshCollection();
    virtual void RefreshOption();
    virtual void AllocateCollection() = 0;
    virtual void FinishSetup(ScreenParams* params, int nodeIndex) = 0;
    virtual void ClearContents() = 0;

    unsigned char color[4]; /* +0xA4 -- GetColor when useColor */
    unsigned char* cellColors; /* +0xA8 -- text cell RGBA table */
    int padAC; /* +0xAC */
    ScreenNode* nodeB0; /* +0xB0 -- GetScreenNode when bindNodeB0 */
    ScreenNode* nodeB4; /* +0xB4 -- GetScreenNode when bindNodeB4 */
    float layout0[3]; /* +0xB8 -- FinishSetup from poly verts */
    float layout1[3]; /* +0xC4 */
    int scrollX; /* +0xD0 -- window origin X (axis with rows / unkE8) */
    int unkD4; /* +0xD4 -- window origin Y (axis with cols / unkEC) */
    int scrollY; /* +0xD8 -- focus X (ScrollLeft/Right; GetIntArray[2]) */
    int unkDC; /* +0xDC -- focus Y (ScrollUp/Down; GetIntArray[3]) */
    int rows; /* +0xE0 -- GetInt(0); visible width on X */
    int cols; /* +0xE4 -- GetInt(1); visible height on Y */
    int unkE8; /* +0xE8 -- GetInt(2); X count; flag104 = (unkE8 != 0) */
    int unkEC; /* +0xEC -- GetInt(3); Y count; flag108 = (unkEC != 0) */
    int flagF0; /* +0xF0 -- GetBoolean(8) */
    int bindNodeB0; /* +0xF4 -- bit0 of GetInt(7) */
    int bindNodeB4; /* +0xF8 -- bit1 of GetInt(7) */
    int useColor; /* +0xFC -- GetBoolean(6) */
    int cellArray; /* +0x100 -- init 0; set 1 when GameVariables present */
    int flag104; /* +0x104 -- (unkE8 != 0) via neg/andc/srwi */
    int flag108; /* +0x108 -- (unkEC != 0) */
};

/* 'SPSH' SpreadSheet_text -- sizeof 0x120. */
class SpreadSheet_text : public SpreadSheet {
public:
    virtual ~SpreadSheet_text();
    virtual void Dispose();
    virtual void Update();
    virtual void AllocateCollection();
    virtual void FinishSetup(ScreenParams* params, int nodeIndex);
    virtual void ClearContents();
    int hasExtraRes; /* +0x10C -- GetBoolean(9) */
    int extraResId; /* +0x110 -- GetResourceID when hasExtraRes */
    int pad114; /* +0x114 */
    ScreenText** unk118; /* +0x118 -- cell nodes */
    char** strings;
};

/* 'SPSI' SpreadSheet_image -- sizeof 0x124. */
class SpreadSheet_image : public SpreadSheet {
public:
    virtual ~SpreadSheet_image();
    virtual void Dispose();
    virtual void Update();
    virtual void AllocateCollection();
    virtual void FinishSetup(ScreenParams* params, int nodeIndex);
    virtual void ClearContents();
    int hasExtraRes; /* +0x10C -- GetBoolean(9) */
    int extraResId; /* +0x110 -- GetResourceID when hasExtraRes */
    int pad114; /* +0x114 */
    ScreenPoly** unk118; /* +0x118 -- cell nodes */
    GMTextureInfo_t textures;
};

/*
 * 'IMLI' ImageList -- image option strip.
 * ProcessParams: +0x108 count, +0x10C wrap, +0x110 hasLinkNode,
 * +0xAC itemNodes[] (Malloc), +0xB0 link node, +0xF4..FC layout floats.
 * +0xA4 is the two-word GMTextureInfo_t state for Get/FreeTextureCollection.
 */
typedef GVTextureCollection ImageListTexCollection;

class ImageList : public ScreenControl {
public:
    virtual ~ImageList();
    virtual void Init();
    virtual void Dispose();
    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    virtual void HandleEvent(ScreenMgr* mgr, int event, int arg);
    virtual void ProcessParams(ScreenParams* params);
    virtual void Update();
    virtual void RefreshCollection();
    virtual void RefreshOption();
    virtual void Increment(int delta);
    virtual void Decrement(int delta);
    virtual void ScrollInc(int delta);
    virtual void ScrollDec(int delta);

    GMTextureInfo_t textureInfo;
    ScreenNode** itemNodes;
    ScreenNode* linkNode;

    unsigned char padB4[0x40];
    float layoutX;
    float layoutY;
    float layoutZ;
    int scrollBase;
    int focusIndex;
    int itemCount;
    int wrap;
    int hasLinkNode;
};
typedef char check_ImageList_layout[
    (sizeof(ScreenControl) == 0xA4 && __alignof__(ImageList) == 4 &&
     sizeof(ImageList) == 0x114 &&
     (unsigned long)&((ImageList*)0)->m_collectionId == 0x90 &&
     (unsigned long)&((ImageList*)0)->m_gvContext == 0x94 &&
     (unsigned long)&((ImageList*)0)->m_optionId == 0x98 &&
     (unsigned long)&((ImageList*)0)->m_pad9C == 0x9C &&
     (unsigned long)&((ImageList*)0)->field_0xA0 == 0xA0 &&
     (unsigned long)&((ImageList*)0)->textureInfo == 0xA4 &&
     (unsigned long)&((ImageList*)0)->itemNodes == 0xAC &&
     (unsigned long)&((ImageList*)0)->linkNode == 0xB0 &&
     (unsigned long)&((ImageList*)0)->padB4 == 0xB4 &&
     (unsigned long)&((ImageList*)0)->layoutX == 0xF4 &&
     (unsigned long)&((ImageList*)0)->layoutY == 0xF8 &&
     (unsigned long)&((ImageList*)0)->layoutZ == 0xFC &&
     (unsigned long)&((ImageList*)0)->scrollBase == 0x100 &&
     (unsigned long)&((ImageList*)0)->focusIndex == 0x104 &&
     (unsigned long)&((ImageList*)0)->itemCount == 0x108 &&
     (unsigned long)&((ImageList*)0)->wrap == 0x10C &&
     (unsigned long)&((ImageList*)0)->hasLinkNode == 0x110) ? 1 : -1];

/* 'WIFI' WifImage -- WiFi status image control.
 * ProcessParams: imageCount@+0xF4 (max 0x10), float@+0xF8, statusNode@+0xAC,
 * then loads TGA names into images[0..count) @ +0xB4.
 */
typedef struct WifImage {
    void Close();

    void* vtbl; /* +0x00 */
    ScreenObjectHead head; /* +0x04 .. +0x8F */
    int collectionId; /* +0x90 */
    int gvContext; /* +0x94 */
    int optionId; /* +0x98 */
    int pad9C; /* +0x9C */
    int unkA0; /* +0xA0 */
    AniTextureControl* curTexture; /* +0xA4 -- while live */
    int curTextureInstance; /* +0xA8 -- atc->instance latch */
    ScreenPoly* statusNode; /* +0xAC -- GetScreenNode(2) */
    int started; /* +0xB0 -- HandleEvent 0x405 latch; Close clears */
    void* images[16]; /* +0xB4 -- TGA* / RwTexture* slots */
    int imageCount; /* +0xF4 -- GetInt(0); clamped <= 0x10 */
    float unkF8; /* +0xF8 -- GetFloat(1); ani framerate */
} WifImage; /* 0xFC */

/*
 * 'LIST' TextList -- mode-select option list.
 * ProcessParams: itemCount, collection/option ids, wrap, hasLink/hasParam
 * flags, optional linkedNode + paramB0, itemNodes[] Malloc, color from text.
 */
class TextList : public ScreenControl {
public:
    virtual ~TextList();
    virtual void Init();
    virtual void Dispose();
    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    virtual void HandleEvent(ScreenMgr* mgr, int event, int arg);
    virtual void ProcessParams(ScreenParams* params);
    virtual void Update();
    virtual void RefreshCollection();
    virtual void RefreshOption();
    virtual void Move(int delta);
    virtual void ScrollInc(int delta);
    virtual void ScrollDec(int delta);

    ScreenNode** itemNodes;
    ScreenNode* linkedNode;
    char** strings;
    int paramB0;
    unsigned char color[4];
    int focusIndex;
    int focusMax;
    int stringCount;
    int itemCount;
    int wrap;
    int hasLinkedNode;
    int hasParamB0;
    int unkD4;
};
typedef char check_TextList_layout[
    (sizeof(ScreenControl) == 0xA4 && __alignof__(TextList) == 4 &&
     sizeof(TextList) == 0xD8 &&
     (unsigned long)&((TextList*)0)->m_collectionId == 0x90 &&
     (unsigned long)&((TextList*)0)->m_gvContext == 0x94 &&
     (unsigned long)&((TextList*)0)->m_optionId == 0x98 &&
     (unsigned long)&((TextList*)0)->m_pad9C == 0x9C &&
     (unsigned long)&((TextList*)0)->field_0xA0 == 0xA0 &&
     (unsigned long)&((TextList*)0)->itemNodes == 0xA4 &&
     (unsigned long)&((TextList*)0)->linkedNode == 0xA8 &&
     (unsigned long)&((TextList*)0)->strings == 0xAC &&
     (unsigned long)&((TextList*)0)->paramB0 == 0xB0 &&
     (unsigned long)&((TextList*)0)->color == 0xB4 &&
     (unsigned long)&((TextList*)0)->focusIndex == 0xB8 &&
     (unsigned long)&((TextList*)0)->focusMax == 0xBC &&
     (unsigned long)&((TextList*)0)->stringCount == 0xC0 &&
     (unsigned long)&((TextList*)0)->itemCount == 0xC4 &&
     (unsigned long)&((TextList*)0)->wrap == 0xC8 &&
     (unsigned long)&((TextList*)0)->hasLinkedNode == 0xCC &&
     (unsigned long)&((TextList*)0)->hasParamB0 == 0xD0 &&
     (unsigned long)&((TextList*)0)->unkD4 == 0xD4) ? 1 : -1];

#endif
