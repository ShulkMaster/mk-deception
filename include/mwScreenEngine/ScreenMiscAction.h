#ifndef MWSCREENENGINE_SCREENMISCACTION_H
#define MWSCREENENGINE_SCREENMISCACTION_H

#include "mwScreenEngine/ScreenAction.h"

/*
 * ScreenMiscAction.o -- Visible / Enable / Focus / Confirm / Question / Else /
 * BlockEvents / SendObjectEvent / SetForward (mwScreenEngine).
 *
 * Most subclasses are size 0x3C (ScreenAction only). Two grow to 0x40:
 *   ScreenElseAction::m_takeElse (+0x3C) -- CreateAction sets 1; Question's
 *     ProcessSubActions clears the next Else (arg 0x429) when the if fires.
 *   ScreenBlockEventsUntilAction::m_elapsed (+0x3C) -- accumulates dt.
 *
 * Vtbl slots match ScreenAction (dtor / Update / Init).
 *
 * Wave C -- ScreenAction::m_arg values used by these Updates:
 *   0x3F6 SetFocus / 0x42F ClearFocus (ScreenSetFocusAction)
 *   0x3FA SetConfirmUser / 0x404..0x407 stage ops (ScreenUserConfirmAction)
 *   0x408 BroadcastEvent (ScreenSendObjectEventAction)
 * Leave-menu mode latch is NOT here -- mkScreenEngineClient::HandleAction
 * m_arg 0x1389 writes target_game_mode (see mwScreenEngineGlue.h).
 */

class ScreenMgr;
class ScreenActionStack;

/* ScreenAction::m_arg ids for docs (same values as .cpp private enum). */
enum {
    kScreenArgSetFocus = 0x3f6,
    kScreenArgSetConfirmUser = 0x3fa,
    kScreenArgEnableObject = 0x403,
    kScreenArgResetStage = 0x404,
    kScreenArgSetStage = 0x405,
    kScreenArgIncStage = 0x406,
    kScreenArgDecStage = 0x407,
    kScreenArgBroadcastEvent = 0x408,
    kScreenArgClearFocus = 0x42f,
    kScreenArgSetScreenVisible = 0x430
};

/* Integer compare ops used by ScreenQuestionAction (param-driven). */
int ScreenIntegerCompare(int lhs, int op, int rhs);

class ScreenVisibleAction : public ScreenAction {
public:
    virtual ~ScreenVisibleAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class SetScreenVisibleAction : public ScreenAction {
public:
    virtual ~SetScreenVisibleAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenElseAction : public ScreenAction {
public:
    virtual ~ScreenElseAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    unsigned int m_takeElse; /* +0x3C -- 1 until Question clears sibling Else */
};

class ScreenQuestionAction : public ScreenAction {
public:
    virtual ~ScreenQuestionAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenEnableAction : public ScreenAction {
public:
    virtual ~ScreenEnableAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenUserConfirmAction : public ScreenAction {
public:
    virtual ~ScreenUserConfirmAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenSendObjectEventAction : public ScreenAction {
public:
    virtual ~ScreenSendObjectEventAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenSetForwardAction : public ScreenAction {
public:
    virtual ~ScreenSetForwardAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenBlockEventsUntilAction : public ScreenAction {
public:
    virtual ~ScreenBlockEventsUntilAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    int m_elapsed; /* +0x3C -- CreateAction zeros; Update adds dt */
};

class ScreenSetFocusAction : public ScreenAction {
public:
    virtual ~ScreenSetFocusAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

#endif
