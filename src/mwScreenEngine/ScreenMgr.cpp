#include "mwScreenEngine/ScreenMgr.h"
#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/ScreenClient.h"
#include "mwScreenEngine/ScreenAction.h"

#include "runtime/cstring.h"

static unsigned int s_openEventData[7] = {
    0x430, 0, 0, 0, 0, 0, 0,
};

#define SCREEN_EVENT_LOAD 0x3E8
#define SCREEN_EVENT_UNLOAD 0x3E9
#define SCREEN_EVENT_OPEN 0x3EA
#define SCREEN_EVENT_CLOSE 0x3EB
#define SCREEN_BRANCH_CAPACITY 16
#define SCREEN_CONFIRM_CAPACITY 4

ScreenMgr::ScreenMgr() {
    m_registerCount = 0;
    m_registerTable = 0;
    m_eventLatch = 0;
    Reset();
}

ScreenMgr::~ScreenMgr() {
    if (m_registerTable != 0) {
        m_registerCount = 0;
        ScreenUtil::Free(m_registerTable);
        m_registerTable = 0;
    }
}

#pragma dont_inline on
/* TODO: [near miss] 87.79%; field stores and both clears agree; empty literal uses SDA instead of retail rodata. */
void ScreenMgr::Reset() {
    int i;
    int stack_index;

    m_rootSet = 0;
    m_currentSet = 0;
    m_eventsEnabled = 1;
    strcpy(m_pathBuf, "");
    strcpy(m_loadPath, "");
    strcpy(m_screenName, "");
    m_pendingOpen = 0;
    m_exitStagePending = 0;
    m_exitStageValue = 0;
    m_suppressDraw = 0;
    m_unk1e8 = 0;
    m_branchDepth = -1;
    m_activeCount = -1;
    m_eventLatch = 0;

    for (i = 0; i < SCREEN_CONFIRM_CAPACITY; i++) {
        m_confirm[i] = 0;
    }
    for (stack_index = 0; stack_index < SCREEN_BRANCH_CAPACITY; stack_index++) {
        m_stack[stack_index] = 0;
    }
}
#pragma dont_inline reset

void ScreenMgr::Init(ScreenClient* client) {
    ScreenUtil::SetScreenClient(client);
    Reset();
}

void ScreenMgr::Dispose(unsigned int flags) {
    if (m_rootSet != 0) {
        DisposeSet(m_rootSet, 1);
        Reset();
    }
    if (flags != 0) {
        m_actionStack.Dispose();
    }
}

Screen* ScreenMgr::GetActiveScreen() {
    if (m_activeCount >= 0) {
        return m_stack[m_activeCount];
    }
    return 0;
}

int ScreenMgr::LoadScreen(char* path, unsigned int flags) {
    Screen* found;

    if (strlen(m_loadPath) == 0) {
        strcpy(m_loadPath, path);
    }

    FindScreen(path, &found);
    m_pendingOpen = flags;

    if (found == 0) {
        int matched = UpdateBranchPath(path);
        if (matched >= m_branchDepth && (unsigned int)m_currentSet->IsInited() != 0) {
            LoadCompleted(m_currentSet);
        } else {
            if ((unsigned int)InitBranchPath() == 0) {
                return 0;
            }
        }
    } else {
        if (m_pendingOpen != 0) {
            if (found != 0) {
                AppendScreen(found);
                OpenScreen(found);
                m_pendingOpen = 0;
            }
        }
    }

    if (flags == 1) {
        m_actionStack.Process(this, 0);
    }
    return 1;
}

#pragma dont_inline on
int ScreenMgr::InitBranchPath() {
    int i;

    for (i = 0; i < m_branchDepth; i++) {
        ScreenSet* set = m_branch[i];
        if ((unsigned int)set->IsInited() == 0) {
            if ((unsigned int)set->Init() == 0) {
                return 0;
            }
        }
    }
    return 1;
}
#pragma dont_inline reset

#pragma dont_inline on
/* TODO: [near miss] 83.037598%; delimiter lifetime recovered; create/dispose register allocation remains. */
int ScreenMgr::UpdateBranchPath(char* path) {
    char pathCopy[0x100];
    char* parts[10];
    const char* delimiters;
    int nParts;
    int matched;
    int limit;
    ScreenSet* walk;
    ScreenSet* keepParent;
    ScreenSet* cur;
    ScreenSet* parent;
    ScreenSet* child;
    int depth;
    int unloadId;

    strcpy(m_pathBuf, path);
    delimiters = "/\\";
    strcpy(pathCopy, path);
    nParts = SplitPath(pathCopy, delimiters, parts, sizeof(parts) / sizeof(parts[0]));

    walk = m_rootSet;
    keepParent = 0;
    limit = nParts - 1;

    for (matched = 0; walk != 0 && matched < m_branchDepth && matched < limit;
         matched++) {
        if (stricmp(walk->GetName(), parts[matched]) != 0) {
            break;
        }
        keepParent = walk;
        walk = m_branch[matched + 1];
    }

    cur = m_currentSet;
    if (cur != 0) {
        while (cur != keepParent) {
            cur->BroadcastEvent(this, SCREEN_EVENT_UNLOAD, 0);
            parent = cur->GetParent();
            unloadId = cur->m_unloadId;
            DisposeSet(cur, 1);
            ScreenUtil::UnloadScreenSet(unloadId);
            cur = parent;
        }
        m_currentSet = keepParent;
    }

    depth = matched;
    parent = keepParent;
    child = 0;
    for (; depth < limit; depth++) {
        if (parent != 0) {
            child = parent->GetChild(parts[depth]);
        }
        if (child == 0) {
            child = new ScreenSet();
            child->SetName(parts[depth]);
            child->m_mgr = this;
            if (m_rootSet == 0) {
                m_rootSet = child;
            } else if (parent != 0) {
                parent->AddChild(child);
            }
        }
        m_branch[depth] = child;
        parent = child;
    }

    if (nParts > 0) {
        m_branchDepth = limit;
        if (m_branchDepth > 0) {
            m_currentSet = m_branch[m_branchDepth - 1];
        }
        strcpy(m_screenName, parts[m_branchDepth]);
    }

    return matched;
}
#pragma dont_inline reset

/* TODO: [breakthrough needed] 96.794868%; body closes with verified paired string-pooling mode; await object flag integration. */
void ScreenMgr::LoadCompleted(ScreenSet* set) {
    if (set == m_currentSet && set != 0) {
        Screen* screen;

        screen = set->GetScreen(m_screenName);
        if (screen == 0) {
            ScreenUtil::ReportError("Load screen failed. Screen not found",
                                    "ScreenMgr.cpp", 0x1cb);
        }
        if (m_pendingOpen != 0 && screen != 0) {
            AppendScreen(screen);
            OpenScreen(screen);
            m_pendingOpen = 0;
        }
    }
}

#pragma dont_inline on
/* TODO: [breakthrough] 94.108696%; delimiter lifetime recovered; verified pooled-string mode awaits object flag integration. */
int ScreenMgr::FindScreen(char* path, Screen** outScreen) {
    char pathCopy[0x100];
    char* parts[10];
    int depth;
    const char* delimiters = "/\\";
    int nParts;
    ScreenSet* parent;

    strcpy(pathCopy, path);
    nParts = SplitPath(pathCopy, delimiters, parts, sizeof(parts) / sizeof(parts[0]));
    *outScreen = 0;
    depth = 0;

    if (m_rootSet != 0) {
        parent = FindParent(m_rootSet, parts, nParts, depth);
        if (parent != 0 && (unsigned int)parent->IsInited() != 0) {
            *outScreen = parent->GetScreen(parts[nParts - 1]);
        }
    }
    return depth;
}
#pragma dont_inline reset

#pragma dont_inline on
ScreenSet* ScreenMgr::FindParent(ScreenSet* set, char** parts, int nParts, int& depth) {
    int i;
    int nChildren;
    ScreenSet* child;
    ScreenSet* found;

    if (set != 0 && depth < nParts && stricmp(set->GetName(), parts[depth]) == 0) {
        depth += 1;

        nChildren = set->GetNumChildren();
        i = 0;
        while (i < nChildren) {
            child = set->GetChild(i);
            found = FindParent(child, parts, nParts, depth);
            if (found != 0) {
                return found;
            }
            i += 1;
        }
        return set;
    }
    return 0;
}
#pragma dont_inline reset

int ScreenMgr::SplitPath(char* path, const char* delim, char** outParts, int maxParts) {
    int count = 0;
    char* tok = strtok(path, delim);

    while (tok != 0) {
        if (count < maxParts) {
            outParts[count++] = tok;
            tok = strtok(0, delim);
        } else {
            return count;
        }
    }
    return count;
}

int ScreenMgr::GetScreenIndex(Screen* screen) {
    int found = -1;
    int i;

    for (i = 0; i <= m_activeCount; i++) {
        if (m_stack[i] == screen) {
            found = i;
            break;
        }
    }
    return found;
}

#pragma dont_inline on
int ScreenMgr::RemoveScreen(Screen* screen) {
    int found = -1;
    unsigned int shifted = 0;
    int i;

    if (screen == 0) {
        return -1;
    }

    if (m_activeCount != -1 && screen != 0 && m_stack[m_activeCount] == screen) {
        ScreenObject* root = m_stack[m_activeCount]->GetRoot();
        if (root != 0) {
            root->ProcessEvent(this, 0x3ed, 0);
        }
    }

    for (i = 0; i <= m_activeCount; i++) {
        if (m_stack[i] == screen) {
            screen->BroadcastEvent(this, 0x408, 0);
            screen->ShutoffAnimScenes();
            found = i;
            shifted = 1;
        }
        if (shifted != 0 && i < m_activeCount) {
            m_stack[i] = m_stack[i + 1];
        }
    }

    if (shifted == 1) {
        screen->m_visible = 0;
        m_stack[m_activeCount] = 0;
        m_activeCount -= 1;
    }

    if (m_activeCount != -1) {
        m_stack[m_activeCount]->GetRoot()->ProcessEvent(this, 0x3ec, 0);
    }
    return found;
}
#pragma dont_inline reset

void ScreenMgr::RemoveTopScreen() {
    if (m_activeCount >= 0) {
        RemoveScreen(m_stack[m_activeCount]);
    }
}

void ScreenMgr::RemoveScreens(ScreenSet* set) {
    int i;

    for (i = m_activeCount; i >= 0; i--) {
        Screen* screen = m_stack[i];
        if (screen->m_set == set) {
            if (screen->m_state != 2) {
                screen->m_state = 2;
                screen->BroadcastEvent(this, 0x3eb, 0);
            }
            screen->BroadcastEvent(this, 0x408, 0);
            int j;
            for (j = i; j < m_activeCount; j++) {
                m_stack[j] = m_stack[j + 1];
            }
            m_activeCount -= 1;
        }
    }

    int start = m_activeCount + 1;
    int j;
    for (j = start; j < SCREEN_BRANCH_CAPACITY; j++) {
        m_stack[j] = 0;
    }
    m_actionStack.Process(this, 0);
    m_actionStack.RemoveActions(set);
}

#pragma dont_inline on
void ScreenMgr::AppendScreen(Screen* screen) {
    InsertScreen(screen, m_activeCount + 1);
}
#pragma dont_inline reset

void ScreenMgr::InsertScreen(Screen* screen, int index) {
    int i;
    ScreenAction* action;

    if (screen == 0) {
        return;
    }
    for (i = 0; i <= m_activeCount; i++) {
        if (m_stack[i] == screen) {
            return;
        }
    }

    screen->m_visible = 0;
    if ((unsigned int)screen->m_opened == 0) {
        screen->BroadcastEvent(this, 0x409, 0);
    }
    screen->BroadcastEvent(this, 0x407, 0);

    action = ScreenActionStack::CreateAction(0x430);
    action->Init(
        (ScreenEvent*)s_openEventData, 0x430,
        screen->GetRoot(), 0x430, 0, 0);
    m_actionStack.PushAction(action);

    if (m_activeCount != -1) {
        if (index <= m_activeCount) {
            for (i = index; i <= m_activeCount; i++) {
                m_stack[i + 1] = m_stack[i];
            }
        } else {
            ScreenObject* topRoot = m_stack[m_activeCount]->GetRoot();
            if (topRoot != 0) {
                topRoot->ProcessEvent(this, 0x3ed, 0);
            }
            ScreenObject* root = screen->GetRoot();
            if (root != 0) {
                root->ProcessEvent(this, 0x3ec, 0);
            }
        }
    }

    m_activeCount += 1;
    m_stack[index] = screen;
}

#pragma dont_inline on
void ScreenMgr::CloseScreen(Screen* screen) {
    if (screen == 0) {
        return;
    }
    screen->m_state = 2;
    screen->BroadcastEvent(this, 0x3eb, 0);
    ScreenInstancer::CloseScreen(screen);
}
#pragma dont_inline reset

Screen* ScreenMgr::CloseTopScreen() {
    Screen* top = 0;
    if (m_activeCount >= 0) {
        top = m_stack[m_activeCount];
        CloseScreen(top);
    }
    return top;
}

#pragma dont_inline on
void ScreenMgr::OpenScreen(Screen* screen) {
    if (screen == 0) {
        return;
    }
    if ((unsigned int)screen->m_opened == 0) {
        screen->BroadcastEvent(this, 0x3e8, 0);
        screen->m_opened = 1;
    }
    screen->m_state = 0;
    screen->BroadcastEvent(this, 0x3ea, 0);
}
#pragma dont_inline reset

#pragma dont_inline on
void ScreenMgr::DisposeSet(ScreenSet* set, unsigned int flags) {
    int child_index = set->GetNumChildren();
    ScreenSet* parent;
    int branch_index;

    while (child_index-- != 0) {
        ScreenSet* child = set->GetChild(child_index);
        DisposeSet(child, flags);
    }

    parent = set->GetParent();
    if ((unsigned int)set->IsInited() != 0) {
        set->BroadcastEvent(this, 0x3e9, 0);
        RemoveScreens(set);
        set->Dispose();
        if (set == m_currentSet) {
            m_currentSet = parent;
            m_branchDepth -= 1;
        }
    }

    if (flags != 0) {
        if (parent != 0) {
            parent->RemoveChild(set);
        }
        for (branch_index = 0; branch_index < m_branchDepth; branch_index++) {
            if (set == m_branch[branch_index]) {
                m_branch[branch_index] = 0;
            }
        }
        delete set;
    }
}
#pragma dont_inline reset

#pragma dont_inline on
void ScreenMgr::FireEvent(int event, int arg, unsigned int force) {
    if (force == 0) {
        if (m_actionStack.IsActionBlockingEvents() != 0 || m_eventsEnabled == 0) {
            return;
        }
    }

    if (m_eventLatch != 0) {
        m_actionStack.Process(this, 0);
    }

    if (m_activeCount >= 0) {
        m_stack[m_activeCount]->FireEvent(this, event, arg, 1);
        m_eventLatch = 1;
    }
}
#pragma dont_inline reset

/* TODO: [breakthrough needed] 81.56%; broadcast loop shape differs from retail's countdown. */
void ScreenMgr::BroadcastEvent(int event, int activeOnly, int arg) {
    int count = m_activeCount;

    if (count < 0) {
        return;
    }

    if (activeOnly != 0) {
        Screen* screen = GetActiveScreen();
        if (screen != 0) {
            screen->BroadcastEvent(this, event, arg);
        }
    } else {
        int i = count + 1;
        while (i != 0) {
            i -= 1;
            m_stack[i]->BroadcastEvent(this, event, arg);
        }
    }
}

void ScreenMgr::Idle(int dt) {
    m_eventLatch = 0;
    InitBranchPath();
    m_actionStack.Process(this, dt);

    if (m_actionStack.IsActionBlockingEvents() == 0) {
        int i;
        for (i = 0; i <= m_activeCount; i++) {
            m_stack[i]->ProcessIdleEvent(this);
        }
    }
}

void ScreenMgr::UpdateAnimations(int dt) {
    int i;

    if (m_suppressDraw == 0) {
        for (i = 0; i <= m_activeCount; i++) {
            Screen* screen = m_stack[i];
            if (screen != 0) {
                screen->UpdateSceneAnimation(dt);
            }
        }
    }
}

void ScreenMgr::Render() {
    int i;

    if (m_suppressDraw == 0) {
        ScreenUtil::PreRender();
        for (i = 0; i <= m_activeCount; i++) {
            Screen* screen = m_stack[i];
            ScreenUtil::SetCurrent(screen->m_set);
            screen->RenderAll();
            ScreenUtil::Reset();
        }
        ScreenUtil::PostRender();
    }
}

void ScreenMgr::SetConfirmUser(int index, unsigned int value, int checkCount) {
    unsigned int allSet = 1;
    int i;

    m_confirm[index] = value;

    for (i = 0; i < checkCount; i++) {
        if ((int)m_confirm[i] == 0) {
            allSet = 0;
            break;
        }
    }

    if (allSet != 0) {
        FireEvent(0x3fc, 0, 0);
    }
}

void ScreenMgr::ResetStagesTo(int value) {
    int i;

    for (i = 0; i < 4; i++) {
        m_confirm[i] = value;
    }
}

int ScreenMgr::GetStage(int index) {
    if (index > 0) {
        if (index < 4) {
            return m_confirm[index - 1];
        }
    }
    return 0;
}

void ScreenMgr::SetStage(int index, int value) {
    if (index <= 0) {
        return;
    }
    if (index >= 4) {
        return;
    }
    m_confirm[index - 1] = value;
}

/* TODO: [near miss] 86.16%; register-table walk: table reload, count register and id scheduling differ. */
int ScreenMgr::ProcessRegisterActions(const ScreenAction* action) {
    int i;
    unsigned int id;
    ScreenRegisterEntry* table;

    if (action != 0) {
        id = action->m_id;
        i = 0;
        table = m_registerTable;
        while (i < m_registerCount) {
            if (id == table[i].id) {
                if ((unsigned int)table[i].fn(action) == 1) {
                    return 1;
                }
            }
            i += 1;
        }
    }
    return 0;
}
