/*
 * GameVariables.o -- option/collection dispatcher (mwScreenEngine).
 *
 * Drives menu confirm (GetInt/SetInt/HandleEvent -> target_game_mode via
 * ScreenControl option ids / m_objTag 'SCtl').
 *
 * GameVariables is the abstract base; the dispatcher walks the m_next chain,
 * filters with the non-virtual range helpers, and makes ordinary virtual
 * calls on the node. Declaration order in the header is the vtable slot
 * order -- see GameVariables.h before touching it.
 */

#include "mwScreenEngine/GameVariables.h"
#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/ScreenControl.h"
#include "mwScreenEngine/ScreenNode.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenParams.h"

#define GAME_VARIABLE_ANY_OWNER ((unsigned int)-1)

enum {
    kArgRefreshOption = 0x7db,
    kArgRefreshCollection = 0x7e0,
    kArgRefreshCollectionAlt = 0x7e1,
    kInitRefreshCollection = 0x7d0,
    kInitRefreshOption = 0x7d1,
    kTagSCtl = 0x5343746c /* 'SCtl' */
};

GameVariables::GameVariables() {
    /* Retail store order: vtbl (implicit), m_next, opt*, col* -- pad14 unset. */
    m_next = 0;
    m_optMin = 0;
    m_optMax = 0;
    m_colMin = 0;
    m_colMax = 0;
}

unsigned int GameVariables::IsValidInt(int /*a*/, int /*b*/, int /*c*/) {
    return 1;
}

/* Keep range helpers out-of-line -- dispatcher walks bl these (not inline). */
#pragma dont_inline on
unsigned int GameVariables::IsValidOptionRange(unsigned int id) {
    unsigned int ok;

    ok = 0;
    if (m_optMin <= (int)id && (int)id <= m_optMax) {
        ok = 1;
    }
    return ok & 0xff;
}

unsigned int GameVariables::IsValidCollectionRange(unsigned int id) {
    unsigned int ok;

    ok = 0;
    if (m_colMin <= (int)id && (int)id <= m_colMax) {
        ok = 1;
    }
    return ok & 0xff;
}
#pragma dont_inline reset

void GameVariables::SetOptionRange(unsigned int minId, unsigned int maxId) {
    m_optMin = (int)minId;
    m_optMax = (int)maxId;
}

void GameVariables::SetCollectionRange(unsigned int minId, unsigned int maxId) {
    m_colMin = (int)minId;
    m_colMax = (int)maxId;
}

int GameVariables::HandleAction(ScreenMgr* /*mgr*/, const ScreenAction* /*action*/) {
    return 0;
}

void GameVariables::HandleEvent(ScreenObject* /*object*/, int /*event*/, int /*arg*/) {}

int GameVariables::GetRowState(int /*id*/, int /*row*/) {
    return 0;
}

void GameVariables::SetRowState(int /*id*/, int /*row*/, int /*value*/) {}

int GameVariables::GetColState(int /*id*/, int /*col*/) {
    return 0;
}

void GameVariables::SetColState(int /*id*/, int /*col*/, int /*value*/) {}

int GameVariables::IsValidOption(int /*id*/) {
    return 1;
}

GameVariableDispatcher::GameVariableDispatcher() {
    m_head = 0;
}

void GameVariableDispatcher::Register(GameVariables* vars) {
    GameVariables* cur;
    GameVariables* next;

    next = m_head;
    cur = next;
    if (next == 0) {
        m_head = vars;
        vars->m_next = 0;
        return;
    }

    while ((next = cur->m_next) != 0) {
        if (cur == vars) {
            return;
        }
        cur = next;
    }

    if (cur != vars) {
        cur->m_next = vars;
        vars->m_next = 0;
    }
}

int GameVariableDispatcher::IsValidInt(unsigned int /*a*/, unsigned int b,
                                       unsigned int /*c*/, unsigned int id,
                                       int value) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            return cur->IsValidInt(b, id, value);
        }
        cur = cur->m_next;
    }
    return 1;
}

#pragma dont_inline on
int GameVariableDispatcher::GetInt(unsigned int /*unused*/, unsigned int id) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            return cur->GetInt(id);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::SetInt(unsigned int /*unused*/, unsigned int id,
                                    int value) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            cur->SetInt(id, value);
            return;
        }
        cur = cur->m_next;
    }
}

char* GameVariableDispatcher::GetString(unsigned int /*unused*/, unsigned int id) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            return cur->GetString(id);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::GetIntArray(unsigned int /*unused*/, unsigned int id,
                                         int* out, int count) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            cur->GetIntArray(id, out, count);
            return;
        }
        cur = cur->m_next;
    }
}

void GameVariableDispatcher::SetIntArray(unsigned int /*unused*/, unsigned int id,
                                         int* values, int count) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            cur->SetIntArray(id, values, count);
            return;
        }
        cur = cur->m_next;
    }
}

void GameVariableDispatcher::SetString(unsigned int /*unused*/, unsigned int id,
                                       char* str) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            cur->SetString(id, str);
            return;
        }
        cur = cur->m_next;
    }
}

int GameVariableDispatcher::GetRowState(unsigned int /*unused*/, unsigned int id,
                                        int row) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            return cur->GetRowState(id, row);
        }
        cur = cur->m_next;
    }
    return 0;
}

int GameVariableDispatcher::GetColState(unsigned int /*unused*/, unsigned int id,
                                        int col) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            return cur->GetColState(id, col);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::SetRowState(unsigned int /*unused*/, unsigned int id,
                                         int row, int value) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            cur->SetRowState(id, row, value);
            return;
        }
        cur = cur->m_next;
    }
}

void GameVariableDispatcher::SetColState(unsigned int /*unused*/, unsigned int id,
                                         int col, int value) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            cur->SetColState(id, col, value);
            return;
        }
        cur = cur->m_next;
    }
}

int GameVariableDispatcher::IsValidOption(unsigned int /*unused*/, unsigned int id) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            return cur->IsValidOption(id);
        }
        cur = cur->m_next;
    }
    return 0;
}

int GameVariableDispatcher::GetStringCollection(unsigned int /*unused*/,
                                                unsigned int id, char*** out) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            return cur->GetStringCollection(id, out);
        }
        cur = cur->m_next;
    }
    *out = 0;
    return 0;
}

int GameVariableDispatcher::GetStringMatrixCollection(unsigned int /*unused*/,
                                                      unsigned int id, char*** out,
                                                      int& rows) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            return cur->GetStringMatrixCollection(id, out, rows);
        }
        cur = cur->m_next;
    }
    *out = 0;
    return 0;
}

void GameVariableDispatcher::FreeStringCollection(unsigned int /*unused*/,
                                                  unsigned int id, char** strings,
                                                  unsigned int count) {
    GameVariables* cur;

    if (strings == 0) {
        return;
    }
    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            cur->FreeStringCollection(id, strings, count);
            return;
        }
        cur = cur->m_next;
    }
}

int GameVariableDispatcher::GetTextureCollection(unsigned int /*unused*/, int id,
                                                 GMTextureInfo_t* out,
                                                 unsigned int& count) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange((unsigned int)id) != 0) {
            return cur->GetTextureCollection(id, out, count);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::FreeTextureCollection(unsigned int /*unused*/, int id,
                                                   GMTextureInfo_t* info) {
    GameVariables* cur;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange((unsigned int)id) != 0) {
            cur->FreeTextureCollection(id, info);
            return;
        }
        cur = cur->m_next;
    }
}
#pragma dont_inline reset

int GameVariableDispatcher::HandleAction(ScreenMgr* mgr, const ScreenAction* action) {
    ScreenAction tmp;
    ScreenParams* params;
    ScreenNode* node;
    int arg;
    GameVariables* cur;

    /* Case order follows retail .text body order, not numeric order. */
    params = action->m_params;
    if (params != 0) {
        arg = action->m_arg;
        switch (arg) {
        case kArgRefreshCollection:
            node = params->GetScreenNode(0);
            if (node != 0) {
                tmp.Init(action->m_event, action->m_eventIndex, action->m_object,
                         kInitRefreshCollection, action->m_params, action->m_flags);
                node->HandleAction(mgr, &tmp);
            }
            return 1;
        case kArgRefreshCollectionAlt:
            node = params->GetScreenNode(0);
            if (node != 0) {
                tmp.Init(action->m_event, action->m_eventIndex, action->m_object,
                         kInitRefreshOption, action->m_params, action->m_flags);
                node->HandleAction(mgr, &tmp);
            }
            return 1;
        case kArgRefreshOption:
            node = params->GetScreenNode(0);
            if (node != 0) {
                node->HandleAction(mgr, action);
            }
            return 1;
        }
    }

    cur = m_head;
    while (cur != 0) {
        cur->HandleAction(mgr, action);
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::HandleEvent(ScreenObject* object, int event, int arg) {
    unsigned int optionId;
    GameVariables* cur;
    ScreenControl* ctrl;

    /*
     * Retail order: non-null object first (tag @ +0x3C = m_objTag 'SCtl'),
     * then null-object head vcall. Confirm path: SCtl + optionId @ +0x98.
     */
    if (object != 0) {
        if (object->m_objTag == kTagSCtl) {
            ctrl = (ScreenControl*)object;
            cur = m_head;
            optionId = (unsigned int)ctrl->m_optionId;
            while (cur != 0) {
                if (cur->IsValidOptionRange(optionId) != 0) {
                    cur->HandleEvent(object, event, arg);
                    return;
                }
                cur = cur->m_next;
            }
        }
        return;
    }

    cur = m_head;
    if (cur != 0) {
        cur->HandleEvent(object, event, arg);
    }
}

int GameVariableDispatcher::GetInt(int id) {
    return GetInt(GAME_VARIABLE_ANY_OWNER, (unsigned int)id);
}

void GameVariableDispatcher::SetString(int id, char* str) {
    SetString(GAME_VARIABLE_ANY_OWNER, (unsigned int)id, str);
}

int GameVariableDispatcher::GetTextureCollection(int id, GMTextureInfo_t* out,
                                                 unsigned int& count) {
    return GetTextureCollection(GAME_VARIABLE_ANY_OWNER, id, out, count);
}

void GameVariableDispatcher::FreeTextureCollection(int id, GMTextureInfo_t* info) {
    FreeTextureCollection(GAME_VARIABLE_ANY_OWNER, id, info);
}
