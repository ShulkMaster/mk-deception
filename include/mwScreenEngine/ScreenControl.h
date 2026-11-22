#ifndef MWSCREENENGINE_SCREENCONTROL_H
#define MWSCREENENGINE_SCREENCONTROL_H

#include "mwScreenEngine/ScreenObject.h"

class GameVariables;
class GameVariableDispatcher;
class ScreenMgr;
class ScreenAction;
class Screen;
class ScreenParams;
class ScreenRenderInfo;

enum {
    kScreenArgCtrlRefreshSelf = 0x7db,
    kScreenArgCtrlFwdCollection = 0x7e0,
    kScreenArgCtrlFwdOption = 0x7e1,
    kScreenArgCtrlRefreshAllOptions = 0x7e3,
    kScreenArgCtrlRefreshAllCollections = 0x7e4,
    kScreenArgCtrlSetId98 = 0x7ee,
    kScreenArgCtrlSetId90 = 0x7ef,
    kScreenArgRefreshCollection = 0x7d0,
    kScreenArgRefreshOption = 0x7d1
};

class ScreenControl : public ScreenObject {
public:
    ScreenControl();
    virtual ~ScreenControl();

    virtual void Init();
    virtual void Dispose();
    virtual void Render(ScreenRenderInfo* info);

    static void RegisterGameVariables(unsigned int unused, GameVariables* vars);

    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    static void* operator new(unsigned long size);
    static void operator delete(void* p);
    virtual void ProcessParams(ScreenParams* params) = 0;
    virtual void Update();
    virtual void RefreshCollection();
    virtual void RefreshOption();

    static void RefreshAllCollections(Screen* screen);
    static void RefreshAllOptions(Screen* screen);
    void RefreshAllCollections();
    void RefreshAllOptions();

    static GameVariableDispatcher* m_pGameVariables;

    int m_collectionId;
    int m_gvContext;
    int m_optionId;
    int m_pad9C;
    int field_0xA0;
};

#endif
