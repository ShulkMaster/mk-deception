#ifndef MWSCREENENGINE_SCREENSET_H
#define MWSCREENENGINE_SCREENSET_H

#include "mwScreenEngine/Screen.h"

class ScreenMgr;
class ScreenResourceLibrary;

class ScreenSet {
public:
    ScreenSet();
    virtual ~ScreenSet();

    int Init();
    void DoneLoadingScreens();
    void Dispose();
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

     int m_unloadId;
     ScreenResourceLibrary* m_resourceLib;
     SEScreenSet_t* m_setData;
     unsigned char m_inited;
     unsigned char m_pad11[3];
     int m_numScreens;
     Screen* m_screens;
     int m_numChildren;
     ScreenSet* m_children[16];
     ScreenSet* m_parent;
     char m_name[0x50];
     unsigned char m_padB4[4];
     ScreenMgr* m_mgr;
};

#endif
