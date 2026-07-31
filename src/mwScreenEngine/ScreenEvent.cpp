#include "mwScreenEngine/ScreenEvent.h"

unsigned char ScreenEvent::HasSubActions(unsigned int index) const {
    return actions[index].subActionCount != 0;
}

unsigned short ScreenEvent::GetStartOfSubAction(unsigned int index) const {
    return actions[index].subActionStart;
}

unsigned short ScreenEvent::GetNumOfSubActions(unsigned int index) const {
    return actions[index].subActionCount;
}

unsigned int ScreenEvent::GetAction(unsigned int index) const {
    return actions[index].actionType;
}

ScreenParams* ScreenEvent::GetParams(unsigned int index) const {
    return actions[index].attributes;
}
