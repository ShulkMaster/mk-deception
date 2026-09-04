#include "mwScreenEngine/ScreenScreenAction.h"
#include "mwScreenEngine/ScreenActionStack.h"
#include "mwScreenEngine/ScreenMgr.h"
#include "mwScreenEngine/ScreenParams.h"
#include "mwScreenEngine/ScreenUtil.h"
#include "mwScreenEngine/Screen.h"

extern "C" {
void* memcpy(void* dst, const void* src, unsigned long n);
}

/* Exit/special: m_arg == 0x3EE drives DisposeSet + stage latch on mgr. */
enum { kExitSpecialArg = 0x3EE };

int ScreenOpenScreenAction::Update(ScreenMgr* mgr, ScreenActionStack& stack,
                                   int /*dt*/) {
    ScreenParams* params;
    Screen* foundPhase0;
    Screen* found;
    char* name;

    params = m_params;
    m_blocksEvents = 1;

    if (m_phase == 0) {
        if (params == 0) {
            m_alive = 0;
            m_yield = 0;
            return 1;
        }

        stack.StartLocal();
        name = params->GetScreenName(0);
        memcpy(m_screenName, name, 0x80);

        foundPhase0 = 0;
        mgr->FindScreen(name, &foundPhase0);
        if (foundPhase0 != 0) {
            mgr->AppendScreen(foundPhase0);
            mgr->OpenScreen(foundPhase0);
            m_alive = 0;
            m_yield = 0;
        } else {
            m_loadResult = (unsigned int)mgr->LoadScreen(name, 0);
            m_phase += 1;
        }

        stack.EndLocal();
        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 1) {
        stack.StartLocal();
        found = 0;
        mgr->FindScreen(m_screenName, &found);
        if (found != 0) {
            mgr->AppendScreen(found);
            mgr->OpenScreen(found);
            m_alive = 0;
            m_yield = 0;
        } else if (m_loadResult == 0) {
            m_loadResult = (unsigned int)mgr->InitBranchPath();
        }
        stack.EndLocal();
    }

    return 1;
}

int ScreenExitScreenAction::Update(ScreenMgr* mgr, ScreenActionStack& stack,
                                   int /*dt*/) {
    ScreenParams* params;
    Screen* found;
    Screen* top;
    ScreenSet* set;
    char* name;

    params = m_params;
    m_blocksEvents = 1;

    if (m_phase == 0) {
        if (params == 0) {
            m_alive = 0;
            m_yield = 0;
            if (m_arg == kExitSpecialArg) {
                mgr->m_exitStageValue = 0;
                mgr->m_exitStagePending = 1;
            }
            return 0;
        }

        if (m_arg == kExitSpecialArg) {
            if ((int)params->GetCount() > 1) {
                m_stageValue = params->GetInt(1);
            } else {
                m_stageValue = 0;
            }
        }

        name = params->GetScreenName(0);
        memcpy(m_screenName, name, 0x80);

        stack.StartLocal();
        m_loadResult = 0;
        m_screen = mgr->CloseTopScreen();
        m_phase += 1;
        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 1) {
        stack.StartLocal();
        if (m_arg != kExitSpecialArg) {
            m_loadResult = (unsigned int)mgr->LoadScreen(m_screenName, 0);
        }
        m_phase += 1;
        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 2) {
        mgr->FindScreen(m_screenName, &found);
        if (found != 0) {
            stack.StartLocal();
            if (m_arg != kExitSpecialArg) {
                if (mgr->m_activeCount < 0) {
                    top = 0;
                } else {
                    top = mgr->m_stack[mgr->m_activeCount];
                }
                if (m_screen == top) {
                    mgr->RemoveTopScreen();
                }
            }

            mgr->AppendScreen(found);
            mgr->OpenScreen(found);
            stack.EndLocal();

            if (m_arg != kExitSpecialArg) {
                m_alive = 0;
                m_yield = 0;
            } else {
                m_phase += 1;
                if (stack.GetHead() != this) {
                    return 1;
                }
            }
        } else if (m_loadResult == 0) {
            m_loadResult = (unsigned int)mgr->InitBranchPath();
        }
    }

    if (m_phase == 3) {
        if (stack.GetHead() == this) {
            stack.StartLocal();
            set = m_screen->m_set;
            mgr->RemoveScreen(m_screen);
            if (m_arg == kExitSpecialArg) {
                mgr->DisposeSet(set, 0);
            }
            mgr->LoadScreen(m_screenName, 0);
            stack.EndLocal();
            m_alive = 0;
            m_yield = 0;
            mgr->m_exitStagePending = 1;
            mgr->m_exitStageValue = m_stageValue;
        }
    }

    return 1;
}

int ScreenReplaceScreenAction::Update(ScreenMgr* mgr, ScreenActionStack& stack,
                                      int /*dt*/) {
    ScreenParams* params;
    Screen* found;
    char* replaceName;

    found = 0;
    params = m_params;
    m_blocksEvents = 1;

    if (m_phase == 0) {
        replaceName = params->GetScreenName(0);
        memcpy(m_screenName, params->GetName(1), 0x80);

        mgr->FindScreen(replaceName, &found);
        m_phase += 1;

        if (found != 0) {
            stack.StartLocal();
            m_screen = found;
            mgr->CloseScreen(found);
            if ((unsigned int)stack.EndLocal() == 1u) {
                return 1;
            }
        }
    }

    if (m_phase == 1) {
        if (stack.GetHead() == this) {
            m_insertIndex = mgr->RemoveScreen(m_screen);
            stack.StartLocal();
            if (m_insertIndex == -1) {
                m_insertIndex = mgr->m_activeCount + 1;
            }

            mgr->FindScreen(m_screenName, &found);
            if (found != 0) {
                mgr->InsertScreen(found, m_insertIndex);
                mgr->OpenScreen(found);
                m_alive = 0;
                m_yield = 0;
            } else {
                mgr->LoadScreen(m_screenName, 0);
                m_phase += 1;
            }

            if ((unsigned int)stack.EndLocal() == 1u) {
                return 1;
            }
        }
    } else if (m_phase == 2) {
        if (stack.GetHead() == this) {
            mgr->FindScreen(m_screenName, &found);
            if (found != 0) {
                stack.StartLocal();
                mgr->InsertScreen(found, m_insertIndex);
                mgr->OpenScreen(found);
                stack.EndLocal();
                m_alive = 0;
                m_yield = 0;
            }
        }
    }

    return 1;
}

ScreenTransitionScreenAction::ScreenTransitionScreenAction() {
    /* Base ctor (inlined) zeros phase/flag/screen/loadResult and sets m_unk14.
     * Retail then only latches m_blocksEvents after the Transition vtable. */
    m_blocksEvents = 1;
}

int ScreenTransitionScreenAction::Update(ScreenMgr* mgr,
                                         ScreenActionStack& stack, int /*dt*/) {
    ScreenParams* params;
    Screen* foundPhase1;
    Screen* foundPhase3;
    Screen* active;
    char* name;
    char* name2;

    foundPhase1 = 0;
    params = m_params;
    m_blocksEvents = 1;

    if (m_phase == 0) {
        name = params->GetScreenName(0);
        memcpy(m_screenName, name, 0x80);
        name2 = params->GetScreenName(1);
        memcpy(m_screenName2, name2, 0x80);
        m_flag = (unsigned int)params->GetBoolean(2);

        if ((int)params->GetCount() >= 4) {
            m_preloadId = params->GetInt(3);
        } else {
            m_preloadId = -1;
        }

        stack.StartLocal();
        m_screen = mgr->CloseTopScreen();
        m_phase += 1;
        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 1) {
        stack.StartLocal();
        mgr->FindScreen(m_screenName2, &foundPhase1);
        if (foundPhase1 != 0) {
            mgr->AppendScreen(foundPhase1);
            mgr->OpenScreen(foundPhase1);
            m_phase += 1;
        }
        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 2 && stack.GetHead() == this) {
        if (m_preloadId != -1) {
            ScreenUtil::PreloadData(m_preloadId);
        }
        mgr->RemoveScreen(m_screen);
        stack.StartLocal();
        m_loadResult = (unsigned int)mgr->LoadScreen(m_screenName, 0);
        m_phase += 1;
        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 3 && stack.GetHead() == this) {
        if (m_preloadId != -1) {
            if ((unsigned int)ScreenUtil::IsPreloadDataDone(m_preloadId) == 0u) {
                return 1;
            }
        }

        stack.StartLocal();
        foundPhase3 = 0;
        mgr->FindScreen(m_screenName, &foundPhase3);
        if (foundPhase3 != 0) {
            mgr->InsertScreen(foundPhase3, mgr->m_activeCount);
            if (m_flag != 0) {
                mgr->CloseTopScreen();
            }
            m_phase += 1;
        } else if (m_loadResult == 0) {
            m_loadResult = (unsigned int)mgr->InitBranchPath();
        }
        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 4) {
        stack.StartLocal();
        mgr->RemoveTopScreen();
        active = mgr->GetActiveScreen();
        mgr->OpenScreen(active);
        stack.EndLocal();
        m_alive = 0;
        m_yield = 0;
    }

    return 1;
}

int ScreenInsertScreenAction::Update(ScreenMgr* mgr, ScreenActionStack& stack,
                                     int /*dt*/) {
    ScreenParams* params;
    Screen* insertScreen;
    Screen* anchor;

    insertScreen = 0;
    anchor = 0;
    params = m_params;
    m_blocksEvents = 1;

    if (m_phase == 0) {
        memcpy(m_anchorName, params->GetScreenName(0), 0x80);
        memcpy(m_screenName, params->GetScreenName(1), 0x80);
        m_insertMode = params->GetInt(2);

        mgr->FindScreen(m_anchorName, &anchor);
        if (anchor == 0) {
            m_alive = 0;
            m_yield = 0;
        }

        mgr->FindScreen(m_screenName, &insertScreen);
        if (insertScreen == 0) {
            m_loadResult = (unsigned int)mgr->LoadScreen(m_screenName, 0);
        } else {
            m_loadResult = 1;
        }
        m_phase += 1;
    }

    if (m_phase == 1) {
        mgr->FindScreen(m_screenName, &insertScreen);
        if (insertScreen != 0) {
            mgr->FindScreen(m_anchorName, &anchor);
            m_stackIndex = mgr->GetScreenIndex(anchor);
            if (m_insertMode == 0) {
                m_stackIndex += 1;
            }

            stack.StartLocal();
            mgr->InsertScreen(insertScreen, m_stackIndex);
            mgr->OpenScreen(insertScreen);
            stack.EndLocal();
            m_alive = 0;
            m_yield = 0;
        } else if (m_loadResult == 0) {
            m_loadResult = (unsigned int)mgr->InitBranchPath();
        }
    }

    return 1;
}

int ScreenCloseScreenAction::Update(ScreenMgr* mgr, ScreenActionStack& stack,
                                    int /*dt*/) {
    ScreenParams* params;
    char* name;

    m_blocksEvents = 1;

    if (m_phase == 0) {
        params = m_params;
        stack.StartLocal();

        if (params != 0) {
            name = params->GetScreenName(0);
            mgr->FindScreen(name, &m_screen);
            /* Shared fail path: null screen or not on stack. */
            if (m_screen != 0 && mgr->GetScreenIndex(m_screen) != -1) {
                mgr->CloseScreen(m_screen);
            } else {
                m_alive = 0;
                m_yield = 0;
            }
            m_phase += 1;
        } else {
            m_alive = 0;
            m_yield = 0;
        }

        if ((unsigned int)stack.EndLocal() == 1u) {
            return 1;
        }
    }

    if (m_phase == 1) {
        if (stack.GetHead() == this) {
            stack.StartLocal();
            if (m_screen != 0) {
                mgr->RemoveScreen(m_screen);
            }
            stack.EndLocal();
        }
        m_alive = 0;
        m_yield = 0;
    }

    return 1;
}
