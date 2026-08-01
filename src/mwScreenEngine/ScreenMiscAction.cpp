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

/* Action type ids stored in ScreenAction::m_arg (CreateAction / Init). */
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

/* SEObjectExt::flags bit1 -- toggled by ScreenEnableAction. */
enum { kObjectFlagEnabled = 0x2 };

enum ScreenCompareOp {
    kCompareEqual = 0,
    kCompareNotEqual = 1,
    kCompareLess = 2,
    kCompareGreaterEqual = 3,
    kCompareGreater = 4,
    kCompareLessEqual = 5,
};

int ScreenVisibleAction::Update(ScreenMgr* /*mgr*/, ScreenActionStack& /*stack*/,
                                int /*dt*/) {
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

int SetScreenVisibleAction::Update(ScreenMgr* mgr, ScreenActionStack& /*stack*/,
                                   int /*dt*/) {
    ScreenParams* params;
    char* name;
    int visible;
    Screen* screen;
    ScreenObject* object;

    params = m_params;
    if (params != 0) {
        name = const_cast<char*>(params->GetScreenName(0));
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

int ScreenIntegerCompare(int lhs, int op, int rhs) {
    /*
     * Soft ceiling ~95%: retail < (op2) / > (op4) xor operand order is
     * swapped vs MWCC's emission of lhs<rhs / lhs>rhs; stop.
     */
    switch (op) {
    case kCompareEqual:
        return lhs == rhs;
    case kCompareNotEqual:
        return lhs != rhs;
    case kCompareLess:
        return lhs < rhs;
    case kCompareGreaterEqual:
        return lhs >= rhs;
    case kCompareGreater:
        return lhs > rhs;
    case kCompareLessEqual:
        return lhs <= rhs;
    default:
        return 0;
    }
}

int ScreenElseAction::Update(ScreenMgr* /*mgr*/, ScreenActionStack& /*stack*/,
                             int /*dt*/) {
    ScreenObject* object;

    object = m_object;
    m_alive = 0;
    m_yield = 0;

    if (object != 0 && m_takeElse != 0) {
        object->ProcessSubActions(this, 0);
    }
    return 1;
}

int ScreenQuestionAction::Update(ScreenMgr* mgr, ScreenActionStack& /*stack*/,
                                 int /*dt*/) {
    ScreenParams* params;
    ScreenObject* object;
    int arg;
    int paramIndex;
    int lhs;
    int op;
    int rhs;
    int matched;
    int stage;
    int exclude;
    int i;
    ScreenObject* probe;
    ScreenNode* node;

    params = m_params;
    m_alive = 0;
    m_yield = 0;
    if (params == 0) {
        return 1;
    }

    arg = m_arg;
    object = m_object;

    /* Sparse action ids lower to the retail cmpwi/beq/bge decision tree. */
    switch (arg) {
    case kArgQuestionVisible:
        paramIndex = 1;
        node = params->GetScreenNode(0);
        lhs = (int)node->IsVisible();
        break;
    case kArgQuestionFocus:
    {
        int focusIndex;

        probe = params->GetScreenObject(0);
        paramIndex = 2;
        focusIndex = params->GetInt(1);
        lhs = (probe == probe->m_parent->GetFocus(focusIndex));
        break;
    }
    case kArgQuestionEnabled:
        paramIndex = 1;
        probe = params->GetScreenObject(0);
        lhs = (int)((probe->m_ext->flags >> 1) & 1);
        break;
    case kArgQuestionStage:
        paramIndex = 1;
        stage = params->GetInt(0);
        if (stage == 0) {
            stage = (int)m_flags;
        }
        lhs = mgr->GetStage(stage);
        break;
    case kArgQuestionGameVar:
    {
        int resourceId;

        paramIndex = 1;
        resourceId = params->GetResourceID(0);
        lhs = ScreenControl::m_pGameVariables->GetInt(resourceId);
        break;
    }
    default:
        /* Includes 0x41e/0x41f; loops below remain dead in retail. */
        return 1;
    }

    op = params->GetInt((unsigned int)paramIndex++);
    rhs = params->GetInt((unsigned int)paramIndex++);

    /*
     * Retail still emits AllStages (0x41e) / AnyStage (0x41f) after op/rhs.
     * Those m_arg values return above, so the loops are dead in the DOL --
     * keep them so MWCC emits the same bytes.
     */
    if (arg == kArgQuestionAllStages) {
        exclude = params->GetInt((unsigned int)paramIndex);
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
            if ((unsigned int)matched == 1u) {
                break;
            }
        } while (i-- != 0);
    } else {
        matched = ScreenIntegerCompare(lhs, op, rhs);
    }

    /* Retail: cmplwi matched; no null-check on m_object. */
    if ((unsigned int)matched != 0u) {
        object->ProcessSubActions(this, 0);
    }
    return 1;
}

int ScreenEnableAction::Update(ScreenMgr* mgr, ScreenActionStack& /*stack*/,
                               int /*dt*/) {
    ScreenParams* params;
    ScreenMgr* eventsMgr;

    /* Matching: scope target/enable so else-branch eventsMgr reuses params NV (r29). */
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
            /* Non-0x403: write boolean into ScreenMgr::m_eventsEnabled. */
            eventsMgr = m_object->m_screen->m_set->m_mgr;
            eventsMgr->m_eventsEnabled = params->GetBoolean(0);
        }
        ScreenUtil::HandleAction(mgr, this, 0);
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}

int ScreenUserConfirmAction::Update(ScreenMgr* mgr, ScreenActionStack& /*stack*/,
                                    int /*dt*/) {
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
            /* Retail: subic/subfe/clrlwi coerces GetBoolean to 0/1.
             * Soft ceiling ~98%: remaining NV color; stop. */
            mgr->SetConfirmUser(value - 1, (unsigned int)(flag != 0), confirmId);
        } else {
            stageIndex = (int)m_flags;
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
                                        int /*dt*/) {
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

int ScreenSetForwardAction::Update(ScreenMgr* /*mgr*/,
                                   ScreenActionStack& /*stack*/, int /*dt*/) {
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

int ScreenBlockEventsUntilAction::Update(ScreenMgr* /*mgr*/,
                                         ScreenActionStack& /*stack*/, int dt) {
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
    /*
     * Soft ceiling ~99.08%: xor/and temp color (retail xor r5 vs our xor r4);
     * same math (duration^elapsed branchless <). Tried operand swap, fold,
     * idiomatic < (subfc -7%), signed/decl, two-step xor. Stop (F coloring).
    */
    {
        unsigned int x;
        unsigned int anded;
        int diff;

        x = (unsigned int)duration ^ (unsigned int)elapsed;
        anded = x & (unsigned int)duration;
        diff = ((int)x >> 1) - (int)anded;
        flag = (unsigned int)diff >> 31;
    }
    m_blocksEvents = flag;
    m_alive = flag;
    m_yield = flag;
    m_stepDone = 1;
    return 1;
}

int ScreenSetFocusAction::Update(ScreenMgr* mgr, ScreenActionStack& /*stack*/,
                                 int /*dt*/) {
    ScreenParams* params;
    int focusIndex;
    ScreenObject* target;
    ScreenObject* prev;
    ScreenObject* parent;

    /* SetFocus(..., fireEvents=1) emits 0x3ED lose / 0x3EC gain; host must
     * not replace this with visibility toggles only. */
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
            focusIndex = (int)m_flags;
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
            target = static_cast<ScreenObject*>(target->FindNextFocusObject(m_event->m_id));
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

/* Dtors -- retail order (SetFocus .. Visible). */

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
