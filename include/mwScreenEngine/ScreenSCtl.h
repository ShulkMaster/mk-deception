#ifndef MWSCREENENGINE_SCREEN_SCTL_H
#define MWSCREENENGINE_SCREEN_SCTL_H

#include "mwScreenEngine/TextureCollection.h"

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
 * Not full C++ class layouts -- Glue CreateInstance / CreateElement only.
 */

/* Shared ScreenControl head (ScreenObject 0x90 + control ids). */
typedef struct ScreenControlView {
    void* vtbl; /* +0x00 */
    unsigned char so[0x8C]; /* +0x04 .. +0x8F */
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

/*
 * 'TEXT' TextItem -- editable / scrollable label bound to a ScreenText node.
 * ProcessParams: +0xA4 = GetScreenNode(1), +0x98 = GetResourceID(0).
 */
typedef struct TextItem {
    void* vtbl; /* +0x00 */
    unsigned char so[0x8C]; /* +0x04 .. +0x8F */
    int collectionId; /* +0x90 */
    int gvContext; /* +0x94 */
    int optionId; /* +0x98 -- CreateInstance zeros (overrides ctor -1) */
    int pad9C; /* +0x9C */
    int unkA0; /* +0xA0 */
    ScreenNode* textNode; /* +0xA4 -- ScreenText node */
    char* editBuf; /* +0xA8 -- scroll copy; Malloc when scrollLimit > 1 */
    char* gvString; /* +0xAC -- GameVariables GetString latch */
    signed char curChar; /* +0xB0 -- init -1; NUL hole restore */
    unsigned char padB1[3];
    int cursorPos; /* +0xB4 -- init -1; index of NUL hole */
    int scrollPos; /* +0xB8 -- init 0; wrap-line window origin */
    int scrollLimit; /* +0xBC -- init -1; GetStartArray line count */
    int* indexTable; /* +0xC0 -- GetStartArray wrap starts */
} TextItem; /* 0xC4 */

/* 'KPAD' KeyPad -- soft keyboard.
 * ProcessParams: pageResIds[4] @ +0xC8, maxLen @ +0xD8, pageCount @ +0xE0.
 * SetKey: editBuf[i] @ +0xA4+i, editLen @ +0xC4 (was misnamed keyString).
 */
typedef struct KeyPad {
    void* vtbl; /* +0x00 */
    unsigned char so[0x8C];
    int collectionId; /* +0x90 */
    int gvContext; /* +0x94 */
    int optionId; /* +0x98 */
    int pad9C; /* +0x9C */
    int unkA0; /* +0xA0 */
    char editBuf[0x20]; /* +0xA4 -- NUL-terminated input chars */
    int editLen; /* +0xC4 -- current length / cursor */
    int pageResIds[4]; /* +0xC8 -- GameVariables option ids per page */
    int maxLen; /* +0xD8 -- GetInt(4) */
    int pageIndex; /* +0xDC -- active page; ProcessParams clears */
    int pageCount; /* +0xE0 -- GetInt(5) */
    int active; /* +0xE4 -- case/active latch; ChangeCase writes */
} KeyPad; /* 0xE8 */

/*
 * 'SPSH' / 'SPSI' SpreadSheet base fields (CreateInstance shared block).
 * ProcessParams: dims at +0xE0..+0xEC, collection/option ids, bind flags
 * +0xF4/+0xF8/+0xFC, optional color @ +0xA4, nodes at +0xB0/+0xB4.
 */
typedef struct SpreadSheet {
    void* vtbl; /* +0x00 */
    unsigned char so[0x8C];
    int collectionId; /* +0x90 */
    int gvContext; /* +0x94 */
    int optionId; /* +0x98 */
    int pad9C; /* +0x9C */
    int unkA0; /* +0xA0 */
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
} SpreadSheet; /* through +0x10C; subclasses grow */

/* 'SPSH' SpreadSheet_text -- sizeof 0x120. */
typedef struct SpreadSheet_text {
    SpreadSheet sheet;
    int hasExtraRes; /* +0x10C -- GetBoolean(9) */
    int extraResId; /* +0x110 -- GetResourceID when hasExtraRes */
    int pad114; /* +0x114 */
    ScreenText** unk118; /* +0x118 -- cell nodes */
    int unk11C; /* +0x11C -- init 0 */
} SpreadSheet_text; /* 0x120 */

/* 'SPSI' SpreadSheet_image -- sizeof 0x124. */
typedef struct SpreadSheet_image {
    SpreadSheet sheet;
    int hasExtraRes; /* +0x10C -- GetBoolean(9) */
    int extraResId; /* +0x110 -- GetResourceID when hasExtraRes */
    int pad114; /* +0x114 */
    ScreenPoly** unk118; /* +0x118 -- cell nodes */
    int unk11C; /* +0x11C -- init 0 */
    int unk120; /* +0x120 -- init 0 */
} SpreadSheet_image; /* 0x124 */

/*
 * 'IMLI' ImageList -- image option strip.
 * ProcessParams: +0x108 count, +0x10C wrap, +0x110 hasLinkNode,
 * +0xAC itemNodes[] (Malloc), +0xB0 link node, +0xF4..FC layout floats.
 * +0xA4 is the two-word GMTextureInfo_t state for Get/FreeTextureCollection.
 */
typedef GVTextureCollection ImageListTexCollection;

typedef struct ImageList {
    void* vtbl; /* +0x00 */
    unsigned char so[0x8C];
    int collectionId; /* +0x90 */
    int gvContext; /* +0x94 -- Dispatcher unused arg (often -1) */
    int optionId; /* +0x98 */
    int pad9C; /* +0x9C */
    int unkA0; /* +0xA0 */
    GMTextureInfo_t textureInfo; /* +0xA4 */
    ScreenNode** itemNodes; /* +0xAC -- Malloc(count*4) */
    ScreenNode* linkNode; /* +0xB0 -- GetScreenNode when hasLinkNode */
    /*
     * +0xB4 .. +0xF3: unused by ImageList methods (ProcessParams / Update /
     * Refresh* / Scroll* / HandleEvent). Gap before layout floats.
     */
    unsigned char padB4[0x40]; /* +0xB4 .. +0xF3 */
    float layoutX; /* +0xF4 -- from first poly verts when linked */
    float layoutY; /* +0xF8 */
    float layoutZ; /* +0xFC */
    int scrollBase; /* +0x100 -- Update window origin */
    int focusIndex; /* +0x104 -- clamped to tex count */
    int itemCount; /* +0x108 -- GetInt(0) */
    int wrap; /* +0x10C -- GetBoolean(3); init 1 */
    int hasLinkNode; /* +0x110 -- GetBoolean(4) */
} ImageList; /* 0x114 */

/* 'WIFI' WifImage -- WiFi status image control.
 * ProcessParams: imageCount@+0xF4 (max 0x10), float@+0xF8, statusNode@+0xAC,
 * then loads TGA names into images[0..count) @ +0xB4.
 */
typedef struct WifImage {
    void* vtbl; /* +0x00 */
    unsigned char so[0x8C];
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
typedef struct TextList {
    void* vtbl; /* +0x00 */
    unsigned char so[0x8C];
    int collectionId; /* +0x90 */
    int gvContext; /* +0x94 */
    int optionId; /* +0x98 */
    int pad9C; /* +0x9C */
    int unkA0; /* +0xA0 */
    ScreenNode** itemNodes; /* +0xA4 -- ScreenNode table */
    ScreenNode* linkedNode; /* +0xA8 -- GetScreenNode when hasLinkedNode */
    char** strings; /* +0xAC -- GameVariables string collection */
    int paramB0; /* +0xB0 -- GetInt when hasParamB0 */
    unsigned char color[4]; /* +0xB4 -- RGBA from ScreenText seData */
    int focusIndex; /* +0xB8 -- init 0 */
    int focusMax; /* +0xBC -- option cursor / string index */
    int stringCount; /* +0xC0 -- GetStringCollection count */
    int itemCount; /* +0xC4 -- GetInt(0) */
    int wrap; /* +0xC8 -- GetBoolean(3); init 1 */
    int hasLinkedNode; /* +0xCC -- GetBoolean(5) */
    int hasParamB0; /* +0xD0 -- GetBoolean(4) */
    int unkD4; /* +0xD4 -- set 1 when GameVariables present */
} TextList; /* 0xD8 */

#endif
