#include "mwScreenEngine/ScreenAnimKey.h"

float ScreenAnimKey::GetEaseIn() { return m_easeIn; }
float ScreenAnimKey::GetEaseOut() { return m_easeOut; }

void ScreenAnimKey::GetValue(float* out) {
    int n;
    int i;
    n = m_count / 3;
    for (i = 0; i < n; i += 1) out[i] = samples[i];
}

void ScreenAnimKey::GetInTan(float* out) {
    int n;
    int i;
    float* src;
    n = m_count / 3;
    src = &samples[n];
    for (i = 0; i < n; i += 1) out[i] = src[i];
}

void ScreenAnimKey::GetOutTan(float* out) {
    int n;
    int i;
    float* src;
    n = m_count / 3;
    src = &samples[n * 2];
    for (i = 0; i < n; i += 1) out[i] = src[i];
}

int ScreenAnimKey::GetTime() { return m_time; }
unsigned int ScreenAnimKey::GetFlags() { return m_flags; }
