#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenAnimScene.h"
#include "mwScreenEngine/ScreenAnimEffect.h"

#define ANIM_SCENE_PLAYING 0x10
#define ANIM_SCENE_FORWARD 0x20
#define ANIM_SCENE_UNTIL_TIME 0x40

int ScreenAnimScene::CalculateMaxTime() {
    int effectCount;
    int timeOffset;
    SEAnimSceneData_t* data;
    int trackIdx;
    int trackOffset;
    int effectIdx;
    int effectOffset;
    SEAnimTrack_t* track;
    int maxTime;
    SEAnimEffects_t* effects;
    int t;
    ScreenAnimEffect* effect;

    maxTime = 0;
    trackIdx = 0;
    trackOffset = 0;
    data = m_data;
    while (trackIdx < data->trackCount) {
        track = SEAnimTrackAtOffset(data, trackOffset);
        effectIdx = 0;
        effectOffset = 0;
        effects = track->effects;
        effectCount = effects->count;
        while (effectIdx < effectCount) {
            /* Retail reloads effects + timeOffset each iter before bl. */
            effects = track->effects;
            timeOffset = track->timeOffset;
            effect = SEAnimEffectAtOffset(effects, effectOffset);
            t = effect->GetMaxTime() + timeOffset;
            if (t > maxTime) {
                maxTime = t;
            }
            effectIdx += 1;
            effectOffset += 4;
        }
        trackIdx += 1;
        trackOffset += 0x0C;
    }
    data->maxTime = maxTime;
    return maxTime;
}

void ScreenAnimScene::PlayUntilTime(int time) {
    m_flags |= ANIM_SCENE_PLAYING;
    m_untilTime = (float)time;
    /* Retail: fcmpo until vs (float)m_time; ble -> clear 0x20, else set. */
    if (m_untilTime > (float)m_time) {
        m_flags |= ANIM_SCENE_FORWARD;
    } else {
        m_flags &= ~ANIM_SCENE_FORWARD;
    }
    m_flags |= ANIM_SCENE_UNTIL_TIME;
}

void ScreenAnimScene::SnapToTime(int time) {
    m_flags |= ANIM_SCENE_PLAYING;
    m_time = time;
    m_flags &= ~ANIM_SCENE_UNTIL_TIME;
    m_untilTime = 0.0f;
    Process(0);
    m_flags &= ~ANIM_SCENE_PLAYING;
}

void ScreenAnimScene::Process(int dt) {
    SEAnimSceneData_t* data;
    AnimDirectionE dir;
    int processed;
    int finished;
    int trackIdx;
    SEAnimTrack_t* track;
    SEAnimEffects_t* effects;
    int effectIdx;
    int effectCount;
    int localTime;
    int timeOffset;
    ScreenAnimEffect* effect;
    int done;

    /* Retail loads m_data before the playing check. */
    data = m_data;
    if ((m_flags & ANIM_SCENE_PLAYING) == 0) {
        return;
    }

    dir = GetDirection();
    processed = 0;
    finished = 0;

    if ((m_flags & ANIM_SCENE_FORWARD) != 0) {
        m_time += (int)((float)dt * m_speed);
        if ((m_flags & ANIM_SCENE_UNTIL_TIME) != 0) {
            if ((float)m_time > m_untilTime) {
                m_time = (int)m_untilTime;
            }
        }
    } else {
        m_time -= (int)((float)dt * m_speed);
        if ((m_flags & ANIM_SCENE_UNTIL_TIME) != 0) {
            if ((float)m_time < m_untilTime) {
                m_time = (int)m_untilTime;
            }
        }
    }

    trackIdx = 0;
    while (trackIdx < data->trackCount) {
        track = SEAnimTrackAt(data, trackIdx);
        timeOffset = track->timeOffset;
        effects = track->effects;
        effectCount = effects->count;
        localTime = m_time - timeOffset;
        effectIdx = 0;
        while (effectIdx < effectCount) {
            /* Retail reloads effects each iter then lwzx. */
            effects = track->effects;
            effect = *SEAnimEffectPtrSlot(effects, effectIdx);
            if (effect != 0) {
                processed += 1;
                done = (int)effect->Process(localTime, (int)dir, m_elements);
                if (done != 0) {
                    finished += 1;
                }
            }
            effectIdx += 1;
        }
        trackIdx += 1;
    }

    if ((m_flags & ANIM_SCENE_UNTIL_TIME) != 0) {
        if ((m_flags & ANIM_SCENE_FORWARD) != 0) {
            if ((float)m_time >= m_untilTime) {
                m_flags &= ~ANIM_SCENE_PLAYING;
                m_flags &= ~ANIM_SCENE_UNTIL_TIME;
            }
        } else {
            if ((float)m_time <= m_untilTime) {
                m_flags &= ~ANIM_SCENE_PLAYING;
                m_flags &= ~ANIM_SCENE_UNTIL_TIME;
            }
        }
    }

    if (processed == finished) {
        m_flags &= ~ANIM_SCENE_PLAYING;
    }
}

int ScreenAnimScene::GetTime() {
    return m_time;
}

void ScreenAnimScene::Reset() {
    m_flags &= ~ANIM_SCENE_PLAYING;
    m_flags &= ~ANIM_SCENE_UNTIL_TIME;
    m_untilTime = 0.0f;
}

void ScreenAnimScene::Play(AnimDirectionE direction, unsigned int resetTime) {
    m_flags |= ANIM_SCENE_PLAYING;
    m_flags &= ~ANIM_SCENE_UNTIL_TIME;
    SetDirection(direction);
    if (resetTime != 0) {
        ResetTime();
    }
    m_untilTime = 0.0f;
}

void ScreenAnimScene::Stop() {
    m_flags &= ~ANIM_SCENE_PLAYING;
    m_flags &= ~ANIM_SCENE_UNTIL_TIME;
    m_untilTime = 0.0f;
}

unsigned int ScreenAnimScene::GetState() {
    unsigned int bit = m_flags & ANIM_SCENE_PLAYING;

    return ((-bit) & ~bit) >> 31;
}

void ScreenAnimScene::ResetTime() {
    if ((int)(m_flags & ANIM_SCENE_FORWARD) > 0) {
        m_time = 0;
    } else {
        m_time = m_data->maxTime;
    }
    m_speed = 1.0f;
}

AnimDirectionE ScreenAnimScene::GetDirection() {
    return (AnimDirectionE)__rlwnm(1U, __cntlzw(m_flags & ANIM_SCENE_FORWARD), 31, 31);
}

void ScreenAnimScene::SetDirection(AnimDirectionE direction) {
    if (direction == kAnimDirectionForward) {
        m_flags |= ANIM_SCENE_FORWARD;
    } else {
        m_flags &= ~ANIM_SCENE_FORWARD;
    }
}

void ScreenAnimScene::SetSpeed(float speed) {
    m_speed = speed;
}
