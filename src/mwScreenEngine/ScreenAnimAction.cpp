#include "mwScreenEngine/ScreenAnimAction.h"
#include "mwScreenEngine/ScreenActionStack.h"
#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenMgr.h"
#include "mwScreenEngine/ScreenObject.h"
#include "mwScreenEngine/ScreenParams.h"

/* Action type ids in ScreenAction::m_arg (CreateAction / Init). */
enum {
    kArgWaitAnim = 0x400,
    kArgWaitAnimCond = 0x420,
    kAnimStatePlaying = 1,
};

static ScreenAnimScene* _GetAnimAction(ScreenParams* params, ScreenObject* object) {
    int index = params->GetInt(0);
    return object->m_screen->GetAnimScene(index);
}

ScreenPlayAnimAction::ScreenPlayAnimAction(AnimDirectionE direction) {
    m_direction = (int)direction;
}

ScreenPlayAnimAction::~ScreenPlayAnimAction() {
}

int ScreenPlayAnimAction::Update(ScreenMgr* /*mgr*/, ScreenActionStack& /*stack*/,
                                 int /*dt*/) {
    ScreenParams* params;
    ScreenAnimScene* scene;
    unsigned int resetTime;

    m_alive = 0;
    m_yield = 0;
    params = m_params;
    resetTime = 1;

    if (params == 0) {
        return 1;
    }

    scene = _GetAnimAction(params, m_object);
    if ((int)params->GetCount() > 1) {
        resetTime = (unsigned int)params->GetBoolean(1);
    }
    if (scene != 0) {
        scene->Play((AnimDirectionE)m_direction, resetTime);
    }
    return 1;
}

int ScreenSetAnimSpeedAction::Update(ScreenMgr* /*mgr*/,
                                     ScreenActionStack& /*stack*/, int /*dt*/) {
    ScreenParams* params;
    ScreenAnimScene* scene;
    float speed;

    params = m_params;
    m_alive = 0;
    m_yield = 0;

    if (params == 0) {
        return 1;
    }

    /* Retail: GetFloat always, then SetSpeed if scene non-null (one NV GPR). */
    scene = _GetAnimAction(params, m_object);
    speed = params->GetFloat(1);
    if (scene != 0) {
        scene->SetSpeed(speed);
    }
    return 1;
}

int ScreenStopAnimAction::Update(ScreenMgr* /*mgr*/, ScreenActionStack& /*stack*/,
                                 int /*dt*/) {
    ScreenParams* params;
    ScreenAnimScene* scene;

    params = m_params;
    m_alive = 0;
    m_yield = 0;

    if (params != 0) {
        scene = _GetAnimAction(params, m_object);
        if (scene != 0) {
            scene->Stop();
        }
    }
    return 1;
}

int ScreenWaitAnimAction::Update(ScreenMgr* mgr, ScreenActionStack& /*stack*/,
                                 int /*dt*/) {
    ScreenParams* params;
    ScreenAnimScene* scene;
    int playing;
    Screen* screen;
    unsigned int playingFlag;

    /*
     * Soft ceiling ~99.8%: retail clrlwi r29,r31,24 vs our r31,r31 (F coloring).
     * Tried: screen cast-reuse, unsigned char, decl order, scope screen,
     * signed flag, early-return params -- no change or regress; stop.
     */
    params = m_params;
    if (params != 0) {
        screen = m_object->m_screen;
        scene = _GetAnimAction(params, m_object);
        if (scene == 0) {
            m_alive = 0;
            m_yield = 0;
            return 1;
        }

        playing = 0;
        if ((int)scene->GetState() == kAnimStatePlaying) {
            if (mgr->GetScreenIndex(screen) != -1) {
                playing = 1;
            }
        }
        playingFlag = (unsigned int)(unsigned char)playing;

        if (m_arg == kArgWaitAnimCond) {
            m_alive = 0;
            m_yield = 0;
            if (playingFlag == (unsigned int)params->GetBoolean(1)) {
                m_object->ProcessSubActions(this, 0);
            }
        } else if (m_arg == kArgWaitAnim) {
            if (playingFlag == 0) {
                m_alive = 0;
                m_yield = 0;
                m_stepDone = 0;
                if (m_object != 0) {
                    m_object->ProcessSubActions(this, 0);
                }
            } else {
                m_alive = 1;
                m_yield = 0;
                m_stepDone = 1;
                m_blocksEvents = 0;
            }
        }
    } else {
        m_alive = 0;
        m_yield = 0;
    }
    return 1;
}

int ScreenSnapAnimAction::Update(ScreenMgr* /*mgr*/, ScreenActionStack& /*stack*/,
                                 int /*dt*/) {
    ScreenParams* params;
    ScreenAnimScene* scene;
    int time;

    params = m_params;
    if (params != 0) {
        scene = _GetAnimAction(params, m_object);
        if (scene != 0) {
            time = params->GetInt(1);
            scene->SnapToTime(time);
        }
    }

    m_alive = 0;
    m_yield = 0;
    return 1;
}

int ScreenPlayAnimUntilAction::Update(ScreenMgr* /*mgr*/,
                                      ScreenActionStack& /*stack*/, int /*dt*/) {
    ScreenParams* params;
    int time;
    ScreenAnimScene* scene;
    int maxTime;

    params = m_params;
    if (params == 0) {
        m_yield = 0;
        m_stepDone = 0;
        m_alive = 0;
        return 1;
    }

    scene = _GetAnimAction(params, m_object);
    if (scene != 0) {
        time = params->GetInt(1);
        if ((int)params->GetCount() >= 3) {
            if (params->GetBoolean(2) != 0) {
                time += scene->GetTime();
                if (time < 0) {
                    time = 0;
                } else {
                    maxTime = scene->m_data->maxTime;
                    if (time > maxTime) {
                        time = maxTime;
                    }
                }
            }
        }
        scene->PlayUntilTime(time);
    }

    m_yield = 0;
    m_stepDone = 0;
    m_alive = 0;
    return 1;
}

ScreenPlayAnimUntilAction::~ScreenPlayAnimUntilAction() {
}

ScreenSnapAnimAction::~ScreenSnapAnimAction() {
}

ScreenWaitAnimAction::~ScreenWaitAnimAction() {
}

ScreenStopAnimAction::~ScreenStopAnimAction() {
}

ScreenSetAnimSpeedAction::~ScreenSetAnimSpeedAction() {
}
