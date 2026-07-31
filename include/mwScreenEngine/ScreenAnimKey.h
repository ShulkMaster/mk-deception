#ifndef MWSCREENENGINE_SCREENANIMKEY_H
#define MWSCREENENGINE_SCREENANIMKEY_H
class ScreenAnimKey {
public:
    int m_time;
    unsigned int m_flags;
    float m_easeIn;
    float m_easeOut;
    unsigned int pad10;
    unsigned int pad14;
    unsigned int pad18;
    int m_count;
    float samples[1];
    float GetEaseIn();
    float GetEaseOut();
    void GetValue(float*);
    void GetInTan(float*);
    void GetOutTan(float*);
    int GetTime();
    unsigned int GetFlags();
};
#endif
