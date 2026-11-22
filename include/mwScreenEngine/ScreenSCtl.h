#ifndef MWSCREENENGINE_SCREEN_SCTL_H
#define MWSCREENENGINE_SCREEN_SCTL_H

#include "mwScreenEngine/TextureCollection.h"
#include "mwScreenEngine/ScreenControl.h"

typedef struct AniTextureControl AniTextureControl;
typedef struct ScreenNode ScreenNode;
typedef struct ScreenPoly ScreenPoly;
typedef struct ScreenText ScreenText;

typedef struct ScreenObjectHead {
    unsigned char node[0x18];
    struct SEObject_t* ext;
    struct ScreenView* screen;
    void* parent;
    unsigned char pad28[0x68];
} ScreenObjectHead;

typedef struct ScreenControlView {
    void* vtbl;
    ScreenObjectHead head;
    int collectionId;
    int gvContext;
    int optionId;
    int pad9C;
    int field_0xA0;
} ScreenControlView;

typedef struct KeyEntry {
    ScreenControlView ctrl;
} KeyEntry;

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
    unsigned char ShouldAutoScrollDown();

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

    unsigned char color[4];
    unsigned char* cellColors;
    int padAC;
    ScreenNode* nodeB0;
    ScreenNode* nodeB4;
    float layout0[3];
    float layout1[3];
    int scrollX;
    int unkD4;
    int scrollY;
    int unkDC;
    int rows;
    int cols;
    int unkE8;
    int unkEC;
    int flagF0;
    int bindNodeB0;
    int bindNodeB4;
    int useColor;
    int cellArray;
    int flag104;
    int flag108;
};

class SpreadSheet_text : public SpreadSheet {
public:
    virtual ~SpreadSheet_text();
    virtual void Dispose();
    virtual void Update();
    virtual void AllocateCollection();
    virtual void FinishSetup(ScreenParams* params, int nodeIndex);
    virtual void ClearContents();
    int hasExtraRes;
    int extraResId;
    int pad114;
    ScreenText** unk118;
    char** strings;
};

class SpreadSheet_image : public SpreadSheet {
public:
    virtual ~SpreadSheet_image();
    virtual void Dispose();
    virtual void Update();
    virtual void AllocateCollection();
    virtual void FinishSetup(ScreenParams* params, int nodeIndex);
    virtual void ClearContents();
    int hasExtraRes;
    int extraResId;
    int pad114;
    ScreenPoly** unk118;
    GMTextureInfo_t textures;
};

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
    void SubmitOption();

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

typedef struct WifImage {
    void Close();

    void* vtbl;
    ScreenObjectHead head;
    int collectionId;
    int gvContext;
    int optionId;
    int pad9C;
    int unkA0;
    AniTextureControl* curTexture;
    int curTextureInstance;
    ScreenPoly* statusNode;
    int started;
    void* images[16];
    int imageCount;
    float unkF8;
} WifImage;

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
