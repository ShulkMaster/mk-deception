#ifndef MWSCREENENGINE_SCREENACTION_H
#define MWSCREENENGINE_SCREENACTION_H

/*
 * ScreenAction -- base action instance (size 0x3C).
 *
 * Vtbl (RTTI off, MW header at +0x00/+0x04 of the vtable object):
 *   m_vtbl[2] (+0x08)  dtor(this, deleteFlag)
 *   m_vtbl[3] (+0x0C)  Update(this, ScreenMgr*, ScreenActionStack&, dt)
 *   m_vtbl[4] (+0x10)  Init(this, ScreenEvent*, int, ScreenObject*, int,
 *                            ScreenParams*, unsigned int)
 *
 * CreateAction fallback / typed subclasses allocate 0x3C (or larger).
 * Compiler vptr at +0x00; subclasses may still be dispatched via
 * ScreenActionStack / ScreenObject using these slot indices.
 */

class ScreenObject;
class ScreenEvent;
class ScreenParams;
class ScreenMgr;
class ScreenActionStack;

class ScreenAction {
public:
    ScreenAction();
    virtual ~ScreenAction();

    virtual int Update(ScreenMgr* mgr, ScreenActionStack& stack, int dt);
    virtual void Init(ScreenEvent* event, int eventIndex, ScreenObject* object,
                      int arg, ScreenParams* params, unsigned int flags);

    void Clear();

    void* operator new(unsigned long size);
    void operator delete(void* p);

    /* +0x00 vptr */
    unsigned int m_alive; /* +0x04 -- 0 => Process deletes */
    unsigned int m_yield; /* +0x08 -- non-zero + still head => Process stops */
    unsigned int m_blocksEvents; /* +0x0C -- IsActionBlockingEvents */
    unsigned int m_stepDone; /* +0x10 -- non-zero => move to completed list */
    unsigned int field_0x14; /* +0x14 -- Init clears to 0 */
    ScreenEvent* m_event; /* +0x18 -- Clear nulls; Init stores event */
    int m_arg; /* +0x1C -- Init fourth arg (int after ScreenObject*) */
    int m_eventIndex; /* +0x20 -- Init second arg; Clear sets -1 */
    unsigned int m_id; /* +0x24 -- action type id (ProcessRegisterActions) */
    unsigned int m_flags; /* +0x28 -- Init unsigned flags */
    ScreenObject* m_object; /* +0x2C */
    ScreenParams* m_params; /* +0x30 */
    ScreenAction* m_prev; /* +0x34 */
    ScreenAction* m_next; /* +0x38 */
};

#endif
