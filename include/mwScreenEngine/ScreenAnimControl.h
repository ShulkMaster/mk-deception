#ifndef MWSCREENENGINE_SCREENANIMCONTROL_H
#define MWSCREENENGINE_SCREENANIMCONTROL_H

#include "mwScreenEngine/ScreenAnimKey.h"

struct SERefTable;

/*
 * Per-track animation controller (disc SEAnimEffectItem_t, keys @ +0x18).
 * SetComponent reads type/flag; GetValue interpolates keyframes.
 *
 * Layout:
 *   +0x00 type / +0x04 flag
 *   +0x08 .. +0x0F pad (no Control/SetComponent readers)
 *   +0x10 m_preMode / +0x14 m_postMode (2/3 = loop)
 *   +0x18 m_keys -- SERefTable of ScreenAnimKey* (ILP32 packed +4)
 */
class ScreenAnimControl {
public:
    unsigned int type; /* +0x00 -- SetComponent switch key */
    int flag; /* +0x04 */
    unsigned int pad08; /* +0x08 -- disc reserved; unread */
    unsigned int pad0C; /* +0x0C -- disc reserved; unread */
    int m_preMode; /* +0x10 -- wrap before min (2/3 = loop) */
    int m_postMode; /* +0x14 -- wrap after max (2/3 = loop) */
    SERefTable* m_keys; /* +0x18 -- ScreenAnimKey* table */

    /* out = interpolated sample vector; time = keyframe clock. */
    void GetValue(float* out, int time);
    float Ease(float t, float easeOut, float easeIn);
    void ComputeHermiteBasis(float t, float* out);
    int GetMinTime();
    int GetMaxTime();
    void CopyValue(float* dst, float* src, unsigned int count);
};

/*
 * ILP32 packed SERefTable: count @ +0, ScreenAnimKey* refs @ +4.
 * Macro - MWCC will not inline a C++ helper here (emits bl).
 */
#define ScreenAnimKeyAt(table, i) \
    ((ScreenAnimKey*)((table)->refs[(unsigned int)(i)]))


#endif
