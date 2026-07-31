#ifndef MWSCREENENGINE_SCREENEVENT_H
#define MWSCREENENGINE_SCREENEVENT_H
class ScreenAction;
class ScreenParams;
struct ScreenEventSlot {
    unsigned int actionType;
    unsigned short startOfSubAction;
    unsigned short numOfSubActions;
    ScreenParams* params;
    unsigned int nextLead;
};
class ScreenEvent {
public:
    int m_id;
    int m_actionCount;
    int m_numActions;
    int m_firstActionLead;
    ScreenEventSlot slots[1];
    unsigned int HasSubActions(unsigned int) const;
    unsigned int GetStartOfSubAction(unsigned int) const;
    unsigned int GetNumOfSubActions(unsigned int) const;
    unsigned int GetAction(unsigned int) const;
    ScreenParams* GetParams(unsigned int) const;
};
#endif
