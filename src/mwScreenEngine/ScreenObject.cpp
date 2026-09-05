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

/* Retail stores each constant rotation axis as four floats. */
struct ScreenRotationAxis {
    Screen3DVector vector;
    float w;
};

static const ScreenRotationAxis s_identityAxes[3] = {
    { { 1.0f, 0.0f, 0.0f }, 0.0f },
    { { 0.0f, 1.0f, 0.0f }, 0.0f },
    { { 0.0f, 0.0f, 1.0f }, 0.0f },
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

/* TODO: [near miss] 94.78723%; constant and event-loop GPR coloring remains; counted loops agree. */
ScreenObject::ScreenObject() {
    int i;

    /* Retail store order: m_ext, m_parent, m_matrixStack, m_screen, m_flags. */
    m_ext = 0;
    m_parent = 0;
    m_matrixStack = 0;
    m_screen = 0;
    m_flags = 7;
    for (i = 0; i < 4; i++) {
        m_focus[i] = 0;
    }
    typeTag = kScreenTagOBJ;
    m_objTag = kScreenTagOBJ;
    /* Retail stfs order: +0x18, +0x14, +0x10. */
    m_extraTrans[2] = 0.0f;
    m_extraTrans[1] = 0.0f;
    m_extraTrans[0] = 0.0f;
    for (i = 0; i < 10; i++) {
        m_lastEvents[i].control = 0;
        m_lastEvents[i].lastEvent = -1;
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
    ScreenChildList* list;
    ScreenObject* child;
    unsigned int count;
    unsigned int i;

    ext = m_ext;
    if ((ext->flags & 1) == 0) {
        return;
    }

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
        info = &local;
    }

    list = m_ext->children;
    count = (unsigned int)list->count;
    i = 0;
    while (i < count) {
        child = SeEntryObject(ScreenChildEntryAt(list, i));
        child->Render(info);
        i += 1;
    }
    m_flags = 0;
}

void ScreenObject::SetMatrixStack(ScreenMatrixStack* stack) {
    SEObjectExt* ext;
    ScreenChildList* list;
    ScreenObject* child;
    unsigned int count;
    unsigned int i;

    ext = m_ext;
    if (ext->transform != 0) {
        if (stack != 0) {
            /* +0x18 AddChild: parent stack adopts this object's frame. */
            stack->AddChild(m_matrixStack);
        }
        stack = m_matrixStack;
    }

    list = m_ext->children;
    count = (unsigned int)list->count;
    i = 0;
    while (i < count) {
        child = SeEntryObject(ScreenChildEntryAt(list, i));
        child->SetMatrixStack(stack);
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

/* TODO: [near miss] 99.64602%; anonymous axis relocation labels differ; data-value match is exact. */
void ScreenObject::UpdateTransform() {
    SETransform* xform;
    SETransform* xformScales;
    Screen3DVector* pivot;
    Screen3DVector negPivot;
    const ScreenRotationAxis* axes = s_identityAxes;
    if (m_ext->transform != 0) {
        ScreenRotationAxis axisX = axes[0];
        ScreenRotationAxis axisY = axes[1];
        ScreenRotationAxis axisZ = axes[2];

        m_matrixStack->SetIdentity();
        xform = m_ext->transform;
        pivot = &xform->pivotVector;
        negPivot.x = -pivot->x;
        negPivot.y = -pivot->y;
        negPivot.z = -pivot->z;
        m_matrixStack->Translate(&negPivot);
        m_matrixStack->Scale(&xform->rotationVector);
        xformScales = m_ext->transform;
        m_matrixStack->Rotate(&axisX.vector, xformScales->scale[0]);
        m_matrixStack->Rotate(&axisY.vector, xformScales->scale[1]);
        m_matrixStack->Rotate(&axisZ.vector, xformScales->scale[2]);
        m_matrixStack->Translate(pivot);
        m_matrixStack->Translate(&xform->translationVector);
        m_matrixStack->Translate(&m_extraTranslation);
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

/* TODO: [near miss] 98.666664%; zero/index initialization ordering remains; reverse walk agrees. */
void ScreenObject::ClearActiveObjects() {
    int i;

    for (i = 3; i >= 0; i--) {
        m_focus[i] = 0;
    }
}

void ScreenObject::SetParent(ScreenObject* parent) {
    m_parent = parent;
}

/* TODO: [near miss] 96.40367%; action-result copy and GPR coloring remain; stop at equivalent action traversal. */
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
    Screen* screen;
    ScreenSet* set;
    ScreenMgr* mgr;

    screen = m_screen;
    set = screen->m_set;
    mgr = set->m_mgr;
    ProcessSubActions(mgr, action, match);
}

/* TODO: [near miss] 99.666664%; action-stack and event-argument GPR homes remain swapped. */
void ScreenObject::ProcessEvent(ScreenMgr* mgr, int eventId, int arg) {
    unsigned int numEvents;
    unsigned int numActions;
    ScreenActionStack* stack;
    unsigned int i;
    int actionType;
    ScreenAction* created;
    ScreenEvent* event;
    unsigned int j;
    ScreenParams* params;

    stack = &mgr->m_actionStack;
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
                          (unsigned int)arg);
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

/* TODO: [near miss] 99.49275%; entry/tag scratch GPR coloring remains; finite event switch agrees. */
void ScreenObject::BroadcastEvent(ScreenMgr* mgr, int event, int arg) {
    ScreenChildList* list;
    ScreenChildEntry* entry;
    unsigned int count;
    unsigned int i;
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
        if (tag == kScreenTagOBJ || tag == kScreenTagGROP) {
            SeEntryObject(entry)->BroadcastEvent(mgr, event, arg);
        } else {
            switch (event) {
            case 0x3e8:
            case 0x3e9:
            case 0x3ea:
            case 0x3eb:
            case 0x407:
            case 0x408:
                SeEntryObject(entry)->ProcessEngineEvent(mgr, event);
                break;
            }
        }
        i += 1;
    }

    HandleEvent(mgr, event, arg);
    ProcessEvent(mgr, event, arg);
}

/* TODO: [near miss] 99.803925%; this/reverse-index GPR coloring remains; stop at equivalent traversal. */
ScreenObject* ScreenObject::FindNextFocusObject(int eventId) {
    unsigned int numEvents;
    unsigned int i;
    ScreenEvent* event;
    int j;
    int actionType;
    ScreenParams* params;

    numEvents = (unsigned int)GetNumEvents();
    for (i = 0; i < numEvents; i++) {
        event = GetEvent(i);
        if (eventId == (int)event->m_id) {
            for (j = event->m_numActions - 1; j >= 0; j--) {
                actionType = (int)event->GetAction((unsigned int)j);
                if (actionType == 0x3e8) {
                    params = event->GetParams((unsigned int)j);
                    return params->GetScreenObject(0);
                }
                if (actionType == SCREEN_ACTION_SET_FOCUS) {
                    params = event->GetParams((unsigned int)j);
                    return params->GetScreenObject(1);
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
    case 0x17: {
        Screen* screen;
        ScreenSet* set;
        ScreenMgr* mgr;

        asInt = (int)values[0];
        last = GetLastEvent(ctrl);
        if (ctrl->flag == -1) {
            SetLastEvent(ctrl, -1);
        }
        if (asInt == -1) {
            SetLastEvent(ctrl, asInt);
            ctrl->flag = 1;
        } else if (asInt != last) {
            screen = m_screen;
            set = screen->m_set;
            mgr = set->m_mgr;
            FireEvent(mgr, asInt, 0, 0);
            SetLastEvent(ctrl, asInt);
            ctrl->flag = 1;
        }
        break;
    }
    default:
        break;
    }
}

/* SetVisible / IsVisible: weak bodies in Glue -- not emitted in this TU. */
