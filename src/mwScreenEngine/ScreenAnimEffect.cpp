#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenAnimEffect.h"
#include "mwScreenEngine/ScreenAnimControl.h"
#include "mwScreenEngine/ScreenObject.h"

#define ANIM_DIRECTION_FORWARD 0
#define ANIM_DIRECTION_REVERSE 1
#define ANIM_POST_LOOP 2
#define ANIM_POST_PING_PONG 3

int ScreenAnimEffect::GetMaxTime() {
    int count;
    int maxTime;
    int i;
    int t;

    maxTime = 0;
    i = 0;
    count = (int)m_tracks->count;
    while (i < count) {
        t = ScreenAnimControlAt(m_tracks, i)->GetMaxTime();
        if (t > maxTime) {
            maxTime = t;
        }
        i += 1;
    }
    return maxTime;
}

ScreenObject* ScreenAnimEffect::GetObject(SEElements_t* elements) {
    unsigned int index = m_elementIndex;

    if (index < (unsigned int)elements->count) {
        return ScreenAnimEffectObjectAt(elements, index);
    }
    return 0;
}

unsigned int ScreenAnimEffect::Process(int time, int direction,
                                       SEElements_t* elements) {
    int done;
    int count;
    ScreenObject* obj;
    int i;
    ScreenAnimControl* ctrl;
    ScreenAnimKey* firstKey;
    float values[4];
    int n;
    int maxT;

    done = 0;
    count = 0;
    obj = GetObject(elements);
    if (obj != 0) {
        i = 0;
        count = (int)m_tracks->count;
        while (i < count) {
            ctrl = ScreenAnimControlAt(m_tracks, i);
            firstKey = ScreenAnimKeyAt(ctrl->m_keys, 0);
            n = firstKey->m_count / 3;
            ctrl->GetValue(values, time);
            obj->SetComponent(ctrl, values, n * 4);

            if (direction == ANIM_DIRECTION_FORWARD) {
                maxT = ctrl->GetMaxTime();
                if (time >= maxT && ctrl->m_postMode != ANIM_POST_LOOP &&
                    ctrl->m_postMode != ANIM_POST_PING_PONG) {
                    done += 1;
                }
            } else if (direction == ANIM_DIRECTION_REVERSE) {
                if (time < 0 && ctrl->m_postMode != ANIM_POST_LOOP &&
                    ctrl->m_postMode != ANIM_POST_PING_PONG) {
                    done += 1;
                }
            }
            i += 1;
        }
    }

    /* Retail srawi/srwi/subfc/adde boolize (same as ScreenIntegerCompare >=). */
    return done >= count;
}
