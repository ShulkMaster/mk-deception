/*
 * GameVariables.o -- option/collection dispatcher (mwScreenEngine).
 *
 * NonMatching callable for menu confirm (GetInt/SetInt/HandleEvent ->
 * target_game_mode via ScreenControl option ids / m_objTag 'SCtl').
 *
 * Soft ceilings (walkers / HandleAction):
 *   GetInt/SetInt/SetString ~90.1% -- vtbl load temp (r5/r6 vs r12)
 *   GetIntArray/SetIntArray ~90.5% -- vtbl load temp + arg order
 *   HandleAction ~87.6% -- retail binary cmp tree vs sequential ifs
 *   HandleEvent ~89.3% -- vtbl temp coloring
 *   Register ~94.8% -- lwz r0/mr r5 vs lwz r5
 * Algo matches; stop Matching-grind on reg coloring / branch schedule.
 */

#include "mwScreenEngine/GameVariables.h"
#include "mwScreenEngine/ScreenAction.h"
#include "mwScreenEngine/ScreenControl.h"
#include "mwScreenEngine/ScreenNode.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenParams.h"

extern void* __vt__13GameVariables;

#define GAME_VARIABLE_ANY_OWNER ((unsigned int)-1)

enum {
    kArgRefreshOption = 0x7db,
    kArgRefreshCollection = 0x7e0,
    kArgRefreshCollectionAlt = 0x7e1,
    kInitRefreshCollection = 0x7d0,
    kInitRefreshOption = 0x7d1,
    kTagSCtl = 0x5343746c /* 'SCtl' */
};

/* Vtbl word indices (retail __vt__13GameVariables). */
enum {
    kVtHandleAction = 4, /* +0x10 */
    kVtHandleEvent = 5, /* +0x14 */
    kVtGetInt = 6, /* +0x18 */
    kVtIsValidInt = 7, /* +0x1C */
    kVtSetInt = 8, /* +0x20 */
    kVtGetString = 11, /* +0x2C */
    kVtSetString = 12, /* +0x30 */
    kVtGetIntArray = 13, /* +0x34 */
    kVtSetIntArray = 14, /* +0x38 */
    kVtIsValidOption = 15, /* +0x3C */
    kVtGetRowState = 16, /* +0x40 */
    kVtSetRowState = 17, /* +0x44 */
    kVtGetColState = 18, /* +0x48 */
    kVtSetColState = 19, /* +0x4C */
    kVtGetStringCollection = 20, /* +0x50 */
    kVtGetStringMatrix = 21, /* +0x54 */
    kVtFreeStringCollection = 22, /* +0x58 */
    kVtGetTextureCollection = 24, /* +0x60 */
    kVtFreeTextureCollection = 25 /* +0x64 */
};

/* Typed vtbl word table -- prefer over repeated (void**)m_vtbl cast soup. */
static inline void** GameVariablesVtbl(GameVariables* gv) {
    return (void**)gv->m_vtbl;
}

typedef int (*GV_HandleActionFn)(GameVariables*, ScreenMgr*, const ScreenAction*);
typedef void (*GV_HandleEventFn)(GameVariables*, ScreenObject*, int, int);
typedef int (*GV_GetIntFn)(GameVariables*, unsigned int);
typedef int (*GV_IsValidIntFn)(GameVariables*, unsigned int, unsigned int, int);
typedef void (*GV_SetIntFn)(GameVariables*, unsigned int, int);
typedef char* (*GV_GetStringFn)(GameVariables*, unsigned int);
typedef void (*GV_GetIntArrayFn)(GameVariables*, unsigned int, int*, int);
typedef void (*GV_SetIntArrayFn)(GameVariables*, unsigned int, int*, int);
typedef void (*GV_SetStringFn)(GameVariables*, unsigned int, char*);
typedef int (*GV_IsValidOptionFn)(GameVariables*, unsigned int);
typedef int (*GV_GetRowStateFn)(GameVariables*, unsigned int, int);
typedef void (*GV_SetRowStateFn)(GameVariables*, unsigned int, int, int);
typedef int (*GV_GetColStateFn)(GameVariables*, unsigned int, int);
typedef void (*GV_SetColStateFn)(GameVariables*, unsigned int, int, int);
typedef int (*GV_GetStringCollectionFn)(GameVariables*, unsigned int, char***);
typedef int (*GV_GetStringMatrixFn)(GameVariables*, unsigned int, char***, int*);
typedef void (*GV_FreeStringCollectionFn)(GameVariables*, unsigned int, char**,
                                          unsigned int);
typedef int (*GV_GetTextureCollectionFn)(GameVariables*, int, GMTextureInfo_t*,
                                         unsigned int*);
typedef void (*GV_FreeTextureCollectionFn)(GameVariables*, int, GMTextureInfo_t*);

GameVariables::GameVariables() {
    /* Retail store order: vtbl, m_next, opt*, col* -- pad14 unset. */
    m_vtbl = &__vt__13GameVariables;
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

int GameVariables::GetRowState(int /*a*/, int /*b*/) {
    return 0;
}

void GameVariables::SetRowState(int /*a*/, int /*b*/, int /*c*/) {}

int GameVariables::GetColState(int /*a*/, int /*b*/) {
    return 0;
}

void GameVariables::SetColState(int /*a*/, int /*b*/, int /*c*/) {}

int GameVariables::IsValidOption(int /*id*/) {
    return 1;
}

GameVariableDispatcher::GameVariableDispatcher() {
    m_head = 0;
}

void GameVariableDispatcher::Register(GameVariables* vars) {
    GameVariables* cur;

    /* Soft ceiling: Register ~94.8% -- lwz r0/mr r5 vs lwz r5; stop. */
    cur = m_head;
    if (cur == 0) {
        m_head = vars;
        vars->m_next = 0;
        return;
    }

    while (cur->m_next != 0) {
        if (cur == vars) {
            return;
        }
        cur = cur->m_next;
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
            return ((GV_IsValidIntFn)GameVariablesVtbl(cur)[kVtIsValidInt])(cur, b, id,
                                                                            value);
        }
        cur = cur->m_next;
    }
    return 1;
}

#pragma dont_inline on
int GameVariableDispatcher::GetInt(unsigned int /*unused*/, unsigned int id) {
    GameVariables* cur;
    void** vtbl;

    /* Soft ceiling: GetInt ~90.1% -- vtbl load temp r5 vs r12; stop. */
    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_GetIntFn)vtbl[kVtGetInt])(cur, id);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::SetInt(unsigned int /*unused*/, unsigned int id,
                                    int value) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_SetIntFn)vtbl[kVtSetInt])(cur, id, value);
            return;
        }
        cur = cur->m_next;
    }
}

char* GameVariableDispatcher::GetString(unsigned int /*unused*/, unsigned int id) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_GetStringFn)vtbl[kVtGetString])(cur, id);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::GetIntArray(unsigned int /*unused*/, unsigned int id,
                                         int* out, int count) {
    GameVariables* cur;
    void** vtbl;

    /* Soft ceiling: GetIntArray ~90.5% -- vtbl load temp + arg order; stop. */
    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_GetIntArrayFn)vtbl[kVtGetIntArray])(cur, id, out, count);
            return;
        }
        cur = cur->m_next;
    }
}

void GameVariableDispatcher::SetIntArray(unsigned int /*unused*/, unsigned int id,
                                         int* values, int count) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_SetIntArrayFn)vtbl[kVtSetIntArray])(cur, id, values, count);
            return;
        }
        cur = cur->m_next;
    }
}

void GameVariableDispatcher::SetString(unsigned int /*unused*/, unsigned int id,
                                       char* str) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_SetStringFn)vtbl[kVtSetString])(cur, id, str);
            return;
        }
        cur = cur->m_next;
    }
}

int GameVariableDispatcher::GetRowState(unsigned int /*unused*/, unsigned int id,
                                        int row) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_GetRowStateFn)vtbl[kVtGetRowState])(cur, id, row);
        }
        cur = cur->m_next;
    }
    return 0;
}

int GameVariableDispatcher::GetColState(unsigned int /*unused*/, unsigned int id,
                                        int col) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_GetColStateFn)vtbl[kVtGetColState])(cur, id, col);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::SetRowState(unsigned int /*unused*/, unsigned int id,
                                         int row, int value) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_SetRowStateFn)vtbl[kVtSetRowState])(cur, id, row, value);
            return;
        }
        cur = cur->m_next;
    }
}

void GameVariableDispatcher::SetColState(unsigned int /*unused*/, unsigned int id,
                                         int col, int value) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_SetColStateFn)vtbl[kVtSetColState])(cur, id, col, value);
            return;
        }
        cur = cur->m_next;
    }
}

int GameVariableDispatcher::IsValidOption(unsigned int /*unused*/, unsigned int id) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidOptionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_IsValidOptionFn)vtbl[kVtIsValidOption])(cur, id);
        }
        cur = cur->m_next;
    }
    return 0;
}

int GameVariableDispatcher::GetStringCollection(unsigned int /*unused*/,
                                                unsigned int id, char*** out) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_GetStringCollectionFn)vtbl[kVtGetStringCollection])(cur, id,
                                                                             out);
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
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_GetStringMatrixFn)vtbl[kVtGetStringMatrix])(cur, id, out,
                                                                    &rows);
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
    void** vtbl;

    if (strings == 0) {
        return;
    }
    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange(id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_FreeStringCollectionFn)vtbl[kVtFreeStringCollection])(cur, id, strings,
                                                                       count);
            return;
        }
        cur = cur->m_next;
    }
}

int GameVariableDispatcher::GetTextureCollection(unsigned int /*unused*/, int id,
                                                 GMTextureInfo_t* out,
                                                 unsigned int& count) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange((unsigned int)id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            return ((GV_GetTextureCollectionFn)vtbl[kVtGetTextureCollection])(
                cur, id, out, &count);
        }
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::FreeTextureCollection(unsigned int /*unused*/, int id,
                                                   GMTextureInfo_t* info) {
    GameVariables* cur;
    void** vtbl;

    cur = m_head;
    while (cur != 0) {
        if (cur->IsValidCollectionRange((unsigned int)id) != 0) {
            vtbl = GameVariablesVtbl(cur);
            ((GV_FreeTextureCollectionFn)vtbl[kVtFreeTextureCollection])(cur, id, info);
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

    /*
     * Soft ceiling: HandleAction ~87.6% -- sequential ifs vs retail binary
     * cmp tree (0x7E0 / 0x7E1 / 0x7DB); body order matches .text. Stop.
     */
    params = action->m_params;
    if (params != 0) {
        arg = action->m_arg;
        if (arg == kArgRefreshCollection) {
            node = params->GetScreenNode(0);
            if (node != 0) {
                tmp.Init(action->m_event, action->m_eventIndex, action->m_object,
                         kInitRefreshCollection, action->m_params, action->m_flags);
                node->HandleAction(mgr, &tmp);
            }
            return 1;
        }
        if (arg == kArgRefreshCollectionAlt) {
            node = params->GetScreenNode(0);
            if (node != 0) {
                tmp.Init(action->m_event, action->m_eventIndex, action->m_object,
                         kInitRefreshOption, action->m_params, action->m_flags);
                node->HandleAction(mgr, &tmp);
            }
            return 1;
        }
        if (arg == kArgRefreshOption) {
            node = params->GetScreenNode(0);
            if (node != 0) {
                node->HandleAction(mgr, action);
            }
            return 1;
        }
    }

    cur = m_head;
    while (cur != 0) {
        ((GV_HandleActionFn)GameVariablesVtbl(cur)[kVtHandleAction])(cur, mgr, action);
        cur = cur->m_next;
    }
    return 0;
}

void GameVariableDispatcher::HandleEvent(ScreenObject* object, int event, int arg) {
    GameVariables* cur;
    ScreenControl* ctrl;
    unsigned int optionId;
    void** vtbl;

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
                    vtbl = GameVariablesVtbl(cur);
                    ((GV_HandleEventFn)vtbl[kVtHandleEvent])(cur, object, event, arg);
                    return;
                }
                cur = cur->m_next;
            }
        }
        return;
    }

    cur = m_head;
    if (cur != 0) {
        vtbl = GameVariablesVtbl(cur);
        ((GV_HandleEventFn)vtbl[kVtHandleEvent])(cur, object, event, arg);
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
