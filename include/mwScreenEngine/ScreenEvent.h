#ifndef MWSCREENENGINE_SCREENEVENT_H
#define MWSCREENENGINE_SCREENEVENT_H
class ScreenParams;

struct SEAction_t {
    unsigned int lead;             /* +0x00 */
    unsigned int actionType;       /* +0x04 */
    unsigned short subActionStart; /* +0x08 */
    unsigned short subActionCount; /* +0x0A */
    ScreenParams* attributes;      /* +0x0C */
};

class ScreenEvent {
public:
    unsigned int eventID;     /* +0x00 */
    unsigned int actionCount; /* +0x04 -- patched SEAction_t count */
    unsigned int numActions;  /* +0x08 -- runtime action count */
    SEAction_t actions[1];     /* +0x0C */

    unsigned char HasSubActions(unsigned int index) const;
    unsigned short GetStartOfSubAction(unsigned int index) const;
    unsigned short GetNumOfSubActions(unsigned int index) const;
    unsigned int GetAction(unsigned int index) const;
    ScreenParams* GetParams(unsigned int index) const;
};
#endif
