#ifndef MWSCREENENGINE_SCREENEVENT_H
#define MWSCREENENGINE_SCREENEVENT_H

/*
 * One event slot (stride 0x10) starting at ScreenEvent + 0x10.
 * Retail accessors index with (i << 4) then read these fields.
 *
 * GetAction returns an action *type id* (unsigned), not a ScreenAction*.
 * CreateAction(type) builds the instance; vtbl[+0x10] then binds it.
 */

class ScreenAction;
class ScreenParams;

/*
 * Runtime view of one action (stride 0x10). Getters use event+(i<<4)+off
 * so slots[0] begins at event+0x10. Disc Patch sees SEAction_t at +0x0C
 * (lead @ +0x0C / actionType @ +0x10 / attrs@params @ +0x18).
 */
struct ScreenEventSlot {
    unsigned int actionType; /* +0x00 -- type id for CreateAction */
    unsigned short startOfSubAction; /* +0x04 */
    unsigned short numOfSubActions; /* +0x06 */
    ScreenParams* params; /* +0x08 -- SEAttributes_t* after Patch */
    unsigned int nextLead; /* +0x0C -- SEAction[i+1].lead; pad to 0x10 */
};

class ScreenEvent {
public:
    int m_id; /* +0x00 -- ProcessEvent match key */
    int m_actionCount; /* +0x04 -- PatchScreenEvent actionCount */
    int m_numActions; /* +0x08 -- ProcessEvent / FindNextFocus walk */
    int m_firstActionLead; /* +0x0C -- SEAction[0].lead */
    ScreenEventSlot slots[1]; /* @ +0x10; retail indexes via (i<<4)+base */

    /*
     * Retail callers consume these as full register-width integers. The
     * stored start/count fields remain u16; narrowing the C++ return type
     * inserts non-retail clrlwi operations in ProcessSubActions.
     */
    unsigned int HasSubActions(unsigned int index) const;
    unsigned int GetStartOfSubAction(unsigned int index) const;
    unsigned int GetNumOfSubActions(unsigned int index) const;
    unsigned int GetAction(unsigned int index) const;
    ScreenParams* GetParams(unsigned int index) const;
};

#endif
