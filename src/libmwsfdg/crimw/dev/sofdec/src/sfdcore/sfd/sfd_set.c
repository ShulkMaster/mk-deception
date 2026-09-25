#include "sofdec/sfd_error.h"
#include "sofdec/sfd_library.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_transport.h"

static inline int sfset_CanSet(SfdHandle* handle, int condition,
                               SfdConditionValue value)
{
    if (condition == 6 && value == 1 && SFTRN_IsSetup(handle, 3) == 0) {
        return 0;
    }
    if (condition == 5 && value == 1 && SFTRN_IsSetup(handle, 2) == 0) {
        return 0;
    }
    return 1;
}

static inline void sfset_SetCondDef(SfdHandle* handle, int condition,
                                    SfdConditionValue value)
{
    if (sfset_CanSet(handle, condition, value) != 0) {
        handle->conditions_secondary[condition] = value;
    }
}

int SFD_GetTrHn(SfdHandle* handle, int transport_index, void** output)
{
    void* context;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000117);
    }
    context = handle->transports[transport_index].context;
    if (context == 0) {
        *output = 0;
    } else {
        *output = *(void**)context;
    }
    return 0;
}

SfdConditionValue SFSET_GetCond(SfdHandle* handle, int condition)
{
    return handle->conditions_primary[condition];
}

int SFD_GetCond(SfdHandle* handle, int condition, int* value)
{
    if (handle == 0) {
        *value = SFLIB_libwork.default_conditions[condition];
    } else {
        if (SFLIB_CheckHn(handle) != 0) {
            return SFLIB_SetErr(0, 0xFF000113);
        }
        *value = handle->conditions_primary[condition];
    }
    return 0;
}

void SFSET_SetCond(SfdHandle* handle, int condition,
                   SfdConditionValue value)
{
    if (sfset_CanSet(handle, condition, value) != 0) {
        handle->conditions_primary[condition] = value;
    }
}

/* TODO: [near miss] 99.669420%; r28/r31 index/handle coloring; RE4's `ofs = id << 2`
 * byte-offset local regressed to 99.34% (ofs colored first); permuter found no honest form. */
int SFD_SetCond(SfdHandle* handle, int condition, SfdConditionValue value)
{
    int i;

    if (handle == 0) {
        SfdHandle** slot = SFLIB_libwork.handles;
        for (i = 0; i < 8; i++, slot++) {
            SfdHandle* current = *slot;
            if (current != 0) {
                SFSET_SetCond(current, condition, value);
            }
        }
        SFLIB_libwork.default_conditions[condition] = value;
    } else {
        if (SFLIB_CheckHn(handle) != 0) {
            return SFLIB_SetErr(0, 0xFF000112);
        }
        SFSET_SetCond(handle, condition, value);
        sfset_SetCondDef(handle, condition, value);
    }
    return 0;
}

int SFD_GetHnStat(SfdHandle* handle)
{
    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF000111);
    }
    return handle->playback_state;
}
