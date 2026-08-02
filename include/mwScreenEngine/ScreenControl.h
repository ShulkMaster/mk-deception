#ifndef MWSCREENENGINE_SCREENCONTROL_H
#define MWSCREENENGINE_SCREENCONTROL_H

#include "mwScreenEngine/ScreenObject.h"

/*
 * ScreenControl -- binds GameVariables into the screen engine (extends
 * ScreenObject; retail size past 0x90 with option/collection ids).
 *
 * Retail MUST-RUN:
 *   RegisterGameVariables(unused, vars) -- init_screen_engine chains Glue
 *   BSS `game_variables` (0x1C) onto m_pGameVariables dispatcher.
 *   HandleAction / RefreshAll* -- mode-select control nodes ('SCtl') refresh
 *   option/collection lists when pad focus moves. Leave-menu confirm
 *   (m_arg 0x1389 -> set_target_game_mode) stays on mkScreenEngineClient.
 *
 * HandleAction m_arg ids (retail jump table 0x7db..0x7ef):
 *   0x7DB  refresh this or forward HandleAction to param node
 *   0x7E0  rewrite as 0x7D0 + forward (RefreshCollection subclass)
 *   0x7E1  rewrite as 0x7D1 + forward (RefreshOption subclass)
 *   0x7E3  RefreshAllOptions()
 *   0x7E4  RefreshAllCollections()
 *   0x7EE  set +0x98 + RefreshOption()
 *   0x7EF  set +0x90 + RefreshCollection()
 *   (emit SetId90 case before SetId98 for Matching -- Q20)
 *
 * Vtbl after ScreenObject::HandleEvent:
 *   +0x40 ProcessParams (pure / null on base; TextList etc. override)
 *   +0x44 Update / +0x48 RefreshCollection / +0x4C RefreshOption
 * RefreshCollection/RefreshOption bodies are weak in Glue (blr stubs).
 */

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
    /* Args: 0x7EE -> +0x98/RefreshOption; 0x7EF -> +0x90/RefreshCollection.
     * Emit SetId90 case before SetId98 so .text matches retail (Q20). */
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

    /* Static: r3 unused, r4 = GameVariables* (retail preserves r4 into Register). */
    static void RegisterGameVariables(unsigned int unused, GameVariables* vars);

    virtual int HandleAction(ScreenMgr* mgr, const ScreenAction* action);
    static void* operator new(unsigned long size);
    static void operator delete(void* p);
    /* Pure: retail vtbl +0x40 is null; Glue subclasses override. */
    virtual void ProcessParams(ScreenParams* params) = 0;
    virtual void Update();
    /* Bodies are weak blr stubs in Glue -- do not define in ScreenControl.cpp. */
    virtual void RefreshCollection();
    virtual void RefreshOption();

    /* Static: retail no-arg wrappers load m_screen into r3 (not r4). */
    static void RefreshAllCollections(Screen* screen);
    static void RefreshAllOptions(Screen* screen);
    void RefreshAllCollections();
    void RefreshAllOptions();

    static GameVariableDispatcher* m_pGameVariables;

    /* ScreenObject ends at 0x90; control ids follow. */
    int m_collectionId; /* +0x90 -- init -1; action 0x7EF + RefreshCollection */
    int m_gvContext; /* +0x94 -- Dispatcher unused arg (ImageList Free/GetTexture) */
    int m_optionId; /* +0x98 -- init -1; action 0x7EE + RefreshOption */
    int m_pad9C; /* +0x9C */
    int field_0xA0; /* +0xA0 -- init 0; purpose not confirmed */
};

#endif
