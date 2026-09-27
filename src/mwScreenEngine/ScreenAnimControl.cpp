#include "mwScreenEngine/Screen.h"
#include "mwScreenEngine/ScreenAnimControl.h"

#define ANIM_EXTRAPOLATE_LOOP 2
#define ANIM_EXTRAPOLATE_PING_PONG 3
#define ANIM_KEY_LINEAR 2
#define ANIM_KEY_HOLD 8

/* TODO: [near miss] 97.73%; key accesses and stack layout match (retail reloads m_keys->refs at each use);
 * residual is nonvolatile numbering (n r31 vs r30, offset r30 vs r31) and the blend-time scheduling. */
void ScreenAnimControl::GetValue(float* out, int time) {
    int n;
    unsigned int count;
    ScreenAnimKey* first;
    ScreenAnimKey* keyA;
    int minT;
    int maxT;
    unsigned int i;
    float t;
    float easeT;
    float valB[4];
    float valA[4];
    float inTan[4];
    float outTan[4];
    float basis[4];
    int tA;
    int tB;
    int j;

    count = m_keys->count;
    if (count == 0) {
        return;
    }

    first = ScreenAnimKeyAt(m_keys, 0);
    n = first->m_count / 3;

    if (count == 1) {
        first->GetValue(valB);
        CopyValue(out, valB, (unsigned int)n);
        return;
    }

    minT = GetMinTime();
    maxT = GetMaxTime();

    if (time < minT) {
        if (m_preMode == ANIM_EXTRAPOLATE_LOOP ||
            m_preMode == ANIM_EXTRAPOLATE_PING_PONG) {
            time = maxT + ((time - minT) % (maxT - minT));
        } else {
            ScreenAnimKeyAt(m_keys, 0)->GetValue(valB);
            CopyValue(out, valB, (unsigned int)n);
            return;
        }
    } else if (time >= maxT) {
        if (m_postMode == ANIM_EXTRAPOLATE_LOOP ||
            m_postMode == ANIM_EXTRAPOLATE_PING_PONG) {
            time = minT + ((time - minT) % (maxT - minT));
        } else {
            ScreenAnimKeyAt(m_keys, m_keys->count - 1)->GetValue(valB);
            CopyValue(out, valB, (unsigned int)n);
            return;
        }
    }

    if (ScreenAnimKeyAt(m_keys, 0)->GetTime() >= time) {
        ScreenAnimKeyAt(m_keys, 0)->GetValue(valB);
        CopyValue(out, valB, (unsigned int)n);
        return;
    }
    if (ScreenAnimKeyAt(m_keys, m_keys->count - 1)->GetTime() <= time) {
        ScreenAnimKeyAt(m_keys, m_keys->count - 1)->GetValue(valB);
        CopyValue(out, valB, (unsigned int)n);
        return;
    }

    for (i = 1; i < m_keys->count; i++) {
        if (ScreenAnimKeyAt(m_keys, i)->GetTime() <= time) {
            continue;
        }
        if ((ScreenAnimKeyAt(m_keys, i - 1)->GetFlags() & ANIM_KEY_HOLD) != 0) {
            ScreenAnimKeyAt(m_keys, i - 1)->GetValue(valB);
            CopyValue(out, valB, (unsigned int)n);
            return;
        }

        keyA = ScreenAnimKeyAt(m_keys, i - 1);
        tB = ScreenAnimKeyAt(m_keys, i)->GetTime();
        tA = keyA->GetTime();
        t = (float)(tB - tA);
        tA = keyA->GetTime();
        t = (float)(time - tA) / t;
        easeT = Ease(t, ScreenAnimKeyAt(m_keys, i - 1)->GetEaseOut(),
                     ScreenAnimKeyAt(m_keys, i)->GetEaseIn());

        ScreenAnimKeyAt(m_keys, i)->GetValue(valB);
        ScreenAnimKeyAt(m_keys, i - 1)->GetValue(valA);

        /* Linear blend when A is linear and B is linear or hold; else Hermite. */
        if (((ScreenAnimKeyAt(m_keys, i - 1)->GetFlags() & ANIM_KEY_LINEAR) != 0 &&
             (ScreenAnimKeyAt(m_keys, i)->GetFlags() & ANIM_KEY_LINEAR) != 0) ||
            ((ScreenAnimKeyAt(m_keys, i - 1)->GetFlags() & ANIM_KEY_LINEAR) != 0 &&
             (ScreenAnimKeyAt(m_keys, i)->GetFlags() & ANIM_KEY_HOLD) != 0)) {
            for (j = 0; j < n; j++) {
                out[j] = easeT * (valB[j] - valA[j]) + valA[j];
            }
            return;
        }

        ScreenAnimKeyAt(m_keys, i)->GetInTan(inTan);
        ScreenAnimKeyAt(m_keys, i - 1)->GetOutTan(outTan);
        ComputeHermiteBasis(easeT, basis);
        for (j = 0; j < n; j++) {
            out[j] = basis[0] * valA[j] + basis[1] * valB[j] +
                     basis[2] * outTan[j] + basis[3] * inTan[j];
        }
        return;
    }
}

float ScreenAnimControl::Ease(float t, float easeOut, float easeIn) {
    float sum;
    float o;
    float i;
    float k;
    float s;

    sum = easeOut + easeIn;
    if (t == 0.0f || t == 1.0f) {
        return t;
    }
    if (sum == 0.0f) {
        return t;
    }
    o = easeOut;
    i = easeIn;
    if (sum > 1.0f) {
        o = easeOut / sum;
        i = easeIn / sum;
    }
    k = 1.0f / ((2.0f - o) - i);
    if (t < o) {
        return t * (t * (k / o));
    }
    if (t < (1.0f - i)) {
        return k * ((2.0f * t) - o);
    }
    s = 1.0f - t;
    return -((s * (s * (k / i))) - 1.0f);
}

void ScreenAnimControl::ComputeHermiteBasis(float t, float* out) {
    float t2;
    float c3;
    float c2;
    float c1;
    float t3;
    float threeT2;
    float negT2;
    float h;

    /* Retail order: t2, load 3/2/1, t3, 3*t2, fnmsubs t-=2*t2, fneg -t2, h. */
    t2 = t * t;
    c3 = 3.0f;
    c2 = 2.0f;
    c1 = 1.0f;
    t3 = t2 * t;
    threeT2 = c3 * t2;
    t = t - (c2 * t2);
    negT2 = -t2;
    h = (c2 * t3) - threeT2;
    out[0] = c1 + h;
    out[1] = -h;
    out[2] = t3 + t;
    out[3] = negT2 + t3;
}

int ScreenAnimControl::GetMinTime() {
    SERefTable* keys = m_keys;
    int result;

    if (keys->count != 0) {
        result = ScreenAnimKeyAt(keys, 0)->GetTime();
    } else {
        result = 0;
    }
    return result;
}

int ScreenAnimControl::GetMaxTime() {
    unsigned int count;
    SERefTable* keys;
    int result;

    keys = m_keys;
    count = keys->count;
    if (count != 0) {
        result = ScreenAnimKeyAt(keys, count - 1)->GetTime();
    } else {
        result = 0;
    }
    return result;
}

void ScreenAnimControl::CopyValue(float* dst, float* src, unsigned int count) {
    unsigned int i;

    for (i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}
