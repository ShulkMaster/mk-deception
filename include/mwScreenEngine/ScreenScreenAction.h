#ifndef MWSCREENENGINE_SCREENSCREENACTION_H
#define MWSCREENENGINE_SCREENSCREENACTION_H

#include "mwScreenEngine/ScreenAction.h"

/*
 * Screen*ScreenAction family -- Open / Exit / Replace / Transition / Insert /
 * Close screen stack ops (mwScreenEngine ScreenScreenAction.o).
 *
 * ScreenBaseScreenAction extends ScreenAction (0x3C) with phase state:
 *   +0x3C m_phase
 *   +0x40 m_flag          (Transition: close-top bool)
 *   +0x44 m_screenName[0x80]
 *   +0xC4 m_screen
 *   +0xC8 m_loadResult
 *   +0xCC subclass-specific (int, or Insert's second name buffer)
 *
 * Transition adds m_screenName2 at +0xD0.
 * Insert reuses +0xCC as m_anchorName[0x80], then +0x14C / +0x150.
 *
 * Vtbl slots match ScreenAction (dtor / Update / Init).
 */

class Screen;
class ScreenMgr;
class ScreenActionStack;

class ScreenBaseScreenAction : public ScreenAction {
public:
    ScreenBaseScreenAction() {
        m_phase = 0;
        m_flag = 0;
        m_screen = 0;
        m_loadResult = 0;
        field_0x14 = 1;
    }
    virtual ~ScreenBaseScreenAction() {}

    int m_phase; /* +0x3C */
    unsigned int m_flag; /* +0x40 */
    char m_screenName[0x80]; /* +0x44 */
    Screen* m_screen; /* +0xC4 */
    unsigned int m_loadResult; /* +0xC8 */
};

class ScreenOpenScreenAction : public ScreenBaseScreenAction {
public:
    ScreenOpenScreenAction();
    virtual ~ScreenOpenScreenAction();

    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenExitScreenAction : public ScreenBaseScreenAction {
public:
    ScreenExitScreenAction();
    virtual ~ScreenExitScreenAction();

    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    int m_stageValue; /* +0xCC -- optional GetInt(1) when m_arg == 0x3EE */
};

class ScreenReplaceScreenAction : public ScreenBaseScreenAction {
public:
    ScreenReplaceScreenAction();
    virtual ~ScreenReplaceScreenAction();

    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    int m_insertIndex; /* +0xCC */
};

class ScreenTransitionScreenAction : public ScreenBaseScreenAction {
public:
    ScreenTransitionScreenAction();
    virtual ~ScreenTransitionScreenAction();

    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    int m_preloadId; /* +0xCC */
    char m_screenName2[0x80]; /* +0xD0 */
};

class ScreenInsertScreenAction : public ScreenBaseScreenAction {
public:
    ScreenInsertScreenAction();
    virtual ~ScreenInsertScreenAction();

    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    char m_anchorName[0x80]; /* +0xCC */
    int m_stackIndex; /* +0x14C */
    int m_insertMode; /* +0x150 -- 0 => insert after anchor */
};

class ScreenCloseScreenAction : public ScreenBaseScreenAction {
public:
    ScreenCloseScreenAction();
    virtual ~ScreenCloseScreenAction();

    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

#endif
