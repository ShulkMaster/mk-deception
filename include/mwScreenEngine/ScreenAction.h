#ifndef MWSCREENENGINE_SCREENACTION_H
#define MWSCREENENGINE_SCREENACTION_H

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

    unsigned int m_alive;
    unsigned int m_yield;
    unsigned int m_blocksEvents;
    unsigned int m_stepDone;
    unsigned int field_0x14;
    ScreenEvent* m_event;
    int m_arg;
    int m_eventIndex;
    unsigned int m_id;
    unsigned int m_flags;
    ScreenObject* m_object;
    ScreenParams* m_params;
    ScreenAction* m_prev;
    ScreenAction* m_next;
};

void __ct__12ScreenActionFv(void* action);
void* __nw__12ScreenActionFUl(unsigned long size);
#endif
