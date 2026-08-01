#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/ScreenActionStack.h"
#include "mwScreenEngine/ScreenEvent.h"
#include "mwScreenEngine/ScreenMiscAction.h"
#include "mwScreenEngine/ScreenMatrixStack.h"
#include "mwScreenEngine/ScreenMgr.h"
#include "mwScreenEngine/ScreenParams.h"
#include "mwScreenEngine/ScreenUtil.h"

#define SCREEN_EVENT_FOCUS_GAIN 0x3EC
#define SCREEN_EVENT_FOCUS_LOSS 0x3ED
#define SCREEN_ACTION_SET_FOCUS 0x3F6
#define SCREEN_ACTION_MATCH 0x424
#define SCREEN_ACTION_ELSE 0x429

/* Identity basis rows for UpdateTransform (retail @230/@231/@232 contiguous). */
static const float s_identityAxes[12] = {
    1.0f, 0.0f, 0.0f, 0.0f, /* X @230 */
    0.0f, 1.0f, 0.0f, 0.0f, /* Y @231 */
    0.0f, 0.0f, 1.0f, 0.0f, /* Z @232 */
};

/* Macros (not static fns): with -inline off, static helpers become out-of-line
 * symbols; retail inlines clamp at each site. */
#define Clamp01(_p)                                                                            \
    do {                                                                                       \
        float* _vp = (_p);                                                                     \
        if (*_vp > 1.0f) {                                                                     \
            *_vp = 1.0f;                                                                       \
        } else if (*_vp < 0.0f) {                                                              \
            *_vp = 0.0f;                                                                       \
        }                                                                                      \
    } while (0)

ScreenObject::ScreenObject() {
    int i;
    int negOne;
    unsigned int objTag;

    /* Retail store order: m_ext, m_parent, m_matrixStack, m_screen, m_flags. */
    m_ext = 0;
    m_parent = 0;
    m_matrixStack = 0;
    m_screen = 0;
    m_flags = 7;
    /* Soft ceiling: focus/lastEvent loops -- mtctr/bdnz vs subic./bne; stop. */
    for (i = 0; i < 4; i++) {
        m_focus[i] = 0;
    }
    objTag = kScreenTagOBJ;
    typeTag = objTag;
    m_objTag = objTag;
    negOne = -1;
    /* Retail stfs order: +0x18, +0x14, +0x10. */
    m_extraTrans[2] = 0.0f;
    m_extraTrans[1] = 0.0f;
    m_extraTrans[0] = 0.0f;
    for (i = 0; i < 10; i++) {
        m_lastEvents[i].control = 0;
        m_lastEvents[i].lastEvent = negOne;
    }
}

ScreenObject::~ScreenObject() {
    /* Retail: optional operator delete only; Dispose is separate. */
}

void ScreenObject::CreateMatrixStack() {
    m_matrixStack = ScreenUtil::CreateMatrixStack();
    m_matrixStack->Init();
    m_matrixStack->SetIdentity();
}

void ScreenObject::Dispose() {
    if (m_matrixStack != 0) {
        m_matrixStack->Dispose();
        ScreenUtil::DestroyMatrixStack(m_matrixStack);
        m_matrixStack = 0;
    }
}

void ScreenObject::Render(ScreenRenderInfo* info) {
    SEObjectExt* ext;
    SETransform* xform;
    ScreenRenderInfo local;
    ScreenRenderInfo* pass;
    ScreenChildList* list;
    ScreenObject* child;
    unsigned int i;
    unsigned int count;

    ext = m_ext;
    if ((ext->flags & 1) == 0) {
        return;
    }

    pass = info;
    xform = ext->transform;
    if (xform != 0) {
        local.colorScale[0] = info->colorScale[0] * xform->colorScale[0];
        local.colorScale[1] = info->colorScale[1] * xform->colorScale[1];
        local.colorScale[2] = info->colorScale[2] * xform->colorScale[2];
        local.colorScale[3] = info->colorScale[3] * xform->colorScale[3];
        local.colorTranslation[0] = info->colorTranslation[0] + xform->colorTranslation[0];
        local.colorTranslation[1] = info->colorTranslation[1] + xform->colorTranslation[1];
        local.colorTranslation[2] = info->colorTranslation[2] + xform->colorTranslation[2];
        local.colorTranslation[3] = info->colorTranslation[3] + xform->colorTranslation[3];
        local.flags = info->flags | m_flags;
        local.matrixStack = m_matrixStack;
        pass = &local;
    }

    list = ext->children;
    count = (unsigned int)list->count;
    i = 0;
    while (i < count) {
        child = SeEntryObject(ScreenChildEntryAt(list, i));
        child->Render(pass);
        i += 1;
    }
    m_flags = 0;
}

void ScreenObject::SetMatrixStack(ScreenMatrixStack* stack) {
    SEObjectExt* ext;
    ScreenChildList* list;
    ScreenObject* child;
    ScreenMatrixStack* pass;
    unsigned int i;
    unsigned int count;

    /* Keep `this` and stack in NVs across the child walk (retail stmw r27). */
    pass = stack;
    ext = m_ext;
    if (ext->transform != 0) {
        if (stack != 0) {
            /* +0x18 AddChild: parent stack adopts this object's frame. */
            stack->AddChild(m_matrixStack);
        }
        pass = m_matrixStack;
    }

    list = ext->children;
    count = (unsigned int)list->count;
    i = 0;
    while (i < count) {
        child = SeEntryObject(ScreenChildEntryAt(list, i));
        child->SetMatrixStack(pass);
        i += 1;
    }
}

void ScreenObject::SetColorScale(SEVec4_t* color) {
    SETransform* xform;

    xform = m_ext->transform;
    if (xform == 0) {
        return;
    }
    xform->colorScale[0] = color->x;
    xform->colorScale[1] = color->y;
    xform->colorScale[2] = color->z;
    xform->colorScale[3] = color->w;
    /* Retail reloads transform from m_ext each clamp; keep same semantics. */
    Clamp01(&m_ext->transform->colorScale[0]);
    Clamp01(&m_ext->transform->colorScale[1]);
    Clamp01(&m_ext->transform->colorScale[2]);
    Clamp01(&m_ext->transform->colorScale[3]);
    m_flags |= 1;
}

void ScreenObject::SetColorTranslation(SEVec4_t* color) {
    SETransform* xform;

    xform = m_ext->transform;
    if (xform == 0) {
        return;
    }
    xform->colorTranslation[0] = color->x;
    xform->colorTranslation[1] = color->y;
    xform->colorTranslation[2] = color->z;
    xform->colorTranslation[3] = color->w;
    Clamp01(&m_ext->transform->colorTranslation[0]);
    Clamp01(&m_ext->transform->colorTranslation[1]);
    Clamp01(&m_ext->transform->colorTranslation[2]);
    Clamp01(&m_ext->transform->colorTranslation[3]);
    m_flags |= 1;
}

void ScreenObject::UpdateTransform() {
    SETransform* xform;
    SETransform* xformScales;
    float* pivot;
    float negPivot[3];
    /*
     * Retail spill: X @sp+0x28, Y @sp+0x18, Z @sp+0x08.
     * MWCC allocates first-declared local at highest address.
     */
    float axisX[4];
    float axisY[4];
    float axisZ[4];
    const float* src;
    const unsigned int* words;
    unsigned int* dx;
    unsigned int* dy;
    unsigned int* dz;

    /*
     * Q23: keep xform + pivot(=+0x24) live across Rotate; second reload for
     * scale loads (retail r29/r28 + r30) to stmw r28. Opcodes byte-match retail;
     * objdiff may still show ~99.65% from @230/@231/@232 vs one array label.
     */
    src = s_identityAxes;
    if (m_ext->transform != 0) {
        words = reinterpret_cast<const unsigned int*>(src);
        dx = reinterpret_cast<unsigned int*>(axisX);
        dy = reinterpret_cast<unsigned int*>(axisY);
        dz = reinterpret_cast<unsigned int*>(axisZ);
        dx[0] = words[0];
        dx[1] = words[1];
        dx[2] = words[2];
        dx[3] = words[3];
        dy[0] = words[4];
        dy[1] = words[5];
        dy[2] = words[6];
        dy[3] = words[7];
        dz[0] = words[8];
        dz[1] = words[9];
        dz[2] = words[10];
        dz[3] = words[11];

        m_matrixStack->SetIdentity();
        xform = m_ext->transform;
        pivot = xform->pivot;
        negPivot[0] = -pivot[0];
        negPivot[1] = -pivot[1];
        negPivot[2] = -pivot[2];
        m_matrixStack->Translate(reinterpret_cast<Screen3DVector*>(negPivot));
        m_matrixStack->Scale(reinterpret_cast<Screen3DVector*>(xform->rotation));
        xformScales = m_ext->transform;
        m_matrixStack->Rotate(reinterpret_cast<Screen3DVector*>(axisX), xformScales->scale[0]);
        m_matrixStack->Rotate(reinterpret_cast<Screen3DVector*>(axisY), xformScales->scale[1]);
        m_matrixStack->Rotate(reinterpret_cast<Screen3DVector*>(axisZ), xformScales->scale[2]);
        m_matrixStack->Translate(reinterpret_cast<Screen3DVector*>(pivot));
        m_matrixStack->Translate(reinterpret_cast<Screen3DVector*>(xform->translation));
        m_matrixStack->Translate(reinterpret_cast<Screen3DVector*>(m_extraTrans));
    }
    m_flags |= 2;
}

int ScreenObject::GetNumEvents() const {
    ScreenEventList* events = SeEventsOf(m_ext);

    if (events != 0) {
        return events->count;
    }
    return 0;
}

ScreenEvent* ScreenObject::GetEvent(unsigned int index) {
    ScreenEventList* events = SeEventsOf(m_ext);

    if (events != 0) {
        return ScreenEventAt(events, index);
    }
    return 0;
}

void ScreenObject::HandleEvent(ScreenMgr* /*mgr*/, int /*event*/, int /*arg*/) {}

ScreenObject* ScreenObject::GetFocus(int index) {
    return m_focus[index];
}

void ScreenObject::SetFocus(ScreenMgr* mgr, ScreenObject* obj, int index, int fireEvents) {
    ScreenObject* prev;

    prev = m_focus[index];
    if (prev == obj) {
        return;
    }
    if (obj == 0) {
        return;
    }
    if (obj->m_parent != this) {
        return;
    }

    if (prev != 0 && fireEvents != 0) {
        prev->FireEvent(mgr, SCREEN_EVENT_FOCUS_LOSS, index, 0);
    }
    m_focus[index] = obj;
    if (obj != 0 && fireEvents != 0) {
        obj->FireEvent(mgr, SCREEN_EVENT_FOCUS_GAIN, index, 0);
    }
}

void ScreenObject::ClearActiveObjects() {
    int i;

    for (i = 3; i >= 0; i--) {
        m_focus[i] = 0;
    }
}

void ScreenObject::SetParent(ScreenObject* parent) {
    m_parent = parent;
}

void ScreenObject::ProcessSubActions(ScreenMgr* mgr, const ScreenAction* action,
                                     int match) {
    ScreenActionStack* stack;
    ScreenEvent* event;
    unsigned int eventIndex;
    unsigned int flags;
    unsigned int start;
    unsigned int count;
    unsigned int end;
    unsigned int i;
    ScreenParams* params;
    unsigned int actionType;
    ScreenAction* created;
    ScreenAction* next;

    next = action->m_next;
    event = action->m_event;
    eventIndex = (unsigned int)action->m_eventIndex;
    flags = action->m_flags;
    stack = &mgr->m_actionStack;

    /* Subclass field past ScreenAction (0x3C): clear when next arg is 0x429. */
    if (next != 0 && next->m_arg == SCREEN_ACTION_ELSE) {
        static_cast<ScreenElseAction*>(next)->m_takeElse = 0;
    }

    stack->StartLocal();
    if (event->HasSubActions(eventIndex)) {
        start = event->GetStartOfSubAction(eventIndex);
        count = event->GetNumOfSubActions(eventIndex);
        end = (unsigned int)start + (unsigned int)count;

        for (i = start; i < end; i++) {
            actionType = event->GetAction(i);
            if (actionType != SCREEN_ACTION_MATCH) {
                continue;
            }
            params = event->GetParams(i);
            if (match != params->GetInt(0)) {
                continue;
            }
            actionType = event->GetAction(i);
            created = ScreenActionStack::CreateAction(actionType);
            created->Init(event, (int)i, this, (int)actionType, params, flags);
            stack->PushAction(created);
            return;
        }

        for (i = start; i < end; i++) {
            actionType = event->GetAction(i);
            params = event->GetParams(i);
            created = ScreenActionStack::CreateAction(actionType);
            created->Init(event, (int)i, this, (int)actionType, params, flags);
            stack->PushAction(created);
        }
    }
    stack->EndLocal();
}

void ScreenObject::ProcessSubActions(const ScreenAction* action, int match) {
    ProcessSubActions(m_screen->m_set->m_mgr, action, match);
}

void ScreenObject::ProcessEvent(ScreenMgr* mgr, int eventId, int arg) {
    int eventArg;
    unsigned int numEvents;
    unsigned int numActions;
    ScreenActionStack* stack;
    unsigned int i;
    int actionType;
    ScreenAction* created;
    ScreenEvent* event;
    unsigned int j;
    ScreenParams* params;

    /* Soft ceiling ~99.7%: stack/eventArg NV swap only; stop. */
    stack = &mgr->m_actionStack;
    eventArg = arg;
    numEvents = GetNumEvents();
    for (i = 0; i < numEvents; i++) {
        event = GetEvent((unsigned int)i);
        /* Retail cmpw: eventId vs m_id. */
        if (eventId != (int)event->m_id) {
            continue;
        }
        numActions = (unsigned int)event->m_numActions;
        for (j = 0; j < numActions; j++) {
            actionType = (int)event->GetAction(j);
            params = event->GetParams(j);
            created = ScreenActionStack::CreateAction((unsigned int)actionType);
            created->Init(event, (int)j, this, actionType, params,
                          (unsigned int)eventArg);
            stack->PushAction(created);
        }
    }
}

int ScreenObject::HasEvent(int eventId) {
    unsigned int numEvents;
    unsigned int i;
    ScreenEvent* event;

    /* Decl order: this/eventId live across GetEvent (retail stmw r28). */
    numEvents = (unsigned int)GetNumEvents();
    for (i = 0; i < numEvents; i++) {
        event = GetEvent(i);
        if (eventId == (int)event->m_id) {
            return 1;
        }
    }
    return 0;
}

void ScreenObject::FireEvent(ScreenMgr* mgr, int event, int arg, unsigned int flag) {
    int focusIndex;
    ScreenObject* focus;

    if ((m_ext->flags & 2) == 0 && flag == 1) {
        return;
    }

    focusIndex = arg;
    if (arg > 0 && GetFocus(arg) == 0) {
        focusIndex = 0;
    }
    focus = GetFocus(focusIndex);
    if (focus != 0) {
        focus->FireEvent(mgr, event, arg, 0);
    }
    ScreenUtil::HandleEvent(this, event, arg);
    HandleEvent(mgr, event, arg);
    ProcessEvent(mgr, event, arg);
}

void ScreenObject::BroadcastEvent(ScreenMgr* mgr, int event, int arg) {
    ScreenChildList* list;
    ScreenChildEntry* entry;
    ScreenObject* child;
    unsigned int i;
    unsigned int count;
    unsigned int tag;

    list = m_ext->children;
    if (list == 0) {
        return;
    }

    count = (unsigned int)list->count;
    i = 0;
    while (i < count) {
        entry = ScreenChildEntryAt(list, i);
        tag = entry->typeTag;
        child = SeEntryObject(entry);
        if (tag == kScreenTagOBJ || tag == kScreenTagGROP) {
            child->BroadcastEvent(mgr, event, arg);
        } else if ((event >= 0x3e8 && event < 0x3ec) || (event >= 0x407 && event < 0x409)) {
            child->ProcessEngineEvent(mgr, event);
        }
        i += 1;
    }

    HandleEvent(mgr, event, arg);
    ProcessEvent(mgr, event, arg);
}

ScreenNode* ScreenObject::FindNextFocusObject(int eventId) {
    unsigned int numEvents;
    unsigned int i;
    ScreenEvent* event;
    int j;
    int actionType;
    ScreenParams* params;

    /* Soft ceiling ~98.8%: this and reverse-index NV homes are swapped; stop. */
    numEvents = (unsigned int)GetNumEvents();
    for (i = 0; i < numEvents; i++) {
        event = GetEvent(i);
        if (eventId == (int)event->m_id) {
            for (j = event->m_numActions - 1; j >= 0; j--) {
                actionType = (int)event->GetAction((unsigned int)j);
                if (actionType == 0x3e8) {
                    params = event->GetParams((unsigned int)j);
                    return params->GetScreenNode(0);
                }
                if (actionType == SCREEN_ACTION_SET_FOCUS) {
                    params = event->GetParams((unsigned int)j);
                    return params->GetScreenNode(1);
                }
            }
        }
    }
    return 0;
}

int ScreenObject::HandleAction(ScreenMgr* /*mgr*/, const ScreenAction* /*action*/) {
    return 1;
}

int ScreenObject::GetLastEvent(ScreenAnimControl* ctrl) {
    int i;

    /* Soft ceiling: mtctr/bdnz vs indexed for; typed m_lastEvents. Soft OK. */
    for (i = 0; i < 10; i++) {
        if (m_lastEvents[i].control == ctrl) {
            return m_lastEvents[i].lastEvent;
        }
    }
    return -1;
}

void ScreenObject::SetLastEvent(ScreenAnimControl* ctrl, int event) {
    int i;

    for (i = 0; i < 10; i++) {
        if (m_lastEvents[i].control == ctrl) {
            if (event == -1) {
                m_lastEvents[i].control = 0;
            }
            m_lastEvents[i].lastEvent = event;
            return;
        }
    }
    if (event == -1) {
        return;
    }
    for (i = 0; i < 10; i++) {
        if (m_lastEvents[i].control == 0) {
            m_lastEvents[i].control = ctrl;
            m_lastEvents[i].lastEvent = event;
            return;
        }
    }
}

void ScreenObject::SetComponent(ScreenAnimControl* ctrl, float* values, int /*unused*/) {
    SETransform* xform;
    unsigned int type;
    int last;
    int asInt;

    type = ctrl->type;
    /* No early `type > 0x18` -- switch default covers it (avoids dual bgt). */
    switch (type) {
    case 0:
        xform = m_ext->transform;
        if (xform != 0 && values != 0) {
            xform->translation[0] = values[0];
            xform->translation[1] = values[1];
            xform->translation[2] = values[2];
        }
        UpdateTransform();
        break;
    case 5:
        xform = m_ext->transform;
        if (xform != 0 && values != 0) {
            xform->pivot[0] = values[0];
            xform->pivot[1] = values[1];
            xform->pivot[2] = values[2];
        }
        UpdateTransform();
        break;
    case 6:
        xform = m_ext->transform;
        if (xform != 0 && values != 0) {
            xform->scale[0] = values[0];
            xform->scale[1] = values[1];
            xform->scale[2] = values[2];
        }
        UpdateTransform();
        break;
    case 7:
        xform = m_ext->transform;
        if (xform != 0 && values != 0) {
            xform->rotation[0] = values[0];
            xform->rotation[1] = values[1];
            xform->rotation[2] = values[2];
        }
        UpdateTransform();
        break;
    case 0x14:
        /* Retail: fcmpu -> mfcr/extrwi EQ into SetVisible arg (no li 0/1). */
        SetVisible((unsigned int)(values[0] == 1.0f));
        break;
    case 0x18:
        /* Case body order: retail emits 0x18 before 0x15..0x17. */
        if (values[0] == 1.0f) {
            m_ext->flags |= 2;
        } else {
            m_ext->flags &= ~2u;
        }
        break;
    case 0x15:
        SetColorScale(reinterpret_cast<SEVec4_t*>(values));
        break;
    case 0x16:
        SetColorTranslation(reinterpret_cast<SEVec4_t*>(values));
        break;
    case 0x17:
        asInt = (int)values[0];
        last = GetLastEvent(ctrl);
        if (ctrl->flag == -1) {
            SetLastEvent(ctrl, -1);
        }
        if (asInt == -1) {
            SetLastEvent(ctrl, asInt);
            ctrl->flag = 1;
        } else if (asInt != last) {
            FireEvent(m_screen->m_set->m_mgr, asInt, 0, 0);
            SetLastEvent(ctrl, asInt);
            ctrl->flag = 1;
        }
        break;
    default:
        break;
    }
}

/* SetVisible / IsVisible: weak bodies in Glue -- not emitted in this TU. */
