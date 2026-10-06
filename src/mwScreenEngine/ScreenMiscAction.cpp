/* BUILD: -inline off: keep bl ScreenIntegerCompare from Question Update. */

#include "mwScreenEngine/ScreenMiscAction.h"
#include "mwScreenEngine/ScreenActionStack.h"
#include "mwScreenEngine/ScreenControl.h"
#include "mwScreenEngine/ScreenEvent.h"
#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenMgr.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenParams.h"
#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/GameVariables.h"


enum {
    kArgSetFocus = 0x3f6,
    kArgSetConfirmUser = 0x3fa,
    kArgEnableObject = 0x403,
    kArgResetStage = 0x404,
    kArgSetStage = 0x405,
    kArgIncStage = 0x406,
    kArgDecStage = 0x407,
    kArgBroadcastEvent = 0x408,
    kArgQuestionVisible = 0x41a,
    kArgQuestionFocus = 0x41b,
    kArgQuestionEnabled = 0x41c,
    kArgQuestionStage = 0x41d,
    kArgQuestionAllStages = 0x41e,
    kArgQuestionAnyStage = 0x41f,
    kArgElse = 0x429,
    kArgClearFocus = 0x42f,
    kArgSetScreenVisible = 0x430,
    kArgQuestionGameVar = 0x2af9,
};


enum { kObjectFlagEnabled = 0x2 };

enum ScreenCompareOp {
    kCompareEqual = 0,
    kCompareNotEqual = 1,
    kCompareGreater = 2,
    kCompareGreaterEqual = 3,
    kCompareLess = 4,
    kCompareLessEqual = 5,
};

int ScreenVisibleAction::Update(ScreenMgr*, ScreenActionStack&,
                                int) {
    ScreenParams* params;
    ScreenNode* node;
    int visible;

    params = m_params;
    if (params != 0) {
        node = params->GetScreenNode(0);
        visible = params->GetBoolean(1);
        node->SetVisible((unsigned int)visible);
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}

int SetScreenVisibleAction::Update(ScreenMgr* mgr, ScreenActionStack&,
                                   int) {
    ScreenParams* params;
    char* name;
    int visible;
    Screen* screen;
    ScreenObject* object;

    params = m_params;
    if (params != 0) {
        name = params->GetScreenName(0);
        visible = params->GetBoolean(1);
        mgr->FindScreen(name, &screen);
        if (screen != 0) {
            screen->m_visible = visible;
        }
    }

    if (m_arg == kArgSetScreenVisible) {
        object = m_object;
        if (object != 0) {
            screen = object->m_screen;
            screen->m_visible = 1;
            screen->UpdateSceneAnimation(0);
        }
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}

unsigned int ScreenIntegerCompare(int lhs, int op, int rhs) {
    unsigned int result;

    switch (op) {
    case kCompareEqual:
        result = lhs == rhs;
        break;
    case kCompareNotEqual:
        result = lhs != rhs;
        break;
    case kCompareGreater:
        result = lhs > rhs;
        break;
    case kCompareGreaterEqual:
        result = lhs >= rhs;
        break;
    case kCompareLess:
        result = lhs < rhs;
        break;
    case kCompareLessEqual:
        result = lhs <= rhs;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

int ScreenElseAction::Update(ScreenMgr*, ScreenActionStack&,
                             int) {
    ScreenObject* object;

    object = m_object;
    m_alive = 0;
    m_yield = 0;

    if (object != 0 && m_takeElse != 0) {
        object->ProcessSubActions(this, 0);
    }
    return 1;
}

#pragma push
#pragma optimization_level 2
/* TODO: [near miss] 98.79%; owner rotation reduced; case-constant staging and remaining value GPR pairs differ. */
int ScreenQuestionAction::Update(ScreenMgr* mgr, ScreenActionStack&,
                                 int) {
    ScreenParams* params;
    ScreenObject* object;
    int arg;
    unsigned int paramIndex;
    int lhs;
    unsigned int matched;
    int exclude;
    int i;

    params = m_params;
    m_alive = 0;
    m_yield = 0;
    if (params == 0) {
        return 1;
    }

    arg = m_arg;
    object = m_object;

    switch (arg) {
    case kArgQuestionVisible:
    {
        paramIndex = 1;
        ScreenNode* node = params->GetScreenNode(0);
        lhs = node->IsVisible();
        break;
    }
    case kArgQuestionFocus:
    {
        int focusIndex;

        ScreenObject* probe = params->GetScreenObject(0);
        paramIndex = 2;
        focusIndex = params->GetInt(1);
        lhs = (probe->m_parent->GetFocus(focusIndex) == probe);
        break;
    }
    case kArgQuestionEnabled:
    {
        paramIndex = 1;
        ScreenObject* probe = params->GetScreenObject(0);
        lhs = (probe->m_ext->flags >> 1) & 1;
        break;
    }
    case kArgQuestionStage:
    {
        paramIndex = 1;
        int stage = params->GetInt(0);
        if (stage == 0) {
            stage = m_flags;
        }
        lhs = mgr->GetStage(stage);
        break;
    }
    case kArgQuestionGameVar:
    {
        int resourceId;

        paramIndex = 1;
        resourceId = params->GetResourceID(0);
        lhs = ScreenControl::m_pGameVariables->GetInt(resourceId);
        break;
    }
    default:
        return 1;
    }

    int op = params->GetInt(paramIndex++);
    int rhs = params->GetInt(paramIndex++);

    if (arg == kArgQuestionAllStages) {
        exclude = params->GetInt(paramIndex);
        matched = 1;
        i = 3;
        do {
            if (exclude != mgr->GetStage(i)) {
                matched = ScreenIntegerCompare(mgr->GetStage(i), op, rhs);
                if (matched == 0) {
                    break;
                }
            }
        } while (i-- != 0);
    } else if (arg == kArgQuestionAnyStage) {
        i = 3;
        do {
            matched = ScreenIntegerCompare(mgr->GetStage(i), op, rhs);
            if (matched == 1u) {
                break;
            }
        } while (i-- != 0);
    } else {
        matched = ScreenIntegerCompare(lhs, op, rhs);
    }

    if (matched != 0u) {
        object->ProcessSubActions(this, 0);
    }
    return 1;
}
#pragma pop

int ScreenEnableAction::Update(ScreenMgr* mgr, ScreenActionStack&,
                               int) {
    ScreenParams* params;
    ScreenMgr* eventsMgr;

    params = m_params;
    if (params != 0) {
        if (m_arg == kArgEnableObject) {
            ScreenObject* target;
            unsigned int enable;

            if ((int)params->GetCount() == 1) {
                target = m_object;
                enable = params->GetBoolean(0);
            } else {
                target = params->GetScreenObject(0);
                enable = params->GetBoolean(1);
            }
            if (enable != 0) {
                target->m_ext->flags |= kObjectFlagEnabled;
            } else {
                target->m_ext->flags &= ~kObjectFlagEnabled;
            }
        } else {
            Screen* screen;
            ScreenSet* set;

            screen = m_object->m_screen;
            set = screen->m_set;
            eventsMgr = set->m_mgr;
            eventsMgr->m_eventsEnabled = params->GetBoolean(0);
        }
        ScreenUtil::HandleAction(mgr, this, 0);
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}

/* TODO: [near miss] 98.20755%; this/manager/params and scalar saved-register homes differ;
 * branch scoping and scalar declaration order do not close the residue. */
int ScreenUserConfirmAction::Update(ScreenMgr* mgr, ScreenActionStack&,
                                    int) {
    ScreenParams* params;
    int stageIndex;
    int confirmId;
    int value;
    int flag;

    params = m_params;
    if (params != 0) {
        if (m_arg == kArgSetConfirmUser) {
            confirmId = params->GetInt(0);
            value = params->GetInt(1);
            flag = params->GetBoolean(2);
            mgr->SetConfirmUser(value - 1, (unsigned int)(flag != 0), confirmId);
        } else {
            stageIndex = m_flags;
            if (params->GetInt(0) != 0) {
                stageIndex = params->GetInt(0);
            }

            switch (m_arg) {
            case kArgResetStage:
                if (params->GetInt(0) == 0) {
                    mgr->ResetStagesTo(0);
                } else {
                    mgr->SetStage(stageIndex, 0);
                }
                break;
            case kArgSetStage:
                mgr->SetStage(stageIndex, params->GetInt(1));
                break;
            case kArgIncStage:
                mgr->SetStage(stageIndex, mgr->GetStage(stageIndex) + 1);
                break;
            case kArgDecStage:
                mgr->SetStage(stageIndex, mgr->GetStage(stageIndex) - 1);
                break;
            default:
                break;
            }
        }
        ScreenUtil::HandleAction(mgr, this, 0);
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}

int ScreenSendObjectEventAction::Update(ScreenMgr* mgr, ScreenActionStack& stack,
                                        int) {
    ScreenParams* params;
    int eventId;
    ScreenObject* object;

    eventId = 0;
    params = m_params;
    if (params != 0) {
        if (m_arg == kArgBroadcastEvent) {
            eventId = params->GetResourceID(0);
            stack.StartLocal();
            mgr->BroadcastEvent(eventId, 0, (int)m_flags);
            stack.EndLocal();
        } else {
            object = params->GetScreenObject(0);
            eventId = params->GetResourceID(1);
            stack.StartLocal();
            object->FireEvent(mgr, eventId, (int)m_flags, 0);
            stack.EndLocal();
        }
    }

    ScreenUtil::HandleEvent(0, eventId, 0);
    m_alive = 0;
    m_yield = 0;
    return 1;
}

int ScreenSetForwardAction::Update(ScreenMgr*,
                                   ScreenActionStack&, int) {
    ScreenParams* params;
    ScreenObject* object;

    params = m_params;
    if (params != 0) {
        object = params->GetScreenObject(0);
        object->ProcessSubActions(this, 0);
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}

int ScreenBlockEventsUntilAction::Update(ScreenMgr*,
                                         ScreenActionStack&, int dt) {
    ScreenParams* params;
    int duration;
    int elapsed;
    unsigned int flag;

    params = m_params;
    if (params == 0) {
        m_blocksEvents = 0;
        m_alive = 0;
        m_yield = 0;
        m_stepDone = 0;
        return 1;
    }

    duration = params->GetInt(0);
    m_elapsed += dt;
    elapsed = m_elapsed;
    flag = elapsed < duration;
    m_blocksEvents = flag;
    m_alive = flag;
    m_yield = flag;
    m_stepDone = 1;
    return 1;
}

int ScreenSetFocusAction::Update(ScreenMgr* mgr, ScreenActionStack&,
                                 int) {
    ScreenParams* params;
    int focusIndex;
    ScreenObject* target;
    ScreenObject* prev;
    ScreenObject* parent;

    focusIndex = -1;
    params = m_params;
    if (params == 0) {
        m_alive = 0;
        m_yield = 0;
        return 1;
    }

    if (m_arg == kArgSetFocus) {
        focusIndex = params->GetInt(0);
        target = params->GetScreenObject(1);
        parent = target->m_parent;
    } else if (m_arg == kArgClearFocus) {
        target = params->GetScreenObject(0);
        target->ClearActiveObjects();
        m_alive = 0;
        m_yield = 0;
        return 1;
    } else {
        target = params->GetScreenObject(0);
        parent = target->m_parent;
        if (target->m_parent != 0) {
            focusIndex = m_flags;
            if (focusIndex > 0 && parent->GetFocus(focusIndex) == 0) {
                focusIndex = 0;
            }
        }
    }

    if (parent != 0) {
        prev = 0;
        while (target != 0 && prev != target) {
            if ((target->m_ext->flags & kObjectFlagEnabled) != 0) {
                break;
            }
            prev = target;
            target = target->FindNextFocusObject(m_event->m_id);
        }

        if (target != 0 &&
            (target->m_ext->flags & kObjectFlagEnabled) != 0 &&
            parent != 0 && target != parent->GetFocus(focusIndex)) {
            parent->SetFocus(mgr, target, focusIndex, 1);
        }
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}



ScreenSetFocusAction::~ScreenSetFocusAction() {}

ScreenBlockEventsUntilAction::~ScreenBlockEventsUntilAction() {}

ScreenSetForwardAction::~ScreenSetForwardAction() {}

ScreenSendObjectEventAction::~ScreenSendObjectEventAction() {}

ScreenUserConfirmAction::~ScreenUserConfirmAction() {}

ScreenEnableAction::~ScreenEnableAction() {}

ScreenQuestionAction::~ScreenQuestionAction() {}

ScreenElseAction::~ScreenElseAction() {}

SetScreenVisibleAction::~SetScreenVisibleAction() {}

ScreenVisibleAction::~ScreenVisibleAction() {}
