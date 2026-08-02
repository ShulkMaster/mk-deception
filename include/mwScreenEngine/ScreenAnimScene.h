#ifndef MWSCREENENGINE_SCREENANIMSCENE_H
#define MWSCREENENGINE_SCREENANIMSCENE_H

struct SERefTable;
struct SEAnimSceneData_t;
struct SEElements_t;

/* Matches ScreenAnimAction / Play mangling AnimDirectionE. */
enum AnimDirectionE {
    kAnimDirectionForward = 0,
    kAnimDirectionReverse = 1
};

/*
 * Packed ScreenAnimScene entry (stride 0x18 in ScreenAnimSceneList).
 *
 * Layout (ILP32):
 *   +0x00 m_speed
 *   +0x04 m_time
 *   +0x08 m_untilTime
 *   +0x0C m_flags
 *   +0x10 m_elements -- SEElements_t* (object table for effects)
 *   +0x14 m_data -- SEAnimSceneData_t*
 *
 * Flags @ +0x0C:
 *   0x10 playing (GetState)
 *   0x20 forward when set (SetDirection Forward=0 sets; GetDirection -> 0)
 *   0x40 play-until mode
 */
class ScreenAnimScene {
public:
    float m_speed; /* +0x00 */
    int m_time; /* +0x04 */
    float m_untilTime; /* +0x08 */
    unsigned int m_flags; /* +0x0C */
    SEElements_t* m_elements; /* +0x10 -- object table for effects */
    SEAnimSceneData_t* m_data; /* +0x14 */

    int CalculateMaxTime();
    void PlayUntilTime(int time);
    void SnapToTime(int time);
    void Process(int dt);
    int GetTime();
    void Reset();
    void Play(AnimDirectionE direction, unsigned int resetTime);
    void Stop();
    unsigned int GetState();
    void ResetTime();
    AnimDirectionE GetDirection();
    void SetDirection(AnimDirectionE direction);
    void SetSpeed(float speed);
};

#endif
