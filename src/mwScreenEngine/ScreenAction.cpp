#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenMgr.h"
#include "mwScreenEngine/ScreenUtil.h"

ScreenAction::ScreenAction() {
    m_object = 0;
    m_params = 0;
    m_stepDone = 0;
    m_arg = 0;
    m_flags = 0;
    m_alive = 0;
    m_yield = 0;
    m_blocksEvents = 0;
    m_prev = 0;
}

ScreenAction::~ScreenAction() {
    /* Retail deleting dtor: restore vptr then optional operator delete. */
}

void ScreenAction::Clear() {
    m_event = 0;
    m_object = 0;
    m_params = 0;
    m_eventIndex = -1;
}

void ScreenAction::Init(ScreenEvent* event, int eventIndex, ScreenObject* object,
                        int arg, ScreenParams* params, unsigned int flags) {
    m_event = event;
    m_arg = arg;
    m_eventIndex = eventIndex;
    m_flags = flags;
    m_object = object;
    m_params = params;
    m_alive = 1;
    m_yield = 1;
    field_0x14 = 0;
}

int ScreenAction::Update(ScreenMgr* mgr, ScreenActionStack& /*stack*/,
                         int /*dt*/) {
    int handled;
    ScreenObject* object;

    handled = 1;
    object = m_object;
    if (object != 0) {
        handled = object->HandleAction(mgr, this);
    }

    m_alive = 0;
    m_yield = 0;

    if ((unsigned int)mgr->ProcessRegisterActions(this) != 0) {
        return 1;
    }

    ScreenUtil::HandleAction(mgr, this, handled);
    return 1;
}

void* ScreenAction::operator new(unsigned long size) {
    char* name;
    int tag;

    name = (char*)"Action";
    tag = 0x444e5941;
    return ScreenUtil::Malloc(size, tag, name);
}

void ScreenAction::operator delete(void* p) {
    ScreenUtil::Free(p);
}
