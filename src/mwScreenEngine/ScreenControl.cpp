#include "mwScreenEngine/ScreenControl.h"
#include "mwScreenEngine/GameVariables.h"
#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/ScreenParams.h"
#include "mwScreenEngine/ScreenUtil.h"

enum {
    kMallocTagInit = 0x494E4954
};

static const char s_ctrlDataName[] = "SS-CtrlData";

static GameVariableDispatcher s_staticDispatcher;
GameVariableDispatcher* ScreenControl::m_pGameVariables = &s_staticDispatcher;

ScreenControl::ScreenControl() {
    m_collectionId = -1;
    m_optionId = -1;
    field_0xA0 = 0;
    m_objTag = kScreenTagSCTL;
}

ScreenControl::~ScreenControl() {}

void ScreenControl::Init() {
    ScreenNode::Init();
}

void ScreenControl::Dispose() {
    ScreenObject::Dispose();
}

void ScreenControl::Render(ScreenRenderInfo* info) {
    ScreenObject::Render(info);
}

void ScreenControl::RegisterGameVariables(unsigned int,
                                          GameVariables* vars) {
    m_pGameVariables->Register(vars);
}

int ScreenControl::HandleAction(ScreenMgr* mgr, const ScreenAction* action) {
    ScreenAction local;
    ScreenParams* params;
    ScreenNode* node;
    int handled;
    int value;

    params = action->m_params;
    handled = 1;
    if (params == 0) {
        return 0;
    }

    switch (action->m_arg) {
    case kScreenArgCtrlFwdCollection:
        node = params->GetScreenNode(0);
        if (node != 0) {
            local.Init(action->m_event, action->m_eventIndex, action->m_object,
                       kScreenArgRefreshCollection, action->m_params,
                       action->m_flags);
            node->HandleAction(mgr, &local);
        }
        break;

    case kScreenArgCtrlFwdOption:
        node = params->GetScreenNode(0);
        if (node != 0) {
            local.Init(action->m_event, action->m_eventIndex, action->m_object,
                       kScreenArgRefreshOption, action->m_params,
                       action->m_flags);
            node->HandleAction(mgr, &local);
        }
        break;

    case kScreenArgCtrlRefreshSelf:
        node = params->GetScreenNode(0);
        if (node == this) {
            value = params->GetInt(1);
            if (value == 0) {
                RefreshCollection();
            } else if (value == 1) {
                RefreshOption();
            }
        } else if (node != 0) {
            node->HandleAction(mgr, action);
        }
        break;

    case kScreenArgCtrlSetId90:
        m_collectionId = params->GetInt(0);
        RefreshCollection();
        break;

    case kScreenArgCtrlSetId98:
        m_optionId = params->GetInt(0);
        RefreshOption();
        break;

    case kScreenArgCtrlRefreshAllOptions:
        RefreshAllOptions();
        break;

    case kScreenArgCtrlRefreshAllCollections:
        RefreshAllCollections();
        break;

    default:
        handled = 0;
        break;
    }

    return handled;
}

void* ScreenControl::operator new(unsigned long size) {
    return ScreenUtil::Malloc(size, kMallocTagInit, (char*)s_ctrlDataName);
}

void ScreenControl::operator delete(void* p) {
    ScreenUtil::Free(p);
}

void ScreenControl::Update() {}

static void _RefreshData(ScreenObject* obj, int refreshOptions) {
    ScreenObject* child;
    ScreenChildList* children;
    unsigned int tag;
    ScreenChildEntry* entry;
    unsigned int i;

    i = 0;
    children = obj->m_ext->children;
    while (i < (unsigned int)children->count) {
        entry = ScreenChildEntryAt(children, i);
        tag = entry->typeTag;
        if (tag == kScreenTagGROP || tag == kScreenTagOBJ) {
            child = SeEntryObject(entry);
            if (child->m_objTag == kScreenTagSCTL) {
                if (refreshOptions != 0) {
                    static_cast<ScreenControl*>(child)->RefreshOption();
                } else {
                    static_cast<ScreenControl*>(child)->RefreshCollection();
                }
            }
            _RefreshData(child, refreshOptions);
        }
        i += 1;
    }
}

void ScreenControl::RefreshAllCollections(Screen* screen) {
    ScreenObject* root;

    root = screen->GetRoot();
    if (root->m_objTag == kScreenTagSCTL) {
        static_cast<ScreenControl*>(root)->RefreshCollection();
    }
    _RefreshData(root, 0);
}

void ScreenControl::RefreshAllOptions(Screen* screen) {
    ScreenObject* root;

    root = screen->GetRoot();
    if (root->m_objTag == kScreenTagSCTL) {
        static_cast<ScreenControl*>(root)->RefreshOption();
    }
    _RefreshData(root, 1);
}

void ScreenControl::RefreshAllCollections() {
    RefreshAllCollections(m_screen);
}

void ScreenControl::RefreshAllOptions() {
    RefreshAllOptions(m_screen);
}
