#include "dolphin/ax.h"

static AXPROFILE* __AXProfile;
static unsigned long __AXMaxProfiles;
static unsigned long __AXCurrentProfile;
static unsigned long __AXProfileInitialized;

AXPROFILE* __AXGetCurrentProfile(void)
{
    AXPROFILE* profile;

    if (__AXProfileInitialized != 0) {
        profile = &__AXProfile[__AXCurrentProfile];
        __AXCurrentProfile += 1;
        __AXCurrentProfile %= __AXMaxProfiles;
        return profile;
    }

    return 0;
}
