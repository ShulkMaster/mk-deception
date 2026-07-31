#include "mwScreenEngine/ScreenAnimKey.h"

float ScreenAnimKey::GetEaseIn() { return m_easeIn; }
float ScreenAnimKey::GetEaseOut() { return m_easeOut; }

void ScreenAnimKey::GetValue(float* out) {
    int count;
    int three;
    int n;
    int i;
    count = m_count;
    three = 3;
    n = count / three;
    for (i = 0; i < n; i += 1) out[i] = samples[i];
}

void ScreenAnimKey::GetInTan(float* out) {
    int count;
    int three;
    int n;
    int i;
    float* src;
    count = m_count;
    three = 3;
    n = count / three;
    src = &samples[n];
    for (i = 0; i < n; i += 1) out[i] = src[i];
}

void ScreenAnimKey::GetOutTan(float* out) {
    int count;
    int three;
    int n;
    int i;
    float* src;
    count = m_count;
    three = 3;
    n = count / three;
    src = &samples[n * 2];
    for (i = 0; i < n; i += 1) out[i] = src[i];
}

int ScreenAnimKey::GetTime() { return m_time; }
unsigned int ScreenAnimKey::GetFlags() { return m_flags; }
