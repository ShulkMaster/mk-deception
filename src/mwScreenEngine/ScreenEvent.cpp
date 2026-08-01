#include "mwScreenEngine/ScreenEvent.h"

unsigned int ScreenEvent::HasSubActions(unsigned int index) const {
    unsigned char hasSubActions = slots[index].numOfSubActions != 0;
    return hasSubActions;
}

unsigned int ScreenEvent::GetStartOfSubAction(unsigned int index) const {
    return slots[index].startOfSubAction;
}

unsigned int ScreenEvent::GetNumOfSubActions(unsigned int index) const {
    return slots[index].numOfSubActions;
}

unsigned int ScreenEvent::GetAction(unsigned int index) const {
    return slots[index].actionType;
}

ScreenParams* ScreenEvent::GetParams(unsigned int index) const {
    return slots[index].params;
}
