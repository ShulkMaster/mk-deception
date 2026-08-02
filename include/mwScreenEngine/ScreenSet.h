#ifndef MWSCREENENGINE_SCREENSET_H
#define MWSCREENENGINE_SCREENSET_H

#include "mwScreenEngine/Screen.h"

class ScreenMgr;
class ScreenResourceLibrary;

/*
 * ScreenSet -- named screen group + child tree (retail size 0xBC).
 *
 * Init() -> ScreenUtil::LoadScreenSet (host/client). Retail client path:
 *   sprintf("scr_%s.sec", GetName())
 *   load named binaries "STRINGS" then "SCREEN" from that SEC
 *   ScreenInstancer::LoadSetData(this, screenBlob, size, 0)
 * See ScreenInstancer / SEScreenSet_t in Screen.h.
 */
class ScreenSet {
public:
    ScreenSet();
    virtual ~ScreenSet();

    int Init();
    void DoneLoadingScreens();
    void Dispose();
    /* Soft ceiling: IsInited ~50% - MWCC 2.7 omits retail bool-normalize. */
    int IsInited() const;

    char* GetName();
    void SetName(char* name);

    int GetNumChildren() const;
    ScreenSet* GetChild(int index);
    ScreenSet* GetChild(char* name);
    int GetChildIndex(char* name);
    void RemoveChild(ScreenSet* child);
    void AddChild(ScreenSet* child);

    ScreenSet* GetParent();
    void SetParent(ScreenSet* parent);

    Screen* GetScreen(char* name);
    int GetScreenIndex(char* name);
    void BroadcastEvent(ScreenMgr* mgr, int event, int arg);

    void* operator new(unsigned long size);
    void operator delete(void* p);

    /* +0x00 vptr (virtual dtor) */
    /* +0x04 */ int m_unloadId;
    /* +0x08 */ ScreenResourceLibrary* m_resourceLib;
    /* +0x0C */ SEScreenSet_t* m_setData; /* patched SCREEN binary root */
    /* +0x10 */ unsigned char m_inited;
    /* +0x11 */ unsigned char m_pad11[3];
    /* +0x14 */ int m_numScreens;
    /* +0x18 */ Screen* m_screens;
    /* +0x1C */ int m_numChildren;
    /* +0x20 */ ScreenSet* m_children[16];
    /* +0x60 */ ScreenSet* m_parent;
    /* +0x64 */ char m_name[0x50];
    /* +0xB4 */ unsigned char m_padB4[4];
    /* +0xB8 */ ScreenMgr* m_mgr;
};

#endif
