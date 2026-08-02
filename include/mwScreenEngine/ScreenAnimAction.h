#ifndef MWSCREENENGINE_SCREENANIMACTION_H
#define MWSCREENENGINE_SCREENANIMACTION_H

#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/ScreenAnimScene.h"

/*
 * ScreenAnimAction.o -- anim control ScreenAction subclasses.
 *
 * CreateAction ids (ScreenActionStack::CreateAction):
 *   0x3F8  ScreenPlayAnimAction(Forward)  -- params: scene index [, resetTime bool]
 *   0x3FB  ScreenPlayAnimAction(Reverse)
 *   0x3F9  ScreenStopAnimAction
 *   0x3FC  ScreenSetAnimSpeedAction       -- params: scene index, speed float
 *   0x3FD  ScreenPlayAnimUntilAction
 *   0x3FE  ScreenSnapAnimAction           -- params: scene index, time int
 *   0x400  ScreenWaitAnimAction           -- wait until scene not playing
 *   0x420  ScreenWaitAnimAction (cond)    -- wait until playing==GetBoolean(1)
 *
 * Mode-select / Idle contract (host):
 *   Engine Idle fires object event **0x405** (ProcessIdleEvent / HasEvent),
 *   which is NOT PlayAnim -- CreateAction maps 0x405 to ScreenUserConfirmAction.
 *   Do **not** Play all anim scenes on screen open. Retail only runs scenes
 *   that SCREEN actions explicitly Play (0x3F8/0x3FB/...) or Snap/Until.
 *   Wait (0x400/0x420) gates sub-actions on GetState()==playing.
 */

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
    virtual ~ScreenSetAnimSpeedAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenStopAnimAction : public ScreenAction {
public:
    virtual ~ScreenStopAnimAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenWaitAnimAction : public ScreenAction {
public:
    virtual ~ScreenWaitAnimAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenSnapAnimAction : public ScreenAction {
public:
    virtual ~ScreenSnapAnimAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
};

class ScreenPlayAnimUntilAction : public ScreenAction {
public:
    virtual ~ScreenPlayAnimUntilAction();
    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);

    unsigned int field_0x3C; /* +0x3C -- CreateAction zeros */
    unsigned int field_0x40; /* +0x40 -- CreateAction zeros */
};

#endif
