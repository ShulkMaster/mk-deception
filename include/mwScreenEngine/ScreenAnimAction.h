#ifndef MWSCREENENGINE_SCREENANIMACTION_H
#define MWSCREENENGINE_SCREENANIMACTION_H

#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/ScreenAnimScene.h"

class ScreenMgr;
class ScreenActionStack;

class ScreenPlayAnimAction : public ScreenAction {
public:
    ScreenPlayAnimAction(AnimDirectionE direction);
    virtual ~ScreenPlayAnimAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    int m_direction; /* +0x3C -- AnimDirectionE */
};

class ScreenSetAnimSpeedAction : public ScreenAction {
public:
    virtual ~ScreenSetAnimSpeedAction() {}
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenStopAnimAction : public ScreenAction {
public:
    virtual ~ScreenStopAnimAction() {}
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenWaitAnimAction : public ScreenAction {
public:
    virtual ~ScreenWaitAnimAction() {}
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenSnapAnimAction : public ScreenAction {
public:
    virtual ~ScreenSnapAnimAction() {}
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenPlayAnimUntilAction : public ScreenAction {
public:
    ScreenPlayAnimUntilAction() {
        field_0x3C = 0;
        field_0x40 = 0;
    }
    virtual ~ScreenPlayAnimUntilAction() {}
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    unsigned int field_0x3C; /* +0x3C -- ctor zeros */
    unsigned int field_0x40; /* +0x40 -- ctor zeros */
};

#endif
