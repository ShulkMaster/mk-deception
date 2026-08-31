#include "sofdec/sfd_error.h"
#include "sofdec/sfd_library.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_transport.h"

static inline int sfset_CanSet(SfdHandle* handle, int condition, int value)
{
    if (condition == 6 && value == 1 && SFTRN_IsSetup(handle, 3) == 0) {
        return 0;
    }
    if (condition == 5 && value == 1 && SFTRN_IsSetup(handle, 2) == 0) {
        return 0;
    }
    return 1;
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

int SFSET_GetCond(SfdHandle* handle, int condition)
{
    return handle->conditions_primary[condition];
}

int SFD_GetCond(SfdHandle* handle, int condition, int* value)
{
    if (handle == 0) {
        *value = SFLIB_libwork.default_conditions[condition];
        return 0;
    }
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000113);
    }
    *value = handle->conditions_primary[condition];
    return 0;
}

void SFSET_SetCond(SfdHandle* handle, int condition, int value)
{
    if (sfset_CanSet(handle, condition, value) != 0) {
        handle->conditions_primary[condition] = value;
    }
}

int SFD_SetCond(SfdHandle* handle, int condition, int value)
{
    int i;

    if (handle == 0) {
        for (i = 0; i < 8; i++) {
            SfdHandle* current = SFLIB_libwork.handles[i];
            if (current != 0) {
                SFSET_SetCond(current, condition, value);
            }
        }
        SFLIB_libwork.default_conditions[condition] = value;
        return 0;
    }
    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000112);
    }
    SFSET_SetCond(handle, condition, value);
    if (sfset_CanSet(handle, condition, value) != 0) {
        handle->conditions_secondary[condition] = value;
    }
    return 0;
}

int SFD_GetHnStat(SfdHandle* handle)
{
    if (SFLIB_CheckHn(handle) != 0) {
        SFLIB_SetErr(0, 0xFF000111);
    }
    return handle->requested_state;
}
