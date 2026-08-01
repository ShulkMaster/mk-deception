#include "mwScreenEngine/ScreenActionStack.h"
#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/ScreenMiscAction.h"
#include "mwScreenEngine/ScreenScreenAction.h"
#include "mwScreenEngine/ScreenAnimAction.h"

#define SCREEN_ACTION_PLAY_UNTIL 0x3FD
#define SCREEN_ACTION_BLOCK_UNTIL 0x3FF
#define SCREEN_ACTION_ELSE 0x429

ScreenActionStack::ScreenActionStack() {
    m_mgr = 0;
    m_processing = 0;
    m_head = 0;
    m_tail = 0;
    m_localHead = 0;
    m_localTail = 0;
    m_localMode = 0;
}

int ScreenActionStack::IsActionBlockingEvents() {
    ScreenAction* action;

    for (action = m_head; action != 0; action = action->m_next) {
        if (action->m_alive != 0 && action->m_blocksEvents != 0) {
            return 1;
        }
    }
    return 0;
}

void ScreenActionStack::StartLocal() {
    m_localMode = 1;
}

int ScreenActionStack::EndLocal() {
    ScreenAction* localTail;
    ScreenAction* head;

    if (m_localMode == 1) {
        m_localMode = 0;
        localTail = m_localTail;
        if (localTail == 0) {
            return 0;
        }
        localTail->m_next = m_head;
        head = m_head;
        if (head != 0) {
            head->m_prev = m_localTail;
            m_head = m_localHead;
        } else {
            m_head = m_localHead;
            m_tail = m_localTail;
        }
        m_localHead = 0;
        m_localTail = 0;
        return 1;
    }
    return 0;
}

/* Retail duplicates main vs local paths (no head/tail pointer temps). */
void ScreenActionStack::PushAction(ScreenAction* action) {
    ScreenAction* tail;

    if (m_localMode == 0) {
        tail = m_tail;
        if (tail == 0) {
            m_head = action;
            m_tail = action;
            action->m_next = 0;
            action->m_prev = 0;
            return;
        }
        tail->m_next = action;
        action->m_prev = m_tail;
        action->m_next = 0;
        m_tail = action;
        return;
    }

    tail = m_localTail;
    if (tail == 0) {
        m_localHead = action;
        m_localTail = action;
        action->m_next = 0;
        action->m_prev = 0;
        return;
    }
    tail->m_next = action;
    action->m_prev = m_localTail;
    action->m_next = 0;
    m_localTail = action;
}

void ScreenActionStack::Dispose() {
    ScreenAction* action;
    ScreenAction* cur;

    action = m_tail;
    while (action != 0) {
        cur = action;
        action = cur->m_prev;
        delete cur;
    }

    action = m_localTail;
    while (action != 0) {
        cur = action;
        action = cur->m_prev;
        delete cur;
    }

    m_localHead = 0;
    m_localTail = 0;
    m_tail = 0;
    m_head = 0;
    m_processing = 0;
    m_localMode = 0;
}

/* dont_inline: RemoveActions must bl ClearActions (not inline the body). */
#pragma dont_inline on
void ScreenActionStack::ClearActions(ScreenSet* set, ScreenAction** pTail,
                                     ScreenAction** pHead) {
    ScreenAction* action;
    ScreenAction* prev;
    ScreenAction* next;
    ScreenObject* obj;

    action = *pTail;
    while (action != 0) {
        prev = action->m_prev;
        obj = action->m_object;
        if (obj != 0 && obj->m_screen->m_set == set) {
            next = action->m_next;
            if (action->m_stepDone != 0) {
                if (next != 0) {
                    next->m_prev = prev;
                }
                if (prev != 0) {
                    prev->m_next = next;
                }
                if (*pHead == action) {
                    *pHead = next;
                }
                if (*pTail == action) {
                    *pTail = prev;
                }
                delete action;
            } else {
                action->Clear();
            }
        }
        action = prev;
    }
}
#pragma dont_inline reset

void ScreenActionStack::RemoveActions(ScreenSet* set) {
    ClearActions(set, &m_tail, &m_head);
    ClearActions(set, &m_localTail, &m_localHead);
}

void ScreenActionStack::Process(ScreenMgr* mgr, int dt) {
    ScreenAction* action;
    ScreenAction* next;
    ScreenAction* prev;

    if (m_processing == 1) {
        return;
    }

    ScreenAction* completedTail;
    ScreenAction* completedHead;

    m_processing = 1;
    completedHead = 0;
    completedTail = 0;
    m_mgr = mgr;

    while ((action = m_head) != 0) {
        if (action->m_alive == 0) {
            m_head = action->m_next;
            if (m_head != 0) {
                m_head->m_prev = 0;
            } else {
                m_tail = 0;
            }
            delete action;
            continue;
        }

        action->Update(mgr, *this, dt);

        if (action->m_alive == 0) {
            continue;
        }

        if (m_head == action && action->m_yield != 0) {
            break;
        }

        if (action->m_stepDone == 0) {
            continue;
        }

        next = action->m_next;
        prev = action->m_prev;
        if (next != 0) {
            next->m_prev = prev;
        }
        if (prev != 0) {
            prev->m_next = next;
        }

        if (completedTail == 0) {
            completedHead = action;
        } else {
            completedTail->m_next = action;
            action->m_prev = completedTail;
        }

        m_head = action->m_next;
        completedTail = action;
    }

    if (completedTail != 0) {
        completedTail->m_next = 0;
        completedHead->m_prev = 0;
        m_head = completedHead;
        m_tail = completedTail;
    }

    m_processing = 0;
}

/*
 * Soft ceiling: CreateAction ~80% -- switch factory complete (incl. anim FX);
 * leaf new/ctor schedule vs retail inlined vtbl poke / leaf block order; stop.
 */
ScreenAction* ScreenActionStack::CreateAction(unsigned int type) {
    int id;
    ScreenAction* action;
    ScreenElseAction* elseAction;
    ScreenBlockEventsUntilAction* blockAction;
    ScreenPlayAnimUntilAction* untilAction;

    id = (int)type;
    action = ScreenUtil::CreateAction(id);
    if (action != 0) {
        return action;
    }

    switch (id) {
    case 0x3e8:
    case 0x3f6:
    case 0x42f:
        action = new ScreenSetFocusAction();
        break;
    case 0x3ea:
        action = new ScreenVisibleAction();
        break;
    case 0x3eb:
    case 0x3ee:
        action = new ScreenExitScreenAction();
        break;
    case 0x3ec:
    case 0x408:
        action = new ScreenSendObjectEventAction();
        break;
    case 0x3ef:
    case 0x430:
        action = new SetScreenVisibleAction();
        break;
    case 0x3f0:
    case 0x3f1:
        action = new ScreenVisibleAction();
        break;
    case 0x3f2:
        action = new ScreenOpenScreenAction();
        break;
    case 0x3f3:
        action = new ScreenCloseScreenAction();
        break;
    case 0x3f4:
        action = new ScreenReplaceScreenAction();
        break;
    case 0x3f5:
        action = new ScreenTransitionScreenAction();
        break;
    case 0x3f8:
        action = new ScreenPlayAnimAction(kAnimDirectionForward);
        break;
    case 0x3f9:
        action = new ScreenStopAnimAction();
        break;
    case 0x3fa:
    case 0x404:
    case 0x405:
    case 0x406:
    case 0x407:
        action = new ScreenUserConfirmAction();
        break;
    case 0x3fb:
        action = new ScreenPlayAnimAction(kAnimDirectionReverse);
        break;
    case 0x3fc:
        action = new ScreenSetAnimSpeedAction();
        break;
    case SCREEN_ACTION_PLAY_UNTIL:
        untilAction = new ScreenPlayAnimUntilAction();
        if (untilAction != 0) {
            untilAction->field_0x3C = 0;
            untilAction->field_0x40 = 0;
        }
        action = untilAction;
        break;
    case 0x3fe:
        action = new ScreenSnapAnimAction();
        break;
    case SCREEN_ACTION_BLOCK_UNTIL:
        blockAction = new ScreenBlockEventsUntilAction();
        if (blockAction != 0) {
            blockAction->m_elapsed = 0;
        }
        action = blockAction;
        break;
    case 0x400:
    case 0x420:
        action = new ScreenWaitAnimAction();
        break;
    case 0x401:
        action = new ScreenInsertScreenAction();
        break;
    case 0x402:
        action = new ScreenSetForwardAction();
        break;
    case 0x403:
    case 0x431:
        action = new ScreenEnableAction();
        break;
    case 0x41a:
    case 0x41b:
    case 0x41c:
    case 0x41d:
    case 0x41e:
    case 0x41f:
    case 0x2af9:
        action = new ScreenQuestionAction();
        break;
    case SCREEN_ACTION_ELSE:
        elseAction = new ScreenElseAction();
        if (elseAction != 0) {
            elseAction->m_takeElse = 1;
        }
        action = elseAction;
        break;
    default:
        action = 0;
        break;
    }

    if (action == 0) {
        action = new ScreenAction();
    }
    return action;
}

ScreenInsertScreenAction::ScreenInsertScreenAction() {
    m_stackIndex = -1;
}

ScreenReplaceScreenAction::ScreenReplaceScreenAction() {
    m_insertIndex = -1;
}

ScreenCloseScreenAction::ScreenCloseScreenAction() {
}

ScreenOpenScreenAction::ScreenOpenScreenAction() {
}

ScreenExitScreenAction::ScreenExitScreenAction() {
}
