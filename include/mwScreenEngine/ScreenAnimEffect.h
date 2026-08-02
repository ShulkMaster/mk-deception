#ifndef MWSCREENENGINE_SCREENANIMEFFECT_H
#define MWSCREENENGINE_SCREENANIMEFFECT_H

struct SERefTable;
struct SEElements_t;
struct ScreenChildEntry;
class ScreenObject;
class ScreenAnimControl;

/*
 * One animated element binding (disc SEAnimEffect_t).
 * m_elementIndex indexes SEElements_t; m_tracks holds ScreenAnimControl*.
 *
 * Layout (ILP32, size 0x10):
 *   +0x00 m_elementIndex
 *   +0x04 .. +0x0B pad (unread by GetMaxTime/GetObject/Process/Patch)
 *   +0x0C m_tracks -- SERefTable of ScreenAnimControl*
 */
class ScreenAnimEffect {
public:
    unsigned int m_elementIndex; /* +0x00 */
    unsigned int pad04; /* +0x04 -- disc reserved; unread */
    unsigned int pad08; /* +0x08 -- disc reserved; unread */
    SERefTable* m_tracks; /* +0x0C -- ScreenAnimControl* table */

    int GetMaxTime();
    ScreenObject* GetObject(SEElements_t* elements);
    unsigned int Process(int time, int direction, SEElements_t* elements);
};

/*
 * ILP32 packed SERefTable slot: count @ +0, ScreenAnimControl* refs @ +4.
 * Macro required -- MWCC -inline off emits bl for C++ inline helpers.
 */
#define ScreenAnimControlAt(table, i) \
    ((ScreenAnimControl*)((table)->refs[(unsigned int)(i)]))

/*
 * SEElements_t child -> live ScreenObject* @ entry+0x08.
 * Prefer unsigned int* refs[index+1] form in GetObject (add+lwz, Matching);
 * this macro is the +4 packed equivalent for call sites that need it.
 */
#define ScreenAnimEffectObjectAt(elements, i) \
    ((elements)->entries[(unsigned int)(i)]->object)

#endif
